/****************************************************************************
**
** Copyright (C) 2016, 2017 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk/SquareDeskPlayer application.
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
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QVariant>
#include <QDebug>
#include <vector>
#include <map>

#include "songsettings.h"
using namespace std;




void SongSettings::exec(const char *where, QSqlQuery &q)
{
    q.exec();
    debugErrors(where, q);
}

void SongSettings::exec(const char *where, QSqlQuery &q, const QString &str)
{
    q.exec(str);
    debugErrors(where, q);
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
        exec("ensureSchema", q, *alter);
    }
}


RowDefinition song_rows[] =
{
    RowDefinition("filename", "text PRIMARY KEY"),
    RowDefinition("songname", "text",
        "CREATE INDEX songs_songname ON songs(songname)"),
    RowDefinition("name", "text",
        "CREATE INDEX songs_name_idx ON songs(name)"),
    RowDefinition("pitch", "int"),
    RowDefinition("tempo", "int"),
    RowDefinition("tempoIsPercent", "int"),
    RowDefinition("volume", "int"),
    RowDefinition("introPos", "float"),
    RowDefinition("outroPos", "float"),
    RowDefinition("last_cuesheet", "text"),
    RowDefinition("songLength", "float"),
    RowDefinition("introOutroIsTimeBased", "int"),
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
    RowDefinition("session_rowid", "int references session(rowid)"),
    RowDefinition("played_on", "DATETIME DEFAULT CURRENT_TIMESTAMP",
                  "CREATE INDEX song_play_song_session_played_on songs(song_rowid,session_rowid,played_on)"),
    RowDefinition(NULL, NULL),
};
TableDefinition song_plays_table("song_plays", song_play_rows);


RowDefinition call_taught_on_rows[] =
{
    RowDefinition("dance_program", "TEXT"),
    RowDefinition("call_name", "TEXT"),
    RowDefinition("session_rowid", "INT REFERENCES session(rowid)",
                  "CREATE INDEX call_taught_on_dance_program_call_name_session ON call_taught_on(dance_program, call_name, session_rowid"),
    RowDefinition("taught_on", "DATETIME DEFAULT CURRENT_TIMESTAMP"),
    RowDefinition(NULL, NULL),
};
TableDefinition call_taught_on_table("call_taught_on", call_taught_on_rows);

/*
"CREATE TABLE play_history (
 id int auto_increment primary key,
 song_rowid int REFERENCES songs(id),
 audience_id INT REFERENCES play_audiences(id)
 played_on TIMESTAMP,
    );
*/

SongSettings::SongSettings() :
    databaseOpened(false)
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

static const char database_type_name[] = "QSQLITE";
void SongSettings::openDatabase(const QString& path,
                                const QString& root_dir,
                                const QString& guest_dir,
                                bool in_memory)
{
    closeDatabase();
    root_directories.clear();
    if (root_dir.length() > 0)
        root_directories.push_back(root_dir);
    if (guest_dir.length() > 0)
        root_directories.push_back(guest_dir);

    m_db = QSqlDatabase::addDatabase(database_type_name);
    if (in_memory)
    {
        m_db.setDatabaseName(":memory:");
    }
    else
    {
        QDir dir(path);

        if (!dir.exists())
        {
            dir.mkpath(".");
        }
        m_db.setDatabaseName(path + "/SquareDesk.sqlite3");
    }

    if (!m_db.open())
    {
        qDebug() << "Error: database fail: " << path << ":" << m_db.lastError();
    }
    else
    {
        databaseOpened = true;
//        qDebug() << "Database: connection ok";
    }
    ensureSchema(&song_table);
    ensureSchema(&session_table);
    ensureSchema(&song_plays_table);
    ensureSchema(&call_taught_on_table);
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


int SongSettings::getSongIDFromFilenameAlone(const QString &filename)
{
    int id = -1;

    {
        QSqlQuery q(m_db);
        q.prepare("SELECT rowid FROM songs WHERE filename=:filename");
        q.bindValue(":filename", filename);
        exec("getSongIDFromFilename",q);
        while (q.next())
        {
            id = q.value(0).toInt();
        }
    }
    return id;
}

int SongSettings::getSongIDFromFilename(const QString &filename, const QString &filenameWithPathNormalized)
{
    int id = getSongIDFromFilenameAlone(filenameWithPathNormalized);
    if (-1 == id)
    {
        id = getSongIDFromFilenameAlone(filename);
        if (-1 != id)
        {
            QSqlQuery q(m_db);
            q.prepare("UPDATE songs SET filename=:newfilename, songname=:songname WHERE rowid=:id");
            q.bindValue(":newfilename", filenameWithPathNormalized);
            q.bindValue(":songname", filename);
            q.bindValue(":id", id);
            exec("updatingSongName", q);
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

QString SongSettings::primaryRootDir()
{
    return root_directories[0];
}

QString SongSettings::removeRootDirs(const QString &filenameWithPath)
{
    QString filenameWithPathNormalized(filenameWithPath);
    for (QString root_dir : root_directories)
    {
        if (filenameWithPath.startsWith(root_dir))
        {
            filenameWithPathNormalized.remove(0, root_dir.length());
            break;
        }
    }
    return filenameWithPathNormalized;
}

void SongSettings::markSongPlayed(const QString &filename, const QString &filenameWithPath)
{
    QString filenameWithPathNormalized = removeRootDirs(filenameWithPath);
    int song_rowid = getSongIDFromFilename(filename, filenameWithPathNormalized);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO song_plays(song_rowid,session_rowid) VALUES (:song_rowid, :session_rowid)");
    q.bindValue(":song_rowid", song_rowid);
    q.bindValue(":session_rowid", current_session_id);
    exec("markSongPlayed", q);
}

QString SongSettings::getCallTaughtOn(const QString &program, const QString &call_name)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT date(taught_on, 'localtime') FROM call_taught_on WHERE dance_program= :dance_program AND call_name = :call_name AND session_rowid = :session_rowid");
    q.bindValue(":session_rowid", current_session_id);
    q.bindValue(":dance_program", program);
    q.bindValue(":call_name", call_name);
    exec("getCallTaughtOn", q);
    if (q.next())
    {
        return q.value(0).toString();
    }
    return QString("");
}

void SongSettings::setCallTaught(const QString &program, const QString &call_name)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO call_taught_on(dance_program, call_name, session_rowid) VALUES (:dance_program, :call_name, :session_rowid)");
    q.bindValue(":session_rowid", current_session_id);
    q.bindValue(":dance_program", program);
    q.bindValue(":call_name", call_name);
    exec("setCallTaught", q);
}
void SongSettings::deleteCallTaught(const QString &program, const QString &call_name)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM call_taught_on WHERE dance_program = :dance_program AND call_name = :call_name AND session_rowid = :session_rowid");
    q.bindValue(":session_rowid", current_session_id);
    q.bindValue(":dance_program", program);
    q.bindValue(":call_name", call_name);
    exec("deleteCallTaught", q);
}

void SongSettings::clearTaughtCalls(const QString &program)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM call_taught_on WHERE session_rowid = :session_rowid AND dance_program = :dance_program");
    q.bindValue(":session_rowid", current_session_id);
    q.bindValue(":dance_program", program);
    exec("clearTaughtCalls", q);
}

void SongSettings::getSongAges(QHash<QString,QString> &ages, bool show_all_sessions)
{
    QString sql("SELECT filename, julianday('now') - julianday(max(played_on)) FROM songs JOIN song_plays ON song_plays.song_rowid=songs.rowid");
    if (!show_all_sessions)
        sql += " WHERE session_rowid = :session_rowid";
    sql += " GROUP BY songs.rowid";
    QSqlQuery q(m_db);
    q.prepare(sql);
    q.bindValue(":session_rowid", current_session_id);

    exec("songAges", q);
    while (q.next())
    {
        int age = q.value(1).toInt();
        QString str(QString("%1").arg(age, 3));
        ages[q.value(0).toString()] = str;
    }
}

QString SongSettings::getSongAge(const QString &filename, const QString &filenameWithPath, bool show_all_sessions)
{
    QString filenameWithPathNormalized = removeRootDirs(filenameWithPath);
    QString sql = "SELECT julianday('now') - julianday(played_on) FROM song_plays JOIN songs ON songs.rowid = song_plays.song_rowid WHERE ";
    if (!show_all_sessions)
    {
        sql += "session_rowid = :session_rowid AND ";
    }

    sql += "songs.filename = :filename ORDER BY played_on DESC LIMIT 1";

    {
        QSqlQuery q(m_db);
        q.prepare(sql);
        q.bindValue(":filename", filenameWithPathNormalized);
        q.bindValue(":session_rowid", current_session_id);
        exec("getSongAge", q);

        if (q.next())
        {
            int age = q.value(0).toInt();
            QString str(QString("%1").arg(age, 3));
            return str;
        }
    }

    {
        QSqlQuery q(m_db);
        q.prepare(sql);
        q.bindValue(":filename", filename);
        q.bindValue(":session_rowid", current_session_id);
        exec("getSongAge", q);

        if (q.next())
        {
            int age = q.value(0).toInt();
            QString str(QString("%1").arg(age, 3));
            return str;
        }
    }
    return QString("");
}


SongSetting::SongSetting()
    :
    m_filename(),
    set_filename(false),
    m_filenameWithPath(),
    set_filenameWithPath(false),
    m_songname(),
    set_songname(false),
    m_volume(),
    set_volume(false),
    m_pitch(),
    set_pitch(false),
    m_tempo(),
    set_tempo(false),
    m_tempoIsPercent(),
    set_tempoIsPercent(false),
    m_introPos(),
    set_introPos(false),
    m_outroPos(),
    set_outroPos(false),
    m_cuesheetName(),
    set_cuesheetName(false),
    m_introOutroIsTimeBased(false),     // init to false, because this is a new column and if not present, it's not time-based
    set_introOutroIsTimeBased(false),   // all of the set_* should be false here, I think.
    m_songLength(0.0),                  // init to zero, and this will blow up if used for division
    set_songLength(false)               // all of the set_* should be false here, I think.
{
}

QDebug operator<<(QDebug dbg, const SongSetting &setting)
{
    QDebugStateSaver stateSaver(dbg);
    dbg.nospace() << "SongSetting(" <<
                     "\n    filename: " << setting.m_filename << "," << setting.set_filename << ";" <<
                     "\n    filenameWithPath: " << setting.m_filenameWithPath << "," << setting.set_filenameWithPath << ";" <<
                     "\n    songname: " << setting.m_songname << "," << setting.set_songname << ";" <<
                     "\n    volume: " << setting.m_volume << "," << setting.set_volume << ";" <<
                     "\n    pitch: " << setting.m_pitch << "," << setting.set_pitch << ";" <<
                     "\n    tempo: " << setting.m_tempo << "," << setting.set_tempo << ";" <<
                     "\n    tempoIsPercent: " << setting.m_tempoIsPercent << "," << setting.set_tempoIsPercent << ";" <<
                     "\n    introPos: " << setting.m_introPos << "," << setting.set_introPos << ";" <<
                     "\n    outroPos: " << setting.m_outroPos << "," << setting.set_outroPos << ";" <<
                     "\n    cuesheetName: " << setting.m_cuesheetName << "," << setting.set_cuesheetName <<
                     "\n    introOutroIsTimeBased: " << setting.m_introOutroIsTimeBased << "," << setting.set_introOutroIsTimeBased <<
                     "\n    songLength: " << setting.m_songLength << "," << setting.set_songLength << ")";


    return dbg;
}


void SongSettings::saveSettings(const QString &filenameWithPath,
                                const SongSetting &settings)
{
    QString filenameWithPathNormalized = removeRootDirs(filenameWithPath);
    int id = getSongIDFromFilename(settings.getFilename(), filenameWithPathNormalized);

    QStringList fields;
    if (settings.isSetFilename()) { fields.append("songname" ); }
    if (settings.isSetPitch()) { fields.append("pitch" ); }
    if (settings.isSetTempo()) { fields.append("tempo" ); }
    if (settings.isSetTempoIsPercent()) { fields.append("tempoIsPercent" ); }
    if (settings.isSetIntroPos()) { fields.append("introPos" ); }
    if (settings.isSetOutroPos()) { fields.append("outroPos" ); }
    if (settings.isSetVolume()) { fields.append("volume" ); }
    if (settings.isSetSongname()) { fields.append("name" ); }
    if (settings.isSetCuesheetName()) { fields.append("last_cuesheet" ); }  // ADDED ********
    if (settings.isSetSongLength()) { fields.append("songLength" ); }
    if (settings.isSetIntroOutroIsTimeBased()) { fields.append("introOutroIsTimeBased" ); }


    QSqlQuery q(m_db);
    if (id == -1)
    {
        fields.append("filename");
        QString sql("INSERT INTO songs(");
        {
            QListIterator<QString> iter(fields);
            bool first = true;
            while (iter.hasNext())
            {
                QString s = iter.next();
                if (!first) { sql += ","; }
                first = false;
                sql += s;
            }
        }
        sql += ") VALUES (";
        {
            QListIterator<QString> iter(fields);
            bool first = true;
            while (iter.hasNext())
            {
                QString s = iter.next();
                if (!first) { sql += ","; }
                first = false;
                sql += ":" + s;
            }
        }
        sql += ")";
        q.prepare(sql);
    }
    else
    {
        QString sql("UPDATE songs SET ");
        {
            QListIterator<QString> iter(fields);
            bool first = true;
            while (iter.hasNext())
            {
                QString s = iter.next();
                if (!first) { sql += ","; }
                first = false;
                sql += s + " = :" + s;
            }
        }
        sql += " WHERE rowid = :rowid";
        q.prepare(sql);
        q.bindValue(":rowid", id);
    }

    q.bindValue(":filename", filenameWithPathNormalized);
    q.bindValue(":songname", settings.getFilename() );
    q.bindValue(":pitch", settings.getPitch() );
    q.bindValue(":tempo", settings.getTempo() );
    q.bindValue(":tempoIsPercent", settings.getTempoIsPercent() );
    q.bindValue(":introPos", settings.getIntroPos() );
    q.bindValue(":outroPos", settings.getOutroPos() );
    q.bindValue(":volume", settings.getVolume() );
    q.bindValue(":name", settings.getSongname() );
    q.bindValue(":last_cuesheet", settings.getCuesheetName() );
    q.bindValue(":songLength", settings.getSongLength() );
    q.bindValue(":introOutroIsTimeBased", settings.getIntroOutroIsTimeBased() );

    exec("saveSettings", q);
}


void setSongSettingFromSQLQuery(QSqlQuery &q, SongSetting &settings)
{
    if (!q.value(1).isNull()) { settings.setPitch(q.value(1).toInt()); };
    if (!q.value(2).isNull()) { settings.setTempo(q.value(2).toInt()); };
    if (!q.value(3).isNull()) { settings.setIntroPos(q.value(3).toFloat()); };
    if (!q.value(4).isNull()) { settings.setOutroPos(q.value(4).toFloat()); };
    if (!q.value(5).isNull()) { settings.setVolume(q.value(5).toInt()); };
    if (!q.value(6).isNull()) { settings.setCuesheetName(q.value(6).toString()); };
    if (!q.value(7).isNull()) { settings.setTempoIsPercent(q.value(7).toBool()); };
    if (!q.value(8).isNull()) { settings.setSongLength(q.value(8).toFloat()); };
    if (!q.value(9).isNull()) { settings.setIntroOutroIsTimeBased(q.value(9).toBool()); };
}

bool SongSettings::loadSettings(const QString &filenameWithPath,
                                SongSetting &settings)
{
    QString baseSql = "SELECT filename, pitch, tempo, introPos, outroPos, volume, last_cuesheet,tempoIsPercent,songLength,introOutroIsTimeBased FROM songs WHERE ";
    QString filenameWithPathNormalized = removeRootDirs(filenameWithPath);
    bool foundResults = false;
    {
        QSqlQuery q(m_db);
        q.prepare(baseSql + "filename=:filename");
        q.bindValue(":filename", filenameWithPathNormalized);
        exec("loadSettings", q);

        while (q.next())
        {
            foundResults = true;
            setSongSettingFromSQLQuery(q, settings);
        }
    }
    if (!foundResults && settings.isSetFilename())
    {
        QSqlQuery q(m_db);
        q.prepare(baseSql + " WHERE filename=:filename");
        q.bindValue(":filename", settings.getFilename());
        exec("loadSettings", q);

        while (q.next())
        {
            foundResults = true;
            setSongSettingFromSQLQuery(q, settings);
        }
    }
    if (!foundResults && settings.isSetSongname())
    {
        QSqlQuery q(m_db);
        q.prepare(baseSql + " WHERE name=:name");
        q.bindValue(":name", settings.getSongname());
        exec("loadSettings", q);
        while (q.next())
        {
            foundResults = true;
            setSongSettingFromSQLQuery(q, settings);
        }
    }
    return foundResults;
}

void SongSettings::closeDatabase()
{
    if (databaseOpened)
    {
        QString connection;
        connection = m_db.connectionName();
        m_db.close();
        m_db = QSqlDatabase();
        m_db.removeDatabase(connection);
    }
}
