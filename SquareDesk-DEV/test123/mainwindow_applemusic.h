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

#pragma once
#include <string>
#include <vector>

struct PlaylistTrack {
    std::string playlistType;   // "smart" or "static"
    std::string playlistName;
    int         itemNumber;
    std::string absolutePath;
    std::string title;
    std::string artist;
    std::string composer;
    std::string genre;
    int         beatsPerMinute; // 0 = not set
    int         rating;         // 0 = not set, otherwise 20/40/60/80/100
    int         year;           // 0 = not set
    std::string grouping;
    std::string work;
    std::string modifiedDate;   // ISO 8601, empty if not set
    std::string album;          // the fields below are here for the square dance filter and the
    std::string albumArtist;    //   Type mapping in Preferences > Apple Music (issue #1740)
    std::string comments;
    int         totalTimeMS;    // 0 = not set
};

// Returns all tracks from every playlist via the ITLibrary framework.
// On failure, returns an empty vector and sets errorOut.
std::vector<PlaylistTrack> readAllPlaylists(std::string &errorOut);

// The metadata fields of a single song, independent of which playlist it came from.  This is
//   what the square dance filter and the Type mapping in Preferences > Apple Music are written
//   against, and what their live preview lists (issue #1740, item 6).
struct AppleMusicTrackMeta {
    std::string absolutePath;
    std::string title;
    std::string artist;
    std::string albumArtist;
    std::string album;
    std::string genre;
    std::string grouping;
    std::string composer;
    std::string comments;
    std::string work;

    // Display-only, for the song table's Apple Music columns.  Deliberately NOT filterable:
    //   appleMusicFields[] is the filterable set, and it is all strings, so one operator list
    //   covers every field (issue #1740, item 2).
    int year;                   // 0 = not set
    int totalTimeMS;            // 0 = not set
};
