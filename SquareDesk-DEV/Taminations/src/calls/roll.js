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

define(['env','calls/quarter_turns'],function(Env,QuarterTurns) {
  var Roll = Env.extend(QuarterTurns);
  Roll.prototype.name = "and Roll";
  Roll.prototype.select = function(ctx,d) {
    //  Look at the last curve of the path
    var rollm = d.path.movelist.reversed().find(function(m) {
      return !m.isStand();
    });
    if (rollm) {
      var roll = rollm.brotate.rolling();
//    var roll = d.path.movelist.last().brotate.rolling();
      if (roll < -0.1)
        return 'Quarter Right';
      if (roll > 0.1)
        return 'Quarter Left';
    }
    return 'Stand';
  };
  return Roll;
});
