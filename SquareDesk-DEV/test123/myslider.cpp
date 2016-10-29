#include "myslider.h"

// ==========================================================================================
// TODO: Move this out into its own file: MySlider.cpp
MySlider::MySlider(QWidget *parent) : QSlider(parent)
{
    drawLoopPoints = false;
    origin = 0;
}

void MySlider::SetLoop(bool b)
{
    drawLoopPoints = b;
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

    if(drawLoopPoints) {
        int offset = 8;  // for the handles
        int height = this->height();
        int width = this->width() - 2 * offset;

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

    QSlider::paintEvent(e);         // parent draws
}

