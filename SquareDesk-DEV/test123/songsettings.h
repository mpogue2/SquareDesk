/****************************************************************************
**
** Copyright (C) 2016, 2017, 2018 Mike Pogue, Dan Lyke
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

class SongSetting
{
private:
    QString m_filename;
    bool set_filename;
    QString m_filenameWithPath;
    bool set_filenameWithPath;
    QString m_songname;
    bool set_songname;
    int m_volume;
    bool set_volume;
    int m_pitch;
    bool set_pitch;
    int m_tempo;
    bool set_tempo;
    bool m_tempoIsPercent;
    bool set_tempoIsPercent;
    double m_introPos;
    bool set_introPos;
    double m_outroPos;
    bool set_outroPos;
    QString m_cuesheetName;
    bool set_cuesheetName;

    bool m_introOutroIsTimeBased;
    bool set_introOutroIsTimeBased;
    double m_songLength;
    bool set_songLength;

    int m_treble;
    bool set_treble;
    int m_bass;
    bool set_bass;
    int m_midrange;
    bool set_midrange;
    int m_mix;
    bool set_mix;

    int m_loop;
    bool set_loop;

    QString m_tags;
    bool set_tags;
    
public:
    SongSetting();
    QString getFilename() const { return m_filename; }
    bool isSetFilename() const { return set_filename; }
    void setFilename(QString p) { m_filename = p; set_filename = true; }
    QString getFilenameWithPath() const { return m_filenameWithPath; }
    bool isSetFilenameWithPath() const { return set_filenameWithPath; }
    void setFilenameWithPath(QString p) { m_filenameWithPath = p; set_filenameWithPath = true; }
    QString getSongname() const { return m_songname; }
    bool isSetSongname() const { return set_songname; }
    void setSongname(QString p) { m_songname = p; set_songname = true; }
    int getVolume() const { return m_volume; }
    bool isSetVolume() const { return set_volume; }
    void setVolume(int p) { m_volume = p; set_volume = true; }
    int getPitch() const { return m_pitch; }
    bool isSetPitch() const { return set_pitch; }
    void setPitch(int p) { m_pitch = p; set_pitch = true; }
    int getTempo() const { return m_tempo; }
    bool isSetTempo() const { return set_tempo; }
    void setTempo(int p) { m_tempo = p; set_tempo = true; }
    bool getTempoIsPercent() const { return m_tempoIsPercent; }
    bool isSetTempoIsPercent() const { return set_tempoIsPercent; }
    void setTempoIsPercent(bool p) { m_tempoIsPercent = p; set_tempoIsPercent = true; }
    double getIntroPos() const { return m_introPos; }
    bool isSetIntroPos() const { return set_introPos; }
    void setIntroPos(double p) { m_introPos = p; set_introPos = true; }
    double getOutroPos() const { return m_outroPos; }
    bool isSetOutroPos() const { return set_outroPos; }
    void setOutroPos(double p) { m_outroPos = p; set_outroPos = true; }
    QString getCuesheetName() const { return m_cuesheetName; }
    bool isSetCuesheetName() const { return set_cuesheetName; }
    void setCuesheetName(QString p) { m_cuesheetName = p; set_cuesheetName = true; }

    bool getIntroOutroIsTimeBased() const { return m_introOutroIsTimeBased; }
    bool isSetIntroOutroIsTimeBased() const { return set_introOutroIsTimeBased; }
    void setIntroOutroIsTimeBased(bool p) { m_introOutroIsTimeBased = p; set_introOutroIsTimeBased = true; }
    double getSongLength() const { return m_songLength; }
    bool isSetSongLength() const { return set_songLength; }
    void setSongLength(double p) { m_songLength = p; set_songLength = true; }

    int getTreble() const { return m_treble; }
    bool isSetTreble() const { return set_treble; }
    void setTreble(int p) { m_treble = p; set_treble = true; }
    int getBass() const { return m_bass; }
    bool isSetBass() const { return set_bass; }
    void setBass(int p) { m_bass = p; set_bass = true; }
    int getMidrange() const { return m_midrange; }
    bool isSetMidrange() const { return set_midrange; }
    void setMidrange(int p) { m_midrange = p; set_midrange = true; }

    int getMix() const { return m_mix; }
    bool isSetMix() const { return set_mix; }
    void setMix(int p) { m_mix = p; set_mix = true; }

    int getLoop() const { return m_loop; }
    bool isSetLoop() const { return set_loop; }
    void setLoop(int p) { m_loop = p; set_loop = true; }

    QString getTags() const { return m_tags; }
    bool isSetTags() const { return set_tags; }
    void setTags(QString p) { m_tags = p; set_tags = true; }
    
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
                      const QString &guestRootDir,
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

private:
    bool debugErrors(const char *where, QSqlQuery &q);
    void exec(const char *where, QSqlQuery &q);
    void exec(const char *where, QSqlQuery &q, const QString &str);

    bool databaseOpened;
    QSqlDatabase m_db;
    int current_session_id;

    void ensureSchema(TableDefinition *);
    void ensureIndex(IndexDefinition *index_definition);
    int getSongIDFromFilename(const QString &filename, const QString &filenameWithPathNormalized);
    int getSongIDFromFilenameAlone(const QString &filename);
    int getSessionIDFromName(const QString &name);

    std::vector<QString> root_directories;
};

#endif /* ifndef SONGSETTINGS_H_INCLUDED */
