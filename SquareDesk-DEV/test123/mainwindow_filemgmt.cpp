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

// FILE MANAGEMENT FUNCTIONS

#include "globaldefines.h"

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

// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"
// #include "utility.h"
#include "perftimer.h"
#include "tablenumberitem.h"
// #include "tablelabelitem.h"
// #include "importdialog.h"
#include "exportdialog.h"
#include "songhistoryexportdialog.h"
#include "calllistcheckbox.h"
// #include "sessioninfo.h"
#include "songtitlelabel.h"
// #include "tablewidgettimingitem.h"
// #include "danceprograms.h"
#include "startupwizard.h"
#include "makeflashdrivewizard.h"
// #include "downloadmanager.h"
#include "songlistmodel.h"
#include "mytablewidget.h"

#include "svgWaveformSlider.h"
#include "auditionbutton.h"

// #include "src/communicator.h"

#if defined(Q_OS_MAC) | defined(Q_OS_WIN)
#ifndef M1MAC
#include "JlCompress.h"
#endif
#endif

// #include <iostream>
// #include <sstream>
// #include <iomanip>

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
#include <string>

// #include "typetracker.h"
using namespace TagLib;

#include <QProxyStyle>

static const char *music_file_extensions[] = { "mp3", "wav", "m4a", "flac" };       // NOTE: must use Qt::CaseInsensitive compares for these
static const char *cuesheet_file_extensions[3] = { "htm", "html", "txt" };          // NOTE: must use Qt::CaseInsensitive compares for these

extern flexible_audio *cBass;  // make this accessible to PreferencesDialog

// TITLE COLORS

static QRegularExpression spanPrefixRemover("<span style=\"color:.*\">(.*)</span>", QRegularExpression::InvertedGreedinessOption);
static QRegularExpression title_tags_remover("(\\&nbsp\\;)*\\<\\/?span( .*?)?>");

// QString title_tags_prefix("&nbsp;<span style=\"background-color:%1; color: %2;\"> ");
// QString title_tags_suffix(" </span>");
extern QString title_tags_prefix;
extern QString title_tags_suffix;

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

// -----------------------------------------------------------------------------
void MainWindow::reloadCurrentMP3File() {
    // if there is a song loaded, reload it (to pick up, e.g. new cuesheets)
    if ((currentMP3filenameWithPath != "") && (currentSongTitle != "") && (currentSongTypeName != "")) {
//        qDebug() << "reloading song: " << currentMP3filename;
        // loadMP3File(currentMP3filenameWithPath, currentSongTitle, currentSongTypeName, currentSongLabel);
        loadMP3File(currentMP3filenameWithPath, currentSongTitle, currentSongTypeName, "NA");
    }
}

void MainWindow::loadMP3File(QString MP3FileName, QString songTitle, QString songType, QString songLabel, QString nextFilename)
{
    Q_UNUSED(songLabel)
    on_loopButton_toggled(false); // when we load a new file, make sure looping is turned OFF (turn it on when needed for patter)

    // loadTimer.start();
    // qDebug() << "loadMP3File: nextFilename = " << nextFilename;
    override_filename = "";
    override_cuesheet = "";
    ui->darkSegmentButton->setHidden(true);

#ifdef DEBUG_LIGHT_MODE
    svgClockStateChanged("UNINITIALIZED");
#endif
    // allow ID3 only on MP3 files right now!
    ui->actionUpdate_ID3_Tags->setEnabled(MP3FileName.endsWith(".mp3", Qt::CaseInsensitive));

    PerfTimer t("loadMP3File", __LINE__);

    // ***** NO. We decided that the first part after the musicRootPath is the type, and that's the songType passed in to us in loadMP3File.
    // // override songType, and just look at the path now
    // QStringList pathParts = MP3FileName.split("/");
    // songType = pathParts[pathParts.size()-2]; // second-to-last path part is the assumed type

    // ***** ***** OK, itemDoubleClicked is passing in the wrong type, so we're going to override
    //  that here.  The songType passed in HERE is the songType in the songTable, but there's one use case
    //  where that string is NOT the actual song type, and that's when the songTable is showing a playlist.
    //  In that case, it will start with a greek letter, in which case we KNOW it's from a playlist.
    //  Also, if it starts with an APPLE, then we know it's from Apple Music.  The Apple Music ones are
    //  already overridden as type = "EXTRAS", and colored red by default.

    // qDebug() << "loadMP3File::songType: " << songType << songType[0];

    if (songType[0] == QChar(0x039E) || songType[0] == QChar(0x03BE)) {
        // we're loading a file from the songTable that came from a playlist.
        // ignore the songType and replace it with the real songType.
        // NOTE: Maybe we should just ALWAYS override the songType and use the type extracted from the path here?

        QString thePath = MP3FileName;
        thePath.replace(musicRootPath + "/", "");
        songType = thePath.split("/")[0];
        // qDebug() << "loadMP3File: " << thePath << "new songType" << songType;
    }

    setCurrentSongMetadata(songType); // set current* based on the type extracted from the pathname, for later use all over the place

    //currentSongTypeName = songType;     // save it for session coloring on the analog clock later...
    // currentSongType is referenced in loadCuesheets
    if (!loadCuesheets(MP3FileName, "", nextFilename)) {
        // load cuesheets up here first, so that the original pathname is used, rather than the pointed-to (rewritten) pathname.
        //   A symlink or alias in the Singing folder pointing at the real file in the Patter folder should work now.
        //   A symlink or alias in the Patter folder pointing at the real file in the Singing folder should also work now.
        //   In both cases: as Singer, it has lyrics, and as Patter, it has looping and no lyrics.
        return;  // if the user cancelled the load in the "you have unsaved cuesheet changes" dialog, abort the load.
    }

    songLoaded = false;  // seekBar updates are disabled, while we are loading

    ui->darkSeekBar->setWholeTrackPeak(1.0); // disable waveform scaling until after we know scale

    filewatcherIsTemporarilyDisabled = true;  // disable the FileWatcher for a few seconds to workaround the Ventura extended attribute problem
    fileWatcherDisabledTimer->start(std::chrono::milliseconds(5000)); // re-enable the FileWatcher after 5s
//    qDebug() << "***** filewatcher is disabled for 5 seconds!";

//    qDebug() << "MainWindow::loadMP3File: songTitle: " << songTitle << ", songType: " << songType << ", songLabel: " << songLabel;
//    RecursionGuard recursion_guard(loadingSong);
    loadingSong = true;
    firstTimeSongIsPlayed = true;

    currentMP3filenameWithPath = MP3FileName;

    // currentSongLabel = songLabel;   // remember it, in case we need it later

    // resolve aliases at load time, rather than findFilesRecursively time, because it's MUCH faster
    QFileInfo fi(MP3FileName);
    QString resolvedFilePath = fi.symLinkTarget(); // path with the symbolic links followed/removed
    if (resolvedFilePath != "") {
        MP3FileName = resolvedFilePath;
    }

    QFileInfo f(MP3FileName);
    if (!f.exists()) {
        // sometimes an old playlist will point at items that do not exist anymore
        QMessageBox msgBox;
        msgBox.setText(QString("ERROR: Can't find:\n") + MP3FileName);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        return;
    }

    t.elapsed(__LINE__);

    ui->pushButtonEditLyrics->setChecked(false); // lyrics/cuesheets of new songs when loaded default to NOT editable

    ui->pushButtonCueSheetEditSave->hide();   // the two save buttons are now invisible
    ui->actionSave_Cuesheet->setEnabled(false);  // if locked, we can't Cuesheet > Save Cuesheet

    ui->pushButtonCueSheetEditSaveAs->hide();
    ui->actionSave_Cuesheet->setEnabled(false);  // FIX: we can Cuesheet > Save Cuesheet As, if there is a cuesheet

    ui->pushButtonRevertEdits->hide();   // the revert edits buttons is also now invisible

    ui->pushButtonEditLyrics->show();  // and the "unlock for editing" button shows up!
    ui->pushButtonNewFromTemplate->show(); // allow creating new cuesheets from templates in the "lyrics" directory

    QStringList pieces = MP3FileName.split( "/" );
    QString filebase = pieces.value(pieces.length()-1);
    currentMP3filename = filebase;
    int lastDot = currentMP3filename.lastIndexOf('.');
    if (lastDot > 1)
    {
        currentMP3filename = currentMP3filename.right(lastDot - 1);
    }

    if (songTitle != "") {
        // qDebug() << "loadMP3File: " << songTitle;
        setNowPlayingLabelWithColor(songTitle);
    }
    else {
        setNowPlayingLabelWithColor(currentMP3filename);
    }
    currentSongTitle = ui->darkTitle->text();  // save, in case we are Flash Calling

    // now clear out the waveform (if there is one)
    // qDebug() << "updateBgPixmap called from loadMP3File with nullptr, 0 to clear out";
    ui->darkSeekBar->updateBgPixmap(nullptr, 0); // this means clear it out!
//    QDir md(MP3FileName);
//    QString canonicalFN = md.canonicalPath();

    t.elapsed(__LINE__);

    // let's do a quick preview (takes <1ms), to see if the intro/outro are already set.
    SongSetting settings1;
    double intro1 = 0.0;
    double outro1 = 0.0;
    // bool isSetIntro1 = false;
    // bool isSetOutro1 = false;
    if (songSettings.loadSettings(currentMP3filenameWithPath, settings1)) {
        if (settings1.isSetIntroPos()) {
            intro1 = settings1.getIntroPos();
            // isSetIntro1 = true;
            // qDebug() << "loadMP3File: intro was set to: " << intro1 << isSetIntro1;
        }
        if (settings1.isSetOutroPos()) {
            outro1 = settings1.getOutroPos();
            // isSetOutro1 = true;
            // qDebug() << "loadMP3File: outtro was set to: " << outro1 << isSetOutro1;
        }
    }

    // TODO: intro1 and outro1 are NOT used in cBass anymore
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

    // ui->playButton->setEnabled(true);
    // ui->stopButton->setEnabled(true);

    ui->darkPlayButton->setEnabled(true);
    ui->darkStopButton->setEnabled(true);
    ui->actionPlay->setEnabled(true);
    ui->actionStop->setEnabled(true);
    ui->actionSkip_Forward->setEnabled(true);
    ui->actionSkip_Backward->setEnabled(true);

    // ui->seekBar->setEnabled(true);
    ui->seekBarCuesheet->setEnabled(true);

    // emit ui->pitchSlider->valueChanged(ui->pitchSlider->value());    // force pitch change, if pitch slider preset before load
    // emit ui->volumeSlider->valueChanged(ui->volumeSlider->value());  // force vol change, if vol slider preset before load
    // emit ui->mixSlider->valueChanged(ui->mixSlider->value());        // force mix change, if mix slider preset before load

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

    // emit ui->bassSlider->valueChanged(ui->bassSlider->value()); // force bass change, if bass slider preset before load
    // emit ui->midrangeSlider->valueChanged(
    //     ui->midrangeSlider->value()); // force midrange change, if midrange slider preset before load
    // emit ui->trebleSlider->valueChanged(ui->trebleSlider->value()); // force treble change, if treble slider preset before load

    cBass->Stop();

#ifdef Q_OS_MACOS
    // Update Now Playing info immediately after loading to register as active media app
    // This allows F8 to work even before the first play
    updateNowPlayingMetadata();
    // printf("Updated Now Playing metadata after song load to enable F8 key\n");
#endif

}

// ======================================================================
void findFilesRecursively(QDir rootDir, QList<QString> *pathStack, QList<QString> *pathStackCuesheets, QList<QString> *pathStackReference, QString suffix, Ui::MainWindow *ui, QMap<int, QString> *soundFXarray, QMap<int, QString> *soundFXname)
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
            if (newType == "reference") {
                // qDebug() << "pathStackReference adding:" << resolvedFilePath;
                pathStackReference->append(newType + "#!#" + resolvedFilePath);
            } else if (newType != "sd" && newType != "choreography") {
                if (newType == "lyrics" || resolvedFilePath.endsWith(".html") || resolvedFilePath.endsWith(".htm")) {
                    pathStackCuesheets->append(newType + "#!#" + resolvedFilePath);
                } else {
                    pathStack->append(newType + "#!#" + resolvedFilePath);
                }
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

void MainWindow::findMusic(QString mainRootDir, bool refreshDatabase)
{
    // qDebug() << "***** findMusic";
    PerfTimer t("findMusic", __LINE__);
    t.start(__LINE__);

    QString databaseDir(mainRootDir + "/.squaredesk");

    if (refreshDatabase)
    {
        songSettings.openDatabase(databaseDir, mainRootDir, false);
    }
    t.elapsed(__LINE__);

    // // always gets rid of the old pathstack and pathStackCuesheets
    pathStack->clear();
    pathStackCuesheets->clear();

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

    t.elapsed(__LINE__);

    findFilesRecursively(rootDir1, pathStack, pathStackCuesheets, pathStackReference, "", ui, &soundFXfilenames, &soundFXname);  // appends to the pathstack

    t.elapsed(__LINE__);

    // APPLE MUSIC ------------
    if (prefsManager.GetenableAppleMusic()) {
        getAppleMusicPlaylists(); // and add them to the pathStackPlaylists, with newType == "AppleMusicPlaylistName$!$AppleMusicTitle", and fullPath = path to MP3 file on disk
    }

    t.elapsed(__LINE__);

    // LOCAL SQUAREDESK PLAYLISTS -------
    // getLocalPlaylists();  // and add them to the pathStackPlaylists, with newType == "PlaylistName%!% ", and fullPath = path to MP3 file on disk
    // t.elapsed(__LINE__);

    // // updateTreeWidget will reload the local Playlists, just clear it out first  NO
    // pathStackPlaylists->clear();  NO

    // updateTreeWidget() now clears the pathStackPlaylists, because it always reloads it
    updateTreeWidget(); // this will also show the Apple Music playlists, found just now
    t.elapsed(__LINE__);
    t.stop();

    // // DEBUG DEBUG DEBUG =========
    // for (const auto &a : *pathStack) {
    //     if (a.contains("Baker")) {
    //         qDebug() << "pathStack:" << a;
    //     }
    // }
    // for (const auto &a : *pathStackCuesheets) {
    //     if (a.contains("Baker")) {
    //         qDebug() << "pathStackCuesheets:" << a;
    //     }
    // }
    // // ===========================
}


void MainWindow::filterMusic()
{
    // OBSOLETE
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
void MainWindow::darkFilterMusic()
{
    // qDebug() << "darkFilterMusic()" << typeSearch << labelSearch << titleSearch << searchAllFields;

    PerfTimer t("darkFilterMusic", __LINE__);
    t.start(__LINE__);

    //    static QRegularExpression rx("(\\ |\\,|\\.|\\:|\\t\\')"); //RegEx for ' ' or ',' or '.' or ':' or '\t', includes ' to handle the "it's" case.
    // static QRegularExpression rx("(\\ |\\,|\\.|\\:|\\t)"); //RegEx for ' ' or ',' or '.' or ':' or '\t', does NOT include ' now
    static QRegularExpression rx("(\\ |\\,|\\:|\\t)"); //RegEx for ' ' or ',' or '.' or ':' or '\t', does NOT include ' or . now

    QStringList label = labelSearch.split(rx);
    QStringList type  = typeSearch.split(rx);
    QStringList title = titleSearch.split(rx);

    //    qDebug() << "filterMusic: title: " << title;
    ui->darkSongTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);  // DO NOT SET height of rows (for now)

    ui->darkSongTable->setSortingEnabled(false);

    int initialRowCount = ui->darkSongTable->rowCount();
    int rowsVisible = initialRowCount;
    int firstVisibleRow = -1;
    for (int i=0; i<ui->darkSongTable->rowCount(); i++) {
//        QString songTitle = getTitleColText(ui->darkSongTable, i);
        QString songTitle = dynamic_cast<QLabel*>(ui->darkSongTable->cellWidget(i, kTitleCol))->text();

        QString songType = ui->darkSongTable->item(i,kTypeCol)->text();
        QString songLabel = ui->darkSongTable->item(i,kLabelCol)->text();

        bool show = true;

        // if (!filterContains(songLabel,label)
        //     || !filterContains(songType, type)
        //     || !filterContains(songTitle, title))
        // {
        //     show = false;
        // }

        // qDebug() << "filter: " << searchAllFields << songTitle << title << filterContains(songTitle, title);
        // show = (searchAllFields
        //           ? filterContains(songLabel,label) || filterContains(songType, type) || filterContains(songTitle, title)
        //           : !(!filterContains(songLabel,label) || !filterContains(songType, type) || !filterContains(songTitle, title))
        //             );

        if (!searchAllFields) {
            if (!filterContains(songLabel,label) || !filterContains(songType, type) || !filterContains(songTitle, title))
            {
                show = false;
            }
        } else {
            // search all fields with a single word, e.g. "foo",
            // in this case, the titleSearch variable contains the thing to search for
            if (!filterContains(songLabel,title) && !filterContains(songType, title) && !filterContains(songTitle, title)) {
                show = false;
            }
        }

        ui->darkSongTable->setRowHidden(i, !show);
        rowsVisible -= (show ? 0 : 1); // decrement row count, if hidden
        if (show && firstVisibleRow == -1) {
            firstVisibleRow = i;
        }
    }
    ui->darkSongTable->setSortingEnabled(true);

    t.elapsed(__LINE__);

    // qDebug() << "darkFilterMusic::firstVisibleRow: " << firstVisibleRow;

    if (rowsVisible > 0) {
        // qDebug() << "good select row!" << firstVisibleRow;
        ui->darkSongTable->selectRow(firstVisibleRow);
    }
    ui->darkSearch->setFocus();  // restore focus after selectRow

    t.stop();

//    ui->songTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);  // auto set height of rows
}

// --------------------------------------------------------------------------------

void MainWindow::loadMusicList()
{
    // OBSOLETE
}

// --------------------------------------------------------------------------------
// filter from a pathStack into the darkSongTable, BUT
//   nullptr: just refresh what's there (currentlyShowingPathStack)
//   pathStack, pathStackPlaylists: use one of these
//
// Note: if aPathStack == currentShowingPathStack, then don't do anything

void MainWindow::darkLoadMusicList(QList<QString> *aPathStack, QString typeFilter, bool forceFilter, bool reloadPaletteSlots, bool suppressSelectionChange)
{
    // qDebug() << "darkLoadMusicList: " << typeFilter << forceFilter << reloadPaletteSlots;

    // if (aPathStack != nullptr) {
    //     int size0 = aPathStack->size();
    //     qDebug() << "aPathStack.size" << size0;
    //     if (size0 > 0) {
    //             // qDebug() << "aPathStack[0]" << pathStack[0];
    //     }
    // } else {
    //     qDebug() << "aPathStack == nullptr";
    // }

    // if (currentlyShowingPathStack != nullptr) {
    //     int sizeA = currentlyShowingPathStack->size();
    //     qDebug() << "currentlyShowingPathStack.size" << sizeA;
    //     if (sizeA > 0) {
    //             // qDebug() << "pathStack[0]" << pathStack[0];
    //     }
    // } else {
    //     qDebug() << "currentlyShowingPathStack == nullptr";
    // }

    // if (pathStack != nullptr) {
    //     int size1 = pathStack->size();
    //     qDebug() << "pathStack.size" << size1;
    //     if (size1 > 0) {
    //             // qDebug() << "pathStack[0]" << pathStack[0];
    //     }
    // } else {
    //     qDebug() << "pathStack == nullptr";
    // }

    // if (pathStackPlaylists != nullptr) {
    //     int size2 = pathStackPlaylists->size();
    //     qDebug() << "pathStackPlaylists.size" << size2;
    //     if (size2 > 0) {
    //             // qDebug() << "pathStackPlaylists[0]" << pathStackPlaylists[0];
    //     }
    // } else {
    //     qDebug() << "pathStackPlaylists == nullptr";
    // }

    // if (pathStackApplePlaylists != nullptr) {
    //     int size3 = pathStackApplePlaylists->size();
    //     qDebug() << "pathStackApplePlaylists.size" << size3;
    //     if (size3 > 0) {
    //             // qDebug() << "pathStackApplePlaylists[0]" << pathStackApplePlaylists[0];
    //     }
    // } else {
    //     qDebug() << "pathStackApplePlaylists == nullptr";
    // }

    // if ((currentlyShowingPathStack != nullptr) && (aPathStack == currentlyShowingPathStack)) {
    //     return; // just return, because it's already up to date
    // }

    if (!forceFilter && (currentTypeFilter == typeFilter)) {
        return; // nothing to do, if not a forced refresh and what's showing is what was asked for
    }

    // if we get here, for sure we are refreshing the contents of the darkSongTable --------

    // qDebug() << "***** darkLoadMusicList()";
    PerfTimer t("darkLoadMusicList", __LINE__);
    t.start(__LINE__);

    // for performance ---
    ui->darkSongTable->hide();
    ui->darkSongTable->setSortingEnabled(false);
    ui->darkSongTable->blockSignals(true);  // block signals, so changes are not recursive

    // PLAYLIST HANDLING -----
    // Need to remember the PL# mapping here, and reapply it after the filter
    // left = path, right = number string
    // QMap<QString, QString> path2playlistNum;

    // // SONGTABLEREFACTOR
    // // Iterate over the songTable, saving the mapping in "path2playlistNum"
    // // TODO: optimization: save this once, rather than recreating each time.
    // for (int i=0; i<ui->songTable->rowCount(); i++) {
    //     QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
    //     QString playlistIndex = theItem->text();  // this is the playlist #
    //     QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();  // this is the full pathname
    //     if (playlistIndex != " " && playlistIndex != "") {
    //         // item HAS an index (that is, it is on the list, and has a place in the ordering)
    //         // TODO: reconcile int here with double elsewhere on insertion
    //         path2playlistNum[pathToMP3] = playlistIndex;
    //     }
    // }

    // clear out the table
    ui->darkSongTable->setRowCount(0);
    ui->darkSongTable->setColumnCount(8);

    QStringList m_TableHeader;
    m_TableHeader << "" << "Type" << "Label" << "Title" << "Recent" << "Age" << "Pitch" << "Tempo";
    ui->darkSongTable->setHorizontalHeaderLabels(m_TableHeader);
    ui->darkSongTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->darkSongTable->horizontalHeader()->setVisible(true);

    // if we passed in nullptr, just use whatever is currently loaded
    if (aPathStack == nullptr) {
        aPathStack = currentlyShowingPathStack; // just refresh the one that's already there (forced reload to pick up changes)
    }

    QListIterator<QString> iter(*aPathStack); // filter the one we were given (pathStack, pathStackPlaylists, pathStackApplePlaylists), OR refresh the last one

    QColor textCol = QColor::fromRgbF(0.0/255.0, 0.0/255.0, 0.0/255.0);  // defaults to Black
    bool show_all_ages = ui->actionShow_All_Ages->isChecked();

    // first, filter down to just those string that are the type we are looking for -------
    // QString startsWithTypeFilter(QString("^") + typeFilter + ".*");
    // static QRegularExpression typeFilterRegex(startsWithTypeFilter);
    // QStringList justMyType = aPathStack->filter(typeFilterRegex);

    bool takeAll = (typeFilter == "") || (typeFilter == "Tracks") || (typeFilter == "Playlists") || (typeFilter == "Apple Music"); // NOTE: lack of final slash, means this is top level request
    // qDebug() << "darkLoadMusicList filtering:" << typeFilter << takeAll;

    // NOTE: Items can look like:
    // "patter#!#/Users/mpogue/Library/Mobile Documents/com~apple~CloudDocs/SquareDance/squareDanceMusic_iCloud/patter/RIV 842 - Bluegrass Swing.mp3"
    // "singing#!#/Users/mpogue/Library/Mobile Documents/com~apple~CloudDocs/SquareDance/squareDanceMusic_iCloud/singing/HH 5363 -  I Just Called To Say I Love You.mp3"
    // "StarEights/StarEights_2024.07.21%!%0,126,03#!#/Users/mpogue/Library/Mobile Documents/com~apple~CloudDocs/SquareDance/squareDanceMusic_iCloud/patter/TBT 918 - Bad Guy.mp3"
    // "Second Playlist$!$04$!$Butterfly (Instrumental)#!#/Users/mpogue/Music/iTunes/iTunes Media/Music/Swingrowers/Butterfly - Single/02 Butterfly (Instrumental).m4a"

    QStringList justMyType;
    QString typeFilterNarrow  = typeFilter + "#!#"; // type must exactly equal, not just a prefix of
    QString typeFilterNarrow2 = typeFilter + "%!%"; // special delimiter for SquareDesk playlists
    QString typeFilterNarrow2b = typeFilter;        // special case for non-leaf nodes of SquareDesk playlists
    QString typeFilterNarrow3 = typeFilter + "$!$"; // special delimiter for Apple playlists
    QRegularExpression reTypeFilterNarrow2(typeFilterNarrow2);

    for (auto const &item : *aPathStack) {
        // qDebug() << "ITEM:" << item;
        if (takeAll
                || item.startsWith(typeFilterNarrow) // Tracks
                || item.startsWith(typeFilterNarrow2) // SquareDesk playlists (e.g. "Jokers/2024/Jokers_2024.01.23")
                || item.startsWith(typeFilterNarrow3) // Apple playlists
                || (typeFilter.endsWith("/") && item.startsWith(typeFilterNarrow2b))  // SquareDesk non-leaf playlists (e.g. "Jokers/2024")
            ) {
            // qDebug() << "TAKEN:" << item;
            justMyType.append(item);
        }
    }
    t.elapsed(__LINE__);

    // qDebug() << "justMyType.size() = " << justMyType.size();

    // second, make sure that only audio files are left  -------
    // iterate over every item in the pathStack, and stick songs into the songTable
    //   with their SQLITE DB info populated later (when songs are visible)
    static QRegularExpression musicRegex(".*\\.(mp3|m4a|wav|flac)$", QRegularExpression::CaseInsensitiveOption); // match with music extensions
    QStringList justMusic = justMyType.filter(musicRegex); // we are interested only in songs here
    t.elapsed(__LINE__);

    // qDebug() << "justMusic.size() = " << justMusic.size();

    ui->darkSongTable->setRowCount(justMusic.length()); // make all the rows at once for speed
    t.elapsed(__LINE__);

    // The font that we'll use for the QLabels that are used to implement the Title-with-Tags field
    QFont darkSongTableFont("Avenir Next");
    darkSongTableFont.setPointSize(20);
    darkSongTableFont.setWeight(QFont::Medium);

    // int totalNumberOfSquareDeskSongs = 0;
    // int totalNumberOfAppleSongs = 0;

    int i = 0;
    for (const auto &s : justMusic) {
        // qDebug() << "s:" << s;
        QStringList sl1 = s.split("#!#");

        QString type     = sl1[0];  // the type (of original pathname, before following aliases)
        QString origPath = sl1[1];  // everything else

        // NO:
        // QStringList pathParts = origPath.split("/");
        // QString typeFromPath = pathParts[pathParts.size()-2]; // second-to-last path part is the assumed type

        // YES:
        QString typeFromPath = filepath2SongCategoryName(origPath);

        QFileInfo fi(origPath);
        if (fi.canonicalPath() == musicRootPath) {
            typeFromPath = "";
        }

        // qDebug() << "origPath: " << origPath;
        // double check that type is non-music type (see Issue #298)
        if (type == "reference" || type == "soundfx" || type == "sd") {
            continue;
        }

        // --------------------------------
        QString label = "";
        QString labelnum = "";
        QString labelnum_extra = "";
        QString title = "";
        QString shortTitle = "";
        QString baseName = fi.completeBaseName();        

        // e.g. "/Users/mpogue/__squareDanceMusic/patter/RIV 307 - Going to Ceili (Patter).mp3" --> "RIV 307 - Going to Ceili (Patter)"
        breakFilenameIntoParts(baseName, label, labelnum, labelnum_extra, title, shortTitle);
        labelnum += labelnum_extra;

        QString pitchOverride = "";
        QString tempoOverride = "";

        if (type.contains("$!$")) {
            // This is Apple Music, so we're going to override everything that breakFilenameIntoParts did (or tried to do)
            // e.g. "ApplePlaylistName$!$lineNumber$!$Short Title from Apple Music#!#FullPathname"
            QStringList sl10 = type.split("$!$");
            QString ApplePlaylistName = sl10[0];
            QString AppleLineNumber = sl10[1];
            label = "Apple Music";
            title = sl10[2];
            shortTitle = title;

            QString appleSymbol = QChar(0xF8FF);  // use APPLE symbol for Apple Music (sorts at the bottom)
            // QString appleSymbol = QChar(0x039E);  // use GREEK XI for Local Playlists (sorts almost at the bottom, and looks like a playlist!)
            type = appleSymbol + " " + sl10[0] + " " + AppleLineNumber; // this is tricky.  Leading Apple will force sort to bottom for "<APPLESYMBOL> Apple Playlist Name".
            labelnum = "";
            labelnum_extra = "";
            // totalNumberOfAppleSongs++;
        } else if (type.contains("%!%")) {
            // This is a SquareDesk Playlist, so we're going to override everything that breakFilenameIntoParts did (or tried to do)
            // e.g. "SquareDeskPlaylistName%!%pitch,tempo#!#FullPathname"
            QStringList sl11 = type.split("%!%");
            QStringList sl11a = sl11[0].split("/");
            QString playlistName = sl11[0];
            QString pitchTempo = sl11[1];
            QStringList sl12 = pitchTempo.split(",");
            QString pitchOverride = sl12[0]; // TODO: implement this!
            QString tempoOverride = sl12[1]; // not "" if there is an override because playlist
            QString lineNumber = sl12[2];
            type = sl11a[sl11a.size()-1] + " " + lineNumber;  // take only the last part of the hierarchical playlist name, tack on a line number for sorting
            // label = "Playlist";
            // title = lineNumber + " - " + title; // use the default SquareDesk name (from the FullPathname), but prepend the line number and a dash
            shortTitle = title;

            QString GreekXi = QChar(0x039E);  // use GREEK XI for Local Playlists (sorts almost at the bottom, and looks like a playlist!)
            type = GreekXi + " " + type; // this is tricky.  Leading GREEK XI will force sort to almost bottom for "<GREEKXI> SquareDesk Playlist Name".
            // labelnum = "";
            // labelnum_extra = "";
        } else {
            // totalNumberOfSquareDeskSongs++; // only count songs, not playlist entries
        }

        // User preferences for colors
        // qDebug() << "origPath/typeFromPath:" << typeFromPath << origPath;
        QString cType = typeFromPath.toLower();  // type for Color purposes
        if (cType.right(1)=="*") {
            cType.chop(1);  // remove the "*" for the purposes of coloring
        }

        // if (songTypeNamesForExtras.contains(cType)) {
        //     textCol = QColor(extrasColorString);
        // }
        // else if (songTypeNamesForPatter.contains(cType)) {
        //     textCol = QColor(patterColorString);
        // }
        // else if (songTypeNamesForSinging.contains(cType)) {
        //     textCol = QColor(singingColorString);
        // }
        // else if (songTypeNamesForCalled.contains(cType)) {
        //     textCol = QColor(calledColorString);
        // } else {
        //     // textCol = QColor(QColor("#A0A0A0"));  // if not a recognized type, color it white-ish
        //     textCol = QColor(extrasColorString);  // if not a recognized type, color it the xtras color (so user can control it)
        // }

        if (cType == "patter") {
            textCol = QColor(patterColorString);
        } else if (cType == "singing") {
            textCol = QColor(singingColorString);
        } else if (cType == "called") {
            textCol = QColor(calledColorString);
        } else if (cType == "extras") {
            textCol = QColor(extrasColorString);
        } else {
            textCol = QColor(extrasColorString); // if not a recognized type, color it the xtras color (so user can control it)
        }

        QBrush textBrush(textCol); // make a brush for most of the widgets

        // PLAYLIST HANDLING -----
        // NOTE: PLAYLISTS NOT IN DARK SONG TABLE!
        // look up origPath in the path2playlistNum map, and reset the s2 text to the user's playlist # setting (if any)
        // QString s2("");
        // if (path2playlistNum.contains(origPath)) {
        //     s2 = path2playlistNum[origPath];
        // }
        // TableNumberItem *newTableItem4 = new TableNumberItem(s2);

        // newTableItem4->setTextAlignment(Qt::AlignCenter);
        // newTableItem4->setForeground(textCol);
        // // # items are editable by default
        // ui->darkSongTable->setItem(i, kNumberCol, newTableItem4);

        // # COLUMN IS NOW USED FOR AUDITION BUTTONS -----
        QTableWidgetItem *auditionItem = new QTableWidgetItem();

        auditionButton *auditionButton1 = new auditionButton();
        auditionButton1->setFlat(true);
        auditionButton1->setObjectName("auditionButton");
        auditionButton1->origPath = origPath;

        connect(auditionButton1, &QPushButton::pressed, this,
                [this]() {
                    auditionSingleShotTimer.stop();
                    auditionInProgress = true;
                    // qDebug() << "setting auditionInProgress to true";

                    QString origPath = ((auditionButton *)(sender()))->origPath;
                    // qDebug() << "AUDITION PATH:" << origPath;

                    auditionSetStartMs(origPath);

                    // QModelIndexList list = this->ui->darkSongTable->selectionModel()->selectedRows();
                    // int row = list.at(0).row();
                    // QString origPath = this->ui->darkSongTable->item(row,kPathCol)->data(Qt::UserRole).toString();
                    // // qDebug() << "QPushButton pressed, row:" << row << origPath;

                    this->auditionPlayer.setSource(QUrl::fromLocalFile(origPath));
                    
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
                });

        connect(auditionButton1, &QPushButton::released, this,
                [this]() {
                    // QModelIndexList list = this->ui->darkSongTable->selectionModel()->selectedRows();
                    // int row = list.at(0).row();
                    // QString origPath = this->ui->darkSongTable->item(row,kPathCol)->data(Qt::UserRole).toString();
                    // qDebug() << "QPushButton released, row:" << row << origPath;
                    this->auditionPlayer.stop();
                    this->ui->darkSongTable->setFocus(); // just released a button, so set focus back to the darkSongTable

                    auditionSingleShotTimer.start(1000);
                    // qDebug() << "KLUDGE: setting auditionInProgress to false 1000 ms in the future";
                    //  While this is kludgey, I'm not sure that there's an alternative.  If you press an audition button,
                    //  and move off the button (with Left Mouse Button still down), it will initiate a drag and drop, which
                    //  we do not want.  This timer gives you one second to let the left button up, before a drag and drop is inferred.
                    //  In my testing, that seemed about right.  No false drag and drops, but normal drag and drop still works as expected.
                });

        // QIcon playbackIcon = QIcon::fromTheme(QIcon::ThemeIcon::MultimediaPlayer);

        auditionButton1->setIcon(QIcon(":/graphics/icons8-square-play-button-100.png"));

        // auditionButton1->setIconSize(QSize(26,26));

        ui->darkSongTable->setItem(i, kNumberCol, auditionItem);
        ui->darkSongTable->setCellWidget(i, kNumberCol, auditionButton1);

        // TYPE FIELD -----
        QTableWidgetItem *twi1 = new QTableWidgetItem(type);
        twi1->setForeground(textBrush);
        twi1->setFlags(twi1->flags() & ~Qt::ItemIsEditable);      // not editable
        ui->darkSongTable->setItem(i, kTypeCol, twi1);

        // LABEL + LABELNUM FIELD -----
        QTableWidgetItem *twi2 = new QTableWidgetItem(label + " " + labelnum);
        twi2->setForeground(textBrush);
        twi2->setFlags(twi2->flags() & ~Qt::ItemIsEditable);      // not editable
        ui->darkSongTable->setItem(i, kLabelCol, twi2);

//         INVISIBLE TABLE WIDGET ITEM -----
//         FYI: THIS IS FOR SORTING...
        InvisibleTableWidgetItem *titleItem(new InvisibleTableWidgetItem(title));
        titleItem->setFlags(titleItem->flags() & ~Qt::ItemIsEditable);      // not editable
        ui->darkSongTable->setItem(i, kTitleCol, titleItem);

        // TITLE WIDGET (WITH TAGS) -----
        SongSetting settings;
        songSettings.loadSettings(origPath, settings);  // get settings from the SQLITE DB --
        if (settings.isSetTags()) {
            songSettings.addTags(settings.getTags());
        }

        // format the title string --
        QString titlePlusTags(FormatTitlePlusTags(title, settings.isSetTags(), settings.getTags(), textCol.name()));
        darkSongTitleLabel *titleLabel = new darkSongTitleLabel(this);
        titleLabel->setTextFormat(Qt::RichText);
        titleLabel->setText(titlePlusTags);
        titleLabel->textColor = textCol.name();  // remember the text color, so we can restore it when deselected
        titleLabel->setFont(darkSongTableFont);

        ui->darkSongTable->setCellWidget(i, kTitleCol, titleLabel);

        // AGE FIELD -----
        QString ageString = songSettings.getSongAge(fi.completeBaseName(), origPath, show_all_ages);
        QString ageAsIntString = ageToIntString(ageString);
        QTableWidgetItem *twi4 = new TableNumberItem(ageAsIntString); // TableNumberItem so it's numerically sortable
        twi4->setForeground(textBrush);
        twi4->setTextAlignment(Qt::AlignCenter);
        twi4->setFlags(twi4->flags() & ~Qt::ItemIsEditable);      // not editable
        // qDebug() << "TITLE/AGE:" << title << ageString << ageAsIntString;
        ui->darkSongTable->setItem(i, kAgeCol, twi4);

        // RECENT FIELD (must come after AGE field, because it uses age to determine recent string) -----
        QString recentString = ageToRecent(ageString);  // passed as double string
        QTableWidgetItem *twi4b = new QTableWidgetItem(recentString);
        twi4b->setForeground(textBrush);
        twi4b->setTextAlignment(Qt::AlignCenter);
        twi4b->setFlags(twi4b->flags() & ~Qt::ItemIsEditable);      // not editable
        ui->darkSongTable->setItem(i, kRecentCol, twi4b);

        // ((darkSongTitleLabel *)(ui->darkSongTable->cellWidget(i, kTitleCol)))->setSongUsed(true || recentString != ""); // rewrite the song's title to be strikethrough and/or green background

        // PITCH FIELD -----
        int pitch = 0;
        if (settings.isSetPitch()) { pitch = settings.getPitch(); }

        QTableWidgetItem *twi5 = new QTableWidgetItem(QString("%1").arg(pitch));
        twi5->setForeground(textBrush);
        twi5->setTextAlignment(Qt::AlignCenter);
        twi5->setFlags(twi5->flags() & ~Qt::ItemIsEditable);      // not editable
        ui->darkSongTable->setItem(i, kPitchCol, twi5);

        // TEMPO FIELD -----
        int tempo = 0;
        bool loadedTempoIsPercent(false);
        if (settings.isSetTempo()) { tempo = settings.getTempo(); }
        if (settings.isSetTempoIsPercent()) { loadedTempoIsPercent = settings.getTempoIsPercent(); }
        QString tempoStr = QString("%1").arg(tempo);
        if (loadedTempoIsPercent) tempoStr += "%";
        //        qDebug() << "loadMusicList() is setting the kTempoCol to: " << tempoStr;

        QTableWidgetItem *twi6 = new QTableWidgetItem(QString("%1").arg(tempoStr));
        twi6->setForeground(textBrush);
        twi6->setTextAlignment(Qt::AlignCenter);
        twi6->setFlags(twi6->flags() & ~Qt::ItemIsEditable);      // not editable
        ui->darkSongTable->setItem(i, kTempoCol, twi6);

        // PATH FIELD (VARIANT SAVED IN INVISIBLE LOCATION) -----
        // keep the path around, for loading in when we double click on it
        ui->darkSongTable->item(i, kPathCol)->setData(Qt::UserRole, QVariant(origPath)); // path set on cell in col 0

//        if (i < 10) {
//            qDebug() << type << label << labelnum << title << shortTitle; // << titlePlusTags;
//        }

        i++;
    }

    t.elapsed(__LINE__);

    // darkFilterMusic(); // I don't think this is needed here.

    ui->darkSongTable->resizeColumnToContents(kNumberCol);  // and force resizing of column widths to match songs
    ui->darkSongTable->resizeColumnToContents(kTypeCol);
    ui->darkSongTable->resizeColumnToContents(kLabelCol);
    ui->darkSongTable->resizeColumnToContents(kPitchCol);
    ui->darkSongTable->resizeColumnToContents(kTempoCol);

    ui->darkSongTable->blockSignals(false);  // unblock signals
    ui->darkSongTable->setSortingEnabled(true);

    // performance -----
    // these must be in "backwards" order to get the right order, which
    //   is that Type is primary, Title is secondary, Label is tertiary
    // qDebug() << "darkLoadMusicList::sortItems";

    // NOTE: here is where we set the sort order

    QString desiredSortOrder = prefsManager.GetcurrentSortOrder();
    // qDebug() << "darkLoadMusicList desiredSortOrder: " << desiredSortOrder;
    if (desiredSortOrder == "") {
        // it hasn't been set yet!
        // ui->darkSongTable->sortItems(kLabelCol);  // sort last by label/label #
        // ui->darkSongTable->sortItems(kTitleCol);  // sort second by title in alphabetical order
        // ui->darkSongTable->sortItems(kTypeCol);   // sort first by type (singing vs patter)
        // qDebug() << "setting to default sort order!";
        sortByDefaultSortOrder();  // this will be persisted...
    } else {
        // qDebug() << "setting to new sort order!" << desiredSortOrder;
        ui->darkSongTable->setOrderFromString(desiredSortOrder);
    }

    // qDebug() << "darkLoadMusicList::DONE with sortItems";

        ui->darkSongTable->show();

    if (!suppressSelectionChange) {
        ui->darkSongTable->selectRow(0); // DEBUG
        ui->darkSongTable->scrollToItem(ui->darkSongTable->item(0, kTypeCol)); // EnsureVisible row 0 (which is highlighted)

    // // PERFORMANCE TESTING --------
    // QFile file("/Users/mpogue/pathStack.txt");
    // if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    //     QTextStream out(&file);
    //     for (const QString &str : *pathStack) {
    //         out << str << "\n";
    //     }
    //     file.close();
    // } else {
    //     // Handle file opening error
    //     qDebug() << "Error opening file for pathStack:" << file.errorString();
    // }


    // QString msg1 = QString::number(ui->darkSongTable->rowCount()) + QString(" audio files found");
    // QString msg1 = QString("Songs found: %1 SquareDesk + %2 Apple Music, pathStack: %3")
    //         .arg(QString::number(totalNumberOfSquareDeskSongs))
    //         .arg(QString::number(totalNumberOfAppleSongs))
    //         .arg(pathStack->size());
    // QString msg1 = QString("Songs found: %1 SquareDesk + %2 Apple Music")
    //         .arg(QString::number(totalNumberOfSquareDeskSongs))
    //         .arg(QString::number(totalNumberOfAppleSongs));
    // ui->statusBar->showMessage(msg1);

        // do this only if we WANT the selection and focus to go to the darkSearch or darkSongTable
        //   when reloading darkSongTable + filter because of a playlist change, do NOT change focus or selection
        lastSongTableRowSelected = -1;  // don't modify previous one, just set new selected one to color
        on_darkSongTable_itemSelectionChanged();  // to re-highlight the selection, if music was reloaded while an item was selected
        lastSongTableRowSelected = 0; // first row is highlighted now

        ui->darkSongTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->darkSongTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
        bool searchHasFocus = ui->darkSearch->hasFocus();
        bool darkSongTableHasFocus = ui->darkSongTable->hasFocus();
        // qDebug() << "yeah, I don't want to go to row 0 here";
        // ui->darkSongTable->selectRow(0);
        if (searchHasFocus) {
            ui->darkSearch->setFocus();
        } else if (darkSongTableHasFocus) {
            ui->darkSongTable->setFocus();
        }

        // ui->darkSongTable->scrollToItem(ui->darkSongTable->item(0, kTypeCol)); // EnsureVisible row 0 (which is highlighted)
        ui->darkSearch->setFocus();
    } else {
        // qDebug() << "SELECTION/FOCUS CHANGE SUPPRESSED";
    }

    t.elapsed(__LINE__);

    if (reloadPaletteSlots) {
        // now, if we just loaded the darkMusicList, we have to check the palette slots, to see if they need to
        //  be reloaded, too.  This normally happens just when the fileWatcher is triggered.
        // qDebug() << "reloading the palette slots too";
        for (int i = 0; i < 3; i++) {
    //        qDebug() << "TRACKS? " << relPathInSlot[i];
            if (relPathInSlot[i].startsWith("/tracks")) {
                QString playlistFilePath = musicRootPath + relPathInSlot[i] + ".csv";
                playlistFilePath.replace("/tracks/", "/Tracks/"); // FIX: someday I'll make this one capitalization
    //            qDebug() << "NEED TO RELOAD THIS SLOT BECAUSE TRACKS" << i << playlistFilePath;
                int songCount;
                loadPlaylistFromFileToPaletteSlot(playlistFilePath, i, songCount);
            }
        }
    }

    t.stop();

    if (aPathStack != nullptr) {
        currentlyShowingPathStack = aPathStack; // we're showing either the one we were given (pathStack, pathStackPlaylists), or we just refreshed the currently showing one
    }

    currentTypeFilter = typeFilter;
}

// Custom QTableWidgetItem for numeric sorting
class NumericTableWidgetItem : public QTableWidgetItem {
public:
    NumericTableWidgetItem(const QString &text) : QTableWidgetItem(text) {}

    bool operator<(const QTableWidgetItem &other) const override {
        return text().toInt() < other.text().toInt();
    }
};

// ====================================================================================
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDirIterator>
#include <QDebug>
#include <QDialog>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QSet>
#include <QPushButton>
#include <QPainter>
#include <QHBoxLayout>
#include <QFile>
#include <QMessageBox>

enum CopyAction {
    Ask,
    ReplaceAll,
    SkipAll,
    CancelAll
};

static CopyAction currentCopyAction = Ask;

// Custom QTableWidgetItem for numeric sorting

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

QString toTitleCase(const QString& input) {
    QStringList words = input.split(' ', Qt::SkipEmptyParts);
    QStringList titleCaseWords;
    for (const QString& word : words) {
        if (!word.isEmpty()) {
            titleCaseWords.append(word.at(0).toUpper() + word.mid(1).toLower());
        }
    }
    return titleCaseWords.join(' ');
}

QString MainWindow::proposeCanonicalName(QString baseName, bool withLabelNumExtra) {
    QString theLabel, theLabelNum, theLabelNumExtra, theTitle, theShortTitle;
    breakFilenameIntoParts(baseName, theLabel, theLabelNum, theLabelNumExtra, theTitle, theShortTitle);

    QString proposedBaseName;

    if (!baseName.contains("-")) {
        return(baseName); // e.g. "containerwidth" or "foo.png"
    }

    if (!withLabelNumExtra) {
        theLabelNumExtra = "";
    }

    theTitle = toTitleCase(theTitle); // "foo bar" --> "Foo Bar"
    theLabel = theLabel.toUpper();    // "riv" --> "RIV"

    switch (songFilenameFormat) {
        case SongFilenameNameDashLabel:
            // e.g. "Foo Bar - RIV 123"
            proposedBaseName = theTitle + "  " + theLabel + " " + theLabelNum + theLabelNumExtra;
        case SongFilenameBestGuess:
        case SongFilenameLabelDashName:
            // e.g. "RIV 123 - Foo Bar"
            proposedBaseName = theLabel + " " + theLabelNum + theLabelNumExtra + " - " + theTitle;
    }

    return(proposedBaseName);
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (!mimeData->hasUrls()) {
        return;
    }

    // Dark grey background color for unchecked/strikethrough rows
    const QColor darkGreyBackground("#A0A0A0"); // RGB(160, 160, 160)

    // 1. Gather all file paths, recursing into directories
    QStringList allFilePaths;
    for (const QUrl &url : mimeData->urls()) {
        QString path = url.toLocalFile();
        QFileInfo fileInfo(path);
        if (fileInfo.isDir()) {
            QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                allFilePaths.append(it.next());
            }
        } else {
            allFilePaths.append(path);
        }
    }
    allFilePaths.sort();

    // 2. Create the dialog and its components
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Import and Organize Files"));
    // Calculate minimum width to ensure all columns are visible
    int minWidth = 50 + 250 + 250 + 150 + 50 + 20; // sum of column widths + scrollbar
    dialog.setMinimumSize(minWidth, 600);
    dialog.resize(minWidth + 225, 600);  // Default size 225 pixels wider
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"Item", "Original Filename", "Proposed Title (double-click to edit)", "Destination", "Copy"});
    // Set column resize modes
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);        // Item - fixed width
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Original Filename - resize to contents
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);      // Proposed Title - stretch to fill
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);        // Destination - fixed width
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);        // Copy - fixed width
    // Set fixed column widths
    table->setColumnWidth(0, 50);   // Item
    table->setColumnWidth(3, 150);  // Destination
    table->setColumnWidth(4, 50);   // Copy
    // Disable horizontal scrollbar
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->verticalHeader()->setVisible(false);
    
    // Enable zebra striping for alternating row colors
    table->setAlternatingRowColors(true);

    // Make header bold using a stylesheet for reliability
    table->horizontalHeader()->setStyleSheet("QHeaderView::section { font-weight: bold; }");

    layout->addWidget(table);

    QStringList patterSingingTypes = songTypeNamesForPatter + songTypeNamesForSinging; // no single quotes
    QStringList patterSingingTypesSQ = patterSingingTypes; // will add single quotes to each for user dialog
    QString defaultPatterFolderName  = songTypeNamesForPatter[0];  // e.g. "patter"
    QString defaultSingingFolderName = songTypeNamesForSinging[0]; // e.g. "singing"
    QString defaultVocalsFolderName  = songTypeNamesForCalled[0];  // e.g. "vocals"
    QString defaultLyricsFolderName  = "lyrics";
    QString allTypes;

    // put single quotes around the types, to make them easier to read
    for (int i = 0; i < patterSingingTypesSQ.size(); ++i) {
        patterSingingTypesSQ[i] = "'" + patterSingingTypesSQ[i] + "'";
    }

    // now put in a nice A, B, C, and D format...
    if (patterSingingTypesSQ.count() > 1) {
        // Join all elements except the last one
        allTypes = patterSingingTypesSQ.mid(0, patterSingingTypesSQ.count() - 1).join(", ");
        // Add " and " before the last element
        allTypes += ", and " + patterSingingTypesSQ.last();
    } else if (patterSingingTypesSQ.count() == 1) {
        // Handle the case of a single element
        allTypes = patterSingingTypesSQ.first();
    }

    // qDebug() << "noteLabel:" << songTypeNamesForPatter << songTypeNamesForSinging << allTypes;

    QLabel *noteLabel = new QLabel(QString("Note: the NEW tag will be set on files that are copied to: ") + allTypes + ".");
    QFont font = noteLabel->font();
    font.setPointSize(font.pointSize() * 1.0);
    noteLabel->setFont(font);
    noteLabel->setAlignment(Qt::AlignLeft); // Align to the right
    layout->addWidget(noteLabel);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    // 3. Pre-process to find all HTML files for default logic (case-insensitive)
    QSet<QString> htmlBasenames;
    static QRegularExpression vocalPattern("\\d+[Vv]"); // Matches one or more digits followed by 'V' or 'v'

    for (const QString &path : allFilePaths) {
        if (path.endsWith(".htm", Qt::CaseInsensitive) || path.endsWith(".html", Qt::CaseInsensitive)) {
            QString baseNameLower = QFileInfo(path).completeBaseName().toLower();
            htmlBasenames.insert(baseNameLower.simplified());
        }
    }

    // 4. Populate the table
    table->setRowCount(0);
    int currentItem = 1;

    QStringList pathsInTable;

    for (const QString &path : allFilePaths) {
        QFileInfo fileInfo(path);
        QString fileName = fileInfo.fileName();
        QString baseName = fileInfo.completeBaseName().simplified();
        QString extension = fileInfo.suffix();

        QStringList badExtensions;
        badExtensions << "jpg" << "jpeg" << "xml" << "png" << "thmx";

        if (badExtensions.contains(extension.toLower())) {
            continue; // then skip processing this item
        }

        pathsInTable << path;  // these are the paths that made it into the table

        table->insertRow( table->rowCount() ); // add a row!

        // Item Number (read-only, numeric sort)
        QTableWidgetItem *itemNumber = new NumericTableWidgetItem(QString::number(currentItem));
        itemNumber->setFlags(itemNumber->flags() & ~Qt::ItemIsEditable);
        itemNumber->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        // Original Title (read-only)
        QTableWidgetItem *titleItem = new QTableWidgetItem(fileName);
        titleItem->setFlags(titleItem->flags() & ~Qt::ItemIsEditable);

        // New Title (editable) - includes extension
        QString proposedBaseName = proposeCanonicalName(baseName);
        QString proposedFullName = proposedBaseName + "." + extension.toLower();
        QTableWidgetItem *newTitleItem = new QTableWidgetItem(proposedFullName);
        newTitleItem->setBackground(QColor(255, 255, 224)); // Light yellow to indicate editable

        // Destination (combo box)
        QComboBox *destCombo = new QComboBox();
        // destCombo->addItems({"patter", "singing", "vocals", "lyrics"});

        QStringList destFolders = songTypeNamesForPatter + songTypeNamesForSinging + songTypeNamesForCalled + songTypeNamesForExtras + QStringList("lyrics");
        destCombo->addItems(destFolders); // e.g. patter, hoedown, singing, xtras, lyrics

        static QRegularExpression parenRegex("\\(.*\\)");
        QString noExtraName = proposeCanonicalName(baseName.toLower(), false);
        noExtraName.replace(parenRegex,"");
        noExtraName = noExtraName.simplified();
        // qDebug() << baseName << noExtraName << htmlBasenames;

        QStringList noCopyLyricsExtensions;
        noCopyLyricsExtensions << "pdf" << "doc" << "rtf";

        // --- Apply default destination logic ---
        // defaults are the first items on each of the songNamesFor* lists (set by user in Preferences)
        if (extension.toLower() == "htm" || extension.toLower() == "html") {
            destCombo->setCurrentText("lyrics");
        } else if (noCopyLyricsExtensions.contains(extension.toLower())) {
            // destCombo->setCurrentText("singing");
            destCombo->setCurrentText(defaultLyricsFolderName);
            // these will have destination LYRICS, but will default to NO COPY
        } else if (baseName.toLower().contains(vocalPattern)) {
            // destCombo->setCurrentText("vocals");
            destCombo->setCurrentText(defaultVocalsFolderName);
        } else if (htmlBasenames.contains(baseName.toLower())) {
            // destCombo->setCurrentText("singing");
            destCombo->setCurrentText(defaultSingingFolderName);
        } else if (htmlBasenames.contains(noExtraName.toLower())) {
            // e.g. "RIV 123b - Foo Bar"
            // destCombo->setCurrentText("singing");
            destCombo->setCurrentText(defaultSingingFolderName);
        } else {
            // destCombo->setCurrentText("patter");
            destCombo->setCurrentText(defaultPatterFolderName);
        }

        // Set text colors based on destination type (like playlists.cpp:243)
        QString cType = destCombo->currentText().toLower();
        QColor textCol;
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
            textCol = QColor(extrasColorString); // default fallback
        }
        
        // Apply the color to filename columns
        titleItem->setForeground(QBrush(textCol));
        newTitleItem->setForeground(QBrush(textCol));

        // Connect destination dropdown to update colors when changed
        connect(destCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
                [=](const QString &newType) {
                    QString cType = newType.toLower();
                    QColor textCol;
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
                        textCol = QColor(extrasColorString); // default fallback
                    }
                    
                    // Update the color of filename columns
                    titleItem->setForeground(QBrush(textCol));
                    newTitleItem->setForeground(QBrush(textCol));
                });

        // Copy checkbox
        QPushButton *copyButton = new QPushButton();
        copyButton->setCheckable(true);

        copyButton->setFlat(true);
        copyButton->setMinimumSize(24, 24);

        QPixmap checkPixmap(16, 16);
        checkPixmap.fill(Qt::white);
        QPainter checkPainter(&checkPixmap);
        checkPainter.setRenderHint(QPainter::Antialiasing);
        QPen checkPen(Qt::darkGreen, 2);
        checkPainter.setPen(checkPen);
        checkPainter.drawLine(3, 8, 7, 12);
        checkPainter.drawLine(7, 12, 13, 4);
        checkPainter.end();
        QIcon greenCheckIcon(checkPixmap);

        QPixmap xPixmap(16, 16);
        xPixmap.fill(Qt::white);
        QPainter xPainter(&xPixmap);
        xPainter.setRenderHint(QPainter::Antialiasing);
        QPen xPen(Qt::red, 2);
        xPainter.setPen(xPen);
        xPainter.drawLine(4, 4, 12, 12);
        xPainter.drawLine(4, 12, 12, 4);
        xPainter.end();
        QIcon redXIcon(xPixmap);

        // copyButton->setIcon(greenCheckIcon);

        QStringList musicExtensions;
        musicExtensions << "mp3" << "wav" << "m4a" << "flac";

        QStringList lyricsExtensions;
        lyricsExtensions << "htm" << "html" << "txt"; // no pdf or doc files copied

        if (musicExtensions.contains(extension.toLower()) ||
            lyricsExtensions.contains(extension.toLower())) {
            copyButton->setChecked(true);
        } else {
            copyButton->setChecked(false);
        }

        bool checked = copyButton->isChecked();

        copyButton->setIcon(checked ? greenCheckIcon : redXIcon);
        QFont font;

        font = itemNumber->font();
        font.setStrikeOut(!checked);
        itemNumber->setFont(font);
        itemNumber->setBackground(!checked ? QBrush(darkGreyBackground) : QBrush());

        font = titleItem->font();
        font.setStrikeOut(!checked);
        titleItem->setFont(font);
        titleItem->setBackground(!checked ? QBrush(darkGreyBackground) : QBrush());

        font = newTitleItem->font();
        font.setStrikeOut(!checked);
        newTitleItem->setFont(font);
        newTitleItem->setBackground(!checked ? QBrush(darkGreyBackground) : QBrush());

        const int currentRow = currentItem - 1;
        connect(copyButton, &QPushButton::toggled, [=](bool checked) {
            copyButton->setIcon(checked ? greenCheckIcon : redXIcon);
            QFont font;

            font = itemNumber->font();
            font.setStrikeOut(!checked);
            itemNumber->setFont(font);
            itemNumber->setBackground(!checked ? QBrush(darkGreyBackground) : QBrush());

            font = titleItem->font();
            font.setStrikeOut(!checked);
            titleItem->setFont(font);
            titleItem->setBackground(!checked ? QBrush(darkGreyBackground) : QBrush());

            font = newTitleItem->font();
            font.setStrikeOut(!checked);
            newTitleItem->setFont(font);
            newTitleItem->setBackground(!checked ? QBrush(darkGreyBackground) : QBrush());

            // Update background color of cells containing widgets
            if (table->item(currentRow, 3)) {
                table->item(currentRow, 3)->setBackground(!checked ? QBrush(darkGreyBackground) : QBrush());
            }
            if (table->item(currentRow, 4)) {
                table->item(currentRow, 4)->setBackground(!checked ? QBrush(darkGreyBackground) : QBrush());
            }

            // Update OK button text
            int checkedCount = 0;
            for (int r = 0; r < table->rowCount(); ++r) {
                QWidget* w = table->cellWidget(r, 4);
                QPushButton* b = w->findChild<QPushButton*>();
                if (b && b->isChecked()) {
                    checkedCount++;
                }
            }
            okButton->setText(tr("Copy %1 Files").arg(checkedCount));
        });

        QWidget* widget = new QWidget();
        widget->setStyleSheet("background-color: transparent; border: none;");
        QHBoxLayout* hLayout = new QHBoxLayout(widget);
        hLayout->addWidget(copyButton);
        hLayout->setAlignment(Qt::AlignCenter);
        hLayout->setContentsMargins(0,0,0,0);

        int row = currentItem - 1;
        table->setItem(row, 0, itemNumber);
        table->setItem(row, 1, titleItem);
        table->setItem(row, 2, newTitleItem);
        
        // Create items for cells that will contain widgets (for background color control)
        QTableWidgetItem *destItem = new QTableWidgetItem();
        destItem->setFlags(destItem->flags() & ~Qt::ItemIsEditable);
        destItem->setBackground(!checked ? QBrush(darkGreyBackground) : QBrush());
        table->setItem(row, 3, destItem);
        
        QTableWidgetItem *copyItem = new QTableWidgetItem();
        copyItem->setFlags(copyItem->flags() & ~Qt::ItemIsEditable);
        copyItem->setBackground(!checked ? QBrush(darkGreyBackground) : QBrush());
        table->setItem(row, 4, copyItem);
        
        table->setCellWidget(row, 3, destCombo);
        table->setCellWidget(row, 4, widget);

        currentItem++;
    }
    // Column widths are now handled by resize modes

    table->setSortingEnabled(true);
    table->sortByColumn(0, Qt::AscendingOrder);

    // Initial update of OK button text
    int initialCheckedCount = 0;
    for (int r = 0; r < table->rowCount(); ++r) {
        QWidget* w = table->cellWidget(r, 4);
        QPushButton* b = w->findChild<QPushButton*>();
        if (b && b->isChecked()) {
            initialCheckedCount++;
        }
    }
    okButton->setText(tr("Copy %1 Files").arg(initialCheckedCount));

    qDebug() << "pathsInTable" << pathsInTable;

    // 5. Show the dialog and process the result
    if (dialog.exec() == QDialog::Accepted) {
        // qDebug() << "--- Begin Import Selections ---";
        bool didAtLeastOneCopy = false;
        for (int i = 0; i < table->rowCount(); ++i) {
            QWidget* w = table->cellWidget(i, 4);
            QPushButton* b = w->findChild<QPushButton*>();

            if (b && b->isChecked()) {
                QString originalPath = pathsInTable.at(i);
                QString newTitleFromTable = table->item(i, 2)->text();
                QComboBox *combo = static_cast<QComboBox*>(table->cellWidget(i, 3));
                QString dest = combo->currentText();

                QString finalPath = musicRootPath + "/" + dest + "/" + newTitleFromTable;

                // qDebug() << "COPYING "
                //          << "From:" << originalPath
                //          << "To:" << finalPath;

                bool didCopy = false;

                // --- File Copying Logic ---
                if (QFile::exists(finalPath)) {
                    if (currentCopyAction == Ask) {
                        QMessageBox msgBox;
                        msgBox.setWindowTitle(tr("File Exists"));
                        msgBox.setText(tr("The file \"%1\" already exists. What do you want to do?").arg(QFileInfo(finalPath).fileName()));
                        msgBox.setInformativeText(tr("Source:\n%1\n\nDestination:\n%2").arg(originalPath).arg(finalPath));
                        msgBox.setIcon(QMessageBox::Question);

                        QPushButton *replaceButton = msgBox.addButton(tr("Replace"), QMessageBox::AcceptRole);
                        QPushButton *skipButton = msgBox.addButton(tr("Skip"), QMessageBox::RejectRole);
                        QPushButton *cancelButton = msgBox.addButton(tr("Cancel"), QMessageBox::DestructiveRole);
                        QPushButton *replaceAllButton = msgBox.addButton(tr("Replace All"), QMessageBox::AcceptRole);
                        QPushButton *skipAllButton = msgBox.addButton(tr("Skip All"), QMessageBox::RejectRole);

                        msgBox.setDefaultButton(replaceButton);
                        msgBox.exec();

                        if (msgBox.clickedButton() == replaceButton) {
                            currentCopyAction = Ask; // Reset for next time if not "All"
                            QFile::remove(finalPath);
                            QFile::copy(originalPath, finalPath);
                            didCopy = true;
                        } else if (msgBox.clickedButton() == skipButton) {
                            currentCopyAction = Ask; // Reset for next time if not "All"
                            // Do nothing, effectively skipping
                        } else if (msgBox.clickedButton() == cancelButton) {
                            currentCopyAction = CancelAll;
                            break; // Exit the loop
                        } else if (msgBox.clickedButton() == replaceAllButton) {
                            currentCopyAction = ReplaceAll;
                            QFile::remove(finalPath);
                            QFile::copy(originalPath, finalPath);
                            didCopy = true;
                        } else if (msgBox.clickedButton() == skipAllButton) {
                            currentCopyAction = SkipAll;
                            // Do nothing, effectively skipping
                        }
                    } else if (currentCopyAction == ReplaceAll) {
                        QFile::remove(finalPath);
                        QFile::copy(originalPath, finalPath);
                        didCopy = true;
                    } else if (currentCopyAction == SkipAll) {
                        // Do nothing, effectively skipping
                    } else if (currentCopyAction == CancelAll) {
                        break; // Exit the loop
                    }
                } else {
                    // File does not exist, just copy
                    QFile::copy(originalPath, finalPath);
                    didCopy = true;
                }

                // qDebug() << "FOO:" << didCopy << patterSingingTypes << dest;
                if (didCopy && (patterSingingTypes.contains(dest))) {
                    // qDebug() << "didCopy, so set the NEW tag on" << finalPath;
                    darkChangeTagOnPathToMP3(finalPath, QString("NEW"), true); // add the NEW tag to the copied file
                    didAtLeastOneCopy = true;
                }
            }
        }
        // qDebug() << "--- End Import Selections ---";

        if (didAtLeastOneCopy) {
            // qDebug() << "******* REFRESH SONGTABLE NEEDED *******";
            // this is here for future expansion.  Doing the copy already triggers
            //   a refresh of the darkSongTable, so we don't technically need to do this now.
        }
    }
    currentCopyAction = Ask; // In all cases, Reset to ASK for next time
}

void MainWindow::on_actionImport_and_Organize_Files_triggered()
{
    QMessageBox::information(
        this,
        tr("Import and Organize Files"),
        tr("To import files, drag and drop them from the Finder onto the main window.")
    );
}
