#ifndef SQUAREDANCERSCENE_H_INCLUDED
#define SQUAREDANCERSCENE_H_INCLUDED

#include <QGraphicsView>
class MainWindow;

class SquareDancerScene : public QGraphicsView {
public:
SquareDancerScene(QWidget *parent) : QGraphicsView(parent) {}
    void resizeEvent(QResizeEvent *event);
};

#endif /* ifndef SQUAREDANCERSCENE_H_INCLUDED */
