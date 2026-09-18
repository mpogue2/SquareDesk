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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include <QColorDialog>
#include <QMessageBox>
#include "keybindings.h"
#include <QKeySequenceEdit>
#include <QTimeEdit>
#include "sessioninfo.h"
#include "applemusicfilter.h"
#include <algorithm>
#include <utility>

// Preferences > Apple Music (issue #1740, item 6)
#include <QApplication>
#include <QFileInfo>
#include <QSet>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QRadioButton>
#include <QScrollBar>
#include <QSplitter>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include "globaldefines.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "utility.h"

extern flexible_audio *cBass;    // global in MainWindow.cpp <-- use this on M1 Silicon

static const int kSessionsColName = 0;
static const int kSessionsColDay = 1;
static const int kSessionsColTime = 2;
static const int kSessionsColID = 3;

static const int kTagsColTag = 0;
static const int kTagsColForeground = 1;
static const int kTagsColBackground = 2;
static const int kTagsColExample = 3;

static const char *strWhiteHashFFFFFF = "#ffffff";


static const QString COLOR_STYLE("QPushButton { background-color : %1; color : %1; }");

static void  SetTimerPulldownValuesToFirstDigit(QComboBox *comboBox)
{
    for (int i = 0; i < comboBox->count(); ++i)
    {
        QString theText = comboBox->itemText(i);
        QStringList sl1 = theText.split(" ");
        QString mText = sl1[0];
        int length = mText.toInt();
        comboBox->setItemData(i, QVariant(length));
    }
}
static void SetPulldownValuesToItemNumberPlusN(QComboBox *comboBox, int N)
{
    for (int i = 0; i < comboBox->count(); ++i)
    {
        comboBox->setItemData(i, QVariant(i + N));
    }
}


static void RemoveAllOtherHotkeysFromTable(QTableWidget *tableWidgetKeyBindings,
                                           int rowNewKey,
                                           int colNewKey,
                                           QKeySequence keySequenceNewKey)
{
    for (int row = 0; row < tableWidgetKeyBindings->rowCount(); ++row)
    {
        for (int col = 1; col < tableWidgetKeyBindings->columnCount(); ++col)
        {
            if (row != rowNewKey || col != colNewKey)
            {
                QKeySequenceEdit *keySequenceEdit = dynamic_cast<QKeySequenceEdit*>(tableWidgetKeyBindings->cellWidget(row,col));
                QKeySequence keySequence = keySequenceEdit->keySequence();
                if (keySequence.toString() == keySequenceNewKey.toString())
                {
                    keySequenceEdit->clear();
                }
            }
        }
    }
}

// https://stackoverflow.com/questions/47195261/qtableview-disable-sorting-for-some-columns
void PreferencesDialog::onSortIndicatorChanged(int column, Qt::SortOrder order)
{
    if (column == kTagsColTag){
        // Record the sort order when it is by "tag name"
        sortOrder = order;
    }
    else {
        // Restore the "tag name" sort order (ignoring all other columns)
        ui->tableWidgetTagColors->sortByColumn(kTagsColTag, sortOrder);
    }
}

// -------------------------------------------------------------------
PreferencesDialog::PreferencesDialog(QMap<int, QString> *soundFXname, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    mw = (MainWindow *)parent;
    swallowSoundFX = true;
    songTableReloadNeeded = false;

    ui->setupUi(this);

    ui->currentThemeLabel->setText("[Theme: " + mw->currentThemeString + "]");

    // minVolume feature is now deprecated. This will be removed in the near future.
    ui->minVolumeLabel->setVisible(false);
    ui->limitVolumespinBox->setVisible(false);

    {
        ui->tableWidgetSessionsList->setColumnHidden(kSessionsColID, true);
        QHeaderView *headerView = ui->tableWidgetSessionsList->horizontalHeader();
        headerView->setSectionResizeMode(kSessionsColName, QHeaderView::Stretch);
        headerView->setSectionResizeMode(kSessionsColDay, QHeaderView::Interactive);
        headerView->setSectionResizeMode(kSessionsColTime, QHeaderView::Stretch);
    }

    {
        // disable column resizing
        QHeaderView *headerView = ui->tableWidgetTagColors->horizontalHeader();
        headerView->setSectionResizeMode(kTagsColTag,        QHeaderView::Fixed);
        headerView->setSectionResizeMode(kTagsColForeground, QHeaderView::Fixed);
        headerView->setSectionResizeMode(kTagsColBackground, QHeaderView::Fixed);
        headerView->setSectionResizeMode(kTagsColExample,    QHeaderView::Stretch);

        // disable sorting on columns OTHER than the tag name column
        connect(ui->tableWidgetTagColors->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, &PreferencesDialog::onSortIndicatorChanged);
    }

// validator for initial BPM setting
    validator = new QIntValidator(100, 150, this);
    ui->initialBPMLineEdit->setValidator(validator);

    setFontSizes();

    // settings for experimental break/tip timers are:
    SetTimerPulldownValuesToFirstDigit(ui->breakLength);
    SetTimerPulldownValuesToFirstDigit(ui->longTipLength);

    SetPulldownValuesToItemNumberPlusN(ui->comboBoxMusicFormat,1);
    SetPulldownValuesToItemNumberPlusN(ui->comboBoxAnimationSettings,0);
    SetPulldownValuesToItemNumberPlusN(ui->comboBoxSessionDefault,1);

    // puldown menus for break and long tip sounds
    ui->afterLongTipAction->clear();
    ui->afterLongTipAction->addItem("visual indicator only");
    ui->afterLongTipAction->addItem("play long tip reminder tone");

    ui->afterBreakAction->clear();
    ui->afterBreakAction->addItem("visual indicator only");
    ui->afterBreakAction->addItem("play break over reminder tone");

    QMapIterator<int, QString> i(*soundFXname);
    while (i.hasNext()) {
        i.next();
//        qDebug() << i.key() << ": " << i.value();
        QString thisName = i.value();
        if (thisName != QString("")) {
            ui->afterLongTipAction->addItem("play " + thisName + " sound");
            ui->afterBreakAction->addItem("play " + thisName + " sound");
        } else {
            ui->afterLongTipAction->addItem("--disabled--");
            ui->afterBreakAction->addItem("--disabled--");
        }
    }


    SetPulldownValuesToItemNumberPlusN(ui->afterLongTipAction, 2); // 0 = visual only, 1 = long tip tone
    SetPulldownValuesToItemNumberPlusN(ui->afterBreakAction, 2);   // 0 = visual only, 1 = break over tone

    // ---------------------
    QVector<KeyAction*> availableActions(KeyAction::availableActions());

    QTableWidget *tableWidgetKeyBindings = ui->tableWidgetKeyBindings;

    tableWidgetKeyBindings->setRowCount(availableActions.length());
    for (int row = 0; row < availableActions.length(); ++row)
    {
        QTableWidgetItem *newTableItem(new QTableWidgetItem(availableActions[row]->name()));
        newTableItem->setFlags(newTableItem->flags() & ~Qt::ItemIsEditable);
        tableWidgetKeyBindings->setItem(row, 0, newTableItem);
        
        for (int col = 1; col < tableWidgetKeyBindings->columnCount(); ++col)
        {
            QKeySequenceEdit *keySequenceEdit(new QKeySequenceEdit);
#if QT_VERSION > QT_VERSION_CHECK(6, 4, 0)
            keySequenceEdit->setClearButtonEnabled(true);  // allow single-click clear of existing shortcut
#endif
            tableWidgetKeyBindings->setCellWidget(row, col, keySequenceEdit);
            connect(keySequenceEdit, &QKeySequenceEdit::editingFinished, this, [keySequenceEdit, tableWidgetKeyBindings, row, col]()
                    {
                        RemoveAllOtherHotkeysFromTable(tableWidgetKeyBindings, row, col, keySequenceEdit->keySequence());
                    });
        }
    }

    ui->tableWidgetKeyBindings->resizeColumnToContents(0);
    ui->tableWidgetKeyBindings->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    // qDebug() << "setting tab to music!";
    ui->tabWidget->setCurrentIndex(0); // Music tab (not Experimental tab) is primary, regardless of last setting in Qt Designer

    on_intelBoostEnabledCheckbox_toggled(ui->intelBoostEnabledCheckbox->isChecked());

    setupAppleMusicTab();
}


// ----------------------------------------------------------------
PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

// ----------------------------------------------------------------
void PreferencesDialog::setFontSizes()
{

    int preferredSmallFontSize;
#if defined(Q_OS_MAC)
    preferredSmallFontSize = 11;
#elif defined(Q_OS_WIN32)
    preferredSmallFontSize = 8;
#elif defined(Q_OS_LINUX)
    preferredSmallFontSize = 9;
#endif

    QFont font = ui->musicDirHelpLabel->font();
    font.setPointSize(preferredSmallFontSize);

    ui->musicDirHelpLabel->setFont(font);
    ui->clockColoringHelpLabel->setFont(font);
    ui->musicTypesHelpLabel->setFont(font);
    ui->musicFormatHelpLabel->setFont(font);
//    ui->saveSongPrefsHelpLabel->setFont(font);
    ui->labelToggleSequenceHelpTextLabel->setFont(font);
}

static int getLastSelectedRow(QTableWidget *tableWidget)
{
    int lastSelectedRow = -1;
    for (int row = 0; row < tableWidget->rowCount(); ++row)
    {
        for (int col = 0; col < tableWidget->columnCount(); ++col)
        {
            QTableWidgetItem *item = tableWidget->item(row,col);
            if (item && item->isSelected())
            {
                lastSelectedRow = row;
            }
        }
    }
    if (lastSelectedRow < 0)
    {
        lastSelectedRow = tableWidget->rowCount();
    }
    return lastSelectedRow;
}

static void setPushButtonColor(QPushButton *pushButton, QString color)
{
    QString styleSheet(COLOR_STYLE.arg(color));
    pushButton->setStyleSheet(styleSheet);
    pushButton->setFlat(true);
    pushButton->setAutoFillBackground(true);
    pushButton->setText(color);
}

static void setTagColorsSample(QTableWidget *tableWidget, int row)
{
    QString tag(tableWidget->item(row, kTagsColTag)->text().toHtmlEscaped());
    PushButtonColorTag *foregroundButton(dynamic_cast<PushButtonColorTag*>(tableWidget->cellWidget(row, kTagsColForeground)));
    PushButtonColorTag *backgroundButton(dynamic_cast<PushButtonColorTag*>(tableWidget->cellWidget(row, kTagsColBackground)));
    QLabel *labelItem(dynamic_cast<QLabel *>(tableWidget->cellWidget(row, kTagsColExample)));

    if (labelItem) {
        QString foreground = foregroundButton->text();
        QString background = backgroundButton->text();
        QString str("<span style=\"background-color:%1; color: %2;\">&nbsp;");
        str = str.arg(background).arg(foreground);
        str += tag;
        str += "&nbsp;</span>";
        labelItem->setText(str);
    } else {
        qDebug() << "ERROR: setTagColorsSample::labelItem was NULL!";
    }
}

PushButtonColorTag::PushButtonColorTag(PreferencesDialog *prefsDialog,
                                       const QString &tagName,
                                       const QString &initialColor,
                                       bool foreground) :
        prefsDialog(prefsDialog), tagName(tagName), color(initialColor), foreground(foreground)
{
    setPushButtonColor(this, initialColor);
    connect(this, SIGNAL(clicked()), this, SLOT(selectColor()));
}

void PushButtonColorTag::selectColor()
{
    QColor chosenColor = QColorDialog::getColor(QColor(color), this); //return the color chosen by user
    if (chosenColor.isValid())
    {
        color = chosenColor.name();
        setPushButtonColor(this, color);
        prefsDialog->setTagColor(tagName, color, foreground);
    }
}



static void addRowToTagColors(PreferencesDialog *prefsDialog, QTableWidget *tableWidget, QString tag, QString background, QString foreground)
{
    PushButtonColorTag *backgroundButton(new PushButtonColorTag(prefsDialog, tag, background, false));
    PushButtonColorTag *foregroundButton(new PushButtonColorTag(prefsDialog, tag, foreground, false));
    // int row = getLastSelectedRow(tableWidget);
    int row = tableWidget->rowCount(); // always add at the end, since table is auto-sorted nowadays
    tableWidget->insertRow(row);
    tableWidget->setCellWidget(row, kTagsColForeground, foregroundButton);
    tableWidget->setCellWidget(row, kTagsColBackground, backgroundButton);
    QTableWidgetItem *widgetItem(new QTableWidgetItem( tag ));
    tableWidget->setItem(row, kTagsColTag, widgetItem);
    
    QLabel *labelItem(new QLabel(tableWidget));
    labelItem->setAlignment(Qt::AlignCenter);
    labelItem->setTextFormat(Qt::RichText);
    tableWidget->setCellWidget(row, kTagsColExample, labelItem);
    // NOTE: the text for labelItem will be set by setTagColorsSample() below.

    setPushButtonColor(backgroundButton, background);
    setPushButtonColor(foregroundButton, foreground);
    setTagColorsSample(tableWidget, row);
}


void PreferencesDialog::on_pushButtonTagAdd_clicked()
{
    songTableReloadNeeded = true;
    ui->tableWidgetTagColors->setSortingEnabled(false);
    addRowToTagColors(this, ui->tableWidgetTagColors, "<NEWTAG>", ui->pushButtonTagsBackgroundColor->text(), ui->pushButtonTagsForegroundColor->text());
    ui->tableWidgetTagColors->setSortingEnabled(true);
}

void PreferencesDialog::on_pushButtonTagRemove_clicked()
{
    songTableReloadNeeded = true;
    for (int row = 0; row < ui->tableWidgetTagColors->rowCount(); ++row)
    {
        for (int col = 0; col < ui->tableWidgetTagColors->columnCount(); ++col)
        {
            QTableWidgetItem *item = ui->tableWidgetTagColors->item(row,col);
            if (item && item->isSelected())
            {
                ui->tableWidgetTagColors->removeRow(row);
                --row;
                break;
            }
        } /* end of column iteration */
    } /* end of row iteration */
}

void PreferencesDialog::setTagColors( const QHash<QString,QPair<QString,QString>> &colors)
{
    songTableReloadNeeded = true;

    ui->tableWidgetTagColors->setSortingEnabled(false);
    for (auto color = colors.cbegin(); color != colors.cend(); ++color)
    {
        addRowToTagColors(this, ui->tableWidgetTagColors, color.key(), color.value().first, color.value().second);
    }
    
    ui->tableWidgetTagColors->setSortingEnabled(true);
    ui->tableWidgetTagColors->sortByColumn(kTagsColTag, Qt::AscendingOrder);

}

void PreferencesDialog::setTagColor(const QString &tagName, const QString & /* color */, bool /* foreground */)
{
    for (int row = 0; row < ui->tableWidgetTagColors->rowCount(); ++row)
    {
        if (0 == tagName.compare(ui->tableWidgetTagColors->item(row, kTagsColTag)->text()))
        {
            setTagColorsSample(ui->tableWidgetTagColors, row);
        }
    }
    songTableReloadNeeded = true;
}

QHash<QString,QPair<QString,QString>> PreferencesDialog::getTagColors()
{
    QHash<QString,QPair<QString,QString>> tagColors;
    for (int row = 0; row < ui->tableWidgetTagColors->rowCount(); ++row)
    {
        QTableWidgetItem *item = ui->tableWidgetTagColors->item(row, kTagsColTag);
        QString tagName = item->text();
        QPushButton *foregroundButton = dynamic_cast<QPushButton*>(ui->tableWidgetTagColors->cellWidget(row, kTagsColForeground));
        QPushButton *backgroundButton = dynamic_cast<QPushButton*>(ui->tableWidgetTagColors->cellWidget(row, kTagsColBackground));
        tagColors[tagName] =
            QPair<QString, QString>(backgroundButton->text(),
                                    foregroundButton->text());
    }
    return tagColors;
}

QHash<QString, KeyAction *> PreferencesDialog::getHotkeys()
{
    QHash<QString, KeyAction *> keyActionBindings;
    QHash<QString, KeyAction*> actions(KeyAction::actionNameToActionMappings());
    for (int row = 0; row < ui->tableWidgetKeyBindings->rowCount(); ++row)
    {
        QString actionName(ui->tableWidgetKeyBindings->item(row, 0)->text());


        for (int col = 1; col < ui->tableWidgetKeyBindings->columnCount(); ++col)
        {
            QKeySequenceEdit *keySequenceEdit = dynamic_cast<QKeySequenceEdit*>(ui->tableWidgetKeyBindings->cellWidget(row,col));
            QKeySequence keySequence = keySequenceEdit->keySequence();
            keyActionBindings[keySequence.toString()] = actions[actionName];
        }
    }

    return keyActionBindings;
}


bool LongStringsFirstThenAlpha(const QString &a, const QString &b)
{
    if (a.length() > 1 && b.length() > 1)
    {
        return a < b;
    }
    else if (a.length() > 1)
    {
        return true;
    }
    else if (b.length() > 1)
    {
        return false;
    }
    return a < b;
          
}

void PreferencesDialog::setHotkeys(QHash<QString, KeyAction *> keyActions)
{
    QHash<QString, QStringList> keysByActionName;

    for (QHash<QString, KeyAction *>::iterator keyAction = keyActions.begin();
         keyAction != keyActions.end();
         ++keyAction)
    {
        keysByActionName[keyAction.value()->name()].append(keyAction.key());
    }

    for (int row = 0; row < ui->tableWidgetKeyBindings->rowCount(); ++row)
    {
        QString actionName(ui->tableWidgetKeyBindings->item(row, 0)->text());
        auto keyAction = keysByActionName.find(actionName);
        QStringList keys;
        if (keyAction != keysByActionName.end())
            keys = keyAction.value();
        std::sort(keys.begin(), keys.end(), LongStringsFirstThenAlpha);

        for (int col = 1;
             col < ui->tableWidgetKeyBindings->columnCount();
             ++col)
        {
            QKeySequenceEdit *keySequenceEdit = dynamic_cast<QKeySequenceEdit *>(ui->tableWidgetKeyBindings->cellWidget(row, col));

            if (col <= keys.length())
            {
                QKeySequence sequence(QKeySequence::fromString(keys[col - 1]));
                keySequenceEdit->setKeySequence(sequence);
            }
            else
            {
                keySequenceEdit->setKeySequence(QKeySequence());
            }
        }
    }
}

static QComboBox *weekSelectionComboBox()
{
    QComboBox *comboDay = new QComboBox();

    comboDay->addItem("<none>", 0);  // note that this is not localized

    // comboDay->addItem("Monday", 1);
    // comboDay->addItem("Tuesday", 2);
    // comboDay->addItem("Wednesday", 3);
    // comboDay->addItem("Thursday", 4);
    // comboDay->addItem("Friday", 5);
    // comboDay->addItem("Saturday", 6);
    // comboDay->addItem("Sunday", 7);

    QLocale locale = QLocale::system(); // Use system locale, or specify one like QLocale("fr_FR")

    // Get day names, starting with Monday
    for (int i = 1; i <= 7; i++) {
        // Get long format (Monday, Tuesday, etc.)
        QString longName = locale.dayName(i, QLocale::LongFormat);
        comboDay->addItem(longName, i); // populate dropdown menu with localized names for the days of the week
    }

    return comboDay;
}


static QTimeEdit *timeSelectionControl(int start_minutes)
{
    QTimeEdit *timeEdit = new QTimeEdit(QTime((int)(start_minutes / 60),
                                              start_minutes % 60));

    // NEW: use the locale to set the timeEdit displayFormat
    // Get the current system locale
    QLocale locale = QLocale::system();

    // Set the display format based on the locale's date and time formats
    QString timeFormat = locale.timeFormat(QLocale::ShortFormat);

    // Apply the format to the widget
    timeEdit->setDisplayFormat(timeFormat);

    // timeEdit->setDisplayFormat("h:mm AP");
    return timeEdit;
}


void PreferencesDialog::setSessionInfoList(const QList<SessionInfo> &sessions)
{
    ui->tableWidgetSessionsList->setRowCount(sessions.length());
    for (int row = 0; row < sessions.length(); ++row)
    {
        const SessionInfo &session(sessions[row]);
        QTableWidgetItem *item = new QTableWidgetItem(session.name);
        ui->tableWidgetSessionsList->setItem(row, 0, item);

        QComboBox *comboDay = weekSelectionComboBox();
        setDynamicPropertyRecursive(comboDay, "theme", mw->currentThemeString);

        comboDay->setCurrentIndex(session.day_of_week);
        ui->tableWidgetSessionsList->setCellWidget(row, 1, comboDay);

        QTimeEdit *timeEdit = timeSelectionControl(session.start_minutes);
        setDynamicPropertyRecursive(timeEdit, "theme", mw->currentThemeString);
                
        ui->tableWidgetSessionsList->setCellWidget(row,2,timeEdit);
        
        QTableWidgetItem *itemId = new QTableWidgetItem(QString("%1").arg(session.id));
        ui->tableWidgetSessionsList->setItem(row,3,itemId);
    }
 
}


QList<SessionInfo> PreferencesDialog::getSessionInfoList()
{
    QList<SessionInfo> info;
    for (int row = 0; row < ui->tableWidgetSessionsList->rowCount(); ++row)
    {
        SessionInfo session;

        session.name = ui->tableWidgetSessionsList->item(row,kSessionsColName)->text();;
        QComboBox *comboDay = dynamic_cast<QComboBox *>(ui->tableWidgetSessionsList->cellWidget(row,kSessionsColDay));
        if (comboDay)
            session.day_of_week = comboDay->currentIndex();
        QTimeEdit *timeEdit = dynamic_cast<QTimeEdit *>(ui->tableWidgetSessionsList->cellWidget(row,kSessionsColTime));
        QTime t(timeEdit->time());
        session.start_minutes = t.hour() * 60 + t.minute();
        session.id = -1;
        if (ui->tableWidgetSessionsList->item(row, kSessionsColID) != nullptr) // NULL)
        {
            QString session_id(ui->tableWidgetSessionsList->item(row, kSessionsColID)->text());
            session.id = session_id.toInt();
        }
        session.order_number = row;
        info.append(session);
    }
    return info;
}


void PreferencesDialog::on_toolButtonSessionAddItem_clicked()
{
    int lastSelectedRow = getLastSelectedRow(ui->tableWidgetSessionsList);
//    ui->tableWidgetSessionsList->setRowCount(ui->tableWidgetSessionsList->rowCount() + 1);
    ui->tableWidgetSessionsList->insertRow(lastSelectedRow);
    QComboBox *comboDay = weekSelectionComboBox();
    comboDay->setCurrentIndex(0);
    ui->tableWidgetSessionsList->setCellWidget(lastSelectedRow, kSessionsColDay, comboDay);
    QTimeEdit *timeEdit = timeSelectionControl(0);
    ui->tableWidgetSessionsList->setCellWidget(lastSelectedRow, kSessionsColTime, timeEdit);
}

void PreferencesDialog::on_toolButtonSessionRemoveItem_clicked()
{
    for (int row = 0; row < ui->tableWidgetSessionsList->rowCount(); ++row)
    {
        for (int col = 0; col < ui->tableWidgetSessionsList->columnCount(); ++col)
        {
            QTableWidgetItem *item = ui->tableWidgetSessionsList->item(row,col);
            if (item && item->isSelected())
            {
                ui->tableWidgetSessionsList->removeRow(row);
                --row;
                break;
            }
        } /* end of column iteration */
    } /* end of row iteration */
    
}


static void swapSessionInfoRows(QTableWidget *tableWidget, int rowA, int rowB)
{
    // Doing selection state for both rows independently 'cause who
    // knows what's going on with swapping values vs widgets.
    
    bool *selectedA = new bool[tableWidget->columnCount()];
    bool *selectedB = new bool[tableWidget->columnCount()];
    for (int col = 0; col < tableWidget->columnCount(); col++)
    {
        {
            QTableWidgetItem *item = tableWidget->item(rowA,col);
            selectedA[col] = item && item->isSelected();
        }
        {
            QTableWidgetItem *item = tableWidget->item(rowB,col);
            selectedB[col] = item && item->isSelected();
        }
    }
    
    QTableWidgetItem *itemName(tableWidget->takeItem(rowA, kSessionsColName));
    QComboBox *comboDayA = dynamic_cast<QComboBox *>(tableWidget->cellWidget(rowA,kSessionsColDay));
    int day_of_week = comboDayA->currentIndex();
    QTimeEdit *timeEditA = dynamic_cast<QTimeEdit *>(tableWidget->cellWidget(rowA,kSessionsColTime));
    QTime t(timeEditA->time());
    QTableWidgetItem *itemID(tableWidget->takeItem(rowA, kSessionsColID));


    tableWidget->setItem(rowA, kSessionsColName, tableWidget->takeItem(rowB, kSessionsColName));
    QComboBox *comboDayB = dynamic_cast<QComboBox *>(tableWidget->cellWidget(rowB,kSessionsColDay));
    comboDayA->setCurrentIndex(comboDayB->currentIndex());
    QTimeEdit *timeEditB = dynamic_cast<QTimeEdit *>(tableWidget->cellWidget(rowB,kSessionsColTime));
    timeEditA->setTime(timeEditB->time());
    tableWidget->setItem(rowA, kSessionsColID, tableWidget->takeItem(rowB, kSessionsColID));

    tableWidget->setItem(rowB, kSessionsColName, itemName);
    comboDayB->setCurrentIndex(day_of_week);
    timeEditB->setTime(t);
    tableWidget->setItem(rowB, kSessionsColID, itemID);

    for (int col = 0; col < tableWidget->columnCount(); col++)
    {
        {
            QTableWidgetItem *item = tableWidget->item(rowA,col);
            if (item) item->setSelected(selectedB[col]);
        }
        {
            QTableWidgetItem *item = tableWidget->item(rowB,col);
            if (item) item->setSelected(selectedA[col]);
        }
    }
    delete[] selectedA;
    delete[] selectedB;
}


void PreferencesDialog::on_toolButtonSessionMoveItemDown_clicked()
{
    for (int row = ui->tableWidgetSessionsList->rowCount() - 2; row >= 0 ; --row)
    {
        bool rowSelected = false;
        for (int col = 0; col < ui->tableWidgetSessionsList->columnCount(); ++col)
        {
            QTableWidgetItem *item = ui->tableWidgetSessionsList->item(row,col);
            if (item && item->isSelected())
            {
                rowSelected = true;
            }
        } /* end of column iteration */
        if (rowSelected)
        {
            swapSessionInfoRows(ui->tableWidgetSessionsList, row, row + 1);
        }
    } /* end of row iteration */
}

void PreferencesDialog::on_toolButtonSessionMoveItemUp_clicked()
{
    for (int row = 1; row < ui->tableWidgetSessionsList->rowCount(); ++row)
    {
        bool rowSelected = false;
        for (int col = 0; col < ui->tableWidgetSessionsList->columnCount(); ++col)
        {
            QTableWidgetItem *item = ui->tableWidgetSessionsList->item(row,col);
            if (item && item->isSelected())
            {
                rowSelected = true;
            }
        } /* end of column iteration */
        if (rowSelected)
        {
            swapSessionInfoRows(ui->tableWidgetSessionsList, row, row - 1);
        }
        
    } /* end of row iteration */
}




void PreferencesDialog::on_pushButtonResetHotkeysToDefaults_clicked()
{
    // QMessageBox::StandardButton reply;
    // reply = QMessageBox::question(this, "Reset Hotkeys To Defaults",
    //                               "Do you really want to reset all hotkeys back to their default? This operation cannot be undone.",
    //                               QMessageBox::Yes|QMessageBox::No);
    // if (reply == QMessageBox::Yes)
    // {
    //     setHotkeys(KeyAction::defaultKeyToActionMappings());
    // }

    QMessageBox msgBox;
    msgBox.setText("Resetting all hotkeys back to their default cannot be undone.");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("OK to proceed?");
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes) {
        setHotkeys(KeyAction::defaultKeyToActionMappings());
    }
}

void PreferencesDialog::on_comboBoxMusicFormat_currentIndexChanged(int /* currentIndex */)
{
    songTableReloadNeeded = true;
}


void PreferencesDialog::on_chooseMusicPathButton_clicked()
{
    QString dir =
        QFileDialog::getExistingDirectory(this, tr("Select Base Directory for Music"),
                                          QDir::homePath(),
                                          QFileDialog::ShowDirsOnly
                                          | QFileDialog::DontResolveSymlinks);

    if (dir.isNull()) {
        return;  // user cancelled the "Select Base Directory for Music" dialog...so don't do anything, just return
    }

    songTableReloadNeeded = true;  // change to Music Directory requires reload of the songTable

    ui->musicPath->setText(dir);
    musicPath = dir;
}

// ------------

void PreferencesDialog::on_calledColorButton_clicked()
{
    QString calledColorString = ui->calledColorButton->text();
    QColor chosenColor = QColorDialog::getColor(QColor(calledColorString), this); //return the color chosen by user
    if (chosenColor.isValid()) {
        calledColorString = chosenColor.name();

        if (chosenColor.name() == strWhiteHashFFFFFF) {
            calledColorString = DEFAULTCALLEDCOLOR;  // a way to reset the colors individually
        }

        setPushButtonColor(ui->calledColorButton, calledColorString);
        songTableReloadNeeded = true;  // change to colors requires reload of the songTable
    }
}

void PreferencesDialog::on_extrasColorButton_clicked()
{
    QString extrasColorString = ui->extrasColorButton->text();
    QColor chosenColor = QColorDialog::getColor(QColor(extrasColorString), this); //return the color chosen by user
    if (chosenColor.isValid()) {
        extrasColorString = chosenColor.name();

        if (chosenColor == strWhiteHashFFFFFF) {
            extrasColorString = DEFAULTEXTRASCOLOR;  // a way to reset the colors individually
        }

        setPushButtonColor(ui->extrasColorButton, extrasColorString);
        songTableReloadNeeded = true;  // change to colors requires reload of the songTable
    }
}

void PreferencesDialog::on_patterColorButton_clicked()
{
    QString patterColorString = ui->patterColorButton->text();
    QColor chosenColor = QColorDialog::getColor(QColor(patterColorString), this); //return the color chosen by user
    if (chosenColor.isValid()) {
        patterColorString = chosenColor.name();

        if (chosenColor == strWhiteHashFFFFFF) {
            patterColorString = DEFAULTPATTERCOLOR;  // a way to reset the colors individually
        }

        setPushButtonColor(ui->patterColorButton, patterColorString);
        songTableReloadNeeded = true;  // change to colors requires reload of the songTable
    }
}

void PreferencesDialog::on_singingColorButton_clicked()
{
    QString singingColorString = ui->singingColorButton->text();
    QColor chosenColor = QColorDialog::getColor(QColor(singingColorString), this); //return the color chosen by user
    if (chosenColor.isValid()) {
        singingColorString = chosenColor.name();

        if (chosenColor == strWhiteHashFFFFFF) {
            singingColorString = DEFAULTSINGINGCOLOR;  // a way to reset the colors individually
        }

        setPushButtonColor(ui->singingColorButton, singingColorString);
        songTableReloadNeeded = true;  // change to colors requires reload of the songTable
    }
}


/* See the large comment at the top of prefs_options.h */

#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_INT_NO_PREFS(name,default)

#define CONFIG_ATTRIBUTE_STRING(control, name, default)                 \
    QString PreferencesDialog::Get##name() const { return ui->control->text(); } \
    void PreferencesDialog::Set##name(QString value) { ui->control->setText(value); }

#define CONFIG_ATTRIBUTE_COLOR(control, name, default)                 \
    QString PreferencesDialog::Get##name() const { return ui->control->text(); } \
    void PreferencesDialog::Set##name(QString value) \
    { \
        setPushButtonColor(ui->control, value); \
    }

#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) \
    bool PreferencesDialog::Get##name() const { return ui->control->isChecked(); } \
    void PreferencesDialog::Set##name(bool value) { ui->control->setChecked(value); }

#define CONFIG_ATTRIBUTE_COMBO(control, name, default) \
    int PreferencesDialog::Get##name() const { return ui->control->itemData(ui->control->currentIndex()).toInt(); } \
    void PreferencesDialog::Set##name(int value) \
    { for (int i = 0; i < ui->control->count(); ++i) { \
            if (ui->control->itemData(i).toInt() == value) { ui->control->setCurrentIndex(i); break; } \
        } }

#define CONFIG_ATTRIBUTE_INT(control, name, default)                 \
    int PreferencesDialog::Get##name() const { return ui->control->text().toInt(); } \
    void PreferencesDialog::Set##name(int value) { ui->control->setText(QString::number(value)); }

// Sliders are done via value() and setValue(), rather than text() and setText()
#define CONFIG_ATTRIBUTE_SLIDER(control, name, default)                 \
    int PreferencesDialog::Get##name() const { return ui->control->value(); } \
    void PreferencesDialog::Set##name(int value) { ui->control->setValue(value); }

#include "prefs_options.h"

#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR
#undef CONFIG_ATTRIBUTE_INT
#undef CONFIG_ATTRIBUTE_SLIDER

#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
#undef CONFIG_ATTRIBUTE_INT_NO_PREFS

void PreferencesDialog::on_initialBPMLineEdit_textChanged(const QString &arg1)
{
    int pos = 0;
    bool acceptable = (ui->initialBPMLineEdit->validator()->validate((QString &)arg1,pos) == QValidator::Acceptable);

    QColor notOKcolor("#f08080");

    if (acceptable) {
        ui->initialBPMLineEdit->setStyleSheet("");
    } else {
        ui->initialBPMLineEdit->setStyleSheet(COLOR_STYLE.arg(notOKcolor.name()));
    }
}

void PreferencesDialog::on_lineEditMusicTypePatter_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    songTableReloadNeeded = true;  // change to folder names requires reload of the songTable
}

void PreferencesDialog::on_lineEditMusicTypeExtras_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    songTableReloadNeeded = true;  // change to folder names requires reload of the songTable
}

void PreferencesDialog::on_lineEditMusicTypeSinging_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    songTableReloadNeeded = true;  // change to folder names requires reload of the songTable
}

void PreferencesDialog::on_lineEditMusicTypeCalled_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    songTableReloadNeeded = true;  // change to folder names requires reload of the songTable
}


// ------------

void PreferencesDialog::on_pushButtonTagsBackgroundColor_clicked()
{
    QString originalColorString = ui->pushButtonTagsBackgroundColor->text();
    QColor chosenColor = QColorDialog::getColor(QColor(originalColorString), this); //return the color chosen by user
    if (chosenColor.isValid()) {
        originalColorString = chosenColor.name();

        setPushButtonColor(ui->pushButtonTagsBackgroundColor,originalColorString);
        SetLabelTagAppearanceColors();
        songTableReloadNeeded = true;  // change to colors requires reload of the songTable
    }
}

void PreferencesDialog::on_pushButtonTagsForegroundColor_clicked()
{
    QString originalColorString = ui->pushButtonTagsForegroundColor->text();
    QColor chosenColor = QColorDialog::getColor(QColor(originalColorString), this); //return the color chosen by user
    if (chosenColor.isValid()) {
        originalColorString = chosenColor.name();

        setPushButtonColor(ui->pushButtonTagsForegroundColor,originalColorString);
        SetLabelTagAppearanceColors();
        songTableReloadNeeded = true;  // change to colors requires reload of the songTable
    }
}

void PreferencesDialog::SetLabelTagAppearanceColors()
{
    QString format("<span style=\"background-color:%1; color:%2\">&nbsp;tags </span>&nbsp;<span style=\"background-color:%1; color:%2\"> look </span>&nbsp;<span style=\"background-color:%1; color:%2\"> like </span>&nbsp;<span style=\"background-color:%1; color:%2\"> this </span>");
    QString str(format.arg(ui->pushButtonTagsBackgroundColor->text()).arg(ui->pushButtonTagsForegroundColor->text()));
    ui->labelTagAppearance->setText(str);
}

void PreferencesDialog::on_tabWidget_currentChanged(int /* tab */)
{
    setPushButtonColor(ui->pushButtonTagsBackgroundColor,
                       ui->pushButtonTagsBackgroundColor->text());
    setPushButtonColor(ui->pushButtonTagsForegroundColor,
                       ui->pushButtonTagsForegroundColor->text());
}

void PreferencesDialog::finishPopulation()
{
    SetLabelTagAppearanceColors();

    // The two filter radio buttons are one preference; unchecking the "only tracks matching"
    //   one does not automatically check its partner, so do that here.
    ui->appleMusicNoFilterRadio->setChecked(!ui->appleMusicFilterEnabledRadio->isChecked());
    updateAppleMusicEnabledStates();
}

int PreferencesDialog::getActiveTab()
{
    // qDebug() << "getActiveTab will return: " << ui->tabWidget->currentIndex();
    return ui->tabWidget->currentIndex();
}

void PreferencesDialog::setActiveTab(int tabnum)
{
    // qDebug() << "setActiveTab: " << tabnum;
    ui->tabWidget->setCurrentIndex(tabnum);
}

void PreferencesDialog::on_afterLongTipAction_currentIndexChanged(int index)
{
    if (swallowSoundFX) {
        return;
    }

    if (index == 0) {
        mw->stopSFX();  // visual indicator only
    } else if (index == 1) {
        mw->playSFX("long_tip");
    } else {
//        qDebug() << "index: " << index;
        mw->playSFX(QString::number(index-1));
    }
}

void PreferencesDialog::on_afterBreakAction_currentIndexChanged(int index)
{
    if (swallowSoundFX) {
        return;
    }

    if (index == 0) {
        mw->stopSFX();  // visual indicator only
    } else if (index == 1) {
        mw->playSFX("break_over");
    } else {
//        qDebug() << "index: " << index;
        mw->playSFX(QString::number(index-1));
    }

}


//// Replay Gain ---------------------------------
//void PreferencesDialog::on_replayGainCheckbox_toggled(bool checked)
//{
//    if (checked) {
////        qDebug() << "replayGainCheckbox checked";
//        cBass->SetReplayGainVolume(mw->songLoadedReplayGain_dB); // restore to last loaded song
//    } else {
////        qDebug() << "replayGainCheckbox NOT checked";
//        cBass->SetReplayGainVolume(0.0); // set to 0.0dB (replayGain disabled)
//    }
//}

// Intelligibility Boost -----------------------------------------------------------------------
void PreferencesDialog::on_intelCenterFreqDial_valueChanged(int value)
{
// value is in Hz/10
    float centerFreq_KHz = static_cast<float>(value)/10.0f; // only integer tenths of a KHz
    ui->intelCenterFreq_KHz->setText(QStringLiteral("%1KHz").arg(centerFreq_KHz));
    cBass->SetIntelBoost(0, centerFreq_KHz);
}

void PreferencesDialog::on_intelWidthDial_valueChanged(int value)
{
// value is in octaves * 10
    float width_octaves = static_cast<float>(value)/10.0f;  // only integer tenths of an octave
    ui->intelWidth_oct->setText(QStringLiteral("%1").arg(width_octaves, 3, 'f', 1));
    cBass->SetIntelBoost(1, width_octaves);
}

void PreferencesDialog::on_intelGainDial_valueChanged(int value)
{
// value is in gain dB * 10
    float gain_dB = static_cast<float>(value)/10.0f;  // only integer tenths of a dB
    if (gain_dB == 0.0f) {
        ui->intelGain_dB->setText(QStringLiteral("%1dB").arg(gain_dB));
    } else {
        ui->intelGain_dB->setText(QStringLiteral("-%1dB").arg(gain_dB));
    }
    cBass->SetIntelBoost(2, gain_dB);
}

void PreferencesDialog::on_intelResetButton_clicked()
{
    ui->intelCenterFreqDial->setValue(16);   // 1.6KHz
    ui->intelWidthDial->setValue(20);        // 2.0 octaves
    ui->intelGainDial->setValue(30);         // -3.0dB

    on_intelCenterFreqDial_valueChanged(16); // force calls to cBass->..
    on_intelWidthDial_valueChanged(20);
    on_intelGainDial_valueChanged(30);
}

void PreferencesDialog::on_intelBoostEnabledCheckbox_toggled(bool checked)
{
    ui->intelCenterFreqDial->setEnabled(checked);
    ui->intelWidthDial->setEnabled(checked);
    ui->intelGainDial->setEnabled(checked);

    ui->intelCenterFreqLabel->setEnabled(checked);
    ui->intelWidthLabel->setEnabled(checked);
    ui->intelGainLabel->setEnabled(checked);

    ui->intelCenterFreq_KHz->setEnabled(checked);
    ui->intelWidth_oct->setEnabled(checked);
    ui->intelGain_dB->setEnabled(checked);

    ui->intelResetButton->setEnabled(checked);

    ui->intelBoostBypassed->setVisible(!checked);

    int delta = 60;
    int leftExtend = 20;
    int yLoc = 40;
    if (checked) {
        ui->intelBoostLine->setGeometry(330 - leftExtend - delta, yLoc, 390 + leftExtend + delta, 20);
    } else {
        ui->intelBoostLine->setGeometry(330 - leftExtend, yLoc, 390 + leftExtend, 20);
    }

    cBass->SetIntelBoostEnabled(checked);
}

void PreferencesDialog::on_panEQGainDial_valueChanged(int value)
{
    // value is in gain dB * 10
    float gain_dB = static_cast<float>(value)/2.0f;  // only in increments of 0.5dB
    ui->panEQGain_dB->setText(QStringLiteral("%1dB").arg(gain_dB));

    cBass->SetPanEQVolumeCompensation(gain_dB);
}


// ============================================================================================
// Preferences > Apple Music: the square dance filter, and the metadata field that determines
//   Type (issue #1740, item 6).
//
// Three different Apple Music users organize their libraries three different ways (genre,
//   album, album artist), so both "is this square dance music at all" and "is this patter or
//   a singing call" have to be configurable rather than hardwired.
//
// NOTE: this is the UI only.  Nothing here is read by the Resync path yet; the live match
//   count and the preview read the real iTunes library so the rules can be tried out.
// ============================================================================================

// The two running totals are the numbers the user is actually watching while they edit a
//   rule, so they get bold italic.  Rich text, not a font: see the comment about the
//   re-polish in setupAppleMusicTab().
static QString appleMusicBoldItalic(const QString &text)
{
    return "<b><i>" + text.toHtmlEscaped() + "</i></b>";
}

static void appleMusicTidySemicolons(QLineEdit *edit)
{
    const QString before = edit->text();
    const QString after = appleMusicCollapseSemicolons(before);
    if (after == before) {
        return;
    }

    // Collapsing the text to the left of the cursor the same way says where the cursor lands.
    const int newCursorPosition = appleMusicCollapseSemicolons(before.left(edit->cursorPosition())).length();

    edit->setText(after);   // re-enters this function via textChanged(), but then after == before
    edit->setCursorPosition(newCursorPosition);
}

// -------------------------------------------------------------------
void PreferencesDialog::setupAppleMusicTab()
{
    // itemData == itemIndex for all four of these, so currentIndex() can be used directly
    SetPulldownValuesToItemNumberPlusN(ui->appleMusicFilterMatchCombo, 0);
    SetPulldownValuesToItemNumberPlusN(ui->appleMusicTypeFieldCombo, 0);
    SetPulldownValuesToItemNumberPlusN(ui->appleMusicTypeDefaultCombo, 0);
    SetPulldownValuesToItemNumberPlusN(ui->appleMusicTypeColumnFormatCombo, 0);

    // Recounting walks the whole library, so coalesce the keystrokes of someone typing a value.
    appleMusicRecountTimer = new QTimer(this);
    appleMusicRecountTimer->setSingleShot(true);
    appleMusicRecountTimer->setInterval(250);
    connect(appleMusicRecountTimer, &QTimer::timeout, this, &PreferencesDialog::updateAppleMusicPreview);

    connect(ui->appleMusicNoFilterRadio,             &QRadioButton::toggled,  this, [this]{ appleMusicSettingsChanged(); });
    connect(ui->appleMusicFilterEnabledRadio,        &QRadioButton::toggled,  this, [this]{ appleMusicSettingsChanged(); });
    connect(ui->appleMusicFilterInPlaylistsCheckbox, &QCheckBox::toggled,     this, [this]{ appleMusicSettingsChanged(); });
    connect(ui->appleMusicFilterMatchCombo,          &QComboBox::currentIndexChanged, this, [this]{ appleMusicSettingsChanged(); });
    connect(ui->appleMusicTypeDefaultCombo,          &QComboBox::currentIndexChanged, this, [this]{ appleMusicSettingsChanged(); });
    connect(ui->appleMusicTypeColumnFormatCombo,     &QComboBox::currentIndexChanged, this, [this]{ appleMusicSettingsChanged(); });

    connect(ui->appleMusicTypeFieldCombo, &QComboBox::currentIndexChanged, this, [this]{
        // the four Type value pickers below now offer values from a different field
        appleMusicSettingsChanged();
    });

    struct { QLineEdit *edit; QToolButton *button; } typeRows[] = {
        { ui->lineEditAppleMusicTypePatter,  ui->appleMusicTypePatterPickerButton  },
        { ui->lineEditAppleMusicTypeSinging, ui->appleMusicTypeSingingPickerButton },
        { ui->lineEditAppleMusicTypeCalled,  ui->appleMusicTypeCalledPickerButton  },
        { ui->lineEditAppleMusicTypeExtras,  ui->appleMusicTypeExtrasPickerButton  },
    };
    for (auto &row : typeRows) {
        QLineEdit *edit = row.edit;
        QToolButton *button = row.button;
        connect(edit, &QLineEdit::textChanged, this, [this, edit]{
            appleMusicTidySemicolons(edit);   // ";;" is always a typo, or a value that was just deleted
            appleMusicSettingsChanged();
        });
        connect(button, &QToolButton::clicked, this, [this, edit, button]{ showAppleMusicValueMenu(edit, button); });
    }

    // Reading the iTunes library takes a moment, so don't do it until this tab is looked at.
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int tab){
        if (tab == ui->tabWidget->indexOf(ui->tab_appleMusic)) {
            loadAppleMusicLibraryIfNeeded();
            updateAppleMusicPreview();
        }
    });

    // The live preview at the bottom of the tab.  It is rebuilt from scratch on every change,
    //   so it must stay cheap: no word wrap, fixed short rows, and columns clamped below.
    QTableWidget *previewTable = ui->appleMusicPreviewTable;
    previewTable->setRowCount(0);
    previewTable->setColumnCount(0);
    previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    previewTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    previewTable->setSelectionMode(QAbstractItemView::SingleSelection);
    previewTable->setAlternatingRowColors(true);
    previewTable->setWordWrap(false);
    previewTable->setTextElideMode(Qt::ElideRight);
    previewTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    previewTable->setSortingEnabled(true);   // sorting by Type is the fastest way to audit a mapping
    previewTable->verticalHeader()->setVisible(false);
    previewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    // Columns are clamped below, so on a wide dialog they can add up to less than the table.
    //   Let the last one take up whatever is left over rather than leaving a dead strip; when
    //   the columns are wider than the table this does nothing and the scrollbar appears.
    previewTable->horizontalHeader()->setStretchLastSection(true);
    previewTable->setMinimumHeight(110);

    // The settings scroll on their own above the preview, so the preview is always in view,
    //   and the user can give either half more room.
    ui->appleMusicSplitter->setChildrenCollapsible(false);
    ui->appleMusicSplitter->setStretchFactor(0, 3);
    ui->appleMusicSplitter->setStretchFactor(1, 2);
    ui->appleMusicSplitter->setSizes({ 330, 190 });

    // The two section headings are QLabels holding "<b>...</b>", each with a horizontal rule
    //   under it, rather than QGroupBox titles.
    //
    // A QGroupBox title is drawn by the style, so its font gets re-resolved out from under us:
    //   SquareDesk keeps an application-wide stylesheet loaded (themesFileModified() in
    //   mainwindow_themes.cpp), and MainWindow calls setDynamicPropertyRecursive(prefDialog,
    //   "theme", ...) AFTER this dialog is built, which unpolishes and re-polishes every widget
    //   in it.  Neither setFont() nor a "QGroupBox::title { font: bold ... }" rule survived
    //   that.  Bold that lives in the label's rich text is content rather than font, so nothing
    //   the style does later can take it away.
    //
    // Only the size comes from a stylesheet, which does survive the re-polish.
    QLabel *headings[] = { ui->appleMusicFilterHeaderLabel, ui->appleMusicTypeHeaderLabel };
    for (QLabel *heading : headings) {
        const QFont baseFont = heading->font();
        if (baseFont.pointSizeF() > 0) {
            heading->setStyleSheet(QString("font-size: %1pt;").arg(baseFont.pointSizeF() + 3.0));
        } else if (baseFont.pixelSize() > 0) {
            // macOS system fonts are specified in pixels, so pointSizeF() is -1 there
            heading->setStyleSheet(QString("font-size: %1px;").arg(baseFont.pixelSize() + 4));
        }
    }

    // A little air above each heading, so the sections read as separate blocks.  Back to front,
    //   so inserting doesn't shift the index of the item that hasn't been done yet.
    QVBoxLayout *contents = ui->appleMusicContentsLayout;
    contents->insertSpacing(contents->indexOf(ui->appleMusicTypeSection), 12);
    contents->insertSpacing(contents->indexOf(ui->appleMusicFilterSection), 12);

    appleMusicSetupDone = true;
    updateAppleMusicEnabledStates();
}

int PreferencesDialog::appleMusicRuleRowCount() const
{
    return ui->appleMusicRulesLayout->count();
}

QWidget *PreferencesDialog::appleMusicRuleRowAt(int i) const
{
    QLayoutItem *item = ui->appleMusicRulesLayout->itemAt(i);
    return item ? item->widget() : nullptr;
}

void PreferencesDialog::addAppleMusicRuleRow(const QString &fieldKey, const QString &opKey, const QString &value)
{
    QWidget *row = new QWidget(ui->appleMusicRulesContainer);
    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(4);

    QComboBox *fieldCombo = new QComboBox(row);
    fieldCombo->setObjectName("ruleField");
    for (int i = 0; i < numAppleMusicFields; ++i) {
        fieldCombo->addItem(appleMusicFields[i].display, QString(appleMusicFields[i].key));
    }
    int fieldIndex = fieldCombo->findData(fieldKey);
    fieldCombo->setCurrentIndex(fieldIndex >= 0 ? fieldIndex : 0);

    QComboBox *opCombo = new QComboBox(row);
    opCombo->setObjectName("ruleOp");
    for (int i = 0; i < numAppleMusicOperators; ++i) {
        opCombo->addItem(appleMusicOperators[i].display, QString(appleMusicOperators[i].key));
    }
    int opIndex = opCombo->findData(opKey);
    opCombo->setCurrentIndex(opIndex >= 0 ? opIndex : 0);

    // Editable, so the user can type a value that isn't in the library yet, but with a
    //   pulldown of the values that ARE in the library (with counts), so nobody has to
    //   remember exactly how they spelled "square dancing" in iTunes.
    QComboBox *valueCombo = new QComboBox(row);
    valueCombo->setObjectName("ruleValue");
    valueCombo->setEditable(true);
    valueCombo->setInsertPolicy(QComboBox::NoInsert);
    valueCombo->setMinimumWidth(220);
    valueCombo->setEditText(value);

    QToolButton *removeButton = new QToolButton(row);
    removeButton->setText("-");
    removeButton->setToolTip("Remove this rule");

    QToolButton *addButton = new QToolButton(row);
    addButton->setText("+");
    addButton->setToolTip("Add another rule");

    rowLayout->addWidget(fieldCombo);
    rowLayout->addWidget(opCombo);
    rowLayout->addWidget(valueCombo, 1);
    rowLayout->addWidget(removeButton);
    rowLayout->addWidget(addButton);

    connect(fieldCombo, &QComboBox::currentIndexChanged, this, [this, fieldCombo, valueCombo]{
        refreshAppleMusicValuePicker(valueCombo, fieldCombo->currentData().toString());
        appleMusicSettingsChanged();
    });
    connect(opCombo, &QComboBox::currentIndexChanged, this, [this, opCombo, valueCombo]{
        valueCombo->setEnabled(opCombo->currentData().toString() != "notEmpty");
        appleMusicSettingsChanged();
    });
    connect(valueCombo, &QComboBox::editTextChanged, this, [this]{ appleMusicSettingsChanged(); });
    connect(valueCombo, &QComboBox::activated, this, [valueCombo](int index){
        // items are displayed as "square dancing  (412)", but the rule wants just the value
        valueCombo->setEditText(valueCombo->itemData(index).toString());
    });
    connect(removeButton, &QToolButton::clicked, this, [this, row]{
        if (appleMusicRuleRowCount() <= 1) {
            return;  // always leave one row, so there is something to type into
        }
        ui->appleMusicRulesLayout->removeWidget(row);
        row->deleteLater();
        appleMusicSettingsChanged();
    });
    connect(addButton, &QToolButton::clicked, this, [this, row]{
        int index = ui->appleMusicRulesLayout->indexOf(row);
        addAppleMusicRuleRow("genre", "is", "");
        // addAppleMusicRuleRow() appends; move the new row to just below the one that was clicked
        QLayoutItem *newItem = ui->appleMusicRulesLayout->takeAt(ui->appleMusicRulesLayout->count() - 1);
        ui->appleMusicRulesLayout->insertItem(index + 1, newItem);
        appleMusicSettingsChanged();
    });

    valueCombo->setEnabled(opCombo->currentData().toString() != "notEmpty");
    ui->appleMusicRulesLayout->addWidget(row);
    refreshAppleMusicValuePicker(valueCombo, fieldCombo->currentData().toString());
}

QList<AppleMusicRule> PreferencesDialog::appleMusicRulesFromWidgets() const
{
    QList<AppleMusicRule> rules;
    for (int i = 0; i < appleMusicRuleRowCount(); ++i) {
        QWidget *row = appleMusicRuleRowAt(i);
        if (!row) {
            continue;
        }
        QComboBox *fieldCombo = row->findChild<QComboBox*>("ruleField");
        QComboBox *opCombo    = row->findChild<QComboBox*>("ruleOp");
        QComboBox *valueCombo = row->findChild<QComboBox*>("ruleValue");
        if (!fieldCombo || !opCombo || !valueCombo) {
            continue;
        }
        rules.append({ fieldCombo->currentData().toString(),
                       opCombo->currentData().toString(),
                       valueCombo->currentText() });
    }
    return rules;
}

// The live preview has to show what the import will actually do, so it runs the same
//   AppleMusicFilter the importer runs -- just filled in from the widgets rather than from the
//   saved preferences, since the user hasn't pressed OK yet.
AppleMusicFilter PreferencesDialog::appleMusicFilterFromWidgets() const
{
    AppleMusicFilter filter;

    filter.filterEnabled          = ui->appleMusicFilterEnabledRadio->isChecked();
    filter.matchAll               = (ui->appleMusicFilterMatchCombo->currentIndex() == 0);
    filter.appliesInsidePlaylists = ui->appleMusicFilterInPlaylistsCheckbox->isChecked();
    filter.rules                  = appleMusicRulesFromWidgets();

    const int typeFieldIndex = ui->appleMusicTypeFieldCombo->currentIndex();
    filter.typeFieldKey = (typeFieldIndex > 0) ? appleMusicFields[typeFieldIndex - 1].key : QString();

    filter.typePatter       = ui->lineEditAppleMusicTypePatter->text();
    filter.typeSinging      = ui->lineEditAppleMusicTypeSinging->text();
    filter.typeCalled       = ui->lineEditAppleMusicTypeCalled->text();
    filter.typeExtras       = ui->lineEditAppleMusicTypeExtras->text();
    filter.typeDefault      = ui->appleMusicTypeDefaultCombo->currentIndex();
    filter.typeColumnFormat = ui->appleMusicTypeColumnFormatCombo->currentIndex();

    return filter;
}

QString PreferencesDialog::getAppleMusicFilterRules()
{
    return AppleMusicFilter::rulesToJSON(appleMusicRulesFromWidgets());
}

void PreferencesDialog::setAppleMusicFilterRules(const QString &rulesJSON)
{
    while (QLayoutItem *item = ui->appleMusicRulesLayout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    const QList<AppleMusicRule> rules = AppleMusicFilter::rulesFromJSON(rulesJSON);
    for (const AppleMusicRule &rule : rules) {
        addAppleMusicRuleRow(rule.fieldKey, rule.opKey, rule.value);
    }

    if (appleMusicRuleRowCount() == 0) {
        addAppleMusicRuleRow("genre", "is", "");  // Eric's setup, as a starting point
    }
}

// -------------------------------------------------------------------
void PreferencesDialog::loadAppleMusicLibraryIfNeeded()
{
    if (appleMusicLibraryLoaded) {
        return;
    }
    appleMusicLibraryLoaded = true;  // one attempt per visit to Preferences, success or not

    // Read the PLAYLISTS, not the whole library.  SquareDesk only ever imports tracks that are
    //   in an Apple Music playlist, so a song that is merely in the library can't arrive however
    //   the rules are written, and counting it would overstate the match by a wide margin.  This
    //   is the same call getAppleMusicInfo() makes, so the preview sees exactly what the import
    //   will see (issue #1740).
    std::vector<PlaylistTrack> playlistTracks;

#ifdef NEWAPPLEMUSICINTEGRATION
    QApplication::setOverrideCursor(Qt::WaitCursor);
    std::string error;
    playlistTracks = readAllPlaylists(error);
    QApplication::restoreOverrideCursor();
    appleMusicLibraryError = QString::fromStdString(error);
#else
    appleMusicLibraryError = "Apple Music integration is not available in this build.";
#endif

    // Then drop everything else the importer would drop, so the preview and the counts only ever
    //   promise songs that can actually arrive: formats SquareDesk can't play (.m4p is FairPlay
    //   DRM, and Apple Music libraries are full of them), and files that aren't on this Mac --
    //   an iCloud track that was never downloaded, or one moved or deleted out from under Music.
    //   These mirror the two checks at the top of getAppleMusicInfo().
    QSet<QString> alreadySeen;
    appleMusicTracks.clear();
    appleMusicTracks.reserve(playlistTracks.size());

    for (const PlaylistTrack &track : playlistTracks) {
        const QString absolutePath = QString::fromStdString(track.absolutePath);

        // One song can be in several playlists.  The import makes a row per playlist membership,
        //   but "is this square dance music, and what Type is it" is a question about the SONG,
        //   so list it once.
        if (alreadySeen.contains(absolutePath)) {
            continue;
        }
        alreadySeen.insert(absolutePath);

        if (!appleMusicIsPlayableFormat(absolutePath) || !QFileInfo::exists(absolutePath)) {
            continue;
        }
        appleMusicTracks.push_back(appleMusicMetaOf(track));
    }

    // now that we know what's in the library, the pulldowns can offer real values
    for (int i = 0; i < appleMusicRuleRowCount(); ++i) {
        QWidget *row = appleMusicRuleRowAt(i);
        if (!row) {
            continue;
        }
        QComboBox *fieldCombo = row->findChild<QComboBox*>("ruleField");
        QComboBox *valueCombo = row->findChild<QComboBox*>("ruleValue");
        if (fieldCombo && valueCombo) {
            refreshAppleMusicValuePicker(valueCombo, fieldCombo->currentData().toString());
        }
    }
}

// The distinct values of one field, most common first, as "value" -> count.
static QList<QPair<QString,int>> appleMusicDistinctValues(const std::vector<AppleMusicTrackMeta> &tracks,
                                                          const QString &fieldKey)
{
    QHash<QString,int> counts;
    for (const AppleMusicTrackMeta &track : tracks) {
        QString value = appleMusicFieldValue(track, fieldKey).trimmed();
        if (!value.isEmpty()) {
            counts[value]++;
        }
    }

    QList<QPair<QString,int>> result;
    result.reserve(counts.size());
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        result.append(qMakePair(it.key(), it.value()));
    }
    std::sort(result.begin(), result.end(), [](const QPair<QString,int> &a, const QPair<QString,int> &b){
        if (a.second != b.second) {
            return a.second > b.second;
        }
        return a.first.compare(b.first, Qt::CaseInsensitive) < 0;
    });

    const int maxValues = 300;  // Title and Album have thousands; a pulldown that long is useless
    if (result.size() > maxValues) {
        result = result.mid(0, maxValues);
    }
    return result;
}

void PreferencesDialog::refreshAppleMusicValuePicker(QComboBox *valueCombo, const QString &fieldKey)
{
    QString currentText = valueCombo->currentText();

    valueCombo->blockSignals(true);
    valueCombo->clear();
    const QList<QPair<QString,int>> values = appleMusicDistinctValues(appleMusicTracks, fieldKey);
    for (const auto &value : values) {
        valueCombo->addItem(QString("%1  (%2)").arg(value.first).arg(value.second), value.first);
    }
    valueCombo->setEditText(currentText);
    valueCombo->blockSignals(false);
}

void PreferencesDialog::showAppleMusicValueMenu(QLineEdit *target, QToolButton *button)
{
    loadAppleMusicLibraryIfNeeded();

    int fieldIndex = ui->appleMusicTypeFieldCombo->currentIndex();
    if (fieldIndex <= 0) {
        return;  // no field chosen, so there are no values to offer
    }
    const QString fieldKey = appleMusicFields[fieldIndex - 1].key;

    // Only offer values from tracks that survive the filter -- the others aren't square
    //   dance music, so their Groupings would just be noise in this list.
    const AppleMusicFilter filter = appleMusicFilterFromWidgets();
    std::vector<AppleMusicTrackMeta> filtered;
    for (const AppleMusicTrackMeta &track : appleMusicTracks) {
        if (filter.passes(track)) {
            filtered.push_back(track);
        }
    }

    QMenu menu(this);
    const QList<QPair<QString,int>> values = appleMusicDistinctValues(filtered, fieldKey);
    if (values.isEmpty()) {
        menu.addAction("(no values found)")->setEnabled(false);
    }
    for (const auto &value : values) {
        QString valueText = value.first;
        QAction *action = menu.addAction(QString("%1  (%2)").arg(valueText).arg(value.second));
        connect(action, &QAction::triggered, this, [target, valueText]{
            QString existing = target->text().trimmed();
            if (appleMusicValueInList(valueText, existing)) {
                return;  // already in this list
            }
            target->setText(existing.isEmpty() ? valueText : existing + ";" + valueText);
        });
    }
    menu.exec(button->mapToGlobal(QPoint(0, button->height())));
}

// -------------------------------------------------------------------
// -------------------------------------------------------------------
void PreferencesDialog::appleMusicSettingsChanged()
{
    if (!appleMusicSetupDone) {
        return;
    }
    songTableReloadNeeded = true;
    updateAppleMusicEnabledStates();
    appleMusicRecountTimer->start();  // restarts the timer, so typing coalesces into one recount
}

void PreferencesDialog::updateAppleMusicEnabledStates()
{
    const bool syncOn = ui->enableAppleMusicCheckbox->isChecked();
    ui->appleMusicFilterSection->setEnabled(syncOn);
    ui->appleMusicTypeSection->setEnabled(syncOn);
    ui->appleMusicTypeColumnLabel->setEnabled(syncOn);
    ui->appleMusicTypeColumnFormatCombo->setEnabled(syncOn);

    const bool filterOn = ui->appleMusicFilterEnabledRadio->isChecked();
    ui->appleMusicFilterMatchCombo->setEnabled(filterOn);
    ui->appleMusicFilterMatchLabel->setEnabled(filterOn);
    ui->appleMusicRulesContainer->setEnabled(filterOn);
    ui->appleMusicFilterInPlaylistsCheckbox->setEnabled(filterOn);

    const bool typeFromMetadata = (ui->appleMusicTypeFieldCombo->currentIndex() > 0);
    QWidget *typeWidgets[] = {
        ui->appleMusicTypePatterLabel,  ui->lineEditAppleMusicTypePatter,  ui->appleMusicTypePatterPickerButton,
        ui->appleMusicTypeSingingLabel, ui->lineEditAppleMusicTypeSinging, ui->appleMusicTypeSingingPickerButton,
        ui->appleMusicTypeCalledLabel,  ui->lineEditAppleMusicTypeCalled,  ui->appleMusicTypeCalledPickerButton,
        ui->appleMusicTypeExtrasLabel,  ui->lineEditAppleMusicTypeExtras,  ui->appleMusicTypeExtrasPickerButton,
        ui->appleMusicCopyTypesButton,
        ui->appleMusicTypeDefaultLabel, ui->appleMusicTypeDefaultCombo,
    };
    for (QWidget *widget : typeWidgets) {
        widget->setEnabled(typeFromMetadata);
    }
}

// Rebuilding the preview walks the whole library and makes a QTableWidgetItem per cell, so
//   only the first few hundred matches are listed.  That's plenty to eyeball a rule, and it
//   keeps the rebuild fast enough to run on every keystroke (behind the 250ms coalescing timer).
static const int kApplePreviewMaxRows = 400;

// Title, Album and Comments can be enormous.  Sizing to contents and then clamping keeps the
//   short columns narrow without letting one long title push everything else off the right.
static const int kApplePreviewMaxColumnWidth = 220;

void PreferencesDialog::updateAppleMusicPreview()
{
    QTableWidget *table = ui->appleMusicPreviewTable;

    if (!appleMusicLibraryLoaded) {
        ui->appleMusicMatchCountLabel->setText("");
        ui->appleMusicPreviewSummaryLabel->setText("");
        table->clearContents();
        table->setRowCount(0);
        return;
    }

    if (!appleMusicLibraryError.isEmpty()) {
        ui->appleMusicMatchCountLabel->setText(appleMusicLibraryError.split("\n").first());
        ui->appleMusicMatchCountLabel->setToolTip(appleMusicLibraryError);
        ui->appleMusicPreviewSummaryLabel->setText("");
        table->clearContents();
        table->setRowCount(0);
        return;
    }

    // The same AppleMusicFilter the importer will run, filled in from the widgets as they
    //   stand right now -- built once here rather than per track.
    const AppleMusicFilter filter = appleMusicFilterFromWidgets();

    // When a Type field has been chosen, show its raw value next to the Type it produced, so
    //   it's obvious WHY a track came out patter, and which values are still unclassified.
    const QString typeFieldKey = filter.typeFieldKey;
    const bool showTypeField = filter.typeComesFromMetadata();

    // Column 0 is the Type, column 1 is the field that produced it (when there is one), and
    //   then the fields worth seeing anyway -- minus whichever one is already column 1, so the
    //   same values never appear in two columns.
    static const char *previewColumnKeys[] = { "title", "artist", "album", "genre", "grouping" };

    QStringList fieldKeys;    // one per column AFTER the Type column
    if (showTypeField) {
        fieldKeys << typeFieldKey;
    }
    for (const char *key : previewColumnKeys) {
        if (typeFieldKey != key) {
            fieldKeys << key;
        }
    }

    QStringList headers;
    headers << "Type";
    for (const QString &fieldKey : std::as_const(fieldKeys)) {
        headers << appleMusicFieldDisplay(fieldKey);
    }

    // A rebuild shouldn't throw away a column the user widened, or scroll them back to the top.
    const bool columnsChanged = (headers != appleMusicPreviewHeaders);
    const int savedScroll = table->verticalScrollBar()->value();

    table->setUpdatesEnabled(false);
    table->setSortingEnabled(false);   // rows can't be inserted into a sorted table
    table->clearContents();
    table->setRowCount(0);
    if (columnsChanged) {
        table->setColumnCount(headers.count());
        table->setHorizontalHeaderLabels(headers);
        appleMusicPreviewHeaders = headers;
    }

    int matched = 0;
    int shown = 0;
    QHash<QString,int> byType;

    for (const AppleMusicTrackMeta &track : appleMusicTracks) {
        if (!filter.passes(track)) {
            continue;
        }
        matched++;

        // ...and matching the importer, the Type mapping only speaks up when a field is chosen
        const QString type = showTypeField ? filter.typeOf(track) : QString();
        if (showTypeField) {
            byType[type]++;
        }

        if (shown >= kApplePreviewMaxRows) {
            continue;
        }

        QStringList cells;
        cells << (!showTypeField     ? QString("\u2014")            // no Type field chosen: unchanged
                  : type.isEmpty()   ? QString("(not imported)")
                                     : type);
        for (const QString &fieldKey : std::as_const(fieldKeys)) {
            const QString value = appleMusicFieldValue(track, fieldKey);
            // only the Type field earns "(empty)"; an empty Album is just an empty cell
            const bool isTypeField = (showTypeField && fieldKey == typeFieldKey);
            cells << ((isTypeField && value.trimmed().isEmpty()) ? QString("(empty)") : value);
        }

        table->insertRow(shown);
        for (int col = 0; col < cells.count(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(cells[col]);
            item->setToolTip(cells[col]);   // the cell is clamped, the tooltip isn't
            if (col == 0 && type.isEmpty()) {
                item->setForeground(QBrush(QColor("#A0A0A0")));  // this one won't be imported
            }
            table->setItem(shown, col, item);
        }
        shown++;
    }

    if (columnsChanged) {
        table->resizeColumnsToContents();
        for (int col = 0; col < table->columnCount(); ++col) {
            table->setColumnWidth(col, qMin(table->columnWidth(col), kApplePreviewMaxColumnWidth));
        }
    }
    table->setSortingEnabled(true);    // re-sorts by whatever column the user last clicked
    table->setUpdatesEnabled(true);
    table->verticalScrollBar()->setValue(savedScroll);

    QLocale locale;
    ui->appleMusicMatchCountLabel->setText(appleMusicBoldItalic(
                                              QString("%1 of the %2 songs in your Apple Music playlists match.")
                                               .arg(locale.toString(matched))
                                               .arg(locale.toString((int)appleMusicTracks.size()))));
    ui->appleMusicMatchCountLabel->setToolTip("");

    QStringList typeParts;
    const char *types[] = { "patter", "singing", "called", "extras" };
    for (const char *type : types) {
        if (byType.value(type) > 0) {
            typeParts << QString("%1 %2").arg(locale.toString(byType.value(type))).arg(type);
        }
    }
    if (byType.value("") > 0) {
        typeParts << QString("%1 not imported").arg(locale.toString(byType.value("")));
    }

    QString summary;
    if (matched == 0) {
        summary = appleMusicBoldItalic("No tracks match.");
    } else {
        // the tally is bold italic; the asides after it are not
        summary = appleMusicBoldItalic(typeParts.isEmpty()
                                           ? QString("%1 matching tracks.").arg(locale.toString(matched))
                                           : typeParts.join(", ") + ".");
        // runs of spaces collapse in rich text, so space the asides out with non-breaking ones
        if (!showTypeField) {
            summary += "&nbsp;&nbsp; No Type field chosen, so these keep the Type column and color they have now.";
        }
        if (matched > shown) {
            summary += QString("&nbsp;&nbsp; Showing the first %1.").arg(locale.toString(shown));
        }
    }
    ui->appleMusicPreviewSummaryLabel->setText(summary);
}

// -------------------------------------------------------------------
void PreferencesDialog::on_enableAppleMusicCheckbox_toggled(bool /* checked */)
{
    if (!appleMusicSetupDone) {
        return;
    }
    loadAppleMusicLibraryIfNeeded();
    appleMusicSettingsChanged();
}

void PreferencesDialog::on_appleMusicCopyTypesButton_clicked()
{
    // Most people's iTunes vocabulary won't match their folder names, but when it does this
    //   saves retyping.  These are deliberately separate preferences: the Music Types tab's
    //   lists double as folder names in the Music Directory (see darkLoadMusicList()).
    ui->lineEditAppleMusicTypePatter->setText(ui->lineEditMusicTypePatter->text());
    ui->lineEditAppleMusicTypeSinging->setText(ui->lineEditMusicTypeSinging->text());
    ui->lineEditAppleMusicTypeCalled->setText(ui->lineEditMusicTypeCalled->text());
    ui->lineEditAppleMusicTypeExtras->setText(ui->lineEditMusicTypeExtras->text());
}
