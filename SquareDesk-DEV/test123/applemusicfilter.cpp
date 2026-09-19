/****************************************************************************
**
** Copyright (C) 2016-2026 Mike Pogue, Dan Lyke
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

#include "applemusicfilter.h"
#include "prefsmanager.h"
#include "playlist_constants.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

const AppleMusicNamedItem appleMusicFields[] = {
    { "genre",       "Genre"        },
    { "grouping",    "Grouping"     },
    { "album",       "Album"        },
    { "albumArtist", "Album Artist" },
    { "artist",      "Artist"       },
    { "composer",    "Composer"     },
    { "comments",    "Comments"     },
    { "work",        "Work"         },
    { "title",       "Title"        },
};
const int numAppleMusicFields = sizeof(appleMusicFields)/sizeof(appleMusicFields[0]);

const AppleMusicNamedItem appleMusicOperators[] = {
    { "is",          "is"                },
    { "isNot",       "is not"            },
    { "contains",    "contains"          },
    { "notContains", "does not contain"  },
    { "startsWith",  "starts with"       },
    { "endsWith",    "ends with"         },
    { "matches",     "matches (wildcard)"},
    { "notEmpty",    "is not empty"      },
};
const int numAppleMusicOperators = sizeof(appleMusicOperators)/sizeof(appleMusicOperators[0]);

QString appleMusicFieldValue(const AppleMusicTrackMeta &track, const QString &fieldKey)
{
    if (fieldKey == "genre")       return QString::fromStdString(track.genre);
    if (fieldKey == "grouping")    return QString::fromStdString(track.grouping);
    if (fieldKey == "album")       return QString::fromStdString(track.album);
    if (fieldKey == "albumArtist") return QString::fromStdString(track.albumArtist);
    if (fieldKey == "artist")      return QString::fromStdString(track.artist);
    if (fieldKey == "composer")    return QString::fromStdString(track.composer);
    if (fieldKey == "comments")    return QString::fromStdString(track.comments);
    if (fieldKey == "work")        return QString::fromStdString(track.work);
    if (fieldKey == "title")       return QString::fromStdString(track.title);
    return QString();
}

QString appleMusicFieldDisplay(const QString &fieldKey)
{
    for (int i = 0; i < numAppleMusicFields; ++i) {
        if (fieldKey == appleMusicFields[i].key) {
            return appleMusicFields[i].display;
        }
    }
    return fieldKey;
}

bool appleMusicRuleMatches(const QString &value, const QString &opKey, const QString &arg)
{
    if (opKey == "notEmpty")    return !value.trimmed().isEmpty();
    if (opKey == "is")          return value.compare(arg, Qt::CaseInsensitive) == 0;
    if (opKey == "isNot")       return value.compare(arg, Qt::CaseInsensitive) != 0;
    if (opKey == "contains")    return value.contains(arg, Qt::CaseInsensitive);
    if (opKey == "notContains") return !value.contains(arg, Qt::CaseInsensitive);
    if (opKey == "startsWith")  return value.startsWith(arg, Qt::CaseInsensitive);
    if (opKey == "endsWith")    return value.endsWith(arg, Qt::CaseInsensitive);
    if (opKey == "matches") {
        QRegularExpression re = QRegularExpression::fromWildcard(arg, Qt::CaseInsensitive);
        return re.match(value).hasMatch();
    }
    return false;
}

// One entry of a semicolon-separated Type list, e.g. "hoedown;patter;SD *".  Wildcards are
//   allowed here too, so a user whose Groupings are "SD-Patter-1".."SD-Patter-9" can write
//   "SD-Patter-*".
bool appleMusicValueInList(const QString &value, const QString &semicolonList)
{
    const QStringList entries = semicolonList.split(';', Qt::SkipEmptyParts);
    for (const QString &entryRaw : entries) {
        QString entry = entryRaw.trimmed();
        if (entry.isEmpty()) {
            continue;
        }
        if (entry.contains('*') || entry.contains('?')) {
            if (QRegularExpression::fromWildcard(entry, Qt::CaseInsensitive).match(value).hasMatch()) {
                return true;
            }
        } else if (value.compare(entry, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

// "hoedown;;patter" (or a leading ";") is what's left behind when a value is deleted from the
//   middle of one of the Type lists.  The list parser skips empty entries anyway, but leaving
//   them in the field looks broken.
QString appleMusicCollapseSemicolons(QString text)
{
    static const QRegularExpression runOfSemicolons(";{2,}");
    text.replace(runOfSemicolons, ";");
    while (text.startsWith(';')) {
        text.remove(0, 1);
    }
    return text;
}

bool appleMusicIsPlayableFormat(const QString &absolutePath)
{
    static const QRegularExpression playable(SUPPORTED_AUDIO_EXTENSIONS_REGEX,
                                             QRegularExpression::CaseInsensitiveOption);
    return absolutePath.contains(playable);
}

AppleMusicTrackMeta appleMusicMetaOf(const PlaylistTrack &track)
{
    AppleMusicTrackMeta meta;
    meta.absolutePath = track.absolutePath;
    meta.title       = track.title;
    meta.artist      = track.artist;
    meta.albumArtist = track.albumArtist;
    meta.album       = track.album;
    meta.genre       = track.genre;
    meta.grouping    = track.grouping;
    meta.composer    = track.composer;
    meta.comments    = track.comments;
    meta.work        = track.work;
    meta.year        = track.year;
    meta.totalTimeMS = track.totalTimeMS;
    meta.rating         = track.rating;          // was imported but dropped here until #1744
    meta.ratingComputed = track.ratingComputed;
    meta.addedDate      = track.addedDate;
    meta.lastPlayedDate = track.lastPlayedDate;
    return meta;
}

// -------------------------------------------------------------------
void AppleMusicFilter::loadFrom(PreferencesManager &prefs)
{
    filterEnabled          = prefs.GetappleMusicFilterEnabled();
    matchAll               = (prefs.GetappleMusicFilterMatch() == 0);   // 0 = all, 1 = any
    appliesInsidePlaylists = prefs.GetappleMusicFilterAppliesInsidePlaylists();
    rules                  = rulesFromJSON(prefs.GetappleMusicFilterRules());

    // 0 is "Nothing -- use the default Type below"; 1..N index appleMusicFields
    const int typeFieldIndex = prefs.GetappleMusicTypeField();
    typeFieldKey = (typeFieldIndex > 0 && typeFieldIndex <= numAppleMusicFields)
                       ? appleMusicFields[typeFieldIndex - 1].key
                       : QString();

    typePatter       = prefs.GetappleMusicTypePatter();
    typeSinging      = prefs.GetappleMusicTypeSinging();
    typeCalled       = prefs.GetappleMusicTypeCalled();
    typeExtras       = prefs.GetappleMusicTypeExtras();
    typeDefault      = prefs.GetappleMusicTypeDefault();
    typeColumnFormat = prefs.GetappleMusicTypeColumnFormat();
}

bool AppleMusicFilter::passes(const AppleMusicTrackMeta &track) const
{
    if (!filterEnabled) {
        return true;
    }

    int ruleCount = 0;
    for (const AppleMusicRule &rule : rules) {
        const QString arg = rule.value.trimmed();
        if (arg.isEmpty() && rule.opKey != "notEmpty") {
            continue;  // a half-typed rule shouldn't suddenly match (or reject) everything
        }
        ruleCount++;

        const bool matched = appleMusicRuleMatches(appleMusicFieldValue(track, rule.fieldKey),
                                                   rule.opKey, arg);
        if (matchAll && !matched) {
            return false;
        }
        if (!matchAll && matched) {
            return true;
        }
    }

    if (ruleCount == 0) {
        return true;  // nothing filled in yet: don't pretend the library is empty
    }
    return matchAll;
}

QString AppleMusicFilter::typeOf(const AppleMusicTrackMeta &track) const
{
    if (typeComesFromMetadata()) {
        const QString value = appleMusicFieldValue(track, typeFieldKey);
        if (appleMusicValueInList(value, typePatter))  return "patter";
        if (appleMusicValueInList(value, typeSinging)) return "singing";
        if (appleMusicValueInList(value, typeCalled))  return "called";
        if (appleMusicValueInList(value, typeExtras))  return "extras";
    }

    switch (typeDefault) {
        case AppleMusicDefaultPatter:  return "patter";
        case AppleMusicDefaultSinging: return "singing";
        case AppleMusicDefaultCalled:  return "called";
        case AppleMusicDefaultExtras:  return "extras";
        default:                       return "";      // AppleMusicDefaultSkip
    }
}

QList<AppleMusicRule> AppleMusicFilter::rulesFromJSON(const QString &json)
{
    QList<AppleMusicRule> result;
    const QJsonArray array = QJsonDocument::fromJson(json.toUtf8()).array();
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        result.append({ object["field"].toString("genre"),
                        object["op"].toString("is"),
                        object["value"].toString() });
    }
    return result;
}

QString AppleMusicFilter::rulesToJSON(const QList<AppleMusicRule> &rules)
{
    QJsonArray array;
    for (const AppleMusicRule &rule : rules) {
        QJsonObject object;
        object["field"] = rule.fieldKey;
        object["op"]    = rule.opKey;
        object["value"] = rule.value;
        array.append(object);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}
