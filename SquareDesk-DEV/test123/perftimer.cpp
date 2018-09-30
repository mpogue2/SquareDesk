#include "perftimer.h"
#include <QDebug>

PerfTimer::PerfTimer(const char *name)
//    : name(name), timer(), stopped(false)
    : timer(), stopped(false)
{
    Q_UNUSED(name)
    start();
}

void PerfTimer::start()
{
    timer.start();
    stopped = false;
}

void PerfTimer::stop()
{
    if (!stopped)
    {
//        qint64 elapsed = timer.elapsed();
//        qDebug() << "Timer " << name << " took " << elapsed;  // uncomment when in use
    }
}

PerfTimer::~PerfTimer()
{
    stop();
}

