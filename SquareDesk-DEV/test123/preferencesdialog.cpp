/****************************************************************************
**
** Copyright (C) 2016, 2017, 2018 Mike Pogue, Dan Lyke
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


static const int kSessionsColName = 0;
static const int kSessionsColDay = 1;
static const int kSessionsColTime = 2;
static const int kSessionsColID = 3;

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
static void SetPulldownValuesToItemNumberPlusOne(QComboBox *comboBox)
{
    for (int i = 0; i < comboBox->count(); ++i)
    {
        comboBox->setItemData(i, QVariant(i + 1));
    }
}



// -------------------------------------------------------------------
PreferencesDialog::PreferencesDialog(QString soundFXname[6], QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    songTableReloadNeeded = false;

    ui->setupUi(this);

    {
        ui->tableWidgetSessionsList->setColumnHidden(kSessionsColID, true);
        QHeaderView *headerView = ui->tableWidgetSessionsList->horizontalHeader();
        headerView->setSectionResizeMode(kSessionsColName, QHeaderView::Stretch);
        headerView->setSectionResizeMode(kSessionsColDay, QHeaderView::Interactive);
        headerView->setSectionResizeMode(kSessionsColTime, QHeaderView::Stretch);
    }
    // validator for initial BPM setting
    validator = new QIntValidator(100, 150, this);
    ui->initialBPMLineEdit->setValidator(validator);

    setFontSizes();

    // settings for experimental break/tip timers are:
    SetTimerPulldownValuesToFirstDigit(ui->breakLength);
    SetTimerPulldownValuesToFirstDigit(ui->longTipLength);
    SetPulldownValuesToItemNumberPlusOne(ui->comboBoxMusicFormat);
    SetPulldownValuesToItemNumberPlusOne(ui->comboBoxSessionDefault);

    ui->afterLongTipAction->setItemText(0,"play long tip reminder tone");
    ui->afterBreakAction->setItemText(0,"play break over reminder tone");

    for (int i = 0; i < 6; i++) {
        if (soundFXname != QString("")) {
            ui->afterLongTipAction->setItemText(i+1,"play " + soundFXname[i] + " sound");
            ui->afterBreakAction->setItemText(i+1,"play " + soundFXname[i] + " sound");
        } else {
            ui->afterLongTipAction->setItemText(i+1,"--disabled--");
            ui->afterBreakAction->setItemText(i+1,"--disabled--");
        }
    }

    SetPulldownValuesToItemNumberPlusOne(ui->afterLongTipAction);
    SetPulldownValuesToItemNumberPlusOne(ui->afterBreakAction);

#ifdef KEYBINDINGS_KEY_FIRST
    QVector<KeyAction*> availableActions(KeyAction::availableActions());
    QVector<enum Qt::Key> mappableKeys(KeyAction::mappableKeys());

    ui->tableWidgetKeyBindings->setColumnWidth(0,80);
    QHeaderView *headerView = ui->tableWidgetKeyBindings->horizontalHeader();
    headerView->setSectionResizeMode(1, QHeaderView::Stretch);

    size_t key_binding_count = mappableKeys.length();
    ui->tableWidgetKeyBindings->setRowCount(key_binding_count);
    for (size_t row = 0; row < key_binding_count; ++row)
    {
        QTableWidgetItem *newTableItem(new QTableWidgetItem(QKeySequence(mappableKeys[row]).toString()));
        newTableItem->setFlags(newTableItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidgetKeyBindings->setItem(row, 0, newTableItem);
        QComboBox *comboBox(new QComboBox);
        for (auto action: availableActions)
        {
            QString function_name(action->name());
            comboBox->addItem(function_name, function_name);
        }
        ui->tableWidgetKeyBindings->setCellWidget(row, 1, comboBox);
    }
#else
    QVector<KeyAction*> availableActions(KeyAction::availableActions());
    QVector<enum Qt::Key> mappableKeys(KeyAction::mappableKeys());

    ui->tableWidgetKeyBindings->setRowCount(availableActions.length());
    for (int row = 0; row < availableActions.length(); ++row)
    {
        QTableWidgetItem *newTableItem(new QTableWidgetItem(availableActions[row]->name()));
        ui->tableWidgetKeyBindings->setItem(row, 0, newTableItem);
        QKeySequenceEdit *keySequenceEdit(new QKeySequenceEdit);
        ui->tableWidgetKeyBindings->setCellWidget(row, 1, keySequenceEdit);
    }

    ui->tableWidgetKeyBindings->resizeColumnToContents(0);
    ui->tableWidgetKeyBindings->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
#endif


    ui->tabWidget->setCurrentIndex(0); // Music tab (not Experimental tab) is primary, regardless of last setting in Qt Designer
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
    ui->timersHelpLabel->setFont(font);
    ui->clockColoringHelpLabel->setFont(font);
    ui->musicTypesHelpLabel->setFont(font);
    ui->musicFormatHelpLabel->setFont(font);
//    ui->saveSongPrefsHelpLabel->setFont(font);

}


QHash<Qt::Key, KeyAction *> PreferencesDialog::getHotkeys()
{
    QHash<Qt::Key, KeyAction *> keyActionBindings;
#ifdef KEYBINDINGS_KEY_FIRST
    QHash<QString, KeyAction*> actions(KeyAction::actionNameToActionMappings());

    QVector<Qt::Key> mappableKeys(KeyAction::mappableKeys());
    QHash<QString, Qt::Key> keysByName;
    for (auto key : mappableKeys)
    {
        keysByName[QKeySequence(key).toString()] = key;
    }

    for (int row = 0; row < ui->tableWidgetKeyBindings->rowCount(); ++row)
    {
        QString keyName = ui->tableWidgetKeyBindings->item(row,0)->text();
        QComboBox *comboBox = dynamic_cast<QComboBox*>(ui->tableWidgetKeyBindings->cellWidget(row, 1));
        if (comboBox->currentIndex() > 0)
        {
            QString actionName = comboBox->itemData(comboBox->currentIndex()).toString();
            auto action = actions.find(actionName);
            QHash<QString, Qt::Key>::iterator key = keysByName.find(keyName);

            if (action != actions.end() && key != keysByName.end())
            {
                keyActionBindings[*key] = *action;
            }
        }
    }
#else
    QHash<QString, KeyAction*> actions(KeyAction::actionNameToActionMappings());
    for (int row = 0; row < ui->tableWidgetKeyBindings->rowCount(); ++row)
    {
        QString actionName(ui->tableWidgetKeyBindings->item(row, 0)->text());
        QKeySequenceEdit *keySequenceEdit = dynamic_cast<QKeySequenceEdit*>(ui->tableWidgetKeyBindings->cellWidget(row,1));
        QKeySequence keySequence = keySequenceEdit->keySequence();
        for (int i = 0; i < keySequence.count(); ++i)
        {
            keyActionBindings[static_cast<Qt::Key>(keySequence[i])] = actions[actionName];
        }
    }
#endif

    return keyActionBindings;
}



void PreferencesDialog::setHotkeys(QHash<Qt::Key, KeyAction *> keyActions)
{
#if 0
    QHash<QString, KeyAction*> actionsByName(KeyAction::actionNameToActionMappings());
    QVector<Qt::Key> mappableKeys(KeyAction::mappableKeys());
    QHash<QString, Qt::Key> keysByName;
    for (auto key : mappableKeys)
    {
        keysByName[QKeySequence(key).toString()] = key;
    }
    for (int row = 0; row < ui->tableWidgetKeyBindings->rowCount(); ++row)
    {
        int selectedKeyRow = 0;
        QComboBox *comboBox = dynamic_cast<QComboBox *>(ui->tableWidgetKeyBindings->cellWidget(row, 1));
        auto keyName = ui->tableWidgetKeyBindings->item(row,0)->text();
        auto key = keysByName[keyName];
        auto keyAction = keyActions.find(key);
        if (keyAction != keyActions.end())
        {
            for (int i = 0; i < comboBox->count(); ++i)
            {
                if (comboBox->itemData(i).toString() == keyAction.value()->name())
                {
                    selectedKeyRow = i;
                    break;
                }
            }
        }
        comboBox->setCurrentIndex(selectedKeyRow);
    }
#else
    QHash<QString, QKeySequence > keysByActionName;

    for (QHash<Qt::Key, KeyAction *>::iterator keyAction = keyActions.begin();
         keyAction != keyActions.end();
         ++keyAction)
    {
        auto keyMap = keysByActionName.find(keyAction.value()->name());
        if (keyMap != keysByActionName.end())
        {
            switch (keyMap.value().count())
            {
            case 1 :
                keysByActionName[keyAction.value()->name()] =
                    QKeySequence(keyMap.value()[0],keyAction.key());
                break;
            case 2 :
                keysByActionName[keyAction.value()->name()] =
                    QKeySequence(keyMap.value()[0],keyMap.value()[1],keyAction.key());
                break;
            case 3 :
                keysByActionName[keyAction.value()->name()] =
                    QKeySequence(keyMap.value()[0],keyMap.value()[1],
                                keyMap.value()[2],keyAction.key());
                break;

            default :
                qDebug() << "Whoah, too many or too few keyMap values " << keyMap.value().count();
                break;
            }
        }
        else
        {
            keysByActionName[keyAction.value()->name()] = QKeySequence(keyAction.key());
        }
    }

    for (int row = 0; row < ui->tableWidgetKeyBindings->rowCount(); ++row)
    {
        QKeySequenceEdit *keySequenceEdit = dynamic_cast<QKeySequenceEdit *>(ui->tableWidgetKeyBindings->cellWidget(row, 1));
        auto actionName = ui->tableWidgetKeyBindings->item(row,0)->text();
        auto keyAction = keysByActionName.find(actionName);
        if (keyAction != keysByActionName.end())
        {
            keySequenceEdit->setKeySequence(keyAction.value());
        }
    }

#endif
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
        if (ui->tableWidgetSessionsList->item(row, kSessionsColID) != NULL)
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
    int lastSelectedRow = -1;
    for (int row = 0; row < ui->tableWidgetSessionsList->rowCount(); ++row)
    {
        for (int col = 0; col < ui->tableWidgetSessionsList->columnCount(); ++col)
        {
            QTableWidgetItem *item = ui->tableWidgetSessionsList->item(row,col);
            if (item && item->isSelected())
            {
                lastSelectedRow = row;
            }
        }
    }
    if (lastSelectedRow < 0)
    {
        lastSelectedRow = ui->tableWidgetSessionsList->rowCount();
    }
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
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Reset Hotkeys To Defaults",
                                  "Do you really want to reset all hotkeys back to their default? This operation cannot be undone.",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
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

        if (chosenColor.name() == "#ffffff") {
            calledColorString = DEFAULTCALLEDCOLOR;  // a way to reset the colors individually
        }

        const QString COLOR_STYLE("QPushButton { background-color : %1; color : %2; }");
        ui->calledColorButton->setStyleSheet(COLOR_STYLE.arg(calledColorString).arg(calledColorString));
        ui->calledColorButton->setAutoFillBackground(true);
        ui->calledColorButton->setFlat(true);
        ui->calledColorButton->setText(calledColorString);  // remember it in the control's text

        songTableReloadNeeded = true;  // change to colors requires reload of the songTable
    }
}

void PreferencesDialog::on_extrasColorButton_clicked()
{
    QString extrasColorString = ui->extrasColorButton->text();
    QColor chosenColor = QColorDialog::getColor(QColor(extrasColorString), this); //return the color chosen by user
    if (chosenColor.isValid()) {
        extrasColorString = chosenColor.name();

        if (chosenColor == "#ffffff") {
            extrasColorString = DEFAULTEXTRASCOLOR;  // a way to reset the colors individually
        }

        const QString COLOR_STYLE("QPushButton { background-color : %1; color : %2; }");
        ui->extrasColorButton->setStyleSheet(COLOR_STYLE.arg(extrasColorString).arg(extrasColorString));
        ui->extrasColorButton->setAutoFillBackground(true);
        ui->extrasColorButton->setFlat(true);
        ui->extrasColorButton->setText(extrasColorString);  // remember it in the control's text

        songTableReloadNeeded = true;  // change to colors requires reload of the songTable
    }
}

void PreferencesDialog::on_patterColorButton_clicked()
{
    QString patterColorString = ui->patterColorButton->text();
    QColor chosenColor = QColorDialog::getColor(QColor(patterColorString), this); //return the color chosen by user
    if (chosenColor.isValid()) {
        patterColorString = chosenColor.name();

        if (chosenColor == "#ffffff") {
            patterColorString = DEFAULTPATTERCOLOR;  // a way to reset the colors individually
        }

        const QString COLOR_STYLE("QPushButton { background-color : %1; color : %2; }");
        ui->patterColorButton->setStyleSheet(COLOR_STYLE.arg(patterColorString).arg(patterColorString));
        ui->patterColorButton->setAutoFillBackground(true);
        ui->patterColorButton->setFlat(true);
        ui->patterColorButton->setText(patterColorString);  // remember it in the control's text

        songTableReloadNeeded = true;  // change to colors requires reload of the songTable
    }
}

void PreferencesDialog::on_singingColorButton_clicked()
{
    QString singingColorString = ui->singingColorButton->text();
    QColor chosenColor = QColorDialog::getColor(QColor(singingColorString), this); //return the color chosen by user
    if (chosenColor.isValid()) {
        singingColorString = chosenColor.name();

        if (chosenColor == "#ffffff") {
            singingColorString = DEFAULTSINGINGCOLOR;  // a way to reset the colors individually
        }

        const QString COLOR_STYLE("QPushButton { background-color : %1; color : %2; }");
        ui->singingColorButton->setStyleSheet(COLOR_STYLE.arg(singingColorString).arg(singingColorString));
        ui->singingColorButton->setAutoFillBackground(true);
        ui->singingColorButton->setFlat(true);
        ui->singingColorButton->setText(singingColorString);  // remember it in the control's text

        songTableReloadNeeded = true;  // change to colors requires reload of the songTable
    }
}


/* See the large comment at the top of prefs_options.h */

#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_INT_NO_PREFS(name,default)

#define CONFIG_ATTRIBUTE_STRING(control, name, default)                 \
    QString PreferencesDialog::Get##name() { return ui->control->text(); } \
    void PreferencesDialog::Set##name(QString value) { ui->control->setText(value); }

#define CONFIG_ATTRIBUTE_COLOR(control, name, default)                 \
    QString PreferencesDialog::Get##name() { return ui->control->text(); } \
    void PreferencesDialog::Set##name(QString value) \
    { \
       const QString COLOR_STYLE("QPushButton { background-color : %1; color : %2; }"); \
       ui->control->setStyleSheet(COLOR_STYLE.arg(value).arg(value)); \
       ui->control->setAutoFillBackground(true); \
       ui->control->setFlat(true); \
       ui->control->setText(value); \
    }

#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) \
    bool PreferencesDialog::Get##name() { return ui->control->isChecked(); } \
    void PreferencesDialog::Set##name(bool value) { ui->control->setChecked(value); }

#define CONFIG_ATTRIBUTE_COMBO(control, name, default) \
    int PreferencesDialog::Get##name() { return ui->control->itemData(ui->control->currentIndex()).toInt(); } \
    void PreferencesDialog::Set##name(int value) \
    { for (int i = 0; i < ui->control->count(); ++i) { \
            if (ui->control->itemData(i).toInt() == value) { ui->control->setCurrentIndex(i); break; } \
        } }

#define CONFIG_ATTRIBUTE_INT(control, name, default)                 \
    int PreferencesDialog::Get##name() { return ui->control->text().toInt(); } \
    void PreferencesDialog::Set##name(int value) { ui->control->setText(QString::number(value)); }

#include "prefs_options.h"

#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR
#undef CONFIG_ATTRIBUTE_INT

#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
#undef CONFIG_ATTRIBUTE_INT_NO_PREFS

void PreferencesDialog::on_initialBPMLineEdit_textChanged(const QString &arg1)
{
    int pos = 0;
    bool acceptable = (ui->initialBPMLineEdit->validator()->validate((QString &)arg1,pos) == QValidator::Acceptable);

    const QString COLOR_STYLE("QLineEdit { background-color : %1; }");
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
