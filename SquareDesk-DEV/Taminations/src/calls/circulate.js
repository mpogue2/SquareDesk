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

define(['env','calls/action','callcontext','callerror'],function(Env,Action,CallContext,CallError) {

  var Circulate = Env.extend(Action);
  Circulate.prototype.name = "Circulate";

  Circulate.prototype.perform = function(ctx) {
    //  If just 4 dancers, try Box Circulate
    if (ctx.actives.length == 4) {
      try {
        ctx.applyCalls('box circulate');
      } catch (err) {
        if (err instanceof CallError) {
          //  That didn't work, try to find a circulate path for each dancer
          Action.prototype.perform.call(this,ctx);
        } else
          throw err;
      }
    }
    //  If in waves or lines, then do All 8 Circulate
    else if (ctx.isLines())
      ctx.applyCalls('all 8 circulate');
    //  If in columns, do Column Circulate
    else if (ctx.isColumns())
      ctx.applyCalls('column circulate');
    //  Otherwise ... ???
    else
      throw new CallError('Cannot figure out how to Circulate.');
  };

  Circulate.prototype.performOne = function(d,ctx) {
    if (d.leader) {
      //  Find another active dancer in the same line and move to that spot
      var d2 = ctx.dancerClosest(d,function(dx) {
        return dx.active && (ctx.isRight(d,dx) || ctx.isLeft(d,dx)) } );
      if (d2 != null) {
        var dist = ctx.distance(d,d2);
        return TamUtils.getMove(ctx.isRight(d,d2) ? 'Run Right' : 'Run Left')
                       .scale(dist/3,dist/2).changebeats(4);
      }
    } else if (d.trailer) {
      //  Looking at active leader?  Then take its place
      //  TODO maybe allow diagonal circulate?
      var d2 = ctx.dancerInFront(d);
      if (d2 != null && d2.active) {
        var dist = ctx.distance(d,d2);
        return TamUtils.getMove('Forward').scale(dist,1).changebeats(4);
      }
    }
    throw new CallError('Cannot figure out how to Circulate.');
  }



  Circulate.requiresxml = ['All 8 Circulate'];
  return Circulate;
});
