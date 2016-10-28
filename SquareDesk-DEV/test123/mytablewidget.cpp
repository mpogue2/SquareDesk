#include "mytablewidget.h"
#include <QDebug>

MyTableWidget::MyTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
//    qDebug() << "MyTableWidget::MyTableWidget()";
}

// returns true, if one of the items in the table is being edited (in which case,
//   we don't want keypresses to be hotkeys, we want them to go into the field being edited.
bool MyTableWidget::isEditing() {
//    qDebug() << "MyTableWidget::isEditing()";
    return (state() == QTableWidget::EditingState);
}
