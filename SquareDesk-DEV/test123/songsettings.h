#ifndef SONGSETTINGS_H_INCLUDED
#define SONGSETTINGS_H_INCLUDED
#include <QtSql/QSqlDatabase>
#include <QtSql>

class TableDefinition;
class SongSettings
{
public:
    SongSettings();
    void OpenDatabase(const QString &path);
    void CloseDatabase();
    void SaveSettings(const QString &filename,
                      const QString &songname,
                 int pitch,
                 int temp,
                 double introPos,
                 double outroPos);
    bool LoadSettings(const QString &filename,
                      const QString &songname,
                 int &pitch,
                 int &temp,
                 double &introPos,
                 double &outroPos);
    void MarkSongPlayed(const QString &filename,
                        const QString &session);
    void InitializeSessionsModel();
    QSqlTableModel modelSessions;
    
private:
    QSqlDatabase m_db;
    
    void EnsureSchema(TableDefinition *);
    int GetSongIDFromFilename(const QString &filename);
    int GetSessionIDFromName(const QString &name);
};

/*     QString writeLocation = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString sqLiteDBPath = writeLocation + "/SquareDeskPlayer.sqlite3";
 */

#endif /* ifndef SONGSETTINGS_H_INCLUDED */
