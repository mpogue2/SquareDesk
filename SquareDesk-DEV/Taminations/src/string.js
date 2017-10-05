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

//  Extend String with some useful stuff
/**
 *   Return string with first letter of each word capitalized
 */
define(function() {
  var funcprop = {writable: true, enumerable: false};
  String.prototype.toCapCase = function()
  {
    return this.replace(/\b\w+\b/g, function(word) {
      return word.substring(0,1).toUpperCase() +
      word.substring(1).toLowerCase();
    });
  };
  /** Remove leading and trailing whitespace  */
  String.prototype.trim = function () {
    return this.replace(/^\s+|\s+$/g, "");
  };
  /**  Remove all spaces  */
  String.prototype.collapse = function() {
    return this.replace(/\s+/g,'');
  };
  /**  Remove all non-alphanumerics   */
  String.prototype.alphanums = function() {
    return this.replace(/\W/g,'');
  };
  /**  Capitalize every word and remove all spaces  */
  String.prototype.toCamelCase = function() {
    return this.toCapCase().collapse();
  };

  //String extension to help with parsing
  //Returns an array of strings, starting with the entire string,
  //and each subsequent string chopping one word off the end
  String.prototype.chopped = function() {
    var ss = [];
    return this.split(/\s+/).map(function(s) {
      ss.push(s);
      return ss.join(' ');
    }).reverse();
  };

  //Return an array of strings, each removing one word from the start
  String.prototype.diced = function() {
    var ss = [];
    return this.split(/\s+/).reverse().map(function(s) {
      ss.unshift(s);
      return ss.join(' ');
    }).reverse();
  };

  /**
  *   Return all combinations of words from a string
  */
  String.prototype.minced = function() {
    return this.chopped().map(function(s) {
      return s.diced();
    }).flatten();
  };

  /**  Parse parameters out of a search string  */
  String.prototype.toArgs = function() {
    return this.split(/\&/).reduce(function(args,a) {
      var v = true;
      var b = a.split(/=/);
      if (b.length > 1) {
        a = b[0];
        v = b[1];
        if (v.match(/n|no|false|0/i))
          v = false;
      }
      a = a.toLowerCase().replace(/\W/g,"");
      args[a] = v;
      return args;
    },{});
  };

  var entityMap = {
      '&': '&amp;',
      '<': '&lt;',
      '>': '&gt;',
      '"': '&quot;',
      "'": '&#39;',
      '/': '&#x2F;',
      '`': '&#x60;',
      '=': '&#x3D;'
    };

    String.prototype.escapeHtml = function() {
      return this.replace(/[&<>"'`=\/]/g, function (s) {
        return entityMap[s];
      });
    }  
  
  /**  Apply an extension to a filename, replacing any previous extension */
  String.prototype.extension = function(ext) {
    // remove any existing extension
    var f = this.replace(/\.[^\/.]*$/,'');
    // add the extension, make sure with just one dot
    return f + '.' + ext.replace(/^\./,'');
  };

  Object.defineProperties(String.prototype, {
    toCapCase: funcprop,
    trim: funcprop,
    collapse: funcprop,
    alphanums: funcprop,
    toCamelCase: funcprop,
    toArgs: funcprop,
    escapeHTML : funcprop
  });
  return String;
});
