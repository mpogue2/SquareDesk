// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2022  William B. Ackerman.
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

/* This defines the following functions:
   get_escape_string
   write_history_line
   unparse_call_name
   print_recurse
   clear_screen
   ui_utils::writechar
   newline
   doublespace_file
   ui_utils::writestuff
   parse_block::clear
   parse_block::get_block
   parse_block::get_parse_block_mark
   parse_block::release_parse_blocks_to_mark
   parse_block::final_cleanup
   copy_parse_tree
   reset_parse_tree
   save_parse_state
   restore_parse_state
   randomize_couple_colors
   string_copy
   display_initial_history
   initialize_parse
   run_program
and the following external variables:
   global_error_flag
   global_cache_failed_flag
   global_cache_miss_reason
   global_reply
   clipboard
   clipboard_size
   wrote_a_sequence
   retain_after_error
   outfile_string
   outfile_prefix
   header_comment
   creating_new_session
   sequence_number
   starting_sequence_number
   old_filename_strings
   new_filename_strings
   filename_strings
   concept_key_table
*/


#include <stdlib.h>
#include <string.h>

#include "sd.h"
#include "sort.h"


// External variables.
ui_option_type ui_options;
Cstring cardinals[NUM_CARDINALS+1];
Cstring ordinals[NUM_CARDINALS+1];
abridge_mode_t glob_abridge_mode;
error_flag_type global_error_flag;
bool global_cache_failed_flag;
// Word 0 is the error code
//   (Zero if hit, missing index+1 if miss, 9 if couldn't open the cache file.)
// Word 1 is what we wanted at the index.
// Word 2 is what we got.
int global_cache_miss_reason[15];
uims_reply_thing global_reply(ui_user_cancel, 99);
configuration *clipboard = (configuration *) 0;
int clipboard_size = 0;
bool wrote_a_sequence = false;
bool retain_after_error = false;
char outfile_string[MAX_FILENAME_LENGTH] = SEQUENCE_FILENAME;
char outfile_prefix[MAX_FILENAME_LENGTH] = "";
char header_comment[MAX_TEXT_LINE_LENGTH];
bool creating_new_session = false;
int sequence_number = -1;
int starting_sequence_number;

// Under DJGPP, the default is always old-style filenames, because
// the underlying system (DOS or Windows 3.1) presumably can only
// handle "8.3" failenames.  Even under Windows NT, the emulation
// seems to handle only 8.3 filenames.  Under Windows 2000, it
// seems to handle long filenames, but is totally broken in other
// respects (compilation with Cygwin crashed in ntvdm.)  The bug
// has been reported to Microsoft, and, of course, was never fixed.

#if defined(DJGPP)
const Cstring *filename_strings = old_filename_strings;
#else
const Cstring *filename_strings = old_filename_strings; // ******** For now
#endif

// BEWARE!!  These lists are keyed to the definition of "dance_level" in database.h
const Cstring old_filename_strings[] = {
   ".MS",
   ".Plus",
   ".A1",
   ".A2",
   ".C1",
   ".C2",
   ".C3A",
   ".C3",
   ".C3X",
   ".C4A",
   ".C4",
   ".C4X",
   ".all",
   ".all",
   ""};

const Cstring new_filename_strings[] = {
   "_MS.txt",
   "_Plus.txt",
   "_A1.txt",
   "_A2.txt",
   "_C1.txt",
   "_C2.txt",
   "_C3A.txt",
   "_C3.txt",
   "_C3X.txt",
   "_C4A.txt",
   "_C4.txt",
   "_C4X.txt",
   "_all.txt",
   "_all.txt",
   ""};

const Cstring concept_key_table[] = {
   /* These are the standard bindings. */
   "cu     deleteline",
   "cx     deleteword",
   "e6     lineup",
   "e8     linedown",
   "e1     pageup",
   "e2     pagedown",
   "+f1    heads start",
   "+sf1   sides start",
   "+cf1   just as they are",
   "f2     two calls in succession",
   "sf2    twice",
   "f3     pick random call",
   "sf3    pick concept call",
   "cf3    pick simple call",
   "f4     resolve",
   "e4     resolve",            // home
   "sf4    reconcile",
   "cf4    normalize",
   "f5     refresh display",
   "sf5    keep picture",
   "cf5    insert a comment",
   "e13    insert a comment",   // insert
   "f6     simple modifications",
   "sf6    allow modifications",
   "cf6    centers",
   "f7     toggle concept levels",
   "+f7    toggle concept levels",
   "sf7    toggle active phantoms",
   "+sf7   toggle active phantoms",
   "f8     quoteanything",
   "sf8    cut to clipboard",
   "cf8    paste one call",
   "f9     undo last call",
   "+f9    exit from the program",
   "*f9    abort the search",
   "sf9    undo last call",
   "+sf9   exit from the program",
   "*sf9   abort the search",
   "f10    write this sequence",
   "*f10   write this sequence",
   "*e3    write this sequence",    // end
   "sf10   change output file",
   "+sf10  change output file",
   "f11    pick level call",
   "sf11   pick 8 person level call",
   "cf11   standardize",
   "*f12   find another",
   "*sf12  accept current choice",
   "*cf12  previous",
   "*af12  next",
   "*se6   raise reconcile point",  // shift up arrow
   "*e5    previous",               // left arrow
   "*e7    find another",           // right arrow
   "*se8   lower reconcile point",  // shift down arrow
   (char *) 0};


static Cstring sessions_init_table[] = {
   "[Options]",
   "new_style_filename",
   "",
   "[Sessions]",
   "+                    C1               1      Sample",
   "",
   "# \"Enhanced\" accelerator keys can be plain, shifted, control, alt, or control-alt.",
   "# e1 = page up",
   "# e2 = page down",
   "# e3 = end",
   "# e4 = home",
   "# e5 = left arrow",
   "# e6 = up arrow",
   "# e7 = right arrow",
   "# e8 = down arrow",
   "# e13 = insert",
   "# e14 = delete",
   "",
   "[Accelerators]",
   (char *) 0};

static Cstring abbreviations_table[] = {
   "[Abbreviations]",
   "u       U-turn back",
   "rlt     right and left thru",
   (char *) 0};


// Some things fail under Visual C++ version 5 and version 6.  (Under different
// circumstances for those 2 compilers!)  I complained, and they won't even
// acknowledge the existence of the bug report unless I pay them money.
extern void FuckingThingToTryToKeepTheFuckingStupidMicrosoftCompilerFromScrewingUp()
{
}


/* Getting blanks into all the right places in the presence of substitions,
   insertions, and elisions is way too complicated to do in the database, or
   to test.  For example, consider calls like "@4keep @5busy@1",
   "fascinat@pe@q@ning@o@t", or "spin the pulley@7 but @8".  So we try to
   help by never putting two blanks together, always putting blanks adjacent
   to the outside of brackets, and never putting blanks adjacent to the
   inside of brackets.  This procedure is part of that mechanism. */
void ui_utils::write_blank_if_needed()
{
   if (m_writechar_block.lastchar != ' ' &&
       m_writechar_block.lastchar != '[' &&
       m_writechar_block.lastchar != '(' &&
       m_writechar_block.lastchar != '-') writechar(' ');
}

/* This examines the indicator character after an "@" escape.  If the escape
   is for something that is supposed to appear in the menu as a "<...>"
   wildcard, it returns that string.  If it is an escape that starts a
   substring that would normally be elided, as in "@7 ... @8", it returns
   a non-null pointer to a null character.  If it is an escape that is normally
   simply dropped, it returns a null pointer. */

const char *get_escape_string(char c)
{
   switch (c) {
   case '0': case 'm': case 'T':
      return "<ANYTHING>";
   case 'N':
      return "<ANYCIRC>";
   case '6': case 'k': case 'K': case 'V':
      return "<ANYONE>";
   case 'h':
      return "<DIRECTION>";
   case '9':
      return "<N>";
   case 'a': case 'b': case 'B': case 'D':
      return "<N/4>";
   case 'u':
      return "<Nth>";
   case 'v': case 'w': case 'x': case 'y':
      return "<ATC>";
   case '7': case 'n': case 'j': case 'J': case 'E': case 'Q':
      return "";
   case 'X':
      return "others";
   default:
      return (char *) 0;
   }
}


void ui_utils::write_nice_number(char indicator, int num)
{
   if (num < 0) {
      writestuff(get_escape_string(indicator));
   }
   else {
      switch (indicator) {
      case '9': case 'a': case 'b': case 'B': case 'D':
         if (indicator == '9')
            writestuff(cardinals[num]);
         else if (indicator == 'B' || indicator == 'D') {
            if (num == 1)
               writestuff("quarter");
            else if (num == 2)
               writestuff("half");
            else if (num == 3)
               writestuff("three quarter");
            else if (num == 4)
               writestuff("four quarter");
            else {
               writestuff(cardinals[num]);
               writestuff("/4");
            }
         }
         else if (num == 2)
            writestuff("1/2");
         else if (num == 4 && indicator == 'a')
            writestuff("full");
         else {
            writestuff(cardinals[num]);
            writestuff("/4");
         }
         break;
      case 'u':     /* Need to plug in an ordinal number. */
         writestuff(ordinals[num]);
         break;
      }
   }
}


ui_option_type::ui_option_type() :
   color_scheme(color_by_gender),
   force_session(-1000000),
   sequence_num_override(-1),
   no_graphics(0),
   no_c3x(false),
   no_intensify(false),
   reverse_video(false),
   pastel_color(false),
   use_magenta(false),
   use_cyan(false),
   singlespace_mode(false),
   nowarn_mode(false),
   keep_all_pictures(false),
   accept_single_click(false),
   hide_glyph_numbers(false),
   diagnostic_mode(false),
   no_sound(false),
   tab_changes_focus(false),
   max_print_length(59),
   resolve_test_minutes(0),
   resolve_test_random_seed(0),
   singing_call_mode(0),
   use_escapes_for_drawing_people(0),
   pn1("11223344"),
   pn2("BGBGBGBG"),
   direc("?>?<????^?V?????"),
   stddirec("?>?<????^?V?????"),
   squeeze_this_newline(0),
   drawing_picture(0)
{}




void ui_utils::writestuff_with_decorations(const call_conc_option_state *cptr, Cstring f, bool is_concept)
{
   uint32_t index = cptr->number_fields;
   int howmany = cptr->howmanynumbers;
   call_conc_option_state recurse_ptr = *cptr;
   recurse_ptr.who.collapse_down();

   while (f[0]) {
      if (f[0] == '@') {
         switch (f[1]) {
         case 'a': case 'b': case 'B': case 'D': case 'u': case '9':
            // DJGPP has a problem with this, need convert to int.
            write_nice_number(f[1], (howmany <= 0) ? -1 : (int) (index & NUMBER_FIELD_MASK));
            index >>= BITS_PER_NUMBER_FIELD;
            howmany--;
            break;
         case 'h':
            writestuff(is_concept ? direction_names[cptr->where].name_uc : direction_names[cptr->where].name);
            break;
         case '6': case 'K': case 'V':
            writestuff_with_decorations(&recurse_ptr, selector_list[cptr->who.who[0]].name_uc, is_concept);
            break;
         case 'k':
            writestuff_with_decorations(&recurse_ptr, selector_list[cptr->who.who[0]].sing_name_uc, is_concept);
            break;
         default:   /**** maybe we should really do what "translate_menu_name"
                          does, using call to "get_escape_string". */
            break;
         }

         f += 2;
      }
      else
         writechar(*f++);
   }
}


void ui_utils::printperson(uint32_t x)
{
   if (x & BIT_PERSON) {
      if (enable_file_writing || ui_options.use_escapes_for_drawing_people == 0) {
         /* We never write anything other than standard ASCII to the transcript file. */
         writechar(' ');
         writechar(ui_options.pn1[(x >> 6) & 7]);
         writechar(ui_options.pn2[(x >> 6) & 7]);
         if (enable_file_writing)
            writechar(ui_options.stddirec[x & 017]);
         else
            writechar(ui_options.direc[x & 017]);
      }
      else {
         /* Write an escape sequence, so that the user interface can display
            the person in color. */
         writechar('\013');
         writechar((char) (((x >> 6) & 7) | 040));
         writechar((char) ((x & 017) | 040));
      }
   }
   else {
      if (enable_file_writing || ui_options.use_escapes_for_drawing_people <= 1)
         writestuff("  . ");
      else
         writechar('\014');  /* If we have full ("checker") escape sequences, use this. */
   }
}


void ui_utils::do_write(Cstring s)
{
   char c;

   int ri = roti * 011;

   for (;;) {
      if (!(c=(*s++))) return;
      else if (c == '@') {
         if (*s == '7') {
            s++;
            ui_options.squeeze_this_newline = 1;
            newline();
            ui_options.squeeze_this_newline = 0;
         }
         else
            newline();
      }
      else if (c >= 'a' && c <= 'x')
         printperson(rotperson(printarg->people[personstart + ((c-'a'-offs)%modulus)].id1, ri));
      else {
         // We need to do the mundane translation of "5" and "6" if the result
         // isn't going to be used by something that uses same.
         if (enable_file_writing ||
             ui_options.use_escapes_for_drawing_people <= 1 ||
             ui_options.no_graphics != 0) {
            if (c == '6')
               writestuff("    ");    /* space equivalent to 1 full glyph. */
            else if (c == '9')
               writestuff("   ");     /* space equivalent to 3/4 glyph. */
            else if (c == '5')
               writestuff("  ");      /* space equivalent to 1/2 glyph. */
            else if (c == '8')
               writestuff(" ");       /* Like 5, but one space less if doing ASCII.
                                         (Exactly same as 5 if doing checkers. */
            else
               writechar(c);
         }
         else
            writechar(c);
      }
   }
}



void ui_utils::print_4_person_setup(int ps, small_setup *s, int elong)
{
   Cstring str;

   modulus = attr::klimit(s->skind)+1;
   roti = (s->srotation & 3);
   personstart = ps;

   offs = (((roti >> 1) & 1) * (modulus / 2)) - modulus;

   if (s->skind == s2x2) {
      offs = 0;
      if (elong < 0)
         str = "ab@dc@";
      else if (elong == 1)
         str = "a6b@d6c@";
      else if (elong == 2)
         str = "ab@@@dc@";
      else
         str = "a  b@@d  c@";
   }
   else if (s->skind == s1x4 && (roti & 1) && two_couple_calling) {
      str = "a@@b@@d@@c";
   }
   else if (s->skind == s_trngl4 && two_couple_calling) {
      Cstring tgl4strings[4] = {
         "cd@5b@5a@",
         " 6 6c@7a b@7 6 6d@",
         "5a@5b@dc@",
         "d@76b a@7c@"};

      offs = 0;
      str = tgl4strings[roti];
   }
   else
      str = setup_attrs[s->skind].print_strings[roti & 1];

   if (str) {
      newline();
      do_write(str);
   }
   else
      writestuff(" ????");

   if (s->seighth_rotation != 0) {
      writestuff("  Note:  Actual setup is 45 degrees clockwise from diagram above.");
      newline();
   }

   newline();
}



void ui_utils::printsetup(setup *x)
{
   Cstring str;

   ui_options.drawing_picture = 1;
   printarg = x;
   modulus = attr::slimit(x)+1;
   roti = x->rotation & 3;

   newline();

   personstart = 0;

   if ((setup_attrs[x->kind].setup_props & SPROP_4_WAY_SYMMETRY) != 0) {
      /* still to do???
         s_1x1
         s2x2
         s_star
         s_hyperglass */

      offs = (roti * (modulus / 4)) - modulus;
      str = setup_attrs[x->kind].print_strings[0];
   }
   else {
      offs = ((roti & 2) * (modulus / 4)) - modulus;
      str = setup_attrs[x->kind].print_strings[roti & 1];
   }

   if (str && !two_couple_calling)
      do_write(str);
   else {
      switch (x->kind) {
      case s_qtag:
         if ((x->people[0].id1 & x->people[1].id1 &
              x->people[4].id1 & x->people[5].id1 & 1) &&
             (x->people[2].id1 & x->people[3].id1 &
              x->people[6].id1 & x->people[7].id1 & 010)) {
            // People are in diamond-like orientation.
            if (x->rotation & 1)
               do_write("6  g@7f  6  a@76  h@@6  d@7e  6  b@76  c");
            else
               do_write("5 a6 b@@g h d c@@5 f6 e");
         }
         else {
            // People are not.  Probably 1/4-tag-like orientation.
            if (x->rotation & 1)
               do_write("6  g@f  h  a@e  d  b@6  c");
            else
               do_write("6  a  b@@g  h  d  c@@6  f  e");
         }
         break;
      case s_c1phan:
         // Look for nice "classic C1 phantom" occupations, and,
         // if so, draw tighter diagram.
         if (!(x->people[0].id1 | x->people[2].id1 |
               x->people[4].id1 | x->people[6].id1 |
               x->people[8].id1 | x->people[10].id1 |
               x->people[12].id1 | x->people[14].id1))
            str = "8  b@786       h  f@78  d@7@868         l@7n  p@7868         j";
         else if (!(x->people[1].id1 | x->people[3].id1 |
                    x->people[5].id1 | x->people[7].id1 |
                    x->people[9].id1 | x->people[11].id1 |
                    x->people[13].id1 | x->people[15].id1))
            str = "868         e@7a  c@7868         g@7@8  o@786       k  i@78  m";
         else
            str = "58b66e@a88c  h88f@58d66g@@58o66l@n88p  k88i@58m66j";

         do_write(str);
         break;
      case s_trngl8:
         offs = 0;

         switch (roti) {
         case 0:
            str = "e f g h@@6 5d@6 5c@6 5b@6 5a";
            break;
         case 1:
            str = "6 6 6 6 e@@6 6 6 6 f@7a b c d@76 6 6 6 g@@6 6 6 6 h";
            break;
         case 2:
            str = "6 5a@6 5b@6 5c@6 5d@@h g f e";
            break;
         default:
            str = "h@@g@76 d c b a@7f@@e";
            break;
         }

         do_write(str);
         break;
      case s1x3p1dmd:
         offs = 0;

         switch (roti) {
         case 0:
            str = "6 6 6 d@7a b c 6 e@76 6 6 f";
            break;
         case 1:
            str = " 5a@@ 5b@@ 5c@@ fd@@ 5e@";
            break;
         case 2:
            str = "6 f@7e 6 c b a@76 d";
            break;
         default:
            str = " 5e@@ df@@ 5c@@ 5b@@ 5a@";
            break;
         }

         do_write(str);
         break;
      case s4p2x1dmd:
         offs = 0;

         switch (roti) {
         case 0:
            str = "6 6 6 9e@@a b c d g f@@6 6 6 9h";
            break;
         case 1:
            str = "6  a@@6  b@@6  c@@6  d@7h  6  e@76  g@@6  f";
            break;
         case 2:
            str = "6 9h@@f g d c b a@@6 9e";
            break;
         default:
            str = "6  f@@6  g@7e  6  h@76  d@@6  c@@6  b@@6  a";
            break;
         }

         do_write(str);
         break;
      case s3p1x1dmd:
         // Stolen from s4p2x1dmd
         offs = 0;

         switch (roti) {
         case 0:
            str = "6 6 9d@@a b c e@@6 6 9f";
            break;
         case 1:
            str = "6  a@@6  b@@6  c@7f  6  d@76  e";
            break;
         case 2:
            str = "9f@@e c b a@@9d";
            break;
         default:
            str = "6  e@7d  6  f@76  c@@6  b@@6  a";
            break;
         }

         do_write(str);
         break;
      case splinepdmd:
         offs = 0;

         switch (roti) {
         case 0:
            str = "6 6 6 6 6 f@7a b c d e 6 g@76 6 6 6 6 h";
            break;
         case 1:
            str = " 5a@@ 5b@@ 5c@@ 5d@@ 5e@@ hf@@ 5g@";
            break;
         case 2:
            str = "6 h@7g 6 e d c b a@76 f";
            break;
         default:
            str = " 5g@@ fh@@ 5e@@ 5d@@ 5c@@ 5b@@ 5a@";
            break;
         }

         do_write(str);
         break;
      case splinedmd:
         offs = 0;

         switch (roti) {
         case 0:
            str = "6 6 6 6 9f@@a b c d e g@@6 6 6 6 9h";
            break;
         case 1:
            str = "6  a@@6  b@@6  c@@6  d@@6  e@7h  6  f@76  g";
            break;
         case 2:
            str = "9h@@g e d c b a@@9f";
            break;
         default:
            str = "6  g@7f  6  h@76  e@@6  d@@6  c@@6  b@@6  a";
            break;
         }

         do_write(str);
         break;
      case slinedmd:
         offs = 0;

         switch (roti) {
         case 0:
            str = "e f h g@@6  8b@78a  6  c@76  8d";
            break;
         case 1:
            str = "9a9e@969f@7d b@7969h@9c9g";
            break;
         case 2:
            str = "6  8d@78c  6  a@76  8b@@g h f e";
            break;
         default:
            str = "g9c@h@76 b d@7f@e9a";
            break;
         }

         do_write(str);
         break;
      case slinepdmd:
         offs = 0;

         switch (roti) {
         case 0:
            str = "e f h g@@6 5c@@6 b d@@6 5a";
            break;
         case 1:
            str = "6 6 6 e@6 b 6 f@7a 6 c@76 d 6 h@6 6 6 g";
            break;
         case 2:
            str = "6 5a@@6 d b@@6 5c@@g h f e";
            break;
         default:
            str = "g@h 6 d@76 c 6 a@7f 6 b@e";
            break;
         }

         do_write(str);
         break;
      case slinebox:
         offs = 0;

         switch (roti) {
         case 0:
            str = "6 6 6 6 e f@7a b c d@76 6 6 6 h g";
            break;
         case 1:
            str = " 5a@@ 5b@@ 5c@@ 5d@@ he@@ gf@";
            break;
         case 2:
            str = "g h@76 6 d c b a@7f e";
            break;
         default:
            str = " fg@@ eh@@ 5d@@ 5c@@ 5b@@ 5a@";
            break;
         }

         do_write(str);
         break;
      case slinejbox:
         offs = 0;

         switch (roti) {
         case 0:
            str = "6 6 c d e@7a b@76 6 h g f";
            break;
         case 1:
            str = " 5a@@ 5b@@ hc@@ gd@@ fe";
            break;
         case 2:
            str = "f g h@76 6 6 b a@7e d c";
            break;
         default:
            str = " ef@@ dg@@ ch@@ 5b@@ 5a";
            break;
         }

         do_write(str);
         break;
      case slinevbox:
         offs = 0;

         switch (roti) {
         case 0:
            str = "6 6 6 6 6 6 g@7a b c d e f@76 6 6 6 6 6 h";
            break;
         case 1:
            str = " 5a@@ 5b@@ 5c@@ 5d@@ 5e@@ 5f@@ hg@";
            break;
         case 2:
            str = "h@76 f e d c b a@7g";
            break;
         default:
            str = " gh@@ 5f@@ 5e@@ 5d@@ 5c@@ 5b@@ 5a@";
            break;
         }

         do_write(str);
         break;
      case slineybox:
         offs = 0;

         switch (roti) {
         case 0:
            str = "a 6 6 e f@76 c d@7b 6 6 h g";
            break;
         case 1:
            str = " ba@@ 5c@@ 5d@@ he@@ gf";
            break;
         case 2:
            str = "g h 6 6 b@76 6 d c@7f e 6 6 a";
            break;
         default:
            str = " fg@@ eh@@ 5d@@ 5c@@ ab";
            break;
         }

         do_write(str);
         break;
      case slinefbox:
         offs = 0;

         switch (roti) {
         case 0:
            str = "6 6 6 6 e@7a b c d 6 g f@76 6 6 6 h";
            break;
         case 1:
            str = " 5a@@ 5b@@ 5c@@ 5d@@ he@@ 5g@@ 5f@";
            break;
         case 2:
            str = "6 6 h@7f g 6 d c b a@76 6 e";
            break;
         default:
            str = " 5f@@ 5g@@ eh@@ 5d@@ 5c@@ 5b@@ 5a@";
            break;
         }

         do_write(str);
         break;
      case sdbltrngl4:
         offs = 0;

         switch (roti) {
         case 0:
            str = "6 6 c 6 6 g@7a b 6 e f@76 6 d 6 6 h";
            break;
         case 1:
            str = " 5a@@ 5b@@ dc@@ 5e@@ 5f@@ hg";
            break;
         case 2:
            str = "h 6 6 d@76 f e 6 b a@7g 6 6 c";
            break;
         default:
            str = " gh@@ 5f@@ 5e@@ cd@@ 5b@@ 5a";
            break;
         }

         do_write(str);
         break;
      case sboxdmd:
         offs = 0;

         switch (roti) {
         case 0:
            str = "6e f@@6h g@@6  8b@78a  6  c@76  8d";
            break;
         case 1:
            str = "9a@969h e@7d b@7969g f@9c";
            break;
         case 2:
            str = "6  8d@78c  6  a@76  8b@@6g h@@6f e";
            break;
         default:
            str = "696 c@f g@76 6 b d@7e h@696 a";
            break;
         }

         do_write(str);
         break;
      case sdbltrngl:
         offs = 0;

         switch (roti) {
         case 0:
            str = "6 b 6 e@7a 6 d@76 c 6 f";
            break;
         case 1:
            str = "6 5a@@6 c b@@6 5d@@6 f e";
            break;
         case 2:
            str = "f 6 c@76 6 d 6 a@7e 6 b";
            break;
         default:
            str = "6 e f@@6 5d@@6 b c@@6 5a";
            break;
         }

         do_write(str);
         break;
      case sboxpdmd:
         offs = 0;

         switch (roti) {
         case 0:
            str = "6 e f@@6 h g@@6 5c@@6 b d@@6 5a";
            break;
         case 1:
            str = "6 b 6 h e@7a 6 c@76 d 6 g f";
            break;
         case 2:
            str = "6 5a@@6 d b@@6 5c@@6 g h@@6 f e";
            break;
         default:
            str = "f g 6 d@76 6 c 6 a@7e h 6 b";
            break;
         }

         do_write(str);
         break;
      case sdmdpdmd:
         offs = 0;

         switch (roti) {
         case 0:
            str = "65f@7e  6  g@765h@@65c@@6b d@@65a";
            break;
         case 1:
            str = "6 6 6 5 e@76 b@7a 6 c h f@76 d@76 6 6 5 g";
            break;
         case 2:
            str = "65a@@6d b@@65c@@65h@7g  6  e@765f";
            break;
         default:
            str = "5g@76 6 6 d@7f h c 6 a@76 6 6 b@75e";
            break;
         }

         do_write(str);
         break;
      case s_dead_concentric:
         ui_options.drawing_picture = 0;
         writestuff(" centers only:");
         newline();
         ui_options.drawing_picture = 1;
         print_4_person_setup(0, &(x->inner), -1);
         break;
      case s_normal_concentric:
         ui_options.drawing_picture = 0;
         writestuff(" centers:");
         newline();
         ui_options.drawing_picture = 1;
         print_4_person_setup(0, &(x->inner), -1);
         ui_options.drawing_picture = 0;
         writestuff(" ends:");
         newline();
         ui_options.drawing_picture = 1;
         print_4_person_setup(12, &(x->outer), x->concsetup_outer_elongation);
         break;
      default:
         if (two_couple_calling) {
            small_setup ss;
            ss.skind = x->kind;
            ss.srotation = x->rotation;
            ss.seighth_rotation = x->eighth_rotation;
            print_4_person_setup(0, &ss, 3);
         }
         else {
            ui_options.drawing_picture = 0;
            writestuff("???? UNKNOWN SETUP ????");
            newline();
            ui_options.drawing_picture = 1;
         }
      }
   }

   newline();
   ui_options.drawing_picture = 0;
   newline();
}


void ui_utils::write_history_line(int history_index,
                                 bool picture,
                                 bool leave_missing_calls_blank,
                                 file_write_flag write_to_file)
{
   m_leave_missing_calls_blank = leave_missing_calls_blank;
   m_selector_recursion_level = -1;  // Will go up to >= 0 when printing selectors.

   int w, i;
   parse_block *thing;
   configuration *this_item = &configuration::history[history_index];

   if (write_to_file == file_write_double && !ui_options.singlespace_mode)
      doublespace_file();

   // Do not put index numbers into output file -- user may edit it later.

   if (!enable_file_writing && !ui_options.diagnostic_mode) {
      i = history_index-configuration::whole_sequence_low_lim+1;
      if (i > 0) {
         char indexbuf[10];
         sprintf(indexbuf, "%2d:   ", i);
         writestuff(indexbuf);
      }
   }

   if (this_item->nontrivial_startinfo_specific()) {
      if (this_item->get_startinfo_specific()->into_the_middle) goto morefinal;
      writestuff(this_item->get_startinfo_specific()->name);
      goto final;
   }

   thing = this_item->command_root;

   // Need to check for the special case of starting a sequence with heads or sides.
   // If this is the first line of the history, and we started with heads of sides,
   // change the name of this concept from "centers" to the appropriate thing.

   if (history_index == 2 &&
       thing->concept->kind == concept_centers_or_ends &&
       thing->concept->arg1 == selector_centers) {
      if (configuration::history[1].get_startinfo_specific()->into_the_middle) {
         writestuff(configuration::history[1].get_startinfo_specific()->name);
         writestuff(" ");
         thing = thing->next;
      }
   }

   print_recurse(thing, 0);

   final:

   newline();

   morefinal:

   // Check for warnings to print.
   // Do not double space them, even if writing final output.

   // First, don't print both "bad concept level" and "bad modifier level".
   if (this_item->test_one_warning_specific(warn__bad_concept_level))
      this_item->clear_one_warning_specific(warn__bad_modifier_level);

   // Or "opt for parallelogram" and "each 1x4".
   if (this_item->test_one_warning_specific(warn__check_pgram))
      this_item->clear_one_warning_specific(warn__each1x4);

   // Or both varieties of "each 1x3".
   if (this_item->test_one_warning_specific(warn__split_to_1x3s_always))
      this_item->set_one_warning_specific(warn__split_to_1x3s);
   this_item->clear_one_warning_specific(warn__split_to_1x3s_always);

   // Or "each 1x6" and "each 1x3".
   if (this_item->test_one_warning_specific(warn__split_to_1x3s))
      this_item->clear_one_warning_specific(warn__split_to_1x6s);

   // Or "controversial" and "other axis".
   if (this_item->test_one_warning_specific(warn_other_axis))
      this_item->clear_one_warning_specific(warn_controversial);

   // Or "really_no_eachsetup".
   this_item->clear_one_warning_specific(warn__really_no_eachsetup);

   // Or "do your part" (that is, the "warn__dyp_or_2faced" version) and "you ought to say 2-faced".
   // And don't say "do your part" twice.
   if (this_item->test_one_warning_specific(warn__two_faced))
      this_item->clear_one_warning_specific(warn__unusual_or_2faced);

   if (!ui_options.nowarn_mode) {
      for (w=0 ; w<warn__NUM_WARNINGS ; w++) {
         if (this_item->test_one_warning_specific((warning_index) w)) {
            writestuff("  Warning:  ");
            writestuff(&warning_strings[w][1]);
            newline();
         }
      }
   }

   // The "keep all pictures" flag could trick us into drawing a picture of
   // an invalid setup during sequence startup when the very intricate
   // "heads/sides start" mechanism is being used.  But when writing out
   // the transcript file, the same setup will be valid.  We use the
   // "state_is_valid" flag to tell when it is permissible to draw the
   // picture.

   if (this_item->state_is_valid &&
       (picture || this_item->draw_pic || ui_options.keep_all_pictures)) {
      printsetup(&this_item->state);

      if (this_item->state.eighth_rotation != 0) {
         writestuff("  Note:  Actual setup is 45 degrees clockwise from diagram above.");
         newline();
         if (write_to_file == file_write_double && ui_options.singlespace_mode)
            doublespace_file();
      }
   }

   // Record that this history item has been written to the UI.
   this_item->text_line = text_line_count;
}



uint32_t ui_utils::get_number_fields(int nnumbers, bool odd_number_only, bool forbid_zero)
{
   int i;
   uint32_t number_fields = matcher_p->m_final_result.match.call_conc_options.number_fields;
   int howmanynumbers = matcher_p->m_final_result.match.call_conc_options.howmanynumbers;
   uint32_t number_list = 0;

   for (i=0 ; i<nnumbers ; i++) {
      uint32_t this_num;

      if (!matcher_p->m_final_result.valid || (howmanynumbers <= 0)) {
         this_num = iob88.get_one_number(*matcher_p);
      }
      else {
         this_num = number_fields & NUMBER_FIELD_MASK;
         number_fields >>= BITS_PER_NUMBER_FIELD;
         howmanynumbers--;
      }

      // Check legality.
      if ((odd_number_only && !(this_num & 1)) ||
          (forbid_zero && this_num == 0) ||
          (this_num >= NUM_CARDINALS))
         return UINT32_C(~0);

      number_list |= (this_num << (i*BITS_PER_NUMBER_FIELD));
   }

   return number_list;
}


bool ui_utils::look_up_abbreviations(int which)
{
   abbrev_block *asearch = (abbrev_block *) 0;

   if (which == matcher_class::e_match_startup_commands)
      asearch = matcher_p->m_abbrev_table_start;
   else if (which == matcher_class::e_match_resolve_commands)
      asearch = matcher_p->m_abbrev_table_resolve;
   else if (which >= 0)
      asearch = matcher_p->m_abbrev_table_normal;

   for ( ; asearch ; asearch = asearch->next) {
      if (!strcmp(asearch->key, matcher_p->m_user_input)) {
         char linebuff[INPUT_TEXTLINE_SIZE+1];
         if (matcher_p->process_accel_or_abbrev(asearch->value, linebuff)) {
            iob88.dispose_of_abbreviation(linebuff);
            return true;
         }
         break;   // Couldn't be processed.  Stop.  No other abbreviations will match.
      }
   }

   return false;
}


void ui_utils::unparse_call_name(Cstring name, char *s, const call_conc_option_state *options)
{
   writechar_block_type saved_writeblock = m_writechar_block;
   m_writechar_block.destcurr = s;
   m_writechar_block.usurping_writechar = true;

   writestuff_with_decorations(options, name, false);
   writechar('\0');

   m_writechar_block = saved_writeblock;
   m_writechar_block.usurping_writechar = false;
}


/* There are a few bits that are meaningful in the argument to "print_recurse":
         PRINT_RECURSE_STAR
   This means to print an asterisk for a call that is missing in the
   current type-in state.
         PRINT_RECURSE_CIRC
   This means that this is a circulate-substitute call, and should have any
   @O ... @P stuff elided from it.
         PRINT_RECURSE_SELECTOR
   This means that we are really just printing a selector, from the stack
   in local_cptr->options.who.
         PRINT_RECURSE_SELECTOR_SING
         As above, but print singular name. */




selector_kind fix_short_other_selector(selector_kind kk)
{
   selector_kind opp = selector_list[kk].opposite;
   if (two_couple_calling) {
      switch (opp) {
      case selector_outer6: opp = selector_ends; break;
      case selector_farsix: opp = selector_fartwo; break;
      case selector_nearsix: opp = selector_neartwo; break;
      case selector_farfive: opp = selector_farthest1; break;
      case selector_nearfive: opp = selector_nearest1; break;
      }
   }

   return opp;
}






void ui_utils::print_recurse(parse_block *thing, int print_recurse_arg)
{
   bool use_left_name = false;
   bool use_cross_name = false;
   bool use_magic_name = false;
   bool use_grand_name = false;
   bool use_intlk_name = false;
   bool allow_deferred_concept = true;
   parse_block *deferred_concept = (parse_block *) 0;
   int deferred_concept_paren = 0;
   int comma_after_next_concept = 0;    // 1 for comma, 2 for the word "all", 5 to skip an extra if it's "tandem".
   int did_comma = 0;                   // 1 for comma, 2 for the word "all", 5 to skip an extra if it's "tandem".
   bool did_concept = false;
   bool last_was_t_type = false;
   bool last_was_l_type = false;
   bool request_final_space = false;

   parse_block *local_cptr = thing;

   while (local_cptr) {
      concept_kind k;
      const concept_descriptor *item = local_cptr->concept;
      k = item->kind;

      if (k == concept_comment) {
         comment_block *fubb;

         fubb = (comment_block *) local_cptr->call_to_print;
         if (request_final_space) writestuff(" ");
         writestuff("{ ");
         writestuff(fubb->txt);
         writestuff(" } ");
         local_cptr = local_cptr->next;
         request_final_space = false;
         last_was_t_type = false;
         last_was_l_type = false;
         comma_after_next_concept = 0;
      }
      else if (k > marker_end_of_list) {
         // This is a concept.
         bool force = false;
         // 1 for comma, 2 for the word "all", 3 to skip an extra if it's "tandem".
         int request_comma_after_next_concept = 0;

         // Some concepts look better with a comma after them.

         if (item->concparseflags & CONCPARSE_PARSE_F_TYPE) {
            // This is an "F" type concept.
            comma_after_next_concept = 1;
            last_was_t_type = false;
            force = did_concept && !last_was_l_type;
            last_was_l_type = false;
            did_concept = true;
         }
         else if (item->concparseflags & CONCPARSE_PARSE_L_TYPE) {
            // This is an "L" type concept.
            last_was_t_type = false;
            last_was_l_type = true;
         }
         else if (item->concparseflags & CONCPARSE_PARSE_G_TYPE) {
            // This is a "leading T/trailing L" type concept, also known as a "G" concept.
            force = last_was_t_type && !last_was_l_type;;
            last_was_t_type = false;
            last_was_l_type = true;
         }
         else {
            // This is a "T" type concept.
            if (did_concept && k != concept_tandem_in_setup) comma_after_next_concept = 1;
            force = last_was_t_type && !last_was_l_type;
            last_was_t_type = true;
            last_was_l_type = false;
            did_concept = true;
         }

         // We never put a comma before things like "in a 1/4 tag".
         if (local_cptr->say_and)
            writestuff(" AND ");
         else if (force && did_comma == 0 && k != concept_tandem_in_setup) writestuff(", ");
         else if (request_final_space) writestuff(" ");

         parse_block *next_cptr = local_cptr->next;    // Now it points to the thing after this concept.

         request_final_space = false;

         if (concept_table[k].concept_prop & CONCPROP__SECOND_CALL) {
            parse_block *subsidiary_ptr = local_cptr->subsidiary_root;

            if (k == concept_centers_and_ends) {
               if (item->arg1 == selector_center6 ||
                   item->arg1 == selector_center2 ||
                   item->arg1 == selector_verycenters)
                  writestuff(selector_list[item->arg1].name_uc);
               else
                  writestuff(selector_list[selector_centers].name_uc);

               writestuff(" ");
            }
            else if (k == concept_some_vs_others) {
               selective_key sk = (selective_key) item->arg1;

               if (sk == selective_key_dyp)
                  writestuff_with_decorations(&local_cptr->options, "DO YOUR PART, @6 ", true);
               else if (sk == selective_key_own)
                  writestuff_with_decorations(&local_cptr->options, "OWN THE @6, ", true);
               else if (sk == selective_key_plain)
                  writestuff_with_decorations(&local_cptr->options, "@6 ", true);
               else
                  writestuff_with_decorations(&local_cptr->options, "@6 DISCONNECTED ", true);
            }
            else if (k == concept_sequential) {
               writestuff("(");
            }
            else if (k == concept_special_sequential ||
                     k == concept_special_sequential_num ||
                     k == concept_special_sequential_4num) {
               switch (item->arg1) {
               case part_key_use_last_part:
                  writestuff("USE ");

                  // If stuff hasn't been completely entered, show what we want.
                  if (!local_cptr->next || !subsidiary_ptr)
                     writestuff_with_decorations(&local_cptr->options, "(for last part) ", true);
                  break;
               case part_key_use_nth_part:
                  writestuff("USE ");

                  // If stuff hasn't been completely entered, show the number.
                  if (!local_cptr->next || !subsidiary_ptr)
                     writestuff_with_decorations(&local_cptr->options, "(for @u part) ", true);
                  break;
               case part_key_frac_and_frac:
                  writestuff_with_decorations(&local_cptr->options, item->name, true);
                  writestuff(" ");
                  break;
               default:
                  writestuff(item->name);
                  writestuff(" ");
                  break;
               }
            }
            else if (k == concept_replace_nth_part ||
                     k == concept_replace_last_part ||
                     k == concept_interrupt_at_fraction) {
               if (local_cptr->next && subsidiary_ptr) {
                  writestuff("DELAY: ");
               }
               else {
                  switch (local_cptr->concept->arg1) {
                  case 0:
                     writestuff("replace the last part of ");
                     if (!local_cptr->next) writestuff("this call:");
                     break;
                  case 1:
                     writestuff("interrupt before the last part of ");
                     if (!local_cptr->next) writestuff("this call:");
                     break;
                  case 2:
                     if (!local_cptr->next)
                        writestuff_with_decorations(&local_cptr->options,
                                                    "interrupt this call after @9/@9:", true);
                     else
                        writestuff("interrrupt ");
                     break;
                  case 8:
                     writestuff_with_decorations(&local_cptr->options,
                                                 "replace the @u part of ", true);
                     if (!local_cptr->next) writestuff("this call:");
                     break;
                  case 9:
                     writestuff_with_decorations(&local_cptr->options,
                                                 "interrupting after the @u part of ", true);
                     if (!local_cptr->next) writestuff("this call:");
                     break;
                  }
               }
            }
            else {
               writestuff(item->name);
               writestuff(" ");
            }

            print_recurse(local_cptr->next, 0);

            // If echoing incomplete input, we can't show the second call.
            // But we may want to show something useful.
            if (!subsidiary_ptr) {
               if (local_cptr->next) {
                  if (k == concept_replace_nth_part ||
                      k == concept_replace_last_part ||
                      k == concept_interrupt_at_fraction) {
                     switch (local_cptr->concept->arg1) {
                     case 0: case 1: case 8: case 9:
                        writestuff(" with this call:");
                        break;
                     case 2:
                        writestuff_with_decorations(&local_cptr->options,
                                                    " after @9/@9 with this call:", true);
                        break;
                     }
                  }
               }
               break;
            }

            did_concept = false;                /* We're starting over. */
            last_was_t_type = false;
            last_was_l_type = false;
            comma_after_next_concept = 0;
            request_final_space = true;

            if (k == concept_centers_and_ends) {
               selector_kind opp = selector_list[item->arg1].opposite;

               writestuff(" WHILE THE ");
               writestuff((opp == selector_uninitialized) ?
                          ((Cstring) "OTHERS") :
                          selector_list[opp].name_uc);

               if (item->arg2)
                  writestuff(" CONCENTRIC");
            }
            else if (k == concept_some_vs_others &&
                     (selective_key) item->arg1 != selective_key_own) {
               selector_kind opp = fix_short_other_selector(local_cptr->options.who.who[0]);
               writestuff(" WHILE THE ");
               writestuff((opp == selector_uninitialized) ?
                          ((Cstring) "OTHERS") :
                          selector_list[opp].name_uc);
            }
            else if (k == concept_on_your_own)
               writestuff(" AND");
            else if (k == concept_interlace)
               writestuff(" WITH");
            else if (k == concept_sandwich)
               writestuff(" AROUND");
            else if (k == concept_replace_nth_part ||
                     k == concept_replace_last_part ||
                     k == concept_interrupt_at_fraction) {
               writestuff(" BUT ");
               writestuff_with_decorations(&local_cptr->options, local_cptr->concept->name, true);
               writestuff(" WITH A [");
               request_final_space = false;
            }
            else if (k == concept_sequential)
               writestuff(" ;");
            else if (k == concept_special_sequential ||
                     k == concept_special_sequential_num ||
                     k == concept_special_sequential_4num) {
               switch (item->arg1) {
               case part_key_start_with:
                  writestuff(" :");
                  break;
               case part_key_use:
                  writestuff(" IN");
                  break;
               case part_key_use_nth_part:
                  writestuff_with_decorations(&local_cptr->options, " FOR THE @u PART: ", true);
                  request_final_space = false;
                  break;
               case part_key_half_and_half:
               case part_key_frac_and_frac:
                  writestuff(" AND");
                  break;
               case part_key_use_last_part:
                  writestuff_with_decorations(&local_cptr->options, " FOR THE LAST PART: ", true);
                  request_final_space = false;
                  break;
               default:
                  writestuff(",");
                  break;
               }
            }
            else
               writestuff(" BY");

            // Note that we leave "allow_deferred_concept" on.  This means that
            // if we say "twice" immediately inside the second clause
            // of an interlace, it will get the special processing.
            // The first clause will get the special processing by virtue
            // of the recursion.

            next_cptr = subsidiary_ptr;

            // Setting this means that, if the second argument uses "twice",
            // we will put it in parens.  This is needed to disambiguate
            // this situation from the use of "twice" before the entire
            // "interlace".
            if (deferred_concept_paren == 0) deferred_concept_paren = 2;
         }
         else {
            const call_with_name *target_call = (call_with_name *) 0;
            const parse_block *tptr;

            // Look for special concepts that, in conjunction with calls that have
            // certain escape codes in them, get deferred and inserted into the call name.

            if (local_cptr && (k == concept_left ||
                               k == concept_cross ||
                               k == concept_magic ||
                               k == concept_grand ||
                               k == concept_interlocked)) {

               // These concepts want to take special action if there are no following
               // concepts and certain escape characters are found in the name of
               // the following call.

               final_and_herit_flags junk_concepts;
               junk_concepts.clear_all_herit_and_final_bits();

               // Skip all final concepts, then demand that what remains is a marker
               // (as opposed to a serious concept), and that a real call
               // has been entered, and that its name starts with "@g".
               tptr = process_final_concepts(next_cptr, false, &junk_concepts, false, false);

               if (tptr && tptr->concept->kind <= marker_end_of_list) target_call = tptr->call_to_print;
            }

            if (target_call &&
                k == concept_left &&
                (target_call->the_defn.callflagsf & ESCAPE_WORD__LEFT)) {
               use_left_name = true;
            }
            else if (target_call &&
                     k == concept_magic &&
                     (target_call->the_defn.callflagsf & ESCAPE_WORD__MAGIC)) {
               use_magic_name = true;
            }
            else if (target_call &&
                     k == concept_grand &&
                     (target_call->the_defn.callflagsf & ESCAPE_WORD__GRAND)) {
               use_grand_name = true;
            }
            else if (target_call &&
                     k == concept_interlocked &&
                     (target_call->the_defn.callflagsf & ESCAPE_WORD__INTLK)) {
               use_intlk_name = true;
            }
            else if (target_call &&
                     k == concept_cross &&
                     (target_call->the_defn.callflagsf & ESCAPE_WORD__CROSS)) {
               use_cross_name = true;
            }
            else if (allow_deferred_concept &&
                     next_cptr &&
                     ((k == concept_n_times_const && item->arg2 <= 100) ||
                      k == concept_n_times ||
                      (k == concept_fractional && item->arg1 == 2))) {
               deferred_concept = local_cptr;
               comma_after_next_concept = 0;
               did_concept = false;

               // If there is another concept, we need parens.
               if (next_cptr->concept->kind > marker_end_of_list) deferred_concept_paren |= 1;

               if (deferred_concept_paren == 3) writestuff("(");
               if (deferred_concept_paren) writestuff("(");
            }
            else {
               if ((get_meta_key_props(item) & MKP_COMMA_NEXT) ||
                   (k == concept_snag_mystic && item->arg1 == CMD_MISC2__SAID_INVERT)) {
                  // This is "DO THE <Nth> PART",
                  // or INVERT followed by another concept, which must be SNAG or MYSTIC,
                  // or INITIALLY/FINALLY.
                  // These concepts require a comma after the following concept.
                  request_comma_after_next_concept = 1;
               }
               else if (k == concept_so_and_so_only &&
                        ((selective_key) item->arg1) == selective_key_work_concept) {
                  // "<ANYONE> WORK"
                  // This concept requires the word "all" after the following concept.
                  request_comma_after_next_concept = 2;
               }
               else if (k == concept_c1_phantom &&
                        comma_after_next_concept == 1 &&
                        next_cptr &&
                        (next_cptr->concept->kind == concept_tandem ||
                         next_cptr->concept->kind == concept_frac_tandem)) {
                  comma_after_next_concept = 5;
               }

               writestuff_with_decorations(&local_cptr->options, local_cptr->concept->name, true);
               request_final_space = true;
            }

            // For some concepts, we still permit the "defer" stuff.  But don't do it
            // if others are doing the call, because that would lead to
            // "<anyone> work 1-1/2, swing thru" turning into
            // "<anyone> work swing thru 1-1/2".

            if ((k != concept_so_and_so_only || item->arg2) &&
                k != concept_centers_or_ends &&
                k != concept_c1_phantom &&
                k != concept_tandem)
               allow_deferred_concept = false;
         }

         if (comma_after_next_concept == 2) {
            writestuff(", ALL");
            request_final_space = true;
         }
         else if (comma_after_next_concept == 1) {
            if (!(next_cptr && next_cptr->say_and))
               writestuff(",");
            request_final_space = true;
         }
         else if (comma_after_next_concept == 5) {
            request_comma_after_next_concept = 1;
         }

         // 1 for comma, 2 for the word "all", 3 to skip an extra if it's "tandem".
         did_comma = comma_after_next_concept;

         if (comma_after_next_concept == 3)
            comma_after_next_concept = 2;
         else
            comma_after_next_concept = request_comma_after_next_concept;

         if (comma_after_next_concept == 2 && next_cptr) {
            skipped_concept_info foo;
            foo.m_root_of_result_of_skip = (parse_block **) 0;

            if (check_for_concept_group(next_cptr, foo, false))
               comma_after_next_concept = 3;    // Will try again later.
         }

         local_cptr = next_cptr;

         if (k == concept_sequential) {
            if (request_final_space) writestuff(" ");
            print_recurse(local_cptr, PRINT_RECURSE_STAR);
            writestuff(")");
            break;
         }
         else if (k == concept_replace_nth_part ||
                  k == concept_replace_last_part ||
                  k == concept_interrupt_at_fraction) {
            if (request_final_space) writestuff(" ");
            print_recurse(local_cptr, PRINT_RECURSE_STAR);
            writestuff("]");
            break;
         }
      }
      else {
         // This is a "marker", so it has a call, perhaps with a selector and/or number.
         // The call may be null if we are printing a partially entered line.  Beware.

         parse_block *sub1_ptr;
         parse_block *sub2_ptr;
         parse_block *search;
         bool pending_subst1, pending_subst2;

         parse_block *save_cptr = local_cptr;

         bool subst1_in_use = false;
         bool subst2_in_use = false;

         if (request_final_space) writestuff(" ");

         if (k == concept_another_call_next_mod) {
            search = save_cptr->next;
            while (search) {
               parse_block *subsidiary_ptr = search->subsidiary_root;
               bool this_is_subst1 = false;
               bool this_is_subst2 = false;
               if (subsidiary_ptr) {
                  switch (search->replacement_key) {
                     case DFM1_CALL_MOD_ANYCALL/DFM1_CALL_MOD_BIT:
                     case DFM1_CALL_MOD_MAND_ANYCALL/DFM1_CALL_MOD_BIT:
                        this_is_subst1 = true;
                        break;
                     case DFM1_CALL_MOD_OR_SECONDARY/DFM1_CALL_MOD_BIT:
                     case DFM1_CALL_MOD_MAND_SECONDARY/DFM1_CALL_MOD_BIT:
                        this_is_subst2 = true;
                        break;
                  }

                  if (this_is_subst1) {
                     if (subst1_in_use) writestuff("ERROR!!!");
                     subst1_in_use = true;
                     sub1_ptr = subsidiary_ptr;
                  }

                  if (this_is_subst2) {
                     if (subst2_in_use) writestuff("ERROR!!!");
                     subst2_in_use = true;
                     sub2_ptr = subsidiary_ptr;
                  }
               }
               search = search->next;
            }
         }

         pending_subst1 = subst1_in_use;
         pending_subst2 = subst2_in_use;

         /* Now "subst_in_use" is on if there is a replacement call that goes in naturally.  During the
            scan of the name, we will try to fit that replacement into the name of the call as directed
            by atsign-escapes.  If we succeed at this, we will clear "pending_subst".
            In addition to all of this, there may be any number of forcible replacements. */

         /* Call = NIL means we are echoing input and user hasn't entered call yet. */

         direction_kind idirjunk = local_cptr->options.where;
         uint32_t number_list = local_cptr->options.number_fields;
         const call_with_name *localcall = local_cptr->call_to_print;

         if (localcall) {
            Cstring np;

            if (print_recurse_arg & PRINT_RECURSE_SELECTOR_SING)
               np = selector_list[local_cptr->options.who.who[m_selector_recursion_level]].sing_name;
            else if (print_recurse_arg & PRINT_RECURSE_SELECTOR)
               np = selector_list[local_cptr->options.who.who[m_selector_recursion_level]].name;
            else {
               np = localcall->name;
            }

            while (*np) {
               char c = *np++;

               if (c == '@') {
                  char savec = *np++;

                  switch (savec) {
                  case '6': case 'k': case 'K': case 'V':
                     write_blank_if_needed();
                     m_selector_recursion_level++;
                     print_recurse(local_cptr, savec == 'k' ? PRINT_RECURSE_SELECTOR_SING : PRINT_RECURSE_SELECTOR);
                     if (np[0] && np[0] != ' ' && np[0] != ']' && np[0] != '-')
                        writestuff(" ");

                     m_selector_recursion_level--;
                     break;
                  case 'v': case 'w': case 'x': case 'y':
                     write_blank_if_needed();
                     /* Find the base tag call that this is invoking. */

                     search = save_cptr->next;
                     while (search) {
                        parse_block *subsidiary_ptr = search->subsidiary_root;
                        if (subsidiary_ptr &&
                            subsidiary_ptr->call_to_print &&
                            (subsidiary_ptr->call_to_print->the_defn.callflags1 & CFLAG1_BASE_TAG_CALL_MASK)) {
                           print_recurse(subsidiary_ptr, 0);
                           goto did_tagger;
                        }
                        search = search->next;
                     }

                     /* We didn't find the tagger.  It must not have been entered into
                           the parse tree.  See if we can get it from the "tagger" field. */

                     if (save_cptr->options.tagger != 0)
                        writestuff(tagger_calls
                                   [save_cptr->options.tagger >> 5]
                                   [(save_cptr->options.tagger & 0x1F)-1]->menu_name);
                     else
                        writestuff("NO TAGGER???");

                  did_tagger:

                     if (np[0] && np[0] != ' ' && np[0] != ']')
                        writestuff(" ");
                     break;
                  case 'N':
                     write_blank_if_needed();

                     /* Find the base circ call that this is invoking. */

                     search = save_cptr->next;
                     while (search) {
                        parse_block *subsidiary_ptr = search->subsidiary_root;
                        if (subsidiary_ptr &&
                            subsidiary_ptr->call_to_print &&
                            (subsidiary_ptr->call_to_print->the_defn.callflags1 &
                             CFLAG1_BASE_CIRC_CALL)) {
                           print_recurse(subsidiary_ptr, PRINT_RECURSE_CIRC);
                           goto did_circcer;
                        }
                        search = search->next;
                     }

                     /* We didn't find the circcer.  It must not have been entered into the parse tree.
                           See if we can get it from the "circcer" field. */

                     if (save_cptr->options.circcer > 0)
                        writestuff(circcer_calls[(save_cptr->options.circcer)-1].the_circcer->menu_name);
                     else
                        writestuff("NO CIRCCER???");

                  did_circcer:

                     if (np[0] && np[0] != ' ' && np[0] != ']')
                        writestuff(" ");
                     break;
                  case 'h':                   // Need to plug in a direction.
                     write_blank_if_needed();
                     writestuff(direction_names[idirjunk].name);
                     if (np[0] && np[0] != ' ' && np[0] != ']')
                        writestuff(" ");
                     break;
                  case '9': case 'a': case 'b': case 'B': case 'D': case 'u':
                     // Need to plug in a number.
                     write_blank_if_needed();

                     // Watch for "zero and a half".
                     if (savec == '9' && (number_list & NUMBER_FIELD_MASK) == 0 &&
                         np[0] == '-' && np[1] == '1' &&
                         np[2] == '/' && np[3] == '2') {
                        writestuff("1/2");
                        np += 4;
                     }
                     else
                        write_nice_number(savec, number_list & NUMBER_FIELD_MASK);

                     number_list >>= BITS_PER_NUMBER_FIELD;    // Get ready for next number.
                     break;
                  case 'e':
                     if (use_left_name) {
                        while (*np != '@') np++;
                        if (m_writechar_block.lastchar == ']') writestuff(" ");
                        writestuff("left");
                        np += 2;
                     }
                     break;
                  case 'j':
                     if (!use_cross_name) {
                        while (*np != '@') np++;
                        np += 2;
                     }
                     break;
                  case 'C':
                     if (use_cross_name) {
                        write_blank_if_needed();
                        writestuff("cross");
                     }
                     break;
                  case 'S':                   // Look for star turn replacement.
                     if (save_cptr->options.star_turn_option < 0) {
                        writestuff(", don't turn the star");
                     }
                     else if (save_cptr->options.star_turn_option != 0) {
                        writestuff(", turn the star ");
                        write_nice_number('b', save_cptr->options.star_turn_option & 0xF);
                     }
                     break;
                  case 'Q':
                     if (!use_grand_name) {
                        while (*np != '@') np++;
                        np += 2;
                     }
                     break;
                  case 'G':
                     if (use_grand_name) {
                        if (m_writechar_block.lastchar != ' ' &&
                            m_writechar_block.lastchar != '[') writechar(' ');
                        writestuff("grand");
                     }
                     break;
                  case 'J':
                     if (!use_magic_name) {
                        while (*np != '@') np++;
                        np += 2;
                     }
                     break;
                  case 'M':
                     if (use_magic_name) {
                        if (m_writechar_block.lastchar != ' ' &&
                            m_writechar_block.lastchar != '[') writechar(' ');
                        writestuff("magic");
                     }
                     break;
                  case 'E':
                     if (!use_intlk_name) {
                        while (*np != '@') np++;
                        np += 2;
                     }
                     break;
                  case 'I':
                     if (use_intlk_name) {
                        if (m_writechar_block.lastchar == 'a' &&
                            m_writechar_block.lastlastchar == ' ')
                           writestuff("n ");
                        else if (m_writechar_block.lastchar != ' ' &&
                                 m_writechar_block.lastchar != '[')
                           writechar(' ');
                        writestuff("interlocked");
                     }
                     break;
                  case 'l': case 'L': case 'R': case 'F': case '8': case 'o':
                     // Just skip these -- they end stuff that we could have
                     // elided but didn't.
                     break;
                  case 'n': case 'p': case 'r': case 'm': case 't':
                     if (subst2_in_use) {
                        if (savec == 'p' || savec == 'r') {
                           while (*np != '@') np++;
                           np += 2;
                        }
                     }
                     else {
                        if (savec == 'n') {
                           while (*np != '@') np++;
                           np += 2;
                        }
                     }

                     if (savec != 'p' && savec != 'n') {
                        if (pending_subst2) {
                           write_blank_if_needed();
                           writestuff("[");
                           print_recurse(sub2_ptr, PRINT_RECURSE_STAR);
                           writestuff("]");
                           pending_subst2 = false;
                        }
                        else if (savec == 'm' && !m_leave_missing_calls_blank) {
                           write_blank_if_needed();
                           writestuff("[???]");
                        }
                     }

                     break;
                  case 'O':
                     if (print_recurse_arg & PRINT_RECURSE_CIRC) {
                        while (*np != '@') np++;
                        np += 2;
                     }

                     break;
                  default:
                     if (subst1_in_use) {
                        if (savec == '2' || savec == '4') {
                           while (*np != '@') np++;
                           np += 2;
                        }
                     }
                     else {
                        if (savec == '7') {
                           while (*np != '@') np++;
                           np += 2;
                        }
                     }

                     if (savec != '4' && savec != '7') {
                        if (pending_subst1) {
                           write_blank_if_needed();
                           writestuff("[");
                           print_recurse(sub1_ptr, PRINT_RECURSE_STAR);
                           writestuff("]");
                           if (savec == 'T') writestuff(" er's");
                           pending_subst1 = false;
                        }
                        else if (savec == '0' && !m_leave_missing_calls_blank) {
                           write_blank_if_needed();
                           writestuff("[???]");
                        }
                     }

                     break;
                  }
               }
               else {
                  if (m_writechar_block.lastchar == ']' && c != ' ' && c != ']')
                     writestuff(" ");

                  if ((m_writechar_block.lastchar != ' ' && m_writechar_block.lastchar != '[') || c != ' ') writechar(c);
               }
            }

            if (m_writechar_block.lastchar == ']' && *np && *np != ' ' && *np != ']')
               writestuff(" ");
         }
         else if (print_recurse_arg & PRINT_RECURSE_STAR) {
            writestuff("*");
         }

         /* Now if "pending_subst" is still on, we have to do by hand what should have been
            a natural replacement.  In any case, we have to find forcible replacements and
            report them. */

         if (k == concept_another_call_next_mod &&
             ((print_recurse_arg & (PRINT_RECURSE_SELECTOR_SING | PRINT_RECURSE_SELECTOR)) == 0)) {

            int first_replace = 0;

            for (search = save_cptr->next ; search ; search = search->next) {
               const call_with_name *cc;
               parse_block *subsidiary_ptr = search->subsidiary_root;

               /* If we have a subsidiary_ptr, handle the replacement that is indicated.
                  BUT:  if the call shown in the subsidiary_ptr is a base tag call, don't
                  do anything -- such substitutions were already taken care of.
                     BUT:  only check if there is actually a call there. */

               if (!subsidiary_ptr) continue;
               cc = subsidiary_ptr->call_to_print;

               if (!cc ||    /* If no call pointer, it isn't a tag base call. */
                   (!(cc->the_defn.callflags1 & CFLAG1_BASE_TAG_CALL_MASK) &&
                    (!(cc->the_defn.callflags1 & CFLAG1_BASE_CIRC_CALL) ||
                     search->call_to_print != base_calls[base_call_circcer]))) {
                  call_with_name *replaced_call = search->call_to_print;

                  /* Need to check for case of replacing one star turn with another. */

                  if ((first_replace == 0) &&
                      (replaced_call->the_defn.callflagsf & CFLAG2_IS_STAR_CALL) &&
                      ((subsidiary_ptr->concept->kind == marker_end_of_list) ||
                       subsidiary_ptr->concept->kind == concept_another_call_next_mod) &&
                      cc &&
                      ((cc->the_defn.callflagsf & CFLAG2_IS_STAR_CALL) ||
                       cc->the_defn.schema == schema_nothing)) {
                     first_replace++;

                     if (cc->the_defn.schema == schema_nothing)
                        writestuff(", don't turn the star");
                     else {
                        writestuff(", ");
                        print_recurse(subsidiary_ptr, PRINT_RECURSE_STAR);
                     }
                  }
                  else {
                     switch (search->replacement_key) {
                     case DFM1_CALL_MOD_ANYCALL/DFM1_CALL_MOD_BIT:
                     case DFM1_CALL_MOD_MAND_ANYCALL/DFM1_CALL_MOD_BIT:
                     case DFM1_CALL_MOD_ALLOW_PLAIN_MOD/DFM1_CALL_MOD_BIT:
                        // This is a natural replacement.
                        // It may already have been taken care of.
                        if (pending_subst1 ||
                            search->replacement_key ==
                            DFM1_CALL_MOD_ALLOW_PLAIN_MOD/DFM1_CALL_MOD_BIT) {
                           write_blank_if_needed();
                           if (search->replacement_key ==
                               DFM1_CALL_MOD_ALLOW_PLAIN_MOD/DFM1_CALL_MOD_BIT)
                              writestuff("but [");
                           else
                              writestuff("[modification: ");
                           print_recurse(subsidiary_ptr, PRINT_RECURSE_STAR);
                           writestuff("]");
                        }
                        break;
                     case DFM1_CALL_MOD_OR_SECONDARY/DFM1_CALL_MOD_BIT:
                     case DFM1_CALL_MOD_MAND_SECONDARY/DFM1_CALL_MOD_BIT:
                        // This is a secondary replacement.
                        // It may already have been taken care of.
                        if (pending_subst2) {
                           write_blank_if_needed();
                           writestuff("[modification: ");
                           print_recurse(subsidiary_ptr, PRINT_RECURSE_STAR);
                           writestuff("]");
                        }
                        break;
                     default:
                        /* This is a forced replacement. */
                        write_blank_if_needed();
                        first_replace++;
                        if (first_replace == 1)
                           writestuff("BUT REPLACE ");
                        else
                           writestuff("AND REPLACE ");

                        /* Star turn calls can have funny names like "nobox". */

                        writestuff_with_decorations(
                           &search->options,
                           (replaced_call->the_defn.callflagsf & CFLAG2_IS_STAR_CALL) ?
                           "turn the star @b" : replaced_call->name, false);

                        writestuff(" WITH [");
                        print_recurse(subsidiary_ptr, PRINT_RECURSE_STAR);
                        writestuff("]");
                        break;
                     }
                  }
               }
            }
         }

         break;
      }
   }

   if (deferred_concept) {
      if (deferred_concept_paren & 1) writestuff(")");
      writestuff(" ");

      if (deferred_concept->concept->kind == concept_n_times_const &&
          deferred_concept->concept->arg2 == 3)
         writestuff("3 TIMES");
      else
         writestuff_with_decorations(&deferred_concept->options,
                                     deferred_concept->concept->name, true);
      if (deferred_concept_paren & 2) writestuff(")");
   }

   return;
}

static char current_line[MAX_TEXT_LINE_LENGTH];

void ui_utils::open_text_line()
{
   m_writechar_block.destcurr = current_line;
   m_writechar_block.lastchar = ' ';
   m_writechar_block.lastlastchar = ' ';
   m_writechar_block.lastblank = (char *) 0;
}

void ui_utils::clear_screen()
{
   written_history_items = -1;
   text_line_count = 0;

   iob88.reduce_line_count(0);
   open_text_line();
}

void ui_utils::write_header_stuff(bool with_ui_version, uint32_t act_phan_flags)
{
   if (!ui_options.diagnostic_mode) {
      // Log creation version info.
      if (with_ui_version) {     // This is the "pretty" form that we display while running.
         writestuff("Sd ");
         writestuff(sd_version_string());
         writestuff(" : db");
         writestuff(database_version);
         writestuff(" : ui");
         writestuff(iob88.version_string());
      }
      else {                     // This is the "compact" form that goes into the file.
         writestuff("Sd");
         writestuff(sd_version_string());
         writestuff(":db");
         writestuff(database_version);
      }
   }

   if (act_phan_flags & RESULTFLAG__ACTIVE_PHANTOMS_ON) {
      if (act_phan_flags & RESULTFLAG__ACTIVE_PHANTOMS_OFF)
         writestuff(" (AP-)");
      else
         writestuff(" (AP)");
   }

   writestuff("     ");

   // Log level info.
   writestuff(getout_strings[calling_level]);

   if (glob_abridge_mode == abridge_mode_abridging)
      writestuff(" (abridged)");
}


void ui_utils::writechar(char src)
{
   // Don't write two consecutive commas.
   if (src == ',' && src == m_writechar_block.lastchar) return;

   m_writechar_block.lastlastchar = m_writechar_block.lastchar;

   *m_writechar_block.destcurr = m_writechar_block.lastchar = src;
   if (src == ' ' && m_writechar_block.destcurr != current_line)
      m_writechar_block.lastblank = m_writechar_block.destcurr;

   // If drawing a picture, don't do automatic line breaks.

   if (m_writechar_block.destcurr < &current_line[ui_options.max_print_length] ||
       m_writechar_block.usurping_writechar ||
       ui_options.drawing_picture)
      m_writechar_block.destcurr++;
   else {
      // Line overflow.  Try to write everything up to the last
      // blank, then fill next line with everything since that blank.

      char save_buffer[MAX_TEXT_LINE_LENGTH];
      char *q = save_buffer;
      char *p = m_writechar_block.lastblank+1;

      // If we are after a blank, see if we are after a whole bunch of blanks; delete same.
      // This could happen if the only blanks are the indentation at the start of the line.
      // In that case, strip it all away.
      while (m_writechar_block.lastblank && m_writechar_block.lastblank > current_line && m_writechar_block.lastblank[-1] == ' ')
         m_writechar_block.lastblank--;

      // If there is nonblank text before the last blank, write it out and save
      // whatever comes next for the next round.  But if the only blanks are those
      // at the beginning of the line, treat is as though there weren't any blanks
      // at all--break a word.
      if (m_writechar_block.lastblank && m_writechar_block.lastblank > current_line) {
         while (p <= m_writechar_block.destcurr) *q++ = *p++;
         m_writechar_block.destcurr = m_writechar_block.lastblank;
      }
      else {
         // Must break a word.  Save just the final character that we couldn't fit.
         *q++ = *m_writechar_block.destcurr;
      }

      *q = '\0';

      newline();           // End the current buffer and write it out.
      writestuff("   ");   // Make a new line, copying saved stuff into it.
      writestuff(save_buffer);
   }
}

void ui_utils::newline()
{
   /* Erase any trailing blanks.  Failure to do so can lead to some
      incredibly obscure bugs when some editors on PC's try to "fix"
      the file, ultimately leading to apparent loss of entire sequences.
      The problem was that the editor took out the trailing blanks
      and moved the rest of the text downward, but didn't reduce the
      file's byte count.  Control Z's were thereby introduced at the
      end of the file.  Subsequent runs of Sd appended new sequences
      to the file, leaving the ^Z's intact.  (Making Sd look for ^Z's
      at the end of a file it is about to append to, and remove same,
      is very hard.)  It turns out that some printing software stops,
      as though it reached the end of the file, when it encounters a
      ^Z, so the appended sequences were effectively lost. */
   while (m_writechar_block.destcurr != current_line && m_writechar_block.destcurr[-1] == ' ')
      m_writechar_block.destcurr--;

   *m_writechar_block.destcurr = '\0';

   // There will be no special "5" or "6" characters in pictures
   // (ui_options.drawing_picture&1) if:
   //
   // Enable_file_writing is on (we don't write the special stuff to a file,
   //    of course, and we don't even write it to the transcript when showing the
   //    final card)
   //
   // Use_escapes_for_drawing_people is 0 or 1 (so we don't do it for Sdtty under
   //    DJGPP or GCC (0), or for Sdtty under Windows (1))
   //
   // No_graphics != 0 (so we only do it if showing full checkers in Sd).

   if (enable_file_writing)
      write_file(current_line);

   text_line_count++;
   iob88.add_new_line(current_line,
      enable_file_writing ?
                    0 :
                    (ui_options.drawing_picture | (ui_options.squeeze_this_newline << 1)));
   open_text_line();
}





void ui_utils::doublespace_file()
{
   write_file("");
}


void ui_utils::writestuff(const char *s)
{
   while (*s) writechar(*s++);
}

void ui_utils::show_match_item()
{
   if (matcher_p->m_final_result.indent) writestuff("   ");
   writestuff(matcher_p->m_user_input);
   writestuff(matcher_p->m_full_extension);
   newline();
}


parse_block *parse_block::parse_active_list = (parse_block *) 0;
parse_block *parse_block::parse_inactive_list = (parse_block *) 0;

void parse_block::initialize(const concept_descriptor *cc)
{
   more_finalherit_flags.clear_all_herit_and_final_bits();
   concept = cc;
   call = (call_with_name *) 0;
   call_to_print = (call_with_name *) 0;
   options.initialize();
   replacement_key = 0;
   no_check_call_level = false;
   say_and = false;
   subsidiary_root = (parse_block *) 0;
   next = (parse_block *) 0;
}



void release_parse_blocks_to_mark(parse_block *mark_point)
{
   while (parse_block::parse_active_list && parse_block::parse_active_list != mark_point) {
      parse_block *item = parse_block::parse_active_list;

      parse_block::parse_active_list = item->gc_ptr;
      item->gc_ptr = parse_block::parse_inactive_list;
      parse_block::parse_inactive_list = item;

      // Clear pointers so we will notice if it gets erroneously re-used.
      item->initialize((concept_descriptor *) 0);
   }
}


void parse_block::final_cleanup()
{
   parse_block *item;

   while ((item = parse_active_list)) {
      parse_active_list = item->gc_ptr;
      delete item;
   }

   while ((item = parse_inactive_list)) {
      parse_inactive_list = item->gc_ptr;
      delete item;
   }
}


extern parse_block *copy_parse_tree(parse_block *original_tree)
{
   parse_block *new_item, *new_root;

   if (!original_tree) return NULL;

   new_item = get_parse_block();
   new_root = new_item;

   for (;;) {
      new_item->concept = original_tree->concept;
      new_item->call = original_tree->call;
      new_item->call_to_print = original_tree->call_to_print;
      new_item->options = original_tree->options;
      new_item->replacement_key = original_tree->replacement_key;
      new_item->no_check_call_level = original_tree->no_check_call_level;

      if (original_tree->subsidiary_root)
         new_item->subsidiary_root = copy_parse_tree(original_tree->subsidiary_root);

      if (!original_tree->next) break;

      new_item->next = get_parse_block();
      new_item = new_item->next;
      original_tree = original_tree->next;
   }

   return new_root;
}


/* Stuff for saving parse state while we resolve. */

static parse_state_type saved_parse_state;
static parse_block *saved_command_root;


SDLIB_API extern void reset_parse_tree(parse_block *original_tree, parse_block *final_head)
{
   parse_block *new_item = final_head;
   parse_block *old_item = original_tree;

   for (;;) {
      if (!new_item || !old_item) crash_print(__FILE__, __LINE__, 0, (setup *) 0);
      new_item->concept = old_item->concept;
      new_item->call = old_item->call;
      new_item->call_to_print = old_item->call_to_print;
      new_item->options = old_item->options;

      // Chop off branches that don't belong.

      if (!old_item->subsidiary_root)
         new_item->subsidiary_root = (parse_block *) 0;
      else
         reset_parse_tree(old_item->subsidiary_root, new_item->subsidiary_root);

      if (!old_item->next) {
         new_item->next = 0;
         break;
      }

      new_item = new_item->next;
      old_item = old_item->next;
   }
}


/* Save the entire parse stack, and make a copy of the dynamic blocks that
   comprise the parse state. */
extern void save_parse_state()
{
   saved_parse_state = parse_state;
   saved_command_root = copy_parse_tree(configuration::next_config().command_root);
}


/* Restore the parse state.  We write directly over the original dynamic blocks
   of the current parse state (making use of the fact that the alterations that
   could have happened will add to the tree but never delete anything.)  This way,
   after we have saved and restored things, they are all in their original,
   locations, so that the pointers in the parse stack will still be valid. */
extern void restore_parse_state()
{
   parse_state = saved_parse_state;

   if (saved_command_root)
      reset_parse_tree(saved_command_root, configuration::next_config().command_root);
   else
      configuration::next_config().command_root = 0;
}


void randomize_couple_colors()
{
   color_randomizer[0] = 0;
   color_randomizer[1] = 1;
   color_randomizer[2] = 2;
   color_randomizer[3] = 3;

   int i = generate_random_number(4);
   int t = color_randomizer[0];
   color_randomizer[0] = color_randomizer[i];
   color_randomizer[i] = t;

   i = generate_random_number(3)+1;
   t = color_randomizer[1];
   color_randomizer[1] = color_randomizer[i];
   color_randomizer[i] = t;

   i = generate_random_number(2)+2;
   t = color_randomizer[2];
   color_randomizer[2] = color_randomizer[i];
   color_randomizer[i] = t;
}


void string_copy(char **dest, Cstring src)
{
   Cstring f = src;
   char *t = *dest;

   while ((*t++ = *f++));
   *dest = t-1;
}



/* Clear the screen and display the initial part of the history.
   This attempts to re-use material already displayed on the screen.
   The "num_pics" argument tells how many of the last history items
   are to have pictures forced, so we can tell exactly what items
   have pictures. */
void ui_utils::display_initial_history(int upper_limit, int num_pics)
{
   int j, startpoint;

   // See if we can re-use some of the safely written history.
   // First, cut down overly optimistic estimates.
   if (written_history_items > upper_limit) written_history_items = upper_limit;
   // And normalize the "nopic" number.
   if (written_history_nopic > written_history_items) written_history_nopic = written_history_items;

   // Check whether pictures are faithful.  If any item in the written history has its
   // "picture forced" property (as determined by written_history_nopic)
   // different from what we want that property to be (as determined by upper_limit-num_pics),
   // and that item didn't have the draw_pic flag on, that item needs to be rewritten.
   // We cut written_history_items down to below that item if such is the case.

   for (j=1; j<=written_history_items; j++) {
      int t = ((int) ((unsigned int) (written_history_nopic-j)) ^
                 ((unsigned int) (upper_limit-num_pics-j)));
      if (t < 0 && !configuration::history[j].draw_pic) {
         written_history_items = j-1;
         break;
      }
   }

   if (written_history_items > 0) {
      // We win.  Back up the text line count to the right place, and rewrite the rest.

      text_line_count = configuration::history[written_history_items].text_line;
      iob88.reduce_line_count(text_line_count);
      open_text_line();
      startpoint = written_history_items+1;
   }
   else {
      // We lose, there is nothing we can use.
      if (no_erase_before_this != 0)
         iob88.no_erase_before_n(no_erase_before_this);

      clear_screen();
      write_header_stuff(true, 0);
      newline();
      newline();
      startpoint = 1;
   }

   for (j=startpoint; j<=upper_limit-num_pics; j++)
      write_history_line(j, false, false, file_write_no);

   // Now write stuff with forced pictures.

   for (j=upper_limit-num_pics+1; j<=upper_limit; j++) {
      if (j >= startpoint) write_history_line(j, true, false, file_write_no);
   }

   written_history_items = upper_limit;   // This stuff is now safe.
   written_history_nopic = written_history_items-num_pics;
   no_erase_before_this = 0;
}



extern void initialize_parse()
{
   parse_state.concept_write_base = &configuration::next_config().command_root;
   parse_state.concept_write_ptr = parse_state.concept_write_base;
   *(parse_state.concept_write_ptr) = (parse_block *) 0;
   parse_state.parse_stack_index = 0;
   parse_state.base_call_list_to_use = find_proper_call_list(&configuration::current_config().state);
   parse_state.call_list_to_use = parse_state.base_call_list_to_use;
   configuration::next_config().init_centersp_specific();
   configuration::init_warnings();
   configuration::next_config().draw_pic = false;
   configuration::next_config().state_is_valid = false;

   if (written_history_items > config_history_ptr)
      written_history_items = config_history_ptr;

   parse_state.specialprompt[0] = '\0';
   parse_state.topcallflags1 = 0;
}




void ui_utils::do_change_outfile(bool signal)
{
   char newfile_string[MAX_FILENAME_LENGTH];
   char buffer[MAX_TEXT_LINE_LENGTH];
   sprintf(buffer, "Current sequence output file is \"%s\".", outfile_string);

   if (iob88.get_popup_string(buffer,
                              "*Enter new name (or '+' to base it on today's date)",
                              "Enter new file name (or '+' to base it on today's date):",
                              outfile_string, newfile_string) == POPUP_ACCEPT_WITH_STRING && newfile_string[0]) {
      char confirm_message[MAX_FILENAME_LENGTH+25];
      const char *final_message;

      if (install_outfile_string(newfile_string)) {
         strncpy(confirm_message, "Output file changed to \"", 25);
         strncat(confirm_message, outfile_string, MAX_FILENAME_LENGTH);
         strncat(confirm_message, "\"", 2);
         final_message = confirm_message;
      }
      else
         final_message = "No write access to that file, no action taken.";

      if (signal) {
         specialfail(final_message);
      }
      else {
         writestuff(final_message);
         newline();
      }
   }
}


void ui_utils::do_change_outprefix(bool signal)
{
   char newprefix_string[MAX_FILENAME_LENGTH];
   char buffer[MAX_TEXT_LINE_LENGTH];
   sprintf(buffer, "Current sequence output prefix is \"%s\".", outfile_prefix);

   if (iob88.get_popup_string(buffer,
                              "*Enter new prefix",
                              "Enter new prefix:",
                              outfile_prefix, newprefix_string) == POPUP_DECLINE)
      return;

   char confirm_message[MAX_FILENAME_LENGTH+25];

   strncpy(outfile_prefix, newprefix_string, MAX_FILENAME_LENGTH);
   strncpy(confirm_message, "Output prefix changed to \"", 27);
   strncat(confirm_message, outfile_prefix, MAX_FILENAME_LENGTH);
   strncat(confirm_message, "\"", 2);

   if (signal) {
      specialfail(confirm_message);
   }
   else {
      writestuff(confirm_message);
      newline();
   }
}


namespace {

// Returns TRUE if it successfully backed up one parse block.
bool backup_one_item()
{

   // User wants to undo a call.  The concept parse list is not set up
   // for easy backup, so we search forward from the beginning.

   parse_block **this_ptr = parse_state.concept_write_base;
   if (!this_ptr || !*this_ptr) return false;

   if ((config_history_ptr == 1) && configuration::history[1].get_startinfo_specific()->into_the_middle)
      this_ptr = &((*this_ptr)->next);

   for (;;) {
      parse_block **last_ptr;

      if (!*this_ptr) break;
      last_ptr = this_ptr;
      this_ptr = &((*this_ptr)->next);

      if (this_ptr == parse_state.concept_write_ptr) {
         parse_state.concept_write_ptr = last_ptr;

         // See whether we need to destroy a frame in the parse stack.
         if (parse_state.parse_stack_index != 0 &&
               parse_state.parse_stack[parse_state.parse_stack_index-1].concept_write_save_ptr == last_ptr)
            parse_state.parse_stack_index--;

         *last_ptr = (parse_block *) 0;
         return true;
      }

      if ((*last_ptr)->concept->kind <= marker_end_of_list) break;
   }

   // We did not find our place.

   return false;
}

}   // End blank namespace.

// Returns true if sequence was written.
bool ui_utils::write_sequence_to_file() THROW_DECL
{
   char date[MAX_TEXT_LINE_LENGTH];
   char second_header[MAX_TEXT_LINE_LENGTH];
   char seqstring[20];
   int j;

   // Put up the getout popup to see if the user wants to enter a header string.

   popup_return getout_ind;

   if (header_comment[0]) {
      char buffer[MAX_TEXT_LINE_LENGTH+MAX_FILENAME_LENGTH];
      sprintf(buffer, "Session title is \"%s\".", header_comment);
      getout_ind = iob88.get_popup_string(buffer,
                                          "You can give an additional comment for just this sequence.",
                                          "Enter comment:", "", second_header);
   }
   else {
      getout_ind = iob88.get_popup_string("",
                                          "Type comment for this sequence, if desired.",
                                          "Enter comment:", "", second_header);
   }

   second_header[MAX_TEXT_LINE_LENGTH-1] = 0;

   // Some user interfaces (those with buttons or icons) may have a button to abort the
   // sequence, rather than just decline the comment.  Such an action comes back as
   // "POPUP_DECLINE".  Otherwise, we get POPUP_ACCEPT_WITH_STRING if a nonempty string
   // was given, or POPUP_ACCEPT if the string was empty.

   if (getout_ind == POPUP_DECLINE)
      return false;    // User didn't want to end this sequence after all.
   else if (getout_ind != POPUP_ACCEPT_WITH_STRING) second_header[0] = '\0';

   // Open the file and write it.

   clear_screen();
   open_file();
   enable_file_writing = true;
   doublespace_file();
   get_date(date);
   writestuff(date);
   writestuff("     ");
   write_header_stuff(false, configuration::current_config().state.result_flags.misc);
   newline();

   if (!configuration::sequence_is_resolved()) {
      writestuff("             NOT RESOLVED");
      newline();
   }

   if (ui_options.singing_call_mode != 0) {
      writestuff(
         (ui_options.singing_call_mode == 2) ?
         "             Singing call reverse progression" :
         "             Singing call progression");
      newline();
   }

   // Write header comment, if it exists.

   if (header_comment[0]) {
      writestuff("             ");
      writestuff(header_comment);
   }

   if (sequence_number >= 0) {
      (void) sprintf(seqstring, "%d", sequence_number);
      writestuff("   ");
      writestuff(seqstring);
   }

   // Write secondary header comment, if it exists.

   if (second_header[0]) {
      writestuff("       ");
      writestuff(second_header);
   }

   if (header_comment[0] || second_header[0] || sequence_number >= 0) newline();

   newline();

   if (sequence_number >= 0) sequence_number++;

   for (j=configuration::whole_sequence_low_lim; j<=config_history_ptr; j++)
      write_history_line(j, false, false, file_write_double);

   // Echo the concepts entered so far.

   if (parse_state.concept_write_ptr != &configuration::next_config().command_root) {
      write_history_line(config_history_ptr+1, false, false, file_write_double);
   }

   if (configuration::sequence_is_resolved())
      write_resolve_text(true);

   newline();
   enable_file_writing = false;
   newline();

   close_file();     // This will signal a "specialfail" if a file error occurs.

   writestuff("Sequence");

   if (sequence_number >= 0) {
      writestuff(" ");
      writestuff(seqstring);
   }

   writestuff(" written to \"");
   writestuff(outfile_prefix);
   writestuff(outfile_string);
   writestuff("\".");
   newline();

   wrote_a_sequence = true;
   return true;
}

namespace {

uint32_t id_fixer_array[16] = {
   07777525252, 07777454545, 07777313131, 07777262626,
   07777522525, 07777453232, 07777314646, 07777265151,
   07777255225, 07777324532, 07777463146, 07777512651,
   07777252552, 07777323245, 07777464631, 07777515126};


selector_kind translate_selector_permutation1(uint32_t x)
{
   switch (x & 077) {
   case 01: return selector_sidecorners;
   case 02: return selector_headcorners;
   case 04: return selector_girls;
   case 010: return selector_boys;
   case 020: return selector_sides;
   case 040: return selector_heads;
   default: return selector_uninitialized;
   }
}


selector_kind translate_selector_permutation2(uint32_t x)
{
   switch (x & 07) {
   case 04: return selector_headboys;
   case 05: return selector_headgirls;
   case 06: return selector_sideboys;
   case 07: return selector_sidegirls;
   default: return selector_uninitialized;
   }
}

}   // End blank namespace.


// Returned value with "2" bit on means error occurred and could not translate.
// Selectors are messed up.  Should only occur if in unsymmetrical formation
// in which it can't figure out what is going on.
// Or if we get "end boys" or something like that, that we can't handle yet.
// Otherwise, "1" bit says at
// least one selector changed.  Zero means nothing changed.

extern uint32_t translate_selector_fields(parse_block *xx, uint32_t mask)
{
   selector_kind z;
   uint32_t retval = 0;

   for ( ; xx ; xx=xx->next) {
      switch (xx->options.who.who[0]) {
      case selector_heads:
         z = translate_selector_permutation1(mask >> 13);
         break;
      case selector_sides:
         z = translate_selector_permutation1(mask >> 13);
         z = selector_list[z].opposite;
         break;
      case selector_boys:
         z = translate_selector_permutation1(mask >> 7);
         break;
      case selector_girls:
         z = translate_selector_permutation1(mask >> 7);
         z = selector_list[z].opposite;
         break;
      case selector_headcorners:
         z = translate_selector_permutation1(mask >> 1);
         break;
      case selector_sidecorners:
         z = translate_selector_permutation1(mask >> 1);
         z = selector_list[z].opposite;
         break;

      case selector_end_boys:
         if (((mask >> 7) & 077) == 010) z = selector_end_boys;
         else if (((mask >> 7) & 077) == 04) z = selector_end_girls;
         else z = selector_uninitialized;
         break;
      case selector_end_girls:
         if (((mask >> 7) & 077) == 010) z = selector_end_girls;
         else if (((mask >> 7) & 077) == 04) z = selector_end_boys;
         else z = selector_uninitialized;
         break;
      case selector_center_boys:
         if (((mask >> 7) & 077) == 010) z = selector_center_boys;
         else if (((mask >> 7) & 077) == 04) z = selector_center_girls;
         else z = selector_uninitialized;
         break;
      case selector_center_girls:
         if (((mask >> 7) & 077) == 010) z = selector_center_girls;
         else if (((mask >> 7) & 077) == 04) z = selector_center_boys;
         else z = selector_uninitialized;
         break;

      case selector_headliners:
         if (mask & 1) z = selector_sideliners;
         break;
      case selector_sideliners:
         if (mask & 1) z = selector_headliners;
         break;
      case selector_headboys:
         z = translate_selector_permutation2(mask >> 19);
         break;
      case selector_headgirls:
         z = translate_selector_permutation2(mask >> 22);
         break;
      case selector_sideboys:
         z = translate_selector_permutation2(mask >> 25);
         break;
      case selector_sidegirls:
         z = translate_selector_permutation2(mask >> 28);
         break;
      default: goto nofix;
      }

      if (z == selector_uninitialized) retval = 2;   // Raise error.
      if (z != xx->options.who.who[0]) retval |= 1;         // Note that we changed something.
      xx->options.who.who[0] = z;

   nofix:

      retval |= translate_selector_fields(xx->subsidiary_root, mask);
   }

   return retval;
}

// This alters the parse tree in configuration::next_config().command_root.
extern bool fix_up_call_for_fidelity_test(const setup *old, const setup *nuu, uint32_t &global_status)
{
   // If the setup, population, and facing directions don't match, the
   // call execution is problematical.  We don't translate selectors.
   // The operator is responsible for what happens.

   if (nuu->kind != old->kind) {
      global_status |= 4;
      return true;
   }

   uint32_t mask = 0777777;
   uint32_t directions1 = 0;
   uint32_t directions2 = 0;
   uint32_t livemask1 = 0;
   uint32_t livemask2 = 0;

   // Find out whether the formations agree, and gather the information
   // that we need to translate the selectors.

   for (int i=0; i<=attr::slimit(old); i++) {
      uint32_t q = old->people[i].id1;
      uint32_t p = nuu->people[i].id1;
      uint32_t oldmask = mask;
      uint32_t a = (p >> 6) & 3;
      uint32_t b = (q >> 6) & 3;

      livemask1 <<= 1;
      livemask2 <<= 1;
      if (p) livemask1 |= 1;
      if (q) livemask2 |= 1;
      directions1 = (directions1<<2) | (p&3);
      directions2 = (directions2<<2) | (q&3);

      if ((p | q) == 0) continue;

      mask |= (b|4) << (a*3 + 18);
      oldmask ^= mask;     // The bits that changed.
      // Demand that, if anything changed at all, some new field got
      // set.  This has the effect of demanding that existing fields
      // never change, and that only new fields are created or existing
      // fields are rewritten with their original data.
      if (oldmask != 0 && (mask & 04444000000) == 0)
         mask |= 07777000000;  // Raise error.
      mask &= id_fixer_array[(a<<2) | b];
   }

   if (directions1 != directions2 || livemask1 != livemask2) {
      global_status |= 4;
      return true;
   }

   // Everything matches.  Translate the selectors.

   // If error happened, be sure everyone knows about it.
   if ((mask & 07777000000) == 07777000000) mask &= ~07777000000;

   global_status |=
      translate_selector_fields(configuration::next_config().command_root,
                                (mask << 1) | ((nuu->rotation ^ old->rotation) & 1));

   return false;
}





popup_return ui_utils::do_header_popup(char *dest)
{
   char myPrompt[MAX_TEXT_LINE_LENGTH];

   if (header_comment[0])
      sprintf(myPrompt, "Current title is \"%s\".", header_comment);
   else
      myPrompt[0] = 0;

   return iob88.get_popup_string(myPrompt, "*Enter new title:", "Enter new title:", "", dest);
}


void ui_utils::run_program(iobase & ggg)
{
   no_erase_before_this = 0;
   global_error_flag = (error_flag_type) 0;
   interactivity = interactivity_normal;
   clear_screen();

   if (!ui_options.diagnostic_mode) {
      writestuff("SD -- square dance caller's helper.");
      newline();
      newline();
      writestuff("Copyright (c) 1990-2022 William B. Ackerman");
      newline();
      writestuff("   and Stephen Gildea.");
      newline();
      writestuff("Copyright (c) 1992-1993 Alan Snyder");
      newline();
      writestuff("Copyright (c) 1995, Robert E. Cays");
      newline();
      writestuff("Copyright (c) 1996, Charles Petzold");
      newline();
      writestuff("Copyright (c) 2001-2002  Chris \"Fen\" Tamanaha");
      newline();
      writestuff("SD comes with ABSOLUTELY NO WARRANTY;");
      newline();
      writestuff("   for details see the license.");
      newline();
      writestuff("This is free software, and you are");
      writestuff(" welcome to redistribute it under certain");
      writestuff(" conditions; for details see the license.");
      newline();
      writestuff("You should have received a copy of the GNU General");
      writestuff(" Public License along with this program, in the file");
      writestuff(" \"COPYING.txt\"; if not, write to");
      writestuff(" the Free Software Foundation, Inc., 59 Temple Place,");
      writestuff(" Suite 330, Boston, MA 02111-1307 USA.");
      newline();
      newline();
      writestuff("At any time that you don't know what you can type,");
      newline();
      writestuff("type a question mark (?).  The program will show you all");
      newline();
      writestuff("legal choices.");
      newline();
      newline();

      FILE *session = fopen(SESSION_FILENAME, "r");
      if (session) {
         (void) fclose(session);
      }
      else {
         writestuff("You do not have a session control file.  If you want to create one"
                    ", give the command \"initialize session file\".");
         newline();
         newline();
      }
   }

   if (global_cache_failed_flag) {
      newline();
      writestuff("Cache file mechanism failed, continuing anyway.");
      newline();
      newline();
   }

   // If in diagnostic mode, we print a detailed reason for any cache miss.
   if (ui_options.diagnostic_mode && global_cache_miss_reason[0] != 0) {
      char localreason[500];

      sprintf(localreason, "Cache miss, reloaded: %d %d %d %d %d %d %d.",
              global_cache_miss_reason[0],
              global_cache_miss_reason[1],
              global_cache_miss_reason[2],
              global_cache_miss_reason[3],
              global_cache_miss_reason[4],
              global_cache_miss_reason[5],
              global_cache_miss_reason[6]);
      newline();
      writestuff(localreason);
      newline();
      newline();
   }

   if (ui_options.color_scheme == color_by_couple_random)
      randomize_couple_colors();

   // If anything in the "try" block throws an exception, we will get here
   // with error_flag nonzero.

 got_an_exception:

   // Enter, or re-enter, the big try block.

   try {

      if (global_error_flag == error_flag_user_wants_to_resolve) {
         global_error_flag = error_flag_none;
         goto do_a_resolve;
      }

      if (global_error_flag) {
         // The call we were trying to do has failed.  Abort it and display the error message.

         if (interactivity == interactivity_database_init ||
             interactivity == interactivity_verify)
            ggg.fatal_error_exit(1, "Unknown error context", error_message1);

         // If this is a real call execution error, save the call that caused it.

         if (global_error_flag < error_flag_wrong_command) {
            configuration::history[0] = configuration::next_config();     // So failing call will get printed.
            // But copy the parse tree, since we are going to clip it.
            configuration::history[0].command_root = copy_parse_tree(configuration::history[0].command_root);
            // But without any warnings we may have collected.
            configuration::history[0].init_warnings_specific();
         }
         if (global_error_flag == error_flag_wrong_command) {
            // Special signal -- user clicked on special thing while trying to get subcall.
            if ((global_reply.majorpart == ui_command_select) &&
                ((global_reply.minorpart == command_quit) ||
                 (global_reply.minorpart == command_undo) ||
                 (global_reply.minorpart == command_cut_to_clipboard) ||
                 (global_reply.minorpart == command_delete_entire_clipboard) ||
                 (global_reply.minorpart == command_delete_one_call_from_clipboard) ||
                 (global_reply.minorpart == command_paste_one_call) ||
                 (global_reply.minorpart == command_paste_all_calls) ||
                 (global_reply.minorpart == command_erase) ||
                 (global_reply.minorpart == command_abort)))
               m_reply_pending = true;
            goto start_with_pending_reply;
         }

         // Try to remove the call from the current parse tree, but leave everything else
         // in place.  This will fail if the parse tree, or our place on it, is too
         // complicated.  Also, we do not do it if in diagnostic mode, or if the user
         // did not specify "retain_after_error", or if the special "heads into the middle and ..."
         // operation is in place.

         if (!ui_options.diagnostic_mode &&
             retain_after_error &&
             ((config_history_ptr != 1) || !configuration::history[1].get_startinfo_specific()->into_the_middle) &&
             backup_one_item()) {
            m_reply_pending = false;
            // Take out warnings that arose from the failed call,
            // since we aren't going to do that call.
            configuration::init_warnings();
            goto simple_restart;
         }
         goto start_cycle;      // Failed, reinitialize the whole line.
      }

   show_banner:

      writestuff("Version ");
      write_header_stuff(true, 0);
      newline();
      writestuff("Output file is \"");
      writestuff(outfile_prefix);
      writestuff(outfile_string);
      writestuff("\"");
      newline();

      if (creating_new_session) {
         do_change_outfile(false);
         do_header_popup(header_comment);
         creating_new_session = false;
      }

   new_sequence:

      // Here to start a fresh sequence.  If first time, or if we got here
      // by clicking on "abort", the screen has been cleared.  Otherwise,
      // it shows the last sequence that we wrote.

      // Replace all the parse blocks left from the last sequence.
      // But if we have stuff in the clipboard, we save everything.

      if (clipboard_size == 0) release_parse_blocks_to_mark((parse_block *) 0);

      // Update the console window title.

      {
         char numstuff[50];
         char title[MAX_TEXT_LINE_LENGTH];

         if (sequence_number >= 0)
            sprintf(numstuff, " (%d:%d)", starting_sequence_number, sequence_number);
         else
            numstuff[0] = '\0';

         if (header_comment[0])
            sprintf(title, "%s  %s%s",
                    &old_filename_strings[calling_level][1], header_comment, numstuff);
         else
            sprintf(title, "%s%s",
                    &old_filename_strings[calling_level][1], numstuff);

         ggg.set_window_title(title);
      }

      // Query for the starting setup.

      global_reply = ggg.get_startup_command();

      if (global_reply.majorpart == ui_command_select && global_reply.minorpart == command_quit) goto normal_exit;
      if (global_reply.majorpart != ui_start_select) goto normal_exit;           // Huh?

      switch (global_reply.minorpart) {
      case start_select_toggle_conc:
         allowing_all_concepts = !allowing_all_concepts;
          goto new_sequence;
      case start_select_toggle_singlespace:
         ui_options.singlespace_mode = !ui_options.singlespace_mode;
         goto new_sequence;
      case start_select_toggle_minigrand:
         allowing_minigrand = !allowing_minigrand;
         goto new_sequence;
      case start_select_toggle_bend_home:
         allow_bend_home_getout = !allow_bend_home_getout;
         goto new_sequence;
      case start_select_toggle_overflow_warn:
         enforce_overcast_warning = !enforce_overcast_warning;
         goto new_sequence;
      case start_select_toggle_act:
         using_active_phantoms = !using_active_phantoms;
         goto new_sequence;
      case start_select_toggle_retain:
         retain_after_error = !retain_after_error;
         goto new_sequence;
      case start_select_toggle_nowarn_mode:
         ui_options.nowarn_mode = !ui_options.nowarn_mode;
         goto new_sequence;
      case start_select_toggle_keepallpic_mode:
         ui_options.keep_all_pictures = !ui_options.keep_all_pictures;
         goto new_sequence;
      case start_select_toggle_singleclick_mode:
         ui_options.accept_single_click = !ui_options.accept_single_click;
         goto new_sequence;
      case start_select_toggle_singer:
         if (ui_options.singing_call_mode != 0)
            ui_options.singing_call_mode = 0;    // Turn it off.
         else
            ui_options.singing_call_mode = 1;    // 1 for forward progression, 2 for backward.
         goto new_sequence;
      case start_select_toggle_singer_backward:
         if (ui_options.singing_call_mode != 0)
            ui_options.singing_call_mode = 0;    // Turn it off.
         else
            ui_options.singing_call_mode = 2;
         goto new_sequence;
      case start_select_select_print_font:
         if (!ggg.choose_font()) {
            writestuff("Printing is not supported in this program.");
            newline();
         }
         goto new_sequence;
      case start_select_print_current:
         if (!ggg.print_this()) {
            writestuff("Printing is not supported in this program.");
            newline();
         }
         goto new_sequence;
      case start_select_print_any:
         if (!ggg.print_any()) {
            writestuff("Printing is not supported in this program.");
            newline();
         }
         goto new_sequence;
      case start_select_init_session_file:
         {
            const Cstring *q;
            FILE *session = fopen(SESSION_FILENAME, "r");

            if (session) {
               fclose(session);

               if (ggg.yesnoconfirm("Confirmation",
                                    "You already have a session file.",
                                    "Do you really want to delete it and start over?",
                                    true, false) != POPUP_ACCEPT) {
                  writestuff("No action has been taken.");
                  newline();
                  goto new_sequence;
               }
               else if (!rename(SESSION_FILENAME, SESSION2_FILENAME)) {
                  writestuff("File '" SESSION_FILENAME "' has been saved as '"
                             SESSION2_FILENAME "'.");
                  newline();
               }
            }

            if (!(session = fopen(SESSION_FILENAME, "w"))) {
               writestuff("Failed to create '" SESSION_FILENAME "'.");
               newline();
               goto new_sequence;
            }

            for (q = sessions_init_table ; *q ; q++) {
               if (fputs(*q, session) == EOF) goto copy_failed;
               if (fputs("\n", session) == EOF) goto copy_failed;
            }

            for (q = concept_key_table ; *q ; q++) {
               if (fputs(*q, session) == EOF) goto copy_failed;
               if (fputs("\n", session) == EOF) goto copy_failed;
            }

            for (q = abbreviations_table ; *q ; q++) {
               if (fputs(*q, session) == EOF) goto copy_failed;
               if (fputs("\n", session) == EOF) goto copy_failed;
            }

            if (fputs("\n", session) == EOF) goto copy_failed;
            fclose(session);
            writestuff("The file has been initialized, and will take effect the next time the program is started.");
            newline();
            writestuff("Exit and restart the program if you want to use it now.");
            newline();
            goto new_sequence;

         copy_failed:

            writestuff("Failed to create '" SESSION_FILENAME "'");
            newline();
            fclose(session);
            goto new_sequence;
         }
      case start_select_change_to_new_style_filename:
         {
            FILE *session = fopen(SESSION_FILENAME, "r");

            if (!session) {
               writestuff("You must have a session file to do this.");
               newline();
               writestuff("Give the 'initialize session file command' to create it.");
               newline();
               goto new_sequence;
            }

            fclose(session);
            rewrite_with_new_style_filename = true;
            writestuff("Exit and restart the program to have this take effect.");
            newline();
            goto new_sequence;
         }

      case start_select_randomize_couple_colors:
         randomize_couple_colors();
         goto new_sequence;
      case start_select_change_outfile:
         do_change_outfile(false);
         goto new_sequence;
      case start_select_change_outprefix:
         do_change_outprefix(false);
         goto new_sequence;
      case start_select_change_title:
         do_header_popup(header_comment);
         goto new_sequence;
      case start_select_exit:
         goto normal_exit;
      }

      // We now know that global_reply.minorpart is in the range 1 to 5, that is,
      // start_select_h1p2p ... start_select_as_they_are.  We will put
      // that into the startinfo stuff in the history.

      configuration::initialize_history(global_reply.minorpart);   // Clear the position history.
      configuration::history[1].init_warnings_specific();
      configuration::history[1].init_resolve();
      // Put the people into their starting position.
      configuration::history[1].state = *configuration::history[1].get_startinfo_specific()->the_setup_p;
      two_couple_calling = (attr::klimit(configuration::history[1].state.kind) < 4);
      configuration::history[1].state_is_valid = true;

      written_history_items = -1;
      no_erase_before_this = 1;

      global_error_flag = (error_flag_type) 0;

      // Come here to read a bunch of concepts and a call and add an item to the history.

   start_cycle:

      m_reply_pending = false;

   start_with_pending_reply:

      allowing_modifications = 0;

      // See if we need to increase the size of the history array.
      // We must have history_allocation at least equal to history_ptr+2,
      // so that history items [0..history_ptr+1] will exist.
      // We also need to allow for MAX_RESOLVE_SIZE extra items, so that the
      // resolver can work.  Why don't we just increase the allocation
      // at the start of the resolver if we are too close?  We tried that once.
      // The resolver uses the current parse state, so we can do "TANDEM <resolve>".
      // This means that things like "parse_state.concept_write_base", which point
      // into the history array, must remain valid.  So the resolver can't reallocate
      // the history array.  There is only one place where it is safe to reallocate,
      // and that is right here.  Note that we are about to call "initialize_parse",
      // which destroys any lingering pointers into the history array.

      if (history_allocation < config_history_ptr+MAX_RESOLVE_SIZE+2) {
         int new_history_allocation = history_allocation * 2 + 5;
         configuration *new_history = new configuration[new_history_allocation];
         memcpy(new_history, configuration::history, history_allocation * sizeof(configuration));
         delete [] configuration::history;
         history_allocation = new_history_allocation;
         configuration::history = new_history;
      }

      initialize_parse();

      // Check for first call given to heads or sides only.

      if ((config_history_ptr == 1) && configuration::history[1].get_startinfo_specific()->into_the_middle)
         deposit_concept(&concept_centers_concept);

      // Come here to get a concept or call or whatever from the user.

      // Display the menu and make a choice!!!!

   simple_restart:

      if ((!m_reply_pending) && (!query_for_call())) {
         // User specified a call (and perhaps concepts too).

         // The call to toplevelmove may make a call to "fail", which will get caught
         // by the cleanup handler above, reset history_ptr, and go to start_cycle
         // with the error message displayed.

         toplevelmove();
         finish_toplevelmove();
         config_history_ptr++;         // Call successfully completed; save it.
         goto start_cycle;
      }

      // If get here, query_for_call exitted without completing its parse,
      // because the operator selected something like "quit", "undo",
      // or "resolve", or because we have such a command already pending.

      m_reply_pending = false;

      if (global_reply.majorpart == ui_command_select) {
         switch ((command_kind) global_reply.minorpart) {
         case command_quit:
            if (ggg.do_abort_popup() != POPUP_ACCEPT)
               goto simple_restart;
            goto normal_exit;
         case command_abort:
            if (ggg.do_abort_popup() != POPUP_ACCEPT)
               goto simple_restart;
            clear_screen();
            goto show_banner;
         case command_cut_to_clipboard:
            while (backup_one_item()) ;   // Repeatedly remove any parse blocks that we have.
            initialize_parse();

            if (config_history_ptr <= 1 ||
                (config_history_ptr == 2 && configuration::history[1].get_startinfo_specific()->into_the_middle))
               specialfail("Can't cut past this point.");

            if (m_clipboard_allocation <= clipboard_size) {
               int new_allocation = clipboard_size+1;
               // Increase by 50% beyond what we have now.
               new_allocation += new_allocation >> 1;
               m_clipboard_allocation = new_allocation;
               configuration *new_clipboard = new configuration[new_allocation];
               memcpy(new_clipboard, clipboard, clipboard_size * sizeof(configuration));
               delete [] clipboard;
               clipboard = new_clipboard;
            }

            clipboard[clipboard_size++] = configuration::history[config_history_ptr-1];
            clipboard[clipboard_size-1].command_root = configuration::current_config().command_root;
            config_history_ptr--;
            goto start_cycle;
         case command_delete_entire_clipboard:
            if (clipboard_size != 0) {
               if (ggg.yesnoconfirm("Confirmation", "There are calls in the clipboard.",
                                    "Do you want to delete all of them?",
                                    false, true) != POPUP_ACCEPT)
                  goto simple_restart;
            }

            clipboard_size = 0;
            goto simple_restart;
         case command_delete_one_call_from_clipboard:
            if (clipboard_size != 0) clipboard_size--;
            goto simple_restart;
         case command_paste_one_call:
         case command_paste_all_calls:
            if (clipboard_size == 0) specialfail("The clipboard is empty.");

            while (backup_one_item()) ;   // Repeatedly remove any parse blocks that we have.
            initialize_parse();

            if (config_history_ptr >= 1 &&
                (config_history_ptr >= 2 || !configuration::history[1].get_startinfo_specific()->into_the_middle)) {
               uint32_t status = 0;

               while (clipboard_size != 0) {
                  setup *old = &configuration::current_config().state;
                  setup *nuu = &clipboard[clipboard_size-1].state;

                  configuration::next_config() = clipboard[clipboard_size-1];

                  // Save the entire parse tree, in case it gets damaged
                  // by an aborted selector replacement.

                  parse_block *saved_root = copy_parse_tree(configuration::next_config().command_root);

                  if (fix_up_call_for_fidelity_test(old, nuu, status))
                     goto doitanyway;

                  if (status & 2) {
                     reset_parse_tree(saved_root, configuration::next_config().command_root);
                     specialfail("Sorry, can't fix person identifier.  "
                                 "You can give the command 'delete one call from clipboard' "
                                 "to remove this call.");
                  }

               doitanyway:

                  interactivity = interactivity_no_query_at_all;
                  testing_fidelity = true;

                  // Create a temporary error handler.

                  try {
                     toplevelmove();
                     finish_toplevelmove();
                     config_history_ptr++;
                  }
                  catch(error_flag_type) {
                     // The call failed.
                     interactivity = interactivity_normal;
                     testing_fidelity = false;
                     reset_parse_tree(saved_root, configuration::next_config().command_root);
                     specialfail("The pasted call has failed.  "
                                 "You can give the command 'delete one call from clipboard' "
                                 "to remove it.");
                  }

                  interactivity = interactivity_normal;
                  testing_fidelity = false;
                  clipboard_size--;
                  if ((command_kind) global_reply.minorpart == command_paste_one_call) break;
               }

               if (status & 4) global_error_flag = error_flag_formation_changed;
               else if (status & 1) global_error_flag = error_flag_selector_changed;
               else global_error_flag = (error_flag_type) 0;
            }

            goto start_cycle;
         case command_undo:
            if (backup_one_item()) {
               // We succeeded in backing up by one concept.  Continue from that point.
               // But if we backed all the way to the beginning, reset the call menu list.

               if (parse_state.concept_write_base == parse_state.concept_write_ptr &&
                   parse_state.parse_stack_index == 0)
                  parse_state.call_list_to_use = parse_state.base_call_list_to_use;

               m_reply_pending = false;
               goto simple_restart;
            }
            else if (parse_state.concept_write_base != &configuration::next_config().command_root) {
               // Failed to back up, but some concept exists.  This must have been inside
               // a "checkpoint" or similar complex thing.  Just throw it all away,
               // but do not delete any completed calls.
               m_reply_pending = false;
               goto start_cycle;
            }
            else if (parse_state.concept_write_base != parse_state.concept_write_ptr) {
               // Failed to back up, but some concept exists.
               // Check for just one concept, and that concept is in place
               // only because it is the "centers" concept for a "heads/sides start".
               if (parse_state.concept_write_base &&
                   &((*parse_state.concept_write_base)->next) == parse_state.concept_write_ptr &&
                   config_history_ptr == 1 &&
                   configuration::current_config().get_startinfo_specific()->into_the_middle) {
                  // User typed "undo" at the start of the sequence.
                  // Just abort the sequence.  Don't even ask for permission --
                  // there is nothing of value.
                  clear_screen();
                  goto show_banner;
               }
               else {
                  m_reply_pending = false;
                  goto start_cycle;
               }
            }
            else {
               // There were no concepts entered.  Throw away the entire preceding line.
               if (config_history_ptr > 1) {
                  configuration::current_config().draw_pic = false;
                  configuration::current_config().state_is_valid = false;
                  config_history_ptr--;
                  // Going to start_cycle will make sure written_history_items
                  // does not exceed history_ptr.
                  goto start_cycle;
               }
               else {
                  // User typed "undo" at the start of the sequence.  Abort the sequence.
                  clear_screen();
                  goto show_banner;
               }
            }
         case command_erase:
            m_reply_pending = false;
            goto start_cycle;
         case command_save_pic:
            configuration::current_config().draw_pic = true;
            // We have to back up to BEFORE the item we just changed.
            if (written_history_items > config_history_ptr-1)
               written_history_items = config_history_ptr-1;
            goto simple_restart;

         case command_help:
            {
               char help_string[MAX_ERR_LENGTH];
               const char *prefix;
               int current_length;

               switch (parse_state.call_list_to_use) {
               case call_list_lin:
                  prefix = "You are in facing lines."
                     "  Try typing something like 'pass thru' and pressing Enter.";
                  break;
               case call_list_lout:
                  prefix = "You are in back-to-back lines."
                     "  Try typing something like 'wheel and deal' and pressing Enter.";
                  break;
               case call_list_8ch:
                  prefix = "You are in an 8-chain."
                     "  Try typing something like 'pass thru' and pressing Enter.";
                  break;
               case call_list_tby:
                  prefix = "You are in a trade-by."
                     "  Try typing something like 'trade by' and pressing Enter.";
                  break;
               case call_list_rcol: case call_list_lcol:
                  prefix = "You are in columns."
                     "  Try typing something like 'column circulate' and pressing Enter.";
                  break;
               case call_list_rwv: case call_list_lwv:
                  prefix = "You are in waves."
                     "  Try typing something like 'swing thru' and pressing Enter.";
                  break;
               case call_list_r2fl: case call_list_l2fl:
                  prefix = "You are in 2-faced lines."
                     "  Try typing something like 'ferris wheel' and pressing Enter.";
                  break;
               case call_list_dpt:
                  prefix = "You are in a starting DPT."
                     "  Try typing something like 'double pass thru' and pressing Enter.";
                  break;
               case call_list_cdpt:
                  prefix = "You are in a completed DPT."
                     "  Try typing something like 'cloverleaf' and pressing Enter.";
                  break;
               default:
                  prefix = "Type a call and press Enter.";
                  break;
               }

               (void) strncpy(help_string, prefix, MAX_ERR_LENGTH);
               help_string[MAX_ERR_LENGTH-1] = '\0';
               current_length = strlen(help_string);

               if (configuration::sequence_is_resolved()) {
                  (void) strncpy(&help_string[current_length],
                                 "  You may also write out this finished sequence "
                                 "by typing 'write this sequence'.",
                                 MAX_ERR_LENGTH-current_length);
               }
               else {
                  (void) strncpy(&help_string[current_length],
                                 "  You may also type 'resolve'.",
                                 MAX_ERR_LENGTH-current_length);
               }

               help_string[MAX_ERR_LENGTH-1] = '\0';
               specialfail(help_string);
            }
         case command_change_outfile:
            do_change_outfile(true);
            goto start_cycle;
         case command_change_outprefix:
            do_change_outprefix(true);
            goto start_cycle;
         case command_change_title:
            {
               char newhead_string[MAX_TEXT_LINE_LENGTH];

               // Process it even if it's the null string.
               if (do_header_popup(newhead_string) != POPUP_DECLINE) {
                  strncpy(header_comment, newhead_string, MAX_TEXT_LINE_LENGTH);

                  if (newhead_string[0]) {
                     char confirm_message[MAX_TEXT_LINE_LENGTH+25];
                     strncpy(confirm_message, "Header comment changed to \"", 28);
                     strncat(confirm_message, header_comment, MAX_TEXT_LINE_LENGTH);
                     strncat(confirm_message, "\"", 2);
                     specialfail(confirm_message);
                  }
                  else {
                     specialfail("Header comment deleted");
                  }
               }
               goto start_cycle;
            }
         case command_getout:
            // Check that it is really resolved.

            if (!configuration::sequence_is_resolved()) {
               if (ggg.yesnoconfirm("Confirmation",
                                    "This sequence is not resolved.",
                                    "Do you want to write it anyway?",
                                    false, true) != POPUP_ACCEPT)
                  specialfail("This sequence is not resolved.");
               configuration::current_config().draw_pic = true;
            }

            if (!write_sequence_to_file())
               goto start_cycle; // User cancelled action.

            goto new_sequence;
         case command_select_print_font:
            if (!ggg.choose_font())
               specialfail("Printing is not supported in this program.");
            goto start_cycle;
         case command_print_current:
            if (!ggg.print_this())
               specialfail("Printing is not supported in this program.");
            goto start_cycle;
         case command_print_any:
            if (!ggg.print_any())
               specialfail("Printing is not supported in this program.");
            goto start_cycle;
         case command_help_manual:
            if (!ggg.help_manual())
               specialfail("Manual browsing is not supported in this program.");
            goto start_cycle;
         case command_help_faq:
            if (!ggg.help_faq())
               specialfail("Manual browsing is not supported in this program.");
            goto start_cycle;
         default:     // Should be some kind of search command.

            // If it wasn't, we have a serious problem.
            if (((command_kind) global_reply.minorpart) < command_resolve) goto normal_exit;

         do_a_resolve:

            search_goal = (command_kind) global_reply.minorpart;
            global_reply = full_resolve();

            // If full_resolve refused to operate (for example, we clicked on "reconcile"
            // when in an hourglass), it returned "ui_search_accept", which will cause
            // us simply to go to start_cycle.

            // If user clicked on something random, treat it as though he clicked on
            // "accept" followed by whatever it was.  This includes "quit", "abort",
            // "end this sequence", and any further search commands.
            // So a search command (e.g. "pick random call") will cause the last
            // search result to be accepted, and begin another search on top of that result.

            if (global_reply.majorpart == ui_command_select) {
               switch ((command_kind) global_reply.minorpart) {
               case command_quit:
               case command_abort:
               case command_getout:
                  break;
               default:
                  if (((command_kind) global_reply.minorpart) < command_resolve) goto start_cycle;
                  break;
               }

               allowing_modifications = 0;
               configuration::next_config().draw_pic = false;
               configuration::next_config().state_is_valid = false;
               parse_state.concept_write_base = &configuration::next_config().command_root;
               parse_state.concept_write_ptr = parse_state.concept_write_base;
               *parse_state.concept_write_ptr = (parse_block *) 0;
               m_reply_pending = true;
               // Going to start_with_pending_reply will make sure
               // written_history_items does not exceed history_ptr.
               goto start_with_pending_reply;
            }

            goto start_cycle;
         }
      }

      goto normal_exit;
   }
   catch(error_flag_type sss) {
      global_error_flag = sss;
      goto got_an_exception;
   }

 normal_exit: ;
}
