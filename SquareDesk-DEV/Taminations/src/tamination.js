/*

    Copyright 2017 Brad Christie

    This file is part of TAMinations.

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
var animations = 0;
var formations = 0;
var paths = 0;
var crossrefs = {};
var formationdata;
var movedata;

//  global (TODO make property of TAMination)
var calllistdata = [];

var prefix = '';
//  Make the links work from both taminations directory and its subdirectories
//  TODO Merge with tampage code
if (document.URL.search(/(info|b1|b2|ms|plus|adv|a1|a2|c1|c2|c3a|c3b)/) >= 0)
  prefix = '../';
if (document.URL.search(/embed/) >= 0)
  prefix = '';

var tam;  // for global access TODO remove
var TAMination = function(xmlpage,f,errfun,call)
{
  this.loadcount = 0;
  this.loadfinish = f;
  this.loadXML('formations.xml',function(a) { formationdata = a; });
  this.loadXML('moves.xml',function(a) { movedata = a; });
  if (xmlpage)
    this.loadXMLforAnimation(xmlpage,errfun);
  this.loadXML('callindex.xml',function (calls) {
    //  Pre-process the XML index for faster searching
    $('call[level!="Info"]',calls).filter(function() {
      calllistdata.push({
        title:$(this).attr('title'),
        link:$(this).attr('link'),
        sublevel:$(this).attr('sublevel'),
        languages:$(this).attr('languages')
      });
    });
  });

  TAMination.tam = this;
};

//  Code for singleton pattern
TAMination.tam = 0;
TAMination.getTam = function() {
  if (!TAMination.tam)
    TAMination.tam = new TAMination();
  return TAMination.tam;
}

TAMination.searchCalls = function(query,options)
{
  options = options || {};
  var domain = options.domain || calllistdata;
  var keyfun = options.keyfun || function(d) { return d.title; };
  var results = [];
  query = query.toLowerCase();
  query = query.replace(/[^a-zA-Z0-9 ]/g,"");
  //  Use upper case and dup numbers while building regex so expressions don't get compounded
  //  Through => Thru
  query = query.replace(/\bthrou?g?h?\b/g,"THRU");
  //  One and a half
  query = query.replace(/\b1.5\b/g,"ONEANDAHALF");
  query = query.replace(/\b1 12\b/g,"ONEANDAHALF");
  //  Process fractions 1/2 3/4 1/4 2/3
  query = query.replace(/\b12|(one.)?half\b/g,"(HALF|1122)");
  query = query.replace(/\b(three.quarters?|34)\b/g,"(THREEQUARTERS|3344)");
  query = query.replace(/\b((one.)?quarter|14)\b/g,"((ONE)?QUARTER|1144)");
  query = query.replace(/\btwo.thirds?\b/g,"(TWOTHIRDS|2233)");
  //  Process any other numbers
  query = query.replace(/\b(1|onc?e)\b/g,"(11|ONE)");
  query = query.replace(/\b(2|two)\b/g,"(22|TWO)");
  query = query.replace(/\b(3|three)\b/g,"(33|THREE)");
  query = query.replace(/\b(4|four)\b/g,"(44|FOUR)");
  query = query.replace(/\b(5|five)\b/g,"(55|FIVE)");
  query = query.replace(/\b(6|six)\b/g,"(66|SIX)");
  query = query.replace(/\b(7|seven)\b/g,"(77|SEVEN)");
  query = query.replace(/\b(8|eight)\b/g,"(88|EIGHT)");
  query = query.replace(/\b(9|nine)\b/g,"(99|NINE)");
  //  Accept single and plural forms of some words
  query = query.replace(/\bboys?\b/,"BOYS?");
  query = query.replace(/\bgirls?\b/,"GIRLS?");
  query = query.replace(/\bends?\b/,"ENDS?");
  query = query.replace(/\bcenters?\b/,"CENTERS?");
  query = query.replace(/\bheads?\b/,"HEADS?");
  query = query.replace(/\bsides?\b/,"SIDES");
  //  Accept optional "dancers" e.g. "head dancers" == "heads"
  query = query.replace(/\bdancers?\b/,"(DANCERS?)?");
  //  Misc other variations
  query = query.replace(/\bswap(\s+around)?\b/,"SWAP (AROUND)?");

  //  Finally repair the upper case and dup numbers
  query = query.toLowerCase().replace(/([0-9])\1/g, "$1").collapse();
  if (options.exact)
    query = "^" + query + "$";

  domain.forEach(function(d) {
    if (keyfun(d).toLowerCase().alphanums().match(query))
      results.push(d);
  });
  return results;
}

TAMination.selectLanguage = function(url)
{
  var translated = false;
  //  Skip if we already have a foreign language
  if (url.match(/(\w+\/\w+)\.html/)) {
    //  Find the entry in callindex for this call
    var link = url.match(/(\w+\/\w+)\.html/)[1];
    var userlang = 'lang-'+navigator.language.substr(0,2);
    calllistdata.forEach(function(d) {
      if (translated || !d.link || url.indexOf(d.link) < 0)
        return;  //  to the next forEach iteration
      //  Get the additional languages available for this call
      //  See if there's a match to the users' language
      if (d.languages && d.languages.indexOf(userlang) >= 0) {
        //  Remember the page
        translated = url.replace('html',userlang+'.html');
      }
    });
  }
  return translated;
}


//  Scan the doc for cross-references and load any found
TAMination.prototype.scanforXrefs = function(xmldoc) {
  var me = this;
  $('tamxref',xmldoc).each(function() {
    var link = $(this).attr('xref-link');
    me.loadXML(prefix+link+'.xml',function(b) {
      crossrefs[link] = b;
    });
  });
};

TAMination.prototype.loadXMLforAnimation = function(xmlpage,errfun) {
  var me = this;
  this.loadXML(xmlpage,function(a) { me.xmldoc = a; me.scanforXrefs(a); },errfun);
};

TAMination.loadXML =
TAMination.prototype.loadXML = function(url,f,e) {
  var me = this;
  this.loadcount++;
  $.ajax(url,{
    dataType:"xml",
    error: typeof e == 'function'
      ? function() { e(me); }
      : function(jq,stat,err) {
          alert("Unable to load "+url+" : "+stat+' '+err);
    },
    success: f,
    complete:function() {
      if (me) {
        me.loadcount--;
        if (me.loadcount == 0)
          me.init('');
      }
    }
  });
};

var locals = {
  'Abbrev' : {
    'ja' : '省略された'
  },
  'Full' : {
    'ja' : '遺漏なく'
  }
};
TAMination.localize = function(str)
{
  var retval = str;
  var k = locals[str];
  if (k) {
    var tr = k[navigator.language.substr(0,2)];
    if (tr)
      retval = tr;
  }
  return retval;
};



TAMination.prototype.init = function(call) {
  tam = this;
  tam.callnum = 0;
  if (typeof this.loadfinish == 'function') {
    this.loadfinish(this);
    this.loadfinish = 0;
  }
};

TAMination.prototype.selectAnimation = function(n) {
  this.callnum = n;
};

TAMination.prototype.selectByCall = function(call) {
  tam.call = call;
  tam.animations().each(function(n) {
    var str = $(this).attr("title") + "from" + $(this).attr("from");
    if ($(this).attr("group") != undefined)
      str = $(this).attr("title");
    //  First replace strips "(DBD)" et al
    //  Second strips all non-alphanums, not valid in html ids
    str = str.replace(/ \(DBD.*/,"").replace(/\W/g,"");
    if (str.toLowerCase() == tam.call.toLowerCase())
      tam.callnum = n;
  });
};

TAMination.prototype.animations = function() {
  return $('tam[display!="none"],tamxref',this.xmldoc);
};

TAMination.prototype.animation = function(n) {
  if (arguments.length == 0 || typeof n == 'undefined')
    n = this.callnum;
  if (typeof n == 'number')
    return this.animations().eq(n);
  return $(n);  // should be a tam element
};

TAMination.prototype.animationXref = function(n) {
  var a = this.animation(n);
  if (a.attr('xref-link') != undefined) {
    var s = '';
    if (a.attr('xref-title') != undefined)
      s += "[title|='"+a.attr('xref-title')+"']";
    if (a.attr('xref-from') != undefined)
      s += '[from="'+a.attr('xref-from')+'"]';
    a = $('tam'+s,crossrefs[a.attr('xref-link')]);
  }
  return a;
};

  //  Return the formation for the current animation.
  //  If the animation uses a named formation, it is looked up and
  //  the definition returned.
  //  The return value is an XML document element with dancers
TAMination.prototype.startingFormation = false;
TAMination.prototype.setFormation = function(f) { this.startingFormation = f; };
TAMination.prototype.getFormation = function() {
  if (this.startingFormation)  // sequence
    return getNamedFormation(this.startingFormation);
  var a = this.animationXref();
  var f = $(a).find("formation");
  var retval = undefined;
  if (f.length > 0) {
    //  Formation defined as an element in the animation
    retval = f;
  } else {
    //  Named formation as an attribute
    retval = getNamedFormation(a.attr('formation'));
  }
  return retval;
};


TAMination.prototype.getParts = function() {
  var a = this.animationXref();
  return a.attr("parts") ? a.attr("parts") : '';
};
TAMination.prototype.getFractions = function() {
  var a = this.animationXref();
  return a.attr("fractions") ? a.attr("fractions") : '';
};

TAMination.prototype.getTitle = function(n) {
  var a = this.animation(n);
  return a.attr("title");
};

TAMination.prototype.getComment = function(n) {
  return this.animation(n).find('taminator').text();
};

TAMination.prototype.getPath = function(a)
{
  var tam = this;
  var retval = [];
  $("path",this.animationXref(a)).each(function(n) {
    var onepath = TamUtils.translatePath(this);
    retval.push(onepath);
  });
  return retval;
};

TAMination.prototype.getNumbers = function() {
  var a = this.animationXref();
  // np is the number of paths not including phantoms (which raise it > 4)
  var np =  Math.min($('path',a).length,4);
  var retval = ['1','2','3','4','5','6','7','8'];
  var i = 0;
  $("path",a).each(function(n) {
    var n = $(this).attr('numbers');
    if (n) {  //  numbers given in animation XML
      var nn = n.split(/ /);
      retval[i*2] = nn[0];
      retval[i*2+1] = nn[1];
    }
    else if (i > 3) {  // phantoms
      retval[i*2] = ' ';
      retval[i*2+1] = ' ';
    }
    else {  //  default numbering
      retval[i*2] = (i+1)+'';
      retval[i*2+1] = (i+1+np)+'';
    }
    i += 1;
  });
  return retval;
};

TAMination.prototype.getCouples = function() {
  var a = this.animationXref();
  var retval = ['1','3','1','3','2','4','2','4','5','6','5','6',' ',' ',' ',' '];
  $("path",a).each(function(n) {
    var c = $(this).attr('couples');
    if (c) {
      var cc = c.split(/ /);
      retval[n*2] = cc[0];
      retval[n*2+1] = cc[1];
    }
  });
  return retval;
};

var TamUtils = new Object();
TamUtils.translate = function(item) {
  var tag = $(item).prop('tagName');
  tag = tag.toCapCase();
  return TamUtils['translate'+tag](item);
};

  //  Takes a path, which is an XML element with children that
  //  are moves or movements.
  //  Returns an array of JS movements
TamUtils.translatePath = function(path) {
  var retval = [];
  $(path).children().each(function() {
    retval = retval.concat(TamUtils.translate(this));
  });
  return retval;
};

  //  Takes a move, which is an XML element that references another XML
  //  path with its "select" attribute
  //  Returns an array of JS movements
TamUtils.translateMove = function(move) {
  //  First retrieve the requested path
  var movename = $(move).attr('select');
  var moveitem = $('path[name="'+movename+'"]',movedata).get(0);
  if (moveitem == undefined)
    throw new Error('move "'+movename+'" not defined');
  //  Get the list of movements
  var movements = TamUtils.translate(moveitem);
  //  Now apply any modifications
  //  Get any modifications
  var scaleX = $(move).attr("scaleX") != undefined
    ? Number($(move).attr("scaleX")) : 1;
  var scaleY = ($(move).attr("scaleY") != undefined
    ? Number($(move).attr("scaleY")) : 1) *
      ($(move).attr("reflect") ? -1 : 1);
  var offsetX = $(move).attr("offsetX") != undefined
    ? Number($(move).attr("offsetX")) : 0;
  var offsetY =  $(move).attr("offsetY") != undefined
    ? Number($(move).attr("offsetY")) : 0;
  var hands = $(move).attr("hands");
  //  Sum up the total beats so if beats is given as a modification
  //  we know how much to change each movement
  var oldbeats = movements.reduce(function(s,m) { return s+m.beats; },0);
  var beatfactor = $(move).attr("beats") != undefined
    ? Number($(move).attr("beats"))/oldbeats : 1.0;
  //  Now go through the movements applying the modifications
  //  The resulting list is the return value
  //  Note : skew probably works ok only if just one movement
  return movements.map(function(m) {
      return m.useHands(hands != undefined ? Movement.getHands(hands) : m.hands)
       .scale(scaleX,scaleY)
       .skew(offsetX,offsetY)
       .time(m.beats*beatfactor)
    });
};

  //  Accepts a movement element from a XML file, either an animation definition
  //  or moves.xml
  //  Returns an array of a single JS movement
TamUtils.translateMovement = function(move) {
  return [Movement.fromElement(move)];
};

TamUtils.getMove = function(movename) {
  return new Path(TamUtils.translatePath($('path[name="'+movename+'"]',movedata).get(0)));
}


//  The program calls this as the animation reaches each part of the call
//  If there's an element with id or class "<call><part>" or "Part<part>" it
//  be highlighted
function setPart(part)
{
  if ($('span').length > 0) {
    //  Remove current highlights
    $('span').removeClass('definition-highlight');
    $('span').filter('.'+currentcall+part+
                    ',#'+currentcall+part+
                    ',.Part'+part+',#Part'+part).addClass('definition-highlight')
    //  hide and show is needed to force Webkit browsers to show the change
                    .hide().show();
  }
}

function getNamedFormation(name)
{
  return $('formation[name="'+name+'"]',formationdata);
}
