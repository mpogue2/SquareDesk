/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
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
#include "globaldefines.h"

#include <QActionGroup>
#include <QColorDialog>
#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QHostInfo>
#include <QMap>
#include <QMapIterator>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressDialog>
#include <QRandomGenerator>
#include <QScreen>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QThread>
#include <QStandardItemModel>
#include <utility>
#include <QStandardItem>
#include <QWidget>
#include <QInputDialog>

#include <QPrinter>
#include <QPrintDialog>

// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#include "cuesheetmatchingdebugdialog.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"
#include "perftimer.h"
#include "exportdialog.h"
#include "songhistoryexportdialog.h"
#include "calllistcheckbox.h"
#include "sessioninfo.h"
#include "startupwizard.h"
#include "makeflashdrivewizard.h"
#include "songlistmodel.h"
#include "mytablewidget.h"

#include "svgWaveformSlider.h"


#if defined(Q_OS_MAC) | defined(Q_OS_WIN)
#ifndef M1MAC
#include "JlCompress.h"
#endif
#endif

// extern bool comparePlaylistExportRecord(const PlaylistExportRecord &a, const PlaylistExportRecord &b);

// experimental removal of silence at the beginning of the song
// disabled right now, because it's not reliable enough.
//#define REMOVESILENCE 1

// REMINDER TO FUTURE SELF: (I forget how to do this every single time) -- to set a layout to fill a single tab:
//    In designer, you should first in form preview select requested tab,
//    than in tree-view click to PARENT QTabWidget and set the layout as for all tabs.
//    Really this layout appears as new properties for selected tab only. Every tab has own layout.
//    And, the tab must have at least one widget on it already.
// https://stackoverflow.com/questions/3492739/auto-expanding-layout-with-qt-designer

// #include <iostream>
// #include <sstream>
// #include <iomanip>
using namespace std;

// TAGLIB stuff is MAC OS X and WIN32 only for now...
#include <taglib/toolkit/tlist.h>
#include <taglib/fileref.h>
#include <taglib/toolkit/tfile.h>
#include <taglib/tag.h>
#include <taglib/toolkit/tpropertymap.h>

#include <taglib/mpeg/mpegfile.h>
#include <taglib/mpeg/id3v2/id3v2tag.h>
#include <taglib/mpeg/id3v2/id3v2frame.h>
#include <taglib/mpeg/id3v2/id3v2header.h>

#include <taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.h>
#include <taglib/mpeg/id3v2/frames/commentsframe.h>
#include <taglib/mpeg/id3v2/frames/textidentificationframe.h>
// #include <string>

#include "typetracker.h"

using namespace TagLib;

extern flexible_audio *cBass;

// ----------------------------------------------------------------------
// MAINWINDOW CONSTRUCTOR -----
// ----------------------------------------------------------------------

MainWindow::MainWindow(SplashScreen *splash, bool dark, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    sd_redo_stack(new SDRedoStack())
{
    Q_UNUSED(dark)

    // NEW INITIALIZATION STUFF ==============
    PerfTimer t("MainWindow::MainWindow", __LINE__);
    t.start(__LINE__);

    splash->setProgress(5, "Initializing the playback engine...");
    theSplash = splash;

#if defined(Q_OS_LINUX)
#define OS_FALLTHROUGH [[fallthrough]]
#elif defined(Q_OS_WIN)
#define OS_FALLTHROUGH
#else
    // already defined on Mac OS X
#endif

    // INITIALIZE AUDIO SUBSYSTEM UP FRONT -----------
    initializeAudioEngine();

    // General UI initialization ----------------------
    splash->setProgress(10, "Setting up SquareDesk for you...");
    initializeUI();

    // Music tab initialization
    splash->setProgress(15, "Initializing the Music tab...");
    initializeMusicPlaybackControls();

    splash->setProgress(20, "Finding your music...");
    initializeMusicSearch();

    splash->setProgress(40, "Loading your music...");
    initializeMusicSongTable();

    splash->setProgress(50, "Loading your playlists...");
    initializeMusicPlaylists();

    // Other tabs initialization
    splash->setProgress(55, "Initializing the Cuesheet tab...");
    initializeCuesheetTab();

    splash->setProgress(60, "Loading SD and Taminations...");
    initializeSDTab();
    initializeTaminationsTab();

    splash->setProgress(65, "Initializing the Dance Programs Tab...");
    initializeDanceProgramsTab();

    splash->setProgress(70, "Initializing the Reference Tab...");
    initializeReferenceTab();

    // OLD INITIALIZATION STUFF ==============


    // NOTE: This MUST be down here at the end of the constructor now.
    //   The startup sequence takes longer now, and we don't want the
    //   timer to start too early, or it won't affect the focus.
    //
    // startup the file watcher, AND make sure that focus has gone to the Title search field
    QTimer::singleShot(100, [this]{
        // qDebug("Starting up FileWatcher now (intentionally delayed from app startup, to avoid Box.net locks retriggering loadMusicList)");
        QObject::connect(&musicRootWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(musicRootModified(QString)));

        if (!ui->darkSearch->hasFocus()) {
            // qDebug() << "HACK: DARK SEARCH DOES NOT HAVE FOCUS. FIXING THIS.";
            ui->darkSearch->setFocus();
        }

        ui->darkSongTable->selectRow(0);
        ui->darkSongTable->scrollToItem(ui->darkSongTable->item(0, kTypeCol)); // EnsureVisible row 0 (which is highlighted)
    });

    splash->setProgress(90, "Almost there...");

    mainWindowReady = true;

    initializeSessions();

    // this is down here intentionally.
    // darkLoadMusicList will either:
    //  a) not have a previous sort order, so it will sort by label/title/type
    //  b) it will have a previous sort order, so it will set that up.
    // Now that it's initialized, this connect will allow FUTURE clicks on the darksongTable's header to
    //   persist the NEW sort order, based on any FUTURE changes to it.
    connect(ui->darkSongTable, SIGNAL(newStableSort(QString)),
            this, SLOT(handleNewSort(QString))); // for persisting stable sort of darkSongTable

    // if the user wants to go back to the default sort order
    ui->darkSongTable->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->darkSongTable->horizontalHeader(), &QTableWidget::customContextMenuRequested,
            this, [this]() {
                QMenu *hdrMenu = new QMenu(this);
                hdrMenu->setProperty("theme", currentThemeString);
                hdrMenu->addAction(QString("Reset Sort Order"),
                                   [this]() {   // qDebug() << "Resetting sort order...";
                                       sortByDefaultSortOrder(); } );
                hdrMenu->popup(QCursor::pos());
                hdrMenu->exec();
                // delete hdrMenu; // done with it

            });

// This needs to be down here
#ifdef USE_JUCE
    // JUCE ---------------
    juce::initialiseJuce_GUI(); // not sure this is needed
    scanForPlugins();
#endif

    initializeLightDarkTheme();

}
// END CONSTRUCTOR ---------

// ========================================================================================================
// ========================================================================================================
// General UI initialization
void MainWindow::initializeUI() {
    oldFocusWidget = nullptr;
    trapKeypresses = true;
    totalZoom = 0;
    // darkmode = dark; // true if we're using the new dark UX
    darkmode = true; // true if we're using the new dark UX

    checkLockFile(); // warn, if some other copy of SquareDesk has database open

    // Disable extra (Native Mac) tab bar
#if defined(Q_OS_MAC)
    macUtils.disableWindowTabbing();
#endif

    prefDialog = nullptr;      // no preferences dialog created yet

    ui->setupUi(this);

    ui->statusBar->showMessage("");
    micStatusLabel = new QLabel("MICS OFF");
    ui->statusBar->addPermanentWidget(micStatusLabel);

    usePersistentFontSize(); // sets the font of the songTable, which is used by adjustFontSizes to scale other font sizes

    this->setWindowTitle(QString("SquareDesk Music Player/Sequence Editor"));

    ui->menuFile->addSeparator();

    // NOTE: MAC OS X & Linux ONLY
#if !defined(Q_OS_WIN)
    QAction *aboutAct = new QAction(QIcon(), tr("&About SquareDesk..."), this);
    aboutAct->setStatusTip(tr("SquareDesk Information"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(aboutBox()));
    ui->menuFile->addAction(aboutAct);
#endif

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

    keybindingActionToMenuAction[keyActionName_StopSong] = ui->actionStop;
    //   keybindingActionToMenuAction[keyActionName_RestartSong] = ;
    keybindingActionToMenuAction[keyActionName_Forward15Seconds] = ui->actionSkip_Forward;
    keybindingActionToMenuAction[keyActionName_Backward15Seconds] = ui->actionSkip_Backward;
    keybindingActionToMenuAction[keyActionName_VolumePlus] = ui->actionVolume_Up;
    keybindingActionToMenuAction[keyActionName_VolumeMinus] = ui->actionVolume_Down;
    keybindingActionToMenuAction[keyActionName_TempoPlus] = ui->actionSpeed_Up;
    keybindingActionToMenuAction[keyActionName_TempoMinus] = ui->actionSlow_Down;
    // keybindingActionToMenuAction[keyActionName_PlayPrevious] = ui->actionPrevious_Playlist_Item;
    // keybindingActionToMenuAction[keyActionName_PlayNext] = ui->actionNext_Playlist_Item;
    keybindingActionToMenuAction[keyActionName_Mute] = ui->actionMute;
    keybindingActionToMenuAction[keyActionName_PitchPlus] = ui->actionPitch_Up;
    keybindingActionToMenuAction[keyActionName_PitchMinus] = ui->actionPitch_Down;
    keybindingActionToMenuAction[keyActionName_FadeOut ] = ui->actionFade_Out;
    keybindingActionToMenuAction[keyActionName_LoopToggle] = ui->actionLoop;
    keybindingActionToMenuAction[keyActionName_TestLoop] = ui->actionTest_Loop;
    keybindingActionToMenuAction[keyActionName_PlaySong] = ui->actionPlay;

    keybindingActionToMenuAction[keyActionName_SDSquareYourSets] = ui->actionSDSquareYourSets;
    keybindingActionToMenuAction[keyActionName_SDHeadsStart] = ui->actionSDHeadsStart;
    keybindingActionToMenuAction[keyActionName_SDHeadsSquareThru] = ui->actionSDHeadsSquareThru;
    keybindingActionToMenuAction[keyActionName_SDHeads1p2p] = ui->actionSDHeads1p2p;

    keybindingActionToMenuAction[keyActionName_PlaySong] = ui->actionPlay;

    // This lets us set default hotkeys in the menus so that the default button in the dialog box works.
    QHash<QString, KeyAction*> menuHotkeyMappings;
    AddHotkeyMappingsFromMenus(menuHotkeyMappings);
    KeyAction::setKeybindingsFromMenuObjects(menuHotkeyMappings);

    QHash<QString, KeyAction *> hotkeyMappings = prefsManager.GetHotkeyMappings();
    AddHotkeyMappingsFromMenus(hotkeyMappings);

    QVector<KeyAction *> availableActions = KeyAction::availableActions();
    for (auto action : availableActions)
    {
        action->setMainWindow(this);
        for (int keypress = 0; keypress < MAX_KEYPRESSES_PER_ACTION; ++keypress)
        {
            QString what("%1 - %2");
            QShortcut *shortcut(new QShortcut(this));
            shortcut->setEnabled(false);
            shortcut->setWhatsThis(what.arg(action->name()).arg(keypress));
            //            qDebug() << "availableAction:" << action->name() << "," << keypress;
            //            qDebug() << "shortcut: " << shortcut;
            hotkeyShortcuts[action->name()].append(shortcut);
            connect(shortcut, SIGNAL(activated()), action, SLOT(do_activated()));
            connect(shortcut, SIGNAL(activatedAmbiguously()), action, SLOT(do_activated()));
        }
    }

    SetKeyMappings(hotkeyMappings, hotkeyShortcuts);

#if !defined(Q_OS_MAC)
    // disable this menu item for WIN and LINUX, until
    //   the rsync stuff is ported to those platforms
    ui->actionMake_Flash_Drive_Wizard->setVisible(false);
#endif

    // setup playback timer
    UIUpdateTimer = new QTimer(this);
    connect(UIUpdateTimer, SIGNAL(timeout()), this, SLOT(on_UIUpdateTimerTick()));
    UIUpdateTimer->start(1000);           //adjust from GUI with timer->setInterval(newValue)

    closeEventHappened = false;

    Info_Seekbar(false);

    connect(ui->theSVGClock, SIGNAL(newState(QString)), this, SLOT(svgClockStateChanged(QString)));

    // CLOCK COLORING =============
    ui->theSVGClock->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->theSVGClock, SIGNAL(customContextMenuRequested(QPoint)), ui->theSVGClock, SLOT(customMenuRequested(QPoint)));

    // where is the root directory where all the music is stored?
    pathStack = new QList<QString>();
    pathStackCuesheets      = new QList<QString>();
    pathStackPlaylists      = new QList<QString>();
    pathStackApplePlaylists = new QList<QString>();
    pathStackReference      = new QList<QString>();
    currentlyShowingPathStack = nullptr; // nothing is showing yet

    musicRootPath = prefsManager.GetmusicPath();      // defaults to ~/squareDeskMusic at very first startup

    // make required folders in the MusicDir, if and only if they are not there already --------
    maybeMakeAllRequiredFolders();
    maybeInstallSoundFX();
    maybeInstallReferencefiles();
    maybeInstallTemplates();

    // ERROR LOGGING ------
    logFilePath = musicRootPath + "/.squaredesk/debug.log";

//#define DISABLEFILEWATCHER 1
#ifndef DISABLEFILEWATCHER
    PerfTimer t2("filewatcher init", __LINE__);

    // ---------------------------------------
    //    // let's watch for changes in the musicDir
    //    QDirIterator it(musicRootPath, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QDirIterator it(musicRootPath, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    static QRegularExpression ignoreTheseDirs("/(reference|choreography|notes|playlists|sd|soundfx|lyrics)");
    while (it.hasNext()) {
        QString aPath = it.next();
        //        if (ignoreTheseDirs.indexIn(aPath) == -1) {
        if (aPath.indexOf(ignoreTheseDirs) == -1) {
            musicRootWatcher.addPath(aPath); // watch for add/deletes to musicDir and interesting subdirs
            // qDebug() << "adding to musicRootWatcher: " << aPath;
        }
    }

    // musicRootWatcher.addPath(musicRootPath); // watch for add/deletes to musicDir, too
    // qDebug() << "Also adding to musicRootWatcher: " << musicRootPath;


    fileWatcherTimer = new QTimer();            // Retriggerable timer for file watcher events
    QObject::connect(fileWatcherTimer, SIGNAL(timeout()), this, SLOT(fileWatcherTriggered())); // this calls musicRootModified again (one last time!)

    fileWatcherDisabledTimer = new QTimer();    // 5 sec timer for working around the Venture extended attribute problem
    QObject::connect(fileWatcherDisabledTimer, SIGNAL(timeout()), this, SLOT(fileWatcherDisabledTriggered())); // this calls musicRootModified again (one last time!)

#ifdef DEBUG_LIGHT_MODE
    QString qssDir = qApp->applicationDirPath() + "/../Resources";
    // qDebug() << "WATCHING: " << qssDir;
    lightModeWatcher.addPath(qssDir); // watch for add/deletes for light mode QSS file changing

    QObject::connect(&lightModeWatcher, SIGNAL(directoryChanged(QString)),
                     this, SLOT(themesFileModified()));

#endif

    playlistSlotWatcherTimer = new QTimer();            // Retriggerable timer for slot watcher events
    QObject::connect(playlistSlotWatcherTimer, SIGNAL(timeout()),
                     this, SLOT(playlistSlotWatcherTriggered()));

    // make sure that the "downloaded" directory exists, so that when we sync up with the Cloud,
    //   it will cause a rescan of the songTable and dropdown

    // QDir dir(musicRootPath + "/lyrics/downloaded");
    // if (!dir.exists()) {
    //     dir.mkpath(".");
    // }

    // ---------------------------------------
//    qDebug() << "Dirs 1:" << musicRootWatcher.directories();
//    qDebug() << "Dirs 2:" << lyricsWatcher.directories();

#endif

    // set initial colors for text in songTable, also used for shading the clock
    patterColorString = prefsManager.GetpatterColorString();
    singingColorString = prefsManager.GetsingingColorString();
    calledColorString = prefsManager.GetcalledColorString();
    extrasColorString = prefsManager.GetextrasColorString();

    // Tell the clock what colors to use for session segments
    ui->theSVGClock->setColorForType(NONE, QColor(Qt::red));
    ui->theSVGClock->setColorForType(PATTER, QColor(patterColorString));
    ui->theSVGClock->setColorForType(SINGING, QColor(singingColorString));
    ui->theSVGClock->setColorForType(SINGING_CALLED, QColor(calledColorString));
    ui->theSVGClock->setColorForType(XTRAS, QColor(extrasColorString));
    // ----------------------------------------------
    // Save the new settings for experimental break and patter timers --------
    tipLengthTimerEnabled = prefsManager.GettipLengthTimerEnabled();
    tipLength30secEnabled = prefsManager.GettipLength30secEnabled();
    // int tipLengthTimerLength = prefsManager.GettipLengthTimerLength();
    tipLengthAlarmAction = prefsManager.GettipLengthAlarmAction();

    breakLengthTimerEnabled = prefsManager.GetbreakLengthTimerEnabled();
    breakLengthTimerLength = prefsManager.GetbreakLengthTimerLength();
    breakLengthAlarmAction = prefsManager.GetbreakLengthAlarmAction();

    ui->theSVGClock->tipLengthTimerEnabled = tipLengthTimerEnabled;      // tell the clock whether the patter alarm is enabled
    ui->theSVGClock->tipLength30secEnabled = tipLength30secEnabled;      // tell the clock whether the patter 30 sec warning is enabled
    ui->theSVGClock->breakLengthTimerEnabled = breakLengthTimerEnabled;  // tell the clock whether the break alarm is enabled

    songFilenameFormat = static_cast<SongFilenameMatchingType>(prefsManager.GetSongFilenameFormat());

    SetAnimationSpeed(static_cast<AnimationSpeed>(prefsManager.GetAnimationSpeed()));

    // define type names (before reading in the music filenames!) ------------------
    QString value;
    value = prefsManager.GetMusicTypeSinging();
    songTypeNamesForSinging = value.toLower().split(";", Qt::KeepEmptyParts);

    value = prefsManager.GetMusicTypePatter();
    songTypeNamesForPatter = value.toLower().split(";", Qt::KeepEmptyParts);

    value = prefsManager.GetMusicTypeExtras();
    songTypeNamesForExtras = value.toLower().split(';', Qt::KeepEmptyParts);

    value = prefsManager.GetMusicTypeCalled();
    songTypeNamesForCalled = value.toLower().split(';', Qt::KeepEmptyParts);

    value = prefsManager.GetToggleSingingPatterSequence();
    songTypeToggleList = value.toLower().split(';', Qt::KeepEmptyParts);

    loadChoreographyList();

    clockColoringHidden = !prefsManager.GetexperimentalClockColoringEnabled();
    ui->theSVGClock->setHidden(clockColoringHidden);

    ui->actionAutostart_playback->setChecked(prefsManager.Getautostartplayback());

    // in the Designer, these have values, making it easy to visualize there
    //   must clear those out, because a song is not loaded yet.
    ui->currentLocLabel3->setText("");
    ui->timeSlash->setVisible(false);
    ui->songLengthLabel2->setText("");

    inPreferencesDialog = false;

    ui->tabWidget->setCurrentIndex(0); // DARK MODE tab is primary, regardless of last setting in Qt Designer
    on_tabWidget_currentChanged(0);    // update the menu item names

    QSize availableSize = QGuiApplication::screens()[0]->geometry().size();
    QSize newSize = QSize(availableSize.width()*0.7, availableSize.height()*0.7);

    setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            newSize,
            QGuiApplication::screens()[0]->geometry()

            )
        );

    tipLengthTimerEnabled = prefsManager.GettipLengthTimerEnabled();  // save new settings in MainWindow, too
    tipLength30secEnabled = prefsManager.GettipLength30secEnabled();
    tipLengthTimerLength = prefsManager.GettipLengthTimerLength();
    tipLengthAlarmAction = prefsManager.GettipLengthAlarmAction();

    breakLengthTimerEnabled = prefsManager.GetbreakLengthTimerEnabled();
    breakLengthTimerLength = prefsManager.GetbreakLengthTimerLength();
    breakLengthAlarmAction = prefsManager.GetbreakLengthAlarmAction();

    ui->theSVGClock->tipLengthTimerEnabled = tipLengthTimerEnabled;
    ui->theSVGClock->tipLength30secEnabled = tipLength30secEnabled;
#ifdef DEBUGCLOCK
    ui->theSVGClock->tipLengthAlarmMinutes = 10; // FIX FIX FIX
#else
    ui->theSVGClock->tipLengthAlarmMinutes = tipLengthTimerLength;
#endif
    ui->theSVGClock->tipLengthAlarm = tipLengthAlarmAction;

    ui->theSVGClock->breakLengthTimerEnabled = breakLengthTimerEnabled;
#ifdef DEBUGCLOCK
    ui->theSVGClock->breakLengthAlarmMinutes = 10; // FIX FIX FIX
#else
    ui->theSVGClock->breakLengthAlarmMinutes = breakLengthTimerLength;
#endif

    ui->warningLabelCuesheet->setText("");
    ui->warningLabelSD->setText("");
    ui->darkWarningLabel->setText("");
#ifndef DEBUG_LIGHT_MODE
    ui->darkWarningLabel->setStyleSheet("QLabel { color : red; }");
#endif

#ifdef DEBUG_LIGHT_MODE
    themesActionGroup = new QActionGroup(this);  // themes: light, dark, etc.
    themesActionGroup->setExclusive(true);
    connect(themesActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(themeTriggered(QAction*)));

    themesActionGroup->addAction(ui->actionLight);
    themesActionGroup->addAction(ui->actionDark);
#endif

    if (prefsManager.GetenableAutoAirplaneMode()) {
        airplaneMode(true);
    }

    connect(QApplication::instance(), SIGNAL(applicationStateChanged(Qt::ApplicationState)),
            this, SLOT(changeApplicationState(Qt::ApplicationState)));
    connect(QApplication::instance(), SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(focusChanged(QWidget*,QWidget*)));


    songSettings.setDefaultTagColors( prefsManager.GettagsBackgroundColorString(), prefsManager.GettagsForegroundColorString());

    {
        // MUSIC TAB VERTICAL SPLITTER IS PERSISTENT ---------
        QString sizesStr = prefsManager.GetMusicTabVerticalSplitterPosition();
        //sizesStr = "";  // DEBUG ONLY
        // qDebug() << "prefsManager Vertical sizes: " << sizesStr;
        QList<int> sizes;
        if (!sizesStr.isEmpty())
        {
            // override the current sizes with the previously saved sizes
            for (const QString &sizeStr : sizesStr.split(","))
            {
                sizes.append(sizeStr.toInt());
            }
        } else {
            sizes.append(1000);
            sizes.append(4000);
        }
        // qDebug() << "    Music Tab Vertical splitter using: " << sizes;
        ui->splitterMusicTabVertical->setSizes(sizes);
    }

    {
        // MUSIC TAB HORIZONTAL SPLITTER IS PERSISTENT ---------
        QString sizesStr = prefsManager.GetMusicTabHorizontalSplitterPosition();
        // sizesStr = "";  // DEBUG ONLY
        // qDebug() << "prefsManager Horizontal sizes: " << sizesStr;
        QList<int> sizes;
        if (!sizesStr.isEmpty())
        {
            // override the current sizes with the previously saved sizes
            for (const QString &sizeStr : sizesStr.split(","))
            {
                sizes.append(sizeStr.toInt());
            }
        } else {
            sizes.append(1000);
            sizes.append(3000);
        }
        // qDebug() << "    Music Tab Horizontal splitter using: " << sizes;
        ui->splitterMusicTabHorizontal->setSizes(sizes);
    }

    // THEME STUFF -------------
    svgClockStateChanged("UNINITIALIZED");

    // initial stylesheet loaded here -----------
    QString modeString = QString("Setting up %1 Theme...").arg(themePreference);
    // splash->setProgress(65, modeString); // Setting up {Light, Dark} theme...

    themesFileModified(); // load the Themes.qss file for the first time

    // // and set the dynamic properties for LIGHT mode to start with
    // themeTriggered(ui->actionLight); // start out with the mandatory "Light" theme (do NOT delete "Light")

    ui->actionSwitch_to_Light_Mode->setVisible(false);  // in NEW LIGHT/DARK MODES, don't show the old Switch to X Mode menu item

    // Initialize Preview Playback Device menu
    populatePlaybackDeviceMenu();

    // read label names and IDs
    readLabelNames();

    // Initialize Now Playing integration for iOS/watchOS remote control
    setupNowPlaying();
    setAcceptDrops(true);
}


// ====================================================
// Music tab initialization
void MainWindow::initializeMusicPlaybackControls() {
    lastWidgetBeforePlaybackWasSongTable = false;
    firstTimeSongIsPlayed = false;
    loadingSong = false;
    lastMinuteInHour = -1;
    lastSessionID = -2; // initial startup
    pathsOfCalledSongs.clear(); // no songs (that we know of) have been used recently


    lastAudioDeviceName = "";

    // Recall any previous flashcards file
    lastFlashcardsUserFile = prefsManager.Getlastflashcalluserfile();
    lastFlashcardsUserDirectory = prefsManager.Getlastflashcalluserdirectory();
    flashCallsVisible = false;

    filewatcherShouldIgnoreOneFileSave = false;
    filewatcherIsTemporarilyDisabled = false;

    justWentActive = false;

    soundFXfilenames.clear();
    soundFXname.clear();

    //    qDebug() << "preferences recentFenceDateTime: " << prefsManager.GetrecentFenceDateTime();
    recentFenceDateTime = QDateTime::fromString(prefsManager.GetrecentFenceDateTime(),
                                                "yyyy-MM-dd'T'hh:mm:ss'Z'");
    // recentFenceDateTime.setTimeSpec(Qt::UTC);  // set timezone (all times are UTC) <-- deprecated soon
    recentFenceDateTime.setTimeZone(QTimeZone::UTC);  // set timezone (all times are UTC)
    // qDebug() << "recent fence time (definition of 'recent'): " << recentFenceDateTime;

    songLoaded = false;     // no song is loaded, so don't update the currentLocLabel

    ui->darkPlayButton->setEnabled(false);
    ui->darkStopButton->setEnabled(false);

    currentPitch = 0;
    tempoIsBPM = false;

    cBass->SetVolume(100);

    currentVolume = 100;
    previousVolume = 100;
    Info_Volume();

    // VU Meter -----
    vuMeterTimer = new QTimer(this);
    connect(vuMeterTimer, SIGNAL(timeout()), this, SLOT(on_vuMeterTimerTick()));
    vuMeterTimer->start(100);           // adjust from GUI with timer->setInterval(newValue)

    on_monoButton_toggled(prefsManager.Getforcemono());
    on_actionNormalize_Track_Audio_toggled(prefsManager.GetnormalizeTrackAudio());

    on_actionAuto_scroll_during_playback_toggled(prefsManager.Getenableautoscrolllyrics());
    autoScrollLyricsEnabled = prefsManager.Getenableautoscrolllyrics();

    ui->theSVGClock->setTimerLabel(ui->warningLabelCuesheet, ui->warningLabelSD, ui->darkWarningLabel);  // tell the clock which labels to use for the main patter timer

    // restore the Flash Calls menu checkboxes state -----
    on_flashcallbasic_toggled(prefsManager.Getflashcallbasic());
    on_flashcallmainstream_toggled(prefsManager.Getflashcallmainstream());
    on_flashcallplus_toggled(prefsManager.Getflashcallplus());
    on_flashcalla1_toggled(prefsManager.Getflashcalla1());
    on_flashcalla2_toggled(prefsManager.Getflashcalla2());
    on_flashcallc1_toggled(prefsManager.Getflashcallc1());
    on_flashcallc2_toggled(prefsManager.Getflashcallc2());
    on_flashcallc3a_toggled(prefsManager.Getflashcallc3a());
    on_flashcallc3b_toggled(prefsManager.Getflashcallc3b());
    on_flashcalluserfile_toggled(prefsManager.Getflashcalluserfile());

    currentSongTypeName = "";
    currentSongCategoryName = "";
    currentSongTitle = "";
    // currentSongLabel = "";

    // mutually exclusive items in Flash Call Timing menu
    flashCallTimingActionGroup = new QActionGroup(this);
    ui->action5_seconds->setActionGroup(flashCallTimingActionGroup);
    ui->action10_seconds->setActionGroup(flashCallTimingActionGroup);
    ui->action15_seconds->setActionGroup(flashCallTimingActionGroup);
    ui->action20_seconds->setActionGroup(flashCallTimingActionGroup);
    ui->action15_seconds->setChecked(true);

    // mutually exclusive items in Music > Snap To menu
    snapActionGroup = new QActionGroup(this);
    ui->actionDisabled->setActionGroup(snapActionGroup);
    ui->actionNearest_Beat->setActionGroup(snapActionGroup);
    ui->actionNearest_Measure->setActionGroup(snapActionGroup);

    QString flashCallTimingSecs = prefsManager.Getflashcalltiming();
    if (flashCallTimingSecs == "5") {
        ui->action5_seconds->setChecked(true);
    } else if (flashCallTimingSecs == "10") {
        ui->action10_seconds->setChecked(true);
    } else if (flashCallTimingSecs == "15") {
        ui->action15_seconds->setChecked(true);
    } else {
        ui->action20_seconds->setChecked(true);
    }
    updateFlashFileMenu();
    readFlashCallsList();

    QString snapSetting = prefsManager.Getsnap();
    if (snapSetting == "disabled") {
        ui->actionDisabled->setChecked(true);
    } else if (snapSetting == "beat") {
        ui->actionNearest_Beat->setChecked(true);
    } else {
        // "measure"
        ui->actionNearest_Measure->setChecked(true);
    }

    // ============= DARKMODE INIT ================
    // QString darkTextColor = "#C0C0C0";
    QString darkTextColor = "black";
    if (darkmode) {
        darkTextColor = "#C0C0C0";
    }

    // DARK MODE UI TESTING --------------------

    ui->darkWarningLabel->setToolTip("Shows Time-in-Tip (Patter) in MM:SS, and Section-in-Tip (Singer).\nClick here to reset.");
    ui->currentLocLabel3->setToolTip("Shows Position-in-Song in MM:SS.");
    ui->songLengthLabel2->setToolTip("Shows Length-of-Song in MM:SS.");

    // LOOP CONTROLS =========
    ui->darkStartLoopButton->setToolTip(QString("Sets start point of a loop (Patter) or Intro point (Singing Call)\n\nShortcuts: set Start [, set Start and End: %1[").arg(QChar(0x21e7)));
    ui->darkEndLoopButton->setToolTip(QString("Sets end point of a loop (Patter) or Outro point (Singing Call)\n\nShortcuts: set End ]"));

    // layout the QDials in QtDesigner, promote to svgDial's, and then make sure to init all 3 parameters (in this order)
    ui->darkTrebleKnob->setKnobFile("knobs/knob_bg_regular.svg");
    ui->darkTrebleKnob->setNeedleFile("knobs/knob_indicator_regular_grey.svg");
    ui->darkTrebleKnob->setArcColor("#909090"); // no longer triggers finish of init
    ui->darkTrebleKnob->setToolTip("Treble\nControls the amount of high frequencies in this song.");

    ui->darkMidKnob->setKnobFile("knobs/knob_bg_regular.svg");
    ui->darkMidKnob->setNeedleFile("knobs/knob_indicator_regular_grey.svg");
    ui->darkMidKnob->setArcColor("#909090"); // no longer triggers finish of init
    ui->darkMidKnob->setToolTip("Midrange\nControls the amount of midrange frequencies in this song.");

    ui->darkBassKnob->setKnobFile("knobs/knob_bg_regular.svg");
    ui->darkBassKnob->setNeedleFile("knobs/knob_indicator_regular_grey.svg");
    ui->darkBassKnob->setArcColor("#909090"); // no longer triggers finish of init
    ui->darkBassKnob->setToolTip("Bass\nControls the amount of low frequencies in this song.");

#ifndef DEBUG_LIGHT_MODE
    ui->label_T->setStyleSheet("color: " + darkTextColor);
    ui->label_M->setStyleSheet("color: " + darkTextColor);
    ui->label_B->setStyleSheet("color: " + darkTextColor);
#endif

    // sliders ==========

    // VOLUME:
    ui->darkVolumeSlider->setBgFile("sliders/slider_volume_deck.svg");
    ui->darkVolumeSlider->setHandleFile("sliders/knob_volume_deck.svg");
    ui->darkVolumeSlider->setVeinColor("#00797B");
    ui->darkVolumeSlider->setDefaultValue(100.0);
    ui->darkVolumeSlider->setIncrement(1.0);
    ui->darkVolumeSlider->setCenterVeinType(false);
    ui->darkVolumeSlider->setToolTip(QString("Volume (in %)\nControls the loudness of this song.\n\nShortcuts: volume up %1%2, volume down %1%3").arg(QChar(0x2318)).arg(QChar(0x2191)).arg(QChar(0x2193)));

#ifndef DEBUG_LIGHT_MODE
    ui->darkVolumeLabel->setStyleSheet("color: " + darkTextColor);
#endif

    // TEMPO:
    ui->darkTempoSlider->setBgFile("sliders/slider_pitch_deck2.svg");
    ui->darkTempoSlider->setHandleFile("sliders/knob_volume_deck.svg");
    ui->darkTempoSlider->setVeinColor("#CA4E09");
    ui->darkTempoSlider->setDefaultValue(0.0);
    ui->darkTempoSlider->setIncrement(1.0);
    ui->darkTempoSlider->setCenterVeinType(true);
    ui->darkTempoSlider->setToolTip(QString("Tempo (in BPM)\nControls the tempo of this song (independent from Pitch).\n\nShortcuts: faster %1+, slower %1-").arg(QChar(0x2318)));

#ifndef DEBUG_LIGHT_MODE
    ui->darkTempoLabel->setStyleSheet("color: " + darkTextColor);
#endif

    // PITCH:
    ui->darkPitchSlider->setBgFile("sliders/slider_pitch_deck2.svg");
    ui->darkPitchSlider->setHandleFile("sliders/knob_volume_deck.svg");
    ui->darkPitchSlider->setVeinColor("#177D0F");
    ui->darkPitchSlider->setDefaultValue(0.0);
    ui->darkPitchSlider->setIncrement(1.0);
    ui->darkPitchSlider->setCenterVeinType(true);
    ui->darkPitchSlider->setToolTip(QString("Pitch (in semitones)\nControls the pitch of this song (relative to song's original pitch).\n\nShortcuts: pitch up %1u, pitch down %1d").arg(QChar(0x2318)));

#ifndef DEBUG_LIGHT_MODE
    ui->darkPitchLabel->setStyleSheet("color: " + darkTextColor);
#endif

    // VUMETER:
    ui->darkVUmeter->levelChanged(0, 0, false);  // initialize the VUmeter

    // TITLE:
    setProp(ui->darkTitle, "flashcall", false);
    ui->darkTitle->setText(""); // get rid of the placeholder text

    // TOOLBUTTONS:
    QString toolButtonIconColor = "#A0A0A0";

    QStyle* style = QApplication::style();
    QPixmap pixmap = style->standardPixmap(QStyle::SP_MediaPlay);
    //#ifndef DEBUG_LIGHT_MODE
    QBitmap mask = pixmap.createMaskFromColor(QColor("transparent"), Qt::MaskInColor);
    pixmap.fill((QColor(toolButtonIconColor)));
    pixmap.setMask(mask);
    //#endif
    //    pixmap.save("darkPlay.png");

    darkPlayIcon = new QIcon(pixmap);

    pixmap = style->standardPixmap(QStyle::SP_MediaPause);
    //#ifndef DEBUG_LIGHT_MODE
    mask = pixmap.createMaskFromColor(QColor("transparent"), Qt::MaskInColor);
    pixmap.fill((QColor(toolButtonIconColor)));
    pixmap.setMask(mask);
    //#endif
    //    pixmap.save("darkPause.png");

    darkPauseIcon = new QIcon(pixmap);

    pixmap = style->standardPixmap(QStyle::SP_MediaStop);
    //#ifndef DEBUG_LIGHT_MODE
    mask = pixmap.createMaskFromColor(QColor("transparent"), Qt::MaskInColor);
    pixmap.fill((QColor(toolButtonIconColor)));
    pixmap.setMask(mask);
    //#endif

    //    pixmap.save("darkStop.png");

    darkStopIcon = new QIcon(pixmap);

    ui->darkStopButton->setIcon(*darkStopIcon);  // SET THE INITIAL STOP BUTTON ICON
    ui->darkPlayButton->setIcon(*darkPlayIcon);  // SET THE INITIAL PLAY BUTTON ICON

    // Note: I don't do it this way below, because changing color then requires editing the resource files.
    //  Instead, the above method allows me to change colors at any time by regenerating the cached icons from pixmaps.
    //    QPixmap pixmap2(":/graphics/darkPlay.png");
    //    QIcon darkPlayPixmap2(pixmap2);

    //    QIcon dd(":/graphics/darkPlay.png"); // I can't stick this in the MainWindow object without getting errors either...

    waveform = new float[WAVEFORMSAMPLES];

    auditionInProgress = false;
    auditionSingleShotTimer.setSingleShot(true);
    connect(&auditionSingleShotTimer, &QTimer::timeout, this,
            [this]() {
                // if the timer ever times out, it will set auditionInProgress to false;
                // qDebug() << "setting auditionInProgress to false";
                auditionInProgress = false;
            });

    ui->theSVGClock->finishInit();

    ui->darkSeekBar->finishInit();  // load everything up!

    ui->darkTrebleKnob->finishInit();
    ui->darkMidKnob->finishInit();
    ui->darkBassKnob->finishInit();

    ui->darkPitchSlider->finishInit();
    ui->darkTempoSlider->finishInit();
    ui->darkVolumeSlider->finishInit();

    ui->FXbutton->setVisible(false); // if USE_JUCE is enabled, and if LoudMax AU is present, this button will be made visible
    ui->FXbutton->setChecked(false); // checked = LoudMaxWin is visible

}

// ====================================================
void MainWindow::initializeMusicPlaylists() {
    lastSavedPlaylist = "";  // no playlists saved yet in this session
    playlistHasBeenModified = false; // playlist hasn't been modified yet

    linesInCurrentPlaylist = 0;

    currentSongPlaylistTable = nullptr;
    currentSongPlaylistRow = 0;

    connect(ui->playlist1Table, &QTableWidget::itemDoubleClicked, this, &MainWindow::handlePlaylistDoubleClick);
    connect(ui->playlist2Table, &QTableWidget::itemDoubleClicked, this, &MainWindow::handlePlaylistDoubleClick);
    connect(ui->playlist3Table, &QTableWidget::itemDoubleClicked, this, &MainWindow::handlePlaylistDoubleClick);

    // PLAYLISTS:
    ui->playlist1Label->setText("<img src=\":/graphics/icons8-menu-64.png\" width=\"10\" height=\"9\">Jokers_2023.09.20");
    ui->playlist1Label->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->playlist1Label, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customPlaylistMenuRequested(QPoint)));

    ui->playlist1Table->setMainWindow(this);
    ui->playlist1Table->resizeColumnToContents(COLUMN_NUMBER); // number
    ui->playlist1Table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // title
    ui->playlist1Table->setColumnWidth(2,20); // pitch
    ui->playlist1Table->setColumnWidth(3,45); // tempo
    ui->playlist1Table->setStyleSheet("::section { background-color: #393939; color: #A0A0A0; }");
    ui->playlist1Table->horizontalHeaderItem(0)->setTextAlignment( Qt::AlignCenter);
    ui->playlist1Table->horizontalHeaderItem(2)->setTextAlignment( Qt::AlignCenter);
    ui->playlist1Table->horizontalHeaderItem(3)->setTextAlignment( Qt::AlignCenter);
    ui->playlist1Table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->playlist1Table->verticalHeader()->setMaximumSectionSize(28);

    ui->playlist1Table->horizontalHeader()->setSectionHidden(4, true); // hide fullpath
    ui->playlist1Table->horizontalHeader()->setSectionHidden(5, true); // hide loaded
    ui->playlist1Table->horizontalHeader()->setSectionHidden(2, false); // hide pitch
    ui->playlist1Table->horizontalHeader()->setSectionHidden(3, false); // hide tempo

    // WARNING: THIS CODE IS ONLY FOR THE NUMBERS COLUMN OF PLAYLIST TABLES.
    //  THE NORMAL CONTEXT MENU IS IN PLAYLISTS.CPP SOURCE FILE.
    //
    // THIS IS THE CONTEXT MENU FOR THE WHOLE PLAYLIST1 TABLE (NOT INCLUDING THE TITLE FIELD) -------------
    ui->playlist1Table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->playlist1Table, &QTableWidget::customContextMenuRequested,
            this, [this](QPoint q) {

                // qDebug() << "***** PLAYLIST 1 CONTEXT MENU REQUESTED";
                // if (this->ui->playlist1Table->itemAt(q) == nullptr) { return; } // if mouse right-clicked over a non-existent row, just ignore it

                int rowCount = this->ui->playlist1Table->selectionModel()->selectedRows().count();
                if (rowCount < 1) {
                    return;  // if mouse clicked and nothing was selected (this should be impossible)
                }
                QMenu *plMenu = new QMenu(this);

                plMenu->setProperty("theme", currentThemeString);

                // can only move slot items if they are playlists, NOT tracks
                QString plural;
                if (rowCount == 1) {
                    plural = "item";
                } else {
                    plural = QString::number(rowCount) + " items";
                }

                // Move up/down/top/bottom in playlist
                if (!relPathInSlot[0].contains("/tracks/")) {
                    plMenu->addAction(QString("Move " + plural + " to TOP of playlist"),    [this]() { this->PlaylistItemsToTop();    } );
                    plMenu->addAction(QString("Move " + plural + " UP in playlist"),        [this]() { this->PlaylistItemsMoveUp();   } );
                    plMenu->addAction(QString("Move " + plural + " DOWN in playlist"),      [this]() { this->PlaylistItemsMoveDown(); } );
                    plMenu->addAction(QString("Move " + plural + " to BOTTOM of playlist"), [this]() { this->PlaylistItemsToBottom(); } );
                    plMenu->addSeparator();
                    plMenu->addAction(QString("Remove " + plural + " from playlist"),       [this]() { this->PlaylistItemsRemove(); } );
                }

                if (rowCount == 1) {
                    // Reveal Audio File and Cuesheet in Finder
                    plMenu->addSeparator();
                    QString fullPath = this->ui->playlist1Table->item(this->ui->playlist1Table->itemAt(q)->row(), 4)->text();
                    QString enclosingFolderName = QFileInfo(fullPath).absolutePath();
                    //                        qDebug() << "customContextMenu for playlist1Table" << fullPath << enclosingFolderName;

                    QFileInfo fi(fullPath);
                    QString menuString = "Reveal Audio File In Finder";
                    QString thingToOpen = fullPath;

                    if (!fi.exists()) {
                        menuString = "Reveal Enclosing Folder In Finder";
                        thingToOpen = enclosingFolderName;
                    }

                    plMenu->addAction(QString(menuString),
                                      [thingToOpen]() {
            // opens either the folder and highlights the file (if file exists), OR
            // opens the folder where the file was SUPPOSED to exist.
#if defined(Q_OS_MAC)
                                          QStringList args;
                                          args << "-e";
                                          args << "tell application \"Finder\"";
                                          args << "-e";
                                          args << "activate";
                                          args << "-e";
                                          args << "select POSIX file \"" + thingToOpen + "\"";
                                          args << "-e";
                                          args << "end tell";

                                          //    QProcess::startDetached("osascript", args);

                                          // same as startDetached, but suppresses output from osascript to console
                                          //   as per: https://www.qt.io/blog/2017/08/25/a-new-qprocessstartdetached
                                          QProcess process;
                                          process.setProgram("osascript");
                                          process.setArguments(args);
                                          process.setStandardOutputFile(QProcess::nullDevice());
                                          process.setStandardErrorFile(QProcess::nullDevice());
                                          qint64 pid;
                                          process.startDetached(&pid);
#endif

                                      });

                    // if the current song has a cuesheet, offer to show it to the user -----
                    QString fullMP3Path = this->ui->playlist1Table->item(this->ui->playlist1Table->itemAt(q)->row(), 4)->text();
                    QString cuesheetPath;

                    SongSetting settings1;
                    if (songSettings.loadSettings(fullMP3Path, settings1)) {
                        cuesheetPath = settings1.getCuesheetName();
                    } else {
                        // qDebug() << "Tried to revealAttachedLyricsFile, but could not get settings for: " << currentMP3filenameWithPath;
                    }

                    // qDebug() << "convertCuesheetPathNameToCurrentRoot BEFORE:" << cuesheetPath;
                    cuesheetPath = convertCuesheetPathNameToCurrentRoot(cuesheetPath);
                    // qDebug() << "convertCuesheetPathNameToCurrentRoot AFTER:" << cuesheetPath;

                    if (cuesheetPath != "") {
                        plMenu->addAction(QString("Reveal Current Cuesheet in Finder"),
                                          [this, cuesheetPath]() {
                                              showInFinderOrExplorer(cuesheetPath);
                                          }
                                          );
                        plMenu->addAction(QString("Load Cuesheets"),
                                          [this, fullMP3Path, cuesheetPath]() {
                                              maybeLoadCuesheets(fullMP3Path, cuesheetPath);
                                          }
                                          );
                    }
                }

                plMenu->popup(QCursor::pos());
                plMenu->exec();
            }
            );

    // -----
#ifndef DEBUG_LIGHT_MODE
    ui->playlist2Label->setStyleSheet("font-size: 11pt; background-color: #404040; color: #AAAAAA;");
#endif
    ui->playlist2Label->setText("<img src=\":/graphics/icons8-menu-64.png\" width=\"10\" height=\"9\">Jokers_2023.09.20");
    ui->playlist2Label->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->playlist2Label, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customPlaylistMenuRequested(QPoint)));

    ui->playlist2Table->setMainWindow(this);
    ui->playlist2Table->resizeColumnToContents(COLUMN_NUMBER); // number
    ui->playlist2Table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // title
    ui->playlist2Table->setColumnWidth(2,20); // pitch
    ui->playlist2Table->setColumnWidth(3,45); // tempo
    ui->playlist2Table->setStyleSheet("::section { background-color: #393939; color: #A0A0A0; }");
    ui->playlist2Table->horizontalHeaderItem(0)->setTextAlignment( Qt::AlignCenter );
    ui->playlist2Table->horizontalHeaderItem(2)->setTextAlignment( Qt::AlignCenter );
    ui->playlist2Table->horizontalHeaderItem(3)->setTextAlignment( Qt::AlignCenter );
    ui->playlist2Table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->playlist2Table->verticalHeader()->setMaximumSectionSize(28);

    ui->playlist2Table->horizontalHeader()->setSectionHidden(4, true); // hide fullpath
    ui->playlist2Table->horizontalHeader()->setSectionHidden(5, true); // hide loaded
    ui->playlist2Table->horizontalHeader()->setSectionHidden(2, false); // hide pitch
    ui->playlist2Table->horizontalHeader()->setSectionHidden(3, false); // hide tempo

    // WARNING: THIS CODE IS ONLY FOR THE NUMBERS COLUMN OF PLAYLIST TABLES.
    //  THE NORMAL CONTEXT MENU IS IN PLAYLISTS.CPP SOURCE FILE.
    //
    // THIS IS THE CONTEXT MENU FOR THE WHOLE PLAYLIST2 TABLE (NOT INCLUDING THE TITLE FIELD) -------------
    ui->playlist2Table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->playlist2Table, &QTableWidget::customContextMenuRequested,
            this, [this](QPoint q) {

                // qDebug() << "***** PLAYLIST 2 CONTEXT MENU REQUESTED";
                // if (this->ui->playlist2Table->itemAt(q) == nullptr) { return; } // if mouse right-clicked over a non-existent row, just ignore it

                int rowCount = this->ui->playlist2Table->selectionModel()->selectedRows().count();
                if (rowCount < 1) {
                    return;  // if mouse clicked and nothing was selected (this should be impossible)
                }
                QString plural;
                if (rowCount == 1) {
                    plural = "item";
                } else {
                    plural = QString::number(rowCount) + " items";
                }

                QMenu *plMenu = new QMenu(this);
                plMenu->setProperty("theme", currentThemeString);

                // Move up/down/top/bottom in playlist
                if (!relPathInSlot[1].contains("/tracks/")) {
                    plMenu->addAction(QString("Move " + plural + " to TOP of playlist"),    [this]() { this->PlaylistItemsToTop();    } );
                    plMenu->addAction(QString("Move " + plural + " UP in playlist"),        [this]() { this->PlaylistItemsMoveUp();   } );
                    plMenu->addAction(QString("Move " + plural + " DOWN in playlist"),      [this]() { this->PlaylistItemsMoveDown(); } );
                    plMenu->addAction(QString("Move " + plural + " to BOTTOM of playlist"), [this]() { this->PlaylistItemsToBottom(); } );
                    plMenu->addSeparator();
                    plMenu->addAction(QString("Remove " + plural + " from playlist"),       [this]() { this->PlaylistItemsRemove(); } );
                }

                if (rowCount == 1) {
                    // Reveal Audio File and Cuesheet in Finder
                    plMenu->addSeparator();
                    QString fullPath = this->ui->playlist2Table->item(this->ui->playlist2Table->itemAt(q)->row(), 4)->text();
                    QString enclosingFolderName = QFileInfo(fullPath).absolutePath();
                    //                        qDebug() << "customContextMenu for playlist1Table" << fullPath << enclosingFolderName;

                    QFileInfo fi(fullPath);
                    QString menuString = "Reveal Audio File In Finder";
                    QString thingToOpen = fullPath;

                    if (!fi.exists()) {
                        menuString = "Reveal Enclosing Folder In Finder";
                        thingToOpen = enclosingFolderName;
                    }

                    plMenu->addAction(QString(menuString),
                                      [thingToOpen]() {
            // opens either the folder and highlights the file (if file exists), OR
            // opens the folder where the file was SUPPOSED to exist.
#if defined(Q_OS_MAC)
                                          QStringList args;
                                          args << "-e";
                                          args << "tell application \"Finder\"";
                                          args << "-e";
                                          args << "activate";
                                          args << "-e";
                                          args << "select POSIX file \"" + thingToOpen + "\"";
                                          args << "-e";
                                          args << "end tell";

                                          //    QProcess::startDetached("osascript", args);

                                          // same as startDetached, but suppresses output from osascript to console
                                          //   as per: https://www.qt.io/blog/2017/08/25/a-new-qprocessstartdetached
                                          QProcess process;
                                          process.setProgram("osascript");
                                          process.setArguments(args);
                                          process.setStandardOutputFile(QProcess::nullDevice());
                                          process.setStandardErrorFile(QProcess::nullDevice());
                                          qint64 pid;
                                          process.startDetached(&pid);
#endif

                                      });

                    // if the current song has a cuesheet, offer to show it to the user -----
                    QString fullMP3Path = this->ui->playlist2Table->item(this->ui->playlist2Table->itemAt(q)->row(), 4)->text();
                    QString cuesheetPath;

                    SongSetting settings1;
                    if (songSettings.loadSettings(fullMP3Path, settings1)) {
                        cuesheetPath = settings1.getCuesheetName();
                    } else {
                        // qDebug() << "Tried to revealAttachedLyricsFile, but could not get settings for: " << currentMP3filenameWithPath;
                    }

                    // qDebug() << "convertCuesheetPathNameToCurrentRoot BEFORE:" << cuesheetPath;
                    cuesheetPath = convertCuesheetPathNameToCurrentRoot(cuesheetPath);
                    // qDebug() << "convertCuesheetPathNameToCurrentRoot AFTER:" << cuesheetPath;

                    if (cuesheetPath != "") {
                        plMenu->addAction(QString("Reveal Current Cuesheet in Finder"),
                                          [this, cuesheetPath]() {
                                              showInFinderOrExplorer(cuesheetPath);
                                          }
                                          );
                        plMenu->addAction(QString("Load Cuesheets"),
                                          [this, fullMP3Path, cuesheetPath]() {
                                              maybeLoadCuesheets(fullMP3Path, cuesheetPath);
                                          }
                                          );
                    }
                }

                plMenu->popup(QCursor::pos());
                plMenu->exec();
            }
            );

    // -----
    ui->playlist3Table->setMainWindow(this);
#ifndef DEBUG_LIGHT_MODE
    ui->playlist3Label->setStyleSheet("font-size: 11pt; background-color: #404040; color: #AAAAAA;");
#endif
    ui->playlist3Label->setText("<img src=\":/graphics/icons8-menu-64.png\" width=\"10\" height=\"9\">Jokers_2023.09.20");
    ui->playlist3Label->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->playlist3Label, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customPlaylistMenuRequested(QPoint)));

    ui->playlist3Table->resizeColumnToContents(COLUMN_NUMBER); // number
    ui->playlist3Table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // title
    ui->playlist3Table->setColumnWidth(2,20); // pitch
    ui->playlist3Table->setColumnWidth(3,45); // tempo
    ui->playlist3Table->setStyleSheet("::section { background-color: #393939; color: #A0A0A0; }");
    ui->playlist3Table->horizontalHeaderItem(0)->setTextAlignment( Qt::AlignCenter );
    ui->playlist3Table->horizontalHeaderItem(2)->setTextAlignment( Qt::AlignCenter );
    ui->playlist3Table->horizontalHeaderItem(3)->setTextAlignment( Qt::AlignCenter );
    ui->playlist3Table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->playlist3Table->verticalHeader()->setMaximumSectionSize(28);

    ui->playlist3Table->horizontalHeader()->setSectionHidden(4, true); // hide fullpath
    ui->playlist3Table->horizontalHeader()->setSectionHidden(5, true); // hide loaded
    ui->playlist3Table->horizontalHeader()->setSectionHidden(2, false); // hide pitch
    ui->playlist3Table->horizontalHeader()->setSectionHidden(3, false); // hide tempo

    // WARNING: THIS CODE IS ONLY FOR THE NUMBERS COLUMN OF PLAYLIST TABLES.
    //  THE NORMAL CONTEXT MENU IS IN PLAYLISTS.CPP SOURCE FILE.
    //
    // THIS IS THE CONTEXT MENU FOR THE WHOLE PLAYLIST3 TABLE (NOT INCLUDING THE TITLE FIELD) -------------
    ui->playlist3Table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->playlist3Table, &QTableWidget::customContextMenuRequested,
            this, [this](QPoint q) {

                // qDebug() << "***** PLAYLIST 3 CONTEXT MENU REQUESTED";
                // if (this->ui->playlist3Table->itemAt(q) == nullptr) { return; } // if mouse right-clicked over a non-existent row, just ignore it

                int rowCount = this->ui->playlist3Table->selectionModel()->selectedRows().count();
                if (rowCount < 1) {
                    return;  // if mouse clicked and nothing was selected (this should be impossible)
                }
                QString plural;
                if (rowCount == 1) {
                    plural = "item";
                } else {
                    plural = QString::number(rowCount) + " items";
                }

                QMenu *plMenu = new QMenu(this);
                plMenu->setProperty("theme", currentThemeString);

                // Move up/down/top/bottom in playlist
                if (!relPathInSlot[2].contains("/tracks/")) {
                    plMenu->addAction(QString("Move " + plural + " to TOP of playlist"),    [this]() { this->PlaylistItemsToTop();    } );
                    plMenu->addAction(QString("Move " + plural + " UP in playlist"),        [this]() { this->PlaylistItemsMoveUp();   } );
                    plMenu->addAction(QString("Move " + plural + " DOWN in playlist"),      [this]() { this->PlaylistItemsMoveDown(); } );
                    plMenu->addAction(QString("Move " + plural + " to BOTTOM of playlist"), [this]() { this->PlaylistItemsToBottom(); } );
                    plMenu->addSeparator();
                    plMenu->addAction(QString("Remove " + plural + " from playlist"),       [this]() { this->PlaylistItemsRemove(); } );
                }

                if (rowCount == 1) {
                    // Reveal Audio File and Cuesheet in Finder
                    plMenu->addSeparator();
                    QString fullPath = this->ui->playlist3Table->item(this->ui->playlist3Table->itemAt(q)->row(), 4)->text();
                    QString enclosingFolderName = QFileInfo(fullPath).absolutePath();
                    //                        qDebug() << "customContextMenu for playlist1Table" << fullPath << enclosingFolderName;

                    QFileInfo fi(fullPath);
                    QString menuString = "Reveal Audio File In Finder";
                    QString thingToOpen = fullPath;

                    if (!fi.exists()) {
                        menuString = "Reveal Enclosing Folder In Finder";
                        thingToOpen = enclosingFolderName;
                    }

                    plMenu->addAction(QString(menuString),
                                      [thingToOpen]() {
            // opens either the folder and highlights the file (if file exists), OR
            // opens the folder where the file was SUPPOSED to exist.
#if defined(Q_OS_MAC)
                                          QStringList args;
                                          args << "-e";
                                          args << "tell application \"Finder\"";
                                          args << "-e";
                                          args << "activate";
                                          args << "-e";
                                          args << "select POSIX file \"" + thingToOpen + "\"";
                                          args << "-e";
                                          args << "end tell";

                                          //    QProcess::startDetached("osascript", args);

                                          // same as startDetached, but suppresses output from osascript to console
                                          //   as per: https://www.qt.io/blog/2017/08/25/a-new-qprocessstartdetached
                                          QProcess process;
                                          process.setProgram("osascript");
                                          process.setArguments(args);
                                          process.setStandardOutputFile(QProcess::nullDevice());
                                          process.setStandardErrorFile(QProcess::nullDevice());
                                          qint64 pid;
                                          process.startDetached(&pid);
#endif

                                      });
            // if the current song has a cuesheet, offer to show it to the user -----
            QString fullMP3Path = this->ui->playlist3Table->item(this->ui->playlist3Table->itemAt(q)->row(), 4)->text();
            QString cuesheetPath;

            SongSetting settings1;
            if (songSettings.loadSettings(fullMP3Path, settings1)) {
                cuesheetPath = settings1.getCuesheetName();
            } else {
                // qDebug() << "Tried to revealAttachedLyricsFile, but could not get settings for: " << currentMP3filenameWithPath;
            }

            // qDebug() << "convertCuesheetPathNameToCurrentRoot BEFORE:" << cuesheetPath;
            cuesheetPath = convertCuesheetPathNameToCurrentRoot(cuesheetPath);
            // qDebug() << "convertCuesheetPathNameToCurrentRoot AFTER:" << cuesheetPath;

            if (cuesheetPath != "") {
                plMenu->addAction(QString("Reveal Current Cuesheet in Finder"),
                                  [this, cuesheetPath]() {
                                      showInFinderOrExplorer(cuesheetPath);
                                  }
                                  );
                plMenu->addAction(QString("Load Cuesheets"),
                                  [this, fullMP3Path, cuesheetPath]() {
                                      maybeLoadCuesheets(fullMP3Path, cuesheetPath);
                                  }
                                  );

            }
        }

        plMenu->popup(QCursor::pos());
        plMenu->exec();
    }
            );

    ui->toggleShowPaletteTables->setVisible(false);
    ui->splitterMusicTabHorizontal->setCollapsible(1, false); // TEST TEST TEST

    clearSlot(0);
    clearSlot(1);
    clearSlot(2);

    reloadPaletteSlots();  // reload all the palette slots, based on the last time we ran SquareDesk

    for (int i = 0; i < 3; i++) {
        slotModified[i] = false;
    }

    // remember what we had set last time the app ran -----
    QString numPaletteSlots = prefsManager.Getnumpaletteslots();
    if (numPaletteSlots == "0") {
        on_action0paletteSlots_triggered();
    } else if (numPaletteSlots == "1") {
        on_action1paletteSlots_triggered();
    } else if (numPaletteSlots == "2") {
        on_action2paletteSlots_triggered();
    } else {
        on_action3paletteSlots_triggered();
    }

}

// ====================================================
void MainWindow::initializeMusicSearch() {
    currentTreePath = "Tracks/";

    ui->darkSearch->setFocus();  // this should be the intial focus

    // TREEWIDGET:
    QList<QTreeWidgetItem *> trackItem = ui->treeWidget->findItems("Tracks", Qt::MatchExactly);
    doNotCallDarkLoadMusicList = true;
    trackItem[0]->setSelected(true); // entire Tracks was already loaded by actionTags
    doNotCallDarkLoadMusicList = false;

    // enable context menus for TreeWidget
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customTreeWidgetMenuRequested(QPoint)));

    // SEARCH BOX:
    ui->darkSearch->setToolTip("Search\nFilter songs by specifying Type:Label:Title.\n\nExamples:\nlove = any song where type or label or title contains 'love'\nsing::heart = singing calls where title contains 'heart'\np:riv = patter from Riverboat\netc.");

    // SEARCH -----------
    typeSearch = labelSearch = titleSearch = ""; // no filters at startup
    searchAllFields = true; // search is OR, not AND

}

// ====================================================
void MainWindow::initializeMusicSongTable() {
    doNotCallDarkLoadMusicList = false;
    longSongTableOperationCount = 0;  // initialize counter to zero (unblocked)
    startLongSongTableOperation("MainWindow");

    connect(ui->darkSongTable->horizontalHeader(), &QHeaderView::sortIndicatorChanged,
            ui->darkSongTable, &MyTableWidget::onHeaderClicked);

    ui->darkSongTable->setColumnWidth(kNumberCol,40);  // NOTE: This must remain a fixed width, due to a bug in Qt's tracking of its width.
    ui->darkSongTable->setColumnWidth(kTypeCol,96);
    ui->darkSongTable->setColumnWidth(kLabelCol,80);

    ui->darkSongTable->setColumnWidth(kTitleCol,350);
    //  TODO: kTitleCol should be always expandable, so don't set width here

    ui->darkSongTable->setColumnWidth(kRecentCol, 70);
    ui->darkSongTable->setColumnWidth(kAgeCol, 60);
    ui->darkSongTable->setColumnWidth(kPitchCol,60);
    ui->darkSongTable->setColumnWidth(kTempoCol,60);

    zoomInOut(0);  // trigger reloading of all fonts, including horizontalHeader of songTable()

    findMusic(musicRootPath, true);  // get the filenames from the user's directories

    // At this point, the pathStack was populated by findMusic, so we can do the status message:
    vampStatus = -1;  // UNKNOWN
    vampStatus = testVamp();  // 0 = GOOD, otherwise positive integer error code
    // qDebug() << "testVamp:" << testVamp();

    // QString vampIndicator = (vampStatus == 0 ? QString(QChar(0x263a)) + " " : QString(QChar(0x26a0)) + " "); // smiley vs ! warning triangle
    QString vampIndicator = (vampStatus == 0 ? "" : QString(QChar(0x26a0))); // <nothing> vs ! warning triangle
    QString msg1 = QString("%1Songs found: %2")
                       .arg(vampIndicator, QString::number(pathStack->size()));
    ui->statusBar->showMessage(msg1);

    if (vampStatus != 0) {
        ui->statusBar->setToolTip(QString("Bar/Beat detection and Segmentation disabled: please report error ") + (QString::number(100 + vampStatus)));
    } else {
        ui->statusBar->setToolTip(""); // no problems, so no tooltip for you
    }

    doNotCallDarkLoadMusicList = false; // We want it to load the darkSongTable

    currentlyShowingPathStack = pathStack;  // IMPORTANT: the very first time, we must work with the pathStack (Tracks)
    ui->actionViewTags->setChecked(prefsManager.GetshowSongTags()); // this can invoke darkLoadMusicList, so currentlyShowingPathStack must be set to pathStack by here
    on_actionViewTags_toggled(prefsManager.GetshowSongTags()); // NOTE: Calls darkLoadMusicList() to load songs, but it will be shortcutted, because it was (probably) already called above

    updateSongTableColumnView(); // update the actual view of Age/Pitch/Tempo in the songTable view

    on_actionRecent_toggled(prefsManager.GetshowRecentColumn());
    on_actionAge_toggled(prefsManager.GetshowAgeColumn());
    on_actionPitch_toggled(prefsManager.GetshowPitchColumn());
    on_actionTempo_toggled(prefsManager.GetshowTempoColumn());

    {
        // Now that the current_session_id is setup, we can load the call lists,
        //  and they will reflect the current session ID
#if defined(Q_OS_MAC) | defined(Q_OS_WIN)
        ui->tableWidgetCallList->setColumnWidth(kCallListOrderCol,67);
        ui->tableWidgetCallList->setColumnWidth(kCallListCheckedCol, 34);
        ui->tableWidgetCallList->setColumnWidth(kCallListWhenCheckedCol, 100);
        ui->tableWidgetCallList->setColumnWidth(kCallListTimingCol, 200);
#elif defined(Q_OS_LINUX)
        ui->tableWidgetCallList->setColumnWidth(kCallListOrderCol,40);
        ui->tableWidgetCallList->setColumnWidth(kCallListCheckedCol, 24);
        ui->tableWidgetCallList->setColumnWidth(kCallListWhenCheckedCol, 100);
        ui->tableWidgetCallList->setColumnWidth(kCallListTimingCol, 200);
#endif
        ui->tableWidgetCallList->verticalHeader()->setVisible(false);  // turn off row numbers (we already have the Teach order, which is #'s)

        // #define kCallListNameCol        2
        QHeaderView *headerView = ui->tableWidgetCallList->horizontalHeader();
        headerView->setSectionResizeMode(kCallListOrderCol, QHeaderView::Fixed);
        headerView->setSectionResizeMode(kCallListCheckedCol, QHeaderView::Fixed);
        headerView->setSectionResizeMode(kCallListNameCol, QHeaderView::Stretch);
        headerView->setSectionResizeMode(kCallListWhenCheckedCol, QHeaderView::Fixed);
        headerView->setSectionResizeMode(kCallListTimingCol, QHeaderView::Stretch);
        headerView->setStretchLastSection(true);
        QString lastDanceProgram(prefsManager.MySettings.value("lastCallListDanceProgram").toString());
        loadDanceProgramList(lastDanceProgram);

        ui->tableWidgetCallList->resizeColumnToContents(kCallListWhenCheckedCol);  // and force resizing of column width to match date
        ui->tableWidgetCallList->resizeColumnToContents(kCallListNameCol);  // and force resizing of column width to match names
    }

    lastSongTableRowSelected = -1;  // meaning "no selection"

    if (ui->darkSongTable->rowCount() >= 1) {
        // ui->darkSongTable->selectRow(0); // select row 1 after initial load of the songTable (if there are rows)
        ui->darkSearch->setFocus();
    }

    // SONGTABLE:
    ui->darkSongTable->setAutoScroll(true); // Ensure that selection is always visible

    QHeaderView *verticalHeader = ui->darkSongTable->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->darkSongTable->setAlternatingRowColors(true);

#ifndef DEBUG_LIGHT_MODE
    ui->darkSongTable->setStyleSheet("::section { background-color: #393939; color: #A0A0A0; }");
#endif

//    splash->setProgress(45, "Adjusting the column layout...");

    ui->darkSongTable->resizeColumnToContents(kNumberCol);  // and force resizing of column widths to match songs
    ui->darkSongTable->resizeColumnToContents(kTypeCol);
    ui->darkSongTable->resizeColumnToContents(kLabelCol);
    ui->darkSongTable->resizeColumnToContents(kPitchCol);
    ui->darkSongTable->resizeColumnToContents(kTempoCol);

    ui->darkSongTable->setMainWindow(this);

    stopLongSongTableOperation("MainWindow");

}

// ====================================================
// Other tabs initialization
void MainWindow::initializeCuesheetTab() {
    lastCuesheetSavePath = "";
    cuesheetEditorReactingToCursorMovement = false;
    cuesheetIsUnlockedForEditing = false;

    lyricsCopyIsAvailable = false;
    lyricsTabNumber = 1;
    lyricsForDifferentSong = false;
    cueSheetLoaded = false;
    override_filename = "";
    override_cuesheet = "";

    loadedCuesheetNameWithPath = "";
    cuesheetDebugDialog = nullptr;

    // switchToLyricsOnPlay = false;
    switchToLyricsOnPlay = prefsManager.GetswitchToLyricsOnPlay();

    ui->textBrowserCueSheet->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textBrowserCueSheet, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customLyricsMenuRequested(QPoint)));

    // LYRICS TAB ------------
    ui->pushButtonSetIntroTime->setEnabled(false);  // initially not singing call, buttons will be greyed out on Lyrics tab
    ui->pushButtonSetOutroTime->setEnabled(false);

    ui->darkStartLoopButton->setEnabled(false);  // initially not singing call, buttons will be greyed out on Lyrics tab
    ui->darkEndLoopButton->setEnabled(false);

    ui->dateTimeEditIntroTime->setEnabled(false);  // initially not singing call, buttons will be greyed out on Lyrics tab
    ui->dateTimeEditOutroTime->setEnabled(false);

    ui->darkStartLoopTime->setEnabled(false);  // initially not singing call, buttons will be greyed out on Lyrics tab
    ui->darkEndLoopTime->setEnabled(false);

    ui->pushButtonTestLoop->setHidden(false); // ALWAYS VISIBLE NOW
    ui->pushButtonTestLoop->setEnabled(false);

    //    ui->darkTestLoopButton->setHidden(true);
    ui->darkTestLoopButton->setEnabled(false);

    ui->darkSegmentButton->setHidden(true);
    ui->darkSegmentButton->setEnabled(false);
    ui->darkSegmentButton->setToolTip("EXPERIMENTAL: Click to segment a patter recording, to help set loops.\nCan take up to 30 seconds to complete.");

    lastCuesheetSavePath = prefsManager.MySettings.value("lastCuesheetSavePath").toString();

    maybeLoadCSSfileIntoTextBrowser(true);

    ui->textBrowserCueSheet->setFocusPolicy(Qt::NoFocus);  // lyrics editor can't get focus until unlocked

    lockForEditing();

    connect(ui->textBrowserCueSheet, SIGNAL(copyAvailable(bool)),
            this, SLOT(LyricsCopyAvailable(bool)));

    ui->seekBarCuesheet->setFusionMode(true); // allow click-to-move-there
    updateSongTableColumnView();

    // set up Template menu ---------
    templateMenu = new QMenu(this);

    // find all template HTML files, and add to the menu
    QString templatesDir = musicRootPath + "/lyrics/templates";
    QString templatePattern = "*template*html";
    QDir currentDir(templatesDir);
    // const QString prefix = templatesDir + "/";
    for (const auto &match : currentDir.entryList(QStringList(templatePattern), QDir::Files | QDir::NoSymLinks)) {
        // qDebug() << "FOUND ONE: " << match;
        QString s = match;
        s.replace(".html", "");
        QAction *a = templateMenu->addAction(s, this, [this] { newFromTemplate(); });
        a->setProperty("name", s); // e.g. "lyrics.template"
    }

    ui->pushButtonNewFromTemplate->setMenu(templateMenu);

    ui->pushButtonEditLyrics->setVisible(false); // don't show this button until there are cuesheets available to edit
    ui->pushButtonNewFromTemplate->setVisible(false); // don't show this button until there is a song loaded

    // Initialize Cuesheet menu
    setupCuesheetMenu();

}


// ========
void MainWindow::rescanForSDDances() {
    QMenu *oldMenu = ui->actionDance->menu(); // get the current one

    // set up Dances menu, items are mutually exclusive
    sdActionGroupDances = new QActionGroup(this);  // Dances are mutually exclusive
    sdActionGroupDances->setExclusive(true);
    connect(sdActionGroupDances, SIGNAL(triggered(QAction*)), this, SLOT(sdActionTriggeredDances(QAction*)));

    QString parentFolder = musicRootPath + "/sd/dances";
    QStringList allDances;
    //    QStringList allDancesShortnames;
    QDirIterator directories(parentFolder, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    while(directories.hasNext()){
        directories.next();
        QString thePath = directories.filePath();
        QString shortName = thePath.split('/').takeLast();
        allDances << shortName;
    }

    auto dances = allDances;

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    std::sort(dances.begin(), dances.end(), collator); // sort by natural alphanumeric order

    // qDebug() << "rescanForSDDances: " << dances;

    QMenu *dancesSubmenu = new QMenu("Load Dance"); // This is the NEW one
    QString lastDance = prefsManager.GetlastDance();
    QAction *matchAction = NULL;	// which item to be made selected
    int whichItem = 0;
    frameName = "";
    for (const auto& shortName : dances) {
        //        qDebug() << "shortName: " << shortName;
        QAction *actionOne = dancesSubmenu->addAction(shortName);
        actionOne->setActionGroup(sdActionGroupDances);
        actionOne->setCheckable(true);
        actionOne->setChecked(false);	// one will be checked below
        if (whichItem == 0 || lastDance == shortName) {
            frameName = shortName;
            matchAction = actionOne;
        }
        whichItem++;
    }
    if (matchAction != NULL) {
        sdActionTriggeredDances(matchAction); // call the init function for the last dance if found else the first one
        matchAction->setChecked(true);        // make it selected
    } // else there were not dances found at all (should be caught by test below)

    if (frameName == "") {
        // if ZERO dances found, make one
        sdLoadDance("SampleDance"); // TODO: is there a better name for this?
        // NOTE: if there was SOME frame (dance) found, then don't load it until later, when we make the menu
    }

    // ui->menuSequence->insertMenu(ui->actionDance, dancesSubmenu);
    // ui->actionDance->setVisible(false);  // TODO: This is a kludge, because it's just a placeholder, so I could stick the Dances item at the top

    ui->actionDance->setMenu(dancesSubmenu);
    if (oldMenu) {
        delete oldMenu; // clean up the old one
    }
}

// ====================================================
void MainWindow::initializeSDTab() {
    sd_animation_running = false;
    sdLastLine = -1;
    sdWasNotDrawingPicture = true;
    sdHasSubsidiaryCallContinuation = false;
    sdLastLineWasResolve = false;
    sdOutputtingAvailableCalls = false;
    sdLineEditSDInputLengthWhenAvailableCallsWasBuilt = -1;
    shortcutSDTabUndo = nullptr;
    shortcutSDTabRedo = nullptr;
    shortcutSDCurrentSequenceSelectAll = nullptr;
    shortcutSDCurrentSequenceCopy = nullptr;

    // SD ------
    readAbbreviations();

    // -----------------------------
    sdViewActionGroup = new QActionGroup(this);
    sdViewActionGroup->setExclusive(true);  // exclusivity is set up below here...
    connect(sdViewActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(sdViewActionTriggered(QAction*)));

    sessionActionGroup = new QActionGroup(this);
    sessionActionGroup->setExclusive(true);

    sdActionGroup1 = new QActionGroup(this);  // checker styles
    sdActionGroup1->setExclusive(true);
    connect(sdActionGroup1, SIGNAL(triggered(QAction*)), this, SLOT(sdActionTriggered(QAction*)));

    sdActionGroupColors = new QActionGroup(this);  // checker styles: colors
    sdActionGroupColors->setExclusive(true);
    connect(sdActionGroupColors, SIGNAL(triggered(QAction*)), this, SLOT(sdActionTriggeredColors(QAction*)));

    sdActionGroupNumbers = new QActionGroup(this);  // checker styles: numbers
    sdActionGroupNumbers->setExclusive(true);
    connect(sdActionGroupNumbers, SIGNAL(triggered(QAction*)), this, SLOT(sdActionTriggeredNumbers(QAction*)));

    sdActionGroupGenders = new QActionGroup(this);  // checker styles: genders
    sdActionGroupGenders->setExclusive(true);
    connect(sdActionGroupGenders, SIGNAL(triggered(QAction*)), this, SLOT(sdActionTriggeredGenders(QAction*)));

    // let's look through the items in the SD menu (this method is less fragile now)
    QStringList ag1;
    ag1 << "Normal" << "Color only" << "Mental image" << "Sight" << "Random" << "Tam" << "Random Color only"; // OK to have one be prefix of another

    QStringList ag2;
    ag2 << "Sequence Designer" << "Dance Arranger";

    // ITERATE THRU MENU SETTING UP EXCLUSIVITY -------
    QString submenuName;
    for (const auto &action : ui->menuSequence->actions()) {
        if (action->isSeparator()) {  // top level separator
            //            qDebug() << "separator";
        } else if (action->menu()) {  // top level menu
            //            qDebug() << "item with submenu: " << action->text();
            submenuName = action->text();  // remember which submenu we're in
            // iterating just one level down
            for (const auto &action2 : action->menu()->actions()) {
                if (action2->isSeparator()) {  // one level down separator
                    //                    qDebug() << "     separator";
                } else if (action2->menu()) {  // one level down sub-menu
                    //                    qDebug() << "     item with second-level submenu: " << action2->text();
                } else {
                    //                    qDebug() << "     item: " << action2->text() << "in submenu: " << submenuName;  // one level down item
                    if (submenuName == "Colors") {
                        sdActionGroupColors->addAction(action2);
                    } else if (submenuName == "Labels") {
                        //                        qDebug() << "init labels into actionGroupNumbers " << action2;
                        sdActionGroupNumbers->addAction(action2);  // set up the mutual exclusivity
                        ui->actionNormal_3->setChecked(true); // KLUDGE: because sdViewActionTriggered happens before the actionGroupNumbers is set up, this does the init to "NUMBERS"
                    } else if (submenuName == "Genders") {
                        sdActionGroupGenders->addAction(action2);
                    }
                }
            }
        } else {
            if (ag1.contains(action->text()) ) {
                sdActionGroup1->addAction(action); // ag1 items are all mutually exclusive, and are all at top level
                // qDebug() << "ag1 item: " << action->text(); // top level item
            } else if (ag2.contains(action->text())) {
                sdViewActionGroup->addAction(action); // ag2 items are all mutually exclusive, and are all at top level
                //                qDebug() << "ag2 item: " << action->text(); // top level item
                //                if (action->text() == "Sequence Designer") {
                if (action->text() == "Dance Arranger") {
                    //                    qDebug() << "sdViewActionTriggered";
                    sdViewActionTriggered(action); // make sure this gets run at startup time
                }
            }
        }
    }
    // END ITERATION -------

    initialize_internal_sd_tab();

    currentSDVUILevel      = "Plus"; // one of sd's names: {basic, mainstream, plus, a1, a2, c1, c2, c3a} // DEPRECATED
    currentSDKeyboardLevel = "UNK"; // one of sd's names: {basic, mainstream, plus, a1, a2, c1, c2, c3a}

    startSDThread(get_current_sd_dance_program());

    on_actionShow_group_station_toggled(prefsManager.Getenablegroupstation());
    on_actionShow_order_sequence_toggled(prefsManager.Getenableordersequence());

    {
        // SD TAB VERTICAL SPLITTER IS PERSISTENT ---------
        QString sizesStr = prefsManager.GetSDTabVerticalSplitterPosition();
        // sizesStr = "";  // DEBUG ONLY
        // qDebug() << "Vertical sizes: " << sizesStr;
        QList<int> sizes;
        if (!sizesStr.isEmpty())
        {
            // override the current sizes with the previously saved sizes
            for (const QString &sizeStr : sizesStr.split(","))
            {
                sizes.append(sizeStr.toInt());
            }
        } else {
            sizes.append(300);
            sizes.append(140);
        }
        // NOTE: assumes two widgets
        if (sizes[0] == 0) {
            sizes[0] += 300;
            sizes[1] -= 300;  // please, oh Layout Manager, pay attention to the minheight of the checkers widget
            //   so that user is not confused when the checkers widget disappears by dragging upward
            //   and they don't notice the tiny little splitter handle.
        }
        if (sizes[1] == 0) {
            sizes[0] -= 300;
            sizes[1] += 300;  // please, oh Layout Manager, pay attention to the minheight of the menu options widget
            //   so that user is not confused when the menu options widget disappears by dragging downward
            //   and they don't notice the tiny little splitter handle.
        }
        // qDebug() << "    Vertical using: " << sizes;
        ui->splitterSDTabVertical->setSizes(sizes);
    }

    {
        // SD TAB HORIZONTAL SPLITTER IS PERSISTENT ---------
        QString sizesStr = prefsManager.GetSDTabHorizontalSplitterPosition();
        // sizesStr = "";  // DEBUG ONLY
        // qDebug() << "Horizontal sizes: " << sizesStr;
        QList<int> sizes;
        if (!sizesStr.isEmpty())
        {
            // override the current sizes with the previously saved sizes
            for (const QString &sizeStr : sizesStr.split(","))
            {
                sizes.append(sizeStr.toInt());
            }
        } else {
            sizes.append(400);
            sizes.append(440);
        }
        // NOTE: assumes two widgets
        if (sizes[0] == 0) {
            sizes[0] += 300;
            sizes[1] -= 300;  // please, oh Layout Manager, pay attention to the minwidth of the current sequence widget
            //   so that user is not confused when the current sequence widget disappears by dragging leftward
            //   and they don't notice the tiny little splitter handle.
        }
        if (sizes[1] == 0) {
            sizes[0] -= 300;
            sizes[1] += 300;  // please, oh Layout Manager, pay attention to the minwidth of the checkers widget
            //   so that user is not confused when the menu options widget disappears by dragging rightward
            //   and they don't notice the tiny little splitter handle.
        }
        // qDebug() << "    Horizontal using: " << sizes;
        ui->splitterSDTabHorizontal->setSizes(sizes);
    }

    sdSliderSidesAreSwapped = false;  // start out NOT swapped
    if (prefsManager.GetSwapSDTabInputAndAvailableCallsSides())
    {
        // if we SHOULD be swapped, swap them...
        sdSliderSidesAreSwapped = true;
        ui->splitterSDTabHorizontal->addWidget(ui->splitterSDTabHorizontal->widget(0));
    }

    on_actionSD_Output_triggered(); // initialize visibility of SD Output tab in SD tab
    on_actionShow_Frames_triggered(); // show or hide frames
    //    ui->actionSequence_Designer->setChecked(true);
    ui->actionDance_Arranger->setChecked(true);  // this sets the default view

    // hide both menu items for now
    ui->actionSequence_Designer->setVisible(false);
    ui->actionDance_Arranger->setVisible(false);

    connect(ui->boy1,  &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->girl1, &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->boy2,  &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->girl2, &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->boy3,  &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->girl3, &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->boy4,  &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->girl4, &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);

    ui->boy1->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE1COLOR.name() + "; color: #000000;}");
    ui->girl1->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE1COLOR.name() + "; color: #000000;}");
    ui->boy2->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE2COLOR.name() + "; color: #000000;}");
    ui->girl2->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE2COLOR.name() + "; color: #000000;}");
    ui->boy3->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE3COLOR.name() + "; color: #000000;}");
    ui->girl3->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE3COLOR.name() + "; color: #000000;}");
    ui->boy4->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE4COLOR.name() + "; color: #000000;}");
    ui->girl4->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE4COLOR.name() + "; color: #000000;}");

    // restore SD's Coloring, Numbering, and Gendering schemes from saved -------

    QString sdColorTheme = prefsManager.GetSDColoringScheme();
    QString sdNumberTheme = prefsManager.GetSDNumberingScheme();
    QString sdGenderTheme = prefsManager.GetSDGenderingScheme();

    if (sdColorTheme == "Normal") {
        ui->actionNormal_2->setChecked(true);
    } else if (sdColorTheme == "Mental Image") {
        ui->actionMental_Image->setChecked(true);
    } else if (sdColorTheme == "Sight") {
        ui->actionSight_2->setChecked(true);
    } else if (sdColorTheme == "Randomize") {
        ui->actionRandomize->setChecked(true);
    } else if (sdColorTheme == "Tam") {
        ui->actionTam->setChecked(true);
    } else {
        qDebug() << "unknown color theme: " << sdColorTheme;
    }

    setSDCoupleColoringScheme(sdColorTheme);

    if (sdNumberTheme == "None") {
        ui->actionInvisible->setChecked(true);
    } else if (sdNumberTheme == "Numbers") {
        ui->actionNormal_3->setChecked(true);
    } else if (sdNumberTheme == "Names") {
        ui->actionNames->setChecked(true);
    } else {
        qDebug() << "unknown numbering theme: " << sdNumberTheme;
    }

    setSDCoupleNumberingScheme(sdNumberTheme);

    if (sdGenderTheme == "Normal") {
        ui->actionNormal_4->setChecked(true);
    } else if (sdGenderTheme == "Arky (reversed)") {
        ui->actionArky_reversed->setChecked(true);
    } else if (sdGenderTheme == "Randomize") {
        ui->actionRandomize_3->setChecked(true);
    } else if (sdGenderTheme == "None (hex)") {
        ui->actionNone_hex->setChecked(true);
    } else {
        qDebug() << "unknown gendering theme: " << sdGenderTheme;
    }

    setSDCoupleGenderingScheme(sdGenderTheme);

    // restore SD level from saved
    QString sdLevel = prefsManager.GetSDLevel();
    //    qDebug() << "RESTORING SD LEVEL TO: " << sdLevel;
    if (sdLevel == "Mainstream") {
        ui->actionSDDanceProgramMainstream->setChecked(true);
    } else if (sdLevel == "Plus") {
        ui->actionSDDanceProgramPlus->setChecked(true);
    } else if (sdLevel == "A1") {
        ui->actionSDDanceProgramA1->setChecked(true);
    } else if (sdLevel == "A2") {
        ui->actionSDDanceProgramA2->setChecked(true);
    } else if (sdLevel == "C1") {
        ui->actionSDDanceProgramC1->setChecked(true);
    } else if (sdLevel == "C2") {
        ui->actionSDDanceProgramC2->setChecked(true);
    } else if (sdLevel == "C3a") {
        ui->actionSDDanceProgramC3A->setChecked(true);
    } else if (sdLevel == "C3") {
        ui->actionSDDanceProgramC3->setChecked(true);
    } else if (sdLevel == "C3x") {
        ui->actionSDDanceProgramC3x->setChecked(true);
    } else if (sdLevel == "C4") {
        ui->actionSDDanceProgramC4->setChecked(true);
    } else if (sdLevel == "C4x") {
        ui->actionSDDanceProgramC4x->setChecked(true);
    } else {
        qDebug() << "ERROR: Can't restore SD to level: " << sdLevel;
    }

    dance_level currentLevel = get_current_sd_dance_program(); // quick way to translate from string("Plus") to dance_level l_plus
    setCurrentSDDanceProgram(currentLevel);

    //    // INIT SD FRAMES ----------------
    //    frameName = "hoedown1";
    //    frameFiles   << "biggie"           << "easy"           << "medium"            << "hard";
    //    frameVisible << "sidebar"          << "central"        << "sidebar"           << "sidebar";
    //    frameCurSeq  << 1                  << 1                << 1                   << 1;          // These are persistent in /sd/.current.csv
    //    frameMaxSeq  << 1                  << 1                << 1                   << 1;          // These are updated at init time by scanning.


    //    QString pathToFrameFolder(musicRootPath + "/sd/frames/" + frameName);
    //    if (!QDir(pathToFrameFolder).exists()) {
    //        // if the frameName folder does not exist, make it.
    //        qDebug() << "frameName: " << frameName << "does not exist, so making it.";
    //        QDir().mkpath(pathToFrameFolder); // now the other file creators below should work
    //    }

    //    // TODO: Do these whenever a new frame is loaded...
    //    SDMakeFrameFilesIfNeeded(); // if there aren't any files in <frameName>, make some
    //    SDGetCurrentSeqs();   // get the frameCurSeq's for each of the frameFiles (this must be
    //    SDScanFramesForMax(); // update the framMaxSeq's with real numbers (MUST BE DONE AFTER GETCURRENTSEQS)
    //    SDReadSequencesUsed();  // update the local cache with the status that was persisted in this sequencesUsed.csv

    SDtestmode = false;
    //    refreshSDframes();

    // ------------------
    // set up Dances menu, items are mutually exclusive
    rescanForSDDances();

    // ------------------


    // TODO: if any frameVisible = {central, sidebar} file is not found, disable all the edit buttons for now
    if (SDtestmode) {
        ui->pushButtonSDSave->setVisible(false);
        ui->actionSave_Sequence->setEnabled(false);
        ui->pushButtonSDUnlock->setVisible(false);
        ui->pushButtonSDNew->setVisible(false);
    }

    QMenu *saveSDMenu = new QMenu(this);

    // TODO: passing parameters in the SLOT portion?  THIS IS THE IDIOMATIC WAY TO DO IT **********
    //   from: https://stackoverflow.com/questions/5153157/passing-an-argument-to-a-slot

    // TODO: These strings must be dynamically created, based on current selections for F1-F7 frame files
    QMenu* submenuMove = saveSDMenu->addMenu("Move Sequence to");
    submenuMove->addAction(QString("F1 ") + frameFiles[0] + " ", QKeyCombination(Qt::ShiftModifier, Qt::Key_F1), this, [this]{ SDMoveCurrentSequenceToFrame(0); });
    submenuMove->addAction(QString("F2 ") + frameFiles[1] + " ", QKeyCombination(Qt::ShiftModifier, Qt::Key_F2), this, [this]{ SDMoveCurrentSequenceToFrame(1); });
    submenuMove->addAction(QString("F3 ") + frameFiles[2] + " ", QKeyCombination(Qt::ShiftModifier, Qt::Key_F3), this, [this]{ SDMoveCurrentSequenceToFrame(2); });
    submenuMove->addAction(QString("F4 ") + frameFiles[3] + " ", QKeyCombination(Qt::ShiftModifier, Qt::Key_F4), this, [this]{ SDMoveCurrentSequenceToFrame(3); });

    QMenu* submenuCopy = saveSDMenu->addMenu("Append Sequence to");
    submenuCopy->addAction(QString("F1 ") + frameFiles[0] + " ", QKeyCombination(Qt::AltModifier | Qt::ShiftModifier, Qt::Key_F1), this, [this]{ SDAppendCurrentSequenceToFrame(0); });
    submenuCopy->addAction(QString("F2 ") + frameFiles[1] + " ", QKeyCombination(Qt::AltModifier | Qt::ShiftModifier, Qt::Key_F2), this, [this]{ SDAppendCurrentSequenceToFrame(1); });
    submenuCopy->addAction(QString("F3 ") + frameFiles[2] + " ", QKeyCombination(Qt::AltModifier | Qt::ShiftModifier, Qt::Key_F3), this, [this]{ SDAppendCurrentSequenceToFrame(2); });
    submenuCopy->addAction(QString("F4 ") + frameFiles[3] + " ", QKeyCombination(Qt::AltModifier | Qt::ShiftModifier, Qt::Key_F4), this, [this]{ SDAppendCurrentSequenceToFrame(3); });

    for (const auto &action : submenuCopy->actions()){ // enable shortcuts for all Copy actions
        action->setShortcutVisibleInContextMenu(true);
    }
    for (const auto &action : submenuMove->actions()){ // enable shortcuts for all Move actions
        action->setShortcutVisibleInContextMenu(true);
    }

    ui->pushButtonSDMove->setMenu(saveSDMenu);
    ui->pushButtonSDMove->setVisible(false);

    SDExitEditMode(); // make sure buttons are visible/invisible

    selectFirstItemOnLoad = true; // TEST

    getMetadata();

    //    debugCSDSfile("plus"); // DEBUG DEBUG DEBUG THIS HELPS TO DEBUG IMPORT OF CSDS SEQUENCES TO SD FORMAT *********

    newSequenceInProgress = editSequenceInProgress = false; // no sequence being edited right now.
    on_actionFormation_Thumbnails_triggered(); // make sure that the thumbnails are turned OFF, if Formation Thumbnails is not initially checked

    // also watch the abbrevs.txt file for changes, and reload the abbreviations if it changed
    abbrevsWatcher.addPath(musicRootPath + "/sd/abbrevs.txt");
    QObject::connect(&abbrevsWatcher, SIGNAL(fileChanged(QString)), this, SLOT(readAbbreviations()));

}

// ====================================================
void MainWindow::initializeTaminationsTab() {
    // embedded HTTP server for Taminations
    startTaminationsServer();
}

// ====================================================
void MainWindow::initializeDanceProgramsTab() {

}

// ====================================================
void MainWindow::initializeReferenceTab() {
    // Reference tab ---
    webViews.clear();

    initReftab();
}

// ====================================================
void MainWindow::initializeSessions() {
    // what session are we in to start with?
    SessionDefaultType sessionDefault =
        static_cast<SessionDefaultType>(prefsManager.GetSessionDefault()); // preference setting

    if (sessionDefault == SessionDefaultDOW) {

        int currentSessionID = songSettings.currentSessionIDByTime(); // what session are we in?
        setCurrentSessionId(currentSessionID); // save it in songSettings

        populateMenuSessionOptions(); // update the sessions menu with whatever is checked now

        // splash->setProgress(25, "Looking up when songs were last played...");
        reloadSongAges(ui->actionShow_All_Ages->isChecked());


        lastSessionID = currentSessionID;
        currentSongSecondsPlayed = 0; // reset the counter, because this is a new session
        currentSongSecondsPlayedRecorded = false; // not reported yet, because this is a new session
    } else {
        // do this once at startup, and never again.  I think that was the intent of this mode.
        int practiceID = 1; // wrong, if there are deleted rows in Sessions table
        QList<SessionInfo> sessions = songSettings.getSessionInfo();

        for (const auto &s : std::as_const(sessions)) {
            // qDebug() << s.day_of_week << s.id << s.name << s.order_number << s.start_minutes;
            if (s.order_number == 0) { // 0 is the first non-deleted row where order_number == 0
                // qDebug() << "Found it: " << s.name << "row:" << s.id;
                practiceID = s.id; // now it's right!
            }
        }

        setCurrentSessionId(practiceID); // save it in songSettings

        reloadSongAges(ui->actionShow_All_Ages->isChecked());

        populateMenuSessionOptions(); // update the sessions menu with whatever is checked now
        lastSessionID = practiceID;
        currentSongSecondsPlayed = 0; // reset the counter, because this is a new session
        currentSongSecondsPlayedRecorded = false; // not reported yet, because this is a new session
        // qDebug() << "***** We are now in Practice Session, id =" << practiceID;
    }

    populateMenuSessionOptions();  // update the Session menu
}

void MainWindow::initializeAudioEngine() {

    cBass = new flexible_audio();
    connect(cBass, SIGNAL(haveDuration()), this, SLOT(haveDuration2()));  // when decode complete, we know MP3 duration
    cBass->Init();

    cBass->SetIntelBoostEnabled(prefsManager.GetintelBoostIsEnabled());
    cBass->SetIntelBoost(FREQ_KHZ, static_cast<float>(prefsManager.GetintelCenterFreq_KHz()/10.0)); // yes, we have to initialize these manually
    cBass->SetIntelBoost(BW_OCT,   static_cast<float>(prefsManager.GetintelWidth_oct()/10.0));
    cBass->SetIntelBoost(GAIN_DB,  static_cast<float>(prefsManager.GetintelGain_dB()/10.0));  // expressed as positive number

    cBass->SetPanEQVolumeCompensation(static_cast<float>(prefsManager.GetpanEQGain_dB()/2.0)); // expressed as signed half-dB's

    connect(&auditionPlayer, &QMediaPlayer::mediaStatusChanged,
            this,[=](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::MediaStatus::BufferedMedia)
            {
                // auditionPlayer->pause();
                // qDebug() << "connect: " << auditionStartHere_ms;
                auditionPlayer.setPosition(auditionStartHere_ms);
                // auditionPlayer->pause();
            }
        });
}

void MainWindow::initializeLightDarkTheme() {
    themePreference = prefsManager.GetactiveTheme();
    // qDebug() << "themePreference:" << themePreference;

    // set the theme, but do not call darkLoadSongList again
    doNotCallDarkLoadMusicList = true;  // avoid calling it twice at startup, but allow later for it to be called by both actionViewTags and ThemeToggled
    if (themePreference == "Light") {
        // qDebug() << "    setting to Light";
        ui->actionLight->setChecked(true);
        themeTriggered(ui->actionLight);
    } else if (themePreference == "Dark") {
        // qDebug() << "    setting to Dark";
        ui->actionDark->setChecked(true);
        themeTriggered(ui->actionDark);
    }
    doNotCallDarkLoadMusicList = false;
}
