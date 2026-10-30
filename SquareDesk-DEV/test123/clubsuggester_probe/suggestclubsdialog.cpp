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

static const int kEndedDays = 8 * 7;   // a weekly rule made from nights this recent gets no end

enum { kNightColKeep = 0, kNightColDate = 1, kNightColTime = 2, kNightColPlays = 3, kNightColStatus = 4 };

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
// A strip of the years the suggestion spans, with a tick for each night: tall in the club's color
//   for a night in one of its rules, shorter for a one-off event, and short and gray for a night
//   left in Other.
class NightsTimeline : public QWidget
{
public:
    enum State { InRule, OneOff, Other };

    explicit NightsTimeline(QWidget *parent = nullptr) : QWidget(parent)
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

    void setNights(const QList<QPair<QDate, State>> &nights, const QDate &from, const QDate &to, const QColor &color)
    {
        m_nights = nights;
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
        for (const auto &night : std::as_const(m_nights)) {
            const double x = xOf(night.first);
            const double full = strip.height() - 8;
            double h = full;
            QColor c = m_color;
            if (night.second == OneOff) {
                h = full * 0.65;
            } else if (night.second == Other) {
                h = full * 0.35;
                c = gray;
            }
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawRoundedRect(QRectF(x - 1.25, strip.bottom() - 4 - h, 2.5, h), 1, 1);
        }
    }

private:
    QList<QPair<QDate, State>> m_nights;
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

    // likely clubs first, then the rest, each in the suggester's order (most nights first)
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
    c.rules.clear();
    c.oneOffs.clear();
    c.nightRules.clear();
    for (const ClubRule &r : s.club.rules) {
        if (r.isOneOff()) {
            if (s.checked) {       // an unchecked suggestion's one-offs start out in Other too
                c.oneOffs.insert(r.since);
            }
            c.nightRules.insert(r.since, r);
        } else {
            c.rules.append(r);
        }
    }
    // a night with no one-off event of its own (e.g. one the suggester left in Other) gets one
    //   from its own times, should the user keep it
    for (const SuggestedEvent &e : s.events) {
        if (!c.nightRules.contains(e.date)) {
            ClubRule r;
            r.kind = ClubRule::Once;
            r.weekday = e.date.dayOfWeek();
            r.start_minutes = floorQuarter(e.start_minutes - 10);
            r.end_minutes = ceilQuarter(e.end_minutes + 5) % 1440;
            r.since = r.until = e.date;
            c.nightRules.insert(e.date, r);
        }
    }
}

void SuggestClubsDialog::buildUi()
{
    QLabel *intro = new QLabel(
        "SquareDesk looked at when you played songs, and found these regular nights. The likely clubs are"
        " checked. Choose the ones to keep, rename them, or add their nights to a club you already have."
        " Nights you don't keep stay in Other. Nothing is saved until you click OK.");
    intro->setWordWrap(true);

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
                refreshListItem(m_current);
                refreshNights();
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
    m_asMore = new QRadioButton("More nights for");
    QButtonGroup *asGroup = new QButtonGroup(this);
    asGroup->addButton(m_asNew);
    asGroup->addButton(m_asMore);
    m_target = new QComboBox();
    // not as wide as the longest club name: that would set the pane's minimum width, and leave
    //   the splitter nowhere to go
    m_target->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_target->setMinimumContentsLength(16);
    for (const ClubInfo &club : std::as_const(m_existing)) {
        m_target->addItem(swatchIcon(club.color), club.name, club.id);
    }
    QHBoxLayout *asRow = new QHBoxLayout();
    QLabel *addAs = new QLabel("Add as:");
    addAs->setObjectName("clubRuleWord");
    asRow->addWidget(addAs);
    asRow->addWidget(m_asNew);
    asRow->addSpacing(12);
    asRow->addWidget(m_asMore);
    asRow->addWidget(m_target);
    asRow->addStretch(1);
    if (m_existing.isEmpty()) {
        m_asMore->setEnabled(false);
        m_target->setEnabled(false);
        m_asMore->setToolTip("You don't have any clubs yet");
    }

    m_timeline = new NightsTimeline();
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
    m_noRules = new QLabel("No recurring rule. Its nights below can be kept as one-off events, or made weekly.");
    m_noRules->setObjectName("clubRuleWord");
    m_noRules->setWordWrap(true);

    m_nightsHeading = new QLabel();
    m_nightsHeading->setObjectName("clubsListHeading");
    m_makeWeekly = new QPushButton("Make Checked Nights Weekly");
    m_makeWeekly->setToolTip("Replace the checked one-off nights with one weekly rule, from the first of them to"
                             " the last (no end, when the last one is recent)");
    QHBoxLayout *nightsRow = new QHBoxLayout();
    nightsRow->addWidget(m_nightsHeading);
    nightsRow->addStretch(1);
    nightsRow->addWidget(m_makeWeekly);

    m_nights = new QTableWidget(0, 5);
    m_nights->setHorizontalHeaderLabels({ "", "Night", "Time", "Plays", "" });
    m_nights->verticalHeader()->hide();
    m_nights->setSelectionMode(QAbstractItemView::NoSelection);
    m_nights->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_nights->setShowGrid(false);
    m_nights->horizontalHeader()->setSectionResizeMode(kNightColKeep, QHeaderView::Fixed);
    m_nights->horizontalHeader()->resizeSection(kNightColKeep, 30);
    m_nights->horizontalHeader()->setSectionResizeMode(kNightColDate, QHeaderView::ResizeToContents);
    m_nights->horizontalHeader()->setSectionResizeMode(kNightColTime, QHeaderView::ResizeToContents);
    m_nights->horizontalHeader()->setSectionResizeMode(kNightColPlays, QHeaderView::ResizeToContents);
    m_nights->horizontalHeader()->setStretchLastSection(true);
    m_nights->verticalHeader()->setDefaultSectionSize(22);

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
    detailLayout->addLayout(nightsRow);
    detailLayout->addWidget(m_nights, 2);

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
    layout->addWidget(intro);
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
            m_keep->setChecked(m_choices[index].keep);
        }
        refreshSummary();
    });
    connect(m_keep, &QCheckBox::toggled, this, [this](bool on) {
        if (m_filling || m_current < 0) {
            return;
        }
        m_choices[m_current].keep = on;
        refreshListItem(m_current);
        refreshSummary();
    });
    connect(m_name, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (m_current >= 0) {
            m_choices[m_current].name = text;
            refreshListItem(m_current);
            refreshSummary();
        }
    });
    connect(m_reset, &QPushButton::clicked, this, [this]() {
        if (m_current >= 0) {
            resetChoice(m_choices[m_current]);
            refreshListItem(m_current);
            selectChoice(m_current);
            refreshSummary();
        }
    });
    auto targetChanged = [this]() {
        if (m_filling || m_current < 0) {
            return;
        }
        m_choices[m_current].targetClubId = m_asMore->isChecked() ? m_target->currentData().toInt() : -1;
        selectChoice(m_current);   // the name and color follow the target
        refreshListItem(m_current);
        refreshSummary();
    };
    connect(m_asNew, &QRadioButton::toggled, this, targetChanged);
    connect(m_target, &QComboBox::currentIndexChanged, this, [this, targetChanged]() {
        if (!m_filling) {
            m_asMore->setChecked(true);
            targetChanged();
        }
    });
    connect(m_makeWeekly, &QPushButton::clicked, this, &SuggestClubsDialog::makeOneOffsWeekly);
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
                heading->setToolTip("These look like practice, one-off dances, or nights a club of yours already has."
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
        if (c.targetClubId >= 0) {
            title = QString::fromUtf8("→ ") + clubNameForId(c.targetClubId);
            for (const ClubInfo &club : m_existing) {
                color = (club.id == c.targetClubId) ? club.color : color;
            }
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

// A night that the existing club these nights would go to already has
bool SuggestClubsDialog::coveredByTarget(const Choice &c, const QDate &date) const
{
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
    const SuggestedEvent &first = c.original.events.first();
    QString when = en.dayName(first.date.dayOfWeek(), QLocale::ShortFormat);
    if (!c.rules.isEmpty()) {
        when += " " + timeText(c.rules.first().start_minutes) + QString::fromUtf8("–") + timeText(c.rules.first().end_minutes);
    }
    parts << when;

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
        parts << QString("%1 nights, all in Other").arg(c.original.events.size());
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
    const bool asMore = (c.targetClubId >= 0);
    m_asMore->setChecked(asMore);
    m_asNew->setChecked(!asMore);
    if (asMore) {
        m_target->setCurrentIndex(std::max(0, m_target->findData(c.targetClubId)));
    } else if (c.original.existingClubId >= 0) {
        m_target->setCurrentIndex(std::max(0, m_target->findData(c.original.existingClubId)));
    }
    // a club of yours keeps its own name and color
    m_name->setEnabled(!asMore);
    m_colorButton->setEnabled(!asMore);
    QString color = c.color;
    m_name->setText(asMore ? clubNameForId(c.targetClubId) : c.name);
    if (asMore) {
        for (const ClubInfo &club : std::as_const(m_existing)) {
            color = (club.id == c.targetClubId) ? club.color : color;
        }
    }
    m_colorButton->setIcon(swatchIcon(color));

    QStringList notes;
    const QDate from = c.original.events.first().date, to = c.original.events.last().date;
    notes << QString("%1 nights from %2 to %3, %4 songs played.")
                 .arg(c.original.events.size())
                 .arg(QLocale(QLocale::English).toString(from, "MMM d, yyyy"),
                      QLocale(QLocale::English).toString(to, "MMM d, yyyy"))
                 .arg(c.original.plays);
    if (!c.original.note.isEmpty()) {
        QString note = c.original.note;
        note[0] = note[0].toUpper();
        notes << note + ".";
    }
    if (c.original.existingClubId >= 0 && c.original.existingShare < 0.8) {
        notes << QString("%1% of these nights are already %2 nights.")
                     .arg(int(std::lround(100 * c.original.existingShare))).arg(c.original.existingClubName);
    }
    m_note->setText(notes.join(" "));

    m_filling = false;
    rebuildRuleCards();
    rebuildNights();
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
            refreshNights();
            refreshListItem(m_current);
        };
        card->onRemove = [this, i]() {
            m_choices[m_current].rules.removeAt(i);
            rebuildRuleCards();
            refreshNights();
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
    // no taller than the cards, so when they fit, the nights get all the rest
    m_rulesLayout->activate();
    m_rulesScroll->setMaximumHeight(m_rulesScroll->widget()->sizeHint().height() + 2);
}

// The nights table's rows, one per night, made once for each suggestion shown; refreshNights() then
//   updates them in place, so the table keeps its scroll position
void SuggestClubsDialog::rebuildNights()
{
    if (m_current < 0) {
        return;
    }
    const QLocale en(QLocale::English);
    m_nights->setRowCount(0);
    for (const SuggestedEvent &e : m_choices[m_current].original.events) {
        const int row = m_nights->rowCount();
        m_nights->insertRow(row);
        m_nights->setItem(row, kNightColDate, new QTableWidgetItem(en.toString(e.date, "ddd MMM d, yyyy")));
        m_nights->setItem(row, kNightColTime, new QTableWidgetItem(
            timeText(e.start_minutes) + QString::fromUtf8(" – ") + timeText(e.end_minutes % 1440)));
        QTableWidgetItem *plays = new QTableWidgetItem(QString::number(e.plays));
        plays->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_nights->setItem(row, kNightColPlays, plays);
        m_nights->setItem(row, kNightColStatus, new QTableWidgetItem());
    }
    m_nights->scrollToTop();
    refreshNights();
}

// The check box that keeps a night as a one-off event: a real QCheckBox in the cell, as in the
//   call list, because the item view's own check boxes come and go as the table scrolls on macOS
//   under the app's stylesheet
QCheckBox *SuggestClubsDialog::nightCheckBox(int row) const
{
    QWidget *cell = m_nights->cellWidget(row, kNightColKeep);
    return cell ? cell->findChild<QCheckBox *>() : nullptr;
}

void SuggestClubsDialog::refreshNights()
{
    if (m_current < 0) {
        return;
    }
    const Choice &c = m_choices[m_current];
    QList<QPair<QDate, NightsTimeline::State>> timeline;
    int inRules = 0, already = 0, oneOffs = 0, other = 0;

    for (int row = 0; row < m_nights->rowCount() && row < c.original.events.size(); ++row) {
        const QDate date = c.original.events[row].date;
        QString status;
        NightsTimeline::State state;
        bool checkable = false;
        if (coveredByRule(c, date)) {
            status = "in a rule";
            state = NightsTimeline::InRule;
            ++inRules;
        } else if (coveredByTarget(c, date)) {
            status = "already " + clubNameForId(c.targetClubId);
            state = NightsTimeline::InRule;
            ++already;
        } else {
            checkable = true;
            if (c.oneOffs.contains(date)) {
                status = "one-off event";
                state = NightsTimeline::OneOff;
                ++oneOffs;
            } else {
                status = "Other";
                state = NightsTimeline::Other;
                ++other;
            }
        }

        QCheckBox *box = nightCheckBox(row);
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
                } else {
                    chosen.oneOffs.remove(date);
                }
                refreshNights();   // in place: this check box stays
                refreshListItem(m_current);
                refreshSummary();
            });
            m_nights->setCellWidget(row, kNightColKeep, cell);
            stampTheme(cell);
        } else if (!checkable && box != nullptr) {
            m_nights->removeCellWidget(row, kNightColKeep);
            box = nullptr;
        }
        if (box != nullptr) {
            const QSignalBlocker blocker(box);
            box->setChecked(c.oneOffs.contains(date));
        }

        QTableWidgetItem *statusItem = m_nights->item(row, kNightColStatus);
        statusItem->setText(status);
        statusItem->setForeground(m_nights->palette().color(state == NightsTimeline::Other ? QPalette::PlaceholderText
                                                                                            : QPalette::Text));
        timeline.append({ date, state });
    }

    QStringList counts;
    if (inRules > 0)  counts << QString("%1 in rules").arg(inRules);
    if (already > 0)  counts << QString("%1 already %2's").arg(already).arg(clubNameForId(c.targetClubId));
    if (oneOffs > 0)  counts << QString("%1 one-off").arg(oneOffs);
    if (other > 0)    counts << QString("%1 in Other").arg(other);
    m_nightsHeading->setText(QString("NIGHTS (%1)").arg(counts.join(", ")));

    // making nights weekly needs a few of them, on one weekday
    m_makeWeekly->setEnabled(oneOffs >= 2);

    QString color = c.color;
    for (const ClubInfo &club : std::as_const(m_existing)) {
        color = (club.id == c.targetClubId) ? club.color : color;
    }
    m_timeline->setNights(timeline, c.original.events.first().date, std::max(m_asOf, c.original.events.last().date),
                          QColor(color));
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
    // the nights on the first one's weekday, at their usual (median) times
    const int weekday = dates.first().dayOfWeek();
    QList<int> starts, ends;
    QList<QDate> used;
    for (const QDate &d : std::as_const(dates)) {
        if (d.dayOfWeek() == weekday) {
            const ClubRule &r = c.nightRules[d];
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
    rebuildRuleCards();
    refreshNights();
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
        } else {
            ++newClubs;
        }
    }
    QStringList parts;
    if (newClubs > 0) {
        parts << QString("%1 new club%2").arg(newClubs).arg(newClubs == 1 ? "" : "s");
    }
    if (!added.isEmpty()) {
        parts << "more nights for " + added.join(", ");
    }
    m_summary->setText(parts.isEmpty() ? QString("Nothing will be added.") : "OK adds " + parts.join(", and ") + ".");
    m_ok->setEnabled(!parts.isEmpty());
}

QList<ClubInfo> SuggestClubsDialog::clubs() const
{
    QList<ClubInfo> result = m_existing;
    int order = int(result.size());
    for (const Choice &c : m_choices) {
        if (!c.keep) {
            continue;
        }
        QList<ClubRule> rules = c.rules;
        QList<QDate> oneOffs(c.oneOffs.cbegin(), c.oneOffs.cend());
        std::sort(oneOffs.begin(), oneOffs.end());
        for (const QDate &d : std::as_const(oneOffs)) {
            if (!coveredByRule(c, d) && !coveredByTarget(c, d)) {
                rules.append(c.nightRules.value(d));
            }
        }
        if (rules.isEmpty()) {
            continue;
        }
        bool merged = false;
        for (ClubInfo &club : result) {
            if (c.targetClubId >= 0 && club.id == c.targetClubId) {
                club.rules.append(rules);
                sortClubRules(club.rules);
                merged = true;
                break;
            }
        }
        if (!merged) {
            ClubInfo club;
            club.name = c.name.trimmed().isEmpty() ? c.original.club.name : c.name.trimmed();
            club.color = c.color;
            club.order_number = order++;
            club.rules = rules;
            sortClubRules(club.rules);
            result.append(club);
        }
    }
    return result;
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
