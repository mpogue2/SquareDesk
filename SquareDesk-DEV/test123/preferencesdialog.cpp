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
#include <algorithm>

static const int kSessionsColName = 0;
static const int kSessionsColDay = 1;
static const int kSessionsColTime = 2;
static const int kSessionsColID = 3;

static const int kTagsColTag = 0;
static const int kTagsColForeground = 1;
static const int kTagsColBackground = 2;
static const int kTagsColExample = 3;


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

    {
        QHeaderView *headerView = ui->tableWidgetTagColors->horizontalHeader();
        headerView->setSectionResizeMode(kTagsColExample, QHeaderView::Stretch);
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


    QVector<KeyAction*> availableActions(KeyAction::availableActions());

    ui->tableWidgetKeyBindings->setRowCount(availableActions.length());
    for (int row = 0; row < availableActions.length(); ++row)
    {
        QTableWidgetItem *newTableItem(new QTableWidgetItem(availableActions[row]->name()));
        ui->tableWidgetKeyBindings->setItem(row, 0, newTableItem);
        for (int col = 1; col < ui->tableWidgetKeyBindings->columnCount(); ++col)
        {
            QKeySequenceEdit *keySequenceEdit(new QKeySequenceEdit);
            ui->tableWidgetKeyBindings->setCellWidget(row, col, keySequenceEdit);
        }
    }

    ui->tableWidgetKeyBindings->resizeColumnToContents(0);
    ui->tableWidgetKeyBindings->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);


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
    pushButton->setStyleSheet(COLOR_STYLE.arg(color));
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


static bool LongStringsFirstThenAlpha(const QString &a, const QString &b)
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
        if (keyAction != keysByActionName.end())
        {
            QStringList keys(keyAction.value());
            std::sort(keys.begin(), keys.end(), LongStringsFirstThenAlpha);
            for (int col = 1;
                 col < ui->tableWidgetKeyBindings->columnCount();
                 ++col)
                 {
                     QKeySequenceEdit *keySequenceEdit = dynamic_cast<QKeySequenceEdit *>(ui->tableWidgetKeyBindings->cellWidget(row, col));

                     if (col <= keys.length())
                     {
                         qDebug() << "Setting " << row << "," << col << " : " << keys[col - 1];
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

        if (chosenColor == "#ffffff") {
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

        if (chosenColor == "#ffffff") {
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

        if (chosenColor == "#ffffff") {
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
    QString format("<span style=\"background-color:%1; color:%2\"> tags </span>&nbsp;<span style=\"background-color:%1; color:%2\"> look </span>&nbsp;<span style=\"background-color:%1; color:%2\"> like </span>&nbsp;<span style=\"background-color:%1; color:%2\"> this </span>");
    QString str(format.arg(ui->pushButtonTagsBackgroundColor->text()).arg(ui->pushButtonTagsForegroundColor->text()));
    ui->labelTagAppearance->setText(str);
}


void PreferencesDialog::finishPopulation()
{
    SetLabelTagAppearanceColors();
}
