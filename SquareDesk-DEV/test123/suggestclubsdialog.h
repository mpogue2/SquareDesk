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

#ifndef SUGGESTCLUBSDIALOG_H
#define SUGGESTCLUBSDIALOG_H

#include <QDialog>
#include <QHash>
#include <QSet>

#include "clubinfo.h"
#include "clubsuggester.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QScrollArea;
class QTableWidget;
class QToolButton;
class QVBoxLayout;
class EventsTimeline;

// Clubs > Suggest Clubs from Play History..., and Clubs > Set Up Clubs with AI > Import Club Rules
//   from AI... (issue #1814)
//
// The suggestions on the left, likely clubs first (checked) and then the ones that probably aren't
//   clubs (unchecked).  On the right, the selected one: its name and color, whether it becomes a new
//   club or more rules for an existing one, a timeline of its events, its rules (the same editable
//   cards as Preferences > Clubs), and every event it found, where an event not in a rule can be
//   kept as a one-off event or left in Other.  Nothing is saved until OK; clubs() then has the
//   result.
class SuggestClubsDialog : public QDialog
{
    Q_OBJECT

public:
    SuggestClubsDialog(const QList<ClubSuggestion> &suggestions, const QList<ClubInfo> &existingClubs,
                       const QDate &asOf, const QString &theme, QWidget *parent = nullptr);

    // The existing clubs, with any rules added to them, then the new clubs (id -1)
    QList<ClubInfo> clubs() const;

    // the words over the list (for Import Club Rules from AI, whose suggestions came from one)
    void setIntroText(const QString &text);

    // select the nth suggestion in the list (0 = the first), e.g. for the probe's screenshots
    void selectSuggestion(int n);

private:
    // What the user has made of one suggestion
    struct Choice {
        ClubSuggestion original;
        bool keep;
        QString name;
        QString color;
        int targetClubId;                  // an existing club these events join, or -1
        int targetChoice;                  // or another suggestion (an index) whose new club they join, or -1
        QList<ClubRule> rules;             // the recurring rules
        QSet<QDate> oneOffs;               // the events kept as one-off events
        QHash<QDate, ClubRule> eventRules; // each event's one-off event, should it be kept
    };

    void buildUi();
    void fillList();
    void refreshListItem(int index);
    void refreshAllListItems();
    void targetsChanged();     // keep targets consistent (see the .cpp), then refresh
    void selectChoice(int index);
    void rebuildRuleCards();
    void rebuildEvents();      // the events table's rows, for the selected suggestion
    void refreshEvents();      // the rows' check boxes and status, and the timeline, in place
    QCheckBox *eventCheckBox(int row) const;
    void refreshSummary();
    void makeOneOffsWeekly();
    void keepCurrent();       // check Keep for the selected suggestion
    void stampTheme(QWidget *widget);

    void resetChoice(Choice &c) const;
    bool coveredByRule(const Choice &c, const QDate &date) const;
    bool coveredByTarget(const Choice &c, const QDate &date) const;
    QString scheduleText(const Choice &c) const;
    QString clubNameForId(int id) const;
    bool hasTarget(const Choice &c) const;
    QString choiceName(int index) const;
    QString targetName(const Choice &c) const;
    QString targetColor(const Choice &c) const;

    QList<Choice> m_choices;
    QList<ClubInfo> m_existing;
    QDate m_asOf;
    QString m_theme;
    int m_current = -1;
    bool m_filling = false;

    QLabel *m_intro;
    QListWidget *m_list;
    QWidget *m_detail;
    QCheckBox *m_keep;
    QToolButton *m_colorButton;
    QLineEdit *m_name;
    QPushButton *m_reset;
    QLabel *m_note;
    QRadioButton *m_asNew;
    QRadioButton *m_asMore;
    QComboBox *m_target;
    EventsTimeline *m_timeline;
    QScrollArea *m_rulesScroll;
    QVBoxLayout *m_rulesLayout;
    QLabel *m_noRules;
    QList<QWidget *> m_ruleCards;
    QCheckBox *m_allEvents;
    QLabel *m_eventsHeading;
    QPushButton *m_makeWeekly;
    QTableWidget *m_eventsTable;
    QLabel *m_summary;
    QPushButton *m_ok;
};

#endif // SUGGESTCLUBSDIALOG_H
