#ifndef SDPRINT_H
#define SDPRINT_H

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2004  William B. Ackerman.
//    Copyright (C) 1996 Charles Petzold.
//
//    This file is part of "Sd".
//
//    Sd is free software; you can redistribute it and/or modify it
//    under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    Sd is distributed in the hope that it will be useful, but WITHOUT
//    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
//    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
//    License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Sd; if not, write to the Free Software Foundation, Inc.,
//    59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//    This is for version 36.

// This file is used in print utilities other than Sd.

// The parameters passed here are just the initial defaults.  The user can
// change them by calling "windows_choose_font" and clicking on things.

// Q: Why do we pass "ID" values in a structure at runtime, rather than
//    using values straight out of a "resource.h" file the way the rest
//    of the civilized world does?
// A: This file, and the file sdprint.cpp, are used in different programs,
//    with different resource.h files.  We can't sensibly include resource.h
//    either here or in sdprint.cpp, because we might not get the same file
//    the client used.

struct print_default_info {
   const char *font;
   const char *filter;
   int pointsize;
   bool bold;
   bool italic;
   int IDD_PRINTING_DIALOG;
   int IDC_FILENAME;
};


struct printer_innards;

class printer {

 public:

   printer(HINSTANCE hInstance, HWND hwnd, const print_default_info & info);
   ~printer();

   void set_point_size(int size);
   void choose_font();

   // This prints a specific file.  Not sure what "szMainTitle" actually does.
   void print_this(const char *filename, char *szMainTitle, bool pagenums);

   // This brings up the "choose file" dialog and lets the user click on things.
   void print_any(char *szMainTitle, bool pagenums);

 private:

   printer_innards *innards;
};

#endif
