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

// Everything that Preferences > Apple Music decides about a track: whether it is square dance
//   music at all, and what its Type is (issue #1740, item 6).
//
// This lives on its own so that the Preferences dialog and the importer run the SAME code.
//   The dialog fills an AppleMusicFilter in from its widgets, so its live preview shows what
//   the import will actually do; getAppleMusicInfo() fills one in from the saved preferences.
//   If these two ever diverge, the preview becomes a lie, which is worse than no preview.

#ifndef APPLEMUSICFILTER_H
#define APPLEMUSICFILTER_H

#include <QList>
#include <QString>
#include <QStringList>

#include "mainwindow_applemusic.h"

class PreferencesManager;

struct AppleMusicNamedItem {
    const char *key;
    const char *display;
};

// The string fields ITLibrary actually exposes.  Order matters: it is the order of the items in
//   appleMusicTypeFieldCombo (after its leading "Nothing" item) and of each rule row's field
//   pulldown, both of which are built from this table.
extern const AppleMusicNamedItem appleMusicFields[];
extern const int numAppleMusicFields;

extern const AppleMusicNamedItem appleMusicOperators[];
extern const int numAppleMusicOperators;

QString appleMusicFieldValue(const AppleMusicTrackMeta &track, const QString &fieldKey);
QString appleMusicFieldDisplay(const QString &fieldKey);
bool    appleMusicRuleMatches(const QString &value, const QString &opKey, const QString &arg);
bool    appleMusicValueInList(const QString &value, const QString &semicolonList);
QString appleMusicCollapseSemicolons(QString text);

// The metadata of one track of a playlist, in the shape the rules are written against.
AppleMusicTrackMeta appleMusicMetaOf(const PlaylistTrack &track);

// Could SquareDesk play this file at all?  An Apple Music library is full of things it can't:
//   .m4p (FairPlay DRM), .aiff, video.  The importer and the Preferences preview have to agree
//   about this, or the preview promises songs that never arrive (issue #1740).
bool appleMusicIsPlayableFormat(const QString &absolutePath);

// appleMusicTypeColumnFormat
enum AppleMusicTypeColumnFormat {
    AppleMusicTypeThenPlaylist = 0,   // "patter <APPLE> Sunday Night 014"
    AppleMusicPlaylistOnly     = 1,   // "<APPLE> Sunday Night 014"
    AppleMusicTypeOnly         = 2,   // "patter"
};

// appleMusicTypeDefault
enum AppleMusicDefaultType {
    AppleMusicDefaultPatter  = 0,
    AppleMusicDefaultSinging = 1,
    AppleMusicDefaultCalled  = 2,
    AppleMusicDefaultExtras  = 3,
    AppleMusicDefaultSkip    = 4,     // "Don't import"
};

struct AppleMusicRule {
    QString fieldKey;
    QString opKey;
    QString value;
};

// A track's record label, split the way the song table's Label column wants it: the column
//   shows label + " " + labelnum (issue #1747).
struct AppleMusicLabel {
    QString label;
    QString labelnum;
    QString labelnum_extra;

    bool isEmpty() const { return label.isEmpty() && labelnum.isEmpty(); }
};

class AppleMusicFilter
{
public:
    void loadFrom(PreferencesManager &prefs);

    // Is this track square dance music at all?
    bool passes(const AppleMusicTrackMeta &track) const;

    // "patter", "singing", "called", "extras", or "" meaning don't import this one.
    QString typeOf(const AppleMusicTrackMeta &track) const;

    bool typeComesFromMetadata() const { return !typeFieldKey.isEmpty(); }

    // The label this track's chosen metadata field says it is.  Empty when no Label field has
    //   been chosen, or when this track's copy of that field is empty -- in both cases the
    //   caller falls back to parsing the filename, exactly as a Music Directory song does
    //   (issue #1747).
    AppleMusicLabel labelOf(const AppleMusicTrackMeta &track) const;

    bool labelComesFromMetadata() const { return !labelFieldKey.isEmpty(); }

    static QList<AppleMusicRule> rulesFromJSON(const QString &json);
    static QString rulesToJSON(const QList<AppleMusicRule> &rules);

    // which tracks are square dance music
    bool filterEnabled = false;
    bool matchAll = true;                 // false = match any rule
    bool appliesInsidePlaylists = true;
    QList<AppleMusicRule> rules;

    // how to determine a track's Type
    QString typeFieldKey;                 // "" = no Type field chosen
    QString typePatter;
    QString typeSinging;
    QString typeCalled;
    QString typeExtras;
    int typeDefault = AppleMusicDefaultExtras;
    int typeColumnFormat = AppleMusicTypeThenPlaylist;

    // which metadata field holds the record label, e.g. "album" for someone who keeps "HH-1234"
    //   in the Album field.  "" = read the label out of the filename instead (issue #1747).
    QString labelFieldKey;
};

#endif // APPLEMUSICFILTER_H
