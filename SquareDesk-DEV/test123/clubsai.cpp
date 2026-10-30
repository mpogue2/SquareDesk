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

#include "clubsai.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRegularExpression>
#include <QTextStream>
#include <optional>

static const char *kEventsHeading = "## The events";   // also how isAIExportText() knows the export

static const char *kDayNames[] = { "", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

ClubSuggesterParams aiEventParams()
{
    ClubSuggesterParams params;
    params.minEventPlays = 3;
    params.minEventMinutes = 20;
    return params;
}

// ---------------------------------------------------------------------------------------------
// Writing

static QString hhmm(int minutes)
{
    minutes = ((minutes % 1440) + 1440) % 1440;
    return QString("%1:%2").arg(minutes / 60, 2, 10, QChar('0')).arg(minutes % 60, 2, 10, QChar('0'));
}

static QJsonObject ruleJson(const ClubRule &r)
{
    QJsonObject o;
    static const char *kinds[] = { "weekly", "every_other_week", "monthly", "once" };
    o["kind"] = kinds[std::clamp(r.kind, 0, 3)];
    if (r.kind == ClubRule::Once) {
        o["date"] = r.since.toString(Qt::ISODate);
    } else {
        o["weekday"] = kDayNames[std::clamp(r.weekday, 1, 7)];
        if (r.kind == ClubRule::Monthly) {
            o["nth"] = (r.nth == -1) ? QJsonValue("last") : QJsonValue(r.nth);
        }
    }
    o["start"] = hhmm(r.start_minutes);
    o["end"] = hhmm(r.end_minutes);
    if (r.kind != ClubRule::Once) {
        o["since"] = r.since.toString(Qt::ISODate);
        o["until"] = r.until.isValid() ? QJsonValue(r.until.toString(Qt::ISODate)) : QJsonValue(QJsonValue::Null);
    }
    if (!r.label.trimmed().isEmpty()) {
        o["label"] = r.label.trimmed();
    }
    return o;
}

// One rule per line, in the order the example in the export shows them, so that a person (or an
//   AI) can read it; QJsonDocument would put every field on a line of its own, sorted
static QString ruleLine(const ClubRule &r)
{
    const QJsonObject o = ruleJson(r);
    QStringList fields;
    for (const char *key : { "kind", "weekday", "nth", "date", "start", "end", "since", "until", "label" }) {
        if (!o.contains(key)) {
            continue;
        }
        const QJsonValue v = o.value(key);
        const QString value = v.isNull()   ? QString("null")
                            : v.isDouble() ? QString::number(v.toInt())
                                           : QString::fromUtf8(QJsonDocument(QJsonArray{ v }).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
        fields << QString("\"%1\": %2").arg(key, value);
    }
    return "{ " + fields.join(", ") + " }";
}

static QString quoted(const QString &s)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray{ s }).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
}

QString clubsJsonText(const QList<ClubInfo> &clubs)
{
    QStringList clubTexts;
    for (const ClubInfo &club : clubs) {
        QStringList rules;
        for (const ClubRule &r : club.rules) {
            rules << "        " + ruleLine(r);
        }
        clubTexts << QString("    {\n      \"name\": %1,\n      \"color\": %2,\n      \"rules\": [\n%3\n      ]\n    }")
                         .arg(quoted(club.name), quoted(club.color), rules.join(",\n"));
    }
    return "{\n  \"squaredesk_clubs\": 1,\n  \"clubs\": [\n" + clubTexts.join(",\n") + "\n  ]\n}\n";
}

QString clubsAIExportText(const QList<QDateTime> &playsUtc, const QList<ClubInfo> &clubs, const QDate &asOf)
{
    const QList<SuggestedEvent> events = eventsFromPlays(playsUtc, aiEventParams());

    QString text;
    QTextStream out(&text);
    out << "# My square dance play history, for working out club schedules\n\n"
        << "I'm a square dance caller. I play music at dances with an app called SquareDesk, which"
           " keeps track of which club each dance belongs to, using a schedule for each club (\"every"
           " Wednesday from 6:00 PM to 9:30 PM\"). Please help me work out those schedules from my play"
           " history.\n\n"
        << "Below is every event in my SquareDesk history: each stretch of time when I played music,"
           " worked out from when songs were played. For each one: the date, the day of the week, when"
           " the first and last songs were played, and how many songs. Today is " << asOf.toString(Qt::ISODate) << ".\n\n"
        << "Please find the regular clubs (the same day of the week and time, week after week, every"
           " other week, or on the same week of each month), the classes (a short run of weekly events),"
           " and the one-off dances worth keeping, and suggest a schedule for each club.\n\n"
        << "Things that help:\n"
        << "- Most dances start between 6:30 PM and 7:30 PM and last 2.5 to 3.5 hours. Daytime events"
           " are often classes or workshops.\n"
        << "- Classes and dances start on time, week after week. Events whose start time wanders, short"
           " events, and events that start late (after 9:30 PM) are usually practice at home: leave them"
           " out.\n"
        << "- Songs played a few hours before a club's event, on the same day, are usually practice for"
           " it: leave them out too.\n"
        << "- A club can skip weeks (holidays, summer) and still be weekly. A club that met for a while and"
           " stopped gets an end date.\n"
        << "- A schedule's times should cover the dance, from a little before its first song to a little"
           " after its last, but not the practice before it.\n"
        << "- One-off dances can be listed as one-off events of the club that held them, or left out.\n";
    if (!clubs.isEmpty()) {
        out << "- My clubs so far are listed below. Add to them (by using the same name) rather than"
               " repeating their schedules.\n";
    }
    out << "\n"
        << "Ask me about anything you're not sure of, such as what a group of events was. When we're"
           " done, give me your schedules as JSON, in exactly this format (in a ```json code block), so"
           " that SquareDesk can read them:\n\n"
        << "```json\n"
        << "{\n"
        << "  \"squaredesk_clubs\": 1,\n"
        << "  \"clubs\": [\n"
        << "    {\n"
        << "      \"name\": \"Jokers\",\n"
        << "      \"rules\": [\n"
        << "        { \"kind\": \"weekly\", \"weekday\": \"Wed\", \"start\": \"18:00\", \"end\": \"21:30\","
           " \"since\": \"2019-10-02\", \"until\": null },\n"
        << "        { \"kind\": \"weekly\", \"weekday\": \"Sat\", \"start\": \"12:45\", \"end\": \"16:00\","
           " \"since\": \"2026-01-31\", \"until\": \"2026-03-28\", \"label\": \"SSD class\" },\n"
        << "        { \"kind\": \"once\", \"date\": \"2022-09-24\", \"start\": \"19:15\", \"end\": \"22:00\","
           " \"label\": \"Fall dance\" }\n"
        << "      ]\n"
        << "    }\n"
        << "  ]\n"
        << "}\n"
        << "```\n\n"
        << "Each rule's \"kind\" is one of:\n"
        << "- \"weekly\": every week on \"weekday\", from \"since\" until \"until\" (null for no end)\n"
        << "- \"every_other_week\": every other week on \"weekday\", counted from the first one on or after"
           " \"since\"\n"
        << "- \"monthly\": the \"nth\" \"weekday\" of each month, where \"nth\" is 1 to 5, or \"last\"\n"
        << "- \"once\": a one-off event on \"date\"\n"
        << "\"weekday\" is Mon, Tue, Wed, Thu, Fri, Sat or Sun. \"start\" and \"end\" are 24-hour times"
           " (\"end\" can be after midnight, e.g. \"00:30\"). Dates are YYYY-MM-DD. \"label\" is optional.\n";

    if (!clubs.isEmpty()) {
        out << "\n## My clubs so far\n\n```json\n" << clubsJsonText(clubs) << "```\n";
    }

    out << "\n" << kEventsHeading << "\n\n"
        << "An event's last song can be after midnight, e.g. 19:30 to 00:15.\n\n"
        << "date,weekday,first_song,last_song,songs\n";
    for (const SuggestedEvent &e : events) {
        out << e.date.toString(Qt::ISODate) << "," << kDayNames[e.date.dayOfWeek()] << ","
            << hhmm(e.start_minutes) << "," << hhmm(e.end_minutes) << "," << e.plays << "\n";
    }
    out.flush();
    return text;
}

// ---------------------------------------------------------------------------------------------
// Reading

// the JSON in an answer: a ```json block (the first one that parses), or else everything from the
//   first "{" or "[" to the last "}" or "]"
static QString jsonIn(const QString &text)
{
    static const QRegularExpression fence("```[a-zA-Z]*\\s*\\n(.*?)```", QRegularExpression::DotMatchesEverythingOption);
    auto it = fence.globalMatch(text);
    while (it.hasNext()) {
        const QString block = it.next().captured(1);
        QJsonParseError error;
        QJsonDocument::fromJson(block.toUtf8(), &error);
        if (error.error == QJsonParseError::NoError) {
            return block;
        }
    }
    const qsizetype brace = text.indexOf('{'), bracket = text.indexOf('[');
    qsizetype from = (brace < 0) ? bracket : (bracket < 0) ? brace : std::min(brace, bracket);
    qsizetype to = std::max(text.lastIndexOf('}'), text.lastIndexOf(']'));
    return (from >= 0 && to > from) ? text.mid(from, to - from + 1) : text;
}

static std::optional<int> weekdayFrom(const QJsonValue &v)
{
    if (v.isDouble()) {
        const int day = v.toInt();
        return (day >= 1 && day <= 7) ? std::optional<int>(day) : std::nullopt;
    }
    const QString s = v.toString().trimmed().left(3).toLower();
    for (int day = 1; day <= 7; ++day) {
        if (s == QString(kDayNames[day]).toLower()) {
            return day;
        }
    }
    return std::nullopt;
}

static std::optional<int> nthFrom(const QJsonValue &v)
{
    if (v.isDouble()) {
        const int n = v.toInt();
        return ((n >= 1 && n <= 5) || n == -1) ? std::optional<int>(n) : std::nullopt;
    }
    const QString s = v.toString().trimmed().toLower();
    if (s == "last" || s == "-1") {
        return -1;
    }
    static const QStringList words = { "first", "second", "third", "fourth", "fifth" };
    for (int n = 1; n <= 5; ++n) {
        if (s == words[n - 1] || s.startsWith(QString::number(n))) {
            return n;
        }
    }
    return std::nullopt;
}

// minutes after midnight, from "18:00", "6:00 PM", "6pm", "6:00 p.m.", "1800"
static std::optional<int> timeFrom(const QJsonValue &v)
{
    QString s = v.toString().trimmed().toUpper();
    s.remove('.');
    s.replace(QRegularExpression("\\s+"), " ");
    if (s == "24:00" || s == "MIDNIGHT") {
        return 23 * 60 + 59;
    }
    if (s == "NOON") {
        return 12 * 60;
    }
    static const QRegularExpression digits("^\\d{3,4}$");
    if (digits.match(s).hasMatch()) {
        s = s.left(s.size() - 2) + ":" + s.right(2);
    }
    const QLocale en(QLocale::English);
    for (const char *format : { "H:mm", "H:mm:ss", "h:mm AP", "h:mmAP", "h AP", "hAP", "h:mm:ss AP" }) {
        const QTime t = en.toTime(s, format);
        if (t.isValid()) {
            return t.hour() * 60 + t.minute();
        }
    }
    return std::nullopt;
}

static QDate dateFrom(const QJsonValue &v)
{
    return QDate::fromString(v.toString().trimmed().left(10), Qt::ISODate);
}

static std::optional<int> kindFrom(const QJsonValue &v)
{
    QString s = v.toString().toLower();
    s.remove(QRegularExpression("[^a-z]"));
    if (s == "weekly" || s == "everyweek") {
        return ClubRule::Weekly;
    }
    if (s == "everyotherweek" || s == "biweekly" || s == "fortnightly" || s == "alternateweeks") {
        return ClubRule::Biweekly;
    }
    if (s == "monthly" || s == "everymonth") {
        return ClubRule::Monthly;
    }
    if (s == "once" || s == "oneoff" || s == "single" || s == "event" || s == "date") {
        return ClubRule::Once;
    }
    return std::nullopt;
}

// One rule, or nothing (and why, in problems)
static std::optional<ClubRule> ruleFrom(const QJsonObject &o, const QString &where, QStringList *problems)
{
    auto problem = [&](const QString &what) {
        problems->append(where + ": " + what);
        return std::nullopt;
    };
    const std::optional<int> kind = kindFrom(o.value("kind"));
    if (!kind) {
        return problem(QString("unknown kind \"%1\"").arg(o.value("kind").toString()));
    }
    ClubRule r;
    r.kind = *kind;
    r.label = o.value("label").toString().trimmed();

    const std::optional<int> start = timeFrom(o.value("start")), end = timeFrom(o.value("end"));
    if (!start || !end) {
        return problem(QString("can't read the time \"%1\"")
                           .arg(!start ? o.value("start").toString() : o.value("end").toString()));
    }
    r.start_minutes = *start;
    r.end_minutes = *end;

    if (r.kind == ClubRule::Once) {
        const QJsonValue date = o.contains("date") ? o.value("date") : o.value("since");
        r.since = dateFrom(date);
        if (!r.since.isValid()) {
            return problem(date.toString().trimmed().isEmpty()
                               ? QString("a one-off event with no date")
                               : QString("can't read the date \"%1\"").arg(date.toString()));
        }
        r.until = r.since;
        r.weekday = r.since.dayOfWeek();
        return r;
    }

    r.since = dateFrom(o.value("since"));
    if (!r.since.isValid()) {
        return problem(o.value("since").toString().trimmed().isEmpty()
                           ? QString("no \"since\" date")
                           : QString("can't read the date \"%1\"").arg(o.value("since").toString()));
    }
    const QJsonValue until = o.value("until");
    if (!until.isNull() && !until.isUndefined() && !until.toString().trimmed().isEmpty()) {
        r.until = dateFrom(until);
        if (!r.until.isValid()) {
            return problem(QString("can't read the date \"%1\"").arg(until.toString()));
        }
        if (r.until < r.since) {
            return problem("its \"until\" is before its \"since\"");
        }
    }
    if (o.contains("weekday")) {
        const std::optional<int> day = weekdayFrom(o.value("weekday"));
        if (!day) {
            return problem(QString("unknown weekday \"%1\"").arg(o.value("weekday").toString()));
        }
        r.weekday = *day;
    } else {
        r.weekday = r.since.dayOfWeek();
    }
    if (r.kind == ClubRule::Monthly) {
        const std::optional<int> nth = nthFrom(o.value("nth"));
        if (!nth) {
            return problem("a monthly rule needs \"nth\": 1 to 5, or \"last\"");
        }
        r.nth = *nth;
    }
    return r;
}

QList<ClubInfo> clubsFromAIAnswer(const QString &text, QStringList *problems)
{
    QList<ClubInfo> clubs;
    QJsonParseError error;
    const QString json = jsonIn(text);
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        problems->append(QString("The answer's JSON can't be read: %1 (at character %2 of it).")
                             .arg(error.errorString()).arg(error.offset));
        return clubs;
    }
    const QJsonArray array = doc.isArray() ? doc.array() : doc.object().value("clubs").toArray();
    if (array.isEmpty()) {
        problems->append("The answer has no \"clubs\" in it.");
        return clubs;
    }

    const QStringList palette = clubPaletteColors();
    for (int i = 0; i < array.size(); ++i) {
        const QJsonObject o = array[i].toObject();
        ClubInfo club;
        club.name = o.value("name").toString().trimmed();
        if (club.name.isEmpty()) {
            club.name = QString("Club %1").arg(i + 1);
        }
        // "#rrggbb", as the Clubs tab keeps them; anything else gets a palette color
        static const QRegularExpression hexColor("^#[0-9A-Fa-f]{6}$");
        const QString color = o.value("color").toString().trimmed();
        club.color = hexColor.match(color).hasMatch() ? color.toUpper()
                                                      : (palette.isEmpty() ? QString("#888888") : palette[i % palette.size()]);
        const QJsonArray rules = o.value("rules").toArray();
        for (int k = 0; k < rules.size(); ++k) {
            const std::optional<ClubRule> rule =
                ruleFrom(rules[k].toObject(), QString("%1, rule %2").arg(club.name).arg(k + 1), problems);
            if (rule) {
                club.rules.append(*rule);
            }
        }
        if (club.rules.isEmpty()) {
            problems->append(club.name + ": no rules that SquareDesk can use, so it's left out");
            continue;
        }
        sortClubRules(club.rules);
        clubs.append(club);
    }
    return clubs;
}

QList<ClubSuggestion> suggestionsForImportedClubs(const QList<ClubInfo> &imported, const QList<QDateTime> &playsUtc,
                                                  const QList<ClubInfo> &existing, const QDate &asOf,
                                                  QStringList *upToDate)
{
    const QList<SuggestedEvent> events = eventsFromPlays(playsUtc, aiEventParams());
    QList<ClubSuggestion> suggestions;
    QList<int> namedFor;   // for each suggestion, the user's club it's named like, or -1
    for (const ClubInfo &club : imported) {
        ClubSuggestion s;
        s.club = club;
        s.club.id = -1;
        s.checked = true;   // the AI chose it

        // a club named like one of the user's adds to it, and the rules that club already has
        //   (the AI was asked to name a club to add to it) are left out
        int named = -1;
        bool nothingNew = false;
        for (const ClubInfo &mine : existing) {
            if (mine.name.trimmed().compare(club.name.trimmed(), Qt::CaseInsensitive) == 0) {
                named = mine.id;
                QList<ClubRule> fresh;
                for (const ClubRule &r : std::as_const(s.club.rules)) {
                    if (!clubHasRule(mine.rules, r)) {
                        fresh.append(r);
                    }
                }
                const qsizetype repeated = s.club.rules.size() - fresh.size();
                s.club.rules = fresh;
                if (fresh.isEmpty()) {
                    nothingNew = true;   // the AI repeated this club exactly: not a suggestion at all
                    if (upToDate != nullptr && !upToDate->contains(mine.name)) {
                        upToDate->append(mine.name);
                    }
                } else if (repeated > 0) {
                    s.note = QString("%1 of its rules %2 already in %3, and left out")
                                 .arg(repeated).arg(repeated == 1 ? "is" : "are").arg(mine.name);
                }
                break;
            }
        }
        if (nothingNew) {
            continue;
        }

        // the events its (remaining) rules hold: the middle of each event, so that one with practice
        //   before it still counts
        const QList<ClubWindow> windows = clubEventWindowsUtc(s.club.rules, asOf);
        for (const SuggestedEvent &e : events) {
            const int middle = (e.start_minutes + e.end_minutes) / 2;
            const QDateTime t(e.date.addDays(middle / 1440), QTime((middle % 1440) / 60, middle % 60));
            if (clubWindowsContain(windows, t.toUTC())) {
                s.events.append(e);
                s.plays += e.plays;
            }
        }
        if (s.events.isEmpty() && s.note.isEmpty()) {
            s.note = "no events in your play history are inside its rules yet";
        }
        suggestions.append(s);
        namedFor.append(named);
    }

    // the others go, by default, to the club of the user's that already has most of their events;
    //   whether one is worth keeping was the AI's call, so none is unchecked as already covered
    QList<ClubSuggestion> unnamed;
    for (int i = 0; i < suggestions.size(); ++i) {
        if (namedFor[i] < 0) {
            unnamed.append(suggestions[i]);
        }
    }
    matchExistingClubs(unnamed, existing, asOf);
    for (int i = 0, u = 0; i < suggestions.size(); ++i) {
        ClubSuggestion &s = suggestions[i];
        if (namedFor[i] >= 0) {
            s.existingClubId = namedFor[i];
            for (const ClubInfo &mine : existing) {
                s.existingClubName = (mine.id == namedFor[i]) ? mine.name : s.existingClubName;
            }
            continue;
        }
        const ClubSuggestion &matched = unnamed[u++];
        s.existingClubId = matched.existingClubId;
        s.existingClubName = matched.existingClubName;
        s.existingShare = matched.existingShare;
    }
    return suggestions;
}

QString importedClubsIntroText(const QStringList &upToDate)
{
    QString text = "These are the clubs from your AI's answer, each with the events in your play history that its"
                   " rules hold. Choose the ones to keep, rename them, or add their events to a club you already"
                   " have. Events you don't keep stay in Other. Nothing is saved until you click OK.";
    if (!upToDate.isEmpty()) {
        text += "\n\nAlready up to date, with nothing new in the answer: " + upToDate.join(", ") + ".";
    }
    return text;
}

bool isAIExportText(const QString &text)
{
    return text.contains(kEventsHeading) && text.contains("date,weekday,first_song,last_song,songs");
}
