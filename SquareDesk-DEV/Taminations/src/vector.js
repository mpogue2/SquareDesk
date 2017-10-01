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

define(function() {

  //  Vector class
  Vector = function(x,y,z)
  {
    if (arguments.length > 0 && x instanceof Vector) {
      this.x = x.x;
      this.y = x.y;
      this.z = x.z;
    } else {
      this.x = arguments.length > 0 ? x : 0;
      this.y = arguments.length > 1 ? y : 0;
      this.z = arguments.length > 2 ? z : 0;
    }
  };

  Object.defineProperties(Vector.prototype, {
    //  Return angle of vector from the origin
    angle: { get: function() {
      return Math.atan2(this.y,this.x);
    }},
    //  Return distance from origin
    length: { get: function() {
      return Math.sqrt(this.x*this.x+this.y*this.y+this.z*this.z);
    }},
  });

  //  Add/subtract two vectors
  Vector.prototype.add = function(v)
  {
    return new Vector(this.x+v.x,this.y+v.y,this.z+v.z);
  };

  Vector.prototype.subtract = function(v)
  {
    return new Vector(this.x-v.x,this.y-v.y,this.z-v.z);
  };

  Vector.prototype.scale = function(sx,sy,sz)
  {
    if (sy == undefined)
      sy = sx;
    if (sz == undefined)
      sz = sy;
    return new Vector(this.x*sx,this.y*sy,this.z*sz);
  }

  //  Compute the cross product
  Vector.prototype.cross = function(v)
  {
    return new Vector(
        this.y*v.z - this.z*v.y,
        this.z*v.x - this.x*v.z,
        this.x*v.y - this.y*v.x
    );
  };

  //  Rotate by a given angle
  Vector.prototype.rotate = function(th)
  {
    var d = Math.sqrt(this.x*this.x+this.y*this.y);
    var a = this.angle + th;
    return new Vector(
        d * Math.cos(a),
        d * Math.sin(a),
        this.z);
  };

  //  Return true if this vector followed by vector 2 is clockwise
  Vector.prototype.isCW = function(v)
  {
    return this.cross(v).z < 0;
  };

  Vector.prototype.isCCW = function(v)
  {
    return this.cross(v).z > 0;
  };

  //  Return true if the vector from this to v points left of the origin
  Vector.prototype.isLeft = function(v)
  {
    var v1 = new Vector().subtract(this);
    var v2 = new Vector(v).subtract(this);
    return v1.isCCW(v2);
  };

  Vector.prototype.toString = function()
  {
    return "("+Math.round(this.x*10)/10+","+Math.round(this.y*10)/10+")";
  };

  return Vector;
});
