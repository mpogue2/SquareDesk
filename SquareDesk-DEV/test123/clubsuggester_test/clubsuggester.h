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

#ifndef CLUBSUGGESTER_H_INCLUDED
#define CLUBSUGGESTER_H_INCLUDED

#include "clubinfo.h"

#include <QDateTime>
#include <QList>
#include <QString>

// Suggested Clubs from play history (issue #1814).
//
// Uses nothing but the times songs were played: not session names, not existing clubs.
//   1. Events: plays split wherever no song was played for `gapMinutes`.  An event needs
//      `minEventPlays` plays over at least `minEventMinutes`.
//   2. Slots: for each weekday, the busiest `slotWidthMinutes` window of event start times is a
//      slot, then the next busiest of what's left, and so on.  Slots on the same weekday whose
//      times overlap are one slot (a club that moved its start time).  Each slot is one
//      suggested club, or one per era when it goes `eraDays` without an event (an era with
//      fewer than `minSlotEvents` events is dropped).
//   3. Schedule: a slot's dates are cut wherever it goes `breakWeeks` without an event, and each
//      piece becomes a weekly, every-other-week, or nth-weekday-of-the-month rule when it fits
//      one.  Otherwise any series inside it (`minSeriesEvents` or more events, each within
//      `maxSeriesGapDays` of the last, e.g. a class) becomes a rule, and the rest are one-offs.
//   4. Times: from the slot's early and late events, rounded out to the quarter hour.
//   5. Checked or not: every suggestion is returned, but only the likely clubs start out checked.
//      One-off nights that come fewer than `minOneOffsPerYear` times in their busiest year are
//      left in Other (probably one-off dances or practice).  Unchecked too: one-offs that don't
//      start at a steady time (practice), a slot that is mostly practice before a bigger club on
//      the same days, and one that starts late at night.  A series needs a steady start too.

class ClubSuggesterParams {
public:
    int gapMinutes = 60;
    int minEventPlays = 5;
    int minEventMinutes = 60;
    int slotWidthMinutes = 90;
    int minSlotEvents = 4;          // fewer events than this is never a club
    double slotMergeOverlap = 0.5;  // same-weekday slots whose times overlap this much (of the shorter) are one
    int minSeriesEvents = 3;        // a class series: this many events ...
    int maxSeriesGapDays = 14;      // ... each no more than this after the last
    int minOneOffsPerYear = 8;      // one-off nights this often (busiest year) are a club of one-offs
    int steadyStartMinutes = 20;    // dances and classes start on time: a series, or a club of one-offs,
    double steadyStartShare = 2.0 / 3;  // ... needs this share of its events to start this close to the median
    double practiceBeforeShare = 0.5;  // this share of events end before a bigger club's, same days: practice
    int lateStartMinutes = 21 * 60 + 30;   // a slot starting this late is practice
    int breakWeeks = 13;            // longer than a summer break
    int endedWeeks = 8;             // a slot quiet this long before the last play has ended
    int eraDays = 365;              // a slot this long without an event is a different club after it
};

class SuggestedEvent {
public:
    QDate date;           // local date the event started
    int start_minutes;    // local minutes after midnight
    int end_minutes;      // may be past 1440 when it ran past midnight
    int plays;
    bool absorbed = false;   // joined a slot for its overlap, not its start time (left out of the slot's times)
};

class ClubSuggestion {
public:
    ClubInfo club;                 // name, color, and rules; id -1 (not saved)
    QList<SuggestedEvent> events;  // the events that made it
    int plays = 0;                 // songs played in those events
    bool checked = true;           // starts out checked in the review dialog
    QString note;                  // why it isn't checked, or what was left out of it
    QList<QDate> leftInOther;      // one-off nights not put in its rules (too few a year)
    int existingClubId = -1;       // the user's club that holds most of its nights (matchExistingClubs()), or -1
    QString existingClubName;
    double existingShare = 0;      // how many of its nights that club already holds, 0..1
};

// `playsUtc` in any order.  `asOf` is "now", for deciding which clubs still meet.
QList<ClubSuggestion> suggestClubs(const QList<QDateTime> &playsUtc, const QDate &asOf,
                                   const ClubSuggesterParams &params = ClubSuggesterParams());

// For each suggestion, the user's existing club whose events already hold the most of its nights.
//   At `coveredShare` or more, the suggestion is unchecked as already covered; at `likeShare` or
//   more, that club is where its rules would go by default (see existingClubId).
void matchExistingClubs(QList<ClubSuggestion> &suggestions, const QList<ClubInfo> &clubs, const QDate &asOf,
                        double coveredShare = 0.8, double likeShare = 0.3);

// The events that plays make (step 1), exposed for the test harness
QList<SuggestedEvent> eventsFromPlays(const QList<QDateTime> &playsUtc, const ClubSuggesterParams &params);

// "Every week on Wed from 6:15 PM to 9:30 PM, since Oct 2, 2019, no end", as the Clubs tab reads
QString clubRuleSentence(const ClubRule &rule);

#endif /* ifndef CLUBSUGGESTER_H_INCLUDED */
