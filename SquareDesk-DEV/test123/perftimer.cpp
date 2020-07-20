#include "perftimer.h"
#include <QDebug>

// comment this out, if you don't want timing info
//#define ENABLEPERFTIMER 1

int PerfTimer::indentLevel = 0;  // static

PerfTimer::PerfTimer(const char *name, int lineNumber)
    : name(name), timer(), stopped(false), lastElapsedTime_ms(0)
{
#ifdef ENABLEPERFTIMER
    start(lineNumber);
#else
    Q_UNUSED(lineNumber)
#endif
}

void PerfTimer::start(int lineNumber)
{
#ifdef ENABLEPERFTIMER
    timer.start();
    stopped = false;
    indentLevel++;  // increase indent level
    qDebug().noquote() << QString("  ").repeated(indentLevel) <<
                          "*** Timer " << name << "[line" << lineNumber << "], started";
#else
    Q_UNUSED(lineNumber)
    Q_UNUSED(name)  // gets rid of the warning around "name"
#endif
}

void PerfTimer::elapsed(int lineNumber)
{
#ifdef ENABLEPERFTIMER
    qint64 elapsed_ms = timer.elapsed();
    qDebug().noquote() << QString("  ").repeated(indentLevel) <<
                          "*** Timer " << name << "[line" << lineNumber << "], elapsed:" << elapsed_ms <<
                          "ms (delta_ms:" << elapsed_ms - lastElapsedTime_ms << ")";
    // no change to indentLevel...
    lastElapsedTime_ms = elapsed_ms;  // save last elapsed time
#else
    Q_UNUSED(lineNumber)
    Q_UNUSED(lastElapsedTime_ms)
#endif
}

void PerfTimer::stop()
{
#ifdef ENABLEPERFTIMER
    if (!stopped)
    {
        stopped = true;  // note that QElapsedTimers do not need to be manually stopped
        qint64 elapsed = timer.elapsed();
        qDebug().noquote() << QString("  ").repeated(indentLevel) <<
                              "*** Timer " << name << " stopped:" << elapsed << "ms";
        indentLevel--;
    }
#else
    Q_UNUSED(stopped)
#endif
}

PerfTimer::~PerfTimer()
{
#ifdef ENABLEPERFTIMER
    stop();
#endif
}

