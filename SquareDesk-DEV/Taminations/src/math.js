/*

    Copyright 2017 Brad Christie

    This file is part of TAMinations.

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
"use strict";

//  Math class extensions
define(function() {

  var funcprop = {writable: true, enumerable: false};

  Math.toRadians = function(deg) {
    return deg * Math.PI / 180;
  };

  Math.toDegrees = function(rad) {
    return rad * 180 / Math.PI;
  };

  Math.IEEEremainder = function(d1,d2) {
    var n = Math.round(d1/d2);
    return d1 - n*d2;
  };

  Math.isApprox = function(a,b,delta) {
    delta = delta || 0.1;
    return Math.abs(a-b) < delta;
  };

  Math.angleDiff = function(a1,a2) {
    return ((((a1-a2) % (Math.PI*2)) + (Math.PI*3)) % (Math.PI*2)) - Math.PI;
  };

  Math.anglesEqual = function(a1,a2) {
    return Math.isApprox(Math.angleDiff(a1,a2),0);
  };

  Math.sign = function(a) {
    return a < 0 ? -1 : a > 0 ? 1 : 0;
  }
  Object.defineProperties(Number.prototype, {
    abs: { get: function() { return Math.abs(this); }},
    sin: { get: function() { return Math.sin(this); }},
    cos: { get: function() { return Math.cos(this); }},
    floor: { get: function() { return Math.floor(this); }},
    ceil: { get: function() { return Math.ceil(this); }},
    sqrt: { get: function() { return Math.sqrt(this); }},
    sq: { get: function() { return this * this; }},
    sign: { get: function() { return this < 0 ? -1 : this > 0 ? 1 : 0; }},
    toRadians: { get: function() { return Math.toRadians(this); }},
    toDegrees: { get: function() { return Math.toDegrees(this); }},
  });


  //  SVD simple and fast for 2x2 arrays
  //  for matching 2d formations
  Math.svd22 = function svd(A) {
    var a = A[0][0];
    var b = A[0][1];
    var c = A[1][0];
    var d = A[1][1];
    //  Check for trivial case
    var epsilon = 0.0001;
    if (b.abs < epsilon && c.abs < epsilon) {
      var V = [[ (a < 0.0) ? -1.0 : 1.0, 0.0],
               [0.0, (d < 0.0) ? -1.0 : 1.0]];
      var Sigma = [a.abs,d.abs];
      var U = [[1.0,0.0],[0.0,1.0]];
      return {U:U,S:Sigma,V:V};
    } else {
      var j = a.sq + b.sq;
      var k = c.sq + d.sq;
      var vc = a*c + b*d;
      //  Check to see if A^T*A is diagonal
      if (vc.abs < epsilon) {
        var s1 = j.sqrt;
        var s2 = ((j-k).abs < epsilon) ? s1 : k.sqrt;
        var Sigma = [s1,s2];
        var V = [[1.0,0.0],[0.0,1.0]];
        var U = [[a/s1,b/s1],[c/s2,d/s2]];
        return {U:U,S:Sigma,V:V};
      } else {   //  Otherwise, solve quadratic for eigenvalues
        var atanarg1 = 2 * a * c + 2 * b * d;
        var atanarg2 = a * a + b * b - c * c - d * d;
        var Theta = 0.5 * Math.atan2(atanarg1,atanarg2);
        var U = [[Theta.cos, -Theta.sin],
                 [Theta.sin, Theta.cos]];

        var Phi = 0.5 * Math.atan2(2 * a * b + 2 * c * d, a.sq - b.sq + c.sq - d.sq);
        var s11 = (a * Theta.cos + c * Theta.sin) * Phi.cos +
                  (b * Theta.cos + d * Theta.sin) * Phi.sin;
        var s22 = (a * Theta.sin - c * Theta.cos) * Phi.sin +
                  (-b * Theta.sin + d * Theta.cos) * Phi.cos;

        var S1 = a.sq + b.sq + c.sq + d.sq;
        var S2 = ((a.sq + b.sq - c.sq - d.sq).sq + 4 * (a * c + b * d).sq).sqrt;
        var Sigma = [(S1 + S2).sqrt / 2, (S1 - S2).sqrt / 2];

        var V = [[s11.sign * Phi.cos, -s22.sign * Phi.sin],
                 [s11.sign * Phi.sin, s22.sign * Phi.cos]];
        return {U:U,S:Sigma,V:V};
      }
    }
  }


  //  A few routines adapted from numeric.js
  Math.transposeArray = function(x) {
    var m = x.length;
    var n = x[0].length;
    var ret = Array(n);
    for (var j=0;j<n;j++) {
      ret[j] = Array(m);
      for (var i=0; i<m; i++)
        ret[j][i] = x[i][j];
    }
    return ret;
  };

  Math.cloneArray = function(x) {
    var m = x.length;
    var n = x[0].length;
    var ret = Array(m);
    for (var i=0; i<m; i++) {
      ret[i] = Array(n);
      for (var j=0; j<n; j++)
        ret[i][j] = x[i][j];
    }
    return ret;
  };


  Object.defineProperties(Math,{
    toRadians: funcprop,
    IEEEremainder: funcprop,
    isApprox: funcprop,
    angleDiff: funcprop,
    cloneArray: funcprop,
    transposeArray: funcprop,
    repArray: funcprop,
    svd: funcprop
  });



  return Math;

});
