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

#include <QFile>
#include <QMenu>
#include <QRegularExpression>
#include <QTime>
#include "globaldefines.h"
#include "svgClock.h"
#include "math.h"

// *** NOTE: To debug the svgClock, set DEBUGCLOCK in globaldefines.h

// -------------------------------------
// from: https://stackoverflow.com/questions/7537632/custom-look-svg-gui-widgets-in-qt-very-bad-performance
svgClock::svgClock(QWidget *parent) :
    QLabel(parent)
{
    // TIMER LABEL HANDLING ---------------
    timerLabelCuesheet = nullptr;
    timerLabelSD = nullptr;
    timerLabelDark = nullptr;
    currentTimerState = TIMERNOTEXPIRED;
    singingCallSection = "";
    // ------------------------------------

    sethourHandColor(QColor("#000"));
    setminuteHandColor(QColor("#000"));
    setsecondHandColor(QColor("#000"));
    setnumberColor(QColor("#000"));
    settickColor(QColor("#000"));

    view.setStyleSheet("background: transparent; border: none");
    view.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    QFont  clockFont("Arial", 16);
    QBrush numberBrush(numberColor());

    QFont  digitalTimeFont("Arial", 24);
    QBrush digitalTimeBrush(digitalTimeColor());

//    QRect extent = this->geometry();
    double clockSize = 170;
    view.setFixedSize(clockSize, clockSize);
    QRect extent(0, 0, clockSize * 0.9, clockSize * 0.9);

    center = extent.center();
    radius = extent.width()/2.2;

//    qDebug() << "geometry: " << extent << center << radius;

    // NUMBERS ---------
    t12 = new QGraphicsSimpleTextItem("12");
    t12->setFont(clockFont);
    t12->setBrush(numberBrush);
    QRectF r12 = t12->boundingRect();
//    qDebug() << "r12" << r12;
    t12->setPos(center - QPoint(0, radius) - r12.center() + QPoint(0, -3)); // 3 pixel correction
    scene.addItem(t12);

    t3 = new QGraphicsSimpleTextItem("3");
    t3->setFont(clockFont);
    t3->setBrush(numberBrush);
    QRectF r3 = t3->boundingRect();
//    qDebug() << "r3" << r3;
    t3->setPos(center + QPoint(radius, 0) - r3.center());
    scene.addItem(t3);

    t6 = new QGraphicsSimpleTextItem("6");
    t6->setFont(clockFont);
    t6->setBrush(numberBrush);
    QRectF r6 = t6->boundingRect();
//    qDebug() << "r6" << r6;
    t6->setPos(center + QPoint(0, radius) - r6.center() + QPoint(0, 3)); // 3 pixel correction
    scene.addItem(t6);

    t9 = new QGraphicsSimpleTextItem("9");
    t9->setFont(clockFont);
    t9->setBrush(numberBrush);
    QRectF r9 = t9->boundingRect();
//    qDebug() << "r9" << r9;
    t9->setPos(center - QPoint(radius, 0) - r9.center());
    scene.addItem(t9);

    // TICKS -----------
#ifdef DEBUG_LIGHT_MODE
    QPen tickPen(tickColor(), 1, Qt::SolidLine);
#else
    QPen tickPen(QColor("#a0a0a0"), 1, Qt::SolidLine);
#endif

    double tickRotationDegrees[8] = {30.0, 60.0, 120.0, 150.0, 210.0, 240.0, 300.0, 330.0};

    for (int i = 0; i < 8; i++) {
        double tickRotationDegrees2 = tickRotationDegrees[i];
//        qDebug() << "tickRotationDegrees:" << i << tickRotationDegrees2;
        tick[i] = new QGraphicsLineItem(
                                        center.rx() + radius * 1.0 * sin(M_PI * (tickRotationDegrees2/180.0)),
                                        center.ry() - radius * 1.0 * cos(M_PI * (tickRotationDegrees2/180.0)),
                                        center.rx() + radius * 1.05 * sin(M_PI * (tickRotationDegrees2/180.0)),
                                        center.ry() - radius * 1.05 * cos(M_PI * (tickRotationDegrees2/180.0))
                                        );
        tick[i]->setPen(tickPen);
        scene.addItem(tick[i]);
    }

    // DIGITAL PART ------
    digitalTime = new QGraphicsSimpleTextItem("XX:YY");
    digitalTime->setFont(digitalTimeFont);
    digitalTime->setBrush(digitalTimeBrush);
    QRectF rD = digitalTime->boundingRect();
    //    qDebug() << "r12" << r12;
    digitalTime->setPos(center + QPoint(6, 25) - rD.center());
    scene.addItem(digitalTime);

    // CLOCK COLORING ----------
#define ARCPENSIZE (6.0)

    double arcRadius = 60;
    for (int i = 0; i < 60; i++) {
        QColor arcColor(Qt::red);
        // 6 degrees per: 1 open + 4 on + 1 open
        arc[i] = new QGraphicsArcItem(16, 15, 2 * arcRadius, 2 * arcRadius);
        arc[i]->setPen(QPen(arcColor, ARCPENSIZE, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));  // NOTE: arcColor is QColor
        arc[i]->setStartAngle(16.0 * (90.0 - 6 * i - 2.0));  // -2.0 centers the arc[0] at 12:00
        arc[i]->setSpanAngle(16.0 * 4);
        scene.addItem(arc[i]);
    }

    // HANDS ------------
    double hourRotationDegrees = 0.0;
    double minuteRotationDegrees = 0.0;

//    QPen hourPen(QColor("#DB384A"), 4, Qt::SolidLine, Qt::RoundCap);
//    QPen hourPen(QColor("#606060"), 4, Qt::SolidLine, Qt::RoundCap);
//    QPen minutePen(QColor("#E0E0E0"), 2, Qt::SolidLine, Qt::RoundCap);
#ifdef DEBUG_LIGHT_MODE
    QPen hourPen(hourHandColor(), 4, Qt::SolidLine, Qt::RoundCap);
    QPen minutePen(minuteHandColor(), 2, Qt::SolidLine, Qt::RoundCap);
#else
    QPen hourPen(QColor("#A0A0A0"), 4, Qt::SolidLine, Qt::RoundCap);
    QPen minutePen(QColor("#D0D0D0"), 2, Qt::SolidLine, Qt::RoundCap);
#endif

#ifdef SHOWSECONDHAND
    double secondRotationDegrees = 0.0;
#endif
#ifdef DEBUG_LIGHT_MODE
    // qDebug() << "secondHandCOlor:" << secondHandColor();
    QPen secondPen(secondHandColor(), 1.2, Qt::SolidLine, Qt::RoundCap);
#else
    QPen secondPen(Qt::white, 1.2, Qt::SolidLine, Qt::RoundCap);
#endif

    lengthOfSecondHand = 0.85;
#ifdef SHOWSECONDHAND
    lengthOfShortSecondHand = 0.25;
#endif
    lengthOfMinuteHand = lengthOfSecondHand * (49.0/52.0);  // was: 46/52
    lengthOfHourHand   = lengthOfSecondHand * (32.0/52.0);

    hourHand = new QGraphicsLineItem(center.rx(),
                                     center.ry(),
                                     center.rx() + radius * lengthOfHourHand * sin(M_PI * (hourRotationDegrees/180.0)),
                                     center.ry() - radius * lengthOfHourHand * cos(M_PI * (hourRotationDegrees/180.0))
                                     );
    hourHand->setPen(hourPen);
    scene.addItem(hourHand);

    minuteHand = new QGraphicsLineItem(center.rx(),
                                       center.ry(),
                                       center.rx() + radius * lengthOfMinuteHand * sin(M_PI * (minuteRotationDegrees/180.0)),
                                       center.ry() - radius * lengthOfMinuteHand * cos(M_PI * (minuteRotationDegrees/180.0))
                                       );
    minuteHand->setPen(minutePen);
    scene.addItem(minuteHand);

#ifdef SHOWSECONDHAND
    secondHand = new QGraphicsLineItem(center.rx() - radius * lengthOfShortSecondHand * sin(M_PI * (secondRotationDegrees/180.0)),
                                       center.ry() + radius * lengthOfShortSecondHand * cos(M_PI * (secondRotationDegrees/180.0)),
                                       center.rx() + radius * lengthOfSecondHand * sin(M_PI * (secondRotationDegrees/180.0)),
                                       center.ry() - radius * lengthOfSecondHand * cos(M_PI * (secondRotationDegrees/180.0))
                                       );
    secondHand->setPen(secondPen);
    scene.addItem(secondHand);
#endif

    double dotSize = 3;
    secondDot = new QGraphicsEllipseItem(center.rx() - dotSize, center.ry() - dotSize, 2.0 * dotSize, 2.0 * dotSize);
    secondDot->setPen(secondPen);
    secondDot->setBrush(QBrush(Qt::white));
    scene.addItem(secondDot);

    // ---------------------
    view.setDisabled(true);

    view.setScene(&scene);
    view.setParent(this, Qt::FramelessWindowHint);

    //    currentSongType = NONE; // TEST: not sure what the type is yet

    clearClockColoring();  // NOTE: do this only after the arc[] are initialized

    // UPDATE TO INITIAL TIME ------
    updateClock();

    // update once per second after now...
    connect(&this->secondTimer, &QTimer::timeout,
            this, [this](){
//                        qDebug() << "TIMER!";
                        updateClock();
                      } );
#ifdef SHOWSECONDHAND
#ifdef SMOOTHROTATION
    secondTimer.start(100); // every 100ms
#else
    secondTimer.start(1000); // every second
#endif
#else

#ifdef DEBUGCLOCK
    secondTimer.start(1.0 * 1000); // every second
#else
    secondTimer.start(1.0 * 1000); // every 60 seconds
#endif

#endif

}

// ========================================================================================================================
void svgClock::updateClock() {
    // HANDS -----------
    QTime theTime = QTime::currentTime();

#ifdef DEBUG_LIGHT_MODE
    QPen hourPen(hourHandColor(), 4, Qt::SolidLine, Qt::RoundCap);
    QPen minutePen(minuteHandColor(), 2, Qt::SolidLine, Qt::RoundCap);
    QPen secondPen(secondHandColor(), 1.2, Qt::SolidLine, Qt::RoundCap);
    QPen tickPen(tickColor(), 1, Qt::SolidLine);

    QBrush numberBrush(numberColor());
    QBrush digitalTimeBrush(digitalTimeColor());

    hourHand->setPen(hourPen);
    minuteHand->setPen(minutePen);
    secondHand->setPen(secondPen);

    digitalTime->setBrush(digitalTimeBrush);

    t3->setBrush(numberBrush);
    t6->setBrush(numberBrush);
    t9->setBrush(numberBrush);
    t12->setBrush(numberBrush);

    // TODO: allow changing of tick color (need to move tick processing down here) ********
#endif

#ifdef SMOOTHROTATION
    // seconds smoothly go round -----
    double hourRotationDegrees = 30.0 * (theTime.hour() + (theTime.minute()/60.0) + (theTime.second()/3600.0));
    double minuteRotationDegrees = 360 * ((theTime.minute()/60.0) + (theTime.second()/3600.0));
#ifdef SHOWSECONDHAND
    double secondRotationDegrees = 360 * ((theTime.second() + theTime.msec()/1000.0)/60.0);
#endif
#else

#ifdef DEBUGCLOCK
    // seconds tick one at a time -----
    double hourRotationDegrees = 30.0 * (theTime.minute() + (theTime.second()/60.0));
    double minuteRotationDegrees = 360 * ((theTime.second()/60.0));

//    minuteRotationDegrees = (theTime.second() % 4) * 90.0;
    // qDebug() << "UpdateClock:" << hourRotationDegrees << minuteRotationDegrees;
#else
    // seconds tick one at a time -----
    double hourRotationDegrees = 30.0 * (theTime.hour() + (theTime.minute()/60.0));
    double minuteRotationDegrees = 360 * ((theTime.minute()/60.0));
#endif

#ifdef SHOWSECONDHAND
    double secondRotationDegrees = 360 * ((theTime.second())/60.0);
#endif
#endif

    hourHand->setLine(center.rx(),
                                     center.ry(),
                                     center.rx() + radius * lengthOfHourHand * sin(M_PI * (hourRotationDegrees/180.0)),
                                     center.ry() - radius * lengthOfHourHand * cos(M_PI * (hourRotationDegrees/180.0))
                                     );

    minuteHand->setLine(center.rx(),
                                       center.ry(),
                                       center.rx() + radius * lengthOfMinuteHand * sin(M_PI * (minuteRotationDegrees/180.0)),
                                       center.ry() - radius * lengthOfMinuteHand * cos(M_PI * (minuteRotationDegrees/180.0))
                                       );

#ifdef SHOWSECONDHAND
    secondHand->setLine(center.rx() - radius * lengthOfShortSecondHand * sin(M_PI * (secondRotationDegrees/180.0)),
                                       center.ry() + radius * lengthOfShortSecondHand * cos(M_PI * (secondRotationDegrees/180.0)),
                                       center.rx() + radius * lengthOfSecondHand * sin(M_PI * (secondRotationDegrees/180.0)),
                                       center.ry() - radius * lengthOfSecondHand * cos(M_PI * (secondRotationDegrees/180.0))
                                       );
#endif

    // DIGITAL TIME ------
    QLocale myLocale;
    QString theLocaleTimeFormat = myLocale.timeFormat(QLocale::NarrowFormat);
    QString digitalTimeString = theTime.toString(theLocaleTimeFormat).replace(" AM", "").replace(" PM", ""); // Europe uses 24 hour time, US = 12 hr time (no AM/PM)
    digitalTime->setText(digitalTimeString);

    // now center it
    QRectF digitalTimeRect = digitalTime->boundingRect();
    digitalTime->setPos(center + QPoint(0, 25) - digitalTimeRect.center());

    // CLOCK COLORING ----------------
//    setSegment(theTime.hour(), theTime.minute(), theTime.second(), currentSongType); // TEST: whenever the minute hand moves, update the new segment with the right color IMMEDIATELY

//    // hide them all to start
//    for (int i = 0; i < 60; i++) {
//        arc[i]->setVisible(false);
//    }

    // time now!
#ifndef DEBUGCLOCK
    int startMinute = theTime.minute();
#else
    int startMinute = theTime.second();
#endif

    if (!coloringIsHidden) {
        for (int i = 0; i < 60; i++) {
            int currentMinute = startMinute - i;
            if (currentMinute < 0) {
                currentMinute += 60;
            }
            if (typeInMinute[currentMinute] != NONE) {
                if (arc[currentMinute] != nullptr) {
//                    qDebug() << "****** arc[" << currentMinute << "] = " << currentMinute << typeInMinute[currentMinute] << colorForType[typeInMinute[currentMinute]];
//                    arc[currentMinute]->setBrush(colorForType[typeInMinute[currentMinute]]);
                    arc[currentMinute]->setPen(QPen(colorForType[typeInMinute[currentMinute]], ARCPENSIZE, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));  // NOTE: arcColor is QColor
                    arc[currentMinute]->setVisible(true);
                }
            } else {
                // arc is hidden
                if (arc[currentMinute] != nullptr) {
//                    qDebug() << "HIDDEN arc[" << currentMinute << "] = " << currentMinute << typeInMinute[currentMinute] << colorForType[typeInMinute[currentMinute]];
                    arc[currentMinute]->setVisible(false);
//#define DEBUGCOLORING
#ifdef DEBUGCOLORING
//                    arc[currentMinute]->setPen(QPen(QColor(Qt::red), 3.0, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));  // NOTE: arcColor is QColor // DEBUG
                    arc[currentMinute]->setVisible(true); // DEBUG
#endif
                }
            }
        }
    }

    // TIMER LABEL HANDLING -----------
    handleTimerLabels();
}

svgClock::~svgClock()
{
}

// ------------------------------------
void svgClock::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED(pe)
}

void svgClock::resizeEvent(QResizeEvent *re)
{
    QLabel::resizeEvent(re);
}

// CLOCK COLORING ------
void svgClock::setSegment(unsigned int hour, unsigned int minute, unsigned int second, unsigned int type)
{
//    qDebug() << "svgClock::setSegment:" << hour << minute << second << type;

    // TODO: I have discovered a truly marvelous idea for showing session activity on the clock face,
    //    which this margin is too narrow to contain....
    if (type == NONE) {
        if (hour != lastHourSet[minute]) {
            lastHourSet[minute] = hour;  // remember that we set this segment for this hour
            typeInMinute[minute] = NONE; //   clear out the color, as the minute hand goes around, but only ONCE
        }
    }
    else {
        // normal type, always update the current minute
        typeInMinute[minute] = type;
    }

    // TODO: use the timeSegmentList instead of the typeInMinute[] and lastHourSet[] to do Clock Coloring

    Q_UNUSED(second)
    typeTracker.addSecond(type);  // NOTE: THIS clock needs to call typeTracker here, because analogClock no longer does it
    // typeTracker.printState("svgClock::setSegment():"); // DEBUG
}

void svgClock::setColorForType(int type, QColor theColor) {
    colorForType[type] = theColor;
}

void svgClock::setHidden(bool hidden) {
    coloringIsHidden = hidden;
}

void svgClock::clearClockColoring() {
    for (int i = 0; i < 60; i++) {
        typeInMinute[i] = NONE;
        lastHourSet[i] = -1;     // invalid hour, matches no real hour
    }
    updateClock(); // immediately update the clock coloring
}

void svgClock::setTheme(QString s) {
    currentThemeString = s;
}

void svgClock::customMenuRequested(QPoint pos){
    Q_UNUSED(pos)

    QMenu *menu = new QMenu(this);

    menu->setProperty("theme", currentThemeString);

    QAction action1("Clear clock coloring", this);
    connect(&action1, SIGNAL(triggered()), this, SLOT(clearClockColoring()));
    menu -> addAction(&action1);

    menu->popup(QCursor::pos());
    menu->exec();

    delete(menu);
}

void svgClock::finishInit() {
    // TICKS -----------
#ifdef DEBUG_LIGHT_MODE
    QPen tickPen(tickColor(), 1, Qt::SolidLine);
#else
    QPen tickPen(QColor("#a0a0a0"), 1, Qt::SolidLine);
#endif

    // qDebug() << "tickColor is now: " << tickColor();
    for (int i = 0; i < 8; i++) {
        tick[i]->setPen(tickPen); // set the colors as per QSS
    }

}

// TIMER LABEL HANDLING ---------------------------
void svgClock::handleTimerLabels() {
    // qDebug() << "***** handleTimerLabels *****";

    // Check for LONG PATTER ALARM
    int patterLengthSecs = typeTracker.currentPatterLength();
    unsigned int secs = patterLengthSecs;
    unsigned int mm = (unsigned int)(secs/60);
    unsigned int ss = secs - 60*mm;

// BREAK timer is a count-down timer to 00:00 (end of break)
//   but it keeps on counting

// DEBUG: change to 1 seconds per minute of patter, just for debugging
//#define DEBUGCLOCK
#ifdef DEBUGCLOCK
#define SECSPERMIN  1
#else
#define SECSPERMIN  60
#endif

    int breakLengthSecs = typeTracker.currentBreakLength();
    int b_secs = breakLengthAlarmMinutes*SECSPERMIN - breakLengthSecs;  // seconds to go in the break; to debug, change 60 to 5
    int b_mm = abs(b_secs/SECSPERMIN);
    int b_ss = abs(b_secs) - SECSPERMIN*abs(b_mm);  // always positive

    //    qDebug() << "BREAK PIECES: " << breakLengthSecs << b_secs << b_mm << b_ss;

    // To debug, change 60 to 5
    int maxPatterLength = tipLengthAlarmMinutes * SECSPERMIN;  // the user's preference for MAX PATTER LENGTH (converted to seconds)
    int maxBreakLength = breakLengthAlarmMinutes * SECSPERMIN;  // the user's preference for MAX BREAK LENGTH (converted to seconds)

    // DDD(maxPatterLength)
    // DDD(maxBreakLength)

    currentTimerState = TIMERNOTEXPIRED;  // clear clear
    if (timerLabelSD != nullptr && timerLabelCuesheet != nullptr && timerLabelDark != nullptr) {
        if (patterLengthSecs == -1 || !tipLengthTimerEnabled) {
            // if not patter, or the patter timer is disabled
            if (breakLengthSecs == -1 || !breakLengthTimerEnabled) {
                // AND if also it's not break or the break timer is disabled
                //timerLabel->setVisible(false);  // make the timerLabel disappear
                // it's either a singing call, or we're stopped.  Leave it VISIBLE.

                // setTimerLabelColor("red");  // set all 3 labels to red, only if they are not already red (to save CPU cycles)

                goToState("IN_SINGER");

                if (singingCallSection != "") {
                    timerLabelSD->setVisible(!editModeSD);  // make the timerLabelSD appear if we're NOT in editing mode
                    timerLabelSD->setText(singingCallSection);

                    timerLabelCuesheet->setVisible(true);  // make the timerLabelCuesheet appear
                    timerLabelCuesheet->setText(singingCallSection);

                    timerLabelDark->setVisible(true);  // make the timerLabelDark appear
                    timerLabelDark->setText(singingCallSection);
                    update(); // FIX: IS THIS NEEDED?
                } else {
                    timerLabelCuesheet->setText("");
                    timerLabelSD->setText("");
                    timerLabelDark->setText("");

                    // if (timerLabelCuesheet->text() != "") {
                    //     timerLabelCuesheet->setText("");
                    //     timerLabelDark->setText("");
                    // }
                    // if (timerLabelSD->text() != "") {
                    //     timerLabelSD->setText("");
                    // }
                }
            } else if (breakLengthSecs < maxBreakLength && typeTracker.timeSegmentList.length()>=2 && typeTracker.timeSegmentList.at(0).type == NONE) {
                // it is for sure a BREAK, the break timer is enabled, and it's under the break time limit,
                //   and we played something before the break, and we're currently in state NONE (NOTE: can't use patter or singing calls or extras as break music)
                // qDebug() << "for sure a BREAK";
                timerLabelDark->setVisible(true);
                timerLabelDark->setText(QString("") + QString("%1").arg(b_mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(b_ss, 2, 10, QChar('0')));

                timerLabelSD->setVisible(!editModeSD); // make it visible if we are NOT in edit mode
                timerLabelSD->setText(QString("") + QString("%1").arg(b_mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(b_ss, 2, 10, QChar('0')));

                timerLabelCuesheet->setVisible(true);
                timerLabelCuesheet->setText(QString("") + QString("%1").arg(b_mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(b_ss, 2, 10, QChar('0')));

                // if (darkmode) {
                //     setTimerLabelColor("#8080FF");
                // } else {
                //     setTimerLabelColor("blue");
                // }

                goToState("IN_BREAK");

                currentTimerState &= ~LONGTIPTIMEREXPIRED;  // clear
                currentTimerState &= ~BREAKTIMEREXPIRED;  // clear
            } else if (typeTracker.timeSegmentList.length()>=2 && typeTracker.timeSegmentList.at(0).type == NONE) {
                // the break has expired.  We played something before the break, and we're currently in NONE state.
                //                qDebug() << "expired BREAK";
                // timerLabel->setVisible(true);
                timerLabelCuesheet->setVisible(true);
                timerLabelDark->setVisible(true);
                timerLabelSD->setVisible(!editModeSD); // make it visible if SD is NOT in edit mode

                // setTimerLabelColor("red"); // turns red when break is over
#ifdef DEBUG_LIGHT_MODE
                goToState("BREAK_OVER");
#endif \
    // alternate the time (negative now), and "END BREAK"
                if (b_ss % 2 == 0) {
                    // timerLabel->setText("End BRK");
                    timerLabelDark->setText("End BRK");
                    timerLabelSD->setText("End BRK");
                    timerLabelCuesheet->setText("End BRK");
                } else {
                    QString newtext = QString("-") + QString("%1").arg(b_mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(b_ss, 2, 10, QChar('0'));
                    timerLabelDark->setText(newtext);
                    timerLabelSD->setText(newtext);
                    timerLabelCuesheet->setText(newtext);
                }
                currentTimerState &= ~LONGTIPTIMEREXPIRED;  // clear
                currentTimerState |= BREAKTIMEREXPIRED;  // set
                // TODO: optionally play a sound, or start the next song
            } else {
                // either we have 1 state known (e.g. NONE for 3600 secs), OR
                //   we are not in None state right now, and we know what we're doing (e.g. playing Extras or Singers as break music)
                //                qDebug() << "none state";
                timerLabelCuesheet->setText("");
                timerLabelDark->setText("");
                timerLabelSD->setText("");
            }
        } else if (patterLengthSecs < maxPatterLength) {
            //            qDebug() << "patter under the time limit";
            // UNDER THE TIME LIMIT
            // timerLabel->setVisible(true);
            // timerLabel->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));

            QString newtext = QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0'));

            timerLabelDark->setVisible(true);
            timerLabelDark->setText(newtext);

            timerLabelSD->setVisible(!editModeSD); // make visible if SD is NOT in edit mode
            timerLabelSD->setText(newtext);

            timerLabelCuesheet->setVisible(true);
            timerLabelCuesheet->setText(newtext);

            // if (darkmode) {
            //     setTimerLabelColor("#D0D0D0");
            // } else {
            //     setTimerLabelColor("black");
            // }

            goToState("IN_PATTER");

            currentTimerState &= ~LONGTIPTIMEREXPIRED;  // clear
            currentTimerState &= ~BREAKTIMEREXPIRED;    // clear

#ifdef DEBUGCLOCK
            qDebug() << "30 sec warning: " << patterLengthSecs << maxPatterLength-30;
#endif
            if (patterLengthSecs > maxPatterLength-30) {
                currentTimerState |= THIRTYSECWARNING;  // set
            }

        } else if (patterLengthSecs < maxPatterLength + 15) {  // it will be red for 15 seconds, before also starting to flash "LONG TIP"
            // qDebug() << "patter OVER the time limit";
            // OVER THE TIME LIMIT, so make the time-in-patter RED.
            // DDD(patterLengthSecs)
            // DDD(maxPatterLength+15)
            QString newtext = QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0'));

            timerLabelDark->setVisible(true);
            timerLabelDark->setText(newtext);

            timerLabelSD->setVisible(!editModeSD); // make it visible if SD is NOT in edit mode
            timerLabelSD->setText(newtext);

            timerLabelCuesheet->setVisible(true);
            timerLabelCuesheet->setText(newtext);

            goToState("PATTER_OVER");

            currentTimerState |= LONGTIPTIMEREXPIRED;  // set
            currentTimerState &= ~BREAKTIMEREXPIRED;    // clear
            // TODO: play a sound, if enabled
        } else {
            // DDD(patterLengthSecs)
            // DDD(maxPatterLength+15)
            // qDebug() << "patter REALLY over the time limit";
            // REALLY OVER THE TIME LIMIT!!  So, flash "LONG TIP" alternately with the time-in-patter.

            timerLabelCuesheet->setVisible(true);
            timerLabelDark->setVisible(true);
            timerLabelSD->setVisible(!editModeSD); // make visible if SD is NOT in edit mode

            goToState("LONG_PATTER");

            if (ss % 2 == 0) {
                timerLabelDark->setText("LONG");
                timerLabelSD->setText("LONG");
                timerLabelCuesheet->setText("LONG");
            } else {
                QString newtext = QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0'));
                timerLabelDark->setText(newtext);
                timerLabelSD->setText(newtext);
                timerLabelCuesheet->setText(newtext);
            }
            // TODO: play a more serious sound, if enabled
            currentTimerState |= LONGTIPTIMEREXPIRED;  // set
            currentTimerState &= ~BREAKTIMEREXPIRED;    // clear
        }
    }

    update();  // for future reference, only call update() when absolutely needed.  Most widgets will call update() automatically when
    //   something changes on them.
}

// --------------------------------------------------------------------------
// svgClock wants the 3 or 4 important labels to change color ---------------
// newStateName: { IN_BREAK, BREAK_OVER, IN_SINGER, IN_PATTER, PATTER_OVER, LONG_PATTER }
void svgClock::goToState(QString newStateName) {
#ifdef DEBUGCLOCK
        qDebug() << "svgClock::goToState" << newStateName;
#endif
    emit newState(newStateName);
}

// -------------------------------------------------------------------------
void svgClock::setSDEditMode(bool e) {
    editModeSD = e; // tell the analog clock whether we're in SD edit or playback mode
}

// -------------------------------------------------------------------------
void svgClock::setTimerLabel(clickableLabel *theCuesheetLabel, QLabel *theSDLabel, clickableLabel *theDarkWarningLabel)
{
    // qDebug() << "svgClock::setTimerLabel";
    timerLabelSD = theSDLabel;
    timerLabelSD->setText("STL");
    timerLabelSD->setVisible(true);

    timerLabelCuesheet = theCuesheetLabel;
    timerLabelCuesheet->setText("STL");
    timerLabelCuesheet->setVisible(true);

    timerLabelDark = theDarkWarningLabel;
        timerLabelDark->setText("STL");
        timerLabelCuesheet->setVisible(true);
}

// -------------------------------------------------------------------------
void svgClock::resetPatter(void)
{
    timerLabelSD->setText("00:00");
    timerLabelCuesheet->setText("00:00");
    timerLabelDark->setText("00:00");

    typeTracker.addStop();
}

// -------------------------------------------------------------------------
void svgClock::setSingingCallSection(QString s)
{
    singingCallSection = s;
}
