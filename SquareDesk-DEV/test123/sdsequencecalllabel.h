#ifndef SDSEQUENCECALLLABEL_H_INCLUDED
#define SDSEQUENCECALLLABEL_H_INCLUDED

#include <QLabel>
class MainWindow;

class SDSequenceCallLabel : public QLabel {
private:
    MainWindow *mw;
public:
SDSequenceCallLabel(MainWindow *mw) : QLabel(), mw(mw) {}
    void mouseDoubleClickEvent(QMouseEvent *) override;
};

#endif /* ifndef SDSEQUENCECALLLABEL_H_INCLUDED */
