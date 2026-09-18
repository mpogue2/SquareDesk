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
#include <algorithm>

// Preferences > Apple Music (issue #1740, item 6)
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QRadioButton>
#include <QRegularExpression>
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

struct AppleMusicNamedItem {
    const char *key;
    const char *display;
};

// Order must match the items in appleMusicTypeFieldCombo (after its leading "Nothing" item)
//   and in the rule rows' field pulldown, both of which are built from this table.
static const AppleMusicNamedItem appleMusicFields[] = {
    { "genre",       "Genre"        },
    { "grouping",    "Grouping"     },
    { "album",       "Album"        },
    { "albumArtist", "Album Artist" },
    { "artist",      "Artist"       },
    { "composer",    "Composer"     },
    { "comments",    "Comments"     },
    { "work",        "Work"         },
    { "title",       "Title"        },
};
static const int numAppleMusicFields = sizeof(appleMusicFields)/sizeof(appleMusicFields[0]);

static const AppleMusicNamedItem appleMusicOperators[] = {
    { "is",          "is"                },
    { "isNot",       "is not"            },
    { "contains",    "contains"          },
    { "notContains", "does not contain"  },
    { "startsWith",  "starts with"       },
    { "endsWith",    "ends with"         },
    { "matches",     "matches (wildcard)"},
    { "notEmpty",    "is not empty"      },
};
static const int numAppleMusicOperators = sizeof(appleMusicOperators)/sizeof(appleMusicOperators[0]);

static QString appleMusicFieldValue(const AppleMusicTrackMeta &track, const QString &fieldKey)
{
    if (fieldKey == "genre")       return QString::fromStdString(track.genre);
    if (fieldKey == "grouping")    return QString::fromStdString(track.grouping);
    if (fieldKey == "album")       return QString::fromStdString(track.album);
    if (fieldKey == "albumArtist") return QString::fromStdString(track.albumArtist);
    if (fieldKey == "artist")      return QString::fromStdString(track.artist);
    if (fieldKey == "composer")    return QString::fromStdString(track.composer);
    if (fieldKey == "comments")    return QString::fromStdString(track.comments);
    if (fieldKey == "work")        return QString::fromStdString(track.work);
    if (fieldKey == "title")       return QString::fromStdString(track.title);
    return QString();
}

static bool appleMusicRuleMatches(const QString &value, const QString &opKey, const QString &arg)
{
    if (opKey == "notEmpty")    return !value.trimmed().isEmpty();
    if (opKey == "is")          return value.compare(arg, Qt::CaseInsensitive) == 0;
    if (opKey == "isNot")       return value.compare(arg, Qt::CaseInsensitive) != 0;
    if (opKey == "contains")    return value.contains(arg, Qt::CaseInsensitive);
    if (opKey == "notContains") return !value.contains(arg, Qt::CaseInsensitive);
    if (opKey == "startsWith")  return value.startsWith(arg, Qt::CaseInsensitive);
    if (opKey == "endsWith")    return value.endsWith(arg, Qt::CaseInsensitive);
    if (opKey == "matches") {
        QRegularExpression re = QRegularExpression::fromWildcard(arg, Qt::CaseInsensitive);
        return re.match(value).hasMatch();
    }
    return false;
}

// One entry of a semicolon-separated Type list, e.g. "hoedown;patter;SD *".  Wildcards are
//   allowed here too, so a user whose Groupings are "SD-Patter-1".."SD-Patter-9" can write "SD-Patter-*".
static bool appleMusicValueInList(const QString &value, const QString &semicolonList)
{
    const QStringList entries = semicolonList.split(';', Qt::SkipEmptyParts);
    for (const QString &entryRaw : entries) {
        QString entry = entryRaw.trimmed();
        if (entry.isEmpty()) {
            continue;
        }
        if (entry.contains('*') || entry.contains('?')) {
            if (QRegularExpression::fromWildcard(entry, Qt::CaseInsensitive).match(value).hasMatch()) {
                return true;
            }
        } else if (value.compare(entry, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

// "hoedown;;patter" (or a leading ";") is what's left behind when a value is deleted from the
//   middle of one of the Type lists.  The list parser skips empty entries anyway, but leaving
//   them in the field looks broken, so collapse them as they appear.
static QString appleMusicCollapseSemicolons(QString text)
{
    static const QRegularExpression runOfSemicolons(";{2,}");
    text.replace(runOfSemicolons, ";");
    while (text.startsWith(';')) {
        text.remove(0, 1);
    }
    return text;
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
    connect(appleMusicRecountTimer, &QTimer::timeout, this, &PreferencesDialog::updateAppleMusicMatchCount);

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
            updateAppleMusicMatchCount();
        }
    });

    // The two group box titles are the section headers of this tab, so make them look like it.
    //   A QGroupBox propagates its font down to its children, and only the title should get
    //   the larger bold font, so give every existing child the base font explicitly.  Widgets
    //   created later (the rule rows) are parented to appleMusicRulesContainer, which is one
    //   of those children, so they inherit the base font too.
    QGroupBox *sections[] = { ui->appleMusicFilterGroupBox, ui->appleMusicTypeGroupBox };
    for (QGroupBox *section : sections) {
        const QFont baseFont = section->font();
        QFont titleFont = baseFont;
        titleFont.setBold(true);
        if (baseFont.pointSizeF() > 0) {
            titleFont.setPointSizeF(baseFont.pointSizeF() + 2.0);
        }
        section->setFont(titleFont);

        const QList<QWidget*> children = section->findChildren<QWidget*>();
        for (QWidget *child : children) {
            child->setFont(baseFont);
        }
    }

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

QString PreferencesDialog::getAppleMusicFilterRules()
{
    QJsonArray rules;
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
        QJsonObject rule;
        rule["field"] = fieldCombo->currentData().toString();
        rule["op"]    = opCombo->currentData().toString();
        rule["value"] = valueCombo->currentText();
        rules.append(rule);
    }
    return QString::fromUtf8(QJsonDocument(rules).toJson(QJsonDocument::Compact));
}

void PreferencesDialog::setAppleMusicFilterRules(const QString &rulesJSON)
{
    while (QLayoutItem *item = ui->appleMusicRulesLayout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    const QJsonArray rules = QJsonDocument::fromJson(rulesJSON.toUtf8()).array();
    for (const QJsonValue &ruleValue : rules) {
        const QJsonObject rule = ruleValue.toObject();
        addAppleMusicRuleRow(rule["field"].toString("genre"),
                             rule["op"].toString("is"),
                             rule["value"].toString());
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

#ifdef NEWAPPLEMUSICINTEGRATION
    QApplication::setOverrideCursor(Qt::WaitCursor);
    std::string error;
    appleMusicTracks = readAllLibraryTrackMeta(error);
    QApplication::restoreOverrideCursor();
    appleMusicLibraryError = QString::fromStdString(error);
#else
    appleMusicLibraryError = "Apple Music integration is not available in this build.";
#endif

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
    std::vector<AppleMusicTrackMeta> filtered;
    for (const AppleMusicTrackMeta &track : appleMusicTracks) {
        if (appleMusicTrackPasses(track)) {
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
bool PreferencesDialog::appleMusicTrackPasses(const AppleMusicTrackMeta &track) const
{
    if (!ui->appleMusicFilterEnabledRadio->isChecked()) {
        return true;
    }

    const bool matchAll = (ui->appleMusicFilterMatchCombo->currentIndex() == 0);
    int ruleCount = 0;

    for (int i = 0; i < ui->appleMusicRulesLayout->count(); ++i) {
        QLayoutItem *item = ui->appleMusicRulesLayout->itemAt(i);
        QWidget *row = item ? item->widget() : nullptr;
        if (!row) {
            continue;
        }
        QComboBox *fieldCombo = row->findChild<QComboBox*>("ruleField");
        QComboBox *opCombo    = row->findChild<QComboBox*>("ruleOp");
        QComboBox *valueCombo = row->findChild<QComboBox*>("ruleValue");
        if (!fieldCombo || !opCombo || !valueCombo) {
            continue;
        }

        const QString opKey = opCombo->currentData().toString();
        const QString arg   = valueCombo->currentText().trimmed();
        if (arg.isEmpty() && opKey != "notEmpty") {
            continue;  // a half-typed rule shouldn't suddenly match (or reject) everything
        }
        ruleCount++;

        const bool matched = appleMusicRuleMatches(appleMusicFieldValue(track, fieldCombo->currentData().toString()),
                                                   opKey, arg);
        if (matchAll && !matched) {
            return false;
        }
        if (!matchAll && matched) {
            return true;
        }
    }

    if (ruleCount == 0) {
        return true;  // nothing typed in yet: don't pretend the library is empty
    }
    return matchAll;
}

QString PreferencesDialog::appleMusicTypeOf(const AppleMusicTrackMeta &track) const
{
    int fieldIndex = ui->appleMusicTypeFieldCombo->currentIndex();
    if (fieldIndex > 0) {
        const QString value = appleMusicFieldValue(track, appleMusicFields[fieldIndex - 1].key);
        if (appleMusicValueInList(value, ui->lineEditAppleMusicTypePatter->text()))  return "patter";
        if (appleMusicValueInList(value, ui->lineEditAppleMusicTypeSinging->text())) return "singing";
        if (appleMusicValueInList(value, ui->lineEditAppleMusicTypeCalled->text()))  return "called";
        if (appleMusicValueInList(value, ui->lineEditAppleMusicTypeExtras->text()))  return "extras";
    }

    switch (ui->appleMusicTypeDefaultCombo->currentIndex()) {
        case 0:  return "patter";
        case 1:  return "singing";
        case 2:  return "called";
        case 3:  return "extras";
        default: return "";        // "Don't import"
    }
}

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
    ui->appleMusicFilterGroupBox->setEnabled(syncOn);
    ui->appleMusicTypeGroupBox->setEnabled(syncOn);
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
    };
    for (QWidget *widget : typeWidgets) {
        widget->setEnabled(typeFromMetadata);
    }
}

void PreferencesDialog::updateAppleMusicMatchCount()
{
    if (!appleMusicLibraryLoaded) {
        ui->appleMusicMatchCountLabel->setText("");
        return;
    }

    if (!appleMusicLibraryError.isEmpty()) {
        ui->appleMusicMatchCountLabel->setText(appleMusicLibraryError.split("\n").first());
        ui->appleMusicMatchCountLabel->setToolTip(appleMusicLibraryError);
        ui->appleMusicPreviewButton->setEnabled(false);
        return;
    }

    int matched = 0;
    QHash<QString,int> byType;
    for (const AppleMusicTrackMeta &track : appleMusicTracks) {
        if (!appleMusicTrackPasses(track)) {
            continue;
        }
        matched++;
        byType[appleMusicTypeOf(track)]++;
    }

    QLocale locale;
    QString text = QString("%1 of %2 songs match.")
                       .arg(locale.toString(matched))
                       .arg(locale.toString((int)appleMusicTracks.size()));

    if (ui->appleMusicTypeFieldCombo->currentIndex() > 0) {
        QStringList parts;
        const char *types[] = { "patter", "singing", "called", "extras" };
        for (const char *type : types) {
            if (byType.value(type) > 0) {
                parts << QString("%1 %2").arg(locale.toString(byType.value(type))).arg(type);
            }
        }
        if (byType.value("") > 0) {
            parts << QString("%1 not imported").arg(locale.toString(byType.value("")));
        }
        if (!parts.isEmpty()) {
            text += "   (" + parts.join(", ") + ")";
        }
    }

    ui->appleMusicMatchCountLabel->setText(text);
    ui->appleMusicMatchCountLabel->setToolTip("");
    ui->appleMusicPreviewButton->setEnabled(true);
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

void PreferencesDialog::on_appleMusicPreviewButton_clicked()
{
    loadAppleMusicLibraryIfNeeded();

    // When a Type field has been chosen, show its raw value next to the Type it produced, so
    //   it's obvious WHY a track came out patter, and which values are still unclassified.
    const int typeFieldIndex = ui->appleMusicTypeFieldCombo->currentIndex();
    const QString typeFieldKey = (typeFieldIndex > 0) ? appleMusicFields[typeFieldIndex - 1].key : QString();
    const bool showTypeField = !typeFieldKey.isEmpty();

    QStringList headers;
    headers << "Type";
    if (showTypeField) {
        headers << appleMusicFields[typeFieldIndex - 1].display;
    }
    headers << "Title" << "Artist" << "Album";
    if (typeFieldKey != "genre") {
        headers << "Genre";
    }
    if (showTypeField && typeFieldKey != "grouping") {
        headers << "Grouping";
    }

    QDialog preview(this);
    preview.setWindowTitle("Apple Music tracks matching the filter");
    preview.resize(880, 500);

    QVBoxLayout *layout = new QVBoxLayout(&preview);

    QTableWidget *table = new QTableWidget(&preview);
    table->setColumnCount(headers.count());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    table->setSortingEnabled(false);

    const int maxRows = 5000;
    int shown = 0;
    int matched = 0;
    QHash<QString,int> byType;

    for (const AppleMusicTrackMeta &track : appleMusicTracks) {
        if (!appleMusicTrackPasses(track)) {
            continue;
        }
        matched++;

        const QString type = appleMusicTypeOf(track);
        byType[type]++;

        if (shown >= maxRows) {
            continue;
        }

        QStringList cells;
        cells << (type.isEmpty() ? QString("(not imported)") : type);
        if (showTypeField) {
            const QString value = appleMusicFieldValue(track, typeFieldKey);
            cells << (value.trimmed().isEmpty() ? QString("(empty)") : value);
        }
        cells << QString::fromStdString(track.title)
              << QString::fromStdString(track.artist)
              << QString::fromStdString(track.album);
        if (typeFieldKey != "genre") {
            cells << QString::fromStdString(track.genre);
        }
        if (showTypeField && typeFieldKey != "grouping") {
            cells << QString::fromStdString(track.grouping);
        }

        table->insertRow(shown);
        for (int col = 0; col < cells.count(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(cells[col]);
            if (col == 0 && type.isEmpty()) {
                item->setForeground(QBrush(QColor("#A0A0A0")));  // this one won't be imported
            }
            table->setItem(shown, col, item);
        }
        shown++;
    }

    table->setSortingEnabled(true);   // sorting by Type is the fastest way to audit the mapping
    table->resizeColumnsToContents();

    QLocale locale;
    QString summary = QString("%1 of %2 songs match.")
                          .arg(locale.toString(matched))
                          .arg(locale.toString((int)appleMusicTracks.size()));

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
    if (!typeParts.isEmpty()) {
        summary += "   Type: " + typeParts.join(", ") + ".";
    }
    if (!showTypeField) {
        summary += "   (No Type field chosen, so every track gets the default Type.)";
    }
    if (matched > shown) {
        summary += QString("   Showing the first %1.").arg(locale.toString(shown));
    }

    QLabel *summaryLabel = new QLabel(summary, &preview);
    summaryLabel->setWordWrap(true);
    layout->addWidget(summaryLabel);
    layout->addWidget(table, 1);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &preview);
    connect(buttons, &QDialogButtonBox::rejected, &preview, &QDialog::reject);
    layout->addWidget(buttons);

    preview.exec();
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
