/*
 * Copyright (C) 2020 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of LomiriVNC.
 *
 * LomiriVNC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LomiriVNC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LomiriVNC.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LOMIRIVNC_VNC_CLIENT_H
#define LOMIRIVNC_VNC_CLIENT_H

#include <QObject>
#include <QScopedPointer>

class QImage;
class QKeyEvent;
class QPointF;
class QQuickItem;

namespace LomiriVNC {

class VncClientPrivate;
class VncClient: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionStatusChanged)

public:
    VncClient(QObject *parent = nullptr);
    virtual ~VncClient();

    bool isConnected() const;

    void addViewer(QQuickItem *viewer);
    void removeViewer(QQuickItem *viewer);
    const QImage &image() const;

    Q_INVOKABLE bool connectToServer(const QString &host, const QString &password);
    Q_INVOKABLE void disconnect();

    void sendKeyEvent(QChar c);
    void sendKeyEvent(QKeyEvent *keyEvent, bool pressed);
    Q_INVOKABLE void sendMouseEvent(const QPointF &pos,
                                    Qt::MouseButtons buttons);

Q_SIGNALS:
    void connectionStatusChanged();

private:
    Q_DECLARE_PRIVATE(VncClient)
    QScopedPointer<VncClientPrivate> d_ptr;
};

} // namespace

#endif // LOMIRIVNC_VNC_CLIENT_H
