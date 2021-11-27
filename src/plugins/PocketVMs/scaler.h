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

#ifndef LOMIRIVNC_VNC_SCALER_H
#define LOMIRIVNC_VNC_SCALER_H

#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTransform>

class ScalerTest;

namespace LomiriVNC {

class Scaler
{
public:
    struct InputData {
        QSizeF sourceSize;
        QSizeF itemSize;
        double requestedScale;
        QPointF requestedCenter; /* as offset from the source center, in source
                                    coordinates */
    };

    struct OutputData {
        QRectF sourceVisibleRect;
        QRectF itemPaintedRect;
        double scale;
        QPointF center;
        QTransform itemToSource;
        QTransform sourceToItem;
    };

    static bool updateMapping(const InputData &in, OutputData *out);

private:
    friend class ::ScalerTest;
    static QPointF computeFitOffset(const QRectF &rect, const QSizeF &size);
};

} // namespace

#endif // LOMIRIVNC_VNC_SCALER_H
