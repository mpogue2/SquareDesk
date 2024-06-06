/****************************************************************************
**
** Copyright (C) 2016-2024 Mike Pogue, Dan Lyke
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

// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop
#include "ui_mainwindow.h"

MyTableWidget::MyTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
    setAcceptDrops(true);
    mw = nullptr;
}

// returns true, if one of the items in the table is being edited (in which case,
//   we don't want keypresses to be hotkeys, we want them to go into the field being edited.
bool MyTableWidget::isEditing()
{
    return (state() == QTableWidget::EditingState);
}

// -----------------------------------------------
bool MyTableWidget::moveSelectedItemsUp() {
    QModelIndexList list = selectionModel()->selectedRows();

    QList<int> rows;
    foreach (const QModelIndex &m, list) {
        rows.append(m.row());
    }

    std::sort(rows.begin(), rows.end()); // ascending sort intentionally

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
        foreach (const int & row, rows) {

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
    foreach (const QModelIndex &m, list) {
        rows.append(m.row());
    }

    std::sort(rows.begin(), rows.end(), std::greater<int>()); // descending sort intentionally

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
        foreach (const int & row, rows) {

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
foreach (const QModelIndex &m, list) {
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

    return true; // we did it!
} else {
    // MULTI ROW CASE
    int numMoved = 0;
    // if we need to move 2,3,4 we move 4 to the top, then we have 4,1,2,3 and numMoved = 1
    //  then we move 3 + numMoved = 4 (because it's now in the fourth row)
    //  etc.
    foreach (const int & row, rows) {

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

    return true; // we did it!
    }
}

// -----------------------------------------------
bool MyTableWidget::moveSelectedItemsToBottom(bool scrollWhenDone) { // defaults to scrollWhenDone == true
    QModelIndexList list = selectionModel()->selectedRows();

    QList<int> rows;
    foreach (const QModelIndex &m, list) {
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

        return true; // we did it!
    } else {
        // MULTI ROW CASE
        int numMoved = 0;
        // if we need to move 1,2,3, we move 1, then numMoved = 1
        //  then we move 2 - numMoved = 1 (because it's now in the first row)
        //  etc.
        foreach (const int & row, rows) {

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

        return true; // we did it!
    }
}

// -----------------------------------------------
bool MyTableWidget::removeSelectedItems() {
    QModelIndexList list = selectionModel()->selectedRows();

    QList<int> rows;
    foreach (const QModelIndex &m, list) {
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
            selectRow(row);    // select it
            scrollToItem(item(row-1, 0)); // EnsureVisible for the row that was deleted, OR last row in table
        }
        return true;
    } else {
        // MULTI ROW CASE
        // now delete, starting with the lowest down row in the list, going UP
        foreach (const int &row, rows) {
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
        selectRow(0); // we are done deleting all the rows, so select first row (what would be better?)
        scrollToItem(item(0, 0));     // EnsureVisible for the first row in the table

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
    if (event->button() == Qt::LeftButton) {
        dragStartPosition = event->pos();
    }
    QTableWidget::mousePressEvent(event);
}

static QRegularExpression title_tags_remover2("(\\&nbsp\\;)*\\<\\/?span( .*?)?>");
static QRegularExpression spanPrefixRemover2("<span style=\"color:.*\">(.*)</span>", QRegularExpression::InvertedGreedinessOption);

void MyTableWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return; // return if not left button down and move
    }
    if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        return; // return if haven't moved far enough with L mouse button down
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    int sourceRow; // = itemAt(dragStartPosition)->row();
    QString sourceInfo;
    int rowNum = 0;

    if (objectName().startsWith("playlist")) {
        // the source is a playlist or track filter --------

        foreach (const QModelIndex &mi, selectionModel()->selectedRows()) {
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

        mimeData->setText(sourceInfo);
        drag->setMimeData(mimeData);

        Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
        Q_UNUSED(dropAction)
    } else {
        // the source is the darkSongTable --------

        foreach (const QModelIndex &mi, selectionModel()->selectedRows()) {
            sourceRow = mi.row();  // this is the actual row number of each selected row, overriding the cursor-located row (just pick all selected rows)
            // qDebug() << "DRAGGING THIS ROW NUMBER:" << sourceRow;

            QString title = dynamic_cast<QLabel*>(this->cellWidget(sourceRow, kTitleCol))->text();
            title.replace(spanPrefixRemover2, "\\1"); // remove <span style="color:#000000"> and </span> title string coloring
            int where = title.indexOf(title_tags_remover2);
            if (where >= 0) {
                title.truncate(where);
            }
            title.replace("&quot;","\"").replace("&amp;","&").replace("&gt;",">").replace("&lt;","<");  // if filename contains HTML encoded chars, put originals back

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

        mimeData->setText(sourceInfo); // send all the info!
        drag->setMimeData(mimeData);

        Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
        Q_UNUSED(dropAction)
    }
}

void MyTableWidget::dragEnterEvent(QDragEnterEvent *event)
{
    // qDebug() << "dragEnterEvent!" << this;
    if (event->mimeData()->hasFormat("text/plain")) {
        // qDebug() << "    event accepted." << event->mimeData()->text() << acceptDrops();
        event->acceptProposedAction();
    }
}

void MyTableWidget::dragMoveEvent(QDragMoveEvent *event)
{
    // qDebug() << "dragMoveEvent: " << event;
    event->acceptProposedAction();
}

void MyTableWidget::dropEvent(QDropEvent *event)
{
    // qDebug() << "***** dropEvent *****";
    // qDebug() << this << " got this text: " << event->mimeData()->text();

    event->acceptProposedAction();

    QStringList rows = event->mimeData()->text().split("!!&!!");

    foreach (const QString &r, rows) {
        // qDebug() << "drop got this: " << r; // we get one per selected row in darkSongTable

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
            sourceRelPath = ((MainWindow *)mw)->relPathInSlot[sourceSlot];
            destRelPath = ((MainWindow *)mw)->relPathInSlot[destSlot];
        }
        bool sourceIsTrackFilter = sourceRelPath.startsWith("/tracks/");
        bool destIsTrackFilter   = destRelPath.startsWith("/tracks/");

        if (sourceName == "darkSongTable") {
            if (destName == "darkSongTable") {
                // NOPE.  We don't allow reordering of the darkSongTable ever.
                // qDebug() << "NO REORDERING OF DARKSONGTABLE";
            } else {
                // from darkSongTable to this playlist, this is feature request #1018
                // qDebug() << "***** DROP:" << sourceName << sourceTrackName << sourcePitch << sourceTempo << sourcePath;

                // (MainWindow*)mw->darkAddPlaylistItemToBottom(whichSlot, sourceTrackName, sourcePitch, sourceTempo, sourcePath, ""); // slot is 0 - 2

                if (destIsTrackFilter) {
                    // qDebug() << "NO DROPPING FROM DARKSONGTABLE TO TRACK FILTERS";
                } else {
                    if (mw != nullptr) {
                        // playlists only, not track filters
                        ((MainWindow *)mw)->darkAddPlaylistItemToBottom(destSlot, sourceTrackName, sourcePitch, sourceTempo, sourcePath, "");
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
                        ((MainWindow *)mw)->darkAddPlaylistItemToBottom(destSlot, sourceTrackName, sourcePitch, sourceTempo, sourcePath, "");
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
                        // qDebug() << "***** DRAG N DROP BETWEEN PLAYLISTS, source:" << sourceName << ", destination:" << destName;
                        if (mw != nullptr) {
                            ((MainWindow *)mw)->darkAddPlaylistItemToBottom(destSlot, sourceTrackName, sourcePitch, sourceTempo, sourcePath, "");
                        } else {
                            qDebug() << "ERROR: mw not valid";
                        }
                    }
                }
            }

        }


    } // foreach selected row in the source table


}

void MyTableWidget::setMainWindow(void *m) {
    mw = m; // save it.  MainWindow will cast it
}
