/****************************************************************************
o**
** Copyright (C) 2016-2022 Mike Pogue, Dan Lyke
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

#ifndef SDFORMATION_UTILS_INCLUDED
#define SDFORMATION_UTILS_INCLUDED

#include "common_enums.h"

struct dancer { int coupleNum, gender, x, y, topside, leftside; };

void getDancerOrder(struct dancer dancers[], Order *boyOrder, Order *girlOrder);
Order whichOrder(double p1_x, double p1_y, double p2_x, double p2_y);


#endif // ifndef SDFORMATION_UTILS_INCLUDED
