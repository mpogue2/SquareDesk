/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
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

// clazy:excludeall=connect-by-name

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "globaldefines.h"

#include "splashscreen.h"

#include "mytablewidget.h"
#include <QtMultimedia/qmediaplayer.h>
// #include "svgWaveformSlider.h"
#define NO_TIMING_INFO 1

#include <QObject>
#include <functional>

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
#include <QMap>
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
#include <QTextEdit>
#include <QToolTip>
#include <QVariant>
#include <QVector>
#include <QShortcut>
#include <QtWebEngineWidgets/QtWebEngineWidgets>
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

#include "flexible_audio.h"
#include "embeddedserver.h"

#include "myslider.h"
#include "preferencesdialog.h"

#include "updateid3tagsdialog.h"

#include "levelmeter.h"
// #include "analogclock.h"
// #include "console.h"
//#include "renderarea.h"
#include "songsettings.h"

// Forward declaration for debug dialog
class CuesheetMatchingDebugDialog;

#if defined(Q_OS_MAC)
#include "macUtils.h"
#include <stdio.h>
// #include <errno.h>
#endif
//#include <tidy/tidy.h>
//#include <tidy/tidybuffio.h>

#ifdef USE_JUCE
#ifdef Q_OS_LINUX
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#else
#include "JuceHeader.h"
#endif
#endif

#include "sdinterface.h"

// Extracted helper classes
#include "sddancer.h"
#include "globaleventfilter.h"

// uncomment this to force a Light <-> Dark mode toggle every 5 seconds, checking for crashes
// #define TESTRESTARTINGSQUAREDESK 1


// WHEN WE RELEASE A NEW VERSION:
//   remember to change the VERSIONSTRING below
//   remember to change the VERSION in PackageIt.command and PackageIt_X86.command
//   remember to change the version in the .plist file

// Also remember to change the "latest" file on GitHub (for Beta releases)!
#define VERSIONSTRING "1.1.6"

// cuesheets are assumed to be at the top level of the SquareDesk repo, and they
//   will be fetched from there.
#define CURRENTSQVIEWLYRICSNAME "SqViewCueSheets_2025.05.10"
#define CURRENTSQVIEWCUESHEETSDIR "https://squaredesk.net/cuesheets/SqViewCueSheets_2025.05.10/"

#define NUMBEREDSOUNDFXFILES 8

#define RESTART_SQUAREDESK (-123)

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    // ============================================================================
    // FRIEND CLASSES
    // ============================================================================
    friend class updateID3TagsDialog;
    friend class CuesheetMatchingDebugDialog;
    friend class SDLineEdit;

#define KEYACTION(NAME, STRNAME, ACTION) friend class KeyAction##NAME;
#include "keyactions.h"
#undef KEYACTION

public:
    // ============================================================================
    // CONSTRUCTOR & DESTRUCTOR  
    // ============================================================================
    explicit MainWindow(SplashScreen *splash, bool dark, QWidget *parent = nullptr);
    ~MainWindow() override;

    // ============================================================================
    // CORE UI & APPLICATION STATE
    // ============================================================================
    Ui::MainWindow *ui;
    SplashScreen *theSplash;
    bool mainWindowReady = false;
    bool darkmode;
    QString currentThemeString;
    int longSongTableOperationCount;

    // Key handling and focus management
    bool handleKeypress(int key, QString text);
    bool handleSDFunctionKey(QKeyCombination key, QString text);
    bool someWebViewHasFocus();
    bool optionCurrentlyPressed;

    // ============================================================================
    // AUDIO SYSTEM & PLAYBACK
    // ============================================================================
    // Audio playback
    void handleDurationBPM();
    int testVamp();

    // Sound effects
    void stopSFX();
    void playSFX(QString which);

    // Audition functionality
    QMediaPlayer auditionPlayer;
    bool auditionInProgress;
    bool auditionPlaying = false;
    QTimer auditionSingleShotTimer;
    QString auditionPlaybackDeviceName;
    qint64 auditionStartHere_ms;

    void auditionByKeyPress(void);
    void auditionByKeyRelease(void);
    void auditionSetStartMs(QString filepath);

    // Preview Playback Device functions
    void populatePlaybackDeviceMenu();
    void setPreviewPlaybackDevice(const QString &playbackDeviceName);
    QAudioDevice getAudioDeviceByName(const QString &deviceName);

    // ============================================================================
    // SQUARE DANCE (SD) SYSTEM
    // ============================================================================
    // SD public interface
    QMap<QString, QString> abbrevs;
    QString sdLastFormationName;
    bool newSequenceInProgress;
    bool editSequenceInProgress;
    QStringList highlightedCalls;

    // SD Frame management
    QString frameName;
    QStringList frameFiles;
    QStringList frameVisible;
    QVector<int> frameCurSeq;
    QVector<int> frameMaxSeq;
    bool selectFirstItemOnLoad;
    bool SDtestmode;

    // SD User/Sequence management
    int userID;
    int nextSequenceID;
    QString authorID;
    QString currentSequenceRecordNumber;
    int currentSequenceRecordNumberi;
    QString currentSequenceAuthor;
    QString currentSequenceTitle;
    QHash<int, int> sequenceStatus;
    
    // SD Frame state
    int currentFrameNumber;
    QString currentFrameTextName;
    QString currentFrameHTMLName;

    // SD Core functions
    void do_sd_tab_completion();
    void setCurrentSDDanceProgram(dance_level);
    dance_level get_current_sd_dance_program();
    void setSDCoupleNumberingScheme(const QString &scheme);
    void setPeopleNumberingScheme(QList<SDDancer> &sdpeople, const QString &numberScheme);
    void refreshSDframes();
    void loadFrame(int i, QString filename, int seqNum, QListWidget *list);
    void SDReadSequencesUsed();
    void getMetadata();
    void writeMetadata(int userID, int nextSequenceID, QString authorID);
    void debugCSDSfile(QString frameName);
    bool checkSDforErrors();
    QString translateCall(QString call);
    QString translateCallToLevel(QString thePrettifiedCall);
    QString makeLevelString(const char *levelCalls[]);

    // ============================================================================
    // PLAYLIST MANAGEMENT
    // ============================================================================
    bool slotModified[3];
    QString relPathInSlot[3];

    // Playlist operations
    void saveSlotNow(int whichSlot);
    void saveSlotNow(MyTableWidget *mtw);
    QStringList parseCSV(const QString &string);
    bool parseFilenameIntoParts(const QString &s, QString &label, QString &labelnum, QString &labelnum_extra,
                                QString &title, QString &shortTitle);
    void movePlaylistItems(std::function<bool(MyTableWidget*)> moveOperation);
    std::pair<QTableWidget*, QLabel*> getSlotWidgets(int slotNumber);
    void clearDuplicateSlots(const QString& relPath);
    void setPaletteSlotVisibility(int numSlots);
    void setLastPlaylistLoaded(int slotNumber, const QString& playlistPath);

    // ============================================================================
    // CUESHEET & LYRICS MANAGEMENT
    // ============================================================================
    bool lyricsCopyIsAvailable;
    QString maybeCuesheetLevel(QString filePath);
    QString postProcessHTMLtoSemanticHTML(QString cuesheet);

    // Cuesheet menu functions
    void setupCuesheetMenu();

    // ============================================================================
    // FILE MANAGEMENT & MUSIC LIBRARY
    // ============================================================================
    QString musicRootPath;
    QString makeCanonicalRelativePath(QString s);

    // Label management
    void readLabelNames(void);
    QMultiMap<QString, QString> labelName2labelID;

    // Flash calls
    void selectUserFlashFile();
    void updateFlashFileMenu();
    void readFlashCallsList();

    // Reference documentation
    QList<QWebEngineView*> webViews;
    QTabWidget *documentsTab;
    QStringList patterTemplateCuesheets;
    QStringList lyricsTemplateCuesheets;

    // ============================================================================
    // BULK OPERATIONS & PROCESSING
    // ============================================================================
    int processOneFile(const QString &mp3filename);
    void processFiles(QStringList &mp3filenames);
    QStringList mp3FilenamesToProcess;
    QMap<QString, int> mp3Results;
    QMutex mp3ResultsLock;
    QFuture<int> vampFuture;
    bool killAllVamps;
    int vampStatus;
    void EstimateSectionsForThisSong(QString pathToMP3);
    void EstimateSectionsForTheseSongs(QList<int> rowNumbers);
    void RemoveSectionsForThisSong(QString pathToMP3);
    void RemoveSectionsForTheseSongs(QList<int>);
    int MP3FileSampleRate(QString pathToMP3);
    QString getSongFileIdentifier(QString pathToSong);

    // ============================================================================
    // EMBEDDED SERVER & EXTERNAL INTEGRATIONS
    // ============================================================================
    EmbeddedServer *taminationsServer;
    void startTaminationsServer(void);

    // Now Playing integration for iOS/watchOS remote control
    void setupNowPlaying();
    void updateNowPlayingMetadata();
    void nowPlayingPlay();
    void nowPlayingPause();
    void nowPlayingNext();
    void nowPlayingPrevious();
    void nowPlayingSeek(double timeInSeconds);

    // ============================================================================
    // ACTION GROUPS & MENUS
    // ============================================================================
    PreferencesDialog *prefDialog;
    updateID3TagsDialog *updateDialog;
    QActionGroup *sdViewActionGroup;
    QActionGroup *sessionActionGroup;
    QAction **sessionActions;
    QActionGroup *sdActionGroup1;
    QActionGroup *sdActionGroup2;
    QActionGroup *sdActionGroupDanceProgram;
    QActionGroup *sdActionGroupColors;
    QActionGroup *sdActionGroupNumbers;
    QActionGroup *sdActionGroupGenders;
    QActionGroup *sdActionGroupDances;
    QActionGroup *flashCallTimingActionGroup;
    QActionGroup *snapActionGroup;
    QActionGroup *themesActionGroup;

    // ============================================================================
    // UTILITY & HELPER FUNCTIONS
    // ============================================================================
    void checkLockFile();
    void clearLockFile(QString path);

    // Key action functions
    void actionTempoPlus();
    void actionTempoMinus();
    void actionFadeOutAndPause();
    void actionNextTab();
    void actionSwitchToTab(const char *tabname);
    void actionFilterSongsToPatter();
    void actionFilterSongsToSingers();
    void actionFilterSongsPatterSingersToggle();
    void actionToggleCuesheetAutoscroll();

    // Call list management
    QStringList callListOriginalOrder;
    void loadCallList(SongSettings &songSettings, QTableWidget *tableWidget, const QString &danceProgram, const QString &filename);
    void tableWidgetCallList_checkboxStateChanged(int row, int state);

    // UI helper functions
    void darkPaletteTitleLabelDoubleClicked(QMouseEvent *e);
    void setTitleField(QTableWidget *whichTable, int whichRow, QString fullPath,
                       bool isPlaylist, QString PlaylistFileName, QString theRealPath = "");
    void titleLabelDoubleClicked(QMouseEvent * /* event */);
    void darkTitleLabelDoubleClicked(QMouseEvent * /* event */);
#ifndef NO_TIMING_INFO
    void sdSequenceCallLabelDoubleClicked(QMouseEvent * /* event */);
#endif

    // ============================================================================
    // SD ENGINE INTEGRATION 
    // ============================================================================
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
    void submit_lineEditSDInput_contents_to_sd(QString s = "", int firstCall = 0);

    // ============================================================================
    // ERROR LOGGING
    // ============================================================================
    static void customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static void customMessageOutputQt(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static QString logFilePath;

public slots:
    // ============================================================================
    // UI EVENT SLOTS
    // ============================================================================
    void handleNewSort(QString newSortString);
    void changeApplicationState(Qt::ApplicationState state);
    void haveDuration2(void);
    void focusChanged(QWidget *old, QWidget *now);

#ifdef DEBUG_LIGHT_MODE
    void svgClockStateChanged(QString newStateName);
#endif

    // ============================================================================
    // PLAYLIST SLOTS
    // ============================================================================
    void PlaylistItemsToTop();
    void PlaylistItemsToBottom();
    void PlaylistItemsMoveUp();
    void PlaylistItemsMoveDown();
    void PlaylistItemsRemove();
    void darkAddPlaylistItemsToBottom(int slot);
    void darkAddPlaylistItemToBottom(int whichSlot, QString title, QString thePitch, QString theTempo, QString theFullPath, QString isLoaded);
    void darkAddPlaylistItemAt(int whichSlot, const QString &trackName, const QString &pitch, const QString &tempo, const QString &path, const QString &extra, int insertRow);
    void darkRevealInFinder();

    // ============================================================================
    // LYRICS & CUESHEET SLOTS
    // ============================================================================
    void LyricsCopyAvailable(bool yes);
    void customLyricsMenuRequested(QPoint pos);
    void customPlaylistMenuRequested(QPoint pos);
    void customTreeWidgetMenuRequested(QPoint pos);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    QString proposeCanonicalName(QString baseName, bool withLabelNumExtra = true);
    void dropEvent(QDropEvent *event) override;
    // ============================================================================
    // PROTECTED OVERRIDES & EVENT HANDLERS
    // ============================================================================
    bool maybeSavePlaylist(int whichSlot);
    bool maybeSaveCuesheet(int optionCount);
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
#ifdef USE_JUCE
    bool eventFilter(QObject *watched, QEvent *event) Q_DECL_OVERRIDE;
#endif
    // Protected UI event handlers
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
    void on_flashcalluserfile_toggled(bool checked);
    void on_flashcallfilechooser_toggled(bool checked);
    void airplaneMode(bool turnItOn);
    void sdLoadDance(QString danceName);

private slots:
    // ============================================================================
    // AUDIO CONTROL SLOTS
    // ============================================================================
    void on_actionMute_triggered();
    void on_actionLoop_triggered();
    void on_actionSpeed_Up_triggered();
    void on_actionSlow_Down_triggered();
    void on_actionSkip_Forward_triggered();
    void on_actionSkip_Backward_triggered();
    void on_actionVolume_Up_triggered();
    void on_actionVolume_Down_triggered();
    void on_actionPlay_triggered();
    void on_actionStop_triggered();
    void on_actionForce_Mono_Aahz_mode_triggered();
    void on_actionPitch_Up_triggered();
    void on_actionPitch_Down_triggered();
    void on_actionFade_Out_triggered();
    void on_actionTest_Loop_triggered();
    void on_actionNormalize_Track_Audio_toggled(bool arg1);

    // Dark mode audio controls
    void on_darkPlayButton_clicked();
    void on_darkStopButton_clicked();
    void on_darkStartLoopButton_clicked();
    void on_darkEndLoopButton_clicked();
    void on_darkTestLoopButton_clicked();
    void on_darkVolumeSlider_valueChanged(int value);
    void on_darkTempoSlider_valueChanged(int value);
    void on_darkPitchSlider_valueChanged(int value);
    void on_darkTrebleKnob_valueChanged(int value);
    void on_darkMidKnob_valueChanged(int value);
    void on_darkBassKnob_valueChanged(int value);
    void on_darkSeekBar_valueChanged(int value);
    void on_darkSeekBar_sliderMoved(int value);

    // ============================================================================
    // SQUARE DANCE (SD) SLOTS
    // ============================================================================
    void dancerNameChanged();
    void sdActionTriggered(QAction * action);
    void sdAction2Triggered(QAction * action);
    void sdActionTriggeredColors(QAction * action);
    void sdActionTriggeredNumbers(QAction * action);
    void sdActionTriggeredGenders(QAction * action);
    void sdActionTriggeredDances(QAction * action);
    void sdViewActionTriggered(QAction *action);

    // SD Menu actions
    void on_actionSDSquareYourSets_triggered();
    void on_actionSDHeadsStart_triggered();
    void on_actionSDHeadsSquareThru_triggered();
    void on_actionSDHeads1p2p_triggered();
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

    // SD Input handling
    void SDDebug(const QString &str, bool bold = false);
    void on_lineEditSDInput_returnPressed();
    void on_lineEditSDInput_textChanged();
    void on_listWidgetSDOptions_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidgetSDOutput_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidgetSDAdditionalOptions_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidgetSDQuestionMarkComplete_itemDoubleClicked(QListWidgetItem *item);
    void on_tableWidgetCurrentSequence_itemDoubleClicked(QTableWidgetItem *item);
    void on_tableWidgetCurrentSequence_itemSelectionChanged();
    void on_tableWidgetCurrentSequence_customContextMenuRequested(const QPoint &pos);

    // SD Operations
    void paste_to_tableWidgetCurrentSequence();
    void add_comment_to_tableWidgetCurrentSequence();
    void delete_comments_from_tableWidgetCurrentSequence();
    void copy_selection_from_tableWidgetCurrentSequence();
    void copy_selection_from_tableWidgetCurrentSequence_html();
    void set_sd_copy_options_entire_sequence();
    void set_sd_copy_options_selected_rows();
    void set_sd_copy_options_selected_cells();
    void toggle_sd_copy_html_includes_headers();
    void toggle_sd_copy_html_formations_as_svg();
    void toggle_sd_copy_include_deep();
    void update_sd_animations();
    void toggleHighlight();
    void clearHighlights();
    void refreshHighlights();
    void undo_sd_to_row();
    void undo_last_sd_action();
    void redo_last_sd_action();
    void select_all_sd_current_sequence();
    void on_listWidgetSDOutput_customContextMenuRequested(const QPoint&);
    void copy_selection_from_listWidgetSDOutput();

    // SD Button controls
    void SDMakeFrameFilesIfNeeded();
    void SDGetCurrentSeqs();
    void SDSetCurrentSeqs(int i);
    void SDScanFramesForMax();
    void SDAppendCurrentSequenceToFrame(int i);
    void SDMoveCurrentSequenceToFrame(int i);
    void SDDeleteCurrentSequence();
    void SDReplaceCurrentSequence();
    void SDExitEditMode();
    void on_pushButtonSDUnlock_clicked();
    void on_pushButtonSDNew_clicked();
    void on_pushButtonSDDelete_clicked();
    void on_pushButtonSDSave_clicked();
    void on_pushButtonSDRevert_clicked();

    // ============================================================================
    // FLASH CALLS SLOTS
    // ============================================================================
    void on_actionFlashCallBasic_triggered();
    void on_actionFlashCallMainstream_triggered();
    void on_actionFlashCallPlus_triggered();
    void on_actionFlashCallA1_triggered();
    void on_actionFlashCallA2_triggered();
    void on_actionFlashCallC1_triggered();
    void on_actionFlashCallC2_triggered();
    void on_actionFlashCallC3a_triggered();
    void on_actionFlashCallC3b_triggered();
    void on_actionFlashCallFilechooser_triggered();
    void on_actionFlashCallUserFile_triggered();

    // ============================================================================
    // CUESHEET & LYRICS SLOTS
    // ============================================================================
    void on_textBrowserCueSheet_selectionChanged();
    void on_textBrowserCueSheet_currentCharFormatChanged(const QTextCharFormat & f);
    void on_pushButtonCueSheetEditItalic_toggled(bool checked);
    void on_pushButtonCueSheetEditBold_toggled(bool checked);
    void on_pushButtonCueSheetEditHeader_clicked(bool checked);
    void on_pushButtonCueSheetEditTitle_clicked(bool checked);
    void on_pushButtonCueSheetEditArtist_clicked(bool checked);
    void on_pushButtonCueSheetEditLabel_clicked(bool checked);
    void on_pushButtonCueSheetEditLyrics_clicked(bool checked);
    void on_pushButtonCueSheetEditSave_clicked();
    void on_pushButtonCueSheetEditSaveAs_clicked();
    void on_pushButtonCueSheetClearFormatting_clicked();
    void on_pushButtonEditLyrics_toggled(bool checked);
    void showHTML(QString fromWhere);
    void setCueSheetAdditionalControlsVisible(bool visible);
    bool cueSheetAdditionalControlsVisible();
    void setInOutButtonState();
    void on_pushButtonRevertEdits_clicked();

    // Cuesheet timing controls
    void getCurrentPointInStream(double *pos, double *len);
    void on_pushButtonSetIntroTime_clicked();
    void on_pushButtonSetOutroTime_clicked();
    void on_seekBarCuesheet_valueChanged(int);
    void on_dateTimeEditIntroTime_timeChanged(const QTime &time);
    void on_dateTimeEditOutroTime_timeChanged(const QTime &time);
    void on_darkStartLoopTime_timeChanged(const QTime &time);
    void on_darkEndLoopTime_timeChanged(const QTime &time);

    // ============================================================================
    // SONG TABLE & PLAYLIST SLOTS
    // ============================================================================
    void on_darkSongTable_itemDoubleClicked(QTableWidgetItem *item);
    void on_darkSongTable_customContextMenuRequested(const QPoint &pos);
    void on_darkSongTable_itemSelectionChanged();
    void on_treeWidget_itemSelectionChanged();
    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_playlist1Table_itemSelectionChanged();
    void on_playlist2Table_itemSelectionChanged();
    void on_playlist3Table_itemSelectionChanged();
    void handlePlaylistDoubleClick(QTableWidgetItem *item);

    // Song table operations
    void darkEditTags();
    void revealInFinder();
    void revealLyricsFileInFinder();
    void revealAttachedLyricsFileInFinder();
    void copyIt();
    void pasteIt();
    void cutIt();
    void selectLine();
    bool getSectionLimits(int &sectionStart, int &sectionEnd);
    void selectSection();
    void swapHeadsAndSidesInSelection();
    void columnHeaderResized(int logicalIndex, int oldSize, int newSize);
    void columnHeaderSorted(int logicalIndex, Qt::SortOrder order);

    // ============================================================================
    // FILE & MENU ACTION SLOTS
    // ============================================================================
    void on_actionOpen_MP3_file_triggered();
    void on_actionOpen_Audio_File_triggered();
    void on_actionClear_Search_triggered();
    void on_actionAutostart_playback_triggered();
    void on_actionPreferences_triggered();
    void on_actionImport_triggered();
    void on_actionExport_triggered();
    void on_actionExport_Play_Data_triggered();
    void on_actionExport_Current_Song_List_triggered();
    void on_actionFilePrint_triggered();
    void on_actionSave_triggered();
    void on_actionSave_As_triggered();
    void on_actionSave_Cuesheet_triggered();
    void on_actionSave_Cuesheet_As_triggered();
    void on_actionSave_Sequence_triggered();
    void on_actionSave_Sequence_As_triggered();
    void on_actionSave_Current_Dance_As_HTML_triggered();
    void on_actionPrint_Cuesheet_triggered();
    void on_actionPrint_Sequence_triggered();
    void on_actionLoad_Sequence_triggered();

    void on_actionImport_and_Organize_Files_triggered();

    // ============================================================================
    // VIEW & UI CONTROL SLOTS
    // ============================================================================
    void on_actionViewTags_toggled(bool);
    void on_actionRecent_toggled(bool arg1);
    void on_actionAge_toggled(bool arg1);
    void on_actionPitch_toggled(bool arg1);
    void on_actionTempo_toggled(bool arg1);
    void on_actionShow_All_Ages_triggered(bool checked);
    void on_actionIn_Out_Loop_points_to_default_triggered(bool checked);
    void on_actionAuto_scroll_during_playback_toggled(bool arg1);
    void on_actionShow_group_station_toggled(bool arg1);
    void on_actionShow_order_sequence_toggled(bool arg1);
    void on_actionZoom_In_triggered();
    void on_actionZoom_Out_triggered();
    void on_actionReset_triggered();
    void on_actionSwitch_to_Light_Mode_triggered();
    void on_toggleShowPaletteTables_toggled(bool checked);

    // ============================================================================
    // TIMING & LOOP CONTROL SLOTS
    // ============================================================================
    void on_action5_seconds_triggered();
    void on_action10_seconds_triggered();
    void on_action15_seconds_triggered();
    void on_action20_seconds_triggered();
    void on_actionDisabled_triggered();
    void on_actionNearest_Beat_triggered();
    void on_actionNearest_Measure_triggered();

    // ============================================================================
    // SYSTEM & APPLICATION SLOTS
    // ============================================================================
    void on_UIUpdateTimerTick(void);
    void on_vuMeterTimerTick(void);
    void aboutBox();
    void on_actionCheck_for_Updates_triggered();
    void on_actionSquareDesk_Help_triggered();
    void on_actionSD_Help_triggered();
    void on_actionReport_a_Bug_triggered();
    void on_actionStartup_Wizard_triggered();
    void on_actionMake_Flash_Drive_Wizard_triggered();

    // ============================================================================
    // SOUND EFFECTS SLOTS
    // ============================================================================
    void on_action_1_triggered();
    void on_action_2_triggered();
    void on_action_3_triggered();
    void on_action_4_triggered();
    void on_action_5_triggered();
    void on_action_6_triggered();
    void on_actionStop_Sound_FX_triggered();

    // ============================================================================
    // MISCELLANEOUS SLOTS
    // ============================================================================
    void on_darkSearch_textChanged(const QString &arg1);
    void on_tabWidget_currentChanged(int index);
    void microphoneStatusUpdate();
    void on_warningLabelCuesheet_clicked();
    void on_darkWarningLabel_clicked();
    void on_menuLyrics_aboutToShow();
    void on_actionLyricsCueSheetRevert_Edits_triggered(bool /*checked*/);
    void on_actionExplore_Cuesheet_Matching_triggered();
    void on_comboBoxCuesheetSelector_currentIndexChanged(int currentIndex);
    void on_comboBoxCallListProgram_currentIndexChanged(int currentIndex);
    void action_session_change_triggered();
    void on_actionDownload_Cuesheets_triggered();
    void on_actionBold_triggered();
    void on_actionItalic_triggered();
    void on_actionClear_Recent_triggered();
    void on_actionAuto_format_Lyrics_triggered();
    void on_actionSD_Output_triggered();
    void on_actionShow_Frames_triggered();
    void on_darkSegmentButton_clicked();
    void on_actionEstimate_for_this_song_triggered();
    void on_actionEstimate_for_all_songs_triggered();
    void on_actionRemove_for_this_song_triggered();
    void on_actionRemove_for_all_songs_triggered();
    void on_actionUpdate_ID3_Tags_triggered();
    void on_action0paletteSlots_triggered();
    void on_action1paletteSlots_triggered();
    void on_action2paletteSlots_triggered();
    void on_action3paletteSlots_triggered();
    void on_actionNew_Dance_triggered();

#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    void on_listWidgetChoreographyFiles_itemChanged(QListWidgetItem *item);
    void on_lineEditChoreographySearch_textChanged();
    void on_listWidgetChoreographySequences_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidgetChoreography_itemDoubleClicked(QListWidgetItem *item);
#endif

#ifdef DEBUG_LIGHT_MODE
    void themeTriggered(QAction *action);
    void themesFileModified();
    void setProp(QWidget *theWidget, const char *thePropertyName, bool b);
    void setProp(QWidget *theWidget, const char *thePropertyName, QString s);
#endif

    // File system and progress
    void setSongTableFont(QTableWidget *songTable, const QFont &currentFont);
    void lyricsDownloadEnd();
    void cuesheetListDownloadEnd();
    void makeProgress();
    void cancelProgress();
    void fileWatcherTriggered();
    void fileWatcherDisabledTriggered();
    void musicRootModified(QString s);
    void maybeLyricsChanged();
    void lockForEditing();
    void playlistSlotWatcherTriggered();
    void readAbbreviations();
    QString expandAbbreviations(QString s);
    void reloadPaletteSlots();
    void saveSlotAsPlaylist(int whichSlot);
    void clearSlot(int whichSlot);
    void showInFinderOrExplorer(QString s);
    void on_pushButtonClearTaughtCalls_clicked();
    void on_pushButtonTestLoop_clicked();

    // SD helper functions
    QStringList UserInputToSDCommand(QString userInputString);
    QString SDOutputToUserOutput(QString sdOutputString);
    void sdtest();

    void on_actionMove_on_to_Next_Song_triggered();

    void on_actionReset_Patter_Timer_triggered();

private:
    // ============================================================================
    // INITIALIZATION METHODS (REFACTORED FROM CONSTRUCTOR)
    // ============================================================================
    void initializeUI(); // general UI init

    // Music tab initialization
    void initializeMusicPlaybackControls();
    void initializeMusicPlaylists();
    void initializeMusicSearch();
    void initializeMusicSongTable();

    // Other tabs initialization
    void initializeCuesheetTab();
    void rescanForSDDances();
    void initializeSDTab();
    void initializeTaminationsTab();
    void initializeDanceProgramsTab();
    void initializeReferenceTab();

    // other initialization
    void initializeSessions();
    void initializeAudioEngine();
    void initializeLightDarkTheme();

    // ============================================================================
    // CORE APPLICATION STATE
    // ============================================================================
    QString lastAudioDeviceName;
    bool flashCallsVisible;
    int lastSongTableRowSelected;
    bool doNotCallDarkLoadMusicList;
    unsigned int screensaverSeconds;
    QLabel *micStatusLabel;
    bool justWentActive;
    bool inPreferencesDialog;
    enum SongFilenameMatchingType songFilenameFormat;
    bool closeEventHappened;
    QAction *closeAct;
    QWidget *oldFocusWidget;
    bool lastWidgetBeforePlaybackWasSongTable;

    // ============================================================================
    // CUESHEET & LYRICS SYSTEM
    // ============================================================================
    CuesheetMatchingDebugDialog *cuesheetDebugDialog;
    bool cueSheetLoaded;
    QString loadedCuesheetNameWithPath;
    QString lastCuesheetSavePath;

    QString cuesheetSquareDeskVersion;
    enum charsType { TitleChars=1, LabelChars=96, ArtistChars=255, HeaderChars=2, LyricsChars=3, NoneChars=0};
    charsType FG_BG_to_type(QColor fg, QColor bg);
    QTextCharFormat lastKnownTextCharFormat;
    int currentSelectionContains();
    const int titleBit = 0x01;
    const int labelBit = 0x02;
    const int artistBit = 0x04;
    const int headerBit = 0x08;
    const int lyricsBit = 0x10;
    const int noneBit = 0x20;
    QString getResourceFile(QString s);
    void saveLyrics();
    void saveLyricsAs();
    void saveSequenceAs();
    void fetchListOfCuesheetsFromCloud();
    QList<QString> getListOfCuesheets();
    QList<QString> getListOfMusicFiles();
    bool fuzzyMatchFilenameToCuesheetname(QString s1, QString s2);
    void downloadCuesheetFileIfNeeded(QString cuesheetFilename);
    QString loadLyrics(QString MP3FileName);
    int lyricsTabNumber;
    bool hasLyrics;
    QString txtToHTMLlyrics(QString text, QString filePathname);
    bool cuesheetIsUnlockedForEditing;

    // ============================================================================
    // PLAYLIST MANAGEMENT PRIVATE
    // ============================================================================
    int linesInCurrentPlaylist;
    bool playlistHasBeenModified;
    QString lastSavedPlaylist;
    QString lastFlashcardsUserFile;
    QString lastFlashcardsUserDirectory;
    void loadPlaylistFromFileToPaletteSlot(QString PlaylistFileName, int slotNumber, int &songCount);
    void loadTrackFilterToSlot(QString PlaylistFileName, QString relativePath, int slotNumber, int &songCount);
    void loadAppleMusicPlaylistToSlot(QString PlaylistFileName, QString relativePath, int slotNumber, int &songCount);
    void loadRegularPlaylistToSlot(QString PlaylistFileName, QString relativePath, int slotNumber, int &songCount);
    void loadPlaylistFromFileToSlot(int whichSlot);
    void printPlaylistFromSlot(int whichSlot);
    void updateRecentPlaylistsList(const QString &playlistPath);
    void refreshAllPlaylists();
    void getAppleMusicPlaylists();
    void getLocalPlaylists();
    QList<QStringList> allAppleMusicPlaylists;
    QStringList allAppleMusicPlaylistNames;

    // ============================================================================
    // FONT & UI MANAGEMENT
    // ============================================================================
    int preferredVerySmallFontSize;
    int preferredSmallFontSize;
    int preferredWarningLabelFontSize;
    int preferredNowPlayingFontSize;
    QFont currentSongTableFont;
    int iFontsize;
    int currentMacPointSize;
    int pointSizeToIndex(int pointSize);
    int indexToPointSize(int index);
    void setFontSizes();
    void adjustFontSizes();
    void usePersistentFontSize();
    void persistNewFontSize(int points);
    void zoomInOut(int increment);
    int col0_width;
    int totalZoom;

    QString themePreference;

    // ============================================================================
    // AUDIO SYSTEM PRIVATE
    // ============================================================================
    int currentPitch;
    unsigned short currentVolume;
    int previousVolume;
    double startOfSong_sec;
    double endOfSong_sec;
    bool tempoIsBPM;
    double baseBPM;
    QString targetPitch;
    QString targetTempo;
    QString targetNumber;
    bool switchToLyricsOnPlay;
    void Info_Volume(void);
    void Info_Seekbar(bool forceSlider);
    double previousPosition;
    QString position2String(int position, bool pad);
    unsigned int oldTimerState, newTimerState;

    // ============================================================================
    // MUSIC LIBRARY & FILE MANAGEMENT
    // ============================================================================
    bool songLoaded;
    bool fileModified;
    bool lyricsForDifferentSong;
    QString mp3ForDifferentCuesheet;
    QString override_filename;
    QString override_cuesheet;
    QString typeSearch, labelSearch, titleSearch;
    bool searchAllFields;
    QString currentMP3filename;
    QString currentMP3filenameWithPath;
    QString currentSongTypeName;
    QString currentSongCategoryName;
    bool currentSongIsPatter;
    bool currentSongIsSinger;
    bool currentSongIsVocal;
    bool currentSongIsExtra;
    bool currentSongIsUndefined;
    QString currentSongTitle;
    // QString currentSongLabel;
    int currentSongMP3SampleRate;
    QString currentSongIdentifier;
    uint32_t currentSongID;

    QTableWidget *currentSongPlaylistTable;
    int currentSongPlaylistRow;

    int randCallIndex;

    // File operations
    void writeCuesheet(QString filename);
    void saveCurrentSongSettings();
    void saveCuesheet(const QString songFilename, const QString cuesheetFilename);
    void loadSettingsForSong(QString songTitle);
    bool compareCuesheetPathNamesRelative(QString str1, QString str2);
    QString convertCuesheetPathNameToCurrentRoot(QString str1);
    void loadGlobalSettingsForSong(QString songTitle);
    void randomizeFlashCall();
    QString filepath2SongCategoryName(QString MP3Filename);
    int getRsyncFileCount(QString sourceDir, QString destDir);

    // ID3 tag operations
    int readID3Tags(QString fileName, double *bpm, double *tbpm, uint32_t *loopStartSamples, uint32_t *loopLengthSamples);
    void printID3Tags(QString fileName);
    double getID3BPM(QString MP3FileName);
    int getMP3SampleRate(QString fileName);

    // Music loading
    void reloadCurrentMP3File();
    void loadMP3File(QString filepath, QString songTitle, QString songCategory, QString songLabel, QString nextFilename="");
    void secondHalfOfLoad(QString songTitle);
    void maybeLoadCSSfileIntoTextBrowser(bool useSquareDeskCSS);
    void maybeLoadCuesheets(const QString &MP3FileName, const QString cuesheetFilename);
    void loadCuesheet(const QString cuesheetFilename);
    bool loadCuesheets(const QString &MP3FileName, const QString preferredCuesheet = QString(), QString nextFilename="");
    void findPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets);
    void betterFindPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets);
    bool breakFilenameIntoParts(const QString &s, QString &label, QString &labelnum, QString &labenum_extra,
                                QString &title, QString &shortTitle);
    int MP3FilenameVsCuesheetnameScore(QString fn, QString cn, QTextEdit *debugOut = nullptr);

    // Music library management
    void findMusic(QString mainRootDir, bool refreshDatabase);
    void updateTreeWidget();
    void filterMusic();
    void loadMusicList();
    void darkFilterMusic();
    void darkLoadMusicList(QList<QString> *aPathStack, QString typeFilter, bool forceTypeFilter, bool reloadPaletteSlots, bool suppressSelectionChange = false);
    QString FormatTitlePlusTags(const QString &title, bool setTags, const QString &strtags, QString titleColor = "");
    void changeTagOnCurrentSongSelection(QString tag, bool add);
    void darkChangeTagOnPathToMP3(QString pathToMP3, QString tag, bool add);  // add/remove tag on specific song
    void darkChangeTagOnCurrentSongSelection(QString tag, bool add);
    void removeAllTagsFromSong();
    void removeAllTagsFromSongRow(int row);
    void loadChoreographyList();
    void filterChoreography();
    QStringList getUncheckedItemsFromCurrentCallList();
    void sortByDefaultSortOrder();

    void filterSongsToFirstItemInList(QStringList &list);
    QTreeWidgetItem * treeWidgetTrackItem();
    // ============================================================================
    // FILE SYSTEM WATCHING & MONITORING
    // ============================================================================
    QFileSystemWatcher musicRootWatcher;
    QFileSystemWatcher lyricsWatcher;
    QFileSystemWatcher abbrevsWatcher;
    QFileSystemWatcher *fileWatcher;
    bool filewatcherShouldIgnoreOneFileSave;
    bool filewatcherIsTemporarilyDisabled;

#ifdef DEBUG_LIGHT_MODE
    QFileSystemWatcher lightModeWatcher;
#endif

    // ============================================================================
    // TIMERS & UI UPDATES
    // ============================================================================
    QTimer *UIUpdateTimer;
    QTimer *vuMeterTimer;
    QTimer *fileWatcherTimer;
    QTimer *fileWatcherDisabledTimer;
    QTimer *playlistSlotWatcherTimer;
    LevelMeter *vuMeter;

    // ============================================================================
    // PATH MANAGEMENT & SONG TYPES
    // ============================================================================
    QList<QString> *pathStack;
    QList<QString> *pathStackCuesheets;
    QList<QString> *pathStackPlaylists;
    QList<QString> *pathStackApplePlaylists;
    QList<QString> *pathStackReference;
    QList<QString> *currentlyShowingPathStack;
    QString currentTypeFilter;
    QString currentTreePath;
    bool forceFilter;
    bool trapKeypresses;
    QString reverseLabelTitle;
    QStringList songTypeNamesForPatter;
    QStringList songTypeNamesForSinging;
    QStringList songTypeNamesForCalled;
    QStringList songTypeNamesForExtras;
    QStringList songTypeToggleList;

    // ============================================================================
    // UI STATE & EXPERIMENTAL FEATURES
    // ============================================================================
    bool showTimersTab;
    bool showLyricsTab;
    bool clockColoringHidden;
    QMap<int,QPair<QWidget *,QString> > tabmap;
    QString patterColorString, singingColorString, calledColorString, extrasColorString;
    QSet<QString> pathsOfCalledSongs;
    bool tipLengthTimerEnabled, breakLengthTimerEnabled, tipLength30secEnabled;
    int tipLengthTimerLength, breakLengthTimerLength;
    int tipLengthAlarmAction, breakLengthAlarmAction;

    // ============================================================================
    // PLATFORM-SPECIFIC CODE
    // ============================================================================
#ifdef Q_OS_MAC
    MacUtils macUtils;
#endif

    // ============================================================================
    // SOUND EFFECTS & FLASH CALLS
    // ============================================================================
    QStringList flashCalls;
    QMap<int, QString> soundFXfilenames;
    QMap<int, QString> soundFXname;
    void maybeInstallSoundFX();
    void maybeInstallReferencefiles();
    void maybeInstallTemplates();
    void maybeMakeRequiredFolder(QString folderName);
    void maybeMakeAllRequiredFolders();

    // ============================================================================
    // PROGRESS & LONG OPERATIONS
    // ============================================================================
    void startLongSongTableOperation(QString s);
    void stopLongSongTableOperation(QString s);
    QProgressDialog *progressDialog;
    QTimer *progressTimer;
    double progressTotal;
    double progressOffset;
    bool progressCancelled;

    // ============================================================================
    // SD ENGINE PRIVATE DATA
    // ============================================================================
    void initSDtab();
    void initReftab();
    QString currentSDVUILevel;
    QString currentSDKeyboardLevel;
    Highlighter *highlighter;
    QString uneditedData;
    QString editedData;
    QString copyrightText;
    bool copyrightShown;
    bool voiceInputEnabled;

    // ============================================================================
    // SETTINGS & SESSION MANAGEMENT
    // ============================================================================
    SongSettings songSettings;
    PreferencesManager prefsManager;
    int lastMinuteInHour;
    int lastSessionID;
    int currentSongSecondsPlayed;
    bool currentSongSecondsPlayedRecorded;
    bool firstTimeSongIsPlayed;
    bool loadingSong;
    bool cuesheetEditorReactingToCursorMovement;
    void setCurrentSessionId(int id);
    void setCurrentSessionIdReloadSongAges(int id);
    void setCurrentSessionIdReloadSongAgesCheckMenu(int id);
    void setNowPlayingLabelWithColor(QString s, bool flashcall = false);
    QString ageToRecent(QString age);
    QString ageToIntString(QString ageString);
    QDateTime recentFenceDateTime;
    void reloadSongAges(bool show_all_sessions);
    bool autoScrollLyricsEnabled;
    void loadDanceProgramList(QString lastDanceProgram);
    Qt::ApplicationState currentApplicationState;

    // ============================================================================
    // KEY BINDINGS & SHORTCUTS
    // ============================================================================
    QHash<QString, QVector<QShortcut *> > hotkeyShortcuts;
    void SetKeyMappings(const QHash<QString, KeyAction *> &hotkeyMappings, QHash<QString, QVector<QShortcut* > > hotkeyShortcuts);
    void AddHotkeyMappingsFromMenus(QHash<QString, KeyAction *> &hotkeyMappings);
    void AddHotkeyMappingsFromShortcuts(QHash<QString, KeyAction *> &hotkeyMappings);
    QHash<QString, QAction *> keybindingActionToMenuAction;

    // ============================================================================
    // UTILITY FUNCTIONS
    // ============================================================================
    void saveCheckBoxState(const char *key_string, QCheckBox *checkBox);
    void restoreCheckBoxState(const char *key_string, QCheckBox *checkBox, bool checkedDefault);
    QString removePrefix(QString prefix, QString s);
    void updateSongTableColumnView();
    int selectedSongRow();
    int previousVisibleSongRow();
    int nextVisibleSongRow();
    int darkPreviousVisibleSongRow();
    int darkNextVisibleSongRow();
    int getSelectionRowForFilename(const QString &filePath);
    int darkGetSelectionRowForFilename(const QString &filePath);
    int darkSelectedSongRow();

private: // SD Engine Implementation
    // ============================================================================
    // SD ENGINE CORE
    // ============================================================================
    SDThread *sdthread;
    QStringList sdformation;
    QGraphicsScene sd_animation_scene;
    QGraphicsScene sd_fixed_scene;
    bool sd_animation_running;
    QList<SDDancer> sd_animation_people;
    QList<SDDancer> sd_fixed_people;
    bool sdSliderSidesAreSwapped;

    // SD state tracking
    int sdLastLine;
    int sdLastNonEmptyLine;
    int sdUndoToLine;
    bool sdWasNotDrawingPicture;
    bool sdHasSubsidiaryCallContinuation;
    bool sdLastLineWasResolve;
    bool sdOutputtingAvailableCalls;
    QList<SDAvailableCall> sdAvailableCalls;
    int sdLineEditSDInputLengthWhenAvailableCallsWasBuilt;

    // SD graphics
    QGraphicsTextItem *graphicsTextItemSDStatusBarText_fixed;
    QGraphicsTextItem *graphicsTextItemSDStatusBarText_animated;
    QGraphicsTextItem *graphicsTextItemSDLeftGroupText_fixed;
    QGraphicsTextItem *graphicsTextItemSDLeftGroupText_animated;
    QGraphicsTextItem *graphicsTextItemSDTopGroupText_fixed;
    QGraphicsTextItem *graphicsTextItemSDTopGroupText_animated;

    // SD grouping and ordering
    int leftGroup, topGroup;
    Order boyOrder, girlOrder;
    QAction **danceProgramActions;

    // SD visual settings
    void setSDCoupleColoringScheme(const QString &scheme);
    void setSDCoupleGenderingScheme(const QString &scheme);

    // SD output and rendering
    QString get_current_sd_sequence_as_html(bool all_rows, bool graphics_as_text);
    void render_current_sd_scene_to_tableWidgetCurrentSequence(int row, const QString &formation);
    void set_current_sequence_icons_visible(bool visible);

    // SD shortcuts and undo
    QShortcut *shortcutSDTabUndo;
    QShortcut *shortcutSDTabRedo;
    QShortcut *shortcutSDCurrentSequenceSelectAll;
    QShortcut *shortcutSDCurrentSequenceCopy;
    SDRedoStack *sd_redo_stack;

    // SD text processing
    QString upperCaseWHO(QString call);
    QString prettify(QString call);

    // SD animation
    double sd_animation_t_value;
    double sd_animation_delta_t;
    double sd_animation_msecs_per_frame;
    void render_sd_item_data(QTableWidgetItem *item);
    void SetAnimationSpeed(AnimationSpeed speed);
    void set_sd_last_formation_name(const QString&);
    void set_sd_last_groupness();

    // SD utility functions
    bool compareRelative(QString s1, QString s2);
    void decode_formation_into_dancer_destinations(const QStringList &sdformation, QList<SDDancer> &sdpeople);

    // SD ordering functions
    void inOrder(struct dancer dancers[]);
    Order whichOrder(double p1_x, double p1_y, double p2_x, double p2_y);

    // SD grouping functions
    bool sameGroup(int group1, int group2);
    int whichGroup (struct dancer dancers[], bool top);
    int groupNum(struct dancer dancers[], bool top, int coupleNum, int gender);

    // SD rendering
    QString render_image_item_as_html(QTableWidgetItem *imageItem, QGraphicsScene &scene, QList<SDDancer> &people, bool graphics_as_text);
    void reset_sd_dancer_locations();
    void startSDThread(dance_level dance_program);
    void restartSDThread(dance_level dance_program);

    // ============================================================================
    // UI THEME & APPEARANCE
    // ============================================================================
    bool lastFlashcall;
    QIcon *darkStopIcon;
    QIcon *darkPlayIcon;
    QIcon *darkPauseIcon;
    float *waveform;
    QList<int> currentSplitterSizes;
    MyTableWidget *sourceForLoadedSong;
    QString editingArrowStart = "➡";
    QString currentAnalogClockState;

#ifdef TESTRESTARTINGSQUAREDESK
    bool testRestartingSquareDesk;
    QTimer testRestartingSquareDeskTimer;
#endif

    // ============================================================================
    // TEMPLATE SYSTEM
    // ============================================================================
    void setCurrentSongMetadata(QString type);
    QMenu *templateMenu;
    void newFromTemplate();

#ifdef USE_JUCE
    // ============================================================================
    // JUCE AUDIO PLUGINS
    // ============================================================================
    std::unique_ptr<juce::AudioPluginInstance> loudMaxPlugin;
    juce::HostedAudioProcessorParameter *paramThresh;
    std::unique_ptr<juce::DocumentWindow> loudMaxWin;
    void scanForPlugins();
    QString getCurrentLoudMaxSettings();
    void resetLoudMax();
    void setLoudMaxFromPersistedSettings(QString s);
    void updateFXButtonLED(bool active);
    bool currentFXButtonLEDState;
#endif
};

// font sizes
#define SMALLESTZOOM (11)
#define BIGGESTZOOM (25)
#define ZOOMINCREMENT (2)

// song table column info moved over to songlistmodel.h in preparation
// for switching to a model/view treatment of the song table.

// columns in tableViewCallList
#define kCallListOrderCol       0
#define kCallListCheckedCol     1
#define kCallListNameCol        2
#define kCallListWhenCheckedCol 3
#define kCallListTimingCol      4

// If this is changed, change the string for lyricsTab in mainwindow.ui
// and in keyactions.h
#define CUESHEET_TAB_NAME "Cuesheet"

// -----------------------

#endif // MAINWINDOW_H
