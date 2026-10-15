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

#ifndef PLAYLISTPRINTER_H
#define PLAYLISTPRINTER_H

#include <QColor>
#include <QList>
#include <QString>
#include <QStringList>

class QPagedPaintDevice;

// Everything needed to print one palette slot (issue #1804).  It's filled in from what the slot is
//   already showing, at print time, so nothing extra has to be kept in the tables day to day.
struct PlaylistPrintTag {
    QString text;
    QColor  foreground;
    QColor  background;
};

struct PlaylistPrintRow {
    bool    isMarker = false;
    QString number;              // what the slot's # column shows: tip number, line number, or "" for songs in tip mode
    QString title;               // as shown in the slot, e.g. "RS 703 - Girl From Ipanema" or "PLUS - ==== WORKSHOP ===="
    QColor  titleColor;
    QList<PlaylistPrintTag> tags;
    QString levels;              // e.g. "SMP"
    QString pitch;
    QString tempo;
    bool    missing = false;     // file not found, shown struck through in red in the slot
};

struct PlaylistPrintJob {
    QString     name;            // e.g. "Jokers_2026.10.14"
    QStringList folders;         // e.g. {"Playlists", "Jokers", "2026"}
    QString     dateLine;        // e.g. "Wednesday, October 14, 2026"
    bool        tipNumbers = false; // # column holds tip numbers, so head it "TIP" instead of "#"
    QList<PlaylistPrintRow> rows;
};

// Draws the job onto a printer or PDF writer, starting new pages as needed.
void printPlaylist(QPagedPaintDevice *device, const PlaylistPrintJob &job);

// "PLUS - ==== WORKSHOP ====" --> "PLUS  —  WORKSHOP", "SSD - ===============" --> "SSD"
QString cleanPlaylistMarkerTitle(const QString &title);

#endif // PLAYLISTPRINTER_H
