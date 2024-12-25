#include <QCoreApplication>
#include "svgSlider.h"
#include "math.h"

// -------------------------------------
// from: https://stackoverflow.com/questions/7537632/custom-look-svg-gui-widgets-in-qt-very-bad-performance
svgSlider::svgSlider(QWidget *parent) :
    QSlider(parent)
{
//    qDebug() << "======== svgSlider default constructor!";
    // these are all explicitly uninitialized until the call to finishInit()
    //   which is initiated by setCenterVeinType(), called LAST.
    bg = nullptr;
    handle = nullptr;
    vein = nullptr;
}

svgSlider::~svgSlider()
{
}

// ------------------------------------
void svgSlider::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED(pe)
//    qDebug() << "qSVGSlider::paintEvent";
}

void svgSlider::resizeEvent(QResizeEvent *re)
{
    QSlider::resizeEvent(re);
}

double svgSlider::valueFromMouseEvent(QMouseEvent* e) {
    double value100 = e->pos().y() + 3; // 3 - 110
    double sliderValue;

    if (value100 < 14) {
        sliderValue = 100;
    } else if (value100 > 104) {
        sliderValue = 0;
    } else {
        sliderValue = ((104-14) - (value100 - 14)) * (100.0/(104-14));
    }
    sliderValue = minimum() + (maximum() - minimum()) * sliderValue/100.0;
//    qDebug() << "valueFromMouseEvent: " << sliderValue << sliderValue;
    return sliderValue;  // a number between min and max of this slider
}

void svgSlider::mouseMoveEvent(QMouseEvent* e) {
    double value = valueFromMouseEvent(e);

//    this->setValue(m_increment * round(value/m_increment));  // calls the overridden setValue() below, which also updates handle
    this->setValue(value);  // calls the overridden setValue() below, which also updates handle

//    // update handle
//    double offset = bot - (this->value()/100.0) * (104-14) + 2.5;
//    handle->setPos(0, bSize.height()/2.0 + offset);  // initial position of the handle

//    double endVein = bSize.height()/2.0 + offset + 7;
//    vein->setLine(21, startVein, 21, endVein);
//    qDebug() << "mouseMoveEvent: new value is " << value;
}

void svgSlider::setValue(int value) {

    QSlider::setValue(value);  // set the value of the parent class

    if (bg == nullptr) {
        return; // slider is not fully initialized yet
    }

    double value100 = 100.0 * (value - minimum())/(maximum() - minimum());

    // and then update the handle
//    double offset = bot - (this->value()/100.0) * (104-14) + 2.5;
    double offset = bot - (value100/100.0) * (104-14) + 2.5;
    handle->setPos(0, bSize.height()/2.0 + offset);  // initial position of the handle

    double endVein = bSize.height()/2.0 + offset + 7;
    vein->setLine(21, startVein, 21, endVein);
};


void svgSlider::mousePressEvent(QMouseEvent* e) {
    double value, roundedValue;
    double offset;
    double endVein;
    double value100;  // 0.0 to 100.0

    switch (e->button()) {
    case Qt::LeftButton:
        value = valueFromMouseEvent(e);

//        this->setValue(m_increment * round(value/m_increment));
        roundedValue = m_increment * round(value/m_increment);
        this->setValue(roundedValue);

        // update handle
        value100 = 100.0 * (roundedValue - minimum())/(maximum() - minimum());
        offset = bot - (value100/100.0) * (104-14) + 2.5;
        handle->setPos(0, bSize.height()/2.0 + offset);  // initial position of the handle

        endVein = bSize.height()/2.0 + offset + 7;
        vein->setLine(21, startVein, 21, endVein);
//        qDebug() << "mousePressEvent: " << m_increment << value << roundedValue;
        break;
    case Qt::MiddleButton:
    case Qt::RightButton:
        break;
    default:
        break;
    }
}

void svgSlider::mouseDoubleClickEvent(QMouseEvent* e) {
//    qDebug() << "mouseDoubleClickEvent: " << e;
    Q_UNUSED(e)

//    this->setValue(m_increment * round(m_defaultValue/m_increment)); // double click --> (whatever the default value is)

    double value = this->getDefaultValue();
    double value100;
    setValue(value);

    // update handle
    value100 = 100.0 * (value - minimum())/(maximum() - minimum());
//    double offset = bot - (this->value()/100.0) * (104-14) + 2.5;
    double offset = bot - (value100/100.0) * (104-14) + 2.5;
    handle->setPos(0, bSize.height()/2.0 + offset);  // initial position of the handle

    double endVein = bSize.height()/2.0 + offset + 7;
    vein->setLine(21, startVein, 21, endVein);
}

void svgSlider::mouseReleaseEvent(QMouseEvent* e) {
    Q_UNUSED(e)
}

// this is called to update based on Light/Dark Mode change
// do NOT kill the scene or View, they are fine the way they were made at the start
void svgSlider::reinit() {
    // return;  // DEBUG DEBUG DEBUG

    if (m_bgFile == "" || m_handleFile == "" || m_veinColor == "") {
        return;
    }

    // qDebug() << "svgSlider::reinit() ===========" << m_bgFile << m_handleFile;
    QString pathToResources = QCoreApplication::applicationDirPath() + "/";

#if defined(Q_OS_MAC)
    pathToResources = pathToResources + "../Resources/";
#endif

    // make the graphical items
    bg     = new QGraphicsSvgItem(pathToResources + m_bgFile);
    handle = new QGraphicsSvgItem(pathToResources + m_handleFile);
    vein   = new QGraphicsLineItem(21, 100, 21, 40);

    // veinColor.setNamedColor(m_veinColor); // convert string to QColor
    veinColor = QColor(m_veinColor);
    vein->setPen(QPen(veinColor, 2, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));

    view.setStyleSheet("background: transparent; border: none;");

    bSize = bg->renderer()->defaultSize();
    hSize = handle->renderer()->defaultSize();

    //    qDebug() << bSize << hSize;

    // bg->setTransformOriginPoint(bSize.width()/2, bSize.height()/2);
    // handle->setTransformOriginPoint(hSize.width()/2, hSize.height()/2);

    if (bSize != hSize) {
        //        needle->setPos((k.width()-n.width())/2, (k.height()-n.height())/2);
    }

    //    this->setMinimum(0);
    //    this->setMaximum(100);
    //    this->setValue(m_increment * round(m_defaultValue/m_increment));    // initial position

    //    qDebug() << "finishInit:" << getDefaultValue();

    // setValue(getDefaultValue());    // initial position and double-click position to return to

    qDebug() << "current slider value is" << value();
    setValue(value());  // force recalculation of the vein

    setFixedSize(42,107); // no need for this

    // -56 is at top = position 100
    // -12 is middle
    //  32 is at bottom = position 0

    top = -56;
    bot = 32;
    length = bot - top;

    // double value100 = 100.0 * (getDefaultValue() - minimum())/(maximum() - minimum());
    double value100 = 100.0 * (value() - minimum())/(maximum() - minimum());

    //    double offset = bot - (this->value()/100.0) * length;
    double offset = bot - (value100/100.0) * length;
    handle->setPos(0, bSize.height()/2.0 + offset);  // initial position of the handle

    double endVein = bSize.height()/2.0 + offset + 7;

    if (m_centerVeinType) {
        startVein = 54;
    } else {
        startVein = 100;
    }
    vein->setLine(21, startVein, 21, endVein);

    // ---------------------
    // view.setDisabled(true);
    // view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    scene.clear();

    scene.addItem(bg);
    scene.addItem(vein);
    scene.addItem(handle);

    // view.setScene(&scene);
    // view.setParent(this);
}

void svgSlider::finishInit() {
    // qDebug() << "svgSlider::finishInit() ===========" << m_bgFile << m_handleFile;

    QString pathToResources = QCoreApplication::applicationDirPath() + "/";

#if defined(Q_OS_MAC)
    pathToResources = pathToResources + "../Resources/";
#endif

    if (m_bgFile == "" || m_handleFile == "" || m_veinColor == "") {
        return;
    }

    // make the graphical items
    bg     = new QGraphicsSvgItem(pathToResources + m_bgFile);
    handle = new QGraphicsSvgItem(pathToResources + m_handleFile);
    vein   = new QGraphicsLineItem(21, 100, 21, 40);

    // veinColor.setNamedColor(m_veinColor); // convert string to QColor
    veinColor = QColor(m_veinColor);
    vein->setPen(QPen(veinColor, 2, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));

    view.setStyleSheet("background: transparent; border: none;");

    bSize = bg->renderer()->defaultSize();
    hSize = handle->renderer()->defaultSize();

    //    qDebug() << bSize << hSize;

    //    bg->setTransformOriginPoint(bSize.width()/2, bSize.height()/2);
    //    handle->setTransformOriginPoint(hSize.width()/2, hSize.height()/2);

    if (bSize != hSize) {
        //        needle->setPos((k.width()-n.width())/2, (k.height()-n.height())/2);
    }

//    this->setMinimum(0);
//    this->setMaximum(100);
//    this->setValue(m_increment * round(m_defaultValue/m_increment));    // initial position

//    qDebug() << "finishInit:" << getDefaultValue();

    setValue(getDefaultValue());    // initial position and double-click position to return to

    setFixedSize(42,107);

    // -56 is at top = position 100
    // -12 is middle
    //  32 is at bottom = position 0

    top = -56;
    bot = 32;
    length = bot - top;

    double value100 = 100.0 * (getDefaultValue() - minimum())/(maximum() - minimum());

//    double offset = bot - (this->value()/100.0) * length;
    double offset = bot - (value100/100.0) * length;
    handle->setPos(0, bSize.height()/2.0 + offset);  // initial position of the handle

    double endVein = bSize.height()/2.0 + offset + 7;

    if (m_centerVeinType) {
        startVein = 54;
    } else {
        startVein = 100;
    }
    vein->setLine(21, startVein, 21, endVein);

    // ---------------------
    view.setDisabled(true);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    scene.addItem(bg);
    scene.addItem(vein);
    scene.addItem(handle);

    view.setScene(&scene);
    //    view.setParent(this, Qt::FramelessWindowHint);
    view.setParent(this);
}

void svgSlider::setBgFile(QString s) {
    m_bgFile = s;
    emit bgFileChanged(s);
}

QString svgSlider::getBgFile() const {
    return(m_bgFile);
}

void svgSlider::setHandleFile(QString s) {
    m_handleFile = s;
    emit handleFileChanged(s);
}

QString svgSlider::getHandleFile() const {
    return(m_handleFile);
}

void svgSlider::setVeinColor(QString s) {
    m_veinColor = s;
    emit veinColorChanged(s);
}

QString svgSlider::getVeinColor() const {
    return(m_veinColor);
}

void svgSlider::setDefaultValue(double d) {
    m_defaultValue = d;
    emit defaultValueChanged(d);
}

double svgSlider::getDefaultValue() const {
    return(m_defaultValue);
}

void svgSlider::setIncrement(double d) {
    m_increment = d;
    emit incrementChanged(d);
}

double svgSlider::getIncrement() const {
    return(m_increment);
}

void svgSlider::setCenterVeinType(bool s) {
    m_centerVeinType = s;
    emit centerVeinTypeChanged(s);

    // finishInit(); // after parameters are set, finish up the init stuff, before slider is visible for the first time
}

bool svgSlider::getCenterVeinType() const {
    return(m_centerVeinType);
}
