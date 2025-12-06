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
#include "playlist_constants.h"
#include "playlistexport.h"
#include "mytreewidget.h"
#include <functional>
#include <utility>

#if defined(Q_OS_LINUX)
#define OS_FALLTHROUGH [[fallthrough]]
#elif defined(Q_OS_WIN)
#define OS_FALLTHROUGH
#else
    // already defined on Mac OS X
#endif

// CONSTANTS moved to common_enums.h for global access

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
// Helper function to handle common playlist movement operations
void MainWindow::movePlaylistItems(std::function<bool(MyTableWidget*)> moveOperation) {
    if (!darkmode) return;

    MyTableWidget* tables[] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};
    bool anyModified = false;

    for (int i = 0; i < MAX_PLAYLIST_SLOTS; ++i) {
        if (!relPathInSlot[i].startsWith(TRACKS_PATH_PREFIX)) {
            bool modified = moveOperation(tables[i]);
            slotModified[i] = modified || slotModified[i];
            if (slotModified[i]) {
                saveSlotNow(i);
                anyModified = true;
            }
        }
    }

    // Refresh indentation after move operations (issue #1547)
    if (anyModified) {
        refreshAllPlaylists();
    }
}

// ----------------------------------------------------------------------
// Helper function to get table widget and label for a given slot number
std::pair<QTableWidget*, QLabel*> MainWindow::getSlotWidgets(int slotNumber) {
    switch (slotNumber) {
        case SLOT_1: return {ui->playlist1Table, ui->playlist1Label};
        case SLOT_2: return {ui->playlist2Table, ui->playlist2Label};
        case SLOT_3: return {ui->playlist3Table, ui->playlist3Label};
        default: return {ui->playlist1Table, ui->playlist1Label};
    }
}

// ----------------------------------------------------------------------
// Helper function to clear any existing slots that contain the same playlist
// Helper function to set last playlist loaded preference for any slot
void MainWindow::setLastPlaylistLoaded(int slotNumber, const QString& playlistPath) {
    switch (slotNumber) {
        case SLOT_1: prefsManager.SetlastPlaylistLoaded(playlistPath); break;
        case SLOT_2: prefsManager.SetlastPlaylistLoaded2(playlistPath); break;
        case SLOT_3: prefsManager.SetlastPlaylistLoaded3(playlistPath); break;
        default: break; // Invalid slot number
    }
}

void MainWindow::clearDuplicateSlots(const QString& relPath) {
    for (int i = 0; i < MAX_PLAYLIST_SLOTS; i++) {
        if (relPath == relPathInSlot[i]) {
            saveSlotNow(i);
            clearSlot(i);
            setLastPlaylistLoaded(i, "");
        }
    }
}

// ----------------------------------------------------------------------
// Helper function to set palette slot visibility
void MainWindow::setPaletteSlotVisibility(int numSlots) {
    ui->action0paletteSlots->setChecked(numSlots == 0);
    ui->action1paletteSlots->setChecked(numSlots == 1);
    ui->action2paletteSlots->setChecked(numSlots == 2);
    ui->action3paletteSlots->setChecked(numSlots == 3);
    
    ui->playlist1Label->setVisible(numSlots >= 1);
    ui->playlist1Table->setVisible(numSlots >= 1);
    ui->playlist2Label->setVisible(numSlots >= 2);
    ui->playlist2Table->setVisible(numSlots >= 2);
    ui->playlist3Label->setVisible(numSlots >= 3);
    ui->playlist3Table->setVisible(numSlots >= 3);
    
    prefsManager.Setnumpaletteslots(QString::number(numSlots));
}

// ----------------------------------------------------------------------
void MainWindow::PlaylistItemsToTop() {
    movePlaylistItems([](MyTableWidget* table) { return table->moveSelectedItemsToTop(); });
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemsToBottom() {
    movePlaylistItems([](MyTableWidget* table) { return table->moveSelectedItemsToBottom(); });
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemsMoveUp() {
    movePlaylistItems([](MyTableWidget* table) { return table->moveSelectedItemsUp(); });
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemsMoveDown() {
    movePlaylistItems([](MyTableWidget* table) { return table->moveSelectedItemsDown(); });
}

// --------------------------------------------------------------------
void MainWindow::PlaylistItemsRemove() {
    movePlaylistItems([](MyTableWidget* table) { return table->removeSelectedItems(); });
}

// ============================================================================================================
// Detect if a filename is a playlist marker/separator (issue #1547)
// Returns true if filename contains 3+ consecutive separator chars (=, -, _, or +)
bool MainWindow::isPlaylistMarker(const QString &filename) {
    // Extract short title (filename without path/extension)
    static QRegularExpression dotMusicSuffix(SUPPORTED_AUDIO_EXTENSIONS_REGEX,
                                              QRegularExpression::CaseInsensitiveOption);
    QString shortTitle = filename.split('/').last().replace(dotMusicSuffix, "");

    // Check for 3+ consecutive separator characters
    static QRegularExpression markerPattern("[=\\-_+]{3,}");
    return markerPattern.match(shortTitle).hasMatch();
}

// ============================================================================================================
// Helper function to determine if a row should be indented based on previous rows (issue #1547)
// Scans backwards from rowNum to find the most recent marker
bool MainWindow::shouldIndentPlaylistRow(QTableWidget *table, int rowNum) {
    if (rowNum < 0 || table == nullptr) {
        return false;
    }

    // Check if current row is a marker - markers themselves are never indented
    if (table->item(rowNum, COLUMN_PATH) != nullptr) {
        QString currentPath = table->item(rowNum, COLUMN_PATH)->text();
        if (isPlaylistMarker(currentPath)) {
            return false;  // Markers are not indented
        }
    }

    // Scan backwards to find if we're under a marker
    for (int i = rowNum - 1; i >= 0; i--) {
        if (table->item(i, COLUMN_PATH) != nullptr) {
            QString path = table->item(i, COLUMN_PATH)->text();
            if (isPlaylistMarker(path)) {
                return true;  // Found a marker above us, so we should be indented
            }
        }
    }

    return false;  // No marker found above us
}

// ============================================================================================================
void MainWindow::setTitleField(QTableWidget *whichTable, int whichRow, QString relativePath,
                               bool isPlaylist, QString PlaylistFileName, QString theRealPath,
                               bool shouldIndent) {

    // relativePath is relative to the MusicRootPath, and it might be fake (because it was reversed, like Foo Bar - RIV123.mp3,
    //     we reversed it for presentation
    // theRealPath is the actual relative path, used to determine whether the file exists or not

    bool isAppleMusicFile = theRealPath.contains("/iTunes/iTunes Media/");
    bool isDarkSongTable = (whichTable == ui->darkSongTable);

    static QRegularExpression dotMusicSuffix(SUPPORTED_AUDIO_EXTENSIONS_REGEX, QRegularExpression::CaseInsensitiveOption); // match with music extensions
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
        textCol = QColor(extrasColorString);
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
    QString appleSymbol = QChar(APPLE_SYMBOL_UNICODE);
    if (isAppleMusicFile && !isDarkSongTable) {
        shortTitle = appleSymbol + " " + shortTitle; // add Apple space as prefix to actual short title
    }

    QString titlePlusTags(FormatTitlePlusTags(shortTitle, settings.isSetTags(), settings.getTags(), textCol.name()));

    // Check if this is a marker row (issue #1547)
    bool isMarker = isPlaylistMarker(relativePath);

    // Make marker rows bold using HTML (issue #1547)
    if (isMarker) {
        titlePlusTags = "<b>" + titlePlusTags + "</b>";
    }

    // Add indentation for non-marker rows following markers (issue #1547)
    if (shouldIndent) {
        QString indentSpaces;
        for (int i = 0; i < PLAYLIST_INDENT_SPACES; i++) {
            indentSpaces += "&nbsp;";  // HTML non-breaking space
        }
        titlePlusTags = indentSpaces + titlePlusTags;
    }

    // qDebug() << "setTitleField:" << shortTitle << titlePlusTags;
    title->setText(titlePlusTags);

    QTableWidgetItem *blankItem = new QTableWidgetItem;
    whichTable->setItem(whichRow, COLUMN_TITLE, blankItem); // null item so I can get the row() later

    whichTable->setCellWidget(whichRow, 1, title);

    // Also make the number column bold for markers (issue #1547)
    if (isMarker && whichTable->item(whichRow, COLUMN_NUMBER) != nullptr) {
        QFont f = whichTable->item(whichRow, COLUMN_NUMBER)->font();
        f.setBold(true);
        whichTable->item(whichRow, COLUMN_NUMBER)->setFont(f);
    }

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
        QString shortPlaylistName = PlaylistFileName.split('/').last().replace(CSV_FILE_EXTENSION,"");
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

                QMenu *plMenu = new QMenu(this);
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

                    QString fullPath = whichTable->item(theRow, COLUMN_PATH)->text();
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
                            plMenu->addAction(QString("Load Cuesheets"),
                                              [this, fullMP3Path, cuesheetPath]() {
                                                  maybeLoadCuesheets(fullMP3Path, cuesheetPath);
                                              }
                                              );
                        }
                    }

#if defined(Q_OS_MAC)
                    // SECTIONS STUFF (single row) ============
                    plMenu->addSeparator();
                    plMenu->addAction("Calculate Section Info for this song...",
                                      [this, fullPath]() {
                                          EstimateSectionsForThisSong(fullPath);
                                      });
                    plMenu->addAction("Remove Section info for this song....",
                                      [this, fullPath]() {
                                          RemoveSectionsForThisSong(fullPath);
                                      });
#endif
                } else {
                    // MULTIPLE ROWS SELECTED
#if defined(Q_OS_MAC)
                    // Extract all selected file paths
                    QStringList selectedPaths;
                    QItemSelectionModel *selectionModel = whichTable->selectionModel();
                    QModelIndexList selected = selectionModel->selectedRows();

                    for (const auto &index : selected) {
                        int row = index.row();
                        QString path = whichTable->item(row, COLUMN_PATH)->text();
                        selectedPaths.append(path);
                    }

                    // SECTIONS STUFF (multiple rows) ============
                    plMenu->addSeparator();
                    plMenu->addAction("Calculate Section Info for these " + QString::number(rowCount) + " songs...",
                                      [this, selectedPaths]() {
                                          EstimateSectionsForThesePaths(selectedPaths);
                                      });
                    plMenu->addAction("Remove Section info for these " + QString::number(rowCount) + " songs....",
                                      [this, selectedPaths]() {
                                          RemoveSectionsForThesePaths(selectedPaths);
                                      });
#endif
                } // if there's just one row in the selection

                plMenu->popup(QCursor::pos());
                plMenu->exec();
            }
            );
}

// ============================================================================================================
// updates the songCount as it goes (1 return value via reference)
void MainWindow::loadPlaylistFromFileToPaletteSlot(QString PlaylistFileName, int slotNumber, int &songCount) {

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

    // Dispatcher logic
    if (relativePath.startsWith("/Tracks") || relativePath.startsWith("Tracks")) {
        loadTrackFilterToSlot(PlaylistFileName, relativePath, slotNumber, songCount);
    }
    else if (relativePath.startsWith(APPLE_MUSIC_PATH_PREFIX)) {
        loadAppleMusicPlaylistToSlot(PlaylistFileName, relativePath, slotNumber, songCount);
    }
    else {
        loadRegularPlaylistToSlot(PlaylistFileName, relativePath, slotNumber, songCount);
    }
}

// ============================================================================================================
// Handler for Track filters (e.g., from /Tracks directory)
void MainWindow::loadTrackFilterToSlot(QString PlaylistFileName, QString relativePath, int slotNumber, int &songCount) {
    relativePath.replace("/Tracks/", "").replace("Tracks/", "").replace(CSV_FILE_EXTENSION, "");

    QString rPath = QString(TRACKS_PATH_PREFIX) + relativePath;

    // ALLOW ONLY ONE COPY OF A TRACK FILTER LOADED IN THE SLOT PALETTE AT A TIME ------
    clearDuplicateSlots(rPath);

    // Set up table widget and label based on slot number
    auto [theTableWidget, theLabel] = getSlotWidgets(slotNumber);
    
    setLastPlaylistLoaded(slotNumber, "tracks/" + relativePath);

    theTableWidget->hide();
    theTableWidget->setSortingEnabled(false);
    theTableWidget->setRowCount(0); // delete all the rows in the slot

    // The only way to get here is to RIGHT-CLICK on a Tracks/filterName in the TreeWidget.
    // In that case, the darkSongTable has already been loaded with the filtered rows.
    // We just need to transfer them up to the playlist.

    static QRegularExpression title_tags_remover("(\\&nbsp\\;)*\\<\\/?span( .*?)?>");
    static QRegularExpression spanPrefixRemover("<span style=\"color:.*\">(.*)</span>", QRegularExpression::InvertedGreedinessOption);

    songCount = 0;
    bool currentlyUnderMarker = false;  // Track if we're in a section under a marker (issue #1547)
    for (int i = 0; i < ui->darkSongTable->rowCount(); i++) {
        if (true || !ui->darkSongTable->isRowHidden(i)) {

            // make sure this song has the "type" that we're looking for...
            QString pathToMP3 = ui->darkSongTable->item(i,kPathCol)->data(Qt::UserRole).toString();
            QString p1 = pathToMP3;
            p1 = p1.replace(musicRootPath + "/", "");

            QString typeOfFile = p1.split("/").first();

            if (typeOfFile != relativePath) {
                continue; // skip this one, if the type is not what we want
            }

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

            // make a new row, if needed
            if (songCount > theTableWidget->rowCount()) {
                theTableWidget->insertRow(theTableWidget->rowCount());
            }

            // # column
            QTableWidgetItem *num = new TableNumberItem(QString::number(songCount)); // use TableNumberItem so that it sorts numerically
            theTableWidget->setItem(songCount-1, COLUMN_NUMBER, num);

            // TITLE column ================================
            QString s12 = QString("/") + p1;
            QString categoryName = filepath2SongCategoryName(s12);
            QString fakePath = "/" + categoryName + "/" + label + " - " + shortTitle;
            if (!songTypeNamesForCalled.contains(categoryName) &&
                !songTypeNamesForExtras.contains(categoryName) &&
                !songTypeNamesForPatter.contains(categoryName) &&
                !songTypeNamesForSinging.contains(categoryName)
                ) {
                fakePath = "/xtras" + fakePath;
            }

            // Determine if this row is a marker and if it should be indented (issue #1547)
            bool thisRowIsMarker = isPlaylistMarker(pathToMP3);
            bool shouldIndent;
            if (thisRowIsMarker) {
                currentlyUnderMarker = true;  // Start a new marker section
                shouldIndent = false;         // Markers themselves are NOT indented
            } else {
                shouldIndent = currentlyUnderMarker;  // Indent if we're under a marker
            }

            setTitleField(theTableWidget, songCount-1, fakePath, false, PlaylistFileName, "", shouldIndent); // whichTable, whichRow, relativePath or pre-colored title, bool isPlaylist, PlaylistFilename (for errors and for filters it's colored)

            // PITCH column
            QTableWidgetItem *pit = new QTableWidgetItem(pitch);
            theTableWidget->setItem(songCount-1, COLUMN_PITCH, pit);

            // TEMPO column
            QTableWidgetItem *tem = new QTableWidgetItem(tempo);
            theTableWidget->setItem(songCount-1, COLUMN_TEMPO, tem);

            // PATH column
            QTableWidgetItem *fullPath = new QTableWidgetItem(pathToMP3); // full ABSOLUTE path
            theTableWidget->setItem(songCount-1, COLUMN_PATH, fullPath);

            // LOADED column
            QTableWidgetItem *loaded = new QTableWidgetItem("");
            theTableWidget->setItem(songCount-1, COLUMN_LOADED, loaded);
        }
    }

    theTableWidget->resizeColumnToContents(COLUMN_NUMBER);

    theTableWidget->setSortingEnabled(true);
    theTableWidget->show();

    // redo the label, but with an indicator that these are Tracks, not a Playlist
    QString playlistShortName = relativePath;
    theLabel->setText(QString("<img src=\":/graphics/icons8-musical-note-60.png\" width=\"%1\" height=\"%2\">").arg(TRACK_ICON_WIDTH).arg(TRACK_ICON_HEIGHT) + playlistShortName);

    relPathInSlot[slotNumber] = TRACKS_PATH_PREFIX + relativePath;

    slotModified[slotNumber] = false;

    return; // no errors
}

// ============================================================================================================
// Handler for Apple Music playlists
void MainWindow::loadAppleMusicPlaylistToSlot(QString PlaylistFileName, QString relativePath, int slotNumber, int &songCount) {
    QString relPath = relativePath;     // e.g. /Apple Music/foo.csv
    relPath.replace(CSV_FILE_EXTENSION, "");          // e.g. /Apple Music/foo

    // ALLOW ONLY ONE COPY OF A PLAYLIST LOADED IN THE SLOT PALETTE AT A TIME ------
    clearDuplicateSlots(relPath);

    relPath.replace(APPLE_MUSIC_PATH_PREFIX, ""); // relPath is e.g. "foo"

    QString applePlaylistName = relPath;


    auto [theTableWidget, theLabel] = getSlotWidgets(slotNumber);
    
    setLastPlaylistLoaded(slotNumber, "Apple Music/" + relPath);

    theTableWidget->setRowCount(0); // delete all the rows

    linesInCurrentPlaylist = 0;
    songCount = 0;
    bool currentlyUnderMarker = false;  // Track if we're in a section under a marker (issue #1547)

    for (const auto& sl : std::as_const(allAppleMusicPlaylists)) {
        bool isPlayableAppleMusicItem = sl[2].endsWith(".wav", Qt::CaseInsensitive) ||
                          sl[2].endsWith(".mp3", Qt::CaseInsensitive) ||
                          sl[2].endsWith(".m4a", Qt::CaseInsensitive);

        if (sl[0] == applePlaylistName && isPlayableAppleMusicItem)  {
            // YES! it belongs to the playlist we want
            songCount++;

            // make a new row, if needed
            if (songCount > theTableWidget->rowCount()) {
                theTableWidget->insertRow(theTableWidget->rowCount());
            }

            // # column
            QTableWidgetItem *num = new TableNumberItem(QString::number(songCount)); // use TableNumberItem so that it sorts numerically
            theTableWidget->setItem(songCount-1, COLUMN_NUMBER, num);

            // TITLE column
            QString appleSymbol = QChar(APPLE_SYMBOL_UNICODE); // TODO: factor into global?
            QString shortTitle = sl[1];
            shortTitle = appleSymbol + " " + shortTitle;

            // Determine if this row is a marker and if it should be indented (issue #1547)
            bool thisRowIsMarker = isPlaylistMarker(sl[2]);
            bool shouldIndent;
            if (thisRowIsMarker) {
                currentlyUnderMarker = true;  // Start a new marker section
                shouldIndent = false;         // Markers themselves are NOT indented
            } else {
                shouldIndent = currentlyUnderMarker;  // Indent if we're under a marker
            }

            setTitleField(theTableWidget, songCount-1, "/xtras/" + shortTitle, false, sl[1], "", shouldIndent); // whichTable, whichRow, relativePath or pre-colored title, bool isPlaylist, PlaylistFilename (for errors and for filters it's colored)

            // PITCH column
            QTableWidgetItem *pit = new QTableWidgetItem("0"); // defaults to no pitch change
            theTableWidget->setItem(songCount-1, COLUMN_PITCH, pit);

            // TEMPO column
            QTableWidgetItem *tem = new QTableWidgetItem("0"); // defaults to tempo 0 = "unknown".  Set to midpoint on load.
            theTableWidget->setItem(songCount-1, COLUMN_TEMPO, tem);

            // PATH column
            QTableWidgetItem *fullPath = new QTableWidgetItem(sl[2]); // full ABSOLUTE path
            theTableWidget->setItem(songCount-1, COLUMN_PATH, fullPath);

            // LOADED column
            QTableWidgetItem *loaded = new QTableWidgetItem("");  // NOT LOADED
            theTableWidget->setItem(songCount-1, COLUMN_LOADED, loaded);
        }
    }

    theTableWidget->resizeColumnToContents(COLUMN_NUMBER);

    QString theRelativePath = relativePath.replace(APPLE_MUSIC_PATH_PREFIX,"").replace(CSV_FILE_EXTENSION,"");
    theLabel->setText(QString("<img src=\":/graphics/icons8-apple-48.png\" width=\"%1\" height=\"%2\">").arg(APPLE_MUSIC_ICON_WIDTH).arg(APPLE_MUSIC_ICON_HEIGHT) + theRelativePath);

    relPathInSlot[slotNumber] = PlaylistFileName;
    relPathInSlot[slotNumber].replace(musicRootPath, "").replace(CSV_FILE_EXTENSION,""); // e.g. /Apple Music/Second Playlist
    slotModified[slotNumber] = false; // this is a filter so nothing to save here

    theTableWidget->setCurrentItem(theTableWidget->item(0, COLUMN_NUMBER)); // select first item
    theTableWidget->setFocus();

    return; // no errors
}

// ============================================================================================================
// Handler for regular CSV playlists
void MainWindow::loadRegularPlaylistToSlot(QString PlaylistFileName, QString relativePath, int slotNumber, int &songCount) {
    QString relPath = relativePath; // this is the name of a playlist
    relPath.replace(PLAYLISTS_PATH_PREFIX, "").replace(CSV_FILE_EXTENSION, ""); // relPath is e.g. "5thWed/5thWed_2021.12.29" same as relPathInSlot now

    // ALLOW ONLY ONE COPY OF A PLAYLIST LOADED IN THE SLOT PALETTE AT A TIME ------
    clearDuplicateSlots(relPath);
    QFile inputFile(PlaylistFileName);
    if (inputFile.open(QIODevice::ReadOnly)) { // defaults to Text mode

        auto [theTableWidget, theLabel] = getSlotWidgets(slotNumber);
        
        setLastPlaylistLoaded(slotNumber, "playlists/" + relPath);

        theTableWidget->setRowCount(0); // delete all the rows

        linesInCurrentPlaylist = 0;

        QTextStream in(&inputFile);

        if (PlaylistFileName.endsWith(CSV_FILE_EXTENSION, Qt::CaseInsensitive)) {
            // CSV FILE =================================

            QString header = in.readLine();  // read header (and throw away for now), should be "abspath,pitch,tempo"
            Q_UNUSED(header) // turn off the warning (actually need the readLine to happen);

            songCount = 0;
            bool currentlyUnderMarker = false;  // Track if we're in a section under a marker (issue #1547)

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

                    // make a new row, if needed
                    if (songCount > theTableWidget->rowCount()) {
                        theTableWidget->insertRow(theTableWidget->rowCount());
                    }

                    // # column
                    QTableWidgetItem *num = new TableNumberItem(QString::number(songCount)); // use TableNumberItem so that it sorts numerically
                    theTableWidget->setItem(songCount-1, COLUMN_NUMBER, num);

                    // TITLE column
                    QString absPath = musicRootPath + list1[0];
                    if (list1[0].startsWith("/Users/")) {
                        absPath = list1[0]; // override for absolute pathnames
                    }

                    QFileInfo fi(absPath);
                    QString theBaseName = fi.completeBaseName();
                    QString theSuffix = fi.suffix();

                    QString theLabel, theLabelNum, theLabelNumExtra, theTitle, theShortTitle;
                    breakFilenameIntoParts(theBaseName, theLabel, theLabelNum, theLabelNumExtra, theTitle, theShortTitle);

                    // check to see if it needs to be colored like "/xtras"
                    QString categoryName = filepath2SongCategoryName(fi.absolutePath());
                    if (!songTypeNamesForCalled.contains(categoryName) &&
                        !songTypeNamesForExtras.contains(categoryName) &&
                        !songTypeNamesForPatter.contains(categoryName) &&
                        !songTypeNamesForSinging.contains(categoryName)
                        ) {
                        categoryName = "xtras";
                    }

                    QString theFakePath = "/" + categoryName + "/" + theLabel + " " + theLabelNum + " - " + theTitle + "." + theSuffix;

                    if (absPath.contains("/iTunes/iTunes Media/")) {
                        theFakePath = absPath;
                    }

                    // Determine if this row is a marker and if it should be indented (issue #1547)
                    bool thisRowIsMarker = isPlaylistMarker(list1[0]);
                    bool shouldIndent;
                    if (thisRowIsMarker) {
                        currentlyUnderMarker = true;  // Start a new marker section
                        shouldIndent = false;         // Markers themselves are NOT indented
                    } else {
                        shouldIndent = currentlyUnderMarker;  // Indent if we're under a marker
                    }

                    setTitleField(theTableWidget, songCount-1, theFakePath, true, PlaylistFileName, list1[0], shouldIndent); // list1[0] points at the file, theFakePath might have been reversed, e.g. Foo - RIV123.mp3

                    // PITCH column
                    QTableWidgetItem *pit = new QTableWidgetItem(list1[1]);
                    theTableWidget->setItem(songCount-1, COLUMN_PITCH, pit);

                    // TEMPO column
                    QTableWidgetItem *tem = new QTableWidgetItem(list1[2]);
                    theTableWidget->setItem(songCount-1, COLUMN_TEMPO, tem);

                    // PATH column
                    QTableWidgetItem *fullPath = new QTableWidgetItem(absPath); // full ABSOLUTE path
                    theTableWidget->setItem(songCount-1, COLUMN_PATH, fullPath);

                    // LOADED column
                    QTableWidgetItem *loaded = new QTableWidgetItem("");
                    theTableWidget->setItem(songCount-1, COLUMN_LOADED, loaded);
                }
            } // while
        }

        inputFile.close();

        theTableWidget->resizeColumnToContents(COLUMN_NUMBER);

        QString theRelativePath = relativePath.replace(PLAYLISTS_PATH_PREFIX,"").replace(CSV_FILE_EXTENSION,"");
        theLabel->setText(QString("<img src=\":/graphics/icons8-menu-64.png\" width=\"%1\" height=\"%2\">").arg(PLAYLIST_ICON_WIDTH).arg(PLAYLIST_ICON_HEIGHT) + theRelativePath);

        relPathInSlot[slotNumber] = PlaylistFileName;
        relPathInSlot[slotNumber] = relPathInSlot[slotNumber].replace(musicRootPath + PLAYLISTS_PATH_PREFIX, "").replace(CSV_FILE_EXTENSION,"");

        theTableWidget->setCurrentItem(theTableWidget->item(0, COLUMN_NUMBER)); // select first item
        theTableWidget->setFocus();
    }
    else {
        // file didn't open...
        return;
    }
}

void MainWindow::handlePlaylistDoubleClick(QTableWidgetItem *item)
{
    if (!item) return;
    auto tableWidget = item->tableWidget();

    int slotNumber = -1;
    if (tableWidget == ui->playlist1Table) {
        slotNumber = 0;
    } else if (tableWidget == ui->playlist2Table) {
        slotNumber = 1;
    } else if (tableWidget == ui->playlist3Table) {
        slotNumber = 2;
    }

    if (slotNumber == -1) {
        return;
    }

    PerfTimer t("on_playlistTable_itemDoubleClicked", __LINE__);

    on_darkStopButton_clicked();  // if we're loading a new MP3 file, stop current playback
    saveCurrentSongSettings();

    t.elapsed(__LINE__);

    int row = item->row();
    QString pathToMP3 = tableWidget->item(row, COLUMN_PATH)->text();

    currentSongPlaylistTable = tableWidget;
    currentSongPlaylistRow = tableWidget->selectedRanges()[0].topRow();
    // qDebug() << "currentSongPlaylistRow" << currentSongPlaylistRow;

    QFileInfo fi(pathToMP3);
    if (!fi.exists()) {
        qDebug() << "ERROR: File does not exist " << pathToMP3;
        return;  // can't load a file that doesn't exist
    }

    QString nextFile = "";
    if (row+1 < tableWidget->rowCount()) {
        nextFile = tableWidget->item(row+1, COLUMN_PATH)->text();
    }
    static QRegularExpression dotMusicSuffix(SUPPORTED_AUDIO_EXTENSIONS_REGEX, QRegularExpression::CaseInsensitiveOption); // match with music extensions
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

    // these must be up here to get the correct values...
    QString pitch  = tableWidget->item(row, COLUMN_PITCH)->text();
    QString tempo  = tableWidget->item(row, COLUMN_TEMPO)->text();

    targetPitch = pitch;  // save this string, and set pitch slider AFTER base BPM has been figured out
    targetTempo = tempo;  // save this string, and set tempo slider AFTER base BPM has been figured out

    t.elapsed(__LINE__);

    loadMP3File(pathToMP3, songTitle, songType, songLabel, nextFile);

    t.elapsed(__LINE__);

    // clear all the "1"s and arrows from palette slots (IF and only IF one of the palette slots is being currently edited
    MyTableWidget *tables[] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};
    for (int i = 0; i < 3; i++) {
        MyTableWidget *table = tables[i];
        for (int j = 0; j < table->rowCount(); j++) {
            if (table->item(j, COLUMN_LOADED)->text() == "1") {
                QFont currentFont = table->item(j, COLUMN_TITLE)->font(); // font goes to neutral (not bold or italic, and normal size) for NOT-loaded items
                currentFont.setBold(false);
                currentFont.setItalic(false);
                table->item(j, COLUMN_NUMBER)->setFont(currentFont);
                // For COLUMN_TITLE, we need to modify the QLabel widget's HTML (issue #1547)
                QLabel *titleLabel = dynamic_cast<QLabel*>(table->cellWidget(j, COLUMN_TITLE));
                if (titleLabel) {
                    QString html = titleLabel->text();
                    // Remove <b><i> tags if present
                    html.replace(QRegularExpression("</?b>"), "");
                    html.replace(QRegularExpression("</?i>"), "");
                    titleLabel->setText(html);
                }
                table->item(j, COLUMN_PITCH)->setFont(currentFont);
                table->item(j, COLUMN_TEMPO)->setFont(currentFont);
            }
            table->item(j, COLUMN_LOADED)->setText(""); // clear out the old table
        }
    }

    sourceForLoadedSong = qobject_cast<MyTableWidget*>(tableWidget); // THIS is where we got the currently loaded song (this is the NEW table)

    for (int i = 0; i < sourceForLoadedSong->rowCount(); i++) {
        if (i == row) {
            QFont currentFont = sourceForLoadedSong->item(i, COLUMN_TITLE)->font();  // font goes to BOLD ITALIC BIGGER for loaded items
            currentFont.setBold(true);
            currentFont.setItalic(true);
            sourceForLoadedSong->item(i, COLUMN_NUMBER)->setFont(currentFont);
            // For COLUMN_TITLE, we need to modify the QLabel widget's HTML (issue #1547)
            QLabel *titleLabel = dynamic_cast<QLabel*>(sourceForLoadedSong->cellWidget(i, COLUMN_TITLE));
            if (titleLabel) {
                QString html = titleLabel->text();
                // Remove any existing <b><i> tags first to avoid nesting issues (e.g., from marker rows)
                html.replace(QRegularExpression("</?b>"), "");
                html.replace(QRegularExpression("</?i>"), "");
                // Wrap in <b><i> for loaded items
                html = "<b><i>" + html + "</i></b>";
                titleLabel->setText(html);
            }
            sourceForLoadedSong->item(i, COLUMN_PITCH)->setFont(currentFont);
            sourceForLoadedSong->item(i, COLUMN_TEMPO)->setFont(currentFont);
        }
        sourceForLoadedSong->item(i, COLUMN_LOADED)->setText((i == row) ? "1" : ""); // and this is the one being edited (clear out others)
    }

    // these must be down here, to set the correct values...
    int pitchInt = pitch.toInt();

    ui->darkPitchSlider->setValue(pitchInt);

    on_darkPitchSlider_valueChanged(pitchInt); // manually call this, in case the setValue() line doesn't call valueChanged() when the value set is
        //   exactly the same as the previous value.  This will ensure that cBass->setPitch() gets called (right now) on the new stream.
    if (ui->actionAutostart_playback->isChecked()) {
        on_darkPlayButton_clicked();
    }

    tableWidget->setFocus();
    tableWidget->resizeColumnToContents(COLUMN_NUMBER); // number column needs to be resized, if bolded

    t.elapsed(__LINE__);
}



// ========================
// DARK MODE: file dialog to ask for a file name, then save slot to that file
// TODO: much of this code is same as saveSlotAsPlaylist(whichSlot), which just opens a file dialog
//   so this should be factored.
void MainWindow::saveSlotAsPlaylist(int whichSlot)  // slots 0 - 2
{
    auto [theTableWidget, theLabel] = getSlotWidgets(whichSlot);

    // // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    // const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");

    // QString startingPlaylistDirectory = prefsManager.MySettings.value(DEFAULT_PLAYLIST_DIR_KEY).toString();
    // if (startingPlaylistDirectory.isNull()) {
    //     // first time through, start at HOME
    //     startingPlaylistDirectory = QDir::homePath();
    // }

    QString startingPlaylistDirectory;

    if (relPathInSlot[whichSlot] == "") {
        QString tomorrowString = QDate::currentDate().addDays(1).toString("yyyy.MM.dd");
        startingPlaylistDirectory = musicRootPath + "/playlists/" + tomorrowString + ".csv";
    } else {
        startingPlaylistDirectory = musicRootPath + PLAYLISTS_PATH_PREFIX + relPathInSlot[whichSlot] + "_copy.csv";
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

    QString playlistShortName = PlaylistFileName.replace(musicRootPath,"").split('/').last().replace(CSV_FILE_EXTENSION,""); // PlaylistFileName is now altered
//    qDebug() << "playlistShortName: " << playlistShortName;

    // ACTUAL SAVE TO FILE ============
    filewatcherShouldIgnoreOneFileSave = true;  // I don't know why we have to do this, but rootDir is being watched, which causes this to be needed.

    // add on .CSV if user didn't specify (File Options Dialog?)
    if (!fullFilePath.endsWith(CSV_FILE_EXTENSION, Qt::CaseInsensitive)) {
        fullFilePath = fullFilePath + CSV_FILE_EXTENSION;
    }

    QFile file(fullFilePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream stream(&file);

        stream << CSV_HEADER_STRING << ENDL;  // NOTE: This is now a RELATIVE PATH, and "relpath" is used to detect that.

        for (int i = 0; i < theTableWidget->rowCount(); i++) {
            QString path = theTableWidget->item(i, COLUMN_PATH)->text();
            path = path.replace(musicRootPath,"").replace("\"","\"\""); // if path contains a QUOTE, it needs to be changed to DOUBLE QUOTE in CSV

            QString pitch = theTableWidget->item(i, COLUMN_PITCH)->text();
            QString tempo = theTableWidget->item(i, COLUMN_TEMPO)->text();
            // qDebug() << path + "," + pitch + "," + tempo;
            stream << "\"" + path + "\"," + pitch + "," + tempo << ENDL; // relative path with quotes, then pitch then tempo (% or bpm)
        }

        file.close(); // OK, we're done saving the file, so...
        slotModified[whichSlot] = false;

        // and update the QString array (for later use)
        QString rel = fullFilePath;
        relPathInSlot[whichSlot] = rel.replace(musicRootPath + PLAYLISTS_PATH_PREFIX, "").replace(CSV_FILE_EXTENSION,""); // relative to musicDir/playlists
        // qDebug() << "saveSlotsAsPlaylist, relPathInSlot set to: " << relPathInSlot[whichSlot];

        // update the label with the partial path, e.g. "Jokers/2024/playlist_06.05"
        theLabel->setText(QString("<img src=\":/graphics/icons8-menu-64.png\" width=\"%1\" height=\"%2\">%3").arg(PLAYLIST_ICON_WIDTH).arg(PLAYLIST_ICON_HEIGHT).arg(relPathInSlot[whichSlot]));

        // and refresh the TreeWidget, because we have a new playlist now...
        updateTreeWidget();

        // NOW, make it so at app restart we reload this one that now has a name...
        setLastPlaylistLoaded(whichSlot, "playlists/" + relPathInSlot[whichSlot]);

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

    QString PlaylistFileName = musicRootPath + PLAYLISTS_PATH_PREFIX + relPathInSlot[whichSlot] + CSV_FILE_EXTENSION;
//    qDebug() << "Let's save this slot:" << whichSlot << ":" << PlaylistFileName;

//    return; // TEST TEST TEST

    // which tableWidget are we dealing with?
    auto [theTableWidget, theLabel] = getSlotWidgets(whichSlot);
    QString theTableLabelText = theLabel->text();

    QString playlistShortName = PlaylistFileName;
    playlistShortName = playlistShortName.replace(musicRootPath,"").split('/').last().replace(CSV_FILE_EXTENSION,""); // PlaylistFileName is now altered
//    qDebug() << "playlistShortName: " << playlistShortName;

    // ACTUAL SAVE TO FILE ============
    filewatcherShouldIgnoreOneFileSave = true;  // I don't know why we have to do this, but rootDir is being watched, which causes this to be needed.

    QFile file(PlaylistFileName);

    if (PlaylistFileName.contains(APPLE_MUSIC_PATH_PREFIX, Qt::CaseInsensitive)) {
        // Apple Music playlists are not SquareDesk CSV playlists, so they don't need to be saved
        return;
    }

    // qDebug() << "***** Saving playlist: " << PlaylistFileName;

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream stream(&file);

        stream << CSV_HEADER_STRING << ENDL;  // NOTE: This is a RELATIVE PATH, and "relpath" is used to detect that.

        for (int i = 0; i < theTableWidget->rowCount(); i++) {
            QString path = theTableWidget->item(i, COLUMN_PATH)->text();
            path = path.replace(musicRootPath,"").replace("\"","\"\""); // if path contains a QUOTE, it needs to be changed to DOUBLE QUOTE in CSV

            QString pitch = theTableWidget->item(i, COLUMN_PITCH)->text();
            QString tempo = theTableWidget->item(i, COLUMN_TEMPO)->text();
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
                QString path = theTableWidget->item(i, COLUMN_PATH)->text();
                QString pitch = theTableWidget->item(i, COLUMN_PITCH)->text();
                QString tempo = theTableWidget->item(i, COLUMN_TEMPO)->text();

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
    if (PlaylistFileName.endsWith(CSV_FILE_EXTENSION) && 
        !PlaylistFileName.contains("/Tracks/") && 
        !PlaylistFileName.contains(APPLE_MUSIC_PATH_PREFIX)) {
        updateRecentPlaylistsList(PlaylistFileName);
    }
}

// ----------------------------
void MainWindow::updateRecentPlaylistsList(const QString &playlistPath)
{
    // Convert full path to relative path for storage
    QString relativePath = playlistPath;
    QString playlistsPrefix = musicRootPath + PLAYLISTS_PATH_PREFIX;
    
    // Remove the musicRootPath + PLAYLISTS_PATH_PREFIX prefix and CSV_FILE_EXTENSION suffix
    if (relativePath.startsWith(playlistsPrefix)) {
        relativePath = relativePath.mid(playlistsPrefix.length());
    }
    if (relativePath.endsWith(CSV_FILE_EXTENSION)) {
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
    while (recentPlaylists.size() > MAX_RECENT_PLAYLISTS) {
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
        rec.title = table->item(i, COLUMN_PATH)->text(); // fullPath
        rec.pitch = table->item(i, COLUMN_PITCH)->text();
        rec.tempo = table->item(i, COLUMN_TEMPO)->text();

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
    for (const auto& rec : exports)
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
    QTableWidget *playlistTables[3] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};

    for (int i = 0; i < 3; i++) {
        // qDebug() << "LOOK AT SLOT:" << i;
        QTableWidget *theTable = playlistTables[i];

        QItemSelectionModel *selectionModel = theTable->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();
        int row = -1;

        if (selected.count() == 1) {
            // exactly 1 row was selected (good)
            QModelIndex index = selected.at(0);
            row = index.row();

            // qDebug() << "Table:" << i << ", Row:" << row << " DOUBLE CLICKED";

            QString path = theTable->item(row, COLUMN_PATH)->text();
            // qDebug() << "Path: " << path;

            // pretend we clicked on the tempo label of that row, and let the existing functions do the work...
            handlePlaylistDoubleClick(theTable->item(row, COLUMN_TEMPO));

            break; // break out of the for loop, because we found what was clicked on
        } // else more than 1 row or no rows, just return -1

    }
}

void MainWindow::refreshAllPlaylists() {
    // qDebug() << "Refreshing playlist views, because TAGS visibility changed.";
    // bool tagsAreVisible = ui->actionViewTags->isChecked();

    QTableWidget *playlistTables[3] = {ui->playlist1Table, ui->playlist2Table, ui->playlist3Table};

    for (int i = 0; i < 3; i++) {
        // qDebug() << "LOOK AT SLOT:" << i;
        QTableWidget *theTable = playlistTables[i];

        for (int j = 0; j < theTable->rowCount(); j++) {
            // for each title in the table
            QString relativePath;
            QString PlaylistFileName;
            if (dynamic_cast<QLabel*>(theTable->cellWidget(j, 1)) != nullptr) {
                // QString title = dynamic_cast<QLabel*>(theTable->cellWidget(j, 1))->text(); // don't need this right now...
                relativePath = theTable->item(j, COLUMN_PATH)->text();
                relativePath.replace(musicRootPath, "");

                PlaylistFileName = musicRootPath + PLAYLISTS_PATH_PREFIX + relPathInSlot[i] + CSV_FILE_EXTENSION;
                // qDebug() << "refreshAllPlaylists: relativePath, PlaylistFileName:" << relativePath << PlaylistFileName;

                // Determine if this row should be indented (issue #1547)
                bool shouldIndent = shouldIndentPlaylistRow(theTable, j);

                // this is what we want: "/patter/RIV 1180 - Sea Chanty.mp3" "/Users/mpogue/Library/CloudStorage/Box-Box/__squareDanceMusic_Box/playlists/Jokers/2024/Jokers_2024.06.05.csv"
                setTitleField(theTable, j, relativePath, true, PlaylistFileName, relativePath, shouldIndent);
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

    // NOTE: THIS NEEDS TO MATCH STEPS 1d and 5 IN MP3FilenameVsCuesheetnameScore()
    //   TODO: Factor this code out into a separate function
    //

    // Step 1d: note that this requires capitalized "NB" and no spaces for it to work
    static QRegularExpression NewBeatAndNumber("NB-([0-9]?)");
    name.replace(NewBeatAndNumber, "NB \\1");

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
    destTableWidget->setItem(insertRowNum, COLUMN_NUMBER, num);

    // Now we have to renumber all the rows BELOW insertRowNum, to add 1 to them
    for (int i = insertRowNum+1; i < songCount; i++) {
        int currentNum = destTableWidget->item(i, COLUMN_NUMBER)->text().toInt();
        // qDebug() << "currentNum:" << currentNum;
        destTableWidget->item(i, COLUMN_NUMBER)->setText(QString::number(currentNum+1));
    }

    // TITLE column
    QString theRelativePath = theFullPath;
    QString absPath = theFullPath; // already is fully qualified

    theRelativePath.replace(musicRootPath, "");
    // qDebug() << "darkAddPlaylistItemAt calling setTitleField" << theRelativePath;

    QString theCanonicalRelativePath = makeCanonicalRelativePath(theRelativePath);

    // First set the PATH column so shouldIndentPlaylistRow() can check it
    // PATH column (set early for indentation calculation)
    QTableWidgetItem *tempPath = new QTableWidgetItem(absPath);
    destTableWidget->setItem(insertRowNum, COLUMN_PATH, tempPath);

    // Determine if this row should be indented (issue #1547)
    bool shouldIndent = shouldIndentPlaylistRow(destTableWidget, insertRowNum);

    setTitleField(destTableWidget, insertRowNum, theCanonicalRelativePath, true, PlaylistFileName, theRelativePath, shouldIndent); // whichTable, whichRow, fullPath, bool isPlaylist, PlaylistFilename (for errors)

    // PITCH column
    QTableWidgetItem *pit = new QTableWidgetItem(thePitch);
    destTableWidget->setItem(insertRowNum, COLUMN_PITCH, pit);

    // TEMPO column
    QTableWidgetItem *tem = new QTableWidgetItem(theTempo);
    destTableWidget->setItem(insertRowNum, COLUMN_TEMPO, tem);

    // PATH column already set above for indentation calculation

    // LOADED column
    QTableWidgetItem *loaded = new QTableWidgetItem("");
    destTableWidget->setItem(insertRowNum, COLUMN_LOADED, loaded);

    destTableWidget->resizeColumnToContents(COLUMN_NUMBER); // FIX: perhaps only if this is the first row?
    //    theTableWidget->resizeColumnToContents(COLUMN_PITCH);
    //    theTableWidget->resizeColumnToContents(COLUMN_TEMPO);

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
    auto [theTableWidget, theLabel] = getSlotWidgets(slotNumber);

    theTableWidget->setRowCount(0); // delete all the rows in the slot
    theLabel->setText(QString("<img src=\":/graphics/icons8-menu-64.png\" width=\"%1\" height=\"%2\">Untitled playlist").arg(PLAYLIST_ICON_WIDTH).arg(PLAYLIST_ICON_HEIGHT)); // clear out the label
    slotModified[slotNumber] = false;  // not modified now
    relPathInSlot[slotNumber] = "";    // nobody home now
    // qDebug() << "clearSlot" << slotNumber;
}

void MainWindow::on_action0paletteSlots_triggered()
{
    setPaletteSlotVisibility(0);
}

void MainWindow::on_action1paletteSlots_triggered()
{
    setPaletteSlotVisibility(1);
}

void MainWindow::on_action2paletteSlots_triggered()
{
    setPaletteSlotVisibility(2);
}

void MainWindow::on_action3paletteSlots_triggered()
{
    setPaletteSlotVisibility(3);
}

// ================================================================================
// Add songs to a playlist file (from tree widget drag & drop)
// ================================================================================
bool MainWindow::addItemsToPlaylistFile(const QString &playlistRelPath, const QList<SongDragInfo> &songs)
{
    if (songs.isEmpty()) {
        return false;
    }

    // Build full path to the playlist file
    QString playlistFullPath = musicRootPath + "/playlists/" + playlistRelPath + ".csv";

    // qDebug() << "Adding" << songs.count() << "songs to playlist:" << playlistFullPath;

    // Read existing playlist contents
    QStringList existingLines;
    QFile readFile(playlistFullPath);

    if (readFile.exists() && readFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&readFile);
        while (!in.atEnd()) {
            existingLines.append(in.readLine());
        }
        readFile.close();
    } else {
        // File doesn't exist yet, create header
        existingLines.append(CSV_HEADER_STRING);
    }

    // Ensure we have the header
    if (existingLines.isEmpty() || existingLines.first() != CSV_HEADER_STRING) {
        existingLines.prepend(CSV_HEADER_STRING);
    }

    // Append new songs
    for (const auto &song : songs) {
        // Convert full path to relative path
        QString relPath = song.path;
        relPath = relPath.replace(musicRootPath, "");

        // Escape quotes in path
        relPath = relPath.replace("\"", "\"\"");

        // Format: "relpath",pitch,tempo
        QString line = QString("\"%1\",%2,%3").arg(relPath).arg(song.pitch).arg(song.tempo);
        existingLines.append(line);
    }

    // Write back to file
    QFile writeFile(playlistFullPath);
    if (!writeFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qDebug() << "ERROR: Could not open playlist file for writing:" << playlistFullPath;
        return false;
    }

    QTextStream out(&writeFile);
    for (const auto &line : std::as_const(existingLines)) {
        out << line << ENDL;
    }
    writeFile.close();

    // Check if this playlist is currently loaded in any palette slot and refresh it
    for (int slot = 0; slot < 3; slot++) {
        QString slotRelPath = relPathInSlot[slot];
        if (!slotRelPath.isEmpty() && slotRelPath == playlistRelPath) {
            // This playlist is loaded in a slot, reload it
            // qDebug() << "Playlist is in slot" << slot << ", reloading...";

            int songCount = 0;
            loadPlaylistFromFileToPaletteSlot(playlistFullPath, slot, songCount);
            slotModified[slot] = false; // Mark as not modified since we just saved
        }
    }

    // Refresh the tree widget and song table if needed
    getLocalPlaylists(); // Refresh tree widget

    return true;
}
