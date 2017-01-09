#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QVariant>
#include <QDebug>
#include <vector>

#include "songsettings.h"
using namespace std;


void SongSettings::exec(const char *where, QSqlQuery &q)
{
    if (!q.exec())
    {
        debugErrors(where, q);
    }
}

void SongSettings::exec(const char *where, QSqlQuery &q, const QString &str)
{
    if (!q.exec(str))
    {
        debugErrors(where, q);
    }
}


void SongSettings::debugErrors(const char *where, QSqlQuery & /* q */)
{
    if (m_db.lastError().type() != QSqlError::NoError)
    {
        qDebug() << where << ":" << m_db.lastError();
    }
}


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


void SongSettings::ensureSchema(TableDefinition *table_definition)
{
    QSqlQuery q(m_db);
    exec("ensureSchema", q, "PRAGMA TABLE_INFO(" + QString(table_definition->name) + ")");
    

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
        alter += "\n)\n";

        alter_statements.insert(alter_statements.begin(), alter);
    } 
    for (vector<QString>::iterator alter = alter_statements.begin();
         alter != alter_statements.end();
         ++alter)
    {
        qDebug() << "Executing " << *alter;
        exec("ensureSchema", q, *alter);
        if (m_db.lastError().type() != QSqlError::NoError)
        {
            qDebug() << "Creating database: " << m_db.lastError();
        }
    }
}


RowDefinition song_rows[] =
{
    RowDefinition("filename", "text PRIMARY KEY"),
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
    RowDefinition("name", "text",
                  "CREATE UNIQUE INDEX session_name_idx ON session(name)"),
    RowDefinition(NULL, NULL),
};
TableDefinition session_table("sessions", song_rows);

RowDefinition song_play_rows[] =
{
    RowDefinition("song_rowid", "int references songs(rowid)"),
    RowDefinition("session_rowid", "int references songs(rowid)"),
    RowDefinition("played_on", "DATETIME DEFAULT CURRENT_TIMESTAMP",
                  "CREATE INDEX song_play_song_session_played_on songs(song_rowid,session_rowid,played_on)"),
    RowDefinition(NULL, NULL),
};
TableDefinition song_plays_table("song_plays", song_play_rows);


/*
"CREATE TABLE play_history (
 id int auto_increment primary key,
 song_rowid int REFERENCES songs(id),
 audience_id INT REFERENCES play_audiences(id)
 played_on TIMESTAMP,
    );
*/

SongSettings::SongSettings()
{
    QDate date(QDate::currentDate());
    current_session_id = date.dayOfWeek() + 1;
}

static const char *default_session_names[] =
{
    "Practice",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
    "Sunday",
    NULL
};

void SongSettings::openDatabase(const QString& path)
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
    ensureSchema(&song_table);
    ensureSchema(&session_table);
    ensureSchema(&song_plays_table);
    {
        unsigned int sessions_available = 0;
        {
            QSqlQuery q(m_db);
            q.prepare("SELECT rowid FROM sessions WHERE id <= 8");
            exec("openDatabase", q);
            while (q.next())
            {
                int id = q.value(0).toInt();
                sessions_available |= (1 << id);
            }
        }
        {
            for (int id = 1; default_session_names[id - 1]; ++id)
            {
                if (!(sessions_available & (1 << id)))
                {
                    QSqlQuery q(m_db);
                    q.prepare("INSERT into SESSIONS(rowid,name) VALUES(:id, :name)");
                    q.bindValue(":name",default_session_names[id - 1]);
                    q.bindValue(":id", id);
                    exec("openDatabase", q);
                }
            }
        }
    }

    initializeSessionsModel();
}

int SongSettings::getSongIDFromFilename(const QString &filename)
{
    int id = -1;
    {
        QSqlQuery q(m_db);
        q.prepare("SELECT rowid FROM songs WHERE filename=:filename");
        q.bindValue(":filename", filename);
        qDebug() << "Looking for song ID for " << filename;
        exec("getSongIDFromFilename",q);
        while (q.next())
        {
            id = q.value(0).toInt();
            qDebug() << "Found" << id;
        }
    }
    return id;
}
int SongSettings::getSessionIDFromName(const QString &name)
{
    int id = -1;
    {
        QSqlQuery q(m_db);
        q.prepare("SELECT rowid FROM sessions WHERE name=:name");
        q.bindValue(":name", name);
        exec("getSessionIDFromFilename",q);
        while (q.next())
        {
            id = q.value(0).toInt();
        }
    }
    return id;
}


void SongSettings::initializeSessionsModel()
{
    modelSessions.setTable("sessions");
    modelSessions.setEditStrategy(QSqlTableModel::OnManualSubmit);
    modelSessions.select();
    modelSessions.setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    modelSessions.setHeaderData(1, Qt::Horizontal, QObject::tr("Name"));

}



void SongSettings::markSongPlayed(const QString &filename)
{
    qDebug() << "Marking song played" << filename;
    int song_rowid = getSongIDFromFilename(filename);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO song_plays(song_rowid,session_rowid) VALUES (:song_rowid, :session_rowid)");
    q.bindValue(":song_rowid", song_rowid);
    q.bindValue(":session_rowid", current_session_id);
    exec("markSongPlayed", q);
}

QString SongSettings::getSongAge(const QString &filename)
{
    QSqlQuery q(m_db);
    qDebug() << "getsongage " << filename << " : " << current_session_id;
    q.prepare("SELECT julianday('now') - julianday(played_on) FROM song_plays JOIN songs ON songs.rowid = song_plays.song_rowid WHERE session_rowid = :session_rowid and songs.filename = :filename ORDER BY played_on DESC LIMIT 1");
    q.bindValue(":filename", filename);
    q.bindValue(":session_rowid", current_session_id);
    exec("getSongAge", q);

    if (q.next())
    {
        QString str(QString("%1").arg(q.value(0).toInt(), 3));
        qDebug() << "Returning song age " << str << "for" << filename;
        return str;
    }
    return QString("");
}

void SongSettings::saveSettings(const QString &filename,
                                const QString &songname,
                                int pitch,
                                int tempo,
                                double introPos,
                                double outroPos)
{
    int id = getSongIDFromFilename(filename);

    QSqlQuery q(m_db);
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
    q.bindValue(":name", songname);
    exec("saveSettings", q);
}

bool SongSettings::loadSettings(const QString &filename,
                                const QString &songname,
                                int &pitch,
                                int &tempo,
                                double &introPos,
                                double &outroPos)
{
    bool foundResults = false;
    {
        QSqlQuery q(m_db);
        q.prepare("SELECT filename, pitch, tempo, introPos, outroPos FROM songs WHERE filename=:filename");
        q.bindValue(":filename", filename);
        exec("loadSettings", q);

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
        QSqlQuery q(m_db);
        q.prepare("SELECT filename, pitch, tempo, introPos, outroPos FROM songs WHERE name=:name");
        q.bindValue(":name", songname);
        exec("loadSettings", q);
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

void SongSettings::closeDatabase()
{
    m_db.close();
}
