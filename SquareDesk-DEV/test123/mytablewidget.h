#ifndef MYTABLEWIDGET_H
#define MYTABLEWIDGET_H

#include <QTableWidget>

class MyTableWidget : public QTableWidget
{
public:
    MyTableWidget(QWidget *parent);
    bool isEditing();
};

#endif // MYTABLEWIDGET_H
