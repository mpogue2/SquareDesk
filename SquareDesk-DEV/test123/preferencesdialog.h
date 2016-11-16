#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QSettings>

namespace Ui {
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
    QString GetMusicTypeSinging();
    QString GetMusicTypePatter();
    QString GetMusicTypeExtras();
    QString GetMusicTypeCalled();

private slots:
    void on_chooseMusicPathButton_clicked();

    void on_EnableTimersTabCheckbox_toggled(bool checked);

    void on_EnablePitchTempoViewCheckbox_toggled(bool checked);

    void on_EnableClockColoring_toggled(bool checked);

private:
    Ui::PreferencesDialog *ui;
};

#endif // PREFERENCESDIALOG_H
