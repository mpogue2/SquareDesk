/****************************************************************************
**
** Copyright (C) 2016-2020 Mike Pogue, Dan Lyke
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

#include "typetracker.h"
#include <QDebug>

TypeTracker::TypeTracker()
{
    // You can stop the Patter for up to N seconds (right now, 60s), without auto-going to break.
    // So, if you want to give direction to dancers without going to break, make sure it's
    //   less than N=60 seconds.  In general, it's better to turn down the volume with your hand-wheel,
    //   than to stop the music entirely.
    //   >60 seconds of not playing patter means we're in BREAK (and the patter timer is auto-reset)
    //   singing calls take us out of patter timer mode immediately, and when singing calls stop,
    //     the auto-break timer auto-starts.  (Might want to wait 15 second before doing so for singing calls, too).
    //   NOTE: this 60s is not lost, it's subtracted from the initial break timer when the break timer starts.
    // Right now, the break timer is a count-down timer, while the patter timer is a count-up timer.
    //
    // changed to 60s at suggestion of David Dvorak (I think he's right)
    // TODO: make a Preference for this value
    consolidateNSecondsOfNone = 60;

    TimeSegment t;
    t.type = NONE;
    t.length = 3600;  // track the last hour worth of seconds (max)
    timeSegmentList.append(t);

//    printState("TypeTracker():");
}

// adds one second of "type" (e.g. PATTER, NONE), expiring the old stuff
void TypeTracker::addSecond(int Type)
{
    // get rid of *all* STOP segments (NONE:0), when they reach the bottom
    while (timeSegmentList.length() >= 1 && timeSegmentList.last().length == 0) {
        timeSegmentList.removeLast();
    }

    // add one at the front of the deque
    if (timeSegmentList.first().type == Type) {
        // extend the current segment by one second
        timeSegmentList.first().length += 1;
    } else {
        // make a new segment
        TimeSegment t;
        t.type = Type;
        t.length = 1;
        timeSegmentList.push_front(t);
    }

    // always expire (take one second away) from the timeSegment at the end of the deque
    timeSegmentList.last().length -= 1;
    if (timeSegmentList.last().length <= 0) {
        // segment had zero seconds left, so remove it entirely
        timeSegmentList.removeLast();
    }

    // Now, consolidate segments.  Short segments of NONE (<consolidateNSecondsOfNone) that are in between
    //   two segments of the same non-NONE Type are consolidated into a single segment (3 -> 1).
    // This only has to be done for the first three entries in the list, because nothing
    //   can escape the consolidator to get beyond that.

    if (timeSegmentList.length() >= 3) {
        // consolidation might be possible
        if (timeSegmentList.at(0).type == timeSegmentList.at(2).type &&
                timeSegmentList.at(0).type != NONE &&
                timeSegmentList.at(1).type == NONE &&
                timeSegmentList.at(1).length > 0 &&
                timeSegmentList.at(1).length < consolidateNSecondsOfNone) {  // FIX FIX FIX: preference?  If over this number, it's a true break, not a pause.
            // this is the ONLY case that can be consolidated.
            // A singing call segment in between patter call segments does NOT count as a patter segment, for example.
            // Passes the test, so consolidate all three:
            timeSegmentList[0].length = timeSegmentList.at(0).length + timeSegmentList.at(1).length + timeSegmentList.at(2).length;
            timeSegmentList.removeAt(1);  // removes the NONE segment
            timeSegmentList.removeAt(1);  // removes the second Type segment
        }
    }

    // get rid of *all* STOP segments (NONE:0), when they reach the bottom (again, in case the last segment was deleted and some STOP segments remain)
    while (timeSegmentList.length() >= 1 && timeSegmentList.last().length == 0) {
//        qDebug() << "removed last...";
        timeSegmentList.removeLast();
    }

    // THIS IS THE ONE YOU WANT TO LOOK AT MOST OF THE TIME.
//    printState("addSecond():");
}

// prints out the current state info
void TypeTracker::printState(QString header)
{
    QString type2String[] = {"None", "Patter", "Singing", "Vocals", "Extras"};

    qDebug() << "\n" << header;
    // add em up (to be safe)
    QList<TimeSegment>::iterator i;
    unsigned int totalSeconds = 0;
    for (i = timeSegmentList.begin(); i != timeSegmentList.end(); ++i) {
        qDebug() << "\ttype = " << type2String[i->type] << ", length =" << i->length;
        totalSeconds += i->length;
    }
    qDebug() << "\tTOTAL = " << totalSeconds;
    qDebug() << "\tpatterLength = " << currentPatterLength();
    qDebug() << "\tbreakLength = " << currentBreakLength();
}

// returns number of seconds in the current (maybe-still-possible-to-consolidate) patter segment, or -1 if not in a patter segment
int TypeTracker::currentPatterLength(void) {
    if (timeSegmentList.first().type == NONE &&
            timeSegmentList.first().length < consolidateNSecondsOfNone &&
            timeSegmentList.length() >= 2 &&
            timeSegmentList.at(1).type == PATTER) {
        // special case, where we MIGHT be able to consolidate if Patter starts up again.  Treat this NONE time as still being PATTER time.
        // return the sum of these two times in this case
        return(timeSegmentList.first().length + timeSegmentList.at(1).length);
    }

    if (timeSegmentList.first().type != PATTER) {
        // we're not in a PATTER segment, so return -1
        return(-1);
    }

    // we are in a PATTER segment, and it's not one of those potentially consolidatable ones
    return(timeSegmentList.first().length);
}

// returns number of seconds in the current break (not counting the maybe-still-possible-to-consolidate None segment, or -1 if not in a break segment
int TypeTracker::currentBreakLength(void) {
    if (timeSegmentList.first().type == NONE &&
            timeSegmentList.first().length < consolidateNSecondsOfNone &&
            timeSegmentList.length() >= 2 &&
            timeSegmentList.at(1).type == PATTER) {
        // special case, where we MIGHT be able to consolidate if Patter starts up again.  Treat this NONE time as still being PATTER time.
        // return -1 in this case, because we're NOT in a break (quite yet)
        return(-1);
    }

    if (timeSegmentList.first().type != NONE) {
        // we're not in a BREAK segment, so return -1
        return(-1);
    }

    // we are in a BREAK/NONE segment, and it's not one of those potentially consolidatable patter sections
    return(timeSegmentList.first().length);
}

// pushes a null segment, that is, a segment with type == NONE, and duration 0.
// this does not add time, but it does stop consolidation in it's tracks (thereby turning
//   off the Patter Length Warning.
void TypeTracker::addStop(void) {
    // make a new segment
    TimeSegment t;
    t.type = NONE;
    t.length = 0;
    timeSegmentList.push_front(t);
//    printState("after addStop");
}
