#ifndef SONGSETTINGS_H_INCLUDED
#define SONGSETTINGS_H_INCLUDED
#include <QtSql/QSqlDatabase>
#include <QtSql>
#include <vector>


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
                      int tempo,
                      bool tempoIsBPM,
                      double introPos,
                      double outroPos,
                      const QString &cuesheetName);
    
    bool loadSettings(const QString &filename,
                      const QString &filenameWithPath,
                      const QString &songname,
                      int &volume,
                      int &pitch,
                      int &tempo,
                      bool &tempoIsBPM,
                      double &introPos,
                      double &outroPos,
                      QString &cuesheetName);

    bool loadSettings(const QString &filename,
                      const QString &filenameWithPath,
                      const QString &songname,
                      int &volume,
                      int &pitch,
                      int &tempo,
                      bool &tempoIsBPM,
                      double &introPos,
                      double &outroPos);
    void initializeSessionsModel();
    QSqlTableModel modelSessions;
    void setCurrentSession(int id) { current_session_id = id; }
    int getCurrentSession() { return current_session_id; }
    QString getSongAge(const QString &filename, const QString &filenameWithPath);
    void markSongPlayed(const QString &filename, const QString &filenameWithPath);

    QString getCallTaughtOn(const QString &program, const QString &call_name);
    void setCallTaught(const QString &program, const QString &call_name);
    void deleteCallTaught(const QString &program, const QString &call_name);
    void clearTaughtCalls();
    
    
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

    std::vector<QString> root_directories;
};

/*     QString writeLocation = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString sqLiteDBPath = writeLocation + "/SquareDeskPlayer.sqlite3";
 */

#endif /* ifndef SONGSETTINGS_H_INCLUDED */
