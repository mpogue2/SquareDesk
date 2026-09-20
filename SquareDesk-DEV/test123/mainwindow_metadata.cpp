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
#include "ui_updateID3TagsDialog.h"

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
#include <QDateTime>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QThread>
#include <QThreadPool>
#include <QApplication>
#include <QDebug>

#ifdef Q_OS_LINUX
#include <QtConcurrent/QtConcurrent>
#else
#include <QtConcurrent>
#endif
#include <QtMultimedia>

#define MINIMP3_FLOAT_OUTPUT
#include "third_party/minimp3_ex.h"

#include "xxhash64.h"

// TAGLIB stuff is MAC OS X and WIN32 only for now...
#include <taglib/toolkit/tlist.h>
#include <taglib/fileref.h>
#include <taglib/toolkit/tfile.h>
#include <taglib/tag.h>
#include <taglib/toolkit/tpropertymap.h>

#include <taglib/mpeg/mpegfile.h>
#include <taglib/mpeg/mpegproperties.h>
#include <taglib/mpeg/xingheader.h>
#include <taglib/mpeg/id3v2/id3v2tag.h>
#include <taglib/mpeg/id3v2/id3v2frame.h>
#include <taglib/mpeg/id3v2/id3v2header.h>

#include <taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.h>
#include <taglib/mpeg/id3v2/frames/commentsframe.h>
#include <taglib/mpeg/id3v2/frames/textidentificationframe.h>
#include <string>

#include "utility.h"

using namespace TagLib;

// ---------------------------------------------------------------------------------
QString MainWindow::getSongFileIdentifier(QString pathToSong) {
    Q_UNUSED(pathToSong)

    // LOAD TO MEMORY -----------
    mp3dec_t mp3d;
    mp3dec_file_info_t info;

    if (mp3dec_load(&mp3d, pathToSong.toStdString().c_str(), &info, NULL, NULL))
    {
        qDebug() << "ERROR: mp3dec_load, can't get SongFileID.";
        return(QString("0xFFFFFFFF"));  // ERROR: -1
    }

    // qDebug() << "SongFileID: " << info.samples << info.channels << info.hz << sizeof(mp3d_sample_t);

    // NOTE: THIS HASH ALGORITHM IS FOR LITTLE-ENDIAN MACHINES ONLY, e.g. Intel, ARM *********
    // On an M1 mac, this hash takes only about 20ms for 31836672 * 4 bytes = 6400MB/s (!)

    uint64_t myseed   = 0;
    uint64_t numBytes = info.samples * sizeof(mp3d_sample_t);
    uint64_t result2  = XXHash64::hash(info.buffer, numBytes, myseed);

    QString hexHash = QString::number(result2, 16); // QString of exactly 16 hex characters

    free(info.buffer);

    return(hexHash);
}

// ID3 ----------------------
// read ID3Tags from an MP3 file
//   inputs: if one of the input pointers is NULL, then do NOT return a value for that variable
//   outputs:
//      -1 returned by the function is an error return
//       0 or 0.0 returned in one of the return vars is a "not found" return
//
int MainWindow::readID3Tags(QString fileName, double *bpm, double *tbpm, uint32_t *loopStartSamples, uint32_t *loopLengthSamples) {

    // qDebug() << "readID3Tags:" << fileName << *bpm << *tbpm << *loopStartSamples << *loopLengthSamples;

    if (!fileName.endsWith(".mp3", Qt::CaseInsensitive)) {
        return(-1);
    }

    TagLib::File *theFile  = new MPEG::File(fileName.toStdString().c_str());;
    ID3v2::Tag   *id3v2tag = ((MPEG::File *)theFile)->ID3v2Tag(true);  // NULL if it doesn't have a tag, otherwise the address of the tag

    // set all return values to "not present" aka "didn't find"
    if (bpm != nullptr) {
        *bpm = 0.0;
    }
    if (tbpm != nullptr) {
        *tbpm = 0.0;
    }
    if (loopStartSamples != nullptr) {
        *loopStartSamples = 0;
    }
    if (loopLengthSamples != nullptr) {
        *loopLengthSamples = 0;
    }

    // iterate over each Frame in the Tag --------
    ID3v2::FrameList::ConstIterator it = id3v2tag->frameList().begin();
    for (; it != id3v2tag->frameList().end(); it++)
    {
        ByteVector b = (*it)->frameID();

        QString theKey; // OK, we're gonna have to do this the hard way, eh?
        for (unsigned int i = 0; i < b.size(); i++) {
            theKey.append(b.at(i));
        }

        QString theValue((*it)->toString().toCString());
        // qDebug() << "DEBUG: " << b.size() << theKey << ":" << theValue;

        if (theKey == "TXXX") // User Text frame <-- use these for LOOPSTART/LOOPLENGTH
        {
            TagLib::ID3v2::UserTextIdentificationFrame *tf = (TagLib::ID3v2::UserTextIdentificationFrame *)(*it);
            QString description = tf->description().toCString();
            // qDebug() << "DESCRIPTION:" << description;
            if (description == "LOOPSTART") {
                // qDebug() << "TXXX:" << description << ":" << tf->fieldList()[1].toCString();
                QString strValue = tf->fieldList()[1].toCString();
                uint32_t theLoopStart = strValue.toInt();
                if (loopStartSamples != nullptr) {
                    *loopStartSamples = theLoopStart;
                    // qDebug() << "found LOOPSTART:" << *loopStartSamples;
                }
            } else if (description == "LOOPLENGTH") {
                // qDebug() << "TXXX:" << description << ":" << tf->fieldList()[1].toCString();
                QString strValue = tf->fieldList()[1].toCString();
                uint32_t theLoopLength = strValue.toInt();
                if (loopLengthSamples != nullptr) {
                    *loopLengthSamples = theLoopLength;
                    // qDebug() << "found LOOPLENGTH:" << *loopLengthSamples;
                }
            } else {
                // IGNORE OTHER TXXX FRAMES
            }
        } else if ((*it)->frameID() == "TBPM") {
            QString TBPM((*it)->toString().toCString());
            double theTBPM = TBPM.toDouble();
            if (tbpm != nullptr) {
                *tbpm = theTBPM;
                // qDebug() << "TBPM:" << *tbpm;
            }
        } else if ((*it)->frameID() == "BPM") {
            QString BPM((*it)->toString().toCString());
            double theBPM = BPM.toDouble();
            if (bpm != nullptr) {
                *bpm = theBPM;
                // qDebug() << "BPM:" << *bpm;
            }
        }
    }

    return(0);
}

// ------------------------------------------------------------------------
int MainWindow::getMP3SampleRate(QString fileName) {
    int sampleRate = -1;

    TagLib::FileRef f(fileName.toUtf8().constData());

    if (!f.isNull()) {
        TagLib::AudioProperties *properties = f.audioProperties();
        sampleRate = properties->sampleRate();
    }

    return(sampleRate);
}

// ------------------------------------------------------------------------
// Exact MP3 duration, by counting frames with minimp3 (0 if the file can't be read).
//
// This reads the WHOLE file, so it is only for the MP3s that can't be answered from a header.
//   It does not decode: MP3D_SEEK_TO_SAMPLE walks the frame headers to build minimp3's seek
//   index, and dec.samples is then the exact total sample count.
static qint64 mp3FrameCountDurationMS(const QString &pathname) {
    mp3dec_ex_t dec;

    if (mp3dec_ex_open(&dec, pathname.toUtf8().constData(), MP3D_SEEK_TO_SAMPLE) != 0) {
        return(0);
    }

    qint64 durationMS = 0;
    if (dec.info.hz > 0 && dec.info.channels > 0) {
        // dec.samples counts every channel, so divide by both the rate and the channel count
        const quint64 samplesPerSecond = static_cast<quint64>(dec.info.hz)
                                       * static_cast<quint64>(dec.info.channels);
        durationMS = static_cast<qint64>((dec.samples * 1000ULL) / samplesPerSecond);
    }

    mp3dec_ex_close(&dec);
    return(durationMS);
}

// ------------------------------------------------------------------------
// DURATION of a local audio file, in milliseconds (0 if it can't be read).
//
// Apple Music tracks already have a duration, handed to us by ITLibrary as totalTimeMS, and
//   nothing here ever runs for those.  A song in the Music Directory has one too -- it just
//   wasn't being read (issue #1753).
//
// The awkward case is MP3.  When an MP3 has no Xing/Info header, TagLib does not work out the
//   length at all: it takes the bitrate of the FIRST FRAME and assumes the whole file runs at
//   that rate (mpegproperties.cpp, "we hope that we're in a constant bitrate file").  For a VBR
//   file that is simply wrong, and badly so when the song opens quietly -- one of the songs in
//   the Music Directory starts with a 32 kbps frame but averages 235 kbps, and TagLib called
//   that 3:56 song 28:57.  ReadStyle::Accurate does not help; the frame-counting path it enables
//   is reached only for ADTS streams, never for MP3.  This is not a stale-version problem
//   either: the code is byte-for-byte identical in TagLib 2.3.2, which still carries the
//   upstream "TODO: Make this more robust with audio property detection for VBR without a Xing
//   header".  (Martchus/tagparser makes exactly the same CBR assumption, and fared worse.)
//
// So: trust the header when it carries a real frame count, and count frames ourselves when it
//   doesn't.  Over the 1854 files in the Music Directory this agreed with a full decode on every
//   single one, where header-only was wrong on 8.
qint64 MainWindow::getSongDurationMS(const QString &pathname) {
    const QByteArray pathUTF8 = pathname.toUtf8();

    if (pathname.endsWith(".mp3", Qt::CaseInsensitive)) {
        // A Xing/Info header states the frame count outright, so the length derived from it is
        //   exact and costs one header read.  About a third of the Music Directory has one.
        TagLib::MPEG::File mpegFile(pathUTF8.constData(), true, TagLib::AudioProperties::Fast);
        if (mpegFile.isValid()) {
            auto *properties = dynamic_cast<TagLib::MPEG::Properties *>(mpegFile.audioProperties());
            if (properties != nullptr && properties->xingHeader() != nullptr) {
                int lengthMS = properties->lengthInMilliseconds();
                if (lengthMS > 0) {
                    return(static_cast<qint64>(lengthMS));
                }
            }
        }

        // No frame count in the file, so the only honest answer costs a full read.
        return(mp3FrameCountDurationMS(pathname));
    }

    // .wav/.flac/.m4a all state their length exactly in the header -- no VBR guesswork -- so
    //   TagLib is both right and cheap for those.
    TagLib::FileRef f(pathUTF8.constData(), true, TagLib::AudioProperties::Fast);

    if (f.isNull() || f.audioProperties() == nullptr) {
        return(0);   // e.g. a Finder alias with a .mp3 name; caller leaves the cell blank
    }

    int lengthMS = f.audioProperties()->lengthInMilliseconds();
    return(lengthMS > 0 ? static_cast<qint64>(lengthMS) : 0);
}

// ------------------------------------------------------------------------
// The duration we already know for this file, or 0 if we've never measured it.
qint64 MainWindow::cachedSongDurationMS(const QString &pathname) {
    return(durationCacheByPath.value(songSettings.removeRootDirs(pathname)).durationMS);
}

// ------------------------------------------------------------------------
// Make sure every one of these paths has a duration in durationCacheByPath, reading the ones
//   that don't.  Normally there is nothing to do and this costs one stat() per file (~5ms for
//   the whole Music Directory); it does real work only the first time each song is shown with
//   the Duration column turned on.
//
// The FIRST time is not free.  An MP3 with no Xing header has to be read in full to be counted
//   (see getSongDurationMS()), and two thirds of the Music Directory is in that shape, so the
//   very first pass over all 1854 songs takes ~2.8s on four threads.  That happens once for a
//   given song, ever -- the answers go into the song_durations table and come back from there on
//   every later launch -- and only for the songs actually on screen, so turning the column on
//   while Patter is selected measures the patter files and nothing else.
void MainWindow::warmDurationCache(const QStringList &pathnames) {
    // Everything measured in an earlier run of SquareDesk comes back here, in one query.  Done
    //   on demand rather than at startup, so a user who never turns the Duration column on never
    //   touches the table at all.
    if (!durationCacheLoadedFromDB) {
        songSettings.loadSongDurations(durationCacheByPath);
        durationCacheLoadedFromDB = true;
    }

    // Which ones do we actually have to open?  A stored duration is good only while the file's
    //   mtime and size are unchanged, so replacing a song on disk re-measures it by itself.
    QStringList toRead;                  // absolute paths, for the reader
    QList<QString> toReadKeys;           // ...and the relative paths they are stored under
    QList<SongDuration> toReadStats;     // ...and the mtime/size they were measured at

    for (const auto &pathname : pathnames) {
        QFileInfo fi(pathname);
        if (!fi.isFile()) {
            continue;   // playlists can name songs that aren't there any more
        }

        SongDuration entry;
        entry.mtimeMS  = fi.lastModified().toMSecsSinceEpoch();
        entry.fileSize = fi.size();

        const QString key = songSettings.removeRootDirs(pathname);

        auto it = durationCacheByPath.constFind(key);
        if (it != durationCacheByPath.constEnd() && it->mtimeMS == entry.mtimeMS && it->fileSize == entry.fileSize) {
            continue;   // still good
        }

        toRead.append(pathname);
        toReadKeys.append(key);
        toReadStats.append(entry);
    }

    if (toRead.isEmpty()) {
        return;
    }

    // Read them in parallel, on a pool of our own.  NOT the global pool: a bulk section
    //   calculation (see processFiles()) takes it over for minutes at a time, and queueing
    //   behind those jobs would wedge the UI for exactly as long.  Four threads measured
    //   fastest here -- this is I/O bound, and eight was slower than four.
    QThreadPool durationPool;
    durationPool.setMaxThreadCount(qMin(4, QThread::idealThreadCount()));

    // The output type is named explicitly rather than left to QtConcurrent's deduction, which
    //   has to rebind a QStringList of paths into a list of durations -- QStringList being a
    //   plain class derived from QList<QString> rather than a template.  Qt 6.11 deduces
    //   QList<qint64> correctly, but saying it outright costs nothing and can't drift.
    // blockingMapped() reduces in input order, which is what lets us pair the results with
    //   toReadStats by index below.
    const QList<qint64> durations = QtConcurrent::blockingMapped<QList<qint64>>(&durationPool, toRead,
                                        [this](const QString &pathname) {
                                            return(getSongDurationMS(pathname));
                                        });

    QHash<QString, SongDuration> measured;
    for (int i = 0; i < toRead.size(); i++) {
        SongDuration entry = toReadStats[i];
        entry.durationMS = durations[i];
        durationCacheByPath.insert(toReadKeys[i], entry);   // 0 is kept too, so we don't retry it
        measured.insert(toReadKeys[i], entry);
    }

    // Remember them, so that no later run of SquareDesk has to read these files again.
    songSettings.saveSongDurations(measured);
}

// --------------------------------------
// Extract TBPM tag from ID3v2 to know (for sure) what the BPM is for a song (overrides beat detection) -----
//
// NOTE: SquareDesk does NOT use the BPM tag, it uses the more modern TBPM tag that is
//  supported by ID3v2.

double MainWindow::getID3BPM(QString MP3FileName) {
    double TBPM = 0.0;

    readID3Tags(MP3FileName, nullptr, &TBPM, nullptr, nullptr);

    return(TBPM);
}

void MainWindow::on_actionUpdate_ID3_Tags_triggered()
{
    if (currentMP3filenameWithPath == "" ||
        currentMP3filenameWithPath.endsWith(".wav", Qt::CaseInsensitive)) {  // we don't support ID3v2 tags in WAV yet
        return;
    }

    updateDialog = new updateID3TagsDialog(this);
    setDynamicPropertyRecursive(updateDialog, "theme", currentThemeString);

    // UpdateID3Tags dialog is MODAL ---------
    int dialogCode = updateDialog->exec();

    QString oldMusicRootPath = prefsManager.GetmusicPath();

    // act on dialog return code
    if(dialogCode == QDialog::Accepted) {
        uint8_t TBPM = updateDialog->newTBPM;
        uint32_t START = updateDialog->newLoopStart;
        uint32_t LENGTH = updateDialog->newLoopLength;

        QString MODIFYFILE = currentMP3filenameWithPath;
        QString BACKUPFILE = musicRootPath + "/" + updateDialog->ui->fileNameToBackupTo->text();

        QFileInfo fi_mod(MODIFYFILE);
        QFileInfo fi_back(BACKUPFILE);

        bool MODIFYFILEEXISTS = fi_mod.exists();
        bool BACKUPFILEEXISTS = fi_back.exists();

        bool MAKEBACKUPFILE = updateDialog->ui->doBackupCheckBox->isChecked();

        if (MODIFYFILEEXISTS &&
            (updateDialog->newTBPMValid) &&
            (updateDialog->newLoopStartValid) &&
            (updateDialog->newLoopLengthValid)) {
            // then all 3 are valid, and the file still exists (just checking!)

            // qDebug() << "File valid, and all 3 parameters are valid.";

            if (MAKEBACKUPFILE) {

                if (BACKUPFILEEXISTS) {
                    // WARNING DIALOG!! ---------
                    QMessageBox msgBox;

                    msgBox.setText(QString("Backup file already exists."));
                    msgBox.setIcon(QMessageBox::Question);
                    msgBox.setInformativeText("Overwrite the backup file with a new backup file?");
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
                    msgBox.setDefaultButton(QMessageBox::Cancel);

                    int ret = msgBox.exec();

                    if (ret == QMessageBox::Yes) {
                        QFile oldBackupFile(BACKUPFILE);
                        bool removeSuccess = oldBackupFile.remove();  // <----- HERE IS THE REMOVAL OF THE OLD BACKUP FILE

                        if (!removeSuccess) {
                            QMessageBox msgBox2;
                            msgBox2.setText("The backup file could not be removed.\nID3v2 tag NOT updated.");
                            msgBox2.exec();
                            return; // can't recover from this...
                        }
                    } else {
                        // user told us not to remove the old backup file.
                        QMessageBox msgBox3;
                        msgBox3.setText("User cancelled removal of backup file.\nID3v2 tag NOT updated.");
                        msgBox3.exec();
                        return; // can't recover from this...
                    }
                }

                filewatcherShouldIgnoreOneFileSave = true; // don't reload songTable just because we made a backup audio file
                // qDebug() << "***** COPYING:" << MODIFYFILE << " to " << BACKUPFILE;

                QFile mfile(MODIFYFILE);

                bool backupCopySuccess = mfile.copy(BACKUPFILE);   // <----- HERE IS THE COPY OF THE AUDIO FILE TO BACKUP FILE

                if (!backupCopySuccess) {
                    QMessageBox msgBox4;
                    msgBox4.setText("Could not copy to the backup file.\nID3v2 tag NOT updated.");
                    msgBox4.exec();
                    return; // can't recover from this...
                }
            }

            filewatcherShouldIgnoreOneFileSave = true; // don't reload songTable because we wrote to an audio file
            // NOTE: these two sets-to-true should work to stop the FileWatcher, which should only NOT wake up once,
            //   since both the COPY and the WRITE ID3v2 should occur before we get back to the main loop

            // qDebug() << "***** WRITE ID3v2 TAG TO:" << MODIFYFILE << ", with:" << TBPM << START << LENGTH;

            TagLib::MPEG::File f(MODIFYFILE.toUtf8().constData());
            ID3v2::Tag *id3v2tag = f.ID3v2Tag(true);

            // =========================================================================================================
            // UPDATE TO NEW VALUES.  NOTE: If any of the edit box values is "-", don't update that value in the MP3 file.
            // =========================================================================================================
            QString newTBPM = updateDialog->ui->newTBPMEditBox->text();

            if (newTBPM != "-") {
                id3v2tag->setTBPM(newTBPM.toStdString());
            }

            // =====================
            QString newLOOPSTART = updateDialog->ui->newLoopStartEditBox->text();
            TagLib::ID3v2::UserTextIdentificationFrame *frame = 0;

            if (newLOOPSTART != "-") {
                frame = TagLib::ID3v2::UserTextIdentificationFrame::find(id3v2tag, String("LOOPSTART"));
                if (!frame) {
                    // wasn't one, so let's make a new one
                    frame = new TagLib::ID3v2::UserTextIdentificationFrame("TXXX"); // make a new one
                    frame->setDescription("LOOPSTART");
                    frame->setText(newLOOPSTART.toStdString()); // and update it
                    id3v2tag->addFrame(frame);  // then add it
                }

                // now it should always succeed
                TagLib::ID3v2::UserTextIdentificationFrame *frame2 = 0;
                frame2 = TagLib::ID3v2::UserTextIdentificationFrame::find(id3v2tag, String("LOOPSTART"));
                if (frame2) {
                    frame2->setText(newLOOPSTART.toStdString()); // and update it
                } else {
                    qDebug() << "ERROR: didn't make a LOOPSTART frame";
                }
            }

            // =====================
            QString newLOOPLENGTH = updateDialog->ui->newLoopLengthEditBox->text();
            TagLib::ID3v2::UserTextIdentificationFrame *frame3 = 0;

            if (newLOOPLENGTH != "-") {
                frame3 = TagLib::ID3v2::UserTextIdentificationFrame::find(id3v2tag, String("LOOPLENGTH"));
                if (!frame3) {
                    // wasn't one, so let's make a new one
                    frame3 = new TagLib::ID3v2::UserTextIdentificationFrame("TXXX"); // make a new one
                    frame3->setDescription("LOOPLENGTH");
                    frame3->setText(newLOOPLENGTH.toStdString()); // and update it
                    id3v2tag->addFrame(frame3);  // then add it
                }

                // now it should always succeed
                TagLib::ID3v2::UserTextIdentificationFrame *frame4 = 0;
                frame4 = TagLib::ID3v2::UserTextIdentificationFrame::find(id3v2tag, String("LOOPLENGTH"));
                if (frame4) {
                    frame4->setText(newLOOPLENGTH.toStdString()); // and update it
                } else {
                    qDebug() << "ERROR: didn't make a LOOPLENGTH frame";
                }
            }

            // qDebug() << "SAVING FILE!";
            bool saveSuccess = f.save(); // <----- HERE IS THE SAVE OF THE AUDIO FILE WITH NEW TAGS

            if (!saveSuccess) {
                QMessageBox msgBox5;
                msgBox5.setText("Could not save the new version of the file with updated ID3v2 tags.\nID3v2 tag probably NOT updated.");
                msgBox5.exec();
                return; // can't recover from this...
            }

            // qDebug() << "DONE.";

        } else {
            qDebug() << "***** ERROR 79: problem modifying ID3v2 tag" << MODIFYFILE << MODIFYFILEEXISTS << TBPM << START << LENGTH;
            QMessageBox msgBox6;
            msgBox6.setText("Could not write the IDv3 tag, because some of the new values were not valid.");
            msgBox6.setIcon(QMessageBox::Critical);
            msgBox6.exec();
        }

    } else {
        // qDebug() << "REJECTED.  So sad.";
    }

}

void MainWindow::printID3Tags(QString fileName) {
#if DEBUGID3
    TagLib::MPEG::File f(fileName.toUtf8().constData());
    ID3v2::Tag *id3v2tag = f.ID3v2Tag(true);

    // print NO MODIFY the entire framelist, showing updates ----------
    // qDebug() << "printID3Tags --------------------";
    TagLib::ID3v2::FrameList fl = id3v2tag->frameList();  // update the framelist
    for (TagLib::ID3v2::FrameList::Iterator lit = fl.begin(); lit != fl.end(); ++lit) {
        String key((*lit)->frameID());
        // qDebug() << "Key:" << key.toCString();
        if ( key == "TIT2" || key == "TALB" || key == "TPE1" || key == "TBPM")
        {
            TagLib::ID3v2::TextIdentificationFrame* textFrame = dynamic_cast<ID3v2::TextIdentificationFrame*>(*lit);
            Q_UNUSED(textFrame)
            // qDebug() << key.toCString()      // key
            //          << textFrame->toString().toCString(); // value
        } else if ( key == "TXXX") {
            TagLib::ID3v2::UserTextIdentificationFrame* textFrame = dynamic_cast<ID3v2::UserTextIdentificationFrame*>(*lit);
            QString description = textFrame->description().toCString();
            Q_UNUSED(description)
            // qDebug() << "TXXX"                                  // TXXX
            //          << description                             // e.g. LOOPSTART
            //          << textFrame->fieldList()[1].toCString();  // value
        }
    }
    // qDebug() << "-------------------- printID3Tags";
    Q_UNUSED(fileName)
#else
    Q_UNUSED(fileName)
#endif
}

void MainWindow::on_actionIn_Out_Loop_points_to_default_triggered(bool checked) {
    Q_UNUSED(checked)
    double BPM, TBPM;
    uint32_t loopStart_samples, loopLength_samples;

    int result = readID3Tags(currentMP3filenameWithPath, &BPM, &TBPM, &loopStart_samples, &loopLength_samples);
    if ((result == -1) || (loopLength_samples == 0)) { // TODO: BUG - do I ever check for loopStart_samples == 0, which is VALID?
        return;
    }

    int sampleRate = getMP3SampleRate(currentMP3filenameWithPath);
    if (sampleRate != -1) {
        double loopStart_ms = 1000.0 * (double)loopStart_samples / (double)sampleRate;
        double loopEnd_ms   = 1000.0 * (double)(loopStart_samples + loopLength_samples) / (double)sampleRate;

        // qDebug() << "NOW SET THESE: " << loopStart_samples << loopLength_samples << loopStart_ms << loopEnd_ms;

        ui->darkStartLoopTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(loopStart_ms)));
        ui->darkEndLoopTime->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(loopEnd_ms)));
    }
}
