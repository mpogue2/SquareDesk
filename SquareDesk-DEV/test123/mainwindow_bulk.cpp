/****************************************************************************
**
** Copyright (C) 2016-2026 Mike Pogue, Dan Lyke
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
// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757

#include "ui_mainwindow.h"
#include "songlistmodel.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include <QList>
#include <QMap>
#include <QTextStream>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QElapsedTimer>
#include <QApplication>
#include <QDebug>

#include <utility>
#ifdef Q_OS_LINUX
#include <QtConcurrent/QtConcurrent>
#else
#include <QtConcurrent>
#endif
#include <QtMultimedia>

#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "third_party/minimp3_ex.h"

// special signature for the drop-in replacement for mp3dec_load()
int audiodec_load(mp3dec_t *mp3d, const char *file_name, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data);

#include "xxhash64.h"

// forward decl's from wav_file.h -------
extern "C"
{
typedef struct {
    int     SampleRate;
    int     NumberOfSamples;                // Per channel
    short   NumberOfChannels;
    short   WordLength;
    short   BytesPerSample;
    short   DataFormat;
} WAV_FILE_INFO;

WAV_FILE_INFO wav_set_info (const int, const int, const short, const short, const short, const short);

int wav_write_file_float1 (const float *pData,
                          const char *fileName,
                          const WAV_FILE_INFO WavInfo,
                          const int BufLen);
}

// ========================================================================
// BULK processing for MP3 files in SquareDesks's musicDir, for:
// - beat/bar detection
// - patter segmentation
// - BPM detection
// - ReplayGain calculation
// - etc.

// int  processOneFile(const double &d);
// void processFiles(QList<double> dlist);

// Where the cached section info for ONE song lives.
//
// A song in the Music Directory keeps the pathname it has always had: the same path, with
//   musicRootPath swapped for .squaredesk/bulk, plus ".results.txt".  That spelling is deliberate
//   and must not change -- every .results.txt a user has already calculated is sitting at it.
//
// An Apple Music track is not under musicRootPath at all, so that swap used to be a silent no-op
//   and the results file landed in the user's Apple Music media folder, next to the track
//   (issue #1760).  Those go into a folder of their own instead, keyed by Music's persistentID
//   rather than by pathname -- the same key the songs table uses, and for the same reason: an
//   Apple Music pathname is not stable, but the persistentID survives retitling and moving
//   (issue #1747).
//
// The fallback for a track with no persistentID is a hash of the absolute path.  It is only as
//   stable as the path is, but it is bounded in length and contains no separators, which a raw
//   path does not.
QString MainWindow::sectionResultsPathForSong(const QString &songPath) const {
    const QString bulkDirname = musicRootPath + "/.squaredesk/bulk";

    if (songPath.startsWith(musicRootPath)) {
        QString resultsFilename = songPath;
        resultsFilename.replace(musicRootPath, bulkDirname);
        return(resultsFilename + ".results.txt");
    }

    // Not in the Music Directory, so it's an Apple Music track (or something else external).
    QString key = appleMusicPersistentIDByPath.value(songPath);
    if (key.isEmpty()) {
        const QByteArray utf8 = songPath.toUtf8();
        key = QString("path-%1").arg(XXHash64::hash(utf8.constData(), utf8.size(), 0), 16, 16, QChar('0'));
    }

    return(bulkDirname + "/AppleMusic/" + key + ".results.txt");
}

// The Type of a song, for the purpose of deciding whether it can have section info.
//
// filepath2SongCategoryName() works it out from the Type folder in the pathname, which an Apple
//   Music track does not have -- it lives in the iTunes media folder, not the Music Directory, so
//   that function always came back with something meaningless and patter coming from Apple Music
//   was refused outright (issue #1760).  Preferences > Apple Music already worked the Type out
//   from the track's metadata at import time, so use that when there is one, exactly as
//   loadMP3File() does for playback (issue #1740, item 6).
QString MainWindow::songCategoryForSectionInfo(const QString &songPath) const {
    const QString appleMusicType = appleMusicTypeByPath.value(songPath);
    if (!appleMusicType.isEmpty()) {
        return(appleMusicType);
    }
    return(filepath2SongCategoryName(songPath));
}

// Which of these songs are worth calculating sections for.  Only patter has sections, and "test"
//   is the developer escape hatch that has always been allowed alongside it.
QStringList MainWindow::patterPathsAmong(const QStringList &paths) const {
    QStringList result;
    for (const auto &path : std::as_const(paths)) {
        const QString theCategory = songCategoryForSectionInfo(path);
        if (theCategory == "patter" || theCategory == "test") {
            result.append(path);
        }
    }
    return(result);
}

int MainWindow::processOneFile(const QString &fn) {
    // returns 0 if OK, else error code

    // qDebug() << "processOneFile: " << fn;
    // sleep(10); // sleep 10 seconds!

    // PROCESS MP3 FILE ==================

    // FIGURE OUT FILENAMES, AND SKIP IF RESULTS ALREADY PRESENT ---------
    // NOTE: we run on the thread pool, so we read the pathname that startSectionEstimation()
    //   resolved for us on the main thread, rather than touching appleMusicPersistentIDByPath
    //   here -- a library rescan could be rebuilding that hash while we run (issue #1760).
    const QString resultsFilename = sectionResultsPathSnapshot.value(fn);

    QFileInfo resultsFileinfo(resultsFilename);
    const QString resultsFiledir = resultsFileinfo.absolutePath();

    // qDebug() << "resultsFiledir (results go here): " << resultsFiledir;
    QDir().mkpath(resultsFiledir); // make sure that the results folder exists, e.g. .squaredesk/bulk/patter/RIV 123 - foo.results.txt

    if (resultsFileinfo.exists() && resultsFileinfo.size() > 10) {
        // file needs to exist AND it needs to have stuff in it, otherwise we're going to reprocess it.
        mp3ResultsLock.lock();
        mp3Results[fn] = 1; // record the results (0 = OK, 1 = results already existed, skipping.)
        mp3ResultsLock.unlock();
        // qDebug() << "skipping " << fn << ": results.txt file already exists";
        // qDebug() << "mp3FilenamesToProcess" << mp3FilenamesToProcess;
        // qDebug() << "mp3Results: " << mp3Results;
        return(0);
    }

    // qDebug() << "***** Processing: " << fn;

    QString resolvedFilePath = QFileInfo(fn).symLinkTarget(); // path with the symbolic links followed/removed
    if (resolvedFilePath != "") {
        // qDebug() << "REAL FILE IS HERE:" << fn << resolvedFilePath;
    }

    // LOAD TO MEMORY -----------
    mp3dec_t mp3d;
    mp3dec_file_info_t info;

    PerfTimer t("audiodec_load", __LINE__); // TIMER TIMER TIMER
    t.start(__LINE__);

    // if (mp3dec_load(&mp3d, fn.toStdString().c_str(), &info, NULL, NULL))
    if (audiodec_load(&mp3d, fn.toStdString().c_str(), &info, NULL, NULL))
    {
        qDebug() << "ERRORL mp3dec_load()";
        return(-1);
    }

    t.elapsed(__LINE__);  // TIMER TIMER TIMER

    // NOTE TO SELF: This is super fast to load the whole file in at once.  Consider replacing existing code with this,
    //   at least for MP3 files...

    // qDebug() << "DONE:" << info.samples << info.avg_bitrate_kbps << info.channels << info.hz << " : " << fn;
    // qDebug() << "   song length:" << (float)info.samples/((float)info.hz * (float)info.channels) << "seconds";

    // MIXDOWN TO MONO -------------
    if (info.channels != 1) {
        // skip this mixdown, if already mono
        if (info.channels == 2) {
            // if it's exactly 2 channels, mixdown
            for (size_t i = 0; i < info.samples/2; i++) {
                // each sample is exactly 2 channels
                info.buffer[i] = (info.buffer[2*i] + info.buffer[2*i+1])/2.0; // mixdown to mono
            }
        } else {
            qDebug() << "Channel error:" << info.channels << info.samples << info.hz << info.avg_bitrate_kbps << fn;
            free(info.buffer); // done with that memory, so free it
            return(-2); // error
        }
    }

    // NOW: first info.samples/2 float's are the mono data

    // TODO: filter LPF1500
    // // LOW PASS FILTER IT, TO ELIMINATE THE CHUCK of BOOM-CHUCK -------------------------------------
    // biquad_params<float> bq[1];
    // bq[0] = biquad_lowpass(1500.0 / 44100.0, 0.5); // Q = 0.5
    // biquad_filter<float> *filter = new biquad_filter<float>(bq);  // init the filter

    // filter->apply(monoBuffer, framesInSong);   // NOTE: applies IN PLACE

    // delete filter;

    // WRITE TO TEMP WAV FILE -----------
    // qDebug() << "fn:" << fn; // /Users/mpogue/Library/CloudStorage/Box-Box/__squareDanceMusic_Box/patter/RR 1303 - Rhythm Cloggers Medley.mp3

    // The mono WAV that Vamp actually reads is a temp file, so the song's own container format
    //   never matters here: audiodec_load() above has already decoded the MP3, M4A, WAV or FLAC
    //   to float samples, and what segmentino sees is always a WAV (issue #1760).
    QTemporaryFile temp1;           // use a temporary file
    bool errOpen = temp1.open();    // this creates the WAV file
    QString WAVfilename = temp1.fileName(); // the open created it, if it's a temp file
    temp1.setAutoRemove(false);  // don't remove it until after we process it asynchronously

    // qDebug() << "TEMP WAVfilename: " << WAVfilename;

    if (!errOpen) {
        free(info.buffer); // done with that memory, so free it
        return(-4);
    }

    int framesInSong = info.samples/2;
    WAV_FILE_INFO wavInfo2 = wav_set_info(info.hz, framesInSong, 1, 16, 2, 1);   // assumes 44.1KHz sample rate, floats, range: -1.0 to 1.0
    wav_write_file_float1(info.buffer, WAVfilename.toStdString().c_str(), wavInfo2, framesInSong);   // write entire song as mono to a file

    // RUN VAMP ON IT -----------------------------
    // start vamp on temp file, with output file a temp file, ultimate destination .../.squaredesk/bulk/patter/<filename>.results.txt
    //   and wait for it to finish

    QString pathNameToVamp(QCoreApplication::applicationDirPath());
    pathNameToVamp.append("/vamp-simple-host");

    // qDebug() << "VAMP path: " << pathNameToVamp;

    if (!QFileInfo::exists(pathNameToVamp) ) {
        qDebug() << "Vamp does not exist";
        return(-3); // ERROR, VAMP DOES NOT EXIST
    }

    QTemporaryFile tempResultsfile;           // use a temporary file for results, then copy to resultsFilename, if all goes well
    bool errOpen2 = tempResultsfile.open();   // this creates the results file in the temp directory
    tempResultsfile.setAutoRemove(true);      // remove it when we leave scope
    // tempResultsfile.setAutoRemove(false);      // DEBUG DO NOT remove it when we leave scope

    if (!errOpen2) {
        free(info.buffer); // done with that memory, so free it
        return(-4);
    }

     // qDebug() << "tempResultsFile: " << tempResultsfile.fileName();

    QProcess vampSegment;
    // qDebug() << "EXECUTING: " << pathNameToVamp << "segmentino::segmentino" << WAVfilename << "-o" << tempResultsfile.fileName();

    vampSegment.setWorkingDirectory(QCoreApplication::applicationDirPath()); // MUST set this, or it won't run
    // // vampSegment.setStandardOutputFile("/Users/mpogue/segmentDetect.so.txt");   // DEBUG
    // // vampSegment.setStandardErrorFile("/Users/mpogue/segmentDetect.se.txt");    // DEBUG
    // vampSegment.setStandardOutputFile(musicRootPath + "/.squaredesk/bulk/segmentDetect.so.txt");   // DEBUG
    // vampSegment.setStandardErrorFile(musicRootPath + "/.squaredesk/bulk/segmentDetect.se.txt");   // DEBUG

    vampSegment.start(pathNameToVamp, QStringList() << "segmentino:segmentino" << WAVfilename << "-o" << tempResultsfile.fileName()); // intentionally no "-s", to get results as float seconds
    // vampSegment.waitForFinished(5*60000);  // SYNCHRONOUS -- wait for process to be done, max 5 minutes.  Don't start another one until this one is done.

    // qint64 processId = vampSegment.processId(); // this goes away when the process finishes, so cache it here for debugging.

    for (int i = 0; i < 300; i++) {
        // qDebug() << "vampSegment " << processId << " is running...";
        bool b = vampSegment.waitForFinished(1000);  // wait for 1 second
        if (b) {
            // qDebug() << "vampSegment " << processId << " has completed normally.";
            // finished normally
            break;
        }
        if (killAllVamps || i >= 299) {
            // if we are Quitting SquareDesk, OR if Vamp has already taken 300 seconds (give up)
            // qDebug() << "Trying to kill: vampSegment" << processId << vampSegment.state();
            vampSegment.kill(); // then kill our QProcess

            // wait until it's dead (well, up to 3 seconds)
            // TODO: we still get the warning, even when we wait for 10 seconds:
            //     QProcess: Destroyed while process ("/Users/mpogue/clean3/SquareDesk/build-SquareDesk-Qt_6_5_3_for_macOS-Release/test123/SquareDesk.app/Contents/MacOS/vamp-simple-host") is still running.
            for (int j = 0; j < 3; j++) {
                sleep(1); // check status once per second
                if (vampSegment.state() != QProcess::Running) {
                    break;
                }
            }

            free(info.buffer);
            return(0);  // and break out of here, SquareDesk is going down!
        }
    }

    // COPY to final destination -------------
    // we do this so that the file is completely ready when it shows up in .squaredesk/bulk.  If we don't do this,
    //   the temporarily empty file will wake up the cloud services (dropbox, box.net, icloud, etc.) who will start to copy that
    //   empty file to the cloud.  This way, they only wake up once, and copy the final file only to the cloud.
    if (QFile::exists(resultsFilename)) {
        // qDebug() << "Removing existing file: " << resultsFilename;
        QFile::remove(resultsFilename);  // belt and suspenders
    }

    QFile::copy(tempResultsfile.fileName(), resultsFilename);  // copy it (source file will be auto-deleted

    // qDebug() << "Copy done.";

    // CLEANUP -----------
    free(info.buffer); // done with that memory, so free it

    QFile theWAVfile(WAVfilename);
    theWAVfile.remove(); // delete the temp WAV file

    // qDebug() << "DONE: " << fn;

    // RETURN RESULT CODE ----------------------
    int resultCode = 0; // all is OK
    mp3ResultsLock.lock();
    mp3Results[fn] = resultCode; // record the results
    mp3ResultsLock.unlock();

    // qDebug() << "mp3Results WITH ADD: " << mp3Results;
    // qDebug() << "finished: " << fn;

    return(resultCode);
}

void MainWindow::processFiles(QStringList &files) {
    // this will run as many in parallel as makes sense on the user's system

    if (vampFuture.isRunning()) {
        QMessageBox msgBox;
        msgBox.setText("Section calculations already in progress.\n\nPlease wait until the current calculations are complete.");
        msgBox.exec();
        return;
    }

    int n = QThread::idealThreadCount();

    if (n > 2) {
        n -= 1;  // on an 8-core machine, use 7 cores for processing
    }
    // qDebug() << "\n\nprocessFiles: " << files << n;

    QThreadPool::globalInstance()->setMaxThreadCount(n);  // this will be set back when we detect completion...
    // qDebug() << "limit to " << n << " threads";

    QThreadPool::globalInstance()->setExpiryTimeout(5*60000); // 2 minutes max, then *POOF*

    killAllVamps = false; // don't kill anything

    // start them all up!  vampFuture.cancel() will clear out any unstarted jobs
    vampFuture = QtConcurrent::mapped(files,
                      [this] (const QString &fn)
                        {
                            // qDebug() << "fn:" << fn;
                            return(processOneFile(fn));
                        }
                  );
    // qDebug() << files.length() << " files submitted for asynchronous processing...";

    // ui->statusBar->showMessage(QString::number(files.length()) + " audio files submitted for segmentation...");
}

void MainWindow::removeSectionInfoForPath(const QString &path) {
    // delete the cached section info for one song, e.g. because the audio file itself was just replaced.
    //   processOneFile() skips any file that already has a non-trivial .results.txt, so a stale results
    //   file would otherwise be reused for the new audio. (Issue #1530)
    QFile::remove(sectionResultsPathForSong(path));
}

void MainWindow::startSectionEstimation(const QStringList &paths) {
    // start section calculations for these paths, with no confirmation dialog.
    //   The caller is responsible for asking the user first, if that's appropriate.
    QStringList pathsCopy = paths;  // copy FIRST, in case the caller handed us mp3FilenamesToProcess itself

    mp3FilenamesToProcess.clear();
    mp3ResultsLock.lock();
    mp3Results.clear();
    mp3ResultsLock.unlock();

    mp3FilenamesToProcess = pathsCopy;

    // Work out where each song's results file goes HERE, on the main thread, while nothing else
    //   is touching appleMusicPersistentIDByPath.  processOneFile() runs on the thread pool and
    //   only reads this snapshot, so a library rescan part way through a long run can rebuild
    //   that hash without racing the workers (issue #1760).  processFiles() refuses to start a
    //   second run while one is in flight, so the snapshot can't be swapped out mid-run either.
    sectionResultsPathSnapshot.clear();
    sectionResultsPathSnapshot.reserve(mp3FilenamesToProcess.count());
    for (const auto &path : std::as_const(mp3FilenamesToProcess)) {
        sectionResultsPathSnapshot.insert(path, sectionResultsPathForSong(path));
    }

    // qDebug() << "mp3FilenamesToProcess:\n" << mp3FilenamesToProcess;

    processFiles(mp3FilenamesToProcess);
}

// Music > Sections.  "Current Song" is the song that's loaded; "Selected Songs" is the selection
//   in darkSongTable, which is how a user does a big run now that "for all songs..." is gone --
//   Select All in the song table, then Calculate, and the dialog tells them what they're in for
//   before anything starts (issue #1760).
void MainWindow::on_menuSections_aboutToShow()
{
    const int selectedCount = darkSongTableSelectedVisibleRows().count();

    const bool haveCurrentSong = !currentMP3filenameWithPath.isEmpty();
    ui->actionEstimate_for_this_song->setEnabled(haveCurrentSong);
    ui->actionRemove_for_this_song->setEnabled(haveCurrentSong);

    // Keep the count in the menu item itself honest, so the user knows how big a job they're
    //   about to ask for before they even let go of the mouse.
    const QString howMany = (selectedCount == 0) ? QString("Selected Songs")
                                                 : QString("%1 Selected Songs").arg(selectedCount);

    ui->actionEstimate_for_selected_songs->setText("Calculate Section Info for " + howMany + "...");
    ui->actionRemove_for_selected_songs->setText("Remove Section Info for " + howMany + "...");

    ui->actionEstimate_for_selected_songs->setEnabled(selectedCount > 0);
    ui->actionRemove_for_selected_songs->setEnabled(selectedCount > 0);
}

void MainWindow::on_actionEstimate_for_this_song_triggered()
{
    EstimateSectionsForThisSong(currentMP3filenameWithPath);
}


void MainWindow::on_actionEstimate_for_selected_songs_triggered()
{
    EstimateSectionsForTheseSongs(darkSongTableSelectedVisibleRows());
}


void MainWindow::on_actionRemove_for_this_song_triggered()
{
    RemoveSectionsForThisSong(currentMP3filenameWithPath);
}


void MainWindow::on_actionRemove_for_selected_songs_triggered()
{
    RemoveSectionsForTheseSongs(darkSongTableSelectedVisibleRows());
}


void MainWindow::on_actionRemove_for_all_songs_triggered()
{
    // QMessageBox::StandardButton reply;
    // reply = QMessageBox::question(this, "Remove Segmentation for ALL Patter tracks",
    //                               QString("Removing section information for all songs can't be undone.\n\nOK to proceed?"),
    //                               QMessageBox::Yes|QMessageBox::No);

    // if (reply == QMessageBox::No) {
    //     return;
    // }

    QMessageBox msgBox;
    msgBox.setText("Removing section information for all songs cannot be undone.");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("OK to proceed?");
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();

    if (ret == QMessageBox::No) {
        return;
    }

    QDir dir(musicRootPath + "/.squaredesk/bulk"); // BE VEWY VEWY CAREFUL

    // qDebug() << "**** REMOVING ALL RESULTS FILES FROM: " << dir.absolutePath();

    // remove files at top level
    dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
    for (const auto &dirItem : dir.entryList() ) {
        dir.remove( dirItem );
    }

    // remove subdirectories recursively
    dir.setFilter( QDir::NoDotAndDotDot | QDir::Dirs );
    for (const auto &dirItem : dir.entryList() )
    {
        QDir subDir( dir.absoluteFilePath( dirItem ) );
        subDir.removeRecursively();
    }

    // We definitely cleared the section info for the currently loaded song, so get rid of the coloring in the waveform display
    //   in case there was any...
    ui->darkSeekBar->updateBgPixmap((float*)1, 1);  // update the bg pixmap, since we no longer have section info for this song
}

// The rows the user has selected in darkSongTable, skipping any hidden by the current search
//   filter.  A song you can't see isn't one you meant to select.
QList<int> MainWindow::darkSongTableSelectedVisibleRows() const {
    QList<int> selectedRows;
    for (const auto &mi : ui->darkSongTable->selectionModel()->selectedRows()) {
        if (!ui->darkSongTable->isRowHidden(mi.row())) {
            selectedRows.append(mi.row());
        }
    }
    return(selectedRows);
}

QStringList MainWindow::darkSongTablePathsForRows(const QList<int> &rows) const {
    QStringList paths;
    for (const auto &r : std::as_const(rows)) {
        QTableWidgetItem *theItem = ui->darkSongTable->item(r, kPathCol);
        if (theItem != nullptr) {
            paths.append(theItem->data(Qt::UserRole).toString());
        }
    }
    return(paths);
}

// "about 4 minutes", "about 30 seconds", etc.
static QString humanizedDuration(double seconds) {
    if (seconds < 90.0) {
        return(QString("about %1 seconds").arg(qRound(seconds / 5.0) * 5));
    }
    const int minutes = qRound(seconds / 60.0);
    if (minutes < 60) {
        return(QString("about %1 minutes").arg(minutes));
    }
    return(QString("about %1 hours").arg(QString::number(seconds / 3600.0, 'f', 1)));
}

void MainWindow::EstimateSectionsForTheseSongs(QList<int> rows) {
    // qDebug() << "Estimate Sections for these rows in darkSongTable: " << rows;
    EstimateSectionsForThesePaths(darkSongTablePathsForRows(rows));
}

void MainWindow::RemoveSectionsForTheseSongs(QList<int> rows) {
    // qDebug() << "Remove Sections for rows: " << rows;
    RemoveSectionsForThesePaths(darkSongTablePathsForRows(rows));
}

// THE implementation for "calculate section info for this set of songs".  Everything that offers
//   that command -- the Music > Sections menu, the darkSongTable context menu, the playlist slot
//   context menu -- lands here, so the filtering and the warning are the same wherever you start.
void MainWindow::EstimateSectionsForThesePaths(QStringList mp3Paths) {
    // qDebug() << "Estimate Sections for these paths: " << mp3Paths;

    if (mp3Paths.isEmpty()) {
        return;
    }

    // Filter to patter BEFORE asking, not after.  "Select all, then Calculate" is the intended
    //   way to do a big run now that "for all songs..." is gone, so the number in the dialog has
    //   to be the number of songs that will actually be processed -- quoting a time based on all
    //   2000 selected songs when 300 of them are patter is worse than useless (issue #1760).
    const QStringList pathsToProcess = patterPathsAmong(mp3Paths);

    if (pathsToProcess.isEmpty()) {
        QMessageBox errorBox;
        errorBox.setText("Only patter has section info.");

        // Say WHY nothing qualified.  An Apple Music track whose Type SquareDesk doesn't know is
        //   the one case the user can actually fix, so point at the preference that fixes it.
        bool anyUntypedAppleMusic = false;
        for (const auto &path : std::as_const(mp3Paths)) {
            if (appleMusicPersistentIDByPath.contains(path) && appleMusicTypeByPath.value(path).isEmpty()) {
                anyUntypedAppleMusic = true;
                break;
            }
        }

        if (anyUntypedAppleMusic) {
            errorBox.setInformativeText("SquareDesk doesn't know the Type of these Apple Music tracks.\n\n"
                                        "Set Preferences > Apple Music > \"Read Type from\" to the metadata "
                                        "field that says which of your tracks are patter.");
        } else {
            errorBox.setInformativeText(mp3Paths.count() == 1 ? "This song is not patter."
                                                             : "None of the selected songs are patter.");
        }
        errorBox.exec();
        return;
    }

    // About 30 seconds of work per song, run on the same number of threads processFiles() will
    //   use.  Note this is ROUNDED UP to whole batches, not just divided: one song takes its full
    //   30 seconds no matter how many idle cores are standing by.  Rough, but it's the difference
    //   between "this is fine" and "don't start this right before a dance".
    int threads = QThread::idealThreadCount();
    if (threads > 2) {
        threads -= 1;
    }
    threads = qMax(1, threads);
    const int batches = (pathsToProcess.count() + threads - 1) / threads; // ceil()
    const QString howLong = humanizedDuration(30.0 * batches);

    QString what;
    if (pathsToProcess.count() == mp3Paths.count()) {
        what = (pathsToProcess.count() == 1)
                   ? QString("Section info will be calculated for this song.")
                   : QString("Section info will be calculated for all %1 selected songs.").arg(pathsToProcess.count());
    } else {
        what = QString("Section info will be calculated for %1 of the %2 selected songs (only patter has sections).")
                   .arg(pathsToProcess.count()).arg(mp3Paths.count());
    }

    QMessageBox msgBox;
    msgBox.setText(what);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText(QString("This takes %1.  You can keep working while it runs.\n\nOK to start it now?").arg(howLong));
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();

    if (ret == QMessageBox::No) {
        return;
    }

    startSectionEstimation(pathsToProcess);
}

// THE implementation for "remove section info for this set of songs".
void MainWindow::RemoveSectionsForThesePaths(QStringList mp3Paths) {
    // qDebug() << "Remove Sections for these paths: " << mp3Paths;

    if (mp3Paths.isEmpty()) {
        return;
    }

    QMessageBox msgBox;
    msgBox.setText(mp3Paths.count() == 1
                       ? QString("Removing section info for this song cannot be undone.")
                       : QString("Removing section info for these %1 songs cannot be undone.").arg(mp3Paths.count()));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("OK to proceed?");
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();

    if (ret == QMessageBox::No) {
        return;
    }

    // NOTE: no ".mp3" test here any more.  Section info is keyed by song, not by container
    //   format, and an .m4a coming from Apple Music has a results file just like an .mp3 does --
    //   the old guard silently did nothing for those (issue #1760).  A song with no results file
    //   is a no-op anyway, since QFile::remove() just fails harmlessly.
    for (const auto &filenameToRemove : std::as_const(mp3Paths)) {
        QFile::remove(sectionResultsPathForSong(filenameToRemove));

        if (filenameToRemove == currentMP3filenameWithPath) {
            // if we just cleared the section info for the currently loaded song, get rid of the coloring in the waveform display
            ui->darkSeekBar->updateBgPixmap((float*)1, 1);
        }
    }
}


// Single-song convenience, for the currently loaded song and for one row / one playlist item.
//   Both of these just defer to the set-based implementations above, so there is exactly one copy
//   of the patter test, the warning wording and the results pathname rule (issue #1760).
void MainWindow::EstimateSectionsForThisSong(QString mp3Filename) {
    // qDebug() << "EstimateSections for" << mp3Filename;

    if (mp3Filename.isEmpty() || !QFile::exists(mp3Filename)) {
        // qDebug() << "No file loaded, or file does not exist: " << mp3Filename;
        QMessageBox msgBox;
        msgBox.setText("Could not find: '" + mp3Filename + "'");
        msgBox.exec();
        return;
    }

    EstimateSectionsForThesePaths(QStringList(mp3Filename));
}

void MainWindow::RemoveSectionsForThisSong(QString mp3Filename) {
    // qDebug() << "RemoveSections for" << mp3Filename;

    if (mp3Filename.isEmpty()) {
        return;
    }

    RemoveSectionsForThesePaths(QStringList(mp3Filename));
}
