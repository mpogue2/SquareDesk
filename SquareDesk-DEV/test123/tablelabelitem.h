/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
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

#ifndef TABLELABELITEM_H
#define TABLELABELITEM_H

#include <QObject>
#include <QString>
#include <QTableWidgetItem>
#include <QDebug>

// Label strings need special sorting.  They typically look like this:
//  LABEL 12345a
// We need to first sort by LABEL, then by 12345, and finally by a

// http://stackoverflow.com/questions/7848683/how-to-sort-datas-in-qtablewidget
class TableLabelItem : public QTableWidgetItem
{
public:
    TableLabelItem(const QString txt = QString("*"));
    bool operator <(const QTableWidgetItem &other) const;
};

#endif // TABLELABELITEM_H
