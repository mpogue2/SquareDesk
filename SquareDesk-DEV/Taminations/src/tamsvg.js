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

/*

  TamSVG - Javascript+SVG implementation of the original Tamination.java

 */
define(['tamination','cookie','handhold','color','affinetransform','vector','bezier','movement','path','dancer',,'jquerysvg','jquerymousewheel'],
    function(Tamination,Cookie,Handhold,Color,AffineTransform,Vector,Bezier,Movement,Path,Dancer) {

  //  Setup - called when page is loaded
  function TamSVG(svg_in)
  {
    if (this instanceof TamSVG) {
      //  Called as 'new TamSVG(x)'
      this.init(svg_in);
      window.tamsvg = this;
    }
    else
      //  Called as 'TamSVG(x)'
      window.tamsvg = new TamSVG(svg_in);
    return window.tamsvg;
  }

  TamSVG.prototype = {
      init: function(svg_in)
      {
        var me = this;
        cookie = new Cookie("TAMination");
        this.cookie = cookie;
        this.animationListener = null;
        this.titlegroup = false;
        $(document).bind("contextmenu",function() { return false; });
        //  Get initial values from cookie and anything in the URL
        if (typeof args != 'object')
          args = {};
        this.hexagon = cookie.hexagon == "true";
        if (args.hexagon !== undefined)
          this.hexagon = args.hexagon;
        this.bigon = cookie.bigon == "true";
        if (args.bigon !== undefined)
          this.bigon = args.bigon;

        if (args.speed != undefined) {
          if (args.speed == 'slow')
            this.slow(true);
          else if (args.speed == 'fast')
            this.fast(true);
          else
            this.normal(true);
        }
        else if (cookie.speed == 'slow')
          this.slow(true);
        else if (cookie.speed == 'fast')
          this.fast(true);
        else
          this.normal(true);

        this.loop = cookie.loop == "true";
        if (args.loop != undefined)
          this.loop = args.loop;
        this.grid = cookie.grid == "true";
        if (args.grid != undefined)
          this.grid = args.grid;
        this.numbers = cookie.numbers == 'true';
        if (args.numbers != undefined)
          this.numbers = args.numbers;
        this.couples = cookie.couples == 'true';
        if (args.couples != undefined)
          this.couples = args.couples;
        if (this.couples)
          this.numbers = false;
        this.showPhantoms = cookie.phantoms == "true";
        if (cookie.svg != 'true') {
          cookie.svg = "true";
          cookie.store(365,'/tamination');
        }
        this.currentpart = 0;
        this.barstool = 0;
        this.compass = 0;
        this.animationStopped = function() { };
        //  Set up the dance floor
        this.svg = svg_in;
        this.svg.configure({width: '100%', height:'100%', viewBox: '0 0 100 100'});
        this.floorsvg = this.svg.svg(null,0,0,100,100,-6.5,-6.5,13,13);
        this.floorsvg.setAttribute('width','100%');
        this.floorsvg.setAttribute('height','100%');
        this.allp = tam.getPath();
        //  Save parts or fractions
        var partstr = tam.getParts();
        var fracstr = tam.getFractions();
        if (partstr + fracstr == '')
          this.parts = [];
        else {
          this.parts = (partstr+fracstr).split(/;/);
          for (var i in this.parts)
            this.parts[i] = Number(this.parts[i]);
        }
        this.hasParts = partstr != '';
        //  first token is 'Formation', followed by e.g. boy 1 2 180 ...
        var formation = tam.getFormation();
        //  Flip the y direction on the dance floor to match our math
        this.floor = this.svg.group(this.floorsvg);
        this.floor.setAttribute('transform',AffineTransform.getScaleInstance(1,-1).toString());
        this.svg.rect(this.floor,-6.5,-6.5,13,13,{fill:'#ffffc0'});

        //  Add title, optionally with audio link
        this.titlegroup = this.svg.group(this.floorsvg);
        this.titlesvg = this.svg.text(this.titlegroup,0,0,' ',{fontSize: "10", transform:"translate(-6.4,-5.5) scale(0.1)"});
        if (typeof tam.getTitle() != "undefined") {
          var tt = tam.getTitle().replace(/\(.*?\)/g,' ');
          this.setTitle(tt);
        }

        this.gridgroup = this.svg.group(this.floor,{fill:"none",stroke:"black",strokeWidth:0.01});
        this.hexgridgroup = this.svg.group(this.floor,{fill:"none",stroke:"black",strokeWidth:0.01});
        this.bigongridgroup = this.svg.group(this.floor,{fill:"none",stroke:"black",strokeWidth:0.01});
        this.bigoncentergroup = this.svg.group(this.floor,{fill:"none",stroke:"black",strokeWidth:0.01});
        this.drawGrid();
        if (!this.grid || this.hexagon || this.bigon)
          this.gridgroup.setAttribute('visibility','hidden');
        if (!this.grid || !this.hexagon)
          this.hexgridgroup.setAttribute('visibility','hidden');
        if (!this.grid || !this.bigon)
          this.bigongridgroup.setAttribute('visibility','hidden');
        if (!this.bigon)
          this.bigoncentergroup.setAttribute('visibility','hidden');
        this.pathparent = this.svg.group(this.floor);
        if (!this.showPaths)
          this.pathparent.setAttribute('visibility','hidden');
        this.handholds = this.svg.group(this.floor);
        this.dancegroup = this.svg.group(this.floor);
        this.dancers = [];
        var dancerColor = [ Color.red, Color.green, Color.blue, Color.yellow,
                            Color.lightGray, Color.lightGray, Color.lightGray, Color.lightGray ];
        var numbers = tam.getNumbers();
        var couples = tam.getCouples();
        $('dancer',formation).each(function(j) {
          var gender = Dancer.genders[$(this).attr('gender')];
          var d = new Dancer({
            tamsvg: me,
            gender: gender,
            x: -Number($(this).attr('y')),
            y: -Number($(this).attr('x')),
            angle: Number($(this).attr('angle'))+180,
            color:  dancerColor[couples[j*2]-1],
            path: me.allp[j],
            number: gender==Dancer.PHANTOM ? ' ' : numbers[me.dancers.length],
                couplesnumber: gender==Dancer.PHANTOM ? ' ' : couples[j*2],
                    hidden: gender == Dancer.PHANTOM && !me.showPhantoms
          });
          me.dancers.push(d);
          d = new Dancer({
            tamsvg: me,
            gender: gender,
            x: Number($(this).attr('y')),
            y: Number($(this).attr('x')),
            angle: Number($(this).attr('angle')),
            color:  dancerColor[couples[j*2+1]-1],
            path: me.allp[j],
            number: gender==Dancer.PHANTOM ? ' ' : numbers[me.dancers.length],
                couplesnumber: gender==Dancer.PHANTOM ? ' ' : couples[j*2+1],
                    hidden: gender == Dancer.PHANTOM && !me.showPhantoms
          });
          me.dancers.push(d);
        });
        this.barstoolmark = this.svg.circle(this.floor,0,0,0.2,{fill:'black'});
        var pth = this.svg.createPath();
        this.compassmark = this.svg.path(this.floor,
            pth.move(0,-0.4).line(0,0.4).move(-0.4,0).line(0.4,0),
            {stroke:'black',strokeWidth:0.05});
        this.barstoolmark.setAttribute('visibility','hidden');
        this.compassmark.setAttribute('visibility','hidden');

        //  Compute animation length
        this.beats = 0.0;
        for (var d in this.dancers)
          this.beats = Math.max(this.beats,this.dancers[d].beats());
        this.beats += 2.0;

        //  Mouse wheel moves the animation
        $(this.floorsvg).on('mousewheel',function(event) {
          me.beat += event.deltaY * 0.2;
          me.animate();
        });

        //  Initialize the animation
        if (this.hexagon)
          this.convertToHexagon();
        else if (this.bigon)
          this.convertToBigon();
        this.beat = -2.0;
        this.prevbeat = -2.0;
        this.running = false;
        for (var i in this.dancers)
          this.dancers[i].recalculate();
        this.lastPaintTime = new Date().getTime();
        this.animate();
      },

      setTitle: function(tt)
      {
        $(this.titlesvg).text(tt);
        var ln = this.titlesvg.getComputedTextLength();
        if (ln > 110) {
          var s = 110/ln;
          this.titlesvg.setAttribute('transform',"translate(-6.4,"+(-6.5+s)+") scale("+(s/10)+")");
        }
        /*  Turn this on if we ever get audio
    //  Find out if we have audio for this title
    var ttid = '#'+tt.replace(/ /g,'_').replace(/\W/g,'').toLowerCase()+'_audio';
    if ($(ttid).length > 0) {
      //  Speaker SVG grabbed from Wikipedia (public domain)
      var speakergroup = this.svg.group(this.titlegroup,
          {fill:"none",stroke:"brown",strokeWidth:5,'stroke-linejoin':"round",'stroke-linecap':"round",
        transform:"translate(4.7,5) scale(0.02)"});
      this.svg.polygon(speakergroup,
          [[39.389,13.769], [22.235,28.606], [6,28.606], [6,47.699], [21.989,47.699], [39.389,62.75], [39.389,13.769]],
          {fill:"red"}
      );
      this.svg.path(speakergroup,
          this.svg.createPath().move(48.128,49.03).curveC([[50.057,45.934, 51.19,42.291, 51.19,38.377],
                                                           [51.19,34.399, 50.026,30.703, 48.043,27.577]]));
      this.svg.path(speakergroup,
          this.svg.createPath().move(55.082,20.537).curveC([[58.777,25.523, 60.966,31.694, 60.966,38.377],
                                                            [60.966,44.998, 58.815,51.115, 55.178,56.076]]));
      this.svg.path(speakergroup,
          this.svg.createPath().move(61.71,62.611).curveC([[66.977,55.945, 70.128,47.531, 70.128,38.378],
                                                           [70.128,29.161, 66.936,20.696, 61.609,14.01]]));
      $(speakergroup).mousedown(function() {
        $(ttid).get(0).play();
      });
    }  */
      },

      hideTitle: function()
      {
        if (this.titlegroup)
          this.titlegroup.setAttribute('visibility','hidden');
      },

      toggleHexagon: function()
      {
        this.setHexagon(!this.hexagon);
      },

      toggleBigon: function()
      {
        this.setBigon(!this.bigon);
      },

      setAnimationListener: function(l)
      {
        this.animationListener = l;
      },

      //  This function is called repeatedly to move the dancers
      animate: function()
      {
        //  Update the animation time
        var now = new Date().getTime();
        var diff = now - this.lastPaintTime;
        if (this.running)
          this.beat += diff/this.speed;
        //  Update the dancers
        this.paint();
        this.lastPaintTime = now;
        //  Update the slider
        //  would probably be better to do this with a callback
        $('#playslider').slider('value',this.beat*100);
        if (this.animationListener != null)
          this.animationListener(this.beat);
        //$('#animslider').val(Math.floor(this.beat*100)).slider('refresh');
        if (this.beat >= this.beats) {
          if (this.loop)
            this.beat = -2;
          else if (this.running)
            this.stop();
        }
        if (this.beat > this.beats)
          this.beat = this.beats;
        if (this.beat < -2)
          this.beat = -2;
        //  Update the definition highlight
        var thispart = 1;
        var partsum = 0;
        for (var i in this.parts) {
          if (this.parts[i]+partsum < this.beat)
            thispart++;
          partsum += this.parts[i];
        }
        if (this.beat < 0 || this.beat > this.beats-2)
          thispart = 0;
        if (thispart != this.currentpart) {
          if (this.setPart)
            this.setPart(thispart);
          this.currentpart = thispart;
        }
      },

      setBeat: function(b)
      {
        this.beat = b;
        this.animate();
      },

      paint: function(beat)
      {
        if (arguments.length == 0)
          beat = this.beat;
        //  If a big jump from the last hexagon animation, calculate some
        //  intervening ones so the wrap works
        while (this.hexagon && Math.abs(beat-this.prevbeat) > 1.1)
          this.paint(this.prevbeat + (beat > this.prevbeat ? 1.0 : -1.0));
        this.prevbeat = beat;

        //  Move dancers
        for (var i in this.dancers)
          this.dancers[i].animate(beat);

        //  If hexagon, rotate relative to center
        if (this.hexagon) {
          for (var i in this.dancers) {
            var d = this.dancers[i];
            var a0 = Math.atan2(d.starty,d.startx);  // hack
            var a1 = d.tx.location.angle;
            //  Correct for wrapping around +/- pi
            if (this.beat <= 0.0)
              d.prevangle = a1;
            var wrap = Math.round((a1-d.prevangle)/(Math.PI*2));
            a1 -= wrap*Math.PI*2;
            var a2 = -(a1-a0)/3;
            d.concatenate(AffineTransform.getRotateInstance(a2));
            d.prevangle = a1;
          }
        }
        //  If bigon, rotate relative to center
        if (this.bigon) {
          for (var i in this.dancers) {
            var d = this.dancers[i];
            var a0 = Math.atan2(d.starty,d.startx);  // hack
            var a1 = d.tx.location.angle;
            //  Correct for wrapping around +/- pi
            if (this.beat <= 0.0)
              d.prevangle = a1;
            var wrap = Math.round((a1-d.prevangle)/(Math.PI*2));
            a1 -= wrap*Math.PI*2;
            var a2 = +(a1-a0);
            d.concatenate(AffineTransform.getRotateInstance(a2));
            d.prevangle = a1;
          }
        }

        //  If barstool, translate to keep the barstool dancer stationary in center
        if (this.barstool) {
          var tx = AffineTransform.getTranslateInstance(
              -this.barstool.tx.translateX,
              -this.barstool.tx.translateY);
          for (var i in this.dancers) {
            this.dancers[i].concatenate(tx);
          }
          this.barstoolmark.setAttribute('cx',0);
          this.barstoolmark.setAttribute('cy',0);
          this.barstoolmark.setAttribute('visibility','visible');
        }
        else
          this.barstoolmark.setAttribute('visibility','hidden');

        //  If compass, rotate relative to compass dancer
        if (this.compass) {
          var tx = AffineTransform.getRotateInstance(
              this.compass.startangle*Math.PI/180-this.compass.tx.angle);
          for (var i in this.dancers) {
            this.dancers[i].concatenate(tx);
          }
          this.compassmark.setAttribute('transform',
              AffineTransform.getTranslateInstance(
                  this.compass.tx.location));
          this.compassmark.setAttribute('visibility','visible');
        }
        else
          this.compassmark.setAttribute('visibility','hidden');

        //  Compute handholds
        Handhold.dfactor0 = this.hexagon ? 1.15 : 1.0;
        var hhlist = [];
        for (var i0 in this.dancers) {
          var d0 = this.dancers[i0];
          d0.rightdancer = d0.leftdancer = null;
          d0.rightHandNewVisibility = false;
          d0.leftHandNewVisibility = false;
        }
        for (var i1=0; i1<this.dancers.length-1; i1++) {
          var d1 = this.dancers[i1];
          if (d1.gender==Dancer.PHANTOM && !this.showPhantoms)
            continue;
          for (var i2=i1+1; i2<this.dancers.length; i2++) {
            var d2 = this.dancers[i2];
            if (d2.gender==Dancer.phantom && !this.showPhantoms)
              continue;
            //  Check that there is no dancer between these two
            //  This is particularly useful for calls with phantoms, where
            //  without the phantom there would be a handhold
            var d1d2 = d1.distanceTo(d2);
            for (var i3=0; i3<this.dancers.length; i3++) {
              var d3 = this.dancers[i3];
              if (i3!=i1 && i3!=i2 && Math.isApprox(d3.distanceTo(d1)+d3.distanceTo(d2),d1d2))
                d1d2 = 0;
            }
            if (d1d2==0)
              continue;        var hh = Handhold.getHandhold(d1,d2);
              if (hh != null)
                hhlist.push(hh);
          }
        }

        hhlist.sort(function(a,b) { return a.score - b.score; });
        for (var h in hhlist) {
          var hh = hhlist[h];
          /*if (this.bigon) {
        if (Math.abs(hh.d1.centerAngle-3*Math.PI/2) < 3 &&
            hh.d1.hands == Movement.RIGHTHAND)
          continue;
      }*/
          //  Check that the hands aren't already used
          var incenter = this.hexagon && hh.inCenter();
          if (incenter ||
              (hh.h1 == Movement.RIGHTHAND && hh.d1.rightdancer == null ||
                  hh.h1 == Movement.LEFTHAND && hh.d1.leftdancer == null) &&
                  (hh.h2 == Movement.RIGHTHAND && hh.d2.rightdancer == null ||
                      hh.h2 == Movement.LEFTHAND && hh.d2.leftdancer == null)) {
            hh.paint();
            if (incenter)
              continue;
            if (hh.h1 == Movement.RIGHTHAND) {
              hh.d1.rightdancer = hh.d2;
              if ((hh.d1.hands & Movement.GRIPRIGHT) == Movement.GRIPRIGHT)
                hh.d1.rightgrip = hh.d2;
            } else {
              hh.d1.leftdancer = hh.d2;
              if ((hh.d1.hands & Movement.GRIPLEFT) == Movement.GRIPLEFT)
                hh.d1.leftgrip = hh.d2;
            }
            if (hh.h2 == Movement.RIGHTHAND) {
              hh.d2.rightdancer = hh.d1;
              if ((hh.d2.hands & Movement.GRIPRIGHT) == Movement.GRIPRIGHT)
                hh.d2.rightgrip = hh.d1;
            } else {
              hh.d2.leftdancer = hh.d1;
              if ((hh.d2.hands & Movement.GRIPLEFT) == Movement.GRIPLEFT)
                hh.d2.leftgrip = hh.d1;
            }
          }
        }
        //  Clear handholds no longer visible
        for (var i in this.dancers) {
          var d = this.dancers[i];
          if (d.rightHandVisibility && !d.rightHandNewVisibility) {
            d.righthand.setAttribute('visibility','hidden');
            d.rightHandVisibility = false;
          }
          if (d.leftHandVisibility && !d.leftHandNewVisibility) {
            d.lefthand.setAttribute('visibility','hidden');
            d.leftHandVisibility = false;
          }
        }

        //  Paint dancers with hands
        //$('#animslider').val(this.beat);
        for (var i in this.dancers)
          this.dancers[i].paint();

      },

      rewind: function()
      {
        this.stop();
        this.beat = -2;
        this.animate();
      },

      prev: function()
      {
        var b = 0;
        var best = this.beat;
        for (var i in this.parts) {
          b += this.parts[i];
          if (b < this.beat)
            best = b;
        }
        if (best == this.beat && best > 0)
          best = 0;
        else if (this.beat <= 0)
          best = -2;
        this.beat = best;
        this.animate();
      },

      backward: function()
      {
        this.stop();
        if (this.beat > 0.1)
          this.beat -= 0.1;
        this.animate();
      },

      stop: function()
      {
        if (this.timer != null)
          clearInterval(this.timer);
        this.timer = null;
        this.running = false;
        this.animationStopped();
      },

      start: function()
      {
        this.lastPaintTime = new Date().getTime();
        if (this.timer == null) {
          var me = this;
          this.timer = setInterval(function() { me.animate(); },25);
        }
        if (this.beat >= this.beats)
          this.beat = -2;
        this.running = true;
        $('#playButton').attr('value','Stop');
      },

      play: function()
      {
        if (this.running)
          this.stop();
        else
          this.start();
      },

      forward: function()
      {
        this.stop();
        if (this.beat < this.beats)
          this.beat += 0.1;
        this.animate();
      },

      next: function()
      {
        var b = 0;
        for (var i in this.parts) {
          b += this.parts[i];
          if (b > this.beat) {
            this.beat = b;
            b = -1000;
          }
        }
        if (b >= 0 && b < this.beats-2)
          this.beat = this.beats-2;
        this.animate();
      },

      end: function()
      {
        this.stop();
        if (this.beat < this.beats-2)
          this.beat = this.beats-2;
        this.animate();
      },

      slow: function(setcookie)
      {
        this.speed = 1500;
        if (arguments.length > 0) {
          this.cookie.speed = 'slow';
          this.cookie.store(365,'/tamination');
        }
      },
      normal: function(setcookie)
      {
        this.speed = 500;
        if (arguments.length > 0) {
          this.cookie.speed = 'normal';
          this.cookie.store(365,'/tamination');
        }
      },
      fast: function(setcookie)
      {
        this.speed = 200;
        if (arguments.length > 0) {
          this.cookie.speed = 'fast';
          this.cookie.store(365,'/tamination');
        }
      },
      isSlow: function()
      {
        return this.speed == 1500;
      },
      isNormal: function()
      {
        return this.speed == 500;
      },
      isFast: function()
      {
        return this.speed == 200;
      },

      setLoop: function(v) {
        if (arguments.length > 0)
          this.loop = v;
        this.cookie.loop = this.loop;
        this.cookie.store(365,'/tamination');
        return this.loop;
      },

      setGrid: function(v) {
        if (arguments.length > 0) {
          this.grid = v;
          this.hexgridgroup.setAttribute('visibility','hidden');
          this.bigongridgroup.setAttribute('visibility','hidden');
          this.gridgroup.setAttribute('visibility','hidden');
          if (this.grid) {
            if (tamsvg.hexagon)
              tamsvg.hexgridgroup.setAttribute('visibility','visible');
            else if (tamsvg.bigon)
              tamsvg.bigongridgroup.setAttribute('visibility','visible');
            else
              tamsvg.gridgroup.setAttribute('visibility','visible');
          }
        }
        this.cookie.grid = this.grid;
        this.cookie.store(365,'/tamination');
        return this.grid;
      },

      setPaths: function(v) {
        if (arguments.length > 0)
          this.showPaths = v;
        this.pathparent.setAttribute('visibility',this.showPaths ? 'visible' : 'hidden');
        for (var i in this.dancers) {
          var nopath = this.dancers[i].gender == Dancer.PHANTOM && !this.showPhantoms;
          this.dancers[i].pathgroup.setAttribute('visibility',this.showPaths && !nopath ? 'visible' : 'hidden');
          this.dancers[i].beziergroup.setAttribute('visibility','hidden');
          this.dancers[i].pathVisible = this.showPaths;
        }
        return this.showPaths;
      },

      setNumbers: function(v)
      {
        if (arguments.length > 0) {
          this.numbers = v;
          if (this.numbers)
            this.couples = false;
          for (var i in this.dancers) {
            var d = this.dancers[i];
            if (this.numbers)
              d.showNumber();
            else
              d.hideNumbers();
          }
          this.cookie.numbers = this.numbers;
          this.cookie.couples = this.couples;
          this.cookie.store(365,'/tamination');
        }
        return this.numbers;
      },

      setCouples: function(v)
      {
        if (arguments.length > 0) {
          this.couples = v;
          if (this.couples)
            this.numbers = false;
          for (var i in this.dancers) {
            var d = this.dancers[i];
            if (this.couples)
              d.showCouplesNumber();
            else
              d.hideNumbers();
          }
          this.cookie.numbers = this.numbers;
          this.cookie.couples = this.couples;
          this.cookie.store(365,'/tamination');
        }
        return this.couples;
      },

      setPhantoms: function(v)
      {
        if (arguments.length > 0) {
          this.showPhantoms = v;
          for (var i in this.dancers)
            if (this.dancers[i].gender == Dancer.PHANTOM) {
              if (this.showPhantoms)
                this.dancers[i].show();
              else
                this.dancers[i].hide();
            }
        }
        this.cookie.phantoms = this.showPhantoms;
        this.cookie.store(365,'/tamination');
        return this.showPhantoms;
      },

      setHexagon: function(v)
      {
        if (arguments.length > 0) {
          this.hexagon = v;
          if (this.hexagon) {
            if (this.bigon) {
              this.bigon = false;
              this.revertFromBigon();
            }
            this.convertToHexagon();
            this.animate();
            if (this.grid) {
              this.gridgroup.setAttribute('visibility','hidden');
              this.bigongridgroup.setAttribute('visibility','hidden');
              this.hexgridgroup.setAttribute('visibility','visible');
            }
          } else {
            this.revertFromHexagon();
            this.animate();
            if (this.grid) {
              this.hexgridgroup.setAttribute('visibility','hidden');
              this.gridgroup.setAttribute('visibility','visible');
            }
          }
          for (var i in this.dancers)
            this.dancers[i].paintPath();
          this.animate();
          this.cookie.hexagon = this.hexagon;
          this.cookie.bigon = false;
          this.cookie.store(365,'/tamination');
        }
        return this.hexagon;
      },

      setBigon: function(v)
      {
        if (arguments.length > 0) {
          this.bigon = v;
          if (this.bigon) {
            if (this.hexagon) {
              this.hexagon = false;
              this.revertFromHexagon();
            }
            this.convertToBigon();
            this.animate();
            if (this.grid) {
              this.gridgroup.setAttribute('visibility','hidden');
              this.hexgridgroup.setAttribute('visibility','hidden');
              this.bigongridgroup.setAttribute('visibility','visible');
            }
          } else {
            this.revertFromBigon();
            this.animate();
            if (this.grid) {
              this.bigongridgroup.setAttribute('visibility','hidden');
              this.gridgroup.setAttribute('visibility','visible');
            }
          }
          for (var i in this.dancers)
            this.dancers[i].paintPath();
          this.animate();
          this.cookie.bigon = this.bigon;
          this.cookie.hexagon = false;
          this.cookie.store(365,'/tamination');
        }
        return this.bigon;
      },


      drawGrid: function()
      {
        //  Square grid
        for (var x=-7.5; x<=7.5; x+=1)
          this.svg.line(this.gridgroup,x,-7.5,x,7.5);
        for (var y=-7.5; y<=7.5; y+=1)
          this.svg.line(this.gridgroup,-7.5,y,7.5,y);

        //  Hex grid
        for (var x0=0.5; x0<=8.5; x0+=1) {
          var points = [];
          // moveto 0, x0
          points.push([0,x0]);
          for (var y0=0.5; y0<=8.5; y0+=0.5) {
            var a = Math.atan2(y0,x0)*2/3;
            var r = Math.sqrt(x0*x0+y0*y0);
            var x = r*Math.sin(a);
            var y = r*Math.cos(a);
            // lineto x,y
            points.push([x,y]);
          }
          //  reflect and rotate the result
          for (var a=0; a<6; a++) {
            var t = "rotate("+(a*60)+")";
            this.svg.polyline(this.hexgridgroup,points,{transform:t});
            this.svg.polyline(this.hexgridgroup,points,{transform:t+" scale(1,-1)"});
          }
        }

        //  Bigon grid
        for (var x1=-7.5; x1<=7.5; x1+=1) {
          var points = [];
          points.push([0,Math.abs(x1)]);
          for (var y1=0.2; y1<=7.5; y1+=0.2) {
            var a = 2*Math.atan2(y1,x1);
            var r = Math.sqrt(x1*x1+y1*y1);
            var x = r*Math.sin(a);
            var y = r*Math.cos(a);
            points.push([x,y]);
          }
          this.svg.polyline(this.bigongridgroup,points);
          this.svg.polyline(this.bigongridgroup,points,{transform:"scale(1,-1)"});
        }

        //  Bigon center mark
        this.svg.line(this.bigoncentergroup,0,-0.5,0,0.5);
        this.svg.line(this.bigoncentergroup,-0.5,0,0.5,0);

      },

      convertToHexagon: function()
      {
        //  Save current dancers
        this.setBeat(-2);
        for (var i in this.dancers)
          this.dancers[i].hide();
        this.saveDancers = this.dancers;
        this.dancers = [];
        var dancerColor = [ Color.red, Color.green, Color.magenta, Color.blue, Color.yellow,
                            Color.cyan, Color.lightGray ];
        var hexnumbers = [ "A","E","I","B","F","J",
                           "C","G","K","D","H","L",' ' ];
        var hexcouples = [ "1", "3", "5", "1", "3", "5",
                           "2", "4", "6", "2", "4", "6", ' ' ];
        //  Generate hexagon dancers
        for (var i=0; i<this.saveDancers.length; i+=2) {
          var j = Math.floor(i/4);
          var jj = j <= 1 ? [j,j+2,j+4] : [6, 6, 6];
          var isPh = this.saveDancers[i].gender == Dancer.PHANTOM;
          this.dancers.push(new Dancer({dancer:this.saveDancers[i],
            path:this.saveDancers[i].path,
            angle:30,
            color:dancerColor[jj[0]],
            hidden: isPh && !this.showPhantoms,
            number:isPh?' ':hexnumbers[i/2*3],
                couplesnumber:isPh?' ':hexcouples[i/2*3]}));
          this.dancers.push(new Dancer({dancer:this.saveDancers[i],
            path:this.saveDancers[i].path,
            angle:150,
            color:dancerColor[jj[1]],
            hidden: isPh && !this.showPhantoms,
            number:isPh?' ':hexnumbers[i/2*3+1],
                couplesnumber:isPh?' ':hexcouples[i/2*3+1]}));
          this.dancers.push(new Dancer({dancer:this.saveDancers[i],
            path:this.saveDancers[i].path,
            angle:270,
            color:dancerColor[jj[2]],
            hidden: isPh && !this.showPhantoms,
            number:isPh?' ':hexnumbers[i/2*3+2],
                couplesnumber:isPh?' ':hexcouples[i/2*3+2]}));
        }
        //  Convert to hexagon positions and paths
        for (var i=0; i<this.dancers.length; i++) {
          this.hexagonify(this.dancers[i],30+(i%3)*120);
        }
      },

      convertToBigon: function()
      {
        //  Save current dancers
        this.setBeat(-2);
        for (var i in this.dancers)
          this.dancers[i].hide();
        this.saveDancers = this.dancers;
        this.dancers = [];
        var dancerColor = [ Color.red, Color.yellow, Color.green, Color.blue,
                            Color.magenta, Color.cyan ];
        for (var i=0; i<this.saveDancers.length; i+=2) {
          var j = Math.floor(i/2);
          var isPh = this.saveDancers[i].gender == Dancer.PHANTOM;
          var numstr = isPh ? ' ' : (j+1)+'';
          this.dancers.push(new Dancer({dancer:this.saveDancers[i],
            path:this.saveDancers[i].path,
            number:numstr,couplesnumber:numstr,
            hidden: isPh && !this.showPhantoms,
            color:dancerColor[j]}));
        }
        //  Generate Bigon dancers
        for (var i=0; i<this.dancers.length; i++) {
          this.bigonify(this.dancers[i]);
        }
        this.bigoncentergroup.setAttribute('visibility','visible');

      },

      revertFromHexagon: function()
      {
        for (var i in this.dancers)
          this.dancers[i].hide();
        this.dancers = this.saveDancers;
        for (var i in this.dancers)
          if (this.dancers[i].gender != Dancer.PHANTOM || this.showPhantoms)
            this.dancers[i].show();
        this.animate();
      },

      revertFromBigon: function()
      {
        this.bigoncentergroup.setAttribute('visibility','hidden');
        //  Just restore the saved dancers
        this.revertFromHexagon();
      },

      //  Moves the position and angle of a dancer from square to hexagon
      hexagonify: function(d,a)
      {
        a = a*Math.PI/180;
        var x = d.startx;
        var y = d.starty;
        var r = Math.sqrt(x*x+y*y);
        var angle = Math.atan2(y,x);
        var dangle = 0.0;
        if (angle < 0)
          dangle = -(Math.PI+angle)/3;
        else
          dangle = (Math.PI-angle)/3;
        d.startx = r*Math.cos(angle+dangle+a);
        d.starty = r*Math.sin(angle+dangle+a);
        d.startangle += dangle*180/Math.PI;
        d.computeStart();
        d.recalculate();
      },

      bigonify: function(d)
      {
        var cangle = Math.PI/2.0;
        var x = d.startx;
        var y = d.starty;
        var r = Math.sqrt(x*x+y*y);
        var angle = Math.atan2(y,x)+cangle;
        var bigangle = angle*2-cangle;
        d.startx = r*Math.cos(bigangle);
        d.starty = r*Math.sin(bigangle);
        d.startangle += angle*180/Math.PI;
        d.computeStart();
        d.recalculate();
      }


  };

  //  Build buttons and slider below animation
  TamSVG.prototype.generateButtonPanel = function() {
    $('#buttonpanel').remove();
    $('#svgcontainer').append('<div id="buttonpanel" style="background-color: #c0c0c0"></div>');

    $('#buttonpanel').append('<div id="optionpanel"></div>');
    $('#optionpanel').append('<input type="button" class="appButton" id="slowButton" value="Slow" style="width:20%"/>');
    $('#optionpanel').append('<input type="button" class="appButton" id="fastButton" value="Fast" style="width:20%"/>');
    if (this.isSlow())
      $('#slowButton').addClass('selected');
    else if (this.isFast())
      $('#fastButton').addClass('selected');
    $('#optionpanel').append('<input type="button" class="appButton" id="loopButton" value="Loop" style="width:20%"/>');
    if (this.loop)
      $('#loopButton').addClass('selected');
    $('#optionpanel').append('<input type="button" class="appButton" id="gridButton" value="Grid" style="width:20%"/>');
    if (this.grid)
      $('#gridButton').addClass('selected');
    $('#optionpanel').append('<input type="button" class="appButton" id="couplesButton" value="#4" style="width:10%"/>');
    if (this.couples)
      $('#couplesButton').addClass('selected');
    $('#optionpanel').append('<input type="button" class="appButton" id="numbersButton" value="#8" style="width:10%"/>');
    if (this.numbers)
      $('#numbersButton').addClass('selected');
    $('#optionpanel').append('<br/>');
    $('#optionpanel').append('<input type="button" class="appButton" id="hexagonButton" value="Hexagon" style="width:25%"/>');
    if (this.hexagon)
      $('#hexagonButton').addClass('selected');
    $('#optionpanel').append('<input type="button" class="appButton" id="bigonButton" value="Bi-gon" style="width:25%"/>');
    if (this.bigon)
      $('#bigonButton').addClass('selected');
    $('#optionpanel').append('<input type="button" class="appButton" id="phantomsButton" value="Phantoms" style="width:25%"/>');
    if (this.dancers.length <= 8)
      $('#phantomsButton').addClass('disabled');
    else if (this.showPhantoms)
      $('#phantomsButton').addClass('selected');
    $('#optionpanel').append('<input type="button" class="appButton" id="pathsButton" value="Paths" style="width:25%"/>');
    if (this.showPaths)
      $('#pathsButton').addClass('selected');

    // Speed button actions
    var me = this;
    $('#slowButton').click(function() {
      if (me.isSlow()) {
        me.normal(true);
        $('#slowButton').removeClass('selected');
      } else {
        me.slow(true);
        $('#slowButton').addClass('selected');
        $('#fastButton').removeClass('selected');
      }
    });
    $('#fastButton').click(function() {
      if (me.isFast()) {
        me.normal(true);
        $('#fastButton').removeClass('selected');
      } else {
        me.fast(true);
        $('#fastButton').addClass('selected');
        $('#slowButton').removeClass('selected');
      }
    });

    // Actions for other options
    $('#loopButton').click(function() {
      if (me.loop) {
        me.loop = false;
        $('#loopButton').removeClass('selected');
      } else {
        me.loop = true;
        $('#loopButton').addClass('selected');
      }
      cookie.loop = me.loop;
      cookie.store(365,'/tamination');
    });
    $('#pathsButton').click(function() {
      if (me.showPaths) {
        me.showPaths = false;
        me.setPaths(false);
        //tamsvg.pathparent.setAttribute('visibility','hidden');
        $('#pathsButton').removeClass('selected');
      } else {
        me.showPaths = true;
        me.setPaths(true);
        //tamsvg.pathparent.setAttribute('visibility','visible');
        $('#pathsButton').addClass('selected');
      }
      cookie.paths = me.showPaths;
      cookie.store(365,'/tamination');
    });
    $('#gridButton').click(function() {
      if (me.grid) {
        me.grid = false;
        me.hexgridgroup.setAttribute('visibility','hidden');
        me.bigongridgroup.setAttribute('visibility','hidden');
        me.gridgroup.setAttribute('visibility','hidden');
        $('#gridButton').removeClass('selected');
      } else {
        me.grid = true;
        if (me.hexagon)
          me.hexgridgroup.setAttribute('visibility','visible');
        else if (me.bigon)
          me.bigongridgroup.setAttribute('visibility','visible');
        else
          me.gridgroup.setAttribute('visibility','visible');
        $('#gridButton').addClass('selected');
      }
      cookie.grid = me.grid;
      cookie.store(365,'/tamination');
    });
    $('#phantomsButton').click(function() {
      if (me.setPhantoms()) {
        me.setPhantoms(false);
        $('#phantomsButton').removeClass('selected');
      } else {
        me.setPhantoms(true);
        $('#phantomsButton').addClass('selected');
      }
      me.animate();
    });
    $('#numbersButton').click(function() {
      me.setNumbers(!me.setNumbers());
      if (me.numbers) {
        $('#numbersButton').addClass('selected');
        $('#couplesButton').removeClass('selected');
      }
      else
        $('#numbersButton').removeClass('selected');
      cookie.numbers = me.numbers;
      cookie.store(365,'/tamination');
    });
    $('#couplesButton').click(function() {
      me.setCouples(!me.setCouples());
      if (me.couples) {
        $('#couplesButton').addClass('selected');
        $('#numbersButton').removeClass('selected');
      }
      else
        $('#couplesButton').removeClass('selected');
      cookie.couples = me.couples;
      cookie.store(365,'/tamination');
    });
    $('#hexagonButton').click(function() {
      me.toggleHexagon();
      $('#hexagonButton').toggleClass('selected',me.hexagon);
      $('#bigonButton').removeClass('selected');
    });
    $('#bigonButton').click(function() {
      me.toggleBigon();
      $('#bigonButton').toggleClass('selected',me.bigon);
      $('#hexagonButton').removeClass('selected');
    });

    // Slider
    $('#buttonpanel').append('<div id="playslider" style="margin:10px 10px 0 10px"></div>');
    $('#playslider').slider({min: -200, max: this.beats*100, value: -200,
      slide: function(event,ui) {
        me.setBeat(ui.value/100);
      }});
    // Slider tick marks
    $('#buttonpanel').append('<div id="playslidertics" style="position: relative; height:10px; width:100%"></div>');
    $('#buttonpanel').append('<div id="playsliderlegend" style="color: black; position: relative; top:0; left:0; width:100%; height:20px"></div>');
    this.updateSliderMarks();

    // Bottom row of buttons
    $('#buttonpanel').append('<input type="button" class="appButton" id="rewindButton" value="&lt;&lt;"/>');
    $('#rewindButton').click(function() { me.rewind(); });
    $('#buttonpanel').append('<input type="button" class="appButton" id="prevButton" value="|&lt;"/>');
    $('#prevButton').click(function() { me.prev(); });
    $('#buttonpanel').append('<input type="button" class="appButton" id="backButton" value="&lt;"/>');
    $('#backButton').click(function() { me.backward(); });
    $('#buttonpanel').append('<input type="button" class="appButton" id="playButton" value="Play"/>');
    $('#playButton').click(function() { me.play(); });
    $('#buttonpanel').append('<input type="button" class="appButton" id="forwardButton" value="&gt;"/>');
    $('#forwardButton').click(function() { me.forward(); });
    $('#buttonpanel').append('<input type="button" class="appButton" id="nextButton" value="&gt;|"/>');
    $('#nextButton').click(function() { me.next(); });
    $('#buttonpanel').append('<input type="button" class="appButton" id="endButton" value="&gt;&gt;"/>');
    $('#endButton').click(function() { me.end(); });
    this.animationStopped = function()
    {
      $('#playButton').attr('value','Play');
    };

    //  Some browsers wrap the top row of buttons, this code is to fix that
    var fontsize = 14;
    while ($('#numbersButton').position().top > $('#slowButton').position().top) {
      fontsize--;
      $('.appButton').css('font-size',fontsize+'px');
    }


  }

  TamSVG.prototype.updateSliderMarks = function(nofractions) {
    //  Set the end of the slider to the current length of the animation
    $('#playslider').slider('option','max',this.beats*100);
    //  (Re)generate slider tic marks
    $('#playslidertics').empty();
    for (var i=-1; i<this.beats; i++) {
      var x = (i+2) * $('#buttonpanel').width() / (this.beats+2);
      $('#playslidertics').append('<div style="position: absolute; background-color: black; top:0; left:'+x+'px; height:100%; width: 1px"></div>');
    }
    // Add "Start", "End" and part/fraction numbers below slider
    $('#playsliderlegend').empty();
    if (this.beats > 2) {  // skip if no calls (sequencer)
      var startx = 2 * $('#buttonpanel').width() / (this.beats+2) - 50;
      var endx = this.beats * $('#buttonpanel').width() / (this.beats+2) - 50;
      $('#playsliderlegend').append('<div style="position:absolute; top:0; left:'+startx+'px; width:100px; text-align: center">Start</div>');
      $('#playsliderlegend').append('<div style="position:absolute;  top:0; left:'+endx+'px; width: 100px; text-align:center">End</div>');
      var offset = 0;
      for (var i in this.parts) {
        if (this.parts[i] > 0) {
          var t = '<font size=-2><sup>'+(Number(i)+1) + '</sup>/<sub>' + (this.parts.length+1) + '</sub></font>';
          if (nofractions || this.hasParts) {
            if (this.hasParts && i==0)
              t = '<font size=-2>Part 2</font>';
            else
              t = '<font size=-2>'+(Number(i)+2)+'</font>';
          }
          offset += this.parts[i];
          var x = (offset+2) * $('#buttonpanel').width() / (this.beats+2) - 20;
          $('#playsliderlegend').append('<div style="position:absolute; top:0; left:'+x+
              'px; width:40px; text-align: center">'+t+'</div>');
        }
      }
    }
  }

  window.TamSVG = TamSVG;  // because higher-level code is not modular
  return TamSVG;

});
