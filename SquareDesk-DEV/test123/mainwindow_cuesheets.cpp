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
#include "downloadmanager.h"
#include "songlistmodel.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"
#include "utility.h"
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
#include <QCoreApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QClipboard>
#include <QtSvg/QSvgGenerator>
#include <algorithm>  // for random_shuffle

struct FilenameMatchers {
    QRegularExpression regex;
    int title_match;
    int label_match;
    int number_match;
    int additional_label_match;
    int additional_title_match;
};

struct FilenameMatchers *getFilenameMatchersForType(enum SongFilenameMatchingType songFilenameFormat)
{
    static struct FilenameMatchers best_guess_matches[] = {
        { QRegularExpression("^([Oo][Gg][Rr][Mm][Pp]3\\s*\\d{1,5})\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },    // e.g. "OGRMP3 04 - Addam's Family.mp3"
        { QRegularExpression("^(4-[Bb][Aa][Rr]-[Bb]\\s*\\d{1,5})\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },    // e.g. "4-bar-b 123 - Chicken Plucker"
        { QRegularExpression("^(.*) - ([A-Z]+[\\- ]\\d+)( *-?[VMA-C]|\\-\\d+)?$"), 1, 2, -1, 3, -1 },
        { QRegularExpression("^([A-Z]+[\\- ]\\d+)(-?[VvMA-C]?) - (.*)$"), 3, 1, -1, 2, -1 },
        { QRegularExpression("^([A-Z]+ ?\\d+)([MV]?)[ -]+(.*)$/"), 3, 1, -1, 2, -1 },
        { QRegularExpression("^([A-Z]?[0-9][A-Z]+[\\- ]?\\d+)([MV]?)[ -]+(.*)$"), 3, 1, -1, 2, -1 },
        { QRegularExpression("^(.*) - ([A-Z]{1,5}+)[\\- ](\\d+)( .*)?$"), 1, 2, 3, -1, 4 },
        { QRegularExpression("^([A-Z]+ ?\\d+)([ab])?[ -]+(.*)$/"), 3, 1, -1, 2, -1 },
        { QRegularExpression("^([A-Z]+\\-\\d+)\\-(.*)/"), 2, 1, -1, -1, -1 },
//    { QRegularExpression("^(\\d+) - (.*)$"), 2, -1, -1, -1, -1 },         // first -1 prematurely ended the search (typo?)
//    { QRegularExpression("^(\\d+\\.)(.*)$"), 2, -1, -1, -1, -1 },         // first -1 prematurely ended the search (typo?)
        { QRegularExpression("^(\\d+)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },  // e.g. "123 - Chicken Plucker"
        { QRegularExpression("^(\\d+\\.)(.*)$"), 2, 1, -1, -1, -1 },            // e.g. "123.Chicken Plucker"
//        { QRegularExpression("^(.*?) - (.*)$"), 2, 1, -1, -1, -1 },           // ? is a non-greedy match (So that "A - B - C", first group only matches "A")
        { QRegularExpression("^([A-Z]{1,5}+[\\- ]*\\d+[A-Z]*)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 }, // e.g. "ABC 123-Chicken Plucker"
        { QRegularExpression("^([A-Z]{1,5}+[\\- ]*\\d+[A-Za-z0-9]*)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 }, // e.g. "ABC 123h1-Chicken Plucker"
        { QRegularExpression("^([A-Z0-9]{1,5}+)\\s*(\\d+)([a-zA-Z]{1,2})?\\s*-\\s*(.*?)\\s*(\\(.*\\))?$"), 4, 1, 2, 3, 5 }, // SIR 705b - Papa Was A Rollin Stone (Instrumental).mp3
        { QRegularExpression("^([A-Z0-9]{1,5}+)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },    // e.g. "POP - Chicken Plucker" (if it has a dash but fails all other tests,
        { QRegularExpression("^(.*?)\\s*\\-\\s*([A-Z]{1,5})(\\d{1,5})\\s*(\\(.*\\))?$"), 1, 2, 3, -1, 4 },    // e.g. "A Summer Song - CHIC3002 (female vocals)
        { QRegularExpression("^(.*?)\\s*\\-\\s*([A-Za-z]{1,7}|4-[Bb][Aa][Rr]-[Bb])-(\\d{1,5})(\\-?([AB]))?$"), 1, 2, 3, 5, -1 },    // e.g. "Paper Doll - Windsor-4936B"
        { QRegularExpression("^(.*?)\\s*\\-\\s*([A-Za-z]{1,7}|4-[Bb][Aa][Rr]-[Bb]) (\\d{1,5})(\\-?([AB]))?$"), 1, 2, 3, 5, -1 },    // e.g. "Paper Doll - Windsor 4936B"
        { QRegularExpression("^(.*) - ([A-Z]{1,5})[\\- ]?([0-9]{1,5}[A-Za-z]{1,3})$"), 1, 2, 3, -1, -1}, // e.g. "Play It Cool - BS 2534a.mp3"

        { QRegularExpression(), -1, -1, -1, -1, -1 }
    };
    static struct FilenameMatchers label_first_matches[] = {
        { QRegularExpression("^(.*)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },    // e.g. "ABC123X - Chicken Plucker"
        { QRegularExpression(), -1, -1, -1, -1, -1 }
    };
    static struct FilenameMatchers filename_first_matches[] = {
        { QRegularExpression("^(.*)\\s*-\\s*(.*)$"), 1, 2, -1, -1, -1 },    // e.g. "Chicken Plucker - ABC123X"
        { QRegularExpression(), -1, -1, -1, -1, -1 }
    };

    switch (songFilenameFormat) {
        case SongFilenameNameDashLabel :
            return filename_first_matches;
        case SongFilenameBestGuess :
            return best_guess_matches;
        case SongFilenameLabelDashName :
//        default:  // all the cases are covered already (default is not needed here)
            return label_first_matches;
    }
    // Shut up the warnings, all the returns happen in the switch above
    return label_first_matches;
}


bool MainWindow::breakFilenameIntoParts(const QString &s,
                                        QString &label, QString &labelnum,
                                        QString &labelnum_extra,
                                        QString &title, QString &shortTitle )
{
    bool foundParts = true;
    int match_num = 0;
    struct FilenameMatchers *matches = getFilenameMatchersForType(songFilenameFormat);

    for (match_num = 0;
         matches[match_num].label_match >= 0
             && matches[match_num].title_match >= 0;
         ++match_num) {
        QRegularExpressionMatch match = matches[match_num].regex.match(s);
        if (match.hasMatch()) {
            if (matches[match_num].label_match >= 0) {
                label = match.captured(matches[match_num].label_match);
            }
            if (matches[match_num].title_match >= 0) {
                title = match.captured(matches[match_num].title_match);
                shortTitle = title;
            }
            if (matches[match_num].number_match >= 0) {
                labelnum = match.captured(matches[match_num].number_match);
            }
            if (matches[match_num].additional_label_match >= 0) {
                labelnum_extra = match.captured(matches[match_num].additional_label_match);
            }
            if (matches[match_num].additional_title_match >= 0
                && !match.captured(matches[match_num].additional_title_match).isEmpty()) {
                title += " " + match.captured(matches[match_num].additional_title_match);
            }
            break;
        } else {
//                qDebug() << s << "didn't match" << matches[match_num].regex;
        }
    }
    if (!(matches[match_num].label_match >= 0
          && matches[match_num].title_match >= 0)) {
        label = "";
        title = s;
        foundParts = false;
    }

    label = label.simplified();

    if (labelnum.length() == 0)
    {
        static QRegularExpression regexLabelPlusNum = QRegularExpression("^(\\w+)[\\- ](\\d+)(\\w?)$");
        QRegularExpressionMatch match = regexLabelPlusNum.match(label);
        if (match.hasMatch())
        {
            label = match.captured(1);
            labelnum = match.captured(2);
            if (labelnum_extra.length() == 0)
            {
                labelnum_extra = match.captured(3);
            }
            else
            {
                labelnum = labelnum + match.captured(3);
            }
        }
    }
    labelnum = labelnum.simplified();
    title = title.simplified();
    shortTitle = shortTitle.simplified();

    return foundParts;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
class CuesheetWithRanking {
public:
    QString filename;
    QString name;
    int score;
};
#pragma clang diagnostic pop

static bool CompareCuesheetWithRanking(CuesheetWithRanking *a, CuesheetWithRanking *b)
{
    if (a->score == b->score) {
        return a->name < b->name;  // if scores are equal, sort names lexicographically (note: should be QCollator numericMode() for natural sort order)
    }
    // else:
    return a->score > b->score;
}

// -----------------------------------------------------------------
QStringList splitIntoWords(const QString &str)
{
//    static QRegularExpression regexNotAlnum(QRegularExpression("\\W+"));
    static QRegularExpression regexNotAlnum(QRegularExpression("[^a-zA-Z0-9_']+"));

    QStringList words = str.split(regexNotAlnum);

    static QRegularExpression LetterNumber("[A-Z][0-9]|[0-9][A-Z]"); // do we need to split?  Most of the time, no.
    QRegularExpressionMatch quickmatch(LetterNumber.match(str));

    if (quickmatch.hasMatch()) {
        static QRegularExpression regexLettersAndNumbers("^([A-Z]+)([0-9].*)$");
        static QRegularExpression regexNumbersAndLetters("^([0-9]+)([A-Z].*)$");
//        qDebug() << "quickmatch!";
        // we gotta split it one word at a time
//        words = str.split(regexNotAlnum);
        for (int i = 0; i < words.length(); ++i)
        {
            bool splitFurther = true;

            while (splitFurther)
            {
                splitFurther = false;
                QRegularExpressionMatch match(regexLettersAndNumbers.match(words[i]));
                if (match.hasMatch())
                {
                    words.append(match.captured(1));
                    words[i] = match.captured(2);
                    splitFurther = true;
                }
                match = regexNumbersAndLetters.match(words[i]);
                if (match.hasMatch())
                {
                    splitFurther = true;
                    words.append(match.captured(1));
                    words[i] = match.captured(2);
                }
            }
        }
    }
    // else no splitting needed (e.g. it's already split, as is the case for most cuesheets)
    //   so we skip the per-word splitting, and go right to sorting
    words.sort(Qt::CaseInsensitive);
    return words;
}

int compareSortedWordListsForRelevance(const QStringList &l1, const QStringList l2)
{
    int i1 = 0, i2 = 0;
    int score = 0;

    while (i1 < l1.length() &&  i2 < l2.length())
    {
        int comp = l1[i1].compare(l2[i2], Qt::CaseInsensitive);

        if (comp == 0)
        {
            ++score;
            ++i1;
            ++i2;
        }
        else if (comp < 0)
        {
            ++i1;
        }
        else
        {
            ++i2;
        }
    }

    if (l1.length() >= 2 && l2.length() >= 2 &&
        (
            (score > ((l1.length() + l2.length()) / 4))
            || (score >= l1.length())                       // all of l1 words matched something in l2
            || (score >= l2.length())                       // all of l2 words matched something in l1
            || score >= 4)
        )
    {
//        QString s1 = l1.join("-");
//        QString s2 = l2.join("-");
        return score * 500 + 100 * (abs(l1.length()) - l2.length());
    }
    else
        return 0;
}


// TODO: the match needs to be a little fuzzier, since RR103B - Rocky Top.mp3 needs to match RR103 - Rocky Top.html
void MainWindow::findPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets)
{
    PerfTimer t("findPossibleCuesheets", __LINE__);

    QString fileType = filepath2SongType(MP3Filename);
    bool fileTypeIsPatter = (fileType == "patter");

    t.elapsed(__LINE__);

    QFileInfo mp3FileInfo(MP3Filename);
    QString mp3CanonicalPath = mp3FileInfo.canonicalPath();
    QString mp3CompleteBaseName = mp3FileInfo.completeBaseName();
    QString mp3Label = "";
    QString mp3Labelnum = "";
    QString mp3Labelnum_short = "";
    QString mp3Labelnum_extra = "";
    QString mp3Title = "";
    QString mp3ShortTitle = "";

    t.elapsed(__LINE__);

    breakFilenameIntoParts(mp3CompleteBaseName, mp3Label, mp3Labelnum, mp3Labelnum_extra, mp3Title, mp3ShortTitle);
    QList<CuesheetWithRanking *> possibleRankings;

    QStringList mp3Words = splitIntoWords(mp3CompleteBaseName);
    mp3Labelnum_short = mp3Labelnum;
    while (mp3Labelnum_short.length() > 0 && mp3Labelnum_short[0] == '0')
    {
        mp3Labelnum_short.remove(0,1);
    }

    t.elapsed(__LINE__);

    QListIterator<QString> iter(*pathStack);
    while (iter.hasNext()) {

        QString s = iter.next();

        int extensionIndex = 0;

        if (s.endsWith("htm", Qt::CaseInsensitive)) {
            // nothing
        } else if (s.endsWith("html", Qt::CaseInsensitive)) {
            extensionIndex = 1;
        } else if (s.endsWith("txt", Qt::CaseInsensitive)) {
            extensionIndex = 2;
        } else {
            continue;
        }

        QStringList sl1 = s.split("#!#");
        QString type = sl1[0];  // the type (of original pathname, before following aliases)
        QString filename = sl1[1];  // everything else

//        qDebug() << "possibleCuesheets(): " << fileTypeIsPatter << filename << filepath2SongType(filename) << type;
        if (fileTypeIsPatter && (type=="lyrics")) {
            // if it's a patter MP3, then do NOT match it against anything in the lyrics folder
            continue;
        }

//        if (type=="choreography" || type == "sd" || type=="reference") {
//            // if it's a dance program .txt file, or an sd sequence file, or a reference .txt file, don't bother trying to match
//            continue;
//        }

        QFileInfo fi(filename);

        QString label = "";
        QString labelnum = "";
        QString title = "";
        QString labelnum_extra;
        QString shortTitle = "";


        QString completeBaseName = fi.completeBaseName(); // e.g. "/Users/mpogue/__squareDanceMusic/patter/RIV 307 - Going to Ceili (Patter).mp3" --> "RIV 307 - Going to Ceili (Patter)"
        breakFilenameIntoParts(completeBaseName, label, labelnum, labelnum_extra, title, shortTitle);
        QStringList words = splitIntoWords(completeBaseName);

        QString labelnum_short = labelnum;
        while (labelnum_short.length() > 0 && labelnum_short[0] == '0')
        {
            labelnum_short.remove(0,1);
        }

//        qDebug() << "Comparing: " << completeBaseName << " to " << mp3CompleteBaseName;
//        qDebug() << "           " << type;
//        qDebug() << "           " << title << " to " << mp3Title;
//        qDebug() << "           " << shortTitle << " to " << mp3ShortTitle;
//        qDebug() << "    label: " << label << " to " << mp3Label <<
//            " and num " << labelnum << " to " << mp3Labelnum <<
//            " short num " << labelnum_short << " to " << mp3Labelnum_short;
//        qDebug() << "    title: " << mp3Title << " to " << QString(label + "-" + labelnum);
        int score = 0;
        // Minimum criteria:
        if (completeBaseName.compare(mp3CompleteBaseName, Qt::CaseInsensitive) == 0
            || title.compare(mp3Title, Qt::CaseInsensitive) == 0
            || (shortTitle.length() > 0
                && shortTitle.compare(mp3ShortTitle, Qt::CaseInsensitive) == 0)
            || (labelnum_short.length() > 0 && label.length() > 0
                &&  labelnum_short.compare(mp3Labelnum_short, Qt::CaseInsensitive) == 0
                && label.compare(mp3Label, Qt::CaseInsensitive) == 0
                )
            || (labelnum.length() > 0 && label.length() > 0
                && mp3Title.length() > 0
                && mp3Title.compare(label + "-" + labelnum, Qt::CaseInsensitive) == 0)
            )
        {

            score = extensionIndex
                + (mp3CanonicalPath.compare(fi.canonicalPath(), Qt::CaseInsensitive) == 0 ? 10000 : 0)
                + (mp3CompleteBaseName.compare(fi.completeBaseName(), Qt::CaseInsensitive) == 0 ? 1000 : 0)
                + (title.compare(mp3Title, Qt::CaseInsensitive) == 0 ? 100 : 0)
                + (shortTitle.compare(mp3ShortTitle, Qt::CaseInsensitive) == 0 ? 50 : 0)
                + (labelnum.compare(mp3Labelnum, Qt::CaseInsensitive) == 0 ? 10 : 0)
                + (labelnum_short.compare(mp3Labelnum_short, Qt::CaseInsensitive) == 0 ? 7 : 0)
                + (mp3Label.compare(mp3Label, Qt::CaseInsensitive) == 0 ? 5 : 0);
// qDebug() << "Candidate: " << filename << completeBaseName << score;
            CuesheetWithRanking *cswr = new CuesheetWithRanking();
            cswr->filename = filename;
            cswr->name = completeBaseName;
            cswr->score = score;
            possibleRankings.append(cswr);
        } /* end of if we minimally included this cuesheet */
        else if ((score = compareSortedWordListsForRelevance(mp3Words, words)) > 0)
        {
            CuesheetWithRanking *cswr = new CuesheetWithRanking();
            cswr->filename = filename;
            cswr->name = completeBaseName;
            cswr->score = score;
            possibleRankings.append(cswr);
        }
    } /* end of looping through all files we know about */

    t.elapsed(__LINE__);

//    qSort(possibleRankings.begin(), possibleRankings.end(), CompareCuesheetWithRanking);
    std::sort(possibleRankings.begin(), possibleRankings.end(), CompareCuesheetWithRanking);
    QListIterator<CuesheetWithRanking *> iterRanked(possibleRankings);
    while (iterRanked.hasNext())
    {
        CuesheetWithRanking *cswr = iterRanked.next();
        possibleCuesheets.append(cswr->filename);
        delete cswr;
    }

    // append the MP3 filename itself, IF it contains lyrics, but do this at the end,
    //  after all the other (scored/ranked) cuesheets are added to the list of possibleCuesheets,
    //  so that the default for a new song will be a real cuesheet, rather than the lyrics inside the MP3 file.

    //    qDebug() << "findPossibleCuesheets():";
        QString mp3Lyrics = loadLyrics(MP3Filename);
    //    qDebug() << "mp3Lyrics:" << mp3Lyrics;
        if (mp3Lyrics.length())
        {
            possibleCuesheets.append(MP3Filename);
        }
}

void MainWindow::loadCuesheets(const QString &MP3FileName, const QString prefCuesheet)
{
    hasLyrics = false;
    bool isPatter;

    QString preferredCuesheet = prefCuesheet;
    QString filenameToCheck = MP3FileName;
    lyricsForDifferentSong = false;
    // We may search twice:  for a patter with no lyrics we see if next is singer with lyrics
    for(int attempt=0; attempt<2; attempt++) {
//    QString HTML;

        QStringList possibleCuesheets;
        findPossibleCuesheets(filenameToCheck, possibleCuesheets);

//    qDebug() << "possibleCuesheets:" << possibleCuesheets;

        ui->comboBoxCuesheetSelector->clear();
        
        if (attempt) {
            SongSetting settings;
            if (songSettings.loadSettings(filenameToCheck, settings)) {
                QString cuesheetName = settings.getCuesheetName();
                if (cuesheetName.length() > 0) {
                    preferredCuesheet = cuesheetName;
                    lyricsForDifferentSong = true;
                }
            }
        }
        
        int defaultCuesheetIndex = 0;
        loadedCuesheetNameWithPath = ""; // nothing loaded yet

//    QString firstCuesheet(preferredCuesheet);

        foreach (const QString &cuesheet, possibleCuesheets)
            {
                RecursionGuard guard(cuesheetEditorReactingToCursorMovement);
                if ((!preferredCuesheet.isNull()) && preferredCuesheet.length() >= 0
                    && cuesheet == preferredCuesheet)
                    {
                        defaultCuesheetIndex = ui->comboBoxCuesheetSelector->count();
                    }

                QString displayName = cuesheet;
                if (displayName.startsWith(musicRootPath))
                    displayName.remove(0, musicRootPath.length());

                ui->comboBoxCuesheetSelector->addItem(displayName,
                                                      cuesheet);
            }

        if (ui->comboBoxCuesheetSelector->count() > 0)
            {
                ui->comboBoxCuesheetSelector->setCurrentIndex(defaultCuesheetIndex);
                // if it was zero, we didn't load it because the index didn't change,
                // and we skipped loading it above. Sooo...
                if (0 == defaultCuesheetIndex)
                    on_comboBoxCuesheetSelector_currentIndexChanged(0);
                hasLyrics = true;
            }

        // be careful here.  The Lyrics tab can now be the Patter tab.
        isPatter = songTypeNamesForPatter.contains(currentSongType);

        if (attempt == 0) {
            // Checking the real file;  continue if have lyrics or not patter
            if (hasLyrics || !isPatter) {
                break;
            }
            // Patter, no lyrics -- if next song is a Singer then get its lyrics.
            int row = nextVisibleSongRow();
            if (row < 0) {
                break;          // no next song
            }
            filenameToCheck = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();
            QString nextSongType = ui->songTable->item(row,kTypeCol)->text();
            
            if (songTypeNamesForSinging.contains(nextSongType)) {
                // Try this song
//              qDebug() << "loadCuesheets: now trying " << filenameToCheck;
            } else {
                break;  // it's not a singer
            }
        } else {
            // Second time around, whether or not we found lyrics the actual song played is patter
            isPatter = true;
        }               
    }
//    qDebug() << "loadCuesheets: " << currentSongType << isPatter;

    ui->tabWidget->setTabText(lyricsTabNumber, CUESHEET_TAB_NAME);
    if (isPatter) {
        // ----- PATTER -----
        ui->menuLyrics->setTitle("Patter");
//        ui->actionFilePrint->setText("Print Patter...");
//        ui->actionSave_Lyrics->setText("Save Patter");
//        ui->actionSave_Lyrics_As->setText("Save Patter As...");
        ui->actionAuto_scroll_during_playback->setText("Auto-scroll Patter");

        if (!hasLyrics || lyricsTabNumber == -1) {

            // ------------------------------------------------------------------
            // get pre-made patter.template.html file, if it exists
            QString patterTemplate = getResourceFile("patter.template.html");
//            qDebug() << "patterTemplate: " << patterTemplate;
            if (patterTemplate.isEmpty()) {
                ui->textBrowserCueSheet->setHtml("No patter found for this song.");
                loadedCuesheetNameWithPath = "";
            } else {
                ui->textBrowserCueSheet->setHtml(patterTemplate);
                loadedCuesheetNameWithPath = "patter.template.html";  // as a special case, this is allowed to not be the full path
            }

        } // else (sequence could not be found)
    } else {
        // ----- SINGING CALL -----
        ui->menuLyrics->setTitle("Lyrics");
        ui->actionAuto_scroll_during_playback->setText("Auto-scroll Cuesheet");

        if (!hasLyrics || lyricsTabNumber == -1) {

            // ------------------------------------------------------------------
            // get pre-made lyrics.template.html file, if it exists
            QString lyricsTemplate = getResourceFile("lyrics.template.html");
//            qDebug() << "lyricsTemplate: " << lyricsTemplate;
            if (lyricsTemplate.isEmpty()) {
                ui->textBrowserCueSheet->setHtml("No lyrics found for this song.");
                loadedCuesheetNameWithPath = "";
            } else {
                ui->textBrowserCueSheet->setHtml(lyricsTemplate);
                loadedCuesheetNameWithPath = "lyrics.template.html";  // as a special case, this is allowed to not be the full path
            }

        } // else (lyrics could not be found)
    } // isPatter

}


// =========================================
// -------------------------------------
// LYRICS CUESHEET FETCHING

void MainWindow::fetchListOfCuesheetsFromCloud() {
//    qDebug() << "MainWindow::fetchListOfCuesheetsFromCloud() -- Download is STARTING...";

    // TODO: only fetch if the time is newer than the one we got last time....
    // TODO:    check Expires date.

//    QList<QString> cuesheets;

    Downloader *d = new Downloader(this);

    // Apache Directory Listing, because it ends in "/" (~1.1MB uncompressed, ~250KB compressed)
    QUrl cuesheetListURL(QString(CURRENTSQVIEWCUESHEETSDIR));
    QString cuesheetListFilename = musicRootPath + "/.squaredesk/publishedCuesheets.html";

//    qDebug() << "cuesheet URL to download:" << cuesheetListURL.toDisplayString();
//    qDebug() << "             put it here:" << cuesheetListFilename;

    d->doDownload(cuesheetListURL, cuesheetListFilename);  // download URL and put it into cuesheetListFilename

    QObject::connect(d,SIGNAL(downloadFinished()), this, SLOT(cuesheetListDownloadEnd()));
    QObject::connect(d,SIGNAL(downloadFinished()), d, SLOT(deleteLater()));

    // PROGRESS BAR ---------------------
    progressDialog = new QProgressDialog("Downloading list of available cuesheets...\n ", "Cancel", 0, 100, this);
    progressDialog->setMinimumDuration(0);  // start it up right away
    progressCancelled = false;
    progressDialog->setWindowModality(Qt::WindowModal);  // stays until cancelled
    progressDialog->setMinimumWidth(450);  // avoid bug with Cancel button resizing itself

    connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelProgress()));
    connect(progressDialog, SIGNAL(canceled()), d, SLOT(abortTransfer()));      // if user hits CANCEL, abort transfer in progress.
    progressTimer = new QTimer(this);

    connect(progressTimer, SIGNAL(timeout()), this, SLOT(makeProgress()));
    progressOffset = 0.0;
    progressTotal = 0.0;
    progressTimer->start(1000);  // once per second to 33%
}

bool MainWindow::fuzzyMatchFilenameToCuesheetname(QString s1, QString s2) {
//    qDebug() << "trying to match: " << s1 << "," << s2;

// **** EXACT MATCH OF COMPLETE BASENAME (just for testing)
//    QFileInfo fi1(s1);
//    QFileInfo fi2(s2);

//    bool match = fi1.completeBaseName() == fi2.completeBaseName();
//    if (match) {
//        qDebug() << "fuzzy match: " << s1 << "," << s2;
//    }

//    return(match);

// **** OUR FUZZY MATCHING (same as findPossibleCuesheets)

    // SPLIT APART THE MUSIC FILENAME --------
    QFileInfo mp3FileInfo(s1);
//    QString mp3CanonicalPath = mp3FileInfo.canonicalPath();
    QString mp3CompleteBaseName = mp3FileInfo.completeBaseName();
    QString mp3Label = "";
    QString mp3Labelnum = "";
    QString mp3Labelnum_short = "";
    QString mp3Labelnum_extra = "";
    QString mp3Title = "";
    QString mp3ShortTitle = "";
    breakFilenameIntoParts(mp3CompleteBaseName, mp3Label, mp3Labelnum, mp3Labelnum_extra, mp3Title, mp3ShortTitle);
//    QList<CuesheetWithRanking *> possibleRankings;

    QStringList mp3Words = splitIntoWords(mp3CompleteBaseName);
    mp3Labelnum_short = mp3Labelnum;
    while (mp3Labelnum_short.length() > 0 && mp3Labelnum_short[0] == '0')
    {
        mp3Labelnum_short.remove(0,1);
    }

    // SPLIT APART THE CUESHEET FILENAME --------
    QFileInfo fi(s2);
    QString label = "";
    QString labelnum = "";
    QString title = "";
    QString labelnum_extra;
    QString shortTitle = "";

    QString completeBaseName = fi.completeBaseName(); // e.g. "/Users/mpogue/__squareDanceMusic/patter/RIV 307 - Going to Ceili (Patter).mp3" --> "RIV 307 - Going to Ceili (Patter)"
    breakFilenameIntoParts(completeBaseName, label, labelnum, labelnum_extra, title, shortTitle);
    QStringList words = splitIntoWords(completeBaseName);
    QString labelnum_short = labelnum;
    while (labelnum_short.length() > 0 && labelnum_short[0] == '0')
    {
        labelnum_short.remove(0,1);
    }

    // NOW SCORE IT ----------------------------
    int score = 0;
//    qDebug() << "label: " << label;
    if (completeBaseName.compare(mp3CompleteBaseName, Qt::CaseInsensitive) == 0         // exact match: entire filename
        || title.compare(mp3Title, Qt::CaseInsensitive) == 0                            // exact match: title (without label/labelNum)
        || (shortTitle.length() > 0                                                     // exact match: shortTitle
            && shortTitle.compare(mp3ShortTitle, Qt::CaseInsensitive) == 0)
        || (labelnum_short.length() > 0 && label.length() > 0                           // exact match: shortLabel + shortLabelNumber
            &&  labelnum_short.compare(mp3Labelnum_short, Qt::CaseInsensitive) == 0
            && label.compare(mp3Label, Qt::CaseInsensitive) == 0
            )
        || (labelnum.length() > 0 && label.length() > 0
            && mp3Title.length() > 0
            && mp3Title.compare(label + "-" + labelnum, Qt::CaseInsensitive) == 0)
        )
    {
        // Minimum criteria (we will accept as a match, without looking at sorted words):
//        qDebug() << "fuzzy match (meets minimum criteria): " << s1 << "," << s2;
        return(true);
    } else if ((score = compareSortedWordListsForRelevance(mp3Words, words)) > 0)
    {
        Q_UNUSED(score)
        // fuzzy match, using the sorted words in the titles
//        qDebug() << "fuzzy match (meets sorted words criteria): " << s1 << "," << s2;
        return(true);
    }

    return(false);
}

void MainWindow::cuesheetListDownloadEnd() {

    if (progressDialog->wasCanceled()) {
        return;
    }

//    qDebug() << "MainWindow::cuesheetListDownloadEnd() -- Download is DONE";

    qApp->processEvents();  // allow the progress bar to move
//    qDebug() << "MainWindow::cuesheetListDownloadEnd() -- Making list of music files...";
    QList<QString> musicFiles = getListOfMusicFiles();

    qApp->processEvents();  // allow the progress bar to move
//    qDebug() << "MainWindow::cuesheetListDownloadEnd() -- Making list of cuesheet files...";
    QList<QString> cuesheetsInCloud = getListOfCuesheets();

    progressOffset = 33;
    progressTotal = 0;
    progressDialog->setValue(33);
//    progressDialog->setLabelText("Matching your music with cuesheets...");
    progressTimer->stop();

//    qDebug() << "MainWindow::cuesheetListDownloadEnd() -- Matching them up...";

//    qDebug() << "***** Here's the list of musicFiles:" << musicFiles;
//    qDebug() << "***** Here's the list of cuesheets:" << cuesheetsInCloud;

    double numCuesheets = cuesheetsInCloud.length();
    double numChecked = 0;

    // match up the music filenames against the cuesheets that the Cloud has
    QList<QString> maybeFilesToDownload;

    QList<QString>::iterator i;
    QList<QString>::iterator j;
    for (j = cuesheetsInCloud.begin(); j != cuesheetsInCloud.end(); ++j) {
        // should we download this Cloud cuesheet file?

        if ( static_cast<unsigned int>(numChecked) % 50 == 0 ) {
            progressDialog->setLabelText("Found matching cuesheets: " +
                                         QString::number(static_cast<unsigned int>(maybeFilesToDownload.length())) +
                                         " out of " + QString::number(static_cast<unsigned int>(numChecked)) +
                                         "\n" + (maybeFilesToDownload.length() > 0 ? maybeFilesToDownload.last() : "")
                                         );
            progressDialog->setValue(static_cast<int>(33 + 33.0*(numChecked/numCuesheets)));
            qApp->processEvents();  // allow the progress bar to move every 100 checks
            if (progressDialog->wasCanceled()) {
                return;
            }
        }

        numChecked++;

        for (i = musicFiles.begin(); i != musicFiles.end(); ++i) {
            if (fuzzyMatchFilenameToCuesheetname(*i, *j)) {
                // yes, let's download it, if we don't have it already.
                maybeFilesToDownload.append(*j);
//                qDebug() << "Will maybe download: " << *j;
                break;  // once we've decided to download this file, go on and look at the NEXT cuesheet
            }
        }
    }

    progressDialog->setValue(66);

//    qDebug() << "MainWindow::cuesheetListDownloadEnd() -- Maybe downloading " << maybeFilesToDownload.length() << " files";
//  qDebug() << "***** Maybe downloading " << maybeFilesToDownload.length() << " files.";

    double numDownloads = maybeFilesToDownload.length();
    double numDownloaded = 0;

    // download them (if we don't have them already)
    QList<QString>::iterator k;
    for (k = maybeFilesToDownload.begin(); k != maybeFilesToDownload.end(); ++k) {
        progressDialog->setLabelText("Downloading matching cuesheets (if needed): " +
                                     QString::number(static_cast<unsigned int>(numDownloaded++)) +
                                     " out of " +
                                     QString::number(static_cast<unsigned int>(numDownloads)) +
                                     "\n" +
                                     *k
                                     );
        progressDialog->setValue(static_cast<int>((66 + 33.0*(numDownloaded/numDownloads))));
        qApp->processEvents();  // allow the progress bar to move constantly
        if (progressDialog->wasCanceled()) {
            break;
        }

        downloadCuesheetFileIfNeeded(*k);
    }

// qDebug() << "MainWindow::cuesheetListDownloadEnd() -- DONE.  All cuesheets we didn't have are downloaded.";

    progressDialog->setLabelText("Done.");
    progressDialog->setValue(100);  // kill the progress bar
    progressTimer->stop();
    progressOffset = 0;
    progressTotal = 0;

    // FINALLY, RESCAN THE ENTIRE MUSIC DIRECTORY FOR SONGS AND LYRICS ------------
    maybeLyricsChanged();

//    findMusic(musicRootPath,"","main", true);  // get the filenames from the user's directories
//    loadMusicList(); // and filter them into the songTable

//    reloadCurrentMP3File(); // in case the list of matching cuesheets changed by the recent addition of cuesheets
}

void MainWindow::downloadCuesheetFileIfNeeded(QString cuesheetFilename) {

//    qDebug() << "Maybe fetching: " << cuesheetFilename;
//    cout << ".";

    QString musicDirPath = prefsManager.GetmusicPath();
    //    QString tempDirPath = "/Users/mpogue/clean4";
    QString destinationFolder = musicDirPath + "/lyrics/downloaded/";

    QDir dir(musicDirPath);
    dir.mkpath("lyrics/downloaded");    // make sure that the destination path exists (including intermediates)

    QFile file(destinationFolder + cuesheetFilename);
    QFileInfo fileinfo(file);

    // if we don't already have it...
    if (!fileinfo.exists()) {
        // ***** SYNCHRONOUS FETCH *****
        // "http://squaredesk.net/cuesheets/SqViewCueSheets_2017.03.14/"
        QNetworkAccessManager *networkMgr = new QNetworkAccessManager(this);
        QString URLtoFetch = CURRENTSQVIEWCUESHEETSDIR + cuesheetFilename; // individual files (~10KB)
        QNetworkReply *reply = networkMgr->get( QNetworkRequest( QUrl(URLtoFetch) ) );

        QEventLoop loop;
        QObject::connect(reply, SIGNAL(readyRead()), &loop, SLOT(quit()));

//        qDebug() << "Fetching file we don't have: " << URLtoFetch;
        // Execute the event loop here, now we will wait here until readyRead() signal is emitted
        // which in turn will trigger event loop quit.
        loop.exec();

        QString resultString(reply->readAll());  // only fetch this once!
        // qDebug() << "result:" << resultString;

        // OK, we have the file now...
        if (resultString.length() == 0) {
            qDebug() << "ERROR: file we got was zero length.";
            return;
        }

//        qDebug() << "***** WRITING TO: " << destinationFolder + cuesheetFilename;
        // let's try to write it
        if ( file.open(QIODevice::WriteOnly) )
        {
            QTextStream stream( &file );
            stream << resultString;
            stream.flush();
            file.close();
        } else {
            qDebug() << "ERROR: couldn't open the file for writing...";
        }
    } else {
//        qDebug() << "     Not fetching it, because we already have it.";
    }
}

// ====================
QList<QString> MainWindow::getListOfCuesheets() {

    QList<QString> list;

    QString cuesheetListFilename = musicRootPath + "/.squaredesk/publishedCuesheets.html";
    QFile inputFile(cuesheetListFilename)
            ;
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
//        int line_number = 0;
        while (!in.atEnd()) //  && line_number < 10)
        {
//            line_number++;
            QString line = in.readLine();

            // OLD FORMAT:
            // <li><a href="RR%20147%20-%20Amarillo%20By%20Morning.html"> RR 147 - Amarillo By Morning.html</a></li>

            // NEW FORMAT (at SquareDesk.net):
            // <a href="RR%20147%20-%20Amarillo%20By%20Morning.html">RR 147 - Amarillo By..&gt;</a>

//            static QRegularExpression regex_cuesheetName("^<li><a href=\"(.*?)\">(.*)</a></li>$"); // don't be greedy!
            static QRegularExpression regex_cuesheetName("^.*<a href=\"(.*?)\">(.*)</a>.*$"); // don't be greedy!
            QRegularExpressionMatch match = regex_cuesheetName.match(line);
//            qDebug() << "line: " << line;
            if (match.hasMatch())
            {
//                QString cuesheetFilename(match.captured(2).trimmed());
                // we do NOT want the elided filename ending in ".."
                // AND we want to replace HTML-encoded spaces with real spaces
                QString cuesheetFilename(match.captured(1).replace("%20"," ").trimmed());
//                qDebug() << "****** Cloud has cuesheet: " << cuesheetFilename << " *****";

                list.append(cuesheetFilename);
//                downloadCuesheetFileIfNeeded(cuesheetFilename, &musicFiles);
            }
        }
        inputFile.close();
        } else {
            qDebug() << "ERROR: could not open " << cuesheetListFilename;
        }

    return(list);
}

void MainWindow::maybeLyricsChanged() {
//    qDebug() << "maybeLyricsChanged()";
    // AND, just in case the list of matching cuesheets for the current song has been
    //   changed by the recent addition of cuesheets...
//    if (!filewatcherShouldIgnoreOneFileSave) {
//        // don't rescan, if this is a SAVE or SAVE AS Lyrics (those get added manually to the pathStack)
//        // RESCAN THE ENTIRE MUSIC DIRECTORY FOR LYRICS FILES (and music files that might match) ------------
//        findMusic(musicRootPath,"","main", true);  // get the filenames from the user's directories
//        loadMusicList(); // and filter them into the songTable

//        // reload only if this isn't a SAVE LYRICS FILE
////        reloadCurrentMP3File();
//    }
    filewatcherShouldIgnoreOneFileSave = false;
}

// ==========================================
void MainWindow::on_actionDownload_Cuesheets_triggered()
{
    fetchListOfCuesheetsFromCloud();
}

void MainWindow::makeProgress() {
#if defined(Q_OS_MAC) | defined(Q_OS_WIN)
    if (progressTotal < 70) {
        progressTotal += 7.0;
    } else if (progressTotal < 90) {
        progressTotal += 2.0;
    } else if (progressTotal < 98) {
        progressTotal += 0.5;
    } // else no progress for you.

//    qDebug() << "making progress..." << progressOffset << "," << progressTotal;

    progressDialog->setValue(static_cast<int>(progressOffset + 33.0*(progressTotal/100.0)));
#endif
}

void MainWindow::cancelProgress() {
#if defined(Q_OS_MAC) | defined(Q_OS_WIN)
//    qDebug() << "cancelling progress...";
    progressTimer->stop();
    progressTotal = 0;
    progressOffset = 0;
    progressCancelled = true;
#endif
}


void MainWindow::lyricsDownloadEnd() {
}

// =======================
// invoked from context menu on Lyrics tab
void MainWindow::revealLyricsFileInFinder() {
    showInFinderOrExplorer(loadedCuesheetNameWithPath);
}

// --------------
// invoked from context menu on songTable entry
void MainWindow::revealAttachedLyricsFileInFinder() {

    int selectedRow = selectedSongRow();  // get current row or -1
    if (selectedRow == -1) {
        qDebug() << "Tried to revealAttachedLyricsFile, but no selected row."; // if nothing selected, give up
        return;
    }

    QString currentMP3filenameWithPath = ui->songTable->item(selectedRow, kPathCol)->data(Qt::UserRole).toString();

    SongSetting settings1;
    if (songSettings.loadSettings(currentMP3filenameWithPath, settings1)) {
        QString cuesheetPath = settings1.getCuesheetName();
        showInFinderOrExplorer(cuesheetPath);
    } else {
        qDebug() << "Tried to revealAttachedLyricsFile, but could not get settings for: " << currentMP3filenameWithPath;
    }
}
