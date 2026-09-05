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

#ifndef PLAYLISTNUMBERDELEGATE_H
#define PLAYLISTNUMBERDELEGATE_H

#include <QStyledItemDelegate>

// The # column of a playlist slot always holds the true sequential line number as its text,
//   because MyTableWidget's move/reorder code swaps those numbers and then sortItems(0)'s on
//   them.  When tip numbering is enabled, MainWindow::updatePlaylistTipNumbers() stashes what
//   the user should actually SEE in this role, and this delegate paints that instead.  When the
//   role is not set (pref off, or playlist has no markers), the line number is painted as usual.
//   (issue #1714)
constexpr int PLAYLIST_TIPNUMBER_ROLE = Qt::UserRole + 17;

class PlaylistNumberDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit PlaylistNumberDelegate(QObject *parent = nullptr);

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

#endif // PLAYLISTNUMBERDELEGATE_H
