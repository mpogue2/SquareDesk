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
** $SQUAREDESK_BEGIN_LICENSE$
**
****************************************************************************/

#include "mytreewidget.h"
#include "mainwindow.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QTimer>
#include <QDebug>
#include <QLabel>
#include <QCursor>

MyTreeWidget::MyTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setAcceptDrops(true);
    setDropIndicatorShown(false); // We'll draw our own
    mw = nullptr;
    dropIndicatorItem = nullptr;
}

void MyTreeWidget::setMainWindow(void *m) {
    mw = m;
}

void MyTreeWidget::setMusicRootPath(const QString &path) {
    musicRootPath = path;
}

// Helper: Check if item is a playlist leaf node (not a folder, not the "Playlists" parent)
bool MyTreeWidget::isPlaylistLeafNode(QTreeWidgetItem* item) const
{
    if (!item) {
        return false;
    }

    // Must have no children (leaf node)
    if (item->childCount() > 0) {
        return false;
    }

    // Must have a parent (not a top-level item)
    if (!item->parent()) {
        return false;
    }

    // Check if any ancestor is "Playlists"
    QTreeWidgetItem* ancestor = item->parent();
    while (ancestor) {
        if (ancestor->text(0) == "Playlists") {
            return true; // It's under the Playlists hierarchy
        }
        ancestor = ancestor->parent();
    }

    return false;
}

// Helper: Check if item is an Apple Music playlist (read-only)
bool MyTreeWidget::isAppleMusicPlaylist(QTreeWidgetItem* item) const
{
    if (!item) {
        return false;
    }

    // Check if any ancestor is "Apple Music"
    QTreeWidgetItem* ancestor = item;
    while (ancestor) {
        if (ancestor->text(0) == "Apple Music") {
            return true;
        }
        ancestor = ancestor->parent();
    }

    return false;
}

// Helper: Get the relative path to the playlist file from the tree hierarchy
QString MyTreeWidget::getPlaylistRelativePath(QTreeWidgetItem* item) const
{
    if (!item) {
        return QString();
    }

    QStringList pathParts;
    QTreeWidgetItem* current = item;

    // Walk up the tree, collecting text from each level
    while (current && current->parent()) {
        pathParts.prepend(current->text(0));
        current = current->parent();

        // Stop if we hit "Playlists" or "Apple Music"
        if (current && (current->text(0) == "Playlists" || current->text(0) == "Apple Music")) {
            break;
        }
    }

    // Join to create the path
    return pathParts.join("/");
}

// Helper: Parse the MIME data from drag operation
QList<SongDragInfo> MyTreeWidget::parseDragData(const QString &mimeText) const
{
    QList<SongDragInfo> songs;

    // Split by row separator "!!&!!"
    QStringList rows = mimeText.split("!!&!!");

    for (const auto &row : std::as_const(rows)) {
        if (row.isEmpty()) {
            continue;
        }

        // Split by field separator "!#!"
        QStringList fields = row.split("!#!");

        if (fields.size() >= 5) {
            SongDragInfo song;
            song.trackName = fields[0].trimmed();
            song.sourceName = fields[1];
            song.pitch = fields[2];
            song.tempo = fields[3];
            song.path = fields[4];
            songs.append(song);
        }
    }

    return songs;
}

// Helper: Flash the tree item to provide visual feedback
void MyTreeWidget::flashItem(QTreeWidgetItem* item)
{
    if (!item) {
        return;
    }

    // Store original background
    QBrush originalBrush = item->background(0);

    // Set green background
    item->setBackground(0, QBrush(QColor(144, 238, 144))); // Light green

    // Schedule restoration of original background after 500ms
    QTimer::singleShot(500, this, [item, originalBrush]() {
        if (item) {
            item->setBackground(0, originalBrush);
        }
    });
}

// Helper: Show a popup feedback message near the drop location
void MyTreeWidget::showDropFeedback(const QString &message, const QPoint &globalPos)
{
    // Create a temporary label widget
    QLabel* popup = new QLabel(message, nullptr);

    // Style the popup with fully opaque green background
    popup->setStyleSheet(
        "QLabel {"
        "    background-color: rgb(76, 175, 80);"  // Solid green, no transparency
        "    color: white;"
        "    padding: 10px 20px;"
        "    border-radius: 6px;"
        "    border: 2px solid rgb(56, 142, 60);"  // Darker green border
        "    font-weight: bold;"
        "    font-size: 14px;"
        "}"
    );

    // Make it frameless and stay on top
    popup->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    popup->setAttribute(Qt::WA_DeleteOnClose);

    // Adjust size to content
    popup->adjustSize();

    // Position it very close to the drop point, just slightly offset
    QPoint popupPos = globalPos + QPoint(5, 5);
    popup->move(popupPos);

    // Show the popup
    popup->show();

    // Auto-close after 2.5 seconds
    QTimer::singleShot(1500, popup, &QLabel::close);
}

void MyTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    // Check if we have the right MIME type
    if (!event->mimeData()->hasFormat("text/plain")) {
        event->ignore();
        return;
    }

    // Check if the data is not empty
    if (event->mimeData()->text().isEmpty()) {
        event->ignore();
        return;
    }

    // Accept the drag at the widget level - we'll validate the specific
    // drop position in dragMoveEvent and dropEvent
    // This allows the drag to continue even if the mouse enters over
    // a non-droppable item (like the "Playlists" parent node)
    event->acceptProposedAction();
}

void MyTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    // Get the item at the current position
    QTreeWidgetItem* item = itemAt(event->position().toPoint());

    // Check if it's a valid drop target
    if (!isPlaylistLeafNode(item) || isAppleMusicPlaylist(item)) {
        dropIndicatorItem = nullptr;
        viewport()->update();
        event->ignore();
        return;
    }

    // Update drop indicator
    dropIndicatorItem = item;
    viewport()->update();
    event->acceptProposedAction();
}

void MyTreeWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
    // Clear the drop indicator
    dropIndicatorItem = nullptr;
    viewport()->update();
}

void MyTreeWidget::dropEvent(QDropEvent *event)
{
    // Get the target item
    QTreeWidgetItem* targetItem = itemAt(event->position().toPoint());

    // Clear drop indicator
    dropIndicatorItem = nullptr;
    viewport()->update();

    // Always accept the event to properly complete the drag operation
    // This prevents the drag from getting stuck in a weird state
    event->setDropAction(Qt::IgnoreAction);
    event->accept();

    // Validate target
    if (!isPlaylistLeafNode(targetItem) || isAppleMusicPlaylist(targetItem)) {
        return;
    }

    // Get the relative path to the playlist file
    QString playlistRelPath = getPlaylistRelativePath(targetItem);
    if (playlistRelPath.isEmpty()) {
        qDebug() << "ERROR: Could not determine playlist path for item:" << targetItem->text(0);
        return;
    }

    // Parse the drag data
    QList<SongDragInfo> songs = parseDragData(event->mimeData()->text());
    if (songs.isEmpty()) {
        qDebug() << "ERROR: No valid songs in drag data";
        return;
    }

    // Call MainWindow method to add the songs to the playlist file
    if (mw != nullptr) {
        MainWindow* mainWindow = static_cast<MainWindow*>(mw);

        // Add each song to the playlist
        bool success = mainWindow->addItemsToPlaylistFile(playlistRelPath, songs);

        if (success) {
            // Set the action to CopyAction for successful drops
            event->setDropAction(Qt::CopyAction);

            // Flash the tree item for visual feedback
            flashItem(targetItem);

            // Show popup feedback near the cursor
            QString playlistName = targetItem->text(0);
            QString message = QString("Added %1 song%2 to '%3'")
                .arg(songs.count())
                .arg(songs.count() == 1 ? "" : "s")
                .arg(playlistName);

            // Get the global cursor position for the popup
            QPoint globalPos = QCursor::pos();
            showDropFeedback(message, globalPos);

            // Also show in status bar for those who prefer it
            emit statusMessage(message, 3000);
        } else {
            qDebug() << "ERROR: Failed to add songs to playlist";
        }
    } else {
        qDebug() << "ERROR: MainWindow pointer not set";
    }
}

void MyTreeWidget::paintEvent(QPaintEvent *event)
{
    QTreeWidget::paintEvent(event);

    // Draw drop indicator if needed
    if (dropIndicatorItem) {
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Get the rectangle for this item
        QRect itemRect = visualItemRect(dropIndicatorItem);

        // Draw a green border around the item
        QPen pen(Qt::green, 2);
        painter.setPen(pen);
        painter.drawRect(itemRect.adjusted(1, 1, -1, -1));
    }
}
