#ifndef SONGTITLELABEL_H_INCLUDED
#define SONGTITLELABEL_H_INCLUDED

#include <QLabel>
class MainWindow;

class SongTitleLabel : public QLabel {
private:
    MainWindow *mw;
public:
SongTitleLabel(MainWindow *mw) : QLabel(), mw(mw) {}
    void mouseDoubleClickEvent(QMouseEvent *) override;
};

#endif /* ifndef SONGTITLELABEL_H_INCLUDED */
