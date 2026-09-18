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
};

// Returns all tracks from every playlist via the ITLibrary framework.
// On failure, returns an empty vector and sets errorOut.
std::vector<PlaylistTrack> readAllPlaylists(std::string &errorOut);

// The metadata fields of a single song in the Apple Music library, independent of any
//   playlist it may belong to.  Used by Preferences > Apple Music to build the value
//   pickers and the live match count for the square dance filter (issue #1740, item 6).
struct AppleMusicTrackMeta {
    std::string title;
    std::string artist;
    std::string albumArtist;
    std::string album;
    std::string genre;
    std::string grouping;
    std::string composer;
    std::string comments;
    std::string work;
};

// Returns every song in the library that has a local file (movies, podcasts, audiobooks
//   and cloud-only tracks are skipped).  On failure, returns an empty vector and sets errorOut.
std::vector<AppleMusicTrackMeta> readAllLibraryTrackMeta(std::string &errorOut);
