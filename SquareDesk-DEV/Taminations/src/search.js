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

//  This is the code to handle searching calls
var prevsearch = '';
$(document).ready(function() {
  window.setInterval(function() {
    //  Monitor the text input every 1/10 second
    //  Will do work only if there's a new search
    var text = $('#searchbox.active').text();
    if (text != prevsearch && text.match(/\w/)) {
      dosearch(text);
      prevsearch = text;
    }
  },100);
});

//  Update the table of search results
function dosearch(text) {
  //  Create result box if needed
  if ($('#searchresults').length == 0) {
    $('#definition').append('<div id="searchresultsdiv"><div><div>X</div></div>'+
        '<table><tbody id="searchresults"></tbody></table></div>')
    $('#searchresultsdiv').width($('#definition').width()*0.9);
    var h = $('#definition').height();
    if (h == 0)
      h = $(window).height() - $('#menudiv table').height() - $('.title').height();
    $('#searchresultsdiv').height(h*0.9);
    var p = $('#definition').position();
    $('#searchresultsdiv').offset({top:p.top+10, left:p.left+10});
  }
  //  Clear out previous results
  $('#searchresults').empty();
  $('#searchresultsdiv').show('slow');
  $('#searchresultsdiv div div').click(function() {
    $('#searchresultsdiv').hide('slow');
  });

  TAMination.searchCalls(text).forEach(function(d) {
    //  Append one row to the search results table for each match
    //  Include the level of each call as second item on each row
    $('#searchresults').append('<tr><td><a href="../' + d.link +
               '.html?' + d.title.replace(/\W|\s/g,'') + '">' + d.title +
               '</a></td><td style="padding-left:20px">' + d.sublevel +
               '</td></tr>');
  });
}
