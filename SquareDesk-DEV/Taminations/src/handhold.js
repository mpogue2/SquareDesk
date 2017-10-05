/*

    Copyright 2017 Brad Christie

    This file is part of Taminations.

    Taminations is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Taminations is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with Taminations.  If not, see <http://www.gnu.org/licenses/>.

 */

//  Handhold class for computing the potential handhold between two dancers
//  The actual graphic hands are part of the Dancer object

define(['movement','affinetransform'],function(Movement,AffineTransform) {
  //  Properties of Handhold object
  //  Dancer d1,d2;
  //  int h1,h2;
  //  angle ah1, ah2; (in radians)
  //  double score;
  //  private boolean isincenter = false;
  //  public static double dfactor0 = 1.0;
  Handhold = function(/*Dancer*/ dd1, /*Dancer*/ dd2,
      /*int*/ hh1, /*int*/ hh2, /*angle*/ ahh1, ahh2, /*distance*/ d, s) {
    this.d1 = dd1;
    this.d2 = dd2;
    this.h1 = hh1;
    this.h2 = hh2;
    this.ah1 = ahh1;
    this.ah2 = ahh2;
    this.distance = d;
    this.score = s;
  };

  //  If two dancers can hold hands, create and return a handhold.
  //  Else return null.
  Handhold.getHandhold = function(/*Dancer*/ d1, /*Dancer*/ d2)
  {
    if (d1.hidden || d2.hidden)
      return null;
    //  Turn off grips if not specified in current movement
    if ((d1.hands & Movement.GRIPRIGHT) != Movement.GRIPRIGHT)
      d1.rightgrip = null;
    if ((d1.hands & Movement.GRIPLEFT) != Movement.GRIPLEFT)
      d1.leftgrip = null;
    if ((d2.hands & Movement.GRIPRIGHT) != Movement.GRIPRIGHT)
      d2.rightgrip = null;
    if ((d2.hands & Movement.GRIPLEFT) != Movement.GRIPLEFT)
      d2.leftgrip = null;


    //  Check distance
    var x1 = d1.tx.translateX;
    var y1 = d1.tx.translateY;
    var x2 = d2.tx.translateX;
    var y2 = d2.tx.translateY;
    var dx = x2-x1;
    var dy = y2-y1;
    var dfactor1 = 0.1;  // for distance up to 2.0
    var dfactor2 = 2.0;  // for distance past 2.0
    var cutover = 2.0;
    if (d1.tamsvg.hexagon)
      cutover = 2.5;
    if (d1.tamsvg.bigon)
      cutover = 3.7;
    var d = Math.sqrt(dx*dx+dy*dy);
    var d0 = d*Handhold.dfactor0;
    var score1 = d0 > cutover ? (d0-cutover)*dfactor2+2*dfactor1 : d0*dfactor1;
    var score2 = score1;
    //  Angle between dancers
    var a0 = Math.atan2(dy,dx);
    //  Angle each dancer is facing
    var a1 = Math.atan2(d1.tx.shearY,d1.tx.scaleY);
    var a2 = Math.atan2(d2.tx.shearY,d2.tx.scaleY);
    //  For each dancer, try left and right hands
    var h1 = 0;
    var h2 = 0;
    var ah1 = 0;
    var ah2 = 0;
    var afactor1 = 0.2;
    var afactor2 = 1.0;
    if (d1.tamsvg.bigon)
      afactor2 = 0.6;
    //  Dancer 1
    var a = Math.abs(Math.IEEEremainder(Math.abs(a1-a0+Math.PI*3/2),Math.PI*2));
    var ascore = a > Math.PI/6 ? (a-Math.PI/6)*afactor2+Math.PI/6*afactor1
        : a*afactor1;
    if (score1+ascore < 1.0 && (d1.hands & Movement.RIGHTHAND) != 0 &&
        d1.rightgrip==null || d1.rightgrip==d2) {
      score1 = d1.rightgrip==d2 ? 0.0 : score1 + ascore;
      h1 = Movement.RIGHTHAND;
      ah1 = a1-a0+Math.PI*3/2;
    } else {
      a = Math.abs(Math.IEEEremainder(Math.abs(a1-a0+Math.PI/2),Math.PI*2));
      ascore = a > Math.PI/6 ? (a-Math.PI/6)*afactor2+Math.PI/6*afactor1
          : a*afactor1;
      if (score1+ascore < 1.0 && (d1.hands & Movement.LEFTHAND) != 0 &&
          d1.leftgrip==null || d1.leftgrip==d2) {
        score1 = d1.leftgrip==d2 ? 0.0 : score1 + ascore;
        h1 = Movement.LEFTHAND;
        ah1 = a1-a0+Math.PI/2;
      } else
        score1 = 10;
    }
    //  Dancer 2
    a = Math.abs(Math.IEEEremainder(Math.abs(a2-a0+Math.PI/2),Math.PI*2));
    ascore = a > Math.PI/6 ? (a-Math.PI/6)*afactor2+Math.PI/6*afactor1
        : a*afactor1;
    if (score2+ascore < 1.0 && (d2.hands & Movement.RIGHTHAND) != 0 &&
        d2.rightgrip==null || d2.rightgrip==d1) {
      score2 = d2.rightgrip==d1 ? 0.0 : score2 + ascore;
      h2 = Movement.RIGHTHAND;
      ah2 = a2-a0+Math.PI/2;
    } else {
      a = Math.abs(Math.IEEEremainder(Math.abs(a2-a0+Math.PI*3/2),Math.PI*2));
      ascore = a > Math.PI/6 ? (a-Math.PI/6)*afactor2+Math.PI/6*afactor1
          : a*afactor1;
      if (score2+ascore < 1.0 && (d2.hands & Movement.LEFTHAND) != 0 &&
          d2.leftgrip==null || d2.leftgrip==d1) {
        score2 = d2.leftgrip==d1 ? 0.0 : score2 + ascore;
        h2 = Movement.LEFTHAND;
        ah2 = a2-a0+Math.PI*3/2;
      } else
        score2 = 10;
    }

    if (d1.rightgrip == d2 && d2.rightgrip == d1)
      return new Handhold(d1,d2,Movement.RIGHTHAND,Movement.RIGHTHAND,ah1,ah2,d,0);
    if (d1.rightgrip == d2 && d2.leftgrip == d1)
      return new Handhold(d1,d2,Movement.RIGHTHAND,Movement.LEFTHAND,ah1,ah2,d,0);
    if (d1.leftgrip == d2 && d2.rightgrip == d1)
      return new Handhold(d1,d2,Movement.LEFTHAND,Movement.RIGHTHAND,ah1,ah2,d,0);
    if (d1.leftgrip == d2 && d2.leftgrip == d1)
      return new Handhold(d1,d2,Movement.LEFTHAND,Movement.LEFTHAND,ah1,ah2,d,0);

    if (score1 > 1.0 || score2 > 1.0 || score1+score2 > 1.2)
      return null;
    return new Handhold(d1,d2,h1,h2,ah1,ah2,d,score1+score2);
  };

  /* boolean */
  Handhold.prototype.inCenter = function()
  {
    this.isincenter = this.d1.location.length < 1.1 &&
                      this.d2.location.length < 1.1;
    if (this.isincenter) {
      this.ah1 = 0;
      this.ah2 = 0;
      this.distance = 2.0;
    }
    return this.isincenter;
  };

  //  Make the handhold visible
  Handhold.prototype.paint = function()
  {
    //  Scale should be 1 if distance is 2
    var scale = this.distance/2;
    if (this.h1 == Movement.RIGHTHAND || this.h1 == Movement.GRIPRIGHT) {
      if (!this.d1.rightHandVisibility) {
        this.d1.righthand.setAttribute('visibility','visible');
        this.d1.rightHandVisibility = true;
      }
      this.d1.rightHandNewVisibility = true;
      this.d1.rightHandTransform = AffineTransform.getRotateInstance(-this.ah1)
      .preConcatenate(AffineTransform.getScaleInstance(scale,scale));
    }
    if (this.h1 == Movement.LEFTHAND || this.h1 == Movement.GRIPLEFT) {
      if (!this.d1.leftHandVisibility) {
        this.d1.lefthand.setAttribute('visibility','visible');
        this.d1.leftHandVisibility = true;
      }
      this.d1.leftHandNewVisibility = true;
      this.d1.leftHandTransform = AffineTransform.getRotateInstance(-this.ah1)
      .preConcatenate(AffineTransform.getScaleInstance(scale,scale));
    }
    if (this.h2 == Movement.RIGHTHAND || this.h2 == Movement.GRIPRIGHT) {
      if (!this.d2.rightHandVisibility) {
        this.d2.righthand.setAttribute('visibility','visible');
        this.d2.rightHandVisibility = true;
      }
      this.d2.rightHandNewVisibility = true;
      this.d2.rightHandTransform = AffineTransform.getRotateInstance(-this.ah2)
      .preConcatenate(AffineTransform.getScaleInstance(scale,scale));
    }
    if (this.h2 == Movement.LEFTHAND || this.h2 == Movement.GRIPLEFT) {
      if (!this.d2.leftHandVisibility) {
        this.d2.lefthand.setAttribute('visibility','visible');
        this.d2.leftHandVisibility = true;
      }
      this.d2.leftHandNewVisibility = true;
      this.d2.leftHandTransform = AffineTransform.getRotateInstance(-this.ah2)
      .preConcatenate(AffineTransform.getScaleInstance(scale,scale));
    }
  };

  return Handhold;

});
