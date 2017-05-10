/****************************************************************************
**
** Copyright (C) 2016, 2017 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk/SquareDeskPlayer application.
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

#include <QActionGroup>
#include <QColorDialog>
#include <QCoreApplication>
#include <QDesktopWidget>
#include <QElapsedTimer>
#include <QMap>
#include <QMapIterator>
#include <QProcess>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTextDocument>
#include <QThread>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QWidget>

#include "analogclock.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utility.h"
#include "tablenumberitem.h"
#include "prefsmanager.h"

#define CUSTOM_FILTER
#include "startupwizard.h"


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
// Load Playlist
// Save Playlist
// Next Song                K                            K                                K
// Prev Song
//
// MUSIC MENU
// play/pause               space                       space, Alt-M-P                    space
// rewind/stop              S, ESC, END, Cmd-.          S, ESC, END, Alt-M-S, Ctrl-.
// rewind/play (playing)    HOME, . (while playing)     HOME, .  (while playing)          .
// skip/back 5 sec          Cmd-RIGHT/LEFT,RIGHT/LEFT   Ctrl-RIGHT/LEFT, RIGHT/LEFT,
//                                                        Alt-M-A/Alt-M-B
// volume up/down           Cmd-UP/DOWN,UP/DOWN         Ctrl-UP/DOWN, UP/DOWN             N/B
// mute                     Cmd-M, M                    Ctrl-M, M
// go faster                Cmd-+,+,=                   Ctrl-+,+,=                        R
// go slower                Cmd--,-                     Ctrl--,-                          E
// force mono                                           Alt-M-F
// clear search             Cmd-/                       Alt-M-S
// pitch up                 Cmd-U, U                    Ctrl-U, U, Alt-M-U                F
// pitch down               Cmd-D, D                    Ctrl-D, D, Alt-M-D                D

// GLOBALS:
bass_audio cBass;


// ----------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    timerCountUp(NULL),
    timerCountDown(NULL),
    trapKeypresses(true),
    sd(NULL),
    firstTimeSongIsPlayed(false),
    loadingSong(false)
{
//    QSettings mySettings;
//    QString settingsPath = mySettings.fileName();
//    qDebug() << "settingsPath: " << settingsPath;

    justWentActive = false;

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

    setCueSheetAdditionalControlsVisible(false);
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

    PreferencesManager prefsManager; // Will be using application information for correct location of your settings

    musicRootPath = prefsManager.GetmusicPath();
    guestRootPath = ""; // initially, no guest music
    guestVolume = "";   // and no guest volume present
    guestMode = "main"; // and not guest mode

    switchToLyricsOnPlay = prefsManager.GetswitchToLyricsOnPlay();

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

    ui->songTable->setColumnWidth(kNumberCol,36);
    ui->songTable->setColumnWidth(kTypeCol,96);
    ui->songTable->setColumnWidth(kLabelCol,80);
//  kTitleCol is always expandable, so don't set width here
    ui->songTable->setColumnWidth(kAgeCol, 36);
    ui->songTable->setColumnWidth(kPitchCol,50);
    ui->songTable->setColumnWidth(kTempoCol,50);

    // ----------
    pitchAndTempoHidden = !prefsManager.GetexperimentalPitchTempoViewEnabled();
    updatePitchTempoView(); // update the actual view of these 2 columns in the songTable

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
//    qDebug() << "MainWindow():: lyricsTabNumber:" << lyricsTabNumber; // FIX


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
//        qDebug() << "call: " << call << ", level: " << level;

        if (level == "plus") {
            flashCalls.append(call);
        }
    }

//    qDebug() << "flashCalls:" << flashCalls;

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

    initSDtab();  // init sd, pocketSphinx, and the sd tab widgets

    QSettings settings;
    if (settings.value("cueSheetAdditionalControlsVisible").toBool()
        && !cueSheetAdditionalControlsVisible())
    {
        on_pushButtonShowHideCueSheetAdditional_clicked();
    }

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
//    qDebug() << "CurrentPlaylistFileName = " << CurrentPlaylistFileName;
    firstBadSongLine = loadPlaylistFromFile(CurrentPlaylistFileName, songCount);  // load "current.csv" (if doesn't exist, do nothing)
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

void MainWindow::setCurrentSessionIdReloadMusic(int id)
{
    setCurrentSessionId(id);
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QString origPath = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
        QFileInfo fi(origPath);
        ui->songTable->item(i,kAgeCol)->setText(songSettings.getSongAge(fi.completeBaseName(),origPath).trimmed());
        ui->songTable->item(i,kAgeCol)->setTextAlignment(Qt::AlignCenter);
    }
}

void MainWindow::on_comboBoxCuesheetSelector_currentIndexChanged(int currentIndex)
{
    QString cuesheetFilename = ui->comboBoxCuesheetSelector->itemData(currentIndex).toString();
    loadCuesheet(cuesheetFilename);
}


void MainWindow::on_actionCompact_triggered(bool checked)
{
    bool visible = !checked;
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



void MainWindow::on_actionPractice_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadMusic(1);
}

void MainWindow::on_actionMonday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadMusic(2);
}

void MainWindow::on_actionTuesday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadMusic(3);
}

void MainWindow::on_actionWednesday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadMusic(4);
}

void MainWindow::on_actionThursday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadMusic(5);
}

void MainWindow::on_actionFriday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadMusic(6);
}

void MainWindow::on_actionSaturday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadMusic(7);
}

void MainWindow::on_actionSunday_triggered(bool /* checked */)
{
    setCurrentSessionIdReloadMusic(8);
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
}

void MainWindow::setFontSizes()
{
    int preferredSmallFontSize;
    int preferredNowPlayingSize;
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
    ui->basicCallList1->setStyleSheet(styleForCallerlabDefinitions);
    ui->basicCallList2->setStyleSheet(styleForCallerlabDefinitions);
    ui->basicCallList3->setStyleSheet(styleForCallerlabDefinitions);
    ui->basicCallList4->setStyleSheet(styleForCallerlabDefinitions);
    ui->plusCallList1->setStyleSheet(styleForCallerlabDefinitions);
    ui->plusCallList2->setStyleSheet(styleForCallerlabDefinitions);
    ui->a1CallList1->setStyleSheet(styleForCallerlabDefinitions);
    ui->a1CallList2->setStyleSheet(styleForCallerlabDefinitions);
    ui->a2CallList1->setStyleSheet(styleForCallerlabDefinitions);
    ui->a2CallList2->setStyleSheet(styleForCallerlabDefinitions);

    font.setPointSize(preferredNowPlayingSize);
    ui->nowPlayingLabel->setFont(font);

    font.setPointSize((preferredSmallFontSize + preferredNowPlayingSize)/2);
    ui->warningLabel->setFont(font);
}

// ----------------------------------------------------------------------
void MainWindow::updatePitchTempoView()
{
    if (pitchAndTempoHidden) {
        ui->songTable->setColumnHidden(kAgeCol,true); // hide the age column
        ui->songTable->setColumnHidden(kPitchCol,true); // hide the pitch column
        ui->songTable->setColumnHidden(kTempoCol,true); // hide the tempo column
    }
    else {

        ui->songTable->setColumnHidden(kAgeCol,false); // show the age column
        ui->songTable->setColumnHidden(kPitchCol,false); // show the pitch column
        ui->songTable->setColumnHidden(kTempoCol,false); // show the tempo column

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
}


// ----------------------------------------------------------------------
void MainWindow::on_loopButton_toggled(bool checked)
{
    if (checked) {
        ui->actionLoop->setChecked(true);

        ui->seekBar->SetLoop(true);
        ui->seekBarCuesheet->SetLoop(true);

        double songLength = cBass.FileLength;
        cBass.SetLoop(songLength * 0.9, songLength * 0.1); // FIX: use parameters in the MP3 file

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
            int row = -1;
            if (selected.count() == 1) {
                // exactly 1 row was selected (good)
                QModelIndex index = selected.at(0);
                row = index.row();
//                ui->songTable->item(row, kAgeCol)->setText("  0");
                ui->songTable->item(row, kAgeCol)->setText("0");
                ui->songTable->item(row, kAgeCol)->setTextAlignment(Qt::AlignCenter);
            }
            if (switchToLyricsOnPlay)
            {
                for (int i = 0; i < ui->tabWidget->count(); ++i)
                {
                    if (ui->tabWidget->tabText(i).endsWith("*Lyrics"))
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
    int row = getSelectionRowForFilename(currentMP3filenameWithPath);
    if (row != -1)
    {
        ui->songTable->item(row, kPitchCol)->setText(QString::number(currentPitch)); // already trimmed()
    }
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
    cBass.SetVolume(voltageLevelToSet);  // now logarithmic, 0 --> 0.01, 50 --> 0.1, 100 --> 1.0 (values * 100 for libbass)
//    qDebug() << "in/out = " << value << voltageLevelToSet;
    currentVolume = value;  // this will be saved with the song (0-100)

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
//        float baseBPM = (float)cBass.Stream_BPM;    // original detected BPM
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
    seekBar->setMaximum((int)cBass.FileLength-1); // NOTE: tricky, counts on == below
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

        if (currentPos_i == fileLen_i) {  // NOTE: tricky, counts on -1 above
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
void MainWindow::setCueSheetAdditionalControlsVisible(bool visible)
{
    // Really want to do this:
    // ui->horizontalLayoutCueSheetAdditional->setVisible(false);

    for(int i = 0; i < ui->horizontalLayoutCueSheetAdditional->count(); i++)
    {
        QWidget *qw = qobject_cast<QWidget*>(ui->horizontalLayoutCueSheetAdditional->itemAt(i)->widget());
        if (qw)
        {
            qw->setVisible(visible);
        }
    }
}
bool MainWindow::cueSheetAdditionalControlsVisible()
{
    for(int i = 0; i < ui->horizontalLayoutCueSheetAdditional->count(); i++)
    {
        QWidget *qw = qobject_cast<QWidget*>(ui->horizontalLayoutCueSheetAdditional->itemAt(i)->widget());

        if (qw)
            return qw->isVisible();
    }
    return false;
}

// --------------------------------1--------------------------------------

void MainWindow::on_pushButtonShowHideCueSheetAdditional_clicked()
{
    bool visible = !cueSheetAdditionalControlsVisible();
    ui->pushButtonShowHideCueSheetAdditional->setText(visible ? "\u25BC" : "\u25B6");
    setCueSheetAdditionalControlsVisible(visible);
}


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
    return t;
}

QString doubleToTime(double t)
{
    double minutes = floor(t / 60);
    double seconds = t - minutes * 60;
    QString str = QString("%1:%2").arg(minutes).arg(seconds, 6, 'f', 3, '0');
    return str;
}


void MainWindow::on_lineEditOutroTime_textChanged()
{
    bool ok = false;
    double position = timeToDouble(ui->lineEditOutroTime->text(),&ok);
    if (ok)
    {
        int length = ui->seekBarCuesheet->maximum();
        double t = (double)((double)position / (double)length);
        ui->seekBarCuesheet->SetOutro((float)t);
        ui->seekBar->SetOutro((float)t);
    }
}
void MainWindow::on_lineEditIntroTime_textChanged()
{
    bool ok = false;
    double position = timeToDouble(ui->lineEditIntroTime->text(),&ok);
    if (ok)
    {
        int length = ui->seekBarCuesheet->maximum();
        double t = (double)((double)position / (double)length);
        ui->seekBarCuesheet->SetIntro((float)t);
        ui->seekBar->SetIntro((float)t);
    }
}

// --------------------------------1--------------------------------------
void MainWindow::on_pushButtonSetIntroTime_clicked()
{
    int length = ui->seekBarCuesheet->maximum();
    int position = ui->seekBarCuesheet->value();
    double t = (double)((double)position / (double)length);
    ui->seekBarCuesheet->SetIntro((float)t);
    ui->seekBar->SetIntro((float)t);
    ui->lineEditIntroTime->setText(doubleToTime(position));
}

// --------------------------------1--------------------------------------

void MainWindow::on_pushButtonSetOutroTime_clicked()
{
    int length = ui->seekBarCuesheet->maximum();
    int position = ui->seekBarCuesheet->value();

    double t = (double)((double)position / (double)length);
    ui->seekBarCuesheet->SetOutro((float)t);
    ui->seekBar->SetOutro((float)t);
    ui->lineEditOutroTime->setText(doubleToTime(position));
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
    }
#ifndef DEBUGCLOCK
    analogClock->setSegment(time.hour(), time.minute(), time.second(), theType);  // always called once per second
#else
    analogClock->setSegment(0, time.minute(), time.second(), theType);  // DEBUG DEBUG DEBUG
#endif

    // ------------------------
    // Sounds associated with Tip and Break Timers (one-shots)
    newTimerState = analogClock->currentTimerState;
//    qDebug() << "oldTimerState:" << oldTimerState << "newTimerState:" << newTimerState <<
//                "tipLengthAlarmAction" << tipLengthAlarmAction << "breakLengthAlarmAction" << breakLengthAlarmAction;

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
//        qDebug() << "no change to volume list...";
    } else {
//        qDebug() << "***** change to volume list...";
//        qDebug() << "old volume list:" << lastKnownVolumeList;
//        qDebug() << "new volume list:" << newVolumeList;
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
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowState", saveState());
        settings.setValue("cueSheetAdditionalControlsVisible", cueSheetAdditionalControlsVisible());
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
    msgBox.setText(QString("<p><h2>SquareDesk Player, V0.8.1a</h2>") +
                   QString("<p>See our website at <a href=\"http://squaredesk.net\">squaredesk.net</a></p>") +
                   QString("Uses: <a href=\"http://www.un4seen.com/bass.html\">libbass</a>, ") +
                   QString("<a href=\"http://www.jobnik.org/?mnu=bass_fx\">libbass_fx</a>, ") +
                   QString("<a href=\"http://www.lynette.org/sd\">sd</a>, and ") +
                   QString("<a href=\"http://cmusphinx.sourceforge.net\">PocketSphinx</a>.") +
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

//        qDebug() << "mainwindow::GlobalEventFilter:" << KeyEvent;
//        qDebug() << "KeyEvent->key() = " << KeyEvent->key() << "Qt::Key_Escape =" << Qt::Key_Escape;
//        if (ui->labelSearch->hasFocus()) {
//            qDebug() << "labelSearch has focus.";
//        } else if (ui->typeSearch->hasFocus()) {
//            qDebug() << "typeSearch has focus.";
//        } else if (ui->titleSearch->hasFocus()) {
//            qDebug() << "titleSearch has focus.";
//        } else if (ui->lineEditCountDownTimer->hasFocus()) {
//            qDebug() << "lineEditCountDownTimer has focus.";
//        } else if (ui->songTable->isEditing()) {
//            qDebug() << "songTable is being edited.";
//        } else if (((MainWindow *)(((QApplication *)Object)->activeWindow()))->console->hasFocus()) {
//            qDebug() << "console has focus.";
//        } else if (((MainWindow *)(((QApplication *)Object)->activeWindow()))->currentSequenceWidget->hasFocus()) {
//            qDebug() << "currentSequenceWidget has focus.";
//        } else {
//            qDebug() << "something else has focus, or there is no focus right now.";
//        }

//        QString currentWindowName = ((((QApplication *)Object)->activeWindow()))->objectName();
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
               ui->lineEditIntroTime->hasFocus() ||
               ui->lineEditOutroTime->hasFocus() ||
               ui->lineEditCountDownTimer->hasFocus() ||
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

    switch (key) {

        case Qt::Key_Escape:
            // ESC is special:  it always gets the music to stop, and gets you out of
            //   editing a search field or timer field.

            oldFocusWidget = 0;  // indicates that we want NO FOCUS on restore
            if (QApplication::focusWidget() != NULL) {
                QApplication::focusWidget()->clearFocus();  // clears the focus from ALL widgets
            }
            oldFocusWidget = 0;  // indicates that we want NO FOCUS on restore, yes both of these are needed.

            // FIX: should we also stop editing of the songTable on ESC?

            // clear the search fields, too now, so that ESC means "GET ME OUT OF HERE".
            ui->labelSearch->setText("");
            ui->typeSearch->setText("");
            ui->titleSearch->setText("");

            if (currentState == kPlaying) {
                on_playButton_clicked();  // we were playing, so PAUSE now.
            }
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

        case Qt::Key_PageDown:
            // only move the scrolled Lyrics area, if the Lyrics tab is currently showing, and lyrics are loaded
            tabTitle = ui->tabWidget->tabText(ui->tabWidget->currentIndex());
            if (tabTitle.endsWith("*Lyrics")) {
                ui->textBrowserCueSheet->verticalScrollBar()->setValue(ui->textBrowserCueSheet->verticalScrollBar()->value() + 200);
            }
            break;

        case Qt::Key_PageUp:
            // only move the scrolled Lyrics area, if the Lyrics tab is currently showing, and lyrics are loaded
            tabTitle = ui->tabWidget->tabText(ui->tabWidget->currentIndex());
            if (tabTitle.endsWith("*Lyrics")) {
                ui->textBrowserCueSheet->verticalScrollBar()->setValue(ui->textBrowserCueSheet->verticalScrollBar()->value() - 200);
            }
            break;

        case Qt::Key_L:
            on_loopButton_toggled(!ui->actionLoop->isChecked());  // toggle it
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

static const char *music_file_extensions[] = { "mp3", "wav" };
static const char *cuesheet_file_extensions[] = { "htm", "html", "txt" };


//QString capitalize(const QString &str)
//{
//    QString tmp = str;
//    // if you want to ensure all other letters are lowercase:
//    tmp = tmp.toLower();
//    tmp[0] = str[0].toUpper();
//    return tmp;
//}
struct FilenameMatchers {
    QRegularExpression regex;
    int title_match;
    int label_match;
    int number_match;
    int additional_title_match;
};

struct FilenameMatchers *getFilenameMatchersForType(enum SongFilenameMatchingType songFilenameFormat)
{
    static struct FilenameMatchers best_guess_matches[] = {
        { QRegularExpression("^(.*) - ([A-Z]+[\\- ]\\d+)( *-?[VMA-C]|\\-\\d+)?$"), 1, 2, -1, 3 },
        { QRegularExpression("^([A-Z]+[\\- ]\\d+)(-?[VvMA-C]?) - (.*)$"), 3, 1, -1, 2 },
        { QRegularExpression("^([A-Z]+ ?\\d+)([MV]?)[ -]+(.*)$/"), 3, 1, -1, 2 },
        { QRegularExpression("^([A-Z]?[0-9][A-Z]+[\\- ]?\\d+)([MV]?)[ -]+(.*)$"), 3, 1, -1, 2 },
        { QRegularExpression("^(.*) - ([A-Z]{1,5}+[\\- ]\\d+)( .*)?$"), 1, 2, -1, 3 },
        { QRegularExpression("^([A-Z]+ ?\\d+)([ab])?[ -]+(.*)$/"), 3, 1, -1, 2 },
        { QRegularExpression("^([A-Z]+\\-\\d+)\\-(.*)/"), 2, 1, -1, -1 },
//    { QRegularExpression("^(\\d+) - (.*)$"), 2, -1, -1, -1 },         // first -1 prematurely ended the search (typo?)
//    { QRegularExpression("^(\\d+\\.)(.*)$"), 2, -1, -1, -1 },         // first -1 prematurely ended the search (typo?)
        { QRegularExpression("^(\\d+)\\s*-\\s*(.*)$"), 2, 1, -1, -1 },  // e.g. "123 - Chicken Plucker"
        { QRegularExpression("^(\\d+\\.)(.*)$"), 2, 1, -1, -1 },            // e.g. "123.Chicken Plucker"
//        { QRegularExpression("^(.*?) - (.*)$"), 2, 1, -1, -1 },           // I'm not sure what the ? does here (typo?)
        { QRegularExpression("^([A-Z]{1,5}+[\\- ]*\\d+[A-Z]*)\\s*-\\s*(.*)$"), 2, 1, -1, -1 }, // e.g. "ABC 123-Chicken Plucker"
        { QRegularExpression("^([A-Z0-9]{1,5}+)\\s*-\\s*(.*)$"), 2, 1, -1, -1 },    // e.g. "POP - Chicken Plucker" (if it has a dash but fails all other tests,
                                                                    //    assume label on the left, if it's short and all caps/#s (1-5 chars long))
        { QRegularExpression(), -1, -1, -1, -1 }
    };
    static struct FilenameMatchers label_first_matches[] = {
        { QRegularExpression("^(.*)\\s*-\\s*(.*)$"), 2, 1, -1, -1 },    // e.g. "ABC123X - Chicken Plucker"
        { QRegularExpression(), -1, -1, -1, -1 }
    };
    static struct FilenameMatchers filename_first_matches[] = {
        { QRegularExpression("^(.*)\\s*-\\s*(.*)$"), 1, 2, -1, -1 },    // e.g. "Chicken Plucker - ABC123X"
        { QRegularExpression(), -1, -1, -1, -1 }
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


bool MainWindow::breakFilenameIntoParts(const QString &s, QString &label, QString &labelnum, QString &title, QString &shortTitle )
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
                if (matches[match_num].additional_title_match >= 0) {
                    title = title + " " + match.captured(matches[match_num].additional_title_match);
                }
            }
//                qDebug() << s << "*** MATCHED ***" << matches[match_num].regex;
//                qDebug() << "label:" << label << ", title:" << title;
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
        static QRegularExpression regexLabelPlusNum = QRegularExpression("^(\\w+)[\\- ](\\d+\\w?)$");
        QRegularExpressionMatch match = regexLabelPlusNum.match(label);
        if (match.hasMatch())
        {
            label = match.captured(1);
            labelnum = match.captured(2);
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
        ui->textBrowserCueSheet->setSource(cuesheetUrl);
    }

}


// TODO: the match needs to be a little fuzzier, since RR103B - Rocky Top.mp3 needs to match RR103 - Rocky Top.html
void MainWindow::findPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets)
{
    QElapsedTimer timer;
    timer.start();

    QFileInfo mp3FileInfo(MP3Filename);
    QString mp3CanonicalPath = mp3FileInfo.canonicalPath();
    QString mp3CompleteBaseName = mp3FileInfo.completeBaseName();
    QString mp3Label = "";
    QString mp3Labelnum = "";
    QString mp3Title = "";
    QString mp3ShortTitle = "";
    breakFilenameIntoParts(mp3CompleteBaseName, mp3Label, mp3Labelnum, mp3Title, mp3ShortTitle);
    QList<CuesheetWithRanking *> possibleRankings;


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

        // Is this a file esxtension we recognize as a cuesheet file?
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
        QString shortTitle = "";


        QString completeBaseName = fi.completeBaseName(); // e.g. "/Users/mpogue/__squareDanceMusic/patter/RIV 307 - Going to Ceili (Patter).mp3" --> "RIV 307 - Going to Ceili (Patter)"
        breakFilenameIntoParts(completeBaseName, label, labelnum, title, shortTitle);

//        qDebug() << "Comparing: " << completeBaseName << " to " << mp3CompleteBaseName;
//        qDebug() << "           " << title << " to " << mp3Title;
//        qDebug() << "           " << shortTitle << " to " << mp3ShortTitle;
//        qDebug() << "    label: " << label << " to " << mp3Label << " and num " << labelnum << " to " << mp3Labelnum;
//        qDebug() << "    title: " << mp3Title << " to " << QString(label + "-" + labelnum);

        // Minimum criteria:
        if (completeBaseName.compare(mp3CompleteBaseName, Qt::CaseInsensitive) == 0
            || title.compare(mp3Title, Qt::CaseInsensitive) == 0
            || (shortTitle.length() > 0
                && shortTitle.compare(mp3ShortTitle, Qt::CaseInsensitive) == 0)
//            || (labelnum.length() > 0 && label.length() > 0
//                &&  labelnum.compare(mp3Labelnum, Qt::CaseInsensitive)
//                && label.compare(mp3Label, Qt::CaseInsensitive) == 0
//                )
            || (labelnum.length() > 0 && label.length() > 0
                && mp3Title.length() > 0
                && mp3Title.compare(label + "-" + labelnum, Qt::CaseInsensitive) == 0)
            )
        {

            int score = extensionIndex
                + (mp3CanonicalPath.compare(fi.canonicalPath(), Qt::CaseInsensitive) == 0 ? 10000 : 0)
                + (mp3CompleteBaseName.compare(fi.completeBaseName(), Qt::CaseInsensitive) == 0 ? 1000 : 0)
                + (title.compare(mp3Title, Qt::CaseInsensitive) == 0 ? 100 : 0)
                + (shortTitle.compare(mp3ShortTitle, Qt::CaseInsensitive) == 0 ? 50 : 0)
                + (labelnum.compare(mp3Labelnum, Qt::CaseInsensitive) == 0 ? 10 : 0)
                + (mp3Label.compare(mp3Label, Qt::CaseInsensitive) == 0 ? 5 : 0);

            CuesheetWithRanking *cswr = new CuesheetWithRanking();
            cswr->filename = filename;
            cswr->name = completeBaseName;
            cswr->score = score;
            possibleRankings.append(cswr);
        } /* end of if we minimally included this cuesheet */
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

//    qDebug() << "time(FindPossibleCuesheets)=" << timer.elapsed() << " msec";
//    qDebug() << possibleCuesheets;
}

void MainWindow::loadCuesheets(const QString &MP3FileName)
{
    hasLyrics = false;

    QString HTML;

    QStringList possibleCuesheets;
    findPossibleCuesheets(MP3FileName, possibleCuesheets);


    QString firstCuesheet("");
    ui->comboBoxCuesheetSelector->clear();

    foreach (const QString &cuesheet, possibleCuesheets)
    {
        if (firstCuesheet.length() == 0)
        {
            firstCuesheet = cuesheet;
        }

        QString displayName = cuesheet;
        if (displayName.startsWith(musicRootPath))
            displayName.remove(0, musicRootPath.length());

        ui->comboBoxCuesheetSelector->addItem(displayName,
                                              cuesheet);
    }

    if (firstCuesheet.length() > 0)
    {
        loadCuesheet(firstCuesheet);
        hasLyrics = true;
    }

    if (hasLyrics && lyricsTabNumber != -1) {
        ui->tabWidget->setTabText(lyricsTabNumber, "*Lyrics");
    } else {
        ui->textBrowserCueSheet->setHtml("No lyrics found for this song.");
        ui->tabWidget->setTabText(lyricsTabNumber, "Lyrics");
    }
}


float MainWindow::getID3BPM(QString MP3FileName) {
    MPEG::File *mp3file;
    ID3v2::Tag *id3v2tag;  // NULL if it doesn't have a tag, otherwise the address of the tag

    mp3file = new MPEG::File(MP3FileName.toStdString().c_str()); // FIX: this leaks on read of another file
    id3v2tag = mp3file->ID3v2Tag(true);  // if it doesn't have one, create one

//    qDebug() << "mp3file: " << mp3file << ", id3: " << id3v2tag;

    float theBPM = 0.0;

    ID3v2::FrameList::ConstIterator it = id3v2tag->frameList().begin();
    for (; it != id3v2tag->frameList().end(); it++)
    {
//        cout << (*it)->frameID() << " - \"" << (*it)->toString() << "\"" << endl;
//        cout << (*it)->frameID() << endl;

        if ((*it)->frameID() == "TBPM")  // This is an Apple standard, which means it's everybody's standard now.
        {
//            qDebug() << "getID3BPM -- found a TBPM frame (Apple's embedded ID3 BPM string)!";
            QString BPM((*it)->toString().toCString());
//            qDebug() << "    BPM = " << BPM;
            theBPM = BPM.toFloat();
        }

    }

    return(theBPM);
}


void MainWindow::loadMP3File(QString MP3FileName, QString songTitle, QString songType)
{
    RecursionGuard recursion_guard(loadingSong);
    firstTimeSongIsPlayed = true;

    // resolve aliases at load time, rather than findFilesRecursively time, because it's MUCH faster
    QFileInfo fi(MP3FileName);
    QString resolvedFilePath = fi.symLinkTarget(); // path with the symbolic links followed/removed
    if (resolvedFilePath != "") {
        MP3FileName = resolvedFilePath;
    }

//    qDebug() << "loadMP3File: MP3FileName=" << MP3FileName << ", resolved=" << resolvedFilePath;

    loadCuesheets(MP3FileName);

    currentMP3filenameWithPath = MP3FileName;

    currentSongType = songType;  // save it for session coloring on the analog clock later...

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
//        qDebug() << "embedded ID3/TBPM frame of " << (int)songBPM_ID3 << "overrides libbass BPM estimate of " << songBPM;
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
//        ui->tempoSlider->setValue(songBPM);
//        ui->tempoSlider->valueChanged(songBPM);  // fixes bug where second song with same BPM doesn't update songtable::tempo

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

    if (songType == "patter") {
        on_loopButton_toggled(true); // default is to loop, if type is patter
    }
    else {
        // not patter, so Loop mode defaults to OFF
        on_loopButton_toggled(false); // default is to loop, if type is patter
    }

    bool isSingingCall = songTypeNamesForSinging.contains(songType) ||
                         songTypeNamesForCalled.contains(songType);
    ui->seekBar->SetSingingCall(isSingingCall); // if singing call, color the seek bar
    ui->seekBarCuesheet->SetSingingCall(isSingingCall); // if singing call, color the seek bar

    ui->pushButtonSetIntroTime->setEnabled(isSingingCall);  // if not singing call, buttons will be greyed out on Lyrics tab
    ui->pushButtonSetOutroTime->setEnabled(isSingingCall);

    ui->lineEditIntroTime->setText("");
    ui->lineEditOutroTime->setText("");
    ui->lineEditIntroTime->setEnabled(isSingingCall);
    ui->lineEditOutroTime->setEnabled(isSingingCall);


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
void findFilesRecursively(QDir rootDir, QList<QString> *pathStack, QString suffix)
{
//    QElapsedTimer timer1;
//    timer1.start();

//    qDebug() << "FFR: rootDir =" << rootDir;
    QDirIterator it(rootDir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while(it.hasNext()) {
        QString s1 = it.next();
//        qDebug() << "FFR:" << s1;
        // If alias, follow it.
//        QString resolvedFilePath = it.fileInfo().symLinkTarget(); // path with the symbolic links followed/removed
//        if (resolvedFilePath == "") {
//            // If NOT alias, then use the original fileName
//            resolvedFilePath = s1;
//        }
        QString resolvedFilePath=s1;

        QFileInfo fi(s1);
        QStringList section = fi.canonicalPath().split("/");
        QString type = section[section.length()-1] + suffix;  // must be the last item in the path
                                                              // of where the alias is, not where the file is, and append "*" or not
        if (section[section.length()-1] != "soundfx") {
            // add to the pathStack iff it's not a sound FX .mp3 file (those are internal)
            pathStack->append(type + "#!#" + resolvedFilePath);
        }
    }
//    qDebug() << "timer1:" << timer1.elapsed();
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
//        qDebug() << "looking for files in the mainRootDir";
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

        findFilesRecursively(rootDir1, pathStack, "");  // appends to the pathstack
    }

    if (guestRootDir != "" && (mode == "guest" || mode == "both")) {
        // looks for files in the guestRootDir --------
//        qDebug() << "looking for files in the guestRootDir";
        QDir rootDir2(guestRootDir);
        rootDir2.setFilter(QDir::Files | QDir::Dirs | QDir::NoDot | QDir::NoDotDot);

        QStringList qsl;
        qsl.append("*.mp3");                // I only want MP3 files
        qsl.append("*.wav");                //          or WAV files
        rootDir2.setNameFilters(qsl);

        findFilesRecursively(rootDir2, pathStack, "*");  // appends to the pathstack, "*" for "Guest"
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
#else /* ifdef CUSTOM_FILTER */
    loadMusicList();
#endif /* else ifdef CUSTOM_FILTER */
}

// --------------------------------------------------------------------------------
void MainWindow::loadMusicList()
{
    ui->songTable->setSortingEnabled(false);

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
        QString title = "";
        QString shortTitle = "";

        s = fi.completeBaseName(); // e.g. "/Users/mpogue/__squareDanceMusic/patter/RIV 307 - Going to Ceili (Patter).mp3" --> "RIV 307 - Going to Ceili (Patter)"
        breakFilenameIntoParts(s, label, labelnum, title, shortTitle);

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
        addStringToLastRowOfSongTable(textCol, ui->songTable, songSettings.getSongAge(fi.completeBaseName(), origPath), kAgeCol);

        int pitch = 0;
        int tempo = 0;
        int volume = 0;
        double intro = 0;
        double outro = 0;
        bool loadedTempoIsPercent;
        songSettings.loadSettings(fi.completeBaseName(),
                                  origPath,
                                  title,
                                  volume,
                                  pitch, tempo,
                                  loadedTempoIsPercent,
                                  intro, outro);

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
    ui->songTable->setSortingEnabled(true);

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

    if (tempo != "0") {
        // iff tempo is known, then update the table
        QString tempo2 = tempo.replace("%",""); // if percentage (not BPM) just get rid of the "%" (setValue knows what to do)
        int tempoInt = tempo2.toInt();
        ui->tempoSlider->setValue(tempoInt);
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


// --------------------------------------------------------
void MainWindow::on_actionPreferences_triggered()
{
    inPreferencesDialog = true;
    trapKeypresses = false;
//    on_stopButton_clicked();  // stop music, if it was playing...
    PreferencesManager prefsManager;

    prefDialog = new PreferencesDialog;
    prefsManager.populatePreferencesDialog(prefDialog);

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
//        if (prefsManager.GetexperimentalCuesheetEnabled()) {
//            lyricsTabNumber = (showTimersTab ? 2 : 1);
//            if (!showLyricsTab) {
//                // iff the Lyrics tab was NOT showing, make it show up now
//                ui->tabWidget->insertTab((showTimersTab ? 2 : 1), tabmap.value(2).first, tabmap.value(2).second);  // bring it back now!
//            }
//            showLyricsTab = true;
//        }
//        else {
//            lyricsTabNumber = -1;  // not shown
//            if (showLyricsTab) {
//                // iff Lyrics tab was showing, remove it
//                ui->tabWidget->removeTab((showTimersTab ? 2 : 1));  // hidden, but we can bring it back later
//            }
//            showLyricsTab = false;
//        }

//        qDebug() << "After Preferences:: lyricsTabNumber:" << lyricsTabNumber; // FIX
        if (hasLyrics && lyricsTabNumber != -1) {
            ui->tabWidget->setTabText(lyricsTabNumber, "*Lyrics");
        } else {
            ui->tabWidget->setTabText(lyricsTabNumber, "Lyrics");
        }

        // ----------------------------------------------------------------
        // Save the new value for experimentalPitchTempoViewEnabled --------
        pitchAndTempoHidden = !prefsManager.GetexperimentalPitchTempoViewEnabled();
        updatePitchTempoView();  // update the columns in songTable, as per the user's NEW setting

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

        loadMusicList();
    }

    if (prefsManager.GetenableAutoAirplaneMode()) {
        // if the user JUST set the preference, turn Airplane Mode on RIGHT NOW (radios OFF).
        airplaneMode(true);
    } else {
        // if the user JUST set the preference, turn Airplane Mode OFF RIGHT NOW (radios ON).
        airplaneMode(false);
    }

    delete prefDialog;
    prefDialog = NULL;
    inPreferencesDialog = false;  // FIX: might not need this anymore...
}

QString MainWindow::removePrefix(QString prefix, QString s)
{
    QString s2 = s.remove( prefix );
    return s2;
}

// returns first song error, and also updates the songCount as it goes (2 return values)
QString MainWindow::loadPlaylistFromFile(QString PlaylistFileName, int &songCount) {
    // --------
    QString firstBadSongLine = "";
//    int songCount = 0;
    QFile inputFile(PlaylistFileName);
    if (inputFile.open(QIODevice::ReadOnly)) { // defaults to Text mode
//        ui->songTable->setSortingEnabled(false);  // sorting must be disabled to clear

        // first, clear all the playlist numbers that are there now.
        for (int i = 0; i < ui->songTable->rowCount(); i++) {
            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
            theItem->setText("");

//            QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);  // clear out the hidden pitches, too
//            theItem2->setText("0");

//            QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);  // clear out the hidden tempos, too
//            theItem3->setText("0");
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
                    QStringList list1 = line.split(",");

                    list1[0].replace("\"","");  // get rid of all double quotes in the abspath
                    list1[1].replace("\"",
                                     "");  // get rid of all double quotes in the pitch (they should not be there at all, this is an INT)

                    bool match = false;
                    // exit the loop early, if we find a match
                    for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {
                        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
                        if (list1[0] == pathToMP3) { // FIX: this is fragile, if songs are moved around
                            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
                            theItem->setText(QString::number(songCount));

                            QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);
                            theItem2->setText(list1[1].trimmed());

                            QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);
                            theItem3->setText(list1[2].trimmed());

                            match = true;
//                            QTableWidgetItem *theItemAge = ui->songTable->item(i,kAgeCol);
//                            theItemAge->setText("***");
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

//                            QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);
//                            theItem2->setText("0");  // M3U doesn't have pitch yet

//                            QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);
//                            theItem3->setText("0");  // M3U doesn't have tempo yet

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
void MainWindow::on_actionLoad_Playlist_triggered()
{
    on_stopButton_clicked();  // if we're loading a new PLAYLIST file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");
    PreferencesManager prefsManager;
    QString musicRootPath = prefsManager.GetmusicPath();
    QString startingPlaylistDirectory = prefsManager.Getdefault_playlist_dir();
//    qDebug() << "startingPlaylistDirectory =" << startingPlaylistDirectory;
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
//    qDebug() << "Setting default playlist dir to: " << fInfo.absolutePath();

    // --------
    QString firstBadSongLine = "";
    int songCount = 0;
    ui->songTable->setSortingEnabled(false);  // sorting must be disabled to clear
    firstBadSongLine = loadPlaylistFromFile(PlaylistFileName, songCount);

//    // --------
//    QString firstBadSongLine = "";
//    int songCount = 0;
//    QFile inputFile(PlaylistFileName);
//    if (inputFile.open(QIODevice::ReadOnly)) { // defaults to Text mode
//        ui->songTable->setSortingEnabled(false);  // sorting must be disabled to clear

//        // first, clear all the playlist numbers that are there now.
//        for (int i = 0; i < ui->songTable->rowCount(); i++) {
//            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//            theItem->setText("");

//            QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);  // clear out the hidden pitches, too
//            theItem2->setText("0");

//            QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);  // clear out the hidden tempos, too
//            theItem3->setText("0");
//        }

//        QTextStream in(&inputFile);

//        if (PlaylistFileName.endsWith(".csv")) {
//            // CSV FILE =================================
//            int lineCount = 1;

//            QString header = in.readLine();  // read header (and throw away for now), should be "abspath,pitch,tempo"

//            while (!in.atEnd()) {
//                QString line = in.readLine();
//                if (line == "abspath") {
//                    // V1 of the CSV file format has exactly one field, an absolute pathname in quotes
//                }
//                else if (line == "") {
//                    // ignore, it's a blank line
//                }
//                else {
//                    songCount++;  // it's a real song path
//                    QStringList list1 = line.split(",");

//                    list1[0].replace("\"","");  // get rid of all double quotes in the abspath
//                    list1[1].replace("\"",
//                                     "");  // get rid of all double quotes in the pitch (they should not be there at all, this is an INT)

//                    bool match = false;
//                    // exit the loop early, if we find a match
//                    for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {
//                        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
//                        if (list1[0] == pathToMP3) { // FIX: this is fragile, if songs are moved around
//                            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//                            theItem->setText(QString::number(songCount));

//                            QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);
//                            theItem2->setText(list1[1].trimmed());

//                            QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);
//                            theItem3->setText(list1[2].trimmed());

//                            match = true;
////                            QTableWidgetItem *theItemAge = ui->songTable->item(i,kAgeCol);
////                            theItemAge->setText("***");
//                        }
//                    }
//                    // if we had no match, remember the first non-matching song path
//                    if (!match && firstBadSongLine == "") {
//                        firstBadSongLine = line;
//                    }

//                }

//                lineCount++;
//            } // while
//        }
//        else {
//            // M3U FILE =================================
//            int lineCount = 1;

//            while (!in.atEnd()) {
//                QString line = in.readLine();

//                if (line == "#EXTM3U") {
//                    // ignore, it's the first line of the M3U file
//                }
//                else if (line == "") {
//                    // ignore, it's a blank line
//                }
//                else if (line.at( 0 ) == '#' ) {
//                    // it's a comment line
//                    if (line.mid(0,7) == "#EXTINF") {
//                        // it's information about the next line, ignore for now.
//                    }
//                }
//                else {
//                    songCount++;  // it's a real song path

//                    bool match = false;
//                    // exit the loop early, if we find a match
//                    for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {
//                        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
//                        if (line == pathToMP3) { // FIX: this is fragile, if songs are moved around
//                            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//                            theItem->setText(QString::number(songCount));

//                            QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);
//                            theItem2->setText("0");  // M3U doesn't have pitch yet

//                            QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);
//                            theItem3->setText("0");  // M3U doesn't have tempo yet

//                            match = true;
//                        }
//                    }
//                    // if we had no match, remember the first non-matching song path
//                    if (!match && firstBadSongLine == "") {
//                        firstBadSongLine = line;
//                    }

//                }

//                lineCount++;
//            }
//        }

//        inputFile.close();

//    }
//    else {
//        // file didn't open...
//        return;
//    }
    sortByDefaultSortOrder();
    ui->songTable->sortItems(kNumberCol);  // sort by playlist # as primary (must be LAST)
    ui->songTable->setSortingEnabled(true);  // sorting must be disabled to clear

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

// SAVE CURRENT PLAYLIST TO FILE
void MainWindow::saveCurrentPlaylistToFile(QString PlaylistFileName) {
    // --------
    QMap<int, QString> imports, importsPitch, importsTempo;

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
            imports[playlistIndex.toInt()] = pathToMP3;
            importsPitch[playlistIndex.toInt()] = pitch;
            importsTempo[playlistIndex.toInt()] = tempo;
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
    if (PlaylistFileName.endsWith(".m3u")) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {  // delete, if it exists already
            QTextStream stream(&file);
            stream << "#EXTM3U" << endl << endl;

            // list is auto-sorted here
            QMapIterator<int, QString> i(imports);
            while (i.hasNext()) {
                i.next();
                stream << "#EXTINF:-1," << endl;  // nothing after the comma = no special name
                stream << i.value() << endl;
            }
            file.close();
        }
        else {
            ui->statusBar->showMessage(QString("ERROR: could not open M3U file."));
        }
    }
    else if (PlaylistFileName.endsWith(".csv")) {
        if (file.open(QIODevice::ReadWrite)) {
            QTextStream stream(&file);
            stream << "abspath,pitch,tempo" << endl;

            // list is auto-sorted here
            QMapIterator<int, QString> i(imports);
            while (i.hasNext()) {
                i.next();
                stream << "\"" << i.value() << "\"," <<
                       importsPitch[i.key()] << "," <<
                       importsTempo[i.key()] << endl; // quoted absolute path, integer pitch (no quotes), integer tempo (opt % or 0)
            }
            file.close();
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
//    QDir CurrentDir;
//    MySettings.setValue(DEFAULT_PLAYLIST_DIR_KEY, CurrentDir.absoluteFilePath(PlaylistFileName));
    QFileInfo fInfo(PlaylistFileName);
    PreferencesManager prefsManager;
    prefsManager.Setdefault_playlist_dir(fInfo.absolutePath());

    saveCurrentPlaylistToFile(PlaylistFileName);  // SAVE IT

//    // --------
//    QMap<int, QString> imports, importsPitch, importsTempo;

//    // Iterate over the songTable
//    for (int i=0; i<ui->songTable->rowCount(); i++) {
//        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//        QString playlistIndex = theItem->text();
//        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
//        QString songTitle = ui->songTable->item(i,kTitleCol)->text();
//        QString pitch = ui->songTable->item(i,kPitchCol)->text();
//        QString tempo = ui->songTable->item(i,kTempoCol)->text();

//        if (playlistIndex != "") {
//            // item HAS an index (that is, it is on the list, and has a place in the ordering)
//            // TODO: reconcile int here with float elsewhere on insertion
//            imports[playlistIndex.toInt()] = pathToMP3;
//            importsPitch[playlistIndex.toInt()] = pitch;
//            importsTempo[playlistIndex.toInt()] = tempo;
//        }
//    }

//    // TODO: strip the initial part of the path off the Paths, e.g.
//    //   /Users/mpogue/__squareDanceMusic/patter/C 117 - Restless Romp (Patter).mp3
//    //   becomes
//    //   patter/C 117 - Restless Romp (Patter).mp3
//    //
//    //   So, the remaining path is relative to the root music directory.
//    //   When loading, first look at the patter and the rest
//    //     if no match, try looking at the rest only
//    //     if no match, then error (dialog?)
//    //   Then on Save Playlist, write out the NEW patter and the rest

//    // TODO: get rid of the single space, replace with nothing

//    QFile file(PlaylistFileName);
//    if (PlaylistFileName.endsWith(".m3u")) {
//        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {  // delete, if it exists already
//            QTextStream stream(&file);
//            stream << "#EXTM3U" << endl << endl;

//            // list is auto-sorted here
//            QMapIterator<int, QString> i(imports);
//            while (i.hasNext()) {
//                i.next();
//                stream << "#EXTINF:-1," << endl;  // nothing after the comma = no special name
//                stream << i.value() << endl;
//            }
//            file.close();
//        }
//        else {
//            ui->statusBar->showMessage(QString("ERROR: could not open M3U file."));
//        }
//    }
//    else if (PlaylistFileName.endsWith(".csv")) {
//        if (file.open(QIODevice::ReadWrite)) {
//            QTextStream stream(&file);
//            stream << "abspath,pitch,tempo" << endl;

//            // list is auto-sorted here
//            QMapIterator<int, QString> i(imports);
//            while (i.hasNext()) {
//                i.next();
//                stream << "\"" << i.value() << "\"," <<
//                       importsPitch[i.key()] << "," <<
//                       importsTempo[i.key()] << endl; // quoted absolute path, integer pitch (no quotes), integer tempo (opt % or 0)
//            }
//            file.close();
//        }
//        else {
//            ui->statusBar->showMessage(QString("ERROR: could not open CSV file."));
//        }
//    }

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
//            qDebug() << "not in a playlist";
            // if not in a Playlist then we can add it at Top or Bottom, that's it.
            ui->actionAt_TOP->setEnabled(true);
            ui->actionAt_BOTTOM->setEnabled(true);
        } else {
//            qDebug() << "in a playlist";
            ui->actionRemove_from_Playlist->setEnabled(true);  // can remove it

            // current item is on the Playlist already
            if (playlistItemCount > 1) {
//                qDebug() << "more than one item on the playlist";
                // more than one item on the list
                if (currentNumberInt == 1) {
//                   qDebug() << "first item on the playlist";
//                     it's the first item, and there's more than one item on the list, so moves make sense
                    ui->actionAt_BOTTOM->setEnabled(true);
                    ui->actionDOWN_in_Playlist->setEnabled(true);
                } else if (currentNumberInt == playlistItemCount) {
//                    qDebug() << "last item on the playlist";
                    // it's the last item, and there's more than one item on the list, so moves make sense
                    ui->actionAt_TOP->setEnabled(true);
                    ui->actionUP_in_Playlist->setEnabled(true);
                } else {
//                    qDebug() << "middle item on the playlist";
                    // it's somewhere in the middle, and there's more than one item on the list, so moves make sense
                    ui->actionAt_TOP->setEnabled(true);
                    ui->actionAt_BOTTOM->setEnabled(true);
                    ui->actionUP_in_Playlist->setEnabled(true);
                    ui->actionDOWN_in_Playlist->setEnabled(true);
                }
            } else {
//                qDebug() << "one item on the playlist, and this is it.";
                // One item on the playlist, and this is it.
                // Can't move up/down or to top/bottom.
                // Can remove it, though.
            }
        }
    }
}

void MainWindow::on_actionClear_Playlist_triggered()
{
    // Iterate over the songTable
    ui->songTable->setSortingEnabled(false);  // must turn sorting off, or else sorting on # will not clear all
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        theItem->setText(""); // clear out the current list

        // let's intentionally NOT clear the pitches.  They are persistent within a session.
        // let's intentionally NOT clear the tempos.  They are persistent within a session.
    }

    sortByDefaultSortOrder();
    ui->songTable->setSortingEnabled(true);  // reenable sorting

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
//    qDebug() << "items in the playlist:" << playlistItemCount;
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

//    qDebug() << "PlaylistItemToTop(): " << selectedRow << currentNumberText;

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
//                qDebug() << "old, new:" << playlistIndex << newIndex;
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

//    qDebug() << "PlaylistItemToBottom(): " << selectedRow << currentNumberText;
    int playlistItemCount = PlaylistItemCount();  // how many items in the playlist right now?

    if (currentNumberText == "") {
        // add to list, and make it the bottom

        // Iterate over the entire songTable, not touching every item
        // TODO: turn off sorting

//        for (int i=0; i<ui->songTable->rowCount(); i++) {
//            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//            QString playlistIndex = theItem->text();  // this is the playlist #
//            if (playlistIndex != "") {
//                playlistItemCount++;
//                // if a # was set, increment it
////                QString newIndex = QString::number(playlistIndex.toInt()+1);
////                qDebug() << "old, new:" << playlistIndex << newIndex;
////                ui->songTable->item(i,kNumberCol)->setText(newIndex);
//            }
//        }

//        ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(playlistItemCount+1));  // this one is the new #LAST
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
//                    qDebug() << "old, new:" << playlistIndexText << newIndex;
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

//    qDebug() << "PlaylistMoveUp(): " << selectedRow << currentNumberText;

    // Iterate over the entire songTable, find the item just above this one, and move IT down (only)
    // TODO: turn off sorting

    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();  // this is the playlist #
        if (playlistIndex != "") {
            int playlistIndexInt = playlistIndex.toInt();
            if (playlistIndexInt == currentNumberInt - 1) {
                QString newIndex = QString::number(playlistIndex.toInt()+1);
//                qDebug() << "old, new:" << playlistIndex << newIndex;
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

//    qDebug() << "PlaylistMoveUDown(): " << selectedRow << currentNumberText;

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
//                qDebug() << "old, new:" << playlistIndex << newIndex;
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

//    qDebug() << "PlaylistItemRemove(): " << selectedRow << currentNumberText;

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
//                qDebug() << "old, new:" << playlistIndexText << newIndex;
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
//            ui->songTable->item(row, kPitchCol)->setText(QString::number(currentPitch));
            QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
            int currentNumberInt = currentNumberText.toInt();
            int playlistItemCount = PlaylistItemCount();
//            qDebug() << "currentNumberText: " << currentNumberText << "100: " << QString::number(100);

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
            x1 = ui->songTable->columnWidth(0) + 35;
            y1 = ui->typeSearch->y();
            w1 = newSize - 4;
            h1 = ui->typeSearch->height();
            ui->typeSearch->setGeometry(x1,y1,w1,h1);
            ui->typeSearch->setFixedWidth(w1);

            x2 = x1 + w1 + 6;
            y2 = ui->labelSearch->y();
//        w2 = ui->labelSearch->width();
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

        songSettings.saveSettings(currentMP3filename,
                                  currentMP3filenameWithPath,
                                  currentSong,
                                  currentVolume,
                                  pitch, tempo,
                                  !tempoIsBPM,
                                  ui->seekBarCuesheet->GetIntro(),
                                  ui->seekBarCuesheet->GetOutro(),
                                  cuesheetFilename
            );
        // TODO: Loop points!
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
    bool loadedTempoIsPercent;

    if (songSettings.loadSettings(currentMP3filename,
                                  currentMP3filenameWithPath,
                                  songTitle,
                                  volume,
                                  pitch, tempo,
                                  loadedTempoIsPercent,
                                  intro, outro, cuesheetName))
    {
        ui->pitchSlider->setValue(pitch);
        ui->tempoSlider->setValue(tempo);
        ui->volumeSlider->setValue(volume);
        ui->seekBarCuesheet->SetIntro(intro);
        ui->seekBarCuesheet->SetOutro(outro);

        double length = (double)(ui->seekBarCuesheet->maximum());
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
    }
}

// ------------------------------------------------------------------------------------------
QString MainWindow::loadLyrics(QString MP3FileName)
{
//    qDebug() << "Attempting to load lyrics for: " << MP3FileName;

    QString USLTlyrics;

    MPEG::File *mp3file;
    ID3v2::Tag *id3v2tag;  // NULL if it doesn't have a tag, otherwise the address of the tag

    mp3file = new MPEG::File(MP3FileName.toStdString().c_str()); // FIX: this leaks on read of another file
    id3v2tag = mp3file->ID3v2Tag(true);  // if it doesn't have one, create one

//    qDebug() << "mp3file: " << mp3file << ", id3: " << id3v2tag;

    ID3v2::FrameList::ConstIterator it = id3v2tag->frameList().begin();
    for (; it != id3v2tag->frameList().end(); it++)
    {
//        cout << (*it)->frameID() << " - \"" << (*it)->toString() << "\"" << endl;
//        cout << (*it)->frameID() << endl;

        if ((*it)->frameID() == "SYLT")
        {
//            qDebug() << "LOAD LYRICS -- found an SYLT frame!";
        }

        if ((*it)->frameID() == "USLT")
        {
//            qDebug() << "LOAD LYRICS -- found a USLT frame!";

            ID3v2::UnsynchronizedLyricsFrame* usltFrame = (ID3v2::UnsynchronizedLyricsFrame*)(*it);
            USLTlyrics = usltFrame->text().toCString();

//            QString lyrics0 = dialog->lyrics.replace(QRegExp("^<br>\\[header\\]"),"[header]");
//            QString lyrics1 = "<HTML><BODY style=\"font-family:arial;color:#000000;font-size:200%\">" + theUSLTLyrics;
//            QString lyrics2 = lyrics1.replace("[header]","<FONT size=\"+3\" face=\"Arial\">");
//            QString lyrics3 = lyrics2.replace("[lyrics]","<FONT color=\"#F51B67\"> <span>");
//            QString lyrics4 = lyrics3.replace("[¤]","'"); // .replace("<br><br>","<br>");
//            qDebug() << "PROCESSED RESULT = '" << lyrics3.toStdString() << "'" << endl;
        }

    }

//    qDebug() << "END OF FRAME LIST ***************";
//    qDebug() << "Lyrics found: " << USLTlyrics;

    return (USLTlyrics);
}

// ------------------------------------------------------------------------
QString MainWindow::txtToHTMLlyrics(QString text, QString filePathname) {
    QStringList pieces = filePathname.split( "/" );
    pieces.pop_back(); // get rid of actual filename, keep the path
    QString filedir = pieces.join("/"); // FIX: MAC SPECIFIC?
//    qDebug() << "filePathname:" << filePathname << ", filedir" << filedir;

    // TODO: we could get fancy, and keep looking in parent directories until we
    //  find a CSS file, or until we hit the musicRootPath.  That allows for an overall
    //  default, with overrides for individual subdirs.

    QString css("");
    bool fileIsOpen = false;
    QFile f1(filedir + "/cuesheet2.css");  // This is the SqView convention for a CSS file
    if ( f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // if there's a "cuesheet2.csv" file in the same directory as the .txt file,
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

//        qDebug() << "incoming:" << text << ", outgoing:" << HTML;

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
//                qDebug() << "name:" << storageVolume.name() << ", device:" << storageVolume.device();
//                qDebug() << "fileSystemType:" << storageVolume.fileSystemType();
//                qDebug() << "size:" << storageVolume.bytesTotal()/1000/1000 << "MB";
//                qDebug() << "availableSize:" << storageVolume.bytesAvailable()/1000/1000 << "MB";
            volumeDirList.append(storageVolume.name());}
    }
#endif

#if defined(Q_OS_WIN32)
    foreach (const QFileInfo &fileinfo, QDir::drives()) {
//        qDebug() << fileinfo.absoluteFilePath();
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
//            qDebug() << "MERGING IN NEW VOLUME:" << item;
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
//            qDebug() << "d" << d << "section.length():" << section.length() << "section" << section;
            if (section.length() >= 1) {
                QString dirName = section[section.length()-1];
//                qDebug() << "dirName:" << dirName;

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
//            qDebug() << "REMOVING GUEST VOLUME:" << item;
            goneVolume = item;
        }

        ui->statusBar->showMessage("REMOVING GUEST VOLUME: " + goneVolume);
        QApplication::beep();  // beep on MAC OS X and Win32
        QThread::sleep(1);  // FIX: not sure this is needed.

        guestMode = "main";
        guestRootPath = "";
        findMusic(musicRootPath, "", guestMode, false);  // get the filenames from the user's directories
    } else {
        qDebug() << "No volume added/lost by the time we got here. I give up. :-(";
        return;
    }

    lastKnownVolumeList = newVolumeList;

    loadMusicList(); // and filter them into the songTable
}

void MainWindow::on_warningLabel_clicked() {
    // TODO: clear the timer
    analogClock->resetPatter();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (ui->tabWidget->tabText(index) == "SD") {
        if (console != 0) {
            console->setFocus();
        }
    }
    microphoneStatusUpdate();
}

void MainWindow::microphoneStatusUpdate() {
    int index = ui->tabWidget->currentIndex();

    if (ui->tabWidget->tabText(index) == "SD") {
        if (voiceInputEnabled && currentApplicationState == Qt::ApplicationActive) {
            ui->statusBar->setStyleSheet("color: red");
            ui->statusBar->showMessage("Microphone enabled for voice input (Level: PLUS)");
        } else {
            ui->statusBar->setStyleSheet("color: black");
            ui->statusBar->showMessage("Microphone disabled (Level: PLUS)");
        }
    } else {
        if (voiceInputEnabled && currentApplicationState == Qt::ApplicationActive) {
            ui->statusBar->setStyleSheet("color: black");
            ui->statusBar->showMessage("Microphone will be enabled for voice input in SD tab");
        } else {
            ui->statusBar->setStyleSheet("color: black");
            ui->statusBar->showMessage("Microphone disabled");
        }
    }
}

void MainWindow::writeSDData(const QByteArray &data)
{
    if (data != "") {
        // console has data, send to sd
        QString d = data;
        d.replace("\r","\n");
//        qDebug() << "writeSDData() to SD:" << data << d.toUtf8();
        if (d.at(d.length()-1) == '\n') {
//            qDebug() << "writeSDData1:" << d.toUtf8();
            sd->write(d.toUtf8());
//            sd->write(d.toUtf8() + "\x15refresh display\n"); // assumes no errors (doesn't work if errors)
            sd->waitForBytesWritten();
        } else {
//            qDebug() << "writeSDData2:" << d.toUtf8();
            sd->write(d.toUtf8());
            sd->waitForBytesWritten();
        }
    }
}

void MainWindow::readSDData()
{
    // sd has data, send to console
    QByteArray s = sd->readAll();
//    qDebug() << "readData() from sd:" << s;

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
//    qDebug() << "unedited data:" << uneditedData;
    QString errorLine;
    QString resolveLine;
//    QString copyrightText;
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
//            editedData += line;  // no NL
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
//            qDebug() << "lastPrompt:" << lastPrompt;
        } else {
            editedData += line;  // no layout lines make it into here
            editedData += "\n";  // no layout lines make it into here
        }
    }

//    qDebug() << "RESOLVE:" << resolveLine;

    editedData += lastPrompt.replace("\a","");  // keep only the last prompt (no NL)
//    qDebug() << "editedData:" << editedData;
//    qDebug() << "copyrightText:" << copyrightText;

    // echo is needed for entering the level and entering comments, but NOT wanted after that
    if (lastPrompt.contains("Enter startup command>") || lastPrompt.contains("Enter comment:")) {
        console->setLocalEchoEnabled(true);
    } else {
        console->setLocalEchoEnabled(false);
    }

//    qDebug() << "currentSequence:" << currentSequence;
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

    //editedData.chop(1);  // no last NL
//    qDebug() << "edited data:" << editedData;

    console->clear();
    console->putData(QByteArray(editedData.toLatin1()));

    // look at unedited last line to see if there's a prompt
    if (lines[lines.length()-1].contains("-->")) {
//        qDebug() << "Found prompt:" << lines[lines.length()-1];
        QString formation;
        QRegExp r1( "[(](.*)[)]-->" );
        int pos = r1.indexIn( lines[lines.length()-1] );
        if ( pos > -1 ) {
            formation = r1.cap( 1 ); // "waves"
        }
        renderArea->setFormation(formation);
    }

//    qDebug() << "lastLayout1:" << lastLayout1;
//    qDebug() << "lastFormatList:" << lastFormatList;

//    qDebug() << "lastPrompt:" << lastPrompt;
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

    if (!voiceInputEnabled || currentApplicationState != Qt::ApplicationActive) {
        // if we're not on the sd tab OR the app is not Active, then voiceInput is disabled,
        //  and we're going to read the data from PS and just throw it away.
        return;
    }

//    qDebug() << "data from PS:" << s;

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
    s2 = s2.replace("four quarters","4/4");
    s2 = s2.replace("three quarters","3/4").replace("three quarter","3/4");  // always replace the longer thing first!
    s2 = s2.replace("two quarters","2/4").replace("one half","1/2").replace("half","1/2");
    s2 = s2.replace("one quarter", "1/4").replace("a quarter", "1/4").replace("quarter","1/4");

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
//            qDebug() << "path1" << s2;
            s2 = "[explode and [" + explodeAndRollCall.cap(1).trimmed() + "]] and roll\n";
        } else if (s2.indexOf(explodeAndNotRollCall) != -1) {
//            qDebug() << "path2" << s2;
            // not a roll, for sure.  Must be a naked "explode and <anything>\n"
            s2 = "explode and [" + explodeAndNotRollCall.cap(1).trimmed() + "]\n";
        } else {
//            qDebug() << "not explode.";
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
//    QString who = QString("(^[heads|sides|centers|ends|outsides|insides|couples|everybody") +
////                  QString("center boys|end boys|outside boys|inside boys|all four boys|") +
////                  QString("center girls|end girls|outside girls|inside girls|all four girls|") +
////                  QString("center men|end men|outside men|inside men|all four men|") +
////                  QString("center ladies|end ladies|outside ladies|inside ladies|all four ladies") +
//                  QString("]*)");
////    qDebug() << "who:" << who;
//    QRegExp andSweepCall(QString("<who>[ ]*(.*) and sweep.*").replace("<who>",who));
////    qDebug() << "andSweepCall" << andSweepCall;
//    if (s2.indexOf(andSweepCall) != -1) {
////        qDebug() << "CAPTURED:" << andSweepCall.capturedTexts();
//        s2 = andSweepCall.cap(1) + " [" + andSweepCall.cap(2) + "] and sweep 1/4\n";
//        s2 = s2.replace(QRegExp("^ "),""); // trim off possibly extra starting space
//        s2 = s2.replace("[ ","[");  // trim off possibly extra space because (.*) was greedy...
//    }

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

//        sd->write("heads start\n");   // THIS IS OPTIONAL.  For now, don't do it -- user needs to say it.
//        sd->waitForBytesWritten();

    } else if (s2 == "undo last call\n") {
        // TODO: put more synonyms of this in...
//        qDebug() << "sending to SD: \"undo last call\n\"";
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
//        qDebug() << "sending to SD:" << s2;
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

//    qDebug() << pathToPS << PSargs;

    ps = new QProcess(Q_NULLPTR);

    ps->setWorkingDirectory(QCoreApplication::applicationDirPath()); // NOTE: nothing will be written here
    ps->start(pathToPS, PSargs);

//    qDebug() << "Waiting to start ps...";
    ps->waitForStarted();
//    qDebug() << "   started.";

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

//    qDebug() << "sequencesDir:" << sequencesDir;

    sd->setWorkingDirectory(sequencesDir);
    sd->setProcessChannelMode(QProcess::MergedChannels);
//    qDebug() << "pathToSD:" << pathToSD << ", args:" << SDargs;
    sd->start(pathToSD, SDargs);

//    qDebug() << "Waiting to start sd...";
    if (sd->waitForStarted() == false) {
        qDebug() << "ERROR: sd did not start properly.";
    } else {
//        qDebug() << "sd started.";
    }

    // DEBUG: Send a couple of startup calls to SD...
//    if (true) {
//        sd->write("heads start\n\r");  // DEBUG
//        sd->waitForBytesWritten();
//        sd->write("square thru 4\n"); // DEBUG
//        sd->waitForBytesWritten();
//    } else {
//        sd->write("just as they are\n");
//        sd->waitForBytesWritten();
//    }

    connect(sd, &QProcess::readyReadStandardOutput, this, &MainWindow::readSDData);  // output data from sd
//    connect(sd, &QProcess::readyReadStandardError, this, &MainWindow::readSDData);  // Not needed, now that we merged the output channels
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

void MainWindow::playSFX(QString which) {
    QString soundEffect = musicRootPath + "/soundfx/" + which + ".mp3";
//    qDebug() << "PLAY SFX:" << soundEffect;
    if(QFileInfo(soundEffect).exists()) {
        // play sound FX only if file exists...
        cBass.PlaySoundEffect(soundEffect.toLocal8Bit().constData());  // convert to C string; defaults to volume 100%
    }
}
