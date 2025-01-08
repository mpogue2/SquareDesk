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


class SongSetting
{
    // Attributes are set in songsetting_attributes.h
#define SONGSETTING_ELEMENT(type, name) private:                \
    type m_##name;                                              \
    bool set_##name;                                            \
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

    void setCurrentSession(int id) { current_session_id = id; }
    int getCurrentSession() { return current_session_id; }
    void getSongAges(QHash<QString,QString> &ages, bool show_all_sessions);
    QString getSongAge(const QString &filename, const QString &filenameWithPath, bool show_all_sessions);
    void markSongPlayed(const QString &filename, const QString &filenameWithPath);

    QString removeRootDirs(const QString &filenameWithPath);
    QString primaryRootDir();

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

private:
    bool debugErrors(const char *where, QSqlQuery &q);
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
};

#endif /* ifndef SONGSETTINGS_H_INCLUDED */
