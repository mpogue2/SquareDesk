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
  var Hinge = Env.extend(Action);
  Hinge.prototype.name = "Hinge";
  Hinge.prototype.performOne = function(d,ctx)
  {
    //  Find the dancer to hinge with
    var d2 = null;
    var d3 = ctx.dancerToRight(d);
    var d4 = ctx.dancerToLeft(d);
    if (d.partner && d.partner.active)
      d2 = d.partner;
    else if (d3 && d3.active)
      d2 = d3;
    else if (d4 && d4.active)
      d2 = d4;
    if (!d2)
      return undefined;
    //  TODO handle partner hinge
    var scalefactor = ctx.distance(d,d2)/2.0;
    if (ctx.isRight(d,d2))
      return TamUtils.getMove("Hinge Right").scale(scalefactor,scalefactor);
    else
      return TamUtils.getMove("Hinge Left").scale(scalefactor,scalefactor);
  }
  return Hinge;
});
