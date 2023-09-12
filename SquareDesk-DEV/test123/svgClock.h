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

#include "typetracker.h"
#include "svgDial.h"  // for QGraphicsArcItem

// ---------
#define SHOWSECONDHAND
//#define SMOOTHROTATION

// -----------------------------------
class svgClock : public QLabel
{
    Q_OBJECT

public:
    explicit svgClock(QWidget *parent = 0);
    ~svgClock();

    void setSegment(unsigned int hour, unsigned int minute, unsigned int second, unsigned int type);
    unsigned int typeInMinute[60]; // remembers the type for this minute
    unsigned int lastHourSet[60];  // remembers the hour when the type was set for that minute

    void setColorForType(int type, QColor theColor);
    QColor colorForType[10];

    void setHidden(bool hidden);
    bool coloringIsHidden;

//    int currentSongType; // TEST: setType()

public slots:
    void customMenuRequested(QPoint pos);

private slots:
    void clearClockColoring();

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

    QGraphicsArcItem *arc[60];  // 60 of these pointers

    QTimer secondTimer;

    double lengthOfHourHand, lengthOfMinuteHand, lengthOfSecondHand, lengthOfShortSecondHand;

};

#endif // SVGCLOCK_H
