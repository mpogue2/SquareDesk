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

#include "selectionretainer.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"

// #include <iostream>
// #include <fstream>
#include <string>
#include <sstream>
#include <regex>
#include <unordered_map>
#include <algorithm>
#include <cctype>

// NEW TRANSFORM CODE -------------
// Helper function to replace numeric HTML entities
std::string replaceNumericEntities(const std::string& input) {
    std::string result = input;
    std::regex numeric_entity_regex("&#(\\d+);");

    // Find all matches
    std::smatch match;
    std::string::const_iterator searchStart(input.cbegin());
    std::vector<std::pair<size_t, std::pair<size_t, char>>> replacements;

    // First, collect all replacements
    while (std::regex_search(searchStart, input.cend(), match, numeric_entity_regex)) {
        size_t pos = std::distance(input.cbegin(), match[0].first);
        int code = std::stoi(match[1]);

        // Only convert codes in the ASCII range (0-127)
        if (code <= 127) {
            replacements.push_back({pos, {match[0].length(), static_cast<char>(code)}});
        }

        searchStart = match.suffix().first;
    }

    // Apply replacements in reverse order to avoid invalidating positions
    for (auto it = replacements.rbegin(); it != replacements.rend(); ++it) {
        result.replace(it->first, it->second.first, 1, it->second.second);
    }

    return result;
}

// Helper function to check if a string starts with another string (case insensitive)
bool startsWithCaseInsensitive(const std::string& str, const std::string& prefix) {
    if (str.size() < prefix.size()) {
        return false;
    }

    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(str[i]) != std::tolower(prefix[i])) {
            return false;
        }
    }

    return true;
}

// Helper function to normalize line endings, handle whitespace, and format specific lines
std::string normalizeAndFormatText(const std::string& input) {
    // Step 1: Normalize line endings by converting "\r\n" to "\n"
    std::string result = input;
    std::regex crlf_regex("\\r\\n");
    result = std::regex_replace(result, crlf_regex, "\n");

    // Step 2: Process the text line by line to handle whitespace and format specific lines
    std::istringstream iss(result);
    std::ostringstream oss;
    std::string line;
    std::vector<std::string> lines;

    // Keywords to capitalize when found at the beginning of a line
    std::vector<std::string> keywords = {
                                         "opener", "closer", "middle break",
                                         "figure 1", "figure 2", "figure 3", "figure 4", "figure",
                                         "tag"};

    // Read all lines
    while (std::getline(iss, line)) {
        // Check if the line consists only of whitespace
        bool whitespace_only = true;
        for (char c : line) {
            if (c != ' ' && c != '\t') {
                whitespace_only = false;
                break;
            }
        }

        // Skip whitespace-only lines
        if (whitespace_only) {
            lines.push_back("");
            continue;
        }

        // Remove leading whitespace (tabs, spaces) from the line
        size_t first_non_whitespace = line.find_first_not_of(" \t");
        if (first_non_whitespace != std::string::npos) {
            line = line.substr(first_non_whitespace);
        }

        // Consolidate consecutive spaces into a single space
        std::regex multiple_spaces_regex("\\s{2,}");
        line = std::regex_replace(line, multiple_spaces_regex, " ");

        // Check if the line starts with any of the keywords (case insensitive)
        bool keyword_found = false;
        for (const auto& keyword : keywords) {
            if (startsWithCaseInsensitive(line, keyword)) {
                // Convert the keyword part to uppercase
                std::string uppercase_keyword = keyword;
                std::transform(uppercase_keyword.begin(), uppercase_keyword.end(), uppercase_keyword.begin(), ::toupper);

                // Replace the keyword part with its uppercase version
                line.replace(0, keyword.length(), uppercase_keyword);
                keyword_found = true;
                break;
            }
        }

        // If not a keyword line, apply capitalization rules
        if (!keyword_found && !line.empty()) {
            //     // Check if first character is capital
            //     bool first_char_capital = std::isupper(line[0]);
            //     // Check if there are other capital letters (excluding standalone "I")
            //     bool has_other_capitals = false;
            //     std::istringstream word_stream(line);
            //     std::string word;
            //     std::vector<std::string> words;

            //     while (word_stream >> word) {
            //         words.push_back(word);
            //         // std::cout << "word: " << word << std::endl;

            //         // Check for capital letters at start of words other than "I", "I'm", etc.
            //         //   ignore the first word on the line
            //         if (word != "I" && word.substr(0, 2) != "I'" && words.size() > 1) {
            //             if (std::isupper(word[0])) {
            //                 has_other_capitals = true;
            //                 // break;
            //             }
            //         }
            //     }

            //     // std::cout << "line: " << line << has_other_capitals << std::endl;

            //     // Apply capitalization rules
            //     if (first_char_capital && has_other_capitals) {
            //         // Capitalize all words
            //         for (auto& word : words) {
            //             if (!word.empty()) {
            //                 word[0] = std::toupper(word[0]);
            //                 for (size_t i = 1; i < word.length(); i++) {
            //                     word[i] = std::tolower(word[i]);
            //                 }
            //             }
            //         }
            //     } else {
            //         // Capitalize only first word, lowercase the rest
            //         if (!words.empty() && !words[0].empty()) {
            //             words[0][0] = std::toupper(words[0][0]);
            //             for (size_t i = 1; i < words[0].length(); i++) {
            //                 words[0][i] = std::tolower(words[0][i]);
            //             }
            //         }

            //         for (size_t w = 1; w < words.size(); w++) {
            //             for (size_t i = 0; i < words[w].length(); i++) {
            //                 // Keep "I" as capital
            //                 if (words[w] == "I" || words[w].substr(0, 2) == "I'") {
            //                     continue;
            //                 }
            //                 words[w][i] = std::tolower(words[w][i]);
            //             }
            //         }
            //     }

            //     // Reconstruct the line
            //     std::ostringstream reconstructed;
            //     for (size_t i = 0; i < words.size(); i++) {
            //         reconstructed << words[i];
            //         if (i < words.size() - 1) {
            //             reconstructed << " ";
            //         }
            //     }
            //     line = reconstructed.str();
        }
        // std::cout << "OUTLINE: " << line << std::endl;

        lines.push_back(line);
    }

    // Step 3: Remove all whitespace at the beginning of the file
    size_t first_non_empty = 0;
    while (first_non_empty < lines.size() && lines[first_non_empty].empty()) {
        first_non_empty++;
    }

    // Step 4: Join the lines and add newlines, with special handling for keyword lines
    for (size_t i = first_non_empty; i < lines.size(); ++i) {
        oss << lines[i] << "\n";

        // Check if this line starts with a keyword
        bool is_keyword_line = false;
        for (const auto& keyword : keywords) {
            std::string uppercase_keyword = keyword;
            std::transform(uppercase_keyword.begin(), uppercase_keyword.end(), uppercase_keyword.begin(), ::toupper);
            if (lines[i].find(uppercase_keyword) == 0) {
                is_keyword_line = true;
                break;
            }
        }

        // If this is a keyword line, skip any empty lines that follow it
        if (is_keyword_line) {
            size_t next = i + 1;
            while (next < lines.size() && lines[next].empty()) {
                next++;
            }
            // Skip to the next non-empty line (minus 1 because the loop will increment i)
            if (next > i + 1) {
                i = next - 1;
            }
        }
    }

    result = oss.str();

    // Step 5: For non-keyword lines, consolidate 3 or more consecutive newlines into 2 newlines
    std::regex multi_newline_regex("\\n{3,}");
    result = std::regex_replace(result, multi_newline_regex, "\n\n");

    // Step 6: Remove whitespace at the end of the file, except for a single newline
    size_t last_non_whitespace = result.find_last_not_of(" \t\n");
    if (last_non_whitespace != std::string::npos) {
        result = result.substr(0, last_non_whitespace + 1) + "\n";
    }

    // Step 7: Capitalize the WHO of each call (Heads, Sides, Boys, Girls)
    result = std::regex_replace(result, std::regex("heads", std::regex_constants::icase), "HEADS");
    result = std::regex_replace(result, std::regex("sides", std::regex_constants::icase), "SIDES");
    result = std::regex_replace(result, std::regex("boys", std::regex_constants::icase), "BOYS");
    result = std::regex_replace(result, std::regex("girls", std::regex_constants::icase), "GIRLS");

    return result;
}

void remove_non_ascii(std::string& str) {
    str.erase(std::remove_if(str.begin(), str.end(), [](char c){ return c < 0 || c > 127; }), str.end());
}

// Transformation function that processes HTML content
std::string transform2text(const std::string& input) {
    std::string result = input;

    // Step 1: Remove HTML comments (<!-- ... -->)
    std::regex comments_regex("<!--[\\s\\S]*?-->");
    result = std::regex_replace(result, comments_regex, "");

    // Step 2: Remove style sections (<style> ... </style>)
    std::regex style_regex("<style[^>]*>[\\s\\S]*?</style>");
    result = std::regex_replace(result, style_regex, "");

    // Step 3: Replace common HTML character entities with ASCII equivalents
    // Define a map of common HTML character entities and their ASCII equivalents
    std::unordered_map<std::string, std::string> entity_map = {
        {"&#8217;", "'"},   // closing single quote/apostrophe
        {"&#8220;", "\""},  // opening double quote
        {"&#8221;", "\""},  // closing double quote
        {"&#8216;", "'"},   // opening single quote
        {"&#8211;", "-"},   // en dash
        {"&#8212;", "--"},  // em dash
        {"&#8230;", "..."}  // ellipsis
    };

    // qDebug() << "result initial **** " << result << "*****************\n";

    // Replace each entity with its ASCII equivalent
    for (const auto& pair : entity_map) {
        size_t pos = 0;
        while ((pos = result.find(pair.first, pos)) != std::string::npos) {
            result.replace(pos, pair.first.length(), pair.second);
            pos += pair.second.length();
        }
        // qDebug() << "result **** " << result << "*****************\n";
    }

    // Step 4: Replace &nbsp; with a regular space
    std::regex nbsp_regex("&nbsp;");
    result = std::regex_replace(result, nbsp_regex, " ");

    result.erase(std::remove(result.begin(), result.end(), '\xA0'), result.end());

    // Step 5: Replace other numeric HTML entities with their ASCII equivalents
    result = replaceNumericEntities(result);

    // Step 5.2: Remove all character that are not between ASCII 0x0 and 0x80
    remove_non_ascii(result);

    // Step 5.5: Delete everything between <TITLE> and </TITLE> tags (case insensitive), including the tags
    std::regex title_regex("<title>.*?</title>", std::regex::icase);
    result = std::regex_replace(result, title_regex, "");

    // // Step 5.7: Replace newlines between <span and /span> with spaces
    // std::regex span_regex("<span[\\s\\S]*?/span>", std::regex::icase);
    // std::string::const_iterator search_start(result.cbegin());
    // std::smatch match;
    // std::vector<std::pair<size_t, std::pair<size_t, std::string>>> replacements;

    // // Find all span tags
    // while (std::regex_search(search_start, result.cend(), match, span_regex)) {
    //     size_t pos = std::distance(result.cbegin(), match[0].first);
    //     std::string span_content = match[0];
    //     size_t span_length = span_content.length();
    //     std::cout << "  span_content: " << span_content << span_length << std::endl;

    //     // Replace newlines with spaces within the span content
    //     std::string modified_span = span_content;
    //     size_t nl_pos = 0;
    //     while ((nl_pos = modified_span.find('\n', nl_pos)) != std::string::npos) {
    //         modified_span.replace(nl_pos, 1, " ");
    //         nl_pos += 1; // Move past the replacement
    //     }

    //     // Store the position, length, and replacement
    //     replacements.push_back({pos, {span_length, modified_span}});

    //     search_start = match.suffix().first;
    // }
    // std::cout << "  replacements: " << replacements.size() << std::endl;

    // // Apply replacements in reverse order to avoid invalidating positions
    // for (auto it = replacements.rbegin(); it != replacements.rend(); ++it) {
    //     result.replace(it->first, it->second.first, it->second.second);
    // }

    // Step 5.5: Change <BR /> to \n
    std::regex br_tag("<BR[ ]*/>", std::regex_constants::icase);
    result = std::regex_replace(result, br_tag, "\n");

    // Step 6: Remove all remaining HTML tags
    std::regex html_tag("<[^>]*>");
    result = std::regex_replace(result, html_tag, "");

    // Step 7: Normalize line endings, handle whitespace, and format specific lines
    result = normalizeAndFormatText(result);

    return result;
}

// // NOT USED ===========
// // Function to transform text to HTML format
// std::string transform2html(const std::string& input, const std::string& filename) {
//     // First, transform the input to text format
//     std::string textResult = transform2text(input);

//     // Keywords to look for (in uppercase)
//     std::vector<std::string> keywords = {
//                                          "OPENER", "CLOSER", "MIDDLE BREAK",
//                                          "FIGURE 1", "FIGURE 2", "FIGURE 3", "FIGURE 4", "FIGURE",
//                                          "TAG"};

//     // Process the text line by line to create HTML
//     std::istringstream iss(textResult);
//     std::ostringstream oss;
//     std::string line;

//     // Add HTML and BODY tags at the beginning
//     oss << "<!DOCTYPE html>\n";
//     oss << "<HTML>\n";
//     oss << "<HEAD>\n";
//     oss << "    <META charset=\"UTF-8\">\n";
//     oss << "    <TITLE>" << filename << "</TITLE>\n";
//     oss << "</HEAD>\n";
//     oss << "<BODY>\n";

//     // Process each line
//     while (std::getline(iss, line)) {
//         // Skip empty lines but still add a line break
//         if (line.empty()) {
//             oss << "    <BR>\n";
//             continue;
//         }

//         // Add indentation (4 spaces)
//         oss << "    ";

//         // Check if the line starts with any of the keywords
//         bool keyword_found = false;
//         for (const auto& keyword : keywords) {
//             if (line.find(keyword) == 0) {
//                 // Add span tags around the keyword
//                 size_t keywordLength = keyword.length();
//                 std::string beforeKeyword = line.substr(0, 0); // Empty string before the keyword
//                 std::string afterKeyword = line.substr(keywordLength); // Everything after the keyword

//                 oss << beforeKeyword << "<SPAN class=\"hdr\">" << keyword << "</SPAN>" << afterKeyword << "<BR>\n";
//                 keyword_found = true;
//                 break;
//             }
//         }

//         // If no keyword was found, just add the line as is
//         if (!keyword_found) {
//             oss << line << "<BR>\n";
//         }
//     }

//     // Add closing BODY and HTML tags
//     oss << "</BODY>\n</HTML>\n";

//     return oss.str();
// }

// DEBUG ------------
// bool writeStringToFile(const QString& filePath, const QString& content) {
//     QFile file(filePath);
//     if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//         return false;
//     }
//     QTextStream out(&file);
//     out << content;
//     file.close();
//     return true;
// }

// Automatically format a cuesheet:
void MainWindow::on_actionAuto_format_Lyrics_triggered()
{
    // qDebug() << "AUTO FORMAT CUESHEET";

    SelectionRetainer retainer(ui->textBrowserCueSheet);
    QTextCursor cursor = ui->textBrowserCueSheet->textCursor();

    // now look at it as HTML
    QString selected = cursor.selection().toHtml();
    // qDebug() << "***** STEP 1 *****\n" << selected << "\n***** DONE *****";

    if (selected.isEmpty()) {
        int ret = QMessageBox::question(this, tr("Auto Format Entire Cuesheet?"),
                                       tr("You have not selected any text to Auto Format.\n"
                                          "Do you want to Auto Format the entire cuesheet?"),
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No);


        if (ret == QMessageBox::Yes) {
            // select everything and fetch it
            ui->textBrowserCueSheet->selectAll();
            cursor = ui->textBrowserCueSheet->textCursor();
            selected = cursor.selection().toHtml();
        } else if (ret == QMessageBox::No) {
            return;
        }
    }

    // // TODO: USE NEW TRANSFORM() HERE ----
    // QString transformed0 = selected;
    QString transformed1 = QString::fromStdString(transform2text(selected.toStdString()));
    // QString transformed2 = QString::fromStdString(transform2html(selected.toStdString(), ""));
    // writeStringToFile("/Users/mpogue/testAAA0.html", selected);
    // writeStringToFile("/Users/mpogue/testAAA1.html", transformed1);
    // writeStringToFile("/Users/mpogue/testAAA2.html", transformed2);
    // qDebug() << "***** STEP 1.5 *****\n" << transformed << "\n***** DONE *****";

    selected = transformed1;

    // Qt gives us a whole HTML doc here.  Strip off all the parts we don't want.
    QRegularExpression startSpan("<span.*>", QRegularExpression::InvertedGreedinessOption);  // don't be greedy!

    selected.replace(QRegularExpression("<.*<!--StartFragment-->"),"")
        .replace(QRegularExpression("<!--EndFragment-->.*</html>"),"")
        .replace(startSpan,"")
        .replace("</span>","")
        ;

    //    qDebug() << "***** STEP 2 *****\n" << selected << "\n***** DONE *****";

    // SUPER CLEAN -------
    //    if (cursor.selectionStart() == 0 && cursor.atEnd()) {
    if (true) {
        // selection starts at beginning of file (position 0), and cursor is at end of file, means everything in the file is selected
        //   if this is the case, let's do a SUPER CLEAN to get rid of all formatting from say MS Word or OpenOffice.
        //            qDebug() << "BEFORE CLEANING: '" << selected << "'";
        selected
            .replace(QRegularExpression("<!DOCTYPE.*>"), "") // DOCTYPE needs to be gone, or extra /n at beginning of file will happen
            .replace(QRegularExpression("<HEAD>.*</HEAD>", QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption | QRegularExpression::InvertedGreedinessOption), "") // delete <head> section
            .replace(QRegularExpression("<BR *>",  QRegularExpression::CaseInsensitiveOption), "|BRBRBR|") // preserve line breaks
            .replace(QRegularExpression("<BR */>", QRegularExpression::CaseInsensitiveOption), "|BRBRBR|")
            //                .replace(QRegularExpression("<.*>", QRegularExpression::InvertedGreedinessOption), "") // delete ALL TAGS
            .replace("|BRBRBR|", "<BR>") // restore line breaks
            .replace(QRegularExpression("^(<BR>)+"), "")    // delete extra BR's at the start of the file
            .replace(QRegularExpression("^\n+"), "")        // delete extra NL's at the start of the file
            .replace("\n", "<BR>\n")     // restore line breaks
            ;
        //            qDebug() << "SUPER CLEANED: '" << selected << "'";
    }


    // DO THE HARD WORK HERE ---------------
    //    qDebug() << "***** STEP 3 *****\n" << selected << "\n***** DONE *****";

    QString headerStart("<span style=\" font-family:'Verdana'; font-size:x-large; color:#ff0002;\">"); // should that 25 be 23?
    QString headerEnd("</span>");

    selected
        .replace(QRegularExpression("<BR>\nOPENER",       QRegularExpression::CaseInsensitiveOption),   "<BR>\n" + headerStart + "OPENER" + headerEnd)
        .replace(QRegularExpression("<BR>\nFIGURE",       QRegularExpression::CaseInsensitiveOption),   "<BR>\n" + headerStart + "FIGURE" + headerEnd)
        .replace(QRegularExpression("<BR>\nFIGURE 1",     QRegularExpression::CaseInsensitiveOption),   "<BR>\n" + headerStart + "FIGURE 1" + headerEnd)
        .replace(QRegularExpression("<BR>\nFIGURE 2",     QRegularExpression::CaseInsensitiveOption),   "<BR>\n" + headerStart + "FIGURE 2" + headerEnd)
        .replace(QRegularExpression("<BR>\nBREAK",        QRegularExpression::CaseInsensitiveOption),   "<BR>\n" + headerStart + "MIDDLE BREAK" + headerEnd)
        .replace(QRegularExpression("<BR>\nMIDDLE BREAK", QRegularExpression::CaseInsensitiveOption),   "<BR>\n" + headerStart + "MIDDLE BREAK" + headerEnd)
        .replace(QRegularExpression("<BR>\nFIGURE 3",     QRegularExpression::CaseInsensitiveOption),   "<BR>\n" + headerStart + "FIGURE 3" + headerEnd)
        .replace(QRegularExpression("<BR>\nFIGURE 4",     QRegularExpression::CaseInsensitiveOption),   "<BR>\n" + headerStart + "FIGURE 4" + headerEnd)
        .replace(QRegularExpression("<BR>\nCLOSER",       QRegularExpression::CaseInsensitiveOption),   "<BR>\n" + headerStart + "CLOSER" + headerEnd)
        .replace(QRegularExpression("<BR>\nTAG",          QRegularExpression::CaseInsensitiveOption),   "<BR>\n" + headerStart + "TAG"    + headerEnd)
        .replace(QRegularExpression("HEADS ",           QRegularExpression::CaseInsensitiveOption),   "HEADS ")
        .replace(QRegularExpression("SIDES ",           QRegularExpression::CaseInsensitiveOption),   "SIDES ")
        .replace(QRegularExpression("BOYS ",            QRegularExpression::CaseInsensitiveOption),   "BOYS ")
        .replace(QRegularExpression("GIRLS ",           QRegularExpression::CaseInsensitiveOption),   "GIRLS ")
        .replace(QRegularExpression("<html><body><BR>\n", QRegularExpression::CaseInsensitiveOption), "<html><body>") // get rid of extra NL at start
        ;

    //    qDebug() << "***** STEP 4 *****\n" << selected << "\n***** DONE *****";

    // WARNING: this has a dependency on internal cuesheet2.css's definition of BODY text.
    QString HTMLreplacement =
        "<span style=\" font-family:'Verdana'; font-size:large; color:#000000;\">" +
        selected +
        "</span>";

    cursor.beginEditBlock(); // start of grouping for UNDO purposes
    cursor.removeSelectedText();  // remove the rich text...
    cursor.insertHtml(HTMLreplacement);  // ...and put back in the stripped-down text
    cursor.endEditBlock(); // end of grouping for UNDO purposes

    // qDebug() << "FINAL" << HTMLreplacement << "***************";

    maybeLoadCSSfileIntoTextBrowser(true); // it's now a SquareDesk cuesheet, so restore the SquareDesk CSS
}
