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

    void setKnobFile(QString s) {
        m_knobFile = s;
        emit knobFileChanged(s);
    }

    QString getKnobFile() const {
        return(m_knobFile);
    }

    void setNeedleFile(QString s) {
        m_needleFile = s;
        emit needleFileChanged(s);
    }

    QString getNeedleFile() const {
        return(m_needleFile);
    }

    void setArcColor(QString s) {
        m_arcColor = s;
        emit arcColorChanged(s);

        finishInit(); // after knobFile, needleFile, and arcColor are set, finish up the init stuff, before knob is visible for the first time
    }

    QString getArcColor() const {
        return(m_arcColor);
    }

    void setValue(int value);

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

    void finishInit();
};

#endif // svgDial_H
