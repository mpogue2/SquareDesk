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

define(['env','calls/action','movement','path'],function(Env,Action,Movement,Path) {
  var BoxCounterRotate = Env.extend(Action);
  BoxCounterRotate.prototype.name = "Box Counter Rotate";
  BoxCounterRotate.prototype.performOne = function(d,ctx)
  {
    var v = d.location;
    var v2 = v;
    var cy4, y4;
    var a1 = d.angle*Math.PI/180;
    var a2 = v.angle;
    //  Determine if this is a rotate left or right
    var angdif = Math.angleDiff(a2,a1);
    if (angdif < 0) {
      //  Left
      v2 = v.rotate(Math.PI/2)
      cy4 = 0.45;
      y4 = 1;
    }
    else {
      //  Right
      v2 = v.rotate(-Math.PI/2)
      cy4 = -0.45;
      y4 = -1;
    }
    //  Compute the control points
    var dv = v2.subtract(v).rotate(-a1);
    var cv1 = v2.scale(.5).rotate(-a1);
    var cv2 = v.scale(.5).rotate(-a1).add(dv);
    var m = new Movement(
        2.0,Movement.NOHHANDS,
        cv1.x,cv1.y,cv2.x,cv2.y,dv.x,dv.y,
        0.55, 1, cy4, 1, y4
        );
    return new Path(m);
  };
  return BoxCounterRotate;
});
