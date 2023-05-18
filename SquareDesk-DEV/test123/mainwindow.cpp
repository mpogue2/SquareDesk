/****************************************************************************
**
** Copyright (C) 2016-2023 Mike Pogue, Dan Lyke
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

#include <QActionGroup>
#include <QColorDialog>
#include <QCoreApplication>
#include <QDateTime>
//#include <QDesktopWidget>
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

#include "analogclock.h"

// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"
#include "utility.h"
#include "perftimer.h"
#include "tablenumberitem.h"
#include "tablelabelitem.h"
#include "importdialog.h"
#include "exportdialog.h"
#include "songhistoryexportdialog.h"
#include "calllistcheckbox.h"
#include "sessioninfo.h"
#include "songtitlelabel.h"
#include "tablewidgettimingitem.h"
#include "danceprograms.h"
#include "startupwizard.h"
#include "makeflashdrivewizard.h"
#include "downloadmanager.h"
#include "songlistmodel.h"
#include "mytablewidget.h"

#include "src/communicator.h"

#if defined(Q_OS_MAC) | defined(Q_OS_WIN)
#ifndef M1MAC
#include "JlCompress.h"
#endif
#endif

extern bool comparePlaylistExportRecord(const PlaylistExportRecord &a, const PlaylistExportRecord &b);

class InvisibleTableWidgetItem : public QTableWidgetItem {
private:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
    QString text;
#pragma clang diagnostic pop
public:
    InvisibleTableWidgetItem(const QString &t);
    bool operator< (const QTableWidgetItem &other) const override;
    virtual ~InvisibleTableWidgetItem() override;
};

InvisibleTableWidgetItem::InvisibleTableWidgetItem(const QString &t) : QTableWidgetItem(), text(t) {}
InvisibleTableWidgetItem::~InvisibleTableWidgetItem() {}
bool InvisibleTableWidgetItem::operator< (const QTableWidgetItem &other) const
{
    const InvisibleTableWidgetItem *otherItem = dynamic_cast<const InvisibleTableWidgetItem*>(&other);
//    return this->text < otherItem->text;
    return (QString::compare(text, otherItem->text, Qt::CaseInsensitive ) < 0 );
}

// experimental removal of silence at the beginning of the song
// disabled right now, because it's not reliable enough.
//#define REMOVESILENCE 1

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
#include <string>

#include "typetracker.h"

using namespace TagLib;

// =================================================================================================
// SquareDesk Keyboard Shortcuts:
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

// M1 Silicon Mac only:
extern flexible_audio *cBass;  // make this accessible to PreferencesDialog
flexible_audio *cBass;

static const char *music_file_extensions[] = { "mp3", "wav", "m4a", "flac" };       // NOTE: must use Qt::CaseInsensitive compares for these
static const char *cuesheet_file_extensions[3] = { "htm", "html", "txt" };          // NOTE: must use Qt::CaseInsensitive compares for these

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
    cBass->StreamGetLength();  // tell everybody else what the length of the stream is...
    InitializeSeekBar(ui->seekBar);          // and now we can set the max of the seekbars, so they show up
    InitializeSeekBar(ui->seekBarCuesheet);  // and now we can set the max of the seekbars, so they show up

//    qDebug() << "haveDuration2 BPM = " << cBass->Stream_BPM;

    handleDurationBPM();  // finish up the UI stuff, when we know duration and BPM

    secondHalfOfLoad(currentSongTitle);  // now that we have duration and BPM, can finish up asynchronous load.
}

// ----------------------------------------------------------------------
MainWindow::MainWindow(QSplashScreen *splash, QWidget *parent) :
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
//    sdtest();

    cBass = new flexible_audio();
    //Q_UNUSED(splash)
    longSongTableOperationCount = 0;  // initialize counter to zero (unblocked)

    connect(cBass, SIGNAL(haveDuration()), this, SLOT(haveDuration2()));  // when decode complete, we know MP3 duration
    lastAudioDeviceName = "";

    lyricsCopyIsAvailable = false;
    lyricsTabNumber = 1;
    lyricsForDifferentSong = false;

    lastSavedPlaylist = "";  // no playlists saved yet in this session

    // Recall any previous flashcards file
    lastFlashcardsUserFile = prefsManager.Getlastflashcalluserfile();
    lastFlashcardsUserDirectory = prefsManager.Getlastflashcalluserdirectory();
    
    filewatcherShouldIgnoreOneFileSave = false;
    filewatcherIsTemporarilyDisabled = false;

    PerfTimer t("MainWindow::MainWindow", __LINE__);

    theSplash = splash;
    checkLockFile(); // warn, if some other copy of SquareDesk has database open

    flashCallsVisible = false;

    for (size_t i = 0; i < sizeof(webview) / sizeof(*webview); ++i)
        webview[i] = nullptr;
    
    linesInCurrentPlaylist = 0;

    loadedCuesheetNameWithPath = "";
    justWentActive = false;

    soundFXfilenames.clear();
    soundFXname.clear();

    maybeInstallSoundFX();
    maybeInstallReferencefiles();

    t.elapsed(__LINE__);

//    qDebug() << "preferences recentFenceDateTime: " << prefsManager.GetrecentFenceDateTime();
    recentFenceDateTime = QDateTime::fromString(prefsManager.GetrecentFenceDateTime(),
                                                          "yyyy-MM-dd'T'hh:mm:ss'Z'");
    recentFenceDateTime.setTimeSpec(Qt::UTC);  // set timezone (all times are UTC)

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

    // NEW INIT ORDER *******
    t.elapsed(__LINE__);
//    setFontSizes();

    t.elapsed(__LINE__);
    usePersistentFontSize(); // sets the font of the songTable, which is used by adjustFontSizes to scale other font sizes

    //on_actionReset_triggered();  // set initial layout

    t.elapsed(__LINE__);

    ui->songTable->setStyleSheet(QString("QTableWidget::item:selected{ color: #FFFFFF; background-color: #4C82FC } QHeaderView::section { font-size: %1pt; }").arg(13));  // TODO: factor out colors

    ui->songTable->setColumnWidth(kNumberCol,40);  // NOTE: This must remain a fixed width, due to a bug in Qt's tracking of its width.
    ui->songTable->setColumnWidth(kTypeCol,96);
    ui->songTable->setColumnWidth(kLabelCol,80);
//  kTitleCol is always expandable, so don't set width here
    ui->songTable->setColumnWidth(kRecentCol, 70);
    ui->songTable->setColumnWidth(kAgeCol, 36);
    ui->songTable->setColumnWidth(kPitchCol,60);
    ui->songTable->setColumnWidth(kTempoCol,60);

    zoomInOut(0);  // trigger reloading of all fonts, including horizontalHeader of songTable()

    t.elapsed(__LINE__);

    ui->songTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);  // auto set height of rows

    t.elapsed(__LINE__);

    this->setWindowTitle(QString("SquareDesk Music Player/Sequence Editor"));

    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

    ui->previousSongButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    ui->nextSongButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));

    ui->playButton->setEnabled(false);
    ui->stopButton->setEnabled(false);

    ui->nextSongButton->setEnabled(false);
    ui->previousSongButton->setEnabled(false);

    t.elapsed(__LINE__);

    // ============
    ui->menuFile->addSeparator();

    // disable clear buttons on Mac OS X and Windows (they take up too much space)
    bool enableClearButtons = true;
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    enableClearButtons = false;
#endif
    ui->typeSearch->setClearButtonEnabled(enableClearButtons);
    ui->labelSearch->setClearButtonEnabled(enableClearButtons);
    ui->titleSearch->setClearButtonEnabled(enableClearButtons);

    t.elapsed(__LINE__);

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
    keybindingActionToMenuAction[keyActionName_Forward15Seconds] = ui->actionSkip_Ahead_15_sec;
    keybindingActionToMenuAction[keyActionName_Backward15Seconds] = ui->actionSkip_Back_15_sec;
    keybindingActionToMenuAction[keyActionName_VolumePlus] = ui->actionVolume_Up;
    keybindingActionToMenuAction[keyActionName_VolumeMinus] = ui->actionVolume_Down;
    keybindingActionToMenuAction[keyActionName_TempoPlus] = ui->actionSpeed_Up;
    keybindingActionToMenuAction[keyActionName_TempoMinus] = ui->actionSlow_Down;
    keybindingActionToMenuAction[keyActionName_PlayPrevious] = ui->actionPrevious_Playlist_Item;
    keybindingActionToMenuAction[keyActionName_PlayNext] = ui->actionNext_Playlist_Item;
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

//    currentState = kStopped;
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

    vuMeter = new LevelMeter(this);
    ui->gridLayout_2->addWidget(vuMeter, 1,5);  // add it to the layout in the right spot
    vuMeter->setFixedHeight(20);

    vuMeter->reset();
    vuMeter->setEnabled(true);
    vuMeter->setVisible(true);

    t.elapsed(__LINE__);

    // analog clock -----
    analogClock = new AnalogClock(this);
    analogClock->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(analogClock, SIGNAL(customContextMenuRequested(QPoint)), analogClock, SLOT(customMenuRequested(QPoint)));
    ui->gridLayout_2->addWidget(analogClock, 2,6,4,1);  // add it to the layout in the right spot
    analogClock->setFixedSize(QSize(110,110));
    analogClock->setEnabled(true);
    analogClock->setVisible(true);
    ui->gridLayout_2->setAlignment(analogClock, Qt::AlignHCenter);  // center the clock horizontally

    ui->textBrowserCueSheet->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textBrowserCueSheet, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customLyricsMenuRequested(QPoint)));

    // where is the root directory where all the music is stored?
    pathStack = new QList<QString>();

    musicRootPath = prefsManager.GetmusicPath();      // defaults to ~/squareDeskMusic at very first startup

    // SD ------
    readAbbreviations();

    // ERROR LOGGING ------
    logFilePath = musicRootPath + "/.squaredesk/debug.log";

    switchToLyricsOnPlay = prefsManager.GetswitchToLyricsOnPlay();

    updateRecentPlaylistMenu();

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
//            qDebug() << "adding to musicRootWatcher: " << aPath;
        }
        t.elapsed(__LINE__);
    }

    t.elapsed(__LINE__);

    fileWatcherTimer = new QTimer();            // Retriggerable timer for file watcher events
    QObject::connect(fileWatcherTimer, SIGNAL(timeout()), this, SLOT(fileWatcherTriggered())); // this calls musicRootModified again (one last time!)

    fileWatcherDisabledTimer = new QTimer();    // 5 sec timer for working around the Venture extended attribute problem
    QObject::connect(fileWatcherDisabledTimer, SIGNAL(timeout()), this, SLOT(fileWatcherDisabledTriggered())); // this calls musicRootModified again (one last time!)

    // make sure that the "downloaded" directory exists, so that when we sync up with the Cloud,
    //   it will cause a rescan of the songTable and dropdown

    t.elapsed(__LINE__);

    QDir dir(musicRootPath + "/lyrics/downloaded");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

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
    analogClock->setColorForType(PATTER, QColor(patterColorString));
    analogClock->setColorForType(SINGING, QColor(singingColorString));
    analogClock->setColorForType(SINGING_CALLED, QColor(calledColorString));
    analogClock->setColorForType(XTRAS, QColor(extrasColorString));

    // ----------------------------------------------
    // Save the new settings for experimental break and patter timers --------
    tipLengthTimerEnabled = prefsManager.GettipLengthTimerEnabled();
    tipLength30secEnabled = prefsManager.GettipLength30secEnabled();
    int tipLengthTimerLength = prefsManager.GettipLengthTimerLength();
    tipLengthAlarmAction = prefsManager.GettipLengthAlarmAction();

    breakLengthTimerEnabled = prefsManager.GetbreakLengthTimerEnabled();
    breakLengthTimerLength = prefsManager.GetbreakLengthTimerLength();
    breakLengthAlarmAction = prefsManager.GetbreakLengthAlarmAction();

    analogClock->tipLengthTimerEnabled = tipLengthTimerEnabled;      // tell the clock whether the patter alarm is enabled
    analogClock->tipLength30secEnabled = tipLength30secEnabled;      // tell the clock whether the patter 30 sec warning is enabled
    analogClock->breakLengthTimerEnabled = breakLengthTimerEnabled;  // tell the clock whether the break alarm is enabled

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

    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    t.elapsed(__LINE__);

    findMusic(musicRootPath, true);  // get the filenames from the user's directories

    t.elapsed(__LINE__);

    on_actionViewTags_toggled(prefsManager.GetshowSongTags()); // THIS WILL CALL loadMusicList().  Doing it this way to avoid loading twice at startup.

    connect(ui->songTable->horizontalHeader(),&QHeaderView::sectionResized,
            this, &MainWindow::columnHeaderResized);

    // watch for changes in the number column, which will cause renumbering
    connect(ui->songTable,&QTableWidget::itemChanged,
            this, &MainWindow::tableItemChanged);

    t.elapsed(__LINE__);

    ui->songTable->resizeColumnToContents(kTypeCol);  // and force resizing of column widths to match songs
    ui->songTable->resizeColumnToContents(kLabelCol);

    t.elapsed(__LINE__);

#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    ui->listWidgetChoreographySequences->setStyleSheet(
         "QListWidget::item { border-bottom: 1px solid black; }" );
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    loadChoreographyList();

    t.elapsed(__LINE__);

#ifndef COOLGUI
    ui->pushButtonEditLyrics->setStyleSheet("QToolButton { border: 1px solid #575757; border-radius: 4px; background-color: palette(base); }");  // turn off border
#else
    QString toolButtonGradient("QToolButton{background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #F0F0F0, stop: 1 #A0A0A0);border-style: solid;border-color: #909090;border-width: 2px;border-radius: 8px;}");

    ui->previousSongButton->setStyleSheet(toolButtonGradient);
    ui->nextSongButton->setStyleSheet(toolButtonGradient);
    ui->playButton->setStyleSheet(toolButtonGradient);
    ui->stopButton->setStyleSheet(toolButtonGradient);
#endif

    // ----------
    updateSongTableColumnView(); // update the actual view of Age/Pitch/Tempo in the songTable view

    t.elapsed(__LINE__);

    // ----------
    clockColoringHidden = !prefsManager.GetexperimentalClockColoringEnabled();
    analogClock->setHidden(clockColoringHidden);

    // -----------
    ui->actionAutostart_playback->setChecked(prefsManager.Getautostartplayback());
//    ui->actionViewTags->setChecked(prefsManager.GetshowSongTags());  // DO NOT DO THIS HERE.  SEE ABOVE loadMusicList() commented out call.

    // -------
    on_monoButton_toggled(prefsManager.Getforcemono());

    on_actionRecent_toggled(prefsManager.GetshowRecentColumn());
    on_actionAge_toggled(prefsManager.GetshowAgeColumn());
    on_actionPitch_toggled(prefsManager.GetshowPitchColumn());
    on_actionTempo_toggled(prefsManager.GetshowTempoColumn());

// voice input is only available on MAC OS X and Win32 right now...
//    on_actionEnable_voice_input_toggled(prefsManager.Getenablevoiceinput());
//    voiceInputEnabled = prefsManager.Getenablevoiceinput();

    t.elapsed(__LINE__);

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
    ui->volumeSlider->SetOrigin(100);  // double click goes here
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

    // in the Designer, these have values, making it easy to visualize there
    //   must clear those out, because a song is not loaded yet.
    ui->currentLocLabel->setText("");
    ui->currentLocLabel_2->setText("");
    ui->songLengthLabel->setText("");

    inPreferencesDialog = false;

    t.elapsed(__LINE__);

    ui->tabWidget->setCurrentIndex(0); // Music Player tab is primary, regardless of last setting in Qt Designer
    on_tabWidget_currentChanged(0);     // update the menu item names

//    ui->tabWidget->setTabText(lyricsTabNumber, "Lyrics");  // c.f. Preferences

    // ----------
//    connect(ui->songTable->horizontalHeader(),&QHeaderView::sectionResized,
//            this, &MainWindow::columnHeaderResized);
    connect(ui->songTable->horizontalHeader(),&QHeaderView::sortIndicatorChanged,
            this, &MainWindow::columnHeaderSorted);

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

    ui->titleSearch->setFocus();  // this should be the intial focus

#ifdef DEBUGCLOCK
    analogClock->tipLengthAlarmMinutes = 10;  // FIX FIX FIX
    analogClock->breakLengthAlarmMinutes = 10;
#endif
    analogClock->tipLengthAlarmMinutes = tipLengthTimerLength;
    analogClock->breakLengthAlarmMinutes = breakLengthTimerLength;

    ui->warningLabel->setText("");
    ui->warningLabel->setStyleSheet("QLabel { color : red; }");
    ui->warningLabelCuesheet->setText("");
    ui->warningLabelCuesheet->setStyleSheet("QLabel { color : red; }");
    ui->warningLabelSD->setText("");
    ui->warningLabelSD->setStyleSheet("QLabel { color : red; }");

    t.elapsed(__LINE__);

    // LYRICS TAB ------------
    ui->pushButtonSetIntroTime->setEnabled(false);  // initially not singing call, buttons will be greyed out on Lyrics tab
    ui->pushButtonSetOutroTime->setEnabled(false);
    ui->dateTimeEditIntroTime->setEnabled(false);  // initially not singing call, buttons will be greyed out on Lyrics tab
    ui->dateTimeEditOutroTime->setEnabled(false);

    t.elapsed(__LINE__);
    ui->pushButtonTestLoop->setHidden(true);
    ui->pushButtonTestLoop->setEnabled(false);

    t.elapsed(__LINE__);
//    analogClock->setTimerLabel(ui->warningLabel, ui->warningLabelCuesheet);  // tell the clock which label to use for the patter timer
    analogClock->setTimerLabel(ui->warningLabel, ui->warningLabelCuesheet, ui->warningLabelSD);  // tell the clock which labels to use for the main patter timer

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

    currentSongType = "";
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

    t.elapsed(__LINE__);

    // let's look through the items in the SD menu (this method is less fragile now)
    QStringList ag1;
    ag1 << "Normal" << "Color only" << "Mental image" << "Sight" << "Random" << "Random Color only"; // OK to have one be prefix of another

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
//                qDebug() << "ag1 item: " << action->text(); // top level item
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

    {
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

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    ui->songTable->verticalScrollBar()->setSingleStep(10);
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

    ui->pushButtonCueSheetEditTitle->setStyleSheet("font-weight: bold;");

    ui->pushButtonCueSheetEditBold->setStyleSheet("font-weight: bold;");
    ui->pushButtonCueSheetEditItalic->setStyleSheet("font: italic;");

    ui->pushButtonCueSheetEditHeader->setStyleSheet("color: red");

    t.elapsed(__LINE__);

    ui->pushButtonCueSheetEditArtist->setStyleSheet("color: #0000FF");

    t.elapsed(__LINE__);

    ui->pushButtonCueSheetEditLabel->setStyleSheet("color: #60C060");

    ui->pushButtonCueSheetEditLyrics->setStyleSheet(
                "QPushButton {background-color: #FFC0CB; color: #000000; border-radius:4px; padding:1px 8px; border:0.5px solid #CF9090;}"
                "QPushButton:checked { background-color: qlineargradient(x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 #1E72FE, stop: 1 #3E8AFC); color: #FFFFFF; border:0.5px solid #0D60E3;}"
                "QPushButton:pressed { background-color: qlineargradient(x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 #1E72FE, stop: 1 #3E8AFC); color: #FFFFFF; border:0.5px solid #0D60E3;}"
                "QPushButton:disabled { background-color: #FFC0CB; color: #000000; border-radius:4px; padding:1px 8px; border:0.5px solid #CF9090;}"
                );

    t.elapsed(__LINE__);

    maybeLoadCSSfileIntoTextBrowser();

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
    setCurrentSessionIdReloadSongAgesCheckMenu(
        static_cast<SessionDefaultType>(prefsManager.GetSessionDefault() == SessionDefaultDOW)
        ? songSettings.currentSessionIDByTime() : 1); // on app entry, ages must show current session
    populateMenuSessionOptions();

    // mutually exclusive items in Flash Call Timing menu
    flashCallTimingActionGroup = new QActionGroup(this);
    ui->action5_seconds->setActionGroup(flashCallTimingActionGroup);
    ui->action10_seconds->setActionGroup(flashCallTimingActionGroup);
    ui->action15_seconds->setActionGroup(flashCallTimingActionGroup);
    ui->action20_seconds->setActionGroup(flashCallTimingActionGroup);
    ui->action15_seconds->setChecked(true);

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

    lastSongTableRowSelected = -1;  // meaning "no selection"

    lockForEditing();
    ui->pushButtonSetIntroTime->setEnabled(false);
    ui->pushButtonSetOutroTime->setEnabled(false);
    ui->dateTimeEditIntroTime->setEnabled(false);
    ui->dateTimeEditOutroTime->setEnabled(false);

    cBass->SetIntelBoostEnabled(prefsManager.GetintelBoostIsEnabled());
    cBass->SetIntelBoost(FREQ_KHZ, static_cast<float>(prefsManager.GetintelCenterFreq_KHz()/10.0)); // yes, we have to initialize these manually
    cBass->SetIntelBoost(BW_OCT,  static_cast<float>(prefsManager.GetintelWidth_oct()/10.0));
    cBass->SetIntelBoost(GAIN_DB, static_cast<float>(prefsManager.GetintelGain_dB()/10.0));  // expressed as positive number

    on_actionShow_group_station_toggled(prefsManager.Getenablegroupstation());
    on_actionShow_order_sequence_toggled(prefsManager.Getenableordersequence());
    {
        QString sizesStr = prefsManager.GetSDTabVerticalSplitterPosition();
        if (!sizesStr.isEmpty())
        {
            QList<int> sizes;
            for (const QString &sizeStr : sizesStr.split(","))
            {
                sizes.append(sizeStr.toInt());
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
            ui->splitterSDTabVertical->setSizes(sizes);
        }
    }
    {
        QString sizesStr = prefsManager.GetSDTabHorizontalSplitterPosition();
        if (!sizesStr.isEmpty())
        {
            QList<int> sizes;
            for (const QString &sizeStr : sizesStr.split(","))
            {
                sizes.append(sizeStr.toInt());
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

            ui->splitterSDTabHorizontal->setSizes(sizes);
        }
    }

    sdSliderSidesAreSwapped = false;  // start out NOT swapped
    if (prefsManager.GetSwapSDTabInputAndAvailableCallsSides())
    {
        // if we SHOULD be swapped, swap them...
        sdSliderSidesAreSwapped = true;
        ui->splitterSDTabHorizontal->addWidget(ui->splitterSDTabHorizontal->widget(0));
    }

    connect(ui->textBrowserCueSheet, SIGNAL(copyAvailable(bool)),
            this, SLOT(LyricsCopyAvailable(bool)));

    // Finally, if there was a playlist loaded the last time we ran SquareDesk, load it again
    QString loadThisPlaylist = prefsManager.GetlastPlaylistLoaded(); // "" if no playlist was loaded

    if (loadThisPlaylist != "") {
        QString fullPlaylistPath = musicRootPath + "/playlists/" + loadThisPlaylist + ".csv";
        finishLoadingPlaylist(fullPlaylistPath); // load it!
    }

    stopLongSongTableOperation("MainWindow");

//    qDebug() << "selected: " << ui->songTable->selectedItems().first()->row();

    on_songTable_itemSelectionChanged(); // call this to update the colors

    // startup the file watcher, AND make sure that focus has gone to the Title search field
    QTimer::singleShot(2000, [this]{
//            qDebug("Starting up FileWatcher now (intentionally delayed from app startup, to avoid Box.net locks retriggering loadMusicList)");
            QObject::connect(&musicRootWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(musicRootModified(QString)));
            if (!ui->titleSearch->hasFocus()) {
//                qDebug() << "HACK: TITLE SEARCH DOES NOT HAVE FOCUS. FIXING THIS.";
                ui->titleSearch->setFocus();
            }
        });

    on_actionSD_Output_triggered(); // initialize visibility of SD Output tab in SD tab
    on_actionShow_Frames_triggered(); // show or hide frames
//    ui->actionSequence_Designer->setChecked(true);
    ui->actionDance_Arranger->setChecked(true);  // this sets the default view

    // hide both menu items for now
    ui->actionSequence_Designer->setVisible(false);
    ui->actionDance_Arranger->setVisible(false);

//    // ------------------
//    // set up Dances menu, items are mutually exclusive
//    sdActionGroupDances = new QActionGroup(this);  // Dances are mutually exclusive
//    sdActionGroupDances->setExclusive(true);
//    connect(sdActionGroupDances, SIGNAL(triggered(QAction*)), this, SLOT(sdActionTriggeredDances(QAction*)));

//    QString parentFolder = musicRootPath + "/sd/frames";
////    QStringList allDances;
////    QStringList allDancesShortnames;
//    QDirIterator directories(parentFolder, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

//    QMenu *dancesSubmenu = new QMenu("Dances");

//    int whichItem = 0;
//    while(directories.hasNext()){
//        directories.next();
//        QString thePath = directories.filePath();
////        allDances << thePath;
//        QString shortName = thePath.split('/').takeLast();
////        qDebug() << "shortName: " << shortName;

//        QAction *actionOne = dancesSubmenu->addAction(shortName);
//        actionOne->setActionGroup(sdActionGroupDances);
//        actionOne->setCheckable(true);
//        if (whichItem == 0) {
//            sdActionTriggeredDances(actionOne); // call the init function for the very first item in the list TODO: remember item
//            frameName = shortName;
//        }
//        actionOne->setChecked(whichItem++ == 0); // first item gets checked, others not
//    }

////    QAction *actionOne = dancesSubmenu->addAction("One");
////    actionOne->setActionGroup(sdActionGroupDances);
////    actionOne->setCheckable(true);
////    actionOne->setChecked(true);

////    QAction *actionTwo = dancesSubmenu->addAction("Two");
////    actionTwo->setActionGroup(sdActionGroupDances);
////    actionTwo->setCheckable(true);
////    actionTwo->setChecked(false);

//    ui->menuSequence->insertMenu(ui->actionDance, dancesSubmenu);
//    ui->actionDance->setVisible(false);  // TODO: This is a kludge, because it's just a placeholder, so I could stick the Dances item at the top
//    // ------------------

    connect(ui->boy1,  &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->girl1, &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->boy2,  &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->girl2, &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->boy3,  &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->girl3, &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->boy4,  &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);
    connect(ui->girl4, &QLineEdit::textChanged, this, &MainWindow::dancerNameChanged);

    ui->boy1->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE1COLOR.name() + ";}");
    ui->girl1->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE1COLOR.name() + ";}");
    ui->boy2->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE2COLOR.name() + ";}");
    ui->girl2->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE2COLOR.name() + ";}");
    ui->boy3->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE3COLOR.name() + ";}");
    ui->girl3->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE3COLOR.name() + ";}");
    ui->boy4->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE4COLOR.name() + ";}");
    ui->girl4->setStyleSheet(QString("QLineEdit {background-color: ") + COUPLE4COLOR.name() + ";}");

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
    } else if (sdLevel == "C3A") {
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

//    qDebug() << "allDances, dances: " << allDances << dances;

    QMenu *dancesSubmenu = new QMenu("Dances");

    int whichItem = 0;
    frameName = "";
    for (const auto& shortName : dances) {
//        qDebug() << "shortName: " << shortName;

        QAction *actionOne = dancesSubmenu->addAction(shortName);
        actionOne->setActionGroup(sdActionGroupDances);
        actionOne->setCheckable(true);
        if (whichItem == 0) {
            sdActionTriggeredDances(actionOne); // call the init function for the very first item in the list TODO: remember item
            frameName = shortName;
        }
        actionOne->setChecked(whichItem++ == 0); // first item gets checked, others not
    }

    if (frameName == "") {
        // if ZERO dances found, make one
        sdLoadDance("SampleDance"); // TODO: is there a better name for this?
        // NOTE: if there was SOME frame (dance) found, then don't load it until later, when we make the menu
    }

    ui->menuSequence->insertMenu(ui->actionDance, dancesSubmenu);
    ui->actionDance->setVisible(false);  // TODO: This is a kludge, because it's just a placeholder, so I could stick the Dances item at the top
    // ------------------


    // TODO: if any frameVisible = {central, sidebar} file is not found, disable all the edit buttons for now
    if (SDtestmode) {
        ui->pushButtonSDSave->setVisible(false);
        ui->pushButtonSDUnlock->setVisible(false);
        ui->pushButtonSDNew->setVisible(false);
    }

    QMenu *saveSDMenu = new QMenu(this);
//    saveSDMenu->addAction("Save Sequence to Current Frame", /* QKeyCombination(Qt::ControlModifier, Qt::Key_S), */ this, [this]{ SDReplaceCurrentSequence(); }); // CMD-S to save (later)
////    saveSDMenu->addAction("Delete Sequence from Current Frame", this, [this]{ SDDeleteCurrentSequence(); });
//    saveSDMenu->addAction("Delete Sequence from Current Frame", this, [this]{ on_pushButtonSDDelete_clicked(); });
//    saveSDMenu->addSeparator();

    // TODO: passing parameters in the SLOT portion?  THIS IS THE IDIOMATIC WAY TO DO IT **********
    //   from: https://stackoverflow.com/questions/5153157/passing-an-argument-to-a-slot

    // TODO: These strings must be dynamically created, based on current selections for F1-F7 frame files
    QMenu* submenuMove = saveSDMenu->addMenu("Move Sequence to");
    submenuMove->addAction(QString("F1 ") + frameFiles[0] + " ", QKeyCombination(Qt::ShiftModifier, Qt::Key_F1), this, [this]{ SDMoveCurrentSequenceToFrame(0); });
    submenuMove->addAction(QString("F2 ") + frameFiles[1] + " ", QKeyCombination(Qt::ShiftModifier, Qt::Key_F2), this, [this]{ SDMoveCurrentSequenceToFrame(1); });
    submenuMove->addAction(QString("F3 ") + frameFiles[2] + " ", QKeyCombination(Qt::ShiftModifier, Qt::Key_F3), this, [this]{ SDMoveCurrentSequenceToFrame(2); });
    submenuMove->addAction(QString("F4 ") + frameFiles[3] + " ", QKeyCombination(Qt::ShiftModifier, Qt::Key_F4), this, [this]{ SDMoveCurrentSequenceToFrame(3); });
//    submenuMove->addAction("F5 ceder.a2",    QKeyCombination(Qt::ShiftModifier, Qt::Key_F5), this, [this]{ SDMoveCurrentSequenceToFrame(4); });
//    submenuMove->addAction("F6 local.plus",  QKeyCombination(Qt::ShiftModifier, Qt::Key_F6), this, [this]{ SDMoveCurrentSequenceToFrame(5); });
//    submenuMove->addAction("F7 local.c1",    QKeyCombination(Qt::ShiftModifier, Qt::Key_F7), this, [this]{ SDMoveCurrentSequenceToFrame(6); });

    QMenu* submenuCopy = saveSDMenu->addMenu("Append Sequence to");
    submenuCopy->addAction(QString("F1 ") + frameFiles[0] + " ", QKeyCombination(Qt::AltModifier | Qt::ShiftModifier, Qt::Key_F1), this, [this]{ SDAppendCurrentSequenceToFrame(0); });
    submenuCopy->addAction(QString("F2 ") + frameFiles[1] + " ", QKeyCombination(Qt::AltModifier | Qt::ShiftModifier, Qt::Key_F2), this, [this]{ SDAppendCurrentSequenceToFrame(1); });
    submenuCopy->addAction(QString("F3 ") + frameFiles[2] + " ", QKeyCombination(Qt::AltModifier | Qt::ShiftModifier, Qt::Key_F3), this, [this]{ SDAppendCurrentSequenceToFrame(2); });
    submenuCopy->addAction(QString("F4 ") + frameFiles[3] + " ", QKeyCombination(Qt::AltModifier | Qt::ShiftModifier, Qt::Key_F4), this, [this]{ SDAppendCurrentSequenceToFrame(3); });
//    submenuCopy->addAction("F5 ceder.a2",    QKeyCombination(Qt::AltModifier | Qt::ShiftModifier, Qt::Key_F5), this, [this]{ SDAppendCurrentSequenceToFrame(4); });
//    submenuCopy->addAction("F6 local.plus",  QKeyCombination(Qt::AltModifier | Qt::ShiftModifier, Qt::Key_F6), this, [this]{ SDAppendCurrentSequenceToFrame(5); });
//    submenuCopy->addAction("F7 local.c1",    QKeyCombination(Qt::AltModifier | Qt::ShiftModifier, Qt::Key_F7), this, [this]{ SDAppendCurrentSequenceToFrame(6); });

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

//    debugCSDSfile("plus"); // DEBUG DEBUG DEBUG THIS HELPS TO DEBUG IMPORT OF CSDS SEQUENCES TO SD FORMAT *********

    newSequenceInProgress = editSequenceInProgress = false; // no sequence being edited right now.
    on_actionFormation_Thumbnails_triggered(); // make sure that the thumbnails are turned OFF, if Formation Thumbnails is not initially checked

    if (ui->songTable->rowCount() >= 1) {
        ui->songTable->selectRow(0); // select row 1 after initial load of the songTable (if there are rows)
    }

}


void MainWindow::fileWatcherTriggered() {
//    qDebug() << "fileWatcherTriggered()";
    musicRootModified(QString("DONE"));
}

void MainWindow::fileWatcherDisabledTriggered() {
//    qDebug() << "***** fileWatcherDisabledTriggered(): FileWatcher is re-enabled now....";
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

//    qDebug() << "Music root modified (File Watcher awakened for real!): " << s;
    if (!filewatcherShouldIgnoreOneFileSave) { // yes, we need this here, too...because root watcher watches playlists (don't ask me!)
        // qDebug() << "*** musicRootModified!!!";
        Qt::SortOrder sortOrder(ui->songTable->horizontalHeader()->sortIndicatorOrder());
        int sortSection(ui->songTable->horizontalHeader()->sortIndicatorSection());
        // reload the musicTable.  Note that it will switch to default sort order.
        //   TODO: At some point, this probably should save the sort order, and then restore it.
//        qDebug() << "WE ARE RELOADING THE SONG TABLE NOW ------";
        findMusic(musicRootPath, true);  // get the filenames from the user's directories
        loadMusicList(); // and filter them into the songTable
        ui->songTable->horizontalHeader()->setSortIndicator(sortSection, sortOrder);
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
    PerfTimer t("reloadSongAges", __LINE__);
    QHash<QString,QString> ages;
    songSettings.getSongAges(ages, show_all_ages);

    ui->songTable->setSortingEnabled(false);
    ui->songTable->hide();

    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QString origPath = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
        QString path = songSettings.removeRootDirs(origPath);
        QHash<QString,QString>::const_iterator age = ages.constFind(path);

        ui->songTable->item(i,kAgeCol)->setText(age == ages.constEnd() ? "" : ageToIntString(age.value()));
        ui->songTable->item(i,kAgeCol)->setTextAlignment(Qt::AlignCenter);

        ui->songTable->item(i,kRecentCol)->setText(age == ages.constEnd() ? "" : ageToRecent(age.value()));
        ui->songTable->item(i,kRecentCol)->setTextAlignment(Qt::AlignCenter);
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
    if (currentIndex != -1 && !cuesheetEditorReactingToCursorMovement) {
        QString cuesheetFilename = ui->comboBoxCuesheetSelector->itemData(currentIndex).toString();
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
                    setCurrentSessionIdReloadSongAges(session.id);
                    break;
                }
            }
            break;
        }
    }
}

void MainWindow::populateMenuSessionOptions()
{
    QList<QAction *> oldActions(sessionActionGroup->actions());
    for (auto action : oldActions)
    {
        sessionActionGroup->removeAction(action);
        ui->menuSession->removeAction(action);
    }

    QList<QAction *> sessionActions(ui->menuSession->actions());
    QAction *topSeparator = sessionActions[0];
    
    int id = songSettings.getCurrentSession();
    QList<SessionInfo> sessions(songSettings.getSessionInfo());
    
    for (const auto &session : sessions)
    {
        QAction *action = new QAction(session.name, this);
        action->setCheckable(true);
        ui->menuSession->insertAction(topSeparator, action);
        connect(action, SIGNAL(triggered()), this, SLOT(action_session_change_triggered()));
        sessionActionGroup->addAction(action);
        if (session.id == id)
            action->setChecked(true);
    }
}

void MainWindow::on_menuLyrics_aboutToShow()
{
    // only allow Save if it's not a template, and the doc was modified
    ui->actionSave->setEnabled(ui->textBrowserCueSheet->document()->isModified() && !loadedCuesheetNameWithPath.contains(".template.html"));
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

    ui->pushButtonTestLoop->setHidden(!songTypeNamesForPatter.contains(currentSongType)); // this button is PATTER ONLY
    return;
}


void MainWindow::on_actionShow_All_Ages_triggered(bool checked)
{
    reloadSongAges(checked);
}



// ----------------------------------------------------------------------
MainWindow::~MainWindow()
{
    SDSetCurrentSeqs(0);  // this doesn't take very long

    // bug workaround: https://bugreports.qt.io/browse/QTBUG-56448
    QColorDialog colorDlg(nullptr);
    colorDlg.setOption(QColorDialog::NoButtons);
    colorDlg.setCurrentColor(Qt::white);

    delete ui;
    delete sd_redo_stack;

    if (sdthread)
    {
        sdthread->finishAndShutdownSD();
    }
//    if (ps) {
//        ps->kill();
//    }

    if (prefsManager.GetenableAutoAirplaneMode()) {
        airplaneMode(false);
    }

    delete[] danceProgramActions;
    delete sessionActionGroup;
    delete sdActionGroupDanceProgram;
    delete cBass;
    QString currentMusicRootPath = prefsManager.GetmusicPath();
    clearLockFile(currentMusicRootPath); // release the lock that we took (other locks were thrown away)
}

// ----------------------------------------------------------------------
void MainWindow::updateSongTableColumnView()
{
    ui->songTable->setColumnHidden(kRecentCol,!prefsManager.GetshowRecentColumn());
    ui->songTable->setColumnHidden(kAgeCol,!prefsManager.GetshowAgeColumn());
    ui->songTable->setColumnHidden(kPitchCol,!prefsManager.GetshowPitchColumn());
    ui->songTable->setColumnHidden(kTempoCol,!prefsManager.GetshowTempoColumn());

    // http://www.qtcentre.org/threads/3417-QTableWidget-stretch-a-column-other-than-the-last-one
    QHeaderView *headerView = ui->songTable->horizontalHeader();
    headerView->setSectionResizeMode(kNumberCol, QHeaderView::Interactive);
    headerView->setSectionResizeMode(kTypeCol, QHeaderView::Interactive);
    headerView->setSectionResizeMode(kLabelCol, QHeaderView::Interactive);
    headerView->setSectionResizeMode(kTitleCol, QHeaderView::Stretch);

    headerView->setSectionResizeMode(kRecentCol, QHeaderView::Fixed);
    headerView->setSectionResizeMode(kAgeCol, QHeaderView::Fixed);
    headerView->setSectionResizeMode(kPitchCol, QHeaderView::Fixed);
    headerView->setSectionResizeMode(kTempoCol, QHeaderView::Fixed);
    headerView->setStretchLastSection(false);

    ui->songTable->horizontalHeaderItem(kNumberCol)->setTextAlignment( Qt::AlignCenter | Qt::AlignVCenter );
    ui->songTable->horizontalHeaderItem(kRecentCol)->setTextAlignment( Qt::AlignCenter | Qt::AlignVCenter );
    ui->songTable->horizontalHeaderItem(kAgeCol)->setTextAlignment( Qt::AlignCenter | Qt::AlignVCenter );
    ui->songTable->horizontalHeaderItem(kPitchCol)->setTextAlignment( Qt::AlignCenter | Qt::AlignVCenter );
    ui->songTable->horizontalHeaderItem(kTempoCol)->setTextAlignment( Qt::AlignCenter | Qt::AlignVCenter );
}


// ----------------------------------------------------------------------
void MainWindow::on_loopButton_toggled(bool checked)
{
    if (checked) {
        ui->actionLoop->setChecked(true);

        ui->seekBar->SetLoop(true);
        ui->seekBarCuesheet->SetLoop(true);

        double songLength = cBass->FileLength;
//        qDebug() << "songLength: " << songLength << ", Intro: " << ui->seekBar->GetIntro();

//        cBass->SetLoop(songLength * 0.9, songLength * 0.1); // FIX: use parameters in the MP3 file
        cBass->SetLoop(songLength * static_cast<double>(ui->seekBar->GetOutro()),
                      songLength * static_cast<double>(ui->seekBar->GetIntro()));
    }
    else {
        ui->actionLoop->setChecked(false);

        ui->seekBar->SetLoop(false);
        ui->seekBarCuesheet->SetLoop(false);

        cBass->ClearLoop();
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_monoButton_toggled(bool checked)
{
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
void MainWindow::on_stopButton_clicked()
{
// TODO: instead of removing focus on STOP, better we should restore focus to the previous focused widget on STOP
//    if (QApplication::focusWidget() != NULL) {
//        QApplication::focusWidget()->clearFocus();  // we don't want to continue editing the search fields after a STOP
//                                                    //  or it will eat our keyboard shortcuts
//    }

    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
    ui->actionPlay->setText("Play");  // now stopped, press Cmd-P to Play
//    currentState = kStopped;

    cBass->Stop();                 // Stop playback, rewind to the beginning
    cBass->StopAllSoundEffects();  // and, it also stops ALL sound effects

    setNowPlayingLabelWithColor(currentSongTitle);

#ifdef REMOVESILENCE
    // last thing we do is move the stream position to 1 sec before start of music
    // this will move BOTH seekBar's to the right spot
    cBass->StreamSetPosition((double)startOfSong_sec);
    Info_Seekbar(false);  // update just the text
#else
    ui->seekBar->setValue(0);
    ui->seekBarCuesheet->setValue(0);
    Info_Seekbar(false);  // update just the text
#endif

    int cindex = ui->tabWidget->currentIndex();  // get index of tab, so we can see which it is
    bool tabIsSD = (ui->tabWidget->tabText(cindex) == "SD");

     // if it's the SD tab, do NOT change focus to songTable, leave it in the Current Sequence pane
    if (!tabIsSD) {
        ui->songTable->setFocus();
    } else {
//        qDebug() << "stopButtonClicked: Tab was SD, so NOT changing focus to songTable";
    }

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

// ----------------------------------------------------------------------
void MainWindow::on_playButton_clicked()
{
    PerfTimer t("MainWindow::on_playButtonClicked", __LINE__);
    if (!songLoaded) {
        return;  // if there is no song loaded, no point in toggling anything.
    }

    int cindex = ui->tabWidget->currentIndex();  // get index of tab, so we can see which it is
    bool tabIsSD = (ui->tabWidget->tabText(cindex) == "SD");

//    if (currentState == kStopped || currentState == kPaused) {
    uint32_t Stream_State = cBass->currentStreamState();
    if (Stream_State != BASS_ACTIVE_PLAYING) {
        cBass->Play();  // currently stopped or paused or stalled, so try to start playing

        // randomize the Flash Call, if PLAY (but not PAUSE) is pressed
        randomizeFlashCall();

        if (firstTimeSongIsPlayed)
        {
            PerfTimer t("MainWindow::on_playButtonClicked firstTimeSongIsPlayed", __LINE__);

            firstTimeSongIsPlayed = false;
            saveCurrentSongSettings();
            if (!ui->actionDon_t_Save_Plays->isChecked())
            {
                songSettings.markSongPlayed(currentMP3filename, currentMP3filenameWithPath);
//                QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
//                QModelIndexList selected = selectionModel->selectedRows();

                ui->songTable->setSortingEnabled(false);

// SONGTABLEREFACTOR
                int row = getSelectionRowForFilename(currentMP3filenameWithPath);
                if (row != -1)
                {
                    ui->songTable->item(row, kAgeCol)->setText("0");
                    ui->songTable->item(row, kAgeCol)->setTextAlignment(Qt::AlignCenter);

                    ui->songTable->item(row, kRecentCol)->setText(ageToRecent("0"));
                    ui->songTable->item(row, kRecentCol)->setTextAlignment(Qt::AlignCenter);
                }
            }

            if (switchToLyricsOnPlay &&
                    (songTypeNamesForSinging.contains(currentSongType) || songTypeNamesForCalled.contains(currentSongType)))
            {
                // switch to Lyrics tab ONLY for singing calls or vocals
              if (currentSongType == "singing")  // do not switch if patter (using Cuesheet tab for written Patter)
                    {
                      ui->tabWidget->setCurrentIndex(1);
                    }
            }
            ui->songTable->setSortingEnabled(true);
        }

        // If we just started playing, clear focus from all widgets
        if (QApplication::focusWidget() != nullptr) {
            lastWidgetBeforePlaybackWasSongTable = (QApplication::focusWidget() == ui->songTable); // for restore on STOP
            oldFocusWidget = QApplication::focusWidget();

            if (!tabIsSD) {
//                qDebug() << "playButtonClicked: Tab was NOT SD, so clearing focus from existing widget";
                QApplication::focusWidget()->clearFocus();  // we don't want to continue editing the search fields after a STOP
                                                            //  or it will eat our keyboard shortcuts (like P, J, K, period, etc.)
            } else {
//                qDebug() << "playButtonClicked: Tab was SD, so NOT clearing focus from existing widget";
            }
        }
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));  // change PLAY to PAUSE
        ui->actionPlay->setText("Pause");

//        ui->songTable->setFocus(); // while playing, songTable has focus

        // if it's the SD tab, do NOT change focus to songTable, leave it in the Current Sequence pane
        if (!tabIsSD) {
//            qDebug() << "playButtonClicked: Tab was NOT SD, so changing focus to songTable";
            ui->songTable->setFocus();
        } else {
//            qDebug() << "playButtonClicked: Tab was SD, so NOT changing focus to songTable";
        }


//        currentState = kPlaying;
    }
    else {
        // TODO: we might want to restore focus here....
        // currently playing, so pause playback
        cBass->Pause();
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
        ui->actionPlay->setText("Play");
//        currentState = kPaused;
        setNowPlayingLabelWithColor(currentSongTitle);

        // restore focus
        if (oldFocusWidget != nullptr  && !tabIsSD) { // only set focus back on pause when tab is NOT SD
            oldFocusWidget->setFocus();
        }
//        if (lastWidgetBeforePlaybackWasSongTable) {
////            qDebug() << "restoring songTable focus...";
//            ui->songTable->setFocus();  // if STOP, restore the focus to widget that had it before PLAY began, IFF it was songTable
//        }

    }

}

// SONGTABLEREFACTOR
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
    ui->currentPitchLabel->setText(sign + QString::number(currentPitch) +" semitone" + plural);

    saveCurrentSongSettings();
// SONGTABLEREFACTOR
    // update the hidden pitch column
    ui->songTable->setSortingEnabled(false);
    int row = getSelectionRowForFilename(currentMP3filenameWithPath);
    if (row != -1)
    {
        ui->songTable->item(row, kPitchCol)->setText(QString::number(currentPitch)); // already trimmed()
    }
    ui->songTable->setSortingEnabled(true);

    // special checking for playlist ------
    if (targetNumber != "" && !loadingSong) {
        // current song is on a playlist, AND we're not doing the initial load
//        qDebug() << "current song is on playlist, and PITCH changed!";
        markPlaylistModified(true); // turn ON the * in the status bar, because a playlist entry changed its tempo
    }
}

// ----------------------------------------------------------------------
void MainWindow::Info_Volume(void)
{
    int volSliderPos = ui->volumeSlider->value();
    if (volSliderPos == 0) {
        ui->currentVolumeLabel->setText("MUTE");
        ui->currentVolumeLabel->setStyleSheet("QLabel { color : red; }");
    }
    else if (volSliderPos == 100) {
        ui->currentVolumeLabel->setText("MAX");
        ui->currentVolumeLabel->setStyleSheet("QLabel { color : black; }");
    }
    else {
        ui->currentVolumeLabel->setText(QString::number(volSliderPos)+"%");
        if (volSliderPos >= 50) {
            ui->currentVolumeLabel->setStyleSheet("QLabel { color : royalblue; }");
        } else {
            ui->currentVolumeLabel->setStyleSheet("QLabel { color : indianred; }");
        }
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_volumeSlider_valueChanged(int value)
{
    int voltageLevelToSet = static_cast<int>(100.0*pow(10.0,(((value*0.8)+20)/2.0 - 50)/20.0));
    if (value == 0) {
        voltageLevelToSet = 0;  // special case for slider all the way to the left (MUTE)
    }
    cBass->SetVolume(voltageLevelToSet);     // now logarithmic, 0 --> 0.01, 50 --> 0.1, 100 --> 1.0 (values * 100 for libbass)
    currentVolume = static_cast<unsigned short>(value);    // this will be saved with the song (0-100)

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
//    qDebug() << "on_tempoSlider_valueChanged:" << value;
    if (tempoIsBPM) {
        double desiredBPM = static_cast<double>(value);            // desired BPM
        int newBASStempo = static_cast<int>(round(100.0*desiredBPM/baseBPM));
        cBass->SetTempo(newBASStempo);
        ui->currentTempoLabel->setText(QString::number(value) + " BPM (" + QString::number(newBASStempo) + "%)");
    }
    else {
        double basePercent = 100.0;                      // original detected percent
        double desiredPercent = static_cast<double>(value);            // desired percent
        int newBASStempo = static_cast<int>(round(100.0*desiredPercent/basePercent));
        cBass->SetTempo(newBASStempo);
        ui->currentTempoLabel->setText(QString::number(value) + "%");
    }

    saveCurrentSongSettings();
// SONGTABLEREFACTOR
    // update the hidden tempo column
    ui->songTable->setSortingEnabled(false);
    int row = getSelectionRowForFilename(currentMP3filenameWithPath);
    if (row != -1)
    {
        if (tempoIsBPM) {
            ui->songTable->item(row, kTempoCol)->setText(QString::number(value));
//            qDebug() << "on_tempoSlider_valueChanged: setting text for tempo to: " << QString::number(value);
        }
        else {
            ui->songTable->item(row, kTempoCol)->setText(QString::number(value) + "%");
//            qDebug() << "on_tempoSlider_valueChanged: setting text for tempo to: " << QString::number(value) + "%";
        }
    }
    ui->songTable->setSortingEnabled(true);

    // special checking for playlist ------
    if (targetNumber != "" && !loadingSong) {
        // current song is on a playlist, AND we're not doing the initial load
//        qDebug() << "current song is on playlist, and TEMPO changed!";
        markPlaylistModified(true); // turn ON the * in the status bar, because a playlist entry changed its tempo
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
    cBass->SetPan(value/100.0);

    saveCurrentSongSettings();  // MIX should be persisted when changed
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
    seekBar->ClearMarkers();
}

// ----------------------------------------------------------------------
void MainWindow::Info_Seekbar(bool forceSlider)
{
    static bool in_Info_Seekbar = false;
    if (in_Info_Seekbar) {
        return;
    }
    RecursionGuard recursion_guard(in_Info_Seekbar);

    if (songLoaded) {
        cBass->StreamGetPosition();  // update cBass->Current_Position

        if (ui->pitchSlider->value() != cBass->Stream_Pitch) {  // DEBUG DEBUG DEBUG
            qDebug() << "ERROR: Song was loaded, and cBass pitch did not match pitch slider!";
            qDebug() << "    pitchSlider =" << ui->pitchSlider->value();
            qDebug() << "    cBass->Pitch/Tempo/Volume = " << cBass->Stream_Pitch << ", " << cBass->Stream_Tempo << ", " << cBass->Stream_Volume;
            cBass->SetPitch(ui->pitchSlider->value());
        }

        int currentPos_i = static_cast<int>(cBass->Current_Position);

// to help track down the failure-to-progress error...
#define WATCHDOG460
#ifdef WATCHDOG460
        // watchdog for "failure to progress" error ------
        if (cBass->currentStreamState() == BASS_ACTIVE_PLAYING) {
            if (cBass->Current_Position <= previousPosition
                    // || previousPosition > 10.0  // DEBUG
               )
            {
//                qDebug() << "Failure/manual seek:" << previousPosition << cBass->Current_Position
//                         << "songloaded: " << songLoaded;
                previousPosition = -1.0;  // uninitialized
            } else {
//                qDebug() << "Progress:" << previousPosition << cBass->Current_Position;
                // this will always be executed on the first info_seekbar update when playing
                previousPosition = cBass->Current_Position;  // remember previous position
            }
        } else {
//            qDebug() << "Paused:" << previousPosition << currentPos_i;
            // when stopped, we'll set previous position to -1
            previousPosition = -1.0;  // uninitialized
        }
#endif

        if (forceSlider) {
            SetSeekBarPosition(ui->seekBar, currentPos_i);
            SetSeekBarPosition(ui->seekBarCuesheet, currentPos_i);

            int minScroll = ui->textBrowserCueSheet->verticalScrollBar()->minimum();
            int maxScroll = ui->textBrowserCueSheet->verticalScrollBar()->maximum();
            int maxSeekbar = ui->seekBar->maximum();  // NOTE: minSeekbar is always 0
            double fracSeekbar = static_cast<double>(currentPos_i)/static_cast<double>(maxSeekbar);
            double targetScroll = 1.08 * fracSeekbar * (maxScroll - minScroll) + minScroll;  // FIX: this is heuristic and not right yet

            // NOTE: only auto-scroll when the lyrics are LOCKED (if not locked, you're probably editing).
            //   AND you must be playing.  If you're not playing, we're not going to override the InfoBar position.
            if (autoScrollLyricsEnabled &&
                    !ui->pushButtonEditLyrics->isChecked() &&
//                    currentState == kPlaying) {
                    cBass->currentStreamState() == BASS_ACTIVE_PLAYING &&
                    !lyricsForDifferentSong) {
                // lyrics scrolling at the same time as the InfoBar
                ui->textBrowserCueSheet->verticalScrollBar()->setValue(static_cast<int>(targetScroll));
            }
        }
        int fileLen_i = static_cast<int>(cBass->FileLength);

        // songs must be longer than 5 seconds for this to work (belt and suspenders)
        if ((currentPos_i == fileLen_i) && (currentPos_i > 5) && (fileLen_i > 5)) {  // NOTE: TRICKY, counts on -1 above in InitializeSeekBar()
            // avoids the problem of manual seek to max slider value causing auto-STOP
            if (!ui->actionContinuous_Play->isChecked()) {
//                qDebug() << "AUTO_STOP TRIGGERED (NORMAL): currentPos_i:" << currentPos_i << ", fileLen_i:" << fileLen_i;
                on_stopButton_clicked(); // pretend we pressed the STOP button when EOS is reached
            }
            else {
//                qDebug() << "AUTO_STOP TRIGGERED (CONT PLAY): currentPos_i:" << currentPos_i << ", fileLen_i:" << fileLen_i;
                // figure out which row is currently selected
                int row = selectedSongRow();
                if (row < 0) {
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

        if (prefsManager.GetuseTimeRemaining()) {
            // time remaining in song
            ui->currentLocLabel->setText(position2String(fileLen_i - currentPos_i, true));  // pad on the left
            ui->currentLocLabel_2->setText(position2String(fileLen_i - currentPos_i, true));  // pad on the left
        } else {
            // current position in song
            ui->currentLocLabel->setText(position2String(currentPos_i, true));              // pad on the left
            ui->currentLocLabel_2->setText(position2String(currentPos_i, true));              // pad on the left
        }

        ui->songLengthLabel->setText("/ " + position2String(fileLen_i));    // no padding

        // singing call sections
        if (songTypeNamesForSinging.contains(currentSongType) || songTypeNamesForCalled.contains(currentSongType)) {
            double introLength = static_cast<double>(ui->seekBar->GetIntro()) * cBass->FileLength; // seconds
            double outroTime = static_cast<double>(ui->seekBar->GetOutro()) * cBass->FileLength; // seconds
//            qDebug() << "InfoSeekbar()::introLength: " << introLength << ", " << outroTime;
            double outroLength = fileLen_i-outroTime;

            int section;
            if (currentPos_i < introLength) {
                section = 0; // intro
            } else if (currentPos_i > outroTime) {
                section = 8;  // tag
            } else {
                section = static_cast<int>(1.0 + 7.0*((currentPos_i - introLength)/(fileLen_i-(introLength+outroLength))));
                if (section > 8 || section < 0) {
                    section = 0; // needed for the time before fields are fully initialized
                }
            }

            QStringList sectionName;
            sectionName << "Intro" << "Opener" << "Figure 1" << "Figure 2"
                        << "Break" << "Figure 3" << "Figure 4" << "Closer" << "Tag";

//            if (cBass->Stream_State == BASS_ACTIVE_PLAYING &&
            if (//cBass->currentStreamState() == BASS_ACTIVE_PLAYING &&
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

        // FLASH CALL FEATURE ======================================
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
//            if (cBass->Stream_State == BASS_ACTIVE_PLAYING && songTypeNamesForPatter.contains(currentSongType)) {
            if (cBass->currentStreamState() == BASS_ACTIVE_PLAYING && songTypeNamesForPatter.contains(currentSongType)) {
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

    // set in ms
    ui->dateTimeEditIntroTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*position+0.5))); // milliseconds

    // set in fractional form
    double frac = position/length;
    ui->seekBarCuesheet->SetIntro(frac);  // after the events are done, do this.
    ui->seekBar->SetIntro(frac);

    on_loopButton_toggled(ui->actionLoop->isChecked()); // then finally do this, so that cBass is told what the loop points are (or they are cleared)
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

    // set in ms
    ui->dateTimeEditOutroTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*position+0.5))); // milliseconds

    // set in fractional form
    double frac = position/length;
    ui->seekBarCuesheet->SetOutro(frac);  // after the events are done, do this.
    ui->seekBar->SetOutro(frac);

    on_loopButton_toggled(ui->actionLoop->isChecked()); // then finally do this, so that cBass is told what the loop points are (or they are cleared)
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
    cBass->StreamSetPosition(value);
    Info_Seekbar(false);
}

// ----------------------------------------------------------------------
void MainWindow::on_clearSearchButton_clicked()
{
    // FIX: bug when clearSearch is pressed, the order in the songTable can change.

    // figure out which row is currently selected
    int row = selectedSongRow();

    ui->labelSearch->setText("");
    ui->typeSearch->setText("");
    ui->titleSearch->setText("");

    if (row != -1) {
        // if a row was selected, restore it after a clear search
        // FIX: this works much of the time, but it doesn't handle the case where search field is typed, then cleared.  In this case,
        //   the row isn't highlighted again.
        ui->songTable->selectRow(row);
    }

    ui->titleSearch->setFocus();  // When Clear Search is clicked (or ESC ESC), set focus to the titleSearch field, so that UP/DOWN works
    filterMusic(); // highlights first visible row (if there are any rows)
}

// ----------------------------------------------------------------------
void MainWindow::on_actionLoop_triggered()
{
    on_loopButton_toggled(ui->actionLoop->isChecked());
}

// ----------------------------------------------------------------------
void MainWindow::on_UIUpdateTimerTick(void)
{
    // This is called once per second, to update the seekbar and associated dynamic text

//    qDebug() << "VERTICAL SCROLL VALUE: " << ui->textBrowserCueSheet->verticalScrollBar()->value();

    Info_Seekbar(true);

    // update the session coloring analog clock
    QTime time = QTime::currentTime();
    int theType = NONE;
//    qDebug() << "Stream_State:" << cBass->Stream_State; //FIX
//    if (cBass->Stream_State == BASS_ACTIVE_PLAYING) {
    uint32_t Stream_State = cBass->currentStreamState();
    if (Stream_State == BASS_ACTIVE_PLAYING) {
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
//    } else if (cBass->Stream_State == BASS_ACTIVE_PAUSED) {
    } else if (Stream_State == BASS_ACTIVE_PAUSED || Stream_State == BASS_ACTIVE_STOPPED) {  // TODO: Check to make sure it doesn't mess up X86.
        // if we paused due to FADE, for example...
        // FIX: this could be factored out, it's used twice.
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
        ui->actionPlay->setText("Play");
//        currentState = kPaused;
        setNowPlayingLabelWithColor(currentSongTitle);
    }

#ifndef DEBUGCLOCK
    analogClock->setSegment(static_cast<unsigned int>(time.hour()),
                            static_cast<unsigned int>(time.minute()),
                            static_cast<unsigned int>(time.second()),
                            static_cast<unsigned int>(theType));  // always called once per second
#else
//    analogClock->setSegment(0, time.minute(), time.second(), theType);  // DEBUG DEBUG DEBUG
    analogClock->setSegment(static_cast<unsigned int>(time.minute()),
                            static_cast<unsigned int>(time.second()),
                            0,
                            static_cast<unsigned int>(theType));  // always called once per second
#endif

    // ------------------------
    // Sounds associated with Tip and Break Timers (one-shots)
    newTimerState = analogClock->currentTimerState;

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
}

// ----------------------------------------------------------------------
void MainWindow::on_vuMeterTimerTick(void)
{
    double currentVolumeSlider = ui->volumeSlider->value();
    int levelR      = cBass->StreamGetVuMeterR();        // do not reset peak detector
    int levelL_mono = cBass->StreamGetVuMeterL_mono();   // get AND reset peak detector

    double levelL_monof = (currentVolumeSlider/100.0)*levelL_mono/32768.0;
    double levelRf      = (currentVolumeSlider/100.0)*levelR/32768.0;

    bool isMono = cBass->GetMono();

    // TODO: iff music is playing.
    vuMeter->levelChanged(levelL_monof, levelRf, isMono);  // 10X/sec, update the vuMeter
}

// --------------
// follows this example: https://doc.qt.io/qt-6/qtwidgets-mainwindows-application-example.html
bool MainWindow::maybeSave() {
    QString current = ui->statusBar->currentMessage();
    bool isModified = current.endsWith('*');
    static QRegularExpression asteriskAtEndRegex("\\*$");
    current.replace(asteriskAtEndRegex, "");  // delete the star at the end, if present
    static QRegularExpression playlistColonAtStartRegex("^Playlist: ");
    current.replace(playlistColonAtStartRegex, ""); // delete off the start of the string

    if (!isModified) {
//        qDebug() << "maybeSave() returning, because playlist has not been modified";
        return true; // user wants application to close
    }

    if (current == "") {
        current = "Untitled";
    }

    const QMessageBox::StandardButton ret
        = QMessageBox::warning(this, "SquareDesk",
                               QString("The playlist '") + current + "' has been modified.\n\nDo you want to save your changes?",
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (ret) {

        case QMessageBox::Save:
//            qDebug() << "User clicked SAVE";
            if (current == "Untitled") {
                // qDebug() << "No known filename, so asking for new filename now (Save As)";
                on_actionSave_Playlist_triggered(); // File > Save As...
            } else {
//                qDebug() << "Saving as: " << current;
                savePlaylistAgain(); // File > Save
            }
//            qDebug() << "Playlist SAVED (even if user clicked CANCEL on the Save As).";
            return true; // all is well

        case QMessageBox::Cancel:
//            qDebug() << "User clicked CANCEL, returning FALSE";
            return false;

        default:
//            qDebug() << "DEFAULT";
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

    if (!maybeSave()) {
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
    msgBox.setText(QString("<p><h2>SquareDesk, V") + QString(VERSIONSTRING) + QString(" (Qt") + QString(QT_VERSION_STR) + QString(")") + QString("</h2>") +
                   QString("<p>Visit our website at <a href=\"http://squaredesk.net\">squaredesk.net</a></p>") +
                   QString("Uses: ") +
                   QString("<a href=\"http://www.lynette.org/sd\">sd</a>, ") +
//                   QString("<a href=\"http://cmusphinx.sourceforge.net\">PocketSphinx</a>, ") +
                   QString("<a href=\"https://github.com/yshurik/qpdfjs\">qpdfjs</a>, ") +
                   QString("<a href=\"https://github.com/breakfastquay/minibpm\">miniBPM</a>, ") +
//                   QString("<a href=\"http://tidy.sourceforge.net\">tidy-html5</a>, ") +
//                   QString("<a href=\"http://quazip.sourceforge.net\">QuaZIP</a>, ") +
                   QString("<a href=\"https://www.kfrlib.com\">kfr</a>, and ") +
                   QString("<a href=\"https://www.surina.net/soundtouch/\">SoundTouch</a>.") +
                   QString("<p>Thanks to: <a href=\"http://all8.com\">all8.com</a>"));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

bool MainWindow::someWebViewHasFocus() {
#define WEBVIEWSWORK
#ifdef WEBVIEWSWORK
//    qDebug() << "numWebviews: " << numWebviews;
    bool hasFocus = false;
    for (unsigned int i=0; i<numWebviews && !hasFocus; i++) {
//        qDebug() << "     checking: " << i;
        hasFocus = hasFocus || ((numWebviews > i) && (webview[i] != nullptr) && (webview[i]->hasFocus()));
    }
//    qDebug() << "     returning: " << hasFocus;
    return(hasFocus);
#else
    return(false);
#endif
}


// ------------------------------------------------------------------------
// http://www.codeprogress.com/cpp/libraries/qt/showQtExample.php?key=QApplicationInstallEventFilter&index=188
bool GlobalEventFilter::eventFilter(QObject *Object, QEvent *Event)
{
    if (Event->type() == QEvent::KeyPress) {
        QKeyEvent *KeyEvent = dynamic_cast<QKeyEvent *>(Event);
        int theKey = KeyEvent->key();

        MainWindow *maybeMainWindow = dynamic_cast<MainWindow *>((dynamic_cast<QApplication *>(Object))->activeWindow());
        if (maybeMainWindow == nullptr) {
            // if the PreferencesDialog is open, for example, do not dereference the NULL pointer (duh!).
//            qDebug() << "QObject::eventFilter()";
            return QObject::eventFilter(Object,Event);
        }

//        qDebug() << "Key event: " << KeyEvent->key() << KeyEvent->modifiers() << ui->tableWidgetCurrentSequence->hasFocus();

        int cindex = ui->tabWidget->currentIndex();  // get index of tab, so we can see which it is
        bool tabIsCuesheet = (ui->tabWidget->tabText(cindex) == CUESHEET_TAB_NAME);

        bool tabIsSD = (ui->tabWidget->tabText(cindex) == "SD");

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
        else if ( !(ui->labelSearch->hasFocus() ||      // IF NO TEXT HANDLING WIDGET HAS FOCUS...
               ui->typeSearch->hasFocus() ||
               ui->titleSearch->hasFocus() ||
               (ui->textBrowserCueSheet->hasFocus() && ui->pushButtonEditLyrics->isChecked()) ||
               ui->dateTimeEditIntroTime->hasFocus() ||
               ui->dateTimeEditOutroTime->hasFocus() ||
               ui->lineEditSDInput->hasFocus() ||
#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
               ui->lineEditCountDownTimer->hasFocus() ||
               ui->lineEditChoreographySearch->hasFocus() ||
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
               ui->songTable->isEditing() ||
//               maybeMainWindow->webview[0]->hasFocus() ||      // EXPERIMENTAL FIX FIX FIX, will crash if webview[n] not exist yet
               maybeMainWindow->someWebViewHasFocus() ) ||           // safe now (won't crash, if there are no webviews!)

             ( (ui->labelSearch->hasFocus() ||          // OR IF A TEXT HANDLING WIDGET HAS FOCUS AND ESC/` IS PRESSED
                ui->typeSearch->hasFocus() ||
                ui->titleSearch->hasFocus() ||
                ui->dateTimeEditIntroTime->hasFocus() ||
                ui->dateTimeEditOutroTime->hasFocus() ||

                ui->lineEditSDInput->hasFocus() || 
                ui->textBrowserCueSheet->hasFocus()) &&
                (theKey == Qt::Key_Escape
#if defined(Q_OS_MACOS)
                 // on MAC OS X, backtick is equivalent to ESC (e.g. devices that have TouchBar)
                 || theKey == Qt::Key_QuoteLeft
#endif
                                                          ) )  ||
                  // OR, IF ONE OF THE SEARCH FIELDS HAS FOCUS, AND RETURN/UP/DOWN_ARROW IS PRESSED
             ( (ui->labelSearch->hasFocus() || ui->typeSearch->hasFocus() || ui->titleSearch->hasFocus()) &&
               (theKey == Qt::Key_Return || theKey == Qt::Key_Up || theKey == Qt::Key_Down)
             )
                  // These next 3 help work around a problem where going to a different non-SDesk window and coming back
                  //   puts the focus into a the Type field, where there was NO FOCUS before.  But, that field usually doesn't
                  //   have any characters in it, so if the user types SPACE, the right thing happens, and it goes back to NO FOCUS.
                  // I think this is a reasonable tradeoff right now.
                  // OR, IF THE LABEL SEARCH FIELD HAS FOCUS, AND IT HAS NO CHARACTERS OF TEXT YET, AND SPACE OR PERIOD IS PRESSED
                  || (ui->labelSearch->hasFocus() && ui->labelSearch->text().length() == 0 && (theKey == Qt::Key_Space || theKey == Qt::Key_Period))
                  // OR, IF THE TYPE SEARCH FIELD HAS FOCUS, AND IT HAS NO CHARACTERS OF TEXT YET, AND SPACE OR PERIOD IS PRESSED
                  || (ui->typeSearch->hasFocus() && ui->typeSearch->text().length() == 0 && (theKey == Qt::Key_Space || theKey == Qt::Key_Period))
                  // OR, IF THE TITLE SEARCH FIELD HAS FOCUS, AND IT HAS NO CHARACTERS OF TEXT YET, AND SPACE OR PERIOD IS PRESSED
                  || (ui->titleSearch->hasFocus() && ui->titleSearch->text().length() == 0 && (theKey == Qt::Key_Space || theKey == Qt::Key_Period))
                  // OR, IF THE LYRICS TAB SET INTRO FIELD HAS FOCUS, AND SPACE OR PERIOD IS PRESSED
                  || (ui->dateTimeEditIntroTime->hasFocus() && (theKey == Qt::Key_Space || theKey == Qt::Key_Period))
                  // OR, IF THE LYRICS TAB SET OUTRO FIELD HAS FOCUS, AND SPACE OR PERIOD IS PRESSED
                  || (ui->dateTimeEditOutroTime->hasFocus() && (theKey == Qt::Key_Space || theKey == Qt::Key_Period))
           ) {
            // call handleKeypress on the Applications's active window ONLY if this is a MainWindow
//            qDebug() << "eventFilter SPECIAL KEY:" << ui << maybeMainWindow << theKey << KeyEvent->text();
            // THEN HANDLE IT AS A SPECIAL KEY

            if ((theKey == Qt::Key_Up || theKey == Qt::Key_Down) &&
                    (KeyEvent->modifiers() & Qt::ShiftModifier) &&
                    (KeyEvent->modifiers() & Qt::ControlModifier)) {

                if (theKey == Qt::Key_Up) {
//                    qDebug() << "SHIFT-CMD-UP detected.";
                    maybeMainWindow->PlaylistItemMoveUp();
                } else {
//                    qDebug() << "SHIFT-CMD-DOWN detected.";
                    maybeMainWindow->PlaylistItemMoveDown();
                }

                return true;
            }

            return (maybeMainWindow->handleKeypress(KeyEvent->key(), KeyEvent->text()));
        }

    }
    return QObject::eventFilter(Object,Event);
}

void MainWindow::actionTempoPlus()
{
    ui->tempoSlider->setValue(ui->tempoSlider->value() + 1);
    on_tempoSlider_valueChanged(ui->tempoSlider->value());
}
void MainWindow::actionTempoMinus()
{
    ui->tempoSlider->setValue(ui->tempoSlider->value() - 1);
    on_tempoSlider_valueChanged(ui->tempoSlider->value());
}
void MainWindow::actionFadeOutAndPause()
{
    cBass->FadeOutAndPause();
}
void MainWindow::actionNextTab()
{
    int currentTab = ui->tabWidget->currentIndex();
    if (currentTab == 0) {
        // if Music tab active, go to Lyrics tab
        ui->tabWidget->setCurrentIndex(lyricsTabNumber);
    } else if (currentTab == lyricsTabNumber) {
        // if Lyrics tab active, go to Music tab
        ui->tabWidget->setCurrentIndex(0);
    } else {
        // if currently some other tab, just go to the Music tab
        ui->tabWidget->setCurrentIndex(0);
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
    QString currentFilter(ui->typeSearch->text());

    if (songTypeToggleList.length() > 1)
    {
        int nextToggle = 0;

        for (int i = 0; i < songTypeToggleList.length(); ++i)
        {
            QString s = songTypeToggleList[i];
            if (0 == s.compare(currentFilter, Qt::CaseInsensitive))
            {
                nextToggle = i + 1;
            }
        }
        if (nextToggle >= songTypeToggleList.length())
            nextToggle = 0;
        ui->typeSearch->setText(songTypeToggleList[nextToggle]);
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
    if (songTypeNamesForPatter.length() > 0)
        ui->typeSearch->setText(songTypeNamesForPatter[0]);
}

void MainWindow::actionFilterSongsToSingers()
{
    if (songTypeNamesForSinging.length() > 0)
        ui->typeSearch->setText(songTypeNamesForSinging[0]);
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

            on_clearSearchButton_clicked(); // clears search fields, selects first visible item in songTable

            if (ui->labelSearch->text() != "" || ui->typeSearch->text() != "" || ui->titleSearch->text() != "") {
                // clear the search fields, if there was something in them.  (First press of ESCAPE).
//                ui->labelSearch->setText("");
//                ui->typeSearch->setText("");
//                ui->titleSearch->setText("");

            } else {
                // if the search fields were already clear, then this is the second press of ESCAPE (or the first press
                //   of ESCAPE when the search function was not being used).  So, ONLY NOW do we Stop Playback.
                // So, GET ME OUT OF HERE is now "ESC ESC", or "Hit ESC a couple of times".
                //    and, CLEAR SEARCH is just ESC (or click on the Clear Search button).
//                if (currentState == kPlaying) {
                if (cBass->currentStreamState() == BASS_ACTIVE_PLAYING) {
                    on_playButton_clicked();  // we were playing, so PAUSE now.
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
//            qDebug() << "Key RETURN/ENTER detected.";
            if (ui->typeSearch->hasFocus() || ui->labelSearch->hasFocus() || ui->titleSearch->hasFocus() ||
                    ui->songTable->hasFocus()) { // also now allow pressing Return to load, if songTable has focus
//                qDebug() << "   and search OR songTable has focus.";

//                QWidget *lastWidget = QApplication::focusWidget(); // save current focus (destroyed by itemDoubleClicked) because reasons

                // figure out which row is currently selected
                int row = selectedSongRow();
                if (row < 0) {
                    // more than 1 row or no rows at all selected (BAD)
                    return true;
                }

                on_songTable_itemDoubleClicked(ui->songTable->item(row,1)); // note: alters focus

//                lastWidget->setFocus(); // restore focus to widget that had it before
                ui->songTable->setFocus(); // THIS IS BETTER
            }
            break;

        case Qt::Key_Down:
        case Qt::Key_Up:
//            qDebug() << "Key up/down detected.";
            if (ui->typeSearch->hasFocus() || ui->labelSearch->hasFocus() || ui->titleSearch->hasFocus() ||
                    ui->songTable->hasFocus()) {
//                qDebug() << "   and search OR songTable has focus.";
                if (key == Qt::Key_Up) {
                    int row = previousVisibleSongRow();
                    if (row < 0) { 
                        // more than 1 row or no rows at all selected (BAD)
                        return true;
                    }
                    ui->songTable->selectRow(row); // select new row!

                } else {
                    int row = nextVisibleSongRow();
                    if (row < 0) {
                        // more than 1 row or no rows at all selected (BAD)
                        return true;
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
    cBass->StreamGetPosition();  // update the position
    // set the position to one second before the end, so that RIGHT ARROW works as expected
    cBass->StreamSetPosition(static_cast<int>(fmin(cBass->Current_Position + 10.0, cBass->FileLength-1.0)));
    Info_Seekbar(true);
}

void MainWindow::on_actionSkip_Back_15_sec_triggered()
{
    Info_Seekbar(true);
    cBass->StreamGetPosition();  // update the position
    cBass->StreamSetPosition(static_cast<int>(fmax(cBass->Current_Position - 10.0, 0.0)));
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
    cBass->SetEq(0, static_cast<double>(value));
    saveCurrentSongSettings();
}

void MainWindow::on_midrangeSlider_valueChanged(int value)
{
    cBass->SetEq(1, static_cast<double>(value));
    saveCurrentSongSettings();
}

void MainWindow::on_trebleSlider_valueChanged(int value)
{
    cBass->SetEq(2, static_cast<double>(value));
    saveCurrentSongSettings();
}



// Extract TBPM tag from ID3v2 to know (for sure) what the BPM is for a song (overrides beat detection) -----
double MainWindow::getID3BPM(QString MP3FileName) {
    MPEG::File *mp3file;
    ID3v2::Tag *id3v2tag;  // NULL if it doesn't have a tag, otherwise the address of the tag

    mp3file = new MPEG::File(MP3FileName.toStdString().c_str()); // FIX: this leaks on read of another file
    id3v2tag = mp3file->ID3v2Tag(true);  // if it doesn't have one, create one

    double theBPM = 0.0;

    ID3v2::FrameList::ConstIterator it = id3v2tag->frameList().begin();
    for (; it != id3v2tag->frameList().end(); it++)
    {
        if ((*it)->frameID() == "TBPM")  // This is an Apple standard, which means it's everybody's standard now.
        {
            QString BPM((*it)->toString().toCString());
            theBPM = BPM.toDouble();
        }

    }
//    qDebug() << "getID3BPM filename: " << MP3FileName << "BPM: " << theBPM;
    return(theBPM);
}

void MainWindow::reloadCurrentMP3File() {
    // if there is a song loaded, reload it (to pick up, e.g. new cuesheets)
    if ((currentMP3filenameWithPath != "")&&(currentSongTitle != "")&&(currentSongType != "")) {
//        qDebug() << "reloading song: " << currentMP3filename;
        loadMP3File(currentMP3filenameWithPath, currentSongTitle, currentSongType, currentSongLabel);
    }
}

void MainWindow::loadMP3File(QString MP3FileName, QString songTitle, QString songType, QString songLabel)
{
    PerfTimer t("loadMP3File", __LINE__);
    songLoaded = false;  // seekBar updates are disabled, while we are loading

    filewatcherIsTemporarilyDisabled = true;  // disable the FileWatcher for a few seconds to workaround the Ventura extended attribute problem
    fileWatcherDisabledTimer->start(std::chrono::milliseconds(5000)); // re-enable the FileWatcher after 5s
//    qDebug() << "***** filewatcher is disabled for 5 seconds!";

//    qDebug() << "MainWindow::loadMP3File: songTitle: " << songTitle << ", songType: " << songType << ", songLabel: " << songLabel;
//    RecursionGuard recursion_guard(loadingSong);
    loadingSong = true;
    firstTimeSongIsPlayed = true;

    currentMP3filenameWithPath = MP3FileName;

    currentSongType = songType;     // save it for session coloring on the analog clock later...
    currentSongLabel = songLabel;   // remember it, in case we need it later

    loadCuesheets(MP3FileName); // load cuesheets up here first, so that the original pathname is used, rather than the pointed-to (rewritten) pathname.
                                //   A symlink or alias in the Singing folder pointing at the real file in the Patter folder should work now.
                                //   A symlink or alias in the Patter folder pointing at the real file in the Singing folder should also work now.
                                //   In both cases: as Singer, it has lyrics, and as Patter, it has looping and no lyrics.

    // resolve aliases at load time, rather than findFilesRecursively time, because it's MUCH faster
    QFileInfo fi(MP3FileName);
    QString resolvedFilePath = fi.symLinkTarget(); // path with the symbolic links followed/removed
    if (resolvedFilePath != "") {
        MP3FileName = resolvedFilePath;
    }

    t.elapsed(__LINE__);

    ui->pushButtonEditLyrics->setChecked(false); // lyrics/cuesheets of new songs when loaded default to NOT editable
    ui->pushButtonCueSheetEditSave->hide();   // the two save buttons are now invisible
    ui->pushButtonCueSheetEditSaveAs->hide();
    ui->pushButtonEditLyrics->show();  // and the "unlock for editing" button shows up!
    

    QStringList pieces = MP3FileName.split( "/" );
    QString filebase = pieces.value(pieces.length()-1);
    currentMP3filename = filebase;
    int lastDot = currentMP3filename.lastIndexOf('.');
    if (lastDot > 1)
    {
        currentMP3filename = currentMP3filename.right(lastDot - 1);
    }

    if (songTitle != "") {
        setNowPlayingLabelWithColor(songTitle);
    }
    else {
        setNowPlayingLabelWithColor(currentMP3filename);
    }
    currentSongTitle = ui->nowPlayingLabel->text();  // save, in case we are Flash Calling

//    QDir md(MP3FileName);
//    QString canonicalFN = md.canonicalPath();

    t.elapsed(__LINE__);

    // let's do a quick preview (takes <1ms), to see if the intro/outro are already set.
    SongSetting settings1;
    double intro1 = 0.0;
    double outro1 = 0.0;
    if (songSettings.loadSettings(currentMP3filenameWithPath, settings1)) {
        if (settings1.isSetIntroPos()) {
            intro1 = settings1.getIntroPos();
//            qDebug() << "intro was set to: " << intro1;
        }
        if (settings1.isSetOutroPos()) {
            outro1 = settings1.getOutroPos();
//            qDebug() << "outro was set to: " << outro1;
        }
    }

    cBass->StreamCreate(MP3FileName.toStdString().c_str(), &startOfSong_sec, &endOfSong_sec, intro1, outro1);  // load song, and figure out where the song actually starts and ends

    t.elapsed(__LINE__);

    // OK, by this time we always have an introOutro
    //   if DB had one, we didn't scan, and just used that one
    //   if DB did not have one, we scanned

    //    qDebug() << "song starts: " << startOfSong_sec << ", ends: " << endOfSong_sec;

#ifdef REMOVESILENCE
        startOfSong_sec = (startOfSong_sec > 1.0 ? startOfSong_sec - 1.0 : 0.0);  // start 1 sec before non-silence
#endif

    QStringList ss = MP3FileName.split('/');
    QString fn = ss.at(ss.size()-1);
    this->setWindowTitle(fn + QString(" - SquareDesk MP3 Player/Editor"));

    t.elapsed(__LINE__);

    fileModified = false;

    ui->playButton->setEnabled(true);
    ui->stopButton->setEnabled(true);

    ui->actionPlay->setEnabled(true);
    ui->actionStop->setEnabled(true);
    ui->actionSkip_Ahead_15_sec->setEnabled(true);
    ui->actionSkip_Back_15_sec->setEnabled(true);

    ui->seekBar->setEnabled(true);
    ui->seekBarCuesheet->setEnabled(true);

    emit ui->pitchSlider->valueChanged(ui->pitchSlider->value());    // force pitch change, if pitch slider preset before load
    emit ui->volumeSlider->valueChanged(ui->volumeSlider->value());  // force vol change, if vol slider preset before load
    emit ui->mixSlider->valueChanged(ui->mixSlider->value());        // force mix change, if mix slider preset before load

    ui->actionMute->setEnabled(true);
    ui->actionLoop->setEnabled(true);
    ui->actionTest_Loop->setEnabled(true);
    ui->actionIn_Out_Loop_points_to_default->setEnabled(true);

    ui->actionVolume_Down->setEnabled(true);
    ui->actionVolume_Up->setEnabled(true);
    ui->actionSpeed_Up->setEnabled(true);
    ui->actionSlow_Down->setEnabled(true);
    ui->actionForce_Mono_Aahz_mode->setEnabled(true);
    ui->actionPitch_Down->setEnabled(true);
    ui->actionPitch_Up->setEnabled(true);

    emit ui->bassSlider->valueChanged(ui->bassSlider->value()); // force bass change, if bass slider preset before load
    emit ui->midrangeSlider->valueChanged(
        ui->midrangeSlider->value()); // force midrange change, if midrange slider preset before load
    emit ui->trebleSlider->valueChanged(ui->trebleSlider->value()); // force treble change, if treble slider preset before load

    cBass->Stop();

}

void MainWindow::secondHalfOfLoad(QString songTitle) {
    // This function is called when the files is actually loaded into memory, and the filelength is known.
//    qDebug() << "***** secondHalfOfLoad()";

    // We are NOT doing automatic start-of-song finding right now.
    startOfSong_sec = 0.0;
    endOfSong_sec = cBass->FileLength;  // used by setDefaultIntroOutroPositions below

    // song is loaded now, so init the seekbar min/max (once)
    InitializeSeekBar(ui->seekBar);
    InitializeSeekBar(ui->seekBarCuesheet);

    Info_Seekbar(true);  // update the slider and all the text

    ui->dateTimeEditIntroTime->setTime(QTime(0,0,0,0));
    ui->dateTimeEditOutroTime->setTime(QTime(23,59,59,0));

    ui->seekBarCuesheet->SetDefaultIntroOutroPositions(tempoIsBPM, cBass->Stream_BPM, startOfSong_sec, endOfSong_sec, cBass->FileLength);
    ui->seekBar->SetDefaultIntroOutroPositions(tempoIsBPM, cBass->Stream_BPM, startOfSong_sec, endOfSong_sec, cBass->FileLength);

    ui->dateTimeEditIntroTime->setEnabled(true);
    ui->dateTimeEditOutroTime->setEnabled(true);

    ui->pushButtonSetIntroTime->setEnabled(true);  // always enabled now, because anything CAN be looped now OR it has an intro/outro
    ui->pushButtonSetOutroTime->setEnabled(true);
    ui->pushButtonTestLoop->setEnabled(true);

    cBass->SetVolume(100);
    currentVolume = 100;
    previousVolume = 100;
    Info_Volume();

//    qDebug() << "**** NOTE: currentSongTitle = " << currentSongTitle;
    loadSettingsForSong(songTitle); // also loads replayGain, if song has one; also loads tempo from DB (not necessarily same as songTable if playlist loaded)

    loadGlobalSettingsForSong(songTitle); // sets global eq (e.g. Intelligibility Boost), AFTER song is loaded

    // NOTE: this needs to be down here, to override the tempo setting loaded by loadSettingsForSong()
    bool tryToSetInitialBPM = prefsManager.GettryToSetInitialBPM();
    int initialBPM = prefsManager.GetinitialBPM();

//    qDebug() << "tryToSetInitialBPM: " << tryToSetInitialBPM;

    if (tryToSetInitialBPM && tempoIsBPM) {
//        qDebug() << "tryToSetInitialBPM overrides to: " << initialBPM;
        // if the user wants us to try to hit a particular BPM target, use that value
        //  iff the tempo is actually measured in BPM for this song
        ui->tempoSlider->setValue(initialBPM);
        emit ui->tempoSlider->valueChanged(initialBPM);  // fixes bug where second song with same BPM doesn't update songtable::tempo
    } else {
//        qDebug() << "using targetTempo" << targetTempo;
        if (targetTempo != "0" && targetTempo != "0%") {
            // iff tempo is known, then update the table
            QString tempo2 = targetTempo.replace("%",""); // if percentage (not BPM) just get rid of the "%" (setValue knows what to do)
            int tempoInt = tempo2.toInt();
            if (tempoInt != 0)
            {
                ui->tempoSlider->setValue(tempoInt); // set slider to target BPM or percent, as per songTable (overriding DB)
            }
        }
    }

//    qDebug() << "using targetPitch" << targetPitch;
    int pitchInt = targetPitch.toInt();
    ui->pitchSlider->setValue(pitchInt);
    emit ui->pitchSlider->valueChanged(pitchInt); // make sure that the on value changed code gets executed, even if this isn't really a change.

//    qDebug() << "setting stream position to: " << startOfSong_sec;
    cBass->StreamSetPosition(startOfSong_sec);  // last thing we do is move the stream position to 1 sec before start of music

    songLoaded = true;  // now seekBar can be updated
    setInOutButtonState();
    loadingSong = false;

    if (ui->actionAutostart_playback->isChecked()) {
//        qDebug() << "----- AUTO START PRESSING PLAY, BECAUSE SONG IS NOW LOADED";
        on_playButton_clicked();
    }
}

void MainWindow::on_actionOpen_MP3_file_triggered()
{
    on_stopButton_clicked();  // if we're loading a new MP3 file, stop current playback

    saveCurrentSongSettings();

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    QString startingDirectory = prefsManager.Getdefault_dir();

    QString MP3FileName =
        QFileDialog::getOpenFileName(this,
                                     tr("Import Audio File"),
                                     startingDirectory,
                                     tr("Audio Files (*.mp3 *.m4a *.wav)"));
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
    loadMP3File(MP3FileName, QString(""), QString(""), QString(""));  // "" means use title from the filename
}


// this function stores the absolute paths of each file in a QVector
void findFilesRecursively(QDir rootDir, QList<QString> *pathStack, QString suffix, Ui::MainWindow *ui, QMap<int, QString> *soundFXarray, QMap<int, QString> *soundFXname)
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
        if (section[section.length()-1] != "soundfx") {
//            qDebug() << "findFilesRecursively() adding " + type + "#!#" + resolvedFilePath + " to pathStack";
//            pathStack->append(type + "#!#" + resolvedFilePath);

            // add to the pathStack iff it's not a sound FX .mp3 file (those are internal) AND iff it's not sd, choreography, or reference
            if (newType != "sd" && newType != "choreography" && newType != "reference") {
//                qDebug() << "FFR: " << fi.path() << resolvedFilePath << type << newType;
                pathStack->append(newType + "#!#" + resolvedFilePath);
            }
        } else {
            if (suffix != "*") {
                // if it IS a sound FX file, then let's squirrel away the paths so we can play them later
                QString path1 = resolvedFilePath;
                QString baseName = resolvedFilePath.replace(rootDir.absolutePath(),"").replace("/soundfx/","");
                QStringList sections = baseName.split(".");
                if (sections.length() == 3 &&
                    sections[0].toInt() >= 1 &&
                    sections[2].compare("mp3",Qt::CaseInsensitive) == 0) {

                    soundFXname->insert(sections[0].toInt()-1, sections[1]);  // save for populating Preferences dropdown later

                    switch (sections[0].toInt()) {
                        case 1: ui->action_1->setText(sections[1]); break;
                        case 2: ui->action_2->setText(sections[1]); break;
                        case 3: ui->action_3->setText(sections[1]); break;
                        case 4: ui->action_4->setText(sections[1]); break;
                        case 5: ui->action_5->setText(sections[1]); break;
                        case 6: ui->action_6->setText(sections[1]); break;
                        default:
                            // ignore all but the first 6 soundfx
                            break;
                    } // switch

                    soundFXarray->insert(sections[0].toInt()-1, path1);  // remember the path for playing it later
                } // if
            } // if
        } // else
    }
}

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
            // probably another instance of SquareDesk somewhere
            QMessageBox msgBox(QMessageBox::Warning,
                               "TITLE",
                               QString("The SquareDesk database is already being used by '") + hostname + QString("'.")
                               );
            msgBox.setInformativeText("If you continue, any changes might be lost.");
            msgBox.addButton(tr("&Continue anyway"), QMessageBox::AcceptRole);
            msgBox.addButton(tr("&Quit"), QMessageBox::RejectRole);
            theSplash->close();  // if we have a lock problem, let's close the splashscreen before this dialog goes up,
                              //   so splashscreen doesn't cover up the dialog.
            if (msgBox.exec() != QMessageBox::AcceptRole) {
                exit(-1);
            }
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


void MainWindow::findMusic(QString mainRootDir, bool refreshDatabase)
{
    PerfTimer t("findMusic", __LINE__);
    QString databaseDir(mainRootDir + "/.squaredesk");

    if (refreshDatabase)
    {
        songSettings.openDatabase(databaseDir, mainRootDir, false);
    }
    // always gets rid of the old pathstack...
    if (pathStack) {
        delete pathStack;
    }

    // make a new one
    pathStack = new QList<QString>();

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
        // qDebug() << "findMusic():" << cuesheet_file_extensions[i];
        qsl.append(starDot + cuesheet_file_extensions[i]);
    }

    rootDir1.setNameFilters(qsl);

    findFilesRecursively(rootDir1, pathStack, "", ui, &soundFXfilenames, &soundFXname);  // appends to the pathstack
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


bool filterContains(QString str, const QStringList &list)
{
    if (list.isEmpty())
        return true;
//    // Make "it's" and "its" equivalent.
//    str.replace("'","");

    str.replace(spanPrefixRemover, "\\1"); // remove <span style="color:#000000"> and </span> title string coloring

    int title_end = str.indexOf(title_tags_remover); // locate tags
    str.replace(title_tags_remover, " ");

    int index = 0;

    if (title_end < 0) title_end = str.length();
                
    for (const auto &t : list)
    {
        QString filterWord(t);
        bool tagsOnly(false);
        bool exclude(false);

        while (filterWord.length() > 0 &&
               ('#' == filterWord[0] ||
                '-' == filterWord[0]))
        {
            tagsOnly = ('#' == filterWord[0]);
            exclude = ('-' == filterWord[0]);
            filterWord.remove(0,1);
        }
        if (filterWord.length() == 0)
            continue;

        // Keywords can get matched in any order
        if (index > title_end)
            index = title_end;
        int i = str.indexOf(filterWord,
                            tagsOnly ? title_end : (exclude ? 0 : index),
                            Qt::CaseInsensitive);
        if (i < 0)
        {
            if (!exclude)
                return false;
        }
        else
        {
            if (exclude)
                return false;
        }
        if (!tagsOnly && !exclude)
            index = i + t.length();
    }
    return true;
}

// --------------------------------------------------------------------------------
void MainWindow::filterMusic()
{
//    static QRegularExpression rx("(\\ |\\,|\\.|\\:|\\t\\')"); //RegEx for ' ' or ',' or '.' or ':' or '\t', includes ' to handle the "it's" case.
    static QRegularExpression rx("(\\ |\\,|\\.|\\:|\\t)"); //RegEx for ' ' or ',' or '.' or ':' or '\t', does NOT include ' now

    QStringList label = ui->labelSearch->text().split(rx);
    QStringList type = ui->typeSearch->text().split(rx);
    QStringList title = ui->titleSearch->text().split(rx);

//    qDebug() << "filterMusic: title: " << title;
    ui->songTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);  // DO NOT SET height of rows (for now)

    ui->songTable->setSortingEnabled(false);

    int initialRowCount = ui->songTable->rowCount();
    int rowsVisible = initialRowCount;
    int firstVisibleRow = -1;
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QString songTitle = getTitleColText(ui->songTable, i);
        QString songType = ui->songTable->item(i,kTypeCol)->text();
        QString songLabel = ui->songTable->item(i,kLabelCol)->text();

        bool show = true;

        if (!filterContains(songLabel,label)
            || !filterContains(songType, type)
            || !filterContains(songTitle, title))
        {
            show = false;
        }
        ui->songTable->setRowHidden(i, !show);
        rowsVisible -= (show ? 0 : 1); // decrement row count, if hidden
        if (show && firstVisibleRow == -1) {
            firstVisibleRow = i;
        }
    }
    ui->songTable->setSortingEnabled(true);

    if (rowsVisible > 0) {
        ui->songTable->selectRow(firstVisibleRow);
    }

    ui->songTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);  // auto set height of rows
}


QString MainWindow::FormatTitlePlusTags(const QString &title, bool setTags, const QString &strtags, QString titleColor)
{
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
void MainWindow::loadMusicList()
{
//    qDebug() << "LOAD MUSIC LIST()";
//    ui->songTable->clearSelection();  // DEBUG DEBUG

    PerfTimer t("loadMusicList", __LINE__);
    startLongSongTableOperation("loadMusicList");  // for performance, hide and sorting off

    // Need to remember the PL# mapping here, and reapply it after the filter
    // left = path, right = number string
    QMap<QString, QString> path2playlistNum;

// SONGTABLEREFACTOR
    // Iterate over the songTable, saving the mapping in "path2playlistNum"
    // TODO: optimization: save this once, rather than recreating each time.
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();  // this is the playlist #
        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();  // this is the full pathname
        if (playlistIndex != " " && playlistIndex != "") {
            // item HAS an index (that is, it is on the list, and has a place in the ordering)
            // TODO: reconcile int here with double elsewhere on insertion
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
            if (s.endsWith(extension, Qt::CaseInsensitive))
            {
                foundExtension = true;
            }
        }
        if (!foundExtension) {
            continue;
        }

        QStringList sl1 = s.split("#!#");
        QString type = sl1[0];  // the type (of original pathname, before following aliases)
        s = sl1[1];  // everything else
        QString origPath = s;  // for when we double click it later on...

        // double check that type is non-music type (see Issue #298)
        if (type == "reference" || type == "soundfx" || type == "sd") {
            continue;
        }

        QFileInfo fi(s);

        if (fi.canonicalPath() == musicRootPath) {
            type = "";
        }

//        QStringList section = fi.canonicalPath().split("/");
        QString label = "";
        QString labelnum = "";
        QString labelnum_extra = "";
        QString title = "";
        QString shortTitle = "";

        s = fi.completeBaseName(); // e.g. "/Users/mpogue/__squareDanceMusic/patter/RIV 307 - Going to Ceili (Patter).mp3" --> "RIV 307 - Going to Ceili (Patter)"
        breakFilenameIntoParts(s, label, labelnum, labelnum_extra, title, shortTitle);
        labelnum += labelnum_extra;

        ui->songTable->setRowCount(ui->songTable->rowCount()+1);  // make one more row for this line

        QString cType = type.toLower();  // type for Color purposes
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
        newTableItem4->setForeground(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, kNumberCol, newTableItem4);      // add it to column 0

        addStringToLastRowOfSongTable(textCol, ui->songTable, type, kTypeCol);
        addStringToLastRowOfSongTable(textCol, ui->songTable, label + " " + labelnum, kLabelCol );
//        addStringToLastRowOfSongTable(textCol, ui->songTable, title, kTitleCol);
       
        InvisibleTableWidgetItem *titleItem(new InvisibleTableWidgetItem(title));
        ui->songTable->setItem(ui->songTable->rowCount()-1, kTitleCol, titleItem);
        SongSetting settings;
        songSettings.loadSettings(origPath,
                                  settings);
        if (settings.isSetTags())
            songSettings.addTags(settings.getTags());

        // format the title string -----
        QString titlePlusTags(FormatTitlePlusTags(title, settings.isSetTags(), settings.getTags(), textCol.name()));
        SongTitleLabel *titleLabel = new SongTitleLabel(this);
        titleLabel->setTextFormat(Qt::RichText);
        titleLabel->setText(titlePlusTags);
        titleLabel->textColor = textCol.name();  // remember the text color, so we can restore it when deselected

        ui->songTable->setCellWidget(ui->songTable->rowCount()-1, kTitleCol, titleLabel);
        
        QString ageString = songSettings.getSongAge(fi.completeBaseName(), origPath,
                                                    show_all_ages);

        QString ageAsIntString = ageToIntString(ageString);

        addStringToLastRowOfSongTable(textCol, ui->songTable, ageAsIntString, kAgeCol);
        QString recentString = ageToRecent(ageString);  // passed as double string
        addStringToLastRowOfSongTable(textCol, ui->songTable, recentString, kRecentCol);

        int pitch = 0;
        int tempo = 0;
        bool loadedTempoIsPercent(false);

        if (settings.isSetPitch()) { pitch = settings.getPitch(); }
        if (settings.isSetTempo()) { tempo = settings.getTempo(); }
        if (settings.isSetTempoIsPercent()) { loadedTempoIsPercent = settings.getTempoIsPercent(); }

        addStringToLastRowOfSongTable(textCol, ui->songTable,
                                      QString("%1").arg(pitch),
                                      kPitchCol);

        QString tempoStr = QString("%1").arg(tempo);
        if (loadedTempoIsPercent) tempoStr += "%";
//        qDebug() << "loadMusicList() is setting the kTempoCol to: " << tempoStr;
        addStringToLastRowOfSongTable(textCol, ui->songTable,
                                      tempoStr,
                                      kTempoCol);
        // keep the path around, for loading in when we double click on it
        ui->songTable->item(ui->songTable->rowCount()-1, kPathCol)->setData(Qt::UserRole,
                QVariant(origPath)); // path set on cell in col 0

        // Filter out (hide) rows that we're not interested in, based on the search fields...
        //   4 if statements is clearer than a gigantic single if....
//        QString labelPlusNumber = label + " " + labelnum;
    }

    QFont currentFont = ui->songTable->font();
    setSongTableFont(ui->songTable, currentFont);
    filterMusic();

    sortByDefaultSortOrder();
    stopLongSongTableOperation("loadMusicList");  // for performance, sorting on again and show

    QString msg1 = QString::number(ui->songTable->rowCount()) + QString(" audio files found");
    ui->statusBar->showMessage(msg1);

    lastSongTableRowSelected = -1;  // don't modify previous one, just set new selected one to color
    on_songTable_itemSelectionChanged();  // to re-highlight the selection, if music was reloaded while an item was selected
    lastSongTableRowSelected = 0; // first row is highlighted now

    ui->songTable->scrollToItem(ui->songTable->item(0, kNumberCol)); // EnsureVisible row 0 (which is highlighted)

//    qDebug() << "AFTER LOAD MUSIC LIST lastSongTableRowSelected:" << lastSongTableRowSelected;
    ui->titleSearch->setFocus();
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
    QListIterator<QString> iter(*pathStack);
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
        if (!outputDir.exists())
        {
            outputDir.mkpath(".");
        }

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


void MainWindow::titleLabelDoubleClicked(QMouseEvent * /* event */)
{
    int row = selectedSongRow();
    if (row >= 0) {
        on_songTable_itemDoubleClicked(ui->songTable->item(row,kPathCol));
    } else {
        // more than 1 row or no rows at all selected (BAD)
    }
    
}

void MainWindow::on_songTable_itemDoubleClicked(QTableWidgetItem *item)
{
    PerfTimer t("on_songTable_itemDoubleClicked", __LINE__);

    on_stopButton_clicked();  // if we're loading a new MP3 file, stop current playback
    saveCurrentSongSettings();

    t.elapsed(__LINE__);

    int row = item->row();
    QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();

    QString songTitle = getTitleColTitle(ui->songTable, row);
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,kTypeCol)->text().toLower();
    QString songLabel = ui->songTable->item(row,kLabelCol)->text().toLower();

    // these must be up here to get the correct values...
    QString pitch = ui->songTable->item(row,kPitchCol)->text();
    QString tempo = ui->songTable->item(row,kTempoCol)->text();
    QString number = ui->songTable->item(row, kNumberCol)->text();

    targetPitch = pitch;  // save this string, and set pitch slider AFTER base BPM has been figured out
    targetTempo = tempo;  // save this string, and set tempo slider AFTER base BPM has been figured out
    targetNumber = number; // save this, because tempo changes when this is set are playlist modifications, too

//    qDebug() << "on_songTable_itemDoubleClicked: " << songTitle << songType << songLabel << pitch << tempo;

    t.elapsed(__LINE__);

    loadMP3File(pathToMP3, songTitle, songType, songLabel);

    t.elapsed(__LINE__);

    // these must be down here, to set the correct values...
    int pitchInt = pitch.toInt();
    ui->pitchSlider->setValue(pitchInt);

    on_pitchSlider_valueChanged(pitchInt); // manually call this, in case the setValue() line doesn't call valueChanged() when the value set is
                                           //   exactly the same as the previous value.  This will ensure that cBass->setPitch() gets called (right now) on the new stream.

    if (ui->actionAutostart_playback->isChecked()) {
        on_playButton_clicked();
    }

    t.elapsed(__LINE__);
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
    prefsManager.Setautostartplayback(ui->actionAutostart_playback->isChecked());
}

void MainWindow::on_actionImport_triggered()
{
    RecursionGuard dialog_guard(inPreferencesDialog);
    qDebug() << "IMPORT TRIGGERED";
 
    ImportDialog *importDialog = new ImportDialog();
    int dialogCode = importDialog->exec();
    RecursionGuard keypress_guard(trapKeypresses);
    if (dialogCode == QDialog::Accepted)
    {
        importDialog->importSongs(songSettings, pathStack);
        loadMusicList();
    }
    delete importDialog;
    importDialog = nullptr;
}

void MainWindow::on_actionExport_Play_Data_triggered()
{
    RecursionGuard dialog_guard(inPreferencesDialog);

    SongHistoryExportDialog *dialog = new SongHistoryExportDialog();
    dialog->populateOptions(songSettings);
    int dialogCode = dialog->exec();
    RecursionGuard keypress_guard(trapKeypresses);
    if (dialogCode == QDialog::Accepted)
    {
        dialog->exportSongPlayData(songSettings);
    }
    delete dialog;
    dialog = nullptr;
}

void MainWindow::on_actionExport_triggered()
{
    RecursionGuard dialog_guard(inPreferencesDialog);

    if (/* DISABLES CODE */ (true))
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

            // 1) release the lock on the old music directory (if it exists), before we change the music root path
            clearLockFile(oldMusicRootPath);

            // 2) take a lock on the new music directory
            checkLockFile();

            // 3) TODO: clear out the lyrics
            // 4) TODO: clear out the reference tab
            // 5) TODO: load the reference material from the new musicDirectory
        }

        // USER SAID "OK", SO HANDLE THE UPDATED PREFS ---------------
        musicRootPath = prefsManager.GetmusicPath();

        songSettings.setSessionInfo(prefDialog->getSessionInfoList());
        if (previousSessionDefaultType != static_cast<SessionDefaultType>(prefsManager.GetSessionDefault())
            && static_cast<SessionDefaultType>(prefsManager.GetSessionDefault()) == SessionDefaultDOW)
        {
            setCurrentSessionIdReloadSongAgesCheckMenu(songSettings.currentSessionIDByTime());
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

        bool isPatter = songTypeNamesForPatter.contains(currentSongType);
//        qDebug() << "actionPreferences_triggered: " << currentSongType << isPatter;

        ui->tabWidget->setTabText(lyricsTabNumber, CUESHEET_TAB_NAME);

        // -----------------------------------------------------------------------
        // Save the new settings for experimental break and patter timers --------
        tipLengthTimerEnabled = prefsManager.GettipLengthTimerEnabled();  // save new settings in MainWindow, too
        tipLength30secEnabled = prefsManager.GettipLength30secEnabled();
        tipLengthTimerLength = prefsManager.GettipLengthTimerLength();
        tipLengthAlarmAction = prefsManager.GettipLengthAlarmAction();
//        qDebug() << "Saving tipLengthAlarmAction:" << tipLengthAlarmAction;

        breakLengthTimerEnabled = prefsManager.GetbreakLengthTimerEnabled();
        breakLengthTimerLength = prefsManager.GetbreakLengthTimerLength();
        breakLengthAlarmAction = prefsManager.GetbreakLengthAlarmAction();
//        qDebug() << "Saving breakLengthAlarmAction:" << breakLengthAlarmAction;

        // and tell the clock, too.
        analogClock->tipLengthTimerEnabled = tipLengthTimerEnabled;
        analogClock->tipLength30secEnabled = tipLength30secEnabled;
        analogClock->tipLengthAlarmMinutes = tipLengthTimerLength;

        analogClock->breakLengthTimerEnabled = breakLengthTimerEnabled;
        analogClock->breakLengthAlarmMinutes = breakLengthTimerLength;

        // ----------------------------------------------------------------
        // Save the new value for experimentalClockColoringEnabled --------
        clockColoringHidden = !prefsManager.GetexperimentalClockColoringEnabled();
        analogClock->setHidden(clockColoringHidden);

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
            loadMusicList();
        }

        if (prefsManager.GetenableAutoAirplaneMode()) {
            // if the user JUST set the preference, turn Airplane Mode on RIGHT NOW (radios OFF).
            airplaneMode(true);
        } else {
            // if the user JUST set the preference, turn Airplane Mode OFF RIGHT NOW (radios ON).
            airplaneMode(false);
        }
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


void MainWindow::on_songTable_itemSelectionChanged()
{
    // When item selection is changed, enable Next/Previous song buttons,
    //   if at least one item in the table is selected.
    //
    // figure out which row is currently selected
    int row = selectedSongRow();
    if (row >= 0) {
        ui->nextSongButton->setEnabled(true);
        ui->previousSongButton->setEnabled(true);
    } else {
        ui->nextSongButton->setEnabled(false);
        ui->previousSongButton->setEnabled(false);
    }

    // ----------------
    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow != -1) {
        // we've clicked on a real row, which needs its Title text to be highlighted

        // first: let's un-highlight the previous SongTitleLabel back to its original color
        if (lastSongTableRowSelected != -1) {
//            qDebug() << "lastSongTableRowSelected == -1";
            QString origColor = dynamic_cast<const SongTitleLabel*>(ui->songTable->cellWidget(lastSongTableRowSelected, kTitleCol))->textColor;

            QString currentText = dynamic_cast<QLabel*>(ui->songTable->cellWidget(lastSongTableRowSelected, kTitleCol))->text();
//            qDebug() << "CURRENT TEXT OF LASTSONGTABLEROWSELECTED: " << currentText;
            currentText.replace("\"color: white;\"", QString("\"color: %1;\"").arg(origColor));
//            qDebug() << "NEW TEXT OF LASTSONGTABLEROWSELECTED: " << currentText;
            dynamic_cast<QLabel*>(ui->songTable->cellWidget(lastSongTableRowSelected, kTitleCol))->setText(currentText);
        }

        // second: let's highlight the current select row's Title
        QString currentText = dynamic_cast<QLabel*>(ui->songTable->cellWidget(selectedRow, kTitleCol))->text();
//        qDebug() << "CURRENT TEXT: " << currentText;
        currentText.replace(QRegularExpression("\"color: #[0-9a-z]+;\"",
                                                QRegularExpression::InvertedGreedinessOption), "\"color: white;\"");
//        qDebug() << "NEW TEXT: " << currentText;
        dynamic_cast<QLabel*>(ui->songTable->cellWidget(selectedRow, kTitleCol))->setText(currentText);

        // finally:
        lastSongTableRowSelected = selectedRow;  // remember that THIS row is now highlighted
    }

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

        // this function is always called when playlist items are added or deleted, so
        // figure out whether save/save as are enabled here
        linesInCurrentPlaylist = playlistItemCount;
//        qDebug() << "songTableItemSelectionChanged:" << playlistItemCount;
        if ((playlistItemCount > 0) && (lastSavedPlaylist != "")) {
            ui->actionSave->setEnabled(true);
            QString basefilename = lastSavedPlaylist.section("/",-1,-1).replace(".csv", "");
            ui->actionSave->setText(QString("Save Playlist") + " '" + basefilename + "'"); // and now it has a name
            ui->actionSave_As->setEnabled(true);
        }

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

// Return the previous visible song row if just one selected, else -1
int MainWindow::previousVisibleSongRow()
{
    int row = selectedSongRow();
    if (row < 0) {
        // more than 1 row or no rows at all selected (BAD)
        return row;
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
    return row;
}

// Return the next visible song row if just one selected, else -1
int MainWindow::nextVisibleSongRow() {
    int row = selectedSongRow();
    if (row < 0) {
        return row;
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
    
    return row;
}

// END OF PLAYLIST SECTION ================

void MainWindow::on_songTable_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos)
    QStringList currentTags;

    if (ui->songTable->selectionModel()->hasSelection()) {
        QMenu menu(this);

        int selectedRow = selectedSongRow();  // get current row or -1

        if (selectedRow != -1) {
            // if a single row was selected
            QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
            int currentNumberInt = currentNumberText.toInt();
            int playlistItemCount = PlaylistItemCount();
            
            QString pathToMP3 = ui->songTable->item(selectedRow,kPathCol)->data(Qt::UserRole).toString();
            SongSetting settings;
            songSettings.loadSettings(pathToMP3, settings);
            if (settings.isSetTags())
            {
                currentTags = settings.getTags().split(" ");
            }
            

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
        menu.addAction( "Reveal Audio File in Finder" , this , SLOT (revealInFinder()) );
        menu.addAction( "Reveal Current Cuesheet in Finder" , this , SLOT (revealAttachedLyricsFileInFinder()) );
#endif

#if defined(Q_OS_WIN)
        menu.addAction ( "Show in Explorer" , this , SLOT (revealInFinder()) );
#endif

#if defined(Q_OS_LINUX)
        menu.addAction ( "Open containing folder" , this , SLOT (revealInFinder()) );
#endif
        menu.addSeparator();
        menu.addAction( "Edit Tags...", this, SLOT (editTags()) );

        QMenu *tagsMenu(new QMenu("Tags"));
        QHash<QString,QPair<QString,QString>> tagColors(songSettings.getTagColors());

        QStringList tags(tagColors.keys());
        tags.sort(Qt::CaseInsensitive);

        for (const auto &tagUntrimmed : tags)
        {
            QString tag(tagUntrimmed.trimmed());

            if (tag.length() <= 0)
                continue;
            
            bool set = false;
            for (const auto &t : currentTags)
            {
                if (t.compare(tag, Qt::CaseInsensitive) == 0)
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
                        this->changeTagOnCurrentSongSelection(tag, !set);
                    });
            tagsMenu->addAction(action);
        }

        menu.addMenu(tagsMenu);
//        menu.addAction( "Load Song", this, SLOT (loadSong()) );
        menu.popup(QCursor::pos());
        menu.exec();
    }
}

void MainWindow::changeTagOnCurrentSongSelection(QString tag, bool add)
{
    int row = selectedSongRow();
    if (row >= 0) {
        // exactly 1 row was selected (good)
        QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();
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

        if (add && !tags.contains(tag, Qt::CaseInsensitive))
            tags.append(tag);
        if (!add)
        {
            int i = tags.indexOf(tag, Qt::CaseInsensitive);
            if (i >= 0)
                tags.removeAt(i);
        }
        settings.setTags(tags.join(" "));
        songSettings.saveSettings(pathToMP3, settings);
        QString title = getTitleColTitle(ui->songTable, row);

        // we know for sure that this item is selected (because that's how we got here), so let's highlight text color accordingly
        QString titlePlusTags(FormatTitlePlusTags(title, settings.isSetTags(), settings.getTags(), "white"));

        dynamic_cast<QLabel*>(ui->songTable->cellWidget(row,kTitleCol))->setText(titlePlusTags);
    }
}

void MainWindow::editTags()
{
    int row = selectedSongRow();
    if (row >= 0) {
        // exactly 1 row was selected (good)
        QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();
        SongSetting settings;
        songSettings.loadSettings(pathToMP3, settings);
        QString tags;
        bool ok(false);
        QString title = getTitleColTitle(ui->songTable, row);
        if (settings.isSetTags())
            tags = settings.getTags();
        QString newtags(QInputDialog::getText(this, "Edit Tags", "Tags for " + title, QLineEdit::Normal, tags, &ok));
        if (ok)
        {
            songSettings.removeTags(tags);
            settings.setTags(newtags);
            songSettings.saveSettings(pathToMP3, settings);
            songSettings.addTags(newtags);

            // we know for sure that this item is selected (because that's how we got here), so let's highlight text color accordingly
            QString titlePlusTags(FormatTitlePlusTags(title, settings.isSetTags(), settings.getTags(), "white"));

            dynamic_cast<QLabel*>(ui->songTable->cellWidget(row,kTitleCol))->setText(titlePlusTags);
        }
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
    }
}

void MainWindow::loadSong()
{
    int row = selectedSongRow();
    if (row >= 0) {
        // exactly 1 row was selected (good)
        on_songTable_itemDoubleClicked(ui->songTable->item(row,kPathCol));
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
    }
}

void MainWindow::revealInFinder()
{
    int row = selectedSongRow();
    if (row >= 0) {
        // exactly 1 row was selected (good)
        QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();
        showInFinderOrExplorer(pathToMP3);
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
    }
}

void MainWindow::columnHeaderSorted(int logicalIndex, Qt::SortOrder order)
{
    Q_UNUSED(logicalIndex)
    Q_UNUSED(order)
    adjustFontSizes();  // when sort triangle is added, columns need to adjust width a bit...
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
//            y1 = ui->typeSearch->y();
            w1 = ui->typeSearch->width();
//            h1 = ui->typeSearch->height();

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
    if (loadingSong) {
//        qDebug() << "***** WARNING: MainWindow::saveCurrentSongSettings tried to save while loadingSong was true";
        return;
    }
//    qDebug() << "MainWindow::saveCurrentSongSettings trying to save settings...";
    QString currentSong = ui->nowPlayingLabel->text();

    if (!currentSong.isEmpty()) {
        int pitch = ui->pitchSlider->value();
        int tempo = ui->tempoSlider->value();
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

        setting.setTreble( ui->trebleSlider->value() );
        setting.setBass( ui->bassSlider->value() );
        setting.setMidrange( ui->midrangeSlider->value() );
        setting.setMix( ui->mixSlider->value() );

//        setting.setReplayGain();  // TODO:

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

void MainWindow::loadSettingsForSong(QString songTitle)
{
    int pitch = ui->pitchSlider->value();
    int tempo = ui->tempoSlider->value();  // qDebug() << "loadSettingsForSong, current tempo slider: " << tempo;
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
        if (settings.isSetTempo()) { tempo = settings.getTempo(); /*qDebug() << "loadSettingsForSong: " << tempo;*/ }
        if (settings.isSetVolume()) { volume = settings.getVolume(); }
        if (settings.isSetIntroPos()) { intro = settings.getIntroPos(); }
        if (settings.isSetOutroPos()) { outro = settings.getOutroPos(); }
        if (settings.isSetCuesheetName()) { cuesheetName = settings.getCuesheetName(); } // ADDED *****

        // double length = (double)(ui->seekBarCuesheet->maximum());  // This is not correct, results in non-round-tripping
        double length = cBass->FileLength;  // This seems to work better, and round-tripping looks like it is working now.
        if (settings.isSetIntroOutroIsTimeBased() && settings.getIntroOutroIsTimeBased())
        {
            intro = intro / length;
            outro = outro / length;
        }

        // setup Markers for this song in the seekBar ----------------------------------
        // ui->seekBar->AddMarker(20.0/length);  // stored as fraction of the song length  // DEBUG DEBUG DEBUG, 20 sec for now
        // TODO: key to toggle a marker at (OR NEAR WITHIN A THRESHOLD) the current song position
        // TODO: repurpose |<< and >>| to jump to the next/previous marker (and if at beginning, go to prev song, at end, go to next song)
        // TODO: load from/store to DB for this song
        // -----------------------------------------------------------------------------

        ui->pitchSlider->setValue(pitch);
        ui->tempoSlider->setValue(tempo); // qDebug() << "loadSettingsForSong: just set tempo slider to: " << tempo;
        ui->volumeSlider->setValue(volume);
        ui->seekBarCuesheet->SetIntro(intro);
//        qDebug() << "MainWindow::loadSettingsForSong 1: outro,intro = " << outro << intro;
        ui->seekBarCuesheet->SetOutro(outro);

        QTime iTime = QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*intro*length+0.5));
        QTime oTime = QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*outro*length+0.5));
//        qDebug() << "MainWindow::loadSettingsForSong 2: intro,outro,length = " << iTime << ", " << oTime << "," << length;
        ui->dateTimeEditIntroTime->setTime(iTime); // milliseconds
        ui->dateTimeEditOutroTime->setTime(oTime);

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
        ui->trebleSlider->setValue(0);
        ui->bassSlider->setValue(0);
        ui->midrangeSlider->setValue(0);
        ui->mixSlider->setValue(0);
    }
}

void MainWindow::loadGlobalSettingsForSong(QString songTitle) {
   Q_UNUSED(songTitle)
//   qDebug() << "loadGlobalSettingsForSong";
   cBass->SetGlobals();
}

// ------------------------------------------------------------------------------------------
QString MainWindow::loadLyrics(QString MP3FileName)
{
    QString USLTlyrics;

    MPEG::File mp3file(MP3FileName.toStdString().c_str());
    ID3v2::Tag *id3v2tag = mp3file.ID3v2Tag(true);

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

            ID3v2::UnsynchronizedLyricsFrame* usltFrame = dynamic_cast<ID3v2::UnsynchronizedLyricsFrame*>(*it);
            USLTlyrics = usltFrame->text().toCString();
        }
    }
//    qDebug() << "Got lyrics:" << USLTlyrics;
    return (USLTlyrics);
}

// ------------------------------------------------------------------------
void MainWindow::on_warningLabel_clicked() {
    analogClock->resetPatter();
}

void MainWindow::on_warningLabelCuesheet_clicked() {
    // this one is clickable, too!
    on_warningLabel_clicked();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (ui->tabWidget->tabText(index) == "SD"
        || ui->tabWidget->tabText(index) == "SD 2") {
        // SD Tab ---------------

        ui->lineEditSDInput->setFocus();
//        ui->actionSave_Lyrics->setDisabled(true);
//        ui->actionSave_Lyrics_As->setDisabled(true);
//        ui->actionPrint_Lyrics->setDisabled(true);

        ui->actionFilePrint->setEnabled(true); // FIX: when sequences can be printed
        ui->actionFilePrint->setText("Print SD Sequence...");

        ui->actionSave->setDisabled(true);      // sequences can't be saved (no default filename to save to)
        ui->actionSave->setText("Save SD Sequence");        // greyed out
        ui->actionSave_As->setDisabled(false);  // sequences can be saved
        ui->actionSave_As->setText("Save SD Sequence As...");
    } else if (ui->tabWidget->tabText(index) == CUESHEET_TAB_NAME) {
        // Lyrics Tab ---------------
//        ui->actionPrint_Lyrics->setDisabled(false);
//        ui->actionSave_Lyrics->setDisabled(false);      // TODO: enable only when we've made changes
//        ui->actionSave_Lyrics_As->setDisabled(false);   // always enabled, because we can always save as a different name
        ui->actionFilePrint->setDisabled(false);

        // THIS IS WRONG: enabled iff hasLyrics and editing is enabled
        bool okToSave = hasLyrics && ui->pushButtonEditLyrics->isChecked();
        ui->actionSave->setEnabled(okToSave);      // cuesheet can be saved when there are lyrics to save
        ui->actionSave_As->setEnabled(okToSave);   // cuesheet can be saved as when there are lyrics to save


        ui->actionSave->setText("Save Cuesheet"); // but greyed out, until modified
        ui->actionSave_As->setText("Save Cuesheet As...");  // greyed out until modified
        
        ui->actionFilePrint->setText("Print Cuesheet...");

    } else if (ui->tabWidget->tabText(index) == "Music Player") {
        bool playlistModified = ui->statusBar->currentMessage().endsWith("*");
//        qDebug() << "tabWidget" << linesInCurrentPlaylist << lastSavedPlaylist << playlistModified;

        ui->actionSave->setEnabled((linesInCurrentPlaylist != 0) && (lastSavedPlaylist != "") && playlistModified);      // playlist can be saved if there are >0 lines and it was not current.m3u
        if (lastSavedPlaylist != "") {
            QString basefilename = lastSavedPlaylist.section("/",-1,-1).replace(".csv", "");
            ui->actionSave->setText(QString("Save Playlist") + " '" + basefilename + "'"); // it has a name
        } else {
            ui->actionSave->setText(QString("Save Playlist")); // it doesn't have a name yet
        }
//        qDebug() << "linesInCurrentPlaylist: " << linesInCurrentPlaylist;
        ui->actionSave_As->setEnabled(linesInCurrentPlaylist != 0);  // playlist can be saved as if there are >0 lines
        ui->actionSave_As->setText("Save Playlist As...");  // greyed out until modified

        ui->actionFilePrint->setEnabled(true);
        ui->actionFilePrint->setText("Print Playlist...");
    } else  {
        // dance programs, reference
//        ui->actionPrint_Lyrics->setDisabled(true);
//        ui->actionSave_Lyrics->setDisabled(true);
//        ui->actionSave_Lyrics_As->setDisabled(true);

        ui->actionSave->setDisabled(true);      // dance programs etc can't be saved
        ui->actionSave->setText("Save"); // but greyed out
        ui->actionSave_As->setDisabled(true);  // patter can be saved as
        ui->actionSave_As->setText("Save As...");  // greyed out

        ui->actionFilePrint->setDisabled(true);
    }

    microphoneStatusUpdate();
}

void MainWindow::microphoneStatusUpdate() {
//    bool killVoiceInput(true);

//    int index = ui->tabWidget->currentIndex();

//    QString micsON("MICS ON (Voice: " + currentSDVUILevel + ", Kybd: " + currentSDKeyboardLevel + ")");
//    QString micsOFF("MICS OFF (Voice: " + currentSDVUILevel + ", Kybd: " + currentSDKeyboardLevel + ")");

//    QString kybdStatus("Audio: " + lastAudioDeviceName + ", SD Level: " + currentSDKeyboardLevel + ", Dance: " + frameName);
    QString kybdStatus("SD (Level: " + currentSDKeyboardLevel + ", Dance: " + frameName + "), Audio: " + lastAudioDeviceName);
    micStatusLabel->setStyleSheet("color: black");
    micStatusLabel->setText(kybdStatus);

}

// ---------------------------------------------
void MainWindow::initReftab() {
    static QRegularExpression re1("reference/0[0-9][0-9]\\.[a-zA-Z0-9' ]+\\.txt$", QRegularExpression::CaseInsensitiveOption);

    numWebviews = 0;
    
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
                webview[numWebviews] = new QWebEngineView();
//                QString indexFileURL = "file://" + filename + whichHTM;
#if defined(Q_OS_WIN32)
                QString indexFileURL = "file:///" + filename + whichHTM;  // Yes, Windows is special
#else
                QString indexFileURL = "file://" + filename + whichHTM;   // UNIX-like OS's only need one slash.
#endif
//                qDebug() << "    indexFileURL:" << indexFileURL;
                webview[numWebviews]->setUrl(QUrl(indexFileURL));
                webview[numWebviews]->page()->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);  // allow index.html to access 3P remote website

                documentsTab->addTab(webview[numWebviews], tabname);
                numWebviews++;
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

                webview[numWebviews] = new QWebEngineView();

#if defined(Q_OS_WIN32)
                QString indexFileURL = "file:///" + filename;  // Yes, Windows is special: file:///Y:/Dropbox/__squareDanceMusic/reference/100.Broken Hearts.txt
#else
                QString indexFileURL = "file://" + filename;   // UNIX-like OS's only need one slash.
#endif
//                qDebug() << "    indexFileURL:" << indexFileURL;
                webview[numWebviews]->setUrl(QUrl(indexFileURL));
                documentsTab->addTab(webview[numWebviews], tabname);
                numWebviews++;
        } else if (filename.endsWith(".md", Qt::CaseInsensitive)) {  // is not a Dance Program file in /reference
                static QRegularExpression re4(".[Mm][Dd]$");
                tabname = filename.split("/").last().remove(re4);
                webview[numWebviews] = new QWebEngineView();

                QFile f1(filename);
                f1.open(QIODevice::ReadOnly | QIODevice::Text);
                QTextStream in(&f1);
                QString html = txtToHTMLlyrics(in.readAll(), filename);
                webview[numWebviews]->setHtml(html);
                webview[numWebviews]->setZoomFactor(1.5);
                documentsTab->addTab(webview[numWebviews], tabname);
                numWebviews++;
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

                webview[numWebviews] = new QWebEngineView();
                QWebChannel * channel = new QWebChannel(this);
                channel->registerObject(QStringLiteral("communicator"), m_communicator);
                webview[numWebviews]->page()->setWebChannel(channel);

                webview[numWebviews]->load(url);


//                QString indexFileURL = "file://" + filename;
//                qDebug() << "    indexFileURL:" << indexFileURL;

//                webview[numWebviews]->setUrl(QUrl(pdf_path));
//                QFileInfo fInfo(filename);
                static QRegularExpression re5(".pdf$");
                tabname = filename.split("/").last().remove(re5);      // get basename, remove .pdf
                static QRegularExpression re6("^1[0-9][0-9]\\.");
                tabname.remove(re6);                         // 122.bar.txt -> "bar"  // remove optional 1##. at front
                documentsTab->addTab(webview[numWebviews], tabname);
                numWebviews++;
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
        filterMusic(); // and filter them into the songTable
//        qDebug() << "LOAD MUSIC LIST FROM STARTUP WIZARD TRIGGERED";
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

//        // set Names as current tab
//        ui->tabWidgetSDMenuOptions->setCurrentIndex(3);
//        ui->actionNames->setChecked(true);
//        setSDCoupleNumberingScheme("Names");

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
        msgBox.setText("<H2>You are ahead of the latest Beta version.</H2>\nYour version of SquareDesk: " + QString(VERSIONSTRING) +
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

        // if the reference directory doesn't exist, create it (always, automatically)
        QDir dir(referenceDir);
        if (!dir.exists()) {
            dir.mkpath(".");  // make it
        }

        // ---------------
        // and populate it with the SD doc, if it didn't exist (somewhere) already
        bool hasSDpdf = false;
        QDirIterator it(referenceDir);
        while(it.hasNext()) {
            QString s2 = it.next();
            QString s1 = it.fileName();
            // if 123.SD.pdf or SD.pdf, then do NOT copy one in as 195.SD.pdf
            static QRegularExpression re10("^[0-9]+\\.SD.pdf");
            static QRegularExpression re11("^SD.pdf");
            if (s1.contains(re10) || s1.contains(re11)) {
               hasSDpdf = true;
            }
        }

        if (!hasSDpdf) {
            QString source = QCoreApplication::applicationDirPath() + pathFromAppDirPathToResources + "/sd_doc.pdf";
            QString destination = referenceDir + "/195.SD.pdf";
            QFile::copy(source, destination);
        }

        // ---------------
        // and populate it with the SquareDesk doc, if it didn't exist (somewhere) already
        bool hasSDESKpdf = false;
        QDirIterator it2(referenceDir);
        while(it2.hasNext()) {
            QString s2 = it2.next();
            QString s1 = it2.fileName();
            // if 123.SDESK.pdf or SDESK.pdf, then do NOT copy one in as 190.SDESK.pdf
            static QRegularExpression re12("^[0-9]+\\.SDESK.pdf");
            static QRegularExpression re13("^SDESK.pdf");
            if (s1.contains(re12) || s1.contains(re13)) {
               hasSDESKpdf = true;
            }
        }

        if (!hasSDESKpdf) {
            QString source = QCoreApplication::applicationDirPath() + pathFromAppDirPathToResources + "/squaredesk.pdf";
            QString destination = referenceDir + "/190.SDESK.pdf";
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


void MainWindow::on_actionViewTags_toggled(bool /* checked */)
{
//    qDebug() << "VIEW TAGS TOGGLED";
    prefsManager.SetshowSongTags(ui->actionViewTags->isChecked());
    loadMusicList();
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
//    qDebug() << "startLongSongTableOperation: " << s;

    longSongTableOperationCount++;

    ui->songTable->hide();
    ui->songTable->setSortingEnabled(false);
    ui->songTable->blockSignals(true);  // block signals, so changes are not recursive
}

void MainWindow::stopLongSongTableOperation(QString s) {
    Q_UNUSED(s)
//    qDebug() << "stopLongSongTableOperation: " << s;

    if (--longSongTableOperationCount == 0) {
//        qDebug() << "unblocking all.";
        ui->songTable->blockSignals(false);  // unblock signals
        ui->songTable->setSortingEnabled(true);
        ui->songTable->show();
    }
}


QString MainWindow::filepath2SongType(QString MP3Filename)
{
    // returns the type (as a string).  patter, hoedown -> "patter", as per user prefs

    MP3Filename.replace(QRegularExpression("^" + musicRootPath),"");  // delete the <path to musicDir> from the front of the pathname
    QStringList parts = MP3Filename.split("/");

    if (parts.length() <= 1) {
        return("unknown");
    } else {
        QString folderTypename = parts[1];
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
    // this is the only tab that has printable content right now
    // this is only a double-check (the menu item should be disabled when not on Lyrics/Patter
    QPrinter printer;
    QPrintDialog printDialog(&printer, this);

    int i = ui->tabWidget->currentIndex();
    if (ui->tabWidget->tabText(i) == CUESHEET_TAB_NAME) {
        // PRINT CUESHEET FOR PATTER OR SINGER -------------------------------------------------------
        if (currentSongType == "singing") {
            printDialog.setWindowTitle("Print Cuesheet");
        } else {
            printDialog.setWindowTitle("Print Patter");
        }

        if (printDialog.exec() == QDialog::Rejected) {
            return;
        }

        ui->textBrowserCueSheet->print(&printer);
    } else if (ui->tabWidget->tabText(i).endsWith("SD")) {
        // PRINT SD SEQUENCE -------------------------------------------------------
        QPrinter printer;
        QPrintDialog printDialog(&printer, this);
        printDialog.setWindowTitle("Print SD Sequence");

        if (printDialog.exec() == QDialog::Rejected) {
            return;
        }

        QTextDocument doc;
        QSizeF paperSize;
        paperSize.setWidth(printer.width());
        paperSize.setHeight(printer.height());
        doc.setPageSize(paperSize);
        
        QString contents(get_current_sd_sequence_as_html(true, true));
        doc.setHtml(contents);
        doc.print(&printer);
    } else if (ui->tabWidget->tabText(i).endsWith("Music Player")) {
        // PRINT PLAYLIST -------------------------------------------------------
        QPrinter printer;
        QPrintDialog printDialog(&printer, this);
        printDialog.setWindowTitle("Print Playlist");

        if (printDialog.exec() == QDialog::Rejected) {
            return;
        }

        // --------
        QList<PlaylistExportRecord> exports;

        // Iterate over the songTable to get all the info about the playlist
        for (int i=0; i<ui->songTable->rowCount(); i++) {
            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
            QString playlistIndex = theItem->text();
            QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
//            QString songTitle = getTitleColTitle(ui->songTable, i);
            QString pitch = ui->songTable->item(i,kPitchCol)->text();
            QString tempo = ui->songTable->item(i,kTempoCol)->text();

            if (playlistIndex != "") {
                // item HAS an index (that is, it is on the list, and has a place in the ordering)
                // TODO: reconcile int here with double elsewhere on insertion
                PlaylistExportRecord rec;
                rec.index = playlistIndex.toInt();
    //            rec.title = songTitle;
                rec.title = pathToMP3;  // NOTE: this is an absolute path that does not survive moving musicDir
                rec.pitch = pitch;
                rec.tempo = tempo;
                exports.append(rec);
            }
        }

        std::sort(exports.begin(), exports.end(), comparePlaylistExportRecord);  // they are now IN INDEX ORDER

        // GET SHORT NAME FOR PLAYLIST ---------
        static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
        QRegularExpressionMatch match = regex1.match(lastSavedPlaylist);
        QString shortPlaylistName("ERROR 7");

        if (match.hasMatch())
        {
            shortPlaylistName = match.captured(1);
        }

        QString toBePrinted = "<h2>PLAYLIST: " + shortPlaylistName + "</h2>\n"; // TITLE AT TOP OF PAGE

        QTextDocument doc;
        QSizeF paperSize;
        paperSize.setWidth(printer.width());
        paperSize.setHeight(printer.height());
        doc.setPageSize(paperSize);

        patterColorString = prefsManager.GetpatterColorString();
        singingColorString = prefsManager.GetsingingColorString();
        calledColorString = prefsManager.GetcalledColorString();
        extrasColorString = prefsManager.GetextrasColorString();

        char buf[256];
        foreach (const PlaylistExportRecord &rec, exports)
        {
            QString baseName = rec.title;
            baseName.replace(QRegularExpression("^" + musicRootPath),"");  // delete musicRootPath at beginning of string

            snprintf(buf, sizeof(buf), "%02d: %s\n", rec.index, baseName.toLatin1().data());

            QStringList parts = baseName.split("/");
            QString folderTypename = parts[1]; // /patter/foo.mp3 --> "patter"
            QString colorString("black");
            if (songTypeNamesForPatter.contains(folderTypename)) {
                colorString = patterColorString;
            } else if (songTypeNamesForSinging.contains(folderTypename)) {
                colorString = singingColorString;
            } else if (songTypeNamesForCalled.contains(folderTypename)) {
                colorString = calledColorString;
            } else if (songTypeNamesForExtras.contains(folderTypename)) {
                colorString = extrasColorString;
            }
            toBePrinted += "<span style=\"color:" + colorString + "\">" + QString(buf) + "</span><BR>\n";
        }

//        qDebug() << "PRINT: " << toBePrinted;

        doc.setHtml(toBePrinted);
        doc.print(&printer);

    }
}

void MainWindow::on_actionSave_triggered()
{
//    qDebug() << "actionSave";
    int i = ui->tabWidget->currentIndex();
    if (ui->tabWidget->tabText(i).endsWith("Music Player")) {
        // playlist
        savePlaylistAgain(); // Now a true SAVE (if one was already saved)
    } else if (ui->tabWidget->tabText(i) == CUESHEET_TAB_NAME) {
        // lyrics/patter
        saveLyrics();
    } else if (ui->tabWidget->tabText(i).endsWith("SD")) {
        // sequence
        saveSequenceAs();  // intentionally ..As()
    } else {
        // dance program
        // reference
        // timers
        // intentionally nothing...
    }
}

void MainWindow::on_actionSave_As_triggered()
{
//    qDebug() << "actionSave_As";
    int i = ui->tabWidget->currentIndex();
    if (ui->tabWidget->tabText(i).endsWith("Music Player")) {
        // playlist
        on_actionSave_Playlist_triggered(); // really "Save As..."
    } else if (ui->tabWidget->tabText(i) == CUESHEET_TAB_NAME) {
        // lyrics/patter
        saveLyricsAs();
    } else if (ui->tabWidget->tabText(i).endsWith("SD")) {
        // sequence
        saveSequenceAs();  // intentionally ..As()
    } else {
        // dance program
        // reference
        // timers
        // intentionally nothing...
    }
}



QList<QString> MainWindow::getListOfMusicFiles()
{
    QList<QString> list;

    QListIterator<QString> iter(*pathStack);
    while (iter.hasNext()) {
        QString s = iter.next();
        QStringList sl1 = s.split("#!#");
        QString type = sl1[0];      // the type (of original pathname, before following aliases)
        QString filename = sl1[1];  // everything else

        // TODO: should we allow "patter" to match cuesheets?
        if ((type == "singing" || type == "vocals") && (filename.endsWith("mp3", Qt::CaseInsensitive) ||
                                                        filename.endsWith("m4a", Qt::CaseInsensitive) ||
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

    double songLength = cBass->FileLength;
//    double intro = ui->seekBar->GetIntro(); // 0.0 - 1.0
    double outro = ui->seekBar->GetOutro(); // 0.0 - 1.0

    double startPosition_sec = fmax(0.0, songLength*outro - 5.0);

    cBass->StreamSetPosition(startPosition_sec);
    Info_Seekbar(false);  // update just the text

//    on_playButton_clicked();  // play, starting 5 seconds before the loop

    cBass->Play();  // currently paused, so start playing at new location
}

void MainWindow::on_dateTimeEditIntroTime_timeChanged(const QTime &time)
{
//    qDebug() << "newIntroTime: " << time;

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
    ui->dateTimeEditIntroTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*position_sec+0.5))); // milliseconds

    // set in fractional form
    double frac = position_sec/length;
    ui->seekBarCuesheet->SetIntro(frac);  // after the events are done, do this.
    ui->seekBar->SetIntro(frac);

    on_loopButton_toggled(ui->actionLoop->isChecked()); // then finally do this, so that cBass is told what the loop points are (or they are cleared)
    saveCurrentSongSettings();
}

void MainWindow::on_dateTimeEditOutroTime_timeChanged(const QTime &time)
{
//    qDebug() << "newOutroTime: " << time;

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
//    qDebug() << "dateTimeEditOutro changed: " << currentIntroTimeSec << "," << position_sec;
    ui->dateTimeEditOutroTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*position_sec+0.5))); // milliseconds

    // set in fractional form
    double frac = position_sec/length;
    ui->seekBarCuesheet->SetOutro(frac);  // after the events are done, do this.
    ui->seekBar->SetOutro(frac);

    on_loopButton_toggled(ui->actionLoop->isChecked()); // then finally do this, so that cBass is told what the loop points are (or they are cleared)
    saveCurrentSongSettings();
}

void MainWindow::on_pushButtonTestLoop_clicked()
{
    on_actionTest_Loop_triggered();
}

void MainWindow::on_actionClear_Recent_triggered()
{
    QString nowISO8601 = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);  // add days is for later
//    qDebug() << "Setting fence to: " << nowISO8601;

    prefsManager.SetrecentFenceDateTime(nowISO8601);  // e.g. "2018-01-01T01:23:45Z"
    recentFenceDateTime = QDateTime::fromString(prefsManager.GetrecentFenceDateTime(),
                                                          "yyyy-MM-dd'T'hh:mm:ss'Z'");
    recentFenceDateTime.setTimeSpec(Qt::UTC);  // set timezone (all times are UTC)

    firstTimeSongIsPlayed = true;   // this forces writing another record to the songplays DB when the song is next played
                                    //   so that the song will be marked Recent if it is played again after a Clear Recents,
                                    //   even though it was already loaded and played once before the Clear Recents.

    // update the song table
    reloadSongAges(ui->actionShow_All_Ages->isChecked());
}


// --------------------------
QString MainWindow::logFilePath;  // static members must be explicitly instantiated if in class scope

void MainWindow::customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QHash<QtMsgType, QString> msgLevelHash({{QtDebugMsg, "Debug"}, {QtInfoMsg, "Info"}, {QtWarningMsg, "Warning"}, {QtCriticalMsg, "Critical"}, {QtFatalMsg, "Fatal"}});
//    QByteArray localMsg = msg.toLocal8Bit();

    QString dateTime = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);  // use ISO8601 UTC timestamps
    QString logLevelName = msgLevelHash[type];
    QString txt = QString("%1 %2: %3 (%4)").arg(dateTime, logLevelName, msg, context.file);

    if (msg.contains("The provided value 'moz-chunked-arraybuffer' is not a valid enum value of type XMLHttpRequestResponseType")) {
//         This is a known warning that is spit out once per loaded PDF file.  It's just noise, so suppressing it from the debug.log file.
        return;
    }

    if (msg.contains("js:")) {
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
    Q_UNUSED(context)
    QHash<QtMsgType, QString> msgLevelHash({{QtDebugMsg, "Debug"}, {QtInfoMsg, "Info"}, {QtWarningMsg, "Warning"}, {QtCriticalMsg, "Critical"}, {QtFatalMsg, "Fatal"}});
//    QByteArray localMsg = msg.toLocal8Bit();

//    QString dateTime = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);  // use ISO8601 UTC timestamps
    QString logLevelName = msgLevelHash[type];
//    QString txt = QString("%1 %2: %3 (%4)").arg(dateTime, logLevelName, msg, context.file);
    QString txt = QString("%1: %2").arg(logLevelName, msg);

    // suppress known warnings from QtCreator Application Output window
    if (msg.contains("The provided value 'moz-chunked-arraybuffer' is not a valid enum value of type XMLHttpRequestResponseType") ||
            msg.contains("js:") ||
            txt.contains("Warning: #") ||
            msg.contains("GL Type: core_profile")) {
        return;
    }

    qDebug() << txt; // inside QtCreator, just log it to the console

}

// ----------------------------------------------------------------------
// sets the NowPlaying label, and color (based on whether it's a flashcall or not)
//   this is done many times, so factoring it out to here.
// flashcall defaults to false
void MainWindow::setNowPlayingLabelWithColor(QString s, bool flashcall) {
    if (flashcall) {
        ui->nowPlayingLabel->setStyleSheet("QLabel { color : red; font-style: italic; }");
    } else {
        ui->nowPlayingLabel->setStyleSheet("QLabel { color : black; font-style: normal; }");
    }
    ui->nowPlayingLabel->setText(s);
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

    bool isSingingCall = songTypeNamesForSinging.contains(currentSongType) ||
                         songTypeNamesForCalled.contains(currentSongType);

    bool isPatter = songTypeNamesForPatter.contains(currentSongType);

// If the MP3 file has an embedded TBPM frame in the ID3 tag, then it overrides the libbass auto-detect of BPM
    double songBPM_ID3 = getID3BPM(currentMP3filenameWithPath);  // returns 0.0, if not found or not understandable

    if (songBPM_ID3 != 0.0) {
        songBPM = static_cast<int>(songBPM_ID3);
        tempoIsBPM = true;  // this song's tempo is BPM, not %
    }

    baseBPM = songBPM;  // remember the base-level BPM of this song, for when the Tempo slider changes later

    // Intentionally compare against a narrower range here than BPM detection, because BPM detection
    //   returns a number at the limits, when it's actually out of range.
    // Also, turn off BPM for xtras (they are all over the place, including round dance cues, which have no BPM at all).
    //
    // TODO: make the types for turning off BPM detection a preference
    if ((songBPM>=125-15) && (songBPM<=125+15) && currentSongType != "xtras") {
        tempoIsBPM = true;
        ui->currentTempoLabel->setText(QString::number(songBPM) + " BPM (100%)"); // initial load always at 100%

        ui->tempoSlider->setMinimum(songBPM-15);
        ui->tempoSlider->setMaximum(songBPM+15);

        ui->tempoSlider->setValue(songBPM);  // qDebug() << "handleDurationBPM set tempo slider to: " << songBPM;
        emit ui->tempoSlider->valueChanged(songBPM);  // fixes bug where second song with same BPM doesn't update songtable::tempo

        ui->tempoSlider->SetOrigin(songBPM);    // when double-clicked, goes here (MySlider function)
        ui->tempoSlider->setEnabled(true);
//        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
//                                 ", base tempo: " + QString::number(songBPM) + " BPM");
    }
    else {
        tempoIsBPM = false;
        // if we can't figure out a BPM, then use percent as a fallback (centered: 100%, range: +/-20%)
        ui->currentTempoLabel->setText("100%");
        ui->tempoSlider->setMinimum(100-20);        // allow +/-20%
        ui->tempoSlider->setMaximum(100+20);
        ui->tempoSlider->setValue(100);
        emit ui->tempoSlider->valueChanged(100);  // fixes bug where second song with same 100% doesn't update songtable::tempo
        ui->tempoSlider->SetOrigin(100);  // when double-clicked, goes here
        ui->tempoSlider->setEnabled(true);
//        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
//                                 ", base tempo: 100%");
    }

    // NOTE: we need to set the bounds BEFORE we set the actual positions
//    qDebug() << "MainWindow::handleDurationBPM: length_sec = " << length_sec;
    ui->dateTimeEditIntroTime->setTimeRange(QTime(0,0,0,0), QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*length_sec+0.5)));
    ui->dateTimeEditOutroTime->setTimeRange(QTime(0,0,0,0), QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*length_sec+0.5)));

    ui->seekBar->SetSingingCall(isSingingCall); // if singing call, color the seek bar
    ui->seekBarCuesheet->SetSingingCall(isSingingCall); // if singing call, color the seek bar

    if (isPatter) {
        on_loopButton_toggled(true); // default is to loop, if type is patter
//        ui->tabWidget->setTabText(lyricsTabNumber, "Patter");  // Lyrics tab does double duty as Patter tab
        ui->pushButtonSetIntroTime->setText("Start Loop");
        ui->pushButtonSetOutroTime->setText("End Loop");
        ui->pushButtonTestLoop->setHidden(false);
        analogClock->setSingingCallSection("");
    } else {
        // NOTE: if unknown type, it will be treated as a singing call, so as to NOT set looping
        // singing call or vocals or xtras, so Loop mode defaults to OFF
        on_loopButton_toggled(false); // default is to loop, if type is patter
//        ui->tabWidget->setTabText(lyricsTabNumber, "Lyrics");  // Lyrics tab is named "Lyrics"
        ui->pushButtonSetIntroTime->setText("In");
        ui->pushButtonSetOutroTime->setText("Out");
        ui->pushButtonTestLoop->setHidden(true);
    }

}

void MainWindow::on_actionSquareDesk_Help_triggered()
{
    QString musicDirPath = prefsManager.GetmusicPath();
    QString referenceDir = musicDirPath + "/reference";

    // ---------------
    // Find something that looks like the SquareDesk doc (we probably copied it there)
    bool hasSDESKpdf = false;
    QString pathToSquareDeskDoc;

    QDirIterator it2(referenceDir);
    while(it2.hasNext()) {
        it2.next();
        QString s1 = it2.fileName();
        // if 123.SDESK.pdf or SDESK.pdf, then do NOT copy one in as 190.SDESK.pdf
        static QRegularExpression re12("^[0-9]+\\.SDESK.pdf");
        static QRegularExpression re13("^SDESK.pdf");
        if (s1.contains(re12) || s1.contains(re13)) {
           hasSDESKpdf = true;
           pathToSquareDeskDoc = QString("file://") + referenceDir + "/" + s1;
           break;
        }
    }

    if (hasSDESKpdf) {
//        qDebug() << "FOUND SQUAREDESK DOC:" << pathToSquareDeskDoc;
        QDesktopServices::openUrl(QUrl(pathToSquareDeskDoc, QUrl::TolerantMode));
    }
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
        static QRegularExpression re10("^[0-9]+\\.SD.pdf");
        static QRegularExpression re11("^SD.pdf");
        if (s1.contains(re10) || s1.contains(re11)) {
           hasSDpdf = true;
           pathToSDdoc = QString("file://") + referenceDir + "/" + s1;
           break;
        }
    }

    if (hasSDpdf) {
//        qDebug() << "FOUND SD DOC:" << pathToSDdoc;
        QDesktopServices::openUrl(QUrl(pathToSDdoc, QUrl::TolerantMode));
    }

}

void MainWindow::on_actionReport_a_Bug_triggered()
{
    QString pathToGithubSquaredeskIssues = "https://github.com/mpogue2/SquareDesk/issues";
    QDesktopServices::openUrl(QUrl(pathToGithubSquaredeskIssues, QUrl::TolerantMode));
}

