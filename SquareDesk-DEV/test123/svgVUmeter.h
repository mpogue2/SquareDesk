/****************************************************************************
**
** Copyright (C) 2016-2024 Mike Pogue, Dan Lyke
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

#ifndef SVGVUMETER_H
#define SVGVUMETER_H

#include <QWidget>
#include <QLabel>

#include <QSvgRenderer>
#include <QGraphicsEllipseItem>
#include <QGraphicsSvgItem>

#include <QGraphicsView>
#include <QGraphicsScene>

#include <QMouseEvent>
#include <QPoint>
#include <QTimer>
#include <QElapsedTimer>

#include "globaldefines.h"

// -----------------------------------
class svgVUmeter : public QLabel
{
    Q_OBJECT
    QP_V(QColor, bgColor);

public:
    explicit svgVUmeter(QWidget *parent = 0);
    ~svgVUmeter();

    void levelChanged(double peakL_mono, double peakR, bool isMono);

private:
    void updateMeter();

    void paintEvent(QPaintEvent *pe);
    void resizeEvent(QResizeEvent *re);

    QElapsedTimer timer;

    QGraphicsView view;
    QGraphicsScene scene;

    QGraphicsLineItem *greenBarL;
    QGraphicsLineItem *greenBarR;
    QGraphicsLineItem *yellowBarL;
    QGraphicsLineItem *yellowBarR;
    QGraphicsLineItem *redBarL;
    QGraphicsLineItem *redBarR;

    QGraphicsLineItem *grayBarL;
    QGraphicsLineItem *grayBarR;
    QGraphicsLineItem *darkRedBarL;
    QGraphicsLineItem *darkRedBarR;

    QTimer VUmeterTimer;
    QPoint center;

    double valueL, valueR;
    double oldLimitL, oldLimitR; // old values, to prevent updating the lines too often
};

#endif // SVGVUMETER_H
