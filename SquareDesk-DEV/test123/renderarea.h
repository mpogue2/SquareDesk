#ifndef RENDERAREA_H
#define RENDERAREA_H

#include <QBrush>
#include <QPen>
#include <QPixmap>
#include <QWidget>

#include <QString>
#include <QStringList>
#include "common.h"

class RenderArea : public QWidget
{
    Q_OBJECT

public:
    RenderArea(QWidget *parent = 0);

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;

public slots:
    void setPen(const QPen &pen);
    void setBrush(const QBrush &brush);
    void setLayout1(QString s);
    void setLayout2(QStringList sl);
    void setFormation(QString s);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    QPen pen;
    QBrush brush;
    QPixmap pixmap;
    QString layout1;
    QStringList layout2;
    unsigned long bad;
    QString formation;
};

#endif // RENDERAREA_H
