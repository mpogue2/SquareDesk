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
