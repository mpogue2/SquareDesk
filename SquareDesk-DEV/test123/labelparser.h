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

// Turning text into a record label + label number (issue #1747).
//
// Two callers, deliberately sharing the dash handling:
//   - breakFilenameIntoParts(), which parses a whole FILENAME ("RIV 307 - Going to Ceili")
//   - parseLabelFieldIntoParts(), which parses a metadata field holding the LABEL ALONE
//       ("RYL-220"), for a user who keeps it in Album (or Grouping, or ...) rather than in the
//       filename.  Preferences > Apple Music > "Read Label from" chooses that field.
//
// The two can't share a regex -- one expects a title after the label and the other doesn't --
//   but they MUST share the dash handling, which is the subtle part.  See DashedLabelMask.

#ifndef LABELPARSER_H
#define LABELPARSER_H

#include <QString>

// Some real record labels contain dashes: "4-Bar-B", "Circle-D", "Jay-Bar-Kay".  Every label
//   regex here treats a dash as a separator, so those names have to be hidden behind a
//   dash-free stand-in for the duration of the match and put back afterwards.
//
// Usage is always apply() ... match ... unmask(), on the same object:
//     DashedLabelMask mask;
//     QString masked = mask.apply(input);
//     // ...match against masked, capture a label...
//     label = mask.unmask(label);       // restores the original spelling AND capitalization
class DashedLabelMask
{
public:
    // Returns s with any dashed label names replaced by their stand-ins.
    QString apply(const QString &s);

    // Returns s with the stand-ins turned back into what they replaced.  Safe to call on a
    //   string that contains none of them, and on a mask that matched nothing.
    QString unmask(const QString &s) const;

private:
    // What each stand-in replaced, exactly as it was spelled in the input ("4-Bar-B" vs
    //   "4-BAR-B"), so unmasking restores the user's capitalization rather than a canonical one.
    QString original[3];
};

// Parses a metadata field that holds a label and (usually) a number, with no title attached:
//   "RYL-220"         -> label "RYL",       labelnum "220"
//   "HH 1234"         -> label "HH",        labelnum "1234"
//   "Chaparral 117b"  -> label "Chaparral", labelnum "117",  labelnum_extra "b"
//   "4-Bar-B 6039"    -> label "4-Bar-B",   labelnum "6039"
//   "Hoedown"         -> label "Hoedown",   labelnum ""       (no number in there at all)
//
// The dash between label and number is normalized away, so "RYL-220" and "RYL 220" both end up
//   spelled the same in the Label column -- but a dash INSIDE a label name survives.
//
// Returns true when a label number was found.  When it returns false the whole (trimmed) value
//   is handed back as the label, which is the honest answer for a field the user explicitly
//   pointed us at: they said this field holds the label, so show it rather than discarding it.
bool parseLabelFieldIntoParts(const QString &s,
                              QString &label, QString &labelnum, QString &labelnum_extra);

#endif // LABELPARSER_H
