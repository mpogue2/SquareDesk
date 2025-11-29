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

#ifndef MYTREEWIDGET_H
#define MYTREEWIDGET_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "songdraginfo.h"

// -------------------------------------------------------
class MyTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    MyTreeWidget(QWidget *parent = nullptr);

    // for Drag and Drop
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void setMainWindow(void *m);  // so we can access MainWindow methods
    void setMusicRootPath(const QString &path);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void *mw;
    QString musicRootPath;
    QTreeWidgetItem *dropIndicatorItem; // for drop location highlighting

    // Helper methods
    bool isPlaylistLeafNode(QTreeWidgetItem* item) const;
    bool isAppleMusicPlaylist(QTreeWidgetItem* item) const;
    QString getPlaylistRelativePath(QTreeWidgetItem* item) const;
    QList<SongDragInfo> parseDragData(const QString &mimeText) const;
    void flashItem(QTreeWidgetItem* item);
    void showDropFeedback(const QString &message, const QPoint &globalPos);

signals:
    void statusMessage(const QString &message, int timeout = 3000);
};

#endif // MYTREEWIDGET_H
