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

Machine::Machine() {
    this->m_process = new QProcess(this);
}

void Machine::start() {
    if (this->m_process->state() == QProcess::Running) {
        qWarning() << "VM process already running";
        return;
    }

    const QStringList vmArgs = getLaunchArguments();

    const QString pwd = QCoreApplication::applicationDirPath();
    const QString qemuBin = QStringLiteral("%1/bin/qemu-system-%2").arg(pwd, this->arch);

    QStringList args;
    if (hasKvm())
        args << QStringLiteral("-enable-kvm");
    args << vmArgs;

    qDebug() << "Start:" << qemuBin << args;

    this->m_process->start(qemuBin, args);
    this->m_process->waitForStarted();
    if (this->m_process->state() != QProcess::Running) {
        qWarning() << "Starting machine failed:" << this->m_process->readAllStandardError();
        return;
    }

    QTimer::singleShot(1000, this, [=](){
        emit started("localhost:5901");
    });
}

QStringList Machine::getLaunchArguments()
{
    QStringList ret;
    ret << QStringLiteral("-drive") << QStringLiteral("if=virtio,format=qcow2,file=%1").arg(this->hdd);
    ret << QStringLiteral("-cdrom") << this->dvd;
    if (this->arch == "aarch64")
        ret << QStringLiteral("-machine") << QStringLiteral("virt");
    ret << QStringLiteral("-vnc") << QStringLiteral(":1");
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
