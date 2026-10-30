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

#include "clubsuggester.h"

#include <QHash>
#include <QLocale>
#include <QSet>
#include <algorithm>
#include <cmath>

static int minutesOf(const QDateTime &local)
{
    return local.time().hour() * 60 + local.time().minute();
}

static int floorQuarter(int m) { return int(std::floor(m / 15.0)) * 15; }
static int ceilQuarter(int m)  { return int(std::ceil(m / 15.0)) * 15; }

// the p-th fraction of sorted values, nearest rank
static int percentile(QList<int> values, double p)
{
    if (values.isEmpty()) {
        return 0;
    }
    std::sort(values.begin(), values.end());
    int i = std::clamp(int(std::lround(p * (values.size() - 1))), 0, int(values.size()) - 1);
    return values[i];
}

QList<SuggestedEvent> eventsFromPlays(const QList<QDateTime> &playsUtc, const ClubSuggesterParams &params)
{
    QList<QDateTime> plays;
    plays.reserve(playsUtc.size());
    for (const QDateTime &t : playsUtc) {
        plays.append(t.toLocalTime());
    }
    std::sort(plays.begin(), plays.end());

    QList<SuggestedEvent> events;
    int i = 0;
    while (i < plays.size()) {
        int j = i;
        while (j + 1 < plays.size() && plays[j].secsTo(plays[j + 1]) <= params.gapMinutes * 60) {
            ++j;
        }
        const int count = j - i + 1;
        const qint64 spanMinutes = plays[i].secsTo(plays[j]) / 60;
        if (count >= params.minEventPlays && spanMinutes >= params.minEventMinutes) {
            SuggestedEvent e;
            e.date = plays[i].date();
            e.start_minutes = minutesOf(plays[i]);
            e.end_minutes = e.start_minutes + int(spanMinutes);
            e.plays = count;
            events.append(e);
        }
        i = j + 1;
    }
    return events;
}

namespace {

// One weekday's events at about the same time of day
struct Slot {
    int weekday;
    QList<SuggestedEvent> events;
    int start_minutes = 0;   // the rule times, rounded out to the quarter hour
    int end_minutes = 0;
};

void computeSlotTimes(Slot &slot)
{
    QList<int> starts, ends;
    for (const SuggestedEvent &e : std::as_const(slot.events)) {
        if (!e.absorbed) {
            starts.append(e.start_minutes);
            ends.append(e.end_minutes);
        }
    }
    slot.start_minutes = floorQuarter(percentile(starts, 0.25));
    slot.end_minutes = ceilQuarter(percentile(ends, 0.9) + 5);
}

// Busiest window of start times first, then the next busiest of what's left
QList<Slot> slotsForWeekday(int weekday, QList<SuggestedEvent> events, const ClubSuggesterParams &params)
{
    std::sort(events.begin(), events.end(), [](const SuggestedEvent &a, const SuggestedEvent &b) {
        return a.start_minutes < b.start_minutes;
    });

    QList<Slot> timeSlots;
    while (true) {
        int bestFrom = -1, bestCount = 0;
        int j = 0;
        for (int i = 0; i < events.size(); ++i) {
            j = std::max(j, i);
            while (j + 1 < events.size() && events[j + 1].start_minutes - events[i].start_minutes <= params.slotWidthMinutes) {
                ++j;
            }
            if (j - i + 1 > bestCount) {
                bestCount = j - i + 1;
                bestFrom = i;
            }
        }
        if (bestCount < params.minSlotEvents) {
            break;
        }
        Slot slot;
        slot.weekday = weekday;
        slot.events = events.mid(bestFrom, bestCount);
        events.remove(bestFrom, bestCount);
        computeSlotTimes(slot);

        // An event that started early (or late) but mostly ran during the slot's time belongs to
        //   it, e.g. a dance event that began with an hour of practice.  Taken now, before it can
        //   help make a slot of its own.
        for (qsizetype i = events.size() - 1; i >= 0; --i) {
            const SuggestedEvent &e = events[i];
            int overlap = std::min(e.end_minutes, slot.end_minutes) - std::max(e.start_minutes, slot.start_minutes);
            if (overlap >= 0.6 * (slot.end_minutes - slot.start_minutes)) {
                slot.events.append(e);
                slot.events.last().absorbed = true;
                events.remove(i);
            }
        }
        timeSlots.append(slot);
    }

    // Slots whose times overlap are one club that started earlier or later in some years, e.g. a
    //   club that went from 7:15 PM to 6:00 PM
    for (bool merged = true; merged; ) {
        merged = false;
        for (qsizetype a = 0; a < timeSlots.size() && !merged; ++a) {
            for (qsizetype b = a + 1; b < timeSlots.size() && !merged; ++b) {
                const Slot &x = timeSlots[a], &y = timeSlots[b];
                const int overlap = std::min(x.end_minutes, y.end_minutes) - std::max(x.start_minutes, y.start_minutes);
                const int shorter = std::min(x.end_minutes - x.start_minutes, y.end_minutes - y.start_minutes);
                if (overlap >= params.slotMergeOverlap * shorter) {
                    timeSlots[a].events.append(y.events);
                    timeSlots.remove(b);
                    computeSlotTimes(timeSlots[a]);
                    merged = true;
                }
            }
        }
    }

    // one event per date
    for (Slot &slot : timeSlots) {
        std::sort(slot.events.begin(), slot.events.end(), [](const SuggestedEvent &a, const SuggestedEvent &b) {
            return (a.date != b.date) ? (a.date < b.date) : (a.start_minutes < b.start_minutes);
        });
        QList<SuggestedEvent> merged;
        for (const SuggestedEvent &e : std::as_const(slot.events)) {
            if (!merged.isEmpty() && merged.last().date == e.date) {
                merged.last().end_minutes = std::max(merged.last().end_minutes, e.end_minutes);
                merged.last().plays += e.plays;
                merged.last().absorbed = merged.last().absorbed && e.absorbed;
            } else {
                merged.append(e);
            }
        }
        slot.events = merged;
    }
    return timeSlots;
}

// Whether most of the events start close to their median start: dances and classes start on
//   time, and practice doesn't
bool steadyStarts(const QList<SuggestedEvent> &events, const ClubSuggesterParams &params)
{
    QList<int> starts;
    for (const SuggestedEvent &e : events) {
        starts.append(e.start_minutes);
    }
    const int median = percentile(starts, 0.5);
    int close = 0;
    for (int m : std::as_const(starts)) {
        close += (std::abs(m - median) <= params.steadyStartMinutes) ? 1 : 0;
    }
    return !starts.isEmpty() && close >= params.steadyStartShare * starts.size();
}

int nthOfMonth(const QDate &d) { return (d.day() - 1) / 7 + 1; }
bool isLastOfMonth(const QDate &d) { return d.day() + 7 > d.daysInMonth(); }

ClubRule baseRule(const Slot &slot, int kind, const QDate &since, const QDate &until)
{
    ClubRule r;
    r.kind = kind;
    r.weekday = slot.weekday;
    r.nth = 1;
    r.start_minutes = slot.start_minutes;
    r.end_minutes = slot.end_minutes % 1440;
    r.since = since;
    r.until = until;
    return r;
}

// A one-off event's own times, widened from the slot's when the event ran outside them
ClubRule oneOffRule(const Slot &slot, const SuggestedEvent &e)
{
    ClubRule r = baseRule(slot, ClubRule::Once, e.date, e.date);
    int start = std::min(slot.start_minutes, floorQuarter(e.start_minutes - 10));
    int end = std::max(slot.end_minutes, ceilQuarter(e.end_minutes + 5));
    r.start_minutes = start;
    r.end_minutes = end % 1440;
    return r;
}

// The rules for one stretch of a slot's dates with no long break in it
QList<ClubRule> rulesForPiece(const Slot &slot, const QList<SuggestedEvent> &piece, bool openEnded,
                              const ClubSuggesterParams &params)
{
    QList<ClubRule> rules;
    const int n = int(piece.size());
    const QDate first = piece.first().date;
    const QDate last = piece.last().date;
    const QDate until = openEnded ? QDate() : last;
    const double weeks = first.daysTo(last) / 7.0 + 1;

    if (n >= params.minSlotEvents && n >= 0.5 * weeks) {
        rules.append(baseRule(slot, ClubRule::Weekly, first, until));
        return rules;
    }

    bool allFortnights = (n >= 3);
    for (int i = 1; i < n && allFortnights; ++i) {
        allFortnights = (piece[i - 1].date.daysTo(piece[i].date) % 14 == 0);
    }
    if (allFortnights && n >= 0.6 * (weeks + 1) / 2) {
        rules.append(baseRule(slot, ClubRule::Biweekly, first, until));
        return rules;
    }

    if (n >= params.minSlotEvents) {
        QHash<int, int> nthCount;   // -1 = last
        for (const SuggestedEvent &e : piece) {
            nthCount[nthOfMonth(e.date)]++;
            if (isLastOfMonth(e.date)) {
                nthCount[-1]++;
            }
        }
        int bestNth = 0, bestCount = 0;
        for (auto it = nthCount.cbegin(); it != nthCount.cend(); ++it) {
            if (it.value() > bestCount || (it.value() == bestCount && it.key() > 0)) {
                bestNth = it.key();
                bestCount = it.value();
            }
        }
        const int months = (last.year() - first.year()) * 12 + (last.month() - first.month()) + 1;
        if (bestCount >= 0.8 * n && n >= 0.6 * months) {
            ClubRule r = baseRule(slot, ClubRule::Monthly, first, until);
            r.nth = bestNth;
            rules.append(r);
            return rules;
        }
    }

    // No pattern for the whole stretch: a series inside it becomes a rule (e.g. a class, which
    //   may skip a week, in the middle of one-off dance events), and the rest are one-offs
    int i = 0;
    while (i < n) {
        int j = i;
        bool fortnights = true;
        while (j + 1 < n && piece[j].date.daysTo(piece[j + 1].date) <= params.maxSeriesGapDays) {
            fortnights = fortnights && (piece[j].date.daysTo(piece[j + 1].date) == 14);
            ++j;
        }
        if (j - i + 1 >= params.minSeriesEvents && steadyStarts(piece.mid(i, j - i + 1), params)) {
            const bool seriesOpen = openEnded && (j == n - 1);
            rules.append(baseRule(slot, fortnights ? ClubRule::Biweekly : ClubRule::Weekly,
                                  piece[i].date, seriesOpen ? QDate() : piece[j].date));
        } else {
            for (int k = i; k <= j; ++k) {
                rules.append(oneOffRule(slot, piece[k]));
            }
        }
        i = j + 1;
    }
    return rules;
}

QString partOfDay(int minutes)
{
    if (minutes < 12 * 60) {
        return "morning";
    }
    if (minutes < 17 * 60) {
        return "afternoon";
    }
    return "evening";
}

} // namespace

QList<ClubSuggestion> suggestClubs(const QList<QDateTime> &playsUtc, const QDate &asOf,
                                   const ClubSuggesterParams &params)
{
    QList<SuggestedEvent> byWeekday[8];
    const QList<SuggestedEvent> events = eventsFromPlays(playsUtc, params);
    for (const SuggestedEvent &e : events) {
        byWeekday[e.date.dayOfWeek()].append(e);
    }

    QList<Slot> timeSlots;
    for (int weekday = 1; weekday <= 7; ++weekday) {
        timeSlots.append(slotsForWeekday(weekday, byWeekday[weekday], params));
    }

    // A slot that goes a year or more without an event is a different club on each side of the gap
    //   (Tuesday events in 2018 are not the Tuesday club of 2024), so each era is its own suggestion,
    //   with its own times
    QList<Slot> eras;
    QList<int> eraCounts;   // for each era, how many eras its slot has
    for (const Slot &slot : std::as_const(timeSlots)) {
        const qsizetype firstEra = eras.size();
        for (const SuggestedEvent &e : slot.events) {
            if (eras.size() == firstEra || eras.last().events.last().date.daysTo(e.date) > params.eraDays) {
                Slot era;
                era.weekday = slot.weekday;
                eras.append(era);
            }
            eras.last().events.append(e);
        }
        for (qsizetype i = eras.size() - 1; i >= firstEra; --i) {
            if (eras[i].events.size() < params.minSlotEvents) {
                eras.remove(i);
            }
        }
        for (qsizetype i = firstEra; i < eras.size(); ++i) {
            computeSlotTimes(eras[i]);
            eraCounts.append(int(eras.size() - firstEra));
        }
    }

    QList<ClubSuggestion> suggestions;
    for (int eraIndex = 0; eraIndex < eras.size(); ++eraIndex) {
        const Slot &slot = eras[eraIndex];
        // split at long breaks
        QList<QList<SuggestedEvent>> pieces;
        for (const SuggestedEvent &e : slot.events) {
            if (pieces.isEmpty() || pieces.last().last().date.daysTo(e.date) > params.breakWeeks * 7) {
                pieces.append(QList<SuggestedEvent>());
            }
            pieces.last().append(e);
        }

        QList<ClubRule> rules;
        for (int p = 0; p < pieces.size(); ++p) {
            const bool openEnded = (p == pieces.size() - 1)
                                   && pieces[p].last().date.daysTo(asOf) <= params.endedWeeks * 7;
            rules.append(rulesForPiece(slot, pieces[p], openEnded, params));
        }

        // One-off events make a club only when they come often (ECR); a few a year are one-off
        //   dances or practice, and are left in Other
        ClubSuggestion s;
        QList<ClubRule> recurring, oneOffs;
        for (const ClubRule &r : std::as_const(rules)) {
            (r.isOneOff() ? oneOffs : recurring).append(r);
        }
        int busiestYear = 0;
        for (int i = 0, j = 0; i < oneOffs.size(); ++i) {
            while (oneOffs[j].since.daysTo(oneOffs[i].since) > 365) {
                ++j;
            }
            busiestYear = std::max(busiestYear, i - j + 1);
        }
        QList<SuggestedEvent> oneOffEvents;
        for (const SuggestedEvent &e : slot.events) {
            for (const ClubRule &r : std::as_const(oneOffs)) {
                if (r.since == e.date) {
                    oneOffEvents.append(e);
                    break;
                }
            }
        }
        if (!oneOffs.isEmpty() && busiestYear >= params.minOneOffsPerYear && !steadyStarts(oneOffEvents, params)) {
            if (recurring.isEmpty()) {
                s.checked = false;
                s.note = QString("%1 one-off events, but they don't start at a steady time: probably practice")
                             .arg(oneOffs.size());
            } else {
                for (const ClubRule &r : std::as_const(oneOffs)) {
                    s.leftInOther.append(r.since);
                }
                rules = recurring;
                s.note = QString("%1 one-off events left in Other (no steady start time)").arg(oneOffs.size());
            }
        } else if (!oneOffs.isEmpty() && busiestYear < params.minOneOffsPerYear) {
            if (recurring.isEmpty()) {
                s.checked = false;
                s.note = QString("%1 one-off events, at most %2 in a year: probably one-off dances or practice,"
                                 " so left in Other").arg(oneOffs.size()).arg(busiestYear);
            } else {
                for (const ClubRule &r : std::as_const(oneOffs)) {
                    s.leftInOther.append(r.since);
                }
                rules = recurring;
                s.note = QString("%1 one-off events left in Other").arg(oneOffs.size());
            }
        }
        if (slot.start_minutes >= params.lateStartMinutes) {
            s.checked = false;
            s.note = "starts late at night: probably practice";
        }

        s.club.rules = rules;
        sortClubRules(s.club.rules);
        s.events = slot.events;
        for (const SuggestedEvent &e : slot.events) {
            s.plays += e.plays;
        }
        s.club.name = QLocale(QLocale::English).dayName(slot.weekday) + " " + partOfDay(slot.start_minutes);
        if (eraCounts[eraIndex] > 1) {
            const int from = slot.events.first().date.year(), to = slot.events.last().date.year();
            if (rules.last().until.isValid() || rules.last().isOneOff() ? false : true) {
                s.club.name += QString(" (since %1)").arg(from);
            } else {
                s.club.name += (from == to) ? QString(" (%1)").arg(from) : QString(" (%1-%2)").arg(from).arg(to);
            }
        }
        suggestions.append(s);
    }

    std::stable_sort(suggestions.begin(), suggestions.end(), [](const ClubSuggestion &a, const ClubSuggestion &b) {
        return a.events.size() > b.events.size();
    });

    // names that would clash get their start time, and the colors go in turn
    QHash<QString, int> nameCount;
    for (const ClubSuggestion &s : std::as_const(suggestions)) {
        nameCount[s.club.name]++;
    }
    const QStringList palette = clubPaletteColors();
    for (int i = 0; i < suggestions.size(); ++i) {
        ClubSuggestion &s = suggestions[i];
        if (nameCount[s.club.name] > 1) {
            const int m = s.club.rules.first().start_minutes;
            s.club.name += " " + QLocale(QLocale::English).toString(QTime(m / 60, m % 60), "h:mm AP");
        }
        s.club.color = palette.isEmpty() ? QString("#888888") : palette[i % palette.size()];
        s.club.order_number = i;
    }
    // Practice before a bigger club: on the same days, ending before the club's event starts
    for (qsizetype a = 0; a < suggestions.size(); ++a) {
        ClubSuggestion &s = suggestions[a];
        const int weekday = s.events.first().date.dayOfWeek();
        for (qsizetype b = 0; b < a; ++b) {
            const ClubSuggestion &club = suggestions[b];
            if (!club.checked || club.events.first().date.dayOfWeek() != weekday) {
                continue;
            }
            QHash<QDate, int> clubStarts;
            for (const SuggestedEvent &e : club.events) {
                clubStarts.insert(e.date, e.start_minutes);
            }
            int before = 0;
            for (const SuggestedEvent &e : std::as_const(s.events)) {
                if (clubStarts.contains(e.date) && e.end_minutes <= clubStarts[e.date] + 15) {
                    ++before;
                }
            }
            if (before >= params.practiceBeforeShare * s.events.size()) {
                s.checked = false;
                s.note = QString("%1 of its %2 events end before %3 starts, on the same day: probably"
                                 " practice for it").arg(before).arg(s.events.size()).arg(club.club.name);
                break;
            }
        }
    }

    return suggestions;
}

void matchExistingClubs(QList<ClubSuggestion> &suggestions, const QList<ClubInfo> &clubs, const QDate &asOf,
                        double coveredShare, double likeShare)
{
    QList<QList<ClubWindow>> windows;
    for (const ClubInfo &club : clubs) {
        windows.append(clubEventWindowsUtc(club.rules, asOf));
    }
    for (ClubSuggestion &s : suggestions) {
        int best = -1, bestCount = 0;
        for (int c = 0; c < clubs.size(); ++c) {
            int count = 0;
            for (const SuggestedEvent &e : std::as_const(s.events)) {
                // the middle of the event, so that one that started early still counts
                const int middle = (e.start_minutes + e.end_minutes) / 2;
                QDateTime t(e.date.addDays(middle / 1440), QTime((middle % 1440) / 60, middle % 60));
                count += clubWindowsContain(windows[c], t.toUTC()) ? 1 : 0;
            }
            if (count > bestCount) {
                best = c;
                bestCount = count;
            }
        }
        const double share = s.events.isEmpty() ? 0 : double(bestCount) / s.events.size();
        if (best < 0 || share < likeShare) {
            continue;
        }
        s.existingClubId = clubs[best].id;
        s.existingClubName = clubs[best].name;
        s.existingShare = share;
        if (share >= coveredShare) {
            s.checked = false;
            s.note = QString("already covered by %1: %2 of its %3 events are %1 events")
                         .arg(clubs[best].name).arg(bestCount).arg(s.events.size());
        }
    }
}

QString clubRuleSentence(const ClubRule &r)
{
    const QLocale en(QLocale::English);
    auto time = [&](int m) { return en.toString(QTime(m / 60, m % 60), "h:mm AP"); };
    auto date = [&](const QDate &d) { return en.toString(d, "MMM d, yyyy"); };
    const QString day = en.dayName(r.weekday, QLocale::ShortFormat);
    const QString times = QString("from %1 to %2").arg(time(r.start_minutes), time(r.end_minutes));

    if (r.kind == ClubRule::Once) {
        QString s = QString("One-off on %1 %2").arg(date(r.since), times);
        return r.label.isEmpty() ? s : s + "  [" + r.label + "]";
    }

    QString what;
    switch (r.kind) {
    case ClubRule::Weekly:   what = "Every week on " + day; break;
    case ClubRule::Biweekly: what = "Every other week on " + day; break;
    case ClubRule::Monthly: {
        static const char *nths[] = { "", "1st", "2nd", "3rd", "4th", "5th" };
        what = QString("%1 %2 of the month").arg((r.nth == -1) ? "Last" : nths[std::clamp(r.nth, 1, 5)], day);
        break;
    }
    }
    QString s = QString("%1 %2, since %3, %4").arg(what, times, date(r.since),
                                                    r.until.isValid() ? "until " + date(r.until) : "no end");
    return r.label.isEmpty() ? s : s + "  [" + r.label + "]";
}
