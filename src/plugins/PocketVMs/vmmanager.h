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

#ifndef VMMANAGER_H
#define VMMANAGER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "machine.h"

class VMManager: public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList vms MEMBER m_vms NOTIFY vmsChanged)
    Q_PROPERTY(bool refreshing MEMBER m_refreshing NOTIFY refreshingChanged)

    Q_PROPERTY(int maxRam READ maxRam CONSTANT)
    Q_PROPERTY(int maxCores READ maxCores CONSTANT)
    Q_PROPERTY(int maxHddSize READ maxHddSize CONSTANT)

public:
    VMManager();
    ~VMManager() = default;

    Q_INVOKABLE void startVM(Machine* machine);
    Q_INVOKABLE void refreshVMs();
    Q_INVOKABLE static Machine* fromQml(const QVariantMap& vm);
    Q_INVOKABLE static bool createVM(Machine* machine);
    Q_INVOKABLE static bool editVM(Machine* machine);
    Q_INVOKABLE static bool deleteVM(Machine* machine);
    Q_INVOKABLE static bool resetEFIFirmware(Machine* machine);
    Q_INVOKABLE static bool resetEFINVRAM(Machine* machine);

    Q_INVOKABLE static bool canVirtualize(const QString& arch);

private:
    static QVariantMap listEntryForJSON(const QString& path, const QString& storage);
    static QByteArray machineToJSON(const Machine* machine);
    void setRefreshing(bool value);

    static int maxRam();
    static int maxCores();
    static int maxHddSize();

    QVariantList m_vms;
    bool m_refreshing;

signals:
    void vmsChanged();
    void refreshingChanged();
};

#endif
