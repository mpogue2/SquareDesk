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

//  Array class extensions
define(function() {

  var funcprop = {writable: true, enumerable: false};

  Array.prototype.first = function() {
    return this[0];
  };

  Array.prototype.last = function() {
    return this[this.length-1];
  };

  /**
   *  Takes a nested array and returns a non-nested array
   *  with the elements in depth-first order
   */
  Array.prototype.flatten = function() {
    return this.reduce(function(a1,a2) {
      return a1.concat(a2 instanceof Array ? a2.flatten() : a2);
    },[]);
  };

  /**
   *   Return a shallow copy
   */
  Array.prototype.copy = function() {
    return this.filter(function() { return true; });
  };

  /**
   *   Return a reversed copy
   *   The Array.prototype.reverse method reverses an array in place
   */
  Array.prototype.reversed = function() {
    var a = this.copy();
    a.reverse();
    return a;
  }

  Object.defineProperties(Array.prototype, {
    first: funcprop,
    last: funcprop,
    flatten: funcprop,
    copy: funcprop,
    reversed: funcprop
  });

  return Array;

});
