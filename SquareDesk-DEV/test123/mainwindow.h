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
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QProcess>
#include <QProxyStyle>
#include <QSettings>
#include <QSlider>
#include <QStack>
#include <QTableWidgetItem>
#include <QToolTip>
#include <QVariant>
#include <QWheelEvent>
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
#endif

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
    QActionGroup *sdActionGroup1;

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    void on_loopButton_toggled(bool checked);
    void on_monoButton_toggled(bool checked);

private slots:
    void sdActionTriggered(QAction * action);

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

    void on_pushButtonCountDownTimerStartStop_clicked();
    void on_pushButtonCountDownTimerReset_clicked();
    void on_pushButtonCountUpTimerStartStop_clicked();
    void on_pushButtonCountUpTimerReset_clicked();

    void on_checkBoxPlayOnEnd_clicked();
    void on_checkBoxStartOnPlay_clicked();
    void on_pushButtonSetIntroTime_clicked();
    void on_pushButtonSetOutroTime_clicked();
    void on_seekBarCuesheet_valueChanged(int);

    void on_pushButtonShowHideCueSheetAdditional_clicked();
    void on_lineEditOutroTime_textChanged();
    void on_lineEditIntroTime_textChanged();
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

    void on_warningLabel_clicked();

    void on_tabWidget_currentChanged(int index);

    void writeSDData(const QByteArray &data);
    void readSDData();
    void readPSData();

    void on_actionEnable_voice_input_toggled(bool arg1);
    void microphoneStatusUpdate();


    void on_actionPractice_triggered(bool checked);
    void on_actionMonday_triggered(bool checked);
    void on_actionTuesday_triggered(bool checked);
    void on_actionWednesday_triggered(bool checked);
    void on_actionThursday_triggered(bool checked);
    void on_actionFriday_triggered(bool checked);
    void on_actionSaturday_triggered(bool checked);
    void on_actionSunday_triggered(bool checked);

    void on_actionCompact_triggered(bool checked);
    void on_actionAuto_scroll_during_playback_toggled(bool arg1);

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

private:
    QAction *closeAct;  // WINDOWS only

    int iFontsize;  // preferred font size (for eyeballs that can use some help)
    bool inPreferencesDialog;
    QString musicRootPath, guestRootPath, guestVolume, guestMode;
    enum SongFilenameMatchingType songFilenameFormat;

    bool showTimersTab;         // EXPERIMENTAL TIMERS STUFF
    bool showLyricsTab;         // EXPERIMENTAL LYRICS STUFF
    bool pitchAndTempoHidden;   // EXPERIMENTAL PITCH/TEMPO VIEW STUFF
    bool clockColoringHidden;   // EXPERIMENTAL CLOCK COLORING STUFF

    QMap<int,QPair<QWidget *,QString> > tabmap; // keep track of experimental tabs

    unsigned char currentState;
    short int currentPitch;
    unsigned short currentVolume;
    int previousVolume;

    bool tempoIsBPM;

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

    void saveCurrentSongSettings();
    void loadSettingsForSong(QString songTitle);

    void loadMP3File(QString filepath, QString songTitle, QString songType);
    void loadCuesheet(const QString &cuesheetFilename);
    void loadCuesheets(const QString &MP3FileName);
    void findPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets);
    bool breakFilenameIntoParts(const QString &s, QString &label, QString &labelnum, QString &title, QString &shortTitle );

    void findMusic(QString mainRootDir, QString guestRootDir, QString mode, bool refreshDatabase);    // get the filenames into pathStack
    void filterMusic();  // filter them into the songTable
    void loadMusicList();  // filter them into the songTable

    void sortByDefaultSortOrder();  // sort songTable by default order (not including # column)

#if defined(Q_OS_MAC) | defined(Q_OS_WIN32)
    // Lyrics stuff
    QString loadLyrics(QString MP3FileName);
#endif
    int lyricsTabNumber;
    bool hasLyrics;

    QString txtToHTMLlyrics(QString text, QString filePathname);

    QList<QString> *pathStack;

    bool saveSongPreferencesInConfig;
    // Experimental Timer stuff
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

    void updatePitchTempoView();
    void setFontSizes();

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
    bool tipLengthTimerEnabled, breakLengthTimerEnabled;
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
    void initSDtab();

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
    void setCurrentSessionId(int id);
    void setCurrentSessionIdReloadMusic(int id);
    bool autoScrollLyricsEnabled;
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
