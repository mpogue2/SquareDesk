#ifndef SVGCLOCK_H
#define SVGCLOCK_H

#include <QWidget>
#include <QLabel>

#include <QSvgRenderer>
#include <QGraphicsEllipseItem>
#include <QGraphicsSvgItem>

#include <QGraphicsView>
#include <QGraphicsScene>

#include <QMouseEvent>
#include <QPoint>
#include <QTimer>

// ---------
//#define SHOWSECONDHAND
//#define SMOOTHROTATION

// -----------------------------------
class svgClock : public QLabel
{
    Q_OBJECT

public:
    explicit svgClock(QWidget *parent = 0);
    ~svgClock();

private:
    void updateClock();

    void paintEvent(QPaintEvent *pe);
    void resizeEvent(QResizeEvent *re);

    QGraphicsView view;
    QGraphicsScene scene;

    QPoint center;
    double radius;

    QGraphicsSimpleTextItem *t12;
    QGraphicsSimpleTextItem *t3;
    QGraphicsSimpleTextItem *t6;
    QGraphicsSimpleTextItem *t9;

    QGraphicsSimpleTextItem *digitalTime;

    QGraphicsLineItem *hourHand;
    QGraphicsLineItem *minuteHand;
    QGraphicsLineItem *secondHand;
    QGraphicsEllipseItem *secondDot;

    QTimer secondTimer;

    double lengthOfHourHand, lengthOfMinuteHand, lengthOfSecondHand, lengthOfShortSecondHand;

};

#endif // SVGCLOCK_H
