/*
 * Copyright (C) 2021  Alfred Neumayer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * pvms is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QTimer>

#include "machine.h"

Machine::Machine()
{
    this->m_process = new QProcess(this);
    QObject::connect(this, &Machine::started, this, [=](){
        if (this->running)
            return;
        this->running = true;
        emit runningChanged();
    });
    QObject::connect(this, &Machine::stopped, this, [=](){
        if (!this->running)
            return;
        this->running = false;
        emit runningChanged();
    });
}

Machine::~Machine()
{
    stop();
}

void Machine::start()
{
    if (this->m_process->state() == QProcess::Running) {
        qWarning() << "VM process already running";
        return;
    }

    const QStringList vmArgs = getLaunchArguments();

    const QString pwd = QCoreApplication::applicationDirPath();
    const QString qemuBin = QStringLiteral("%1/bin/qemu-system-%2").arg(pwd, this->arch);

    QStringList args;
    if (hasKvm() && canVirtualize())
        args << QStringLiteral("-enable-kvm");
    args << vmArgs;

    qDebug() << "Start:" << qemuBin << args;

    this->m_process->start(qemuBin, args);
    this->m_process->waitForStarted();
    if (this->m_process->state() != QProcess::Running) {
        qWarning() << "Starting machine failed:" << this->m_process->readAllStandardError();
        emit stopped();
        return;
    }

    QTimer::singleShot(1000, this, [=](){
        emit started();
    });
}

void Machine::stop()
{
    if (this->m_process->state() == QProcess::NotRunning) {
        qWarning() << "VM process already stopped";
        return;
    }

    this->m_process->kill();
    this->m_process->waitForFinished();
    emit stopped();
}

QStringList Machine::getLaunchArguments()
{
    QStringList ret;

    // Machine setup
    ret << QStringLiteral("-smp") << QString::number(this->cores);
    ret << QStringLiteral("-m") << QStringLiteral("%1M").arg(this->mem);

    // Main drives
    ret << QStringLiteral("-drive") << QStringLiteral("if=virtio,format=qcow2,file=%1").arg(this->hdd);
    ret << QStringLiteral("-cdrom") << this->dvd;

    // Setup firmware
    if (!this->flash1.isEmpty())
        ret << QStringLiteral("-drive") << QStringLiteral("if=pflash,format=raw,file=%1").arg(this->flash1);
    if (!this->flash2.isEmpty())
        ret << QStringLiteral("-drive") << QStringLiteral("if=pflash,format=raw,file=%1").arg(this->flash2);

    if (this->arch == "aarch64")
        ret << QStringLiteral("-machine") << QStringLiteral("virt");

    ret << QStringLiteral("-vnc") << QStringLiteral(":%1").arg(QString::number(this->number));
    return ret;
}

bool Machine::hasKvm()
{
    const QString kvmPath = QStringLiteral("/dev/kvm");

    if (!QFile::exists(kvmPath))
        return false;

    QFileInfo kvmInfo(kvmPath);
    if (!kvmInfo.isReadable() || !kvmInfo.isWritable())
        return false;

    return true;
}

bool Machine::canVirtualize()
{
    QProcess unameMachine;
    unameMachine.start("/bin/uname", QStringList() << "-m");
    unameMachine.waitForFinished(1000);
    const QString machineType = unameMachine.readAllStandardOutput();

    return (machineType == this->arch);
}
