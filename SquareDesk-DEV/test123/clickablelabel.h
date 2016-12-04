#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>

class clickableLabel : public QLabel
{
    Q_OBJECT
public:
    clickableLabel(QWidget* parent);

    explicit clickableLabel( const QString &text="", QWidget *parent=0 );
    ~clickableLabel();
signals:
    void clicked();
protected:
    void mousePressEvent(QMouseEvent *event);
};

#endif // CLICKABLELABEL_H
