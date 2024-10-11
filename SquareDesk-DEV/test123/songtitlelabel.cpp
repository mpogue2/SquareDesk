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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "songtitlelabel.h"
#include "songlistmodel.h"

void SongTitleLabel::mouseDoubleClickEvent(QMouseEvent *e)
{
    mw->titleLabelDoubleClicked(e);
}

#ifdef DARKMODE
// ===============================================================
static QRegularExpression title_tags_remover3("(\\&nbsp\\;)*\\<\\/?span( .*?)?>");
static QRegularExpression spanPrefixRemover3("<span style=\"color:.*\">(.*)</span>", QRegularExpression::InvertedGreedinessOption);

void darkSongTitleLabel::mouseDoubleClickEvent(QMouseEvent *e)
{
    mw->darkTitleLabelDoubleClicked(e);
}

void darkSongTitleLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragStartPosition = event->pos();
    }
    QLabel::mousePressEvent(event);
}

void darkSongTitleLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return; // return if not left button down and move
    }
    if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        return; // return if haven't moved far enough with L mouse button down
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    // int sourceRow; // = itemAt(dragStartPosition)->row();
    QString sourceInfo;
    // int rowNum = 0;

    // qDebug() << "THIS:" << this << this->parent()->parent();
    // parent is viewport, parent/parent is the QTableWidget
    // QPoint pos = this->pos();
    QTableWidget *theTable = qobject_cast<QTableWidget *>(this->parent()->parent());
    // int row = theTable->indexAt(pos).row()
    // qDebug() << "INDEX AT ROW: " << row;

    // the source is the darkSongTable --------
    int sourceRow;
    int rowNum = 0;
    foreach (const QModelIndex &mi, theTable->selectionModel()->selectedRows()) {
        sourceRow = mi.row();  // this is the actual row number of each selected row, overriding the cursor-located row (just pick all selected rows)
        // qDebug() << "***** darkSongTitleLabel: DRAGGING THIS ROW NUMBER:" << sourceRow;

        QString title = text();
        title.replace(spanPrefixRemover3, "\\1"); // remove <span style="color:#000000"> and </span> title string coloring
        int where = title.indexOf(title_tags_remover3);
        if (where >= 0) {
            title.truncate(where);
        }
        title.replace("&quot;","\"").replace("&amp;","&").replace("&gt;",">").replace("&lt;","<");  // if filename contains HTML encoded chars, put originals back

        if (theTable->isRowHidden(sourceRow)) {
            // don't allow drag and drop for rows that are not visible!  This only is a problem for darkSongTable, which may have filters applied.
            // qDebug() << "no drag and drop for you: " << sourceRow << title;
            continue;
        } else {
            // qDebug() << "drag and drop is OK for you: " << sourceRow << title;
        }

        QString sourceName = "darkSongTable";

        QString sourceTrackName = theTable->item(sourceRow, kLabelCol)->text() + " - " + title; // e.g. "ESP 1234 - Ricochet"
        QString sourcePitch = theTable->item(sourceRow, kPitchCol)->text();
        QString sourceTempo = theTable->item(sourceRow, kTempoCol)->text();
        QString sourcePath = theTable->item(sourceRow, kPathCol)->data(Qt::UserRole).toString();

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


// ===============================================================
void darkPaletteSongTitleLabel::mouseDoubleClickEvent(QMouseEvent *e)
{
    // tell the tableWidget that this item was double-clicked (it should be selected)
    // mtw->darkPaletteTitleLabelDoubleClicked(e);
    mw->darkPaletteTitleLabelDoubleClicked(e);
}

void darkPaletteSongTitleLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragStartPosition = event->pos();
    }
    QLabel::mousePressEvent(event);
}

void darkPaletteSongTitleLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return; // return if not left button down and move
    }
    if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        return; // return if haven't moved far enough with L mouse button down
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    // int sourceRow; // = itemAt(dragStartPosition)->row();
    QString sourceInfo;
    // int rowNum = 0;

    // qDebug() << "THIS:" << this << this->parent()->parent();
    // parent is viewport, parent/parent is the QTableWidget
    // QPoint pos = this->pos();
    QTableWidget *theTable = qobject_cast<QTableWidget *>(this->parent()->parent());
    // int row = theTable->indexAt(pos).row()
    // qDebug() << "INDEX AT ROW: " << row;

    // the source is the darkSongTable --------
    int sourceRow;
    int rowNum = 0;
    foreach (const QModelIndex &mi, theTable->selectionModel()->selectedRows()) {
        sourceRow = mi.row();  // this is the actual row number of each selected row, overriding the cursor-located row (just pick all selected rows)
        // qDebug() << "***** darkSongTitleLabel: DRAGGING THIS ROW NUMBER:" << sourceRow;

        QString title = text();
        title.replace(spanPrefixRemover3, "\\1"); // remove <span style="color:#000000"> and </span> title string coloring
        int where = title.indexOf(title_tags_remover3);
        if (where >= 0) {
            title.truncate(where);
        }
        title.replace("&quot;","\"").replace("&amp;","&").replace("&gt;",">").replace("&lt;","<");  // if filename contains HTML encoded chars, put originals back

        QString sourceName = theTable->objectName();

        QString sourceTrackName = title; // e.g. "ESP 1234 - Ricochet"
        QString sourcePitch = theTable->item(sourceRow, 2)->text();
        QString sourceTempo = theTable->item(sourceRow, 3)->text();
        QString sourcePath = theTable->item(sourceRow, 4)->text();

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

#define ENABLESTRIKETHROUGH 0

// true = song was used recently (Recent == "*")
void darkSongTitleLabel::setSongUsed(bool b) {
#if ENABLESTRIKETHROUGH==1
    // qDebug() << "setSongUsed: current text =" << this->text() << songUsed << b;

    // QString strikethrough("text-decoration:line-through;background-color:#00FF00;");
    QString strikethrough("text-decoration:line-through;text-decoration-color: #00FF00;");

    if (songUsed & !b) {
        // turning off used marker
        // typical text: <span style="color: #a364dc;text-decoration:line-through;background-color:#00FF00;">Gobs Of Guitars</span>
        // typical text with tags: <span style="color: #a364dc;text-decoration:line-through;background-color:#00FF00;">Tango Burlesque</span>&nbsp;&nbsp;<span style="background-color:#ffffff; color: #c64434;"> NEW </span>
        QString currentText = this->text();
        currentText.replace(strikethrough, "");
        this->setText(currentText); // revised text, WITHOUT strikethrough
    } else if (!songUsed & b) {
        // turning on used marker
        // typical text: <span style="color: #a364dc;">Gobs Of Guitars</span>
        // typical text with tags: <span style="color: #a364dc;">Tango Burlesque</span>&nbsp;&nbsp;<span style="background-color:#ffffff; color: #c64434;"> NEW </span>
        QString currentText = this->text();
        int endOfFirstTag = currentText.indexOf("\">"); // end of the title's span
        if (endOfFirstTag != -1) {
            currentText.insert(endOfFirstTag, strikethrough);
        }
        this->setText(currentText); // revised text, WITH strikethrough
    }
#endif
    songUsed = b;
}

// true = song was used recently (Recent == "*")
void darkPaletteSongTitleLabel::setSongUsed(bool b) {
#if ENABLESTRIKETHROUGH==1
    // qDebug() << "PALETTE setSongUsed: current text =" << text() << songUsed << b;

    // QString strikethrough("text-decoration:line-through;background-color:#00FF00;");
    QString strikethrough("text-decoration:line-through;text-decoration-color: #00FF00;text-decoration-style: wavy;");

    if (songUsed & !b) {
        // turning off used marker
        // typical text: <span style="color: #a364dc;text-decoration:line-through;background-color:#00FF00;">Gobs Of Guitars</span>
        // typical text with tags: <span style="color: #a364dc;text-decoration:line-through;background-color:#00FF00;">Tango Burlesque</span>&nbsp;&nbsp;<span style="background-color:#ffffff; color: #c64434;"> NEW </span>
        QString currentText = text();
        currentText.replace(strikethrough, "");
        setText(currentText); // revised text, WITHOUT strikethrough
    } else if (!songUsed & b) {
        // turning on used marker
        // typical text: <span style="color: #a364dc;">Gobs Of Guitars</span>
        // typical text with tags: <span style="color: #a364dc;">Tango Burlesque</span>&nbsp;&nbsp;<span style="background-color:#ffffff; color: #c64434;"> NEW </span>
        QString currentText = text();
        int endOfFirstTag = currentText.indexOf("\">"); // end of the title's span
        if (endOfFirstTag != -1) {
            currentText.insert(endOfFirstTag, strikethrough);
        }
        setText(currentText); // revised text, WITH strikethrough
    }
#endif
    songUsed = b;
}


#endif
