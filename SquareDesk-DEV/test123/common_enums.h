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

#ifndef COMMON_ENUMS_H
#define COMMON_ENUMS_H

enum SongFilenameMatchingType {
    SongFilenameLabelDashName = 1,
    SongFilenameNameDashLabel,
    SongFilenameBestGuess
};

enum SessionDefaultType {
    SessionDefaultPractice = 1,
    SessionDefaultDOW
};

enum ColumnExportData
{
    ExportDataFileName = 0,
    ExportDataPitch,
    ExportDataTempo,
    ExportDataIntro,
    ExportDataOutro,
    ExportDataVolume,
    ExportDataCuesheetPath,
    ExportDataNone
};

enum CheckerColorScheme
{
    CheckerColorSchemeNormal = 0,
    CheckerColorSchemeColorOnly,
    CheckerColorSchemeMentalImage,
    CheckerColorSchemeSight
};

enum AnimationSpeed
{
    AnimationSpeedOff = 0,
    AnimationSpeedSlow,
    AnimationSpeedMedium,
    AnimationSpeedFast,
};

enum Gender
{
    Boys = 0,
    Girls,
    UnknownGender
};

enum Order
{
    InOrder = 0,
    OutOfOrder,
    UnknownOrder
};

// Table column indices for playlist tables
constexpr int COLUMN_NUMBER = 0;
constexpr int COLUMN_TITLE = 1;
constexpr int COLUMN_PITCH = 2;
constexpr int COLUMN_TEMPO = 3;
constexpr int COLUMN_PATH = 4;
constexpr int COLUMN_LOADED = 5;

// Playlist slot numbers
constexpr int MAX_PLAYLIST_SLOTS = 3;
constexpr int SLOT_1 = 0;
constexpr int SLOT_2 = 1;
constexpr int SLOT_3 = 2;

// Playlist management constants
constexpr int MAX_RECENT_PLAYLISTS = 5;
constexpr int APPLE_SYMBOL_UNICODE = 0xF8FF;

// Playlist marker indentation (issue #1547)
// Number of non-breaking spaces to indent items following a marker row
constexpr int PLAYLIST_INDENT_SPACES = 4;

// Icon dimensions
constexpr int TRACK_ICON_WIDTH = 15;
constexpr int TRACK_ICON_HEIGHT = 15;
constexpr int APPLE_MUSIC_ICON_WIDTH = 12;
constexpr int APPLE_MUSIC_ICON_HEIGHT = 12;
constexpr int PLAYLIST_ICON_WIDTH = 10;
constexpr int PLAYLIST_ICON_HEIGHT = 9;

// Note: QString constants moved to playlist_constants.h to avoid Qt header dependency issues

// Filename parsing constraints
constexpr int MAX_LABEL_LENGTH = 20;
constexpr int MAX_LABEL_NUMBER_LENGTH = 5;
constexpr int MAX_LABEL_EXTRA_LENGTH = 4;

#endif // ifndef COMMON_ENUMS_H
