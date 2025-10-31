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

#ifndef SVGSLIDER_H
#define SVGSLIDER_H

#include <QWidget>
#include <QSlider>

#include <QSvgRenderer>
#include <QGraphicsEllipseItem>
#include <QGraphicsSvgItem>

#include <QGraphicsView>
#include <QGraphicsScene>

#include <QMouseEvent>
#include <QPoint>

// -----------------------------------
class svgSlider : public QSlider
{
    Q_OBJECT
    Q_PROPERTY(QString bgFile     READ getBgFile     WRITE setBgFile     NOTIFY bgFileChanged)
    Q_PROPERTY(QString handleFile READ getHandleFile WRITE setHandleFile NOTIFY handleFileChanged)
    Q_PROPERTY(QString veinColor  READ getVeinColor  WRITE setVeinColor  NOTIFY veinColorChanged)
    Q_PROPERTY(double  defaultValue    READ getDefaultValue    WRITE setDefaultValue    NOTIFY defaultValueChanged)
    Q_PROPERTY(double  increment       READ getIncrement       WRITE setIncrement       NOTIFY incrementChanged)
    Q_PROPERTY(bool    centerVeinType  READ getCenterVeinType  WRITE setCenterVeinType  NOTIFY centerVeinTypeChanged)

public:
    explicit svgSlider(QWidget *parent = 0);
    ~svgSlider();

    void setBgFile(QString s);
    QString getBgFile() const;

    void setHandleFile(QString s);
    QString getHandleFile() const;

    void setVeinColor(QString s);
    QString getVeinColor() const;

    void setDefaultValue(double d);
    double getDefaultValue() const;

    void setIncrement(double d);
    double getIncrement() const;

    void setCenterVeinType(bool s);
    bool getCenterVeinType() const;

    void setValue(int value);

    void reinit();
    void finishInit();

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    double valueFromMouseEvent(QMouseEvent* e);

signals:
    void bgFileChanged(QString s);
    void handleFileChanged(QString s);
    void veinColorChanged(QColor c);
    void defaultValueChanged(double d);
    void incrementChanged(double d);
    void centerVeinTypeChanged(bool b);
    void incrementRequested();
    void decrementRequested();

private:
    void paintEvent(QPaintEvent *pe);
    void resizeEvent(QResizeEvent *re);

    QString m_bgFile;
    QString m_handleFile;

    QString m_veinColor;  // string version
    QColor  veinColor;    // QColor version

    double m_defaultValue = 0.0;
    bool   m_centerVeinType; // false for volume, true for tempo
    double m_increment;  // snaps to these boundaries

    QGraphicsView view;
    QGraphicsScene scene;

    QGraphicsSvgItem *bg;
    QGraphicsSvgItem *handle;
    QGraphicsLineItem *vein;

    QSize bSize,hSize;

    double top, bot, length;
    double startVein;

    // Starting point when left mouse button is pressed
    QPoint m_startPos;
    QPoint m_prevPos;
};

#endif // SVGSLIDER_H
