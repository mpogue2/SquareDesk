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

$(document).ready(function() {
  //  Make sure this is run *after* the document.ready function
  //  in tampage.js.  This is a bit of a hack.
  window.setTimeout(bezier_setup,10);
  //  Fill the list of movements available for loading
  $('path',movedata).each(function() {
    if ($('movement',this).length == 1) {
      var name = $(this).attr('name');
      $('#movelist').append('<option value="'+name+'">'+name+'</option>');
    }
  });
});

function bezier_setup()
{
  tamsvg.dancers[1].hide();
  $('#taminatorsays').empty();
  $(".selectRadio").hide();
  $("#controls").detach().appendTo("#animationlist");
  $("#load").click(load_movement);
  $(".BezierSpinner").spinner({step:0.1,numberFormat:"n", stop: read_bezier });
  $(".BeatsSpinner").spinner({step:0.1,numberFormat:"n", min:0.1, stop: read_bezier });
  $("#checkRotation").change(function() {
    if ($('#checkRotation:checked').length > 0)
      $("#cx3,#cx4,#cy4,#x4,#y4").spinner("disable");
    else
      $("#cx3,#cx4,#cy4,#x4,#y4").spinner("enable");
    read_bezier();
  });
  $("#hands").change(read_bezier);
  read_bezier();
}

function read_bezier()
{
  var movetext = '<movement '+
      'beats="'+$("#beats").spinner("value")+'" '+
      'hands="'+$("#hands").val()+'" '+
      'cx1="'+$("#cx1").spinner("value")+'" '+
      'cy1="'+$("#cy1").spinner("value")+'" '+
      'cx2="'+$("#cx2").spinner("value")+'" '+
      'cy2="'+$("#cy2").spinner("value")+'" '+
      'x2="'+$("#x2").spinner("value")+'" '+
      'y2="'+$("#y2").spinner("value")+'" ';
  if ($('#checkRotation:checked').length == 0)
    movetext +=
      'cx3="'+$("#cx3").spinner("value")+'" '+
      'cx4="'+$("#cx4").spinner("value")+'" '+
      'cy4="'+$("#cy4").spinner("value")+'" '+
      'x4="'+$("#x4").spinner("value")+'" '+
      'y4="'+$("#y4").spinner("value")+'" ';
  movetext += '/>';
  $("#movement").text(movetext);
  $("path",animations).empty().append(movetext);
  PickAnimation(0);
  tamsvg.dancers[1].hide();
  $('#taminatorsays').empty();
}

function load_movement()
{
  var t = $("#movelist option:selected").text();
  var m = $('path[name="'+t+'"] > movement',movedata);
  $('#beats').spinner('value',m.attr('beats'));
  $('#hands').val(m.attr('hands'));
  $('#cx1').spinner('value',m.attr('cx1'));
  $('#cy1').spinner('value',m.attr('cy1'));
  $('#cx2').spinner('value',m.attr('cx2'));
  $('#cy2').spinner('value',m.attr('cy2'));
  $('#x2').spinner('value',m.attr('x2'));
  $('#y2').spinner('value',m.attr('y2'));
  if (m.attr('cx3') == undefined) {
    $("#checkRotation").prop('checked',true);
    $("#cx3,#cx4,#cy4,#x4,#y4").spinner("disable");  }
  else {
    $("#checkRotation").prop('checked',false);
    $("#cx3,#cx4,#cy4,#x4,#y4").spinner("enable");
    $('#cx3').spinner('value',m.attr('cx3'));
    $('#cx4').spinner('value',m.attr('cx4'));
    $('#cy4').spinner('value',m.attr('cy4'));
    $('#x4').spinner('value',m.attr('x4'));
    $('#y4').spinner('value',m.attr('y4'));
  }
  read_bezier();
}
