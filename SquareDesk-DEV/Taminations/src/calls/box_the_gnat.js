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

define(['env','calls/action','movement','path'],
       function(Env,Action,Movement,Path) {
  var BoxTheGnat = Env.extend(Action);
  BoxTheGnat.prototype.name = "Box the Gnat";
  BoxTheGnat.prototype.performOne = function(d,ctx) {
    var d2 = ctx.dancerInFront(d);
    var dist = ctx.distance(d,d2);
    var cy1 = d.gender == Dancer.BOY ? 1 : .1;
    var y4 = d.gender == Dancer.BOY ? -2 : 2;
    var hands = d.gender == Dancer.BOY ? "gripleft" : "gripright";
    var m = new Movement(
        4.0, hands,
        1, cy1, dist/2, cy1, dist/2+1, 0,     1.3, 1.3, y4, 0, y4
        );
    return new Path(m);
  }
  return BoxTheGnat;
});
