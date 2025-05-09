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

#ifndef UTILITY_H_INCLUDED
#define UTILITY_H_INCLUDED

#include <QString>
#include <math.h>

#include <QApplication>
#include <QWidget>
#include <QStyle>

#if QT_VERSION >= 0x051400
#define KEEP_EMPTY_PARTS Qt::KeepEmptyParts
#define SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#else
#define KEEP_EMPTY_PARTS QString::KeepEmptyParts
#define SKIP_EMPTY_PARTS QString::SkipEmptyParts
#endif

#define ENDL Qt::endl

class RecursionGuard
{
private:
    bool &guard_value;
public:
    RecursionGuard(bool &guard_value) :
        guard_value(guard_value)
    {
        guard_value = true;
    }
    ~RecursionGuard()
    {
        guard_value = false;
    }
};

QString doubleToTime(double t);
double timeToDouble(QString t);

// HELPER FUNCTIONS for setting properties on all widgets
//   the qss can then use darkmode and !darkmode
void setDynamicPropertyRecursive(QWidget* widget, const QString& propertyName, const QVariant& value);
void setDynamicPropertyOnAllWidgets(const QString& propertyName, const QVariant& value);

#endif // ifndef UTILITY_H_INCLUDED

