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

#ifndef SVGCLOCK_H
#define SVGCLOCK_H

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

#include "typetracker.h"
#include "svgDial.h"  // for QGraphicsArcItem

#include "globaldefines.h"

// ---------
#define SHOWSECONDHAND
//#define SMOOTHROTATION

// -----------------------------------
class svgClock : public QLabel
{
    Q_OBJECT
    QP_V(QColor, hourHandColor);
    QP_V(QColor, minuteHandColor);
    QP_V(QColor, secondHandColor);
    QP_V(QColor, tickColor);
    QP_V(QColor, numberColor);
    QP_V(QColor, digitalTimeColor);

public:
    explicit svgClock(QWidget *parent = 0);
    ~svgClock();

    void finishInit(); // color the ticks
    void setSegment(unsigned int hour, unsigned int minute, unsigned int second, unsigned int type);
    unsigned int typeInMinute[60]; // remembers the type for this minute
    unsigned int lastHourSet[60];  // remembers the hour when the type was set for that minute

    void setColorForType(int type, QColor theColor);
    QColor colorForType[10];

    void setHidden(bool hidden);
    bool coloringIsHidden;

    void setTheme(QString s);

//    int currentSongType; // TEST: setType()

public slots:
    void customMenuRequested(QPoint pos);

private slots:
    void clearClockColoring();

private:
    QString currentThemeString;
    void updateClock();

    void paintEvent(QPaintEvent *pe);
    void resizeEvent(QResizeEvent *re);

    QGraphicsView view;
    QGraphicsScene scene;

    QPoint center;
    double radius;

    QGraphicsSimpleTextItem *t12;
    QGraphicsSimpleTextItem *t3;
    QGraphicsSimpleTextItem *t6;
    QGraphicsSimpleTextItem *t9;

    QGraphicsSimpleTextItem *digitalTime;

    QGraphicsLineItem *hourHand;
    QGraphicsLineItem *minuteHand;
    QGraphicsLineItem *secondHand;
    QGraphicsEllipseItem *secondDot;

    QGraphicsArcItem *arc[60];  // 60 of these pointers
    QGraphicsLineItem *tick[8]; //  8 of these
    QTimer secondTimer;

    double lengthOfHourHand, lengthOfMinuteHand, lengthOfSecondHand, lengthOfShortSecondHand;

};

#endif // SVGCLOCK_H
