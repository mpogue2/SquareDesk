#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QValidator>

#include "common_enums.h"

namespace Ui
{
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();

    QString musicPath;
    QString experimentalTimersTabEnabled;
    QString experimentalLyricsTabEnabled;
    QString experimentalPitchTempoViewEnabled;
    QString experimentalClockColoringEnabled;

    QString tipLengthTimerEnabledString;
    QString breakLengthTimerEnabledString;
    unsigned int tipLength;
    unsigned int breakLength;
    unsigned int tipAlarmAction;
    unsigned int breakAlarmAction;

    void setFontSizes();
    void setColorSwatches(QString patter, QString singing, QString called, QString extras);
    QColor patterColor;
    QColor singingColor;
    QColor calledColor;
    QColor extrasColor;

    void setDefaultColors(QString patter, QString singing, QString called, QString extras);
    QString defaultPatterColor, defaultSingingColor, defaultCalledColor, defaultExtrasColor;

    QString GetMusicTypeSinging();
    QString GetMusicTypePatter();
    QString GetMusicTypeExtras();
    QString GetMusicTypeCalled();
    enum SongFilenameMatchingType GetSongFilenameFormat();
    bool GetSaveSongPreferencesInMainConfig();

    QValidator *validator;

    bool bpmEnabled;
    unsigned int bpmTarget;

private slots:
    void on_chooseMusicPathButton_clicked();
    void on_EnableTimersTabCheckbox_toggled(bool checked);
    void on_EnableLyricsTabCheckbox_toggled(bool checked);
    void on_EnablePitchTempoViewCheckbox_toggled(bool checked);
    void on_EnableClockColoring_toggled(bool checked);

    void on_calledColorButton_clicked();
    void on_extrasColorButton_clicked();
    void on_patterColorButton_clicked();
    void on_singingColorButton_clicked();

    void on_longTipCheckbox_toggled(bool checked);
    void on_longTipLength_currentIndexChanged(int index);
    void on_afterLongTipAction_currentIndexChanged(int index);

    void on_breakTimerCheckbox_toggled(bool checked);
    void on_breakLength_currentIndexChanged(int index);
    void on_afterBreakAction_currentIndexChanged(int index);

    void on_initialBPM_textChanged(const QString &arg1);

    void on_initialBPMcheckbox_toggled(bool checked);

private:
    Ui::PreferencesDialog *ui;
};

#endif // PREFERENCESDIALOG_H
