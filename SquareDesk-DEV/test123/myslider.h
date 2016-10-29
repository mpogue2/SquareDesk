#ifndef MYSLIDER_H
#define MYSLIDER_H

#include <QSlider>
#include <QPainter>
#include <QPen>

// ---------------------------------------------
class MySlider : public QSlider
{

public:
    MySlider(QWidget *parent = 0);
    void SetLoop(bool b);           // turn on loop points
    void SetOrigin(int newOrigin);  // use an origin other than zero, when double-clicked

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);

private:
    bool drawLoopPoints;
    int origin;  // reset to this point when double-clicked

};

#endif // MYSLIDER_H

