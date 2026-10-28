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

#include "clubstab.h"

#include <algorithm>
#include <QColorDialog>
#include <QComboBox>
#include <QDateEdit>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QAbstractItemView>
#include <QLocale>
#include <QPushButton>
#include <QMenu>
#include <QPainter>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStyle>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimeEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

// All the colors for this tab are in Themes.qss, keyed on these object names and on the "theme"
//   dynamic property.  The only exceptions are the two links (QLabel can't style an <a> from a
//   stylesheet), which take their color from linkColor() below.

static const int kAgendaMonths = 3;   // how far back and ahead the agenda looks, and how far "Show more" extends it

enum { kListColName = 0, kListColDay = 1 };
enum { kAgendaColDate = 0, kAgendaColTime = 1, kAgendaColName = 2, kAgendaColTag = 3 };

// --------------------------------------------------------------------------------------------
static QIcon swatchIcon(const QString &color)
{
    const qreal dpr = 2.0;   // always draw at 2x, so the swatch is crisp on a Retina screen
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
    icon.addPixmap(pixmap, QIcon::Selected);   // or the selected club's swatch is drawn tinted
    return icon;
}

static QString shortDayName(int weekday)
{
    return QLocale::system().dayName(weekday, QLocale::ShortFormat);
}

static QTime minutesToTime(int minutes)
{
    return QTime(minutes / 60, minutes % 60);
}

static int timeToMinutes(const QTime &t)
{
    return t.hour() * 60 + t.minute();
}

// "7:30 – 9:30 PM" when both ends share an AM/PM suffix, else "11:00 AM – 1:00 PM" (or the
//   24-hour equivalent, in a locale that uses one)
static QString timeRangeText(int startMinutes, int endMinutes)
{
    QLocale locale = QLocale::system();
    QString a = locale.toString(minutesToTime(startMinutes), QLocale::ShortFormat);
    QString b = locale.toString(minutesToTime(endMinutes), QLocale::ShortFormat);

    // macOS puts a narrow no-break space (U+202F) before "PM", not an ordinary one
    static const QRegularExpression suffixed("^(.*\\d)[\\s\\x{00A0}\\x{202F}]*(\\D+)$");
    QRegularExpressionMatch ma = suffixed.match(a);
    QRegularExpressionMatch mb = suffixed.match(b);
    if (ma.hasMatch() && mb.hasMatch() && ma.captured(2) == mb.captured(2)) {
        a = ma.captured(1);
    }
    return a + QString::fromUtf8(" – ") + b;
}

static QString linkColor(const QString &theme)
{
    return (theme == "Dark") ? "#7FC4C9" : "#0E5C63";
}

// The kind chip holds both the kind and, for a monthly rule, which week of the month
static int kindIndexForRule(const ClubRule &rule)
{
    switch (rule.kind) {
    case ClubRule::Biweekly: return 1;
    case ClubRule::Monthly:  return (rule.nth == -1) ? 7 : 1 + std::clamp(rule.nth, 1, 5);
    default:                 return 0;
    }
}

static void applyKindIndex(ClubRule &rule, int index)
{
    if (index == 0) {
        rule.kind = ClubRule::Weekly;
    } else if (index == 1) {
        rule.kind = ClubRule::Biweekly;
    } else {
        rule.kind = ClubRule::Monthly;
        rule.nth = (index == 7) ? -1 : index - 1;
    }
}

// A QComboBox is as wide as its longest item, so "2nd" would be as wide as "Every other week".
//   A chip should hug its current text instead.
static void fitComboToCurrentText(QComboBox *combo)
{
    combo->ensurePolished();   // so the font is the stylesheet's (the kind chip is bold)
    QFontMetrics metrics = combo->fontMetrics();
    combo->setFixedWidth(metrics.horizontalAdvance(combo->currentText()) + 44);   // 2 x 8px padding, plus the arrow

    // ...but the popup still has to fit the longest item, or "Every other week" is cut off
    int widest = 0;
    for (int i = 0; i < combo->count(); ++i) {
        widest = std::max(widest, metrics.horizontalAdvance(combo->itemText(i)));
    }
    combo->view()->setMinimumWidth(widest + 40);
}

static QLabel *wordLabel(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setObjectName("clubRuleWord");
    return label;
}

static QTimeEdit *chipTimeEdit(int minutes)
{
    QTimeEdit *edit = new QTimeEdit(minutesToTime(minutes));
    edit->setObjectName("clubRuleChip");
    edit->setDisplayFormat(QLocale::system().timeFormat(QLocale::ShortFormat));
    edit->setButtonSymbols(QAbstractSpinBox::NoButtons);   // a chip: click a part and type, or use the arrow keys
    edit->setAlignment(Qt::AlignCenter);
    return edit;
}

static QDateEdit *chipDateEdit(const QDate &date)
{
    QDateEdit *edit = new QDateEdit(date);
    edit->setObjectName("clubRuleChip");
    edit->setDisplayFormat("MMM d, yyyy");
    edit->setCalendarPopup(true);
    edit->setAlignment(Qt::AlignCenter);
    return edit;
}

// ============================================================================================
ClubRuleCard::ClubRuleCard(const ClubRule &rule, QWidget *parent) :
    QFrame(parent),
    m_rule(rule)
{
    setObjectName("clubRuleCard");

    m_kind = new QComboBox();
    m_kind->setObjectName("clubRuleKindChip");
    m_kind->addItems({ "Every week", "Every other week", "1st", "2nd", "3rd", "4th", "5th", "Last" });
    m_kind->setCurrentIndex(kindIndexForRule(rule));

    m_on = wordLabel("on");

    m_weekday = new QComboBox();
    m_weekday->setObjectName("clubRuleChip");
    for (int day = 1; day <= 7; ++day) {
        m_weekday->addItem(shortDayName(day), day);
    }
    m_weekday->setCurrentIndex(std::clamp(rule.weekday, 1, 7) - 1);

    m_ofTheMonth = wordLabel("of the month");
    m_start = chipTimeEdit(rule.start_minutes);
    m_end   = chipTimeEdit(rule.end_minutes);

    m_since = chipDateEdit(rule.since.isValid() ? rule.since : QDate::currentDate());

    m_endKind = new QComboBox();
    m_endKind->setObjectName("clubRuleChip");
    m_endKind->addItems({ "no end", "until" });
    m_endKind->setCurrentIndex(rule.until.isValid() ? 1 : 0);

    m_until = chipDateEdit(rule.until.isValid() ? rule.until : QDate::currentDate());
    m_until->setMinimumDate(m_since->date());

    m_label = new QLineEdit(rule.label);
    m_label->setObjectName("clubRuleLabel");
    m_label->setPlaceholderText("add a label");

    QToolButton *remove = new QToolButton();
    remove->setObjectName("clubRuleRemove");
    remove->setText(QString::fromUtf8("✕"));
    remove->setToolTip("Delete this rule");
    remove->setCursor(Qt::PointingHandCursor);

    QHBoxLayout *line1 = new QHBoxLayout();
    line1->setSpacing(6);
    line1->addWidget(m_kind);
    line1->addWidget(m_on);
    line1->addWidget(m_weekday);
    line1->addWidget(m_ofTheMonth);
    line1->addWidget(wordLabel("from"));
    line1->addWidget(m_start);
    line1->addWidget(wordLabel("to"));
    line1->addWidget(m_end);
    line1->addStretch(1);
    line1->addWidget(remove, 0, Qt::AlignTop);

    QHBoxLayout *line2 = new QHBoxLayout();
    line2->setSpacing(6);
    line2->addWidget(wordLabel("since"));
    line2->addWidget(m_since);
    line2->addWidget(wordLabel(","));
    line2->addWidget(m_endKind);
    line2->addWidget(m_until);
    line2->addSpacing(4);
    line2->addWidget(wordLabel(QString::fromUtf8("·")));
    line2->addWidget(m_label, 1);

    QVBoxLayout *lines = new QVBoxLayout(this);
    lines->setContentsMargins(12, 8, 8, 8);
    lines->setSpacing(6);
    lines->addLayout(line1);
    lines->addLayout(line2);

    updateFromRule();

    connect(m_kind, &QComboBox::currentIndexChanged, this, [this](int index) {
        applyKindIndex(m_rule, index);
        updateFromRule();
        emitChanged();
    });
    connect(m_weekday, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_rule.weekday = index + 1;
        fitComboToCurrentText(m_weekday);
        emitChanged();
    });
    connect(m_start, &QTimeEdit::timeChanged, this, [this](const QTime &t) {
        m_rule.start_minutes = timeToMinutes(t);
        emitChanged();
    });
    connect(m_end, &QTimeEdit::timeChanged, this, [this](const QTime &t) {
        m_rule.end_minutes = timeToMinutes(t);
        emitChanged();
    });
    connect(m_since, &QDateEdit::dateChanged, this, [this](const QDate &d) {
        m_rule.since = d;
        m_until->setMinimumDate(d);   // may move the until date, which reports its own change
        emitChanged();
    });
    connect(m_endKind, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index == 1 && m_until->date() < m_since->date()) {
            m_until->setDate(m_since->date());
        }
        m_rule.until = (index == 1) ? m_until->date() : QDate();
        updateFromRule();
        emitChanged();
    });
    connect(m_until, &QDateEdit::dateChanged, this, [this](const QDate &d) {
        if (m_endKind->currentIndex() == 1) {
            m_rule.until = d;
            emitChanged();
        }
    });
    connect(m_label, &QLineEdit::textEdited, this, [this](const QString &text) {
        m_rule.label = text;
        emitChanged();
    });
    connect(remove, &QToolButton::clicked, this, [this]() {
        if (onRemove) {
            onRemove();
        }
    });
}

// "Every week on Tue" vs. "2nd Sat of the month", and "no end" vs. "until <date>"
void ClubRuleCard::updateFromRule()
{
    bool monthly = (m_rule.kind == ClubRule::Monthly);
    m_on->setVisible(!monthly);
    m_ofTheMonth->setVisible(monthly);
    m_until->setVisible(m_rule.until.isValid());

    // the kind chip is tinted by kind, like the mockup: one color for weekly, another for monthly
    m_kind->setProperty("chipKind", monthly ? "monthly" : "weekly");
    m_kind->style()->unpolish(m_kind);
    m_kind->style()->polish(m_kind);

    fitComboToCurrentText(m_kind);
    fitComboToCurrentText(m_weekday);
    fitComboToCurrentText(m_endKind);
}

void ClubRuleCard::emitChanged()
{
    if (onChanged) {
        onChanged(m_rule);
    }
}

// ============================================================================================
ClubOneOffCard::ClubOneOffCard(const ClubRule &rule, QWidget *parent) :
    QFrame(parent),
    m_rule(rule)
{
    setObjectName("clubRuleCard");

    // not a menu: a one-off can't turn into a rule (it has no "since" or "until")
    QLabel *chip = new QLabel("One-off");
    chip->setObjectName("clubOneOffChip");

    QDateEdit *date = chipDateEdit(rule.since.isValid() ? rule.since : QDate::currentDate());
    date->setDisplayFormat("ddd MMM d, yyyy");   // with the day, which a rule card shows in its own chip
    QTimeEdit *start = chipTimeEdit(rule.start_minutes);
    QTimeEdit *end   = chipTimeEdit(rule.end_minutes);

    QLineEdit *label = new QLineEdit(rule.label);
    label->setObjectName("clubRuleLabel");
    label->setPlaceholderText("add a label");

    QToolButton *remove = new QToolButton();
    remove->setObjectName("clubRuleRemove");
    remove->setText(QString::fromUtf8("✕"));
    remove->setToolTip("Delete this one-off event");
    remove->setCursor(Qt::PointingHandCursor);

    // the widest date this format makes, so the year is never clipped (the Dark style's arrow is wider)
    date->ensurePolished();
    date->setMinimumWidth(date->fontMetrics().horizontalAdvance("Wed Sep 30, 2026") + 48);

    QHBoxLayout *line1 = new QHBoxLayout();
    line1->setSpacing(6);
    line1->addWidget(chip);
    line1->addWidget(wordLabel("on"));
    line1->addWidget(date);
    line1->addWidget(wordLabel("from"));
    line1->addWidget(start);
    line1->addWidget(wordLabel("to"));
    line1->addWidget(end);
    line1->addStretch(1);
    line1->addWidget(remove, 0, Qt::AlignTop);

    // the label gets a line of its own: on the first, it would be squeezed to a few letters
    QHBoxLayout *line2 = new QHBoxLayout();
    line2->setSpacing(6);
    line2->addWidget(label, 1);

    QVBoxLayout *lines = new QVBoxLayout(this);
    lines->setContentsMargins(12, 8, 8, 8);
    lines->setSpacing(6);
    lines->addLayout(line1);
    lines->addLayout(line2);

    connect(date, &QDateEdit::dateChanged, this, [this](const QDate &d) {
        m_rule.since = d;
        m_rule.until = d;
        m_rule.weekday = d.dayOfWeek();   // unused, but keeps the stored row self-consistent
        emitChanged();
    });
    connect(start, &QTimeEdit::timeChanged, this, [this](const QTime &t) {
        m_rule.start_minutes = timeToMinutes(t);
        emitChanged();
    });
    connect(end, &QTimeEdit::timeChanged, this, [this](const QTime &t) {
        m_rule.end_minutes = timeToMinutes(t);
        emitChanged();
    });
    connect(label, &QLineEdit::textEdited, this, [this](const QString &text) {
        m_rule.label = text;
        emitChanged();
    });
    connect(remove, &QToolButton::clicked, this, [this]() {
        if (onRemove) {
            onRemove();
        }
    });
}

void ClubOneOffCard::emitChanged()
{
    if (onChanged) {
        onChanged(m_rule);
    }
}

// ============================================================================================
ClubsTab::ClubsTab(const QString &theme, QWidget *parent) :
    QWidget(parent),
    m_theme(theme),
    m_current(-1),
    m_agendaFrom(QDate::currentDate().addMonths(-kAgendaMonths)),
    m_agendaThrough(QDate::currentDate().addMonths(kAgendaMonths).addDays(-1))
{
    buildUi();
}

// Every rule in Themes.qss is keyed on the "theme" dynamic property, which MainWindow stamps
//   onto the whole Preferences dialog right after building it.  The rule cards and agenda tags
//   are built later than that -- every time a club is selected -- so they'd match no theme rule
//   at all.  Stamp each one as it is built (the same problem as the Apple Music rows, #1740).
void ClubsTab::stampTheme(QWidget *widget)
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

void ClubsTab::buildUi()
{
    // LEFT: the clubs list ------------------------------------------------------------------
    QLabel *listHeading = new QLabel("CLUBS");
    listHeading->setObjectName("clubsListHeading");

    m_clubList = new QTableWidget(0, 2);
    m_clubList->setObjectName("clubsList");
    m_clubList->horizontalHeader()->setVisible(false);
    m_clubList->verticalHeader()->setVisible(false);
    m_clubList->horizontalHeader()->setSectionResizeMode(kListColName, QHeaderView::Stretch);
    m_clubList->horizontalHeader()->setSectionResizeMode(kListColDay, QHeaderView::ResizeToContents);
    m_clubList->verticalHeader()->setDefaultSectionSize(28);
    m_clubList->setShowGrid(false);
    m_clubList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_clubList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_clubList->setEditTriggers(QAbstractItemView::NoEditTriggers);   // the name is edited on the right
    m_clubList->setIconSize(QSize(12, 12));
    m_clubList->setFocusPolicy(Qt::NoFocus);

    // same icons as the Sessions tab's buttons; the text is the fallback if there's no icon
    auto listButton = [](const QString &text, const QString &icon, const QString &tip) {
        QToolButton *button = new QToolButton();
        button->setText(text);
        button->setIcon(QIcon(icon));
        button->setToolTip(tip);
        return button;
    };
    QToolButton *addButton    = listButton("+", ":/graphics/list-add.png",    "Add a club");
    QToolButton *removeButton = listButton("-", ":/graphics/list-remove.png", "Remove the selected club");
    QToolButton *upButton     = listButton(QString::fromUtf8("↑"), ":/graphics/go-up.png",   "Move the selected club up");
    QToolButton *downButton   = listButton(QString::fromUtf8("↓"), ":/graphics/go-down.png", "Move the selected club down");

    QHBoxLayout *listButtons = new QHBoxLayout();
    listButtons->addWidget(addButton);
    listButtons->addWidget(removeButton);
    listButtons->addWidget(upButton);
    listButtons->addWidget(downButton);
    listButtons->addStretch(1);

    QWidget *left = new QWidget();
    left->setFixedWidth(190);
    QVBoxLayout *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(listHeading);
    leftLayout->addWidget(m_clubList, 1);
    leftLayout->addLayout(listButtons);

    QPushButton *resetFromSessions = new QPushButton("Re-create from Sessions");
    resetFromSessions->setToolTip("Replace every club with ones made from the Sessions tab, as on first use.\n"
                                  "Nothing is saved until OK.  (A debugging aid for now.)");
    resetFromSessions->hide();   // until clubsFromSessions is set
    m_resetFromSessions = resetFromSessions;
    leftLayout->addWidget(resetFromSessions);
    connect(resetFromSessions, &QPushButton::clicked, this, [this]() {
        if (clubsFromSessions) {
            setClubs(clubsFromSessions());
        }
    });

    // RIGHT: the selected club --------------------------------------------------------------
    m_nameEdit = new QLineEdit();
    m_nameEdit->setObjectName("clubsNameEdit");
    m_nameEdit->setPlaceholderText("Club name");

    m_colorButton = new QToolButton();
    m_colorButton->setObjectName("clubsColorButton");
    m_colorButton->setPopupMode(QToolButton::InstantPopup);
    m_colorButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_colorButton->setIconSize(QSize(14, 14));
    m_colorButton->setToolTip("The club's color");

    QMenu *colorMenu = new QMenu(m_colorButton);
    const QStringList palette = clubPaletteColors();
    for (const QString &color : palette) {
        QAction *action = colorMenu->addAction(swatchIcon(color), QString());
        action->setIconVisibleInMenu(true);
        connect(action, &QAction::triggered, this, [this, color]() {
            if (m_current < 0) return;
            m_clubs[m_current].color = color;
            refreshColorButton();
            refreshClubListRow(m_current);
        });
    }
    colorMenu->addSeparator();
    QAction *otherColor = colorMenu->addAction(QString::fromUtf8("Other…"));
    connect(otherColor, &QAction::triggered, this, [this]() {
        if (m_current < 0) return;
        QColor chosen = QColorDialog::getColor(QColor(m_clubs[m_current].color), this);
        if (chosen.isValid()) {
            m_clubs[m_current].color = chosen.name();
            refreshColorButton();
            refreshClubListRow(m_current);
        }
    });
    m_colorButton->setMenu(colorMenu);

    QHBoxLayout *nameRow = new QHBoxLayout();
    nameRow->addWidget(new QLabel("Name"));
    nameRow->addWidget(m_nameEdit, 1);
    nameRow->addSpacing(12);
    nameRow->addWidget(new QLabel("Color"));
    nameRow->addWidget(m_colorButton);

    // Schedule sub-tab: the rules, then the agenda
    QWidget *rulesContainer = new QWidget();
    m_rulesLayout = new QVBoxLayout(rulesContainer);
    m_rulesLayout->setContentsMargins(0, 0, 0, 0);
    m_rulesLayout->setSpacing(8);

    // A few cards, plus the agenda, are more than the dialog is tall.  The cards scroll rather
    //   than be squashed; see rebuildRuleCards() for how tall this is.
    m_rulesScroll = new QScrollArea();
    m_rulesScroll->setObjectName("clubsRulesScroll");
    m_rulesScroll->setWidget(rulesContainer);
    m_rulesScroll->setWidgetResizable(true);
    m_rulesScroll->setFrameShape(QFrame::NoFrame);
    m_rulesScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_rulesScroll->viewport()->setAutoFillBackground(false);
    rulesContainer->setAutoFillBackground(false);

    QFrame *addRuleCard = new QFrame();
    addRuleCard->setObjectName("clubAddRuleCard");
    QLabel *addRuleLink = new QLabel();
    addRuleLink->setObjectName("clubAddRuleLink");
    addRuleLink->setText(QString("<a href=\"rule\" style=\"color:%1; text-decoration:none;\">+ Add a schedule rule</a>"
                                 " &nbsp;&middot;&nbsp; "
                                 "<a href=\"oneoff\" style=\"color:%1; text-decoration:none;\">+ Add a one-off event</a>")
                         .arg(linkColor(m_theme)));
    addRuleLink->setAlignment(Qt::AlignCenter);
    QVBoxLayout *addRuleLayout = new QVBoxLayout(addRuleCard);
    addRuleLayout->setContentsMargins(8, 6, 8, 6);
    addRuleLayout->addWidget(addRuleLink);
    connect(addRuleLink, &QLabel::linkActivated, this, [this](const QString &link) {
        if (link == "oneoff") {
            addOneOff();
        } else {
            addRule();
        }
    });

    QFrame *agenda = new QFrame();
    agenda->setObjectName("clubsAgenda");

    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    QLabel *agendaTitle = new QLabel("EVENTS THESE RULES PRODUCE");
    agendaTitle->setObjectName("clubsAgendaTitle");
    QFont titleFont = mono;
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    agendaTitle->setFont(titleFont);

    m_agendaRange = new QLabel();
    m_agendaRange->setObjectName("clubsAgendaRange");
    m_agendaRange->setFont(mono);

    QWidget *agendaHeader = new QWidget();
    agendaHeader->setObjectName("clubsAgendaHeader");
    QHBoxLayout *agendaHeaderLayout = new QHBoxLayout(agendaHeader);
    agendaHeaderLayout->setContentsMargins(12, 7, 12, 7);
    agendaHeaderLayout->addWidget(agendaTitle);
    agendaHeaderLayout->addStretch(1);
    agendaHeaderLayout->addWidget(m_agendaRange);

    m_agendaTable = new QTableWidget(0, 4);
    m_agendaTable->setObjectName("clubsAgendaTable");
    m_agendaTable->horizontalHeader()->setVisible(false);
    m_agendaTable->verticalHeader()->setVisible(false);
    m_agendaTable->horizontalHeader()->setSectionResizeMode(kAgendaColDate, QHeaderView::ResizeToContents);
    m_agendaTable->horizontalHeader()->setSectionResizeMode(kAgendaColTime, QHeaderView::ResizeToContents);
    m_agendaTable->horizontalHeader()->setSectionResizeMode(kAgendaColName, QHeaderView::Stretch);
    m_agendaTable->horizontalHeader()->setSectionResizeMode(kAgendaColTag, QHeaderView::Fixed);   // holds cell widgets, which ResizeToContents doesn't measure
    m_agendaTable->setColumnWidth(kAgendaColTag, 84);   // room for "ONE-OFF"
    m_agendaTable->verticalHeader()->setDefaultSectionSize(26);
    m_agendaTable->setShowGrid(false);
    m_agendaTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_agendaTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_agendaTable->setFocusPolicy(Qt::NoFocus);
    m_agendaTable->setFrameShape(QFrame::NoFrame);
    m_agendaTable->setMinimumHeight(110);

    // The agenda only looks a few months ahead, and the rules go on past that.  This last line
    //   is what says so, right where the eye stops at the end of the list.
    m_agendaFooter = new QLabel();
    m_agendaFooter->setObjectName("clubsAgendaFooter");
    m_agendaFooter->setContentsMargins(12, 5, 12, 6);
    connect(m_agendaFooter, &QLabel::linkActivated, this, [this]() {
        m_agendaThrough = m_agendaThrough.addMonths(kAgendaMonths);
        refreshAgenda(true);   // to the new events at the end
    });

    QVBoxLayout *agendaLayout = new QVBoxLayout(agenda);
    agendaLayout->setContentsMargins(1, 1, 1, 1);
    agendaLayout->setSpacing(0);
    agendaLayout->addWidget(agendaHeader);
    agendaLayout->addWidget(m_agendaTable, 1);
    agendaLayout->addWidget(m_agendaFooter);

    QWidget *schedulePage = new QWidget();
    QVBoxLayout *scheduleLayout = new QVBoxLayout(schedulePage);
    scheduleLayout->setContentsMargins(8, 10, 8, 8);
    scheduleLayout->setSpacing(8);
    scheduleLayout->addWidget(m_rulesScroll, 2);
    scheduleLayout->addWidget(addRuleCard);
    scheduleLayout->addWidget(agenda, 1);

    // Settings and Calendar sync will be the other sub-tabs, later (see the issue)
    m_subTabs = new QTabWidget();
    m_subTabs->setObjectName("clubsSubTabs");
    m_subTabs->addTab(schedulePage, "Schedule");

    m_detail = new QWidget();
    QVBoxLayout *detailLayout = new QVBoxLayout(m_detail);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->addLayout(nameRow);
    detailLayout->addWidget(m_subTabs, 1);

    QHBoxLayout *top = new QHBoxLayout(this);
    top->addWidget(left);
    top->addSpacing(6);
    top->addWidget(m_detail, 1);

    // ----------------------------------------------------------------------------------------
    connect(m_clubList, &QTableWidget::currentCellChanged, this, [this](int row) { selectClub(row); });
    connect(m_nameEdit, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (m_current < 0) return;
        m_clubs[m_current].name = text;
        refreshClubListRow(m_current);
        refreshAgenda();   // events with no label are named after the club
    });
    connect(addButton,    &QToolButton::clicked, this, [this]() { addClub(); });
    connect(removeButton, &QToolButton::clicked, this, [this]() { removeClub(); });
    connect(upButton,     &QToolButton::clicked, this, [this]() { moveClub(-1); });
    connect(downButton,   &QToolButton::clicked, this, [this]() { moveClub(+1); });
}

// --------------------------------------------------------------------------------------------
void ClubsTab::setClubs(const QList<ClubInfo> &clubs)
{
    m_resetFromSessions->setVisible(bool(clubsFromSessions));
    m_clubs = clubs;
    m_current = m_clubs.isEmpty() ? -1 : 0;
    rebuildClubList();
    selectClub(m_current);
}

void ClubsTab::selectClubWithId(int id)
{
    for (int row = 0; row < m_clubs.size(); ++row) {
        if (id != -1 && m_clubs[row].id == id) {
            m_current = row;
            rebuildClubList();
            selectClub(row);
            return;
        }
    }
}

QList<ClubInfo> ClubsTab::clubs() const
{
    QList<ClubInfo> result = m_clubs;
    for (int i = 0; i < result.size(); ++i) {
        result[i].order_number = i;
        if (result[i].name.trimmed().isEmpty()) {
            result[i].name = "Untitled club";
        }
    }
    return result;
}

void ClubsTab::rebuildClubList()
{
    QSignalBlocker blocker(m_clubList);
    m_clubList->setRowCount(m_clubs.size());
    for (int row = 0; row < m_clubs.size(); ++row) {
        refreshClubListRow(row);
    }
    if (m_current >= 0) {
        m_clubList->setCurrentCell(m_current, kListColName);
    }
}

void ClubsTab::refreshClubListRow(int row)
{
    const ClubInfo &club = m_clubs[row];

    QTableWidgetItem *nameItem = m_clubList->item(row, kListColName);
    if (!nameItem) {
        nameItem = new QTableWidgetItem();
        m_clubList->setItem(row, kListColName, nameItem);
    }
    nameItem->setText(club.name);
    nameItem->setIcon(swatchIcon(club.color));

    // the day of the club's first recurring rule ("Wed"), or a dash for a club with none
    QTableWidgetItem *dayItem = m_clubList->item(row, kListColDay);
    if (!dayItem) {
        dayItem = new QTableWidgetItem();
        dayItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        dayItem->setForeground(QColor(m_theme == "Dark" ? "#8a8a8a" : "#7a7a7a"));
        m_clubList->setItem(row, kListColDay, dayItem);
    }
    auto firstRule = std::find_if(club.rules.cbegin(), club.rules.cend(), [](const ClubRule &r) { return !r.isOneOff(); });
    dayItem->setText(firstRule == club.rules.cend() ? QString::fromUtf8("—") : shortDayName(firstRule->weekday));
}

void ClubsTab::selectClub(int row)
{
    m_current = (row >= 0 && row < m_clubs.size()) ? row : -1;
    m_detail->setEnabled(m_current >= 0);
    if (m_current >= 0) {
        // Not re-sorted while a date is being edited, which would move the card out from under
        //   the cursor -- only when the club is selected again.
        sortClubRules(m_clubs[m_current].rules);
    }

    m_nameEdit->setText(m_current >= 0 ? m_clubs[m_current].name : QString());
    refreshColorButton();
    rebuildRuleCards();
    refreshAgenda();
}

void ClubsTab::refreshColorButton()
{
    m_colorButton->setIcon(swatchIcon(m_current >= 0 ? m_clubs[m_current].color : QString("#888888")));
}

void ClubsTab::rebuildRuleCards()
{
    // A card can ask for this from inside its own delete button's click, so the old cards are
    //   only scheduled for deletion, never deleted here.
    for (QWidget *card : std::as_const(m_ruleCards)) {
        card->hide();
        card->deleteLater();
    }
    m_ruleCards.clear();

    if (m_current < 0) {
        m_rulesScroll->setMaximumHeight(0);
        return;
    }

    const QList<ClubRule> &rules = m_clubs[m_current].rules;
    for (int i = 0; i < rules.size(); ++i) {
        auto onChanged = [this, i](const ClubRule &rule) {
            m_clubs[m_current].rules[i] = rule;
            refreshClubListRow(m_current);
            refreshAgenda();
        };
        auto onRemove = [this, i]() {
            m_clubs[m_current].rules.removeAt(i);
            rebuildRuleCards();
            refreshClubListRow(m_current);
            refreshAgenda();
        };
        QWidget *card;
        if (rules[i].isOneOff()) {
            ClubOneOffCard *oneOff = new ClubOneOffCard(rules[i]);
            oneOff->onChanged = onChanged;
            oneOff->onRemove = onRemove;
            card = oneOff;
        } else {
            ClubRuleCard *ruleCard = new ClubRuleCard(rules[i]);
            ruleCard->onChanged = onChanged;
            ruleCard->onRemove = onRemove;
            card = ruleCard;
        }
        m_rulesLayout->addWidget(card);
        card->show();   // now, not on the next pass through the event loop, so it counts in the height below
        m_ruleCards.append(card);
        stampTheme(card);
    }

    // No taller than the cards, so when they fit, the agenda gets all the rest.  When they don't,
    //   the cards and the agenda share the space, 2:1, and the cards scroll.
    m_rulesScroll->setMaximumHeight(m_rulesScroll->widget()->sizeHint().height());
}

void ClubsTab::refreshAgenda(bool scrollToEnd)
{
    QLocale locale = QLocale::system();
    const QDate today = QDate::currentDate();
    const QDate from = m_agendaFrom;

    QList<ClubOccurrence> events;
    QString footer;
    if (m_current >= 0) {
        const ClubInfo &club = m_clubs[m_current];
        events = clubOccurrencesBetween(club.rules, from, m_agendaThrough);
        footer = clubContinuationText(club.rules, m_agendaThrough);
    }

    m_agendaRange->setText(QString("%1 %2 %3  %4  %5 event%6")
                           .arg(locale.toString(from, "MMM d, yyyy"),
                                QString::fromUtf8("–"),
                                locale.toString(m_agendaThrough, "MMM d, yyyy"),
                                QString::fromUtf8("·"))
                           .arg(events.size())
                           .arg(events.size() == 1 ? "" : "s"));

    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_agendaTable->clearSpans();
    m_agendaTable->setRowCount(0);

    if (m_current >= 0 && m_clubs[m_current].rules.isEmpty()) {
        m_agendaTable->setRowCount(1);
        QTableWidgetItem *item = new QTableWidgetItem("No schedule yet. Add a rule or a one-off event above to see its events here.");
        item->setForeground(QColor(m_theme == "Dark" ? "#8a8a8a" : "#7a7a7a"));
        m_agendaTable->setItem(0, kAgendaColDate, item);
        m_agendaTable->setSpan(0, kAgendaColDate, 1, 4);
    }

    m_agendaTable->setRowCount(std::max(m_agendaTable->rowCount(), int(events.size())));
    for (int row = 0; row < events.size(); ++row) {
        const ClubOccurrence &event = events[row];
        const ClubInfo &club = m_clubs[m_current];
        const ClubRule &rule = club.rules[event.rule_index];

        // with the year: the agenda can span two of them
        QTableWidgetItem *dateItem = new QTableWidgetItem(locale.toString(event.date, "ddd MMM dd, yyyy"));
        dateItem->setFont(mono);
        QTableWidgetItem *timeItem = new QTableWidgetItem(timeRangeText(event.start_minutes, event.end_minutes));
        timeItem->setFont(mono);
        QTableWidgetItem *nameItem = new QTableWidgetItem(rule.label.trimmed().isEmpty() ? club.name : rule.label);

        m_agendaTable->setItem(row, kAgendaColDate, dateItem);
        m_agendaTable->setItem(row, kAgendaColTime, timeItem);
        m_agendaTable->setItem(row, kAgendaColName, nameItem);

        // events already past are muted, so today stands out as the line between them
        if (event.date < today) {
            QColor muted(m_theme == "Dark" ? "#7a7a7a" : "#9a9a9a");
            dateItem->setForeground(muted);
            timeItem->setForeground(muted);
            nameItem->setForeground(muted);
        }

        // where this event came from: a rule or a one-off.  Skipped / edited / ics come later.
        QLabel *tag = new QLabel(rule.isOneOff() ? "ONE-OFF" : "RULE");
        tag->setObjectName("clubsAgendaTag");
        tag->setProperty("tagKind", rule.isOneOff() ? "oneoff" : "rule");
        tag->setFont(mono);
        tag->setAlignment(Qt::AlignCenter);
        QWidget *tagCell = new QWidget();
        QHBoxLayout *tagLayout = new QHBoxLayout(tagCell);
        tagLayout->setContentsMargins(0, 0, 10, 0);
        tagLayout->addWidget(tag, 0, Qt::AlignRight | Qt::AlignVCenter);
        m_agendaTable->setCellWidget(row, kAgendaColTag, tagCell);
        stampTheme(tagCell);
        // sized from the stylesheet's (smaller) font, which only applies once it's polished
        QFontMetrics tagMetrics = tag->fontMetrics();
        tag->setFixedSize(tagMetrics.horizontalAdvance(tag->text()) + 14, tagMetrics.height() + 4);
    }

    // Start with the next event at the top, and the past ones just a scroll up.  Deferred, because
    //   the first time through the table hasn't been laid out yet, and has nothing to scroll.
    int firstUpcoming = -1;
    for (int row = 0; row < events.size(); ++row) {
        if (events[row].date >= today) {
            firstUpcoming = row;
            break;
        }
    }
    QTimer::singleShot(0, m_agendaTable, [this, scrollToEnd, firstUpcoming]() {
        if (scrollToEnd) {
            m_agendaTable->scrollToBottom();
        } else if (firstUpcoming >= 0 && m_agendaTable->item(firstUpcoming, kAgendaColDate)) {
            m_agendaTable->scrollToItem(m_agendaTable->item(firstUpcoming, kAgendaColDate), QAbstractItemView::PositionAtTop);
        }
    });

    if (footer.isEmpty()) {
        m_agendaFooter->hide();
    } else {
        m_agendaFooter->setText(QString("%1 %2 &nbsp;&middot;&nbsp; "
                                        "<a href=\"more\" style=\"color:%3; text-decoration:none;\">Show %4 more months</a>")
                                .arg(QString::fromUtf8("⋯"), footer.toHtmlEscaped(), linkColor(m_theme))
                                .arg(kAgendaMonths));
        m_agendaFooter->show();
    }
}

// --------------------------------------------------------------------------------------------
void ClubsTab::addClub()
{
    ClubInfo club;
    club.name = "New club";
    const QStringList palette = clubPaletteColors();
    club.color = palette[m_clubs.size() % palette.size()];

    int row = (m_current >= 0) ? m_current + 1 : m_clubs.size();
    m_clubs.insert(row, club);
    m_current = row;
    rebuildClubList();
    selectClub(row);
    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

void ClubsTab::removeClub()
{
    if (m_current < 0) {
        return;
    }
    m_clubs.removeAt(m_current);
    m_current = std::min(m_current, int(m_clubs.size()) - 1);
    rebuildClubList();
    selectClub(m_current);
}

void ClubsTab::moveClub(int delta)
{
    int target = m_current + delta;
    if (m_current < 0 || target < 0 || target >= m_clubs.size()) {
        return;
    }
    m_clubs.swapItemsAt(m_current, target);
    m_current = target;
    rebuildClubList();
}

void ClubsTab::addRule()
{
    if (m_current < 0) {
        return;
    }
    // A new rule starts as this weekday, evenings, from today on.  Most of the time the user
    //   only has to change the day and the times.
    ClubRule rule;
    rule.weekday = QDate::currentDate().dayOfWeek();
    rule.since = QDate::currentDate();
    QList<ClubRule> &rules = m_clubs[m_current].rules;
    rules.append(rule);
    sortClubRules(rules);   // above the one-offs
    int added = 0;
    while (added + 1 < rules.size() && !rules[added + 1].isOneOff()) {
        ++added;            // the last recurring rule, which is the new one
    }
    rebuildRuleCards();
    scrollToRuleCard(added);
    refreshClubListRow(m_current);
    refreshAgenda();
}

void ClubsTab::addOneOff()
{
    if (m_current < 0) {
        return;
    }
    // A new one-off starts at the club's next regular event, so its times are usually right and
    //   only the date needs changing.  With no recurring rule, today at the default times.
    QList<ClubRule> recurring;
    for (const ClubRule &r : std::as_const(m_clubs[m_current].rules)) {
        if (!r.isOneOff()) {
            recurring.append(r);
        }
    }
    ClubRule rule;
    rule.since = QDate::currentDate();
    const QList<ClubOccurrence> next = clubOccurrencesBetween(recurring, QDate::currentDate(), QDate::currentDate().addYears(1));
    if (!next.isEmpty()) {
        rule.since = next.first().date;
        rule.start_minutes = next.first().start_minutes;
        rule.end_minutes = next.first().end_minutes;
    }
    rule.kind = ClubRule::Once;
    rule.until = rule.since;
    rule.weekday = rule.since.dayOfWeek();
    QList<ClubRule> &rules = m_clubs[m_current].rules;
    rules.append(rule);
    sortClubRules(rules);
    int added = 0;
    for (int i = 0; i < rules.size(); ++i) {
        if (rules[i].isOneOff() && rules[i].since <= rule.since) {
            added = i;      // the sort is stable, so the new one is the last on or before its date
        }
    }
    rebuildRuleCards();
    scrollToRuleCard(added);
    refreshAgenda();
}

// The cards scroll, so a new one may land out of sight.  Deferred until the cards are laid out.
void ClubsTab::scrollToRuleCard(int index)
{
    if (index < 0 || index >= m_ruleCards.size()) {
        return;
    }
    QWidget *card = m_ruleCards[index];
    QTimer::singleShot(0, card, [this, card]() { m_rulesScroll->ensureWidgetVisible(card, 0, 0); });
}
