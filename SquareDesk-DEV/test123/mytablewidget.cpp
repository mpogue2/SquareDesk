/****************************************************************************
**
** Copyright (C) 2016-2023 Mike Pogue, Dan Lyke
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

#include "globaldefines.h"

#include "mytablewidget.h"
#include <QDebug>

MyTableWidget::MyTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
}

// returns true, if one of the items in the table is being edited (in which case,
//   we don't want keypresses to be hotkeys, we want them to go into the field being edited.
bool MyTableWidget::isEditing()
{
    return (state() == QTableWidget::EditingState);
}

// -----------------------------------------------
bool MyTableWidget::moveSelectedItemUp() {
#ifdef DARKMODE
    QModelIndexList list = selectionModel()->selectedRows();

    if (list.count() != 1) {
        // if it's zero or >= 2 return
        return false; // we did nothing, this was not meant for us
    }

    int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

    if (row == 0) {
        // we are already at the top
        return false; // we did what was requested, but no modifications FIX FIX FIX
    }

    // swap the numbers
    QTableWidgetItem *theItem      = item(row,0);    // # column
    QTableWidgetItem *theItemAbove = item(row - 1,0);

    QString t = theItem->text();
    theItem->setText(theItemAbove->text());
    theItemAbove->setText(t);

    sortItems(0);  // resort, based on column 0 (the #)

    scrollToItem(item(row - 1, 0)); // EnsureVisible for the moved-up row

    return true; // we did it!
#endif
}

// -----------------------------------------------
bool MyTableWidget::moveSelectedItemDown() {
#ifdef DARKMODE
    QModelIndexList list = selectionModel()->selectedRows();

    if (list.count() != 1) {
        // if it's zero or >= 2 return
        return false; // we did nothing, this was not meant for us
    }

    int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

    if (row == rowCount()-1) {
        // we are already at the bottom
        return false; // we did what was requested, but no changes actually made FIX FIX FIX
    }

    // swap the numbers
    QTableWidgetItem *theItem      = item(row,0);    // # column
    QTableWidgetItem *theItemBelow = item(row + 1,0);

    QString t = theItem->text();
    theItem->setText(theItemBelow->text());
    theItemBelow->setText(t);

    sortItems(0);  // resort, based on column 0 (the #)

    scrollToItem(item(row + 1, 0)); // EnsureVisible for the moved-down row

    return true; // we did it!
#endif
}

// -----------------------------------------------
bool MyTableWidget::moveSelectedItemToTop() {
// NOTE: If you can't move the selected item to the top by keyboard shortcut,
//  check to make sure that songTable's selected row is NOT row 0.  I'll fix this soon... FIX FIX FIX
#ifdef DARKMODE
    QModelIndexList list = selectionModel()->selectedRows();

    if (list.count() != 1) {
        // if it's zero or >= 2 return
        return false; // we did nothing, this was not meant for us
    }

    int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

    if (row == 0) {
        // we are already at the top
        return false; // we did what was requested
    }

    // Iterate over the entire songTable, incrementing items BELOW this item

    // what's the number of THIS item?
    QTableWidgetItem *theItem1 = item(row,0);
    QString playlistIndexText1 = theItem1->text();  // this is the playlist #
    int currentNumberInt = playlistIndexText1.toInt();

    // iterate through the entire table, and if number is less than THIS item's number, increment it
    for (int i=0; i<rowCount(); i++) {
        QTableWidgetItem *theItem = item(i,0);
        QString playlistIndexText = theItem->text();  // this is the playlist #
        int playlistIndexInt = playlistIndexText.toInt();

        if (playlistIndexInt < currentNumberInt) {
            // if a # was less, increment it
            QString newIndex = QString::number(playlistIndexInt+1);
            item(i,0)->setText(newIndex);
        }
    }

    theItem1->setText("1");  // this one becomes #1

    sortItems(0);  // resort, based on column 0 (the #)

    scrollToTop();

    return true;  // we did it!
#endif
}

// -----------------------------------------------
bool MyTableWidget::moveSelectedItemToBottom() {
#ifdef DARKMODE
    QModelIndexList list = selectionModel()->selectedRows();

    if (list.count() != 1) {
        // if it's zero or >= 2 return
        return false; // we did nothing, this was not meant for us
    }

    int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

    if (row == rowCount()-1) {
        // we are already at the bottom
        return false; // we did what was requested
    }

    // Iterate over the entire songTable, incrementing items BELOW this item

    // what's the number of THIS item?
    QTableWidgetItem *theItem1 = item(row,0);
    QString playlistIndexText1 = theItem1->text();  // this is the playlist #
    int currentNumberInt = playlistIndexText1.toInt();

    // iterate through the entire table, and if number is greater than THIS item's number, decrement it
    for (int i=0; i<rowCount(); i++) {
        QTableWidgetItem *theItem = item(i,0);
        QString playlistIndexText = theItem->text();  // this is the playlist #
        int playlistIndexInt = playlistIndexText.toInt();

        if (playlistIndexInt > currentNumberInt) {
            // if a # was less, decrement it
            QString newIndex = QString::number(playlistIndexInt - 1);
            item(i,0)->setText(newIndex);
        }
    }

    int theLastNumber = rowCount();
    theItem1->setText(QString::number(theLastNumber));  // this one becomes #<last>

    sortItems(0);  // resort, based on column 0 (the #)

    scrollToBottom();

    return true; // we did it!
#endif
}

// -----------------------------------------------
bool MyTableWidget::removeSelectedItem() {
#ifdef DARKMODE
    QModelIndexList list = selectionModel()->selectedRows();

    if (list.count() != 1) {
        // if it's zero or >= 2 return
        return false; // we did nothing, this was not meant for us
    }

    // OK, remember the row number, because we're going to restore it at the end...
    int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

    if (moveSelectedItemToBottom()) {

        // if this call was meant for us, then we can remove the row; otherwise, ignore
        removeRow(rowCount()-1);      // and remove the last row

        if (row > rowCount()-1) {
            // if we were removing the LAST row, we want to select the second-to-last row now
            //   any other row, and we just redo the selection to that row
            row = rowCount()-1;
        }

        selectRow(row);    // select it
        scrollToItem(item(row, 0)); // EnsureVisible for the row that was deleted, OR last row in table

        return true;
    }
    return false; // not meant for us
#endif
}

// -----------------------------------------------
void MyTableWidget::moveSelectionUp() {
    QModelIndexList list = selectionModel()->selectedRows();

    if (list.count() != 1) {
        // if it's zero or >= 2 return
        return; // we did nothing, this was not meant for us
    }

    int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

    if (row == 0) {
        // we are already at the top
        return; // we did what was requested
    }

    selectRow(row - 1);
}

// -----------------------------------------------
void MyTableWidget::moveSelectionDown() {
    QModelIndexList list = selectionModel()->selectedRows();

    if (list.count() != 1) {
        // if it's zero or >= 2 return
        return; // we did nothing, this was not meant for us
    }

    int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

    if (row == rowCount() - 1) {
        // we are already at the top
        return; // we did what was requested
    }

    selectRow(row + 1);
}

// -----------------------------------------------
void MyTableWidget::moveSelectionToTop() {
    QModelIndexList list = selectionModel()->selectedRows();

    if (list.count() != 1) {
        // if it's zero or >= 2 return
        return; // we did nothing, this was not meant for us
    }

    int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

    if (row == 0) {
        // we are already at the top
        return; // we did what was requested
    }

    selectRow(0);
}

// -----------------------------------------------
void MyTableWidget::moveSelectionToBottom() {
    QModelIndexList list = selectionModel()->selectedRows();

    if (list.count() != 1) {
        // if it's zero or >= 2 return
        return; // we did nothing, this was not meant for us
    }

    int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

    if (row == rowCount() - 1) {
        // we are already at the bottom
        return; // we did what was requested
    }

    selectRow(rowCount() - 1);
}

// -----------------------------------------------
QString MyTableWidget::fullPathOfSelectedSong() {
    QString fullPath; // null string will be returned, if nothing was selected (call not meant for this table)

    QModelIndexList list = selectionModel()->selectedRows();
//    qDebug() << "list:" << list;

    if (list.count() == 1) {
        // if exactly one row selected...the call was meant for us
        int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1
//        qDebug() << "row: " << row;

        fullPath = item(row, 4)->text();
    }
//    qDebug() << "fullPath: " << fullPath;
    return(fullPath);
}
