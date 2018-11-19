/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
** Copyright (C) 2016, 2017, 2018 Mike Pogue, Dan Lyke
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

#include "levelmeter.h"

#include <math.h>

#include <QPainter>
#include <QTimer>
#include <QDebug>
#include <QColor>

// Constants
const int RedrawInterval = 100; // ms
const qreal PeakDecayRate = 0.001;
const int PeakHoldLevelDuration = 2000; // ms

LevelMeter::LevelMeter(QWidget *parent)
    :   QWidget(parent)
    ,   m_rmsLevel(0.0)
    ,   m_peakLevel(0.0)
    ,   m_oldDecayedPeakLevel(0.0)
    ,   m_decayedPeakLevel(0.0)
    ,   m_peakDecayRate(PeakDecayRate)
    ,   m_peakHoldLevel(0.0)
    ,   m_redrawTimer(new QTimer(this))
    ,   m_rmsColor(Qt::red)
    ,   m_peakColor(255, 200, 200, 255)
{
    connect(m_redrawTimer, SIGNAL(timeout()), this, SLOT(redrawTimerExpired()));
    m_redrawTimer->start(RedrawInterval);
}

LevelMeter::~LevelMeter()
{

}

void LevelMeter::reset()
{
    m_rmsLevel = 0.0;
    m_peakLevel = 0.0;
    m_decayedPeakLevel = m_oldDecayedPeakLevel = 0.0;
    update();
}

void LevelMeter::levelChanged(qreal rmsLevel, qreal peakLevel, int numSamples)
{
    // Smooth the RMS signal
    const qreal smooth = pow(qreal(0.9), static_cast<qreal>(numSamples) / 256); // TODO: remove this magic number
    m_rmsLevel = (m_rmsLevel * smooth) + (rmsLevel * (1.0 - smooth));

    if (peakLevel > m_decayedPeakLevel) {
        m_peakLevel = peakLevel;
        m_decayedPeakLevel = peakLevel;
        m_peakLevelChanged.start();
    }

    if (peakLevel > m_peakHoldLevel) {
        m_peakHoldLevel = peakLevel;
        m_peakHoldLevelChanged.start();
    }

//    update();  // no update needed here, will be picked up when redrawTimerExpired is called
}

void LevelMeter::redrawTimerExpired()
{
    // Decay the peak signal
    const int elapsedMs = m_peakLevelChanged.elapsed();
    const qreal decayAmount = m_peakDecayRate * elapsedMs;
    if (decayAmount < m_peakLevel) {
        m_decayedPeakLevel = m_peakLevel - decayAmount;
    }
    else {
        m_decayedPeakLevel = 0.0;
    }

    // Check whether to clear the peak hold level
    if (m_peakHoldLevelChanged.elapsed() > PeakHoldLevelDuration) {
        m_peakHoldLevel = 0.0;
    }

    if (m_decayedPeakLevel != m_oldDecayedPeakLevel) {
        // only update, if the level has changed
        update();
//        qDebug() << m_decayedPeakLevel << m_oldDecayedPeakLevel;
        m_oldDecayedPeakLevel = m_decayedPeakLevel;
    }

//    update();
}

void LevelMeter::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

#if defined(Q_OS_MAC)
    painter.fillRect(rect(), QColor(225,225,225));  // background color of VU Meter
#elif defined(Q_OS_WIN)
    painter.fillRect(rect(), QColor(255,255,255));
#else  // Q_OS_LINUX
    painter.fillRect(rect(), QColor(225,225,225));
#endif

    // 10 bars, painted from left to right
    // 6 green, 2 yellow, 2 red
    QColor barColor;
    QColor noBarColor;  // color to use, if level is not that high
    const unsigned int numBoxes = 10;
    for (unsigned int i = 0; i < numBoxes; i++) {
        QRect bar = rect();
        int bot = static_cast<int>(rect().left() + (static_cast<float>(i)/static_cast<float>(numBoxes))*rect().width());
        int top = static_cast<int>(rect().left() + (static_cast<float>(i+1)/static_cast<float>(numBoxes))*rect().width());

        bar.setLeft(top);
        bar.setRight(bot);
        bar.setTop(1 + 5);
        bar.setBottom(rect().bottom()-1 - 5);

        int noBarLevel = 120;

        if (i < 0.6 * numBoxes) {
            barColor = Qt::green;
            noBarColor = QColor(0,noBarLevel,0);
        }
        else if (i < 0.8 * numBoxes) {
            barColor = Qt::yellow;
            noBarColor = QColor(noBarLevel,noBarLevel,0);
        }
        else {
            barColor = Qt::red;
            noBarColor = QColor(noBarLevel,0,0);
        }
        if (m_decayedPeakLevel*static_cast<double>(numBoxes) >= (i+1)) {
            painter.fillRect(bar, barColor);
        }
        else {
//            break;  // break out of the for loop early, if no more boxes to draw
            painter.fillRect(bar, noBarColor);
        }
    }

}
