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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>

#include <QActionGroup>
#include <QDebug>
#include <QMainWindow>
#include <QTimer>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QList>
#include <QListIterator>
#include <QListWidget>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QProcess>
#include <QProgressDialog>
#include <QProxyStyle>
#include <QSettings>
#include <QSlider>
#include <QStack>
#include <QTableWidgetItem>
#include <QToolTip>
#include <QVariant>
#include <QWheelEvent>
#include <QWidget>
#include <QFileSystemWatcher>

#include <QDateTime>

#include "common_enums.h"
#include "sdhighlighter.h"

#include "math.h"
#include "bass_audio.h"
#include "myslider.h"
#include "preferencesdialog.h"
#include "levelmeter.h"
#include "analogclock.h"
#include "console.h"
#include "renderarea.h"
#include "songsettings.h"

#if defined(Q_OS_MAC)
#include "macUtils.h"
#include <stdio.h>
#include <errno.h>
#endif
#include <tidy/tidy.h>
#include <tidy/tidybuffio.h>

// REMEMBER TO CHANGE THIS WHEN WE RELEASE A NEW VERSION.
//  Also remember to change the "latest" file on GitHub!

#define VERSIONSTRING "0.8.2alpha7"

#define CURRENTSQVIEWCUESHEETSDIR "https://squaredesk.net/cuesheets/SqViewCueSheets_2017.03.14/"
#define CURRENTSQVIEWLYRICSNAME "SqViewCueSheets_2017.03.14"

namespace Ui
{
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    Ui::MainWindow *ui;
    bool handleKeypress(int key, QString text);

    Console *console;                   // these are public so that eventFilter can browse them
    QTextEdit *currentSequenceWidget;

    PreferencesDialog *prefDialog;
    QActionGroup *sessionActionGroup;
    QAction **sessionActions;
    QActionGroup *sdActionGroup1;
    QActionGroup *sdActionGroup2;

    void checkLockFile();
    void clearLockFile();

    QStringList parseCSV(const QString &string);
    QString tidyHTML(QString s);  // return the tidied HTML
    QString postProcessHTMLtoSemanticHTML(QString cuesheet);

    void readFlashCallsList();  // re-read the flashCalls file, keep just those selected

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    void on_loopButton_toggled(bool checked);
    void on_monoButton_toggled(bool checked);

    void on_flashcallbasic_toggled(bool checked);
    void on_flashcallmainstream_toggled(bool checked);
    void on_flashcallplus_toggled(bool checked);
    void on_flashcalla1_toggled(bool checked);
    void on_flashcalla2_toggled(bool checked);
    void on_flashcallc1_toggled(bool checked);
    void on_flashcallc2_toggled(bool checked);
    void on_flashcallc3a_toggled(bool checked);
    void on_flashcallc3b_toggled(bool checked);

    void airplaneMode(bool turnItOn);

private slots:
    void sdActionTriggered(QAction * action);  // checker style
    void sdAction2Triggered(QAction * action); // SD level

    void on_stopButton_clicked();
    void on_playButton_clicked();
    void on_pitchSlider_valueChanged(int value);
    void on_volumeSlider_valueChanged(int value);
    void on_actionMute_triggered();
    void on_tempoSlider_valueChanged(int value);
    void on_mixSlider_valueChanged(int value);
    void on_seekBar_valueChanged(int value);
    void on_clearSearchButton_clicked();
    void on_actionLoop_triggered();

    void on_UIUpdateTimerTick(void);
    void on_vuMeterTimerTick(void);

    void on_newVolumeMounted();
    void aboutBox();

    void on_actionSpeed_Up_triggered();
    void on_actionSlow_Down_triggered();
    void on_actionSkip_Ahead_15_sec_triggered();
    void on_actionSkip_Back_15_sec_triggered();
    void on_actionVolume_Up_triggered();
    void on_actionVolume_Down_triggered();
    void on_actionPlay_triggered();
    void on_actionStop_triggered();
    void on_actionForce_Mono_Aahz_mode_triggered();

    void on_bassSlider_valueChanged(int value);
    void on_midrangeSlider_valueChanged(int value);
    void on_trebleSlider_valueChanged(int value);

    void on_actionOpen_MP3_file_triggered();
    void on_songTable_itemDoubleClicked(QTableWidgetItem *item);

    void on_labelSearch_textChanged();
    void on_typeSearch_textChanged();
    void on_titleSearch_textChanged();

    void on_actionClear_Search_triggered();
    void on_actionPitch_Up_triggered();
    void on_actionPitch_Down_triggered();

    void on_actionAutostart_playback_triggered();
    void on_actionPreferences_triggered();
    void on_actionImport_triggered();
    void on_actionExport_triggered();

    void on_pushButtonClearTaughtCalls_clicked();
    void on_pushButtonCountDownTimerStartStop_clicked();
    void on_pushButtonCountDownTimerReset_clicked();
    void on_pushButtonCountUpTimerStartStop_clicked();
    void on_pushButtonCountUpTimerReset_clicked();

    void on_checkBoxPlayOnEnd_clicked();
    void on_checkBoxStartOnPlay_clicked();

    void getCurrentPointInStream(double *tt, double *pos);
    void on_pushButtonSetIntroTime_clicked();
    void on_pushButtonSetOutroTime_clicked();
    void on_seekBarCuesheet_valueChanged(int);

    void on_lineEditOutroTime_textChanged();
    void on_lineEditIntroTime_textChanged();

    void on_textBrowserCueSheet_selectionChanged();
    void on_textBrowserCueSheet_currentCharFormatChanged(const QTextCharFormat & f);

    // toggles
    void on_pushButtonCueSheetEditItalic_toggled(bool checked);
    void on_pushButtonCueSheetEditBold_toggled(bool checked);

    // clicks
    void on_pushButtonCueSheetEditHeader_clicked(bool checked);
    void on_pushButtonCueSheetEditTitle_clicked(bool checked);
    void on_pushButtonCueSheetEditArtist_clicked(bool checked);
    void on_pushButtonCueSheetEditLabel_clicked(bool checked);
    void on_pushButtonCueSheetEditLyrics_clicked(bool checked);
    void showHTML();

    void on_pushButtonCueSheetEditSave_clicked();

    void setCueSheetAdditionalControlsVisible(bool visible);
    bool cueSheetAdditionalControlsVisible();

    // TODO: change to use the auto-wiring naming convention, when manual slot/signal wiring is removed...
    void timerCountUp_update();
    void timerCountDown_update();

    void on_actionLoad_Playlist_triggered();
    void on_actionSave_Playlist_triggered();
    void on_actionNext_Playlist_Item_triggered();
    void on_actionPrevious_Playlist_Item_triggered();

    void on_previousSongButton_clicked();
    void on_nextSongButton_clicked();

    void on_songTable_itemSelectionChanged();

    void on_actionClear_Playlist_triggered();

    void showInFinderOrExplorer(QString s);

    void on_songTable_customContextMenuRequested(const QPoint &pos);
    void revealInFinder();

    void columnHeaderResized(int logicalIndex, int oldSize, int newSize);
    void columnHeaderSorted(int logicalIndex, Qt::SortOrder order);

    void on_warningLabel_clicked();
    void on_warningLabelCuesheet_clicked();

    void on_tabWidget_currentChanged(int index);

    void writeSDData(const QByteArray &data);
    void readSDData();
    void readPSData();

    void on_actionEnable_voice_input_toggled(bool arg1);
    void microphoneStatusUpdate();


    void on_actionShow_All_Ages_triggered(bool checked);
    void on_actionPractice_triggered(bool checked);
    void on_actionMonday_triggered(bool checked);
    void on_actionTuesday_triggered(bool checked);
    void on_actionWednesday_triggered(bool checked);
    void on_actionThursday_triggered(bool checked);
    void on_actionFriday_triggered(bool checked);
    void on_actionSaturday_triggered(bool checked);
    void on_actionSunday_triggered(bool checked);

    void on_actionIn_Out_Loop_points_to_default_triggered(bool /* checked */);
    void on_actionCompact_triggered(bool checked);
    void on_actionAuto_scroll_during_playback_toggled(bool arg1);

    void on_menuLyrics_aboutToShow();
    void on_actionLyricsCueSheetRevert_Edits_triggered(bool /*checked*/);

    void PlaylistItemToTop();       // moves to top, or adds to the list and moves to top
    void PlaylistItemToBottom();    // moves to bottom, or adds to the list and moves to bottom
    void PlaylistItemMoveUp();          // moves up one position (must already be on the list)
    void PlaylistItemMoveDown();        // moves down one position (must already be on the list)
    void PlaylistItemRemove();      // removes item from the playlist (must already be on the list)

    void on_actionAt_TOP_triggered();

    void on_actionAt_BOTTOM_triggered();

    void on_actionRemove_from_Playlist_triggered();

    void on_actionUP_in_Playlist_triggered();

    void on_actionDOWN_in_Playlist_triggered();

    void on_actionStartup_Wizard_triggered();
    void on_comboBoxCuesheetSelector_currentIndexChanged(int currentIndex);
    void on_comboBoxCallListProgram_currentIndexChanged(int currentIndex);
#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    void on_listWidgetChoreographyFiles_itemChanged(QListWidgetItem *item);
    void on_lineEditChoreographySearch_textChanged();
    void on_listWidgetChoreographySequences_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidgetChoreography_itemDoubleClicked(QListWidgetItem *item);
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT


    void changeApplicationState(Qt::ApplicationState state);
    void focusChanged(QWidget *old, QWidget *now);

    void showContextMenu(const QPoint &pt);  // popup context window for sequence pane in SD

    void lyricsDownloadEnd();
    void cuesheetListDownloadEnd();

    void makeProgress();
    void cancelProgress();

    void musicRootModified(QString s);

    // END SLOTS -----------

    void on_action_1_triggered();
    void on_action_2_triggered();
    void on_action_3_triggered();

    void playSFX(QString which);

    void on_actionRecent1_triggered();
    void on_actionRecent2_triggered();
    void on_actionRecent3_triggered();
    void on_actionRecent4_triggered();
    void on_actionClear_Recent_List_triggered();
    void on_actionCheck_for_Updates_triggered();

    void on_action_4_triggered();
    void on_action_5_triggered();
    void on_action_6_triggered();

    void on_actionStop_Sound_FX_triggered();

    void on_actionZoom_In_triggered();
    void on_actionZoom_Out_triggered();
    void on_actionReset_triggered();

    void on_actionAge_toggled(bool arg1);
    void on_actionPitch_toggled(bool arg1);
    void on_actionTempo_toggled(bool arg1);

    void on_actionFade_Out_triggered();

    void on_actionDownload_Cuesheets_triggered();

    void on_pushButtonCueSheetClearFormatting_clicked();
    void on_toolButtonEditLyrics_toggled(bool checked);

    void on_actionFilePrint_triggered();
    void on_actionSave_triggered();
    void on_actionSave_As_triggered();

    void on_actionFlashCallBasic_triggered();

    void on_actionFlashCallMainstream_triggered();

    void on_actionFlashCallPlus_triggered();

    void on_actionFlashCallA1_triggered();

    void on_actionFlashCallA2_triggered();

    void on_actionFlashCallC1_triggered();

    void on_actionFlashCallC2_triggered();

    void on_actionFlashCallC3a_triggered();

    void on_actionFlashCallC3b_triggered();

//    void on_actionDownload_matching_lyrics_triggered();

private:

    unsigned int screensaverSeconds;  // increments every second, disable screensaver every 60 seconds

    void saveLyrics();
    void saveLyricsAs();
    void saveSequenceAs();

    void fetchListOfCuesheetsFromCloud();
    QList<QString> getListOfCuesheets();
    QList<QString> getListOfMusicFiles();
    bool fuzzyMatchFilenameToCuesheetname(QString s1, QString s2);
    void downloadCuesheetFileIfNeeded(QString cuesheetFilename);

    int linesInCurrentPlaylist;      // 0 if no playlist loaded (not likely, because of current.m3u)

    int preferredVerySmallFontSize;  // preferred font sizes for UI items
    int preferredSmallFontSize;
    int preferredWarningLabelFontSize;
    int preferredNowPlayingFontSize;

    unsigned int oldTimerState, newTimerState;  // break and tip timer states from the analog clock

    QAction *closeAct;  // WINDOWS only
    QWidget *oldFocusWidget;  // last widget that had focus (or NULL, if none did)

    bool justWentActive;

    int iFontsize;  // preferred font size (for eyeballs that can use some help)
    bool inPreferencesDialog;
    QString musicRootPath, guestRootPath, guestVolume, guestMode;
    QString lastCuesheetSavePath;
    QString loadedCuesheetNameWithPath;
    enum SongFilenameMatchingType songFilenameFormat;

    QFileSystemWatcher musicRootWatcher;  // watch for add/deletes in musicRootPath

    bool showTimersTab;         // EXPERIMENTAL TIMERS STUFF
    bool showLyricsTab;         // EXPERIMENTAL LYRICS STUFF
    bool clockColoringHidden;   // EXPERIMENTAL CLOCK COLORING STUFF

    QMap<int,QPair<QWidget *,QString> > tabmap; // keep track of experimental tabs

    unsigned char currentState;
    short int currentPitch;
    unsigned short currentVolume;
    int previousVolume;

    bool tempoIsBPM;
    float baseBPM;   // base-level detected BPM (either libbass or embedded TBPM frame in ID3)
    bool switchToLyricsOnPlay;

    void Info_Volume(void);
    void Info_Seekbar(bool forceSlider);
    QString position2String(int position, bool pad);

    bool closeEventHappened;

    QString currentMP3filename;
    QString currentMP3filenameWithPath;
    bool songLoaded;
    bool fileModified;

    QString currentSongType;
    QString currentSongTitle;
    int randCallIndex;     // for Flash Calls

    void writeCuesheet(QString filename);
    void saveCurrentSongSettings();
    void loadSettingsForSong(QString songTitle);
    void randomizeFlashCall();

    QString filepath2SongType(QString MP3Filename);  // returns the type (as a string).  patter, hoedown -> "patter", as per user prefs

    float getID3BPM(QString MP3FileName);

    void reloadCurrentMP3File();
    void loadMP3File(QString filepath, QString songTitle, QString songType);
    void maybeLoadCSSfileIntoTextBrowser();
    void loadCuesheet(const QString &cuesheetFilename);
    void loadCuesheets(const QString &MP3FileName, const QString preferredCuesheet = QString());
    void findPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets);
    bool breakFilenameIntoParts(const QString &s, QString &label, QString &labelnum, QString &labenum_extra,
                                QString &title, QString &shortTitle );

    void findMusic(QString mainRootDir, QString guestRootDir, QString mode, bool refreshDatabase);    // get the filenames into pathStack
    void filterMusic();  // filter them into the songTable
    void loadMusicList();  // filter them into the songTable
    void loadChoreographyList();
    void filterChoreography();
    QStringList getUncheckedItemsFromCurrentCallList();

    unsigned int pointSizeToIndex(unsigned int pointSize);
    unsigned int indexToPointSize(unsigned int index);

    unsigned int currentMacPointSize;

    void setFontSizes();
    void adjustFontSizes();
    void usePersistentFontSize();
    void persistNewFontSize(int points);

    int col0_width;  // work around a Qt bug, wherein it does not track the width of the # column

    void sortByDefaultSortOrder();  // sort songTable by default order (not including # column)

    // Playlist stuff ----------
    QString loadPlaylistFromFile(QString PlaylistFileName, int &songCount); // returns error song string and songCount
    void finishLoadingPlaylist(QString PlaylistFileName);

    void saveCurrentPlaylistToFile(QString PlaylistFileName);

    void loadRecentPlaylist(int num);
    void updateRecentPlaylistMenu();
    void addFilenameToRecentPlaylist(QString filename);

    // Lyrics stuff ----------
    QString loadLyrics(QString MP3FileName);
    int lyricsTabNumber;
    bool hasLyrics;

    QString txtToHTMLlyrics(QString text, QString filePathname);

    QList<QString> *pathStack;

    // Experimental Timer stuff ----------
    QTimer *timerCountUp;
    qint64 timeCountUpZeroMs;
    QTimer *timerCountDown;
    qint64 timeCountDownZeroMs;
    bool trapKeypresses;
    QString reverseLabelTitle;

    void saveCheckBoxState(const char *key_string, QCheckBox *checkBox);
    void restoreCheckBoxState(const char *key_string, QCheckBox *checkBox,
                              bool checkedDefault);
    bool timerStopStartClick(QTimer *&timer, QPushButton *button);
    int updateTimer(qint64 timeZero, QLabel *label);

    QString removePrefix(QString prefix, QString s);

    void updateSongTableColumnView();

    int selectedSongRow();  // returns -1 if none selected
    int PlaylistItemCount(); // returns the number of items in the currently loaded playlist
    int getSelectionRowForFilename(const QString &filePath);

    // Song types
    QStringList songTypeNamesForPatter;
    QStringList songTypeNamesForSinging;
    QStringList songTypeNamesForCalled;
    QStringList songTypeNamesForExtras;

    // VU Meter support
    QTimer *UIUpdateTimer;
    QTimer *vuMeterTimer;

    LevelMeter *vuMeter;

    AnalogClock *analogClock;

    QString patterColorString, singingColorString, calledColorString, extrasColorString;  // current values

    // experimental break and patter timers
    bool tipLengthTimerEnabled, breakLengthTimerEnabled, tipLength30secEnabled;
    unsigned int tipLengthTimerLength, breakLengthTimerLength;
    unsigned int tipLengthAlarmAction, breakLengthAlarmAction;

#ifdef Q_OS_MAC
    MacUtils macUtils;  // singleton
#endif

    QFileSystemWatcher *fileWatcher;
    QStringList getCurrentVolumes();
    QStringList lastKnownVolumeList;  // list of volume pathnames, one per volume
    QStringList newVolumeList;    // list of volume pathnames, one per volume

    QStringList flashCalls;

    // --------------
    void restartSDprocess(QString SDdanceLevel);
    void initSDtab();

    QString currentSDVUILevel;
    QString currentSDKeyboardLevel;

    QProcess *sd;  // sd process
    QProcess *ps;  // pocketsphinx process
    Highlighter *highlighter;
    RenderArea *renderArea;
    QString uneditedData;
    QString editedData;
    QString copyrightText;  // sd copyright string (shown once at start)
    bool copyrightShown;

    bool voiceInputEnabled;

    SongSettings songSettings;


    bool firstTimeSongIsPlayed;
    bool loadingSong; // guard to prevent text setting stuff from munging settings
    bool cuesheetEditorReactingToCursorMovement;
    void setCurrentSessionId(int id);
    void setCurrentSessionIdReloadSongAges(int id);
    void reloadSongAges(bool show_all_sessions);
    bool autoScrollLyricsEnabled;
    void loadDanceProgramList(QString lastDanceProgram);

    Qt::ApplicationState currentApplicationState;  // if app state is inactive, mics are disabled.

    // get/set microphone volume
    int currentInputVolume;
    int getInputVolume();                   // returns the current input volume, or -1 if it doesn't know.
    void setInputVolume(int newVolume);     // set the input volume, ignores -1
    void muteInputVolume();         // call this one
    void unmuteInputVolume();      //   and this one (generally avoid calling setInputVolume() directly)

    // sound fx
    QString soundFXarray[6];
    QString soundFXname[6];
    void maybeInstallSoundFX();

    int totalZoom;  // total zoom for Lyrics pane, so it can be undone with a Reset Zoom

    QElapsedTimer t1; //, t2;  // used for simple timing operations
    void startLongSongTableOperation(QString s);
    void stopLongSongTableOperation(QString s);  // use the same string each time

    QProgressDialog *progressDialog;
    QTimer *progressTimer;
    float progressTotal;    // 0 - 100 for each stage, resets to zero at start of each stage...
    float progressOffset;   // 0, 33, 66 for each of the 3 stages of downloading/matching...
    bool progressCancelled; // true if user said STOP

private:
    QHash<Qt::Key, KeyAction *> hotkeyMappings;
public:


    // Key actions:
    friend class KeyActionStopSong;
    friend class KeyActionPlaySong;
    friend class KeyActionRestartSong;
    friend class KeyActionForward15Seconds;
    friend class KeyActionBackward15Seconds;
    friend class KeyActionVolumeMinus;
    friend class KeyActionVolumePlus;
    friend class KeyActionTempoPlus;
    friend class KeyActionTempoMinus;
    friend class KeyActionPlayNext;
    friend class KeyActionMute;
    friend class KeyActionPitchPlus;
    friend class KeyActionPitchMinus;
    friend class KeyActionFadeOut ;
    friend class KeyActionLoopToggle;
    friend class KeyActionNextTab;

    // actions which aren't mapped to keys above:
    void actionTempoPlus();
    void actionTempoMinus();
    void actionFadeOutAndPause();
    void actionNextTab();

    void loadCallList(SongSettings &songSettings, QTableWidget *tableWidget, const QString &danceProgram, const QString &filename);
    void tableWidgetCallList_checkboxStateChanged(int row, int state);
};

// currentState:
#define kStopped 0
#define kPlaying 1
#define kPaused  2

// columns in songTable
#define kNumberCol 0
#define kTypeCol 1
#define kPathCol 1
// path is stored in the userData portion of the Type column...
#define kLabelCol 2
#define kTitleCol 3
#define kAgeCol   4

// hidden columns:
#define kPitchCol 5
#define kTempoCol 6

// font sizes
#define SMALLESTZOOM (11)
#define BIGGESTZOOM (25)
#define ZOOMINCREMENT (2)

//#define SMALLESTZOOM (0)
//#define RESETZOOM (1)
//#define BIGGESTZOOM (6)

// columns in tableViewCallList
#define kCallListOrderCol       0
#define kCallListCheckedCol     1
#define kCallListNameCol        2
#define kCallListWhenCheckedCol 3


// ---------------------------------------------
// http://stackoverflow.com/questions/24719739/how-to-use-qstylesh-tooltip-wakeupdelay-to-set-tooltip-wake-up-time
class MyProxyStyle : public QProxyStyle
{
    Q_OBJECT
public:
    int styleHint(StyleHint hint,
                  const QStyleOption *option,
                  const QWidget *widget,
                  QStyleHintReturn *returnData) const Q_DECL_OVERRIDE
    {
        if (hint == QStyle::SH_ToolTip_WakeUpDelay) {
            return 2000;    // 2 seconds for tooltips
        }

        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

// ---------------------
class GlobalEventFilter: public QObject
{

public:
    GlobalEventFilter(Ui::MainWindow *ui1)
    {
        ui = ui1;
    }
    Ui::MainWindow *ui;
    bool eventFilter(QObject *Object, QEvent *Event);
};


#endif // MAINWINDOW_H
