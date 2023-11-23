#include <QTime>
#include <QElapsedTimer>

#include "svgVUmeter.h"
#include "math.h"

// -------------------------------------
// from: https://stackoverflow.com/questions/7537632/custom-look-svg-gui-widgets-in-qt-very-bad-performance
svgVUmeter::svgVUmeter(QWidget *parent) :
    QLabel(parent)
{
    view.setStyleSheet("background: transparent; border: none");

    // ANTI-ALIASING NOT NEEDED (everything is on pixel boundaries
//    view.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    setGeometry(0,0,100,200);

    int vuMeterX = 21;
    int vuMeterY = 111;
    QRect vuMeterSize(0, 0, vuMeterX, vuMeterY);

    view.setFixedSize(vuMeterX, vuMeterY);
    view.setSceneRect(0, 0, vuMeterX, vuMeterY);

    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QBrush bgBrush(QColor("#212121"));
    view.setBackgroundBrush(bgBrush);

    center = vuMeterSize.center();

//    qDebug() << "geometry: " << vuMeterSize << center;

    // BARS -----------
    QPen greenPen(QColor(Qt::green), 8, Qt::DashLine, Qt::FlatCap);
    QPen yellowPen(QColor(Qt::yellow), 8, Qt::DashLine, Qt::FlatCap);
    QPen redPen(QColor(Qt::red), 8, Qt::DashLine, Qt::FlatCap);
    QPen grayPen(QColor("#282828"), 8, Qt::DashLine, Qt::FlatCap);
    QPen darkRedPen(QColor("#48020D"), 8, Qt::DashLine, Qt::FlatCap);

    QVector<qreal> dashes;
    dashes << 0.25 << 0.25;
    greenPen.setDashPattern(dashes);
    yellowPen.setDashPattern(dashes);
    redPen.setDashPattern(dashes);
    grayPen.setDashPattern(dashes);
    darkRedPen.setDashPattern(dashes);

    // gray (always draw)
    grayBarL = new QGraphicsLineItem(0.3*vuMeterX,vuMeterY-7,0.3*vuMeterX,vuMeterY*0.19);
    grayBarL->setPen(grayPen);
    scene.addItem(grayBarL);

    grayBarR = new QGraphicsLineItem(0.7*vuMeterX,vuMeterY-7,0.7*vuMeterX,vuMeterY*0.19);
    grayBarR->setPen(grayPen);
    scene.addItem(grayBarR);

    // darkRed (always draw)
    darkRedBarL = new QGraphicsLineItem(0.3*vuMeterX,vuMeterY*0.18,0.3*vuMeterX,vuMeterY*0.08);
    darkRedBarL->setPen(darkRedPen);
    scene.addItem(darkRedBarL);

    darkRedBarR = new QGraphicsLineItem(0.7*vuMeterX,vuMeterY*0.18,0.7*vuMeterX,vuMeterY*0.08);
    darkRedBarR->setPen(darkRedPen);
    scene.addItem(darkRedBarR);

    // ----------
    // green
    greenBarL = new QGraphicsLineItem(0.3*vuMeterX,vuMeterY-7,0.3*vuMeterX,32);
    greenBarL->setPen(greenPen);
    scene.addItem(greenBarL);

    greenBarR = new QGraphicsLineItem(0.7*vuMeterX,vuMeterY-7,0.7*vuMeterX,32);
    greenBarR->setPen(greenPen);
    scene.addItem(greenBarR);

    // yellow
    yellowBarL = new QGraphicsLineItem(0.3*vuMeterX,vuMeterY*0.288,0.3*vuMeterX,vuMeterY*0.19);
    yellowBarL->setPen(yellowPen);
    scene.addItem(yellowBarL);

    yellowBarR = new QGraphicsLineItem(0.7*vuMeterX,vuMeterY*0.288,0.7*vuMeterX,vuMeterY*0.19);
    yellowBarR->setPen(yellowPen);
    scene.addItem(yellowBarR);

    // red
    redBarL = new QGraphicsLineItem(0.3*vuMeterX,vuMeterY*0.18,0.3*vuMeterX,vuMeterY*0.08);
    redBarL->setPen(redPen);
    scene.addItem(redBarL);

    redBarR = new QGraphicsLineItem(0.7*vuMeterX,vuMeterY*0.18,0.7*vuMeterX,vuMeterY*0.08);
    redBarR->setPen(redPen);
    scene.addItem(redBarR);

    // UPDATE TO INITIAL TIME ------
    updateMeter();

    // ---------------------
    view.setDisabled(true);

    view.setScene(&scene);
    view.setParent(this, Qt::FramelessWindowHint);

    valueL = valueR = 0.0;

    connect(&this->VUmeterTimer, &QTimer::timeout,
            this, [this](){
//                        qDebug() << "VUMETER!";
                        updateMeter();
                      } );

    VUmeterTimer.start(100); // looks sluggish @ 200ms, pretty good @ 100ms, no real improvement @ 50ms, so 100ms it is

    oldLimitL = oldLimitR = -10.0;  // force update on first encounter

//    // measure time -----------
//    timer.start();
//    QPixmap px(200,200);
//    QPainter painter(&px);
//    for (int i = 0; i < 100; i++) {
//        scene.render(&painter);
//    }
//    qDebug() << "TIME TO RENDER 100 times: " << timer.elapsed() << "ms";
//    //    // -----------------------
}

void svgVUmeter::updateMeter() {

    if (valueL < 0 || valueL > 1.0 || valueR < 0 || valueR > 1.0) {
        return;  // ignore bad values
    }

    // if small, just make it zero
    if (valueL < 0.001) {
        valueL = 0.00000001;
    }
    if (valueR < 0.001) {
        valueR = 0.00000001;
    }

    double vuMeterX = 21;
    double vuMeterY = 111;
    double bottom = vuMeterY-7;
    double top = vuMeterY*0.08;

//    if (valueL != 0 || valueR != 0) {
//        qDebug() << "updateMeter:" << valueL << valueR;
//    }

    // valueL/R are 0-1.0 here

    // mapping to log domain ---
    double levelL = 20.0 * log10f(valueL);
    double levelR = 20.0 * log10f(valueR);

    double boxesL = 10.0 + 2.0 * levelL/6.0 + 0.5;
    double boxesR = 10.0 + 2.0 * levelR/6.0 + 0.5;

    if (boxesL < 4.0) {
        boxesL = (boxesL + 12.0)/4.0; // map -12 to 4 --> 0 to 4
    }
    if (boxesR < 4.0) {
        boxesR = (boxesR + 12.0)/4.0; // map -12 to 4 --> 0 to 4
    }

    valueL = boxesL / 10.0; // remap to 0.0 to 1.0 for the darkVUmeter
    valueR = boxesR / 10.0; // remap to 0.0 to 1.0 for the darkVUmeter
    // -------------------------

    double limitL = bottom - valueL * (bottom - top);
    double limitR = bottom - valueR * (bottom - top);

    if (fabs(limitL - oldLimitL) > 1.0) {
        greenBarL->setLine(0.3*vuMeterX,vuMeterY-7, 0.3*vuMeterX,fmin(limitL,vuMeterY-7));
        yellowBarL->setLine(0.3*vuMeterX,vuMeterY*0.288, 0.3*vuMeterX,fmin(fmax(limitL, vuMeterY*0.19),vuMeterY*0.288));
        redBarL->setLine(0.3*vuMeterX,vuMeterY*0.18, 0.3*vuMeterX,fmin(fmax(limitL,vuMeterY*0.08),vuMeterY*0.18));
        oldLimitL = limitL;
    }

    if (fabs(limitR - oldLimitR) > 1.0) {
        greenBarR->setLine(0.7*vuMeterX,vuMeterY-7,0.7*vuMeterX,fmin(limitR,vuMeterY-7));
        yellowBarR->setLine(0.7*vuMeterX,vuMeterY*0.288,0.7*vuMeterX,fmin(fmax(limitR, vuMeterY*0.19),vuMeterY*0.288));
        redBarR->setLine(0.7*vuMeterX,vuMeterY*0.18,0.7*vuMeterX,fmin(fmax(limitR,vuMeterY*0.08),vuMeterY*0.18));
        oldLimitR = limitR;
    }

}

svgVUmeter::~svgVUmeter()
{
}

// ------------------------------------
void svgVUmeter::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED(pe)
}

void svgVUmeter::resizeEvent(QResizeEvent *re)
{
    Q_UNUSED(re)
//    QLabel::resizeEvent(re);
}

void svgVUmeter::levelChanged(double peakL_mono, double peakR, bool isMono) {
    Q_UNUSED(isMono)  // TODO: IMPLEMENT THIS (2 bars --> 1 bar)

//    if (peakL_mono != 0.0 || peakR != 0.0) {
//        qDebug() << "levelChanged:" << peakL_mono << peakR;
//    }
    valueL = peakL_mono;
    valueR = peakR;
}
