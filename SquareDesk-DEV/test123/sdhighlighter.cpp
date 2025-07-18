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

#include "sdhighlighter.h"
#include <utility>
#include <QDebug>

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

//     4B>   3GV   3BV   2G<
//
//     4G>   1B^   1G^   2B<

    // couple #1 rule
    couple1Format.setForeground(COUPLE1COLOR);
    couple1Format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("1[BG][<>^V]");
    rule.format = couple1Format;
    highlightingRules.append(rule);

    // couple #2 rule
    couple2Format.setForeground(COUPLE2COLOR);
    couple2Format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("2[BG][<>^V]");
    rule.format = couple2Format;
    highlightingRules.append(rule);

    // couple #3 rule
    couple3Format.setForeground(COUPLE3COLOR);
    couple3Format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("3[BG][<>^V]");
    rule.format = couple3Format;
    highlightingRules.append(rule);

    // couple #4 rule
    couple4Format.setForeground(COUPLE4COLOR_DARK);
    couple4Format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("4[BG][<>^V]");
    rule.format = couple4Format;
    highlightingRules.append(rule);

    // prompt highlight
    QTextCharFormat promptFormat;
    promptFormat.setForeground(Qt::gray);
//    couple4Format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("(.*)-->");
    rule.format = promptFormat;
    highlightingRules.append(rule);

}

void Highlighter::highlightBlock(const QString &text)
{
    for (const auto &rule : std::as_const(highlightingRules)) {
        QRegularExpression expression(rule.pattern);
//        int index = expression.indexIn(text);
//        while (index >= 0) {
//            int length = expression.matchedLength();
//            setFormat(index, length, rule.format);
//            index = expression.indexIn(text, index + length);
//        }
        // Reference: https://stackoverflow.com/questions/22489723/qt-c-qregularexpression-multiple-matches
//        qDebug() << "highlightBlock START: " << text;
        QRegularExpressionMatchIterator i = expression.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            if (match.hasMatch()) {
                setFormat(match.capturedStart(0), match.capturedLength(0), rule.format);
            }
        }
//        qDebug() << "highlightBlock END: " << text;

    }
}
