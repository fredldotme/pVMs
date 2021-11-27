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

#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QString>
#include <QVariant>

#include "vmmanager.h"

const QString KEY_DESC = QStringLiteral("description");
const QString KEY_ARCH = QStringLiteral("arch");
const QString KEY_DVD = QStringLiteral("dvd");
const QString KEY_HDD = QStringLiteral("hdd");

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
            vm = listEntryForJSON(dirIt.filePath());
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

    machine->name = vm.value(KEY_DESC).toString();
    machine->arch = vm.value(KEY_ARCH).toString();
    machine->hdd = vm.value(KEY_HDD).toString();
    machine->dvd = vm.value(KEY_DVD).toString();

    return machine;
}

QVariantMap VMManager::listEntryForJSON(const QString& path)
{
    QVariantMap ret;
    QFile jsonFile(path);

    ret.insert("path", path);

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
    ret.insert("arch", rootObject.value(KEY_ARCH).toString());

    if (!rootObject.contains(KEY_HDD))
        throw "Missing 'hdd'";
    ret.insert("hdd", rootObject.value(KEY_HDD).toString());

    if (!rootObject.contains(KEY_DVD))
        throw "Missing 'dvd'";
    ret.insert("dvd", rootObject.value(KEY_DVD).toString());

    return ret;
}
