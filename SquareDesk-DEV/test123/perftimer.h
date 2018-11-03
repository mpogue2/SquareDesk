#ifndef PERFTIMER_H_INCLUDED
#define PERFTIMER_H_INCLUDED
#include <QElapsedTimer>

class PerfTimer {
private:
    const char *name;
    QElapsedTimer timer;
    bool stopped;
    qint64 lastElapsedTime_ms;
    static int indentLevel;
public:
    PerfTimer(const char *name, int lineNumber);
    virtual ~PerfTimer();
    void start(int lineNumber);
    void elapsed(int lineNumber);
    void stop();
};

#endif /* ifndef PERFTIMER_H_INCLUDED */
