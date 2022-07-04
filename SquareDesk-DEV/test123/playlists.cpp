// PLAYLIST MANAGEMENT -------------------------------

// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"
#include "utility.h"
#include "songlistmodel.h"

// FORWARD DECLS ---------
extern QString getTitleColTitle(MyTableWidget *songTable, int row);

#ifndef M1MAC
// All other platforms:
extern bass_audio cBass;  // make this accessible to PreferencesDialog
#else
// M1 Silicon Mac only:
extern flexible_audio cBass;  // make this accessible to PreferencesDialog
#endif

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

        int lineCount = 1;
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
                            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
                            theItem->setText(QString::number(songCount));

                            QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);
                            theItem2->setText(list1[1].trimmed());

                            QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);
                            theItem3->setText(list1[2].trimmed());

                            match = true;
                        }
                    }
                    // if we had no match, remember the first non-matching song path
                    if (!match && firstBadSongLine == "") {
                        firstBadSongLine = line;
                    }

                }

                lineCount++;
            } // while
        }
        else {
            // M3U FILE =================================
            // to workaround old-style Mac files that have a bare "\r" (which readLine() can't handle)
            //   in particular, iTunes exported playlists use this old format.
            QStringList theWholeFile = in.readAll().replace("\r\n","\n").replace("\r","\n").split("\n");

            foreach (const QString &line, theWholeFile) {
//                qDebug() << "line:" << line;
                if (line == "#EXTM3U") {
                    // ignore, it's the first line of the M3U file
                }
                else if (line == "") {
                    // ignore, it's a blank line
                }
                else if (line.at( 0 ) == '#' ) {
                    // it's a comment line
                    if (line.mid(0,7) == "#EXTINF") {
                        // it's information about the next line, ignore for now.
                    }
                }
                else {
                    songCount++;  // it's a real song path

                    bool match = false;
                    // exit the loop early, if we find a match
                    for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {
                        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
//                        qDebug() << "pathToMP3:" << pathToMP3;
                        if (line == pathToMP3) { // FIX: this is fragile, if songs are moved around
                            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
                            theItem->setText(QString::number(songCount));

                            match = true;
                        }
                    }
                    // if we had no match, remember the first non-matching song path
                    if (!match && firstBadSongLine == "") {
                        firstBadSongLine = line;
                    }

                }

                lineCount++;

            } // for each line in the M3U file

        } // else M3U file

        inputFile.close();
        linesInCurrentPlaylist += songCount; // when non-zero, this enables saving of the current playlist
//        qDebug() << "linesInCurrentPlaylist:" << linesInCurrentPlaylist;

//        qDebug() << "FBS:" << firstBadSongLine << ", linesInCurrentPL:" << linesInCurrentPlaylist;
        if (firstBadSongLine=="" && linesInCurrentPlaylist != 0) {
            // a playlist is now loaded, NOTE: side effect of loading a playlist is enabling Save/SaveAs...
            ui->actionSave->setEnabled(false);  // save playlist is disabled, because we haven't changed it yet
            ui->actionSave_As->setEnabled(true);  // save playlist as...
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
    if (firstBadSongLine == "") {
        ui->songTable->selectRow(0); // select first row of newly loaded and sorted playlist!
        on_actionPrevious_Playlist_Item_triggered();
    }

    stopLongSongTableOperation("finishLoadingPlaylist"); // for performance measurements, sorting on again and show

    QString msg1 = QString("Loaded playlist with ") + QString::number(songCount) + QString(" items.");
    if (firstBadSongLine != "") {
        // if there was a non-matching path, tell the user what the first one of those was
        msg1 = QString("ERROR: could not find '...") + firstBadSongLine + QString("'");
        ui->songTable->clearSelection(); // select nothing, if error
    }
    ui->statusBar->showMessage(msg1);
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
                                     tr("Playlist Files (*.m3u *.csv)"));
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

    bool savingToM3U = PlaylistFileName.endsWith(".m3u", Qt::CaseInsensitive);

    // Iterate over the songTable
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();
        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
        QString songTitle = getTitleColTitle(ui->songTable, i);
        QString pitch = ui->songTable->item(i,kPitchCol)->text();
        QString tempo = ui->songTable->item(i,kTempoCol)->text();

        if (playlistIndex != "") {
            // item HAS an index (that is, it is on the list, and has a place in the ordering)
            // TODO: reconcile int here with double elsewhere on insertion
            PlaylistExportRecord rec;
            rec.index = playlistIndex.toInt();
            if (savingToM3U) {
                rec.title = pathToMP3;  // NOTE: M3U must remain absolute
                                        //    it will NOT survive moving musicDir or sharing it via Dropbox or Google Drive
            } else {
                rec.title = pathToMP3.replace(musicRootPath, "");  // NOTE: this is now a relative (to the musicDir) path
                                                                   //    that should survive moving musicDir or sharing it via Dropbox or Google Drive
            }
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

    filewatcherShouldIgnoreOneFileSave = true;  // I don't know why we have to do this, but rootDir is being watched, which causes this to be needed.
    QFile file(PlaylistFileName);
    if (PlaylistFileName.endsWith(".m3u", Qt::CaseInsensitive)) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {  // delete, if it exists already
            QTextStream stream(&file);
            stream << "#EXTM3U" << ENDL << ENDL;

            // list is auto-sorted here
            foreach (const PlaylistExportRecord &rec, exports)
            {
                stream << "#EXTINF:-1," << ENDL;  // nothing after the comma = no special name
                stream << rec.title << ENDL;
            }
            file.close();
            addFilenameToRecentPlaylist(PlaylistFileName);  // add to the MRU list
        }
        else {
            ui->statusBar->showMessage(QString("ERROR: could not open M3U file."));
        }
    }
    else if (PlaylistFileName.endsWith(".csv", Qt::CaseInsensitive)) {
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
        }
        else {
            ui->statusBar->showMessage(QString("ERROR: could not open CSV file."));
        }
    }
}

void MainWindow::savePlaylistAgain() // saves without asking for a filename
{
//    on_stopButton_clicked();  // if we're saving a new PLAYLIST file, stop current playback

    if (lastSavedPlaylist == "") {
        // nothing saved yet!
//        qDebug() << "NOTHING SAVED YET.";
        return;  // so just return without saving anything
    }

//    qDebug() << "OK, SAVED TO: " << lastSavedPlaylist;

    // else use lastSavedPlaylist
    saveCurrentPlaylistToFile(lastSavedPlaylist);  // SAVE IT

    // TODO: if there are no songs specified in the playlist (yet, because not edited, or yet, because
    //   no playlist was loaded), Save Playlist... should be greyed out.
    QString basefilename = lastSavedPlaylist.section("/",-1,-1);
//    qDebug() << "Basefilename: " << basefilename;
    ui->statusBar->showMessage(QString("Playlist saved as '") + basefilename + "'");
    // no need to remember it here, it's already the one we remembered.

//    ui->actionSave->setEnabled(false);  // once saved, this is not reenabled, until you change it.
}

// TODO: strip off the root directory before saving...
void MainWindow::on_actionSave_Playlist_triggered()  // NOTE: this is really misnamed, it's Save As.
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

    QString startHere = startingPlaylistDirectory + "/playlist.csv";  // if we haven't saved yet
    if (lastSavedPlaylist != "") {
        startHere = lastSavedPlaylist;  // if we HAVE saved already, default to same file
    }

    QString PlaylistFileName =
        QFileDialog::getSaveFileName(this,
                                     tr("Save Playlist"),
                                     startHere,
                                     tr("M3U playlists (*.m3u);;CSV files (*.csv)"),
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
    QString basefilename = PlaylistFileName.section("/",-1,-1);
//    qDebug() << "Basefilename: " << basefilename;
    ui->statusBar->showMessage(QString("Playlist saved as '") + basefilename + "'");

    lastSavedPlaylist = PlaylistFileName; // remember it, for the next SAVE operation (defaults to last saved in this session)

    ui->actionSave->setEnabled(true);  // now that we have Save As'd something, we can now Save that thing
    ui->actionSave->setText(QString("Save Playlist") + " '" + basefilename + "'"); // and now it has a name
}

void MainWindow::on_actionNext_Playlist_Item_triggered()
{
    // This code is similar to the row double clicked code...
    on_stopButton_clicked();  // if we're going to the next file in the playlist, stop current playback
    saveCurrentSongSettings();

    // figure out which row is currently selected
    QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
        return;
    }

    int maxRow = ui->songTable->rowCount() - 1;

    // which is the next VISIBLE row?
    int lastVisibleRow = row;
    row = (maxRow < row+1 ? maxRow : row+1); // bump up by 1
    while (ui->songTable->isRowHidden(row) && row < maxRow) {
        // keep bumping, until the next VISIBLE row is found, or we're at the END
        row = (maxRow < row+1 ? maxRow : row+1); // bump up by 1
    }
    if (ui->songTable->isRowHidden(row)) {
        // if we try to go past the end of the VISIBLE rows, stick at the last visible row (which
        //   was the last one we were on.  Well, that's not always true, but this is a quick and dirty
        //   solution.  If I go to a row, select it, and then filter all rows out, and hit one of the >>| buttons,
        //   hilarity will ensue.
        row = lastVisibleRow;
    }
    ui->songTable->selectRow(row); // select new row!

    // load all the UI fields, as if we double-clicked on the new row
    QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();
    QString songTitle = getTitleColTitle(ui->songTable, row);
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,kTypeCol)->text();
    QString songLabel = ui->songTable->item(row,kLabelCol)->text();

//    // must be up here...
//    QString pitch = ui->songTable->item(row,kPitchCol)->text();
//    QString tempo = ui->songTable->item(row,kTempoCol)->text();

    loadMP3File(pathToMP3, songTitle, songType, songLabel);

//    // must be down here...
//    int pitchInt = pitch.toInt();
//    ui->pitchSlider->setValue(pitchInt);

//    if (tempo != "0") {
//        QString tempo2 = tempo.replace("%",""); // get rid of optional "%", slider->setValue will do the right thing
//        int tempoInt = tempo2.toInt();
//        ui->tempoSlider->setValue(tempoInt);
//    }

    if (ui->actionAutostart_playback->isChecked()) {
        on_playButton_clicked();
    }

}

void MainWindow::on_actionPrevious_Playlist_Item_triggered()
{
    cBass.StreamGetPosition();  // get ready to look at the playhead position
    if (cBass.Current_Position != 0.0) {
        on_stopButton_clicked();  // if the playhead was not at the beginning, just stop current playback and go to START
        return;                   //   and return early... (don't save the current song settings)
    }
    // else, stop and move to the previous song...

    on_stopButton_clicked();  // if we were playing, just stop current playback
    // This code is similar to the row double clicked code...
    saveCurrentSongSettings();

    QItemSelectionModel *selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
        return;
    }

    // which is the next VISIBLE row?
    int lastVisibleRow = row;
    row = (row-1 < 0 ? 0 : row-1); // bump backwards by 1

    while (ui->songTable->isRowHidden(row) && row > 0) {
        // keep bumping backwards, until the previous VISIBLE row is found, or we're at the BEGINNING
        row = (row-1 < 0 ? 0 : row-1); // bump backwards by 1
    }
    if (ui->songTable->isRowHidden(row)) {
        // if we try to go past the beginning of the VISIBLE rows, stick at the first visible row (which
        //   was the last one we were on.  Well, that's not always true, but this is a quick and dirty
        //   solution.  If I go to a row, select it, and then filter all rows out, and hit one of the >>| buttons,
        //   hilarity will ensue.
        row = lastVisibleRow;
    }

    ui->songTable->selectRow(row); // select new row!

    // load all the UI fields, as if we double-clicked on the new row
    QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();
    QString songTitle = getTitleColTitle(ui->songTable, row);
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,kTypeCol)->text();
    QString songLabel = ui->songTable->item(row,kLabelCol)->text();

    // must be up here...
    QString pitch = ui->songTable->item(row,kPitchCol)->text();
    QString tempo = ui->songTable->item(row,kTempoCol)->text();

    loadMP3File(pathToMP3, songTitle, songType, songLabel);

    // must be down here...
    int pitchInt = pitch.toInt();
    ui->pitchSlider->setValue(pitchInt);

// not sure this is needed anymore...test with next song, previous song to be sure
//    if (tempo != "0") {
//        QString tempo2 = tempo.replace("%",""); // get rid of optional "%", setValue will take care of it
//        int tempoInt = tempo.toInt();
//        ui->tempoSlider->setValue(tempoInt);
//    }

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

    // Iterate over the songTable
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        theItem->setText(""); // clear out the current list

        // let's intentionally NOT clear the pitches.  They are persistent within a session.
        // let's intentionally NOT clear the tempos.  They are persistent within a session.
    }

    linesInCurrentPlaylist = 0;
    ui->actionSave->setDisabled(true);
    ui->actionSave_As->setDisabled(true);

    sortByDefaultSortOrder();

    stopLongSongTableOperation("on_actionClear_Playlist_triggered");  // for performance, sorting on again and show

    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled
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

    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
    int currentNumberInt = currentNumberText.toInt();

    if (currentNumberText == "") {
        // add to list, and make it the #1

        // Iterate over the entire songTable, incrementing every item
        // TODO: turn off sorting
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
        // TODO: turn on sorting again

    } else {
        // already on the list
        // Iterate over the entire songTable, incrementing items BELOW this item
        // TODO: turn off sorting
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

    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    // moved an item, so we must enable saving of the playlist
    ui->actionSave->setEnabled(true);       // menu item Save is enabled now
    ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

    // ensure that the selected row is still visible in the current songTable window
    selectedRow = selectedSongRow();
    ui->songTable->scrollToItem(ui->songTable->item(selectedRow, kNumberCol)); // EnsureVisible
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

    if (currentNumberText == "") {
        // add to list, and make it the bottom

        // Iterate over the entire songTable, not touching every item
        // TODO: turn off sorting
        ui->songTable->item(selectedRow, kNumberCol)->setText(QString::number(playlistItemCount+1));  // this one is the new #LAST
        // TODO: turn on sorting again

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
    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    // moved an item, so we must enable saving of the playlist
    ui->actionSave->setEnabled(true);       // menu item Save is enabled now
    ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

    // ensure that the selected row is still visible in the current songTable window
    selectedRow = selectedSongRow();
    ui->songTable->scrollToItem(ui->songTable->item(selectedRow, kNumberCol)); // EnsureVisible
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemMoveUp() {
    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
    int currentNumberInt = currentNumberText.toInt();

    // Iterate over the entire songTable, find the item just above this one, and move IT down (only)
    // TODO: turn off sorting

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
    // TODO: turn on sorting again
    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    // moved an item, so we must enable saving of the playlist
    ui->actionSave->setEnabled(true);       // menu item Save is enabled now
    ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

    // ensure that the selected row is still visible in the current songTable window
    selectedRow = selectedSongRow();
    ui->songTable->scrollToItem(ui->songTable->item(selectedRow, kNumberCol)); // EnsureVisible
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemMoveDown() {
    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
    int currentNumberInt = currentNumberText.toInt();

    // add to list, and make it the bottom

    // Iterate over the entire songTable, find the item just BELOW this one, and move it UP (only)
    // TODO: turn off sorting

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
    // TODO: turn on sorting again
    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    // moved an item, so we must enable saving of the playlist
    ui->actionSave->setEnabled(true);       // menu item Save is enabled now
    ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now

    // ensure that the selected row is still visible in the current songTable window
    selectedRow = selectedSongRow();
    ui->songTable->scrollToItem(ui->songTable->item(selectedRow, kNumberCol)); // EnsureVisible
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemRemove() {
    int selectedRow = selectedSongRow();  // get current row or -1

    if (selectedRow == -1) {
        return;
    }

    QString currentNumberText = ui->songTable->item(selectedRow, kNumberCol)->text();  // get current number
    int currentNumberInt = currentNumberText.toInt();

    // already on the list
    // Iterate over the entire songTable, decrementing items BELOW this item
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
    ui->songTable->item(selectedRow, kNumberCol)->setText("");  // this one is off the list
    on_songTable_itemSelectionChanged();  // reevaluate which menu items are enabled

    // removed an item, so we must enable saving of the playlist
    ui->actionSave->setEnabled(true);       // menu item Save is enabled now
    ui->actionSave_As->setEnabled(true);    // menu item Save as... is also enabled now
}
