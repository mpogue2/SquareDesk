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
#include <math.h>
#include "sdformationutils.h"
#include <QDebug>

#define BETWEEN(a, b, c) ((b <= a) && (a <= c))


Order whichOrder(double p1_x, double p1_y, double p2_x, double p2_y) {
    double p1_deg = (180.0/M_PI) * atan2(p1_y, p1_x);
    double p2_deg = (180.0/M_PI) * atan2(p2_y, p2_x);

//    qDebug() << "p1/2: " << p1_deg << "," << p2_deg;

    double p12 = p2_deg - p1_deg;  // from boy 1 to boy 2

    if (BETWEEN(p12, -1, 1) || BETWEEN(p12, 179, 181) || BETWEEN(p12, -181, -179) || (p12 >= 359) || p12 <= -359) {
//        qDebug() << "Person order UNKNOWN (colinear)" << p12;
        return(UnknownOrder);
    } else if (BETWEEN(p12, 0, 180) || BETWEEN(p12, -360, -180)) {
//        qDebug() << "Person order IN ORDER" << p12;
        return(InOrder);
    } else {
        // between 180 - 360, it flips
        // similarly, between -180 and -360 it flips
//        qDebug() << "Person order OUT OF ORDER" << p12;
        return(OutOfOrder);
    }
}


void getDancerOrder(struct dancer dancers[], Order *boyOrder, Order *girlOrder) {

    double minx, maxx, miny, maxy;
    minx = miny = 99;
    maxx = maxy = -99;
    for (int i = 0; i < 8; i++) {
        if (!dancers[i].foundInThisRenderingPass) {
            continue;
        }
        minx = fmin(minx, dancers[i].x);
        miny = fmin(miny, dancers[i].y);
        maxx = fmax(maxx, dancers[i].x);
        maxy = fmax(maxy, dancers[i].y);
    }
//    qDebug() << "min x/y = " << minx << miny << ", max x/y = " << maxx << maxy;
    double dividerx = (minx + maxx)/2.0;  // find midpoint dividers of formation
    double dividery = (miny + maxy)/2.0;

    // let's calculate the X and Y positions for each dancer
    double boy1_x = 0, boy1_y = 0, boy2_x = 0, boy2_y = 0;
    double girl1_x = 0, girl1_y = 0, girl2_x = 0, girl2_y = 0;
    for (int i = 0; i < 8; i++) {
        if (!dancers[i].foundInThisRenderingPass) {
            continue;
        }
        double xpos = 1.0 * (dancers[i].x - dividerx);  // X relative to the center
        double ypos = -3.0 * (dancers[i].y - dividery);  // Y rel to center needs to be mult by 3 and flipped

        // grab just #1 boy/girl and #2 boy/girl (they might not be in order by i)
        if (dancers[i].gender != 1) {
            // boys
            if (dancers[i].coupleNum == 0) {  // couple # 1
                boy1_x = xpos;
                boy1_y = ypos;
            } else if (dancers[i].coupleNum == 1) { // couple # 2
                boy2_x = xpos;
                boy2_y = ypos;
            }
        } else {
            // girls
            if (dancers[i].coupleNum == 0) {  // couple # 1
                girl1_x = xpos;
                girl1_y = ypos;
            } else if (dancers[i].coupleNum == 1) {  // couple # 2
                girl2_x = xpos;
                girl2_y = ypos;
            }
        }
    }
//    qDebug() << "Boy1/2: " << boy1_x << boy1_y << boy2_x << boy2_y;
//    qDebug() << "Girl1/2: " << girl1_x << girl1_y << girl2_x << girl2_y;

    // set globals
    *boyOrder = whichOrder(boy1_x, boy1_y, boy2_x, boy2_y);
    *girlOrder = whichOrder(girl1_x, girl1_y, girl2_x, girl2_y);
//    qDebug() << "Boys: " << orderToString(*bOrder).c_str() << ", Girls: " << orderToString(*gOrder).c_str();
}
