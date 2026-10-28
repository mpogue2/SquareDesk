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
#include <QList>
#include <QStringList>

// Clubs, and the schedule rules that say when each one meets (issue #1721).
//
// For now these are only edited and remembered (Preferences > Clubs).  Nothing reads them yet:
//   the Sessions tab is still what decides Recent / Age.

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

// The order the Clubs tab shows a club's rules in: the recurring rules as the user arranged them,
//   then the one-off events by date
void sortClubRules(QList<ClubRule> &rules);

class ClubInfo {
public:
    QString name;
    QString color;       // "#rrggbb"
    int id;              // rowid in the clubs table, or -1 for a club that hasn't been saved yet
    int order_number;
    int session_rowid;   // the session this club was seeded from, or -1.  Not read yet.
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

// One club per session, same name and order.  A session with a day gets one rule:
//   "Every week on <day> from <start time> to 11:59 PM, since Jan 1, 2019, no end";
//   a session with no day (Practice) gets none.
QList<ClubInfo> clubsSeededFromSessions(const QList<SessionInfo> &sessions);

// All of a club's events in [from, to], in date then start-time order
QList<ClubOccurrence> clubOccurrencesBetween(const QList<ClubRule> &rules, const QDate &from, const QDate &to);

// How the agenda tells the user that it isn't showing everything, for the events after `shownThrough`:
//   "continues weekly with no end", "4 more, until May 4, 2027", or "" when nothing comes after.
//   One-off events count toward "N more".
QString clubContinuationText(const QList<ClubRule> &rules, const QDate &shownThrough);

#endif /* ifndef CLUBINFO_H_INCLUDED */
