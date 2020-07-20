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

