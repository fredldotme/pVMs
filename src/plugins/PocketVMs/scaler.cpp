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

#include "scaler.h"

#include <QDebug>

using namespace LomiriVNC;

/* Compute the minimum offset (from the origin) that needs to be applied to
 * the rect `view` so that an object of size `objectSize` located at the
 * origin would optimally fit into it.
 * "Optimally" means:
 * 1. `view` is as filled as possible with the object (minimize the empty
 *    area)
 * 2. If the object is entirely visible, it should be centered into the view.
 */
QPointF Scaler::computeFitOffset(const QRectF &view, const QSizeF &objectSize)
{
    double offsetX = 0, offsetY = 0;

    if (view.width() > objectSize.width()) {
        /* center the source objectSize within the view */
        offsetX = -view.x() - (view.width() - objectSize.width()) / 2;
    } else {
        double leftBorder = -view.x();
        double rightBorder = view.right() - objectSize.width();
        if (leftBorder > 0) {
            offsetX = leftBorder;
        } else if (rightBorder > 0) {
            offsetX = -rightBorder;
        }
    }

    if (view.height() > objectSize.height()) {
        /* center the source objectSize within the view */
        offsetY = -view.y() - (view.height() - objectSize.height()) / 2;
    } else {
        double topBorder = -view.y();
        double bottomBorder = view.bottom() - objectSize.height();
        if (topBorder > 0) {
            offsetY = topBorder;
        } else if (bottomBorder > 0) {
            offsetY = -bottomBorder;
        }
    }
    return QPointF(offsetX, offsetY);
}

bool Scaler::updateMapping(const InputData &in, OutputData *out)
{
    QTransform itemToSource;

    const QSizeF &sourceSize = in.sourceSize;
    const QSizeF &itemSize = in.itemSize;

    if (Q_UNLIKELY(sourceSize.isEmpty() || itemSize.isEmpty())) {
        return false;
    }

    const QRectF itemRect(QPointF(0, 0), itemSize);

    // Compute the minimum scale
    QSizeF scaledSize = sourceSize.scaled(itemSize, Qt::KeepAspectRatio);
    double minScale = qMin(scaledSize.width() / sourceSize.width(), 1.0);
    double scale = qMax(in.requestedScale, minScale);

    // Compute the transformation matrix
    QRectF sourceRect = QRectF(QPointF(0, 0), sourceSize);
    scaledSize = sourceSize * scale;

    QPointF center = in.requestedCenter;

    QPointF itemRectCenter = itemRect.center();
    QPointF sourceRectCenter = sourceRect.center();
    itemToSource.translate(sourceRectCenter.x() + center.x(),
                           sourceRectCenter.y() + center.y());
    itemToSource.scale(1.0 / scale, 1.0 / scale);
    itemToSource.translate(-itemRectCenter.x(), -itemRectCenter.y());

    /* Ensure that as much as possible of the source is visible. If the source
     * is completely visible, make sure that it's centered in the view.
     */
    QPointF centerAdjustment =
        computeFitOffset(itemToSource.mapRect(itemRect), sourceSize);
    if (!centerAdjustment.isNull()) {
        // Apply the offset to the transformation matrix
        QTransform adjustment =
            QTransform::fromTranslate(centerAdjustment.x(),
                                      centerAdjustment.y());
        itemToSource *= adjustment;
    }

    /* Compute the painted rect: use the inverse transformation to figure out
     * the position and size of the source item in view coordinates, and then
     * clip it to the view.
     */
    QTransform sourceToItem = itemToSource.inverted();
    QRectF virtualSourceRect = sourceToItem.mapRect(sourceRect);
    QRectF paintedRect = virtualSourceRect.intersected(itemRect);

    // Compute the visible area of the source (in src coordinates)
    QRectF sourceVisibleRect = itemToSource.mapRect(paintedRect);

    // Prepare the return structure
    out->center = center + centerAdjustment;
    out->sourceVisibleRect = sourceVisibleRect;
    out->itemPaintedRect = paintedRect;
    out->scale = scale;
    out->itemToSource = itemToSource;
    out->sourceToItem = sourceToItem;
    return true;
}
