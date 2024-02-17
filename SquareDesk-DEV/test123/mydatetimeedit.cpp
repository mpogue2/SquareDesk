/****************************************************************************
**
** Copyright (C) 2016-2024 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usage
** For commercial licensing terms and conditions, contact the authors via the
** email address above.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appear in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.
**
** $SQUAREDESK_END_LICENSE$
**
****************************************************************************/

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
//    qDebug() << "currentTime: " << currentTime;

    QDateTimeEdit::Section temp_section = currentSection();
    if (temp_section == QDateTimeEdit::MinuteSection) {
//        qDebug() << "current section is MINUTES";
    } else if (temp_section == QDateTimeEdit::SecondSection) {
//        qDebug() << "current section is SECONDS";
        QTime next_time(currentTime.addSecs(steps));
        if (next_time.minute() != currentTime.minute()) {
            QDateTimeEdit::setTime(next_time);
            steps = 0;
        }
    } else if (temp_section == QDateTimeEdit::MSecSection) {
//        qDebug() << "current section is MILLISECONDS";
        QTime next_time(currentTime.addMSecs(steps));
        if (next_time.second() != currentTime.second() ||
                next_time.minute() != currentTime.minute()) {
            QDateTimeEdit::setTime(next_time);
            steps = 0;
        }
    }
    QDateTimeEdit::stepBy(steps);
}
