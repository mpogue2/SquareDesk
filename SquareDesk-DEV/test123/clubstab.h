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

#ifndef CLUBSTAB_H
#define CLUBSTAB_H

#include <QWidget>
#include <QFrame>
#include <QDate>
#include <functional>

#include "clubinfo.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QScrollArea;
class QTableWidget;
class QTabWidget;
class QTimeEdit;
class QToolButton;
class QVBoxLayout;

// Preferences > Clubs (issue #1721).
//
// A list of clubs on the left; on the right, the selected club's name and color, and a Schedule
//   sub-tab holding its rules and one-off events (each one an editable sentence) and a read-only
//   agenda of the events those produce.  Built in code rather than in preferencesdialog.ui, so that the
//   whole tab lives in one place.
//
// Edits are held here until PreferencesDialog's caller reads them back with clubs() on OK.
class ClubsTab : public QWidget
{
    Q_OBJECT

public:
    explicit ClubsTab(const QString &theme, QWidget *parent = nullptr);

    void setClubs(const QList<ClubInfo> &clubs);
    QList<ClubInfo> clubs() const;

    // the selected club, as a row of clubs(), or -1; and selecting one by its id (rowid), if it's there
    int currentRow() const { return m_current; }
    void selectClubWithId(int id);

    // For debugging only, for now: "Re-create from Sessions" replaces every club with ones made
    //   from the Sessions tab.  The button only appears when this is set.
    std::function<QList<ClubInfo>()> clubsFromSessions;

private:
    void buildUi();
    void rebuildClubList();
    void refreshClubListRow(int row);
    void selectClub(int row);
    void rebuildRuleCards();
    void scrollToRuleCard(int index);
    void refreshAgenda(bool scrollToEnd = false);
    void refreshColorButton();
    void stampTheme(QWidget *widget);   // see the comment in the .cpp

    void addClub();
    void removeClub();
    void moveClub(int delta);
    void addRule();
    void addOneOff();

    QString m_theme;
    QList<ClubInfo> m_clubs;
    int m_current;
    QDate m_agendaFrom;                 // the agenda shows m_agendaFrom through m_agendaThrough
    QDate m_agendaThrough;
    QWidget *m_resetFromSessions;

    QTableWidget *m_clubList;
    QWidget *m_detail;
    QLineEdit *m_nameEdit;
    QToolButton *m_colorButton;
    QTabWidget *m_subTabs;
    QScrollArea *m_rulesScroll;
    QVBoxLayout *m_rulesLayout;
    QList<QWidget *> m_ruleCards;
    QLabel *m_agendaRange;
    QTableWidget *m_agendaTable;
    QLabel *m_agendaFooter;
};

// One rule, as a sentence of small inline editors:
//   [Every week] on [Tue] from [7:30 PM] to [9:30 PM]                                       x
//   since [Jan 1, 2019] , [no end]  ·  [label]
class ClubRuleCard : public QFrame
{
public:
    explicit ClubRuleCard(const ClubRule &rule, QWidget *parent = nullptr);

    std::function<void(const ClubRule &)> onChanged;
    std::function<void()> onRemove;

private:
    void updateFromRule();     // the parts of the sentence that depend on the kind
    void emitChanged();

    ClubRule m_rule;
    QComboBox *m_kind;
    QLabel *m_on;
    QComboBox *m_weekday;
    QLabel *m_ofTheMonth;
    QTimeEdit *m_start;
    QTimeEdit *m_end;
    QDateEdit *m_since;
    QComboBox *m_endKind;
    QDateEdit *m_until;
    QLineEdit *m_label;
};

// One one-off event, as a sentence:
//   [One-off] on [Sat Dec 13, 2026] from [8:00 PM] to [11:00 PM]                            x
//   [label]
// Stored as a ClubRule of kind Once, so it has the same callbacks as a rule card.
class ClubOneOffCard : public QFrame
{
public:
    explicit ClubOneOffCard(const ClubRule &rule, QWidget *parent = nullptr);

    std::function<void(const ClubRule &)> onChanged;
    std::function<void()> onRemove;

private:
    void emitChanged();

    ClubRule m_rule;
};

#endif // CLUBSTAB_H
