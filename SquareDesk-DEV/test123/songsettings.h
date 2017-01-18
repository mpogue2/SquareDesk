#ifndef SONGSETTINGS_H_INCLUDED
#define SONGSETTINGS_H_INCLUDED
#include <QtSql/QSqlDatabase>
#include <QtSql>

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
};

/*     QString writeLocation = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString sqLiteDBPath = writeLocation + "/SquareDeskPlayer.sqlite3";
 */

#endif /* ifndef SONGSETTINGS_H_INCLUDED */
