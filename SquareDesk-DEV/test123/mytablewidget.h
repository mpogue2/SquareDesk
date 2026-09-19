/****************************************************************************
**
** Copyright (C) 2016-2026 Mike Pogue, Dan Lyke
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

#ifndef MYTABLEWIDGET_H
#define MYTABLEWIDGET_H

#include <QTableWidget>
#include <QSet>

class QMenu;
class QToolButton;
#include <QQueue>

struct sortOperation {
    int column;
    Qt::SortOrder sortOrder;

    sortOperation(int c, Qt::SortOrder so) : column(c), sortOrder(so) {};
    sortOperation(QString s) {
        // e.g. "c:2,so:0"
        QStringList sl = s.split(",");
        QStringList c1 = sl[0].split(":");
        QStringList so1 = sl[1].split(":");
        column = c1[1].toInt();
        sortOrder = static_cast<Qt::SortOrder>(so1[1].toInt());
    };

    QString toString() const {
        return QString("c:%1,so:%2").arg(column).arg(sortOrder); // 0 = ascending, 1 = descending
    }
};

// -------------------------------------------------------
class MyTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    MyTableWidget(QWidget *parent = nullptr);
    bool isEditing();

    bool moveSelectedItemsUp();
    bool moveSelectedItemsDown();
    bool moveSelectedItemsToTop();
    bool moveSelectedItemsToBottom(bool scrollWhenDone = true);
    bool removeSelectedItems();

    void moveSelectionUp();
    void moveSelectionDown();
    void moveSelectionToTop();
    void moveSelectionToBottom();

    QString fullPathOfSelectedSong();

    // for Drag and Drop
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void setMainWindow(void *m);  // so we can do stuff

    // void sortItems(int column, Qt::SortOrder order = Qt::AscendingOrder); // override
    void setOrderFromString(QString s);
    void initializeSortOrder();

    // Columns that must not take part in sorting, e.g. darkSongTable's Audition column, whose
    //   items have no text at all (issue #1740).
    void setColumnNotSortable(int column);

    // Puts a button in this table's top-left header section -- the one belonging to a column
    //   made non-sortable above -- which pops up the given menu.  Used for the song table's
    //   column-visibility menu, which is View > Columns itself rather than a copy of it.
    void setCornerMenu(QMenu *menu);

    // Nominates one column as this table's "slack" column: whenever the viewport width changes,
    //   that column grows or shrinks to swallow whatever space the other columns don't use, down
    //   to minimumWidth, after which the horizontal scrollbar takes over.  Every other column
    //   keeps the width the user gave it.
    //
    // This is deliberately NOT QHeaderView::Stretch.  Stretch reacts to any section resize, a
    //   divider drag included, which is what made most of darkSongTable's dividers misbehave --
    //   Title absorbed the drag and the column slid sideways instead of resizing (issue #1744).
    //   Keying off the VIEWPORT width instead means a drag can never trigger it, because dragging
    //   a divider doesn't change the viewport.
    //
    // Opt-in, because MyTableWidget also backs the three playlist tables, which don't want it.
    void setSlackColumn(int column, int minimumWidth = 150);
    void updateSlackColumn();   // also call after anything that changes column widths or visibility

protected:
    void paintEvent(QPaintEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void *mw;
    int dropIndicatorRow; // for drop location highlighting
    QPoint dragStartPosition;
    
    // Helper method to determine if this table is a writable drop target
    bool isWritableTarget() const;

    int maxSortOperations = 7;
    QList<sortOperation> sortOperations;

signals:
    void newStableSort(QString sortString);

public slots:
    void onHeaderClicked(int column);

private:
    QSet<int> notSortableColumns;
    QMenu *cornerMenu = nullptr;
    QToolButton *cornerMenuButton = nullptr;
    int cornerMenuColumn = -1;
    void positionCornerMenuButton();

    int slackColumn = -1;           // -1 == no slack column, i.e. feature off
    int slackColumnMinimumWidth = 150;
    bool inSlackColumnResize = false;
};

#endif // MYTABLEWIDGET_H
