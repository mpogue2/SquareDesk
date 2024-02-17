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

#ifndef DANCEPROGRAMS_H_INCLUDED
#define DANCEPROGRAMS_H_INCLUDED

extern const char *danceprogram_basic1[];
extern const char *danceprogram_basic2[];
extern const char *danceprogram_SSD[];
extern const char *danceprogram_mainstream[];
extern const char *danceprogram_plus[];
extern const char *danceprogram_a1[];
extern const char *danceprogram_a2[];

struct DanceProgramCallInfo {
    const char * program;
    const char * name;
    const char * timing;
};
extern const struct DanceProgramCallInfo danceprogram_callinfo[];
#endif // ifndef DANCEPROGRAMS_H_INCLUDED

