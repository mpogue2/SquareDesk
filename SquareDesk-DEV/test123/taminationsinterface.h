/****************************************************************************
**
** Copyright (C) 2016-2020 Mike Pogue, Dan Lyke
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
#ifndef TAMINATIONSINTERFACE_H_INCLUDED
#define TAMINATIONSINTERFACE_H_INCLUDED

#include <QString>
#include <QPointF>
#include <QDir>
#include "math.h"
#include <vector>


class TamAffineTransform;

class TamVector {
public:
    double x, y;
    TamVector concatenate(const TamVector &tx);
    TamVector preConcatenate(const TamVector &tx);
    TamVector(double x, double y) : x(x), y(y) {}
    TamVector add(const TamVector &p) { return TamVector(x+p.x, y+p.y); }
    TamVector subtract(const TamVector &p) { return TamVector(x-p.x, y-p.y); }
    double length() { return sqrt(x*x+y*y);}
};


class TamAffineTransform {
public:
    double x1, x2, x3, y1, y2, y3;
    TamAffineTransform();
    TamAffineTransform(double x1, double x2, double x3, double y1, double y2, double y3);

    double scaleX() { return x1; }
    double scaleY() { return y2; }
    double shearX() { return x2; }
    double shearY() { return y1; }
    double translateX() { return x3; }
    double translateY() { return y3; }
    TamVector location() { return TamVector(x3,y3); }
    double angle() { return atan2(y1,y2); }; 

    //  Generate a new transform that moves to a new location
    static TamAffineTransform getTranslateInstance(double x, double y);
    static TamAffineTransform getRotateInstance(double theta);
    static TamAffineTransform getScaleInstance(double x, double y);
    TamAffineTransform translate(double x, double y);
    void scale(double x, double y);
    //  Add a rotation to this transform
    TamAffineTransform rotate(double angle);
    TamAffineTransform copy(TamAffineTransform const *);
    TamAffineTransform copy(const TamAffineTransform &);
    TamAffineTransform preConcatenate(const TamAffineTransform &tx);
    TamAffineTransform concatenate(const TamAffineTransform &tx);
    TamAffineTransform concatenate(const TamVector &vx);
    //  Compute and return the inverse matrix - only for affine transform matrix
    TamAffineTransform getInverse();
    //  Apply a tranTamsform to a Vector
    //  A bit of a hack, in that we are modifying the Vector class here in the
    //  AffineTransform class.  But this avoids a circular dependency.
    TamAffineTransform preConcatenate(const TamVector &tx);
    //  Return a string that can be used as the svg transform attribute
    QString toString();
};

class TamAffineTransform;

class TamBezier {
public:
    double x1, y1, ctrlx1, ctrly1, ctrlx2, ctrly2, x2, y2;
    double cx, bx, ax, cy, by, ay;
    TamBezier() : x1(NAN), y1(NAN), ctrlx1(NAN), ctrly1(NAN), ctrlx2(NAN), ctrly2(NAN), x2(NAN), y2(NAN) {};
    TamBezier(double x1, double y1, double ctrlx1, double ctrly1, double ctrlx2, double ctrly2, double x2, double y2);
    void calculatecoefficients();
    double xt(double t);
    double yt(double t);
    double dxt(double t);
    double dyt(double t);
    double angle(double t);
    double rolling();
    double turn(double t);
    QString toString();
//  Return the movement along the curve given "t" between 0 and 1
    TamAffineTransform translate(double t);
    TamAffineTransform rotate(double t);
    
};


typedef enum {
    TamHandsBoth, /* hands="both" */
    TamHandsGripBoth, /* hands="gripboth" */
    TamHandsGripLeft, /* hands="gripleft" */
    TamHandsGripRight, /* hands="gripright" */
    TamHandsLeft, /* hands="left" */
    TamHandsNone, /* hands="none" */
    TamHandsRight, /* hands="right" */
    TamHandsAnyGrip, /* hands="right" */
} TamHands;


TamHands TamHandsFromQString(const QString &hands);

class TamMovement;
typedef TamMovement *TamMovementPtr;


class TamMovement {
public:
    double fullbeats;
    TamHands hands;
    double cx1, cy1, cx2, cy2, x2, y2, cx3, /* cy3?, */ cx4, cy4, x4, y4;
    double beats;

   
    TamBezier btranslate;
    TamBezier brotate;

public:
    static TamHands NOHANDS;
    static TamHands LEFTHAND;
    static TamHands RIGHTHAND;
    static TamHands BOTHHANDS;
    static TamHands GRIPLEFT;
    static TamHands GRIPRIGHT;
    static TamHands GRIPBOTH;
    static TamHands ANYGRIP;

    
    TamMovement(double fullbeats, QString hands,  double cx1,  double cy1,
                double cx2,  double cy2,  double x2,  double y2,
                double cx3 = NAN,  double cx4 = NAN,  double cy4 = NAN,
                double x4 = NAN,  double y4 = NAN, double beats = NAN);
    TamMovement(double fullbeats, TamHands hands,  double cx1,  double cy1,
                double cx2,  double cy2,  double x2,  double y2,
                double cx3 = NAN,  double cx4 = NAN,  double cy4 = NAN,
                double x4 = NAN,  double y4 = NAN, double beats = NAN);
    TamMovement(TamMovementPtr m);
    TamMovementPtr time(double beats);
    TamMovementPtr useHands(QString hands);
    double scaled_tt(double t);
    TamAffineTransform translate(double t = NAN);
    TamAffineTransform rotate(double t = NAN);
    TamAffineTransform transform(double t);
    TamMovementPtr clone();
    TamMovementPtr reflect();
    TamMovementPtr scale(double x, double y);
    TamMovementPtr skew(double x, double y);
    TamMovementPtr skewFull(double x, double y);
    TamMovementPtr clip(double b);
    TamMovementPtr skewClip(double x,double y);
    QString toString();
    bool isStand();
};



class TamPath;
typedef TamPath* TamPathPtr;

class TamPath {
public:
    std::vector<TamMovementPtr> movelist;
    std::vector<TamAffineTransform> transformlist;

    TamPath(TamPathPtr);

    void clear();
    void recalculate();
    double beats();
    TamPathPtr changebeats(double newbeats);
    TamPathPtr changehands(const QString &hands);
    //  Change the path by scale factors
    TamPathPtr scale(double x, double y);
    //  Skew the path by translating the destination point
    TamPathPtr skew(double x,double y);
    //  Append one movement or all the movements of another path
    //  to the end of the Path
    TamPathPtr add(const TamMovementPtr m);
    TamPathPtr add(TamPathPtr path);
    //  Remove and return the last movement of the path
    TamMovementPtr pop();
    //  Reflect the path about the x-axis
    TamPathPtr reflect();
};


class TaminationsInterface {
public:
    TaminationsInterface();
    virtual ~TaminationsInterface();
    void readTaminationsDirectory(QDir taminationsDir);

private:
};


#endif /* ifndef TAMINATIONSINTERFACE_H_INCLUDED */
