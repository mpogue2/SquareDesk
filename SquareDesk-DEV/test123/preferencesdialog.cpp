#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

static struct {
    const char *description;
    const char *tag;
}
    kSONG_NAME_FORMATS[] =
    {
        { "Label-# - Song name.mp3", "label_dash_name" },
        { "Song name - Label-#.mp3", "name_dash_label" }
    };

void setCheckboxFromString(QSettings &MySettings, QCheckBox *checkBox, const char *key, QString &value)
{
    value = MySettings.value(key).toString();
    checkBox->setChecked(value == "true");
}


PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    // musicPath preference -------
    QSettings MySettings;
    musicPath = MySettings.value("musicPath").toString();
    ui->musicPath->setText(musicPath);

    setCheckboxFromString(MySettings, ui->EnableTimersTabCheckbox,
                          "experimentalTimersTabEnabled",
                          experimentalTimersTabEnabled);

    setCheckboxFromString(MySettings, ui->EnableCuesheetTabCheckbox,
                          "experimentalCuesheetTabEnabled",
                          experimentalTimersTabEnabled);
    setCheckboxFromString(MySettings, ui->EnablePitchTempoViewCheckbox,
                          "experimentalPitchTempoViewEnabled",
                          experimentalPitchTempoViewEnabled);
    setCheckboxFromString(MySettings, ui->EnablePitchTempoViewCheckbox,
                          "experimentalPitchTempoViewEnabled",
                          experimentalPitchTempoViewEnabled);

    reverseLabelTitle = MySettings.value("reverselabeltitle").toString();
    int selectedTag = 1;
    for (size_t i = 0;
         i < sizeof(kSONG_NAME_FORMATS) / sizeof(kSONG_NAME_FORMATS[0]);
         ++i)
    {
        ui->comboBoxSongNameFormat->insertItem(i,
                                               kSONG_NAME_FORMATS[i].description,
                                               kSONG_NAME_FORMATS[i].tag);
        qDebug() << "Comparing " << kSONG_NAME_FORMATS[i].tag << " == " << reverseLabelTitle;
        if (kSONG_NAME_FORMATS[i].tag == reverseLabelTitle)
        {
            qDebug() << "Selected tag is now " << selectedTag;
            selectedTag = i;
        }
    }
    ui->comboBoxSongNameFormat->setCurrentIndex(selectedTag);
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
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

void PreferencesDialog::on_EnableCuesheetTabCheckbox_toggled(bool checked)
{
    if (checked) {
        experimentalCuesheetTabEnabled = "true";
    } else {
        experimentalCuesheetTabEnabled = "false";
    }
//    qDebug() << "User selected timers tab: " << experimentalCuesheetTabEnabled;
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

void PreferencesDialog::on_comboBoxSongNameFormat_currentIndexChanged(int index)
{
    qDebug() << "Pre compare Setting index to " << index;
    if (index >= 0 && index < (int)(sizeof(kSONG_NAME_FORMATS) / sizeof(kSONG_NAME_FORMATS[0])))
    {
        qDebug() << "Setting index to " << index;
        reverseLabelTitle = kSONG_NAME_FORMATS[index].tag;
    }
}
