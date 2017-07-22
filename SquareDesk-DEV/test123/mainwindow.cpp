/****************************************************************************
**
** Copyright (C) 2016, 2017 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk/SquareDeskPlayer application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usages
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

#include <QActionGroup>
#include <QColorDialog>
#include <QCoreApplication>
#include <QDesktopWidget>
#include <QElapsedTimer>
#include <QMap>
#include <QMapIterator>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressDialog>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QThread>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QWidget>

#include <QPrinter>
#include <QPrintDialog>

#include "analogclock.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utility.h"
#include "tablenumberitem.h"
#include "prefsmanager.h"
#include "importdialog.h"
#include "exportdialog.h"

#include "danceprograms.h"
#define CUSTOM_FILTER
#include "startupwizard.h"
#include "downloadmanager.h"

#if defined(Q_OS_MAC)
#include "JlCompress.h"
#endif

// BUG: Cmd-K highlights the next row, and hangs the app
// BUG: searching then clearing search will lose selection in songTable
// TODO: consider selecting the row in the songTable, if there is only one row valid as the result of a search
//   then, ENTER could select it, maybe?  Think about this.
// BUG: if you're playing a song on an external flash drive, and remove it, playback stops, but the song is still
//   in the currenttitlebar, and it tries to play (silently).  Should clear everything out at that point and unload the song.

// BUG: should allow Ctrl-U in the sd window, to clear the line (equiv to "erase that")

// TODO: include a license in the executable bundle for Mac (e.g. GPL2).  Include the same
//   license next to the Win32 executable (e.g. COPYING).

// REMINDER TO FUTURE SELF: (I forget how to do this every single time) -- to set a layout to fill a single tab:
//    In designer, you should first in form preview select requested tab,
//    than in tree-view click to PARENT QTabWidget and set the layout as for all tabs.
//    Really this layout appears as new properties for selected tab only. Every tab has own layout.
//    And, the tab must have at least one widget on it already.

#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

// TAGLIB stuff is MAC OS X and WIN32 only for now...
#include <taglib/tlist.h>
#include <taglib/fileref.h>
#include <taglib/tfile.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/id3v2header.h>

#include <taglib/unsynchronizedlyricsframe.h>
#include <string>

#include "typetracker.h"

#if defined(Q_OS_MAC) | defined(Q_OS_WIN32)
#define POCKETSPHINXSUPPORT 1
#endif

using namespace TagLib;

// =================================================================================================
// SquareDeskPlayer Keyboard Shortcuts:
//
// function                 MAC                         PC                                SqView
// -------------------------------------------------------------------------------------------------
// FILE MENU
// Open Audio file          Cmd-O                       Ctrl-O, Alt-F-O
// Save                     Cmd-S                       Ctrl-S, Alt-F-S
// Save as                  Shft-Cmd-S                  Alt-F-A
// Quit                     Cmd-Q                       Ctrl-F4, Alt-F-E
//
// PLAYLIST MENU
// Load Playlist            Cmd-E
// Load Recent Playlist
// Save Playlist
// Add to Playlist TOP      Shft-Cmd-LeftArrow
// Add to Playlist BOT      Shft-Cmd-RightArrow
// Move Song UP             Shft-Cmd-UpArrow
// Move Song DOWN           Shft-Cmd-DownArrow
// Remove from Playlist     Shft-Cmd-DEL
// Next Song                K                            K                                K
// Prev Song
// Continuous Play
//
// MUSIC MENU
// play/pause               space                       space, Alt-M-P                    space
// rewind/stop              S, ESC, END, Cmd-.          S, ESC, END, Alt-M-S, Ctrl-.
// rewind/play (playing)    HOME, . (while playing)     HOME, .  (while playing)          .
// skip/back 5 sec          Cmd-RIGHT/LEFT,RIGHT/LEFT   Ctrl-RIGHT/LEFT, RIGHT/LEFT,
//                                                        Alt-M-A/Alt-M-B
// volume up/down           Cmd-UP/DOWN,UP/DOWN         Ctrl-UP/DOWN, UP/DOWN             N/B
// mute                     Cmd-M, M                    Ctrl-M, M
// pitch up                 Cmd-U, U                    Ctrl-U, U, Alt-M-U                F
// pitch down               Cmd-D, D                    Ctrl-D, D, Alt-M-D                D
// go faster                +,=                         +,=                               R
// go slower                -                           -                                 E
// Fade Out                 Cmd-Y, Y
// Loop                     Cmd-L, L
// force mono                                           Alt-M-F
// Autostart Playback
// Sound FX                 Cmd-1 thru 6
// clear search             ESC, Cmd-/                  Alt-M-S
//
// LYRICS MENU
// Auto-scroll cuesheet
//
// SD MENU
// Enable Input             Cmd-I
//
// VIEW MENU
// Zoom IN/OUT              Cmd-=, Cmd-Minus
// Reset Zoom               Cmd-0
//
// OTHER (Not in a menu)
// Toggle between Music/Lyrics  T

// GLOBALS:
bass_audio cBass;
static const char *music_file_extensions[] = { "mp3", "wav", "m4a" };
static const char *cuesheet_file_extensions[] = { "htm", "html", "txt" };



// ----------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    oldFocusWidget(NULL),
    lastCuesheetSavePath(),
    timerCountUp(NULL),
    timerCountDown(NULL),
    trapKeypresses(true),
    sd(NULL),
    firstTimeSongIsPlayed(false),
    loadingSong(false),
    cuesheetEditorReactingToCursorMovement(false),
    totalZoom(0)
{
    justWentActive = false;

    for (int i=0; i<6; i++) {
        soundFXarray[i] = "";
    }

    maybeInstallSoundFX();

    PreferencesManager prefsManager; // Will be using application information for correct location of your settings

    if (prefsManager.GetenableAutoMicsOff()) {
        currentInputVolume = getInputVolume();  // save current volume
    }

    // Disable ScreenSaver while SquareDesk is running
#if defined(Q_OS_MAC)
    macUtils.disableScreensaver();
#elif defined(Q_OS_WIN)
#pragma comment(lib, "user32.lib")
    SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE , NULL, SPIF_SENDWININICHANGE);
#elif defined(Q_OS_LINUX)
    // TODO
#endif

    // Disable extra (Native Mac) tab bar
#if defined(Q_OS_MAC)
    macUtils.disableWindowTabbing();
#endif

    prefDialog = NULL;      // no preferences dialog created yet
    songLoaded = false;     // no song is loaded, so don't update the currentLocLabel

    ui->setupUi(this);
    ui->statusBar->showMessage("");

    setFontSizes();

    this->setWindowTitle(QString("SquareDesk Music Player/Sequence Editor"));

    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

    ui->previousSongButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    ui->nextSongButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));

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
    delete ui->mainToolBar; // remove toolbar on WINDOWS (toolbar is not present on Mac)
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
    switchToLyricsOnPlay = false;

    Info_Seekbar(false);

    // setup playback timer
    UIUpdateTimer = new QTimer(this);
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
    previousVolume = 100;
    Info_Volume();

    // VU Meter -----
    vuMeterTimer = new QTimer(this);
    connect(vuMeterTimer, SIGNAL(timeout()), this, SLOT(on_vuMeterTimerTick()));
    vuMeterTimer->start(50);           // adjust from GUI with timer->setInterval(newValue)

    vuMeter = new LevelMeter(this);
    ui->gridLayout_2->addWidget(vuMeter, 1,5);  // add it to the layout in the right spot
    vuMeter->setFixedHeight(20);

    vuMeter->reset();
    vuMeter->setEnabled(true);
    vuMeter->setVisible(true);

    // analog clock -----
    analogClock = new AnalogClock(this);
    ui->gridLayout_2->addWidget(analogClock, 2,6,4,1);  // add it to the layout in the right spot
    analogClock->setFixedSize(QSize(110,110));
    analogClock->setEnabled(true);
    analogClock->setVisible(true);

    // where is the root directory where all the music is stored?
    pathStack = new QList<QString>();

    musicRootPath = prefsManager.GetmusicPath();
    guestRootPath = ""; // initially, no guest music
    guestVolume = "";   // and no guest volume present
    guestMode = "main"; // and not guest mode

    switchToLyricsOnPlay = prefsManager.GetswitchToLyricsOnPlay();

    updateRecentPlaylistMenu();

#if defined(Q_OS_MAC) | defined(Q_OS_WIN32)
    // initial Guest Mode stuff works on Mac OS and WIN32 only
    //   should be straightforward to extend to Linux.
    lastKnownVolumeList = getCurrentVolumes();  // get initial list
    newVolumeList = lastKnownVolumeList;  // keep lists sorted, for easy comparisons
#endif

    // set initial colors for text in songTable, also used for shading the clock
    patterColorString = prefsManager.GetpatterColorString();
    singingColorString = prefsManager.GetsingingColorString();
    calledColorString = prefsManager.GetcalledColorString();
    extrasColorString = prefsManager.GetextrasColorString();

    // Tell the clock what colors to use for session segments
    analogClock->setColorForType(PATTER, QColor(patterColorString));
    analogClock->setColorForType(SINGING, QColor(singingColorString));
    analogClock->setColorForType(SINGING_CALLED, QColor(calledColorString));
    analogClock->setColorForType(XTRAS, QColor(extrasColorString));

    // ----------------------------------------------
    // Save the new settings for experimental break and patter timers --------
    tipLengthTimerEnabled = prefsManager.GettipLengthTimerEnabled();
    int tipLengthTimerLength = prefsManager.GettipLengthTimerLength();
    tipLengthAlarmAction = prefsManager.GettipLengthAlarmAction();

    breakLengthTimerEnabled = prefsManager.GetbreakLengthTimerEnabled();
    breakLengthTimerLength = prefsManager.GetbreakLengthTimerLength();
    breakLengthAlarmAction = prefsManager.GetbreakLengthAlarmAction();

    analogClock->tipLengthTimerEnabled = tipLengthTimerEnabled;      // tell the clock whether the patter alarm is enabled
    analogClock->breakLengthTimerEnabled = breakLengthTimerEnabled;  // tell the clock whether the break alarm is enabled

    // ----------------------------------------------
    songFilenameFormat = static_cast<SongFilenameMatchingType>(prefsManager.GetSongFilenameFormat());

    // define type names (before reading in the music filenames!) ------------------
    QString value;
    value = prefsManager.GetMusicTypeSinging();
    songTypeNamesForSinging = value.toLower().split(";", QString::KeepEmptyParts);

    value = prefsManager.GetMusicTypePatter();
    songTypeNamesForPatter = value.toLower().split(";", QString::KeepEmptyParts);

    value = prefsManager.GetMusicTypeExtras();
    songTypeNamesForExtras = value.toLower().split(';', QString::KeepEmptyParts);

    value = prefsManager.GetMusicTypeCalled();
    songTypeNamesForCalled = value.toLower().split(';', QString::KeepEmptyParts);

    // -------------------------
    setCurrentSessionId((SessionDefaultPractice ==
                         static_cast<SessionDefaultType>(prefsManager.GetSessionDefault()))
                        ? 1 : songSettings.getCurrentSession());

    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled
    // used to store the file paths
    findMusic(musicRootPath,"","main", true);  // get the filenames from the user's directories
    loadMusicList(); // and filter them into the songTable
#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    ui->listWidgetChoreographySequences->setStyleSheet(
         "QListWidget::item { border-bottom: 1px solid black; }" );
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    loadChoreographyList();

    ui->toolButtonEditLyrics->setStyleSheet("QToolButton { border: 1px solid #575757; border-radius: 4px; background-color: palette(base); }");  // turn off border

    // ----------
    updateSongTableColumnView(); // update the actual view of Age/Pitch/Tempo in the songTable view

    // ----------
    clockColoringHidden = !prefsManager.GetexperimentalClockColoringEnabled();
    analogClock->setHidden(clockColoringHidden);

    // -----------
    ui->actionAutostart_playback->setChecked(prefsManager.Getautostartplayback());

    // -----------

    ui->checkBoxPlayOnEnd->setChecked(prefsManager.Getstartplaybackoncountdowntimer());
    ui->checkBoxStartOnPlay->setChecked(prefsManager.Getstartcountuptimeronplay());

    // -------
    on_monoButton_toggled(prefsManager.Getforcemono());

    on_actionAge_toggled(prefsManager.GetshowAgeColumn());
    on_actionPitch_toggled(prefsManager.GetshowPitchColumn());
    on_actionTempo_toggled(prefsManager.GetshowTempoColumn());

// voice input is only available on MAC OS X and Win32 right now...
#ifdef POCKETSPHINXSUPPORT
    on_actionEnable_voice_input_toggled(prefsManager.Getenablevoiceinput());
    voiceInputEnabled = prefsManager.Getenablevoiceinput();
#else
    on_actionEnable_voice_input_toggled(false);
    voiceInputEnabled = false;
#endif

    on_actionAuto_scroll_during_playback_toggled(prefsManager.Getenableautoscrolllyrics());
    autoScrollLyricsEnabled = prefsManager.Getenableautoscrolllyrics();

    // Volume, Pitch, and Mix can be set before loading a music file.  NOT tempo.
    ui->pitchSlider->setEnabled(true);
    ui->pitchSlider->setValue(0);
    ui->currentPitchLabel->setText("0 semitones");
    // FIX: initial focus is titleSearch, so the shortcuts for these menu items don't work
    //   at initial start.
    ui->actionPitch_Down->setEnabled(true);  // and the corresponding menu items
    ui->actionPitch_Up->setEnabled(true);

    ui->volumeSlider->setEnabled(true);
    ui->volumeSlider->setValue(100);
    ui->currentVolumeLabel->setText("Max");
    // FIX: initial focus is titleSearch, so the shortcuts for these menu items don't work
    //   at initial start.
    ui->actionVolume_Down->setEnabled(true);  // and the corresponding menu items
    ui->actionVolume_Up->setEnabled(true);
    ui->actionMute->setEnabled(true);

    ui->mixSlider->setEnabled(true);
    ui->mixSlider->setValue(0);
    ui->currentMixLabel->setText("100%L/100%R");

    // ...and the EQ sliders, too...
    ui->bassSlider->setEnabled(true);
    ui->midrangeSlider->setEnabled(true);
    ui->trebleSlider->setEnabled(true);

    // in the Designer, these have values, making it easy to visualize there
    //   must clear those out, because a song is not loaded yet.
    ui->currentLocLabel->setText("");
    ui->songLengthLabel->setText("");

    inPreferencesDialog = false;

    // save info about the experimental timers tab
    // experimental timers tab is tab #1 (second tab)
    // experimental lyrics tab is tab #2 (third tab)
    tabmap.insert(1, QPair<QWidget *,QString>(ui->tabWidget->widget(1), ui->tabWidget->tabText(1)));
    tabmap.insert(2, QPair<QWidget *,QString>(ui->tabWidget->widget(2), ui->tabWidget->tabText(2)));

    bool timersEnabled = prefsManager.GetexperimentalTimersEnabled();
    // ----------
    showTimersTab = true;
    if (!timersEnabled) {
        ui->tabWidget->removeTab(1);  // it's remembered, don't worry!
        showTimersTab = false;
    }

    // ----------
    bool lyricsEnabled = true;
    showLyricsTab = true;
    lyricsTabNumber = (showTimersTab ? 2 : 1);
    if (!lyricsEnabled) {
        ui->tabWidget->removeTab(timersEnabled ? 2 : 1);  // it's remembered, don't worry!
        showLyricsTab = false;
        lyricsTabNumber = -1;
    }
    ui->tabWidget->setCurrentIndex(0); // Music Player tab is primary, regardless of last setting in Qt Designer
    ui->tabWidget->setTabText(lyricsTabNumber, "Lyrics");  // c.f. Preferences

    // ----------
    connect(ui->songTable->horizontalHeader(),&QHeaderView::sectionResized,
            this, &MainWindow::columnHeaderResized);

    resize(QDesktopWidget().availableGeometry(this).size() * 0.7);  // initial size is 70% of screen

    setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            size(),
            qApp->desktop()->availableGeometry()
        )
    );

    ui->titleSearch->setFocus();  // this should be the intial focus

#ifdef DEBUGCLOCK
    analogClock->tipLengthAlarmMinutes = 5;  // FIX FIX FIX
    analogClock->breakLengthAlarmMinutes = 10;
#endif
    analogClock->tipLengthAlarmMinutes = tipLengthTimerLength;
    analogClock->breakLengthAlarmMinutes = breakLengthTimerLength;
    ui->warningLabel->setText("");
    ui->warningLabel->setStyleSheet("QLabel { color : red; }");

    // LYRICS TAB ------------
    ui->pushButtonSetIntroTime->setEnabled(false);  // initially not singing call, buttons will be greyed out on Lyrics tab
    ui->pushButtonSetOutroTime->setEnabled(false);

    analogClock->setTimerLabel(ui->warningLabel);  // tell the clock which label to use for the patter timer

    // read list of calls (in application bundle on Mac OS X)
    // TODO: make this work on other platforms, but first we have to figure out where to put the allcalls.csv
    //   file on those platforms.  It's convenient to stick it in the bundle on Mac OS X.  Maybe parallel with
    //   the executable on Windows and Linux?

#if defined(Q_OS_MAC)
    QString appPath = QApplication::applicationFilePath();
    QString allcallsPath = appPath + "/Contents/Resources/allcalls.csv";
    allcallsPath.replace("Contents/MacOS/SquareDeskPlayer/","");
#endif

#if defined(Q_OS_WIN32)
    // TODO: There has to be a better way to do this.
    QString appPath = QApplication::applicationFilePath();
    QString allcallsPath = appPath + "/allcalls.csv";
    allcallsPath.replace("SquareDeskPlayer.exe/","");
#endif

#if defined(Q_OS_MAC) | defined(Q_OS_WIN32)
    QFile file(allcallsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open 'allcalls.csv' file.";
        qDebug() << "looked here:" << allcallsPath;
        return;
    }

    while (!file.atEnd()) {
        QString line = file.readLine().simplified();
        QStringList lineparts = line.split(',');
        QString level = lineparts[0];
        QString call = lineparts[1].replace("\"","");

        if (level == "plus") {
            flashCalls.append(call);
        }
    }

    currentSongType = "";
    currentSongTitle = "";

    qsrand(QTime::currentTime().msec());  // different random sequence of calls each time, please.
    randCallIndex = qrand() % flashCalls.length();   // start out with a number other than zero, please.

#endif

    // Make menu items mutually exclusive
    QList<QAction*> actions = ui->menuSequence->actions();
    //    qDebug() << "ACTIONS:" << actions;

    sdActionGroup1 = new QActionGroup(this);
    sdActionGroup1->setExclusive(true);

    sdActionGroup1->addAction(actions[2]);  // NORMAL
    sdActionGroup1->addAction(actions[3]);  // Color only
    sdActionGroup1->addAction(actions[4]);  // Mental image
    sdActionGroup1->addAction(actions[5]);  // Sight

    connect(sdActionGroup1, SIGNAL(triggered(QAction*)), this, SLOT(sdActionTriggered(QAction*)));

    QSettings settings;


    {
        ui->tableWidgetCallList->setColumnWidth(kCallListOrderCol,40);
        ui->tableWidgetCallList->setColumnWidth(kCallListCheckedCol, 24);
        // #define kCallListNameCol        2
        ui->tableWidgetCallList->setColumnWidth(kCallListWhenCheckedCol, 100);
        QHeaderView *headerView = ui->tableWidgetCallList->horizontalHeader();
        headerView->setSectionResizeMode(kCallListOrderCol, QHeaderView::Fixed);
        headerView->setSectionResizeMode(kCallListCheckedCol, QHeaderView::Fixed);
        headerView->setSectionResizeMode(kCallListNameCol, QHeaderView::Stretch);
        headerView->setSectionResizeMode(kCallListWhenCheckedCol, QHeaderView::Fixed);
        headerView->setStretchLastSection(false);
        QString lastDanceProgram(settings.value("lastCallListDanceProgram").toString());
        loadDanceProgramList(lastDanceProgram);
    }

    lastCuesheetSavePath = settings.value("lastCuesheetSavePath").toString();

    initSDtab();  // init sd, pocketSphinx, and the sd tab widgets

    if (prefsManager.GetenableAutoAirplaneMode()) {
        airplaneMode(true);
    }

    connect(QApplication::instance(), SIGNAL(applicationStateChanged(Qt::ApplicationState)),
            this, SLOT(changeApplicationState(Qt::ApplicationState)));
    connect(QApplication::instance(), SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(focusChanged(QWidget*,QWidget*)));

    int songCount = 0;
    QString firstBadSongLine;
    QString CurrentPlaylistFileName = musicRootPath + "/.squaredesk/current.m3u";
    firstBadSongLine = loadPlaylistFromFile(CurrentPlaylistFileName, songCount);  // load "current.csv" (if doesn't exist, do nothing)

    ui->songTable->setColumnWidth(kNumberCol,40);  // NOTE: This must remain a fixed width, due to a bug in Qt's tracking of its width.
    ui->songTable->setColumnWidth(kTypeCol,96);
    ui->songTable->setColumnWidth(kLabelCol,80);
//  kTitleCol is always expandable, so don't set width here
    ui->songTable->setColumnWidth(kAgeCol, 36);
    ui->songTable->setColumnWidth(kPitchCol,50);
    ui->songTable->setColumnWidth(kTempoCol,50);

    usePersistentFontSize(); // sets the font of the songTable, which is used by adjustFontSizes to scale other font sizes

    adjustFontSizes();  // now adjust to match contents ONCE
    //on_actionReset_triggered();  // set initial layout
    ui->songTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);  // auto set height of rows

    QPalette* palette1 = new QPalette();
    palette1->setColor(QPalette::ButtonText, Qt::red);
    ui->pushButtonCueSheetEditHeader->setPalette(*palette1);

    QPalette* palette2 = new QPalette();
    palette2->setColor(QPalette::ButtonText, QColor("#0000FF"));
    ui->pushButtonCueSheetEditArtist->setPalette(*palette2);

    QPalette* palette3 = new QPalette();
    palette3->setColor(QPalette::ButtonText, QColor("#60C060"));
    ui->pushButtonCueSheetEditLabel->setPalette(*palette3);

//    QPalette* palette4 = new QPalette();
//    palette4->setColor(QPalette::Background, QColor("#FFC0CB"));
//    ui->pushButtonCueSheetEditLyrics->setPalette(*palette4);

    ui->pushButtonCueSheetEditLyrics->setAutoFillBackground(true);
    ui->pushButtonCueSheetEditLyrics->setStyleSheet(
                "QPushButton {background-color: #FFC0CB; color: #000000; border-radius:4px; padding:1px 8px; border:0.5px solid #CF9090;}"
                "QPushButton:pressed { background-color: qlineargradient(x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 #1E72FE, stop: 1 #3E8AFC); color: #FFFFFF; border:0.5px solid #0D60E3;}"
                "QPushButton:disabled {background-color: #F1F1F1; color: #7F7F7F; border-radius:4px; padding:1px 8px; border:0.5px solid #D0D0D0;}"
                );
}

void MainWindow::changeApplicationState(Qt::ApplicationState state)
{
    currentApplicationState = state;
    microphoneStatusUpdate();  // this will disable the mics, if not Active state

    if (state == Qt::ApplicationActive) {
        justWentActive = true;
    }
}

void MainWindow::focusChanged(QWidget *old1, QWidget *new1)
{
    // all this mess, just to restore NO FOCUS, after ApplicationActivate, if there was NO FOCUS
    //   when going into Inactive state
    if (!justWentActive && new1 == 0) {
        oldFocusWidget = old1;  // GOING INACTIVE, RESTORE THIS ONE
    } else if (justWentActive) {
        if (oldFocusWidget == 0) {
            if (QApplication::focusWidget() != 0) {
                QApplication::focusWidget()->clearFocus();  // BOGUS
            }
        } else {
            oldFocusWidget->setFocus(); // RESTORE HAPPENS HERE
        }
        justWentActive = false;  // this was a one-shot deal.
    } else {
        oldFocusWidget = new1;  // clicked on something, this is the one to restore
    }

}

void MainWindow::setCurrentSessionId(int id)
{
    QAction *actions[] = {
        ui->actionPractice,
        ui->actionMonday,
        ui->actionTuesday,
        ui->actionWednesday,
        ui->actionThursday,
        ui->actionFriday,
        ui->actionSaturday,
        ui->actionSunday,
        NULL
    };
    for (int i = 0; actions[i]; ++i)
    {
        int this_id = i+1;
        bool checked = this_id == id;
        actions[i]->setChecked(checked);
    }
    songSettings.setCurrentSession(id);
}

void MainWindow::reloadSongAges(bool show_all_ages)
{
    QHash<QString,QString> ages;
    songSettings.getSongAges(ages, show_all_ages);

    ui->songTable->setSortingEnabled(false);
    ui->songTable->hide();

    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QString origPath = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
        QString path = songSettings.removeRootDirs(origPath);
        QHash<QString,QString>::const_iterator age = ages.constFind(path);

        ui->songTable->item(i,kAgeCol)->setText(age == ages.constEnd() ? "" : age.value());
        ui->songTable->item(i,kAgeCol)->setTextAlignment(Qt::AlignCenter);
    }
    ui->songTable->show();
    ui->songTable->setSortingEnabled(true);
}

void MainWindow::setCurrentSessionIdReloadSongAges(int id)
{
    setCurrentSessionId(id);
    reloadSongAges(ui->actionShow_All_Ages->isChecked());
    on_comboBoxCallListProgram_currentIndexChanged(ui->comboBoxCallListProgram->currentIndex());
}

static void AddItemToCallList(QTableWidget *tableWidget,
                              const QString &number, const QString &name,
                              const QString &taughtOn)
{
    int initialRowCount = tableWidget->rowCount();
    tableWidget->setRowCount(initialRowCount + 1);
    int row = initialRowCount;

    QTableWidgetItem *numberItem = new QTableWidgetItem(number);
    QTableWidgetItem *nameItem = new QTableWidgetItem(name);

    numberItem->setFlags(numberItem->flags() & ~Qt::ItemIsEditable);
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);

    tableWidget->setItem(row, kCallListOrderCol, numberItem);
    tableWidget->setItem(row, kCallListNameCol, nameItem);

    QTableWidgetItem *dateItem = new QTableWidgetItem(taughtOn);
    dateItem->setFlags(dateItem->flags() | Qt::ItemIsEditable);
    tableWidget->setItem(row, kCallListWhenCheckedCol, dateItem);

    QTableWidgetItem *checkBoxItem = new QTableWidgetItem();
    checkBoxItem->setCheckState((taughtOn.isNull() || taughtOn.isEmpty()) ? Qt::Unchecked : Qt::Checked);
    tableWidget->setItem(row, kCallListCheckedCol, checkBoxItem);
    tableWidget->item(row, kCallListCheckedCol)->setTextAlignment(Qt::AlignCenter);
}

static void loadCallList(SongSettings &songSettings, QTableWidget *tableWidget, const QString &danceProgram, const QString &filename)
{
    static QRegularExpression regex_numberCommaName(QRegularExpression("^((\\s*\\d+)(\\.\\w+)?)\\,?\\s+(.*)$"));

    tableWidget->setRowCount(0);

    QFile inputFile(filename);
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
        int line_number = 0;
        while (!in.atEnd())
        {
            line_number++;
            QString line = in.readLine();

            QString number(QString("%1").arg(line_number, 2));
            QString name(line);

            QRegularExpressionMatch match = regex_numberCommaName.match(line);
            if (match.hasMatch())
            {
                QString prefix("");
                if (match.captured(2).length() < 2)
                {
                    prefix = " ";
                }
                number = prefix + match.captured(1);
                name = match.captured(4);
            }
            QString taughtOn = songSettings.getCallTaughtOn(danceProgram, name);
            AddItemToCallList(tableWidget, number, name, taughtOn);
        }
        inputFile.close();
    }
}

void breakDanceProgramIntoParts(const QString &filename,
                                QString &name,
                                QString &program)
{
    QFileInfo fi(filename);
    name = fi.completeBaseName();
    program = name;
    int j = name.indexOf('.');
    if (-1 != j)
    {
        bool isSortOrder = true;
        for (int i = 0; i < j; ++i)
        {
            if (!name[i].isDigit())
            {
                isSortOrder = false;
            }
        }
        if (!isSortOrder)
        {
            program = name.left(j);
        }
        name.remove(0, j + 1);
        j = name.indexOf('.');
        if (-1 != j && isSortOrder)
        {
            program = name.left(j);
            name.remove(0, j + 1);
        }
        else
        {
            program = name;
        }
    }

}

// ----------------------------------------------------------------------
// Text editor stuff

void MainWindow::showHTML() {
    QString editedCuesheet = ui->textBrowserCueSheet->toHtml();
    QString tEditedCuesheet = tidyHTML(editedCuesheet);
    QString pEditedCuesheet = postProcessHTMLtoSemanticHTML(tEditedCuesheet);
//    qDebug().noquote() << "***** Post-processed HTML will be:\n" << pEditedCuesheet;
}

void MainWindow::on_toolButtonEditLyrics_toggled(bool checkState)
{
    bool checked = (checkState != Qt::Unchecked);

    ui->pushButtonCueSheetEditTitle->setEnabled(checked);
    ui->pushButtonCueSheetEditLabel->setEnabled(checked);
    ui->pushButtonCueSheetEditArtist->setEnabled(checked);
    ui->pushButtonCueSheetEditHeader->setEnabled(checked);
    ui->pushButtonCueSheetEditLyrics->setEnabled(checked);
    ui->pushButtonCueSheetEditBold->setEnabled(checked);
    ui->pushButtonCueSheetEditItalic->setEnabled(checked);

    ui->pushButtonCueSheetClearFormatting->setEnabled(checked);  // this one is special.
}

void MainWindow::on_textBrowserCueSheet_selectionChanged()
{
//    QTextCursor cursor = ui->textBrowserCueSheet->textCursor();
//    QString selectedText = cursor.selectedText();
//    qDebug() << "New selected text: '" << selectedText << "'";

    // the Clear Line Format is only available when the cursor is somewhere on a line,
    //  but is not selecting any text AND editing is enabled (i.e. the lock is UNLOCKED).
    //  Formatting will be cleared on that line only.  This is the best we can do right now, I think,
    //  given the limitations of QTextEdit (which is not a general HTML editor).
    QTextCursor cursor = ui->textBrowserCueSheet->textCursor();
    QString selectedText = cursor.selectedText();
    ui->pushButtonCueSheetClearFormatting->setEnabled(selectedText.isEmpty() && ui->toolButtonEditLyrics->isChecked());
}

// TODO: can't make a doc from scratch yet.

void MainWindow::on_pushButtonCueSheetClearFormatting_clicked()
{
        QTextCursor cursor = ui->textBrowserCueSheet->textCursor();

        // NOTE: in this initial version, Clear Line Format works just on
        //  the line that the cursor is on.  I'm not sure how to make it work
        //  on a selected block of text only.  It does not work properly when
        //  the current selection spans across lines.

        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

        // now look at it as HTML
        QString selected = cursor.selection().toHtml();
//        qDebug() << "cursor.selection(): " << selected;

        // Qt gives us a whole HTML doc here.  Strip off all the parts we don't want.
        QRegExp startSpan("<span.*>");
        startSpan.setMinimal(true);  // don't be greedy!

        selected.replace(QRegExp("<.*<!--StartFragment-->"),"")
                .replace(QRegExp("<!--EndFragment-->.*</html>"),"")
                .replace(startSpan,"")
                .replace("</span>","")
                ;
//        qDebug() << "current replacement: " << selected;

        // WARNING: this might have a dependency on cuesheet2.css's definition of BODY text.
        QString HTMLreplacement =
                "<span style=\" font-family:'Verdana'; font-size:large; color:#000000;\">" +
                selected +
                "</span>";

        cursor.beginEditBlock(); // start of grouping for UNDO purposes
        cursor.removeSelectedText();  // remove the rich text...
//        cursor.insertText(selected);  // ...and put back in the stripped-down text
        cursor.insertHtml(HTMLreplacement);  // ...and put back in the stripped-down text
        cursor.endEditBlock(); // end of grouping for UNDO purposes
}

void MainWindow::on_textBrowserCueSheet_currentCharFormatChanged(const QTextCharFormat & f)
{
    RecursionGuard guard(cuesheetEditorReactingToCursorMovement);
//    ui->pushButtonCueSheetEditHeader->setChecked(f.fontPointSize() == 14);
    ui->pushButtonCueSheetEditItalic->setChecked(f.fontItalic());
    ui->pushButtonCueSheetEditBold->setChecked(f.fontWeight() == QFont::Bold);
}

static void setSelectedTextToClass(QTextEdit *editor, QString blockClass)
{
//    qDebug() << "setSelectedTextToClass: " << blockClass;
    QTextCursor cursor = editor->textCursor();

    if (!cursor.hasComplexSelection())
    {
        // TODO: remove <SPAN class="title"></SPAN> from entire rest of the document (title is a singleton)
        QString selectedText = cursor.selectedText();
        if (selectedText.isEmpty())
        {
            // if cursor is not selecting any text, make the change apply to the entire line (vs block)
            cursor.movePosition(QTextCursor::StartOfLine);
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            selectedText = cursor.selectedText();
        }

        if (!selectedText.isEmpty())
        {
            cursor.beginEditBlock(); // start of grouping for UNDO purposes
            cursor.removeSelectedText();
            cursor.insertHtml("<P class=\"" + blockClass + "\">" + selectedText.toHtmlEscaped() + "</P>");
            cursor.endEditBlock(); // end of grouping for UNDO purposes
        }


    } else {
        // this should only happen for tables, which we really don't support yet.
        qDebug() << "Sorry, on_pushButtonCueSheetEdit...: " + blockClass + ": Title_toggled has complex selection...";
    }
}

void MainWindow::on_pushButtonCueSheetEditHeader_clicked(bool /* checked */)
{
    if (!cuesheetEditorReactingToCursorMovement)
    {
        setSelectedTextToClass(ui->textBrowserCueSheet, "hdr");
//    ui->textBrowserCueSheet->setFontPointSize(checked ? 18 : 14);
    }
//    showHTML();
}

void MainWindow::on_pushButtonCueSheetEditItalic_toggled(bool checked)
{
    if (!cuesheetEditorReactingToCursorMovement)
    {
        ui->textBrowserCueSheet->setFontItalic(checked);
    }
//    showHTML();
}

void MainWindow::on_pushButtonCueSheetEditBold_toggled(bool checked)
{
    if (!cuesheetEditorReactingToCursorMovement)
    {
        ui->textBrowserCueSheet->setFontWeight(checked ? QFont::Bold : QFont::Normal);
    }
//    showHTML();
}


void MainWindow::on_pushButtonCueSheetEditTitle_clicked(bool checked)
{
    Q_UNUSED(checked)
    if (!cuesheetEditorReactingToCursorMovement)
    {
        setSelectedTextToClass(ui->textBrowserCueSheet, "title");
    }
//    showHTML();
}

void MainWindow::on_pushButtonCueSheetEditArtist_clicked(bool checked)
{
    Q_UNUSED(checked)
    if (!cuesheetEditorReactingToCursorMovement)
    {
        setSelectedTextToClass(ui->textBrowserCueSheet, "artist");
    }
//    showHTML();
}

void MainWindow::on_pushButtonCueSheetEditLabel_clicked(bool checked)
{
    Q_UNUSED(checked)
    if (!cuesheetEditorReactingToCursorMovement)
    {
        setSelectedTextToClass(ui->textBrowserCueSheet, "label");
    }
//    showHTML();
}

void MainWindow::on_pushButtonCueSheetEditLyrics_clicked(bool checked)
{
    Q_UNUSED(checked)
    if (!cuesheetEditorReactingToCursorMovement)
    {
        setSelectedTextToClass(ui->textBrowserCueSheet, "lyrics");
    }
//    showHTML();
}

static bool isFileInPathStack(QList<QString> *pathStack, const QString &checkFilename)
{
    QListIterator<QString> iter(*pathStack);
    while (iter.hasNext()) {
        QString s = iter.next();
        QStringList sl1 = s.split("#!#");
        QString type = sl1[0];  // the type (of original pathname, before following aliases)
        QString filename = sl1[1];  // everything else
        if (filename == checkFilename)
            return true;
    }
    return false;
}

void MainWindow::writeCuesheet(QString filename)
{
    bool needs_extension = true;
    for (size_t i = 0; i < (sizeof(cuesheet_file_extensions) / sizeof(*cuesheet_file_extensions)); ++i)
    {
        QString ext(".");
        ext.append(cuesheet_file_extensions[i]);
        if (filename.endsWith(ext))
        {
            needs_extension = false;
            break;
        }
    }
    if (needs_extension)
    {
        filename += ".html";
    }

    QFile file(filename);
    QDir d = QFileInfo(file).absoluteDir();
    QString directoryName = d.absolutePath();  // directory of the saved filename

    lastCuesheetSavePath = directoryName;

    if ( file.open(QIODevice::WriteOnly) )
    {
        // Make sure the destructor gets called before we try to load this file...
        {
            QTextStream stream( &file );
//                qDebug() << "************** SAVE FILE ***************";
            showHTML();

            QString editedCuesheet = ui->textBrowserCueSheet->toHtml();
//                qDebug().noquote() << "***** editedCuesheet to write:\n" << editedCuesheet;

            QString tEditedCuesheet = tidyHTML(editedCuesheet);
//                qDebug().noquote() << "***** tidied editedCuesheet to write:\n" << tEditedCuesheet;
            QString postProcessedCuesheet = postProcessHTMLtoSemanticHTML(tEditedCuesheet);
//                qDebug().noquote() << "***** WRITING TO FILE postProcessed:\n" << postProcessedCuesheet;

            stream << postProcessedCuesheet;
            stream.flush();
        }

        if (!isFileInPathStack(pathStack, filename))
        {
            QFileInfo fi(filename);
            QStringList section = fi.path().split("/");
            QString type = section[section.length()-1];  // must be the last item in the path
//            qDebug() << "writeCuesheet() adding " + type + "#!#" + filename + " to pathStack";
            pathStack->append(type + "#!#" + filename);
        }
    }
}

void MainWindow::on_pushButtonCueSheetEditSave_clicked()
{
    on_actionSave_Lyrics_As_triggered();
}


// ----------------------------------------------------------------------
void MainWindow::on_tableWidgetCallList_cellChanged(int row, int col)
{
    if (kCallListCheckedCol == col)
    {
        int currentIndex = ui->comboBoxCallListProgram->currentIndex();
        QString programFilename(ui->comboBoxCallListProgram->itemData(currentIndex).toString());
        QString displayName;
        QString danceProgram;
        breakDanceProgramIntoParts(programFilename, displayName, danceProgram);

        QString callName = ui->tableWidgetCallList->item(row,kCallListNameCol)->text();

        if (ui->tableWidgetCallList->item(row,col)->checkState() == Qt::Checked)
        {
            songSettings.setCallTaught(danceProgram, callName);
        }
        else
        {
            songSettings.deleteCallTaught(danceProgram, callName);
        }

        QTableWidgetItem *dateItem(new QTableWidgetItem(songSettings.getCallTaughtOn(danceProgram, callName)));
        ui->tableWidgetCallList->setItem(row, kCallListWhenCheckedCol, dateItem);
    }
}

void MainWindow::on_comboBoxCallListProgram_currentIndexChanged(int currentIndex)
{
    ui->tableWidgetCallList->setRowCount(0);
    ui->tableWidgetCallList->setSortingEnabled(false);
    QString programFilename(ui->comboBoxCallListProgram->itemData(currentIndex).toString());
    if (!programFilename.isNull() && !programFilename.isEmpty())
    {
        QString name;
        QString program;
        breakDanceProgramIntoParts(programFilename, name, program);

        loadCallList(songSettings, ui->tableWidgetCallList, program, programFilename);
        QSettings settings;
        settings.setValue("lastCallListDanceProgram",program);
    }
    ui->tableWidgetCallList->setSortingEnabled(true);
}


void MainWindow::on_comboBoxCuesheetSelector_currentIndexChanged(int currentIndex)
{
    if (currentIndex != -1 && !cuesheetEditorReactingToCursorMovement) {
        QString cuesheetFilename = ui->comboBoxCuesheetSelector->itemData(currentIndex).toString();
        loadCuesheet(cuesheetFilename);
    }
}

void MainWindow::on_menuLyrics_aboutToShow()
{
    ui->actionSave_Lyrics->setEnabled(ui->textBrowserCueSheet->document()->isModified());
    ui->actionLyricsCueSheetRevert_Edits->setEnabled(ui->textBrowserCueSheet->document()->isModified());
}

void MainWindow::on_actionLyricsCueSheetRevert_Edits_triggered(bool /*checked*/)
{
    on_comboBoxCuesheetSelector_currentIndexChanged(ui->comboBoxCuesheetSelector->currentIndex());
}

void MainWindow::on_actionCompact_triggered(bool checked)
{
    bool visible = !checked;
    setCueSheetAdditionalControlsVisible(visible);
    ui->actionCompact->setChecked(!visible);

    for (int col = 0; col < ui->gridLayout_2->columnCount(); ++col)
    {
        for (int row = 2; row < ui->gridLayout_2->rowCount(); ++row)
        {
            QLayoutItem *layout_item = ui->gridLayout_2->itemAtPosition(row,col);
            if (layout_item)
            {
                QWidget *widget = layout_item->widget();
                if (widget)
                {
                    if (visible)
                    {
                        widget->show();
                    }
                    else
                    {
                        widget->hide();
                    }
                }
            }
        }
    }
    return;
}


void MainWindow::on_actionShow_All_Ages_triggered(bool checked)
{
    reloadSongAges(checked);
}

void MainWindow::on_actionPractice_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadSongAges(1);
}

void MainWindow::on_actionMonday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadSongAges(2);
}

void MainWindow::on_actionTuesday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadSongAges(3);
}

void MainWindow::on_actionWednesday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadSongAges(4);
}

void MainWindow::on_actionThursday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadSongAges(5);
}

void MainWindow::on_actionFriday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadSongAges(6);
}

void MainWindow::on_actionSaturday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadSongAges(7);
}

void MainWindow::on_actionSunday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadSongAges(8);
}


// ----------------------------------------------------------------------
MainWindow::~MainWindow()
{
    // Just before the app quits, save the current playlist state in "current.m3u", and it will be reloaded
    //   when the app starts up again.
    // Save the current playlist state to ".squaredesk/current.m3u".  Tempo/pitch are NOT saved here.
    QString PlaylistFileName = musicRootPath + "/.squaredesk/current.m3u";
    saveCurrentPlaylistToFile(PlaylistFileName);

    PreferencesManager prefsManager; // Will be using application information for correct location of your settings

    // bug workaround: https://bugreports.qt.io/browse/QTBUG-56448
    QColorDialog colorDlg(0);
    colorDlg.setOption(QColorDialog::NoButtons);
    colorDlg.setCurrentColor(Qt::white);

    delete ui;

    // REENABLE SCREENSAVER, RELEASE THE KRAKEN
#if defined(Q_OS_MAC)
    macUtils.reenableScreensaver();
#elif defined(Q_OS_WIN32)
    SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE , NULL, SPIF_SENDWININICHANGE);
#endif
    if (sd) {
        sd->kill();
    }
#if defined(POCKETSPHINXSUPPORT)
    if (ps) {
        ps->kill();
    }
#endif

    if (prefsManager.GetenableAutoAirplaneMode()) {
        airplaneMode(false);
    }

    if (prefsManager.GetenableAutoMicsOff()) {
        unmuteInputVolume();  // if it was muted, it's now unmuted.
    }
}

void MainWindow::setFontSizes()
{
#if defined(Q_OS_MAC)
    preferredSmallFontSize = 13;
    preferredNowPlayingSize = 27;
#elif defined(Q_OS_WIN32)
    preferredSmallFontSize = 8;
    preferredNowPlayingSize = 16;
#elif defined(Q_OS_LINUX)
    preferredSmallFontSize = 13;  // FIX: is this right?
    preferredNowPlayingSize = 27;

    QFont fontSmall = ui->currentTempoLabel->font();
    fontSmall.setPointSize(8);
    fontSmall.setPointSize(preferredSmallFontSize);

    ui->titleSearch->setFont(fontSmall);
    ui->labelSearch->setFont(fontSmall);
    ui->titleSearch->setFont(fontSmall);
    ui->clearSearchButton->setFont(fontSmall);
    ui->songTable->setFont(fontSmall);
#endif

    QFont font = ui->currentTempoLabel->font();
    font.setPointSize(preferredSmallFontSize);

    ui->tabWidget->setFont(font);  // most everything inherits from this one
    ui->statusBar->setFont(font);
    ui->currentLocLabel->setFont(font);
    ui->songLengthLabel->setFont(font);
    ui->bassLabel->setFont(font);
    ui->midrangeLabel->setFont(font);
    ui->trebleLabel->setFont(font);
    ui->EQgroup->setFont(font);

    font.setPointSize(preferredSmallFontSize-2);
#if defined(Q_OS_MAC)
    QString styleForCallerlabDefinitions("QLabel{font-size:10pt;}");
#endif
#if defined(Q_OS_WIN)
    QString styleForCallerlabDefinitions("QLabel{font-size:6pt;}");
#endif
#if defined(Q_OS_LINUX)
    QString styleForCallerlabDefinitions("QLabel{font-size:6pt;}");  // DAN, PLEASE ADJUST THIS
#endif
    font.setPointSize(preferredNowPlayingSize);
    ui->nowPlayingLabel->setFont(font);

    font.setPointSize((preferredSmallFontSize + preferredNowPlayingSize)/2);
    ui->warningLabel->setFont(font);
}

// ----------------------------------------------------------------------
void MainWindow::updateSongTableColumnView()
{
    PreferencesManager prefsManager;

    ui->songTable->setColumnHidden(kAgeCol,!prefsManager.GetshowAgeColumn());
    ui->songTable->setColumnHidden(kPitchCol,!prefsManager.GetshowPitchColumn());
    ui->songTable->setColumnHidden(kTempoCol,!prefsManager.GetshowTempoColumn());

    // http://www.qtcentre.org/threads/3417-QTableWidget-stretch-a-column-other-than-the-last-one
    QHeaderView *headerView = ui->songTable->horizontalHeader();
    headerView->setSectionResizeMode(kNumberCol, QHeaderView::Interactive);
    headerView->setSectionResizeMode(kTypeCol, QHeaderView::Interactive);
    headerView->setSectionResizeMode(kLabelCol, QHeaderView::Interactive);
    headerView->setSectionResizeMode(kTitleCol, QHeaderView::Stretch);
    headerView->setSectionResizeMode(kAgeCol, QHeaderView::Fixed);
    headerView->setSectionResizeMode(kPitchCol, QHeaderView::Fixed);
    headerView->setSectionResizeMode(kTempoCol, QHeaderView::Fixed);
    headerView->setStretchLastSection(false);
}


// ----------------------------------------------------------------------
void MainWindow::on_loopButton_toggled(bool checked)
{
    if (checked) {
        ui->actionLoop->setChecked(true);

        ui->seekBar->SetLoop(true);
        ui->seekBarCuesheet->SetLoop(true);

        double songLength = cBass.FileLength;
//        qDebug() << "songLength: " << songLength << ", Intro: " << ui->seekBar->GetIntro();

//        cBass.SetLoop(songLength * 0.9, songLength * 0.1); // FIX: use parameters in the MP3 file
        cBass.SetLoop(songLength * ui->seekBar->GetOutro(),
                      songLength * ui->seekBar->GetIntro());
    }
    else {
        ui->actionLoop->setChecked(false);

        ui->seekBar->SetLoop(false);
        ui->seekBarCuesheet->SetLoop(false);

        cBass.ClearLoop();
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_monoButton_toggled(bool checked)
{
    if (checked) {
        ui->actionForce_Mono_Aahz_mode->setChecked(true);
        cBass.SetMono(true);
    }
    else {
        ui->actionForce_Mono_Aahz_mode->setChecked(false);
        cBass.SetMono(false);
    }

    // the Force Mono (Aahz Mode) setting is persistent across restarts of the application
    PreferencesManager prefsManager; // Will be using application information for correct location of your settings
    prefsManager.Setforcemono(ui->actionForce_Mono_Aahz_mode->isChecked());
}

// ----------------------------------------------------------------------
void MainWindow::on_stopButton_clicked()
{
// TODO: instead of removing focus on STOP, better we should restore focus to the previous focused widget on STOP
//    if (QApplication::focusWidget() != NULL) {
//        QApplication::focusWidget()->clearFocus();  // we don't want to continue editing the search fields after a STOP
//                                                    //  or it will eat our keyboard shortcuts
//    }

    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
    ui->actionPlay->setText("Play");  // now stopped, press Cmd-P to Play
    currentState = kStopped;

    cBass.Stop();  // Stop playback, rewind to the beginning

    ui->nowPlayingLabel->setText(currentSongTitle);  // restore the song title, if we were Flash Call mucking with it

    ui->seekBar->setValue(0);
    ui->seekBarCuesheet->setValue(0);
    Info_Seekbar(false);  // update just the text
}

// ----------------------------------------------------------------------
void MainWindow::randomizeFlashCall() {
    int numCalls = flashCalls.length();
    if (!numCalls)
        return;

    int newRandCallIndex;
    do {
        newRandCallIndex = qrand() % numCalls;
    } while (newRandCallIndex == randCallIndex);
    randCallIndex = newRandCallIndex;
}

// ----------------------------------------------------------------------
void MainWindow::on_playButton_clicked()
{
    if (!songLoaded) {
        return;  // if there is no song loaded, no point in toggling anything.
    }

    cBass.Play();  // currently paused, so start playing
    if (currentState == kStopped || currentState == kPaused) {
        // randomize the Flash Call, if PLAY (but not PAUSE) is pressed
        randomizeFlashCall();

        if (firstTimeSongIsPlayed)
        {
            firstTimeSongIsPlayed = false;
            saveCurrentSongSettings();
            songSettings.markSongPlayed(currentMP3filename, currentMP3filenameWithPath);
            QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
            QModelIndexList selected = selectionModel->selectedRows();

            ui->songTable->setSortingEnabled(false);
            int row = getSelectionRowForFilename(currentMP3filenameWithPath);
            if (row != -1)
            {
                ui->songTable->item(row, kAgeCol)->setText("0");
                ui->songTable->item(row, kAgeCol)->setTextAlignment(Qt::AlignCenter);
            }
            ui->songTable->setSortingEnabled(true);

            if (switchToLyricsOnPlay &&
                    (songTypeNamesForSinging.contains(currentSongType) || songTypeNamesForCalled.contains(currentSongType)))
            {
                // switch to Lyrics tab ONLY for singing calls or vocals
                for (int i = 0; i < ui->tabWidget->count(); ++i)
                {
                    if (ui->tabWidget->tabText(i).endsWith("*Lyrics"))  // do not switch if *Patter or Patter (using Lyrics tab for written Patter)
                    {
                        ui->tabWidget->setCurrentIndex(i);
                        break;
                    }
                }
            }
        }
        // If we just started playing, clear focus from all widgets
        if (QApplication::focusWidget() != NULL) {
            QApplication::focusWidget()->clearFocus();  // we don't want to continue editing the search fields after a STOP
                                                        //  or it will eat our keyboard shortcuts
        }
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));  // change PLAY to PAUSE
        ui->actionPlay->setText("Pause");
        currentState = kPlaying;
    }
    else {
        // TODO: we might want to restore focus here....
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
        ui->actionPlay->setText("Play");
        currentState = kPaused;
        ui->nowPlayingLabel->setText(currentSongTitle);  // restore the song title, if we were Flash Call mucking with it
    }
    if (ui->checkBoxStartOnPlay->isChecked()) {
        on_pushButtonCountUpTimerStartStop_clicked();
    }
}

// ----------------------------------------------------------------------
bool MainWindow::timerStopStartClick(QTimer *&timer, QPushButton *button)
{
    if (timer) {
        button->setText("Start");
        timer->stop();
        delete timer;
        timer = NULL;
    }
    else {
        button->setText("Stop");
        timer = new QTimer(this);
        timer->start(1000);
    }
    return NULL != timer;
}

// ----------------------------------------------------------------------
int MainWindow::updateTimer(qint64 timeZeroEpochMs, QLabel *label)
{
    QDateTime now(QDateTime::currentDateTime());
    qint64 timeNowEpochMs = now.currentMSecsSinceEpoch();
    int signedSeconds = (int)((timeNowEpochMs - timeZeroEpochMs) / 1000);
    int seconds = signedSeconds;
    char sign = ' ';

    if (seconds < 0) {
        sign = '-';
        seconds = -seconds;
    }

    stringstream ss;
    int hours = seconds / (60*60);
    int minutes = (seconds / 60) % 60;

    ss << sign;
    if (hours) {
        ss << hours << ":" << setw(2);
    }
    ss << setfill('0') << minutes << ":" << setw(2) << setfill('0') << (seconds % 60);
    string s(ss.str());
    label->setText(s.c_str());
    return signedSeconds;
}

// ----------------------------------------------------------------------
void MainWindow::on_pushButtonCountDownTimerStartStop_clicked()
{
    if (timerStopStartClick(timerCountDown,
                            ui->pushButtonCountDownTimerStartStop)) {
        on_pushButtonCountDownTimerReset_clicked();
        connect(timerCountDown, SIGNAL(timeout()), this, SLOT(timerCountDown_update()));
    }
}

// ----------------------------------------------------------------------

const qint64 timerJitter = 50;

void MainWindow::on_pushButtonCountDownTimerReset_clicked()
{
    QString offset(ui->lineEditCountDownTimer->text());

    int seconds = 0;
    int minutes = 0;
    bool found_colon = false;

    for (int i = 0; i < offset.length(); ++i) {
        int ch = offset[i].unicode();

        if (ch >= '0' && ch <= '9') {
            if (found_colon) {
                seconds *= 10;
                seconds += ch - '0';
            }
            else {
                minutes *= 10;
                minutes += ch - '0';
            }
        }
        else if (ch == ':') {
            found_colon = true;
        }
    }
    timeCountDownZeroMs = QDateTime::currentDateTime().currentMSecsSinceEpoch();
    timeCountDownZeroMs += (qint64)(minutes * 60 + seconds) * (qint64)(1000) + timerJitter;
    updateTimer(timeCountDownZeroMs, ui->labelCountDownTimer);
}

// ----------------------------------------------------------------------
void MainWindow::on_pushButtonCountUpTimerStartStop_clicked()
{
    if (timerStopStartClick(timerCountUp,
                            ui->pushButtonCountUpTimerStartStop)) {
        on_pushButtonCountUpTimerReset_clicked();
        connect(timerCountUp, SIGNAL(timeout()), this, SLOT(timerCountUp_update()));
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_pushButtonCountUpTimerReset_clicked()
{
    timeCountUpZeroMs = QDateTime::currentDateTime().currentMSecsSinceEpoch() + timerJitter;
    updateTimer(timeCountUpZeroMs, ui->labelCountUpTimer);
}

// ----------------------------------------------------------------------
void MainWindow::timerCountUp_update()
{
    updateTimer(timeCountUpZeroMs, ui->labelCountUpTimer);
}

// ----------------------------------------------------------------------
void MainWindow::timerCountDown_update()
{
    if (updateTimer(timeCountDownZeroMs, ui->labelCountDownTimer) >= 0
            && ui->checkBoxPlayOnEnd->isChecked()
            && currentState == kStopped) {
        on_playButton_clicked();
    }
}

int MainWindow::getSelectionRowForFilename(const QString &filePath)
{
    for (int i=0; i < ui->songTable->rowCount(); i++) {
        QString origPath = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
        if (filePath == origPath)
            return i;
    }
    return -1;
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

    saveCurrentSongSettings();
    // update the hidden pitch column
    ui->songTable->setSortingEnabled(false);
    int row = getSelectionRowForFilename(currentMP3filenameWithPath);
    if (row != -1)
    {
        ui->songTable->item(row, kPitchCol)->setText(QString::number(currentPitch)); // already trimmed()
    }
    ui->songTable->setSortingEnabled(true);
}

// ----------------------------------------------------------------------
void MainWindow::Info_Volume(void)
{
    int volSliderPos = ui->volumeSlider->value();
    if (volSliderPos == 0) {
        ui->currentVolumeLabel->setText("Mute");
    }
    else if (volSliderPos == 100) {
        ui->currentVolumeLabel->setText("MAX");
    }
    else {
        ui->currentVolumeLabel->setText(QString::number(volSliderPos)+"%");
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_volumeSlider_valueChanged(int value)
{
    int voltageLevelToSet = 100.0*pow(10.0,((((float)value*0.8)+20)/2.0 - 50)/20.0);
    if (value == 0) {
        voltageLevelToSet = 0;  // special case for slider all the way to the left (MUTE)
    }
    cBass.SetVolume(voltageLevelToSet);     // now logarithmic, 0 --> 0.01, 50 --> 0.1, 100 --> 1.0 (values * 100 for libbass)
    currentVolume = value;                  // this will be saved with the song (0-100)

    Info_Volume();  // update the slider text

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
        float desiredBPM = (float)value;            // desired BPM
        int newBASStempo = (int)(round(100.0*desiredBPM/baseBPM));
        cBass.SetTempo(newBASStempo);
        ui->currentTempoLabel->setText(QString::number(value) + " BPM (" + QString::number(newBASStempo) + "%)");
    }
    else {
        float basePercent = 100.0;                      // original detected percent
        float desiredPercent = (float)value;            // desired percent
        int newBASStempo = (int)(round(100.0*desiredPercent/basePercent));
        cBass.SetTempo(newBASStempo);
        ui->currentTempoLabel->setText(QString::number(value) + "%");
    }

    saveCurrentSongSettings();
    // update the hidden tempo column
    ui->songTable->setSortingEnabled(false);
    int row = getSelectionRowForFilename(currentMP3filenameWithPath);
    if (row != -1)
    {
        if (tempoIsBPM) {
            ui->songTable->item(row, kTempoCol)->setText(QString::number(value));
        }
        else {
            ui->songTable->item(row, kTempoCol)->setText(QString::number(value) + "%");
        }
    }
    ui->songTable->setSortingEnabled(true);

}

// ----------------------------------------------------------------------
void MainWindow::on_mixSlider_valueChanged(int value)
{
    int Lpercent, Rpercent;

    // NOTE: we're misleading the user a bit here.  It SOUNDS like it's doing the right thing,
    //   but under-the-covers we're implementing Constant Power, so the overall volume is (correctly)
    //   held constant.  From the user's perspective, the use of Constant Power means sin/cos(), which is
    //   not intuitive when converted to percent.  So, let's tell the user that it's all nicely linear
    //   (which will agree with the user's ear), and let's do the Right Thing Anyway internally.

    if (value < 0) {
        Lpercent = 100;
        Rpercent = 100 + value;
    } else {
        Rpercent = 100;
        Lpercent = 100 - value;
    }

    QString s = QString::number(Lpercent) + "%L/" + QString::number(Rpercent) + "%R ";

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
            case 4:
                s = "   " + s; // 4 + 3 = 7 chars
                break;
            case 5:
                s = "  " + s;  // 5 + 2 = 7 chars
                break;
            default:
                break;
        }
    }

    return s;
}

void InitializeSeekBar(MySlider *seekBar)
{
    seekBar->setMinimum(0);
    seekBar->setMaximum((int)cBass.FileLength-1); // NOTE: TRICKY, counts on == below
    seekBar->setTickInterval(10);  // 10 seconds per tick
}
void SetSeekBarPosition(MySlider *seekBar, int currentPos_i)
{
    seekBar->blockSignals(true); // setValue should NOT initiate a valueChanged()
    seekBar->setValue(currentPos_i);
    seekBar->blockSignals(false);
}
void SetSeekBarNoSongLoaded(MySlider *seekBar)
{
    seekBar->setMinimum(0);
    seekBar->setValue(0);
}

// ----------------------------------------------------------------------
void MainWindow::Info_Seekbar(bool forceSlider)
{
    static bool in_Info_Seekbar = false;
    if (in_Info_Seekbar) {
        return;
    }
    RecursionGuard recursion_guard(in_Info_Seekbar);

    if (songLoaded) {  // FIX: this needs to pay attention to the bool
        // FIX: this code doesn't need to be executed so many times.
        InitializeSeekBar(ui->seekBar);
        InitializeSeekBar(ui->seekBarCuesheet);

        cBass.StreamGetPosition();  // update cBass.Current_Position

        int currentPos_i = (int)cBass.Current_Position;
        if (forceSlider) {
            SetSeekBarPosition(ui->seekBar, currentPos_i);
            SetSeekBarPosition(ui->seekBarCuesheet, currentPos_i);

            int minScroll = ui->textBrowserCueSheet->verticalScrollBar()->minimum();
            int maxScroll = ui->textBrowserCueSheet->verticalScrollBar()->maximum();
            int maxSeekbar = ui->seekBar->maximum();  // NOTE: minSeekbar is always 0
            float fracSeekbar = (float)currentPos_i/(float)maxSeekbar;
            float targetScroll = 1.08 * fracSeekbar * (maxScroll - minScroll) + minScroll;  // FIX: this is heuristic and not right yet

            if (autoScrollLyricsEnabled) {
                // lyrics scrolling at the same time as the InfoBar
                ui->textBrowserCueSheet->verticalScrollBar()->setValue((int)targetScroll);
            }
        }
        int fileLen_i = (int)cBass.FileLength;

        if (currentPos_i == fileLen_i) {  // NOTE: TRICKY, counts on -1 above
            // avoids the problem of manual seek to max slider value causing auto-STOP
            if (!ui->actionContinuous_Play->isChecked()) {
                on_stopButton_clicked(); // pretend we pressed the STOP button when EOS is reached
            }
            else {
                // figure out which row is currently selected
                QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
                QModelIndexList selected = selectionModel->selectedRows();
                int row = -1;
                if (selected.count() == 1) {
                    // exactly 1 row was selected (good)
                    QModelIndex index = selected.at(0);
                    row = index.row();
                }
                else {
                    // more than 1 row or no rows at all selected (BAD)
                    return;
                }

                int maxRow = ui->songTable->rowCount() - 1;

                if (row != maxRow) {
                    on_nextSongButton_clicked(); // pretend we pressed the NEXT SONG button when EOS is reached, then:
                    on_playButton_clicked();     // pretend we pressed the PLAY button
                }
                else {
                    on_stopButton_clicked();     // pretend we pressed the STOP button when End of Playlist is reached
                }
            }
            return;
        }

        PreferencesManager prefsManager; // Will be using application information for correct location of your settings
        if (prefsManager.GetuseTimeRemaining()) {
            // time remaining in song
            ui->currentLocLabel->setText(position2String(fileLen_i - currentPos_i, true));  // pad on the left
        } else {
            // current position in song
            ui->currentLocLabel->setText(position2String(currentPos_i, true));              // pad on the left
        }
        ui->songLengthLabel->setText("/ " + position2String(fileLen_i));    // no padding

        // singing call sections
        if (songTypeNamesForSinging.contains(currentSongType) || songTypeNamesForCalled.contains(currentSongType)) {
            double introLength = timeToDouble(ui->lineEditIntroTime->text());
            double outroTime = timeToDouble(ui->lineEditOutroTime->text());
            double outroLength = fileLen_i-outroTime;

            int section;
            if (currentPos_i < introLength) {
                section = 0; // intro
            } else if (currentPos_i > outroTime) {
                section = 8;  // tag
            } else {
                section = 1.0 + 7.0*((currentPos_i - introLength)/(fileLen_i-(introLength+outroLength)));
                if (section > 8 || section < 0) {
                    section = 0; // needed for the time before fields are fully initialized
                }
            }

            QStringList sectionName;
            sectionName << "Intro" << "Opener" << "Figure 1" << "Figure 2"
                        << "Break" << "Figure 3" << "Figure 4" << "Closer" << "Tag";

            if (cBass.Stream_State == BASS_ACTIVE_PLAYING &&
                    (songTypeNamesForSinging.contains(currentSongType) || songTypeNamesForCalled.contains(currentSongType))) {
                // if singing call OR called, then tell the clock to show the section type
                analogClock->setSingingCallSection(sectionName[section]);
            } else {
                // else tell the clock that there isn't a section type
                analogClock->setSingingCallSection("");
            }

//            qDebug() << "currentPos:" << currentPos_i << ", fileLen: " << fileLen_i
//                     << "outroTime:" << outroTime
//                     << "introLength:" << introLength
//                     << "outroLength:" << outroLength
//                     << "section: " << section
//                     << "sectionName[section]: " << sectionName[section];
        }

#if defined(Q_OS_MAC) | defined(Q_OS_WIN32)
        // FLASH CALL FEATURE ======================================
        // TODO: do this only if patter? -------------------
        // TODO: right now this is hard-coded for Plus calls.  Need to add a preference to specify other levels (not
        //   mutually exclusive, either).
        int flashCallEverySeconds = 10;
        if (currentPos_i % flashCallEverySeconds == 0 && currentPos_i != 0) {
            // Now pick a new random number, but don't pick the same one twice in a row.
            // TODO: should really do a permutation over all the allowed calls, with no repeats
            //   but this should be good enough for now, if the number of calls is sufficiently
            //   large.
            randomizeFlashCall();
        }

        if (prefsManager.GetenableFlashCalls()) {
             if (cBass.Stream_State == BASS_ACTIVE_PLAYING && songTypeNamesForPatter.contains(currentSongType)) {
                 // if playing, and Patter type
                 // TODO: don't show any random calls until at least the end of the first N seconds
                 ui->nowPlayingLabel->setStyleSheet("QLabel { color : red; font-style: italic; }");
                 ui->nowPlayingLabel->setText(flashCalls[randCallIndex]);
             } else {
                 ui->nowPlayingLabel->setStyleSheet("QLabel { color : black; font-style: normal; }");
                 ui->nowPlayingLabel->setText(currentSongTitle);
             }
        }
#endif
    }
    else {
        SetSeekBarNoSongLoaded(ui->seekBar);
        SetSeekBarNoSongLoaded(ui->seekBarCuesheet);
    }
}

// --------------------------------1--------------------------------------

// blame https://stackoverflow.com/questions/4065378/qt-get-children-from-layout
bool isChildWidgetOfAnyLayout(QLayout *layout, QWidget *widget)
{
   if (layout == NULL or widget == NULL)
      return false;

   if (layout->indexOf(widget) >= 0)
      return true;

   foreach(QObject *o, layout->children())
   {
      if (isChildWidgetOfAnyLayout((QLayout*)o,widget))
         return true;
   }

   return false;
}

void setVisibleWidgetsInLayout(QLayout *layout, bool visible)
{
   if (layout == NULL)
      return;

   QWidget *pw = layout->parentWidget();
   if (pw == NULL)
      return;

   foreach(QWidget *w, pw->findChildren<QWidget*>())
   {
      if (isChildWidgetOfAnyLayout(layout,w))
          w->setVisible(visible);
   }
}

bool isVisibleWidgetsInLayout(QLayout *layout)
{
   if (layout == NULL)
      return false;

   QWidget *pw = layout->parentWidget();
   if (pw == NULL)
      return false;

   foreach(QWidget *w, pw->findChildren<QWidget*>())
   {
      if (isChildWidgetOfAnyLayout(layout,w))
          return w->isVisible();
   }
   return false;
}


void MainWindow::setCueSheetAdditionalControlsVisible(bool visible)
{
    setVisibleWidgetsInLayout(ui->verticalLayoutCueSheetAdditional, visible);
}

bool MainWindow::cueSheetAdditionalControlsVisible()
{
    return isVisibleWidgetsInLayout(ui->verticalLayoutCueSheetAdditional);
}

// --------------------------------1--------------------------------------


double timeToDouble(const QString &str, bool *ok)
{
    double t = -1;
    static QRegularExpression regex = QRegularExpression("((\\d+)\\:)?(\\d+(\\.\\d+)?)");
    QRegularExpressionMatch match = regex.match(str);
    if (match.hasMatch())
    {
        t = match.captured(3).toDouble(ok);
        if (*ok)
        {
            if (match.captured(2).length())
            {
                t += 60 * match.captured(2).toDouble(ok);
            }
        }
    }
//    qDebug() << "timeToDouble: " << str << ", out: " << t;
    return t;
}

void MainWindow::on_lineEditOutroTime_textChanged()
{
    bool ok = false;
    double position = timeToDouble(ui->lineEditOutroTime->text(),&ok);
    if (ok)
    {
        double length = cBass.FileLength;
        double t = position/length;

        ui->seekBarCuesheet->SetOutro((float)t);
        ui->seekBar->SetOutro((float)t);

//        qDebug() << "on_lineEditOutroTime_textChanged: " << position << ", t: " << t << ", length: " << length;

        on_loopButton_toggled(ui->actionLoop->isChecked()); // call this, so that cBass is told what the loop points are (or they are cleared)
    }
}

void MainWindow::on_lineEditIntroTime_textChanged()
{
    bool ok = false;
    double position = timeToDouble(ui->lineEditIntroTime->text(),&ok);

    if (ok)
    {
        double length = cBass.FileLength;
        double t = position/length;

        ui->seekBarCuesheet->SetIntro((float)t);
        ui->seekBar->SetIntro((float)t);

//        qDebug() << "on_lineEditIntroTime_textChanged: " << position << ", t: " << t << ", length: " << length;

        on_loopButton_toggled(ui->actionLoop->isChecked()); // call this, so that cBass is told what the loop points are (or they are cleared)
    }
}

void MainWindow::on_pushButtonClearTaughtCalls_clicked()
{
    QString danceProgram(ui->comboBoxCallListProgram->currentText());
    QMessageBox::StandardButton reply;

    reply = QMessageBox::question(this, "Clear Taught Calls",
                                  "Do you really want to clear all taught calls for the " +
                                   danceProgram +
                                  " dance program for the current session?",
                                  QMessageBox::Yes|QMessageBox::No);
  if (reply == QMessageBox::Yes) {
      songSettings.clearTaughtCalls(danceProgram);
      on_comboBoxCallListProgram_currentIndexChanged(ui->comboBoxCallListProgram->currentIndex());
  } else {
  }
}

// --------------------------------1--------------------------------------
void MainWindow::getCurrentPointInStream(double *tt, double *pos) {
    double position, length;

    if (cBass.Stream_State == BASS_ACTIVE_PLAYING) {
        // if we're playing, this is accurate to sub-second.
        cBass.StreamGetPosition(); // snapshot the current position
        position = cBass.Current_Position;
        length = cBass.FileLength;  // always use the value with maximum precision

//        qDebug() << "     PLAYING length: " << length << ", position: " << position;
    } else {
        // if we're NOT playing, this is accurate to the second.
        position = (double)(ui->seekBarCuesheet->value());
        length = ui->seekBarCuesheet->maximum();
//        qDebug() << "     NOT PLAYING length: " << length << ", position: " << position;
    }

    double t = (double)((double)position / (double)length);
//    qDebug() << "     T: " << t;

    // return values
    *tt = t;
    *pos = position;
}

// --------------------------------1--------------------------------------
void MainWindow::on_pushButtonSetIntroTime_clicked()
{
    double t, position;
    getCurrentPointInStream(&t, &position);

    ui->lineEditIntroTime->setText(doubleToTime(position));  // NOTE: This MUST be done first, because otherwise it messes up the blue bracket position due to conversions
    ui->seekBarCuesheet->SetIntro((float)t);  // after the events are done, do this.
    ui->seekBar->SetIntro((float)t);

    on_loopButton_toggled(ui->actionLoop->isChecked()); // then finally do this, so that cBass is told what the loop points are (or they are cleared)
}

// --------------------------------1--------------------------------------

void MainWindow::on_pushButtonSetOutroTime_clicked()
{
    double t, position;
    getCurrentPointInStream(&t, &position);

    ui->lineEditOutroTime->setText(doubleToTime(position));    // NOTE: This MUST be done first, because otherwise it messes up the blue bracket position due to conversions
    ui->seekBarCuesheet->SetOutro((float)t);  // after the events are done, do this.
    ui->seekBar->SetOutro((float)t);

    on_loopButton_toggled(ui->actionLoop->isChecked());  // then finally do this, so that cBass is told what the loop points are (or they are cleared)
}

// --------------------------------1--------------------------------------
void MainWindow::on_seekBarCuesheet_valueChanged(int value)
{
    on_seekBar_valueChanged(value);
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
    // FIX: bug when clearSearch is pressed, the order in the songTable can change.

    // figure out which row is currently selected
    QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
    }

    ui->labelSearch->setText("");
    ui->typeSearch->setText("");
    ui->titleSearch->setText("");

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
}

// ----------------------------------------------------------------------
void MainWindow::on_UIUpdateTimerTick(void)
{
    Info_Seekbar(true);

    // update the session coloring analog clock
    QTime time = QTime::currentTime();
    int theType = NONE;
//    qDebug() << "Stream_State:" << cBass.Stream_State; //FIX
    if (cBass.Stream_State == BASS_ACTIVE_PLAYING) {
        // if it's currently playing (checked once per second), then color this segment
        //   with the current segment type
        if (songTypeNamesForPatter.contains(currentSongType)) {
            theType = PATTER;
        }
        else if (songTypeNamesForSinging.contains(currentSongType)) {
            theType = SINGING;
        }
        else if (songTypeNamesForCalled.contains(currentSongType)) {
            theType = SINGING_CALLED;
        }
        else if (songTypeNamesForExtras.contains(currentSongType)) {
            theType = XTRAS;
        }
        else {
            theType = NONE;
        }

        analogClock->breakLengthAlarm = false;  // if playing, then we can't be in break
    } else if (cBass.Stream_State == BASS_ACTIVE_PAUSED) {
        // if we paused due to FADE, for example...
        // FIX: this could be factored out, it's used twice.
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
        ui->actionPlay->setText("Play");
        currentState = kPaused;
        ui->nowPlayingLabel->setText(currentSongTitle);  // restore the song title, if we were Flash Call mucking with it
    }

#ifndef DEBUGCLOCK
    analogClock->setSegment(time.hour(), time.minute(), time.second(), theType);  // always called once per second
#else
    analogClock->setSegment(0, time.minute(), time.second(), theType);  // DEBUG DEBUG DEBUG
#endif

    // ------------------------
    // Sounds associated with Tip and Break Timers (one-shots)
    newTimerState = analogClock->currentTimerState;

    if ((newTimerState & LONGTIPTIMEREXPIRED)&&!(oldTimerState & LONGTIPTIMEREXPIRED)) {
        // one-shot transitioned to Long Tip
        switch (tipLengthAlarmAction) {
        case 2: playSFX("long_tip"); break;
        default: break;
        }
    }

    if ((newTimerState & BREAKTIMEREXPIRED)&&!(oldTimerState & BREAKTIMEREXPIRED)) {
        // one-shot transitioned to End of Break
        switch (breakLengthAlarmAction) {
        case 2: playSFX("break_over"); break;
        default: break;
        }
    }
    oldTimerState = newTimerState;

    // -----------------------
    // about once a second, poll for volume changes
    newVolumeList = getCurrentVolumes();  // get the current state of the world
    qSort(newVolumeList);  // sort to allow direct comparison
    if(lastKnownVolumeList == newVolumeList){
        // This space intentionally blank.
    } else {
        on_newVolumeMounted();
    }
    lastKnownVolumeList = newVolumeList;
}

// ----------------------------------------------------------------------
void MainWindow::on_vuMeterTimerTick(void)
{
    float currentVolumeSlider = ui->volumeSlider->value();
    int level = cBass.StreamGetVuMeter();
    float levelF = (currentVolumeSlider/100.0)*((float)level)/32768.0;
    // TODO: iff music is playing.
    vuMeter->levelChanged(levelF/2.0,levelF,256);  // 10X/sec, update the vuMeter
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
        saveCurrentSongSettings();

        // as per http://doc.qt.io/qt-5.7/restoring-geometry.html
        QSettings settings;
        settings.setValue("lastCuesheetSavePath", lastCuesheetSavePath);
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowState", saveState());
        QMainWindow::closeEvent(event);
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
    msgBox.setText(QString("<p><h2>SquareDesk Player, V") + QString(VERSIONSTRING) + QString("</h2>") +
                   QString("<p>See our website at <a href=\"http://squaredesk.net\">squaredesk.net</a></p>") +
                   QString("Uses: <a href=\"http://www.un4seen.com/bass.html\">libbass</a>, ") +
                   QString("<a href=\"http://www.jobnik.org/?mnu=bass_fx\">libbass_fx</a>, ") +
                   QString("<a href=\"http://www.lynette.org/sd\">sd</a>, ") +
                   QString("<a href=\"http://cmusphinx.sourceforge.net\">PocketSphinx</a>, ") +
                   QString("<a href=\"http://tidy.sourceforge.net\">tidy-html5</a>, and ") +
                   QString("<a href=\"http://quazip.sourceforge.net\">QuaZIP</a>.") +
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

        MainWindow *maybeMainWindow = dynamic_cast<MainWindow *>(((QApplication *)Object)->activeWindow());
        if (maybeMainWindow == 0) {
            // if the PreferencesDialog is open, for example, do not dereference the NULL pointer (duh!).
            return QObject::eventFilter(Object,Event);
        }

        // if any of these widgets has focus, let them process the key
        //  otherwise, we'll process the key
        // UNLESS it's one of the search/timer edit fields and the ESC key is pressed (we must still allow
        //   stopping of music when editing a text field).  Sorry, can't use the SPACE BAR
        //   when editing a search field, because " " is a valid character to search for.
        //   If you want to do this, hit ESC to get out of edit search field mode, then SPACE.
        if ( !(ui->labelSearch->hasFocus() ||
               ui->typeSearch->hasFocus() ||
               ui->titleSearch->hasFocus() ||
               (ui->textBrowserCueSheet->hasFocus() && ui->toolButtonEditLyrics->isChecked()) ||
               ui->lineEditIntroTime->hasFocus() ||
               ui->lineEditOutroTime->hasFocus() ||
#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
               ui->lineEditCountDownTimer->hasFocus() ||
               ui->lineEditChoreographySearch->hasFocus() ||
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
               ui->songTable->isEditing() ||
               maybeMainWindow->console->hasFocus() )     ||
             ( (ui->labelSearch->hasFocus() ||
                ui->typeSearch->hasFocus() ||
                ui->titleSearch->hasFocus() ||
                ui->lineEditIntroTime->hasFocus() ||
                ui->lineEditOutroTime->hasFocus() ||
                ui->lineEditCountDownTimer->hasFocus()) &&
                (KeyEvent->key() == Qt::Key_Escape) )
           ) {
            // call handleKeypress on the Applications's active window ONLY if this is a MainWindow
//            qDebug() << "eventFilter YES:" << ui << currentWindowName << maybeMainWindow;
            return (maybeMainWindow->handleKeypress(KeyEvent->key(), KeyEvent->text()));
        }

    }
    return QObject::eventFilter(Object,Event);
}

// ----------------------------------------------------------------------
bool MainWindow::handleKeypress(int key, QString text)
{
    Q_UNUSED(text)
    QString tabTitle;

    if (inPreferencesDialog || !trapKeypresses || (prefDialog != NULL) || console->hasFocus()) {
        return false;
    }

    int currentTab = 0;

    switch (key) {

        case Qt::Key_Escape:
            // ESC is special:  it always gets you out of editing a search field or timer field, and it can
            //   also STOP the music (second press, worst case)

            oldFocusWidget = 0;  // indicates that we want NO FOCUS on restore
            if (QApplication::focusWidget() != NULL) {
                QApplication::focusWidget()->clearFocus();  // clears the focus from ALL widgets
            }
            oldFocusWidget = 0;  // indicates that we want NO FOCUS on restore, yes both of these are needed.

            // FIX: should we also stop editing of the songTable on ESC?

            if (ui->labelSearch->text() != "" || ui->typeSearch->text() != "" || ui->titleSearch->text() != "") {
                // clear the search fields, if there was something in them.  (First press of ESCAPE).
                ui->labelSearch->setText("");
                ui->typeSearch->setText("");
                ui->titleSearch->setText("");
            } else {
                // if the search fields were already clear, then this is the second press of ESCAPE (or the first press
                //   of ESCAPE when the search function was not being used).  So, ONLY NOW do we Stop Playback.
                // So, GET ME OUT OF HERE is now "ESC ESC", or "Hit ESC a couple of times".
                //    and, CLEAR SEARCH is just ESC (or click on the Clear Search button).
                if (currentState == kPlaying) {
                    on_playButton_clicked();  // we were playing, so PAUSE now.
                }
            }

            cBass.StopAllSoundEffects();  // and, it also stops ALL sound effects
            break;

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
            on_warningLabel_clicked();  // also reset the Patter Timer to zero
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
            ui->tempoSlider->setValue(ui->tempoSlider->value() + 1);
            on_tempoSlider_valueChanged(ui->tempoSlider->value());
            break;
        case Qt::Key_Minus:
            ui->tempoSlider->setValue(ui->tempoSlider->value() - 1);
            on_tempoSlider_valueChanged(ui->tempoSlider->value());
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

        case Qt::Key_PageDown:
            // only move the scrolled Lyrics area, if the Lyrics tab is currently showing, and lyrics are loaded
            //   or if Patter is currently showing and patter is loaded
            tabTitle = ui->tabWidget->tabText(ui->tabWidget->currentIndex());
            if (tabTitle.endsWith("*Lyrics") || tabTitle.endsWith("*Patter")) {
                ui->textBrowserCueSheet->verticalScrollBar()->setValue(ui->textBrowserCueSheet->verticalScrollBar()->value() + 200);
            }
            break;

        case Qt::Key_PageUp:
            // only move the scrolled Lyrics area, if the Lyrics tab is currently showing, and lyrics are loaded
            //   or if Patter is currently showing and patter is loaded
            tabTitle = ui->tabWidget->tabText(ui->tabWidget->currentIndex());
            if (tabTitle.endsWith("*Lyrics") || tabTitle.endsWith("*Patter")) {
                ui->textBrowserCueSheet->verticalScrollBar()->setValue(ui->textBrowserCueSheet->verticalScrollBar()->value() - 200);
            }
            break;

        case Qt::Key_Y:
            cBass.FadeOutAndPause();
            break;

        case Qt::Key_L:
            on_loopButton_toggled(!ui->actionLoop->isChecked());  // toggle it
            break;

        case Qt::Key_T:
            currentTab = ui->tabWidget->currentIndex();
            if (currentTab == 0) {
                // if Music tab active, go to Lyrics tab
                ui->tabWidget->setCurrentIndex(1);
            } else if (currentTab == 1) {
                // if Lyrics tab active, go to Music tab
                ui->tabWidget->setCurrentIndex(0);
            } else {
                // if currently some other tab, just go to the Music tab
                ui->tabWidget->setCurrentIndex(0);
            }
            break;

        default:
//            qDebug() << "unhandled key:" << key;
            break;
    }

    Info_Seekbar(true);
    return true;
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


struct FilenameMatchers {
    QRegularExpression regex;
    int title_match;
    int label_match;
    int number_match;
    int additional_label_match;
    int additional_title_match;
};

struct FilenameMatchers *getFilenameMatchersForType(enum SongFilenameMatchingType songFilenameFormat)
{
    static struct FilenameMatchers best_guess_matches[] = {
        { QRegularExpression("^(.*) - ([A-Z]+[\\- ]\\d+)( *-?[VMA-C]|\\-\\d+)?$"), 1, 2, -1, 3, -1 },
        { QRegularExpression("^([A-Z]+[\\- ]\\d+)(-?[VvMA-C]?) - (.*)$"), 3, 1, -1, 2, -1 },
        { QRegularExpression("^([A-Z]+ ?\\d+)([MV]?)[ -]+(.*)$/"), 3, 1, -1, 2, -1 },
        { QRegularExpression("^([A-Z]?[0-9][A-Z]+[\\- ]?\\d+)([MV]?)[ -]+(.*)$"), 3, 1, -1, 2, -1 },
        { QRegularExpression("^(.*) - ([A-Z]{1,5}+)[\\- ](\\d+)( .*)?$"), 1, 2, 3, -1, 4 },
        { QRegularExpression("^([A-Z]+ ?\\d+)([ab])?[ -]+(.*)$/"), 3, 1, -1, 2, -1 },
        { QRegularExpression("^([A-Z]+\\-\\d+)\\-(.*)/"), 2, 1, -1, -1, -1 },
//    { QRegularExpression("^(\\d+) - (.*)$"), 2, -1, -1, -1, -1 },         // first -1 prematurely ended the search (typo?)
//    { QRegularExpression("^(\\d+\\.)(.*)$"), 2, -1, -1, -1, -1 },         // first -1 prematurely ended the search (typo?)
        { QRegularExpression("^(\\d+)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },  // e.g. "123 - Chicken Plucker"
        { QRegularExpression("^(\\d+\\.)(.*)$"), 2, 1, -1, -1, -1 },            // e.g. "123.Chicken Plucker"
//        { QRegularExpression("^(.*?) - (.*)$"), 2, 1, -1, -1, -1 },           // ? is a non-greedy match (So that "A - B - C", first group only matches "A")
        { QRegularExpression("^([A-Z]{1,5}+[\\- ]*\\d+[A-Z]*)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 }, // e.g. "ABC 123-Chicken Plucker"
        { QRegularExpression("^([A-Z0-9]{1,5}+)\\s*(\\d+)([a-zA-Z]{1,2})?\\s*-\\s*(.*?)\\s*(\\(.*\\))?$"), 4, 1, 2, 3, 5 }, // SIR 705b - Papa Was A Rollin Stone (Instrumental).mp3
        { QRegularExpression("^([A-Z0-9]{1,5}+)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },    // e.g. "POP - Chicken Plucker" (if it has a dash but fails all other tests,
        { QRegularExpression("^(.*?)\\s*\\-\\s*([A-Z]{1,5})(\\d{1,5})\\s*(\\(.*\\))?$"), 1, 2, 3, -1, 4 },    // e.g. "A Summer Song - CHIC3002 (female vocals)
        { QRegularExpression("^(.*?)\\s*\\-\\s*([A-Za-z]{1,7})-(\\d{1,5})(\\-?([AB]))?$"), 1, 2, 3, 5, -1 },    // e.g. "Paper Doll - Windsor-4936B"

        { QRegularExpression(), -1, -1, -1, -1, -1 }
    };
    static struct FilenameMatchers label_first_matches[] = {
        { QRegularExpression("^(.*)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },    // e.g. "ABC123X - Chicken Plucker"
        { QRegularExpression(), -1, -1, -1, -1, -1 }
    };
    static struct FilenameMatchers filename_first_matches[] = {
        { QRegularExpression("^(.*)\\s*-\\s*(.*)$"), 1, 2, -1, -1, -1 },    // e.g. "Chicken Plucker - ABC123X"
        { QRegularExpression(), -1, -1, -1, -1, -1 }
    };

    switch (songFilenameFormat) {
        default:
        case SongFilenameLabelDashName :
            return label_first_matches;
        case SongFilenameNameDashLabel :
            return filename_first_matches;
        case SongFilenameBestGuess :
            return best_guess_matches;
    }
}


bool MainWindow::breakFilenameIntoParts(const QString &s,
                                        QString &label, QString &labelnum,
                                        QString &labelnum_extra,
                                        QString &title, QString &shortTitle )
{
    bool foundParts = true;
    int match_num = 0;
    struct FilenameMatchers *matches = getFilenameMatchersForType(songFilenameFormat);

    for (match_num = 0;
         matches[match_num].label_match >= 0
             && matches[match_num].title_match >= 0;
         ++match_num) {
        QRegularExpressionMatch match = matches[match_num].regex.match(s);
        if (match.hasMatch()) {
            if (matches[match_num].label_match >= 0) {
                label = match.captured(matches[match_num].label_match);
            }
            if (matches[match_num].title_match >= 0) {
                title = match.captured(matches[match_num].title_match);
                shortTitle = title;
            }
            if (matches[match_num].number_match >= 0) {
                labelnum = match.captured(matches[match_num].number_match);
            }
            if (matches[match_num].additional_label_match >= 0) {
                labelnum_extra = match.captured(matches[match_num].additional_label_match);
            }
            if (matches[match_num].additional_title_match >= 0
                && !match.captured(matches[match_num].additional_title_match).isEmpty()) {
                title += " " + match.captured(matches[match_num].additional_title_match);
            }
            break;
        } else {
//                qDebug() << s << "didn't match" << matches[match_num].regex;
        }
    }
    if (!(matches[match_num].label_match >= 0
          && matches[match_num].title_match >= 0)) {
        label = "";
        title = s;
        foundParts = false;
    }

    label = label.simplified();

    if (labelnum.length() == 0)
    {
        static QRegularExpression regexLabelPlusNum = QRegularExpression("^(\\w+)[\\- ](\\d+)(\\w?)$");
        QRegularExpressionMatch match = regexLabelPlusNum.match(label);
        if (match.hasMatch())
        {
            label = match.captured(1);
            labelnum = match.captured(2);
            if (labelnum_extra.length() == 0)
            {
                labelnum_extra = match.captured(3);
            }
            else
            {
                labelnum = labelnum + match.captured(3);
            }
        }
    }
    labelnum.simplified();
    title = title.simplified();
    shortTitle = shortTitle.simplified();

    return foundParts;
}

class CuesheetWithRanking {
public:
    QString filename;
    QString name;
    int score;
};

static bool CompareCuesheetWithRanking(CuesheetWithRanking *a, CuesheetWithRanking *b)
{
    return a->score > b->score;
}

QString MainWindow::tidyHTML(QString cuesheet) {

    // first get rid of <L> and </L>.  Those are ILLEGAL.
    cuesheet.replace("<L>","",Qt::CaseInsensitive).replace("</L>","",Qt::CaseInsensitive);

    // then get rid of <NOBR> and </NOBR>, NOT SUPPORTED BY W3C.
    cuesheet.replace("<NOBR>","",Qt::CaseInsensitive).replace("</NOBR>","",Qt::CaseInsensitive);

    // and &nbsp; too...let the layout engine do its thing.
    cuesheet.replace("&nbsp;"," ");

    // convert to a c_string, for HTML-TIDY
    char* tidyInput;
    string csheet = cuesheet.toStdString();
    tidyInput = new char [csheet.size()+1];
    strcpy( tidyInput, csheet.c_str() );

//    qDebug().noquote() << "\n***** TidyInput:\n" << QString((char *)tidyInput);

    TidyBuffer output;// = {0};
    TidyBuffer errbuf;// = {0};
    tidyBufInit(&output);
    tidyBufInit(&errbuf);
    int rc = -1;
    Bool ok;

    // TODO: error handling here...using GOTO!

    TidyDoc tdoc = tidyCreate();
    ok = tidyOptSetBool( tdoc, TidyHtmlOut, yes );  // Convert to XHTML
    if (ok) {
        ok = tidyOptSetBool( tdoc, TidyUpperCaseTags, yes );  // span -> SPAN
    }
//    if (ok) {
//        ok = tidyOptSetInt( tdoc, TidyUpperCaseAttrs, TidyUppercaseYes );  // href -> HREF
//    }
    if (ok) {
        ok = tidyOptSetBool( tdoc, TidyDropEmptyElems, yes );  // Discard empty elements
    }
    if (ok) {
        ok = tidyOptSetBool( tdoc, TidyDropEmptyParas, yes );  // Discard empty p elements
    }
    if (ok) {
        ok = tidyOptSetInt( tdoc, TidyIndentContent, TidyYesState );  // text/block level content indentation
    }
    if (ok) {
        ok = tidyOptSetInt( tdoc, TidyWrapLen, 150 );  // text/block level content indentation
    }
    if (ok) {
        ok = tidyOptSetBool( tdoc, TidyMark, no);  // do not add meta element indicating tidied doc
    }
    if (ok) {
        ok = tidyOptSetBool( tdoc, TidyLowerLiterals, yes);  // Folds known attribute values to lower case
    }
    if (ok) {
        ok = tidyOptSetInt( tdoc, TidySortAttributes, TidySortAttrAlpha);  // Sort attributes
    }
    if ( ok )
        rc = tidySetErrorBuffer( tdoc, &errbuf );      // Capture diagnostics
    if ( rc >= 0 )
        rc = tidyParseString( tdoc, tidyInput );           // Parse the input
    if ( rc >= 0 )
        rc = tidyCleanAndRepair( tdoc );               // Tidy it up!
    if ( rc >= 0 )
        rc = tidyRunDiagnostics( tdoc );               // Kvetch
    if ( rc > 1 )                                    // If error, force output.
        rc = ( tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1 );
    if ( rc >= 0 )
        rc = tidySaveBuffer( tdoc, &output );          // Pretty Print

    QString cuesheet_tidied;
    if ( rc >= 0 )
    {
        if ( rc > 0 ) {
//            qDebug().noquote() << "\n***** Diagnostics:" << QString((char*)errbuf.bp);
//        qDebug().noquote() << "\n***** TidyOutput:\n" << QString((char*)output.bp);
        }
        cuesheet_tidied = QString((char*)output.bp);
    }
    else {
        qDebug() << "Severe error:" << rc;
    }

    tidyBufFree( &output );
    tidyBufFree( &errbuf );
    tidyRelease( tdoc );

    // get rid of TIDY cruft
//    cuesheet_tidied.replace("<META NAME=\"generator\" CONTENT=\"HTML Tidy for HTML5 for Mac OS X version 5.5.31\">","");

    return(cuesheet_tidied);
}

// ------------------------
QString MainWindow::postProcessHTMLtoSemanticHTML(QString cuesheet) {
    // margin-top:12px;
    // margin-bottom:12px;
    // margin-left:0px;
    // margin-right:0px;
    // -qt-block-indent:0;
    // text-indent:0px;
    // line-height:100%;
    // KEEP: background-color:#ffffe0;
    cuesheet
            .replace(QRegExp("margin-top:[0-9]+px;"), "")
            .replace(QRegExp("margin-bottom:[0-9]+px;"), "")
            .replace(QRegExp("margin-left:[0-9]+px;"), "")
            .replace(QRegExp("margin-right:[0-9]+px;"), "")
            .replace(QRegExp("text-indent:[0-9]+px;"), "")
            .replace(QRegExp("line-height:[0-9]+%;"), "")
            .replace(QRegExp("-qt-block-indent:[0-9]+;"), "")
            ;

    // get rid of unwanted QTextEdit tags
    QRegExp styleRegExp("(<STYLE.*</STYLE>)|(<META.*>)");
    styleRegExp.setMinimal(true);
    styleRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    cuesheet.replace(styleRegExp,"");  // don't be greedy

//    qDebug().noquote() << "***** postProcess 1: " << cuesheet;
    QString cuesheet3 = tidyHTML(cuesheet);

    // now the semantic replacement.
    // assumes that QTextEdit spits out spans in a consistent way
    // TODO: allow embedded NL (due to line wrapping)
    // NOTE: background color is optional here, because I got rid of the the spec for it in BODY
    cuesheet3.replace(QRegExp("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:x-large; color:#ff0000;[\\s\n]*(background-color:#ffffe0;)*\">"),
                             "<SPAN class=\"hdr\">");
    cuesheet3.replace(QRegExp("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:large; color:#000000; background-color:#ffc0cb;\">"),  // background-color required for lyrics
                             "<SPAN class=\"lyrics\">");
    cuesheet3.replace(QRegExp("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:medium; color:#60c060;[\\s\n]*(background-color:#ffffe0;)*\">"),
                             "<SPAN class=\"label\">");
    cuesheet3.replace(QRegExp("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:medium; color:#0000ff;[\\s\n]*(background-color:#ffffe0;)*\">"),
                             "<SPAN class=\"artist\">");
    cuesheet3.replace(QRegExp("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:x-large;[\\s\n]*(font-weight:600;)*[\\s\n]*color:#000000;[\\s\n]*(background-color:#ffffe0;)*\">"),
                             "<SPAN class=\"title\">");
    cuesheet3.replace(QRegExp("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:large;[\\s\n]*font-weight:600;[\\s\n]*color:#000000;[\\s\n]*(background-color:#ffffe0;)*\">"),
                                     "<SPAN style=\"font-weight: Bold;\">");

    cuesheet3.replace("<P style=\"\">","<P>");
    cuesheet3.replace("<P style=\"background-color:#ffffe0;\">","<P>");  // background color already defaults via the BODY statement
    cuesheet3.replace("<BODY bgcolor=\"#FFFFE0\" style=\"font-family:'.SF NS Text'; font-size:13pt; font-weight:400; font-style:normal;\">","<BODY>");  // must go back to USER'S choices in cuesheet2.css

    // multi-step replacement
//    qDebug().noquote() << "***** REALLY BEFORE:\n" << cuesheet3;
    //      <SPAN style="font-family:'Verdana'; font-size:large; color:#000000; background-color:#ffffe0;">
    cuesheet3.replace(QRegExp("\"font-family:'Verdana'; font-size:large; color:#000000;( background-color:#ffffe0;)*\""),"\"XXXXX\""); // must go back to USER'S choices in cuesheet2.css
//    qDebug().noquote() << "***** BEFORE:\n" << cuesheet3;
    cuesheet3.replace(QRegExp("<SPAN style=[\\s\n]*\"XXXXX\">"),"<SPAN>");
//    qDebug().noquote() << "***** AFTER:\n" << cuesheet3;

    // now replace null SPAN tags
    QRegExp nullStyleRegExp("<SPAN>(.*)</SPAN>");
    nullStyleRegExp.setMinimal(true);
    nullStyleRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    cuesheet3.replace(nullStyleRegExp,"\\1");  // don't be greedy, and replace <SPAN>foo</SPAN> with foo

    // TODO: bold -- <SPAN style="font-family:'Verdana'; font-size:large; font-weight:600; color:#000000;">
    // TODO: italic -- TBD
    // TODO: get rid of style="background-color:#ffffe0;, yellowish, put at top once
    // TODO: get rid of these, use body: <SPAN style="font-family:'Verdana'; font-size:large; color:#000000;">

    // put the <link rel="STYLESHEET" type="text/css" href="cuesheet2.css"> back in
    if (!cuesheet3.contains("<link",Qt::CaseInsensitive)) {
//        qDebug() << "Putting the <LINK> back in...";
        cuesheet3.replace("</TITLE>","</TITLE><LINK rel=\"STYLESHEET\" type=\"text/css\" href=\"cuesheet2.css\">");
    }
    // tidy it one final time before writing it to a file (gets rid of blank SPAN's, among other things)
    QString cuesheet4 = tidyHTML(cuesheet3);


//    qDebug().noquote() << "***** postProcess 2: " << cuesheet4;
    return(cuesheet4);
}

void MainWindow::loadCuesheet(const QString &cuesheetFilename)
{
    QUrl cuesheetUrl(QUrl::fromLocalFile(cuesheetFilename));  // NOTE: can contain HTML that references a customer's cuesheet2.css
    if (cuesheetFilename.endsWith(".txt")) {
        // text files are read in, converted to HTML, and sent to the Lyrics tab
        QFile f1(cuesheetFilename);
        f1.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream in(&f1);
        QString html = txtToHTMLlyrics(in.readAll(), cuesheetFilename);
        ui->textBrowserCueSheet->setText(html);
        f1.close();
    }
    else if (cuesheetFilename.endsWith(".mp3")) {
        QString embeddedID3Lyrics = loadLyrics(cuesheetFilename);
        if (embeddedID3Lyrics != "") {
            QString html(txtToHTMLlyrics(embeddedID3Lyrics, cuesheetFilename));  // embed CSS, if found, since USLT is plain text
            ui->textBrowserCueSheet->setHtml(html);
        }
    } else {
        // read the CSS file (if any)
        PreferencesManager prefsManager;
        QString musicDirPath = prefsManager.GetmusicPath();
        QString lyricsDir = musicDirPath + "/lyrics";

        // if the lyrics directory doesn't exist, create it
        QDir dir(lyricsDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        // now check for the cuesheet2.css file... ---------------
        QString cuesheetCSSDestPath = lyricsDir + "/cuesheet2.css";
        QFile cuesheetCSSDestFile(cuesheetCSSDestPath);

#if defined(Q_OS_MAC)
        QString appPath = QApplication::applicationFilePath();
        QString cuesheet2SrcPath = appPath + "/Contents/Resources/cuesheet2.css";
        cuesheet2SrcPath.replace("Contents/MacOS/SquareDeskPlayer/","");
#elif defined(Q_OS_WIN32)
        // TODO: There has to be a better way to do this.
        QString appPath = QApplication::applicationFilePath();
        QString cuesheet2SrcPath = appPath + "/cuesheet2.css";
        cuesheet2SrcPath.replace("SquareDeskPlayer.exe/","");
#else
        qWarning() << "Warning: cuesheet2 path is almost certainly wrong here for Linux.";
        QString appPath = QApplication::applicationFilePath();
        QString cuesheet2SrcPath = appPath + "/cuesheet2.css";
        cuesheet2SrcPath.replace("SquareDeskPlayer/","");
#endif

        if (!cuesheetCSSDestFile.exists()) {
            // copy in a cuesheet2 file, if there's not one there already
            QFile::copy(cuesheet2SrcPath, cuesheetCSSDestPath);  // copy one in
        }

        // Now, open it. ---------------
        QFile css(lyricsDir + "/cuesheet2.css");  // cuesheet2.css is located in the LYRICS directory (NOTE: GLOBAL! All others are now IGNORED)

        QString cssString;
        if ( css.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in1(&css);
            cssString = in1.readAll();  // read the entire CSS file, if it exists
            css.close();
        }

        // read in the HTML for the cuesheet

        QFile f1(cuesheetFilename);
        QString cuesheet;
        if ( f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f1);
            cuesheet = in.readAll();  // read the entire CSS file, if it exists

            if (cuesheet.contains("charset=windows-1252") || cuesheetFilename.contains("GP 956")) {  // WARNING: HACK HERE
                // this is very likely to be an HTML file converted from MS WORD,
                //   and it still uses windows-1252 encoding.

                f1.seek(0);  // go back to the beginning of the file

                QByteArray win1252bytes(f1.readAll());  // and read it again (as bytes this time)
                QTextCodec *codec = QTextCodec::codecForName("windows-1252");  // FROM win-1252 bytes
                cuesheet = codec->toUnicode(win1252bytes);                     // TO Unicode QString
            }

//            qDebug() << "Cuesheet: " << cuesheet;
            cuesheet.replace("\xB4","'");  // replace wacky apostrophe, which doesn't display well in QEditText
            // NOTE: o-umlaut is already translated (incorrectly) here to \xB4, too.  There's not much we
            //   can do with non UTF-8 HTML files that aren't otherwise marked as to encoding.

            // set the CSS
//            qDebug().noquote() << "***** CSS:\n" << cssString;
            ui->textBrowserCueSheet->document()->setDefaultStyleSheet(cssString);

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
            // HTML-TIDY IT ON INPUT *********
            QString cuesheet_tidied = tidyHTML(cuesheet);
#else
            QString cuesheet_tidied = cuesheet;  // LINUX, WINDOWS
#endif

            // ----------------------
            // set the HTML for the cuesheet itself (must set CSS first)
//            ui->textBrowserCueSheet->setHtml(cuesheet);
            ui->textBrowserCueSheet->setHtml(cuesheet_tidied);
            f1.close();
        }

    }
    ui->textBrowserCueSheet->document()->setModified(false);
}


QStringList splitIntoWords(const QString &str)
{
    static QRegExp regexNotAlnum(QRegExp("\\W+"));
    static QRegularExpression regexLettersAndNumbers("^([A-Z]+)([0-9].*)$");
    static QRegularExpression regexNumbersAndLetters("^([0-9]+)([A-Z].*)$");
    QStringList words = str.split(regexNotAlnum);
    for (int i = 0; i < words.length(); ++i)
    {
        bool splitFurther = true;

        while (splitFurther)
        {
            splitFurther = false;
            QRegularExpressionMatch match(regexLettersAndNumbers.match(words[i]));
            if (match.hasMatch())
            {
                words.append(match.captured(1));
                words[i] = match.captured(2);
                splitFurther = true;
            }
            match = regexNumbersAndLetters.match(words[i]);
            if (match.hasMatch())
            {
                splitFurther = true;
                words.append(match.captured(1));
                words[i] = match.captured(2);
            }
        }
    }
    words.sort(Qt::CaseInsensitive);
    return words;
}

int compareSortedWordListsForRelevance(const QStringList &l1, const QStringList l2)
{
    int i1 = 0, i2 = 0;
    int score = 0;

    while (i1 < l1.length() &&  i2 < l2.length())
    {
        int comp = l1[i1].compare(l2[i2], Qt::CaseInsensitive);
        if (comp == 0)
        {
            ++score;
            ++i1;
            ++i2;
        }
        else if (comp < 0)
        {
            ++i1;
        }
        else
        {
            ++i2;
        }
    }
    if (l1.length() >= 2 && l2.length() >= 2 &&
        (
            (score > ((l1.length() + l2.length()) / 4))
            || (score >= l1.length())
            || (score >= l2.length())
            )
        )
    {
        QString s1 = l1.join("-");
        QString s2 = l2.join("-");
//        qDebug() << "Score" << score << " / " << l1.length() << "/" << l2.length() << " s1: " << s1 << " s2: " << s2;
        return score * 500 - 100 * (abs(l1.length()) - l2.length());
    }
    else
        return 0;
}

// TODO: the match needs to be a little fuzzier, since RR103B - Rocky Top.mp3 needs to match RR103 - Rocky Top.html
void MainWindow::findPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets)
{
    QFileInfo mp3FileInfo(MP3Filename);
    QString mp3CanonicalPath = mp3FileInfo.canonicalPath();
    QString mp3CompleteBaseName = mp3FileInfo.completeBaseName();
    QString mp3Label = "";
    QString mp3Labelnum = "";
    QString mp3Labelnum_short = "";
    QString mp3Labelnum_extra = "";
    QString mp3Title = "";
    QString mp3ShortTitle = "";
    breakFilenameIntoParts(mp3CompleteBaseName, mp3Label, mp3Labelnum, mp3Labelnum_extra, mp3Title, mp3ShortTitle);
    QList<CuesheetWithRanking *> possibleRankings;

    QStringList mp3Words = splitIntoWords(mp3CompleteBaseName);
    mp3Labelnum_short = mp3Labelnum;
    while (mp3Labelnum_short.length() > 0 && mp3Labelnum_short[0] == '0')
    {
        mp3Labelnum_short.remove(0,1);
    }

    QList<QString> extensions;
    QString dot(".");
    for (size_t i = 0; i < sizeof(cuesheet_file_extensions) / sizeof(*cuesheet_file_extensions); ++i)
    {
        extensions.append(dot + cuesheet_file_extensions[i]);
    }

    QListIterator<QString> iter(*pathStack);
    while (iter.hasNext()) {
        QString s = iter.next();
        int extensionIndex = 0;

        // Is this a file extension we recognize as a cuesheet file?
        QListIterator<QString> extensionIterator(extensions);
        bool foundExtension = false;
        while (extensionIterator.hasNext())
        {
            extensionIndex++;
            QString extension(extensionIterator.next());
            if (s.endsWith(extension))
            {
                foundExtension = true;
                break;
            }
        }
        if (!foundExtension)
            continue;

        QStringList sl1 = s.split("#!#");
        QString type = sl1[0];  // the type (of original pathname, before following aliases)
        QString filename = sl1[1];  // everything else

        QFileInfo fi(filename);

        if (fi.canonicalPath() == musicRootPath && type.right(1) != "*") {
            // e.g. "/Users/mpogue/__squareDanceMusic/C 117 - Bad Puppy (Patter).mp3" --> NO TYPE PRESENT and NOT a guest song
            type = "";
        }

        QString label = "";
        QString labelnum = "";
        QString title = "";
        QString labelnum_extra;
        QString shortTitle = "";


        QString completeBaseName = fi.completeBaseName(); // e.g. "/Users/mpogue/__squareDanceMusic/patter/RIV 307 - Going to Ceili (Patter).mp3" --> "RIV 307 - Going to Ceili (Patter)"
        breakFilenameIntoParts(completeBaseName, label, labelnum, labelnum_extra, title, shortTitle);
        QStringList words = splitIntoWords(completeBaseName);
        QString labelnum_short = labelnum;
        while (labelnum_short.length() > 0 && labelnum_short[0] == '0')
        {
            labelnum_short.remove(0,1);
        }

//        qDebug() << "Comparing: " << completeBaseName << " to " << mp3CompleteBaseName;
//        qDebug() << "           " << title << " to " << mp3Title;
//        qDebug() << "           " << shortTitle << " to " << mp3ShortTitle;
//        qDebug() << "    label: " << label << " to " << mp3Label <<
//            " and num " << labelnum << " to " << mp3Labelnum <<
//            " short num " << labelnum_short << " to " << mp3Labelnum_short;
//        qDebug() << "    title: " << mp3Title << " to " << QString(label + "-" + labelnum);
        int score = 0;
        // Minimum criteria:
        if (completeBaseName.compare(mp3CompleteBaseName, Qt::CaseInsensitive) == 0
            || title.compare(mp3Title, Qt::CaseInsensitive) == 0
            || (shortTitle.length() > 0
                && shortTitle.compare(mp3ShortTitle, Qt::CaseInsensitive) == 0)
            || (labelnum_short.length() > 0 && label.length() > 0
                &&  labelnum_short.compare(mp3Labelnum_short, Qt::CaseInsensitive) == 0
                && label.compare(mp3Label, Qt::CaseInsensitive) == 0
                )
            || (labelnum.length() > 0 && label.length() > 0
                && mp3Title.length() > 0
                && mp3Title.compare(label + "-" + labelnum, Qt::CaseInsensitive) == 0)
            )
        {

            score = extensionIndex
                + (mp3CanonicalPath.compare(fi.canonicalPath(), Qt::CaseInsensitive) == 0 ? 10000 : 0)
                + (mp3CompleteBaseName.compare(fi.completeBaseName(), Qt::CaseInsensitive) == 0 ? 1000 : 0)
                + (title.compare(mp3Title, Qt::CaseInsensitive) == 0 ? 100 : 0)
                + (shortTitle.compare(mp3ShortTitle, Qt::CaseInsensitive) == 0 ? 50 : 0)
                + (labelnum.compare(mp3Labelnum, Qt::CaseInsensitive) == 0 ? 10 : 0)
                + (labelnum_short.compare(mp3Labelnum_short, Qt::CaseInsensitive) == 0 ? 7 : 0)
                + (mp3Label.compare(mp3Label, Qt::CaseInsensitive) == 0 ? 5 : 0);

            CuesheetWithRanking *cswr = new CuesheetWithRanking();
            cswr->filename = filename;
            cswr->name = completeBaseName;
            cswr->score = score;
            possibleRankings.append(cswr);
        } /* end of if we minimally included this cuesheet */
        else if ((score = compareSortedWordListsForRelevance(mp3Words, words)) > 0)
        {
            CuesheetWithRanking *cswr = new CuesheetWithRanking();
            cswr->filename = filename;
            cswr->name = completeBaseName;
            cswr->score = score;
            possibleRankings.append(cswr);
        }
    } /* end of looping through all files we know about */

    QString mp3Lyrics = loadLyrics(MP3Filename);
    if (mp3Lyrics.length())
    {
        possibleCuesheets.append(MP3Filename);
    }

    qSort(possibleRankings.begin(), possibleRankings.end(), CompareCuesheetWithRanking);
    QListIterator<CuesheetWithRanking *> iterRanked(possibleRankings);
    while (iterRanked.hasNext())
    {
        CuesheetWithRanking *cswr = iterRanked.next();
        possibleCuesheets.append(cswr->filename);
        delete cswr;
    }

}


void MainWindow::loadCuesheets(const QString &MP3FileName, const QString preferredCuesheet)
{
    hasLyrics = false;

    QString HTML;

    QStringList possibleCuesheets;
    findPossibleCuesheets(MP3FileName, possibleCuesheets);

    int defaultCuesheetIndex = 0;


    QString firstCuesheet(preferredCuesheet);
    ui->comboBoxCuesheetSelector->clear();

    foreach (const QString &cuesheet, possibleCuesheets)
    {
        RecursionGuard guard(cuesheetEditorReactingToCursorMovement);
        if ((!preferredCuesheet.isNull()) && preferredCuesheet.length() >= 0
            && cuesheet == preferredCuesheet)
        {
            defaultCuesheetIndex = ui->comboBoxCuesheetSelector->count();
        }

        QString displayName = cuesheet;
        if (displayName.startsWith(musicRootPath))
            displayName.remove(0, musicRootPath.length());

        ui->comboBoxCuesheetSelector->addItem(displayName,
                                              cuesheet);
    }

    if (ui->comboBoxCuesheetSelector->count() > 0)
    {
        ui->comboBoxCuesheetSelector->setCurrentIndex(defaultCuesheetIndex);
        // if it was zero, we didn't load it because the index didn't change,
        // and we skipped loading it above. Sooo...
        if (0 == defaultCuesheetIndex)
            on_comboBoxCuesheetSelector_currentIndexChanged(0);
        hasLyrics = true;
    }

    // be careful here.  The Lyrics tab can now be the Patter tab.
    bool isPatter = songTypeNamesForPatter.contains(currentSongType);

//    qDebug() << "loadCuesheets: " << currentSongType << isPatter;

    if (isPatter) {
        // ----- PATTER -----
        if (hasLyrics && lyricsTabNumber != -1) {
//            qDebug() << "loadCuesheets 2: " << "setting to *";
            ui->tabWidget->setTabText(lyricsTabNumber, "*Patter");
        } else {
//            qDebug() << "loadCuesheets 2: " << "setting to NOT *";
            ui->tabWidget->setTabText(lyricsTabNumber, "Patter");

    #if defined(Q_OS_MAC)
            QString appPath = QApplication::applicationFilePath();
            QString patterTemplatePath = appPath + "/Contents/Resources/patter.template.html";
            patterTemplatePath.replace("Contents/MacOS/SquareDeskPlayer/","");
    #elif defined(Q_OS_WIN32)
            // TODO: There has to be a better way to do this.
            QString appPath = QApplication::applicationFilePath();
            QString patterTemplatePath = appPath + "/patter.template.html";
            patterTemplatePath.replace("SquareDeskPlayer.exe/","");
    #else
            // Linux
    #endif

    #if defined(Q_OS_MAC) | defined(Q_OS_WIN32)
            QFile file(patterTemplatePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qDebug() << "Could not open 'patter.template.html' file.";
                qDebug() << "looked here:" << patterTemplatePath;
                return;  // NOTE: early return, couldn't find template file
            } else {
                QString patterTemplate(file.readAll());
                file.close();
                ui->textBrowserCueSheet->setHtml(patterTemplate);
            }
    #else
            // LINUX
            ui->textBrowserCueSheet->setHtml("No patter found for this song.");
    #endif
        } // else (sequence could not be found)
    } else {
        // ----- SINGING CALL -----
        if (hasLyrics && lyricsTabNumber != -1) {
            ui->tabWidget->setTabText(lyricsTabNumber, "*Lyrics");
        } else {
            ui->tabWidget->setTabText(lyricsTabNumber, "Lyrics");

    #if defined(Q_OS_MAC)
            QString appPath = QApplication::applicationFilePath();
            QString lyricsTemplatePath = appPath + "/Contents/Resources/lyrics.template.html";
            lyricsTemplatePath.replace("Contents/MacOS/SquareDeskPlayer/","");
    #elif defined(Q_OS_WIN32)
            // TODO: There has to be a better way to do this.
            QString appPath = QApplication::applicationFilePath();
            QString lyricsTemplatePath = appPath + "/lyrics.template.html";
            lyricsTemplatePath.replace("SquareDeskPlayer.exe/","");
    #else
            // Linux
    #endif

    #if defined(Q_OS_MAC) | defined(Q_OS_WIN32)
            QFile file(lyricsTemplatePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qDebug() << "Could not open 'lyrics.template.html' file.";
                qDebug() << "looked here:" << lyricsTemplatePath;
                return;  // NOTE: early return, couldn't find template file
            } else {
                QString lyricsTemplate(file.readAll());
                file.close();
                ui->textBrowserCueSheet->setHtml(lyricsTemplate);
            }
    #else
            // LINUX
            ui->textBrowserCueSheet->setHtml("No lyrics found for this song.");
    #endif
        } // else (lyrics could not be found)
    } // isPatter
}


float MainWindow::getID3BPM(QString MP3FileName) {
    MPEG::File *mp3file;
    ID3v2::Tag *id3v2tag;  // NULL if it doesn't have a tag, otherwise the address of the tag

    mp3file = new MPEG::File(MP3FileName.toStdString().c_str()); // FIX: this leaks on read of another file
    id3v2tag = mp3file->ID3v2Tag(true);  // if it doesn't have one, create one

    float theBPM = 0.0;

    ID3v2::FrameList::ConstIterator it = id3v2tag->frameList().begin();
    for (; it != id3v2tag->frameList().end(); it++)
    {
        if ((*it)->frameID() == "TBPM")  // This is an Apple standard, which means it's everybody's standard now.
        {
            QString BPM((*it)->toString().toCString());
            theBPM = BPM.toFloat();
        }

    }

    return(theBPM);
}


void MainWindow::loadMP3File(QString MP3FileName, QString songTitle, QString songType)
{
    RecursionGuard recursion_guard(loadingSong);
    firstTimeSongIsPlayed = true;

    currentMP3filenameWithPath = MP3FileName;

    // resolve aliases at load time, rather than findFilesRecursively time, because it's MUCH faster
    QFileInfo fi(MP3FileName);
    QString resolvedFilePath = fi.symLinkTarget(); // path with the symbolic links followed/removed
    if (resolvedFilePath != "") {
        MP3FileName = resolvedFilePath;
    }

    currentSongType = songType;  // save it for session coloring on the analog clock later...

    ui->toolButtonEditLyrics->setChecked(false); // lyrics/cuesheets of new songs when loaded default to NOT editable

    loadCuesheets(MP3FileName);

    QStringList pieces = MP3FileName.split( "/" );
    QString filebase = pieces.value(pieces.length()-1);
    QStringList pieces2 = filebase.split(".");

    currentMP3filename = pieces2.value(pieces2.length()-2);


    if (songTitle != "") {
        ui->nowPlayingLabel->setText(songTitle);
    }
    else {
        ui->nowPlayingLabel->setText(currentMP3filename);  // FIX?  convert to short version?
    }
    currentSongTitle = ui->nowPlayingLabel->text();  // save, in case we are Flash Calling

    QDir md(MP3FileName);
    QString canonicalFN = md.canonicalPath();

    cBass.StreamCreate(MP3FileName.toStdString().c_str());

    QStringList ss = MP3FileName.split('/');
    QString fn = ss.at(ss.size()-1);
    this->setWindowTitle(fn + QString(" - SquareDesk MP3 Player/Editor"));

    int length_sec = cBass.FileLength;
    int songBPM = round(cBass.Stream_BPM);  // libbass's idea of the BPM

    // If the MP3 file has an embedded TBPM frame in the ID3 tag, then it overrides the libbass auto-detect of BPM
    float songBPM_ID3 = getID3BPM(MP3FileName);  // returns 0.0, if not found or not understandable

    if (songBPM_ID3 != 0.0) {
        songBPM = (int)songBPM_ID3;
    }

    baseBPM = songBPM;  // remember the base-level BPM of this song, for when the Tempo slider changes later

    // Intentionally compare against a narrower range here than BPM detection, because BPM detection
    //   returns a number at the limits, when it's actually out of range.
    // Also, turn off BPM for xtras (they are all over the place, including round dance cues, which have no BPM at all).
    //
    // TODO: make the types for turning off BPM detection a preference
    if ((songBPM>=125-15) && (songBPM<=125+15) && songType != "xtras") {
        tempoIsBPM = true;
        ui->currentTempoLabel->setText(QString::number(songBPM) + " BPM (100%)"); // initial load always at 100%

        ui->tempoSlider->setMinimum(songBPM-15);
        ui->tempoSlider->setMaximum(songBPM+15);

        PreferencesManager prefsManager;
        bool tryToSetInitialBPM = prefsManager.GettryToSetInitialBPM();
        int initialBPM = prefsManager.GetinitialBPM();
        if (tryToSetInitialBPM) {
            // if the user wants us to try to hit a particular BPM target, use that value
            ui->tempoSlider->setValue(initialBPM);
            ui->tempoSlider->valueChanged(initialBPM);  // fixes bug where second song with same BPM doesn't update songtable::tempo
        } else {
            // otherwise, if the user wants us to start with the slider at the regular detected BPM
            //   NOTE: this can be overridden by the "saveSongPreferencesInConfig" preference, in which case
            //     all saved tempo preferences will always win.
            ui->tempoSlider->setValue(songBPM);
            ui->tempoSlider->valueChanged(songBPM);  // fixes bug where second song with same BPM doesn't update songtable::tempo
        }

        ui->tempoSlider->SetOrigin(songBPM);    // when double-clicked, goes here
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
        ui->tempoSlider->valueChanged(100);  // fixes bug where second song with same 100% doesn't update songtable::tempo
        ui->tempoSlider->SetOrigin(100);  // when double-clicked, goes here
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
    ui->seekBarCuesheet->setEnabled(true);

    // when we add Pitch to the songTable as a hidden column, we do NOT need to force pitch anymore, because it
    //   will be set by the loader to the correct value (which is zero, if the MP3 file wasn't on the current playlist).
//    ui->pitchSlider->valueChanged(ui->pitchSlider->value()); // force pitch change, if pitch slider preset before load
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
    ui->midrangeSlider->valueChanged(
        ui->midrangeSlider->value()); // force midrange change, if midrange slider preset before load
    ui->trebleSlider->valueChanged(ui->trebleSlider->value()); // force treble change, if treble slider preset before load

    cBass.Stop();

    songLoaded = true;
    Info_Seekbar(true);

    bool isSingingCall = songTypeNamesForSinging.contains(songType) ||
                         songTypeNamesForCalled.contains(songType);

    bool isPatter = songTypeNamesForPatter.contains(songType);

    ui->lineEditIntroTime->setText("");
    ui->lineEditOutroTime->setText("");
    ui->seekBarCuesheet->SetDefaultIntroOutroPositions();
    ui->seekBar->SetDefaultIntroOutroPositions();

    ui->lineEditIntroTime->setEnabled(true);  // always enabled now, because anything CAN be looped now OR it has an intro/outro
    ui->lineEditOutroTime->setEnabled(true);

    ui->pushButtonSetIntroTime->setEnabled(true);  // always enabled now, because anything CAN be looped now OR it has an intro/outro
    ui->pushButtonSetOutroTime->setEnabled(true);

    if (isPatter) {
        on_loopButton_toggled(true); // default is to loop, if type is patter
//        ui->tabWidget->setTabText(lyricsTabNumber, "Patter");  // Lyrics tab does double duty as Patter tab
        ui->pushButtonSetIntroTime->setText("Start Loop");
        ui->pushButtonSetOutroTime->setText("End Loop");
    } else {
        // singing call or vocals or xtras, so Loop mode defaults to OFF
        on_loopButton_toggled(false); // default is to loop, if type is patter
//        ui->tabWidget->setTabText(lyricsTabNumber, "Lyrics");  // Lyrics tab is named "Lyrics"
        ui->pushButtonSetIntroTime->setText("In");
        ui->pushButtonSetOutroTime->setText("Out");
    }

    ui->seekBar->SetSingingCall(isSingingCall); // if singing call, color the seek bar
    ui->seekBarCuesheet->SetSingingCall(isSingingCall); // if singing call, color the seek bar

//    ui->pushButtonSetIntroTime->setEnabled(isSingingCall);  // if not singing call, buttons will be greyed out on Lyrics tab
//    ui->pushButtonSetOutroTime->setEnabled(isSingingCall);

//    ui->lineEditIntroTime->setText("");
//    ui->lineEditOutroTime->setText("");
//    ui->lineEditIntroTime->setEnabled(isSingingCall);
//    ui->lineEditOutroTime->setEnabled(isSingingCall);

    cBass.SetVolume(100);
    currentVolume = 100;
    previousVolume = 100;
    Info_Volume();

    loadSettingsForSong(songTitle);
}

void MainWindow::on_actionOpen_MP3_file_triggered()
{
    on_stopButton_clicked();  // if we're loading a new MP3 file, stop current playback

    saveCurrentSongSettings();

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    PreferencesManager prefsManager; // Will be using application information for correct location of your settings

    QString startingDirectory = prefsManager.Getdefault_dir();

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
    prefsManager.Setdefault_dir(CurrentDir.absoluteFilePath(MP3FileName));

    ui->songTable->clearSelection();  // if loaded via menu, then clear previous selection (if present)
    ui->nextSongButton->setEnabled(false);  // and, next/previous song buttons are disabled
    ui->previousSongButton->setEnabled(false);

    // --------
    loadMP3File(MP3FileName, QString(""), QString(""));  // "" means use title from the filename
}


// this function stores the absolute paths of each file in a QVector
void findFilesRecursively(QDir rootDir, QList<QString> *pathStack, QString suffix, Ui::MainWindow *ui, QString soundFXarray[6])
{
    QDirIterator it(rootDir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while(it.hasNext()) {
        QString s1 = it.next();
        QString resolvedFilePath=s1;

        QFileInfo fi(s1);

        // Option A: first sub-folder is the type of the song.
        // Examples:
        //    <musicDir>/foo/*.mp3 is of type "foo"
        //    <musicDir>/foo/bar/music.mp3 is of type "foo"
        //    <musicDir>/foo/bar/ ... /baz/music.mp3 is of type "foo"
        //    <musicDir>/music.mp3 is of type ""

        QString newType = (fi.path().replace(rootDir.path() + "/","").split("/"))[0];
        QStringList section = fi.path().split("/");

//        QString type = section[section.length()-1] + suffix;  // must be the last item in the path
//                                                              // of where the alias is, not where the file is, and append "*" or not
//        qDebug() << "FFR: " << fi.path() << rootDir.path() << type << newType;
        if (section[section.length()-1] != "soundfx") {
//            qDebug() << "findFilesRecursively() adding " + type + "#!#" + resolvedFilePath + " to pathStack";
//            pathStack->append(type + "#!#" + resolvedFilePath);

            // add to the pathStack iff it's not a sound FX .mp3 file (those are internal)
            pathStack->append(newType + "#!#" + resolvedFilePath);
        } else {
            if (suffix != "*") {
                // if it IS a sound FX file (and not GUEST MODE), then let's squirrel away the paths so we can play them later
                QString path1 = resolvedFilePath;
                QString baseName = resolvedFilePath.replace(rootDir.absolutePath(),"").replace("/soundfx/","");
                QStringList sections = baseName.split(".");
                if (sections.length() == 3 && sections[0].toInt() != 0 && sections[2] == "mp3") {
                    switch (sections[0].toInt()) {
                        case 1: ui->action_1->setText(sections[1]); break;
                        case 2: ui->action_2->setText(sections[1]); break;
                        case 3: ui->action_3->setText(sections[1]); break;
                        case 4: ui->action_4->setText(sections[1]); break;
                        case 5: ui->action_5->setText(sections[1]); break;
                        case 6: ui->action_6->setText(sections[1]); break;
                        default: break;
                    }
                soundFXarray[sections[0].toInt()-1] = path1;  // remember the path for playing it later
                } // if
            } // if
        } // else
    }
}


void MainWindow::findMusic(QString mainRootDir, QString guestRootDir, QString mode, bool refreshDatabase)
{
    QString databaseDir(mainRootDir + "/.squaredesk");

    if (refreshDatabase)
    {
        songSettings.openDatabase(databaseDir, mainRootDir, guestRootDir, false);
    }
    // always gets rid of the old pathstack...
    if (pathStack) {
        delete pathStack;
    }

    // make a new one
    pathStack = new QList<QString>();

    // mode == "main": look only in the main directory (e.g. there isn't a guest directory)
    // mode == "guest": look only in the guest directory (e.g. guest overrides main)
    // mode == "both": look in both places for MP3 files (e.g. merge guest into main)
    if (mode == "main" || mode == "both") {
        // looks for files in the mainRootDir --------
        QDir rootDir1(mainRootDir);
        rootDir1.setFilter(QDir::Files | QDir::Dirs | QDir::NoDot | QDir::NoDotDot);

        QStringList qsl;
        QString starDot("*.");
        for (size_t i = 0; i < sizeof(music_file_extensions) / sizeof(*music_file_extensions); ++i)
        {
            qsl.append(starDot + music_file_extensions[i]);
        }
        for (size_t i = 0; i < sizeof(cuesheet_file_extensions) / sizeof(*cuesheet_file_extensions); ++i)
        {
            qsl.append(starDot + cuesheet_file_extensions[i]);
        }

        rootDir1.setNameFilters(qsl);

        findFilesRecursively(rootDir1, pathStack, "", ui, soundFXarray);  // appends to the pathstack
    }

    if (guestRootDir != "" && (mode == "guest" || mode == "both")) {
        // looks for files in the guestRootDir --------
        QDir rootDir2(guestRootDir);
        rootDir2.setFilter(QDir::Files | QDir::Dirs | QDir::NoDot | QDir::NoDotDot);

        QStringList qsl;
        qsl.append("*.mp3");                // I only want MP3 files
        qsl.append("*.wav");                //          or WAV files
        rootDir2.setNameFilters(qsl);

        findFilesRecursively(rootDir2, pathStack, "*", ui, soundFXarray);  // appends to the pathstack, "*" for "Guest"
    }
}

void addStringToLastRowOfSongTable(QColor &textCol, MyTableWidget *songTable,
                                   QString str, int column)
{
    QTableWidgetItem *newTableItem;
    if (column == kNumberCol || column == kAgeCol || column == kPitchCol || column == kTempoCol) {
        newTableItem = new TableNumberItem( str.trimmed() );  // does sorting correctly for numbers
    } else {
        newTableItem = new QTableWidgetItem( str.trimmed() );
    }
    newTableItem->setFlags(newTableItem->flags() & ~Qt::ItemIsEditable);      // not editable
    newTableItem->setTextColor(textCol);
    if (column == kAgeCol || column == kPitchCol || column == kTempoCol) {
        newTableItem->setTextAlignment(Qt::AlignCenter);
    }
    songTable->setItem(songTable->rowCount()-1, column, newTableItem);
}



// --------------------------------------------------------------------------------
void MainWindow::filterMusic()
{
#ifdef CUSTOM_FILTER
    QString label = ui->labelSearch->text();
    QString type = ui->typeSearch->text();
    QString title = ui->titleSearch->text();

    ui->songTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);  // DO NOT SET height of rows (for now)

    ui->songTable->setSortingEnabled(false);

    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QString songTitle = ui->songTable->item(i,kTitleCol)->text();
        QString songType = ui->songTable->item(i,kTypeCol)->text();
        QString songLabel = ui->songTable->item(i,kLabelCol)->text();

        bool show = true;

        if (!(label.isEmpty()
              || songLabel.contains(label, Qt::CaseInsensitive)))
        {
            show = false;
        }
        if (!(type.isEmpty()
              || songType.contains(type, Qt::CaseInsensitive)))
        {
            show = false;
        }
        if (!(title.isEmpty()
              || songTitle.contains(title, Qt::CaseInsensitive)))
        {
            show = false;
        }
        ui->songTable->setRowHidden(i, !show);
    }
    ui->songTable->setSortingEnabled(true);
#else /* ifdef CUSTOM_FILTER */
    loadMusicList();
#endif /* else ifdef CUSTOM_FILTER */

    ui->songTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);  // auto set height of rows
}

// --------------------------------------------------------------------------------
void MainWindow::loadMusicList()
{
    startLongSongTableOperation("loadMusicList");  // for performance, hide and sorting off

    // Need to remember the PL# mapping here, and reapply it after the filter
    // left = path, right = number string
    QMap<QString, QString> path2playlistNum;

    // Iterate over the songTable, saving the mapping in "path2playlistNum"
    // TODO: optimization: save this once, rather than recreating each time.
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();  // this is the playlist #
        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();  // this is the full pathname
        if (playlistIndex != " " && playlistIndex != "") {
            // item HAS an index (that is, it is on the list, and has a place in the ordering)
            // TODO: reconcile int here with float elsewhere on insertion
            path2playlistNum[pathToMP3] = playlistIndex;
        }
    }

    // clear out the table
    ui->songTable->setRowCount(0);

    QStringList m_TableHeader;
    m_TableHeader << "#" << "Type" << "Label" << "Title";
    ui->songTable->setHorizontalHeaderLabels(m_TableHeader);
    ui->songTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->songTable->horizontalHeader()->setVisible(true);

    QListIterator<QString> iter(*pathStack);
    QColor textCol = QColor::fromRgbF(0.0/255.0, 0.0/255.0, 0.0/255.0);  // defaults to Black
    QList<QString> extensions;
    QString dot(".");
    for (size_t i = 0; i < sizeof(music_file_extensions) / sizeof(*music_file_extensions); ++i)
    {
        extensions.append(dot + music_file_extensions[i]);
    }
    bool show_all_ages = ui->actionShow_All_Ages->isChecked();

    while (iter.hasNext()) {
        QString s = iter.next();

        QListIterator<QString> extensionIterator(extensions);
        bool foundExtension = false;
        while (extensionIterator.hasNext())
        {
            QString extension(extensionIterator.next());
            if (s.endsWith(extension))
            {
                foundExtension = true;
            }
        }
        if (!foundExtension)
            continue;

        QStringList sl1 = s.split("#!#");
        QString type = sl1[0];  // the type (of original pathname, before following aliases)
        s = sl1[1];  // everything else
        QString origPath = s;  // for when we double click it later on...

        QFileInfo fi(s);

        if (fi.canonicalPath() == musicRootPath && type.right(1) != "*") {
            // e.g. "/Users/mpogue/__squareDanceMusic/C 117 - Bad Puppy (Patter).mp3" --> NO TYPE PRESENT and NOT a guest song
            type = "";
        }

        QStringList section = fi.canonicalPath().split("/");
        QString label = "";
        QString labelnum = "";
        QString labelnum_extra = "";
        QString title = "";
        QString shortTitle = "";

        s = fi.completeBaseName(); // e.g. "/Users/mpogue/__squareDanceMusic/patter/RIV 307 - Going to Ceili (Patter).mp3" --> "RIV 307 - Going to Ceili (Patter)"
        breakFilenameIntoParts(s, label, labelnum, labelnum_extra, title, shortTitle);
        labelnum += labelnum_extra;

        ui->songTable->setRowCount(ui->songTable->rowCount()+1);  // make one more row for this line

        QString cType = type;  // type for Color purposes
        if (cType.right(1)=="*") {
            cType.chop(1);  // remove the "*" for the purposes of coloring
        }

        if (songTypeNamesForExtras.contains(cType)) {
            textCol = QColor(extrasColorString);
        }
        else if (songTypeNamesForPatter.contains(cType)) {
            textCol = QColor(patterColorString);
        }
        else if (songTypeNamesForSinging.contains(cType)) {
            textCol = QColor(singingColorString);
        }
        else if (songTypeNamesForCalled.contains(cType)) {
            textCol = QColor(calledColorString);
        } else {
            textCol = QColor(Qt::black);  // if not a recognized type, color it black.
        }

        // look up origPath in the path2playlistNum map, and reset the s2 text to the user's playlist # setting (if any)
        QString s2("");
        if (path2playlistNum.contains(origPath)) {
            s2 = path2playlistNum[origPath];
        }
        TableNumberItem *newTableItem4 = new TableNumberItem(s2);

        newTableItem4->setTextAlignment(Qt::AlignCenter);                           // editable by default
        newTableItem4->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, kNumberCol, newTableItem4);      // add it to column 0

        addStringToLastRowOfSongTable(textCol, ui->songTable, type, kTypeCol);
        addStringToLastRowOfSongTable(textCol, ui->songTable, label + " " + labelnum, kLabelCol );
        addStringToLastRowOfSongTable(textCol, ui->songTable, title, kTitleCol);
        addStringToLastRowOfSongTable(textCol, ui->songTable,
                                      songSettings.getSongAge(fi.completeBaseName(), origPath,
                                                              show_all_ages),
                                      kAgeCol);

        int pitch = 0;
        int tempo = 0;
        bool loadedTempoIsPercent(false);
        SongSetting settings;
        songSettings.loadSettings(origPath,
                                  settings);

        if (settings.isSetPitch()) { pitch = settings.getPitch(); }
        if (settings.isSetTempo()) { tempo = settings.getTempo(); }
        if (settings.isSetTempoIsPercent()) { loadedTempoIsPercent = settings.getTempoIsPercent(); }

        addStringToLastRowOfSongTable(textCol, ui->songTable,
                                      QString("%1").arg(pitch),
                                      kPitchCol);

        QString tempoStr = QString("%1").arg(tempo);
        if (loadedTempoIsPercent) tempoStr += "%";
        addStringToLastRowOfSongTable(textCol, ui->songTable,
                                      tempoStr,
                                      kTempoCol);
        // keep the path around, for loading in when we double click on it
        ui->songTable->item(ui->songTable->rowCount()-1, kPathCol)->setData(Qt::UserRole,
                QVariant(origPath)); // path set on cell in col 0

        // Filter out (hide) rows that we're not interested in, based on the search fields...
        //   4 if statements is clearer than a gigantic single if....
        QString labelPlusNumber = label + " " + labelnum;
#ifndef CUSTOM_FILTER
        if (ui->labelSearch->text() != "" &&
                !labelPlusNumber.contains(QString(ui->labelSearch->text()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }

        if (ui->typeSearch->text() != "" && !type.contains(QString(ui->typeSearch->text()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }

        if (ui->titleSearch->text() != "" &&
                !title.contains(QString(ui->titleSearch->text()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }
#endif /* ifndef CUSTOM_FILTER */
    }

#ifdef CUSTOM_FILTER
    filterMusic();
#endif /* ifdef CUSTOM_FILTER */

    sortByDefaultSortOrder();
    stopLongSongTableOperation("loadMusicList");  // for performance, sorting on again and show

    QString msg1;
    if (guestMode == "main") {
        msg1 = QString::number(ui->songTable->rowCount()) + QString(" audio files found.");
    } else if (guestMode == "guest") {
        msg1 = QString::number(ui->songTable->rowCount()) + QString(" guest audio files found.");
    } else if (guestMode == "both") {
        msg1 = QString::number(ui->songTable->rowCount()) + QString(" total audio files found.");
    }
    ui->statusBar->showMessage(msg1);
}

QString processSequence(QString sequence,
                        const QStringList &include,
                        const QStringList &exclude)
{
    static QRegularExpression regexEmpty("^[\\s\\n]*$");
    QRegularExpressionMatch match = regexEmpty.match(sequence);
    if (match.hasMatch())
    {
        return QString();
    }

    for (int i = 0; i < exclude.length(); ++i)
    {
        if (sequence.contains(exclude[i], Qt::CaseInsensitive))
        {
            return QString();
        }
    }
    for (int i = 0; i < include.length(); ++i)
    {
        if (!sequence.contains(include[i], Qt::CaseInsensitive))
        {
            return QString();
        }
    }

    return sequence.trimmed();

//    QRegExp regexpAmp("&");
//    QRegExp regexpLt("<");
//    QRegExp regexpGt(">");
//    QRegExp regexpApos("'");
//    QRegExp regexpNewline("\n");
//
//    sequence = sequence.replace(regexpAmp, "&amp;");
//    sequence = sequence.replace(regexpLt, "&lt;");
//    sequence = sequence.replace(regexpGt, "&gt;");
//    sequence = sequence.replace(regexpApos, "&apos;");
//    sequence = sequence.replace(regexpNewline, "<br/>\n");
//
//    return "<h1>" + title + "</h1>\n<p>" + sequence + "</p>\n";

}

void extractSequencesFromFile(QStringList &sequences,
                                 const QString &filename,
                                 const QString &program,
                                 const QStringList &include,
                                 const QStringList &exclude)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&file);
    bool isSDFile(false);
    bool firstSDLine(false);
    QString thisProgram = "";
    QString title(program);

    if (filename.contains(program, Qt::CaseInsensitive))
    {
        thisProgram = program;
    }

    // Sun Jan 10 17:03:38 2016     Sd38.58:db38.58     Plus
    static QRegularExpression regexIsSDFile("^(Mon|Tue|Wed|Thur|Fri|Sat|Sun)\\s+" // Sun
                                           "(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\\s+" // Jan
                                           "\\d+\\s+\\d+\\:\\d+\\:\\d+\\s+\\d\\d\\d\\d\\s+" // 10 17:03:38 2016
                                           "Sd\\d+\\.\\d+\\:db\\d+\\.\\d+\\s+" //Sd38.58:db38.58
                                           "(\\w+)\\s*$"); // Plus

    QString sequence;

    while (!in.atEnd())
    {
        QString line(in.readLine());

        QRegularExpressionMatch match = regexIsSDFile.match(line);

        if (match.hasMatch())
        {
            if (0 == thisProgram.compare(program, Qt::CaseInsensitive))
            {
                sequences << processSequence(sequence, include, exclude);
            }
            isSDFile = true;
            firstSDLine = true;
            thisProgram = match.captured(3);
            sequence.clear();
            title.clear();
        }
        else if (!isSDFile)
        {
            QString line_simplified = line.simplified();
            if (line_simplified.startsWith("Basic", Qt::CaseInsensitive))
            {
                if (0 == thisProgram.compare(title, program, Qt::CaseInsensitive))
                {
                    sequences << processSequence(sequence, include, exclude);
                }
                thisProgram = "Basic";
                sequence.clear();
            }
            else if (line_simplified.startsWith("+", Qt::CaseInsensitive)
                || line_simplified.startsWith("Plus", Qt::CaseInsensitive))
            {
                if (0 == thisProgram.compare(title, program, Qt::CaseInsensitive))
                {
                    sequences << processSequence(sequence, include, exclude);
                }
                thisProgram = "Plus";
                sequence.clear();
            }
            else if (line_simplified.length() == 0)
            {
                if (0 == thisProgram.compare(title, program, Qt::CaseInsensitive))
                {
                    sequences << processSequence(sequence, include, exclude);
                }
                sequence.clear();
            }
            else
            {
                sequence += line + "\n";
            }

        }
        else // is SD file
        {
            QString line_simplified = line.simplified();

            if (firstSDLine)
            {
                if (line_simplified.length() == 0)
                {
                    firstSDLine = false;
                }
                else
                {
                    title += line;
                }
            }
            else
            {
                if (!(line_simplified.length() == 0))
                {
                    sequence += line + "\n";
                }
            }
        }
    }
    sequences << processSequence(sequence, include, exclude);
}




QStringList MainWindow::getUncheckedItemsFromCurrentCallList()
{
    QStringList uncheckedItems;
    for (int row = 0; row < ui->tableWidgetCallList->rowCount(); ++row)
    {
        if (ui->tableWidgetCallList->item(row, kCallListCheckedCol)->checkState() == Qt::Unchecked)
        {
            uncheckedItems.append(ui->tableWidgetCallList->item(row, kCallListNameCol)->data(0).toString());
        }
    }
    return uncheckedItems;
}

void MainWindow::filterChoreography()
{
    QStringList exclude(getUncheckedItemsFromCurrentCallList());
    QString program = ui->comboBoxCallListProgram->currentText();
#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    QStringList include = ui->lineEditChoreographySearch->text().split(",");
    for (int i = 0; i < include.length(); ++i)
    {
        include[i] = include[i].simplified();
    }

    if (ui->comboBoxChoreographySearchType->currentIndex() == 0)
    {
        exclude.clear();
    }

    QStringList sequences;

    for (int i = 0; i < ui->listWidgetChoreographyFiles->count()
             && sequences.length() < 128000; ++i)
    {
        QListWidgetItem *item = ui->listWidgetChoreographyFiles->item(i);
        if (item->checkState() == Qt::Checked)
        {
            QString filename = item->data(1).toString();
            extractSequencesFromFile(sequences, filename, program,
                                     include, exclude);
        }
    }

    ui->listWidgetChoreographySequences->clear();
    for (auto sequence : sequences)
    {
        if (!sequence.isEmpty())
        {
            QListWidgetItem *item = new QListWidgetItem(sequence);
            ui->listWidgetChoreographySequences->addItem(item);
        }
    }
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
}

#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
void MainWindow::on_listWidgetChoreographySequences_itemDoubleClicked(QListWidgetItem * /* item */)
{
    QListWidgetItem *choreoItem = new QListWidgetItem(item->text());
    ui->listWidgetChoreography->addItem(choreoItem);
}

void MainWindow::on_listWidgetChoreography_itemDoubleClicked(QListWidgetItem * /* item */)
{
    ui->listWidgetChoreography->takeItem(ui->listWidgetChoreography->row(item));
}


void MainWindow::on_lineEditChoreographySearch_textChanged()
{
    filterChoreography();
}

void MainWindow::on_listWidgetChoreographyFiles_itemChanged(QListWidgetItem * /* item */)
{
    filterChoreography();
}
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT

void MainWindow::loadChoreographyList()
{
#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    ui->listWidgetChoreographyFiles->clear();

    QListIterator<QString> iter(*pathStack);

    while (iter.hasNext()) {
        QString s = iter.next();

        if (s.endsWith(".txt")
            && (s.contains("sequence", Qt::CaseInsensitive)
                || s.contains("singer", Qt::CaseInsensitive)))
        {
            QStringList sl1 = s.split("#!#");
            QString type = sl1[0];  // the type (of original pathname, before following aliases)
            QString origPath = sl1[1];  // everything else

            QFileInfo fi(origPath);
//            QStringList section = fi.canonicalPath().split("/");
            QString name = fi.completeBaseName();
            QListWidgetItem *item = new QListWidgetItem(name);
            item->setData(1,origPath);
            item->setCheckState(Qt::Unchecked);
            ui->listWidgetChoreographyFiles->addItem(item);
        }
    }
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
}


static void addToProgramsAndWriteTextFile(QStringList &programs, QDir outputDir,
                                   const char *filename,
                                   const char *fileLines[])
{
    QString outputFile = outputDir.canonicalPath() + "/" + filename;
    QFile file(outputFile);
    if (file.open(QIODevice::ReadWrite)) {
        QTextStream stream(&file);
        for (int i = 0; fileLines[i]; ++i)
        {
            stream << fileLines[i] << endl;
        }
        programs << outputFile;
    }
}




void MainWindow::loadDanceProgramList(QString lastDanceProgram)
{
    ui->comboBoxCallListProgram->clear();
    QListIterator<QString> iter(*pathStack);
    QStringList programs;


    while (iter.hasNext()) {
        QString s = iter.next();

        if (s.endsWith(".txt", Qt::CaseInsensitive))
        {
            QStringList sl1 = s.split("#!#");
            QString type = sl1[0];  // the type (of original pathname, before following aliases)
            QString origPath = sl1[1];  // everything else
            QFileInfo fi(origPath);
            if (fi.dir().canonicalPath().endsWith("/reference"))
            {
                programs << origPath;
            }
        }
    }

    if (programs.length() == 0)
    {
        QString referencePath = musicRootPath + "/reference";
        QDir outputDir(referencePath);
        if (!outputDir.exists())
        {
            outputDir.mkpath(".");
        }

        addToProgramsAndWriteTextFile(programs, outputDir, "010.basic1.txt", danceprogram_basic1);
        addToProgramsAndWriteTextFile(programs, outputDir, "020.basic2.txt", danceprogram_basic2);
        addToProgramsAndWriteTextFile(programs, outputDir, "030.mainstream.txt", danceprogram_mainstream);
        addToProgramsAndWriteTextFile(programs, outputDir, "040.plus.txt", danceprogram_plus);
        addToProgramsAndWriteTextFile(programs, outputDir, "050.a1.txt", danceprogram_a1);
        addToProgramsAndWriteTextFile(programs, outputDir, "060.a2.txt", danceprogram_a2);

    }
    programs.sort(Qt::CaseInsensitive);
    QListIterator<QString> program(programs);
    while (program.hasNext())
    {
        QString origPath = program.next();
        QString name;
        QString program;
        breakDanceProgramIntoParts(origPath, name, program);
        ui->comboBoxCallListProgram->addItem(name, origPath);
    }

    if (ui->comboBoxCallListProgram->maxCount() == 0)
    {
        ui->comboBoxCallListProgram->addItem("<no dance programs found>", "");
    }

    for (int i = 0; i < ui->comboBoxCallListProgram->count(); ++i)
    {
        if (ui->comboBoxCallListProgram->itemText(i) == lastDanceProgram)
        {
            ui->comboBoxCallListProgram->setCurrentIndex(i);
            break;
        }
    }
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
    saveCurrentSongSettings();

    int row = item->row();
    QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();

    QString songTitle = ui->songTable->item(row,kTitleCol)->text();
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,kTypeCol)->text().toLower();

    // these must be up here to get the correct values...
    QString pitch = ui->songTable->item(row,kPitchCol)->text();
    QString tempo = ui->songTable->item(row,kTempoCol)->text();

    loadMP3File(pathToMP3, songTitle, songType);

    // these must be down here, to set the correct values...
    int pitchInt = pitch.toInt();
    ui->pitchSlider->setValue(pitchInt);

    if (tempo != "0" && tempo != "0%") {
        // iff tempo is known, then update the table
        QString tempo2 = tempo.replace("%",""); // if percentage (not BPM) just get rid of the "%" (setValue knows what to do)
        int tempoInt = tempo2.toInt();
        if (tempoInt !=0)
        {
            ui->tempoSlider->setValue(tempoInt);
        }
    }
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
    PreferencesManager prefsManager;
    prefsManager.Setautostartplayback(ui->actionAutostart_playback->isChecked());
}

void MainWindow::on_checkBoxPlayOnEnd_clicked()
{
    PreferencesManager prefsManager;
    prefsManager.Setstartplaybackoncountdowntimer(ui->checkBoxPlayOnEnd->isChecked());
}

void MainWindow::on_checkBoxStartOnPlay_clicked()
{
    PreferencesManager prefsManager;
    prefsManager.Setstartcountuptimeronplay(ui->checkBoxStartOnPlay->isChecked());
}


void MainWindow::on_actionImport_triggered()
{
    RecursionGuard dialog_guard(inPreferencesDialog);

    ImportDialog *importDialog = new ImportDialog();
    int dialogCode = importDialog->exec();
    RecursionGuard keypress_guard(trapKeypresses);
    if (dialogCode == QDialog::Accepted)
    {
        importDialog->importSongs(songSettings, pathStack);
        loadMusicList();
    }
    delete importDialog;
    importDialog = NULL;
}

void MainWindow::on_actionExport_triggered()
{
    RecursionGuard dialog_guard(inPreferencesDialog);

    if (true)
    {
        QString filename =
            QFileDialog::getSaveFileName(this, tr("Select Export File"),
                                         QDir::homePath(),
                                         tr("Tab Separated (*.tsv);;Comma Separated (*.csv)"));
        if (!filename.isNull())
        {
            QFile file( filename );
            if ( file.open(QIODevice::WriteOnly) )
            {
                QTextStream stream( &file );

                enum ColumnExportData outputFields[7];
                int outputFieldCount = sizeof(outputFields) / sizeof(*outputFields);
                char separator = filename.endsWith(".csv", Qt::CaseInsensitive) ? ',' :
                    '\t';

                outputFields[0] = ExportDataFileName;
                outputFields[1] = ExportDataPitch;
                outputFields[2] = ExportDataTempo;
                outputFields[3] = ExportDataIntro;
                outputFields[4] = ExportDataOutro;
                outputFields[5] = ExportDataVolume;
                outputFields[6] = ExportDataCuesheetPath;

                exportSongList(stream, songSettings, pathStack,
                               outputFieldCount, outputFields,
                               separator,
                               true, false);
            }
        }
    }
    else
    {
        ExportDialog *exportDialog = new ExportDialog();
        int dialogCode = exportDialog->exec();
        RecursionGuard keypress_guard(trapKeypresses);
        if (dialogCode == QDialog::Accepted)
        {
            exportDialog->exportSongs(songSettings, pathStack);
        }
        delete exportDialog;
        exportDialog = NULL;
    }
}

// --------------------------------------------------------
void MainWindow::on_actionPreferences_triggered()
{
    RecursionGuard dialog_guard(inPreferencesDialog);
    trapKeypresses = false;
//    on_stopButton_clicked();  // stop music, if it was playing...
    PreferencesManager prefsManager;

    prefDialog = new PreferencesDialog;
    prefsManager.populatePreferencesDialog(prefDialog);
    prefDialog->songTableReloadNeeded = false;  // nothing has changed...yet.

    // modal dialog
    int dialogCode = prefDialog->exec();
    trapKeypresses = true;

    // act on dialog return code
    if(dialogCode == QDialog::Accepted) {
        // OK clicked
        // Save the new value for musicPath --------
        prefsManager.extractValuesFromPreferencesDialog(prefDialog);

        // USER SAID "OK", SO HANDLE THE UPDATED PREFS ---------------
        musicRootPath = prefsManager.GetmusicPath();

        findMusic(musicRootPath, "", "main", true); // always refresh the songTable after the Prefs dialog returns with OK
        switchToLyricsOnPlay = prefsManager.GetswitchToLyricsOnPlay();

        // Save the new value for music type colors --------
        patterColorString = prefsManager.GetpatterColorString();
        singingColorString = prefsManager.GetsingingColorString();
        calledColorString = prefsManager.GetcalledColorString();
        extrasColorString = prefsManager.GetextrasColorString();

        // ----------------------------------------------------------------
        // Show the Timers tab, if it is enabled now
        if (prefsManager.GetexperimentalTimersEnabled()) {
            if (!showTimersTab) {
                // iff the tab was NOT showing, make it show up now
                ui->tabWidget->insertTab(1, tabmap.value(1).first, tabmap.value(1).second);  // bring it back now!
            }
            showTimersTab = true;
        }
        else {
            if (showTimersTab) {
                // iff timers tab was showing, remove it
                ui->tabWidget->removeTab(1);  // hidden, but we can bring it back later
            }
            showTimersTab = false;
        }

        // ----------------------------------------------------------------
        // Show the Lyrics tab, if it is enabled now
        lyricsTabNumber = (showTimersTab ? 2 : 1);

        bool isPatter = songTypeNamesForPatter.contains(currentSongType);
//        qDebug() << "actionPreferences_triggered: " << currentSongType << isPatter;

        if (isPatter) {
            if (hasLyrics && lyricsTabNumber != -1) {
                ui->tabWidget->setTabText(lyricsTabNumber, "*Patter");
            } else {
                ui->tabWidget->setTabText(lyricsTabNumber, "Patter");
            }
        } else {
            if (hasLyrics && lyricsTabNumber != -1) {
                ui->tabWidget->setTabText(lyricsTabNumber, "*Lyrics");
            } else {
                ui->tabWidget->setTabText(lyricsTabNumber, "Lyrics");
            }
        }

        // -----------------------------------------------------------------------
        // Save the new settings for experimental break and patter timers --------
        tipLengthTimerEnabled = prefsManager.GettipLengthTimerEnabled();  // save new settings in MainWindow, too
        tipLengthTimerLength = prefsManager.GettipLengthTimerLength();
        tipLengthAlarmAction = prefsManager.GettipLengthAlarmAction();

        breakLengthTimerEnabled = prefsManager.GetbreakLengthTimerEnabled();
        breakLengthTimerLength = prefsManager.GetbreakLengthTimerLength();
        breakLengthAlarmAction = prefsManager.GetbreakLengthAlarmAction();

        // and tell the clock, too.
        analogClock->tipLengthTimerEnabled = tipLengthTimerEnabled;
        analogClock->tipLengthAlarmMinutes = tipLengthTimerLength;

        analogClock->breakLengthTimerEnabled = breakLengthTimerEnabled;
        analogClock->breakLengthAlarmMinutes = breakLengthTimerLength;

        // ----------------------------------------------------------------
        // Save the new value for experimentalClockColoringEnabled --------
        clockColoringHidden = !prefsManager.GetexperimentalClockColoringEnabled();
        analogClock->setHidden(clockColoringHidden);

        {
            QString value;
            value = prefsManager.GetMusicTypeSinging();
            songTypeNamesForSinging = value.toLower().split(";", QString::KeepEmptyParts);

            value = prefsManager.GetMusicTypePatter();
            songTypeNamesForPatter = value.toLower().split(";", QString::KeepEmptyParts);

            value = prefsManager.GetMusicTypeExtras();
            songTypeNamesForExtras = value.toLower().split(";", QString::KeepEmptyParts);

            value = prefsManager.GetMusicTypeCalled();
            songTypeNamesForCalled = value.split(";", QString::KeepEmptyParts);
        }
        songFilenameFormat = static_cast<enum SongFilenameMatchingType>(prefsManager.GetSongFilenameFormat());

        if (prefDialog->songTableReloadNeeded) {
            loadMusicList();
        }

        if (prefsManager.GetenableAutoAirplaneMode()) {
            // if the user JUST set the preference, turn Airplane Mode on RIGHT NOW (radios OFF).
            airplaneMode(true);
        } else {
            // if the user JUST set the preference, turn Airplane Mode OFF RIGHT NOW (radios ON).
            airplaneMode(false);
        }

        if (prefsManager.GetenableAutoMicsOff()) {
            microphoneStatusUpdate();
        }
    }

    delete prefDialog;
    prefDialog = NULL;
}

QString MainWindow::removePrefix(QString prefix, QString s)
{
    QString s2 = s.remove( prefix );
    return s2;
}

// Adapted from: https://github.com/hnaohiro/qt-csv/blob/master/csv.cpp
QStringList MainWindow::parseCSV(const QString &string)
{
    enum State {Normal, Quote} state = Normal;
    QStringList fields;
    QString value;

    for (int i = 0; i < string.size(); i++)
    {
        QChar current = string.at(i);

        // Normal state
        if (state == Normal)
        {
            // Comma
            if (current == ',')
            {
                // Save field
                fields.append(value);
                value.clear();
            }

            // Double-quote
            else if (current == '"')
                state = Quote;

            // Other character
            else
                value += current;
        }

        // In-quote state
        else if (state == Quote)
        {
            // Another double-quote
            if (current == '"')
            {
                if (i+1 < string.size())
                {
                    QChar next = string.at(i+1);

                    // A double double-quote?
                    if (next == '"')
                    {
                        value += '"';
                        i++;
                    }
                    else
                        state = Normal;
                }
            }

            // Other character
            else
                value += current;
        }
    }
    if (!value.isEmpty())
        fields.append(value);

    return fields;
}

// returns first song error, and also updates the songCount as it goes (2 return values)
QString MainWindow::loadPlaylistFromFile(QString PlaylistFileName, int &songCount) {

//    qDebug() << "loadPlaylist: " << PlaylistFileName;
    addFilenameToRecentPlaylist(PlaylistFileName);  // remember it in the Recent list

    // --------
    QString firstBadSongLine = "";
    QFile inputFile(PlaylistFileName);
    if (inputFile.open(QIODevice::ReadOnly)) { // defaults to Text mode

        // first, clear all the playlist numbers that are there now.
        for (int i = 0; i < ui->songTable->rowCount(); i++) {
            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
            theItem->setText("");
        }

        QTextStream in(&inputFile);

        if (PlaylistFileName.endsWith(".csv")) {
            // CSV FILE =================================
            int lineCount = 1;

            QString header = in.readLine();  // read header (and throw away for now), should be "abspath,pitch,tempo"

            while (!in.atEnd()) {
                QString line = in.readLine();

                if (line == "abspath") {
                    // V1 of the CSV file format has exactly one field, an absolute pathname in quotes
                }
                else if (line == "") {
                    // ignore, it's a blank line
                }
                else {
                    songCount++;  // it's a real song path

                    QStringList list1 = parseCSV(line);  // This is more robust than split(). Handles commas inside double quotes, double double quotes, etc.

                    bool match = false;
                    // exit the loop early, if we find a match
                    for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {

                        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();

                        if (list1[0] == pathToMP3) { // FIX: this is fragile, if songs are moved around, since absolute paths are used.

                            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
                            theItem->setText(QString::number(songCount));

                            QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);
                            theItem2->setText(list1[1].trimmed());

                            QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);
                            theItem3->setText(list1[2].trimmed());

                            match = true;
                        }
                    }
                    // if we had no match, remember the first non-matching song path
                    if (!match && firstBadSongLine == "") {
                        firstBadSongLine = line;
                    }

                }

                lineCount++;
            } // while
        }
        else {
            // M3U FILE =================================
            int lineCount = 1;

            while (!in.atEnd()) {
                QString line = in.readLine();

                if (line == "#EXTM3U") {
                    // ignore, it's the first line of the M3U file
                }
                else if (line == "") {
                    // ignore, it's a blank line
                }
                else if (line.at( 0 ) == '#' ) {
                    // it's a comment line
                    if (line.mid(0,7) == "#EXTINF") {
                        // it's information about the next line, ignore for now.
                    }
                }
                else {
                    songCount++;  // it's a real song path

                    bool match = false;
                    // exit the loop early, if we find a match
                    for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {
                        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
                        if (line == pathToMP3) { // FIX: this is fragile, if songs are moved around
                            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
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
        }

        inputFile.close();

    }
    else {
        // file didn't open...
        return("");
    }

    return(firstBadSongLine);  // return error song (if any)
}


// PLAYLIST MANAGEMENT ===============================================
void MainWindow::finishLoadingPlaylist(QString PlaylistFileName) {

    startLongSongTableOperation("finishLoadingPlaylist"); // for performance measurements, hide and sorting off

    // --------
    QString firstBadSongLine = "";
    int songCount = 0;

    firstBadSongLine = loadPlaylistFromFile(PlaylistFileName, songCount);

    sortByDefaultSortOrder();
    ui->songTable->sortItems(kNumberCol);  // sort by playlist # as primary (must be LAST)

    // select the very first row, and trigger a GO TO PREVIOUS, which will load row 0 (and start it, if autoplay is ON).
    // only do this, if there were no errors in loading the playlist numbers.
    if (firstBadSongLine == "") {
        ui->songTable->selectRow(0); // select first row of newly loaded and sorted playlist!
        on_actionPrevious_Playlist_Item_triggered();
    }

    stopLongSongTableOperation("finishLoadingPlaylist"); // for performance measurements, sorting on again and show

    QString msg1 = QString("Loaded playlist with ") + QString::number(songCount) + QString(" items.");
    if (firstBadSongLine != "") {
        // if there was a non-matching path, tell the user what the first one of those was
        msg1 = QString("ERROR: could not find '") + firstBadSongLine + QString("'");
        ui->songTable->clearSelection(); // select nothing, if error
    }
    ui->statusBar->showMessage(msg1);
}

void MainWindow::on_actionLoad_Playlist_triggered()
{
    on_stopButton_clicked();  // if we're loading a new PLAYLIST file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");
    PreferencesManager prefsManager;
    QString musicRootPath = prefsManager.GetmusicPath();
    QString startingPlaylistDirectory = prefsManager.Getdefault_playlist_dir();

    trapKeypresses = false;
    QString PlaylistFileName =
        QFileDialog::getOpenFileName(this,
                                     tr("Load Playlist"),
                                     startingPlaylistDirectory,
                                     tr("Playlist Files (*.m3u *.csv)"));
    trapKeypresses = true;
    if (PlaylistFileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
    QDir CurrentDir;
    QFileInfo fInfo(PlaylistFileName);
    prefsManager.Setdefault_playlist_dir(fInfo.absolutePath());

    finishLoadingPlaylist(PlaylistFileName);
}

struct PlaylistExportRecord
{
    int index;
    QString title;
    QString pitch;
    QString tempo;
};

static bool comparePlaylistExportRecord(const PlaylistExportRecord &a, const PlaylistExportRecord &b)
{
    return a.index < b.index;
}

// SAVE CURRENT PLAYLIST TO FILE
void MainWindow::saveCurrentPlaylistToFile(QString PlaylistFileName) {
    // --------
    QList<PlaylistExportRecord> exports;

    // Iterate over the songTable
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();
        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
        QString songTitle = ui->songTable->item(i,kTitleCol)->text();
        QString pitch = ui->songTable->item(i,kPitchCol)->text();
        QString tempo = ui->songTable->item(i,kTempoCol)->text();

        if (playlistIndex != "") {
            // item HAS an index (that is, it is on the list, and has a place in the ordering)
            // TODO: reconcile int here with float elsewhere on insertion
            PlaylistExportRecord rec;
            rec.index = playlistIndex.toInt();
//            rec.title = songTitle;
            rec.title = pathToMP3;  // NOTE: this is an absolute path that does not survive moving musicDir
            rec.pitch = pitch;
            rec.tempo = tempo;
            exports.append(rec);
        }
    }

    qSort(exports.begin(), exports.end(), comparePlaylistExportRecord);
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

    QFile file(PlaylistFileName);
    if (PlaylistFileName.endsWith(".m3u")) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {  // delete, if it exists already
            QTextStream stream(&file);
            stream << "#EXTM3U" << endl << endl;

            // list is auto-sorted here
            foreach (const PlaylistExportRecord &rec, exports)
            {
                stream << "#EXTINF:-1," << endl;  // nothing after the comma = no special name
                stream << rec.title << endl;
            }
            file.close();
            addFilenameToRecentPlaylist(PlaylistFileName);  // add to the MRU list
        }
        else {
            ui->statusBar->showMessage(QString("ERROR: could not open M3U file."));
        }
    }
    else if (PlaylistFileName.endsWith(".csv")) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream stream(&file);
            stream << "abspath,pitch,tempo" << endl;

            foreach (const PlaylistExportRecord &rec, exports)
            {
                stream << "\"" << rec.title << "\"," <<
                    rec.pitch << "," <<
                    rec.tempo << endl; // quoted absolute path, integer pitch (no quotes), integer tempo (opt % or 0)
            }
            file.close();
            addFilenameToRecentPlaylist(PlaylistFileName);  // add to the MRU list
        }
        else {
            ui->statusBar->showMessage(QString("ERROR: could not open CSV file."));
        }
    }
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

    QString preferred("CSV files (*.csv)");
    trapKeypresses = false;
    QString PlaylistFileName =
        QFileDialog::getSaveFileName(this,
                                     tr("Save Playlist"),
                                     startingPlaylistDirectory,
                                     tr("M3U playlists (*.m3u);;CSV files (*.csv)"),
                                     &preferred);  // preferred is CSV
    trapKeypresses = true;
    if (PlaylistFileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
    QFileInfo fInfo(PlaylistFileName);
    PreferencesManager prefsManager;
    prefsManager.Setdefault_playlist_dir(fInfo.absolutePath());

    saveCurrentPlaylistToFile(PlaylistFileName);  // SAVE IT

    // TODO: if there are no songs specified in the playlist (yet, because not edited, or yet, because
    //   no playlist was loaded), Save Playlist... should be greyed out.

    if (PlaylistFileName.endsWith(".csv")) {
        ui->statusBar->showMessage(QString("Playlist items saved as CSV file."));
    }
    else if (PlaylistFileName.endsWith(".m3u")) {
        ui->statusBar->showMessage(QString("Playlist items saved as M3U file."));
    }
    else {
        ui->statusBar->showMessage(QString("ERROR: Can't save to that format."));
    }
}

void MainWindow::on_actionNext_Playlist_Item_triggered()
{
    // This code is similar to the row double clicked code...
    on_stopButton_clicked();  // if we're going to the next file in the playlist, stop current playback
    saveCurrentSongSettings();

    // figure out which row is currently selected
    QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
        return;
    }

    int maxRow = ui->songTable->rowCount() - 1;

    // which is the next VISIBLE row?
    int lastVisibleRow = row;
    row = (maxRow < row+1 ? maxRow : row+1); // bump up by 1
    while (ui->songTable->isRowHidden(row) && row < maxRow) {
        // keep bumping, until the next VISIBLE row is found, or we're at the END
        row = (maxRow < row+1 ? maxRow : row+1); // bump up by 1
    }
    if (ui->songTable->isRowHidden(row)) {
        // if we try to go past the end of the VISIBLE rows, stick at the last visible row (which
        //   was the last one we were on.  Well, that's not always true, but this is a quick and dirty
        //   solution.  If I go to a row, select it, and then filter all rows out, and hit one of the >>| buttons,
        //   hilarity will ensue.
        row = lastVisibleRow;
    }
    ui->songTable->selectRow(row); // select new row!

    // load all the UI fields, as if we double-clicked on the new row
    QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();
    QString songTitle = ui->songTable->item(row,kTitleCol)->text();
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,kTypeCol)->text();

    // must be up here...
    QString pitch = ui->songTable->item(row,kPitchCol)->text();
    QString tempo = ui->songTable->item(row,kTempoCol)->text();

    loadMP3File(pathToMP3, songTitle, songType);

    // must be down here...
    int pitchInt = pitch.toInt();
    ui->pitchSlider->setValue(pitchInt);

    if (tempo != "0") {
        QString tempo2 = tempo.replace("%",""); // get rid of optional "%", slider->setValue will do the right thing
        int tempoInt = tempo2.toInt();
        ui->tempoSlider->setValue(tempoInt);
    }

    if (ui->actionAutostart_playback->isChecked()) {
        on_playButton_clicked();
    }

}

void MainWindow::on_actionPrevious_Playlist_Item_triggered()
{
    // This code is similar to the row double clicked code...
    on_stopButton_clicked();  // if we're going to the next file in the playlist, stop current playback
    saveCurrentSongSettings();

    QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
        return;
    }

    // which is the next VISIBLE row?
    int lastVisibleRow = row;
    row = (row-1 < 0 ? 0 : row-1); // bump backwards by 1

    while (ui->songTable->isRowHidden(row) && row > 0) {
        // keep bumping backwards, until the previous VISIBLE row is found, or we're at the BEGINNING
        row = (row-1 < 0 ? 0 : row-1); // bump backwards by 1
    }
    if (ui->songTable->isRowHidden(row)) {
        // if we try to go past the beginning of the VISIBLE rows, stick at the first visible row (which
        //   was the last one we were on.  Well, that's not always true, but this is a quick and dirty
        //   solution.  If I go to a row, select it, and then filter all rows out, and hit one of the >>| buttons,
        //   hilarity will ensue.
        row = lastVisibleRow;
    }

    ui->songTable->selectRow(row); // select new row!

    // load all the UI fields, as if we double-clicked on the new row
    QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();
    QString songTitle = ui->songTable->item(row,kTitleCol)->text();
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,kTypeCol)->text();

    // must be up here...
    QString pitch = ui->songTable->item(row,kPitchCol)->text();
    QString tempo = ui->songTable->item(row,kTempoCol)->text();

    loadMP3File(pathToMP3, songTitle, songType);

    // must be down here...
    int pitchInt = pitch.toInt();
    ui->pitchSlider->setValue(pitchInt);

    if (tempo != "0") {
        QString tempo2 = tempo.replace("%",""); // get rid of optional "%", setValue will take care of it
        int tempoInt = tempo.toInt();
        ui->tempoSlider->setValue(tempoInt);
    }

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
    QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if (selected.count() == 1) {
        ui->nextSongButton->setEnabled(true);
        ui->previousSongButton->setEnabled(true);
    }
    else {
        ui->nextSongButton->setEnabled(false);
        ui->previousSongButton->setEnabled(false);
    }

    // ----------------
    int selectedRow = selectedSongRow();  // get current row or -1

    // turn them all OFF
    ui->actionAt_TOP->setEnabled(false);
    ui->actionAt_BOTTOM->setEnabled(false);
    ui->actionUP_in_Playlist->setEnabled(false);
    ui->actionDOWN_in_Playlist->setEnabled(false);
    ui->actionRemove_from_Playlist->setEnabled(false);

    if (selectedRow != -1) {
        // if a single row was selected
        QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
        int currentNumberInt = currentNumberText.toInt();
        int playlistItemCount = PlaylistItemCount();

        if (currentNumberText == "") {
            // if not in a Playlist then we can add it at Top or Bottom, that's it.
            ui->actionAt_TOP->setEnabled(true);
            ui->actionAt_BOTTOM->setEnabled(true);
        } else {
            ui->actionRemove_from_Playlist->setEnabled(true);  // can remove it

            // current item is on the Playlist already
            if (playlistItemCount > 1) {
                // more than one item on the list
                if (currentNumberInt == 1) {
//                     it's the first item, and there's more than one item on the list, so moves make sense
                    ui->actionAt_BOTTOM->setEnabled(true);
                    ui->actionDOWN_in_Playlist->setEnabled(true);
                } else if (currentNumberInt == playlistItemCount) {
                    // it's the last item, and there's more than one item on the list, so moves make sense
                    ui->actionAt_TOP->setEnabled(true);
                    ui->actionUP_in_Playlist->setEnabled(true);
                } else {
                    // it's somewhere in the middle, and there's more than one item on the list, so moves make sense
                    ui->actionAt_TOP->setEnabled(true);
                    ui->actionAt_BOTTOM->setEnabled(true);
                    ui->actionUP_in_Playlist->setEnabled(true);
                    ui->actionDOWN_in_Playlist->setEnabled(true);
                }
            } else {
                // One item on the playlist, and this is it.
                // Can't move up/down or to top/bottom.
                // Can remove it, though.
            }
        }
    }
}

void MainWindow::on_actionClear_Playlist_triggered()
{
    startLongSongTableOperation("on_actionClear_Playlist_triggered");  // for performance, hide and sorting off

    // Iterate over the songTable
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        theItem->setText(""); // clear out the current list

        // let's intentionally NOT clear the pitches.  They are persistent within a session.
        // let's intentionally NOT clear the tempos.  They are persistent within a session.
    }

    sortByDefaultSortOrder();

    stopLongSongTableOperation("on_actionClear_Playlist_triggered");  // for performance, sorting on again and show

    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled
}

// ---------------------------------------------------------
void MainWindow::showInFinderOrExplorer(QString filePath)
{
// From: http://lynxline.com/show-in-finder-show-in-explorer/
#if defined(Q_OS_MAC)
    QStringList args;
    args << "-e";
    args << "tell application \"Finder\"";
    args << "-e";
    args << "activate";
    args << "-e";
    args << "select POSIX file \""+filePath+"\"";
    args << "-e";
    args << "end tell";
    QProcess::startDetached("osascript", args);
#endif

#if defined(Q_OS_WIN)
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(filePath);
    QProcess::startDetached("explorer", args);
#endif

#ifdef Q_OS_LINUX
    QStringList args;
    args << QFileInfo(filePath).absoluteDir().canonicalPath();
    QProcess::startDetached("xdg-open", args);
#endif // ifdef Q_OS_LINUX
}

// ---------------------------------------------------------
int MainWindow::getInputVolume()
{
#if defined(Q_OS_MAC)
    QProcess getVolumeProcess;
    QStringList args;
    args << "-e";
    args << "set ivol to input volume of (get volume settings)";

    getVolumeProcess.start("osascript", args);
    getVolumeProcess.waitForFinished(); // sets current thread to sleep and waits for pingProcess end
    QString output(getVolumeProcess.readAllStandardOutput());

    int vol = output.trimmed().toInt();

    return(vol);
#endif

#if defined(Q_OS_WIN)
    return(-1);
#endif

#ifdef Q_OS_LINUX
    return(-1);
#endif
}

void MainWindow::setInputVolume(int newVolume)
{
#if defined(Q_OS_MAC)
    if (newVolume != -1) {
        QProcess getVolumeProcess;
        QStringList args;
        args << "-e";
        args << "set volume input volume " + QString::number(newVolume);

        getVolumeProcess.start("osascript", args);
        getVolumeProcess.waitForFinished();
        QString output(getVolumeProcess.readAllStandardOutput());
    }
#endif

#if defined(Q_OS_WIN)
#endif

#ifdef Q_OS_LINUX
#endif
}

void MainWindow::muteInputVolume()
{
    PreferencesManager prefsManager; // Will be using application information for correct location of your settings
    if (!prefsManager.GetenableAutoMicsOff()) {
        return;
    }

    int vol = getInputVolume();
    if (vol > 0) {
        // if not already muted, save the current volume (for later restore)
        currentInputVolume = vol;
        setInputVolume(0);
    }
}

void MainWindow::unmuteInputVolume()
{
    PreferencesManager prefsManager; // Will be using application information for correct location of your settings
    if (!prefsManager.GetenableAutoMicsOff()) {
        return;
    }

    int vol = getInputVolume();
    if (vol > 0) {
        // the user has changed it, so don't muck with it!
    } else {
        setInputVolume(currentInputVolume);     // restore input from the mics
    }
}

// ----------------------------------------------------------------------
int MainWindow::selectedSongRow() {
    QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } // else more than 1 row or no rows, just return -1
    return row;
}

// ----------------------------------------------------------------------
int MainWindow::PlaylistItemCount() {
    int playlistItemCount = 0;

    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();  // this is the playlist #
        if (playlistIndex != "") {
            playlistItemCount++;
        }
    }
    return (playlistItemCount);
}

// ----------------------------------------------------------------------
void MainWindow::PlaylistItemToTop() {
    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
    int currentNumberInt = currentNumberText.toInt();

    if (currentNumberText == "") {
        // add to list, and make it the #1

        // Iterate over the entire songTable, incrementing every item
        // TODO: turn off sorting
        for (int i=0; i<ui->songTable->rowCount(); i++) {
            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
            QString playlistIndex = theItem->text();  // this is the playlist #
            if (playlistIndex != "") {
                // if a # was set, increment it
                QString newIndex = QString::number(playlistIndex.toInt()+1);
                ui->songTable->item(i,kNumberCol)->setText(newIndex);
            }
        }

        ui->songTable->item(selectedRow, kNumberCol)->setText("1");  // this one is the new #1
        // TODO: turn on sorting again

    } else {
        // already on the list
        // Iterate over the entire songTable, incrementing items BELOW this item
        // TODO: turn off sorting
        for (int i=0; i<ui->songTable->rowCount(); i++) {
            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
            QString playlistIndexText = theItem->text();  // this is the playlist #
            if (playlistIndexText != "") {
                int playlistIndexInt = playlistIndexText.toInt();
                if (playlistIndexInt < currentNumberInt) {
                    // if a # was set and less, increment it
                    QString newIndex = QString::number(playlistIndexInt+1);
//                    qDebug() << "old, new:" << playlistIndexText << newIndex;
                    ui->songTable->item(i,kNumberCol)->setText(newIndex);
                }
            }
        }
        // and then set this one to #1
        ui->songTable->item(selectedRow, kNumberCol)->setText("1");  // this one is the new #1
    }

    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemToBottom() {
    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
    int currentNumberInt = currentNumberText.toInt();

    int playlistItemCount = PlaylistItemCount();  // how many items in the playlist right now?

    if (currentNumberText == "") {
        // add to list, and make it the bottom

        // Iterate over the entire songTable, not touching every item
        // TODO: turn off sorting
        ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(playlistItemCount+1));  // this one is the new #LAST
        // TODO: turn on sorting again

    } else {
        // already on the list
        // Iterate over the entire songTable, decrementing items ABOVE this item
        // TODO: turn off sorting
        for (int i=0; i<ui->songTable->rowCount(); i++) {
            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
            QString playlistIndexText = theItem->text();  // this is the playlist #
            if (playlistIndexText != "") {
                int playlistIndexInt = playlistIndexText.toInt();
                if (playlistIndexInt > currentNumberInt) {
                    // if a # was set and more, decrement it
                    QString newIndex = QString::number(playlistIndexInt-1);
                    ui->songTable->item(i,kNumberCol)->setText(newIndex);
                }
            }
        }
        // and then set this one to #LAST
        ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(playlistItemCount));  // this one is the new #1
    }
    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemMoveUp() {
    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
    int currentNumberInt = currentNumberText.toInt();

    // Iterate over the entire songTable, find the item just above this one, and move IT down (only)
    // TODO: turn off sorting

    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();  // this is the playlist #
        if (playlistIndex != "") {
            int playlistIndexInt = playlistIndex.toInt();
            if (playlistIndexInt == currentNumberInt - 1) {
                QString newIndex = QString::number(playlistIndex.toInt()+1);
                ui->songTable->item(i,kNumberCol)->setText(newIndex);
            }
        }
    }

    ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(currentNumberInt-1));  // this one moves UP
    // TODO: turn on sorting again
    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemMoveDown() {
    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
    int currentNumberInt = currentNumberText.toInt();

    // add to list, and make it the bottom

    // Iterate over the entire songTable, find the item just BELOW this one, and move it UP (only)
    // TODO: turn off sorting

    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();  // this is the playlist #
        if (playlistIndex != "") {
            int playlistIndexInt = playlistIndex.toInt();
            if (playlistIndexInt == currentNumberInt + 1) {
                QString newIndex = QString::number(playlistIndex.toInt()-1);
                ui->songTable->item(i,kNumberCol)->setText(newIndex);
            }
        }
    }

    ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(currentNumberInt+1));  // this one moves UP
    // TODO: turn on sorting again
    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemRemove() {
    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
    int currentNumberInt = currentNumberText.toInt();

    // already on the list
    // Iterate over the entire songTable, decrementing items BELOW this item
    // TODO: turn off sorting
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndexText = theItem->text();  // this is the playlist #
        if (playlistIndexText != "") {
            int playlistIndexInt = playlistIndexText.toInt();
            if (playlistIndexInt > currentNumberInt) {
                // if a # was set and more, decrement it
                QString newIndex = QString::number(playlistIndexInt-1);
                ui->songTable->item(i,kNumberCol)->setText(newIndex);
            }
        }
    }
    // and then set this one to #LAST
    ui->songTable->item(selectedRow, kNumberCol)->setText("");  // this one is off the list
    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled
}


void MainWindow::on_songTable_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);

    if (ui->songTable->selectionModel()->hasSelection()) {
        QMenu menu(this);

        int selectedRow = selectedSongRow();  // get current row or -1

        if (selectedRow != -1) {
            // if a single row was selected
            QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
            int currentNumberInt = currentNumberText.toInt();
            int playlistItemCount = PlaylistItemCount();

            if (currentNumberText == "") {
                if (playlistItemCount == 0) {
                    menu.addAction ( "Add to playlist" , this , SLOT (PlaylistItemToTop()) );
                } else {
                    menu.addAction ( "Add to TOP of playlist" , this , SLOT (PlaylistItemToTop()) );
                    menu.addAction ( "Add to BOTTOM of playlist" , this , SLOT (PlaylistItemToBottom()) );
                }
            } else {
                // currently on the playlist
                if (playlistItemCount > 1) {
                    // more than one item
                    if (currentNumberInt == 1) {
                        // already the first item, and there's more than one item on the list, so moves make sense
                        menu.addAction ( "Move DOWN in playlist" , this , SLOT (PlaylistItemMoveDown()) );
                        menu.addAction ( "Move to BOTTOM of playlist" , this , SLOT (PlaylistItemToBottom()) );
                    } else if (currentNumberInt == playlistItemCount) {
                        // already the last item, and there's more than one item on the list, so moves make sense
                        menu.addAction ( "Move to TOP of playlist" , this , SLOT (PlaylistItemToTop()) );
                        menu.addAction ( "Move UP in playlist" , this , SLOT (PlaylistItemMoveUp()) );
                    } else {
                        // somewhere in the middle, and there's more than one item on the list, so moves make sense
                        menu.addAction ( "Move to TOP of playlist" , this , SLOT (PlaylistItemToTop()) );
                        menu.addAction ( "Move UP in playlist" , this , SLOT (PlaylistItemMoveUp()) );
                        menu.addAction ( "Move DOWN in playlist" , this , SLOT (PlaylistItemMoveDown()) );
                        menu.addAction ( "Move to BOTTOM of playlist" , this , SLOT (PlaylistItemToBottom()) );
                    }
                } else {
                    // exactly one item, and this is it.
                }
                // this item is on the playlist, so it can be removed.
                menu.addSeparator();
                menu.addAction ( "Remove from playlist" , this , SLOT (PlaylistItemRemove()) );
            }
        }
        menu.addSeparator();

#if defined(Q_OS_MAC)
        menu.addAction ( "Reveal in Finder" , this , SLOT (revealInFinder()) );
#endif

#if defined(Q_OS_WIN)
        menu.addAction ( "Show in Explorer" , this , SLOT (revealInFinder()) );
#endif

#if defined(Q_OS_LINUX)
        menu.addAction ( "Open containing folder" , this , SLOT (revealInFinder()) );
#endif

        menu.popup(QCursor::pos());
        menu.exec();
    }
}

void MainWindow::revealInFinder()
{
    QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();

        QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();
        showInFinderOrExplorer(pathToMP3);
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
    }
}

void MainWindow::columnHeaderResized(int logicalIndex, int /* oldSize */, int newSize)
{
    int x1,y1,w1,h1;
    int x2,y2,w2,h2;
    int x3,y3,w3,h3;

    switch (logicalIndex) {
        case 0: // #
            // FIX: there's a bug here if the # column width is changed.  Qt doesn't seem to keep track of
            //  the correct size of the # column thereafter.  This is particularly visible on Win10, but it's
            //  also present on Mac OS X (Sierra).

            // # column width is not tracked by Qt (BUG), so we have to do it manually
            col0_width = newSize;

            x1 = newSize + 14;
            y1 = ui->typeSearch->y();
            w1 = ui->songTable->columnWidth(1) - 5;
            h1 = ui->typeSearch->height();
            ui->typeSearch->setGeometry(x1,y1,w1,h1);

            x2 = x1 + w1 + 6 - 1;
            y2 = ui->labelSearch->y();
            w2 = ui->songTable->columnWidth(2) - 5;
            h2 = ui->labelSearch->height();
            ui->labelSearch->setGeometry(x2,y2,w2,h2);

            x3 = x2 + w2 + 6;
            y3 = ui->titleSearch->y();
            w3 = ui->songTable->width() - ui->clearSearchButton->width() - x3;
            h3 = ui->titleSearch->height();
            ui->titleSearch->setGeometry(x3,y3,w3,h3);

            break;

        case 1: // Type
            x1 = col0_width + 35;
            y1 = ui->typeSearch->y();
            w1 = newSize - 4;
            h1 = ui->typeSearch->height();
            ui->typeSearch->setGeometry(x1,y1,w1,h1);
            ui->typeSearch->setFixedWidth(w1);

            x2 = x1 + w1 + 6;
            y2 = ui->labelSearch->y();
            w2 = ui->songTable->columnWidth(2) - 6;
            h2 = ui->labelSearch->height();
            ui->labelSearch->setGeometry(x2,y2,w2,h2);
            ui->labelSearch->setFixedWidth(w2);

            x3 = x2 + w2 + 6;
            y3 = ui->titleSearch->y();
            w3 = ui->songTable->width() - ui->clearSearchButton->width() - x3 + 17;
            h3 = ui->titleSearch->height();
            ui->titleSearch->setGeometry(x3,y3,w3,h3);
            break;

        case 2: // Label
            x1 = ui->typeSearch->x();
            y1 = ui->typeSearch->y();
            w1 = ui->typeSearch->width();
            h1 = ui->typeSearch->height();

            x2 = x1 + w1 + 6;
            y2 = ui->labelSearch->y();
            w2 = newSize - 6;
            h2 = ui->labelSearch->height();
            ui->labelSearch->setGeometry(x2,y2,w2,h2);
            ui->labelSearch->setFixedWidth(w2);

            x3 = x2 + w2 + 6;
            y3 = ui->titleSearch->y();
            w3 = ui->songTable->width() - ui->clearSearchButton->width() - x3 + 17;
            h3 = ui->titleSearch->height();
            ui->titleSearch->setGeometry(x3,y3,w3,h3);
            break;

        case 3: // Title
            break;

        default:
            break;
    }

}

// ----------------------------------------------------------------------
void MainWindow::saveCurrentSongSettings()
{
    if (loadingSong)
        return;

    QString currentSong = ui->nowPlayingLabel->text();

    if (!currentSong.isEmpty()) {
        int pitch = ui->pitchSlider->value();
        int tempo = ui->tempoSlider->value();
        int cuesheetIndex = ui->comboBoxCuesheetSelector->currentIndex();
        QString cuesheetFilename = cuesheetIndex >= 0 ?
            ui->comboBoxCuesheetSelector->itemData(cuesheetIndex).toString()
            : "";

        SongSetting setting;
        setting.setFilename(currentMP3filename);
        setting.setFilenameWithPath(currentMP3filenameWithPath);
        setting.setSongname(currentSong);
        setting.setVolume(currentVolume);
        setting.setPitch(pitch);
        setting.setTempo(tempo);
        setting.setTempoIsPercent(!tempoIsBPM);
        setting.setIntroPos(ui->seekBarCuesheet->GetIntro());
        setting.setOutroPos(ui->seekBarCuesheet->GetOutro());
        setting.setIntroOutroIsTimeBased(false);
        setting.setCuesheetName(cuesheetFilename);
        setting.setSongLength((double)(ui->seekBarCuesheet->maximum()));

        setting.setTreble( ui->trebleSlider->value() );
        setting.setBass( ui->bassSlider->value() );
        setting.setMidrange( ui->midrangeSlider->value() );
        setting.setMix( ui->mixSlider->value() );

        songSettings.saveSettings(currentMP3filenameWithPath,
                                  setting);
        if (ui->checkBoxAutoSaveLyrics->isChecked())
        {
            writeCuesheet(cuesheetFilename);
        }
    }


}

void MainWindow::loadSettingsForSong(QString songTitle)
{
    int pitch = ui->pitchSlider->value();
    int tempo = ui->tempoSlider->value();
    int volume = ui->volumeSlider->value();
    double intro = ui->seekBarCuesheet->GetIntro();
    double outro = ui->seekBarCuesheet->GetOutro();
    QString cuesheetName = "";

    SongSetting settings;
    settings.setFilename(currentMP3filename);
    settings.setFilenameWithPath(currentMP3filenameWithPath);
    settings.setSongname(songTitle);
    settings.setVolume(volume);
    settings.setPitch(pitch);
    settings.setTempo(tempo);
    settings.setIntroPos(intro);
    settings.setOutroPos(outro);

    if (songSettings.loadSettings(currentMP3filenameWithPath,
                                  settings))
    {
        if (settings.isSetPitch()) { pitch = settings.getPitch(); }
        if (settings.isSetTempo()) { tempo = settings.getTempo(); }
        if (settings.isSetVolume()) { volume = settings.getVolume(); }
        if (settings.isSetIntroPos()) { intro = settings.getIntroPos(); }
        if (settings.isSetOutroPos()) { outro = settings.getOutroPos(); }
        if (settings.isSetCuesheetName()) { cuesheetName = settings.getCuesheetName(); } // ADDED *****

        double length = (double)(ui->seekBarCuesheet->maximum());
        if (settings.isSetIntroOutroIsTimeBased() && settings.getIntroOutroIsTimeBased())
        {
            intro = intro / length;
            outro = outro / length;
        }

        ui->pitchSlider->setValue(pitch);
        ui->tempoSlider->setValue(tempo);
        ui->volumeSlider->setValue(volume);
        ui->seekBarCuesheet->SetIntro(intro);
        ui->seekBarCuesheet->SetOutro(outro);

        ui->lineEditIntroTime->setText(doubleToTime(intro * length));
        ui->lineEditOutroTime->setText(doubleToTime(outro * length));

        if (cuesheetName.length() > 0)
        {
            for (int i = 0; i < ui->comboBoxCuesheetSelector->count(); ++i)
            {
                QString itemName = ui->comboBoxCuesheetSelector->itemData(i).toString();
                if (itemName == cuesheetName)
                {
                    ui->comboBoxCuesheetSelector->setCurrentIndex(i);
                    break;
                }
            }
        }
        if (settings.isSetTreble())
        {
            ui->trebleSlider->setValue(settings.getTreble() );
        }
        else
        {
            ui->trebleSlider->setValue(0) ;
        }
        if (settings.isSetBass())
        {
            ui->bassSlider->setValue( settings.getBass() );
        }
        else
        {
            ui->bassSlider->setValue(0);
        }
        if (settings.isSetMidrange())
        {
            ui->midrangeSlider->setValue( settings.getMidrange() );
        }
        else
        {
            ui->midrangeSlider->setValue(0);
        }
        if (settings.isSetMix())
        {
            ui->mixSlider->setValue( settings.getMix() );
        }
        else
        {
            ui->mixSlider->setValue(0);
        }

    }
    else
    {
        ui->trebleSlider->setValue(0);
        ui->bassSlider->setValue(0);
        ui->midrangeSlider->setValue(0);
        ui->mixSlider->setValue(0);
    }
    double length = (double)(ui->seekBarCuesheet->maximum());
    ui->lineEditIntroTime->setText(doubleToTime(intro * length));
    ui->lineEditOutroTime->setText(doubleToTime(outro * length));
}

// ------------------------------------------------------------------------------------------
QString MainWindow::loadLyrics(QString MP3FileName)
{
    QString USLTlyrics;

    MPEG::File *mp3file;
    ID3v2::Tag *id3v2tag;  // NULL if it doesn't have a tag, otherwise the address of the tag

    mp3file = new MPEG::File(MP3FileName.toStdString().c_str()); // FIX: this leaks on read of another file
    id3v2tag = mp3file->ID3v2Tag(true);  // if it doesn't have one, create one

    ID3v2::FrameList::ConstIterator it = id3v2tag->frameList().begin();
    for (; it != id3v2tag->frameList().end(); it++)
    {
        if ((*it)->frameID() == "SYLT")
        {
//            qDebug() << "LOAD LYRICS -- found an SYLT frame!";
        }

        if ((*it)->frameID() == "USLT")
        {
//            qDebug() << "LOAD LYRICS -- found a USLT frame!";

            ID3v2::UnsynchronizedLyricsFrame* usltFrame = (ID3v2::UnsynchronizedLyricsFrame*)(*it);
            USLTlyrics = usltFrame->text().toCString();
        }

    }

    return (USLTlyrics);
}

// ------------------------------------------------------------------------
QString MainWindow::txtToHTMLlyrics(QString text, QString filePathname) {
    QStringList pieces = filePathname.split( "/" );
    pieces.pop_back(); // get rid of actual filename, keep the path
    QString filedir = pieces.join("/"); // FIX: MAC SPECIFIC?

    // TODO: we could get fancy, and keep looking in parent directories until we
    //  find a CSS file, or until we hit the musicRootPath.  That allows for an overall
    //  default, with overrides for individual subdirs.

    QString css("");
    bool fileIsOpen = false;
    QFile f1(filedir + "/cuesheet2.css");  // This is the SqView convention for a CSS file
    if ( f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // if there's a "cuesheet2.css" file in the same directory as the .txt file,
        //   then we're going to embed it into the HTML representation of the .txt file,
        //   so that the font preferences therein apply.
        fileIsOpen = true;
        QTextStream in(&f1);
        css = in.readAll();  // read the entire CSS file, if it exists
    }

    text = text.toHtmlEscaped();  // convert ">" to "&gt;" etc
    text = text.replace(QRegExp("[\r|\n]"),"<br/>\n");

    // TODO: fancier translation of .txt to .html, recognizing headers, etc.

    QString HTML;
    HTML += "<HTML>\n";
    HTML += "<HEAD><STYLE>" + css + "</STYLE></HEAD>\n";
    HTML += "<BODY>\n" + text + "</BODY>\n";
    HTML += "</HTML>\n";

    if (fileIsOpen) {
        f1.close();
    }
    return(HTML);
}

// ----------------------------------------------------------------------------
QStringList MainWindow::getCurrentVolumes() {

    QStringList volumeDirList;

#if defined(Q_OS_MAC)
    foreach (const QStorageInfo &storageVolume, QStorageInfo::mountedVolumes()) {
            if (storageVolume.isValid() && storageVolume.isReady()) {
            volumeDirList.append(storageVolume.name());}
    }
#endif

#if defined(Q_OS_WIN32)
    foreach (const QFileInfo &fileinfo, QDir::drives()) {
        volumeDirList.append(fileinfo.absoluteFilePath());
    }
#endif

    qSort(volumeDirList);     // always return sorted, for convenient comparisons later

    return(volumeDirList);
}

// ----------------------------------------------------------------------------
void MainWindow::on_newVolumeMounted() {

    QSet<QString> newVolumes, goneVolumes;
    QString newVolume, goneVolume;

    guestMode = "both"; // <-- replace, don't merge (use "both" for merge, "guest" for just guest's music)

    if (lastKnownVolumeList.length() < newVolumeList.length()) {
        // ONE OR MORE VOLUMES APPEARED
        //   ONLY LOOK AT THE LAST ONE IN THE LIST THAT'S NEW (if more than 1)
        newVolumes = newVolumeList.toSet().subtract(lastKnownVolumeList.toSet());
        foreach (const QString &item, newVolumes) {
            newVolume = item;  // first item is the volume added
        }

#if defined(Q_OS_MAC)
        guestVolume = newVolume;  // e.g. MIKEPOGUE
        guestRootPath = "/Volumes/" + guestVolume + "/";  // this is where to search for a Music Directory
        QApplication::beep();  // beep only on MAC OS X (Win32 already beeps by itself)
#endif

#if defined(Q_OS_WIN32)
        guestVolume = newVolume;            // e.g. E:
        guestRootPath = newVolume + "\\";   // this is where to search for a Music Directory
#endif

        // We do it this way, so that the name of the root directory is case insensitive (squareDanceMusic, squaredancemusic, etc.)
        QDir newVolumeRootDir(guestRootPath);
        newVolumeRootDir.setFilter(QDir::Dirs | QDir::NoDot | QDir::NoDotDot);

        QDirIterator it(newVolumeRootDir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        QString d;
        bool foundSquareDeskMusicDir = false;
        QString foundDirName;
        while(it.hasNext()) {
            d = it.next();
            // If alias, try to follow it.
            QString resolvedFilePath = it.fileInfo().symLinkTarget(); // path with the symbolic links followed/removed
            if (resolvedFilePath == "") {
                // If NOT alias, then use the original fileName
                resolvedFilePath = d;
            }

            QFileInfo fi(d);
            QStringList section = fi.canonicalFilePath().split("/");  // expand
            if (section.length() >= 1) {
                QString dirName = section[section.length()-1];

                if (dirName.compare("squaredancemusic",Qt::CaseInsensitive) == 0) { // exact match, but case-insensitive
                    foundSquareDeskMusicDir = true;
                    foundDirName = dirName;
                    break;  // break out of the for loop when we find first directory that matches "squaredancemusic"
                }
            } else {
                continue;
            }
        }

        if (!foundSquareDeskMusicDir) {
            return;  // if we didn't find anything, just return
        }

        guestRootPath += foundDirName;  // e.g. /Volumes/MIKE/squareDanceMusic, or E:\SquareDanceMUSIC

        ui->statusBar->showMessage("SCANNING GUEST VOLUME: " + newVolume);
        QThread::sleep(1);  // FIX: not sure this is needed, but it sometimes hangs if not used, on first mount of a flash drive.

        findMusic(musicRootPath, guestRootPath, guestMode, false);  // get the filenames from the guest's directories
    } else if (lastKnownVolumeList.length() > newVolumeList.length()) {
        // ONE OR MORE VOLUMES WENT AWAY
        //   ONLY LOOK AT THE LAST ONE IN THE LIST THAT'S GONE
        goneVolumes = lastKnownVolumeList.toSet().subtract(newVolumeList.toSet());
        foreach (const QString &item, goneVolumes) {
            goneVolume = item;
        }

        ui->statusBar->showMessage("REMOVING GUEST VOLUME: " + goneVolume);
        QApplication::beep();  // beep on MAC OS X and Win32
        QThread::sleep(1);  // FIX: not sure this is needed.

        guestMode = "main";
        guestRootPath = "";
        findMusic(musicRootPath, "", guestMode, false);  // get the filenames from the user's directories
    } else {
//        qDebug() << "No volume added/lost by the time we got here. I give up. :-(";
        return;
    }

    lastKnownVolumeList = newVolumeList;

    loadMusicList(); // and filter them into the songTable
}

void MainWindow::on_warningLabel_clicked() {
    analogClock->resetPatter();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (ui->tabWidget->tabText(index) == "SD") {
        // SD Tab ---------------
        if (console != 0) {
            console->setFocus();
        }
        ui->actionSave_Lyrics->setDisabled(true);
        ui->actionSave_Lyrics_As->setDisabled(true);
        ui->actionPrint_Lyrics->setDisabled(true);
    } else if (ui->tabWidget->tabText(index) == "Lyrics" || ui->tabWidget->tabText(index) == "*Lyrics" ||
               ui->tabWidget->tabText(index) == "Patter" || ui->tabWidget->tabText(index) == "*Patter") {
        // Lyrics Tab ---------------
        ui->actionPrint_Lyrics->setDisabled(false);
        ui->actionSave_Lyrics->setDisabled(false);      // TODO: enable only when we've made changes
        ui->actionSave_Lyrics_As->setDisabled(false);   // always enabled, because we can always save as a different name
    } else  {
        ui->actionPrint_Lyrics->setDisabled(true);
        ui->actionSave_Lyrics->setDisabled(true);
        ui->actionSave_Lyrics_As->setDisabled(true);
    }

    microphoneStatusUpdate();
}

void MainWindow::microphoneStatusUpdate() {
    int index = ui->tabWidget->currentIndex();

    if (ui->tabWidget->tabText(index) == "SD") {
        if (voiceInputEnabled && currentApplicationState == Qt::ApplicationActive) {
            ui->statusBar->setStyleSheet("color: red");
            ui->statusBar->showMessage("Microphone enabled for voice input (Level: PLUS)");
            unmuteInputVolume();
        } else {
            ui->statusBar->setStyleSheet("color: black");
            ui->statusBar->showMessage("Microphone disabled (Level: PLUS)");
            muteInputVolume();                      // disable all input from the mics
        }
    } else {
        if (voiceInputEnabled && currentApplicationState == Qt::ApplicationActive) {
            ui->statusBar->setStyleSheet("color: black");
            ui->statusBar->showMessage("Microphone will be enabled for voice input in SD tab");
            muteInputVolume();                      // disable all input from the mics
        } else {
            ui->statusBar->setStyleSheet("color: black");
            ui->statusBar->showMessage("Microphone disabled");
            muteInputVolume();                      // disable all input from the mics
        }
    }
}

void MainWindow::writeSDData(const QByteArray &data)
{
    if (data != "") {
        // console has data, send to sd
        QString d = data;
        d.replace("\r","\n");
        if (d.at(d.length()-1) == '\n') {
            sd->write(d.toUtf8());
            sd->waitForBytesWritten();
        } else {
            sd->write(d.toUtf8());
            sd->waitForBytesWritten();
        }
    }
}

void MainWindow::readSDData()
{
    // sd has data, send to console
    QByteArray s = sd->readAll();

    s = s.replace("\r\n","\n");  // This should be safe on all platforms, but is required for Win32

    QString qs(s);

    if (qs.contains("\a")) {
        QApplication::beep();
        qs = qs.replace("\a","");  // delete all BEL chars
    }

    uneditedData.append(qs);

    // do deletes early
    bool done = false;
    while (!done) {
        int beforeLength = uneditedData.length();
        uneditedData.replace(QRegExp(".\b \b"),""); // if coming in as bulk text, delete one char
        int afterLength = uneditedData.length();
        done = (beforeLength == afterLength);
    }

    QString lastLayout1;
    QString format2line;
    QList<QString> lastFormatList;

    QRegExp sequenceLine("^ *([0-9]+):(.*)");
    QRegExp sectionStart("Sd 38.89");

    QStringList currentSequence;
    QString lastPrompt;

    // TODO: deal with multiple resolve lines
    QString errorLine;
    QString resolveLine;
    copyrightText = "";
    bool grabResolve = false;

    // scan the unedited lines for sectionStart, layout1/2, and sequence lines
    QStringList lines = uneditedData.split("\n");
    foreach (const QString &line, lines) {
        if (grabResolve) {
            resolveLine = line;  // grabs next line, then stops.
            grabResolve = false;
        } else if (line.contains("layout1:")) {
            lastLayout1 = line;
            lastLayout1 = lastLayout1.replace("layout1: ","").replace("\"","");
            lastFormatList.clear();
        } else if (line.contains("layout2:")) {
            format2line = line;
            format2line = format2line.replace("layout2: ","").replace("\"","");
            lastFormatList.append(format2line);
            errorLine = "";     // clear out possible error line
            resolveLine = "";   // clear out possible resolve line
        } else if (sequenceLine.indexIn(line) > -1) {
            // line like "3: square thru"
            int itemNumber = sequenceLine.cap(1).toInt();
            QString call = sequenceLine.cap(2).trimmed();

            if (call == "HEADS" || call == "SIDES") {
                call += " start";
            }

            if (itemNumber-1 < currentSequence.length()) {
                currentSequence.replace(itemNumber-1, call);
            } else {
                currentSequence.append(call);
            }
        } else if (line.contains(sectionStart)) {
            currentSequence.clear();
            editedData = "";  // sectionStart causes clear of editedData
        } else if (line == "") {
            // skip blank lines
        } else if (line.contains("Enter startup command>") ||
                   line.contains("Do you really want to abort it?") ||
                   line.contains("Enter search command>") ||
                   line.contains("Enter comment:") ||
                   line.contains("Do you want to write it anyway?")) {
            // PROMPTS
            lastPrompt = errorLine + line;  // this is a bold idea.  treat this as the last prompt, too.
        } else if (line.startsWith("SD --") || line.contains("Copyright") || line.contains("Gildea")) {
            copyrightText += line + "\n";
        } else if (line.contains("(no matches)")) {
            // special case for this error message
             errorLine = line + "\n";
        } else if (line.contains("resolve is:")) {
            // special case for this line
            grabResolve = true;
        } else if (line.contains("-->")) {
            // suppress output, but record it
            lastPrompt = errorLine + line;  // this is a bold idea.  show last error only.
        } else {
            editedData += line;  // no layout lines make it into here
            editedData += "\n";  // no layout lines make it into here
        }
    }

    editedData += lastPrompt.replace("\a","");  // keep only the last prompt (no NL)

    // echo is needed for entering the level and entering comments, but NOT wanted after that
    if (lastPrompt.contains("Enter startup command>") || lastPrompt.contains("Enter comment:")) {
        console->setLocalEchoEnabled(true);
    } else {
        console->setLocalEchoEnabled(false);
    }

    // capitalize all the words in each call
    for (int i=0; i < currentSequence.length(); i++ ) {
        QString current = currentSequence.at(i);
        QStringList words = current.split(" ");
        for (int j=0; j < words.length(); j++ ) {
            QString current2 = words.at(j);
            QString replacement2;
            if (current2.length() >= 1 && current2.at(0) == '[') {
                // left bracket case
                replacement2 = "[" + QString(current2.at(1)).toUpper() + current2.mid(2).toLower();
            } else {
                // normal case
                replacement2 = current2.left(1).toUpper() + current2.mid(1).toLower();
            }
            words.replace(j, replacement2);
        }
        QString replacement = words.join(" ");
        currentSequence.replace(i, replacement);
    }

    if (resolveLine == "") {
        currentSequenceWidget->setText(currentSequence.join("\n"));
    } else {
        currentSequenceWidget->setText(currentSequence.join("\n") + "\nresolve is: " + resolveLine);
    }

    // always scroll to make the last line visible, as we're adding lines
    QScrollBar *sb = currentSequenceWidget->verticalScrollBar();
    sb->setValue(sb->maximum());

    console->clear();
    console->putData(QByteArray(editedData.toLatin1()));

    // look at unedited last line to see if there's a prompt
    if (lines[lines.length()-1].contains("-->")) {
        QString formation;
        QRegExp r1( "[(](.*)[)]-->" );
        int pos = r1.indexIn( lines[lines.length()-1] );
        if ( pos > -1 ) {
            formation = r1.cap( 1 ); // "waves"
        }
        renderArea->setFormation(formation);
    }

    if (lastPrompt.contains("Enter startup command>")) {
        renderArea->setLayout1("");
        renderArea->setLayout2(QStringList());      // show squared up dancers
        renderArea->setFormation("Squared set");    // starting formation
        if (!copyrightShown) {
            currentSequenceWidget->setText(copyrightText + "\nSquared set\n\nTry: 'Heads start'");    // clear out current sequence
            copyrightShown = true;
        } else {
            currentSequenceWidget->setText("Squared set\n\nTry: 'Heads start'");    // clear out current sequence
        }
    } else {
        renderArea->setLayout1(lastLayout1);
        renderArea->setLayout2(lastFormatList);
    }

}

void MainWindow::readPSData()
{
    // ASR --------------------------------------------
    // pocketsphinx has a valid string, send it to sd
    QByteArray s = ps->readAll();

    int index = ui->tabWidget->currentIndex();
    if (!voiceInputEnabled || (currentApplicationState != Qt::ApplicationActive) || (ui->tabWidget->tabText(index) != "SD")) {
        // if voiceInput is explicitly disabled, or the app is not Active, we're not on the sd tab, then voiceInput is disabled,
        //  and we're going to read the data from PS and just throw it away.
        // This is a cheesy way to do it.  We really should disable the mics somehow.
        return;
    }

    // NLU --------------------------------------------
    // This section does the impedance match between what you can say and the exact wording that sd understands.
    // TODO: put this stuff into an external text file, read in at runtime?
    //
    QString s2 = s.toLower();
    s2.replace("\r\n","\n");  // for Windows PS only, harmless to Mac/Linux
    s2.replace(QRegExp("allocating .* buffers of .* samples each\\n"),"");  // garbage from windows PS only, harmless to Mac/Linux

    // handle quarter, a quarter, one quarter, half, one half, two quarters, three quarters
    // must be in this order, and must be before number substitution.
    s2 = s2.replace("once and a half","1-1/2");
    s2 = s2.replace("one and a half","1-1/2");
    s2 = s2.replace("two and a half","2-1/2");
    s2 = s2.replace("three and a half","3-1/2");
    s2 = s2.replace("four and a half","4-1/2");
    s2 = s2.replace("five and a half","5-1/2");
    s2 = s2.replace("six and a half","6-1/2");
    s2 = s2.replace("seven and a half","7-1/2");
    s2 = s2.replace("eight and a half","8-1/2");

    s2 = s2.replace("four quarters","4/4");
    s2 = s2.replace("three quarters","3/4").replace("three quarter","3/4");  // always replace the longer thing first!
    s2 = s2.replace("two quarters","2/4").replace("one half","1/2").replace("half","1/2");
    s2 = s2.replace("and a quarter more", "AND A QUARTER MORE");  // protect this.  Separate call, must NOT be "1/4"
    s2 = s2.replace("one quarter", "1/4").replace("a quarter", "1/4").replace("quarter","1/4");
    s2 = s2.replace("AND A QUARTER MORE", "and a quarter more");  // protect this.  Separate call, must NOT be "1/4"

    // handle 1P2P
    s2 = s2.replace("one pee two pee","1P2P");

    // handle "third" --> "3rd", for dixie grand
    s2 = s2.replace("third", "3rd");

    // handle numbers: one -> 1, etc.
    s2 = s2.replace("eight chain","EIGHT CHAIN"); // sd wants "eight chain 4", not "8 chain 4", so protect it
    s2 = s2.replace("one","1").replace("two","2").replace("three","3").replace("four","4");
    s2 = s2.replace("five","5").replace("six","6").replace("seven","7").replace("eight","8");
    s2 = s2.replace("EIGHT CHAIN","eight chain"); // sd wants "eight chain 4", not "8 chain 4", so protect it

    // handle optional words at the beginning

    s2 = s2.replace("do the centers", "DOTHECENTERS");  // special case "do the centers part of load the boat"
    s2 = s2.replace(QRegExp("^go "),"").replace(QRegExp("^do a "),"").replace(QRegExp("^do "),"");
    s2 = s2.replace("DOTHECENTERS", "do the centers");  // special case "do the centers part of load the boat"

    // handle specialized sd spelling of flutter wheel, and specialized wording of reverse flutter wheel
    s2 = s2.replace("flutterwheel","flutter wheel");
    s2 = s2.replace("reverse the flutter","reverse flutter wheel");

    // handle specialized sd wording of first go *, next go *
    s2 = s2.replace(QRegExp("first[a-z ]* go left[a-z ]* next[a-z ]* go right"),"first couple go left, next go right");
    s2 = s2.replace(QRegExp("first[a-z ]* go right[a-z ]* next[a-z ]* go left"),"first couple go right, next go left");
    s2 = s2.replace(QRegExp("first[a-z ]* go left[a-z ]* next[a-z ]* go left"),"first couple go left, next go left");
    s2 = s2.replace(QRegExp("first[a-z ]* go right[a-z ]* next[a-z ]* go right"),"first couple go right, next go right");

    // handle "single circle to an ocean wave" -> "single circle to a wave"
    s2 = s2.replace("single circle to an ocean wave","single circle to a wave");

    // handle manually-inserted brackets
    s2 = s2.replace(QRegExp("left bracket\\s+"), "[").replace(QRegExp("\\s+right bracket"),"]");

    // handle "single hinge" --> "hinge", "single file circulate" --> "circulate", "all 8 circulate" --> "circulate" (quirk of sd)
    s2 = s2.replace("single hinge", "hinge").replace("single file circulate", "circulate").replace("all 8 circulate", "circulate");

    // handle "men <anything>" and "ladies <anything>", EXCEPT for ladies chain
    if (!s2.contains("ladies chain") && !s2.contains("men chain")) {
        s2 = s2.replace("men", "boys").replace("ladies", "girls");  // wacky sd!
    }

    // handle "allemande left alamo style" --> "allemande left in the alamo style"
    s2 = s2.replace("allemande left alamo style", "allemande left in the alamo style");

    // handle "right a left thru" --> "right and left thru"
    s2 = s2.replace("right a left thru", "right and left thru");

    // handle "separate [go] around <n> [to a line]" --> delete "go"
    s2 = s2.replace("separate go around", "separate around");

    // handle "dixie style [to a wave|to an ocean wave]" --> "dixie style to a wave"
    s2 = s2.replace(QRegExp("dixie style.*"), "dixie style to a wave\n");

    // handle the <anything> and roll case
    //   NOTE: don't do anything, if we added manual brackets.  The user is in control in that case.
    if (!s2.contains("[")) {
        QRegExp andRollCall("(.*) and roll.*");
        if (s2.indexOf(andRollCall) != -1) {
            s2 = "[" + andRollCall.cap(1) + "] and roll\n";
        }

        // explode must be handled *after* roll, because explode binds tightly with the call
        // e.g. EXPLODE AND RIGHT AND LEFT THRU AND ROLL must be translated to:
        //      [explode and [right and left thru]] and roll

        // first, handle both: "explode and <anything> and roll"
        //  by the time we're here, it's already "[explode and <anything>] and roll\n", because
        //  we've already done the roll processing.
        QRegExp explodeAndRollCall("\\[explode and (.*)\\] and roll");
        QRegExp explodeAndNotRollCall("^explode and (.*)");

        if (s2.indexOf(explodeAndRollCall) != -1) {
            s2 = "[explode and [" + explodeAndRollCall.cap(1).trimmed() + "]] and roll\n";
        } else if (s2.indexOf(explodeAndNotRollCall) != -1) {
            // not a roll, for sure.  Must be a naked "explode and <anything>\n"
            s2 = "explode and [" + explodeAndNotRollCall.cap(1).trimmed() + "]\n";
        } else {
        }
    }

    // handle <ANYTHING> and spread
    if (!s2.contains("[")) {
        QRegExp andSpreadCall("(.*) and spread");
        if (s2.indexOf(andSpreadCall) != -1) {
            s2 = "[" + andSpreadCall.cap(1) + "] and spread\n";
        }
    }

    // handle "undo [that]" --> "undo last call"
    s2 = s2.replace("undo that", "undo last call");
    if (s2 == "undo\n") {
        s2 = "undo last call\n";
    }

    // handle "peel your top" --> "peel the top"
    s2 = s2.replace("peel your top", "peel the top");

    // handle "veer (to the) left/right" --> "veer left/right"
    s2 = s2.replace("veer to the", "veer");

    // handle the <anything> and sweep case
    // FIX: this needs to add center boys, etc, but that messes up the QRegExp

    // <ANYTHING> AND SWEEP (A | ONE) QUARTER [MORE]
    QRegExp andSweepPart(" and sweep.*");
    int found = s2.indexOf(andSweepPart);
    if (found != -1) {
        if (s2.contains("[")) {
            // if manual brackets added, don't add more of them.
            s2 = s2.replace(andSweepPart,"") + " and sweep 1/4\n";
        } else {
            s2 = "[" + s2.replace(andSweepPart,"") + "] and sweep 1/4\n";
        }
    }

    // handle "square thru" -> "square thru 4"
    if (s2 == "square thru\n") {
        s2 = "square thru 4\n";
    }

    // SD COMMANDS -------
    // square your|the set -> square thru 4
    if (s2 == "square the set\n" || s2 == "square your set\n") {
#if defined(Q_OS_MAC)
        sd->write(QByteArray("\x15"));  // send a Ctrl-U to clear the current user string
#endif
#if defined(Q_OS_WIN32) | defined(Q_OS_LINUX)
        sd->write(QByteArray("\x1B"));  // send an ESC to clear the current user string
#endif
        sd->waitForBytesWritten();

        console->clear();

        sd->write("abort this sequence\n");
        sd->waitForBytesWritten();

        sd->write("y\n");
        sd->waitForBytesWritten();

    } else if (s2 == "undo last call\n") {
        // TODO: put more synonyms of this in...
#if defined(Q_OS_MAC)
        sd->write(QByteArray("\x15"));  // send a Ctrl-U to clear the current user string
#endif
#if defined(Q_OS_WIN32) | defined(Q_OS_LINUX)
        sd->write(QByteArray("\x1B"));  // send an ESC to clear the current user string
#endif
        sd->waitForBytesWritten();

        sd->write("undo last call\n");  // back up one call
        sd->waitForBytesWritten();

        sd->write("refresh display\n");  // refresh
        sd->waitForBytesWritten();
    } else if (s2 == "erase\n" || s2 == "erase that\n") {
        // TODO: put more synonyms in, e.g. "cancel that"
#if defined(Q_OS_MAC)
        sd->write(QByteArray("\x15"));  // send a Ctrl-U to clear the current user string
#endif
#if defined(Q_OS_WIN32) | defined(Q_OS_LINUX)
        sd->write(QByteArray("\x1B"));  // send an ESC to clear the current user string
#endif
        sd->waitForBytesWritten();
    } else if (s2 != "\n") {
        sd->write(s2.toLatin1());
        sd->waitForBytesWritten();
    }
}

void MainWindow::showContextMenu(const QPoint &pt)
{
    QMenu *menu = currentSequenceWidget->createStandardContextMenu();
    menu->exec(currentSequenceWidget->mapToGlobal(pt));
    delete menu;
}

void MainWindow::initSDtab() {

#ifndef POCKETSPHINXSUPPORT
    ui->actionEnable_voice_input->setEnabled(false);  // if no PS support, grey out the menu item
#endif

    renderArea = new RenderArea;
    renderArea->setPen(QPen(Qt::blue));
    renderArea->setBrush(QBrush(Qt::green));

    console = new Console;
    console->setEnabled(true);
    console->setLocalEchoEnabled(true);
    console->setFixedHeight(100);

    currentSequenceWidget = new QTextEdit();
    currentSequenceWidget->setStyleSheet("QLabel { background-color : white; color : black; }");
    currentSequenceWidget->setAlignment(Qt::AlignTop);
    currentSequenceWidget->setReadOnly(true);
    currentSequenceWidget->setFocusPolicy(Qt::NoFocus);  // do not allow this widget to get focus (console always has it)

    // allow for cut/paste from the sequence window using Right-click
    currentSequenceWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(currentSequenceWidget,SIGNAL(customContextMenuRequested(const QPoint&)),
            this,SLOT(showContextMenu(const QPoint &)));

    ui->seqGridLayout->addWidget(currentSequenceWidget,0,0,1,1);
    ui->seqGridLayout->addWidget(console, 1,0,1,2);
    ui->seqGridLayout->addWidget(renderArea, 0,1);

//    console->setFixedHeight(150);

    // POCKET_SPHINX -------------------------------------------
    //    WHICH=5365
    //    pocketsphinx_continuous -dict $WHICH.dic -lm $WHICH.lm -inmic yes
    // MAIN CMU DICT: /usr/local/Cellar/cmu-pocketsphinx/HEAD-584be6e/share/pocketsphinx/model/en-us
    // TEST DIR: /Users/mpogue/Documents/QtProjects/SquareDeskPlayer/build-SquareDesk-Desktop_Qt_5_7_0_clang_64bit-Debug/test123/SquareDeskPlayer.app/Contents/MacOS
    // TEST PS MANUALLY: pocketsphinx_continuous -dict 5365a.dic -jsgf plus.jsgf -inmic yes -hmm ../models/en-us
    //   also try: -remove_noise yes, as per http://stackoverflow.com/questions/25641154/noise-reduction-before-pocketsphinx-reduces-recognition-accuracy
    // TEST SD MANUALLY: ./sd
    QString danceLevel = "plus"; // one of sd's names: {basic, mainstream, plus, a1, a2, c1, c2, c3a}

#if defined(POCKETSPHINXSUPPORT)
    unsigned int whichModel = 5365;
    QString appDir = QCoreApplication::applicationDirPath() + "/";  // this is where the actual ps executable is
    QString pathToPS = appDir + "pocketsphinx_continuous";

#if defined(Q_OS_WIN32)
    pathToPS += ".exe";   // executable has a different name on Win32
#endif

    // NOTE: <whichmodel>a.dic and <dancelevel>.jsgf MUST be in the same directory.
    QString pathToDict = QString::number(whichModel) + "a.dic";
    QString pathToJSGF = danceLevel + ".jsgf";

#if defined(Q_OS_MAC)
    // The acoustic models are one level up in the models subdirectory on MAC
    QString pathToHMM  = "../models/en-us";
#endif
#if defined(Q_OS_WIN32)
    // The acoustic models are at the same level, but in the models subdirectory on MAC
    QString pathToHMM  = "models/en-us";
#endif

    QStringList PSargs;
    PSargs << "-dict" << pathToDict     // pronunciation dictionary
           << "-jsgf" << pathToJSGF     // language model
           << "-inmic" << "yes"         // use the built-in microphone
           << "-remove_noise" << "yes"  // try turning on PS noise reduction
           << "-hmm" << pathToHMM;      // the US English acoustic model (a bunch of files) is in ../models/en-us

    ps = new QProcess(Q_NULLPTR);

    ps->setWorkingDirectory(QCoreApplication::applicationDirPath()); // NOTE: nothing will be written here
    ps->start(pathToPS, PSargs);

    ps->waitForStarted();

    connect(ps,   &QProcess::readyReadStandardOutput,
            this, &MainWindow::readPSData);                 // output data from ps

    // SD -------------------------------------------
    copyrightShown = false;  // haven't shown it once yet

#if defined(Q_OS_MAC)
    // NOTE: sd and sd_calls.dat must be in the same directory in the SDP bundle (Mac OS X).
    QString pathToSD          = QCoreApplication::applicationDirPath() + "/sd";
    QString pathToSD_CALLSDAT = QCoreApplication::applicationDirPath() + "/sd_calls.dat";
#else
    // NOTE: sd and sd_calls.dat must be in the same directory as SquareDeskPlayer.exe (Win32).
    QString pathToSD          = QCoreApplication::applicationDirPath() + "/sdtty.exe";
    QString pathToSD_CALLSDAT = QCoreApplication::applicationDirPath() + "/sd_calls.dat";
#endif

    // start sd as a process -----
    QStringList SDargs;
//    SDargs << "-help";  // this is an excellent place to start!
//    SDargs << "-no_color" << "-no_cursor" << "-no_console" << "-no_graphics" // act as server only
      SDargs << "-no_color" << "-no_cursor" << "-no_graphics" // act as server only. TEST WIN32
           << "-lines" << "1000"
           << "-db" << pathToSD_CALLSDAT                        // sd_calls.dat file is in same directory as sd
           << danceLevel;                                       // default level for sd
    sd = new QProcess(Q_NULLPTR);

    // Let's make an "sd" directory in the Music Directory
    //  This will be used for storing sequences (until we move that into SQLite).
    PreferencesManager prefsManager;
    QString sequencesDir = prefsManager.GetmusicPath() + "/sd";

    // if the sequences directory doesn't exist, create it
    QDir dir(sequencesDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    sd->setWorkingDirectory(sequencesDir);
    sd->setProcessChannelMode(QProcess::MergedChannels);
    sd->start(pathToSD, SDargs);

    if (sd->waitForStarted() == false) {
        qDebug() << "ERROR: sd did not start properly.";
    } else {
//        qDebug() << "sd started.";
    }

    connect(sd, &QProcess::readyReadStandardOutput, this, &MainWindow::readSDData);  // output data from sd
    connect(console, &Console::getData, this, &MainWindow::writeSDData);      // input data to sd

    highlighter = new Highlighter(console->document());
#endif
}

void MainWindow::on_actionEnable_voice_input_toggled(bool checked)
{
    if (checked) {
        ui->actionEnable_voice_input->setChecked(true);
        voiceInputEnabled = true;
    }
    else {
        ui->actionEnable_voice_input->setChecked(false);
        voiceInputEnabled = false;
    }

    microphoneStatusUpdate();

    // the Enable Voice Input setting is persistent across restarts of the application
    PreferencesManager prefsManager;
    prefsManager.Setenablevoiceinput(ui->actionEnable_voice_input->isChecked());
}

void MainWindow::on_actionAuto_scroll_during_playback_toggled(bool checked)
{
    if (checked) {
        ui->actionAuto_scroll_during_playback->setChecked(true);
        autoScrollLyricsEnabled = true;
    }
    else {
        ui->actionAuto_scroll_during_playback->setChecked(false);
        autoScrollLyricsEnabled = false;
    }

    // the Enable Auto-scroll during playback setting is persistent across restarts of the application
    PreferencesManager prefsManager;
    prefsManager.Setenableautoscrolllyrics(ui->actionAuto_scroll_during_playback->isChecked());
}

void MainWindow::on_actionAt_TOP_triggered()    // Add > at TOP
{
    PlaylistItemToTop();
}

void MainWindow::on_actionAt_BOTTOM_triggered()  // Add > at BOTTOM
{
    PlaylistItemToBottom();
}

void MainWindow::on_actionRemove_from_Playlist_triggered()
{
    PlaylistItemRemove();
}

void MainWindow::on_actionUP_in_Playlist_triggered()
{
    PlaylistItemMoveUp();
}

void MainWindow::on_actionDOWN_in_Playlist_triggered()
{
    PlaylistItemMoveDown();
}

void MainWindow::on_actionStartup_Wizard_triggered()
{
    StartupWizard wizard;
    int dialogCode = wizard.exec();

    if(dialogCode == QDialog::Accepted) {
        // must setup internal variables, from updated Preferences..
        PreferencesManager prefsManager; // Will be using application information for correct location of your settings

        musicRootPath = prefsManager.GetmusicPath();

        QString value;
        value = prefsManager.GetMusicTypeSinging();
        songTypeNamesForSinging = value.toLower().split(";", QString::KeepEmptyParts);

        value = prefsManager.GetMusicTypePatter();
        songTypeNamesForPatter = value.toLower().split(";", QString::KeepEmptyParts);

        value = prefsManager.GetMusicTypeExtras();
        songTypeNamesForExtras = value.toLower().split(';', QString::KeepEmptyParts);

        value = prefsManager.GetMusicTypeCalled();
        songTypeNamesForCalled = value.toLower().split(';', QString::KeepEmptyParts);

        // used to store the file paths
        findMusic(musicRootPath,"","main", true);  // get the filenames from the user's directories
        filterMusic(); // and filter them into the songTable
        loadMusicList();

        // install soundFX if not already present
        maybeInstallSoundFX();

        // FIX: When SD directory is changed, we need to kill and restart SD, or SD output will go to the old directory.
        // initSDtab();  // sd directory has changed, so startup everything again.
    }
}

void MainWindow::sortByDefaultSortOrder()
{
    // these must be in "backwards" order to get the right order, which
    //   is that Type is primary, Title is secondary, Label is tertiary
    ui->songTable->sortItems(kLabelCol);  // sort last by label/label #
    ui->songTable->sortItems(kTitleCol);  // sort second by title in alphabetical order
    ui->songTable->sortItems(kTypeCol);   // sort first by type (singing vs patter)
}

void MainWindow::sdActionTriggered(QAction * action) {
//    qDebug() << "***** sdActionTriggered()" << action << action->isChecked();
    action->setChecked(true);  // check the new one
    renderArea->setCoupleColoringScheme(action->text());
}

void MainWindow::airplaneMode(bool turnItOn) {
#if defined(Q_OS_MAC)
    char cmd[100];
    if (turnItOn) {
        sprintf(cmd, "osascript -e 'do shell script \"networksetup -setairportpower en0 off\"'\n");
    } else {
        sprintf(cmd, "osascript -e 'do shell script \"networksetup -setairportpower en0 on\"'\n");
    }
    system(cmd);
#endif
}

void MainWindow::on_action_1_triggered()
{
    playSFX("1");
}

void MainWindow::on_action_2_triggered()
{
    playSFX("2");
}

void MainWindow::on_action_3_triggered()
{
    playSFX("3");
}

void MainWindow::on_action_4_triggered()
{
    playSFX("4");
}

void MainWindow::on_action_5_triggered()
{
    playSFX("5");
}

void MainWindow::on_action_6_triggered()
{
    playSFX("6");
}

void MainWindow::playSFX(QString which) {
    QString soundEffectFile;

    if (which.toInt() > 0) {
        soundEffectFile = soundFXarray[which.toInt()-1];
    } else {
        // conversion failed, this is break_end or long_tip.mp3
        soundEffectFile = musicRootPath + "/soundfx/" + which + ".mp3";
    }

    if(QFileInfo(soundEffectFile).exists()) {
        // play sound FX only if file exists...
        cBass.PlayOrStopSoundEffect(which.toInt(),
                                    soundEffectFile.toLocal8Bit().constData());  // convert to C string; defaults to volume 100%
    }
}

void MainWindow::on_actionClear_Recent_List_triggered()
{
    QSettings settings;
    QStringList recentFilePaths;  // empty list

    settings.setValue("recentFiles", recentFilePaths);  // remember the new list
    updateRecentPlaylistMenu();
}

void MainWindow::loadRecentPlaylist(int i) {

    on_stopButton_clicked();  // if we're loading a new PLAYLIST file, stop current playback

    QSettings settings;
    QStringList recentFilePaths = settings.value("recentFiles").toStringList();

    if (i < recentFilePaths.size()) {
        // then we can do it
        QString filename = recentFilePaths.at(i);
        finishLoadingPlaylist(filename);

        addFilenameToRecentPlaylist(filename);
    }
}

void MainWindow::updateRecentPlaylistMenu() {
    QSettings settings;
    QStringList recentFilePaths = settings.value("recentFiles").toStringList();

    int numRecentPlaylists = recentFilePaths.length();
    ui->actionRecent1->setVisible(numRecentPlaylists >=1);
    ui->actionRecent2->setVisible(numRecentPlaylists >=2);
    ui->actionRecent3->setVisible(numRecentPlaylists >=3);
    ui->actionRecent4->setVisible(numRecentPlaylists >=4);

    QString playlistsPath = musicRootPath + "/playlists/";

    switch(numRecentPlaylists) {
        case 4: ui->actionRecent4->setText(QString(recentFilePaths.at(3)).replace(playlistsPath,""));  // intentional fall-thru
        case 3: ui->actionRecent3->setText(QString(recentFilePaths.at(2)).replace(playlistsPath,""));  // intentional fall-thru
        case 2: ui->actionRecent2->setText(QString(recentFilePaths.at(1)).replace(playlistsPath,""));  // intentional fall-thru
        case 1: ui->actionRecent1->setText(QString(recentFilePaths.at(0)).replace(playlistsPath,""));  // intentional fall-thru
        default: break;
    }

    ui->actionClear_Recent_List->setEnabled(numRecentPlaylists > 0);
}

void MainWindow::addFilenameToRecentPlaylist(QString filename) {
    if (!filename.endsWith(".squaredesk/current.m3u")) {  // do not remember the initial persistent playlist
        QSettings settings;
        QStringList recentFilePaths = settings.value("recentFiles").toStringList();

        recentFilePaths.removeAll(filename);  // remove if it exists already
        recentFilePaths.prepend(filename);    // push it onto the front
        while (recentFilePaths.size() > 4) {  // get rid of those that fell off the end
            recentFilePaths.removeLast();
        }

        settings.setValue("recentFiles", recentFilePaths);  // remember the new list
        updateRecentPlaylistMenu();
    }
}

void MainWindow::on_actionRecent1_triggered()
{
    loadRecentPlaylist(0);
}

void MainWindow::on_actionRecent2_triggered()
{
    loadRecentPlaylist(1);
}

void MainWindow::on_actionRecent3_triggered()
{
    loadRecentPlaylist(2);
}

void MainWindow::on_actionRecent4_triggered()
{
    loadRecentPlaylist(3);
}

void MainWindow::on_actionCheck_for_Updates_triggered()
{
    QString latestVersionURL = "https://raw.githubusercontent.com/mpogue2/SquareDesk/master/latest";

    QNetworkAccessManager* manager = new QNetworkAccessManager();

    QUrl murl = latestVersionURL;
    QNetworkReply *reply = manager->get(QNetworkRequest(murl));

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit())); // just in case
    timer.start(5000);

    loop.exec();

    QByteArray result = reply->readAll();

    if ( reply->error() != QNetworkReply::NoError ) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("<B>ERROR</B><P>Sorry, the GitHub server is not reachable right now.<P>" + reply->errorString());
        msgBox.exec();
        return;
    }

    // "latest" is of the form "X.Y.Z\n", so trim off the NL
    QString latestVersionNumber = QString(result).trimmed();

    if (latestVersionNumber == VERSIONSTRING) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("<B>You are running the latest version of SquareDesk.</B>");
        msgBox.exec();
    } else {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("<H2>Newer version available</H2>\nYour version of SquareDesk: " + QString(VERSIONSTRING) +
                       "<P>Latest version of SquareDesk: " + latestVersionNumber +
                       "<P><a href=\"https://github.com/mpogue2/SquareDesk/releases\">Download new version</a>");
        msgBox.exec();
    }

}

void MainWindow::maybeInstallSoundFX() {

#if defined(Q_OS_MAC)
    QString pathFromAppDirPathToStarterSet = "/../soundfx";
#elif defined(Q_OS_WIN32)
    // WARNING: untested
    QString pathFromAppDirPathToStarterSet = "/soundfx";
#elif defined(Q_OS_LINUX)
    // WARNING: untested
    QString pathFromAppDirPathToStarterSet = "/soundfx";
#endif

    // Let's make a "soundfx" directory in the Music Directory, if it doesn't exist already
    PreferencesManager prefsManager;
    QString musicDirPath = prefsManager.GetmusicPath();
    QString soundfxDir = musicDirPath + "/soundfx";

    // if the soundfx directory doesn't exist, create it (always, automatically)
    QDir dir(soundfxDir);
    if (!dir.exists()) {
        dir.mkpath(".");  // make it
    }

    // and populate it with the starter set, if it didn't exist already
    // check for break_over.mp3 and copy it in, if it doesn't exist already
    QFile breakOverSound(soundfxDir + "/break_over.mp3");
    if (!breakOverSound.exists()) {
        QString source = QCoreApplication::applicationDirPath() + pathFromAppDirPathToStarterSet + "/break_over.mp3";
        QString destination = soundfxDir + "/break_over.mp3";
        //            qDebug() << "COPY from: " << source << " to " << destination;
        QFile::copy(source, destination);
    }

    // check for long_tip.mp3 and copy it in, if it doesn't exist already
    QFile longTipSound(soundfxDir + "/long_tip.mp3");
    if (!longTipSound.exists()) {
        QString source = QCoreApplication::applicationDirPath() + pathFromAppDirPathToStarterSet + "/long_tip.mp3";
        QString destination = soundfxDir + "/long_tip.mp3";
        QFile::copy(source, destination);
    }

    // check for individual soundFX files, and copy in a starter-set sound, if one doesn't exist,
    // which is checked when the App starts, and when the Startup Wizard finishes setup.  In which case, we
    //   only want to copy files into the soundfx directory if there aren't already files there.
    //   It might seem like overkill right now (and it is, right now).
    bool foundSoundFXfile[6];
    for (int i=0; i<6; i++) {
        foundSoundFXfile[i] = false;
    }

    QDirIterator it(soundfxDir);
    while(it.hasNext()) {
        QString s1 = it.next();

        QString baseName = s1.replace(soundfxDir,"");
        QStringList sections = baseName.split(".");

        if (sections.length() == 3 && sections[0].toInt() != 0 && sections[2] == "mp3") {
            foundSoundFXfile[sections[0].toInt() - 1] = true;  // found file of the form <N>.<something>.mp3
        }
    } // while

    QString starterSet[6] = {
        "1.whistle.mp3",
        "2.clown_honk.mp3",
        "3.submarine.mp3",
        "4.applause.mp3",
        "5.fanfare.mp3",
        "6.fade.mp3"
    };
    for (int i=0; i<6; i++) {
        if (!foundSoundFXfile[i]) {
            QString destination = soundfxDir + "/" + starterSet[i];
            QFile dest(destination);
            if (!dest.exists()) {
                QString source = QCoreApplication::applicationDirPath() + pathFromAppDirPathToStarterSet + "/" + starterSet[i];
                QFile::copy(source, destination);
            }
        }
    } // for

}

void MainWindow::on_actionStop_Sound_FX_triggered()
{
    // whatever SFX are playing, stop them.
    cBass.StopAllSoundEffects();
}

void MainWindow::adjustFontSizes()
{
    QFont currentFont = ui->songTable->font();
    int currentFontPointSize = currentFont.pointSize();

//    qDebug() << "currentFontPointSize: " << currentFontPointSize;

//    ui->songTable->resizeColumnToContents(0);  // nope
    ui->songTable->resizeColumnToContents(1);
    ui->songTable->resizeColumnToContents(2);
    // 3/4/5/6 = nope

    ui->songTable->setColumnWidth(4, 3.5*currentFontPointSize);
    ui->songTable->setColumnWidth(5, 3.5*currentFontPointSize);
    ui->songTable->setColumnWidth(6, 4*currentFontPointSize);

    ui->typeSearch->setFixedHeight(2*currentFontPointSize);
    ui->labelSearch->setFixedHeight(2*currentFontPointSize);
    ui->titleSearch->setFixedHeight(2*currentFontPointSize);

    // set all the related fonts to the same size
    ui->typeSearch->setFont(currentFont);
    ui->labelSearch->setFont(currentFont);
    ui->titleSearch->setFont(currentFont);

    ui->tempoLabel->setFont(currentFont);
    ui->pitchLabel->setFont(currentFont);
    ui->volumeLabel->setFont(currentFont);
    ui->mixLabel->setFont(currentFont);

    currentSequenceWidget->setFont(currentFont); // In the SD tab, follow the user-selected font size

#define CURRENTSCALE (7.75)
    int newCurrentWidth = CURRENTSCALE*currentFontPointSize;
    ui->currentTempoLabel->setFont(currentFont);
    ui->currentTempoLabel->setFixedWidth(newCurrentWidth);
    ui->currentPitchLabel->setFont(currentFont);
    ui->currentPitchLabel->setFixedWidth(newCurrentWidth);
    ui->currentVolumeLabel->setFont(currentFont);
    ui->currentVolumeLabel->setFixedWidth(newCurrentWidth);
    ui->currentMixLabel->setFont(currentFont);
    ui->currentMixLabel->setFixedWidth(newCurrentWidth);

    ui->statusBar->setFont(currentFont);
    ui->currentLocLabel->setFont(currentFont);
    ui->songLengthLabel->setFont(currentFont);

    ui->currentLocLabel->setFixedWidth(3.0*currentFontPointSize);
    ui->songLengthLabel->setFixedWidth(3.0*currentFontPointSize);

    ui->clearSearchButton->setFont(currentFont);
    ui->clearSearchButton->setFixedWidth(8*currentFontPointSize);

    ui->tabWidget->setFont(currentFont);  // most everything inherits from this one

    // these are special -- don't want them to get too big, even if user requests huge fonts
    currentFont.setPointSize(currentFontPointSize > 16 ? 16 : currentFontPointSize);  // no bigger than 20pt
    ui->bassLabel->setFont(currentFont);
    ui->midrangeLabel->setFont(currentFont);
    ui->trebleLabel->setFont(currentFont);
    ui->EQgroup->setFont(currentFont);

    // resize the icons for the buttons
    int newIconDimension = (int)((float)currentFontPointSize*(24.0/13.0));
    QSize newIconSize(newIconDimension, newIconDimension);
    ui->stopButton->setIconSize(newIconSize);
    ui->playButton->setIconSize(newIconSize);
    ui->previousSongButton->setIconSize(newIconSize);
    ui->nextSongButton->setIconSize(newIconSize);

    // these are special MEDIUM
    int warningLabelFontSize = ((int)((float)currentFontPointSize * (preferredNowPlayingSize+preferredSmallFontSize)/2.0)/preferredSmallFontSize); // keep ratio constant
    currentFont.setPointSize((warningLabelFontSize));
    ui->warningLabel->setFont(currentFont);
    ui->warningLabel->setFixedWidth(5.5*warningLabelFontSize);

    // these are special BIG
    int nowPlayingLabelFontSize = ((int)((float)currentFontPointSize * (preferredNowPlayingSize/preferredSmallFontSize))); // keep ratio constant
    currentFont.setPointSize(nowPlayingLabelFontSize);
    ui->nowPlayingLabel->setFont(currentFont);
    ui->nowPlayingLabel->setFixedHeight(1.25*nowPlayingLabelFontSize);

#define BUTTONSCALE (0.75)
    ui->stopButton->setFixedSize(2.5*BUTTONSCALE*nowPlayingLabelFontSize,1.5*BUTTONSCALE*nowPlayingLabelFontSize);
    ui->playButton->setFixedSize(2.5*BUTTONSCALE*nowPlayingLabelFontSize,1.5*BUTTONSCALE*nowPlayingLabelFontSize);
    ui->previousSongButton->setFixedSize(2.5*BUTTONSCALE*nowPlayingLabelFontSize,1.5*BUTTONSCALE*nowPlayingLabelFontSize);
    ui->nextSongButton->setFixedSize(2.5*BUTTONSCALE*nowPlayingLabelFontSize,1.5*BUTTONSCALE*nowPlayingLabelFontSize);
}

#define SMALLESTZOOM (11)
#define BIGGESTZOOM (25)
#define ZOOMINCREMENT (2)

void MainWindow::usePersistentFontSize() {
    PreferencesManager prefsManager;

    int newPointSize = prefsManager.GetsongTableFontSize(); // gets the persisted value
    if (newPointSize == 0) {
        newPointSize = 13;  // default backstop, if not set properly
    }

    // ensure it is a reasonable size...
    newPointSize = (newPointSize > BIGGESTZOOM ? BIGGESTZOOM : newPointSize);
    newPointSize = (newPointSize < SMALLESTZOOM ? SMALLESTZOOM : newPointSize);

    //qDebug() << "usePersistentFontSize: " << newPointSize;

    QFont currentFont = ui->songTable->font();  // set the font size in the songTable
    currentFont.setPointSize(newPointSize);
    ui->songTable->setFont(currentFont);

    adjustFontSizes();  // use that font size to scale everything else (relative)
}


void MainWindow::persistNewFontSize(int points) {
    PreferencesManager prefsManager;

    prefsManager.SetsongTableFontSize(points);  // persist this
//    qDebug() << "persisting new font size: " << points;
}

void MainWindow::on_actionZoom_In_triggered()
{
    QFont currentFont = ui->songTable->font();
    int newPointSize = currentFont.pointSize() + ZOOMINCREMENT;
    newPointSize = (newPointSize > BIGGESTZOOM ? BIGGESTZOOM : newPointSize);
    newPointSize = (newPointSize < SMALLESTZOOM ? SMALLESTZOOM : newPointSize);

    if (newPointSize > currentFont.pointSize()) {
        ui->textBrowserCueSheet->zoomIn(2*ZOOMINCREMENT);
        totalZoom += 2*ZOOMINCREMENT;
    }

    currentFont.setPointSize(newPointSize);
    ui->songTable->setFont(currentFont);

    persistNewFontSize(newPointSize);

    adjustFontSizes();
}

void MainWindow::on_actionZoom_Out_triggered()
{
    QFont currentFont = ui->songTable->font();
    int newPointSize = currentFont.pointSize() - ZOOMINCREMENT;
    newPointSize = (newPointSize > BIGGESTZOOM ? BIGGESTZOOM : newPointSize);
    newPointSize = (newPointSize < SMALLESTZOOM ? SMALLESTZOOM : newPointSize);

    if (newPointSize < currentFont.pointSize()) {
        ui->textBrowserCueSheet->zoomOut(2*ZOOMINCREMENT);
        totalZoom -= 2*ZOOMINCREMENT;
    }

    currentFont.setPointSize(newPointSize);
    ui->songTable->setFont(currentFont);

    persistNewFontSize(newPointSize);

    adjustFontSizes();
}

void MainWindow::on_actionReset_triggered()
{
    QFont currentFont;  // system font, and system default point size
    int newPointSize = currentFont.pointSize();  // start out with the default system font size
    currentFont.setPointSize(newPointSize);
    ui->songTable->setFont(currentFont);

    ui->textBrowserCueSheet->zoomOut(totalZoom);  // undo all zooming in the lyrics pane
    totalZoom = 0;

    persistNewFontSize(newPointSize);
    adjustFontSizes();
}

void MainWindow::on_actionAge_toggled(bool checked)
{
    ui->actionAge->setChecked(checked);  // when this function is called at constructor time, preferences sets the checkmark

    // the showAgeColumn setting is persistent across restarts of the application
    PreferencesManager prefsManager;
    prefsManager.SetshowAgeColumn(checked);

    updateSongTableColumnView();
}

void MainWindow::on_actionPitch_toggled(bool checked)
{
    ui->actionPitch->setChecked(checked);  // when this function is called at constructor time, preferences sets the checkmark

    // the showAgeColumn setting is persistent across restarts of the application
    PreferencesManager prefsManager;
    prefsManager.SetshowPitchColumn(checked);

    updateSongTableColumnView();
}

void MainWindow::on_actionTempo_toggled(bool checked)
{
    ui->actionTempo->setChecked(checked);  // when this function is called at constructor time, preferences sets the checkmark

    // the showAgeColumn setting is persistent across restarts of the application
    PreferencesManager prefsManager;
    prefsManager.SetshowTempoColumn(checked);

    updateSongTableColumnView();
}

void MainWindow::on_actionFade_Out_triggered()
{
    cBass.FadeOutAndPause();
}

// For improving as well as measuring performance of long songTable operations
// The hide() really gets most of the benefit:
//
//                                          no hide()   with hide()
// loadMusicList                             129ms      112ms
// finishLoadingPlaylist (50 items list)    3227ms     1154ms
// clear playlist                           2119ms      147ms
//
void MainWindow::startLongSongTableOperation(QString s) {
    Q_UNUSED(s)
//    t1.start();  // DEBUG

    ui->songTable->hide();
    ui->songTable->setSortingEnabled(false);
}

void MainWindow::stopLongSongTableOperation(QString s) {
    Q_UNUSED(s)

    ui->songTable->setSortingEnabled(true);
    ui->songTable->show();

//    qDebug() << s << ": " << t1.elapsed() << "ms.";  // DEBUG
}


void MainWindow::on_actionDownload_Cuesheets_triggered()
{
#if defined(Q_OS_MAC)

    PreferencesManager prefsManager;
    QString musicDirPath = prefsManager.GetmusicPath();
    QString lyricsDirPath = musicDirPath + "/lyrics";

    QDir lyricsDir(lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME);

    if (lyricsDir.exists()) {
//        qDebug() << "You already have the latest cuesheets downloaded.  Are you sure you want to download them again (erasing any edits you made)?";

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(QString("You already have the latest lyrics files: '") + CURRENTSQVIEWLYRICSNAME + "'");
        msgBox.setInformativeText("Are you sure?  This will overwrite any lyrics that you have edited in that folder.");
        QPushButton *downloadButton = msgBox.addButton(tr("Download Anyway"), QMessageBox::AcceptRole);
        QPushButton *abortButton = msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
        msgBox.exec();

        if (msgBox.clickedButton() == downloadButton) {
            // Download
//            qDebug() << "DOWNLOAD WILL PROCEED NORMALLY.";
        } else if (msgBox.clickedButton() == abortButton) {
            // Abort
//            qDebug() << "ABORTING DOWNLOAD.";
            return;
        }
    }

    Downloader *d = new Downloader(this);

    QUrl lyricsZipFileURL(QString("https://raw.githubusercontent.com/mpogue2/SquareDesk/master/") + CURRENTSQVIEWLYRICSNAME + QString(".zip"));  // FIX: hard-coded for now
//    qDebug() << "url to download:" << lyricsZipFileURL.toDisplayString();

    QString lyricsZipFileName = lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME + ".zip";

    d->doDownload(lyricsZipFileURL, lyricsZipFileName);  // download URL and put it into lyricsZipFileName

    QObject::connect(d,SIGNAL(downloadFinished()), this, SLOT(lyricsDownloadEnd()));
    QObject::connect(d,SIGNAL(downloadFinished()), d, SLOT(deleteLater()));

    // PROGRESS BAR ---------------------
    // assume ~10MB
    progressDialog = new QProgressDialog("Downloading lyrics...", "Cancel Download", 0, 100, this);
    connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelProgress()));
    progressTimer = new QTimer(this);
    connect(progressTimer, SIGNAL(timeout()), this, SLOT(makeProgress()));
    progressTotal = 0.0;
    progressTimer->start(500);  // once per second
#endif
}

void MainWindow::makeProgress() {
#if defined(Q_OS_MAC)
    if (progressTotal < 80) {
        progressTotal += 10.0;
    } else if (progressTotal < 90) {
        progressTotal += 1.0;
    } else if (progressTotal < 98) {
        progressTotal += 0.25;
    } // else no progress for you.

//    qDebug() << "making progress..." << progressTotal;

    progressDialog->setValue((unsigned int)progressTotal);
#endif
}

void MainWindow::cancelProgress() {
#if defined(Q_OS_MAC)
//    qDebug() << "cancelling progress...";
    progressTimer->stop();
    progressTotal = 0;
#endif
}


void MainWindow::lyricsDownloadEnd() {
#if defined(Q_OS_MAC)
//    qDebug() << "MainWindow::lyricsDownloadEnd() -- Download done:";

//    qDebug() << "UNPACKING ZIP FILE INTO LYRICS DIRECTORY...";
    PreferencesManager prefsManager;
    QString musicDirPath = prefsManager.GetmusicPath();
    QString lyricsDirPath = musicDirPath + "/lyrics";
    QString lyricsZipFileName = lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME + ".zip";

    QString destinationDir = lyricsDirPath;

    // extract the ZIP file
    QStringList extracted = JlCompress::extractDir(lyricsZipFileName, destinationDir); // extracts /root/lyrics/SqView_xxxxxx.zip to /root/lyrics/Text

    if (extracted.empty()) {
//        qDebug() << "There was a problem extracting the files.  No files extracted.";
        return;  // and don't delete the ZIP file, for debugging
    }

//    qDebug() << "DELETING ZIP FILE...";
    QFile file(lyricsZipFileName);
    file.remove();

    QDir currentLyricsDir(lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME);
    if (currentLyricsDir.exists()) {
//        qDebug() << "Refused to overwrite existing cuesheets, renamed to: " << lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME + "_backup";
        QFile::rename(lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME, lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME + "_backup");
    }

//    qDebug() << "RENAMING Text/ TO SqViewCueSheets_2017.03.14/ ...";
    QFile::rename(lyricsDirPath + "/Text", lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME);

    // RESCAN THE ENTIRE MUSIC DIRECTORY FOR LYRICS ------------
    findMusic(musicRootPath,"","main", true);  // get the filenames from the user's directories
    loadMusicList(); // and filter them into the songTable

//    qDebug() << "DONE DOWNLOADING LATEST LYRICS: " << CURRENTSQVIEWLYRICSNAME << "\n";

    progressDialog->setValue(100);  // kill the progress bar
    progressTimer->stop();
#endif
}

void MainWindow::on_actionPrint_Lyrics_triggered()
{
    QPrinter printer;
    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec() == QDialog::Rejected) {
        return;
    }
    ui->textBrowserCueSheet->print(&printer);
}

void MainWindow::on_actionSave_Lyrics_triggered()
{
    // Save cuesheet to the current cuesheet filename...
    RecursionGuard dialog_guard(inPreferencesDialog);

    QString cuesheetFilename = ui->comboBoxCuesheetSelector->itemData(ui->comboBoxCuesheetSelector->currentIndex()).toString();
    if (!cuesheetFilename.isNull())
    {
        writeCuesheet(cuesheetFilename);
        loadCuesheets(currentMP3filenameWithPath, cuesheetFilename);
        saveCurrentSongSettings();
    }

}

void MainWindow::on_actionSave_Lyrics_As_triggered()
{
    // Ask me where to save it...
    RecursionGuard dialog_guard(inPreferencesDialog);
    QFileInfo fi(currentMP3filenameWithPath);

    if (lastCuesheetSavePath.isEmpty())
        lastCuesheetSavePath = musicRootPath;

    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save Cue Sheet"),
                                                    lastCuesheetSavePath + "/" + fi.completeBaseName() + ".html",
                                                    tr("HTML (*.html)"));
    if (!filename.isNull())
    {
        writeCuesheet(filename);
        loadCuesheets(currentMP3filenameWithPath, filename);
        saveCurrentSongSettings();
    }
}

