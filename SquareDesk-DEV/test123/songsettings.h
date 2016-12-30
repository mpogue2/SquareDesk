#ifndef SONGSETTINGS_H_INCLUDED
#define SONGSETTINGS_H_INCLUDED
#include <QtSql/QSqlDatabase>

class TableDefinition;
class SongSettings
{
public:
    SongSettings();
    void OpenDatabase(const QString &path);
    void SaveSettings(const QString &filename,
                 int pitch,
                 int temp,
                 double introPos,
                 double outroPos);
    bool LoadSettings(const QString &filename,
                 int &pitch,
                 int &temp,
                 double &introPos,
                 double &outroPos);
private:
    QSqlDatabase m_db;
    void EnsureSchema(TableDefinition *);
    int GetSongIDFromFilename(const QString &filename);
};

/*     QString writeLocation = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString sqLiteDBPath = writeLocation + "/SquareDeskPlayer.sqlite3";
 */

#endif /* ifndef SONGSETTINGS_H_INCLUDED */
