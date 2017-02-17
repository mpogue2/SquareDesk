#ifndef SONGSETTINGS_H_INCLUDED
#define SONGSETTINGS_H_INCLUDED
#include <QtSql/QSqlDatabase>
#include <QtSql>

#define SONGSETTINGS_INCLUDE_SONG_ID_CACHE
#define SONGSETTINGS_INCLUDE_SONG_CACHE
#define SONGSETTINGS_INCLUDE_SONG_AGE_CACHE


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
    void openDatabase(const QString &path);
    void closeDatabase();
    void saveSettings(const QString &filename,
                      const QString &songname,
                      int volume,
                 int pitch,
                 int temp,
                 double introPos,
                 double outroPos);
    bool loadSettings(const QString &filename,
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
    QString getSongAge(const QString &filename);
    void markSongPlayed(const QString &filename);

    
private:
    void debugErrors(const char *where, QSqlQuery &q);
    void exec(const char *where, QSqlQuery &q);
    void exec(const char *where, QSqlQuery &q, const QString &str);
    
    QSqlDatabase m_db;
    int current_session_id;
    
    void ensureSchema(TableDefinition *);
    int getSongIDFromFilename(const QString &filename);
    int getSessionIDFromName(const QString &name);
    
#ifdef SONGSETTINGS_INCLUDE_SONG_ID_CACHE
    std::map<QString,int> song_id_cache;
#endif
#ifdef SONGSETTINGS_INCLUDE_SONG_CACHE
    std::map<QString,SongSetting> song_cache;
#endif
#ifdef SONGSETTINGS_INCLUDE_SONG_AGE_CACHE
    std::map<QString, int> song_age_cache;
#endif
    
};

/*     QString writeLocation = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString sqLiteDBPath = writeLocation + "/SquareDeskPlayer.sqlite3";
 */

#endif /* ifndef SONGSETTINGS_H_INCLUDED */
