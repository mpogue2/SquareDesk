#ifndef SDINTERFACE_H_INCLUDED
#define SDINTERFACE_H_INCLUDED
#include <functional>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>

class SquareDesk_iofull;
class MainWindow;

class SDThread : public QThread {
    Q_OBJECT
    
public :
    SDThread(MainWindow *mw);
    ~SDThread();
    
public:
    void run() override;

public:
    void on_user_input(QString str);
    void stop();

signals:
    void on_sd_update_status_bar(QString s);
    void on_sd_awaiting_input();
    void on_sd_set_window_title(QString s);
    void on_sd_add_new_line(QString s, int dp);
    void on_sd_set_matcher_options(QStringList);
    void on_sd_set_pick_string(QString);
    void on_sd_dispose_of_abbreviation(QString);

private:
    MainWindow *mw;
    QWaitCondition eventLoopWaitCond;
    QMutex mutex;
    bool abort;
    SquareDesk_iofull *iofull;    

};

#endif /* ifndef SDINTERFACE_H_INCLUDED */
