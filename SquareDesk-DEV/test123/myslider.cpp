#include "myslider.h"
#include "QDebug"

// ==========================================================================================
MySlider::MySlider(QWidget *parent) : QSlider(parent)
{
    drawLoopPoints = false;
    singingCall = false;
    origin = 0;
}

void MySlider::SetLoop(bool b)
{
    drawLoopPoints = b;
    update();
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
        QColor color1 = Qt::red;
        QColor color2 = Qt::blue;
        QColor color3 = QColor("#7cd38b");
        float left = 1;
        float right = width + offset + 5;
#if defined(Q_OS_WIN)
        right += 1;  // only on Windows...
#endif
        float endIntro = left + 1.2 * (right-left)/7.0;
        float startEnding = right - 1.2 * (right-left)/7.0;
        float lengthSection = (startEnding - endIntro)/5.0;

        // section 1: Opener
        pen.setColor(color1);
        painter.setPen(pen);
        QLineF line1(left, middle, endIntro, middle);
        painter.drawLine(line1);

        // section 2: Heads 1
        pen.setColor(color2);
        painter.setPen(pen);
        QLineF line2(endIntro + 0*lengthSection, middle, endIntro + 1*lengthSection, middle);
        painter.drawLine(line2);

        // section 3: Heads 2
        pen.setColor(color3);
        painter.setPen(pen);
        QLineF line3(endIntro + 1*lengthSection, middle, endIntro + 2*lengthSection, middle);
        painter.drawLine(line3);

        // section 4: Middle break
        pen.setColor(color1);
        painter.setPen(pen);
        QLineF line4(endIntro + 2*lengthSection, middle, endIntro + 3*lengthSection, middle);
        painter.drawLine(line4);

        // section 5: Sides 1
        pen.setColor(color2);
        painter.setPen(pen);
        QLineF line5(endIntro + 3*lengthSection, middle, endIntro + 4*lengthSection, middle);
        painter.drawLine(line5);

        // section 6: Sides 2
        pen.setColor(color3);
        painter.setPen(pen);
        QLineF line6(endIntro + 4*lengthSection, middle, endIntro + 5*lengthSection, middle);
        painter.drawLine(line6);

        // section 7: Closer
        pen.setColor(color1);
        painter.setPen(pen);
        QLineF line7(startEnding, middle, right, middle);
        painter.drawLine(line7);

    }

    QSlider::paintEvent(e);         // parent draws
}

