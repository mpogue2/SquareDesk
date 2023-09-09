#include <QTime>
#include "svgClock.h"
#include "math.h"

// -------------------------------------
// from: https://stackoverflow.com/questions/7537632/custom-look-svg-gui-widgets-in-qt-very-bad-performance
svgClock::svgClock(QWidget *parent) :
    QLabel(parent)
{
    view.setStyleSheet("background: transparent; border: none");
    view.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    QFont  clockFont("Arial", 16);
    QBrush numberBrush(QColor("#a0a0a0"));

    QFont  digitalTimeFont("Arial", 24);
//    QBrush digitalTimeBrush(QColor("#7070d0"));
    QBrush digitalTimeBrush(QColor("#7070B0"));

//    QRect extent = this->geometry();
    double clockSize = 170;
    view.setFixedSize(clockSize, clockSize);
    QRect extent(0, 0, clockSize * 0.9, clockSize * 0.9);

    center = extent.center();
    radius = extent.width()/2.2;

//    qDebug() << "geometry: " << extent << center << radius;

    // NUMBERS ---------
    t12 = new QGraphicsSimpleTextItem("12");
    t12->setFont(clockFont);
    t12->setBrush(numberBrush);
    QRectF r12 = t12->boundingRect();
//    qDebug() << "r12" << r12;
    t12->setPos(center - QPoint(0, radius) - r12.center());
    scene.addItem(t12);

    t3 = new QGraphicsSimpleTextItem("3");
    t3->setFont(clockFont);
    t3->setBrush(numberBrush);
    QRectF r3 = t3->boundingRect();
//    qDebug() << "r3" << r3;
    t3->setPos(center + QPoint(radius, 0) - r3.center());
    scene.addItem(t3);

    t6 = new QGraphicsSimpleTextItem("6");
    t6->setFont(clockFont);
    t6->setBrush(numberBrush);
    QRectF r6 = t6->boundingRect();
//    qDebug() << "r6" << r6;
    t6->setPos(center + QPoint(0, radius) - r6.center());
    scene.addItem(t6);

    t9 = new QGraphicsSimpleTextItem("9");
    t9->setFont(clockFont);
    t9->setBrush(numberBrush);
    QRectF r9 = t9->boundingRect();
//    qDebug() << "r9" << r9;
    t9->setPos(center - QPoint(radius, 0) - r9.center());
    scene.addItem(t9);

    // TICKS -----------
    QPen tickPen(QColor("#a0a0a0"), 1, Qt::SolidLine);
    double tickRotationDegrees[8] = {30.0, 60.0, 120.0, 150.0, 210.0, 240.0, 300.0, 330.0};

    for (int i = 0; i < 8; i++) {
        double tickRotationDegrees2 = tickRotationDegrees[i];
//        qDebug() << "tickRotationDegrees:" << i << tickRotationDegrees2;
        QGraphicsLineItem *tick = new QGraphicsLineItem(
                                        center.rx() + radius * 1.0 * sin(M_PI * (tickRotationDegrees2/180.0)),
                                        center.ry() - radius * 1.0 * cos(M_PI * (tickRotationDegrees2/180.0)),
                                        center.rx() + radius * 1.05 * sin(M_PI * (tickRotationDegrees2/180.0)),
                                        center.ry() - radius * 1.05 * cos(M_PI * (tickRotationDegrees2/180.0))
                                        );
        tick->setPen(tickPen);
        scene.addItem(tick);
    }

    // DIGITAL PART ------
    digitalTime = new QGraphicsSimpleTextItem("XX:YY");
    digitalTime->setFont(digitalTimeFont);
    digitalTime->setBrush(digitalTimeBrush);
    QRectF rD = digitalTime->boundingRect();
    //    qDebug() << "r12" << r12;
    digitalTime->setPos(center + QPoint(6, 25) - rD.center());
    scene.addItem(digitalTime);

    // HANDS ------------
    double hourRotationDegrees = 0.0;
    double minuteRotationDegrees = 0.0;

//    QPen hourPen(QColor("#DB384A"), 4, Qt::SolidLine, Qt::RoundCap);
    QPen hourPen(QColor("#606060"), 4, Qt::SolidLine, Qt::RoundCap);
//    QPen minutePen(QColor("#E0E0E0"), 2, Qt::SolidLine, Qt::RoundCap);
    QPen minutePen(QColor("#D0D0D0"), 2, Qt::SolidLine, Qt::RoundCap);

#ifdef SHOWSECONDHAND
    double secondRotationDegrees = 0.0;
#endif
    QPen secondPen(Qt::white, 1.2, Qt::SolidLine, Qt::RoundCap);

    lengthOfSecondHand = 0.85;
#ifdef SHOWSECONDHAND
    lengthOfShortSecondHand = 0.25;
#endif
    lengthOfMinuteHand = lengthOfSecondHand * (50.0/52.0);  // was: 46/52
    lengthOfHourHand   = lengthOfSecondHand * (32.0/52.0);

    hourHand = new QGraphicsLineItem(center.rx(),
                                     center.ry(),
                                     center.rx() + radius * lengthOfHourHand * sin(M_PI * (hourRotationDegrees/180.0)),
                                     center.ry() - radius * lengthOfHourHand * cos(M_PI * (hourRotationDegrees/180.0))
                                     );
    hourHand->setPen(hourPen);
    scene.addItem(hourHand);

    minuteHand = new QGraphicsLineItem(center.rx(),
                                       center.ry(),
                                       center.rx() + radius * lengthOfMinuteHand * sin(M_PI * (minuteRotationDegrees/180.0)),
                                       center.ry() - radius * lengthOfMinuteHand * cos(M_PI * (minuteRotationDegrees/180.0))
                                       );
    minuteHand->setPen(minutePen);
    scene.addItem(minuteHand);

#ifdef SHOWSECONDHAND
    secondHand = new QGraphicsLineItem(center.rx() - radius * lengthOfShortSecondHand * sin(M_PI * (secondRotationDegrees/180.0)),
                                       center.ry() + radius * lengthOfShortSecondHand * cos(M_PI * (secondRotationDegrees/180.0)),
                                       center.rx() + radius * lengthOfSecondHand * sin(M_PI * (secondRotationDegrees/180.0)),
                                       center.ry() - radius * lengthOfSecondHand * cos(M_PI * (secondRotationDegrees/180.0))
                                       );
    secondHand->setPen(secondPen);
    scene.addItem(secondHand);
#endif

    double dotSize = 3;
    secondDot = new QGraphicsEllipseItem(center.rx() - dotSize, center.ry() - dotSize, 2.0 * dotSize, 2.0 * dotSize);
    secondDot->setPen(secondPen);
    secondDot->setBrush(QBrush(Qt::white));
    scene.addItem(secondDot);

    // UPDATE TO INITIAL TIME ------
    updateClock();

    // ---------------------
    view.setDisabled(true);

    view.setScene(&scene);
    view.setParent(this, Qt::FramelessWindowHint);

    connect(&this->secondTimer, &QTimer::timeout,
            this, [this](){
//                        qDebug() << "TIMER!";
                        updateClock();
                      } );
#ifdef SHOWSECONDHAND
#ifdef SMOOTHROTATION
    secondTimer.start(100); // every 100ms
#else
    secondTimer.start(1000); // every second
#endif
#else
    secondTimer.start(60.0 * 1000); // every 60 seconds
#endif
}

void svgClock::updateClock() {
    // HANDS -----------
    QTime theTime = QTime::currentTime();

#ifdef SMOOTHROTATION
    // seconds smoothly go round -----
    double hourRotationDegrees = 30.0 * (theTime.hour() + (theTime.minute()/60.0) + (theTime.second()/3600.0));
    double minuteRotationDegrees = 360 * ((theTime.minute()/60.0) + (theTime.second()/3600.0));
#ifdef SHOWSECONDHAND
    double secondRotationDegrees = 360 * ((theTime.second() + theTime.msec()/1000.0)/60.0);
#endif
#else
    // seconds tick one at a time -----
    double hourRotationDegrees = 30.0 * (theTime.hour() + (theTime.minute()/60.0));
    double minuteRotationDegrees = 360 * ((theTime.minute()/60.0));
#ifdef SHOWSECONDHAND
    double secondRotationDegrees = 360 * ((theTime.second())/60.0);
#endif
#endif

    hourHand->setLine(center.rx(),
                                     center.ry(),
                                     center.rx() + radius * lengthOfHourHand * sin(M_PI * (hourRotationDegrees/180.0)),
                                     center.ry() - radius * lengthOfHourHand * cos(M_PI * (hourRotationDegrees/180.0))
                                     );

    minuteHand->setLine(center.rx(),
                                       center.ry(),
                                       center.rx() + radius * lengthOfMinuteHand * sin(M_PI * (minuteRotationDegrees/180.0)),
                                       center.ry() - radius * lengthOfMinuteHand * cos(M_PI * (minuteRotationDegrees/180.0))
                                       );

#ifdef SHOWSECONDHAND
    secondHand->setLine(center.rx() - radius * lengthOfShortSecondHand * sin(M_PI * (secondRotationDegrees/180.0)),
                                       center.ry() + radius * lengthOfShortSecondHand * cos(M_PI * (secondRotationDegrees/180.0)),
                                       center.rx() + radius * lengthOfSecondHand * sin(M_PI * (secondRotationDegrees/180.0)),
                                       center.ry() - radius * lengthOfSecondHand * cos(M_PI * (secondRotationDegrees/180.0))
                                       );
#endif

    // DIGITAL TIME ------
    QLocale myLocale;
    QString theLocaleTimeFormat = myLocale.timeFormat(QLocale::NarrowFormat);
    QString digitalTimeString = theTime.toString(theLocaleTimeFormat).replace(" AM", "").replace(" PM", ""); // Europe uses 24 hour time, US = 12 hr time (no AM/PM)
    digitalTime->setText(digitalTimeString);

    // now center it
    QRectF digitalTimeRect = digitalTime->boundingRect();
    digitalTime->setPos(center + QPoint(0, 25) - digitalTimeRect.center());
}

svgClock::~svgClock()
{
}

// ------------------------------------
void svgClock::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED(pe)
}

void svgClock::resizeEvent(QResizeEvent *re)
{
    QLabel::resizeEvent(re);
}
