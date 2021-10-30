// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2021  William B. Ackerman.
//    Copyright (C) 1995  Robert E. Cays
//    Copyright (C) 1996  Charles Petzold
//
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

//    sdui-win.cpp - SD -- Microsoft Windows User Interface

#define UI_VERSION_STRING "4.10"

// This file defines all the functions in class iofull.

// Do this to get WM_MOUSEWHEEL defined in windows header files.
#define _WIN32_WINNT 0x0400
#define _CRT_SECURE_NO_WARNINGS

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>
// This would have come in automatically if we hadn't specified WIN32_LEAN_AND_MEAN
#include <shellapi.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "sd.h"

static int window_size_args[4] = {780, 560, 10, 20};

#include "sdprint.h"
#include "resource.h"


#if defined(_MSC_VER)
#pragma comment(lib, "comctl32")
#endif

/* Privately defined message codes. */
/* The user-definable message codes begin at 1024, according to winuser.h.
   However, our resources use up numbers in that range (and, in fact,
   start at 1003, but that's Developer Studio's business).  So we start at 2000. */
#define CM_REINIT      2000
/* The special key codes start at 128 (due to the way Sdtty
   handles them), so we subtract 128 as we embed them into the
   Windows command user segment. */
#define SPECIAL_KEY_OFFSET (CM_REINIT-128+1)

static char szMainWindowName[] = "Sd main window class";
static char szTranscriptWindowName[] = "Sd transcript window class";

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TranscriptAreaWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TextInputAreaWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CallMenuWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK AcceptButtonWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CancelButtonWndProc(HWND, UINT, WPARAM, LPARAM);

static WNDPROC OldTextInputAreaWndProc;
static WNDPROC OldCallMenuWndProc;
static WNDPROC OldAcceptButtonWndProc;
static WNDPROC OldCancelButtonWndProc;



// Under Sdtty, if you type something ambiguous and press ENTER, it
// isn't accepted.  We want to be keystroke-compatible with Sdtty.
// BUT: If the user has used any up-down arrows to move around in the
// menu, and then presses ENTER, we accept the highlighted item, even
// if it would have been ambiguous to the matcher.  This is obviously
// what the user wants, and, by using the arrow keys, the user has
// gone outside of the Sdtty behavior.  This variable keeps track of
// whether the menu changed.

static bool menu_moved;


#define TEXT_INPUT_AREA_INDEX 99
#define CALL_MENU_INDEX 98
#define PROGRESS_INDEX 96
#define TRANSCRIPT_AREA_INDEX 95
#define ACCEPT_BUTTON_INDEX 94
#define CANCEL_BUTTON_INDEX 93
// Concocted index for user hitting <enter>.
#define ENTER_INDEX 92
#define ESCAPE_INDEX 91
#define STATUSBAR_INDEX 90


#define DISPLAY_LINE_LENGTH 90

struct DisplayType {
   char Line [DISPLAY_LINE_LENGTH];
   int in_picture;
   int Height;
   int DeltaToNext;
   DisplayType *Next;
   DisplayType *Prev;
};


#define ui_undefined -999


static char szOutFilename     [MAX_TEXT_LINE_LENGTH];
static char szDatabaseFilename[MAX_TEXT_LINE_LENGTH];
static char szResolveWndTitle [MAX_TEXT_LINE_LENGTH];
static int GLOBStatusBarLength;
static Cstring szGLOBFirstPane;
static HPALETTE hPalette;   // The palette that the system makes for us.
static LPBITMAPINFO lpBi;   // Address of the DIB (bitmap file) mapped in memory.
static LPTSTR lpBits;       // Address of the pixel data in same.


static HINSTANCE GLOBhInstance;
static int GLOBiCmdShow;
static printer *GLOBprinter;

static HWND hwndMain;
static HWND hwndAcceptButton;
static HWND hwndCancelButton;
static HWND hwndTextInputArea;
static HWND hwndCallMenu;
static HWND hwndProgress;
static HWND hwndTranscriptArea;
static HWND hwndStatusBar;

/* If not in a popup, the focus table has
   hwndTextInputArea, hwndAcceptButton, and hwndTranscriptArea.
   If in a popup, it has
   hwndTextInputArea, hwndAcceptButton, hwndCancelButton, and hwndTranscriptArea.
*/

static HWND ButtonFocusTable[4];


static bool InPopup = false;
static int ButtonFocusIndex = 0;  // What thing (from ButtonFocusTable) has the focus.
static int ButtonFocusHigh = 2;   // 3 if in popup, else 2
static bool WaitingForCommand;
static int nLastOne;
static int nTotalImageHeight;   // Total height of the stuff that we would
                                // like to show in the transcript window.
static int nImageOffTop = 0;       // Amount of that stuff that is scrolled off the top.
static int nActiveTranscriptSize = 500;   // Amount that we tell the scroll
                                          // that we believe the screen holds
static int pagesize;           // Amount we tell it to scroll if user clicks in dead scroll area
static int BottomFudge;
static uims_reply_kind MenuKind;
static DisplayType *DisplayRoot = NULL;
static DisplayType *CurDisplay = NULL;

// These control how big we think the menu windows
// are and where we center the selected item.
#define PAGE_LEN 31
#define LISTBOX_SCROLL_POINT 15


static int TranscriptTextWidth;
static int TranscriptTextHeight;
static int AnsiTextWidth;
static int AnsiTextHeight;
static int SystemTextWidth;
static int SystemTextHeight;
static int ButtonTopYCoord;
static int TranscriptEdge;





static RECT CallsClientRect;
static RECT TranscriptClientRect;

// This is the last title sent by the main program.  We add stuff to it.
static char szMainTitle[MAX_TEXT_LINE_LENGTH];



static void uims_bell()
{
   if (!ui_options.no_sound) MessageBeep(MB_ICONEXCLAMATION);
}


static void UpdateStatusBar(Cstring szFirstPane)
{
   int StatusBarDimensions[7];

   if (szFirstPane)
      szGLOBFirstPane = szFirstPane;

   StatusBarDimensions[0] = (50*GLOBStatusBarLength)>>7;
   StatusBarDimensions[1] = (63*GLOBStatusBarLength)>>7;
   StatusBarDimensions[2] = (76*GLOBStatusBarLength)>>7;
   StatusBarDimensions[3] = (89*GLOBStatusBarLength)>>7;
   StatusBarDimensions[4] = (102*GLOBStatusBarLength)>>7;
   StatusBarDimensions[5] = (115*GLOBStatusBarLength)>>7;
   StatusBarDimensions[6] = -1;

   if (allowing_modifications || allowing_all_concepts ||
       using_active_phantoms || allowing_minigrand ||
       ui_options.singing_call_mode || ui_options.nowarn_mode) {
      SendMessage(hwndStatusBar, SB_SETPARTS, 7, (LPARAM) StatusBarDimensions);

      SendMessage(hwndStatusBar, SB_SETTEXT, 1,
                  (LPARAM) ((allowing_modifications == 2) ? "all mods" :
                            (allowing_modifications ? "simple mods" : "")));

      SendMessage(hwndStatusBar, SB_SETTEXT, 2,
                  (LPARAM) (allowing_all_concepts ? "all concepts" : ""));

      SendMessage(hwndStatusBar, SB_SETTEXT, 3,
                  (LPARAM) (using_active_phantoms ? "act phan" : ""));

      SendMessage(hwndStatusBar, SB_SETTEXT, 4,
                  (LPARAM) ((ui_options.singing_call_mode == 2) ? "rev singer" :
                            (ui_options.singing_call_mode ? "singer" : "")));

      SendMessage(hwndStatusBar, SB_SETTEXT, 5,
                  (LPARAM) (ui_options.nowarn_mode ? "no warn" : ""));

      SendMessage(hwndStatusBar, SB_SETTEXT, 6,
                  (LPARAM) (allowing_minigrand ? "minigrand" : ""));
   }
   else {
      SendMessage(hwndStatusBar, SB_SETPARTS, 1, (LPARAM) StatusBarDimensions);
   }

   SendMessage(hwndStatusBar, SB_SETTEXT, 0, (LPARAM) szGLOBFirstPane);
   SendMessage(hwndStatusBar, SB_SIMPLE, 0, 0);
   UpdateWindow(hwndStatusBar);
}


static void update_transcript_scroll()
{
   SCROLLINFO Scroll;
   Scroll.cbSize = sizeof(SCROLLINFO);
   Scroll.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
   Scroll.nMin = 0;
   Scroll.nMax = (nTotalImageHeight/TranscriptTextHeight)-1;
   Scroll.nPage = (nActiveTranscriptSize/TranscriptTextHeight);
   Scroll.nPos = nImageOffTop;

   SetScrollInfo(hwndTranscriptArea, SB_VERT, &Scroll, TRUE);
}


static void Update_text_display()
{
   RECT ClientRect;
   DisplayType *DisplayPtr;
   int Ystuff;

   for (Ystuff=0,DisplayPtr=DisplayRoot;
        DisplayPtr->Line[0] != -1;
        DisplayPtr=DisplayPtr->Next) {
      Ystuff += DisplayPtr->DeltaToNext;
   }

   nTotalImageHeight = Ystuff;

   // Round this up.
   nImageOffTop = (nTotalImageHeight-nActiveTranscriptSize+TranscriptTextHeight-1)/
      TranscriptTextHeight;

   if (nImageOffTop < 0) nImageOffTop = 0;
   update_transcript_scroll();
   GetClientRect(hwndTranscriptArea, &ClientRect);
   InvalidateRect(hwndTranscriptArea, &ClientRect, TRUE);  // Be sure we erase the background.
}


static DisplayType *question_stuff_to_erase = (DisplayType *) 0;

static void erase_questionable_stuff()
{
   if (question_stuff_to_erase) {
      CurDisplay = DisplayRoot;
      while (CurDisplay->Line[0] != -1 && CurDisplay != question_stuff_to_erase)
         CurDisplay = CurDisplay->Next;

      CurDisplay->Line[0] = -1;
      question_stuff_to_erase = (DisplayType *) 0;
      Update_text_display();
   }
}


void iofull::show_match()
{
   get_utils_ptr()->show_match_item();
}



static void check_text_change(bool doing_escape)
{
   char szLocalString[MAX_TEXT_LINE_LENGTH];
   int nLen;
   int nIndex;
   char *p;
   int matches;
   bool changed_editbox = false;
   matcher_class &matcher = *gg77->matcher_p;

   // Find out what the text input box contains now.

   GetWindowText(hwndTextInputArea, szLocalString, MAX_TEXT_LINE_LENGTH);
   nLen = strlen(szLocalString) - 1;    // Location of last character.

   for (nIndex=0 ; nIndex<=nLen ; nIndex++)
      szLocalString[nIndex] = tolower(szLocalString[nIndex]);

   // Only process stuff if it changed from what we thought it was.
   // Otherwise we would unnecessarily process changes that weren't typed
   // by the user but were merely the stuff we put in due to completion.

   if (doing_escape) {
      nLen++;
      matches = matcher.match_user_input(nLastOne, false, false, false);
      p = matcher.m_echo_stuff;
      if (*p) {
         changed_editbox = true;

         while (*p) {
            szLocalString[nLen++] = tolower(*p++);
            szLocalString[nLen] = '\0';
         }

      }
   }
   else if (lstrcmp(szLocalString, matcher.m_user_input)) {
      if (nLen >= 0) {
         char cCurChar = szLocalString[nLen];

         if (cCurChar == '!' || cCurChar == '?') {
            szLocalString[nLen] = '\0';   // Erase the '?' itself.

            // Before we start, erase any previous stuff.
            erase_questionable_stuff();

            // Next line used to be "nLen > 0" so that we wouldn't do this if
            // the line was blank.  It was believed that the '?' operation
            // was unwieldy if it printed absolutely everything.  Maybe it
            // was when computers were slower.  But it seems fine now.
            // In particular, on a 150 MHz machine (very slow in 2002),
            // it takes less than 2 seconds to show the full output from
            // a question mark at C4.
            if (nLen > -33) {    // Don't do this on a blank line.
               int saved_image_height = nTotalImageHeight;
               DisplayType *my_mark = CurDisplay;
               matcher.copy_to_user_input(szLocalString);
               // This will call show_match with each match.
               matcher.match_user_input(nLastOne, true, cCurChar == '?', false);
               question_stuff_to_erase = my_mark;
               // Restore the scroll position so that the user will see the start,
               // not the end, of what we displayed.
               nImageOffTop = saved_image_height/TranscriptTextHeight;
               int AmountWeHaveToHide = (nTotalImageHeight-nActiveTranscriptSize)
                  /TranscriptTextHeight + 1;
               if (AmountWeHaveToHide<0) AmountWeHaveToHide = 0;
               // Don't hide stuff unnecessarily.  If we would have space to show
               // some pre-existing text as well as all "?" output, do so.
               if (nImageOffTop > AmountWeHaveToHide) nImageOffTop = AmountWeHaveToHide;
               update_transcript_scroll();
               // Give focus to the transcript, so the user can scroll easily.
               SetFocus(hwndTranscriptArea);
            }
            changed_editbox = true;
         }
         else if (cCurChar == ' ' || cCurChar == '-') {
            erase_questionable_stuff();
            lstrcpy(matcher.m_user_input, szLocalString);
            matcher.m_user_input[nLen] = '\0';
            // **** do we think nLen has the right stuff here?
            matcher.m_user_input_size = strlen(matcher.m_user_input);
            // Extend only to one space or hyphen, inclusive.
            matches = matcher.match_user_input(nLastOne, false, false, true);
            p = matcher.m_echo_stuff;

            if (*p) {
               changed_editbox = true;

               while (*p) {
                  if (*p != ' ' && *p != '-') {
                     szLocalString[nLen++] = *p++;
                     szLocalString[nLen] = '\0';
                  }
                  else {
                     szLocalString[nLen++] = cCurChar;
                     szLocalString[nLen] = '\0';
                     goto pack_us;
                  }
               }
            }
            else if (!matcher.m_space_ok || matches <= 1) {
               uims_bell();
               szLocalString[nLen] = '\0';    // Do *not* pack the character.
               changed_editbox = true;
            }
         }
         else
            erase_questionable_stuff();
      }
      else {
         erase_questionable_stuff();
         goto scroll_call_menu;
      }
   }
   else {
      goto scroll_call_menu;
   }

 pack_us:

   matcher.copy_to_user_input(szLocalString);

   for (p=matcher.m_user_input ; *p ; p++)
      *p = tolower(*p);

   // Write it back to the window.

   if (changed_editbox) {
      SendMessage(hwndTextInputArea, WM_SETTEXT, 0, (LPARAM) szLocalString);
      // This moves the cursor to the end of the text, apparently.
      SendMessage(hwndTextInputArea, EM_SETSEL, MAX_TEXT_LINE_LENGTH - 1, MAX_TEXT_LINE_LENGTH - 1);
   }

 scroll_call_menu:

   // Search call menu for match on current string.

   nIndex = SendMessage(hwndCallMenu, LB_SELECTSTRING, (WPARAM) -1, (LPARAM) szLocalString);

   if (nIndex >= 0) {
      // If possible, scroll the call menu so that
      // current selection remains centered.
      SendMessage(
         hwndCallMenu, LB_SETTOPINDEX,
         (nIndex > LISTBOX_SCROLL_POINT) ? nIndex - LISTBOX_SCROLL_POINT : 0,
         0);
      menu_moved = false;
   }
   else if (!szLocalString[0]) {
      // No match and no string.
      nIndex = 0;  // Select first entry in call menu.
      SendMessage(hwndCallMenu, LB_SETCURSEL, 0, 0L);
   }
   else
      menu_moved = false;
}




/* Look for special programmed keystroke.  Act on it, and return 2 if it finds one.
   Return 1 if we did not recognize it, but we don't want to shift
   focus automatically to the edit window.
   Return 0 if it is not a recognized defined keystroke, but we think the
   edit window ought to handle it. */
static int LookupKeystrokeBinding(
   UINT iMsg, WPARAM wParam, LPARAM lParam, WPARAM crcode)
{
   modifier_block *keyptr;
   int nc;
   uint32_t ctlbits;
   int newparm = -99;
   matcher_class &matcher = *gg77->matcher_p;

   switch (iMsg) {
   case WM_KEYDOWN:
   case WM_SYSKEYDOWN:
      ctlbits = (GetKeyState(VK_CONTROL)>>15) & 1;
      ctlbits |= (GetKeyState(VK_SHIFT)>>14) & 2;
      if (HIWORD(lParam) & KF_ALTDOWN) ctlbits |= 4;

      if (wParam == VK_RETURN || wParam == VK_ESCAPE) {
         // These were already handled as "WM_CHAR" messages.
         // Don't let either kind of message get to the default windproc
         // when "enter" or "escape" is typed.
         return 2;
      }
      else if (wParam == VK_SHIFT || wParam == VK_CONTROL) {
         return 1;    // We are not handling these, but don't change focus.
      }
      else if (wParam == VK_TAB) {
         // The tab key, under normal Windows conventions, moves focus around
         // in one direction or the other among the focusable windows.  We have
         // very little use for that, since most of the time focus is on the
         // edit box, that is, the text input area.  On the other hand, we commonly
         // want a "completion" character.  In Sdtty, that is either tab or escape.
         // In Sd, escape will have that effect in any case, but we usually want
         // tab to do that also.  The reason is that tab has that meaning in Emacs,
         // and commonly has that meaning in a Windows command prompt window.
         // Escape in a Windows command prompt window erases all input, so it's
         // much safer to be in the habit of typing tab to get completion.
         // Therefore, we normally usurp the conventional Windows meaning of tab,
         // and have it cause completion.  Windows also has the convention that
         // shift-tab moves focus in the opposite direction, and we leave that
         // convention in place.  So you can still manually shift focus if you
         // want to.  We also allow the "tab_changes_focus" option to make
         // plain tab have its convention Windows meaning.
         // Windows is used in this paragraph as a reference to a registered
         // trademark of Microsoft, a notorious software / virus propagation
         // company.
         if (ui_options.tab_changes_focus || (ctlbits & 2)) {
            ButtonFocusIndex += 1 - (ctlbits & 2);   // Yeah, sleazy.
            if (ButtonFocusIndex > ButtonFocusHigh) ButtonFocusIndex = 0;
            else if (ButtonFocusIndex < 0) ButtonFocusIndex = ButtonFocusHigh;
            SetFocus(ButtonFocusTable[ButtonFocusIndex]);
         }
         else
            PostMessage(hwndMain, WM_COMMAND, ESCAPE_INDEX, (LPARAM) hwndCallMenu);

         return 2;    // One way or the other, we have handled it.
      }
      else if (wParam == 0x0C) {
         if (!(HIWORD(lParam) & KF_EXTENDED))
            newparm = 5-200;
      }
      else if (wParam >= VK_PRIOR && wParam <= VK_DELETE) {
         if (HIWORD(lParam) & KF_EXTENDED) {
            if (ctlbits == 0)      newparm = wParam-VK_PRIOR+1+EKEY+SPECIAL_KEY_OFFSET;
            else if (ctlbits == 1) newparm = wParam-VK_PRIOR+1+CEKEY+SPECIAL_KEY_OFFSET;
            else if (ctlbits == 2) newparm = wParam-VK_PRIOR+1+SEKEY+SPECIAL_KEY_OFFSET;
            else if (ctlbits == 4) newparm = wParam-VK_PRIOR+1+AEKEY+SPECIAL_KEY_OFFSET;
            else if (ctlbits == 5) newparm = wParam-VK_PRIOR+1+CAEKEY+SPECIAL_KEY_OFFSET;
         }
         else {
            switch (wParam) {
            case 0x2D: newparm = 0-200; break;
            case 0x23: newparm = 1-200; break;
            case 0x28: newparm = 2-200; break;
            case 0x22: newparm = 3-200; break;
            case 0x25: newparm = 4-200; break; // 5 (wParam == 0x0C) is handled above
            case 0x27: newparm = 6-200; break;
            case 0x24: newparm = 7-200; break;
            case 0x26: newparm = 8-200; break;
            case 0x21: newparm = 9-200; break;
            }
         }
      }
      else if (wParam >= VK_F1 && wParam <= VK_F12) {
            if (ctlbits == 0)      newparm = wParam-VK_F1+1+FKEY+SPECIAL_KEY_OFFSET;
            else if (ctlbits == 1) newparm = wParam-VK_F1+1+CFKEY+SPECIAL_KEY_OFFSET;
            else if (ctlbits == 2) newparm = wParam-VK_F1+1+SFKEY+SPECIAL_KEY_OFFSET;
            else if (ctlbits == 4) newparm = wParam-VK_F1+1+AFKEY+SPECIAL_KEY_OFFSET;
            else if (ctlbits == 5) newparm = wParam-VK_F1+1+CAFKEY+SPECIAL_KEY_OFFSET;
      }
      else if (wParam >= 'A' && wParam <= 'Z' && (ctlbits & 4)) {
         // We take alt or ctl-alt letters as key presses.
         if (ctlbits & 1) newparm = wParam+CTLALTLET+SPECIAL_KEY_OFFSET;
         else             newparm = wParam+ALTLET+SPECIAL_KEY_OFFSET;
      }
      else if (wParam >= '0' && wParam <= '9') {
            if (ctlbits == 1)      newparm = wParam-'0'+CTLDIG+SPECIAL_KEY_OFFSET;
            else if (ctlbits == 4) newparm = wParam-'0'+ALTDIG+SPECIAL_KEY_OFFSET;
            else if (ctlbits == 5) newparm = wParam-'0'+CTLALTDIG+SPECIAL_KEY_OFFSET;
      }
      break;
   case WM_CHAR:
   case WM_SYSCHAR:
      if (wParam == VK_RETURN) {
         // If user hit "<enter>", act roughly as though she clicked the "ACCEPT"
         // button, or whatever.  It depends on what box had the focus.
         // If this is the edit box, use "ENTER_INDEX", which makes the parser
         // go through all the ambiguity resolution stuff.  If this is the list
         // box, use "ACCEPT_BUTTON_INDEX", which causes the indicated choice to be
         // accepted immediately, just as though the "ACCEPT" button had been clicked.

         // But if user tabbed to the cancel button, do that instead.
         if (InPopup && ButtonFocusIndex == 2)
            crcode = CANCEL_BUTTON_INDEX;

         SetFocus(hwndTextInputArea);    // Take focus away from the button.
         ButtonFocusIndex = 0;
         PostMessage(hwndMain, WM_COMMAND, crcode, (LPARAM) hwndCallMenu);
         return 2;
      }
      else if (wParam == VK_ESCAPE) {
         PostMessage(hwndMain, WM_COMMAND, ESCAPE_INDEX, (LPARAM) hwndCallMenu);
         return 2;
      }
      else if (wParam == VK_TAB)
         // This is being handled as a "WM_KEYDOWN" message.
         return 2;
      else if (wParam >= ('A' & 0x1F) && wParam <= ('Z' & 0x1F)) {
         // We take ctl letters as characters.
         newparm = wParam+0x40+CTLLET+SPECIAL_KEY_OFFSET;
      }
      else if (wParam >= '0' && wParam <= '9') {
         // We deliberately throw away alt digits.
         // The default window proc would ding them (so we have to return 2
         // to prevent that), and we are grabbing them through the KEYDOWN message,
         // so we don't need them here.
         if (HIWORD(lParam) & KF_ALTDOWN)
            return 2;
      }

      break;
   }

   /* Now see whether this keystroke was a special "accelerator" key.
      If so, just do the command.
      If it was "<enter>", treat it as though we had double-clicked the selected
      menu item, or had clicked on "accept" in the menu.
      Otherwise, it goes back to the real window.  But be sure that window
      is the edit window, not the menu. */

   // Check first for special numeric keypad hit, indicated by a number
   // close to -200.  If no control or alt was pressed, treat it as a
   // plain digit.

   if (newparm < -150) {
      ctlbits &= ~2;     // Take out shift bit.
         if (ctlbits == 0) {
            // Plain numeric keypad is same as the digit itself.
            SendMessage(hwndTextInputArea, WM_CHAR, newparm+200+'0', lParam);
            return 2;
         }
         else if (ctlbits == 1) newparm += 200+CTLNKP+SPECIAL_KEY_OFFSET;
         else if (ctlbits == 5) newparm += 200+CTLALTNKP+SPECIAL_KEY_OFFSET;
         else if (ctlbits == 4) newparm += 200+ALTNKP+SPECIAL_KEY_OFFSET;
   }

   if (newparm == -99) return 0;
   nc = newparm - SPECIAL_KEY_OFFSET;

   if (nc < FCN_KEY_TAB_LOW || nc > FCN_KEY_TAB_LAST)
      return 0;      /* How'd this happen?  Ignore it. */

   keyptr = matcher.m_fcn_key_table_normal[nc-FCN_KEY_TAB_LOW];

   /* Check for special bindings like "delete line" or "page up".
      These always come from the main binding table, even if
      we are doing something else, like a resolve.  So, in that case,
      we bypass the search for a menu-specific binding. */

   if (!keyptr || keyptr->index >= 0) {

      /* Look for menu-specific bindings like
         "split phantom boxes" or "find another". */

      if (nLastOne == matcher_class::e_match_startup_commands)
         keyptr = matcher.m_fcn_key_table_start[nc-FCN_KEY_TAB_LOW];
      else if (nLastOne == matcher_class::e_match_resolve_commands)
         keyptr = matcher.m_fcn_key_table_resolve[nc-FCN_KEY_TAB_LOW];
      else if (nLastOne < 0)
         keyptr = (modifier_block *) 0;    /* We are scanning for
                                              direction/selector/number/etc. */
   }

   if (keyptr) {
      if (keyptr->index < 0) {
         // This function key specifies a special "syntactic" action.
         int nCount;
         int nIndex = 1;
         char szLocalString[MAX_TEXT_LINE_LENGTH];

         switch (keyptr->index) {
         case special_index_pageup:
            nIndex -= PAGE_LEN-1;     // !!!! FALL THROUGH !!!!
         case special_index_lineup:
            nIndex -= PAGE_LEN+1;     // !!!! FALL THROUGH !!!!
         case special_index_pagedown:
            nIndex += PAGE_LEN-1;     // !!!! FALL THROUGH !!!!
         case special_index_linedown:
            // !!!! FELL THROUGH !!!!
            // nIndex now tells how far we want to move forward or back in the menu.
            // Change that to the absolute new position, by adding the old position.
            nIndex += SendMessage(hwndCallMenu, LB_GETCURSEL, 0, 0);

            // Clamp to the menu limits.
            nCount = SendMessage(hwndCallMenu, LB_GETCOUNT, 0, 0) - 1;
            if (nIndex > nCount) nIndex = nCount;
            if (nIndex < 0) nIndex = 0;
            menu_moved = true;
            // Select the new item.
            SendMessage(hwndCallMenu, LB_SETCURSEL, nIndex, 0);
            break;
         case special_index_deleteword:
            GetWindowText(hwndTextInputArea, szLocalString, MAX_TEXT_LINE_LENGTH);
            matcher.copy_to_user_input(szLocalString);
            matcher.delete_matcher_word();
            SendMessage(hwndTextInputArea, WM_SETTEXT, 0, (LPARAM) matcher.m_user_input);
            SendMessage(hwndTextInputArea, EM_SETSEL, MAX_TEXT_LINE_LENGTH, MAX_TEXT_LINE_LENGTH);
            break;
         case special_index_deleteline:
            matcher.erase_matcher_input();
            SendMessage(hwndTextInputArea, WM_SETTEXT, 0, (LPARAM) matcher.m_user_input);
            SendMessage(hwndTextInputArea, EM_SETSEL, MAX_TEXT_LINE_LENGTH, MAX_TEXT_LINE_LENGTH);
            break;
         case special_index_copytext:
            SendMessage(hwndTextInputArea, WM_COPY, 0, 0);
            break;
         case special_index_cuttext:
            SendMessage(hwndTextInputArea, WM_CUT, 0, 0);
            break;
         case special_index_pastetext:
            SendMessage(hwndTextInputArea, WM_PASTE, 0, 0);
            break;
         case special_index_quote_anything:
            GetWindowText(hwndTextInputArea, szLocalString, MAX_TEXT_LINE_LENGTH);
            lstrcat(szLocalString, "<anything>");
            matcher.copy_to_user_input(szLocalString);
            SendMessage(hwndTextInputArea, WM_SETTEXT, 0, (LPARAM) matcher.m_user_input);
            SendMessage(hwndTextInputArea, EM_SETSEL, MAX_TEXT_LINE_LENGTH, MAX_TEXT_LINE_LENGTH);
            break;
         }
      }
      else {
         // This function key specifies a normal "dancing" action.
         matcher.m_final_result.match = *keyptr;
         matcher.m_final_result.indent = false;
         matcher.m_final_result.valid = true;

         // We have the fully filled in match item.
         // Process it and exit from the command loop.

         WaitingForCommand = false;
      }

      return 2;
   }

   return 0;
}



static void MainWindow_OnDestroy(HWND hwnd)
{
   PostQuitMessage(0);
}



/* Process "about" window messages */

LRESULT WINAPI AboutWndProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
   switch (Message) {
   case WM_INITDIALOG:
      return TRUE;
   case WM_COMMAND:
      EndDialog(hDlg, TRUE);
      return TRUE;
   }
   return FALSE;
}

static uint32_t text_color_translate[8] = {
  RGB(0, 0, 0),      // 0 - not used
  RGB(128, 128, 0),  // 1 - substitute for yellow against bright background
  RGB(255, 0, 0),    // 2 - red
  RGB(0, 255, 0),    // 3 - green
  RGB(255, 255, 0),  // 4 - yellow
  RGB(0, 0, 255),    // 5 - blue
  RGB(255, 0, 255),  // 6 - magenta
  RGB(0, 255, 255)}; // 7 - cyan

RGBQUAD icon_color_translate[8];    // Will be filled in during initialization.


// Margin, in pixels, around the top, right, and bottom of the transcript.
// That is, this is the amount of gray space from the edge of the black
// space to the edge of the client area.  That is, the client area as
// defined by Windows.  So we set these values to whatever makes the gray
// border look nice.  We don't define a left edge margin, because the left
// edge is against other things, rather than against the window edge.
#define TRANSCRIPT_TOPMARGIN 2
#define TRANSCRIPT_BOTMARGIN 0
#define TRANSCRIPT_RIGHTMARGIN 0

// Distance from bottom of call menu to top of "Accept" button.
#define MENU_TO_BUTTON 8
// Distance from top of "Accept" button to bottom of screen.
#define BUTTONTOP 25

// Distance from the left edge of the client rectangle (as defined
// by Windows) to the start of the listbox/editbox/etc.
#define LEFTJUNKEDGE 8
#define RIGHTJUNKEDGE 13

#define EDITTOP 2
#define EDITTOLIST 3

// Height of progress bar.
#define PROGRESSHEIGHT 15
#define PROGRESSBOT 5

// Vertical margin between the text and the edge of the transcript area.
// That is, this is the margin in the black space.  It applies at both
// top and bottom, but the bottom margin may be greater than this because
// we display only an integral number of lines.
#define TVOFFSET 5
// Horizontal margin between the left edge of the black area and the start
// of the text.  We don't define a right margin -- we display as much as
// possible when a line is very long.
#define THOFFSET 5

// Size of the square pixel array in the bitmap for one person.
// The bitmap is exactly 8 of these wide and 8 of them (well, 9) high.
#define BMP_PERSON_SIZE 36
// This should be even.
// was 10
#define BMP_PERSON_SPACE 0

static uint32_t plaintext_fg, plaintext_bg;

static void Transcript_OnPaint(HWND hwnd)
{
   PAINTSTRUCT PaintStruct;
   int Y;

   HDC PaintDC = BeginPaint(hwnd, &PaintStruct);

   // Be sure we never paint in the margins.

   if (PaintStruct.rcPaint.top < TVOFFSET)
      PaintStruct.rcPaint.top = TVOFFSET;

   if (PaintStruct.rcPaint.bottom > TranscriptClientRect.bottom-TVOFFSET)
      PaintStruct.rcPaint.bottom = TranscriptClientRect.bottom-TVOFFSET;

   SelectFont(PaintDC, GetStockObject(OEM_FIXED_FONT));
   //SelectFont(PaintDC, GetStockObject(SYSTEM_FIXED_FONT));
   //SelectFont(PaintDC, GetStockObject(ANSI_FIXED_FONT));
   SetTextColor(PaintDC, plaintext_fg);
   SetBkColor(PaintDC, plaintext_bg);

   SelectPalette(PaintStruct.hdc, hPalette, FALSE);
   RealizePalette(PaintStruct.hdc);

   DisplayType *DisplayPtr;

   for (Y=TVOFFSET-nImageOffTop*TranscriptTextHeight,DisplayPtr=DisplayRoot;
        DisplayPtr && DisplayPtr->Line[0] != -1;
        Y+=DisplayPtr->DeltaToNext,DisplayPtr=DisplayPtr->Next) {
      int x, xdelta;
      const char *cp;

      // See if we are at the part scrolled off the top of the screen.
      if (Y+DisplayPtr->Height < TVOFFSET) continue;

      // Or if we have run off the bottom.
      if (Y > TranscriptClientRect.bottom-TVOFFSET) break;

      for (cp=DisplayPtr->Line,x=THOFFSET;
           *cp;
           cp++,x+=xdelta) {
         int xgoodies, ygoodies, glyph_height, glyph_offset;
         int the_count = 1;         // Fill in some defaults.
         const char *the_string = cp;

         if (DisplayPtr->in_picture & 1) {
            if (*cp == '\013') {
               // Display a person glyph.
               int personidx = (*++cp) & 7;
               int persondir = (*++cp) & 0xF;

               int randomized_person_color = personidx;

               if (ui_options.color_scheme == color_by_couple_random && ui_options.hide_glyph_numbers) {
                  randomized_person_color = (color_randomizer[personidx>>1] << 1) | (personidx & 1);
               }

               if (ui_options.no_graphics == 0) {
                  xgoodies = randomized_person_color*BMP_PERSON_SIZE;
                  ygoodies = BMP_PERSON_SIZE*((persondir & 3)+(ui_options.hide_glyph_numbers ? 4 : 0));
                  goto do_DIB_thing;
               }
               else {
                  char cc[3];
                  cc[0] = ' ';

                  ExtTextOut(PaintDC, x, Y, ETO_CLIPPED, &PaintStruct.rcPaint, cc, 1, 0);

                  if (ui_options.color_scheme != no_color)
                     SetTextColor(PaintDC, text_color_translate[color_index_list[randomized_person_color]]);

                  cc[0] = ui_options.pn1[personidx];
                  cc[1] = ui_options.pn2[personidx];
                  cc[2] = ui_options.direc[persondir];

                  ExtTextOut(PaintDC, x, Y, ETO_CLIPPED, &PaintStruct.rcPaint, cc, 3, 0);

                  // Set back to plain "white".

                  if (ui_options.color_scheme != no_color)
                     SetTextColor(PaintDC, plaintext_fg);

                  xdelta = TranscriptTextWidth*4;
                  continue;
               }
            }
            else if (*cp == '\014') {
               // Display a dot for a phantom.
               if (ui_options.no_graphics == 0) {
                  xgoodies = 0;
                  ygoodies = BMP_PERSON_SIZE*8;
                  goto do_DIB_thing;
               }
               else {
                  the_string = "  . ";
                  the_count = 4;
               }
            }
            else if (*cp == '6') {
               // 6 means space equivalent to one person size.
               xdelta = (ui_options.no_graphics == 0) ?
                  (BMP_PERSON_SIZE) : (TranscriptTextWidth*4);
               continue;
            }
            else if (*cp == '5') {
               // 5 means space equivalent to half of a person size.
               xdelta = (ui_options.no_graphics == 0) ?
                  (BMP_PERSON_SIZE/2) : (TranscriptTextWidth*2);
               continue;
            }
            else if (*cp == '9') {
               // 9 means space equivalent to 3/4 of a person size.
               xdelta = (ui_options.no_graphics == 0) ?
                  (3*BMP_PERSON_SIZE/4) : (TranscriptTextWidth*3);
               continue;
            }
            else if (*cp == '8') {
               // 8 means space equivalent to half of a person size
               // if doing checkers, but only one space if in ASCII.
               xdelta = (ui_options.no_graphics == 0) ?
                  (BMP_PERSON_SIZE/2) : (TranscriptTextWidth);
               continue;
            }
            else if (*cp == ' ') {
               // The tables generally use two blanks as the inter-person spacing.
               xdelta = (ui_options.no_graphics == 0) ?
                  (BMP_PERSON_SPACE/2) : (TranscriptTextWidth);
               continue;
            }
         }

         // If we get here, we need to write a plain text string.

         xdelta = TranscriptTextWidth*the_count;
         ExtTextOut(PaintDC, x, Y, ETO_CLIPPED, &PaintStruct.rcPaint, the_string, the_count, 0);
         continue;

      do_DIB_thing:

         // Clip this stuff -- be sure we don't go into the top or bottom margin.

         glyph_height = BMP_PERSON_SIZE;
         glyph_offset = 0;

         if (Y+BMP_PERSON_SIZE > PaintStruct.rcPaint.bottom) {
            glyph_height -= Y+BMP_PERSON_SIZE-PaintStruct.rcPaint.bottom;
            ygoodies += Y+BMP_PERSON_SIZE-PaintStruct.rcPaint.bottom;
         }
         else if (Y < PaintStruct.rcPaint.top) {
            glyph_height -= PaintStruct.rcPaint.top-Y;
            glyph_offset = PaintStruct.rcPaint.top-Y;
         }

         SetDIBitsToDevice(PaintStruct.hdc,
                           x, Y+glyph_offset,   // XY coords on screen where we put UL corner
                           BMP_PERSON_SIZE,     // width of it
                           glyph_height,        // height of it
                           xgoodies,            // X of LL corner of DIB
                           ygoodies,            // Y of LL corner of DIB
                           0,                   // starting scan line of the DIB
                           lpBi->bmiHeader.biHeight,  // It needs the rasterization info.
                           lpBits,     // ptr to actual image in the DIB
                           lpBi,       // ptr to header and color data
                           DIB_RGB_COLORS);

         xdelta = BMP_PERSON_SIZE;
      }
   }

   EndPaint(hwnd, &PaintStruct);
}


static void Transcript_OnScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
   int delta;
   SCROLLINFO Scroll;
   RECT ClientRect;
   int oldnImageOffTop = nImageOffTop;
   // Round this up.
   int newmax = (nTotalImageHeight-nActiveTranscriptSize+TranscriptTextHeight-1)/
      TranscriptTextHeight;

   switch (code) {
   case SB_TOP:
      nImageOffTop = 0;
      break;
   case SB_BOTTOM:
      nImageOffTop = newmax;
      break;
   case SB_LINEUP:
      nImageOffTop--;
      break;
   case SB_LINEDOWN:
      nImageOffTop++;
      break;
   case SB_PAGEUP:
      nImageOffTop -= pagesize;
      break;
   case SB_PAGEDOWN:
      nImageOffTop += pagesize;
      break;
   case SB_THUMBPOSITION:
   case SB_THUMBTRACK:
      nImageOffTop = pos;
      break;
   default:
      return;
   }

   if (nImageOffTop > newmax) nImageOffTop = newmax;
   if (nImageOffTop < 0) nImageOffTop = 0;

   delta = nImageOffTop - oldnImageOffTop;

   if (delta == 0) return;

   Scroll.cbSize = sizeof(SCROLLINFO);
   Scroll.fMask = SIF_POS;
   Scroll.nPos = nImageOffTop;

   SetScrollInfo(hwnd, SB_VERT, &Scroll, TRUE);

   GetClientRect(hwnd, &ClientRect);
   ClientRect.left += THOFFSET;
   ClientRect.top += TVOFFSET;
   ClientRect.bottom -= BottomFudge;

   ScrollWindowEx(hwnd, 0, -delta*TranscriptTextHeight,
                         &ClientRect, &ClientRect, NULL, NULL, SW_ERASE | SW_INVALIDATE);

   if (delta > 0) {
      // Invalidate bottom part only.
      ClientRect.top += nActiveTranscriptSize - delta*TranscriptTextHeight;
   }
   else {
      // Invalidate top part only.
      ClientRect.bottom -= nActiveTranscriptSize + delta*TranscriptTextHeight;
   }

   InvalidateRect(hwnd, &ClientRect, TRUE);  // Be sure we erase the background.
}



// Process get-text dialog box messages.

static popup_return PopupStatus;
static char szPrompt1[MAX_TEXT_LINE_LENGTH];
static char szPrompt2[MAX_TEXT_LINE_LENGTH];
static char szSeed[MAX_TEXT_LINE_LENGTH];
static char szTextEntryResult[MAX_TEXT_LINE_LENGTH];
LRESULT WINAPI TEXT_ENTRY_DIALOG_WndProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
   int Len;

   switch (Message) {
   case WM_INITDIALOG:
      /* If we did this, it would set the actual window title
         SetWindowText(hDlg, "FOOBAR!"); */
      SetDlgItemText(hDlg, IDC_FILE_TEXT1, szPrompt1);
      SetDlgItemText(hDlg, IDC_FILE_TEXT2, szPrompt2);
      SendDlgItemMessage(hDlg, IDC_FILE_EDIT, WM_SETTEXT, 0, (LPARAM) szSeed);
      return TRUE;
   case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDC_FILE_ACCEPT:
         Len = SendDlgItemMessage(hDlg, IDC_FILE_EDIT, EM_LINELENGTH, 0, 0L);
         if (Len > MAX_TEXT_LINE_LENGTH - 1)
            Len = MAX_TEXT_LINE_LENGTH - 1;

         if (Len > 0) {
            GetWindowText(GetDlgItem (hDlg, IDC_FILE_EDIT),
                          szTextEntryResult, MAX_TEXT_LINE_LENGTH);
            PopupStatus = POPUP_ACCEPT_WITH_STRING;
         }
         else
            PopupStatus = POPUP_ACCEPT;

         EndDialog(hDlg, TRUE);
         return TRUE;
      case IDC_FILE_CANCEL:
      case IDCANCEL:
         PopupStatus = POPUP_DECLINE;
         EndDialog(hDlg, TRUE);
         return TRUE;
      }

      return FALSE;
   default:
      return FALSE;      // We do *NOT* call the system default handler.
   }
}


popup_return iofull::get_popup_string(Cstring prompt1, Cstring prompt2, Cstring /*final_inline_prompt*/,
                                      Cstring seed, char *dest)
{
   // We ignore the "final_inline_prompt".  We assume that the appearance of the
   // edit box will clue the user.  But we show other prompts, even if they have asterisks.

   if (prompt1 && prompt1[0] && prompt1[0] == '*')
      strncpy(szPrompt1, prompt1+1, MAX_TEXT_LINE_LENGTH);
   else
      strncpy(szPrompt1, prompt1, MAX_TEXT_LINE_LENGTH);

   if (prompt2 && prompt2[0] && prompt2[0] == '*')
      strncpy(szPrompt2, prompt2+1, MAX_TEXT_LINE_LENGTH);
   else
      strncpy(szPrompt2, prompt2, MAX_TEXT_LINE_LENGTH);

   strncpy(szSeed, seed, MAX_TEXT_LINE_LENGTH);
   DialogBox(GLOBhInstance, MAKEINTRESOURCE(IDD_TEXT_ENTRY_DIALOG),
             hwndMain, (DLGPROC) TEXT_ENTRY_DIALOG_WndProc);
   if (PopupStatus == POPUP_ACCEPT_WITH_STRING)
      lstrcpy(dest, szTextEntryResult);
   else dest[0] = 0;
   return PopupStatus;
}

// This is the top-level entry for Sd, on Windows.  The OS should invoke this
// when the command is given.
int WINAPI WinMain(
   HINSTANCE hInstance,
   HINSTANCE hPrevInstance,
   PSTR szCmdLine,
   int iCmdShow)
{
   GLOBiCmdShow = iCmdShow;
   GLOBhInstance = hInstance;

   // Set the UI options for Sd.

   ui_options.reverse_video = false;
   ui_options.pastel_color = false;

   // Initialize all the callbacks that sdlib will need.
   iofull ggg;

   // Run the Sd program.  The system-supplied variables "__argc"
   // and "__argv" provide the predigested-as-in-traditional-C-programs
   // command-line arguments.
   //
   // January, 2014:  An upgrade to MinGW changed the names of these.
   // They had previously had two initial underscores.  The double-underscore
   // symbols exist, but they don't give what we want.

   return sdmain(__argc, __argv, ggg);
}


BOOL MainWindow_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
   TEXTMETRIC tm;
   HDC hdc = GetDC(hwnd);

   SelectFont(hdc, GetStockObject(OEM_FIXED_FONT));
   GetTextMetrics(hdc, &tm);
   TranscriptTextWidth = tm.tmAveCharWidth;
   TranscriptTextHeight = tm.tmHeight+tm.tmExternalLeading;

   SelectFont(hdc, GetStockObject(ANSI_VAR_FONT));
   GetTextMetrics(hdc, &tm);
   AnsiTextWidth = tm.tmAveCharWidth;
   AnsiTextHeight = tm.tmHeight+tm.tmExternalLeading;

   SelectFont(hdc, GetStockObject(SYSTEM_FONT));
   GetTextMetrics(hdc, &tm);
   SystemTextWidth = tm.tmAveCharWidth;
   SystemTextHeight = tm.tmHeight+tm.tmExternalLeading;

   ReleaseDC(hwnd, hdc);

   hwndTextInputArea = CreateWindow("edit", NULL,
      /* We use "autoscroll" so that it will scroll if we type in too
         much text, but we don't put up a scroll bar with HSCROLL. */
      WS_CHILD|WS_BORDER|ES_LEFT|ES_AUTOHSCROLL,
      0, 0, 0, 0,
      hwnd, (HMENU) TEXT_INPUT_AREA_INDEX,
      lpCreateStruct->hInstance, NULL);

   OldTextInputAreaWndProc = (WNDPROC) SetWindowLong(hwndTextInputArea, GWL_WNDPROC, (LONG) TextInputAreaWndProc);

   hwndCallMenu = CreateWindow("listbox", NULL,
      WS_CHILD|LBS_NOTIFY|WS_VSCROLL|WS_BORDER,
      0, 0, 0, 0,
      hwnd, (HMENU) CALL_MENU_INDEX,
      lpCreateStruct->hInstance, NULL);

   SendMessage(hwndCallMenu, WM_SETFONT, (WPARAM) GetStockObject(ANSI_VAR_FONT), 0);
   OldCallMenuWndProc = (WNDPROC) SetWindowLong(hwndCallMenu, GWL_WNDPROC, (LONG) CallMenuWndProc);

   hwndAcceptButton = CreateWindow("button", "Accept",
      WS_CHILD|BS_DEFPUSHBUTTON,
      0, 0, 0, 0,
      hwnd, (HMENU) ACCEPT_BUTTON_INDEX,
      lpCreateStruct->hInstance, NULL);

   SendMessage(hwndAcceptButton, WM_SETFONT, (WPARAM) GetStockObject(ANSI_VAR_FONT), 0);
   OldAcceptButtonWndProc = (WNDPROC) SetWindowLong(hwndAcceptButton, GWL_WNDPROC, (LONG) AcceptButtonWndProc);

   hwndCancelButton = CreateWindow("button", "Cancel",
      WS_CHILD,
      0, 0, 0, 0,
      hwnd, (HMENU) CANCEL_BUTTON_INDEX,
      lpCreateStruct->hInstance, NULL);

   SendMessage(hwndCancelButton, WM_SETFONT, (WPARAM) GetStockObject(ANSI_VAR_FONT), 0);
   OldCancelButtonWndProc = (WNDPROC) SetWindowLong(hwndCancelButton, GWL_WNDPROC, (LONG) CancelButtonWndProc);

   hwndProgress = CreateWindow(PROGRESS_CLASS, NULL,
      WS_CHILD|WS_CLIPSIBLINGS,
      0, 0, 0, 0,
      hwnd, (HMENU) PROGRESS_INDEX,
      lpCreateStruct->hInstance, NULL);

   hwndTranscriptArea = CreateWindow(
      szTranscriptWindowName, NULL,
      WS_CHILD|WS_VISIBLE|WS_BORDER|WS_CLIPSIBLINGS | WS_VSCROLL,
      0, 0, 0, 0,
      hwnd, (HMENU) TRANSCRIPT_AREA_INDEX,
      lpCreateStruct->hInstance, NULL);

   hwndStatusBar = CreateStatusWindow(
      WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|CCS_BOTTOM|SBARS_SIZEGRIP,
      "",
      hwnd,
      STATUSBAR_INDEX);

   if (!hwndProgress||!hwndAcceptButton||!hwndCancelButton||!hwndCallMenu||
       !hwndTextInputArea||!hwndTranscriptArea||!hwndStatusBar) {
      gg77->iob88.fatal_error_exit(1, "Can't create windows", 0);
   }

   return TRUE;
}


static void PositionAcceptButtons()
{
   if (InPopup) {
      ButtonFocusTable[0] = hwndTextInputArea;
      ButtonFocusTable[1] = hwndAcceptButton;
      ButtonFocusTable[2] = hwndCancelButton;
      ButtonFocusTable[3] = hwndTranscriptArea;

      MoveWindow(hwndAcceptButton,
                 (TranscriptEdge/2)-12*AnsiTextWidth, ButtonTopYCoord,
                 10*AnsiTextWidth, 7*AnsiTextHeight/4, TRUE);

      MoveWindow(hwndCancelButton,
                 (TranscriptEdge/2)+2*AnsiTextWidth, ButtonTopYCoord,
                 10*AnsiTextWidth, 7*AnsiTextHeight/4, TRUE);
   }
   else {
      ButtonFocusTable[0] = hwndTextInputArea;
      ButtonFocusTable[1] = hwndAcceptButton;
      ButtonFocusTable[2] = hwndTranscriptArea;

      MoveWindow(hwndAcceptButton,
                 (TranscriptEdge/2)-5*AnsiTextWidth, ButtonTopYCoord,
                 10*AnsiTextWidth, 7*AnsiTextHeight/4, TRUE);
   }
}


void MainWindow_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
   RECT ClientRect;
   RECT rWindow;
   int TranscriptXSize;
   int TranscriptYSize;
   int cyy;
   int Listtop;
   int newmax;

   // We divide between the menu and the transcript at 40%.
   TranscriptEdge = 4*cx/10;

   GetWindowRect(hwndStatusBar, &rWindow);
   cyy = rWindow.bottom - rWindow.top;
   cy -= cyy;    // Subtract the status bar height.

   MoveWindow(hwndStatusBar, 0, cy, cx, cyy, TRUE);
   GLOBStatusBarLength = cx;
   UpdateStatusBar((Cstring) 0);

   TranscriptXSize = cx-TranscriptEdge-TRANSCRIPT_RIGHTMARGIN;
   TranscriptYSize = cy-TRANSCRIPT_BOTMARGIN-TRANSCRIPT_TOPMARGIN;
   // Y-coordinate of the top of the "accept" and "cancel" buttons.
   ButtonTopYCoord = cy-BUTTONTOP;

   // Y-coordinate of the top of the call menu.
   Listtop = EDITTOP+21*SystemTextHeight/16+EDITTOLIST;

   MoveWindow(hwndTextInputArea,
      LEFTJUNKEDGE, EDITTOP,
      TranscriptEdge-LEFTJUNKEDGE-RIGHTJUNKEDGE, 21*SystemTextHeight/16, TRUE);

   MoveWindow(hwndCallMenu,
      LEFTJUNKEDGE, Listtop,
      TranscriptEdge-LEFTJUNKEDGE-RIGHTJUNKEDGE, ButtonTopYCoord-MENU_TO_BUTTON-Listtop, TRUE);

   GetClientRect(hwndCallMenu, &CallsClientRect);

   PositionAcceptButtons();

   MoveWindow(hwndProgress,
      LEFTJUNKEDGE, cy-PROGRESSHEIGHT-PROGRESSBOT,
      TranscriptEdge-LEFTJUNKEDGE-RIGHTJUNKEDGE, PROGRESSHEIGHT, TRUE);

   MoveWindow(hwndTranscriptArea,
      TranscriptEdge, TRANSCRIPT_TOPMARGIN,
      TranscriptXSize, TranscriptYSize, TRUE);

   GetClientRect(hwndTranscriptArea, &TranscriptClientRect);

   // Allow TVOFFSET amount of margin at both top and bottom.
   nActiveTranscriptSize = TranscriptYSize-TVOFFSET-TVOFFSET;
   // We overlap by 5 scroll units when we scroll by a whole page.
   // That way, at least one checker will be preserved.
   pagesize = nActiveTranscriptSize/TranscriptTextHeight-5;
   if (pagesize < 5) pagesize = 5;
   BottomFudge = TranscriptYSize-TVOFFSET - nActiveTranscriptSize;

   // Round this up.
   newmax = (nTotalImageHeight-nActiveTranscriptSize+TranscriptTextHeight-1)/
      TranscriptTextHeight;

   if (nImageOffTop > newmax) nImageOffTop = newmax;
   if (nImageOffTop < 0) nImageOffTop = 0;

   update_transcript_scroll();
   GetClientRect(hwnd, &ClientRect);
   // **** This is actually excessive.  Try to invalidate just the newly exposed stuff.
   InvalidateRect(hwnd, &ClientRect, TRUE);  // Be sure we erase the background.
}


bool iofull::help_manual()
{
   ShellExecute(NULL, "open", "c:\\sd\\sd_doc.html", NULL, NULL, SW_SHOWNORMAL);
   return TRUE;
}


bool iofull::help_faq()
{
   ShellExecute(NULL, "open", "c:\\sd\\faq.html", NULL, NULL, SW_SHOWNORMAL);
   return TRUE;
}


void MainWindow_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   int i;
   int matches;
   int nMenuIndex;
   matcher_class &matcher = *gg77->matcher_p;

   switch (id) {
   case ID_HELP_ABOUTSD:
      DialogBox(GLOBhInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, (DLGPROC) AboutWndProc);
      break;
   case ID_HELP_SDHELP:
      // The claim is that we can take this clause out, and the normal
      // program mechanism will do the same thing.  That claim isn't yet
      // completely true, so we leave this in for now.
      gg77->iob88.help_manual();
      break;
   case ID_HELP_FAQ:
      // Ditto.
      gg77->iob88.help_faq();
      break;
   case ID_FILE_EXIT:
      SendMessage(hwndMain, WM_CLOSE, 0, 0L);
      break;
   case TEXT_INPUT_AREA_INDEX:
      if (codeNotify == EN_UPDATE)
         check_text_change(false);
      break;
   case ESCAPE_INDEX:
      check_text_change(true);
      break;
   case CANCEL_BUTTON_INDEX:
      matcher.m_final_result.match.index = -1;
      WaitingForCommand = false;
      break;
   case CALL_MENU_INDEX:
      // See whether this an appropriate single-click or double-click.
      if (codeNotify != (ui_options.accept_single_click ?
                         (UINT) LBN_SELCHANGE : (UINT) LBN_DBLCLK))
         break;
      // !!!! FALL THROUGH !!!!
   case ENTER_INDEX:
   case ACCEPT_BUTTON_INDEX:
      // !!!! FELL THROUGH !!!!

      erase_questionable_stuff();
      nMenuIndex = SendMessage(hwndCallMenu, LB_GETCURSEL, 0, 0L);

      // If the user moves around in the call menu (listbox) while there is
      // stuff in the edit box, and then types a CR, we need to clear the
      // edit box, so that the listbox selection will be taken exactly.
      // This is because the wandering around in the call menu may have
      // gone to something that has nothing to do with what was typed
      // in the edit box.  We detect this condition by noticing that the
      // listbox selection has changed from what we left it when we were
      // last trying to make the call menu track the edit box.

      // We also do this if the user selected by clicking the mouse.

      if (id != ENTER_INDEX || menu_moved) {
         SendMessage(hwndTextInputArea, WM_SETTEXT, 0, (LPARAM)"");
         matcher.erase_matcher_input();
      }

      // Look for abbreviations.
      if (gg77->look_up_abbreviations(nLastOne))
         return;

      matches = matcher.match_user_input(nLastOne, false, false, false);

      /* We forbid a match consisting of two or more "direct parse" concepts, such as
         "grand cross".  Direct parse concepts may only be stacked if they are followed
         by a call.  The "match.next" field indicates that direct parse concepts
         were stacked. */

      if ((matches == 1 || matches - matcher.m_yielding_matches == 1 || matcher.m_final_result.exact) &&
          ((!matcher.m_final_result.match.packed_next_conc_or_subcall &&
            !matcher.m_final_result.match.packed_secondary_subcall) ||
           matcher.m_final_result.match.kind == ui_call_select ||
           matcher.m_final_result.match.kind == ui_concept_select)) {

         // The matcher found an acceptable (and possibly quite complex)
         // utterance.  Use it directly.

         SendMessage(hwndTextInputArea, WM_SETTEXT, 0, (LPARAM)"");  // Erase the edit box.
         WaitingForCommand = false;
         return;
      }

      // The matcher isn't happy.  If we got here because the user typed <enter>,
      // that's not acceptable.  Just ignore it.  Unless, of course, the type-in
      // buffer was empty and the user scrolled around, in which case the user
      // clearly meant to accept the currently highlighted item.

      if (id == ENTER_INDEX &&
          (matcher.m_user_input[0] != '\0' || !menu_moved)) break;

      // Or if, for some reason, the menu isn't anywhere, we don't accept it.

      if (nMenuIndex == LB_ERR) break;

      // But if the user clicked on "accept", or did an acceptable single- or
      // double-click of a menu item, that item is clearly what she wants, so
      // we use it.

      i = SendMessage(hwndCallMenu, LB_GETITEMDATA, nMenuIndex, (LPARAM) 0);
      matcher.m_final_result.match.index = LOWORD(i);
      matcher.m_final_result.match.kind = (uims_reply_kind) HIWORD(i);

   use_computed_match:

      matcher.m_final_result.match.packed_next_conc_or_subcall = 0;
      matcher.m_final_result.match.packed_secondary_subcall = 0;
      matcher.m_final_result.match.call_conc_options.initialize();
      matcher.m_final_result.real_next_subcall = (match_result *) 0;
      matcher.m_final_result.real_secondary_subcall = (match_result *) 0;

      SendMessage(hwndTextInputArea, WM_SETTEXT, 0, (LPARAM)"");  // Erase the edit box.

      /* We have the needed info.  Process it and exit from the command loop.
         However, it's not a fully filled in match item from the parser.
         So we need to concoct a low-class match item. */

      if (nLastOne == matcher_class::e_match_number) {
      }
      else if (nLastOne == matcher_class::e_match_circcer) {
         matcher.m_final_result.match.call_conc_options.circcer =
            matcher.m_final_result.match.index+1;
      }
      else if (nLastOne >= matcher_class::e_match_taggers &&
               nLastOne < matcher_class::e_match_taggers+NUM_TAGGER_CLASSES) {
         matcher.m_final_result.match.call_conc_options.tagger =
            ((nLastOne-matcher_class::e_match_taggers) << 5)+matcher.m_final_result.match.index+1;
      }
      else {
         if (matcher.m_final_result.match.kind == ui_concept_select) {
            matcher.m_final_result.match.concept_ptr =
               access_concept_descriptor_table(matcher.m_final_result.match.index);
         }
         else if (matcher.m_final_result.match.kind == ui_call_select) {
            matcher.m_final_result.match.call_ptr =
               main_call_lists[parse_state.call_list_to_use][matcher.m_final_result.match.index];
         }
      }

      WaitingForCommand = false;
      break;
   case ID_COMMAND_COPY_TEXT:
      SendMessage(hwndTextInputArea, WM_COPY, 0, 0);
      break;
   case ID_COMMAND_CUT_TEXT:
      SendMessage(hwndTextInputArea, WM_CUT, 0, 0);
      break;
   case ID_COMMAND_PASTE_TEXT:
      SendMessage(hwndTextInputArea, WM_PASTE, 0, 0);
      break;
   default:
      if (nLastOne == matcher_class::e_match_startup_commands) {
         for (i=0 ; startup_menu[i].startup_name ; i++) {
            if (id == startup_menu[i].resource_id) {
               matcher.m_final_result.match.index = i;
               matcher.m_final_result.match.kind = ui_start_select;
               goto use_computed_match;
            }
         }
      }
      else if (nLastOne >= 0) {
         for (i=0 ; command_menu[i].command_name ; i++) {
            if (id == command_menu[i].resource_id) {
               matcher.m_final_result.match.index = i;
               matcher.m_final_result.match.kind = ui_command_select;
               goto use_computed_match;
            }
         }
      }
      else
         break;
   }
}



LRESULT CALLBACK TextInputAreaWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   // If a bound key is sent to the text area, just act on it.
   // Don't change the focus.  Just do it.  If it's bound to some
   // up/down function, the right thing will be done with the call menu,
   // independently of the focus.

   if (LookupKeystrokeBinding(iMsg, wParam, lParam, ENTER_INDEX) == 2)
      return 1;

   // If it is unbound but is a real up/down arrow key, the user
   // presumably wants to scroll the call menu anyway.  Send the keystroke
   // to the call menu.  Same for the mouse wheel.

   if ((iMsg == WM_KEYDOWN && (HIWORD(lParam) & KF_EXTENDED) &&
        (wParam == VK_PRIOR || wParam == VK_NEXT ||
         wParam == VK_UP || wParam == VK_DOWN)) ||
       iMsg == WM_MOUSEWHEEL) {
      PostMessage(hwndCallMenu, iMsg, wParam, lParam);
      return 1;
   }

   // Otherwise, it belongs here.  This includes unbound left/right/home/end.

   return CallWindowProc(OldTextInputAreaWndProc, hwnd, iMsg, wParam, lParam);
}


LRESULT CALLBACK CallMenuWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   // If a bound key is sent to the call menu, just act on it.
   // Don't change the focus.  Just do it.  If it's bound to some
   // up/down function, the right thing will be done with the call menu,
   // independently of the focus.

   switch (LookupKeystrokeBinding(iMsg, wParam, lParam, ACCEPT_BUTTON_INDEX)) {
   case 2:
      return 1;
   case 0:
      switch (iMsg) {
      case WM_CHAR:
      case WM_SYSCHAR:
         // If a character is sent while the call menu has the focus, it is
         // obviously intended as input to the TextInputArea.  Change the focus
         // to the TextInputArea box and send the character to same.
         SetFocus(hwndTextInputArea);
         ButtonFocusIndex = 0;
         PostMessage(hwndTextInputArea, iMsg, wParam, lParam);
         return 1;
      case WM_MOUSEWHEEL:
         // The user rolled the mouse wheel while focus was in either the
         // text input area or the call menu.  (If it was in the text input
         // area, it was forwarded to us.)
         // The normal system behavior, if we were simply to forward this to
         // the old wndproc, would be to scroll the window but not attempt to
         // keep the highlighted item visible.  We can do better than that.
         // We will move the highlighted item appropriately.

         // But there's more.  Windows doesn't take any action on a menu
         // if the control or shift keys are down.  We are going to take
         // that as a signal that we want to scroll by just 1 instead of 3.
         bool shift_or_control = LOWORD(wParam) != 0;
         int numclicks = (((int) (short int) HIWORD(wParam))/WHEEL_DELTA);

         if (!shift_or_control) numclicks *= 3;
         int newpos = SendMessage(hwndCallMenu, LB_GETCURSEL, 0, 0) - numclicks;

         // Clamp to the menu limits.
         int nCount = SendMessage(hwndCallMenu, LB_GETCOUNT, 0, 0) - 1;
         if (newpos > nCount) newpos = nCount;
         if (newpos < 0) newpos = 0;
         menu_moved = true;
         // Select the new item.
         SendMessage(hwndCallMenu, LB_SETCURSEL, newpos, 0);

         // Then tell the system to scroll the menu, unless shift or control was down,
         // in which case:
         // (a) it can't.  We can't tell the system handler to scroll by 1/3 of the usual
         //    amount.  Believe it or not, it doesn't know how to scroll by one item.
         //    Instead, it scrolls by 3 items every 3rd time we send this message.  That
         //    looks really stupid.
         // (b) Just moving the selection by 1 (which we did above), and not scrolling
         //    the menu at all (unless it needs to do so to avoid having the selection
         //    go off the screen), looks nicer anyway.
         if (shift_or_control) return 1;
         return CallWindowProc(OldCallMenuWndProc, hwnd, iMsg, wParam, lParam);
      }
   }

   return CallWindowProc(OldCallMenuWndProc, hwnd, iMsg, wParam, lParam);
}




LRESULT CALLBACK TranscriptAreaWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   switch (iMsg) {
      HANDLE_MSG(hwnd, WM_PAINT, Transcript_OnPaint);
      HANDLE_MSG(hwnd, WM_VSCROLL, Transcript_OnScroll);
   case WM_MOUSEWHEEL:
      Transcript_OnScroll(hwnd, (HWND) lParam, SB_THUMBTRACK,
                          nImageOffTop-(((int) (short int) HIWORD(wParam))*3/WHEEL_DELTA));
      return 0;
   case WM_LBUTTONDOWN:
      /* User clicked mouse in the transcript area.  Presumably this
         is because she wishes to use the up/down arrow keys for scrolling.
         Take the focus, so that we can do that. */
      SetFocus(hwndTranscriptArea);
      ButtonFocusIndex = ButtonFocusHigh;
      break;
   case WM_KEYDOWN:
      /* User typed a key while the focus was in the transcript area.
         If it's an arrow key, scroll the window.  Otherwise, give the focus
         to the edit window and send the character there. */
      switch (wParam) {
      case VK_PRIOR:
         Transcript_OnScroll(hwnd, hwnd, SB_PAGEUP, 0);
         return 0;
      case VK_NEXT:
         Transcript_OnScroll(hwnd, hwnd, SB_PAGEDOWN, 0);
         return 0;
      case VK_UP:
         Transcript_OnScroll(hwnd, hwnd, SB_LINEUP, 0);
         return 0;
      case VK_DOWN:
         Transcript_OnScroll(hwnd, hwnd, SB_LINEDOWN, 0);
         return 0;
      case VK_HOME:
         Transcript_OnScroll(hwnd, hwnd, SB_TOP, 0);
         return 0;
      case VK_END:
         Transcript_OnScroll(hwnd, hwnd, SB_BOTTOM, 0);
         return 0;
      default:
         /* Some other character.  Check for special stuff.  This will also
            handle tabs, tabbing to the next item for focus. */

         switch (LookupKeystrokeBinding(iMsg, wParam, lParam, ENTER_INDEX)) {
         case 2:
            return 1;
         case 0:
            /* A normal character.  It must be a mistake that we have the focus.
               The edit window is a much more plausible recipient.  Set the focus
               there and post the message. */

            SetFocus(hwndTextInputArea);
            ButtonFocusIndex = 0;
            PostMessage(hwndTextInputArea, iMsg, wParam, lParam);
            return 0;
         }
      }

      break;
   case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
   }

   return DefWindowProc(hwnd, iMsg, wParam, lParam);
}


LRESULT CALLBACK AcceptButtonWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   if (LookupKeystrokeBinding(iMsg, wParam, lParam, ACCEPT_BUTTON_INDEX) == 2)
      return 1;

   return CallWindowProc(OldAcceptButtonWndProc, hwnd, iMsg, wParam, lParam);
}


LRESULT CALLBACK CancelButtonWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   if (LookupKeystrokeBinding(iMsg, wParam, lParam, CANCEL_BUTTON_INDEX) == 2)
      return 1;

   return CallWindowProc(OldCancelButtonWndProc, hwnd, iMsg, wParam, lParam);
}




void MainWindow_OnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags)
{
   UINT UIStringbase = 0;

   if (item != 0)
      MenuHelp(WM_MENUSELECT, item, (LPARAM) hmenu, NULL,
               GLOBhInstance, hwndStatusBar, &UIStringbase);
   else
      SendMessage(hwndStatusBar, SB_SIMPLE, 0, 0);
}


void MainWindow_OnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
   SetFocus(hwndCallMenu);    // Is this right?
}


LRESULT CALLBACK MainWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   switch (iMsg) {
      HANDLE_MSG(hwnd, WM_DESTROY, MainWindow_OnDestroy);
      HANDLE_MSG(hwnd, WM_CREATE, MainWindow_OnCreate);
      HANDLE_MSG(hwnd, WM_SIZE, MainWindow_OnSize);
      HANDLE_MSG(hwnd, WM_COMMAND, MainWindow_OnCommand);
      HANDLE_MSG(hwnd, WM_MENUSELECT, MainWindow_OnMenuSelect);
      HANDLE_MSG(hwnd, WM_SETFOCUS, MainWindow_OnSetFocus);
   case WM_CLOSE:

      // We get here if the user presses alt-F4 and we haven't bound it to anything,
      // or if the user selects "exit" from the "file" menu.

      if (MenuKind != ui_start_select && gg77->iob88.do_abort_popup() != POPUP_ACCEPT)
         return 0;  // Queried user; user said no; so we don't shut down.

      // Close journal and session files; call general_final_exit,
      // which sends WM_USER+2 and shuts us down for real.

      general_final_exit(0);
      break;
   case WM_USER+2:
      // Real shutdown -- change to WM_CLOSE and go to default wndproc.
      iMsg = WM_CLOSE;
      break;
   }

   return DefWindowProc(hwnd, iMsg, wParam, lParam);
}




static void setup_level_menu(HWND hDlg)
{
   int lev;

   SetDlgItemText(hDlg, IDC_START_CAPTION, "Choose a level");

   SendDlgItemMessage(hDlg, IDC_START_LIST, LB_RESETCONTENT, 0, 0L);

   for (lev=(int)l_mainstream ; ; lev++) {
      Cstring this_string = getout_strings[lev];
      if (!this_string[0]) break;
      SendDlgItemMessage(hDlg, IDC_START_LIST, LB_ADDSTRING, 0, (LPARAM) this_string);
   }

   SendDlgItemMessage (hDlg, IDC_START_LIST, LB_SETCURSEL, 0, 0L);
}


static void SetTitle()
{
   UpdateStatusBar((Cstring) 0);
   SetWindowText(hwndMain, (LPSTR) szMainTitle);
}


void iofull::set_pick_string(Cstring string)
{
   if (string && *string) {
      UpdateStatusBar((Cstring) 0);
      SetWindowText(hwndMain, (LPSTR) string);
   }
   else {
      SetTitle();   // End of pick, reset to our main title.
   }
}

void iofull::set_window_title(char s[])
{
   lstrcpy(szMainTitle, "Sd ");
   lstrcat(szMainTitle, s);
   SetTitle();
}



static enum dialog_menu_type {
   dialog_session,
   dialog_level,
   dialog_none}
dialog_menu_type;

static bool request_deletion = false;
static Cstring session_error_msg;


// Process Startup dialog box messages.

static void Startup_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   int i;

   switch (id) {
   case IDC_START_LIST:

      // See if user has double-clicked in the call menu.
      // We don't try to respond to single clicks.  It's too much
      // work to distinguish them from selection changes due to
      // the cursor arrow keys.

      if (codeNotify == (UINT) LBN_DBLCLK)
         goto accept_listbox;

      break;
   case IDC_WRITE_LIST:
   case IDC_WRITE_FULL_LIST:
   case IDC_ABRIDGE:
      // User clicked on some call list option.  Enable and seed the file name.
      EnableWindow(GetDlgItem(hwnd, IDC_ABRIDGE_NAME), TRUE);
      GetWindowText(GetDlgItem(hwnd, IDC_ABRIDGE_NAME),
                    szDatabaseFilename, MAX_TEXT_LINE_LENGTH);
      if (!szDatabaseFilename[0])
         SetDlgItemText(hwnd, IDC_ABRIDGE_NAME, "abridge.txt");
      return;
   case IDC_NORMAL:
   case IDC_CANCEL_ABRIDGE:
      EnableWindow(GetDlgItem(hwnd, IDC_ABRIDGE_NAME), FALSE);
      return;
   case IDC_USERDEFINED:
      // User clicked on a special database file.  Enable and seed the file name.
      EnableWindow(GetDlgItem(hwnd, IDC_DATABASE_NAME), TRUE);
      GetWindowText(GetDlgItem(hwnd, IDC_DATABASE_NAME),
                    szDatabaseFilename, MAX_TEXT_LINE_LENGTH);
      if (!szDatabaseFilename[0])
         SetDlgItemText(hwnd, IDC_DATABASE_NAME, "database.txt");
      return;
   case IDC_DEFAULT:
      EnableWindow(GetDlgItem(hwnd, IDC_DATABASE_NAME), FALSE);
      return;
   case IDCANCEL:         /* User hit the "close window" thing in the upper right corner. */
   case IDC_START_CANCEL: /* User hit the "cancel" button. */
      EndDialog(hwnd, TRUE);
      session_index = 0;  // Prevent attempts to update session file.
      general_final_exit(0);
      return;
   case IDC_START_ACCEPT:

      /* User hit the "accept" button.  Read out all the information.
         But it's more complicated than that.  We sometimes do a two-stage
         presentation of this dialog, getting the session number and then
         the level.  So we may have to go back to the second stage.
         The variable "dialog_menu_type" tells what we were getting. */

   accept_listbox:
      i = SendDlgItemMessage(hwnd, IDC_START_LIST, LB_GETCURSEL, 0, 0L);

      if (dialog_menu_type == dialog_session) {
         /* The user has just responded to the session selection.
            Figure out what to do.  We may need to go back and get the
            level. */

         session_index = i;

         // If the user wants that session deleted, do that, and get out
         // immediately.  Setting the number to zero will cause it to
         // be deleted when the session file is updated during program
         // termination.

         if (IsDlgButtonChecked(hwnd, IDC_START_DELETE_SESSION_CHECKED)) {
            session_index = -session_index;
            request_deletion = true;
            goto getoutahere;
         }

         // If the user selected the button for canceling the abridgement
         // on the current session, do so.

         if (IsDlgButtonChecked(hwnd, IDC_CANCEL_ABRIDGE))
            glob_abridge_mode = abridge_mode_deleting_abridge;

         // Analyze the indicated session number.

         int session_info = process_session_info(&session_error_msg);

         if (session_info & 1) {
            // We are not using a session, either because the user selected
            // "no session", or because of some error in processing the
            // selected session.
            session_index = 0;
            sequence_number = -1;
         }

         if (session_info & 2)
            gg77->iob88.serious_error_print(session_error_msg);

         // If the level never got specified, either from a command line
         // argument or from the session file, put up the level selection
         // screen and go back for another round.

         if (calling_level == l_nonexistent_concept) {
            setup_level_menu(hwnd);
            dialog_menu_type = dialog_level;
            return;
         }
      }
      else if (dialog_menu_type == dialog_level) {
         // Either there was no session file, or there was a session
         // file but the user selected no session or a new session.
         // In the latter case, we went back and asked for the level.
         // So now we have both, and we can proceed.
         calling_level = (dance_level) i;
         strncat(outfile_string, filename_strings[calling_level], MAX_FILENAME_LENGTH);
      }

      // If a session was selected, and that session specified
      // an abridgement file, that file overrides what the buttons
      // and the abridgement file name edit box specify.

      if (glob_abridge_mode != abridge_mode_abridging) {
         if (IsDlgButtonChecked(hwnd, IDC_ABRIDGE))
            glob_abridge_mode = abridge_mode_abridging;
         else if (IsDlgButtonChecked(hwnd, IDC_WRITE_LIST))
            glob_abridge_mode = abridge_mode_writing_only;
         else if (IsDlgButtonChecked(hwnd, IDC_WRITE_FULL_LIST))
            glob_abridge_mode = abridge_mode_writing_full;
         else if (IsDlgButtonChecked(hwnd, IDC_NORMAL))
            glob_abridge_mode = abridge_mode_none;

         // If the user specified a call list file, get the name.

         if (glob_abridge_mode >= abridge_mode_abridging) {
            char szCallListFilename[MAX_TEXT_LINE_LENGTH];

            // This may have come from the command-line switches,
            // in which case we already have the file name.

            GetWindowText(GetDlgItem(hwnd, IDC_ABRIDGE_NAME),
                          szCallListFilename, MAX_TEXT_LINE_LENGTH);

            if (szCallListFilename[0])
               strncpy(abridge_filename, szCallListFilename, MAX_TEXT_LINE_LENGTH);
         }
      }
      else if (IsDlgButtonChecked(hwnd, IDC_ABRIDGE)) {
         // But if the session specified an abridgement file, and the user
         // also clicked the button for abridgement and gave a file name,
         // use that file name, overriding what was in the session line.

         char szCallListFilename[MAX_TEXT_LINE_LENGTH];

         GetWindowText(GetDlgItem(hwnd, IDC_ABRIDGE_NAME),
                       szCallListFilename, MAX_TEXT_LINE_LENGTH);

         if (szCallListFilename[0])
            strncpy(abridge_filename, szCallListFilename, MAX_TEXT_LINE_LENGTH);
      }

      // If user specified the output file during startup dialog, install that.
      // It overrides anything from the command line.

      GetWindowText(GetDlgItem(hwnd, IDC_OUTPUT_NAME),
                    szOutFilename, MAX_TEXT_LINE_LENGTH);

      if (szOutFilename[0])
         new_outfile_string = szOutFilename;

      // Handle user-specified database file.

      if (IsDlgButtonChecked(hwnd, IDC_USERDEFINED)) {
         GetWindowText(GetDlgItem(hwnd, IDC_DATABASE_NAME),
                       szDatabaseFilename, MAX_TEXT_LINE_LENGTH);
         database_filename = szDatabaseFilename;
      }

      ui_options.sequence_num_override =
         SendMessage(GetDlgItem(hwnd, IDC_SEQ_NUM_OVERRIDE_SPIN), UDM_GETPOS, 0, 0L);

   getoutahere:

      EndDialog(hwnd, TRUE);
   }
}


static BOOL Startup_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   // Set up the sequence number override.  Its text is the null string
   // unless a command line value was given.

   SetDlgItemText(hwnd, IDC_SEQ_NUM_OVERRIDE, "");

   HWND hb = GetDlgItem(hwnd, IDC_SEQ_NUM_OVERRIDE_SPIN);
   SendMessage(hb, UDM_SETRANGE, 0, (LPARAM) MAKELONG(32000, 0));

   if (ui_options.sequence_num_override > 0)
      SendMessage(hb, UDM_SETPOS, 0, (LPARAM) MAKELONG(ui_options.sequence_num_override, 0));

   // Select the default radio buttons.

   switch (glob_abridge_mode) {
   case abridge_mode_writing_only:
      CheckRadioButton(hwnd, IDC_NORMAL, IDC_ABRIDGE, IDC_WRITE_LIST);
      EnableWindow(GetDlgItem(hwnd, IDC_ABRIDGE_NAME), TRUE);
      if (abridge_filename[0]) SetDlgItemText(hwnd, IDC_ABRIDGE_NAME, abridge_filename);
      break;
   case abridge_mode_writing_full:
      CheckRadioButton(hwnd, IDC_NORMAL, IDC_ABRIDGE, IDC_WRITE_FULL_LIST);
      EnableWindow(GetDlgItem(hwnd, IDC_ABRIDGE_NAME), TRUE);
      if (abridge_filename[0]) SetDlgItemText(hwnd, IDC_ABRIDGE_NAME, abridge_filename);
      break;
   case abridge_mode_abridging:
      CheckRadioButton(hwnd, IDC_NORMAL, IDC_ABRIDGE, IDC_ABRIDGE);
      EnableWindow(GetDlgItem(hwnd, IDC_ABRIDGE_NAME), TRUE);
      if (abridge_filename[0]) SetDlgItemText(hwnd, IDC_ABRIDGE_NAME, abridge_filename);
      break;
   default:
      CheckRadioButton(hwnd, IDC_NORMAL, IDC_ABRIDGE, IDC_NORMAL);
      SetDlgItemText(hwnd, IDC_ABRIDGE_NAME, "");
      break;
   }

   CheckRadioButton(hwnd, IDC_DEFAULT, IDC_USERDEFINED, IDC_DEFAULT);

   // Seed the various file names with the null string.

   SetDlgItemText(hwnd, IDC_OUTPUT_NAME, "");
   SetDlgItemText(hwnd, IDC_DATABASE_NAME, "");

   // Put up the session list or the level list,
   // depending on whether a session file is in use.

   SendDlgItemMessage(hwnd, IDC_START_LIST, LB_RESETCONTENT, 0, 0L);

   // If user specified session number on the command line, we must
   // just be looking for a level.  So skip the session part.

   if (ui_options.force_session == -1000000 && !get_first_session_line()) {
      char line[MAX_FILENAME_LENGTH];

      SetDlgItemText(hwnd, IDC_START_CAPTION, "Choose a session");

      while (get_next_session_line(line))
         SendDlgItemMessage(hwnd, IDC_START_LIST, LB_ADDSTRING, 0, (LPARAM) line);
      dialog_menu_type = dialog_session;
      SendDlgItemMessage(hwnd, IDC_START_LIST, LB_SETCURSEL, 0, 0L);
   }
   else if (calling_level == l_nonexistent_concept) {
      setup_level_menu(hwnd);
      dialog_menu_type = dialog_level;
   }
   else {
      SetDlgItemText(hwnd, IDC_START_CAPTION, "");
      dialog_menu_type = dialog_none;
   }

   return TRUE;
}

static struct { int id; const char *message; } dialog_help_list[] = {
   // These two must be first, so that the level-vs-session stuff
   // will come out right.
   {IDC_START_LIST,
    "Double-click a level to choose it and start Sd."},
   {IDC_START_LIST,
    "Double-click a session to choose it and start Sd.\n"
    "Double-click \"(no session)\" if you don't want to run under any session at this time.\n"
    "Double-click \"(create a new session)\" if you want to add a new session to the list.\n"
    "You will be asked about the particulars for that new session."},
   {IDC_START_CANCEL,
    "This exits Sd immediately."},
   {IDC_START_ACCEPT,
    "This accepts whatever session is highlighted (and whatever other\n"
    "things you may have set), and starts Sd."},
   {IDC_START_DELETE_SESSION_CHECKED,
    "If you check this box and choose a session (by double-clicking it or\n"
    "clicking \"accept\"), that session will be PERMANENTLY DELETED\n"
    "instead of being used, and the program will exit immediately.\n"
    "You should then restart the program."},
   {IDC_OUTPUT_NAME,
    "You normally don't need this, since sessions already have output file names.\n"
    "If running without a session, you might want to enter something here.\n"
    "(If you don't, it will default to a name like \"sequence_C1.txt\".)\n"
    "If creating a new session, it will ask you for the file name, so you don't\n"
    "need to do anything here.  You can, in any case, use the \"change output file\"\n"
    "command to change the file name later."},
   {IDC_SEQ_NUM_OVERRIDE,
    "Sessions generally keep track of sequence (card) numbers, so you\n"
    "usually don't need this.  If you specify a number here and then choose\n"
    "a session, that session's numbering will be permanently changed."},
   {IDC_SEQ_NUM_OVERRIDE_SPIN,
    "Sessions generally keep track of sequence (card) numbers, so you\n"
    "usually don't need this.  If you specify a number here and then choose\n"
    "a session, that session's numbering will be permanently changed."},
   {IDC_DEFAULT,
    "Leave this checked, unless you want an alternative database, for experts only."},
   {IDC_USERDEFINED,
    "This selects an alternative calls database, for experts only."},
   {IDC_DATABASE_NAME,
    "Enter the name of the alternative calls database, for experts only."},
   {IDC_NORMAL,
    "Unless you are reading or writing an abridgement file (for preparing\n"
    "material for a group that knows only part of a dance level), leave this checked."},
   {IDC_WRITE_LIST,
    "If you want to make an abridgement list (for preparing material for a group\n"
    "that knows only part of a dance level), check this, set the file name if desired,\n"
    "choose \"no session\", and choose the level you want to abridge.\n"
    "The abrigement list will be written to that file, and the program\n"
    "will exit immediately.  As the group learns calls, delete them\n"
    "from the abridgement file with a text editor.  To write material for the group,\n"
    "check the \"use abridged list\" box below when you run the program."},
   {IDC_ABRIDGE,
    "After an abridgement list has been prepared (and the calls that the group knows\n"
    "have been deleted from it with a text editor), check this box, and set the file\n"
    "name if desired, to write material for the group.  This is most conveniently done\n"
    "when using a session.  If you do this once, and choose a session, that file name\n"
    "will be remembered along with the other information for the session.\n"
    "The abridgement file name for a session is shown in the list above right after the\n"
    "level, separated from it with a hyphen."},
   {IDC_CANCEL_ABRIDGE,
    "Use this when you want to continue using a session, but no longer want an abridgement\n"
    "file name associated with it.  Check the box, and choose the session.  The association\n"
    "of the abridgement file with that session will be permanently removed."},
   {IDC_ABRIDGE_NAME,
    "Set this to the desired abridgement file name if the default name \"abridge.txt\" is\n"
    "not suitable.  This would be true, for example, if you have two sessions associated\n"
    "with different abridgement files."},
   {-1, ""}
};



LRESULT WINAPI Startup_Dialog_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   LPHELPINFO lpHelpInfo;
   int i = 1;

   switch (message) {
      HANDLE_MSG(hwnd, WM_INITDIALOG, Startup_OnInitDialog);
      HANDLE_MSG(hwnd, WM_COMMAND, Startup_OnCommand);
   case WM_HELP:
      lpHelpInfo = (LPHELPINFO) lParam;

      // Search for a help message.  But first, if we are doing the
      // "level" menu instead of the "session" menu, use a different message.
      // We do this by starting the search at 1 under normal circumstances,
      // but starting it at 0 when we want to pick up the "level" message.

      if (dialog_menu_type == dialog_level) i = 0;

      for ( ; dialog_help_list[i].id >= 0 ; i++) {
         if (lpHelpInfo->iCtrlId == dialog_help_list[i].id) {
            MessageBox(hwnd, dialog_help_list[i].message, "Help", MB_OK);
            return TRUE;
         }
      }
      return FALSE;
   default:
      return FALSE;      // We do *NOT* call the system default handler.
   }
}



// Set the default font: 14 point bold Courier New.

print_default_info printer_default_info = {
   "Courier New",
   "Text Files (*.txt)\0*.txt\0" "All Files (*.*)\0*.*\0",
   14,
   true,
   false,
   IDD_PRINTING_DIALOG,
   IDC_FILENAME};


void iofull::set_utils_ptr(ui_utils *utils_ptr) { m_ui_utils_ptr = utils_ptr; }
ui_utils *iofull::get_utils_ptr() { return m_ui_utils_ptr; }

bool iofull::init_step(init_callback_state s, int n)
{
   WNDCLASSEX wndclass;

   switch (s) {

   case get_session_info:

      // Create and register the class for the main window.

      wndclass.cbSize = sizeof(wndclass);
      wndclass.style = CS_HREDRAW | CS_VREDRAW/* | CS_NOCLOSE*/;
      wndclass.lpfnWndProc = MainWndProc;
      wndclass.cbClsExtra = 0;
      wndclass.cbWndExtra = 0;
      wndclass.hInstance = GLOBhInstance;
      wndclass.hIcon = LoadIcon(GLOBhInstance, MAKEINTRESOURCE(IDI_ICON1));
      wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
      wndclass.hbrBackground  = (HBRUSH) (COLOR_BTNFACE+1);
      wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
      wndclass.lpszClassName = szMainWindowName;
      wndclass.hIconSm = wndclass.hIcon;
      RegisterClassEx(&wndclass);

      // Create and register the class for the transcript window.

      wndclass.cbSize = sizeof(wndclass);
      wndclass.style = 0;
      wndclass.lpfnWndProc = TranscriptAreaWndProc;
      wndclass.cbClsExtra = 0;
      wndclass.cbWndExtra = 0;
      wndclass.hInstance = GLOBhInstance;
      wndclass.hIcon = NULL;
      wndclass.hCursor = NULL;
      wndclass.hbrBackground =
         GetStockBrush(ui_options.reverse_video ?
                       BLACK_BRUSH :
                       (ui_options.no_intensify ? LTGRAY_BRUSH : WHITE_BRUSH));
      wndclass.lpszMenuName = NULL;
      wndclass.lpszClassName = szTranscriptWindowName;
      wndclass.hIconSm = wndclass.hIcon;
      RegisterClassEx(&wndclass);

      InitCommonControls();

      hwndMain = CreateWindow(
         szMainWindowName, "Sd",
         WS_OVERLAPPEDWINDOW,
         window_size_args[2],
         window_size_args[3],
         window_size_args[0],
         window_size_args[1],
         NULL, NULL, GLOBhInstance, NULL);

      if (!hwndMain)
         fatal_error_exit(1, "Can't create main window", 0);

      GLOBprinter = new printer(GLOBhInstance, hwndMain, printer_default_info);

      // If the user specified a specific session, do that session.
      // If we succeed at it, we won't put up the dialog box at all.

      if (ui_options.force_session != -1000000 &&
          ui_options.force_session != 0 &&
          !get_first_session_line()) {
         while (get_next_session_line((char *) 0));   // Need to scan the file anyway.
         session_index = ui_options.force_session;
         if (session_index < 0) {
            request_deletion = true;
         }
         else {
            int session_info = process_session_info(&session_error_msg);

            if (session_info & 1) {
               // We are not using a session, either because the user selected
               // "no session", or because of some error in processing the
               // selected session.
               session_index = 0;
               sequence_number = -1;
            }

            if (session_info & 2)
               serious_error_print(session_error_msg);
         }
      }
      else {
         session_index = 0;
         sequence_number = -1;
      }

      // Now put up the dialog box if we need a session or a level.
      // If the user requested an explicit session,
      // we don't do the dialog, since the user is presumably running
      // from command-line arguments, and doesn't need any of the
      // info that the dialog would give.

      if (ui_options.force_session == -1000000) {
         DialogBox(GLOBhInstance, MAKEINTRESOURCE(IDD_START_DIALOG),
                   hwndMain, (DLGPROC) Startup_Dialog_WndProc);
      }

      if (request_deletion) return true;
      break;

   case final_level_query:
      calling_level = l_mainstream;   // User really doesn't want to tell us the level.
      strncat(outfile_string, filename_strings[calling_level], MAX_FILENAME_LENGTH);
      break;

   case init_database1:
      // The level has been chosen.  We are about to open the database.
      // Put up the main window.

      ShowWindow(hwndMain, GLOBiCmdShow);
      UpdateWindow(hwndMain);

      UpdateStatusBar("Reading database");
      break;

   case init_database2:
      ShowWindow(hwndProgress, SW_SHOWNORMAL);
      UpdateStatusBar("Creating Menus");
      break;

   case calibrate_tick:
      SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELONG(0, n));
      SendMessage(hwndProgress, PBM_SETSTEP, 1, 0);
      break;

   case do_tick:
      SendMessage(hwndProgress, PBM_SETSTEP, n, 0);
      SendMessage(hwndProgress, PBM_STEPIT, 0, 0);
      break;

   case tick_end:
      break;

   case do_accelerator:
      ShowWindow(hwndProgress, SW_HIDE);
      UpdateStatusBar("Processing Accelerator Keys");
      break;
   }

   return false;
}



void iofull::final_initialize()
{
   ui_options.use_escapes_for_drawing_people = 2;

   // Install the pointy triangles.

   if (ui_options.no_graphics < 2)
      ui_options.direc = "?\020?\021????\036?\037?????";

   HANDLE hRes = LoadResource(GLOBhInstance,
                              FindResource(GLOBhInstance,
                                           MAKEINTRESOURCE(IDB_BITMAP1), RT_BITMAP));

   if (!hRes) fatal_error_exit(1, "Can't load resources", 0);

   // Map the bitmap file into memory.
   LPBITMAPINFO lpBitsTemp = (LPBITMAPINFO) LockResource(hRes);

   lpBi = (LPBITMAPINFO) GlobalAlloc(GMEM_FIXED,
                                     lpBitsTemp->bmiHeader.biSize +
                                     16*sizeof(RGBQUAD) +
                                     BMP_PERSON_SIZE*BMP_PERSON_SIZE*4*9);

   memcpy(lpBi, lpBitsTemp,
          lpBitsTemp->bmiHeader.biSize +
          16*sizeof(RGBQUAD) +
          BMP_PERSON_SIZE*BMP_PERSON_SIZE*4*9);

   lpBits = ((LPTSTR) lpBi) + lpBi->bmiHeader.biSize + 16*sizeof(RGBQUAD);

   HANDLE hPal = GlobalAlloc(GHND, sizeof(LOGPALETTE) + (16*sizeof(PALETTEENTRY)));
   LPLOGPALETTE lpPal = (LPLOGPALETTE) GlobalLock(hPal);
   lpPal->palVersion = 0x300;
   lpPal->palNumEntries = 16;
   for (int i=0 ; i<16 ; i++) {
      lpPal->palPalEntry[i].peRed   = lpBi->bmiColors[i].rgbRed;
      lpPal->palPalEntry[i].peGreen = lpBi->bmiColors[i].rgbGreen;
      lpPal->palPalEntry[i].peBlue  = lpBi->bmiColors[i].rgbBlue;
   }

   hPalette = CreatePalette(lpPal);
   GlobalUnlock(hPal);
   GlobalFree(hPal);

   // Now that we know the coloring options,
   // fudge the color table in the mapped DIB.

   // The standard 4-plane color scheme is:
   //   0  black
   //   1  dark red
   //   2  dark green
   //   3  dark yellow
   //   4  dark blue
   //   5  dark magenta
   //   6  dark cyan
   //   7  light gray
   //   8  dark gray
   //   9  bright red
   //   10 bright green
   //   11 bright yellow
   //   12 bright blue
   //   13 bright magenta
   //   14 bright cyan
   //   15 white

   RGBQUAD glyphtext_fg, glyphtext_bg;

   if (ui_options.reverse_video) {
      if (ui_options.no_intensify) {
         glyphtext_fg = lpBitsTemp->bmiColors[7];
         plaintext_fg = RGB(192, 192, 192);
         glyphtext_bg = lpBitsTemp->bmiColors[0];
         plaintext_bg = RGB(0, 0, 0);
      }
      else {
         glyphtext_fg = lpBitsTemp->bmiColors[15];
         plaintext_fg = RGB(255, 255, 255);
         glyphtext_bg = lpBitsTemp->bmiColors[0];
         plaintext_bg = RGB(0, 0, 0);
      }
   }
   else {
      if (ui_options.no_intensify) {
         glyphtext_fg = lpBitsTemp->bmiColors[0];
         plaintext_fg = RGB(0, 0, 0);
         glyphtext_bg = lpBitsTemp->bmiColors[7];
         plaintext_bg = RGB(192, 192, 192);;
      }
      else {
         glyphtext_fg = lpBitsTemp->bmiColors[0];
         plaintext_fg = RGB(0, 0, 0);
         glyphtext_bg = lpBitsTemp->bmiColors[15];
         plaintext_bg = RGB(255, 255, 255);
      }
   }

   if (ui_options.color_scheme == no_color) {
      icon_color_translate[1] = glyphtext_fg;
      icon_color_translate[2] = glyphtext_fg;
      icon_color_translate[3] = glyphtext_fg;
      icon_color_translate[4] = glyphtext_fg;
      icon_color_translate[5] = glyphtext_fg;
      icon_color_translate[6] = glyphtext_fg;
      icon_color_translate[7] = glyphtext_fg;
   }
   else {
      icon_color_translate[1] = lpBitsTemp->bmiColors[3];   // dark yellow
      icon_color_translate[2] = lpBitsTemp->bmiColors[9];   // red
      icon_color_translate[3] = lpBitsTemp->bmiColors[10];  // green
      icon_color_translate[4] = lpBitsTemp->bmiColors[11];  // yellow
      icon_color_translate[5] = lpBitsTemp->bmiColors[12];  // blue
      icon_color_translate[6] = lpBitsTemp->bmiColors[13];  // magenta
      icon_color_translate[7] = lpBitsTemp->bmiColors[14];  // cyan
   }

   // Now fill in the palette through which the pixels in
   // the DIB will be translated.
   // The people are "colored" in the DIB file as (colors in parentheses
   //     are what the DIB would look like under a normal color map;
   //     those colors are irrelevant for this program):
   //   1G - 1  (dark red)
   //   2G - 2  (dark green)
   //   3G - 3  (dark yellow)
   //   4G - 4  (dark blue)
   //   1B - 9  (bright red)
   //   2B - 10 (bright green)
   //   3B - 11 (bright yellow)
   //   4B - 12 (bright blue)
   //   Also, the text showing the person number inside
   //   each glyph is 15 (white) on 0 (black).

   lpBi->bmiColors[1]  = icon_color_translate[color_index_list[1]];
   lpBi->bmiColors[2]  = icon_color_translate[color_index_list[3]];
   lpBi->bmiColors[3]  = icon_color_translate[color_index_list[5]];
   lpBi->bmiColors[4]  = icon_color_translate[color_index_list[7]];
   lpBi->bmiColors[9]  = icon_color_translate[color_index_list[0]];
   lpBi->bmiColors[10] = icon_color_translate[color_index_list[2]];
   lpBi->bmiColors[11] = icon_color_translate[color_index_list[4]];
   lpBi->bmiColors[12] = icon_color_translate[color_index_list[6]];

   lpBi->bmiColors[0]  = glyphtext_bg;
   lpBi->bmiColors[15] = glyphtext_fg;

   SetTitle();

   ShowWindow(hwndCallMenu, SW_SHOWNORMAL);
   ShowWindow(hwndTextInputArea, SW_SHOWNORMAL);
   ShowWindow(hwndAcceptButton, SW_SHOWNORMAL);

   UpdateWindow(hwndMain);

   // Initialize the display window linked list.

   DisplayRoot = new DisplayType;
   DisplayRoot->Line[0] = -1;
   DisplayRoot->Next = NULL;
   DisplayRoot->Prev = NULL;
   CurDisplay = DisplayRoot;
   nTotalImageHeight = 0;
}



// Process Windows Messages.
void EnterMessageLoop()
{
   MSG Msg;

   gg77->matcher_p->m_active_result.valid = false;
   gg77->matcher_p->erase_matcher_input();
   WaitingForCommand = true;

   while (GetMessage(&Msg, NULL, 0, 0) && WaitingForCommand) {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
   }

   // The message loop has been broken.  Either we got a message
   // that requires action (user clicked on a call, as opposed to
   // WM_PAINT), or the Windows message mechanism recognizes that
   // the program is closing.  The former case is indicated by
   // the message handler turning off WaitingForCommand.  Final
   // exit is indicated by GetMessage returning false while
   // WaitingForCommand remains true.

   if (WaitingForCommand) {
      // User closed the window.
      delete GLOBprinter;
      general_final_exit(Msg.wParam);
   }
}


void iofull::display_help() {}



const char *iofull::version_string ()
{
   return UI_VERSION_STRING "win";
}


void iofull::process_command_line(int *argcp, char ***argvp)
{
   int argno = 1;
   char **argv = *argvp;

   while (argno < (*argcp)) {
      int i;

      if (strcmp(argv[argno], "-no_line_delete") == 0)
         {}
      else if (strcmp(argv[argno], "-no_cursor") == 0)
         {}
      else if (strcmp(argv[argno], "-no_console") == 0)
         {}
      else if (strcmp(argv[argno], "-alternate_glyphs_1") == 0)
         {}
      else if (strcmp(argv[argno], "-lines") == 0 && argno+1 < (*argcp)) {
         goto remove_two;
      }
      else if (strcmp(argv[argno], "-journal") == 0 && argno+1 < (*argcp)) {
         goto remove_two;
      }
      else if (strcmp(argv[argno], "-maximize") == 0) {
         GLOBiCmdShow = SW_SHOWMAXIMIZED;
      }
      else if (strcmp(argv[argno], "-window_size") == 0 && argno+1 < (*argcp)) {
         int nn = sscanf(argv[argno+1], "%dx%dx%dx%d",
                         &window_size_args[0],
                         &window_size_args[1],
                         &window_size_args[2],
                         &window_size_args[3]);

         // We allow the user to give two numbers (size only) or 4 numbers (size and position).
         if (nn != 2 && nn != 4) {
            fatal_error_exit(1, "Bad size argument", argv[argno+1]);
         }

         goto remove_two;
      }
      else {
         argno++;
         continue;
      }

      (*argcp)--;      /* Remove this argument from the list. */
      for (i=argno+1; i<=(*argcp); i++) argv[i-1] = argv[i];
      continue;

      remove_two:

      (*argcp) -= 2;      /* Remove two arguments from the list. */
      for (i=argno+1; i<=(*argcp); i++) argv[i-1] = argv[i+1];
      continue;
   }
}



static void scan_menu(Cstring name, HDC hDC, int *nLongest_p, uint32_t itemdata)
{
   SIZE Size;

   GetTextExtentPoint(hDC, name, strlen(name), &Size);
   if ((Size.cx > *nLongest_p) && (Size.cx > CallsClientRect.right)) {
      SendMessage(hwndCallMenu, LB_SETHORIZONTALEXTENT, Size.cx, 0L);
      *nLongest_p = Size.cx;
   }

   int nMenuIndex = SendMessage(hwndCallMenu, LB_ADDSTRING, 0, (LPARAM) name);
   SendMessage(hwndCallMenu, LB_SETITEMDATA, nMenuIndex, (LPARAM) itemdata);
}



void ShowListBox(int nWhichOne)
{
   if (nWhichOne != nLastOne) {
      HDC hDC;

      nLastOne = nWhichOne;
      menu_moved = false;

      SendMessage(hwndCallMenu, LB_RESETCONTENT, 0, 0L);
      hDC = GetDC(hwndCallMenu);
      SendMessage(hwndCallMenu, WM_SETREDRAW, FALSE, 0L);

      int nLongest = 0;

      if (nLastOne == matcher_class::e_match_number) {
         UpdateStatusBar("<number>");

         for (int iu=0 ; iu<NUM_CARDINALS; iu++)
            scan_menu(cardinals[iu], hDC, &nLongest, MAKELONG(iu, 0));
      }
      else if (nLastOne == matcher_class::e_match_circcer) {
         UpdateStatusBar("<circulate replacement>");

         for (unsigned int iu=0 ; iu<number_of_circcers ; iu++)
            scan_menu(get_call_menu_name(circcer_calls[iu].the_circcer), hDC, &nLongest, MAKELONG(iu, 0));
      }
      else if (nLastOne >= matcher_class::e_match_taggers &&
               nLastOne < matcher_class::e_match_taggers+NUM_TAGGER_CLASSES) {
         int tagclass = nLastOne - matcher_class::e_match_taggers;

         UpdateStatusBar("<tagging call>");

         for (unsigned int iu=0 ; iu<number_of_taggers[tagclass] ; iu++)
            scan_menu(get_call_menu_name(tagger_calls[tagclass][iu]), hDC, &nLongest, MAKELONG(iu, 0));
      }
      else if (nLastOne == matcher_class::e_match_startup_commands) {
         UpdateStatusBar("<startup>");

         for (int i=0 ; i<num_startup_commands ; i++)
            scan_menu(startup_commands[i],
                      hDC, &nLongest, MAKELONG(i, (int) ui_start_select));
      }
      else if (nLastOne == matcher_class::e_match_resolve_commands) {
         for (int i=0 ; i<number_of_resolve_commands ; i++)
            scan_menu(resolve_command_strings[i],
                      hDC, &nLongest, MAKELONG(i, (int) ui_resolve_select));
      }
      else if (nLastOne == matcher_class::e_match_directions) {
         UpdateStatusBar("<direction>");

         for (int i=0 ; i<last_direction_kind ; i++)
            scan_menu(direction_menu_list[i+1],
                      hDC, &nLongest, MAKELONG(i, (int) ui_special_concept));
      }
      else if (nLastOne == matcher_class::e_match_selectors) {
         UpdateStatusBar("<selector>");

         // Menu is shorter than it appears, because we are skipping first item.
         for (int i=0 ; i<selector_INVISIBLE_START-1 ; i++)
            scan_menu(selector_menu_list[i],
                      hDC, &nLongest, MAKELONG(i, (int) ui_special_concept));
      }
      else {
         int i;

         UpdateStatusBar(menu_names[nLastOne]);

         for (i=0; i<number_of_calls[nLastOne]; i++)
            scan_menu(get_call_menu_name(main_call_lists[nLastOne][i]),
                      hDC, &nLongest, MAKELONG(i, (int) ui_call_select));

         short int *item;
         int menu_length;

         index_list *list_to_use = allowing_all_concepts ?
            &gg77->matcher_p->m_concept_list :
            &gg77->matcher_p->m_level_concept_list;
         item = list_to_use->the_list;
         menu_length = list_to_use->the_list_size;

         for (i=0 ; i<menu_length ; i++)
            scan_menu(get_concept_menu_name(access_concept_descriptor_table(item[i])),
                      hDC, &nLongest, MAKELONG(item[i], ui_concept_select));

         for (i=0 ;  ; i++) {
            Cstring name = command_menu[i].command_name;
            if (!name) break;
            scan_menu(name, hDC, &nLongest, MAKELONG(i, ui_command_select));
         }
      }

      SendMessage(hwndCallMenu, WM_SETREDRAW, TRUE, 0L);
      InvalidateRect(hwndCallMenu, NULL, TRUE);
      ReleaseDC(hwndCallMenu, hDC);
      SendMessage(hwndCallMenu, LB_SETCURSEL, 0, (LPARAM) 0);
   }

   ButtonFocusIndex = 0;
   SetFocus(hwndTextInputArea);
}


void iofull::prepare_for_listing()
{
}

void iofull::create_menu(call_list_kind cl) {}


uims_reply_thing iofull::get_startup_command()
{
   matcher_class &matcher = *gg77->matcher_p;
   nLastOne = ui_undefined;
   MenuKind = ui_start_select;
   ShowListBox(matcher_class::e_match_startup_commands);

   EnterMessageLoop();

   int index = matcher.m_final_result.match.index;

   if (index < 0)
      // Special encoding from a function key.
      return uims_reply_thing(matcher.m_final_result.match.kind, -1-index);
   else if (matcher.m_final_result.match.kind == ui_command_select) {
      // Translate the command.
   return uims_reply_thing(matcher.m_final_result.match.kind, (int) command_command_values[index]);
   }
   else if (matcher.m_final_result.match.kind == ui_start_select) {
      // Translate the command.
      return uims_reply_thing(matcher.m_final_result.match.kind, (int) startup_command_values[index]);
   }

   return uims_reply_thing(matcher.m_final_result.match.kind, index);
}


// This returns ui_user_cancel if it fails, e.g. the user waves the mouse away.
uims_reply_thing iofull::get_call_command()
{
   matcher_class &matcher = *gg77->matcher_p;

 startover:
   if (allowing_modifications)
      parse_state.call_list_to_use = call_list_any;

   SetTitle();
   nLastOne = ui_undefined;    /* Make sure we get a new menu,
                                  in case concept levels were toggled. */
   MenuKind = ui_call_select;
   ShowListBox(parse_state.call_list_to_use);
   EnterMessageLoop();

   int index = matcher.m_final_result.match.index;

   if (index < 0) {
      // Special encoding from a function key.
      return uims_reply_thing(matcher.m_final_result.match.kind, -1-index);
   }
   else if (matcher.m_final_result.match.kind == ui_command_select) {
      // Translate the command.
      return uims_reply_thing(matcher.m_final_result.match.kind, (int) command_command_values[index]);
   }
   else if (matcher.m_final_result.match.kind == ui_special_concept) {
      return uims_reply_thing(matcher.m_final_result.match.kind, index);
   }
   else {
      // Reject off-level concept accelerator key presses.
      if (!allowing_all_concepts && matcher.m_final_result.match.kind == ui_concept_select &&
          get_concept_level(matcher.m_final_result.match.concept_ptr) > calling_level)
         goto startover;

      call_conc_option_state save_stuff = matcher.m_final_result.match.call_conc_options;
      there_is_a_call = false;
      uims_reply_kind my_reply = matcher.m_final_result.match.kind;
      bool retval = deposit_call_tree(&matcher.m_final_result.match, (parse_block *) 0, 2);
      matcher.m_final_result.match.call_conc_options = save_stuff;
      if (there_is_a_call) {
         parse_state.topcallflags1 = the_topcallflags;
         my_reply = ui_call_select;
      }

      return uims_reply_thing(retval ? ui_user_cancel : my_reply, index);
   }
}


void iofull::dispose_of_abbreviation(const char *linebuff)
{
   SendMessage(hwndTextInputArea, WM_SETTEXT, 0, (LPARAM)"");  // Erase the edit box.
   WaitingForCommand = false;
}


uims_reply_thing iofull::get_resolve_command()
{
   matcher_class &matcher = *gg77->matcher_p;
   UpdateStatusBar(szResolveWndTitle);

   nLastOne = ui_undefined;
   MenuKind = ui_resolve_select;
   ShowListBox(matcher_class::e_match_resolve_commands);
   EnterMessageLoop();

   if (matcher.m_final_result.match.index < 0)
      // Special encoding from a function key.
      return uims_reply_thing(matcher.m_final_result.match.kind, -1-matcher.m_final_result.match.index);
   else
      return uims_reply_thing(matcher.m_final_result.match.kind, (int) resolve_command_values[matcher.m_final_result.match.index]);
}



int iofull::yesnoconfirm(Cstring title, Cstring line1, Cstring line2, bool excl, bool info)
{
   char finalline[200];

   if (line1 && line1[0]) {
      strcpy(finalline, line1);
      strcat(finalline, "\n");
      strcat(finalline, line2);
   }
   else {
      strcpy(finalline, line2);
   }

   uint32_t flags = MB_YESNO | MB_DEFBUTTON2;
   if (excl) flags |= MB_ICONEXCLAMATION;
   if (info) flags |= MB_ICONINFORMATION;

   if (MessageBox(hwndMain, finalline, title, flags) == IDYES)
      return POPUP_ACCEPT;
   else
      return POPUP_DECLINE;
}

int iofull::do_abort_popup()
{
   return yesnoconfirm("Confirmation", (char *) 0,
                       "Do you really want to abort this sequence?", true, false);
}


// This returns true if it got a real result, false if user cancelled.
static bool do_popup(int nWhichOne)
{
   uims_reply_kind SavedMenuKind = MenuKind;
   nLastOne = ui_undefined;
   MenuKind = ui_call_select;
   InPopup = true;
   ButtonFocusHigh = 3;
   ButtonFocusIndex = 0;
   PositionAcceptButtons();
   ShowWindow(hwndCancelButton, SW_SHOWNORMAL);
   ShowListBox(nWhichOne);
   EnterMessageLoop();
   InPopup = false;
   ButtonFocusHigh = 2;
   ButtonFocusIndex = 0;
   PositionAcceptButtons();
   ShowWindow(hwndCancelButton, SW_HIDE);
   MenuKind = SavedMenuKind;
   // A value of -1 means that the user hit the "cancel" button.
   return (gg77->matcher_p->m_final_result.match.index >= 0);
}


selector_kind iofull::do_selector_popup(matcher_class &matcher)
{
   match_result saved_match = matcher.m_final_result;
   // We add 1 to the menu position to get the actual selector enum; the enum effectively starts at 1.
   // Item zero in the enum is selector_uninitialized, which we return if the user cancelled.
   selector_kind retval = do_popup((int) matcher_class::e_match_selectors) ?
      (selector_kind) (matcher.m_final_result.match.index+1) : selector_uninitialized;
   matcher.m_final_result = saved_match;
   return retval;
}


direction_kind iofull::do_direction_popup(matcher_class &matcher)
{
   match_result saved_match = matcher.m_final_result;
   // We add 1 to the menu position to get the actual direction enum; the enum effectively starts at 1.
   // Item zero in the enum is direction_uninitialized, which we return if the user cancelled.
   direction_kind retval = do_popup((int) matcher_class::e_match_directions) ?
      (direction_kind) (matcher.m_final_result.match.index+1) : direction_uninitialized;
   matcher.m_final_result = saved_match;
   return retval;
}



int iofull::do_circcer_popup()
{
   matcher_class &matcher = *gg77->matcher_p;
   uint32_t retval = 0;

   if (interactivity == interactivity_verify) {
      retval = verify_options.circcer;
      if (retval == 0) retval = 1;
   }
   else if (!matcher.m_final_result.valid || (matcher.m_final_result.match.call_conc_options.circcer == 0)) {
      match_result saved_match = matcher.m_final_result;
      if (do_popup((int) matcher_class::e_match_circcer))
         retval = matcher.m_final_result.match.call_conc_options.circcer;
      matcher.m_final_result = saved_match;
   }
   else {
      retval = matcher.m_final_result.match.call_conc_options.circcer;
      matcher.m_final_result.match.call_conc_options.circcer = 0;
   }

   return retval;
}



int iofull::do_tagger_popup(int tagger_class)
{
   matcher_class &matcher = *gg77->matcher_p;
   match_result saved_match = matcher.m_final_result;
   saved_match.match.call_conc_options.tagger = 0;

   if (do_popup(((int) matcher_class::e_match_taggers) + tagger_class))
      saved_match.match.call_conc_options.tagger = matcher.m_final_result.match.call_conc_options.tagger;
   matcher.m_final_result = saved_match;

   int retval = matcher.m_final_result.match.call_conc_options.tagger;
   matcher.m_final_result.match.call_conc_options.tagger = 0;
   return retval;
}


uint32_t iofull::get_one_number(matcher_class &matcher)
{
   match_result saved_match = matcher.m_final_result;
   // Return excessively high value if user cancelled; client will notice.
   uint32_t retval = do_popup((int) matcher_class::e_match_number) ? matcher.m_final_result.match.index : NUM_CARDINALS+99;
   matcher.m_final_result = saved_match;
   return retval;
}


void iofull::add_new_line(const char the_line[], uint32_t drawing_picture)
{
   erase_questionable_stuff();
   lstrcpyn(CurDisplay->Line, the_line, DISPLAY_LINE_LENGTH-1);
   CurDisplay->Line[DISPLAY_LINE_LENGTH-1] = 0;
   CurDisplay->in_picture = drawing_picture;

   if ((CurDisplay->in_picture & 1) && ui_options.no_graphics == 0) {
      if ((CurDisplay->in_picture & 2)) {
         CurDisplay->Height = BMP_PERSON_SIZE+BMP_PERSON_SPACE;
         CurDisplay->DeltaToNext = (BMP_PERSON_SIZE+BMP_PERSON_SPACE)/2;
      }
      else {
         if (!CurDisplay->Line[0])
            CurDisplay->Height = 0;
         else
            CurDisplay->Height = BMP_PERSON_SIZE+BMP_PERSON_SPACE;

         CurDisplay->DeltaToNext = CurDisplay->Height;
      }
   }
   else {
      CurDisplay->Height = TranscriptTextHeight;
      CurDisplay->DeltaToNext = TranscriptTextHeight;
   }

   if (!CurDisplay->Next) {
      CurDisplay->Next = new DisplayType;
      CurDisplay->Next->Prev = CurDisplay;
      CurDisplay = CurDisplay->Next;
      CurDisplay->Next = NULL;
   }
   else
      CurDisplay = CurDisplay->Next;

   CurDisplay->Line[0] = -1;

   Update_text_display();
}


// We don't do anything here.
void iofull::no_erase_before_n(int n)
{}


void iofull::reduce_line_count(int n)
{
   CurDisplay = DisplayRoot;
   while (CurDisplay->Line[0] != -1 && n--) {
      CurDisplay = CurDisplay->Next;
   }

   CurDisplay->Line[0] = -1;

   Update_text_display();
}


void iofull::update_resolve_menu(command_kind goal, int cur, int max,
                                 resolver_display_state state)
{
   create_resolve_menu_title(goal, cur, max, state, szResolveWndTitle);
   UpdateStatusBar(szResolveWndTitle);

   // Put it in the transcript area also, where it's easy to see.

   get_utils_ptr()->writestuff(szResolveWndTitle);
   get_utils_ptr()->newline();
}


bool iofull::choose_font()
{
   GLOBprinter->choose_font();
   return true;
}

bool iofull::print_this()
{
   char full_outfile_name[MAX_FILENAME_LENGTH];
   strncpy(full_outfile_name, outfile_prefix, MAX_FILENAME_LENGTH);
   strncat(full_outfile_name, outfile_string, MAX_FILENAME_LENGTH);
   GLOBprinter->print_this(full_outfile_name, szMainTitle, false);
   return true;
}

bool iofull::print_any()
{
   GLOBprinter->print_any(szMainTitle, false);
   return true;
}


void iofull::bad_argument(Cstring s1, Cstring s2, Cstring s3)
{
   // Argument s3 isn't important.  It only arises when the level can't
   // be parsed, and it consists of a list of all the available levels.
   // In Sd, they were all on the menu.

   fatal_error_exit(1, s1, s2);
}


void iofull::fatal_error_exit(int code, Cstring s1, Cstring s2)
{
   if (s2 && s2[0]) {
      char msg[200];
      wsprintf(msg, "%s: %s", s1, s2);
      s1 = msg;   // Yeah, we can do that.  Yeah, it's sleazy.
   }

   serious_error_print(s1);
   session_index = 0;  // Prevent attempts to update session file.
   general_final_exit(code);
}


void iofull::serious_error_print(Cstring s1)
{
   MessageBox(hwndMain, s1, "Error", MB_OK | MB_ICONEXCLAMATION);
}


void iofull::terminate(int code)
{
   if (hwndMain) {
      // Check whether we should write out the transcript file.
      if (code == 0 && wrote_a_sequence) {
         if (yesnoconfirm("Confirmation", (char *) 0,
                          "Do you want to print the file?",
                          false, true) == POPUP_ACCEPT)
            print_this();
      }

      SendMessage(hwndMain, WM_USER+2, 0, 0L);
   }

   GlobalFree(lpBi);
   ExitProcess(code);
}
