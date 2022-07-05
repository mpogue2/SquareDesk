/****************************************************************************
**
** Copyright (C) 2016-2022 Mike Pogue, Dan Lyke
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

#if defined(Q_OS_MAC) | defined(Q_OS_WIN)
#include "src/communicator.h"
#endif

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

#ifndef M1MAC
// All other platforms:
extern bass_audio cBass;  // make this accessible to PreferencesDialog
bass_audio cBass;
#else
// M1 Silicon Mac only:
extern flexible_audio cBass;  // make this accessible to PreferencesDialog
flexible_audio cBass;
#endif

static const char *music_file_extensions[] = { "mp3", "wav", "m4a", "flac" };       // NOTE: must use Qt::CaseInsensitive compares for these
static const char *cuesheet_file_extensions[3] = { "htm", "html", "txt" };          // NOTE: must use Qt::CaseInsensitive compares for these
static QString title_tags_prefix("&nbsp;<span style=\"background-color:%1; color: %2;\"> ");
static QString title_tags_suffix(" </span>");
static QRegularExpression title_tags_remover("(\\&nbsp\\;)*\\<\\/?span( .*?)?>");


//class SelectionRetainer {
//    QTextEdit *textEdit;
//    QTextCursor cursor;
//    int anchorPosition;
//    int cursorPosition;
//    // No inadvertent copies
//private:
//    SelectionRetainer() {};
//    SelectionRetainer(const SelectionRetainer &) {};
//public:
//    SelectionRetainer(QTextEdit *textEdit) : textEdit(textEdit), cursor(textEdit->textCursor())
//    {
//        anchorPosition = cursor.anchor();
//        cursorPosition = cursor.position();
//    }
//    ~SelectionRetainer() {
//        cursor.setPosition(anchorPosition);
//        cursor.setPosition(cursorPosition, QTextCursor::KeepAnchor);
//        textEdit->setTextCursor(cursor);
//    }
//};


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

    for (QString hotkey : hotkeys)
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
    cBass.StreamGetLength();  // tell everybody else what the length of the stream is...
    InitializeSeekBar(ui->seekBar);          // and now we can set the max of the seekbars, so they show up
    InitializeSeekBar(ui->seekBarCuesheet);  // and now we can set the max of the seekbars, so they show up

//    qDebug() << "haveDuration2 BPM = " << cBass.Stream_BPM;

    handleDurationBPM();  // finish up the UI stuff, when we know duration and BPM

    secondHalfOfLoad(currentSongTitle);  // now that we have duration and BPM, can finish up asynchronous load.
}

// ----------------------------------------------------------------------
MainWindow::MainWindow(QSplashScreen *splash, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    oldFocusWidget(nullptr),
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
    //Q_UNUSED(splash)

#ifdef M1MAC
    connect(&cBass, SIGNAL(haveDuration()), this, SLOT(haveDuration2()));  // when decode complete, we know MP3 duration
#endif
    lastAudioDeviceName = "";

    lyricsCopyIsAvailable = false;
    lyricsTabNumber = 1;

    lastSavedPlaylist = "";  // no playlists saved yet in this session

    filewatcherShouldIgnoreOneFileSave = false;

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

    // Disable ScreenSaver while SquareDesk is running
//#if defined(Q_OS_MAC)
//    macUtils.disableScreensaver(); // NOTE: now needs to be called every N seconds
//#elif defined(Q_OS_WIN)
//#pragma comment(lib, "user32.lib")
//    SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE , NULL, SPIF_SENDWININICHANGE);
//#elif defined(Q_OS_LINUX)
//    // TODO
//#endif

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
    cBass.Init();

    //Set UI update
    cBass.SetVolume(100);
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

    musicRootPath = prefsManager.GetmusicPath();
    guestRootPath = ""; // initially, no guest music
    guestVolume = "";   // and no guest volume present
    guestMode = "main"; // and not guest mode

    // ERROR LOGGING ------
    logFilePath = musicRootPath + "/.squaredesk/debug.log";

    switchToLyricsOnPlay = prefsManager.GetswitchToLyricsOnPlay();

    updateRecentPlaylistMenu();

    t.elapsed(__LINE__);

//#define DISABLEFILEWATCHER 1

#ifndef DISABLEFILEWATCHER
    PerfTimer t2("filewatcher init", __LINE__);

    // ---------------------------------------
//    // let's watch for changes in the musicDir
//    QDirIterator it(musicRootPath, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QDirIterator it(musicRootPath, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    QRegularExpression ignoreTheseDirs("/(reference|choreography|notes|playlists|sd|soundfx|lyrics)");
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

    fileWatcherTimer = new QTimer();  // Retriggerable timer for file watcher events
    QObject::connect(fileWatcherTimer, SIGNAL(timeout()), this, SLOT(fileWatcherTriggered())); // this calls musicRootModified again (one last time!)


//    musicRootWatcher.addPath(musicRootPath);  // let's not forget the musicDir itself
    QObject::connect(&musicRootWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(musicRootModified(QString)));

    // make sure that the "downloaded" directory exists, so that when we sync up with the Cloud,
    //   it will cause a rescan of the songTable and dropdown

    t.elapsed(__LINE__);

    QDir dir(musicRootPath + "/lyrics/downloaded");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    t.elapsed(__LINE__);

    // do the same for lyrics (included the "downloaded" subdirectory) -------
//    QDirIterator it2(musicRootPath + "/lyrics", QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
//    while (it2.hasNext()) {
//        QString aPath = it2.next();
//        lyricsWatcher.addPath(aPath); // watch for add/deletes to musicDir and interesting subdirs
////        qDebug() << "adding lyrics path: " << aPath;
//        t.elapsed(__LINE__);
//    }

    t.elapsed(__LINE__);

    // COMMENTING THESE OUT:  the lyrics watcher was only useful for drag copying in new lyrics files, which is done infrequently.
    //   And, because of a bug, it was crashing.  So, commented out maybeLyricsChanged AND turned off the lyricsWatcher entirely for now.
//    lyricsWatcher.addPath(musicRootPath + "/lyrics");      // add the root lyrics directory itself
//    lyricsWatcher.addPath(musicRootPath + "/lyrics/downloaded");

//    QObject::connect(&lyricsWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(maybeLyricsChanged()));
    // ---------------------------------------
    t.elapsed(__LINE__);

    t2.elapsed(__LINE__);

//    qDebug() << "Dirs 1:" << musicRootWatcher.directories();
//    qDebug() << "Dirs 2:" << lyricsWatcher.directories();

#endif

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

    // used to store the file paths
//    splash->showMessage("Locating songs...", Qt::AlignBottom + Qt::AlignHCenter, Qt::red);
//    QCoreApplication::instance()->processEvents();  // process events to date

    findMusic(musicRootPath,"","main", true);  // get the filenames from the user's directories

//    splash->showMessage("Loading song info...", Qt::AlignBottom + Qt::AlignHCenter, Qt::red);
//    QCoreApplication::instance()->processEvents();  // process events to date

    t.elapsed(__LINE__);

    loadMusicList(); // and filter them into the songTable

    connect(ui->songTable->horizontalHeader(),&QHeaderView::sectionResized,
            this, &MainWindow::columnHeaderResized);

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

    ui->pushButtonEditLyrics->setStyleSheet("QToolButton { border: 1px solid #575757; border-radius: 4px; background-color: palette(base); }");  // turn off border

    // ----------
    updateSongTableColumnView(); // update the actual view of Age/Pitch/Tempo in the songTable view

    t.elapsed(__LINE__);

    // ----------
    clockColoringHidden = !prefsManager.GetexperimentalClockColoringEnabled();
    analogClock->setHidden(clockColoringHidden);

    // -----------
    ui->actionAutostart_playback->setChecked(prefsManager.Getautostartplayback());
    ui->actionViewTags->setChecked(prefsManager.GetshowSongTags());

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
    analogClock->setTimerLabel(ui->warningLabel, ui->warningLabelCuesheet);  // tell the clock which label to use for the patter timer

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

    t.elapsed(__LINE__);

    readFlashCallsList();

    t.elapsed(__LINE__);

    currentSongType = "";
    currentSongTitle = "";
    currentSongLabel = "";

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
    QStringList ag1; // , ag2;
    ag1 << "Normal" << "Color only" << "Mental image" << "Sight" << "Random" << "Random Color only"; // OK to have one be prefix of another

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
                    } else if (submenuName == "Numbers") {
                        sdActionGroupNumbers->addAction(action2);
                    } else if (submenuName == "Genders") {
                        sdActionGroupGenders->addAction(action2);
                    }
                }
            }
        } else {
//            qDebug() << "item: " << action->text(); // top level item
            if (ag1.contains(action->text()) ) {
                sdActionGroup1->addAction(action); // ag1 items are all mutually exclusive, and are all at top level
            }
        }
    }
    // END ITERATION -------

    // -----------------------
//    // Make SD menu items for GUI level mutually exclusive
//    QList<QAction*> actions2 = ui->menuSequence->actions();
//    qDebug() << "ACTIONS 2:" << actions2;

//    sdActionGroup2 = new QActionGroup(this);
//    sdActionGroup2->setExclusive(true);

//    // WARNING: fragile.  If you add menu items above these, the numbers must be changed manually.
//    //   Is there a better way to do this?
//#define BASICNUM (0)
//    sdActionGroup2->addAction(actions2[BASICNUM]);      // Basic
//    sdActionGroup2->addAction(actions2[BASICNUM+1]);    // Mainstream
//    sdActionGroup2->addAction(actions2[BASICNUM+2]);    // Plus
//    sdActionGroup2->addAction(actions2[BASICNUM+3]);    // A1
//    sdActionGroup2->addAction(actions2[BASICNUM+4]);    // A2
//    sdActionGroup2->addAction(actions2[BASICNUM+5]);    // C1
//    sdActionGroup2->addAction(actions2[BASICNUM+6]);    // C2
//    sdActionGroup2->addAction(actions2[BASICNUM+7]);    // C3a

//    connect(sdActionGroup2, SIGNAL(triggered(QAction*)), this, SLOT(sdAction2Triggered(QAction*)));

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

    int songCount = 0;
    QString firstBadSongLine;

    QString oldCurrentPlaylistFileName = musicRootPath + "/.squaredesk/current.m3u";
    QString newCurrentPlaylistFileName = musicRootPath + "/.squaredesk/current.csv";

    if (QFileInfo::exists(newCurrentPlaylistFileName)) {
        // if the V2 version exists, use it, and ignore the M3U file
        firstBadSongLine = loadPlaylistFromFile(newCurrentPlaylistFileName, songCount);  // load "current.csv" (if doesn't exist, do nothing)
    } else if (QFileInfo::exists(oldCurrentPlaylistFileName)) {
        // if we didn't find a new V2 version, look for an old M3U version, and it present, load it.
        // when saved next time, it will become a V2 CSV version.
        firstBadSongLine = loadPlaylistFromFile(oldCurrentPlaylistFileName, songCount);  // load "current.m3u" (if doesn't exist, do nothing)
    }

    t.elapsed(__LINE__);

    ui->pushButtonCueSheetEditTitle->setStyleSheet("font-weight: bold;");

    ui->pushButtonCueSheetEditBold->setStyleSheet("font-weight: bold;");
    ui->pushButtonCueSheetEditItalic->setStyleSheet("font: italic;");

//    QPalette* palette1 = new QPalette();
//    palette1->setColor(QPalette::ButtonText, Qt::red);
//    ui->pushButtonCueSheetEditHeader->setPalette(*palette1);
    ui->pushButtonCueSheetEditHeader->setStyleSheet("color: red");

    t.elapsed(__LINE__);

//    QPalette* palette2 = new QPalette();
//    palette2->setColor(QPalette::ButtonText, QColor("#0000FF"));
//    ui->pushButtonCueSheetEditArtist->setPalette(*palette2);
    ui->pushButtonCueSheetEditArtist->setStyleSheet("color: #0000FF");

    t.elapsed(__LINE__);

//    QPalette* palette3 = new QPalette();
//    palette3->setColor(QPalette::ButtonText, QColor("#60C060"));
//    ui->pushButtonCueSheetEditLabel->setPalette(*palette3);
    ui->pushButtonCueSheetEditLabel->setStyleSheet("color: #60C060");

//    QPalette* palette4 = new QPalette();
//    palette4->setColor(QPalette::Background, QColor("#FFC0CB"));
//    ui->pushButtonCueSheetEditLyrics->setPalette(*palette4);

//    ui->pushButtonCueSheetEditLyrics->setAutoFillBackground(true);
//    ui->pushButtonCueSheetEditLyrics->setStyleSheet(
//                "QPushButton {background-color: #FFC0CB; color: #000000; border-radius:4px; padding:1px 8px; border:0.5px solid #CF9090;}"
//                "QPushButton:checked { background-color: qlineargradient(x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 #1E72FE, stop: 1 #3E8AFC); color: #FFFFFF; border:0.5px solid #0D60E3;}"
//                "QPushButton:pressed { background-color: qlineargradient(x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 #1E72FE, stop: 1 #3E8AFC); color: #FFFFFF; border:0.5px solid #0D60E3;}"
//                "QPushButton:disabled {background-color: #F1F1F1; color: #7F7F7F; border-radius:4px; padding:1px 8px; border:0.5px solid #D0D0D0;}"
//                );
    ui->pushButtonCueSheetEditLyrics->setStyleSheet(
                "QPushButton {background-color: #FFC0CB; color: #000000; border-radius:4px; padding:1px 8px; border:0.5px solid #CF9090;}"
                "QPushButton:checked { background-color: qlineargradient(x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 #1E72FE, stop: 1 #3E8AFC); color: #FFFFFF; border:0.5px solid #0D60E3;}"
                "QPushButton:pressed { background-color: qlineargradient(x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 #1E72FE, stop: 1 #3E8AFC); color: #FFFFFF; border:0.5px solid #0D60E3;}"
                "QPushButton:disabled { background-color: #FFC0CB; color: #000000; border-radius:4px; padding:1px 8px; border:0.5px solid #CF9090;}"
                );

    t.elapsed(__LINE__);

    maybeLoadCSSfileIntoTextBrowser();

    QTimer::singleShot(0,ui->titleSearch,SLOT(setFocus()));

    t.elapsed(__LINE__);

//    splash->showMessage("Loading reference docs...", Qt::AlignBottom + Qt::AlignHCenter, Qt::red);
//    QCoreApplication::instance()->processEvents();  // process events to date

    initReftab();

    t.elapsed(__LINE__);

    currentSDVUILevel      = "Plus"; // one of sd's names: {basic, mainstream, plus, a1, a2, c1, c2, c3a}
    currentSDKeyboardLevel = "Plus"; // one of sd's names: {basic, mainstream, plus, a1, a2, c1, c2, c3a}

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

    lastSongTableRowSelected = -1;  // meaning "no selection"

    ui->pushButtonCueSheetEditSave->hide();   // the two save buttons are now invisible
    ui->pushButtonCueSheetEditSaveAs->hide();
    ui->pushButtonEditLyrics->show();  // and the "unlock for editing" button shows up!
    ui->actionSave->setEnabled(false);  // save is disabled to start out
    ui->actionSave_As->setEnabled(false);  // save as... is also disabled at the start
    ui->pushButtonSetIntroTime->setEnabled(false);
    ui->pushButtonSetOutroTime->setEnabled(false);
    ui->dateTimeEditIntroTime->setEnabled(false);
    ui->dateTimeEditOutroTime->setEnabled(false);

    cBass.SetIntelBoostEnabled(prefsManager.GetintelBoostIsEnabled());
    cBass.SetIntelBoost(FREQ_KHZ, static_cast<float>(prefsManager.GetintelCenterFreq_KHz()/10.0)); // yes, we have to initialize these manually
    cBass.SetIntelBoost(BW_OCT,  static_cast<float>(prefsManager.GetintelWidth_oct()/10.0));
    cBass.SetIntelBoost(GAIN_DB, static_cast<float>(prefsManager.GetintelGain_dB()/10.0));  // expressed as positive number

    on_actionShow_group_station_toggled(prefsManager.Getenablegroupstation());
    on_actionShow_order_sequence_toggled(prefsManager.Getenableordersequence());
    {
        QString sizesStr = prefsManager.GetSDTabVerticalSplitterPosition();
        if (!sizesStr.isEmpty())
        {
            QList<int> sizes;
            for (QString sizeStr : sizesStr.split(","))
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
            for (QString sizeStr : sizesStr.split(","))
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
}

void MainWindow::fileWatcherTriggered() {
//    qDebug() << "fileWatcherTriggered()";
    musicRootModified(QString("DONE"));
}

void MainWindow::musicRootModified(QString s)
{
//    Q_UNUSED(s)
//    qDebug() << "musicRootModified() = " << s;
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
        findMusic(musicRootPath,"","main", true);  // get the filenames from the user's directories
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
    // all this mess, just to restore NO FOCUS, after ApplicationActivate, if there was NO FOCUS
    //   when going into Inactive state
    if (!justWentActive && new1 == nullptr) {
        oldFocusWidget = old1;  // GOING INACTIVE, RESTORE THIS ONE
    } else if (justWentActive) {
        if (oldFocusWidget == nullptr) {
            if (QApplication::focusWidget() != nullptr) {
                QApplication::focusWidget()->clearFocus();  // BOGUS
            }
        } else {
//            oldFocusWidget->setFocus(); // RESTORE HAPPENS HERE;  RESTORE DISABLED because it crashes
                                          //  if leave app with # open for edit and come back in
        }
        justWentActive = false;  // this was a one-shot deal.
    } else {
        oldFocusWidget = new1;  // clicked on something, this is the one to restore
    }

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

//    QHash<QString,QString>::const_iterator i;
//    for (i = ages.constBegin(); i != ages.constEnd(); ++i) {
//        qDebug() << "key: " << i.key() << ", val: " << i.value();
//    }

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
// =============================================================================
// =============================================================================

// LYRICS EDITOR WAS HERE

// =============================================================================
// =============================================================================
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
            for (auto session : sessions)
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
    
    for (auto session : sessions)
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

//void MainWindow::on_actionIn_Out_Loop_points_to_default_triggered(bool /* checked */)
//{
//    // MUST scan here (once), because the user asked us to, and SetDefaultIntroOutroPositions() (below) needs it
//    cBass.songStartDetector(qPrintable(currentMP3filenameWithPath), &startOfSong_sec, &endOfSong_sec);

//    ui->seekBarCuesheet->SetDefaultIntroOutroPositions(tempoIsBPM, cBass.Stream_BPM, startOfSong_sec, endOfSong_sec, cBass.FileLength);
//    ui->seekBar->SetDefaultIntroOutroPositions(tempoIsBPM, cBass.Stream_BPM, startOfSong_sec, endOfSong_sec, cBass.FileLength);
//    double length = cBass.FileLength;
//    double intro = ui->seekBarCuesheet->GetIntro();
//    double outro = ui->seekBarCuesheet->GetOutro();

//    ui->dateTimeEditIntroTime->setTime(QTime(0,0,0,0).addMSecs((int)(1000.0*intro*length+0.5)));  // milliseconds
//    ui->dateTimeEditOutroTime->setTime(QTime(0,0,0,0).addMSecs((int)(1000.0*outro*length+0.5)));  // milliseconds
//}

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
    // Just before the app quits, save the current playlist state in "current.csv", and it will be reloaded
    //   when the app starts up again.
    // Save the current playlist state to ".squaredesk/current.m3u".  Tempo/pitch are NOT saved here.
    QString PlaylistFileName = musicRootPath + "/.squaredesk/current.csv";
    saveCurrentPlaylistToFile(PlaylistFileName);

    // bug workaround: https://bugreports.qt.io/browse/QTBUG-56448
    QColorDialog colorDlg(nullptr);
    colorDlg.setOption(QColorDialog::NoButtons);
    colorDlg.setCurrentColor(Qt::white);

    delete ui;
    delete sd_redo_stack;

//    // REENABLE SCREENSAVER, RELEASE THE KRAKEN
//#if defined(Q_OS_MAC)
//    macUtils.reenableScreensaver();
//#elif defined(Q_OS_WIN32)
//    SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE , NULL, SPIF_SENDWININICHANGE);
//#endif
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

        double songLength = cBass.FileLength;
//        qDebug() << "songLength: " << songLength << ", Intro: " << ui->seekBar->GetIntro();

//        cBass.SetLoop(songLength * 0.9, songLength * 0.1); // FIX: use parameters in the MP3 file
        cBass.SetLoop(songLength * static_cast<double>(ui->seekBar->GetOutro()),
                      songLength * static_cast<double>(ui->seekBar->GetIntro()));
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

    cBass.Stop();                 // Stop playback, rewind to the beginning
    cBass.StopAllSoundEffects();  // and, it also stops ALL sound effects

    setNowPlayingLabelWithColor(currentSongTitle);

#ifdef REMOVESILENCE
    // last thing we do is move the stream position to 1 sec before start of music
    // this will move BOTH seekBar's to the right spot
    cBass.StreamSetPosition((double)startOfSong_sec);
    Info_Seekbar(false);  // update just the text
#else
    ui->seekBar->setValue(0);
    ui->seekBarCuesheet->setValue(0);
    Info_Seekbar(false);  // update just the text
#endif
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

//    if (currentState == kStopped || currentState == kPaused) {
    uint32_t Stream_State = cBass.currentStreamState();
    if (Stream_State != BASS_ACTIVE_PLAYING) {
        cBass.Play();  // currently stopped or paused or stalled, so try to start playing

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
                QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
                QModelIndexList selected = selectionModel->selectedRows();

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
                for (int i = 0; i < ui->tabWidget->count(); ++i)
                {
                    if (ui->tabWidget->tabText(i).endsWith("*Lyrics"))  // do not switch if *Patter or Patter (using Lyrics tab for written Patter)
                    {
                        ui->tabWidget->setCurrentIndex(i);
                        break;
                    }
                }
            }
            ui->songTable->setSortingEnabled(true);
        }
        // If we just started playing, clear focus from all widgets
        if (QApplication::focusWidget() != nullptr) {
            QApplication::focusWidget()->clearFocus();  // we don't want to continue editing the search fields after a STOP
                                                        //  or it will eat our keyboard shortcuts
        }
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));  // change PLAY to PAUSE
        ui->actionPlay->setText("Pause");
//        currentState = kPlaying;
    }
    else {
        // TODO: we might want to restore focus here....
        // currently playing, so pause playback
        cBass.Pause();
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
        ui->actionPlay->setText("Play");
//        currentState = kPaused;
        setNowPlayingLabelWithColor(currentSongTitle);
    }

}

// ----------------------------------------------------------------------
//bool MainWindow::timerStopStartClick(QTimer *&timer, QPushButton *button)
//{
//    if (timer) {
//        button->setText("Start");
//        timer->stop();
//        delete timer;
//        timer = nullptr;
//    }
//    else {
//        button->setText("Stop");
//        timer = new QTimer(this);
//        timer->start(1000);
//    }
//    return nullptr != timer;
//}

// ----------------------------------------------------------------------
//int MainWindow::updateTimer(qint64 timeZeroEpochMs, QLabel *label)
//{
//    QDateTime now(QDateTime::currentDateTime());
//    qint64 timeNowEpochMs = now.currentMSecsSinceEpoch();
//    int signedSeconds = static_cast<int>((timeNowEpochMs - timeZeroEpochMs) / 1000);
//    int seconds = signedSeconds;
//    char sign = ' ';

//    if (seconds < 0) {
//        sign = '-';
//        seconds = -seconds;
//    }

//    stringstream ss;
//    int hours = seconds / (60*60);
//    int minutes = (seconds / 60) % 60;

//    ss << sign;
//    if (hours) {
//        ss << hours << ":" << setw(2);
//    }
//    ss << setfill('0') << minutes << ":" << setw(2) << setfill('0') << (seconds % 60);
//    string s(ss.str());
//    label->setText(s.c_str());
//    return signedSeconds;
//}

//// ----------------------------------------------------------------------
//void MainWindow::on_pushButtonCountDownTimerStartStop_clicked()
//{
//}

//// ----------------------------------------------------------------------

//void MainWindow::on_pushButtonCountDownTimerReset_clicked()
//{
//}

//// ----------------------------------------------------------------------
//void MainWindow::on_pushButtonCountUpTimerStartStop_clicked()
//{
//}

//// ----------------------------------------------------------------------
//void MainWindow::on_pushButtonCountUpTimerReset_clicked()
//{
//}

//// ----------------------------------------------------------------------
//void MainWindow::timerCountUp_update()
//{
//}

//// ----------------------------------------------------------------------
//void MainWindow::timerCountDown_update()
//{
//}

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
// SONGTABLEREFACTOR
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
    cBass.SetVolume(voltageLevelToSet);     // now logarithmic, 0 --> 0.01, 50 --> 0.1, 100 --> 1.0 (values * 100 for libbass)
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
        cBass.SetTempo(newBASStempo);
        ui->currentTempoLabel->setText(QString::number(value) + " BPM (" + QString::number(newBASStempo) + "%)");
    }
    else {
        double basePercent = 100.0;                      // original detected percent
        double desiredPercent = static_cast<double>(value);            // desired percent
        int newBASStempo = static_cast<int>(round(100.0*desiredPercent/basePercent));
        cBass.SetTempo(newBASStempo);
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
    seekBar->setMaximum(static_cast<int>(cBass.FileLength)-1); // NOTE: TRICKY, counts on == below
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
        cBass.StreamGetPosition();  // update cBass.Current_Position

        if (ui->pitchSlider->value() != cBass.Stream_Pitch) {  // DEBUG DEBUG DEBUG
            qDebug() << "ERROR: Song was loaded, and cBass pitch did not match pitch slider!";
            qDebug() << "    pitchSlider =" << ui->pitchSlider->value();
            qDebug() << "    cBass.Pitch/Tempo/Volume = " << cBass.Stream_Pitch << ", " << cBass.Stream_Tempo << ", " << cBass.Stream_Volume;
            cBass.SetPitch(ui->pitchSlider->value());
        }

        int currentPos_i = static_cast<int>(cBass.Current_Position);

// to help track down the failure-to-progress error...
#define WATCHDOG460
#ifdef WATCHDOG460
        // watchdog for "failure to progress" error ------
        if (cBass.currentStreamState() == BASS_ACTIVE_PLAYING) {
            if (cBass.Current_Position <= previousPosition
                    // || previousPosition > 10.0  // DEBUG
               )
            {
                qDebug() << "Failure/manual seek:" << previousPosition << cBass.Current_Position
                         << "songloaded: " << songLoaded;
                previousPosition = -1.0;  // uninitialized
            } else {
//                qDebug() << "Progress:" << previousPosition << cBass.Current_Position;
                // this will always be executed on the first info_seekbar update when playing
                previousPosition = cBass.Current_Position;  // remember previous position
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
                    cBass.currentStreamState() == BASS_ACTIVE_PLAYING) {
                // lyrics scrolling at the same time as the InfoBar
                ui->textBrowserCueSheet->verticalScrollBar()->setValue(static_cast<int>(targetScroll));
            }
        }
        int fileLen_i = static_cast<int>(cBass.FileLength);

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
            double introLength = static_cast<double>(ui->seekBar->GetIntro()) * cBass.FileLength; // seconds
            double outroTime = static_cast<double>(ui->seekBar->GetOutro()) * cBass.FileLength; // seconds
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

//            if (cBass.Stream_State == BASS_ACTIVE_PLAYING &&
            if (//cBass.currentStreamState() == BASS_ACTIVE_PLAYING &&
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
//            if (cBass.Stream_State == BASS_ACTIVE_PLAYING && songTypeNamesForPatter.contains(currentSongType)) {
            if (cBass.currentStreamState() == BASS_ACTIVE_PLAYING && songTypeNamesForPatter.contains(currentSongType)) {
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

//    if (cBass.Stream_State == BASS_ACTIVE_PLAYING) {
    if (cBass.currentStreamState() == BASS_ACTIVE_PLAYING) {
        // if we're playing, this is accurate to sub-second.
        cBass.StreamGetPosition(); // snapshot the current position
        position = cBass.Current_Position;
        length = cBass.FileLength;  // always use the value with maximum precision

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
    // This is called once per second, to update the seekbar and associated dynamic text

//#if defined(Q_OS_MAC)
//    if (screensaverSeconds++ % 55 == 0) {  // 55 because lowest screensaver setting is 60 seconds of idle time
//        macUtils.disableScreensaver(); // NOTE: now needs to be called every N seconds
//    }
//#endif

//    qDebug() << "VERTICAL SCROLL VALUE: " << ui->textBrowserCueSheet->verticalScrollBar()->value();


    Info_Seekbar(true);

    // update the session coloring analog clock
    QTime time = QTime::currentTime();
    int theType = NONE;
//    qDebug() << "Stream_State:" << cBass.Stream_State; //FIX
//    if (cBass.Stream_State == BASS_ACTIVE_PLAYING) {
    uint32_t Stream_State = cBass.currentStreamState();
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
//    } else if (cBass.Stream_State == BASS_ACTIVE_PAUSED) {
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

    // -----------------------
    // about once a second, poll for volume changes
    newVolumeList = getCurrentVolumes();  // get the current state of the world
//    qSort(newVolumeList);  // sort to allow direct comparison
    std::sort(newVolumeList.begin(), newVolumeList.end());
    if(lastKnownVolumeList == newVolumeList){
        // This space intentionally blank.
    } else {
        on_newVolumeMounted();
    }
    lastKnownVolumeList = newVolumeList;

    // check for Audio Device changes, and set UI status bar, if there's a change
    if ((cBass.currentAudioDevice != lastAudioDeviceName)||(lastAudioDeviceName == "")) {
//        qDebug() << "NEW CURRENT AUDIO DEVICE: " << cBass.currentAudioDevice;
        lastAudioDeviceName = cBass.currentAudioDevice;
        microphoneStatusUpdate();  // now also updates the audioOutputDevice status
//        ui->statusBar->showMessage("Audio output: " + lastAudioDeviceName);
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_vuMeterTimerTick(void)
{
    double currentVolumeSlider = ui->volumeSlider->value();
    int level = cBass.StreamGetVuMeter();
    double levelF = (currentVolumeSlider/100.0)*level/32768.0;
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
#ifndef M1MAC
                   QString("<a href=\"http://www.un4seen.com/bass.html\">libbass</a>, ") +
                   QString("<a href=\"http://www.jobnik.org/?mnu=bass_fx\">libbass_fx</a>, ") +
#endif
                   QString("<a href=\"http://www.lynette.org/sd\">sd</a>, ") +
//                   QString("<a href=\"http://cmusphinx.sourceforge.net\">PocketSphinx</a>, ") +
                   QString("<a href=\"https://github.com/yshurik/qpdfjs\">qpdfjs</a>, ") +
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
// #define WEBVIEWSWORK
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

        MainWindow *maybeMainWindow = dynamic_cast<MainWindow *>((dynamic_cast<QApplication *>(Object))->activeWindow());
        if (maybeMainWindow == nullptr) {
            // if the PreferencesDialog is open, for example, do not dereference the NULL pointer (duh!).
//            qDebug() << "QObject::eventFilter()";
            return QObject::eventFilter(Object,Event);
        }

        int cindex = ui->tabWidget->currentIndex();  // get index of tab, so we can see which it is
        bool tabIsLyricsOrPatter = (ui->tabWidget->tabText(cindex) == "Lyrics" || ui->tabWidget->tabText(cindex) == "*Lyrics" ||  // and I'm on the Lyrics/Patter tab
                                    ui->tabWidget->tabText(cindex) == "Patter" || ui->tabWidget->tabText(cindex) == "*Patter");

        bool cmdC_KeyPressed = (KeyEvent->modifiers() & Qt::ControlModifier) && KeyEvent->key() == Qt::Key_C;

//        qDebug() << "tabIsLyricsOrPatter: " << tabIsLyricsOrPatter << ", cmdC_KeyPressed: " << cmdC_KeyPressed <<
//                    "lyricsCopyIsAvailable: " << maybeMainWindow->lyricsCopyIsAvailable << "has focus: " << ui->textBrowserCueSheet->hasFocus();

        // if any of these widgets has focus, let them process the key
        //  otherwise, we'll process the key
        // UNLESS it's one of the search/timer edit fields and the ESC key is pressed (we must still allow
        //   stopping of music when editing a text field).  Sorry, can't use the SPACE BAR
        //   when editing a search field, because " " is a valid character to search for.
        //   If you want to do this, hit ESC to get out of edit search field mode, then SPACE.
        if (ui->lineEditSDInput->hasFocus()
            && KeyEvent->key() == Qt::Key_Tab)
        {
            maybeMainWindow->do_sd_tab_completion();
            return true;
        }
        else if (cmdC_KeyPressed                        // When CMD-C is pressed
                 && tabIsLyricsOrPatter                 // and we're on the Lyrics editor tab
                 && maybeMainWindow->lyricsCopyIsAvailable    // and the lyrics edit widget told us that copy was available
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
                (KeyEvent->key() == Qt::Key_Escape
#if defined(Q_OS_MACOS)
                 // on MAC OS X, backtick is equivalent to ESC (e.g. devices that have TouchBar)
                 || KeyEvent->key() == Qt::Key_QuoteLeft
#endif
                                                          ) )  ||
                  // OR, IF ONE OF THE SEARCH FIELDS HAS FOCUS, AND RETURN/UP/DOWN_ARROW IS PRESSED
             ( (ui->labelSearch->hasFocus() || ui->typeSearch->hasFocus() || ui->titleSearch->hasFocus()) &&
               (KeyEvent->key() == Qt::Key_Return || KeyEvent->key() == Qt::Key_Up || KeyEvent->key() == Qt::Key_Down)
             )
                  // These next 3 help work around a problem where going to a different non-SDesk window and coming back
                  //   puts the focus into a the Type field, where there was NO FOCUS before.  But, that field usually doesn't
                  //   have any characters in it, so if the user types SPACE, the right thing happens, and it goes back to NO FOCUS.
                  // I think this is a reasonable tradeoff right now.
                  // OR, IF THE LABEL SEARCH FIELD HAS FOCUS, AND IT HAS NO CHARACTERS OF TEXT YET, AND SPACE OR PERIOD IS PRESSED
                  || (ui->labelSearch->hasFocus() && ui->labelSearch->text().length() == 0 && (KeyEvent->key() == Qt::Key_Space || KeyEvent->key() == Qt::Key_Period))
                  // OR, IF THE TYPE SEARCH FIELD HAS FOCUS, AND IT HAS NO CHARACTERS OF TEXT YET, AND SPACE OR PERIOD IS PRESSED
                  || (ui->typeSearch->hasFocus() && ui->typeSearch->text().length() == 0 && (KeyEvent->key() == Qt::Key_Space || KeyEvent->key() == Qt::Key_Period))
                  // OR, IF THE TITLE SEARCH FIELD HAS FOCUS, AND IT HAS NO CHARACTERS OF TEXT YET, AND SPACE OR PERIOD IS PRESSED
                  || (ui->titleSearch->hasFocus() && ui->titleSearch->text().length() == 0 && (KeyEvent->key() == Qt::Key_Space || KeyEvent->key() == Qt::Key_Period))
                  // OR, IF THE LYRICS TAB SET INTRO FIELD HAS FOCUS, AND SPACE OR PERIOD IS PRESSED
                  || (ui->dateTimeEditIntroTime->hasFocus() && (KeyEvent->key() == Qt::Key_Space || KeyEvent->key() == Qt::Key_Period))
                  // OR, IF THE LYRICS TAB SET OUTRO FIELD HAS FOCUS, AND SPACE OR PERIOD IS PRESSED
                  || (ui->dateTimeEditOutroTime->hasFocus() && (KeyEvent->key() == Qt::Key_Space || KeyEvent->key() == Qt::Key_Period))
           ) {
            // call handleKeypress on the Applications's active window ONLY if this is a MainWindow
//            qDebug() << "eventFilter YES:" << ui << currentWindowName << maybeMainWindow;
            // THEN HANDLE IT AS A SPECIAL KEY
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
    cBass.FadeOutAndPause();
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
        for (QString s : songTypeNamesForPatter)
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
//                if (currentState == kPlaying) {
                if (cBass.currentStreamState() == BASS_ACTIVE_PLAYING) {
                    on_playButton_clicked();  // we were playing, so PAUSE now.
                }
            }

            cBass.StopAllSoundEffects();  // and, it also stops ALL sound effects
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
            if (ui->typeSearch->hasFocus() || ui->labelSearch->hasFocus() || ui->titleSearch->hasFocus()) {
//                qDebug() << "   and search has focus.";

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
                    return true;
                }

                on_songTable_itemDoubleClicked(ui->songTable->item(row,1));
                ui->typeSearch->clearFocus();
                ui->labelSearch->clearFocus();
                ui->titleSearch->clearFocus();
            }
            break;

        case Qt::Key_Down:
        case Qt::Key_Up:
//            qDebug() << "Key up/down detected.";
            if (ui->typeSearch->hasFocus() || ui->labelSearch->hasFocus() || ui->titleSearch->hasFocus()) {
//                qDebug() << "   and search has focus.";
                if (key == Qt::Key_Up) {
                    // TODO: this same code appears FOUR times.  FACTOR IT
                    // on_actionPrevious_Playlist_Item_triggered();
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
                        return true;
                    }

// SONGTABLEREFACTOR
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

                } else {
// SONGTABLEREFACTOR
                    // TODO: this same code appears FOUR times.  FACTOR IT
                    // on_actionNext_Playlist_Item_triggered();
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
    cBass.StreamGetPosition();  // update the position
    // set the position to one second before the end, so that RIGHT ARROW works as expected
    cBass.StreamSetPosition(static_cast<int>(fmin(cBass.Current_Position + 10.0, cBass.FileLength-1.0)));
    Info_Seekbar(true);
}

void MainWindow::on_actionSkip_Back_15_sec_triggered()
{
    Info_Seekbar(true);
    cBass.StreamGetPosition();  // update the position
    cBass.StreamSetPosition(static_cast<int>(fmax(cBass.Current_Position - 10.0, 0.0)));
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
    cBass.SetEq(0, static_cast<double>(value));
    saveCurrentSongSettings();
}

void MainWindow::on_midrangeSlider_valueChanged(int value)
{
    cBass.SetEq(1, static_cast<double>(value));
    saveCurrentSongSettings();
}

void MainWindow::on_trebleSlider_valueChanged(int value)
{
    cBass.SetEq(2, static_cast<double>(value));
    saveCurrentSongSettings();
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
        { QRegularExpression("^([Oo][Gg][Rr][Mm][Pp]3\\s*\\d{1,5})\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },    // e.g. "OGRMP3 04 - Addam's Family.mp3"
        { QRegularExpression("^(4-[Bb][Aa][Rr]-[Bb]\\s*\\d{1,5})\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },    // e.g. "4-bar-b 123 - Chicken Plucker"
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
        { QRegularExpression("^([A-Z]{1,5}+[\\- ]*\\d+[A-Za-z0-9]*)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 }, // e.g. "ABC 123h1-Chicken Plucker"
        { QRegularExpression("^([A-Z0-9]{1,5}+)\\s*(\\d+)([a-zA-Z]{1,2})?\\s*-\\s*(.*?)\\s*(\\(.*\\))?$"), 4, 1, 2, 3, 5 }, // SIR 705b - Papa Was A Rollin Stone (Instrumental).mp3
        { QRegularExpression("^([A-Z0-9]{1,5}+)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },    // e.g. "POP - Chicken Plucker" (if it has a dash but fails all other tests,
        { QRegularExpression("^(.*?)\\s*\\-\\s*([A-Z]{1,5})(\\d{1,5})\\s*(\\(.*\\))?$"), 1, 2, 3, -1, 4 },    // e.g. "A Summer Song - CHIC3002 (female vocals)
        { QRegularExpression("^(.*?)\\s*\\-\\s*([A-Za-z]{1,7}|4-[Bb][Aa][Rr]-[Bb])-(\\d{1,5})(\\-?([AB]))?$"), 1, 2, 3, 5, -1 },    // e.g. "Paper Doll - Windsor-4936B"
        { QRegularExpression("^(.*?)\\s*\\-\\s*([A-Za-z]{1,7}|4-[Bb][Aa][Rr]-[Bb]) (\\d{1,5})(\\-?([AB]))?$"), 1, 2, 3, 5, -1 },    // e.g. "Paper Doll - Windsor-4936B"

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
        case SongFilenameNameDashLabel :
            return filename_first_matches;
        case SongFilenameBestGuess :
            return best_guess_matches;
        case SongFilenameLabelDashName :
//        default:  // all the cases are covered already (default is not needed here)
            return label_first_matches;
    }
    // Shut up the warnings, all the returns happen in the switch above
    return label_first_matches;
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
    labelnum = labelnum.simplified();
    title = title.simplified();
    shortTitle = shortTitle.simplified();

    return foundParts;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
class CuesheetWithRanking {
public:
    QString filename;
    QString name;
    int score;
};
#pragma clang diagnostic pop

static bool CompareCuesheetWithRanking(CuesheetWithRanking *a, CuesheetWithRanking *b)
{
    if (a->score == b->score) {
        return a->name < b->name;  // if scores are equal, sort names lexicographically (note: should be QCollator numericMode() for natural sort order)
    }
    // else:
    return a->score > b->score;
}

// -----------------------------------------------------------------
QStringList splitIntoWords(const QString &str)
{
    static QRegularExpression regexNotAlnum(QRegularExpression("\\W+"));

    QStringList words = str.split(regexNotAlnum);

    static QRegularExpression LetterNumber("[A-Z][0-9]|[0-9][A-Z]"); // do we need to split?  Most of the time, no.
    QRegularExpressionMatch quickmatch(LetterNumber.match(str));

    if (quickmatch.hasMatch()) {
        static QRegularExpression regexLettersAndNumbers("^([A-Z]+)([0-9].*)$");
        static QRegularExpression regexNumbersAndLetters("^([0-9]+)([A-Z].*)$");
//        qDebug() << "quickmatch!";
        // we gotta split it one word at a time
//        words = str.split(regexNotAlnum);
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
    }
    // else no splitting needed (e.g. it's already split, as is the case for most cuesheets)
    //   so we skip the per-word splitting, and go right to sorting
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
            || (score >= l1.length())                       // all of l1 words matched something in l2
            || (score >= l2.length())                       // all of l2 words matched something in l1
            || score >= 4)
        )
    {
        QString s1 = l1.join("-");
        QString s2 = l2.join("-");
        return score * 500 + 100 * (abs(l1.length()) - l2.length());
    }
    else
        return 0;
}

// TODO: the match needs to be a little fuzzier, since RR103B - Rocky Top.mp3 needs to match RR103 - Rocky Top.html
void MainWindow::findPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets)
{
    PerfTimer t("findPossibleCuesheets", __LINE__);

    QString fileType = filepath2SongType(MP3Filename);
    bool fileTypeIsPatter = (fileType == "patter");

    t.elapsed(__LINE__);

    QFileInfo mp3FileInfo(MP3Filename);
    QString mp3CanonicalPath = mp3FileInfo.canonicalPath();
    QString mp3CompleteBaseName = mp3FileInfo.completeBaseName();
    QString mp3Label = "";
    QString mp3Labelnum = "";
    QString mp3Labelnum_short = "";
    QString mp3Labelnum_extra = "";
    QString mp3Title = "";
    QString mp3ShortTitle = "";

    t.elapsed(__LINE__);

    breakFilenameIntoParts(mp3CompleteBaseName, mp3Label, mp3Labelnum, mp3Labelnum_extra, mp3Title, mp3ShortTitle);
    QList<CuesheetWithRanking *> possibleRankings;

    QStringList mp3Words = splitIntoWords(mp3CompleteBaseName);
    mp3Labelnum_short = mp3Labelnum;
    while (mp3Labelnum_short.length() > 0 && mp3Labelnum_short[0] == '0')
    {
        mp3Labelnum_short.remove(0,1);
    }

//    QList<QString> extensions;
//    QString dot(".");
//    for (size_t i = 0; i < sizeof(cuesheet_file_extensions) / sizeof(*cuesheet_file_extensions); ++i)
//    {
//        extensions.append(dot + cuesheet_file_extensions[i]);
//    }

    t.elapsed(__LINE__);

    QListIterator<QString> iter(*pathStack);
    while (iter.hasNext()) {

        QString s = iter.next();

//        // Is this a file extension we recognize as a cuesheet file?
//        QListIterator<QString> extensionIterator(extensions);
//        bool foundExtension = false;
//        while (extensionIterator.hasNext())
//        {
//            extensionIndex++;
//            QString extension(extensionIterator.next());
//            if (s.endsWith(extension))
//            {
//                foundExtension = true;
//                break;
//            }
//        }

//        bool foundExtension0 = s.endsWith("htm");
//        bool foundExtension1 = s.endsWith("html");
//        bool foundExtension2 = s.endsWith("txt");

        int extensionIndex = 0;

        if (s.endsWith("htm", Qt::CaseInsensitive)) {
            // nothing
        } else if (s.endsWith("html", Qt::CaseInsensitive)) {
            extensionIndex = 1;
        } else if (s.endsWith("txt", Qt::CaseInsensitive)) {
            extensionIndex = 2;
        } else {
            continue;
        }

        QStringList sl1 = s.split("#!#");
        QString type = sl1[0];  // the type (of original pathname, before following aliases)
        QString filename = sl1[1];  // everything else

//        qDebug() << "possibleCuesheets(): " << fileTypeIsPatter << filename << filepath2SongType(filename) << type;
        if (fileTypeIsPatter && (type=="lyrics")) {
            // if it's a patter MP3, then do NOT match it against anything in the lyrics folder
            continue;
        }

//        if (type=="choreography" || type == "sd" || type=="reference") {
//            // if it's a dance program .txt file, or an sd sequence file, or a reference .txt file, don't bother trying to match
//            continue;
//        }

        QFileInfo fi(filename);
//        QString fi2 = fi.canonicalPath();

//        if (fi2 == musicRootPath && type.right(1) != "*") {
//            // e.g. "/Users/mpogue/__squareDanceMusic/C 117 - Bad Puppy (Patter).mp3" --> NO TYPE PRESENT and NOT a guest song
//            type = "";
//        }

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
//        qDebug() << "           " << type;
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
// qDebug() << "Candidate: " << filename << completeBaseName << score;
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

    t.elapsed(__LINE__);

//    qSort(possibleRankings.begin(), possibleRankings.end(), CompareCuesheetWithRanking);
    std::sort(possibleRankings.begin(), possibleRankings.end(), CompareCuesheetWithRanking);
    QListIterator<CuesheetWithRanking *> iterRanked(possibleRankings);
    while (iterRanked.hasNext())
    {
        CuesheetWithRanking *cswr = iterRanked.next();
        possibleCuesheets.append(cswr->filename);
        delete cswr;
    }

    // append the MP3 filename itself, IF it contains lyrics, but do this at the end,
    //  after all the other (scored/ranked) cuesheets are added to the list of possibleCuesheets,
    //  so that the default for a new song will be a real cuesheet, rather than the lyrics inside the MP3 file.

    //    qDebug() << "findPossibleCuesheets():";
        QString mp3Lyrics = loadLyrics(MP3Filename);
    //    qDebug() << "mp3Lyrics:" << mp3Lyrics;
        if (mp3Lyrics.length())
        {
            possibleCuesheets.append(MP3Filename);
        }
}

void MainWindow::loadCuesheets(const QString &MP3FileName, const QString preferredCuesheet)
{
    hasLyrics = false;

    QString HTML;

    QStringList possibleCuesheets;
    findPossibleCuesheets(MP3FileName, possibleCuesheets);

//    qDebug() << "possibleCuesheets:" << possibleCuesheets;

    int defaultCuesheetIndex = 0;
    loadedCuesheetNameWithPath = ""; // nothing loaded yet

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
        ui->menuLyrics->setTitle("Patter");
//        ui->actionFilePrint->setText("Print Patter...");
//        ui->actionSave_Lyrics->setText("Save Patter");
//        ui->actionSave_Lyrics_As->setText("Save Patter As...");
        ui->actionAuto_scroll_during_playback->setText("Auto-scroll Patter");

        if (hasLyrics && lyricsTabNumber != -1) {
//            qDebug() << "loadCuesheets 2: " << "setting to *";
            ui->tabWidget->setTabText(lyricsTabNumber, "*Patter");
        } else {
//            qDebug() << "loadCuesheets 2: " << "setting to NOT *";
            ui->tabWidget->setTabText(lyricsTabNumber, "Patter");

            // ------------------------------------------------------------------
            // get pre-made patter.template.html file, if it exists
            QString patterTemplate = getResourceFile("patter.template.html");
//            qDebug() << "patterTemplate: " << patterTemplate;
            if (patterTemplate.isEmpty()) {
                ui->textBrowserCueSheet->setHtml("No patter found for this song.");
                loadedCuesheetNameWithPath = "";
            } else {
                ui->textBrowserCueSheet->setHtml(patterTemplate);
                loadedCuesheetNameWithPath = "patter.template.html";  // as a special case, this is allowed to not be the full path
            }

        } // else (sequence could not be found)
    } else {
        // ----- SINGING CALL -----
        ui->menuLyrics->setTitle("Lyrics");
        ui->actionAuto_scroll_during_playback->setText("Auto-scroll Cuesheet");

        if (hasLyrics && lyricsTabNumber != -1) {
            ui->tabWidget->setTabText(lyricsTabNumber, "*Lyrics");
        } else {
            ui->tabWidget->setTabText(lyricsTabNumber, "Lyrics");

            // ------------------------------------------------------------------
            // get pre-made lyrics.template.html file, if it exists
            QString lyricsTemplate = getResourceFile("lyrics.template.html");
//            qDebug() << "lyricsTemplate: " << lyricsTemplate;
            if (lyricsTemplate.isEmpty()) {
                ui->textBrowserCueSheet->setHtml("No lyrics found for this song.");
                loadedCuesheetNameWithPath = "";
            } else {
                ui->textBrowserCueSheet->setHtml(lyricsTemplate);
                loadedCuesheetNameWithPath = "lyrics.template.html";  // as a special case, this is allowed to not be the full path
            }

        } // else (lyrics could not be found)
    } // isPatter

}


double MainWindow::getID3BPM(QString MP3FileName) {
    Q_UNUSED(MP3FileName)
/*
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

    return(theBPM); */  // M1MAC FIX
    return(0.0);
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

//    qDebug() << "MainWindow::loadMP3File: songTitle: " << songTitle << ", songType: " << songType << ", songLabel: " << songLabel;
//    RecursionGuard recursion_guard(loadingSong);
    loadingSong = true;
    firstTimeSongIsPlayed = true;

    currentMP3filenameWithPath = MP3FileName;

    // resolve aliases at load time, rather than findFilesRecursively time, because it's MUCH faster
    QFileInfo fi(MP3FileName);
    QString resolvedFilePath = fi.symLinkTarget(); // path with the symbolic links followed/removed
    if (resolvedFilePath != "") {
        MP3FileName = resolvedFilePath;
    }

    t.elapsed(__LINE__);

    currentSongType = songType;  // save it for session coloring on the analog clock later...
    currentSongLabel = songLabel;   // remember it, in case we need it later

    ui->pushButtonEditLyrics->setChecked(false); // lyrics/cuesheets of new songs when loaded default to NOT editable

    loadCuesheets(MP3FileName);

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

    QDir md(MP3FileName);
    QString canonicalFN = md.canonicalPath();

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

    cBass.StreamCreate(MP3FileName.toStdString().c_str(), &startOfSong_sec, &endOfSong_sec, intro1, outro1);  // load song, and figure out where the song actually starts and ends

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

#ifndef M1MAC
    // THIS CODE IS ONLY HERE FOR X86.  M1MAC MOVES THIS DOWN TO secondHalfOfLoad().
    int length_sec = static_cast<int>(cBass.FileLength);
    int songBPM = static_cast<int>(round(cBass.Stream_BPM));  // libbass's idea of the BPM

    bool isSingingCall = songTypeNamesForSinging.contains(songType) ||
                         songTypeNamesForCalled.contains(songType);

    bool isPatter = songTypeNamesForPatter.contains(songType);

    bool isRiverboat = songLabel.startsWith(QString("riv"), Qt::CaseInsensitive);

    if (isRiverboat && isPatter) {
        // All Riverboat patter records are recorded at 126BPM, according to the publisher.
        // This can always be overridden using TBPM in the ID3 tag inside a specific patter song, if needed.
        //        qDebug() << "Riverboat patter detected!";
        songBPM = 126;
        tempoIsBPM = true;  // this song's tempo is BPM, not %
    }

    // If the MP3 file has an embedded TBPM frame in the ID3 tag, then it overrides the libbass auto-detect of BPM
    double songBPM_ID3 = getID3BPM(MP3FileName);  // returns 0.0, if not found or not understandable

    if (songBPM_ID3 != 0.0) {
        songBPM = static_cast<int>(songBPM_ID3);
        tempoIsBPM = true;  // this song's tempo is BPM, not %
    }

    baseBPM = songBPM;  // remember the base-level BPM of this song, for when the Tempo slider changes later

    t.elapsed(__LINE__);

    // Intentionally compare against a narrower range here than BPM detection, because BPM detection
    //   returns a number at the limits, when it's actually out of range.
    // Also, turn off BPM for xtras (they are all over the place, including round dance cues, which have no BPM at all).
    //
    // TODO: make the types for turning off BPM detection a preference
    if ((songBPM>=125-15) && (songBPM<=125+15) && songType != "xtras") {
        tempoIsBPM = true;
        ui->currentTempoLabel->setText(QString::number(songBPM) + " BPM (100%)"); // initial load always at 100%
        t.elapsed(__LINE__);

        ui->tempoSlider->setMinimum(songBPM-15);
        ui->tempoSlider->setMaximum(songBPM+15);

        t.elapsed(__LINE__);
//        bool tryToSetInitialBPM = prefsManager.GettryToSetInitialBPM();
//        int initialBPM = prefsManager.GetinitialBPM();
//        t.elapsed(__LINE__);
//        qDebug() << "tryToSetInitialBPM: " << tryToSetInitialBPM;
//        if (tryToSetInitialBPM) {
//            qDebug() << "tryToSetInitialBPM true: " << initialBPM;
//            // if the user wants us to try to hit a particular BPM target, use that value
//            ui->tempoSlider->setValue(initialBPM);
//            ui->tempoSlider->valueChanged(initialBPM);  // fixes bug where second song with same BPM doesn't update songtable::tempo
//            t.elapsed(__LINE__);
//        } else {
//            // otherwise, if the user wants us to start with the slider at the regular detected BPM
//            //   NOTE: this can be overridden by the "saveSongPreferencesInConfig" preference, in which case
//            //     all saved tempo preferences will always win.
        ui->tempoSlider->setValue(songBPM);
        ui->tempoSlider->valueChanged(songBPM);  // fixes bug where second song with same BPM doesn't update songtable::tempo
//            t.elapsed(__LINE__);
//        }

        ui->tempoSlider->SetOrigin(songBPM);    // when double-clicked, goes here (MySlider function)
        ui->tempoSlider->setEnabled(true);
        t.elapsed(__LINE__);
//        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
//                                 ", base tempo: " + QString::number(songBPM) + " BPM");
        t.elapsed(__LINE__);
    }
    else {
        tempoIsBPM = false;
        // if we can't figure out a BPM, then use percent as a fallback (centered: 100%, range: +/-20%)
        t.elapsed(__LINE__);
        ui->currentTempoLabel->setText("100%");
        t.elapsed(__LINE__);
        ui->tempoSlider->setMinimum(100-20);        // allow +/-20%
        t.elapsed(__LINE__);
        ui->tempoSlider->setMaximum(100+20);
        t.elapsed(__LINE__);
        ui->tempoSlider->setValue(100);
        t.elapsed(__LINE__);
        ui->tempoSlider->valueChanged(100);  // fixes bug where second song with same 100% doesn't update songtable::tempo
        t.elapsed(__LINE__);
        ui->tempoSlider->SetOrigin(100);  // when double-clicked, goes here
        t.elapsed(__LINE__);
        ui->tempoSlider->setEnabled(true);
        t.elapsed(__LINE__);
//        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
//                                 ", base tempo: 100%");
        t.elapsed(__LINE__);
    }
#endif

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

    ui->pitchSlider->valueChanged(ui->pitchSlider->value());    // force pitch change, if pitch slider preset before load
    ui->volumeSlider->valueChanged(ui->volumeSlider->value());  // force vol change, if vol slider preset before load
    ui->mixSlider->valueChanged(ui->mixSlider->value());        // force mix change, if mix slider preset before load

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

    ui->bassSlider->valueChanged(ui->bassSlider->value()); // force bass change, if bass slider preset before load
    ui->midrangeSlider->valueChanged(
        ui->midrangeSlider->value()); // force midrange change, if midrange slider preset before load
    ui->trebleSlider->valueChanged(ui->trebleSlider->value()); // force treble change, if treble slider preset before load

    cBass.Stop();

}

void MainWindow::secondHalfOfLoad(QString songTitle) {
    // This function is called when the files is actually loaded into memory, and the filelength is known.

    // We are NOT doing automatic start-of-song finding right now.
    startOfSong_sec = 0.0;
    endOfSong_sec = cBass.FileLength;  // used by setDefaultIntroOutroPositions below

    // song is loaded now, so init the seekbar min/max (once)
    InitializeSeekBar(ui->seekBar);
    InitializeSeekBar(ui->seekBarCuesheet);

    Info_Seekbar(true);  // update the slider and all the text

    ui->dateTimeEditIntroTime->setTime(QTime(0,0,0,0));
    ui->dateTimeEditOutroTime->setTime(QTime(23,59,59,0));

#ifndef M1MAC
    // THIS IS ONLY FOR X86 BUILD RIGHT NOW.
    // NOTE: we need to set the bounds BEFORE we set the actual positions
    int length_sec = static_cast<int>(cBass.FileLength); // secondHalfOfLoad isn't called until after gotDuratioBPM, which gives cBass the FileLength
    ui->dateTimeEditIntroTime->setTimeRange(QTime(0,0,0,0), QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*length_sec+0.5)));
    ui->dateTimeEditOutroTime->setTimeRange(QTime(0,0,0,0), QTime(0,0,0,0).addMSecs(static_cast<int>(1000.0*length_sec+0.5)));
#endif

    ui->seekBarCuesheet->SetDefaultIntroOutroPositions(tempoIsBPM, cBass.Stream_BPM, startOfSong_sec, endOfSong_sec, cBass.FileLength);
    ui->seekBar->SetDefaultIntroOutroPositions(tempoIsBPM, cBass.Stream_BPM, startOfSong_sec, endOfSong_sec, cBass.FileLength);

    ui->dateTimeEditIntroTime->setEnabled(true);
    ui->dateTimeEditOutroTime->setEnabled(true);

    ui->pushButtonSetIntroTime->setEnabled(true);  // always enabled now, because anything CAN be looped now OR it has an intro/outro
    ui->pushButtonSetOutroTime->setEnabled(true);
    ui->pushButtonTestLoop->setEnabled(true);

    // FIX FIX FIX:
#ifndef M1MAC
    ui->seekBar->SetSingingCall(isSingingCall); // if singing call, color the seek bar
    ui->seekBarCuesheet->SetSingingCall(isSingingCall); // if singing call, color the seek bar
#endif

    cBass.SetVolume(100);
    currentVolume = 100;
    previousVolume = 100;
    Info_Volume();

    // FIX FIX FIX
#ifndef M1MAC
    // On M1MAC, THIS IS RELOCATED TO handleDurationBPM()....
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
#endif

//    qDebug() << "**** NOTE: currentSongTitle = " << currentSongTitle;
    loadSettingsForSong(songTitle); // also loads replayGain, if song has one; also loads tempo

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
        ui->tempoSlider->valueChanged(initialBPM);  // fixes bug where second song with same BPM doesn't update songtable::tempo
    }

//    qDebug() << "setting stream position to: " << startOfSong_sec;
    cBass.StreamSetPosition(startOfSong_sec);  // last thing we do is move the stream position to 1 sec before start of music

    songLoaded = true;  // now seekBar can be updated

//#ifdef Q_OS_MACXXX
//    // TODO: HOW TO FORCE RECALC OF REPLAYGAIN ON A FILE?
//    songLoadedReplayGain_dB = 0.0;
//    bool songHasReplayGain = settings1.isSetReplayGain();
//    if (songHasReplayGain) {
////        qDebug() << "   loadMP3File: replayGain found in DB:" << settings1.getReplayGain();
//        songLoadedReplayGain_dB = settings1.getReplayGain();
//    }
//    if (!songHasReplayGain) // || (songLoadedReplayGain_dB == 0.0) )
//    {
//        // only trigger replayGain calculation, if replayGain is enabled (checkbox is checked)
//        //   and song does NOT have replayGain
//        // //   OR it has ReplayGain, but that gain was exactly 0.0 (which means that ReplayGain
//        // //     failed the last time)  <-- this part disabled intentionally
//        bool replayGainCheckboxIsChecked = prefsManager.GetreplayGainIsEnabled();
//        if (replayGainCheckboxIsChecked) {
//            if (!replayGain_dB(MP3FileName)) {
////                qDebug() << "ERROR: can't get ReplayGain for: " << MP3FileName;
//            } else {
////                qDebug() << "Now calculating ReplayGain for: " << MP3FileName;
//            }
//        } else {
////            qDebug() << "ReplayGain is currently disabled, so calculation will not be started.";
//        }
//    }
//#endif

    setInOutButtonState();
    loadingSong = false;

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

        QString type = section[section.length()-1] + suffix;  // must be the last item in the path
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
                // if it IS a sound FX file (and not GUEST MODE), then let's squirrel away the paths so we can play them later
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


void MainWindow::findMusic(QString mainRootDir, QString guestRootDir, QString mode, bool refreshDatabase)
{
    PerfTimer t("findMusic", __LINE__);
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
            // qDebug() << "findMusic():" << cuesheet_file_extensions[i];
            qsl.append(starDot + cuesheet_file_extensions[i]);
        }

        rootDir1.setNameFilters(qsl);

        findFilesRecursively(rootDir1, pathStack, "", ui, &soundFXfilenames, &soundFXname);  // appends to the pathstack
    }

    if (guestRootDir != "" && (mode == "guest" || mode == "both")) {
        // looks for files in the guestRootDir --------
        QDir rootDir2(guestRootDir);
        rootDir2.setFilter(QDir::Files | QDir::Dirs | QDir::NoDot | QDir::NoDotDot);

        QStringList qsl;
        qsl.append("*.mp3");                // I only want MP3 files
        qsl.append("*.m4a");                //          or M4A files
        qsl.append("*.wav");                //          or WAV files
        rootDir2.setNameFilters(qsl);

        findFilesRecursively(rootDir2, pathStack, "*", ui, &soundFXfilenames, &soundFXname);  // appends to the pathstack, "*" for "Guest"
    }
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
    // Make "it's" and "its" equivalent.
    str.replace("'","");

    int title_end = str.indexOf(title_tags_remover);
    str.replace(title_tags_remover, " ");
    int index = 0;

    if (title_end < 0) title_end = str.length();
                
    for (auto t : list)
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
    QRegularExpression rx("(\\ |\\,|\\.|\\:|\\t\\')"); //RegEx for ' ' or ',' or '.' or ':' or '\t', includes ' to handle the "it's" case.

    QStringList label = ui->labelSearch->text().split(rx);
    QStringList type = ui->typeSearch->text().split(rx);
    QStringList title = ui->titleSearch->text().split(rx);

    ui->songTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);  // DO NOT SET height of rows (for now)

    ui->songTable->setSortingEnabled(false);

// SONGTABLEREFACTOR
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

//    qDebug() << "rowsVisible: " << rowsVisible << ", initialRowCount: " << initialRowCount << ", firstVisibleRow: " << firstVisibleRow;
    if (rowsVisible > 0 && rowsVisible != initialRowCount && firstVisibleRow != -1) {
        ui->songTable->selectRow(firstVisibleRow);
    } else {
        ui->songTable->clearSelection();
    }

    ui->songTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);  // auto set height of rows
}


QString MainWindow::FormatTitlePlusTags(const QString &title, bool setTags, const QString &strtags)
{
    QString titlePlusTags(title.toHtmlEscaped());
    if (setTags && !strtags.isEmpty() && ui->actionViewTags->isChecked())
    {
        titlePlusTags += "&nbsp;";
        QStringList tags = strtags.split(" ");
        for (auto tag : tags)
        {
            QString tagTrimmed(tag.trimmed());
            if (tagTrimmed.length() > 0)
            {
                QPair<QString,QString> color = songSettings.getColorForTag(tag);
                QString prefix = title_tags_prefix.arg(color.first).arg(color.second);
                titlePlusTags +=  prefix + tag.toHtmlEscaped() + title_tags_suffix;
            }
        }
    }
    return titlePlusTags;
}

void setSongTableFont(QTableWidget *songTable, const QFont &currentFont)
{
    for (int row = 0; row < songTable->rowCount(); ++row)
        dynamic_cast<QLabel*>(songTable->cellWidget(row,kTitleCol))->setFont(currentFont);
}


// --------------------------------------------------------------------------------
void MainWindow::loadMusicList()
{
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
        
        QString titlePlusTags(FormatTitlePlusTags(title, settings.isSetTags(), settings.getTags()));
        SongTitleLabel *titleLabel = new SongTitleLabel(this);
        QString songNameStyleSheet("QLabel { color : %1; }");
        titleLabel->setStyleSheet(songNameStyleSheet.arg(textCol.name()));
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
        addStringToLastRowOfSongTable(textCol, ui->songTable,
                                      tempoStr,
                                      kTempoCol);
        // keep the path around, for loading in when we double click on it
        ui->songTable->item(ui->songTable->rowCount()-1, kPathCol)->setData(Qt::UserRole,
                QVariant(origPath)); // path set on cell in col 0

        // Filter out (hide) rows that we're not interested in, based on the search fields...
        //   4 if statements is clearer than a gigantic single if....
        QString labelPlusNumber = label + " " + labelnum;
    }

    QFont currentFont = ui->songTable->font();
    setSongTableFont(ui->songTable, currentFont);
    filterMusic();

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

//    QRegularExpression regexpAmp("&");
//    QRegularExpression regexpLt("<");
//    QRegularExpression regexpGt(">");
//    QRegularExpression regexpApos("'");
//    QRegularExpression regexpNewline("\n");
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

        if (s.endsWith(".txt", Qt::CaseInsensitive)
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
            stream << fileLines[i] << ENDL;
        }
        programs << outputFile;
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
        if (s.indexOf(QRegularExpression("reference/0[0-9][0-9]\\.[a-zA-Z0-9' ]+\\.txt$", QRegularExpression::CaseInsensitiveOption)) != -1)  // matches the Dance Program files in /reference
        {
            //qDebug() << "Dance Program Match:" << s;
            QStringList sl1 = s.split("#!#");
            QString type = sl1[0];  // the type (of original pathname, before following aliases)
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

        addToProgramsAndWriteTextFile(programs, outputDir, "010.basic1.txt", danceprogram_basic1);
        addToProgramsAndWriteTextFile(programs, outputDir, "020.basic2.txt", danceprogram_basic2);
        addToProgramsAndWriteTextFile(programs, outputDir, "025.SSD.txt", danceprogram_SSD);
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


void MainWindow::titleLabelDoubleClicked(QMouseEvent * /* event */)
{
    QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
        on_songTable_itemDoubleClicked(ui->songTable->item(row,kPathCol));
    }
    else {
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

    t.elapsed(__LINE__);

    loadMP3File(pathToMP3, songTitle, songType, songLabel);

    t.elapsed(__LINE__);

    // these must be down here, to set the correct values...
    int pitchInt = pitch.toInt();
    ui->pitchSlider->setValue(pitchInt);

    on_pitchSlider_valueChanged(pitchInt); // manually call this, in case the setValue() line doesn't call valueChanged() when the value set is
                                           //   exactly the same as the previous value.  This will ensure that cBass.setPitch() gets called (right now) on the new stream.

// this code does not appear to do what the comment intended.
//    if (tempo != "0" && tempo != "0%") {
//        // iff tempo is known, then update the table
//        QString tempo2 = tempo.replace("%",""); // if percentage (not BPM) just get rid of the "%" (setValue knows what to do)
//        int tempoInt = tempo2.toInt();
//        if (tempoInt !=0)
//        {
//            ui->tempoSlider->setValue(tempoInt);
//        }
//    }
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

//void MainWindow::on_checkBoxPlayOnEnd_clicked()
//{
//}

//void MainWindow::on_checkBoxStartOnPlay_clicked()
//{
//}

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

        findMusic(musicRootPath, "", "main", true); // always refresh the songTable after the Prefs dialog returns with OK
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
    return(relativePlaylistPath == stripped2);
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

//    qDebug() << "Row " << selectedRow << " is selected now.";

    if (selectedRow != -1) {
        // we've clicked on a real row, which now needs its Title text to be highlighted

        // first: let's un-highlight the previous SongTitleLabel back to its original color
        if (lastSongTableRowSelected != -1) {
            QString origColor = dynamic_cast<const SongTitleLabel*>(ui->songTable->cellWidget(lastSongTableRowSelected, kTitleCol))->textColor;
            QString origColorStyleSheet("QLabel { color : %1; }");
            ui->songTable->cellWidget(lastSongTableRowSelected, kTitleCol)->setStyleSheet(origColorStyleSheet.arg(origColor));
        }

        // second: let's highlight the current selected row's Title
        QString songNameStyleSheet("QLabel { color : white; }");  // TODO: does this need to be fished out of some platform-specific QPalette somewhere?
        ui->songTable->cellWidget(selectedRow, kTitleCol)->setStyleSheet(songNameStyleSheet);

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
            QString basefilename = lastSavedPlaylist.section("/",-1,-1);
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
        menu.addAction ( "Reveal in Finder" , this , SLOT (revealInFinder()) );
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

        for (auto tagUntrimmed : tags)
        {
            QString tag(tagUntrimmed.trimmed());

            if (tag.length() <= 0)
                continue;
            
            bool set = false;
            for (auto t : currentTags)
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
    QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
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
        QString titlePlusTags(FormatTitlePlusTags(title, settings.isSetTags(), settings.getTags()));
        dynamic_cast<QLabel*>(ui->songTable->cellWidget(row,kTitleCol))->setText(titlePlusTags);
    }
}

void MainWindow::editTags()
{
    QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
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

            QString titlePlusTags(FormatTitlePlusTags(title, settings.isSetTags(), settings.getTags()));
            dynamic_cast<QLabel*>(ui->songTable->cellWidget(row,kTitleCol))->setText(titlePlusTags);
        }
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
    }
}

void MainWindow::loadSong()
{
    QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
        on_songTable_itemDoubleClicked(ui->songTable->item(row,kPathCol));
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
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
//        qDebug() << "saveCurrentSongSettings: " << ui->seekBarCuesheet->GetIntro() << ui->seekBarCuesheet->GetOutro();
        setting.setIntroOutroIsTimeBased(false);
        setting.setCuesheetName(cuesheetFilename);
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
//        if (cBass.Stream_replayGain_dB != 0.0) {
////            qDebug() << "***** saveCurrentSongSettings: saving replayGain value for" << currentSong << "= " << cBass.Stream_replayGain_dB;
//            setting.setReplayGain(cBass.Stream_replayGain_dB);
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

        // double length = (double)(ui->seekBarCuesheet->maximum());  // This is not correct, results in non-round-tripping
        double length = cBass.FileLength;  // This seems to work better, and round-tripping looks like it is working now.
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
        ui->tempoSlider->setValue(tempo);
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
   cBass.SetGlobals();
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

//    qSort(volumeDirList);     // always return sorted, for convenient comparisons later
    std::sort(volumeDirList.begin(), volumeDirList.end());

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
//#if QT_VERSION >= 0x051400
        newVolumes = QSet<QString>(newVolumeList.begin(), newVolumeList.end()).subtract(QSet<QString>(lastKnownVolumeList.begin(), lastKnownVolumeList.end()));
//#else
//        newVolumes = newVolumeList.toSet().subtract(lastKnownVolumeList.toSet());
//#endif
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
//#if QT_VERSION >= 0x051400
        goneVolumes = QSet<QString>(lastKnownVolumeList.begin(), lastKnownVolumeList.end()).subtract(QSet<QString>(newVolumeList.begin(), newVolumeList.end()));
//#else
//        goneVolumes = lastKnownVolumeList.toSet().subtract(newVolumeList.toSet());
//#endif
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
    } else if (ui->tabWidget->tabText(index) == "Lyrics" || ui->tabWidget->tabText(index) == "*Lyrics" ||
               ui->tabWidget->tabText(index) == "Patter" || ui->tabWidget->tabText(index) == "*Patter") {
        // Lyrics Tab ---------------
//        ui->actionPrint_Lyrics->setDisabled(false);
//        ui->actionSave_Lyrics->setDisabled(false);      // TODO: enable only when we've made changes
//        ui->actionSave_Lyrics_As->setDisabled(false);   // always enabled, because we can always save as a different name
        ui->actionFilePrint->setDisabled(false);

        // THIS IS WRONG: enabled iff hasLyrics and editing is enabled
        bool okToSave = hasLyrics && ui->pushButtonEditLyrics->isChecked();
        ui->actionSave->setEnabled(okToSave);      // lyrics/patter can be saved when there are lyrics to save
        ui->actionSave_As->setEnabled(okToSave);   // lyrics/patter can be saved as when there are lyrics to save

        if (ui->tabWidget->tabText(index) == "Lyrics" || ui->tabWidget->tabText(index) == "*Lyrics") {
            ui->actionSave->setText("Save Lyrics"); // but greyed out, until modified
            ui->actionSave_As->setText("Save Lyrics As...");  // greyed out until modified

            ui->actionFilePrint->setText("Print Lyrics...");
        } else {
            ui->actionSave->setText("Save Patter"); // but greyed out, until modified
            ui->actionSave_As->setText("Save Patter As...");  // greyed out until modified

            ui->actionFilePrint->setText("Print Patter...");
        }
    } else if (ui->tabWidget->tabText(index) == "Music Player") {
        ui->actionSave->setEnabled((linesInCurrentPlaylist != 0) && (lastSavedPlaylist != ""));      // playlist can be saved if there are >0 lines and it was not current.m3u
        if (lastSavedPlaylist != "") {
            QString basefilename = lastSavedPlaylist.section("/",-1,-1);
            ui->actionSave->setText(QString("Save Playlist") + " '" + basefilename + "'"); // it has a name
        } else {
            ui->actionSave->setText(QString("Save Playlist")); // it doesn't have a name yet
        }
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

    QString kybdStatus("Audio:" + lastAudioDeviceName + ", Kybd:" + currentSDKeyboardLevel);
    micStatusLabel->setStyleSheet("color: black");
    micStatusLabel->setText(kybdStatus);

//    if (ui->tabWidget->tabText(index) == "SD"
//        || ui->tabWidget->tabText(index) == "SD 2") {
//        if (voiceInputEnabled &&
//            currentApplicationState == Qt::ApplicationActive) {
//            if (!ps) {
//                initSDtab();
//            }
//        }
//        if (voiceInputEnabled && ps &&
//            currentApplicationState == Qt::ApplicationActive) {
//            micStatusLabel->setStyleSheet("color: red");
//            micStatusLabel->setText(micsON);
//            killVoiceInput = false;
//        } else {
//            micStatusLabel->setStyleSheet("color: black");
//            micStatusLabel->setText(micsOFF);
//        }
//    } else {
//        if (voiceInputEnabled && currentApplicationState == Qt::ApplicationActive) {
//            micStatusLabel->setStyleSheet("color: black");
//            micStatusLabel->setText(micsOFF);
//        } else {
//            micStatusLabel->setStyleSheet("color: black");
//            micStatusLabel->setText(micsOFF);
//        }
//    }
//    if (killVoiceInput && ps)
//    {
//        ps->kill();
//        delete ps;
//        ps = nullptr;
//    }
}


//void MainWindow::readPSStdErr()
//{
//    QByteArray s(ps->readAllStandardError());
//    QString str = QString::fromUtf8(s.data());
//}

//void MainWindow::readPSData()
//{
//    // ASR --------------------------------------------
//    // pocketsphinx has a valid string, send it to sd
//    QByteArray s = ps->readAllStandardOutput();
//    QString str = QString::fromUtf8(s.data());
//    if (str.startsWith("INFO: "))
//        return;

////    qDebug() << "PS str: " << str;

//    int index = ui->tabWidget->currentIndex();
//    if (!voiceInputEnabled || (currentApplicationState != Qt::ApplicationActive) ||
//        ((ui->tabWidget->tabText(index) != "SD" && ui->tabWidget->tabText(index) != "SD 2"))) {
//        // if voiceInput is explicitly disabled, or the app is not Active, we're not on the sd tab, then voiceInput is disabled,
//        //  and we're going to read the data from PS and just throw it away.
//        // This is a cheesy way to do it.  We really should disable the mics somehow.
//        return;
//    }

//    // NLU --------------------------------------------
//    // This section does the impedance match between what you can say and the exact wording that sd understands.
//    // TODO: put this stuff into an external text file, read in at runtime?
//    //
//    QString s2 = str.toLower();
//    s2.replace("\r\n","\n");  // for Windows PS only, harmless to Mac/Linux
//    s2.replace(QRegularExpression("allocating .* buffers of .* samples each\\n"),"");  // garbage from windows PS only, harmless to Mac/Linux
//    s2 = s2.simplified();

//    if (s2 == "erase" || s2 == "erase that") {
//        ui->lineEditSDInput->clear();
//    }
//    else
//    {
//        ui->lineEditSDInput->setText(s2);
//        on_lineEditSDInput_returnPressed();
//    }
//}

//void MainWindow::pocketSphinx_errorOccurred(QProcess::ProcessError error)
//{
//    Q_UNUSED(error)
//}

//void MainWindow::pocketSphinx_started()
//{
//}

void MainWindow::initReftab() {

#ifdef M1MAC
    ui->tabWidget->setTabVisible(4,false);  // turn off the References tab for now in the M1 build
    return;  // FIX: turned off right now for M1 Mac, because of crashes in the WebEngine on M1
             // FIX: also seeing errors like this: [0306/194140.909675:ERROR:icu_util.cc(252)] Couldn't mmap icu data file, see issue #636
#endif

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
                tabname.remove(QRegularExpression("^1[0-9][0-9]\\."));  // 101.foo/index.html -> "foo"
                HTMLfolderExists = true;
                whichHTM = "/index.html";
            } else {
                QFileInfo info3(filename + "/index.htm");
                if (info3.exists()) {
//                    qDebug() << "    FOUND INDEX.HTM";
                    tabname = filename.split("/").last();
                    tabname.remove(QRegularExpression("^1[0-9][0-9]\\."));  // 101.foo/index.htm -> "foo"
                    HTMLfolderExists = true;
                    whichHTM = "/index.htm";
                }
            }
            if (HTMLfolderExists) {
#if defined(Q_OS_MAC) | defined(Q_OS_WIN32)
                webview[numWebviews] = new QWebEngineView();
#else
                webview[numWebviews] = new QWebView();
#endif

//                QString indexFileURL = "file://" + filename + whichHTM;
#if defined(Q_OS_WIN32)
                QString indexFileURL = "file:///" + filename + whichHTM;  // Yes, Windows is special
#else
                QString indexFileURL = "file://" + filename + whichHTM;   // UNIX-like OS's only need one slash.
#endif
//                qDebug() << "    indexFileURL:" << indexFileURL;
                webview[numWebviews]->setUrl(QUrl(indexFileURL));
                documentsTab->addTab(webview[numWebviews], tabname);
                numWebviews++;
            }
        } else if (filename.endsWith(".txt") &&   // ends in .txt, AND
//                   QRegExp("reference/0[0-9][0-9]\\.[a-zA-Z0-9' ]+\\.txt$", Qt::CaseInsensitive).indexIn(filename) == -1) {  // is not a Dance Program file in /reference
            filename.indexOf(QRegularExpression("reference/0[0-9][0-9]\\.[a-zA-Z0-9' ]+\\.txt$", QRegularExpression::CaseInsensitiveOption)) == -1) {
//                qDebug() << "    FOUND TXT FILE";
                tabname = filename.split("/").last().remove(QRegularExpression(".txt$"));
                tabname.remove(QRegularExpression("^1[0-9][0-9]\\."));  // 122.bar.txt -> "bar"

//                qDebug() << "    tabname:" << tabname;

#if defined(Q_OS_MAC) | defined(Q_OS_WIN32)
                webview[numWebviews] = new QWebEngineView();
#else
                webview[numWebviews] = new QWebView();
#endif

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
                tabname = filename.split("/").last().remove(QRegularExpression(".[Mm][Dd]$"));
#if defined(Q_OS_MAC) | defined(Q_OS_WIN32)
                webview[numWebviews] = new QWebEngineView();
#else
                webview[numWebviews] = new QWebView();
#endif

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

#if defined(Q_OS_MAC) | defined(Q_OS_WIN32)
                Communicator *m_communicator = new Communicator(this);
                m_communicator->setUrl(pdf_path);

                webview[numWebviews] = new QWebEngineView();
                QWebChannel * channel = new QWebChannel(this);
                channel->registerObject(QStringLiteral("communicator"), m_communicator);
                webview[numWebviews]->page()->setWebChannel(channel);

                webview[numWebviews]->load(url);
#else
                webview[numWebviews] = new QWebView();
                webview[numWebviews]->setUrl(url);
#endif


//                QString indexFileURL = "file://" + filename;
//                qDebug() << "    indexFileURL:" << indexFileURL;

//                webview[numWebviews]->setUrl(QUrl(pdf_path));
                QFileInfo fInfo(filename);
                tabname = filename.split("/").last().remove(QRegularExpression(".pdf$"));      // get basename, remove .pdf
                tabname.remove(QRegularExpression("^1[0-9][0-9]\\."));                         // 122.bar.txt -> "bar"  // remove optional 1##. at front
                documentsTab->addTab(webview[numWebviews], tabname);
                numWebviews++;
        }

    } // for loop, iterating through <musicRoot>/reference

    ui->refGridLayout->addWidget(documentsTab, 0,1);
}

void MainWindow::initSDtab() {
//    console->setFixedHeight(150);

//    // POCKET_SPHINX -------------------------------------------
//    //    WHICH=5365
//    //    pocketsphinx_continuous -dict $WHICH.dic -lm $WHICH.lm -inmic yes
//    // MAIN CMU DICT: /usr/local/Cellar/cmu-pocketsphinx/HEAD-584be6e/share/pocketsphinx/model/en-us
//    // TEST DIR: /Users/mpogue/Documents/QtProjects/SquareDesk/build-SquareDesk-Desktop_Qt_5_7_0_clang_64bit-Debug/test123/SquareDesk.app/Contents/MacOS
//    // TEST PS MANUALLY: pocketsphinx_continuous -dict 5365a.dic -jsgf plus.jsgf -inmic yes -hmm ../models/en-us
//    //   also try: -remove_noise yes, as per http://stackoverflow.com/questions/25641154/noise-reduction-before-pocketsphinx-reduces-recognition-accuracy
//    // TEST SD MANUALLY: ./sd
//    unsigned int whichModel = 5365;
//#if defined(Q_OS_MAC) | defined(Q_OS_WIN32)
//    QString appDir = QCoreApplication::applicationDirPath() + "/";  // this is where the actual ps executable is
//    QString pathToPS = appDir + "pocketsphinx_continuous";
//#if defined(Q_OS_WIN32)
//    pathToPS += ".exe";   // executable has a different name on Win32
//#endif

//#else /* must be (Q_OS_LINUX) */
//    QString pathToPS = "pocketsphinx_continuous";
//#endif
//    // NOTE: <whichmodel>a.dic and <VUIdanceLevel>.jsgf MUST be in the same directory.
//    QString pathToDict = QString::number(whichModel) + "a.dic";
//    QString pathToJSGF = currentSDVUILevel + ".jsgf";

//#if defined(Q_OS_MAC)
//    // The acoustic models are one level up in the models subdirectory on MAC
//    QString pathToHMM  = "../models/en-us";
//#endif
//#if defined(Q_OS_WIN32)
//    // The acoustic models are at the same level, but in the models subdirectory on MAC
//    QString pathToHMM  = "models/en-us";
//#endif
//#if defined(Q_OS_LINUX)
//    QString pathToHMM = "../pocketsphinx/binaries/win32/models/en-us/";
//#endif

//    QStringList PSargs;
//    PSargs << "-dict" << pathToDict     // pronunciation dictionary
//           << "-jsgf" << pathToJSGF     // language model
//           << "-inmic" << "yes"         // use the built-in microphone
//           << "-remove_noise" << "yes"  // try turning on PS noise reduction
//           << "-hmm" << pathToHMM;      // the US English acoustic model (a bunch of files) is in ../models/en-us

//    ps = new QProcess(Q_NULLPTR);

//    qDebug() << "PS start: " << pathToPS << PSargs;

//    ps->setWorkingDirectory(QCoreApplication::applicationDirPath()); // NOTE: nothing will be written here
//    ps->setProcessChannelMode(QProcess::MergedChannels);
//    ps->setReadChannel(QProcess::StandardOutput);
//    connect(ps, SIGNAL(readyReadStandardOutput()),
//            this, SLOT(readPSData()));                 // output data from ps
//    connect(ps, SIGNAL(readyReadStandardError()),
//            this, SLOT(readPSStdErr()));                 // output data from ps
//    connect(ps, SIGNAL(errorOccurred(QProcess::ProcessError)),
//            this, SLOT(pocketSphinx_errorOccurred(QProcess::ProcessError)));
//    connect(ps, SIGNAL(started()),
//            this, SLOT(pocketSphinx_started()));
//    ps->start(pathToPS, PSargs);

//    bool startedStatus = ps->waitForStarted();
//    if (!startedStatus)
//    {
//        delete ps;
//        ps = nullptr;
//    }

    // SD -------------------------------------------
    copyrightShown = false;  // haven't shown it once yet
}

//void MainWindow::on_actionEnable_voice_input_toggled(bool checked)
//{
//    if (checked) {
//        ui->actionEnable_voice_input->setChecked(true);
//        voiceInputEnabled = true;
//    }
//    else {
//        ui->actionEnable_voice_input->setChecked(false);
//        voiceInputEnabled = false;
//    }

//    microphoneStatusUpdate();

//    // the Enable Voice Input setting is persistent across restarts of the application
//    prefsManager.Setenablevoiceinput(ui->actionEnable_voice_input->isChecked());
//}

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
//    renderArea->setCoupleColoringScheme(action->text());  // removed: causes crash on Win32, and not needed
    setSDCoupleColoringScheme(action->text());
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
        sprintf(cmd, "osascript -e 'do shell script \"networksetup -setairportpower en0 off\"'\n");
    } else {
        sprintf(cmd, "osascript -e 'do shell script \"networksetup -setairportpower en0 on\"'\n");
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
    cBass.StopAllSoundEffects();
}

void MainWindow::playSFX(QString which) {
    QString soundEffectFile;

    if (which.toInt() > 0) {
        soundEffectFile = soundFXfilenames[which.toInt()-1];
    } else {
        // conversion failed, this is break_end or long_tip.mp3
        soundEffectFile = musicRootPath + "/soundfx/" + which + ".mp3";
    }

    if(QFileInfo(soundEffectFile).exists()) {
        // play sound FX only if file exists...
        cBass.PlayOrStopSoundEffect(which.toInt(),
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
    QRegularExpression rxVersion("^(\\d+)\\.(\\d+)\\.(\\d+)");
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
            if (s1.contains(QRegularExpression("^[0-9]+\\.SD.pdf")) || s1.contains(QRegularExpression("^SD.pdf"))) {
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
            if (s1.contains(QRegularExpression("^[0-9]+\\.SDESK.pdf")) || s1.contains(QRegularExpression("^SDESK.pdf"))) {
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
    cBass.StopAllSoundEffects();
}

// FONT SIZE STUFF ========================================
int MainWindow::pointSizeToIndex(int pointSize) {
    // converts our old-style range: 11,13,15,17,19,21,23,25
    //   to an index:                 0, 1, 2, 3, 4, 5, 6, 7
    // returns -1 if not in range, or if even number
    if (pointSize < 11 || pointSize > 25 || pointSize % 2 != 1) {
        return(-1);
    }
    return((pointSize-11)/2);
}

int MainWindow::indexToPointSize(int index) {
#if defined(Q_OS_MAC)
    return (2*index + 11);
#elif defined(Q_OS_WIN)
    return static_cast<int>((8.0/13.0)*(static_cast<double>(2*index + 11)));
#elif defined(Q_OS_LINUX)
    return (2*index + 11);
#endif
}

void MainWindow::setFontSizes()
{
    // initial font sizes (with no zoom in/out)

#if defined(Q_OS_MAC)
    preferredVerySmallFontSize = 13;
    preferredSmallFontSize = 13;
    preferredWarningLabelFontSize = 20;
    preferredNowPlayingFontSize = 27;
#elif defined(Q_OS_WIN32)
    preferredVerySmallFontSize = 8;
    preferredSmallFontSize = 8;
    preferredWarningLabelFontSize = 12;
    preferredNowPlayingFontSize = 16;
#elif defined(Q_OS_LINUX)
    preferredVerySmallFontSize = 8;
    preferredSmallFontSize = 13;
    preferredWarningLabelFontSize = 20;
    preferredNowPlayingFontSize = 27;
#endif

    QFont font = ui->currentTempoLabel->font();  // system font for most everything

    // preferred very small text
    font.setPointSize(preferredVerySmallFontSize);
    ui->typeSearch->setFont(font);
    ui->labelSearch->setFont(font);
    ui->titleSearch->setFont(font);
    ui->lineEditSDInput->setFont(font);  // SD Input box needs to resize, too.
    ui->clearSearchButton->setFont(font);
    ui->songTable->setFont(font);

    // preferred normal small text
    font.setPointSize(preferredSmallFontSize);
    ui->tabWidget->setFont(font);  // most everything inherits from this one
    ui->statusBar->setFont(font);
    micStatusLabel->setFont(font);
    ui->currentLocLabel->setFont(font);
    ui->songLengthLabel->setFont(font);

    ui->bassLabel->setFont(font);
    ui->midrangeLabel->setFont(font);
    ui->trebleLabel->setFont(font);
    ui->EQgroup->setFont(font);

    ui->pushButtonCueSheetEditTitle->setFont(font);
    ui->pushButtonCueSheetEditLabel->setFont(font);
    ui->pushButtonCueSheetEditArtist->setFont(font);
    ui->pushButtonCueSheetEditHeader->setFont(font);
    ui->pushButtonCueSheetEditBold->setFont(font);
    ui->pushButtonCueSheetEditItalic->setFont(font);

    // preferred Warning Label (medium sized)
    font.setPointSize(preferredWarningLabelFontSize);
    ui->warningLabel->setFont(font);
    ui->warningLabelCuesheet->setFont(font);

    // preferred Now Playing (large sized)
    font.setPointSize(preferredNowPlayingFontSize);
    ui->nowPlayingLabel->setFont(font);
}

void MainWindow::adjustFontSizes()
{
    // ui->songTable->resizeColumnToContents(kNumberCol);  // nope
    ui->songTable->resizeColumnToContents(kTypeCol);
    ui->songTable->resizeColumnToContents(kLabelCol);
    // kTitleCol = nope

    QFont currentFont = ui->songTable->font();
    int currentFontPointSize = currentFont.pointSize();  // platform-specific point size

    int index = pointSizeToIndex(currentMacPointSize);  // current index

    // give a little extra space when sorted...
    int sortedSection = ui->songTable->horizontalHeader()->sortIndicatorSection();

    // pixel perfection for each platform
#if defined(Q_OS_MAC)
    double extraColWidth[8] = {0.25, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0};

    double numberBase = 2.0;
    double recentBase = 4.5;
    double ageBase = 3.5;
    double pitchBase = 4.0;
    double tempoBase = 4.5;

    double numberFactor = 0.5;
    double recentFactor = 0.9;
    double ageFactor = 0.5;
    double pitchFactor = 0.5;
    double tempoFactor = 0.9;

    int searchBoxesHeight[8] = {20, 21, 22, 24,  26, 28, 30, 32};
    double scaleWidth1 = 7.75;
    double scaleWidth2 = 3.25;
    double scaleWidth3 = 8.5;

    // lyrics buttons
    unsigned int TitleButtonWidth[8] = {55,60,65,70, 80,90,95,105};

    double maxEQsize = 16.0;
    double scaleIcons = 24.0/13.0;

    int warningLabelSize[8] = {16,20,23,26, 29,32,35,38};  // basically 20/13 * pointSize
//    unsigned int warningLabelWidth[8] = {93,110,126,143, 160,177,194,211};  // basically 20/13 * pointSize * 5.5
    int warningLabelWidth[8] = {65,75,85,95, 105,115,125,135};  // basically 20/13 * pointSize * 5.5

    int nowPlayingSize[8] = {22,27,31,35, 39,43,47,51};  // basically 27/13 * pointSize

    double nowPlayingHeightFactor = 1.5;

    double buttonSizeH = 1.875;
    double buttonSizeV = 1.125;
#elif defined(Q_OS_WIN32)
    double extraColWidth[8] = {0.25, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0};

    double numberBase = 1.5;
    double recentBase = 7.5;
    double ageBase = 5.5;
    double pitchBase = 6.0;
    double tempoBase = 7.5;

    double numberFactor = 0.0;
    double recentFactor = 0.0;
    double ageFactor = 0.0;
    double pitchFactor = 0.0;
    double tempoFactor = 0.0;

    int searchBoxesHeight[8] = {22, 26, 30, 34,  38, 42, 46, 50};
    double scaleWidth1 = 12.75;
    double scaleWidth2 = 5.25;
    double scaleWidth3 = 10.5;

    // lyrics buttons
    unsigned int TitleButtonWidth[8] = {55,60,65,70, 80,90,95,105};

    double maxEQsize = 16.0;
    double scaleIcons = (1.5*24.0/13.0);
    int warningLabelSize[8] = {9,12,14,16, 18,20,22,24};  // basically 20/13 * pointSize
    int warningLabelWidth[8] = {84,100,115,130, 146, 161, 176, 192};  // basically 20/13 * pointSize * 5

//    unsigned int nowPlayingSize[8] = {22,27,31,35, 39,43,47,51};  // basically 27/13 * pointSize
    int nowPlayingSize[8] = {16,20,23,26, 29,32,35,38};  // basically 27/13 * pointSize

    double nowPlayingHeightFactor = 1.5;

    double buttonSizeH = 1.5*1.875;
    double buttonSizeV = 1.5*1.125;
#elif defined(Q_OS_LINUX)
    double extraColWidth[8] = {0.25, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0};

    double numberBase = 2.0;
    double recentBase = 4.5;
    double ageBase = 3.5;
    double pitchBase = 4.0;
    double tempoBase = 4.5;

    double numberFactor = 0.5;
    double recentFactor = 0.9;
    double ageFactor = 0.5;
    double pitchFactor = 0.5;
    double tempoFactor = 0.9;

    int searchBoxesHeight[8] = {22, 26, 30, 34,  38, 42, 46, 50};
    double scaleWidth1 = 7.75;
    double scaleWidth2 = 3.25;
    double scaleWidth3 = 8.5;

    // lyrics buttons
    unsigned int TitleButtonWidth[8] = {55,60,65,70, 80,90,95,105};

    double maxEQsize = 16.0;
    double scaleIcons = 24.0/13.0;
    int warningLabelSize[8] = {16,20,23,26, 29,32,35,38};  // basically 20/13 * pointSize
    int warningLabelWidth[8] = {93,110,126,143, 160,177,194,211};  // basically 20/13 * pointSize * 5.5

    int nowPlayingSize[8] = {22,27,31,35, 39,43,47,51};  // basically 27/13 * pointSize

    double nowPlayingHeightFactor = 1.5;

    double buttonSizeH = 1.875;
    double buttonSizeV = 1.125;
#endif

    // a little extra space when a column is sorted
    // also a little extra space for the smallest zoom size
    ui->songTable->setColumnWidth(kNumberCol, static_cast<int>((numberBase + (sortedSection==kNumberCol?numberFactor:0.0)) *currentFontPointSize));
    ui->songTable->setColumnWidth(kRecentCol, static_cast<int>((recentBase+(sortedSection==kRecentCol?recentFactor:0.0)+extraColWidth[index])*currentFontPointSize));
    ui->songTable->setColumnWidth(kAgeCol, static_cast<int>((ageBase+(sortedSection==kAgeCol?ageFactor:0.0)+extraColWidth[index])*currentFontPointSize));
    ui->songTable->setColumnWidth(kPitchCol, static_cast<int>((pitchBase+(sortedSection==kPitchCol?pitchFactor:0.0)+extraColWidth[index])*currentFontPointSize));
    ui->songTable->setColumnWidth(kTempoCol, static_cast<int>((tempoBase+(sortedSection==kTempoCol?tempoFactor:0.0)+extraColWidth[index])*currentFontPointSize));

    ui->typeSearch->setFixedHeight(searchBoxesHeight[index]);
    ui->labelSearch->setFixedHeight(searchBoxesHeight[index]);
    ui->titleSearch->setFixedHeight(searchBoxesHeight[index]);
    ui->lineEditSDInput->setFixedHeight(searchBoxesHeight[index]);

    ui->dateTimeEditIntroTime->setFixedHeight(searchBoxesHeight[index]);  // this scales the intro/outro button height, too...
    ui->dateTimeEditOutroTime->setFixedHeight(searchBoxesHeight[index]);

#if defined(Q_OS_MAC)
    // the Mac combobox is not height resizeable.  This styled one is, and it looks fine.
    ui->comboBoxCuesheetSelector->setStyle(QStyleFactory::create("Windows"));
    ui->comboBoxCallListProgram->setStyle(QStyleFactory::create("Windows"));
#endif

    // set all the related fonts to the same size
    ui->typeSearch->setFont(currentFont);
    ui->labelSearch->setFont(currentFont);
    ui->titleSearch->setFont(currentFont);
    ui->lineEditSDInput->setFont(currentFont);

    ui->tempoLabel->setFont(currentFont);
    ui->pitchLabel->setFont(currentFont);
    ui->volumeLabel->setFont(currentFont);
    ui->mixLabel->setFont(currentFont);

    ui->currentTempoLabel->setFont(currentFont);
    ui->currentPitchLabel->setFont(currentFont);
    ui->currentVolumeLabel->setFont(currentFont);
    ui->currentMixLabel->setFont(currentFont);

    int newCurrentWidth = static_cast<int>(scaleWidth1 * currentFontPointSize);
    ui->currentTempoLabel->setFixedWidth(newCurrentWidth);
    ui->currentPitchLabel->setFixedWidth(newCurrentWidth);
    ui->currentVolumeLabel->setFixedWidth(newCurrentWidth);
    ui->currentMixLabel->setFixedWidth(newCurrentWidth);

    ui->statusBar->setFont(currentFont);
    micStatusLabel->setFont(currentFont);

    ui->currentLocLabel->setFont(currentFont);
    ui->songLengthLabel->setFont(currentFont);

    ui->currentLocLabel->setFixedWidth(static_cast<int>(scaleWidth2 * currentFontPointSize));
    ui->songLengthLabel->setFixedWidth(static_cast<int>(scaleWidth2 * currentFontPointSize));

    ui->clearSearchButton->setFont(currentFont);
    ui->clearSearchButton->setFixedWidth(static_cast<int>(scaleWidth3 * currentFontPointSize));

    ui->tabWidget->setFont(currentFont);  // most everything inherits from this one

    ui->pushButtonCueSheetEditTitle->setFont(currentFont);
    ui->pushButtonCueSheetEditLabel->setFont(currentFont);
    ui->pushButtonCueSheetEditArtist->setFont(currentFont);
    ui->pushButtonCueSheetEditHeader->setFont(currentFont);
    ui->pushButtonCueSheetEditLyrics->setFont(currentFont);

    ui->pushButtonEditLyrics->setFont(currentFont);
    ui->pushButtonCueSheetEditSave->setFont(currentFont);
    ui->pushButtonCueSheetEditSaveAs->setFont(currentFont);

    ui->pushButtonCueSheetEditBold->setFont(currentFont);
    ui->pushButtonCueSheetEditItalic->setFont(currentFont);

    ui->pushButtonClearTaughtCalls->setFont(currentFont);
    ui->pushButtonClearTaughtCalls->setFixedWidth(static_cast<int>(TitleButtonWidth[index] * 1.5));

    ui->pushButtonCueSheetEditTitle->setFixedWidth(static_cast<int>(TitleButtonWidth[index]));
    ui->pushButtonCueSheetEditLabel->setFixedWidth(static_cast<int>(TitleButtonWidth[index]));
    ui->pushButtonCueSheetEditArtist->setFixedWidth(static_cast<int>(TitleButtonWidth[index]));
    ui->pushButtonCueSheetEditHeader->setFixedWidth(static_cast<int>(TitleButtonWidth[index] * 1.5));
    ui->pushButtonCueSheetEditLyrics->setFixedWidth(static_cast<int>(TitleButtonWidth[index]));

    ui->pushButtonCueSheetClearFormatting->setFixedWidth(static_cast<int>(TitleButtonWidth[index] * 2.25));

    ui->tableWidgetCallList->horizontalHeader()->setFont(currentFont);
    ui->songTable->horizontalHeader()->setFont(currentFont);
    ui->songTable->horizontalHeader()->setFixedHeight(searchBoxesHeight[index]);
//    qDebug() << "setting font to: " << currentFont;

    ui->tableWidgetCallList->setColumnWidth(kCallListOrderCol,static_cast<int>(67*(currentMacPointSize/13.0)));
    ui->tableWidgetCallList->setColumnWidth(kCallListCheckedCol, static_cast<int>(34*(currentMacPointSize/13.0)));
    ui->tableWidgetCallList->setColumnWidth(kCallListWhenCheckedCol, static_cast<int>(100*(currentMacPointSize/13.0)));
    ui->tableWidgetCallList->setColumnWidth(kCallListTimingCol, static_cast<int>(200*(currentMacPointSize/13.0)));

    // these are special -- don't want them to get too big, even if user requests huge fonts
    currentFont.setPointSize(currentFontPointSize > maxEQsize ? static_cast<int>(maxEQsize) : currentFontPointSize);  // no bigger than 20pt
    ui->bassLabel->setFont(currentFont);
    ui->midrangeLabel->setFont(currentFont);
    ui->trebleLabel->setFont(currentFont);
    ui->EQgroup->setFont(currentFont);

    // resize the icons for the buttons
    int newIconDimension = static_cast<int>(currentFontPointSize * scaleIcons);
    QSize newIconSize(newIconDimension, newIconDimension);
    ui->stopButton->setIconSize(newIconSize);
    ui->playButton->setIconSize(newIconSize);
    ui->previousSongButton->setIconSize(newIconSize);
    ui->nextSongButton->setIconSize(newIconSize);

    //ui->pushButtonEditLyrics->setIconSize(newIconSize);

    // these are special MEDIUM
    int warningLabelFontSize = warningLabelSize[index]; // keep ratio constant
    currentFont.setPointSize(warningLabelFontSize);
    ui->warningLabel->setFont(currentFont);
    ui->warningLabelCuesheet->setFont(currentFont);

    ui->warningLabel->setFixedWidth(warningLabelWidth[index]);
    ui->warningLabelCuesheet->setFixedWidth(warningLabelWidth[index]);

    // these are special BIG
    int nowPlayingLabelFontSize = (nowPlayingSize[index]); // keep ratio constant
    currentFont.setPointSize(nowPlayingLabelFontSize);
    ui->nowPlayingLabel->setFont(currentFont);
    ui->nowPlayingLabel->setFixedHeight(static_cast<int>(nowPlayingHeightFactor * nowPlayingLabelFontSize));

    // BUTTON SIZES ---------
    ui->stopButton->setFixedSize(static_cast<int>(buttonSizeH*nowPlayingLabelFontSize), static_cast<int>(buttonSizeV*nowPlayingLabelFontSize));
    ui->playButton->setFixedSize(static_cast<int>(buttonSizeH*nowPlayingLabelFontSize), static_cast<int>(buttonSizeV*nowPlayingLabelFontSize));
    ui->previousSongButton->setFixedSize(static_cast<int>(buttonSizeH*nowPlayingLabelFontSize), static_cast<int>(buttonSizeV*nowPlayingLabelFontSize));
    ui->nextSongButton->setFixedSize(static_cast<int>(buttonSizeH*nowPlayingLabelFontSize), static_cast<int>(buttonSizeV*nowPlayingLabelFontSize));
}


void MainWindow::usePersistentFontSize() {
    PerfTimer t("usePersistentFontSize", __LINE__);
    int newPointSize = prefsManager.GetsongTableFontSize(); // gets the persisted value
    if (newPointSize == 0) {
        newPointSize = 13;  // default backstop, if not set properly
    }

    t.elapsed(__LINE__);

    // ensure it is a reasonable size...
    newPointSize = (newPointSize > BIGGESTZOOM ? BIGGESTZOOM : newPointSize);
    newPointSize = (newPointSize < SMALLESTZOOM ? SMALLESTZOOM : newPointSize);

    //qDebug() << "usePersistentFontSize: " << newPointSize;

    t.elapsed(__LINE__);

    QFont currentFont = ui->songTable->font();  // set the font size in the songTable
//    qDebug() << "index: " << pointSizeToIndex(newPointSize);
    int platformPS = indexToPointSize(pointSizeToIndex(newPointSize));  // convert to PLATFORM pointsize
//    qDebug() << "platformPS: " << platformPS;
    currentFont.setPointSize(platformPS);
    ui->songTable->setFont(currentFont);
    currentMacPointSize = newPointSize;

    t.elapsed(__LINE__);

    ui->songTable->setStyleSheet(QString("QTableWidget::item:selected{ color: #FFFFFF; background-color: #4C82FC } QHeaderView::section { font-size: %1pt; }").arg(platformPS));

    t.elapsed(__LINE__);

    setSongTableFont(ui->songTable, currentFont);
    t.elapsed(__LINE__);

    adjustFontSizes();  // use that font size to scale everything else (relative)
    t.elapsed(__LINE__);
}


void MainWindow::persistNewFontSize(int currentMacPointSize) {
//    qDebug() << "NOT PERSISTING: " << currentMacPointSize;
//    return;
    prefsManager.SetsongTableFontSize(currentMacPointSize);  // persist this
//    qDebug() << "persisting new Mac font size: " << currentMacPointSize;
}

void MainWindow::zoomInOut(int increment) {
    QFont currentFont = ui->songTable->font();
    int newPointSize = currentMacPointSize + increment;

    newPointSize = (newPointSize > BIGGESTZOOM ? BIGGESTZOOM : newPointSize);
    newPointSize = (newPointSize < SMALLESTZOOM ? SMALLESTZOOM : newPointSize);

    if (newPointSize > currentMacPointSize) {
        ui->textBrowserCueSheet->zoomIn(2*ZOOMINCREMENT);
        totalZoom += 2*ZOOMINCREMENT;
    }

    int platformPS = indexToPointSize(pointSizeToIndex(newPointSize));  // convert to PLATFORM pointsize
    currentFont.setPointSize(platformPS);
    ui->songTable->setFont(currentFont);
    currentMacPointSize = newPointSize;

    persistNewFontSize(currentMacPointSize);

    ui->songTable->setStyleSheet(QString("QTableWidget::item:selected{ color: #FFFFFF; background-color: #4C82FC } QHeaderView::section { font-size: %1pt; }").arg(platformPS));

    setSongTableFont(ui->songTable, currentFont);
    adjustFontSizes();
//    qDebug() << "currentMacPointSize:" << newPointSize << ", totalZoom:" << totalZoom;
}

void MainWindow::on_actionZoom_In_triggered()
{
    zoomInOut(ZOOMINCREMENT);
}

void MainWindow::on_actionZoom_Out_triggered()
{
    zoomInOut(-ZOOMINCREMENT);
}

void MainWindow::on_actionReset_triggered()
{
    QFont currentFont;  // system font, and system default point size

    currentMacPointSize = 13; // by definition
    int platformPS = indexToPointSize(pointSizeToIndex(currentMacPointSize));  // convert to PLATFORM pointsize

    currentFont.setPointSize(platformPS);
    ui->songTable->setFont(currentFont);

    ui->textBrowserCueSheet->zoomOut(totalZoom);  // undo all zooming in the lyrics pane
    totalZoom = 0;

    persistNewFontSize(currentMacPointSize);
    setSongTableFont(ui->songTable, currentFont);
    adjustFontSizes();
//    qDebug() << "currentMacPointSize:" << currentMacPointSize << ", totalZoom:" << totalZoom;
}

void MainWindow::on_actionViewTags_toggled(bool /* checked */)
{
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
    ui->songTable->hide();
    ui->songTable->setSortingEnabled(false);
}

void MainWindow::stopLongSongTableOperation(QString s) {
    Q_UNUSED(s)

    ui->songTable->setSortingEnabled(true);
    ui->songTable->show();
}


void MainWindow::on_actionDownload_Cuesheets_triggered()
{
    fetchListOfCuesheetsFromCloud();
//  cuesheetListDownloadEnd();  // <-- will be called, when the fetch from the Cloud is complete

//#if defined(Q_OS_MAC) | defined(Q_OS_WIN)

//    QString musicDirPath = prefsManager.GetmusicPath();
//    QString lyricsDirPath = musicDirPath + "/lyrics";

//    QDir lyricsDir(lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME);

//    if (lyricsDir.exists()) {
////        qDebug() << "You already have the latest cuesheets downloaded.  Are you sure you want to download them again (erasing any edits you made)?";

//        QMessageBox msgBox;
//        msgBox.setIcon(QMessageBox::Warning);
//        msgBox.setText(QString("You already have the latest lyrics files: '") + CURRENTSQVIEWLYRICSNAME + "'");
//        msgBox.setInformativeText("Are you sure?  This will overwrite any lyrics that you have edited in that folder.");
//        QPushButton *downloadButton = msgBox.addButton(tr("Download Anyway"), QMessageBox::AcceptRole);
//        QPushButton *abortButton = msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
//        msgBox.exec();

//        if (msgBox.clickedButton() == downloadButton) {
//            // Download
////            qDebug() << "DOWNLOAD WILL PROCEED NORMALLY.";
//        } else if (msgBox.clickedButton() == abortButton) {
//            // Abort
////            qDebug() << "ABORTING DOWNLOAD.";
//            return;
//        }
//    }

//    Downloader *d = new Downloader(this);

//    QUrl lyricsZipFileURL(QString("https://raw.githubusercontent.com/mpogue2/SquareDesk/master/") + CURRENTSQVIEWLYRICSNAME + QString(".zip"));  // FIX: hard-coded for now
////    qDebug() << "url to download:" << lyricsZipFileURL.toDisplayString();

//    QString lyricsZipFileName = lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME + ".zip";

//    d->doDownload(lyricsZipFileURL, lyricsZipFileName);  // download URL and put it into lyricsZipFileName

//    QObject::connect(d,SIGNAL(downloadFinished()), this, SLOT(lyricsDownloadEnd()));
//    QObject::connect(d,SIGNAL(downloadFinished()), d, SLOT(deleteLater()));

//    // PROGRESS BAR ---------------------
//    // assume ~10MB
//    progressDialog = new QProgressDialog("Downloading lyrics...", "Cancel Download", 0, 100, this);
//    connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelProgress()));
//    progressTimer = new QTimer(this);
//    connect(progressTimer, SIGNAL(timeout()), this, SLOT(makeProgress()));

//    progressOffset = 0.0;
//    progressTotal = 0.0;
//    progressTimer->start(500);  // twice per second
//#endif
}

void MainWindow::makeProgress() {
#if defined(Q_OS_MAC) | defined(Q_OS_WIN)
    if (progressTotal < 70) {
        progressTotal += 7.0;
    } else if (progressTotal < 90) {
        progressTotal += 2.0;
    } else if (progressTotal < 98) {
        progressTotal += 0.5;
    } // else no progress for you.

//    qDebug() << "making progress..." << progressOffset << "," << progressTotal;

    progressDialog->setValue(static_cast<int>(progressOffset + 33.0*(progressTotal/100.0)));
#endif
}

void MainWindow::cancelProgress() {
#if defined(Q_OS_MAC) | defined(Q_OS_WIN)
//    qDebug() << "cancelling progress...";
    progressTimer->stop();
    progressTotal = 0;
    progressOffset = 0;
    progressCancelled = true;
#endif
}


void MainWindow::lyricsDownloadEnd() {
//#if defined(Q_OS_MAC) | defined(Q_OS_WIN)
////    qDebug() << "MainWindow::lyricsDownloadEnd() -- Download done:";

////    qDebug() << "UNPACKING ZIP FILE INTO LYRICS DIRECTORY...";
//    QString musicDirPath = prefsManager.GetmusicPath();
//    QString lyricsDirPath = musicDirPath + "/lyrics";
//    QString lyricsZipFileName = lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME + ".zip";

//    QString destinationDir = lyricsDirPath;

//    // extract the ZIP file
//    QStringList extracted = JlCompress::extractDir(lyricsZipFileName, destinationDir); // extracts /root/lyrics/SqView_xxxxxx.zip to /root/lyrics/Text

//    if (extracted.empty()) {
////        qDebug() << "There was a problem extracting the files.  No files extracted.";
//        progressDialog->setValue(100);  // kill the progress bar
//        progressTimer->stop();
//        progressOffset = 0;
//        progressTotal = 0;
//        progressCancelled = false;

//        QMessageBox msgBox;
//        msgBox.setText("The lyrics have been downloaded to <musicDirectory>/lyrics, but you need to manually unpack them.\nWindows: Right click, Extract All on: " +
//                       lyricsZipFileName);
//        msgBox.exec();
//        return;  // and don't delete the ZIP file, for debugging
//    }

////    qDebug() << "DELETING ZIP FILE...";
//    QFile file(lyricsZipFileName);
//    file.remove();

//    QDir currentLyricsDir(lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME);
//    if (currentLyricsDir.exists()) {
////        qDebug() << "Refused to overwrite existing cuesheets, renamed to: " << lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME + "_backup";
//        QFile::rename(lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME, lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME + "_backup");
//    }

////    qDebug() << "RENAMING Text/ TO SqViewCueSheets_2017.03.14/ ...";
//    QFile::rename(lyricsDirPath + "/Text", lyricsDirPath + "/" + CURRENTSQVIEWLYRICSNAME);

//    // RESCAN THE ENTIRE MUSIC DIRECTORY FOR LYRICS ------------
//    findMusic(musicRootPath,"","main", true);  // get the filenames from the user's directories
//    loadMusicList(); // and filter them into the songTable

////    qDebug() << "DONE DOWNLOADING LATEST LYRICS: " << CURRENTSQVIEWLYRICSNAME << "\n";

//    progressDialog->setValue(100);  // kill the progress bar
//    progressTimer->stop();
//    progressOffset = 0;
//    progressTotal = 0;

//#endif
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
    if (ui->tabWidget->tabText(i).endsWith("Lyrics") || ui->tabWidget->tabText(i).endsWith("Patter")) {
        if (ui->tabWidget->tabText(i).endsWith("Lyrics")) {
            printDialog.setWindowTitle("Print Cuesheet");
        } else {
            printDialog.setWindowTitle("Print Patter");
        }

        if (printDialog.exec() == QDialog::Rejected) {
            return;
        }

        ui->textBrowserCueSheet->print(&printer);
    } else if (ui->tabWidget->tabText(i).endsWith("SD")) {
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
        QPrinter printer;
        QPrintDialog printDialog(&printer, this);
        printDialog.setWindowTitle("Print Playlist");

        if (printDialog.exec() == QDialog::Rejected) {
            return;
        }

        QPainter painter;

        painter.begin(&printer);

        QFont font = painter.font();
        font.setPixelSize(14);
        painter.setFont(font);

        // --------
        QList<PlaylistExportRecord> exports;

        // Iterate over the songTable to get all the info about the playlist
        for (int i=0; i<ui->songTable->rowCount(); i++) {
            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
            QString playlistIndex = theItem->text();
            QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
            QString songTitle = getTitleColTitle(ui->songTable, i);
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

//        qSort(exports.begin(), exports.end(), comparePlaylistExportRecord);  // they are now IN INDEX ORDER
        std::sort(exports.begin(), exports.end(), comparePlaylistExportRecord);  // they are now IN INDEX ORDER

        QString toBePrinted = "PLAYLIST\n\n";

        char buf[128];
        // list is sorted here, in INDEX order
        foreach (const PlaylistExportRecord &rec, exports)
        {
            QString baseName = rec.title;
            baseName.replace(QRegularExpression("^" + musicRootPath),"");  // delete musicRootPath at beginning of string

            sprintf(buf, "%02d: %s\n", rec.index, baseName.toLatin1().data());
            toBePrinted += buf;
        }

        //        painter.drawText(20, 20, 500, 500, Qt::AlignLeft|Qt::AlignTop, "Hello world!\nMore lines\n");
        painter.drawText(20,20,500,500, Qt::AlignLeft|Qt::AlignTop, toBePrinted);

        painter.end();
    }
}

void MainWindow::saveSequenceAs()
{
    // Ask me where to save it...
    RecursionGuard dialog_guard(inPreferencesDialog);

    QString sequenceFilename = QFileDialog::getSaveFileName(this,
                                                    tr("Save SD Sequence"),
                                                    musicRootPath + "/sd/sequence.txt",
                                                    tr("TXT (*.txt);;HTML (*.html *.htm)"));
    if (!sequenceFilename.isNull())
    {
        QFile file(sequenceFilename);
        if ( file.open(QIODevice::WriteOnly) )
        {
            QTextStream stream( &file );
            if (sequenceFilename.endsWith(".html", Qt::CaseInsensitive)
                || sequenceFilename.endsWith(".htm", Qt::CaseInsensitive))
            {
                QTextStream stream( &file );
                stream << get_current_sd_sequence_as_html(true, false);
            }
            else
            {
                for (int row = 0; row < ui->tableWidgetCurrentSequence->rowCount();
                     ++row)
                {
                    QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,0);
                    stream << item->text() + "\n";
                }
            }
            stream.flush();
            file.close();
        }
    }
}

void MainWindow::on_actionSave_triggered()
{
//    qDebug() << "actionSave";
    int i = ui->tabWidget->currentIndex();
    if (ui->tabWidget->tabText(i).endsWith("Music Player")) {
        // playlist
        savePlaylistAgain(); // Now a true SAVE (if one was already saved)
    } else if (ui->tabWidget->tabText(i).endsWith("Lyrics") || ui->tabWidget->tabText(i).endsWith("Patter")) {
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
    } else if (ui->tabWidget->tabText(i).endsWith("Lyrics") || ui->tabWidget->tabText(i).endsWith("Patter")) {
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

// ----------------------------------------------------------------------
void MainWindow::on_flashcallbasic_toggled(bool checked)
{
    ui->actionFlashCallBasic->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallbasic(ui->actionFlashCallBasic->isChecked());
}

void MainWindow::on_actionFlashCallBasic_triggered()
{
    on_flashcallbasic_toggled(ui->actionFlashCallBasic->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcallmainstream_toggled(bool checked)
{
    ui->actionFlashCallMainstream->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallmainstream(ui->actionFlashCallMainstream->isChecked());
}

void MainWindow::on_actionFlashCallMainstream_triggered()
{
    on_flashcallmainstream_toggled(ui->actionFlashCallMainstream->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcallplus_toggled(bool checked)
{
    ui->actionFlashCallPlus->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallplus(ui->actionFlashCallPlus->isChecked());
}

void MainWindow::on_actionFlashCallPlus_triggered()
{
    on_flashcallplus_toggled(ui->actionFlashCallPlus->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcalla1_toggled(bool checked)
{
    ui->actionFlashCallA1->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcalla1(ui->actionFlashCallA1->isChecked());
}

void MainWindow::on_actionFlashCallA1_triggered()
{
    on_flashcalla1_toggled(ui->actionFlashCallA1->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcalla2_toggled(bool checked)
{
    ui->actionFlashCallA2->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcalla2(ui->actionFlashCallA2->isChecked());
}

void MainWindow::on_actionFlashCallA2_triggered()
{
    on_flashcalla2_toggled(ui->actionFlashCallA2->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcallc1_toggled(bool checked)
{
    ui->actionFlashCallC1->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallc1(ui->actionFlashCallC1->isChecked());
}

void MainWindow::on_actionFlashCallC1_triggered()
{
    on_flashcallc1_toggled(ui->actionFlashCallC1->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcallc2_toggled(bool checked)
{
    ui->actionFlashCallC2->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallc2(ui->actionFlashCallC2->isChecked());
}

void MainWindow::on_actionFlashCallC2_triggered()
{
    on_flashcallc2_toggled(ui->actionFlashCallC2->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcallc3a_toggled(bool checked)
{
    ui->actionFlashCallC3a->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallc3a(ui->actionFlashCallC3a->isChecked());
}

void MainWindow::on_actionFlashCallC3a_triggered()
{
    on_flashcallc3a_toggled(ui->actionFlashCallC3a->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcallc3b_toggled(bool checked)
{
    ui->actionFlashCallC3b->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallc3b(ui->actionFlashCallC3b->isChecked());
}

void MainWindow::on_actionFlashCallC3b_triggered()
{
    on_flashcallc3b_toggled(ui->actionFlashCallC3b->isChecked());
    readFlashCallsList();
}

// -----
void MainWindow::readFlashCallsList() {
#if defined(Q_OS_MAC)
    QString appPath = QApplication::applicationFilePath();
    QString allcallsPath = appPath + "/Contents/Resources/allcalls.csv";
    allcallsPath.replace("Contents/MacOS/SquareDesk/","");
#elif defined(Q_OS_WIN32)
    // TODO: There has to be a better way to do this.
    QString appPath = QApplication::applicationFilePath();
    QString allcallsPath = appPath + "/allcalls.csv";
    allcallsPath.replace("SquareDesk.exe/","");
#else
    QString allcallsPath = "allcalls.csv";
    // Linux
    QStringList paths;
    paths.append(QApplication::applicationFilePath());
    paths.append("/usr/share/SquareDesk");
    paths.append(".");

    for (auto path : paths)
    {
        QString filename(path + "/allcalls.csv");
        QFileInfo check_file(filename);
        if (check_file.exists() && check_file.isFile())
        {
            allcallsPath = filename;
            break;
        }
    }
#endif

    QFile file(allcallsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open 'allcalls.csv' file. Flash calls will not work.";
        qDebug() << "looked here:" << allcallsPath;
        return;
    }

    flashCalls.clear();  // remove all calls, let's read them in again

    while (!file.atEnd()) {
        QString line = file.readLine().simplified();
        QStringList lineparts = line.split(',');
        QString level = lineparts[0];
        QString call = lineparts[1].replace("\"","");

        if ((level == "basic" && ui->actionFlashCallBasic->isChecked()) ||
                (level == "ms" && ui->actionFlashCallMainstream->isChecked()) ||
                (level == "plus" && ui->actionFlashCallPlus->isChecked()) ||
                (level == "a1" && ui->actionFlashCallA1->isChecked()) ||
                (level == "a2" && ui->actionFlashCallA2->isChecked()) ||
                (level == "c1" && ui->actionFlashCallC1->isChecked()) ||
                (level == "c2" && ui->actionFlashCallC2->isChecked()) ||
                (level == "c3a" && ui->actionFlashCallC3a->isChecked()) ||
                (level == "c3b" && ui->actionFlashCallC3b->isChecked()) ) {
            flashCalls.append(call);
        }
    }
//    qDebug() << "Flash calls" << flashCalls;
//    qsrand(static_cast<uint>(QTime::currentTime().msec()));  // different random sequence of calls each time, please.
    if (flashCalls.length() == 0) {
        randCallIndex = 0;
    } else {
//        randCallIndex = qrand() % flashCalls.length();   // start out with a number other than zero, please.
        randCallIndex = QRandomGenerator::global()->bounded(flashCalls.length());   // start out with a number other than zero, please.
    }

//    qDebug() << "flashCalls: " << flashCalls;
//    qDebug() << "randCallIndex: " << randCallIndex;
}

// -------------------------------------
// LYRICS CUESHEET FETCHING

void MainWindow::fetchListOfCuesheetsFromCloud() {
//    qDebug() << "MainWindow::fetchListOfCuesheetsFromCloud() -- Download is STARTING...";

    // TODO: only fetch if the time is newer than the one we got last time....
    // TODO:    check Expires date.

    QList<QString> cuesheets;

    Downloader *d = new Downloader(this);

    // Apache Directory Listing, because it ends in "/" (~1.1MB uncompressed, ~250KB compressed)
    QUrl cuesheetListURL(QString(CURRENTSQVIEWCUESHEETSDIR));
    QString cuesheetListFilename = musicRootPath + "/.squaredesk/publishedCuesheets.html";

//    qDebug() << "cuesheet URL to download:" << cuesheetListURL.toDisplayString();
//    qDebug() << "             put it here:" << cuesheetListFilename;

    d->doDownload(cuesheetListURL, cuesheetListFilename);  // download URL and put it into cuesheetListFilename

    QObject::connect(d,SIGNAL(downloadFinished()), this, SLOT(cuesheetListDownloadEnd()));
    QObject::connect(d,SIGNAL(downloadFinished()), d, SLOT(deleteLater()));

    // PROGRESS BAR ---------------------
    progressDialog = new QProgressDialog("Downloading list of available cuesheets...\n ", "Cancel", 0, 100, this);
    progressDialog->setMinimumDuration(0);  // start it up right away
    progressCancelled = false;
    progressDialog->setWindowModality(Qt::WindowModal);  // stays until cancelled
    progressDialog->setMinimumWidth(450);  // avoid bug with Cancel button resizing itself

    connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelProgress()));
    connect(progressDialog, SIGNAL(canceled()), d, SLOT(abortTransfer()));      // if user hits CANCEL, abort transfer in progress.
    progressTimer = new QTimer(this);

    connect(progressTimer, SIGNAL(timeout()), this, SLOT(makeProgress()));
    progressOffset = 0.0;
    progressTotal = 0.0;
    progressTimer->start(1000);  // once per second to 33%
}

bool MainWindow::fuzzyMatchFilenameToCuesheetname(QString s1, QString s2) {
//    qDebug() << "trying to match: " << s1 << "," << s2;

// **** EXACT MATCH OF COMPLETE BASENAME (just for testing)
//    QFileInfo fi1(s1);
//    QFileInfo fi2(s2);

//    bool match = fi1.completeBaseName() == fi2.completeBaseName();
//    if (match) {
//        qDebug() << "fuzzy match: " << s1 << "," << s2;
//    }

//    return(match);

// **** OUR FUZZY MATCHING (same as findPossibleCuesheets)

    // SPLIT APART THE MUSIC FILENAME --------
    QFileInfo mp3FileInfo(s1);
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

    // SPLIT APART THE CUESHEET FILENAME --------
    QFileInfo fi(s2);
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

    // NOW SCORE IT ----------------------------
    int score = 0;
//    qDebug() << "label: " << label;
    if (completeBaseName.compare(mp3CompleteBaseName, Qt::CaseInsensitive) == 0         // exact match: entire filename
        || title.compare(mp3Title, Qt::CaseInsensitive) == 0                            // exact match: title (without label/labelNum)
        || (shortTitle.length() > 0                                                     // exact match: shortTitle
            && shortTitle.compare(mp3ShortTitle, Qt::CaseInsensitive) == 0)
        || (labelnum_short.length() > 0 && label.length() > 0                           // exact match: shortLabel + shortLabelNumber
            &&  labelnum_short.compare(mp3Labelnum_short, Qt::CaseInsensitive) == 0
            && label.compare(mp3Label, Qt::CaseInsensitive) == 0
            )
        || (labelnum.length() > 0 && label.length() > 0
            && mp3Title.length() > 0
            && mp3Title.compare(label + "-" + labelnum, Qt::CaseInsensitive) == 0)
        )
    {
        // Minimum criteria (we will accept as a match, without looking at sorted words):
//        qDebug() << "fuzzy match (meets minimum criteria): " << s1 << "," << s2;
        return(true);
    } else if ((score = compareSortedWordListsForRelevance(mp3Words, words)) > 0)
    {
        // fuzzy match, using the sorted words in the titles
//        qDebug() << "fuzzy match (meets sorted words criteria): " << s1 << "," << s2;
        return(true);
    }

    return(false);
}

void MainWindow::cuesheetListDownloadEnd() {

    if (progressDialog->wasCanceled()) {
        return;
    }

//    qDebug() << "MainWindow::cuesheetListDownloadEnd() -- Download is DONE";

    qApp->processEvents();  // allow the progress bar to move
//    qDebug() << "MainWindow::cuesheetListDownloadEnd() -- Making list of music files...";
    QList<QString> musicFiles = getListOfMusicFiles();

    qApp->processEvents();  // allow the progress bar to move
//    qDebug() << "MainWindow::cuesheetListDownloadEnd() -- Making list of cuesheet files...";
    QList<QString> cuesheetsInCloud = getListOfCuesheets();

    progressOffset = 33;
    progressTotal = 0;
    progressDialog->setValue(33);
//    progressDialog->setLabelText("Matching your music with cuesheets...");
    progressTimer->stop();

//    qDebug() << "MainWindow::cuesheetListDownloadEnd() -- Matching them up...";

//    qDebug() << "***** Here's the list of musicFiles:" << musicFiles;
//    qDebug() << "***** Here's the list of cuesheets:" << cuesheetsInCloud;

    double numCuesheets = cuesheetsInCloud.length();
    double numChecked = 0;

    // match up the music filenames against the cuesheets that the Cloud has
    QList<QString> maybeFilesToDownload;

    QList<QString>::iterator i;
    QList<QString>::iterator j;
    for (j = cuesheetsInCloud.begin(); j != cuesheetsInCloud.end(); ++j) {
        // should we download this Cloud cuesheet file?

        if ( static_cast<unsigned int>(numChecked) % 50 == 0 ) {
            progressDialog->setLabelText("Found matching cuesheets: " +
                                         QString::number(static_cast<unsigned int>(maybeFilesToDownload.length())) +
                                         " out of " + QString::number(static_cast<unsigned int>(numChecked)) +
                                         "\n" + (maybeFilesToDownload.length() > 0 ? maybeFilesToDownload.last() : "")
                                         );
            progressDialog->setValue(static_cast<int>(33 + 33.0*(numChecked/numCuesheets)));
            qApp->processEvents();  // allow the progress bar to move every 100 checks
            if (progressDialog->wasCanceled()) {
                return;
            }
        }

        numChecked++;

        for (i = musicFiles.begin(); i != musicFiles.end(); ++i) {
            if (fuzzyMatchFilenameToCuesheetname(*i, *j)) {
                // yes, let's download it, if we don't have it already.
                maybeFilesToDownload.append(*j);
//                qDebug() << "Will maybe download: " << *j;
                break;  // once we've decided to download this file, go on and look at the NEXT cuesheet
            }
        }
    }

    progressDialog->setValue(66);

//    qDebug() << "MainWindow::cuesheetListDownloadEnd() -- Maybe downloading " << maybeFilesToDownload.length() << " files";
//  qDebug() << "***** Maybe downloading " << maybeFilesToDownload.length() << " files.";

    double numDownloads = maybeFilesToDownload.length();
    double numDownloaded = 0;

    // download them (if we don't have them already)
    QList<QString>::iterator k;
    for (k = maybeFilesToDownload.begin(); k != maybeFilesToDownload.end(); ++k) {
        progressDialog->setLabelText("Downloading matching cuesheets (if needed): " +
                                     QString::number(static_cast<unsigned int>(numDownloaded++)) +
                                     " out of " +
                                     QString::number(static_cast<unsigned int>(numDownloads)) +
                                     "\n" +
                                     *k
                                     );
        progressDialog->setValue(static_cast<int>((66 + 33.0*(numDownloaded/numDownloads))));
        qApp->processEvents();  // allow the progress bar to move constantly
        if (progressDialog->wasCanceled()) {
            break;
        }

        downloadCuesheetFileIfNeeded(*k);
    }

// qDebug() << "MainWindow::cuesheetListDownloadEnd() -- DONE.  All cuesheets we didn't have are downloaded.";

    progressDialog->setLabelText("Done.");
    progressDialog->setValue(100);  // kill the progress bar
    progressTimer->stop();
    progressOffset = 0;
    progressTotal = 0;

    // FINALLY, RESCAN THE ENTIRE MUSIC DIRECTORY FOR SONGS AND LYRICS ------------
    maybeLyricsChanged();

//    findMusic(musicRootPath,"","main", true);  // get the filenames from the user's directories
//    loadMusicList(); // and filter them into the songTable

//    reloadCurrentMP3File(); // in case the list of matching cuesheets changed by the recent addition of cuesheets
}

void MainWindow::downloadCuesheetFileIfNeeded(QString cuesheetFilename) {

//    qDebug() << "Maybe fetching: " << cuesheetFilename;
//    cout << ".";

    QString musicDirPath = prefsManager.GetmusicPath();
    //    QString tempDirPath = "/Users/mpogue/clean4";
    QString destinationFolder = musicDirPath + "/lyrics/downloaded/";

    QDir dir(musicDirPath);
    dir.mkpath("lyrics/downloaded");    // make sure that the destination path exists (including intermediates)

    QFile file(destinationFolder + cuesheetFilename);
    QFileInfo fileinfo(file);

    // if we don't already have it...
    if (!fileinfo.exists()) {
        // ***** SYNCHRONOUS FETCH *****
        // "http://squaredesk.net/cuesheets/SqViewCueSheets_2017.03.14/"
        QNetworkAccessManager *networkMgr = new QNetworkAccessManager(this);
        QString URLtoFetch = CURRENTSQVIEWCUESHEETSDIR + cuesheetFilename; // individual files (~10KB)
        QNetworkReply *reply = networkMgr->get( QNetworkRequest( QUrl(URLtoFetch) ) );

        QEventLoop loop;
        QObject::connect(reply, SIGNAL(readyRead()), &loop, SLOT(quit()));

//        qDebug() << "Fetching file we don't have: " << URLtoFetch;
        // Execute the event loop here, now we will wait here until readyRead() signal is emitted
        // which in turn will trigger event loop quit.
        loop.exec();

        QString resultString(reply->readAll());  // only fetch this once!
        // qDebug() << "result:" << resultString;

        // OK, we have the file now...
        if (resultString.length() == 0) {
            qDebug() << "ERROR: file we got was zero length.";
            return;
        }

//        qDebug() << "***** WRITING TO: " << destinationFolder + cuesheetFilename;
        // let's try to write it
        if ( file.open(QIODevice::WriteOnly) )
        {
            QTextStream stream( &file );
            stream << resultString;
            stream.flush();
            file.close();
        } else {
            qDebug() << "ERROR: couldn't open the file for writing...";
        }
    } else {
//        qDebug() << "     Not fetching it, because we already have it.";
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

QList<QString> MainWindow::getListOfCuesheets() {

    QList<QString> list;

    QString cuesheetListFilename = musicRootPath + "/.squaredesk/publishedCuesheets.html";
    QFile inputFile(cuesheetListFilename)
            ;
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
        int line_number = 0;
        while (!in.atEnd()) //  && line_number < 10)
        {
            line_number++;
            QString line = in.readLine();

            // <li><a href="RR%20147%20-%20Amarillo%20By%20Morning.html"> RR 147 - Amarillo By Morning.html</a></li>

            QRegularExpression regex_cuesheetName("^<li><a href=\"(.*?)\">(.*)</a></li>$"); // don't be greedy!
            QRegularExpressionMatch match = regex_cuesheetName.match(line);
//            qDebug() << "line: " << line;
            if (match.hasMatch())
            {
                QString cuesheetFilename(match.captured(2).trimmed());
//                qDebug() << "****** Cloud has cuesheet: " << cuesheetFilename << " *****";

                list.append(cuesheetFilename);
//                downloadCuesheetFileIfNeeded(cuesheetFilename, &musicFiles);
            }
        }
        inputFile.close();
        } else {
            qDebug() << "ERROR: could not open " << cuesheetListFilename;
        }

    return(list);
}

void MainWindow::maybeLyricsChanged() {
//    qDebug() << "maybeLyricsChanged()";
    // AND, just in case the list of matching cuesheets for the current song has been
    //   changed by the recent addition of cuesheets...
//    if (!filewatcherShouldIgnoreOneFileSave) {
//        // don't rescan, if this is a SAVE or SAVE AS Lyrics (those get added manually to the pathStack)
//        // RESCAN THE ENTIRE MUSIC DIRECTORY FOR LYRICS FILES (and music files that might match) ------------
//        findMusic(musicRootPath,"","main", true);  // get the filenames from the user's directories
//        loadMusicList(); // and filter them into the songTable

//        // reload only if this isn't a SAVE LYRICS FILE
////        reloadCurrentMP3File();
//    }
    filewatcherShouldIgnoreOneFileSave = false;
}

void MainWindow::on_actionTest_Loop_triggered()
{
//    qDebug() << "Test Loop Intro: " << ui->seekBar->GetIntro() << ", outro: " << ui->seekBar->GetOutro();

    if (!songLoaded) {
        return;  // if there is no song loaded, no point in doing anything.
    }

    cBass.Stop();  // always pause playback

    double songLength = cBass.FileLength;
//    double intro = ui->seekBar->GetIntro(); // 0.0 - 1.0
    double outro = ui->seekBar->GetOutro(); // 0.0 - 1.0

    double startPosition_sec = fmax(0.0, songLength*outro - 5.0);

    cBass.StreamSetPosition(startPosition_sec);
    Info_Seekbar(false);  // update just the text

//    on_playButton_clicked();  // play, starting 5 seconds before the loop

    cBass.Play();  // currently paused, so start playing at new location
}

void MainWindow::on_dateTimeEditIntroTime_timeChanged(const QTime &time)
{
//    qDebug() << "newIntroTime: " << time;

    double position_sec = 60*time.minute() + time.second() + time.msec()/1000.0;
    double length = cBass.FileLength;
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
    double length = cBass.FileLength;
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


void MainWindow::on_actionBold_triggered()
{
    bool isBoldNow = ui->pushButtonCueSheetEditBold->isChecked();
    bool isEditable = ui->pushButtonEditLyrics->isChecked();
    if (isEditable) {
        ui->textBrowserCueSheet->setFontWeight(isBoldNow ? QFont::Normal : QFont::Bold);
    }
}

void MainWindow::on_actionItalic_triggered()
{
    bool isItalicsNow = ui->pushButtonCueSheetEditItalic->isChecked();
    bool isEditable = ui->pushButtonEditLyrics->isChecked();
    if (isEditable) {
        ui->textBrowserCueSheet->setFontItalic(!isItalicsNow);
    }
}

// --------------------------
QString MainWindow::logFilePath;  // static members must be explicitly instantiated if in class scope

void MainWindow::customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QHash<QtMsgType, QString> msgLevelHash({{QtDebugMsg, "Debug"}, {QtInfoMsg, "Info"}, {QtWarningMsg, "Warning"}, {QtCriticalMsg, "Critical"}, {QtFatalMsg, "Fatal"}});
    QByteArray localMsg = msg.toLocal8Bit();

    QString dateTime = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);  // use ISO8601 UTC timestamps
    QString logLevelName = msgLevelHash[type];
    QString txt = QString("%1 %2: %3 (%4)").arg(dateTime, logLevelName, msg, context.file);

    if (msg.contains("The provided value 'moz-chunked-arraybuffer' is not a valid enum value of type XMLHttpRequestResponseType")) {
//         This is a known warning that is spit out once per loaded PDF file.  It's just noise, so suppressing it from the debug.log file.
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

// ----------------------------------------------------------------------
void MainWindow::on_action5_seconds_triggered()
{
    prefsManager.Setflashcalltiming("5");
}

void MainWindow::on_action10_seconds_triggered()
{
    prefsManager.Setflashcalltiming("10");
}

void MainWindow::on_action15_seconds_triggered()
{
    prefsManager.Setflashcalltiming("15");
}

void MainWindow::on_action20_seconds_triggered()
{
    prefsManager.Setflashcalltiming("20");
}

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

// ----------------------------------------------------------------------
void MainWindow::on_actionShow_group_station_toggled(bool showGroupStation)
{
//    Q_UNUSED(showGroupStation)
//    qDebug() << "TOGGLED: " << showGroupStation;
    ui->actionShow_group_station->setChecked(showGroupStation); // when called from constructor
    prefsManager.Setenablegroupstation(showGroupStation);  // persistent menu item
    on_sd_update_status_bar(sdLastFormationName);  // refresh SD graphical display
}

void MainWindow::on_actionShow_order_sequence_toggled(bool showOrderSequence)
{
//    Q_UNUSED(showOrderSequence)
//    qDebug() << "TOGGLED ORDER SEQUENCE: " << showOrderSequence;
    ui->actionShow_order_sequence->setChecked(showOrderSequence); // when called from constructor
    prefsManager.Setenableordersequence(showOrderSequence);  // persistent menu item
    on_sd_update_status_bar(sdLastFormationName);  // refresh SD graphical display
}

void MainWindow::customLyricsMenuRequested(QPoint pos) {
    Q_UNUSED(pos)

    if (loadedCuesheetNameWithPath != "" && !loadedCuesheetNameWithPath.contains(".template.html")) {
        // context menu is available, only if we have loaded a cuesheet
        QMenu *menu = new QMenu(this);
#if defined(Q_OS_MAC)
        menu->addAction( "Reveal in Finder" , this , SLOT (revealLyricsFileInFinder()) );
#endif

#if defined(Q_OS_WIN)
        menu->addAction( "Show in Explorer" , this , SLOT (revealLyricsFileInFinder()) );
#endif

#if defined(Q_OS_LINUX)
        menu->addAction( "Open containing folder" , this , SLOT (revealLyricsFileInFinder()) );
#endif

        menu->popup(QCursor::pos());
        menu->exec();

        delete(menu);
    }
}

void MainWindow::revealLyricsFileInFinder() {
//    qDebug() << "path: " << loadedCuesheetNameWithPath;
    showInFinderOrExplorer(loadedCuesheetNameWithPath);
}

void MainWindow::handleDurationBPM() {
//    qDebug() << "***** handleDurationBPM()";
    int length_sec = static_cast<int>(cBass.FileLength);
    int songBPM = static_cast<int>(round(cBass.Stream_BPM));  // libbass's idea of the BPM

    bool isSingingCall = songTypeNamesForSinging.contains(currentSongType) ||
                         songTypeNamesForCalled.contains(currentSongType);

    bool isPatter = songTypeNamesForPatter.contains(currentSongType);

// If the MP3 file has an embedded TBPM frame in the ID3 tag, then it overrides the libbass auto-detect of BPM
//    double songBPM_ID3 = getID3BPM(MP3FileName);  // returns 0.0, if not found or not understandable

//    if (songBPM_ID3 != 0.0) {
//        songBPM = static_cast<int>(songBPM_ID3);
//        tempoIsBPM = true;  // this song's tempo is BPM, not %
//    }

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

        ui->tempoSlider->setValue(songBPM);
        ui->tempoSlider->valueChanged(songBPM);  // fixes bug where second song with same BPM doesn't update songtable::tempo

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
        ui->tempoSlider->valueChanged(100);  // fixes bug where second song with same 100% doesn't update songtable::tempo
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
