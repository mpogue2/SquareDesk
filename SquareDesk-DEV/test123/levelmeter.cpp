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

#include "levelmeter.h"

#include <math.h>

#include <QPainter>
#include <QTimer>
#include <QDebug>
#include <QColor>

LevelMeter::LevelMeter(QWidget *parent)
    :   QWidget(parent)
    ,   m_peakLevelL_mono(0.0)
    ,   m_peakLevelR(0.0)
    ,   m_isMono(false)
{
}

LevelMeter::~LevelMeter()
{
}

void LevelMeter::reset()
{
    m_peakLevelL_mono = 0.0;
    m_peakLevelR      = 0.0;
    m_isMono = false;

    update();
}

void LevelMeter::levelChanged(double peakL_mono, double peakR, bool isMono)
{
    if ((m_peakLevelL_mono != peakL_mono) ||
        (m_peakLevelR != peakR) ||
        (m_isMono != isMono)) {
        // if something has changed, we'll need to call update(), after we save the new values (otherwise, don't call update)
        m_peakLevelL_mono = peakL_mono;
        m_peakLevelR      = peakR;
        m_isMono = isMono;
        update(); // call update ONLY if something has changed that affects the presentation of the VU meter
    }
}

void LevelMeter::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

    // all platforms use the same background color and frame color
    QRect frame = rect();
    frame.setTop(1+2);
    frame.setBottom(rect().bottom()-1-2);
    painter.fillRect(frame, QColor(0,0,0));  // background color of frame around VU Meter

    // 10 bars, painted from left to right
    // 6 green, 2 yellow, 2 red
    QColor barColor;
    QColor noBarColor;  // color to use, if level is not that high
    const unsigned int numBoxes = 10;

    float levelL_mono_dB = 20.0f * log10f(m_peakLevelL_mono); // NOTE: output of filter for input 1.0 (clipping) is about 0.51
    double highlightBoxesL_mono;

    float levelR_dB = 20.0f * log10f(m_peakLevelR); // NOTE: output of filter for input 1.0 (clipping) is about 0.51
    double highlightBoxesR;

    // iterate through the boxes one at a time, left to right, and color appropriately
    for (unsigned int i = 0; i < numBoxes; i++) {
        QRect barL = rect();
        QRect barR = rect();
        int bot = static_cast<int>(rect().left() + 2 + (static_cast<float>(i)/static_cast<float>(numBoxes))*(rect().width()-4));
        int top = static_cast<int>(rect().left() + 2 + (static_cast<float>(i+1)/static_cast<float>(numBoxes))*(rect().width()-4));

        double halfwayUp = ((1 + 5) + (rect().bottom()-1 - 5))/2.0;

        barL.setLeft(top);
        barL.setRight(bot);
        barL.setTop(1 + 5);
//        barL.setBottom(rect().bottom()-1 - 5);
        if (m_isMono) {
            // it is MONO, L will be used to color the whole bar
            barL.setBottom(rect().bottom()-1 - 5);
        } else {
            // it is STEREO
            barL.setBottom(halfwayUp - 1); // barL will be used for just the top half

            barR.setLeft(top);
            barR.setRight(bot);
            barR.setTop(halfwayUp + 2);
            barR.setBottom(rect().bottom()-1 - 5);
        }


        int noBarLevel = 120;

        // MONO or STEREO
        highlightBoxesL_mono = static_cast<double>(numBoxes) + 2.0 * levelL_mono_dB/6.0 + 0.5;  // 0.5 is to compensate for loss in filter
        if (!m_isMono) {
            // STEREO
            highlightBoxesR      = static_cast<double>(numBoxes) + 2.0 * levelR_dB/6.0 + 0.5;       // 0.5 is to compensate for loss in filter
        }

        if (i < 0.4 * numBoxes) {
            barColor = Qt::green;
            noBarColor = QColor(0,noBarLevel,0);
            // map -12 - 4  to 0 - 4
            highlightBoxesL_mono = (highlightBoxesL_mono + 12.0)/4.0; // override to 9dB per box vs normal 3dB per box
            if (!m_isMono) {
                highlightBoxesR  = (highlightBoxesR      + 12.0)/4.0; // override to 9dB per box vs normal 3dB per box
            }
        }
        else if (i < 0.8 * numBoxes) {
            barColor = Qt::green;
            noBarColor = QColor(0,noBarLevel,0);
        }
        else if (i < 0.9 * numBoxes) {
            barColor = Qt::yellow;
            noBarColor = QColor(noBarLevel,noBarLevel,0);
        }
        else {
            barColor = Qt::red;
            noBarColor = QColor(noBarLevel + 10,0,0);
        }

        // MONO or STEREO
        if (highlightBoxesL_mono >= (i+1)) {
            painter.fillRect(barL, barColor);
        }
        else {
            painter.fillRect(barL, noBarColor);
        }

        if (!m_isMono) {
            // STEREO
            if (highlightBoxesR >= (i+1)) {
                painter.fillRect(barR, barColor);
            }
            else {
                painter.fillRect(barR, noBarColor);
            }
        }
    }

}
