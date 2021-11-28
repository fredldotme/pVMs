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

public:
    VMManager();
    ~VMManager() = default;

    Q_INVOKABLE void startVM(Machine* machine);
    Q_INVOKABLE void refreshVMs();
    Q_INVOKABLE Machine* fromQml(QVariantMap vm);
    Q_INVOKABLE bool createVM(Machine* machine);
    Q_INVOKABLE bool editVM(Machine* machine);
    Q_INVOKABLE bool deleteVM(Machine* machine);

private:
    QVariantMap listEntryForJSON(const QString& path, const QString& storage);
    QByteArray machineToJSON(const Machine* machine);
    void setRefreshing(bool value);

    QVariantList m_vms;
    bool m_refreshing;

signals:
    void vmsChanged();
    void refreshingChanged();
};

#endif
