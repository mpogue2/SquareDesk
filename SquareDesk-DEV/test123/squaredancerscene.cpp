/****************************************************************************
**
** Copyright (C) 2016-2023 Mike Pogue, Dan Lyke
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

#include "mainwindow.h"
#include "squaredancerscene.h"
#include <math.h>
// for MIN(...,...)
#include <sys/param.h>

void SquareDancerScene::resizeEvent(QResizeEvent *event) {
//    QSize oldSize = event->oldSize();
//    if (oldSize.width() == -1 || oldSize.height() == -1)
//        return;
    const float padding = 8;
    QRectF bounds = scene()->sceneRect();
    QSize size = event->size();
    double horizontalScale  = (double)(size.width()) / (bounds.width() + padding);
    double verticalScale  = (double)(size.height()) / (bounds.height() + padding);
    double scale = MIN(horizontalScale, verticalScale);
    QTransform currentTransform = transform();
    // cheat, just the X scale, no rotation compensation
    double currentScale = currentTransform.m11();
    double newScale = scale / currentScale;
    QTransform scaledTransform = currentTransform.scale(newScale, newScale);
    setTransform(scaledTransform);
    
    QGraphicsView::resizeEvent(event);
}

