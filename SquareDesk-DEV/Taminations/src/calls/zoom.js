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

define(['env','calls/action','path','callerror'],
       function(Env,Action,Path,CallError) {
  var Zoom = Env.extend(Action);
  Zoom.prototype.name = 'Zoom';
  Zoom.prototype.performOne = function(d,ctx)
  {
    var retval = new Path();
    if (d.leader) {
      var d2 = ctx.dancerInBack(d);
      if (!d2.trailer)
        throw new CallError("Dancer "+d+' is not in a tandem.')
      var a = ctx.angle(d);
      var c = a < 0 ? 'Run Left' : 'Run Right';
      if (!d2.active)
        throw new CallError('Trailer of dancer '+d+' is not active.');
      var dist = ctx.distance(d,d2);
      retval.add(TamUtils.getMove(c).changebeats(2).skew(-dist/2,0));
      retval.add(TamUtils.getMove(c).changebeats(2).skew(dist/2,0));
    } else if (d.trailer) {
      var d2 = ctx.dancerInFront(d);
      if (!d2.leader)
        throw new CallError("Dancer "+d+' is not in a tandem.')
      if (!d2.active)
        throw new CallError('Leader of dancer '+d+' is not active.');
      var dist = ctx.distance(d,d2);
      retval.add(TamUtils.getMove("Forward").changebeats(4).scale(dist,1));
    } else {
      throw new CallError('Dancer '+d+' cannot Zoom.');
    }
    return retval;
  };
  return Zoom;
});
