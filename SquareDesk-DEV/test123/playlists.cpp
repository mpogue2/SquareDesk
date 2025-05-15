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

// PLAYLIST MANAGEMENT -------------------------------

// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
// #include "tablelabelitem.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"
#include "utility.h"
#include "songlistmodel.h"
#include "tablenumberitem.h"
#include "songtitlelabel.h"

#if defined(Q_OS_LINUX)
#define OS_FALLTHROUGH [[fallthrough]]
#elif defined(Q_OS_WIN)
#define OS_FALLTHROUGH
#else
    // already defined on Mac OS X
#endif

// FORWARD DECLS ---------
extern QString getTitleColTitle(MyTableWidget *songTable, int row);

extern flexible_audio *cBass;  // make this accessible to PreferencesDialog

// CSV parsing (used only here) -----------------------------------------
// Adapted from: https://github.com/hnaohiro/qt-csv/blob/master/csv.cpp
QStringList MainWindow::parseCSV(const QString &string)
{
    enum State {Normal, Quote} state = Normal;
    QStringList fields;
    QString value;

    for (int i = 0; i < string.size(); i++)
    {
        QChar current = string.at(i);

        // Normal state
        if (state == Normal)
        {
            // Comma
            if (current == ',' || current == ';') // semicolon may be used in non-en_us locales
            {
                // Save field
                fields.append(value);
                value.clear();
            }

            // Double-quote
            else if (current == '"')
                state = Quote;

            // Other character
            else
                value += current;
        }

        // In-quote state
        else if (state == Quote)
        {
            // Another double-quote
            if (current == '"')
            {
                if (i+1 < string.size())
                {
                    QChar next = string.at(i+1);

                    // A double double-quote?
                    if (next == '"')
                    {
                        value += '"';
                        i++;
                    }
                    else
                        state = Normal;
                }
            }

            // Other character
            else
                value += current;
        }
    }
    if (!value.isEmpty())
        fields.append(value);

    return fields;
}

// returns first song error, and also updates the songCount as it goes (2 return values)
QString MainWindow::loadPlaylistFromFile(QString PlaylistFileName, int &songCount) {
    Q_UNUSED(PlaylistFileName)
    Q_UNUSED(songCount)
    return("ERROR");

//     if (!QFileInfo::exists(PlaylistFileName)) {
//         // qDebug() << "loadPlaylistFromFile could not find playlist: " << PlaylistFileName;
//         return("FILE NOT FOUND");
//     }

// //    qDebug() << "loadPlaylist: " << PlaylistFileName;
//     addFilenameToRecentPlaylist(PlaylistFileName);  // remember it in the Recent list

//     // --------
//     QString firstBadSongLine = "";
//     QFile inputFile(PlaylistFileName);
//     if (inputFile.open(QIODevice::ReadOnly)) { // defaults to Text mode

//         // first, clear all the playlist numbers that are there now.
//         for (int i = 0; i < ui->songTable->rowCount(); i++) {
//             QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//             theItem->setText("");
//         }

// //        int lineCount = 1;
//         linesInCurrentPlaylist = 0;

//         QTextStream in(&inputFile);

//         if (PlaylistFileName.endsWith(".csv", Qt::CaseInsensitive)) {
//             // CSV FILE =================================

//             QString header = in.readLine();  // read header (and throw away for now), should be "abspath,pitch,tempo"

//             // V1 playlists use absolute paths, V2 playlists use relative paths
//             // This allows two things:
//             //   - moving an entire Music Directory from one place to another, and playlists still work
//             //   - sharing an entire Music Directory across platforms, and playlists still work
//             bool v2playlist = header.contains("relpath");
//             bool v1playlist = !v2playlist;

//             while (!in.atEnd()) {
//                 QString line = in.readLine();

//                 if (line == "") {
//                     // ignore, it's a blank line
//                 }
//                 else {
//                     songCount++;  // it's a real song path

//                     QStringList list1 = parseCSV(line);  // This is more robust than split(). Handles commas inside double quotes, double double quotes, etc.

//                     bool match = false;
//                     // exit the loop early, if we find a match
//                     for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {

//                         QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();

//                         if ( (v1playlist && (list1[0] == pathToMP3)) ||             // compare absolute pathnames (fragile, old V1 way)
//                              (v2playlist && compareRelative(list1[0], pathToMP3))   // compare relative pathnames (better, new V2 way)
//                            ) {
//                             // qDebug() << "MATCH::" << list1[0] << pathToMP3;
// //                            qDebug() << "loadPlaylistFromFile: " << pathToMP3;
//                             QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//                             theItem->setText(QString::number(songCount));
// //                            qDebug() << "                     number: " << QString::number(songCount);

//                             QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);
//                             theItem2->setText(list1[1].trimmed());
// //                            qDebug() << "                     pitch: " << list1[1].trimmed();

//                             QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);
//                             theItem3->setText(list1[2].trimmed());
// //                            qDebug() << "                     tempo: " << list1[2].trimmed();

//                             match = true;
//                         }
//                     }
//                     // if we had no match, remember the first non-matching song path
//                     if (!match && firstBadSongLine == "") {
//                         firstBadSongLine = line;
//                     }

//                 }

// //                lineCount++;
//             } // while
//         }
// //        else {
// //            // M3U FILE =================================
// //            // to workaround old-style Mac files that have a bare "\r" (which readLine() can't handle)
// //            //   in particular, iTunes exported playlists use this old format.
// //            QStringList theWholeFile = in.readAll().replace("\r\n","\n").replace("\r","\n").split("\n");

// //            foreach (const QString &line, theWholeFile) {
// ////                qDebug() << "line:" << line;
// //                if (line == "#EXTM3U") {
// //                    // ignore, it's the first line of the M3U file
// //                }
// //                else if (line == "") {
// //                    // ignore, it's a blank line
// //                }
// //                else if (line.at( 0 ) == '#' ) {
// //                    // it's a comment line
// //                    if (line.mid(0,7) == "#EXTINF") {
// //                        // it's information about the next line, ignore for now.
// //                    }
// //                }
// //                else {
// //                    songCount++;  // it's a real song path

// //                    bool match = false;
// //                    // exit the loop early, if we find a match
// //                    for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {
// //                        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
// ////                        qDebug() << "pathToMP3:" << pathToMP3;
// //                        if (line == pathToMP3) { // FIX: this is fragile, if songs are moved around
// //                            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
// //                            theItem->setText(QString::number(songCount));

// //                            match = true;
// //                        }
// //                    }
// //                    // if we had no match, remember the first non-matching song path
// //                    if (!match && firstBadSongLine == "") {
// //                        firstBadSongLine = line;
// //                    }

// //                }

// //                lineCount++;

// //            } // for each line in the M3U file

// //        } // else M3U file

//         inputFile.close();
//         linesInCurrentPlaylist += songCount; // when non-zero, this enables saving of the current playlist
// //        qDebug() << "linesInCurrentPlaylist:" << linesInCurrentPlaylist;

// //        qDebug() << "FBS:" << firstBadSongLine << ", linesInCurrentPL:" << linesInCurrentPlaylist;
//         if (firstBadSongLine=="" && linesInCurrentPlaylist != 0) {
//             // a playlist is now loaded, NOTE: side effect of loading a playlist is enabling Save/SaveAs...
// //            qDebug() << "LOADED CORRECTLY. enabling Save As";
//             ui->actionSave_Playlist_2->setEnabled(true);  // Playlist > Save Playlist 'name' is enabled, because you can always save a playlist with a diff name
//             ui->actionSave_Playlist->setEnabled(true);  // Playlist > Save Playlist As...
//             ui->actionPrint_Playlist->setEnabled(true);  // Playlist > Print Playlist...
//         }
//     }
//     else {
//         // file didn't open...
//         return("");
//     }

//     return(firstBadSongLine);  // return error song (if any)
}

// PLAYLIST MANAGEMENT ===============================================
void MainWindow::finishLoadingPlaylist(QString PlaylistFileName) {
    Q_UNUSED(PlaylistFileName)
// //    qDebug() << "finishLoadingPlaylist::PlaylistFileName: " << PlaylistFileName;

//     startLongSongTableOperation("finishLoadingPlaylist"); // for performance measurements, hide and sorting off

//     // --------
//     QString firstBadSongLine = "";
//     int songCount = 0;

//     firstBadSongLine = loadPlaylistFromFile(PlaylistFileName, songCount);

//     if (firstBadSongLine == "FILE NOT FOUND") {
//         // qDebug() << "finishLoadingPlaylist could not find: " << PlaylistFileName;
//         stopLongSongTableOperation("finishLoadingPlaylist");
//         return;
//     }

// #ifdef DARKMODE
//     loadPlaylistFromFileToPaletteSlot(PlaylistFileName, 0, songCount); // also load it into slot 1
// #endif

//     // simplify path, for the error message case
//     firstBadSongLine = firstBadSongLine.split(",")[0].replace("\"", "").replace(musicRootPath, "");

//     sortByDefaultSortOrder();
//     ui->songTable->sortItems(kNumberCol);  // sort by playlist # as primary (must be LAST)

//     // select the very first row, and trigger a GO TO PREVIOUS, which will load row 0 (and start it, if autoplay is ON).
//     // only do this, if there were no errors in loading the playlist numbers.
// //    if (firstBadSongLine == "") {
// //        ui->songTable->selectRow(1); // select first row of newly loaded and sorted playlist!
// ////        on_actionPrevious_Playlist_Item_triggered();
// //    }

//     stopLongSongTableOperation("finishLoadingPlaylist"); // for performance measurements, sorting on again and show

//     // MAKE SHORT NAME FOR PLAYLIST FROM FILENAME -------
// //    qDebug() << "PlaylistFileName: " << PlaylistFileName;

// //    static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
// //    QRegularExpressionMatch match = regex1.match(PlaylistFileName);
// //    QString shortPlaylistName("ERROR 7");

// //    if (match.hasMatch())
// //    {
// //        shortPlaylistName = match.captured(1);
// //    }

//     QString shortPlaylistName = PlaylistFileName;
//     shortPlaylistName = shortPlaylistName.replace(musicRootPath + "/", "").replace(".csv","");

//     ui->actionSave_Playlist_2->setText(QString("Save Playlist '") + shortPlaylistName + "'"); // and now Playlist > Save Playlist 'name' has a name, because it was loaded (possibly with errors)

// //    QString msg1 = QString("Loaded playlist with ") + QString::number(songCount) + QString(" items.");
//     // QString msg1 = QString("Playlist: ") + shortPlaylistName;
//     QString msg1;

//     int currentTab = ui->tabWidget->currentIndex();
//     QString currentTabName = ui->tabWidget->tabText(currentTab);
//     if (currentTabName != DARKMUSICTABNAME) {
//         // suppress playlist error messages when on DarkMode tab, because they are handled
//         //   by coloring the bad entries instead.
//         if (firstBadSongLine != "") {
//             // if there was a non-matching path, tell the user what the first one of those was
//             msg1 = QString("ERROR: missing '...") + firstBadSongLine + QString("'");
//             ui->songTable->clearSelection(); // select nothing, if error
// #ifndef DEBUG_LIGHT_MODE
//             ui->statusBar->setStyleSheet("color: red");
// #endif
//         } else {
// #ifndef DEBUG_LIGHT_MODE
//             ui->statusBar->setStyleSheet("color: black");
// #endif
//         }
//     }
//     if (!darkmode) {
//         shortPlaylistName = shortPlaylistName.replace("playlists/", "");
//         msg1 = QString("Playlist: ") + shortPlaylistName;
//         ui->statusBar->showMessage(msg1);
//     }

// //    qDebug() << "FINISHED LOADED CORRECTLY. enabling Save As";
//     ui->actionSave_Playlist_2->setEnabled(false);  // Playlist > Save Playlist 'name' is disabled, because it's not modified yet
//     ui->actionSave_Playlist->setEnabled(true);  // Playlist > Save Playlist As...
//     ui->actionPrint_Playlist->setEnabled(true);  // Playlist > Print Playlist...

//     ui->songTable->scrollToItem(ui->songTable->item(0, kNumberCol)); // EnsureVisible row 0
//     ui->songTable->selectRow(0); // select first row of newly loaded and sorted playlist!

//     lastSavedPlaylist = PlaylistFileName;  // have to save something here to enable File > Save (to same place as loaded).

// //    qDebug() << "finishLoadingPlaylist: calling SetlastPlaylistLoaded with:" << shortPlaylistName;
//     prefsManager.SetlastPlaylistLoaded(shortPlaylistName); // save the name of the playlist, so we can reload at app start time
}

void MainWindow::markPlaylistModified(bool isModified) {
    Q_UNUSED(isModified)

//     playlistHasBeenModified = isModified; // we don't track this by looking at the status message anymore.

// //    qDebug() << "markPlaylistModified: " << isModified;
//     QString current = ui->statusBar->currentMessage();
//     if (current == "") {
//         current = "Playlist: Untitled"; // if there is no playlist, called it "Untitled" when we add something for first time
//     }
// //    qDebug() << "current: " << current;
//     static QRegularExpression asteriskAtEndRegex("\\*$");
//     current.replace(asteriskAtEndRegex, "");  // delete the star at the end, if present
// //    qDebug() << "now current: " << current;
//     if (isModified) {
// //        qDebug() << "updating: " << current + "*";
//         ui->statusBar->showMessage(current + "*");
//         ui->actionSave->setEnabled(true);
//     } else {
// //        qDebug() << "updating: " << current;
//         ui->statusBar->showMessage(current);
//         ui->actionSave->setEnabled(false);
//     }
}

void MainWindow::on_actionLoad_Playlist_triggered()
{
    on_darkStopButton_clicked();  // if we're loading a new PLAYLIST file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
//    const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");
//    QString musicRootPath = prefsManager.GetmusicPath();
    QString startingPlaylistDirectory = prefsManager.Getdefault_playlist_dir();

    trapKeypresses = false;
    QString PlaylistFileName =
        QFileDialog::getOpenFileName(this,
                                     tr("Load Playlist"),
                                     startingPlaylistDirectory,
                                     tr("Playlist Files (*.csv)"));
    trapKeypresses = true;
    if (PlaylistFileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
//    QDir CurrentDir;
    QFileInfo fInfo(PlaylistFileName);
    prefsManager.Setdefault_playlist_dir(fInfo.absolutePath());

//    qDebug() << "on_actionLoad_Playlist_triggered: " << PlaylistFileName;
    finishLoadingPlaylist(PlaylistFileName);
}

bool comparePlaylistExportRecord(const PlaylistExportRecord &a, const PlaylistExportRecord &b)
{
    return a.index < b.index;
}

// SAVE CURRENT PLAYLIST TO FILE
void MainWindow::saveCurrentPlaylistToFile(QString PlaylistFileName) {
    Q_UNUSED(PlaylistFileName)
//     // --------
//     QList<PlaylistExportRecord> exports;

// //    bool savingToM3U = PlaylistFileName.endsWith(".m3u", Qt::CaseInsensitive);

//     // Iterate over the songTable
//     for (int i=0; i<ui->songTable->rowCount(); i++) {
//         QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//         QString playlistIndex = theItem->text();
//         QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
// //        QString songTitle = getTitleColTitle(ui->songTable, i);
//         QString pitch = ui->songTable->item(i,kPitchCol)->text();
//         QString tempo = ui->songTable->item(i,kTempoCol)->text();
// //        qDebug() << "saveCurrentPlaylistToFile: " << tempo;

//         if (playlistIndex != "") {
//             // item HAS an index (that is, it is on the list, and has a place in the ordering)
//             // TODO: reconcile int here with double elsewhere on insertion
//             PlaylistExportRecord rec;
//             rec.index = playlistIndex.toInt();
// //            if (savingToM3U) {
// //                rec.title = pathToMP3;  // NOTE: M3U must remain absolute
// //                                        //    it will NOT survive moving musicDir or sharing it via Dropbox or Google Drive
// //            } else {
//             rec.title = pathToMP3.replace(musicRootPath, "");  // NOTE: this is now a relative (to the musicDir) path
//                                                                    //    that should survive moving musicDir or sharing it via Dropbox or Google Drive
// //            }
//             rec.pitch = pitch;
//             rec.tempo = tempo;
//             exports.append(rec);
//         }
//     }

// //    qSort(exports.begin(), exports.end(), comparePlaylistExportRecord);
//     std::sort(exports.begin(), exports.end(), comparePlaylistExportRecord);
//     // TODO: strip the initial part of the path off the Paths, e.g.
//     //   /Users/mpogue/__squareDanceMusic/patter/C 117 - Restless Romp (Patter).mp3
//     //   becomes
//     //   patter/C 117 - Restless Romp (Patter).mp3
//     //
//     //   So, the remaining path is relative to the root music directory.
//     //   When loading, first look at the patter and the rest
//     //     if no match, try looking at the rest only
//     //     if no match, then error (dialog?)
//     //   Then on Save Playlist, write out the NEW patter and the rest

//     static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
//     QRegularExpressionMatch match = regex1.match(PlaylistFileName);
//     QString shortPlaylistName("ERROR 7");

//     if (match.hasMatch())
//     {
//         shortPlaylistName = match.captured(1);
//     }

//     filewatcherShouldIgnoreOneFileSave = true;  // I don't know why we have to do this, but rootDir is being watched, which causes this to be needed.
//     QFile file(PlaylistFileName);
// //    if (PlaylistFileName.endsWith(".m3u", Qt::CaseInsensitive)) {
// //        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {  // delete, if it exists already
// //            QTextStream stream(&file);
// //            stream << "#EXTM3U" << ENDL << ENDL;

// //            // list is auto-sorted here
// //            foreach (const PlaylistExportRecord &rec, exports)
// //            {
// //                stream << "#EXTINF:-1," << ENDL;  // nothing after the comma = no special name
// //                stream << rec.title << ENDL;
// //            }
// //            file.close();
// //            addFilenameToRecentPlaylist(PlaylistFileName);  // add to the MRU list
// //            markPlaylistModified(false); // turn off the * in the status bar
// //            lastSavedPlaylist = PlaylistFileName;  // have to save something here to enable File > Save (to same place as loaded) and Auto-load
// //            prefsManager.SetlastPlaylistLoaded(shortPlaylistName); // save the name of the playlist, so we can reload at app start time
// //            ui->actionSave->setText(QString("Save Playlist '") + shortPlaylistName + "'"); // and now it has a name, because it was loaded (possibly with errors)
// //        }
// //        else {
// //            ui->statusBar->showMessage(QString("ERROR: could not open M3U file."));
// //        }
// //    }
//     /* else */ if (PlaylistFileName.endsWith(".csv", Qt::CaseInsensitive)) {
//         if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
//             QTextStream stream(&file);
//             stream << "relpath,pitch,tempo" << ENDL;  // NOTE: This is now a RELATIVE PATH, and "relpath" is used to detect that.

//             foreach (const PlaylistExportRecord &rec, exports)
//             {
//                 stream << "\"" << rec.title << "\"," <<
//                     rec.pitch << "," <<
//                     rec.tempo << ENDL; // quoted absolute path, integer pitch (no quotes), integer tempo (opt % or 0)
//             }
//             file.close();
//             addFilenameToRecentPlaylist(PlaylistFileName);  // add to the MRU list
//             markPlaylistModified(false); // turn off the * in the status bar
//             lastSavedPlaylist = PlaylistFileName;  // have to save something here to enable File > Save (to same place as loaded) and Auto-load

//             // qDebug() << "saveCurrentPlaylistToFile calling setlastPlaylistLoaded with:" << shortPlaylistName;

//             prefsManager.SetlastPlaylistLoaded(shortPlaylistName); // save the name of the playlist, so we can reload at app start time
//             ui->actionSave_Playlist_2->setText(QString("Save Playlist '") + shortPlaylistName + "'"); // and now Playlist > Save Playlist 'name' has a name, because it was loaded (possibly with errors)
//         }
//         else {
//             ui->statusBar->showMessage(QString("ERROR: could not open CSV file."));
//         }
//     }
}

void MainWindow::savePlaylistAgain() // saves without asking for a filename (File > Save)
{
// //    on_stopButton_clicked();  // if we're saving a new PLAYLIST file, stop current playback
// //    qDebug() << "savePlaylistAgain()";
//     if (lastSavedPlaylist == "") {
//         // nothing saved yet!
// //        qDebug() << "NOTHING SAVED YET.";
//         return;  // so just return without saving anything
//     }

// //    qDebug() << "OK, SAVED TO: " << lastSavedPlaylist;

//     // else use lastSavedPlaylist
// //    qDebug() << "savePlaylistAgain::lastSavedPlaylist: " << lastSavedPlaylist;
//     saveCurrentPlaylistToFile(lastSavedPlaylist);  // SAVE IT

//     // TODO: if there are no songs specified in the playlist (yet, because not edited, or yet, because
//     //   no playlist was loaded), Save Playlist... should be greyed out.
// //    QString basefilename = lastSavedPlaylist.section("/",-1,-1);

//     static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
//     QRegularExpressionMatch match = regex1.match(lastSavedPlaylist);
//     QString shortPlaylistName("ERROR 7");

//     if (match.hasMatch())
//     {
//         shortPlaylistName = match.captured(1);
//     }

// //    qDebug() << "Basefilename: " << basefilename;
//     ui->statusBar->showMessage(QString("Playlist: ") + shortPlaylistName); // this message is under the next one
// //    ui->statusBar->showMessage(QString("Playlist saved as '") + basefilename + "'", 4000); // on top for 4 sec
//     // no need to remember it here, it's already the one we remembered.

// //    ui->actionSave->setEnabled(false);  // once saved, this is not reenabled, until you change it.

//     markPlaylistModified(false); // after File > Save, file is NOT modified
}

QString MainWindow::getShortPlaylistName() {

    // if (lastSavedPlaylist == "") {
    //     return("Untitled");
    // }

    // static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
    // QRegularExpressionMatch match = regex1.match(lastSavedPlaylist);
    // QString shortPlaylistName("ERROR 17");

    // if (match.hasMatch())
    // {
    //     shortPlaylistName = match.captured(1);
    // }

    // return(shortPlaylistName);
    return("ERROR");
}

// TODO: strip off the root directory before saving...
void MainWindow::on_actionSave_Playlist_triggered()  // NOTE: this is really misnamed, it's (File > Save As)
{
// //    on_stopButton_clicked();  // if we're saving a new PLAYLIST file, stop current playback

//     // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
//     const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");

//     QString startingPlaylistDirectory = prefsManager.MySettings.value(DEFAULT_PLAYLIST_DIR_KEY).toString();
//     if (startingPlaylistDirectory.isNull()) {
//         // first time through, start at HOME
//         startingPlaylistDirectory = QDir::homePath();
//     }

//     QString preferred("CSV files (*.csv)");
//     trapKeypresses = false;

//     QString startHere = startingPlaylistDirectory + "/Untitled.csv";  // if we haven't saved yet
//     if (lastSavedPlaylist != "") {
//         startHere = lastSavedPlaylist;  // if we HAVE saved already, default to same file
//     }

//     QString PlaylistFileName =
//         QFileDialog::getSaveFileName(this,
//                                      tr("Save Playlist"),
//                                      startHere,
//                                      tr("CSV files (*.csv)"),
//                                      &preferred);  // preferred is CSV
//     trapKeypresses = true;
//     if (PlaylistFileName.isNull()) {
//         return;  // user cancelled...so don't do anything, just return
//     }

//     // not null, so save it in Settings (File Dialog will open in same dir next time)
//     QFileInfo fInfo(PlaylistFileName);
//     prefsManager.Setdefault_playlist_dir(fInfo.absolutePath());

//     saveCurrentPlaylistToFile(PlaylistFileName);  // SAVE IT

//     // TODO: if there are no songs specified in the playlist (yet, because not edited, or yet, because
//     //   no playlist was loaded), Save Playlist... should be greyed out.
// //    QString basefilename = PlaylistFileName.section("/",-1,-1);

//     static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
//     QRegularExpressionMatch match = regex1.match(PlaylistFileName);
//     QString shortPlaylistName("ERROR 7");

//     if (match.hasMatch())
//     {
//         shortPlaylistName = match.captured(1);
//     }

// //    qDebug() << "Basefilename: " << basefilename;
//     ui->statusBar->showMessage(QString("Playlist: ") + shortPlaylistName);
// //    ui->statusBar->showMessage(QString("Playlist saved as '") + basefilename + "'", 4000);

//     lastSavedPlaylist = PlaylistFileName; // remember it, for the next SAVE operation (defaults to last saved in this session)
// //    qDebug() << "Save As::lastSavedPlaylist: " << lastSavedPlaylist;

//     ui->actionSave_Playlist_2->setEnabled(false);  // now that we have Save As'd something, we can't Save that thing until modified
//     ui->actionSave_Playlist_2->setText(QString("Save Playlist") + " '" + shortPlaylistName + "'"); // and now Playlist > Save Playlist 'name' has a name
}

void MainWindow::on_actionNext_Playlist_Item_triggered()
{
    // qDebug() << "on_actionNext_Playlist_Item_triggered";

    on_darkStopButton_clicked(); // stop current playback
    saveCurrentSongSettings();
    // sourceForLoadedSong points at the table where we loaded the song from

    if (sourceForLoadedSong == nullptr || sourceForLoadedSong == ui->darkSongTable) {
        return;
    }

    QItemSelectionModel *selectionModel = sourceForLoadedSong->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;

    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } // else more than 1 row or no rows, just return -1

    // row now contains the currently-playing item, and that row is selected

    // now find the NEXT VISIBLE ROW after that one
    int maxRow = sourceForLoadedSong->rowCount() - 1;

    // which is the next VISIBLE row?
    int lastVisibleRow = row;
    row = (maxRow < row+1 ? maxRow : row+1); // bump up by 1
    while (sourceForLoadedSong->isRowHidden(row) && row < maxRow) {
        // keep bumping, until the next VISIBLE row is found, or we're at the END
        row = (maxRow < row+1 ? maxRow : row+1); // bump up by 1
    }
    if (sourceForLoadedSong->isRowHidden(row)) {
        // if we try to go past the end of the VISIBLE rows, stick at the last visible row (which
        //   was the last one we were on.  Well, that's not always true, but this is a quick and dirty
        //   solution.  If I go to a row, select it, and then filter all rows out, and hit one of the >>| buttons,
        //   hilarity will ensue.
        row = lastVisibleRow;
    }

    // if (row < 0) {
    //   // more than 1 row or no rows at all selected (BAD)
    //   return;
    // }

    if (sourceForLoadedSong == ui->darkSongTable) {
        // This does not work right now, because selection doesn't move
        // maybe because the darkSongTable is sorted?
        // on_darkSongTable_itemDoubleClicked(sourceForLoadedSong->item(row,1)); // as if we double clicked the next line
    } else {
        // it was a palette slot
        if (sourceForLoadedSong == ui->playlist1Table) {
            on_playlist1Table_itemDoubleClicked(sourceForLoadedSong->item(row,1));
        } else if (sourceForLoadedSong == ui->playlist2Table) {
            on_playlist2Table_itemDoubleClicked(sourceForLoadedSong->item(row,1));
        } else if (sourceForLoadedSong == ui->playlist3Table) {
            on_playlist3Table_itemDoubleClicked(sourceForLoadedSong->item(row,1));
        } else {
            qDebug() << "ERROR 177";
        }
    }

    // start playback automatically, if Autostart is enabled
    if (ui->actionAutostart_playback->isChecked()) {
        on_darkPlayButton_clicked();
    }

    sourceForLoadedSong->clearSelection(); // unselect current row!
    sourceForLoadedSong->selectRow(row); // select that NEXT row!

    // // This code is similar to the row double clicked code...
    // on_stopButton_clicked();  // if we're going to the next file in the playlist, stop current playback
    // saveCurrentSongSettings();

    // int row = nextVisibleSongRow();
    // if (row < 0) {
    //   // more than 1 row or no rows at all selected (BAD)
    //   return;
    // }
    // ui->songTable->selectRow(row); // select new row!

    // on_songTable_itemDoubleClicked(ui->songTable->item(row,1)); // as if we double clicked the next line

    // // start playback automatically, if Autostart is enabled
    // if (ui->actionAutostart_playback->isChecked()) {
    //     on_playButton_clicked();
    // }

}

void MainWindow::on_actionPrevious_Playlist_Item_triggered()
{
    // cBass->StreamGetPosition();  // get ready to look at the playhead position
    // if (cBass->Current_Position != 0.0) {
    //     on_stopButton_clicked();  // if the playhead was not at the beginning, just stop current playback and go to START
    //     return;                   //   and return early... (don't save the current song settings)
    // }
    // // else, stop and move to the previous song...

    // on_stopButton_clicked();  // if we were playing, just stop current playback
    // // This code is similar to the row double clicked code...
    // saveCurrentSongSettings();

    // int row = previousVisibleSongRow();
    // if (row < 0) {
    //     // more than 1 row or no rows at all selected (BAD)
    //     return;
    // }

    // ui->songTable->selectRow(row); // select new row!

    // on_songTable_itemDoubleClicked(ui->songTable->item(row,1)); // as if we double clicked the previous line

    // // start playback automatically, if Autostart is enabled
    // if (ui->actionAutostart_playback->isChecked()) {
    //     on_playButton_clicked();
    // }
}

void MainWindow::on_actionClear_Playlist_triggered()
{
    // startLongSongTableOperation("on_actionClear_Playlist_triggered");  // for performance, hide and sorting off
    // ui->songTable->blockSignals(true); // no signals when we are modifying

    // // Iterate over the songTable
    // for (int i=0; i<ui->songTable->rowCount(); i++) {
    //     QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
    //     theItem->setText(""); // clear out the current list

    //     // let's intentionally NOT clear the pitches.  They are persistent within a session.
    //     // let's intentionally NOT clear the tempos.  They are persistent within a session.
    // }

    // lastSavedPlaylist = "";       // clear the name of the last playlist, since none is loaded
    // linesInCurrentPlaylist = 0;   // the number of lines in the playlist is zero
    // ui->actionSave_Playlist_2->setText(QString("Save Playlist")); // and now it has no name, because not loaded
    // ui->actionSave_Playlist_2->setDisabled(true);
    // ui->actionSave_Playlist->setDisabled(true);
    // ui->actionPrint_Playlist->setDisabled(true);

    // sortByDefaultSortOrder();

    // ui->songTable->blockSignals(false); // re-enable signals
    // stopLongSongTableOperation("on_actionClear_Playlist_triggered");  // for performance, sorting on again and show

    // ui->songTable->clearSelection(); // when playlist is cleared, select no rows by default
    // ui->songTable->scrollToItem(ui->songTable->item(0, kNumberCol)); // EnsureVisible row 0

    // on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    // ui->statusBar->showMessage("");  // no playlist at all right now
    // markPlaylistModified(false); // turn ON the * in the status bar

    // // if playlist cleared, save that fact in Preferences by setting to ""
    // prefsManager.SetlastPlaylistLoaded(""); // save the fact that no playlist was loaded

    // ui->songTable->selectRow(1); // These 2 lines are intentionally down here
    // ui->songTable->selectRow(0); // make coloring of row 1 Title correct (KLUDGE)
    // ui->songTable->setFocus();  // if we're going to highlight the first row, we need to make it ENTER/RETURN-able
}

// ----------------------------------------------------------------------
int MainWindow::PlaylistItemCount() {
    // int playlistItemCount = 0;

    // for (int i=0; i<ui->songTable->rowCount(); i++) {
    //     QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
    //     QString playlistIndex = theItem->text();  // this is the playlist #
    //     if (playlistIndex != "") {
    //         playlistItemCount++;
    //     }
    // }

    // return (playlistItemCount);
    return(0);
}

// ----------------------------------------------------------------------
void MainWindow::PlaylistItemsToTop() {
// #ifdef DARKMODE
    if (darkmode) {
        if (!relPathInSlot[0].startsWith("/tracks/")) {
            slotModified[0] = ui->playlist1Table->moveSelectedItemsToTop() || slotModified[0];  // if nothing was selected in this slot, this call will do nothing
        }
        if (!relPathInSlot[1].startsWith("/tracks/")) {
            slotModified[1] = ui->playlist2Table->moveSelectedItemsToTop() || slotModified[1];  // if nothing was selected in this slot, this call will do nothing
        }
        if (!relPathInSlot[2].startsWith("/tracks/")) {
            slotModified[2] = ui->playlist3Table->moveSelectedItemsToTop() || slotModified[2];  // if nothing was selected in this slot, this call will do nothing
        }
        if (slotModified[0] || slotModified[1] || slotModified[2]) {
            playlistSlotWatcherTimer->start(std::chrono::seconds(10));
            return; // we changed something, no need to check songTable
        }
    }
// #endif

    // drop into this section, if it was the songTable and not one of the playlist slots that was selected
    // selections in these 4 tables are mutually exclusive

//    PerfTimer t("PlaylistItemToTop", __LINE__);
//    t.start(__LINE__);

//     int selectedRow = selectedSongRow();  // get current row or -1

//     if (selectedRow == -1) {
//         return;
//     }

//     QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
//     int currentNumberInt = currentNumberText.toInt();

// //    ui->songTable->blockSignals(true); // while updating, do NOT call itemChanged
//     startLongSongTableOperation("PlaylistItemToTop");
// //    t.elapsed(__LINE__);

//     if (currentNumberText == "") {
//         // add to list, and make it the #1

//         // Iterate over the entire songTable, incrementing every item
//         for (int i=0; i<ui->songTable->rowCount(); i++) {
//             QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//             QString playlistIndex = theItem->text();  // this is the playlist #
//             if (playlistIndex != "") {
//                 // if a # was set, increment it
//                 QString newIndex = QString::number(playlistIndex.toInt()+1);
//                 ui->songTable->item(i,kNumberCol)->setText(newIndex);
//             }
//         }

//         ui->songTable->item(selectedRow, kNumberCol)->setText("1");  // this one is the new #1

//     } else {
//         // already on the list
//         // Iterate over the entire songTable, incrementing items BELOW this item
//         for (int i=0; i<ui->songTable->rowCount(); i++) {
//             QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//             QString playlistIndexText = theItem->text();  // this is the playlist #
//             if (playlistIndexText != "") {
//                 int playlistIndexInt = playlistIndexText.toInt();
//                 if (playlistIndexInt < currentNumberInt) {
//                     // if a # was set and less, increment it
//                     QString newIndex = QString::number(playlistIndexInt+1);
// //                    qDebug() << "old, new:" << playlistIndexText << newIndex;
//                     ui->songTable->item(i,kNumberCol)->setText(newIndex);
//                 }
//             }
//         }
//         // and then set this one to #1
//         ui->songTable->item(selectedRow, kNumberCol)->setText("1");  // this one is the new #1
//     }

// //    ui->songTable->blockSignals(false); // done updating.
//     stopLongSongTableOperation("PlaylistItemToTop");
// //    t.elapsed(__LINE__);

//     on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

//     // moved an item, so we must enable saving of the playlist
//     ui->actionSave->setEnabled(true);       // menu item Save is enabled now
//     ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

//     // ensure that the selected row is still visible in the current songTable window
//     selectedRow = selectedSongRow();
//     ui->songTable->scrollToItem(ui->songTable->item(selectedRow, kNumberCol)); // EnsureVisible

//     // mark playlist modified
//     markPlaylistModified(true); // turn ON the * in the status bar

// //    t.elapsed(__LINE__);
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemsToBottom() {
// #ifdef DARKMODE
    if (darkmode) {
        if (!relPathInSlot[0].startsWith("/tracks/")) {
            slotModified[0] = ui->playlist1Table->moveSelectedItemsToBottom() || slotModified[0];  // if nothing was selected in this slot, this call will do nothing
        }
        if (!relPathInSlot[1].startsWith("/tracks/")) {
            slotModified[1] = ui->playlist2Table->moveSelectedItemsToBottom() || slotModified[1];  // if nothing was selected in this slot, this call will do nothing
        }
        if (!relPathInSlot[2].startsWith("/tracks/")) {
            slotModified[2] = ui->playlist3Table->moveSelectedItemsToBottom() || slotModified[2];  // if nothing was selected in this slot, this call will do nothing
        }
        if (slotModified[0] || slotModified[1] || slotModified[2]) {
            playlistSlotWatcherTimer->start(std::chrono::seconds(10));
            return; // we changed something, no need to check songTable
        }
    }
// #endif

//     // drop into this section, if it was the songTable and not one of the playlist slots that was selected
//     // selections in these 4 tables are mutually exclusive
//     int selectedRow = selectedSongRow();  // get current row or -1

//     if (selectedRow == -1) {
//         return;
//     }

//     QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
//     int currentNumberInt = currentNumberText.toInt();

//     int playlistItemCount = PlaylistItemCount();  // how many items in the playlist right now?

// //    ui->songTable->blockSignals(true); // while updating, do NOT call itemChanged
//     startLongSongTableOperation("PlaylistItemToBottom");

//     if (currentNumberText == "") {
//         // add to list, and make it the bottom

//         // Iterate over the entire songTable, not touching every item
//         ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(playlistItemCount+1));  // this one is the new #LAST

//     } else {
//         // already on the list
//         // Iterate over the entire songTable, decrementing items ABOVE this item
//         // TODO: turn off sorting
//         for (int i=0; i<ui->songTable->rowCount(); i++) {
//             QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//             QString playlistIndexText = theItem->text();  // this is the playlist #
//             if (playlistIndexText != "") {
//                 int playlistIndexInt = playlistIndexText.toInt();
//                 if (playlistIndexInt > currentNumberInt) {
//                     // if a # was set and more, decrement it
//                     QString newIndex = QString::number(playlistIndexInt-1);
//                     ui->songTable->item(i,kNumberCol)->setText(newIndex);
//                 }
//             }
//         }
//         // and then set this one to #LAST
//         ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(playlistItemCount));  // this one is the new #1
//     }

// //    ui->songTable->blockSignals(false); // done updating
//     stopLongSongTableOperation("PlaylistItemToBottom");

//     on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

//     // moved an item, so we must enable saving of the playlist
//     ui->actionSave->setEnabled(true);       // menu item Save is enabled now
//     ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

//     // ensure that the selected row is still visible in the current songTable window
//     selectedRow = selectedSongRow();
//     ui->songTable->scrollToItem(ui->songTable->item(selectedRow, kNumberCol)); // EnsureVisible

//     // mark playlist modified
//     markPlaylistModified(true); // turn ON the * in the status bar
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemsMoveUp() {

// #ifdef DARKMODE
    if (darkmode) {
        // qDebug() << "relPathInSlot: " << relPathInSlot[0] << relPathInSlot[1] << relPathInSlot[2];
        if (!relPathInSlot[0].startsWith("/tracks/")) {
            slotModified[0] = ui->playlist1Table->moveSelectedItemsUp() || slotModified[0];  // if nothing was selected in this slot, this call will do nothing
        }
        if (!relPathInSlot[1].startsWith("/tracks/")) {
            slotModified[1] = ui->playlist2Table->moveSelectedItemsUp() || slotModified[1];  // if nothing was selected in this slot, this call will do nothing
        }
        if (!relPathInSlot[2].startsWith("/tracks/")) {
            slotModified[2] = ui->playlist3Table->moveSelectedItemsUp() || slotModified[2];  // if nothing was selected in this slot, this call will do nothing
        }
        if (slotModified[0] || slotModified[1] || slotModified[2]) {
            playlistSlotWatcherTimer->start(std::chrono::seconds(10));
            return; // we changed something, no need to check songTable
        }
    }
// #endif

    // // drop into this section, if it was the songTable and not one of the playlist slots that was selected
    // // selections in these 4 tables are mutually exclusive
    // int selectedRow = selectedSongRow();  // get current row or -1

    // if (selectedRow == -1) {
    //     return;
    // }

    // QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number

    // if (currentNumberText == "") {
    //     // return, this is not an item that is on the playlist
    //     return;
    // }

    // int currentNumberInt = currentNumberText.toInt();

    // if (currentNumberInt <= 1) {
    //     // return, if we're already at the top of the playlist
    //     return;
    // }

    // // Iterate over the entire songTable, find the item just above this one, and move IT down (only)
    // ui->songTable->blockSignals(true); // while updating, do NOT call itemChanged

    // for (int i=0; i<ui->songTable->rowCount(); i++) {
    //     QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
    //     QString playlistIndex = theItem->text();  // this is the playlist #
    //     if (playlistIndex != "") {
    //         int playlistIndexInt = playlistIndex.toInt();
    //         if (playlistIndexInt == currentNumberInt - 1) {
    //             QString newIndex = QString::number(playlistIndex.toInt()+1);
    //             ui->songTable->item(i,kNumberCol)->setText(newIndex);
    //         }
    //     }
    // }

    // ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(currentNumberInt-1));  // this one moves UP
    // ui->songTable->blockSignals(false); // done updating

    // on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    // // moved an item, so we must enable saving of the playlist
    // ui->actionSave->setEnabled(true);       // menu item Save is enabled now
    // ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

    // // ensure that the selected row is still visible in the current songTable window
    // selectedRow = selectedSongRow();
    // ui->songTable->scrollToItem(ui->songTable->item(selectedRow, kNumberCol)); // EnsureVisible

    // // mark playlist modified
    // markPlaylistModified(true); // turn ON the * in the status bar
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemsMoveDown() {
// #ifdef DARKMODE
    if (darkmode) {
        if (!relPathInSlot[0].startsWith("/tracks/")) {
            slotModified[0] = ui->playlist1Table->moveSelectedItemsDown() || slotModified[0];  // if nothing was selected in this slot, this call will do nothing
        }
        if (!relPathInSlot[1].startsWith("/tracks/")) {
            slotModified[1] = ui->playlist2Table->moveSelectedItemsDown() || slotModified[1];  // if nothing was selected in this slot, this call will do nothing
        }
        if (!relPathInSlot[2].startsWith("/tracks/")) {
            slotModified[2] = ui->playlist3Table->moveSelectedItemsDown() || slotModified[2];  // if nothing was selected in this slot, this call will do nothing
        }
        if (slotModified[0] || slotModified[1] || slotModified[2]) {
            playlistSlotWatcherTimer->start(std::chrono::seconds(10));
            return; // we changed something, no need to check songTable
        }
    }
// #endif

//     // drop into this section, if it was the songTable and not one of the playlist slots that was selected
//     // selections in these 4 tables are mutually exclusive
//     int selectedRow = selectedSongRow();  // get current row or -1
//     // qDebug() << "selectedRow: " << selectedRow;
//     if (selectedRow == -1) {
//         return;
//     }

//     QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number

//     // qDebug() << "currentNumberText: " << currentNumberText;
//     if (currentNumberText == "") {
//         // return, this is not an item that is on the playlist
//         return;
//     }

//     int currentNumberInt = currentNumberText.toInt();
//     // qDebug() << "currentNumberInt: " << currentNumberInt;

//     int playlistItemCount = PlaylistItemCount();
//     // qDebug() << "playlistItemCount: " << playlistItemCount;

//     if (currentNumberInt >= playlistItemCount) {
//         // return, if we're already at the bottom of the playlist
//         return;
//     }

//     // Iterate over the entire songTable, find the item just BELOW this one, and move it UP (only)
//     ui->songTable->blockSignals(true); // while updating, do NOT call itemChanged

//     for (int i=0; i<ui->songTable->rowCount(); i++) {
//         QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//         QString playlistIndex = theItem->text();  // this is the playlist #
//         if (playlistIndex != "") {
//             int playlistIndexInt = playlistIndex.toInt();
//             if (playlistIndexInt == currentNumberInt + 1) {
//                 QString newIndex = QString::number(playlistIndex.toInt()-1);
//                 ui->songTable->item(i,kNumberCol)->setText(newIndex);
//             }
//         }
//     }

//     ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(currentNumberInt+1));  // this one moves UP
//     ui->songTable->blockSignals(false); // done updating

//     on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

//     // moved an item, so we must enable saving of the playlist
//     ui->actionSave->setEnabled(true);       // menu item Save is enabled now
//     ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

//     // ensure that the selected row is still visible in the current songTable window
//     selectedRow = selectedSongRow();
//     ui->songTable->scrollToItem(ui->songTable->item(selectedRow, kNumberCol)); // EnsureVisible

//     // mark playlist modified
// //    qDebug() << "marking modified...";
//     markPlaylistModified(true); // turn ON the * in the status bar
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemsRemove() {

// #ifdef DARKMODE
    if (darkmode) {
        if (!relPathInSlot[0].startsWith("/tracks/")) {
    //        ui->playlist1Table->removeSelectedItem();  // if nothing was selected in this slot, this call does nothing
            slotModified[0] = ui->playlist1Table->removeSelectedItems() || slotModified[0];  // if nothing was selected in this slot, this call will do nothing
        }
        if (!relPathInSlot[1].startsWith("/tracks/")) {
    //        ui->playlist2Table->removeSelectedItem();  // if nothing was selected in this slot, this call does nothing
            slotModified[1] = ui->playlist2Table->removeSelectedItems() || slotModified[1];  // if nothing was selected in this slot, this call will do nothing
        }
        if (!relPathInSlot[2].startsWith("/tracks/")) {
    //        ui->playlist3Table->removeSelectedItem();  // if nothing was selected in this slot, this call does nothing
            slotModified[2] = ui->playlist3Table->removeSelectedItems() || slotModified[2];  // if nothing was selected in this slot, this call will do nothing
        }

        if (slotModified[0] || slotModified[1] || slotModified[2]) {
            playlistSlotWatcherTimer->start(std::chrono::seconds(10));
            return; // we changed something, no need to check songTable
        }

        // if (darkmode) {
            return;
        // }
    }
// // #endif

//     // This is only required for lightmode

//     int selectedRow = selectedSongRow();  // get current row or -1

//     if (selectedRow == -1) {
//         return;
//     }

//     QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
//     int currentNumberInt = currentNumberText.toInt();

// //    ui->songTable->blockSignals(true); // while updating, do NOT call itemChanged
//     startLongSongTableOperation("PlaylistItemRemove");

//     // already on the list
//     // Iterate over the entire songTable, decrementing items BELOW this item
//     for (int i=0; i<ui->songTable->rowCount(); i++) {
//         QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//         QString playlistIndexText = theItem->text();  // this is the playlist #
//         if (playlistIndexText != "") {
//             int playlistIndexInt = playlistIndexText.toInt();
//             if (playlistIndexInt > currentNumberInt) {
//                 // if a # was set and more, decrement it
//                 QString newIndex = QString::number(playlistIndexInt-1);
//                 ui->songTable->item(i,kNumberCol)->setText(newIndex);
//             }
//         }
//     }
//     // and then set this one to #LAST
//     ui->songTable->item(selectedRow, kNumberCol)->setText("");  // this one is off the list

// //    ui->songTable->blockSignals(false); // done updating
//     stopLongSongTableOperation("PlaylistItemRemove");

//     on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

//     // removed an item, so we must enable saving of the playlist
//     ui->actionSave->setEnabled(true);       // menu item Save is enabled now
//     ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

//     // mark playlist modified
//     markPlaylistModified(true); // turn ON the * in the status bar
}

// LOAD RECENT PLAYLIST --------------------------------------------------------------------
void MainWindow::on_actionClear_Recent_List_triggered()
{
    QStringList recentFilePaths;  // empty list

    prefsManager.MySettings.setValue("recentFiles", recentFilePaths);  // remember the new list
    updateRecentPlaylistMenu();
}

void MainWindow::loadRecentPlaylist(int i) {
    Q_UNUSED(i)
//     on_stopButton_clicked();  // if we're loading a new PLAYLIST file, stop current playback

//     QStringList recentFilePaths = prefsManager.MySettings.value("recentFiles").toStringList();

//     if (i < recentFilePaths.size()) {
//         // then we can do it
//         QString filename = recentFilePaths.at(i);
// //        qDebug() << "loadRecentPlaylist: " << filename;
//         finishLoadingPlaylist(filename);

//         addFilenameToRecentPlaylist(filename);
//     }
}

void MainWindow::updateRecentPlaylistMenu() {
    QStringList recentFilePaths = prefsManager.MySettings.value("recentFiles").toStringList();

    int numRecentPlaylists = recentFilePaths.length();
    ui->actionRecent1->setVisible(numRecentPlaylists >=1);
    ui->actionRecent2->setVisible(numRecentPlaylists >=2);
    ui->actionRecent3->setVisible(numRecentPlaylists >=3);
    ui->actionRecent4->setVisible(numRecentPlaylists >=4);
    ui->actionRecent5->setVisible(numRecentPlaylists >=5);
    ui->actionRecent6->setVisible(numRecentPlaylists >=6);

    QString playlistsPath = musicRootPath + "/playlists/";

    switch(numRecentPlaylists) {
        case 6: ui->actionRecent6->setText(QString(recentFilePaths.at(5)).replace(playlistsPath,"").replace(".csv",""));  // intentional fall-thru
        OS_FALLTHROUGH;
        case 5: ui->actionRecent5->setText(QString(recentFilePaths.at(4)).replace(playlistsPath,"").replace(".csv",""));  // intentional fall-thru
        OS_FALLTHROUGH;
        case 4: ui->actionRecent4->setText(QString(recentFilePaths.at(3)).replace(playlistsPath,"").replace(".csv",""));  // intentional fall-thru
        OS_FALLTHROUGH;
        case 3: ui->actionRecent3->setText(QString(recentFilePaths.at(2)).replace(playlistsPath,"").replace(".csv",""));  // intentional fall-thru
        OS_FALLTHROUGH;
        case 2: ui->actionRecent2->setText(QString(recentFilePaths.at(1)).replace(playlistsPath,"").replace(".csv",""));  // intentional fall-thru
        OS_FALLTHROUGH;
        case 1: ui->actionRecent1->setText(QString(recentFilePaths.at(0)).replace(playlistsPath,"").replace(".csv",""));  // intentional fall-thru
        OS_FALLTHROUGH;
        default: break;
    }

    ui->actionClear_Recent_List->setEnabled(numRecentPlaylists > 0);
}

void MainWindow::addFilenameToRecentPlaylist(QString filename) {
//    qDebug() << "addFilenameToRecentPlaylist:" << filename;

    if (!filename.endsWith(".squaredesk/current.csv")) {  // do not remember the initial persistent playlist
        QStringList recentFilePaths = prefsManager.MySettings.value("recentFiles").toStringList();

        recentFilePaths.removeAll(filename);  // remove if it exists already
        recentFilePaths.prepend(filename);    // push it onto the front
        while (recentFilePaths.size() > 6) {  // get rid of those that fell off the end
            recentFilePaths.removeLast();
        }

        prefsManager.MySettings.setValue("recentFiles", recentFilePaths);  // remember the new list
        updateRecentPlaylistMenu();
    }
}

void MainWindow::on_actionRecent1_triggered()
{
    loadRecentPlaylist(0);
}

void MainWindow::on_actionRecent2_triggered()
{
    loadRecentPlaylist(1);
}

void MainWindow::on_actionRecent3_triggered()
{
    loadRecentPlaylist(2);
}

void MainWindow::on_actionRecent4_triggered()
{
    loadRecentPlaylist(3);
}

void MainWindow::on_actionRecent5_triggered()
{
    loadRecentPlaylist(4);
}

void MainWindow::on_actionRecent6_triggered()
{
    loadRecentPlaylist(5);
}

// -------------------------
void MainWindow::tableItemChanged(QTableWidgetItem* item) {
    Q_UNUSED(item)
//     // return; // DEBUG
//     int col = ((TableNumberItem *)item)->column();
//     if (col != 0) {
//         return;  // ignore all except # column
//     }

//     ui->songTable->setSortingEnabled(false); // disable sorting

//     bool ok;
//     QString itemText = ((TableNumberItem *)item)->text();
//     int itemInteger = (itemText.toInt(&ok));
// //    itemText.toInt(&ok);  // just sets ok or not, throw away the itemInteger intentionally
//     int itemRow = ((TableNumberItem *)item)->row();

// //    qDebug() << "tableItemChanged: " << itemText << ok << itemInteger << itemRow;

//     if (ok && itemInteger < 0) {
//         // user: don't try anything funny...
//         itemText = "1";
//         itemInteger = 1;
//         ui->songTable->blockSignals(true);  // block signals, so changes are not recursive
//         item->setText("1");
//         ui->songTable->blockSignals(false);  // block signals, so changes are not recursive
//     }

//     if ((itemText == "") || (ok && itemInteger >= 1)) { // if item was deleted by clearing it out manually, or if it was an integer, then renumber
//         // NOTE: the itemInteger >= 1 is just to turn off the warning about itemInteger not being read...
// //        qDebug() << "tableItemChanged row/col: " << itemRow << "," << col << " = " << itemText;

//         ui->songTable->blockSignals(true);  // block signals, so changes are not recursive

//         // Step 1: count up how many of each number we have in the # column
//         int numRows = ui->songTable->rowCount();
//         int *count = new int[numRows]();   // initialized to zero
//         int *renumberTo = new int[numRows]();
//         int minNum = 1E6;
//         int maxNum = -1000;
// //        int numNums = 0;
//         for (int i = 0; i < numRows; i++) {
//             QTableWidgetItem *p = ui->songTable->item(i, kNumberCol); // get pointer to item
//             QString s = p->text();
//             if (s != "") {
//                 // if there's a number in this cell...
//                 int i1 = (s.toInt(&ok));
//                 count[i1]++;  // bump the count
//                 minNum = qMin(minNum, i1);
//                 maxNum = qMax(maxNum, i1);
// //                numNums++;
//             }
//         }

//         minNum = 1;  // this is true always, if everything is in order (which it should always be)

//         // Step 2: figure out which numbers are doubled (if any) and which are missing (if any)
//         int doubled = -1;
//         int missing = -1;
//         for (int i = minNum; i <= maxNum; i++) {
//             switch (count[i]) {
//                 case 0: missing = i; break;
//                 case 1: break;
//                 case 2: doubled = i; break;
//                 default: break;
//             }
//         }

// //        qDebug() << "Doubled: " << doubled << ", missing: " << missing;

//         // Step 3: figure out what to renumber each of the existing numbers to, to
//         //   have no dups and no gaps
// //        qDebug() << "minNum,maxNum = " << minNum << maxNum;
//         int j = 1;
//         for (int i = minNum; i <= maxNum; i++) {
//             switch (count[i]) {
//                 case 0: break;
//                 case 1: renumberTo[i] = j++; break;
//                 case 2: if ((doubled < missing) || (doubled == -1) || (missing == -1)) {
//                             renumberTo[i] = ++j; j++; break; // moving UP the list or new item
//                         } else {
//                             renumberTo[i] = j; j += 2; break; // moving DOWN the list
//                         }
//                 default: break;
//             }

// //            qDebug() << "  " << i << "=" << count[i] << " -> " << renumberTo[i];
//         }

//         // Step 4: renumber all the existing #'s as per the renumberTo table above
//         for (int i = 0; i < ui->songTable->rowCount(); i++) {
//             // iterate over all rows
//             QTableWidgetItem *p = ui->songTable->item(i, kNumberCol); // get pointer to item
//             QString s = p->text();
//             if (s != "") {
//                 // if there's a number in this cell...
//                 int thisInteger = (s.toInt(&ok));
//                 if (itemRow != i) {
//                     // NOT the one the user changed, so it can't be a dup
//                     p->setText(QString::number(renumberTo[thisInteger]));
//                 } else {
//                     if (count[thisInteger] == 2) {
//                         // dup, so what the user typed in was correct, do nothing
//                     } else {
//                         // NOT a dup, so what the user typed in might NOT be correct, so renumber it
// //                        qDebug() << "setText to:" << renumberTo[thisInteger];
//                         p->setText(QString::number(renumberTo[thisInteger]));
//                     }
//                 }
//             }
//         }
//         ui->songTable->blockSignals(false);

//         delete[] count;
//         delete[] renumberTo;
//     } else {
//         item->setText(""); // if it's not a number, set to null string
//     }

//     ui->songTable->setSortingEnabled(true); // re-enable sorting

//     // mark playlist modified
//     markPlaylistModified(true); // turn ON the * in the status bar, because some # entry changed

//     // TODO: remove any non-numeric chars from the string before updating
}

// ----------------------------------------------------------------
void MainWindow::on_actionSave_Playlist_2_triggered()
{
    // This is Playlist > Save Playlist
    savePlaylistAgain();
}

// NOTE: Save Playlist As is handled by:

void MainWindow::on_actionPrint_Playlist_triggered()
{
    // // Playlist > Print Playlist...
    // QPrinter printer;
    // QPrintDialog printDialog(&printer, this);
    // printDialog.setWindowTitle("Print Playlist");

    // if (printDialog.exec() == QDialog::Rejected) {
    //     return;
    // }

    // // --------
    // QList<PlaylistExportRecord> exports;

    // // Iterate over the songTable to get all the info about the playlist
    // for (int i=0; i<ui->songTable->rowCount(); i++) {
    //     QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
    //     QString playlistIndex = theItem->text();
    //     QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
    //     //            QString songTitle = getTitleColTitle(ui->songTable, i);
    //     QString pitch = ui->songTable->item(i,kPitchCol)->text();
    //     QString tempo = ui->songTable->item(i,kTempoCol)->text();

    //     if (playlistIndex != "") {
    //         // item HAS an index (that is, it is on the list, and has a place in the ordering)
    //         // TODO: reconcile int here with double elsewhere on insertion
    //         PlaylistExportRecord rec;
    //         rec.index = playlistIndex.toInt();
    //         //            rec.title = songTitle;
    //         rec.title = pathToMP3;  // NOTE: this is an absolute path that does not survive moving musicDir
    //         rec.pitch = pitch;
    //         rec.tempo = tempo;
    //         exports.append(rec);
    //     }
    // }

    // std::sort(exports.begin(), exports.end(), comparePlaylistExportRecord);  // they are now IN INDEX ORDER

    // // GET SHORT NAME FOR PLAYLIST ---------
    // static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
    // QRegularExpressionMatch match = regex1.match(lastSavedPlaylist);
    // QString shortPlaylistName("ERROR 7");

    // if (match.hasMatch())
    // {
    //     shortPlaylistName = match.captured(1);
    // }

    // QString toBePrinted = "<h2>PLAYLIST: " + shortPlaylistName + "</h2>\n"; // TITLE AT TOP OF PAGE

    // QTextDocument doc;
    // QSizeF paperSize;
    // paperSize.setWidth(printer.width());
    // paperSize.setHeight(printer.height());
    // doc.setPageSize(paperSize);

    // patterColorString = prefsManager.GetpatterColorString();
    // singingColorString = prefsManager.GetsingingColorString();
    // calledColorString = prefsManager.GetcalledColorString();
    // extrasColorString = prefsManager.GetextrasColorString();

    // char buf[256];
    // foreach (const PlaylistExportRecord &rec, exports)
    // {
    //     QString baseName = rec.title;
    //     baseName.replace(QRegularExpression("^" + musicRootPath),"");  // delete musicRootPath at beginning of string

    //     snprintf(buf, sizeof(buf), "%02d: %s\n", rec.index, baseName.toLatin1().data());

    //     QStringList parts = baseName.split("/");
    //     QString folderTypename = parts[1]; // /patter/foo.mp3 --> "patter"

    //     // qDebug() << "on_actionPrint_playlist_triggered(): folderTypename = " << folderTypename;

    //     QString colorString("black");
    //     if (songTypeNamesForPatter.contains(folderTypename)) {
    //         colorString = patterColorString;
    //     } else if (songTypeNamesForSinging.contains(folderTypename)) {
    //         colorString = singingColorString;
    //     } else if (songTypeNamesForCalled.contains(folderTypename)) {
    //         colorString = calledColorString;
    //     } else if (songTypeNamesForExtras.contains(folderTypename)) {
    //         colorString = extrasColorString;
    //     }
    //     toBePrinted += "<span style=\"color:" + colorString + "\">" + QString(buf) + "</span><BR>\n";
    // }

    // //        qDebug() << "PRINT: " << toBePrinted;

    // doc.setHtml(toBePrinted);
    // doc.print(&printer);
}

// ====================================
// ====================================
// load playlist to palette slot

// ============================================================================================================
void MainWindow::setTitleField(QTableWidget *whichTable, int whichRow, QString relativePath,
                               bool isPlaylist, QString PlaylistFileName, QString theRealPath) {

    // relativePath is relative to the MusicRootPath, and it might be fake (because it was reversed, like Foo Bar - RIV123.mp3,
    //     we reversed it for presentation
    // theRealPath is the actual relative path, used to determine whether the file exists or not

    // qDebug() << "setTitleField" << isPlaylist << whichRow << relativePath << PlaylistFileName << theRealPath;
    static QRegularExpression dotMusicSuffix("\\.(mp3|m4a|wav|flac)$", QRegularExpression::CaseInsensitiveOption); // match with music extensions
    QString shortTitle = relativePath.split('/').last().replace(dotMusicSuffix, "");

    // example:
    // list1[0] = "/singing/BS 2641H - I'm Beginning To See The Light.mp3"
    // type = "singing"
    QString cType = relativePath.split('/').at(1).toLower();
    // qDebug() << "cType:" << cType;

    QColor textCol; // = QColor::fromRgbF(0.0/255.0, 0.0/255.0, 0.0/255.0);  // defaults to Black
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
        if (darkmode) { // BUG: I don't think this is set yet by this point
            textCol = QColor("#808080");  // if not a recognized type, color it white-ish (dark mode!)
        } else {
            textCol = QColor("#808080");  // if not a recognized type, color it black (light mode!)
        }
    }

    // darkPaletteSongTitleLabel *title = new darkPaletteSongTitleLabel(this, (MyTableWidget *)whichTable);
    darkPaletteSongTitleLabel *title = new darkPaletteSongTitleLabel(this);
    // darkPaletteSongTitleLabel *title = new darkPaletteSongTitleLabel(slotNumber);
    title->setTextFormat(Qt::RichText);
    // title->textColor = "red";  // remember the text color, so we can restore it when deselected

    // figure out colors
    SongSetting settings;
    // QString origPath = musicRootPath + relativePath;
    QString origPath = musicRootPath + theRealPath;
    songSettings.loadSettings(origPath,
                              settings);
    if (settings.isSetTags())
        songSettings.addTags(settings.getTags());
    // qDebug() << "origPath:" << origPath << settings;

    // format the title string -----
    QString titlePlusTags(FormatTitlePlusTags(shortTitle, settings.isSetTags(), settings.getTags(), textCol.name()));

    // qDebug() << "setTitleField:" << shortTitle << titlePlusTags;
    title->setText(titlePlusTags);

    QTableWidgetItem *blankItem = new QTableWidgetItem;
    whichTable->setItem(whichRow, 1, blankItem); // null item so I can get the row() later

    whichTable->setCellWidget(whichRow, 1, title);

    // qDebug() << "setTitleField: " << relativePath;

    QString absPath = musicRootPath + relativePath;

    if (relativePath.startsWith("/Users/")) {
        absPath = relativePath; // SPECIAL HANDLING for items that point into the local Apple Music library
    }

    QString realPath = absPath;  // the usual case
    if (theRealPath != "") {
        // real path specified, so use it to point at the real file
        realPath = musicRootPath + theRealPath;
    }

    // DDD(theRealPath)

    // QFileInfo fi(absPath);
    QFileInfo fi(realPath);
    if (isPlaylist && !fi.exists()) {
        QFont f = title->font();
        f.setStrikeOut(true);
        title->setFont(f); // strikethrough the text until it's fixed

        // set background color of the title portion to RED
        QString badText = title->text(); // e.g. <span style="color: #a364dc;">ESP 450 - Ricochet2</span>

        int colonLoc = badText.indexOf(":");
        badText.remove(colonLoc, 10); // <span style="color">ESP 450 - Ricochet2</span>
        badText.insert(colonLoc, ":black;background-color:red;"); // now: <span style="color:black;background-color:red;">ESP 450 - Ricochet2</span>

        title->setText(badText);

        // title->setBackground(QBrush(Qt::red));  // does not exist, tell the user!  // FIX FIX FIX NEED TO ADD THIS TO OUR WIDGET *******
        // TODO: provide context menu to get dialog with reasons why
        QString shortPlaylistName = PlaylistFileName.split('/').last().replace(".csv","");
        title->setToolTip(QString("File '%1'\nin playlist '%2' does not exist.\n\nFIX: RIGHT CLICK in the playlist header, and select 'Edit %2 in text editor' to edit manually.\nWhen done editing, save it, and then reload the playlist.").arg(absPath, shortPlaylistName));
    }

    // context menu for the TITLE QLABEL palette slot item that is in a PLAYLIST -----
    title->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(title, &QLabel::customContextMenuRequested,
            this, [this, whichTable, isPlaylist](QPoint q) {
                Q_UNUSED(q)
                // qDebug() << "PLAYLIST *QLABEL* CONTEXT MENU!";

                int rowCount = whichTable->selectionModel()->selectedRows().count();
                if (rowCount < 1) {
                    return;  // if mouse clicked and nothing was selected (this should be impossible)
                }
                QString plural;
                if (rowCount == 1) {
                    plural = "item";
                } else {
                    plural = QString::number(rowCount) + " items";
                }

                // qDebug() << "QLABEL CONTEXT MENU: " << rowCount;

                QMenu *plMenu = new QMenu();
                plMenu->setProperty("theme", currentThemeString);

                // Move up/down/top/bottom in playlist
                if (isPlaylist) {
                    plMenu->addAction(QString("Move " + plural + " to TOP of playlist"),    [this]() { this->PlaylistItemsToTop();    } );
                    plMenu->addAction(QString("Move " + plural + " UP in playlist"),        [this]() { this->PlaylistItemsMoveUp();   } );
                    plMenu->addAction(QString("Move " + plural + " DOWN in playlist"),      [this]() { this->PlaylistItemsMoveDown(); } );
                    plMenu->addAction(QString("Move " + plural + " to BOTTOM of playlist"), [this]() { this->PlaylistItemsToBottom(); } );
                    plMenu->addSeparator();
                    plMenu->addAction(QString("Remove " + plural + " from playlist"),       [this]() { this->PlaylistItemsRemove(); } );
                }

                if (rowCount == 1) {
                    // ONLY A SINGLE ROW SELECTED

                    // Reveal Audio File and Cuesheet in Finder
                    // First, let's figure out which row was clicked.  It's the row that is currently selected.
                    //   NOTE: This is a kludge, but I don't know how better to get the row number of a double-clicked
                    //   cellWidget inside a QTableWidget.

                    plMenu->addSeparator();

                    QItemSelectionModel *selectionModel = whichTable->selectionModel();
                    QModelIndexList selected = selectionModel->selectedRows();
                    int theRow = -1;

                    if (selected.count() == 1) {
                        // exactly 1 row was selected (good)
                        QModelIndex index = selected.at(0);
                        theRow = index.row();
                        // qDebug() << "the row was: " << theRow;
                    }

                    QString fullPath = whichTable->item(theRow,4)->text();
                    QString enclosingFolderName = QFileInfo(fullPath).absolutePath();

                    // qDebug() << "customContextMenu: " << theRow << fullPath << enclosingFolderName;

                    QFileInfo fi(fullPath);
                    QString menuString = "Reveal Audio File In Finder";
                    QString thingToOpen = fullPath;

                    if (!fi.exists()) {
                        menuString = "Reveal Enclosing Folder In Finder";
                        thingToOpen = enclosingFolderName;
                    }

                    plMenu->addAction(QString(menuString),
                                      [thingToOpen]() {
            // opens either the folder and highlights the file (if file exists), OR
            // opens the folder where the file was SUPPOSED to exist.
    #if defined(Q_OS_MAC)
                                          QStringList args;
                                          args << "-e";
                                          args << "tell application \"Finder\"";
                                          args << "-e";
                                          args << "activate";
                                          args << "-e";
                                          args << "select POSIX file \"" + thingToOpen + "\"";
                                          args << "-e";
                                          args << "end tell";

                                          //    QProcess::startDetached("osascript", args);

                                          // same as startDetached, but suppresses output from osascript to console
                                          //   as per: https://www.qt.io/blog/2017/08/25/a-new-qprocessstartdetached
                                          QProcess process;
                                          process.setProgram("osascript");
                                          process.setArguments(args);
                                          process.setStandardOutputFile(QProcess::nullDevice());
                                          process.setStandardErrorFile(QProcess::nullDevice());
                                          qint64 pid;
                                          process.startDetached(&pid);
    #endif

                                      });

                    if (isPlaylist) {
                        // if the current song has a cuesheet, offer to show it to the user -----
                        // QString fullMP3Path = this->ui->playlist1Table->item(this->ui->playlist1Table->itemAt(q)->row(), 4)->text();
                        QString fullMP3Path = fullPath;
                        QString cuesheetPath;

                        SongSetting settings1;
                        if (songSettings.loadSettings(fullMP3Path, settings1)) {
                            // qDebug() << "here are the settings: " << settings1;
                            cuesheetPath = settings1.getCuesheetName();
                        } else {
                            qDebug() << "Tried to revealAttachedLyricsFile, but could not get settings for: " << fullMP3Path;
                        }

                        // qDebug() << "cuesheetPath: " << cuesheetPath;

                        // qDebug() << "convertCuesheetPathNameToCurrentRoot BEFORE:" << cuesheetPath;
                        cuesheetPath = convertCuesheetPathNameToCurrentRoot(cuesheetPath);
                        // qDebug() << "convertCuesheetPathNameToCurrentRoot AFTER:" << cuesheetPath;

                        if (cuesheetPath != "") {
                            plMenu->addAction(QString("Reveal Current Cuesheet in Finder"),
                                              [this, cuesheetPath]() {
                                                  showInFinderOrExplorer(cuesheetPath);
                                              }
                                              );
                        }
                    }
                } // if there's just one row in the selection

                plMenu->popup(QCursor::pos());
                plMenu->exec();
                delete plMenu; // done with it
            }
            );
}

// ============================================================================================================
// returns first song error, and also updates the songCount as it goes (2 return values)
QString MainWindow::loadPlaylistFromFileToPaletteSlot(QString PlaylistFileName, int slotNumber, int &songCount) {

    // qDebug() << "===== loadPlaylistFromFileToPaletteSlot: " << PlaylistFileName << slotNumber << songCount;

    // NOTE: Slot number is 0 to 2
    // --------
    // PlaylistFileName could be either a playlist relPath
    //  e.g. "/Users/mpogue/Library/CloudStorage/Box-Box/__squareDanceMusic_Box/playlists/Jokers/2018/Jokers_2018.02.14.csv"
    // OR it could be a FAKE Track filter specifier path
    //  e.g. "/Users/mpogue/Library/CloudStorage/Box-Box/__squareDanceMusic_Box/Tracks/vocals.csv"
    // OR it could be a FAKE Apple Music playlist specifier
    //  e.g. "/Users/mpogue/Library/CloudStorage/Box-Box/__squareDanceMusic_Box/Apple Music/Mike's playlist.csv" // FAKE APPLE MUSIC

    if (slotModified[slotNumber]) {
        // if the current resident of the slot has been modified, and the playlistSlotWatcherTimer
        //   has gone off yet, and so that playlist hasn't been saved yet, save it now, before we
        //   overwrite the slot.
        saveSlotNow(slotNumber);
    }

    QString relativePath = PlaylistFileName;
    relativePath.replace(musicRootPath, "");

//    qDebug() << "loadPlaylistFromFileToPaletteSlot: " << PlaylistFileName << relativePath;

    if (relativePath.startsWith("/Tracks") || relativePath.startsWith("Tracks")) { // FIX: might want to collapse these two later...
        relativePath.replace("/Tracks/", "").replace("Tracks/", "").replace(".csv", "");

//        qDebug() << "TRACK SPECIFIER:" << relativePath;

        // -------------
        // now that we have a TRACK FILTER SPECIFIER, let's load THAT into a palette slot
        QTableWidget *theTableWidget;
        QLabel *theLabel;

        switch (slotNumber) {
            case 0: theTableWidget = ui->playlist1Table; theLabel = ui->playlist1Label; prefsManager.SetlastPlaylistLoaded("tracks/" + relativePath); break;
            case 1: theTableWidget = ui->playlist2Table; theLabel = ui->playlist2Label; prefsManager.SetlastPlaylistLoaded2("tracks/" + relativePath); break;
            case 2: theTableWidget = ui->playlist3Table; theLabel = ui->playlist3Label; prefsManager.SetlastPlaylistLoaded3("tracks/" + relativePath); break;
        }

        theTableWidget->hide();
        theTableWidget->setSortingEnabled(false);

        theTableWidget->setRowCount(0); // delete all the rows in the slot

        // The only way to get here is to RIGHT-CLICK on a Tracks/filterName in the TreeWidget.
        //  In that case, the darkSongTable has already been loaded with the filtered rows.
        //  We just need to transfer them up to the playlist.

        static QRegularExpression title_tags_remover("(\\&nbsp\\;)*\\<\\/?span( .*?)?>");
        static QRegularExpression spanPrefixRemover("<span style=\"color:.*\">(.*)</span>", QRegularExpression::InvertedGreedinessOption);

        int songCount = 0;
        for (int i = 0; i < ui->darkSongTable->rowCount(); i++) {
            if (true || !ui->darkSongTable->isRowHidden(i)) {

                // make sure this song has the "type" that we're looking for...
                QString pathToMP3 = ui->darkSongTable->item(i,kPathCol)->data(Qt::UserRole).toString();
                QString p1 = pathToMP3;
                p1 = p1.replace(musicRootPath + "/", "");

                QString typeOfFile = p1.split("/").first();
                //                qDebug() << "TRACK: " << typeOfFile << pathToMP3;

                if (typeOfFile != relativePath) {
                    continue; // skip this one, if the type is not what we want
                }
                //                else {
                //                    qDebug() << "  ** GOOD";
                //                }

                // type IS what we want, so add it to the playlist slot -----
                QString label = ui->darkSongTable->item(i, kLabelCol)->text();
                QString shortTitle = dynamic_cast<QLabel*>(ui->darkSongTable->cellWidget(i, kTitleCol))->text();
                QString coloredTitle = shortTitle; // title with coloring AND tags with coloring

                shortTitle.replace(spanPrefixRemover, "\\1"); // remove <span style="color:#000000"> and </span> title string coloring

                int where = shortTitle.indexOf(title_tags_remover);
                if (where >= 0) {
                    shortTitle.truncate(where);
                }

                shortTitle.replace("&quot;","\"").replace("&amp;","&").replace("&gt;",">").replace("&lt;","<");  // if title contains HTML encoded chars, put originals back

                if (shortTitle.contains("span")) { // DEBUG DEBUG
                    // qDebug() << "FOUND SPAN BEFORE: " << dynamic_cast<QLabel*>(ui->darkSongTable->cellWidget(i, kTitleCol))->text();
                    // qDebug() << "FOUND SPAN AFTER: " << shortTitle;
                }

                QString pitch = ui->darkSongTable->item(i, kPitchCol)->text();
                QString tempo = ui->darkSongTable->item(i, kTempoCol)->text();

                songCount++;
//                qDebug() << "ROW: " << songCount << label << " - " << shortTitle << pitch << tempo << pathToMP3;

                // make a new row, if needed
                if (songCount > theTableWidget->rowCount()) {
                    theTableWidget->insertRow(theTableWidget->rowCount());
                }

                // # column
                QTableWidgetItem *num = new TableNumberItem(QString::number(songCount)); // use TableNumberItem so that it sorts numerically
                theTableWidget->setItem(songCount-1, 0, num);

                // TITLE column ================================
                // coloredTitle.replace(shortTitle, label + " - " + shortTitle); // add the label back in here

                // DDD(QString("/") + p1)
                QString s12 = QString("/") + p1;
                QString categoryName = filepath2SongCategoryName(s12);
                // DDD(categoryName)
                // QString fakePath = "/" + p1;
                QString fakePath = "/" + label + " - " + shortTitle;
                if (!songTypeNamesForCalled.contains(categoryName) &&
                    !songTypeNamesForExtras.contains(categoryName) &&
                    !songTypeNamesForPatter.contains(categoryName) &&
                    !songTypeNamesForSinging.contains(categoryName)
                    ) {
                    fakePath = "/xtras" + fakePath;
                }
                // DDD(fakePath)
                // DDD(label)
                // DDD(shortTitle)
                // setTitleField(theTableWidget, songCount-1, "/" + p1, false, PlaylistFileName); // whichTable, whichRow, relativePath or pre-colored title, bool isPlaylist, PlaylistFilename (for errors and for filters it's colored)
                setTitleField(theTableWidget, songCount-1, fakePath, false, PlaylistFileName); // whichTable, whichRow, relativePath or pre-colored title, bool isPlaylist, PlaylistFilename (for errors and for filters it's colored)

                // PITCH column
                QTableWidgetItem *pit = new QTableWidgetItem(pitch);
                theTableWidget->setItem(songCount-1, 2, pit);

                // TEMPO column
                QTableWidgetItem *tem = new QTableWidgetItem(tempo);
                theTableWidget->setItem(songCount-1, 3, tem);

                // PATH column
                QTableWidgetItem *fullPath = new QTableWidgetItem(pathToMP3); // full ABSOLUTE path
                theTableWidget->setItem(songCount-1, 4, fullPath);

                // LOADED column
                QTableWidgetItem *loaded = new QTableWidgetItem("");
                theTableWidget->setItem(songCount-1, 5, loaded);
            }
        }

        theTableWidget->resizeColumnToContents(0);
//        theTableWidget->resizeColumnToContents(2);
//        theTableWidget->resizeColumnToContents(3);

        theTableWidget->setSortingEnabled(true);
        theTableWidget->show();

        // redo the label, but with an indicator that these are Tracks, not a Playlist
        QString playlistShortName = relativePath;
        theLabel->setText(QString("<img src=\":/graphics/icons8-musical-note-60.png\" width=\"15\" height=\"15\">") + playlistShortName);

        relPathInSlot[slotNumber] = "/tracks/" + relativePath;
        // qDebug() << "TRACKS: Setting relPath[" << slotNumber << "] to: " << relPathInSlot[slotNumber];

        slotModified[slotNumber] = false;

        return ""; // no errors
    }

    // ===============================================================================
    if (relativePath.startsWith("/Apple Music/")) {
        // WE KNOW IT'S AN APPLE MUSIC PLAYLIST
        //   e.g. /Apple Music/

        // qDebug() << "WE KNOW IT'S AN APPLE MUSIC PLAYLIST!";

        QString relPath = relativePath; // e.g. /Apple Music/foo.csv
        // relPath = relPath.replace("/Apple Music/", "").replace(".csv", ""); // relPath is e.g. "Second playlist"
        relPath.replace("/Apple Music/", "").replace(".csv", ""); // relPath is e.g. "Second playlist"

        // qDebug() << "APPLE MUSIC PLAYLIST relPathInSlot: " << relPathInSlot[0] << relPathInSlot[1] << relPathInSlot[2];
        // qDebug() << "APPLE MUSIC PLAYLIST relativePath: " << relativePath;
        // qDebug() << "APPLE MUSIC PLAYLIST relPath: " << relPath;

        QString applePlaylistName = relPath;

        // ALLOW ONLY ONE COPY OF A PLAYLIST LOADED IN THE SLOT PALETTE AT A TIME ------
        //  previous one will be saved, and same one may be loaded into a subsequent slot
        //  so, e.g. starting with no slots filled, double-click loads into slot 1. Double-click same playlist again, MOVES it to slot 2.
        if (relPath == relPathInSlot[0]) {
            saveSlotNow(0);  // save it, if it has something in there
            clearSlot(0);    // clear the table and the label
            prefsManager.SetlastPlaylistLoaded("");
        } else if (relPath == relPathInSlot[1]) {
            saveSlotNow(1);  // save it, if it has something in there
            clearSlot(1);    // clear the table and the label
            prefsManager.SetlastPlaylistLoaded2("");
        } else if (relPath == relPathInSlot[2]) {
            saveSlotNow(2);  // save it, if it has something in there
            clearSlot(2);    // clear the table and the label
            prefsManager.SetlastPlaylistLoaded3("");
        }

        QTableWidget *theTableWidget;
        QLabel *theLabel;

        // DDD(QString("SetlastPlaylistLoaded to 'Apple Music/") + relPath + "'")

        switch (slotNumber) {
            case 0: theTableWidget = ui->playlist1Table; theLabel = ui->playlist1Label; prefsManager.SetlastPlaylistLoaded("Apple Music/" + relPath); break;
            case 1: theTableWidget = ui->playlist2Table; theLabel = ui->playlist2Label; prefsManager.SetlastPlaylistLoaded2("Apple Music/" + relPath); break;
            case 2: theTableWidget = ui->playlist3Table; theLabel = ui->playlist3Label; prefsManager.SetlastPlaylistLoaded3("Apple Music/" + relPath); break;
        }

        //        theTableWidget->clear();  // this just clears the items/text, it doesn't delete the rows
        theTableWidget->setRowCount(0); // delete all the rows

        linesInCurrentPlaylist = 0;
        int songCount = 0;
        // DDD(applePlaylistName)
        foreach (const QStringList& sl, allAppleMusicPlaylists) {
            // look at each one
            // DDD(sl[0])
            // DDD(sl[2])
            bool isPlayableAppleMusicItem = sl[2].endsWith(".wav", Qt::CaseInsensitive) ||
                              sl[2].endsWith(".mp3", Qt::CaseInsensitive) ||
                              sl[2].endsWith(".m4a", Qt::CaseInsensitive);
            // DDD(isPlayableAppleMusicItem)
            if (sl[0] == applePlaylistName && isPlayableAppleMusicItem)  {
                // YES! it belongs to the playlist we want
                songCount++;

                // make a new row, if needed
                if (songCount > theTableWidget->rowCount()) {
                    theTableWidget->insertRow(theTableWidget->rowCount());
                }

                // # column
                QTableWidgetItem *num = new TableNumberItem(QString::number(songCount)); // use TableNumberItem so that it sorts numerically
                theTableWidget->setItem(songCount-1, 0, num);

                // TITLE column
                // QString absPath = musicRootPath + sl[0];
                // qDebug() << "loading playlist:" << list1[0] << PlaylistFileName;
                // setTitleField(theTableWidget, songCount-1, list1[0], true, PlaylistFileName); // whichTable, whichRow, fullPath, bool isPlaylist, PlaylistFilename (for errors)

                // QTableWidgetItem *tit = new QTableWidgetItem(sl[1]); // defaults to no pitch change
                // theTableWidget->setItem(songCount-1, 1, tit); // title

                setTitleField(theTableWidget, songCount-1, "/xtras/" + sl[1], false, sl[1]); // whichTable, whichRow, relativePath or pre-colored title, bool isPlaylist, PlaylistFilename (for errors and for filters it's colored)

                // PITCH column
                QTableWidgetItem *pit = new QTableWidgetItem("0"); // defaults to no pitch change
                theTableWidget->setItem(songCount-1, 2, pit);

                // TEMPO column
                QTableWidgetItem *tem = new QTableWidgetItem("0"); // defaults to tempo 0 = "unknown".  Set to midpoint on load.
                theTableWidget->setItem(songCount-1, 3, tem);

                // PATH column
                QTableWidgetItem *fullPath = new QTableWidgetItem(sl[2]); // full ABSOLUTE path
                theTableWidget->setItem(songCount-1, 4, fullPath);

                // if (pathsOfCalledSongs.contains(absPath)) {
                //     ((darkPaletteSongTitleLabel *)(theTableWidget->cellWidget(songCount-1, 1)))->setSongUsed(true);
                // }

                // LOADED column
                QTableWidgetItem *loaded = new QTableWidgetItem("");  // NOT LOADED
                theTableWidget->setItem(songCount-1, 5, loaded);
            }
        }

        theTableWidget->resizeColumnToContents(0);

        QString theRelativePath = relativePath.replace("/Apple Music/","").replace(".csv","");
        // theRelativePath = "Untitled playlist";
        theLabel->setText(QString("<img src=\":/graphics/icons8-apple-48.png\" width=\"12\" height=\"12\">") + theRelativePath); // TODO: APPLE ICON HERE

        relPathInSlot[slotNumber] = PlaylistFileName;
        relPathInSlot[slotNumber].replace(musicRootPath, "").replace(".csv",""); // e.g. /Apple Music/Second Playlist
        slotModified[slotNumber] = false; // this is a filter so nothing to save here

        // qDebug() << "APPLE MUSIC: Setting relPath[" << slotNumber << "] to: " << relPathInSlot[slotNumber];

        // relPathInSlot[slotNumber] = ""; // this is an "Untitled playlist"
        // slotModified[slotNumber] = true; // but we've stuck something in it, so must be saved

        // addFilenameToRecentPlaylist(PlaylistFileName); // remember it in the Recents menu, full absolute pathname

        // qDebug() << "setting focus item to item 0 in slot" << slotNumber;
        theTableWidget->setCurrentItem(theTableWidget->item(0,0)); // select first item
        theTableWidget->setFocus();

        // qDebug() << "Exiting the handling of Apple Music playlist -------";
        return ""; // no errors
    }

    // ===============================================================================
    // ELSE:
    // WE KNOW IT'S A PLAYLIST ----------------------

    QString relPath = relativePath; // this is the name of a playlist
    relPath.replace("/playlists/", "").replace(".csv", ""); // relPath is e.g. "5thWed/5thWed_2021.12.29" same as relPathInSlot now

//    qDebug() << "PLAYLIST relPathInSlot: " << relPathInSlot[0] << relPathInSlot[1] << relPathInSlot[2];
//    qDebug() << "PLAYLIST relativePath: " << relativePath;
//    qDebug() << "PLAYLIST relPath: " << relPath;

    // ALLOW ONLY ONE COPY OF A PLAYLIST LOADED IN THE SLOT PALETTE AT A TIME ------
    //  previous one will be saved, and same one may be loaded into a subsequent slot
    //  so, e.g. starting with no slots filled, double-click loads into slot 1. Double-click same playlist again, MOVES it to slot 2.
    if (relPath == relPathInSlot[0]) {
        saveSlotNow(0);  // save it, if it has something in there
        clearSlot(0);    // clear the table and the label
        prefsManager.SetlastPlaylistLoaded("");
    } else if (relPath == relPathInSlot[1]) {
        saveSlotNow(1);  // save it, if it has something in there
        clearSlot(1);    // clear the table and the label
        prefsManager.SetlastPlaylistLoaded2("");
    } else if (relPath == relPathInSlot[2]) {
        saveSlotNow(2);  // save it, if it has something in there
        clearSlot(2);    // clear the table and the label
        prefsManager.SetlastPlaylistLoaded3("");
    }

    QString firstBadSongLine = "";
    QFile inputFile(PlaylistFileName);
    if (inputFile.open(QIODevice::ReadOnly)) { // defaults to Text mode

        QTableWidget *theTableWidget;
        QLabel *theLabel;

        switch (slotNumber) {
            case 0: theTableWidget = ui->playlist1Table; theLabel = ui->playlist1Label; prefsManager.SetlastPlaylistLoaded("playlists/" + relPath); break;
            case 1: theTableWidget = ui->playlist2Table; theLabel = ui->playlist2Label; prefsManager.SetlastPlaylistLoaded2("playlists/" + relPath); break;
            case 2: theTableWidget = ui->playlist3Table; theLabel = ui->playlist3Label; prefsManager.SetlastPlaylistLoaded3("playlists/" + relPath); break;
        }

//        theTableWidget->clear();  // this just clears the items/text, it doesn't delete the rows
        theTableWidget->setRowCount(0); // delete all the rows

        linesInCurrentPlaylist = 0;

        QTextStream in(&inputFile);

        if (PlaylistFileName.endsWith(".csv", Qt::CaseInsensitive)) {
            // CSV FILE =================================

            QString header = in.readLine();  // read header (and throw away for now), should be "abspath,pitch,tempo"
            Q_UNUSED(header) // turn off the warning (actually need the readLine to happen);

            // V1 playlists use absolute paths, V2 playlists use relative paths
            // This allows two things:
            //   - moving an entire Music Directory from one place to another, and playlists still work
            //   - sharing an entire Music Directory across platforms, and playlists still work

            songCount = 0;

            while (!in.atEnd()) {
                QString line = in.readLine();

                if (line == "") {
                    // ignore, it's a blank line
                }
                else {
                    QStringList list1 = parseCSV(line);  // This is more robust than split(). Handles commas inside double quotes, double double quotes, etc.

                    if (list1.length() != 3) {
                        continue;  // skip lines that don't have exactly 3 fields
                    }

                    songCount++;  // it's a real song path

//                    EXAMPLE PLAYLIST CSV FILE:
//                    relpath,pitch,tempo
//                    "/patter/RR 1323 - Diggy.mp3",0,124

                    // make a new row, if needed
                    if (songCount > theTableWidget->rowCount()) {
                        theTableWidget->insertRow(theTableWidget->rowCount());
                    }

                    // # column
                    QTableWidgetItem *num = new TableNumberItem(QString::number(songCount)); // use TableNumberItem so that it sorts numerically
                    theTableWidget->setItem(songCount-1, 0, num);

                    // TITLE column
                    QString absPath = musicRootPath + list1[0];
                    if (list1[0].startsWith("/Users/")) {
                        absPath = list1[0]; // override for absolute pathnames
                    }
                    // DDD(list1[0])

                    QFileInfo fi(list1[0]);
                    QString theBaseName = fi.completeBaseName();
                    QString theSuffix = fi.suffix();

                    QString theLabel, theLabelNum, theLabelNumExtra, theTitle, theShortTitle;
                    breakFilenameIntoParts(theBaseName, theLabel, theLabelNum, theLabelNumExtra, theTitle, theShortTitle);

                    // DDD(theBaseName)
                    // DDD(theLabel)
                    // DDD(theLabelNum)
                    // DDD(theLabelNumExtra)
                    // DDD(theTitle)
                    // DDD(theShortTitle)

                    // // check to see if it needs to be colored like "/xtras"
                    QString categoryName = filepath2SongCategoryName(fi.absolutePath());
                    // DDD(categoryName)
                    if (!songTypeNamesForCalled.contains(categoryName) &&
                        !songTypeNamesForExtras.contains(categoryName) &&
                        !songTypeNamesForPatter.contains(categoryName) &&
                        !songTypeNamesForSinging.contains(categoryName)
                        ) {
                        categoryName = "xtras";
                    }
                    // DDD(categoryName)
                    // DDD(fi.absolutePath())

                    QString theFakePath = "/" + categoryName + "/" + theLabel + " " + theLabelNum + " - " + theTitle + "." + theSuffix;
                    // DDD(theFakePath)
                    // DDD(PlaylistFileName)

                    // qDebug() << "loading playlist:" << list1[0] << PlaylistFileName;
                    // setTitleField(theTableWidget, songCount-1, list1[0], true, PlaylistFileName); // whichTable, whichRow, fullPath, bool isPlaylist, PlaylistFilename (for errors)
                    setTitleField(theTableWidget, songCount-1, theFakePath, true, PlaylistFileName, list1[0]); // list1[0] points at the file, theFakePath might have been reversed, e.g. Foo - RIV123.mp3

                    // PITCH column
                    QTableWidgetItem *pit = new QTableWidgetItem(list1[1]);
                    theTableWidget->setItem(songCount-1, 2, pit);

                    // TEMPO column
                    QTableWidgetItem *tem = new QTableWidgetItem(list1[2]);
                    theTableWidget->setItem(songCount-1, 3, tem);

                    // PATH column
                    QTableWidgetItem *fullPath = new QTableWidgetItem(absPath); // full ABSOLUTE path
                    theTableWidget->setItem(songCount-1, 4, fullPath);

                    // if (pathsOfCalledSongs.contains(absPath)) {
                    //     ((darkPaletteSongTitleLabel *)(theTableWidget->cellWidget(songCount-1, 1)))->setSongUsed(true);
                    // }

                    // LOADED column
                    QTableWidgetItem *loaded = new QTableWidgetItem("");
                    theTableWidget->setItem(songCount-1, 5, loaded);
                }
            } // while
        }

        inputFile.close();

        theTableWidget->resizeColumnToContents(0);
//        theTableWidget->resizeColumnToContents(2);
//        theTableWidget->resizeColumnToContents(3);

        QString theRelativePath = relativePath.replace("/playlists/","").replace(".csv","");
        theLabel->setText(QString("<img src=\":/graphics/icons8-menu-64.png\" width=\"10\" height=\"9\">") + theRelativePath);

        relPathInSlot[slotNumber] = PlaylistFileName;
        relPathInSlot[slotNumber] = relPathInSlot[slotNumber].replace(musicRootPath + "/playlists/", "").replace(".csv","");
        // qDebug() << "PLAYLIST: Setting relPath[" << slotNumber << "] to: " << relPathInSlot[slotNumber];

        addFilenameToRecentPlaylist(PlaylistFileName); // remember it in the Recents menu, full absolute pathname

        // qDebug() << "setting focus item to item 0 in slot" << slotNumber;
        theTableWidget->setCurrentItem(theTableWidget->item(0,0)); // select first item
        theTableWidget->setFocus();
    }
    else {
        // file didn't open...
        return("");
    }

    return(firstBadSongLine);  // return error song (if any)
}

// -------------
// TODO: FACTOR THESE (DUPLICATED CODE), use pointer to a PlaylistTable
void MainWindow::on_playlist1Table_itemDoubleClicked(QTableWidgetItem *item)
{
    PerfTimer t("on_playlist1Table_itemDoubleClicked", __LINE__);

    // playlist format -------
    // col 0: number
    // col 1: short title
    // col 2: pitch
    // col 3: tempo
    // col 4: full path to MP3
    // col 5:

    on_darkStopButton_clicked();  // if we're loading a new MP3 file, stop current playback
    saveCurrentSongSettings();

    t.elapsed(__LINE__);

    int row = item->row();
    QString pathToMP3 = ui->playlist1Table->item(row,4)->text();

    // qDebug() << "Playlist #1 double clicked: " << row << pathToMP3;

    QFileInfo fi(pathToMP3);
    if (!fi.exists()) {
        qDebug() << "ERROR: File does not exist " << pathToMP3;
        return;  // can't load a file that doesn't exist
    }

    QString nextFile = "";
    if (row+1 < ui->playlist1Table->rowCount()) {
        nextFile = ui->playlist1Table->item(row+1,4)->text();
        // qDebug() << "on_playlist1Table_itemDoubleClicked:  nextFile = " << nextFile;
        // } else {
        // qDebug() << "on_playlist1Table_itemDoubleClicked: no nextFile";
    }
    static QRegularExpression dotMusicSuffix("\\.(mp3|m4a|wav|flac)$", QRegularExpression::CaseInsensitiveOption); // match with music extensions
    QString songTitle = pathToMP3.split('/').last().replace(dotMusicSuffix,"");

    // parse the filename into parts, so we can use the shortTitle -----
    QString label;
    QString labelNumber;
    QString labelExtra;
    QString realTitle;
    QString realShortTitle;

    bool success = breakFilenameIntoParts(songTitle, label, labelNumber, labelExtra, realTitle, realShortTitle);
    if (success) {
        songTitle = realShortTitle;
    }

    QString songType = (fi.path().replace(musicRootPath + "/","").split("/"))[0]; // e.g. "hoedown" or "patter"
    QString songLabel = "RIV 123";

    // qDebug() << "***** on_playlist1Table_itemDoubleClicked(): songType = " << songType << songLabel;

    // these must be up here to get the correct values...
    QString pitch  = ui->playlist1Table->item(row, 2)->text();
    QString tempo  = ui->playlist1Table->item(row, 3)->text();
//    QString number = ui->playlist1Table->item(row, 0)->text();

    targetPitch = pitch;  // save this string, and set pitch slider AFTER base BPM has been figured out
    targetTempo = tempo;  // save this string, and set tempo slider AFTER base BPM has been figured out
//    targetNumber = number; // save this, because tempo changes when this is set are playlist modifications, too

//    qDebug() << "on_playlist1Table_itemDoubleClicked: " << songTitle << songType << songLabel << pitch << tempo << pathToMP3;

    t.elapsed(__LINE__);

    loadMP3File(pathToMP3, songTitle, songType, songLabel, nextFile);

    t.elapsed(__LINE__);

    // clear all the "1"s and arrows from palette slots (IF and only IF one of the palette slots is being currently edited
    for (int slot = 0; slot < 3; slot++) {
        MyTableWidget *tables[] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};
        MyTableWidget *table = tables[slot];
        for (int i = 0; i < table->rowCount(); i++) {
            if (table->item(i, 5)->text() == "1") {
                // // clear the arrows out of the other tables
                // QString currentTitleTextWithoutArrow = table->item(i, 1)->text().replace(editingArrowStart, "");
                // table->item(i, 1)->setText(currentTitleTextWithoutArrow);

                QFont currentFont = table->item(i, 1)->font(); // font goes to neutral (not bold or italic, and normal size) for NOT-loaded items
                currentFont.setBold(false);
                currentFont.setItalic(false);
//                    currentFont.setPointSize(currentFont.pointSize() - 2);
                table->item(i, 0)->setFont(currentFont);
                table->item(i, 1)->setFont(currentFont);
                table->item(i, 2)->setFont(currentFont);
                table->item(i, 3)->setFont(currentFont);
            }
            table->item(i, 5)->setText(""); // clear out the old table
        }
    }

    sourceForLoadedSong = ui->playlist1Table; // THIS is where we got the currently loaded song (this is the NEW table)

    for (int i = 0; i < sourceForLoadedSong->rowCount(); i++) {
        if (i == row) {
            // // put arrow on the new one
            // QString currentTitleText = sourceForLoadedSong->item(row, 1)->text().replace(editingArrowStart, "");
            // QString newTitleText = editingArrowStart + currentTitleText;
            // sourceForLoadedSong->item(row, 1)->setText(newTitleText);

            QFont currentFont = sourceForLoadedSong->item(i, 1)->font();  // font goes to BOLD ITALIC BIGGER for loaded items
            currentFont.setBold(true);
            currentFont.setItalic(true);
//            currentFont.setPointSize(currentFont.pointSize() + 2);
            sourceForLoadedSong->item(i, 0)->setFont(currentFont);
            sourceForLoadedSong->item(i, 1)->setFont(currentFont);
            sourceForLoadedSong->item(i, 2)->setFont(currentFont);
            sourceForLoadedSong->item(i, 3)->setFont(currentFont);
        }
        sourceForLoadedSong->item(i, 5)->setText((i == row) ? "1" : ""); // and this is the one being edited (clear out others)
    }

    // these must be down here, to set the correct values...
    int pitchInt = pitch.toInt();

    ui->darkPitchSlider->setValue(pitchInt);

    on_darkPitchSlider_valueChanged(pitchInt); // manually call this, in case the setValue() line doesn't call valueChanged() when the value set is
        //   exactly the same as the previous value.  This will ensure that cBass->setPitch() gets called (right now) on the new stream.
    if (ui->actionAutostart_playback->isChecked()) {
        on_darkPlayButton_clicked();
    }

    ui->playlist1Table->setFocus();
    ui->playlist1Table->resizeColumnToContents(0); // number column needs to be resized, if bolded

    t.elapsed(__LINE__);
}

// -------------
void MainWindow::on_playlist2Table_itemDoubleClicked(QTableWidgetItem *item)
{
    PerfTimer t("on_playlist2Table_itemDoubleClicked", __LINE__);

    on_darkStopButton_clicked();  // if we're loading a new MP3 file, stop current playback
    saveCurrentSongSettings();

    t.elapsed(__LINE__);

    int row = item->row();
    QString pathToMP3 = ui->playlist2Table->item(row,4)->text();

//    qDebug() << "on_playlist2Table_itemDoubleClicked ROW:" << row << pathToMP3;

    QFileInfo fi(pathToMP3);
    if (!fi.exists()) {
        qDebug() << "ERROR: File does not exist " << pathToMP3;
        return;  // can't load a file that doesn't exist
    }

    QString nextFile = "";
    if (row+1 < ui->playlist2Table->rowCount()) {
        nextFile = ui->playlist2Table->item(row+1,4)->text();
        // qDebug() << "on_playlist2Table_itemDoubleClicked:  nextFile = " << nextFile;
        // } else {
        // qDebug() << "on_playlist2Table_itemDoubleClicked: no nextFile";
    }

    static QRegularExpression dotMusicSuffix("\\.(mp3|m4a|wav|flac)$", QRegularExpression::CaseInsensitiveOption); // match with music extensions
    QString songTitle = pathToMP3.split('/').last().replace(dotMusicSuffix,"");

    // parse the filename into parts, so we can use the shortTitle -----
    QString label;
    QString labelNumber;
    QString labelExtra;
    QString realTitle;
    QString realShortTitle;

    bool success = breakFilenameIntoParts(songTitle, label, labelNumber, labelExtra, realTitle, realShortTitle);
    if (success) {
        songTitle = realShortTitle;
    }

    QString songType = (fi.path().replace(musicRootPath + "/","").split("/"))[0]; // e.g. "hoedown" or "patter"
    QString songLabel = "RIV 123";

    // qDebug() << "***** on_playlist2Table_itemDoubleClicked(): songType = " << songType << songLabel;

    // these must be up here to get the correct values...
    QString pitch  = ui->playlist2Table->item(row, 2)->text();
    QString tempo  = ui->playlist2Table->item(row, 3)->text();
//    QString number = ui->playlist2Table->item(row, 0)->text();

    targetPitch = pitch;  // save this string, and set pitch slider AFTER base BPM has been figured out
    targetTempo = tempo;  // save this string, and set tempo slider AFTER base BPM has been figured out
//    targetNumber = number; // save this, because tempo changes when this is set are playlist modifications, too

//    qDebug() << "on_playlist2Table_itemDoubleClicked: " << songTitle << songType << songLabel << pitch << tempo << pathToMP3;

    t.elapsed(__LINE__);

    loadMP3File(pathToMP3, songTitle, songType, songLabel, nextFile);

    t.elapsed(__LINE__);

    // clear all the "1"s and arrows from palette slots, IF and only IF one of the playlist slots is being edited
    for (int slot = 0; slot < 3; slot++) {
        MyTableWidget *tables[] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};
        MyTableWidget *table = tables[slot];
        for (int i = 0; i < table->rowCount(); i++) {
            if (table->item(i, 5)->text() == "1") {
                // // clear the arrows out of the other tables
                // QString currentTitleTextWithoutArrow = table->item(i, 1)->text().replace(editingArrowStart, "");
                // table->item(i, 1)->setText(currentTitleTextWithoutArrow);

                QFont currentFont = table->item(i, 1)->font(); // font goes to neutral (not bold or italic, and normal size) for NOT-loaded items
                currentFont.setBold(false);
                currentFont.setItalic(false);
//                    currentFont.setPointSize(currentFont.pointSize() - 2);
                table->item(i, 0)->setFont(currentFont);
                table->item(i, 1)->setFont(currentFont);
                table->item(i, 2)->setFont(currentFont);
                table->item(i, 3)->setFont(currentFont);
            }
            table->item(i, 5)->setText(""); // clear out the old table
        }
    }

    sourceForLoadedSong = ui->playlist2Table; // THIS is where we got the currently loaded song (this is the NEW table)

    for (int i = 0; i < sourceForLoadedSong->rowCount(); i++) {
        if (i == row) {
            // // put arrow on the new one
            // QString currentTitleText = sourceForLoadedSong->item(row, 1)->text().replace(editingArrowStart, "");
            // QString newTitleText = editingArrowStart + currentTitleText;
            // sourceForLoadedSong->item(row, 1)->setText(newTitleText);

            QFont currentFont = sourceForLoadedSong->item(i, 1)->font();  // font goes to BOLD ITALIC BIGGER for loaded items
            currentFont.setBold(true);
            currentFont.setItalic(true);
//            currentFont.setPointSize(currentFont.pointSize() + 2);
            sourceForLoadedSong->item(i, 0)->setFont(currentFont);
            sourceForLoadedSong->item(i, 1)->setFont(currentFont);
            sourceForLoadedSong->item(i, 2)->setFont(currentFont);
            sourceForLoadedSong->item(i, 3)->setFont(currentFont);
        }
        sourceForLoadedSong->item(i, 5)->setText((i == row) ? "1" : ""); // and this is the one being edited (clear out others)
    }

    // these must be down here, to set the correct values...
    int pitchInt = pitch.toInt();

    ui->darkPitchSlider->setValue(pitchInt);

    on_darkPitchSlider_valueChanged(pitchInt); // manually call this, in case the setValue() line doesn't call valueChanged() when the value set is
        //   exactly the same as the previous value.  This will ensure that cBass->setPitch() gets called (right now) on the new stream.

    if (ui->actionAutostart_playback->isChecked()) {
        on_darkPlayButton_clicked();
    }

    ui->playlist2Table->setFocus();
    ui->playlist2Table->resizeColumnToContents(0); // number column needs to be resized, if bolded

    t.elapsed(__LINE__);
}

// -------------
void MainWindow::on_playlist3Table_itemDoubleClicked(QTableWidgetItem *item)
{
    PerfTimer t("on_playlist3Table_itemDoubleClicked", __LINE__);

    on_darkStopButton_clicked();  // if we're loading a new MP3 file, stop current playback
    saveCurrentSongSettings();

    t.elapsed(__LINE__);

    int row = item->row();
    QString pathToMP3 = ui->playlist3Table->item(row,4)->text();

    QFileInfo fi(pathToMP3);
    if (!fi.exists()) {
        qDebug() << "ERROR: File does not exist " << pathToMP3;
        return;  // can't load a file that doesn't exist
    }

    QString nextFile = "";
    if (row+1 < ui->playlist3Table->rowCount()) {
        nextFile = ui->playlist3Table->item(row+1,4)->text();
        // qDebug() << "on_playlist3Table_itemDoubleClicked:  nextFile = " << nextFile;
        // } else {
        // qDebug() << "on_playlist3Table_itemDoubleClicked: no nextFile";
    }
    static QRegularExpression dotMusicSuffix("\\.(mp3|m4a|wav|flac)$", QRegularExpression::CaseInsensitiveOption); // match with music extensions
    QString songTitle = pathToMP3.split('/').last().replace(dotMusicSuffix,"");

    // parse the filename into parts, so we can use the shortTitle -----
    QString label;
    QString labelNumber;
    QString labelExtra;
    QString realTitle;
    QString realShortTitle;

    bool success = breakFilenameIntoParts(songTitle, label, labelNumber, labelExtra, realTitle, realShortTitle);
    if (success) {
        songTitle = realShortTitle;
    }

    QString songType = (fi.path().replace(musicRootPath + "/","").split("/"))[0]; // e.g. "hoedown" or "patter"
    QString songLabel = "RIV 123";

    // qDebug() << "***** on_playlist3Table_itemDoubleClicked(): songType = " << songType << songLabel;

    // these must be up here to get the correct values...
    QString pitch  = ui->playlist3Table->item(row, 2)->text();
    QString tempo  = ui->playlist3Table->item(row, 3)->text();
//    QString number = ui->playlist3Table->item(row, 0)->text();

    targetPitch = pitch;  // save this string, and set pitch slider AFTER base BPM has been figured out
    targetTempo = tempo;  // save this string, and set tempo slider AFTER base BPM has been figured out
//    targetNumber = number; // save this, because tempo changes when this is set are playlist modifications, too

//    qDebug() << "on_playlist3Table_itemDoubleClicked: " << songTitle << songType << songLabel << pitch << tempo << pathToMP3;

    t.elapsed(__LINE__);

    loadMP3File(pathToMP3, songTitle, songType, songLabel, nextFile);

    t.elapsed(__LINE__);

//     // clear all the "1"s and arrows from palette slots
    for (int slot = 0; slot < 3; slot++) {
        MyTableWidget *tables[] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};
        MyTableWidget *table = tables[slot];
        for (int i = 0; i < table->rowCount(); i++) {
            if (table->item(i, 5)->text() == "1") {
                // // clear the arrows out of the other tables
                // QString currentTitleTextWithoutArrow = table->item(i, 1)->text().replace(editingArrowStart, "");
                // table->item(i, 1)->setText(currentTitleTextWithoutArrow);

                QFont currentFont = table->item(i, 1)->font(); // font goes to neutral (not bold or italic, and normal size) for NOT-loaded items
                currentFont.setBold(false);
                currentFont.setItalic(false);
//                    currentFont.setPointSize(currentFont.pointSize() - 1);
                table->item(i, 0)->setFont(currentFont);
                table->item(i, 1)->setFont(currentFont);
                table->item(i, 2)->setFont(currentFont);
                table->item(i, 3)->setFont(currentFont);
            }
            table->item(i, 5)->setText(""); // clear out the old table
        }
    }

    sourceForLoadedSong = ui->playlist3Table; // THIS is where we got the currently loaded song (this is the NEW table)

    for (int i = 0; i < sourceForLoadedSong->rowCount(); i++) {
        if (i == row) {
            // // put arrow on the new one
            // QString currentTitleText = sourceForLoadedSong->item(row, 1)->text().replace(editingArrowStart, "");
            // QString newTitleText = editingArrowStart + currentTitleText;
            // sourceForLoadedSong->item(row, 1)->setText(newTitleText);

            QFont currentFont = sourceForLoadedSong->item(i, 1)->font();  // font goes to BOLD ITALIC BIGGER for loaded items
            currentFont.setBold(true);
            currentFont.setItalic(true);
//            currentFont.setPointSize(currentFont.pointSize() + 1);
            sourceForLoadedSong->item(i, 0)->setFont(currentFont);
            sourceForLoadedSong->item(i, 1)->setFont(currentFont);
            sourceForLoadedSong->item(i, 2)->setFont(currentFont);
            sourceForLoadedSong->item(i, 3)->setFont(currentFont);
        }
        sourceForLoadedSong->item(i, 5)->setText((i == row) ? "1" : ""); // and this is the one being edited (clear out others)
    }

    // these must be down here, to set the correct values...
    int pitchInt = pitch.toInt();

    ui->darkPitchSlider->setValue(pitchInt);

    on_darkPitchSlider_valueChanged(pitchInt); // manually call this, in case the setValue() line doesn't call valueChanged() when the value set is
        //   exactly the same as the previous value.  This will ensure that cBass->setPitch() gets called (right now) on the new stream.

    if (ui->actionAutostart_playback->isChecked()) {
        on_darkPlayButton_clicked();
    }

    ui->playlist3Table->setFocus();
    ui->playlist3Table->resizeColumnToContents(0); // number column needs to be resized, if bolded

    t.elapsed(__LINE__);
}


// ========================
// DARK MODE: file dialog to ask for a file name, then save slot to that file
// TODO: much of this code is same as saveSlotAsPlaylist(whichSlot), which just opens a file dialog
//   so this should be factored.
void MainWindow::saveSlotAsPlaylist(int whichSlot)  // slots 0 - 2
{
    MyTableWidget *theTableWidget;
    QLabel *theLabel;
    switch (whichSlot) {
        case 1: theTableWidget = ui->playlist2Table; theLabel = ui->playlist2Label; break;
        case 2: theTableWidget = ui->playlist3Table; theLabel = ui->playlist3Label; break;
        case 0:
        default: theTableWidget = ui->playlist1Table; theLabel = ui->playlist1Label; break;
    }

    // // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    // const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");

    // QString startingPlaylistDirectory = prefsManager.MySettings.value(DEFAULT_PLAYLIST_DIR_KEY).toString();
    // if (startingPlaylistDirectory.isNull()) {
    //     // first time through, start at HOME
    //     startingPlaylistDirectory = QDir::homePath();
    // }

    QString startingPlaylistDirectory;

    if (relPathInSlot[whichSlot] == "") {
        startingPlaylistDirectory = musicRootPath + "/playlists/Untitled playlist.csv";
    } else {
        startingPlaylistDirectory = musicRootPath + "/playlists/" + relPathInSlot[whichSlot] + "_copy.csv";
    }

    QString preferred("CSV files (*.csv)");
    trapKeypresses = false;

    QString startHere = startingPlaylistDirectory;  // if we haven't saved yet

    QString PlaylistFileName =
        QFileDialog::getSaveFileName(this,
                                     tr("Save Playlist"),
                                     startHere,
                                     tr("CSV files (*.csv)"),
                                     &preferred);  // CSV required now
    trapKeypresses = true;
    if (PlaylistFileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    QString fullFilePath = PlaylistFileName;
//    qDebug() << "fullFilePath: " << fullFilePath;

    // // not null, so save it in Settings (File Dialog will open in same dir next time)
    // QFileInfo fInfo(PlaylistFileName);
    // prefsManager.Setdefault_playlist_dir(fInfo.absolutePath());

    QString playlistShortName = PlaylistFileName.replace(musicRootPath,"").split('/').last().replace(".csv",""); // PlaylistFileName is now altered
//    qDebug() << "playlistShortName: " << playlistShortName;

    // ACTUAL SAVE TO FILE ============
    filewatcherShouldIgnoreOneFileSave = true;  // I don't know why we have to do this, but rootDir is being watched, which causes this to be needed.

    // add on .CSV if user didn't specify (File Options Dialog?)
    if (!fullFilePath.endsWith(".csv", Qt::CaseInsensitive)) {
        fullFilePath = fullFilePath + ".csv";
    }

    QFile file(fullFilePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream stream(&file);

        stream << "relpath,pitch,tempo" << ENDL;  // NOTE: This is now a RELATIVE PATH, and "relpath" is used to detect that.

        for (int i = 0; i < theTableWidget->rowCount(); i++) {
            QString path = theTableWidget->item(i, 4)->text();
            path = path.replace(musicRootPath,"").replace("\"","\"\""); // if path contains a QUOTE, it needs to be changed to DOUBLE QUOTE in CSV

            QString pitch = theTableWidget->item(i, 2)->text();
            QString tempo = theTableWidget->item(i, 3)->text();
            // qDebug() << path + "," + pitch + "," + tempo;
            stream << "\"" + path + "\"," + pitch + "," + tempo << ENDL; // relative path with quotes, then pitch then tempo (% or bpm)
        }

        file.close(); // OK, we're done saving the file, so...
        slotModified[whichSlot] = false;

        // and update the QString array (for later use)
        QString rel = fullFilePath;
        relPathInSlot[whichSlot] = rel.replace(musicRootPath + "/playlists/", "").replace(".csv",""); // relative to musicDir/playlists
        // qDebug() << "saveSlotsAsPlaylist, relPathInSlot set to: " << relPathInSlot[whichSlot];

        // update the label with the partial path, e.g. "Jokers/2024/playlist_06.05"
        theLabel->setText(QString("<img src=\":/graphics/icons8-menu-64.png\" width=\"10\" height=\"9\">%1").arg(relPathInSlot[whichSlot]));

        // and refresh the TreeWidget, because we have a new playlist now...
        updateTreeWidget();

        // NOW, make it so at app restart we reload this one that now has a name...
        switch (whichSlot) {
            case 0: prefsManager.SetlastPlaylistLoaded( "playlists/" + relPathInSlot[0]); break;
            case 1: prefsManager.SetlastPlaylistLoaded2("playlists/" + relPathInSlot[1]); break;
            case 2: prefsManager.SetlastPlaylistLoaded3("playlists/" + relPathInSlot[2]); break;
            default: break;
        }

        ui->statusBar->showMessage(QString("Saved Playlist %1").arg(playlistShortName));
    } else {
        ui->statusBar->showMessage(QString("ERROR: could not save playlist to CSV file."));
    }

}

// -----------
// DARK MODE: NO file dialog, just save slot to that same file where it came from
// SAVE a playlist in a slot to a CSV file
void MainWindow::saveSlotNow(int whichSlot) {

    if (relPathInSlot[whichSlot] == "") {
        // nothing in this slot
        return;
    }

    if (!slotModified[whichSlot]) {
        // if there's something in the slot, BUT it has not been modified, don't bother to save it on top of itself (no changes)
        return;
    }

    QString PlaylistFileName = musicRootPath + "/playlists/" + relPathInSlot[whichSlot] + ".csv";
//    qDebug() << "Let's save this slot:" << whichSlot << ":" << PlaylistFileName;

//    return; // TEST TEST TEST

    // which tableWidget are we dealing with?
    MyTableWidget *theTableWidget;
    switch (whichSlot) {
        case 1: theTableWidget = ui->playlist2Table; break;
        case 2: theTableWidget = ui->playlist3Table; break;
        case 0:
        default: theTableWidget = ui->playlist1Table; break;
    }

    QString playlistShortName = PlaylistFileName;
    playlistShortName = playlistShortName.replace(musicRootPath,"").split('/').last().replace(".csv",""); // PlaylistFileName is now altered
//    qDebug() << "playlistShortName: " << playlistShortName;

    // ACTUAL SAVE TO FILE ============
    filewatcherShouldIgnoreOneFileSave = true;  // I don't know why we have to do this, but rootDir is being watched, which causes this to be needed.

    QFile file(PlaylistFileName);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream stream(&file);

        stream << "relpath,pitch,tempo" << ENDL;  // NOTE: This is a RELATIVE PATH, and "relpath" is used to detect that.

        for (int i = 0; i < theTableWidget->rowCount(); i++) {
            QString path = theTableWidget->item(i, 4)->text();
            path = path.replace(musicRootPath,"").replace("\"","\"\""); // if path contains a QUOTE, it needs to be changed to DOUBLE QUOTE in CSV

            QString pitch = theTableWidget->item(i, 2)->text();
            QString tempo = theTableWidget->item(i, 3)->text();
            // qDebug() << path + "," + pitch + "," + tempo;
            stream << "\"" + path + "\"," + pitch + "," + tempo << ENDL; // relative path with quotes, then pitch then tempo (% or bpm)
        }

        file.close(); // OK, we're done saving the file, so...
        slotModified[whichSlot] = false;
    } else {
        qDebug() << "ERROR: could not save playlist to CSV file.";
    }
}

void MainWindow::playlistSlotWatcherTriggered() {
    // NOTE: This QTimer is retriggerable, so we only get here if there were no
    //  triggers (modifications to any slots) in the last 10 sec.
    for (int i = 0; i < 3; i++) {
        if (slotModified[i]) {
            saveSlotNow(i);
            // qDebug() << "SAVING SLOT " << i << "which was modified";
//            slotModified[i] = false; // redundant for testing  // THIS SHOULD NOT BE ACTIVE CODE UNDER NORMAL OPERATION
        }
    }

    // for sure, all slots are saved now, so disable the timer until some slot is modified again
    playlistSlotWatcherTimer->stop();
}

void MainWindow::loadPlaylistFromFileToSlot(int whichSlot)
{
    // on_stopButton_clicked();  // if we're loading a new PLAYLIST file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    //    const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");
    //    QString musicRootPath = prefsManager.GetmusicPath();
    QString startingPlaylistDirectory = prefsManager.Getdefault_playlist_dir();

    trapKeypresses = false;
    QString PlaylistFileName =
        QFileDialog::getOpenFileName(this,
                                     tr("Load Playlist"),
                                     startingPlaylistDirectory,
                                     tr("Playlist Files (*.csv)"));
    trapKeypresses = true;
    if (PlaylistFileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
    //    QDir CurrentDir;
    QFileInfo fInfo(PlaylistFileName);
    prefsManager.Setdefault_playlist_dir(fInfo.absolutePath());

    int songCount;
    loadPlaylistFromFileToPaletteSlot(PlaylistFileName, whichSlot, songCount);
}

// ----------------------------
void MainWindow::printPlaylistFromSlot(int whichSlot)
{
    // Playlist > Print Playlist...
    QPrinter printer;
    QPrintDialog printDialog(&printer, this);
    printDialog.setWindowTitle("Print Playlist");

    if (printDialog.exec() == QDialog::Rejected) {
        return;
    }

    // --------
    QList<PlaylistExportRecord> exports;

    // iterate over the playlist to get all the info
    MyTableWidget *tables[] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};
    MyTableWidget *table = tables[whichSlot];

    for (int i = 0; i < table->rowCount(); i++) {
        // qDebug() << "TABLE ITEM:" << table->item(i,0)->text() << table->item(i,1)->text()  << table->item(i,2)->text()  << table->item(i,3)->text()  << table->item(i,4)->text()  << table->item(i,5)->text();
        // 0: i
        // 1: title
        // 2: pitch
        // 3: tempo
        // 4: fullPath
        // 5: editingIndicator (1 = editing)

        PlaylistExportRecord rec;
        rec.index = i+1;
        rec.title = table->item(i,4)->text(); // fullPath
        rec.pitch = table->item(i,2)->text();
        rec.tempo = table->item(i,3)->text();

        exports.append(rec);
    }

    QString shortPlaylistName = relPathInSlot[whichSlot];

    QString toBePrinted = "<h2>PLAYLIST: " + shortPlaylistName + "</h2>\n"; // TITLE AT TOP OF PAGE

    QTextDocument doc;
    QSizeF paperSize;
    paperSize.setWidth(printer.width());
    paperSize.setHeight(printer.height());
    doc.setPageSize(paperSize);

    patterColorString = prefsManager.GetpatterColorString();
    singingColorString = prefsManager.GetsingingColorString();
    calledColorString = prefsManager.GetcalledColorString();
    extrasColorString = prefsManager.GetextrasColorString();

    char buf[256];
    foreach (const PlaylistExportRecord &rec, exports)
    {
        QString baseName = rec.title;
        baseName.replace(QRegularExpression("^" + musicRootPath),"");  // delete musicRootPath at beginning of string

        snprintf(buf, sizeof(buf), "%02d: %s\n", rec.index, baseName.toLatin1().data());

        QStringList parts = baseName.split("/");
        QString folderTypename = parts[1].toLower(); // /patter/foo.mp3 --> "patter"

        QString colorString("black");
        if (songTypeNamesForPatter.contains(folderTypename)) {
            colorString = patterColorString;
        } else if (songTypeNamesForSinging.contains(folderTypename)) {
            colorString = singingColorString;
        } else if (songTypeNamesForCalled.contains(folderTypename)) {
            colorString = calledColorString;
        } else if (songTypeNamesForExtras.contains(folderTypename)) {
            colorString = extrasColorString;
        }
        toBePrinted += "<span style=\"color:" + colorString + "\">" + QString(buf) + "</span><BR>\n";
    }

    //        qDebug() << "PRINT: " << toBePrinted;

    doc.setHtml(toBePrinted);
    doc.print(&printer);
}

// ----------------------------------------------
void MainWindow::darkPaletteTitleLabelDoubleClicked(QMouseEvent * e)
{
    Q_UNUSED(e)
    // qDebug() << "SOME PALETTE TITLE FIELD HAS BEEN DOUBLE-CLICKED.";

    // Let's figure out which palette slot has a selected item. THAT is the double-clicked one.
    QTableWidget *theTables[3] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};

    for (int i = 0; i < 3; i++) {
        // qDebug() << "LOOK AT SLOT:" << i;
        QTableWidget *theTable = theTables[i];

        QItemSelectionModel *selectionModel = theTable->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();
        int row = -1;

        if (selected.count() == 1) {
            // exactly 1 row was selected (good)
            QModelIndex index = selected.at(0);
            row = index.row();

            // qDebug() << "Table:" << i << ", Row:" << row << " DOUBLE CLICKED";

            QString path = theTable->item(row, 4)->text();
            // qDebug() << "Path: " << path;

            // pretend we clicked on the tempo label of that row, and let the existing functions do the work...
            switch (i) {
                case 0: on_playlist1Table_itemDoubleClicked(theTable->item(row, 3)); break;
                case 1: on_playlist2Table_itemDoubleClicked(theTable->item(row, 3)); break;
                case 2: on_playlist3Table_itemDoubleClicked(theTable->item(row, 3)); break;
            }

            break; // break out of the for loop, because we found what was clicked on
        } // else more than 1 row or no rows, just return -1

    }
}

void MainWindow::refreshAllPlaylists() {
    // qDebug() << "Refreshing playlist views, because TAGS visibility changed.";
    // bool tagsAreVisible = ui->actionViewTags->isChecked();

    QTableWidget *theTables[3] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};

    for (int i = 0; i < 3; i++) {
        // qDebug() << "LOOK AT SLOT:" << i;
        QTableWidget *theTable = theTables[i];

        for (int j = 0; j < theTable->rowCount(); j++) {
            // for each title in the table
            QString relativePath;
            QString PlaylistFileName;
            if (dynamic_cast<QLabel*>(theTable->cellWidget(j, 1)) != nullptr) {
                // QString title = dynamic_cast<QLabel*>(theTable->cellWidget(j, 1))->text(); // don't need this right now...
                relativePath = theTable->item(j,4)->text();
                relativePath.replace(musicRootPath, "");

                PlaylistFileName = musicRootPath + "/playlists/" + relPathInSlot[i] + ".csv";
                // qDebug() << "refreshAllPlaylists: relativePath, PlaylistFileName:" << relativePath << PlaylistFileName;

                // this is what we want: "/patter/RIV 1180 - Sea Chanty.mp3" "/Users/mpogue/Library/CloudStorage/Box-Box/__squareDanceMusic_Box/playlists/Jokers/2024/Jokers_2024.06.05.csv"
                setTitleField(theTable, j, relativePath, true, PlaylistFileName, relativePath);
            }

        }

    }
}

void MainWindow::getAppleMusicPlaylists() {
    // qDebug() << "getAppleMusicPlaylists ==================";

    pathStackApplePlaylists->clear(); // start from nothing
    allAppleMusicPlaylists.clear();

    QFile applescriptfile(":/applescript/getAppleMusicPlaylists.txt");

    if (!applescriptfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Handle error, e.g., return an empty string or throw an exception
        qDebug() << "Could not find getAppleMusicPlaylists.txt file in resources";
        return;
    }

    QTextStream in(&applescriptfile);
    QString aScript = in.readAll();
    applescriptfile.close();

    // qDebug() << aScript;

    // RUN IT -----------------
    QString osascript = "/usr/bin/osascript";
    QStringList processArguments;
    processArguments << "-l" << "AppleScript";

    QProcess p;
    p.start(osascript, processArguments);
    p.write(aScript.toUtf8());
    p.closeWriteChannel();
    p.waitForReadyRead(-1);
    QByteArray result = p.readAll();
    QString resultAsString(result); // <-- RESULT IS HERE

    // qDebug() << "the result of the script is --------\n" << resultAsString;

    // qDebug() << "in nicer format --------";
    QStringList s = resultAsString.split("\r"); // split into lines

    QString currentPlaylistName = "";
    int currentItemNumber = 1;

    for (int i = 1; i < s.size(); ++i) { // skip the header line, process lines one at a time
        // qDebug() << "--------";
        QStringList sl = parseCSV(s[i]);
        if (sl.size() != 3) continue;  // removes empty Apple Music playlists
        // qDebug() << "LINE: " << qUtf8Printable(s[i]);
        // qDebug() << "RESULT:" << parseCSV(s[i]);

        QString playlistName = sl[0];

        if (playlistName != currentPlaylistName) {
            currentPlaylistName = playlistName;
            currentItemNumber = 1;
        }

        QString shortTitle = sl[1];
        QString pathName = sl[2];
        allAppleMusicPlaylistNames.append(playlistName);

        // append an entry to the pathStack for Apple Music playlist items, that looks like
        //   "Christmas$!$currentItemNumber$!$Use This Title#!#/Users/mpogue/foo/bar/baz/Use This Title.mp3"
        QString AppleMusicPathStackEntry(playlistName); // type == AppleMusicPlaylistName, e.g. "Christmas"
        AppleMusicPathStackEntry += "$!$";
        if (currentItemNumber >= 100) {
            AppleMusicPathStackEntry += "!"; // 3-digit numbers look like "foo !100" (Yes, this is a kludge for now)
        }
        if (currentItemNumber < 10) {
            AppleMusicPathStackEntry += "0"; // numbers < 100 look like 2 digit with leading zero, "foo 03"
        }
        AppleMusicPathStackEntry += QString::number(currentItemNumber++);
        AppleMusicPathStackEntry += "$!$";
        AppleMusicPathStackEntry += shortTitle; // title to use for the title column
        AppleMusicPathStackEntry += "#!#";
        AppleMusicPathStackEntry += pathName; // full path to audio file
        // qDebug() << "Appending: " << AppleMusicPathStackEntry;
        pathStackApplePlaylists->append(AppleMusicPathStackEntry);

        allAppleMusicPlaylists.append(sl); // FIX: is this still needed?
    }

    allAppleMusicPlaylistNames.removeDuplicates();          // Remove duplicates
    allAppleMusicPlaylistNames.sort(Qt::CaseInsensitive);   // Sort (case INsensitive)

    // qDebug() << "allAppleMusicPlaylistNames" << allAppleMusicPlaylistNames;
    // qDebug() << "allAppleMusicPlaylists" << allAppleMusicPlaylists;

    p.waitForFinished();
}

void MainWindow::getLocalPlaylists() {
    // qDebug() << "getLocalPlaylists ==================";
}
