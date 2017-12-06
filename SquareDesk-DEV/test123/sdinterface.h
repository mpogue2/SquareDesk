#ifndef SDINTERFACE_H_INCLUDED
#define SDINTERFACE_H_INCLUDED
#include <functional>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>

// ZZZZ TODO: I realy don't want to clutter up this with sdlib
// declarations, but I also don't want to duplicate `enum dance_level`
#include "../sdlib/database.h"

class SDAvailableCall {
public :
    dance_level dance_program;
    QString call_name;
SDAvailableCall() : dance_program(l_mainstream), call_name() {}
};

class QListWidget;

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
    void finishAndShutdownSD();
    void unlock();

    enum CurrentInputState {
        InputStateNormal,
        InputStateYesNo,
        InputStateText };
    CurrentInputState currentInputState();
    void set_dance_program(dance_level dance_program);
    dance_level find_dance_program(QString call);
    void do_user_input(QString str);
    void add_selectors_to_list_widget(QListWidget *);
    void add_directions_to_list_widget(QListWidget *listWidget);

private:
    bool on_user_input(QString str);

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
    void sd_begin_available_call_list_output();
    void sd_end_available_call_list_output();

private:
    MainWindow *mw;
    QWaitCondition waitCondSDAwaitingInput;
    QMutex mutexSDAwaitingInput;

    QWaitCondition waitCondAckToMainThread;
    QMutex mutexAckToMainThread;
    
    bool abort;
    SquareDesk_iofull *iofull;    

};

extern QString sd_strip_leading_selectors(QString originalText);

#endif /* ifndef SDINTERFACE_H_INCLUDED */
