#ifndef SONGSETTINGS_H_INCLUDED
#define SONGSETTINGS_H_INCLUDED
#include <QtSql/QSqlDatabase>
#include <QtSql>
#include <vector>

// #define SONGSETTINGS_INCLUDE_SONG_ID_CACHE
// #define SONGSETTINGS_INCLUDE_SONG_CACHE
// #define SONGSETTINGS_INCLUDE_SONG_AGE_CACHE


#ifdef SONGSETTINGS_INCLUDE_SONG_CACHE
struct SongSetting {
    int volume;
    int pitch;
    int tempo;
    double introPos;
    double outroPos;
};
#include <map>
#endif


class TableDefinition;
class SongSettings
{
public:
    SongSettings();
    void openDatabase(const QString &path,
                      const QString &mainRootDir,
                      const QString &guestRootDir,
                      bool in_memory);
    void closeDatabase();
    void saveSettings(const QString &filename,
                      const QString &filenameWithPath,
                      const QString &songname,
                      int volume,
                      int pitch,
                      int temp,
                      double introPos,
                      double outroPos,
                      const QString &cuesheetName);
    
    bool loadSettings(const QString &filename,
                      const QString &filenameWithPath,
                      const QString &songname,
                      int &volume,
                      int &pitch,
                      int &temp,
                      double &introPos,
                      double &outroPos,
                      QString &cuesheetName);

    bool loadSettings(const QString &filename,
                      const QString &filenameWithPath,
                      const QString &songname,
                      int &volume,
                      int &pitch,
                      int &temp,
                      double &introPos,
                      double &outroPos);
    void initializeSessionsModel();
    QSqlTableModel modelSessions;
    void setCurrentSession(int id) { current_session_id = id; }
    int getCurrentSession() { return current_session_id; }
    QString getSongAge(const QString &filename, const QString &filenameWithPath);
    void markSongPlayed(const QString &filename, const QString &filenameWithPath);

    
private:
    void debugErrors(const char *where, QSqlQuery &q);
    void exec(const char *where, QSqlQuery &q);
    void exec(const char *where, QSqlQuery &q, const QString &str);

    bool databaseOpened;
    QSqlDatabase m_db;
    int current_session_id;
    
    void ensureSchema(TableDefinition *);
    int getSongIDFromFilename(const QString &filename, const QString &filenameWithPathNormalized);
    int getSongIDFromFilenameAlone(const QString &filename);
    int getSessionIDFromName(const QString &name);
    QString removeRootDirs(const QString &filenameWithPath);

    
#ifdef SONGSETTINGS_INCLUDE_SONG_ID_CACHE
    std::map<QString,int> song_id_cache;
#endif
#ifdef SONGSETTINGS_INCLUDE_SONG_CACHE
    std::map<QString,SongSetting> song_cache;
#endif
#ifdef SONGSETTINGS_INCLUDE_SONG_AGE_CACHE
    std::map<QString, int> song_age_cache;
#endif
    std::vector<QString> root_directories;
    
};

/*     QString writeLocation = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString sqLiteDBPath = writeLocation + "/SquareDeskPlayer.sqlite3";
 */

#endif /* ifndef SONGSETTINGS_H_INCLUDED */
