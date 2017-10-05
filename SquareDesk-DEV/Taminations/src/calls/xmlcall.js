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

define(['env','calls/call','path'],
       function(Env,Call,Path) {
  var XMLCall = Env.extend(Call);
  XMLCall.prototype.name = '';

  XMLCall.prototype.performCall = function(ctx) {

    var allp = tam.getPath(this.xelem);
    //  If moving just some of the dancers,
    //  see if we can keep them in the same shape
    if (ctx.actives.length < ctx.dancers.length) {
      //  No animations have been done on ctx2, so dancers are still at the start points
      var ctx3 = this.ctx2.clone();
      //  So ctx3 is a copy of the start point
      //  Now add the paths
      ctx3.dancers.forEach(function(d,i) {
        d.path.add(new Path(allp[i>>1]));
      });
      //  And move it to the end point
      ctx3.analyze();
    }

    //  Once a mapping of the current formation to an XML call is found,
    //  we need to compute the difference between the two,
    //  and that difference will be added as an offset to the first movement
    var vdif = ctx.computeFormationOffsets(this.ctx2,this.xmlmap);
    this.xmlmap.forEach(function(m,i3) {
      var p = new Path(allp[m>>1]);
      //  Compute difference between current formation and XML formation
      var vd = vdif[i3].rotate(-ctx.actives[i3].tx.angle);
      //  Apply formation difference to first movement of XML path
      if (vd.length > 0.1 && p.movelist.length > -1) {
        if (p.movelist.length == 0)
          p = TamUtils.getMove("Stand");
        p.movelist.unshift(p.movelist.shift().skew(-vd.x,-vd.y));
      }
      //  Add XML path to dancer
      ctx.actives[i3].path.add(p);
      //  Move dancer to end so any subsequent modifications (e.g. roll)
      //  use the new position
      ctx.actives[i3].animateToEnd();
    },this);

    ctx.levelBeats();
    ctx.analyze();

  };

  XMLCall.prototype.postProcess = function(ctx,index) {
    //  If just this one call then assume it knows what
    //  the ending formation should be
    Call.prototype.postProcess.call(this,ctx,index);
    if (ctx.callstack.length > 1)
      ctx.matchStandardFormation()
  }


  return XMLCall;

});
