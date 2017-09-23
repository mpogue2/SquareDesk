// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

// SD -- square dance caller's helper.
//
//    Copyright (C) 1994-2012  William B. Ackerman.
//

/*
 * sdui-tty.c - SD TTY User Interface
 * Originally for Macintosh.  Unix version by gildea.
 * Time-stamp: <96/05/22 17:17:53 wba>
 * Copyright (c) 1990-1994 Stephen Gildea, William B. Ackerman, and
 *   Alan Snyder
 *
 * Permission to use, copy, modify, and distribute this software for
 * any purpose is hereby granted without fee, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * The authors make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 * By Alan Snyder, December 1992.
 *
 * This version of the Sd UI does completing reads instead of menus.
 * Type '?' to see possibilities.
 *    At the "More" prompt, type SPC to see the next page, DEL to stop.
 * Type SPC to complete the current word.
 * Type TAB to complete as much as possible.
 * Type Control-U to clear the line.
 *
 * For use with version 38 of the Sd program.
 *
 * The version of this file is as shown immediately below.  This string
 * gets displayed at program startup, as the "ui" part of the complete
 * version.
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

#define UI_VERSION_STRING "1.13"
#define UI_TIME_STAMP "wba@alum.mit.edu  5 Jul 2005 $"

/* This file defines all the functions in class iofull
   except for

   iofull::display_help
   iofull::help_manual
   iofull::help_faq
   iofull::final_initialize

and the following other variables:

   screen_height
   no_cursor
   no_console
   no_line_delete
*/


// For "sprintf" and some IO stuff (fflush, printf, stdout) that we use
// during the "database tick" printing before the actual IO package is started.
// During normal operation, we don't do any IO at all in this file.
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

extern void exit(int code);

#include "sdui.h"

// See comments in sdmain.cpp regarding this string.
static const char id[] = "@(#)$He" "ader: Sd: sdui-tty.c " UI_VERSION_STRING "  " UI_TIME_STAMP;


#define DEL 0x7F

// The total version string looks something like
// "1.4:db1.5:ui0.6tty"
// We return the "0.6tty" part.

static char journal_name[MAX_TEXT_LINE_LENGTH];
static FILE *journal_file = (FILE *) 0;
int sdtty_screen_height = 0;  // The "lines" option may set this to something.
                              // Otherwise, any subsystem that sees the value zero
                              // will initialize it to whatever value it thinks best,
                              // perhaps by interrogating the OS.
bool sdtty_no_console = false;
bool sdtty_no_line_delete = false;

const char *iofull::version_string()
{
   return UI_VERSION_STRING "tty";
}

static resolver_display_state resolver_happiness = resolver_display_failed;


int main(int argc, char *argv[])
{
   // In Sdtty, the defaults are reverse video (white-on-black) and pastel colors.

   ui_options.reverse_video = true;
   ui_options.pastel_color = true;

   // Initialize all the callbacks that sdlib will need.
   iofull ggg;

   return sdmain(argc, argv, ggg);
}


// This array is the same as m_user_input in the matcher object, but has the original
// capitalization as typed by the user.  But m_user_input is converted to all
// lower case for ease of searching.
static char user_input[INPUT_TEXTLINE_SIZE+1];
static const char *user_input_prompt;
static const char *function_key_expansion;

void refresh_input()
{
   gg77->matcher_p->erase_matcher_input();
   user_input[0] = '\0';
   function_key_expansion = (const char *) 0;
   clear_line();
   put_line(user_input_prompt);
}


/* This tells how many of the last lines currently on the screen contain
   the text that the main program thinks comprise the transcript.  That is,
   if we count this far up from the bottom of the screen (well, from the
   cursor, to be precise), we will get the line that the main program thinks
   is the first line of the transcript.  This variable will often be greater
   than the main program's belief of the transcript length, because we are
   fooling around with input lines (prompts, error messages, etc.) at the
   bottom of the screen.

   The main program typically updates the transcript in a "clean" way by
   calling "reduce_line_count" to erase some number of lines at the
   end of the transcript, followed by a rewrite of new material.  We compare
   what "reduce_line_count" tells us against the value of this variable
   to find out how many lines to actually erase.  That way, we erase the input
   lines and produce a truly clean transcript, on devices (like VT-100's)
   capable of doing so.  If the display device can't erase lines, the user
   will see the last few lines of the transcript overlapping from one
   command to the next.  This is the appropriate behavior for a printing
   terminal or computer emulation of same. */

static int current_text_line;

static char *call_menu_prompts[call_list_extent];

// For the "alternate_glyphs_1" command-line switch.
static char alt1_names1[] = "        ";
static char alt1_names2[] = "1P2R3O4C";



/*
 * The main program calls this before doing anything else, so we can
 * supply additional command line arguments.
 * Note: If we are writing a call list, the program will
 * exit before doing anything else with the user interface, but this
 * must be made anyway.
 */

void iofull::process_command_line(int *argcp, char ***argvp)
{
   int argno = 1;
   char **argv = *argvp;
   journal_name[0] = '\0';

   while (argno < (*argcp)) {
      int i;

      if (strcmp(argv[argno], "-no_line_delete") == 0)
         sdtty_no_line_delete = true;
      else if (strcmp(argv[argno], "-no_cursor") == 0)
         ;     // This is obsolete.
      else if (strcmp(argv[argno], "-no_console") == 0)
         sdtty_no_console = true;
      else if (strcmp(argv[argno], "-alternate_glyphs_1") == 0) {
         ui_options.pn1 = alt1_names1;
         ui_options.pn2 = alt1_names2;
      }
      else if (strcmp(argv[argno], "-lines") == 0 && argno+1 < (*argcp)) {
         sdtty_screen_height = atoi(argv[argno+1]);
         goto remove_two;
      }
      else if (strcmp(argv[argno], "-journal") == 0 && argno+1 < (*argcp)) {
         strcpy(journal_name, argv[argno+1]);
         journal_file = fopen(argv[argno+1], "w");

         if (!journal_file) {
            printf("Can't open journal file\n");
            perror(argv[argno+1]);
            general_final_exit(1);
         }

         goto remove_two;
      }
      else if (strcmp(argv[argno], "-maximize") == 0)
         {}
      else if (strcmp(argv[argno], "-window_size") == 0 && argno+1 < (*argcp))
         goto remove_two;
      else if (strcmp(argv[argno], "-session") == 0 && argno+1 < (*argcp)) {
         argno += 2;
         continue;
      }
      else if (strcmp(argv[argno], "-resolve_test") == 0 && argno+1 < (*argcp)) {
         argno += 2;
         continue;
      }
      else {
         argno++;
         continue;
      }

      (*argcp)--;      // Remove this argument from the list.
      for (i=argno+1; i<=(*argcp); i++) argv[i-1] = argv[i];
      continue;

      remove_two:

      (*argcp) -= 2;   // Remove two arguments from the list.
      for (i=argno+1; i<=(*argcp); i++) argv[i-1] = argv[i+1];
      continue;
   }
}


static bool really_open_session()
{
   Cstring session_error_msg;

   // Process the session list.  If user has specified a
   // session number from the command line, we don't print
   // stuff or query anyone, but we still go through the
   // file.

   if (get_first_session_line()) goto no_session;

   // If user gave a session specification
   // in the command line, we don't query about the session.

   if (ui_options.force_session == -1000000) {
      char line[MAX_FILENAME_LENGTH];

      put_line("Do you want to use one of the following sessions?\n\n");

      while (get_next_session_line(line)) {
         put_line(line);
         put_line("\n");
      }

      put_line("Enter the number of the desired session\n");
      put_line("   (or a negative number to delete that session):  ");

      get_string(line, MAX_FILENAME_LENGTH);
      if (!line[0] || line[0] == '\r' || line[0] == '\n')
         goto no_session;

      if (!sscanf(line, "%d", &session_index)) {
         session_index = 0;         // User typed garbage -- exit the program immediately.
         return true;
      }
   }
   else {
      while (get_next_session_line((char *) 0));   // Need to scan the file anyway.
      session_index = ui_options.force_session;
   }

   if (session_index < 0)
      return true;    // Exit the program immediately.  Deletion will take place.

   {
      int session_info = process_session_info(&session_error_msg);

      if (session_info & 2) {
         put_line(session_error_msg);
         put_line("\n");
      }

      if (session_info & 1) {
         // We are not using a session, either because the user selected
         // "no session", or because of some error in processing the
         // selected session.
         goto no_session;
      }
   }

   goto really_do_it;

   no_session:

   session_index = 0;
   sequence_number = -1;

 really_do_it:

   return false;
}


static int db_tick_max;
static int db_tick_cur;   /* goes from 0 to db_tick_max */

#define TICK_STEPS 52
static int tick_displayed;   /* goes from 0 to TICK_STEPS */

static bool ttu_is_initialized = false;

void iofull::set_utils_ptr(ui_utils *utils_ptr) { m_ui_utils_ptr = utils_ptr; }
ui_utils *iofull::get_utils_ptr() { return m_ui_utils_ptr; }

bool iofull::init_step(init_callback_state s, int n)
{
   // Some operations (e.g. get_session_info) can return information
   // in the boolean return value.  The others don't care.

   char line[MAX_FILENAME_LENGTH];
   int size;

   switch (s) {

   case get_session_info:
      current_text_line = 0;
      if (!ttu_is_initialized) ttu_initialize();
      ttu_is_initialized = true;
      return really_open_session();

   case final_level_query:
      // The level never got specified, either from a command line argument
      // or from the session file.  Perhaps the program was invoked under
      // a window-ish OS in which one clicks on icons rather than typing
      // a command line.  In that case, we need to query the user for the
      // level.

      calling_level = l_mainstream;   // Default in case we fail.
      put_line("Enter the level: ");

      get_string(line, MAX_FILENAME_LENGTH);

      size = strlen(line);

      while (size > 0 && (line[size-1] == '\n' || line[size-1] == '\r'))
         line[--size] = '\000';

      parse_level(line);

      strncat(outfile_string, filename_strings[calling_level], MAX_FILENAME_LENGTH-80);
      break;

   case init_database1:
      // The level has been chosen.  We are about to open the database.
      call_menu_prompts[call_list_empty] = (char *) "--> ";   // This prompt should never be used.
      break;

   case init_database2:
      break;

   case calibrate_tick:
      db_tick_max = n;
      db_tick_cur = 0;
      put_line("Sd: reading database...");
      tick_displayed = 0;
      break;

   case do_tick:
      db_tick_cur += n;
      {
         int tick_new = (TICK_STEPS*db_tick_cur)/db_tick_max;
         while (tick_displayed < tick_new) {
            put_line(".");
            tick_displayed++;
         }
      }

      break;
   case tick_end:
      put_line("done\n");
      break;

   case do_accelerator:
      break;
   }

   return false;
}



/*
 * Create a menu containing number_of_calls[cl] items.
 * Use the "menu_names" array to create a
 * title line for the menu.  The string is in static storage.
 *
 * This will be called once for each value in the enumeration call_list_kind.
 */

void iofull::create_menu(call_list_kind cl)
{
   call_menu_prompts[cl] = new char[50];  /* *** Too lazy to compute it. */

   if (cl == call_list_any)
      // The menu name here is "(any setup)".  That may be a sensible
      // name for a menu, but it isn't helpful as a prompt.  So we
      // just use a vanilla prompt.
      call_menu_prompts[cl] = (char *) "--> ";
   else
      sprintf(call_menu_prompts[cl], "(%s)--> ", menu_names[cl]);
}





/* The main program calls this after all the call menus have been created,
   after all calls to create_menu.
   This performs any final initialization required by the interface package.
*/




void iofull::set_window_title(char s[])
{
   char full_text[MAX_TEXT_LINE_LENGTH];

   if (journal_name[0]) {
      sprintf(full_text, "Sdtty %s {%s}", s, journal_name);
   }
   else {
      sprintf(full_text, "Sdtty %s", s);
   }

   ttu_set_window_title(full_text);
}


static void uims_bell()
{
   if (!ui_options.no_sound) ttu_bell();
}


static void pack_and_echo_character(char c)
{
   // Really should handle error better -- ring the bell,
   // but this is called inside a loop.

   matcher_class &matcher = *gg77->matcher_p;

   if (matcher.m_user_input_size < INPUT_TEXTLINE_SIZE) {
      user_input[matcher.m_user_input_size] = c;
      matcher.m_user_input[matcher.m_user_input_size++] = tolower(c);
      user_input[matcher.m_user_input_size] = '\0';
      matcher.m_user_input[matcher.m_user_input_size] = '\0';
      put_char(c);
   }
}

// This tells how many more lines of matches (the stuff we print in response
// to a question mark) we can print before we have to say "--More--" to
// get permission from the user to continue.  If showing_has_stopped goes on, the
// user has given a negative reply to one of our queries, and so we don't
// print any more stuff.

// Specifically, this number, minus "text_line_count" is how many lines
// we have left on the current screenful.  When text_line_count reaches
// match_counter, we have run out of space and need to query the user.
static int match_counter;

// This is what we reset the counter to whenever the user confirms.  That is,
// it is the number of lines we print per "screenful".  On a VT-100-like
// ("dumb") terminal, we will actually make it a screenful.  On a printing
// device or a workstation, we don't need to do the hold-screen stuff,
// because the device can handle output intelligently (on a printing device,
// it does this by letting us look at the paper that is spewing out on the
// floor; on a workstation we know that the window system provides real
// (infinite, of course) scrolling).  But on printing devices or workstations
// we still do the output in screenful-like blocks, because the user may not
// want to see an enormous amount of output all at once.  This way, a
// workstation user sees things presented one screenful at a time, and only
// needs to scroll back if she wants to look at earlier screenfuls.
// This also caters to those people unfortunate enough to have to use those
// pathetic excuses for workstations (HP-UX and SunOS come to mind) that don't
// allow infinite scrolling.

static int match_lines;


static bool prompt_for_more_output()
{
   put_line("--More--");

   for (;;) {
      int c = get_char();
      clear_line();    // Erase the "more" line; next item goes on that line.

      switch (c) {
      case '\r':
      case '\n':
         match_counter = text_line_count+1; // Show one more line,
         return true;       // but otherwise keep going.
      case '\b':
      case DEL:
      case EKEY+14:    // The "delete" key on a PC.
      case 'q':
      case 'Q':
         return false; // Stop showing.
      case ' ':
         return true;  // Keep going.
      default:
         put_line("Type Space to see more, Return for next line, Delete to stop:  --More--");
      }
   }
}

void iofull::show_match(int frequency_to_show)
{
   if (get_utils_ptr()->matcher_p->m_showing_has_stopped) return;  // Showing has been turned off.

   if (match_counter <= text_line_count) {
      match_counter = text_line_count+match_lines-2;
      if (!prompt_for_more_output()) {
         match_counter = text_line_count-1;   // Turn it off.
         get_utils_ptr()->matcher_p->m_showing_has_stopped = true;
         return;
      }
   }

   get_utils_ptr()->show_match_item(frequency_to_show);
}


void iofull::prepare_for_listing()
{
   match_lines = get_lines_for_more();
   if (match_lines < 2) match_lines = 25;  // The system is screwed up.
   if (ui_options.diagnostic_mode) match_lines = 1000000;
   match_counter = text_line_count+match_lines-2;   // Count for for "--More--" prompt.
   get_utils_ptr()->matcher_p->m_showing_has_stopped = false;
}


static bool get_user_input(const char *prompt, int which)
{
   char *p;
   char c;
   int nc;
   int matches;
   matcher_class &matcher = *gg77->matcher_p;

   matcher.m_active_result.valid = false;
   user_input_prompt = prompt;
   matcher.erase_matcher_input();
   user_input[0] = '\0';
   function_key_expansion = (const char *) 0;
   put_line(user_input_prompt);

   for (;;) {
      /* At this point we always have the concatenation of "user_input_prompt"
         and "user_input" displayed on the current line. */

      start_expand:

      if (function_key_expansion) {
         c = *function_key_expansion++;
         if (c) goto got_char;
         else function_key_expansion = (const char *) 0;
      }

      nc = get_char();

      if (nc >= 128) {
         modifier_block *keyptr;
         int chars_deleted;
         int which_target = which;

         if (which_target > 0) which_target = 0;

         if (nc < FCN_KEY_TAB_LOW || nc > FCN_KEY_TAB_LAST)
            continue;      /* Ignore this key. */

         if (nc == CLOSEPROGRAMKEY) {
            // User clicked the "X" in the upper-right corner, or used the system menu
            // to close the program, or perhaps used the task manager.  In any case,
            // don't close without querying the user.
            if (which_target == matcher_class::e_match_startup_commands || gg77->iob88.do_abort_popup() == POPUP_ACCEPT)
               general_final_exit(0);
         }

         keyptr = matcher.m_fcn_key_table_normal[nc-FCN_KEY_TAB_LOW];

         // Check for special bindings.
         // These always come from the main binding table, even if
         // we are doing something else, like a resolve.

         if (keyptr && keyptr->index < 0) {
            switch (keyptr->index) {
            case special_index_deleteline:
               matcher.erase_matcher_input();
               strcpy(user_input, matcher.m_user_input);
               function_key_expansion = (const char *) 0;
               clear_line();                   // Clear the current line.
               put_line(user_input_prompt);    // Redisplay the prompt.
               continue;
            case special_index_deleteword:
               chars_deleted = matcher.delete_matcher_word();
               while (chars_deleted-- > 0) rubout();
               strcpy(user_input, matcher.m_user_input);
               function_key_expansion = (const char *) 0;
               continue;
            case special_index_quote_anything:
               function_key_expansion = "<anything>";
               goto do_character;
            default:
               continue;    // Ignore all others.
            }
         }

         if (which_target == matcher_class::e_match_startup_commands)
            keyptr = matcher.m_fcn_key_table_start[nc-FCN_KEY_TAB_LOW];
         else if (which_target == matcher_class::e_match_resolve_commands)
            keyptr = matcher.m_fcn_key_table_resolve[nc-FCN_KEY_TAB_LOW];
         else if (which_target == 0)
            keyptr = matcher.m_fcn_key_table_normal[nc-FCN_KEY_TAB_LOW];
         else
            continue;

         if (!keyptr) {
            // If user hits alt-F4 and there is no binding for it, we handle it in
            // the usual way anyway.  This makes the behavior similar to Sd, where
            // the system automatically provides that action.
            if (nc == AFKEY+4) {
               if (which_target == matcher_class::e_match_startup_commands || gg77->iob88.do_abort_popup() == POPUP_ACCEPT)
                  general_final_exit(0);
            }

            continue;   /* No binding for this key; ignore it. */
         }

         // If we get here, we have a function key to process from the table.

         char linebuff[INPUT_TEXTLINE_SIZE+1];
         if (matcher.process_accel_or_abbrev(*keyptr, linebuff)) {
            put_line(linebuff);
            put_line("\n");

            if (journal_file) {
               fputs(linebuff, journal_file);
               fputc('\n', journal_file);
            }

            current_text_line++;
            return false;
         }

         continue;   // Couldn't be processed; ignore the key press.
      }

   do_character:

      c = nc;

      if (function_key_expansion)
         goto start_expand;

   got_char:

      if ((c == '\b') || (c == DEL)) {
         if (matcher.m_user_input_size > 0) {
            matcher.m_user_input_size--;
            user_input[matcher.m_user_input_size] = '\0';
            matcher.m_user_input[matcher.m_user_input_size] = '\0';
            function_key_expansion = (const char *) 0;
            rubout();    // Update the display with the character erased.
         }
         continue;
      }
      else if (c == '?' || c == '!') {
         put_char(c);
         put_line("\n");
         current_text_line++;
         gg77->iob88.prepare_for_listing();
         matcher.match_user_input(which, true, c == '?', false);
         put_line("\n");     // Write a blank line.
         current_text_line++;
         put_line(user_input_prompt);   /* Redisplay the current line. */
         put_line(user_input);
         continue;
      }
      else if (c == ' ' || c == '-') {
         // Extend only to one space or hyphen, inclusive.
         matches = matcher.match_user_input(which, false, false, true);
         p = matcher.m_echo_stuff;

         if (*p) {
            while (*p) {
               if (*p != ' ' && *p != '-')
                  pack_and_echo_character(*p++);
               else {
                  pack_and_echo_character(c);
                  goto foobar;
               }
            }
            continue;   // Do *not* pack the character.

            foobar: ;
         }
         else if (matcher.m_space_ok && matches > 1)
            pack_and_echo_character(c);
         else if (ui_options.diagnostic_mode)
            goto diagnostic_error;
         else
            uims_bell();
      }
      else if ((c == '\n') || (c == '\r')) {

         // Look for abbreviations.

         if (gg77->look_up_abbreviations(which))
            return false;

         matches = matcher.match_user_input(which, false, false, true);

         if (!strcmp(matcher.m_user_input, "help")) {
            put_line("\n");
            matcher.m_active_result.match.kind = ui_help_simple;
            current_text_line++;
            return true;
         }

         /* We forbid a match consisting of two or more "direct parse" concepts, such as
            "grand cross".  Direct parse concepts may only be stacked if they are followed
            by a call.  The "match.next" field indicates that direct parse concepts
            were stacked. */

         if ((matches == 1 || matches - matcher.m_yielding_matches == 1 || matcher.m_final_result.exact) &&
             ((!matcher.m_final_result.match.packed_next_conc_or_subcall &&
               !matcher.m_final_result.match.packed_secondary_subcall) ||
              matcher.m_final_result.match.kind == ui_call_select ||
              matcher.m_final_result.match.kind == ui_concept_select)) {

            p = matcher.m_echo_stuff;
            while (*p)
               pack_and_echo_character(*p++);

            put_line("\n");

            if (journal_file) {
               fputs(matcher.m_user_input, journal_file);
               fputc('\n', journal_file);
            }

            current_text_line++;
            return false;
         }

         if (ui_options.diagnostic_mode)
            goto diagnostic_error;

         /* Tell how bad it is, then redisplay current line. */
         if (matches > 0) {
            char tempstuff[200];

            sprintf(tempstuff, "  (%d matches, type ! or ? for list)\n", matches);
            put_line(tempstuff);
         }
         else
            put_line("  (no matches)\n");

         put_line(user_input_prompt);
         put_line(user_input);
         current_text_line++;   /* Count that line for erasure. */
      }
      else if (c == '\t' || c == '\033') {
         matcher.match_user_input(which, false, false, true);
         p = matcher.m_echo_stuff;

         if (*p) {
            while (*p)
               pack_and_echo_character(*p++);
         }
         else if (ui_options.diagnostic_mode)
            goto diagnostic_error;
         else
            uims_bell();
      }
      else if (isprint(c))
         pack_and_echo_character(c);
      else if (ui_options.diagnostic_mode)
         goto diagnostic_error;
      else
         uims_bell();
   }

   diagnostic_error:

   fputs("\nParsing error during diagnostic.\n", stdout);
   fputs("\nParsing error during diagnostic.\n", stderr);
   general_final_exit(1);
   return false;
}




static const char *banner_prompts0[] = {
    "",
    "simple modifications",
    "all modifications",
    "??"};

static const char *banner_prompts1[] = {
    "",
    "all concepts",
    "??",
    "??"};

static const char *banner_prompts2[] = {
    "",
    "AP",
    "??",
    "??"};

static const char *banner_prompts3[] = {
    "",
    "SC",
    "RSC",
    "??"};

static const char *banner_prompts4[] = {
    "",
    "MG",
    "??",
    "??"};




uims_reply_thing iofull::get_startup_command()
{
   modifier_block &matchmatch = (*gg77->matcher_p).m_final_result.match;

   for (;;) {
      if (!get_user_input("Enter startup command> ", (int) matcher_class::e_match_startup_commands))
         break;

      get_utils_ptr()->writestuff("The program wants you to start a sequence.  Type, for example, "
                                  "'heads start', and press Enter.  Then type a call, such as "
                                  "'pass the ocean', and press Enter again.");
      get_utils_ptr()->newline();
   }

   int index = matchmatch.index;

   if (matchmatch.kind == ui_start_select) {
      // Translate the command.
      index = (int) startup_command_values[index];
   }

   return uims_reply_thing(matchmatch.kind, index);
}



// This returns ui_user_cancel if it fails, e.g. the user waves the mouse away.
uims_reply_thing iofull::get_call_command()
{
   char prompt_buffer[200];
   const char *prompt_ptr;
   modifier_block &matchmatch = (*gg77->matcher_p).m_final_result.match;

   if (allowing_modifications != 0)
      parse_state.call_list_to_use = call_list_any;

   prompt_ptr = prompt_buffer;
   prompt_buffer[0] = '\0';

   // Put any necessary special things into the prompt.

   int banner_mode =
      (allowing_minigrand ? 256 : 0) |
      (ui_options.singing_call_mode << 6) |
      (using_active_phantoms ? 16 : 0) |
      (allowing_all_concepts ? 4 : 0) |
      (allowing_modifications);

   if (banner_mode != 0) {
      int i;
      int comma = 0;

      strcat(prompt_buffer, "[");

      for (i=0 ; i<5 ; i++,banner_mode>>=2) {
         if (banner_mode&3) {

            if (comma) strcat(prompt_buffer, ",");

            if (i==0)
               strcat(prompt_buffer, banner_prompts0[banner_mode&3]);

            if (i==1)
               strcat(prompt_buffer, banner_prompts1[banner_mode&3]);

            if (i==2)
               strcat(prompt_buffer, banner_prompts2[banner_mode&3]);

            if (i==3)
               strcat(prompt_buffer, banner_prompts3[banner_mode&3]);

            if (i==4)
               strcat(prompt_buffer, banner_prompts4[banner_mode&3]);

            comma |= banner_mode&3;
         }
      }

      strcat(prompt_buffer, "] ");
      strcat(prompt_buffer, call_menu_prompts[parse_state.call_list_to_use]);
   }
   else
      prompt_ptr = call_menu_prompts[parse_state.call_list_to_use];

   if (get_user_input(prompt_ptr, (int) parse_state.call_list_to_use)) {
      // User typed "help".
      return uims_reply_thing(ui_command_select, command_help);
   }

   int index = matchmatch.index;

   if (index < 0) {
      // Special encoding from a function key.
      return uims_reply_thing(matchmatch.kind, -1-index);
   }
   else if (matchmatch.kind == ui_command_select) {
      // Translate the command.
      return uims_reply_thing(matchmatch.kind, (int) command_command_values[matchmatch.index]);
   }
   else {
      call_conc_option_state save_stuff = matchmatch.call_conc_options;
      there_is_a_call = false;
      uims_reply_kind my_reply = matchmatch.kind;
      bool retval = deposit_call_tree(&matchmatch, (parse_block *) 0, DFM1_CALL_MOD_MAND_ANYCALL/DFM1_CALL_MOD_BIT);
      matchmatch.call_conc_options = save_stuff;
      if (there_is_a_call) {
         parse_state.topcallflags1 = the_topcallflags;
         my_reply = ui_call_select;
      }

      return uims_reply_thing(retval ? ui_user_cancel : my_reply, index);
   }
}


void iofull::dispose_of_abbreviation(const char *linebuff)
{
   refresh_input();
   put_line(linebuff);
   put_line("\n");

   if (journal_file) {
      fputs(linebuff, journal_file);
      fputc('\n', journal_file);
   }

   current_text_line++;
}


uims_reply_thing iofull::get_resolve_command()
{
   modifier_block &matchmatch = (*gg77->matcher_p).m_final_result.match;

   for (;;) {
      if (!get_user_input("Enter search command> ", (int) matcher_class::e_match_resolve_commands))
         break;

      if (resolver_happiness == resolver_display_failed)
         get_utils_ptr()->writestuff("The program is trying to resolve, but has failed to find anything."
                                     "  You can type 'find another' to keep trying.");
      else
         get_utils_ptr()->writestuff("The program is searching for resolves.  If you like the currently "
                                     "displayed resolve, you can type 'accept' and press Enter."
                                     "  If not, you can type 'find another'.");

      get_utils_ptr()->newline();
   }

   if (matchmatch.index < 0)
      // Special encoding from a function key.
      return uims_reply_thing(matchmatch.kind, -1-matchmatch.index);
   else
      return uims_reply_thing(matchmatch.kind, (int) resolve_command_values[matchmatch.index]);
}


popup_return iofull::get_popup_string(Cstring prompt1, Cstring prompt2, Cstring final_inline_prompt,
                                      Cstring /*seed*/, char *dest)
{
   // We ignore the "seed".  But Sd might use it.

   // Two lines of prompts are allowed.  But if they start with an asterisk,
   // Sd shows it but Sdtty does not.

   if (prompt1 && prompt1[0] && prompt1[0] != '*') {
      get_utils_ptr()->writestuff(prompt1);
      get_utils_ptr()->newline();
   }

   if (prompt2 && prompt2[0] && prompt2[0] != '*') {
      get_utils_ptr()->writestuff(prompt2);
      get_utils_ptr()->newline();
   }

   char buffer[MAX_TEXT_LINE_LENGTH];
   sprintf(buffer, "%s ", final_inline_prompt);
   put_line(buffer);
   get_string(dest, MAX_TEXT_LINE_LENGTH);
   // Backspace at start of line declines the popup.
   if (dest[0] == '\b') return POPUP_DECLINE;

   current_text_line++;
   return dest[0] ? POPUP_ACCEPT_WITH_STRING : POPUP_ACCEPT;
}


static int confirm(Cstring question)
{
   for (;;) {
      put_line(question);
      put_line(" ");
      char c = get_char();
      if ((c=='n') || (c=='N')) {
         put_line("no\n");
         current_text_line++;
         if (journal_file) fputc('n', journal_file);
         return POPUP_DECLINE;
      }
      if ((c=='y') || (c=='Y')) {
         put_line("yes\n");
         current_text_line++;
         if (journal_file) fputc('y', journal_file);
         return POPUP_ACCEPT;
      }

      if (c >= 0) put_char(c);

      if (ui_options.diagnostic_mode) {
         fputs("\nParsing error during diagnostic.\n", stdout);
         fputs("\nParsing error during diagnostic.\n", stderr);
         general_final_exit(1);
      }

      put_line("\n");
      put_line("Answer y or n\n");
      current_text_line += 2;
      uims_bell();
   }
}

int iofull::yesnoconfirm(Cstring /*title*/, Cstring line1, Cstring line2, bool /*excl*/, bool /*info*/)
{
   if (line1) {
      get_utils_ptr()->writestuff(line1);
      get_utils_ptr()->newline();
   }

   return confirm(line2);
}

int iofull::do_abort_popup()
{
   return yesnoconfirm("Confirmation", "The current sequence will be aborted.",
                       "Do you really want to abort it?", true, false);
}

void iofull::update_resolve_menu(command_kind goal, int cur, int max,
                                 resolver_display_state state)
{
   char title[MAX_TEXT_LINE_LENGTH];

   resolver_happiness = state;

   create_resolve_menu_title(goal, cur, max, state, title);
   get_utils_ptr()->writestuff(title);
   get_utils_ptr()->newline();
}

selector_kind iofull::do_selector_popup(matcher_class &matcher)
{
   match_result saved_match = matcher.m_final_result;

   for (;;) {
      if (!get_user_input("Enter who> ", (int) matcher_class::e_match_selectors))
         break;

      get_utils_ptr()->writestuff("The program wants you to type a person designator.  "
                        "Try typing something like 'boys' and pressing Enter.");
      get_utils_ptr()->newline();
   }

   // We skip the zeroth selector, which is selector_uninitialized.
   selector_kind retval = (selector_kind) (matcher.m_final_result.match.index+1);
   matcher.m_final_result = saved_match;
   return retval;
}

direction_kind iofull::do_direction_popup(matcher_class &matcher)
{
   match_result saved_match = matcher.m_final_result;

   for (;;) {
      if (!get_user_input("Enter direction> ", (int) matcher_class::e_match_directions))
         break;

      get_utils_ptr()->writestuff("The program wants you to type a direction.  "
                        "Try typing something like 'right' and pressing Enter.");
      get_utils_ptr()->newline();
   }

   // We skip the zeroth direction, which is direction_uninitialized.
   direction_kind retval = (direction_kind) (matcher.m_final_result.match.index+1);
   matcher.m_final_result = saved_match;
   return retval;
}

int iofull::do_tagger_popup(int tagger_class)
{
   matcher_class &matcher = *gg77->matcher_p;
   match_result saved_match = matcher.m_final_result;

   for (;;) {
      if (!get_user_input("Enter tagging call> ", ((int) matcher_class::e_match_taggers) + tagger_class))
         break;

      get_utils_ptr()->writestuff("The program wants you to type an 'ATC' (tagging) call.  "
                        "Try typing something like 'vertical tag' and pressing Enter.");
      get_utils_ptr()->newline();
   }

   int retval = matcher.m_final_result.match.call_conc_options.tagger;
   saved_match.match.call_conc_options.tagger = matcher.m_final_result.match.call_conc_options.tagger;
   matcher.m_final_result = saved_match;
   matcher.m_final_result.match.call_conc_options.tagger = 0;
   return retval;
}


int iofull::do_circcer_popup()
{
   uint32 retval;
   matcher_class &matcher = *gg77->matcher_p;

   if (interactivity == interactivity_verify) {
      retval = verify_options.circcer;
      if (retval == 0) retval = 1;
   }
   else if (!matcher.m_final_result.valid || (matcher.m_final_result.match.call_conc_options.circcer == 0)) {
      match_result saved_match = matcher.m_final_result;

      for (;;) {
         if (!get_user_input("Enter circulate replacement> ", (int) matcher_class::e_match_circcer))
            break;

         get_utils_ptr()->writestuff("The program wants you to type a circulating call as part of "
                           "a call like 'in roll motivate'.  "
                           "Try typing something like 'in roll circulate' and pressing Enter.");
         get_utils_ptr()->newline();
      }

      retval = matcher.m_final_result.match.call_conc_options.circcer;
      matcher.m_final_result = saved_match;
   }
   else {
      retval = matcher.m_final_result.match.call_conc_options.circcer;
      matcher.m_final_result.match.call_conc_options.circcer = 0;
   }

   return retval;
}

uint32 iofull::get_one_number(matcher_class &matcher)
{
   for (;;) {
      char buffer[200];
      put_line("How many? ");
      get_string(buffer, 200);
      current_text_line++;
      if (buffer[0] == '!' || buffer[0] == '?') {
         get_utils_ptr()->writestuff("Type a number between 0 and 36");
         get_utils_ptr()->newline();
      }
      else if (!buffer[0]) return ~0U;
      else {
         return atoi(buffer);
      }
   }
}



/*
 * add a line to the text output area.
 * the_line does not have the trailing Newline in it and
 * is volatile, so we must copy it if we need it to stay around.
 */

void iofull::add_new_line(const char the_line[], uint32 drawing_picture)
{
    put_line(the_line);
    put_line("\n");
    current_text_line++;
}

// Make everything before the most recent n lines
// not subject to erasure.  Used to protect the
// display of the transcript of preceding sequences.
void iofull::no_erase_before_n(int n)
{
   current_text_line = n;
}

// Throw away all but the first n lines of the text output.
// N = 0 means to erase the entire buffer.
void iofull::reduce_line_count(int n)
{
   if (current_text_line > n)
      erase_last_n(current_text_line-n);

   current_text_line = n;
}


bool iofull::choose_font() { return false; }
bool iofull::print_this() { return false; }
bool iofull::print_any() { return false; }



void iofull::bad_argument(Cstring s1, Cstring s2, Cstring s3)
{
   if (s2 && s2[0]) {
      fprintf(stderr, "%s: %s\n", s1, s2);
   }
   else {
      fprintf(stderr, "%s\n", s1);
   }

   if (s3) fprintf(stderr, "%s\n", s3);
   fprintf(stderr, "%s", "Use the -help flag for help.\n");
   general_final_exit(1);
}


void iofull::fatal_error_exit(int code, Cstring s1, Cstring s2)
{
   if (s2 && s2[0])
      fprintf(stderr, "%s: %s\n", s1, s2);
   else
      fprintf(stderr, "%s\n", s1);

   session_index = 0;  // Prevent attempts to update session file.
   general_final_exit(code);
}


void iofull::serious_error_print(Cstring s1)
{
   fprintf(stderr, "%s\n", s1);
   fprintf(stderr, "Press 'enter' to resume.\n");
   get_char();
}


void iofull::terminate(int code)
{
   if (journal_file) fclose(journal_file);
   if (ttu_is_initialized) ttu_terminate();
   exit(code);
}
