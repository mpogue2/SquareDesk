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
    QSlider::paintEvent(e);         // parent draws
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
        QColor colorEnds = Qt::yellow;
        QColor colors[3] = { Qt::red, Qt::blue, QColor("#7cd38b") };
        float left = 1;
        float right = width + offset + 5;
#if defined(Q_OS_WIN)
        right += 1;  // only on Windows...
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

        for (int i = 0; i < segments; ++i)
        {
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

    }

}

