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

#ifndef CLUBINFO_H_INCLUDED
#define CLUBINFO_H_INCLUDED

#include <QString>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QStringList>

// Clubs, and the schedule rules that say when each one meets (issue #1721).
//
// Edited in Preferences > Clubs, which replaced Sessions.  The club whose event is going on picks
//   the session that plays are recorded under, and the Clubs menu picks the club whose events decide
//   Age and Recent.

// One recurrence rule, which the Clubs tab shows as one editable sentence:
//   "Every week on Tue from 7:30 PM to 9:30 PM, since Jan 1, 2019, no end"
//   "2nd Sat of the month from 8:00 PM to 11:00 PM, since ..., until ..."
//   "One-off on Dec 13, 2026 from 8:00 PM to 11:00 PM"
class ClubRule {
public:
    enum Kind {
        Weekly    = 0,
        Biweekly  = 1,   // every other week, counted from the first matching day on/after `since`
        Monthly   = 2,   // the nth weekday of the month (nth = 1..5, or -1 for the last one)
        Once      = 3    // a one-off event on `since` (and `until` is the same day); weekday and nth are unused
    };

    int kind;
    int weekday;        // 1 = Monday ... 7 = Sunday, to work with QDate
    int nth;            // Monthly only: 1..5, or -1 = last
    int start_minutes;  // minutes after local midnight
    int end_minutes;
    QDate since;        // first day the rule can produce an event
    QDate until;        // last day it can, or invalid for "no end"
    QString label;      // optional, e.g. "Party night"

    ClubRule() : kind(Weekly), weekday(1), nth(1), start_minutes(19 * 60 + 30), end_minutes(21 * 60 + 30) {}

    bool isOneOff() const { return kind == Once; }

    // every date this rule produces in [from, to], in order
    QList<QDate> datesBetween(const QDate &from, const QDate &to) const;
};

// Whether `rules` already has one that makes the same events as `rule` (its label aside)
bool clubHasRule(const QList<ClubRule> &rules, const ClubRule &rule);

// The order the Clubs tab shows a club's rules in: the recurring rules as the user arranged them,
//   then the one-off events by date
void sortClubRules(QList<ClubRule> &rules);

class ClubInfo {
public:
    QString name;
    QString color;       // "#rrggbb"
    int id;              // rowid in the clubs table, or -1 for a club that hasn't been saved yet
    int order_number;
    int session_rowid;   // the session this club was seeded from, or -1 (see retireEmptyPracticeClub())
    QList<ClubRule> rules;

    ClubInfo() : name(), color(), id(-1), order_number(0), session_rowid(-1) {}
};

// One event that a club's rules produce, for the agenda under the rules
class ClubOccurrence {
public:
    QDate date;
    int start_minutes;
    int end_minutes;
    int rule_index;      // which of the club's rules produced it
};

// The preset swatches in the Clubs tab's color popup; new and seeded clubs take them in turn
QStringList clubPaletteColors();

class SessionInfo;

// One club per session, same name and order, for a user's first Clubs.  With withSchedules (the
//   user had "Automatic: Date and Time" on, so the sessions' days and times were in use), a session
//   with a day gets one rule:
//     "Every week on <day> from <start time> to 11:59 PM, since Jan 1, 2019, no end"
//   Without it, the clubs get no rules, and the Monday ... Sunday sessions that a new database
//   starts with, still unchanged, are left out.
// Either way the first session, when it has no day (Practice), is left out: Other does its job.  And
//   a session named like a built-in club (Other, All) gets " (club)" added.
QList<ClubInfo> clubsSeededFromSessions(const QList<SessionInfo> &sessions, bool withSchedules = true);

// All of a club's events in [from, to], in date then start-time order
QList<ClubOccurrence> clubOccurrencesBetween(const QList<ClubRule> &rules, const QDate &from, const QDate &to);

// The club with an event going on at `now` (a row of `clubs`), or -1 for none.  An event runs from
//   its start minute to its end minute, inclusive; one that ends before it starts runs past
//   midnight.  When events overlap, the one that started most recently wins, as with Sessions.
int currentClubIndex(const QList<ClubInfo> &clubs, const QDateTime &now);

// The two built-in "clubs" (issue #1721), which aren't in the clubs table and have no schedule:
//   Other, every play during no club's event, and All, every play.  They take ids that no real club
//   (a clubs rowid) can have.
const int kClubNone  = -1;   // no club
const int kClubOther = -2;
const int kClubAll   = -3;
QString builtInClubName(int clubID);                  // "Other", "All", or "" for any other id
bool isReservedClubName(const QString &name);         // a name a real club can't take

// Which club Age and Recent are shown for: the schedule's club (the one whose event is going on),
//   or Other when there's none, unless the user chose one from the Clubs menu.  A choice lasts until
//   the schedule's club changes, so that the next event never starts with another club's ages showing.
class ClubForAges {
public:
    int automatic = kClubNone;   // a club id, or kClubNone
    int chosen = kClubNone;      // a club id, kClubOther, kClubAll, or kClubNone for Automatic

    // the schedule's club now; true when that's a change (which also ends any choice)
    bool setAutomatic(int clubID) {
        if (clubID == automatic) {
            return false;
        }
        automatic = clubID;
        chosen = kClubNone;
        return true;
    }
    void choose(int clubID) { chosen = clubID; }     // kClubNone = back to Automatic
    int effective() const {
        if (chosen != kClubNone) {
            return chosen;
        }
        return (automatic != kClubNone) ? automatic : kClubOther;
    }
};

// When a club met, as UTC instants, for matching plays against (Age and Recent, issue #1721): every
//   event its rules produce from the earliest "since" through `through`, each from its first second
//   to the last second of its end minute (and past midnight when it ends before it starts).  The
//   local times are converted one event at a time, so each is right on its own side of a DST change.
//   Sorted, with overlapping events merged, so that clubWindowsContain() can binary-search them.
class ClubWindow {
public:
    QDateTime start;   // UTC
    QDateTime end;     // UTC, inclusive
};
QList<ClubWindow> clubEventWindowsUtc(const QList<ClubRule> &rules, const QDate &through);

// Whether a UTC instant falls inside one of the windows
bool clubWindowsContain(const QList<ClubWindow> &windows, const QDateTime &utc);

// Which plays count for a club, for Age and Recent and the Song History export: for a club, those
//   during its events; for Other, those during no club's events; for All, every one.
class ClubPlayMatcher {
public:
    bool all = false;
    bool inside = true;            // false for Other
    QList<ClubWindow> windows;
    bool matches(const QDateTime &utc) const { return all || (clubWindowsContain(windows, utc) == inside); }
};
ClubPlayMatcher clubPlayMatcher(const QList<ClubInfo> &clubs, int clubID, const QDate &through);

// How the agenda tells the user that it isn't showing everything, for the events after `shownThrough`:
//   "continues weekly with no end", "4 more, until May 4, 2027", or "" when nothing comes after.
//   One-off events count toward "N more".
QString clubContinuationText(const QList<ClubRule> &rules, const QDate &shownThrough);

#endif /* ifndef CLUBINFO_H_INCLUDED */
