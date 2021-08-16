/****************************************************************************
**
** Copyright (C) 2016-2021 Mike Pogue, Dan Lyke
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

#include "tablelabelitem.h"

TableLabelItem::TableLabelItem(const QString txt)
        :QTableWidgetItem(txt)
{
}

bool TableLabelItem::operator <(const QTableWidgetItem &other) const
{
    QString str1 = text();          // LABEL 12345b
    QString str2 = other.text();

    QString label1, number1, extension1;
    QString label2, number2, extension2;

    // extract pieces of the first string argument ------
    QRegularExpression re("([a-z]*)\\s*(\\d*)\\s*([a-z]*)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match1 = re.match(str1);
    if (match1.hasMatch()) {
        label1 = match1.captured(1).toUpper();       // LABEL
        number1 = match1.captured(2);                // 12345
        extension1 = match1.captured(3).toUpper();   // B
    }
//    qDebug() << "labelItem: " << str1 << ": '" << label1 << "," << number1 << "," << extension1 << "'";

    // extract pieces of the second string argument ------
    QRegularExpressionMatch match2 = re.match(str2);
    if (match2.hasMatch()) {
        label2 = match2.captured(1).toUpper();       // LABEL
        number2 = match2.captured(2);                // 12345
        extension2 = match2.captured(3).toUpper();   // B
    }
//    qDebug() << "labelItem: " << str1 << ": '" << label1 << "," << number1 << "," << extension1 << "'";

    if (label1 == "") {
        label1 = "ZZZZZZZZZ";  // no label sorts AFTER a valid label
    }

    if (label2 == "") {
        label2 = "ZZZZZZZZZ";  // no label sorts AFTER a valid label
    }

    if (number1 == "") {
        number1 = "99999999";  // no number sorts AFTER a valid number
    }

    if (number2 == "") {
        number2 = "99999999";  // no number sorts AFTER a valid number
    }

    // no extension sorts BEFORE a valid extension

    if (label1 == label2) {
        bool ok1 = false;
        int i1 = number1.toInt(&ok1);
        bool ok2 = false;
        int i2 = number2.toInt(&ok2);

        if (i1 == i2) {
            if (extension1 == extension2) {
                return (false);
            } else {
                return (extension1 < extension2);
            }
        } else {
            return (i1 < i2);
        }
    } else {
        return (label1 < label2);
    }

    // return(false);  // should never get here.
//    return str1 < str2;  // FIX FIX FIX
}

