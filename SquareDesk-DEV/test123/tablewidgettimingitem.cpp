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

#include "tablewidgettimingitem.h"
#include <ctype.h>

TableWidgetTimingItem::TableWidgetTimingItem(int type)
    : QTableWidgetItem(type)
{
}

TableWidgetTimingItem::TableWidgetTimingItem(const QString & text, int type)
    : QTableWidgetItem(text,type)
{
}

TableWidgetTimingItem::TableWidgetTimingItem(const QIcon & icon, const QString & text, int type)
    : QTableWidgetItem(icon, text, type)
{
}

TableWidgetTimingItem::TableWidgetTimingItem(const QTableWidgetItem & other)
    : QTableWidgetItem(other)
{
}

TableWidgetTimingItem::~TableWidgetTimingItem()
{
}

int extractFirstNumberOrZero(const QString &s)
{
    int value = 0;
    int sign = 1;
    
    for (int i = 0; i < s.length(); ++i)
    {
        int uc = s[i].unicode();
        
        if (uc >= '0' && uc <= '9')
        {
            value *= 10;
            value += uc - '0';
        }
        else if (value)
        {
            return sign * value;
        }
        else if (uc == '-')
        {
            sign = -1;
        }
        else
        {
            sign = 1;
        }
    }
    return sign * value;
}

bool TableWidgetTimingItem::operator< ( const QTableWidgetItem& rhs ) const
{
    return (extractFirstNumberOrZero(text()) < extractFirstNumberOrZero(rhs.text()));
}
