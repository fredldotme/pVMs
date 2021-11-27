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

#ifndef MACHINE_H
#define MACHINE_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

class Machine: public QObject {
    Q_OBJECT

    Q_PROPERTY(QString name MEMBER name NOTIFY nameChanged)
    Q_PROPERTY(QString arch MEMBER arch NOTIFY archChanged)
    Q_PROPERTY(QString hdd MEMBER hdd NOTIFY hddChanged)
    Q_PROPERTY(QString dvd MEMBER dvd NOTIFY dvdChanged)
    Q_PROPERTY(QString cpu MEMBER display NOTIFY cpuChanged)
    Q_PROPERTY(QString display MEMBER display NOTIFY displayChanged)

public:
    Machine();
    ~Machine() = default;

    QString name;
    QString arch;
    QString hdd;
    QString dvd;
    QString cpu;
    QString display;

    Q_INVOKABLE void start();

private:
    QStringList getLaunchArguments();
    bool hasKvm();

    QProcess* m_process = nullptr;

signals:
    void nameChanged();
    void archChanged();
    void cpuChanged();
    void hddChanged();
    void dvdChanged();
    void displayChanged();

    void started(QString host);
    void stopped();
};

#endif
