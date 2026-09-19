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
    TableNumberItem(const QString txt = QString("*"));
    bool operator <(const QTableWidgetItem &other) const;
};

// Sorts on a key held separately from the displayed text, for columns whose text can't be
//   compared usefully: star ratings ("***") and dates in the user's locale format, which are
//   neither alphabetical nor parseable as a number (issue #1744).
//
// Rows with no value at all sort to the end either way, the same convention TableNumberItem
//   uses, so unrated tracks don't sit in the middle of the ratings.
class TableSortKeyItem : public QTableWidgetItem
{
public:
    TableSortKeyItem(const QString txt, double sortKey, bool hasValue = true);
    bool operator <(const QTableWidgetItem &other) const;

private:
    double key;
    bool   valued;
};


#endif // TABLENUMBERITEM_H
