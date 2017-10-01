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

define(['affinetransform','bezier'],function(AffineTransform,Bezier) {

  //  Movement class
  //  Constructor for independent heading and movement
  Movement = function(fullbeats,hands,cx1,cy1,cx2,cy2,x2,y2,
                      cx3,cx4,cy4,x4,y4,beats) {
    this.cx1 = cx1;
    this.cy1 = cy1;
    this.cx2 = cx2;
    this.cy2 = cy2;
    this.x2 = x2;
    this.y2 = y2;
    this.btranslate = new Bezier(0,0,cx1,cy1,cx2,cy2,x2,y2);
    if (cx3 != undefined) {
      this.brotate = new Bezier(0,0,cx3,0,cx4,cy4,x4,y4);
      this.cx3 = cx3;
      this.cx4 = cx4;
      this.cy4 = cy4;
      this.x4 = x4;
      this.y4 = y4;
    }
    else {
      this.brotate = new Bezier(0,0,cx1,0,cx2,cy2,x2,y2);
      this.cx3 = cx1;
      this.cx4 = cx2;
      this.cy4 = cy2;
      this.x4 = x2;
      this.y4 = y2;
    }
    this.beats = this.fullbeats = fullbeats;
    if (typeof beats == "number")
      this.beats = beats;
    if (typeof hands == "string")
      this.hands = Movement.getHands(hands);
    else  //  should be one of the ints below
      this.hands = hands;
  };

  Movement.NOHANDS = 0;
  Movement.LEFTHAND = 1;
  Movement.RIGHTHAND = 2;
  Movement.BOTHHANDS = 3;
  Movement.GRIPLEFT = 5;
  Movement.GRIPRIGHT = 6;
  Movement.GRIPBOTH =  7;
  Movement.ANYGRIP =  4;
  Movement.setHands = { "none": Movement.NOHANDS,
      "left": Movement.LEFTHAND,
      "right": Movement.RIGHTHAND,
      "both": Movement.BOTHHANDS,
      "gripleft": Movement.GRIPLEFT,
      "gripright": Movement.GRIPRIGHT,
      "gripboth": Movement.GRIPBOTH,
      "anygrip": Movement.ANYGRIP };

  Movement.getHands = function(h) {
    return Movement.setHands[h];
  }

  /**
   * Construct a Movement from the attributes of an XML movement
   * @param elem from xml
   */
  Movement.fromElement = function(elem) {
    if ($(elem).attr("cx3") != undefined) {
      return new Movement(Number($(elem).attr("beats")),
          Movement.getHands($(elem).attr("hands")),
          Number($(elem).attr("cx1")),
          Number($(elem).attr("cy1")),
          Number($(elem).attr("cx2")),
          Number($(elem).attr("cy2")),
          Number($(elem).attr("x2")),
          Number($(elem).attr("y2")),
          Number($(elem).attr("cx3")),
          Number($(elem).attr("cx4")),
          Number($(elem).attr("cy4")),
          Number($(elem).attr("x4")),
          Number($(elem).attr("y4")),
          Number($(elem).attr("beats")))
    }
    else {
      return new Movement(Number($(elem).attr("beats")),
          Movement.getHands($(elem).attr("hands")),
          Number($(elem).attr("cx1")),
          Number($(elem).attr("cy1")),
          Number($(elem).attr("cx2")),
          Number($(elem).attr("cy2")),
          Number($(elem).attr("x2")),
          Number($(elem).attr("y2")),
          Number($(elem).attr("cx1")),
          Number($(elem).attr("cx2")),
          Number($(elem).attr("cy2")),
          Number($(elem).attr("x2")),
          Number($(elem).attr("y2")),
          Number($(elem).attr("beats")))
    }
  }

  /**
   * Return a new movement by changing the beats
   */
  Movement.prototype.time = function(b) {
    return new Movement(b,this.hands,
        this.cx1,this.cy1,this.cx2,this.cy2,this.x2,this.y2,
        this.cx3,this.cx4,this.cy4,this.x4,this.y4,b);
  }

  /**
   * Return a new movement by changing the hands
   */
  Movement.prototype.useHands = function(h)
  {
    return new Movement(this.fullbeats,h,
        this.cx1,this.cy1,this.cx2,this.cy2,this.x2,this.y2,
        this.cx3,this.cx4,this.cy4,this.x4,this.y4,this.beats)
  };

  Movement.prototype.clone = function()
  {
    return new Movement(
        this.fullbeats,
        this.hands,
        this.btranslate.ctrlx1,this.btranslate.ctrly1,
        this.btranslate.ctrlx2,this.btranslate.ctrly2,
        this.btranslate.x2,this.btranslate.y2,
        this.brotate.ctrlx1,
        this.brotate.ctrlx2,this.brotate.ctrly2,
        this.brotate.x2,this.brotate.y2,this.beats);
  };

  /**
   * Return a matrix for the translation part of this movement at time t
   * @param t  Time in beats
   * @return   Matrix for using with canvas
   */
  Movement.prototype.translate = function(t)
  {
    if (typeof t != 'number')
      t = this.beats;
    var tt = Math.min(Math.max(0,t),this.beats);
    return this.btranslate.translate(tt/this.fullbeats);
  };

  /**
   * Return a matrix for the rotation part of this movement at time t
   * @param t  Time in beats
   * @return   Matrix for using with canvas
   */
  Movement.prototype.rotate = function(t)
  {
    if (typeof t != 'number')
      t = this.beats;
    var tt = Math.min(Math.max(0,t),this.beats);
    return this.brotate.rotate(tt/this.fullbeats);
  };

  Movement.prototype.transform = function(t)
  {
    var tx = new AffineTransform();
    tx = tx.preConcatenate(this.translate(t));
    tx = tx.preConcatenate(this.rotate(t));
    return tx;
  };

  Movement.prototype.reflect = function()
  {
    return this.scale(1,-1);
  };

  /**
   * Return a new Movement scaled by x and y factors.
   * If y is negative hands are also switched.
   */
  Movement.prototype.scale = function(x,y)
  {
    return new Movement(this.beats,
        (y < 0 && this.hands == Movement.RIGHTHAND) ? Movement.LEFTHAND
          : (y < 0 && this.hands == Movement.LEFTHAND) ? Movement.RIGHTHAND
          : this.hands,  // what about GRIPLEFT, GRIPRIGHT?
        this.cx1*x,this.cy1*y,this.cx2*x,this.cy2*y,this.x2*x,this.y2*y,
        this.cx3*x,this.cx4*x,this.cy4*y,this.x4*x,this.y4*y,this.fullbeats)
  };

  /**
   * Return a new Movement with the end point shifted by x and y
   */
  Movement.prototype.skew = function(x,y) {
    if (this.beats < this.fullbeats)
      return this.skewClip(x,y);
    else
      return this.skewFull(x,y);
  };

  Movement.prototype.skewFull = function(x,y) {
    return new Movement(this.fullbeats,this.hands,this.cx1,this.cy1,
        this.cx2+x,this.cy2+y,this.x2+x,this.y2+y,
        this.cx3,this.cx4,this.cy4,this.x4,this.y4,this.beats)
  };


  Movement.prototype.clip = function(b) {
    if (b > 0 && b < this.fullbeats)
      return new Movement(this.fullbeats,this.hands,this.cx1,this.cy1,
          this.cx2,this.cy2,this.x2,this.y2,
          this.cx3,this.cx4,this.cy4,this.x4,this.y4,b)
    else
      return this;
  }

  /**
   *   Skew a movement that has been clipped, adjusting so the amount of
   *   skew is appplied to the clip point
   */
  Movement.prototype.skewClip = function(x,y) {
    var vdelta = new Vector(x,y);
    var vfinal = this.translate().location.add(vdelta);
    var m = this;
    var maxiter = 100;
    do {
      // Shift the end point by the current difference
      m = m.skewFull(vdelta.x,vdelta.y);
      // See how that affects the clip point
      var loc = m.translate().location;
      vdelta = vfinal.subtract(loc);
      maxiter -= 1;
    } while (vdelta.length > 0.001 && maxiter > 0);
    //  If timed out, return original rather than something that
    //  might put the dancers in outer space
    return maxiter > 0 ? m : this;
  };


  Movement.prototype.toString = function()
  {
    return this.btranslate.toString() + ' '+this.brotate.toString();
  };

  Movement.prototype.isStand = function() {
    return this.x2 == 0 && this.y2 == 0 && this.x4 == 0 && this.y4 == 0;
  }

  return Movement;

});
