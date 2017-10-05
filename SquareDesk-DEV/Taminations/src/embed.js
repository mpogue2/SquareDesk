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

/*  This is the code to embed an animation in an iframe  */

require(['jquery','tamination','tamsvg','jqueryui'],function($) {

  //  Pull the call name and any settings out of the URL
  var search = document.URL.split(/\?/)[1];
  var params = [];
  if (search != null) {
    params = search.split(/\&/);
    var page = params[0].split(/\./)[0];
    var call = params[0].split(/\./)[1];
    if (call == null) {
      params = search[0];
      call = '';
    }
  }
  page += '.xml';
  args = search.toArgs();

  //  This sizes the animation to fit the frame
  function dims() {
    var w = window.innerWidth;
    if (!w)
      w = document.body.scrollWidth;
    var h = window.innerHeight;
    if (!h)
      h = document.body.clientHeight;
    if (!h)
      h = w + 100;
    return {
      w : w,
      h : h
    };
  }

  //  This puts the animation in the frame
  function buildAnimation(tam) {
    tam.selectByCall(call);
    var d = dims();
    var svgstr = '<div id="svgcontainer"><div id="svgdiv" ' + 'style="width:'
    + d.w + 'px; height:' + (d.h - 120)
    + 'px; background-color:#ffffc0"></div></div>';
    $("body").prepend(svgstr);
    $('#svgdiv').svg({
      onLoad : function(svg_in) {
        var t = TamSVG(svg_in);
        t.generateButtonPanel();
        if (args.play)
          tamsvg.start();
      }
    });
  }

  //  Called by jquery when the frame is loaded
  $(document).ready(function() {
    var d = dims();
    $('#tamdiv').width(d.w);
    $('#tamdiv').height(d.h);
    new TAMination(page, buildAnimation, function(tam) {
      //  Try alternatives for ms and adv pages
      //  One of these is sure to fail, but that's ok
      if (page.match(/^ms/)) {
        tam.loadXMLforAnimation(page.replace('ms', 'b1'),function(){});
        tam.loadXMLforAnimation(page.replace('ms', 'b2'),function(){});
      }
      if (page.match(/^adv/)) {
        tam.loadXMLforAnimation(page.replace('adv', 'a1'),function(){});
        tam.loadXMLforAnimation(page.replace('adv', 'a2'),function(){});
      }
    });
  });

});
