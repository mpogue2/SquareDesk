/****************************************************************************
**
** Copyright (C) 2016, 2017 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk/SquareDeskPlayer application.
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

#ifndef TABLENUMBERITEM_H
#define TABLENUMBERITEM_H

#include <QObject>
#include <QString>
#include <QTableWidgetItem>
#include <QDebug>

// http://stackoverflow.com/questions/7848683/how-to-sort-datas-in-qtablewidget
class TableNumberItem : public QTableWidgetItem
{
public:
    TableNumberItem(const QString txt = QString("*"))
        :QTableWidgetItem(txt)
    {
    }

    bool operator <(const QTableWidgetItem &other) const
    {
        QString str1 = text();
        QString str2 = other.text();

        // tempos can have percent signs.  For comparisons, just remove them, and prepend a big number.
        //   sort order is then 0, 100, 123, ... 137, 0%, 80%, 100%.
        if (str1.contains('%')) {
            str1.replace("%","");
            str1 = "100" + str1;
        }
        if (str2.contains('%')) {
            str2.replace("%","");
            str2 = "100" + str2;
        }

        if (str1 == " " || str1 == "") {
            str1 = "9999999.9";
        }

        if (str2 == " " || str2 == "") {
            str2 = "9999999.9";
        }

//        if (str1[0] == '$' || str1[0] == '€') {
//            str1.remove(0, 1);
//            str2.remove(0, 1); // we assume both items have the same format
//        }

//        if (str1[str1.length() - 1] == '%') {
//            str1.chop(1);
//            str2.chop(1); // this works for "N%" and for "N %" formatted strings
//        }

        bool ok1 = false;
        double f1 = str1.toDouble(&ok1);

        bool ok2 = false;
        double f2 = str2.toDouble(&ok2);

//        qDebug() << "foo: " << str1 << str2 << (f1 < f2);

        return f1 < f2;
    }
};


#endif // TABLENUMBERITEM_H
