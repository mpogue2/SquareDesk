#ifndef PERFTIMER_H_INCLUDED
#define PERFTIMER_H_INCLUDED
#include <QElapsedTimer>

class PerfTimer {
private:
//    const char *name;
    QElapsedTimer timer;
    bool stopped;
public:
    PerfTimer(const char *name);
    virtual ~PerfTimer();
    void start();
    void stop();
};

#endif /* ifndef PERFTIMER_H_INCLUDED */
