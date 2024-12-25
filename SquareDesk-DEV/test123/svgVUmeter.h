#ifndef SVGVUMETER_H
#define SVGVUMETER_H

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
#include <QElapsedTimer>

#include "globaldefines.h"

// -----------------------------------
class svgVUmeter : public QLabel
{
    Q_OBJECT
    QP_V(QColor, bgColor);

public:
    explicit svgVUmeter(QWidget *parent = 0);
    ~svgVUmeter();

    void levelChanged(double peakL_mono, double peakR, bool isMono);

private:
    void updateMeter();

    void paintEvent(QPaintEvent *pe);
    void resizeEvent(QResizeEvent *re);

    QElapsedTimer timer;

    QGraphicsView view;
    QGraphicsScene scene;

    QGraphicsLineItem *greenBarL;
    QGraphicsLineItem *greenBarR;
    QGraphicsLineItem *yellowBarL;
    QGraphicsLineItem *yellowBarR;
    QGraphicsLineItem *redBarL;
    QGraphicsLineItem *redBarR;

    QGraphicsLineItem *grayBarL;
    QGraphicsLineItem *grayBarR;
    QGraphicsLineItem *darkRedBarL;
    QGraphicsLineItem *darkRedBarR;

    QTimer VUmeterTimer;
    QPoint center;

    double valueL, valueR;
    double oldLimitL, oldLimitR; // old values, to prevent updating the lines too often
};

#endif // SVGVUMETER_H
