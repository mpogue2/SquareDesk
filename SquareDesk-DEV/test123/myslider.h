#ifndef MYSLIDER_H
#define MYSLIDER_H

// ---------------------------------------------
class MySlider : public QSlider
{

public:
    MySlider(QWidget *parent = 0);
    void SetLoop(bool b);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);

private:
    bool drawLoopPoints;

};

#endif // MYSLIDER_H

