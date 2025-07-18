/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
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
#include <utility>

#include "songsettings.h"
#include "sessioninfo.h"
#include "default_colors.h"
using namespace std;




void SongSettings::exec(const char *where, QSqlQuery &q)
{
    q.exec();
    debugErrors(where, q);
}

void SongSettings::exec(const char *where, QSqlQuery &q, const QString &str)
{
    q.exec(str);
    if (debugErrors(where, q))
    {
        qInfo() << str;
    }
}


bool SongSettings::debugErrors(const char *where, QSqlQuery & q)
{
    bool hadError = false;
    if (m_db.lastError().type() != QSqlError::NoError)
    {
        hadError = true;
        qDebug() << where << ":" << m_db.lastError();
        qInfo() << where << ":" << m_db.lastError();
    }
    if (q.lastError().type() != QSqlError::NoError)
    {
        hadError = true;
        qDebug() << where << ":" << q.lastError();
        qInfo() << where << ":" << q.lastError();
    }
    return hadError;
}


class RowDefinition {
public:
    RowDefinition(const char *name, const char *definition)
        :
        name(name), definition(definition),
        found(false)
    {}
    const char *name;
    const char *definition;
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

class IndexDefinition {
public:
    IndexDefinition(const char *name, const char *definition, bool unique = false) :
        name(name), definition(definition), unique(unique)
    {}

    const char *name;
    const char *definition;
    bool unique;
};


void SongSettings::ensureIndex(IndexDefinition *index_definition)
{
    QSqlQuery q(m_db);
    exec("ensureIndex", q, "PRAGMA INDEX_INFO(" + QString(index_definition->name) + ")");
    bool found_any_fields = false;
    while (q.next())
    {
        found_any_fields = true;
    }
    if (!found_any_fields)
    {
        QString sql = "CREATE ";
        sql +=(index_definition->unique ? "UNIQUE " : "");
        sql += "INDEX ";
        sql += index_definition->name;
        sql += " ON ";
        sql += index_definition->definition;
        exec("ensureIndex: create", q, sql);
    }
}

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
        }
        alter += "\n)\n";

        alter_statements.insert(alter_statements.begin(), alter);
    }
    for (const auto& alter : alter_statements)
    {
        exec("ensureSchema", q, alter);
    }
}


RowDefinition song_rows[] =
{
    RowDefinition("filename", "text PRIMARY KEY"),
    RowDefinition("songname", "text"),
    RowDefinition("name", "text"),
    RowDefinition("pitch", "int"),
    RowDefinition("tempo", "int"),
    RowDefinition("tempoIsPercent", "int"),
    RowDefinition("volume", "int"),
    RowDefinition("introPos", "float"),
    RowDefinition("outroPos", "float"),
    RowDefinition("last_cuesheet", "text"),
    RowDefinition("songLength", "float"),
    RowDefinition("introOutroIsTimeBased", "int"),
    RowDefinition("treble", "int"),
    RowDefinition("bass", "int"),
    RowDefinition("midrange", "int"),
    RowDefinition("mix", "int"),
    RowDefinition("loop", "int"),   // 1 = yes, -1 = no, 0 = not set yet
    RowDefinition("tags", "text"),
    RowDefinition("replayGain", "float"),  // NULL = not set yet, else replayGain in dB

    // Adding a new per-song setting?  This is location 2 out of 6 to change.
    RowDefinition("VSTsettings", "text"),  // NULL = not set yet, else "LoudMax:123,456,789,123;OtherPlugin:a,b,c"

    RowDefinition(nullptr, nullptr), // NULL, NULL),
};

TableDefinition song_table("songs", song_rows);

RowDefinition session_rows[] =
{
    RowDefinition("name", "text"),
    RowDefinition("order_number", "INTEGER DEFAULT 0"),
    RowDefinition("deleted", "INTEGER DEFAULT 0"),
    RowDefinition("day_of_week", "INTEGER DEFAULT -1"),
    RowDefinition("start_minutes", "INTEGER DEFAULT 0"),
    RowDefinition(nullptr, nullptr), // NULL, NULL),
};
TableDefinition session_table("sessions", session_rows);

RowDefinition song_play_rows[] =
{
    RowDefinition("song_rowid", "int references songs(rowid)"),
    RowDefinition("session_rowid", "int references session(rowid)"),
    RowDefinition("played_on", "DATETIME DEFAULT CURRENT_TIMESTAMP"),
    RowDefinition(nullptr, nullptr), // NULL, NULL),
};
TableDefinition song_plays_table("song_plays", song_play_rows);


RowDefinition call_taught_on_rows[] =
{
    RowDefinition("dance_program", "TEXT"),
    RowDefinition("call_name", "TEXT"),
    RowDefinition("session_rowid", "INT REFERENCES session(rowid)"),
    RowDefinition("taught_on", "DATETIME DEFAULT CURRENT_TIMESTAMP"),
    RowDefinition(nullptr, nullptr), // NULL, NULL),
};
TableDefinition call_taught_on_table("call_taught_on", call_taught_on_rows);

RowDefinition tag_colors_rows[] =
{
    RowDefinition("tag", "TEXT PRIMARY KEY"),
    RowDefinition("background", "TEXT"),
    RowDefinition("foreground", "TEXT"),
    RowDefinition(nullptr, nullptr), // NULL, NULL),
};

TableDefinition tag_colors_table("tag_colors", tag_colors_rows);

IndexDefinition index_definitions[] = {
    IndexDefinition("songs_songname", "songs(songname)"),
    IndexDefinition("songs_name_idx","songs(name)"),
    IndexDefinition("session_name_idx", "sessions(name)", true),
    IndexDefinition("song_play_song_session_played_idx", "song_plays(song_rowid,session_rowid,played_on)"),
    IndexDefinition("call_taught_on_dance_program_call_name_session","call_taught_on(dance_program, call_name, session_rowid)")
};

// Markers ----------------
//  Any number of markers can be placed in a song, and the user can FF/REW to the next/previous one.
//  This makes playback of tapes, e.g. for C1, much simpler, because frequently we want to re-do the previous sequence, without
//    having to hunt for where that sequence began.
//  Markers can be created or deleted, but not moved or copied.
RowDefinition markers_rows[] =
{
    RowDefinition("song_rowid", "int references songs(rowid)"),
    RowDefinition("markerPos", "float"),  // 0.0 - 1.0 within a song, just like introPos/outroPos
    RowDefinition(nullptr, nullptr), // NULL, NULL),
};
TableDefinition markers_table("markers", markers_rows);

/*
"CREATE TABLE play_history (
 id int auto_increment primary key,
 song_rowid int REFERENCES songs(id),
 audience_id INT REFERENCES play_audiences(id)
 played_on TIMESTAMP,
    );
*/

SongSettings::SongSettings() :
    tagsBackgroundColorString(DEFAULTTAGSBACKGROUNDCOLOR),
    tagsForegroundColorString(DEFAULTTAGSFOREGROUNDCOLOR),
    databaseOpened(false),
    current_session_id(0), // NOTE: invalid current_session_id (this is supposed to be a row number in the sessions table)
    tagColorCacheSet(false)
{
}

void SongSettings::setDefaultTagColors( const QString &background, const QString & foreground)
{
    tagsBackgroundColorString = background;
    tagsForegroundColorString = foreground;
}


int SongSettings::currentSessionIDByTime()
{
    // find the first non-deleted row where order_number == 0
    int practiceID = 1; // wrong, if there are deleted rows in Sessions table
    QList<SessionInfo> sessions = getSessionInfo();
    for (const auto& s : sessions) {
        // qDebug() << s.day_of_week << s.id << s.name << s.order_number << s.start_minutes;
        if (s.order_number == 0) { // 0 is the first non-deleted row where order_number == 0
            // qDebug() << "Found it: " << s.name << "row:" << s.id;
            practiceID = s.id; // now it's right!
        }
    }

    // int session_id = 1;
    int session_id = practiceID;  // If we're in Automatic mode, and NOTHING matches, default to the first non-deleted session

    QDate date(QDate::currentDate());
    QTime time(QTime::currentTime());
    int day_of_week = date.dayOfWeek();
    int start_minutes = time.hour() * 60 + time.minute();
    QSqlQuery q(m_db);
    q.prepare("SELECT rowid FROM sessions WHERE day_of_week = :day_of_week AND start_minutes <= :start_minutes AND NOT deleted ORDER BY start_minutes DESC LIMIT 1");
    q.bindValue(":day_of_week", day_of_week);
    q.bindValue(":start_minutes", start_minutes);

    exec("currentSession", q);
    if (q.next())
    {
        session_id = q.value(0).toInt();
    }
    // q.prepare("SELECT rowid FROM sessions WHERE day_of_week = :day_of_week AND start_minutes < :start_minutes AND NOT deleted ORDER BY start_minutes DESC LIMIT 1");
    
    return session_id;
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
    nullptr
//    NULL
};

static const char database_type_name[] = "QSQLITE";
void SongSettings::openDatabase(const QString& path,
                                const QString& root_dir,
                                bool in_memory)
{
    closeDatabase();
    root_directories.clear();
    if (root_dir.length() > 0)
        root_directories.push_back(root_dir);

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
    ensureSchema(&tag_colors_table);
    ensureSchema(&markers_table);
    
    for (size_t i = 0; i < sizeof(index_definitions) / sizeof(*index_definitions); ++i)
    {
        ensureIndex(&index_definitions[i]);
    }
    {
        bool sessions_available = false;
        {
            QSqlQuery q(m_db);
            q.prepare("SELECT rowid FROM sessions WHERE NOT deleted");
            exec("openDatabase", q);
            while (q.next())
            {
                sessions_available = true;
            }
        }
        if (!sessions_available)
        {
            QSqlQuery q(m_db);
            q.prepare("INSERT INTO sessions(name,order_number,day_of_week) "
                      " VALUES(:name, :order_number, :day_of_week)");
            for (int id = 0; default_session_names[id]; ++id)
            {
                q.bindValue(":name",default_session_names[id]);
                q.bindValue(":order_number", id);
                q.bindValue(":day_of_week", id);
                exec("openDatabase", q);
            }
        }
    }
}

void SongSettings::setTagColors( const QHash<QString,QPair<QString,QString>> &colors)
{
    QHash<QString, QPair<QString,QString> > existingColors = getTagColors(false);
    
    tagColorCache = colors;
    tagColorCacheSet = true;
    {
        QSqlQuery q(m_db);
        q.prepare("BEGIN");
        exec("setTagColors BEGIN", q);
    }
    {
        QSqlQuery q(m_db);
        q.prepare("DELETE FROM tag_colors");
        exec("setTagColors DELETE", q);
    }
    
    {
        QSqlQuery q(m_db);
        q.prepare("INSERT INTO tag_colors(tag,background,foreground) VALUES (:tag,:background,:foreground)");
        for (auto color = colors.cbegin(); color != colors.cend(); ++color)
        {
            existingColors.remove(color.key());
                
            q.bindValue(":tag", color.key());
            q.bindValue(":background", color.value().first);
            q.bindValue(":foreground", color.value().second);
            exec("setTagColors INSERT", q);
        }
    }

    {
        QSqlQuery q(m_db);
        q.prepare("COMMIT");
        exec("setTagColors COMMIT", q);
    }

    {
        QHash<int, QString> updates;
        
        QSqlQuery q(m_db);
        q.prepare("SELECT rowid, tags FROM songs");
        exec("Tag cleanup", q);
        while (q.next())
        {
            int rowid = q.value(0).toInt();
            QString tagstr = q.value(1).toString();
            QStringList tags = tagstr.split(" ");
            bool changed(false);
            for (int i = 0; i < tags.size(); ++i)
            {
                QString tag = tags[i];
                if (existingColors.contains(tag))
                {
                    changed = true;
                    tags.removeAt(i);
                    --i;
                }
            }
            if (changed)
                updates[rowid] = tags.join(" ");
        }
        QSqlQuery qupdate(m_db);
        qupdate.prepare("UPDATE songs SET tags=:tags WHERE rowid=:rowid");
        for (auto update = updates.cbegin(); update != updates.cend(); ++update)
        {
            qupdate.bindValue(":tags", update.value());
            qupdate.bindValue(":rowid", update.key());
            exec("Tag cleanup update", qupdate);
        }
    }
}


void SongSettings::addTags(const QString &str)
{
    QStringList tags = str.split(" ");
    for (const auto& tag : tags)
    {
        if (tagCounts.contains(tag))
        {
            tagCounts[tag] = tagCounts[tag] + 1;
        }
        else
        {
            tagCounts[tag] = 1;
        }
          
    }
}
void SongSettings::removeTags(const QString &str)
{
    QStringList tags = str.split(" ");
    for (const auto& tag : tags)
    {
        if (tagCounts.contains(tag))
        {
            tagCounts[tag] = tagCounts[tag] - 1;
            if (tagCounts[tag] == 0)
                tagCounts.remove(tag);
        }
        else
        {
//            qDebug() << "Trying to remove nonexistent tag " << tag;
        }
    }
}


QPair<QString,QString> SongSettings::getColorForTag(const QString &tag)
{
    if (!tagColorCacheSet)
        getTagColors();
    if (tagColorCache.contains(tag))
        return tagColorCache[tag];
    return QPair<QString,QString>(tagsBackgroundColorString, tagsForegroundColorString);
}


QHash<QString,QPair<QString,QString>> SongSettings::getTagColors(bool loadCache)
{
    QHash<QString, QPair<QString,QString>> colors;
    QSqlQuery q(m_db);
    exec("getTagColors", q, "SELECT tag,background, foreground FROM tag_colors");
    while (q.next())
    {
        colors[q.value(0).toString()] = QPair<QString,QString>(q.value(1).toString(),q.value(2).toString());
    }

    for (auto tag = tagCounts.cbegin(); tag != tagCounts.cend(); ++tag)
    {
        if (tag.value() > 0 && !colors.contains(tag.key()))
        {
            if (tag.key().trimmed().length() > 0)
            {
                colors[tag.key()] = QPair<QString,QString>(tagsBackgroundColorString, tagsForegroundColorString);
            }
        }
    }

    // do this even if we aren't caching.
    if (colors.empty())
    {
        colors["irish"] = QPair<QString,QString>("#44dd44", "#004400");
        colors["xmas"]  = QPair<QString,QString>("#008800", "#ffaaaa");
        colors["teach"] = QPair<QString,QString>("#d9a6e3","#000000");
        colors["closer"] = QPair<QString,QString>("#0000ff","#ffffff");
        colors["Latin"] = QPair<QString,QString>("#099f9d","#bebf23");
        colors["HOT"] = QPair<QString,QString>("#ff0000","#000000");
        colors["Swing"] = QPair<QString,QString>("#ffdd00","#0000ff");
        colors["Bollywood"] = QPair<QString,QString>("#ff8b00","#000000");
        colors["halloween"] = QPair<QString,QString>("#ff8b00","#00ff00");
        colors["DUET"] = QPair<QString,QString>("#ae8eff","#781e19");

        colors["NEW"] = QPair<QString,QString>("#ffffff","#c64434"); // added to default set for Import and Organize

        for (const auto& color : colors)
        {
            tagCounts[color.first] = 1;
        }
        setTagColors(colors);
    }

    // NOTE: the NEW tag is now required, since it's used by Import and Organize.
    //    If the NEW tag is NOT there, then Import and Organize will silently fail to set the NEW tag
    //    So, let's make sure it is there...
    if (!colors.contains("NEW")) {
        colors["NEW"] = QPair<QString,QString>("#ffffff","#c64434");
        tagCounts["NEW"] = 1;
        setTagColors(colors);  // and remember them all
    }
    
    if (loadCache)
    {
        tagColorCache = colors;
        tagColorCacheSet = true;
    }
    
    return colors;
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


QString SongSettings::primaryRootDir()
{
    return root_directories[0];
}

QString SongSettings::removeRootDirs(const QString &filenameWithPath)
{
    QString filenameWithPathNormalized(filenameWithPath);
    for (const auto& root_dir : root_directories)
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


void SongSettings::getSongPlayHistory(SongPlayEvent &event,
                            int session_id,
                            bool omitStartDate,
                            QString startDate,
                            bool omitEndDate,
                            QString endDate)
{
    QString sql("SELECT name, played_on, datetime(played_on,'localtime'), filename, pitch, tempo, last_cuesheet FROM songs JOIN song_plays ON song_plays.song_rowid=songs.rowid");
    QStringList whereClause;
    
    if (session_id)
    {
        whereClause.append("session_rowid=:session_rowid");
    }
    if (!omitStartDate)
    {
        whereClause.append("played_on > :start_date");
    }
    if (!omitEndDate)
    {
        whereClause.append("played_on < :end_date");
    }
    if (!whereClause.empty())
    {
        sql += " WHERE " + whereClause.join(" AND ");
    }
    QSqlQuery q(m_db);
    q.prepare(sql);
    if (session_id)
    {
        q.bindValue(":session_rowid", session_id);
    }
    if (!omitStartDate)
    {
        q.bindValue(":start_date", startDate);
    }
    if (!omitEndDate)
    {
        q.bindValue(":end_date", endDate);
    }
    exec("songplayhistory", q);
    while (q.next())
    {
        event(q.value(0).toString(), // name
              q.value(1).toString(), // UTC time
              q.value(2).toString(), // local time
              q.value(3).toString(), // filename
              q.value(4).toString(), // pitch (NOTE: current pitch, not necessarily the pitch at playback time - FIX)
              q.value(5).toString(), // tempo (NOTE: current tempo, not necessarily the tempo at playback time - FIX)
              removeRootDirs(q.value(6).toString())  // last_cuesheet (NOTE: current last_cuesheet, not necessarily the last_cuesheet at playback time - FIX)
              );
    }
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
//        int age = q.value(1).toInt();
//        QString str(QString("%1").arg(age, 3));
        QString str = q.value(1).toString();  // leave it as a float string
        ages[q.value(0).toString()] = str;
    }
}

QString SongSettings::getSongAge(const QString &filename, const QString &filenameWithPath, bool show_all_sessions)
{
    Q_UNUSED(filename)
    Q_UNUSED(filenameWithPath)
    Q_UNUSED(show_all_sessions)

    // return(QString("999")); // I don't think the return value is actually used anywhere nowadays, so let's avoid making 2 sqlite queries per songTable entry
    //                         // Using "999" in case this shows up somewhere.  (So far, I don't see any...)
#if 1
    QString filenameWithPathNormalized = removeRootDirs(filenameWithPath);
    // qDebug() << "getSongAge" << filename << filenameWithPath << show_all_sessions << filenameWithPathNormalized;
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
//            int age = q.value(0).toInt();
//            QString str(QString("%1").arg(age, 3));
            QString str = q.value(0).toString();  // leave it as a float string
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
//            int age = q.value(0).toInt();
//            QString str(QString("%1").arg(age, 3));
            QString str = q.value(0).toString();  // leave it as a float string
            // qDebug() << "found it 2" << str;
            return str;
        }
    }
    // qDebug() << "    nope. did not find it 3";
    return QString("");
#endif
}


SongSetting::SongSetting()
    :
#define SONGSETTING_ELEMENT(type,name) m_##name(), set_##name(false),
#include "songsetting_attributes.h"
#undef SONGSETTING_ELEMENT
    // handle the trailing comma problem.
    dummy(false)
{
    dummy = false;  // remove Mac OS X compiler warning (it doesn't realize that the initializer above DOES use dummy)
}

QDebug operator<<(QDebug dbg, const SongSetting &setting)
{
    QDebugStateSaver stateSaver(dbg);
    dbg.nospace() << "SongSetting(" <<
#define SONGSETTING_ELEMENT(type, name) "\n    " #name ": " << setting.m_##name << "," << setting.set_##name << ";" <<
#include "songsetting_attributes.h"
#undef SONGSETTING_ELEMENT
                     ")";
    return dbg;
}


void SongSettings::saveSettings(const QString &filenameWithPath,
                                const SongSetting &settings)
{
    QString filenameWithPathNormalized = removeRootDirs(filenameWithPath);
    int id = getSongIDFromFilename(settings.getFilename(), filenameWithPathNormalized);

//    qDebug() << "saveSettings: id = " << id;

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
    if (settings.isSetTreble()) { fields.append("treble"); }
    if (settings.isSetBass()) { fields.append("bass"); }
    if (settings.isSetMidrange()) { fields.append("midrange"); }
    if (settings.isSetMix()) { fields.append("mix"); }
    if (settings.isSetLoop()) { fields.append("loop"); }
    if (settings.isSetTags()) { fields.append("tags"); }
//    if (settings.isSetReplayGain()) { fields.append("replayGain"); }

    // Adding a new per-song setting?  This is location 3 out of 6 to change.
    if (settings.isSetVSTsettings()) { fields.append("vstSettings"); }

//    qDebug() << "saveSettings: " << fields;

    QSqlQuery q(m_db);
    if (id == -1)
    {
        fields.append("filename");
        QString sql("INSERT INTO songs(");
        {
            bool first = true;
            for (const auto& s : fields)
            {
                if (!first) { sql += ","; }
                first = false;
                sql += s;
            }
        }
        sql += ") VALUES (";
        {
            bool first = true;
            for (const auto& s : fields)
            {
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
            bool first = true;
            for (const auto& s : fields)
            {
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

    QString revisedSongName = settings.getSongname();
    revisedSongName = revisedSongName.replace(QRegularExpression(" \\[L:.*\\]$"), ""); // for safety, remove the " [L: level]"
    // q.bindValue(":name", settings.getSongname() );
    q.bindValue(":name", revisedSongName ); // I don't think the :name is used anymore, but just being safe here.

    q.bindValue(":last_cuesheet", settings.getCuesheetName() );
    q.bindValue(":songLength", settings.getSongLength() );
    q.bindValue(":introOutroIsTimeBased", settings.getIntroOutroIsTimeBased() );
    q.bindValue(":treble", settings.getTreble());
    q.bindValue(":bass", settings.getBass());
    q.bindValue(":midrange", settings.getMidrange());
    q.bindValue(":mix", settings.getMix());
    q.bindValue(":loop", settings.getLoop());
    q.bindValue(":tags", settings.getTags());
//    q.bindValue(":replayGain", settings.getReplayGain());

    // Adding a new per-song setting?  This is location 4 out of 6 to change.
    q.bindValue(":vstSettings", settings.getVSTsettings());

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

    if (!q.value(10).isNull()) { settings.setTreble(q.value(10).toInt()); }
    if (!q.value(11).isNull()) { settings.setBass(q.value(11).toInt()); }
    if (!q.value(12).isNull()) { settings.setMidrange(q.value(12).toInt()); }
    if (!q.value(13).isNull()) { settings.setMix(q.value(13).toInt()); }
    if (!q.value(14).isNull()) { settings.setLoop(q.value(14).toInt()); }
    if (!q.value(15).isNull()) { settings.setTags(q.value(15).toString()); }
//    if (!q.value(16).isNull()) { settings.setReplayGain(q.value(16).toFloat()); }

    // Adding a new per-song setting?  This is location 5 out of 6 to change.
    if (!q.value(16).isNull()) { settings.setVSTsettings(q.value(16).toString()); }
}

bool SongSettings::loadSettings(const QString &filenameWithPath,
                                SongSetting &settings)
{
//    QString baseSql = "SELECT filename, pitch, tempo, introPos, outroPos, volume, last_cuesheet,tempoIsPercent,songLength,introOutroIsTimeBased, treble, bass, midrange, mix, loop, tags, replayGain FROM songs WHERE ";
    // Adding a new per-song setting?  This is location 6 out of 6 to change.
    QString baseSql = "SELECT filename, pitch, tempo, introPos, outroPos, volume, last_cuesheet,tempoIsPercent,songLength,introOutroIsTimeBased, treble, bass, midrange, mix, loop, tags, VSTsettings FROM songs WHERE ";
    QString filenameWithPathNormalized = removeRootDirs(filenameWithPath);

//    qDebug() << "********* DEBUG get/setSongMarkers **********";

//    QMap<float,int> markers;
//    qDebug() << "0. No Markers for: " << filenameWithPathNormalized << " are " << markers;

//    getSongMarkers(filenameWithPathNormalized, markers);
//    qDebug() << "1. Markers found for: " << filenameWithPathNormalized << " are " << markers;

//    markers.insert(0.6, 1);
//    markers.insert(0.26, 1); // note out of order
//    markers.insert(0.16, 1); // note out of order
//    qDebug() << "2. Markers after insertion for: " << filenameWithPathNormalized << " are " << markers;

//    setSongMarkers(filenameWithPathNormalized, markers);
//    qDebug() << "3. Markers after set for: " << filenameWithPathNormalized << " are " << markers;

//    getSongMarkers(filenameWithPathNormalized, markers);
//    qDebug() << "4. Updated Markers after get for: " << filenameWithPathNormalized << " are " << markers;

//    addMarker(0.4, markers);
//    addMarker(0.2, markers);
//    addMarker(0.3, markers);
//    qDebug() << "5. Markers after addMarkers: " << markers;

//    deleteNearbyMarker(0.4, markers);
//    qDebug() << "6. Markers after deleteNearbyMarker: " << markers;

//    deleteNearbyMarker(0.2005, markers);
//    qDebug() << "7. Markers after deleteNearbyMarker: " << markers;

//    qDebug() << "********* END DEBUG get/setSongMarkers **********";

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
        q.prepare(baseSql + " filename=:filename");
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
        q.prepare(baseSql + " name=:name");

        QString revisedSongName = settings.getSongname();
        revisedSongName = revisedSongName.replace(QRegularExpression(" \\[L:.*\\]$"), ""); // for safety, remove the " [L: level]"
        // q.bindValue(":name", settings.getSongname());
        q.bindValue(":name", revisedSongName);  // I don't think the :name is used anymore, but just being safe here.

        exec("loadSettings", q);
        while (q.next())
        {
            foundResults = true;
            setSongSettingFromSQLQuery(q, settings);
        }
    }
    if (foundResults && settings.isSetTags() && !settings.getTags().isNull())
    {
        addTags(settings.getTags());
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


QList<SessionInfo> SongSettings::getSessionInfo()
{
    QList<SessionInfo> sessions;
    
    QString sql = "SELECT name, order_number, rowid, day_of_week, start_minutes FROM sessions WHERE NOT deleted ORDER BY order_number, rowid";
    QSqlQuery q(m_db);
    q.prepare(sql);
    exec("getSessionInfo", q);
    
    while (q.next())
    {
        SessionInfo session;
        session.name = q.value(0).toString();
        session.order_number = q.value(1).toInt();
        session.id = q.value(2).toInt();
        session.day_of_week = q.value(3).toInt();
        if (session.day_of_week < 0)
            session.day_of_week = session.id - 1;
        session.start_minutes = q.value(4).toInt();
        sessions.append(session);
    }
    return sessions;
}

void SongSettings::setSessionInfo(const QList<SessionInfo> &sessions)
{
    QList<SessionInfo> currentSessions(getSessionInfo());
    QHash<int, SessionInfo> sessionsById;
    QHash<QString, SessionInfo> sessionsByName;

    for (auto session : currentSessions)
    {
        sessionsById[session.id] = session;
        sessionsByName[session.name] = session;
    }
    
    {
        QSqlQuery q(m_db);
        q.prepare("BEGIN");
        exec("SessionInfo BEGIN", q);
    }

    {
        QSqlQuery q(m_db);
        exec("SessionInfo DELETE", q, "UPDATE sessions SET deleted = 1,name = 'GarbageValueForUniquifier' || rowid");
    }

    {
        QSqlQuery q_insert(m_db);
        QSqlQuery q_update(m_db);
        q_insert.prepare("INSERT INTO sessions(deleted,name,order_number,day_of_week,start_minutes) "
                         " VALUES (0, :name, :order_number, :day_of_week, :start_minutes)");
        q_update.prepare("UPDATE sessions SET deleted=0,name=:name,order_number=:order_number, "
                         " day_of_week=:day_of_week,start_minutes=:start_minutes"
                         " WHERE rowid=:id");
        for (const auto& session : sessions)
        {
            QSqlQuery *pq;
            if (session.id > 0)
            {
                pq = &q_update; 
                pq->bindValue(":id", session.id);
            }
            else
            {
                pq = &q_insert;
            }
            pq->bindValue(":name", session.name);
            pq->bindValue(":order_number", session.order_number);
            pq->bindValue(":day_of_week", session.day_of_week);
            pq->bindValue(":start_minutes", session.start_minutes);
            exec("SessionInfo UPDATE/INSERT", *pq);
        }
    }


    {
        QSqlQuery q(m_db);
        q.prepare("COMMIT");
        exec("SessionInfo COMMIT", q);
    }
}

// MARKERS ---------------
// TODO: typedef the QMap to "markerSet"
void SongSettings::getSongMarkers(const QString &filename, QMap<float,int> &markers)
{
    markers.clear(); // clear out whatever was in the QMap before
    int song_rowid = getSongIDFromFilenameAlone(filename);

    QSqlQuery q(m_db);
    q.prepare("SELECT markerPos FROM markers WHERE song_rowid=:song_rowid");
    q.bindValue(":song_rowid", song_rowid);
    exec("getSongMarkers", q);

    while (q.next())
    {
        float markerPos = q.value(0).toFloat();
//        qDebug() << "MARKER for: " << filename << " = " << markerPos;

        if (markerPos >= 0.0 && markerPos <= 1.0) {  // a little bit of error checking, to prevent weird corruption
            // only allow getting of valid markerPositions
            markers[markerPos] = 0; // keys are automatically kept sorted, but values (0) are ignored
        }
    }
}

void SongSettings::setSongMarkers(const QString &filename, const QMap<float,int> &markers)
{
    int song_rowid = getSongIDFromFilenameAlone(filename);

    {
        // start transaction -------
        QSqlQuery q(m_db);
        q.prepare("BEGIN");
        exec("setSongMarkers BEGIN", q);
    }

    {
        // delete old markers for this song -------
        QSqlQuery q(m_db);
        q.prepare("DELETE FROM markers WHERE song_rowid=:song_rowid");
        q.bindValue(":song_rowid", song_rowid);
        exec("setSongMarkers DELETE", q);
    }

    {
        // insert new markers for this song --------
        QSqlQuery q(m_db);
        q.prepare("INSERT INTO markers(song_rowid, markerPos) VALUES (:song_rowid,:markerPos)");
        for (QMap<float, int>::const_iterator markerPosition = markers.cbegin(); markerPosition != markers.cend(); ++markerPosition)
        {
            if (markerPosition.key() >= 0.0 && markerPosition.key() <= 1.0) {  // a little bit of error checking, to prevent weird corruption
                // only allow storing of valid markerPositions into DB
                q.bindValue(":song_rowid", song_rowid);             // key is song's rowid
                q.bindValue(":markerPos", markerPosition.key());    // key in QMap is the markerPosition, QMap value is ignored
                exec("setSongMarkers INSERT", q);
            }
        }
    }

    {
        // end transaction --------
        QSqlQuery q(m_db);
        q.prepare("COMMIT");
        exec("setSongMarkers COMMIT", q);
    }
}

// add a marker position to a set of markers
void SongSettings::addMarker(const float markerPos, QMap<float,int> &markers)
{
    markers[markerPos] = 1;  // value is ignored
}

const float markerTolerance = 0.001;  // remember: this is a percentage of the song
                                      // typical song = 5 min = 300 sec, so 0.001 * 300 = 0.3sec either side

// what is the exact marker position of the nearest marker to markerPos (within tolerance)
float SongSettings::getNearbyMarker(const float markerPos, QMap<float,int> &markers)
{
    for (QMap<float, int>::iterator markerPosition = markers.begin(); markerPosition != markers.end(); ++markerPosition)
    {
        if (abs(markerPos - markerPosition.key()) <= markerTolerance ) {
            return(markerPosition.key());
        }
    }
    return(-1.0);  // no nearby marker found
}

// delete a marker that is "close enough to" (within tolerance of) a marker in the set
//   This is so we can delete markers via the GUI, later on...
//   It also takes care of the tolerance problem with matching floats exactly.
//   Passing in exact values is fine, too -- it will do the Right Thing.
void SongSettings::deleteNearbyMarker(const float markerPos, QMap<float,int> &markers)
{
    // Idiom: https://stackoverflow.com/questions/11689465/how-to-remove-values-from-a-qmap
    for (QMap<float, int>::iterator markerPosition = markers.begin(); markerPosition != markers.end(); /* no increment here */)
    {
        if (abs(markerPos - markerPosition.key()) <= markerTolerance ) {
            // it's close enough, so get rid of this one.
            markerPosition = markers.erase(markerPosition); // erase this one, and point at next one
        } else {
            // this is not the droid you're looking for...
            ++markerPosition; // normal increment
        }
    }
}

