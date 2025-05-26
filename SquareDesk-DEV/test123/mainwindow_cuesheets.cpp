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
// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#include "downloadmanager.h"
#include "songlistmodel.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#include "cuesheetmatchingdebugdialog.h"
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

#include <taglib/toolkit/tlist.h>
#include <taglib/fileref.h>
#include <taglib/toolkit/tfile.h>
#include <taglib/tag.h>
#include <taglib/toolkit/tpropertymap.h>

#include <taglib/mpeg/mpegfile.h>
#include <taglib/mpeg/id3v2/id3v2tag.h>
#include <taglib/mpeg/id3v2/id3v2frame.h>
#include <taglib/mpeg/id3v2/id3v2header.h>

#include <taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.h>
#include <taglib/mpeg/id3v2/frames/commentsframe.h>
#include <taglib/mpeg/id3v2/frames/textidentificationframe.h>
using namespace TagLib;

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
        { QRegularExpression("^(.*) - ([A-Za-z]{1,9})[\\- ]?([0-9]{1,5}[A-Za-z]{0,3})$"), 1, 2, 3, -1, -1}, // e.g. "Play It Cool - BS 2534a.mp3"
                                                                                                            // e.g. "Strings Galore - Chaparral 117b.mp3"
        { QRegularExpression("^([Oo][Gg][Rr][Mm][Pp]3\\s*\\d{1,5})\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },    // e.g. "OGRMP3 04 - Addam's Family.mp3"
        { QRegularExpression("^(4-[Bb][Aa][Rr]-[Bb]\\s*\\d{1,5})\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },    // e.g. "4-bar-b 123 - Chicken Plucker"
        { QRegularExpression("^(.*) - ([A-Za-z]+[\\- ]\\d+)( *-?[VMA-C]|\\-\\d+)?$"), 1, 2, -1, 3, -1 },
        { QRegularExpression("^([A-Za-z]+[\\- ]\\d+)(-?[VvMA-C]?) - (.*)$"), 3, 1, -1, 2, -1 },
        { QRegularExpression("^([A-Za-z]+ ?\\d+)([MVmv]?)[ -]+(.*)$/"), 3, 1, -1, 2, -1 },
        { QRegularExpression("^([A-Za-z]?[0-9][A-Z]+[\\- ]?\\d+)([MVmv]?)[ -]+(.*)$"), 3, 1, -1, 2, -1 },
        { QRegularExpression("^(.*) - ([A-Za-z]{1,5}+)[\\- ](\\d+)( .*)?$"), 1, 2, 3, -1, 4 },
        { QRegularExpression("^([A-Za-z]+ ?\\d+)([ABab])?[ -]+(.*)$/"), 3, 1, -1, 2, -1 },
        { QRegularExpression("^([A-Za-z]+\\-\\d+)\\-(.*)/"), 2, 1, -1, -1, -1 },
//    { QRegularExpression("^(\\d+) - (.*)$"), 2, -1, -1, -1, -1 },         // first -1 prematurely ended the search (typo?)
//    { QRegularExpression("^(\\d+\\.)(.*)$"), 2, -1, -1, -1, -1 },         // first -1 prematurely ended the search (typo?)
        { QRegularExpression("^(\\d+)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },  // e.g. "123 - Chicken Plucker"
        { QRegularExpression("^(\\d+\\.)(.*)$"), 2, 1, -1, -1, -1 },            // e.g. "123.Chicken Plucker"
//        { QRegularExpression("^(.*?) - (.*)$"), 2, 1, -1, -1, -1 },           // ? is a non-greedy match (So that "A - B - C", first group only matches "A")
        { QRegularExpression("^([A-Za-z]{1,5}+[\\- ]*\\d+[A-Za-z]*)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 }, // e.g. "ABC 123-Chicken Plucker"
        { QRegularExpression("^([A-Za-z]{1,5}+[\\- ]*\\d+[A-Za-z0-9]*)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 }, // e.g. "ABC 123h1-Chicken Plucker"
        { QRegularExpression("^([A-Za-z0-9]{1,5}+)\\s*(\\d+)([A-Za-z]{1,2})?\\s*-\\s*(.*?)\\s*(\\(.*\\))?$"), 4, 1, 2, 3, 5 }, // SIR 705b - Papa Was A Rollin Stone (Instrumental).mp3
        { QRegularExpression("^([A-Za-z0-9]{1,5}+)\\s*-\\s*(.*)$"), 2, 1, -1, -1, -1 },    // e.g. "POP - Chicken Plucker" (if it has a dash but fails all other tests,
        { QRegularExpression("^(.*?)\\s*\\-\\s*([A-Za-z]{1,5})(\\d{1,5})\\s*(\\(.*\\))?$"), 1, 2, 3, -1, 4 },    // e.g. "A Summer Song - CHIC3002 (female vocals)
        { QRegularExpression("^(.*?)\\s*\\-\\s*([A-Za-z]{1,7}|4-[Bb][Aa][Rr]-[Bb])-(\\d{1,5})(\\-?([ABab]))?$"), 1, 2, 3, 5, -1 },    // e.g. "Paper Doll - Windsor-4936B"
        { QRegularExpression("^(.*?)\\s*\\-\\s*([A-Za-z]{1,7}|4-[Bb][Aa][Rr]-[Bb]) (\\d{1,5})(\\-?([ABab]))?$"), 1, 2, 3, 5, -1 },    // e.g. "Paper Doll - Windsor 4936B"
        { QRegularExpression("^(.*)\\s*\\-\\s*([A-Za-z ]+)\\s*([0-9]{1,5}[A-Za-z]{0,3})$"), 1, 2, 3, -1, -1}, // e.g. "Streets of London - New Beat 203a.mp3"

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
    // DDD(s)
    bool foundParts = true;
    int match_num = 0;
    struct FilenameMatchers *matches = getFilenameMatchersForType(songFilenameFormat);

    // special pre-processing for labels with dashes in their names
    QString s2 = s;

    static QRegularExpression regex1("(4-[Bb][Aa][Rr]-[Bb])");              // 4-Bar-B
    static QRegularExpression regex2("([Cc][Ii][Rr][Cc][Ll][Ee]-[Dd])");    // Circle-D
    static QRegularExpression regex3("([Jj][Aa][Yy]-[Bb][Aa][Rr]-[Kk][Aa][Yy])");  // Jay-Bar-Kay

    QRegularExpressionMatch m1;
    if (s2.contains(regex1, &m1)) {
        // qDebug() << "MATCH 1: " << m1 << m1.captured(1);
        s2.replace(m1.captured(1), "FOURBARB");
    }

    QRegularExpressionMatch m2;
    if (s2.contains(regex2, &m2)) {
        // qDebug() << "MATCH 2: " << m2 << m2.captured(1);
        s2.replace(m2.captured(1), "CIRCLED");
    }

    QRegularExpressionMatch m3;
    if (s2.contains(regex3, &m3)) {
        // qDebug() << "MATCH 3: " << m3 << m3.captured(1);
        s2.replace(m3.captured(1), "JAYBARKAY");
    }

    for (match_num = 0;
         matches[match_num].label_match >= 0
             && matches[match_num].title_match >= 0;
         ++match_num) {
        QRegularExpressionMatch match = matches[match_num].regex.match(s2);
        if (match.hasMatch()) {
            if (matches[match_num].label_match >= 0) {
                label = match.captured(matches[match_num].label_match);
                label.replace("FOURBARB", m1.captured(1)); // keep original capitalization
                label.replace("CIRCLED",  m2.captured(1)); // keep original capitalization
                label.replace("JAYBARKAY", m3.captured(1)); // keep original capitalization
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
QStringList splitIntoWords(const QString &str1)
{
    // let's ignore anything that is in parentheses
    //   e.g. RR 150 - Dream Lover (BECK)
    //        RR 150 - Dream Lover (no melody)

    // if (str1.contains("Want To Miss A Thing", Qt::CaseInsensitive)) {
    //     qDebug() << "DEBUG";
    // }

    QString str = str1;
    QRegularExpression regex("\\([^()]*\\)");

    // Keep replacing until no more matches are found
    QRegularExpressionMatch match;
    while ((match = regex.match(str)).hasMatch()) {
        str.remove(match.capturedStart(), match.capturedLength());
    }

    str = str.simplified(); // AND, trim space at start/end, consolidate other whitespace

    // -----------------------
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

    // qDebug() << "compareSortedWordLists:" << score << l1.length() << l2.length();

    if (l1.length() >= 2 && l2.length() >= 2 &&
        (
            (score > ((l1.length() + l2.length()) / 4))
            || ((score >= ((l1.length() + l2.length() - 4) / 2)) &&
                (l1.length() >= 4) &&
                (l2.length() >= 4)) // 1/2 or more of the words matched, NOT including the label and label#, e.g. "RIV 123 - Dream Lover"
                                   // AND these are not 1-word titles
            || (score >= l1.length())                       // all of l1 words matched something in l2
            || (score >= l2.length())                       // all of l2 words matched something in l1
            || score >= 4)
        )
    {
//        QString s1 = l1.join("-");
//        QString s2 = l2.join("-");
        return score * 100 + 50 * (abs(l1.length() - l2.length()));
    }
    else
        return 0;
}

int MainWindow::MP3FilenameVsCuesheetnameScore(QString fn, QString cn, QTextEdit *debugOut) {
    if (debugOut != nullptr) {
        debugOut->append("=== MP3 FILENAME vs CUESHEET NAME SCORING DEBUG ===");
        debugOut->append(QString("Song filename: %1").arg(fn));
        debugOut->append(QString("Cuesheet filename: %1").arg(cn));
        debugOut->append("");
        debugOut->append("--- Starting MP3 vs Cuesheet scoring analysis ---");
    }
    
    // Helper function to calculate Levenshtein distance
    auto levenshteinDistance = [](const QString &s1, const QString &s2) -> int {
        const int len1 = s1.size();
        const int len2 = s2.size();
        
        QVector<QVector<int>> d(len1 + 1, QVector<int>(len2 + 1));
        
        for (int i = 0; i <= len1; i++)
            d[i][0] = i;
            
        for (int j = 0; j <= len2; j++)
            d[0][j] = j;
            
        for (int i = 1; i <= len1; i++) {
            for (int j = 1; j <= len2; j++) {
                int cost = (s1[i-1].toLower() == s2[j-1].toLower()) ? 0 : 1;
                
                d[i][j] = qMin(qMin(
                    d[i-1][j] + 1,     // deletion
                    d[i][j-1] + 1),    // insertion
                    d[i-1][j-1] + cost // substitution
                );
                
                // Check for transposition (adjacent character swap)
                if (i > 1 && j > 1 && 
                    s1[i-1].toLower() == s2[j-2].toLower() && 
                    s1[i-2].toLower() == s2[j-1].toLower()) {
                    d[i][j] = qMin(d[i][j], d[i-2][j-2] + cost);
                }
            }
        }
        return d[len1][len2];
    };
    
    // Helper function to check if words are "fuzzy equal"
    auto fuzzyWordEqual = [levenshteinDistance](const QString &w1, const QString &w2) -> bool {
        // First check exact match
        if (w1.compare(w2, Qt::CaseInsensitive) == 0)
            return true;
            
        // Different length check (one character added/removed)
        int lenDiff = w1.length() - w2.length();
        if (qAbs(lenDiff) > 1)
            return false;
            
        // Check for transposed letters
        if (w1.length() == w2.length() && w1.length() >= 2) {
            for (int i = 0; i < w1.length() - 1; i++) {
                QString transposed = w1;
                // Swap adjacent characters
                QChar temp = transposed[i];
                transposed[i] = transposed[i+1];
                transposed[i+1] = temp;
                
                if (transposed.compare(w2, Qt::CaseInsensitive) == 0)
                    return true;
            }
        }
        
        // Calculate Levenshtein distance
        int distance = levenshteinDistance(w1, w2);
        
        // Allow 1 edit (character changed, added, or removed)
        return distance <= 1;
    };
    
    // Function to filter words that are 1-2 characters long
    auto filterShortWords = [](const QStringList &words) -> QStringList {
        QStringList filtered;
        for (const QString &word : words) {
            if (word.length() > 2) {
                filtered.append(word);
            }
        }
        return filtered;
    };
    
    // Step 1: Preprocess both filenames
    // Remove text in parentheses
    auto removeParentheses = [](QString str) {
        QRegularExpression regex("\\([^()]*\\)");
        QRegularExpressionMatch match;
        while ((match = regex.match(str)).hasMatch()) {
            str.remove(match.capturedStart(), match.capturedLength());
        }
        return str;
    };
    
    QString mp3Name = removeParentheses(fn);
    QString cuesheetName = removeParentheses(cn);
    
    if (debugOut != nullptr) {
        debugOut->append("Step 1: Preprocessing - Remove parentheses");
        debugOut->append(QString("  MP3:      '%1' -> '%2'").arg(fn, mp3Name));
        debugOut->append(QString("  Cuesheet: '%1' -> '%2'").arg(cn, cuesheetName));
    }
    
    // Trim and simplify (collapse multiple spaces)
    mp3Name = mp3Name.simplified();
    cuesheetName = cuesheetName.simplified();

    if (debugOut != nullptr) {
        debugOut->append("");
        debugOut->append("After simplifying whitespace:");
        debugOut->append(QString("  MP3:      '%1'").arg(mp3Name));
        debugOut->append(QString("  Cuesheet: '%1'").arg(cuesheetName));
    }

    // I like to use cuesheet filenames like "Blue.2.html"
    //   this removes the .2 part, for matching purposes
    QString beforeDotRemoval = cuesheetName;
    QRegularExpression dotNumAtEnd("\\.[0-9]?$");
    cuesheetName.replace(dotNumAtEnd, ""); // THIS IS NOT WORKING HERE
    
    if (debugOut != nullptr && beforeDotRemoval != cuesheetName) {
        debugOut->append(QString("After removing dot-number: '%1' -> '%2'").arg(beforeDotRemoval, cuesheetName));
    }
    
    if (debugOut != nullptr) {
        debugOut->append("");
        debugOut->append("Step 2: Check for exact match after preprocessing");
    }

    // Step 2: Check for exact match after preprocessing
    if (mp3Name.compare(cuesheetName, Qt::CaseInsensitive) == 0) {
        if (debugOut != nullptr) {
            debugOut->append("✓ EXACT MATCH after preprocessing -> SCORE: 100");
        }
        return 100;
    } else {
        if (debugOut != nullptr) {
            debugOut->append("✗ Not an exact match after preprocessing");
            debugOut->append(QString("  MP3:      '%1'").arg(mp3Name));
            debugOut->append(QString("  Cuesheet: '%1'").arg(cuesheetName));
        }
    }
    
    if (debugOut != nullptr) {
        debugOut->append("");
        debugOut->append("Step 3: Split filenames into words and filter");
    }
    
    // Step 3: Split filenames into words for comparison and filter short words
    QStringList mp3AllWords = mp3Name.split(QRegularExpression("\\s+"));
    QStringList cuesheetAllWords = cuesheetName.split(QRegularExpression("\\s+"));
    
    QStringList mp3Words = filterShortWords(mp3AllWords);
    QStringList cuesheetWords = filterShortWords(cuesheetAllWords);
    
    if (debugOut != nullptr) {
        debugOut->append(QString("  MP3 all words: [%1]").arg(mp3AllWords.join(", ")));
        debugOut->append(QString("  MP3 filtered words (>2 chars): [%1]").arg(mp3Words.join(", ")));
        debugOut->append(QString("  Cuesheet all words: [%1]").arg(cuesheetAllWords.join(", ")));
        debugOut->append(QString("  Cuesheet filtered words (>2 chars): [%1]").arg(cuesheetWords.join(", ")));
    }
    
    // If filtering removed all words, use the original lists
    if (mp3Words.isEmpty() && !mp3AllWords.isEmpty()) {
        mp3Words = mp3AllWords;
        if (debugOut != nullptr) {
            debugOut->append("  Using original MP3 words (filtering removed all)");
        }
    }
    if (cuesheetWords.isEmpty() && !cuesheetAllWords.isEmpty()) {
        cuesheetWords = cuesheetAllWords;
        if (debugOut != nullptr) {
            debugOut->append("  Using original cuesheet words (filtering removed all)");
        }
    }
    
    mp3Words.sort(Qt::CaseInsensitive);
    cuesheetWords.sort(Qt::CaseInsensitive);

    if (debugOut != nullptr) {
        debugOut->append("");
        debugOut->append("Step 4: Check if cuesheet words contained in mp3 words or vise-versa (with FUZZY word matching)");
        debugOut->append(QString("  mp3Words:      [%1]").arg(mp3Words.join(";")));
        debugOut->append(QString("  cuesheetWords: [%1]").arg(cuesheetWords.join(";")));
    }
    
    // Step 4: Check if one contains all words of the other in same order (with fuzzy matching)
    bool mp3ContainsAllCuesheet = true;
    bool cuesheetContainsAllMp3 = true;
    int mp3Index = 0;
    int cuesheetIndex = 0;
    
    // Check if cuesheet contains all mp3 words in order
    while (mp3Index < mp3Words.size() && cuesheetIndex < cuesheetWords.size()) {
        if (fuzzyWordEqual(mp3Words[mp3Index], cuesheetWords[cuesheetIndex])) {
            mp3Index++;
            cuesheetIndex++;
        } else {
            cuesheetIndex++;
            cuesheetContainsAllMp3 = false;
        }
    }
    if (mp3Index < mp3Words.size()) {
        cuesheetContainsAllMp3 = false; // Didn't find all mp3 words
    }
    
    // Reset and check if mp3 contains all cuesheet words in order
    mp3Index = 0;
    cuesheetIndex = 0;
    while (cuesheetIndex < cuesheetWords.size() && mp3Index < mp3Words.size()) {
        if (fuzzyWordEqual(cuesheetWords[cuesheetIndex], mp3Words[mp3Index])) {
            cuesheetIndex++;
            mp3Index++;
        } else {
            mp3Index++;
            mp3ContainsAllCuesheet = false;
        }
    }
    if (cuesheetIndex < cuesheetWords.size()) {
        mp3ContainsAllCuesheet = false; // Didn't find all cuesheet words
    }
    
    if (cuesheetContainsAllMp3 && mp3Words.size() >= 2) {
        if (debugOut != nullptr) {
            debugOut->append(QString("✓ Cuesheet contains all %1 MP3 words in order -> SCORE: 95").arg(mp3Words.size()));
        }
        return 95;
    }
    if (mp3ContainsAllCuesheet && cuesheetWords.size() >= 2) {
        if (debugOut != nullptr) {
            debugOut->append(QString("✓ MP3 contains all %1 cuesheet words in order -> SCORE: 90").arg(cuesheetWords.size()));
        }
        return 90;
    }
    
    if (debugOut != nullptr) {
        debugOut->append("✗ No complete word containment pattern found");
        debugOut->append("");
        debugOut->append("Step 5: Parse filenames to extract components");
    }
    
    // Step 5: Parse the filenames to extract components
    // Try to identify label, label number, label extra, and title
    auto parseFilename = [debugOut](const QString &name) {
        struct ParsedName {
            QString label;
            QString labelNum;
            QString labelExtra;
            QString title;
            bool reversed = false;
        };
        
        ParsedName result;

        // Try standard format: LABEL NUM[EXTRA] - TITLE
        QRegularExpression stdFormat("^([A-Za-z ]{1,12})\\s*([0-9]{1,6})([A-Za-z]{0,4})?\\s*-\\s*(.+)$",
                                    QRegularExpression::CaseInsensitiveOption);
        
        // Try reversed format: TITLE - LABEL NUM[EXTRA]
        QRegularExpression revFormat("^(.+)\\s*-\\s*([A-Za-z ]{1,12})\\s*([0-9]{1,5})([A-Za-z]{0,4})?$",
                                    QRegularExpression::CaseInsensitiveOption);
        
        QRegularExpressionMatch match = stdFormat.match(name);
        if (match.hasMatch()) {
            result.label = match.captured(1);
            result.labelNum = match.captured(2);
            result.labelExtra = match.captured(3);
            result.title = match.captured(4);
            result.reversed = false;
            // qDebug() << "NORMAL ORDER: " << name;
            if (debugOut != nullptr) {
                debugOut->append(QString("  '%1' appears to be NORMAL order.").arg(name));
            }
            return result;
        }
        
        match = revFormat.match(name);
        if (match.hasMatch()) {
            result.title = match.captured(1);
            result.label = match.captured(2);
            result.labelNum = match.captured(3);
            result.labelExtra = match.captured(4);
            result.reversed = true;
            // qDebug() << "REVERSED ORDER: " << name;
            if (debugOut != nullptr) {
                debugOut->append(QString("  '%1' appears to be REVERSED order.").arg(name));
            }
            return result;
        }
        
        // If no match, just consider the whole thing as title
        result.title = name;
        // qDebug() << "NEITHER ORDER: " << name;
        if (debugOut != nullptr) {
            debugOut->append(QString("  '%1': could not determine normal vs reversed order.").arg(name));
        }
        return result;
    };
    
    auto mp3Parsed = parseFilename(mp3Name);
    auto cuesheetParsed = parseFilename(cuesheetName);
    
    if (debugOut != nullptr) {
        debugOut->append("");
        debugOut->append(QString("  MP3 parsed - Label: '%1', Number: '%2', Title: '%3'")
                         .arg(mp3Parsed.label, mp3Parsed.labelNum, mp3Parsed.title));
        debugOut->append(QString("  Cuesheet parsed - Label: '%1', Number: '%2', Title: '%3'")
                         .arg(cuesheetParsed.label, cuesheetParsed.labelNum, cuesheetParsed.title));
        debugOut->append("");
        debugOut->append("Step 6: Calculate score based on matching components");
    }
    
    // Step 6: Calculate score based on matching components
    int score = 0;

    bool labelMatch = false;
    bool labelNumberMatch = false;

    // Check if labels match (use fuzzy matching)
    if (!mp3Parsed.label.isEmpty() && !cuesheetParsed.label.isEmpty() &&
        fuzzyWordEqual(mp3Parsed.label, cuesheetParsed.label)) {
        // score += 20;
        labelMatch = true;
        if (debugOut != nullptr) {
            debugOut->append("  ✓ Labels match (fuzzy)");
        }
    } else if (debugOut != nullptr) {
        debugOut->append("  ✗ Labels don't match");
    }
    
    // Check if label numbers match (ignore leading zeros)
    if (!mp3Parsed.labelNum.isEmpty() && !cuesheetParsed.labelNum.isEmpty()) {
        int mp3Num = mp3Parsed.labelNum.toInt();
        int cuesheetNum = cuesheetParsed.labelNum.toInt();
        if (mp3Num == cuesheetNum) {
            // score += 20;
            labelNumberMatch = true;
            if (debugOut != nullptr) {
                debugOut->append(QString("  ✓ Label numbers match (%1 == %2)").arg(mp3Num).arg(cuesheetNum));
            }
        } else if (debugOut != nullptr) {
            debugOut->append(QString("  ✗ Label numbers don't match (%1 != %2)").arg(mp3Num).arg(cuesheetNum));
        }
    } else if (debugOut != nullptr) {
        debugOut->append("  - No label numbers to compare");
    }

    if (labelMatch && labelNumberMatch) {
        score += 36;
        if (debugOut != nullptr) {
            debugOut->append("  → Adding 36 points for label+number match");
        }
    }
    
    // qDebug() << ">>> " << score;

    // Check if label extras match (use fuzzy matching)
    if (!mp3Parsed.labelExtra.isEmpty() && !cuesheetParsed.labelExtra.isEmpty() && 
        fuzzyWordEqual(mp3Parsed.labelExtra, cuesheetParsed.labelExtra)) {
        if (debugOut != nullptr) {
            debugOut->append("  → Adding 2 points for label extras match");
        }
        score += 2;
    }
    
    // Step 7: Calculate longest common sequence of words in title
    QStringList mp3TitleAllWords = mp3Parsed.title.split(QRegularExpression("\\s+"));
    QStringList cuesheetTitleAllWords = cuesheetParsed.title.split(QRegularExpression("\\s+"));
    
    // Filter short words
    QStringList mp3TitleWords = filterShortWords(mp3TitleAllWords);
    QStringList cuesheetTitleWords = filterShortWords(cuesheetTitleAllWords);
    
    // if (cuesheetParsed.title.contains("Blue")) {
    //     // qDebug() << "after filter:"  << cuesheetParsed.title << cuesheetTitleWords.join(".") << mp3TitleWords.join(".");
    // }

    // If filtering removed all words, use the original lists
    if (mp3TitleWords.isEmpty() && !mp3TitleAllWords.isEmpty()) {
        mp3TitleWords = mp3TitleAllWords;
    }
    if (cuesheetTitleWords.isEmpty() && !cuesheetTitleAllWords.isEmpty()) {
        cuesheetTitleWords = cuesheetTitleAllWords;
    }
    
    // Calculate longest common subsequence (LCS) of words with fuzzy matching
    QVector<QVector<int>> lcs(mp3TitleWords.size() + 1, QVector<int>(cuesheetTitleWords.size() + 1, 0));
    int maxLength = 0;
    
    for (int i = 1; i <= mp3TitleWords.size(); i++) {
        for (int j = 1; j <= cuesheetTitleWords.size(); j++) {
            if (fuzzyWordEqual(mp3TitleWords[i-1], cuesheetTitleWords[j-1])) {
                lcs[i][j] = lcs[i-1][j-1] + 1;
                maxLength = qMax(maxLength, lcs[i][j]);
            } else {
                lcs[i][j] = 0;
            }
        }
    }

    // Score based on the proportion of matching words in the title
    int titleWordMatchPercent = 0;
    int maxWordCount = qMax(mp3TitleWords.size(), cuesheetTitleWords.size());
    if (maxWordCount > 0) {
        titleWordMatchPercent = (maxLength * 100) / maxWordCount;
    }

    // if (cuesheetParsed.title.contains("Blue")) {
    //     qDebug() << "   maxLength:" << maxLength << cuesheetParsed.title << titleWordMatchPercent;
    // }

    // Scale the title match score to be at most 54 (so total can't exceed 89)
    int titleMatchScore = qMin(54, titleWordMatchPercent * 54 / 100);
    score += titleMatchScore;
    
    if (debugOut != nullptr) {
        debugOut->append("");
        debugOut->append("Step 7: Title word matching results");
        debugOut->append(QString("  mp3TitleWords:      [%1]").arg(mp3TitleWords.join(";")));
        debugOut->append(QString("  cuesheetTitleWords: [%1]").arg(cuesheetTitleWords.join(";")));
        debugOut->append(QString("  Title word match percentage: %1%").arg(titleWordMatchPercent));
        debugOut->append(QString("  Title match score: %1").arg(titleMatchScore));
        debugOut->append(QString("  Running score: %1").arg(score));
    }
    
    // Ensure score doesn't exceed 89 (reserving 90+ for special cases already handled)
    score = qMin(89, score);
    
    // If score is too low, consider it no match
    if (score <= 35) {
        if (debugOut != nullptr) {
            debugOut->append("");
            debugOut->append(QString("Final score %1 is too low (≤35) -> returning 0").arg(score));
        }
        return 0;
    }
    
    if (debugOut != nullptr) {
        debugOut->append("");
        debugOut->append(QString("=== FINAL SCORE: %1 ===").arg(score));
    }
    
    return score;
}

void MainWindow::betterFindPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets) {

    QFileInfo mp3FileInfo(MP3Filename);
    // QString mp3CanonicalPath = mp3FileInfo.canonicalPath();
    QString mp3CompleteBaseName = mp3FileInfo.completeBaseName();

    QList<CuesheetWithRanking *> possibleRankings;    // results go here

    QListIterator<QString> iter(*pathStackCuesheets); // search through Lyrics/Cuesheets
    while (iter.hasNext()) {
        QString s = iter.next();
        QStringList sl1 = s.split("#!#");
        // QString type = sl1[0];  // the type (of original pathname, before following aliases)
        QString cuesheetFilename = sl1[1];  // everything else
        QFileInfo cuesheetFileInfo(cuesheetFilename);
        QString cuesheetCompleteBaseName = cuesheetFileInfo.completeBaseName();

        int score = this->MP3FilenameVsCuesheetnameScore(mp3CompleteBaseName, cuesheetCompleteBaseName);
        if (score > 0) {
            CuesheetWithRanking *cswr = new CuesheetWithRanking();
            cswr->filename = cuesheetFilename;
            cswr->name = cuesheetCompleteBaseName;
            cswr->score = score;
            possibleRankings.append(cswr);
        }
    }

    // qDebug() << "betterFindPossibleCuesheets:" << possibleRankings;

    // for (const auto &s : possibleRankings) {
    //     qDebug() << s->score << s->name;
    // }

    std::sort(possibleRankings.begin(), possibleRankings.end(), CompareCuesheetWithRanking);
    QListIterator<CuesheetWithRanking *> iterRanked(possibleRankings);
    while (iterRanked.hasNext())
    {
        CuesheetWithRanking *cswr = iterRanked.next();
        QFileInfo winner(cswr->filename);
         // qDebug() << "betterFindPossibleCuesheets" << cswr->score << winner.completeBaseName();
        possibleCuesheets.append(cswr->filename);
        delete cswr;
    }

    // qDebug() << "betterFindPossibleCuesheets:" << possibleCuesheets;
}


// TODO: the match needs to be a little fuzzier, since RR103B - Rocky Top.mp3 needs to match RR103 - Rocky Top.html
void MainWindow::findPossibleCuesheets(const QString &MP3Filename, QStringList &possibleCuesheets)
{
    PerfTimer t("findPossibleCuesheets", __LINE__);

    QString fileCategory = filepath2SongCategoryName(MP3Filename); // get the CATEGORY name
    bool fileCategoryIsPatter = (fileCategory == "patter");

    // qDebug() << "findPossibleCuesheets: fileTypeIsPatter = " << fileTypeIsPatter << ", fileTypeIsSinging = " << fileTypeIsSinging;

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
    patterTemplateCuesheets.clear();
    lyricsTemplateCuesheets.clear();

    QListIterator<QString> iter(*pathStackCuesheets); // search through Lyrics/Cuesheets
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

        if (filename.contains("patter.template")) {
            // qDebug() << "found patter template:" << filename;
            patterTemplateCuesheets.append(filename);
        } else if (filename.contains("lyrics.template")) {
            // qDebug() << "found singing call template:" << filename;
            lyricsTemplateCuesheets.append(filename);
        } // else do nothing

//        qDebug() << "possibleCuesheets(): " << fileTypeIsPatter << filename << filepath2SongType(filename) << type;
        if (fileCategoryIsPatter && (type=="lyrics")) {
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

        // // DEBUG:
        // QString fn = filename;
        // fn.replace(musicRootPath, "");

        // qDebug() << "*** working on: " << completeBaseName;

        // if (completeBaseName.contains("Want To Miss A Thing", Qt::CaseInsensitive) ||
        //     completeBaseName.contains("Want To Cry", Qt::CaseInsensitive)) {
        //     qDebug() << "DREAM";
        // }

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
                + (mp3CompleteBaseName.compare(fi.completeBaseName(), Qt::CaseInsensitive) == 0 ? 3000 : 0)
                + (title.compare(mp3Title, Qt::CaseInsensitive) == 0 ? 3000 : 0)
                + (shortTitle.compare(mp3ShortTitle, Qt::CaseInsensitive) == 0 ? 50 : 0)
                + (labelnum.compare(mp3Labelnum, Qt::CaseInsensitive) == 0 ? 10 : 0)
                + (labelnum_short.compare(mp3Labelnum_short, Qt::CaseInsensitive) == 0 ? 7 : 0)
                + (mp3Label.compare(mp3Label, Qt::CaseInsensitive) == 0 ? 5 : 0);
            // qDebug() << "MIN CRITERIA: " << score << completeBaseName << fn;
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

            // qDebug() << "RANKED:" << score << completeBaseName << fn;
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
        // qDebug() << "findPossibleCuesheets" << cswr->score << cswr->filename;
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

    // // Now append the template files, in case we want to use one of those...
    // if (fileTypeIsPatter && !patterTemplateCuesheets.isEmpty()) {
    //     possibleCuesheets.append(patterTemplateCuesheets);
    // } else if (fileTypeIsSinging && !lyricsTemplateCuesheets.isEmpty()) {
    //     possibleCuesheets.append(lyricsTemplateCuesheets);
    // }
}


bool MainWindow::loadCuesheets(const QString &MP3FileName, const QString prefCuesheet, QString nextFilename)
{
    // qDebug() << "loadCuesheets" << MP3FileName << prefCuesheet << nextFilename;

    // if we're loading cuesheets, check to make sure we were not in the process
    //  of editing one already, and if so, ask what to do.
    // qDebug() << "loadCuesheets: nextFilename = " << nextFilename;
    if (!maybeSaveCuesheet(3))
    {
//            qDebug() << "USER CANCELLED LOAD, returning FALSE";
        return false; // user clicked CANCEL, so do NOT do the load
    }

    hasLyrics = false;
    bool isPatter;

    QString preferredCuesheet = prefCuesheet;
    QString filenameToCheck = MP3FileName;
    lyricsForDifferentSong = false;
    // We may search twice:  for a patter with no lyrics we see if next is singer with lyrics
    for(int attempt=0; attempt<2; attempt++) {
//    QString HTML;

        QStringList possibleCuesheets;
        // findPossibleCuesheets(filenameToCheck, possibleCuesheets);    // THE OLD ALGORITHM
        betterFindPossibleCuesheets(filenameToCheck, possibleCuesheets); // THE NEW ALGORITHM

        // qDebug() << "attempt:" << attempt;
        // qDebug() << "possibleCuesheets:" << possibleCuesheets;
        // qDebug() << "preferredCuesheet:" << preferredCuesheet;

        ui->comboBoxCuesheetSelector->clear();
        
        if (attempt) {
            SongSetting settings;
            if (songSettings.loadSettings(filenameToCheck, settings)) {
                QString cuesheetName = settings.getCuesheetName();
                // qDebug() << "cuesheetName:" << cuesheetName;
                if (cuesheetName.length() > 0) {
                    preferredCuesheet = cuesheetName;
                    lyricsForDifferentSong = true;
                }
            }
        }
        
        int defaultCuesheetIndex = 0;
        loadedCuesheetNameWithPath = ""; // nothing loaded yet

//    QString firstCuesheet(preferredCuesheet);

        foreach (const QString &cuesheet, possibleCuesheets) {
                RecursionGuard guard(cuesheetEditorReactingToCursorMovement);
            // qDebug() << "checking: " << cuesheet << preferredCuesheet;
                if ((!preferredCuesheet.isNull()) && preferredCuesheet.length() >= 0
                    && cuesheet == preferredCuesheet)
                    {
                        defaultCuesheetIndex = ui->comboBoxCuesheetSelector->count();
                        // qDebug() << "defaultCuesheetIndex: " << defaultCuesheetIndex;
                    }

                QString displayName = cuesheet;
                if (displayName.startsWith(musicRootPath)) {
                    displayName.remove(0, musicRootPath.length());
                }
                // qDebug() << "displayName:" << displayName << maybeCuesheetLevel(displayName);
                QString maybeLevelString = maybeCuesheetLevel(displayName);
                if (maybeLevelString != "") {
                    displayName += " [L: " + maybeLevelString + "]";
                }
                ui->comboBoxCuesheetSelector->addItem(displayName,
                                                      cuesheet);
        }

        bool hasCuesheets = ui->comboBoxCuesheetSelector->count() > 0;
        if (hasCuesheets)
        {
            ui->comboBoxCuesheetSelector->setCurrentIndex(defaultCuesheetIndex);
            // if it was zero, we didn't load it because the index didn't change,
            // and we skipped loading it above. Sooo...
            if (0 == defaultCuesheetIndex)
                on_comboBoxCuesheetSelector_currentIndexChanged(0);
            hasLyrics = true;
        }

        // only allow editing (and show the Unlock button), or creating a New cuesheet from template,
        //  when there actually is a cuesheet in the dropdown menu (which means that some cuesheet will be loaded).
        ui->pushButtonEditLyrics->setVisible(hasCuesheets);

        // NOTE: This code does not work yet.
        // if (0 == defaultCuesheetIndex) {
        //     // nothing matched out preferredCuesheet name
        //     if (ui->comboBoxCuesheetSelector->count() > 0)
        //     {
        //         // but if there are cuesheets in there, select the first one
        //         ui->comboBoxCuesheetSelector->setCurrentIndex(0);
        //         // in this case, we have DO NOT actually "have lyrics", so keep looking in the next singer
        //     }
        // } else {
        //     // something DID match, so we definitely have lyrics that the user wanted
        //     ui->comboBoxCuesheetSelector->setCurrentIndex(defaultCuesheetIndex);
        //     hasLyrics = true;
        // }


        // THIS IS THE "PEEK" CODE ----------
        //
        // be careful here.  The Lyrics tab can now be the Patter tab.
        // isPatter = songTypeNamesForPatter.contains(currentSongType);
        isPatter = currentSongIsPatter;

        // qDebug() << "loadCuesheets2: isPatter = " << isPatter;

        if (attempt == 0) {
            // Checking the real file;  continue if have lyrics or not patter
            if (hasLyrics || !isPatter) {
                break;
            }
            if (nextFilename == "") {
                break;          // no next song
            }
            filenameToCheck = nextFilename;

            QString theCategory = filepath2SongCategoryName(filenameToCheck); // get the CATEGORY, e.g. "singing" for all singing call types (e.g. "singer", "singing", "Singing Call", etc.)
            // if (filenameToCheck.contains("/singing/")) {
            if (theCategory == "singing") {
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
//        ui->menuLyrics->setTitle("Patter");
//        ui->actionFilePrint->setText("Print Patter...");
//        ui->actionSave_Lyrics->setText("Save Patter");
//        ui->actionSave_Lyrics_As->setText("Save Patter As...");
        ui->actionAuto_scroll_during_playback->setText("Auto-scroll Cuesheet");

        if (!hasLyrics || lyricsTabNumber == -1) {

            // if there is a cuesheet loaded, leave it alone
            if (!cueSheetLoaded) {
                // ------------------------------------------------------------------
                // get pre-made patter.template.html file, if it exists
                QString patterTemplate = getResourceFile("patter.template.html");
//              qDebug() << "patterTemplate: " << patterTemplate;
                if (patterTemplate.isEmpty()) {
                    ui->textBrowserCueSheet->setHtml("No cuesheet found for this song.");
                    loadedCuesheetNameWithPath = "";
                } else {
                    // ui->textBrowserCueSheet->setHtml(patterTemplate);
                    loadedCuesheetNameWithPath = musicRootPath + "/lyrics/templates/patter.template.html";  // this is now allowed to be the full path
                    loadCuesheet(loadedCuesheetNameWithPath);
                    hasLyrics = true;   // so the "Save as" action is enabled
                }
            }

        } // else (sequence could not be found)
    } else /* if (currentSongIsSinger || currentSongIsVocal) */ {
        // ----- SINGING CALL OR RATHER "NOT PATTER" -----
//        ui->menuLyrics->setTitle("Cuesheet");
        ui->actionAuto_scroll_during_playback->setText("Auto-scroll Cuesheet");

        if (!hasLyrics || lyricsTabNumber == -1) {

            // ------------------------------------------------------------------
            // get pre-made lyrics.template.html file, if it exists
            QString lyricsTemplate = getResourceFile("lyrics.template.html");
//            qDebug() << "lyricsTemplate: " << lyricsTemplate;
            if (true || lyricsTemplate.isEmpty()) {
                ui->textBrowserCueSheet->setHtml("No cuesheet found for this song.");
                loadedCuesheetNameWithPath = "";
            } else {
                // ui->textBrowserCueSheet->setHtml(lyricsTemplate);
                loadedCuesheetNameWithPath = musicRootPath + "/lyrics/templates/lyrics.template.html";  // this is now allowed to be the full path
                loadCuesheet(loadedCuesheetNameWithPath);
            }
        } // else (lyrics could not be found)
    } /*else {
        ui->textBrowserCueSheet->setHtml("No cuesheet found for this song.");
        loadedCuesheetNameWithPath = "";
    }*/

    return true;  // ALL IS WELL
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

    // QDir dir(musicDirPath);
    // dir.mkpath("lyrics/downloaded");    // make sure that the destination path exists (including intermediates)

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

    // int selectedRow = selectedSongRow();  // get current row or -1
    // if (selectedRow == -1) {
    //     qDebug() << "Tried to revealAttachedLyricsFile, but no selected row."; // if nothing selected, give up
    //     return;
    // }

    // QString currentMP3filenameWithPath = ui->songTable->item(selectedRow, kPathCol)->data(Qt::UserRole).toString();

    // SongSetting settings1;
    // if (songSettings.loadSettings(currentMP3filenameWithPath, settings1)) {
    //     QString cuesheetPath = settings1.getCuesheetName();
    //     showInFinderOrExplorer(cuesheetPath);
    // } else {
    //     qDebug() << "Tried to revealAttachedLyricsFile, but could not get settings for: " << currentMP3filenameWithPath;
    // }
}

// ------------------------------------------------
void MainWindow::on_actionSave_Cuesheet_triggered()
{
    // This is the Cuesheet > Save Cuesheet
    on_pushButtonCueSheetEditSave_clicked();
}

void MainWindow::on_actionSave_Cuesheet_As_triggered()
{
    // This is Cuesheet > Save Cuesheet As
    on_pushButtonCueSheetEditSaveAs_clicked();
}

void MainWindow::on_actionPrint_Cuesheet_triggered()
{
    // Cuesheet > Print Cuesheet...
    QPrinter printer;
    QPrintDialog printDialog(&printer, this);

    // qDebug() << "on_actionPrint_Cuesheet_triggered(): currentSongTypeName = " << currentSongTypeName;

    // PRINT CUESHEET FOR PATTER OR SINGER -------------------------------------------------------
    // if (currentSongType == "singing") {
    if (currentSongIsSinger || currentSongIsVocal) {
        printDialog.setWindowTitle("Print Cuesheet");
    } else {
        printDialog.setWindowTitle("Print Patter");
    }

    if (printDialog.exec() == QDialog::Rejected) {
        return;
    }

    ui->textBrowserCueSheet->print(&printer);
}

void MainWindow::on_pushButtonRevertEdits_clicked()
{
    on_actionLyricsCueSheetRevert_Edits_triggered(true);
}

// --------------
// follows this example: https://doc.qt.io/qt-6/qtwidgets-mainwindows-application-example.html
bool MainWindow::maybeSaveCuesheet(int optionCount) {

//    bool isBeingModified = ui->pushButtonEditLyrics->isChecked();
    bool isBeingModified = cuesheetIsUnlockedForEditing;
//    qDebug() << "maybeSaveCuesheet::isBeingModified = " << isBeingModified << optionCount;

    if (!isBeingModified) {
//        qDebug() << "maybeSaveCuesheet() returning, because cuesheet is not being edited...";
        return true;
    }

    QFileInfo fi(loadedCuesheetNameWithPath);
    QString shortCuesheetName = fi.baseName();

    if (shortCuesheetName == "") {
//        qDebug() << "maybeSaveCuesheet() returning, because no cuesheet loaded...";
        return true;
    }

//    qDebug() << "maybeSaveCuesheet continuing with the save (ask the user):" << shortCuesheetName;

    // QMessageBox::StandardButton ret;

    // if (optionCount == 3) {
    //     ret = QMessageBox::warning(this, "SquareDesk",
    //                                QString("The cuesheet '") + shortCuesheetName + "' is being edited.\n\nDo you want to save your changes?",
    //                                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    // } else if (optionCount == 2) {
    //     ret = QMessageBox::warning(this, "SquareDesk",
    //                                QString("The cuesheet '") + shortCuesheetName + "' is being edited.\n\nDo you want to save your changes?",
    //                                QMessageBox::Save | QMessageBox::Discard );
    // } else {
    //     qDebug() << "maybeSaveCuesheet::optionCount error";
    //     return false;
    // }

    QMessageBox msgBox;

    if (optionCount == 3) {
        // ret = QMessageBox::warning(this, "SquareDesk",
        //                            QString("The cuesheet '") + shortCuesheetName + "' is being edited.\n\nDo you want to save your changes?",
        //                            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setText(QString("The cuesheet '") + shortCuesheetName + "' is being edited.");
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
    } else if (optionCount == 2) {
        // ret = QMessageBox::warning(this, "SquareDesk",
        //                            QString("The cuesheet '") + shortCuesheetName + "' is being edited.\n\nDo you want to save your changes?",
        //                            QMessageBox::Save | QMessageBox::Discard );
        msgBox.setText(QString("The cuesheet '") + shortCuesheetName + "' is being edited.");
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
    } else {
        qDebug() << "maybeSaveCuesheet::optionCount error";
        return false;
    }

    int ret = msgBox.exec();

    switch (ret) {
        case QMessageBox::Save:
//            qDebug() << "User clicked SAVE";
            on_actionSave_Cuesheet_triggered(); // Cuesheet > Save 'filename'
            // TODO: should we provide a Save As option here?
            return true; // all is well

        case QMessageBox::Cancel:
//            qDebug() << "User clicked CANCEL, returning FALSE";
            return false; // don't load the new song or don't quit!

        default:
//            qDebug() << "DON'T SAVE";
            break;
    }

//    qDebug() << "RETURNING TRUE, ALL IS WELL.";
    return true;
}

// Function to extract and return the dance level string from the cuesheet
QString MainWindow::maybeCuesheetLevel(QString relativeFilePath) {
    // Convert the relative path to an absolute path using musicRootPath
    QString absolutePath = musicRootPath + "/" + relativeFilePath;
    
    // Open the file
    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return ""; // Return empty string if file can't be opened
    }
    
    // Read up to 30 lines of the file
    QTextStream in(&file);
    int lineCount = 0;
    
    while (!in.atEnd() && lineCount < 30) {
        QString line = in.readLine();
        lineCount++;
        
        // Look for "L:<level string>" with proper spacing and termination
        // Spacing: either preceded by at least one space OR at start of line
        // Termination: followed by a letter and colon, comma, semicolon, <BR, or end of line
        QRegularExpression levelRegex("(^|\\s+)(L|LEVEL):(.*?)([a-zA-Z]:|,|;|<BR|$)",
                                      QRegularExpression::CaseInsensitiveOption);
        
        QRegularExpressionMatch match = levelRegex.match(line);
        if (match.hasMatch()) {
            QString levelString = match.captured(3).simplified();
            return(levelString);
        }
    }
    
    file.close();
    
    // No dance level was found
    return "";
}

// Function moved from mainwindow.cpp
QString MainWindow::loadLyrics(QString MP3FileName)
{
    QString USLTlyrics;

    if (!MP3FileName.endsWith(".mp3", Qt::CaseInsensitive)) {
        // WAV, FLAC, etc can have ID3 tags, but we don't support USLT in them right now
        return(QString(""));
    }

    MPEG::File mp3file(MP3FileName.toStdString().c_str());
    ID3v2::Tag *id3v2tag = mp3file.ID3v2Tag(true);

    ID3v2::FrameList::ConstIterator it = id3v2tag->frameList().begin();
    for (; it != id3v2tag->frameList().end(); it++)
    {
        if ((*it)->frameID() == "SYLT")
        {
//            qDebug() << "LOAD LYRICS -- found an SYLT frame!";
        }

        if ((*it)->frameID() == "USLT")
        {
//            qDebug() << "LOAD LYRICS -- found a USLT frame!";

            ID3v2::UnsynchronizedLyricsFrame* usltFrame = dynamic_cast<ID3v2::UnsynchronizedLyricsFrame*>(*it);
            USLTlyrics = usltFrame->text().toCString();
        }
    }
//    qDebug() << "Got lyrics:" << USLTlyrics;
    return (USLTlyrics);
}


// ----------------------------------------------------------------
void MainWindow::setupCuesheetMenu()
{
    // Set initial state: hide the "Explore Cuesheet Matching..." menu item
    ui->actionExplore_Cuesheet_Matching->setVisible(false);    
}


// ----------------------------------------------------------------
void MainWindow::on_actionExplore_Cuesheet_Matching_triggered()
{
    // Create and show the debug dialog
    if (!cuesheetDebugDialog) {
        cuesheetDebugDialog = new CuesheetMatchingDebugDialog(this);
    }
    
    cuesheetDebugDialog->show();
    cuesheetDebugDialog->raise();
    cuesheetDebugDialog->activateWindow();
}
