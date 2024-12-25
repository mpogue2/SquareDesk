#ifndef AUDITIONBUTTON_H
#define AUDITIONBUTTON_H

#include <QObject>
#include <QWidget>
#include <QPushButton>

class auditionButton : public QPushButton
{
    Q_OBJECT

public:
    explicit auditionButton(QWidget *parent = nullptr);
    QString origPath;

signals:
};

#endif // AUDITIONBUTTON_H
