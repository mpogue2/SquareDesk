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
/****************************************************************************
**
** Copyright (C) 2016, 2017 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk/SquareDeskPlayer application.
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


#ifndef ANALOGCLOCK_H
#define ANALOGCLOCK_H

#include <QWidget>
#include "clickablelabel.h"
#include "typetracker.h"

#define TIMERNOTEXPIRED     (0x00)
#define LONGTIPTIMEREXPIRED (0x01)
#define BREAKTIMEREXPIRED   (0x02)

// set to speed up the clock by 60X
#define DEBUGCLOCK 1

class AnalogClock : public QWidget
{
    Q_OBJECT

public:
    AnalogClock(QWidget *parent = 0);
    QTimer *analogClockTimer;

    void setSegment(unsigned int hour, unsigned int minute, unsigned int second, unsigned int type);
    unsigned int typeInMinute[60]; // remembers the type for this minute
    unsigned int lastHourSet[60];  // remembers the hour when the type was set for that minute

    void setColorForType(int type, QColor theColor);
    QColor colorForType[10];

    void setHidden(bool hidden);
    bool coloringIsHidden;
    bool tipLengthAlarm;
    bool breakLengthAlarm;

    bool tipLengthTimerEnabled;  // TODO: rename to patterLengthTimerEnabled
    int tipLengthAlarmMinutes;   // TODO: rename to patterLengthAlarmMinutes
    bool breakLengthTimerEnabled;
    int breakLengthAlarmMinutes;

    unsigned int currentTimerState;  // contains bits for the LONG TIP timer and the BREAK TIMER

    void setTimerLabel(clickableLabel *theLabel);

    clickableLabel *timerLabel;
    TypeTracker typeTracker;
    void resetPatter(void);

private slots:
    void redrawTimerExpired();

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
};

#endif
