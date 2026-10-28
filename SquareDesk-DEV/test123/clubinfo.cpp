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

#include "clubinfo.h"
#include "sessioninfo.h"

#include <QLocale>
#include <QTimeZone>
#include <algorithm>

QStringList clubPaletteColors()
{
    // bright enough to read as a color at swatch size, in both Light and Dark
    return { "#2BB3C0", "#F2A93B", "#E5566B", "#6C8EF5", "#5DC264", "#A97CF0", "#F07A4A", "#3FB8E8" };
}

QList<ClubInfo> clubsSeededFromSessions(const QList<SessionInfo> &sessions, bool withSchedules)
{
    static const QStringList defaultDayNames { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
    const QStringList palette = clubPaletteColors();
    QList<ClubInfo> clubs;
    for (const auto &session : sessions) {
        const bool hasDay = (session.day_of_week >= 1 && session.day_of_week <= 7);
        if (session.order_number == 0 && !hasDay) {
            continue;   // Practice
        }
        if (!withSchedules && hasDay && session.start_minutes <= 0 &&
            session.name == defaultDayNames[session.day_of_week - 1]) {
            continue;   // a default session, never used
        }
        ClubInfo club;
        club.name = isReservedClubName(session.name) ? session.name + " (club)" : session.name;
        club.color = palette[clubs.size() % palette.size()];
        club.session_rowid = session.id;
        if (withSchedules && hasDay) {
            ClubRule rule;
            rule.kind = ClubRule::Weekly;
            rule.weekday = session.day_of_week;
            rule.start_minutes = session.start_minutes;
            rule.end_minutes = 23 * 60 + 59;
            rule.since = QDate(2019, 1, 1);
            club.rules.append(rule);
        }
        clubs.append(club);
    }
    return clubs;
}

// the first date on or after d that falls on weekday (1 = Monday ... 7 = Sunday)
static QDate firstWeekdayOnOrAfter(const QDate &d, int weekday)
{
    return d.addDays((weekday - d.dayOfWeek() + 7) % 7);
}

// the nth weekday of the given month (nth = 1..5, or -1 for the last one), or an invalid date
//   when that month doesn't have one (most months have no 5th Wednesday)
static QDate nthWeekdayOfMonth(int year, int month, int weekday, int nth)
{
    if (nth == -1) {
        QDate last(year, month, QDate(year, month, 1).daysInMonth());
        return last.addDays(-((last.dayOfWeek() - weekday + 7) % 7));
    }
    QDate d = firstWeekdayOnOrAfter(QDate(year, month, 1), weekday).addDays(7 * (nth - 1));
    return (d.month() == month) ? d : QDate();
}

void sortClubRules(QList<ClubRule> &rules)
{
    std::stable_sort(rules.begin(), rules.end(), [](const ClubRule &a, const ClubRule &b) {
        if (a.isOneOff() != b.isOneOff()) {
            return b.isOneOff();
        }
        return a.isOneOff() && (a.since < b.since);
    });
}

QList<QDate> ClubRule::datesBetween(const QDate &from, const QDate &to) const
{
    QList<QDate> dates;
    if (!since.isValid() || !from.isValid() || !to.isValid()) {
        return dates;
    }
    if (kind == Once) {
        if (since >= from && since <= to) {
            dates.append(since);
        }
        return dates;
    }
    if (weekday < 1 || weekday > 7) {
        return dates;
    }

    QDate first = std::max(from, since);
    QDate last  = until.isValid() ? std::min(to, until) : to;
    if (first > last) {
        return dates;
    }

    switch (kind) {
    case Weekly:
        for (QDate d = firstWeekdayOnOrAfter(first, weekday); d <= last; d = d.addDays(7)) {
            dates.append(d);
        }
        break;

    case Biweekly: {
        // The on-weeks are fixed by `since`, not by the window being shown, so that scrolling
        //   the agenda forward never flips which weeks are on.
        QDate anchor = firstWeekdayOnOrAfter(since, weekday);
        QDate d = anchor;
        if (d < first) {
            qint64 periods = (anchor.daysTo(first) + 13) / 14;
            d = anchor.addDays(14 * periods);
        }
        for (; d <= last; d = d.addDays(14)) {
            dates.append(d);
        }
        break;
    }

    case Monthly:
        for (QDate month(first.year(), first.month(), 1); month <= last; month = month.addMonths(1)) {
            QDate d = nthWeekdayOfMonth(month.year(), month.month(), weekday, nth);
            if (d.isValid() && d >= first && d <= last) {
                dates.append(d);
            }
        }
        break;
    }
    return dates;
}

QList<ClubOccurrence> clubOccurrencesBetween(const QList<ClubRule> &rules, const QDate &from, const QDate &to)
{
    QList<ClubOccurrence> occurrences;
    for (int i = 0; i < rules.size(); ++i) {
        const ClubRule &rule = rules[i];
        const QList<QDate> dates = rule.datesBetween(from, to);
        for (const QDate &d : dates) {
            occurrences.append({ d, rule.start_minutes, rule.end_minutes, i });
        }
    }
    std::stable_sort(occurrences.begin(), occurrences.end(), [](const ClubOccurrence &a, const ClubOccurrence &b) {
        return (a.date != b.date) ? (a.date < b.date) : (a.start_minutes < b.start_minutes);
    });
    return occurrences;
}

int currentClubIndex(const QList<ClubInfo> &clubs, const QDateTime &now)
{
    const QDate today = now.date();
    const int minute = now.time().hour() * 60 + now.time().minute();

    int best = -1;
    int bestStart = 0;
    for (int i = 0; i < clubs.size(); ++i) {
        // yesterday too, for an event that started last night and runs past midnight
        const QList<ClubOccurrence> events = clubOccurrencesBetween(clubs[i].rules, today.addDays(-1), today);
        for (const ClubOccurrence &event : events) {
            // in minutes after today's midnight, so last night's are negative
            int dayOffset = -1440 * int(event.date.daysTo(today));
            int start = dayOffset + event.start_minutes;
            int end   = dayOffset + event.end_minutes + ((event.end_minutes < event.start_minutes) ? 1440 : 0);
            if (start <= minute && minute <= end && (best == -1 || start > bestStart)) {
                best = i;
                bestStart = start;
            }
        }
    }
    return best;
}

QList<ClubWindow> clubEventWindowsUtc(const QList<ClubRule> &rules, const QDate &through)
{
    QDate from;
    for (const ClubRule &rule : rules) {
        if (rule.since.isValid() && (!from.isValid() || rule.since < from)) {
            from = rule.since;
        }
    }
    QList<ClubWindow> windows;
    if (!from.isValid() || !through.isValid() || from > through) {
        return windows;
    }

    const QList<ClubOccurrence> events = clubOccurrencesBetween(rules, from, through);
    windows.reserve(events.size());
    for (const ClubOccurrence &event : events) {
        QDate endDate = (event.end_minutes < event.start_minutes) ? event.date.addDays(1) : event.date;
        QDateTime start(event.date, QTime(event.start_minutes / 60, event.start_minutes % 60, 0));
        QDateTime end(endDate, QTime(event.end_minutes / 60, event.end_minutes % 60, 59));
        windows.append({ start.toUTC(), end.toUTC() });
    }

    std::sort(windows.begin(), windows.end(), [](const ClubWindow &a, const ClubWindow &b) {
        return a.start < b.start;
    });
    QList<ClubWindow> merged;
    for (const ClubWindow &w : std::as_const(windows)) {
        if (!merged.isEmpty() && w.start <= merged.last().end) {
            if (w.end > merged.last().end) {
                merged.last().end = w.end;
            }
        } else {
            merged.append(w);
        }
    }
    return merged;
}

bool clubWindowsContain(const QList<ClubWindow> &windows, const QDateTime &utc)
{
    // the last window that starts at or before utc is the only one that can hold it
    auto after = std::upper_bound(windows.cbegin(), windows.cend(), utc, [](const QDateTime &t, const ClubWindow &w) {
        return t < w.start;
    });
    if (after == windows.cbegin()) {
        return false;
    }
    return utc <= std::prev(after)->end;
}

QString builtInClubName(int clubID)
{
    switch (clubID) {
    case kClubOther: return QString("Other");
    case kClubAll:   return QString("All");
    }
    return QString();
}

bool isReservedClubName(const QString &name)
{
    const QString trimmed = name.trimmed();
    return trimmed.compare(builtInClubName(kClubOther), Qt::CaseInsensitive) == 0 ||
           trimmed.compare(builtInClubName(kClubAll), Qt::CaseInsensitive) == 0;
}

ClubPlayMatcher clubPlayMatcher(const QList<ClubInfo> &clubs, int clubID, const QDate &through)
{
    ClubPlayMatcher matcher;
    if (clubID == kClubAll) {
        matcher.all = true;
    } else if (clubID == kClubOther) {
        QList<ClubRule> everyRule;   // every club's events at once, so the windows come out merged
        for (const ClubInfo &club : clubs) {
            everyRule.append(club.rules);
        }
        matcher.windows = clubEventWindowsUtc(everyRule, through);
        matcher.inside = false;
    } else {
        for (const ClubInfo &club : clubs) {
            if (club.id == clubID) {
                matcher.windows = clubEventWindowsUtc(club.rules, through);
            }
        }
    }
    return matcher;
}

QString clubContinuationText(const QList<ClubRule> &rules, const QDate &shownThrough)
{
    // A rule with no end goes on forever, so there is no count to give -- say how it repeats.
    QList<int> openKinds;
    QDate lastUntil;
    for (const ClubRule &rule : rules) {
        if (!rule.since.isValid()) {
            continue;
        }
        if (rule.isOneOff()) {
            if (!lastUntil.isValid() || rule.since > lastUntil) {
                lastUntil = rule.since;
            }
        } else if (!rule.until.isValid()) {
            if (!openKinds.contains(rule.kind)) {
                openKinds.append(rule.kind);
            }
        } else if (!lastUntil.isValid() || rule.until > lastUntil) {
            lastUntil = rule.until;
        }
    }

    if (!openKinds.isEmpty()) {
        if (openKinds.size() == 1) {
            switch (openKinds.first()) {
            case ClubRule::Weekly:   return QString("continues weekly with no end");
            case ClubRule::Biweekly: return QString("continues every other week with no end");
            case ClubRule::Monthly:  return QString("continues monthly with no end");
            }
        }
        return QString("continues with no end");
    }

    // Every rule ends, so the rest can be counted.
    if (!lastUntil.isValid() || lastUntil <= shownThrough) {
        return QString();
    }
    QList<ClubOccurrence> rest = clubOccurrencesBetween(rules, shownThrough.addDays(1), lastUntil);
    if (rest.isEmpty()) {
        return QString();
    }
    QString lastDate = QLocale::system().toString(rest.last().date, "MMM d, yyyy");
    return QString("%1 more, until %2").arg(rest.size()).arg(lastDate);
}
