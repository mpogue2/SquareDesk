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

#include "utility.h"
#include <QStringList>

QString doubleToTime(double t)
{
    double minutes = floor(t / 60);
    double seconds = t - minutes * 60;
    QString str = QString("%1:%2").arg(minutes).arg(seconds, 6, 'f', 3, '0');
    return str;
}


double timeToDouble(QString t)
{
    QStringList l = t.split(':');
    double d = 0;
    for (int i = 0; i < l.count(); ++i)
    {
        d *= 60;
        d += l[i].toDouble();
    }
    return d;
}

// HELPER FUNCTIONS for setting properties on all widgets
//   the qss can then use darkmode and !darkmode
void setDynamicPropertyRecursive(QWidget* widget, const QString& propertyName, const QVariant& value) {
    if (widget) {
        widget->setProperty(propertyName.toStdString().c_str(), value);
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        const QList<QObject*> children = widget->children();
        for (QObject* child : children) {
            if (QWidget* childWidget = qobject_cast<QWidget*>(child)) {
                setDynamicPropertyRecursive(childWidget, propertyName, value);
            }
        }
    }
}

void setDynamicPropertyOnAllWidgets(const QString& propertyName, const QVariant& value) {
    const QList<QWidget*> topLevelWidgets = QApplication::topLevelWidgets();
    for (QWidget* widget : topLevelWidgets) {
        setDynamicPropertyRecursive(widget, propertyName, value);
    }
}
