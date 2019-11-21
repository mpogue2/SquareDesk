#include "taminationsinterface.h"
#include <algorithm>


using namespace std;

static bool anglesEqual(double theta1, double theta2) {
    return abs(theta1 - theta2) < (M_PI / 180.0);
}

TamAffineTransform::TamAffineTransform() : x1(1.0), x2(0.0), x3(0.0), y1(0.0), y2(1.0), y3(0.0) {}
TamAffineTransform::TamAffineTransform(double x1, double x2, double x3, double y1, double y2, double y3) :  x1(x1), x2(x2), x3(x3), y1(y1), y2(y2), y3(y3) {}

TamAffineTransform TamAffineTransform::getTranslateInstance(double x, double y)
{
    TamAffineTransform a;
    a.x3 = x;
    a.y3 = y;
    return a;
}
    
//  Generate a new transform that does a rotation(
TamAffineTransform TamAffineTransform::getRotateInstance(double theta)
{
    TamAffineTransform ab;
    if (theta) {
        ab.y1 = sin(theta);
        ab.x2 = -ab.y1;
        ab.x1 = cos(theta);
        ab.y2 = ab.x1;
    }
    return ab;
}

//  Generate a new transform that does a scaling
TamAffineTransform TamAffineTransform::getScaleInstance(double x, double y)
{
    TamAffineTransform a;
    a.scale(x,y);
    return a;
};

//  Add a translation to this transform
TamAffineTransform TamAffineTransform::translate(double x, double y)
{
    x3 += x*x1 + y*x2;
    y3 += x*y1 + y*y2;
    return *this;
};

//  Add a scaling to this transform
void  TamAffineTransform::scale(double x, double y)
{
    x1 *= x;
    y1 *= x;
    x2 *= y;
    y2 *= y;
};

//  Add a rotation to this transform
TamAffineTransform TamAffineTransform::rotate(double angle)
{
    double s = sin(angle);
    double c = cos(angle);
    TamAffineTransform copy(*this);
    //{ x1: x1, x2: x2, y1: y1, y2: y2 };
    x1 =  c * copy.x1 + s * copy.x2;
    x2 = -s * copy.x1 + c * copy.x2;
    y1 =  c * copy.y1 + s * copy.y2;
    y2 = -s * copy.y1 + c * copy.y2;
    return *this;
}

TamAffineTransform TamAffineTransform::preConcatenate(const TamAffineTransform &tx)
{
    // [result] = [this] x [Tx]
    TamAffineTransform result;
    result.x1 = x1 * tx.x1 + x2 * tx.y1;
    result.x2 = x1 * tx.x2 + x2 * tx.y2;
    result.x3 = x1 * tx.x3 + x2 * tx.y3 + x3;
    result.y1 = y1 * tx.x1 + y2 * tx.y1;
    result.y2 = y1 * tx.x2 + y2 * tx.y2;
    result.y3 = y1 * tx.x3 + y2 * tx.y3 + y3;
    return result;
}

TamAffineTransform TamAffineTransform::concatenate(const TamAffineTransform &tx)
{
    // [result] = [Tx] x [this]
    TamAffineTransform result;
    result.x1 = tx.x1 * x1 + tx.x2 * y1;
    result.x2 = tx.x1 * x2 + tx.x2 * y2;
    result.x3 = tx.x1 * x3 + tx.x2 * y3 + tx.x3;
    result.y1 = tx.y1 * x1 + tx.y2 * y1;
    result.y2 = tx.y1 * x2 + tx.y2 * y2;
    result.y3 = tx.y1 * x3 + tx.y2 * y3 + tx.y3;
    return result;
}

TamAffineTransform TamAffineTransform::concatenate(const TamVector &vx)
{
    TamAffineTransform result(*this);
    result.x3 += vx.x;
    result.y3 += vx.y;
    return result;
}

//  Compute and return the inverse matrix - only for affine transform matrix
TamAffineTransform TamAffineTransform::getInverse()
{
    TamAffineTransform inv;
    double det = x1*y2 - x2*y1;
    inv.x1 = y2/det;
    inv.y1 = -y1/det;
    inv.x2 = -x2/det;
    inv.y2 = x1/det;
    inv.x3 = (x2*y3 - y2*x3) / det;
    inv.y3 = (y1*x3 - x1*y3) / det;
    return inv;
};
    
//  Apply a tranTamsform to a Vector
//  A bit of a hack, in that we are modifying the Vector class here in the
//  AffineTransform class.  But this avoids a circular dependency.
//TamVector TamVector::concatenate(const TamVector &tx)
//{
//    TamAffineTransform vx = TamAffineTransform::getTranslateInstance(tx.x,tx.y);
//    vx = vx.concatenate(tx);
//    return vx.location();
//}
//TamVector TamVector::preConcatenate(const TamVector &tx)
//{
//    TamAffineTransform vx = TamAffineTransform::getTranslateInstance(tx.x,tx.y);
//    vx = vx.preConcatenate(tx);
//    return vx.location();
//}
//
//  Return a string that can be used as the svg transform attribute
QString TamAffineTransform::toString()
{
    return QString("matrix(%1,%2,%3,%4,%5.%6").arg(x1).arg(y1).arg(x2).arg(y2).arg(x3).arg(y3);
}



TamHands TamHandsFromQString(const QString &hands) {
    if (hands == QString("both")) {
        return TamHandsBoth; }
    if (hands == QString("gripboth")) {
        return TamHandsGripBoth; }
    if (hands == QString("gripleft")) {
        return TamHandsGripLeft; }
    if (hands == QString("gripright")) {
        return TamHandsGripRight; }
    if (hands == QString("left")) {
        return TamHandsLeft; }
    if (hands == QString("none")) {
        return TamHandsNone; }
    if (hands == QString("right")) {
        return TamHandsRight; }
    
    return TamHandsNone;
}



TamBezier::TamBezier(double x1, double y1, double ctrlx1, double ctrly1, double ctrlx2, double ctrly2, double x2, double y2) :
    x1(x1),
    y1(y1),
    ctrlx1(ctrlx1),
    ctrly1(ctrly1),
    ctrlx2(ctrlx2),
    ctrly2(ctrly2),
    x2(x2),
    y2(y2)
{
    calculatecoefficients();
}

void TamBezier::calculatecoefficients()
{
    cx = 3.0*(ctrlx1-x1);
    bx = 3.0*(ctrlx2-ctrlx1) - cx;
    ax = x2 - x1 - cx - bx;

    cy = 3.0*(ctrly1-y1);
    by = 3.0*(ctrly2-ctrly1) - cy;
    ay = y2 - y1 - cy - by;
}

  //  Compute X, Y values for a specific t value
double TamBezier::xt(double t) {
    return x1 + t*(cx + t*(bx + t*ax));
}
double TamBezier::yt(double t) {
    return y1 + t*(cy + t*(by + t*ay));
}
//  Compute dx, dy values for a specific t value
double TamBezier::dxt(double t) {
    return cx + t*(2.0*bx + t*3.0*ax);
}
double TamBezier::dyt(double t) {
    return cy + t*(2.0*by + t*3.0*ay);
}
double TamBezier::angle(double t) {
    return atan2(dyt(t),dxt(t));
}

//  Return the movement along the curve given "t" between 0 and 1
TamAffineTransform TamBezier::translate(double t)
{
    double x = xt(t);
    double y = yt(t);
    return TamAffineTransform::getTranslateInstance(x,y);
};

//  Return the angle of the derivative given "t" between 0 and 1
TamAffineTransform TamBezier::rotate(double t)
{
    double theta = angle(t);
    return TamAffineTransform::getRotateInstance(theta);
};

//  Return turn direction at end of curve
double TamBezier::rolling()
{
    //  Check angle at end
    double theta = angle(1.0);
    //  If it's 180 then use angle at halfway point
    if (anglesEqual(theta,M_PI))
        theta = angle(0.5);
    //  If angle is 0 then no turn
    if (anglesEqual(theta,0))
        return 0;
    else
        return theta;
}

//  Return the angle of the 2nd derivative given "t" between 0 and 1
double TamBezier::turn(double t)
{
    double x = 2.0*bx + t*6.0*ax;
    double y = 2.0*by + t*6.0*ay;
    double theta = atan2(y,x);
    return theta;
}


QString TamBezier::toString()
{
    return QString("[%1,%2,%3,%4,%5,%6,%7,%8]")
        .arg(x1).arg(y1).arg(ctrlx1).arg(ctrly1).arg(ctrlx2).arg(ctrly2).arg(x2).arg(y2);
};




TamHands TamMovement::NOHANDS = TamHandsNone;
TamHands TamMovement::LEFTHAND = TamHandsLeft;
TamHands TamMovement::RIGHTHAND = TamHandsRight;
TamHands TamMovement::BOTHHANDS = TamHandsBoth;
TamHands TamMovement::GRIPLEFT = TamHandsGripLeft;
TamHands TamMovement::GRIPRIGHT = TamHandsGripRight;
TamHands TamMovement::GRIPBOTH = TamHandsGripBoth;
TamHands TamMovement::ANYGRIP = TamHandsAnyGrip;


TamMovement::TamMovement(double fullbeats, TamHands hands,  double cx1,  double cy1,
                         double cx2,  double cy2,  double x2,  double y2,
                         double cx3,  double cx4,  double cy4,
                         double x4,  double y4, double beats) :
    fullbeats(fullbeats), hands(hands),
    cx1(cx1), cy1(cy1), cx2(cx2), cy2(cy2), x2(x2), y2(y2),
    cx3(cx3), cx4(cx4), cy4(cy4), x4(x4), y4(y4), beats(beats)
{
    btranslate = TamBezier(0.0,0.0,cx1,cy1,cx2,cy2,x2,y2);
    if (isnan(cx3)) {
        brotate = TamBezier(0.0,0.0,cx3,0,cx4,cy4,x4,y4);
    }
    else {
        brotate = TamBezier(0.0,0.0,cx1,0,cx2,cy2,x2,y2);
        this->cx3 = cx1;
        this->cx4 = cx2;
        this->cy4 = cy2;
        this->x4 = x2; 
        this->y4 = y2;
    }
    if (isnan(beats)) {
        beats = fullbeats;
    }
}
    

TamMovement::TamMovement(double fullbeats, QString hands,  double cx1,  double cy1,
                         double cx2,  double cy2,  double x2,  double y2,
                         double cx3,  double cx4,  double cy4,
                         double x4,  double y4, double beats) :
    fullbeats(fullbeats), hands(TamHandsFromQString(hands)),
    cx1(cx1), cy1(cy1), cx2(cx2), cy2(cy2), x2(x2), y2(y2),
    cx3(cx3), cx4(cx4), cy4(cy4), x4(x4), y4(y4), beats(beats)
{
    btranslate = TamBezier(0.0,0.0,cx1,cy1,cx2,cy2,x2,y2);
    if (isnan(cx3)) {
        brotate = TamBezier(0.0,0.0,cx3,0,cx4,cy4,x4,y4);
    }
    else {
        brotate = TamBezier(0.0,0.0,cx1,0,cx2,cy2,x2,y2);
        cx3 = cx1;
        cx4 = cx2;
        cy4 = cy2;
        x4 = x2; 
        y4 = y2;
    }
    if (isnan(beats)) {
        beats = fullbeats;
    }
}

TamMovement::TamMovement(TamMovementPtr m) :
    fullbeats(m->fullbeats),
    hands(m->hands),
    cx1(m->cx1),
    cy1(m->cy1),
    cx2(m->cx2),
    cy2(m->cy2),
    x2(m->x2),
    y2(m->y2),
    cx3(m->cx3),
    /* cy3?, */
    cx4(m->cx4),
    cy4(m->cy4),
    x4(m->x4),
    y4(m->y4),
    beats(m->beats),
    btranslate(m->btranslate),
    brotate(m->brotate)
{
}



TamMovementPtr TamMovement::time(double beats) {
    TamMovementPtr n = new TamMovement(beats, hands,
                                       cx1,cy1,cx2,cy2,x2,y2,
                                       cx3,cx4,cy4,x4,y4,beats);
    return n;
}

TamMovementPtr TamMovement::useHands(QString hands) {
    TamMovementPtr n = new TamMovement(fullbeats, hands,
                                   cx1,cy1,cx2,cy2,x2,y2,
                                   cx3,cx4,cy4,x4,y4,beats);
    return n;
}

double TamMovement::scaled_tt(double t) {
    if (isnan(t)) {
        t = beats;
    }
    double tt = min(max(0.0,t), beats);
    double stt = tt/fullbeats;
    return stt;
}
            
    

TamAffineTransform TamMovement::translate(double t) {
    double stt = scaled_tt(t);
    return btranslate.translate(stt);
}

TamAffineTransform TamMovement::rotate(double t) {
    double stt = scaled_tt(t);
    return btranslate.rotate(stt);
}

TamAffineTransform TamMovement::transform(double t) {
    TamAffineTransform tx;
    tx = tx.preConcatenate(translate(t));
    tx = tx.preConcatenate(rotate(t));
    return tx;
}


TamMovementPtr TamMovement::clone() {
    TamMovementPtr n = new TamMovement(
        fullbeats,
        hands,
        btranslate.ctrlx1,btranslate.ctrly1,
        btranslate.ctrlx2,btranslate.ctrly2,
        btranslate.x2,btranslate.y2,
        brotate.ctrlx1,
        brotate.ctrlx2,brotate.ctrly2,
        brotate.x2,brotate.y2,beats);
    return n;
}

TamMovementPtr TamMovement::reflect() {
    return scale(1,-1);
}
    
TamMovementPtr TamMovement::scale(double x, double y) {
    TamMovementPtr n = new TamMovement(beats,
                                       (y < 0 && hands == TamMovement::RIGHTHAND) ? TamMovement::LEFTHAND
                                       : (y < 0 && hands == TamMovement::LEFTHAND) ? TamMovement::RIGHTHAND
                                       : hands,  // what about GRIPLEFT, GRIPRIGHT?
                                       cx1*x,cy1*y,cx2*x,cy2*y,x2*x,y2*y,
                                       cx3*x,cx4*x,cy4*y,x4*x,y4*y,fullbeats);
    return n;
}

TamMovementPtr TamMovement::skew(double x, double y) {
    if (beats < fullbeats)
        return skewClip(x,y);
    else
        return skewFull(x,y);
}

TamMovementPtr TamMovement::skewFull(double x, double y) {
    TamMovementPtr n = new TamMovement(fullbeats,hands,cx1,cy1,
                                       cx2+x,cy2+y,x2+x,y2+y,
                                       cx3,cx4,cy4,x4,y4,beats);
    return n;
}


TamMovementPtr TamMovement::clip(double b) {
    if (b > 0 && b < fullbeats) {
        TamMovementPtr n = new TamMovement(fullbeats,hands,cx1,cy1,
                                       cx2,cy2,x2,y2,
                                       cx3,cx4,cy4,x4,y4,b);
        return n;
    }
    return TamMovementPtr(this);
}

/**
 *   Skew a movement that has been clipped, adjusting so the amount of
 *   skew is appplied to the clip point
 */
TamMovementPtr TamMovement::skewClip(double x,double y) {
    TamVector vdelta = TamVector(x,y);
    TamVector vfinal = translate().location().add(vdelta);
    TamMovementPtr m = this;
    int maxiter = 100;
    do {
        // Shift the end point by the current difference
        TamMovementPtr nm = m->skewFull(vdelta.x,vdelta.y);
        if (m != this)
            delete m;
        m = nm;
        // See how that affects the clip point
        TamVector loc = m->translate().location();
        vdelta = vfinal.subtract(loc);
        maxiter -= 1;
    } while (vdelta.length() > 0.001 && maxiter > 0);
    //  If timed out, return original rather than something that
    //  might put the dancers in outer space
    return maxiter > 0 ? m : new TamMovement(this);
}

QString TamMovement::toString() {
    return btranslate.toString() + " " + brotate.toString();
}

bool TamMovement::isStand() {
    return x2 == 0 && y2 == 0 && x4 == 0 && y4 == 0;
}




void TamPath::clear() {
    for (TamMovementPtr m : movelist)
        delete m;
    
    movelist.clear();
    transformlist.clear();
}

void TamPath::recalculate() {
    transformlist.clear();
    TamAffineTransform tx;
    for (auto m : movelist)
    {
        tx = tx.preConcatenate(m->translate());
        tx = tx.preConcatenate(m->rotate());
        transformlist.push_back(tx);
    }
}

double TamPath::beats() {
    double r = 0.0;
    for (auto m : movelist)
    {
        r += m->beats;
    }
    return r;
}

TamPathPtr TamPath::changebeats(double newbeats)
{
    double factor = newbeats / beats();
    for (size_t i = 0; i < movelist.size(); ++i)
    {
        TamMovementPtr m = movelist[i];
        movelist[i] = m->time(m->beats * factor);
        delete m;
    }
    return this;
}

TamPathPtr TamPath::changehands(const QString &hands)
{
    for (size_t i = 0; i < movelist.size(); ++i)
    {
        TamMovementPtr m = movelist[i];
        movelist[i] = m->useHands(hands);
        delete m;
    }
    return this;
};

  //  Change the path by scale factors
TamPathPtr TamPath::scale(double x, double y)
{
    for (size_t i = 0; i < movelist.size(); ++i)
    {
        TamMovementPtr m = movelist[i];
        movelist[i] = m->scale(x,y);
        delete m;
    }
    recalculate();
    return this;
};

  //  Skew the path by translating the destination point
TamPathPtr TamPath::skew(double x,double y)
{
    TamMovementPtr m = movelist.back();
    movelist.pop_back();
    movelist.push_back(m->skew(x,y));
    delete m;
    recalculate();
    return this;
};

  //  Append one movement or all the movements of another path
  //  to the end of the Path
TamPathPtr TamPath::add(TamMovementPtr m)
{
    movelist.push_back(m);
    recalculate();
    return this;
}

TamPathPtr TamPath::add(TamPathPtr path)
{
    for (TamMovementPtr m : path->movelist) {
        movelist.push_back(new TamMovement(m));
    }
    recalculate();
    return this;
}

//  Remove and return the last movement of the path
TamMovementPtr TamPath::pop() {
    TamMovementPtr m = movelist.back();
    movelist.pop_back();
    recalculate();
    return m;
}

//  Reflect the path about the x-axis
TamPathPtr TamPath::reflect()
{
    for (size_t i = 0; i < movelist.size(); i++)
    {
        TamMovementPtr m = movelist[i];
        movelist[i] = m->reflect();
        delete m;
    }
    recalculate();
    return this;
}

