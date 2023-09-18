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

// PLAYLIST MANAGEMENT -------------------------------

// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"
#include "utility.h"
#include "songlistmodel.h"
#include "tablenumberitem.h"

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
            if (current == ',')
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

//    qDebug() << "loadPlaylist: " << PlaylistFileName;
    addFilenameToRecentPlaylist(PlaylistFileName);  // remember it in the Recent list

    // --------
    QString firstBadSongLine = "";
    QFile inputFile(PlaylistFileName);
    if (inputFile.open(QIODevice::ReadOnly)) { // defaults to Text mode

        // first, clear all the playlist numbers that are there now.
        for (int i = 0; i < ui->songTable->rowCount(); i++) {
            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
            theItem->setText("");
        }

//        int lineCount = 1;
        linesInCurrentPlaylist = 0;

        QTextStream in(&inputFile);

        if (PlaylistFileName.endsWith(".csv", Qt::CaseInsensitive)) {
            // CSV FILE =================================

            QString header = in.readLine();  // read header (and throw away for now), should be "abspath,pitch,tempo"

            // V1 playlists use absolute paths, V2 playlists use relative paths
            // This allows two things:
            //   - moving an entire Music Directory from one place to another, and playlists still work
            //   - sharing an entire Music Directory across platforms, and playlists still work
            bool v2playlist = header.contains("relpath");
            bool v1playlist = !v2playlist;

            while (!in.atEnd()) {
                QString line = in.readLine();

                if (line == "") {
                    // ignore, it's a blank line
                }
                else {
                    songCount++;  // it's a real song path

                    QStringList list1 = parseCSV(line);  // This is more robust than split(). Handles commas inside double quotes, double double quotes, etc.

                    bool match = false;
                    // exit the loop early, if we find a match
                    for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {

                        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();

                        if ( (v1playlist && (list1[0] == pathToMP3)) ||             // compare absolute pathnames (fragile, old V1 way)
                             (v2playlist && compareRelative(list1[0], pathToMP3))   // compare relative pathnames (better, new V2 way)
                           ) {
                            // qDebug() << "MATCH::" << list1[0] << pathToMP3;
//                            qDebug() << "loadPlaylistFromFile: " << pathToMP3;
                            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
                            theItem->setText(QString::number(songCount));
//                            qDebug() << "                     number: " << QString::number(songCount);

                            QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);
                            theItem2->setText(list1[1].trimmed());
//                            qDebug() << "                     pitch: " << list1[1].trimmed();

                            QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);
                            theItem3->setText(list1[2].trimmed());
//                            qDebug() << "                     tempo: " << list1[2].trimmed();

                            match = true;
                        }
                    }
                    // if we had no match, remember the first non-matching song path
                    if (!match && firstBadSongLine == "") {
                        firstBadSongLine = line;
                    }

                }

//                lineCount++;
            } // while
        }
//        else {
//            // M3U FILE =================================
//            // to workaround old-style Mac files that have a bare "\r" (which readLine() can't handle)
//            //   in particular, iTunes exported playlists use this old format.
//            QStringList theWholeFile = in.readAll().replace("\r\n","\n").replace("\r","\n").split("\n");

//            foreach (const QString &line, theWholeFile) {
////                qDebug() << "line:" << line;
//                if (line == "#EXTM3U") {
//                    // ignore, it's the first line of the M3U file
//                }
//                else if (line == "") {
//                    // ignore, it's a blank line
//                }
//                else if (line.at( 0 ) == '#' ) {
//                    // it's a comment line
//                    if (line.mid(0,7) == "#EXTINF") {
//                        // it's information about the next line, ignore for now.
//                    }
//                }
//                else {
//                    songCount++;  // it's a real song path

//                    bool match = false;
//                    // exit the loop early, if we find a match
//                    for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {
//                        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
////                        qDebug() << "pathToMP3:" << pathToMP3;
//                        if (line == pathToMP3) { // FIX: this is fragile, if songs are moved around
//                            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
//                            theItem->setText(QString::number(songCount));

//                            match = true;
//                        }
//                    }
//                    // if we had no match, remember the first non-matching song path
//                    if (!match && firstBadSongLine == "") {
//                        firstBadSongLine = line;
//                    }

//                }

//                lineCount++;

//            } // for each line in the M3U file

//        } // else M3U file

        inputFile.close();
        linesInCurrentPlaylist += songCount; // when non-zero, this enables saving of the current playlist
//        qDebug() << "linesInCurrentPlaylist:" << linesInCurrentPlaylist;

//        qDebug() << "FBS:" << firstBadSongLine << ", linesInCurrentPL:" << linesInCurrentPlaylist;
        if (firstBadSongLine=="" && linesInCurrentPlaylist != 0) {
            // a playlist is now loaded, NOTE: side effect of loading a playlist is enabling Save/SaveAs...
//            qDebug() << "LOADED CORRECTLY. enabling Save As";
            ui->actionSave_Playlist_2->setEnabled(true);  // Playlist > Save Playlist 'name' is enabled, because you can always save a playlist with a diff name
            ui->actionSave_Playlist->setEnabled(true);  // Playlist > Save Playlist As...
            ui->actionPrint_Playlist->setEnabled(true);  // Playlist > Print Playlist...
        }
    }
    else {
        // file didn't open...
        return("");
    }

    return(firstBadSongLine);  // return error song (if any)
}

// PLAYLIST MANAGEMENT ===============================================
void MainWindow::finishLoadingPlaylist(QString PlaylistFileName) {
//    qDebug() << "finishLoadingPlaylist::PlaylistFileName: " << PlaylistFileName;
    startLongSongTableOperation("finishLoadingPlaylist"); // for performance measurements, hide and sorting off

    // --------
    QString firstBadSongLine = "";
    int songCount = 0;

    firstBadSongLine = loadPlaylistFromFile(PlaylistFileName, songCount);

    // simplify path, for the error message case
    firstBadSongLine = firstBadSongLine.split(",")[0].replace("\"", "").replace(musicRootPath, "");

    sortByDefaultSortOrder();
    ui->songTable->sortItems(kNumberCol);  // sort by playlist # as primary (must be LAST)

    // select the very first row, and trigger a GO TO PREVIOUS, which will load row 0 (and start it, if autoplay is ON).
    // only do this, if there were no errors in loading the playlist numbers.
//    if (firstBadSongLine == "") {
//        ui->songTable->selectRow(1); // select first row of newly loaded and sorted playlist!
////        on_actionPrevious_Playlist_Item_triggered();
//    }

    stopLongSongTableOperation("finishLoadingPlaylist"); // for performance measurements, sorting on again and show

    // MAKE SHORT NAME FOR PLAYLIST FROM FILENAME -------
//    qDebug() << "PlaylistFileName: " << PlaylistFileName;
    static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
    QRegularExpressionMatch match = regex1.match(PlaylistFileName);
    QString shortPlaylistName("ERROR 7");

    if (match.hasMatch())
    {
        shortPlaylistName = match.captured(1);
    }

    ui->actionSave_Playlist_2->setText(QString("Save Playlist '") + shortPlaylistName + "'"); // and now Playlist > Save Playlist 'name' has a name, because it was loaded (possibly with errors)

//    QString msg1 = QString("Loaded playlist with ") + QString::number(songCount) + QString(" items.");
    QString msg1 = QString("Playlist: ") + shortPlaylistName;
    if (firstBadSongLine != "") {
        // if there was a non-matching path, tell the user what the first one of those was
        msg1 = QString("ERROR: missing '...") + firstBadSongLine + QString("'");
        ui->statusBar->setStyleSheet("color: red");

        ui->songTable->clearSelection(); // select nothing, if error
    } else {
        ui->statusBar->setStyleSheet("color: black");
    }
    ui->statusBar->showMessage(msg1);

//    qDebug() << "FINISHED LOADED CORRECTLY. enabling Save As";
    ui->actionSave_Playlist_2->setEnabled(false);  // Playlist > Save Playlist 'name' is disabled, because it's not modified yet
    ui->actionSave_Playlist->setEnabled(true);  // Playlist > Save Playlist As...
    ui->actionPrint_Playlist->setEnabled(true);  // Playlist > Print Playlist...

    ui->songTable->scrollToItem(ui->songTable->item(0, kNumberCol)); // EnsureVisible row 0
    ui->songTable->selectRow(0); // select first row of newly loaded and sorted playlist!

    lastSavedPlaylist = PlaylistFileName;  // have to save something here to enable File > Save (to same place as loaded).

    prefsManager.SetlastPlaylistLoaded(shortPlaylistName); // save the name of the playlist, so we can reload at app start time
}

void MainWindow::markPlaylistModified(bool isModified) {

    playlistHasBeenModified = isModified; // we don't track this by looking at the status message anymore.

//    qDebug() << "markPlaylistModified: " << isModified;
    QString current = ui->statusBar->currentMessage();
    if (current == "") {
        current = "Playlist: Untitled"; // if there is no playlist, called it "Untitled" when we add something for first time
    }
//    qDebug() << "current: " << current;
    static QRegularExpression asteriskAtEndRegex("\\*$");
    current.replace(asteriskAtEndRegex, "");  // delete the star at the end, if present
//    qDebug() << "now current: " << current;
    if (isModified) {
//        qDebug() << "updating: " << current + "*";
        ui->statusBar->showMessage(current + "*");
        ui->actionSave->setEnabled(true);
    } else {
//        qDebug() << "updating: " << current;
        ui->statusBar->showMessage(current);
        ui->actionSave->setEnabled(false);
    }
}

void MainWindow::on_actionLoad_Playlist_triggered()
{
    on_stopButton_clicked();  // if we're loading a new PLAYLIST file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");
    QString musicRootPath = prefsManager.GetmusicPath();
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
    QDir CurrentDir;
    QFileInfo fInfo(PlaylistFileName);
    prefsManager.Setdefault_playlist_dir(fInfo.absolutePath());

    finishLoadingPlaylist(PlaylistFileName);
}

bool comparePlaylistExportRecord(const PlaylistExportRecord &a, const PlaylistExportRecord &b)
{
    return a.index < b.index;
}

// SAVE CURRENT PLAYLIST TO FILE
void MainWindow::saveCurrentPlaylistToFile(QString PlaylistFileName) {
    // --------
    QList<PlaylistExportRecord> exports;

//    bool savingToM3U = PlaylistFileName.endsWith(".m3u", Qt::CaseInsensitive);

    // Iterate over the songTable
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();
        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
        QString songTitle = getTitleColTitle(ui->songTable, i);
        QString pitch = ui->songTable->item(i,kPitchCol)->text();
        QString tempo = ui->songTable->item(i,kTempoCol)->text();
//        qDebug() << "saveCurrentPlaylistToFile: " << tempo;

        if (playlistIndex != "") {
            // item HAS an index (that is, it is on the list, and has a place in the ordering)
            // TODO: reconcile int here with double elsewhere on insertion
            PlaylistExportRecord rec;
            rec.index = playlistIndex.toInt();
//            if (savingToM3U) {
//                rec.title = pathToMP3;  // NOTE: M3U must remain absolute
//                                        //    it will NOT survive moving musicDir or sharing it via Dropbox or Google Drive
//            } else {
            rec.title = pathToMP3.replace(musicRootPath, "");  // NOTE: this is now a relative (to the musicDir) path
                                                                   //    that should survive moving musicDir or sharing it via Dropbox or Google Drive
//            }
            rec.pitch = pitch;
            rec.tempo = tempo;
            exports.append(rec);
        }
    }

//    qSort(exports.begin(), exports.end(), comparePlaylistExportRecord);
    std::sort(exports.begin(), exports.end(), comparePlaylistExportRecord);
    // TODO: strip the initial part of the path off the Paths, e.g.
    //   /Users/mpogue/__squareDanceMusic/patter/C 117 - Restless Romp (Patter).mp3
    //   becomes
    //   patter/C 117 - Restless Romp (Patter).mp3
    //
    //   So, the remaining path is relative to the root music directory.
    //   When loading, first look at the patter and the rest
    //     if no match, try looking at the rest only
    //     if no match, then error (dialog?)
    //   Then on Save Playlist, write out the NEW patter and the rest

    static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
    QRegularExpressionMatch match = regex1.match(PlaylistFileName);
    QString shortPlaylistName("ERROR 7");

    if (match.hasMatch())
    {
        shortPlaylistName = match.captured(1);
    }

    filewatcherShouldIgnoreOneFileSave = true;  // I don't know why we have to do this, but rootDir is being watched, which causes this to be needed.
    QFile file(PlaylistFileName);
//    if (PlaylistFileName.endsWith(".m3u", Qt::CaseInsensitive)) {
//        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {  // delete, if it exists already
//            QTextStream stream(&file);
//            stream << "#EXTM3U" << ENDL << ENDL;

//            // list is auto-sorted here
//            foreach (const PlaylistExportRecord &rec, exports)
//            {
//                stream << "#EXTINF:-1," << ENDL;  // nothing after the comma = no special name
//                stream << rec.title << ENDL;
//            }
//            file.close();
//            addFilenameToRecentPlaylist(PlaylistFileName);  // add to the MRU list
//            markPlaylistModified(false); // turn off the * in the status bar
//            lastSavedPlaylist = PlaylistFileName;  // have to save something here to enable File > Save (to same place as loaded) and Auto-load
//            prefsManager.SetlastPlaylistLoaded(shortPlaylistName); // save the name of the playlist, so we can reload at app start time
//            ui->actionSave->setText(QString("Save Playlist '") + shortPlaylistName + "'"); // and now it has a name, because it was loaded (possibly with errors)
//        }
//        else {
//            ui->statusBar->showMessage(QString("ERROR: could not open M3U file."));
//        }
//    }
    /* else */ if (PlaylistFileName.endsWith(".csv", Qt::CaseInsensitive)) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream stream(&file);
            stream << "relpath,pitch,tempo" << ENDL;  // NOTE: This is now a RELATIVE PATH, and "relpath" is used to detect that.

            foreach (const PlaylistExportRecord &rec, exports)
            {
                stream << "\"" << rec.title << "\"," <<
                    rec.pitch << "," <<
                    rec.tempo << ENDL; // quoted absolute path, integer pitch (no quotes), integer tempo (opt % or 0)
            }
            file.close();
            addFilenameToRecentPlaylist(PlaylistFileName);  // add to the MRU list
            markPlaylistModified(false); // turn off the * in the status bar
            lastSavedPlaylist = PlaylistFileName;  // have to save something here to enable File > Save (to same place as loaded) and Auto-load
            prefsManager.SetlastPlaylistLoaded(shortPlaylistName); // save the name of the playlist, so we can reload at app start time
            ui->actionSave_Playlist_2->setText(QString("Save Playlist '") + shortPlaylistName + "'"); // and now Playlist > Save Playlist 'name' has a name, because it was loaded (possibly with errors)
        }
        else {
            ui->statusBar->showMessage(QString("ERROR: could not open CSV file."));
        }
    }
}

void MainWindow::savePlaylistAgain() // saves without asking for a filename (File > Save)
{
//    on_stopButton_clicked();  // if we're saving a new PLAYLIST file, stop current playback
//    qDebug() << "savePlaylistAgain()";
    if (lastSavedPlaylist == "") {
        // nothing saved yet!
//        qDebug() << "NOTHING SAVED YET.";
        return;  // so just return without saving anything
    }

//    qDebug() << "OK, SAVED TO: " << lastSavedPlaylist;

    // else use lastSavedPlaylist
//    qDebug() << "savePlaylistAgain::lastSavedPlaylist: " << lastSavedPlaylist;
    saveCurrentPlaylistToFile(lastSavedPlaylist);  // SAVE IT

    // TODO: if there are no songs specified in the playlist (yet, because not edited, or yet, because
    //   no playlist was loaded), Save Playlist... should be greyed out.
//    QString basefilename = lastSavedPlaylist.section("/",-1,-1);

    static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
    QRegularExpressionMatch match = regex1.match(lastSavedPlaylist);
    QString shortPlaylistName("ERROR 7");

    if (match.hasMatch())
    {
        shortPlaylistName = match.captured(1);
    }

//    qDebug() << "Basefilename: " << basefilename;
    ui->statusBar->showMessage(QString("Playlist: ") + shortPlaylistName); // this message is under the next one
//    ui->statusBar->showMessage(QString("Playlist saved as '") + basefilename + "'", 4000); // on top for 4 sec
    // no need to remember it here, it's already the one we remembered.

//    ui->actionSave->setEnabled(false);  // once saved, this is not reenabled, until you change it.

    markPlaylistModified(false); // after File > Save, file is NOT modified
}

QString MainWindow::getShortPlaylistName() {

    if (lastSavedPlaylist == "") {
        return("Untitled");
    }

    static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
    QRegularExpressionMatch match = regex1.match(lastSavedPlaylist);
    QString shortPlaylistName("ERROR 17");

    if (match.hasMatch())
    {
        shortPlaylistName = match.captured(1);
    }

    return(shortPlaylistName);
}

// TODO: strip off the root directory before saving...
void MainWindow::on_actionSave_Playlist_triggered()  // NOTE: this is really misnamed, it's (File > Save As)
{
//    on_stopButton_clicked();  // if we're saving a new PLAYLIST file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");

    QString startingPlaylistDirectory = prefsManager.MySettings.value(DEFAULT_PLAYLIST_DIR_KEY).toString();
    if (startingPlaylistDirectory.isNull()) {
        // first time through, start at HOME
        startingPlaylistDirectory = QDir::homePath();
    }

    QString preferred("CSV files (*.csv)");
    trapKeypresses = false;

    QString startHere = startingPlaylistDirectory + "/Untitled.csv";  // if we haven't saved yet
    if (lastSavedPlaylist != "") {
        startHere = lastSavedPlaylist;  // if we HAVE saved already, default to same file
    }

    QString PlaylistFileName =
        QFileDialog::getSaveFileName(this,
                                     tr("Save Playlist"),
                                     startHere,
                                     tr("CSV files (*.csv)"),
                                     &preferred);  // preferred is CSV
    trapKeypresses = true;
    if (PlaylistFileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
    QFileInfo fInfo(PlaylistFileName);
    prefsManager.Setdefault_playlist_dir(fInfo.absolutePath());

    saveCurrentPlaylistToFile(PlaylistFileName);  // SAVE IT

    // TODO: if there are no songs specified in the playlist (yet, because not edited, or yet, because
    //   no playlist was loaded), Save Playlist... should be greyed out.
//    QString basefilename = PlaylistFileName.section("/",-1,-1);

    static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
    QRegularExpressionMatch match = regex1.match(PlaylistFileName);
    QString shortPlaylistName("ERROR 7");

    if (match.hasMatch())
    {
        shortPlaylistName = match.captured(1);
    }

//    qDebug() << "Basefilename: " << basefilename;
    ui->statusBar->showMessage(QString("Playlist: ") + shortPlaylistName);
//    ui->statusBar->showMessage(QString("Playlist saved as '") + basefilename + "'", 4000);

    lastSavedPlaylist = PlaylistFileName; // remember it, for the next SAVE operation (defaults to last saved in this session)
//    qDebug() << "Save As::lastSavedPlaylist: " << lastSavedPlaylist;

    ui->actionSave_Playlist_2->setEnabled(false);  // now that we have Save As'd something, we can't Save that thing until modified
    ui->actionSave_Playlist_2->setText(QString("Save Playlist") + " '" + shortPlaylistName + "'"); // and now Playlist > Save Playlist 'name' has a name
}

void MainWindow::on_actionNext_Playlist_Item_triggered()
{
    // This code is similar to the row double clicked code...
    on_stopButton_clicked();  // if we're going to the next file in the playlist, stop current playback
    saveCurrentSongSettings();

    int row = nextVisibleSongRow(); 
    if (row < 0) {
      // more than 1 row or no rows at all selected (BAD)
      return;
    }
    ui->songTable->selectRow(row); // select new row!

    on_songTable_itemDoubleClicked(ui->songTable->item(row,1)); // as if we double clicked the next line

    // start playback automatically, if Autostart is enabled
    if (ui->actionAutostart_playback->isChecked()) {
        on_playButton_clicked();
    }

}

void MainWindow::on_actionPrevious_Playlist_Item_triggered()
{
    cBass->StreamGetPosition();  // get ready to look at the playhead position
    if (cBass->Current_Position != 0.0) {
        on_stopButton_clicked();  // if the playhead was not at the beginning, just stop current playback and go to START
        return;                   //   and return early... (don't save the current song settings)
    }
    // else, stop and move to the previous song...

    on_stopButton_clicked();  // if we were playing, just stop current playback
    // This code is similar to the row double clicked code...
    saveCurrentSongSettings();

    int row = previousVisibleSongRow();
    if (row < 0) {
        // more than 1 row or no rows at all selected (BAD)
        return;
    }

    ui->songTable->selectRow(row); // select new row!

    on_songTable_itemDoubleClicked(ui->songTable->item(row,1)); // as if we double clicked the previous line

    // start playback automatically, if Autostart is enabled
    if (ui->actionAutostart_playback->isChecked()) {
        on_playButton_clicked();
    }
}

void MainWindow::on_previousSongButton_clicked()
{
    on_actionPrevious_Playlist_Item_triggered();
}

void MainWindow::on_nextSongButton_clicked()
{
    on_actionNext_Playlist_Item_triggered();
}


void MainWindow::on_actionClear_Playlist_triggered()
{
    startLongSongTableOperation("on_actionClear_Playlist_triggered");  // for performance, hide and sorting off
    ui->songTable->blockSignals(true); // no signals when we are modifying

    // Iterate over the songTable
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        theItem->setText(""); // clear out the current list

        // let's intentionally NOT clear the pitches.  They are persistent within a session.
        // let's intentionally NOT clear the tempos.  They are persistent within a session.
    }

    lastSavedPlaylist = "";       // clear the name of the last playlist, since none is loaded
    linesInCurrentPlaylist = 0;   // the number of lines in the playlist is zero
    ui->actionSave_Playlist_2->setText(QString("Save Playlist")); // and now it has no name, because not loaded
    ui->actionSave_Playlist_2->setDisabled(true);
    ui->actionSave_Playlist->setDisabled(true);
    ui->actionPrint_Playlist->setDisabled(true);

    sortByDefaultSortOrder();

    ui->songTable->blockSignals(false); // re-enable signals
    stopLongSongTableOperation("on_actionClear_Playlist_triggered");  // for performance, sorting on again and show

    ui->songTable->clearSelection(); // when playlist is cleared, select no rows by default
    ui->songTable->scrollToItem(ui->songTable->item(0, kNumberCol)); // EnsureVisible row 0

    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    ui->statusBar->showMessage("");  // no playlist at all right now
    markPlaylistModified(false); // turn ON the * in the status bar

    // if playlist cleared, save that fact in Preferences by setting to ""
    prefsManager.SetlastPlaylistLoaded(""); // save the fact that no playlist was loaded

    ui->songTable->selectRow(1); // These 2 lines are intentionally down here
    ui->songTable->selectRow(0); // make coloring of row 1 Title correct (KLUDGE)
    ui->songTable->setFocus();  // if we're going to highlight the first row, we need to make it ENTER/RETURN-able
}

// ----------------------------------------------------------------------
int MainWindow::PlaylistItemCount() {
    int playlistItemCount = 0;

    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();  // this is the playlist #
        if (playlistIndex != "") {
            playlistItemCount++;
        }
    }

    return (playlistItemCount);
}

// ----------------------------------------------------------------------
void MainWindow::PlaylistItemToTop() {

//    PerfTimer t("PlaylistItemToTop", __LINE__);
//    t.start(__LINE__);

    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
    int currentNumberInt = currentNumberText.toInt();

//    ui->songTable->blockSignals(true); // while updating, do NOT call itemChanged
    startLongSongTableOperation("PlaylistItemToTop");
//    t.elapsed(__LINE__);

    if (currentNumberText == "") {
        // add to list, and make it the #1

        // Iterate over the entire songTable, incrementing every item
        for (int i=0; i<ui->songTable->rowCount(); i++) {
            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
            QString playlistIndex = theItem->text();  // this is the playlist #
            if (playlistIndex != "") {
                // if a # was set, increment it
                QString newIndex = QString::number(playlistIndex.toInt()+1);
                ui->songTable->item(i,kNumberCol)->setText(newIndex);
            }
        }

        ui->songTable->item(selectedRow, kNumberCol)->setText("1");  // this one is the new #1

    } else {
        // already on the list
        // Iterate over the entire songTable, incrementing items BELOW this item
        for (int i=0; i<ui->songTable->rowCount(); i++) {
            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
            QString playlistIndexText = theItem->text();  // this is the playlist #
            if (playlistIndexText != "") {
                int playlistIndexInt = playlistIndexText.toInt();
                if (playlistIndexInt < currentNumberInt) {
                    // if a # was set and less, increment it
                    QString newIndex = QString::number(playlistIndexInt+1);
//                    qDebug() << "old, new:" << playlistIndexText << newIndex;
                    ui->songTable->item(i,kNumberCol)->setText(newIndex);
                }
            }
        }
        // and then set this one to #1
        ui->songTable->item(selectedRow, kNumberCol)->setText("1");  // this one is the new #1
    }

//    ui->songTable->blockSignals(false); // done updating.
    stopLongSongTableOperation("PlaylistItemToTop");
//    t.elapsed(__LINE__);

    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    // moved an item, so we must enable saving of the playlist
    ui->actionSave->setEnabled(true);       // menu item Save is enabled now
    ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

    // ensure that the selected row is still visible in the current songTable window
    selectedRow = selectedSongRow();
    ui->songTable->scrollToItem(ui->songTable->item(selectedRow, kNumberCol)); // EnsureVisible

    // mark playlist modified
    markPlaylistModified(true); // turn ON the * in the status bar

//    t.elapsed(__LINE__);
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemToBottom() {
    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
    int currentNumberInt = currentNumberText.toInt();

    int playlistItemCount = PlaylistItemCount();  // how many items in the playlist right now?

//    ui->songTable->blockSignals(true); // while updating, do NOT call itemChanged
    startLongSongTableOperation("PlaylistItemToBottom");

    if (currentNumberText == "") {
        // add to list, and make it the bottom

        // Iterate over the entire songTable, not touching every item
        ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(playlistItemCount+1));  // this one is the new #LAST

    } else {
        // already on the list
        // Iterate over the entire songTable, decrementing items ABOVE this item
        // TODO: turn off sorting
        for (int i=0; i<ui->songTable->rowCount(); i++) {
            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
            QString playlistIndexText = theItem->text();  // this is the playlist #
            if (playlistIndexText != "") {
                int playlistIndexInt = playlistIndexText.toInt();
                if (playlistIndexInt > currentNumberInt) {
                    // if a # was set and more, decrement it
                    QString newIndex = QString::number(playlistIndexInt-1);
                    ui->songTable->item(i,kNumberCol)->setText(newIndex);
                }
            }
        }
        // and then set this one to #LAST
        ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(playlistItemCount));  // this one is the new #1
    }

//    ui->songTable->blockSignals(false); // done updating
    stopLongSongTableOperation("PlaylistItemToBottom");

    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    // moved an item, so we must enable saving of the playlist
    ui->actionSave->setEnabled(true);       // menu item Save is enabled now
    ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

    // ensure that the selected row is still visible in the current songTable window
    selectedRow = selectedSongRow();
    ui->songTable->scrollToItem(ui->songTable->item(selectedRow, kNumberCol)); // EnsureVisible

    // mark playlist modified
    markPlaylistModified(true); // turn ON the * in the status bar
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemMoveUp() {
    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number

    if (currentNumberText == "") {
        // return, this is not an item that is on the playlist
        return;
    }

    int currentNumberInt = currentNumberText.toInt();

    if (currentNumberInt <= 1) {
        // return, if we're already at the top of the playlist
        return;
    }

    // Iterate over the entire songTable, find the item just above this one, and move IT down (only)
    ui->songTable->blockSignals(true); // while updating, do NOT call itemChanged

    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();  // this is the playlist #
        if (playlistIndex != "") {
            int playlistIndexInt = playlistIndex.toInt();
            if (playlistIndexInt == currentNumberInt - 1) {
                QString newIndex = QString::number(playlistIndex.toInt()+1);
                ui->songTable->item(i,kNumberCol)->setText(newIndex);
            }
        }
    }

    ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(currentNumberInt-1));  // this one moves UP
    ui->songTable->blockSignals(false); // done updating

    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    // moved an item, so we must enable saving of the playlist
    ui->actionSave->setEnabled(true);       // menu item Save is enabled now
    ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

    // ensure that the selected row is still visible in the current songTable window
    selectedRow = selectedSongRow();
    ui->songTable->scrollToItem(ui->songTable->item(selectedRow, kNumberCol)); // EnsureVisible

    // mark playlist modified
    markPlaylistModified(true); // turn ON the * in the status bar
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemMoveDown() {
    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number

    if (currentNumberText == "") {
        // return, this is not an item that is on the playlist
        return;
    }

    int currentNumberInt = currentNumberText.toInt();

    int playlistItemCount = PlaylistItemCount();

    if (currentNumberInt >= playlistItemCount) {
        // return, if we're already at the bottom of the playlist
        return;
    }

    // Iterate over the entire songTable, find the item just BELOW this one, and move it UP (only)
    ui->songTable->blockSignals(true); // while updating, do NOT call itemChanged

    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();  // this is the playlist #
        if (playlistIndex != "") {
            int playlistIndexInt = playlistIndex.toInt();
            if (playlistIndexInt == currentNumberInt + 1) {
                QString newIndex = QString::number(playlistIndex.toInt()-1);
                ui->songTable->item(i,kNumberCol)->setText(newIndex);
            }
        }
    }

    ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(currentNumberInt+1));  // this one moves UP
    ui->songTable->blockSignals(false); // done updating

    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    // moved an item, so we must enable saving of the playlist
    ui->actionSave->setEnabled(true);       // menu item Save is enabled now
    ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

    // ensure that the selected row is still visible in the current songTable window
    selectedRow = selectedSongRow();
    ui->songTable->scrollToItem(ui->songTable->item(selectedRow, kNumberCol)); // EnsureVisible

    // mark playlist modified
//    qDebug() << "marking modified...";
    markPlaylistModified(true); // turn ON the * in the status bar
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemRemove() {

    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
    int currentNumberInt = currentNumberText.toInt();

//    ui->songTable->blockSignals(true); // while updating, do NOT call itemChanged
    startLongSongTableOperation("PlaylistItemRemove");

    // already on the list
    // Iterate over the entire songTable, decrementing items BELOW this item
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndexText = theItem->text();  // this is the playlist #
        if (playlistIndexText != "") {
            int playlistIndexInt = playlistIndexText.toInt();
            if (playlistIndexInt > currentNumberInt) {
                // if a # was set and more, decrement it
                QString newIndex = QString::number(playlistIndexInt-1);
                ui->songTable->item(i,kNumberCol)->setText(newIndex);
            }
        }
    }
    // and then set this one to #LAST
    ui->songTable->item(selectedRow, kNumberCol)->setText("");  // this one is off the list

//    ui->songTable->blockSignals(false); // done updating
    stopLongSongTableOperation("PlaylistItemRemove");

    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    // removed an item, so we must enable saving of the playlist
    ui->actionSave->setEnabled(true);       // menu item Save is enabled now
    ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

    // mark playlist modified
    markPlaylistModified(true); // turn ON the * in the status bar
}

// LOAD RECENT PLAYLIST --------------------------------------------------------------------
void MainWindow::on_actionClear_Recent_List_triggered()
{
    QStringList recentFilePaths;  // empty list

    prefsManager.MySettings.setValue("recentFiles", recentFilePaths);  // remember the new list
    updateRecentPlaylistMenu();
}

void MainWindow::loadRecentPlaylist(int i) {

    on_stopButton_clicked();  // if we're loading a new PLAYLIST file, stop current playback

    QStringList recentFilePaths = prefsManager.MySettings.value("recentFiles").toStringList();

    if (i < recentFilePaths.size()) {
        // then we can do it
        QString filename = recentFilePaths.at(i);
        finishLoadingPlaylist(filename);

        addFilenameToRecentPlaylist(filename);
    }
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
    // return; // DEBUG
    int col = ((TableNumberItem *)item)->column();
    if (col != 0) {
        return;  // ignore all except # column
    }

    ui->songTable->setSortingEnabled(false); // disable sorting

    bool ok;
    QString itemText = ((TableNumberItem *)item)->text();
    int itemInteger = (itemText.toInt(&ok));
//    itemText.toInt(&ok);  // just sets ok or not, throw away the itemInteger intentionally
    int itemRow = ((TableNumberItem *)item)->row();

    if (itemInteger <= 0) {
        // user: don't try anything funny...
        itemText = "1";
        itemInteger = 1;
        item->setText("1");
    }

    if ((itemText == "") || (ok && itemInteger >= 1)) { // if item was deleted by clearing it out manually, or if it was an integer, then renumber
        // NOTE: the itemInteger >= 1 is just to turn off the warning about itemInteger not being read...
//        qDebug() << "tableItemChanged row/col: " << itemRow << "," << col << " = " << itemText;

        ui->songTable->blockSignals(true);  // block signals, so changes are not recursive

        // Step 1: count up how many of each number we have in the # column
        int numRows = ui->songTable->rowCount();
        int *count = new int[numRows]();   // initialized to zero
        int *renumberTo = new int[numRows]();
        int minNum = 1E6;
        int maxNum = -1000;
//        int numNums = 0;
        for (int i = 0; i < numRows; i++) {
            QTableWidgetItem *p = ui->songTable->item(i, kNumberCol); // get pointer to item
            QString s = p->text();
            if (s != "") {
                // if there's a number in this cell...
                int i1 = (s.toInt(&ok));
                count[i1]++;  // bump the count
                minNum = qMin(minNum, i1);
                maxNum = qMax(maxNum, i1);
//                numNums++;
            }
        }

        minNum = 1;  // this is true always, if everything is in order (which it should always be)

        // Step 2: figure out which numbers are doubled (if any) and which are missing (if any)
        int doubled = -1;
        int missing = -1;
        for (int i = minNum; i <= maxNum; i++) {
            switch (count[i]) {
                case 0: missing = i; break;
                case 1: break;
                case 2: doubled = i; break;
                default: break;
            }
        }

//        qDebug() << "Doubled: " << doubled << ", missing: " << missing;

        // Step 3: figure out what to renumber each of the existing numbers to, to
        //   have no dups and no gaps
//        qDebug() << "numNums,minNum,maxNum = " << numNums << minNum << maxNum;
        int j = 1;
        for (int i = minNum; i <= maxNum; i++) {
            switch (count[i]) {
                case 0: break;
                case 1: renumberTo[i] = j++; break;
                case 2: if ((doubled < missing) || (doubled == -1) || (missing == -1)) {
                            renumberTo[i] = ++j; j++; break; // moving UP the list or new item
                        } else {
                            renumberTo[i] = j; j += 2; break; // moving DOWN the list
                        }
                default: break;
            }

//            qDebug() << "  " << i << "=" << count[i] << " -> " << renumberTo[i];
        }

        // Step 4: renumber all the existing #'s as per the renumberTo table above
        for (int i = 0; i < ui->songTable->rowCount(); i++) {
            // iterate over all rows
            QTableWidgetItem *p = ui->songTable->item(i, kNumberCol); // get pointer to item
            QString s = p->text();
            if (s != "") {
                // if there's a number in this cell...
                int thisInteger = (s.toInt(&ok));
                if (itemRow != i) {
                    // NOT the one the user changed, so it can't be a dup
                    p->setText(QString::number(renumberTo[thisInteger]));
                } else {
                    if (count[thisInteger] == 2) {
                        // dup, so what the user typed in was correct, do nothing
                    } else {
                        // NOT a dup, so what the user typed in might NOT be correct, so renumber it
                        p->setText(QString::number(renumberTo[thisInteger]));
                    }
                }
            }
        }
        ui->songTable->blockSignals(false);

        delete[] count;
        delete[] renumberTo;
    } else {
        item->setText(""); // if it's not a number, set to null string
    }

    ui->songTable->setSortingEnabled(true); // re-enable sorting

    // mark playlist modified
    markPlaylistModified(true); // turn ON the * in the status bar, because some # entry changed

    // TODO: remove any non-numeric chars from the string before updating
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
    // Playlist > Print Playlist...
    QPrinter printer;
    QPrintDialog printDialog(&printer, this);
    printDialog.setWindowTitle("Print Playlist");

    if (printDialog.exec() == QDialog::Rejected) {
        return;
    }

    // --------
    QList<PlaylistExportRecord> exports;

    // Iterate over the songTable to get all the info about the playlist
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();
        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
        //            QString songTitle = getTitleColTitle(ui->songTable, i);
        QString pitch = ui->songTable->item(i,kPitchCol)->text();
        QString tempo = ui->songTable->item(i,kTempoCol)->text();

        if (playlistIndex != "") {
            // item HAS an index (that is, it is on the list, and has a place in the ordering)
            // TODO: reconcile int here with double elsewhere on insertion
            PlaylistExportRecord rec;
            rec.index = playlistIndex.toInt();
            //            rec.title = songTitle;
            rec.title = pathToMP3;  // NOTE: this is an absolute path that does not survive moving musicDir
            rec.pitch = pitch;
            rec.tempo = tempo;
            exports.append(rec);
        }
    }

    std::sort(exports.begin(), exports.end(), comparePlaylistExportRecord);  // they are now IN INDEX ORDER

    // GET SHORT NAME FOR PLAYLIST ---------
    static QRegularExpression regex1 = QRegularExpression(".*/(.*).csv$");
    QRegularExpressionMatch match = regex1.match(lastSavedPlaylist);
    QString shortPlaylistName("ERROR 7");

    if (match.hasMatch())
    {
        shortPlaylistName = match.captured(1);
    }

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
        QString folderTypename = parts[1]; // /patter/foo.mp3 --> "patter"
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
