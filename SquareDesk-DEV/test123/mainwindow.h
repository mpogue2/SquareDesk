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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "globaldefines.h"

#include "splashscreen.h"

#include "mytablewidget.h"
#include "svgWaveformSlider.h"
#define NO_TIMING_INFO 1

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
#include "console.h"
//#include "renderarea.h"
#include "songsettings.h"

// Forward declaration for debug dialog
class CuesheetMatchingDebugDialog;

#if defined(Q_OS_MAC)
#include "macUtils.h"
#include <stdio.h>
#include <errno.h>
#endif
//#include <tidy/tidy.h>
//#include <tidy/tidybuffio.h>

#ifdef USE_JUCE
#include "JuceHeader.h"
#endif

#include "sdinterface.h"

// uncomment this to force a Light <-> Dark mode toggle every 5 seconds, checking for crashes
// #define TESTRESTARTINGSQUAREDESK 1

class SDDancer
{
public:
//    double start_x;
//    double start_y;
//    double start_rotation;

    
    QGraphicsItemGroup *graphics;
//    QGraphicsItem *mainItem;
    QGraphicsItem *boyItem;
    QGraphicsItem *girlItem;
    QGraphicsItem *hexItem;

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
//        qDebug() << "setDestScalingFactors:" << left_x << max_x << max_y << lowest_factor;
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
        double src = source_direction;
        double dest = dest_direction;
        double result;

        if (dest > src) {
            if (dest - src <= 180) {
                // go CCW, like normal, e.g. 0 -> 90
            } else {
                // go CW, e.g. 0 -> 270
                dest -= 360.0;  // reflect, e.g. 0 -> -90
            }
        } else {
            // dest < src
            if (src - dest <= 180) {
                // go CW, like normal, e.g. 90 -> 0
            } else {
                // go CCW, e.g. 270 -> 0
                src -= 360.0;  // reflect, e.g. -90 -> 0
            }
        }

        result = src * (1 - t) + dest * t;

//        if (dest_direction < source_direction && t < 1.0)
//            return (source_direction * (1 - t) + (dest_direction + 360.0) * t);
//        return (source_direction * (1 - t) + dest_direction * t);

        return(result);
    }

private:
    double source_x;
    double source_y;
    double source_direction;
    double dest_x;
    double dest_y;
    double dest_direction;
    QPen pen1;
//    double destination_divisor;
public:
    double labelTranslateX;
    double labelTranslateY;

    void setColor(const QColor &color);
    void setColors(const QColor &baseColor, const QColor &outlineColor);
};

// WHEN WE RELEASE A NEW VERSION:
//   remember to change the VERSIONSTRING below
//   remember to change the VERSION in PackageIt.command and PackageIt_X86.command
//   remember to change the version in the .plist file

// Also remember to change the "latest" file on GitHub (for Beta releases)!
#define VERSIONSTRING "1.1.2"

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

public:
    // explicit MainWindow(QSplashScreen *splash, bool dark, QWidget *parent = nullptr);
    explicit MainWindow(SplashScreen *splash, bool dark, QWidget *parent = nullptr);
    ~MainWindow() override;

    int testVamp();  // see if all is well with vamp

    friend class updateID3TagsDialog; // it can access private members of MainWindow
    friend class CuesheetMatchingDebugDialog; // it can access private members of MainWindow

    QString makeCanonicalRelativePath(QString s);

    EmbeddedServer *taminationsServer;
    void startTaminationsServer(void);

    bool auditionPlaying = false;
    void auditionByKeyPress(void);
    void auditionByKeyRelease(void);
    QString auditionPlaybackDeviceName;

    bool optionCurrentlyPressed;

    void readLabelNames(void);
    QMultiMap<QString, QString> labelName2labelID;

    QString currentThemeString;
    bool mainWindowReady = false;

    int longSongTableOperationCount;

    bool lyricsCopyIsAvailable;

    Ui::MainWindow *ui;
    bool handleKeypress(int key, QString text);
    bool handleSDFunctionKey(QKeyCombination key, QString text);
    bool someWebViewHasFocus();
    
    QString maybeCuesheetLevel(QString filePath); // Returns the dance level found in the specified cuesheet file

    void handleDurationBPM();  // when duration and BPM are ready, call this to setup tempo slider, et.al.

    void stopSFX();
    void playSFX(QString which);

    QMediaPlayer auditionPlayer;
    bool auditionInProgress;
    QTimer auditionSingleShotTimer;

    // ERROR LOGGING...
    static void customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static void customMessageOutputQt(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static QString logFilePath;

    updateID3TagsDialog *updateDialog;

    PreferencesDialog *prefDialog;
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

    QActionGroup *snapActionGroup; // snapping in Music menu

    QActionGroup *themesActionGroup; // mutually exclusive themes

    // QSplashScreen *theSplash;
    SplashScreen *theSplash;
    void checkLockFile();  // implicitly accesses theSplash
    void clearLockFile(QString path);

    QStringList parseCSV(const QString &string);
    QString postProcessHTMLtoSemanticHTML(QString cuesheet);

    void selectUserFlashFile();
    void updateFlashFileMenu();
    void readFlashCallsList();  // re-read the flashCalls file, keep just those selected

    // Reference tab ---
    QList<QWebEngineView*> webViews;
    QTabWidget *documentsTab;

    QStringList patterTemplateCuesheets; // of the form "patter.template*.html"
    QStringList lyricsTemplateCuesheets; // of the form "lyrics.template*.html"

public slots:

    void handleNewSort(QString newSortString);

#ifdef DEBUG_LIGHT_MODE
    void svgClockStateChanged(QString newStateName);
#endif
    void changeApplicationState(Qt::ApplicationState state);

    void LyricsCopyAvailable(bool yes);
    void customLyricsMenuRequested(QPoint pos);
    void haveDuration2(void);

    void PlaylistItemsToTop();       // moves to top, or adds to the list and moves to top
    void PlaylistItemsToBottom();    // moves to bottom, or adds to the list and moves to bottom
    void PlaylistItemsMoveUp();          // moves up one position (must already be on the list)
    void PlaylistItemsMoveDown();        // moves down one position (must already be on the list)
    void PlaylistItemsRemove();      // removes item from the playlist (must already be on the list)

    // void darkAddPlaylistItemToTop(int slot);
    void darkAddPlaylistItemsToBottom(int slot);    // adds multiple selected darkSongTable items to the bottom of playlist in slot n
    void darkAddPlaylistItemToBottom(int whichSlot, QString title, QString thePitch, QString theTempo, QString theFullPath, QString isLoaded); // single item add
    void darkAddPlaylistItemAt(int whichSlot, const QString &trackName, const QString &pitch, const QString &tempo, const QString &path, const QString &extra, int insertRow);

    void darkRevealInFinder();
    void darkRevealAttachedLyricsFileInFinder();

    void customPlaylistMenuRequested(QPoint pos);
    void customTreeWidgetMenuRequested(QPoint pos);

protected:
    bool maybeSavePlaylist(int whichSlot);
    bool maybeSaveCuesheet(int optionCount);
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *watched, QEvent *event) Q_DECL_OVERRIDE; // Handle window state changes
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

    void dancerNameChanged(); // when text in any dancerName field changes

    void sdActionTriggered(QAction * action);  // checker style
    void sdAction2Triggered(QAction * action); // SD level

    void sdActionTriggeredColors(QAction * action);  // checker style: Colors
    void sdActionTriggeredNumbers(QAction * action);  // checker style: Numbers
    void sdActionTriggeredGenders(QAction * action);  // checker style: Genders

    void sdActionTriggeredDances(QAction * action);  // Dances/frames

    void sdViewActionTriggered(QAction *action); // Sequence Designer vs Dance Arranger

#ifdef DEBUG_LIGHT_MODE
    void themeTriggered(QAction *action);
#endif

    void on_actionMute_triggered();
    void on_actionLoop_triggered();

    void on_actionSDSquareYourSets_triggered();
    void on_actionSDHeadsStart_triggered();
    void on_actionSDHeadsSquareThru_triggered();
    void on_actionSDHeads1p2p_triggered();
    
    
    void on_UIUpdateTimerTick(void);
    void on_vuMeterTimerTick(void);

    void aboutBox();

    void on_actionSpeed_Up_triggered();
    void on_actionSlow_Down_triggered();
    void on_actionSkip_Forward_triggered();
    void on_actionSkip_Backward_triggered();
    void on_actionVolume_Up_triggered();
    void on_actionVolume_Down_triggered();
    void on_actionPlay_triggered();
    void on_actionStop_triggered();
    void on_actionForce_Mono_Aahz_mode_triggered();

    void on_actionOpen_MP3_file_triggered();
    void on_darkSongTable_itemDoubleClicked(QTableWidgetItem *item);

    void on_actionClear_Search_triggered();
    void on_actionPitch_Up_triggered();
    void on_actionPitch_Down_triggered();

    void on_actionAutostart_playback_triggered();
    void on_actionPreferences_triggered();
    void on_actionImport_triggered();
    void on_actionExport_triggered();
    void on_actionExport_Play_Data_triggered();
    void on_actionExport_Current_Song_List_triggered();

    void on_pushButtonClearTaughtCalls_clicked();

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

    void reloadPaletteSlots(); // called twice, once at constructor time, once after changing preferences colors

    void saveSlotAsPlaylist(int whichSlot); // SAVE AS a playlist in a slot to a CSV file

    void clearSlot(int whichSlot);  // clears out the slot, both table and label

    void showInFinderOrExplorer(QString s);

    void on_darkSongTable_customContextMenuRequested(const QPoint &pos);

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

    void on_warningLabelCuesheet_clicked();
    void on_darkWarningLabel_clicked();

    void on_tabWidget_currentChanged(int index);

    void microphoneStatusUpdate();

    void on_actionShow_All_Ages_triggered(bool checked);

    void on_actionIn_Out_Loop_points_to_default_triggered(bool checked);
    void on_actionAuto_scroll_during_playback_toggled(bool arg1);

    void on_menuLyrics_aboutToShow();
    void on_actionLyricsCueSheetRevert_Edits_triggered(bool /*checked*/);
    void on_actionExplore_Cuesheet_Matching_triggered();

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

    void setSongTableFont(QTableWidget *songTable, const QFont &currentFont);

    void focusChanged(QWidget *old, QWidget *now);

    void lyricsDownloadEnd();
    void cuesheetListDownloadEnd();

    void makeProgress();
    void cancelProgress();

    void fileWatcherTriggered();
    void fileWatcherDisabledTriggered();
    void musicRootModified(QString s);
#ifdef DEBUG_LIGHT_MODE
    void themesFileModified(); // Themes.qss file has been modified
    void setProp(QWidget *theWidget, const char *thePropertyName, bool b);
    void setProp(QWidget *theWidget, const char *thePropertyName, QString s);
#endif

    void maybeLyricsChanged();
    void lockForEditing();

    void playlistSlotWatcherTriggered();

    void readAbbreviations();
    QString expandAbbreviations(QString s);

    // END SLOTS -----------

    // These are SFX sounds --------
    void on_action_1_triggered();
    void on_action_2_triggered();
    void on_action_3_triggered();
    void on_action_4_triggered();
    void on_action_5_triggered();
    void on_action_6_triggered();

    void on_actionCheck_for_Updates_triggered();

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

    QStringList UserInputToSDCommand(QString userInputString); // returns 1 or 3 items
    QString     SDOutputToUserOutput(QString sdOutputString);
    void sdtest();

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
    void on_actionFlashCallFilechooser_triggered();
    void on_actionFlashCallUserFile_triggered();

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

    void on_actionShow_order_sequence_toggled(bool arg1);

    void on_actionAuto_format_Lyrics_triggered();

    void on_actionSD_Output_triggered();

    void on_actionLoad_Sequence_triggered();

    void on_actionSave_Sequence_As_triggered();

    void on_actionShow_Frames_triggered();

    // slots for SD editing buttons ------
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

    void on_actionSquareDesk_Help_triggered();

    void on_actionSD_Help_triggered();

    void on_actionReport_a_Bug_triggered();

    void on_actionSave_Current_Dance_As_HTML_triggered();

    void on_actionDisabled_triggered();

    void on_actionNearest_Beat_triggered();

    void on_actionNearest_Measure_triggered();

    void on_actionSave_Cuesheet_triggered();

    void on_actionSave_Cuesheet_As_triggered();

    void on_actionOpen_Audio_File_triggered();

    void on_actionSave_Sequence_triggered();

    void on_actionPrint_Cuesheet_triggered();

    void on_actionPrint_Sequence_triggered();

    void on_pushButtonRevertEdits_clicked();
    void on_darkStopButton_clicked();

    void on_darkPlayButton_clicked();

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

    void on_darkSearch_textChanged(const QString &arg1);

    void on_treeWidget_itemSelectionChanged();

    void on_darkStartLoopTime_timeChanged(const QTime &time);

    void on_darkEndLoopTime_timeChanged(const QTime &time);

    void on_toggleShowPaletteTables_toggled(bool checked);

    void on_darkSongTable_itemSelectionChanged();

    void on_playlist3Table_itemSelectionChanged();

    void on_playlist2Table_itemSelectionChanged();

    void on_playlist1Table_itemSelectionChanged();

    void on_playlist1Table_itemDoubleClicked(QTableWidgetItem *item);
    void on_playlist2Table_itemDoubleClicked(QTableWidgetItem *item);
    void on_playlist3Table_itemDoubleClicked(QTableWidgetItem *item);

    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_actionSwitch_to_Light_Mode_triggered();

    void on_actionNormalize_Track_Audio_toggled(bool arg1);

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

public:
    void saveSlotNow(int whichSlot);           // SAVE a playlist in a slot to a CSV file, public intentionally for MyTableWidget()
    void saveSlotNow(MyTableWidget *mtw);       // SAVE a playlist in a slot to a CSV file, public intentionally for MyTableWidget()

    // BULK operations -----
    int  processOneFile(const QString &mp3filename);
    void processFiles(QStringList &mp3filenames);
    QStringList mp3FilenamesToProcess;
    QMap<QString, int> mp3Results;
    QMutex mp3ResultsLock;
    QFuture<int> vampFuture; // cancel this at Quit time to stop more jobs from starting
    bool killAllVamps; // set to true at desstructor time, so that threads will kill their QProcesses ASAP
    int vampStatus;
    void EstimateSectionsForThisSong(QString pathToMP3);
    void EstimateSectionsForTheseSongs(QList<int> rowNumbers);
    void RemoveSectionsForThisSong(QString pathToMP3);
    void RemoveSectionsForTheseSongs(QList<int>);

    int MP3FileSampleRate(QString pathToMP3); // returns 32000, 44100, or 48000 for MP3 files, -1 for non-MP3 files
    QString getSongFileIdentifier(QString pathToSong);

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
    void darkTitleLabelDoubleClicked(QMouseEvent * /* event */);
#ifndef NO_TIMING_INFO
    void sdSequenceCallLabelDoubleClicked(QMouseEvent * /* event */);
#endif
    void submit_lineEditSDInput_contents_to_sd(QString s = "", int firstCall = 0); // 0 means not from initializing the SD engine

    // SD
    QMap<QString, QString> abbrevs;

    QString musicRootPath; // needed by sd to do output_prefix
    QString relPathInSlot[3]; // playlist slot 1 --> relPathInSlot[0], used also by MyTableWidgets to know what's inside themselves

    // Preview Playback Device functions
    void populatePlaybackDeviceMenu();
    void setPreviewPlaybackDevice(const QString &playbackDeviceName);
    QAudioDevice getAudioDeviceByName(const QString &deviceName);
    
    // Cuesheet Menu functions
    void setupCuesheetMenu();
    
    // Now Playing integration for iOS/watchOS remote control
    void setupNowPlaying();
    void updateNowPlayingMetadata();
    void nowPlayingPlay();
    void nowPlayingPause();
    void nowPlayingNext();
    void nowPlayingPrevious();
    void nowPlayingSeek(double timeInSeconds);

private:
    QString lastAudioDeviceName;

    bool flashCallsVisible;

    int lastSongTableRowSelected;

    bool doNotCallDarkLoadMusicList; // used to avoid double call of darkLoadSongList at startup

    // Lyrics editor -------
    QString cuesheetSquareDeskVersion; // if the cuesheet that we loaded was written by SquareDesk, this will contain the version (e.g. 0.9.9)
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

    // Debug dialog for cuesheet matching
    CuesheetMatchingDebugDialog *cuesheetDebugDialog;

    void saveLyrics();
    void saveLyricsAs();
    void saveSequenceAs();

    void fetchListOfCuesheetsFromCloud();
    QList<QString> getListOfCuesheets();
    QList<QString> getListOfMusicFiles();
    bool fuzzyMatchFilenameToCuesheetname(QString s1, QString s2);
    void downloadCuesheetFileIfNeeded(QString cuesheetFilename);

    int linesInCurrentPlaylist;      // 0 if no playlist loaded (not likely, because of current.m3u)
    bool playlistHasBeenModified;
    QString lastSavedPlaylist;       // "" if no playlist was saved in this session
    QString lastFlashcardsUserFile;      // "" if no flashcard file currently
    QString lastFlashcardsUserDirectory;      // "" if no flashcard file currently

    int preferredVerySmallFontSize;  // preferred font sizes for UI items
    int preferredSmallFontSize;
    int preferredWarningLabelFontSize;
    int preferredNowPlayingFontSize;

    QFont currentSongTableFont;

    unsigned int oldTimerState, newTimerState;  // break and tip timer states from the analog clock

    QAction *closeAct;  // WINDOWS only
    QWidget *oldFocusWidget;  // last widget that had focus (or NULL, if none did)

    bool lastWidgetBeforePlaybackWasSongTable;

    bool justWentActive;

    int iFontsize;  // preferred font size (for eyeballs that can use some help)
    bool inPreferencesDialog;
    QString lastCuesheetSavePath;
    QString loadedCuesheetNameWithPath;
    enum SongFilenameMatchingType songFilenameFormat;
    bool cueSheetLoaded;

    QFileSystemWatcher musicRootWatcher;  // watch for add/deletes in musicRootPath
    QFileSystemWatcher lyricsWatcher;     // watch for add/deletes in musicRootPath/lyrics
    QFileSystemWatcher abbrevsWatcher;    // watch for changes to abbreviations

    bool showTimersTab;         // EXPERIMENTAL TIMERS STUFF
    bool showLyricsTab;         // EXPERIMENTAL LYRICS STUFF
    bool clockColoringHidden;   // EXPERIMENTAL CLOCK COLORING STUFF

    QMap<int,QPair<QWidget *,QString> > tabmap; // keep track of experimental tabs

    int currentPitch;
    unsigned short currentVolume;
    int previousVolume;

    double startOfSong_sec;  // beginning of the song that is loaded
    double endOfSong_sec;    // end of the song that is loaded

    bool tempoIsBPM;
    double baseBPM;   // base-level detected BPM (either libbass or embedded TBPM frame in ID3)
    QString targetPitch;  // from songTable, this is what we want after load
    QString targetTempo;  // from songTable, this is what we want after load
    QString targetNumber;  // from songTable, this tells us if current song is on a playlist
    bool switchToLyricsOnPlay;

    // int minimumVolume; // volume slider is not allowed to go lower than this

    void Info_Volume(void);
    void Info_Seekbar(bool forceSlider);
    double previousPosition;  // cBass's previous idea of where we are

    QString position2String(int position, bool pad);

    bool closeEventHappened;

    bool songLoaded;
    bool fileModified;
    bool lyricsForDifferentSong;

    // SEARCH --------------
    QString typeSearch, labelSearch, titleSearch;
    bool searchAllFields;

    // SONG METADATA ------------------
    QString currentMP3filename;
    QString currentMP3filenameWithPath;

    QString currentSongTypeName;     // string extracted from the filename, e.g. "Patter" or "Singing call"
    QString currentSongCategoryName; // e.g. "patter"
    bool currentSongIsPatter;    // categories; note that "Patter" and "hoedown" both map to currentSongIsPatter, etc.
    bool currentSongIsSinger;
    bool currentSongIsVocal;
    bool currentSongIsExtra;
    bool currentSongIsUndefined; // doesn't match any of the 4 known categories

    QString currentSongTitle;    // song title, e.g. "Appalachian Joy"
    QString currentSongLabel;    // record label, e.g. RIV

    int currentSongMP3SampleRate;  // this is BEFORE we load it in with Qt, which resamples all to 44.1kHz.
                                // this will be used to set sample-accurate LOOPSTART (which is in samples).
                                // -1 = doesn't have an MP3 sample rate (e.g. WAV file, etc.)

    QString currentSongIdentifier; // MD5 hash of bytes of MP3 data, i.e. NOT including the ID3v2 tag

    uint32_t currentSongID; // 32-bit MD5 hash of all bytes except the ID3v1 and ID3v2 tags

    int randCallIndex;     // for Flash Calls

    void writeCuesheet(QString filename);
    void saveCurrentSongSettings();
    void loadSettingsForSong(QString songTitle);
    bool compareCuesheetPathNamesRelative(QString str1, QString str2);
    QString convertCuesheetPathNameToCurrentRoot(QString str1);

    void loadGlobalSettingsForSong(QString songTitle); // settings that must be set after song is loaded

    void randomizeFlashCall();

    QString filepath2SongCategoryName(QString MP3Filename);  // returns the CATEGORY name (as a string).  patter, hoedown -> "patter", as per user prefs

    int getRsyncFileCount(QString sourceDir, QString destDir);

    // ID3 ==============
    int readID3Tags(QString fileName, double *bpm, double *tbpm, uint32_t *loopStartSamples, uint32_t *loopLengthSamples);
    void printID3Tags(QString fileName);

    double getID3BPM(QString MP3FileName);      // gets the BPM (should be same as readID3Tags)
    int getMP3SampleRate(QString fileName);  // gets the SAMPLERATE (either 44100 or 48000 for MP3)

    // ----------------------------
    void reloadCurrentMP3File();
    void loadMP3File(QString filepath, QString songTitle, QString songCategory, QString songLabel, QString nextFilename="");
    void secondHalfOfLoad(QString songTitle);  // after we have duration and BPM, execute this

    void maybeLoadCSSfileIntoTextBrowser(bool useSquareDeskCSS);
    void loadCuesheet(const QString cuesheetFilename);
    bool loadCuesheets(const QString &MP3FileName, const QString preferredCuesheet = QString(), QString nextFilename="");

    void findPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets);
    void betterFindPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets);

    bool breakFilenameIntoParts(const QString &s, QString &label, QString &labelnum, QString &labenum_extra,
                                QString &title, QString &shortTitle );

    int MP3FilenameVsCuesheetnameScore(QString fn, QString cn, QTextEdit *debugOut = nullptr);

    bool cuesheetIsUnlockedForEditing;

    void findMusic(QString mainRootDir, bool refreshDatabase);    // get the filenames into pathStack, pathStackCuesheets
    void updateTreeWidget();
    void filterMusic();  // filter them into the songTable
    void loadMusicList();  // filter them into the songTable
    void darkFilterMusic();  // filter them into the songTable
    void darkLoadMusicList(QList<QString> *aPathStack, QString typeFilter, bool forceTypeFilter, bool reloadPaletteSlots, bool suppressSelectionChange = false);  // filter one of the pathstacks into the darkSongTable
    QString FormatTitlePlusTags(const QString &title, bool setTags, const QString &strtags, QString titleColor = "");

    void changeTagOnCurrentSongSelection(QString tag, bool add);
    void darkChangeTagOnCurrentSongSelection(QString tag, bool add);
    void removeAllTagsFromSong();
    void removeAllTagsFromSongRow(int row);
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
    QString loadPlaylistFromFileToPaletteSlot(QString PlaylistFileName, int slotNumber, int &songCount); // returns error song string and songCount
    void loadPlaylistFromFileToSlot(int whichSlot);   // ask user which file, load file into slot
    void printPlaylistFromSlot(int whichSlot);        // show print dialog to user, print this playlist (not tracks)
    void updateRecentPlaylistsList(const QString &playlistPath); // update the recent playlists list

    void refreshAllPlaylists();  // used when View > TAGS changes

    // APPLE MUSIC PLAYLISTS ---------
    void getAppleMusicPlaylists(); // get only the user-defined playlists

    // LOCAL PLAYLISTS ---------
    void getLocalPlaylists();

    QList<QStringList> allAppleMusicPlaylists; // 3 cols: playlistName, title, pathname
    QStringList allAppleMusicPlaylistNames;

    // Lyrics stuff ----------
    QString loadLyrics(QString MP3FileName);
    int lyricsTabNumber;
    bool hasLyrics;

    QString txtToHTMLlyrics(QString text, QString filePathname);

    QList<QString> *pathStack; // for local songs
    QList<QString> *pathStackCuesheets; // for lyrics/cuesheets only
    QList<QString> *pathStackPlaylists; // for LOCAL playlist songs only
    QList<QString> *pathStackApplePlaylists; // for APPLE MUSIC playlist songs only

    QList<QString> *currentlyShowingPathStack;
    QString currentTypeFilter;
    QString currentTreePath; // e.g. "Track/patter", "Playlists/CPSD/" "Apple Music/CuriousBlend"
    bool forceFilter;

    bool trapKeypresses;
    QString reverseLabelTitle;

    void saveCheckBoxState(const char *key_string, QCheckBox *checkBox);
    void restoreCheckBoxState(const char *key_string, QCheckBox *checkBox,
                              bool checkedDefault);

    QString removePrefix(QString prefix, QString s);

    void updateSongTableColumnView();

    int selectedSongRow();  // returns -1 if none selected
    int previousVisibleSongRow();   // previous visible row or -1
    int nextVisibleSongRow();       // return next visible row or -1
    int darkPreviousVisibleSongRow();   // previous visible row or -1
    int darkNextVisibleSongRow();       // return next visible row or -1
    // int PlaylistItemCount(); // returns the number of items in the currently loaded playlist
    int getSelectionRowForFilename(const QString &filePath);

    int darkGetSelectionRowForFilename(const QString &filePath);
    int darkSelectedSongRow();  // returns -1 if none selected

    // Song types
    QStringList songTypeNamesForPatter;
    QStringList songTypeNamesForSinging;
    QStringList songTypeNamesForCalled;
    QStringList songTypeNamesForExtras;

    QStringList songTypeToggleList;

    // VU Meter support
    QTimer *UIUpdateTimer;
    QTimer *vuMeterTimer;

    QTimer *fileWatcherTimer;           // after all changes are made, THEN reload the songTable.
    QTimer *fileWatcherDisabledTimer;   // disable the filewatcher for 5 sec after a LOAD operation (workaround Ventura problem)

    QTimer *playlistSlotWatcherTimer;   // after all changes are made, THEN auto-save the playlist slot.

    LevelMeter *vuMeter;

    QString patterColorString, singingColorString, calledColorString, extrasColorString;  // current values

    QSet<QString> pathsOfCalledSongs; // full path is in this set, if the song has been used (Recent == "*")

    // experimental break and patter timers
    bool tipLengthTimerEnabled, breakLengthTimerEnabled, tipLength30secEnabled;
    int tipLengthTimerLength, breakLengthTimerLength;
    int tipLengthAlarmAction, breakLengthAlarmAction;

#ifdef Q_OS_MAC
    MacUtils macUtils;  // singleton
#endif

    QFileSystemWatcher *fileWatcher;
    bool filewatcherShouldIgnoreOneFileSave;
    bool filewatcherIsTemporarilyDisabled;  // to workaround the Ventura extended attribute problem

#ifdef DEBUG_LIGHT_MODE
    QFileSystemWatcher lightModeWatcher;
#endif

    QStringList flashCalls;

    // --------------
    void initSDtab();
    void initReftab();

    QString currentSDVUILevel;
    QString currentSDKeyboardLevel;

    Highlighter *highlighter;
    QString uneditedData;
    QString editedData;
    QString copyrightText;  // sd copyright string (shown once at start)
    bool copyrightShown;

    bool voiceInputEnabled;

    SongSettings songSettings;
    PreferencesManager prefsManager;

    int lastMinuteInHour; // for updating sessions
    int lastSessionID;    // for updating sessions
    int currentSongSecondsPlayed; // for tracking 15 seconds before marking played
    bool currentSongSecondsPlayedRecorded; // true if it was recorded in DB

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

    // sound fx
    QMap<int, QString> soundFXfilenames;    // e.g. "9.foo.mp3" --> [9,"9.foo.mp3"]
    QMap<int, QString> soundFXname;         // e.g. "9.foo.mp3" --> [9,"foo"]
    void maybeInstallSoundFX();
    void maybeInstallReferencefiles();
    void maybeInstallTemplates();

    void maybeMakeRequiredFolder(QString folderName);  // make it, if it doesn't already exist
    void maybeMakeAllRequiredFolders();                // make them all, if they doesn't already exist

    int totalZoom;  // total zoom for Lyrics pane, so it can be undone with a Reset Zoom

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

    bool slotModified[3];               // true, if the slot has been modified and needs to be saved

    void darkPaletteTitleLabelDoubleClicked(QMouseEvent *e);
    void setTitleField(QTableWidget *whichTable, int whichRow, QString fullPath,
                       bool isPlaylist, QString PlaylistFileName, QString theRealPath = "");

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

    void actionToggleCuesheetAutoscroll();

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

    bool sdSliderSidesAreSwapped;

    int sdLastLine;
    int sdLastNonEmptyLine;
    int sdUndoToLine;
    bool sdWasNotDrawingPicture;
    bool sdHasSubsidiaryCallContinuation;
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

    int leftGroup, topGroup;        // groupness numbers, 0 = P, 1 = RH, 2 = O, 3 = C
    Order boyOrder, girlOrder;      // order: 0 = in order, 1 = out of order, 2 = unknown order

    QAction **danceProgramActions;

    void setSDCoupleColoringScheme(const QString &scheme);
    void setSDCoupleGenderingScheme(const QString &scheme);

    QString get_current_sd_sequence_as_html(bool all_rows, bool graphics_as_text);
    void render_current_sd_scene_to_tableWidgetCurrentSequence(int row, const QString &formation);
    void set_current_sequence_icons_visible(bool visible);
    QShortcut *shortcutSDTabUndo;
    QShortcut *shortcutSDTabRedo;
    QShortcut *shortcutSDCurrentSequenceSelectAll;
    QShortcut *shortcutSDCurrentSequenceCopy;
    SDRedoStack *sd_redo_stack;

    QString upperCaseWHO(QString call); // heads --> HEADS, etc.
    QString prettify(QString call);

    double sd_animation_t_value;
    double sd_animation_delta_t;
    double sd_animation_msecs_per_frame;
    void render_sd_item_data(QTableWidgetItem *item);
    void SetAnimationSpeed(AnimationSpeed speed);
    void set_sd_last_formation_name(const QString&);
    void set_sd_last_groupness(); // update groupness strings

    bool compareRelative(QString s1, QString s2);  // compare pathnames relative to MusicDir

private:
    void decode_formation_into_dancer_destinations(const QStringList &sdformation, QList<SDDancer> &sdpeople);

    // ORDER:
    void inOrder(struct dancer dancers[]);
    Order whichOrder(double p1_x, double p1_y, double p2_x, double p2_y);

    // GROUPness:
    bool sameGroup(int group1, int group2);
    int whichGroup (struct dancer dancers[], bool top);
    int groupNum(struct dancer dancers[], bool top, int coupleNum, int gender);

    QString render_image_item_as_html(QTableWidgetItem *imageItem, QGraphicsScene &scene, QList<SDDancer> &people, bool graphics_as_text);

    void reset_sd_dancer_locations();
    void startSDThread(dance_level dance_program);
    void restartSDThread(dance_level dance_program);
public:
    void do_sd_tab_completion();
    void setCurrentSDDanceProgram(dance_level);
    dance_level get_current_sd_dance_program();
    void setSDCoupleNumberingScheme(const QString &scheme);
    void setPeopleNumberingScheme(QList<SDDancer> &sdpeople, const QString &numberScheme);
    QString sdLastFormationName;

    // one file = one frame, length(frameFiles) = F<max> key assignment
    // TODO: these will be saved as preferences eventually, or maybe these will somewhere in the sqlite DB
    QString     frameName;    // name of the currently loaded DANCE: sd/dances/<frameName>/{biggie,easy,med,hard).txt
    QStringList frameFiles;   // list of which files are assigned to keys F1-F10, e.g. "ceder/basic", "mpogue/hard"
    QStringList frameVisible; // strings representing frame visibility/placement, e.g. ["sidebar", "", "central"], there must be exactly one "central" and 3 "sidebar"
//    QStringList frameLevel;   // strings representing the level ["basic", "ssd", "ms", "plus", "a1", "a2", "c1"] of the frame, e.g. ["basic", "plus", ...]
    QVector<int> frameCurSeq;
    QVector<int> frameMaxSeq;
    bool selectFirstItemOnLoad;

    void SDReadSequencesUsed();      // update the sequenceStatus hash table from the persisted log in sequencesUsed.csv
    QHash<int, int> sequenceStatus;  // maps a sequence number to a status (0 = good, else # = bad reason), and not present = not rated

    int     currentFrameNumber;   // e.g. 0
    QString currentFrameTextName; // e.g. biggie
    QString currentFrameHTMLName; // e.g. <HTML>...F6...local.plus</HTML>

    bool SDtestmode;

    void refreshSDframes();
    void loadFrame(int i, QString filename, int seqNum, QListWidget *list);  // loads a specified frame from a file in <musicDir>/sd/ into a list widget (or a table, if list == null)

    int userID;  // Globally Likely Unique ID for user, range: 1 - 21473
    int nextSequenceID;  // starts at 1 for a given userID, then increments by 1 each time NEW is clicked
    QString authorID;  // use this instead of userID going forward
    void getMetadata(); // fetch the GLUID etc from .squaredesk/metadata.csv
    void writeMetadata(int userID, int nextSequenceID, QString authorID);

    QString currentSequenceRecordNumber;  // REC of currently loaded sequence
    int currentSequenceRecordNumberi;     //  int version of that
    QString currentSequenceAuthor;        // AUTHOR of currently loaded sequence
    QString currentSequenceTitle;         // TITLE of currently loaded sequence

    void debugCSDSfile(QString frameName); // DEBUG
    bool checkSDforErrors(); // DEBUG

    QString translateCall(QString call);

    QString translateCallToLevel(QString thePrettifiedCall);
    QString makeLevelString(const char *levelCalls[]);

    bool newSequenceInProgress;
    bool editSequenceInProgress;

    QStringList highlightedCalls;

    bool lastFlashcall;  // cached previous value of flashcall

    bool darkmode; // true if we're using the new dark UX

    QIcon *darkStopIcon;
    QIcon *darkPlayIcon;
    QIcon *darkPauseIcon;

    float *waveform;

    QList<int> currentSplitterSizes; // save for later restore

    MyTableWidget *sourceForLoadedSong; // points at what the user double-clicked to load a song (4 possible sources)

//    QString editingArrowStart = "✏";  // indicates that this item can have pitch/tempo edited
    QString editingArrowStart = "➡";   // U+27A1

#ifdef TESTRESTARTINGSQUAREDESK
    bool testRestartingSquareDesk;
    QTimer testRestartingSquareDeskTimer;
#endif

    // new type stuff
    void setCurrentSongMetadata(QString type);  // sets all the current* from the pathname

    // template menu for cuesheet
    QMenu *templateMenu;
    void newFromTemplate();

    // THEME STUFF ----------
    QString currentAnalogClockState;

#ifdef USE_JUCE
    // JUCE STUFF -----------
    std::unique_ptr<juce::AudioPluginInstance> loudMaxPlugin;
    // juce::AudioPluginInstance *loudMaxPlugin;
    juce::HostedAudioProcessorParameter *paramThresh;
    // juce::DocumentWindow *loudMaxWin;
    std::unique_ptr<juce::DocumentWindow> loudMaxWin;
    void scanForPlugins();

    QString getCurrentLoudMaxSettings();
    void setLoudMaxFromPersistedSettings(QString s);
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

// ----------------------
class SelectionRetainer {
    QTextEdit *textEdit;
    QTextCursor cursor;
    int anchorPosition;
    int cursorPosition;
    // No inadvertent copies
private:
    SelectionRetainer() {};
    SelectionRetainer(const SelectionRetainer &) {};
public:
    SelectionRetainer(QTextEdit *textEdit) : textEdit(textEdit), cursor(textEdit->textCursor())
    {
        anchorPosition = cursor.anchor();
        cursorPosition = cursor.position();
    }
    ~SelectionRetainer() {
        cursor.setPosition(anchorPosition);
        cursor.setPosition(cursorPosition, QTextCursor::KeepAnchor);
        textEdit->setTextCursor(cursor);
    }
};

// -----------------------
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
struct PlaylistExportRecord
{
    int index;
    QString title;
    QString pitch;
    QString tempo;
};
#pragma clang diagnostic pop

#endif // MAINWINDOW_H
