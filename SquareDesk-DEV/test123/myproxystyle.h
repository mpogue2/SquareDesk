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

#ifndef MYPROXYSTYLE_H
#define MYPROXYSTYLE_H

#include <QProxyStyle>
#include <QStyleOption>
#include <QWidget>
#include <QStyleHintReturn>

// http://stackoverflow.com/questions/24719739/how-to-use-qstylesh-tooltip-wakeupdelay-to-set-tooltip-wake-up-time
class MyProxyStyle : public QProxyStyle
{
    Q_OBJECT
public:
    int styleHint(StyleHint hint,
                  const QStyleOption *option,
                  const QWidget *widget,
                  QStyleHintReturn *returnData) const Q_DECL_OVERRIDE
    {
        if (hint == QStyle::SH_ToolTip_WakeUpDelay) {
            return 2000;    // 2 seconds for tooltips
        }

        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

#endif // MYPROXYSTYLE_H