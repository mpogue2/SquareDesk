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


///////   CallContext class    //////////////
//An instance of the CallContext class is passed around calls
//to hold the working data - dancers, paths, and
//progress performing a call

//A CallContext can be created by any of
//* cloning another CallContext
//* copying an array of dancers
//* current state of TamSVG animation
//* XML formation (?)

define(['calls/call','callnotfounderror','formationnotfounderror',
        'calls/xmlcall','calls/codedcall'],
    function(Call,CallNotFoundError,FormationNotFoundError,
             XMLCall,CodedCall) {

  var CallContext = function(source)
  {
    this.callname = '';
    this.callstack = [];
    this.call = false;
    //  For cases where creating a new context from a source,
    //  get the dancers from the source and clone them.
    //  The new context contains the dancers in their current location
    //  and no paths.
    if (source instanceof CallContext) {
      this.dancers = source.dancers.map(function(d) {
        return new Dancer({dancer:d,computeOnly:true,active:d.active});
      });
    }

    else if (source instanceof TamSVG) {
      this.dancers = source.dancers.map(function(d) {
        return new Dancer({dancer:d,computeOnly:true,active:true});
      });
    }

    else if (source instanceof jQuery) {
      this.dancers = getDancers(source);
    }

    else if (source instanceof Array) {
      this.dancers = source.map(function(d) {
        return new Dancer({dancer:d,computeOnly:true,active:true});
      });
    }

  };
  Object.defineProperty(CallContext.prototype,'actives',{
    get: function() {
      return this.dancers.filter(function(d) { return d.active; });
    }
  });

  CallContext.prototype.clone = function(arg) {
    return new CallContext(arg ? arg : this);
  }

  /**
   *   Append the result of processing this CallContext to it source.
   *   The CallContext must have been previously cloned from the source.
   */
  CallContext.prototype.appendToSource = function() {
    this.dancers.forEach(function(d) {
      d.clonedFrom.path.add(d.path);
      d.clonedFrom.animateToEnd();
    });
  };

  CallContext.prototype.applyCalls = function(/* call names ...  */)
  {
    for (var i=0; i<arguments.length; i++) {
      var ctx = new CallContext(this);
      ctx.interpretCall(arguments[i]);
      ctx.performCall();
      ctx.appendToSource();
    }
  };

  /**
   * This is the main loop for interpreting a call
   * @param calltext  One complete call, lower case, words separated by single spaces
   */
  CallContext.prototype.interpretCall = function(calltext)
  {
    var err = new CallNotFoundError();
    //  Clear out any previous paths from incomplete parsing
    this.dancers.forEach(function(d) {
      d.path = new Path();
    });
    this.callname = '';
    //  If a partial interpretation is found (like 'boys' of 'boys run')
    //  it gets popped off the front and this loop interprets the rest
    while (calltext.length > 0) {
      //  Try chopping off each word from the end of the call until
      //  we find something we know
      if (!calltext.chopped().some(function(callname) {
        var success = false;
        //  First try to find an exact match in Taminations
        //  Then look for a code match
        try {
          success = this.matchXMLcall(callname);
        } catch (err2) {
          err = err2;
        }
        try {
          success = success || this.matchCodedCall(callname);
        } catch (err2) {
          err = err2;
        }
        if (success) {
          //  Remove the words we matched, break out of and
          //  the chopped loop, and continue if any words left
          calltext = calltext.replace(callname,'').trim();
          return true;
        }
      },this))
        //  Every combination from callwords.chopped failed
        throw err;
    }
  };

  //  Given a context and string, try to find an XML animation
  //  If found, the call is added to the context
  CallContext.prototype.matchXMLcall = function(calltext)
  {
    var found = false;
    var match = false;
    var ctx0 = this;
    var ctx = this;

    //  If there are precursors, run them first so the result
    //  will be used to match formations
    //  Needed for calls like "Explode And ..."
    if (this.callstack.length > 0) {
      ctx = new CallContext(this)
      ctx.callstack = this.callstack
      ctx.performCall()
    }

    //  If actives != dancers, create another call context with just the actives
    if (ctx.dancers.length != ctx.actives.length) {
      ctx = new CallContext(ctx.actives);
    }
    // Check that actives == dancers
    if (ctx.dancers.length == ctx.actives.length) {
      //  Try to find a match in the xml animations
      TAMination.searchCalls(calltext,{exact:true}).some(function(d) {
        //  Found xml file with call, now look through each animation
        return TAMination.searchCalls(calltext, {
          domain: $('tam',Call.xmldata[d.link]).toArray(),
          exact:true,
          keyfun: function(d) { return $(d).attr('title'); }}
        ).some(function(xelem) {
          found = true;
          //  Get the formation
          var f = $(xelem).find('formation');
          if (f.size() <= 0) {
            var fs = $(xelem).attr('formation');
            f = getNamedFormation(fs);
          }
          var sexy = $(xelem).attr('gender-specific');
          //  Try to match the formation to the current dancer positions
          var ctx2 = new CallContext(f);
          var mm = matchFormations(ctx,ctx2,sexy,false);
          if (mm) {
            //  Match found
            var call = new XMLCall();
            call.name = $(xelem).attr('title');
            call.xelem = xelem;
            call.xmlmap = mm;
            call.ctx2 = ctx2;
            ctx0.callstack.push(call);
            ctx0.callname += $(xelem).attr('title') + ' ';
            match = true;
            return true;  // break out of callindex loop
          }
          return false;  // match not found, continue
        });
      });
    }
    if (found && !match)
      //  Found the call but formations did not match
      throw new FormationNotFoundError();
    return match;
  };

  //  Once a mapping of two formations is found,
  //  this computes the difference between the two.
  CallContext.prototype.computeFormationOffsets = function(ctx2,mapping) {
    var dvbest = [];
    //  We don't know how the XML formation needs to be turned to overlap
    //  the current formation.  So do an RMS fit to find the best match.
    var bxa = [[0,0],[0,0]];
    this.actives.forEach(function(d1,i) {
      var v1 = d1.location;
      var v2 = ctx2.dancers[mapping[i]].location;
      bxa[0][0] += v1.x * v2.x;
      bxa[0][1] += v1.y * v2.x;
      bxa[1][0] += v1.x * v2.y;
      bxa[1][1] += v1.y * v2.y;
    });
    var mysvd = Math.svd22(bxa);
    var v = new AffineTransform(mysvd.V);
    var ut = new AffineTransform(Math.transposeArray(mysvd.U));
    var rotmat = v.preConcatenate(ut);
    //  Now rotate the formation and compute any remaining
    //  differences in position
    this.actives.forEach(function(d2,j) {
      var v1 = d2.location;
      var v2 = ctx2.dancers[mapping[j]].location.concatenate(rotmat);
      dvbest[j] = v1.subtract(v2);
    });
    return dvbest;
  }

  /**
   *   Reads an XML formation and returns array of the dancers
   * @param formation   XML formation element
   * @returns Array of dancers
   */
  function getDancers(formation) {
    var dancers = [];
    var i = 1;
    $('dancer',formation).each(function() {
      var d = new Dancer({
           tamsvg:tamsvg,
           computeOnly:true,
           gender:Dancer.genders[$(this).attr('gender')],
           x:-Number($(this).attr('y')),
           y:-Number($(this).attr('x')),
           angle:Number($(this).attr('angle'))+180,
           number:i++});
      dancers.push(d);
      d = new Dancer({
           tamsvg:tamsvg,
           computeOnly:true,
           gender:Dancer.genders[$(this).attr('gender')],
           x:Number($(this).attr('y')),
           y:Number($(this).attr('x')),
           angle:Number($(this).attr('angle')),
           number:i++});
      dancers.push(d);
    });
    return dancers;
  };

  /*
   * New algorithm to match formations
   * Match dancers relative to each other, rather than compare absolute positions
   * 2 cases
   *   1.  Dancers facing same or opposite directions
   *       - If dancers are lined up 0, 90, 180, 270 angles must match
   *       - Other angles match by quadrant
   *   2.  Dancers facing other relative directions (commonly 90 degrees)
   *       - Dancers must match quadrant or adj boundary
   *
   *
   *
   */
  function angleBin(a) {
    var retval = -1;
    if (Math.anglesEqual(a,0))
      retval = 0;
    else if (Math.anglesEqual(a,Math.PI/2))
      retval = 2;
    else if (Math.anglesEqual(a,Math.PI))
      retval = 4;
    else if (Math.anglesEqual(a,-Math.PI/2))
      retval = 6;
    else if (a > 0 && a < Math.PI/2)
      retval = 1;
    else if (a > Math.PI/2 && a < Math.PI)
      retval = 3;
    else if (a < 0 && a > -Math.PI/2)
      retval = 7;
    else if (a < -Math.PI/2 && a > -Math.PI)
      retval = 5;
    return retval;
  }

  function angleMask(b,fuzz)
  {
    var mask = 1<<b;
    if (fuzz) {
      mask |= 1<<((b+1)%8);
      mask |= 1<<((b+7)%8);
    }
    return mask;
  }

  function dancerRelation(ctx,d1,d2)
  {
    if (Math.anglesEqual(d1.start.angle,d2.start.angle) ||
        Math.anglesEqual(d1.start.angle,d2.start.angle+180)) {
      //  Case 1
      return angleBin(ctx.angle(d1,d2));
    } else {
      //  Case 2  TODO make fuzzy
      return angleBin(ctx.angle(d1,d2));
    }
  }

  function matchFormations(ctx1,ctx2,sexy,fuzzy)
  {
    if (ctx1.dancers.length != ctx2.dancers.length)
      return false;
    //  Find mapping using DFS
    var mapping = [];
    var rotated = [];
    ctx1.dancers.forEach(function(d,i) { mapping[i] = -1; rotated[i] = false; });
    var mapindex = 0;
    while (mapindex >= 0 && mapindex < ctx1.dancers.length) {
      var nextmapping = mapping[mapindex] + 1;
      while (nextmapping < ctx2.dancers.length) {
        mapping[mapindex] = nextmapping;
        mapping[mapindex+1] = nextmapping ^ 1;
        if (testMapping(ctx1,ctx2,mapping,mapindex,sexy,fuzzy))
          break;
        nextmapping++;
      }
      if (nextmapping >= ctx2.dancers.length) {
        //  No more mappings for this dancer
        mapping[mapindex] = mapping[mapindex+1] = -1;
        //  If fuzzy, try rotating this dancer
        if (fuzzy && !rotated[mapindex]) {
          ctx1.dancers[mapindex].rotateStartAngle(180.0)
          ctx1.dancers[mapindex+1].rotateStartAngle(180.0)
          rotated[mapindex] = true
        } else {
          rotated[mapindex] = false
          mapindex -= 2
        }
      } else {
        //  Mapping found
        mapindex += 2;
      }
    }
    return mapindex < 0 ? false : mapping;
  }

  function testMapping(ctx1,ctx2,mapping,i,sexy,fuzzy)
  {
    if (sexy && (ctx1.dancers[i].gender != ctx2.dancers[mapping[i]].gender))
      return false;
    return ctx1.dancers.every(function(d1,j) {
      if (mapping[j] < 0 || i == j)
        return true;
      var relq1 = dancerRelation(ctx1,ctx1.dancers[i],ctx1.dancers[j]);
      var relt1 = dancerRelation(ctx2,ctx2.dancers[mapping[i]],ctx2.dancers[mapping[j]]);
      var relq2 = dancerRelation(ctx1,ctx1.dancers[j],ctx1.dancers[i]);
      var relt2 = dancerRelation(ctx2,ctx2.dancers[mapping[j]],ctx2.dancers[mapping[i]]);
      //  If dancers are side-by-side, make sure handholding matches by checking distance
      if (!fuzzy && (relq1 == 2 || relq1 == 6)) {
        var d1 = ctx1.distance(i,j);
        var d2 = ctx2.distance(mapping[i],mapping[j]);
        if ((d1 < 2.1) != (d2 < 2.1))
          return false;
      }
      if (fuzzy) {
        var reldif1 = (relt1-relq1).abs;
        var reldif2 = (relt2-relq2).abs;
        return (reldif1==0 || reldif1==1 || reldif1==7) &&
               (reldif2==0 || reldif2==1 || reldif2==7);
      }
      else
        return relq1 == relt1 && relq2 == relt2;
    });
  }

  //  Given a context and string, try to find an animation generated by code
  //  If found, the call is added to the context
  CallContext.prototype.matchCodedCall = function(calltext)
  {
    var call = CodedCall.getCodedCall(calltext)
    if (call) {
      this.callstack.push(call);
      this.callname += call.name + ' ';
      return true;
    }
    return false;
  };


  //  Perform calls by popping them off the stack until the stack is empty.
  //  This doesn't run an animation, rather it takes the stack of calls
  //  and builds the dancer movements.
  CallContext.prototype.performCall = function() {
    this.analyze();
    //  Concepts and modifications primarily use the preProcess and
    //  postProcess methods
    this.callstack.forEach(function(c,i) {
      c.preProcess(this,i);
    },this);
    //  Core calls primarly use the performCall method
    this.callstack.forEach(function(c,i) {
      c.performCall(this,i);
    },this);
    this.callstack.forEach(function(c,i) {
      c.postProcess(this,i);
    },this);
  };

  //  Return max number of beats among all the dancers
  CallContext.prototype.maxBeats = function() {
    return this.dancers.reduce(function(v,d) {
      return Math.max(v,d.path.beats());
    },0);
  };

  //  Level off the number of beats for each dancer
  CallContext.prototype.levelBeats = function()
  {
    //  get the longest number of beats
    var maxbeats = this.maxBeats();
    //  add that number as needed by using the "Stand" move
    this.dancers.forEach(function(d) {
      var b = maxbeats - d.path.beats();
      if (b > 0) {
        var m = TamUtils.translate($('path[name="Stand"]',movedata));
        d.path.add(new Path(m).changebeats(b));
      }
    });
  };

  //  Find the range of the dancers current position
  //  For now we assume the dancers are centered
  //  and return a vector to the 1st quadrant rectangle point
  CallContext.prototype.bounds = function() {
    return this.dancers.map(function(d) { return d.location; })
                       .reduce(function(v1,v2) {
           return new Vector(Math.max(v1.x,v2.x),Math.max(v1.y,v2.y))
    });
  };

  //  Moves the start position of a group of dancers
  //  so they are centered around the origin
  CallContext.prototype.center = function()
  {
    var xave = this.dancers.reduce(function(a,b) { return a+b.startx; },0) / this.dancers.length;
    var yave = this.dancers.reduce(function(a,b) { return a+b.starty; },0) / this.dancers.length;
    this.dancers.forEach(function(d) {
      d.startx -= xave;
      d.starty -= yave;
      d.computeStart();
    });
  };


  //  See if the current dancer positions resemble a standard formation
  //  and, if so, snap to the standard
  var standardFormations = [
    "Normal Lines",
    "Normal Lines Compact",
    "Double Pass Thru",
    "Static Square",
    "Quarter Tag",
    "Tidal Line RH",
    "Diamonds RH Girl Points",
    "Diamonds RH PTP Girl Points",
    "Hourglass RH BP",
    "Galaxy RH GP",
    "Butterfly RH",
    "O RH",
    "Sausage RH"
  ];
  CallContext.prototype.matchStandardFormation = function() {
    //  Make sure newly added animations are finished
    this.dancers.forEach(function(d) {d.path.recalculate(); d.animateToEnd();});
    //  Work on a copy with all dancers active, mapping only uses active dancers
    var ctx1 = new CallContext(this);
    ctx1.dancers.forEach(function(d) {d.active = true; });
    var bestMapping = false;
    standardFormations.forEach(function(f) {
      var ctx2 = new CallContext(getNamedFormation(f));
      //  See if this formation matches
      var mapping = matchFormations(ctx1,ctx2,false,true);
      if (mapping) {
        //  If it does, get the offsets
        var offsets = ctx1.computeFormationOffsets(ctx2,mapping);
        var totOffset = offsets.reduce(function(s,v) { return s+v.length;}, 0.0 );
        if (!bestMapping || totOffset < bestMapping.totOffset)
          bestMapping = {
            name: f,  // only used for debugging
            mapping: mapping,
            offsets: offsets,
            totOffset: totOffset
        }
      }
    });
    if (bestMapping) {
      //alert("Matched "+bestMapping.name);
      this.dancers.forEach(function(d,i) {
        if (bestMapping.offsets[i].length > 0.1) {
          //  Get the last movement
          var m = d.path.movelist.pop();
          //  Transform the offset to the dancer's angle
          d.animateToEnd();
          var vd = bestMapping.offsets[i].rotate(-d.tx.angle);
          //  Apply it
          d.path.movelist.push(m.skew(-vd.x,-vd.y));
          d.animateToEnd();
        }
      });
    }
  };

  ////////////////////////////////////////////////////////////////////////////////////////
////Routines to analyze dancers
  CallContext.prototype.dancer = function(d)
  {
    return d instanceof Dancer ? d : this.dancers[d];
  };
//Distance between two dancers
//If d2 not given, returns distance from origin
  CallContext.prototype.distance = function(d1,d2)
  {
    var v = this.dancer(d1).location;
    if (d2 != undefined)
      v = v.subtract(this.dancer(d2).location);
    return v.length;
  };
//Angle of d2 as viewed from d1
//If angle is 0 then d2 is in front of d1
//If d2 is not given, returns angle to origin
//Angle returned is in the range -pi to pi
  CallContext.prototype.angle = function(d1,d2)
  {
    var v = new Vector(0,0);
    if (d2 != undefined)
      v = this.dancer(d2).location;
    var v2 = v.concatenate(this.dancer(d1).tx.getInverse());
    return v2.angle;
  };
  CallContext.prototype.isFacingIn = function(d)
  {
    var a = this.angle(d).abs
    return !Math.isApprox(a,Math.PI/2) && a < Math.PI/2;
  };
  CallContext.prototype.isFacingOut = function(d)
  {
    var a = this.angle(d).abs;
    return !Math.isApprox(a,Math.PI/2) && a > Math.PI/2;
  };

  //  Test if dancer d2 is directly in front, back. left, right of dancer d1
  CallContext.prototype.isInFront = function(d1,d2)
  {
    return d1 != d2 && Math.anglesEqual(this.angle(d1,d2),0);
  };
  CallContext.prototype.isInBack = function(d1,d2)
  {
    return d1 != d2 && Math.anglesEqual(this.angle(d1,d2),Math.PI);
  };
  CallContext.prototype.isLeft = function(d1,d2)
  {
    return d1 != d2 && Math.anglesEqual(this.angle(d1,d2),Math.PI/2);
  };
  CallContext.prototype.isRight = function(d1,d2)
  {
    return d1 != d2 && Math.anglesEqual(this.angle(d1,d2),Math.PI*3/2);
  };

  //  Return closest dancer that satisfies a given conditional
  CallContext.prototype.dancerClosest = function(d,f)
  {
    var ctx = this;
    return this.dancers.filter(f,ctx).reduce(function(best,d2) {
      return best == undefined || ctx.distance(d2,d) < ctx.distance(best,d)
      ? d2 : best;
    },undefined);
  };

  //  Return all dancers, ordered by distance, that satisfies a conditional
  CallContext.prototype.dancersInOrder = function(d,f)
  {
    var ctx = this;
    return this.dancers.filter(f,ctx).sort(function(a,b) {
      return ctx.distance(d,a) - ctx.distance(d,b);
    });
  };

  //  Return dancer directly in front of given dancer
  CallContext.prototype.dancerInFront = function(d)
  {
    return this.dancerClosest(d,function(d2) {
      return this.isInFront(d,d2);
    },this);
  };

  //  Return dancer directly in back of given dancer
  CallContext.prototype.dancerInBack = function(d)
  {
    return this.dancerClosest(d,function(d2) {
      return this.isInBack(d,d2);
    });
  };

  //  Return dancer directly to the right of given dancer
  CallContext.prototype.dancerToRight = function(d)
  {
    return this.dancerClosest(d,function(d2) {
      return this.isRight(d,d2);
    });
  };

  //  Return dancer directly to the left of given dancer
  CallContext.prototype.dancerToLeft = function(d)
  {
    return this.dancerClosest(d,function(d2) {
      return this.isLeft(d,d2);
    });
  };

  //  Return dancer that is facing this dancer
  CallContext.prototype.dancerFacing = function(d) {
    var d2 = this.dancerInFront(d);
    if (d2 != undefined && this.dancerInFront(d2) != d)
      d2 = undefined;
    return d2;
  };

  //  Return dancers that are in between two other dancers
  CallContext.prototype.inBetween = function(d1,d2)
  {
    return this.dancers.filter(function(d) {
      return d !== d1 && d !== d2 &&
      Math.isApprox(this.distance(d,d1)+this.distance(d,d2),
          this.distance(d1,d2));
    },this);
  };

  //  Return all the dancers to the right, in order
  CallContext.prototype.dancersToRight = function(d)
  {
    return this.dancersInOrder(d,function(d2) {
      return this.isRight(d,d2);
    });
  };

  //  Return all the dancers to the left, in order
  CallContext.prototype.dancersToLeft = function(d)
  {
    return this.dancersInOrder(d,function(d2) {
      return this.isLeft(d,d2);
    });
  };

  //  Return all the dancers in front, in order
  CallContext.prototype.dancersInFront = function(d) {
    return this.dancersInOrder(d,function(d2) {
      return this.isInFront(d,d2);
    })
  }

  //  Return all the dancers in back, in order
  CallContext.prototype.dancersInBack = function(d) {
    return this.dancersInOrder(d,function(d2) {
      return this.isInBack(d,d2);
    })
  }

  //  Return true if this dancer is in a wave or mini-wave
  CallContext.prototype.isInWave = function(d) {
    return d.partner &&
    Math.anglesEqual(this.angle(d,d.partner),this.angle(d.partner,d));
  };

  //  Return true if this dancer is part of a couple facing same direction
  CallContext.prototype.isInCouple = function(d) {
    return d.partner &&
    Math.anglesEqual(d.tx.angle,d.partner.tx.angle);
  }

  //  Return true if this is 4 dancers in a box
  CallContext.prototype.isBox = function()
  {
    //  Must have 4 dancers
    return this.dancers.length == 4 &&
    this.dancers.every(function(d) {
      //  Each dancer must have a partner
      //  and must be either a leader or a trailer
      return d.partner && (d.leader || d.trailer);
    });
  };

  //  Return true if this is 4 dancers in any kind of line, including waves
  CallContext.prototype.isLine = function()
  {
    //  Must have 4 dancers
    return this.dancers.length == 4 &&
    //  Each dancer must have right or left shoulder to origin
    this.dancers.every(function(d) {
      return Math.isApprox(this.angle(d).abs,Math.PI/2);
    },this) &&
    //  All dancers must either be on the y axis
    (this.dancers.every(function(d) {
      return Math.isApprox(d.location.x,0);
    }) ||
    //  or on the x axis
    this.dancers.every(function(d) {
      return Math.isApprox(d.location.y,0);
    }));
  };

  //  Return true if 8 dancers are in 2 general lines of 4 dancers each
  CallContext.prototype.isLines = function() {
    return this.dancers.every(function(d) {
      return this.dancersToRight(d).length + this.dancersToLeft(d).length == 3;
    },this);
  };

  //  Return true if 8 dancers are in 2 general columns of 4 dancers each
  CallContext.prototype.isColumns = function() {
    return this.dancers.every(function(d) {
      return this.dancersInFront(d).length + this.dancersInBack(d).length == 3;
    },this);
  };


  CallContext.prototype.analyze = function(beat)
  {
    this.dancers.forEach(function(d) {
      if (beat != undefined)
        d.animate(beat);
      else
        d.animateToEnd();
      d.beau = false;
      d.belle = false;
      d.leader = false;
      d.trailer = false;
      d.partner = false;
      d.center = false;
      d.end = false;
      d.verycenter = false;
    });
    var istidal = false;
    this.dancers.forEach(function(d1) {
      var bestleft = false;
      var bestright = false;
      var leftcount = 0;
      var rightcount = 0;
      var frontcount = 0;
      var backcount = 0;
      this.dancers.filter(function(d2) {
        return d1 !== d2;
      },this).forEach(function(d2) {
        //  Count dancers to the left and right, and find the closest on each side
        if (this.isRight(d1,d2)) {
          rightcount++;
          if (!bestright || this.distance(d1,d2) < this.distance(d1,bestright))
            bestright = d2;
        }
        else if (this.isLeft(d1,d2)) {
          leftcount++;
          if (!bestleft || this.distance(d1,d2) < this.distance(d1,bestleft))
            bestleft = d2;
        }
        //  Also count dancers in front and in back
        else if (this.isInFront(d1,d2))
          frontcount++;
        else if (this.isInBack(d1,d2))
          backcount++;
      },this);
      //  Use the results of the counts to assign belle/beau/leader/trailer
      //  and partner
      if (leftcount % 2 == 1 && rightcount % 2 == 0 &&
          this.distance(d1,bestleft) < 3) {
        d1.partner = bestleft;
        d1.belle = true;
      }
      else if (rightcount % 2 == 1 && leftcount % 2 == 0 &&
          this.distance(d1,bestright) < 3) {
        d1.partner = bestright;
        d1.beau = true;
      }
      if (frontcount % 2 == 0 && backcount % 2 == 1)
        d1.leader = true;
      else if (frontcount % 2 == 1 && backcount % 2 == 0)
        d1.trailer = true;
      //  Assign ends
      if (rightcount == 0 && leftcount > 1)
        d1.end = true;
      else if (leftcount == 0 && rightcount > 1)
        d1.end = true;
      //  The very centers of a tidal wave are ends
      //  Remember this special case for assigning centers later
      if (rightcount == 3 && leftcount == 4 ||
          rightcount == 4 && leftcount == 3) {
        d1.end = true;
        istidal = true;
      }
    },this);
    //  Analyze for centers and very centers
    //  Sort dancers by distance from center
    var dorder = [];
    this.dancers.forEach(function(d1) {
      dorder.push(d1);
    });
    var ctx = this;
    dorder.sort(function(a,b) {
      return ctx.distance(a) - ctx.distance(b);
    });
    //  The 2 dancers closest to the center
    //  are centers (4 dancers) or very centers (8 dancers)
    if (!Math.isApprox(this.distance(dorder[1]),this.distance(dorder[2]))) {
      if (this.dancers.length == 4) {
        dorder[0].center = true;
        dorder[1].center = true;
      } else {
        dorder[0].verycenter = true;
        dorder[1].verycenter = true;
      }
    }
    // If tidal, then the next 4 dancers are centers
    if (istidal) {
      [2,3,4,5].forEach(function(i) {
        dorder[i].center = true;
      });
    }
    // Otherwise, if there are 4 dancers closer to the center than the other 4,
    // they are the centers
    else if (this.dancers.length > 4 &&
        !Math.isApprox(this.distance(dorder[3]),this.distance(dorder[4]))) {
      [0,1,2,3].forEach(function(i) {
        dorder[i].center = true;
      });
    }

  };
  return CallContext;

});

