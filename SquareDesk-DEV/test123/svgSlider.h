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

    void setBgFile(QString s) {
        m_bgFile = s;
        emit bgFileChanged(s);
    }

    QString getBgFile() const {
        return(m_bgFile);
    }

    void setHandleFile(QString s) {
        m_handleFile = s;
        emit handleFileChanged(s);
    }

    QString getHandleFile() const {
        return(m_handleFile);
    }

    void setVeinColor(QString s) {
        m_veinColor = s;
        emit veinColorChanged(s);
    }

    QString getVeinColor() const {
        return(m_veinColor);
    }

    void setDefaultValue(double d) {
        m_defaultValue = d;
        emit defaultValueChanged(d);
    }

    double getDefaultValue() const {
        return(m_defaultValue);
    }

    void setIncrement(double d) {
        m_increment = d;
        emit incrementChanged(d);
    }

    double getIncrement() const {
        return(m_increment);
    }

    void setCenterVeinType(bool s) {
        m_centerVeinType = s;
        emit centerVeinTypeChanged(s);

        finishInit(); // after parameters are set, finish up the init stuff, before slider is visible for the first time
    }

    bool getCenterVeinType() const {
        return(m_centerVeinType);
    }

    void setValue(int value);

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

    void finishInit();
};

#endif // SVGSLIDER_H
