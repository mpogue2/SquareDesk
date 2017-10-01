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

define(['path','movement','vector','affinetransform','color'],
    function(Path,Movement,Vector,AffineTransform,Color) {

  //  Dancer class
  Dancer = function(args)   // (tamsvg,sex,x,y,angle,color,p,number,couplesnumber)
  {
    this.startangle = 0;      //  startangle is in degrees, not radians
    this.path = new Path();
    for (var arg in args) {
      if (arg == 'dancer') {  //  Copying another dancer
        //  Initialize this new dancer at the other dancer's current location
        //  but with an empty path
        ['tamsvg','fillcolor','drawcolor',
         'gender','number','couplesnumber'].forEach(function(p) {
          this[p] = args.dancer[p];
        },this);
        var loc = args.dancer.location;
        this.startx = loc.x;
        this.starty = loc.y;
        this.startangle = Math.toDegrees(args.dancer.tx.angle);
        this.clonedFrom = args.dancer;
      }
      //  Get additional optional args
      else if (arg == 'x')
        this.startx = args.x;
      else if (arg == 'y')
        this.starty = -args.y;
      else if (arg == 'angle') {
        if (args.dancer)
          this.startangle += args.angle;
        else
          this.startangle = args.angle-90;
      }
      //  If a path is given, clone it, so the original does not
      //  get clobbered if we make modifications
      else if (arg == 'path')
        this.path = new Path(args.path);
      else if (args[arg] != undefined)
        this[arg] = args[arg];
    }
    if (this.gender == Dancer.PHANTOM) {
      this.fillcolor = Color.gray;
      this.drawcolor = this.fillcolor.darker();
    }
    else if (args.color !== undefined) {
      this.fillcolor = args.color;
      this.drawcolor = args.color.darker();
    }

    this.hidden = false;
    this.pathVisible = this.tamsvg.showPaths;
    this.leftgrip = null;
    this.rightgrip = null;
    this.rightHandVisibility = false;
    this.leftHandVisibility = false;
    this.rightHandTransform = new AffineTransform();
    this.leftHandTransform = new AffineTransform();
    this.prevangle = 0;
    this.computeStart();
    if (this.computeOnly)  // temp dancer used for compiling sequences
      return;
    //  Create SVG representation
    this.svg = this.tamsvg.svg.group(this.tamsvg.dancegroup);
    var dancer = this;
    $(this.svg).mousedown(function(ev) {
      if (ev.altKey) {
        if (dancer.pathVisible)
          dancer.hidePath();
        else
          dancer.showBezier();
      }
      else if (ev.shiftKey) {
        if (dancer.tamsvg.barstool == dancer)
          dancer.tamsvg.barstool = 0;
        else
          dancer.tamsvg.barstool = dancer;
        dancer.tamsvg.paint();
      }
      else if (ev.ctrlKey) {
        if (dancer.tamsvg.compass == dancer)
          dancer.tamsvg.compass = 0;
        else
          dancer.tamsvg.compass = dancer;
        dancer.tamsvg.paint();
      }
      else {
        if (dancer.pathVisible)
          dancer.hidePath();
        else
          dancer.showPath();
      }
      ev.preventDefault();
      ev.stopPropagation();
      return false;
    });
    //  handholds
    this.lefthand = this.tamsvg.svg.group(this.tamsvg.handholds,{visibility:'hidden'});
    this.tamsvg.svg.circle(this.lefthand,0,1,1/8,{fill:Color.orange.toString()});
    this.tamsvg.svg.line(this.lefthand,0,0,0,1,{stroke:Color.orange.toString(),'stroke-width':0.05});
    this.righthand = this.tamsvg.svg.group(this.tamsvg.handholds,{visibility:'hidden'});
    this.tamsvg.svg.circle(this.righthand,0,-1,1/8,{fill:Color.orange.toString()});
    this.tamsvg.svg.line(this.righthand,0,0,0,-1,{stroke:Color.orange.toString(),'stroke-width':0.05});
    //  body
    //  Workaround for Safari bug that strokes the circle as a square
    //  - use larger values and scale it down
    this.tamsvg.svg.circle(this.svg,.5,0,1/3,{fill:this.drawcolor.toString()});
    if (this.gender == Dancer.GIRL)
      this.body = this.tamsvg.svg.circle(this.svg,0,0,5,
          {fill:this.fillcolor.toString(),
           stroke:this.drawcolor.toString(),'stroke-width':1,transform:"scale(0.1 0.1)"});
    else if (this.gender == Dancer.BOY)
      this.body = this.tamsvg.svg.rect(this.svg,-.5,-.5,1,1,
          {fill:this.fillcolor.toString(),
        stroke:this.drawcolor.toString(),'stroke-width':0.1});
    if (this.gender == Dancer.PHANTOM)
      this.body = this.tamsvg.svg.rect(this.svg,-.5,-.5,1,1,.3,.3,      // with rounded corners
          {fill:this.fillcolor.toString(),
        stroke:this.drawcolor.toString(),'stroke-width':0.1});
    else if (this.tamsvg.numbers || this.tamsvg.couples)
      this.body.setAttribute('fill',this.fillcolor.veryBright().toString());
    this.numbersvg = this.tamsvg.svg.text(this.svg,-4,5,this.number,{fontSize: "14",transform:"scale(0.04 -0.04)"});
    this.couplessvg = this.tamsvg.svg.text(this.svg,-4,5,this.couplesnumber,{fontSize: "14",transform:"scale(0.04 -0.04)"});
    if (!this.tamsvg.numbers)
      this.numbersvg.setAttribute('visibility','hidden');
    if (!this.tamsvg.couples)
      this.couplessvg.setAttribute('visibility','hidden');
    //  path
    this.pathgroup = this.tamsvg.svg.group(this.tamsvg.pathparent);
    this.beziergroup = this.tamsvg.svg.group(this.tamsvg.pathparent);
    this.paintPath();
    if (args.hidden != undefined && args.hidden)
      this.hide();
  };

  Dancer.BOY = 1;
  Dancer.GIRL = 2;
  Dancer.PHANTOM = 3;
  Dancer.genders =
  { 'boy':Dancer.BOY, 'girl':Dancer.GIRL, 'phantom':Dancer.PHANTOM };

  Object.defineProperties(Dancer.prototype, {
    location: { get: function() {
      return this.tx.location;
    }},
    //  Return distance from center
    distance: { get: function() {
      return this.tx.location.length;
    }},
    //  Return angle from dancer's facing direction to center
    centerAngle: { get: function() {
      return this.tx.angle + this.location.angle;
    }}
  });

  Dancer.prototype.rotateStartAngle = function(angle) {
    this.startangle += angle;
    this.computeStart();
  }

  Dancer.prototype.hidePath = function()
  {
    this.pathgroup.setAttribute('visibility','hidden');
    this.beziergroup.setAttribute('visibility','hidden');
    this.pathVisible = false;
  };

  Dancer.prototype.showPath = function()
  {
    this.pathgroup.setAttribute('visibility','visible');
    this.beziergroup.setAttribute('visibility','hidden');
    this.pathVisible = true;
  };

  Dancer.prototype.showBezier = function()
  {
    this.pathgroup.setAttribute('visibility','visible');
    this.beziergroup.setAttribute('visibility','visible');
    this.pathVisible = true;
  };

  Dancer.prototype.hide = function()
  {
    this.hidden = true;
    this.svg.setAttribute('visibility','hidden');
    this.lefthand.setAttribute('visibility','hidden');
    this.righthand.setAttribute('visibility','hidden');
    this.pathgroup.setAttribute('visibility','hidden');
    this.beziergroup.setAttribute('visibility','hidden');
    this.numbersvg.setAttribute('visibility','hidden');
    this.couplessvg.setAttribute('visibility','hidden');
  };

  Dancer.prototype.show = function()
  {
    this.hidden = false;
    this.svg.setAttribute('visibility','visible');
    if (this.leftHandVisibility)
      this.lefthand.setAttribute('visibility','visible');
    if (this.rightHandVisibility)
      this.righthand.setAttribute('visibility','visible');
    this.pathgroup.setAttribute('visibility','inherit');
    if (this.tamsvg.numbers)
      this.numbersvg.setAttribute('visibility','visible');
    if (this.tamsvg.couples)
      this.couplessvg.setAttribute('visibility','visible');
  };

  Dancer.prototype.computeStart = function()
  {
    this.start = new AffineTransform();
    this.start.translate(this.startx,this.starty);
    this.start.rotate(Math.toRadians(this.startangle));
    this.tx = new AffineTransform(this.start);
    if (this.svg)
      this.svg.setAttribute('transform',this.start.toString());
  };

  Dancer.prototype.beats = function()
  {
    return this.path.beats();
  };

  Dancer.prototype.showNumber = function()
  {
    if (this.gender != Dancer.PHANTOM) {
      this.body.setAttribute('fill',this.fillcolor.veryBright().toString());
      if (!this.hidden) {
        this.couplessvg.setAttribute('visibility','hidden');
        this.numbersvg.setAttribute('visibility','visible');
        this.paint();
      }
    }
  };

  Dancer.prototype.showCouplesNumber = function()
  {
    if (this.gender != Dancer.PHANTOM) {
      this.body.setAttribute('fill',this.fillcolor.veryBright().toString());
      if (!this.hidden) {
        this.numbersvg.setAttribute('visibility','hidden');
        this.couplessvg.setAttribute('visibility','visible');
        this.paint();
      }
    }
  };

  Dancer.prototype.hideNumbers = function()
  {
    if (this.gender != Dancer.PHANTOM) {
      this.body.setAttribute('fill',this.fillcolor.toString());
      this.numbersvg.setAttribute('visibility','hidden');
      this.couplessvg.setAttribute('visibility','hidden');
    }
  };

  Dancer.prototype.recalculate = function()
  {
    this.path.recalculate();
    if (!this.computeOnly)
      this.paintPath();
  };

  //  Return distance to another dancer
  Dancer.prototype.distanceTo = function(d2)
  {
    var loc1 = this.location;
    var loc2 = d2.location;
    return Math.sqrt((loc1.x-loc2.x)*(loc1.x-loc2.x) + (loc1.y-loc2.y)*(loc1.y-loc2.y));
  };

  Dancer.prototype.concatenate = function(tx2)
  {
    this.tx = this.tx.concatenate(tx2);
    this.svg.setAttribute('transform',tx2.toString());
  };

  Dancer.prototype.paintPath = function()
  {
    this.paintBezier();
    this.tamsvg.svg.remove(this.pathgroup);
    this.pathgroup = this.tamsvg.svg.group(this.tamsvg.pathparent);
    var points=[];
    for (var b=0; b<this.beats(); b+=0.1) {
      this.animate(b);
      if (this.tamsvg.hexagon)
        this.hexagonify(b);
      if (this.tamsvg.bigon)
        this.bigonify(b);
      points.push([this.tx.translateX,this.tx.translateY]);
    }
    this.tamsvg.svg.polyline(this.pathgroup,points,
        {fill:'none',stroke:this.drawcolor.toString(),strokeWidth:0.1,strokeOpacity:.3});
  };

  Dancer.prototype.paintBezier = function()
  {
    var ff = function(x,y,t) {
      var v = (new Vector(x,y)).concatenate(t);
      return [v.x,v.y];
    };
    this.tamsvg.svg.remove(this.beziergroup);
    this.beziergroup = this.tamsvg.svg.group(this.tamsvg.pathparent);
    //  Bezier is only visible if user alt-clicks a dancer
    this.beziergroup.setAttribute('visibility','hidden');
    var points=[];
    var t = this.start;
    this.path.movelist.forEach(function(m,i) {
      var pt = ff(0,0,t);
      points.push(pt);
      this.tamsvg.svg.circle(this.beziergroup,pt[0],pt[1],0.2,{fill:this.drawcolor.toString()});
      pt = ff(m.btranslate.ctrlx1,m.btranslate.ctrly1,t);
      points.push(pt);
      this.tamsvg.svg.circle(this.beziergroup,pt[0],pt[1],0.2,{fill:this.drawcolor.toString(),fillOpacity:.3});
      pt = ff(m.btranslate.ctrlx2,m.btranslate.ctrly2,t);
      points.push(pt);
      this.tamsvg.svg.circle(this.beziergroup,pt[0],pt[1],0.2,{fill:this.drawcolor.toString(),fillOpacity:.3});
      pt = ff(m.btranslate.x2,m.btranslate.y2,t);
      points.push(pt);
      this.tamsvg.svg.circle(this.beziergroup,pt[0],pt[1],0.2,{fill:this.drawcolor.toString()});
      t = new AffineTransform(this.start);
      t = t.preConcatenate(this.path.transformlist[i]);
    },this);
    this.tamsvg.svg.polyline(this.beziergroup,points,
        {fill:'none',stroke:'black',strokeWidth:0.1,strokeOpacity:.3});
  };

  //  Compute and apply the transform for a specific time
  Dancer.prototype.animate = function(beat)
  {
    if (beat == undefined)
      beat = this.beats();
    // Be sure to reset grips at start
    if (beat == 0)
      this.rightgrip = this.leftgrip = null;
    //  Start to build transform
    //  Apply all completed movements
    this.tx = new AffineTransform(this.start);
    var m = null;
    if (this.path != null) {
      this.path.movelist.every(function(mi,i) {
        m = mi;
        if (beat >= m.beats) {
          this.tx = new AffineTransform(this.start);
          this.tx = this.tx.preConcatenate(this.path.transformlist[i]);
          beat -= m.beats;
          m = null;
          return true;
        } else
          return false;
      },this);
    }
    //  Apply movement in progress
    if (m != null) {
      this.tx = this.tx.preConcatenate(m.translate(beat));
      this.tx = this.tx.preConcatenate(m.rotate(beat));
      if (beat < 0)
        this.hands = Movement.BOTHHANDS;
      else
        this.hands = m.hands;
      if ((m.hands & Movement.GRIPLEFT) == 0)
        this.leftgrip = null;
      if ((m.hands & Movement.GRIPRIGHT) == 0)
        this.rightgrip = null;
    }
    else  // End of movement
      this.hands = Movement.BOTHHANDS;  // hold hands in ending formation
    this.angle = Math.toDegrees(this.tx.angle);
  };
  Dancer.prototype.animateToEnd = function()
  {
    this.animate(this.beats());
  };

  Dancer.prototype.hexagonify = function(beat)
  {
    var a0 = Math.atan2(this.starty,this.startx);  // hack
    var a1 = this.tx.location.angle;
    //  Correct for wrapping around +/- pi
    if (beat <= 0.0)
      this.prevangle = a1;
    var wrap = Math.round((a1-this.prevangle)/(Math.PI*2));
    a1 -= wrap*Math.PI*2;
    var a2 = -(a1-a0)/3;
    this.concatenate(AffineTransform.getRotateInstance(a2));
    this.prevangle = a1;
  };

  Dancer.prototype.bigonify = function(beat)
  {
    var a0 = Math.atan2(this.starty,this.startx);  // hack
    var a1 = this.tx.location.angle;
    //  Correct for wrapping around +/- pi
    if (beat <= 0.0)
      this.prevangle = a1;
    var wrap = Math.round((a1-this.prevangle)/(Math.PI*2));
    a1 -= wrap*Math.PI*2;
    var a2 = +(a1-a0);
    this.concatenate(AffineTransform.getRotateInstance(a2));
    this.prevangle = a1;
  };

  Dancer.prototype.paint = function()
  {
    this.svg.setAttribute('transform',this.tx.toString());
    if (this.gender == Dancer.PHANTOM)
      this.svg.setAttribute('opacity',0.6);
    this.righthand.setAttribute('transform',
        new AffineTransform(this.tx).preConcatenate(this.rightHandTransform).toString());
    this.lefthand.setAttribute('transform',
        new AffineTransform(this.tx).preConcatenate(this.leftHandTransform).toString());
    if (this.tamsvg.numbers || this.tamsvg.couples) {
      var a = this.tx.angle;
      var t1 = AffineTransform.getScaleInstance(0.04,-0.04);
      var t2 = AffineTransform.getRotateInstance(a);
      if (this.tamsvg.numbers)
        this.numbersvg.setAttribute('transform',t1.preConcatenate(t2).toString());
      else
        this.couplessvg.setAttribute('transform',t1.preConcatenate(t2).toString());
    }
  };

  Dancer.prototype.toString = function()
  {
    return this.number;
  };

  return Dancer;

});
