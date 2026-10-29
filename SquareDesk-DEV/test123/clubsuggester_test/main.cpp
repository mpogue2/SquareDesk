// Test harness for suggesting Clubs from play history (#1814).
//
// Usage: clubsuggester_test [--asof yyyy-MM-dd] [--events] [--detail] DBFILE
//
//   Opens DBFILE (a copy of SquareDesk.sqlite3) read-only, suggests clubs from the times in
//   song_plays alone, and compares them with the clubs and rules already in DBFILE: for each real
//   club, the suggestion that shares the most plays with it, how many of the club's plays and
//   event nights it finds, and how many it adds.  --events also lists every event found, and
//   --detail lists the events behind each suggestion.

#include "clubsuggester.h"

#include <QCoreApplication>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>
#include <QTimeZone>

static QTextStream out(stdout);

struct Club {
    QString name;
    QList<ClubRule> rules;
};

// indices of the plays inside the rules' events, and the local dates those plays were on
static QSet<int> playsInside(const QList<ClubRule> &rules, const QList<QDateTime> &plays, const QDate &asOf,
                             QHash<QDate, int> *nights = nullptr)
{
    const QList<ClubWindow> windows = clubEventWindowsUtc(rules, asOf);
    QSet<int> inside;
    for (int i = 0; i < plays.size(); ++i) {
        if (clubWindowsContain(windows, plays[i])) {
            inside.insert(i);
            if (nights != nullptr) {
                (*nights)[plays[i].toLocalTime().date()]++;
            }
        }
    }
    return inside;
}

// the nights with at least 3 plays: one stray play is not a dance
static QSet<QDate> realNights(const QHash<QDate, int> &nights)
{
    QSet<QDate> result;
    for (auto it = nights.cbegin(); it != nights.cend(); ++it) {
        if (it.value() >= 3) {
            result.insert(it.key());
        }
    }
    return result;
}

static QString pct(qsizetype part, qsizetype whole)
{
    return (whole == 0) ? QString("-") : QString::number(100.0 * part / whole, 'f', 1) + "%";
}

static QString dateList(QList<QDate> dates, int max = 12)
{
    std::sort(dates.begin(), dates.end());
    QStringList s;
    for (int i = 0; i < dates.size() && i < max; ++i) {
        s << dates[i].toString("yyyy-MM-dd ddd");
    }
    if (dates.size() > max) {
        s << QString("... %1 more").arg(dates.size() - max);
    }
    return s.join(", ");
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments().mid(1);
    QDate asOf = QDate::currentDate();
    bool listEvents = false;
    bool detail = false;
    QString dbFile;
    for (int i = 0; i < args.size(); ++i) {
        if (args[i] == "--asof" && i + 1 < args.size()) {
            asOf = QDate::fromString(args[++i], "yyyy-MM-dd");
        } else if (args[i] == "--events") {
            listEvents = true;
        } else if (args[i] == "--detail") {
            detail = true;
        } else {
            dbFile = args[i];
        }
    }
    if (dbFile.isEmpty() || !asOf.isValid()) {
        out << "Usage: clubsuggester_test [--asof yyyy-MM-dd] [--events] [--detail] DBFILE\n";
        return 1;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbFile);
    db.setConnectOptions("QSQLITE_OPEN_READONLY");
    if (!db.open()) {
        out << "Can't open " << dbFile << ": " << db.lastError().text() << "\n";
        return 1;
    }

    // SquareDesk's own plays only: Apple Music imports are left out of Age and Recent too
    QList<QDateTime> plays;
    QSqlQuery q(db);
    q.exec("SELECT played_on FROM song_plays WHERE origin = 'squaredesk' ORDER BY played_on");
    while (q.next()) {
        QDateTime t = QDateTime::fromString(q.value(0).toString(), "yyyy-MM-dd HH:mm:ss");
        t.setTimeZone(QTimeZone::UTC);
        if (t.isValid()) {
            plays.append(t);
        }
    }

    QList<Club> clubs;
    q.exec("SELECT rowid, name FROM clubs WHERE NOT deleted ORDER BY order_number");
    while (q.next()) {
        Club c;
        c.name = q.value(1).toString();
        QSqlQuery r(db);
        r.prepare("SELECT kind, weekday, nth, start_minutes, end_minutes, since_date, until_date, label"
                  " FROM club_rules WHERE club_rowid = ? ORDER BY order_number");
        r.addBindValue(q.value(0));
        r.exec();
        while (r.next()) {
            ClubRule rule;
            rule.kind = r.value(0).toInt();
            rule.weekday = r.value(1).toInt();
            rule.nth = r.value(2).toInt();
            rule.start_minutes = r.value(3).toInt();
            rule.end_minutes = r.value(4).toInt();
            rule.since = QDate::fromString(r.value(5).toString(), "yyyy-MM-dd");
            rule.until = QDate::fromString(r.value(6).toString(), "yyyy-MM-dd");
            rule.label = r.value(7).toString();
            c.rules.append(rule);
        }
        sortClubRules(c.rules);
        clubs.append(c);
    }

    out << "Database: " << dbFile << "\n";
    out << "Plays: " << plays.size() << ", " << plays.first().toLocalTime().date().toString("yyyy-MM-dd")
        << " to " << plays.last().toLocalTime().date().toString("yyyy-MM-dd") << "; as of " << asOf.toString("yyyy-MM-dd") << "\n";

    const ClubSuggesterParams params;
    const QList<SuggestedEvent> events = eventsFromPlays(plays, params);
    out << "Events (" << params.minEventPlays << "+ plays over " << params.minEventMinutes << "+ minutes, split at "
        << params.gapMinutes << "-minute gaps): " << events.size() << "\n";
    if (listEvents) {
        for (const SuggestedEvent &e : events) {
            out << QString("  %1  %2-%3  %4 plays\n").arg(e.date.toString("yyyy-MM-dd ddd"))
                       .arg(QTime(0, 0).addSecs(e.start_minutes * 60).toString("HH:mm"))
                       .arg(QTime(0, 0).addSecs((e.end_minutes % 1440) * 60).toString("HH:mm"))
                       .arg(e.plays);
        }
    }

    const QList<ClubSuggestion> suggestions = suggestClubs(plays, asOf, params);

    out << "\n=== " << suggestions.size() << " SUGGESTED CLUBS (from play times only) ===\n";
    QList<QSet<int>> suggestedPlays;
    QList<QSet<QDate>> suggestedNights;
    for (const ClubSuggestion &s : suggestions) {
        QHash<QDate, int> nights;
        suggestedPlays.append(playsInside(s.club.rules, plays, asOf, &nights));
        suggestedNights.append(realNights(nights));
        out << "\n" << (s.checked ? "[x] " : "[ ] ") << s.club.name << "  (" << s.events.size() << " events, "
            << s.events.first().date.toString("yyyy-MM-dd") << " to " << s.events.last().date.toString("yyyy-MM-dd")
            << "; " << suggestedPlays.last().size() << " plays inside its rules)\n";
        if (!s.note.isEmpty()) {
            out << "    -- " << s.note << (s.leftInOther.isEmpty() ? "" : ": " + dateList(s.leftInOther, 6)) << "\n";
        }
        int oneOffs = 0;
        for (const ClubRule &r : s.club.rules) {
            if (r.isOneOff()) {
                ++oneOffs;
                if (oneOffs > 6) {
                    continue;
                }
            }
            out << "    " << clubRuleSentence(r) << "\n";
        }
        if (oneOffs > 6) {
            out << "    ... " << (oneOffs - 6) << " more one-offs (" << oneOffs << " in all)\n";
        }
        if (detail) {
            out << "  events:\n";
            for (const SuggestedEvent &e : s.events) {
                out << QString("    %1  %2-%3  %4 plays%5\n").arg(e.date.toString("yyyy-MM-dd ddd"))
                           .arg(QTime(0, 0).addSecs(e.start_minutes * 60).toString("h:mm AP"))
                           .arg(QTime(0, 0).addSecs((e.end_minutes % 1440) * 60).toString("h:mm AP"))
                           .arg(e.plays, 2).arg(e.absorbed ? "  (joined for its overlap)" : "");
            }
        }
    }

    out << "\n=== COMPARED WITH THE CLUBS IN THE DATABASE (checked suggestions only) ===\n";
    QSet<int> matchedSuggestions;
    for (const Club &c : std::as_const(clubs)) {
        QHash<QDate, int> nightCounts;
        const QSet<int> actual = playsInside(c.rules, plays, asOf, &nightCounts);
        const QSet<QDate> actualNights = realNights(nightCounts);

        out << "\n" << c.name << "  (" << c.rules.size() << " rules, " << actual.size() << " plays, "
            << actualNights.size() << " nights with 3+ plays)\n";
        for (const ClubRule &r : c.rules) {
            if (!r.isOneOff()) {
                out << "    " << clubRuleSentence(r) << "\n";
            }
        }
        int oneOffs = 0;
        for (const ClubRule &r : c.rules) {
            oneOffs += r.isOneOff() ? 1 : 0;
        }
        if (oneOffs > 0) {
            out << "    + " << oneOffs << " one-offs\n";
        }
        if (actual.isEmpty()) {
            out << "  (no plays inside its rules: nothing to compare)\n";
            continue;
        }

        // the suggestions it shares plays with, most first
        QList<QPair<int, int>> shares;
        for (int i = 0; i < suggestions.size(); ++i) {
            if (!suggestions[i].checked) {
                continue;
            }
            int shared = int(QSet<int>(actual).intersect(suggestedPlays[i]).size());
            if (shared > 0) {
                shares.append({ shared, i });
            }
        }
        std::sort(shares.begin(), shares.end(), std::greater<>());
        if (shares.isEmpty()) {
            out << "  NOT FOUND: no checked suggestion shares any of its plays\n";
            continue;
        }
        const int best = shares.first().second;
        matchedSuggestions.insert(best);
        const QSet<int> both = QSet<int>(actual).intersect(suggestedPlays[best]);
        const QSet<QDate> nightsBoth = QSet<QDate>(actualNights).intersect(suggestedNights[best]);
        const QList<QDate> missed = QSet<QDate>(actualNights).subtract(suggestedNights[best]).values();
        const QList<QDate> extra = QSet<QDate>(suggestedNights[best]).subtract(actualNights).values();

        out << "  Best match: " << suggestions[best].club.name << "\n";
        out << "    plays:  " << pct(both.size(), actual.size()) << " of the club's plays found ("
            << both.size() << "/" << actual.size() << "); "
            << pct(both.size(), suggestedPlays[best].size()) << " of the suggestion's plays are the club's\n";
        out << "    nights: " << nightsBoth.size() << "/" << actualNights.size() << " found ("
            << pct(nightsBoth.size(), actualNights.size()) << "), " << extra.size() << " extra\n";
        if (!missed.isEmpty()) {
            out << "      missed: " << dateList(missed) << "\n";
        }
        if (!extra.isEmpty()) {
            out << "      extra:  " << dateList(extra) << "\n";
        }
        for (int k = 1; k < shares.size() && k < 4; ++k) {
            out << "  Also shares " << shares[k].first << " plays with " << suggestions[shares[k].second].club.name << "\n";
            matchedSuggestions.insert(shares[k].second);
        }
    }

    out << "\n=== CHECKED SUGGESTIONS THAT MATCH NO CLUB IN THE DATABASE ===\n";
    for (int i = 0; i < suggestions.size(); ++i) {
        if (suggestions[i].checked && !matchedSuggestions.contains(i)) {
            const ClubSuggestion &s = suggestions[i];
            out << "  " << s.club.name << ": " << s.events.size() << " events, "
                << s.events.first().date.toString("yyyy-MM-dd") << " to " << s.events.last().date.toString("yyyy-MM-dd") << "\n";
        }
    }
    return 0;
}
