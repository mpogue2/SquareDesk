#ifndef SDINTERFACE_H_INCLUDED
#define SDINTERFACE_H_INCLUDED
#include <functional>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>

class SquareDesk_iofull;
class MainWindow;

const int kSDCallTypeConcepts = (1 << 8);
const int kSDCallTypeCommands = (1 << 9);


class SDThread : public QThread {
    Q_OBJECT
    
public :
    SDThread(MainWindow *mw);
    ~SDThread();
    
public:
    void run() override;

public:
    void on_user_input(QString str);
    void finishAndShutdownSD();
    void unlock();

    enum CurrentInputState {
        InputStateNormal,
        InputStateYesNo,
        InputStateText };
    CurrentInputState currentInputState();
    void set_dance_program(int dance_program);


signals:
    void on_sd_update_status_bar(QString s);
    void on_sd_awaiting_input();
    void on_sd_set_window_title(QString s);
    void on_sd_add_new_line(QString s, int dp);
    void on_sd_set_matcher_options(QStringList, QStringList);
    void on_sd_set_pick_string(QString);
    void on_sd_dispose_of_abbreviation(QString);
    void do_message_box_question(QString title, QString message);
    void do_message_box_get_string(QString title, QString message);

private:
    MainWindow *mw;
    QWaitCondition eventLoopWaitCond;
    QMutex eventLoopMutex;
    bool abort;
    SquareDesk_iofull *iofull;    

};

extern QString sd_strip_leading_selectors(QString originalText);

#endif /* ifndef SDINTERFACE_H_INCLUDED */
