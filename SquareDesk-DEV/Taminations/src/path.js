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

define(['movement','affinetransform'],function(Movement,AffineTransform) {

  //  Path class
  Path = Env.extend(null,function(p)
  {
    this.movelist = [];
    this.transformlist = [];
    if (p instanceof Path) {
      p.movelist.forEach(function(m) {
        this.add(m.clone());
      },this);
    }
    else if (p && (p.select != undefined)) {
      TamUtils.translateMove(p).forEach(function(m) {
        this.add(m);
      },this);
    }
    else if (p instanceof Movement) {
      this.add(p.clone());
    }
    else if (p) {
      p.forEach(function(m) {
        if (m instanceof Movement)
          this.add(m);
        else if (m.cx1 != undefined)
          this.add(new Movement(m));
        else
          this.add(new Path(m));
      },this);
    }
  });

  Path.prototype.clear = function()
  {
    this.movelist = [];
    this.transformlist = [];
  };

  Path.prototype.recalculate = function()
  {
    this.transformlist = [];
    var tx = new AffineTransform();
    this.movelist.forEach(function(m) {
      tx = tx.preConcatenate(m.translate());
      tx = tx.preConcatenate(m.rotate());
      this.transformlist.push(new AffineTransform(tx));
    },this);
  };

  //  Return total number of beats in path
  Path.prototype.beats = function()
  {
    if (this.movelist != null)
      return this.movelist.reduce(function(s,m) {
        return s + m.beats;
      },0.0);
    return 0.0;
  };

  //  Make the path run slower or faster to complete in a given number of beats
  Path.prototype.changebeats = function(newbeats)
  {
    if (this.movelist != null) {
      var factor = newbeats/this.beats();
      this.movelist.forEach(function(m,i) {
        this.movelist[i] = m.time(m.beats*factor);
      },this);
    }
    return this;
  };

  //  Change hand usage
  Path.prototype.changehands = function(hands)
  {
    if (this.movelist != null) {
      this.movelist.forEach(function(m,i) {
        this.movelist[i] = m.useHands(hands);
      },this);
    }
    return this;
  };

  //  Change the path by scale factors
  Path.prototype.scale = function(x,y)
  {
    if (this.movelist != null) {
      this.movelist.forEach(function(m,i) {
        this.movelist[i] = m.scale(x,y);
      },this);
    }
    this.recalculate();
    return this;
  };

  //  Skew the path by translating the destination point
  Path.prototype.skew = function(x,y)
  {
    if (this.movelist != null) {
      this.movelist.push(this.movelist.pop().skew(x,y));
      this.recalculate();
    }
    return this;
  };

  //  Append one movement or all the movements of another path
  //  to the end of the Path
  Path.prototype.add = function(m)
  {
    if (m instanceof Movement)
      this.movelist.push(m);
    if (m instanceof Path)
      this.movelist = this.movelist.concat(m.movelist);
    this.recalculate();
    return this;
  };

  //  Remove and return the last movement of the path
  Path.prototype.pop = function() {
    var m = this.movelist.pop();
    this.recalculate();
    return m;
  }

  //  Reflect the path about the x-axis
  Path.prototype.reflect = function()
  {
    this.movelist.forEach(function(m,i) {
      this.movelist[i] = m.reflect();
    },this);
    this.recalculate();
    return this;
  };

  return Path;

});
