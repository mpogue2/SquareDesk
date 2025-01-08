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

#ifndef TYPETRACKER_H
#define TYPETRACKER_H

#include <QList>

#define NONE 0
#define PATTER 1
#define SINGING 2
#define SINGING_CALLED 3
#define XTRAS 4

typedef struct TimeSegments
{
    int type;
    int length;
} TimeSegment;

class TypeTracker
{
public:
    TypeTracker();

    void addSecond(int Type);
    void printState(QString s);
    QList<TimeSegment> timeSegmentList;  // public, so we can iterate

    int currentPatterLength(void);      // returns current patter length, or -1 if not in patter-or-potentially-in-patter
    int currentBreakLength(void);       // returns current break length, or -1 if not in break (for sure not potentially-in-patter)
    void addStop(void);                 // adds a NONE:0, which prevents consolidation.

    int consolidateNSecondsOfNone;   // when consolidating Patter-None-Patter segments, this many seconds of None becomes a BREAK

private:

};

#endif // TYPETRACKER_H
