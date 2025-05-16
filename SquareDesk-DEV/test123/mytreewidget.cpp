#include <QMouseEvent>
#include <QTableView>
#include <QMenu>

#include "mytreewidget.h"

MyTreeWidget::MyTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
}

// #1390:
// if user RIGHT-clicks in the TreeWidget, show the context menu but do NOT
//   filter the songTable to that item.
// if user LEFT-clicks in the TreeWidget, filter songTable as normal.
void MyTreeWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        // Handle right-click (e.g., show context menu)
        QMenu contextMenu;
        // Add actions to the context menu
        contextMenu.exec(event->globalPosition().toPoint()); // Show the menu
    } else {
        // For other mouse buttons, call the base class implementation
        QTreeWidget::mousePressEvent(event);
    }
}
