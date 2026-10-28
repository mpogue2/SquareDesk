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
#include <algorithm>

QStringList clubPaletteColors()
{
    // bright enough to read as a color at swatch size, in both Light and Dark
    return { "#2BB3C0", "#F2A93B", "#E5566B", "#6C8EF5", "#5DC264", "#A97CF0", "#F07A4A", "#3FB8E8" };
}

QList<ClubInfo> clubsSeededFromSessions(const QList<SessionInfo> &sessions)
{
    const QStringList palette = clubPaletteColors();
    QList<ClubInfo> clubs;
    for (const auto &session : sessions) {
        ClubInfo club;
        club.name = session.name;
        club.color = palette[clubs.size() % palette.size()];
        club.session_rowid = session.id;
        if (session.day_of_week >= 1 && session.day_of_week <= 7) {
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
