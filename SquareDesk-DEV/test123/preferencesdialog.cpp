#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "QColorDialog"

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



PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);
#if 0
//    qDebug() << "PreferencesDialog::PreferencesDialog()";
    // musicPath preference -------
    QSettings MySettings;
    musicPath = MySettings.value("musicPath").toString();
    ui->musicPath->setText(musicPath);

    // Timers tab (experimental) preference -------
    experimentalTimersTabEnabled = MySettings.value("experimentalTimersTabEnabled").toString();
//    qDebug() << "preferencesDialog, expTimers = " << experimentalTimersTabEnabled;  // FIX
    if (experimentalTimersTabEnabled == "true") {
        ui->EnableTimersTabCheckbox->setChecked(true);
    }
    else {
        ui->EnableTimersTabCheckbox->setChecked(false);
    }

    // Pitch/Tempo View (experimental) preference -------
    experimentalPitchTempoViewEnabled = MySettings.value("experimentalPitchTempoViewEnabled").toString();
//    qDebug() << "preferencesDialog, expTimers = " << experimentalPitchTempoViewEnabled;  // FIX
    if (experimentalPitchTempoViewEnabled == "true") {
        ui->EnablePitchTempoViewCheckbox->setChecked(true);
    }
    else {
        ui->EnablePitchTempoViewCheckbox->setChecked(false);
    }

    // Clock coloring (experimental) preference -------
    experimentalClockColoringEnabled = MySettings.value("experimentalClockColoringEnabled").toString();
//    qDebug() << "preferencesDialog, expTimers = " << experimentalPitchTempoViewEnabled;  // FIX
    if (experimentalClockColoringEnabled == "true") {
        ui->EnableClockColoring->setChecked(true);
    }
    else {
        ui->EnableClockColoring->setChecked(false);
    }

    QString value;

    value = MySettings.value("SongPreferencesInConfig").toString();
    if (value == "true") {
        ui->checkBoxSaveSongPreferencesInConfig->setChecked(true);
    }
    else {
        ui->checkBoxSaveSongPreferencesInConfig->setChecked(false);
    }


    value = MySettings.value("MusicTypeSinging").toString();
    if (!value.isNull()) {
        ui->lineEditMusicTypeSinging->setText(value);
    }

    value = MySettings.value("MusicTypePatter").toString();
    if (!value.isNull()) {
        ui->lineEditMusicTypePatter->setText(value);
    }

    value = MySettings.value("MusicTypeExtras").toString();
    if (!value.isNull()) {
        ui->lineEditMusicTypeExtras->setText(value);
    }

    value = MySettings.value("MusicTypeCalled").toString();
    if (!value.isNull()) {
        ui->lineEditMusicTypeCalled->setText(value);
    }


    enum SongFilenameMatchingType songFilenameFormat = SongFilenameLabelDashName;
    if (!MySettings.value("SongFilenameFormat").isNull()) {
        songFilenameFormat = (SongFilenameMatchingType)(MySettings.value("SongFilenameFormat").toInt());
    }
#endif

#if 0
    for (int i = 0; i < ui->comboBoxMusicFormat->maxCount(); ++i) {
        if (songFilenameFormat == ui->comboBoxMusicFormat->itemData(i).toInt()) {
            ui->comboBoxMusicFormat->setCurrentIndex(i);
            break;
        }
    }
#endif
    setFontSizes();
//    setColorSwatches("#ff0000","#00ff00","#0000ff","#ffffff");  // DO NOT DO THIS HERE. PARENT MUST DO THIS.

    // settings for experimental break/tip timers are:
    // tipLengthTimerEnabled
    // tipLengthTimerLength
    // tipLengthAlarmAction
    // breakLengthTimerEnabled
    // breakLengthTimerLength
    // breakLengthAlarmAction
#if 0
    ui->longTipCheckbox->setChecked(MySettings.value("tipLengthTimerEnabled") == "true");
    tipLengthTimerEnabledString = MySettings.value("tipLengthTimerEnabled").toString();

    unsigned int tt = MySettings.value("tipLengthTimerLength").toInt();
    ui->longTipLength->setCurrentIndex(tt-5);
    tipLength = tt;

    unsigned int tt2 = MySettings.value("tipLengthAlarmAction").toInt();
    ui->afterLongTipAction->setCurrentIndex(tt2);
    tipAlarmAction = tt2;

    ui->breakTimerCheckbox->setChecked(MySettings.value("breakLengthTimerEnabled") == "true");
    breakLengthTimerEnabledString = MySettings.value("breakLengthTimerEnabled").toString();

    unsigned int bb = MySettings.value("breakLengthTimerLength").toInt();
    ui->breakLength->setCurrentIndex(bb/5 - 1);
    breakLength = bb;

    unsigned int bb2 = MySettings.value("breakLengthAlarmAction").toInt();
    ui->afterBreakAction->setCurrentIndex(bb2);
    breakAlarmAction = bb2;
#endif

    SetTimerPulldownValuesToFirstDigit(ui->breakLength);
    SetTimerPulldownValuesToFirstDigit(ui->longTipLength);
    SetPulldownValuesToItemNumberPlusOne(ui->comboBoxMusicFormat);

    // Initialize the color chooser buttons
    setColorSwatches(GetpatterColorString(), GetsingingColorString(), GetcalledColorString(), GetextrasColorString());
}


PreferencesDialog::~PreferencesDialog()
{
//    qDebug() << "    PreferencesDialog::~PreferencesDialog()";
    delete ui;
}

void PreferencesDialog::setColorSwatches(QString patter, QString singing, QString called, QString extras) {
    const QString COLOR_STYLE("QPushButton { background-color : %1; color : %2; }");

    qDebug() << "setColorSwatches: " << patter << singing << called << extras;

    QColor patterColor1(patter);
    patterColor = patterColor1;
    ui->patterColorButton->setStyleSheet(COLOR_STYLE.arg(patterColor1.name()).arg(patterColor1.name()));
    ui->patterColorButton->setAutoFillBackground(true);
    ui->patterColorButton->setFlat(true);

    QColor singingColor1(singing);
    singingColor = singingColor1;
    ui->singingColorButton->setStyleSheet(COLOR_STYLE.arg(singingColor1.name()).arg(singingColor1.name()));
    ui->singingColorButton->setAutoFillBackground(true);
    ui->singingColorButton->setFlat(true);

    QColor calledColor1(called);
//    calledColorString = calledColor1.name();
    ui->calledColorButton->setStyleSheet(COLOR_STYLE.arg(calledColor1.name()).arg(calledColor1.name()));
    ui->calledColorButton->setAutoFillBackground(true);
    ui->calledColorButton->setFlat(true);
    ui->calledColorButton->setText(called);  // save it in the text of the button

    QColor extrasColor1(extras);
    extrasColor = extrasColor1;
    ui->extrasColorButton->setStyleSheet(COLOR_STYLE.arg(extrasColor1.name()).arg(extrasColor1.name()));
    ui->extrasColorButton->setAutoFillBackground(true);
    ui->extrasColorButton->setFlat(true);
}

//void PreferencesDialog::setDefaultColors(QString patter, QString singing, QString called, QString extras) {
//    defaultPatterColor=patter;
//    defaultSingingColor=singing;
//    defaultCalledColor=called;
//    defaultExtrasColor=extras;
//}


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
    ui->pitchTempoHelpLabel->setFont(font);
    ui->clockColoringHelpLabel->setFont(font);
    ui->musicTypesHelpLabel->setFont(font);
    ui->musicFormatHelpLabel->setFont(font);
    ui->saveSongPrefsHelpLabel->setFont(font);

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

    ui->musicPath->setText(dir);
    musicPath = dir;
    // NOTE: saving of Preferences is done at the dialog caller site, not here.
}

// ------------

void PreferencesDialog::on_calledColorButton_clicked()
{
    QString calledColorString = ui->calledColorButton->text();
//    qDebug() << "calledColor going in (string): " << calledColorString;
    QColor chosenColor = QColorDialog::getColor(QColor(calledColorString), this); //return the color chosen by user
    if (chosenColor.isValid()) {
        calledColorString = chosenColor.name();

        if (chosenColor.name() == "#ffffff") {
            qDebug() << "setting calledColorString back to default: " << DEFAULTCALLEDCOLOR;
            calledColorString = DEFAULTCALLEDCOLOR;  // a way to reset the colors individually
        }

        const QString COLOR_STYLE("QPushButton { background-color : %1; color : %2; }");
        ui->calledColorButton->setStyleSheet(COLOR_STYLE.arg(calledColorString).arg(calledColorString));
        ui->calledColorButton->setAutoFillBackground(true);
        ui->calledColorButton->setFlat(true);
        ui->calledColorButton->setText(calledColorString);  // remember it in the control's text
    }

//    qDebug() << "on_calledColorButton_clicked(): updated called color (string and button text) = " << calledColorString;
}

void PreferencesDialog::on_extrasColorButton_clicked()
{
    QString extrasColorString = ui->extrasColorButton->text();
//    qDebug() << "extrasColor going in (string): " << extrasColorString;
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
    }

//    qDebug() << "on_extrasColorButton_clicked(): updated extras color (string and button text) = " << extrasColorString;
}

// TODO: I wonder if this was my problem...I was creating the preferences dialog with new() and exec()...FIX THIS?
//  http://stackoverflow.com/questions/25315408/how-to-cancel-out-of-qcolordialoggetcolor

void PreferencesDialog::on_patterColorButton_clicked()
{
    QString patterColorString = ui->patterColorButton->text();
//    qDebug() << "patterColor going in (string): " << patterColorString;
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
    }

//    qDebug() << "on_patterColorButton_clicked(): updated patter color (string and button text) = " << patterColorString;
}

void PreferencesDialog::on_singingColorButton_clicked()
{
    QString singingColorString = ui->singingColorButton->text();
//    qDebug() << "singingColor going in (string): " << singingColorString;
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
    }

//    qDebug() << "on_singingColorButton_clicked(): updated singing color (string and button text) = " << singingColorString;
}


/* See the large comment at the top of prefs_options.h */

#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name,default)

#define CONFIG_ATTRIBUTE_STRING(control, name, default)                 \
    QString PreferencesDialog::Get##name() { return ui->control->text(); } \
    void PreferencesDialog::Set##name(QString value) { ui->control->setText(value); }

#define CONFIG_ATTRIBUTE_COLOR(control, name, default)                 \
    QString PreferencesDialog::Get##name() { qDebug() << "COLOR PD:Get" << #name << ",returning:" << ui->control->text();  return ui->control->text(); } \
    void PreferencesDialog::Set##name(QString value) \
    { \
       qDebug() << "COLOR PreferencesDialog:Set" << #name << #control << value; \
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
            if (ui->control->itemData(i).toInt() == value) { ui->control->setCurrentIndex(value); break; } \
        } }
#include "prefs_options.h"
#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR
#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
