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

#include <QFileInfo>

#include "vmmanager.h"

VMManager::VMManager() {

}

void VMManager::startVM(Machine* machine) {
    const QString kvmArgs = hasKvm() ? "-enable-kvm" : "";
    const QString vmArgs = getLaunchArguments(machine);
}

QString VMManager::getLaunchArguments(Machine* machine)
{
    return QStringLiteral("");
}

bool VMManager::hasKvm()
{
    const QString kvmPath = QStringLiteral("/dev/kvm");

    if (!QFile::exists(kvmPath))
        return false;

    QFileInfo kvmInfo(kvmPath);
    if (!kvmInfo.isReadable() || !kvmInfo.isWritable())
        return false;

    return true;
}
