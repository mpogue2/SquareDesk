#ifndef MYDATETIMEEDIT_H
#define MYDATETIMEEDIT_H

#include <QDateTimeEdit>
#include <QObject>

class myDateTimeEdit : public QDateTimeEdit
{
    Q_OBJECT

public:
    myDateTimeEdit(QWidget *parent = 0);
    myDateTimeEdit(QDate &date, QWidget *parent = 0);
    QDateTimeEdit::StepEnabled stepEnabled() const;
    void stepBy(int steps);
};

#endif // MYDATETIMEEDIT_H
