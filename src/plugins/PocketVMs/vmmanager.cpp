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
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QString>
#include <QUuid>
#include <QVariant>

#include <stdio.h>
#include <unistd.h>

#include "vmmanager.h"

const QString KEY_STORAGE = QStringLiteral("storage");
const QString KEY_DESC = QStringLiteral("description");
const QString KEY_ARCH = QStringLiteral("arch");
const QString KEY_CORES = QStringLiteral("cores");
const QString KEY_MEM = QStringLiteral("mem");
const QString KEY_DVD = QStringLiteral("dvd");
const QString KEY_HDD = QStringLiteral("hdd");
const QString KEY_FLASH1 = QStringLiteral("flash1");
const QString KEY_FLASH2 = QStringLiteral("flash2");

const QStringList VALID_ARCHES = {
    QStringLiteral("x86_64"),
    QStringLiteral("aarch64"),
};

VMManager::VMManager() {

}

void VMManager::setRefreshing(bool value)
{
    if (this->m_refreshing == value)
        return;

    this->m_refreshing = value;
    emit refreshingChanged();
}

void VMManager::refreshVMs()
{
    QVariantList vms;
    QDirIterator dirIt(
                QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
                QStringList() << "info.json", QDir::Files, QDirIterator::Subdirectories);

    setRefreshing(true);

    while (dirIt.hasNext()) {
        dirIt.next();
        QVariantMap vm;
        try {
            vm = listEntryForJSON(dirIt.filePath(), QFileInfo(dirIt.filePath()).canonicalPath());
        } catch (...) {
            continue;
        }
        vms.push_back(vm);
    }
    this->m_vms = vms;
    emit vmsChanged();

    setRefreshing(false);
}

Machine* VMManager::fromQml(QVariantMap vm)
{
    Machine* machine = new Machine();
    QQmlEngine::setObjectOwnership(machine, QQmlEngine::JavaScriptOwnership);

    machine->storage = vm.value(KEY_STORAGE).toString();
    machine->name = vm.value(KEY_DESC).toString();
    machine->arch = vm.value(KEY_ARCH).toString();
    machine->cores = vm.value(KEY_CORES).toInt();
    machine->mem = vm.value(KEY_MEM).toInt();
    machine->hdd = vm.value(KEY_HDD).toString();
    machine->dvd = vm.value(KEY_DVD).toString();
    machine->flash1 = vm.value(KEY_FLASH1).toString();
    machine->flash2 = vm.value(KEY_FLASH2).toString();

    return machine;
}

bool VMManager::createVM(Machine* machine)
{
    if (!machine) {
        qWarning() << "nullptr machine provided";
        return false;
    }

    const QString vmDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
            QStringLiteral("/") + QUuid::createUuid().toString();
    const QString pwd = QCoreApplication::applicationDirPath();

    machine->storage = vmDirPath;

    // Create directory storing the VM image
    {
        QDir vmDir(vmDirPath);
        if (!vmDir.exists()) {
            vmDir.mkpath(vmDirPath);
        }
    }

    // Create the QCOW2 image for the HDD
    {
        const QString qemuImgBin = QStringLiteral("%1/bin/qemu-img").arg(pwd);
        const QString hddPath = QStringLiteral("%1/hdd.qcow2").arg(vmDirPath);

        QStringList qemuImgArgs;
        qemuImgArgs << QStringLiteral("create") << QStringLiteral("-f") << QStringLiteral("qcow2");
        qemuImgArgs << hddPath << QStringLiteral("%1G").arg(machine->hddSize);
        qDebug() << "Creating qcow2 image with arguments:" << qemuImgArgs;

        QProcess qemuImg;
        qemuImg.start(qemuImgBin, qemuImgArgs);
        qemuImg.waitForFinished();
        if (qemuImg.exitCode() != 0) {
            qWarning() << "qemu-img failed:" << qemuImg.readAllStandardError();
            return false;
        }

        machine->hdd = hddPath;
    }

#if 0 // Enable once Content-Hub incoming files can be unlinked from their source location
    // Move the DVD/ISO image from HubIncoming to storage
    {
        const QString dvdStoragePath = QStringLiteral("%1/dvd.iso").arg(vmDirPath);
        if (rename(machine->dvd.toUtf8().data(), dvdStoragePath.toUtf8().data())) {
            qWarning() << "Failed to move" << machine->dvd << "DVD image to target" << dvdStoragePath;
            return false;
        }
        machine->dvd = dvdStoragePath;
    }
#endif

    {
        const bool ret = resetEFIFirmware(machine);
        if (!ret)
            return false;
    }

    {
        const bool ret = resetEFINVRAM(machine);
        if (!ret)
            return false;
    }

    // Finally, create the VM metadata
    {
        const QString jsonFilePath = QStringLiteral("%1/info.json").arg(vmDirPath);
        QFile jsonFile(jsonFilePath);
        if (!jsonFile.open(QFile::ReadWrite)) {
            qWarning() << "Failed to open JSON file for writing";
            return false;
        }

        jsonFile.write(machineToJSON(machine));
    }

    return true;
}


// Copy the EFI firmware to storage
{
    const QString efiFw = QStringLiteral("%1/share/qemu/edk2-%2-code.fd").arg(pwd, machine->arch);
    const QString efiFwTarget = QStringLiteral("%1/efi.fd").arg(vmDirPath);
    if (!QFile::copy(efiFw, efiFwTarget)) {
        qWarning() << "Failed to copy" << efiFw << "EFI firmware to target" << efiFwTarget;
        return false;
    }

    machine->flash1 = efiFwTarget;
    return true;
}

// Copy the EFI NVRAM to storage
{
    const QString varsArch = (machine->arch == QStringLiteral("aarch64")) ?
                QStringLiteral("arm") : QStringLiteral("i386");
    const QString efiVars = QStringLiteral("%1/share/qemu/edk2-%2-vars.fd").arg(pwd, varsArch);
    const QString efiVarsTarget = QStringLiteral("%1/efi_nvram.fd").arg(vmDirPath);
    if (!QFile::copy(efiVars, efiVarsTarget)) {
        qWarning() << "Failed to copy" << efiVars << "EFI NVRAM to target" << efiVarsTarget;
        return false;
    }

    machine->flash2 = efiVarsTarget;
    return true;
}


QVariantMap VMManager::listEntryForJSON(const QString& path, const QString& storage)
{
    QVariantMap ret;
    QFile jsonFile(path);

    ret.insert("path", path);
    ret.insert(KEY_STORAGE, storage);

    if (!jsonFile.exists())
        throw "File doesn't exist";
    if (!jsonFile.open(QFile::ReadOnly))
        throw "Couldn't open file";

    QJsonParseError jsonErr;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll(), &jsonErr);
    if (jsonErr.error != QJsonParseError::NoError)
        throw jsonErr.errorString().toStdString();

    if (!jsonDoc.isObject())
        throw "Invalid VM information";

    QJsonObject rootObject = jsonDoc.object();

    if (!rootObject.contains(KEY_DESC))
        throw "Missing 'description'";
    ret.insert("description", rootObject.value(KEY_DESC).toString());

    if (!rootObject.contains(KEY_ARCH))
        throw "Missing 'arch'";
    const QString arch = rootObject.value(KEY_ARCH).toString();
    if (!VALID_ARCHES.contains(arch))
        throw "Invalid architecture";
    ret.insert("arch", arch);

    if (!rootObject.contains(KEY_CORES))
        throw "Missing 'cores'";
    ret.insert("cores", rootObject.value(KEY_CORES).toString());

    if (!rootObject.contains(KEY_MEM))
        throw "Missing 'mem'";
    ret.insert("mem", rootObject.value(KEY_MEM).toString());

    if (!rootObject.contains(KEY_HDD))
        throw "Missing 'hdd'";
    ret.insert("hdd", rootObject.value(KEY_HDD).toString());

    if (!rootObject.contains(KEY_DVD))
        throw "Missing 'dvd'";
    ret.insert("dvd", rootObject.value(KEY_DVD).toString());

    if (!rootObject.contains(KEY_FLASH1))
        throw "Missing 'flash1'";
    ret.insert("flash1", rootObject.value(KEY_FLASH1).toString());

    if (!rootObject.contains(KEY_FLASH2))
        throw "Missing 'flash2'";
    ret.insert("flash2", rootObject.value(KEY_FLASH2).toString());

    return ret;
}

QByteArray VMManager::machineToJSON(const Machine* machine)
{
    QJsonObject rootObject;
    rootObject.insert(KEY_DESC, QJsonValue(machine->name).toString());
    rootObject.insert(KEY_ARCH, QJsonValue(machine->arch).toString());
    rootObject.insert(KEY_CORES, QJsonValue(QString::number(machine->cores)).toString());
    rootObject.insert(KEY_MEM, QJsonValue(QString::number(machine->mem)).toString());
    rootObject.insert(KEY_DVD, QJsonValue(machine->dvd).toString());
    rootObject.insert(KEY_HDD, QJsonValue(machine->hdd).toString());
    rootObject.insert(KEY_FLASH1, QJsonValue(machine->flash1).toString());
    rootObject.insert(KEY_FLASH2, QJsonValue(machine->flash2).toString());

    QJsonDocument doc(rootObject);
    return doc.toJson();
}

bool VMManager::editVM(Machine* machine)
{
    if (!machine) {
        qWarning() << "nullptr machine provided";
        return false;
    }

    // Edit the VM metadata
    {
        const QString jsonFilePath = QStringLiteral("%1/info.json").arg(machine->storage);
        QFile jsonFile(jsonFilePath);
        if (!jsonFile.open(QFile::ReadWrite | QFile::Truncate)) {
            qWarning() << "Failed to open JSON file for writing";
            return false;
        }

        jsonFile.write(machineToJSON(machine));
    }
}

bool VMManager::deleteVM(Machine* machine)
{
    qDebug() << "Deleting:" << machine->storage;
    return QDir(machine->storage).removeRecursively();
}
