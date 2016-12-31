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

#include "myslider.h"
#include "QDebug"

// ==========================================================================================
MySlider::MySlider(QWidget *parent) : QSlider(parent)
{
    drawLoopPoints = false;
    singingCall = false;
    float defaultSingerLengthInBeats = (64 * 7 + 24);
    introPosition = (float)(16 / defaultSingerLengthInBeats);
    outroPosition = (float)(1.0 - 8 / defaultSingerLengthInBeats );
    origin = 0;
}

void MySlider::SetLoop(bool b)
{
    drawLoopPoints = b;
    update();
}

void MySlider::SetIntro(float intro)
{
    introPosition = intro;
}

void MySlider::SetOutro(float outro)
{
    outroPosition = outro;
}

float MySlider::GetIntro() const
{
    return introPosition;
}
float MySlider::GetOutro() const
{
    return outroPosition;
}

void MySlider::SetSingingCall(bool b)
{
    singingCall = b;
    update();
}


void MySlider::SetOrigin(int newOrigin)
{
    origin = newOrigin;
}

void MySlider::mouseDoubleClickEvent(QMouseEvent *event)  // FIX: this doesn't work
{
    Q_UNUSED(event)
    setValue(origin);
    valueChanged(origin);
}

// http://stackoverflow.com/questions/3894737/qt4-how-to-draw-inside-a-widget
void MySlider::paintEvent(QPaintEvent *e)
{
#ifdef Q_OS_LINUX
    QSlider::paintEvent(e);         // parent draws
#endif // ifdef Q_OS_LINUX
    QPainter painter(this);
    int offset = 8;  // for the handles
    int height = this->height();
    int width = this->width() - 2 * offset;

    if (drawLoopPoints) {
        QPen pen;  //   = (QApplication::palette().dark().color());
        pen.setColor(Qt::blue);
        painter.setPen(pen);

        float from = 0.9f;
        QLineF line1(from * width + offset, 3,          from * width + offset, height-4);
        QLineF line2(from * width + offset, 3,          from * width + offset - 5, 3);
        QLineF line3(from * width + offset, height-4,   from * width + offset - 5, height-4);
        painter.drawLine(line1);
        painter.drawLine(line2);
        painter.drawLine(line3);

        float to = 0.1f;
        QLineF line4(to * width + offset, 3,          to * width + offset, height-4);
        QLineF line5(to * width + offset, 3,          to * width + offset + 5, 3);
        QLineF line6(to * width + offset, height-4,   to * width + offset + 5, height-4);
        painter.drawLine(line4);
        painter.drawLine(line5);
        painter.drawLine(line6);
    }

    if (singingCall) {
        QPen pen;
        pen.setWidth(3);

        int middle = height/2;
#if defined(Q_OS_WIN)
        // Windows sliders are drawn differently, so colors are just
        //   under the horizontal bar on Windows (only).  Otherwise
        //    the colors don't draw at all. :-(
        middle += 2;  // only on Windows...
#endif
        QColor colorEnds = QColor("#e4da20");  // dark yellow, visible on both Mac and Windows
        QColor colors[3] = { Qt::red, Qt::blue, QColor("#7cd38b") };
        float left = 1;
        float right = width + offset + 5;
#if defined(Q_OS_MAC)
        left += 1;   // pixel perfection on Mac OS X Sierra
#endif
#if defined(Q_OS_WIN)
        right += 1;  // pixel perfection on Windows 10
#endif
#ifdef Q_OS_LINUX
        right -= 4;
        left += 5;
#endif // ifdef Q_OS_LINUX

        float width = (right - left);
        float endIntro = left + introPosition * width;
        float startEnding = left  + outroPosition * width;
        int segments = 7;
        float lengthSection = (startEnding - endIntro)/(float)segments;

        // section 1: Opener
        pen.setColor(colorEnds);
        painter.setPen(pen);
        QLineF line1(left, middle, endIntro, middle);
        painter.drawLine(line1);

        for (int i = 0; i < segments; ++i) {
            pen.setColor(colors[i % (sizeof(colors) / sizeof(*colors))]);
            painter.setPen(pen);
            QLineF line2(endIntro + i * lengthSection, middle,
                         endIntro + (i + 1) * lengthSection, middle);
            painter.drawLine(line2);
        }

        // section 7: Closer
        pen.setColor(colorEnds);
        painter.setPen(pen);
        QLineF line7(startEnding, middle, right, middle);
        painter.drawLine(line7);

#ifdef Q_OS_LINUX
        // redraw the handle
        pen.setWidth(1);
        pen.setColor(Qt::gray);
        painter.setPen(pen);
        painter.setBrush(Qt::white);

        float position = value();
        float min = minimum();
        float max = maximum();
        float pixel_position = left + width * position / (max - min);
        float handle_size = 13;
        QRectF rectangle(pixel_position - handle_size / 2,
                         middle - handle_size / 2,
                         handle_size,
                         handle_size);
        painter.drawEllipse(rectangle);
#endif
    }
#ifndef Q_OS_LINUX
    QSlider::paintEvent(e);         // parent draws
#endif // ifndef Q_OS_LINUX
}

