/****************************************************************************
**
** Copyright (C) 2016-2023 Mike Pogue, Dan Lyke
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

#ifndef SESSIONINFO_H_INCLUDED
#define SESSIONINFO_H_INCLUDED

class SessionInfo {
public:
    QString name;
    int day_of_week; /* 1 = Monday, 7 = Sunday, to work with QDate */
    int id;
    int order_number;
    int start_minutes;

SessionInfo() : name(), day_of_week(0), id(-1), order_number(0), start_minutes(-1) {}
    
};


#endif /* ifndef SESSIONINFO_H_INCLUDED */
