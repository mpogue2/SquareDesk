// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2012  William B. Ackerman.

/*
 * sdui-wincon.cpp
 * Time-stamp: <93/07/19 19:38:40 wba>
 * Copyright (c) 1990-1995 Stephen Gildea and William B. Ackerman
 *
 * Permission to use, copy, modify, and distribute this software for
 * any purpose is hereby granted without fee, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * The authors make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 * By Stephen Gildea <gildea@lcs.mit.edu> February 1993
 *
 * Uses "console application" mechanism of Visual C++.
 */

//    This file is part of "Sd".
//
//    ===================================================================
//
//    If you received this file with express permission from the licensor
//    to modify and redistribute it it under the terms of the Creative
//    Commons CC BY-NC-SA 3.0 license, then that license applies.  See
//    http://creativecommons.org/licenses/by-nc-sa/3.0/
//
//    ===================================================================
//
//    Otherwise, the GNU General Public License applies.
//
//    Sd is free software; you can redistribute it and/or modify it
//    under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or
//    (at your option) any later version.
//
//    Sd is distributed in the hope that it will be useful, but WITHOUT
//    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
//    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
//    License for more details.
//
//    You should have received a copy of the GNU General Public License,
//    in the file COPYING.txt, along with Sd.  See
//    http://www.gnu.org/licenses/
//
//    ===================================================================
//
//    This is for version 38.

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>
// This would have come in automatically if we hadn't specified WIN32_LEAN_AND_MEAN
#include <shellapi.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "sdui.h"



static HANDLE consoleStdout, consoleStdin;
// What the OS had right at the start -- we restore it when we exit from the program.
static char SavedWindowTitle[MAX_TEXT_LINE_LENGTH];
// Our string without the effect of "pick" stuff.
static char szMainTitle[MAX_TEXT_LINE_LENGTH];
static CONSOLE_SCREEN_BUFFER_INFO globalconsoleInfo;
static DWORD oldMode;
static DWORD newMode;


static const char *my_pn1;
static const char *my_pn2;
static const char *my_directions;

void iofull::final_initialize()
{
   my_pn1 = ui_options.pn1;
   my_pn2 = ui_options.pn2;

   // Install the pointy triangles.

   if (ui_options.no_graphics < 2)
      ui_options.direc = "?\020?\021????\036?\037?????";

   my_directions = ui_options.direc;

   if (!sdtty_no_console) ui_options.use_escapes_for_drawing_people = 1;
}


extern void ttu_set_window_title(const char *string)
{
   lstrcpy(szMainTitle, string);
   SetConsoleTitle(szMainTitle);
}


void iofull::set_pick_string(Cstring string)
{
   if (string && *string) SetConsoleTitle(string);
   else SetConsoleTitle(szMainTitle);    // End of pick, reset to our main title.
}


void iofull::display_help()
{
   printf("-lines <n>                  assume this many lines on the screen\n");
   printf("-no_console                 do not do any fancy screen processing -- use this\n");
   printf("                                 when input or output is redirected\n");
   printf("-no_cursor                  do not use screen management functions at all\n");
   printf("-journal <filename>         echo input commands to journal file\n");
}


BOOL WINAPI control_c_handler(DWORD ControlInfo)
{
   DWORD numRead;
   INPUT_RECORD inputRecord;

   if (ControlInfo == CTRL_C_EVENT) {
      inputRecord.EventType = KEY_EVENT;
      inputRecord.Event.KeyEvent.bKeyDown = TRUE;
      inputRecord.Event.KeyEvent.wRepeatCount = 1;
      inputRecord.Event.KeyEvent.wVirtualKeyCode = 'C';
      inputRecord.Event.KeyEvent.wVirtualScanCode = 46;
      inputRecord.Event.KeyEvent.uChar.AsciiChar = 'c';
      inputRecord.Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED;
      WriteConsoleInput(consoleStdin, &inputRecord, 1, &numRead);
      return TRUE;
   }
   else if (ControlInfo == CTRL_CLOSE_EVENT) {
      inputRecord.EventType = KEY_EVENT;
      inputRecord.Event.KeyEvent.bKeyDown = TRUE;
      inputRecord.Event.KeyEvent.wRepeatCount = 1;
      inputRecord.Event.KeyEvent.wVirtualKeyCode = VK_NONAME;
      inputRecord.Event.KeyEvent.wVirtualScanCode = 0;
      inputRecord.Event.KeyEvent.uChar.AsciiChar = 'Q';
      inputRecord.Event.KeyEvent.dwControlKeyState = 0;
      WriteConsoleInput(consoleStdin, &inputRecord, 1, &numRead);
      return TRUE;
   }
   else {
      return FALSE;
   }
}


static WORD text_color;


static const WORD bgWHT =
   BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
static const WORD bgGRY =
   BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
static const WORD fgWHT =
   FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
static const WORD fgGRY =
   FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;


static const WORD color_translate_reverse_video[8] = {
   0,
   FOREGROUND_RED  | FOREGROUND_GREEN,                         // 1 - dark yellow
   FOREGROUND_RED                     | FOREGROUND_INTENSITY,  // 2 - red
   FOREGROUND_GREEN                   | FOREGROUND_INTENSITY,  // 3 - green
   FOREGROUND_RED  | FOREGROUND_GREEN | FOREGROUND_INTENSITY,  // 4 - yellow
   FOREGROUND_BLUE                    | FOREGROUND_INTENSITY,  // 5 - blue
   FOREGROUND_RED  | FOREGROUND_BLUE  | FOREGROUND_INTENSITY,  // 6 - magenta
   FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY}; // 7 - cyan

static const WORD color_translate_reverse_video_nointen[8] = {
   0,
   FOREGROUND_RED  | FOREGROUND_GREEN,                         // 1 - dark yellow
   FOREGROUND_RED ,                                            // 2 - red
   FOREGROUND_GREEN,                                           // 3 - green
   FOREGROUND_RED  | FOREGROUND_GREEN,                         // 4 - yellow
   FOREGROUND_BLUE,                                            // 5 - blue
   FOREGROUND_RED  | FOREGROUND_BLUE,                          // 6 - magenta
   FOREGROUND_BLUE | FOREGROUND_GREEN};                        // 7 - cyan

static const WORD color_translate_normal_video[8] = {
   0,
   FOREGROUND_RED  | FOREGROUND_GREEN                        | bgWHT,  // 1 - dark yellow
   FOREGROUND_RED                     | FOREGROUND_INTENSITY | bgWHT,  // 2 - red
   FOREGROUND_GREEN                   | FOREGROUND_INTENSITY | bgWHT,  // 3 - green
   FOREGROUND_RED  | FOREGROUND_GREEN | FOREGROUND_INTENSITY | bgWHT,  // 4 - yellow
   FOREGROUND_BLUE                    | FOREGROUND_INTENSITY | bgWHT,  // 5 - blue
   FOREGROUND_RED  | FOREGROUND_BLUE  | FOREGROUND_INTENSITY | bgWHT,  // 6 - magenta
   FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY | bgWHT}; // 7 - cyan

static const WORD color_translate_normal_video_gray[8] = {
   0,
   FOREGROUND_RED  | FOREGROUND_GREEN                        | bgGRY,  // 1 - dark yellow
   FOREGROUND_RED                     | FOREGROUND_INTENSITY | bgGRY,  // 2 - red
   FOREGROUND_GREEN                   | FOREGROUND_INTENSITY | bgGRY,  // 3 - green
   FOREGROUND_RED  | FOREGROUND_GREEN | FOREGROUND_INTENSITY | bgGRY,  // 4 - yellow
   FOREGROUND_BLUE                    | FOREGROUND_INTENSITY | bgGRY,  // 5 - blue
   FOREGROUND_RED  | FOREGROUND_BLUE  | FOREGROUND_INTENSITY | bgGRY,  // 6 - magenta
   FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY | bgGRY}; // 7 - cyan

static const WORD *color_translate;

extern void ttu_initialize()
{
   DWORD numWrite;
   bool using_default_screen_size = false;

   // Set the default value if the user hasn't explicitly set something.
   if (sdtty_screen_height <= 0) {
      sdtty_screen_height = 25;
      using_default_screen_size = true;
   }

   GetConsoleTitle(SavedWindowTitle, MAX_TEXT_LINE_LENGTH-1);
   SavedWindowTitle[MAX_TEXT_LINE_LENGTH-1] = 0;
   szMainTitle[0] = 0;   // In case ttu_set_window_title never happens.

   if (sdtty_no_console) return;

   consoleStdin = GetStdHandle(STD_INPUT_HANDLE);
   consoleStdout = GetStdHandle(STD_OUTPUT_HANDLE);

   if (consoleStdout == consoleStdin) {
      int err = GetLastError();
      fprintf(stderr, "GetStdHandle failed: %d.\n", err);
      ExitProcess(err);
   }

   if (!GetConsoleMode(consoleStdin, &oldMode)) {
      int err = GetLastError();
      if (err == ERROR_INVALID_HANDLE) {
         sdtty_no_console = true;
         return;
      }
      fprintf(stderr, "GetConsoleMode failed: %d.\n", err);
      ExitProcess(err);
   }

   newMode = oldMode &
      ~ENABLE_LINE_INPUT &
      ~ENABLE_ECHO_INPUT &
      ~ENABLE_WINDOW_INPUT &
      ~ENABLE_MOUSE_INPUT;

   if (!SetConsoleMode(consoleStdin, newMode)) {
      int err = GetLastError();
      if (err == ERROR_INVALID_HANDLE) {
         sdtty_no_console = true;
         return;
      }
      fprintf(stderr, "SetConsoleMode failed: %d.\n", err);
      ExitProcess(err);
   }

   // Get information about the "buffer", that is, the "pad".  The window itself,
   // and all resizing thereof, is irrelevant.  It will take care of itself, the way
   // it does in any decent operating system (unlike some that I've had to deal with.)

   GetConsoleScreenBufferInfo(consoleStdout, &globalconsoleInfo);

   // If the user hasn't explicitly overridden the screen height, set it to
   // whatever the system tells us.
   if (using_default_screen_size)
      sdtty_screen_height = globalconsoleInfo.srWindow.Bottom-globalconsoleInfo.srWindow.Top+1;

   if (ui_options.no_intensify) {
      if (ui_options.reverse_video) {
         text_color = fgGRY;
         color_translate = color_translate_reverse_video_nointen;
      }
      else {
         text_color = bgGRY;
         color_translate = color_translate_normal_video_gray;
      }
   }
   else {
      if (ui_options.reverse_video) {
         text_color = fgWHT;
         color_translate = color_translate_reverse_video;
      }
      else {
         text_color = bgWHT;
         color_translate = color_translate_normal_video;
      }
   }

   FillConsoleOutputAttribute(consoleStdout,
                              text_color,
                              globalconsoleInfo.dwSize.X *
                              (globalconsoleInfo.dwSize.Y-
                               globalconsoleInfo.dwCursorPosition.Y)
                              -globalconsoleInfo.dwCursorPosition.X,
                              globalconsoleInfo.dwCursorPosition, &numWrite);

   SetConsoleTextAttribute(consoleStdout, text_color);

   SetConsoleCtrlHandler(&control_c_handler, TRUE);
}


void ttu_terminate()
{
   if (!sdtty_no_console) {

      CONSOLE_SCREEN_BUFFER_INFO finalconsoleInfo;

      DWORD numWrite;

      // Find out what line we have advanced to.
      GetConsoleScreenBufferInfo(consoleStdout, &finalconsoleInfo);

      // Paint the rest of the screen to the original color.
      FillConsoleOutputAttribute(consoleStdout,
                                 globalconsoleInfo.wAttributes,
                                 finalconsoleInfo.dwSize.X *
                                 (finalconsoleInfo.dwSize.Y-
                                  finalconsoleInfo.dwCursorPosition.Y)
                                 -finalconsoleInfo.dwCursorPosition.X,
                                 finalconsoleInfo.dwCursorPosition, &numWrite);

      SetConsoleTextAttribute(consoleStdout, globalconsoleInfo.wAttributes);
      SetConsoleMode(consoleStdin, oldMode);
      SetConsoleCtrlHandler(&control_c_handler, FALSE);
   }

   SetConsoleTitle(SavedWindowTitle);
}


bool iofull::help_manual()
{
   ShellExecute(NULL, "open", "c:\\sd\\sd_doc.html", NULL, NULL, SW_SHOWNORMAL);
   return true;
}


bool iofull::help_faq()
{
   ShellExecute(NULL, "open", "c:\\sd\\faq.html", NULL, NULL, SW_SHOWNORMAL);
   return TRUE;
}


extern int get_lines_for_more()
{
   return sdtty_screen_height-1;
}

extern void clear_line()
{
   if (!sdtty_no_console) {
      DWORD numWrite;
      CONSOLE_SCREEN_BUFFER_INFO myconsoleInfo;

      GetConsoleScreenBufferInfo(consoleStdout, &myconsoleInfo);
      myconsoleInfo.dwCursorPosition.X = 0;
      SetConsoleCursorPosition(consoleStdout, myconsoleInfo.dwCursorPosition);
      FillConsoleOutputCharacter(consoleStdout, ' ', myconsoleInfo.dwSize.X,
                                 myconsoleInfo.dwCursorPosition,
                                 &numWrite);
   }
   else
      printf(" XXX\n");
}

extern void rubout()
{
   if (!sdtty_no_console) {
      DWORD numWrite;
      CONSOLE_SCREEN_BUFFER_INFO myconsoleInfo;

      GetConsoleScreenBufferInfo(consoleStdout, &myconsoleInfo);
      myconsoleInfo.dwCursorPosition.X--;
      SetConsoleCursorPosition(consoleStdout, myconsoleInfo.dwCursorPosition);
      FillConsoleOutputCharacter(consoleStdout, ' ', 1,
                                 myconsoleInfo.dwCursorPosition,
                                 &numWrite);
   }
   else
      printf("\b \b");
}

extern void erase_last_n(int n)
{
   if (!sdtty_no_console) {
      DWORD numWrite;
      CONSOLE_SCREEN_BUFFER_INFO myconsoleInfo;
      int delta = n;

      GetConsoleScreenBufferInfo(consoleStdout, &myconsoleInfo);

      if (myconsoleInfo.dwCursorPosition.Y < delta)
         delta = myconsoleInfo.dwCursorPosition.Y;

      myconsoleInfo.dwCursorPosition.X = 0;
      myconsoleInfo.dwCursorPosition.Y -= delta;

      SetConsoleCursorPosition(consoleStdout, myconsoleInfo.dwCursorPosition);
      FillConsoleOutputCharacter(consoleStdout, ' ', myconsoleInfo.dwSize.X * delta,
                                 myconsoleInfo.dwCursorPosition,
                                 &numWrite);
   }
}

extern void put_line(const char the_line[])
{
   if (!sdtty_no_console) {
      char c;

      // We need to watch for escape characters denoting people
      // to be printed in a particularly pretty way.

      while ((c = *the_line++)) {
         if (c == '\013') {
            int personidx = (*the_line++) & 7;
            int persondir = (*the_line++) & 0xF;

            int randomized_person_color = personidx;

            if (ui_options.color_scheme == color_by_couple_random && ui_options.hide_glyph_numbers) {
               randomized_person_color = (color_randomizer[personidx>>1] << 1) | (personidx & 1);
            }

            put_char(' ');

            if (ui_options.color_scheme != no_color)
               SetConsoleTextAttribute(consoleStdout,
                                       color_translate[color_index_list[randomized_person_color]]);

            if (ui_options.hide_glyph_numbers)
               put_char(' ');
            else
               put_char(my_pn1[personidx]);

            put_char(my_pn2[personidx]);
            put_char(my_directions[persondir]);

            // Set back to plain "white".

            if (ui_options.color_scheme != no_color)
               SetConsoleTextAttribute(consoleStdout, text_color);
         }
         else
            put_char(c);
      }
   }
   else {
      fputs(the_line, stdout);
   }
}

extern void put_char(int c)
{
   DWORD junk;
   char cc = c;

   if (!sdtty_no_console) {
      if (c == '\n') {
         CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

         // Get information about the "buffer", that is, the "pad".  The window itself,
         // and all resizing thereof, is irrelevant.  It will take care of itself, the way
         // it does in any decent operating system (unlike some that I've had to deal with.)

         GetConsoleScreenBufferInfo(consoleStdout, &consoleInfo);
         consoleInfo.dwCursorPosition.X = 0;   // Cursor position within the pad.

         if (consoleInfo.dwSize.Y-1 != consoleInfo.dwCursorPosition.Y)
            consoleInfo.dwCursorPosition.Y++;
         else {

            // We have run off the end of the pad.

            SMALL_RECT scrollRect;    // The portion of the pad that we are
                                      // going to scroll in (all of it.)
            COORD coord;              // How we are going to move it (up one position.)
            CHAR_INFO consoleFill;    // What we are going to put into vacated area
                                      // (blanks, with same color as everything else.)

            scrollRect.Top = 0;
            scrollRect.Left = 0;
            scrollRect.Bottom = consoleInfo.dwSize.Y-1;
            scrollRect.Right = consoleInfo.dwSize.X-1;
            coord.X = 0;
            coord.Y = -1;
            consoleFill.Attributes = consoleInfo.wAttributes;
            consoleFill.Char.AsciiChar = ' ';
            ScrollConsoleScreenBuffer(consoleStdout, &scrollRect, 0, coord, &consoleFill);
         }

         SetConsoleCursorPosition(consoleStdout, consoleInfo.dwCursorPosition);
      }
      else
         WriteFile(consoleStdout, &cc, 1, &junk, (LPOVERLAPPED) 0);
   }
   else
      putchar(c);
}


extern int get_char()
{
   DWORD numRead;
   INPUT_RECORD inputRecord;

   if (sdtty_no_console)
      return getchar();    // A "stdio" call.

   for ( ;; ) {
      ReadConsoleInput(consoleStdin, &inputRecord, 1, &numRead);

      if (inputRecord.EventType == KEY_EVENT && inputRecord.Event.KeyEvent.bKeyDown) {
         DWORD ctlbits = inputRecord.Event.KeyEvent.dwControlKeyState & (~NUMLOCK_ON | SCROLLLOCK_ON);

         unsigned int c = inputRecord.Event.KeyEvent.uChar.AsciiChar;
         int key = inputRecord.Event.KeyEvent.wVirtualKeyCode;
         int npdigit;

         // Canonicalize the control bits -- we don't distinguish
         //    between left ALT and right ALT, for example.

         if (ctlbits & CAPSLOCK_ON) ctlbits |= SHIFT_PRESSED;
         if (ctlbits & RIGHT_CTRL_PRESSED) ctlbits |= LEFT_CTRL_PRESSED;
         if (ctlbits & RIGHT_ALT_PRESSED) ctlbits |= LEFT_ALT_PRESSED;
         ctlbits &= ~(CAPSLOCK_ON | RIGHT_CTRL_PRESSED | RIGHT_ALT_PRESSED);

         if (key >= 'A' && key <= 'Z') {    // Letter key was pressed.
            ctlbits &= ~SHIFT_PRESSED;
            if (ctlbits == 0) return c;
            else if (ctlbits == LEFT_CTRL_PRESSED)
               return CTLLET + key;
            else if (ctlbits == LEFT_ALT_PRESSED)
               return ALTLET + key;
            else if (ctlbits == (LEFT_ALT_PRESSED|LEFT_CTRL_PRESSED))
               return CTLALTLET + key;
            else
               continue;
         }
         else if (key >= '0' && key <= '9') {       // Digit key.
            ctlbits &= ~SHIFT_PRESSED;
            if (ctlbits == 0) return c;
            else if (ctlbits == LEFT_CTRL_PRESSED)
               return key-'0'+CTLDIG;
            else if (ctlbits == LEFT_ALT_PRESSED)
               return key-'0'+ALTDIG;
            else if (ctlbits == (LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED))
               return key-'0'+CTLALTDIG;
            else
               continue;   // Don't know what it is.
         }
         else if (key == VK_SHIFT || key == VK_CONTROL || key == VK_MENU || key == VK_CAPITAL)
            continue;      // Just an actual shift/control/alt key ("MENU" means ALT.)
         else if (key >= VK_F1 && key <= VK_F12) {    // Function key.
            if (ctlbits == 0)                      return key-VK_F1+FKEY+1;
            else if (ctlbits == SHIFT_PRESSED)     return key-VK_F1+SFKEY+1;
            else if (ctlbits == LEFT_CTRL_PRESSED) return key-VK_F1+CFKEY+1;
            else if (ctlbits == LEFT_ALT_PRESSED)  return key-VK_F1+AFKEY+1;
            else if (ctlbits == (LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED)) return key-0x6F+CAFKEY;
            else
               continue;   // Don't know what it is.
         }
         else if ((ctlbits & ENHANCED_KEY) && key >= 0x20 && key <= 0x20+15) {
            // "Enhanced" key.
            if (ctlbits == ENHANCED_KEY)                          return key-0x20+EKEY;
            else if (ctlbits == (ENHANCED_KEY|SHIFT_PRESSED))     return key-0x20+SEKEY;
            else if (ctlbits == (ENHANCED_KEY|LEFT_CTRL_PRESSED)) return key-0x20+CEKEY;
            else if (ctlbits == (ENHANCED_KEY|LEFT_ALT_PRESSED))  return key-0x20+AEKEY;
            else if (ctlbits == (ENHANCED_KEY|LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED)) return key-0x20+CAEKEY;
            else
               continue;   // Don't know what it is.
         }
         else if (key == VK_NONAME && c == 'Q')  // User clicked the "X" in the upper-right corner.
            return CLOSEPROGRAMKEY;
         else if ((key >= 0xBA && key <= 0xBF) ||     // Random other keys ...
                  (key == VK_SPACE) ||
                  (key == VK_TAB) ||
                  (key == VK_BACK) ||
                  (key == VK_RETURN) ||
                  (key == VK_ESCAPE) ||
                  (key == 0xE2) ||                    // Underscore on Japanese keyboards.
                  (key == 0xC0) ||                    // Random other keys ...
                  (key >= 0xDB && key <= 0xDE)) {     // Random other keys ...
            ctlbits &= ~SHIFT_PRESSED;
            if (ctlbits == 0) return c;
            else continue;
         }
         else if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) {    // Numeric keypad with NUMLOCK on.
            npdigit = key - VK_NUMPAD0;
         }
         else if (key == 0x2D) npdigit = 0;    // Numeric keypad with NUMLOCK off.
         else if (key == 0x23) npdigit = 1;
         else if (key == 0x28) npdigit = 2;
         else if (key == 0x22) npdigit = 3;
         else if (key == 0x25) npdigit = 4;
         else if (key == 0x0C) npdigit = 5;
         else if (key == 0x27) npdigit = 6;
         else if (key == 0x24) npdigit = 7;
         else if (key == 0x26) npdigit = 8;
         else if (key == 0x21) npdigit = 9;
         else if (ctlbits == 0 && isprint(c))
            return c;
         else
            continue;

         // If we get here, this is a numeric keypad press.

         ctlbits &= ~SHIFT_PRESSED;
         if (ctlbits == 0) return npdigit+'0';   // Just return the plain digit.
         else if (ctlbits == LEFT_CTRL_PRESSED)
            return npdigit+CTLNKP;
         else if (ctlbits == LEFT_ALT_PRESSED)
            return npdigit+ALTNKP;
         else if (ctlbits == (LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED))
            return npdigit+CTLALTNKP;
         else
            continue;   // Don't know what it is.
      }
   }
}

extern void get_string(char *dest, int max)
{
   int size = 0;

   for ( ;; ) {
      int key = get_char();

      if (key == (CTLLET + 'U')) {
         size = 0;
         clear_line();
         continue;
      }
      else if (key > 0x7F) continue;    // Ignore special function keys and such.
      else if (key == '\r' || key == '\n') break;
      else if ((key == '\b') || (key == 0x7F)) {
         if (size > 0) {
            size--;
            rubout();
            continue;
         }

         // Backspace at beginning of line; may have special meaning.
         dest[size++] = key;
         dest[size] = '\0';
         return;;
      }

      if (key > 128) continue;   // This was a function key press.

      put_char(key);
      if (size < max-1)
         dest[size++] = key;
   }

   dest[size] = '\0';
   put_line("\n");
}

extern void ttu_bell()
{
   MessageBeep(MB_ICONQUESTION);
}
