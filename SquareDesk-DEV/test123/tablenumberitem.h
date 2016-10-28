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

//        qDebug() << "str1: " << str1 << ", str2: " << str2;
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

//        qDebug() << "result: " << f1 << f2 << ok1 << ok2 << (f1 < f2);

        return f1 < f2;
    }
};


#endif // TABLENUMBERITEM_H
