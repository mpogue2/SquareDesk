#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QVariant>
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

    bool found_any_fields;
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
            alter += ";\n";
            if (row->index != NULL)
            {
                alter_statements.push_back(row->index);
            }
        }
        alter += ")";
        alter_statements.insert(alter_statements.begin(), alter);
    } 
    for (QString alter : alter_statements)
    {
        q.exec(alter);
    }
}


RowDefinition song_rows[] =
{
    RowDefinition("id", "int auto_increment primary_key"),
    RowDefinition("filename", "text primary_key",
                  "CREATE UNIQUE INDEX songs_filename_idx ON songs(filename)"),
    RowDefinition("name", "text",
        "CREATE INDEX songs_name_idx ON SONGS(name)"),
    RowDefinition("pitch", "int"),
    RowDefinition("tempo", "int"),
    RowDefinition("volume", "int"),
    RowDefinition("introPos", "double"),
    RowDefinition("outroPos", "double"),
    RowDefinition(NULL, NULL),
};

TableDefinition song_table("songs", song_rows);


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
    m_db.setDatabaseName(path);
 
    if (!m_db.open())
    {
//        qDebug() << "Error: connection with database fail";
    }
    else
    {
//        qDebug() << "Database: connection ok";
    }
    EnsureSchema(&song_table);
}

int SongSettings::GetSongIDFromFilename(const QString &filename)
{
    int id = -1;
    {
        QSqlQuery q;
        q.prepare("SELECT id FROM songs WHERE filename=:filename");
        q.bindValue(":filename", filename);
        while (q.next())
        {
            id = q.value(0).toInt();
        }
    }
    return id;
}

void SongSettings::SaveSettings(const QString &filename,
                 int pitch,
                 int tempo,
                 double introPos,
                 double outroPos)
{
    int id = GetSongIDFromFilename(filename);

    QSqlQuery q;
    if (id == -1)
    {
        q.prepare("INSERT INTO songs(filename, pitch, tempo, introPos, outroPos) VALUES (:filename, :pitch, :tempo, :introPos, :outroPos)");
    }
    else
    {
        q.prepare("UPDATE songs SET pitch = :pitch, tempo = :tempo, introPos = :introPos, outroPos = :outroPos WHERE filename = :filename");
    }
    q.bindValue(":filename", filename);
    q.bindValue(":pitch", pitch);
    q.bindValue(":tempo", tempo);
    q.bindValue(":introPos", introPos);
    q.bindValue(":outroPos", outroPos);
    q.exec();
}

bool SongSettings::LoadSettings(const QString &filename,
                                int &pitch,
                                int &tempo,
                                double &introPos,
                                double &outroPos)
{
    QSqlQuery q;
    q.prepare("SELECT filename, pitch, tempo, introPos, outroPos FROM songs WHERE filename=:filename");
    bool foundResults = false;
    while (q.next())
    {
        foundResults = true;
        pitch = q.value(1).toInt();
        tempo = q.value(2).toInt();
        introPos = q.value(3).toFloat();
        outroPos = q.value(4).toFloat();
    }
    return foundResults;
}
