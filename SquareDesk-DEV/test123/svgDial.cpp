/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usage
** For commercial licensing terms and conditions, contact the authors via the
** email address above.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appear in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.
**
** $SQUAREDESK_END_LICENSE$
**
****************************************************************************/

#include <QCoreApplication>
#include "svgDial.h"
#include "math.h"

#ifdef DEBUG_LIGHT_MODE
#define ARCWIDTH 3.5
#else
#define ARCWIDTH 1.5
#endif

// -------------------------------------
// from: https://stackoverflow.com/questions/7537632/custom-look-svg-gui-widgets-in-qt-very-bad-performance
svgDial::svgDial(QWidget *parent) :
    QDial(parent)
{
//    qDebug() << "======== svgDial default constructor!";
    knob = nullptr;
    needle = nullptr;
    arc = nullptr;
}

svgDial::~svgDial()
{
}

// ------------------------------------
void svgDial::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED(pe)
}

void svgDial::resizeEvent(QResizeEvent *re)
{
    Q_UNUSED(re)
//    QDial::resizeEvent(re);
}

double svgDial::valueFromMouseEvent(QMouseEvent* e) {
    QPoint cur = e->globalPosition().toPoint();
    QPoint diff = cur - m_prevPos;
    m_prevPos = cur;

    double dist = sqrt(static_cast<double>(diff.x() * diff.x() + diff.y() * diff.y()));
    bool y_dominant = abs(diff.y()) > abs(diff.x());

    // if y is dominant, then treat an increase in dy as negative (y is
    // pointed downward). Otherwise, if y is not dominant and x has
    // decreased, then treat it as negative.
    if ((y_dominant && diff.y() > 0) || (!y_dominant && diff.x() < 0)) {
        dist = -dist;
    }

    double value = this->value() + dist;

    // Clamp to [0.0, 100.0]
    value = std::clamp(value, 0.0, 100.0);

//    if (dist != 0) {
//        qDebug() << "valueFromMouseEvent: " << cur << diff << dist << y_dominant << this->value() << value;
//    }

    return value;
}

void svgDial::mouseMoveEvent(QMouseEvent* e) {
    double value = valueFromMouseEvent(e);
    this->setValue(value);

    // update pointer
    needle->setRotation((this->sliderPosition() - middle)*degPerPos);

    // update arc
    double endAngle = -degPerPos * (this->sliderPosition() - middle);
    arc->setSpanAngle(16.0 * endAngle);
}

void svgDial::mousePressEvent(QMouseEvent* e) {
    Q_UNUSED(e)

    switch (e->button()) {
        case Qt::LeftButton:
            m_startPos = e->globalPosition().toPoint();
            m_prevPos = m_startPos;
            break;
        case Qt::MiddleButton:
        case Qt::RightButton:
            break;
        default:
            break;
    }
}

void svgDial::mouseDoubleClickEvent(QMouseEvent* e) {
    Q_UNUSED(e)

    int value = (this->maximum() + this->minimum())/2;
    this->setValue(value);

    // update pointer
    needle->setRotation((this->sliderPosition() - middle)*degPerPos);

    // update arc
    double endAngle = -degPerPos * (this->sliderPosition() - middle);
    arc->setSpanAngle(16.0 * endAngle);
}

void svgDial::mouseReleaseEvent(QMouseEvent* e) {
    double value = 0;
    double endAngle = 0;

    switch (e->button()) {
        case Qt::LeftButton:
            value = valueFromMouseEvent(e);
            this->setValue(value);

            // update pointer
            needle->setRotation((this->sliderPosition() - middle)*degPerPos);

            // update arc
            endAngle = -degPerPos * (this->value() - middle);
            arc->setSpanAngle(16.0 * endAngle);
            break;
        case Qt::MiddleButton:
        case Qt::RightButton:
            break;
        default:
            break;
    }
}

// -----------------------------
void svgDial::reinit() {
    // qDebug() << "======== reinit with new files and colors:" << m_knobFile << m_needleFile << m_arcColor;
    QString pathToResources = QCoreApplication::applicationDirPath() + "/";

#if defined(Q_OS_MAC)
    pathToResources = pathToResources + "../Resources/";
#endif

    if (m_knobFile == "" || m_needleFile == "") {
        return;
    }

    knob   = new QGraphicsSvgItem(pathToResources + m_knobFile);
    needle = new QGraphicsSvgItem(pathToResources + m_needleFile);

    //view.setStyleSheet("background: transparent; border: none");

    k = knob->renderer()->defaultSize();
    n = needle->renderer()->defaultSize();

    // qDebug() << "k/n" << k << n;

    needle->setTransformOriginPoint(n.width()/2, n.height()/2 + 2.0);
    knob->setTransformOriginPoint(k.width()/2, k.height()/2);

    if (k != n) {
        needle->setPos((k.width()-n.width())/2, (k.height()-n.height())/2);
    }

    // this->setMinimum(0);
    // this->setMaximum(100);

    degPerPos = 270.0/(double)(this->maximum() - this->minimum());
    middle = (this->maximum() - this->minimum())/2;

    int position = value();  // ***** current position
    needle->setRotation((position - middle) * degPerPos);

    mysize = k.width();
    if (mysize < n.width())  mysize = n.width();
    if (mysize < k.height()) mysize = k.height();
    if (mysize < n.height()) mysize = n.height();

    // for the ARC ------
    offset = 5.5;
    xoffset =  0.25;
    yoffset = -0.5;
    radius = mysize;

    //arcColor.setNamedColor(m_arcColor); // convert string to QColor
    arcColor = QColor(m_arcColor);

    // ARC ------------------
    arc = new QGraphicsArcItem(offset+xoffset,offset+yoffset, radius-2*offset+yoffset+xoffset, radius-2*offset+yoffset);
    arc->setPen(QPen(arcColor, ARCWIDTH, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));  // NOTE: arcColor is QColor
    arc->setStartAngle(16.0 * 90.0);

    // draw the arc -----
    double endAngle = 0.0; // TEST TEST
    arc->setSpanAngle(16.0 * endAngle);

    this->setValue(position);

    // ---------------------
    // view.setDisabled(true);
    // view.setDisabled(false);
    // view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    scene.clear(); // added

    // qDebug() << "NEW knob/needle/arc = " << knob << needle << arc;

    scene.addItem(knob);
    scene.addItem(arc);
    scene.addItem(needle);

    // view.setScene(&scene);
    // view.setParent(this,Qt::FramelessWindowHint);
}

// -----------------------------
void svgDial::finishInit() {
    // qDebug() << "======== finishing up svgDial init!" << m_knobFile << m_needleFile;

    QString pathToResources = QCoreApplication::applicationDirPath() + "/";

#if defined(Q_OS_MAC)
    pathToResources = pathToResources + "../Resources/";
#endif

    knob   = new QGraphicsSvgItem(pathToResources + m_knobFile);
    needle = new QGraphicsSvgItem(pathToResources + m_needleFile);

    view.setStyleSheet("background: transparent; border: none");

    k = knob->renderer()->defaultSize();
    n = needle->renderer()->defaultSize();

    // qDebug() << "k/n" << k << n;

    needle->setTransformOriginPoint(n.width()/2, n.height()/2 + 2.0);
    knob->setTransformOriginPoint(k.width()/2, k.height()/2);

    if (k != n) {
            needle->setPos((k.width()-n.width())/2, (k.height()-n.height())/2);
    }

    this->setMinimum(0);
    this->setMaximum(100);

    degPerPos = 270.0/(double)(this->maximum() - this->minimum());
    middle = (this->maximum() - this->minimum())/2;

    int position = 50;  // initial position
    needle->setRotation((position - middle) * degPerPos);

    mysize = k.width();
    if (mysize < n.width())  mysize = n.width();
    if (mysize < k.height()) mysize = k.height();
    if (mysize < n.height()) mysize = n.height();

    // for the ARC ------
    offset = 5.5;
    xoffset =  0.25;
    yoffset = -0.5;
    radius = mysize;

    // arcColor.setNamedColor(m_arcColor); // convert string to QColor
    arcColor = QColor(m_arcColor);

    // ARC ------------------
    arc = new QGraphicsArcItem(offset+xoffset,offset+yoffset, radius-2*offset+yoffset+xoffset, radius-2*offset+yoffset);
    arc->setPen(QPen(arcColor, ARCWIDTH, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));  // NOTE: arcColor is QColor
    arc->setStartAngle(16.0 * 90.0);

    // draw the arc -----
    double endAngle = 0.0; // TEST TEST
    arc->setSpanAngle(16.0 * endAngle);

    this->setValue(position);

    // ---------------------
    view.setDisabled(true);
    // view.setDisabled(false);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    scene.clear(); // added

    // qDebug() << "knob/needle/arc = " << knob << needle << arc << arcColor;

    scene.addItem(knob);
    scene.addItem(arc);
    scene.addItem(needle);

    view.setScene(&scene);
    view.setParent(this,Qt::FramelessWindowHint);
}


void svgDial::setValue(int value) {

    QDial::setValue(value);  // set the value of the parent class

    // update pointer
    needle->setRotation((value - middle)*degPerPos);

    // update arc
    double endAngle = -degPerPos * (value - middle);
    arc->setSpanAngle(16.0 * endAngle);
};

void svgDial::setKnobFile(QString s) {
    // qDebug() << "setKnobFile" << s;
    m_knobFile = s;
    emit knobFileChanged(s);
}

QString svgDial::getKnobFile() const {
    return(m_knobFile);
}

void svgDial::setNeedleFile(QString s) {
    // qDebug() << "setNeedleFile" << s;
    m_needleFile = s;
    emit needleFileChanged(s);
}

QString svgDial::getNeedleFile() const {
    return(m_needleFile);
}

void svgDial::setArcColor(QString s) {
    m_arcColor = s;
    // qDebug() << "setArcColor" << s;
    emit arcColorChanged(s);

    // finishInit(); // after knobFile, needleFile, and arcColor are set, finish up the init stuff, before knob is visible for the first time
}

QString svgDial::getArcColor() const {
    return(m_arcColor);
}
