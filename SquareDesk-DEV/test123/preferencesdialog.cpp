#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    // musicPath preference -------
    QSettings MySettings;
    musicPath = MySettings.value("musicPath").toString();
    ui->musicPath->setText(musicPath);

    // Timers tab (experimental) preference -------
    experimentalTimersTabEnabled = MySettings.value("experimentalTimersTabEnabled").toString();
//    qDebug() << "preferencesDialog, expTimers = " << experimentalTimersTabEnabled;  // FIX
    if (experimentalTimersTabEnabled == "true") {
        ui->EnableTimersTabCheckbox->setChecked(true);
    } else {
        ui->EnableTimersTabCheckbox->setChecked(false);
    }

    // Pitch/Tempo View (experimental) preference -------
    experimentalPitchTempoViewEnabled = MySettings.value("experimentalPitchTempoViewEnabled").toString();
//    qDebug() << "preferencesDialog, expTimers = " << experimentalPitchTempoViewEnabled;  // FIX
    if (experimentalPitchTempoViewEnabled == "true") {
        ui->EnablePitchTempoViewCheckbox->setChecked(true);
    } else {
        ui->EnablePitchTempoViewCheckbox->setChecked(false);
    }

    // Clock coloring (experimental) preference -------
    experimentalClockColoringEnabled = MySettings.value("experimentalClockColoringEnabled").toString();
//    qDebug() << "preferencesDialog, expTimers = " << experimentalPitchTempoViewEnabled;  // FIX
    if (experimentalClockColoringEnabled == "true") {
        ui->EnableClockColoring->setChecked(true);
    } else {
        ui->EnableClockColoring->setChecked(false);
    }

    setFontSizes();
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::setFontSizes() {

    int preferredSmallFontSize;
#if defined(Q_OS_MAC)
    preferredSmallFontSize = 11;
#elif defined(Q_OS_WIN32)
    preferredSmallFontSize = 8;
#elif defined(Q_OS_LINUX)
    preferredSmallFontSize = 13;  // FIX: is this right?
#endif

    QFont font = ui->musicDirHelpLabel->font();
    font.setPointSize(preferredSmallFontSize);

    ui->musicDirHelpLabel->setFont(font);
    ui->timersHelpLabel->setFont(font);
    ui->pitchTempoHelpLabel->setFont(font);
    ui->clockColoringHelpLabel->setFont(font);
}

void PreferencesDialog::on_chooseMusicPathButton_clicked()
{
    QString dir =
        QFileDialog::getExistingDirectory(this, tr("Select Base Directory for Music"),
                                                 QDir::homePath(),
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    if (dir.isNull()) {
//        qDebug() << "User cancelled.";
        return;  // user cancelled the "Select Base Directory for Music" dialog...so don't do anything, just return
    }

//    qDebug() << "User selected directory: " << dir;
    ui->musicPath->setText(dir);
    musicPath = dir;
    // NOTE: saving of Preferences is done at the dialog caller site, not here.
}

void PreferencesDialog::on_EnableTimersTabCheckbox_toggled(bool checked)
{
    if (checked) {
        experimentalTimersTabEnabled = "true";
    } else {
        experimentalTimersTabEnabled = "false";
    }
//    qDebug() << "User selected timers tab: " << experimentalTimersTabEnabled;
    // NOTE: saving of Preferences is done at the dialog caller site, not here.
}

void PreferencesDialog::on_EnablePitchTempoViewCheckbox_toggled(bool checked)
{
    if (checked) {
        experimentalPitchTempoViewEnabled = "true";
    } else {
        experimentalPitchTempoViewEnabled = "false";
    }
//    qDebug() << "User selected Pitch Tempo View Enabled: " << experimentalPitchTempoViewEnabled;
    // NOTE: saving of Preferences is done at the dialog caller site, not here.
}

void PreferencesDialog::on_EnableClockColoring_toggled(bool checked)
{
    if (checked) {
        experimentalClockColoringEnabled = "true";
    } else {
        experimentalClockColoringEnabled = "false";
    }
    //    qDebug() << "User selected Clock Coloring Enabled: " << experimentalClockColoringEnabled;
        // NOTE: saving of Preferences is done at the dialog caller site, not here.
}
