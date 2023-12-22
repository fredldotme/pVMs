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

#include "vnc_output.h"

#include "scaler.h"
#include "vnc_client.h"

#include <QDebug>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QTransform>

using namespace LomiriVNC;

namespace LomiriVNC {

class VncOutputPrivate {
    Q_DECLARE_PUBLIC(VncOutput)

public:
    VncOutputPrivate(VncOutput *q);
    ~VncOutputPrivate();

    void setScale(qreal scale);
    void setCenter(const QPointF &center);
    void updateMapping();

    void sendKeyEvent(const QString &text);
    void sendMouseEvent(const QPointF &pos, Qt::MouseButtons buttons);

private:
    VncClient *m_client;
    QSize m_vncSize;
    QRect m_vncVisibleRect;
    QRectF m_paintedRect;
    QTransform m_itemToVnc;
    QTransform m_vncToItem;
    qreal m_requestedScale;
    qreal m_scale;
    QPointF m_center;
    VncOutput *q_ptr;
};

} // namespace

VncOutputPrivate::VncOutputPrivate(VncOutput *q):
    m_client(nullptr),
    m_requestedScale(0.0),
    m_scale(0.0),
    q_ptr(q)
{
}

VncOutputPrivate::~VncOutputPrivate() = default;

void VncOutputPrivate::setCenter(const QPointF &center)
{
    m_center = center;
    updateMapping();
}

void VncOutputPrivate::updateMapping()
{
    Q_Q(VncOutput);

    qreal oldScale = m_scale;
    QSizeF oldVncSize = m_vncSize;
    QRectF oldPaintedRect = m_paintedRect;

    qreal w = q->width();
    qreal h = q->height();

    m_vncSize = m_client ? m_client->image().size() : QSize();
    Scaler::InputData in {
        m_vncSize,
        QSizeF(w, h),
        m_requestedScale,
        m_center,
    };

    Scaler::OutputData out;
    bool ok = Scaler::updateMapping(in, &out);
    if (Q_LIKELY(ok)) {
        m_vncVisibleRect = out.sourceVisibleRect.toRect();
        m_paintedRect = out.itemPaintedRect;
        m_scale = out.scale;
        m_center = out.center;
        m_itemToVnc = out.itemToSource;
    } else {
        m_vncVisibleRect = QRect();
        m_paintedRect = QRectF();
        m_scale = 0.0;
        m_center = QPointF();
        m_itemToVnc = QTransform();
    }

    m_vncToItem = m_itemToVnc.inverted();

    if (m_scale != oldScale) {
        Q_EMIT q->scaleChanged();
    }
    Q_EMIT q->centerChanged();
    if (m_vncSize != oldVncSize) {
        Q_EMIT q->remoteScreenSizeChanged();
    }
    if (m_paintedRect != oldPaintedRect) {
        Q_EMIT q->marginsChanged();
    }
}

void VncOutputPrivate::sendKeyEvent(const QString &text)
{
    for (const QChar c: text) {
        m_client->sendKeyEvent(c);
    }
}

void VncOutputPrivate::sendMouseEvent(const QPointF &pos,
                                      Qt::MouseButtons buttons)
{
    if (Q_UNLIKELY(!m_client ||
                   !m_paintedRect.contains(pos.toPoint()))) return;

    m_client->sendMouseEvent(m_itemToVnc.map(pos), buttons);
}

VncOutput::VncOutput(QQuickItem *parent):
    QQuickPaintedItem(parent),
    d_ptr(new VncOutputPrivate(this))
{
    setOpaquePainting(true);
    setAntialiasing(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(QQuickItem::ItemAcceptsInputMethod, true);
    /* For some reason if FBO rendering is enabled, only the first
     * frame is drawn, unless the image is a different one
     */
    //setRenderTarget(QQuickPaintedItem::FramebufferObject);
}

VncOutput::~VncOutput() = default;

void VncOutput::setClient(VncClient *client)
{
    Q_D(VncOutput);
    if (client == d->m_client) return;

    if (client) {
        client->addViewer(this);
    } else if (d->m_client) {
        d->m_client->removeViewer(this);
    }
    d->m_client = client;
    d->updateMapping();
    Q_EMIT clientChanged();
}

VncClient *VncOutput::client() const
{
    Q_D(const VncOutput);
    return d->m_client;
}

void VncOutput::setRequestedScale(qreal scale)
{
    Q_D(VncOutput);
    d->m_requestedScale = qMin(scale, 2.0);
    Q_EMIT requestedScaleChanged();
    d->updateMapping();
    update();
}

qreal VncOutput::requestedScale() const
{
    Q_D(const VncOutput);
    return d->m_requestedScale;
}

qreal VncOutput::scale() const
{
    Q_D(const VncOutput);
    return d->m_scale;
}

void VncOutput::setCenter(const QPointF &center)
{
    Q_D(VncOutput);
    d->setCenter(center);
    update();
}

QPointF VncOutput::center() const
{
    Q_D(const VncOutput);
    return d->m_center;
}

QSizeF VncOutput::remoteScreenSize() const
{
    Q_D(const VncOutput);
    return d->m_vncSize;
}

qreal VncOutput::bottomMargin() const
{
    Q_D(const VncOutput);
    return height() - d->m_paintedRect.bottom();
}

qreal VncOutput::leftMargin() const
{
    Q_D(const VncOutput);
    return d->m_paintedRect.x();
}

qreal VncOutput::rightMargin() const
{
    Q_D(const VncOutput);
    return width() - d->m_paintedRect.right();
}

qreal VncOutput::topMargin() const
{
    Q_D(const VncOutput);
    return d->m_paintedRect.y();
}

QPointF VncOutput::itemToVnc(const QPointF &p) const
{
    Q_D(const VncOutput);
    return d->m_itemToVnc.map(p);
}

QPointF VncOutput::vncToItem(const QPointF &p) const
{
    Q_D(const VncOutput);
    return d->m_vncToItem.map(p);
}

void VncOutput::paint(QPainter *painter)
{
    Q_D(VncOutput);

    const QImage &image = d->m_client->image();
    if (image.size() != d->m_vncSize) {
        d->updateMapping();
        Q_EMIT remoteScreenSizeChanged();
        Q_EMIT marginsChanged();
    }
    painter->drawImage(d->m_paintedRect, image, d->m_vncVisibleRect);
}

void VncOutput::geometryChanged(const QRectF &newGeometry,
                                const QRectF &oldGeometry)
{
    Q_D(VncOutput);
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
    d->updateMapping();
    Q_EMIT marginsChanged();
}

void VncOutput::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(VncOutput);
    QQuickPaintedItem::hoverMoveEvent(event);
    d->sendMouseEvent(event->pos(), Qt::NoButton);
}

void VncOutput::inputMethodEvent(QInputMethodEvent *event)
{
    Q_D(VncOutput);
    qDebug() << Q_FUNC_INFO << event;
    d->sendKeyEvent(event->commitString());
}

QVariant VncOutput::inputMethodQuery(Qt::InputMethodQuery query) const
{
    QVariant ret = QQuickPaintedItem::inputMethodQuery(query);
    if (query == Qt::ImHints) {
        ret = int(Qt::ImhHiddenText |
                  Qt::ImhNoAutoUppercase |
                  Qt::ImhNoPredictiveText);
    }
    return ret;
}

void VncOutput::keyPressEvent(QKeyEvent *event)
{
    Q_D(VncOutput);
    if (d->m_client) {
        d->m_client->sendKeyEvent(event, true);
    }
}

void VncOutput::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(VncOutput);
    if (d->m_client) {
        d->m_client->sendKeyEvent(event, false);
    }
}

void VncOutput::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(VncOutput);
    QQuickPaintedItem::mouseMoveEvent(event);
    d->sendMouseEvent(event->localPos(), event->buttons());
    event->accept();
}

void VncOutput::mousePressEvent(QMouseEvent *event)
{
    Q_D(VncOutput);
    QQuickPaintedItem::mousePressEvent(event);
    d->sendMouseEvent(event->localPos(), event->buttons());
    event->accept();
}

void VncOutput::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(VncOutput);
    QQuickPaintedItem::mouseReleaseEvent(event);
    d->sendMouseEvent(event->localPos(), event->buttons());
    event->accept();
}
