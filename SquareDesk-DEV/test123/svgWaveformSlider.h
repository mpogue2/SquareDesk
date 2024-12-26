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

#ifndef SVGWAVEFORMSLIDER_H
#define SVGWAVEFORMSLIDER_H

#include <QWidget>
#include <QSlider>

#include <QSvgRenderer>
#include <QGraphicsEllipseItem>
#include <QGraphicsSvgItem>

#include <QGraphicsView>
#include <QGraphicsScene>

#include <QMouseEvent>
#include <QPoint>

#include "globaldefines.h"

// This is a HORIZONTAL slider that shows the music waveform.
//  It has start and end loop indicators, a current position indicator,
//  and (for future expansion) settable markers.

//#define WAVEFORMWIDTH (491 - 10)
//#define WAVEFORMWIDTH 481

// NOTE: This is because of a bug that limits us to 784-pixel svgWaveformSlider backgrounds.  Much of the time it will be 1:1 mapping to pixels.
#define WAVEFORMSAMPLES 784

// -----------------------------------
class svgWaveformSlider : public QSlider
{
    Q_OBJECT
//    Q_PROPERTY(QString bgFile     READ getBgFile     WRITE setBgFile     NOTIFY bgFileChanged)
    QP_V(QColor, bgColor);
    QP_V(QColor, darkeningColor);
    QP_V(QColor, loopColor);
    QP_V(QColor, currentPositionColor);

public:
    explicit svgWaveformSlider(QWidget *parent = 0);
    ~svgWaveformSlider();

//    void setBgFile(QString s);
//    QString getBgFile() const;

    void setSingingCall(bool b);
    void setOrigin(int i);

    // LOOPS -------
    void setLoop(bool b);
    void setIntro(double frac);
    void setOutro(double frac);
    double getIntro();
    double getOutro();

    void setValue(int value);
    void setFloatValue(float f);

    void setAbsolutePathToSegmentFile(QString s);

    // NORMALIZATION ----
    void setWholeTrackPeak(double p);

    void finishInit();

    void updateBgPixmap(float *f, size_t t);

    void SetDefaultIntroOutroPositions(bool tempoIsBPM, double estimatedBPM,
                                       bool songIsSingerType,
                                       double songStart_sec, double songEnd_sec, double songLength_sec);

    bool eventFilter(QObject *obj, QEvent *event);

    // MARKERS ----------
    //    void AddMarker(double markerLoc);
    //    void DeleteMarker(double markerLoc);
    //    QSet<double> GetMarkers();
    //    void ClearMarkers();

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    double valueFromMouseEvent(QMouseEvent* e);

signals:
    void bgFileChanged(QString s);

private:
    QString absolutePathToSegmentFile;

    void paintEvent(QPaintEvent *pe);
    void resizeEvent(QResizeEvent *re);

    QGraphicsView view;
    QGraphicsScene scene;

    QPixmap *bgPixmap;
    QGraphicsPixmapItem *bg;

    QGraphicsTextItem *loadingMessage;

    QGraphicsItemGroup *currentPos;

    QGraphicsItemGroup *leftLoopMarker;
    QGraphicsItemGroup *rightLoopMarker;

    QGraphicsRectItem *leftLoopCover;
    QGraphicsRectItem *rightLoopCover;

    QSize bSize,hSize;
    double top, bot, length;

    bool singingCall, previousSingingCall;  // previousSingingCall is what it was last time (cached)

    // LOOPS ----
    bool drawLoopPoints;
    double introPosition;
    double outroPosition;

    double introFrac;
    double outroFrac;

    double wholeTrackPeak;

    int origin;  // reset to this point when double-clicked

    bool nowDestroying;

//    int myWidth, myHeight;

    float *cachedWaveform;  // cached pointer to the waveform

    // MARKERS ----
//    bool drawMarkers;
//    QSet<double> markers;

//    QString singer1, else1, s2;  // stylesheets for coloring sliders

};

#endif // SVGWAVEFORMSLIDER_H
