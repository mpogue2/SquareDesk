/****************************************************************************
**
** Copyright (C) 2016-2023 Mike Pogue, Dan Lyke
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

    // if the bulk directory doesn't exist, create it
    QDir dir(WAVfiledir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

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
        return(-3); // ERROR, VAMP DOES NOT EXIST
    }

    QTemporaryFile tempResultsfile;           // use a temporary file for results, then copy to resultsFilename, if all goes well
    bool errOpen2 = tempResultsfile.open();   // this creates the results file in the temp directory
    tempResultsfile.setAutoRemove(true);      // remove it when we leave scope

    if (!errOpen2) {
        free(info.buffer); // done with that memory, so free it
        return(-4);
    }

    // qDebug() << "tempResultsFile: " << tempResultsfile.fileName();

    QProcess vampSegment;
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
    double secondsPerSong = 30.0; // / (QThread::idealThreadCount() - 1);

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "LONG OPERATION: Segmentation for ALL Patter recordings",
                                  QString("Calculating section info can take about ") + QString::number((int)secondsPerSong) + " seconds per song. You can keep working while it runs.\n\nOK to start it now?",
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::No) {
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
        if (numMP3files < maxFiles && s2[0] == "patter") {
            // qDebug() << "adding: " << s;
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
    double secondsPerSong = 30.0; // / (QThread::idealThreadCount() - 1);

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "LONG OPERATION: Segmentation for ALL Patter tracks",
                                  QString("Section calculations average about ") + QString::number((int)secondsPerSong) + " seconds per patter track. You can keep working while it runs.\n\nOK to start it now?",
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::No) {
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
        if (numMP3files < maxFiles && s2[0] == "patter") {
            // qDebug() << "adding: " << s;
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
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Remove Segmentation for ALL Patter tracks",
                                  QString("Removing section information for all songs can't be undone.\n\nOK to proceed?"),
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::No) {
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

    double secondsPerSong = 30.0; // / (QThread::idealThreadCount() - 1);

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "LONG OPERATION: Calculating section info for THIS Patter track",
                                  QString("Section calculations for this track could take ") + QString::number((int)secondsPerSong) + " seconds or longer. You can keep working while it runs.\n\nOK to start it now?",
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::No) {
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

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Remove Section Info for ALL Patter tracks",
                                  QString("Removing section info for this song can't be undone.\n\nOK to proceed?"),
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::No) {
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
}
