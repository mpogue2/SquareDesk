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

#include "suggestclubsdialog.h"
#include "clubstab.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStyle>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

static const int kEndedDays = 8 * 7;   // a weekly rule made from events this recent gets no end

// the "More events for" menu's items: an existing club (its id), or another suggestion (its index)
enum { kTargetKindRole = Qt::UserRole, kTargetIdRole = Qt::UserRole + 1 };
enum { kTargetExisting = 0, kTargetSuggestion = 1 };

enum { kEventColKeep = 0, kEventColDate = 1, kEventColTime = 2, kEventColPlays = 3, kEventColStatus = 4 };

static QIcon swatchIcon(const QString &color)
{
    const qreal dpr = 2.0;
    QPixmap pixmap(QSize(12, 12) * dpr);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(color));
    painter.drawRoundedRect(QRectF(0.5, 0.5, 11, 11), 2.5, 2.5);
    painter.end();
    QIcon icon(pixmap);
    icon.addPixmap(pixmap, QIcon::Selected);
    icon.addPixmap(pixmap, QIcon::Disabled);   // an existing club's color, shown but not editable
    return icon;
}

static QString timeText(int minutes)
{
    return QLocale(QLocale::English).toString(QTime((minutes / 60) % 24, minutes % 60), "h:mm AP");
}

static int floorQuarter(int m) { return int(std::floor(m / 15.0)) * 15; }
static int ceilQuarter(int m)  { return int(std::ceil(m / 15.0)) * 15; }

// --------------------------------------------------------------------------------------------
// A strip of the years the suggestion spans, with a tick for each event: tall in the club's color
//   for an event in one of its rules, shorter for a one-off event, and short and gray for an event
//   left in Other.
class EventsTimeline : public QWidget
{
public:
    enum State { InRule, OneOff, Other };

    explicit EventsTimeline(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(54);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    // the Clubs list's colors in Themes.qss (#clubsList), which a painted widget can't read
    void setTheme(const QString &theme)
    {
        const bool dark = (theme == "Dark");
        m_background = QColor(dark ? "#2d2d2d" : "#fafafa");
        m_border = QColor(dark ? "#4a4a4a" : "#c9c9c9");
        m_text = QColor(dark ? "#9a9a9a" : "#666666");
        update();
    }

    void setEvents(const QList<QPair<QDate, State>> &events, const QDate &from, const QDate &to, const QColor &color)
    {
        m_ticks = events;
        m_from = QDate(from.year(), 1, 1);
        m_to = QDate(to.year() + 1, 1, 1);
        m_color = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor text = m_text;
        const QRectF strip(1, 1, width() - 2, height() - 18);
        p.setPen(QPen(m_border, 1));
        p.setBrush(m_background);
        p.drawRoundedRect(strip, 5, 5);
        if (!m_from.isValid() || m_from >= m_to) {
            return;
        }

        const double span = m_from.daysTo(m_to);
        auto xOf = [&](const QDate &d) { return strip.left() + 6 + (strip.width() - 12) * m_from.daysTo(d) / span; };

        // a line and a label at each new year, thinned out when the years are crowded
        QFont small = font();
        small.setPointSizeF(small.pointSizeF() * 0.85);
        p.setFont(small);
        const int years = m_to.year() - m_from.year();
        const int step = std::max(1, int(std::ceil(years * 40.0 / std::max(1.0, strip.width()))));
        for (int y = m_from.year(); y < m_to.year(); ++y) {
            const double x = xOf(QDate(y, 1, 1));
            p.setPen(QPen(text, 0.5, Qt::DotLine));
            p.drawLine(QPointF(x, strip.top() + 2), QPointF(x, strip.bottom() - 2));
            if ((y - m_from.year()) % step == 0) {
                p.setPen(text);
                p.drawText(QPointF(x + 2, height() - 3), QString::number(y));
            }
        }

        QColor gray = text;
        gray.setAlpha(150);
        for (const auto &event : std::as_const(m_ticks)) {
            const double x = xOf(event.first);
            const double full = strip.height() - 8;
            double h = full;
            QColor c = m_color;
            if (event.second == OneOff) {
                h = full * 0.65;
            } else if (event.second == Other) {
                h = full * 0.35;
                c = gray;
            }
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawRoundedRect(QRectF(x - 1.25, strip.bottom() - 4 - h, 2.5, h), 1, 1);
        }
    }

private:
    QList<QPair<QDate, State>> m_ticks;
    QDate m_from, m_to;
    QColor m_color;
    QColor m_background = QColor("#fafafa");
    QColor m_border = QColor("#c9c9c9");
    QColor m_text = QColor("#666666");
};

// --------------------------------------------------------------------------------------------

SuggestClubsDialog::SuggestClubsDialog(const QList<ClubSuggestion> &suggestions, const QList<ClubInfo> &existingClubs,
                                       const QDate &asOf, const QString &theme, QWidget *parent) :
    QDialog(parent),
    m_existing(existingClubs),
    m_asOf(asOf),
    m_theme(theme)
{
    setWindowTitle("Suggest Clubs from Play History");
    resize(1080, 720);

    // likely clubs first, then the rest, each in the suggester's order (most events first)
    for (int pass = 0; pass < 2; ++pass) {
        for (const ClubSuggestion &s : suggestions) {
            if (s.checked == (pass == 0)) {
                Choice c;
                c.original = s;
                resetChoice(c);
                m_choices.append(c);
            }
        }
    }

    QSet<QString> used;
    for (const ClubInfo &club : std::as_const(m_existing)) {
        used.insert(club.color.toLower());
    }
    const QStringList palette = clubPaletteColors();
    for (Choice &c : m_choices) {
        if (used.contains(c.color.toLower())) {
            for (const QString &color : palette) {
                if (!used.contains(color.toLower())) {
                    c.color = color;
                    c.original.club.color = color;   // so Start Over keeps it
                    break;
                }
            }
        }
        used.insert(c.color.toLower());
    }

    buildUi();
    fillList();
    stampTheme(this);
    refreshSummary();
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->data(Qt::UserRole).toInt() >= 0) {
            m_list->setCurrentRow(i);
            break;
        }
    }
}

void SuggestClubsDialog::setIntroText(const QString &text)
{
    m_intro->setText(text);
}

void SuggestClubsDialog::selectSuggestion(int n)
{
    for (int row = 0; row < m_list->count(); ++row) {
        if (m_list->item(row)->data(Qt::UserRole).toInt() == n) {
            m_list->setCurrentRow(row);
            return;
        }
    }
}

void SuggestClubsDialog::resetChoice(Choice &c) const
{
    const ClubSuggestion &s = c.original;
    c.keep = s.checked;
    c.name = s.club.name;
    c.color = s.club.color;
    c.targetClubId = s.existingClubId;
    c.targetChoice = -1;
    c.rules.clear();
    c.oneOffs.clear();
    c.eventRules.clear();
    for (const ClubRule &r : s.club.rules) {
        if (r.isOneOff()) {
            if (s.checked) {       // an unchecked suggestion's one-offs start out in Other too
                c.oneOffs.insert(r.since);
            }
            c.eventRules.insert(r.since, r);
        } else {
            c.rules.append(r);
        }
    }
    // an event with no one-off event of its own (e.g. one the suggester left in Other) gets one
    //   from its own times, should the user keep it
    for (const SuggestedEvent &e : s.events) {
        if (!c.eventRules.contains(e.date)) {
            ClubRule r;
            r.kind = ClubRule::Once;
            r.weekday = e.date.dayOfWeek();
            r.start_minutes = floorQuarter(e.start_minutes - 10);
            r.end_minutes = ceilQuarter(e.end_minutes + 5) % 1440;
            r.since = r.until = e.date;
            c.eventRules.insert(e.date, r);
        }
    }
}

void SuggestClubsDialog::buildUi()
{
    m_intro = new QLabel(
        "SquareDesk looked at when you played songs, and found these regular events. The likely clubs are"
        " checked. Choose the ones to keep, rename them, or add their events to a club you already have."
        " Events you don't keep stay in Other. Nothing is saved until you click OK.");
    m_intro->setWordWrap(true);

    m_list = new QListWidget();
    m_list->setObjectName("clubsList");
    m_list->setIconSize(QSize(12, 12));
    m_list->setSpacing(1);
    m_list->setMinimumWidth(220);
    m_list->setTextElideMode(Qt::ElideRight);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // ---- the selected suggestion
    m_detail = new QWidget();

    m_keep = new QCheckBox("Keep");
    m_keep->setToolTip("Keep this suggestion when you click OK");
    m_colorButton = new QToolButton();
    m_colorButton->setObjectName("clubsColorButton");
    m_colorButton->setPopupMode(QToolButton::InstantPopup);
    QMenu *colorMenu = new QMenu(m_colorButton);
    for (const QString &color : clubPaletteColors()) {
        QAction *action = colorMenu->addAction(swatchIcon(color), QString());
        connect(action, &QAction::triggered, this, [this, color]() {
            if (m_current >= 0) {
                m_choices[m_current].color = color;
                m_colorButton->setIcon(swatchIcon(color));
                refreshAllListItems();
                refreshEvents();
            }
        });
    }
    m_colorButton->setMenu(colorMenu);
    m_name = new QLineEdit();
    m_name->setObjectName("clubsNameEdit");
    m_name->setPlaceholderText("Club name");
    m_reset = new QPushButton("Start Over");
    m_reset->setToolTip("Undo your changes to this suggestion");

    QHBoxLayout *nameRow = new QHBoxLayout();
    nameRow->addWidget(m_keep);
    nameRow->addSpacing(8);
    nameRow->addWidget(m_colorButton);
    nameRow->addWidget(m_name, 1);
    nameRow->addWidget(m_reset);

    m_note = new QLabel();
    m_note->setObjectName("clubRuleWord");
    m_note->setWordWrap(true);

    m_asNew = new QRadioButton("A new club");
    m_asMore = new QRadioButton("More events for");
    QButtonGroup *asGroup = new QButtonGroup(this);
    asGroup->addButton(m_asNew);
    asGroup->addButton(m_asMore);
    m_target = new QComboBox();
    // not as wide as the longest club name: that would set the pane's minimum width, and leave
    //   the splitter nowhere to go
    m_target->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_target->setMinimumContentsLength(22);   // room for "Wednesday evening (new)"
    QHBoxLayout *asRow = new QHBoxLayout();
    QLabel *addAs = new QLabel("Add as:");
    addAs->setObjectName("clubRuleWord");
    asRow->addWidget(addAs);
    asRow->addWidget(m_asNew);
    asRow->addSpacing(12);
    asRow->addWidget(m_asMore);
    asRow->addWidget(m_target);
    asRow->addStretch(1);

    m_timeline = new EventsTimeline();
    m_timeline->setTheme(m_theme);

    QLabel *rulesHeading = new QLabel("RULES");
    rulesHeading->setObjectName("clubsListHeading");
    QWidget *rulesHost = new QWidget();
    m_rulesLayout = new QVBoxLayout(rulesHost);
    m_rulesLayout->setContentsMargins(0, 0, 0, 0);
    m_rulesLayout->setSpacing(6);
    m_rulesScroll = new QScrollArea();
    m_rulesScroll->setObjectName("clubsRulesScroll");
    m_rulesScroll->setWidget(rulesHost);
    m_rulesScroll->setWidgetResizable(true);
    m_rulesScroll->setFrameShape(QFrame::NoFrame);
    // a scroll bar when the cards are wider than the pane, rather than the cards' width as the
    //   pane's minimum, which would leave the splitter nowhere to go
    m_rulesScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_noRules = new QLabel("No recurring rule. Its events below can be kept as one-off events, or made weekly.");
    m_noRules->setObjectName("clubRuleWord");
    m_noRules->setWordWrap(true);

    m_eventsHeading = new QLabel();
    m_eventsHeading->setObjectName("clubsListHeading");
    m_makeWeekly = new QPushButton("Make Checked Events Weekly");
    m_makeWeekly->setToolTip("Replace the checked one-off events with one weekly rule, from the first of them to"
                             " the last (no end, when the last one is recent)");
    // one click keeps every event not already in a rule as a one-off; the next leaves them all in Other
    m_allEvents = new QCheckBox();
    m_allEvents->setTristate(true);
    m_allEvents->setToolTip("Check or uncheck every event that isn't already in a rule");
    QHBoxLayout *eventsRow = new QHBoxLayout();
    eventsRow->addSpacing(7);   // over the column of check boxes
    eventsRow->addWidget(m_allEvents);
    eventsRow->addSpacing(4);
    eventsRow->addWidget(m_eventsHeading);
    eventsRow->addStretch(1);
    eventsRow->addWidget(m_makeWeekly);

    m_eventsTable = new QTableWidget(0, 5);
    m_eventsTable->setHorizontalHeaderLabels({ "", "Date", "Time", "Plays", "" });
    m_eventsTable->verticalHeader()->hide();
    m_eventsTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_eventsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_eventsTable->setShowGrid(false);
    m_eventsTable->horizontalHeader()->setSectionResizeMode(kEventColKeep, QHeaderView::Fixed);
    m_eventsTable->horizontalHeader()->resizeSection(kEventColKeep, 30);
    m_eventsTable->horizontalHeader()->setSectionResizeMode(kEventColDate, QHeaderView::ResizeToContents);
    m_eventsTable->horizontalHeader()->setSectionResizeMode(kEventColTime, QHeaderView::ResizeToContents);
    m_eventsTable->horizontalHeader()->setSectionResizeMode(kEventColPlays, QHeaderView::ResizeToContents);
    m_eventsTable->horizontalHeader()->setStretchLastSection(true);
    m_eventsTable->verticalHeader()->setDefaultSectionSize(22);

    QVBoxLayout *detailLayout = new QVBoxLayout(m_detail);
    detailLayout->setContentsMargins(12, 0, 0, 0);
    detailLayout->addLayout(nameRow);
    detailLayout->addWidget(m_note);
    detailLayout->addLayout(asRow);
    detailLayout->addSpacing(4);
    detailLayout->addWidget(m_timeline);
    detailLayout->addSpacing(6);
    detailLayout->addWidget(rulesHeading);
    detailLayout->addWidget(m_rulesScroll, 1);
    detailLayout->addWidget(m_noRules);
    detailLayout->addSpacing(6);
    detailLayout->addLayout(eventsRow);
    detailLayout->addWidget(m_eventsTable, 2);

    QSplitter *splitter = new QSplitter();
    splitter->addWidget(m_list);
    splitter->addWidget(m_detail);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({ 330, 750 });
    splitter->setChildrenCollapsible(false);

    m_summary = new QLabel();
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_ok = buttons->button(QDialogButtonBox::Ok);
    QHBoxLayout *bottom = new QHBoxLayout();
    bottom->addWidget(m_summary, 1);
    bottom->addWidget(buttons);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_intro);
    layout->addSpacing(4);
    layout->addWidget(splitter, 1);
    layout->addLayout(bottom);

    // ---- behavior
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        selectChoice(row >= 0 ? m_list->item(row)->data(Qt::UserRole).toInt() : -1);
    });
    connect(m_list, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
        const int index = item->data(Qt::UserRole).toInt();
        if (m_filling || index < 0) {
            return;
        }
        m_choices[index].keep = (item->checkState() == Qt::Checked);
        if (index == m_current) {
            const QSignalBlocker blocker(m_keep);
            m_keep->setChecked(m_choices[index].keep);
        }
        targetsChanged();
    });
    connect(m_keep, &QCheckBox::toggled, this, [this](bool on) {
        if (m_filling || m_current < 0) {
            return;
        }
        m_choices[m_current].keep = on;
        targetsChanged();
    });
    connect(m_name, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (m_current >= 0) {
            m_choices[m_current].name = text;
            refreshAllListItems();   // the suggestions headed for this one show its name
            refreshSummary();
        }
    });
    connect(m_reset, &QPushButton::clicked, this, [this]() {
        if (m_current >= 0) {
            resetChoice(m_choices[m_current]);
            targetsChanged();
        }
    });
    auto targetChanged = [this]() {
        if (m_filling || m_current < 0) {
            return;
        }
        Choice &c = m_choices[m_current];
        c.targetClubId = -1;
        c.targetChoice = -1;
        if (m_asMore->isChecked() && m_target->currentIndex() >= 0) {
            const int id = m_target->currentData(kTargetIdRole).toInt();
            if (m_target->currentData(kTargetKindRole).toInt() == kTargetExisting) {
                c.targetClubId = id;
            } else {
                c.targetChoice = id;
            }
        }
        targetsChanged();   // also shows the target's name and color
    };
    connect(m_asNew, &QRadioButton::toggled, this, targetChanged);
    // activated, not currentIndexChanged: picking the club the menu already shows still chooses it
    connect(m_target, &QComboBox::activated, this, [this, targetChanged]() {
        if (!m_filling) {
            m_asMore->setChecked(true);
            targetChanged();
        }
    });
    connect(m_makeWeekly, &QPushButton::clicked, this, &SuggestClubsDialog::makeOneOffsWeekly);
    connect(m_allEvents, &QCheckBox::clicked, this, [this]() {
        if (m_current < 0) {
            return;
        }
        Choice &c = m_choices[m_current];
        QList<QDate> checkable;
        bool allKept = true;
        for (const SuggestedEvent &e : std::as_const(c.original.events)) {
            if (!coveredByRule(c, e.date) && !coveredByTarget(c, e.date)) {
                checkable.append(e.date);
                allKept = allKept && c.oneOffs.contains(e.date);
            }
        }
        for (const QDate &d : std::as_const(checkable)) {
            if (allKept) {
                c.oneOffs.remove(d);
            } else {
                c.oneOffs.insert(d);
            }
        }
        if (!allKept && !checkable.isEmpty()) {
            keepCurrent();
        }
        refreshEvents();   // which also sets this check box's own state
        refreshListItem(m_current);
        refreshSummary();
    });
}

void SuggestClubsDialog::fillList()
{
    m_filling = true;
    m_list->clear();
    bool headingDone[2] = { false, false };
    for (int i = 0; i < m_choices.size(); ++i) {
        const int section = m_choices[i].original.checked ? 0 : 1;
        if (!headingDone[section]) {
            headingDone[section] = true;
            QListWidgetItem *heading = new QListWidgetItem(section == 0 ? "LIKELY CLUBS" : "PROBABLY NOT CLUBS");
            heading->setFlags(Qt::NoItemFlags);
            heading->setData(Qt::UserRole, -1);
            QFont f = heading->font();
            f.setBold(true);
            f.setPointSizeF(f.pointSizeF() * 0.85);
            heading->setFont(f);
            if (section == 1) {
                heading->setToolTip("These look like practice, one-off dances, or events a club of yours already has."
                                    " Check one to keep it anyway.");
            }
            m_list->addItem(heading);
        }
        QListWidgetItem *item = new QListWidgetItem();
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setData(Qt::UserRole, i);
        m_list->addItem(item);
        refreshListItem(i);
    }
    m_filling = false;
}

bool SuggestClubsDialog::hasTarget(const Choice &c) const
{
    return c.targetClubId >= 0 || c.targetChoice >= 0;
}

QString SuggestClubsDialog::choiceName(int index) const
{
    const Choice &c = m_choices[index];
    return c.name.trimmed().isEmpty() ? c.original.club.name : c.name.trimmed();
}

// the club these events would go to: an existing club, or the new club another suggestion makes
QString SuggestClubsDialog::targetName(const Choice &c) const
{
    return (c.targetChoice >= 0) ? choiceName(c.targetChoice) : clubNameForId(c.targetClubId);
}

QString SuggestClubsDialog::targetColor(const Choice &c) const
{
    if (c.targetChoice >= 0) {
        return m_choices[c.targetChoice].color;
    }
    for (const ClubInfo &club : m_existing) {
        if (club.id == c.targetClubId) {
            return club.color;
        }
    }
    return c.color;
}

// After any change to what's kept or where events go: a suggestion headed for one that isn't kept
//   any more goes back to "A new club", and one headed for a suggestion that is itself headed
//   elsewhere follows it there, so that there are never chains (or loops)
void SuggestClubsDialog::targetsChanged()
{
    for (int pass = 0; pass <= m_choices.size(); ++pass) {
        bool changed = false;
        for (int i = 0; i < m_choices.size(); ++i) {
            Choice &a = m_choices[i];
            if (a.targetChoice < 0) {
                continue;
            }
            const Choice &b = m_choices[a.targetChoice];
            if (a.targetChoice == i || !b.keep || b.targetChoice == i) {
                a.targetChoice = -1;
            } else if (b.targetClubId >= 0) {
                a.targetClubId = b.targetClubId;
                a.targetChoice = -1;
            } else if (b.targetChoice >= 0) {
                a.targetChoice = b.targetChoice;
            } else {
                continue;
            }
            changed = true;
        }
        if (!changed) {
            break;
        }
    }
    refreshAllListItems();
    if (m_current >= 0) {
        selectChoice(m_current);   // the menu, the name and color, and which events the target has
    }
    refreshSummary();
}

void SuggestClubsDialog::refreshAllListItems()
{
    for (int i = 0; i < m_choices.size(); ++i) {
        refreshListItem(i);
    }
}

QString SuggestClubsDialog::clubNameForId(int id) const
{
    for (const ClubInfo &club : m_existing) {
        if (club.id == id) {
            return club.name;
        }
    }
    return QString();
}

void SuggestClubsDialog::refreshListItem(int index)
{
    for (int row = 0; row < m_list->count(); ++row) {
        QListWidgetItem *item = m_list->item(row);
        if (item->data(Qt::UserRole).toInt() != index) {
            continue;
        }
        const Choice &c = m_choices[index];
        const bool wasFilling = m_filling;
        m_filling = true;
        QString title = c.name.trimmed().isEmpty() ? QString("(no name)") : c.name;
        QString color = c.color;
        if (hasTarget(c)) {
            title = QString::fromUtf8("→ ") + targetName(c);
            color = targetColor(c);
        }
        item->setText(title + "\n" + scheduleText(c));
        item->setIcon(swatchIcon(color));
        item->setCheckState(c.keep ? Qt::Checked : Qt::Unchecked);
        item->setToolTip(c.original.note);
        QColor fg = m_list->palette().color(c.keep ? QPalette::Text : QPalette::PlaceholderText);
        item->setForeground(fg);
        m_filling = wasFilling;
        return;
    }
}

bool SuggestClubsDialog::coveredByRule(const Choice &c, const QDate &date) const
{
    for (const ClubRule &r : c.rules) {
        if (!r.datesBetween(date, date).isEmpty()) {
            return true;
        }
    }
    return false;
}

// An event that the club these events would go to already has
bool SuggestClubsDialog::coveredByTarget(const Choice &c, const QDate &date) const
{
    if (c.targetChoice >= 0) {
        const Choice &target = m_choices[c.targetChoice];
        return coveredByRule(target, date) || target.oneOffs.contains(date);
    }
    if (c.targetClubId < 0) {
        return false;
    }
    for (const ClubInfo &club : m_existing) {
        if (club.id == c.targetClubId) {
            for (const ClubRule &r : club.rules) {
                if (!r.datesBetween(date, date).isEmpty()) {
                    return true;
                }
            }
        }
    }
    return false;
}

// "Wed 5:45-9:30 PM · weekly since 2021 · 13 one-offs", from what the user has made of it
QString SuggestClubsDialog::scheduleText(const Choice &c) const
{
    const QLocale en(QLocale::English);
    QStringList parts;
    // an imported club may have no events yet: then its first rule says which day
    //   (and with rules, the day goes with the times shown)
    //   (or nothing does, and no day is shown)
    const int weekday = !c.rules.isEmpty()               ? c.rules.first().weekday
                      : !c.original.events.isEmpty()     ? c.original.events.first().date.dayOfWeek()
                      : !c.original.club.rules.isEmpty() ? c.original.club.rules.first().since.dayOfWeek() : 0;
    if (weekday >= 1 && weekday <= 7) {
        QString when = en.dayName(weekday, QLocale::ShortFormat);
        if (!c.rules.isEmpty()) {
            when += " " + timeText(c.rules.first().start_minutes) + QString::fromUtf8("–") + timeText(c.rules.first().end_minutes);
        }
        parts << when;
    }

    if (c.rules.size() == 1) {
        const ClubRule &r = c.rules.first();
        const QString kind = (r.kind == ClubRule::Biweekly) ? "every other week"
                           : (r.kind == ClubRule::Monthly)  ? "monthly" : "weekly";
        if (!r.until.isValid()) {
            parts << kind + " since " + en.toString(r.since, "MMM yyyy");
        } else {
            parts << kind + ", " + en.toString(r.since, "MMM yyyy") + QString::fromUtf8("–") + en.toString(r.until, "MMM yyyy");
        }
    } else if (c.rules.size() > 1) {
        parts << QString("%1 rules").arg(c.rules.size());
    }
    int oneOffs = 0;
    for (const QDate &d : c.oneOffs) {
        oneOffs += (coveredByRule(c, d) || coveredByTarget(c, d)) ? 0 : 1;
    }
    if (oneOffs > 0) {
        parts << QString("%1 one-off%2").arg(oneOffs).arg(oneOffs == 1 ? "" : "s");
    }
    if (c.rules.isEmpty() && oneOffs == 0) {
        parts << (c.original.events.isEmpty() ? QString("no events")
                                              : QString("%1 events, all in Other").arg(c.original.events.size()));
    }
    return parts.join(QString::fromUtf8(" · "));
}

void SuggestClubsDialog::selectChoice(int index)
{
    m_current = index;
    m_detail->setEnabled(index >= 0);
    if (index < 0) {
        return;
    }
    const Choice &c = m_choices[index];
    m_filling = true;

    m_keep->setChecked(c.keep);

    // "More events for": the user's clubs, then the other kept suggestions that aren't headed
    //   elsewhere themselves (their new clubs)
    m_target->clear();
    for (const ClubInfo &club : std::as_const(m_existing)) {
        m_target->addItem(swatchIcon(club.color), club.name);
        m_target->setItemData(m_target->count() - 1, kTargetExisting, kTargetKindRole);
        m_target->setItemData(m_target->count() - 1, club.id, kTargetIdRole);
    }
    bool separated = m_existing.isEmpty();
    for (int j = 0; j < m_choices.size(); ++j) {
        if (j == index || !m_choices[j].keep || hasTarget(m_choices[j])) {
            continue;
        }
        if (!separated) {
            m_target->insertSeparator(m_target->count());
            separated = true;
        }
        m_target->addItem(swatchIcon(m_choices[j].color), choiceName(j) + " (new)");
        m_target->setItemData(m_target->count() - 1, kTargetSuggestion, kTargetKindRole);
        m_target->setItemData(m_target->count() - 1, j, kTargetIdRole);
    }
    auto findTarget = [this](int kind, int id) {
        for (int i = 0; i < m_target->count(); ++i) {
            if (m_target->itemData(i, kTargetKindRole).toInt() == kind && m_target->itemData(i, kTargetIdRole).toInt() == id
                && m_target->itemData(i, kTargetKindRole).isValid()) {
                return i;
            }
        }
        return -1;
    };
    const bool asMore = hasTarget(c);
    int targetRow = -1;
    if (c.targetChoice >= 0) {
        targetRow = findTarget(kTargetSuggestion, c.targetChoice);
    } else if (c.targetClubId >= 0) {
        targetRow = findTarget(kTargetExisting, c.targetClubId);
    } else if (c.original.existingClubId >= 0) {
        targetRow = findTarget(kTargetExisting, c.original.existingClubId);
    }
    m_target->setCurrentIndex(targetRow >= 0 ? targetRow : (m_target->count() > 0 ? 0 : -1));
    const bool anyTarget = (m_target->count() > 0);
    m_asMore->setEnabled(anyTarget);
    m_target->setEnabled(anyTarget);
    m_asMore->setToolTip(anyTarget ? QString()
                                   : QString("No clubs yet: keep another suggestion, and these events can join its club"));
    m_asMore->setChecked(asMore);
    m_asNew->setChecked(!asMore);

    // the club these events go to keeps its own name and color
    m_name->setEnabled(!asMore);
    m_colorButton->setEnabled(!asMore);
    m_name->setText(asMore ? targetName(c) : c.name);
    m_colorButton->setIcon(swatchIcon(asMore ? targetColor(c) : c.color));

    QStringList notes;
    if (!c.original.events.isEmpty()) {
        const QDate from = c.original.events.first().date, to = c.original.events.last().date;
        notes << QString("%1 events from %2 to %3, %4 songs played.")
                     .arg(c.original.events.size())
                     .arg(QLocale(QLocale::English).toString(from, "MMM d, yyyy"),
                          QLocale(QLocale::English).toString(to, "MMM d, yyyy"))
                     .arg(c.original.plays);
    }
    if (!c.original.note.isEmpty()) {
        QString note = c.original.note;
        note[0] = note[0].toUpper();
        notes << note + ".";
    }
    if (c.original.existingClubId >= 0 && c.original.existingShare > 0 && c.original.existingShare < 0.8) {
        notes << QString("%1% of these events are already %2 events.")
                     .arg(int(std::lround(100 * c.original.existingShare))).arg(c.original.existingClubName);
    }
    m_note->setText(notes.join(" "));

    m_filling = false;
    rebuildRuleCards();
    rebuildEvents();
}

void SuggestClubsDialog::rebuildRuleCards()
{
    // a card can ask for this from inside its own delete button's click (see ClubsTab)
    for (QWidget *card : std::as_const(m_ruleCards)) {
        card->hide();
        card->deleteLater();
    }
    m_ruleCards.clear();
    if (m_current < 0) {
        return;
    }
    const QList<ClubRule> &rules = m_choices[m_current].rules;
    for (int i = 0; i < rules.size(); ++i) {
        ClubRuleCard *card = new ClubRuleCard(rules[i]);
        card->onChanged = [this, i](const ClubRule &rule) {
            m_choices[m_current].rules[i] = rule;
            refreshEvents();
            refreshListItem(m_current);
        };
        card->onRemove = [this, i]() {
            m_choices[m_current].rules.removeAt(i);
            rebuildRuleCards();
            refreshEvents();
            refreshListItem(m_current);
            refreshSummary();
        };
        m_rulesLayout->addWidget(card);
        card->show();
        m_ruleCards.append(card);
        stampTheme(card);
    }
    m_noRules->setVisible(rules.isEmpty());
    m_rulesScroll->setVisible(!rules.isEmpty());
    // no taller than the cards, so when they fit, the events get all the rest
    m_rulesLayout->activate();
    m_rulesScroll->setMaximumHeight(m_rulesScroll->widget()->sizeHint().height() + 2);
}

// The events table's rows, one per event, made once for each suggestion shown; refreshEvents() then
//   updates them in place, so the table keeps its scroll position
void SuggestClubsDialog::rebuildEvents()
{
    if (m_current < 0) {
        return;
    }
    const QLocale en(QLocale::English);
    m_eventsTable->setRowCount(0);
    for (const SuggestedEvent &e : m_choices[m_current].original.events) {
        const int row = m_eventsTable->rowCount();
        m_eventsTable->insertRow(row);
        m_eventsTable->setItem(row, kEventColDate, new QTableWidgetItem(en.toString(e.date, "ddd MMM d, yyyy")));
        m_eventsTable->setItem(row, kEventColTime, new QTableWidgetItem(
            timeText(e.start_minutes) + QString::fromUtf8(" – ") + timeText(e.end_minutes % 1440)));
        QTableWidgetItem *plays = new QTableWidgetItem(QString::number(e.plays));
        plays->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_eventsTable->setItem(row, kEventColPlays, plays);
        m_eventsTable->setItem(row, kEventColStatus, new QTableWidgetItem());
    }
    m_eventsTable->scrollToTop();
    refreshEvents();
}

// The check box that keeps an event as a one-off event: a real QCheckBox in the cell, as in the
//   call list, because the item view's own check boxes come and go as the table scrolls on macOS
//   under the app's stylesheet
QCheckBox *SuggestClubsDialog::eventCheckBox(int row) const
{
    QWidget *cell = m_eventsTable->cellWidget(row, kEventColKeep);
    return cell ? cell->findChild<QCheckBox *>() : nullptr;
}

void SuggestClubsDialog::refreshEvents()
{
    if (m_current < 0) {
        return;
    }
    const Choice &c = m_choices[m_current];
    QList<QPair<QDate, EventsTimeline::State>> timeline;
    int inRules = 0, already = 0, oneOffs = 0, other = 0;

    for (int row = 0; row < m_eventsTable->rowCount() && row < c.original.events.size(); ++row) {
        const QDate date = c.original.events[row].date;
        QString status;
        EventsTimeline::State state;
        bool checkable = false;
        if (coveredByRule(c, date)) {
            status = "in a rule";
            state = EventsTimeline::InRule;
            ++inRules;
        } else if (coveredByTarget(c, date)) {
            status = "already in " + targetName(c);
            state = EventsTimeline::InRule;
            ++already;
        } else {
            checkable = true;
            if (c.oneOffs.contains(date)) {
                status = "one-off event";
                state = EventsTimeline::OneOff;
                ++oneOffs;
            } else {
                status = "Other";
                state = EventsTimeline::Other;
                ++other;
            }
        }

        QCheckBox *box = eventCheckBox(row);
        if (checkable && box == nullptr) {
            QWidget *cell = new QWidget();
            QHBoxLayout *layout = new QHBoxLayout(cell);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setAlignment(Qt::AlignCenter);
            box = new QCheckBox();
            layout->addWidget(box);
            connect(box, &QCheckBox::toggled, this, [this, date](bool on) {
                if (m_current < 0) {
                    return;
                }
                Choice &chosen = m_choices[m_current];
                if (on) {
                    chosen.oneOffs.insert(date);
                    keepCurrent();
                } else {
                    chosen.oneOffs.remove(date);
                }
                refreshEvents();   // in place: this check box stays
                refreshListItem(m_current);
                refreshSummary();
            });
            m_eventsTable->setCellWidget(row, kEventColKeep, cell);
            stampTheme(cell);
        } else if (!checkable && box != nullptr) {
            m_eventsTable->removeCellWidget(row, kEventColKeep);
            box = nullptr;
        }
        if (box != nullptr) {
            const QSignalBlocker blocker(box);
            box->setChecked(c.oneOffs.contains(date));
        }

        QTableWidgetItem *statusItem = m_eventsTable->item(row, kEventColStatus);
        statusItem->setText(status);
        statusItem->setForeground(m_eventsTable->palette().color(state == EventsTimeline::Other ? QPalette::PlaceholderText
                                                                                            : QPalette::Text));
        timeline.append({ date, state });
    }

    QStringList counts;
    if (inRules > 0)  counts << QString("%1 in rules").arg(inRules);
    if (already > 0)  counts << QString("%1 already in %2").arg(already).arg(targetName(c));
    if (oneOffs > 0)  counts << QString("%1 one-off").arg(oneOffs);
    if (other > 0)    counts << QString("%1 in Other").arg(other);
    m_eventsHeading->setText(QString("EVENTS (%1)").arg(counts.isEmpty() ? QString("none") : counts.join(", ")));

    // making events weekly needs a few of them, on one weekday
    m_makeWeekly->setEnabled(oneOffs >= 2);

    {
        const QSignalBlocker blocker(m_allEvents);
        const int checkable = oneOffs + other;
        m_allEvents->setEnabled(checkable > 0);
        m_allEvents->setCheckState((checkable > 0 && oneOffs == checkable) ? Qt::Checked
                                   : (oneOffs == 0)                        ? Qt::Unchecked
                                                                           : Qt::PartiallyChecked);
    }

    const QString color = hasTarget(c) ? targetColor(c) : c.color;
    const QDate from = c.original.events.isEmpty() ? m_asOf : c.original.events.first().date;
    const QDate to = c.original.events.isEmpty() ? m_asOf : std::max(m_asOf, c.original.events.last().date);
    m_timeline->setEvents(timeline, from, to, QColor(color));
}

// Keeping an event, or making events weekly, means the user wants this suggestion: check Keep, or
//   the events would quietly not be saved (e.g. on an unchecked "already covered by Jokers")
void SuggestClubsDialog::keepCurrent()
{
    if (m_current < 0 || m_choices[m_current].keep) {
        return;
    }
    m_choices[m_current].keep = true;
    const QSignalBlocker blocker(m_keep);
    m_keep->setChecked(true);
}

void SuggestClubsDialog::makeOneOffsWeekly()
{
    if (m_current < 0) {
        return;
    }
    Choice &c = m_choices[m_current];
    QList<QDate> dates;
    for (const QDate &d : std::as_const(c.oneOffs)) {
        if (!coveredByRule(c, d) && !coveredByTarget(c, d)) {
            dates.append(d);
        }
    }
    std::sort(dates.begin(), dates.end());
    if (dates.size() < 2) {
        return;
    }
    // the events on the first one's weekday, at their usual (median) times
    const int weekday = dates.first().dayOfWeek();
    QList<int> starts, ends;
    QList<QDate> used;
    for (const QDate &d : std::as_const(dates)) {
        if (d.dayOfWeek() == weekday) {
            const ClubRule &r = c.eventRules[d];
            starts.append(r.start_minutes);
            ends.append(r.end_minutes);
            used.append(d);
        }
    }
    std::sort(starts.begin(), starts.end());
    std::sort(ends.begin(), ends.end());
    ClubRule rule;
    rule.kind = ClubRule::Weekly;
    rule.weekday = weekday;
    rule.start_minutes = starts[starts.size() / 2];
    rule.end_minutes = ends[ends.size() / 2];
    rule.since = used.first();
    rule.until = (used.last().daysTo(m_asOf) <= kEndedDays) ? QDate() : used.last();
    c.rules.append(rule);
    for (const QDate &d : std::as_const(used)) {
        c.oneOffs.remove(d);
    }
    keepCurrent();
    rebuildRuleCards();
    refreshEvents();
    refreshListItem(m_current);
    refreshSummary();
}

void SuggestClubsDialog::refreshSummary()
{
    int newClubs = 0;
    QStringList added;
    for (const Choice &c : std::as_const(m_choices)) {
        if (!c.keep || (c.rules.isEmpty() && c.oneOffs.isEmpty())) {
            continue;
        }
        if (c.targetClubId >= 0) {
            const QString name = clubNameForId(c.targetClubId);
            if (!added.contains(name)) {
                added << name;
            }
        } else if (c.targetChoice < 0) {
            ++newClubs;
        }
    }
    QStringList parts;
    if (newClubs > 0) {
        parts << QString("%1 new club%2").arg(newClubs).arg(newClubs == 1 ? "" : "s");
    }
    if (!added.isEmpty()) {
        parts << "more events for " + added.join(", ");
    }
    m_summary->setText(parts.isEmpty() ? QString("Nothing will be added.") : "OK adds " + parts.join(", and ") + ".");
    m_ok->setEnabled(!parts.isEmpty());
}

QList<ClubInfo> SuggestClubsDialog::clubs() const
{
    // a kept suggestion's rules, and its kept events that no rule (of its own, or of the club it
    //   joins) already has
    auto rulesOf = [this](const Choice &c) {
        QList<ClubRule> rules = c.rules;
        QList<QDate> oneOffs(c.oneOffs.cbegin(), c.oneOffs.cend());
        std::sort(oneOffs.begin(), oneOffs.end());
        for (const QDate &d : std::as_const(oneOffs)) {
            if (!coveredByRule(c, d) && !coveredByTarget(c, d)) {
                rules.append(c.eventRules.value(d));
            }
        }
        return rules;
    };

    QList<ClubInfo> result = m_existing;
    QHash<int, qsizetype> newClubFor;   // a suggestion's index -> its new club, in result
    for (int i = 0; i < m_choices.size(); ++i) {
        const Choice &c = m_choices[i];
        if (!c.keep || hasTarget(c)) {
            continue;
        }
        ClubInfo club;
        club.name = choiceName(i);
        club.color = c.color;
        club.rules = rulesOf(c);
        newClubFor.insert(i, result.size());
        result.append(club);
    }
    for (const Choice &c : m_choices) {
        if (!c.keep || !hasTarget(c)) {
            continue;
        }
        const QList<ClubRule> rules = rulesOf(c);
        // a rule the club already has isn't added again
        auto addTo = [&rules](ClubInfo &club) {
            for (const ClubRule &r : rules) {
                if (!clubHasRule(club.rules, r)) {
                    club.rules.append(r);
                }
            }
        };
        if (c.targetChoice >= 0 && newClubFor.contains(c.targetChoice)) {
            addTo(result[newClubFor[c.targetChoice]]);
            continue;
        }
        for (ClubInfo &club : result) {
            if (club.id >= 0 && club.id == c.targetClubId) {
                addTo(club);
                break;
            }
        }
    }

    // a new club that ended up with nothing in it isn't made
    QList<ClubInfo> clubs;
    for (qsizetype i = 0; i < result.size(); ++i) {
        if (i < m_existing.size() || !result[i].rules.isEmpty()) {
            clubs.append(result[i]);
        }
    }
    int order = 0;
    for (ClubInfo &club : clubs) {
        sortClubRules(club.rules);
        if (club.id < 0) {
            club.order_number = int(m_existing.size()) + order++;
        }
    }
    return clubs;
}

// As in ClubsTab: every rule in Themes.qss is keyed on the "theme" property, and widgets made after
//   the dialog was themed have to be stamped themselves
void SuggestClubsDialog::stampTheme(QWidget *widget)
{
    widget->setProperty("theme", m_theme);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    const QList<QWidget *> children = widget->findChildren<QWidget *>();
    for (QWidget *child : children) {
        child->setProperty("theme", m_theme);
        child->style()->unpolish(child);
        child->style()->polish(child);
    }
}
