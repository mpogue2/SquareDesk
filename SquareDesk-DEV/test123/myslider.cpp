/****************************************************************************
**
** Copyright (C) 2016-2021 Mike Pogue, Dan Lyke
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

#include "myslider.h"
#include "QDebug"
#include <QEvent>

// ==========================================================================================
MySlider::MySlider(QWidget *parent) : QSlider(parent)
{
    drawLoopPoints = false;
    singingCall = false;
    SetDefaultIntroOutroPositions(false, 0.0, 0.0, 0.0, 0.0);  // bare minimum init
    origin = 0;

//    AddMarker(0.0);  // DEBUG
//    AddMarker(10.0);
//    AddMarker(20.0);
//    DeleteMarker(10.0);
//    qDebug() << GetMarkers();  // DEBUG

    // install the wheel/scroll eater for ALL MySlider's
    this->installEventFilter(this);  // eventFilter() is called, where wheel/touchpad scroll events are eaten
}

bool MySlider::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj)

    //    qDebug() << "obj: " << obj << ", event: " << event->type();

    if (event->type() == QEvent::Wheel || event->type() == QEvent::Scroll) {
        // eat wheel and scroll events
        return true;
    }
    return false;  // don't stop anything else
}

void MySlider::SetDefaultIntroOutroPositions(bool tempoIsBPM, double estimatedBPM,
                                             double songStart_sec, double songEnd_sec, double songLength_sec)
{
    if (!tempoIsBPM) {
        double defaultSingerLengthInBeats = (64 * 7 + 24);  // 16 beat intro + 8 beat tag = 24
        introPosition = (16 / defaultSingerLengthInBeats);           // 0.0 - 1.0
        outroPosition = (1.0 - 8 / defaultSingerLengthInBeats );     // 0.0 - 1.0
    } else {
        // we are using BPM (not %)
//        double songLength_beats = (songEnd_sec - songStart_sec)/60.0 * estimatedBPM;
        double phraseLength_beats = 32.0;  // everything is 32-beat phrases
        double phraseLength_sec = 60.0 * phraseLength_beats/estimatedBPM;
//        double songLength_phrases = songLength_beats/phraseLength_beats;
        double loopStart_sec = songStart_sec + 0.05 * (songEnd_sec - songStart_sec);
        double loopStart_frac = loopStart_sec/songLength_sec;   // 0.0 - 1.0

        int numPhrasesThatFit = static_cast<int>((songEnd_sec - loopStart_sec)/phraseLength_sec);
        double loopEnd_sec = loopStart_sec + phraseLength_sec * numPhrasesThatFit;
        double loopEnd_frac = loopEnd_sec/songLength_sec;  // 0.0 - 1.0

        if (songLength_sec - loopEnd_sec < 7) {
            // if too close to the end of the song (because an integer number of phrases just happens to fit)
            //  average phrase is about 15 sec, so 7 is a little less than 1/2 of a phrase
            loopEnd_sec -= phraseLength_sec;  // go back one
            loopEnd_frac = loopEnd_sec/songLength_sec;  // and recalculate it
//            qDebug() << "TOO CLOSE TRIGGERED!";
        }

//        qDebug() << "songLength_beats" << songLength_beats;
//        qDebug() << "songLength_phrases" << songLength_phrases;
//        qDebug() << "--------------";
//        qDebug() << "estimatedBPM" << estimatedBPM;
//        qDebug() << "songStart_sec" << songStart_sec;
//        qDebug() << "songEnd_sec" << songEnd_sec;
//        qDebug() << "songLength_sec" << songLength_sec;

//        qDebug() << "phraseLength_beats" << phraseLength_beats;
//        qDebug() << "phraseLength_sec" << phraseLength_sec;
//        qDebug() << "loopStart_sec" << loopStart_sec;
//        qDebug() << "loopStart_frac" << loopStart_frac;
//        qDebug() << "numPhrasesThatFit" << numPhrasesThatFit;
//        qDebug() << "loopEnd_sec" << loopEnd_sec;
//        qDebug() << "loopEnd_frac" << loopEnd_frac;
//        qDebug() << "--------------";

        introPosition = loopStart_frac;
        outroPosition = loopEnd_frac;
    }
}

void MySlider::SetLoop(bool b)
{
    drawLoopPoints = b;
    update();
}

void MySlider::SetIntro(double intro)
{
//    qDebug() << "MySlider::SetIntro: " << intro;
    introPosition = intro;
}

void MySlider::SetOutro(double outro)
{
//    qDebug() << "MySlider::SetOutro: " << outro;
    outroPosition = outro;
}

double MySlider::GetIntro() const
{
    return introPosition;
}
double MySlider::GetOutro() const
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

// MARKERS ==================
void MySlider::AddMarker(double markerLoc) {
    markers.insert(markerLoc);
    drawMarkers = true;
}

void MySlider::DeleteMarker(double markerLoc) {
    markers.remove(markerLoc);
    drawMarkers = markers.size() > 0;  // only draw markers, if there are some
}

QSet<double> MySlider::GetMarkers() {
    return(markers);
}

void MySlider::ClearMarkers() {
    markers.clear();  // clears out all markers
}


// PAINT IT ====================
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

    // SINGING CALL SECTION COLORS =====================
    if (singingCall) {
        QPen pen;
        pen.setWidth(6);
#if defined(Q_OS_WIN)
        pen.setWidth(10);
#endif
        int middle = height/2 - 2;
#if defined(Q_OS_WIN)
        // Windows sliders are drawn differently, so colors are just
        //   under the horizontal bar on Windows (only).  Otherwise
        //    the colors don't draw at all. :-(
        middle += -1;  // only on Windows...
#endif
        QColor colorEnds = QColor("#e4da20");  // dark yellow, visible on both Mac and Windows
//        QColor colors[3] = { Qt::red, Qt::blue, QColor("#7cd38b") };
        QColor colors[3] = { Qt::red, Qt::cyan, Qt::blue };

        double left = 1;
        double right = width + offset + 5;
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

        double width = (right - left);
        double endIntro = left + introPosition * width;
        double startEnding = left  + outroPosition * width;
        int segments = 7;
        double lengthSection = (startEnding - endIntro)/segments;

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

        double position = value();
        double min = minimum();
        double max = maximum();
        double pixel_position = left + width * position / (max - min);
        double handle_size = 13;
        QRectF rectangle(pixel_position - handle_size / 2,
                         middle - handle_size / 2,
                         handle_size,
                         handle_size);
        painter.drawEllipse(rectangle);
#endif
    }

    // MARKERS ----------------
    if (drawMarkers) {
        // if there are any markers
        QPen pen;  //   = (QApplication::palette().dark().color());
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.setRenderHint( QPainter::Antialiasing );  // be nice now!

        foreach (const double &markerPos, markers) {
//            qDebug() << "Drawing: " << markerPos;
            int triangleWidth = 3;
//            QLineF lineM1(markerPos * width + 6, 0, markerPos * width + 6 - triangleWidth, height-16); // left
//            painter.drawLine(lineM1);
//            QLineF lineM2(markerPos * width + 6, 0, markerPos * width + 6 + triangleWidth, height-16); // right
//            painter.drawLine(lineM2);
//            QLineF lineM3(markerPos * width + 6 - triangleWidth, height-16, markerPos * width + 6 + triangleWidth, height-16); // base
//            painter.drawLine(lineM3);

            QPolygon markerShape;
            markerShape << QPoint(markerPos * width + 6, 0)
                        << QPoint(markerPos * width + 6 - triangleWidth, height-16)
                        << QPoint(markerPos * width + 6 + triangleWidth, height-16)
                        << QPoint(markerPos * width + 6, 0);
            painter.setBrush(QBrush(Qt::red));
            painter.drawPolygon(markerShape);
        }
    }

    // LOOP POINTS ----------------
    if (drawLoopPoints) {
        QPen pen;  //   = (QApplication::palette().dark().color());
        pen.setColor(Qt::blue);
        painter.setPen(pen);

        // double to = 0.1f;
        double to = introPosition;
        QLineF line4(to * width + offset, 1,          to * width + offset, height-5);
        QLineF line5(to * width + offset, 1,          to * width + offset + 5, 1);
        QLineF line6(to * width + offset, height-5,   to * width + offset + 5, height-5);
        painter.drawLine(line4);
        painter.drawLine(line5);
        painter.drawLine(line6);

        // double from = 0.9f;
        double from = outroPosition;
//        qDebug() << "repaint: " << from;
        QLineF line1(from * width + offset, 1,          from * width + offset, height-5);
        QLineF line2(from * width + offset, 1,          from * width + offset - 5, 1);
        QLineF line3(from * width + offset, height-5,   from * width + offset - 5, height-5);
        painter.drawLine(line1);
        painter.drawLine(line2);
        painter.drawLine(line3);
    }

    // qDebug() << "tickInterval = " << tickInterval();  // TODO: draw our own ticks here, and setTickPosition to NoTicks

    // FINISH UP -------------------
#ifndef Q_OS_LINUX
    QSlider::paintEvent(e);         // parent draws
#endif // ifndef Q_OS_LINUX
}

