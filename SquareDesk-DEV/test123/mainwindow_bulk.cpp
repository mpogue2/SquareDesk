/****************************************************************************
**
** Copyright (C) 2016-2024 Mike Pogue, Dan Lyke
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

#include <QtConcurrent>
#include <QtMultimedia>

#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

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

int MainWindow::processOneFile(const QString &fn) {
    // returns 0 if OK, else error code

    // qDebug() << "processOneFile: " << fn;
    // sleep(10); // sleep 10 seconds!

    // PROCESS MP3 FILE ==================

    // FIGURE OUT FILENAMES, AND SKIP IF RESULTS ALREADY PRESENT ---------
    QString bulkDirname = musicRootPath + "/.squaredesk/bulk";

    QString WAVfilename = fn;
    WAVfilename.replace(musicRootPath, bulkDirname);
    QString resultsFilename = WAVfilename + ".results.txt";

    WAVfilename.replace(".mp3",".wav",Qt::CaseInsensitive);
    QFileInfo finfo(WAVfilename);
    QString WAVfiledir = finfo.absolutePath();

    // qDebug() << "WAVfiledir (results go here): " << WAVfiledir;
    QDir().mkpath(WAVfiledir); // make sure that the results folder exists, e.g. .squaredesk/bulk/patter/RIV 123 - foo.results.txt

    QFileInfo resultsFileinfo(resultsFilename);
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

    QString resolvedFilePath = finfo.symLinkTarget(); // path with the symbolic links followed/removed
    if (resolvedFilePath != "") {
        // qDebug() << "REAL FILE IS HERE:" << fn << resolvedFilePath;
    }

    // LOAD TO MEMORY -----------
    mp3dec_t mp3d;
    mp3dec_file_info_t info;
    if (mp3dec_load(&mp3d, fn.toStdString().c_str(), &info, NULL, NULL))
    {
        qDebug() << "ERRORL mp3dec_load()";
        return(-1);
    }

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

    // qDebug() << "path,dir" << WAVfilename << WAVfiledir;

    // // if the bulk directory doesn't exist, create it
    // QDir dir(WAVfiledir);
    // if (!dir.exists()) {
    //     dir.mkpath(".");
    // }

    // write WAV file to .squaredesk/bulk folder (e.g. .../patter/filename.wav)

    // TODO: CHANGE TO A TEMP FILE, SO THAT IT DOESN'T HAVE TO GO TO iCLOUD ***********
    QTemporaryFile temp1;           // use a temporary file
    bool errOpen = temp1.open();    // this creates the WAV file
    WAVfilename = temp1.fileName(); // the open created it, if it's a temp file
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

void MainWindow::on_darkSegmentButton_clicked()
{
    // double secondsPerSong = 30.0; // / (QThread::idealThreadCount() - 1);

    // QMessageBox::StandardButton reply;
    // reply = QMessageBox::question(this, "LONG OPERATION: Segmentation for ALL Patter recordings",
    //                               QString("Calculating section info can take about ") + QString::number((int)secondsPerSong) + " seconds per song. You can keep working while it runs.\n\nOK to start it now?",
    //                               QMessageBox::Yes|QMessageBox::No);

    // if (reply == QMessageBox::No) {
    //     return;
    // }

    QMessageBox msgBox;
    msgBox.setText("Calculating section info can take about 30 seconds per song.  You can keep working while it runs.");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("OK to start it now?");
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();

    if (ret == QMessageBox::No) {
        return;
    }

    mp3FilenamesToProcess.clear();
    mp3ResultsLock.lock();
    mp3Results.clear();
    mp3ResultsLock.unlock();

    int numMP3files = 0;

    QListIterator<QString> iter(*pathStack);
    while (iter.hasNext()) {

        QString s = iter.next();

        int maxFiles = 99999;
        QStringList s2 = s.split("#!#");

        // qDebug() << "on_darkSegmentButton_clicked(): s2[0] = " << s2[0];

        // if (numMP3files < maxFiles && s2[0] == "patter") {
        if (numMP3files < maxFiles && songTypeNamesForPatter.contains(s2[0])) {
            // qDebug() << "adding: " << s2[0] << s;
            if (s2[1].endsWith(".mp3", Qt::CaseInsensitive)) {
                mp3FilenamesToProcess.append(s2[1]);
                numMP3files++;
            }
        }
    }

    // qDebug() << "mp3FilenamesToProcess:\n" << mp3FilenamesToProcess;

    processFiles(mp3FilenamesToProcess);
}


void MainWindow::on_actionEstimate_for_this_song_triggered()
{
    EstimateSectionsForThisSong(currentMP3filenameWithPath);
}


void MainWindow::on_actionEstimate_for_all_songs_triggered()
{
    // double secondsPerSong = 30.0; // / (QThread::idealThreadCount() - 1);

    // QMessageBox::StandardButton reply;
    // reply = QMessageBox::question(this, "LONG OPERATION: Segmentation for ALL Patter tracks",
    //                               QString("Section calculations average about ") + QString::number((int)secondsPerSong) + " seconds per patter track. You can keep working while it runs.\n\nOK to start it now?",
    //                               QMessageBox::Yes|QMessageBox::No);

    // if (reply == QMessageBox::No) {
    //     return;
    // }

    QMessageBox msgBox;
    msgBox.setText("Calculating section info can take about 30 seconds per song.  You can keep working while it runs.");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("OK to start it now?");
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();

    if (ret == QMessageBox::No) {
        return;
    }

    mp3FilenamesToProcess.clear();
    mp3ResultsLock.lock();
    mp3Results.clear();
    mp3ResultsLock.unlock();

    int numMP3files = 0;

    QListIterator<QString> iter(*pathStack);
    while (iter.hasNext()) {

        QString s = iter.next();

        int maxFiles = 99999;
        QStringList s2 = s.split("#!#");

        // qDebug() << "on_actionEstimate_for_all_songs_triggered(): s2[0] = " << s2[0];

        // if (numMP3files < maxFiles && s2[0] == "patter") {
        if (numMP3files < maxFiles && songTypeNamesForPatter.contains(s2[0])) {
            // qDebug() << "adding: " << s << s2[0];
            if (s2[1].endsWith(".mp3", Qt::CaseInsensitive)) {
                mp3FilenamesToProcess.append(s2[1]);
                numMP3files++;
            }
        }
    }

    // qDebug() << "mp3FilenamesToProcess:\n" << mp3FilenamesToProcess;

    processFiles(mp3FilenamesToProcess);
}


void MainWindow::on_actionRemove_for_this_song_triggered()
{
    RemoveSectionsForThisSong(currentMP3filenameWithPath);
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
    foreach( QString dirItem, dir.entryList() ) {
        dir.remove( dirItem );
    }

    // remove subdirectories recursively
    dir.setFilter( QDir::NoDotAndDotDot | QDir::Dirs );
    foreach( QString dirItem, dir.entryList() )
    {
        QDir subDir( dir.absoluteFilePath( dirItem ) );
        subDir.removeRecursively();
    }

    // We definitely cleared the section info for the currently loaded song, so get rid of the coloring in the waveform display
    //   in case there was any...
    ui->darkSeekBar->updateBgPixmap((float*)1, 1);  // update the bg pixmap, since we no longer have section info for this song
}

void MainWindow::EstimateSectionsForTheseSongs(QList<int> rows) {
    qDebug() << "Estimate Sections for these rows in darkSongTable: " << rows;

    QMessageBox msgBox;
    msgBox.setText("Calculating section info can take about 30 seconds per song.  You can keep working while it runs.");
    msgBox.setInformativeText("OK to start it now?");
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();

    if (ret == QMessageBox::No) {
        return;
    }

    mp3FilenamesToProcess.clear();
    mp3ResultsLock.lock();
    mp3Results.clear();
    mp3ResultsLock.unlock();

    foreach (const int &r, rows) {
        mp3FilenamesToProcess.append(ui->darkSongTable->item(r, kPathCol)->data(Qt::UserRole).toString());
    }

    processFiles(mp3FilenamesToProcess);
}

void MainWindow::RemoveSectionsForTheseSongs(QList<int> rows) {
    qDebug() << "Remove Sections for rows: " << rows;

    QMessageBox msgBox;
    msgBox.setText("Removing section info for these songs cannot be undone.");
    msgBox.setInformativeText("OK to proceed?");
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();

    if (ret == QMessageBox::No) {
        return;
    }

    foreach (const int &r, rows) {
        QString filenameToRemove = ui->darkSongTable->item(r, kPathCol)->data(Qt::UserRole).toString();

        if (filenameToRemove.endsWith(".mp3", Qt::CaseInsensitive)) {
            QString resultsFilename = filenameToRemove;
            QString bulkDirname = musicRootPath + "/.squaredesk/bulk";
            resultsFilename.replace(musicRootPath, bulkDirname);
            resultsFilename = resultsFilename + ".results.txt";

            QFile::remove(resultsFilename);
            // qDebug() << "**** REMOVED: " << resultsFilename;
            // qDebug() << "Removing section info for THIS:" << filenameToRemove << currentMP3filenameWithPath;
            if (filenameToRemove == currentMP3filenameWithPath) {
                // if we just cleared the section info for the currently loaded song, get rid of the coloring in the waveform display
                ui->darkSeekBar->updateBgPixmap((float*)1, 1);  // update the bg pixmap, since we no longer have section info for this song
            }

        }

    }
}


void MainWindow::EstimateSectionsForThisSong(QString mp3Filename) {
    // qDebug() << "EstimateSections for" << mp3Filename;

    if (!QFile::exists(mp3Filename)) {
        // qDebug() << "No file loaded, or file does not exist: " << mp3Filename;
        QMessageBox msgBox;
        msgBox.setText("Could not find: '" + mp3Filename + "'");
        msgBox.exec();
        return;
    }

    if (!mp3Filename.endsWith(".mp3")) {
        // qDebug() << "Not an MP3 song: " << mp3Filename;
        QMessageBox msgBox;
        msgBox.setText("Only MP3 files are supported right now.");
        msgBox.exec();
        return;
    }

    QString theCategory = filepath2SongCategoryName(mp3Filename);
    if (theCategory != "patter") {
        QMessageBox msgBox;
        msgBox.setText("Only patter files are supported right now.");
        msgBox.exec();
        return;
    }

    // double secondsPerSong = 30.0; // / (QThread::idealThreadCount() - 1);

    // QMessageBox::StandardButton reply;
    // reply = QMessageBox::question(this, "LONG OPERATION: Calculating section info for THIS Patter track",
    //                               QString("Section calculations for this track could take ") + QString::number((int)secondsPerSong) + " seconds or longer. You can keep working while it runs.\n\nOK to start it now?",
    //                               QMessageBox::Yes|QMessageBox::No);

    // if (reply == QMessageBox::No) {
    //     return;
    // }

    QMessageBox msgBox;
    msgBox.setText("Calculating section info for this track could take up to 30 seconds. You can keep working while it runs.");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("OK to start it now?");
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();

    if (ret == QMessageBox::No) {
        return;
    }

    mp3FilenamesToProcess.clear();
    mp3ResultsLock.lock();
    mp3Results.clear();
    mp3ResultsLock.unlock();

    mp3FilenamesToProcess.append(mp3Filename);

    // qDebug() << "mp3FilenamesToProcess:\n" << mp3FilenamesToProcess;

    processFiles(mp3FilenamesToProcess);
}

void MainWindow::RemoveSectionsForThisSong(QString mp3Filename) {
    // qDebug() << "RemoveSections for" << mp3Filename;

    if (!mp3Filename.endsWith(".mp3", Qt::CaseInsensitive)) {
        // qDebug() << "Not an MP3 song: " << mp3Filename;
        QMessageBox msgBox;
        msgBox.setText("Only MP3 files are supported right now.");
        msgBox.exec();
        return;
    }

    QString theCategory = filepath2SongCategoryName(mp3Filename);
    if (theCategory != "patter") {
        QMessageBox msgBox;
        msgBox.setText("Only patter files are supported right now.");
        msgBox.exec();
        return;
    }

    // QMessageBox::StandardButton reply;
    // reply = QMessageBox::question(this, "Remove Section Info for this track",
    //                               QString("Removing section info for this song can't be undone.\n\nOK to proceed?"),
    //                               QMessageBox::Yes|QMessageBox::No);

    // if (reply == QMessageBox::No) {
    //     return;
    // }

    QMessageBox msgBox;
    msgBox.setText("Removing section info for this song cannot be undone.");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("OK to proceed?");
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();

    if (ret == QMessageBox::No) {
        return;
    }

    if (mp3Filename.endsWith(".mp3", Qt::CaseInsensitive)) {
        QString resultsFilename = mp3Filename;
        QString bulkDirname = musicRootPath + "/.squaredesk/bulk";
        resultsFilename.replace(musicRootPath, bulkDirname);
        resultsFilename = resultsFilename + ".results.txt";

        // qDebug() << "**** REMOVE: " << resultsFilename;
        QFile::remove(resultsFilename);
    }
    // qDebug() << "Removing section info for THIS:" << mp3Filename << currentMP3filenameWithPath;
    if (mp3Filename == currentMP3filenameWithPath) {
        // if we just cleared the section info for the currently loaded song, get rid of the coloring in the waveform display
        ui->darkSeekBar->updateBgPixmap((float*)1, 1);  // update the bg pixmap, since we no longer have section info for this song
    }
}

// ********************
// These functions are variants that only load a single frame, for speed.
//   We use these to know what the sample rate was before we read it in with the Qt framework,
//   which converts everything to 44.1kHz (at our request).
// ********************
int mp3dec_load_cb_ONE(mp3dec_t *dec, mp3dec_io_t *io, uint8_t *buf, size_t buf_size, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
    Q_UNUSED(progress_cb)
    Q_UNUSED(user_data)

    if (!dec || !buf || !info || (size_t)-1 == buf_size || (io && buf_size < MINIMP3_BUF_SIZE))
        return MP3D_E_PARAM;
    uint64_t detected_samples = 0;
    // size_t orig_buf_size = buf_size;
    int to_skip = 0;
    mp3dec_frame_info_t frame_info;
    memset(info, 0, sizeof(*info));
    memset(&frame_info, 0, sizeof(frame_info));

    /* skip id3 */
    size_t filled = 0, consumed = 0;
    int eof = 0;
    // int ret = 0;
    if (io)
    {
        if (io->seek(0, io->seek_data))
            return MP3D_E_IOERROR;
        filled = io->read(buf, MINIMP3_ID3_DETECT_SIZE, io->read_data);
        if (filled > MINIMP3_ID3_DETECT_SIZE)
            return MP3D_E_IOERROR;
        if (MINIMP3_ID3_DETECT_SIZE != filled)
            return 0;
        size_t id3v2size = mp3dec_skip_id3v2(buf, filled);
        if (id3v2size)
        {
            if (io->seek(id3v2size, io->seek_data))
                return MP3D_E_IOERROR;
            filled = io->read(buf, buf_size, io->read_data);
            if (filled > buf_size)
                return MP3D_E_IOERROR;
        } else
        {
            size_t readed = io->read(buf + MINIMP3_ID3_DETECT_SIZE, buf_size - MINIMP3_ID3_DETECT_SIZE, io->read_data);
            if (readed > (buf_size - MINIMP3_ID3_DETECT_SIZE))
                return MP3D_E_IOERROR;
            filled += readed;
        }
        if (filled < MINIMP3_BUF_SIZE)
            mp3dec_skip_id3v1(buf, &filled);
    } else
    {
        mp3dec_skip_id3((const uint8_t **)&buf, &buf_size);
        if (!buf_size)
            return 0;
    }
    /* try to make allocation size assumption by first frame or vbr tag */
    mp3dec_init(dec);
    int samples;
    do
    {
        uint32_t frames;
        int i, delay, padding, free_format_bytes = 0, frame_size = 0;
        const uint8_t *hdr;
        if (io)
        {
            if (!eof && filled - consumed < MINIMP3_BUF_SIZE)
            {   /* keep minimum 10 consecutive mp3 frames (~16KB) worst case */
                memmove(buf, buf + consumed, filled - consumed);
                filled -= consumed;
                consumed = 0;
                size_t readed = io->read(buf + filled, buf_size - filled, io->read_data);
                if (readed > (buf_size - filled))
                    return MP3D_E_IOERROR;
                if (readed != (buf_size - filled))
                    eof = 1;
                filled += readed;
                if (eof)
                    mp3dec_skip_id3v1(buf, &filled);
            }
            i = mp3d_find_frame(buf + consumed, filled - consumed, &free_format_bytes, &frame_size);
            consumed += i;
            hdr = buf + consumed;
        } else
        {
            i = mp3d_find_frame(buf, buf_size, &free_format_bytes, &frame_size);
            buf      += i;
            buf_size -= i;
            hdr = buf;
        }
        if (i && !frame_size)
            continue;
        if (!frame_size)
            return 0;
        frame_info.channels = HDR_IS_MONO(hdr) ? 1 : 2;
        frame_info.hz = hdr_sample_rate_hz(hdr);
        frame_info.layer = 4 - HDR_GET_LAYER(hdr);
        frame_info.bitrate_kbps = hdr_bitrate_kbps(hdr);
        frame_info.frame_bytes = frame_size;
        samples = hdr_frame_samples(hdr)*frame_info.channels;
        if (3 != frame_info.layer)
            break;
        int ret = mp3dec_check_vbrtag(hdr, frame_size, &frames, &delay, &padding);
        if (ret > 0)
        {
            padding *= frame_info.channels;
            to_skip = delay*frame_info.channels;
            detected_samples = samples*(uint64_t)frames;
            if (detected_samples >= (uint64_t)to_skip)
                detected_samples -= to_skip;
            if (padding > 0 && detected_samples >= (uint64_t)padding)
                detected_samples -= padding;
            if (!detected_samples)
                return 0;
        }
        if (ret)
        {
            if (io)
            {
                consumed += frame_size;
            } else
            {
                buf      += frame_size;
                buf_size -= frame_size;
            }
        }
        break;
    } while(1);
    size_t allocated = MINIMP3_MAX_SAMPLES_PER_FRAME*sizeof(mp3d_sample_t);
    if (detected_samples)
        allocated += detected_samples*sizeof(mp3d_sample_t);
    else
        allocated += (buf_size/frame_info.frame_bytes)*samples*sizeof(mp3d_sample_t);
    info->buffer = (mp3d_sample_t*)malloc(allocated);
    if (!info->buffer)
        return MP3D_E_MEMORY;
    /* save info */
    info->channels = frame_info.channels;
    info->hz       = frame_info.hz;
    info->layer    = frame_info.layer;
    /* decode all frames */

    return(0);  // NOTE: THIS MODIFIED VERSION DECODES ZERO MP3 FRAMES *****

    // ================================================

//     size_t avg_bitrate_kbps = 0, frames = 0;
//     do
//     {
//         if ((allocated - info->samples*sizeof(mp3d_sample_t)) < MINIMP3_MAX_SAMPLES_PER_FRAME*sizeof(mp3d_sample_t))
//         {
//             allocated *= 2;
//             mp3d_sample_t *alloc_buf = (mp3d_sample_t*)realloc(info->buffer, allocated);
//             if (!alloc_buf)
//                 return MP3D_E_MEMORY;
//             info->buffer = alloc_buf;
//         }
//         if (io)
//         {
//             if (!eof && filled - consumed < MINIMP3_BUF_SIZE)
//             {   /* keep minimum 10 consecutive mp3 frames (~16KB) worst case */
//                 memmove(buf, buf + consumed, filled - consumed);
//                 filled -= consumed;
//                 consumed = 0;
//                 size_t readed = io->read(buf + filled, buf_size - filled, io->read_data);
//                 if (readed != (buf_size - filled))
//                     eof = 1;
//                 filled += readed;
//                 if (eof)
//                     mp3dec_skip_id3v1(buf, &filled);
//             }
//             samples = mp3dec_decode_frame(dec, buf + consumed, filled - consumed, info->buffer + info->samples, &frame_info);
//             consumed += frame_info.frame_bytes;
//         } else
//         {
//             samples = mp3dec_decode_frame(dec, buf, MINIMP3_MIN(buf_size, (size_t)INT_MAX), info->buffer + info->samples, &frame_info);
//             buf      += frame_info.frame_bytes;
//             buf_size -= frame_info.frame_bytes;
//         }
//         if (samples)
//         {
//             if (info->hz != frame_info.hz || info->layer != frame_info.layer)
//             {
//                 ret = MP3D_E_DECODE;
//                 break;
//             }
//             if (info->channels && info->channels != frame_info.channels)
//             {
// #ifdef MINIMP3_ALLOW_MONO_STEREO_TRANSITION
//                 info->channels = 0; /* mark file with mono-stereo transition */
// #else
//                 ret = MP3D_E_DECODE;
//                 break;
// #endif
//             }
//             samples *= frame_info.channels;
//             if (to_skip)
//             {
//                 size_t skip = MINIMP3_MIN(samples, to_skip);
//                 to_skip -= skip;
//                 samples -= skip;
//                 memmove(info->buffer, info->buffer + skip, samples*sizeof(mp3d_sample_t));
//             }
//             info->samples += samples;
//             avg_bitrate_kbps += frame_info.bitrate_kbps;
//             frames++;
//             if (progress_cb)
//             {
//                 ret = progress_cb(user_data, orig_buf_size, orig_buf_size - buf_size, &frame_info);
//                 if (ret)
//                     break;
//             }
//         }
//     } while (
//         false &&                     // NOTE MODIFIED FROM ORIGINAL mp3dec_load(), ONLY LOAD 1 FRAME
//         frame_info.frame_bytes);

//     if (detected_samples && info->samples > detected_samples)
//         info->samples = detected_samples; /* cut padding */
//     /* reallocate to normal buffer size */
//     if (allocated != info->samples*sizeof(mp3d_sample_t))
//     {
//         mp3d_sample_t *alloc_buf = (mp3d_sample_t*)realloc(info->buffer, info->samples*sizeof(mp3d_sample_t));
//         if (!alloc_buf && info->samples)
//             return MP3D_E_MEMORY;
//         info->buffer = alloc_buf;
//     }
//     if (frames)
//         info->avg_bitrate_kbps = avg_bitrate_kbps/frames;
//     return ret;
}

int mp3dec_load_buf_ONE(mp3dec_t *dec, const uint8_t *buf, size_t buf_size, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
    return mp3dec_load_cb_ONE(dec, 0, (uint8_t *)buf, buf_size, info, progress_cb, user_data);
}

static int mp3dec_load_mapinfo_ONE(mp3dec_t *dec, mp3dec_map_info_t *map_info, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
    int ret = mp3dec_load_buf_ONE(dec, map_info->buffer, map_info->size, info, progress_cb, user_data);
    mp3dec_close_file(map_info);
    return ret;
}

int mp3dec_load_ONE(mp3dec_t *dec, const char *file_name, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
    int ret;
    mp3dec_map_info_t map_info;
    if ((ret = mp3dec_open_file(file_name, &map_info)))
        return ret;
    return mp3dec_load_mapinfo_ONE(dec, &map_info, info, progress_cb, user_data);
}

int MainWindow::MP3FileSampleRate(QString pathToMP3) {

    if (!pathToMP3.endsWith(".mp3", Qt::CaseInsensitive)) {
        return(-1); // no error message, just don't do it.
    }

    // LOAD TO MEMORY -----------
    mp3dec_t mp3d;
    mp3dec_file_info_t info;

    // qDebug() << "***** ABOUT TO TRY TO LOAD JUST ONE FRAME *****";
    if (mp3dec_load_ONE(&mp3d, pathToMP3.toStdString().c_str(), &info, NULL, NULL))
    {
        qDebug() << "ERROR 79: mp3dec_load()";
        return(-1);
    }

    int sampleRate = info.hz;

    free(info.buffer);

    // qDebug() << "sampleRate = " << info.hz << info.channels << info.samples << info.avg_bitrate_kbps;
    // qDebug() << "***** DONE LOADING *****";

    return(sampleRate);
}

QString MainWindow::SongFileIdentifier(QString pathToSong) {
    Q_UNUSED(pathToSong)

    // LOAD TO MEMORY -----------
    mp3dec_t mp3d;
    mp3dec_file_info_t info;

    if (mp3dec_load(&mp3d, pathToSong.toStdString().c_str(), &info, NULL, NULL))
    {
        qDebug() << "ERROR: mp3dec_load";
        return(QString("0xFFFFFFFF"));  // ERROR: -1
    }

    // qDebug() << "SongFileID: " << info.samples << info.channels << info.hz << sizeof(mp3d_sample_t);

    // QByteArray byteArray((const char *)info.buffer, info.samples * info.channels * info.hz);
    // QByteArray hash = QCryptographicHash::hash(byteArray, QCryptographicHash::Md5);

    // // Convert hash to hex string
    // QString hexHash = hash.toHex();

    // NOTE: THIS HASH ALGORITHM IS FOR LITTLE-ENDIAN MACHINES ONLY, e.g. Intel, ARM *********
    // On an M1 mac, this hash takes only about 20ms for 31836672 * 4 bytes = 6400MB/s (!)

    uint64_t myseed   = 0;
    uint64_t numBytes = info.samples * sizeof(mp3d_sample_t);
    uint64_t result2  = XXHash64::hash(info.buffer, numBytes, myseed);

    QString hexHash = QString::number(result2, 16); // QString of exactly 16 hex characters
    // QString hexHash = "0123456789abcdef";  // DEBUG DEBUG

    free(info.buffer);

    return(hexHash);
}
