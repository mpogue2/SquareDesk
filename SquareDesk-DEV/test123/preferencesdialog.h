/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
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

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QValidator>

#include "common_enums.h"
#include "default_colors.h"
#include "keybindings.h"
#include <QPushButton>

class SessionInfo;

namespace Ui
{
class PreferencesDialog;
}



class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    PreferencesDialog(QMap<int, QString> *soundFXname, QWidget *parent = nullptr); // 0);
    ~PreferencesDialog();
    void finishPopulation();

    bool swallowSoundFX;   // avoid spurious playback events, when we're setting up the dialog

    QString musicPath;
    void setFontSizes();

    QValidator *validator;

    bool songTableReloadNeeded;

    QHash<QString, KeyAction *> getHotkeys();
    void setHotkeys(QHash<QString, KeyAction *>);
    void setTagColors( const QHash<QString,QPair<QString,QString>> &);
    QHash<QString,QPair<QString,QString>> getTagColors();

    void setSessionInfoList(const QList<SessionInfo> &);
    QList<SessionInfo> getSessionInfoList();
    void setTagColor(const QString &tagName, const QString &color, bool foreground);

    int getActiveTab();
    void setActiveTab(int);

//    void setColorSwatches(QString patter, QString singing, QString called, QString extras);
//    QColor patterColor;
//    QColor singingColor;
//    QColor calledColor;
//    QString calledColor;
//    QColor extrasColor;

//    void setDefaultColors(QString patter, QString singing, QString called, QString extras);
//    QString defaultPatterColor, defaultSingingColor, defaultCalledColor, defaultExtrasColor;

/* See the large comment at the top of prefs_options.h */

#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_INT_NO_PREFS(name,default)

#define CONFIG_ATTRIBUTE_STRING(control, name, default) QString Get##name() const; void Set##name(QString value);
#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) bool Get##name() const; void Set##name(bool value);
#define CONFIG_ATTRIBUTE_COMBO(control, name, default) int Get##name() const; void Set##name(int value);
#define CONFIG_ATTRIBUTE_COLOR(control, name, default) QString Get##name() const; void Set##name(QString value);
#define CONFIG_ATTRIBUTE_INT(control, name, default) int Get##name() const; void Set##name(int value);
#define CONFIG_ATTRIBUTE_SLIDER(control, name, default) int Get##name() const; void Set##name(int value);

    #include "prefs_options.h"

#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_INT_NO_PREFS

#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR
#undef CONFIG_ATTRIBUTE_INT
#undef CONFIG_ATTRIBUTE_SLIDER

private slots:
    void on_chooseMusicPathButton_clicked();

    void on_calledColorButton_clicked();
    void on_extrasColorButton_clicked();
    void on_patterColorButton_clicked();
    void on_singingColorButton_clicked();

    void on_initialBPMLineEdit_textChanged(const QString &arg1);

    void on_lineEditMusicTypePatter_textChanged(const QString &arg1);

    void on_lineEditMusicTypeExtras_textChanged(const QString &arg1);

    void on_lineEditMusicTypeSinging_textChanged(const QString &arg1);

    void on_lineEditMusicTypeCalled_textChanged(const QString &arg1);

    void on_comboBoxMusicFormat_currentIndexChanged(int /* currentIndex */);
    void on_pushButtonResetHotkeysToDefaults_clicked();

    void on_toolButtonSessionAddItem_clicked();
    void on_toolButtonSessionMoveItemDown_clicked();
    void on_toolButtonSessionMoveItemUp_clicked();
    void on_toolButtonSessionRemoveItem_clicked();
    void on_pushButtonTagsBackgroundColor_clicked();
    void on_pushButtonTagsForegroundColor_clicked();

    void on_pushButtonTagAdd_clicked();
    void on_pushButtonTagRemove_clicked();
    void on_tabWidget_currentChanged(int /* tab */);
    void on_afterLongTipAction_currentIndexChanged(int index);

    void on_afterBreakAction_currentIndexChanged(int index);

//    void on_replayGainCheckbox_toggled(bool checked);

    void on_intelCenterFreqDial_valueChanged(int value);

    void on_intelWidthDial_valueChanged(int value);

    void on_intelGainDial_valueChanged(int value);

    void on_intelResetButton_clicked();

    void on_intelBoostEnabledCheckbox_toggled(bool checked);

    void onSortIndicatorChanged(int column, Qt::SortOrder order);


    void on_panEQGainDial_valueChanged(int value);

private:
    void SetLabelTagAppearanceColors();
    Ui::PreferencesDialog *ui;
    MainWindow *mw;  // so we can play soundFX from within the dialog

    Qt::SortOrder sortOrder;
};

class PushButtonColorTag : public QPushButton {
    Q_OBJECT
private:
    PreferencesDialog *prefsDialog;
    QString tagName;
    QString color;
    bool foreground;
    
public:
    PushButtonColorTag(PreferencesDialog *prefsDialog,
                       const QString &tagName,
                       const QString &initialColor,
                       bool foreground);
private slots:
    void selectColor();
};

extern bool LongStringsFirstThenAlpha(const QString &a, const QString &b);

#endif // PREFERENCESDIALOG_H
