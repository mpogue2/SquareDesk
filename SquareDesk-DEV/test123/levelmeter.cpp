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

#include "levelmeter.h"

#include <math.h>

#include <QPainter>
#include <QTimer>
#include <QDebug>
#include <QColor>

// Constants
//const int RedrawInterval = 100; // ms
const int RedrawInterval = 50; // ms
const qreal PeakDecayRate = 0.001;
const int PeakHoldLevelDuration = 2000; // ms


LevelMeter::LevelMeter(QWidget *parent)
    :   QWidget(parent)
    ,   m_rmsLevel(0.0)
    ,   m_peakLevel(0.0)
    ,   m_decayedPeakLevel(0.0)
    ,   m_peakDecayRate(PeakDecayRate)
    ,   m_peakHoldLevel(0.0)
    ,   m_redrawTimer(new QTimer(this))
    ,   m_rmsColor(Qt::red)
    ,   m_peakColor(255, 200, 200, 255)
{
//    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
//    setMinimumWidth(30);

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
    update();
}

void LevelMeter::levelChanged(qreal rmsLevel, qreal peakLevel, int numSamples)
{
//    qDebug() << "levelChanged";
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

    update();
}

void LevelMeter::redrawTimerExpired()
{
    // Decay the peak signal
    const int elapsedMs = m_peakLevelChanged.elapsed();
    const qreal decayAmount = m_peakDecayRate * elapsedMs;
    if (decayAmount < m_peakLevel)
        m_decayedPeakLevel = m_peakLevel - decayAmount;
    else
        m_decayedPeakLevel = 0.0;

    // Check whether to clear the peak hold level
    if (m_peakHoldLevelChanged.elapsed() > PeakHoldLevelDuration)
        m_peakHoldLevel = 0.0;

    update();
}

void LevelMeter::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

//    qDebug() << "vu paint";

    QPainter painter(this);
//    painter.fillRect(rect(), Qt::black);  // background color of VU Meter
    painter.fillRect(rect(), Qt::lightGray);  // background color of VU Meter
//    qDebug() << "RECT:" << rect();
//    QRect bar = rect();

//    bar.setTop(rect().top() + (1.0 - m_peakHoldLevel) * rect().height());
//    bar.setBottom(bar.top() + 2);  // height of the peak indicator
//    painter.fillRect(bar, m_rmsColor);
//    bar.setBottom(rect().bottom());

//    bar.setTop(rect().top() + (1.0 - m_decayedPeakLevel) * rect().height());
//    painter.fillRect(bar, m_peakColor);

//    bar.setTop(rect().top() + (1.0 - m_rmsLevel) * rect().height());
//    painter.fillRect(bar, m_rmsColor);

//    // 10 bars, painted from the bottom up
//    // 6 green, 2 yellow, 2 red
//    QColor barColor;
//    const unsigned int numBoxes = 20;
//    for (unsigned int i = 0; i < numBoxes; i++) {
//        QRect bar = rect();
//        int bot = rect().bottom() - ((float)(i+1)/(float)numBoxes)*rect().height();
//        int top = rect().bottom() - ((float)(i)/(float)numBoxes)*rect().height();
//        bar.setBottom(bot);
//        bar.setTop(top);
//        bar.setLeft(1);
//        bar.setRight(rect().right()-1);
////        qDebug() << bar;
//        if (i < 0.6 * numBoxes) {
//            barColor = Qt::green;
//        } else if (i < 0.8 * numBoxes) {
//            barColor = Qt::yellow;
//        } else {
//            barColor = Qt::red;
//        }
//        if (m_decayedPeakLevel*(float)numBoxes >= (i+1)) {
//            painter.fillRect(bar, barColor);
//        } else {
//            break;  // break out of the for loop early, if no more boxes to draw
//        }
//    }

    // 10 bars, painted from left to right
    // 6 green, 2 yellow, 2 red
    QColor barColor;
    const unsigned int numBoxes = 20;
    for (unsigned int i = 0; i < numBoxes; i++) {
        QRect bar = rect();
//        int bot = rect().bottom() - ((float)(i+1)/(float)numBoxes)*rect().height();
//        int top = rect().bottom() - ((float)(i)/(float)numBoxes)*rect().height();

//        bar.setBottom(bot);
//        bar.setTop(top);
//        bar.setLeft(1);
//        bar.setRight(rect().right()-1);

//        int bot = rect().right() - ((float)(i+1)/(float)numBoxes)*rect().width();
//        int top = rect().right() - ((float)(i)/(float)numBoxes)*rect().width();
        int bot = rect().left() + ((float)(i)/(float)numBoxes)*rect().width();
        int top = rect().left() + ((float)(i+1)/(float)numBoxes)*rect().width();
//        bar.setLeft(bot);
//        bar.setRight(top);
        bar.setLeft(top);
        bar.setRight(bot);
        bar.setTop(1);
        bar.setBottom(rect().bottom()-1);

//        qDebug() << bar;
        if (i < 0.6 * numBoxes) {
            barColor = Qt::green;
        } else if (i < 0.8 * numBoxes) {
            barColor = Qt::yellow;
        } else {
            barColor = Qt::red;
        }
        if (m_decayedPeakLevel*(float)numBoxes >= (i+1)) {
            painter.fillRect(bar, barColor);
        } else {
            break;  // break out of the for loop early, if no more boxes to draw
        }
    }

}
