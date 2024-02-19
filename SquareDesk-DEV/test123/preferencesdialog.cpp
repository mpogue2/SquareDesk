/****************************************************************************
**
** Copyright (C) 2016-2024 Mike Pogue, Dan Lyke
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
#include "mainwindow.h"

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


    ui->tabWidget->setCurrentIndex(0); // Music tab (not Experimental tab) is primary, regardless of last setting in Qt Designer

    on_intelBoostEnabledCheckbox_toggled(ui->intelBoostEnabledCheckbox->isChecked());
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

    QString foreground = foregroundButton->text();
    QString background = backgroundButton->text();
    QString str("<span style=\"background-color:%1; color: %2;\">&nbsp;");
    str = str.arg(background).arg(foreground);
    str += tag;
    str += "&nbsp;</span>";
    labelItem->setText(str);
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
    int row = getLastSelectedRow(tableWidget);
    tableWidget->insertRow(row);
    tableWidget->setCellWidget(row, kTagsColForeground, foregroundButton);
    tableWidget->setCellWidget(row, kTagsColBackground, backgroundButton);
    QTableWidgetItem *widgetItem(new QTableWidgetItem( tag ));
    tableWidget->setItem(row, kTagsColTag, widgetItem);
    
    QLabel *labelItem(new QLabel(tableWidget));
    labelItem->setAlignment(Qt::AlignCenter);
    labelItem->setTextFormat(Qt::RichText);
    tableWidget->setCellWidget(row, kTagsColExample, labelItem);

    setPushButtonColor(backgroundButton, background);
    setPushButtonColor(foregroundButton, foreground);
    setTagColorsSample(tableWidget, row);
}


void PreferencesDialog::on_pushButtonTagAdd_clicked()
{
    songTableReloadNeeded = true;
    addRowToTagColors(this, ui->tableWidgetTagColors, "<new>", ui->pushButtonTagsBackgroundColor->text(), ui->pushButtonTagsForegroundColor->text());
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
    comboDay->addItem("<none>", 0);
    comboDay->addItem("Monday", 1);
    comboDay->addItem("Tuesday", 2);
    comboDay->addItem("Wednesday", 3);
    comboDay->addItem("Thursday", 4);
    comboDay->addItem("Friday", 5);
    comboDay->addItem("Saturday", 6);
    comboDay->addItem("Sunday", 7);
    return comboDay;
}


static QTimeEdit *timeSelectionControl(int start_minutes)
{
    QTimeEdit *timeEdit = new QTimeEdit(QTime((int)(start_minutes / 60),
                                              start_minutes % 60));
    timeEdit->setDisplayFormat("h:mm AP");
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
        comboDay->setCurrentIndex(session.day_of_week);
        ui->tableWidgetSessionsList->setCellWidget(row, 1, comboDay);

        QTimeEdit *timeEdit = timeSelectionControl(session.start_minutes);
                
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
}

int PreferencesDialog::getActiveTab()
{
    return ui->tabWidget->currentIndex();
}

void PreferencesDialog::setActiveTab(int tabnum)
{
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

