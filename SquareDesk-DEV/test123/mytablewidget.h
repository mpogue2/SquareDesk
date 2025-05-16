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

#ifndef MYTABLEWIDGET_H
#define MYTABLEWIDGET_H

#include <QTableWidget>

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

protected:
    void paintEvent(QPaintEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;

private:
    void *mw;
    int dropIndicatorRow; // for drop location highlighting
    QPoint dragStartPosition;
};

#endif // MYTABLEWIDGET_H
