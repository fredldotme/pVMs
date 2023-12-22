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

#include <QtQml>

#include "plugin.h"
#include "vmmanager.h"
#include "vnc_client.h"
#include "vnc_output.h"

void ExamplePlugin::registerTypes(const char *uri) {
    //@uri VMManager
    qmlRegisterType<Machine>(uri, 1, 0, "Machine");
    qmlRegisterSingletonType<VMManager>(uri, 1, 0, "VMManager", [](QQmlEngine*, QJSEngine*) -> QObject* { return new VMManager; });
    using namespace LomiriVNC;
    qmlRegisterType<VncClient>(uri, 1, 0, "VncClient");
    qmlRegisterType<VncOutput>(uri, 1, 0, "VncOutput");
}
