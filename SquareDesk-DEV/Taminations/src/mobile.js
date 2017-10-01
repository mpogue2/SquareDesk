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

var currentcall = '';  // needed for tamination.js
var callnamedict = {};
var calldata;

/*
 *    Themes
 *    a  default (not used?)
 *    b     not used
 *    c  default list items
 *    d  Green headers
 *    e     not used
 *    f  Basic and Mainstream
 *    g  Basic 1 / Basic 2 / Mainstream
 *    h  Plus and common call
 *    i  Advanced
 *    j  A-1 / A-2 and harder call
 *    k  Challenge and expert call
 *    l  C-1 / C-2 / C-3A / C-3B
 *    m
 *    n
 *    o
 *    p
 */

var leveldata = [
  { name:"Basic and Mainstream", dir:"bms", selector:"level='Basic and Mainstream'", theme:'f' },
  { name:"Basic 1", dir:"b1", selector:"sublevel='Basic 1'", theme:'g' },
  { name:"Basic 2", dir:"b2", selector:"sublevel='Basic 2'", theme:'g' },
  { name:"Mainstream", dir:"ms", selector:"sublevel='Mainstream'", theme:'g' },
  { name:"Plus", dir:"plus", selector:"level='Plus'", theme:'h' },
  { name:"Advanced", dir:"adv", selector:"level='Advanced'", theme:'i' },
  { name:"A-1", dir:"a1", selector:"sublevel='A-1'", theme: 'j' },
  { name:"A-2", dir:"a2", selector:"sublevel='A-2'", theme:'j' },
  { name:"Challenge", dir:"challenge", selector:"level='Challenge'", theme:'k' },
  { name:"C-1", dir:"c1", selector:"sublevel='C-1'", theme:'l' },
  { name:"C-2", dir:"c2", selector:"sublevel='C-2'", theme:'l' },
  { name:"C-3A", dir:"c3a", selector:"sublevel='C-3A'", theme:'l' },
  { name:"C-3B", dir:"c3b", selector:"sublevel='C-3B'", theme:'l' },
  { name:"Index of All Calls", dir:"all", selector:"level!='Info'" }
];

function findLevel(str)
{
  return leveldata.filter(function(j) {
    return j.dir == str || j.name == str;
  })[0];
}

TAMination.loadXML('calls.xml',function(a) { calldata = a; });

//  This function is called just once after the html has loaded
//  Other mobile-specific callbacks are called for each page change
$(document).ready(function() {
  //  Add the functions for the buttons.
  //  Do it here because it should only be done once.
  $('#rewindButton').bind('tap',function(event,ui) {
    tamsvg.rewind();
  });
  $('#backButton').bind('tap',function(event,ui) {
    tamsvg.backward();
  });
  $('#playButton').bind('tap',function(event,ui) {
    tamsvg.play();
    $('#playButton').button('refresh');
  });
  $('#forwardButton').bind('tap',function(event,ui) {
    tamsvg.forward();
  });
  $('#endButton').bind('tap',function(event,ui) {
    tamsvg.end();
  });
  $('#optionsButton').bind('tap',function(event,ui) {
    $.mobile.changePage('#optionspage');
  });
  $('#defButton').bind('tap',function(event,ui) {
    $.mobile.changePage('#definitionpage');
  });
  $('#calltitle').bind('tap',function(event,ui) {
    $.mobile.changePage('#definitionpage');
  });
  $('#slowButton').bind('change',function(event,ui) {
    tamsvg.slow();
  });
  $('#normalButton').bind('change',function(event,ui) {
    tamsvg.normal();
  });
  $('#fastButton').bind('change',function(event,ui) {
    tamsvg.fast();
  });
  $('#loopButton').bind('change',function(event,ui) {
    tamsvg.setLoop($('#loopButton:checked').length==1);
  });
  $('#gridButton').bind('change',function(event,ui) {
    tamsvg.setGrid($('#gridButton:checked').length==1);
  });
  $('#pathsButton').bind('change',function(event,ui) {
    tamsvg.setPaths($('#pathsButton:checked').length==1);
  });

  $('#numbersNoneButton').bind('change',function(event,ui) {
    tamsvg.setNumbers(false);
    tamsvg.setCouples(false);
  });
  $('#numbersDancersButton').bind('change',function(event,ui) {
    tamsvg.setNumbers(true);
  });
  $('#numbersCouplesButton').bind('change',function(event,ui) {
    tamsvg.setCouples(true);
  });

  $('#geometryNoneButton').bind('change',function(event,ui) {
    if (tamsvg.setHexagon())
      tamsvg.setHexagon(false);
    if (tamsvg.setBigon())
      tamsvg.setBigon(false);
  });
  $('#geometryHexagonButton').bind('change',function(event,ui) {
    tamsvg.setHexagon(true);
  });
  $('#geometryBigonButton').bind('change',function(event,ui) {
    tamsvg.setBigon(true);
  });

  $('#hexagonButton').bind('change',function(event,ui) {
    var ishex = $('#hexagonButton:checked').length==1;
    tamsvg.setHexagon(ishex);
    if (ishex) {
      $('#bigonButton').removeAttr('checked').checkboxradio('refresh');
    }
    else
      $('#numbersButton').checkboxradio('enable');
  });
  $('#bigonButton').bind('change',function(event,ui) {
    var isbigon = $('#bigonButton:checked').length==1;
    tamsvg.setBigon(isbigon);
    if (isbigon) {
      $('#hexagonButton').removeAttr('checked').checkboxradio('refresh');
    }
    else
      $('#numbersButton').checkboxradio('enable');
  });

  $('#phantomsButton').bind('change',function(event,ui){
    tamsvg.setPhantoms($('#phantomsButton:checked').length==1);
  });

});

//  This is needed to keep the green color for the level header
$(document).bind('mobileinit',function()
  {
    $.mobile.page.prototype.options.headerTheme = "d";
    $.mobile.listview.prototype.options.headerTheme = "d";
    $.mobile.pushStateEnabled = false;
    $.mobile.defaultPageTransition = 'slide';
  });


function sizeFirstMobilePage()
{
  // Adjust level display items to fit window
  var h = window.innerHeight ? window.innerHeight : document.body.offsetHeight;
  h = h - 50;  // hack
  $('#content').height(h);
  //h = $('#content div div').height();
  h = Math.floor(h/15);
  $('#content div div').height(h);
}


//  This generates the list of calls for a specific level
$(document).bind('pagechange', function(ev,data) {
    if (data.toPage.attr('id') == 'calllistpage') {
      var leveldata = findLevel(data.options.pageData.dir);
      $('#leveltitle').empty().text(leveldata.name);
      $('#calllist').empty();
      $('call['+leveldata.selector+']',calldata).each(function() {
        if ($(this).attr('level') == 'Info')
          return;
        var text = $(this).attr('title');
        var link = $(this).attr('link');
        var level = $(this).attr('sublevel');
        var theme = findLevel(level).theme;
        var htmlpage = encodeURIComponent(link.extension('html'));
        var xmlpage = link.extension('xml');
        if ($(this).attr('anim') == undefined)
          callnamedict[xmlpage] = text;
        var html = '<li data-theme="'+theme+'" data-icon="false">' +
        '<a href="#animlistpage&'+htmlpage+'">'+text+'</a>'+
        '<span class="ui-li-count">' + level + '</span>' +
        '</li>';
        $('#calllist').append(html);
      });
      $('#calllist').listview('refresh');
    }
    if (typeof data.options.n != "undefined") {
      generateAnimation(Number(n));
      bindControls();
    }

 });


//  Fetch xml that lists the animations for a specific call
//  Then build a menu
function loadcall(options,htmlpage)
{
  if (htmlpage.length > 0) {
    htmlpage = decodeURIComponent(htmlpage);
    //  Load the xml file of animations
    var xmlpage = htmlpage.extension('xml');
    new TAMination(xmlpage,function(tam) {
      var prevtitle = "";
      var prevgroup = "";
      var page = $('#animlistpage');
      var content = page.children(":jqmData(role=content)");
      content.empty();
      var html = '<ul data-role="listview">';
      var showDiffLegend = false;

      var tpage = TAMination.selectLanguage(htmlpage) || htmlpage;
      //  Some browsers do better loading the definition as xml, others as html
      //  So we will try both
      $.ajax({url:tpage, datatype:'xml', success:function(a) {
        if ($('body',a).size() > 0) {
          $('#definitioncontent').empty().append($('body',a).children());
          repairDefinition(tpage);
        }
        else {
          $.ajax({url:tpage, datatype:'html', success:function(a) {
            $('#definitioncontent').empty().append(a.match(/<body>((.|\s)*)<\/body>/)[1]);
            repairDefinition(tpage);
          }});
        }
      }});

      tam.animations().each(function(n) {
        var callname = $(this).attr('title') + 'from' + $(this).attr('from');
        var name = $(this).attr('from');
        if ($(this).attr("group") != undefined) {
          if ($(this).attr("group") != prevgroup)
            html += '<li data-role="list-divider">'+$(this).attr("group")+"</li>";
          name = $(this).attr('title').replace($(this).attr('group'),' ');
          callname = $(this).attr('title');
        }
        else if ($(this).attr("title") != prevtitle)
          html += '<li data-role="list-divider">'+$(this).attr("title")+" from</li>";
        var theme = "c";
        if (tam.animation(n).attr("difficulty") != undefined) {
          var j = Number($(this).attr("difficulty"));
          theme = ['c','h','j','k'][j];
          showDiffLegend = true;
        }
        //  First replace strips "(DBD)" et al
        //  Second strips all non-alphanums, not valid in html ids
        callname = callname.replace(/ \(DBD.*/,"").replace(/\W/g,"");
        //animationNumber[callname] = n;
        prevtitle = $(this).attr("title");
        prevgroup = $(this).attr('group');
        if ($("path",this).length == 2)
          name += ' (4 dancers)';
        //  Finally add the line for the call
        //  By setting the style "white-space: normal" we override jQuery Mobile
        //  which always wants to truncate the text
        html += '<li data-theme="'+theme+'" data-icon="false"><a style="white-space:normal" href="#animation-'+n+'">'+name+'</a></li>';
      });
      content.html(html);
      page.page();
      content.find(':jqmData(role=listview)').listview();
      $('#calltitle').empty().text(callnamedict[xmlpage]);
      $('#deftitle').empty().text(callnamedict[xmlpage]);
      if (showDiffLegend)
        $('#difficultylegend').show();
      else
        $('#difficultylegend').hide();
      var dir = xmlpage.split('/')[0];
      var level = findLevel(dir).name;
      //  Set the level icon to go back to the specific level for this call
      $('.levelbutton').attr('href','#calllistpage?dir='+dir).empty().text(level);
      $.mobile.changePage($('#animlistpage'),options);
    });
  }
  else
    $.mobile.changePage($('#animlistpage'),options);
}

function repairDefinition(htmlpage)
{
  //  Repair image locations
  $('#definitioncontent img').each(function(i) {
    $(this).attr('src',htmlpage.match(/(.*)\//)[1]+'/'+$(this).attr('src'));
  });
  //  Strip out any links
  $('#definitioncontent a').each(function(i) {
    $(this).replaceWith($(this).text());
  });
}

$(document).bind('pagebeforechange',function(e,data)
{
  if (typeof tamsvg == 'object') {
    tamsvg.stop();
    //$('#playButton').button('refresh');
  }
  if (typeof data.toPage == 'string') {
    var u = $.mobile.path.parseUrl(data.toPage);
    if (u.hash.indexOf('#animlistpage') == 0) {
      htmlpage = u.hash.substring(14);
      loadcall(data.options,htmlpage);
      e.preventDefault();
    }
    else if (u.hash.indexOf('#animation') == 0) {
      n = u.hash.substring(11);
      if (n.match(/\d+/)) {
        data.options.n = n;
        $.mobile.changePage($('#animationpage'),data.options);
        //generateAnimation(n);
        //bindControls();
      }
      else if (typeof tamsvg != 'object') {
        // "refresh" - go back to main page
        $.mobile.changePage('#level');
        e.preventDefault();
      }
    }
    else if (u.hash.indexOf('#optionspage') == 0) {
      if (typeof tamsvg != 'object') {
        $.mobile.changePage('#level');
        e.preventDefault();
      } else {
        if (tamsvg.isSlow())
          $('#slowButton').attr('checked','checked');
        if (tamsvg.isNormal())
          $('#normalButton').attr('checked','checked');
        if (tamsvg.isFast())
          $('#fastButton').attr('checked','checked');
        if (tamsvg.setLoop())
          $('#loopButton').attr('checked','checked');
        else
          $('#loopButton').removeAttr('checked');
        if (tamsvg.setGrid())
          $('#gridButton').attr('checked','checked');
        else
          $('#gridButton').removeAttr('checked');
        if (tamsvg.setPaths())
          $('#pathsButton').attr('checked','checked');
        else
          $('#pathsButton').removeAttr('checked');

        if (tamsvg.setNumbers())
          $('#numbersDancersButton').attr('checked','checked');
        else if (tamsvg.setCouples())
          $('#numbersCouplesButton').attr('checked','checked');
        else
          $('#numbersNoneButton').attr('checked','checked');

        if (tamsvg.setHexagon())
          $('#geometryHexagonButton').attr('checked','checked');
        else if (tamsvg.setBigon())
          $('#geometryBigonButton').attr('checked','checked');
        else
          $('#geometryNoneButton').attr('checked','checked');

        if (tamsvg.setPhantoms())
          $('#phantomsButton').attr('checked','checked');
        else
          $('#phantomssButton').removeAttr('checked');
      }
    }
  }
});



//  Code to build animation
var args = {};
var definitiondoc;
function svgSize()
{
  var aw = 90;
  var ah = 90;
  var h = window.innerHeight ? window.innerHeight : document.body.offsetHeight;
  var w = window.innerWidth ? window.innerWidth : document.body.offsetWidth;
  if (typeof h == "number" && typeof w == "number") {
    h = h - $('#animform').height() - $('#animheader').height();
    aw = ah = (h > w ? w : h)-10;
  }
  aw = Math.floor(aw);
  ah = Math.floor(ah);
  return { width: aw, height: ah };
}

function generateAnimation(n)
{
  tam.selectAnimation(n);
  $('#animtitle').empty().text(tam.getTitle());
  var dims = svgSize();
  svgstr='<div id="svgdiv" '+
            'style="width:'+dims.width+'px; height:'+dims.height+'px; overflow:hidden">'+
            '</div>';
  $("#animationcontent").empty().append(svgstr).width(dims.width).height(dims.height);
  $('#svgdiv').svg({onLoad:TamSVG});
  tamsvg.hideTitle();
  //  Force the slider to align with the scale below it
  var o = $('.ui-slider-track').offset();
  var p = $('#playslidertics').offset();
  $('.ui-slider-track').width($('#playslidertics').width());
  o.left = p.left;
  $('.ui-slider-track').offset(o);
  //  And hide the box showing the numerical value, which makes no sense here
  $('.ui-slider-input').hide();
  //  Rest of code logic from tampage.js
  //  Add tic  marks and labels to slider
  $('#playslidertics').empty();
  for (var i=-1; i<tamsvg.beats; i++) {
    var x = (i+2) * $('#playslidertics').width() / (tamsvg.beats+2);
    $('#playslidertics').append('<div style="position: absolute; background-color: white; top:0; left:'+x+'px; height:100%; width: 1px"></div>');
  }
  //  "Start", "End" and part numbers below slider
  $('#playsliderlabels').empty();
  var startx = 2 * $('#playsliderlabels').width() / (tamsvg.beats+2) - 50;
  var endx = tamsvg.beats * $('#playsliderlabels').width() / (tamsvg.beats+2) - 50;
  $('#playsliderlabels').append('<div style="position:absolute; top:0; left:'+startx+'px; width:100px; text-align: center">Start</div>');
  $('#playsliderlabels').append('<div style="position:absolute;  top:0; left:'+endx+'px; width: 100px; text-align:center">End</div>');
  var offset = 0;
  for (var i in tamsvg.parts) {
    if (tamsvg.parts[i] > 0) {
      var t = '<font size=-2><sup>'+(Number(i)+1) + '</sup>/<sub>' + (tamsvg.parts.length+1) + '</sub></font>';
      offset += tamsvg.parts[i];
      var x = (offset+2) * $('#playsliderlabels').width() / (tamsvg.beats+2) - 20;
      $('#playsliderlabels').append('<div style="position:absolute; top:0; left:'+x+
          'px; width:40px; text-align: center">'+t+'</div>');
    }
  }
}


function bindControls()
{
  var wasRunning = false;
  var isSliding = false;
  $('#animslider').attr('max',Math.floor(tamsvg.beats*100))
                  .slider({start: function() {
                    wasRunning = tamsvg.running;
                    isSliding = true;
                    tamsvg.stop();
                    },
                    stop: function() {
                      if (wasRunning)
                        tamsvg.start();
                      isSliding = false;
                    }}
                  );
  tamsvg.animationStopped = function()
  {
    $('#playButton').attr('value','Play').button('refresh');
  };
  $('#animslider').change(function()
    {
      if (isSliding) {
        tamsvg.setBeat($('#animslider').val()/100);
        tamsvg.paint();
      }
    });
  tamsvg.setAnimationListener(function(beat) {
    if (!isSliding)
      $('#animslider').val(Math.floor(beat*100)).slider('refresh');
  });
}
