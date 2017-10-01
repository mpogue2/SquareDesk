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

define(['vector'],function(Vector) {

  //  AffineTransform class
  AffineTransform = function(tx)
  {
    if (arguments.length == 0) {
      //  default constructor - return the identity matrix
      this.x1 = 1.0;
      this.x2 = 0.0;
      this.x3 = 0.0;
      this.y1 = 0.0;
      this.y2 = 1.0;
      this.y3 = 0.0;
    }
    else if (tx instanceof AffineTransform) {
      //  return a copy
      this.x1 = tx.x1;
      this.x2 = tx.x2;
      this.x3 = tx.x3;
      this.y1 = tx.y1;
      this.y2 = tx.y2;
      this.y3 = tx.y3;
    }
    else if (tx instanceof Array) {
      //  Convert a 2-D array to an AffineTransform object
      this.x1 = tx[0][0];
      this.x2 = tx[0][1];
      this.x3 = tx[0][2] || 0;
      this.y1= tx[1][0];
      this.y2 = tx[1][1];
      this.y3 = tx[1][2] || 0;
    }
  };

  Object.defineProperties(AffineTransform.prototype, {
    scaleX: { get: function() { return this.x1; } },
    scaleY: { get: function() { return this.y2; } },
    shearX: { get: function() { return this.x2; } },
    shearY: { get: function() { return this.y1; } },
    translateX: { get: function() { return this.x3; } },
    translateY: { get: function() { return this.y3; } },
    location: { get: function() { return new Vector(this.x3,this.y3); } },
    angle: { get: function() { return Math.atan2(this.y1,this.y2); } }
  });

  //  Generate a new transform that moves to a new location
  AffineTransform.getTranslateInstance = function(x,y)
  {
    var a = new AffineTransform();
    a.x3 = x instanceof Vector ? x.x : x;
    a.y3 = x instanceof Vector ? x.y : y;
    return a;
  };

  //  Generate a new transform that does a rotation
  AffineTransform.getRotateInstance = function(theta)
  {
    var ab = new AffineTransform();
    if (theta) {
      ab.y1 = Math.sin(theta);
      ab.x2 = -ab.y1;
      ab.x1 = Math.cos(theta);
      ab.y2 = ab.x1;
    }
    return ab;
  };

  //  Generate a new transform that does a scaling
  AffineTransform.getScaleInstance = function(x,y)
  {
    var a = new AffineTransform();
    a.scale(x,y);
    return a;
  };

  //  Add a translation to this transform
  AffineTransform.prototype.translate = function(x,y)
  {
    this.x3 += x*this.x1 + y*this.x2;
    this.y3 += x*this.y1 + y*this.y2;
  };

  //  Add a scaling to this transform
  AffineTransform.prototype.scale = function(x,y)
  {
    this.x1 *= x;
    this.y1 *= x;
    this.x2 *= y;
    this.y2 *= y;
  };

  //  Add a rotation to this transform
  AffineTransform.prototype.rotate = function(angle)
  {
    var sin = Math.sin(angle);
    var cos = Math.cos(angle);
    var copy = new AffineTransform(this);
    //{ x1: this.x1, x2: this.x2, y1: this.y1, y2: this.y2 };
    this.x1 =  cos * copy.x1 + sin * copy.x2;
    this.x2 = -sin * copy.x1 + cos * copy.x2;
    this.y1 =  cos * copy.y1 + sin * copy.y2;
    this.y2 = -sin * copy.y1 + cos * copy.y2;
    return this;
  };

  AffineTransform.prototype.preConcatenate = function(tx)
  {
    // [result] = [this] x [Tx]
    var result = new AffineTransform();
    result.x1 = this.x1 * tx.x1 + this.x2 * tx.y1;
    result.x2 = this.x1 * tx.x2 + this.x2 * tx.y2;
    result.x3 = this.x1 * tx.x3 + this.x2 * tx.y3 + this.x3;
    result.y1 = this.y1 * tx.x1 + this.y2 * tx.y1;
    result.y2 = this.y1 * tx.x2 + this.y2 * tx.y2;
    result.y3 = this.y1 * tx.x3 + this.y2 * tx.y3 + this.y3;
    return result;
  };

  AffineTransform.prototype.concatenate = function(tx)
  {
    // [result] = [Tx] x [this]
    var result = new AffineTransform();
    result.x1 = tx.x1 * this.x1 + tx.x2 * this.y1;
    result.x2 = tx.x1 * this.x2 + tx.x2 * this.y2;
    result.x3 = tx.x1 * this.x3 + tx.x2 * this.y3 + tx.x3;
    result.y1 = tx.y1 * this.x1 + tx.y2 * this.y1;
    result.y2 = tx.y1 * this.x2 + tx.y2 * this.y2;
    result.y3 = tx.y1 * this.x3 + tx.y2 * this.y3 + tx.y3;
    return result;
  };

  //  Compute and return the inverse matrix - only for affine transform matrix
  AffineTransform.prototype.getInverse = function()
  {
    var inv = new AffineTransform();
    var det = this.x1*this.y2 - this.x2*this.y1;
    inv.x1 = this.y2/det;
    inv.y1 = -this.y1/det;
    inv.x2 = -this.x2/det;
    inv.y2 = this.x1/det;
    inv.x3 = (this.x2*this.y3 - this.y2*this.x3) / det;
    inv.y3 = (this.y1*this.x3 - this.x1*this.y3) / det;
    return inv;
  };

  //  Apply a transform to a Vector
  //  A bit of a hack, in that we are modifying the Vector class here in the
  //  AffineTransform class.  But this avoids a circular dependency.
  Vector.prototype.concatenate = function(tx)
  {
    var vx = AffineTransform.getTranslateInstance(this.x,this.y);
    vx = vx.concatenate(tx);
    return vx.location;
  };
  Vector.prototype.preConcatenate = function(tx)
  {
    var vx = AffineTransform.getTranslateInstance(this.x,this.y);
    vx = vx.preConcatenate(tx);
    return vx.location;
  };

  //  Return a string that can be used as the svg transform attribute
  AffineTransform.prototype.toString = function()
  {
    return 'matrix('+this.x1+','+this.y1+','+
    this.x2+','+this.y2+','+
    this.x3+','+this.y3+')';
  };

  //  A prettier version for debugging
  AffineTransform.prototype.print = function()
  {
    return '[ '+this.x1.toFixed(2)+' '+this.y1.toFixed(2)+' '+
    this.x2.toFixed(2)+' '+this.y2.toFixed(2)+' '+
    this.x3.toFixed(2)+' '+this.y3.toFixed(2)+' ]';
  };
  return AffineTransform;

});
