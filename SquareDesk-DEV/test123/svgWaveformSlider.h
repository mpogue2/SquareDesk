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

// This is a HORIZONTAL slider that shows the music waveform.
//  It has start and end loop indicators, a current position indicator,
//  and (for future expansion) settable markers.

// -----------------------------------
class svgWaveformSlider : public QSlider
{
    Q_OBJECT
    Q_PROPERTY(QString bgFile     READ getBgFile     WRITE setBgFile     NOTIFY bgFileChanged)

//    void SetLoop(bool b);           // turn on loop points
//    void SetSingingCall(bool b);    // turn on singing call coloring
//    void SetOrigin(int newOrigin);  // use an origin other than zero, when double-clicked
//    void SetIntro(double intro);
//    void SetOutro(double outro);
//    double GetIntro() const;
//    double GetOutro() const;

    void SetDefaultIntroOutroPositions(bool tempoIsBPM, double estimatedBPM,
                                       double songStart_sec, double songEnd_sec, double songLength_sec);

    bool eventFilter(QObject *obj, QEvent *event);

// MARKERS ----------
//    void AddMarker(double markerLoc);
//    void DeleteMarker(double markerLoc);
//    QSet<double> GetMarkers();
//    void ClearMarkers();

public:
    explicit svgWaveformSlider(QWidget *parent = 0);
    ~svgWaveformSlider();

    void setBgFile(QString s) {
        m_bgFile = s;
//        emit bgFileChanged(s);
    }

    QString getBgFile() const {
        return(m_bgFile);
    }

    void setSingingCall(bool b) {
        singingCall = b;
    }

    void setOrigin(int i) {
        origin = i;
    }

    // LOOPS -------
    void setLoop(bool b) {
        drawLoopPoints = b;

        if (leftLoopMarker != nullptr && leftLoopMarker != nullptr) {
            leftLoopCover->setVisible(drawLoopPoints);
            leftLoopMarker->setVisible(drawLoopPoints);
            rightLoopCover->setVisible(drawLoopPoints);
            rightLoopMarker->setVisible(drawLoopPoints);
        }

    }

    void setIntro(double frac) {
        introPosition = frac * 491;
//        qDebug() << "*** setIntro:" << introPosition;
        if (leftLoopMarker != nullptr && leftLoopMarker != nullptr) {
            leftLoopMarker->setX(introPosition);
            leftLoopCover->setRect(0,0, introPosition,61);
        }
    }

    void setOutro(double frac) {
        outroPosition = frac * 491;
//        qDebug() << "*** setOutro:" << outroPosition;
        if (rightLoopMarker != nullptr && rightLoopMarker != nullptr) {
            rightLoopMarker->setX(outroPosition - 5); // 5 is to compensate for the width of the green bracket
            rightLoopCover->setRect(outroPosition,0, 491-outroPosition,61);
        }
    }

    double getIntro() {
        return(introPosition);
    }

    double getOutro() {
        return(outroPosition);
    }

    void setValue(int value);

    void finishInit();

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    double valueFromMouseEvent(QMouseEvent* e);

signals:
    void bgFileChanged(QString s);

private:
    void paintEvent(QPaintEvent *pe);
    void resizeEvent(QResizeEvent *re);

    QString m_bgFile;

    QGraphicsView view;
    QGraphicsScene scene;

//    QGraphicsSvgItem *bg;
    QImage  *bgImage;
    QPixmap *bgPixmap;
    QGraphicsPixmapItem *bg;

    QGraphicsLineItem *currentPos;

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

    int origin;  // reset to this point when double-clicked

    // MARKERS ----
//    bool drawMarkers;
//    QSet<double> markers;

//    QString singer1, else1, s2;  // stylesheets for coloring sliders

};

#endif // SVGWAVEFORMSLIDER_H
