#ifndef MYTREEWIDGET_H
#define MYTREEWIDGET_H

#include <QObject>
#include <QTreeWidget>

class MyTreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    MyTreeWidget(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // MYTREEWIDGET_H
