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


#ifndef PLAYLISTLEVELSDELEGATE_H
#define PLAYLISTLEVELSDELEGATE_H

#include <QStyledItemDelegate>

// The Levels column of a palette slot holds a string like "SMP": every level that has a cuesheet
//   for that song.  MainWindow::updatePlaylistCurrentLevels() stashes the ONE letter whose cuesheet
//   will actually load when the song is double-clicked in PLAYLIST_CURRENTLEVEL_ROLE, and the color
//   to draw it in (the singing call color) in PLAYLIST_CURRENTLEVEL_COLOR_ROLE.  This delegate paints
//   that letter in that color, and the rest of the string normally.  The item's text is untouched,
//   so sorting, saving and printing see the same "SMP" as always. (issue #1803)
constexpr int PLAYLIST_CURRENTLEVEL_ROLE       = Qt::UserRole + 18;
constexpr int PLAYLIST_CURRENTLEVEL_COLOR_ROLE = Qt::UserRole + 19;

class PlaylistLevelsDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit PlaylistLevelsDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // PLAYLISTLEVELSDELEGATE_H
