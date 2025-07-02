/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usage
** For commercial licensing terms and conditions, contact the authors via the
** email address above.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appear in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.
**
** $SQUAREDESK_END_LICENSE$
**
****************************************************************************/

#ifndef SDDANCER_H
#define SDDANCER_H

#include <QGraphicsItemGroup>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QColor>

class SDDancer
{
public:
//    double start_x;
//    double start_y;
//    double start_rotation;

    
    QGraphicsItemGroup *graphics;
//    QGraphicsItem *mainItem;
    QGraphicsItem *boyItem;
    QGraphicsItem *girlItem;
    QGraphicsItem *hexItem;

    QGraphicsRectItem *directionRectItem;
    QGraphicsTextItem *label;

    // See note on setDestinationScalingFactors below
    
    void setDestination(int x, int y, int direction)
    {
        source_x = dest_x;
        source_y = dest_y;
        source_direction = dest_direction;
        
        dest_x = x;
        dest_y = y;
        dest_direction = direction;
    }

    
    // This is really gross: We stash stuff in the destination, then
    // when we have finished calculating the common factors, we adjust
    // the destination values. It sucks and is awful and technical
    // debt.
    
    void setDestinationScalingFactors(double left_x, double max_x, double max_y, double lowest_factor)
    {
//        qDebug() << "setDestScalingFactors:" << left_x << max_x << max_y << lowest_factor;
        double dancer_start_x = dest_x - left_x;
        dest_x = (dancer_start_x / lowest_factor - max_x / 2.0);
        dest_y = dest_y - max_y / 2.0;
    }
    
    double getX(double t)
    {
        return (source_x * (1 - t) + dest_x * t);
    }

    double getY(double t)
    {
        return (source_y * (1 - t) + dest_y * t);
    }

    double getDirection(double t)
    {
        double src = source_direction;
        double dest = dest_direction;
        double result;

        if (dest > src) {
            if (dest - src <= 180) {
                // go CCW, like normal, e.g. 0 -> 90
            } else {
                // go CW, e.g. 0 -> 270
                dest -= 360.0;  // reflect, e.g. 0 -> -90
            }
        } else {
            // dest < src
            if (src - dest <= 180) {
                // go CW, like normal, e.g. 90 -> 0
            } else {
                // go CCW, e.g. 270 -> 0
                src -= 360.0;  // reflect, e.g. -90 -> 0
            }
        }

        result = src * (1 - t) + dest * t;

//        if (dest_direction < source_direction && t < 1.0)
//            return (source_direction * (1 - t) + (dest_direction + 360.0) * t);
//        return (source_direction * (1 - t) + dest_direction * t);

        return(result);
    }

private:
    double source_x;
    double source_y;
    double source_direction;
    double dest_x;
    double dest_y;
    double dest_direction;
    QPen pen1;
//    double destination_divisor;
public:
    double labelTranslateX;
    double labelTranslateY;

    void setColor(const QColor &color);
    void setColors(const QColor &baseColor, const QColor &outlineColor);
};

#endif // SDDANCER_H