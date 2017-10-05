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

define(['env','calls/action','path'],function(Env,Action,Path) {
  var QuarterTurns = Env.extend(Action);
  QuarterTurns.prototype.performOne = function(d,ctx)
  {
    var offsetX = 0;
    var offsetY = 0;
    var move = this.select(ctx,d);
    if (move != 'Stand') {
      //  If leader or trailer, make sure to adjust quarter turn
      //  so handhold is possible
      if (d.leader) {
        var d2 = ctx.dancerInBack(d);
        var dist = ctx.distance(d,d2);
        if (dist > 2 && dist < 4.1)
          offsetX = -(dist-2)/2;
      }
      if (d.trailer) {
        var d2 = ctx.dancerInFront(d);
        var dist = ctx.distance(d,d2);
        if (dist > 2 && dist < 4.1)
          offsetX = (dist-2)/2;
      }
    }
    return new Path({select: move, offsetX: offsetX, offsetY: offsetY });
  };
  return QuarterTurns;
});
