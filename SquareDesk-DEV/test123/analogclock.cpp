/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

#include "analogclock.h"
#include <QDebug>

AnalogClock::AnalogClock(QWidget *parent)
    : QWidget(parent)
{
    analogClockTimer = new QTimer(this);
    connect(analogClockTimer, SIGNAL(timeout()), this, SLOT(redrawTimerExpired()));
    analogClockTimer->start(1000);

    // initially, all minutes have no session color
    for (int i = 0; i < 60; i++) {
        typeInMinute[i] = NONE;
        lastHourSet[i] = -1;     // invalid hour, matches no real hour
    }

    coloringIsHidden = true;
    tipLengthAlarm = false;
    breakLengthAlarm = false;

    tipLengthAlarmMinutes = 100;  // if >60, disabled
    tipLengthAlarmMinutes = 100;  // if >60, disabled
}

void AnalogClock::redrawTimerExpired()
{
    update();
}

void AnalogClock::setColorForType(int type, QColor theColor)
{
    colorForType[type] = theColor;
}

void AnalogClock::paintEvent(QPaintEvent *)
{
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
    QTime time = QTime::currentTime();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(width() / 2, height() / 2);
    painter.scale(side / 200.0, side / 200.0);

    // digital part of the analog clock -----
    QString theTime = time.toString("h:mmA");
    theTime.replace("AM","");
    theTime.replace("PM","");  // cheesy way to get 12 hour clock string...
//    if ((time.second() % 2) == 0)
//        theTime.replace(":"," ");  // FIX: this doesn't work well, because ":" and " " have different widths in the default font

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
    int startMinute = time.second();  // FIX FIX FIX FIX FIX **********
#endif

    // check for LONG PATTER ALARM -------
    int numMinutesPatter = 0;

    for (int i = 0; i < 60; i++) {
        int currentMinute = startMinute - i;
        if (currentMinute < 0) {
            currentMinute += 60;
        }
        if (typeInMinute[currentMinute] == PATTER) {
            numMinutesPatter++;
        } else {
            break;  // break out of the for loop, because we're only looking at the first segment
        }
    }

    if (numMinutesPatter >= tipLengthAlarmMinutes) {
        tipLengthAlarm = true;
    } else {
        tipLengthAlarm = false;
    }

    // check for END OF BREAK -------
    int numMinutesBreak = 0;

    for (int i = 0; i < 60; i++) {
        int currentMinute = startMinute - i;
        if (currentMinute < 0) {
            currentMinute += 60;
        }
        if (typeInMinute[currentMinute] == NONE) {
            numMinutesBreak++;
        } else {
            break;  // break out of the for loop, because we're only looking at the first segment
        }
    }

    if (numMinutesBreak >= breakLengthAlarmMinutes && numMinutesBreak < 60) {
        breakLengthAlarm = true;
    } else {
        breakLengthAlarm = false;
    }

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

void AnalogClock::setSegment(int hour, int minute, int type)
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
}

void AnalogClock::setHidden(bool hidden)
{
    coloringIsHidden = hidden;
}

