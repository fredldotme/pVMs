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
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QSysInfo>
#include <QTimer>
#include <QThread>

#include "machine.h"

Machine::Machine()
{
    this->m_process = new QProcess(this);
    QObject::connect(this->m_process, &QProcess::stateChanged, this, [=](QProcess::ProcessState newState){
        if (newState == QProcess::NotRunning) {
            qWarning() << this->m_process->readAllStandardOutput();
            if (this->m_process->exitCode() != 0)
                emit error(this->m_process->readAllStandardError());
        }

        if (newState != QProcess::Running) {
            qWarning() << this->m_process->readAllStandardError();
            emit stopped();
            return;
        }

        if (newState == QProcess::Running) {
            QTimer::singleShot(1000, this, [=](){
                emit started();
            });
        }
    });

    this->m_fileSharingProcess = new QProcess(this);
    QObject::connect(this->m_fileSharingProcess, &QProcess::stateChanged, this, [=](QProcess::ProcessState newState) {
        qDebug() << "virtiofsd new state:" << newState;
        qDebug() << this->m_fileSharingProcess->readAllStandardOutput();
        qWarning() << this->m_fileSharingProcess->readAllStandardError();

        if (newState == QProcess::NotRunning) {
            if(this->m_fileSharingProcess->exitCode() != 0)
                emit fileSharingError(this->m_fileSharingProcess->readAllStandardError());
            emit stopped();
            return;
        }

        if (newState == QProcess::Running) {
            startQemu();
        }
    });

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

bool Machine::start()
{
    if (this->m_process->state() == QProcess::Running) {
        qWarning() << "VM process already running";
        return false;
    }

    if (this->m_fileSharingProcess->state() == QProcess::Starting ||
            this->m_process->state() == QProcess::Starting)
    {
        // Return true as the VM is already starting and should
        // put the UI into a "make it killable" state.
        qWarning() << "VM processes already starting";
        return true;
    }

    // If file sharing is enabled, start QEMU *after* virtiofsd is up
    // Start directly otherwise.
    if (this->enableFileSharing) {
        const QString pwd = QCoreApplication::applicationDirPath();
        const QString fsdBin = QStringLiteral("%1/libexec/virtiofsd").arg(pwd);
        const QString directory = getFileSharingDirectory();
        const QString runtimeDirPath = QStringLiteral("%1/run").arg(this->storage);

        // Create sharing directory if necessary
        {
            QDir sharingDir(directory);
            if (!sharingDir.exists()) {
                sharingDir.mkpath(directory);
            }
        }

        // Also create virtiofsd runtime dir
        {
            QDir sharingRuntimeDir(runtimeDirPath);
            if (!sharingRuntimeDir.exists()) {
                sharingRuntimeDir.mkpath(runtimeDirPath);
            }
        }

        const QStringList fsdArgs = {
            QStringLiteral("--socket-path=%1").arg(getFileSharingSocket()),
            "-o", QStringLiteral("source=%2").arg(directory),
            "-o", "allow_root",
            "-o", "allow_direct_io",
            "-o", "xattr",
            "-o", "writeback",
            "-o", "readdirplus",
            "-o", "posix_lock",
            "-o", "flock",
            "-f"
        };

        // Set virtiofsd runtime dir to something it can write to
        QProcessEnvironment fsdEnv = QProcessEnvironment::systemEnvironment();
        fsdEnv.insert("XDG_RUNTIME_DIR", runtimeDirPath);
        this->m_fileSharingProcess->setProcessEnvironment(fsdEnv);

        qDebug() << "Starting:" << fsdBin << fsdArgs;
        QFile::remove(getFileSharingSocket()); // May fail if it doesn't exist
        this->m_fileSharingProcess->start(fsdBin, fsdArgs);
    } else {
        if (!startQemu())
            return false;
    }
    return true;
}

void Machine::stop()
{
    this->m_process->terminate();
    this->m_process->waitForFinished();
    emit stopped();
}

bool Machine::startQemu()
{
    const QString pwd = QCoreApplication::applicationDirPath();
    const QString qemuBin = QStringLiteral("%1/bin/qemu-system-%2").arg(pwd, this->arch);
    const QStringList args = getLaunchArguments();

    if (this->enableFileSharing) {
        int counter = 0;
        static const int MAX_RETRIES = 3;
        const QString socket = getFileSharingSocket();
        while (!QFile::exists(socket) && counter < MAX_RETRIES) {
            QThread::msleep(1000);
            ++counter;
        }
        if (counter >= MAX_RETRIES && !QFile::exists(socket)) {
            qWarning() << "Waited 3 seconds for socket" << socket;
            return false;
        }
    }

    // Pass proper and valid APP_ID as DESKTOP_FILE_HINT
    QProcessEnvironment qemuEnv = QProcessEnvironment::systemEnvironment();
    qemuEnv.insert("DESKTOP_FILE_HINT", qgetenv("APP_ID"));
    this->m_process->setProcessEnvironment(qemuEnv);

    qDebug() << "Start:" << qemuBin << args;
    this->m_process->start(qemuBin, args);
    return true;
}

QStringList Machine::getLaunchArguments()
{
    QStringList ret;

    const bool useKvm = hasKvm() && canVirtualize();

    // Machine setup
    ret << QStringLiteral("-smp") << QString::number(this->cores);
    ret << QStringLiteral("-m") << QStringLiteral("%1M").arg(this->mem);

    // Use KVM if possible
    if (useKvm)
        ret << QStringLiteral("-enable-kvm");

    // Use "virt" machine on aarch64
    if (this->arch == QStringLiteral("aarch64")) {
        ret << QStringLiteral("-machine") << QStringLiteral("virt");

        // Enable host CPU mode when virtualization is possible
        if (useKvm)
            ret << QStringLiteral("-cpu") << QStringLiteral("host");
        else
            ret << QStringLiteral("-cpu") << QStringLiteral("cortex-a57");
    }
    // Also enable host CPU mode on x86_64 if possible
    else {
        if (useKvm)
            ret << QStringLiteral("-cpu") << QStringLiteral("host");
    }

    if (this->useVirglrenderer) {
        ret << QStringLiteral("-display") << QStringLiteral("egl-headless,gl=es");
    }

    // Display
    if (!this->useVirglrenderer) {
        ret << QStringLiteral("-device") << QStringLiteral("virtio-gpu-pci");
    } else {
        ret << QStringLiteral("-device") << QStringLiteral("virtio-ramfb-gl");
    }

    // ISO/DVD drive
    // This one likes to get lost due to content-hub clearing each app's cache during boot,
    // so add a check whether the file is actually there or not.
    if (!this->dvd.isEmpty() && QFile::exists(this->dvd)) {
        ret << QStringLiteral("-cdrom") << this->dvd;
    }

    // Main drive
    ret << QStringLiteral("-drive") << QStringLiteral("if=virtio,format=qcow2,file=%1").arg(this->hdd);

    // USB and input peripherals
    ret << QStringLiteral("-device") << QStringLiteral("qemu-xhci");
    ret << QStringLiteral("-device") << QStringLiteral("usb-tablet");
    ret << QStringLiteral("-device") << QStringLiteral("usb-kbd");

    // Networking
    ret << QStringLiteral("-netdev") << QStringLiteral("user,id=net0")
        << QStringLiteral("-device") << QStringLiteral("virtio-net-pci,netdev=net0");

    // Setup firmware
    if (!this->flash1.isEmpty())
        ret << QStringLiteral("-drive") << QStringLiteral("if=pflash,format=raw,unit=0,file=%1").arg(this->flash1);
    if (!this->flash2.isEmpty())
        ret << QStringLiteral("-drive") << QStringLiteral("if=pflash,format=raw,unit=1,file=%1").arg(this->flash2);

    // Optional file sharing
    if (this->enableFileSharing) {
        ret << QStringLiteral("-chardev") << QStringLiteral("socket,id=char0,path=%1").arg(getFileSharingSocket())
            << QStringLiteral("-device") << QStringLiteral("vhost-user-fs-pci,chardev=char0,tag=pocketvms")
            << QStringLiteral("-object") << QStringLiteral("memory-backend-memfd,id=mem,size=%1M,share=on").arg(this->mem)
            << QStringLiteral("-numa") << QStringLiteral("node,memdev=mem");
    }

    ret << QStringLiteral("-vnc") << QStringLiteral("unix:%1").arg(QStringLiteral("%1/vnc.sock").arg(this->storage));
    return ret;
}

bool Machine::hasKvm()
{
    const QString kvmPath = QStringLiteral("/dev/kvm");

    if (!QFile::exists(kvmPath)) {
        qWarning() << "KVM is not enabled on this kernel or device.";
        return false;
    }

    QFileInfo kvmInfo(kvmPath);
    if (!kvmInfo.isReadable()) {
        qWarning() << "/dev/kvm is not readable.";
        return false;
    }
    if (!kvmInfo.isWritable()) {
        qWarning() << "/dev/kvm is not writable.";
        return false;
    }

    return true;
}

bool Machine::canVirtualize()
{
    // Only "arm64" and "x86_64" are supported anyway
    const QString currentCpuType = QSysInfo::currentCpuArchitecture();
    const QString machineType = currentCpuType == QStringLiteral("arm64") ?
                QStringLiteral("aarch64") : currentCpuType;
    qDebug() << "uname" << machineType << "vs arch" << this->arch;

    return (machineType == this->arch);
}

QString Machine::getFileSharingDirectory()
{
    const QString path = QStringLiteral("%1/shared").arg(this->storage);
    return path;
}

QString Machine::getFileSharingSocket()
{
    const QString path = QStringLiteral("%1/virtiofsd.sock").arg(this->storage);
    return path;
}
