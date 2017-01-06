#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QVariant>
#include <QDebug>
#include <vector>

#include "songsettings.h"
using namespace std;

class RowDefinition {
public:
    RowDefinition(const char *name, const char *definition,
        const char *index = NULL)
        :
        name(name), definition(definition),
        index(index),
        found(false)
    {}
    const char *name;
    const char *definition;
    const char *index;
    bool found;
};

class TableDefinition {
public:
    TableDefinition(const char *name, RowDefinition *rows)
        :
        name(name), rows(rows)
    {}
    const char *name;
    RowDefinition *rows;
};


void SongSettings::EnsureSchema(TableDefinition *table_definition)
{
    QSqlQuery q;
    q.exec("PRAGMA TABLE_INFO(" + QString(table_definition->name) + ")");

    bool found_any_fields = false;
    vector<QString> alter_statements;
    
    while (q.next())
    {
        found_any_fields = true;
        /* int column_number = q.value(0).toInt(); */
        QString field_name = q.value(1).toString();
        for (int i = 0; table_definition->rows[i].name; ++i)
        {
            RowDefinition *row = &table_definition->rows[i];
            if (field_name == row->name)
            {
                row->found = true;
            }
        }
    }
    if (found_any_fields)
    {
        for (int i = 0; table_definition->rows[i].name; ++i)
        {
            RowDefinition *row = &table_definition->rows[i];
            if (!row->found)
            {
                QString alter = "ALTER TABLE ";
                alter +=  table_definition->name;
                alter += " ADD COLUMN ";
                alter += row->name;
                alter += " ";
                alter += row->definition;
                alter_statements.push_back(alter);
                if (row->index)
                {
                    alter_statements.push_back(row->index);
                }
            }
        }
    }
    else
    {
        QString alter = "CREATE TABLE ";
        alter +=  table_definition->name;
        alter += " (\n";
        for (int i = 0; table_definition->rows[i].name; ++i)
        {
            RowDefinition *row = &table_definition->rows[i];
            alter += "  ";
            alter += row->name;
            alter += " ";
            alter += row->definition;
            if (table_definition->rows[i+1].name)
                alter += ",";
            alter += "\n";
            if (row->index != NULL)
            {
                alter_statements.push_back(row->index);
            }
        }
        alter += ")";
        alter_statements.insert(alter_statements.begin(), alter);
    } 
    for (vector<QString>::iterator alter = alter_statements.begin();
         alter != alter_statements.end();
         ++alter)
    {
        qDebug() << "Executing " << *alter;
        q.exec(*alter);
        if (m_db.lastError().type() != QSqlError::NoError)
        {
            qDebug() << "Creating database: " << m_db.lastError();
        }
    }
}


RowDefinition song_rows[] =
{
    RowDefinition("id", "int auto_increment primary_key"),
    RowDefinition("filename", "text",
                  "CREATE UNIQUE INDEX songs_filename_idx ON songs(filename)"),
    RowDefinition("name", "text",
        "CREATE INDEX songs_name_idx ON SONGS(name)"),
    RowDefinition("pitch", "int"),
    RowDefinition("tempo", "int"),
    RowDefinition("volume", "int"),
    RowDefinition("introPos", "float"),
    RowDefinition("outroPos", "float"),
    RowDefinition(NULL, NULL),
};

TableDefinition song_table("songs", song_rows);

RowDefinition session_rows[] =
{
    RowDefinition("id", "int auto_increment primary_key"),
    RowDefinition("name", "text",
                  "CREATE UNIQUE INDEX session_name_idx ON session(name)"),
    RowDefinition(NULL, NULL),
};
TableDefinition session_table("sessions", song_rows);

RowDefinition song_play_rows[] =
{
    RowDefinition("id", "int auto_increment primary_key"),
    RowDefinition("song_id", "int references songs(id)"),
    RowDefinition("session_id", "int references songs(id)"),
    RowDefinition("timestamp", "DATETIME DEFAULT CURRENT_TIMESTAMP"
                  "CREATE INDEX song_play_song_session_timestamp songs(song_id,session_id,timestamp)"),
    RowDefinition(NULL, NULL),
};
TableDefinition song_plays_table("song_plays", song_play_rows);


/*
"CREATE TABLE play_history (
 id int auto_increment primary key,
 song_id int REFERENCES songs(id),
 audience_id INT REFERENCES play_audiences(id)
 played_on TIMESTAMP,
    );
*/

SongSettings::SongSettings()
{
}

void SongSettings::OpenDatabase(const QString& path)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path + "/SquareDesk.sqlite3");
 
    if (!m_db.open())
    {
        qDebug() << "Error: database faile: " << path << ":" << m_db.lastError();
    }
    else
    {
        qDebug() << "Database: connection ok";
    }
    EnsureSchema(&song_table);
    EnsureSchema(&session_table);
    EnsureSchema(&song_plays_table);
    InitializeSessionsModel();
}

int SongSettings::GetSongIDFromFilename(const QString &filename)
{
    int id = -1;
    {
        QSqlQuery q;
        q.prepare("SELECT id FROM songs WHERE filename=:filename");
        q.bindValue(":filename", filename);
        q.exec();
        while (q.next())
        {
            id = q.value(0).toInt();
        }
    }
    return id;
}
int SongSettings::GetSessionIDFromName(const QString &name)
{
    int id = -1;
    {
        QSqlQuery q;
        q.prepare("SELECT id FROM sessions WHERE name=:name");
        q.bindValue(":name", name);
        q.exec();
        while (q.next())
        {
            id = q.value(0).toInt();
        }
    }
    return id;
}


void SongSettings::InitializeSessionsModel()
{
    modelSessions.setTable("sessions");
    modelSessions.setEditStrategy(QSqlTableModel::OnManualSubmit);
    modelSessions.select();
    modelSessions.setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    modelSessions.setHeaderData(1, Qt::Horizontal, QObject::tr("Name"));

}



void SongSettings::MarkSongPlayed(const QString &filename,
                                  const QString &session)
{
    int song_id = GetSongIDFromFilename(filename);
    int session_id = GetSessionIDFromName(session);
    QSqlQuery q;
    q.prepare("INSERT INTO song_plays(song_id,session_id) VALUES (:song_id, :session_id)");
    q.bindValue(":song_id", song_id);
    q.bindValue(":session_id", session_id);
    q.exec();
}

void SongSettings::SaveSettings(const QString &filename,
                                const QString &songname,
                 int pitch,
                 int tempo,
                 double introPos,
                 double outroPos)
{
    int id = GetSongIDFromFilename(filename);

    QSqlQuery q;
    if (id == -1)
    {
        q.prepare("INSERT INTO songs(filename, name, pitch, tempo, introPos, outroPos) VALUES (:filename, :name, :pitch, :tempo, :introPos, :outroPos)");
    }
    else
    {
        q.prepare("UPDATE songs SET name = :name, pitch = :pitch, tempo = :tempo, introPos = :introPos, outroPos = :outroPos WHERE filename = :filename");
    }
    q.bindValue(":filename", filename);
    q.bindValue(":pitch", pitch);
    q.bindValue(":tempo", tempo);
    q.bindValue(":introPos", introPos);
    q.bindValue(":outroPos", outroPos);
    q.exec();
    if (m_db.lastError().type() != QSqlError::NoError)
    {
        qDebug() << "SaveSettings: " << m_db.lastError();
    }
}

bool SongSettings::LoadSettings(const QString &filename,
                                const QString &songname,
                                int &pitch,
                                int &tempo,
                                double &introPos,
                                double &outroPos)
{
    bool foundResults = false;
    {
        QSqlQuery q;
        q.prepare("SELECT filename, pitch, tempo, introPos, outroPos FROM songs WHERE filename=:filename");
        q.bindValue(":filename", filename);
        q.exec();
        while (q.next())
        {
            foundResults = true;
            pitch = q.value(1).toInt();
            tempo = q.value(2).toInt();
            introPos = q.value(3).toFloat();
            outroPos = q.value(4).toFloat();
        }
    }
    if (!foundResults)
    {
        QSqlQuery q;
        q.prepare("SELECT filename, pitch, tempo, introPos, outroPos FROM songs WHERE name=:name");
        q.bindValue(":name", songname);
        q.exec();
        while (q.next())
        {
            foundResults = true;
            pitch = q.value(1).toInt();
            tempo = q.value(2).toInt();
            introPos = q.value(3).toFloat();
            outroPos = q.value(4).toFloat();
        }
    }
    return foundResults;
}

void SongSettings::CloseDatabase()
{
    m_db.close();
}
