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
var prefix = '';
//  Make the links work from both taminations directory and its subdirectories
if (document.URL.search(/(info|b1|b2|ms|plus|adv|a1|a2|c1|c2|c3a|c3b)/) >= 0)
  prefix = '../';
var tam = 0;
var currentmenu = 0;
var callnumber = -1;
var currentcall = "";
var titleob = 0;
var tamsvg = 0;
var cookie = 0;
var animationNumber = {};
var here = document.URL.split(/\?/)[0];
var search = document.URL.split(/\?/)[1];
if (search != null)
  search = search.split(/\&/);
else
  search = [];
var args = {};
for (var i=0; i<search.length; i++) {
  var arg1 = search[i].split(/=/);
  if (arg1.length > 1)
    args[arg1[0]] = arg1[1];
  else
    args[search[i].toLowerCase().replace(/\W/g,"")] = true;
}
var difficultText = [ '&nbsp;<font color="blue">&diams;</font>',
                      '&nbsp;<font color="red">&diams;&diams;</font>',
                      '&nbsp;<font color="black">&diams;&diams;&diams;</font>' ];

var calldata;

var levelselectors = {
      info: 'call[level="Info"]',
      basicandmainstream: 'call[level="Basic and Mainstream"]',
      basic1: 'call[sublevel="Basic 1"]',
      basic2: 'call[sublevel="Basic 2"]',
      mainstream: 'call[sublevel="Mainstream"]',
      plus: 'call[level="Plus"]',
      advanced: 'call[level="Advanced"]',
      a1: 'call[sublevel="A-1"]',
      a2: 'call[sublevel="A-2"]',
      c1: 'call[sublevel="C-1"]',
      c2: 'call[sublevel="C-2"]',
      c3a: 'call[sublevel="C-3A"]',
      c3b: 'call[sublevel="C-3B"]' };


var isMobile = {
    Android: function() {
        return /Android/i.test(navigator.userAgent);
    },
    BlackBerry: function() {
        return /BlackBerry/i.test(navigator.userAgent);
    },
    iOS: function() {
        return /iPhone|iPad|iPod/i.test(navigator.userAgent);
    },
    Windows: function() {
        return /IEMobile/i.test(navigator.userAgent);
    },
    any: function() {
        return (isMobile.Android() || isMobile.BlackBerry() || isMobile.iOS() || isMobile.Windows());
    }
};

//  Transfer to mobile app if on mobile
if (document.URL.search(/(b1|b2|ms|plus|a1|a2|c1|c2|c3a|c3b)/) >= 0) {
  if (isMobile.Android()) {
    document.location = document.URL.replace(/.*tamination/,'intent://view') +
         '#Intent;package=com.bradchristie.taminationsapp;scheme=Taminations;end">';
    //window.onfocus = function() { window.history.back(); }
    window.history.back();
  }
  else if (isMobile.iOS()) {
    document.location = document.URL.replace(/.*tamtwirlers/,'Taminations://www.tamtwirlers');
    //  iOS does not replace the browser URL, so..
    //  but con't do it immediatly or it cancels the previous line
    //  So instead go back when the window re-gains focus
    window.onfocus = function() { window.history.back(); }
  }
}


// Body onload function
var defwidth = 40;
var animwidth = 30;
$(document).ready(
  function() {
    $(".noshow").hide();
    //  Load the menus
    $("body").prepend('<div style="width:100%; height:48px" id="menudiv"></div>');
    $("#menudiv").hide();  // so we don't flash all the menus
    $("#menuload").addClass("menutitle");
    //  Insert title
    $("body").prepend(getTitle());
    //  Setup some search stuff here that can only be done after the title
    //  has been inserted
    $('#searchbox').focusin(function() {
      if (!$('#searchbox').hasClass('active')) {
        $('#searchbox').addClass('active').text('');
      }
      else
        $('#searchresultsdiv').show('slow');
    });
    //  Build the document structure
    var htmlstr = '<table id="deftable" cellspacing="0" cellpadding="4" width="100%">'+
                    '<tr valign="top">'+
                      '<td width="'+defwidth+'%">'+
                        '<div id="definition">'+
                          '<span class="level"></span>'+
                        '</div>'+
                      '</td>'+
                      '<td width="'+animwidth+'%" class="animation"><div id="svgcontainer"></div></td>'+
                      '<td class="animation">'+
                        '<div id="animationlist"></div>'+
                      '</td>'+
                    '</tr>'+
                  '</table>';

    //  Build the menus
    $("#menudiv").append('<table cellpadding="0" cellspacing="0" width="100%" summary="">'+
                         '<tr>'+
                         '<td id="info" class="menutitle" rowspan="2">Main Menu</td>'+
                         '<td id="basicandmainstream" class="menutitle" colspan="3">Basic and Mainstream</td>'+
                         '<td id="plus" class="menutitle" rowspan="2">Plus</td>'+
                         '<td id="advanced" class="menutitle" colspan="2">Advanced</td>'+
                         '<td id="c1" class="menutitle" rowspan="2">C-1</td>'+
                         '<td id="c2" class="menutitle" rowspan="2">C-2</td>'+
                         '<td id="c3a" class="menutitle" rowspan="2">C-3A</td>'+
                         '<td id="c3b" class="menutitle" rowspan="2">C-3B</td></tr>'+
                     '<tr><td id="basic1" class="menutitle">Basic 1</td>'+
                         '<td id="basic2" class="menutitle">Basic 2</td>'+
                         '<td id="mainstream" class="menutitle">Mainstream</td>'+
                         '<td id="a1" class="menutitle">A-1</td>'+
                         '<td id="a2" class="menutitle">A-2</td>'+
    '</tr></table>');
    $('#menudiv').append('<div class="menu"></div>');
    $('.menutitle').each(function() {
      $(this).click(function() {
        $('.menutitle').removeClass('selected');
        $(this).addClass('selected');
        //  Select the calls for this level
        var level = $(this).attr('id');
        var menu = $(levelselectors[level],calldata);
        var columns = Math.min(Math.ceil(menu.size()/25),4);
        var rows = Math.floor((menu.size() + columns - 1) / columns);
        var menuhtml = '<table cellpadding="0" cellspacing="0">';
        for (var r = 0; r < rows; r++) {
          menuhtml += '<tr>';
          for (var c = 0; c < columns; c++) {
            var mi = c*rows + r;
            if (mi < menu.size()) {
              var menuitem = $(menu.eq(mi));
              var onelink = menuitem.attr('link');
              if (onelink) {
                onelink += '.html';
                if (menuitem.attr('anim') != undefined)
                  onelink += '?' + menuitem.attr('anim');
                menuhtml += '<td onclick="document.location=\''+prefix+onelink+'\'">'+
                             menuitem.attr('title').escapeHtml()+'</td>';
              } else {
                menuhtml += '<td><br/><strong>--'+menuitem.attr('title')+'--</strong></td>';
              }
            }
          }
          menuhtml += '</tr>';
        }
        menuhtml += '</table>';
        $('.menu').empty();
        $('.menu').append(menuhtml);
        $(".menu td[onclick]").addClass("menuitem");
        $(".menuitem").hover(
            function() { $(this).addClass("menuitem-highlight"); },
            function() { $(this).removeClass("menuitem-highlight"); })
            .bind("mousedown",
                function() { return false; });

        //  Position off the screen to get the width without flashing it in the wrong position
        $(".menu").css("left","-1000px").show();
        var mw = $(".menu").width();
        var mh = $(".menu").height();
        var sw = $('body').width();
        var sh = $('body').height();
        var ml = $(this).offset().left;
        var mt = $(this).offset().top + $(this).height() + 4;
        //        Generally the menu goes below the title
        //        But if it pushes the menu off the right side of the screen shift it left
        if (ml+mw > sw)
          ml = sw - mw;
        //    and push it up if it flows below the screen
        if (mt+mh > sh)
          mt = sh - mh;
        $(".menu").css("top",mt+"px");
        $(".menu").css("left",ml+"px");
      });
    });

    //  Hide all the menus
    $(".menutitle > div").addClass("menutitlediv").hide();
    //  Remove any visible menus when user clicks elsewhere
    $(document).bind("mousedown",clearMenus);
    //  Everything's ready, show the menus
    $("#menuload").hide();
    $("#menudiv").show();
    //  Adjustments to make everything fit
    if ($('#how').height() > $('#info').height())
      $('#how').text('How');
    sizeBody();

    //  Finally insert the document structure and build the menu of animations
    $("#menudiv").after(htmlstr);
    //  Parse out base file name, allowing for country codes
    var docname = document.URL;
    if (!docname.match(/\.html/))
      docname += "index.html";
    docname = docname.match(/(\w+)(\.lang-..)?\.html/)[1];
    if (docname != 'index' && docname != 'sequence' && docname != 'embedinfo' &&
        docname != 'overview' && docname != 'howtouse' && docname != 'search' &&
        docname != 'trouble' && docname != 'download')
      tam = new TAMination(docname.extension('xml'),selectLanguage,'');
    else
      tam = new TAMination();
    tam.loadXML('calls.xml',function(a) {
      calldata = a;
    });
    showTranslations();
    //  end of menu load function

  });  // end of document ready function

function selectLanguage()
{
  var t = TAMination.selectLanguage(document.URL);
  if (t) {
    //  Fetch the page
    $.get(t,function(html) {
      //  Strip out what we don't need
      var j = html.indexOf('<body>');
      var k = html.indexOf('</body>');
      html = html.substr(j+7,k-j-7);
      //  And apply it
      $('.definition').html(html);
      generateAnimations();
    },'html');
  }
  else
    generateAnimations();
}

//  Un-hide translations for the user's language
function showTranslations()
{
  var userlang = navigator.language.substr(0,2);
  $('.lang-'+userlang).removeClass('lang-'+userlang);
}


function clearMenus()
{
  $(".menu").hide();
  $('.menutitle').removeClass('selected');
}

//  Generate the title above the menus
function getTitle()
{
  var titlelink = prefix + 'info/index.html';
  return '<div id="searchbox" contentEditable="true">Search Calls</div>' +
         '<div class="title">' +
         '<a href="http://www.tamtwirlers.org/">'+
         '<img height="72" border="0" align="right" src="'+prefix+'info/badge.gif"></a>'+
         '<span id="title"><a href="'+titlelink+'">Taminations</a></span></div>';
}

//  Set height of page sections to fit the window
function sizeBody()
{
  var menudivheight = $('#menudiv table').height();
  $('#menudiv').height(menudivheight);
  var h = $(window).height() - menudivheight - $('.title').height();
  $('#definition').height(h-10);
  $('#calllist').height(h);
  $('#animationlist').height(h-10);
  $('#iframeleft').height(h);
  $('#iframeright').height(h);
}

function svgSize()
{
  var aw = 100;
  var ah = 100;
  var menudivheight = $('#menudiv table').height();
  var h = $(window).height() - menudivheight - $('.title').height();
  var w = $(window).width();
  if (typeof h == "number" && typeof w == "number") {
    ah = h; // - 120;
    aw = (w * animwidth) / 100;
    if (ah - 130 > aw)
      ah = aw + 130;
    else
      aw = ah - 130;
  }
  aw = Math.floor(aw);
  ah = Math.floor(ah);
  return { width: aw, height: ah };
}

function generateAnimations()
{
  var showDiffLegend = false;
  callnumber = 0;
  var callname = '';
  //  Put the call definition in the document structure
  $("#deftable").nextAll().appendTo("#definition");
  $("#radio1").attr("checked",true);
  $("h2").first().prepend(getLevel());
  //  Show either full or abbreviated definition
  //  Load saved options from browser cookie
  cookie = new Cookie("TAMination");
  if ($('.abbrev').length + $('.full').length > 0) {
    if (cookie.full == "true") {
      $('.abbrev').hide();
      $('#full').addClass('selected');
      $('#abbrev').removeClass('selected');
    }
    else
      $('.full').hide();
    $('#full').click(function() {
      $('.abbrev').hide();
      $('.full').show();
      $('#full').addClass('selected');
      $('#abbrev').removeClass('selected');
      cookie.full = "true";
      cookie.store();
    });
    $('#abbrev').click(function() {
      $('.full').hide();
      $('.abbrev').show();
      $('#abbrev').addClass('selected');
      $('#full').removeClass('selected');
      cookie.full = "false";
      cookie.store();
    });
  }
  else
    $('.level > .appButton').hide();
  //  Build the selection list of animations
  var prevtitle = "";
  var prevgroup = "";
  $("#animationlist").empty();  //  disable to restore old animations
  tam.animations().each(function(n) {
    var name = tam.animationXref(n).attr('from');
    callname = $(this).attr('title') + 'from' + name;
    if ($(this).attr("group") != undefined) {
      if ($(this).attr("group") != prevgroup) {
        if (prevtitle)
          $("#animationlist").append('<div style="height:0.4em"/>');
        $("#animationlist").append('<span class="callname">'+$(this).attr("group")+'</span><br />');
      }
      name = $(this).attr('title').replace($(this).attr('group'),' ');
      callname = $(this).attr('title');
    }
    else if ($(this).attr("title") != prevtitle) {
      if (prevtitle)
        $("#animationlist").append('<div style="height:0.4em"/>');
      $("#animationlist").append('<span class="callname">'+$(this).attr("title")+" from</span><br />");
    }
    if (tam.animation(n).attr("difficulty") != undefined) {
      name = name + difficultText[Number(tam.animation(n).attr("difficulty"))-1];
      showDiffLegend = true;
    }
    //  First replace strips "(DBD)" et al
    //  Second strips all non-alphanums, not valid in html ids
    callname = callname.replace(/ \(DBD.*/,"").replace(/\W/g,"");
    animationNumber[callname.toLowerCase()] = n;
    prevtitle = $(this).attr("title");
    prevgroup = $(this).attr('group');
    $('<input name="tamradio" type="radio" class="selectRadio"/>').appendTo("#animationlist")
      .click(function() { PickAnimation(n); });
    $("#animationlist").append(' <a class="selectAnimation" href="javascript:PickAnimation('+n+')">'+
          name + '</a>');
    if ($("path",tam.animationXref(n)).length == 2)
      $("#animationlist").append(' <span class="comment">(4 dancers)</span>');
    $("#animationlist").append('<br />');
  });
  $(".selectAnimation").hover(function() { $(this).addClass("selectHighlight"); },
                              function() { $(this).removeClass("selectHighlight"); });
  //  Add any comment below the animation list
  $('#animationlist').append('<br /><div id="comment" class="comment">' +
                      $('comment *',animations).text() + '</div>');
  //  Passed-in arg overrides cookie
  if (args.svg == 'false' || args.svg == 'true') {
    cookie.svg = args.svg;
    cookie.store();
  }
  if (showDiffLegend)
    $('#animationlist').append(
        difficultText[0]+' Common - New dancers should look at these.<br/>'+
        difficultText[1]+' More difficult - For more experienced dancers.<br/>'+
        difficultText[2]+' Most difficult - For expert dancers.<br/><br/>'
        );

  //  Insert the SVG container
  if (tam.animations().size() > 0) {
    $('#svgcontainer').height($('#svgcontainer').width()+100).width(svgSize().width);
    var dims = svgSize();
    var svgdim = dims.width;
    var svgstr='<div id="svgdiv" '+'style="width:'+svgdim+'px; height:'+svgdim+'px;"></div>';
    $("#svgcontainer").append(svgstr);
    $('#svgdiv').svg({onLoad:function(x) {
        var t = new TamSVG(x);
        t.setPart = setPart;
      }
    });
    //  Make sure the 1st radio button is turned on
    if ($(".selectRadio").get(callnumber))
      $(".selectRadio").get(callnumber).checked = true;
    $("#animationlist > a").eq(callnumber).addClass("selectedHighlight");
    $("#animationlist > a").eq(callnumber).prevAll('.callname:first').addClass("selectedHighlight");
    currentcall = $(tam.animations()).eq(callnumber)
        .attr("title").replace(/ \(DBD.*/,"").replace(/\W/g,"");
  } else {
    //  no animations
    $('#svgcontainer').append("<h3><center>No animation for this call.</center></h3>");
  }
  sizeBody();
  if (tamsvg) {
    tamsvg.generateButtonPanel();
    PickAnimation(0);
    //  If a specific animation is requested in the URL, switch to it
    for (var arg in args) {
      if (animationNumber[arg] != undefined) {
        callnumber = animationNumber[arg];
        callname = arg;
        PickAnimation(callnumber);
      }
    }
  }
}


function PickAnimation(n)
{
  tam.selectAnimation(n);
  if (tamsvg) {
    tamsvg.stop();
    $('#svgcontainer').empty();
    $('#svgdiv').empty();
    var dims = svgSize();
    var svgdim = dims.width;
    var svgstr='<div id="svgdiv" '+'style="width:'+svgdim+'px; height:'+svgdim+'px;"></div>';
    var commentstr = '<div style="color: black; position:relative; top:0; left;0"><div id="commentdiv" style="position:absolute; top:0; left:0"> </div></div>';
    var commentback = '<div style="position:relative; top:0; left;0"><div id="commentback" style="position:absolute; top:0; left:0"></div></div>';
    $("#svgcontainer").append(commentback);
    $("#svgcontainer").append(commentstr);
    $('#commentdiv').text(tam.getComment());
    $('#commentback').width('100%').height($('#commentdiv').height()).css('background-color','white').css('opacity','0.7');
    $("#svgcontainer").append(svgstr);
    $('#svgcontainer > div').css('top',$('#svgdiv').height()-$('#commentdiv').height());
    $('#svgdiv').svg({onLoad:TamSVG});
    tamsvg.setAnimationListener(function(beat) {
      var op = beat < 0 ? -beat/2.0 : 0;
      $('#commentback').css('opacity',op*0.7);
      $('#commentdiv').css('opacity',op);
    });
  }
  $('.selectedHighlight').removeClass("selectedHighlight");
  $("#animationlist > a").eq(n).addClass("selectedHighlight");
  $(".selectRadio").get(n).checked = true;
  //  Note that :first gets the 'first previous' when used with prevAll
  $("#animationlist > a").eq(n).prevAll('.callname:first').addClass("selectedHighlight");
  if (tamsvg) {
    tamsvg.generateButtonPanel();
    tamsvg.setPart = setPart;
    setPart(0);
  }
  currentcall = $(tam.animations()).eq(n)
      .attr("title").replace(/ \(DBD.*/,"");
  //  Strip out spaces for synchronizing with parts from animation
  currentcall = currentcall.replace(/\W/g,"");
}

function getLevel()
{
  var levelstring = " ";
  if (document.URL.match(/\/b1\//))
    levelstring = "Basic 1";
  if (document.URL.match(/\/b2\//))
    levelstring = "Basic 2";
  if (document.URL.match(/\/ms\//))
    levelstring = "Mainstream";
  if (document.URL.match(/\/plus\//))
    levelstring = "Plus";
  if (document.URL.match(/\/adv\//))
    levelstring = "Advanced";
  if (document.URL.match(/\/a1\//))
    levelstring = "A-1";
  if (document.URL.match(/\/a2\//))
    levelstring = "A-2";
  if (document.URL.match(/\/c1\//))
    levelstring = "C-1";
  if (document.URL.match(/\/c2\//))
    levelstring = "C-2";
  if (document.URL.match(/\/c3a\//))
    levelstring = "C-3A";
  if (document.URL.match(/\/c3b\//))
    levelstring = "C-3B";
  return '<span class="level">'+levelstring+'<br/><br/>' +
         '<span class="appButton selected" id="abbrev">'+
         TAMination.localize('Abbrev') +'</span> '+
         '<span class="appButton" id="full">'+
         TAMination.localize('Full')+'</span></span>';
}
