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

#include "labelparser.h"

#include <QRegularExpression>

// The dashed label names, and the dash-free stand-in each one hides behind.  The stand-ins are
//   deliberately all-caps and wordlike, so they sail through the label regexes as an ordinary
//   label would.
namespace {
struct DashedLabel {
    const char *pattern;
    const char *standIn;
};

const DashedLabel dashedLabels[] = {
    { "(4-[Bb][Aa][Rr]-[Bb])",                        "FOURBARB"  },  // 4-Bar-B
    { "([Cc][Ii][Rr][Cc][Ll][Ee]-[Dd])",              "CIRCLED"   },  // Circle-D
    { "([Jj][Aa][Yy]-[Bb][Aa][Rr]-[Kk][Aa][Yy])",     "JAYBARKAY" },  // Jay-Bar-Kay
};
const int numDashedLabels = sizeof(dashedLabels)/sizeof(dashedLabels[0]);
}

QString DashedLabelMask::apply(const QString &s)
{
    // Built once and reused; the patterns are compile-time constants.
    static QRegularExpression regexes[numDashedLabels] = {
        QRegularExpression(dashedLabels[0].pattern),
        QRegularExpression(dashedLabels[1].pattern),
        QRegularExpression(dashedLabels[2].pattern),
    };

    QString masked = s;
    for (int i = 0; i < numDashedLabels; ++i) {
        QRegularExpressionMatch match;
        if (masked.contains(regexes[i], &match)) {
            original[i] = match.captured(1);
            masked.replace(original[i], dashedLabels[i].standIn);
        }
    }
    return masked;
}

QString DashedLabelMask::unmask(const QString &s) const
{
    QString out = s;
    for (int i = 0; i < numDashedLabels; ++i) {
        if (!original[i].isEmpty()) {
            out.replace(dashedLabels[i].standIn, original[i]);  // original capitalization
        }
    }
    return out;
}

bool parseLabelFieldIntoParts(const QString &s,
                              QString &label, QString &labelnum, QString &labelnum_extra)
{
    label = "";
    labelnum = "";
    labelnum_extra = "";

    const QString trimmed = s.simplified();
    if (trimmed.isEmpty()) {
        return false;
    }

    DashedLabelMask mask;
    const QString masked = mask.apply(trimmed);

    // Non-greedy name, then an optional space/dash separator, then the number, then up to three
    //   trailing letters ("117b", "2534a", "4936B").  Anchored at both ends: this field holds a
    //   label and nothing else, so a partial match would be a misread rather than a find.
    static QRegularExpression regexLabelAndNumber(
        "^(.*?)[\\s\\-]*(\\d{1,5})([A-Za-z]{0,3})$");

    QRegularExpressionMatch match = regexLabelAndNumber.match(masked);
    if (match.hasMatch()) {
        const QString namePart = mask.unmask(match.captured(1)).simplified();

        // A label with no letter in it is not a label -- same reasoning as breakFilenameIntoParts().
        //   Here it means the field held a bare number, so treat the whole thing as the label
        //   rather than inventing a nameless one.
        static QRegularExpression regexHasALetter("[A-Za-z]");
        if (namePart.contains(regexHasALetter)) {
            label          = namePart;
            labelnum       = match.captured(2);
            labelnum_extra = match.captured(3);
            return true;
        }
    }

    // No number in there (or no name in front of it).  The user pointed us at this field and
    //   said it holds the label, so believe them and show it as-is.
    label = mask.unmask(masked).simplified();
    return false;
}
