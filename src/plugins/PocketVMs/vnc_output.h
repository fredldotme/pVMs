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

#ifndef LOMIRIVNC_VNC_OUTPUT_H
#define LOMIRIVNC_VNC_OUTPUT_H

#include "vnc_client.h"

#include <QQuickPaintedItem>
#include <QScopedPointer>
#include <QSizeF>

namespace LomiriVNC {

class VncOutputPrivate;
class VncOutput: public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(VncClient *client READ client WRITE setClient NOTIFY clientChanged)
    Q_PROPERTY(qreal requestedScale READ requestedScale WRITE setRequestedScale
               NOTIFY requestedScaleChanged)
    Q_PROPERTY(qreal scale READ scale NOTIFY scaleChanged)
    Q_PROPERTY(QPointF center READ center WRITE setCenter NOTIFY centerChanged)
    Q_PROPERTY(QSizeF remoteScreenSize READ remoteScreenSize
               NOTIFY remoteScreenSizeChanged)
    Q_PROPERTY(qreal bottomMargin READ bottomMargin NOTIFY marginsChanged)
    Q_PROPERTY(qreal leftMargin READ leftMargin NOTIFY marginsChanged)
    Q_PROPERTY(qreal rightMargin READ rightMargin NOTIFY marginsChanged)
    Q_PROPERTY(qreal topMargin READ topMargin NOTIFY marginsChanged)

public:
    VncOutput(QQuickItem *parent = nullptr);
    virtual ~VncOutput();

    void setClient(VncClient *client);
    VncClient *client() const;

    void setRequestedScale(qreal scale);
    qreal requestedScale() const;

    qreal scale() const;

    void setCenter(const QPointF &center);
    QPointF center() const;

    QSizeF remoteScreenSize() const;

    qreal bottomMargin() const;
    qreal leftMargin() const;
    qreal rightMargin() const;
    qreal topMargin() const;

    Q_INVOKABLE QPointF itemToVnc(const QPointF &p) const;
    Q_INVOKABLE QPointF vncToItem(const QPointF &p) const;

    void paint(QPainter *painter) override;

Q_SIGNALS:
    void clientChanged();
    void requestedScaleChanged();
    void scaleChanged();
    void centerChanged();
    void remoteScreenSizeChanged();
    void marginsChanged();

protected:
    void geometryChanged(const QRectF &newGeometry,
                         const QRectF &oldGeometry) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Q_DECLARE_PRIVATE(VncOutput)
    QScopedPointer<VncOutputPrivate> d_ptr;
};

} // namespace

#endif // LOMIRIVNC_VNC_OUTPUT_H
