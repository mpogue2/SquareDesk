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
#include "utility.h"
#include "perftimer.h"
#include "tablenumberitem.h"
#include "tablelabelitem.h"
#include "importdialog.h"
#include "embeddedserver.h"
#include "exportdialog.h"
#include "songhistoryexportdialog.h"
#include "calllistcheckbox.h"
#include "sessioninfo.h"
// #include "songtitlelabel.h"
#include "tablewidgettimingitem.h"
#include "danceprograms.h"
#include "startupwizard.h"
#include "makeflashdrivewizard.h"
// #include "downloadmanager.h"
#include "songlistmodel.h"
#include "mytablewidget.h"

#include "svgWaveformSlider.h"
// #include "auditionbutton.h"

#include "src/communicator.h"

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

// GLOBALS:

// M1 Silicon Mac only:
extern flexible_audio *cBass;  // make this accessible to PreferencesDialog
flexible_audio *cBass;

// TAGS
QString title_tags_prefix("&nbsp;<span style=\"background-color:%1; color: %2;\"> ");
QString title_tags_suffix(" </span>");
static QRegularExpression title_tags_remover("(\\&nbsp\\;)*\\<\\/?span( .*?)?>");

// TITLE COLORS
static QRegularExpression spanPrefixRemover("<span style=\"color:.*\">(.*)</span>", QRegularExpression::InvertedGreedinessOption);

#include <QProxyStyle>

class MySliderClickToMoveStyle : public QProxyStyle
{
public:
//    using QProxyStyle::QProxyStyle;

    int styleHint(QStyle::StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const
    {
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons) {
            return (Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
        }
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};


void MainWindow::SetKeyMappings(const QHash<QString, KeyAction *> &hotkeyMappings, QHash<QString, QVector<QShortcut* > > hotkeyShortcuts)
{
    QStringList hotkeys(hotkeyMappings.keys());
    QVector<KeyAction *> availableActions = KeyAction::availableActions();
    QHash<QString, int> keysSetPerAction;
    for (auto action : availableActions)
    {
        keysSetPerAction[action->name()] = -1;
        for (int keypress = 0; keypress < MAX_KEYPRESSES_PER_ACTION; ++keypress)
        {
            hotkeyShortcuts[action->name()][keypress]->setEnabled(false);
            hotkeyShortcuts[action->name()][keypress]->setKey(0);
        }
    }

    std::sort(hotkeys.begin(), hotkeys.end(), LongStringsFirstThenAlpha);

    for (const QString &hotkey : hotkeys)
    {
        auto mapping = hotkeyMappings.find(hotkey);
        QKeySequence keySequence(mapping.key());
        QString actionName(mapping.value()->name());
        int index = keysSetPerAction[actionName];

        if (index < 0 && keybindingActionToMenuAction.contains(mapping.value()->name()))
        {
            keybindingActionToMenuAction[actionName]->setShortcut(keySequence);
            keysSetPerAction[actionName] = 0;
            continue;
        }

        if (index < 0) index = 0;
        
        if (index < hotkeyShortcuts[actionName].length())
        {
            hotkeyShortcuts[actionName][index]->setKey(keySequence);
            hotkeyShortcuts[actionName][index]->setEnabled(true);
        }
        ++index;
        keysSetPerAction[actionName] = index;
        }
    }
    
void MainWindow::LyricsCopyAvailable(bool yes) {
    lyricsCopyIsAvailable = yes;
}

void InitializeSeekBar(MySlider *seekBar);  // forward decl

void MainWindow::haveDuration2(void) {
//    qDebug() << "MainWindow::haveDuration -- StreamCreate duration and songBPM now available! *****";

    // NOTE: This function is called once at load time, and adds less than 1ms
    currentSongMP3SampleRate = getMP3SampleRate(currentMP3filenameWithPath);

    cBass->StreamGetLength();  // tell everybody else what the length of the stream is...
    // InitializeSeekBar(ui->seekBar);          // and now we can set the max of the seekbars, so they show up
    InitializeSeekBar(ui->seekBarCuesheet);  // and now we can set the max of the seekbars, so they show up

    ui->darkSeekBar->setMinimum(0);
    ui->darkSeekBar->setMaximum(static_cast<int>(cBass->FileLength)-1); // tricky! see InitializeSeekBar

    cBass->getWaveform(waveform, WAVEFORMSAMPLES);
//    qDebug() << "updateBgPixmap called from haveDuration2";
//    ui->darkSeekBar->updateBgPixmap(waveform, WAVEFORMSAMPLES);  // don't need this, because we call it in secondHalfOfLoad, too

//    qDebug() << "haveDuration2 BPM = " << cBass->Stream_BPM;

    handleDurationBPM();  // finish up the UI stuff, when we know duration and BPM

    secondHalfOfLoad(currentSongTitle);  // now that we have duration and BPM, can finish up asynchronous load.

    if (!ui->actionDisabled->isChecked()) {
//        qDebug() << "haveDuration2 snapToClosest starting";
        double maybeError = cBass->snapToClosest(0.1, GRANULARITY_BEAT); // this is a throwaway call, just to initiate beat detection ONLY if beat detection is ON
        // This is 0.1, so that if Vamp is not present, it will return -0.1 (error indication)
//           it will take about 1/2 second to initiate the VAMP process, but the UX should already be updated at this point
//           another 1.6 sec later, the beatMap and measureMap will be filled in.  User should see minimal delay, and ONLY
//           if beat detection is ON.
//        qDebug() << "haveDuration2 snapToClosest returned" << maybeError;
        if (maybeError < 0.0) {
//            qDebug() << "haveDuration2: snap failed, turning off snapping!";
            ui->actionDisabled->setChecked(true); // turn off snapping
        }
    }
}

// ----------------------------------------------------------------------
// MAINWINDOW CONSTRUCTOR -----
MainWindow::MainWindow(SplashScreen *splash, bool dark, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    oldFocusWidget(nullptr),
    lastWidgetBeforePlaybackWasSongTable(false),
    lastCuesheetSavePath(),
//    timerCountUp(nullptr),
//    timerCountDown(nullptr),
    trapKeypresses(true),
//    ps(nullptr),
    firstTimeSongIsPlayed(false),
    loadingSong(false),
    cuesheetEditorReactingToCursorMovement(false),
    totalZoom(0),
    hotkeyShortcuts(),
    sd_animation_running(false),
    sdLastLine(-1),
    sdWasNotDrawingPicture(true),
    sdHasSubsidiaryCallContinuation(false),
    sdLastLineWasResolve(false),
    sdOutputtingAvailableCalls(false),
    sdAvailableCalls(),
    sdLineEditSDInputLengthWhenAvailableCallsWasBuilt(-1),
    shortcutSDTabUndo(nullptr),
    shortcutSDTabRedo(nullptr),
    shortcutSDCurrentSequenceSelectAll(nullptr),
    shortcutSDCurrentSequenceCopy(nullptr),
    sd_redo_stack(new SDRedoStack())
{
#ifdef TESTRESTARTINGSQUAREDESK
    testRestartingSquareDesk = true;  // set to false for NORMAL operation, TRUE to test restarting
#endif

    PerfTimer t("MainWindow::MainWindow", __LINE__);
    t.start(__LINE__);

    splash->setProgress(0, "Finding all your songs and lyrics...");

    doNotCallDarkLoadMusicList = false;

    currentTreePath = "Tracks/";

    QString darkTextColor = "black";
    lastMinuteInHour = -1;
    lastSessionID = -2; // initial startup

    pathsOfCalledSongs.clear(); // no songs (that we know of) have been used recently

    darkmode = dark; // true if we're using the new dark UX

//    sdtest();
    cuesheetIsUnlockedForEditing = false;

    cBass = new flexible_audio();
    //Q_UNUSED(splash)
    longSongTableOperationCount = 0;  // initialize counter to zero (unblocked)

    connect(cBass, SIGNAL(haveDuration()), this, SLOT(haveDuration2()));  // when decode complete, we know MP3 duration
    lastAudioDeviceName = "";

    lyricsCopyIsAvailable = false;
    lyricsTabNumber = 1;
    lyricsForDifferentSong = false;
    cueSheetLoaded = false;

    lastSavedPlaylist = "";  // no playlists saved yet in this session
    playlistHasBeenModified = false; // playlist hasn't been modified yet

    // Recall any previous flashcards file
    lastFlashcardsUserFile = prefsManager.Getlastflashcalluserfile();
    lastFlashcardsUserDirectory = prefsManager.Getlastflashcalluserdirectory();
    
    filewatcherShouldIgnoreOneFileSave = false;
    filewatcherIsTemporarilyDisabled = false;

    t.elapsed(__LINE__);

    theSplash = splash;
    checkLockFile(); // warn, if some other copy of SquareDesk has database open

    flashCallsVisible = false;

    // Reference tab ---
    webViews.clear();
    
    linesInCurrentPlaylist = 0;

    loadedCuesheetNameWithPath = "";
    justWentActive = false;

    soundFXfilenames.clear();
    soundFXname.clear();

    t.elapsed(__LINE__);

//    qDebug() << "preferences recentFenceDateTime: " << prefsManager.GetrecentFenceDateTime();
    recentFenceDateTime = QDateTime::fromString(prefsManager.GetrecentFenceDateTime(),
                                                          "yyyy-MM-dd'T'hh:mm:ss'Z'");
    // recentFenceDateTime.setTimeSpec(Qt::UTC);  // set timezone (all times are UTC) <-- deprecated soon
    recentFenceDateTime.setTimeZone(QTimeZone::UTC);  // set timezone (all times are UTC)
    // qDebug() << "recent fence time (definition of 'recent'): " << recentFenceDateTime;

    t.elapsed(__LINE__);

#if defined(Q_OS_LINUX)
#define OS_FALLTHROUGH [[fallthrough]]
#elif defined(Q_OS_WIN)
#define OS_FALLTHROUGH
#else
    // already defined on Mac OS X
#endif

    // Disable extra (Native Mac) tab bar
#if defined(Q_OS_MAC)
    macUtils.disableWindowTabbing();
#endif

    t.elapsed(__LINE__);

    prefDialog = nullptr;      // no preferences dialog created yet
    songLoaded = false;     // no song is loaded, so don't update the currentLocLabel

    t.elapsed(__LINE__);
    ui->setupUi(this);

    startLongSongTableOperation("MainWindow");

    t.elapsed(__LINE__);
    ui->statusBar->showMessage("");

    t.elapsed(__LINE__);
    micStatusLabel = new QLabel("MICS OFF");
    ui->statusBar->addPermanentWidget(micStatusLabel);

    // Initialize debug dialog
    cuesheetDebugDialog = nullptr;

    // NEW INIT ORDER *******
    t.elapsed(__LINE__);
    // setFontSizes();

    t.elapsed(__LINE__);
    usePersistentFontSize(); // sets the font of the songTable, which is used by adjustFontSizes to scale other font sizes

    //on_actionReset_triggered();  // set initial layout

    t.elapsed(__LINE__);

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

    t.elapsed(__LINE__);

    // ui->songTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);  // auto set height of rows

    t.elapsed(__LINE__);

    this->setWindowTitle(QString("SquareDesk Music Player/Sequence Editor"));

    // ui->playButton->setEnabled(false);
    ui->darkPlayButton->setEnabled(false);
    ui->darkStopButton->setEnabled(false);

    t.elapsed(__LINE__);

    // ============
    ui->menuFile->addSeparator();

    // ------------
    // NOTE: MAC OS X & Linux ONLY
#if !defined(Q_OS_WIN)
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

    t.elapsed(__LINE__);

#if !defined(Q_OS_MAC)
    // disable this menu item for WIN and LINUX, until
    //   the rsync stuff is ported to those platforms
    ui->actionMake_Flash_Drive_Wizard->setVisible(false);
#endif

    currentPitch = 0;
    tempoIsBPM = false;
    switchToLyricsOnPlay = false;

    Info_Seekbar(false);

    // setup playback timer
    UIUpdateTimer = new QTimer(this);
    connect(UIUpdateTimer, SIGNAL(timeout()), this, SLOT(on_UIUpdateTimerTick()));
    UIUpdateTimer->start(1000);           //adjust from GUI with timer->setInterval(newValue)

    closeEventHappened = false;

    //Create Bass audio system
    cBass->Init();

    //Set UI update
    cBass->SetVolume(100);
    currentVolume = 100;
    previousVolume = 100;
    Info_Volume();

    // VU Meter -----
    vuMeterTimer = new QTimer(this);
    connect(vuMeterTimer, SIGNAL(timeout()), this, SLOT(on_vuMeterTimerTick()));
    vuMeterTimer->start(100);           // adjust from GUI with timer->setInterval(newValue)

    t.elapsed(__LINE__);

    connect(ui->theSVGClock, SIGNAL(newState(QString)), this, SLOT(svgClockStateChanged(QString)));

    ui->textBrowserCueSheet->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textBrowserCueSheet, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customLyricsMenuRequested(QPoint)));

    // where is the root directory where all the music is stored?
    pathStack = new QList<QString>();
    pathStackCuesheets = new QList<QString>();
    pathStackPlaylists = new QList<QString>();
    pathStackApplePlaylists = new QList<QString>();
    currentlyShowingPathStack = nullptr; // nothing is showing yet

    musicRootPath = prefsManager.GetmusicPath();      // defaults to ~/squareDeskMusic at very first startup

    t.elapsed(__LINE__);

    // make required folders in the MusicDir, if and only if they are not there already --------
    maybeMakeAllRequiredFolders();
    maybeInstallSoundFX();
    maybeInstallReferencefiles();
    maybeInstallTemplates();

    t.elapsed(__LINE__);

    // SD ------
    readAbbreviations();

    // ERROR LOGGING ------
    logFilePath = musicRootPath + "/.squaredesk/debug.log";

    switchToLyricsOnPlay = prefsManager.GetswitchToLyricsOnPlay();

    // updateRecentPlaylistMenu();  // OLD LIGHT MODE

    t.elapsed(__LINE__);

    // DEBUG
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
        t.elapsed(__LINE__);
    }

    // musicRootWatcher.addPath(musicRootPath); // watch for add/deletes to musicDir, too
    // qDebug() << "Also adding to musicRootWatcher: " << musicRootPath;

    t.elapsed(__LINE__);

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

    t.elapsed(__LINE__);

    // QDir dir(musicRootPath + "/lyrics/downloaded");
    // if (!dir.exists()) {
    //     dir.mkpath(".");
    // }

    // ---------------------------------------
    t2.elapsed(__LINE__);

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

    // ----------------------------------------------
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

    t.elapsed(__LINE__);

    findMusic(musicRootPath, true);  // get the filenames from the user's directories

    splash->setProgress(25, "Loading and sorting the song table...");

    t.elapsed(__LINE__);

    doNotCallDarkLoadMusicList = false; // We want it to load the darkSongTable

    currentlyShowingPathStack = pathStack;  // IMPORTANT: the very first time, we must work with the pathStack (Tracks)
    ui->actionViewTags->setChecked(prefsManager.GetshowSongTags()); // this can invoke darkLoadMusicList, so currentlyShowingPathStack must be set to pathStack by here
    on_actionViewTags_toggled(prefsManager.GetshowSongTags()); // NOTE: Calls darkLoadMusicList() to load songs, but it will be shortcutted, because it was (probably) already called above

    t.elapsed(__LINE__);

#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    ui->listWidgetChoreographySequences->setStyleSheet(
         "QListWidget::item { border-bottom: 1px solid black; }" );
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    loadChoreographyList();

    t.elapsed(__LINE__);

#ifndef DEBUG_LIGHT_MODE
    ui->pushButtonEditLyrics->setStyleSheet("QToolButton { border: 1px solid #575757; border-radius: 4px; background-color: palette(base); }");  // turn off border
#endif

    // ----------
    updateSongTableColumnView(); // update the actual view of Age/Pitch/Tempo in the songTable view

    t.elapsed(__LINE__);

    // ----------
    clockColoringHidden = !prefsManager.GetexperimentalClockColoringEnabled();
    // analogClock->setHidden(clockColoringHidden);

    ui->theSVGClock->setHidden(clockColoringHidden);

    // -----------
    ui->actionAutostart_playback->setChecked(prefsManager.Getautostartplayback());
//    ui->actionViewTags->setChecked(prefsManager.GetshowSongTags());  // DO NOT DO THIS HERE.  SEE ABOVE loadMusicList() commented out call.

    // -------
    on_monoButton_toggled(prefsManager.Getforcemono());
    on_actionNormalize_Track_Audio_toggled(prefsManager.GetnormalizeTrackAudio());

    on_actionRecent_toggled(prefsManager.GetshowRecentColumn());
    on_actionAge_toggled(prefsManager.GetshowAgeColumn());
    on_actionPitch_toggled(prefsManager.GetshowPitchColumn());
    on_actionTempo_toggled(prefsManager.GetshowTempoColumn());

    t.elapsed(__LINE__);

    on_actionAuto_scroll_during_playback_toggled(prefsManager.Getenableautoscrolllyrics());
    autoScrollLyricsEnabled = prefsManager.Getenableautoscrolllyrics();

#ifndef Q_OS_MAC
    ui->seekBar->setStyle(new MySliderClickToMoveStyle());
    ui->tempoSlider->setStyle(new MySliderClickToMoveStyle());
    ui->pitchSlider->setStyle(new MySliderClickToMoveStyle());
    ui->volumeSlider->setStyle(new MySliderClickToMoveStyle());
    ui->mixSlider->setStyle(new MySliderClickToMoveStyle());
    ui->bassSlider->setStyle(new MySliderClickToMoveStyle());
    ui->midrangeSlider->setStyle(new MySliderClickToMoveStyle());
    ui->trebleSlider->setStyle(new MySliderClickToMoveStyle());
    ui->seekBarCuesheet->setStyle(new MySliderClickToMoveStyle());
#endif /* ifndef Q_OS_MAC */

    t.elapsed(__LINE__);

    // // in the Designer, these have values, making it easy to visualize there
    // //   must clear those out, because a song is not loaded yet.
    ui->currentLocLabel3->setText("");
    ui->timeSlash->setVisible(false);
    ui->songLengthLabel2->setText("");
    // ui->songLengthLabel2->setText("");

    inPreferencesDialog = false;

    t.elapsed(__LINE__);

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

    // if (darkmode) {
        ui->darkSearch->setFocus();  // this should be the intial focus
    // } else {
    //     ui->titleSearch->setFocus();  // this should be the intial focus
    // }

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

    t.elapsed(__LINE__);

    // LYRICS TAB ------------
    ui->pushButtonSetIntroTime->setEnabled(false);  // initially not singing call, buttons will be greyed out on Lyrics tab
    ui->pushButtonSetOutroTime->setEnabled(false);

    ui->darkStartLoopButton->setEnabled(false);  // initially not singing call, buttons will be greyed out on Lyrics tab
    ui->darkEndLoopButton->setEnabled(false);

    ui->dateTimeEditIntroTime->setEnabled(false);  // initially not singing call, buttons will be greyed out on Lyrics tab
    ui->dateTimeEditOutroTime->setEnabled(false);

    ui->darkStartLoopTime->setEnabled(false);  // initially not singing call, buttons will be greyed out on Lyrics tab
    ui->darkEndLoopTime->setEnabled(false);

    t.elapsed(__LINE__);
    ui->pushButtonTestLoop->setHidden(false); // ALWAYS VISIBLE NOW
    ui->pushButtonTestLoop->setEnabled(false);

//    ui->darkTestLoopButton->setHidden(true);
    ui->darkTestLoopButton->setEnabled(false);

    ui->darkSegmentButton->setHidden(true);
    ui->darkSegmentButton->setEnabled(false);
    ui->darkSegmentButton->setToolTip("EXPERIMENTAL: Click to segment a patter recording, to help set loops.\nCan take up to 30 seconds to complete.");

    t.elapsed(__LINE__);
    ui->theSVGClock->setTimerLabel(ui->warningLabelCuesheet, ui->warningLabelSD, ui->darkWarningLabel);  // tell the clock which labels to use for the main patter timer

    t.elapsed(__LINE__);

    // read list of calls (in application bundle on Mac OS X)
    // TODO: make this work on other platforms, but first we have to figure out where to put the allcalls.csv
    //   file on those platforms.  It's convenient to stick it in the bundle on Mac OS X.  Maybe parallel with
    //   the executable on Windows and Linux?

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

    t.elapsed(__LINE__);

    t.elapsed(__LINE__);

    currentSongTypeName = "";
    currentSongCategoryName = "";
    currentSongTitle = "";
    currentSongLabel = "";

    // -----------------------------
    sdViewActionGroup = new QActionGroup(this);
    sdViewActionGroup->setExclusive(true);  // exclusivity is set up below here...
    connect(sdViewActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(sdViewActionTriggered(QAction*)));

    // -----------------------------
    sessionActionGroup = new QActionGroup(this);
    sessionActionGroup->setExclusive(true);

    // -----------------------

    t.elapsed(__LINE__);

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

#ifdef DEBUG_LIGHT_MODE
    themesActionGroup = new QActionGroup(this);  // themes: light, dark, etc.
    themesActionGroup->setExclusive(true);
    connect(themesActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(themeTriggered(QAction*)));

    themesActionGroup->addAction(ui->actionLight);
    themesActionGroup->addAction(ui->actionDark);
#endif

    t.elapsed(__LINE__);

    // let's look through the items in the SD menu (this method is less fragile now)
    QStringList ag1;
    ag1 << "Normal" << "Color only" << "Mental image" << "Sight" << "Random" << "Tam" << "Random Color only"; // OK to have one be prefix of another

    QStringList ag2;
    ag2 << "Sequence Designer" << "Dance Arranger";

    // ITERATE THRU MENU SETTING UP EXCLUSIVITY -------
    QString submenuName;
    foreach (QAction *action, ui->menuSequence->actions()) {
        if (action->isSeparator()) {  // top level separator
//            qDebug() << "separator";
        } else if (action->menu()) {  // top level menu
//            qDebug() << "item with submenu: " << action->text();
            submenuName = action->text();  // remember which submenu we're in
            // iterating just one level down
            foreach (QAction *action2, action->menu()->actions()) {
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

    t.elapsed(__LINE__);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    // ui->songTable->verticalScrollBar()->setSingleStep(10);
#endif

    t.elapsed(__LINE__);

    lastCuesheetSavePath = prefsManager.MySettings.value("lastCuesheetSavePath").toString();

    t.elapsed(__LINE__);

    initialize_internal_sd_tab();

    t.elapsed(__LINE__);

    if (prefsManager.GetenableAutoAirplaneMode()) {
        airplaneMode(true);
    }

    t.elapsed(__LINE__);

    connect(QApplication::instance(), SIGNAL(applicationStateChanged(Qt::ApplicationState)),
            this, SLOT(changeApplicationState(Qt::ApplicationState)));
    connect(QApplication::instance(), SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(focusChanged(QWidget*,QWidget*)));

    t.elapsed(__LINE__);

#ifndef DEBUG_LIGHT_MODE
    ui->pushButtonCueSheetEditTitle->setStyleSheet("font-weight: bold;");

    ui->pushButtonCueSheetEditBold->setStyleSheet("font-weight: bold;");
    ui->pushButtonCueSheetEditItalic->setStyleSheet("font: italic;");

    ui->pushButtonCueSheetEditHeader->setStyleSheet("color: red");
#endif

    t.elapsed(__LINE__);

#ifndef DEBUG_LIGHT_MODE
    if (!darkmode) {
        ui->pushButtonCueSheetEditArtist->setStyleSheet("color: #0000FF");
    } else {
        ui->pushButtonCueSheetEditArtist->setStyleSheet("color: #26A4ED"); // QColor("#26A4ED")
    }

    t.elapsed(__LINE__);

    ui->pushButtonCueSheetEditLabel->setStyleSheet("color: #60C060");

    ui->pushButtonCueSheetEditLyrics->setStyleSheet(
                "QPushButton {background-color: #FFC0CB; color: #000000; border-radius:4px; padding:1px 8px; border:0.5px solid #CF9090;}"
                "QPushButton:checked { background-color: qlineargradient(x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 #1E72FE, stop: 1 #3E8AFC); color: #FFFFFF; border:0.5px solid #0D60E3;}"
                "QPushButton:pressed { background-color: qlineargradient(x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 #1E72FE, stop: 1 #3E8AFC); color: #FFFFFF; border:0.5px solid #0D60E3;}"
                "QPushButton:disabled { background-color: #FFC0CB; color: #000000; border-radius:4px; padding:1px 8px; border:0.5px solid #CF9090;}"
                );
#endif

    t.elapsed(__LINE__);

    maybeLoadCSSfileIntoTextBrowser(true);

    t.elapsed(__LINE__);

    initReftab();

    t.elapsed(__LINE__);

    currentSDVUILevel      = "Plus"; // one of sd's names: {basic, mainstream, plus, a1, a2, c1, c2, c3a} // DEPRECATED
    currentSDKeyboardLevel = "UNK"; // one of sd's names: {basic, mainstream, plus, a1, a2, c1, c2, c3a}
    //    ui->tabWidget_2->setTabText(1, QString("Current Sequence: ") + currentSDKeyboardLevel); // Current {Level} Sequence
//    ui->tabWidget_2->setTabText(1, QString("F4 Workshop [3/14]"));

    t.elapsed(__LINE__);

    startSDThread(get_current_sd_dance_program());

    t.elapsed(__LINE__);

    ui->textBrowserCueSheet->setFocusPolicy(Qt::NoFocus);  // lyrics editor can't get focus until unlocked

    t.elapsed(__LINE__);

    songSettings.setDefaultTagColors( prefsManager.GettagsBackgroundColorString(), prefsManager.GettagsForegroundColorString());

    // this is still needed here for the initial setup
    // setCurrentSessionIdReloadSongAgesCheckMenu(
    //     static_cast<SessionDefaultType>(prefsManager.GetSessionDefault() == SessionDefaultDOW)
    //     ? songSettings.currentSessionIDByTime() : 1); // on app entry, ages must show current session, but 1 is wrong

    // what session are we in to start with?
    SessionDefaultType sessionDefault =
        static_cast<SessionDefaultType>(prefsManager.GetSessionDefault()); // preference setting

    if (sessionDefault == SessionDefaultDOW) {
        t.elapsed(__LINE__);

        int currentSessionID = songSettings.currentSessionIDByTime(); // what session are we in?
        setCurrentSessionId(currentSessionID); // save it in songSettings
        t.elapsed(__LINE__);

        populateMenuSessionOptions(); // update the sessions menu with whatever is checked now
        t.elapsed(__LINE__);

        // splash->setProgress(25, "Looking up when songs were last played...");
        reloadSongAges(ui->actionShow_All_Ages->isChecked());

        t.elapsed(__LINE__);

        lastSessionID = currentSessionID;
        currentSongSecondsPlayed = 0; // reset the counter, because this is a new session
        currentSongSecondsPlayedRecorded = false; // not reported yet, because this is a new session
    } else {
        // do this once at startup, and never again.  I think that was the intent of this mode.
        int practiceID = 1; // wrong, if there are deleted rows in Sessions table
        QList<SessionInfo> sessions = songSettings.getSessionInfo();
        t.elapsed(__LINE__);

        foreach (const SessionInfo &s, sessions) {
            // qDebug() << s.day_of_week << s.id << s.name << s.order_number << s.start_minutes;
            if (s.order_number == 0) { // 0 is the first non-deleted row where order_number == 0
                // qDebug() << "Found it: " << s.name << "row:" << s.id;
                practiceID = s.id; // now it's right!
            }
        }
        t.elapsed(__LINE__);

        setCurrentSessionId(practiceID); // save it in songSettings
        t.elapsed(__LINE__);

        reloadSongAges(ui->actionShow_All_Ages->isChecked());
        t.elapsed(__LINE__);

        populateMenuSessionOptions(); // update the sessions menu with whatever is checked now
        lastSessionID = practiceID;
        currentSongSecondsPlayed = 0; // reset the counter, because this is a new session
        currentSongSecondsPlayedRecorded = false; // not reported yet, because this is a new session
        // qDebug() << "***** We are now in Practice Session, id =" << practiceID;
    }

    t.elapsed(__LINE__);

    // qDebug() << "Constructor sets initial session to:" << songSettings.getCurrentSession();
    populateMenuSessionOptions();  // update the Session menu

    t.elapsed(__LINE__);

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

    t.elapsed(__LINE__);

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

    t.elapsed(__LINE__);

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

    t.elapsed(__LINE__);

#ifdef DEBUG_LIGHT_MODE
    QString themePreference = prefsManager.GetactiveTheme();
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

    t.elapsed(__LINE__);

#endif

    QString snapSetting = prefsManager.Getsnap();
    if (snapSetting == "disabled") {
        ui->actionDisabled->setChecked(true);
    } else if (snapSetting == "beat") {
        ui->actionNearest_Beat->setChecked(true);
    } else {
        // "measure"
        ui->actionNearest_Measure->setChecked(true);
    }

    lastSongTableRowSelected = -1;  // meaning "no selection"

    t.elapsed(__LINE__);

    lockForEditing();

    cBass->SetIntelBoostEnabled(prefsManager.GetintelBoostIsEnabled());
    cBass->SetIntelBoost(FREQ_KHZ, static_cast<float>(prefsManager.GetintelCenterFreq_KHz()/10.0)); // yes, we have to initialize these manually
    cBass->SetIntelBoost(BW_OCT,  static_cast<float>(prefsManager.GetintelWidth_oct()/10.0));
    cBass->SetIntelBoost(GAIN_DB, static_cast<float>(prefsManager.GetintelGain_dB()/10.0));  // expressed as positive number

    cBass->SetPanEQVolumeCompensation(static_cast<float>(prefsManager.GetpanEQGain_dB()/2.0)); // expressed as signed half-dB's

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

    sdSliderSidesAreSwapped = false;  // start out NOT swapped
    if (prefsManager.GetSwapSDTabInputAndAvailableCallsSides())
    {
        // if we SHOULD be swapped, swap them...
        sdSliderSidesAreSwapped = true;
        ui->splitterSDTabHorizontal->addWidget(ui->splitterSDTabHorizontal->widget(0));
    }

    t.elapsed(__LINE__);

    connect(ui->textBrowserCueSheet, SIGNAL(copyAvailable(bool)),
            this, SLOT(LyricsCopyAvailable(bool)));

    // if (!darkmode) {  // OLD LIGHT MODE
    //     ui->actionSave_Playlist_2->setEnabled(false); // Playlist > Save Playlist...
    //     ui->actionSave_Playlist->setEnabled(false); // Playlist > Save Playlist As...
    //     ui->actionPrint_Playlist->setEnabled(false);  // Playlist > Print Playlist...

    //     // Finally, if there was a playlist loaded the last time we ran SquareDesk, load it again
    //     QString loadThisPlaylist = prefsManager.GetlastPlaylistLoaded(); // "" if no playlist was loaded

    // //    qDebug() << "CONSTRUCTOR: " << loadThisPlaylist;

    //     if (loadThisPlaylist != "") {

    // //        QString fullPlaylistPath = musicRootPath + "/playlists/" + loadThisPlaylist + ".csv";
    //         if (!loadThisPlaylist.startsWith("playlists/")) {
    //             loadThisPlaylist = QString("playlists/") + loadThisPlaylist; // for compatibility with older versions of SquareDesk
    //         }

    //         QString fullPlaylistPath = musicRootPath + "/" + loadThisPlaylist + ".csv";
    // //        qDebug() << "CONSTRUCTOR: " << fullPlaylistPath;
    //         finishLoadingPlaylist(fullPlaylistPath); // load it! (and enabled Save and Save As and Print
    //     }
    // }

//    stopLongSongTableOperation("MainWindow");

//    qDebug() << "selected: " << ui->songTable->selectedItems().first()->row();

    // on_songTable_itemSelectionChanged(); // call this to update the colors


    on_actionSD_Output_triggered(); // initialize visibility of SD Output tab in SD tab
    on_actionShow_Frames_triggered(); // show or hide frames
//    ui->actionSequence_Designer->setChecked(true);
    ui->actionDance_Arranger->setChecked(true);  // this sets the default view

    t.elapsed(__LINE__);

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

    t.elapsed(__LINE__);

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
    sdActionGroupDances = new QActionGroup(this);  // Dances are mutually exclusive
    sdActionGroupDances->setExclusive(true);
    connect(sdActionGroupDances, SIGNAL(triggered(QAction*)), this, SLOT(sdActionTriggeredDances(QAction*)));

    t.elapsed(__LINE__);

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

    t.elapsed(__LINE__);

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    std::sort(dances.begin(), dances.end(), collator); // sort by natural alphanumeric order

//    qDebug() << "allDances, dances: " << allDances << dances;

    QMenu *dancesSubmenu = new QMenu("Load Dance");
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

    t.elapsed(__LINE__);

    ui->menuSequence->insertMenu(ui->actionDance, dancesSubmenu);
    ui->actionDance->setVisible(false);  // TODO: This is a kludge, because it's just a placeholder, so I could stick the Dances item at the top
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

    t.elapsed(__LINE__);

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

    foreach(QAction *action, submenuCopy->actions()){ // enable shortcuts for all Copy actions
        action->setShortcutVisibleInContextMenu(true);
    }
    foreach(QAction *action, submenuMove->actions()){ // enable shortcuts for all Move actions
        action->setShortcutVisibleInContextMenu(true);
    }

    ui->pushButtonSDMove->setMenu(saveSDMenu);
    ui->pushButtonSDMove->setVisible(false);

    SDExitEditMode(); // make sure buttons are visible/invisible

    selectFirstItemOnLoad = true; // TEST

    getMetadata();

    t.elapsed(__LINE__);

//    debugCSDSfile("plus"); // DEBUG DEBUG DEBUG THIS HELPS TO DEBUG IMPORT OF CSDS SEQUENCES TO SD FORMAT *********

    newSequenceInProgress = editSequenceInProgress = false; // no sequence being edited right now.
    on_actionFormation_Thumbnails_triggered(); // make sure that the thumbnails are turned OFF, if Formation Thumbnails is not initially checked

    if (ui->darkSongTable->rowCount() >= 1) {
        // ui->darkSongTable->selectRow(0); // select row 1 after initial load of the songTable (if there are rows)
        ui->darkSearch->setFocus();
    }

    // minVolume feature now deprecated!
    // minimumVolume = prefsManager.GetlimitVolume(); // initialize the limiting of the volume control

// DARKMODE INIT ===============

    // CHANGE TAB ORDERING (now different from that in QtCreator) =======
    // ui->tabWidget->tabBar()->moveTab(5,0);
    // ui->tabWidget->setTabText(0, DARKMUSICTABNAME);

    // if (darkmode) {
    //     ui->tabWidget->setTabVisible(1, false); // hide the MUSIC tab, leaving the DarkMusic tab visible
    //     ui->tabWidget->setCurrentIndex(0);
    // }

    t.elapsed(__LINE__);

    // CLOCK COLORING =============
    ui->theSVGClock->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->theSVGClock, SIGNAL(customContextMenuRequested(QPoint)), ui->theSVGClock, SLOT(customMenuRequested(QPoint)));

    // ============= DARKMODE INIT ================
    // QString darkTextColor = "#C0C0C0";
    if (darkmode) {
        darkTextColor = "#C0C0C0";
    }
    
    // DARK MODE UI TESTING --------------------

    ui->darkWarningLabel->setToolTip("Shows Time-in-Tip (Patter) in MM:SS, and Section-in-Tip (Singer).");
    ui->currentLocLabel3->setToolTip("Shows Position-in-Song in MM:SS.");
    ui->songLengthLabel2->setToolTip("Shows Length-of-Song in MM:SS.");

    t.elapsed(__LINE__);

    // LOOP CONTROLS =========
#ifndef DEBUG_LIGHT_MODE
    ui->darkStartLoopTime->setStyleSheet("color: " + darkTextColor);
    ui->darkEndLoopTime->setStyleSheet("color: " + darkTextColor);
    ui->darkStartLoopButton->setStyleSheet("color: " + darkTextColor);
    ui->darkEndLoopButton->setStyleSheet("color: " + darkTextColor);
    ui->darkTestLoopButton->setStyleSheet("color: " + darkTextColor);
    ui->darkSegmentButton->setStyleSheet("color: " + darkTextColor);
    ui->currentLocLabel3->setStyleSheet("color: " + darkTextColor);
    ui->songLengthLabel2->setStyleSheet("color: " + darkTextColor);
#endif
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

    t.elapsed(__LINE__);

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

    t.elapsed(__LINE__);

    // SONGTABLE:
    ui->darkSongTable->setAutoScroll(true); // Ensure that selection is always visible

    QHeaderView *verticalHeader = ui->darkSongTable->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::ResizeToContents);

    t.elapsed(__LINE__);

    ui->darkSongTable->setAlternatingRowColors(true);

    t.elapsed(__LINE__);

#ifndef DEBUG_LIGHT_MODE
    ui->darkSongTable->setStyleSheet("::section { background-color: #393939; color: #A0A0A0; }");
#endif

    t.elapsed(__LINE__);

    splash->setProgress(45, "Adjusting the column layout...");

    ui->darkSongTable->resizeColumnToContents(kNumberCol);  // and force resizing of column widths to match songs
    ui->darkSongTable->resizeColumnToContents(kTypeCol);
    ui->darkSongTable->resizeColumnToContents(kLabelCol);
    ui->darkSongTable->resizeColumnToContents(kPitchCol);
    ui->darkSongTable->resizeColumnToContents(kTempoCol);

    t.elapsed(__LINE__);

    ui->darkSongTable->setMainWindow(this);

    t.elapsed(__LINE__);

    // PLAYLISTS:
#ifndef DEBUG_LIGHT_MODE
    ui->playlist1Label->setStyleSheet("font-size: 11pt; background-color: #404040; color: #AAAAAA;");
#endif
    ui->playlist1Label->setText("<img src=\":/graphics/icons8-menu-64.png\" width=\"10\" height=\"9\">Jokers_2023.09.20");
    ui->playlist1Label->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->playlist1Label, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customPlaylistMenuRequested(QPoint)));

    ui->playlist1Table->setMainWindow(this);
    ui->playlist1Table->resizeColumnToContents(0); // number
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
                        QMenu *plMenu = new QMenu();

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
                            }
                        }

                        plMenu->popup(QCursor::pos());
                        plMenu->exec();
                        delete plMenu; // done with it
                  }
            );

    // -----
    t.elapsed(__LINE__);

#ifndef DEBUG_LIGHT_MODE
    ui->playlist2Label->setStyleSheet("font-size: 11pt; background-color: #404040; color: #AAAAAA;");
#endif
    ui->playlist2Label->setText("<img src=\":/graphics/icons8-menu-64.png\" width=\"10\" height=\"9\">Jokers_2023.09.20");
    ui->playlist2Label->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->playlist2Label, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customPlaylistMenuRequested(QPoint)));

    ui->playlist2Table->setMainWindow(this);
    ui->playlist2Table->resizeColumnToContents(0); // number
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

                        QMenu *plMenu = new QMenu();
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
                            }
                        }

                        plMenu->popup(QCursor::pos());
                        plMenu->exec();
                        delete plMenu; // done with it
                  }
            );

    // -----
    t.elapsed(__LINE__);

    ui->playlist3Table->setMainWindow(this);
#ifndef DEBUG_LIGHT_MODE
    ui->playlist3Label->setStyleSheet("font-size: 11pt; background-color: #404040; color: #AAAAAA;");
#endif
    ui->playlist3Label->setText("<img src=\":/graphics/icons8-menu-64.png\" width=\"10\" height=\"9\">Jokers_2023.09.20");
    ui->playlist3Label->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->playlist3Label, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customPlaylistMenuRequested(QPoint)));

    ui->playlist3Table->resizeColumnToContents(0); // number
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

                        QMenu *plMenu = new QMenu();
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
                            }
                        }

                        plMenu->popup(QCursor::pos());
                        plMenu->exec();
                        delete plMenu; // done with it
                  }
            );

    // VUMETER:
    ui->darkVUmeter->levelChanged(0, 0, false);  // initialize the VUmeter

    // SONGTABLE:
//    ui->darkSongTable->selectRow(2);

    // TREEWIDGET:
    QList<QTreeWidgetItem *> trackItem = ui->treeWidget->findItems("Tracks", Qt::MatchExactly);
    doNotCallDarkLoadMusicList = true;
    trackItem[0]->setSelected(true); // entire Tracks was already loaded by actionTags
    doNotCallDarkLoadMusicList = false;

    // enable context menus for TreeWidget
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customTreeWidgetMenuRequested(QPoint)));


    t.elapsed(__LINE__);

    ui->toggleShowPaletteTables->setVisible(false);
    ui->splitterMusicTabHorizontal->setCollapsible(1, false); // TEST TEST TEST

    t.elapsed(__LINE__);


#ifndef DEBUG_LIGHT_MODE
    ui->statusBar->setStyleSheet("color: " + darkTextColor);
#endif

    // MICLABEL (right hand status):
//    micStatusLabel->setStyleSheet("color: #AC8F7E");
#ifndef DEBUG_LIGHT_MODE
    micStatusLabel->setStyleSheet("color: " + darkTextColor);
#endif

    // SEARCH BOX:
    ui->darkSearch->setToolTip("Search\nFilter songs by specifying Type:Label:Title.\n\nExamples:\nlove = any song where type or label or title contains 'love'\nsing::heart = singing calls where title contains 'heart'\np:riv = patter from Riverboat\netc.");

    // TITLE:
    // QLabel { color : #C2AC9E; }
#ifndef DEBUG_LIGHT_MODE
    ui->darkTitle->setStyleSheet("color: "+ darkTextColor); // default color
#else
    setProp(ui->darkTitle, "flashcall", false);
#endif
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

    t.elapsed(__LINE__);

// Note: I don't do it this way below, because changing color then requires editing the resource files.
//  Instead, the above method allows me to change colors at any time by regenerating the cached icons from pixmaps.
//    QPixmap pixmap2(":/graphics/darkPlay.png");
//    QIcon darkPlayPixmap2(pixmap2);

//    QIcon dd(":/graphics/darkPlay.png"); // I can't stick this in the MainWindow object without getting errors either...

    waveform = new float[WAVEFORMSAMPLES];

    clearSlot(0);
    clearSlot(1);
    clearSlot(2);

    t.elapsed(__LINE__);

    if (darkmode) {
        // light mode already loaded the playlist above

        // PLAYLIST LOADING HERE, SO THAT PLAYLIST1 IS INITIALIZED BY NOW
        // ui->actionSave_Playlist_2->setEnabled(false); // Playlist > Save Playlist...
        // ui->actionSave_Playlist->setEnabled(false); // Playlist > Save Playlist As...
        // ui->actionPrint_Playlist->setEnabled(false);  // Playlist > Print Playlist...

        reloadPaletteSlots();  // reload all the palette slots, based on the last time we ran SquareDesk

        for (int i = 0; i < 3; i++) {
            slotModified[i] = false;
        }
    }

    t.elapsed(__LINE__);

    if (darkmode) {
        ui->seekBarCuesheet->setFusionMode(true); // allow click-to-move-there
// #define NOPLAYLISTMENU
// #ifdef NOPLAYLISTMENU
//         ui->menuPlaylist->menuAction()->setVisible(false); // Menu Bar > Playlist not relevant in Dark Mode
// #endif
        updateSongTableColumnView();
    }

    auditionInProgress = false;
    auditionSingleShotTimer.setSingleShot(true);
    connect(&auditionSingleShotTimer, &QTimer::timeout, this,
            [this]() {
        // if the timer ever times out, it will set auditionInProgress to false;
        // qDebug() << "setting auditionInProgress to false";
        auditionInProgress = false;
    });

    stopLongSongTableOperation("MainWindow");

#ifdef TESTRESTARTINGSQUAREDESK
    if (testRestartingSquareDesk) {
        qDebug() << "test restarting of SquareDesk in 5 seconds...CMD-Q to stop this.";
        testRestartingSquareDeskTimer.setSingleShot(true);
        testRestartingSquareDeskTimer.start(5000);  // 5 seconds
        connect(&testRestartingSquareDeskTimer, &QTimer::timeout, this,
                [this]() {
                qDebug() << "TOGGLING...";
                on_actionSwitch_to_Light_Mode_triggered();  // this will toggle us indefinitely, until manually stopped
        });
    }

    QMenu templateMenu;
#endif

    // set up Template menu ---------
    templateMenu = new QMenu();

    // find all template HTML files, and add to the menu
    QString templatesDir = musicRootPath + "/lyrics/templates";
    QString templatePattern = "*template*html";
    QDir currentDir(templatesDir);
    // const QString prefix = templatesDir + "/";
    foreach (const QString &match, currentDir.entryList(QStringList(templatePattern), QDir::Files | QDir::NoSymLinks)) {
        // qDebug() << "FOUND ONE: " << match;
        QString s = match;
        s.replace(".html", "");
        QAction *a = templateMenu->addAction(s, this, [this] { newFromTemplate(); });
        a->setProperty("name", s); // e.g. "lyrics.template"
    }

    ui->pushButtonNewFromTemplate->setMenu(templateMenu);

    ui->pushButtonEditLyrics->setVisible(false); // don't show this button until there are cuesheets available to edit
    ui->pushButtonNewFromTemplate->setVisible(false); // don't show this button until there is a song loaded

#ifdef DEBUG_LIGHT_MODE
    // THEME STUFF -------------
    t.elapsed(__LINE__);

    svgClockStateChanged("UNINITIALIZED");

    // initial stylesheet loaded here -----------
    t.elapsed(__LINE__);
    QString modeString = QString("Setting up %1 Theme...").arg(themePreference);
    splash->setProgress(65, modeString); // Setting up {Light, Dark} theme...

    themesFileModified(); // load the Themes.qss file for the first time

    // // and set the dynamic properties for LIGHT mode to start with
    // themeTriggered(ui->actionLight); // start out with the mandatory "Light" theme (do NOT delete "Light")

    t.elapsed(__LINE__);

    ui->actionSwitch_to_Light_Mode->setVisible(false);  // in NEW LIGHT/DARK MODES, don't show the old Switch to X Mode menu item
#else
    // qDebug() << "turning off menuTheme";
    ui->menuTheme->menuAction()->setVisible(false);  // in OLD LIGHT MODE, don't show the new Theme menu
#endif

    // and update the caches ---------
    t.elapsed(__LINE__);

    ui->theSVGClock->finishInit();
    ui->darkSeekBar->finishInit();  // load everything up!

    // qDebug() << "trigger!";

    t.elapsed(__LINE__);

    ui->darkTrebleKnob->finishInit();
    ui->darkMidKnob->finishInit();
    ui->darkBassKnob->finishInit();

    t.elapsed(__LINE__);

    ui->darkPitchSlider->finishInit();
    ui->darkTempoSlider->finishInit();
    ui->darkVolumeSlider->finishInit();

    t.elapsed(__LINE__);

    ui->FXbutton->setVisible(false); // if USE_JUCE is enabled, and if LoudMax AU is present, this button will be made visible
    ui->FXbutton->setChecked(false); // checked = LoudMaxWin is visible

    t.elapsed(__LINE__);

#ifdef USE_JUCE
    // JUCE ---------------
    juce::initialiseJuce_GUI(); // not sure this is needed
    scanForPlugins();
    t.elapsed(__LINE__);

#endif

    // SEARCH -----------
    typeSearch = labelSearch = titleSearch = ""; // no filters at startup
    searchAllFields = true; // search is OR, not AND

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

    // NOTE: This MUST be down here at the end of the constructor now.
    //   The startup sequence takes longer now, and we don't want the
    //   timer to start too early, or it won't affect the focus.
    //
    // startup the file watcher, AND make sure that focus has gone to the Title search field
    QTimer::singleShot(10, [this]{
//            qDebug("Starting up FileWatcher now (intentionally delayed from app startup, to avoid Box.net locks retriggering loadMusicList)");
            QObject::connect(&musicRootWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(musicRootModified(QString)));
            // if (darkmode) {
            if (!ui->darkSearch->hasFocus()) {
                               // qDebug() << "HACK: DARK SEARCH DOES NOT HAVE FOCUS. FIXING THIS.";
                ui->darkSearch->setFocus();
            }
            // } else {
            //     if (!ui->titleSearch->hasFocus()) {
            //         //                qDebug() << "HACK: TITLE SEARCH DOES NOT HAVE FOCUS. FIXING THIS.";
            //         ui->titleSearch->setFocus();
            //     }
            // }

            // also watch the abbrevs.txt file for changes, and reload the abbreviations if it changed
            abbrevsWatcher.addPath(musicRootPath + "/sd/abbrevs.txt");
            QObject::connect(&abbrevsWatcher, SIGNAL(fileChanged(QString)), this, SLOT(readAbbreviations()));

            ui->darkSongTable->selectRow(0);
            ui->darkSongTable->scrollToItem(ui->darkSongTable->item(0, kTypeCol)); // EnsureVisible row 0 (which is highlighted)
        });

    t.elapsed(__LINE__);
    splash->setProgress(90, "Almost there...");

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

    mainWindowReady = true;

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
                        QMenu *hdrMenu = new QMenu();
                        hdrMenu->setProperty("theme", currentThemeString);
                        hdrMenu->addAction(QString("Reset Sort Order"),
                                           [this]() {   // qDebug() << "Resetting sort order...";
                                                        sortByDefaultSortOrder(); } );
                        hdrMenu->popup(QCursor::pos());
                        hdrMenu->exec();
                        delete hdrMenu; // done with it

    });

    // Initialize Preview Playback Device menu
    populatePlaybackDeviceMenu();
    
    // Initialize Cuesheet menu
    setupCuesheetMenu();

    // read label names and IDs
    readLabelNames();

    // embedded HTTP server for Taminations
    startTaminationsServer();
    
    // Initialize Now Playing integration for iOS/watchOS remote control
    setupNowPlaying();
}
// END CONSTRUCTOR ---------

void MainWindow::newFromTemplate() {
    QString templateName = sender()->property("name").toString();
    // qDebug() << "newFromTemplate()" << templateName;

    // Ask me where to save it...
    RecursionGuard dialog_guard(inPreferencesDialog);
    QFileInfo fi(currentMP3filenameWithPath);

    if (lastCuesheetSavePath.isEmpty()) {
        lastCuesheetSavePath = musicRootPath + "/lyrics";
    }

    loadedCuesheetNameWithPath = lastCuesheetSavePath + "/" + fi.baseName() + ".html";

    QString maybeFilename = loadedCuesheetNameWithPath;
    QFileInfo fi2(loadedCuesheetNameWithPath);
    if (fi2.exists()) {
        // choose the next name in the series (this won't be done, if we came from a template)
        QString cuesheetExt = loadedCuesheetNameWithPath.split(".").last();
        QString cuesheetBase = loadedCuesheetNameWithPath
                .replace(QRegularExpression(cuesheetExt + "$"),"")  // remove extension, e.g. ".html"
                .replace(QRegularExpression("[0-9]+\\.$"),"");      // remove .<number>, e.g. ".2"

        // find an appropriate not-already-used filename to save to
        bool done = false;
        int which = 2;  // I suppose we could be smarter than this at some point.
        while (!done) {
            maybeFilename = cuesheetBase + QString::number(which) + "." + cuesheetExt;
            QFileInfo maybeFile(maybeFilename);
            done = !maybeFile.exists();  // keep going until a proposed filename does not exist (don't worry -- it won't spin forever)
            which++;
        }
    }

    // qDebug() << "newFromTemplate proposed cuesheet filename: " << maybeFilename;

    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save"), // TODO: this could say Lyrics or Patter
                                                    maybeFilename,
                                                    tr("HTML (*.html *.htm)"));
    if (!filename.isNull())
    {
        // this needs to be done BEFORE the actual write, because the reload will cause a bogus "are you sure" message
        // lockForEditing();

        // filewatcherShouldIgnoreOneFileSave = true;  // set flag so that Filewatcher is NOT triggered (one time)
        QString fromFilename = musicRootPath + "/lyrics/templates" + "/" + templateName + ".html";
        QString toFilename   = filename;

        // qDebug() << "newFromTemplate from/to = " << fromFilename << toFilename;

        QFile::copy(fromFilename, toFilename);

        // is it in the pathstack?
        QListIterator<QString> iter(*pathStackCuesheets); // search thru known cuesheets
        bool foundInPathStack = false;
        while (iter.hasNext()) {
            QString s = iter.next();
            // qDebug() << "looking at: " << s;
            QStringList sl1 = s.split("#!#");
            //QString type = sl1[0];  // the type (of original pathname, before following aliases)
            QString filename = sl1[1];  // everything else
            if (filename == toFilename) {
                // qDebug() << "FOUND IT IN PATHSTACK: " << toFilename;
                foundInPathStack = true;
            }

        }

        // if not already in there, stick it into the pathstackCuesheets, so it will be found at loadCuesheets time
        if (!foundInPathStack)
        {
            // qDebug() << "NOT FOUND. So, adding it to the pathStack: " << toFilename;
            QFileInfo fi(filename);
            QStringList section = fi.path().split("/");
            QString type = section[section.length()-1];  // must be the last item in the path
            // qDebug() << "    Adding " + type + "#!#" + filename + " to pathStack";
            pathStackCuesheets->append(type + "#!#" + filename);
        }

        // and reload the cuesheets, to pick up the new one from the updated pathStack
        loadCuesheets(currentMP3filenameWithPath, filename); // ignoring return value

        saveCurrentSongSettings();
    }

}

// ----------------------------------------------------------------------
// load up the palette slots from what was in those slots before...
void MainWindow::reloadPaletteSlots() {
    // Load up first slot -----
    QString loadThisPlaylist1 = prefsManager.GetlastPlaylistLoaded(); // "" if no playlist was loaded

    if (loadThisPlaylist1 != "") {
        if (loadThisPlaylist1.startsWith("tracks/")) {
            // it's a TRACK FILTER
            QString fullPlaylistPath1 = musicRootPath + "/" + loadThisPlaylist1 + ".csv";
            fullPlaylistPath1.replace("/tracks/", "/Tracks/"); // create fake file path for track filter
            int songCount;
            loadPlaylistFromFileToPaletteSlot(fullPlaylistPath1, 0, songCount);
        } else {
            // it's a PLAYLIST
            QString fullPlaylistPath1 = musicRootPath + "/" + loadThisPlaylist1 + ".csv";
            int songCount;
            loadPlaylistFromFileToPaletteSlot(fullPlaylistPath1, 0, songCount); // load it!
        }
    }

    // Load up second slot -----
    QString loadThisPlaylist2 = prefsManager.GetlastPlaylistLoaded2(); // "" if no playlist was loaded
//    qDebug() << "MainWindow constructor, load playlist and palette slot 1: " << loadThisPlaylist2;

    if (loadThisPlaylist2 != "") {
        if (loadThisPlaylist2.startsWith("tracks/")) {
            // it's a TRACK FILTER
            QString fullPlaylistPath2 = musicRootPath + "/" + loadThisPlaylist2 + ".csv";
            fullPlaylistPath2.replace("/tracks/", "/Tracks/"); // create fake file path for track filter
            int songCount;
            loadPlaylistFromFileToPaletteSlot(fullPlaylistPath2, 1, songCount);
        } else {
            // it's a PLAYLIST
            QString fullPlaylistPath2 = musicRootPath + "/" + loadThisPlaylist2 + ".csv";
            int songCount;
            loadPlaylistFromFileToPaletteSlot(fullPlaylistPath2, 1, songCount); // load it! (and enabled Save and Save As and Print) = this also calls loadPlaylistFromFileToPaletteSlot for slot 1
        }
    }

    // Load up third slot -----
    QString loadThisPlaylist3 = prefsManager.GetlastPlaylistLoaded3(); // "" if no playlist was loaded
//    qDebug() << "MainWindow constructor, load playlist and palette slot 2: " << loadThisPlaylist3;

    if (loadThisPlaylist3 != "") {
        if (loadThisPlaylist3.startsWith("tracks/")) {
            // it's a TRACK FILTER
            QString fullPlaylistPath3 = musicRootPath + "/" + loadThisPlaylist3 + ".csv";
            fullPlaylistPath3.replace("/tracks/", "/Tracks/"); // create fake file path for track filter
            int songCount;
            loadPlaylistFromFileToPaletteSlot(fullPlaylistPath3, 2, songCount);
        } else {
            // it's a PLAYLIST
            QString fullPlaylistPath3 = musicRootPath + "/" + loadThisPlaylist3 + ".csv";
            int songCount;
            loadPlaylistFromFileToPaletteSlot(fullPlaylistPath3, 2, songCount); // load it! (and enabled Save and Save As and Print) = this also calls loadPlaylistFromFileToPaletteSlot for slot 2
        }
    }
    
    // Initialize the recent playlists list if it's empty
    QString currentRecentPlaylists = prefsManager.GetlastNPlaylistsLoaded();
    if (currentRecentPlaylists.isEmpty()) {
        QStringList initialRecentPlaylists;
        
        // Add loaded playlists to the recent list (only real playlists, not tracks or Apple Music)
        if (loadThisPlaylist1 != "" && !loadThisPlaylist1.startsWith("tracks/") && !loadThisPlaylist1.startsWith("/Apple Music/")) {
            // Remove "playlists/" prefix if present for consistency
            QString relativePath = loadThisPlaylist1;
            if (relativePath.startsWith("playlists/")) {
                relativePath = relativePath.mid(10); // Remove "playlists/" prefix
            }
            initialRecentPlaylists.append(relativePath);
        }
        
        if (loadThisPlaylist2 != "" && !loadThisPlaylist2.startsWith("tracks/") && !loadThisPlaylist2.startsWith("/Apple Music/")) {
            QString relativePath = loadThisPlaylist2;
            if (relativePath.startsWith("playlists/")) {
                relativePath = relativePath.mid(10);
            }
            if (!initialRecentPlaylists.contains(relativePath)) {
                initialRecentPlaylists.append(relativePath);
            }
        }
        
        if (loadThisPlaylist3 != "" && !loadThisPlaylist3.startsWith("tracks/") && !loadThisPlaylist3.startsWith("/Apple Music/")) {
            QString relativePath = loadThisPlaylist3;
            if (relativePath.startsWith("playlists/")) {
                relativePath = relativePath.mid(10);
            }
            if (!initialRecentPlaylists.contains(relativePath)) {
                initialRecentPlaylists.append(relativePath);
            }
        }
        
        // Save the initial recent playlists list
        if (!initialRecentPlaylists.isEmpty()) {
            QString initialRecentPlaylistsString = initialRecentPlaylists.join(";");
            prefsManager.SetlastNPlaylistsLoaded(initialRecentPlaylistsString);
        }
    }
}

void MainWindow::fileWatcherTriggered() {
    // qDebug() << "fileWatcherTriggered()";
    musicRootModified(QString("DONE"));
}

void MainWindow::fileWatcherDisabledTriggered() {
    // qDebug() << "***** fileWatcherDisabledTriggered(): FileWatcher is re-enabled now....";
    filewatcherIsTemporarilyDisabled = false;   // no longer disabled when this fires!
    fileWatcherDisabledTimer->stop();  // 5s expired, so we don't need to be notified again

}

void MainWindow::musicRootModified(QString s)
{
//    qDebug() << "musicRootModified() = " << s;

    if (filewatcherIsTemporarilyDisabled) {
//        qDebug() << "filewatcher is disabled, skipping the re-scan of songTable....";
        return;
    }

    if (s != "DONE") {
//        qDebug() << "(Re)triggering File Watcher for 3 seconds...";
        fileWatcherTimer->start(std::chrono::milliseconds(500)); // wait 500ms for things to settle
        return;
    }

    fileWatcherTimer->stop();  // 500ms expired, so we don't need to be notified again.

   // qDebug() << "Music root modified (File Watcher awakened for real!): " << s;
    if (!filewatcherShouldIgnoreOneFileSave) { // yes, we need this here, too...because root watcher watches playlists (don't ask me!)
        // qDebug() << "*** File watcher awakens!!!";
        // Qt::SortOrder sortOrder(ui->songTable->horizontalHeader()->sortIndicatorOrder());
        // int sortSection(ui->songTable->horizontalHeader()->sortIndicatorSection());
        // reload the musicTable.  Note that it will switch to default sort order.
        //   TODO: At some point, this probably should save the sort order, and then restore it.
//        qDebug() << "WE ARE RELOADING THE SONG TABLE NOW ------";
        findMusic(musicRootPath, true);  // get the filenames from the user's directories
        // loadMusicList(); // and filter them into the songTable
        // if (darkmode) {
        darkLoadMusicList(nullptr, currentTypeFilter, true, true); // also filter them into the darkSongTable
        darkFilterMusic();   // and redo the filtering (NOTE: might still scroll the darkSongTable)
        // ui->darkSongTable->horizontalHeader()->setSortIndicator(sortSection, sortOrder);

        // update the status message ------
        QString msg1 = QString("Songs found: %1")
                .arg(QString::number(pathStack->size()));
        ui->statusBar->showMessage(msg1);
    }
    filewatcherShouldIgnoreOneFileSave = false;
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
    Q_UNUSED(old1)
    Q_UNUSED(new1)
}

void MainWindow::setCurrentSessionId(int id)
{
    songSettings.setCurrentSession(id);
}

QString MainWindow::ageToIntString(QString ageString) {
    if (ageString == "") {
        return(QString(""));
    }
    int ageAsInt = static_cast<int>(ageString.toFloat());
    QString ageAsIntString(QString("%1").arg(ageAsInt, 3).trimmed());
//    qDebug() << "ageString: " << ageString << ", ageAsInt: " << ageAsInt << ", ageAsIntString: " << ageAsIntString;
    return(ageAsIntString);
}

QString MainWindow::ageToRecent(QString ageInDaysFloatString) {
    QDateTime now = QDateTime::currentDateTimeUtc();

    QString recentString = "";
    if (ageInDaysFloatString != "") {
        qint64 ageInSecs = static_cast<qint64>(60.0*60.0*24.0*ageInDaysFloatString.toDouble());
        bool newerThanFence = now.addSecs(-ageInSecs) > recentFenceDateTime;
        if (newerThanFence) {
//            qDebug() << "recent fence: " << recentFenceDateTime << ", now: " << now << ", ageInSecs: " << ageInSecs;
//            recentString = "🔺";  // this is a nice compromise between clearly visible and too annoying
            recentString = "*";  // this is a nice compromise between clearly visible and too annoying
        } // else it will be ""
    }
    return(recentString);
}

// SONGTABLEREFACTOR
void MainWindow::reloadSongAges(bool show_all_ages)  // also reloads Recent columns entries
{
    // qDebug() << "================ reloadSongAges" << show_all_ages << songSettings.getCurrentSession();
    PerfTimer t("reloadSongAges", __LINE__);
    QHash<QString,QString> ages;
    songSettings.getSongAges(ages, show_all_ages);

    // qDebug() << "***** ages *****";
    // for (auto [key, value] : ages.asKeyValueRange()) {
    //     if (key.contains("Ireland")) {
    //         qDebug() << qPrintable(key) << ": " << value;
    //     }
    // }

    MyTableWidget *thisTable;

    // if (darkmode) {
    ui->darkSongTable->setSortingEnabled(false);
    ui->darkSongTable->hide();
    thisTable = ui->darkSongTable;
    // } else {
    //     ui->songTable->setSortingEnabled(false);
    //     ui->songTable->hide();
    //     thisTable = ui->songTable;
    // }

    for (int i = 0; i < thisTable->rowCount(); i++) {
        QString origPath = thisTable->item(i,kPathCol)->data(Qt::UserRole).toString();
        QString path = songSettings.removeRootDirs(origPath);
        QHash<QString,QString>::const_iterator age = ages.constFind(path);

        QString theAgeString    = (age == ages.constEnd() ? "" : ageToIntString(age.value()));
        QString theRecentString = (age == ages.constEnd() ? "" : ageToRecent(age.value()));

        ui->darkSongTable->item(i,kAgeCol)->setText(theAgeString);
        ui->darkSongTable->item(i,kAgeCol)->setTextAlignment(Qt::AlignCenter);

        ui->darkSongTable->item(i,kRecentCol)->setText(theRecentString);
        ui->darkSongTable->item(i,kRecentCol)->setTextAlignment(Qt::AlignCenter);

        // ((darkSongTitleLabel *)(ui->darkSongTable->cellWidget(i, kTitleCol)))->setSongUsed(theRecentString != ""); // rewrite the song's title to be strikethrough and/or green background
        QString thePath = ui->darkSongTable->item(i, kPathCol)->data(Qt::UserRole).toString();
        if (theRecentString != "") {
            pathsOfCalledSongs.insert(thePath);
        } else {
            pathsOfCalledSongs.remove(thePath);
        }
        // } else {
        //     ui->songTable->item(i,kAgeCol)->setText(age == ages.constEnd() ? "" : ageToIntString(age.value()));
        //     ui->songTable->item(i,kAgeCol)->setTextAlignment(Qt::AlignCenter);

        //     ui->songTable->item(i,kRecentCol)->setText(age == ages.constEnd() ? "" : ageToRecent(age.value()));
        //     ui->songTable->item(i,kRecentCol)->setTextAlignment(Qt::AlignCenter);
        // }
    }

    ui->darkSongTable->show();
    ui->darkSongTable->setSortingEnabled(true);

    // now that we know what's strikethrough and what's not, update the palette slots, too
    // TODO: THIS IS DUPLICATED CODE, FACTOR IT OUT
    // qDebug() << "TIMERTICK pathsOfCalledSongs:" << pathsOfCalledSongs;
    // for (int r = 0; r < ui->playlist1Table->rowCount(); r++) {
    //     if (ui->playlist1Table->item(r,4) != nullptr) {
    //         QString pathToMP3 = ui->playlist1Table->item(r,4)->text();
    //         // qDebug() << "TIMERTICK playlist1Table thePath:" << pathToMP3;
    //         ((darkPaletteSongTitleLabel *)(ui->playlist1Table->cellWidget(r,1)))->setSongUsed(pathsOfCalledSongs.contains(pathToMP3));
    //     }
    // }
    // for (int r = 0; r < ui->playlist2Table->rowCount(); r++) {
    //     if (ui->playlist2Table->item(r,4) != nullptr) {
    //         QString pathToMP3 = ui->playlist2Table->item(r,4)->text();
    //         // qDebug() << "TIMERTICK playlist2Table thePath:" << pathToMP3;
    //         ((darkPaletteSongTitleLabel *)(ui->playlist2Table->cellWidget(r,1)))->setSongUsed(pathsOfCalledSongs.contains(pathToMP3));
    //     }
    // }
    // for (int r = 0; r < ui->playlist3Table->rowCount(); r++) {
    //     if (ui->playlist3Table->item(r,4) != nullptr) {
    //         QString pathToMP3 = ui->playlist3Table->item(r,4)->text();
    //         // qDebug() << "TIMERTICK playlist3Table thePath:" << pathToMP3;
    //         ((darkPaletteSongTitleLabel *)(ui->playlist3Table->cellWidget(r,1)))->setSongUsed(pathsOfCalledSongs.contains(pathToMP3));
    //     }
    // }
}

void MainWindow::setCurrentSessionIdReloadSongAges(int id)
{
    setCurrentSessionId(id);
    lastSessionID = id;
    // qDebug() << "***** manual change of session id to:" << id;
    reloadSongAges(ui->actionShow_All_Ages->isChecked());
    on_comboBoxCallListProgram_currentIndexChanged(ui->comboBoxCallListProgram->currentIndex()); // this tab is gone now
}

void MainWindow::setCurrentSessionIdReloadSongAgesCheckMenu(int id)
{
    setCurrentSessionIdReloadSongAges(id);
}

static CallListCheckBox * AddItemToCallList(QTableWidget *tableWidget,
                                            const QString &number, const QString &name,
                                            const QString &taughtOn,
                                            const QString &timing)
{
    int initialRowCount = tableWidget->rowCount();
    tableWidget->setRowCount(initialRowCount + 1);
    int row = initialRowCount;


    QTableWidgetItem *numberItem = new QTableWidgetItem(number);
    numberItem->setTextAlignment(Qt::AlignCenter);  // center the #'s in the Teach column
    QTableWidgetItem *nameItem = new QTableWidgetItem(name);
    QTableWidgetItem *timingItem = new TableWidgetTimingItem(timing);

    numberItem->setFlags(numberItem->flags() & ~Qt::ItemIsEditable);
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
    timingItem->setFlags(timingItem->flags() & ~Qt::ItemIsEditable);
    timingItem->setTextAlignment(Qt::AlignCenter);  // center the dates

    tableWidget->setItem(row, kCallListOrderCol, numberItem);
    tableWidget->setItem(row, kCallListNameCol, nameItem);
    tableWidget->setItem(row, kCallListTimingCol, timingItem);

    QTableWidgetItem *dateItem = new QTableWidgetItem(taughtOn);
    dateItem->setFlags(dateItem->flags() | Qt::ItemIsEditable);
    dateItem->setTextAlignment(Qt::AlignCenter);  // center the dates
    tableWidget->setItem(row, kCallListWhenCheckedCol, dateItem);

    CallListCheckBox *checkbox = new CallListCheckBox();
    //   checkbox->setStyleSheet("margin-left:50%; margin-right:50%;");
    QHBoxLayout *layout = new QHBoxLayout();
    QWidget *widget = new QWidget();
    layout->setAlignment( Qt::AlignCenter );
    layout->addWidget( checkbox );
    layout->setContentsMargins(0,0,0,0);
    widget->setLayout( layout);
    checkbox->setCheckState((taughtOn.isNull() || taughtOn.isEmpty()) ? Qt::Unchecked : Qt::Checked);
    tableWidget->setCellWidget(row, kCallListCheckedCol, widget );;
    checkbox->setRow(row);
    return checkbox;
}

static QString findTimingForCall(QString danceProgram, const QString &call)
{
    int score = 0;
    QString timing;
    if (0 == danceProgram.compare("basic1", Qt::CaseInsensitive))
        danceProgram = "b1";
    if (0 == danceProgram.compare("basic2", Qt::CaseInsensitive))
        danceProgram = "b2";
    if (0 == danceProgram.compare("mainstream", Qt::CaseInsensitive))
        danceProgram = "ms";
        
    for (int i = 0; danceprogram_callinfo[i].name; ++i)
    {
        if ((0 == danceProgram.compare(danceprogram_callinfo[i].program, Qt::CaseInsensitive))
            && (0 == call.compare(danceprogram_callinfo[i].name, Qt::CaseInsensitive)))
        {
            if (danceprogram_callinfo[i].timing)
            {
                timing = danceprogram_callinfo[i].timing;
                break;
            }
        }
        if (call.startsWith(danceprogram_callinfo[i].name, Qt::CaseInsensitive))
        {
            int thisScore = 100 - abs(call.length() - QString(danceprogram_callinfo[i].name).length());
            if (0 == call.compare(danceprogram_callinfo[i].name, Qt::CaseInsensitive))
                thisScore += 100;
            if (thisScore > score)
            {
                if (danceprogram_callinfo[i].timing)
                {
                    timing = danceprogram_callinfo[i].timing;
                    score = thisScore;
                }
            }
        }
    } /* end of iterating through call info */
    return timing;
}

void MainWindow::loadCallList(SongSettings &songSettings, QTableWidget *tableWidget, const QString &danceProgram, const QString &filename)
{
    static QRegularExpression regex_numberCommaName(QRegularExpression("^((\\s*\\d+)(\\.\\w+)?)\\,?\\s+(.*)$"));
//    qDebug() << "loadCallList: " << danceProgram << filename;
    tableWidget->setRowCount(0);
    callListOriginalOrder.clear();

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
            CallListCheckBox *checkbox = AddItemToCallList(tableWidget, number, name, taughtOn, findTimingForCall(danceProgram, name));
            callListOriginalOrder.append(name);
            checkbox->setMainWindow(this);

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

// =============================================================================
void MainWindow::tableWidgetCallList_checkboxStateChanged(int clickRow, int state)
{
    int currentIndex = ui->comboBoxCallListProgram->currentIndex();
    QString programFilename(ui->comboBoxCallListProgram->itemData(currentIndex).toString());
    QString displayName;
    QString danceProgram;
    breakDanceProgramIntoParts(programFilename, displayName, danceProgram);

    QString callName = callListOriginalOrder[clickRow];
    int row = -1;

    for (row = 0; row < ui->tableWidgetCallList->rowCount(); ++row)
    {
        QTableWidgetItem *theItem = ui->tableWidgetCallList->item(row, kCallListNameCol);
        QString thisCallName = theItem->text();
        if (thisCallName == callName)
            break;
    }
    if (row < 0 || row >= ui->tableWidgetCallList->rowCount())
    {
//        qDebug() << "call list row original call name not found, clickRow " << clickRow << " name " << callName << " row " << row;
        return;
    }
    
    if (state == Qt::Checked)
    {
        songSettings.setCallTaught(danceProgram, callName);
    }
    else
    {
        songSettings.deleteCallTaught(danceProgram, callName);
    }

    QTableWidgetItem *dateItem(new QTableWidgetItem(songSettings.getCallTaughtOn(danceProgram, callName)));
    dateItem->setTextAlignment(Qt::AlignCenter);
    ui->tableWidgetCallList->setItem(row, kCallListWhenCheckedCol, dateItem);
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
        prefsManager.MySettings.setValue("lastCallListDanceProgram",program);
    }
    ui->tableWidgetCallList->setSortingEnabled(true);
}


void MainWindow::on_comboBoxCuesheetSelector_currentIndexChanged(int currentIndex)
{
//    qDebug() << "on_comboBoxCuesheetSelector_currentIndexChanged currentIndex = " << currentIndex;
    if (currentIndex != -1 && !cuesheetEditorReactingToCursorMovement) {
        if (currentIndex < 100) {
            // revert adds 100 to the currentIndex to force a reload
            //  the NON-revert cases will all be normal indices
            maybeSaveCuesheet(2);  // 2 options: don't save or save, no cancel
        } else {
            currentIndex -= 100; // revert case
        }

        QString cuesheetFilename = ui->comboBoxCuesheetSelector->itemData(currentIndex).toString();
//        qDebug() << "on_comboBoxCuesheetSelector_currentIndexChanged is about to load: " << cuesheetFilename;
        loadCuesheet(cuesheetFilename);
    }
}

void MainWindow::action_session_change_triggered()
{
    QList<QAction *> actions(sessionActionGroup->actions());
    QList<SessionInfo> sessions(songSettings.getSessionInfo());
    for (auto action : actions)
    {
        if (action->isChecked())
        {
            QString name(action->text());
            for (const auto &session : sessions)
            {
                if (name == session.name)
                {
                    // qDebug() << "action_session_change_triggered -- setCurrentSessionIdReloadSongAges: " << session.id << action->text();
                    setCurrentSessionIdReloadSongAges(session.id);
                    break;
                }
            }
            break;
        }
    }
    // qDebug() << "manual session change triggered!";
    currentSongSecondsPlayed = 0;
    currentSongSecondsPlayedRecorded = false;
}

void MainWindow::populateMenuSessionOptions()
{
    // what mode are we in right now?
    SessionDefaultType sessionDefault =
        static_cast<SessionDefaultType>(prefsManager.GetSessionDefault()); // preference setting

    QList<QAction *> oldActions(sessionActionGroup->actions());
    for (auto action : oldActions)
    {
        sessionActionGroup->removeAction(action);
        ui->menuSession->removeAction(action);
    }

    QList<QAction *> sessionActions(ui->menuSession->actions());
    QAction *topSeparator = sessionActions[0];
    
    int id = songSettings.getCurrentSession();
    // qDebug() << "populateMenuSessionOptions, currentSessionID:" << id;

    QList<SessionInfo> sessions(songSettings.getSessionInfo());
    
    for (const auto &session : sessions)
    {
        QAction *action = new QAction(session.name, this);
        action->setCheckable(true);
        ui->menuSession->insertAction(topSeparator, action);
        connect(action, SIGNAL(triggered()), this, SLOT(action_session_change_triggered()));
        sessionActionGroup->addAction(action);
        // qDebug() << "   populating sessions menu:" << session.name << session.id;
        if (session.id == id) {
            // qDebug() << "      setting it to CHECKED";
            action->setChecked(true);
        }
        action->setEnabled(sessionDefault == SessionDefaultPractice); // automatic mode disables these menu items, manual mode enables
    }
}

void MainWindow::on_menuLyrics_aboutToShow()
{
    // only allow Save if it's not a template, and the doc was modified
    ui->actionSave_Cuesheet->setEnabled(ui->textBrowserCueSheet->document()->isModified() && !loadedCuesheetNameWithPath.contains(".template.html"));
    ui->actionSave_Cuesheet_As->setEnabled(hasLyrics);  // Cuesheet > Save Cuesheet As... is enabled if there are lyrics
    ui->actionLyricsCueSheetRevert_Edits->setEnabled(ui->textBrowserCueSheet->document()->isModified());

    // qDebug() << "About to show:" << optionCurrentlyPressed;

    // Cuesheet > Explore Cuesheet Matching... dialog box option visible only when OPT is held down
    ui->actionExplore_Cuesheet_Matching->setVisible(optionCurrentlyPressed);
}

void MainWindow::on_actionLyricsCueSheetRevert_Edits_triggered(bool /*checked*/)
{
    on_comboBoxCuesheetSelector_currentIndexChanged(ui->comboBoxCuesheetSelector->currentIndex() + 100); // indicate that we do NOT want to check for being edited
}

void MainWindow::on_actionShow_All_Ages_triggered(bool checked)
{
    reloadSongAges(checked);
}

// ----------------------------------------------------------------------
MainWindow::~MainWindow()
{
    cBass->Pause(); // if we don't do this, LoudMax will be called and will try to touch the window which will be dead already, and BOOM
    cBass->Stop();  // if we don't do this, LoudMax will be called and will try to touch the window which will be dead already, and BOOM

    // clearing the database lock as soon as possible, so that we don't put up scary messages later,
    //   if SD couldn't be killed.
    QString currentMusicRootPath = prefsManager.GetmusicPath();
    clearLockFile(currentMusicRootPath); // release the lock that we took (other locks were thrown away)

    // kill all the vamp subprocesses
    if (vampFuture.isRunning()) {
        vampFuture.cancel();  // ASAP stop any more Vamp jobs from starting
        killAllVamps = true;  // tell the running Vamps to kill themselves
        sleep(5);  // give them a few seconds to go away
    }

    if (darkmode) {
        playlistSlotWatcherTimer->stop();
        playlistSlotWatcherTriggered(); // auto-save anything that hasn't been saved yet
    }

    SDSetCurrentSeqs(0);  // this doesn't take very long

    // bug workaround: https://bugreports.qt.io/browse/QTBUG-56448
    QColorDialog colorDlg(nullptr);
    colorDlg.setOption(QColorDialog::NoButtons);
    colorDlg.setCurrentColor(Qt::white);

    delete ui;
    delete sd_redo_stack;
    
    // Clean up debug dialog
    if (cuesheetDebugDialog) {
        delete cuesheetDebugDialog;
        cuesheetDebugDialog = nullptr;
    }

    if (sdthread)
    {
        sdthread->finishAndShutdownSD(); // try to kill it nicely
        delete sdthread; // call the destructor explicitly, which will terminate the thread, if it is not stopped in 250ms
    }
//    if (ps) {
//        ps->kill();
//    }

    if (prefsManager.GetenableAutoAirplaneMode()) {
        airplaneMode(false);
    }

    if (pathStack) {
        delete pathStack;
    }
    if (pathStackCuesheets) {
        delete pathStackCuesheets;
    }
    if (pathStackPlaylists) {
        delete pathStackPlaylists;
    }
    if (pathStackApplePlaylists) {
        delete pathStackApplePlaylists;
    }

    delete darkStopIcon;
    delete darkPlayIcon;
    delete darkPauseIcon;

    if (fileWatcherTimer) {
        fileWatcherTimer->stop();
        delete fileWatcherTimer;
    }
    if (fileWatcherDisabledTimer) {
        fileWatcherDisabledTimer->stop();
        delete fileWatcherDisabledTimer;
    }
    if (playlistSlotWatcherTimer) {
        playlistSlotWatcherTimer->stop();
        delete playlistSlotWatcherTimer;
    }

    if (waveform) { delete waveform;
                    waveform = nullptr;
    }

    delete[] danceProgramActions;
    delete sessionActionGroup;
    delete sdActionGroupDanceProgram;
    delete cBass;

#ifdef USE_JUCE
    // JUCE -------
    // delete loudMaxWin;
    // delete paramThresh;
#endif
}

// ----------------------------------------------------------------------
void MainWindow::updateSongTableColumnView()
{
    ui->darkSongTable->setColumnHidden(kRecentCol,!prefsManager.GetshowRecentColumn());
    ui->darkSongTable->setColumnHidden(kAgeCol,!prefsManager.GetshowAgeColumn());

    ui->darkSongTable->setColumnHidden(kPitchCol,!prefsManager.GetshowPitchColumn());
    ui->playlist1Table->horizontalHeader()->setSectionHidden(2, !prefsManager.GetshowPitchColumn()); // as the View > Columns > Pitch changes
    ui->playlist2Table->horizontalHeader()->setSectionHidden(2, !prefsManager.GetshowPitchColumn()); //   so does visibility of the pitch column in the playlists
    ui->playlist3Table->horizontalHeader()->setSectionHidden(2, !prefsManager.GetshowPitchColumn());

    ui->darkSongTable->setColumnHidden(kTempoCol,!prefsManager.GetshowTempoColumn());
    ui->playlist1Table->horizontalHeader()->setSectionHidden(3, !prefsManager.GetshowTempoColumn()); // as the View > Columns > Tempo changes
    ui->playlist2Table->horizontalHeader()->setSectionHidden(3, !prefsManager.GetshowTempoColumn()); //   so does visibility of the tempo column in the playlists
    ui->playlist3Table->horizontalHeader()->setSectionHidden(3, !prefsManager.GetshowTempoColumn());

    // http://www.qtcentre.org/threads/3417-QTableWidget-stretch-a-column-other-than-the-last-one
    QHeaderView *darkHeaderView = ui->darkSongTable->horizontalHeader();
    darkHeaderView->setSectionResizeMode(kNumberCol, QHeaderView::Fixed);
    darkHeaderView->setSectionResizeMode(kTypeCol, QHeaderView::Interactive);
    darkHeaderView->setSectionResizeMode(kLabelCol, QHeaderView::Interactive);
    darkHeaderView->setSectionResizeMode(kTitleCol, QHeaderView::Stretch);

    darkHeaderView->setSectionResizeMode(kRecentCol, QHeaderView::Fixed);
    darkHeaderView->setSectionResizeMode(kAgeCol, QHeaderView::Fixed);
    darkHeaderView->setSectionResizeMode(kPitchCol, QHeaderView::Fixed);
    darkHeaderView->setSectionResizeMode(kTempoCol, QHeaderView::Fixed);
    darkHeaderView->setStretchLastSection(false);

    ui->darkSongTable->horizontalHeaderItem(kNumberCol)->setTextAlignment( Qt::AlignCenter );
    ui->darkSongTable->horizontalHeaderItem(kRecentCol)->setTextAlignment( Qt::AlignCenter );
    ui->darkSongTable->horizontalHeaderItem(kAgeCol)->setTextAlignment( Qt::AlignCenter );
    ui->darkSongTable->horizontalHeaderItem(kPitchCol)->setTextAlignment( Qt::AlignCenter );
    ui->darkSongTable->horizontalHeaderItem(kTempoCol)->setTextAlignment( Qt::AlignCenter );
}


// ----------------------------------------------------------------------
void MainWindow::on_loopButton_toggled(bool checked)
{
    Q_UNUSED(checked)
    if (checked) {
        ui->actionLoop->setChecked(true);

        // ui->seekBar->SetLoop(true);
        ui->seekBarCuesheet->SetLoop(true);

        ui->darkSeekBar->setLoop(true);
        double songLength = cBass->FileLength;
//        qDebug() << "songLength: " << songLength << ", Intro: " << ui->seekBar->GetIntro();

//        cBass->SetLoop(songLength * 0.9, songLength * 0.1); // FIX: use parameters in the MP3 file
        cBass->SetLoop(songLength * static_cast<double>(ui->darkSeekBar->getOutroFrac()),
                      songLength * static_cast<double>(ui->darkSeekBar->getIntroFrac()));
    }
    else {
        ui->actionLoop->setChecked(false);

        // ui->seekBar->SetLoop(false);
        ui->seekBarCuesheet->SetLoop(false);
        ui->darkSeekBar->setLoop(false);

        cBass->ClearLoop();
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_monoButton_toggled(bool checked)
{
    Q_UNUSED(checked)
    if (checked) {
        ui->actionForce_Mono_Aahz_mode->setChecked(true);
        cBass->SetMono(true);
    }
    else {
        ui->actionForce_Mono_Aahz_mode->setChecked(false);
        cBass->SetMono(false);
    }

    // the Force Mono (Aahz Mode) setting is persistent across restarts of the application
    prefsManager.Setforcemono(ui->actionForce_Mono_Aahz_mode->isChecked());
}

// ----------------------------------------------------------------------
void MainWindow::randomizeFlashCall() {
    int numCalls = flashCalls.length();
    if (!numCalls)
        return;

    int newRandCallIndex;
    do {
//        newRandCallIndex = qrand() % numCalls;
        newRandCallIndex = QRandomGenerator::global()->bounded(numCalls); // 0 thru numCalls-1
    } while (newRandCallIndex == randCallIndex);
    randCallIndex = newRandCallIndex;
}

// SONGTABLEREFACTOR
int MainWindow::getSelectionRowForFilename(const QString &filePath)
{
    Q_UNUSED(filePath)
    return -1;
}

int MainWindow::darkGetSelectionRowForFilename(const QString &filePath)
{
    for (int i=0; i < ui->darkSongTable->rowCount(); i++) {
        QString origPath = ui->darkSongTable->item(i,kPathCol)->data(Qt::UserRole).toString();
        if (filePath == origPath)
            return i;
    }
    return -1;
}

// ----------------------------------------------------------------------
void MainWindow::on_darkPitchSlider_valueChanged(int value)
{
    // qDebug() << "***** on_darkPitchSlider_valueChanged";

    cBass->SetPitch(value);
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
    // ui->currentPitchLabel->setText(sign + QString::number(currentPitch) +" semitone" + plural);

    if (sourceForLoadedSong == nullptr) {
        // not initialized yet, so just ignore
//        qDebug() << "sourceForLoadedSong not initialized yet...";
        return;
    } else if ((sourceForLoadedSong == ui->playlist1Table && !relPathInSlot[0].startsWith("/tracks")) ||
               (sourceForLoadedSong == ui->playlist2Table && !relPathInSlot[1].startsWith("/tracks")) ||
               (sourceForLoadedSong == ui->playlist3Table && !relPathInSlot[2].startsWith("/tracks"))
               ) {
        // source was one of the playlists, so we just update this one playlist
        // first, find which playlist entry is the one that is loaded
        for (int i = 0; i < sourceForLoadedSong->rowCount(); i++) {
            if (sourceForLoadedSong->item(i, 5)->text() == "1") {
                // second, update the pitch in the playlist
                sourceForLoadedSong->item(i,2)->setText(QString::number(currentPitch));

                // and mark the playlist modified ---------------
                int whichSlot = 0;
                if (sourceForLoadedSong == ui->playlist2Table) {
                    whichSlot = 1;
                } else if (sourceForLoadedSong == ui->playlist3Table) {
                    whichSlot = 2;
                }
                slotModified[whichSlot] = true; // this playlist was modified
                playlistSlotWatcherTimer->start(std::chrono::seconds(10)); // 10 secs after last change, playlist will be written onto disk

                break; // and break out of the loop (because we found the one item we were looking for)
            }
        }
    } else {
        // source was one of the songTables, OR one of the palette slots containing a Track Filter, so update
        //   both songTables and all the palette slots containing Track Filters
        //
        saveCurrentSongSettings();

        // and update the darkSongTable, too
        ui->darkSongTable->setSortingEnabled(false);
        int darkRow = darkGetSelectionRowForFilename(currentMP3filenameWithPath);
        if (darkRow != -1)
        {
            ui->darkSongTable->item(darkRow, kPitchCol)->setText(QString::number(currentPitch)); // already trimmed()
        }
        ui->darkSongTable->setSortingEnabled(true);

        // if the pitch changed, update it in all palette slots that have TRACK FILTERS that contain the currently loaded song
        //   NOTE: Only one track filter can match, because track filters are mutually exclusive...
        for (int i = 0; i < 3; i++) {
            QString rPath = relPathInSlot[i];
            QString rPath2 = rPath;
            rPath2.replace("/tracks", "");
            rPath2 = rPath2 + "/";

            QTableWidget *theTables[3] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};
            QTableWidget *theTable = theTables[i];

            if (rPath.contains("/tracks/") &&
                currentMP3filenameWithPath.contains(rPath2)) {
                for (int j = 0; j < theTable->rowCount(); j++) {
                      if (theTable->item(j, 4)->text() == currentMP3filenameWithPath) {
                        theTable->item(j, 2)->setText(QString::number(currentPitch)); // update pitch in palette slot table
                        break; // no need to look further, because only one item can match per track filter
                      }
                }
            }
        }
    }

    // special checking for playlist ------
    // if (targetNumber != "" && !loadingSong) {
    //     // THIS IS FOR THE LIGHT MODE SONGTABLE ONLY
    //     // current song is on a playlist, AND we're not doing the initial load
    //     // qDebug() << "current song is on playlist, and PITCH changed!" << targetNumber << loadingSong;
    //     markPlaylistModified(true); // turn ON the * in the status bar, because a playlist entry changed its tempo
    // }

    // update the label, too
    QString s = QString::number(value);
    if (value > 0) {
        s = "+" + s;
    }
    this->ui->darkPitchLabel->setText(s);

    QString msg1 = QString("Tempo:") + ui->darkTempoLabel->text() + ", Pitch:" + ui->darkPitchLabel->text();
    ui->statusBar->showMessage(msg1);
}

// ----------------------------------------------------------------------
void MainWindow::Info_Volume(void)
{
}

// ----------------------------------------------------------------------
void MainWindow::on_darkVolumeSlider_valueChanged(int value)
{
    Q_UNUSED(value)

    // CODE SMELL TEST --------------
//     if (value < minimumVolume) {
// //        qDebug() << "volume too low, setting volume to:" << minimumVolume;
//         ui->darkVolumeSlider->setValue(minimumVolume); // this will NOT recurse more than once
//     }

    int voltageLevelToSet = static_cast<int>(100.0*pow(10.0,(((value*0.8)+20)/2.0 - 50)/20.0));
    if (value == 0) {
        voltageLevelToSet = 0;  // special case for slider all the way to the left (MUTE)
    }
    cBass->SetVolume(voltageLevelToSet);     // now logarithmic, 0 --> 0.01, 50 --> 0.1, 100 --> 1.0 (values * 100 for libbass)
    currentVolume = static_cast<unsigned short>(value);    // this will be saved with the song (0-100)

//    Info_Volume();  // update the slider text

    // and update the darkVolumeLabel (NOT via connect and lambda)
    QString s = QString::number(value);
    if (value == 100) {
        s = "MAX";
    } else if (value == 0) {
        s = "MIN";
    }
    this->ui->darkVolumeLabel->setText(s);

    // -------------------------------
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
    if (ui->darkVolumeSlider->value() != 0) {
        previousVolume = ui->darkVolumeSlider->value();
        ui->darkVolumeSlider->setValue(0);
        ui->actionMute->setText("Un&mute");
    }
    else {
        ui->darkVolumeSlider->setValue(previousVolume);
        ui->actionMute->setText("&Mute");
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_darkTempoSlider_valueChanged(int value)
{
    // qDebug() << "on_tempoSlider_valueChanged:" << value;
    if (tempoIsBPM) {
        double desiredBPM = static_cast<double>(value);            // desired BPM
        int newBASStempo = static_cast<int>(round(100.0*desiredBPM/baseBPM));
        cBass->SetTempo(newBASStempo);
        // ui->darkTempoLabel->setText(QString::number(value) + " BPM (" + QString::number(newBASStempo) + "%)");
        ui->darkTempoLabel->setText(QString::number(value));
    }
    else {
        double basePercent = 100.0;                      // original detected percent
        double desiredPercent = static_cast<double>(value);            // desired percent
        int newBASStempo = static_cast<int>(round(100.0*desiredPercent/basePercent));
        cBass->SetTempo(newBASStempo);
        ui->darkTempoLabel->setText(QString::number(value) + "%");
    }

    saveCurrentSongSettings();
// SONGTABLEREFACTOR
    // update the hidden tempo column

    ui->darkSongTable->setSortingEnabled(false);

    int row = darkGetSelectionRowForFilename(currentMP3filenameWithPath);
    if (row != -1)
    {
        if (tempoIsBPM) {
            ui->darkSongTable->item(row, kTempoCol)->setText(QString::number(value));
//            qDebug() << "on_tempoSlider_valueChanged: setting text for tempo to: " << QString::number(value);
        }
        else {
            ui->darkSongTable->item(row, kTempoCol)->setText(QString::number(value) + "%");
//            qDebug() << "on_tempoSlider_valueChanged: setting text for tempo to: " << QString::number(value) + "%";
        }
    }

    ui->darkSongTable->setSortingEnabled(true);

    QString tempoText = QString::number(value);
    if (row != -1)
    {
        if (!tempoIsBPM) {
            tempoText = tempoText + "%";
        }
    }

    if (sourceForLoadedSong == nullptr) {
        // not initialized yet, so just ignore
        //        qDebug() << "sourceForLoadedSong not initialized yet...";
        return;
    } else if ((sourceForLoadedSong == ui->playlist1Table && !relPathInSlot[0].startsWith("/tracks")) ||
               (sourceForLoadedSong == ui->playlist2Table && !relPathInSlot[1].startsWith("/tracks")) ||
               (sourceForLoadedSong == ui->playlist3Table && !relPathInSlot[2].startsWith("/tracks"))
               ) {
        // source was one of the playlists, so we just update this one playlist
        // first, find which playlist entry is the one that is loaded
        for (int i = 0; i < sourceForLoadedSong->rowCount(); i++) {
            if (sourceForLoadedSong->item(i, 5)->text() == "1") {
                // second, update the pitch in the playlist
                sourceForLoadedSong->item(i,3)->setText(tempoText);

                // and mark the playlist modified ---------------
                int whichSlot = 0;
                if (sourceForLoadedSong == ui->playlist2Table) {
                    whichSlot = 1;
                } else if (sourceForLoadedSong == ui->playlist3Table) {
                    whichSlot = 2;
                }
                slotModified[whichSlot] = true; // this playlist was modified
                playlistSlotWatcherTimer->start(std::chrono::seconds(10)); // 10 secs after last change, playlist will be written onto disk

                break; // and break out of the loop (because we found the one item we were looking for)
            }
        }
    } else {
        // source was one of the songTables, OR one of the palette slots containing a Track Filter, so update
        //   both songTables and all the palette slots containing Track Filters
        //
        saveCurrentSongSettings();

        // and update the darkSongTable, too
        ui->darkSongTable->setSortingEnabled(false);

        int darkRow = darkGetSelectionRowForFilename(currentMP3filenameWithPath);
        if (darkRow != -1)
        {
            ui->darkSongTable->item(darkRow, kTempoCol)->setText(tempoText);
        }

        ui->darkSongTable->setSortingEnabled(true);

        // if the tempo changed, update it in all palette slots that have TRACK FILTERS that contain the currently loaded song
        //   NOTE: Only one track filter can match, because track filters are mutually exclusive...
        for (int i = 0; i < 3; i++) {
            QString rPath = relPathInSlot[i];
            QString rPath2 = rPath;
            rPath2.replace("/tracks", "");
            rPath2 = rPath2 + "/";

            QTableWidget *theTables[3] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};
            QTableWidget *theTable = theTables[i];

            if (rPath.contains("/tracks/") &&
                currentMP3filenameWithPath.contains(rPath2)) {
                for (int j = 0; j < theTable->rowCount(); j++) {
                      if (theTable->item(j, 4)->text() == currentMP3filenameWithPath) {
                        theTable->item(j, 3)->setText(tempoText); // update tempo in palette slot table
                        break; // no need to look further, because only one item can match per track filter
                      }
                }
            }
        }
    }

    ui->darkTempoLabel->setText(tempoText);

    QString msg1 = QString("Tempo:") + ui->darkTempoLabel->text() + ", Pitch:" + ui->darkPitchLabel->text();
    ui->statusBar->showMessage(msg1);
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
    seekBar->setMaximum(static_cast<int>(cBass->FileLength)-1); // NOTE: TRICKY, counts on == below
    seekBar->setTickInterval(10);  // 10 seconds per tick
}

void SetSeekBarPositionWithoutValueChanged(QSlider *darkSeekBar, int currentPos_i)
{
    darkSeekBar->blockSignals(true); // setValue should NOT initiate a valueChanged()
    darkSeekBar->setValue(currentPos_i);
    darkSeekBar->blockSignals(false);
}

//void SetSeekBarNoSongLoaded(MySlider *seekBar)
void SetSeekBarNoSongLoaded(QSlider *seekBar)
{
    seekBar->setMinimum(0);
    seekBar->setValue(0);
//    seekBar->ClearMarkers();
}

// ----------------------------------------------------------------------
#define NEWINFOSEEKBAR

// update the text, and optionally move the slide to match cBass's current position
void MainWindow::Info_Seekbar(bool forceSlider)
{
    static bool in_Info_Seekbar = false;
    if (in_Info_Seekbar) {
        qDebug() << "info seekbar recursive return";
        return;
    }
    RecursionGuard recursion_guard(in_Info_Seekbar);

    if (!songLoaded) {
        SetSeekBarNoSongLoaded(ui->seekBarCuesheet);
        SetSeekBarNoSongLoaded(ui->darkSeekBar);
        return;
    }

    cBass->StreamGetPosition();  // update cBass->Current_Position

    // float currentPos_f = static_cast<float>(cBass->Current_Position);
    int currentPos_i   = static_cast<int>(cBass->Current_Position);

    // UPDATE TEXT FIELDS --------------------------------
    int fileLen_i = static_cast<int>(cBass->FileLength);

    if (prefsManager.GetuseTimeRemaining()) {
        // time remaining in song
        // ui->currentLocLabel->setText(position2String(fileLen_i - currentPos_i, true));  // pad on the left
        ui->currentLocLabel_2->setText(position2String(fileLen_i - currentPos_i, true));  // pad on the left
        ui->currentLocLabel3->setText(position2String(fileLen_i - currentPos_i, true));  // pad on the left
    } else {
        // current position in song
        // ui->currentLocLabel->setText(position2String(currentPos_i, true));              // pad on the left
        ui->currentLocLabel_2->setText(position2String(currentPos_i, true));              // pad on the left
        ui->currentLocLabel3->setText(position2String(currentPos_i, true));              // pad on the left
    }

    ui->timeSlash->setVisible(true);
    ui->songLengthLabel2->setText(position2String(fileLen_i));          // no padding, intentionally no prefix "/"

    // SINGING CALL SECTIONS ----------------------------------------------
    if (currentSongIsSinger || currentSongIsVocal) {
        double introLength = static_cast<double>(ui->darkSeekBar->getIntroFrac()) * cBass->FileLength; // seconds
        double outroTime = static_cast<double>(ui->darkSeekBar->getOutroFrac()) * cBass->FileLength; // seconds
        double outroLength = fileLen_i - outroTime;

        double anticipateSectionChange_sec = 2.5; // change singingCallSection indicator in warningLabel slightly before we actually get there

        if (cBass->isPaused()) {
            anticipateSectionChange_sec = 0.0; // override, if we're not playing
        }

        int section;
        if (currentPos_i + anticipateSectionChange_sec < introLength) {
            section = 0; // intro
        } else if (currentPos_i + anticipateSectionChange_sec > outroTime) {
            section = 8;  // tag
        } else {
            section = static_cast<int>(1.0 + 7.0*(((currentPos_i + anticipateSectionChange_sec) - introLength)/(fileLen_i-(introLength+outroLength))));
            if (section > 8 || section < 0) {
                section = 0; // needed for the time before fields are fully initialized
            }
        }

        QStringList sectionName;
        sectionName << "Intro" << "Opener" << "Figure 1" << "Figure 2"
                    << "Break" << "Figure 3" << "Figure 4" << "Closer" << "Tag";

        // TODO: all the other stuff from AnalogClock::redrawTimerExpired() *****

        // if (cBass->currentStreamState() == BASS_ACTIVE_PLAYING && (currentSongIsSinger || currentSongIsVocal)) {
        if (currentSongIsSinger || currentSongIsVocal) {
            // if singing call OR called, then tell the clock to show the section type
            ui->theSVGClock->setSingingCallSection(sectionName[section]);
        } else {
            // else tell the clock that there isn't a section type
            ui->theSVGClock->setSingingCallSection("");
        }

       // qDebug() << "currentPos:" << currentPos_i << ", fileLen: " << fileLen_i
       //          << "outroFrac:" << ui->darkSeekBar->getOutroFrac()
       //          << "outroTime:" << outroTime
       //          << "introLength:" << introLength
       //          << "outroLength:" << outroLength
       //          << "section: " << section
       //          << "sectionName[section]: " << sectionName[section];

    } else {
        // ui->darkWarningLabel->setText(""); // not a singing call
        ui->theSVGClock->setSingingCallSection("");
    }

    // FLASHCALLS ------------------------------------------------------
    int flashCallEverySeconds = prefsManager.Getflashcalltiming().toInt();
    if (currentPos_i % flashCallEverySeconds == 0 && currentPos_i != 0) {
        // Now pick a new random number, but don't pick the same one twice in a row.
        // TODO: should really do a permutation over all the allowed calls, with no repeats
        //   but this should be good enough for now, if the number of calls is sufficiently
        //   large.
        randomizeFlashCall();
    }

    if (flashCalls.length() != 0) {
        // if there are flash calls on the list, then Flash Calls are enabled.
        if (cBass->currentStreamState() == BASS_ACTIVE_PLAYING && (currentSongIsPatter || currentSongIsUndefined)) { // FLASH calls enabled if Patter OR Unknown type
             // if playing, and Patter type
             // TODO: don't show any random calls until at least the end of the first N seconds
             setNowPlayingLabelWithColor(flashCalls[randCallIndex], true);

             flashCallsVisible = true;
         } else {
             // flash calls on the list, but not playing, or not patter
             if (flashCallsVisible) {
                 // if they were visible, they're not now
                 setNowPlayingLabelWithColor(currentSongTitle);

                 flashCallsVisible = false;
             }
         }
    } else {
        // no flash calls on the list
        if (flashCallsVisible) {
            // if they were visible, they're not now
            setNowPlayingLabelWithColor(currentSongTitle);

            flashCallsVisible = false;
        }
    }

    // FORCIBLY MOVE THE SLIDER ----------------------------------------
    if (forceSlider) {
        // force the sliders to reflect current position in the song ----------------
        SetSeekBarPositionWithoutValueChanged(ui->seekBarCuesheet, currentPos_i); // don't generate a ValueChanged event
        // SetSeekBarPositionWithoutValueChanged(ui->darkSeekBar, currentPos_i);     // don't generate a ValueChanged event
        // qDebug() << "Info_Seekbar needs to be moved to:" << currentPos_i;
        ui->darkSeekBar->setValue(currentPos_i);

        // and do auto-scroll -------------------
        int minScroll = ui->textBrowserCueSheet->verticalScrollBar()->minimum();
        int maxScroll = ui->textBrowserCueSheet->verticalScrollBar()->maximum();
        int maxSeekbar = ui->darkSeekBar->maximum();  // NOTE: minSeekbar is always 0
        double fracSeekbar = static_cast<double>(currentPos_i)/static_cast<double>(maxSeekbar);
        double targetScroll = 1.08 * fracSeekbar * (maxScroll - minScroll) + minScroll;  // FIX: this is heuristic and not right yet

        // NOTE: only auto-scroll when the lyrics are LOCKED (if not locked, you're probably editing).
        //   AND you must be playing.  If you're not playing, we're not going to override the InfoBar position.
        if (autoScrollLyricsEnabled &&
                !ui->pushButtonEditLyrics->isChecked() &&
                cBass->currentStreamState() == BASS_ACTIVE_PLAYING &&
                !lyricsForDifferentSong) {
            // lyrics scrolling at the same time as the InfoBar
            ui->textBrowserCueSheet->verticalScrollBar()->setValue(static_cast<int>(targetScroll));
        }
    }
}


// --------------------------------1--------------------------------------

// blame https://stackoverflow.com/questions/4065378/qt-get-children-from-layout
bool isChildWidgetOfAnyLayout(QLayout *layout, QWidget *widget)
{
   if (layout == nullptr || widget == nullptr)
      return false;

   if (layout->indexOf(widget) >= 0)
      return true;

   foreach(QObject *o, layout->children())
   {
      if (isChildWidgetOfAnyLayout(dynamic_cast<QLayout*>(o),widget))
         return true;
   }

   return false;
}

void setVisibleWidgetsInLayout(QLayout *layout, bool visible)
{
   if (layout == nullptr)
      return;

   QWidget *pw = layout->parentWidget();
   if (pw == nullptr)
      return;

   foreach(QWidget *w, pw->findChildren<QWidget*>())
   {
      if (isChildWidgetOfAnyLayout(layout,w))
          w->setVisible(visible);
   }
}

bool isVisibleWidgetsInLayout(QLayout *layout)
{
   if (layout == nullptr)
      return false;

   QWidget *pw = layout->parentWidget();
   if (pw == nullptr)
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


void MainWindow::on_pushButtonClearTaughtCalls_clicked()
{
    QString danceProgram(ui->comboBoxCallListProgram->currentText());

    QMessageBox msgBox;
    msgBox.setText("Do you really want to clear all calls taught for the " + danceProgram + " dance program for the current session?");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes) {
        songSettings.clearTaughtCalls(danceProgram);
        on_comboBoxCallListProgram_currentIndexChanged(ui->comboBoxCallListProgram->currentIndex());
    }
}

// --------------------------------1--------------------------------------
void MainWindow::getCurrentPointInStream(double *pos, double *len) {
    double position, length;

//    if (cBass->Stream_State == BASS_ACTIVE_PLAYING) {
    if (cBass->currentStreamState() == BASS_ACTIVE_PLAYING) {
        // if we're playing, this is accurate to sub-second.
        cBass->StreamGetPosition(); // snapshot the current position
        position = cBass->Current_Position;
        length = cBass->FileLength;  // always use the value with maximum precision

    } else {
        // if we're NOT playing, this is accurate to the second.  (This should be fixed!)
        position = static_cast<double>(ui->seekBarCuesheet->value());
        length = ui->seekBarCuesheet->maximum();
    }

    // return values
    *pos = position;
    *len = length;
}

// --------------------------------1--------------------------------------
void MainWindow::on_pushButtonSetIntroTime_clicked()
{
    double position, length;
    getCurrentPointInStream(&position, &length);
//    qDebug() << "MainWindow::on_pushButtonSetIntroTime_clicked: " << position << length;

    QTime currentOutroTime = ui->dateTimeEditOutroTime->time();
    double currentOutroTimeSec = 60.0*currentOutroTime.minute() + currentOutroTime.second() + currentOutroTime.msec()/1000.0;
    position = fmax(0.0, fmin(position, static_cast<int>(currentOutroTimeSec)-6) );

    // snap to nothing/bar/beat, depending on current snap setting --------
    unsigned char granularity = GRANULARITY_NONE;
    if (ui->actionNearest_Beat->isChecked()) {
        granularity = GRANULARITY_BEAT;
    } else if (ui->actionNearest_Measure->isChecked()) {
        granularity = GRANULARITY_MEASURE;
    }

    position = cBass->snapToClosest(position, granularity);

    if (position < 0) {
        qDebug() << "***** on_pushButtonSetIntroTime_clicked: snap failed, turning off snapping!";
        // ui->actionDisabled->setChecked(true); // turn off snapping
        position *= -1; // and make it positive
    }

    // set in ms
    ui->dateTimeEditIntroTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*position+0.5))); // milliseconds
    ui->darkStartLoopTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*position+0.5))); // milliseconds
    // set in fractional form
    double frac = position/length;
    ui->seekBarCuesheet->SetIntro(frac);  // after the events are done, do this.

    // if (darkmode) {
    ui->darkSeekBar->setIntro(frac);
    ui->darkSeekBar->updateBgPixmap((float*)1, 1);  // update the bg pixmap, in case it was a singing call
    // } else {
    //     ui->seekBar->SetIntro(frac);
    // }

    on_loopButton_toggled(ui->actionLoop->isChecked()); // then finally do this, so that cBass is told what the loop points are (or they are cleared)

    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();

    if (modifiers & Qt::ShiftModifier) {
        // SHIFT-[ also sets the end point of the loop to:
        //   singer: position + 7 * 64 beats = 448 beats later

        // bool isSingingCall = songTypeNamesForSinging.contains(currentSongType) ||
        //                      songTypeNamesForCalled.contains(currentSongType);

        bool isSingingCall = currentSongIsSinger || currentSongIsVocal;

        // qDebug() << "on_pushButtonSetIntroTime_clicked(): isSingingCall = " << isSingingCall;

        if (!isSingingCall) {
            qDebug() << "***** INFO: We have not implemented SHIFT-[ for Patter yet.";
            return;  // we're done here.
        }

        double outroPosition;
        outroPosition = cBass->snapToClosest(position + (7 * 64 * (60.0/baseBPM)), granularity); // Always 7 sections of 64 beats in a singer
                                                                                                 //  If it's a weird singer that's different, don't use this feature.

        if (outroPosition < 0) {
            qDebug() << "***** setting outroPosition: snap failed, turning off snapping!";
            outroPosition *= -1; // and make it positive
        }

        // qDebug() << "SETTING THE OUTRO POSITION BECAUSE THIS IS A SINGING CALL! " << baseBPM << outroPosition;

        // set in ms
        ui->dateTimeEditOutroTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*outroPosition+0.5))); // milliseconds
        ui->darkEndLoopTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*outroPosition+0.5))); // milliseconds
        // set in fractional form
        double frac = outroPosition/length;
        ui->seekBarCuesheet->SetOutro(frac);  // after the events are done, do this.

        // if (darkmode) {
        // qDebug() << "pushButtonSetIntro:" << frac;
        ui->darkSeekBar->setOutro(frac);
        ui->darkSeekBar->updateBgPixmap((float*)1, 1);  // update the bg pixmap, in case it was a singing call
        // } else {
        //     ui->seekBar->SetOutro(frac);
        // }

        on_loopButton_toggled(ui->actionLoop->isChecked()); // then finally do this, so that cBass is told what the loop points are (or they are cleared)
    }
}

// --------------------------------1--------------------------------------
void MainWindow::on_pushButtonSetOutroTime_clicked()
{
    double position, length;
    getCurrentPointInStream(&position, &length);
//    qDebug() << "MainWindow::on_pushButtonSetOutroTime_clicked: " << position << length;

    QTime currentIntroTime = ui->dateTimeEditIntroTime->time();
    double currentIntroTimeSec = 60.0*currentIntroTime.minute() + currentIntroTime.second() + currentIntroTime.msec()/1000.0;
    position = fmin(length, fmax(position, static_cast<int>(currentIntroTimeSec)+6) );

    // snap to nothing/bar/beat, depending on current snap setting --------
    //   this call dups that of setIntroTimeClicked above, in case we need to snap in and out points separately
    //   so I am intentionally NOT putting this into getCurrentPointInStream() right now...
    unsigned char granularity = GRANULARITY_NONE;
    if (ui->actionNearest_Beat->isChecked()) {
        granularity = GRANULARITY_BEAT;
    } else if (ui->actionNearest_Measure->isChecked()) {
        granularity = GRANULARITY_MEASURE;
    }

    position = cBass->snapToClosest(position, granularity);

    if (position < 0) {
//        qDebug() << "on_pushButtonSetOutroTime_clicked: snap failed, turning off snapping!";
        ui->actionDisabled->setChecked(true); // turn off snapping
        position *= -1; // and make it positive
    }

    // set in ms
    ui->dateTimeEditOutroTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*position+0.5))); // milliseconds
    ui->darkEndLoopTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*position+0.5))); // milliseconds
    // set in fractional form
    double frac = position/length;
    ui->seekBarCuesheet->SetOutro(frac);  // after the events are done, do this.

    // if (darkmode) {
    // qDebug() << "pushButtonSetOutro:" << frac;
    ui->darkSeekBar->setOutro(frac);
    ui->darkSeekBar->updateBgPixmap((float*)1, 1);  // update the bg pixmap, in case it was a singing call
    // } else {
    //     ui->seekBar->SetOutro(frac);
    // }

    on_loopButton_toggled(ui->actionLoop->isChecked()); // then finally do this, so that cBass is told what the loop points are (or they are cleared)
}

// --------------------------------1--------------------------------------
void MainWindow::on_seekBarCuesheet_valueChanged(int value)
{
    // qDebug() << "*** on_seekBarCuesheet_valueChanged" << value;
    cBass->StreamSetPosition(value);
    Info_Seekbar(false);
}

// ----------------------------------------------------------------------
void MainWindow::on_darkSeekBar_sliderMoved(int value)
{
    // These must happen in this order.
    // qDebug() << "*** on darkSeekBar sliderMoved" << value;
    cBass->StreamSetPosition(value);
    Info_Seekbar(false);
}

// ----------------------------------------------------------------------
void MainWindow::on_darkSeekBar_valueChanged(int value)
{
    Q_UNUSED(value)
}

// ----------------------------------------------------------------------
void MainWindow::on_actionLoop_triggered()
{
    on_loopButton_toggled(ui->actionLoop->isChecked());
}

// ----------------------------------------------------------------------
void MainWindow::on_UIUpdateTimerTick(void)
{
    if (!mainWindowReady) {
        return;
    }

    int xx = mp3Results.size();
    int outOf = mp3FilenamesToProcess.size();
    if (xx < outOf) {
        ui->statusBar->showMessage(QString("Calculating section info: " + QString::number(xx) + "/" + QString::number(outOf)));
    } else {
        int n = QThread::idealThreadCount();
        if (QThreadPool::globalInstance()->maxThreadCount() < QThread::idealThreadCount()) {
            ui->statusBar->showMessage("");
            // qDebug() << "back to " << n << " threads";
            QThreadPool::globalInstance()->setMaxThreadCount(n); // allow use of all threads again

            ui->darkSeekBar->updateBgPixmap((float*)1, 1);  // update the bg pixmap, in case we now have section info on the loaded song, only update ONCE
        }
    }

    // This is called once per second, to update the seekbar and associated dynamic text

//    qDebug() << "VERTICAL SCROLL VALUE: " << ui->textBrowserCueSheet->verticalScrollBar()->value();

    Info_Seekbar(true);
    
    // update the session coloring analog clock
    QTime time = QTime::currentTime();
    int theType = NONE;
//    qDebug() << "Stream_State:" << cBass->Stream_State; //FIX
//    if (cBass->Stream_State == BASS_ACTIVE_PLAYING) {
    uint32_t Stream_State = cBass->currentStreamState();
    
    // Update Now Playing info when state changes, and less frequently during playback  
    static uint32_t lastStreamState = 0;
    static int nowPlayingUpdateCounter = 0;
    
    bool stateChanged = (Stream_State != lastStreamState);
    lastStreamState = Stream_State;
    
    if (songLoaded && (Stream_State == BASS_ACTIVE_PLAYING || Stream_State == BASS_ACTIVE_PAUSED || Stream_State == BASS_ACTIVE_STOPPED)) {
        // Update immediately if state changed (play/pause/stop)
        if (stateChanged) {
            // printf("Stream state changed from %u to %u, updating Now Playing immediately\n", lastStreamState, Stream_State);
            updateNowPlayingMetadata();
            nowPlayingUpdateCounter = 0;
        }
        // Update periodically during playback for position updates (every 10 seconds)
        else if (Stream_State == BASS_ACTIVE_PLAYING) {
            nowPlayingUpdateCounter++;
            if (nowPlayingUpdateCounter >= 10) {
                // printf("Periodic Now Playing update during playback\n");
                updateNowPlayingMetadata();
                nowPlayingUpdateCounter = 0;
            }
        }
    }
    
    if (Stream_State == BASS_ACTIVE_PLAYING) {
        // if it's currently playing (checked once per second), then color this segment
        //   with the current segment type
        if (currentSongIsPatter || currentSongIsUndefined) { // undefined songs are treated as patter
            theType = PATTER;
        }
        else if (currentSongIsSinger) {
            theType = SINGING;
        }
        else if (currentSongIsVocal) {
            theType = SINGING_CALLED;
        }
        else if (currentSongIsExtra) {
            theType = XTRAS;
        }
        else {
            theType = NONE;
        }

        ui->theSVGClock->breakLengthAlarm = false;  // if playing, then we can't be in break
//    } else if (cBass->Stream_State == BASS_ACTIVE_PAUSED) {
    } else if (Stream_State == BASS_ACTIVE_PAUSED || Stream_State == BASS_ACTIVE_STOPPED) {  // TODO: Check to make sure it doesn't mess up X86.
        // if we paused due to FADE, for example...
        // FIX: this could be factored out, it's used twice.
        // ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
        ui->darkPlayButton->setIcon(*darkPlayIcon);  // change PAUSE to PLAY
        // ui->actionPlay->setText("Play");
        // qDebug() << "updateTimerTick: " << currentSongTitle;
        setNowPlayingLabelWithColor(currentSongTitle);

#ifdef USE_JUCE
        // JUCE ---------
        // if (paramThresh) {
            // qDebug() << "THRESHOLD: " << paramThresh->getValue()
            //          << paramThresh->getCurrentValueAsText().toStdString();
        // }
#endif
    }

#ifndef DEBUGCLOCK
    ui->theSVGClock->setSegment(static_cast<unsigned int>(time.hour()),
                                static_cast<unsigned int>(time.minute()),
                                static_cast<unsigned int>(time.second()),
                                static_cast<unsigned int>(theType));  // always called once per second

#else
    ui->theSVGClock->setSegment(static_cast<unsigned int>(time.minute()),
                            static_cast<unsigned int>(time.second()),
                            0,
                            static_cast<unsigned int>(theType));  // always called once per second
#endif

    // ------------------------
    // Sounds associated with Tip and Break Timers (one-shots)
    newTimerState = ui->theSVGClock->currentTimerState;

    if ((newTimerState & THIRTYSECWARNING)&&!(oldTimerState & THIRTYSECWARNING)) {
        // one-shot transition to 30 second warning
        if (tipLength30secEnabled) {
            playSFX("thirty_second_warning");  // play this file (once), if warning enabled and we cross T-30 boundary
        }
    }

    if ((newTimerState & LONGTIPTIMEREXPIRED) && !(oldTimerState & LONGTIPTIMEREXPIRED)) {
        // one-shot transition to Long Tip
        //  2 => visual indicator only
        //  3 => long_tip sound
        //  4 => play "1.*.mp3", the first item in the user-provided soundfx list
        //  ...
        //  11 => play "8.*.mp3", the 8th item in the user-provided soundfx list
//        qDebug() << "tipLengthAlarmAction:" << tipLengthAlarmAction;
        switch (tipLengthAlarmAction) {
            case 2:
                break;  // visual indicator only
            case 3:
                playSFX("long_tip");
                break;
            default:
                playSFX(QString::number(tipLengthAlarmAction-3));
                break;
        }
    }

    if ((newTimerState & BREAKTIMEREXPIRED) && !(oldTimerState & BREAKTIMEREXPIRED)) {
        // one-shot transition to End of Break
        //  2 => visual indicator only
        //  3 => break_over sound
        //  4 => play "1.*.mp3", the first item in the user-provided soundfx list
        //  ...
        //  11 => play "8.*.mp3", the 8th item in the user-provided soundfx list
//        qDebug() << "breakLengthAlarmAction:" << tipLengthAlarmAction;
        switch (breakLengthAlarmAction) {
            case 2:
                break;
            case 3:
                playSFX("break_over");
                break;
            default:
                playSFX(QString::number(breakLengthAlarmAction-3));
                break;
        }
    }
    oldTimerState = newTimerState;

    // check for Audio Device changes, and set UI status bar, if there's a change
    if ((cBass->currentAudioDevice != lastAudioDeviceName)||(lastAudioDeviceName == "")) {
//        qDebug() << "NEW CURRENT AUDIO DEVICE: " << cBass->currentAudioDevice;
        lastAudioDeviceName = cBass->currentAudioDevice;
        microphoneStatusUpdate();  // now also updates the audioOutputDevice status
//        ui->statusBar->showMessage("Audio output: " + lastAudioDeviceName);
    }

    // figure out what Session we are in, once per minute

    // either SessionDefaultDOW or SessionDefaultPractice
    SessionDefaultType sessionDefault =
        static_cast<SessionDefaultType>(prefsManager.GetSessionDefault()); // preference setting

    // qDebug() << "Ding: " << sessionDefault << SessionDefaultDOW << SessionDefaultPractice;
    if (sessionDefault == SessionDefaultDOW) {
        int currentMinuteInHour = time.minute();
        // qDebug() << "Tick: " << sessionDefault << currentMinuteInHour << lastMinuteInHour;
        if ((currentMinuteInHour != lastMinuteInHour) || (lastSessionID < 0)) { // or cached sessionID is invalid
            // this code is executed once per minute, we're now in a new minute
            int currentSessionID = songSettings.currentSessionIDByTime(); // what session are we in?
            // qDebug() << "Tock: " << currentSessionID << lastSessionID;
            if (currentSessionID != lastSessionID) {
                // only happens at session boundaries, we're now in a new Session, so update Ages column
                setCurrentSessionId(currentSessionID); // save it in songSettings
                populateMenuSessionOptions(); // update the sessions menu with whatever is checked now
                reloadSongAges(ui->actionShow_All_Ages->isChecked());
                on_comboBoxCallListProgram_currentIndexChanged(ui->comboBoxCallListProgram->currentIndex());
                lastSessionID = currentSessionID;
                currentSongSecondsPlayed = 0; // reset the counter, because this is a new session
                currentSongSecondsPlayedRecorded = false; // not reported yet, because this is a new session
                // qDebug() << "***** We are now in Session " << currentSessionID;
            }
            lastMinuteInHour = currentMinuteInHour;
        }
    } else if (sessionDefault == SessionDefaultPractice) {
        if (lastSessionID == -2) {
            // NOTE: this is now done in the constructor, because it's done only once, it's not needed here.
        }
    }

    if (cBass->currentStreamState() == BASS_ACTIVE_PLAYING) {
            currentSongSecondsPlayed++;  // goes from 0 to Inf, but only increments if we're playing
            // qDebug() << "currentSongSecondsPlayed:" << currentSongSecondsPlayed;
    }

    if (!currentSongSecondsPlayedRecorded && (currentSongSecondsPlayed >= 15)) { // GEMA compliance is 15 seconds
        if (!ui->actionDon_t_Save_Plays->isChecked())
        {
            // qDebug() << "actionDon_t_Save_Plays was NOT checked.";
            // marking song "played", because it's been 15 or more seconds of playback in this session
            // qDebug() << "Marking PLAYED:" << currentMP3filename << " in session" << lastSessionID;
            songSettings.markSongPlayed(currentMP3filename, currentMP3filenameWithPath);  // this call is session-aware
            currentSongSecondsPlayedRecorded = true; // not reported yet, because this is a new session

            // now update the darkSongTable because we have now "played" the song
            int row = darkGetSelectionRowForFilename(currentMP3filenameWithPath);
            if (row != -1)
            {
                // update darkSongTable with Recent * and Age 0, because it's been played!
                ui->darkSongTable->item(row, kAgeCol)->setText("0");
                ui->darkSongTable->item(row, kAgeCol)->setTextAlignment(Qt::AlignCenter);

                ui->darkSongTable->item(row, kRecentCol)->setText(ageToRecent("0"));
                ui->darkSongTable->item(row, kRecentCol)->setTextAlignment(Qt::AlignCenter);

                pathsOfCalledSongs.insert(ui->darkSongTable->item(row, kPathCol)->data(Qt::UserRole).toString());
                // ((darkSongTitleLabel *)(ui->darkSongTable->cellWidget(row,kTitleCol)))->setSongUsed(true);
            }
        }
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_vuMeterTimerTick(void)
{
    double currentVolumeSlider = ui->darkVolumeSlider->value();
    int levelR      = cBass->StreamGetVuMeterR();        // do not reset peak detector
    int levelL_mono = cBass->StreamGetVuMeterL_mono();   // get AND reset peak detector

    // levelL_mono = 32768.0; // DEBUG DEBUG DEBUG

    double levelL_monof = (currentVolumeSlider/100.0)*levelL_mono/32768.0;
    double levelRf      = (currentVolumeSlider/100.0)*levelR/32768.0;

    bool isMono = cBass->GetMono();

    // TODO: iff music is playing.
    QString currentTabName = ui->tabWidget->tabText(ui->tabWidget->currentIndex());
    if (currentTabName == "Music") {
        // if (darkmode) {
        ui->darkVUmeter->levelChanged(levelL_monof, levelRf, isMono);  // 10X/sec, update the vuMeter
        //     // qDebug() << "levels: " << levelL_monof << currentVolumeSlider/100.0 << levelRf;
        // } else {
        //     vuMeter->levelChanged(levelL_monof, levelRf, isMono);  // 10X/sec, update the vuMeter
        // }
    }
}


// --------------
bool MainWindow::maybeSavePlaylist(int whichSlot) {
    if (!slotModified[whichSlot]) {
        // slot has not been modified
//        qDebug() << "SLOT HAS NOT BEEN MODIFIED" << whichSlot;
        return true; // all is well
    }

    if (relPathInSlot[whichSlot] != "") {
        // slot has a named something in it
//        qDebug() << "SLOT HAS A NAMED SOMETHING IN IT" << whichSlot << relPathInSlot[whichSlot];
        return true; // all is well
    }

    QMessageBox msgBox;
    msgBox.setText(QString("The 'Untitled playlist' in slot ") + QString::number(whichSlot + 1) + " has been modified.");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setInformativeText("Do you want to save your changes?");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int ret = msgBox.exec();

    switch (ret) {

        case QMessageBox::Save:
            saveSlotAsPlaylist(whichSlot); // Save As it gives it a name
            return true; // all is well

        case QMessageBox::Cancel:
            //            qDebug() << "User clicked CANCEL, returning FALSE";
            return false; // STOP THE QUIT!

        default:
            //            qDebug() << "DEFAULT (DISCARD)";
            break;
    }

    //    qDebug() << "RETURNING TRUE, ALL IS WELL.";
    return true;
}

// --------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    // Work around bug: https://codereview.qt-project.org/#/c/125589/
    if (closeEventHappened) {
        event->accept();
        return;
    }

    // check for unsaved playlists that have been modified, and ask to Save As... each one in turn
    for (int i = 0; i < 3; i++) {
        if (!maybeSavePlaylist(i)) {
            //        qDebug() << "closeEvent ignored, because user cancelled.";
            event->ignore();
            return;
        }
    }

    if (!maybeSaveCuesheet(3)) {
        //        qDebug() << "closeEvent ignored, because user cancelled.";
        event->ignore();
        return;
    }

    closeEventHappened = true;

    if (/* DISABLES CODE */ (true)) {
        on_actionAutostart_playback_triggered();  // write AUTOPLAY setting back
        event->accept();  // OK to close, if user said "OK" or "SAVE"
        saveCurrentSongSettings();

        {
            // SAVE SD TAB VERTICAL SPLITTER SIZES ----------
            QList<int> sizes = ui->splitterSDTabVertical->sizes();
            QString sizeStr("");
            for (int s : sizes)
            {
                if (sizeStr.length())
                {
                    sizeStr += ",";
                }
                QString sstr(QString("%1").arg(s));
                sizeStr += sstr;
            }
            prefsManager.SetSDTabVerticalSplitterPosition(sizeStr);
        }        

        {
            // SAVE SD TAB HORIZONTAL SPLITTER SIZES ----------
            QList<int> sizes = ui->splitterSDTabHorizontal->sizes();
            QString sizeStr("");
            for (int s : sizes)
            {
                if (sizeStr.length())
                {
                    sizeStr += ",";
                }
                QString sstr(QString("%1").arg(s));
                sizeStr += sstr;
            }
            prefsManager.SetSDTabHorizontalSplitterPosition(sizeStr);
        }

        {
            // SAVE MUSIC TAB VERTICAL SPLITTER SIZES ----------
            QList<int> sizes = ui->splitterMusicTabVertical->sizes();
            QString sizeStr("");
            for (int s : sizes)
            {
                if (sizeStr.length())
                {
                    sizeStr += ",";
                }
                QString sstr(QString("%1").arg(s));
                sizeStr += sstr;
            }
            prefsManager.SetMusicTabVerticalSplitterPosition(sizeStr);
        }

        {
            // SAVE MUSIC TAB HORIZONTAL SPLITTER SIZES ----------
            QList<int> sizes = ui->splitterMusicTabHorizontal->sizes();
            QString sizeStr("");
            for (int s : sizes)
            {
                if (sizeStr.length())
                {
                    sizeStr += ",";
                }
                QString sstr(QString("%1").arg(s));
                sizeStr += sstr;
            }
            prefsManager.SetMusicTabHorizontalSplitterPosition(sizeStr);
        }

        // as per http://doc.qt.io/qt-5.7/restoring-geometry.html
        prefsManager.MySettings.setValue("lastCuesheetSavePath", lastCuesheetSavePath);
        prefsManager.MySettings.setValue("geometry", saveGeometry());
        prefsManager.MySettings.setValue("windowState", saveState());
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
#if defined(Q_OS_MAC)
    // as of Qt6.5, links and rich text no longer work in QMessageBoxes
    msgBox.setText("<h2>SquareDesk, V" + QString(VERSIONSTRING) + QString(" (Qt") + QString(QT_VERSION_STR) + QString(")</h2>"));
    msgBox.setText(QString("<p><h2>SquareDesk, V") + QString(VERSIONSTRING) + QString(" (Qt") + QString(QT_VERSION_STR) + QString(")") + QString("</h2>") +
                   QString("<p>Visit our website at <a href=\"http://squaredesk.net\">squaredesk.net</a></p>") +
                   QString("Uses: ") +
                   QString("<a href=\"https://www.tamtwirlers.org/taminations\">Taminations</a>, ") +
                   QString("<a href=\"http://www.lynette.org/sd\">sd</a>, ") +
                   QString("<a href=\"https://github.com/yshurik/qpdfjs\">qpdfjs</a>, ") +
                   QString("<a href=\"https://juce.com\">JUCE</a>, ") +
                   QString("<a href=\"https://github.com/breakfastquay/minibpm\">miniBPM</a>, ") +
                   QString("<a href=\"https://vamp-plugins.org/plugin-doc/qm-vamp-plugins.html\">QM Vamp</a>, ") +
                   QString("<a href=\"https://icons8.com\">Icons8</a>, ") +
                   QString("<a href=\"https://www.kfrlib.com\">kfr</a>, and ") +
                   QString("<a href=\"https://www.surina.net/soundtouch/\">SoundTouch</a>.") +
                   QString("<p>Thanks to: <a href=\"http://all8.com\">all8.com</a>"));
#endif
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

bool MainWindow::someWebViewHasFocus() {
    bool hasFocus = false;
    QListIterator<QWebEngineView*> i(webViews);
    while (!hasFocus && i.hasNext()) {
        hasFocus = hasFocus || (i.next()->hasFocus());
    }
    return(hasFocus);
}

// ------------------------------------------------------------------------
// http://www.codeprogress.com/cpp/libraries/qt/showQtExample.php?key=QApplicationInstallEventFilter&index=188
bool GlobalEventFilter::eventFilter(QObject *Object, QEvent *Event)
{
    // // OPTION key monitoring
    // MainWindow *maybeMainWindow = dynamic_cast<MainWindow *>((dynamic_cast<QApplication *>(Object))->activeWindow());

    // if (maybeMainWindow != nullptr) {
    //     if (Event->type() == QEvent::KeyPress || Event->type() == QEvent::KeyRelease) {
    //         QKeyEvent *keyEvent = static_cast<QKeyEvent*>(Event);
    //         if (keyEvent->key() == Qt::Key_Alt) {
    //             maybeMainWindow->optionCurrentlyPressed = (Event->type() == QEvent::KeyPress);
    //             // qDebug() << "optionCurrentlyPressed:" << maybeMainWindow->optionCurrentlyPressed;
    //         }
    //     }
    // }

    if (Event->type() == QEvent::KeyPress || Event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(Event);
        if (keyEvent->key() == Qt::Key_Alt) {
            if (keyEvent->isAutoRepeat()) {
                return true;  // ignore ALT auto-repeats
            }
            MainWindow *maybeMainWindow = dynamic_cast<MainWindow *>((dynamic_cast<QApplication *>(Object))->activeWindow());
            if (maybeMainWindow == nullptr) {
                return QObject::eventFilter(Object,Event);
            }
            maybeMainWindow->optionCurrentlyPressed = (Event->type() == QEvent::KeyPress);
            // qDebug() << "optionCurrentlyPressed:" << maybeMainWindow->optionCurrentlyPressed;
        }
    }

    if (Event->type() == QEvent::KeyRelease) {
        QKeyEvent *KeyEvent = dynamic_cast<QKeyEvent *>(Event);
        int theKey = KeyEvent->key();

        MainWindow *maybeMainWindow = dynamic_cast<MainWindow *>((dynamic_cast<QApplication *>(Object))->activeWindow());
        if (maybeMainWindow == nullptr) {
            // if the PreferencesDialog is open, for example, do not dereference the NULL pointer (duh!).
//            qDebug() << "QObject::eventFilter()";
            return QObject::eventFilter(Object,Event);
        }

        // if (KeyEvent->key() == Qt::Key_Alt) {
        //     maybeMainWindow->optionCurrentlyPressed = false;
        // }

        // Qt::ControlModifier == CMD on MacOS
        // Qt::AltModifier     == OPT on MacOS
        // Qt::ShiftModifier   == SHIFT on MacOS

        // qDebug() << "Release: " << theKey << KeyEvent->nativeVirtualKey() << KeyEvent->nativeScanCode();

        if (maybeMainWindow->auditionPlaying && (KeyEvent->modifiers() & Qt::AltModifier)) {
            // qDebug() << "KeyRelease OPTION stopped Audition playback";
            maybeMainWindow->auditionPlaying = false;
            maybeMainWindow->auditionByKeyRelease(); // stop audition playback
            return true;
        }

        if (maybeMainWindow->auditionPlaying && theKey == Qt::Key_Slash) {
            // qDebug() << "KeyRelease SLASH stopped Audition playback";
            maybeMainWindow->auditionPlaying = false;
            maybeMainWindow->auditionByKeyRelease(); // stop audition playback
            return true;
        }

    } else if (Event->type() == QEvent::KeyPress) {

        QKeyEvent *KeyEvent = dynamic_cast<QKeyEvent *>(Event);
        int theKey = KeyEvent->key();

        // qDebug() << "Press: " << theKey << KeyEvent->nativeVirtualKey() << KeyEvent->nativeScanCode();

        // qDebug() << "eventFilter: theKey is " << theKey;
        MainWindow *maybeMainWindow = dynamic_cast<MainWindow *>((dynamic_cast<QApplication *>(Object))->activeWindow());
        if (maybeMainWindow == nullptr) {
            // if the PreferencesDialog is open, for example, do not dereference the NULL pointer (duh!).
//            qDebug() << "QObject::eventFilter()";
            return QObject::eventFilter(Object,Event);
        }

        // if (KeyEvent->key() == Qt::Key_Alt) {
        //     maybeMainWindow->optionCurrentlyPressed = true;
        // }

        // special handling for OPT-Slash, which is AUDITION HIGHLIGHTED ITEM
        if (theKey == Qt::Key_Slash) {
            if (KeyEvent->isAutoRepeat()) {
                return true;  // ignore auto-repeated slashes
            } else if ((KeyEvent->modifiers() & Qt::AltModifier) && !(KeyEvent->modifiers() & Qt::ControlModifier)) {
                // qDebug() << "KeyPress OPTION Slash started Audition playback";
                maybeMainWindow->auditionPlaying = true;
                maybeMainWindow->auditionByKeyPress(); // start audition playback
                return true;  // was exactly OPT-SLASH.
            }
            // else do regular processing with /, e.g. CMD-/, CMD-SHIFT-/, CMD-OPT-/
        }

//        qDebug() << "Key event: " << KeyEvent->key() << KeyEvent->modifiers() << ui->tableWidgetCurrentSequence->hasFocus();

        int cindex = ui->tabWidget->currentIndex();  // get index of tab, so we can see which it is
        bool tabIsCuesheet = (ui->tabWidget->tabText(cindex) == CUESHEET_TAB_NAME);
        bool tabIsSD = (ui->tabWidget->tabText(cindex) == "SD");
        // bool tabIsTaminations = (ui->tabWidget->tabText(cindex) == "Taminations");
        bool tabIsDarkMode = (ui->tabWidget->tabText(cindex) == "Music");

        bool cmdC_KeyPressed = (KeyEvent->modifiers() & Qt::ControlModifier) && KeyEvent->key() == Qt::Key_C;

//        qDebug() << "tabIsCuesheet: " << tabIsCuesheet << ", cmdC_KeyPressed: " << cmdC_KeyPressed <<
//                    "lyricsCopyIsAvailable: " << maybeMainWindow->lyricsCopyIsAvailable << "has focus: " << ui->textBrowserCueSheet->hasFocus();

        // if any of these widgets has focus, let them process the key
        //  otherwise, we'll process the key
        // UNLESS it's one of the search/timer edit fields and the ESC key is pressed (we must still allow
        //   stopping of music when editing a text field).  Sorry, can't use the SPACE BAR
        //   when editing a search field, because " " is a valid character to search for.
        //   If you want to do this, hit ESC to get out of edit search field mode, then SPACE.

        if (tabIsSD) {
//            qDebug() << "eventFilter: " << ui->tableWidgetCurrentSequence->hasFocus() << (theKey == Qt::Key_Left);

            // TODO: Move this stuff to handleSDFunctionKey()....
            if (ui->lineEditSDInput->hasFocus() && theKey == Qt::Key_Tab) {
                maybeMainWindow->do_sd_tab_completion();
                return true;
            } else if ((theKey >= Qt::Key_F1 && theKey <= Qt::Key_F12) ||
                       (!ui->lineEditSDInput->hasFocus() && !ui->sdCurrentSequenceTitle->hasFocus() && (theKey == Qt::Key_G || theKey == Qt::Key_B))
                       ) {
                // this has to come first.
                return (maybeMainWindow->handleSDFunctionKey(KeyEvent->keyCombination(), KeyEvent->text()));
            } else if (ui->tableWidgetCurrentSequence->hasFocus() && theKey == Qt::Key_Left) {
                // NOTE: for this to work, the LEFT ARROW must not be assigned to Move -15 sec in Hot Keys
                QKeyCombination combo1(KeyEvent->modifiers(), Qt::Key_F11); // LEFT --> F11, and SHIFT-LEFT --> SHIFT-F11, if on SD page
                return (maybeMainWindow->handleSDFunctionKey(combo1, QString("LEFT_ARROW")));
            } else if (ui->tableWidgetCurrentSequence->hasFocus() && theKey == Qt::Key_Right) {
                // NOTE: for this to work, the RIGHT ARROW must not be assigned to Move +15 sec in Hot Keys
                QKeyCombination combo2(KeyEvent->modifiers(), Qt::Key_F12); // RIGHT --> F12, and SHIFT-RIGHT --> SHIFT-F12, if on SD page
                return (maybeMainWindow->handleSDFunctionKey(combo2, QString("RIGHT_ARROW")));
            } else if (ui->tableWidgetCurrentSequence->hasFocus()) {
                // this has to be second.
                return QObject::eventFilter(Object,Event); // let the tableWidget handle UP/DOWN normally
            } else if (
                       (ui->boy1->hasFocus() || ui->boy2->hasFocus() || ui->boy3->hasFocus() || ui->boy4->hasFocus() ||
                       ui->girl1->hasFocus() || ui->girl2->hasFocus() || ui->girl3->hasFocus() || ui->girl4->hasFocus())
                       ) {
                // Handles TAB from field to field.
                // Arrows won't work (they move from sequence to sequence)
                // TODO: If ENTER is pressed, move to the next field in order.
                if (theKey == Qt::Key_Return) {
                    // ENTER moves between fields in TAB order, in a ring (Girl4 goes back to Boy1)
                    // Note: This is hard-coded, so TAB order and this code must be manually matched.
                    if (ui->boy1->hasFocus()) {
                        ui->girl1->setFocus();
                        ui->girl1->selectAll();
                    } else if (ui->girl1->hasFocus()) {
                        ui->boy2->setFocus();
                        ui->boy2->selectAll();
                    } else if (ui->boy2->hasFocus()) {
                        ui->girl2->setFocus();
                        ui->girl2->selectAll();
                    } else if (ui->girl2->hasFocus()) {
                        ui->boy3->setFocus();
                        ui->boy3->selectAll();
                    } else if (ui->boy3->hasFocus()) {
                        ui->girl3->setFocus();
                        ui->girl3->selectAll();
                    } else if (ui->girl3->hasFocus()) {
                        ui->boy4->setFocus();
                        ui->boy4->selectAll();
                    } else if (ui->boy4->hasFocus()) {
                        ui->girl4->setFocus();
                        ui->girl4->selectAll();
                    } else if (ui->girl4->hasFocus()) {
                        ui->boy1->setFocus();
                        ui->boy1->selectAll();
                    }
                    return(true);
                } else {
                    // TAB moves between fields in TAB order
                    return QObject::eventFilter(Object,Event); // let the lineEditWidget handle it normally
                }
            }
        } else if (tabIsCuesheet &&  // we're on the Lyrics editor tab
                 cmdC_KeyPressed &&      // When CMD-C is pressed
                 maybeMainWindow->lyricsCopyIsAvailable    // and the lyrics edit widget told us that copy was available
                 ) {
            ui->textBrowserCueSheet->copy();  // Then let's do the copy to clipboard manually anyway, even though we might not have focus
            return true;
        }
        else if (tabIsDarkMode && (theKey == Qt::Key_F) && (KeyEvent->modifiers() & Qt::ControlModifier)) {
            // CMD-F moves focus to the dark search field, if we're on the DarkMode tab
            ui->darkSearch->setFocus();
            ui->darkSearch->setSelection(0, ui->darkSearch->text().length());  // select the whole thing when CMD-F is pressed
        }
        else if ( !(
               // ui->labelSearch->hasFocus() ||      // IF NO TEXT HANDLING WIDGET HAS FOCUS...
               // ui->typeSearch->hasFocus() ||
               // ui->titleSearch->hasFocus() ||
               ui->darkSearch->hasFocus() ||
               (ui->textBrowserCueSheet->hasFocus() && ui->pushButtonEditLyrics->isChecked()) ||
               ui->dateTimeEditIntroTime->hasFocus() ||
               ui->dateTimeEditOutroTime->hasFocus() ||
               ui->darkStartLoopTime->hasFocus() ||
               ui->darkEndLoopTime->hasFocus() ||
               ui->lineEditSDInput->hasFocus() ||
#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
               ui->lineEditCountDownTimer->hasFocus() ||
               ui->lineEditChoreographySearch->hasFocus() ||
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
               // ui->songTable->isEditing() ||
               maybeMainWindow->someWebViewHasFocus() ) ||           // safe now (won't crash, if there are no webviews!)

             ( (
                // ui->labelSearch->hasFocus() ||          // OR IF A TEXT HANDLING WIDGET HAS FOCUS AND ESC/` IS PRESSED
                // ui->typeSearch->hasFocus() ||
                ui->darkSearch->hasFocus() ||
                // ui->titleSearch->hasFocus() ||

                ui->dateTimeEditIntroTime->hasFocus() ||
                ui->dateTimeEditOutroTime->hasFocus() ||
                     ui->darkStartLoopTime->hasFocus() ||
                     ui->darkEndLoopTime->hasFocus() ||
                ui->lineEditSDInput->hasFocus() || 
                ui->textBrowserCueSheet->hasFocus()) &&
                (theKey == Qt::Key_Escape
#if defined(Q_OS_MACOS)
                 // on MAC OS X, backtick is equivalent to ESC (e.g. devices that have TouchBar)
                 || theKey == Qt::Key_QuoteLeft
#endif
                                                          ) )  ||
                  // OR, IF ONE OF THE SEARCH FIELDS HAS FOCUS, AND RETURN/UP/DOWN_ARROW IS PRESSED
             ( (
                   // ui->labelSearch->hasFocus() ||
                   //   ui->typeSearch->hasFocus() ||
                     ui->darkSearch->hasFocus()
                     // ui->titleSearch->hasFocus()
               ) &&
               (theKey == Qt::Key_Return || theKey == Qt::Key_Up || theKey == Qt::Key_Down || theKey == Qt::Key_Left || theKey == Qt::Key_Right)
             )
                  // These next 3 help work around a problem where going to a different non-SDesk window and coming back
                  //   puts the focus into a the Type field, where there was NO FOCUS before.  But, that field usually doesn't
                  //   have any characters in it, so if the user types SPACE, the right thing happens, and it goes back to NO FOCUS.
                  // I think this is a reasonable tradeoff right now.
                  // OR, IF THE DARK TITLE SEARCH FIELD HAS FOCUS, AND IT HAS NO CHARACTERS OF TEXT YET, AND SPACE OR PERIOD IS PRESSED
                  || (ui->darkSearch->hasFocus() && ui->darkSearch->text().length() == 0 && (theKey == Qt::Key_Space || theKey == Qt::Key_Period))

                   // OR, IF THE LYRICS TAB SET INTRO FIELD HAS FOCUS, AND SPACE OR PERIOD IS PRESSED
                   || (ui->dateTimeEditIntroTime->hasFocus() && (theKey == Qt::Key_Space || theKey == Qt::Key_Period))
                   // OR, IF THE LYRICS TAB SET OUTRO FIELD HAS FOCUS, AND SPACE OR PERIOD IS PRESSED
                   || (ui->dateTimeEditOutroTime->hasFocus() && (theKey == Qt::Key_Space || theKey == Qt::Key_Period))
                   // OR, IF THE LYRICS TAB SET INTRO FIELD HAS FOCUS, AND SPACE OR PERIOD IS PRESSED
                   || (ui->darkStartLoopTime->hasFocus() && (theKey == Qt::Key_Space || theKey == Qt::Key_Period))
                   // OR, IF THE LYRICS TAB SET OUTRO FIELD HAS FOCUS, AND SPACE OR PERIOD IS PRESSED
                   || (ui->darkEndLoopTime->hasFocus() && (theKey == Qt::Key_Space || theKey == Qt::Key_Period))
           ) {
            // call handleKeypress on the Applications's active window ONLY if this is a MainWindow
//            qDebug() << "eventFilter SPECIAL KEY:" << ui << maybeMainWindow << theKey << KeyEvent->text();
            // THEN HANDLE IT AS A SPECIAL KEY

            if ((KeyEvent->modifiers() & Qt::ControlModifier) && (KeyEvent->modifiers() & Qt::ShiftModifier)) {
                // qDebug() << "Control + Shift:  theKey = " << theKey;
                switch (theKey) {
                    case Qt::Key_Up:        maybeMainWindow->PlaylistItemsMoveUp();           return true; break;
                    case Qt::Key_Down:      maybeMainWindow->PlaylistItemsMoveDown();         return true; break;
                    case Qt::Key_Left:      maybeMainWindow->PlaylistItemsToTop();            return true; break;
                    case Qt::Key_Right:     maybeMainWindow->PlaylistItemsToBottom();         return true; break;
                    case Qt::Key_Backspace: maybeMainWindow->PlaylistItemsRemove();           return true; break;
                    case Qt::Key_1:         maybeMainWindow->darkAddPlaylistItemsToBottom(0); return true; break; // THIS DOES NOT WORK YET except in certain contexts
                    case Qt::Key_2:         maybeMainWindow->darkAddPlaylistItemsToBottom(1); return true; break;
                    case Qt::Key_3:         maybeMainWindow->darkAddPlaylistItemsToBottom(2); return true; break;
                    default: break;
                }
            }
            // qDebug() << "KeyEvent->text() is " << KeyEvent->text();
            return (maybeMainWindow->handleKeypress(KeyEvent->key(), KeyEvent->text()));
        }

    }
    return QObject::eventFilter(Object,Event);
}

void MainWindow::actionTempoPlus()
{
    ui->darkTempoSlider->setValue(ui->darkTempoSlider->value() + 1);
    on_darkTempoSlider_valueChanged(ui->darkTempoSlider->value());
}

void MainWindow::actionTempoMinus()
{
    ui->darkTempoSlider->setValue(ui->darkTempoSlider->value() - 1);
    on_darkTempoSlider_valueChanged(ui->darkTempoSlider->value());
}

void MainWindow::actionFadeOutAndPause()
{
    cBass->FadeOutAndPause();
}

void MainWindow::actionNextTab()
{
    int currentTabNumber = ui->tabWidget->currentIndex();

    if (currentTabNumber == 0) {
        ui->tabWidget->setCurrentIndex(lyricsTabNumber);
    } else {
        ui->tabWidget->setCurrentIndex(0); // Dark Music tab (name: "Music")
    }
}

void MainWindow::actionSwitchToTab(const char *tabname)
{
    for (int tabnum = 0; tabnum < ui->tabWidget->count(); ++tabnum)
    {
        QString tabTitle = ui->tabWidget->tabText(tabnum);
        if (tabTitle.contains(tabname))
        {
            ui->tabWidget->setCurrentIndex(tabnum);
            return;
        }
    }
    qDebug() << "Could not find tab " << tabname;
}


void MainWindow::actionFilterSongsPatterSingersToggle()
{
    QString darkFilter = ui->darkSearch->text();
    qsizetype doubleColonIndex = darkFilter.indexOf("::");
    
    QString currentFilter(doubleColonIndex >= 0 ? darkFilter.mid(0, doubleColonIndex) : "");

    if (songTypeToggleList.length() > 1)
    {
        int nextToggle = 0;
        QList<QTreeWidgetItem *> trackItems = ui->treeWidget->findItems("Tracks", Qt::MatchExactly);
        QTreeWidgetItem *trackItem = trackItems[0];
        
        for (int i = 0; i < songTypeToggleList.length(); ++i)
        {
            QString s = songTypeToggleList[i];
            if (currentFilter.contains(s, Qt::CaseInsensitive))
            {
                nextToggle = i + 1;
            }
        }
        if (nextToggle >= songTypeToggleList.length()) {
            nextToggle = 0;
        }
        QString nextSearch = songTypeToggleList[nextToggle];
        for (int i = 0; i < trackItem->childCount(); ++i) {
            QTreeWidgetItem *item = trackItem->child(i);
            QString t = item->text(0);
            if (t.contains(nextSearch, Qt::CaseInsensitive)) {
                ui->treeWidget->setCurrentItem(item);
                break;
            }
        }
        
        return;
    }
    else
    {
        for (const QString &s : songTypeNamesForPatter)
        {
            if (0 == s.compare(currentFilter, Qt::CaseInsensitive))
            {
                actionFilterSongsToSingers();
                return;
            }
        }
    }
    actionFilterSongsToPatter();
}


void MainWindow::actionFilterSongsToPatter()
{
}

void MainWindow::actionFilterSongsToSingers()
{
}

void MainWindow::actionToggleCuesheetAutoscroll()
{
    // toggle automatic scrolling of the cuesheet
    ui->actionAuto_scroll_during_playback->setChecked(!ui->actionAuto_scroll_during_playback->isChecked());
}


// ----------------------------------------------------------------------
bool MainWindow::handleKeypress(int key, QString text)
{
    Q_UNUSED(text)
    QString tabTitle;

//    qDebug() << "handleKeypress(" << key << ")";
    if (inPreferencesDialog || !trapKeypresses || (prefDialog != nullptr)) {
        return false;
    }
//    qDebug() << "YES: handleKeypress(" << key << ")";

    switch (key) {

        case Qt::Key_Escape:
#if defined(Q_OS_MACOS)
        // on MAC OS X, backtick is equivalent to ESC (e.g. devices that have TouchBar)
        case Qt::Key_QuoteLeft:
#endif
            // ESC is special:  it always gets you out of editing a search field or timer field, and it can
            //   also STOP the music (second press, worst case)

            oldFocusWidget = nullptr;  // indicates that we want NO FOCUS on restore
            if (QApplication::focusWidget() != nullptr) {
                QApplication::focusWidget()->clearFocus();  // clears the focus from ALL widgets
            }
            oldFocusWidget = nullptr;  // indicates that we want NO FOCUS on restore, yes both of these are needed.

            // FIX: should we also stop editing of the songTable on ESC?

            ui->textBrowserCueSheet->clearFocus();  // ESC should always get us out of editing lyrics/patter

            // on_clearSearchButton_clicked(); // clears search fields, selects first visible item in songTable
            ui->darkSearch->setText("");
            ui->darkSearch->setFocus();  // When Clear Search is clicked (or ESC ESC), set focus to the darkSearch field, so that UP/DOWN works
            darkFilterMusic();               // highlights first visible row (if there are any rows)

            if (
                // ui->labelSearch->text() != "" ||
                // ui->typeSearch->text() != "" ||
                ui->darkSearch->text() != ""
                // ui->titleSearch->text() != ""
                ) {
                // clear the search fields, if there was something in them.  (First press of ESCAPE).
//                ui->labelSearch->setText("");
//                ui->typeSearch->setText("");
//                ui->titleSearch->setText("");

            } else {
                // if the search fields were already clear, then this is the second press of ESCAPE (or the first press
                //   of ESCAPE when the search function was not being used).  So, ONLY NOW do we Stop Playback.
                // So, GET ME OUT OF HERE is now "ESC ESC", or "Hit ESC a couple of times".
                //    and, CLEAR SEARCH is just ESC (or click on the Clear Search button).
                if (cBass->currentStreamState() == BASS_ACTIVE_PLAYING) {
                    on_darkPlayButton_clicked();  // we were playing, so PAUSE now.
                }
            }

            cBass->StopAllSoundEffects();  // and, it also stops ALL sound effects
            break;

        case Qt::Key_PageDown:
            // only move the scrolled Lyrics area, if the Lyrics tab is currently showing, and lyrics are loaded
            //   or if Patter is currently showing and patter is loaded
            tabTitle = ui->tabWidget->tabText(ui->tabWidget->currentIndex());
            if (tabTitle == CUESHEET_TAB_NAME) {
                ui->textBrowserCueSheet->verticalScrollBar()->setValue(ui->textBrowserCueSheet->verticalScrollBar()->value() + 200);
            }
            break;

        case Qt::Key_PageUp:
            // only move the scrolled Lyrics area, if the Lyrics tab is currently showing, and lyrics are loaded
            //   or if Patter is currently showing and patter is loaded
            tabTitle = ui->tabWidget->tabText(ui->tabWidget->currentIndex());
            if (tabTitle == CUESHEET_TAB_NAME) {
                ui->textBrowserCueSheet->verticalScrollBar()->setValue(ui->textBrowserCueSheet->verticalScrollBar()->value() - 200);
            }
            break;

        case Qt::Key_Space:
            // SPECIAL CASE: do the "Play/Pause Song" (SPACE) action defined in keyactions.h
            KeyAction::actionByName("Play/Pause Song")->doAction(this);
            break;

        case Qt::Key_Period:
            // SPECIAL CASE: do the "Restart Song" (PERIOD) action defined in keyactions.h
            KeyAction::actionByName("Restart Song")->doAction(this);
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (ui->darkSearch->hasFocus() || ui->darkSongTable->hasFocus()) {
            // else if (ui->darkSearch->hasFocus() || (darkSelectedSongRow() > 0)) {
                // also now allow pressing Return to load, if darkSongTable or darkSearch field have focus
                int row = darkSelectedSongRow();
                if (row < 0) {
                    // more than 1 row or no rows at all selected (BAD)
                    return true;
                }

                if (ui->darkSongTable->isRowHidden(row)) {
                    // if the selected row isn't even visible on the screen, ENTER has no effect.
                    return true;
                }

                on_darkSongTable_itemDoubleClicked(ui->darkSongTable->item(row,1)); // note: alters focus

                //                lastWidget->setFocus(); // restore focus to widget that had it before
                ui->darkSongTable->setFocus(); // THIS IS BETTER
            } else if (ui->playlist1Table->hasFocus() || ui->playlist2Table->hasFocus() || ui->playlist3Table->hasFocus()) {
//                qDebug() << "ENTER pressed for Playlist slot!  Time to load a song!";
//                QString songToLoad = ui->playlist1Table->fullPathOfSelectedSong() +
//                                     ui->playlist2Table->fullPathOfSelectedSong() +
//                                     ui->playlist3Table->fullPathOfSelectedSong();  // only one of these is not the null string
//                qDebug() << "songToLoad: " << songToLoad;

                if (ui->playlist1Table->hasFocus()) {
                    on_playlist1Table_itemDoubleClicked(ui->playlist1Table->selectedItems().at(0));
                } else if (ui->playlist2Table->hasFocus()) {
                    on_playlist2Table_itemDoubleClicked(ui->playlist2Table->selectedItems().at(0));
                } else if (ui->playlist3Table->hasFocus()) {
                    on_playlist3Table_itemDoubleClicked(ui->playlist3Table->selectedItems().at(0));
                }

            }
            break;

        case Qt::Key_Down:
        case Qt::Key_Up:
//            qDebug() << "Key up/down detected.";
            if (ui->darkSearch->hasFocus() || ui->darkSongTable->hasFocus()) {
                bool searchHasFocus = ui->darkSearch->hasFocus();
                if (key == Qt::Key_Up) {
                    int row = darkPreviousVisibleSongRow();
                    if (row < 0) {
                        // more than 1 row or no rows at all selected (BAD)
                        return true;
                    }
                    ui->darkSongTable->selectRow(row); // select new row!
                } else {
                    int row = darkNextVisibleSongRow();
                    if (row < 0) {
                        // more than 1 row or no rows at all selected (BAD)
                        return true;
                    }
                    ui->darkSongTable->selectRow(row); // select new row!
                }
                // now restore focus after selectRow()
                if (searchHasFocus) {
                    ui->darkSearch->setFocus();
                } else {
                    ui->darkSongTable->setFocus();
                }

            } else if (ui->playlist1Table->hasFocus() || ui->playlist2Table->hasFocus() || ui->playlist3Table->hasFocus()) {
//                qDebug() << "PLAYLIST HAS FOCUS, and UP/DOWN pressed...";
                if (key == Qt::Key_Up) {
                    ui->playlist1Table->moveSelectionUp();  // if this slot doesn't have anything selected, this call will be ignored.
                    ui->playlist2Table->moveSelectionUp();  // if this slot doesn't have anything selected, this call will be ignored.
                    ui->playlist3Table->moveSelectionUp();  // if this slot doesn't have anything selected, this call will be ignored.
                } else if (key == Qt::Key_Down){
                    ui->playlist1Table->moveSelectionDown();  // if this slot doesn't have anything selected, this call will be ignored.
                    ui->playlist2Table->moveSelectionDown();  // if this slot doesn't have anything selected, this call will be ignored.
                    ui->playlist3Table->moveSelectionDown();  // if this slot doesn't have anything selected, this call will be ignored.
                } else {
                    // nothing
                }
            }
            break;

    // Bluetooth Remote keys (mapped by Karabiner on Mac OS X to F18/19/20) -----------
    //    https://superuser.com/questions/554489/how-can-i-remap-a-play-button-keypress-from-a-bluetooth-headset-on-os-x
    //    https://pqrs.org/osx/karabiner/

    default:
//        default:
//            auto keyMapping = hotkeyMappings.find((Qt::Key)(key));
//            if (keyMapping != hotkeyMappings.end())
//            {
//                keyMapping.value()->doAction(this);
//            }
//            qDebug() << "unhandled key:" << key;
            break;
    }

    Info_Seekbar(true);
    return true;
}

// ------------------------------------------------------------------------
void MainWindow::on_actionSpeed_Up_triggered()
{
    ui->darkTempoSlider->setValue(ui->darkTempoSlider->value() + 1);
    on_darkTempoSlider_valueChanged(ui->darkTempoSlider->value());
}

void MainWindow::on_actionSlow_Down_triggered()
{
    ui->darkTempoSlider->setValue(ui->darkTempoSlider->value() - 1);
    on_darkTempoSlider_valueChanged(ui->darkTempoSlider->value());
}

// ------------------------------------------------------------------------
void MainWindow::on_actionSkip_Forward_triggered()
{
    cBass->StreamGetPosition();  // update the position
    // set the position to one second before the end, so that RIGHT ARROW works as expected
    cBass->StreamSetPosition(static_cast<int>(fmin(cBass->Current_Position + 10.0, cBass->FileLength-1.0)));
    Info_Seekbar(true);
}

void MainWindow::on_actionSkip_Backward_triggered()
{
    Info_Seekbar(true);
    cBass->StreamGetPosition();  // update the position
    cBass->StreamSetPosition(static_cast<int>(fmax(cBass->Current_Position - 10.0, 0.0)));
    Info_Seekbar(true);
}

// ------------------------------------------------------------------------
void MainWindow::on_actionVolume_Up_triggered()
{
    ui->darkVolumeSlider->setValue(ui->darkVolumeSlider->value() + 5);
}

void MainWindow::on_actionVolume_Down_triggered()
{
    ui->darkVolumeSlider->setValue(ui->darkVolumeSlider->value() - 5);
}

// ------------------------------------------------------------------------
void MainWindow::on_actionPlay_triggered()
{
    on_darkPlayButton_clicked();
}

void MainWindow::on_actionStop_triggered()
{
    on_darkStopButton_clicked();
}

// ------------------------------------------------------------------------
void MainWindow::on_actionForce_Mono_Aahz_mode_triggered()
{
    on_monoButton_toggled(ui->actionForce_Mono_Aahz_mode->isChecked());
}

void MainWindow::secondHalfOfLoad(QString songTitle) {
    // This function is called when the files is actually loaded into memory, and the filelength is known.
    // qDebug() << "***** secondHalfOfLoad()" << loadTimer.elapsed();

    // qDebug() << "secondHalfOfLoad: clearing the secondsPlayed for the loaded song";
    currentSongSecondsPlayed = 0; // reset the counter, because this is a new session
    currentSongSecondsPlayedRecorded = false; // not reported yet, because this is a new session

    // We are NOT doing automatic start-of-song finding right now.
    startOfSong_sec = 0.0;
    endOfSong_sec = cBass->FileLength;  // used by setDefaultIntroOutroPositions below

    // qDebug() << "***** secondHalfOfLoad(): " << startOfSong_sec << endOfSong_sec;

    // song is loaded now, so init the seekbar min/max (once)
    // InitializeSeekBar(ui->seekBar);
    InitializeSeekBar(ui->seekBarCuesheet);

    ui->darkSeekBar->setMinimum(0);
    ui->darkSeekBar->setMaximum(static_cast<int>(cBass->FileLength)-1); // tricky! see InitializeSeekBar


    Info_Seekbar(true);  // update the slider and all the text

    ui->dateTimeEditIntroTime->setTime(QTime(0,0,0,0));
    ui->darkStartLoopTime->setTime(QTime(0,0,0,0));

    ui->dateTimeEditOutroTime->setTime(QTime(23,59,59,0));
//    qDebug() << "second half of load!";
    ui->darkEndLoopTime->setTime(QTime(23,59,59,0));

    // ------------------------------------
    // let's do a quick preview (takes <1ms), to see if the intro/outro are already set.
    SongSetting settings1;

    songSettings.loadSettings(currentMP3filenameWithPath, settings1);
    bool isSetIntro1 = settings1.isSetIntroPos();
    bool isSetOutro1 = settings1.isSetOutroPos();

    uint32_t sampleRate = getMP3SampleRate(currentMP3filenameWithPath);

    // qDebug() << "secondHalfOfLoad checking ID3v2 tags ------";
    printID3Tags(currentMP3filenameWithPath);

    double bpm = 0.0;
    double tbpm = 0.0;
    uint32_t loopStartSamples = 0;
    uint32_t loopLengthSamples = 0;

    int result = readID3Tags(currentMP3filenameWithPath, &bpm, &tbpm, &loopStartSamples, &loopLengthSamples);
    // qDebug() << "secondHalfOfLoad result: " << result << loopStartSamples << loopLengthSamples << sampleRate;

    // enable the "Restore Loop Points from ID3v2 Tag" menu item ONLY if
    //   the LOOPLENGTH (in samples) is non-zero.  It's OK for the loopStart to be zero,
    //   since a loop can indeed start at the beginning of the song.
    //
    ui->actionIn_Out_Loop_points_to_default->setEnabled(loopLengthSamples != 0);

    if (sampleRate > 0 && (double)(cBass->FileLength) > 0.0 && result != -1 && loopLengthSamples != 0 && (!isSetIntro1 || !isSetOutro1)) {
        // There is a Music Provider loop available!
        //    AND the user has NOT set one or both of intro/outro yet
        // NOTE: it's OK (although uncommon) for the loopStart to be zero.
        // So, set the default loop to be the Music Provider's LOOPSTART/LOOPLENGTH

        double iFrac = ((double)loopStartSamples/(double)sampleRate)/(double)(cBass->FileLength);
        double oFrac = ((double)(loopStartSamples + loopLengthSamples)/(double)sampleRate)/(double)(cBass->FileLength);

        // qDebug() << "Music Provider loop available!" << loopStartSamples << loopLengthSamples << iFrac << oFrac;

        ui->seekBarCuesheet->SetIntro(iFrac);
        ui->seekBarCuesheet->SetOutro(oFrac);

        // if (darkmode) {
        qDebug() << "secondHalfOfLoad:" << iFrac;
        ui->darkSeekBar->setIntro(iFrac); // note lowercase 's'
        ui->darkSeekBar->setOutro(oFrac);
        // } else {
        //     ui->seekBar->SetIntro(iFrac);
        //     ui->seekBar->SetOutro(oFrac);
        // }
    } else {
        // The user has set Intro/Outro, OR the MP3 file did NOT contain LOOPSTART/LOOPLENGTH,
        //   OR there was a problem trying to read the ID3v2 tags, or this isn't an MP3 file.
        // So, let's go with a guess (algorithm in SetDefaultIntroOutroPositions)
        //
        // THIS IS A MYSLIDER
        ui->seekBarCuesheet->SetDefaultIntroOutroPositions(tempoIsBPM, cBass->Stream_BPM,
                                                           currentSongIsSinger || currentSongIsVocal,
                                                           startOfSong_sec, endOfSong_sec, cBass->FileLength);
        // set the defaults, but only for one of the two seekBars
        // if (darkmode) {
        // THIS IS A SVGWAVEFORMSLIDER
        ui->darkSeekBar->SetDefaultIntroOutroPositions(tempoIsBPM, cBass->Stream_BPM,
                                                       currentSongIsSinger || currentSongIsVocal,
                                                       startOfSong_sec, endOfSong_sec, cBass->FileLength);
        // } else {
        //     // THIS IS A MYSLIDER
        //     ui->seekBar->SetDefaultIntroOutroPositions(tempoIsBPM, cBass->Stream_BPM,
        //                                                currentSongIsSinger || currentSongIsVocal,
        //                                                startOfSong_sec, endOfSong_sec, cBass->FileLength);
        // }
    }

    // in case loadSettings fails (no settings on the very first load!), we need to set these edit fields
//    qDebug() << "*** secondHalfOfLoad now needs to set these default times into edit fields:" << ui->seekBarCuesheet->GetIntro() << ui->seekBarCuesheet->GetOutro();
    double introFrac = ui->seekBarCuesheet->GetIntro();
    double outroFrac = ui->seekBarCuesheet->GetOutro();
    double length = cBass->FileLength;

    QTime iTime = QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*introFrac*length+0.5));
    QTime oTime = QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*outroFrac*length+0.5));

    ui->dateTimeEditIntroTime->setTime(iTime); // milliseconds --> cuesheet edit fields
    ui->dateTimeEditOutroTime->setTime(oTime);

    if (darkmode) {
        ui->darkStartLoopTime->setTime(iTime); // milliseconds
        ui->darkEndLoopTime->setTime(oTime);   // milliseconds
    }

    // -------------------
    ui->dateTimeEditIntroTime->setEnabled(true);
    ui->dateTimeEditOutroTime->setEnabled(true);

    ui->pushButtonSetIntroTime->setEnabled(true);  // always enabled now, because anything CAN be looped now OR it has an intro/outro
    ui->pushButtonSetOutroTime->setEnabled(true);
    ui->pushButtonTestLoop->setEnabled(true);

    ui->darkStartLoopTime->setEnabled(true);
    ui->darkEndLoopTime->setEnabled(true);

    ui->darkStartLoopButton->setEnabled(true);  // always enabled now, because anything CAN be looped now OR it has an intro/outro
    ui->darkEndLoopButton->setEnabled(true);
    ui->darkTestLoopButton->setEnabled(true);
    ui->darkSegmentButton->setEnabled(true);

    cBass->SetVolume(100);
    currentVolume = 100;
    previousVolume = 100;
    Info_Volume();

//    qDebug() << "**** NOTE: currentSongTitle = " << currentSongTitle;
    // qDebug() << "secondHalfOfLoad is calling loadSettingsForSong: " << songTitle;

    // loadSettingsForSong will set Intro/Outro, if there was one saved in the settings
    //   This will override the defaults possibly set above.
    loadSettingsForSong(songTitle); // also loads replayGain, if song has one; also loads tempo from DB (not necessarily same as songTable if playlist loaded)

    loadGlobalSettingsForSong(songTitle); // sets global eq (e.g. Intelligibility Boost), AFTER song is loaded

    // NOTE: this needs to be down here, to override the tempo setting loaded by loadSettingsForSong()
    // This is a preference, e.g. to "[X] set all songs to 125BPM"
    bool tryToSetInitialBPM = prefsManager.GettryToSetInitialBPM();
    int initialBPM = prefsManager.GetinitialBPM();

   // qDebug() << "tryToSetInitialBPM: " << tryToSetInitialBPM << initialBPM;

    if (tryToSetInitialBPM && tempoIsBPM) {
//        qDebug() << "tryToSetInitialBPM overrides to: " << initialBPM;
        // if the user wants us to try to hit a particular BPM target, use that value
        //  iff the tempo is actually measured in BPM for this song
        // ui->tempoSlider->setValue(initialBPM);
        // emit ui->tempoSlider->valueChanged(initialBPM);  // fixes bug where second song with same BPM doesn't update songtable::tempo
    } else {
        // qDebug() << "using targetTempo" << targetTempo;
        if (targetTempo != "0" && targetTempo != "0%") {
            // iff tempo is known, then update the table
            QString tempo2 = targetTempo.replace("%",""); // if percentage (not BPM) just get rid of the "%" (setValue knows what to do)
            int tempoInt = tempo2.toInt();
            if (tempoInt != 0)
            {
                // ui->tempoSlider->setValue(tempoInt); // set slider to target BPM or percent, as per songTable (overriding DB)
            }
        }

        if (targetTempo == "0" && initialBPM != 0) { // 0 means "set to default value", this is used by initial Apple Music playlists, where tempo = 0 initially
            // qDebug() << "setting slider to default value";
            // ui->tempoSlider->setValue(ui->tempoSlider->GetOrigin());
            // emit ui->tempoSlider->valueChanged(ui->tempoSlider->GetOrigin());
        }
    }

//    qDebug() << "using targetPitch" << targetPitch;
    // int pitchInt = targetPitch.toInt();
    // ui->pitchSlider->setValue(pitchInt);
    // emit ui->pitchSlider->valueChanged(pitchInt); // make sure that the on value changed code gets executed, even if this isn't really a change.

//    qDebug() << "setting stream position to: " << startOfSong_sec;
    cBass->StreamSetPosition(startOfSong_sec);  // last thing we do is move the stream position to 1 sec before start of music

    songLoaded = true;  // now seekBar can be updated
    setInOutButtonState();
    loadingSong = false;

    // Update Now Playing info with new song metadata
    updateNowPlayingMetadata();

    // UPDATE THE WAVEFORM since load is complete! ---------------
//    qDebug() << "end of second half of load...";
//    ui->darkSeekBar->updateBgPixmap(waveform, WAVEFORMWIDTH);
    // qDebug() << "updateBgPixmap called from secondHalfOfLoad";
    if (ui->actionNormalize_Track_Audio->isChecked()) {
        ui->darkSeekBar->setWholeTrackPeak(cBass->GetWholeTrackPeak()); // scale the waveform
    } else {
        ui->darkSeekBar->setWholeTrackPeak(1.0); // don't scale the waveform
    }
    // qDebug() << "now updating BgPixmap";

    QString bulkDirname = musicRootPath + "/.squaredesk/bulk";

    QString WAVfilename = currentMP3filenameWithPath;
    WAVfilename.replace(musicRootPath, bulkDirname);
    QString resultsFilename = WAVfilename + ".results.txt";

    ui->darkSeekBar->setAbsolutePathToSegmentFile(resultsFilename);
    ui->darkSeekBar->updateBgPixmap(waveform, WAVEFORMSAMPLES);
    // ------------------
    if (ui->actionAutostart_playback->isChecked()) {
//        qDebug() << "----- AUTO START PRESSING PLAY, BECAUSE SONG IS NOW LOADED";
        on_darkPlayButton_clicked();
    }
}

void MainWindow::on_actionOpen_MP3_file_triggered()
{
    on_darkStopButton_clicked();  // if we're loading a new MP3 file, stop current playback

    saveCurrentSongSettings();

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    QString startingDirectory = prefsManager.Getdefault_dir();

    QString MP3FileName =
        QFileDialog::getOpenFileName(this,
                                     tr("Import Audio File"),
                                     startingDirectory,
                                     tr("Audio Files (*.mp3 *.m4a *.wav *.flac)"));
    if (MP3FileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
    QDir CurrentDir;
    prefsManager.Setdefault_dir(CurrentDir.absoluteFilePath(MP3FileName));

    // ui->songTable->clearSelection();  // if loaded via menu, then clear previous selection (if present)
    // ui->nextSongButton->setEnabled(false);  // and, next/previous song buttons are disabled
    // ui->previousSongButton->setEnabled(false);

    // --------
    loadMP3File(MP3FileName, QString(""), QString(""), QString(""));  // "" means use title from the filename
}


// this function stores the absolute paths of each file in a QVector
// finds music and cuesheets, but NOT Local playlists or Apple playlists

void MainWindow::checkLockFile() {
//    qDebug() << "checkLockFile()";
    QString musicRootPath = prefsManager.GetmusicPath();

    QString databaseDir(musicRootPath + "/.squaredesk");

    QFileInfo checkFile(databaseDir + "/lock.txt");
    if (checkFile.exists()) {

        // get the hostname of who is using it
        QFile file(databaseDir + "/lock.txt");
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream lockfile(&file);
        QString hostname = lockfile.readLine();
        file.close();

        QString myHostName = QHostInfo::localHostName();

        if (hostname != myHostName) {
            // there's probably another instance of SquareDesk somewhere, and we're sharing the database using a Cloud service
            hostname.replace(".local", ""); // let's make the hostname more friendly to the user
            QMessageBox msgBox(QMessageBox::Warning,
                               "TITLE",
                               QString("The SquareDesk database is currently being used by '") + hostname + QString("'.")
                               );
            msgBox.setInformativeText(QString("If you continue, any unsaved changes made by '") + hostname + QString("' might be lost."));
            QAbstractButton *continueButton =
                    msgBox.addButton(tr("&Continue anyway"), QMessageBox::AcceptRole);
            Q_UNUSED(continueButton)
            QAbstractButton *quitButton = msgBox.addButton(tr("&Quit"), QMessageBox::RejectRole);
            theSplash->close();  // if we have a lock problem, let's close the splashscreen before this dialog goes up,
                              //   so splashscreen doesn't cover up the dialog.

            msgBox.exec();
            if (msgBox.clickedButton() == quitButton) {
                exit(-1);
            }
            // else user clicked on "Continue Anyway" button
        } else {
            // probably a recent crash of SquareDesk on THIS device
            // so we're already locked.  Just return, since we already have the lock.
            return;
        }
    }

    // Lock file does NOT exist yet: create a new lock file with our hostname inside
    QFile file2(databaseDir + "/lock.txt");
    file2.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outfile(&file2);
//    qDebug() << "localHostName: " << QHostInfo::localHostName();
    outfile << QHostInfo::localHostName();
    file2.close();
}

void MainWindow::clearLockFile(QString musicRootPath) {
//    qDebug() << "clearLockFile()";
//    QString musicRootPath = prefsManager.GetmusicPath();

    QString databaseDir(musicRootPath + "/.squaredesk");
    QFileInfo checkFile(databaseDir + "/lock.txt");
    if (checkFile.exists()) {
        QFile file(databaseDir + "/lock.txt");
        file.remove();
    }
}



void MainWindow::updateTreeWidget() {

    // updateTreeWidget always rescans for playlists, so we need to clear pathStackPlaylists here.
    pathStackPlaylists->clear();

    // GET LIST OF TYPES AND POPULATE TREEWIDGET > TRACKS -------------
    QListIterator<QString> iter(*pathStack); // Tracks = search thru music (MP3, M4A) files

    QStringList types;

    while (iter.hasNext()) {
        QString s = iter.next();
        QString type1 = (s.replace(musicRootPath + "/", "").split("/"))[0];
        QStringList sl1 = type1.split("#!#");
        QString theType = sl1[0];  // the type (of original pathname, before following aliases)

        if (theType != "" && theType != "lyrics" && !theType.contains("$!$")) {  // screen OUT the Apple Music entries
//            types[theType] += 1;
            types.append(theType);
        }
    }

    types.removeDuplicates();
    types.sort(Qt::CaseInsensitive);  // sort them

//    qDebug() << "types: " << types;

    QTreeWidgetItem *tracksItem = new QTreeWidgetItem();
    tracksItem->setText(0, "Tracks");
//    tracksItem->setIcon(0, QIcon(":/graphics/darkiTunes.png"));
    tracksItem->setIcon(0, QIcon(":/graphics/icons8-musical-note-60.png"));

//    QMapIterator<QString, int> i(types);
    QStringListIterator i(types);
    while (i.hasNext()) {
        QString s = i.next();
        QTreeWidgetItem *trackTypeItem = new QTreeWidgetItem(tracksItem); // child of tracksItem
//        trackTypeItem->setText(0, i.key());
        trackTypeItem->setText(0, s);
    }

    ui->treeWidget->clear();
    ui->treeWidget->insertTopLevelItem(0, tracksItem);
    tracksItem->setExpanded(true);

    // --------------------------------------------------------------------
    // GET LIST OF LOCAL PLAYLISTS AND POPULATE TREEWIDGET > PLAYLISTS ----------
    QStringList playlists;

    QDirIterator it(musicRootPath + "/playlists", QStringList() << "*.csv", QDir::Files, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

    while(it.hasNext()) {
        QString thePath = it.next().replace(musicRootPath + "/", "");
        QString theType = (thePath.split("/"))[0];

//        qDebug() << "thePath/theType = " << theType << thePath;

        if (theType == "playlists") {
            QString thePath2 = thePath.replace("playlists/", "");
            if (thePath2.endsWith("csv")) {
                playlists.append(thePath2.replace(".csv", ""));
            }
        }
    }

    playlists.sort(Qt::CaseInsensitive);

    // qDebug() << "playlists:" << playlists;

    // top level Playlists item with icon
    QTreeWidgetItem *playlistsItem = new QTreeWidgetItem();
    playlistsItem->setText(0, "Playlists");
//    playlistsItem->setIcon(0, QIcon(":/graphics/darkPlaylists.png"));
    playlistsItem->setIcon(0, QIcon(":/graphics/icons8-menu-64.png"));
    ui->treeWidget->addTopLevelItem(playlistsItem);  // add this one to the tree
    playlistsItem->setExpanded(true);

    // insert filenames
    QTreeWidgetItem *topLevelItem = playlistsItem;
    foreach (const QString &fileName, playlists)
    {
        QStringList splitFileName = (QString("Playlists/") + fileName).split("/"); // tack on "Playlists/" so they will populate under the icon item

        // add root folder as top level item if treeWidget doesn't already have it
        if (ui->treeWidget->findItems(splitFileName[0], Qt::MatchFixedString).isEmpty())
        {
            topLevelItem = new QTreeWidgetItem;
            topLevelItem->setText(0, splitFileName[0]);
            ui->treeWidget->addTopLevelItem(topLevelItem);
        }

        QTreeWidgetItem *parentItem = topLevelItem;

        if (splitFileName.length() > 1) {
            // iterate through non-root directories (file name comes after)
            for (int i = 1; i < splitFileName.size() - 1; ++i)
            {
                // iterate through children of parentItem to see if this directory exists
                bool thisDirectoryExists = false;
                for (int j = 0; j < parentItem->childCount(); ++j)
                {
                    if (splitFileName[i] == parentItem->child(j)->text(0))
                    {
                            thisDirectoryExists = true;
                            parentItem = parentItem->child(j);
                            break;
                    }
                }

                if (!thisDirectoryExists)
                {
                    parentItem = new QTreeWidgetItem(parentItem);
                    parentItem->setText(0, splitFileName[i]);
                }
            }

            QTreeWidgetItem *childItem = new QTreeWidgetItem(parentItem);
            childItem->setText(0, splitFileName.last());
        }
    }

    // TODO: remove all of the local playlist entries from the pathStack

    // and replace them with:
    // Now, find all of the playlist entries, and stick the songs into the pathStack without Greek Xi
    foreach (const QString &fileName, playlists) {

        QString fullPathName = musicRootPath + "/playlists/" + fileName + ".csv";
        QFile file(fullPathName);
        // qDebug() << "fullPathName:" << fullPathName;
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString line;

            // header
            line = in.readLine();
            if (line == "relpath,pitch,tempo") {
                // qDebug() << "GOOD HEADER.";
            } else {
                continue; // skip playlists whose format we do not understand
            }

            int currentLineNumber = 1;
            while (!in.atEnd()) {
                line = in.readLine();
                QStringList SL = parseCSV(line);
                // relpath,pitch,tempo
                QString relpath = SL[0];
                QString fullPath = musicRootPath + relpath;
                QStringList relPathSplit = relpath.split("/");
                // int lastIndex = relPathSplit.size() - 1;
                QString extraZero = (currentLineNumber < 10 ? "0" : "");
                // relPathSplit[lastIndex] = extraZero + QString::number(currentLineNumber++)  + " - " + relPathSplit[lastIndex];
                // QString revisedrelpath = relPathSplit.join("/");
                // QString revisedFullPath = musicRootPath + revisedrelpath;
                QString pitch = SL[1];
                QString tempo = SL[2];
                // e.g. "SquareDeskPlaylistName%!%pitch,tempo,currentPlaylistLineNumber#!#FullPathname"
                QString pathStackEntry = fileName + "%!%" + pitch + "," + tempo + "," + extraZero + QString::number(currentLineNumber++) + "#!#" + fullPath;
                // qDebug() << pathStackEntry;
                pathStackPlaylists->append(pathStackEntry);
            }

            file.close();
        } else {
            // Handle file opening error
        }

    }

    // --------------------------------------------------------------------
    // GET LIST OF PLAYLISTS AND POPULATE TREEWIDGET > APPLE MUSIC ----------

    // if (allAppleMusicPlaylistNames.size() == 0) {
    //     return; // if no Apple Music playlists
    // }

    // qDebug() << "***** APPLE MUSIC";

    // top level Apple Music item with icon
    QTreeWidgetItem *appleMusicItem = new QTreeWidgetItem();
    appleMusicItem->setText(0, "Apple Music");
    appleMusicItem->setIcon(0, QIcon(":/graphics/icons8-apple-48.png"));

    foreach (const QString &playlistName, allAppleMusicPlaylistNames) {
                // qDebug() << "playlistName: " << playlistName;
                QTreeWidgetItem *childItem2 = new QTreeWidgetItem(appleMusicItem);
                childItem2->setText(0, playlistName);
    }

    ui->treeWidget->addTopLevelItem(appleMusicItem);  // add this one to the tree
    appleMusicItem->setExpanded(true);
}

void addStringToLastRowOfSongTable(QColor &textCol, MyTableWidget *songTable,
                                   QString str, int column)
{
    QTableWidgetItem *newTableItem;
    if (column == kNumberCol || column == kAgeCol || column == kPitchCol || column == kTempoCol) {
        newTableItem = new TableNumberItem( str.trimmed() );  // does sorting correctly for numbers
    } else if (column == kLabelCol) {
        newTableItem = new TableLabelItem( str.trimmed() );  // does sorting correctly for labels
    } else {
        newTableItem = new QTableWidgetItem( str.trimmed() );
    }
    newTableItem->setFlags(newTableItem->flags() & ~Qt::ItemIsEditable);      // not editable
    newTableItem->setForeground(textCol);
    if (column == kRecentCol || column == kAgeCol || column == kPitchCol || column == kTempoCol) {
        newTableItem->setTextAlignment(Qt::AlignCenter);
    }
    songTable->setItem(songTable->rowCount()-1, column, newTableItem);
}

static QString getTitleColText(MyTableWidget *songTable, int row)
{
    return dynamic_cast<QLabel*>(songTable->cellWidget(row,kTitleCol))->text();
}

QString getTitleColTitle(MyTableWidget *songTable, int row)
{
    QString title = getTitleColText(songTable, row);

    title.replace(spanPrefixRemover, "\\1"); // remove <span style="color:#000000"> and </span> title string coloring

    int where = title.indexOf(title_tags_remover);
    if (where >= 0) {
        title.truncate(where);
    }

    title.replace("&quot;","\"").replace("&amp;","&").replace("&gt;",">").replace("&lt;","<");  // if filename contains HTML encoded chars, put originals back

    return title;
}

// --------------------------------------------------------------------------------
QString MainWindow::FormatTitlePlusTags(const QString &title, bool setTags, const QString &strtags, QString titleColor)
{
    // qDebug() << "FormatTitlePlusTags:" << title << setTags << strtags << titleColor;
    QString titlePlusTags(title.toHtmlEscaped());

    // if the titleColor is not explicitly specified (defaults to ""), do not add a <span> to specify color.
    // but if specified, surround the title with a span that specifies its color.
    if (titleColor != "") {
        titlePlusTags = QString("<span style=\"color: %1;\">").arg(titleColor) + titlePlusTags + QString("</span>");
//        qDebug() << "titlePlusTags: " << titlePlusTags;
    }

    if (setTags && !strtags.isEmpty() && ui->actionViewTags->isChecked())
    {
        titlePlusTags += "&nbsp;";
        QStringList tags = strtags.split(" ");
        for (const auto &tag : tags)
        {
            QString tagTrimmed(tag.trimmed());
            if (tagTrimmed.length() > 0)
            {
                QPair<QString,QString> color = songSettings.getColorForTag(tag);
//                QString prefix = title_tags_prefix.arg(color.first).arg(color.second);
                QString prefix = title_tags_prefix.arg(color.first, color.second);
                titlePlusTags +=  prefix + tag.toHtmlEscaped() + title_tags_suffix;
            }
        }
    }
    return titlePlusTags;
}

// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// filter from a pathStack into the darkSongTable, BUT
//   nullptr: just refresh what's there (currentlyShowingPathStack)
//   pathStack, pathStackPlaylists: use one of these
//
// Note: if aPathStack == currentShowingPathStack, then don't do anything
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

static void addToProgramsAndWriteTextFile(QStringList &programs, QDir outputDir,
                                   const char *filename,
                                   const char *fileLines[])
{
    // write the program file IFF it doesn't already exist
    // this allows users to modify the files manually, and they
    //   won't get overwritten by us at startup time.
    // BUT, if there are updates from us, the user will now have to manually
    //   delete those files, so that they will be recreated from the newer versions.

    QString outputFile = outputDir.canonicalPath() + "/" + filename;

    QFileInfo info(outputFile);
    if (!info.exists()) {
        QFile file(outputFile);
        if (file.open(QIODevice::ReadWrite)) {
            QTextStream outStream(&file);
            for (int i = 0; fileLines[i]; ++i)
            {
                outStream << fileLines[i] << ENDL;  // write to file
            }
            programs << outputFile; // and append to programs list
        }
    } else {
        programs << outputFile; // just append filename to programs list
    }
}

void MainWindow::loadDanceProgramList(QString lastDanceProgram)
{
    ui->comboBoxCallListProgram->clear();
    QListIterator<QString> iter(*pathStack); // I guess Reference items are in the pathStack too
    QStringList programs;

    // FIX: This should be changed to look only in <rootDir>/reference, rather than looking
    //   at all pathnames in the <rootDir>.  It will be much faster.
    while (iter.hasNext()) {
        QString s = iter.next();

        // Dance Program file names must begin with 0 then exactly 2 numbers followed by a dot, followed by the dance program name, dot text.
//        if (QRegularExpression("reference/0[0-9][0-9]\\.[a-zA-Z0-9' ]+\\.txt$", Qt::CaseInsensitive).indexIn(s) != -1)  // matches the Dance Program files in /reference
        static QRegularExpression re1("reference/0[0-9][0-9]\\.[a-zA-Z0-9' ]+\\.txt$", QRegularExpression::CaseInsensitiveOption);
        if (s.indexOf(re1) != -1)  // matches the Dance Program files in /reference
        {
            //qDebug() << "Dance Program Match:" << s;
            QStringList sl1 = s.split("#!#");
//            QString type = sl1[0];  // the type (of original pathname, before following aliases)
            QString origPath = sl1[1];  // everything else
            QFileInfo fi(origPath);
            if (fi.dir().canonicalPath().endsWith("/reference", Qt::CaseInsensitive))
            {
                programs << origPath;
            }
        }
    }

    if (programs.length() == 0)
    {
        QString referencePath = musicRootPath + "/reference";
        QDir outputDir(referencePath);
        // if (!outputDir.exists())
        // {
        //     outputDir.mkpath(".");
        // }

        addToProgramsAndWriteTextFile(programs, outputDir, "010.basic1.txt",    danceprogram_basic1);
        addToProgramsAndWriteTextFile(programs, outputDir, "020.basic2.txt",    danceprogram_basic2);
        addToProgramsAndWriteTextFile(programs, outputDir, "025.SSD.txt",       danceprogram_SSD);
        addToProgramsAndWriteTextFile(programs, outputDir, "030.mainstream.txt", danceprogram_mainstream);
        addToProgramsAndWriteTextFile(programs, outputDir, "040.plus.txt",      danceprogram_plus);
        addToProgramsAndWriteTextFile(programs, outputDir, "050.a1.txt",        danceprogram_a1);
        addToProgramsAndWriteTextFile(programs, outputDir, "060.a2.txt",        danceprogram_a2);
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

void MainWindow::titleLabelDoubleClicked(QMouseEvent * /* event */)
{    
}

void MainWindow::darkTitleLabelDoubleClicked(QMouseEvent * /* event */)
{
    int row = darkSelectedSongRow();
    if (row >= 0) {
        on_darkSongTable_itemDoubleClicked(ui->darkSongTable->item(row, kPathCol));
    } else {
        // more than 1 row or no rows at all selected (BAD)
    }
}

void MainWindow::on_actionClear_Search_triggered()
{
    ui->darkSearch->setText("");
    ui->darkSearch->setFocus();  // When Clear Search is clicked (or ESC ESC), set focus to the darkSearch field, so that UP/DOWN works

    // on_clearSearchButton_clicked();
}

void MainWindow::on_actionPitch_Up_triggered()
{
    ui->darkPitchSlider->setValue(ui->darkPitchSlider->value() + 1);
}

void MainWindow::on_actionPitch_Down_triggered()
{
    ui->darkPitchSlider->setValue(ui->darkPitchSlider->value() - 1);
}

void MainWindow::on_actionAutostart_playback_triggered()
{
    // the Autostart on Playback mode setting is persistent across restarts of the application
    prefsManager.Setautostartplayback(ui->actionAutostart_playback->isChecked());
}

void MainWindow::on_actionImport_triggered()
{
    RecursionGuard dialog_guard(inPreferencesDialog);
    // qDebug() << "IMPORT TRIGGERED";
 
    ImportDialog *importDialog = new ImportDialog();
    setDynamicPropertyRecursive(importDialog, "theme", currentThemeString);

    int dialogCode = importDialog->exec();
    RecursionGuard keypress_guard(trapKeypresses);
    if (dialogCode == QDialog::Accepted)
    {
        importDialog->importSongs(songSettings, pathStack); // insert into pathStack for SONGS
        darkLoadMusicList(pathStack, currentTypeFilter, true, true);
    }
    delete importDialog;
    importDialog = nullptr;
}

void MainWindow::on_actionExport_Play_Data_triggered()
{
    RecursionGuard dialog_guard(inPreferencesDialog);

    SongHistoryExportDialog *dialog = new SongHistoryExportDialog();
    setDynamicPropertyRecursive(dialog, "theme", currentThemeString);

    dialog->populateOptions(songSettings);
    int dialogCode = dialog->exec();
    RecursionGuard keypress_guard(trapKeypresses);
    if (dialogCode == QDialog::Accepted)
    {
        QString lastExportSaveHistoryDir = prefsManager.MySettings.value("lastExportSaveHistoryDir").toString();
        lastExportSaveHistoryDir = dialog->exportSongPlayData(songSettings, lastExportSaveHistoryDir);
        if (lastExportSaveHistoryDir != "") {
            prefsManager.MySettings.setValue("lastExportSaveHistoryDir", lastExportSaveHistoryDir);
        }
    }
    delete dialog;
    dialog = nullptr;
}

void MainWindow::on_actionExport_Current_Song_List_triggered()
{
    // Ask me where to save it...
    RecursionGuard dialog_guard(inPreferencesDialog);

    QString startingPath = QDir::homePath() + "/currentSongList.csv";
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Export Current Song List"),
                                                    startingPath,
                                                    tr("CSV (*.csv)"));

    if (!filename.isNull())
    {
        QFile CSVfile(filename);
        CSVfile.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream outfile(&CSVfile);
        outfile << "type,label,title,tags\n"; // HEADER ----------

        for (int row=0; row < ui->darkSongTable->rowCount(); row++) {
            if (!ui->darkSongTable->isRowHidden(row)) {
                QString songTitle = getTitleColTitle(ui->darkSongTable, row).trimmed();
                songTitle.replace("\"", "\"\"");  // Titles can have double quotes in them, must quote them
                QString songType = ui->darkSongTable->item(row,kTypeCol)->text().toLower().trimmed();
                QString songLabel = ui->darkSongTable->item(row,kLabelCol)->text().toUpper().trimmed();

                QString pathToMP3 = ui->darkSongTable->item(row,kPathCol)->data(Qt::UserRole).toString();
                pathToMP3.replace(musicRootPath,"");

                SongSetting settings;
                songSettings.loadSettings(pathToMP3, settings);

                // qDebug() << songTitle << songType << songLabel << pathToMP3 << settings.getTags();

                const QString dq = "\""; // double quote

                // WRITE A SINGLE ROW ------------
                outfile << dq << songType << dq << ",";
                outfile << dq << songLabel << dq << ",";
                outfile << dq << songTitle << dq << ",";
                if (settings.isSetTags()) {
                    outfile << dq << settings.getTags() << dq << "\n";  // HAS TAGS (which never themselves contain double quotes)
                } else {
                    outfile << dq << dq << "\n";                        // DOES NOT HAVE ANY TAGS
                }
            }
        }

        CSVfile.close();
    }
}

void MainWindow::on_actionExport_triggered()
{
    RecursionGuard dialog_guard(inPreferencesDialog);

    if (/* DISABLES CODE */ (true))
    {
        QString filename =
            QFileDialog::getSaveFileName(this, tr("Select Export File"),
                                         QDir::homePath(),
                                         tr("Comma Separated (*.csv);;Tab Separated (*.tsv)")); // defaults to CSV
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

                exportSongList(stream, songSettings, pathStack,  // export list of SONGS only
                               outputFieldCount, outputFields,
                               separator,
                               true, true, songTypeNamesForSinging + songTypeNamesForCalled + songTypeNamesForPatter); // export using RELATIVE pathnames only, so it's portable
            }
        }
    }
    else
    {
        // THIS CODE IS OBSOLETE NOW
        ExportDialog *exportDialog = new ExportDialog();
        setDynamicPropertyRecursive(exportDialog, "theme", currentThemeString);

        int dialogCode = exportDialog->exec();
        RecursionGuard keypress_guard(trapKeypresses);
        if (dialogCode == QDialog::Accepted)
        {
            exportDialog->exportSongs(songSettings, pathStack);
        }
        delete exportDialog;
        exportDialog = nullptr;
    }
}

void MainWindow::AddHotkeyMappingsFromMenus(QHash<QString, KeyAction *> &hotkeyMappings)
{
    // Add the menu hotkeys back in.
    for (auto binding = keybindingActionToMenuAction.cbegin(); binding != keybindingActionToMenuAction.cend(); ++binding)
    {
        if (binding.value() && !hotkeyMappings.contains(binding.value()->shortcut().toString()))
        {
            hotkeyMappings[binding.value()->shortcut().toString()] =
                KeyAction::actionByName(binding.key());
        }
    }    
}

void MainWindow::AddHotkeyMappingsFromShortcuts(QHash<QString, KeyAction *> &hotkeyMappings)
{
    for (auto hotkey = hotkeyShortcuts.cbegin(); hotkey != hotkeyShortcuts.cend(); ++hotkey)
    {
        QVector<QShortcut *> shortcuts(hotkey.value());
        for (int keypress = 0; keypress < shortcuts.length(); ++keypress)
        {
            QShortcut *shortcut = shortcuts[keypress];
            if (shortcut->isEnabled())
            {
                hotkeyMappings[shortcut->key().toString()] = KeyAction::actionByName(hotkey.key());
            }
        }
    }
}

// --------------------------------------------------------
void MainWindow::on_actionPreferences_triggered()
{
    RecursionGuard dialog_guard(inPreferencesDialog);
    trapKeypresses = false;
//    on_stopButton_clicked();  // stop music, if it was playing...
    QHash<QString, KeyAction *> hotkeyMappings;

    AddHotkeyMappingsFromShortcuts(hotkeyMappings);
    AddHotkeyMappingsFromMenus(hotkeyMappings);

    prefDialog = new PreferencesDialog(&soundFXname, this);

    setDynamicPropertyRecursive(prefDialog, "theme", currentThemeString);

    prefsManager.SetHotkeyMappings(hotkeyMappings);
    prefsManager.setTagColors(songSettings.getTagColors());
    prefsManager.populatePreferencesDialog(prefDialog);
    prefDialog->songTableReloadNeeded = false;  // nothing has changed...yet.

    prefDialog->swallowSoundFX = false;  // OK to play soundFX now

    SessionDefaultType previousSessionDefaultType =
        static_cast<SessionDefaultType>(prefsManager.GetSessionDefault());

    prefDialog->setSessionInfoList(songSettings.getSessionInfo());

    // modal dialog
    int dialogCode = prefDialog->exec();
    trapKeypresses = true;

    QString oldMusicRootPath = prefsManager.GetmusicPath();

    // act on dialog return code
    if(dialogCode == QDialog::Accepted) {
        // OK clicked
        // Save the new value for musicPath --------
        prefsManager.extractValuesFromPreferencesDialog(prefDialog);
        songSettings.setTagColors(prefsManager.getTagColors());
        QHash<QString, KeyAction *> hotkeyMappings = prefsManager.GetHotkeyMappings();
        SetKeyMappings(hotkeyMappings, hotkeyShortcuts);
        songSettings.setDefaultTagColors( prefsManager.GettagsBackgroundColorString(), prefsManager.GettagsForegroundColorString());

        QString newMusicRootPath = prefsManager.GetmusicPath();

        if (oldMusicRootPath != newMusicRootPath) {
//            qDebug() << "MUSIC ROOT PATH CHANGED!";
            // before we actually make the change to the music root path,

            // check for unsaved playlists that have been modified, and ask to Save As... each one in turn
            for (int i = 0; i < 3; i++) {
                if (!maybeSavePlaylist(i)) {
                    //        qDebug() << "closeEvent ignored, because user cancelled.";
                    continue; // even if CANCELed, we'll ask for each one (this is non-optimal, but OK for this use case, which is rare)
                }
            }
            clearSlot(0);    // then clear out the slot, mark not modified, and no relPathInSlot
            clearSlot(1);    // then clear out the slot, mark not modified, and no relPathInSlot
            clearSlot(2);    // then clear out the slot, mark not modified, and no relPathInSlot

            // and do NOT reload these slots when app restarts
            prefsManager.SetlastPlaylistLoaded("");
            prefsManager.SetlastPlaylistLoaded2("");
            prefsManager.SetlastPlaylistLoaded3("");

            // if the cuesheet was edited, allow last minute save
            maybeSaveCuesheet(2);  // 2 options: don't save or save, no cancel

            // 1) release the lock on the old music directory (if it exists), before we change the music root path
            clearLockFile(oldMusicRootPath);

            musicRootPath = prefsManager.GetmusicPath(); // FROM HERE ON IN, IT'S THE NEW MUSICDIR ---------

            // 2) take a lock on the new music directory
            checkLockFile();

            maybeMakeAllRequiredFolders(); // make all the required folders (if needed) in the NEW musicDir

            maybeInstallSoundFX();
            maybeInstallReferencefiles();

            // 3) TODO: clear out the cuesheet from the UI
            // 4) TODO: clear out the reference tab from the UI and reload
        }

        // USER SAID "OK", SO HANDLE THE UPDATED PREFS ---------------
        musicRootPath = prefsManager.GetmusicPath();

        songSettings.setSessionInfo(prefDialog->getSessionInfoList());
        // qDebug() << "preferences set lastsessionid to -1";
        lastSessionID = -1;  // invalidate the lastSessionID cached value, forcing reevaluation of which session we are in...

        if (previousSessionDefaultType != static_cast<SessionDefaultType>(prefsManager.GetSessionDefault())
            && static_cast<SessionDefaultType>(prefsManager.GetSessionDefault()) == SessionDefaultDOW)
        {
            setCurrentSessionIdReloadSongAgesCheckMenu(songSettings.currentSessionIDByTime()); // TODO: Do we need this now?
        }
        populateMenuSessionOptions();

        findMusic(musicRootPath, true); // always refresh the songTable after the Prefs dialog returns with OK
        switchToLyricsOnPlay = prefsManager.GetswitchToLyricsOnPlay();

        // Save the new value for music type colors --------
        patterColorString = prefsManager.GetpatterColorString();
        singingColorString = prefsManager.GetsingingColorString();
        calledColorString = prefsManager.GetcalledColorString();
        extrasColorString = prefsManager.GetextrasColorString();

        if (prefsManager.GetSwapSDTabInputAndAvailableCallsSides() != sdSliderSidesAreSwapped)
        {
            // if they are in the wrong swappage, swap them.  This is a toggle.
            sdSliderSidesAreSwapped = prefsManager.GetSwapSDTabInputAndAvailableCallsSides();
            ui->splitterSDTabHorizontal->addWidget(ui->splitterSDTabHorizontal->widget(0));
        }

        // ----------------------------------------------------------------
        // Show the Lyrics tab, if it is enabled now
//        lyricsTabNumber = (showTimersTab ? 2 : 1);

        ui->tabWidget->setTabText(lyricsTabNumber, CUESHEET_TAB_NAME);

        // -----------------------------------------------------------------------
        // Save the new settings for experimental break and patter timers --------
        tipLengthTimerEnabled = prefsManager.GettipLengthTimerEnabled();  // save new settings in MainWindow, too
        tipLength30secEnabled = prefsManager.GettipLength30secEnabled();
        tipLengthTimerLength = prefsManager.GettipLengthTimerLength();
        tipLengthAlarmAction = prefsManager.GettipLengthAlarmAction();

        breakLengthTimerEnabled = prefsManager.GetbreakLengthTimerEnabled();
        breakLengthTimerLength = prefsManager.GetbreakLengthTimerLength();
        breakLengthAlarmAction = prefsManager.GetbreakLengthAlarmAction();

        // and tell the clock, too.
        ui->theSVGClock->tipLengthTimerEnabled = tipLengthTimerEnabled;
        ui->theSVGClock->tipLength30secEnabled = tipLength30secEnabled;
        ui->theSVGClock->tipLengthAlarmMinutes = tipLengthTimerLength;
        ui->theSVGClock->tipLengthAlarm = tipLengthAlarmAction;

        ui->theSVGClock->breakLengthTimerEnabled = breakLengthTimerEnabled;
        ui->theSVGClock->breakLengthAlarmMinutes = breakLengthTimerLength;

        // ----------------------------------------------------------------
        // Save the new value for experimentalClockColoringEnabled --------
        clockColoringHidden = !prefsManager.GetexperimentalClockColoringEnabled();
        // analogClock->setHidden(clockColoringHidden);
        ui->theSVGClock->setHidden(clockColoringHidden);

        SetAnimationSpeed(static_cast<AnimationSpeed>(prefsManager.GetAnimationSpeed()));
        
        {
            QString value;
            value = prefsManager.GetMusicTypeSinging();
            songTypeNamesForSinging = value.toLower().split(";", Qt::KeepEmptyParts);

            value = prefsManager.GetMusicTypePatter();
            songTypeNamesForPatter = value.toLower().split(";", Qt::KeepEmptyParts);

            value = prefsManager.GetMusicTypeExtras();
            songTypeNamesForExtras = value.toLower().split(";", Qt::KeepEmptyParts);

            value = prefsManager.GetMusicTypeCalled();
            songTypeNamesForCalled = value.split(";", Qt::KeepEmptyParts);

            value = prefsManager.GetToggleSingingPatterSequence();
            songTypeToggleList = value.toLower().split(';', Qt::KeepEmptyParts);
        }
        songFilenameFormat = static_cast<enum SongFilenameMatchingType>(prefsManager.GetSongFilenameFormat());

        if (prefDialog->songTableReloadNeeded) {
//            qDebug() << "LOAD MUSIC LIST TRIGGERED FROM PREFERENCES TRIGGERED";

            darkLoadMusicList(nullptr, currentTypeFilter, true, true); // just refresh whatever is there
            reloadPaletteSlots();
        }

        if (prefsManager.GetenableAutoAirplaneMode()) {
            // if the user JUST set the preference, turn Airplane Mode on RIGHT NOW (radios OFF).
            airplaneMode(true);
        } else {
            // if the user JUST set the preference, turn Airplane Mode OFF RIGHT NOW (radios ON).
            airplaneMode(false);
        }

        // minimumVolume = prefsManager.GetlimitVolume(); // volume can't go lower than this!
        // if (ui->volumeSlider->value() < minimumVolume) {
        //     // coming out of the prefs dialog, if we set the min pref to be lower than current vol, fix the slider.
        //     ui->volumeSlider->setValue(minimumVolume);
        // }
    }
    setInOutButtonState();

    delete prefDialog;
    prefDialog = nullptr;
}

QString MainWindow::removePrefix(QString prefix, QString s)
{
    QString s2 = s.remove( prefix );
    return s2;
}

bool MainWindow::compareRelative(QString relativePlaylistPath, QString absolutePathstackPath) {
    // compare paths with MusicDir stripped off of the left (relative compare to root of MusicDir)
    QString stripped2 = absolutePathstackPath.replace(musicRootPath, "");
//    return(relativePlaylistPath == stripped2);
    return(QString::compare(relativePlaylistPath, stripped2, Qt::CaseInsensitive) == 0); // 0 if they are equal -> return true
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
int MainWindow::selectedSongRow()
{
    return(0);
}

int MainWindow::darkSelectedSongRow()
{
    QItemSelectionModel *selectionModel = ui->darkSongTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;

    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } // else more than 1 row or no rows, just return -1
    return row;
}

// Return the previous visible song row if just one selected, else -1
int MainWindow::previousVisibleSongRow()
{
    return(0);
}

// Return the next visible song row if just one selected, else -1
int MainWindow::nextVisibleSongRow() {
    return(0);
}

// Return the previous visible song row if just one selected, else -1
int MainWindow::darkPreviousVisibleSongRow()
{
    int row = darkSelectedSongRow();
    if (row < 0) {
        // more than 1 row or no rows at all selected (BAD)
        return row;
    }

    // which is the next VISIBLE row?
    int lastVisibleRow = row;
    row = (row-1 < 0 ? 0 : row-1); // bump backwards by 1

    while (ui->darkSongTable->isRowHidden(row) && row > 0) {
        // keep bumping backwards, until the previous VISIBLE row is found, or we're at the BEGINNING
        row = (row-1 < 0 ? 0 : row-1); // bump backwards by 1
    }
    if (ui->darkSongTable->isRowHidden(row)) {
        // if we try to go past the beginning of the VISIBLE rows, stick at the first visible row (which
        //   was the last one we were on.  Well, that's not always true, but this is a quick and dirty
        //   solution.  If I go to a row, select it, and then filter all rows out, and hit one of the >>| buttons,
        //   hilarity will ensue.
        row = lastVisibleRow;
    }
    return row;
}

// Return the next visible song row if just one selected, else -1
int MainWindow::darkNextVisibleSongRow() {
    int row = darkSelectedSongRow();
    if (row < 0) {
        return row;
    }

    int maxRow = ui->darkSongTable->rowCount() - 1;

    // which is the next VISIBLE row?
    int lastVisibleRow = row;
    row = (maxRow < row+1 ? maxRow : row+1); // bump up by 1
    while (ui->darkSongTable->isRowHidden(row) && row < maxRow) {
        // keep bumping, until the next VISIBLE row is found, or we're at the END
        row = (maxRow < row+1 ? maxRow : row+1); // bump up by 1
    }
    if (ui->darkSongTable->isRowHidden(row)) {
        // if we try to go past the end of the VISIBLE rows, stick at the last visible row (which
        //   was the last one we were on.  Well, that's not always true, but this is a quick and dirty
        //   solution.  If I go to a row, select it, and then filter all rows out, and hit one of the >>| buttons,
        //   hilarity will ensue.
        row = lastVisibleRow;
    }

    return row;
}

// END OF PLAYLIST SECTION ================

void MainWindow::changeTagOnCurrentSongSelection(QString tag, bool add)
{
    Q_UNUSED(tag)
    Q_UNUSED(add)
}

void MainWindow::darkChangeTagOnCurrentSongSelection(QString tag, bool add)
{
    int row = darkSelectedSongRow();
    if (row >= 0) {
        // exactly 1 row was selected (good)
        QString pathToMP3 = ui->darkSongTable->item(row,kPathCol)->data(Qt::UserRole).toString();
        SongSetting settings;
        songSettings.loadSettings(pathToMP3, settings);
        QStringList tags;
        QString oldTags;
        if (settings.isSetTags())
        {
            oldTags = settings.getTags();
            tags = oldTags.split(" ");
            songSettings.removeTags(oldTags);
        }

        // if (add && !tags.contains(tag, Qt::CaseInsensitive))
        if (add && !tags.contains(tag))
            tags.append(tag);
        if (!add)
        {
            // int i = tags.indexOf(tag, Qt::CaseInsensitive);
            int i = tags.indexOf(tag);
            if (i >= 0)
                tags.removeAt(i);
        }
        settings.setTags(tags.join(" "));
        songSettings.saveSettings(pathToMP3, settings);

        // extract the text color of the title
        QString title1 = getTitleColText(ui->darkSongTable, row); // e.g. "<span style=\"color: #7963ff;\">'Cuda NEW</span>"
        static QRegularExpression re("^<span style=[\"]color: (.*?);");
        QRegularExpressionMatch match = re.match(title1);

        QString theOriginalColor = "";
        if (match.hasMatch()) {
            theOriginalColor = match.captured(1);
        }

        QString title = getTitleColTitle(ui->darkSongTable, row);
        // we know for sure that this item is selected (because that's how we got here), so let's highlight text color accordingly
        QString titlePlusTags(FormatTitlePlusTags(title, settings.isSetTags(), settings.getTags(), theOriginalColor));

        dynamic_cast<QLabel*>(ui->darkSongTable->cellWidget(row,kTitleCol))->setText(titlePlusTags);
    }
}

void MainWindow::removeAllTagsFromSongRow(int row) {
    if (row >= 0) {
        // exactly 1 row was selected (good)
        QString pathToMP3 = ui->darkSongTable->item(row,kPathCol)->data(Qt::UserRole).toString();
        SongSetting settings;
        songSettings.loadSettings(pathToMP3, settings);
        // QStringList tags;
        QString oldTags;
        if (settings.isSetTags())
        {
            oldTags = settings.getTags();
            // tags = oldTags.split(" ");
            songSettings.removeTags(oldTags);
        }

        QString newtags = "";
        songSettings.removeTags(oldTags);
        settings.setTags(newtags);
        songSettings.saveSettings(pathToMP3, settings);
        songSettings.addTags(newtags);

        // we know for sure that this item is selected (because that's how we got here), so let's highlight text color accordingly

        // extract the text color of the title
        QString title1 = getTitleColText(ui->darkSongTable, row); // e.g. "<span style=\"color: #7963ff;\">'Cuda NEW</span>"
        static QRegularExpression re("^<span style=[\"]color: (.*?);");
        QRegularExpressionMatch match = re.match(title1);

        QString theOriginalColor = "";
        if (match.hasMatch()) {
            theOriginalColor = match.captured(1);
        }

        // then, put the title back, with the original color, but without the tags
        QString title = getTitleColTitle(ui->darkSongTable, row);
        QString titlePlusTags(FormatTitlePlusTags(title, settings.isSetTags(), settings.getTags(), theOriginalColor));
        dynamic_cast<QLabel*>(ui->darkSongTable->cellWidget(row,kTitleCol))->setText(titlePlusTags);
    }
}


void MainWindow::removeAllTagsFromSong()
{
    int row = darkSelectedSongRow();  // from THIS row
    removeAllTagsFromSongRow(row);
}

void MainWindow::darkEditTags()
{
    int row = darkSelectedSongRow();
    if (row >= 0) {
        // exactly 1 row was selected (good)
        QString pathToMP3 = ui->darkSongTable->item(row,kPathCol)->data(Qt::UserRole).toString();
        SongSetting settings;
        songSettings.loadSettings(pathToMP3, settings);
        QString tags;
        bool ok(false);
        QString title = getTitleColTitle(ui->darkSongTable, row);
        if (settings.isSetTags()) {
            tags = settings.getTags();
        }

        // dialog needs to be bigger, so we have to do all this...
        QString newtags;
        QInputDialog *dialog = new QInputDialog();
        setDynamicPropertyRecursive(dialog, "theme", currentThemeString);

        dialog->setWindowTitle("Edit Tags for '" + title + "'");
        dialog->setInputMode(QInputDialog::TextInput);
        dialog->setLabelText("Note: separate tags by spaces.\n");
        dialog->setTextValue(tags);
        dialog->resize(500,200);
        dialog->exec();

        ok = dialog->result();

        if (ok) {
            newtags = dialog->textValue();
            songSettings.removeTags(tags);
            settings.setTags(newtags);
            songSettings.saveSettings(pathToMP3, settings);
            songSettings.addTags(newtags);

            // extract the text color of the title
            QString title1 = getTitleColText(ui->darkSongTable, row); // e.g. "<span style=\"color: #7963ff;\">'Cuda NEW</span>"
            static QRegularExpression re("^<span style=[\"]color: (.*?);");
            QRegularExpressionMatch match = re.match(title1);

            QString theOriginalColor = "";
            if (match.hasMatch()) {
                theOriginalColor = match.captured(1);
            }

            // we know for sure that this item is selected (because that's how we got here), so let's highlight text color accordingly
            QString titlePlusTags(FormatTitlePlusTags(title, settings.isSetTags(), settings.getTags(), theOriginalColor));
            dynamic_cast<QLabel*>(ui->darkSongTable->cellWidget(row,kTitleCol))->setText(titlePlusTags);
        }
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
    }
}

void MainWindow::revealInFinder()
{
}

void MainWindow::columnHeaderSorted(int logicalIndex, Qt::SortOrder order)
{
    Q_UNUSED(logicalIndex)
    Q_UNUSED(order)
    adjustFontSizes();  // when sort triangle is added, columns need to adjust width a bit...
}


void MainWindow::columnHeaderResized(int logicalIndex, int /* oldSize */, int newSize)
{
    Q_UNUSED(logicalIndex)
    Q_UNUSED(newSize)
}

// HELPER FUNCTIONS ----------------------
// The Old Light Mode sliders are gone, but we need to convert to/from their ranges for settings.
// EQ Knob range is 0 to 100
// EQ Slider range is -15 to +15

int KnobToSlider(int knobValue) {
    int equivalentSliderValue = round(-15 + 30 * (knobValue/100.0)); // rounded to nearest int
    return(equivalentSliderValue);
}

int SliderToKnob(int sliderValue) {
    int equivalentKnobValue = round((sliderValue + 15) * (100.0/30.0));
    return(equivalentKnobValue);
}

// ----------------------------------------------------------------------
void MainWindow::saveCurrentSongSettings()
{
    if (loadingSong) {
        // qDebug() << "***** WARNING: MainWindow::saveCurrentSongSettings tried to save while loadingSong was true";
        return;
    }
//    qDebug() << "MainWindow::saveCurrentSongSettings trying to save settings...";
    QString currentSong = ui->darkTitle->text();

    if (!currentSong.isEmpty()) {
        int pitch = ui->darkPitchSlider->value();
        int tempo = ui->darkTempoSlider->value();
        int cuesheetIndex = ui->comboBoxCuesheetSelector->currentIndex();
        QString cuesheetFilename = !lyricsForDifferentSong && cuesheetIndex >= 0 ?
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
//        qDebug() << "saveCurrentSongSettings: " << ui->seekBarCuesheet->GetIntro() << ui->seekBarCuesheet->GetOutro();
        setting.setIntroOutroIsTimeBased(false);
        if (!lyricsForDifferentSong) {
            setting.setCuesheetName(cuesheetFilename);
        }
        setting.setSongLength(static_cast<double>(ui->seekBarCuesheet->maximum()));

        // qDebug() << "saveCurrentSongSettings:" << ui->darkTrebleKnob->value() << KnobToSlider(ui->darkTrebleKnob->value());
        // qDebug() << "saveCurrentSongSettings:" << ui->darkMidKnob->value() << KnobToSlider(ui->darkMidKnob->value());
        // qDebug() << "saveCurrentSongSettings:" << ui->darkBassKnob->value() << KnobToSlider(ui->darkBassKnob->value());

        setting.setTreble(   KnobToSlider(ui->darkTrebleKnob->value()) );
        setting.setBass(     KnobToSlider(ui->darkBassKnob->value())   );
        setting.setMidrange( KnobToSlider(ui->darkMidKnob->value())    );
        // setting.setMix( ui->mixSlider->value() );

//        setting.setReplayGain();  // TODO:

        // only update the LoudMax setting if the LoudMax plugin is present.
        //  otherwise, leave it alone.  This prevents SquareDesk from erasing previous
        //  LoudMax settings, if transferring to a laptop that temporarily does not
        //  have LoudMax installed.
        if (loudMaxPlugin != nullptr) {
            setting.setVSTsettings(getCurrentLoudMaxSettings()); // get parameters from LoudMax plugin, and persist them
        }

        if (ui->actionLoop->isChecked()) {
            setting.setLoop( 1 );
        } else {
            setting.setLoop( -1 );
        }

//        // When it comes time to save the replayGain value, it should be done being calculated.
//        // So, save it, whatever it is.  This will fail, if the song is quickly clicked through.
//        // We guard against this by NOT saving, if it's exactly 0.0 dB (meaning not calculated yet).
//        if (cBass->Stream_replayGain_dB != 0.0) {
////            qDebug() << "***** saveCurrentSongSettings: saving replayGain value for" << currentSong << "= " << cBass->Stream_replayGain_dB;
//            setting.setReplayGain(cBass->Stream_replayGain_dB);
//        } else {
////            qDebug() << "***** saveCurrentSongSettings: NOT saving replayGain value for" << currentSong << ", because it's 0.0dB, so not set yet";
//        }

        songSettings.saveSettings(currentMP3filenameWithPath,
                                  setting);

//        if (ui->checkBoxAutoSaveLyrics->isChecked())
//        {
//            //writeCuesheet(cuesheetFilename);
//        }
    }


}

QString MainWindow::convertCuesheetPathNameToCurrentRoot(QString str1) {
    // attempts to converts any pathname (even one from an old musicRoot) to a full absolute pathname
    //   in the CURRENT musicRoot.  The file is guaranteed to exist, and is (presumably)
    //   the new location of the file.
    //
    // returns the ABSOLUTE pathname, if everything was OK
    //   or "", if the pathname couldn't be mapped to an existing file in the current musicRoot.

    QFileInfo fi1(str1);

    if (str1.startsWith(musicRootPath) && fi1.exists()) {
        // str1 was already an absolute pathname in the CURRENT root, and the file exists, all is well
        return str1;
    }

    // OK, now we're in the more complicated case, where we need to try to peel off parts off the front of the abs pathname
    //   to see if we can find the file in the current musicRoot.
    //
    // leverage the function that does this for Export Song Data ----
    //   let's have it look in called/vocals and patter folders, too, since some people stick cuesheets there as well.
    QString leftOverPiece = removeCuesheetDirs(str1, songTypeNamesForSinging + songTypeNamesForCalled + songTypeNamesForPatter);
    // qDebug() << "convertCuesheetPathNameToCurrentRoot leftOverPiece " << musicRootPath + leftOverPiece;
    return(musicRootPath + leftOverPiece);
}

bool MainWindow::compareCuesheetPathNamesRelative(QString str1, QString str2) {
    // returns true, if str1 and str2 have equal RELATIVE pathnames, even though
    //   the musicRoots might have been different.
    // This is to fix a bug where the last_cuesheet field in the sqlite3 DB is absolute,
    //   instead of relative (#1379).
    //
    // EXAMPLE: these string are all identical RELATIVE to the roots, which are different:
    // /Users/mpogue/Dropbox/__squareDanceMusic/lyrics/BS 2623 - Besame Mucho.html
    // /Users/mpogue/Library/CloudStorage/Box-Box/__squareDanceMusic_Box/lyrics/BS 2623 - Besame Mucho.html
    // /Users/mpogue/Library/Mobile Documents/com~apple~CloudDocs/SquareDance/squareDanceMusic_iCloud/lyrics/BS 2623 - Besame Mucho.html
    //
    // The current musicRootPath could be any of these three folders, which makes this a hard problem to solve.
    //   We don't have a record of what the other musicRootPaths might have been.
    //   And, cuesheets could have been residing in subdirectories of the musicRoot.
    //
    // So, this implementation is probably not 100% correct for some cases where files have
    //   copies with the same name in other subdirectories, but it's probably close enough for now to get around bug #1379.

    if (str1 == str2) {
        // simplest case, the two strings are identical
        return true;
    }

    // but, if they didn't match, we need to check for matching with possibly different musicRootPaths,
    //  which is more complicated

    int len1 = str1.length();
    int len2 = str2.length();
    int maxlen = (len1 < len2 ? len1 : len2); // the smaller of the two
    QString commonStr = "";  // what the strings have in common

    // look at the string in backwards order
    int i;
    for (i = 0; i < maxlen; i++) {
        QChar c1 = str1[len1 - i - 1];
        QChar c2 = str2[len2 - i - 1];
        if (c1 != c2) {
            break; // if they don't match, exit the for loop, we're done.
        }
        commonStr = c1 + commonStr; // tack this matching character on at the BEGINNING of the string
    }

    // qDebug() << "-----------------------------------------";
    // qDebug() << "compareCuesheetPathNamesRelative str1: " << str1;
    // qDebug() << "compareCuesheetPathNamesRelative str2: " << str2;
    // qDebug() << "compareCuesheetPathNamesRelative commonStr:" << commonStr;
    // qDebug() << "-----------------------------------------";

    bool result = false;  // default answer is false.

    if (commonStr.startsWith(".")) {
        // common case: the basenames don't match at all, so the matching part at the end of the strings is just ".html"
        //   so don't bother to look further
        return false;
    } else if (commonStr.startsWith("/lyrics/")) {
        // another simple case: different roots, but both cuesheets were in /lyrics, so they are RELATIVE equal
        //   they might be in a subdirectory of /lyrics, but they are in the SAME subdirectory, relative to the musicRoot
        // str1 is the correct pathname in the current musicRoot
        result = true;
    } else if (commonStr.startsWith("/singing/")) {
        // a common case: different roots, but both cuesheets were in /singing/, probably along with the MP3 file
        // str1 is the correct pathname in the current musicRoot
        result = true;
    } else if (commonStr.startsWith("/singers/")) {
        // a common case: different roots, but both cuesheets were in /singers/, probably along with the MP3 file in a subdirectory
        // str1 is the correct pathname in the current musicRoot
        result = true;
    }

    // if you changed the names of the singing call folders in Preferences, this function will fail, and you'll need
    //   to manually change the cuesheetComboBox to the correct item in the new musicRoot.

    // qDebug() << "result: " << result;
    return result;
}

void MainWindow::loadSettingsForSong(QString songTitle)
{
    Q_UNUSED(songTitle)

    // qDebug() << "loadSettingsForSong" << songTitle;

    int pitch = ui->darkPitchSlider->value();
    int tempo = ui->darkTempoSlider->value();  // qDebug() << "loadSettingsForSong, current tempo slider: " << tempo;
    int volume = ui->darkVolumeSlider->value();
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
        if (settings.isSetTempo()) { tempo = settings.getTempo(); /*qDebug() << "loadSettingsForSong: " << tempo;*/ }
        if (settings.isSetVolume()) { volume = settings.getVolume(); }
        if (settings.isSetIntroPos()) { intro = settings.getIntroPos(); }
        if (settings.isSetOutroPos()) { outro = settings.getOutroPos(); }

        if (settings.isSetCuesheetName()) { cuesheetName = settings.getCuesheetName(); } // ADDED *****

        // double length = (double)(ui->seekBarCuesheet->maximum());  // This is not correct, results in non-round-tripping
        double length = cBass->FileLength;  // This seems to work better, and round-tripping looks like it is working now.
        if (settings.isSetIntroOutroIsTimeBased() && settings.getIntroOutroIsTimeBased())
        {
            // qDebug() << "INTRO/OUTRO ARE IN SECONDS, CONVERTING TO FRACTION NOW..."; // I think this is old
            intro = intro / length;
            outro = outro / length;
        }
        // qDebug() << "loadSettingsForSong: INTRO/OUTRO ARE: " << intro << outro;

        // setup Markers for this song in the seekBar ----------------------------------
        // ui->seekBar->AddMarker(20.0/length);  // stored as fraction of the song length  // DEBUG DEBUG DEBUG, 20 sec for now
        // TODO: key to toggle a marker at (OR NEAR WITHIN A THRESHOLD) the current song position
        // TODO: repurpose |<< and >>| to jump to the next/previous marker (and if at beginning, go to prev song, at end, go to next song)
        // TODO: load from/store to DB for this song
        // -----------------------------------------------------------------------------

        ui->darkPitchSlider->setValue(pitch);
        ui->darkTempoSlider->setValue(tempo); // qDebug() << "loadSettingsForSong: just set tempo slider to: " << tempo;
        ui->darkVolumeSlider->setValue(volume);

        ui->seekBarCuesheet->SetIntro(intro);
        ui->seekBarCuesheet->SetOutro(outro);

        // if (darkmode) {
            // we need to set this in both places
        ui->darkSeekBar->setIntro(intro);
        ui->darkSeekBar->setOutro(outro);
        // } else {
            // we need to set this in both places
            // ui->seekBar->SetIntro(intro);
            // ui->seekBar->SetOutro(outro);
        // }

        QTime iTime = QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*intro*length+0.5));
        QTime oTime = QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*outro*length+0.5));
        ui->dateTimeEditIntroTime->setTime(iTime); // milliseconds
        ui->dateTimeEditOutroTime->setTime(oTime);
#ifdef DARKMODE
        ui->darkStartLoopTime->setTime(iTime); // milliseconds
        ui->darkEndLoopTime->setTime(oTime); // milliseconds
#endif
        // qDebug() << "***** loadSettingsForSong cuesheetName: " << cuesheetName;
        if (cuesheetName.length() > 0)
        {
            for (int i = 0; i < ui->comboBoxCuesheetSelector->count(); ++i)
            {
                QString itemName = ui->comboBoxCuesheetSelector->itemData(i).toString();
                // qDebug() << "***** loadSettingsForSong itemName:" << itemName;
                bool relativeEqual = compareCuesheetPathNamesRelative(itemName, cuesheetName); // ignore the musicRootPath parts

                // if (itemName == cuesheetName)
                if (relativeEqual)
                {
                    // qDebug() << "RELATIVE EQUAL --------";
                    ui->comboBoxCuesheetSelector->setCurrentIndex(i);  // This will always select a cuesheet in the CURRENT MusicRootPath
                    break;
                }
            }
        }
        // qDebug() << "***** loadSettingsForSong:" << currentMP3filename;
        // qDebug() << "loadSettingsForSong:" << settings.isSetTreble() << settings.getTreble();
        // qDebug() << "loadSettingsForSong:" << settings.isSetMidrange() << settings.getMidrange();
        // qDebug() << "loadSettingsForSong:" << settings.isSetBass() << settings.getBass();

        if (settings.isSetTreble())
        {
            ui->darkTrebleKnob->setValue( SliderToKnob(settings.getTreble()) );
        }
        else
        {
            ui->darkTrebleKnob->setValue( SliderToKnob(0) ) ;
        }
        if (settings.isSetBass())
        {
            ui->darkBassKnob->setValue( SliderToKnob(settings.getBass()) );
        }
        else
        {
            ui->darkBassKnob->setValue( SliderToKnob(0) );
        }
        if (settings.isSetMidrange())
        {
            ui->darkMidKnob->setValue( SliderToKnob(settings.getMidrange()) );
        }
        else
        {
            ui->darkMidKnob->setValue( SliderToKnob(0) );
        }

        // Looping is similar to Mix, but it's a bit more complicated:
        //   If the DB says +1, turn on loop.
        //   If the DB says -1, turn looping off.
        //   otherwise, the DB says "NULL", so use the default that we currently have (patter = loop).
        if (settings.isSetLoop())
        {
            if (settings.getLoop() == -1) {
                on_loopButton_toggled(false);
            } else if (settings.getLoop() == 1) {
                on_loopButton_toggled(true);
            } else {
                // DO NOTHING
            }
        }

    }
    else
    {
        ui->darkTrebleKnob->setValue(SliderToKnob(0)); // song not seen before, so T/M/B = (0 Slider, 50 Knob) = 0dB
        ui->darkBassKnob->setValue(SliderToKnob(0));
        ui->darkMidKnob->setValue(SliderToKnob(0));
        // ui->mixSlider->setValue(0);
    }

    // LoudMax settings for this song -----------------
    if (settings.isSetVSTsettings()) {
        // qDebug() << "Calling setLoudMaxFromPersistedSettings from loadSettingsFor Song...";
        setLoudMaxFromPersistedSettings(settings.getVSTsettings()); } // ADDED *****
}

void MainWindow::loadGlobalSettingsForSong(QString songTitle) {
   Q_UNUSED(songTitle)
//   qDebug() << "loadGlobalSettingsForSong";
   cBass->SetGlobals();
}

// ------------------------------------------------------------------------------------------

void MainWindow::on_warningLabelCuesheet_clicked() {
    // this one is clickable, too!
    on_darkWarningLabel_clicked();
}

void MainWindow::on_darkWarningLabel_clicked() {
    // this one is clickable, too!
    // on_warningLabel_clicked();
    // remember to implement this, used to be analogClock->clearPatter()
    ui->theSVGClock->resetPatter();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    // disable all dynamic menu items
    // ui->actionSave_Playlist_2->setEnabled(false);       // PLAYLIST > SAVE PLAYLIST
    // ui->actionSave_Playlist->setEnabled(false);         // PLAYLIST > SAVE PLAYLIST AS...
    // ui->actionPrint_Playlist->setEnabled(false);        // PLAYLIST > PRINT PLAYLIST...

    ui->actionSave_Cuesheet->setEnabled(false);         // CUESHEET > SAVE CUESHEET
    ui->actionSave_Cuesheet_As->setEnabled(false);      // CUESHEET > SAVE CUESHEET AS...
    ui->actionPrint_Cuesheet->setEnabled(false);        // CUESHEET > PRINT CUESHEET...

    ui->actionSave_Sequence->setEnabled(false);         // SD > SAVE SEQUENCE
    ui->actionSave_Sequence_As->setEnabled(false);      // SD > SAVE SEQUENCE TO FILE...
    ui->actionPrint_Sequence->setEnabled(false);        // SD > PRINT SEQUENCE...

    // clear shortcuts from all dynamic menu items (Print menu items don't get shortcuts, so don't bother with those)
    // ui->actionSave_Playlist_2->setShortcut(QKeySequence());     // PLAYLIST > SAVE PLAYLIST
    // ui->actionSave_Playlist->setShortcut(QKeySequence());       // PLAYLIST > SAVE PLAYLIST AS...

    ui->actionSave_Cuesheet->setShortcut(QKeySequence());       // CUESHEET > SAVE CUESHEET
    ui->actionSave_Cuesheet_As->setShortcut(QKeySequence());    // CUESHEET > SAVE CUESHEET AS...

    ui->actionSave_Sequence->setShortcut(QKeySequence());       // SD > SAVE SEQUENCE
    ui->actionSave_Sequence_As->setShortcut(QKeySequence());    // SD > SAVE SEQUENCE TO FILE...

    QString currentTabName = ui->tabWidget->tabText(index);

//    qDebug() << "on_tabWidget_currentChanged: " << index << currentTabName;

    if (currentTabName == "Music") {
        // MUSIC PLAYER TAB -------------
//        qDebug() << "linesInCurrentPlaylist: " << linesInCurrentPlaylist << playlistHasBeenModified;
        // ui->actionSave_Playlist_2->setEnabled(linesInCurrentPlaylist != 0);  // playlist can be saved if there are >0 lines (this is SAVE PLAYLIST)
        // ui->actionSave_Playlist_2->setShortcut(QKeyCombination(Qt::ControlModifier, Qt::Key_S));  // Cmd-S

        // ui->actionSave_Playlist->setEnabled(playlistHasBeenModified); // this is SAVE PLAYLIST AS
        // ui->actionSave_Playlist->setShortcut(QKeyCombination(Qt::ShiftModifier|Qt::ControlModifier, Qt::Key_S)); // SHIFT-Cmd-S

        // ui->actionPrint_Playlist->setEnabled(true);

        if (cBass->isPaused()) {
            // if (!darkmode) {
            //     ui->titleSearch->setFocus();
            // } else {
            ui->darkSearch->setFocus();
            // }
        }
    } else if (currentTabName == CUESHEET_TAB_NAME) {
        // CUESHEET TAB -----------
        ui->actionSave_Cuesheet->setEnabled(cuesheetIsUnlockedForEditing); // CUESHEET > SAVE CUESHEET
        ui->actionSave_Cuesheet->setShortcut(QKeyCombination(Qt::ControlModifier, Qt::Key_S));  // Cmd-S

        ui->actionSave_Cuesheet_As->setEnabled(hasLyrics);
        ui->actionSave_Cuesheet_As->setShortcut(QKeyCombination(Qt::ShiftModifier|Qt::ControlModifier, Qt::Key_S)); // SHIFT-Cmd-S

        ui->actionPrint_Cuesheet->setEnabled(hasLyrics);
    } else if (currentTabName == "SD") {
        // SD TAB ---------------
        ui->actionSave_Sequence->setEnabled(editSequenceInProgress);
        ui->actionSave_Sequence->setShortcut(QKeyCombination(Qt::ControlModifier, Qt::Key_S));  // Cmd-S

        ui->actionSave_Sequence_As->setEnabled(true);
        ui->actionSave_Sequence_As->setShortcut(QKeyCombination(Qt::ShiftModifier|Qt::ControlModifier, Qt::Key_S)); // SHIFT-Cmd-S

        ui->actionPrint_Sequence->setEnabled(true);

        ui->lineEditSDInput->setFocus();
    } else {
        // DANCE PROGRAMS, REFERENCE TABS --------
        // nothing, leave everything disabled
    }

    // and update the lower right hand status message
    microphoneStatusUpdate();
}

void MainWindow::microphoneStatusUpdate() {
    QString kybdStatus("SD (Level: " + currentSDKeyboardLevel + ", Dance: " + frameName + "), Audio: " + lastAudioDeviceName);

#ifndef DEBUG_LIGHT_MODE
    if (darkmode) {
        micStatusLabel->setStyleSheet("color: #C0C0C0;");
    } else {
        micStatusLabel->setStyleSheet("color: black");
    }
#endif

    micStatusLabel->setText(kybdStatus);
}

// ---------------------------------------------
void MainWindow::initReftab() {
    static QRegularExpression re1("reference/0[0-9][0-9]\\.[a-zA-Z0-9' ]+\\.txt$", QRegularExpression::CaseInsensitiveOption);

    documentsTab = new QTabWidget();

    // Items in the Reference tab are sorted, and are ordered by an optional 3 digits: 1 X X then a dot, then the tabname, then .txt/pdf
    QString referencePath = musicRootPath + "/reference";
    QDir dir(referencePath);
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Name);

    QFileInfoList list = dir.entryInfoList();

    // FOR EACH FILE IN /REFERENCE
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        QString filename = fileInfo.absoluteFilePath();
//        qDebug() << "filename: " << filename;

        QFileInfo info1(filename);
        QString tabname;
        bool HTMLfolderExists = false;
        QString whichHTM = "";
        if (info1.isDir()) {
            QFileInfo info2(filename + "/index.html");
            if (info2.exists()) {
//                qDebug() << "    FOUND INDEX.HTML";
                tabname = filename.split("/").last();
                static QRegularExpression re1("^1[0-9][0-9]\\.");
                tabname.remove(re1);  // 101.foo/index.html -> "foo"
                HTMLfolderExists = true;
                whichHTM = "/index.html";
            } else {
                QFileInfo info3(filename + "/index.htm");
                if (info3.exists()) {
//                    qDebug() << "    FOUND INDEX.HTM";
                    tabname = filename.split("/").last();
                    static QRegularExpression re2("^1[0-9][0-9]\\.");
                    tabname.remove(re2);  // 101.foo/index.htm -> "foo"
                    HTMLfolderExists = true;
                    whichHTM = "/index.htm";
                }
            }
            if (HTMLfolderExists) {
                QWebEngineView *newView = new QWebEngineView();
                webViews.append(newView);

#if defined(Q_OS_WIN32)
                QString indexFileURL = "file:///" + filename + whichHTM;  // Yes, Windows is special
#else
                QString indexFileURL = "file://" + filename + whichHTM;   // UNIX-like OS's only need one slash.
#endif

                newView->setUrl(QUrl(indexFileURL));
                newView->page()->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);  // allow index.html to access 3P remote website
                documentsTab->addTab(newView, tabname);
            }

        } else if (filename.endsWith(".txt") &&   // ends in .txt, AND
//                   QRegExp("reference/0[0-9][0-9]\\.[a-zA-Z0-9' ]+\\.txt$", Qt::CaseInsensitive).indexIn(filename) == -1) {  // is not a Dance Program file in /reference
            filename.indexOf(re1) == -1) {
//                qDebug() << "    FOUND TXT FILE";
                static QRegularExpression re2(".txt$");
                tabname = filename.split("/").last().remove(re2);
                static QRegularExpression re3("^1[0-9][0-9]\\.");
                tabname.remove(re3);  // 122.bar.txt -> "bar"

//                qDebug() << "    tabname:" << tabname;

                QWebEngineView *newView = new QWebEngineView();
                webViews.append(newView);

#if defined(Q_OS_WIN32)
                QString indexFileURL = "file:///" + filename;  // Yes, Windows is special: file:///Y:/Dropbox/__squareDanceMusic/reference/100.Broken Hearts.txt
#else
                QString indexFileURL = "file://" + filename;   // UNIX-like OS's only need one slash.
#endif
//                qDebug() << "    indexFileURL:" << indexFileURL;

                newView->setUrl(QUrl(indexFileURL));
                documentsTab->addTab(newView, tabname);

        } else if (filename.endsWith(".md", Qt::CaseInsensitive)) {  // is not a Dance Program file in /reference
                static QRegularExpression re4(".[Mm][Dd]$");
                tabname = filename.split("/").last().remove(re4);

                QWebEngineView *newView = new QWebEngineView();
                webViews.append(newView);

                QFile f1(filename);
                f1.open(QIODevice::ReadOnly | QIODevice::Text);
                QTextStream in(&f1);
                QString html = txtToHTMLlyrics(in.readAll(), filename);

                newView->setHtml(html);
                newView->setZoomFactor(1.5);
                documentsTab->addTab(newView, tabname);

        } else if (filename.endsWith(".pdf")) {
//                qDebug() << "PDF FILE DETECTED:" << filename;

                QString app_path = qApp->applicationDirPath();
                auto url = QUrl::fromLocalFile(app_path+"/minified/web/viewer.html");  // point at the viewer
//                qDebug() << "    Viewer URL:" << url;

                QDir dir(app_path+"/minified/web/");
                QString pdf_path = dir.relativeFilePath(filename);  // point at the file to be viewed (relative!)
//                qDebug() << "    pdf_path: " << pdf_path;

                Communicator *m_communicator = new Communicator(this);
                m_communicator->setUrl(pdf_path);

                QWebEngineView *newView = new QWebEngineView();
                webViews.append(newView);

                QWebChannel * channel = new QWebChannel(this);
                channel->registerObject(QStringLiteral("communicator"), m_communicator);

                newView->page()->setWebChannel(channel);
                newView->load(url);

//                QString indexFileURL = "file://" + filename;
//                qDebug() << "    indexFileURL:" << indexFileURL;

//                QFileInfo fInfo(filename);
                static QRegularExpression re5(".pdf$");
                tabname = filename.split("/").last().remove(re5);      // get basename, remove .pdf
                static QRegularExpression re6("^1[0-9][0-9]\\.");
                tabname.remove(re6);                         // 122.bar.txt -> "bar"  // remove optional 1##. at front

                documentsTab->addTab(newView, tabname);
        }

    } // for loop, iterating through <musicRoot>/reference

    ui->refGridLayout->addWidget(documentsTab, 0,1);
}

void MainWindow::initSDtab() {
    // SD -------------------------------------------
    copyrightShown = false;  // haven't shown it once yet
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
    prefsManager.Setenableautoscrolllyrics(ui->actionAuto_scroll_during_playback->isChecked());
}

// ==========================================================
void MainWindow::on_actionStartup_Wizard_triggered()
{
    StartupWizard wizard;
    int dialogCode = wizard.exec();

    if(dialogCode == QDialog::Accepted) {
        // must setup internal variables, from updated Preferences..
        musicRootPath = prefsManager.GetmusicPath();

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
        
        // used to store the file paths
        findMusic(musicRootPath, true);  // get the filenames from the user's directories
        darkFilterMusic(); // and filter them into the songTable
//        qDebug() << "LOAD MUSIC LIST FROM STARTUP WIZARD TRIGGERED";
        darkLoadMusicList(nullptr, "Tracks", true, true); // just refresh whatever is showing

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
    ui->darkSongTable->initializeSortOrder(); // clear out the queue and start over

    ui->darkSongTable->sortItems(kLabelCol);  // sort last by label/label #
    ui->darkSongTable->sortItems(kTitleCol);  // sort second by title in alphabetical order
    ui->darkSongTable->sortItems(kTypeCol);   // sort first by type (singing vs patter)
}

void MainWindow::sdActionTriggered(QAction * action) {
//    qDebug() << "***** sdActionTriggered()" << action << action->isChecked();
    action->setChecked(true);  // check the new one
//    renderArea->setCoupleColoringScheme(action->text());  // removed: causes crash on Win32, and not needed
    setSDCoupleColoringScheme(action->text());
}

// SD VIEW =====================================
void MainWindow::sdViewActionTriggered(QAction * action) {
    action->setChecked(true);  // check the new one
//    qDebug() << "***** sdViewActionTriggered()" << action << action->isChecked();

//    ui->labelWorkshop->setHidden(true); // DEBUG

    if (action->text() == "Sequence Designer") {
        // turn on thumbnails
        ui->actionFormation_Thumbnails->setChecked(true);
        on_actionFormation_Thumbnails_triggered();

        // SD Output intentionally not touched by this, since it's default is OFF now
        // Can't make the Options/?/Additional/Names size zero, because Names is now used by SD Dance Arranger (manual resize works tho)
        // instead, set Options as current tab
        ui->tabWidgetSDMenuOptions->setCurrentIndex(0);
        ui->actionNormal_3->setChecked(true);
        setSDCoupleNumberingScheme("Numbers");

        ui->tableWidgetCurrentSequence->clearSelection();
//        ui->tableWidgetCurrentSequence->setSelectionMode(QAbstractItemView::ContiguousSelection); // can select any set of contiguous items with SHIFT
        ui->tableWidgetCurrentSequence->setSelectionMode(QAbstractItemView::SingleSelection); // can select any set of contiguous items with SHIFT
        ui->tableWidgetCurrentSequence->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);  // auto set height of rows

        ui->tabWidgetSDMenuOptions->setTabVisible(0, true); // Options
        ui->tabWidgetSDMenuOptions->setTabVisible(1, true); // ?
        ui->tabWidgetSDMenuOptions->setTabVisible(2, true); // Additional

        ui->lineEditSDInput->setVisible(true);  // make SD input field visible
        ui->lineEditSDInput->setFocus(); // put the focus in the SD input field

        ui->label_SD_Resolve->setVisible(true); // show resolve field

        ui->actionShow_Frames->setChecked(false); // hide frames
        on_actionShow_Frames_triggered();

//        ui->labelWorkshop->setText("<B>BAD</B>");
        ui->label_CurrentSequence->setText("<B>Current Sequence");
    } else {
        // Dance Arranger
        ui->actionFormation_Thumbnails->setChecked(false); // these are not shown, but they are still there..., so we can reference them onDoubleClick below
        on_actionFormation_Thumbnails_triggered();

        bool inSDEditMode = ui->lineEditSDInput->isVisible(); // if we're in UNLOCK/EDIT mode, can do Add/Delete Comments
        ui->sdCurrentSequenceTitle->setFrame(inSDEditMode);  // edit is enabled, so show the frame

        ui->tableWidgetCurrentSequence->clearSelection();
        ui->tableWidgetCurrentSequence->setFocus();
        ui->tableWidgetCurrentSequence->setSelectionMode(QAbstractItemView::SingleSelection); // can only select ONE item
        if (ui->tableWidgetCurrentSequence->rowCount() != 0) {
            ui->tableWidgetCurrentSequence->item(0,0)->setSelected(true); // select first item (if there is a first item)
            //on_tableWidgetCurrentSequence_itemDoubleClicked(ui->tableWidgetCurrentSequence->item(0,1)); // double click on first item to render it (kColCurrentSequenceFormation)

            ui->tableWidgetCurrentSequence->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);  // auto set height of rows
        }

        // NOTE: These tabs need to be visible all the time, OR ELSE we get weird crash **in Qt code** when going from SDesk to QtCreator
        ui->tabWidgetSDMenuOptions->setTabVisible(0, true); // Options
        ui->tabWidgetSDMenuOptions->setTabVisible(1, true); // ?
        ui->tabWidgetSDMenuOptions->setTabVisible(2, true); // Additional
        ui->tabWidgetSDMenuOptions->setTabVisible(3, true); // Names

        // We're going to hide the SDInput field when in Dance Arranger mode for now, because if something is typed in here,
        //   I don't want to have to figure out what happens to changes to that sequence (do they get written back to the Fn frame automatically?
        //   Do I need to ask the user, when they Fn to another frame?  etc.).
        // It is OK to have it visible, it just means more work to figure out what to do for the above cases.  For now, it's off.
        ui->lineEditSDInput->setVisible(false);  // hide SD input field // TEMPORARY FIX FIX FIX when "true"
        ui->tableWidgetCurrentSequence->setFocus(); // put the focus in the current sequence pane, where the first one will be bright blue (not gray)

        // ui->label_SD_Resolve->setVisible(false); // hide resolve field // let's NOT hide it, because AL and RLG can't appear in the Current Sequence right now

        ui->actionShow_Frames->setChecked(true); // show frames
        on_actionShow_Frames_triggered();

//        ui->labelWorkshop->setText("<html><head/><body><p><span style=\" font-weight:700; color:#ff0000;\">BAD</span></p></body></html>");
    }
}

// SD Colors, Numbers, and Genders ------------
void MainWindow::sdActionTriggeredColors(QAction * action) {
//    qDebug() << "***** sdActionTriggeredColors()" << action << action->isChecked();
    action->setChecked(true);  // check the new one
    setSDCoupleColoringScheme(action->text());
}

void MainWindow::sdActionTriggeredNumbers(QAction * action) {
//    qDebug() << "***** sdActionTriggeredNumbers()" << action << action->isChecked();
    action->setChecked(true);  // check the new one
    setSDCoupleNumberingScheme(action->text());
}

void MainWindow::sdActionTriggeredGenders(QAction * action) {
//    qDebug() << "***** sdActionTriggeredGenders()" << action << action->isChecked();
    action->setChecked(true);  // check the new one
    setSDCoupleGenderingScheme(action->text());
}

void MainWindow::sdAction2Triggered(QAction * action) {
//    qDebug() << "***** sdAction2Triggered()" << action << action->isChecked();
    action->setChecked(true);  // check the new one
    currentSDKeyboardLevel = action->text().toLower();   // convert to all lower case for SD
    microphoneStatusUpdate();  // update status message, based on new keyboard SD level
}

void MainWindow::airplaneMode(bool turnItOn) {
#if defined(Q_OS_MAC)
    char cmd[100];
    if (turnItOn) {
        snprintf(cmd, sizeof(cmd), "osascript -e 'do shell script \"networksetup -setairportpower en0 off\"'\n");
    } else {
        snprintf(cmd, sizeof(cmd), "osascript -e 'do shell script \"networksetup -setairportpower en0 on\"'\n");
    }
    system(cmd);
#else
    Q_UNUSED(turnItOn)
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

void MainWindow::stopSFX() {
    cBass->StopAllSoundEffects();
}

void MainWindow::playSFX(QString which) {
    QString soundEffectFile;

    if (which.toInt() > 0) {
        soundEffectFile = soundFXfilenames[which.toInt()-1];
    } else {
        // conversion failed, this is break_end or long_tip.mp3
        soundEffectFile = musicRootPath + "/soundfx/" + which + ".mp3";
    }

//    if(QFileInfo(soundEffectFile).exists()) {
    if(QFileInfo::exists(soundEffectFile)) {
        // play sound FX only if file exists...
        cBass->PlayOrStopSoundEffect(which.toInt(),
                                    soundEffectFile.toLocal8Bit().constData());  // convert to C string; defaults to volume 100%
    } else {
        qDebug() << "***** ERROR: sound FX " << which << " does not exist: " << soundEffectFile;
    }
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
        msgBox.setText("<B>ERROR</B><P>Sorry, the update server is not reachable right now.<P>" + reply->errorString());
        msgBox.exec();
        return;
    }

    // "latest" is of the form "X.Y.Z\n", so trim off the NL
    QString latestVersionNumber = QString(result).trimmed();

    // extract the pieces of the latest one
    static QRegularExpression rxVersion("^(\\d+)\\.(\\d+)\\.(\\d+)");
    QRegularExpressionMatch match1 = rxVersion.match(latestVersionNumber);
    unsigned int major1 = static_cast<unsigned int>(match1.captured(1).toInt());
    unsigned int minor1 = static_cast<unsigned int>(match1.captured(2).toInt());
    unsigned int little1 = static_cast<unsigned int>(match1.captured(3).toInt());
    unsigned int iVersionLatest = 100*(100*major1 + minor1) + little1;

    // extract the pieces of the current one
    QRegularExpressionMatch match2 = rxVersion.match(VERSIONSTRING);
    unsigned int major2 = static_cast<unsigned int>(match2.captured(1).toInt());
    unsigned int minor2 = static_cast<unsigned int>(match2.captured(2).toInt());
    unsigned int little2 = static_cast<unsigned int>(match2.captured(3).toInt());
    unsigned int iVersionCurrent = 100*(100*major2 + minor2) + little2;

//    qDebug() << "***** iVersionLatest: " << iVersionLatest << match1.captured(1) << match1.captured(2) << match1.captured(3);
//    qDebug() << "***** iVersionCurrent: " << iVersionCurrent << match2.captured(1) << match2.captured(2) << match2.captured(3);

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Information);

    if (iVersionLatest == iVersionCurrent) {
        msgBox.setText("<B>You are running the latest version of SquareDesk.</B>");
    } else if (iVersionLatest < iVersionCurrent) {
        msgBox.setText("<H2>You are ahead of the latest version.</H2>\nYour version of SquareDesk: " + QString(VERSIONSTRING) +
                       "<P>Latest version of SquareDesk: " + latestVersionNumber);
    } else {
        msgBox.setText("<H2>Newer version available</H2>\nYour version of SquareDesk: " + QString(VERSIONSTRING) +
                       "<P>Latest version of SquareDesk: " + latestVersionNumber +
                       "<P><a href=\"https://github.com/mpogue2/SquareDesk/releases\">Download new version</a>");
    }

    msgBox.exec();
}

void MainWindow::maybeInstallReferencefiles() {

#if defined(Q_OS_MAC)
    QString pathFromAppDirPathToResources = "/../Resources";

    // Let's make a "reference" directory in the Music Directory, if it doesn't exist already
    QString musicDirPath = prefsManager.GetmusicPath();
    QString referenceDir = musicDirPath + "/reference";

    // // if the reference directory doesn't exist, create it (always, automatically)
    // QDir dir(referenceDir);
    // if (!dir.exists()) {
    //     dir.mkpath(".");  // make it
    // }

    // ---------------
    // and populate it with the SD doc, if it didn't exist (somewhere) already
    bool hasSDpdf = false;
    bool hasNOSD = false;
    QString latestSDFilename = "196.SD_V";
    latestSDFilename = latestSDFilename + SD_VERSION + ".pdf";

    QDirIterator it(referenceDir);
    while(it.hasNext()) {
        QString s2 = it.next();
        QString s1 = it.fileName();

        // if already has a "196.SD.pdf" don't copy the latest SD doc into the Resources folder in musicDir
        if (s1.contains(latestSDFilename)) {
           hasSDpdf = true;
        } else if (s1.contains("NOSD", Qt::CaseInsensitive)) {  // if user has done "touch NOSD", then they do NOT want the latest SD_doc.pdf copied in to "196.SD_V<version>.pdf"
            hasNOSD = true;
        }
        // qDebug() << "Resource: " << s1 << hasSDpdf << hasNOSD;
    }

    if (!hasSDpdf && !hasNOSD) {
        qDebug() << "Copying in...";
        QString source = QCoreApplication::applicationDirPath() + pathFromAppDirPathToResources + "/sd_doc.pdf";
        QString destination = referenceDir + "/" + latestSDFilename;
        QFile::copy(source, destination);
    }
#endif
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
    QString musicDirPath = prefsManager.GetmusicPath();
    QString soundfxDir = musicDirPath + "/soundfx";

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

    // check for thirty_second_warning.mp3 and copy it in, if it doesn't exist already
    QFile thirtySecSound(soundfxDir + "/thirty_second_warning.mp3");
    if (!thirtySecSound.exists()) {
        QString source = QCoreApplication::applicationDirPath() + pathFromAppDirPathToStarterSet + "/thirty_second_warning.mp3";
        QString destination = soundfxDir + "/thirty_second_warning.mp3";
        QFile::copy(source, destination);
    }

    // check for individual soundFX files, and copy in a starter-set sound, if one doesn't exist,
    // which is checked when the App starts, and when the Startup Wizard finishes setup.  In which case, we
    //   only want to copy files into the soundfx directory if there aren't already files there.
    //   It might seem like overkill right now (and it is, right now).
    bool foundSoundFXfile[NUMBEREDSOUNDFXFILES];
    for (int i=0; i<NUMBEREDSOUNDFXFILES; i++) {
        foundSoundFXfile[i] = false;
    }

    QDirIterator it(soundfxDir);
    while(it.hasNext()) {
        QString s1 = it.next();

        QString baseName = s1.replace(soundfxDir + "/","");
        QStringList sections = baseName.split(".");

        if (sections.length() == 3 &&
            sections[0].toInt() >= 1 &&
            sections[0].toInt() <= NUMBEREDSOUNDFXFILES &&   // don't allow accesses to go out-of-bounds (e.g. "9.foo.mp3")
            sections[2].compare("mp3",Qt::CaseInsensitive) == 0) {
            foundSoundFXfile[sections[0].toInt() - 1] = true;  // found file of the form <N>.<something>.mp3
//            qDebug() << "Found soundfx[" << sections[0].toInt() << "]";
        }
    } // while

    QString starterSet[] = {
        "1.whistle.mp3",
        "2.clown_honk.mp3",
        "3.submarine.mp3",
        "4.applause.mp3",
        "5.fanfare.mp3",
        "6.fade.mp3",
        "7.short_bell.mp3",
        "8.ding_ding.mp3"
    };
    for (int i=0; i<NUMBEREDSOUNDFXFILES; i++) {
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
    cBass->StopAllSoundEffects();
}


void MainWindow::on_actionViewTags_toggled(bool checked)
{
    // qDebug() << "VIEW TAGS TOGGLED, and isChecked:" << ui->actionViewTags->isChecked() << checked;

    // prefsManager.SetshowSongTags(ui->actionViewTags->isChecked());
    prefsManager.SetshowSongTags(checked);

    if (!doNotCallDarkLoadMusicList) { // I hate this, but I can't change the signature of actionViewTags or ThemeToggled, or they won't match
        refreshAllPlaylists(); // tags changed, so update the playlist views based on ui->actionViewTags->isChecked()
        darkLoadMusicList(nullptr, currentTypeFilter, true, false); // just refresh whatever is showing
    }
}

// ---------------------------------------------------------------------------------------------------------------------
void MainWindow::on_actionRecent_toggled(bool checked)
{
    ui->actionRecent->setChecked(checked);  // when this function is called at constructor time, preferences sets the checkmark

    // the showRecentColumn setting is persistent across restarts of the application
    prefsManager.SetshowRecentColumn(checked);

    updateSongTableColumnView();
}

void MainWindow::on_actionAge_toggled(bool checked)
{
    ui->actionAge->setChecked(checked);  // when this function is called at constructor time, preferences sets the checkmark

    // the showAgeColumn setting is persistent across restarts of the application
    prefsManager.SetshowAgeColumn(checked);

    updateSongTableColumnView();
}

void MainWindow::on_actionPitch_toggled(bool checked)
{
    ui->actionPitch->setChecked(checked);  // when this function is called at constructor time, preferences sets the checkmark

    // the showAgeColumn setting is persistent across restarts of the application
    prefsManager.SetshowPitchColumn(checked);

    updateSongTableColumnView();
}

void MainWindow::on_actionTempo_toggled(bool checked)
{
    ui->actionTempo->setChecked(checked);  // when this function is called at constructor time, preferences sets the checkmark

    // the showAgeColumn setting is persistent across restarts of the application
    prefsManager.SetshowTempoColumn(checked);

    updateSongTableColumnView();
}

void MainWindow::on_actionFade_Out_triggered()
{
    cBass->FadeOutAndPause();
}

// For improving as well as measuring performance of long songTable operations
// The hide() really gets most of the benefit:
//
//                                          no hide()   with hide()
// loadMusicList                             129ms      112ms
// finishLoadingPlaylist (50 items list)    3227ms     1154ms
// clear playlist                           2119ms      147ms
//
// I also now disable signals on songTable, which will stop itemChanged from being called.
//   I saw one crash where it looked like itemChanged was being called recursively at app startup
//   time, and hopefully this will ensure that it can't happen again.  Plus, it should speed up
//   app startup time.
void MainWindow::startLongSongTableOperation(QString s) {
    Q_UNUSED(s)
}

void MainWindow::stopLongSongTableOperation(QString s) {
    Q_UNUSED(s)
}

QString MainWindow::filepath2SongCategoryName(QString MP3Filename)
{
    // returns the category name (as a string).  patter, hoedown -> "patter", as per user prefs

    MP3Filename.replace(QRegularExpression("^" + musicRootPath),"");  // delete the <path to musicDir> from the front of the pathname
    QStringList parts = MP3Filename.split("/");

    if (parts.length() <= 1) {
        return("unknown");
    } else {
        QString folderTypename = parts[1].toLower();
        if (songTypeNamesForPatter.contains(folderTypename)) {
            return("patter");
        } else if (songTypeNamesForSinging.contains(folderTypename)) {
            return("singing");
        } else if (songTypeNamesForCalled.contains(folderTypename)) {
            return("called");
        } else if (songTypeNamesForExtras.contains(folderTypename)) {
            return("extras");
        } else {
            return(folderTypename);
        }
    }
}

void MainWindow::on_actionFilePrint_triggered()
{
}

void MainWindow::on_actionSave_triggered()
{
}

void MainWindow::on_actionSave_As_triggered()
{
}

QList<QString> MainWindow::getListOfMusicFiles()
{
    QList<QString> list;

    QListIterator<QString> iter(*pathStack); // iterate over list of SONGS
    while (iter.hasNext()) {
        QString s = iter.next();
        QStringList sl1 = s.split("#!#");
        QString type = sl1[0].toLower();      // the type (of original pathname, before following aliases)
        QString filename = sl1[1];  // everything else

        // qDebug() << "getListOfMusicFiles(): type = " << type;

        // TODO: should we allow "patter" to match cuesheets?
        // if ((type == "singing" || type == "vocals") &&
        if ((songTypeNamesForSinging.contains(type) || songTypeNamesForCalled.contains(type)) &&
                                                        (filename.endsWith("mp3", Qt::CaseInsensitive) ||
                                                         filename.endsWith("m4a", Qt::CaseInsensitive) ||
                                                         filename.endsWith("flac", Qt::CaseInsensitive) ||
                                                        filename.endsWith("wav", Qt::CaseInsensitive))) {
            QFileInfo fi(filename);
            QString justFilename = fi.fileName();
            list.append(justFilename);
//            qDebug() << "music that might have a cuesheet: " << type << ":" << justFilename;
        }
    }

    return(list);
}


void MainWindow::on_actionTest_Loop_triggered()
{
//    qDebug() << "Test Loop Intro: " << ui->seekBar->GetIntro() << ", outro: " << ui->seekBar->GetOutro();

    if (!songLoaded) {
        return;  // if there is no song loaded, no point in doing anything.
    }

    cBass->Stop();  // always pause playback

    // If we clicked on Test Loop, we want to make sure that looping is enabled.
    //  Doesn't really make sense otherwise, right?
    on_loopButton_toggled(true);

    double songLength = cBass->FileLength;
//    double intro = ui->seekBar->GetIntro(); // 0.0 - 1.0
    double outro = ui->darkSeekBar->getOutroFrac(); // 0.0 - 1.0

    double startPosition_sec = fmax(0.0, songLength*outro - 5.0);

    cBass->StreamSetPosition(startPosition_sec);
    Info_Seekbar(false);  // update just the text

//    on_playButton_clicked();  // play, starting 5 seconds before the loop

    cBass->Play();  // currently paused, so start playing at new location
}

void MainWindow::on_dateTimeEditIntroTime_timeChanged(const QTime &time)
{
//    qDebug() << "on_dateTimeEditIntroTime_timeChanged: " << time;

    double position_sec = 60*time.minute() + time.second() + time.msec()/1000.0;
    double length = cBass->FileLength;
//    double t_ms = position_ms/length;

//    ui->seekBarCuesheet->SetIntro((float)t_ms/1000.0);
//    ui->seekBar->SetIntro((float)t_ms/1000.0);

//    on_loopButton_toggled(ui->actionLoop->isChecked()); // call this, so that cBass is told what the loop points are (or they are cleared)

    QTime currentOutroTime = ui->dateTimeEditOutroTime->time();
    double currentOutroTimeSec = 60.0*currentOutroTime.minute() + currentOutroTime.second() + currentOutroTime.msec()/1000.0;
    position_sec = fmax(0.0, fmin(position_sec, static_cast<int>(currentOutroTimeSec)-6) );

    // set in ms
//    qDebug() << "dateTimeEditIntro changed: " << currentOutroTimeSec << "," << position_sec;
    ui->dateTimeEditIntroTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*position_sec+0.5))); // milliseconds NOTE: THIS ONE MUST BE FIRST
    ui->darkStartLoopTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*position_sec+0.5))); // milliseconds NOTE: THIS ONE MUST BE SECOND

    // set in fractional form
    double frac = position_sec/length;
    ui->seekBarCuesheet->SetIntro(frac);  // after the events are done, do this.
    // ui->seekBar->SetIntro(frac);
    ui->darkSeekBar->setIntro(frac);
    on_loopButton_toggled(ui->actionLoop->isChecked()); // then finally do this, so that cBass is told what the loop points are (or they are cleared)
    saveCurrentSongSettings();
}

void MainWindow::on_dateTimeEditOutroTime_timeChanged(const QTime &time)
{
//    qDebug() << "on_dateTimeEditOutroTime_timeChanged: " << time;

    double position_sec = 60*time.minute() + time.second() + time.msec()/1000.0;
    double length = cBass->FileLength;
//    double t_ms = position_ms/length;

//    ui->seekBarCuesheet->SetOutro((float)t_ms/1000.0);
//    ui->seekBar->SetOutro((float)t_ms/1000.0);

//    on_loopButton_toggled(ui->actionLoop->isChecked()); // call this, so that cBass is told what the loop points are (or they are cleared)

    QTime currentIntroTime = ui->dateTimeEditIntroTime->time();
    double currentIntroTimeSec = 60.0*currentIntroTime.minute() + currentIntroTime.second() + currentIntroTime.msec()/1000.0;
    position_sec = fmin(length, fmax(position_sec, static_cast<int>(currentIntroTimeSec)+6) );

    // set in ms
//    qDebug() << "dateTimeEditOutro changed: " << currentIntroTimeSec << "," << position_sec << time << length;
    ui->dateTimeEditOutroTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*position_sec+0.5))); // milliseconds NOTE: THIS ONE MUST BE FIRST
    ui->darkEndLoopTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*position_sec+0.5))); // milliseconds NOTE: THIS ONE MUST BE SECOND

    // set in fractional form
    double frac = position_sec/length;
    // qDebug() << "dateTimeEditOutro:" << frac;
    ui->seekBarCuesheet->SetOutro(frac);  // after the events are done, do this.
    // ui->seekBar->SetOutro(frac);

    ui->darkSeekBar->setOutro(frac);

    on_loopButton_toggled(ui->actionLoop->isChecked()); // then finally do this, so that cBass is told what the loop points are (or they are cleared)
    saveCurrentSongSettings();
}

void MainWindow::on_pushButtonTestLoop_clicked()
{
    on_actionTest_Loop_triggered();
}

void MainWindow::on_actionClear_Recent_triggered()
{
    // QString nowISO8601 = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);  // add days is for later <-- deprecated soon
    QString nowISO8601 = QDateTime::currentDateTime().toTimeZone(QTimeZone::UTC).toString(Qt::ISODate);  // add days is for later
//    qDebug() << "Setting fence to: " << nowISO8601;

    prefsManager.SetrecentFenceDateTime(nowISO8601);  // e.g. "2018-01-01T01:23:45Z"
    recentFenceDateTime = QDateTime::fromString(prefsManager.GetrecentFenceDateTime(),
                                                          "yyyy-MM-dd'T'hh:mm:ss'Z'");
    // recentFenceDateTime.setTimeSpec(Qt::UTC);  // set timezone (all times are UTC) <-- deprecated soon
    recentFenceDateTime.setTimeZone(QTimeZone::UTC);  // set timezone (all times are UTC)

    firstTimeSongIsPlayed = true;   // this forces writing another record to the songplays DB when the song is next played
                                    //   so that the song will be marked Recent if it is played again after a Clear Recents,
                                    //   even though it was already loaded and played once before the Clear Recents.

    // update the song table
    pathsOfCalledSongs.clear(); // no songs have been played at this point
    reloadSongAges(ui->actionShow_All_Ages->isChecked());
}


// --------------------------
QString MainWindow::logFilePath;  // static members must be explicitly instantiated if in class scope

void MainWindow::customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QHash<QtMsgType, QString> msgLevelHash({{QtDebugMsg, "Debug"}, {QtInfoMsg, "Info"}, {QtWarningMsg, "Warning"}, {QtCriticalMsg, "Critical"}, {QtFatalMsg, "Fatal"}});
//    QByteArray localMsg = msg.toLocal8Bit();

    // QString dateTime = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);  // use ISO8601 UTC timestamps <-- deprecated soon
    QString dateTime = QDateTime::currentDateTime().toTimeZone(QTimeZone::UTC).toString(Qt::ISODate);  // use ISO8601 UTC timestamps
    QString logLevelName = msgLevelHash[type];
    QString txt = QString("%1 %2: %3 (%4)").arg(dateTime, logLevelName, msg, context.file);

    if (msg.contains("The provided value 'moz-chunked-arraybuffer' is not a valid enum value of type XMLHttpRequestResponseType")) {
//         This is a known warning that is spit out once per loaded PDF file.  It's just noise, so suppressing it from the debug.log file.
        return;
    }

    if (msg.contains("js:") ||
        txt.contains("minified/web/pdf.viewer.js") ||
        txt.contains("Warning: Populating font family aliases") ||
        txt.contains("Warning: Path override") ||
        txt.contains("Warning: #thumb_page_title")
        ) {
//         Suppressing lots of javascript noise from the debug.log file.
        return;
    }


    QFile outFile(logFilePath);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << ENDL;
    outFile.close();

    if (type == QtFatalMsg) {
        abort();
    }
}

void MainWindow::customMessageOutputQt(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type)
    Q_UNUSED(context)
//    QHash<QtMsgType, QString> msgLevelHash({{QtDebugMsg, "Debug"}, {QtInfoMsg, "Info"}, {QtWarningMsg, "Warning"}, {QtCriticalMsg, "Critical"}, {QtFatalMsg, "Fatal"}});
//    QByteArray localMsg = msg.toLocal8Bit();

//    QString dateTime = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);  // use ISO8601 UTC timestamps
//    QString logLevelName = msgLevelHash[type];
////    QString txt = QString("%1 %2: %3 (%4)").arg(dateTime, logLevelName, msg, context.file);
//    QString txt = QString("%1: %2").arg(logLevelName, msg);

    if (msg.contains("invalid nullptr parameter")) {
        // qDebug() << "FOUND IT.";
    }

    // suppress known warnings from QtCreator Application Output window
    if (msg.contains("The provided value 'moz-chunked-arraybuffer' is not a valid enum value of type XMLHttpRequestResponseType") ||
            msg.contains("js:") ||
//            txt.contains("Warning: #") ||
            msg.startsWith("#") ||
            msg.startsWith("skipping QEventPoint") ||
            msg.contains("Accessing QMediaDevices") ||
            msg.contains("GL Type: core_profile")
        ) {
        return;
    }

//    qDebug() << txt; // inside QtCreator, just log it to the console
    qDebug().noquote() << msg; // inside QtCreator, just log it to the console

}

// ----------------------------------------------------------------------
// sets the NowPlaying label, and color (based on whether it's a flashcall or not)
//   this is done many times, so factoring it out to here.
// flashcall defaults to false
void MainWindow::setNowPlayingLabelWithColor(QString s, bool flashcall) {
    if (flashcall != lastFlashcall) {
        // if what user sees is not what we want them to see...
        lastFlashcall = flashcall;
        if (flashcall) {
            // ui->nowPlayingLabel->setStyleSheet("QLabel { color : red; font-style: italic; }");
//            ui->darkTitle->setStyleSheet("QLabel { color : #D04040; font-style: italic; }");
#ifndef DEBUG_LIGHT_MODE
            ui->darkTitle->setStyleSheet("QLabel { color : red; font-style: italic; }"); // flashcall color
#else
            setProp(ui->darkTitle, "flashcall", true);
#endif
        } else {
            // ui->nowPlayingLabel->setStyleSheet("QLabel { color : black; font-style: normal; }");
#ifndef DEBUG_LIGHT_MODE
            ui->darkTitle->setStyleSheet("QLabel { color : #D9D9D9; font-style: normal; }"); // normal color
#else
            setProp(ui->darkTitle, "flashcall", false);
#endif
        }
    }
    if (ui->darkTitle->text() != s) {
//         // update ONLY if there is a change, to save CPU time in relayout
//         ui->nowPlayingLabel->setText(s);
// #ifdef DARKMODE
        ui->darkTitle->setText(s);
        // qDebug() << "WTF:" << s;
// #endif
    }
}

int MainWindow::getRsyncFileCount(QString sourceDir, QString destDir) {
#if defined(Q_OS_MAC)
    QString rsyncCommand = "/usr/bin/rsync";
//    qDebug() << "sourceDir" << sourceDir;
//    qDebug() << "destDir" << destDir;

    QStringList parmsDryRun;
    parmsDryRun << "-avn" << "--no-times" << "--no-perms" << "--size-only" << sourceDir << destDir;
    // parms1DryRun << "-avn" << "--modify-window=2" << sourceDir << destDir;

    QProcess rsync;
    rsync.start(rsyncCommand, parmsDryRun);
    rsync.waitForFinished();
    QString outputString(rsync.readAllStandardOutput());
    QStringList pieces = outputString.split( "\n" );

//    qDebug() << outputString << pieces << pieces4.length()-5;
//    qDebug() << pieces.length() << pieces;

    return(pieces.length());
#else
    Q_UNUSED(sourceDir)
    Q_UNUSED(destDir)
    return(0);
#endif
}

void MainWindow::on_actionMake_Flash_Drive_Wizard_triggered()
{
#if defined(Q_OS_MAC)
    MakeFlashDriveWizard wizard(this);
    int dialogCode = wizard.exec();

    if (dialogCode == QDialog::Accepted) {
        QString destVol = wizard.field("destinationVolume").toString();
//        qDebug() << "MAKE FLASH DRIVE WIZARD ACCEPTED." << destVol;

        // make the directory, if it doesn't already exist
        QDir dir(QString("/Volumes/%1/SquareDesk").arg(destVol));
        if (!dir.exists()) {
//            qDebug() << "Making it...";
            dir.mkpath(".");
        }

        // --------------------
        // DRY RUN for the app ----
        QString sourceDir1 = QCoreApplication::applicationDirPath();  // e.g.
        sourceDir1 += "/../..";
        QString destDir1 = QString("/Volumes/%1/SquareDesk/SquareDesk.app").arg(destVol); // e.g. /Volumes/PATRIOT/squareDesk

        int AppDryRunCount = getRsyncFileCount(sourceDir1, destDir1);
//        qDebug() << "AppDryRunCount: " << AppDryRunCount;

        // DRY RUN for the music files ----
        PreferencesManager prefs;
        QDir musicDir(prefs.GetmusicPath());
        QString sourceDir = musicDir.canonicalPath();  // e.g. /Users/mpogue/squareDanceMusic  (NOTE: no trailing '/')
        QString destDir = QString("/Volumes/%1/SquareDesk").arg(destVol); // e.g. /Volumes/PATRIOT/SquareDesk/ (NOTE: no trailing '/')

        int MusicDryRunCount = getRsyncFileCount(sourceDir, destDir);
//        qDebug() << "Music count:" << MusicDryRunCount;

        int filesToBeCopied = AppDryRunCount + MusicDryRunCount; // total files to be copied
        double p = 0;

        QProgressDialog progress("Copying SquareDesk...", "Abort Copy", 0, 10, this);
        progress.setRange(0,filesToBeCopied);
        progress.setValue(static_cast<int>(p));
        progress.setWindowModality(Qt::WindowModal);

        // COPY THE APP -----------
        QString rsyncCommand = "/usr/bin/rsync";
//        qDebug() << "Copying the SquareDesk.app files ....\n----------";

        QStringList appParms;
        appParms << "-av" << "--no-times" << "--no-perms" << "--size-only" << sourceDir1 << destDir1;

        QProcess rsync;
        rsync.start(rsyncCommand, appParms);

        if (!rsync.waitForStarted()) {
            return;
        }

        while(1) {
            if (rsync.waitForFinished(1000)) { // 1 sec at a time
                // rsync completed
                progress.setValue(AppDryRunCount); // finished copying just the app so far
                break;
            }

            while(rsync.canReadLine()) {
                QString line4 = QString::fromLocal8Bit(rsync.readLine());
                int count4 = line4.count("\n");
                p += count4;
//                qDebug() << count4 << line4;
                progress.setValue(static_cast<int>(p));
            }

            QCoreApplication::processEvents();
            if (progress.wasCanceled()) {
                // user manually cancelled the dialog
                return;  // get outta here, if the user cancelled the copy (no finish dialog)
            }
        }

        // COPY THE MUSIC FILES ==========
//        qDebug() << "Copying music files....\n----------";
        progress.setLabelText(QString("Copying Music..."));

        QStringList musicParms;

        musicParms << "-av" << "--no-times" << "--no-perms" << "--size-only"
                   << "--exclude" << "lock.txt"   // note: do NOT copy the 'lock.txt' file, because flash copy is never locked
                   << "--exclude" << "debug.log"  // note: do NOT copy the 'debug.log' file, because flash copy does not need it
                   << sourceDir << destDir;

        // ------- HERE IS THE REAL FILE COPY (takes a long time, maybe)
        QProcess rsync3;
        rsync3.start(rsyncCommand, musicParms);
        rsync3.setReadChannel(QProcess::StandardOutput);

        if (!rsync3.waitForStarted()) {
            return;
        }

        while(1) {
            if (rsync3.waitForFinished(1000)) { // 1 sec at a time
                // rsync completed
                progress.setValue(filesToBeCopied);  // all done!
//                qDebug() << "rsync of music finished.";
                break;
            }

            while(rsync3.canReadLine()) {
                QString line = QString::fromLocal8Bit(rsync3.readLine());
                int count = line.count("\n");
                p += count;
//                qDebug() << count << line;
                progress.setValue(static_cast<int>(p));
            }

            QCoreApplication::processEvents();
            if (progress.wasCanceled()) {
                // user manually cancelled the dialog
                return;  // get outta here, and don't show the final dialog
            }
        }

//        qDebug() << "Done with copy.\n----------";

        // FINAL DIALOG: copy is done, remember to Eject!
        //    TODO: provide a button to eject it!
        QMessageBox msgBox;
        msgBox.setText(QString("The SquareDesk application and your Music Directory have been copied to '%1'.").arg(destVol));
        msgBox.setInformativeText("NOTE: Be sure to use Finder to eject the flash drive before unplugging it.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();

    } // if accepted
#endif
}


void MainWindow::handleDurationBPM() {
//    qDebug() << "***** handleDurationBPM()";
    int length_sec = static_cast<int>(cBass->FileLength);
    int songBPM = static_cast<int>(round(cBass->Stream_BPM));  // libbass's idea of the BPM
//    qDebug() << "***** handleDurationBPM(): " << songBPM;

// If the MP3 file has an embedded TBPM frame in the ID3 tag, then it overrides the libbass auto-detect of BPM
    double songBPM_ID3 = getID3BPM(currentMP3filenameWithPath);  // returns 0.0, if not found or not understandable

    if (songBPM_ID3 != 0.0) {
        // qDebug() << "handleDurationBPM: file has ID3v2 TBPM, so slider center set to:" << songBPM;
        songBPM = static_cast<int>(songBPM_ID3);
        tempoIsBPM = true;  // this song's tempo is BPM, not %
    }

    baseBPM = songBPM;  // remember the base-level BPM of this song, for when the Tempo slider changes later

    // Intentionally compare against a narrower range here than BPM detection, because BPM detection
    //   returns a number at the limits, when it's actually out of range.
    // Also, turn off BPM for xtras (they are all over the place, including round dance cues, which have no BPM at all).
    //
    // TODO: make the types for turning off BPM detection a preference
    if ((songBPM>=125-15) && (songBPM<=125+15) && currentSongCategoryName != "extras") {
        tempoIsBPM = true;
        // ui->currentTempoLabel->setText(QString::number(songBPM) + " BPM (100%)"); // initial load always at 100%

        // ui->tempoSlider->setMinimum(songBPM-15);
        // ui->tempoSlider->setMaximum(songBPM+15);

        ui->darkTempoSlider->setMinimum(songBPM-15);
        ui->darkTempoSlider->setMaximum(songBPM+15);

        // ui->tempoSlider->setValue(songBPM);  // qDebug() << "handleDurationBPM set tempo slider to: " << songBPM;
        // Necessary because we've changed the minimum and maximum, but haven't forced a redraw, and the on_... only changes the value if it's changed. This forces a redraw.
        ui->darkTempoSlider->setValue(songBPM);
        // emit ui->tempoSlider->valueChanged(songBPM);  // fixes bug where second song with same BPM doesn't update songtable::tempo

        // ui->tempoSlider->SetOrigin(songBPM);    // when double-clicked, goes here (MySlider function)

        ui->darkTempoSlider->setDefaultValue(songBPM);  // when double-clicked, goes here

        // ui->tempoSlider->setEnabled(true);
//        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
//                                 ", base tempo: " + QString::number(songBPM) + " BPM");
    }
    else {
        tempoIsBPM = false;
        // if we can't figure out a BPM, then use percent as a fallback (centered: 100%, range: +/-20%)
        ui->darkTempoSlider->setMinimum(100-20);        // allow +/-20%
        ui->darkTempoSlider->setMaximum(100+20);

        // see comment above about forcing a redraw
        ui->darkTempoSlider->setValue(100);
        // emit ui->tempoSlider->valueChanged(100);  // fixes bug where second song with same 100% doesn't update songtable::tempo
        // ui->tempoSlider->SetOrigin(100);  // when double-clicked, goes here

        ui->darkTempoSlider->setDefaultValue(100);  // when double-clicked, goes here, this is "100%"

        // ui->tempoSlider->setEnabled(true);
//        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
//                                 ", base tempo: 100%");
    }

    // NOTE: we need to set the bounds BEFORE we set the actual positions
//    qDebug() << "MainWindow::handleDurationBPM: length_sec = " << length_sec;
    ui->dateTimeEditIntroTime->setTimeRange(QTime(0,0,0,0), QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*length_sec+0.5)));
    ui->dateTimeEditOutroTime->setTimeRange(QTime(0,0,0,0), QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*length_sec+0.5)));

    bool isSinger = currentSongIsSinger || currentSongIsVocal;

    if (darkmode) {
        ui->darkStartLoopTime->setTimeRange(QTime(0,0,0,0), QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*length_sec+0.5)));
        ui->darkEndLoopTime->setTimeRange(QTime(0,0,0,0), QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*length_sec+0.5)));
        ui->darkSeekBar->setSingingCall(currentSongIsSinger || currentSongIsVocal);
    }

    // ui->seekBar->SetSingingCall(isSinger); // if singing call, color the seek bar
    ui->seekBarCuesheet->SetSingingCall(isSinger); // if singing call, color the seek bar

    if (currentSongIsPatter || currentSongIsUndefined) {
        on_loopButton_toggled(currentSongIsPatter); // default is to ENABLE LOOPING if type is patter (but defaults to OFF if Undefined type)
        ui->pushButtonSetIntroTime->setText("Start Loop");
        ui->pushButtonSetOutroTime->setText("End Loop");
        ui->pushButtonTestLoop->setHidden(false);
        if (darkmode) {
            ui->darkTestLoopButton->setHidden(false);
        }
        // analogClock->setSingingCallSection("");
        ui->theSVGClock->setSingingCallSection("");
    } else {
        // Either it's a SINGER, VOCAL (also a type of SINGER), or XTRA
        on_loopButton_toggled(false); // default is to loop, if type is patter
        ui->pushButtonSetIntroTime->setText("In");
        ui->pushButtonSetOutroTime->setText("Out");
    }

}

void MainWindow::on_actionSquareDesk_Help_triggered()
{
    QString linkToQuickReference = "https://github.com/mpogue2/SquareDesk/wiki/SquareDesk-Quick-Reference-Manual";
    QDesktopServices::openUrl(QUrl(linkToQuickReference));
}

void MainWindow::on_actionSD_Help_triggered()
{
    QString musicDirPath = prefsManager.GetmusicPath();
    QString referenceDir = musicDirPath + "/reference";

    // ---------------
    // Find something that looks like the SD doc (we probably copied it there)
    bool hasSDpdf = false;
    QString pathToSDdoc;

    QDirIterator it(referenceDir);
    while(it.hasNext()) {
        it.next();
        QString s1 = it.fileName();
        // if 123.SD.pdf or SD.pdf, then do NOT copy one in as 195.SD.pdf
        static QRegularExpression re10("^[0-9]+\\.SD_V[0-9]+\\.[0-9]+.pdf$"); // new hotness
        // static QRegularExpression re11("^[0-9]+\\.SD.pdf$"); // old
        // static QRegularExpression re12("^SD.pdf$");  // oldest
        if (s1.contains(re10)) {
            hasSDpdf = true;
            pathToSDdoc = QString("file://") + referenceDir + "/" + s1;
            break;
        }
    }

    if (hasSDpdf) {
        // qDebug() << "FOUND SD DOC:" << pathToSDdoc;
        QDesktopServices::openUrl(QUrl(pathToSDdoc, QUrl::TolerantMode));
    }
}

void MainWindow::on_actionReport_a_Bug_triggered()
{
    QString pathToGithubSquaredeskIssues = "https://github.com/mpogue2/SquareDesk/issues";
    QDesktopServices::openUrl(QUrl(pathToGithubSquaredeskIssues, QUrl::TolerantMode));
}


// SNAP actions (mutually exclusive) ---------------------
void MainWindow::on_actionDisabled_triggered()
{
    prefsManager.Setsnap("disabled");
}

void MainWindow::on_actionNearest_Beat_triggered()
{
    prefsManager.Setsnap("beat");
    if (songLoaded) {
        // if a song is loaded, and we're just switching to BEAT detect ON,
        //   initiate a VAMP beat detection in the background (it'll be done after 2 sec)
        double maybeError = cBass->snapToClosest(0.1, GRANULARITY_BEAT);  // intentionally 0.1, so that -0.1 is error flag
        if (maybeError < 0.0) {
           // error! vamp didn't work
//           qDebug() << "on_actionNearest_Beat_triggered: snap didn't work, disabling snapping.";
           ui->actionDisabled->setChecked(true);
        }
    }
}

void MainWindow::on_actionNearest_Measure_triggered()
{
    prefsManager.Setsnap("measure");
    if (songLoaded) {
        // if a song is loaded, and we're just switching to BEAT detect ON,
        //   initiate a VAMP beat detection in the background (it'll be done after 2 sec)
        double maybeError = cBass->snapToClosest(0.1, GRANULARITY_MEASURE);  // intentionally 0.1, so that -0.1 is error flag
        if (maybeError < 0.0) {
           // error! vamp didn't work
//           qDebug() << "on_actionNearest_Beat_triggered: snap didn't work, disabling snapping.";
           ui->actionDisabled->setChecked(true);
        }
    }
}

void MainWindow::on_actionOpen_Audio_File_triggered()
{
    // This is Music > Open Audio File
    on_actionOpen_MP3_file_triggered();
}

// DARK MODE CONTROLS ----------------------
void MainWindow::on_darkStopButton_clicked()
{
    ui->darkPlayButton->setIcon(*darkPlayIcon);  // change PAUSE to PLAY

    cBass->Stop();                 // Stop playback, rewind to the beginning
    cBass->StopAllSoundEffects();  // and, it also stops ALL sound effects

    if (loudMaxPlugin != nullptr) {
        // qDebug() << "Clearing LoudMax's buffers...";
        loudMaxPlugin->reset();  // clear out the LoudMax audio buffers
    }

    setNowPlayingLabelWithColor(currentSongTitle);
    
    // Update Now Playing info for remote control  
    updateNowPlayingMetadata();

    ui->seekBarCuesheet->setValue(0);
        ui->darkSeekBar->setValue(0);

    int cindex = ui->tabWidget->currentIndex();  // get index of tab, so we can see which it is
    bool tabIsSD = (ui->tabWidget->tabText(cindex) == "SD");

     // if it's the SD tab, do NOT change focus to songTable, leave it in the Current Sequence pane
    if (!tabIsSD) {
        ui->darkSongTable->setFocus();
    } else {
        qDebug() << "stopButtonClicked: Tab was SD, so NOT changing focus to songTable";
    }
}


void MainWindow::on_darkPlayButton_clicked()
{
    PerfTimer t("MainWindow::on_playButtonClicked", __LINE__);
    if (!songLoaded) {
        return;  // if there is no song loaded, no point in toggling anything.
    }

    int cindex = ui->tabWidget->currentIndex();  // get index of tab, so we can see which it is
    bool tabIsSD = (ui->tabWidget->tabText(cindex) == "SD");

    uint32_t Stream_State = cBass->currentStreamState();
    if (Stream_State != BASS_ACTIVE_PLAYING) {
        cBass->Play();  // currently stopped or paused or stalled, so try to start playing

        on_UIUpdateTimerTick();  // do an update, so we pick up clock coloring quicker

        // randomize the Flash Call, if PLAY (but not PAUSE) is pressed
        randomizeFlashCall();

        if (firstTimeSongIsPlayed)
        {
            PerfTimer t("MainWindow::on_playButtonClicked firstTimeSongIsPlayed", __LINE__);

            firstTimeSongIsPlayed = false;
            saveCurrentSongSettings();
            if (!ui->actionDon_t_Save_Plays->isChecked()) {
                ui->darkSongTable->setSortingEnabled(false);

                int row;
                row = darkGetSelectionRowForFilename(currentMP3filenameWithPath);

                if (row != -1)
                {
                        ui->darkSongTable->item(row, kAgeCol)->setText("0");
                        ui->darkSongTable->item(row, kAgeCol)->setTextAlignment(Qt::AlignCenter);

                        ui->darkSongTable->item(row, kRecentCol)->setText(ageToRecent("0"));
                        ui->darkSongTable->item(row, kRecentCol)->setTextAlignment(Qt::AlignCenter);
                }

                ui->darkSongTable->setSortingEnabled(true);
            }

            // qDebug() << "on_playButton_clicked(): " << switchToLyricsOnPlay << songTypeNamesForSinging << currentSongTypeName;

            if (switchToLyricsOnPlay && (currentSongIsSinger || currentSongIsVocal))
            {
                // switch to Lyrics tab ONLY for singing calls or vocals
                ui->tabWidget->setCurrentIndex(lyricsTabNumber);
            }
        }

        // If we just started playing, clear focus from all widgets
        if (QApplication::focusWidget() != nullptr) {
            oldFocusWidget = QApplication::focusWidget();

            if (!tabIsSD) {
//                qDebug() << "playButtonClicked: Tab was NOT SD, so clearing focus from existing widget";
                QApplication::focusWidget()->clearFocus();  // we don't want to continue editing the search fields after a STOP
                                                            //  or it will eat our keyboard shortcuts (like P, J, K, period, etc.)
            } else {
//                qDebug() << "playButtonClicked: Tab was SD, so NOT clearing focus from existing widget";
            }
        }
        ui->darkPlayButton->setIcon(*darkPauseIcon);  // change PLAY to PAUSE
        ui->actionPlay->setText("Pause");
        
        // Update Now Playing info for remote control  
        updateNowPlayingMetadata();

        // ui->songTable->setFocus(); // while playing, songTable has focus

        // if it's the SD tab, do NOT change focus to songTable, leave it in the Current Sequence pane
        if (!tabIsSD) {
//            qDebug() << "playButtonClicked: Tab was NOT SD, so changing focus to songTable";
            ui->darkSongTable->setFocus();
        } else {
//            qDebug() << "playButtonClicked: Tab was SD, so NOT changing focus to songTable";
        }

    }
    else {
        // TODO: we might want to restore focus here....
        // currently playing, so pause playback
        cBass->Pause();
        // ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
        ui->darkPlayButton->setIcon(*darkPlayIcon);  // change PAUSE to PLAY
        ui->actionPlay->setText("Play");
        // qDebug() << "on_play" << currentSongTitle;
        setNowPlayingLabelWithColor(currentSongTitle);

        // Update Now Playing info for remote control  
        updateNowPlayingMetadata();

        // restore focus
        if (oldFocusWidget != nullptr  && !tabIsSD) { // only set focus back on pause when tab is NOT SD
            oldFocusWidget->setFocus();
        }

    }

}


void MainWindow::on_darkStartLoopButton_clicked()
{
    on_pushButtonSetIntroTime_clicked();
}


void MainWindow::on_darkEndLoopButton_clicked()
{
    on_pushButtonSetOutroTime_clicked();
}


void MainWindow::on_darkTestLoopButton_clicked()
{
    on_pushButtonTestLoop_clicked();
}


void MainWindow::on_darkTrebleKnob_valueChanged(int value)
{
    // FOR NOW: the range of the knob is alway 0 - 100
    //  we translate to -15 to 15 for the regular slider

    int translatedValue = round(-15 + 30 * (value/100.0)); // rounded to nearest int

    cBass->SetEq(2, static_cast<double>(translatedValue));
    saveCurrentSongSettings();
}

void MainWindow::on_darkMidKnob_valueChanged(int value)
{
    // FOR NOW: the range of the knob is alway 0 - 100
    //  we translate to -15 to 15 for the regular slider
    int translatedValue = round(-15 + 30 * (value/100.0)); // rounded to nearest int

    cBass->SetEq(1, static_cast<double>(translatedValue));
    saveCurrentSongSettings();
}

void MainWindow::on_darkBassKnob_valueChanged(int value)
{
    // FOR NOW: the range of the knob is alway 0 - 100
    //  we translate to -15 to 15 for the regular slider
    int translatedValue = round(-15 + 30 * (value/100.0)); // rounded to nearest int

    cBass->SetEq(0, static_cast<double>(translatedValue));
    saveCurrentSongSettings();
}

void MainWindow::on_darkSearch_textChanged(const QString &s)
{
    QStringList pieces = s.split(u':'); // use ":" to delimit type:label:title search fields
    int count = pieces.length();

    // qDebug() << pieces.length() << count << pieces;

    // EXAMPLES:
    //    foo = search for types OR labels OR titles containing 'foo'
    //    s::foo = search for singing calls containing 'foo' (NOTE the double colon, because "label" is blank
    //    s:riv = search for singing calls from Riverboat
    //    :riv:world = search for anything (patter or singing etc.) from Riverboat containing 'world'
    //    p:riv:bar  = search for patter from riverboat containing 'bar'

    switch (count) {
        case 0:
            // e.g. ""
            // ui->typeSearch->setText("");
            // ui->labelSearch->setText("");
            // ui->titleSearch->setText("");
            typeSearch = "";
            labelSearch = "";
            titleSearch = "";
            searchAllFields = true;  // search is OR, not AND (really doesn't matter here)
            break;
        case 1:
            // e.g. "foo" = search ALL 3 fields for "foo"
            // ui->typeSearch->setText("");
            // ui->labelSearch->setText("");
            // ui->titleSearch->setText(pieces[0]);
            typeSearch = "";
            labelSearch = "";
            titleSearch = pieces[0];
            searchAllFields = true;  // search is OR, not AND
            break;
        case 2:
            // e.g. "p:riv" = search for "patter" records from "Riverboat"
            // ui->typeSearch->setText(pieces[0]);
            // ui->labelSearch->setText(pieces[1]);
            // ui->titleSearch->setText("");
            typeSearch = pieces[0];
            labelSearch = pieces[1];
            titleSearch = "";
            searchAllFields = false;  // search is OR, not AND
            break;
        default: // 3 or more (only look at first 3 pieces, if 4 or more)
            // e.g. "p:riv:hello" = search for "patter" records from "Riverboat" where the title contains "hello"
            // ui->typeSearch->setText(pieces[0]);
            // ui->labelSearch->setText(pieces[1]);
            // ui->titleSearch->setText(pieces[2]);
            typeSearch = pieces[0];
            labelSearch = pieces[1];
            titleSearch = pieces[2];
            searchAllFields = false;  // search is OR, not AND
            break;
    }

    darkFilterMusic();
}


void MainWindow::on_treeWidget_itemSelectionChanged()
{
    QList<QTreeWidgetItem *> items = ui->treeWidget->selectedItems();
    if (items.length() > 0) {
        QTreeWidgetItem *thisItem = items[0];
        QTreeWidgetItem *maybeParentsItem = thisItem->parent();

        if (thisItem != nullptr) {
            QString theText = thisItem->text(0);
            if (!doNotCallDarkLoadMusicList) {
                ui->darkSearch->setText(""); // clear the search to show all Local Tracks
                if (theText == "Tracks") {
                    darkLoadMusicList(pathStack, "Tracks", true, false);  // show MUSIC
                    return;
                } else if (theText == "Apple Music") {
                    darkLoadMusicList(pathStackApplePlaylists, "Apple Music", true, false);  // show APPLE MUSIC playlists
                    return;
                } else if (theText == "Playlists") {
                    darkLoadMusicList(pathStackPlaylists, "Playlists", true, false);  // show PLAYLISTS
                    return;
                }
            }
        }

        // let's get the full path of the thing the user clicked on, e.g. "Playlists/Jokers/2024/Jokers_2024.01.12"
        // thisItem is a pointer to a QTreeWidgetItem, which is what we clicked on

        QString treePath = thisItem->text(0);
        QTreeWidgetItem *parent = thisItem->parent();
        while (parent != nullptr) {
            treePath = parent->text(0) + "/" + treePath;
            parent = parent->parent();
        }

        // qDebug() << "childCount: " << thisItem->childCount();

        if (thisItem->childCount() > 0) {
            treePath += "/"; // SLASH AT THE END MEANS THIS IS NOT A LEAF, SO USE treePath/* to search
        }

        // qDebug() << "FINAL TREEPATH:" << treePath;

        QString shortTreePath = treePath;
        const QRegularExpression firstItem("^.*?/");
        shortTreePath.replace(firstItem, "");
        // qDebug() << "on_treeWidget_itemSelectionChanged -- TREEPATH/SHORT TREE PATH:" << treePath << shortTreePath;

        // clicked on some item (not a top level category) -------
        if (maybeParentsItem == nullptr) {
//            qDebug() << "PARENT ITEM:" << thisItem->text(0);
            ui->darkSearch->setText(""); // clear the search to show all Tracks
            if (!doNotCallDarkLoadMusicList) {
                currentTreePath = "Tracks/";
                darkLoadMusicList(pathStack, "Tracks", true, false);  // show MUSIC TRACKS
            }
        } else {
            ui->darkSearch->setText(""); // clear the search to show all Tracks
            if (!doNotCallDarkLoadMusicList) {
                currentTreePath = treePath; // e.g. "Tracks/patter", "Playlists/CPSD/" "Apple Music/CuriousBlend"
                if (treePath.startsWith("Tracks")) {
                    darkLoadMusicList(pathStack, shortTreePath, true, false);  // show MUSIC TRACKS
                } else if (treePath.startsWith("Playlists")) {
                    darkLoadMusicList(pathStackPlaylists, shortTreePath, true, false);  // show MUSIC TRACKS
                } else if (treePath.startsWith("Apple Music")) {
                    darkLoadMusicList(pathStackApplePlaylists, shortTreePath, true, false);  // show MUSIC TRACKS
                } else {
                }
            }
        }
    } else {
//        qDebug() << "empty TreeWidget selection";
    }
}

void MainWindow::on_darkSongTable_itemDoubleClicked(QTableWidgetItem *item)
{
    PerfTimer t("on_darkSongTable_itemDoubleClicked", __LINE__);

    on_darkStopButton_clicked();  // if we're loading a new MP3 file, stop current playback
    saveCurrentSongSettings();

    t.elapsed(__LINE__);

    int row = item->row();
    QString pathToMP3 = ui->darkSongTable->item(row,kPathCol)->data(Qt::UserRole).toString();

    QString songTitle = getTitleColTitle(ui->darkSongTable, row);
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->darkSongTable->item(row,kTypeCol)->text().toLower();
    QString songLabel = ui->darkSongTable->item(row,kLabelCol)->text().toLower();

    // these must be up here to get the correct values...
    QString pitch = ui->darkSongTable->item(row,kPitchCol)->text();
    QString tempo = ui->darkSongTable->item(row,kTempoCol)->text();
    QString number = ui->darkSongTable->item(row, kNumberCol)->text();

    targetPitch = pitch;  // save this string, and set pitch slider AFTER base BPM has been figured out
    targetTempo = tempo;  // save this string, and set tempo slider AFTER base BPM has been figured out
    targetNumber = number; // save this, because tempo changes when this is set are playlist modifications, too

    //    qDebug() << "on_songTable_itemDoubleClicked: " << songTitle << songType << songLabel << pitch << tempo;

    t.elapsed(__LINE__);

    loadMP3File(pathToMP3, songTitle, songType, songLabel);

    t.elapsed(__LINE__);

    // set the LOADED flag -----
    for (int slot = 0; slot < 3; slot++) {
        MyTableWidget *tables[] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};
        MyTableWidget *table = tables[slot];
        for (int i = 0; i < table->rowCount(); i++) {
            if (table->item(i, 5)->text() == "1") {
                // clear the arrows out of the other tables
                QString currentTitleTextWithoutArrow = table->item(i, 1)->text().replace(editingArrowStart, "");
                table->item(i, 1)->setText(currentTitleTextWithoutArrow);

                QFont currentFont = table->item(i, 1)->font(); // font goes to neutral (not bold or italic, and normal size) for NOT-loaded items
                currentFont.setBold(false);
                currentFont.setItalic(false);
//                    currentFont.setPointSize(currentFont.pointSize() - 1);
                table->item(i, 0)->setFont(currentFont);
                table->item(i, 1)->setFont(currentFont);
                table->item(i, 2)->setFont(currentFont);
                table->item(i, 3)->setFont(currentFont);
            }
            table->item(i, 5)->setText(""); // clear out the old table
        }
    }

    sourceForLoadedSong = ui->darkSongTable; // THIS is where we got the currently loaded song (this is the NEW table)

    // these must be down here, to set the correct values...
    int pitchInt = pitch.toInt();
    // ui->pitchSlider->setValue(pitchInt);
    ui->darkPitchSlider->setValue(pitchInt);

    // on_pitchSlider_valueChanged(pitchInt); // manually call this, in case the setValue() line doesn't call valueChanged() when the value set is
        //   exactly the same as the previous value.  This will ensure that cBass->setPitch() gets called (right now) on the new stream.

    if (ui->actionAutostart_playback->isChecked()) {
        on_darkPlayButton_clicked();
    }

//    sourceForLoadedSong = ui->darkSongTable; // THIS is where we got the currently loaded song

    t.elapsed(__LINE__);
}

void MainWindow::on_darkStartLoopTime_timeChanged(const QTime &time)
{
    QTime otherTime = (const QTime)(ui->dateTimeEditIntroTime->time());
    qint64 dTime_ms = abs(otherTime.msecsTo(time));

    // bool isSingingCall = songTypeNamesForSinging.contains(currentSongType) ||
    //                      songTypeNamesForCalled.contains(currentSongType);

    bool isSingingCall = currentSongIsSinger || currentSongIsVocal;

    if (dTime_ms != 0) {
        on_dateTimeEditIntroTime_timeChanged(time); // just call over there to set the times on both
    }

    if (isSingingCall && !loadingSong) {
        ui->darkSeekBar->updateBgPixmap((float*)1, 1);  // update the bg pixmap, in case it was a singing call
    }
}

void MainWindow::on_darkEndLoopTime_timeChanged(const QTime &time)
{
    QTime otherTime = (const QTime)(ui->dateTimeEditOutroTime->time());
    qint64 dTime_ms = abs(otherTime.msecsTo(time));

    // bool isSingingCall = songTypeNamesForSinging.contains(currentSongType) ||
    //                      songTypeNamesForCalled.contains(currentSongType);

    bool isSingingCall = currentSongIsSinger || currentSongIsVocal;

    if (dTime_ms != 0) {
        on_dateTimeEditOutroTime_timeChanged(time); // just call over there to set the times on both
    }

    if (isSingingCall && !loadingSong) {
        ui->darkSeekBar->updateBgPixmap((float*)1, 1);  // update the bg pixmap, in case it was a singing call
    }
}


void MainWindow::on_toggleShowPaletteTables_toggled(bool checked)
{
    Q_UNUSED(checked)
}

void MainWindow::on_darkSongTable_itemSelectionChanged()
{
    ui->playlist1Table->blockSignals(true);
    ui->playlist2Table->blockSignals(true);
    ui->playlist3Table->blockSignals(true);

    ui->playlist1Table->clearSelection();
    ui->playlist2Table->clearSelection();
    ui->playlist3Table->clearSelection();

    ui->playlist1Table->blockSignals(false);
    ui->playlist2Table->blockSignals(false);
    ui->playlist3Table->blockSignals(false);
}

// ===============================================
void MainWindow::customPlaylistMenuRequested(QPoint pos) {
    Q_UNUSED(pos)

    QMenu *plMenu = new QMenu();

    plMenu->setProperty("theme", currentThemeString);

    int whichSlot = 0;
    MyTableWidget *theTableWidget = ui->playlist1Table;

    if (sender()->objectName() == "playlist2Label") {
        whichSlot = 1;
        theTableWidget = ui->playlist2Table;
    } else if (sender()->objectName() == "playlist3Label") {
        whichSlot = 2;
        theTableWidget = ui->playlist3Table;
    }

    plMenu->addAction(QString("Load Playlist..."), // to THIS slot
                      [whichSlot, this]() {
                          Q_UNUSED(this)
                          loadPlaylistFromFileToSlot(whichSlot);
                      }
                      );

    // Add "Load Recent Playlist" submenu
    QString recentPlaylistsString = prefsManager.GetlastNPlaylistsLoaded();
    if (!recentPlaylistsString.isEmpty()) {
        QStringList recentPlaylists = recentPlaylistsString.split(";", Qt::SkipEmptyParts);
        if (!recentPlaylists.isEmpty()) {
            QMenu *recentMenu = new QMenu("Load Recent Playlist");
            recentMenu->setProperty("theme", currentThemeString);
            
            for (const QString &relativePath : recentPlaylists) {
                if (!relativePath.isEmpty()) {
                    // Use the relative path as the display name
                    QString displayName = relativePath;
                    
                    recentMenu->addAction(displayName, [this, whichSlot, relativePath]() {
                        // Reconstruct the full path for loading
                        QString fullPath = musicRootPath + "/playlists/" + relativePath + ".csv";
                        
                        int songCount;
                        loadPlaylistFromFileToPaletteSlot(fullPath, whichSlot, songCount);
                        
                        // Update the recent playlists list
                        updateRecentPlaylistsList(fullPath);
                    });
                }
            }
            
            plMenu->addMenu(recentMenu);
            
            // Add "Clear Recent Playlists" menu item (only when there are recent playlists)
            plMenu->addAction(QString("Clear Recent Playlists"), [this]() {
                prefsManager.SetlastNPlaylistsLoaded("");
            });
        }
    }

    if (relPathInSlot[whichSlot] != "" &&
            !relPathInSlot[whichSlot].startsWith("/tracks/") &&
            !relPathInSlot[whichSlot].startsWith("/Apple Music/")
            ) {
        // context menu only available if something is actually in this slot

        // PLAYLIST IS IN SLOT ---------
        QString playlistFilePath = musicRootPath + "/playlists/" + relPathInSlot[whichSlot] + ".csv";
        QString playlistShortName = playlistFilePath.split('/').last().replace(".csv","");

//        qDebug() << "customPlaylistMenuRequested:" << playlistFilePath << playlistShortName << relPathInSlot[whichSlot];

        plMenu->addAction(QString("Save Playlist As..."), // from THIS slot
                          [whichSlot, this]() {
                              Q_UNUSED(this)
                              // qDebug() << "Save Playlist As..." << whichSlot;
                              saveSlotAsPlaylist(whichSlot);
                          }
                          );

        plMenu->addAction(QString("Print Playlist..."), // to THIS slot
                          [whichSlot, this]() {
                              Q_UNUSED(this)
                              // qDebug() << "Print Playlist..." << whichSlot;
                              printPlaylistFromSlot(whichSlot);
                          }
                          );

        plMenu->addSeparator();

        plMenu->addAction(QString("Edit '%1' in text editor").arg(relPathInSlot[whichSlot]),
                          [playlistFilePath]() {
#if defined(Q_OS_MAC)
                              QStringList args;
                              args << "-e";
                              args << "tell application \"TextEdit\"";
                              args << "-e";
                              args << "activate";
                              args << "-e";
                              args << "open POSIX file \"" + playlistFilePath + "\"";
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

        // NOTE: RELOAD DOESN'T GET BACK TO THE ORIGINAL STATE, IF A SONG WAS LOADED FROM THAT PLAYLIST.
        //  SO, I AM DISABLING THIS FOR NOW.  IT WAS ONLY USED FOR ONE USE CASE:
        //  MANUAL EDITING THE FILE VIA "EDIT IN TEXT EDITOR", and THEN HIT RELOAD.
        //  SINCE MANUAL EDITING IS INFREQUENT, JUST RESTART THE APP TO GET BACK TO A KNOWN STATE.
//         plMenu->addAction(QString("Reload '%1'").arg(relPathInSlot[whichSlot]),
//                           [this, playlistFilePath, whichSlot]() {
// //                              qDebug() << "RELOAD PLAYLIST: " << playlistFilePath;
//                               int songCount;
//                               this->loadPlaylistFromFileToPaletteSlot(playlistFilePath, whichSlot, songCount);
//                           });

        plMenu->addSeparator();

        plMenu->addAction(QString("Remove from Palette"),
                          [this, whichSlot]() {
                              saveSlotNow(whichSlot);  // if modified, save it now
                              clearSlot(whichSlot);    // clear table and label, mark not modified, and no relPath
                              switch (whichSlot) {
                                  case 0: prefsManager.SetlastPlaylistLoaded(""); break;
                                  case 1: prefsManager.SetlastPlaylistLoaded2(""); break;
                                  case 2: prefsManager.SetlastPlaylistLoaded3(""); break;
                                  default: break;
                              }
                          }
                          );


    } else {
        // NOTHING OR TRACKS FILTER OR APPLE MUSIC IS IN SLOT ---------

        // if (!relPathInSlot[whichSlot].startsWith("/tracks/")) {
        if (relPathInSlot[whichSlot] == "") {
            // it's an UNTITLED PLAYLIST ---------
            int playlistRowCount = theTableWidget->rowCount();

            if (playlistRowCount > 0 ) {
                plMenu->addAction(QString("Save 'Untitled playlist' As..."),
                                  [whichSlot, this]() {
                                      Q_UNUSED(this)
//                                      qDebug() << "SAVE AS..." << whichSlot;
                                      saveSlotAsPlaylist(whichSlot);
                                  }
                                  );
            }
            if (theTableWidget->rowCount() > 0) {
                plMenu->addSeparator();

                plMenu->addAction(QString("Remove All Songs from 'Untitled playlist'"),
                                  [this, whichSlot]() {
                                      // removes without asking to save it
                                      clearSlot(whichSlot);    // then clear out the slot, mark not modified, and no relPathInSlot
                                  }
                                  );
            }
        } else if (relPathInSlot[whichSlot].startsWith("/Apple Music/")) {
            // IT'S AN APPLE MUSIC FILTER
            int playlistRowCount = theTableWidget->rowCount();

            // NOT NEEDED: just multiple select the Apple Music songs, and drag to an Untitled Playlist, then Right-click Save As...
            // if (playlistRowCount > 0 ) {
            //     plMenu->addAction(QString("Save Apple Music List As SquareDesk Playlist..."),
            //                       [whichSlot, this]() {
            //                           Q_UNUSED(this)
            //                           qDebug() << "TBD: SAVE APPLE MUSIC AS SQUAREDESK PLAYLIST..." << whichSlot;
            //                           // saveSlotAsPlaylist(whichSlot);
            //                       }
            //                       );
            // }
            // plMenu->addSeparator();

            if (playlistRowCount > 0 ) {
                plMenu->addAction(QString("Update Apple Music List from Apple Music"),
                                  [whichSlot, this]() {
                                      Q_UNUSED(this)
                                      // qDebug() << "Update Apple Music List from Apple Music" << whichSlot;
                                      getAppleMusicPlaylists(); // update from Apple Music
                                      // saveSlotAsPlaylist(whichSlot);
                                      // now filter into the table
                                      int songCount;
                                      loadPlaylistFromFileToPaletteSlot(relPathInSlot[whichSlot], whichSlot, songCount);
                                  }
                                  );
            }
            plMenu->addSeparator();

            plMenu->addAction(QString("Remove from Palette"),
                              [this, whichSlot]() {
                                  saveSlotNow(whichSlot);  // save contents of that slot, if it was modified
                                  clearSlot(whichSlot);    // then clear out the slot, mark not modified, and no relPathInSlot

                                  // and do NOT reload this slot when app starts
                                  switch (whichSlot) {
                                      case 0: prefsManager.SetlastPlaylistLoaded(""); break;
                                      case 1: prefsManager.SetlastPlaylistLoaded2(""); break;
                                      case 2: prefsManager.SetlastPlaylistLoaded3(""); break;
                                      default: break;
                                  }
                              }
                              );

        } else {
            // it's a TRACK filter ------------
            plMenu->addSeparator();

            plMenu->addAction(QString("Remove from Palette"),
                              [this, whichSlot]() {
                                  saveSlotNow(whichSlot);  // save contents of that slot, if it was modified
                                  clearSlot(whichSlot);    // then clear out the slot, mark not modified, and no relPathInSlot

                                  // and do NOT reload this slot when app starts
                                  switch (whichSlot) {
                                      case 0: prefsManager.SetlastPlaylistLoaded(""); break;
                                      case 1: prefsManager.SetlastPlaylistLoaded2(""); break;
                                      case 2: prefsManager.SetlastPlaylistLoaded3(""); break;
                                      default: break;
                                  }
                              }
                              );
        }
    }

    plMenu->popup(QCursor::pos());
    plMenu->exec();

    delete(plMenu);
}

void MainWindow::on_playlist1Table_itemSelectionChanged()
{
//    qDebug() << "playlist1Table_itemSelectionChanged";

    ui->playlist2Table->blockSignals(true);
    ui->playlist3Table->blockSignals(true);

    ui->playlist2Table->clearSelection();
    ui->playlist3Table->clearSelection();

    ui->playlist2Table->blockSignals(false);
    ui->playlist3Table->blockSignals(false);

    // ------
    // ui->songTable->blockSignals(true);
    ui->darkSongTable->blockSignals(true);

    // ui->songTable->clearSelection();
    ui->darkSongTable->clearSelection();

    // ui->songTable->blockSignals(false);
    ui->darkSongTable->blockSignals(false);

    // ------
    QItemSelectionModel *selectionModel = ui->playlist1Table->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;

    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } // else more than 1 row or no rows, just return -1

    if (row >= 0) {
//        qDebug() << "playlist1 selection changed to: " << row << ui->playlist1Table->item(row, 1)->text();
    }
}

void MainWindow::on_playlist2Table_itemSelectionChanged()
{
//    qDebug() << "playlist2Table_itemSelectionChanged";
    ui->playlist1Table->blockSignals(true);
    ui->playlist3Table->blockSignals(true);

    ui->playlist1Table->clearSelection();
    ui->playlist3Table->clearSelection();

    ui->playlist1Table->blockSignals(false);
    ui->playlist3Table->blockSignals(false);

    // ------
    // ui->songTable->blockSignals(true);
    ui->darkSongTable->blockSignals(true);

    // ui->songTable->clearSelection();
    ui->darkSongTable->clearSelection();

    // ui->songTable->blockSignals(false);
    ui->darkSongTable->blockSignals(false);

    // ------
    QItemSelectionModel *selectionModel = ui->playlist2Table->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;

    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } // else more than 1 row or no rows, just return -1

    if (row >= 0) {
//        qDebug() << "playlist2 selection changed to: " << row << ui->playlist2Table->item(row, 1)->text();
    }
}

void MainWindow::on_playlist3Table_itemSelectionChanged()
{
//    qDebug() << "playlist3Table_itemSelectionChanged";
    ui->playlist1Table->blockSignals(true);
    ui->playlist2Table->blockSignals(true);

    ui->playlist1Table->clearSelection();
    ui->playlist2Table->clearSelection();

    ui->playlist1Table->blockSignals(false);
    ui->playlist2Table->blockSignals(false);

    // ------
    // ui->songTable->blockSignals(true);
    ui->darkSongTable->blockSignals(true);

    // ui->songTable->clearSelection();
    ui->darkSongTable->clearSelection();

    // ui->songTable->blockSignals(false);
    ui->darkSongTable->blockSignals(false);

    // ------
    QItemSelectionModel *selectionModel = ui->playlist3Table->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;

    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } // else more than 1 row or no rows, just return -1

    if (row >= 0) {
//        qDebug() << "playlist3 selection changed to: " << row << ui->playlist3Table->item(row, 1)->text();
    }
}

// ===============================================
void MainWindow::customTreeWidgetMenuRequested(QPoint pos) {
    Q_UNUSED(pos)

    QTreeWidgetItem *treeItem = ui->treeWidget->itemAt(pos);

    if (treeItem == nullptr) {
        return; // if you didn't click on an item, fuggedabout it
    }

    QMenu *twMenu = new QMenu();
    twMenu->setProperty("theme", currentThemeString);

    QString fullPathToLeaf = treeItem->text(0);
    while (treeItem->parent() != NULL)
    {
        fullPathToLeaf = treeItem->parent()->text(0) + "/" + fullPathToLeaf;
        treeItem = treeItem->parent();
    }

    fullPathToLeaf = fullPathToLeaf.replace("Playlists", "playlists");  // difference between display "P" and file system "p"

    QString PlaylistFileName = musicRootPath + "/" + fullPathToLeaf + ".csv"; // prefix it with the path to musicDir, suffix it with .csv

    twMenu->addAction("Show in palette slot #1",
                      [this, PlaylistFileName](){
                          int songCount;
                          loadPlaylistFromFileToPaletteSlot(PlaylistFileName, 0, songCount);
                      }
                      );
    twMenu->addAction("Show in palette slot #2",
                      [this, PlaylistFileName](){
                          int songCount;
                          loadPlaylistFromFileToPaletteSlot(PlaylistFileName, 1, songCount);
                      }
                      );
    twMenu->addAction("Show in palette slot #3",
                      [this, PlaylistFileName](){
                          int songCount;
                          loadPlaylistFromFileToPaletteSlot(PlaylistFileName, 2, songCount);
                      }
                      );

    if (ui->action0paletteSlots->isChecked()) {
        twMenu->actions()[0]->setEnabled(false);
        twMenu->actions()[1]->setEnabled(false);
        twMenu->actions()[2]->setEnabled(false);
    } else if (ui->action1paletteSlots->isChecked()) {
        twMenu->actions()[1]->setEnabled(false);
        twMenu->actions()[2]->setEnabled(false);
    } else if (ui->action2paletteSlots->isChecked()) {
        twMenu->actions()[2]->setEnabled(false);
    }

    twMenu->popup(QCursor::pos());
    twMenu->exec();

    delete(twMenu);
}


void MainWindow::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *treeItem, int column)
{
    Q_UNUSED(treeItem)
    Q_UNUSED(column)
}

// ======================================================
void MainWindow::on_darkSongTable_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos)
    QStringList currentTags;

    // qDebug() << "***** on_darkSongTable_customContextMenuRequested";

    // ------------------------------------------------------------------------------------
    // we already know that we have at LEAST one row selected (because it's a context menu)
    //  let's find the row numbers
    QList<int> selectedRows;
    foreach (const QModelIndex &mi, ui->darkSongTable->selectionModel()->selectedRows()) {
        if (!ui->darkSongTable->isRowHidden(mi.row())) {
            selectedRows.append(mi.row());
        }
    }
    int rowCount = selectedRows.count();
    // qDebug() << "rows selected: " << selectedRows;

    QString plural;
    if (rowCount <= 1) {
        plural = "track";
    } else {
        plural = QString::number(rowCount) + " tracks";
    }

    // ------------------------------------------------------------------------------------
    QMenu menu(this);

    menu.setProperty("theme", currentThemeString);

    // DDD(relPathInSlot[0])
    // DDD(relPathInSlot[1])
    // DDD(relPathInSlot[2])

    // examples of relPathInSlot:
    // Local Playlist in a slot :: relPathInSlot[0]  =  "CPSD/2024/CPSD_2024.07.11"
    // Track Filter in a slot :: relPathInSlot[1]  =  "/tracks/patter"
    // Untitled Playlist (e.g. Apple Music just loaded into a slot, or nothing in a slot) :: relPathInSlot[2]  =  ""

    // ADD TO BOTTOM OF SLOT {1,2,3} = allows multiple selections
    QString actionString;

    if (!ui->action0paletteSlots->isChecked()) {
        QString rp0 = relPathInSlot[0];
        if (rp0 == "") {
            // "Untitled playlist" (also could have been just imported from Apple Music)
            actionString = "Add " + plural + " to BOTTOM of 'Untitled playlist' in slot #1";
            menu.addAction(actionString, this, [this] { darkAddPlaylistItemsToBottom(0); });
        } else if (rp0.startsWith("/tracks/")) {
            // Track filter, e.g. "/tracks/patter"
            actionString = "Track filter '" + rp0.replace("/tracks/", "") + "' in slot #1 is not editable"; // DO NOT MODIFY relPathInSlot[] with replace!
            menu.addAction(actionString, this, [this] { darkAddPlaylistItemsToBottom(0); })->setEnabled(false);  // it's there, but grey it out
        } else if (rp0.startsWith("/Apple Music/")) {
            // Track filter, e.g. "/Apple Music/Second Playlist"
            actionString = "Apple Playlist '" + rp0.replace("/Apple Music/", "") + "' in slot #1 is not editable"; // DO NOT MODIFY relPathInSlot[] with replace!
            menu.addAction(actionString, this, [this] { darkAddPlaylistItemsToBottom(0); })->setEnabled(false);  // it's there, but grey it out
        } else {
            // Local playlist, e.g. "CPSD/2025/CPSD_2024.12.23"
            actionString = "Add " + plural + " to BOTTOM of playlist '" + rp0 + "'";
            menu.addAction(actionString, this, [this] { darkAddPlaylistItemsToBottom(0); });
        }
    }

    if (!ui->action0paletteSlots->isChecked() && !ui->action1paletteSlots->isChecked()) {
        QString rp1 = relPathInSlot[1];
        if (rp1 == "") {
            // "Untitled playlist" (also could have been just imported from Apple Music)
            actionString = "Add " + plural + " to BOTTOM of 'Untitled playlist' in slot #2";
            menu.addAction(actionString, this, [this] { darkAddPlaylistItemsToBottom(1); });
        } else if (rp1.startsWith("/tracks/")) {
            // Track filter, e.g. "/tracks/patter"
            actionString = "Track filter '" + rp1.replace("/tracks/", "") + "' in slot #2 is not editable"; // DO NOT MODIFY relPathInSlot[] with replace!
            menu.addAction(actionString, this, [this] { darkAddPlaylistItemsToBottom(1); })->setEnabled(false);  // it's there, but grey it out
        } else if (rp1.startsWith("/Apple Music/")) {
            // Apple Music filter, e.g. "/Apple Music/Second Playlist"
            actionString = "Apple Playlist '" + rp1.replace("/Apple Music/", "") + "' in slot #2 is not editable"; // DO NOT MODIFY relPathInSlot[] with replace!
            menu.addAction(actionString, this, [this] { darkAddPlaylistItemsToBottom(0); })->setEnabled(false);  // it's there, but grey it out
        } else {
            // Local playlist, e.g. "CPSD/2025/CPSD_2024.12.23"
            actionString = "Add " + plural + " to BOTTOM of playlist '" + rp1 + "'";
            menu.addAction(actionString, this, [this] { darkAddPlaylistItemsToBottom(1); });
        }
    }

    if (!ui->action0paletteSlots->isChecked() && !ui->action1paletteSlots->isChecked() && !ui->action2paletteSlots->isChecked()) {
        QString rp2 = relPathInSlot[2];
        if (rp2 == "") {
            // "Untitled playlist" (also could have been just imported from Apple Music)
            actionString = "Add " + plural + " to BOTTOM of 'Untitled playlist' in slot #3";
            menu.addAction(actionString, this, [this] { darkAddPlaylistItemsToBottom(2); });
        } else if (rp2.startsWith("/tracks/")) {
            // Track filter, e.g. "/tracks/patter"
            actionString = "Track filter '" + rp2.replace("/tracks/", "") + "' in slot #3 is not editable"; // DO NOT MODIFY relPathInSlot[] with replace!
            menu.addAction(actionString, this, [this] { darkAddPlaylistItemsToBottom(2); })->setEnabled(false);  // it's there, but grey it out
        } else if (rp2.startsWith("/Apple Music/")) {
            // Apple Music filter, e.g. "/Apple Music/Second Playlist"
            actionString = "Apple Playlist '" + rp2.replace("/Apple Music/", "") + "' in slot #3 is not editable"; // DO NOT MODIFY relPathInSlot[] with replace!
            menu.addAction(actionString, this, [this] { darkAddPlaylistItemsToBottom(0); })->setEnabled(false);  // it's there, but grey it out
        } else {
            // Local playlist, e.g. "CPSD/2025/CPSD_2024.12.23"
            actionString = "Add " + plural + " to BOTTOM of playlist '" + rp2 + "'";
            menu.addAction(actionString, this, [this] { darkAddPlaylistItemsToBottom(2); });
        }
    }

#if defined(Q_OS_MAC)
    if (rowCount == 1) {
        QString pathToMP3 = ui->darkSongTable->item(selectedRows[0],kPathCol)->data(Qt::UserRole).toString();

        QFileInfo f(pathToMP3);

        if (f.exists()) {
            // REVEAL STUFF ==============
            // can only reveal a single file or cuesheet in Finder
            menu.addSeparator();
            menu.addAction( "Reveal Audio File in Finder",       this, SLOT (darkRevealInFinder()) );
            menu.addAction( "Reveal Current Cuesheet in Finder", this, SLOT (darkRevealAttachedLyricsFileInFinder()) );

            // SECTIONS STUFF ============
            // just do ONE
            menu.addSeparator();
            menu.addAction("Calculate Section Info for this song...",
                           this, [this, pathToMP3]{ EstimateSectionsForThisSong(pathToMP3); });
            menu.addAction("Remove Section info for this song....",
                           this, [this, pathToMP3]{ RemoveSectionsForThisSong(pathToMP3);   });
        } else {
            // REVEAL STUFF for a file that doesn't exist ==============
            // can only reveal a single file or cuesheet in Finder
            menu.addSeparator();
            menu.addAction( "Reveal Enclosing Folder in Finder",       this, SLOT (darkRevealInFinder()) );
        }
    } else {
        // MORE THAN ONE
        // REVEAL (can't do) =========
        //
        // SECTIONS STUFF ============
        // allows multiple selections
        menu.addSeparator();
        menu.addAction("Calculate Section Info for these " + QString::number(rowCount) + " songs...",
                       this, [this, selectedRows]{ EstimateSectionsForTheseSongs(selectedRows); });
        menu.addAction("Remove Section info for these " + QString::number(rowCount) + " songs....",
                       this, [this, selectedRows]{ RemoveSectionsForTheseSongs(selectedRows);   });
    }
#endif

#if defined(Q_OS_WIN)
            menu.addAction ( "Show in Explorer" , this , SLOT (darkRevealInFinder()) );
#endif

#if defined(Q_OS_LINUX)
            menu.addAction ( "Open containing folder" , this , SLOT (darkRevealInFinder()) );
#endif

    if (rowCount == 1) {

        QString pathToMP3 = ui->darkSongTable->item(selectedRows[0], kPathCol)->data(Qt::UserRole).toString(); // only one row, so one MP3 file

        // just one track selected, so we can do Tags stuff with it (that we can't do with multiple selected tracks)
        // TAGS STUFF ==================
        menu.addSeparator();
        menu.addAction( "Edit Tags...", this, SLOT (darkEditTags()) ); // text editor for tags


        SongSetting settings;
        songSettings.loadSettings(pathToMP3, settings);
        if (settings.isSetTags())
        {
            QString curTagString = settings.getTags();
            // currentTags = settings.getTags().split(" ");
            currentTags = curTagString.split(" ");

            // if the song has some tags, offer to Remove them
            if (currentTags.count() > 1 || currentTags[0] != "") {
                // for some reason, count == 1 is one tag that is an empty string, means there are zero tags
                // but sometimes, like when we manually do Edit Tags, there's no empty string.
                // thus the need for that counterintuitive if clause above.
                menu.addAction( "Remove all Tags from this song", this, [this] {
                    this->removeAllTagsFromSong();
                    refreshAllPlaylists();
                });
            }

        }

        // and the PULLDOWN MENU WITH CHECKBOXES for editing tags
        QMenu *tagsMenu(new QMenu("Tags"));
        QHash<QString,QPair<QString,QString>> tagColors(songSettings.getTagColors()); // we don't use the colors
        QStringList tags(tagColors.keys()); // we use these keys (the names of the tags)
        tags.sort(Qt::CaseInsensitive);

        for (const auto &tagUntrimmed : tags)
        {
            QString tag(tagUntrimmed.trimmed());

            if (tag.length() <= 0)
                continue;

            bool set = false;
            for (const auto &t : currentTags)
            {
                // if (t.compare(tag, Qt::CaseInsensitive) == 0)
                if (t.compare(tag) == 0)
                {
                    set = true;
                }
            }
            QAction *action(new QAction(tag));
            action->setCheckable(true);
            action->setChecked(set);
            connect(action, &QAction::triggered,
                    [this, set, tag]()
                    {
                        this->darkChangeTagOnCurrentSongSelection(tag, !set);
                        refreshAllPlaylists();
                    });
            tagsMenu->addAction(action);
        }
        menu.addMenu(tagsMenu);
    } else if (rowCount > 1) {
        // more than one row in darkSongTable has been selected...
        //   check to see if one or more songs have tags
        bool someSongHasTags = false;

        for (int i = 0; i < rowCount; i++) {
            QString pathToMP3 = ui->darkSongTable->item(selectedRows[i], kPathCol)->data(Qt::UserRole).toString();
            SongSetting settings;
            songSettings.loadSettings(pathToMP3, settings);
            if (settings.isSetTags())
            {
                QString curTagString = settings.getTags();
                // currentTags = settings.getTags().split(" ");
                currentTags = curTagString.split(" ");

                // if the song has some tags, offer to Remove them
                if (currentTags.count() > 1 || currentTags[0] != "") {
                    someSongHasTags = true;
                    break; // and break out of the for loop
                }
            }
        }

        //   if so, show "Remove all tags from these N songs" context menu item
        if (someSongHasTags) {
            menu.addSeparator(); // -------------------
            menu.addAction( "Remove all Tags from these " + QString::number(rowCount) + " songs",
                            this, [this, rowCount, selectedRows, currentTags] {
                            for (int i = 0; i < rowCount; i++) {
                                QString pathToMP3 = ui->darkSongTable->item(selectedRows[i], kPathCol)->data(Qt::UserRole).toString();
                                SongSetting settings;
                                songSettings.loadSettings(pathToMP3, settings);
                                if (settings.isSetTags())
                                {
                                    QString curTagString = settings.getTags();
                                    // currentTags = settings.getTags().split(" ");
                                    QStringList currentTags = curTagString.split(" ");

                                    // if the song has some tags, offer to Remove them
                                    if (currentTags.count() > 1 || currentTags[0] != "") {
                                        // for some reason, count == 1 is one tag that is an empty string, means there are zero tags
                                        // but sometimes, like when we manually do Edit Tags, there's no empty string.
                                        // thus the need for that counterintuitive if clause above.
                                        // qDebug() << "removing tags from row: " << selectedRows[i];
                                        removeAllTagsFromSongRow(selectedRows[i]);
                                    }
                                }
                            }
                            refreshAllPlaylists();
            });
        }

    }

    // DO IT ==========
    menu.popup(QCursor::pos());
    menu.exec();

    // DDD(relPathInSlot[0])
    // DDD(relPathInSlot[1])
    // DDD(relPathInSlot[2])

    return;
}

// -----------------------------------------------
void MainWindow::darkAddPlaylistItemsToBottom(int whichSlot) { // slot is 0 - 2

    qDebug() << "darkPlaylistItemToBottom:" << whichSlot;

    // DDD(relPathInSlot[0])
    // DDD(relPathInSlot[1])
    // DDD(relPathInSlot[2])

    MyTableWidget *theTableWidget;
    QString PlaylistFileName = "foobar";
    switch (whichSlot) {
        case 0: theTableWidget = ui->playlist1Table; break;
        case 1: theTableWidget = ui->playlist2Table; break;
        case 2: theTableWidget = ui->playlist3Table; break;
    }

    // ------------------------------------------------------------------------------------
    foreach (const QModelIndex &mi, ui->darkSongTable->selectionModel()->selectedRows()) {
        int row = mi.row();  // this is the actual row number of each selected row
        // qDebug() << "ROW ADD TO BOTTOM:" << row;

        QString theFullPath = ui->darkSongTable->item(row, kPathCol)->data(Qt::UserRole).toString();
        // qDebug() << "darkPlaylistItemToBottom will add THIS: " << whichSlot << theFullPath;

        if (ui->darkSongTable->isRowHidden(row)) {
            // qDebug() << "SLOT " << whichSlot << ", ROW" << row << "IS HIDDEN: " << theFullPath;
            continue;
        } else {
            // qDebug() << "SLOT " << whichSlot << ", ROW" << row << "WILL BE ADDED: " << theFullPath;
        }

        // make a new row, after all the other ones
        theTableWidget->insertRow(theTableWidget->rowCount()); // always make a new row
        int songCount = theTableWidget->rowCount();

        // # column
        QTableWidgetItem *num = new TableNumberItem(QString::number(songCount)); // use TableNumberItem so that it sorts numerically
        theTableWidget->setItem(songCount-1, 0, num);

        // TITLE column
        QString theRelativePath = theFullPath;
        QString absPath = theFullPath; // already is fully qualified

        theRelativePath.replace(musicRootPath, "");
        setTitleField(theTableWidget, songCount-1, theRelativePath, true, PlaylistFileName); // whichTable, whichRow, fullPath, bool isPlaylist, PlaylistFilename (for errors)

        // PITCH column
        QString thePitch = ui->darkSongTable->item(row, kPitchCol)->text();
        QTableWidgetItem *pit = new QTableWidgetItem(thePitch);
        theTableWidget->setItem(songCount-1, 2, pit);

        // TEMPO column
        QString theTempo = ui->darkSongTable->item(row, kTempoCol)->text();
        QTableWidgetItem *tem = new QTableWidgetItem(theTempo);
        theTableWidget->setItem(songCount-1, 3, tem);

        // PATH column
        QTableWidgetItem *fullPath = new QTableWidgetItem(absPath); // full ABSOLUTE path
        theTableWidget->setItem(songCount-1, 4, fullPath);

        // LOADED column
        QTableWidgetItem *loaded = new QTableWidgetItem("");
        theTableWidget->setItem(songCount-1, 5, loaded);

        theTableWidget->resizeColumnToContents(0); // FIX: perhaps only if this is the first row?
    //    theTableWidget->resizeColumnToContents(2);
    //    theTableWidget->resizeColumnToContents(3);
    }

    QTimer::singleShot(250, [theTableWidget]{
        // NOTE: We have to do it this way with a single-shot timer, because you can't scroll immediately to a new item, until it's been processed
        //   after insertion by the table.  So, there's a delay.  Yeah, this is kludgey, but it works.

        // theTableWidget->selectRow(songCount-1); // for some reason this has to be here, or it won't scroll all the way to the bottom.
        // theTableWidget->scrollToItem(theTableWidget->item(songCount-1,1), QAbstractItemView::PositionAtCenter); // ensure that the new item is visible

        // same thing, but without forcing us to select the last item in the playlist
        theTableWidget->verticalScrollBar()->setSliderPosition(theTableWidget->verticalScrollBar()->maximum());
        });

    slotModified[whichSlot] = true;
    saveSlotNow(whichSlot);
    // playlistSlotWatcherTimer->start(std::chrono::seconds(10)); // no need for this now, it's immediate

    // DDD(relPathInSlot[0])
    // DDD(relPathInSlot[1])
    // DDD(relPathInSlot[2])
}

// -----------------------------------------------
void MainWindow::darkAddPlaylistItemToBottom(int whichSlot, QString title, QString thePitch, QString theTempo, QString theFullPath, QString isLoaded) { // slot is 0 - 2

    Q_UNUSED(title)
    Q_UNUSED(isLoaded)

    MyTableWidget *theTableWidget;
    QString PlaylistFileName = "foobar";
    switch (whichSlot) {
        default:
        case 0: theTableWidget = ui->playlist1Table; break;
        case 1: theTableWidget = ui->playlist2Table; break;
        case 2: theTableWidget = ui->playlist3Table; break;
    }

    // make a new row, after all the other ones
    theTableWidget->insertRow(theTableWidget->rowCount()); // always make a new row
    int songCount = theTableWidget->rowCount();

    // # column
    QTableWidgetItem *num = new TableNumberItem(QString::number(songCount)); // use TableNumberItem so that it sorts numerically
    theTableWidget->setItem(songCount-1, 0, num);

    // TITLE column
    QString theRelativePath = theFullPath;
    QString absPath = theFullPath; // already is fully qualified

    theRelativePath.replace(musicRootPath, "");
    setTitleField(theTableWidget, songCount-1, theRelativePath, true, PlaylistFileName, theRelativePath); // whichTable, whichRow, fullPath, bool isPlaylist, PlaylistFilename (for errors)

    // PITCH column
    QTableWidgetItem *pit = new QTableWidgetItem(thePitch);
    theTableWidget->setItem(songCount-1, 2, pit);

    // TEMPO column
    QTableWidgetItem *tem = new QTableWidgetItem(theTempo);
    theTableWidget->setItem(songCount-1, 3, tem);

    // PATH column
    QTableWidgetItem *fullPath = new QTableWidgetItem(absPath); // full ABSOLUTE path
    theTableWidget->setItem(songCount-1, 4, fullPath);

    // LOADED column
    QTableWidgetItem *loaded = new QTableWidgetItem("");
    theTableWidget->setItem(songCount-1, 5, loaded);

    theTableWidget->resizeColumnToContents(0); // FIX: perhaps only if this is the first row?
//    theTableWidget->resizeColumnToContents(2);
//    theTableWidget->resizeColumnToContents(3);

    QTimer::singleShot(250, [theTableWidget]{
        // NOTE: We have to do it this way with a single-shot timer, because you can't scroll immediately to a new item, until it's been processed
        //   after insertion by the table.  So, there's a delay.  Yeah, this is kludgey, but it works.

        // theTableWidget->selectRow(songCount-1); // for some reason this has to be here, or it won't scroll all the way to the bottom.
        // theTableWidget->scrollToItem(theTableWidget->item(songCount-1,1), QAbstractItemView::PositionAtCenter); // ensure that the new item is visible

        // same thing, but without forcing us to select the last item in the playlist
        theTableWidget->verticalScrollBar()->setSliderPosition(theTableWidget->verticalScrollBar()->maximum());
        });

    slotModified[whichSlot] = true;
    // playlistSlotWatcherTimer->start(std::chrono::seconds(10));  // no need for this, it's immediate now
    saveSlotNow(whichSlot);
}

void MainWindow::darkRevealInFinder()
{
    int row = darkSelectedSongRow();
    if (row >= 0) {

        // exactly 1 row was selected (good)
        QString pathToMP3 = ui->darkSongTable->item(row,kPathCol)->data(Qt::UserRole).toString();

        QStringList pathPieces = pathToMP3.split("/");
        QString pathToOpen;
        bool fiExists;
        do {
            pathToOpen = pathPieces.join("/");
            QFile f(pathToOpen);
            QFileInfo fi(f);
            fiExists = fi.exists();
            pathPieces.removeLast();
            // qDebug() << "CHECKING: " << fiExists << pathToOpen;
        } while (!fiExists);

        // qDebug() << "FILE OR FOLDER TO OPEN TO FIX THIS PROBLEM: " << pathToOpen;

        showInFinderOrExplorer(pathToOpen);
    }
    else {
            // more than 1 row or no rows at all selected (BAD)
    }
}

// --------------
// invoked from context menu on darkSongTable entry
void MainWindow::darkRevealAttachedLyricsFileInFinder() {

    int selectedRow = darkSelectedSongRow();  // get current row or -1
    if (selectedRow == -1) {
            // qDebug() << "Tried to revealAttachedLyricsFile, but no selected row."; // if nothing selected, give up
            return;
    }

    QString currentMP3filenameWithPath = ui->darkSongTable->item(selectedRow, kPathCol)->data(Qt::UserRole).toString();

    SongSetting settings1;
    if (songSettings.loadSettings(currentMP3filenameWithPath, settings1)) {
            QString cuesheetPath = settings1.getCuesheetName();

            // qDebug() << "convertCuesheetPathNameToCurrentRoot BEFORE:" << cuesheetPath;
            cuesheetPath = convertCuesheetPathNameToCurrentRoot(cuesheetPath);
            // qDebug() << "convertCuesheetPathNameToCurrentRoot AFTER:" << cuesheetPath;

            showInFinderOrExplorer(cuesheetPath);
    } else {
            // qDebug() << "Tried to revealAttachedLyricsFile, but could not get settings for: " << currentMP3filenameWithPath;
    }
}

void MainWindow::on_actionSwitch_to_Light_Mode_triggered()
{
    QString newMode = "Dark";
    if (darkmode) {
        newMode = "Light";
    }

#ifdef TESTRESTARTINGSQUAREDESK
    if (!testRestartingSquareDesk) {
#endif
        QMessageBox msgBox;
        msgBox.setText("Switch to " + newMode + " Mode requires restarting SquareDesk.");
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setInformativeText("OK to restart SquareDesk now?");
        msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();

        if (ret == QMessageBox::No) {
            return;
        }
#ifdef TESTRESTARTINGSQUAREDESK
    }
#endif

    // OK to switch!
    // set prefs so we come up in the right mode next time ----
    if (darkmode) {
        // switch to LIGHT mode (OLD)
        prefsManager.SetdarkMode(false);
    } else {
        // switch to DARK mode (NEW)
        prefsManager.SetdarkMode(true);
    }

    // now restart the app
    qApp->exit(RESTART_SQUAREDESK); // special exit code understood by main.cpp
}


void MainWindow::on_actionNormalize_Track_Audio_toggled(bool checked)
{
    if (checked) {
        ui->actionNormalize_Track_Audio->setChecked(true);
        cBass->SetNormalizeTrackAudio(true);
        ui->darkSeekBar->setWholeTrackPeak(cBass->GetWholeTrackPeak()); // scale the waveform
    }
    else {
        ui->actionNormalize_Track_Audio->setChecked(false);
        cBass->SetNormalizeTrackAudio(false);
        ui->darkSeekBar->setWholeTrackPeak(1.0); // disables waveform scaling
    }

    // the Normalize Track Audio setting is persistent across restarts of the application
    prefsManager.SetnormalizeTrackAudio(ui->actionNormalize_Track_Audio->isChecked());

    if (cBass->GetWholeTrackPeak() != 1.0) {
        ui->darkSeekBar->updateBgPixmap((float*)1, 1);  // update the bg pixmap, in case it was a singing call
    }
}

// ===================================================================================================
// make folderName as a subfolder of musicRootPath, if and only if it doesn't already exist
//  NOTE: folderName does NOT have a leading slash
void MainWindow::maybeMakeRequiredFolder(QString folderName) {
    if (musicRootPath == "") {
        // a bit of error checking, just in case...
        return;
    }

    QString fullPathToDir = musicRootPath + "/" + folderName;

    // if the directory doesn't exist, create it)
    QDir dir(fullPathToDir);
    if (!dir.exists()) {
        qDebug() << "Required folder created: " << folderName;
        dir.mkpath(".");  // make it
    }
}

void MainWindow::maybeMakeAllRequiredFolders() {
    maybeMakeRequiredFolder(".squaredesk");
    maybeMakeRequiredFolder(".squaredesk/bulk");
    maybeMakeRequiredFolder("lyrics");
    maybeMakeRequiredFolder("lyrics/downloaded");
    maybeMakeRequiredFolder("lyrics/templates");
    maybeMakeRequiredFolder("patter");
    maybeMakeRequiredFolder("playlists");
    maybeMakeRequiredFolder("reference");
    maybeMakeRequiredFolder("sd");
    maybeMakeRequiredFolder("sd/dances");
    maybeMakeRequiredFolder("singing");
    maybeMakeRequiredFolder("soundfx");
    maybeMakeRequiredFolder("vocals");
    maybeMakeRequiredFolder("xtras");
}

void MainWindow::maybeInstallTemplates() {
    // qDebug() << "maybeInstallTemplates()";

#if defined(Q_OS_MAC)
        QString pathFromAppDirPathToResources = "/../Resources";
        QString templatesDir = musicRootPath + "/lyrics/templates";

        QStringList templateNames = {"lyrics.template.html", "lyrics.template.2col.html", "patter.template.html"};

        foreach (const QString &templateName, templateNames)
        {
            QString theName = templateName;
            QString source = QCoreApplication::applicationDirPath() + pathFromAppDirPathToResources + "/" + theName;
            QString destination = templatesDir + "/" + theName;

            QFile dest(destination);
            if (!dest.exists()) {
                QFile::copy(source, destination);
            }

        }
#endif
}

// sets the current* from the pathname
void MainWindow::setCurrentSongMetadata(QString type) {
    // qDebug() << "setCurrentSongMetadata: " << type;
    currentSongTypeName = type.toLower();  // e.g "hoedown"

    currentSongIsPatter = songTypeNamesForPatter.contains(currentSongTypeName);
    currentSongIsSinger = songTypeNamesForSinging.contains(currentSongTypeName);
    currentSongIsVocal  = songTypeNamesForCalled.contains(currentSongTypeName);
    currentSongIsExtra  = songTypeNamesForExtras.contains(currentSongTypeName);
    currentSongIsUndefined = !(currentSongIsPatter || currentSongIsSinger || currentSongIsVocal || currentSongIsExtra); // none of the existing categories

    // now set the category name, e.g. "hoedown", et.al. --> "patter"
    // NOTE: this must match filepath2SongCategoryName()
    if (currentSongIsPatter) {
        currentSongCategoryName = "patter";
    } else if (currentSongIsSinger) {
        currentSongCategoryName = "singing";
    } else if (currentSongIsVocal) {
        currentSongCategoryName = "called";
    } else if (currentSongIsExtra) {
        currentSongCategoryName = "extras";
    } else {
        currentSongCategoryName = "unknown";
    }

    // qDebug() << "setCurrentSongMetadata(): " << currentSongTypeName  << currentSongCategoryName << currentSongIsPatter << currentSongIsSinger << currentSongIsVocal << currentSongIsExtra << currentSongIsUndefined;
}

// void MainWindow::on_actionMove_on_to_Next_Song_triggered()
// {
//     on_actionNext_Playlist_Item_triggered();
// }

void MainWindow::auditionByKeyPress(void) {
    // qDebug() << "***** auditionByKeyPress";

    QModelIndexList list1 = ui->playlist1Table->selectionModel()->selectedRows();
    QList<int> rowsPaletteSlot1;
    foreach (const QModelIndex &m, list1) {
        rowsPaletteSlot1.append(m.row());
    }

    QModelIndexList list2 = ui->playlist2Table->selectionModel()->selectedRows();
    QList<int> rowsPaletteSlot2;
    foreach (const QModelIndex &m, list2) {
        rowsPaletteSlot2.append(m.row());
    }

    QModelIndexList list3 = ui->playlist3Table->selectionModel()->selectedRows();
    QList<int> rowsPaletteSlot3;
    foreach (const QModelIndex &m, list3) {
        rowsPaletteSlot3.append(m.row());
    }

    QModelIndexList list4 = ui->darkSongTable->selectionModel()->selectedRows();
    QList<int> rowsDarkSongTable;
    foreach (const QModelIndex &m, list4) {
        rowsDarkSongTable.append(m.row());
    }

    // qDebug() << rowsDarkSongTable.count() << rowsPaletteSlot1.count() << rowsPaletteSlot2.count() << rowsPaletteSlot3.count();

    QString auditionSongFilePath = "";

    if (rowsDarkSongTable.count() >= 1) {
        auditionInProgress = true;
        int row = list4.at(0).row();
        auditionSongFilePath = this->ui->darkSongTable->item(row,kPathCol)->data(Qt::UserRole).toString();
    } else if (rowsPaletteSlot1.count() >= 1) {
        int row = list1.at(0).row();
        auditionSongFilePath = ui->playlist1Table->item(row,4)->text();
    } else if (rowsPaletteSlot2.count() >= 1) {
        int row = list2.at(0).row();
        auditionSongFilePath = ui->playlist2Table->item(row,4)->text();
    } else if (rowsPaletteSlot3.count() >= 1) {
        int row = list3.at(0).row();
        auditionSongFilePath = ui->playlist3Table->item(row,4)->text();
    } else {
        return;  // nothing was selected, so nothing to do
    }
    // qDebug() << "selection = auditionSongFilePath:" << auditionSongFilePath;

    auditionInProgress = true;
    // qDebug() << "setting auditionInProgress to true";

    this->auditionPlayer.setSource(QUrl::fromLocalFile(auditionSongFilePath));
    
    // Use the selected audition playback device, or default if not set
    QAudioDevice selectedDevice;
    if (!auditionPlaybackDeviceName.isEmpty()) {
        selectedDevice = getAudioDeviceByName(auditionPlaybackDeviceName);
    } else {
        selectedDevice = QMediaDevices::defaultAudioOutput();
    }
    QAudioOutput *audioOutput = new QAudioOutput(selectedDevice);
    
    this->auditionPlayer.setAudioOutput(audioOutput);
    this->auditionPlayer.play();
}

void MainWindow::auditionByKeyRelease(void) {
    // qDebug() << "***** auditionByKeyRelease";

    this->auditionPlayer.stop();
    auditionInProgress = false;
}

// =============================================================================
// check to see if a) vamp exists, b) vamp can be run, and c) vamp can find the beatbartracker and segmentino plugins
//   returns
//      0: everything is fine
//      1: could not find vamp executable
//      2: could not run vamp
//      3: vamp could not find either QM plugins (beatbartracker)
//      4: vamp could not find Segmentino
int MainWindow::testVamp() {

    // return(99); // DEBUG DEBUG DEBUG

    QString pathNameToVamp(QCoreApplication::applicationDirPath());
    pathNameToVamp.append("/vamp-simple-host");

    // a) Check to see if vamp-simple-host exists ------------------
    if (!QFileInfo::exists(pathNameToVamp) ) {
        return(1);
    }

    // b) Check to see if we can execute ./vamp-simple-host -l ------------------
    QProcess vamp;

    QString workDir = QCoreApplication::applicationDirPath();
    vamp.setWorkingDirectory(workDir);
    // qDebug() << "working dir: " << workDir;

    QStringList vampArgs;
    vampArgs << "-l"; //

    vamp.start(pathNameToVamp, vampArgs);
    if (!vamp.waitForFinished(2000)) { // Wait 2 sec max for vamp to finish
        return(2); // if we timed out, return 2
    }

    QStringList vampOutput = QString(vamp.readAllStandardOutput()).split("\n");
    // qDebug() << "vamp output:" << vampOutput;

    bool foundQM = false;
    bool foundSegmentino = false;
    for (const QString &str : vampOutput) {
        if (str.contains("./qm-vamp-plugins.dylib:")) {
            // qDebug() << "testVamp:" << str;
            foundQM = true;
        } else if (str.contains("./segmentino.dylib:")) {
            // qDebug() << "testVamp:" << str;
            foundSegmentino = true;
        }
    }

    if (!foundQM) {
        return(3); // can't do beat/bar detection
    } else if (!foundSegmentino) {
        return(4); // can't do segmentation
    }

    return(0); // all is well
}

// ---------------------------------------------
// SORT Persistence
//
void MainWindow::handleNewSort(QString newSortString) {
    // qDebug() << "handleNewSort:" << newSortString;
    prefsManager.SetcurrentSortOrder(newSortString); // persist it
}
