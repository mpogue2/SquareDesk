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

if (navigator.userAgent.indexOf('MSIE 8') > 0) {
  var noie8 =  '<h3>Taminations does not work on Internet Explorer 8.<br/>'+
  'Download and install one of these other excellent browsers.</h3>'+
  '<ul><li><a href="https://www.google.com/intl/en/chrome/browser/">Chrome</a></li>'+
  '<li><a href="http://www.mozilla.org/firefox/new/">Firefox</a></li>'+
  '<li><a href="http://www.opera.com/">Opera</a></li></ul></div>';
  document.getElementById('definition').innerHTML = noie8;
} else {

  requirejs.config({
    paths : {
      cookie: '../ext/cookie',
      jquery: '../ext/jquery/jquery-2.1.1.min',
      jquerysvg: '../ext/jquery/jquery-svg/jquery.svg.min',
      jqueryui: '../ext/jquery/jquery-ui-1.11.0.custom/jquery-ui.min',
      jquerymousewheel: '../ext/jquery/jquery-mousewheel-3.1.11/jquery.mousewheel.min',
      jquerymobile: '../ext/jquery/jQueryMobile-1.4.5/jquery.mobile-1.4.5.min',
      jquerymobilepagedata: '../ext/jquery/jqm.page.params',
    },
    shim: {
      cookie : {
        exports: 'Cookie'
      },
      numeric : {
        exports: 'numeric'
      },
      jquerysvg : {
        deps: ['jquery']
      },
      tamination : {
        deps: ['jquery','env','string','math','array'],
        exports: 'TAMination'
      },
      tamsvg : {
        deps: ['tamination','jquerysvg','jquerymousewheel','cookie','handhold',
               'color','affinetransform','vector','bezier','movement','path','dancer']
      },
      tampage : {
        deps: ['tamination','tamsvg','cookie','jqueryui'],
      },
      jquerymobilepagedata : {
        deps : ['jquerymobile']
      },
      mobile : {
        deps: ['cookie','jquery','jquerymobile','jquerymobilepagedata','tamsvg']
      },
      tamsequence : {
        deps: ['tampage','env']
      },
      call : {
        deps : ['tamination','env']
      },
      search : {
        deps : ['tamination','tampage']
      }
    }
  });

  if (document.URL.search('mobile.html') >= 0)
    require(['mobile'],function() { sizeFirstMobilePage(); });
  else if (document.URL.search('sequence.html') >= 0)
    require(['tamsequence'],function(TamSequence) { new TamSequence(); });
  else if (document.URL.search('embed.html') >= 0)
    require(['embed'],function() { });
  else if (document.URL.search('movements.html') >= 0)
    require(['tampage'],movementsPageInit);
  else if (document.URL.search('search.html') >= 0)
    require(['search'],function() { });
  else if (document.URL.search('howtouse.html') >= 0)
    require(['tampage'],function() { setupHighlights(); });
  else
    require(['search'],function() { });
}
