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
#include "songlistmodel.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
#include <QCoreApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QClipboard>
#include <QtSvg/QSvgGenerator>
#include <algorithm>  // for random_shuffle

// FONT SIZE STUFF ========================================

void MainWindow::setSongTableFont(QTableWidget *songTable, const QFont &currentFont)
{
    currentSongTableFont = currentFont;
    if (!darkmode) {
        for (int row = 0; row < songTable->rowCount(); ++row)
            dynamic_cast<QLabel*>(songTable->cellWidget(row,kTitleCol))->setFont(currentFont);
    }
}

int MainWindow::pointSizeToIndex(int pointSize) {
    // converts our old-style range: 11,13,15,17,19,21,23,25
    //   to an index:                 0, 1, 2, 3, 4, 5, 6, 7
    // returns -1 if not in range, or if even number
    if (pointSize < 11 || pointSize > 25 || pointSize % 2 != 1) {
        return(-1);
    }
    return((pointSize-11)/2);
}

int MainWindow::indexToPointSize(int index) {
#if defined(Q_OS_MAC)
    return (2*index + 11);
#elif defined(Q_OS_WIN)
    return static_cast<int>((8.0/13.0)*(static_cast<double>(2*index + 11)));
#elif defined(Q_OS_LINUX)
    return (2*index + 11);
#endif
}

void MainWindow::setFontSizes()
{
    // initial font sizes (with no zoom in/out)

#if defined(Q_OS_MAC)
    preferredVerySmallFontSize = 13;
    preferredSmallFontSize = 13;
    preferredWarningLabelFontSize = 20;
    preferredNowPlayingFontSize = 27;
#elif defined(Q_OS_WIN32)
    preferredVerySmallFontSize = 8;
    preferredSmallFontSize = 8;
    preferredWarningLabelFontSize = 12;
    preferredNowPlayingFontSize = 16;
#elif defined(Q_OS_LINUX)
    preferredVerySmallFontSize = 8;
    preferredSmallFontSize = 13;
    preferredWarningLabelFontSize = 20;
    preferredNowPlayingFontSize = 27;
#endif

    QFont font = ui->currentTempoLabel->font();  // system font for most everything

    // preferred very small text
    font.setPointSize(preferredVerySmallFontSize);
    ui->typeSearch->setFont(font);
    ui->labelSearch->setFont(font);
    ui->titleSearch->setFont(font);
    ui->lineEditSDInput->setFont(font);  // SD Input box needs to resize, too.
    ui->clearSearchButton->setFont(font);
    ui->songTable->setFont(font);

    // preferred normal small text
    font.setPointSize(preferredSmallFontSize);
    ui->tabWidget->setFont(font);  // most everything inherits from this one
    ui->statusBar->setFont(font);
    micStatusLabel->setFont(font);
    ui->currentLocLabel->setFont(font);
    ui->songLengthLabel->setFont(font);

    ui->bassLabel->setFont(font);
    ui->midrangeLabel->setFont(font);
    ui->trebleLabel->setFont(font);
//    ui->EQgroup->setFont(font);

    ui->pushButtonCueSheetEditTitle->setFont(font);
    ui->pushButtonCueSheetEditLabel->setFont(font);
    ui->pushButtonCueSheetEditArtist->setFont(font);
    ui->pushButtonCueSheetEditHeader->setFont(font);
    ui->pushButtonCueSheetEditBold->setFont(font);
    ui->pushButtonCueSheetEditItalic->setFont(font);


    // preferred Warning Label (medium sized)
    font.setPointSize(preferredWarningLabelFontSize);
    ui->warningLabel->setFont(font);
    ui->warningLabelCuesheet->setFont(font);
    ui->warningLabelSD->setFont(font); // SD warning label (really the TIMER label) needs to be MEDIUM
    ui->currentLocLabel_2->setFont(font);

    // preferred Now Playing (large sized)
    font.setPointSize(preferredNowPlayingFontSize);
    ui->nowPlayingLabel->setFont(font);
}

void MainWindow::adjustFontSizes()
{
    // ui->songTable->resizeColumnToContents(kNumberCol);  // nope
    ui->songTable->resizeColumnToContents(kTypeCol);
    ui->songTable->resizeColumnToContents(kLabelCol);
    // kTitleCol = nope

    // QFont currentFont = ui->songTable->font();

    QFont currentFont = currentSongTableFont;

    int currentFontPointSize = currentFont.pointSize();  // platform-specific point size

    int index = pointSizeToIndex(currentMacPointSize);  // current index

    // give a little extra space when sorted...
    int sortedSection = ui->songTable->horizontalHeader()->sortIndicatorSection();

    // pixel perfection for each platform
#if defined(Q_OS_MAC)
    double extraColWidth[8] = {0.25, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0};

    double numberBase = 2.0;
    double recentBase = 4.5;
    double ageBase = 3.5;
    double pitchBase = 4.0;
    double tempoBase = 4.5;

    double numberFactor = 0.5;
    double recentFactor = 0.9;
    double ageFactor = 0.5;
    double pitchFactor = 0.5;
    double tempoFactor = 0.9;

    int searchBoxesHeight[8] = {20, 21, 22, 24,  26, 28, 30, 32};
    double scaleWidth1 = 7.75;
    double scaleWidth2 = 3.25;
    double scaleWidth3 = 8.5;

    // lyrics buttons
    unsigned int TitleButtonWidth[8] = {55,60,65,70, 80,90,95,105};

    double maxEQsize = 16.0;
    double scaleIcons = 24.0/13.0;

    int warningLabelSize[8] = {16,20,23,26, 29,32,35,38};  // basically 20/13 * pointSize
//    unsigned int warningLabelWidth[8] = {93,110,126,143, 160,177,194,211};  // basically 20/13 * pointSize * 5.5
    int warningLabelWidth[8] = {65,75,85,95, 105,115,125,135};  // basically 20/13 * pointSize * 5.5

    int nowPlayingSize[8] = {22,27,31,35, 39,43,47,51};  // basically 27/13 * pointSize

    double nowPlayingHeightFactor = 1.5;

    double buttonSizeH = 1.875;
    double buttonSizeV = 1.125;
#elif defined(Q_OS_WIN32)
    double extraColWidth[8] = {0.25, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0};

    double numberBase = 1.5;
    double recentBase = 7.5;
    double ageBase = 5.5;
    double pitchBase = 6.0;
    double tempoBase = 7.5;

    double numberFactor = 0.0;
    double recentFactor = 0.0;
    double ageFactor = 0.0;
    double pitchFactor = 0.0;
    double tempoFactor = 0.0;

    int searchBoxesHeight[8] = {22, 26, 30, 34,  38, 42, 46, 50};
    double scaleWidth1 = 12.75;
    double scaleWidth2 = 5.25;
    double scaleWidth3 = 10.5;

    // lyrics buttons
    unsigned int TitleButtonWidth[8] = {55,60,65,70, 80,90,95,105};

    double maxEQsize = 16.0;
    double scaleIcons = (1.5*24.0/13.0);
    int warningLabelSize[8] = {9,12,14,16, 18,20,22,24};  // basically 20/13 * pointSize
    int warningLabelWidth[8] = {84,100,115,130, 146, 161, 176, 192};  // basically 20/13 * pointSize * 5

//    unsigned int nowPlayingSize[8] = {22,27,31,35, 39,43,47,51};  // basically 27/13 * pointSize
    int nowPlayingSize[8] = {16,20,23,26, 29,32,35,38};  // basically 27/13 * pointSize

    double nowPlayingHeightFactor = 1.5;

    double buttonSizeH = 1.5*1.875;
    double buttonSizeV = 1.5*1.125;
#elif defined(Q_OS_LINUX)
    double extraColWidth[8] = {0.25, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0};

    double numberBase = 2.0;
    double recentBase = 4.5;
    double ageBase = 3.5;
    double pitchBase = 4.0;
    double tempoBase = 4.5;

    double numberFactor = 0.5;
    double recentFactor = 0.9;
    double ageFactor = 0.5;
    double pitchFactor = 0.5;
    double tempoFactor = 0.9;

    int searchBoxesHeight[8] = {22, 26, 30, 34,  38, 42, 46, 50};
    double scaleWidth1 = 7.75;
    double scaleWidth2 = 3.25;
    double scaleWidth3 = 8.5;

    // lyrics buttons
    unsigned int TitleButtonWidth[8] = {55,60,65,70, 80,90,95,105};

    double maxEQsize = 16.0;
    double scaleIcons = 24.0/13.0;
    int warningLabelSize[8] = {16,20,23,26, 29,32,35,38};  // basically 20/13 * pointSize
    int warningLabelWidth[8] = {93,110,126,143, 160,177,194,211};  // basically 20/13 * pointSize * 5.5

    int nowPlayingSize[8] = {22,27,31,35, 39,43,47,51};  // basically 27/13 * pointSize

    double nowPlayingHeightFactor = 1.5;

    double buttonSizeH = 1.875;
    double buttonSizeV = 1.125;
#endif

    // a little extra space when a column is sorted
    // also a little extra space for the smallest zoom size

    double extraWidth = (index != -1 ? extraColWidth[index] : 0.0);  // get rid of the error case where index returns as -1, make extraWidth zero in that case

    ui->songTable->setColumnWidth(kNumberCol, static_cast<int>((numberBase + (sortedSection==kNumberCol?numberFactor:0.0)) *currentFontPointSize));
    ui->songTable->setColumnWidth(kRecentCol, static_cast<int>((recentBase+(sortedSection==kRecentCol?recentFactor:0.0)+extraWidth)*currentFontPointSize));
    ui->songTable->setColumnWidth(kAgeCol, static_cast<int>((ageBase+(sortedSection==kAgeCol?ageFactor:0.0)+extraWidth)*currentFontPointSize));
    ui->songTable->setColumnWidth(kPitchCol, static_cast<int>((pitchBase+(sortedSection==kPitchCol?pitchFactor:0.0)+extraWidth)*currentFontPointSize));
    ui->songTable->setColumnWidth(kTempoCol, static_cast<int>((tempoBase+(sortedSection==kTempoCol?tempoFactor:0.0)+extraWidth)*currentFontPointSize));

    int searchBoxHeight = (index != -1 ? searchBoxesHeight[index] : searchBoxesHeight[2]); // if index == -1 because error, use something in the middle
    ui->typeSearch->setFixedHeight(searchBoxHeight);
    ui->labelSearch->setFixedHeight(searchBoxHeight);
    ui->titleSearch->setFixedHeight(searchBoxHeight);
    ui->lineEditSDInput->setFixedHeight(searchBoxHeight);

    ui->dateTimeEditIntroTime->setFixedHeight(searchBoxHeight);  // this scales the intro/outro button height, too...
    ui->dateTimeEditOutroTime->setFixedHeight(searchBoxHeight);

#if defined(Q_OS_MAC)
    // the Mac combobox is not height resizeable.  This styled one is, and it looks fine.
    ui->comboBoxCuesheetSelector->setStyle(QStyleFactory::create("Windows"));
    ui->comboBoxCallListProgram->setStyle(QStyleFactory::create("Windows"));
#endif

    // set all the related fonts to the same size
    ui->typeSearch->setFont(currentFont);
    ui->labelSearch->setFont(currentFont);
    ui->titleSearch->setFont(currentFont);
    ui->lineEditSDInput->setFont(currentFont);

    ui->tempoLabel->setFont(currentFont);
    ui->pitchLabel->setFont(currentFont);
    ui->volumeLabel->setFont(currentFont);
    ui->mixLabel->setFont(currentFont);

    ui->currentTempoLabel->setFont(currentFont);
    ui->currentPitchLabel->setFont(currentFont);
    ui->currentVolumeLabel->setFont(currentFont);
    ui->currentMixLabel->setFont(currentFont);

    int newCurrentWidth = static_cast<int>(scaleWidth1 * currentFontPointSize);
    ui->currentTempoLabel->setFixedWidth(newCurrentWidth);
    ui->currentPitchLabel->setFixedWidth(newCurrentWidth);
    ui->currentVolumeLabel->setFixedWidth(newCurrentWidth);
    ui->currentMixLabel->setFixedWidth(newCurrentWidth);

    ui->statusBar->setFont(currentFont);
    micStatusLabel->setFont(currentFont);

    ui->currentLocLabel->setFont(currentFont);
    ui->songLengthLabel->setFont(currentFont);

    ui->currentLocLabel->setFixedWidth(static_cast<int>(scaleWidth2 * currentFontPointSize));
    ui->songLengthLabel->setFixedWidth(static_cast<int>(scaleWidth2 * currentFontPointSize));

    ui->clearSearchButton->setFont(currentFont);
    ui->clearSearchButton->setFixedWidth(static_cast<int>(scaleWidth3 * currentFontPointSize));

    ui->tabWidget->setFont(currentFont);  // most everything inherits from this one

    ui->pushButtonCueSheetEditTitle->setFont(currentFont);
    ui->pushButtonCueSheetEditLabel->setFont(currentFont);
    ui->pushButtonCueSheetEditArtist->setFont(currentFont);
    ui->pushButtonCueSheetEditHeader->setFont(currentFont);
    ui->pushButtonCueSheetEditLyrics->setFont(currentFont);
    ui->pushButtonCueSheetClearFormatting->setFont(currentFont);

    ui->pushButtonEditLyrics->setFont(currentFont);
    ui->pushButtonCueSheetEditSave->setFont(currentFont);
    ui->pushButtonCueSheetEditSaveAs->setFont(currentFont);
    ui->pushButtonRevertEdits->setFont(currentFont);

    ui->pushButtonCueSheetEditBold->setFont(currentFont);
    ui->pushButtonCueSheetEditItalic->setFont(currentFont);

    ui->pushButtonClearTaughtCalls->setFont(currentFont);

    ui->pushButtonSetIntroTime->setFont(currentFont);
    ui->pushButtonSetOutroTime->setFont(currentFont);

    unsigned int titleButtonW = (index != -1 ? TitleButtonWidth[index] : TitleButtonWidth[2]); // if error, use something in the middle
    ui->pushButtonClearTaughtCalls->setFixedWidth(static_cast<int>(titleButtonW * 1.5));

    ui->pushButtonCueSheetEditTitle->setFixedWidth(static_cast<int>(titleButtonW));
    ui->pushButtonCueSheetEditLabel->setFixedWidth(static_cast<int>(titleButtonW));
    ui->pushButtonCueSheetEditArtist->setFixedWidth(static_cast<int>(titleButtonW));
    ui->pushButtonCueSheetEditHeader->setFixedWidth(static_cast<int>(titleButtonW * 1.5));
    ui->pushButtonCueSheetEditLyrics->setFixedWidth(static_cast<int>(titleButtonW));

    ui->pushButtonCueSheetEditBold->setFixedWidth(static_cast<int>(titleButtonW/2));
    ui->pushButtonCueSheetEditItalic->setFixedWidth(static_cast<int>(titleButtonW/2));
    ui->pushButtonCueSheetClearFormatting->setFixedWidth(static_cast<int>(titleButtonW * 2));

    ui->pushButtonEditLyrics->setFixedWidth(static_cast<int>(titleButtonW * 2));
    ui->pushButtonCueSheetEditSave->setFixedWidth(static_cast<int>(titleButtonW * 0.7));
    ui->pushButtonCueSheetEditSaveAs->setFixedWidth(static_cast<int>(titleButtonW * 1.2));
    ui->pushButtonRevertEdits->setFixedWidth(static_cast<int>(titleButtonW * 1.5));

    ui->label_4->setFont(currentFont);
    ui->comboBoxCallListProgram->setFont(currentFont);

    ui->tableWidgetCallList->setFont(currentFont);
    ui->tableWidgetCallList->horizontalHeader()->setFont(currentFont);
    ui->songTable->horizontalHeader()->setFont(currentFont);
    ui->songTable->horizontalHeader()->setFixedHeight(searchBoxHeight); // protected against index == -1
//    qDebug() << "setting font to: " << currentFont;

    ui->tableWidgetCallList->setColumnWidth(kCallListOrderCol,static_cast<int>(32*(currentMacPointSize/13.0)));
    ui->tableWidgetCallList->setColumnWidth(kCallListCheckedCol, static_cast<int>(24*(currentMacPointSize/13.0)));
    ui->tableWidgetCallList->setColumnWidth(kCallListWhenCheckedCol, static_cast<int>(75*(currentMacPointSize/13.0)));
    ui->tableWidgetCallList->setColumnWidth(kCallListTimingCol, static_cast<int>(200*(currentMacPointSize/13.0)));

    ui->tableWidgetCallList->horizontalHeader()->setSectionResizeMode(kCallListNameCol,        QHeaderView::Interactive);
    ui->tableWidgetCallList->horizontalHeader()->setSectionResizeMode(kCallListWhenCheckedCol, QHeaderView::Interactive);
    ui->tableWidgetCallList->horizontalHeader()->setSectionResizeMode(kCallListTimingCol,      QHeaderView::Interactive);

    // these are special -- don't want them to get too big, even if user requests huge fonts
    currentFont.setPointSize(currentFontPointSize > maxEQsize ? static_cast<int>(maxEQsize) : currentFontPointSize);  // no bigger than 20pt
    ui->bassLabel->setFont(currentFont);
    ui->midrangeLabel->setFont(currentFont);
    ui->trebleLabel->setFont(currentFont);
//    ui->EQgroup->setFont(currentFont);

    // resize the icons for the buttons
    int newIconDimension = static_cast<int>(currentFontPointSize * scaleIcons);
    QSize newIconSize(newIconDimension, newIconDimension);
    ui->stopButton->setIconSize(newIconSize);
    ui->playButton->setIconSize(newIconSize);
    ui->previousSongButton->setIconSize(newIconSize);
    ui->nextSongButton->setIconSize(newIconSize);

    //ui->pushButtonEditLyrics->setIconSize(newIconSize);

    // these are special MEDIUM
    int warningLabelFontSize = warningLabelSize[(index != -1 ? index : 2)]; // keep ratio constant
    currentFont.setPointSize(warningLabelFontSize);
    ui->warningLabel->setFont(currentFont);
    ui->warningLabelCuesheet->setFont(currentFont);
    ui->warningLabelSD->setFont(currentFont); // same size as the others
    ui->currentLocLabel_2->setFont(currentFont);

//    int warningLabelFontSizeSD = warningLabelSize[0]; // special small font size for SD warning label (TIMER)
//    currentFont.setPointSize(warningLabelFontSizeSD);
//    ui->warningLabelSD->setFont(currentFont);

    ui->warningLabel->setFixedWidth(warningLabelWidth[(index != -1 ? index : 2)]);
    ui->warningLabelCuesheet->setFixedWidth(warningLabelWidth[(index != -1 ? index : 2)]);
    ui->warningLabelSD->setFixedWidth(warningLabelWidth[(index != -1 ? index : 2)]);  // this one is just like the others
    //    ui->warningLabelSD->setFixedWidth(warningLabelWidth[0]); // special small width for SD warning label (TIMER)

    ui->currentLocLabel_2->setFixedWidth(warningLabelWidth[(index != -1 ? index : 2)]);


    // these are special BIG
    int nowPlayingLabelFontSize = (nowPlayingSize[(index != -1 ? index : 2)]); // keep ratio constant
    currentFont.setPointSize(nowPlayingLabelFontSize);
    ui->nowPlayingLabel->setFont(currentFont);
    ui->nowPlayingLabel->setFixedHeight(static_cast<int>(nowPlayingHeightFactor * nowPlayingLabelFontSize));

    // BUTTON SIZES ---------
    ui->stopButton->setFixedSize(static_cast<int>(buttonSizeH*nowPlayingLabelFontSize), static_cast<int>(buttonSizeV*nowPlayingLabelFontSize));
    ui->playButton->setFixedSize(static_cast<int>(buttonSizeH*nowPlayingLabelFontSize), static_cast<int>(buttonSizeV*nowPlayingLabelFontSize));
    ui->previousSongButton->setFixedSize(static_cast<int>(buttonSizeH*nowPlayingLabelFontSize), static_cast<int>(buttonSizeV*nowPlayingLabelFontSize));
    ui->nextSongButton->setFixedSize(static_cast<int>(buttonSizeH*nowPlayingLabelFontSize), static_cast<int>(buttonSizeV*nowPlayingLabelFontSize));
}


void MainWindow::usePersistentFontSize() {
    PerfTimer t("usePersistentFontSize", __LINE__);
    int newPointSize = prefsManager.GetsongTableFontSize(); // gets the persisted value
    if (newPointSize == 0) {
        newPointSize = 13;  // default backstop, if not set properly
    }

    t.elapsed(__LINE__);

    // ensure it is a reasonable size...
    newPointSize = (newPointSize > BIGGESTZOOM ? BIGGESTZOOM : newPointSize);
    newPointSize = (newPointSize < SMALLESTZOOM ? SMALLESTZOOM : newPointSize);

    //qDebug() << "usePersistentFontSize: " << newPointSize;

    t.elapsed(__LINE__);

    QFont currentFont = ui->songTable->font();  // set the font size in the songTable
//    qDebug() << "index: " << pointSizeToIndex(newPointSize);
    int platformPS = indexToPointSize(pointSizeToIndex(newPointSize));  // convert to PLATFORM pointsize
//    qDebug() << "platformPS: " << platformPS;
    currentFont.setPointSize(platformPS);

    int indexToCuesheetZoom[9] = {-4,0,4,8,12,16,20,24,24};
    int index = pointSizeToIndex(newPointSize);
    // qDebug() << "usePersistentFontSize: " << index << totalZoom;
    totalZoom = indexToCuesheetZoom[index];
    ui->textBrowserCueSheet->zoomIn(totalZoom);

    if (!darkmode) {
        ui->songTable->setFont(currentFont);
    }
    currentMacPointSize = newPointSize;

    t.elapsed(__LINE__);

    if (!darkmode) {
        ui->songTable->setStyleSheet(QString("QTableWidget::item:selected{ color: #FFFFFF; background-color: #4C82FC } QHeaderView::section { font-size: %1pt; }").arg(platformPS));
    }

    t.elapsed(__LINE__);

    setSongTableFont(ui->songTable, currentFont);
    t.elapsed(__LINE__);

    adjustFontSizes();  // use that font size to scale everything else (relative)
    t.elapsed(__LINE__);
}


void MainWindow::persistNewFontSize(int currentMacPointSize) {
//    qDebug() << "NOT PERSISTING: " << currentMacPointSize;
//    return;
    prefsManager.SetsongTableFontSize(currentMacPointSize);  // persist this
//    qDebug() << "persisting new Mac font size: " << currentMacPointSize;
}

void MainWindow::zoomInOut(int increment) {
    PerfTimer t("zoomInOut", __LINE__);

    // QFont currentFont = ui->songTable->font();

    QFont currentFont = currentSongTableFont;

    int newPointSize = currentMacPointSize + increment;

    t.elapsed(__LINE__);
    newPointSize = (newPointSize > BIGGESTZOOM ? BIGGESTZOOM : newPointSize);
    newPointSize = (newPointSize < SMALLESTZOOM ? SMALLESTZOOM : newPointSize);

    t.elapsed(__LINE__);
    if (newPointSize > currentMacPointSize) {
        ui->textBrowserCueSheet->zoomIn(2*ZOOMINCREMENT);
        totalZoom += 2*ZOOMINCREMENT;
    } else if (newPointSize < currentMacPointSize) {
        ui->textBrowserCueSheet->zoomIn(-2*ZOOMINCREMENT);
        totalZoom -= 2*ZOOMINCREMENT;
    }

    // qDebug() << "totalZoom is now: " << totalZoom << ", and currentMacPointSize: " << currentMacPointSize;

    t.elapsed(__LINE__);
    int platformPS = indexToPointSize(pointSizeToIndex(newPointSize));  // convert to PLATFORM pointsize
    currentFont.setPointSize(platformPS);

    if (!darkmode) {
        ui->songTable->setFont(currentFont);
    }
    currentMacPointSize = newPointSize;

    t.elapsed(__LINE__);
    persistNewFontSize(currentMacPointSize);

    t.elapsed(__LINE__);
    startLongSongTableOperation("zoomInOut");
    if (!darkmode) {
        ui->songTable->setStyleSheet(QString("QTableWidget::item:selected{ color: #FFFFFF; background-color: #4C82FC } QHeaderView::section { font-size: %1pt; }").arg(platformPS));
    }

    t.elapsed(__LINE__);
    setSongTableFont(ui->songTable, currentFont);
    t.elapsed(__LINE__);
    adjustFontSizes();
    stopLongSongTableOperation("zoomInOut");
    t.elapsed(__LINE__);
//    qDebug() << "currentMacPointSize:" << newPointSize << ", totalZoom:" << totalZoom;
}

void MainWindow::on_actionZoom_In_triggered()
{
    zoomInOut(ZOOMINCREMENT);
}

void MainWindow::on_actionZoom_Out_triggered()
{
    zoomInOut(-ZOOMINCREMENT);
}

void MainWindow::on_actionReset_triggered()
{
    QFont currentFont;  // system font, and system default point size

    currentMacPointSize = 13; // by definition
    int platformPS = indexToPointSize(pointSizeToIndex(currentMacPointSize));  // convert to PLATFORM pointsize

    currentFont.setPointSize(platformPS);
    ui->songTable->setFont(currentFont);

    ui->textBrowserCueSheet->zoomOut(totalZoom);  // undo all zooming in the lyrics pane, by zooming OUT
    totalZoom = 0;

    persistNewFontSize(currentMacPointSize);
    setSongTableFont(ui->songTable, currentFont);
    adjustFontSizes();
    // qDebug() << "on_actionReset_triggered = currentMacPointSize:" << currentMacPointSize << ", totalZoom:" << totalZoom;
}
