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

#ifndef PLAYLIST_CONSTANTS_H
#define PLAYLIST_CONSTANTS_H

#include <QString>

// Default color values
const QString DEFAULT_UNRECOGNIZED_TYPE_COLOR = "#808080";

// File path constants
const QString TRACKS_PATH_PREFIX = "/tracks/";
const QString PLAYLISTS_PATH_PREFIX = "/playlists/";
const QString APPLE_MUSIC_PATH_PREFIX = "/Apple Music/";
const QString CSV_FILE_EXTENSION = ".csv";

// CSV and audio file constants
const QString CSV_HEADER_STRING = "relpath,pitch,tempo";
const QString SUPPORTED_AUDIO_EXTENSIONS_REGEX = "\\.(mp3|m4a|wav|flac)$";

// Default values for playlist items
const QString DEFAULT_PITCH_VALUE = "0";
const QString DEFAULT_TEMPO_VALUE = "0";
const QString LOADED_INDICATOR_STRING = "1";

// Path stack separators
const QString PLAYLIST_ENTRY_SEPARATOR = "%!%";
const QString PATH_SEPARATOR = "#!#";
const QString APPLE_MUSIC_SEPARATOR = "$!$";

#endif // PLAYLIST_CONSTANTS_H