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

#ifndef CUESHEETCONVERTER_H
#define CUESHEETCONVERTER_H

#include <QByteArray>
#include <QString>
#include <QStringList>

// Converts a cuesheet from arbitrary source HTML (Word exports, Riverboat-style
//   classed files, older SquareDesk files, ...) into SquareDesk's normalized
//   cuesheet format: a fixed <STYLE> block, and a flat <BODY> made only of
//   classed <SPAN>s (title, label, artist, hdr, lyrics, lyrics2, lyrics3, bold),
//   <BR>s, plain text and <A> links.  See GitHub issue #1789.
//
// The source is parsed by QTextDocument.  Class names do not survive that parse,
//   so before parsing, a <STYLE> block is injected that maps each semantic class
//   to a marker font-family, which does survive in each run's QTextCharFormat.
namespace CuesheetConverter {

enum class Mode {
    Auto,     // lyrics = "lyrics*" class, or any highlight/shading colour
    Italic,   // also: italic text is lyrics
    NoBold    // also: a body line with no bold text at all is lyrics
};

struct Result {
    QString html;
    QStringList warnings;
    QString info;          // e.g. "5 sections, 18 lyric lines"
};

// Decodes the raw bytes as UTF-8 if valid, otherwise as Windows-1252.
QString decode(const QByteArray &bytes);

// fixCase normalizes letter case in the body:
//   - lyrics: first word Capitalized, the rest lower case (keeping "I", "God",
//     month names, ..., and probable names in otherwise lower-case lyrics);
//   - everything else (calls, notes): Every Word Capitalized, except the "who"
//     words (HEADS, SIDES, BOYS, GIRLS, MEN, LADIES, ...), which are ALL CAPS.
//   Headings, title, label and artist are not affected.
Result convert(const QString &sourceHtml, Mode mode, const QString &versionString, bool fixCase = true);

// Debugging aid: the flattened lines and segments that convert() works from.
QString dumpLines(const QString &sourceHtml);

// Shared section-heading detector (also intended for Auto-format and #1772).
//   Recognizes lines like "OPENER", "FIGURE 1+2", "Opener - Middle Break - Closer",
//   "Break Lyrics", "FIGURE 1 (mainstream)", and heading-plus-call lines like
//   "Open : Circle Left" or (ALL-CAPS keyword only) "OPENER Circle Left".
//   Rejects lines like "Tag the line".
struct HeadingMatch {
    bool matched = false;
    QString heading;       // normalized: uppercase, single spaces, no spaces around "/"
    QString rest;          // a trailing parenthetical, or the call that follows
    int restStart = -1;    // where rest begins in lineText, or -1 if there is none
};
HeadingMatch matchHeading(const QString &lineText);

} // namespace CuesheetConverter

#endif // CUESHEETCONVERTER_H
