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

define(['calls/call','calls/codedcall','callcontext','callerror'],
        function(Call,CodedCall,CallContext,CallError) {

  var TamSequence = function() {
    this.startingFormation="Static Square";
    this.seq = 0;
    this.prevhtml = '';
    this.editor = null;
    this.callnames = [];
    this.prevtext = '';
    this.filecount = 0;
    this.calls = [];
    this.failedTests = "";
    var me = this;
    this.tam = new TAMination('', function(t) {
      t.setFormation(me.startingFormation);
      me.startAnimations();
      me.editorSetup();
      if (document.URL.match(/\?test/)) {
        me.runAllTests();
      }
    });
  };

  TamSequence.compattern = /[*#].*/;

  //  Start the asynchronous method that runs tests
  TamSequence.prototype.runAllTests = function() {
    this.more = true;
    this.testnum = 1;
    this.runOneTest();
  }

  //  Run one test, and when it is done start up the next test
  TamSequence.prototype.runOneTest = function() {
    var f = "sequences/test" +
            (this.testnum > 99 ? "" : "0") +
            (this.testnum > 9 ? "" : "0") + this.testnum + ".txt";
    var me = this;
    $.ajax({
      url: f,
      dataType: "text",
      //  If error reading sequence file
      //  then we have reached the end of the tests
      error: function() {
        if (me.failedTests.length > 0)
          alert("These tests failed: "+me.failedTests);
        else
          alert("All tests successful");
        more = false;
      },
      success: function(data) {
        me.hasError = false;
        try {
          //  Load the sequence
          me.calls = data.split("\n");
          //  Run the sequence
          me.updateSequence();
        } catch (err) {
          me.failedTests += " " + me.testnum;
        }
        //  On completion record the result
        //  and go to the next test
        tamsvg.animationStopped = function() {
          if (me.hasError)
            me.failedTests += me.testnum + " ";
          if (me.more) {
            me.testnum += 1;
            me.runOneTest();
          }
        };
      }
    });
  }

  TamSequence.prototype.editorSetup = function()
  {
    var me = this;
    tamsvg.setPart = function(n) { me.setCurrentCall(n); };
    this.updateSequence();
    $('#call').change(function() {
      me.calls.push($('#call').val());
      me.updateSequence();
      $('#call').val("");
    })
    $('#calls').on('click','.deleteCall',function() {
      var callnum = Number($(this).parent().attr('id').replace(/Part/,''))-1;
      me.calls.splice(callnum,1);
      me.updateSequence();
      return false;
    });
    $('#calls').on('click','div',function() {
      var callnum = Number($(this).attr('id').replace(/Part/,''))-1;
      tamsvg.setBeat(0);
      var n = callnum;
      while (n > 0) {
        tamsvg.next();
        n -= 1;
      }
      tamsvg.setBeat(tamsvg.beat + 0.01);
      me.setCurrentCall(callnum+1);
    });

    this.startingFormation = $('input[name="formation"]:checked').val();
    this.tam.setFormation(me.startingFormation);
    $('#instructions-link').click(function() {
      $('#instructions').toggle();
    });
    $('#instructions').click(function() {
      $('#instructions').hide();
    });
    $('input[name="formation"]').change(function(ev) {
      me.startingFormation = $(ev.target).val();
      this.tam = new TAMination('',function(t) {
        t.setFormation(me.startingFormation);
        me.startAnimations();
      });
    });
    $('#clearbutton').click(function() {
      me.calls = [];
      $('#call').val("");
      me.updateSequence();
      tamsvg.rewind();
    });
    $(document).mouseup(function() { me.focusHiddenArea() });
    $('#hidden').keydown(function(e) {
      var keynum = e.keyCode;
      var calltext = $('#call').val();
      if (keynum == 13) {  //  return: process call
        me.calls.push(calltext);
        me.updateSequence();
        $('#call').val("");
        e.preventDefault();
      } else if (keynum == 8) {  // backspace
        $('#call').val(calltext.substr(0,calltext.length-1));
        e.preventDefault();
      } else if ((keynum == 32 || (keynum >= 48 && keynum <= 90)
                 || keynum == 191  // slash
                 || keynum == 173)  // dash
                 && !e.altKey && !e.ctrlKey && !e.metaKey) {
        // character
        var char = String.fromCharCode(e.keyCode);
        if (keynum == 191) char = "/";
        if (keynum == 173) char = "-";
        if (!e.shiftKey)
          char = char.toLowerCase()
        $('#call').val(calltext + char);
        e.preventDefault();
      }
      //$('#hidden').val(e.type + ": " + keynum.toString());
      me.focusHiddenArea();
    });
    document.addEventListener('paste',function(e) {
      window.setTimeout(function() {
        me.calls = $('#hidden').val().split("\n");
        me.updateSequence();
      },0);
    });
  };

  TamSequence.prototype.focusHiddenArea = function() {
    var hiddenInput = $('#hidden');
    hiddenInput.val(this.callnames.join("\n"));
    hiddenInput.focus().select();
  };

  TamSequence.prototype.startAnimations = function() {
    if ($('#definition #sequencepage').size() == 0)
      $('#definition').empty().append($('#sequencepage'));
    //  Build the animation
    var dims = svgSize();
    var svgdim = dims.width;
    var svgstr='<div id="svgdiv" '+
               'style="width:'+svgdim+'px; height:'+svgdim+'px;"></div>';
    $("#svgcontainer").empty().width(dims.width).append(svgstr);
    var me = this;
    $('#svgdiv').svg({onLoad:function(x) {
        var t = new TamSVG(x);
        t.setPart = function(n) { me.setCurrentCall(n); };
        //  Add all the calls to the animation
        me.updateSequence();
        t.generateButtonPanel();
      }
    });
  };

  TamSequence.prototype.setCurrentCall = function(n) {
    $('#calls div').removeClass('callhighlight')
       .filter('#Part'+n).addClass('callhighlight');
    tamsvg.setTitle(n > 0 ? this.callnames[n-1] : '');
  };

  //  Highlight a line that has an error
  TamSequence.prototype.showError = function(n) {
    //  If it's the last line, then just erase it
    if (n == this.calls.length) {
      this.calls.pop();
      this.updateSequence();
    }
    else
      $('#calls').find('#Part'+n).addClass('callerror');
  };

  //  This function is called every time the text is changed by the user
  TamSequence.prototype.processCallText = function() {
    //  html is the marked-up calls to stuff back into the text box
    //  so calls are highlighted, errors marked, comments colored, etc.
    var html = [];
    var callnum = 1;
    //  Clear any previous error message
    $('#errortext').html('');
    var lines = this.calls;
    lines.forEach(function(line) {
      var calltext = line;
      var comchar = line.search(/[*#]/);
      if (comchar >= 0) {
        //  Highlight comments
        line = line.substr(0,comchar) + '<span class="commenttext">' +
               line.substr(comchar) + '</span>';
        //  and remove them from the text of calls returned
        calltext = line.substr(0,comchar);
      }
      //  Remove any remaining html tags in preparation for parsing calls
      calltext = $.trim(calltext.replace(/<.*?>/g,'').replace(/\&nbsp;/g,' '));
      //  If we have something left to parse as a call
      if (calltext.search(/\w/) >= 0) {
        //  .. add class to highlight it when animated
        line = '<div id="Part'+callnum+'">' + line + '</span>';
        callnum += 1;
      }
      html.push(line);
    });
    //  Replace the text with our marked-up version
    $('#calls').html(html.join('<br/>'));
  };

  TamSequence.prototype.fetchCall = function(callname) {
    //  Load any animations for this call
    var me = this;
    TAMination.searchCalls(callname,{exact:true}).forEach(function(d) {
      var f = d.link;
      if (!Call.xmldata[f]) {
        //  Call is interpreted by animations
        me.filecount++;
        //  Read and store the animation
        $.get(f.extension('xml'),function(data,status,jqxhr) {
          Call.xmldata[f] = data;
          if (--me.filecount == 0) {
            //  All xml has been read, now we can interpret the calls
            me.buildSequence();
          }
        },"xml").filename = f;
      }
    });

    //  Also load any scripts that perform this call
    me.filecount += CodedCall.getScript(callname, function(c) {
      //  Read and interpret the script
      //  Load any other calls that this script uses
      if (c.requires)
        c.requires.forEach(function(d2) {
          me.fetchCall(d2);
        });
      if (c.requiresxml)
        c.requiresxml.forEach(function(x) {
          me.fetchCall(x);
        });
      if (--me.filecount == 0)
        me.buildSequence();
    });
  };



  TamSequence.prototype.updateSequence = function() {
    //this.processCallText();
    //  Make sure all calls are sent to be fetched
    this.filecount = 100;
    //  Look up the calls fetch the necessary files
    var me = this;
    for (var i in this.calls) {
      //  Need to load xml files, 1 or more for each call
      var callline = this.calls[i].toLowerCase()
                             .replace(/\s/g,' ')  // coalesce spaces
                             .replace(TamSequence.compattern,'');     // remove comments
      //  Fetch calls that are any part of the callname,
      //  to get concepts and modifications
      callline.minced().forEach(function(s) { me.fetchCall(s) });
    }
    //  All calls sent to be fetched, we can remove the safety
    this.filecount -= 100;
    if (!this.filecount)
      //  We already have all the files
      this.buildSequence();
  };

  TamSequence.prototype.callHTML = function(call,n) {
    return '<div id="Part'+n+'">' +
           '<span class="deleteCall">X</span>' + n + " " + call + '</div>';
  }

  TamSequence.prototype.buildSequence = function() {
    //  First clear out the previous animation
    tamsvg.dancers.forEach(function(d) {
      d.path.clear();
      d.animate(0);
    });
    tamsvg.parts = [];
    this.callnames = [];
    //  Clear any previous error message
    $('#errortext').html('Use keyboard to copy and paste.');
    var n2 = 0;
    var callname = '';
    $('#calls').empty();
    for (var n in this.calls) {
      $('#calls').append(this.callHTML(this.calls[n],Number(n)+1));
    }
    try {
      for (n2 in this.calls) {
        callname = this.calls[n2];
        //  Break up the call as above to find and perform modifications
        var callline = this.calls[n2].toLowerCase()
                                .replace(/\s+/g,' ')  // coalesce spaces
                                .replace(TamSequence.compattern,'');     // remove comments
        //  Various user errors in applying calls are detected and thrown here
        //  and also by lower-level code
        var ctx = new CallContext(tamsvg);
        ctx.interpretCall(callline);
        ctx.performCall();
        //  If no error thrown, then we found the call
        //  and created the animation successfully
        //  Copy the call from the working context to each dancer
        tamsvg.dancers.forEach(function(d,i) {
          d.path.add(ctx.dancers[i].path);
          d.animateToEnd();
        });
        //  Each call shown as one "part" on the slider
        tamsvg.parts.push(ctx.dancers[0].path.beats());
        this.callnames.push(ctx.callname);
        var partnum = Number(n2)+1
        $('#Part'+partnum).replaceWith('<div id="Part'+partnum+'"><span class="deleteCall">X</span>' +
            partnum + " " + ctx.callname +
            '</div>');
      } //  repeat for every call

    }  // end of try block

    catch (err) {
      if (err instanceof CallError) {
        this.showError(Number(n2)+1);
        this.hasError = true;
        var msg = err.message.replace(/%s/,'<span class="calltext">'+callname+'</span>')+'<br/>';
        $('#errortext').html(msg);
      }
      else {
        this.showError(Number(n2)+1);
        $('#errortext').html("Internal Sequencer Error<br/>" + err.toString());
      }
    }

    //  All calls parsed and created
    tamsvg.parts.pop();  // last part is implied
    var lastcallstart = tamsvg.beats - 2;
    tamsvg.beats = tamsvg.dancers[0].beats() + 2;
    if (!tamsvg.running && this.calls.length > 0) {
      tamsvg.beat = lastcallstart;
      tamsvg.start();
    }
    tamsvg.updateSliderMarks(true);
  };

  return TamSequence;
});
