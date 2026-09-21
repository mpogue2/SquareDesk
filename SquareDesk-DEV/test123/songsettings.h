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

#ifndef SONGSETTINGS_H_INCLUDED
#define SONGSETTINGS_H_INCLUDED
#include <QtSql/QSqlDatabase>
#include <QtSql>
#include <vector>

class SessionInfo;

class SongPlayEvent {
public:
    virtual void operator() (const QString &name,
                             const QString &playedOnUTC,
                             const QString &playedOnLocal,
                             const QString &playedOnFilename,
                             const QString &playedOnPitch,
                             const QString &playedOnTempo,
                             const QString &playedOnLastCuesheet
                             ) = 0;
    virtual ~SongPlayEvent(){}
};


// One song's duration, as read out of the audio file itself (issue #1753).
//   Reading it is expensive for an MP3 with no Xing header -- the whole file has to be read to
//   count its frames -- so the answer is remembered in the DB rather than worked out again every
//   time SquareDesk starts.  mtimeMS and fileSize are what make that safe: if either has changed,
//   the song on disk is not the one we measured, and the stored duration is ignored.
class SongDuration
{
public:
    qint64 durationMS = 0;
    qint64 mtimeMS    = 0;
    qint64 fileSize   = 0;
};

class SongSetting
{
    // Attributes are set in songsetting_attributes.h
#define SONGSETTING_ELEMENT(type, name) private:                \
    type m_##name;                                              \
    bool set_##name = false;                                    \
    public:                                                     \
    type get##name() const { return m_##name; }                 \
    void set##name(type p) { m_##name = p; set_##name = true; } \
    bool isSet##name() const { return set_##name; }
#include "songsetting_attributes.h"
#undef SONGSETTING_ELEMENT

private :
    // A little wasteful, but I need something to terminate the comma list in the constructor
    // with the preprocessor tricks I'm playing...
    bool dummy;
public:
    SongSetting();
    friend QDebug operator<<(QDebug dbg, const SongSetting &setting);  // DEBUG
};


class TableDefinition;
class IndexDefinition;

class SongSettings
{
public:
    SongSettings();
    void openDatabase(const QString &path,
                      const QString &mainRootDir,
                      bool in_memory);
    void closeDatabase();
    void saveSettings(const QString &filenameWithPath,
                      const SongSetting &settings);
    bool loadSettings(const QString &filenameWithPath,
                      SongSetting &settings);
    void loadSettingsForAllSongs(QHash<QString, SongSetting> &settingsByFilename);

    // Song durations (issue #1753), keyed by the music-root-relative path, like the songs and
    //   cuesheets tables.  Loaded in one query, and written back in one transaction.
    void loadSongDurations(QHash<QString, SongDuration> &durationsByFilename);
    void saveSongDurations(const QHash<QString, SongDuration> &durationsByFilename);

    void setCurrentSession(int id) { current_session_id = id; }
    int getCurrentSession() { return current_session_id; }
    void getSongAges(QHash<QString,QString> &ages, bool show_all_sessions);
    QString getSongAge(const QString &filename, const QString &filenameWithPath, bool show_all_sessions);
    void markSongPlayed(const QString &filename, const QString &filenameWithPath);

    // #1745: record plays that happened outside SquareDesk (Apple Music's lastPlayedDate), so the
    //   Age and Recent columns can see them.  Keyed by absolute path -> ISO 8601 last-played date.
    //   Idempotent: a play already recorded at that exact instant for that song is skipped, so
    //   this is safe to call on every resync.  Returns the number of plays inserted.
    int importExternalPlays(const QHash<QString, QString> &lastPlayedByPath, const QString &origin);

    QString removeRootDirs(const QString &filenameWithPath);
    QString primaryRootDir();

    // ---- Apple Music keying (issue #1747) ----
    // The songs.filename key for a song.  For a song in the Music Directory this is its path
    //   relative to the music root, exactly as removeRootDirs() gives it.  For an Apple Music
    //   track it is Music's own persistentID instead, because the track's PATH is not stable:
    //   Music.app renames the file when the Title is edited, and moves it when the library is
    //   reorganized, either of which silently orphans every setting, play and tag for that song.
    QString songKeyFor(const QString &filenameWithPath);

    // Hands over the absolute path -> persistentID map for the current Apple Music library, as
    //   read by getAppleMusicInfo().  Must be called before songKeyFor() can recognize an Apple
    //   Music path; until then those tracks simply key by path, as they used to.
    void setAppleMusicPersistentIDs(const QHash<QString, QString> &idByAbsolutePath);

    // Moves any songs row still keyed by an Apple Music track's absolute path over to that
    //   track's persistentID key, so existing settings survive the change.  Re-runnable: it only
    //   moves a row when there is no row at the persistentID key already, so a second run finds
    //   nothing to do.  Returns the number of rows moved.
    int migrateAppleMusicKeys();

    QString getCallTaughtOn(const QString &program, const QString &call_name);
    void setCallTaught(const QString &program, const QString &call_name);
    void deleteCallTaught(const QString &program, const QString &call_name);
    void clearTaughtCalls(const QString &program);
    int currentSessionIDByTime();

    QList<SessionInfo> getSessionInfo();
    void setSessionInfo(const QList<SessionInfo> &sessions);
    void setTagColors( const QHash<QString,QPair<QString,QString>> &);
    QHash<QString,QPair<QString,QString>> getTagColors(bool loadCache = true);

    QPair<QString,QString> getColorForTag(const QString &tag);
    void addTags(const QString &str);
    void removeTags(const QString &str);
    void setDefaultTagColors( const QString &background, const QString & foreground);

    void getSongPlayHistory(SongPlayEvent &event,
                            int session_id,
                            bool omitStartDate,
                            QString startDate,
                            bool omitEndDate,
                            QString endDate);

    void getSongMarkers(const QString &filename, QMap<float,int> &markers);  // get all markers associated with a song from DB
    void setSongMarkers(const QString &filename, const QMap<float,int> &markers);  // set markers associated with a song in DB

    void addMarker(const float markerPos, QMap<float,int> &markers);  // add a new marker position to a marker set
    float getNearbyMarker(const float markerPos, QMap<float,int> &markers);
    void deleteNearbyMarker(const float markerPos, QMap<float,int> &markers);  // delete a marker position from a marker set

    // per-cuesheet display settings (#1682); filenameWithPath is the ABSOLUTE path of the cuesheet
    int getCuesheetFontOffset(const QString &filenameWithPath);                // 0, if never customized
    void setCuesheetFontOffset(const QString &filenameWithPath, int offset);   // 0 deletes the row

    // per-cuesheet auto-scroll override (#1724).  Tri-state, because most cuesheets should keep
    //   following the global default in Preferences; only overridden cuesheets get a stored value.
    int getCuesheetAutoScroll(const QString &filenameWithPath);                // -1 follow default, 0 never, 1 always
    void setCuesheetAutoScroll(const QString &filenameWithPath, int state);    // -1 stores SQL NULL

    bool isDatabaseOpened() {
        return(databaseOpened);
    }

private:
    bool debugErrors(const char *where, QSqlQuery &q);
    // a cuesheets row exists only while SOME per-cuesheet setting is non-default (#1724)
    void deleteCuesheetRowIfAllDefault(const QString &relativePath);
    void exec(const char *where, QSqlQuery &q);
    void exec(const char *where, QSqlQuery &q, const QString &str);
    QString tagsBackgroundColorString;
    QString tagsForegroundColorString;
    bool databaseOpened;
    QSqlDatabase m_db;
    int current_session_id;
    QHash<QString, int> tagCounts;

    void ensureSchema(TableDefinition *);
    void ensureIndex(IndexDefinition *index_definition);
    int getSongIDFromFilename(const QString &filename, const QString &filenameWithPathNormalized);
    int getSongIDFromFilenameAlone(const QString &filename);
    int getSessionIDFromName(const QString &name);

    bool tagColorCacheSet;
    QHash<QString,QPair<QString,QString>> tagColorCache;
    
    std::vector<QString> root_directories;

    // absolute path -> "applemusic:<16 hex digits>", for every track in the Apple Music library
    //   that SquareDesk can see (issue #1747).  Empty when Apple Music sync is off, in which case
    //   songKeyFor() degrades to removeRootDirs() for everything.
    QHash<QString, QString> appleMusicKeyByPath;
};

#endif /* ifndef SONGSETTINGS_H_INCLUDED */
