#include "mainwindow.h"
#include "ui_mainwindow.h"

// build target is now Mac OS X 10.7:
// http://stackoverflow.com/questions/24243176/how-to-specify-target-mac-os-x-version-using-qmake
// I modified qmake.conf like this:
//    ~/Qt/5.7/clang_64/mkspecs/macx-clang/qmake.conf
//to:
//#
//# qmake configuration for Clang on OS X
//#
//MAKEFILE_GENERATOR      = UNIX
//CONFIG                 += app_bundle incremental global_init_link_order lib_version_first plugin_no_soname
//QMAKE_INCREMENTAL_STYLE = sublib
//include(../common/macx.conf)
//include(../common/gcc-base-mac.conf)
//include(../common/clang.conf)
//include(../common/clang-mac.conf)
//#QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
//QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
//load(qt_config)
//
// TO DEPLOY:
//
// 1) change version number in AboutBox() below
// 2) clean all, compile all, test
// 3) change version in package.command file
// 4) double click package.command file, wait for it to complete
// 5) retest

// =================================================================================================
// SquareDeskPlayer Keyboard Shortcuts:
//
// function                 MAC                  		PC
// -------------------------------------------------------------------------------------------------
// FILE MENU
// open                     Cmd-O                		Ctrl-O, Alt-F-O
// save                     Cmd-S                		Ctrl-S, Alt-F-S
// save as                  Shft-Cmd-S           		Alt-F-A
// quit                     Cmd-Q                		Ctrl-F4, Alt-F-E
//
// MUSIC MENU
// play/pause               space                		space, Alt-M-P
// rewind/stop              S, ESC, END, Cmd-.   		S, ESC, END, Alt-M-S, Ctrl-.
// rewind/play (playing)    HOME, . (while playing) 	HOME, .  (while playing)
// skip/back 5 sec          Cmd-RIGHT/LEFT,RIGHT/LEFT   Ctrl-RIGHT/LEFT, RIGHT/LEFT, Alt-M-A/Alt-M-B
// volume up/down           Cmd-UP/DOWN,UP/DOWN         Ctrl-UP/DOWN, UP/DOWN
// mute                     Cmd-M, M                	Ctrl-M, M
// go faster                Cmd-+,+,=            		Ctrl-+,+,=
// go slower                Cmd--,-              		Ctrl--,-
// force mono                                    		Alt-M-F
// clear search             Cmd-/                		Alt-M-S
// pitch up                 Cmd-U, U                	Ctrl-U, U, Alt-M-U
// pitch down               Cmd-D, D                	Ctrl-D, D, Alt-M-D

// GLOBALS:
bass_audio cBass;

// ----------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
//    ui->statusBar->showMessage("Current song: Brandy [base: 3:45, 127BPM]");
    ui->statusBar->showMessage("");

    this->setWindowTitle(QString("SquareDesk Music Player/Editor"));

    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

    ui->playButton->setEnabled(false);
    ui->stopButton->setEnabled(false);

    // ============
    ui->menuFile->addSeparator();

    // ------------
#if defined(Q_OS_MAC)
    // NOTE: MAC OS X ONLY
    QAction *aboutAct = new QAction(QIcon(), tr("&About SquareDesk..."), this);
    aboutAct->setStatusTip(tr("SquareDesk Information"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(aboutBox()));
    ui->menuFile->addAction(aboutAct);
#endif

    // ==============
#if defined(Q_OS_WIN)
    // HELP MENU IS WINDOWS ONLY
    QMenu *helpMenu = new QMenu("&Help");

    // ------------
    QAction *aboutAct2 = new QAction(QIcon(), tr("About &SquareDesk..."), this);
    aboutAct2->setStatusTip(tr("SquareDesk Information"));
    connect(aboutAct2, SIGNAL(triggered()), this, SLOT(aboutBox()));
    helpMenu->addAction(aboutAct2);
    menuBar()->addAction(helpMenu->menuAction());
#endif

    // ------------
#if defined(Q_OS_WIN)
    // NOTE: WINDOWS ONLY
    closeAct = new QAction(QIcon(), tr("&Exit"), this);
    closeAct->setShortcuts(QKeySequence::Close);
    closeAct->setStatusTip(tr("Exit the program"));
    connect(closeAct, SIGNAL(triggered()), this, SLOT(close()));
    ui->menuFile->addAction(closeAct);
#endif

    currentState = kStopped;
    currentPitch = 0;

    Info_Seekbar(false);

    // setup playback timer
    QTimer *UIUpdateTimer = new QTimer(this);
    connect(UIUpdateTimer, SIGNAL(timeout()), this, SLOT(on_UIUpdateTimerTick()));
    UIUpdateTimer->start(1000);           //adjust from GUI with timer->setInterval(newValue)

    closeEventHappened = false;

    ui->songTable->clearSelection();
    ui->songTable->clearFocus();

    //Create Bass audio system
    cBass.Init();

    //Set UI update
    cBass.SetVolume(100);
    currentVolume = 100;
    Info_Volume();

    // where is the root directory where all the music is stored?
    pathStack = new QList<QString>();

    QSettings MySettings; // Will be using application information for correct location of your settings
    musicRootPath = MySettings.value("musicPath").toString();
    if (musicRootPath.isNull()) {
        musicRootPath = QDir::homePath() + "/music";
        MySettings.setValue("musicPath", musicRootPath); // set to music subdirectory in user's Home directory, if nothing else
    }

    // FIX USE ONLY IN CASE OF EMERGENCY: **********************************
//    musicRootPath = QDir::homePath() + "/___SquareDanceMusic";
//    MySettings.setValue("musicPath", musicRootPath); // set to music subdirectory in user's Home directory, if nothing else
    // FIX: **********************************

    // used to store the file paths
    findMusic();  // get the filenames from the user's directories
    filterMusic(); // and filter them into the songTable

    ui->songTable->setColumnWidth(0,60);
    ui->songTable->setColumnWidth(1,60);
    ui->songTable->setColumnWidth(2,100);

    // -----------
    const QString AUTOSTART_KEY("autostartplayback");  // default is AUTOSTART ENABLED
    QString autoStartChecked = MySettings.value(AUTOSTART_KEY).toString();

    if (autoStartChecked.isNull()) {
        // first time through, AUTOPLAY is UNCHECKED
        autoStartChecked = "unchecked";
        MySettings.setValue(AUTOSTART_KEY, "unchecked");
    }

    if (autoStartChecked == "checked") {
        ui->actionAutostart_playback->setChecked(true);
    } else {
        ui->actionAutostart_playback->setChecked(false);
    }

    // -------
    // TODO: move keys to better location...
    const QString FORCEMONO_KEY("forcemono");  // default is FALSE (use stereo)
    QString forceMonoChecked = MySettings.value(FORCEMONO_KEY).toString();

    if (forceMonoChecked.isNull()) {
        // first time through, FORCE MONO is FALSE (stereo mode is the default)
        forceMonoChecked = "false";  // FIX: needed?
        MySettings.setValue(FORCEMONO_KEY, "false");
    }

    if (forceMonoChecked == "true") {
//        ui->actionForce_Mono_Aahz_mode->setChecked(true);
        ui->monoButton->setChecked(true);
        on_monoButton_toggled(true);  // sets button and menu item
    } else {
//        ui->actionForce_Mono_Aahz_mode->setChecked(false);
        ui->monoButton->setChecked(false);
        on_monoButton_toggled(false);  // sets button and menu item
    }

//    // -------
//    // TODO: move keys to better location...
//    const QString FONTSIZE_KEY("fontsize");  // default is 13
//    QString fontsize = MySettings.value(FONTSIZE_KEY).toString();

//    if (fontsize.isNull()) {
//        // first time through, FORCE MONO is FALSE (stereo mode is the default)
//        fontsize = "13";  // FIX: needed?
//        MySettings.setValue(FONTSIZE_KEY, "13");
//    }

//    iFontsize = fontsize.toInt();
//    qDebug() << "font size preference is:" << iFontsize;

//    QFont font = ui->currentTempoLabel->font();
//    font.setPointSize(18);
//    ui->currentTempoLabel->setFont(font);
//    ui->tempoLabel->setFont(font);
//    ui->currentPitchLabel->setFont(font);
//    ui->pitchLabel->setFont(font);
//    ui->currentVolumeLabel->setFont(font);
//    ui->volumeLabel->setFont(font);
//    ui->mixLabel->setFont(font);
//    ui->currentMixLabel->setFont(font);
//    ui->currentLocLabel->setFont(font);
//    ui->songLengthLabel->setFont(font);
////    ui->EQgroup->setFont(font);
///

    inPreferencesDialog = false;
}

// ----------------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
}

// ----------------------------------------------------------------------
void MainWindow::on_loopButton_toggled(bool checked)
{
    if (checked) {
        // regular text: "LOOP"
        QFont f = ui->loopButton->font();
        f.setStrikeOut(false);
        ui->loopButton->setFont(f);
        ui->loopButton->setText("LOOP");
        ui->actionLoop->setChecked(true);

        ui->seekBar->SetLoop(true);

        double songLength = cBass.FileLength;
        cBass.SetLoop(songLength * 0.9, songLength * 0.1); // FIX: use parameters in the MP3 file

    }
    else {
        // strikethrough text: "LOOP"
        QFont f = ui->loopButton->font();
        f.setStrikeOut(true);
        ui->loopButton->setFont(f);
        ui->loopButton->setText("LOOP");
        ui->actionLoop->setChecked(false);

        ui->seekBar->SetLoop(false);

        cBass.ClearLoop();
    }
//    qDebug() << "LOOP: TO BE IMPLEMENTED";
}

// ----------------------------------------------------------------------
void MainWindow::on_monoButton_toggled(bool checked)
{
    if (checked) {
        ui->monoButton->setText("MONO");
        ui->actionForce_Mono_Aahz_mode->setChecked(true);
        cBass.SetMono(true);
    }
    else {
        ui->monoButton->setText("STEREO");
        ui->actionForce_Mono_Aahz_mode->setChecked(false);
        cBass.SetMono(false);
    }

    // NOTE: the Action comes here...
    // the Force Mono (Aahz Mode) setting is persistent across restarts of the application
    QSettings MySettings; // Will be using application information for correct location of your settings
    const QString FORCEMONO_KEY("forcemono");  // default is AUTOSTART ENABLED

    if (ui->actionForce_Mono_Aahz_mode->isChecked()) {
        MySettings.setValue(FORCEMONO_KEY, "true");
//        qDebug() << "Setting force mono to TRUE";
    } else {
        MySettings.setValue(FORCEMONO_KEY, "false");
//        qDebug() << "Setting force mono to FALSE";
    }


}

// ----------------------------------------------------------------------
void MainWindow::on_stopButton_clicked()
{
    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
    ui->actionPlay->setText("Play");  // now stopped, press Cmd-P to Play
    currentState = kStopped;

    cBass.Stop();  // Stop playback, rewind to the beginning

    ui->seekBar->setValue(0);
    Info_Seekbar(false);  // update just the text  FIX: is this correct?
}

// ----------------------------------------------------------------------
void MainWindow::on_playButton_clicked()
{
    cBass.Play();  // currently paused, so start playing
    if (currentState == kStopped || currentState == kPaused) {
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));  // change PLAY to PAUSE
        ui->actionPlay->setText("Pause");
        currentState = kPlaying;
    }
    else {
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
        ui->actionPlay->setText("Play");
        currentState = kPaused;
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_pitchSlider_valueChanged(int value)
{
    cBass.SetPitch(value);
    currentPitch = value;
    QString plural;
    if (currentPitch == 1 || currentPitch == -1) {
        plural = "";
    }
    else {
        plural = "s";
    }
    QString sign = "";
    if (currentPitch > 0) {
        sign = "+";
    }
    ui->currentPitchLabel->setText(sign + QString::number(currentPitch) +" semitone" + plural);
}

// ----------------------------------------------------------------------
void MainWindow::Info_Volume(void)
{
    if (cBass.Stream_Volume == 0) {
        ui->currentVolumeLabel->setText("Mute");
    }
    else if (cBass.Stream_Volume == 100) {
        ui->currentVolumeLabel->setText("MAX");
    }
    else {
        ui->currentVolumeLabel->setText(QString::number(cBass.Stream_Volume)+"%");
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_volumeSlider_valueChanged(int value)
{
    cBass.SetVolume(value);
    currentVolume = value;

    Info_Volume();

    if (value == 0) {
        ui->actionMute->setText("Un&mute");
    }
    else {
        ui->actionMute->setText("&Mute");
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_actionMute_triggered()
{
    if (ui->volumeSlider->value() != 0) {
        previousVolume = ui->volumeSlider->value();
        ui->volumeSlider->setValue(0);
        ui->actionMute->setText("Un&mute");
    }
    else {
        ui->volumeSlider->setValue(previousVolume);
        ui->actionMute->setText("&Mute");
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_tempoSlider_valueChanged(int value)
{
    float baseBPM = (float)round(127);  // original song
    float desiredBPM = (float)value;    // desired
    int newBASStempo = (int)(round(100.0*desiredBPM/baseBPM));
    cBass.SetTempo(newBASStempo);
    ui->currentTempoLabel->setText(QString::number(value) + " BPM");
}

// ----------------------------------------------------------------------
void MainWindow::on_mixSlider_valueChanged(int value)
{
    int Rpercent = (int)(100.0*((float)value + 100.0)/200.0);
    int Lpercent = 100-Rpercent;
    QString s = QString::number(Lpercent) + "% L / " + QString::number(Rpercent) + "% R ";
    ui->currentMixLabel->setText(s);
    cBass.SetPan(value/100.0);
}

// ----------------------------------------------------------------------
QString MainWindow::position2String(int position)
{
    int songMin = position/60;
    int songSec = position - 60*songMin;
    QString songSecString = QString("%1").arg(songSec, 2, 10, QChar('0')); // pad with zeros
    QString s(QString::number(songMin) + ":" + songSecString);
    return s;
}

// ----------------------------------------------------------------------
void MainWindow::Info_Seekbar(bool forceSlider)
{

//    int fileLength = 225;
    if (songLoaded) {  // FIX: this needs to pay attention to the bool
        // FIX: this code doesn't need to be executed so many times.
        ui->seekBar->setMinimum(0);
        ui->seekBar->setMaximum((int)cBass.FileLength-1); // NOTE: tricky, counts on == below
        ui->seekBar->setTickInterval(10);  // 10 seconds per tick
        cBass.StreamGetPosition();  // update cBass.Current_Position

        int currentPos_i = (int)cBass.Current_Position;
//        int currentPos_i = ui->seekBar->value();
        if (forceSlider) {
            ui->seekBar->blockSignals(true); // setValue should NOT initiate a valueChanged()
            ui->seekBar->setValue(currentPos_i);
            ui->seekBar->blockSignals(false);
        }
        int fileLen_i = (int)cBass.FileLength;
//        int fileLen_i = 225;  // 3 minutes 45 seconds

        if (currentPos_i == fileLen_i) {  // NOTE: tricky, counts on -1 above
            // avoids the problem of manual seek to max slider value causing auto-STOP
//            qDebug() << "Reached the end of playback!";
            on_stopButton_clicked(); // pretend we pressed the STOP button when EOS is reached
            return;
        }

        ui->currentLocLabel->setText(position2String(currentPos_i));
        ui->songLengthLabel->setText("/ " + position2String(fileLen_i));

//        highlightSyncTextAt(cBass.Current_Position);
    }
    else {
        ui->seekBar->setMinimum(0);
        ui->seekBar->setValue(0);
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_seekBar_valueChanged(int value)
{
    // These must happen in this order.
    cBass.StreamSetPosition(value);
//    if (value > -1) {  // turn off the warning...
    Info_Seekbar(false);
//    }
}

// ----------------------------------------------------------------------
void MainWindow::on_clearSearchButton_clicked()
{
    ui->labelSearch->setPlainText("");
    ui->labelNumberSearch->setPlainText("");
    ui->typeSearch->setPlainText("");
    ui->titleSearch->setPlainText("");

    ui->labelSearch->clearFocus();
    ui->labelNumberSearch->clearFocus();
    ui->typeSearch->clearFocus();
    ui->titleSearch->clearFocus();
}

// ----------------------------------------------------------------------
void MainWindow::on_actionLoop_triggered()
{
    on_loopButton_toggled(ui->actionLoop->isChecked());
}

// ----------------------------------------------------------------------
void MainWindow::on_UIUpdateTimerTick(void)
{
//    if (currentState == kPlaying) {
//        ui->seekBar->setValue(ui->seekBar->value() + 1);
//        Info_Seekbar(true);
//    }

    Info_Seekbar(true);
}

// --------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    // Work around bug: https://codereview.qt-project.org/#/c/125589/
    if (closeEventHappened) {
        event->accept();
        return;
    }
    closeEventHappened = true;
    if (true) {
//        writeSettings();
        on_actionAutostart_playback_triggered();  // write AUTOPLAY setting back
        event->accept();  // OK to close, if user said "OK" or "SAVE"
    }
    else {
        event->ignore();  // do not close, if used said "CANCEL"
        closeEventHappened = false;
    }
}

// ------------------------------------------------------------------------------------------
void MainWindow::aboutBox()
{
    QMessageBox msgBox;
    msgBox.setText(QString("<p><h2>SquareDesk Player, V0.4.4</h2>") +
                   QString("<p>See our website at <a href=\"http://squaredesk.net\">squaredesk.net</a></p>") +
                   QString("Uses: <a href=\"http://www.un4seen.com/bass.html\">libbass</a> and ") +
                   QString("<a href=\"http://www.jobnik.org/?mnu=bass_fx\">libbass_fx</a>") +
//                   QString("<a href=\"https://taglib.github.io\">taglib</a>") +
                   QString("<p>Thanks to: <a href=\"http://all8.com\">all8.com</a>"));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

// ------------------------------------------------------------------------
// http://www.codeprogress.com/cpp/libraries/qt/showQtExample.php?key=QApplicationInstallEventFilter&index=188
bool GlobalEventFilter::eventFilter(QObject *Object, QEvent *Event)
{
    if (Event->type() == QEvent::KeyPress) {
        QKeyEvent *KeyEvent = (QKeyEvent *)Event;

        if (!(ui->labelSearch->hasFocus() || ui->labelNumberSearch->hasFocus() ||
                ui->typeSearch->hasFocus() || ui->titleSearch->hasFocus())) {
            // call handleKeypress on the Applications's active window
            ((MainWindow *)(((QApplication *)Object)->activeWindow()))->handleKeypress(KeyEvent->key());
            return true;
        }
    }
    return QObject::eventFilter(Object,Event);
}

// ----------------------------------------------------------------------
//void MainWindow::keyPressEvent(QKeyEvent *event)
void MainWindow::handleKeypress(int key)
{
//    qDebug() << "key =" << key << ", isPreferencesDialog =" << inPreferencesDialog;
    if (inPreferencesDialog) {
        return;
    }

    // FIX: should this be commented out, or not?
//if (cBass.bPaused) {
////        qDebug() << "    paused... " << endl;
////        event->ignore();
//        return;
//    }

    switch (key) {

        case Qt::Key_Escape:
        case Qt::Key_End:  // FIX: should END go to the end of the song? or stop playback?
        case Qt::Key_S:
            on_stopButton_clicked();
            break;

        case Qt::Key_P:
        case Qt::Key_Space:  // for SqView compatibility ("play/pause")
            //cBass.Play();  // if Stopped, PLAY;  if Playing, Pause.  If Paused, Resume.
            on_playButton_clicked();
            break;

        case Qt::Key_Home:
        case Qt::Key_Period:  // for SqView compatibility ("restart")
            on_stopButton_clicked();
            on_playButton_clicked();
            break;

        case Qt::Key_Right:
            on_actionSkip_Ahead_15_sec_triggered();
            break;
        case Qt::Key_Left:
            on_actionSkip_Back_15_sec_triggered();
            break;

        case Qt::Key_Backspace:  // either one will delete a row
        case Qt::Key_Delete:
//            qDebug() << "DELETE";
            break;

        case Qt::Key_Down:
            ui->volumeSlider->setValue(ui->volumeSlider->value() - 5);
            break;
        case Qt::Key_Up:
            ui->volumeSlider->setValue(ui->volumeSlider->value() + 5);
            break;

        case Qt::Key_Plus:
        case Qt::Key_Equal:
            on_actionSpeed_Up_triggered();
            break;
        case Qt::Key_Minus:
            on_actionSlow_Down_triggered();
            break;

        case Qt::Key_M:
            on_actionMute_triggered();
            break;

        case Qt::Key_U:
            on_actionPitch_Up_triggered();
            break;

        case Qt::Key_D:
            on_actionPitch_Down_triggered();
            break;

        default:
            break;
    }
    Info_Seekbar(true);
}

// ------------------------------------------------------------------------
void MainWindow::on_actionSpeed_Up_triggered()
{
    ui->tempoSlider->setValue(ui->tempoSlider->value() + 1);
    on_tempoSlider_valueChanged(ui->tempoSlider->value());
}

void MainWindow::on_actionSlow_Down_triggered()
{
    ui->tempoSlider->setValue(ui->tempoSlider->value() - 1);
    on_tempoSlider_valueChanged(ui->tempoSlider->value());
}

// ------------------------------------------------------------------------
void MainWindow::on_actionSkip_Ahead_15_sec_triggered()
{
    if (!cBass.bPaused) {
        cBass.StreamGetPosition();  // update the position
        cBass.StreamSetPosition((int)fmin(cBass.Current_Position + 15.0, cBass.FileLength));
    }
    Info_Seekbar(true);
}

void MainWindow::on_actionSkip_Back_15_sec_triggered()
{
    Info_Seekbar(true);
    if (!cBass.bPaused) {
        cBass.StreamGetPosition();  // update the position
        cBass.StreamSetPosition((int)fmax(cBass.Current_Position - 15.0, 0.0));
    }
}

// ------------------------------------------------------------------------
void MainWindow::on_actionVolume_Up_triggered()
{
    ui->volumeSlider->setValue(ui->volumeSlider->value() + 5);
}

void MainWindow::on_actionVolume_Down_triggered()
{
    ui->volumeSlider->setValue(ui->volumeSlider->value() - 5);
}

// ------------------------------------------------------------------------
void MainWindow::on_actionPlay_triggered()
{
    on_playButton_clicked();
}

void MainWindow::on_actionStop_triggered()
{
    on_stopButton_clicked();
}

// ------------------------------------------------------------------------
void MainWindow::on_actionForce_Mono_Aahz_mode_triggered()
{
    on_monoButton_toggled(ui->actionForce_Mono_Aahz_mode->isChecked());
}

// ------------------------------------------------------------------------
void MainWindow::on_bassSlider_valueChanged(int value)
{
//    qDebug() << "bass slider changed:" << value;
    cBass.SetEq(0, (float)value);
}

void MainWindow::on_midrangeSlider_valueChanged(int value)
{
//    qDebug() << "midrange slider changed:" << value;
    cBass.SetEq(1, (float)value);
}

void MainWindow::on_trebleSlider_valueChanged(int value)
{
//    qDebug() << "treble slider changed:" << value;
    cBass.SetEq(2, (float)value);
}

void MainWindow::loadMP3File(QString MP3FileName, QString songTitle, QString songType) {
    // FIX: get just the base name here...
    QStringList pieces = MP3FileName.split( "/" );
    QString filebase = pieces.value(pieces.length()-1);
    QStringList pieces2 = filebase.split(".");

    currentMP3filename = pieces2.value(pieces2.length()-2);

    if (songTitle != "") {
        ui->nowPlayingLabel->setText(songTitle);
    } else {
        ui->nowPlayingLabel->setText(currentMP3filename);  // FIX?  convert to short version?
    }

//    QFileInfo fi(MP3FileName);
    QDir md(MP3FileName);
    QString canonicalFN = md.canonicalPath();
//    qDebug() << "MP3FileName: " << MP3FileName;
//    qDebug() << "canonicalFN(): " << canonicalFN;

    cBass.StreamCreate(MP3FileName.toStdString().c_str());

    QStringList ss = MP3FileName.split('/');
    QString fn = ss.at(ss.size()-1);
    this->setWindowTitle(fn + QString(" - SquareDesk MP3 Player/Editor"));

    int length_sec = cBass.FileLength;
//    int length_min = length_sec/60;
//    int length_rsec = length_sec - 60*length_min;
    int songBPM = round(cBass.Stream_BPM);

    if ((songBPM>=110) && (songBPM<=140)) {
        ui->currentTempoLabel->setText(QString::number(songBPM) + " BPM");
        ui->tempoSlider->setMinimum(songBPM-15);
        ui->tempoSlider->setMaximum(songBPM+15);
        ui->tempoSlider->setValue(songBPM);
        ui->tempoSlider->setEnabled(true);
        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
                                 ", base tempo: " + QString::number(songBPM) + " BPM");
    }
    else {
        ui->currentTempoLabel->setText(" unknown BPM");
        ui->tempoSlider->setEnabled(false);
        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
                                 ", base tempo: unknown BPM");
    }

    fileModified = false;

    ui->playButton->setEnabled(true);
    ui->stopButton->setEnabled(true);

    ui->actionPlay->setEnabled(true);
    ui->actionStop->setEnabled(true);
    ui->actionSkip_Ahead_15_sec->setEnabled(true);
    ui->actionSkip_Back_15_sec->setEnabled(true);

    ui->seekBar->setEnabled(true);

    ui->pitchSlider->setEnabled(true);
    ui->pitchSlider->setValue(0);           // TODO: get from the file itself
    ui->currentPitchLabel->setText("0 semitones");

    ui->volumeSlider->setEnabled(true);
    ui->volumeSlider->setValue(100);
    ui->currentVolumeLabel->setText("Max");

    ui->mixSlider->setEnabled(true);
    ui->mixSlider->setValue(0);
    ui->currentMixLabel->setText("50% L / 50% R");

    ui->actionMute->setEnabled(true);
    ui->actionLoop->setEnabled(true);

    ui->actionVolume_Down->setEnabled(true);
    ui->actionVolume_Up->setEnabled(true);
    ui->actionSpeed_Up->setEnabled(true);
    ui->actionSlow_Down->setEnabled(true);
    ui->actionForce_Mono_Aahz_mode->setEnabled(true);
    ui->actionPitch_Down->setEnabled(true);
    ui->actionPitch_Up->setEnabled(true);

    ui->bassSlider->setEnabled(true);
    ui->midrangeSlider->setEnabled(true);
    ui->trebleSlider->setEnabled(true);

    ui->loopButton->setEnabled(true);
    ui->monoButton->setEnabled(true);

    cBass.Stop();  // FIX: is this still needed?

    songLoaded = true;
    Info_Seekbar(true);

    if (songType == "patter") {
        ui->loopButton->setChecked(true);
        on_loopButton_toggled(true); // default is to loop, if type is patter
    } else {
        // not patter, so Loop mode defaults to OFF
        ui->loopButton->setChecked(false);
        on_loopButton_toggled(false); // default is to loop, if type is patter
//        on_loopButton_toggled(ui->loopButton->isChecked()); // set up the loop, if looping is enabled...
    }

//    on_monoButton_toggled(ui->monoButton->isChecked()); // set up for MONO, if MONO is turned on...
}

void MainWindow::on_actionOpen_MP3_file_triggered()
{
    on_stopButton_clicked();  // if we're loading a new MP3 file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    const QString DEFAULT_DIR_KEY("default_dir");
    QSettings MySettings; // Will be using application informations for correct location of your settings

//    QString as = MySettings.value(QString("autostartplayback")).toString();
//    qDebug() << "as = " << as;

    QString startingDirectory = MySettings.value(DEFAULT_DIR_KEY).toString();
//    qDebug() << "startingDirectory = " << startingDirectory;
    if (startingDirectory.isNull()) {
        // first time through, start at HOME
        startingDirectory = QDir::homePath();
    }

    QString MP3FileName =
        QFileDialog::getOpenFileName(this,
                                     tr("Import Audio File"),
//                                         QDir::homePath(),
                                     startingDirectory,
                                     tr("Audio Files (*.mp3 *.wav)"));
    if (MP3FileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
    QDir CurrentDir;
    MySettings.setValue(DEFAULT_DIR_KEY, CurrentDir.absoluteFilePath(MP3FileName));

    // --------
    qDebug() << "loading: " << MP3FileName;
    loadMP3File(MP3FileName, QString(""), QString(""));  // "" means use title from the filename

//    ui->nowPlayingLabel->setText(currentMP3filename);

//    cBass.StreamCreate(MP3FileName.toStdString().c_str());

//    QStringList ss = MP3FileName.split('/');
//    QString fn = ss.at(ss.size()-1);
//    this->setWindowTitle(fn + QString(" - SquareDesk MP3 Player/Editor"));

//    int length_sec = cBass.FileLength;
////    int length_min = length_sec/60;
////    int length_rsec = length_sec - 60*length_min;
//    int songBPM = round(cBass.Stream_BPM);

//    if ((songBPM>=110) && (songBPM<=140)) {
//        ui->currentTempoLabel->setText(QString::number(songBPM) + " BPM");
//        ui->tempoSlider->setMinimum(songBPM-10);
//        ui->tempoSlider->setMaximum(songBPM+10);
//        ui->tempoSlider->setValue(songBPM);
//        ui->tempoSlider->setEnabled(true);
//        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
//                                 ", base tempo: " + QString::number(songBPM) + " BPM");
//    }
//    else {
//        ui->tempoLabel->setText(" unknown BPM");
//        ui->tempoSlider->setEnabled(false);
//        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
//                                 ", base tempo: unknown BPM");
//    }

//    fileModified = false;

//    ui->playButton->setEnabled(true);
//    ui->stopButton->setEnabled(true);

//    ui->actionPlay->setEnabled(true);
//    ui->actionStop->setEnabled(true);
//    ui->actionSkip_Ahead_15_sec->setEnabled(true);
//    ui->actionSkip_Back_15_sec->setEnabled(true);

//    ui->seekBar->setEnabled(true);

//    ui->pitchSlider->setEnabled(true);
//    ui->pitchSlider->setValue(0);           // TODO: get from the file itself
//    ui->currentPitchLabel->setText("0 semitones");

//    ui->volumeSlider->setEnabled(true);
//    ui->volumeSlider->setValue(100);
//    ui->currentVolumeLabel->setText("Max");

//    ui->mixSlider->setEnabled(true);
//    ui->mixSlider->setValue(0);
//    ui->currentMixLabel->setText("50% L / 50% R");

//    ui->actionVolume_Down->setEnabled(true);
//    ui->actionVolume_Up->setEnabled(true);
//    ui->actionSpeed_Up->setEnabled(true);
//    ui->actionSlow_Down->setEnabled(true);
//    ui->actionForce_Mono_Aahz_mode->setEnabled(true);

//    ui->bassSlider->setEnabled(true);
//    ui->midrangeSlider->setEnabled(true);
//    ui->trebleSlider->setEnabled(true);

//    ui->loopButton->setEnabled(true);
//    ui->monoButton->setEnabled(true);

//    cBass.Stop();  // FIX: is this still needed?

//    songLoaded = true;
//    Info_Seekbar(true);

//    on_loopButton_toggled(ui->loopButton->isChecked()); // set up the loop, if looping is enabled...

//    on_monoButton_toggled(ui->monoButton->isChecked()); // set up for MONO, if MONO is turned on...
}

// ==========================================================================================
MySlider::MySlider(QWidget *parent) : QSlider(parent)
{
//    QWidget::QWidget(parent);
    drawLoopPoints = false;
}

void MySlider::SetLoop(bool b)
{
    drawLoopPoints = b;
    update();
}

void MySlider::mouseDoubleClickEvent(QMouseEvent *event)  // FIX: this doesn't work
{
    Q_UNUSED(event)
    setValue(0);
    valueChanged(0);
//    QSlider::mouseDoubleClickEvent(event);
}

// http://stackoverflow.com/questions/3894737/qt4-how-to-draw-inside-a-widget
void MySlider::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);

    if(drawLoopPoints) {
        int offset = 8;  // for the handles
        int height = this->height();
        int width = this->width() - 2 * offset;

        QPen pen;  //   = (QApplication::palette().dark().color());
        pen.setColor(Qt::blue);
        painter.setPen(pen);

        float from = 0.9f;
        QLineF line1(from * width + offset, 3,          from * width + offset, height-4);
        QLineF line2(from * width + offset, 3,          from * width + offset - 5, 3);
        QLineF line3(from * width + offset, height-4,   from * width + offset - 5, height-4);
        painter.drawLine(line1);
        painter.drawLine(line2);
        painter.drawLine(line3);

        float to = 0.1f;
        QLineF line4(to * width + offset, 3,          to * width + offset, height-4);
        QLineF line5(to * width + offset, 3,          to * width + offset + 5, 3);
        QLineF line6(to * width + offset, height-4,   to * width + offset + 5, height-4);
        painter.drawLine(line4);
        painter.drawLine(line5);
        painter.drawLine(line6);
    }

    QSlider::paintEvent(e);         // parent draws
}


// this function stores the absolute paths of each file in a QVector
void findFilesRecursively(QDir rootDir, QList<QString> *pathStack) {
    QDirIterator it(rootDir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
//    int i = 0;
    while(it.hasNext()) {
        QString s1 = it.next();
//        pathStack->push(s1);
        // If alias, follow it.
        QString resolvedFilePath = it.fileInfo().symLinkTarget(); // path with the symbolic links followed/removed
//        pathStack->append(s1);
        if (resolvedFilePath == "") {
            // If NOT alias, then use the original fileName
            resolvedFilePath = s1;
        }

        QFileInfo fi(s1);
        QStringList section = fi.canonicalPath().split("/");
        QString type = section[section.length()-1];  // must be the last item in the path, of where the alias is, not where the file is

//        qDebug() << "FFR: " << s1 << ";" << type + "#!#" + resolvedFilePath << ", type:" << type; // type is determined HERE
        pathStack->append(type + "#!#" + resolvedFilePath);

//        qDebug() << s1 << ";" << rootDir.filePath(s1);
//        QFile f(s1);
//        qDebug() << f.exists() << ";" << f.symLinkTarget() << ";" << it.filePath() << ";" << it.fileInfo().symLinkTarget();
    }
}

//QString musicRootPath("/Users/mpogue/___squareDanceMusic");

void MainWindow::findMusic()
{
    // always gets rid of the old one...
    if (pathStack) {
//        qDebug() << "deleting old pathStack...";
        delete pathStack;
    }
    pathStack = new QList<QString>();

//    qDebug() << "findMusic: rootPath = " << musicRootPath;

    QDir musicRootDir(musicRootPath);
    musicRootDir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDot | QDir::NoDotDot); // FIX: temporarily don't allow symlinks  | QDir::NoSymLinks

    QStringList qsl;
    qsl.append("*.mp3");                // I only want MP3 files
    qsl.append("*.wav");                //          or WAV files
    musicRootDir.setNameFilters(qsl);

    // --------
    findFilesRecursively(musicRootDir, pathStack);
//    qDebug() << "pathStack size = " << pathStack->size();  // how many songs found
}

void MainWindow::filterMusic() {
//    qDebug() << "filtering song table";

    ui->songTable->setSortingEnabled(false);

    // clear out the table
    ui->songTable->setRowCount(0);

//    qDebug() << "table now zeroed out.";

    QStringList m_TableHeader;
    m_TableHeader<<"Label"<<"#"<<"Type" << "Title";
    ui->songTable->setHorizontalHeaderLabels(m_TableHeader);
    ui->songTable->horizontalHeader()->setVisible(true);

//    int itemCount = pathStack->size();
//    qDebug() << "filterMusic, itemCount = " << itemCount;

    QListIterator<QString> iter(*pathStack);

    while (iter.hasNext()) {
        QString s = iter.next();

        QStringList sl1 = s.split("#!#");
        QString type = sl1[0];  // the type (of original pathname, before following aliases)
        s = sl1[1];  // everything else
        QString origPath = s;  // for when we double click it later on...

//        qDebug() << "CHECK: " << type << ", s: " << s;

        QFileInfo fi(s);
//        qDebug() << "s: " << s;
//        qDebug() << "fi.baseName():" << fi.baseName();
//        qDebug() << "fi.canonicalPath():" << fi.canonicalPath();

//        qDebug() << "musicRootPath: " << musicRootPath;

        if (fi.canonicalPath() == musicRootPath) {
            // e.g. "/Users/mpogue/__squareDanceMusic/C 117 - Bad Puppy (Patter).mp3" --> NO TYPE PRESENT
            type = "";
        }

        QStringList section = fi.canonicalPath().split("/");
//        qDebug() << "length of section: " << section.length();
//        qDebug() << "last section: " << section[section.length()-1];  // FIX: this will not work for aliases.

        QString label = "";
        QString labelnum = "";
        QString title = "";

        s = fi.baseName(); // e.g. "/Users/mpogue/__squareDanceMusic/patter/RIV 307 - Going to Ceili (Patter).mp3" --> "RIV 307 - Going to Ceili (Patter)"
        QRegularExpression re_square("^(.+) - (.+)$");
        QRegularExpressionMatch match_square = re_square.match(s);
        if (match_square.hasMatch()) {
            label = match_square.captured(1);   // label == "RIV 307"
            title = match_square.captured(2);   // title == "Going to Ceili (Patter)"

            QRegularExpression re_labelAndLabelnum("([a-zA-Z]+) *([a-zA-Z0-9]+)");
            QRegularExpressionMatch match_labelAndLabelnum = re_labelAndLabelnum.match(label);
            if (match_labelAndLabelnum.hasMatch()) {
                label = match_labelAndLabelnum.captured(1);        // label = "RIV"
                labelnum = match_labelAndLabelnum.captured(2);     // labelNum = "307"
            } else {
//                    qDebug() << "FAILED TO SPLIT LABEL AND LABEL NUMBER: '" << label << "'";
            }
        } else {
            // e.g. /Users/mpogue/__squareDanceMusic/xtras/Virginia Reel.mp3
//            qDebug() << "FAILED TO MATCH LABEL LABELNUM - TITLE : '" << s << "'";
            title = s;
        }

//        qDebug() << "type: " << type << ", label: " << label << ", labelnum: " << labelnum << ", title: " << title;

        ui->songTable->setRowCount(ui->songTable->rowCount()+1);  // make one more row for this line
//        qDebug() << "now has " << ui->songTable->rowCount() << " rows.";

        QColor textCol = QColor::fromRgbF(0.0/255.0, 0.0/255.0, 0.0/255.0);  // defaults to Black
        if (type == "xtras") {
            textCol = (QColor::fromRgbF(156.0/255.0, 31.0/255.0, 0.0/255.0)); // other: dark red
        } else if (type == "patter") {
            textCol = (QColor::fromRgbF(121.0/255.0, 99.0/255.0, 255.0/255.0)); // patter: Purple
        } else if (type == "singing") {
            textCol = (QColor::fromRgbF(0.0/255.0, 175.0/255.0, 92.0/255.0)); // singing: dark green
        } else if (type == "singing_called") {
            textCol = (QColor::fromRgbF(171.0/255.0, 105.0/255.0, 0.0/255.0)); // singing: dark green
        }

        QTableWidgetItem *newTableItem0 = new QTableWidgetItem( label );
        newTableItem0->setFlags(newTableItem0->flags() & ~Qt::ItemIsEditable);      // not editable
        newTableItem0->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, 0, newTableItem0);      // add it to column 0

        QTableWidgetItem *newTableItem1 = new QTableWidgetItem( labelnum );
        newTableItem1->setFlags(newTableItem1->flags() & ~Qt::ItemIsEditable);      // not editable
        newTableItem1->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, 1, newTableItem1);      // add it to column 1

        QTableWidgetItem *newTableItem2 = new QTableWidgetItem( type );
        newTableItem2->setFlags(newTableItem2->flags() & ~Qt::ItemIsEditable);      // not editable
        newTableItem2->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, 2, newTableItem2);      // add it to column 2

        QTableWidgetItem *newTableItem3 = new QTableWidgetItem( title );
        newTableItem3->setFlags(newTableItem3->flags() & ~Qt::ItemIsEditable);      // not editable
        newTableItem3->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, 3, newTableItem3);      // add it to column 3

//        qDebug() << "rowcount = " << ui->songTable->rowCount() << "origPath=" << origPath;
        // keep the path around, for loading in when we double click on it
        ui->songTable->item(ui->songTable->rowCount()-1, 0)->setData(Qt::UserRole, QVariant(origPath)); // path set on cell in col 0

        // Filter out (hide) rows that we're not interested in, based on the search fields...
        //   4 if statements is clearer than a gigantic single if....
        if (ui->labelSearch->toPlainText() != "" && !label.contains(QString(ui->labelSearch->toPlainText()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }

        if (ui->labelNumberSearch->toPlainText() != "" && !labelnum.contains(QString(ui->labelNumberSearch->toPlainText()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }

        if (ui->typeSearch->toPlainText() != "" && !type.contains(QString(ui->typeSearch->toPlainText()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }

        if (ui->titleSearch->toPlainText() != "" && !title.contains(QString(ui->titleSearch->toPlainText()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }

    }

    ui->songTable->sortItems(3);  // sort third by song title
    ui->songTable->sortItems(1);  // sort third by label number
    ui->songTable->sortItems(0);  // sort second by label
    ui->songTable->sortItems(2);  // sort first by type (singing vs patter)

    ui->songTable->setSortingEnabled(true);

    QString msg1 = QString::number(ui->songTable->rowCount()) + QString(" audio files found.");
    ui->statusBar->showMessage(msg1);
}

void MainWindow::on_labelSearch_textChanged()
{
    filterMusic();
}

void MainWindow::on_labelNumberSearch_textChanged()
{
    filterMusic();
}

void MainWindow::on_typeSearch_textChanged()
{
    filterMusic();
}

void MainWindow::on_titleSearch_textChanged()
{
    filterMusic();
}

void MainWindow::on_songTable_itemDoubleClicked(QTableWidgetItem *item)
{
    on_stopButton_clicked();  // if we're loading a new MP3 file, stop current playback

    int row = item->row();
    QString pathToMP3 = ui->songTable->item(row,0)->data(Qt::UserRole).toString();
//    qDebug() << "Path: " << pathToMP3;

    QString songTitle = ui->songTable->item(row,3)->text();
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,2)->text();
//    qDebug() << "Song type:" << songType;

    loadMP3File(pathToMP3, songTitle, songType);

    if (ui->actionAutostart_playback->isChecked()) {
        on_playButton_clicked();
    }
}

void MainWindow::on_actionClear_Search_triggered()
{
    on_clearSearchButton_clicked();
}

void MainWindow::on_actionPitch_Up_triggered()
{
    ui->pitchSlider->setValue(ui->pitchSlider->value() + 1);
}

void MainWindow::on_actionPitch_Down_triggered()
{
    ui->pitchSlider->setValue(ui->pitchSlider->value() - 1);
}

void MainWindow::on_actionAutostart_playback_triggered()
{
    // the Autostart on Playback mode setting is persistent across restarts of the application
    QSettings MySettings; // Will be using application information for correct location of your settings
    const QString AUTOSTART_KEY("autostartplayback");  // default is AUTOSTART ENABLED

//    QString autoStartChecked = MySettings.value(AUTOSTART_KEY).toString();
    if (ui->actionAutostart_playback->isChecked()) {
        MySettings.setValue(AUTOSTART_KEY, "checked");
//        qDebug() << "Setting autostartplayback to CHECKED";
    } else {
        MySettings.setValue(AUTOSTART_KEY, "unchecked");
//        qDebug() << "Setting autostartplayback to UNCHECKED";
    }
}

// --------------------------------------------------------
void MainWindow::on_actionPreferences_triggered()
{
    inPreferencesDialog = true;
    on_stopButton_clicked();  // stop music, if it was playing...

    PreferencesDialog *dialog = new PreferencesDialog;

    // modal dialog
    int dialogCode = dialog->exec();

    // act on dialog return code
    if(dialogCode == QDialog::Accepted)
    {
        // OK clicked
        // Save the new value
        QSettings MySettings;
        MySettings.setValue("musicPath", dialog->musicPath); // fish out the new dir from the Preferences dialog, and save it

        if (dialog->musicPath != musicRootPath) { // path has changed!
//            qDebug() << "Path has changed!  Finding music again...";
//            qDebug() << "new path is:" << dialog->musicPath;
            musicRootPath = dialog->musicPath;
            findMusic();
            filterMusic();
        }

    } else {
        qDebug() << "You cancelled changes to Preferences....";
    }

    inPreferencesDialog = false;
}
