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

#include "mainwindow.h"

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
#endif

    return true; // we did it!
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

#endif
    return true; // we did it!
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

#endif
    return true;  // we did it!
}

// -----------------------------------------------
bool MyTableWidget::moveSelectedItemToBottom(bool scrollWhenDone) { // defaults to scrollWhenDone == true
#ifdef DARKMODE
    QModelIndexList list = selectionModel()->selectedRows();

    if (list.count() != 1) {
        // if it's zero or >= 2 return
        return false; // we did nothing, this was not meant for us
    }

    int row = list.at(0).row(); // only 1 row can ever be selected, this is its number 0 - N-1

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

#else
    Q_UNUSED(scrollWhenDone)
#endif
    return true; // we did it!
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

    if (moveSelectedItemToBottom(false)) { // do NOT scroll to the bottom

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

        return true;
    }
#endif
    return false; // not meant for us
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

    if (objectName().startsWith("playlist")) {
        return;  // no drag for you!  (yet)
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    int sourceRow = itemAt(dragStartPosition)->row();

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

    mimeData->setText(sourceTrackName + "                                    !#!" + // the spaces are so that the visible drag text is just the track name (hack!)
                      sourceName + "!#!" +
                      sourcePitch + "!#!" +
                      sourceTempo + "!#!" +
                      sourcePath ); // send all the info!
    drag->setMimeData(mimeData);

    Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
    Q_UNUSED(dropAction)
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
    // qDebug() << "dropEvent!";
    // qDebug() << this << " got this text: " << event->mimeData()->text();
    event->acceptProposedAction();

    QStringList fields = event->mimeData()->text().split("!#!");
    QString sourceTrackName = fields[0].trimmed();
    QString sourceName      = fields[1];
    QString sourcePitch     = fields[2];
    QString sourceTempo     = fields[3];
    QString sourcePath      = fields[4];

    QString destName = objectName();

    int whichSlot = 0;

    if (sourceName == "darkSongTable") {
        if (destName == "darkSongTable") {
            // ignore for now
        } else {
            // from darkSongTable to this playlist
            if (destName == "playlist1Table") {
                whichSlot = 0;
            } else if (destName == "playlist2Table") {
                whichSlot = 1;
            } else if (destName == "playlist3Table") {
                whichSlot = 2;
            } else {
                qDebug() << "SLOT ERROR";
            }

            // qDebug() << "***** DROP:" << sourceName << sourceTrackName << sourcePitch << sourceTempo << sourcePath;

            // (MainWindow*)mw->darkAddPlaylistItemToBottom(whichSlot, sourceTrackName, sourcePitch, sourceTempo, sourcePath, ""); // slot is 0 - 2

            if (mw != nullptr) {
                ((MainWindow *)mw)->darkAddPlaylistItemToBottom(whichSlot, sourceTrackName, sourcePitch, sourceTempo, sourcePath, "");
            }
        }
    } else {
        qDebug() << "dragging to darkSongTable is NOT IMPLEMENTED YET.";
    }

}

void MyTableWidget::setMainWindow(void *m) {
    mw = m; // save it.  MainWindow will cast it
}
