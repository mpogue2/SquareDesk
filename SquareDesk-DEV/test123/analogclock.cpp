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

#include "globaldefines.h"

#include <QtWidgets>
#include <QLocale>

#include "analogclock.h"
#include <QDebug>

AnalogClock::AnalogClock(QWidget *parent)
    : QWidget(parent)
{
    analogClockTimer = new QTimer(this);
    connect(analogClockTimer, SIGNAL(timeout()), this, SLOT(redrawTimerExpired()));
    analogClockTimer->start(1000);

    // initially, all minutes have no session color
//    for (int i = 0; i < 60; i++) {
//        typeInMinute[i] = NONE;
//        lastHourSet[i] = -1;     // invalid hour, matches no real hour
//    }

    clearClockColoring();

    coloringIsHidden = true;
//    tipLengthAlarm = false;
//    breakLengthAlarm = false;

//    tipLengthAlarmMinutes = 100;  // if >60, disabled

//    typeTracker.printState("AnalogClock::AnalogClock()");

    currentTimerState = TIMERNOTEXPIRED;

    singingCallSection = "";
}

void AnalogClock::setTimerLabelColor(QString col1) {
    if (lastTimerLabelColor != col1) {
        // only change it, if it is actually changing

        QString col = col1;
        if (darkmode && col == "red") {
            col = "#F04040"; // dark mode "red" is more muted, so as not to blind the user
        }

//        qDebug() << "setting TimerLabelColors to: " << col;
        QString style = "QLabel { color : " + col + "; }";

        timerLabel->setAlignment(Qt::AlignCenter);          // NOTE: these never change, so sticking them here, called rarely
        timerLabelSD->setAlignment(Qt::AlignCenter);
        timerLabelCuesheet->setAlignment(Qt::AlignCenter);

        timerLabel->setStyleSheet(style);
        timerLabelSD->setStyleSheet(style);
        timerLabelCuesheet->setStyleSheet(style);

#ifdef DARKMODE
        timerLabelDark->setAlignment(Qt::AlignRight);
        timerLabelDark->setStyleSheet(style);
#endif

        lastTimerLabelColor = col1;
    }
}

void AnalogClock::redrawTimerExpired()
{
//    qDebug() << "redrawTimerExpired";
//    if (timerLabel != NULL) {
//        if (time.second() % 2 == 0) {
//            timerLabel->setText("12345");
//        } else {
//            timerLabel->setText("54321");
//        }
//    }

    // THIS NEEDS TO BE HERE, OR IT WILL CAUSE LOTS OF PAINT EVENTS

    // NEWSTYLE: check for LONG PATTER ALARM
    int patterLengthSecs = typeTracker.currentPatterLength();
    unsigned int secs = patterLengthSecs;
    unsigned int mm = (unsigned int)(secs/60);
    unsigned int ss = secs - 60*mm;
    // TODO: Patter timer should change to count-down timer

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

    currentTimerState = TIMERNOTEXPIRED;  // clear clear
    if (timerLabel != NULL && timerLabelCuesheet != NULL) {
        if (patterLengthSecs == -1 || !tipLengthTimerEnabled) {
            // if not patter, or the patter timer is disabled
            if (breakLengthSecs == -1 || !breakLengthTimerEnabled) {
                // AND if also it's not break or the break timer is disabled
                //timerLabel->setVisible(false);  // make the timerLabel disappear
                // it's either a singing call, or we're stopped.  Leave it VISIBLE.

                setTimerLabelColor("red");  // set all 3 labels to red, only if they are not already red (to save CPU cycles)

                if (singingCallSection != "") {
                    timerLabel->setVisible(true);  // make the timerLabel appear
                    timerLabel->setText(singingCallSection);
                    timerLabelSD->setVisible(!editModeSD);  // make the timerLabelSD appear if we're NOT in editing mode
                    timerLabelSD->setText(singingCallSection);
                    timerLabelCuesheet->setText(singingCallSection);

#ifdef DARKMODE
                    timerLabelDark->setVisible(true);  // make the timerLabelDark appear
                    timerLabelDark->setText(singingCallSection);
#endif
                    update();
                } else {
                    if (timerLabel->text() != "") {
                        timerLabel->setText("");
#ifdef DARKMODE
                        timerLabelDark->setText("");
#endif
                    }
                    if (timerLabelSD->text() != "") {
                        timerLabelSD->setText("");
                    }
                }
            } else if (breakLengthSecs < maxBreakLength && typeTracker.timeSegmentList.length()>=2 && typeTracker.timeSegmentList.at(0).type == NONE) {
                // it is for sure a BREAK, the break timer is enabled, and it's under the break time limit,
                //   and we played something before the break, and we're currently in state NONE (NOTE: can't use patter or singing calls or extras as break music)
//                qDebug() << "for sure a BREAK";
                timerLabel->setVisible(true);
                timerLabel->setText(QString("") + QString("%1").arg(b_mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(b_ss, 2, 10, QChar('0')));

#ifdef DARKMODE
                timerLabelDark->setVisible(true);
                timerLabelDark->setText(QString("") + QString("%1").arg(b_mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(b_ss, 2, 10, QChar('0')));
#endif

                timerLabelSD->setVisible(!editModeSD); // make it visible if we are NOT in edit mode
                timerLabelSD->setText(QString("") + QString("%1").arg(b_mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(b_ss, 2, 10, QChar('0')));

                timerLabelCuesheet->setText(QString("") + QString("%1").arg(b_mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(b_ss, 2, 10, QChar('0')));

                if (darkmode) {
                    setTimerLabelColor("#8080FF");
                } else {
                    setTimerLabelColor("blue");
                }

                currentTimerState &= ~LONGTIPTIMEREXPIRED;  // clear
                currentTimerState &= ~BREAKTIMEREXPIRED;  // clear
            } else if (typeTracker.timeSegmentList.length()>=2 && typeTracker.timeSegmentList.at(0).type == NONE) {
                // the break has expired.  We played something before the break, and we're currently in NONE state.
//                qDebug() << "expired BREAK";
                timerLabel->setVisible(true);
#ifdef DARKMODE
                timerLabelDark->setVisible(true);
#endif
                timerLabelSD->setVisible(!editModeSD); // make it visible if SD is NOT in edit mode

                setTimerLabelColor("red"); // turns red when break is over

                // alternate the time (negative now), and "END BREAK"
                if (b_ss % 2 == 0) {
                    timerLabel->setText("End BRK");
#ifdef DARKMODE
                    timerLabelDark->setText("End BRK");
#endif
                    timerLabelSD->setText("End BRK");
                    timerLabelCuesheet->setText("End BRK");
                } else {
                    timerLabelCuesheet->setText(QString("-") + QString("%1").arg(b_mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(b_ss, 2, 10, QChar('0')));
                }
                currentTimerState &= ~LONGTIPTIMEREXPIRED;  // clear
                currentTimerState |= BREAKTIMEREXPIRED;  // set
                // TODO: optionally play a sound, or start the next song
            } else {
                // either we have 1 state known (e.g. NONE for 3600 secs), OR
                //   we are not in None state right now, and we know what we're doing (e.g. playing Extras or Singers as break music)
//                qDebug() << "none state";
                timerLabel->setText("");
#ifdef DARKMODE
                timerLabelDark->setText("");
#endif
                timerLabelSD->setText("");
            }
        } else if (patterLengthSecs < maxPatterLength) {
//            qDebug() << "patter under the time limit";
            // UNDER THE TIME LIMIT
            timerLabel->setVisible(true);
            timerLabel->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));

#ifdef DARKMODE
            timerLabelDark->setVisible(true);
            timerLabelDark->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));
#endif
            timerLabelSD->setVisible(!editModeSD); // make visible if SD is NOT in edit mode
            timerLabelSD->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));

            timerLabelCuesheet->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));

            if (darkmode) {
                setTimerLabelColor("#D0D0D0");
            } else {
                setTimerLabelColor("black");
            }

            currentTimerState &= ~LONGTIPTIMEREXPIRED;  // clear
            currentTimerState &= ~BREAKTIMEREXPIRED;    // clear

#ifdef DEBUGCLOCK
            qDebug() << "30 sec warning: " << patterLengthSecs << maxPatterLength-30;
#endif
            if (patterLengthSecs > maxPatterLength-30) {
                currentTimerState |= THIRTYSECWARNING;  // set
            }

        } else if (patterLengthSecs < maxPatterLength + 15) {  // it will be red for 15 seconds, before also starting to flash "LONG TIP"
//            qDebug() << "patter OVER the time limit";
            // OVER THE TIME LIMIT, so make the time-in-patter RED.
            timerLabel->setVisible(true);
            timerLabel->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));

#ifdef DARKMODE
            timerLabelDark->setVisible(true);
            timerLabelDark->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));
#endif

            timerLabelSD->setVisible(!editModeSD); // make it visible if SD is NOT in edit mode
            timerLabelSD->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));

            timerLabelCuesheet->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));

            setTimerLabelColor("red");

            currentTimerState |= LONGTIPTIMEREXPIRED;  // set
            currentTimerState &= ~BREAKTIMEREXPIRED;    // clear
            // TODO: play a sound, if enabled
        } else {
//            qDebug() << "patter REALLY over the time limit";
            // REALLY OVER THE TIME LIMIT!!  So, flash "LONG TIP" alternately with the time-in-patter.
            timerLabel->setVisible(true);            
#ifdef DARKMODE
            timerLabelDark->setVisible(true);
#endif

            timerLabelSD->setVisible(!editModeSD); // make visible if SD is NOT in edit mode

            setTimerLabelColor("red");

            if (ss % 2 == 0) {
                timerLabel->setText("LONG");
#ifdef DARKMODE
                timerLabelDark->setText("LONG");
#endif
                timerLabelSD->setText("LONG");
                timerLabelCuesheet->setText("LONG");
            } else {
                timerLabel->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));
#ifdef DARKMODE
                timerLabelDark->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));
#endif
                timerLabelSD->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));
                timerLabelCuesheet->setText(QString("") + QString("%1").arg(mm, 2, 10, QChar('0')) + ":" + QString("%1").arg(ss, 2, 10, QChar('0')));
            }
            // TODO: play a more serious sound, if enabled
            currentTimerState |= LONGTIPTIMEREXPIRED;  // set
            currentTimerState &= ~BREAKTIMEREXPIRED;    // clear
        }
    }

    update();  // for future reference, only call update() when absolutely needed.  Most widgets will call update() automatically when
    //   something changes on them.
}

void AnalogClock::setColorForType(int type, QColor theColor)
{
    colorForType[type] = theColor;
}

void AnalogClock::paintEvent(QPaintEvent *)
{
//    qDebug() << "clock::paintEvent";  // IMPORTANT: make sure we only get a couple of these per second
    QTime time = QTime::currentTime();

    static const QPoint hourHand[3] = {
        QPoint(7, 8),
        QPoint(-7, 8),
        QPoint(0, -45)  // original 40
    };

    static const QPoint minuteHand[3] = {
        QPoint(7, 8),
        QPoint(-7, 8),
        QPoint(0, -80)  // original 70
    };

    static const QPoint secondHand[3] = {
        QPoint(2, 2),
        QPoint(-2, 2),
        QPoint(0, -90)  // original 70
    };

    static const QPointF oneSegment[7] = {
        QPointF(-5.233, -99.863),  // sin/cos(3 degrees)
        QPointF(0.0,   -100.0),
        QPointF(5.233,  -99.863),

        QPointF(4.814,  -91.874),  // sin/cos(3 degrees), R=92
        QPointF(0.0,    -92.0),
        QPointF(-4.814, -91.874),

        QPointF(-5.233, -99.863)
    };

    QColor hourColor(0,0,0);
    QColor minuteColor(0,0,0, 127);
    QColor minuteTickColor(0,0,0);
    QColor secondColor(127,0,0, 200);

    int side = qMin(width(), height());

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(width() / 2, height() / 2);
    painter.scale(side / 200.0, side / 200.0);

    // digital part of the analog clock -----
    QLocale myLocale;
    QString theLocaleTimeFormat = myLocale.timeFormat(QLocale::NarrowFormat);

    QString theTime = time.toString(theLocaleTimeFormat); // Europe uses 24 hour time, US = 12 hr time (no AM/PM)
    theTime = theTime.replace("AM","").replace("PM", "").simplified();
//    theTime.replace("PM","").simplified();  // cheesy way to get 12 hour clock string...

//    QString theTime = time.toString("H:mm"); // Europe uses 24 hour time (use system pref)

    QPointF pt(0, 45);
    painter.setPen(Qt::blue);

    QFont font; // = ui->currentTempoLabel->font();
#if defined(Q_OS_MAC)
    font.setPointSize(35);
#elif defined(Q_OS_WIN32)
    font.setPointSize(20);
#elif defined(Q_OS_LINUX)
    font.setPointSize(20);  // FIX: is this right?
#endif
    painter.setFont(font);

    const qreal size = 32767.0;

    qreal x=pt.x();
    qreal y=pt.y();
    QPointF corner(x, y - size);
    corner.rx() -= size/2.0;
    corner.ry() += size/2.0;
    QRectF rect(corner, QSizeF(size, size));
    painter.drawText(rect, Qt::AlignVCenter | Qt::AlignHCenter, QString(theTime), 0);

    // time now!
#ifndef DEBUGCLOCK
    int startMinute = time.minute();
#else
    int startMinute = time.second();
#endif

    // SESSION SEGMENTS: what were we doing over the last hour?
    if (!coloringIsHidden) {
        for (int i = 0; i < 60; i++) {
            int currentMinute = startMinute - i;
            if (currentMinute < 0) {
                currentMinute += 60;
            }

            if (typeInMinute[currentMinute] != NONE) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(colorForType[typeInMinute[currentMinute]]);
                painter.save();
                painter.rotate(6.0 * currentMinute + 3.0);
                painter.drawConvexPolygon(oneSegment, 7);
                painter.restore();
            }
        }
    }

    // HOUR HAND
    painter.setPen(Qt::NoPen);
    painter.setBrush(hourColor);

    painter.save();
    painter.rotate(30.0 * ((time.hour() + time.minute() / 60.0)));
    painter.drawConvexPolygon(hourHand, 3);
    painter.restore();

    // HOUR TICKS
    painter.setPen(hourColor);
    for (int i = 0; i < 12; ++i) {
        painter.drawLine(88, 0, 96, 0);
        painter.rotate(30.0);
    }

    // MINUTE HAND
    painter.setPen(Qt::NoPen);
    painter.setBrush(minuteColor);

    painter.save();
    painter.rotate(6.0 * (time.minute() + time.second() / 60.0));
    painter.drawConvexPolygon(minuteHand, 3);
    painter.restore();

    // MINUTE TICKS
    painter.setPen(minuteTickColor);
    for (int j = 0; j < 60; ++j) {
        if ((j % 5) != 0) {
            painter.drawLine(92, 0, 96, 0);
        }
        painter.rotate(6.0);
    }

    // SECOND HAND
    painter.setPen(Qt::NoPen);
    painter.setBrush(secondColor);

    painter.save();
    painter.rotate(6.0 * time.second());
    painter.drawConvexPolygon(secondHand, 3);
    painter.restore();

}

void AnalogClock::setSegment(unsigned int hour, unsigned int minute, unsigned int second, unsigned int type)
{
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
    typeTracker.addSecond(type);
//    typeTracker.printState("AnalogClock::setSegment():");
}

void AnalogClock::setHidden(bool hidden)
{
    coloringIsHidden = hidden;
}

void AnalogClock::setSDEditMode(bool e) {
    editModeSD = e; // tell the analog clock whether we're in SD edit or playback mode
}

void AnalogClock::setTimerLabel(clickableLabel *theLabel, QLabel *theCuesheetLabel, QLabel *theSDLabel, QLabel *theDarkWarningLabel, bool darkmode1)
{
    darkmode = darkmode1; // remember what mode we are in

#ifndef DARKMODE
    Q_UNUSED(theDarkWarningLabel)
#endif

//    qDebug() << "AnalogClock::setTimerLabel";
    timerLabel = theLabel;
    timerLabel->setText("");
    timerLabel->setVisible(true);

    timerLabelSD = theSDLabel;
    timerLabelSD->setText("");
    timerLabelSD->setVisible(true);

    timerLabelCuesheet = theCuesheetLabel;
    timerLabelCuesheet->setText("");
    timerLabelCuesheet->setVisible(true);

#ifdef DARKMODE
    timerLabelDark = theDarkWarningLabel;
    if (timerLabelDark != nullptr) {
        timerLabelDark->setText("");
        timerLabelCuesheet->setVisible(true);
    }
#endif

}

void AnalogClock::resetPatter(void)
{
    timerLabel->setText("00:00");
    timerLabelCuesheet->setText("00:00");

#ifdef DARKMODE
    timerLabelDark->setText("00:00");
#endif

    typeTracker.addStop();
}

void AnalogClock::setSingingCallSection(QString s)
{
    singingCallSection = s;
}

void AnalogClock::customMenuRequested(QPoint pos){
    Q_UNUSED(pos)

    QMenu *menu = new QMenu(this);
    QAction action1("Clear clock coloring", this);
    connect(&action1, SIGNAL(triggered()), this, SLOT(clearClockColoring()));
    menu -> addAction(&action1);

    menu->popup(QCursor::pos());
    menu->exec();

    delete(menu);
}

void AnalogClock::clearClockColoring() {
    for (int i = 0; i < 60; i++) {
        typeInMinute[i] = NONE;
        lastHourSet[i] = -1;     // invalid hour, matches no real hour
    }
}
