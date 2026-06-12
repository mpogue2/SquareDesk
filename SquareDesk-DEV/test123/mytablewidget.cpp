/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
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

#include "QtCore/qmimedata.h"
#include "QtGui/qevent.h"
#include "QtWidgets/qapplication.h"
#include "QtWidgets/qlabel.h"
#include "globaldefines.h"
#include "songlistmodel.h"

#include "mytablewidget.h"
#include <QDebug>
#include <QDrag>
#include <utility>
#include <QPainter>
#include <QStyleOptionViewItem>
#include "songtitlelabel.h"
#include "dragicon.h"

// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop
#include "ui_mainwindow.h"

#include "tablenumberitem.h"

MyTableWidget::MyTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false); // We'll draw our own
    mw = nullptr;
    dropIndicatorRow = -1; // for drop location highlighting
}

// returns true, if one of the items in the table is being edited (in which case,
//   we don't want keypresses to be hotkeys, we want them to go into the field being edited.
bool MyTableWidget::isEditing()
{
    return (state() == QTableWidget::EditingState);
}

bool rowsNumbersAreContiguous(const QList<int>& rowNumbers) {
    // Empty list is considered contiguous
    if (rowNumbers.isEmpty()) {
        return true;
    }

    // Create a sorted copy to check sequence
    QList<int> sorted = rowNumbers;
    std::sort(sorted.begin(), sorted.end());

    // Check if each number is exactly 1 more than the previous
    for (int i = 1; i < sorted.size(); ++i) {
        if (sorted[i] != sorted[i-1] + 1) {
            return false; // Gap found or duplicate number
        }
    }

    return true;
}

// -----------------------------------------------
bool MyTableWidget::moveSelectedItemsUp() {
    QModelIndexList list = selectionModel()->selectedRows();

    QList<int> rows;
    for (const auto &m : std::as_const(list)) {
        rows.append(m.row());
        // qDebug() << "m.row()" << m.row();
    }

    std::sort(rows.begin(), rows.end()); // ascending sort intentionally

    if (rowsNumbersAreContiguous(rows) && rows.startsWith(0)) {
        // qDebug() << "At top AND contiguous!";
        return false;  // do nothing, if they are all at the top AND contiguous
    }

    // qDebug() << "***** MOVESELECTEDITEMS UP SORTED: " << rows;

    if (rows.count() == 0) {
        return false; // we didn't handle it
    } else if (rows.count() == 1) {
        // SINGLE ROW CASE  TODO: this can be folded into the MULTI case
        int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

        if (row == 0) {
            // we are already at the top
            return false; // we did what was requested, but no changes actually made
        }

        // swap the numbers
        QTableWidgetItem *theItem      = item(row,0);    // # column
        QTableWidgetItem *theItemAbove = item(row - 1,0);

        QString t = theItem->text();
        theItem->setText(theItemAbove->text());
        theItemAbove->setText(t);

        sortItems(0);  // resort, based on column 0 (the #)

        // return true;  // We did it! NOTE: FALL THROUGH TO BOTTOM
    } else {
        // MULTI ROW CASE
        // if we need to move 2,3, we move 2 up, so we get 2,1,3,4
        //  then we move 3 up to get 2,3,1,4
        for (const auto &row : std::as_const(rows)) {

            // qDebug() << "moving up: " << row;
            if (row == 0) {
                // we are already at the top
                // qDebug() << "already at the top: " << row;
                continue; // skip this one and do the next one
            }

            // swap the numbers
            QTableWidgetItem *theItem      = item(row,0);    // # column
            QTableWidgetItem *theItemAbove = item(row - 1,0);

            QString t = theItem->text();
            theItem->setText(theItemAbove->text());
            theItemAbove->setText(t);

            sortItems(0);  // resort, based on column 0 (the #), moving row "row" to the bottom
        }

    // NOTE: the rows we moved will still be selected
    // scrollToItem(item(rowCount()-1, 0));      // EnsureVisible for the last row in the table NO NEED TO SCROLL
    // return true; // we did it! // NOTE: FALL THROUGH TO BOTTOM
    }

    // NOTE: the rows we moved will still be selected
    // scrollToItem(item(rowCount()-1, 0));      // EnsureVisible for the last row in the table NO NEED TO SCROLL
    list = selectionModel()->selectedRows(); // update, because row order has changed
    int firstRow = list.at(0).row();
    scrollToItem(item(firstRow, 0));      // EnsureVisible for the last row in the table NO NEED TO SCROLL
    return true; // return true from single row and multi-row cases
}

// -----------------------------------------------
bool MyTableWidget::moveSelectedItemsDown() {
    QModelIndexList list = selectionModel()->selectedRows();

    QList<int> rows;
    for (const auto &m : std::as_const(list)) {
        rows.append(m.row());
        // qDebug() << "m.row()" << m.row();
    }

    std::sort(rows.begin(), rows.end(), std::greater<int>()); // descending sort intentionally

    // qDebug() << "this->rowCount()" << this->rowCount() << rowsNumbersAreContiguous(rows) << rows.endsWith(this->rowCount()-1);
    if (rowsNumbersAreContiguous(rows) && rows.startsWith(this->rowCount()-1)) {
        // use startsWith, because the order is DESCENDING here
        // qDebug() << "At bottom AND contiguous!";
        return false;  // do nothing, if they are all at the bottom AND contiguous
    }

    // qDebug() << "***** MOVESELECTEDITEMS DOWN SORTED: " << rows;

    if (rows.count() == 0) {
        return false; // we didn't handle it
    } else if (rows.count() == 1) {
        // SINGLE ROW CASE  TODO: this can be folded into the MULTI case
        int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

        if (row == rowCount()-1) {
            // we are already at the bottom
            return false; // we did what was requested, but no changes actually made
        }

        // swap the numbers
        QTableWidgetItem *theItem      = item(row,0);    // # column
        QTableWidgetItem *theItemBelow = item(row + 1,0);

        QString t = theItem->text();
        theItem->setText(theItemBelow->text());
        theItemBelow->setText(t);

        sortItems(0);  // resort, based on column 0 (the #)
        // return true;  // We did it! // FALL THROUGH TO BOTTOM
    } else {
        // MULTI ROW CASE
        // if we need to move 1,2,3, we move 3 down, so we get 1,2,4,3
        //  then we move 2 down to get 1,4,2,3
        //  then we move 1 down to get 4,1,2,3, q.e.d.
        for (const auto &row : std::as_const(rows)) {

            // qDebug() << "moving down: " << row;
            if (row == rowCount()-1) {
                // we are already at the bottom
                // qDebug() << "already at the bottom: " << row;
                continue; // skip this one and do the next one
            }

            // swap the numbers
            QTableWidgetItem *theItem      = item(row,0);    // # column
            QTableWidgetItem *theItemBelow = item(row + 1,0);

            QString t = theItem->text();
            theItem->setText(theItemBelow->text());
            theItemBelow->setText(t);

            sortItems(0);  // resort, based on column 0 (the #), moving row "row" to the bottom
    }

        // NOTE: the rows we moved will still be selected!
        // scrollToItem(item(rowCount()-1, 0));      // EnsureVisible for the last row in the table NO NEED TO SCROLL

        // return true; // we did it! // FALL THROUGH TO BOTTOM
    }

    // NOTE: the rows we moved will still be selected
    // scrollToItem(item(rowCount()-1, 0));      // EnsureVisible for the last row in the table NO NEED TO SCROLL
    list = selectionModel()->selectedRows(); // update, because row order has changed
    int firstRow = list.at(0).row();
    scrollToItem(item(firstRow, 0));      // EnsureVisible for the last row in the table NO NEED TO SCROLL
    return true; // return true from single row and multi-row cases
}

// -----------------------------------------------
bool MyTableWidget::moveSelectedItemsToTop() {
QModelIndexList list = selectionModel()->selectedRows();

QList<int> rows;
for (const auto &m : std::as_const(list)) {
    rows.append(m.row());
}

std::sort(rows.begin(), rows.end(), std::greater<int>()); // descending sort intentionally

// qDebug() << "***** MOVESELECTEDITEMSTOTOP SORTED: " << rows;

if (rows.count() == 0) {
    return false; // we didn't handle it
} else if (rows.count() == 1) {
    // SINGLE ROW CASE, as before  TODO: this can be folded into the MULTI case
    int row = list.at(0).row(); // only 1 row is selected, this is its number 0 - N-1

    if (row == 0) {
        // we are already at the top
        return false; // no need to scroll
    }

    // Iterate over the entire table, incrementing items BELOW this item

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
            QString newIndex = QString::number(playlistIndexInt + 1);
            item(i,0)->setText(newIndex);
        }
    }

    theItem1->setText("1");  // this one becomes #1

    sortItems(0);  // resort, based on column 0 (the #)

    // NOTE: the rows we moved to the top will still be selected!
    scrollToItem(item(0, 0));      // EnsureVisible for the first row in the table

    ((MainWindow *)mw)->saveSlotNow(this);  // drag operations save and possibly update the darkSongTable view immediately

    return true; // we did it!
} else {
    // MULTI ROW CASE
    int numMoved = 0;
    // if we need to move 2,3,4 we move 4 to the top, then we have 4,1,2,3 and numMoved = 1
    //  then we move 3 + numMoved = 4 (because it's now in the fourth row)
    //  etc.
    for (const auto &row : std::as_const(rows)) {

        int rowToMove = row + numMoved;
        // qDebug() << "moving to top: " << rowToMove;
        if (rowToMove == 0) {
            // we are already at the top
            // qDebug() << "already at the top: " << rowToMove;
            continue; // skip this one and do the next one (should not be one)
        }

        // Iterate over the entire songTable, incrementing items BELOW this item

        // what's the number of THIS item?
        QTableWidgetItem *theItem1 = item(rowToMove,0);
        QString playlistIndexText1 = theItem1->text();  // this is the playlist #
        int currentNumberInt = playlistIndexText1.toInt();

        // iterate through the entire table, and if number is greater than THIS item's number, decrement it
        for (int i=0; i<rowCount(); i++) {
            QTableWidgetItem *theItem = item(i,0);
            QString playlistIndexText = theItem->text();  // this is the playlist #
            int playlistIndexInt = playlistIndexText.toInt();

            if (playlistIndexInt < currentNumberInt) {
                // if a # was less, increment it
                QString newIndex = QString::number(playlistIndexInt + 1);
                item(i,0)->setText(newIndex);
            }
        }

        theItem1->setText("1");  // this one becomes #1

        sortItems(0);  // resort, based on column 0 (the #), moving row "row" to the bottom

        numMoved++;
    }

    // NOTE: the rows we moved to the top will still be selected!
    scrollToItem(item(0, 0));      // EnsureVisible for the first row in the table

    ((MainWindow *)mw)->saveSlotNow(this);  // drag operations save and possibly update the darkSongTable view immediately

    return true; // we did it!
    }
}

// -----------------------------------------------
bool MyTableWidget::moveSelectedItemsToBottom(bool scrollWhenDone) { // defaults to scrollWhenDone == true
    QModelIndexList list = selectionModel()->selectedRows();

    QList<int> rows;
    for (const auto &m : std::as_const(list)) {
        rows.append(m.row());
    }

    std::sort(rows.begin(), rows.end()); // ascending sort intentionally

    // qDebug() << "***** MOVESELECTEDITEMSTOBOTTOM SORTED: " << rows;

    if (rows.count() == 0) {
        return false; // we didn't handle it
    } else if (rows.count() == 1) {
        // SINGLE ROW CASE  TODO: this can be folded into the MULTI case
        int row = list.at(0).row(); // only 1 row is selected, this is its number 0 - N-1

        if (row == rowCount()-1) {
            // we are already at the bottom
            return false || !scrollWhenDone; // no need to scroll, except return true when we're removing the last row
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

        if (scrollWhenDone) {
            scrollToBottom();
        }

        ((MainWindow *)mw)->saveSlotNow(this);  // drag operations save and possibly update the darkSongTable view immediately

        return true; // we did it!
    } else {
        // MULTI ROW CASE
        int numMoved = 0;
        // if we need to move 1,2,3, we move 1, then numMoved = 1
        //  then we move 2 - numMoved = 1 (because it's now in the first row)
        //  etc.
        for (const auto &row : std::as_const(rows)) {

            int rowToMove = row - numMoved;
            // qDebug() << "moving to bottom: " << rowToMove;
            if (rowToMove == rowCount()-1) {
                // we are already at the bottom
                // qDebug() << "already at the bottom: " << row;
                continue; // skip this one and do the next one (should not be one)
            }

            // Iterate over the entire songTable, incrementing items BELOW this item

            // what's the number of THIS item?
            QTableWidgetItem *theItem1 = item(rowToMove,0);
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

            sortItems(0);  // resort, based on column 0 (the #), moving row "row" to the bottom

            numMoved++;
        }

        // NOTE: the rows we moved to the bottom will still be selected!
        scrollToItem(item(rowCount()-1, 0));      // EnsureVisible for the last row in the table

        ((MainWindow *)mw)->saveSlotNow(this);  // drag operations save and possibly update the darkSongTable view immediately

        return true; // we did it!
    }
}

// -----------------------------------------------
bool MyTableWidget::removeSelectedItems() {
    QModelIndexList list = selectionModel()->selectedRows();

    QList<int> rows;
    for (const auto &m : std::as_const(list)) {
        rows.append(m.row());
    }

    std::sort(rows.begin(), rows.end(), std::greater<int>()); // descending sort

    // qDebug() << "***** REMOVE SORTED: " << rows;

    if (rows.count() == 0) {
        return false;  // not meant for us, go try a different playlist slot
    } else if (rows.count() == 1) {
        // SINGLE ROW CASE    TODO: this can be folded into the MULTI case
        // OK, remember the row number, because we're going to restore it at the end...
        int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

        if (moveSelectedItemsToBottom(false)) { // do NOT scroll to the bottom

            // if this call was meant for us, then we can remove the row; otherwise, ignore
            removeRow(rowCount()-1);      // and remove the last row

            if (row > rowCount()-1) {
                // if we were removing the LAST row, we want to select the second-to-last row now
                //   any other row, and we just redo the selection to that row
                row = rowCount()-1;
            }

            // qDebug() << "SELECTING AND SCROLLING TO ROW:" << row;
            // qDebug() << "BEFORE selected rows: " << selectionModel()->selectedRows() << row;
            // selectRow(row);    // select it
            setCurrentIndex(model()->index(row, 0));
            scrollToItem(item(row-1, 0)); // EnsureVisible for the row that was deleted, OR last row in table
            // qDebug() << "AFTER selected rows: " << selectionModel()->selectedRows() << row;
        }

        ((MainWindow *)mw)->saveSlotNow(this);  // drag operations save and possibly update the darkSongTable view immediately

        return true;
    } else {
        // MULTI ROW CASE
        // now delete, starting with the lowest down row in the list, going UP
        for (const auto &row : std::as_const(rows)) {
            // qDebug() << "REMOVING ROW:" << row;

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

            sortItems(0);  // resort, based on column 0 (the #), which moves row to the bottom

            removeRow(rowCount()-1);      // and remove the last row
        }

        int firstrowaffected = rows.at(rows.count()-1);
        // qDebug() << "firstrowaffected: " << firstrowaffected;
        setCurrentIndex(model()->index(firstrowaffected, 0));
        scrollToItem(item(firstrowaffected, 0));     // EnsureVisible for the first row in the table

        // selectRow(0); // we are done deleting all the rows, so select first row (what would be better?)
        // scrollToItem(item(0, 0));     // EnsureVisible for the first row in the table

        ((MainWindow *)mw)->saveSlotNow(this);  // drag operations save and possibly update the darkSongTable view immediately

        return true; // yeah, we handled this
    }
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

void MyTableWidget::mousePressEvent(QMouseEvent *event)
{
    // qDebug() << "***** MyTableWidget::mousePressEvent";
    if (event->button() == Qt::LeftButton) {
        dragStartPosition = event->position().toPoint(); // FIX: use position().toPoint() instead of deprecated pos()
        // qDebug() << "***** MyTableWidget::mousePressEvent" << dragStartPosition;

    }
    QTableWidget::mousePressEvent(event);
}

static QRegularExpression title_tags_remover2("(\\&nbsp\\;)*\\<\\/?span( .*?)?>");
static QRegularExpression spanPrefixRemover2("<span style=\"color:.*\">(.*)</span>", QRegularExpression::InvertedGreedinessOption);

void MyTableWidget::mouseMoveEvent(QMouseEvent *event)
{
    // qDebug() << "***** MyTableWidget::mouseMoveEvent";
    if (mw != nullptr) {
        // qDebug() << "auditionInProgress: " << ((MainWindow *)mw)->auditionInProgress;
        if (((MainWindow *)mw)->auditionInProgress) {
            return;  // return, if one of the audition buttons is being held down, do NOT do drag and drop in this case
        }
    }

    if (!(event->buttons() & Qt::LeftButton)) {
        // qDebug() << "return myTableWidget no left button pressed";
        return; // return if not left button down and move
    }
    if ((event->position().toPoint() - dragStartPosition).manhattanLength() < QApplication::startDragDistance()) { // FIX: use position().toPoint() instead of deprecated pos()
        // qDebug() << "return myTableWidget not moved far enough";
        return; // return if haven't moved far enough with L mouse button down
    }

    // Defer allocations until after we know there's something to drag

    int sourceRow = 0; // = itemAt(dragStartPosition)->row();
    QString sourceInfo;
    int rowNum = 0;

    // qDebug() << "mouseMoveEvent:" << sourceRow;

    if (objectName().startsWith("playlist")) {
        // the source is a playlist or track filter --------

        for (const auto &mi : selectionModel()->selectedRows()) {
            sourceRow = mi.row();  // this is the actual row number of each selected row, overriding the cursor-located row (just pick all selected rows)
            // qDebug() << "DRAGGING THIS PLAYLIST ROW NUMBER:" << sourceRow;

            QString title = dynamic_cast<QLabel*>(this->cellWidget(sourceRow, 1))->text();
            title.replace(spanPrefixRemover2, "\\1"); // remove <span style="color:#000000"> and </span> title string coloring
            int where = title.indexOf(title_tags_remover2);
            if (where >= 0) {
                title.truncate(where);
            }
            title.replace("&quot;","\"").replace("&amp;","&").replace("&gt;",">").replace("&lt;","<");  // if filename contains HTML encoded chars, put originals back

            QString sourceName = objectName();

            QString sourceTrackName = title; // e.g. "ESP 1234 - Ricochet"
            QString sourcePitch = item(sourceRow, 2)->text();
            QString sourceTempo = item(sourceRow, 3)->text();
            QString sourcePath = item(sourceRow, 4)->text();
            QString sourceInfoForThisRow = (rowNum++ > 0 ? "!!&!!" : "") +  // row separator
                          sourceTrackName + "                                    !#!" + // the spaces are so that the visible drag text is just the track name (hack!)
                          sourceName + "!#!" +
                          sourcePitch + "!#!" +
                          sourceTempo + "!#!" +
                          sourcePath; // send all the info!
            sourceInfo.append(sourceInfoForThisRow);
        }
        // qDebug() << "playlist sourceInfo: " << sourceInfo;  // this has info on ALL the selected items

        if (sourceInfo == "") {
            return;  // we tried to drag "nothing"
        }

        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setText(sourceInfo);
        drag->setMimeData(mimeData);

        // Set custom drag icon with count badge
        QPixmap dragPixmap = DragIcon::createDragIcon(rowNum);
        drag->setPixmap(dragPixmap);
        drag->setHotSpot(QPoint(dragPixmap.width() / 2, dragPixmap.height() / 2));

        Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
        Q_UNUSED(dropAction)
        delete drag; // QDrag takes ownership of mimeData
    } else {
        // the source is the darkSongTable --------

        for (const auto &mi : selectionModel()->selectedRows()) {
            sourceRow = mi.row();  // this is the actual row number of each selected row, overriding the cursor-located row (just pick all selected rows)
            // qDebug() << "DRAGGING THIS ROW NUMBER:" << sourceRow;

            QString title = dynamic_cast<QLabel*>(this->cellWidget(sourceRow, kTitleCol))->text();
            title.replace(spanPrefixRemover2, "\\1"); // remove <span style="color:#000000"> and </span> title string coloring

            int where = title.indexOf(title_tags_remover2);
            if (where >= 0) {
                title.truncate(where);
            }
            title.replace("&quot;","\"").replace("&amp;","&").replace("&gt;",">").replace("&lt;","<");  // if filename contains HTML encoded chars, put originals back

            if (isRowHidden(sourceRow)) {
                // don't allow drag and drop for rows that are not visible!  This only is a problem for darkSongTable, which may have filters applied.
                // qDebug() << "no drag and drop for you: " << sourceRow << title;
                continue;
            } else {
                // qDebug() << "drag and drop is OK for you: " << sourceRow << title;
            }

            QString sourceName = objectName();

            QString sourceTrackName = item(sourceRow, kLabelCol)->text() + " - " + title; // e.g. "ESP 1234 - Ricochet"
            QString sourcePitch = item(sourceRow, kPitchCol)->text();
            QString sourceTempo = item(sourceRow, kTempoCol)->text();
            QString sourcePath = item(sourceRow, kPathCol)->data(Qt::UserRole).toString();

            QString sourceInfoForThisRow = (rowNum++ > 0 ? "!!&!!" : "") +
                                           sourceTrackName + "                                    !#!" + // the spaces are so that the visible drag text is just the track name (hack!)
                                           sourceName + "!#!" +
                                           sourcePitch + "!#!" +
                                           sourceTempo + "!#!" +
                                           sourcePath;
            sourceInfo.append(sourceInfoForThisRow);
        }

        // qDebug() << "sourceInfo: " << sourceInfo;  // this has info on ALL the selected items

        if (sourceInfo == "") {
            return; // we tried to drag "nothing"
        }

        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setText(sourceInfo); // send all the info!
        drag->setMimeData(mimeData);

        // Set custom drag icon with count badge
        QPixmap dragPixmap = DragIcon::createDragIcon(rowNum);
        drag->setPixmap(dragPixmap);
        drag->setHotSpot(QPoint(dragPixmap.width() / 2, dragPixmap.height() / 2));

        Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
        Q_UNUSED(dropAction)
        delete drag; // QDrag takes ownership of mimeData
    }
}

void MyTableWidget::dragEnterEvent(QDragEnterEvent *event)
{
    // qDebug() << "dragEnterEvent!" << this;
    if (event->mimeData()->hasFormat("text/plain")) {
        // Only accept drops on playlist tables that are writable
        if (objectName().startsWith("playlist") && isWritableTarget()) {
            event->acceptProposedAction();
        } else {
            event->ignore();
        }
    } else {
        event->ignore();
    }

    // qDebug() << "dragEnterEvent: event->mimeData()->text()" << event->mimeData()->text();
    if (event->mimeData()->text() == "") {
        // guard against moving nothing, which causes crash in the dropEvent
        event->ignore();
    }
}

// Helper method to determine if this table is a writable drop target
bool MyTableWidget::isWritableTarget() const
{
    // Only playlist tables can be writable
    if (!objectName().startsWith("playlist")) {
        return false;
    }
    
    // If we don't have access to the MainWindow, be safe and assume it's not writable
    if (mw == nullptr) {
        return false;
    }
    
    // Get the slot number based on the table name
    int slot = -1;
    if (objectName() == "playlist1Table") {
        slot = 0;
    } else if (objectName() == "playlist2Table") {
        slot = 1;
    } else if (objectName() == "playlist3Table") {
        slot = 2;
    } else {
        return false;  // Unknown playlist table
    }
    
    // Check if it's a Track Filter or Apple Music (read-only targets)
    QString relPath = ((MainWindow *)mw)->relPathInSlot[slot];
    
    // For debugging
    // qDebug() << "isWritableTarget checking: slot=" << slot << ", relPath=" << relPath;
    
    // Track filters start with "/tracks/" and are read-only
    if (relPath.startsWith("/tracks/") || relPath.startsWith("/Tracks/")) {
        // qDebug() << "  -> Track filter detected, not writable";
        return false;
    }
    
    // Check if the playlist is an Apple Music playlist (read-only)
    if (relPath.startsWith("/Apple Music/")) {
        // qDebug() << "  -> Apple Music playlist detected, not writable";
        return false;
    }
    
    // Check if the path is empty (could be a slot with nothing in it yet)
    if (relPath.isEmpty()) {
        // qDebug() << "  -> Empty path, slot is writable";
        return true;
    }
    
    // It's a regular playlist, which is writable
    // qDebug() << "  -> Regular playlist, is writable";
    return true;
}

void MyTableWidget::dragMoveEvent(QDragMoveEvent *event)
{
    // Only playlist tables that are writable allow reordering/dropping
    if (!objectName().startsWith("playlist") || !isWritableTarget()) {
        event->ignore();
        return;
    }

    // Find the row where the drop would occur
    QPoint pos = event->position().toPoint(); // FIX: use position().toPoint() instead of deprecated pos()
    int row = rowAt(pos.y());
    
    // Set drop indicator row
    if (row < 0) {
        // Below last row
        dropIndicatorRow = rowCount();
    } else {
        QRect rect = visualRect(model()->index(row, 0));
        if (pos.y() < rect.top() + rect.height() / 2) {
            dropIndicatorRow = row;
        } else {
            dropIndicatorRow = row + 1;
        }
    }
    
    viewport()->update();
    event->acceptProposedAction();
}

void MyTableWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
    // Always clear the drop indicator when leaving
    dropIndicatorRow = -1;
    viewport()->update();
}

void MyTableWidget::dropEvent(QDropEvent *event)
{
    // qDebug() << "***** MyTableWidget::dropEvent *****";
    // qDebug() << this << " got this text: " << event->mimeData()->text();

    // Check if this is a writable target - if not, reject the drop
    if (!isWritableTarget()) {
        event->ignore();
        return;
    }

    event->acceptProposedAction();

    QStringList rows = event->mimeData()->text().split("!!&!!");

    // Save drop position before clearing indicator
    int targetRow = dropIndicatorRow; // FIX: avoid shadowing insertRow()
    dropIndicatorRow = -1;
    viewport()->update();

    // If source and dest are the same playlist, do reordering
    QString sourceName;
    if (!rows.isEmpty()) {
        QStringList fields = rows[0].split("!#!");
        if (fields.size() > 1)
            sourceName = fields[1];
    }
    QString destName = objectName();

    if (sourceName == destName) {
        // INTERNAL MOVE/REORDER ===================
        // qDebug() << "INTERNAL MOVE/REORDER";

        QList<int> selRows;
        for (const auto &mi : selectionModel()->selectedRows()) {
            selRows.append(mi.row());
        }
        std::sort(selRows.begin(), selRows.end());

        // Adjust targetRow if moving downwards
        int minSel = selRows.first();
        if (targetRow > minSel)
            targetRow -= selRows.size();

        // Copy row data
        QList<QList<QTableWidgetItem*>> copiedItems;
        QList<QWidget*> copiedWidgets;
        for (int r : selRows) {
            QList<QTableWidgetItem*> items;
            for (int c = 0; c < columnCount(); ++c) {
                QTableWidgetItem* orig = item(r, c);
                items.append(orig ? orig->clone() : nullptr);
            }
            copiedItems.append(items);
            QWidget* w = cellWidget(r, 1);
            if (w) {
                darkPaletteSongTitleLabel* oldLabel = dynamic_cast<darkPaletteSongTitleLabel*>(w);
                if (oldLabel) {
                    // clone the fancy palette label
                    darkPaletteSongTitleLabel* newLabel = new darkPaletteSongTitleLabel((MainWindow *)mw);
                    newLabel->setTextFormat(Qt::RichText);
                    newLabel->setText(oldLabel->text());
                    copiedWidgets.append(newLabel); // copy fancy label
                } else {
                    copiedWidgets.append(nullptr);
                }
            } else {
                copiedWidgets.append(nullptr);
            }
        }

        // Remove from bottom up
        for (int i = selRows.size()-1; i >= 0; --i) {
            removeRow(selRows[i]);
        }

        // Insert at new location
        for (int i = 0; i < copiedItems.size(); ++i) {
            insertRow(targetRow + i);
            for (int c = 0; c < columnCount(); ++c) { // not pasting # item in col 0
                setItem(targetRow + i, c, copiedItems[i][c]);
            }
            if (copiedWidgets[i]) {
                setCellWidget(targetRow + i, 1, copiedWidgets[i]); // paste fancy label
            }
        }

        // internal playlist, must renumber them
        // qDebug() << "INTERNAL RENUMBER:";

        for (int i = 0; i < this->rowCount(); i++) {
            // this->item(i,0)->setText(QString::number(i+1)); // this is wrong, need to recreate the TableNumberItems
            QTableWidgetItem *num = new TableNumberItem(QString::number(i+1)); // use TableNumberItem so that it sorts numerically
            setItem(i, 0, num);
        }

        // --- Fix: Reset dropIndicatorRow and force viewport update to restore drag state ---
        dropIndicatorRow = -1;
        viewport()->update();
        // Also clear selection and focus to avoid selection model confusion
        clearSelection();
        setCurrentCell(-1, -1);
        setFocus();

        event->acceptProposedAction();

        int whichSlot = 0;
        if (this == ((MainWindow *)mw)->ui->playlist1Table) {
            whichSlot = 0;
        } else if (this == ((MainWindow *)mw)->ui->playlist2Table) {
            whichSlot = 1;
        } else if (this == ((MainWindow *)mw)->ui->playlist3Table) {
            whichSlot = 2;
        }

        ((MainWindow *)mw)->slotModified[whichSlot] = true;

        ((MainWindow *)mw)->saveSlotNow(this);  // drag operations save and possibly update the darkSongTable view immediately

        // Refresh indentation after internal move (issue #1547)
        ((MainWindow *)mw)->refreshAllPlaylists();

        return;
    }

    // ...existing code for drag from darkSongTable or other playlist...
    int itemNumber = 0;
    for (const auto &r : std::as_const(rows)) {
        // ...existing code...
        // Use targetRow instead of always appending to bottom
        QStringList fields = r.split("!#!"); // split the row into fields
        QString sourceTrackName = fields[0].trimmed();
        QString sourceName      = fields[1];
        QString sourcePitch     = fields[2];
        QString sourceTempo     = fields[3];
        QString sourcePath      = fields[4];

        QString destName = objectName();

        // TODO: use a QMap here
        int sourceSlot = 0;
        if (sourceName == "playlist1Table") {
            sourceSlot = 0;
        } else if (sourceName == "playlist2Table") {
            sourceSlot = 1;
        } else if (sourceName == "playlist3Table") {
            sourceSlot = 2;
        } else if (sourceName == "darkSongTable") {
            sourceSlot = -1;
        } else {
            qDebug() << "SLOT ERROR: " << sourceName;
        }

        int destSlot = 0;
        if (destName == "playlist1Table") {
            destSlot = 0;
        } else if (destName == "playlist2Table") {
            destSlot = 1;
        } else if (destName == "playlist3Table") {
            destSlot = 2;
        } else if (destName == "darkSongTable") {
            destSlot = -1;
        } else {
            qDebug() << "SLOT ERROR" << destName;
        }

        // source is playlist OR track filter
        // qDebug() << "sourceSlot/destSlot: " << sourceSlot << destSlot;
        QString sourceRelPath, destRelPath;
        if (mw != nullptr) {
            if (sourceSlot != -1) {
                sourceRelPath = ((MainWindow *)mw)->relPathInSlot[sourceSlot];
            } else {
                sourceRelPath = "";
            }
            if (destSlot != -1) {
                destRelPath = ((MainWindow *)mw)->relPathInSlot[destSlot];
            } else {
                destRelPath = "";
            }
        }
        bool sourceIsTrackFilter = sourceRelPath.startsWith("/tracks/");
        bool destIsTrackFilter   = destRelPath.startsWith("/tracks/");

        if (sourceName == "darkSongTable") {
            if (destName == "darkSongTable") {
                // NOPE.  We don't allow reordering of the darkSongTable ever.
                // qDebug() << "NO REORDERING OF DARKSONGTABLE";
            } else {
                // from darkSongTable to this playlist, this is feature request #1018
                // qDebug() << "***** DROP:" << sourceName << sourceTrackName << sourcePitch << sourceTempo << sourcePath << targetRow;

                // (MainWindow*)mw->darkAddPlaylistItemToBottom(whichSlot, sourceTrackName, sourcePitch, sourceTempo, sourcePath, ""); // slot is 0 - 2

                if (destIsTrackFilter) {
                    // qDebug() << "NO DROPPING FROM DARKSONGTABLE TO TRACK FILTERS";
                } else {
                    // FROM DARKSONGTABLE TO PLAYLIST-THAT-IS-NOT-A-TRACK-FILTER ================
                    if (mw != nullptr) {
                        // playlists only, not track filters
                        // ((MainWindow *)mw)->darkAddPlaylistItemToBottom(destSlot, sourceTrackName, sourcePitch, sourceTempo, sourcePath, "");
                        ((MainWindow *)mw)->darkAddPlaylistItemAt(destSlot, sourceTrackName, sourcePitch, sourceTempo, sourcePath, "", targetRow + itemNumber);
                        itemNumber++;
                        ((MainWindow *)mw)->saveSlotNow(destSlot);  // drag operations save and possibly update the darkSongTable view immediately
                    } else {
                        qDebug() << "ERROR: mw not valid";
                    }
                }
            }
        } else {
            if (destName == "darkSongTable") {
                // qDebug() << "NO DROPPING OF ANYTHING ON DARKSONGTABLE (YET)";
                // NOPE.  Not yet at least.
                return;
            }

            // qDebug() << "source is playlist or track filter:" << sourceIsTrackFilter << sourceRelPath << destIsTrackFilter << destRelPath;

            if (sourceIsTrackFilter) {
                // source is TRACK FILTER
                if (destIsTrackFilter) {
                    // NOPE
                    // qDebug() << "NO DROPPING OF TRACK FILTER ON TRACK FILTER";
                } else {
                    // source is Track Filter, dest is Playlist
                    // qDebug() << "***** DRAG N DROP from TRACK FILTER to PLAYLIST: " << sourceName << destName;
                    if (mw != nullptr) {
                        // qDebug() << "TF2PL: " << destSlot << sourceTrackName << sourcePitch << sourceTempo << sourcePath;
                        ((MainWindow *)mw)->darkAddPlaylistItemAt(destSlot, sourceTrackName, sourcePitch, sourceTempo, sourcePath, "", targetRow + itemNumber);
                        itemNumber++;
                        ((MainWindow *)mw)->saveSlotNow(destSlot);  // drag operations save and possibly update the darkSongTable view immediately
                    } else {
                        qDebug() << "ERROR: mw not valid";
                    }
                }
            } else {
                // source is PLAYLIST
                if (destIsTrackFilter) {
                    // NOPE
                    // qDebug() << "NO DROPPING OF PLAYLIST ON TRACK FILTER";
                } else {
                    // source is Playlist, dest is Playlist
                    if (sourceName == destName) {
                        // TODO: dragging within a playlist we want to support
                        // qDebug() << "***** DRAG N DROP WITHIN A PLAYLIST: " << sourceName;
                    } else {
                        // dragging between playlists we want to support
                        // --- Fix: insert at drop position, not at end ---
                        if (mw != nullptr) {
                            ((MainWindow *)mw)->darkAddPlaylistItemAt(destSlot, sourceTrackName, sourcePitch, sourceTempo, sourcePath, "", targetRow + itemNumber);
                            itemNumber++;
                            ((MainWindow *)mw)->saveSlotNow(destSlot);  // drag operations save and possibly update the darkSongTable view immediately
                        } else {
                            qDebug() << "ERROR: mw not valid";
                        }
                    }
                }
            }
        }


    } // for each selected row in the source table

    // Refresh indentation after all drops complete (issue #1547)
    // This handles cases where adding/moving items affects indentation of other rows
    if (mw != nullptr && itemNumber > 0) {
        ((MainWindow *)mw)->refreshAllPlaylists();
    }
}

void MyTableWidget::setMainWindow(void *m) {
    mw = m; // save it.  MainWindow will cast it
}

void MyTableWidget::paintEvent(QPaintEvent *event)
{
    QTableWidget::paintEvent(event);

    // Draw drop indicator if needed - only for writable targets
    if (dropIndicatorRow >= 0 && isWritableTarget()) {
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(Qt::green, 3);
        painter.setPen(pen);

        int y = 0;
        if (dropIndicatorRow < rowCount()) {
            QRect rect = visualRect(model()->index(dropIndicatorRow, 0));
            y = rect.top();
        } else {
            // After last row
            if (rowCount() > 0) {
                QRect rect = visualRect(model()->index(rowCount()-1, 0));
                y = rect.bottom();
            } else {
                y = 0;
            }
        }
        painter.drawLine(0, y, viewport()->width(), y);
    }
}

// ================================================================================
//  Dark Table Widget persistent sorting order
// ================================================================================
//
// ------------------------------------------------
// we're doing this so we can keep track of the ordering, so we can make it persistent
//  this doesn't catch the user header clicks, though...
// void MyTableWidget::sortItems (int column, Qt::SortOrder order) {
//     // emit sort();
//     // qDebug() << "MyTableWidget::sortItems was called: " << column << order;

//     // if (sortOperations.size() >= maxSortOperations) {
//     //     // full
//     //     sortOperations.dequeue();  // throw away the head
//     // }
//     // sortOperations.enqueue(sortOperation(column, order)); // push new one into the FIFO

//     // // DEBUG: tell me what's in the FIFO now -----
//     // qDebug() << "*** updated sortOperations ***";
//     // for (const auto &item : sortOperations) {
//     //     qDebug() << "sortOperations sortItems: " << item.toString();
//     // }

//     QTableWidget::sortItems(column, order);
// }

// this catches the user header clicks -----
void MyTableWidget::onHeaderClicked(int column) {
    QHeaderView *header = this->horizontalHeader();
    Qt::SortOrder order = header->sortIndicatorOrder();
    // qDebug() << "onHeaderClicked to sort: " << column << order;

    // push into the FIFO -----
    if (sortOperations.size() >= maxSortOperations) {
        // it's full, so kick one out
        sortOperations.remove(0);  // throw away the head (item 0)
    }

    // must delete any prior remembered sortOperations on that column, before
    //   we put in this one
    for (int i = 0; i < sortOperations.size(); i++) {
        if (sortOperations[i].column == column) {
            sortOperations.removeAt(i); // delete if the column matches
        }
    }

    sortOperations.append(sortOperation(column, order)); // push new one into the FIFO

    // DEBUG: tell me what's in the FIFO now -----
    QString allSortOperationsString = "";
    // qDebug() << "*** updated sortOperations ***";
    for (int i = 0; i < sortOperations.size(); i++) {
        allSortOperationsString += sortOperations[i].toString();

        if (i < sortOperations.size()-1) {
            allSortOperationsString += ";";
        }
    }

    // qDebug() << "onHeaderClicked allSortOperations =" << allSortOperationsString;
    emit newStableSort(allSortOperationsString); // asks MainWindow to persist it
}

void MyTableWidget::setOrderFromString(QString s) {
    // called from darkloadMusicList to set the sort order in the darkSongTable
    // qDebug() << "setOrderFromString:" << s;

    // sortOperations.clear(); // NO, DON'T clear out the old here, do it only when told in initializeSortOrder() (when resetting)

    // string looks like: "c:2,so:0;c:1,so:0;c:3,so:0"
    const QStringList a = s.split(";");
    for (const auto &q : a) {
        sortOperation so(q);
        sortOperations.append(so);
        sortItems(so.column, so.sortOrder); // do the sorting of the table items
    }

}

void MyTableWidget::initializeSortOrder() {
    sortOperations.clear();
    emit newStableSort("");
}
