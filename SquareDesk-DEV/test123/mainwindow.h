/****************************************************************************
**
** Copyright (C) 2016-2019 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
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
#include "prefsmanager.h"
#include <QSlider>
#include <QSplashScreen>
#include <QStack>
#include <QTableWidgetItem>
#include <QToolTip>
#include <QVariant>
#include <QShortcut>
#if defined(Q_OS_MAC) | defined(Q_OS_WIN)
#include <QtWebEngineWidgets>
#else
#include <QtWebKitWidgets/QWebView>
#endif
#include <QWheelEvent>
#include <QWidget>
#include <QFileSystemWatcher>
#include <QGraphicsScene>
#include <QGraphicsItemGroup>
#include <QDateTime>

#include "common_enums.h"
#include "sdhighlighter.h"
#include "sdredostack.h"

#include "math.h"
#include "bass_audio.h"
#include "myslider.h"
#include "preferencesdialog.h"
#include "levelmeter.h"
#include "analogclock.h"
#include "console.h"
//#include "renderarea.h"
#include "songsettings.h"

#if defined(Q_OS_MAC)
#include "macUtils.h"
#include <stdio.h>
#include <errno.h>
#endif
#include <tidy/tidy.h>
#include <tidy/tidybuffio.h>

#include "sdinterface.h"

class SDDancer
{
public:
//    double start_x;
//    double start_y;
//    double start_rotation;

    
    QGraphicsItemGroup *graphics;
    QGraphicsItem *mainItem;
    QGraphicsRectItem *directionRectItem;
    QGraphicsTextItem *label;

    // See note on setDestinationScalingFactors below
    
    void setDestination(int x, int y, int direction)
    {
        source_x = dest_x;
        source_y = dest_y;
        source_direction = dest_direction;
        
        dest_x = x;
        dest_y = y;
        dest_direction = direction;
    }

    
    // This is really gross: We stash stuff in the destination, then
    // when we have finished calculating the common factors, we adjust
    // the destination values. It sucks and is awful and technical
    // debt.
    
    void setDestinationScalingFactors(double left_x, double max_x, double max_y, double lowest_factor)
    {
        double dancer_start_x = dest_x - left_x;
        dest_x = (dancer_start_x / lowest_factor - max_x / 2.0);
        dest_y = dest_y - max_y / 2.0;
    }
    
    double getX(double t)
    {
        return (source_x * (1 - t) + dest_x * t);
    }
    double getY(double t)
    {
        return (source_y * (1 - t) + dest_y * t);
    }
    double getDirection(double t)
    {
        if (dest_direction < source_direction && t < 1.0)
            return (source_direction * (1 - t) + (dest_direction + 360.0) * t);
        return (source_direction * (1 - t) + dest_direction * t);
    }

private:
    double source_x;
    double source_y;
    double source_direction;
    double dest_x;
    double dest_y;
    double dest_direction;
//    double destination_divisor;
public:
    double labelTranslateX;
    double labelTranslateY;

    void setColor(const QColor &color);
};


// REMEMBER TO CHANGE THIS WHEN WE RELEASE A NEW VERSION.
// ALSO REMEMBER TO CHANGE THE VERSION IN PackageIt.command !
// Also remember to change the "latest" file on GitHub (for Beta releases)!
#define VERSIONSTRING "0.9.2alpha13"

// cuesheets are assumed to be at the top level of the SquareDesk repo, and they
//   will be fetched from there.
#define CURRENTSQVIEWLYRICSNAME "SqViewCueSheets_2017.03.14"
#define CURRENTSQVIEWCUESHEETSDIR "https://squaredesk.net/cuesheets/SqViewCueSheets_2017.10.13/"

#define NUMBEREDSOUNDFXFILES 8

namespace Ui
{
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QSplashScreen *splash, QWidget *parent = nullptr);
    ~MainWindow() override;

    double songLoadedReplayGain_dB;

    Ui::MainWindow *ui;
    bool handleKeypress(int key, QString text);
    bool someWebViewHasFocus();

    void stopSFX();
    void playSFX(QString which);

    // ERROR LOGGING...
    static void customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static QString logFilePath;

    PreferencesDialog *prefDialog;
    QActionGroup *sessionActionGroup;
    QAction **sessionActions;
    QActionGroup *sdActionGroup1;
    QActionGroup *sdActionGroup2;
    QActionGroup *sdActionGroupDanceProgram;

    QActionGroup *flashCallTimingActionGroup;

    void checkLockFile();
    void clearLockFile(QString path);

    QStringList parseCSV(const QString &string);
    QString tidyHTML(QString s);  // return the tidied HTML
    QString postProcessHTMLtoSemanticHTML(QString cuesheet);

    void readFlashCallsList();  // re-read the flashCalls file, keep just those selected

    unsigned int numWebviews;
#define MAX_WEB_VIEWS 16
#if defined(Q_OS_MAC) | defined(Q_OS_WIN)
    QWebEngineView* webview[MAX_WEB_VIEWS];  // max of 16 tabs
#else
    QWebView* webview[MAX_WEB_VIEWS];
#endif    
    QTabWidget *documentsTab;

public slots:

    void changeApplicationState(Qt::ApplicationState state);

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
    void on_actionExport_Play_Data_triggered();
    
    void on_pushButtonClearTaughtCalls_clicked();
    void on_pushButtonCountDownTimerStartStop_clicked();
    void on_pushButtonCountDownTimerReset_clicked();
    void on_pushButtonCountUpTimerStartStop_clicked();
    void on_pushButtonCountUpTimerReset_clicked();

    void on_checkBoxPlayOnEnd_clicked();
    void on_checkBoxStartOnPlay_clicked();

    void getCurrentPointInStream(double *pos, double *len);
    void on_pushButtonSetIntroTime_clicked();
    void on_pushButtonSetOutroTime_clicked();
    void on_seekBarCuesheet_valueChanged(int);

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
    void showHTML(QString fromWhere);

    void on_pushButtonCueSheetEditSave_clicked();

    void setCueSheetAdditionalControlsVisible(bool visible);
    bool cueSheetAdditionalControlsVisible();
    void setInOutButtonState();
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
    void editTags();
    void loadSong();
    void revealInFinder();

    void columnHeaderResized(int logicalIndex, int oldSize, int newSize);
    void columnHeaderSorted(int logicalIndex, Qt::SortOrder order);

    void on_warningLabel_clicked();
    void on_warningLabelCuesheet_clicked();

    void on_tabWidget_currentChanged(int index);

    void readPSData();
    void readPSStdErr();
    void pocketSphinx_errorOccurred(QProcess::ProcessError error);
    void pocketSphinx_started();

    void on_actionEnable_voice_input_toggled(bool arg1);
    void microphoneStatusUpdate();

    void readMP3GainData();
    void MP3Gain_errorOccurred(QProcess::ProcessError error);
    void MP3Gain_finished(int exitCode);

    void on_actionShow_All_Ages_triggered(bool checked);

//    void on_actionIn_Out_Loop_points_to_default_triggered(bool /* checked */);
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
    void action_session_change_triggered();
#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    void on_listWidgetChoreographyFiles_itemChanged(QListWidgetItem *item);
    void on_lineEditChoreographySearch_textChanged();
    void on_listWidgetChoreographySequences_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidgetChoreography_itemDoubleClicked(QListWidgetItem *item);
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT


    void focusChanged(QWidget *old, QWidget *now);

    void lyricsDownloadEnd();
    void cuesheetListDownloadEnd();

    void makeProgress();
    void cancelProgress();

    void musicRootModified(QString s);
    void maybeLyricsChanged();

    // END SLOTS -----------

    void on_action_1_triggered();
    void on_action_2_triggered();
    void on_action_3_triggered();

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

    void on_actionViewTags_toggled(bool);
    void on_actionRecent_toggled(bool arg1);
    void on_actionAge_toggled(bool arg1);
    void on_actionPitch_toggled(bool arg1);
    void on_actionTempo_toggled(bool arg1);

    void on_actionFade_Out_triggered();

    void on_actionDownload_Cuesheets_triggered();

    void on_pushButtonCueSheetClearFormatting_clicked();
    void on_pushButtonEditLyrics_toggled(bool checked);

    void on_actionFilePrint_triggered();
    void on_actionSave_triggered();
    void on_actionSave_As_triggered();

    // SD integration
    void on_lineEditSDInput_returnPressed();
    void on_lineEditSDInput_textChanged();
    void on_listWidgetSDOptions_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidgetSDOutput_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidgetSDAdditionalOptions_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidgetSDQuestionMarkComplete_itemDoubleClicked(QListWidgetItem *item);
    void on_tableWidgetCurrentSequence_itemDoubleClicked(QTableWidgetItem *item);
    void on_tableWidgetCurrentSequence_customContextMenuRequested(const QPoint &pos);
    void copy_selection_from_tableWidgetCurrentSequence();
    void copy_selection_from_tableWidgetCurrentSequence_html();
    void set_sd_copy_options_entire_sequence();
    void set_sd_copy_options_selected_rows();
    void set_sd_copy_options_selected_cells();
    void toggle_sd_copy_html_includes_headers();
    void toggle_sd_copy_html_formations_as_svg();
    void update_sd_animations();

    void undo_sd_to_row();
    void undo_last_sd_action();
    void redo_last_sd_action();
    void select_all_sd_current_sequence();
    void on_listWidgetSDOutput_customContextMenuRequested(const QPoint&);
    void copy_selection_from_listWidgetSDOutput();
    void on_actionSDDanceProgramMainstream_triggered();
    void on_actionSDDanceProgramPlus_triggered();
    void on_actionSDDanceProgramA1_triggered();
    void on_actionSDDanceProgramA2_triggered();
    void on_actionSDDanceProgramC1_triggered();
    void on_actionSDDanceProgramC2_triggered();
    void on_actionSDDanceProgramC3A_triggered();
    void on_actionSDDanceProgramC3_triggered();
    void on_actionSDDanceProgramC3x_triggered();
    void on_actionSDDanceProgramC4_triggered();
    void on_actionSDDanceProgramC4x_triggered();
    void on_actionShow_Concepts_triggered();
    void on_actionShow_Commands_triggered();
    void on_actionFormation_Thumbnails_triggered();

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

    void on_actionTest_Loop_triggered();
    void on_dateTimeEditIntroTime_timeChanged(const QTime &time);
    void on_dateTimeEditOutroTime_timeChanged(const QTime &time);
    void on_pushButtonTestLoop_clicked();

    void on_actionClear_Recent_triggered();

    void on_actionBold_triggered();

    void on_actionItalic_triggered();

    void on_pushButtonCueSheetEditSaveAs_clicked();

    void on_action5_seconds_triggered();

    void on_action10_seconds_triggered();

    void on_action15_seconds_triggered();

    void on_action20_seconds_triggered();

    void on_actionMake_Flash_Drive_Wizard_triggered();

    void on_actionShow_group_station_toggled(bool arg1);

public:
    void on_threadSD_errorString(QString str);
    void on_sd_set_window_title(QString str);
    void on_sd_add_new_line(QString, int drawing_picture);
    void on_sd_set_pick_string(QString);
    void on_sd_dispose_of_abbreviation(QString);
    void on_sd_set_matcher_options(QStringList options, QStringList levels);
    void on_sd_update_status_bar(QString str);
    void on_sd_awaiting_input();
    void sd_begin_available_call_list_output();
    void sd_end_available_call_list_output();
    void initialize_internal_sd_tab();
    void do_sd_double_click_call_completion(QListWidgetItem *item);
    void highlight_sd_replaceables();
    void populateMenuSessionOptions();
    void titleLabelDoubleClicked(QMouseEvent * /* event */);
    void sdSequenceCallLabelDoubleClicked(QMouseEvent * /* event */);
    void submit_lineEditSDInput_contents_to_sd();
private:

    bool flashCallsVisible;

    int lastSongTableRowSelected;

    // Lyrics editor -------
    enum charsType { TitleChars=1, LabelChars=96, ArtistChars=255, HeaderChars=2, LyricsChars=3, NoneChars=0}; // matches blue component of CSS definition
    charsType FG_BG_to_type(QColor fg, QColor bg);
    QTextCharFormat lastKnownTextCharFormat;
    int currentSelectionContains();
    const int titleBit = 0x01;
    const int labelBit = 0x02;
    const int artistBit = 0x04;
    const int headerBit = 0x08;
    const int lyricsBit = 0x10;
    const int noneBit = 0x20;
    QString getResourceFile(QString s);  // get a resource file, and return as string or "" if not found

    unsigned int screensaverSeconds;  // increments every second, disable screensaver every 60 seconds

    QLabel *micStatusLabel;

    void saveLyrics();
    void saveLyricsAs();
    void saveSequenceAs();

    void fetchListOfCuesheetsFromCloud();
    QList<QString> getListOfCuesheets();
    QList<QString> getListOfMusicFiles();
    bool fuzzyMatchFilenameToCuesheetname(QString s1, QString s2);
    void downloadCuesheetFileIfNeeded(QString cuesheetFilename);

    int linesInCurrentPlaylist;      // 0 if no playlist loaded (not likely, because of current.m3u)
    QString lastSavedPlaylist;       // "" if no playlist was saved in this session

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
    QFileSystemWatcher lyricsWatcher;     // watch for add/deletes in musicRootPath/lyrics

    bool showTimersTab;         // EXPERIMENTAL TIMERS STUFF
    bool showLyricsTab;         // EXPERIMENTAL LYRICS STUFF
    bool clockColoringHidden;   // EXPERIMENTAL CLOCK COLORING STUFF

    QMap<int,QPair<QWidget *,QString> > tabmap; // keep track of experimental tabs

//    unsigned char currentState;
    int currentPitch;
    unsigned short currentVolume;
    int previousVolume;

    double startOfSong_sec;  // beginning of the song that is loaded
    double endOfSong_sec;    // end of the song that is loaded

    bool tempoIsBPM;
    double baseBPM;   // base-level detected BPM (either libbass or embedded TBPM frame in ID3)
    bool switchToLyricsOnPlay;

    void Info_Volume(void);
    void Info_Seekbar(bool forceSlider);
    double previousPosition;  // cBass's previous idea of where we are

    QString position2String(int position, bool pad);

    bool closeEventHappened;

    QString currentMP3filename;
    QString currentMP3filenameWithPath;
    bool songLoaded;
    bool fileModified;

    QString currentSongType;
    QString currentSongTitle;
    QString currentSongLabel;  // record label, e.g. RIV

    int randCallIndex;     // for Flash Calls

    void writeCuesheet(QString filename);
    void saveCurrentSongSettings();
    void loadSettingsForSong(QString songTitle);

    void loadGlobalSettingsForSong(QString songTitle); // settings that must be set after song is loaded

    void randomizeFlashCall();

    QString filepath2SongType(QString MP3Filename);  // returns the type (as a string).  patter, hoedown -> "patter", as per user prefs

    int getRsyncFileCount(QString sourceDir, QString destDir);

    double getID3BPM(QString MP3FileName);

    void reloadCurrentMP3File();
    void loadMP3File(QString filepath, QString songTitle, QString songType, QString songLabel);
    void maybeLoadCSSfileIntoTextBrowser();
    void loadCuesheet(const QString &cuesheetFilename);
    void loadCuesheets(const QString &MP3FileName, const QString preferredCuesheet = QString());
    void findPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets);
    bool breakFilenameIntoParts(const QString &s, QString &label, QString &labelnum, QString &labenum_extra,
                                QString &title, QString &shortTitle );

    void findMusic(QString mainRootDir, QString guestRootDir, QString mode, bool refreshDatabase);    // get the filenames into pathStack
    void filterMusic();  // filter them into the songTable
    void loadMusicList();  // filter them into the songTable
    QString FormatTitlePlusTags(const QString &title, bool setTags, const QString &strtags);
    void changeTagOnCurrentSongSelection(QString tag, bool add);
    void loadChoreographyList();
    void filterChoreography();
    QStringList getUncheckedItemsFromCurrentCallList();

    int pointSizeToIndex(int pointSize);
    int indexToPointSize(int index);

    int currentMacPointSize;

    void setFontSizes();
    void adjustFontSizes();
    void usePersistentFontSize();
    void persistNewFontSize(int points);
    void zoomInOut(int increment);

    int col0_width;  // work around a Qt bug, wherein it does not track the width of the # column

    void sortByDefaultSortOrder();  // sort songTable by default order (not including # column)

    // Playlist stuff ----------
    void savePlaylistAgain();  // saves with the same name we used last time (if there was a last time)
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

    QStringList songTypeToggleList;
    // VU Meter support
    QTimer *UIUpdateTimer;
    QTimer *vuMeterTimer;

    LevelMeter *vuMeter;

    AnalogClock *analogClock;

    QString patterColorString, singingColorString, calledColorString, extrasColorString;  // current values

    // experimental break and patter timers
    bool tipLengthTimerEnabled, breakLengthTimerEnabled, tipLength30secEnabled;
    int tipLengthTimerLength, breakLengthTimerLength;
    int tipLengthAlarmAction, breakLengthAlarmAction;

#ifdef Q_OS_MAC
    MacUtils macUtils;  // singleton
#endif

    QFileSystemWatcher *fileWatcher;
    bool filewatcherShouldIgnoreOneFileSave;
    QStringList getCurrentVolumes();
    QStringList lastKnownVolumeList;  // list of volume pathnames, one per volume
    QStringList newVolumeList;    // list of volume pathnames, one per volume

    QStringList flashCalls;

    // --------------
    void initSDtab();
    void initReftab();

    QString currentSDVUILevel;
    QString currentSDKeyboardLevel;

    QProcess *ps;       // pocketsphinx process
    QProcess *mp3gain;  // mp3gain process
    QString mp3gainResult_filepath;    // results of the mp3gain process
    double  mp3gainResult_dB;   // results of the mp3gain process

    Highlighter *highlighter;
//    RenderArea *renderArea;
    QString uneditedData;
    QString editedData;
    QString copyrightText;  // sd copyright string (shown once at start)
    bool copyrightShown;

    bool voiceInputEnabled;

    SongSettings songSettings;
    PreferencesManager prefsManager;

    bool firstTimeSongIsPlayed;
    bool loadingSong; // guard to prevent text setting stuff from munging settings
    bool cuesheetEditorReactingToCursorMovement;
    void setCurrentSessionId(int id);
    void setCurrentSessionIdReloadSongAges(int id);
    void setCurrentSessionIdReloadSongAgesCheckMenu(int id);

    void setNowPlayingLabelWithColor(QString s, bool flashcall = false);

    QString ageToRecent(QString age);
    QString ageToIntString(QString ageString);

    QDateTime recentFenceDateTime;  // the fence -- older than this, and Recent is blank

    void reloadSongAges(bool show_all_sessions);
    bool autoScrollLyricsEnabled;
    void loadDanceProgramList(QString lastDanceProgram);

    Qt::ApplicationState currentApplicationState;  // if app state is inactive, mics are disabled.

//    // get/set microphone volume
//    int currentInputVolume;
//    int getInputVolume();                   // returns the current input volume, or -1 if it doesn't know.
//    void setInputVolume(int newVolume);     // set the input volume, ignores -1
//    void muteInputVolume();         // call this one
//    void unmuteInputVolume();      //   and this one (generally avoid calling setInputVolume() directly)

    // sound fx
    QMap<int, QString> soundFXfilenames;    // e.g. "9.foo.mp3" --> [9,"9.foo.mp3"]
    QMap<int, QString> soundFXname;         // e.g. "9.foo.mp3" --> [9,"foo"]
    void maybeInstallSoundFX();
    void maybeInstallReferencefiles();

    int totalZoom;  // total zoom for Lyrics pane, so it can be undone with a Reset Zoom

//    QElapsedTimer t1; //, t2;  // used for simple timing operations
    void startLongSongTableOperation(QString s);
    void stopLongSongTableOperation(QString s);  // use the same string each time

    QProgressDialog *progressDialog;
    QTimer *progressTimer;
    double progressTotal;    // 0 - 100 for each stage, resets to zero at start of each stage...
    double progressOffset;   // 0, 33, 66 for each of the 3 stages of downloading/matching...
    bool progressCancelled; // true if user said STOP

private:
    QHash<QString, QVector<QShortcut *> > hotkeyShortcuts;
    void SetKeyMappings(const QHash<QString, KeyAction *> &hotkeyMappings, QHash<QString, QVector<QShortcut* > > hotkeyShortcuts);
    void AddHotkeyMappingsFromMenus(QHash<QString, KeyAction *> &hotkeyMappings);
    void AddHotkeyMappingsFromShortcuts(QHash<QString, KeyAction *> &hotkeyMappings);
public:


    // Key actions:
#define KEYACTION(NAME, STRNAME, ACTION) friend class KeyAction##NAME;
#include "keyactions.h"
#undef KEYACTION
    friend class SDLineEdit;

    // actions which aren't mapped to keys above:
    void actionTempoPlus();
    void actionTempoMinus();
    void actionFadeOutAndPause();
    void actionNextTab();
    void actionSwitchToTab(const char *tabname);
    void actionFilterSongsToPatter();
    void actionFilterSongsToSingers();
    void actionFilterSongsPatterSingersToggle();

    QStringList callListOriginalOrder;
    void loadCallList(SongSettings &songSettings, QTableWidget *tableWidget, const QString &danceProgram, const QString &filename);
    void tableWidgetCallList_checkboxStateChanged(int row, int state);

private:
    QHash<QString, QAction *> keybindingActionToMenuAction;

private: // SD
    SDThread *sdthread;
    QStringList sdformation;
    QGraphicsScene sd_animation_scene;
    QGraphicsScene sd_fixed_scene;
    bool sd_animation_running;
    QList<SDDancer> sd_animation_people;
    QList<SDDancer> sd_fixed_people;

    int sdLastLine;
    int sdUndoToLine;
    bool sdWasNotDrawingPicture;
    bool sdLastLineWasResolve;
    bool sdOutputtingAvailableCalls;
    QList<SDAvailableCall> sdAvailableCalls;
    int sdLineEditSDInputLengthWhenAvailableCallsWasBuilt;
    QGraphicsTextItem *graphicsTextItemSDStatusBarText_fixed;
    QGraphicsTextItem *graphicsTextItemSDStatusBarText_animated;

    QGraphicsTextItem *graphicsTextItemSDLeftGroupText_fixed;  // for display of group-ness
    QGraphicsTextItem *graphicsTextItemSDLeftGroupText_animated;
    QGraphicsTextItem *graphicsTextItemSDTopGroupText_fixed;
    QGraphicsTextItem *graphicsTextItemSDTopGroupText_animated;

    int leftGroup, topGroup;  // groupness numbers, 0 = P, 1 = RH, 2 = O, 3 = C

    QAction **danceProgramActions;
    void setSDCoupleColoringScheme(const QString &scheme);
    QString get_current_sd_sequence_as_html(bool all_rows, bool graphics_as_text);
    void render_current_sd_scene_to_tableWidgetCurrentSequence(int row, const QString &formation);
    void set_current_sequence_icons_visible(bool visible);
    QString sdLastFormationName;
    QShortcut *shortcutSDTabUndo;
    QShortcut *shortcutSDCurrentSequenceSelectAll;
    QShortcut *shortcutSDCurrentSequenceCopy;
    SDRedoStack *sd_redo_stack;

    double sd_animation_t_value;
    double sd_animation_delta_t;
    double sd_animation_msecs_per_frame;
    void render_sd_item_data(QTableWidgetItem *item);
    void SetAnimationSpeed(AnimationSpeed speed);
    void set_sd_last_formation_name(const QString&);
    void set_sd_last_groupness(int l, int t); // update groupness strings

    bool replayGain_dB(QString filepath); // async call
public:
    void do_sd_tab_completion();
    void setCurrentSDDanceProgram(dance_level);
    dance_level get_current_sd_dance_program();
};

// currentState:
#define kStopped 0
#define kPlaying 1
#define kPaused  2


// font sizes
#define SMALLESTZOOM (11)
#define BIGGESTZOOM (25)
#define ZOOMINCREMENT (2)

//#define SMALLESTZOOM (0)
//#define RESETZOOM (1)
//#define BIGGESTZOOM (6)

// song table column info moved over to songlistmodel.h in preparation
// for switching to a model/view treatment of the song table.

// columns in tableViewCallList
#define kCallListOrderCol       0
#define kCallListCheckedCol     1
#define kCallListNameCol        2
#define kCallListWhenCheckedCol 3
#define kCallListTimingCol      4


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
