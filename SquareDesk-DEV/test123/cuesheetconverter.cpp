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

#include "cuesheetconverter.h"

#include <QColor>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QStringDecoder>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextFragment>
#include <QTextFrame>

namespace CuesheetConverter {

namespace {

// Marker font-families injected for the semantic classes (see header comment).
const QString kMarkerPrefix = QStringLiteral("sdmark-");

const char *kMarkerStyle =
    "<style>\n"
    ".hdr, .opener, .break, .closer, .figure1, .figure2, .figure3, .figure4, .tag,"
    " .breaklyrics, .closerlyrics, .altlyrics { font-family: \"sdmark-hdr\" !important; }\n"
    ".title   { font-family: \"sdmark-title\" !important; }\n"
    ".label   { font-family: \"sdmark-label\" !important; }\n"
    ".artist  { font-family: \"sdmark-artist\" !important; }\n"
    ".courtesy, .former { font-family: \"sdmark-courtesy\" !important; }\n"
    ".lyrics  { font-family: \"sdmark-lyrics\" !important; }\n"
    ".lyrics2 { font-family: \"sdmark-lyrics2\" !important; }\n"
    ".lyrics3 { font-family: \"sdmark-lyrics3\" !important; }\n"
    "</style>\n";

const char *kOutputStyle =
    "body, p, font { font-family: Verdana; font-size: large; font-weight: Normal; color: #000000; background: #FFFFE0; margin-top: 0px; margin-bottom: 0px;}\n"
    "        .title        { font-family: Verdana; font-size: x-large; font-weight: 699;    color: #010101;}\n"
    "        .label        { font-family: Verdana; font-size: medium;  font-weight: Normal; color: #60C060;}\n"
    "        .artist       { font-family: Verdana; font-size: medium;  font-weight: Normal; color: #0000FF;}\n"
    "        .hdr          { font-family: Verdana; font-size: x-large; font-weight: Normal; color: #FF0002;}\n"
    "        .lyrics       { font-family: Verdana; font-size: large;   font-weight: Normal; color: #030303; background-color: #FFC0CB;}\n"
    "        .lyrics2      { font-family: Verdana; font-size: large;   font-weight: Normal; color: #040404; background-color: #C0D9FF;}\n"
    "        .lyrics3      { font-family: Verdana; font-size: large;   font-weight: Normal; color: #050505; background-color: #C0FFD9;}\n"
    "        .bold         { font-weight: bold; }\n"
    "        .italic       { font-style: italic; }";

struct Seg {
    QString t;
    QString ly;       // "lyrics", "lyrics2", "lyrics3", or "#rrggbb" for a highlight colour
    QString k;        // "title", "label", "artist", "courtesy"
    QString href;
    bool h = false;
    bool b = false;
    bool i = false;
    QColor fg;          // text colour, if set
    qreal size = 0;     // point size, if set
    bool keepCase = false;   // a note whose case fixCase() leaves alone
};

struct Line {
    QList<Seg> s;
    QString txt;
    int headingLevel = 0;
    int block = -1;           // which QTextBlock (paragraph) the line came from
    int hdrLinesInBlock = 0;  // lines in that block marked as headings (class hdr, ...)
    bool isHdr = false;
    QString hdr;
    QList<Seg> rest;      // what follows the heading keyword on its line (may contain lyrics)
    bool restIsNote = false;   // rest is a note like "(mainstream)" or "Basic", not a call
};

QString concatText(const QList<Seg> &s)
{
    QString r;
    for (const Seg &x : s) {
        r += x.t;
    }
    return r;
}

// The segments from character offset off onwards.
QList<Seg> segsFrom(const QList<Seg> &s, int off)
{
    QList<Seg> r;
    int pos = 0;
    for (const Seg &x : s) {
        const int end = pos + x.t.length();
        if (end > off) {
            Seg y = x;
            if (pos < off) {
                y.t = x.t.mid(off - pos);
            }
            r.append(y);
        }
        pos = end;
    }
    return r;
}

// The segments before character offset off.
QList<Seg> segsBefore(const QList<Seg> &s, int off)
{
    QList<Seg> r;
    int pos = 0;
    for (const Seg &x : s) {
        if (pos >= off) {
            break;
        }
        Seg y = x;
        y.t = x.t.left(off - pos);
        r.append(y);
        pos += x.t.length();
    }
    return r;
}

bool anyLyricBefore(const QList<Seg> &s, int off)
{
    int pos = 0;
    for (const Seg &x : s) {
        if (pos >= off) {
            break;
        }
        if (!x.ly.isEmpty() && !x.t.trimmed().isEmpty()) {
            return true;
        }
        pos += x.t.length();
    }
    return false;
}

QString esc(const QString &s)
{
    QString r = s;
    r.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;");
    return r;
}

QString collapseSpaces(const QString &s)
{
    static const QRegularExpression ws(QStringLiteral("[\\s\\x{00A0}\\x{2000}-\\x{200B}\\x{202F}\\x{3000}]+"));
    QString r = s;
    r.replace(ws, QStringLiteral(" "));
    return r;
}

// Page and table backgrounds (#FAFAFA, #FFFFE0, #FFF9CF, white, ...) are not highlights:
//   very light and nearly unsaturated.  Pale highlights like pink #FFC0CB (25%) and
//   #FCC9C3 (23%) are more saturated than #FFF9CF (19%).
bool isNearWhite(const QColor &c)
{
    const QColor hsv = c.toHsv();
    return hsv.valueF() >= 0.94 && hsv.hsvSaturationF() < 0.21;
}

QString markerOf(const QTextCharFormat &fmt)
{
    const QStringList fams = fmt.fontFamilies().toStringList();
    for (const QString &f : fams) {
        if (f.startsWith(kMarkerPrefix, Qt::CaseInsensitive)) {
            return f.mid(kMarkerPrefix.length()).toLower();
        }
    }
    return QString();
}

// Prepares the raw source for QTextDocument: drops embedded images (which Qt
//   would otherwise decode), maps Word's mso-highlight onto background-color,
//   and injects the marker style block after any source styles.
QString prepareSource(const QString &src)
{
    QString html = src;
    static const QRegularExpression dataSrc(QStringLiteral("src\\s*=\\s*([\"'])data:.*?\\1"),
                                            QRegularExpression::CaseInsensitiveOption |
                                            QRegularExpression::DotMatchesEverythingOption);
    html.replace(dataSrc, QStringLiteral("src=\"\""));
    static const QRegularExpression msoHighlight(QStringLiteral("mso-highlight\\s*:"),
                                                 QRegularExpression::CaseInsensitiveOption);
    html.replace(msoHighlight, QStringLiteral("background-color:"));

    int at = html.indexOf(QStringLiteral("</head"), 0, Qt::CaseInsensitive);
    if (at < 0) {
        at = html.indexOf(QStringLiteral("<body"), 0, Qt::CaseInsensitive);
    }
    if (at < 0) {
        at = 0;
    }
    html.insert(at, QString::fromLatin1(kMarkerStyle));
    return html;
}

// Flattens the document into lines of styled segments.  A block (P, DIV, H1,
//   table cell, ...) starts a new line, and so does each <BR> within a block.
QList<Line> rawLines(const QTextDocument &doc)
{
    QList<Line> out;
    Line cur;
    // Text whose background just repeats the page's (e.g. SquareDesk's #FFFFE0) is not highlighted.
    QColor pageBg;
    const QBrush rootBg = doc.rootFrame()->frameFormat().background();
    if (rootBg.style() != Qt::NoBrush) {
        pageBg = rootBg.color();
    }
    auto newLine = [&]() { out.append(cur); cur = Line(); };

    int blockNo = 0;
    for (QTextBlock blk = doc.begin(); blk.isValid(); blk = blk.next(), blockNo++) {
        newLine();
        cur.block = blockNo;
        const QTextBlockFormat bf = blk.blockFormat();
        QColor blockBg;
        if (bf.background().style() != Qt::NoBrush) {
            blockBg = bf.background().color();
        }
        cur.headingLevel = bf.headingLevel();

        for (QTextBlock::iterator it = blk.begin(); !it.atEnd(); ++it) {
            const QTextFragment frag = it.fragment();
            if (!frag.isValid()) {
                continue;
            }
            const QTextCharFormat fmt = frag.charFormat();
            const QString marker = markerOf(fmt);

            Seg proto;
            if (marker == "hdr") {
                proto.h = true;
            } else if (marker == "title" || marker == "label" || marker == "artist" || marker == "courtesy") {
                proto.k = marker;
            } else if (marker.startsWith("lyrics")) {
                proto.ly = marker;
            }
            if (proto.ly.isEmpty() && !proto.h) {     // a heading's background is decoration
                QColor bg;
                if (fmt.background().style() != Qt::NoBrush) {
                    bg = fmt.background().color();
                } else {
                    bg = blockBg;
                }
                if (bg.isValid() && bg.alpha() > 0 && bg != pageBg && !isNearWhite(bg)) {
                    proto.ly = bg.name();   // resolved to lyrics/lyrics2/lyrics3 later
                }
            }
            proto.b = fmt.fontWeight() >= 600;
            proto.i = fmt.fontItalic();
            if (fmt.foreground().style() != Qt::NoBrush) {
                proto.fg = fmt.foreground().color();
            }
            proto.size = fmt.fontPointSize();
            if (fmt.isAnchor() && !fmt.anchorHref().isEmpty()) {
                proto.href = fmt.anchorHref();
            }

            QString text = frag.text();
            text.remove(QChar::ObjectReplacementCharacter);    // images
            const QStringList parts = text.split(QChar::LineSeparator);   // <BR>
            for (int p = 0; p < parts.size(); p++) {
                if (p > 0) {
                    newLine();
                    cur.block = blockNo;
                    cur.headingLevel = bf.headingLevel();
                }
                const QString t = collapseSpaces(parts[p]);
                if (!t.isEmpty()) {
                    Seg s = proto;
                    s.t = t;
                    cur.s.append(s);
                }
            }
        }
    }
    newLine();

    // Trim, and drop empty lines.
    QList<Line> lines;
    for (Line l : std::as_const(out)) {
        QList<Seg> segs;
        for (const Seg &s : std::as_const(l.s)) {
            if (!s.t.isEmpty()) {
                segs.append(s);
            }
        }
        if (segs.isEmpty()) {
            continue;
        }
        if (segs.first().t.startsWith(' ')) {
            segs.first().t.remove(0, 1);
        }
        if (segs.last().t.endsWith(' ')) {
            segs.last().t.chop(1);
        }
        l.s.clear();
        QString txt;
        for (const Seg &s : std::as_const(segs)) {
            if (!s.t.isEmpty()) {
                l.s.append(s);
                txt += s.t;
            }
        }
        l.txt = txt.trimmed();
        if (!l.txt.isEmpty()) {
            lines.append(l);
        }
    }
    QHash<int, int> perBlock;
    for (const Line &l : std::as_const(lines)) {
        for (const Seg &x : l.s) {
            if (x.h) {
                perBlock[l.block]++;
                break;
            }
        }
    }
    for (Line &l : lines) {
        l.hdrLinesInBlock = perBlock.value(l.block);
    }
    return lines;
}

// Words with at least one letter or digit ("OPENER - CLOSER" is 2 words).
int wordCount(const QString &s)
{
    static const QRegularExpression word(QStringLiteral("[\\p{L}\\p{N}]+"));
    int n = 0;
    QRegularExpressionMatchIterator it = word.globalMatch(s);
    while (it.hasNext()) {
        it.next();
        n++;
    }
    return n;
}

QString normalizeHeading(const QString &s)
{
    static const QRegularExpression slash(QStringLiteral("\\s*/\\s*"));
    QString r = collapseSpaces(s).trimmed().toUpper();
    r.replace(slash, QStringLiteral("/"));
    while (r.endsWith(':') || r.endsWith('.') || r.endsWith('-') || r.endsWith(QChar(0x2013)) || r.endsWith(' ')) {
        r.chop(1);
    }
    if (r.startsWith('(') && r.endsWith(')')) {     // "(OPENER)"
        r = r.mid(1, r.length() - 2).trimmed();
    } else if (r.startsWith('(') && !r.contains(')')) {
        r = r.mid(1).trimmed();
    }
    return r;
}

// ---------- Case normalization ----------

// Dancer designators ("who" words) stay ALL CAPS in calls.  Plurals only: singulars
//   are usually not designators ("Swing Your Girl", "Pass To The Center").
const QSet<QString> &whoWords()
{
    static const QSet<QString> w = {
        "HEADS", "SIDES", "BOYS", "MEN", "GENTS", "GIRLS", "LADIES",
        "CENTERS", "ENDS", "LEADS", "LEADERS", "TRAILERS", "BEAUS", "BELLES"
    };
    return w;
}

// Abbreviations that stay ALL CAPS in calls when the source has them that way.
const QSet<QString> &abbreviations()
{
    static const QSet<QString> w = { "MS", "SSD", "DBD", "RLG", "DPT", "CW", "CCW", "OK", "TV", "USA", "II", "III" };
    return w;
}

// Small words that stay lower case in calls, except as the first word.  Not "in",
//   "on", "out", "up" or "thru", which are often part of a call ("Face In").
const QSet<QString> &smallWords()
{
    static const QSet<QString> w = {
        "a", "an", "and", "the", "to", "of", "or", "nor", "but", "for", "at", "by", "with", "from", "as"
    };
    return w;
}

// Words that are always capitalized, even inside a lyric; keyed by lower case.
const QHash<QString, QString> &alwaysCapitalized()
{
    static const QHash<QString, QString> w = [] {
        QHash<QString, QString> h;
        const QStringList words = {
            "I", "I'm", "I'll", "I'd", "I've", "God", "Lord", "Jesus", "Christ", "Christmas", "Santa",
            "Hanukkah", "Easter", "America", "American",
            "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday",
            "January", "February", "April", "June", "July", "September", "October", "November", "December"
        };
        for (const QString &x : words) {
            h.insert(x.toLower(), x);
            if (x.contains('\'')) {
                QString curly = x;
                curly.replace('\'', QChar(0x2019));
                h.insert(curly.toLower(), curly);
            }
        }
        return h;
    }();
    return w;
}

QString capitalized(const QString &w)
{
    return w.left(1).toUpper() + w.mid(1).toLower();
}

bool hasInnerUpper(const QString &w)
{
    for (int i = 1; i < w.length(); i++) {
        if (w[i].isUpper()) {
            return true;
        }
    }
    return false;
}

// Calls: every word Capitalized, except "who" words (ALL CAPS), small words like
//   "and"/"the"/"to" after the first word, and a word right after an apostrophe ("'em").
QString callCase(const QString &text)
{
    static const QRegularExpression word(QStringLiteral("\\p{L}[\\p{L}'\\x{2019}]*"));
    QString r = text;
    QRegularExpressionMatchIterator it = word.globalMatch(text);
    bool firstWord = true;
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const QString w = m.captured(0);
        const QString up = w.toUpper();
        QString out;
        if (whoWords().contains(up)) {
            out = up;
        } else if (abbreviations().contains(up) && w == up) {
            out = up;
        } else if (!firstWord && smallWords().contains(w.toLower())) {
            out = w.toLower();
        } else if (m.capturedStart() > 0 && (text[m.capturedStart() - 1] == '\'' || text[m.capturedStart() - 1] == QChar(0x2019))) {
            out = w.toLower();
        } else {
            out = capitalized(w);
        }
        r.replace(m.capturedStart(), w.length(), out);
        firstWord = false;
    }
    return r;
}

// Counts the words after the first in a lyric span, and how many of them start upper case.
void countLyricCaps(const QString &text, int &nWords, int &nCap)
{
    static const QRegularExpression word(QStringLiteral("\\p{L}[\\p{L}'\\x{2019}]*"));
    QRegularExpressionMatchIterator it = word.globalMatch(text);
    bool firstWord = true;
    while (it.hasNext()) {
        const QString w = it.next().captured(0);
        if (firstWord) {
            firstWord = false;
            continue;
        }
        nWords++;
        if (w[0].isUpper()) {
            nCap++;
        }
    }
}

// Lyrics: the first word Capitalized, the rest lower case, except words that are
//   always capitalized.  If the cuesheet's lyrics are mostly lower case already
//   (!shouty) and this span isn't ALL CAPS, a normally Capitalized word is kept
//   as-is, since it is probably a name ("Winslow Arizona").
QString lyricCase(const QString &text, bool shouty)
{
    static const QRegularExpression word(QStringLiteral("\\p{L}[\\p{L}'\\x{2019}]*"));
    QList<QRegularExpressionMatch> words;
    QRegularExpressionMatchIterator it = word.globalMatch(text);
    while (it.hasNext()) {
        words.append(it.next());
    }
    if (text == text.toUpper() && text != text.toLower()) {
        shouty = true;      // an ALL CAPS span
    }

    QString r = text;
    for (int i = 0; i < words.size(); i++) {
        const QString w = words[i].captured(0);
        QString out;
        if (alwaysCapitalized().contains(w.toLower())) {
            out = alwaysCapitalized().value(w.toLower());
        } else if (i == 0) {
            out = capitalized(w);
        } else if (shouty || hasInnerUpper(w)) {
            out = w.toLower();
        } else {
            out = w;
        }
        r.replace(words[i].capturedStart(), w.length(), out);
    }
    return r;
}

// Applies callCase/lyricCase to each run of non-lyric/lyric segments.  Case
//   mapping is done per run (so a word split across segments is one word) and the
//   result written back into the segments by length.
void fixCase(QList<Seg> &segs, bool shouty)
{
    int i = 0;
    while (i < segs.size()) {
        const bool ly = !segs[i].ly.isEmpty();
        const bool keep = segs[i].keepCase;
        int j = i;
        QString run;
        while (j < segs.size() && (!segs[j].ly.isEmpty() == ly) && segs[j].keepCase == keep) {
            run += segs[j++].t;
        }
        QString fixed = keep ? run : ly ? lyricCase(run, shouty) : callCase(run);
        if (fixed.length() != run.length()) {
            fixed = run;    // a case mapping that changes length (rare): leave the run alone
        }
        int pos = 0;
        for (int k = i; k < j; k++) {
            const int n = segs[k].t.length();
            segs[k].t = fixed.mid(pos, n);
            pos += n;
        }
        i = j;
    }
}

QString renderLine(QList<Seg> s, const QHash<QString, QString> &lyClass)
{
    auto classOf = [&](const Seg &x) { return lyClass.value(x.ly, QStringLiteral("lyrics")); };
    // Closing punctuation that starts a lyric in another colour belongs to the one
    //   before it: "(When I Hear You Breathe" + ") When I hear you breathe" (RI 963).
    static const QRegularExpression closing(QStringLiteral("^[)\\]\\x{201D}]+"));
    for (int k = 1; k < s.size(); k++) {
        if (!s[k].ly.isEmpty() && !s[k - 1].ly.isEmpty() && classOf(s[k]) != classOf(s[k - 1])) {
            const QRegularExpressionMatch m = closing.match(s[k].t);
            if (m.hasMatch() && m.capturedLength() < s[k].t.length()) {
                s[k - 1].t += m.captured(0);
                s[k].t.remove(0, m.capturedLength());
            }
        }
    }

    QString h;
    int i = 0;
    while (i < s.size()) {
        if (!s[i].ly.isEmpty()) {
            // One span per run of the same lyric class (a line may mix singers).
            const QString cls = classOf(s[i]);
            QList<Seg> g;
            int j = i;
            while (j < s.size() && ((!s[j].ly.isEmpty() && classOf(s[j]) == cls) ||
                                    (s[j].ly.isEmpty() && s[j].t.trimmed().isEmpty()))) {
                g.append(s[j++]);
            }
            while (!g.isEmpty() && g.last().ly.isEmpty()) {
                g.removeLast();
                j--;
            }
            bool lead = false;
            while (g.first().t.startsWith(' ')) {
                g.first().t.remove(0, 1);
                lead = true;
            }
            if (lead && !h.isEmpty() && !h.endsWith(' ')) {
                h += ' ';
            }
            bool tail = false;
            while (g.last().t.endsWith(' ')) {
                g.last().t.chop(1);
                tail = true;
            }

            int words = 0, bolds = 0;
            for (const Seg &x : std::as_const(g)) {
                if (!x.t.trimmed().isEmpty()) {
                    words++;
                    if (x.b) {
                        bolds++;
                    }
                }
            }
            // A few bold words inside a lyric are meaningful; mostly-bold is just styling.
            const bool markBold = bolds > 0 && bolds <= words * 0.4;

            QString inner;
            for (const Seg &x : std::as_const(g)) {
                if (markBold && x.b && !x.t.trimmed().isEmpty()) {
                    if (x.t.startsWith(' ')) {
                        inner += ' ';
                    }
                    inner += QStringLiteral("<SPAN class=\"bold\">") + esc(x.t.trimmed()) + QStringLiteral("</SPAN>");
                    if (x.t.endsWith(' ')) {
                        inner += ' ';
                    }
                } else {
                    inner += esc(x.t);
                }
            }
            h += QStringLiteral("<SPAN class=\"%1\">%2</SPAN>").arg(cls, inner);
            if (tail) {
                h += ' ';
            }
            i = j;
        } else {
            const Seg &x = s[i++];
            if (!x.href.isEmpty()) {
                h += QStringLiteral("<A href=\"%1\">%2</A>").arg(esc(x.href), esc(x.t));
            } else {
                h += esc(x.t);
            }
        }
    }
    return h;
}

// The calls that start a section, which belong on the heading's line.  "Swing ..."
//   means Swing Her, Swing Your Partner, etc., but not the call Swing Thru.
bool isMergeableCall(const QString &text)
{
    static const QRegularExpression punct(QStringLiteral("[^\\p{L}\\p{N}&\\s]"));
    static const QRegularExpression ws(QStringLiteral("\\s+"));
    static const QRegularExpression calls(
        QStringLiteral("^(?:circle left|circle right|heads face grand square|sides face grand square"
                       "|grand square|teacup chain|swing\\b(?!\\s+thru\\b).*)$"),
        QRegularExpression::CaseInsensitiveOption);
    QString t = text;
    t.replace(punct, QStringLiteral(" ")).replace(ws, QStringLiteral(" "));
    return calls.match(t.trimmed()).hasMatch();
}

} // namespace

QString decode(const QByteArray &bytes)
{
    QStringDecoder utf8(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
    QString s = utf8.decode(bytes);
    if (!utf8.hasError()) {
        if (s.startsWith(QChar(0xFEFF))) {
            s.remove(0, 1);
        }
        return s;
    }
    // Windows-1252: Latin-1, except for 0x80-0x9F.
    static const char16_t cp1252[32] = {
        0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
        0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
        0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
        0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178
    };
    QString r;
    r.reserve(bytes.size());
    for (char ch : bytes) {
        const unsigned char c = static_cast<unsigned char>(ch);
        r += (c >= 0x80 && c <= 0x9F) ? QChar(cp1252[c - 0x80]) : QChar(c);
    }
    return r;
}

HeadingMatch matchHeading(const QString &lineText)
{
    static const QString figNums = QStringLiteral(
        "(?:\\s*#?\\s*[-\\x{2013}]?\\s*\\d+(?:\\s*(?:[.,&+]|and|[-\\x{2013}])?\\s*\\d+)*\\.?)?");
    static const QString token = QStringLiteral(
        "(?:OPENER|OPENING|OPEN|INTRO(?:DUCTION)?|MIDDLE\\s+BREAK|MIDDLE|BREAKS?|CLOSER|CLOSING|CLOSE"
        "|ENDING|INSTRUMENTAL(?:\\s+TAG)?|TAG|BRIDGE|FIGURES?%1"
        "|(?:ALTERNATE|ALT|ADDITIONAL|EXTRA)\\s+LYRICS|LYRICS)").arg(figNums);
    static const QString sep = QStringLiteral("(?:\\s*(?:[/&,+]|[-\\x{2013}]|\\band\\b)\\s*|\\s+)");
    static const QRegularExpression kw(
        QStringLiteral("^\\s*\\(?\\s*%1(?:%2%1)*(?![\\w])(?:\\s*\\))?").arg(token, sep),   // "(Opener):" too
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression restEmpty(QStringLiteral("^\\s*[:.]?\\s*(\\(.*\\))?\\s*$"));
    static const QRegularExpression restCall(QStringLiteral("^\\s*:\\s*(\\S.*)$"));
    static const QRegularExpression restAfterCaps(QStringLiteral("^\\s*(?:[:.\\-\\x{2013}]\\s*)?(\\S.*)$"));
    static const QRegularExpression hasLower(QStringLiteral("\\p{Ll}"));

    HeadingMatch hm;
    const QRegularExpressionMatch m = kw.match(lineText);
    if (!m.hasMatch()) {
        return hm;
    }
    const QString rest = lineText.mid(m.capturedEnd());
    QRegularExpressionMatch r = restEmpty.match(rest);
    if (r.hasMatch()) {
        hm.matched = true;
        hm.heading = normalizeHeading(m.captured(0));
        hm.rest = r.captured(1).trimmed();
        if (!hm.rest.isEmpty()) {
            hm.restStart = m.capturedEnd() + r.capturedStart(1);
        }
        return hm;
    }
    r = restCall.match(rest);
    // "OPENER Circle Left": an ALL-CAPS keyword followed directly by the call.
    //   (A mixed-case "Tag the line" is a call, not a heading.)
    if (!r.hasMatch() && !m.captured(0).contains(hasLower)) {
        r = restAfterCaps.match(rest);
        hm.callAfterCaps = r.hasMatch();
    }
    if (r.hasMatch()) {
        hm.matched = true;
        hm.heading = normalizeHeading(m.captured(0));
        hm.rest = r.captured(1).trimmed();
        hm.restStart = m.capturedEnd() + r.capturedStart(1);
    }
    return hm;
}

QString dumpLines(const QString &sourceHtml)
{
    QTextDocument doc;
    doc.setHtml(prepareSource(sourceHtml));
    QString r;
    for (const Line &l : rawLines(doc)) {
        r += QStringLiteral("[h%1] ").arg(l.headingLevel);
        for (const Seg &x : l.s) {
            r += QStringLiteral("{%1|ly=%2 k=%3 h=%4 b=%5 i=%6}").arg(x.t, x.ly, x.k).arg(x.h).arg(x.b).arg(x.i);
        }
        r += '\n';
    }
    return r;
}

Result convert(const QString &sourceHtml, Mode mode, const QString &versionString, bool fixCaseOn,
               const QString &fileName)
{
    Result res;

    QTextDocument doc;
    doc.setHtml(prepareSource(sourceHtml));
    QList<Line> L = rawLines(doc);

    if (mode == Mode::Italic) {
        for (Line &l : L) {
            for (Seg &x : l.s) {
                if (x.i && x.ly.isEmpty()) {
                    x.ly = QStringLiteral("lyrics");
                }
            }
        }
    }

    // Section headings.  A line explicitly marked as a heading (class hdr, etc.)
    //   is a heading as-is if it has no lyrics, is short (8 words at most), and is the
    //   only marked line in its paragraph.  (Some Word files mark a whole paragraph,
    //   or every line, of calls and lyrics as "hdr".)  Otherwise it must pass the
    //   keyword detector, and the keyword must not be lyrics.
    for (Line &l : L) {
        bool anyH = false, anyLy = false;
        for (const Seg &x : std::as_const(l.s)) {
            anyH = anyH || x.h;
            anyLy = anyLy || !x.ly.isEmpty();
        }
        const QString text = concatText(l.s);
        const HeadingMatch hm = matchHeading(text);
        if (anyH && !anyLy && l.hdrLinesInBlock == 1 && wordCount(l.txt) <= 8) {
            l.isHdr = true;
            // The heading ends where the text stops looking like it: "OPENER - CLOSER"
            //   in red, then "Basic" in small black text is a note, not heading.
            int split = -1, pos = 0;
            const Seg *firstSeg = nullptr;
            for (const Seg &x : std::as_const(l.s)) {
                if (!x.t.trimmed().isEmpty()) {
                    if (!firstSeg) {
                        firstSeg = &x;
                    } else if (x.fg != firstSeg->fg || x.size != firstSeg->size) {
                        split = pos;
                        break;
                    }
                }
                pos += x.t.length();
            }
            if (split > 0) {
                l.hdr = normalizeHeading(concatText(segsBefore(l.s, split)));
                l.rest = segsFrom(l.s, split);
                // ... shown as "(Basic)", in its own case.
                while (!l.rest.isEmpty() && l.rest.first().t.trimmed().isEmpty()) {
                    l.rest.removeFirst();
                }
                while (!l.rest.isEmpty() && l.rest.last().t.trimmed().isEmpty()) {
                    l.rest.removeLast();
                }
                if (!l.rest.isEmpty()) {
                    l.rest.first().t = l.rest.first().t.trimmed();
                    l.rest.last().t = l.rest.last().t.trimmed();
                    if (!(concatText(l.rest).startsWith('(') && concatText(l.rest).endsWith(')'))) {
                        l.rest.first().t.prepend('(');
                        l.rest.last().t.append(')');
                    }
                }
                for (Seg &x : l.rest) {
                    x.h = false;
                    x.keepCase = true;
                }
                l.restIsNote = true;
            } else {
                l.hdr = normalizeHeading(l.txt);
            }
            continue;
        }
        // The keyword itself must not be sung text; what follows it may be.  Some
        //   cuesheets highlight their headings instead of their lyrics, so a highlighted
        //   keyword is still a heading in the stricter forms ("Opener (Basic):",
        //   "Opener: Circle Left").
        const bool keywordLyric = hm.matched && anyLyricBefore(l.s, hm.restStart < 0 ? text.length() : hm.restStart);
        if (keywordLyric && !hm.callAfterCaps) {
            // Unhighlight the keyword, and a parenthetical after it, but not sung
            //   text after a colon ("Intro: I'll play for you").
            const bool parenOnly = hm.rest.isEmpty() || (hm.rest.startsWith('(') && hm.rest.endsWith(')'));
            const int end = parenOnly ? text.length() : hm.restStart;
            QList<Seg> head = segsBefore(l.s, end);
            for (Seg &x : head) {
                x.ly.clear();
            }
            l.s = head + segsFrom(l.s, end);
        }
        if (hm.matched && !(keywordLyric && hm.callAfterCaps)) {
            l.isHdr = true;
            l.hdr = hm.heading;
            if (anyH && hm.rest.startsWith('(') && hm.rest.endsWith(')')) {
                l.hdr = normalizeHeading(l.txt);    // a marked "FIGURE (long note)" stays whole
            } else if (hm.restStart >= 0) {
                l.rest = segsFrom(l.s, hm.restStart);
                l.restIsNote = hm.rest.startsWith('(') && hm.rest.endsWith(')');
                // "FIGURE (mainstream)" is a note, kept in its own case; but
                //   "OPENER (circle left)" is a call, so it is capitalized like one.
                if (l.restIsNote && !isMergeableCall(hm.rest.mid(1, hm.rest.length() - 2))) {
                    for (Seg &x : l.rest) {
                        x.keepCase = true;
                    }
                }
            }
        }
    }
    int first = -1;
    for (int n = 0; n < L.size(); n++) {
        if (L[n].isHdr) {
            first = n;
            break;
        }
    }
    // After the first heading, any short <H1>..<H6> line without lyrics is a heading
    //   too (e.g. "French Lyrics") -- unless most lines are <H1>..<H6>, as in Word
    //   files that style every paragraph as "Heading 1".  Long ones are calls.  So is
    //   one followed by another line of the same tag that isn't a heading, as in
    //   files that style some paragraphs of a section as "Heading 1" (BS 2623).
    int nHeadingTagged = 0;
    for (int n = qMax(first, 0); n < L.size(); n++) {
        if (L[n].headingLevel > 0) {
            nHeadingTagged++;
        }
    }
    if (first >= 0 && nHeadingTagged * 2 <= L.size() - first) {
        QList<bool> keywordHdr;
        for (const Line &l : std::as_const(L)) {
            keywordHdr.append(l.isHdr);
        }
        for (int n = first + 1; n < L.size(); n++) {
            bool anyLy = false;
            for (const Seg &x : std::as_const(L[n].s)) {
                anyLy = anyLy || !x.ly.isEmpty();
            }
            const bool nextSameTag = n + 1 < L.size() && L[n + 1].headingLevel == L[n].headingLevel &&
                                     !keywordHdr[n + 1];
            if (!L[n].isHdr && L[n].headingLevel > 0 && !anyLy && wordCount(L[n].txt) <= 4 && !nextSameTag) {
                L[n].isHdr = true;
                L[n].hdr = normalizeHeading(L[n].txt);
            }
        }
    }

    if (first < 0) {
        res.warnings << QStringLiteral("No section heading (Opener/Figure/...) found.");
    }
    QList<Line> head = first < 0 ? L.mid(0, 1) : L.mid(0, first);
    QList<Line> body = first < 0 ? L.mid(1) : L.mid(first);

    // A colour legend under the title ("...... kiyomi" in pink, "..... both" in yellow,
    //   ...): at least two short, wholly highlighted lines.  Kept as one line of lyric
    //   spans, so it shows which singer each lyrics class is.
    QList<Seg> legend;
    QList<int> legendLines;
    static const QRegularExpression legendTrim(QStringLiteral("^[^\\p{L}\\p{N}(]+|[^\\p{L}\\p{N})]+$"));
    for (int n = 1; n < head.size(); n++) {
        bool allLy = true;
        QString colour;
        for (const Seg &x : std::as_const(head[n].s)) {
            allLy = allLy && (x.ly.startsWith('#') || x.t.trimmed().isEmpty());
            if (colour.isEmpty()) {
                colour = x.ly;
            }
        }
        const QString name = QString(head[n].txt).remove(legendTrim);
        if (allLy && !colour.isEmpty() && !name.isEmpty() && wordCount(name) <= 3 && !matchHeading(name).matched) {
            if (!legend.isEmpty()) {
                Seg space;
                space.t = QStringLiteral(" ");
                legend.append(space);
            }
            Seg x;
            x.t = name;
            x.ly = colour;
            legend.append(x);
            legendLines.append(n);
        }
    }
    if (legendLines.size() < 2) {
        legend.clear();
    } else {
        for (int k = legendLines.size() - 1; k >= 0; k--) {
            head.removeAt(legendLines[k]);
        }
    }

    if (mode == Mode::NoBold) {
        for (Line &l : body) {
            bool anyB = false;
            for (const Seg &x : std::as_const(l.s)) {
                anyB = anyB || x.b;
            }
            if (!l.isHdr && !anyB) {
                for (Seg &x : l.s) {
                    x.ly = QStringLiteral("lyrics");
                }
            }
        }
    }

    // Title block.
    QString title, label, artist;
    QStringList extraHeadLines;     // head text that isn't title/label/artist, kept verbatim
    bool anyK = false;
    for (const Line &l : head) {
        for (const Seg &x : l.s) {
            if (!x.k.isEmpty() && x.k != "courtesy") {
                anyK = true;
            }
        }
    }
    if (anyK) {
        // Adjacent runs of the same class are one item (Qt may split a run in two);
        //   separate items are joined, e.g. several artists with ", ".
        QHash<QString, QStringList> items;
        for (const Line &l : head) {
            QString plain;
            QString prevK;
            for (const Seg &x : l.s) {
                const QString k = (x.k == "title" || x.k == "label" || x.k == "artist") ? x.k : QString();
                if (k.isEmpty()) {
                    plain += x.t;
                    if (!x.t.trimmed().isEmpty()) {
                        prevK.clear();
                    }
                    continue;
                }
                if (k == prevK) {
                    items[k].last() += x.t;
                } else {
                    items[k] << x.t;
                }
                prevK = k;
            }
            if (!plain.trimmed().isEmpty()) {
                extraHeadLines << collapseSpaces(plain).trimmed();
            }
        }
        auto joined = [&](const QString &k, const QString &sep) {
            QStringList r;
            for (const QString &t : items.value(k)) {
                if (!t.trimmed().isEmpty()) {
                    r << t.trimmed();
                }
            }
            return r.join(sep);
        };
        title = joined("title", " ");
        if (title.isEmpty() && !extraHeadLines.isEmpty()) {
            title = extraHeadLines.takeFirst();     // an unclassed title beside a classed label/artist
        }
        label = joined("label", " ");
        label.remove('(').remove(')');
        artist = joined("artist", ", ");

        // A classed title with an unclassed "(RIV 287) Ken Bower" beside it.
        static const QRegularExpression looseLabel(
            QStringLiteral("^[-\\x{2013}\\s]*\\(?\\s*([A-Z]{2,6}[ -]?\\d{1,5}[A-Za-z]?)\\s*\\)?(?:\\s+|$)"),
            QRegularExpression::CaseInsensitiveOption);
        if (label.isEmpty() && !extraHeadLines.isEmpty()) {
            const QRegularExpressionMatch lm = looseLabel.match(extraHeadLines.first());
            if (lm.hasMatch()) {
                label = lm.captured(1);
                const QString leftover = extraHeadLines.first().mid(lm.capturedEnd()).trimmed();
                extraHeadLines.removeFirst();
                if (artist.isEmpty()) {
                    artist = leftover;
                } else if (!leftover.isEmpty()) {
                    extraHeadLines.prepend(leftover);
                }
            }
        }
    } else {
        QStringList tx;
        for (const Line &l : head) {
            tx << l.txt;
        }
        const QString l0 = tx.value(0);
        static const QRegularExpression parenLabel(
            QStringLiteral("\\(\\s*([A-Z]{2,6}[ -]?\\d{1,5}[A-Za-z]?)\\s*\\)"),
            QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression bareLabel(
            QStringLiteral("^[-\\x{2013}\\s]*([A-Z]{2,6}[ -]?\\d{1,5}[A-Za-z]?)$"),
            QRegularExpression::CaseInsensitiveOption);
        // "E Mau Ke Aloha Pitch:-2 RIV 1097": an upper-case label within the line.
        static const QRegularExpression inlineLabel(
            QStringLiteral("(?<![\\w])([A-Z]{2,6}[ -]?\\d{2,5}[A-Za-z]?)(?![\\w])"));
        QRegularExpressionMatch mm = parenLabel.match(l0);
        if (!mm.hasMatch()) {
            mm = inlineLabel.match(l0);
        }
        const QRegularExpressionMatch bm = bareLabel.match(tx.value(1));
        if (bareLabel.match(l0).hasMatch()) {
            label = bareLabel.match(l0).captured(1);
            title = tx.value(1);
            artist = tx.mid(2).join(' ');
        } else if (mm.hasMatch()) {
            label = mm.captured(1);
            title = l0.left(mm.capturedStart()).trimmed();
            artist = (l0.mid(mm.capturedEnd()) + ' ' + tx.mid(1).join(' ')).trimmed();
        } else if (bm.hasMatch()) {     // title, then "Riv 785" or "- RIV786" on the next line
            title = l0;
            label = bm.captured(1);
            artist = tx.mid(2).join(' ');
        } else {
            title = l0;
            artist = tx.mid(1).join(' ');
        }
    }
    static const QRegularExpression quotes(
        QStringLiteral("^[\"'\\x{201C}\\x{201D}\\x{2018}\\x{2019}\\x{201E}]+|[\"'\\x{201C}\\x{201D}\\x{2018}\\x{2019}\\x{201E}]+$"));
    title = collapseSpaces(title).remove(quotes).trimmed().toUpper();
    static const QRegularExpression letterDigit(QStringLiteral("^([A-Za-z]+)[ -]?(\\d)"));
    label = collapseSpaces(label).trimmed().toUpper().replace(letterDigit, QStringLiteral("\\1 \\2"));
    artist = collapseSpaces(artist).trimmed();
    if (label.isEmpty() && !fileName.isEmpty()) {
        // "EUR 1022 - We Could Be In Love.html", "RWH 1010.html"
        static const QRegularExpression fileLabel(
            QStringLiteral("^\\s*(\\d?[A-Za-z]{1,6})\\s*-?\\s*(\\d{1,5}[A-Za-z]?)(?![\\w])"));   // "4B 6085" too
        const QRegularExpressionMatch fm = fileLabel.match(QFileInfo(fileName).completeBaseName());
        if (fm.hasMatch()) {
            label = fm.captured(1).toUpper() + ' ' + fm.captured(2).toUpper();
        }
    }
    if (title.isEmpty()) {
        res.warnings << QStringLiteral("No title found.");
    }
    if (label.isEmpty()) {
        res.warnings << QStringLiteral("No label number found.");
    }

    // Highlight colours become lyrics, lyrics2, lyrics3, in order of first appearance.
    QHash<QString, QString> lyClass;
    lyClass.insert("lyrics", "lyrics");
    lyClass.insert("lyrics2", "lyrics2");
    lyClass.insert("lyrics3", "lyrics3");
    int nColours = 0;
    QList<Line> coloured = body;
    coloured.prepend(Line());
    coloured.first().s = legend;      // the legend's order, if there is one
    for (const Line &l : std::as_const(coloured)) {
        for (const Seg &x : l.s) {
            if (x.ly.startsWith('#') && !lyClass.contains(x.ly)) {
                nColours++;
                lyClass.insert(x.ly, nColours == 1 ? QStringLiteral("lyrics")
                                                   : QStringLiteral("lyrics%1").arg(qMin(nColours, 3)));
            }
        }
    }

    // A short starting call on the line right after a heading moves up onto the
    //   heading's line ("OPENER" + "Circle Left" -> "OPENER Circle Left"), after any
    //   note there ("OPENER - CLOSER Basic Circle Left"), but not after another call.
    for (int n = 0; n + 1 < body.size(); n++) {
        Line &l = body[n];
        const Line &next = body[n + 1];
        if (!l.isHdr || (!l.rest.isEmpty() && !l.restIsNote) || next.isHdr) {
            continue;
        }
        bool nextHasLyrics = false;
        for (const Seg &x : next.s) {
            nextHasLyrics = nextHasLyrics || !x.ly.isEmpty();
        }
        if (!nextHasLyrics && isMergeableCall(next.txt)) {
            if (!l.rest.isEmpty()) {
                Seg space = next.s.first();
                space.t = QStringLiteral(" ");
                space.keepCase = false;
                l.rest.append(space);
            }
            l.rest += next.s;
            body.removeAt(n + 1);
        }
    }

    if (fixCaseOn) {
        // Are this cuesheet's lyrics written in Title Case or ALL CAPS?
        int nWords = 0, nCap = 0;
        for (const Line &l : std::as_const(body)) {
            const QList<Seg> &segs = l.isHdr ? l.rest : l.s;
            QString run;
            for (const Seg &x : segs) {
                if (!x.ly.isEmpty()) {
                    run += x.t;
                } else if (!run.isEmpty()) {
                    countLyricCaps(run, nWords, nCap);
                    run.clear();
                }
            }
            countLyricCaps(run, nWords, nCap);
        }
        const bool shouty = nCap * 2 > nWords;
        for (Line &l : body) {
            fixCase(l.isHdr ? l.rest : l.s, shouty);
        }
    }

    // Body.
    QString out = QStringLiteral("<SPAN class=\"title\">%1</SPAN>").arg(esc(title));
    if (!label.isEmpty()) {
        out += QStringLiteral(" <SPAN class=\"label\">(%1)</SPAN>").arg(esc(label));
    }
    if (!artist.isEmpty()) {
        out += QStringLiteral(" <SPAN class=\"artist\">%1</SPAN>").arg(esc(artist));
    }
    out += QStringLiteral(" <BR>\n");
    for (const QString &x : std::as_const(extraHeadLines)) {
        out += QStringLiteral("    %1<BR>\n").arg(esc(x));
    }
    if (!legend.isEmpty()) {
        out += QStringLiteral("    ") + renderLine(legend, lyClass) + QStringLiteral("<BR>\n");
    }
    out += QStringLiteral("    <BR>\n");

    int nSections = 0, nLyricLines = 0;
    bool firstH = true;
    for (const Line &l : std::as_const(body)) {
        if (l.isHdr) {
            if (!firstH) {
                out += QStringLiteral("    <BR>\n\n");
            }
            firstH = false;
            nSections++;
            out += QStringLiteral("    <SPAN class=\"hdr\">%1</SPAN>").arg(esc(l.hdr));
            if (!l.rest.isEmpty()) {
                out += ' ' + renderLine(l.rest, lyClass);
                for (const Seg &x : l.rest) {
                    if (!x.ly.isEmpty()) {
                        nLyricLines++;
                        break;
                    }
                }
            }
            out += QStringLiteral("<BR>\n");
        } else {
            bool anyLy = false;
            for (const Seg &x : l.s) {
                anyLy = anyLy || !x.ly.isEmpty();
            }
            if (anyLy) {
                nLyricLines++;
            }
            out += QStringLiteral("    ") + renderLine(l.s, lyClass) + QStringLiteral("<BR>\n");
        }
    }
    if (out.endsWith(QStringLiteral("<BR>\n"))) {
        out.chop(5);
        out += '\n';
    }
    if (first >= 0 && nLyricLines == 0) {
        res.warnings << QStringLiteral("No lyrics found: try another mode, or mark them by hand.");
    }

    const QString titleTag = esc(title) + (label.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(esc(label)));
    res.html = QStringLiteral(
        "<!DOCTYPE html>\n\n<HTML>\n<HEAD> <!-- squaredesk:version = %1 -->\n"
        "    <META charset=\"UTF-8\">\n    <TITLE>%2</TITLE>\n"
        "    <STYLE>\n        %3\n    </STYLE>\n</HEAD>\n\n<BODY>\n%4</BODY>\n</HTML>\n")
        .arg(versionString, titleTag, QString::fromLatin1(kOutputStyle), out);
    res.info = QStringLiteral("%1 sections, %2 lyric lines").arg(nSections).arg(nLyricLines);
    return res;
}

} // namespace CuesheetConverter
