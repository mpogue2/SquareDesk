/****************************************************************************
**
** Copyright (C) 2016-2026 Mike Pogue, Dan Lyke
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

#include "tablenumberitem.h"

TableNumberItem::TableNumberItem(const QString txt)
        :QTableWidgetItem(txt)
{
}

bool TableNumberItem::operator <(const QTableWidgetItem &other) const
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

    bool ok1 = false;
    double f1 = str1.toDouble(&ok1);

    bool ok2 = false;
    double f2 = str2.toDouble(&ok2);

    return f1 < f2;
}


// -------------------------------------------------------------------
TableSortKeyItem::TableSortKeyItem(const QString txt, double sortKey, bool hasValue)
        :QTableWidgetItem(txt), key(sortKey), valued(hasValue)
{
}

bool TableSortKeyItem::operator <(const QTableWidgetItem &other) const
{
    const TableSortKeyItem *o = dynamic_cast<const TableSortKeyItem *>(&other);
    if (o == nullptr) {
        return QTableWidgetItem::operator<(other);  // mixed column, fall back to text
    }

    // Valueless rows go last, regardless of which direction we're sorting.  Comparing them
    //   against each other has to be false both ways round, or the sort is not a strict weak
    //   ordering and std::sort is free to misbehave.
    if (!valued || !o->valued) {
        return valued && !o->valued;
    }

    return key < o->key;
}
