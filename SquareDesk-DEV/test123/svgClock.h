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
#include <QDateTime>

#include "clickablelabel.h"
#include "typetracker.h"
#include "svgDial.h"  // for QGraphicsArcItem

#include "globaldefines.h"

// ---------
#define SHOWSECONDHAND
//#define SMOOTHROTATION

#define TIMERNOTEXPIRED     (0x00)
#define LONGTIPTIMEREXPIRED (0x01)
#define BREAKTIMEREXPIRED   (0x02)
#define THIRTYSECWARNING    (0x04)

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
    
    // Sleep/wake handling methods
    void clearMissedMinutes(const QDateTime &lastTime, const QDateTime &currentTime);
    void updateClockDisplay();

//    int currentSongType; // TEST: setType()

    // TIMER LABEL HANDLING -------------
    bool tipLengthAlarm;
    bool breakLengthAlarm;

    bool tipLengthTimerEnabled;  // TODO: rename to patterLengthTimerEnabled
    bool tipLength30secEnabled;  // TODO: rename
    int tipLengthAlarmMinutes;   // TODO: rename to patterLengthAlarmMinutes
    bool breakLengthTimerEnabled;
    int breakLengthAlarmMinutes;

    unsigned int currentTimerState;  // contains bits for the LONG TIP timer and the BREAK TIMER

    QString singingCallSection;

    bool editModeSD;
    clickableLabel *timerLabelDark;
    clickableLabel *timerLabelCuesheet;
    QLabel *timerLabelSD; // intentionally not clickable

    void setTimerLabel(clickableLabel *theLabelCuesheet, QLabel *theLabelSD, clickableLabel *theLabelDark);
    void setSDEditMode(bool e);
    void setSingingCallSection(QString s);
    void handleTimerLabels(); // do whatever is needed (called by updateClock())

    TypeTracker typeTracker;
    void resetPatter(void);
    void goToState(QString newStateName);
    // ------------------------------------

public slots:
    void customMenuRequested(QPoint pos);

private slots:
    void clearClockColoring();

signals:
    void newState(QString stateName); // INBREAK, INSINGER, etc.

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
    
    // Sleep/wake detection variables
    QDateTime lastUpdateTime;      // Track when updateClock was last called
    bool firstUpdate;              // Flag to handle the first update after startup
};

#endif // SVGCLOCK_H
