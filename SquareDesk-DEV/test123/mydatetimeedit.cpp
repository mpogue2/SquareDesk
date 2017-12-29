#include <QDebug>
#include "mydatetimeedit.h"

myDateTimeEdit::myDateTimeEdit(QWidget *parent) : QDateTimeEdit(parent)
{
}

myDateTimeEdit::myDateTimeEdit(QDate &date, QWidget *parent) : QDateTimeEdit(date, parent)
{
}

// Both up and down are enabled
QDateTimeEdit::StepEnabled myDateTimeEdit::stepEnabled() const
{
    return QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
}

// This allows rollover of MM, SS, and SSS on QDateTimeEdits.
//   NOPE: we don't do hours.
//
void myDateTimeEdit::stepBy(int steps)
{
    QTime currentTime(QDateTimeEdit::time());
    qDebug() << "currentTime: " << currentTime;

    QDateTimeEdit::Section temp_section = currentSection();
    if (temp_section == QDateTimeEdit::MinuteSection) {
        qDebug() << "current section is MINUTES";
    } else if (temp_section == QDateTimeEdit::SecondSection) {
        qDebug() << "current section is SECONDS";
        QTime next_time(currentTime.addSecs(steps));
        if (next_time.minute() != currentTime.minute()) {
            QDateTimeEdit::setTime(next_time);
            steps = 0;
        }
    } else if (temp_section == QDateTimeEdit::MSecSection) {
        qDebug() << "current section is MILLISECONDS";
        QTime next_time(currentTime.addMSecs(steps));
        if (next_time.second() != currentTime.second() ||
                next_time.minute() != currentTime.minute()) {
            QDateTimeEdit::setTime(next_time);
            steps = 0;
        }
    }
    QDateTimeEdit::stepBy(steps);
}
