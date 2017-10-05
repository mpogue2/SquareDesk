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
"use strict";

define(['env','calls/action','calls/xmlcall'],
       function(Env,Action,XMLCall) {
  var Half = Env.extend(Action);
  Half.prototype.name = "Half";

  Half.prototype.perform = function(ctx,i) {

    if (i+1 < ctx.callstack.length) {
      //  Steal the next call off the stack
      this.call = ctx.callstack[i+1];
  
      //  For XML calls there should be an explicit number of parts
      //  Or assume the call is simple enought that 1/2 is half the beats
      this.halfbeats = 0;
      if (this.call instanceof XMLCall) {
        //  Figure out how many beats are in half the call
        var parts = $(this.call.xelem).attr('parts');
        if (parts != undefined) {
          var partnums = parts.split(';');
          partnums.slice(0,(partnums.length+1)/2).forEach(function(b) {
            this.halfbeats += Number(b);
          },this);
        }
      }
      this.prevbeats = ctx.maxBeats();
    }
  };

  //  Call is performed between these two methods

  Half.prototype.postProcess = function(ctx,i) {
    //  Coded calls so far do not have explicit parts
    //  so just divide them in two
    if (this.call instanceof Action || this.halfbeats == 0)
      this.halfbeats = (ctx.maxBeats() - this.prevbeats) / 2;

    //  Chop off the excess half
    ctx.dancers.forEach(function(d) {
      var m = 0;
      while (d.path.beats() > this.prevbeats + this.halfbeats)
        m = d.path.pop();
      if (m && d.path.beats() < this.prevbeats + this.halfbeats) {
        m = m.clip(this.prevbeats + this.halfbeats - d.path.beats());
        d.path.add(m);
      }
    },this);

  };

  return Half;
});
