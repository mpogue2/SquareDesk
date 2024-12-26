/****************************************************************************
**
** Copyright (C) 2016-2024 Mike Pogue, Dan Lyke
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

#ifndef svgDial_H
#define svgDial_H

#include <QWidget>
#include <QDial>

#include <QSvgRenderer>
#include <QGraphicsEllipseItem>
#include <QGraphicsSvgItem>

#include <QGraphicsView>
#include <QGraphicsScene>

#include <QMouseEvent>
#include <QPoint>

#include "globaldefines.h"

// -------------------------------------
// from: https://stackoverflow.com/questions/14279162/qt-qgraphicsscene-drawing-arc
class QGraphicsArcItem : public QGraphicsEllipseItem {
public:
    QGraphicsArcItem ( qreal x, qreal y, qreal width, qreal height, QGraphicsItem * parent = 0 ) :
        QGraphicsEllipseItem(x, y, width, height, parent) {
    }

protected:
    void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
        Q_UNUSED(option)
        Q_UNUSED(widget)

        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(pen());
        painter->setBrush(brush());
        painter->drawArc(rect(), startAngle(), spanAngle());

        //        if (option->state & QStyle::State_Selected)
        //            qt_graphicsItem_highlightSelected(this, painter, option);
    }
};

// -----------------------------------
class svgDial : public QDial
{
    Q_OBJECT
    Q_PROPERTY(QString knobFile   READ getKnobFile   WRITE setKnobFile   NOTIFY knobFileChanged)
    Q_PROPERTY(QString needleFile READ getNeedleFile WRITE setNeedleFile NOTIFY needleFileChanged)
    Q_PROPERTY(QString arcColor   READ getArcColor   WRITE setArcColor   NOTIFY arcColorChanged)

public:
    explicit svgDial(QWidget *parent = 0);
    ~svgDial();

    void setKnobFile(QString s);
    QString getKnobFile() const;

    void setNeedleFile(QString s);
    QString getNeedleFile() const;

    void setArcColor(QString s);
    QString getArcColor() const;

    void setValue(int value);

    void finishInit(); // reinitialize all the QGraphicsItems from files
    void reinit();     // MINIMALLY reinitialize all the QGraphicsItems from files

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    double valueFromMouseEvent(QMouseEvent* e);

signals:
    void knobFileChanged(QString s);
    void needleFileChanged(QString s);
    void arcColorChanged(QColor c);

private:
    void paintEvent(QPaintEvent *pe);
    void resizeEvent(QResizeEvent *re);
    float degPerPos;
    float middle;
    float mysize;

    QGraphicsView view;
    QGraphicsScene scene;

    QGraphicsSvgItem *knob;
    QGraphicsSvgItem *needle;
    QGraphicsArcItem *arc;

    QString m_knobFile;
    QString m_needleFile;

    QString  m_arcColor;  // string version
    QColor arcColor;      // QColor version

    QSize k,n;

    // Starting point when left mouse button is pressed
    QPoint m_startPos;
    QPoint m_prevPos;

    double offset;
    double xoffset;
    double yoffset;
    double radius;
};

#endif // svgDial_H
