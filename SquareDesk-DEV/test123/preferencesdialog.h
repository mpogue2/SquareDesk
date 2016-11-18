#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QSettings>

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
    QString experimentalPitchTempoViewEnabled;
    QString experimentalClockColoringEnabled;

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

private slots:
    void on_chooseMusicPathButton_clicked();
    void on_EnableTimersTabCheckbox_toggled(bool checked);
    void on_EnablePitchTempoViewCheckbox_toggled(bool checked);
    void on_EnableClockColoring_toggled(bool checked);

    void on_calledColorButton_clicked();
    void on_extrasColorButton_clicked();
    void on_patterColorButton_clicked();
    void on_singingColorButton_clicked();

private:
    Ui::PreferencesDialog *ui;
};

#endif // PREFERENCESDIALOG_H
