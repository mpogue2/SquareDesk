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

#include "myslider.h"
#include "QDebug"
#include <QEvent>
#include <QStyle>
#include <QMouseEvent>
#include <QStyleOptionSlider>
#include <utility>

// ==========================================================================================
MySlider::MySlider(QWidget *parent) : QSlider(parent)
{
//    qDebug() << "MySlider::MySlider()";
    drawLoopPoints = false;
    singingCall = false;
    SetDefaultIntroOutroPositions(false, 0.0, false, 0.0, 0.0, 0.0);  // bare minimum init
    origin = 0;

    else1 = "QSlider::groove:horizontal { border: 1px solid #C0C0C0; height: 2px; margin: 2px 0; background: #C0C0C0; }";  // for EVERYTHING ELSE
    singer1 = "QSlider::groove:horizontal { border: 1px transparent; height: 2px; margin: 2px 0; background: transparent; }";  // for SINGERS
    s2 = "QSlider::handle:horizontal { background: white; border: 1px solid #C0C0C0; height: 16px; width: 16px; margin: -8px 0; border-radius: 8px; padding: -8px 0px; }";
    s2 += "QSlider::handle:horizontal:hover { background-color: #A0A0FF; }";  // TODO: not sure if I like this...
    this->setStyleSheet( else1 + s2 );

//    AddMarker(0.0);  // DEBUG
//    AddMarker(10.0);
//    AddMarker(20.0);
//    DeleteMarker(10.0);
//    qDebug() << GetMarkers();  // DEBUG

    // install the wheel/scroll eater for ALL MySlider's
    this->installEventFilter(this);  // eventFilter() is called, where wheel/touchpad scroll events are eaten

    fusionMode = false;
    setMouseTracking(false);
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
                                             bool songIsSingerType,
                                             double songStart_sec, double songEnd_sec, double songLength_sec)
{
    double introFrac, loopStart_frac;
    double outroFrac, loopEnd_frac;

    if (!tempoIsBPM) {
        // if we don't have an estimatedBPM at all, we have to guess
        if (songIsSingerType) {
            // take a guess on a singer or vocal, when we don't know the BPM
            double defaultSingerLengthInBeats = 16 + (64 * 7) + 8;  // 16 beat intro + 7 64-beat sections + 8 beat tag
            introPosition = (16 / defaultSingerLengthInBeats);           // 0.0 - 1.0
            outroPosition = (1.0 - 8 / defaultSingerLengthInBeats );     // 0.0 - 1.0
        } else {
            // take a guess on a patter or xtra or unknown, when we don't know the BPM
            introPosition = 0.033;         // 0.0 - 1.0
            outroPosition = 1.0 - 0.033;   // 0.0 - 1.0
        }
        // we need to keep both updated, updateBgPixmap uses Frac's
        introFrac = introPosition;
        outroFrac = outroPosition;
    } else {
        if (songIsSingerType) {
            // it's SINGER or VOCAL type, so let's estimate using BPM
            // we are using BPM (not %)
            //        double songLength_beats = (songEnd_sec - songStart_sec)/60.0 * estimatedBPM;
            double phraseLength_beats = 64.0;  // everything is 64-beat phrases
            double phraseLength_sec = 60.0 * phraseLength_beats/estimatedBPM;

            double loopStart_sec = songStart_sec + 0.033 * (songEnd_sec - songStart_sec);
            loopStart_frac = loopStart_sec/songLength_sec;   // 0.0 - 1.0

            double loopEnd_sec = loopStart_sec + 7 * phraseLength_sec; // exactly 7 sections, probably
            loopEnd_frac = loopEnd_sec/songLength_sec;  // 0.0 - 1.0

        } else {
            // it's PATTER type, so let's estimate using BPM
            // we are using BPM (not %)
            //        double songLength_beats = (songEnd_sec - songStart_sec)/60.0 * estimatedBPM;
            double phraseLength_beats = 32.0;  // everything is 32-beat phrases
            double phraseLength_sec = 60.0 * phraseLength_beats/estimatedBPM;
            //        double songLength_phrases = songLength_beats/phraseLength_beats;
            double loopStart_sec = songStart_sec + 0.033 * (songEnd_sec - songStart_sec);
            loopStart_frac = loopStart_sec/songLength_sec;   // 0.0 - 1.0

            int numPhrasesThatFit = static_cast<int>((songEnd_sec - loopStart_sec)/phraseLength_sec);
            double loopEnd_sec = loopStart_sec + phraseLength_sec * numPhrasesThatFit;
            loopEnd_frac = loopEnd_sec/songLength_sec;  // 0.0 - 1.0

            if (songLength_sec - loopEnd_sec < 7) {
                // if too close to the end of the song (because an integer number of phrases just happens to fit)
                //  average phrase is about 15 sec, so 7 is a little less than 1/2 of a phrase
                loopEnd_sec -= phraseLength_sec;  // go back one
                loopEnd_frac = loopEnd_sec/songLength_sec;  // and recalculate it
                //            qDebug() << "TOO CLOSE TRIGGERED!";
            }
        }
        // we need to keep both updated, updateBgPixmap uses Frac's
        introFrac = loopStart_frac;
        outroFrac = loopEnd_frac;
    }

    introPosition = introFrac;
    outroPosition = outroFrac;
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

int MySlider::GetOrigin()
{
    return(origin);
}

// when clicking inside the mySlider, jump directly there
void MySlider::mousePressEvent(QMouseEvent *event)
{
    if (!fusionMode) {
        // Light Mode works correctly, because it's NOT Fusion
        return QSlider::mousePressEvent(event);
    }

    // DARK MODE:

    // from: https://stackoverflow.com/questions/52689047/moving-qslider-to-mouse-click-position
    QStyleOptionSlider opt = QStyleOptionSlider();
    initStyleOption(&opt);

    QRect gr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

    int sliderLength;
    int sliderMin;
    int sliderMax;
    int p;

    QPoint pr = event->pos() - sr.center() + sr.topLeft();

    if (orientation() == Qt::Horizontal) {
        sliderLength = sr.width();
        sliderMin = gr.x();
        sliderMax = gr.right() - sliderLength + 1;
        p = pr.x();
    } else {
        sliderLength = sr.height();
        sliderMin = gr.y();
        sliderMax = gr.bottom() - sliderLength + 1;
        p = pr.y();
    }

    int value = QStyle::sliderValueFromPosition(minimum(), maximum(), p - sliderMin, sliderMax - sliderMin, opt.upsideDown);

    if (event->button() == Qt::LeftButton) {
        setValue(value);
    } else {
        return QSlider::mousePressEvent(event);
    }
}

void MySlider::mouseDoubleClickEvent(QMouseEvent *event)  // FIX: this doesn't work
{
    Q_UNUSED(event)
    setValue(origin);
    emit valueChanged(origin);
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
    int middle = height/2;
    double left = 1;
    double right = width + offset + 5;

    // CUSTOM TICK MARKS ===============================
    //   NOTE: This whole section is hand-tuned.  I'm not sure why what Qt plots doesn't match the docs.
    QPen pen1;
    pen1.setWidth(2);
    pen1.setColor("#c0c0c0");
    painter.setPen(pen1);

    int tickInt = tickInterval();
    int incr1 = (tickInt > 0 ? tickInt : 10);
    for (int i = minimum(); i < maximum() + incr1; i += incr1) {
        int pos1 = QStyle::sliderPositionFromValue(minimum(), maximum(), i, this->width() - 22) + 11;
        if (i == origin) {
            int extra = 10;
            painter.drawLine(pos1, middle - 5 - extra, pos1, middle + 5 + extra);
        } else {
            painter.drawLine(pos1, middle - 5, pos1, middle + 5);
        }
    }

    // SINGING CALL SECTION COLORS =====================
    if (singingCall && !previousSingingCall) {
        this->setStyleSheet( singer1 + s2 );
    } else if (!singingCall && previousSingingCall) {
        this->setStyleSheet( else1 + s2 );
    } else {
        // no change, so do nothing.
    }
    previousSingingCall = singingCall;  // cache this

    if (singingCall) {
        QPen pen;
        pen.setWidth(8);
#if defined(Q_OS_WIN)
        pen.setWidth(10);
#endif
//        int middle = height/2 - 2;
#if defined(Q_OS_WIN)
        // Windows sliders are drawn differently, so colors are just
        //   under the horizontal bar on Windows (only).  Otherwise
        //    the colors don't draw at all. :-(
        middle += -1;  // only on Windows...
#endif
        QColor colorEnds = QColor("#e4da20");  // dark yellow, visible on both Mac and Windows
//        QColor colors[3] = { Qt::red, Qt::blue, QColor("#7cd38b") };
        QColor colors[3] = { Qt::red, Qt::cyan, Qt::blue };

//        double left = 1;
//        double right = width + offset + 5;
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

//        middle += 2;

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

        for (const auto &markerPos : std::as_const(markers)) {
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
        if (!fusionMode) {
            pen.setColor(Qt::blue); // Light mode is fine with regular blue
        } else {
            pen.setColor(QColor("#26A4ED")); // Dark Mode needs to be much brighter
        }
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

void MySlider::setFusionMode(bool b) {
    fusionMode = b;
}
