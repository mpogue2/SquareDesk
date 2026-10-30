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

#ifndef CLUBSAI_H_INCLUDED
#define CLUBSAI_H_INCLUDED

#include "clubinfo.h"
#include "clubsuggester.h"

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

// Clubs > Set Up Clubs with AI > Export Play History for AI... and Import Club Rules from AI...
//   (issue #1814)
//
// For the schedules that Suggest Clubs can't work out by itself: the user gives the exported file to
//   any AI, talks it through, and pastes the AI's answer back in.  The AI never touches the database:
//   its answer is read here, and goes through the same review dialog as Suggest Clubs.
//
// The answer is JSON, in the format that the exported file asks for (and that clubsJsonText()
//   writes):
//   { "squaredesk_clubs": 1,
//     "clubs": [ { "name": "Jokers", "color": "#E5566B",
//                  "rules": [ { "kind": "weekly", "weekday": "Wed", "start": "18:00", "end": "21:30",
//                               "since": "2019-10-02", "until": null, "label": "" },
//                             { "kind": "once", "date": "2022-09-24", "start": "19:15", "end": "22:00" } ] } ] }
//   kind: "weekly", "every_other_week", "monthly" (with "nth": 1-5 or "last"), or "once"

// The events (from the plays) that the export lists, and that imported clubs are shown with: looser
//   than Suggest Clubs' events, so that the AI sees short events too
ClubSuggesterParams aiEventParams();

// The whole file to give an AI: what to do, how to answer, the user's clubs so far, and the events
QString clubsAIExportText(const QList<QDateTime> &playsUtc, const QList<ClubInfo> &clubs, const QDate &asOf);

// The clubs as JSON, in the format above
QString clubsJsonText(const QList<ClubInfo> &clubs);

// The clubs in an AI's answer: its JSON is found in the text (a ```json block, or from the first
//   "{" to the last "}") and read leniently ("Wednesday" or "Wed" or 3, "18:00" or "6:00 PM", ...).
//   What couldn't be read is described in problems, one line each; the rest is still returned.
QList<ClubInfo> clubsFromAIAnswer(const QString &text, QStringList *problems);

// Imported clubs, as suggestions for the review dialog: each with the events (from the plays) that
//   its rules hold, matched with the user's clubs (a club named like one of them adds to it, and the
//   rules it already has are left out).  A club the AI repeated exactly isn't a suggestion: its name
//   goes in upToDate instead.
QList<ClubSuggestion> suggestionsForImportedClubs(const QList<ClubInfo> &imported, const QList<QDateTime> &playsUtc,
                                                  const QList<ClubInfo> &existing, const QDate &asOf,
                                                  QStringList *upToDate = nullptr);

// Whether text is the exported file itself (with its example JSON and the user's own clubs), which
//   is easy to paste into Import Club Rules from AI by mistake, instead of the AI's answer
bool isAIExportText(const QString &text);

// The words over the review dialog's list, for clubs that came from an AI, and the user's clubs that
//   the answer had nothing new for
QString importedClubsIntroText(const QStringList &upToDate = QStringList());

#endif /* ifndef CLUBSAI_H_INCLUDED */
