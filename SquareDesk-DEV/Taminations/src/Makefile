
#    Copyright 2015 Brad Christie
#
#    This file is part of Taminations.
#
#    Taminations is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Affero General Public License as published
#    by the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    Taminations is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Affero General Public License for more details.
#
#    You should have received a copy of the GNU Affero General Public License
#    along with Taminations.  If not, see <http://www.gnu.org/licenses/>.

#  This generates files needed for Taminations
#  You need GNU make to run this file
#  and Python to generate files
#  On Windows use Cygwin

RM ?= rm
COPY ?= cp
PYTHON ?= python
ZIP ?= zip
UNZIP ?= unzip
ZIPFILE = ../tamination.zip
DIRS = b1 b2 ms plus a1 a2 c1 c2 c3a c3b info .
XMLSRC = calls.xml callindex.xml formations.xml moves.xml calls.dtd tamination.dtd
XMLCOPIES = $(foreach dir,$(DIRS),$(addprefix ../$(dir)/,$(XMLSRC)))
CALLINDEX = callindex.xml

.phony : all
all : $(CALLINDEX) $(XMLCOPIES) $(ZIPFILE)
	@echo Build complete

unzip :
	cd ../ext && $(UNZIP) jquery.mobile-1.4.2.zip -d jquery/jquery-mobile
	cd ../ext && $(UNZIP) jquery-mousewheel-3.1.11.zip -d jquery
	cd ../ext && $(UNZIP) jquery.svg.package-1.4.5.zip -d jquery/jquery-svg
	cd ../ext && $(UNZIP) jquery-ui-1.11.0.custom.zip -d jquery
	cd ../ext && $(UNZIP) tinymce_4.1.0_jquery.zip

#  Clean target to delete jar and zip
.phony : clean
clean :
	-$(RM) *.class *.jar
	-$(RM) $(ZIPFILE)
	-$(RM) $(XMLCOPIES)
	-$(RM) $(CALLINDEX)

#  Make the zip file.  Exclude my Eclipse project files which are not of interest to others.
#  The wildcard * pattern apparently conveniently excludes .* files
../tamination.zip : $(filter-out $(ZIPFILE),$(wildcard ../* ../*/*))
	-$(RM) $(ZIPFILE)
	cd .. && $(ZIP) -rq tamination.zip info $(DIRS) src ext *.html *.txt *.ico *.xml *.dtd

#  Generate index used by sequencer
$(CALLINDEX) : $(filter-out $(XMLCOPIES),$(wildcard ../*/*.xml) $(wildcard ..*/*/*.*.html) $(wildcard calls/*.js))
	$(PYTHON) indexcalls.py >$@

#  Copy XML files that define menus, moves, and formations to each directory
#  Browsers can be picky about loading AJAX from different directories
../b1/% : %
	cp $< $@
../b2/% : %
	cp $< $@
../ms/% : %
	cp $< $@
../plus/% : %
	cp $< $@
../a1/% : %
	cp $< $@
../a2/% : %
	cp $< $@
../c1/% : %
	cp $< $@
../c2/% : %
	cp $< $@
../c3a/% : %
	cp $< $@
../c3b/% : %
	cp $< $@
../info/% : %
	cp $< $@
../% : %
	cp $< $@
	