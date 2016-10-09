#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utility.h"
#include "tablenumberitem.h"
#include "QMap"
#include "QMapIterator"
#include "QThread"

// BUG: Cmd-K highlights the next row, and hangs the app
// BUG: searching then clearing search will lose selection in songTable
// BUG: NL allowed in the search fields, makes the text disappear until DEL pressed

// =================================================================================================
// SquareDeskPlayer Keyboard Shortcuts:
//
// function                 MAC                  		PC                                SqView
// -------------------------------------------------------------------------------------------------
// FILE MENU
// Open Audio file          Cmd-O                		Ctrl-O, Alt-F-O
// Save                     Cmd-S                		Ctrl-S, Alt-F-S
// Save as                  Shft-Cmd-S           		Alt-F-A
// Quit                     Cmd-Q                		Ctrl-F4, Alt-F-E
//
// PLAYLIST MENU
// Load Playlist
// Save Playlist
// Next Song                K                            K                                K
// Prev Song
//
// MUSIC MENU
// play/pause               space                		space, Alt-M-P                    space
// rewind/stop              S, ESC, END, Cmd-.   		S, ESC, END, Alt-M-S, Ctrl-.
// rewind/play (playing)    HOME, . (while playing) 	HOME, .  (while playing)          .
// skip/back 5 sec          Cmd-RIGHT/LEFT,RIGHT/LEFT   Ctrl-RIGHT/LEFT, RIGHT/LEFT,
//                                                        Alt-M-A/Alt-M-B
// volume up/down           Cmd-UP/DOWN,UP/DOWN         Ctrl-UP/DOWN, UP/DOWN             N/B
// mute                     Cmd-M, M                	Ctrl-M, M
// go faster                Cmd-+,+,=            		Ctrl-+,+,=                        R
// go slower                Cmd--,-              		Ctrl--,-                          E
// force mono                                    		Alt-M-F
// clear search             Cmd-/                		Alt-M-S
// pitch up                 Cmd-U, U                	Ctrl-U, U, Alt-M-U                F
// pitch down               Cmd-D, D                	Ctrl-D, D, Alt-M-D                D

// GLOBALS:
bass_audio cBass;

// ----------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->statusBar->showMessage("");

    this->setWindowTitle(QString("SquareDesk Music Player/Editor"));

    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

    ui->playButton->setEnabled(false);
    ui->stopButton->setEnabled(false);

    ui->nextSongButton->setEnabled(false);
    ui->previousSongButton->setEnabled(false);

    // ============
    ui->menuFile->addSeparator();

    // ------------
    // NOTE: MAC OS X ONLY
#if defined(Q_OS_MAC)
    QAction *aboutAct = new QAction(QIcon(), tr("&About SquareDesk..."), this);
    aboutAct->setStatusTip(tr("SquareDesk Information"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(aboutBox()));
    ui->menuFile->addAction(aboutAct);
#endif

    // ==============
    // HELP MENU IS WINDOWS ONLY
#if defined(Q_OS_WIN)
    QMenu *helpMenu = new QMenu("&Help");

    // ------------
    QAction *aboutAct2 = new QAction(QIcon(), tr("About &SquareDesk..."), this);
    aboutAct2->setStatusTip(tr("SquareDesk Information"));
    connect(aboutAct2, SIGNAL(triggered()), this, SLOT(aboutBox()));
    helpMenu->addAction(aboutAct2);
    menuBar()->addAction(helpMenu->menuAction());
#endif

#if defined(Q_OS_WIN)
    delete ui->mainToolBar; // remove toolbar on WINDOWS (not present on Mac)
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
    tempoIsBPM = false;

    notSorted = true;

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

    // used to store the file paths
    findMusic();  // get the filenames from the user's directories
    filterMusic(); // and filter them into the songTable

    ui->songTable->setColumnWidth(0,36);
    ui->songTable->setColumnWidth(1,96);
    ui->songTable->setColumnWidth(2,80);

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
    const QString FORCEMONO_KEY("forcemono");  // default is FALSE (use stereo)
    QString forceMonoChecked = MySettings.value(FORCEMONO_KEY).toString();

    if (forceMonoChecked.isNull()) {
        // first time through, FORCE MONO is FALSE (stereo mode is the default)
        forceMonoChecked = "false";  // FIX: needed?
        MySettings.setValue(FORCEMONO_KEY, "false");
    }

    if (forceMonoChecked == "true") {
        ui->monoButton->setChecked(true);
        on_monoButton_toggled(true);  // sets button and menu item
    } else {
        ui->monoButton->setChecked(false);
        on_monoButton_toggled(false);  // sets button and menu item
    }

//    // -------
//    // FIX: This is partial code for changing the font size dynamically...
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

    // Volume, Pitch, and Mix can be set before loading a music file.
    ui->pitchSlider->setEnabled(true);
    ui->pitchSlider->setValue(0);
    ui->currentPitchLabel->setText("0 semitones");

    ui->volumeSlider->setEnabled(true);
    ui->volumeSlider->setValue(100);
    ui->currentVolumeLabel->setText("Max");

    ui->mixSlider->setEnabled(true);
    ui->mixSlider->setValue(0);
    ui->currentMixLabel->setText("50% L / 50% R");

    // ...and the EQ sliders, too...
    ui->bassSlider->setEnabled(true);
    ui->midrangeSlider->setEnabled(true);
    ui->trebleSlider->setEnabled(true);

    // in the Designer, these have values, making it easy to visualize there
    //   must clear those out, because a song is not loaded yet.
    ui->currentLocLabel->setText("");
    ui->songLengthLabel->setText("");

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

    // the Force Mono (Aahz Mode) setting is persistent across restarts of the application
    QSettings MySettings; // Will be using application information for correct location of your settings
    const QString FORCEMONO_KEY("forcemono");  // default is AUTOSTART ENABLED

    if (ui->actionForce_Mono_Aahz_mode->isChecked()) {
        MySettings.setValue(FORCEMONO_KEY, "true");
    } else {
        MySettings.setValue(FORCEMONO_KEY, "false");
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
    Info_Seekbar(false);  // update just the text
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
    if (tempoIsBPM) {
        float baseBPM = (float)cBass.Stream_BPM;    // original detected BPM
        float desiredBPM = (float)value;            // desired BPM
        int newBASStempo = (int)(round(100.0*desiredBPM/baseBPM));
        cBass.SetTempo(newBASStempo);
        ui->currentTempoLabel->setText(QString::number(value) + " BPM (" + QString::number(newBASStempo) + "%)");
    } else {
        float basePercent = 100.0;                      // original detected percent
        float desiredPercent = (float)value;            // desired percent
        int newBASStempo = (int)(round(100.0*desiredPercent/basePercent));
        cBass.SetTempo(newBASStempo);
        ui->currentTempoLabel->setText(QString::number(value) + "%");
    }
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
QString MainWindow::position2String(int position, bool pad = false)
{
    int songMin = position/60;
    int songSec = position - 60*songMin;
    QString songSecString = QString("%1").arg(songSec, 2, 10, QChar('0')); // pad with zeros
    QString s(QString::number(songMin) + ":" + songSecString);

    // pad on the left with zeros, if needed to prevent numbers shifting left and right
    if (pad) {
        // NOTE: don't use more than 7 chars total, or Possum Sop (long) will cause weird
        //   shift left/right effects when the slider moves.
        switch (s.length()) {
        case 4: s = "   " + s; // 4 + 3 = 7 chars
            break;
        case 5: s = "  " + s;  // 5 + 2 = 7 chars
            break;
        default:
            break;
        }
    }

    return s;
}

// ----------------------------------------------------------------------
void MainWindow::Info_Seekbar(bool forceSlider)
{
    static bool in_Info_Seekbar = false;
    if (in_Info_Seekbar)
        return;
    RecursionGuard recursion_guard(in_Info_Seekbar);

    if (songLoaded) {  // FIX: this needs to pay attention to the bool
        // FIX: this code doesn't need to be executed so many times.
        ui->seekBar->setMinimum(0);
        ui->seekBar->setMaximum((int)cBass.FileLength-1); // NOTE: tricky, counts on == below
        ui->seekBar->setTickInterval(10);  // 10 seconds per tick
        cBass.StreamGetPosition();  // update cBass.Current_Position

        int currentPos_i = (int)cBass.Current_Position;
        if (forceSlider) {
            ui->seekBar->blockSignals(true); // setValue should NOT initiate a valueChanged()
            ui->seekBar->setValue(currentPos_i);
            ui->seekBar->blockSignals(false);
        }
        int fileLen_i = (int)cBass.FileLength;

        if (currentPos_i == fileLen_i) {  // NOTE: tricky, counts on -1 above
            // avoids the problem of manual seek to max slider value causing auto-STOP
//            qDebug() << "Reached the end of playback!";
            on_stopButton_clicked(); // pretend we pressed the STOP button when EOS is reached
            return;
        }

        ui->currentLocLabel->setText(position2String(currentPos_i, true));  // pad on the left
        ui->songLengthLabel->setText("/ " + position2String(fileLen_i));    // no padding
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
    Info_Seekbar(false);
}

// ----------------------------------------------------------------------
void MainWindow::on_clearSearchButton_clicked()
{
    // figure out which row is currently selected
    QItemSelectionModel* selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } else {
        // more than 1 row or no rows at all selected (BAD)
//        qDebug() << "nothing selected.";
    }

    ui->labelSearch->setPlainText("");
    ui->typeSearch->setPlainText("");
    ui->titleSearch->setPlainText("");

    if (row != -1) {
        // if a row was selected, restore it after a clear search
        // FIX: this works much of the time, but it doesn't handle the case where search field is typed, then cleared.  In this case,
        //   the row isn't highlighted again.
        ui->songTable->selectRow(row);
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_actionLoop_triggered()
{
    on_loopButton_toggled(ui->actionLoop->isChecked());
    ui->loopButton->setChecked(ui->actionLoop->isChecked());
}

// ----------------------------------------------------------------------
void MainWindow::on_UIUpdateTimerTick(void)
{
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
    msgBox.setText(QString("<p><h2>SquareDesk Player, V0.4.5</h2>") +
                   QString("<p>See our website at <a href=\"http://squaredesk.net\">squaredesk.net</a></p>") +
                   QString("Uses: <a href=\"http://www.un4seen.com/bass.html\">libbass</a> and ") +
                   QString("<a href=\"http://www.jobnik.org/?mnu=bass_fx\">libbass_fx</a>") +
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

        if (!(ui->labelSearch->hasFocus() || // ui->labelNumberSearch->hasFocus() ||
                ui->typeSearch->hasFocus() || ui->titleSearch->hasFocus() || ui->songTable->isEditing())) {
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
//    qDebug() << "MainWindow::handleKeypress(), key =" << key << ", isPreferencesDialog =" << inPreferencesDialog;
    if (inPreferencesDialog) {
        return;
    }

    switch (key) {

        case Qt::Key_Escape:
        case Qt::Key_End:  // FIX: should END go to the end of the song? or stop playback?
        case Qt::Key_S:
            on_stopButton_clicked();
            break;

        case Qt::Key_P:
        case Qt::Key_Space:  // for SqView compatibility ("play/pause")
                             // if Stopped, PLAY;  if Playing, Pause.  If Paused, Resume.
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

        case Qt::Key_K:
            on_actionNext_Playlist_Item_triggered();  // compatible with SqView!
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
    cBass.StreamGetPosition();  // update the position
    // set the position to one second before the end, so that RIGHT ARROW works as expected
    cBass.StreamSetPosition((int)fmin(cBass.Current_Position + 15.0, cBass.FileLength-1.0));
    Info_Seekbar(true);
}

void MainWindow::on_actionSkip_Back_15_sec_triggered()
{
    Info_Seekbar(true);
    cBass.StreamGetPosition();  // update the position
    cBass.StreamSetPosition((int)fmax(cBass.Current_Position - 15.0, 0.0));
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
    cBass.SetEq(0, (float)value);
}

void MainWindow::on_midrangeSlider_valueChanged(int value)
{
    cBass.SetEq(1, (float)value);
}

void MainWindow::on_trebleSlider_valueChanged(int value)
{
    cBass.SetEq(2, (float)value);
}

void MainWindow::loadMP3File(QString MP3FileName, QString songTitle, QString songType) {
    QStringList pieces = MP3FileName.split( "/" );
    QString filebase = pieces.value(pieces.length()-1);
    QStringList pieces2 = filebase.split(".");

    currentMP3filename = pieces2.value(pieces2.length()-2);

    if (songTitle != "") {
        ui->nowPlayingLabel->setText(songTitle);
    } else {
        ui->nowPlayingLabel->setText(currentMP3filename);  // FIX?  convert to short version?
    }

    QDir md(MP3FileName);
    QString canonicalFN = md.canonicalPath();

    cBass.StreamCreate(MP3FileName.toStdString().c_str());

    QStringList ss = MP3FileName.split('/');
    QString fn = ss.at(ss.size()-1);
    this->setWindowTitle(fn + QString(" - SquareDesk MP3 Player/Editor"));

    int length_sec = cBass.FileLength;
    int songBPM = round(cBass.Stream_BPM);

    // Intentionally compare against a narrower range here than BPM detection, because BPM detection
    //   returns a number at the limits, when it's actually out of range.
    // Also, turn off BPM for xtras (they are all over the place, including round dance cues, which have no BPM at all).
    //
    // TODO: make the types for turning off BPM detection a preference
    if ((songBPM>=125-10) && (songBPM<=125+10) && songType != "xtras") {
        tempoIsBPM = true;
        ui->currentTempoLabel->setText(QString::number(songBPM) + " BPM (100%)"); // initial load always at 100%

        ui->tempoSlider->setMinimum(songBPM-15);
        ui->tempoSlider->setMaximum(songBPM+15);
        ui->tempoSlider->setValue(songBPM);
        ui->tempoSlider->setEnabled(true);
        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
                                 ", base tempo: " + QString::number(songBPM) + " BPM");
    }
    else {
        tempoIsBPM = false;
        // if we can't figure out a BPM, then use percent as a fallback (centered: 100%, range: +/-20%)
        ui->currentTempoLabel->setText("100%");
        ui->tempoSlider->setMinimum(100-20);        // allow +/-20%
        ui->tempoSlider->setMaximum(100+20);
        ui->tempoSlider->setValue(100);
        ui->tempoSlider->setEnabled(true);
        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
                                 ", base tempo: 100%");
    }

    fileModified = false;

    ui->playButton->setEnabled(true);
    ui->stopButton->setEnabled(true);

    ui->actionPlay->setEnabled(true);
    ui->actionStop->setEnabled(true);
    ui->actionSkip_Ahead_15_sec->setEnabled(true);
    ui->actionSkip_Back_15_sec->setEnabled(true);

    ui->seekBar->setEnabled(true);

    ui->pitchSlider->valueChanged(ui->pitchSlider->value()); // force pitch change, if pitch slider preset before load
    ui->volumeSlider->valueChanged(ui->volumeSlider->value()); // force vol change, if vol slider preset before load
    ui->mixSlider->valueChanged(ui->mixSlider->value()); // force mix change, if mix slider preset before load

    ui->actionMute->setEnabled(true);
    ui->actionLoop->setEnabled(true);

    ui->actionVolume_Down->setEnabled(true);
    ui->actionVolume_Up->setEnabled(true);
    ui->actionSpeed_Up->setEnabled(true);
    ui->actionSlow_Down->setEnabled(true);
    ui->actionForce_Mono_Aahz_mode->setEnabled(true);
    ui->actionPitch_Down->setEnabled(true);
    ui->actionPitch_Up->setEnabled(true);

    ui->bassSlider->valueChanged(ui->bassSlider->value()); // force bass change, if bass slider preset before load
    ui->midrangeSlider->valueChanged(ui->midrangeSlider->value()); // force midrange change, if midrange slider preset before load
    ui->trebleSlider->valueChanged(ui->trebleSlider->value()); // force treble change, if treble slider preset before load

    ui->loopButton->setEnabled(true);
    ui->monoButton->setEnabled(true);

    cBass.Stop();

    songLoaded = true;
    Info_Seekbar(true);

    if (songType == "patter") {
        ui->loopButton->setChecked(true);
        on_loopButton_toggled(true); // default is to loop, if type is patter
    } else {
        // not patter, so Loop mode defaults to OFF
        ui->loopButton->setChecked(false);
        on_loopButton_toggled(false); // default is to loop, if type is patter
    }
}

void MainWindow::on_actionOpen_MP3_file_triggered()
{
    on_stopButton_clicked();  // if we're loading a new MP3 file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    const QString DEFAULT_DIR_KEY("default_dir");
    QSettings MySettings; // Will be using application informations for correct location of your settings


    QString startingDirectory = MySettings.value(DEFAULT_DIR_KEY).toString();
    if (startingDirectory.isNull()) {
        // first time through, start at HOME
        startingDirectory = QDir::homePath();
    }

    QString MP3FileName =
        QFileDialog::getOpenFileName(this,
                                     tr("Import Audio File"),
                                     startingDirectory,
                                     tr("Audio Files (*.mp3 *.wav)"));
    if (MP3FileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
    QDir CurrentDir;
    MySettings.setValue(DEFAULT_DIR_KEY, CurrentDir.absoluteFilePath(MP3FileName));

    ui->songTable->clearSelection();  // if loaded via menu, then clear previous selection (if present)
    ui->nextSongButton->setEnabled(false);  // and, next/previous song buttons are disabled
    ui->previousSongButton->setEnabled(false);

    // --------
//    qDebug() << "loading: " << MP3FileName;
    loadMP3File(MP3FileName, QString(""), QString(""));  // "" means use title from the filename
}

// ==========================================================================================
MySlider::MySlider(QWidget *parent) : QSlider(parent)
{
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
    while(it.hasNext()) {
        QString s1 = it.next();
        // If alias, follow it.
        QString resolvedFilePath = it.fileInfo().symLinkTarget(); // path with the symbolic links followed/removed
        if (resolvedFilePath == "") {
            // If NOT alias, then use the original fileName
            resolvedFilePath = s1;
        }

        QFileInfo fi(s1);
        QStringList section = fi.canonicalPath().split("/");
        QString type = section[section.length()-1];  // must be the last item in the path, of where the alias is, not where the file is

        pathStack->append(type + "#!#" + resolvedFilePath);
    }
}

void MainWindow::findMusic()
{
    // always gets rid of the old one...
    if (pathStack) {
        delete pathStack;
    }
    pathStack = new QList<QString>();

    QDir musicRootDir(musicRootPath);
    musicRootDir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDot | QDir::NoDotDot); // FIX: temporarily don't allow symlinks  | QDir::NoSymLinks

    QStringList qsl;
    qsl.append("*.mp3");                // I only want MP3 files
    qsl.append("*.wav");                //          or WAV files
    musicRootDir.setNameFilters(qsl);

    // --------
    findFilesRecursively(musicRootDir, pathStack);
}

void MainWindow::filterMusic() {
    ui->songTable->setSortingEnabled(false);

    // Need to remember the PL# mapping here, and reapply it after the filter
    // left = path, right = number string
    QMap<QString, QString> path2playlistNum;

    // Iterate over the songTable, saving the mapping in "path2playlistNum"
    // TODO: optimization: save this once, rather than recreating each time.
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,0);
        QString playlistIndex = theItem->text();  // this is the playlist #
        QString pathToMP3 = ui->songTable->item(i,1)->data(Qt::UserRole).toString();  // this is the full pathname
        if (playlistIndex != " " && playlistIndex != "") {
            // item HAS an index (that is, it is on the list, and has a place in the ordering)
//            qDebug() << "remembering playlistIndex:" << playlistIndex << ", origPath:" << pathToMP3;
            // TODO: reconcile int here with float elsewhere on insertion
            path2playlistNum[pathToMP3] = playlistIndex;
        }
    }

    // clear out the table
    ui->songTable->setRowCount(0);

    QStringList m_TableHeader;
    m_TableHeader<< "#" << "Type" << "Label" << "Title";
    ui->songTable->setHorizontalHeaderLabels(m_TableHeader);
    ui->songTable->horizontalHeader()->setVisible(true);

    QListIterator<QString> iter(*pathStack);

    while (iter.hasNext()) {
        QString s = iter.next();

        QStringList sl1 = s.split("#!#");
        QString type = sl1[0];  // the type (of original pathname, before following aliases)
        s = sl1[1];  // everything else
        QString origPath = s;  // for when we double click it later on...

        QFileInfo fi(s);

        if (fi.canonicalPath() == musicRootPath) {
            // e.g. "/Users/mpogue/__squareDanceMusic/C 117 - Bad Puppy (Patter).mp3" --> NO TYPE PRESENT
            type = "";
        }

        QStringList section = fi.canonicalPath().split("/");
        QString label = "";
        QString labelnum = "";
        QString title = "";

        s = fi.baseName(); // e.g. "/Users/mpogue/__squareDanceMusic/patter/RIV 307 - Going to Ceili (Patter).mp3" --> "RIV 307 - Going to Ceili (Patter)"
        QRegularExpression re_square("^(.+) - (.+)$");
        QRegularExpressionMatch match_square = re_square.match(s);
        if (match_square.hasMatch()) {
            label = match_square.captured(1);   // label == "RIV 307"
            title = match_square.captured(2);   // title == "Going to Ceili (Patter)"
        } else {
            // e.g. /Users/mpogue/__squareDanceMusic/xtras/Virginia Reel.mp3
            title = s;
        }

        ui->songTable->setRowCount(ui->songTable->rowCount()+1);  // make one more row for this line

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

        // look up origPath in the path2playlistNum map, and reset the s2 text to the user's playlist # setting (if any)
        QString s2("");
        if (path2playlistNum.contains(origPath)) {
            s2 = path2playlistNum[origPath];
        }
//        qDebug() << "origPath:" << origPath << ", s2:" << s2;

        TableNumberItem *newTableItem4 = new TableNumberItem(s2);

        newTableItem4->setTextAlignment(Qt::AlignCenter);                           // editable by default
        newTableItem4->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, 0, newTableItem4);      // add it to column 0

        QTableWidgetItem *newTableItem2 = new QTableWidgetItem( type );
        newTableItem2->setFlags(newTableItem2->flags() & ~Qt::ItemIsEditable);      // not editable
        newTableItem2->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, 1, newTableItem2);      // add it to column 2

        QTableWidgetItem *newTableItem0 = new QTableWidgetItem( label + " " + labelnum );
        newTableItem0->setFlags(newTableItem0->flags() & ~Qt::ItemIsEditable);      // not editable
        newTableItem0->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, 2, newTableItem0);      // add it to column 0

        QTableWidgetItem *newTableItem3 = new QTableWidgetItem( title );
        newTableItem3->setFlags(newTableItem3->flags() & ~Qt::ItemIsEditable);      // not editable
        newTableItem3->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, 3, newTableItem3);      // add it to column 3

        // keep the path around, for loading in when we double click on it
        ui->songTable->item(ui->songTable->rowCount()-1, 1)->setData(Qt::UserRole, QVariant(origPath)); // path set on cell in col 0

        // Filter out (hide) rows that we're not interested in, based on the search fields...
        //   4 if statements is clearer than a gigantic single if....
        QString labelPlusNumber = label + " " + labelnum;
        if (ui->labelSearch->toPlainText() != "" && !labelPlusNumber.contains(QString(ui->labelSearch->toPlainText()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }

        if (ui->typeSearch->toPlainText() != "" && !type.contains(QString(ui->typeSearch->toPlainText()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }

        if (ui->titleSearch->toPlainText() != "" && !title.contains(QString(ui->titleSearch->toPlainText()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }

    }

    if (notSorted) {
//        qDebug() << "SORTING FOR THE FIRST TIME";
        ui->songTable->sortItems(2);  // sort second by label/label #
        ui->songTable->sortItems(1);  // sort first by type (singing vs patter)

        notSorted = false;
    }

    ui->songTable->setSortingEnabled(true);

    QString msg1 = QString::number(ui->songTable->rowCount()) + QString(" audio files found.");
    ui->statusBar->showMessage(msg1);
}

void MainWindow::on_labelSearch_textChanged()
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
    QString pathToMP3 = ui->songTable->item(row,1)->data(Qt::UserRole).toString();

    QString songTitle = ui->songTable->item(row,3)->text();
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,1)->text();

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

    if (ui->actionAutostart_playback->isChecked()) {
        MySettings.setValue(AUTOSTART_KEY, "checked");
    } else {
        MySettings.setValue(AUTOSTART_KEY, "unchecked");
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
            musicRootPath = dialog->musicPath;
            findMusic();
            filterMusic();
        }

    }

    inPreferencesDialog = false;
}

QString MainWindow::removePrefix(QString prefix, QString s) {
    QString s2 = s.remove( prefix );
//    qDebug() << "prefix:" << prefix << ", s:" << s << ", s2:" << s2;
    return s2;
}

// PLAYLIST MANAGEMENT ===============================================
// TODO: prepend root path here
void MainWindow::on_actionLoad_Playlist_triggered()
{
    on_stopButton_clicked();  // if we're loading a new PLAYLIST file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");
    QSettings MySettings; // Will be using application informations for correct location of your settings
    QString musicRootPath = MySettings.value("musicPath").toString();

    QString startingPlaylistDirectory = MySettings.value(DEFAULT_PLAYLIST_DIR_KEY).toString();
    if (startingPlaylistDirectory.isNull()) {
        // first time through, start at HOME
        startingPlaylistDirectory = QDir::homePath();
    }

    QString PlaylistFileName =
        QFileDialog::getOpenFileName(this,
                                     tr("Load Playlist"),
                                     startingPlaylistDirectory,
                                     tr("Playlist Files (*.m3u)"));
    if (PlaylistFileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
    QDir CurrentDir;
    MySettings.setValue(DEFAULT_PLAYLIST_DIR_KEY, CurrentDir.absoluteFilePath(PlaylistFileName));

    // --------
    int lineCount = 1;
    int songCount = 0;
    QString firstBadSongLine = "";

    QFile inputFile(PlaylistFileName);
    if (inputFile.open(QIODevice::ReadOnly))  // defaults to Text mode
    {
        // first, clear all the playlist numbers that are there now.
        for (int i = 0; i < ui->songTable->rowCount(); i++) {
            QString pathToMP3 = ui->songTable->item(i,1)->data(Qt::UserRole).toString();
                QTableWidgetItem *theItem = ui->songTable->item(i,0);
                theItem->setText("");
        }

        QTextStream in(&inputFile);
       while (!in.atEnd())
       {
          QString line = in.readLine();
//          qDebug() << "line:" << line;

          if (line == "#EXTM3U") {
              // ignore, it's the first line of the M3U file
          } else if (line == "") {
              // ignore, it's a blank line
          } else if (line.at( 0 ) == '#' ) {
              // it's a comment line
              if (line.mid(0,7) == "#EXTINF") {
                  // it's information about the next line, ignore for now.
              }
          } else {
              songCount++;  // it's a real song path
//              qDebug() << "SONG #" << songCount << "SONG PATH:" << line;

              bool match = false;
              // exit the loop early, if we find a match
              for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {
                  QString pathToMP3 = ui->songTable->item(i,1)->data(Qt::UserRole).toString();
                  if (line == pathToMP3) { // FIX: this is fragile, if songs are moved around
                      QTableWidgetItem *theItem = ui->songTable->item(i,0);
                      theItem->setText(QString::number(songCount));
                      match = true;
                  }
              }
              // if we had no match, remember the first non-matching song path
              if (!match && firstBadSongLine == "") {
                  firstBadSongLine = line;
              }

          }

          lineCount++;
       }
       inputFile.close();
    }

    ui->songTable->sortItems(2);  // sort by title as last
    ui->songTable->sortItems(1);  // sort by label/label# as secondary
    ui->songTable->sortItems(0);  // sort by playlist # as primary
    notSorted = false;

    // select the very first row, and trigger a GO TO PREVIOUS, which will load row 0 (and start it, if autoplay is ON).
    // only do this, if there were no errors in loading the playlist numbers.
    if (firstBadSongLine == "") {
        ui->songTable->selectRow(0); // select first row of newly loaded and sorted playlist!
        on_actionPrevious_Playlist_Item_triggered();
    }

    QString msg1 = QString("Loaded playlist with ") + QString::number(songCount) + QString(" items.");
    if (firstBadSongLine != "") {
        // if there was a non-matching path, tell the user what the first one of those was
        msg1 = QString("ERROR: could not find '") + firstBadSongLine + QString("'");
        ui->songTable->clearSelection(); // select nothing, if error
    }
    ui->statusBar->showMessage(msg1);

}

// TODO: strip off the root directory before saving...
void MainWindow::on_actionSave_Playlist_triggered()
{
    on_stopButton_clicked();  // if we're saving a new PLAYLIST file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");
    QSettings MySettings; // Will be using application informations for correct location of your settings

    QString startingPlaylistDirectory = MySettings.value(DEFAULT_PLAYLIST_DIR_KEY).toString();
    if (startingPlaylistDirectory.isNull()) {
        // first time through, start at HOME
        startingPlaylistDirectory = QDir::homePath();
    }

    QString PlaylistFileName =
        QFileDialog::getSaveFileName(this,
                                     tr("Save Playlist"),
                                     startingPlaylistDirectory,
                                     tr("Playlist Files (*.m3u)"));
    if (PlaylistFileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
    QDir CurrentDir;
    MySettings.setValue(DEFAULT_PLAYLIST_DIR_KEY, CurrentDir.absoluteFilePath(PlaylistFileName));

    // --------
//    qDebug() << "TODO: saving playlist: " << PlaylistFileName;

    QMap<int, QString> imports;

    // TODO: iterate over the songTable
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,0);
        QString playlistIndex = theItem->text();
        QString pathToMP3 = ui->songTable->item(i,1)->data(Qt::UserRole).toString();
        QString songTitle = ui->songTable->item(i,3)->text();
        if (playlistIndex != " ") {
            // item HAS an index (that is, it is on the list, and has a place in the ordering)
//            qDebug() << "playlistIndex:" << playlistIndex << ", MP3:" << pathToMP3 << ", Title:" << songTitle;
            // TODO: reconcile int here with float elsewhere on insertion
            imports[playlistIndex.toInt()] = pathToMP3;
        }
    }

    // TODO: strip the initial part of the path off the Paths, e.g.
    //   /Users/mpogue/__squareDanceMusic/patter/C 117 - Restless Romp (Patter).mp3
    //   becomes
    //   patter/C 117 - Restless Romp (Patter).mp3
    //
    //   So, the remaining path is relative to the root music directory.
    //   When loading, first look at the patter and the rest
    //     if no match, try looking at the rest only
    //     if no match, then error (dialog?)
    //   Then on Save Playlist, write out the NEW patter and the rest

    // TODO: get rid of the single space, replace with nothing

    QFile file(PlaylistFileName);
    if (file.open(QIODevice::ReadWrite)) {
        QTextStream stream(&file);
        stream << "#EXTM3U" << endl << endl;

        // list is auto-sorted here
        QMapIterator<int, QString> i(imports);
        while (i.hasNext()) {
            i.next();
//            qDebug() << i.key() << ": " << i.value();
            stream << "#EXTINF:-1," << endl;  // nothing after the comma = no special name
            stream << i.value() << endl;
        }
        file.close();
    }

    // TODO: if there are no songs specified in the playlist (yet, because not edited, or yet, because
    //   no playlist was loaded), Save Playlist... should be greyed out.

    ui->statusBar->showMessage(QString("Playlist items saved."));
}

void MainWindow::on_actionNext_Playlist_Item_triggered()
{
    // This code is similar to the row double clicked code...
    on_stopButton_clicked();  // if we're going to the next file in the playlist, stop current playback

    // figure out which row is currently selected
    QItemSelectionModel* selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } else {
        // more than 1 row or no rows at all selected (BAD)
//        qDebug() << "nothing selected.";
        return;
    }

    int maxRow = ui->songTable->rowCount() - 1;
    row = (maxRow < row+1 ? maxRow : row+1); // bump up by 1
    ui->songTable->selectRow(row); // select new row!

    // load all the UI fields, as if we double-clicked on the new row
    QString pathToMP3 = ui->songTable->item(row,1)->data(Qt::UserRole).toString();
    QString songTitle = ui->songTable->item(row,3)->text();
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,1)->text();

    loadMP3File(pathToMP3, songTitle, songType);

    if (ui->actionAutostart_playback->isChecked()) {
        on_playButton_clicked();
    }

    // TODO: Continuous Play mode to allow playing through an entire playlist without stopping (do not play blank playIndex files).
    //       Continuous Play should be disabled and OFF, unless a playlist is loaded.
}

void MainWindow::on_actionPrevious_Playlist_Item_triggered()
{
    // This code is similar to the row double clicked code...
    on_stopButton_clicked();  // if we're going to the next file in the playlist, stop current playback

    QItemSelectionModel* selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } else {
        // more than 1 row or no rows at all selected (BAD)
        return;
    }

//    int rowCount = ui->songTable->rowCount();
    row = (row-1 < 0 ? 0 : row-1); // bump backwards by 1
    ui->songTable->selectRow(row); // select new row!

    // load all the UI fields, as if we double-clicked on the new row
    QString pathToMP3 = ui->songTable->item(row,1)->data(Qt::UserRole).toString();
    QString songTitle = ui->songTable->item(row,3)->text();
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,1)->text();

    loadMP3File(pathToMP3, songTitle, songType);

    if (ui->actionAutostart_playback->isChecked()) {
        on_playButton_clicked();
    }
}

void MainWindow::on_previousSongButton_clicked()
{
    on_actionPrevious_Playlist_Item_triggered();
}

void MainWindow::on_nextSongButton_clicked()
{
    on_actionNext_Playlist_Item_triggered();
}

void MainWindow::on_songTable_itemSelectionChanged()
{
    // When item selection is changed, enable Next/Previous song buttons,
    //   if at least one item in the table is selected.
    //
    // figure out which row is currently selected
    QItemSelectionModel* selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
//    qDebug() << "songTable:itemSelectionChanged(), selected.count(): " << selected.count();
    if (selected.count() == 1) {
        ui->nextSongButton->setEnabled(true);
        ui->previousSongButton->setEnabled(true);
    } else {
        ui->nextSongButton->setEnabled(false);
        ui->previousSongButton->setEnabled(false);
    }
}
