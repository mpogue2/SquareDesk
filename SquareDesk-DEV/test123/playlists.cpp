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


// ----------------------------------------------------------------------
void MainWindow::PlaylistItemsToTop() {
    if (darkmode) {
        if (!relPathInSlot[0].startsWith("/tracks/")) {
            slotModified[0] = ui->playlist1Table->moveSelectedItemsToTop() || slotModified[0];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[0]) {
                saveSlotNow(0);
            }
        }
        if (!relPathInSlot[1].startsWith("/tracks/")) {
            slotModified[1] = ui->playlist2Table->moveSelectedItemsToTop() || slotModified[1];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[1]) {
                saveSlotNow(1);
            }
        }
        if (!relPathInSlot[2].startsWith("/tracks/")) {
            slotModified[2] = ui->playlist3Table->moveSelectedItemsToTop() || slotModified[2];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[2]) {
                saveSlotNow(2);
            }
        }
        if (slotModified[0] || slotModified[1] || slotModified[2]) {
//            playlistSlotWatcherTimer->start(std::chrono::seconds(10));  // no need for this, because these changes are immediate now
            return; // we changed something, no need to check songTable
        }
    }
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemsToBottom() {
    if (darkmode) {
        if (!relPathInSlot[0].startsWith("/tracks/")) {
            slotModified[0] = ui->playlist1Table->moveSelectedItemsToBottom() || slotModified[0];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[0]) {
                saveSlotNow(0);
            }
        }
        if (!relPathInSlot[1].startsWith("/tracks/")) {
            slotModified[1] = ui->playlist2Table->moveSelectedItemsToBottom() || slotModified[1];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[1]) {
                saveSlotNow(1);
            }
        }
        if (!relPathInSlot[2].startsWith("/tracks/")) {
            slotModified[2] = ui->playlist3Table->moveSelectedItemsToBottom() || slotModified[2];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[2]) {
                saveSlotNow(2);
            }
        }
        if (slotModified[0] || slotModified[1] || slotModified[2]) {
            // playlistSlotWatcherTimer->start(std::chrono::seconds(10));  // not needed now
            return; // we changed something, no need to check songTable
        }
    }
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemsMoveUp() {

    if (darkmode) {
        // qDebug() << "relPathInSlot: " << relPathInSlot[0] << relPathInSlot[1] << relPathInSlot[2];
        if (!relPathInSlot[0].startsWith("/tracks/")) {
            slotModified[0] = ui->playlist1Table->moveSelectedItemsUp() || slotModified[0];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[0]) {
                saveSlotNow(0);
            }
        }
        if (!relPathInSlot[1].startsWith("/tracks/")) {
            slotModified[1] = ui->playlist2Table->moveSelectedItemsUp() || slotModified[1];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[1]) {
                saveSlotNow(1);
            }
        }
        if (!relPathInSlot[2].startsWith("/tracks/")) {
            slotModified[2] = ui->playlist3Table->moveSelectedItemsUp() || slotModified[2];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[2]) {
                saveSlotNow(2);
            }
        }
        if (slotModified[0] || slotModified[1] || slotModified[2]) {
            // playlistSlotWatcherTimer->start(std::chrono::seconds(10));  // not needed now
            return; // we changed something, no need to check songTable
        }
    }
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemsMoveDown() {
    if (darkmode) {
        if (!relPathInSlot[0].startsWith("/tracks/")) {
            slotModified[0] = ui->playlist1Table->moveSelectedItemsDown() || slotModified[0];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[0]) {
                saveSlotNow(0);
            }
        }
        if (!relPathInSlot[1].startsWith("/tracks/")) {
            slotModified[1] = ui->playlist2Table->moveSelectedItemsDown() || slotModified[1];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[1]) {
                saveSlotNow(1);
            }
        }
        if (!relPathInSlot[2].startsWith("/tracks/")) {
            slotModified[2] = ui->playlist3Table->moveSelectedItemsDown() || slotModified[2];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[2]) {
                saveSlotNow(2);
            }
        }
        if (slotModified[0] || slotModified[1] || slotModified[2]) {
            // playlistSlotWatcherTimer->start(std::chrono::seconds(10));  // not needed now
            return; // we changed something, no need to check songTable
        }
    }
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemsRemove() {

    if (darkmode) {
        if (!relPathInSlot[0].startsWith("/tracks/")) {
            slotModified[0] = ui->playlist1Table->removeSelectedItems() || slotModified[0];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[0]) {
                saveSlotNow(0);
            }
        }
        if (!relPathInSlot[1].startsWith("/tracks/")) {
            slotModified[1] = ui->playlist2Table->removeSelectedItems() || slotModified[1];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[1]) {
                saveSlotNow(1);
            }
        }
        if (!relPathInSlot[2].startsWith("/tracks/")) {
            slotModified[2] = ui->playlist3Table->removeSelectedItems() || slotModified[2];  // if nothing was selected in this slot, this call will do nothing
            if (slotModified[2]) {
                saveSlotNow(2);
            }
        }

        if (slotModified[0] || slotModified[1] || slotModified[2]) {
            // playlistSlotWatcherTimer->start(std::chrono::seconds(10));  // not needed now
            return; // we changed something, no need to check songTable
        }

            return;
    }
}

// ============================================================================================================
void MainWindow::setTitleField(QTableWidget *whichTable, int whichRow, QString relativePath,
                               bool isPlaylist, QString PlaylistFileName, QString theRealPath) {

    // relativePath is relative to the MusicRootPath, and it might be fake (because it was reversed, like Foo Bar - RIV123.mp3,
    //     we reversed it for presentation
    // theRealPath is the actual relative path, used to determine whether the file exists or not

    bool isAppleMusicFile = theRealPath.contains("/iTunes/iTunes Media/");
    bool isDarkSongTable = (whichTable == ui->darkSongTable);

    static QRegularExpression dotMusicSuffix("\\.(mp3|m4a|wav|flac)$", QRegularExpression::CaseInsensitiveOption); // match with music extensions
    QString shortTitle = relativePath.split('/').last().replace(dotMusicSuffix, "");

    // qDebug() << "setTitleField:" << isPlaylist << whichRow << relativePath << PlaylistFileName << theRealPath << shortTitle;

    // example:
    // list1[0] = "/singing/BS 2641H - I'm Beginning To See The Light.mp3"
    // type = "singing"
    QString cType = relativePath.split('/').at(1).toLower();
    // qDebug() << "cType:" << cType;

    QColor textCol; // = QColor::fromRgbF(0.0/255.0, 0.0/255.0, 0.0/255.0);  // defaults to Black
    if (songTypeNamesForExtras.contains(cType) || isAppleMusicFile) {  // special case for Apple Music items
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
    QString appleSymbol = QChar(0xF8FF);
    if (isAppleMusicFile && !isDarkSongTable) {
        shortTitle = appleSymbol + " " + shortTitle; // add Apple space as prefix to actual short title
    }

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
    if (theRealPath != "" && !isAppleMusicFile) { // special case for Apple Music items
        // real path specified, so use it to point at the real file
        realPath = musicRootPath + theRealPath;
    }

    // DDD(realPath)

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
                            // qDebug() << "Tried to revealAttachedLyricsFile, but could not get settings for: " << fullMP3Path;
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
                QString fakePath = "/" + categoryName + "/" + label + " - " + shortTitle;
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

                QString appleSymbol = QChar(0xF8FF); // TODO: factor into global?
                QString shortTitle = sl[1];
                shortTitle = appleSymbol + " " + shortTitle;
                setTitleField(theTableWidget, songCount-1, "/xtras/" + shortTitle, false, sl[1]); // whichTable, whichRow, relativePath or pre-colored title, bool isPlaylist, PlaylistFilename (for errors and for filters it's colored)

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

                    // QFileInfo fi(list1[0]);
                    QFileInfo fi(absPath);
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

                    if (absPath.contains("/iTunes/iTunes Media/")) {
                        theFakePath = absPath;
                    }

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

        // addFilenameToRecentPlaylist(PlaylistFileName); // remember it in the Recents menu, full absolute pathname

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
    // qDebug() << "fullFilePath Save Playlist As: " << fullFilePath;

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

        updateRecentPlaylistsList(fullFilePath); // remember this one in the "recently-used playlists" menu
    } else {
        ui->statusBar->showMessage(QString("ERROR: could not save playlist to CSV file."));
    }

}

// -----------
// DARK MODE: NO file dialog, just save slot to that same file where it came from
// SAVE a playlist in a slot to a CSV file

void MainWindow::saveSlotNow(MyTableWidget *mtw) {
    if (mtw == ui->playlist1Table) {
        saveSlotNow(0);
    } else if (mtw == ui->playlist2Table) {
        saveSlotNow(1);
    } else if (mtw == ui->playlist3Table) {
        saveSlotNow(2);
    }
    return;
}

void MainWindow::saveSlotNow(int whichSlot) {

    // qDebug() << "saveSlotNow" << whichSlot;

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
    QString theTableLabelText = "";
    switch (whichSlot) {
        case 1: theTableWidget = ui->playlist2Table;
                theTableLabelText = ui->playlist2Label->text();
                break;
        case 2: theTableWidget = ui->playlist3Table;
                theTableLabelText = ui->playlist3Label->text();
                break;
        case 0:
        default: theTableWidget = ui->playlist1Table;
                theTableLabelText = ui->playlist1Label->text();
                break;

    }

    QString playlistShortName = PlaylistFileName;
    playlistShortName = playlistShortName.replace(musicRootPath,"").split('/').last().replace(".csv",""); // PlaylistFileName is now altered
//    qDebug() << "playlistShortName: " << playlistShortName;

    // ACTUAL SAVE TO FILE ============
    filewatcherShouldIgnoreOneFileSave = true;  // I don't know why we have to do this, but rootDir is being watched, which causes this to be needed.

    QFile file(PlaylistFileName);

    if (PlaylistFileName.contains("/Apple Music/", Qt::CaseInsensitive)) {
        // Apple Music playlists are not SquareDesk CSV playlists, so they don't need to be saved
        return;
    }

    // qDebug() << "***** Saving playlist: " << PlaylistFileName;

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
        qDebug() << "ERROR: could not save playlist to CSV file: " << relPathInSlot[whichSlot];
    }

    // special case:  If a CSV playlist is in a Palette Slot, and also is visible as a playlist in the darkSongTable,
    //   we need to reload the darkSongTable when the Palette Slot is changed (10s after the last change).

    // qDebug() << "saveSlotNow: currentTreePath = " << currentTreePath; // e.g. "Playlists/Jokers/2025/"
    // qDebug() << "saveSlotNow: header title = " << theTableLabelText; // e.g. "<img src=\":/graphics/icons8-menu-64.png\" width=\"10\" height=\"9\">Jokers/2025/Test_2025.05.22"

    if (currentTreePath.startsWith("Playlists/")) {
        QString ctp = currentTreePath;
        ctp.replace(QRegularExpression("^Playlists/"), ""); // e.g. "Jokers/2025/" or "Jokers/2025/Test_2025.05.22"
        QString headerTitle = theTableLabelText.replace(QRegularExpression("^<img.*>"), "");
        // qDebug() << "ctp and headerTitle:" << ctp << headerTitle;
        if (headerTitle.startsWith(ctp)) {
            // it's a playlist, and the title of the modified playlist would be visible in the darkSongTable right now
            // qDebug() << "***** NEED TO RELOAD THE SONG TABLE: ";

            // // before removal
            // for (const auto &item : *pathStackPlaylists) {
            //     qDebug() << "before: " << item;
            // }

            // first we get rid of the old playlist items in pathStackPlaylists
            // items look like e.g. "Jokers/2025/Test_2025.05.22%!%-3,122,01#!#/Users/mpogue/Library/Mobile Documents/com~apple~CloudDocs/SquareDance/squareDanceMusic_iCloud/patter/ESP 451 - 'Cuda.mp3"
            pathStackPlaylists->removeIf([headerTitle](const QString& path) {
                                            return path.startsWith(headerTitle + "%!%"); // e.g. "Jokers/2025/Test_2025.05.22%!%"
                                        });

            // then we stick in the new playlist
            // for (const auto &item : *pathStackPlaylists) {
            //     qDebug() << "after 1: " << item;
            // }

            for (int i = 0; i < theTableWidget->rowCount(); i++) {
                // what we need to append to pathStackPlaylists is an item like:
                //   "Jokers/2025/Test_2025.05.22%!%-3,122,01#!#/Users/mpogue/Library/Mobile Documents/com~apple~CloudDocs/SquareDance/squareDanceMusic_iCloud/patter/ESP 451 - 'Cuda.mp3"
                QString itemNumber = QString::number(i+1);
                if (itemNumber.length() < 2) {
                    itemNumber = "0" + itemNumber; // 9 --> 09
                }
                QString path = theTableWidget->item(i, 4)->text();
                QString pitch = theTableWidget->item(i, 2)->text();
                QString tempo = theTableWidget->item(i, 3)->text();

                QString newItem = headerTitle + "%!%" + pitch + "," + tempo + "," + itemNumber + "#!#" + path;
                // qDebug() << "newItem:" << newItem;
                pathStackPlaylists->append(newItem);
            }

            // for (const auto &item : *pathStackPlaylists) {
            //     qDebug() << "after 2: " << item;
            // }
            // then finally we force a refresh of the darkSongTable
            darkLoadMusicList(pathStackPlaylists, currentTypeFilter, true, false, true);  // refresh and force same filter, and YES suppress focus/selection change
        }
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
    
    // Update the recent playlists list with the newly loaded playlist
    // Only update for real playlist files (not track filters or Apple Music playlists)
    if (PlaylistFileName.endsWith(".csv") && 
        !PlaylistFileName.contains("/Tracks/") && 
        !PlaylistFileName.contains("/Apple Music/")) {
        updateRecentPlaylistsList(PlaylistFileName);
    }
}

// ----------------------------
void MainWindow::updateRecentPlaylistsList(const QString &playlistPath)
{
    // Convert full path to relative path for storage
    QString relativePath = playlistPath;
    QString playlistsPrefix = musicRootPath + "/playlists/";
    
    // Remove the musicRootPath + "/playlists/" prefix and ".csv" suffix
    if (relativePath.startsWith(playlistsPrefix)) {
        relativePath = relativePath.mid(playlistsPrefix.length());
    }
    if (relativePath.endsWith(".csv")) {
        relativePath = relativePath.left(relativePath.length() - 4);
    }
    
    QString currentList = prefsManager.GetlastNPlaylistsLoaded();
    QStringList recentPlaylists;
    
    if (!currentList.isEmpty()) {
        recentPlaylists = currentList.split(";", Qt::SkipEmptyParts);
    }
    
    // Remove the playlist if it already exists in the list
    recentPlaylists.removeAll(relativePath);
    
    // Add the new playlist to the front of the list
    recentPlaylists.prepend(relativePath);
    
    // Keep only the last 5 playlists
    while (recentPlaylists.size() > 5) {
        recentPlaylists.removeLast();
    }
    
    // Save the updated list
    QString updatedList = recentPlaylists.join(";");
    prefsManager.SetlastNPlaylistsLoaded(updatedList);
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

QString MainWindow::makeCanonicalRelativePath(QString s) {
    // input:  "/patter/Hello Test Backwards - AAA 102.mp3"
    // output: "/patter/AAA 102 - Hello Test Backwards.mp3"
    //
    // input:  "/patter/AAA 102 - Hello Test Canonical.mp3"
    // output: "/patter/AAA 102 - Hello Test Canonical.mp3"

    // qDebug() << "makeCanonicalRelativePath:" << s;

    QStringList sl1 = s.split("/");
    int lastItem = sl1.count() - 1;

    QFileInfo fi1(sl1[lastItem]);
    QString name = fi1.completeBaseName();
    QString suffix = fi1.suffix();

    // qDebug() << "suffix:" << suffix;

    // NOTE: THIS NEEDS TO MATCH STEP 5 IN MP3FilenameVsCuesheetnameScore()
    //   TODO: Factor this code out into a separate function
    //
    // Step 5: Parse the filenames to extract components
    // Try to identify label, label number, label extra, and title
    struct ParsedName {
        QString label;
        QString labelNum;
        QString labelExtra;
        QString title;
        QString canonicalTitle;
        bool reversed = false;
    };

    ParsedName result;

    // Try standard format: LABEL NUM[EXTRA] - TITLE
    QRegularExpression stdFormat("^([A-Za-z ]{1,20})\\s*([0-9]{1,5})([A-Za-z]{0,4})?\\s*-\\s*(.+)$",
                                 QRegularExpression::CaseInsensitiveOption);

    // Try reversed format: TITLE - LABEL NUM[EXTRA]
    QRegularExpression revFormat("^(.+)\\s*-\\s*([A-Za-z ]{1,20})\\s*([0-9]{1,5})([A-Za-z]{0,4})?$",
                                 QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatch match = stdFormat.match(name);
    if (match.hasMatch()) {
        // NORMAL ORDER, e.g. "AAA 102 - Foo Bar.mp3"
        result.label = match.captured(1);
        result.labelNum = match.captured(2);
        result.labelExtra = match.captured(3);
        result.title = match.captured(4);
        result.reversed = false;
        result.canonicalTitle = s; // no change
        // qDebug() << "makeCanonicalRelativePath: NORMAL ORDER: " << name << result.canonicalTitle;
    } else {
        match = revFormat.match(name);
        if (match.hasMatch()) {
            // REVERSED ORDER, e.g. "Foo Bar - AAA 102.mp3"
            result.title = match.captured(1);
            result.label = match.captured(2);
            result.labelNum = match.captured(3);
            result.labelExtra = match.captured(4);
            result.reversed = true;
            sl1[lastItem] = result.label + result.labelNum + " - " + result.title.simplified() + "." + suffix;
            result.canonicalTitle = sl1.join("/");
            // qDebug() << "makeCanonicalRelativePath: REVERSED ORDER: " << result.canonicalTitle;
        } else {
            // If no match, just consider the whole thing as title
            result.canonicalTitle = s;
        }
    }

    return(result.canonicalTitle);
}

void MainWindow::darkAddPlaylistItemAt(int whichSlot, const QString &trackName, const QString &thePitch, const QString &theTempo, const QString &theFullPath, const QString &extra, int insertRowNum) {
    Q_UNUSED(trackName)
    Q_UNUSED(extra)

    // qDebug() << "darkAddPlaylistItemAt:" << whichSlot << trackName << extra << theFullPath << insertRowNum;

    MyTableWidget *destTableWidget;
    QString PlaylistFileName = "foobar";
    switch (whichSlot) {
    default:
    case 0: destTableWidget = ui->playlist1Table; break;
    case 1: destTableWidget = ui->playlist2Table; break;
    case 2: destTableWidget = ui->playlist3Table; break;
    }

    // make a new row, after all the other ones, and fill it in with the new info
    destTableWidget->insertRow(insertRowNum); // always make a new row
    int songCount = destTableWidget->rowCount();

    // # column
    QTableWidgetItem *num = new TableNumberItem(QString::number(insertRowNum+1)); // use TableNumberItem so that it sorts numerically
    destTableWidget->setItem(insertRowNum, 0, num);

    // Now we have to renumber all the rows BELOW insertRowNum, to add 1 to them
    for (int i = insertRowNum+1; i < songCount; i++) {
        int currentNum = destTableWidget->item(i,0)->text().toInt();
        // qDebug() << "currentNum:" << currentNum;
        destTableWidget->item(i,0)->setText(QString::number(currentNum+1));
    }

    // TITLE column
    QString theRelativePath = theFullPath;
    QString absPath = theFullPath; // already is fully qualified

    theRelativePath.replace(musicRootPath, "");
    // qDebug() << "darkAddPlaylistItemAt calling setTitleField" << theRelativePath;

    QString theCanonicalRelativePath = makeCanonicalRelativePath(theRelativePath);

    setTitleField(destTableWidget, insertRowNum, theCanonicalRelativePath, true, PlaylistFileName, theRelativePath); // whichTable, whichRow, fullPath, bool isPlaylist, PlaylistFilename (for errors)

    // PITCH column
    QTableWidgetItem *pit = new QTableWidgetItem(thePitch);
    destTableWidget->setItem(insertRowNum, 2, pit);

    // TEMPO column
    QTableWidgetItem *tem = new QTableWidgetItem(theTempo);
    destTableWidget->setItem(insertRowNum, 3, tem);

    // PATH column
    QTableWidgetItem *fullPath = new QTableWidgetItem(absPath); // full ABSOLUTE path
    destTableWidget->setItem(insertRowNum, 4, fullPath);

    // LOADED column
    QTableWidgetItem *loaded = new QTableWidgetItem("");
    destTableWidget->setItem(insertRowNum, 5, loaded);

    destTableWidget->resizeColumnToContents(0); // FIX: perhaps only if this is the first row?
    //    theTableWidget->resizeColumnToContents(2);
    //    theTableWidget->resizeColumnToContents(3);

    // QTimer::singleShot(250, [destTableWidget]{
    //     // NOTE: We have to do it this way with a single-shot timer, because you can't scroll immediately to a new item, until it's been processed
    //     //   after insertion by the table.  So, there's a delay.  Yeah, this is kludgey, but it works.

    //     // theTableWidget->selectRow(songCount-1); // for some reason this has to be here, or it won't scroll all the way to the bottom.
    //     // theTableWidget->scrollToItem(theTableWidget->item(songCount-1,1), QAbstractItemView::PositionAtCenter); // ensure that the new item is visible

    //     // same thing, but without forcing us to select the last item in the playlist
    //     // theTableWidget->verticalScrollBar()->setSliderPosition(theTableWidget->verticalScrollBar()->maximum());
    //     });

    slotModified[whichSlot] = true;
    // playlistSlotWatcherTimer->start(std::chrono::seconds(10)); // no need for this now, always done via saveSlotNow elsewhere

}

// Functions moved from mainwindow.cpp

void MainWindow::clearSlot(int slotNumber) {
    QTableWidget *theTableWidget;
    QLabel *theLabel;

    switch (slotNumber) {
        case 0: theTableWidget = ui->playlist1Table; theLabel = ui->playlist1Label; break;
        case 1: theTableWidget = ui->playlist2Table; theLabel = ui->playlist2Label; break;
        case 2: theTableWidget = ui->playlist3Table; theLabel = ui->playlist3Label; break;
        default: theTableWidget = ui->playlist1Table; theLabel = ui->playlist1Label; break; // make the compiler warning go away
    }

    theTableWidget->setRowCount(0); // delete all the rows in the slot
    theLabel->setText("<img src=\":/graphics/icons8-menu-64.png\" width=\"10\" height=\"9\">Untitled playlist"); // clear out the label
    slotModified[slotNumber] = false;  // not modified now
    relPathInSlot[slotNumber] = "";    // nobody home now
    // qDebug() << "clearSlot" << slotNumber;
}

void MainWindow::on_action0paletteSlots_triggered()
{
    ui->action0paletteSlots->setChecked(true);
    ui->action1paletteSlots->setChecked(false);
    ui->action2paletteSlots->setChecked(false);
    ui->action3paletteSlots->setChecked(false);
    ui->playlist1Label->setVisible(false);
    ui->playlist1Table->setVisible(false);
    ui->playlist2Label->setVisible(false);
    ui->playlist2Table->setVisible(false);
    ui->playlist3Label->setVisible(false);
    ui->playlist3Table->setVisible(false);
    prefsManager.Setnumpaletteslots("0");
}

void MainWindow::on_action1paletteSlots_triggered()
{
    ui->action0paletteSlots->setChecked(false);
    ui->action1paletteSlots->setChecked(true);
    ui->action2paletteSlots->setChecked(false);
    ui->action3paletteSlots->setChecked(false);
    ui->playlist1Label->setVisible(true);
    ui->playlist1Table->setVisible(true);
    ui->playlist2Label->setVisible(false);
    ui->playlist2Table->setVisible(false);
    ui->playlist3Label->setVisible(false);
    ui->playlist3Table->setVisible(false);
    prefsManager.Setnumpaletteslots("1");
}

void MainWindow::on_action2paletteSlots_triggered()
{
    ui->action0paletteSlots->setChecked(false);
    ui->action1paletteSlots->setChecked(false);
    ui->action2paletteSlots->setChecked(true);
    ui->action3paletteSlots->setChecked(false);
    ui->playlist1Label->setVisible(true);
    ui->playlist1Table->setVisible(true);
    ui->playlist2Label->setVisible(true);
    ui->playlist2Table->setVisible(true);
    ui->playlist3Label->setVisible(false);
    ui->playlist3Table->setVisible(false);
    prefsManager.Setnumpaletteslots("2");
}

void MainWindow::on_action3paletteSlots_triggered()
{
    ui->action0paletteSlots->setChecked(false);
    ui->action1paletteSlots->setChecked(false);
    ui->action2paletteSlots->setChecked(false);
    ui->action3paletteSlots->setChecked(true);
    ui->playlist1Label->setVisible(true);
    ui->playlist1Table->setVisible(true);
    ui->playlist2Label->setVisible(true);
    ui->playlist2Table->setVisible(true);
    ui->playlist3Label->setVisible(true);
    ui->playlist3Table->setVisible(true);
    prefsManager.Setnumpaletteslots("3");
}
