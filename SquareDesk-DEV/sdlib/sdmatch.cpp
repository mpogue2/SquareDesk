// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2021  William B. Ackerman.
//    Copyright (C) 1993 Alan Snyder
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
//    You should have received a copy of the GNU General Public License,
//    in the file COPYING.txt, along with Sd.  See
//    http://www.gnu.org/licenses/
//
//    ===================================================================

/* This file defines the following functions:
   matcher_initialize
   all methods of matcher_class
*/

#include <string.h> /* for strcpy */
#include <stdio.h>  /* for sprintf */
#include <ctype.h>  /* for tolower */

// The definition of "matcher_class" is in sdmatch.h, called from sd.h.

#include "sd.h"


// Constructor.
matcher_class::matcher_class() : s_modifier_active_list((modifier_block *) 0),
                                 s_modifier_inactive_list((modifier_block *) 0),
                                 m_abbrev_table_normal((abbrev_block *) 0),
                                 m_abbrev_table_start((abbrev_block *) 0),
                                 m_abbrev_table_resolve((abbrev_block *) 0)
{
   // These lists are allocated to size NUM_NAME_HASH_BUCKETS+2.
   // So they will have two extra items at the end:
   //   The first is for calls whose names can't be hashed.
   //   The second is the bucket that any string starting with left bracket hashes to.

   call_hashers = new index_list[NUM_NAME_HASH_BUCKETS+2];
   conc_hashers = new index_list[NUM_NAME_HASH_BUCKETS+2];
   conclvl_hashers = new index_list[NUM_NAME_HASH_BUCKETS+2];

   m_fcn_key_table_normal = new modifier_block *[FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1];
   m_fcn_key_table_start = new modifier_block *[FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1];
   m_fcn_key_table_resolve = new modifier_block *[FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1];

   ::memset(m_fcn_key_table_normal, 0,
            sizeof(modifier_block *) * (FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1));
   ::memset(m_fcn_key_table_start, 0,
            sizeof(modifier_block *) * (FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1));
   ::memset(m_fcn_key_table_resolve, 0,
            sizeof(modifier_block *) * (FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1));
}

// Destructor.
matcher_class::~matcher_class()
{
   delete [] call_hashers;
   delete [] conc_hashers;
   delete [] conclvl_hashers;

   delete [] m_fcn_key_table_normal;
   delete [] m_fcn_key_table_start;
   delete [] m_fcn_key_table_resolve;
}



void matcher_class::initialize_for_parse(int which_commands, bool show, bool only_want_extension)
{
   // Reclaim all old modifier blocks.  These are global over the entire program.
   // All other fields get initialized (right here!) for every parse.

   while (s_modifier_active_list) {
      modifier_block *item = s_modifier_active_list;
      s_modifier_active_list = item->gc_ptr;
      item->gc_ptr = s_modifier_inactive_list;
      s_modifier_inactive_list = item;
   }

   // (Mostly) clear out m_active_result.  We used to do this as one big memset over the
   // whole object.  But modern compilers have gotten extremely fussy about this.
   //
   // It is of type "match_result", which is defined in sd.h around line 2306.
   //
   // Yeah, yeah, we should use nice constructors and do this up real purty.

   m_active_result.valid = true;   // The field we don't clear.
   m_active_result.exact = false;
   m_active_result.indent = false;
   m_active_result.real_next_subcall = (const match_result *) 0;
   m_active_result.real_secondary_subcall = (const match_result *) 0;
   m_active_result.recursion_depth = 0;
   m_active_result.yield_depth = 0;

   m_active_result.match.kind = ui_start_select;
   m_active_result.match.index = 0;
   m_active_result.match.call_conc_options.initialize();
   m_active_result.match.call_ptr = (call_with_name *) 0;
   m_active_result.match.concept_ptr = (const concept_descriptor *) 0;
   m_active_result.match.packed_next_conc_or_subcall = (modifier_block *) 0;
   m_active_result.match.packed_secondary_subcall = (modifier_block *) 0;
   m_active_result.match.gc_ptr = (modifier_block *) 0;

   m_current_result = &m_active_result;
   m_only_extension = only_want_extension;
   m_user_bracket_depth = 0;
   m_match_count = 0;
   m_exact_count = 0;
   m_showing = show;
   m_showing_has_stopped = false;
   m_verify = false;
   m_lowest_yield_depth = 999;
   m_call_menu = which_commands;

   m_echo_stuff[0] = 0;   // Needed if no matches or user input is empty.
   m_yielding_matches = 0;
   m_final_result.valid = false;
   m_final_result.exact = false;
   m_space_ok = false;

   /* ****  Also need to consider initializing these?
      int m_extended_bracket_depth;
      char m_user_input[INPUT_TEXTLINE_SIZE+1];     // the current user input
      char m_full_extension[INPUT_TEXTLINE_SIZE+1]; // the extension for the current pattern
      int m_user_input_size;                        // This is always equal to strlen(m_user_input).
   */
}


namespace {

Cstring n_4_patterns[] = {
   "0/4",
   "1/4",
   "2/4",
   "3/4",
   "4/4",
   "5/4",
   (Cstring) 0
};


int translate_keybind_spec(char key_name[])
{
   int key_length;
   int d1, d2, digits;
   bool shift = false;
   bool ctl = false;
   bool alt = false;
   bool ctlalt = false;

   /* Compress hyphens out, and canonicalize to lower case. */
   for (d1=key_length=0 ; key_name[d1] ; d1++) {
      if (key_name[d1] != '-') key_name[key_length++] = tolower(key_name[d1]);
   }

   if (key_length < 2) return -1;

   switch (key_name[0]) {
   case 's': shift = true; break;
   case 'c':   ctl = true; break;
   case 'a':
   case 'm':   alt = true; break;
   }

   switch (key_name[1]) {
   case 'c':   ctlalt = alt; break;
   case 'a':
   case 'm':   ctlalt = ctl; break;
   }

   d2 = key_name[key_length-1] - '0';
   if (d2 >= 0 && d2 <= 9) {
      digits = d2;
      d1 = key_name[key_length-2] - '0';
      if (d1 >= 0 && d1 <= 9) {
         digits += d1*10;
         key_length--;
      }

      if (key_name[key_length-2] == 'f') {
         if (digits < 1 || digits > 12)
            return -1;
         else if (key_length == 2) {
            return FKEY+digits;
         }
         else if (key_length == 3 && shift) {
            return SFKEY+digits;
         }
         else if (key_length == 3 && ctl) {
            return CFKEY+digits;
         }
         else if (key_length == 3 && alt) {
            return AFKEY+digits;
         }
         else if (key_length == 4 && ctlalt) {
            return CAFKEY+digits;
         }
         else {
            return -1;
         }
      }
      if (key_name[key_length-2] == 'n') {
         if (digits > 9 || key_length < 3)
            return -1;
         else if (key_length == 3 && ctl) {
            return CTLNKP+digits;
         }
         else if (key_length == 3 && alt) {
            return ALTNKP+digits;
         }
         else if (key_length == 4 && ctlalt) {
            return CTLALTNKP+digits;
         }
         else {
            return -1;
         }
      }
      if (key_name[key_length-2] == 'e') {
         if (digits > 15)
            return -1;

         if (key_length == 2) {
            return EKEY+digits;
         }
         else if (key_length == 3 && shift) {
            return SEKEY+digits;
         }
         else if (key_length == 3 && ctl) {
            return CEKEY+digits;
         }
         else if (key_length == 3 && alt) {
            return AEKEY+digits;
         }
         else if (key_length == 4 && ctlalt) {
            return CAEKEY+digits;
         }
         else {
            return -1;
         }
      }
      else if (key_length == 2 && ctl) {
         return CTLDIG+digits;
      }
      else if (key_length == 2 && alt) {
         return ALTDIG+digits;
      }
      else if (key_length == 3 && ctlalt) {
         return CTLALTDIG+digits;
      }
      else {
         return -1;
      }
   }
   else if (key_name[key_length-1] >= 'a' && key_name[key_length-1] <= 'z') {
      if (key_length == 2 && ctl) {
         return CTLLET+key_name[key_length-1]+'A'-'a';
      }
      else if (key_length == 2 && alt) {
         return ALTLET+key_name[key_length-1]+'A'-'a';
      }
      else if (key_length == 3 && ctlalt) {
         return CTLALTLET+key_name[key_length-1]+'A'-'a';
      }
      else {
         return -1;
      }
   }
   else
      return -1;
}



// This returns zero if the name was clearly not able to be hashed
// (contains NUL, comma, atsign, or single quote).  It returns 1
// if it could clearly be hashed (no blanks).  It returns 2 if it
// had blanks, and might be questionable.  Patterns to match
// (call or concept names, etc. can have blanks in them and still
// be hashed.  Strings the user types can't be hashed if they have blanks.
int get_hash(Cstring string, int *bucket_p)
{
   char c1 = string[0];
   char c2 = string[1];
   char c3 = string[2];
   int bucket;

   if (c1 == '<' && ((int) c2 & ~32) == 'A' && ((int) c3 & ~32) == 'N') {
      *bucket_p = matcher_class::BRACKET_HASH;
      return 1;
   }
   else if ((c1 && c1 != ',' && c1 != '@' && c1 != '\'') &&
            (c2 && c2 != ',' && c2 != '@' && c2 != '\'') &&
            (c3 && c3 != ',' && c3 != '@' && c3 != '\'')) {
      // We use a hash function that ignores the "32" bit, so it is insensitive to case.
      bucket = (((((int) c1 & ~32) << 3) + ((int) c2 & ~32)) << 3) + ((int) c3 & ~32);
      bucket += bucket * 20;
      bucket += bucket >> 7;
      bucket &= (matcher_class::NUM_NAME_HASH_BUCKETS-1);
      *bucket_p = bucket;

      if (c1 != ' ' && c2 != ' ' && c3 != ' ') return 1;
      else return 2;
   }
   else
      return 0;
}


void hash_me(int bucket, int i)
{
   gg77->matcher_p->call_hashers[bucket].add_one(i);
}


bool foobar(const char *user)
{
   if (!user[0]) return true;
   if (user[0] != ' ') return false;
   if (!user[1]) return true;
   if (user[1] != 'e') return false;
   if (!user[2]) return true;
   if (user[2] != 'r') return false;
   if (!user[3]) return true;
   if (user[3] != 's' && user[3] != '\'') return false;
   return true;
}

}   // namespace

void matcher_class::copy_sublist(const match_result *outbar, modifier_block *tails)
{
   if (outbar->real_next_subcall) {
      modifier_block *out;
      const match_result *newoutbar = outbar->real_next_subcall;

      if (s_modifier_inactive_list) {
         out = s_modifier_inactive_list;
         s_modifier_inactive_list = out->gc_ptr;
      }
      else
         out = new modifier_block;

      *out = newoutbar->match;
      out->packed_next_conc_or_subcall = (modifier_block *) 0;
      out->packed_secondary_subcall = (modifier_block *) 0;
      out->gc_ptr = s_modifier_active_list;
      s_modifier_active_list = out;
      tails->packed_next_conc_or_subcall = out;
      copy_sublist(newoutbar, out);
   }

   if (outbar->real_secondary_subcall) {
      modifier_block *out;
      const match_result *newoutbar = outbar->real_secondary_subcall;

      if (s_modifier_inactive_list) {
         out = s_modifier_inactive_list;
         s_modifier_inactive_list = out->gc_ptr;
      }
      else
         out = new modifier_block;

      *out = newoutbar->match;
      out->packed_next_conc_or_subcall = (modifier_block *) 0;
      out->packed_secondary_subcall = (modifier_block *) 0;
      out->gc_ptr = s_modifier_active_list;
      s_modifier_active_list = out;
      tails->packed_secondary_subcall = out;
      copy_sublist(newoutbar, out);
   }
}


void matcher_class::copy_to_user_input(const char *stuff)
{
   // See comments in sd.h about INPUT_TEXTLINE_SIZE and MAX_TEXT_LINE_LENGTH.
   // The target is known to be allocated to INPUT_TEXTLINE_SIZE+1.
   // The source is null-terminated and in an area of size MAX_TEXT_LINE_LENGTH.
   ::strcpy(m_user_input, stuff);
   m_user_input_size = ::strlen(m_user_input);
}


void matcher_class::erase_matcher_input()
{
   m_user_input[0] = '\0';
   m_user_input_size = 0;
}


int matcher_class::delete_matcher_word()
{
   bool deleted_letter = false;
   int orig_size = m_user_input_size;

   while (m_user_input_size > 0) {
      if (m_user_input[m_user_input_size-1] == ' ') {
         if (deleted_letter) break;
      }
      else
         deleted_letter = true;

      m_user_input_size--;
      m_user_input[m_user_input_size] = '\0';
   }

   return orig_size-m_user_input_size;
}

// Returns true if it processed the thing.
bool matcher_class::process_accel_or_abbrev(modifier_block & mb, char linebuff[])
{
   m_active_result.match = mb;
   m_active_result.indent = false;
   m_active_result.real_next_subcall = (match_result *) 0;
   m_active_result.real_secondary_subcall = (match_result *) 0;
   m_final_result.match = m_active_result.match;

   switch (m_final_result.match.kind) {
   case ui_command_select:
      strcpy(linebuff, command_commands[m_final_result.match.index]);
      m_final_result.match.index = -1-command_command_values[m_final_result.match.index];
      break;
   case ui_resolve_select:
      strcpy(linebuff, resolve_menu[m_final_result.match.index].command_name);
      m_final_result.match.index = -1-resolve_menu[m_final_result.match.index].action;
      break;
   case ui_start_select:
      strcpy(linebuff, startup_commands[m_final_result.match.index]);
      break;
   case ui_concept_select:
      gg77->unparse_call_name(get_concept_name(m_final_result.match.concept_ptr),
                              linebuff,
                              &m_final_result.match.call_conc_options);
      // Reject off-level concept accelerator key presses.
      if (!allowing_all_concepts &&
          get_concept_level(m_final_result.match.concept_ptr) > calling_level)
         return false;

      m_final_result.match.index = 0;
      break;
   case ui_call_select:
      gg77->unparse_call_name(get_call_name(m_final_result.match.call_ptr),
                              linebuff,
                              &m_final_result.match.call_conc_options);
      m_final_result.match.index = 0;
      break;
   default:
      return false;
   }


   m_final_result.valid = true;
   m_active_result.valid = true;
   return true;
}


void matcher_class::do_accelerator_spec(Cstring inputline, bool is_accelerator)
{
   if (!inputline[0] || inputline[0] == '\r' || inputline[0] == '\n' || inputline[0] == '#') return;   // This is a blank line or a comment.

   char key_name[MAX_FILENAME_LENGTH];
   char *key_org;
   char junk_name[MAX_FILENAME_LENGTH];
   int ccount;
   int menu_type = call_list_any;
   int keybindcode = -1;

   if (sscanf(inputline, "%s %n%s", key_name, &ccount, junk_name) == 2) {
      key_org = key_name;

      if (key_name[0] == '+') {
         menu_type = e_match_startup_commands;
         key_org = &key_name[1];
      }
      else if (key_name[0] == '*') {
         menu_type = e_match_resolve_commands;
         key_org = &key_name[1];
      }

      if (is_accelerator)
         keybindcode = translate_keybind_spec(key_org);
      else
         keybindcode = 0;
   }

   if (keybindcode < 0)
      gg77->iob88.fatal_error_exit(1, "Bad format in key binding", inputline);

   m_active_result.match.kind = ui_call_select;

   Cstring target = inputline+ccount;

   if (!strcmp(target, "deleteline"))
      m_active_result.match.index = special_index_deleteline;
   else if (!strcmp(target, "deleteword"))
      m_active_result.match.index = special_index_deleteword;
   else if (!strcmp(target, "copytext"))
      m_active_result.match.index = special_index_copytext;
   else if (!strcmp(target, "cuttext"))
      m_active_result.match.index = special_index_cuttext;
   else if (!strcmp(target, "pastetext"))
      m_active_result.match.index = special_index_pastetext;
   else if (!strcmp(target, "lineup"))
      m_active_result.match.index = special_index_lineup;
   else if (!strcmp(target, "linedown"))
      m_active_result.match.index = special_index_linedown;
   else if (!strcmp(target, "pageup"))
      m_active_result.match.index = special_index_pageup;
   else if (!strcmp(target, "pagedown"))
      m_active_result.match.index = special_index_pagedown;
   else if (!strcmp(target, "quoteanything"))
      m_active_result.match.index = special_index_quote_anything;
   else {
      ::strcpy(m_user_input, target);
      m_user_input_size = ::strlen(m_user_input);
      int matches = match_user_input(menu_type, false, false, false);
      m_active_result = m_final_result;

      if ((matches != 1 && matches - m_yielding_matches != 1 && !m_active_result.exact)) {
         // Didn't find the target of the key binding.  Below C4X, failure to find
         // something could just mean that it was a call off the list.  At C4X, we
         // take it seriously.  So the initialization file should always be tested at C4X.
         if (calling_level >= l_c4x)
            gg77->iob88.fatal_error_exit(1, "Didn't find target of accelerator or abbreviation", inputline);

         return;
      }

      if (m_active_result.match.packed_next_conc_or_subcall ||
          m_active_result.match.packed_secondary_subcall) {
         if (calling_level >= l_c4x)
            gg77->iob88.fatal_error_exit(1, "Target of accelerator or abbreviation is too complicated", inputline);

         return;
      }
   }

   if (is_accelerator) {
      modifier_block **table_thing;

      if (m_active_result.match.kind == ui_start_select) {
         table_thing = &m_fcn_key_table_start[keybindcode-FCN_KEY_TAB_LOW];
      }
      else if (m_active_result.match.kind == ui_resolve_select) {
         table_thing = &m_fcn_key_table_resolve[keybindcode-FCN_KEY_TAB_LOW];
      }
      else if (m_active_result.match.kind == ui_concept_select ||
               m_active_result.match.kind == ui_call_select ||
               m_active_result.match.kind == ui_command_select) {
         table_thing = &m_fcn_key_table_normal[keybindcode-FCN_KEY_TAB_LOW];
      }
      else
         gg77->iob88.fatal_error_exit(1, "Anomalous accelerator", inputline);

      modifier_block *newthing = new modifier_block;
      *newthing = m_active_result.match;

      if (*table_thing)
         gg77->iob88.fatal_error_exit(1, "Redundant accelerator", inputline);

      *table_thing = newthing;
   }
   else {
      abbrev_block **table_thing;

      if (m_active_result.match.kind == ui_start_select) {
         table_thing = &m_abbrev_table_start;
      }
      else if (m_active_result.match.kind == ui_resolve_select) {
         table_thing = &m_abbrev_table_resolve;
      }
      else if (m_active_result.match.kind == ui_concept_select ||
               m_active_result.match.kind == ui_call_select ||
               m_active_result.match.kind == ui_command_select) {
         table_thing = &m_abbrev_table_normal;
      }
      else
         gg77->iob88.fatal_error_exit(1, "Anomalous abbreviation", inputline);

      abbrev_block *newthing = new abbrev_block;
      newthing->key = new char[::strlen(key_org)+1];
      ::strcpy((char *) newthing->key, key_org);
      newthing->value = m_active_result.match;
      newthing->next = *table_thing;
      *table_thing = newthing;
   }
}



/* Call MATCHER_INITIALIZE first.
   This function sets up the concept list.  The concepts are found
   in the external array concept_descriptor_table.  For each
   i, the field concept_descriptor_table[i].name has the text that we
   should display for the user.
*/
void matcher_initialize()
{
   int i, j;
   int concept_number;
   const concept_descriptor *p;

   // Decide whether we allow the "diagnose" concept, by deciding
   // when we will stop the concept list scan.
   concept_kind end_marker = (ui_options.diagnostic_mode) ?
      constant_with_marker_end_of_list :
      constant_with_concept_diagnose;

   // Create the concept lists, both universal and level-restricted.

   for (concept_number=0 ; ; concept_number++) {
      p = access_concept_descriptor_table(concept_number);
      if (get_concept_kind(p) == end_marker) break;
      gg77->matcher_p->m_concept_list.add_one(concept_number);
      if (get_concept_level(p) <= calling_level)
         gg77->matcher_p->m_level_concept_list.add_one(concept_number);
   }

   // Initialize the hash buckets for selectors, concepts, and calls.

   int bucket;
   uint32_t ku;

   // First, do the selectors.  Before that, be sure "<anyone>" is hashed.
   // These list all the buckets that selectors can go to.

   if (!get_hash("<an", &bucket))
      gg77->iob88.fatal_error_exit(2, "Can't hash selector base");

   index_list selector_hasher;
   selector_hasher.add_one(bucket);

   for (i=1; i<selector_INVISIBLE_START; i++) {
      if (selector_list[i].name[0] == '@' && (selector_list[i].name[1] == 'k' || selector_list[i].name[1] == '6')) {
         // This is a selector like "<anyone>-based triangles".  Put it into every bucket
         // that could match a selector.  Is any of this stuff necessary?
         continue;
      }

      if (!get_hash(selector_list[i].name, &bucket)) {
         char errbuf[255];
         sprintf(errbuf, "Can't hash selector %d - 1!", i);
         gg77->iob88.fatal_error_exit(2, errbuf);
      }

      // See if this bucket is already accounted for.

      for (j=0; j<selector_hasher.the_list_size; j++) {
         if (selector_hasher.the_list[j] == bucket) goto already_in1;
      }

      selector_hasher.add_one(bucket);

      // Now do it again for the singular names.

   already_in1:

      if (selector_list[i].sing_name[0] == '@' && (selector_list[i].sing_name[1] == 'k')) {
         // This is a selector like "<anyone>-based triangle".  Put it into every bucket
         // that could match a selector.  Is any of this stuff necessary?
         continue;
      }

      if (!get_hash(selector_list[i].sing_name, &bucket)) {
         char errbuf[255];
         sprintf(errbuf, "Can't hash selector %d - 2!", i);
         gg77->iob88.fatal_error_exit(2, errbuf);
      }

      for (j=0; j<selector_hasher.the_list_size; j++) {
         if (selector_hasher.the_list[j] == bucket) goto already_in2;
      }

      selector_hasher.add_one(bucket);

   already_in2: ;
   }

   // Next, do the taggers.  Before that, be sure "<atc>" is hashed.
   // These list all the buckets that taggers can go to.

   if (!get_hash("<at", &bucket))
      gg77->iob88.fatal_error_exit(2, "Can't hash tagger base!");

   index_list tagger_hasher;
   tagger_hasher.add_one(bucket);

   for (i=0; i<NUM_TAGGER_CLASSES; i++) {
      for (ku=0; ku<number_of_taggers[i]; ku++) {
         // Some taggers ("@6 start vertical tag") can't be hashed because
         // of the at sign.  That's OK.
         if (get_call_name(tagger_calls[i][ku])[0] != '@') {
            if (!get_hash(get_call_name(tagger_calls[i][ku]), &bucket)) {
               char errbuf[255];
               sprintf(errbuf, "Can't hash tagger %d %d!", i, (int) ku);
               gg77->iob88.fatal_error_exit(2, errbuf);
            }
         }

         for (j=0; j<tagger_hasher.the_list_size; j++) {
            if (tagger_hasher.the_list[j] == bucket) goto already_in3;
         }

         tagger_hasher.add_one(bucket);

      already_in3: ;
      }
   }

   /* Now do the calls. */

   for (i=0; i<number_of_calls[call_list_any]; i++) {
      Cstring name = get_call_name(main_call_lists[call_list_any][i]);

   doitagain:

      if (name[0] == '@') {
         switch (name[1]) {
         case '6': case 'k': case 'K': case 'V':
            // This is a call like "<anyone> run".  Put it into every bucket
            // that could match a selector.

            for (j=0 ; j<selector_hasher.the_list_size ; j++)
               hash_me(selector_hasher.the_list[j], i);

            continue;
         case 'v': case 'w': case 'x': case 'y':
            // This is a call like "<atc> your neighbor".
            // Put it into every bucket that could match a tagger.

            for (j=0 ; j<tagger_hasher.the_list_size ; j++)
               hash_me(tagger_hasher.the_list[j], i);

            continue;
         case '0': case 'T': case 'm':
            // We act as though any string starting with "[" hashes to BRACKET_HASH.
            hash_me(matcher_class::BRACKET_HASH, i);
            continue;
         case 'e':
            // If this is "@e", hash it to both "left" and to whatever naturally follows.
            (void) get_hash("left", &bucket);
            hash_me(bucket, i);
            name += 2;
            goto doitagain;
         default:
            if (!get_escape_string(name[1])) {
               // If this escape is something like "@2", as in "@2scoot and plenty",
               // ignore it.  Hash it under "scoot and plenty".
               name += 2;
               goto doitagain;
            }
            break;
         }
      }

      if (get_hash(name, &bucket)) {
         hash_me(bucket, i);
         continue;
      }

      // If we get here, this call needs to be put into the extra bucket at the end,
      // and also into EVERY OTHER BUCKET!!!!
      for (bucket=0 ; bucket < matcher_class::NUM_NAME_HASH_BUCKETS+1 ; bucket++)
         hash_me(bucket, i);
   }

   // Now do the concepts from the full concept list.

   for (i=0; i<gg77->matcher_p->m_concept_list.the_list_size; i++) {
      int the_item = gg77->matcher_p->m_concept_list.the_list[i];
      Cstring name = get_concept_name(access_concept_descriptor_table(the_item));

      if (name[0] == '@' && (name[1] == '6' || name[1] == 'k' || name[1] == 'K' || name[1] == 'V')) {
         // This is a call like "<anyone> run".  Put it into every bucket
         // that could match a selector.

         for (j=0 ; j<selector_hasher.the_list_size ; j++) {
            bucket = selector_hasher.the_list[j];
            gg77->matcher_p->conc_hashers[bucket].add_one(the_item);
         }
         continue;
      }

      if (get_hash(name, &bucket)) {
         gg77->matcher_p->conc_hashers[bucket].add_one(the_item);
         continue;
      }

      // If we get here, this concept needs to be put into the extra bucket at the end,
      // and also into EVERY OTHER BUCKET!!!!
      for (bucket=0 ; bucket < matcher_class::NUM_NAME_HASH_BUCKETS+1 ; bucket++) {
         gg77->matcher_p->conc_hashers[bucket].add_one(the_item);
      }
   }

   // Now do the "level concepts".

   for (i=0; i<gg77->matcher_p->m_level_concept_list.the_list_size; i++) {
      int the_item = gg77->matcher_p->m_level_concept_list.the_list[i];
      Cstring name = get_concept_name(access_concept_descriptor_table(the_item));

      if (name[0] == '@' && (name[1] == '6' || name[1] == 'k' || name[1] == 'K' || name[1] == 'V')) {
         // This is a call like "<anyone> run".  Put it into every bucket
         // that could match a selector.

         for (j=0 ; j<selector_hasher.the_list_size ; j++) {
            bucket = selector_hasher.the_list[j];
            gg77->matcher_p->conclvl_hashers[bucket].add_one(the_item);
         }
         continue;
      }
      else if (get_hash(name, &bucket)) {
         gg77->matcher_p->conclvl_hashers[bucket].add_one(the_item);
         continue;
      }

      // If we get here, this concept needs to be put into the extra bucket at the end,
      // and also into EVERY OTHER BUCKET!!!!
      for (bucket=0 ; bucket < matcher_class::NUM_NAME_HASH_BUCKETS+1 ; bucket++) {
         gg77->matcher_p->conclvl_hashers[bucket].add_one(the_item);
      }
   }
}


/*
 * Call Verification
 */



/* These variables are actually local to verify_call, but they are
   expected to be preserved across the throw, so they must be static. */
static parse_block *parse_mark;
static call_list_kind savecl;

/*
 * Return TRUE if the specified call appears to be legal in the
 * current context.
 */

bool matcher_class::verify_call()
{
   // If we are not verifying, we return TRUE immediately,
   // thereby causing the item to be listed.
   if (!m_verify) return true;

   bool resultval = true;
   interactivity = interactivity_verify;   // So deposit_call doesn't ask user for info.
   warning_info saved_warnings = config_save_warnings();
   int old_history_ptr = config_history_ptr;

   parse_mark = get_parse_block_mark();
   save_parse_state();
   savecl = parse_state.call_list_to_use;

   start_sel_dir_num_iterator();

 try_another_selector:

   selector_used = false;
   direction_used = false;
   number_used = false;
   mandatory_call_used = false;
   verify_used_number = false;
   verify_used_selector = false;
   verify_used_direction = false;

   // Do the call.  An error will signal and go to failed.

   try {
      bool theres_a_call_in_here = false;
      parse_block *save1 = (parse_block *) 0;
      modifier_block *anythings = &m_final_result.match;

      restore_parse_state();

      /* This stuff is duplicated in uims_get_call_command in sdui-tty.c . */

      while (anythings) {
         call_conc_option_state save_stuff = m_final_result.match.call_conc_options;

         // First, if we have already deposited a call, and we see more stuff, it must be
         // concepts or calls for an "anything" subcall.

         if (save1) {
            parse_block *tt = get_parse_block();
            set_parse_block_concept(save1, &concept_marker_concept_mod);
            set_parse_block_next(save1, tt);
            set_parse_block_concept(tt, &concept_marker_concept_mod);
            set_parse_block_call(tt, access_base_calls(base_call_null));
            set_parse_block_call_to_print(tt, access_base_calls(base_call_null));
            set_parse_block_replacement_key(tt, DFM1_CALL_MOD_MAND_ANYCALL/DFM1_CALL_MOD_BIT);
            parse_state.concept_write_ptr = get_parse_block_subsidiary_root_addr(tt);
            save1 = (parse_block *) 0;
         }

         m_final_result.match.call_conc_options = anythings->call_conc_options;

         if (anythings->kind == ui_call_select) {
            verify_options = anythings->call_conc_options;
            if (deposit_call(anythings->call_ptr, &anythings->call_conc_options)) {
               // The problem may be just that the current number is
               // inconsistent with the call's "odd number only" requirement.
               number_used = true;
               if (iterate_over_sel_dir_num(verify_used_selector,
                                            verify_used_direction,
                                            verify_used_number))
                  goto try_another_selector;

               goto failed;
            }
            save1 = *parse_state.concept_write_ptr;
            theres_a_call_in_here = true;
         }
         else if (anythings->kind == ui_concept_select) {
            verify_options = anythings->call_conc_options;
            if (deposit_concept(anythings->concept_ptr)) goto failed;
         }
         else break;   /* Huh? */

         m_final_result.match.call_conc_options = save_stuff;
         anythings = anythings->packed_next_conc_or_subcall;
      }

      parse_state.call_list_to_use = savecl;         /* deposit_concept screwed it up */

         /* If we didn't see a call, the user is just verifying
            a string of concepts. Accept it. */

      if (!theres_a_call_in_here) goto accept;

      /* If the parse stack is nenempty, a subsidiary call is needed and hasn't been filled in.
            Therefore, the parse tree is incomplete.  We can print such parse trees, but we
            can't execute them.  So we just assume the call works. */

      if (parse_state.parse_stack_index != 0) goto accept;

      toplevelmove();   // This might throw an error.
      goto accept;
   }
   catch(error_flag_type) {
      // A call failed.  If the call had some mandatory substitution, pass it anyway.

      if (mandatory_call_used) goto accept;

      // Or a bad choice of selector or number may be the cause.
      // Try again with a different selector, until we run out of ideas.

      if (iterate_over_sel_dir_num(verify_used_selector,
                                   verify_used_direction,
                                   verify_used_number))
         goto try_another_selector;

      goto failed;
   }

   failed:
      resultval = false;

   accept:

   restore_parse_state();
   release_parse_blocks_to_mark(parse_mark);

   config_history_ptr = old_history_ptr;
   config_restore_warnings(saved_warnings);
   interactivity = interactivity_normal;

   return resultval;
}




/*
 * Record a match.  Extension is how the current input would be extended to
 * match the current pattern.  Result is the value of the
 * current pattern.  Special case: if user input is empty, extension is
 * not set.
 */

void matcher_class::record_a_match()
{
   int old_yield = m_lowest_yield_depth;
   m_extended_bracket_depth = 0;

   if (!m_showing) {
      char *s1 = m_echo_stuff;
      const char *s2 = m_full_extension;

      if (m_match_count == 0) {
         // This is the first match.  Set m_echo_stuff to the
         // full extension that we have.  Count brackets.
         for ( ; ; s1++,s2++) {
            *s1 = *s2;
            if (!*s1) break;
            else if (*s1 == '[') m_extended_bracket_depth++;
            else if (*s1 == ']') m_extended_bracket_depth--;
         }
      }
      else {
         // Shorten m_echo_stuff to the maximal common prefix.
         // Count brackets.
         for ( ; ; s1++,s2++) {
            if (!*s1) break;
            else if (*s1 != *s2) {
               *s1 = 0;
               break;
            }
            else if (*s1 == '[') m_extended_bracket_depth++;
            else if (*s1 == ']') m_extended_bracket_depth--;
         }
      }
   }

   // Copy if we are doing the "show" operation, whether we are verifying or not.
   if (m_showing) m_final_result = m_active_result;

   // Always copy the first match.
   // Also, always copy, and process modifiers, if we are processing the
   // "verify" operation.  Also, copy the first exact match, and any
   // exact match that isn't yielding relative to what we have so far.

   if (m_match_count == 0 ||
       m_verify ||
       (*m_full_extension == '\0' &&
        (m_exact_count == 0 || m_current_result->yield_depth <= old_yield))) {
      m_final_result = m_active_result;
      m_lowest_yield_depth = m_current_result->yield_depth;

      // We need to copy the modifiers to reasonably stable storage.
      copy_sublist(&m_final_result, &m_final_result.match);
   }

   if (*m_full_extension == '\0') {
      m_exact_count++;
      m_final_result.exact = true;
   }

   m_match_count++;

   if (m_current_result->yield_depth > old_yield)
      m_yielding_matches++;

   if (m_showing) {
      if (verify_call()) gg77->iob88.show_match();
   }
}

/* ************************************************************************

This procedure must obey certain properties.  Be sure that no modifications
to this procedure compromise this, or else fix the stuff in
"scan_concepts_and_calls" that depends on it.

Theorem 1 (pure calls):

   If user != nil
         AND
   pat1[0..2] does not contain NUL, comma, atsign, or apostrophe
         AND
   user[0..2] does not contain NUL or blank
         AND
   user[0..2] does not match pat1[0..2] case insensitively,
         THEN
   this procedure will do nothing.

Theorem 2 (<anyone> calls):

   If user != nil
         AND
   pat1[0..1] = "@6" or "@k",
         AND
   user[0..2] does not contain NUL or blank
         AND
   user[0..2] does not match any selector name
      (note that we have already determined that no selector
      name has NUL, comma, atsign, or apostrophe in the first 3
      characters, and have determined what hash buckets they must
      lie in)
         AND
   user[0..2] does not match "<an" case insensitively,
         THEN
   this procedure will do nothing.

Theorem 3 (<atc> calls):

   If user != nil
         AND
   pat1[0..1] = "@v", "@w", "@x" or "@y",
         AND
   user[0..2] does not contain NUL or blank
         AND
   user[0..2] does not match any tagger name
      (note that we have already determined that no tagger
      name has NUL, comma, atsign, or apostrophe in the first 3
      characters, and have determined what hash buckets they must
      lie in)
         AND
   user[0..2] does not match "<at" case insensitively,
         THEN
   this procedure will do nothing.

Theorem 4 (<anything> calls):

   If user != nil
         AND
   pat1[0..1] = "@0" or "@m",
         AND
   user[0] is not left bracket or NUL
***** under what circumstances is it true even if user[0] = NUL???????
Well, we need m_showing off, and pat2, if non-nil, must have folks_to_restore = nil
then it won't do anything except record bunches of extra partial matches (but no exact ones),
and generally mess around.
         AND
   user[0..2] does not match "<an" case insensitively,
         THEN
   this procedure will do nothing.

Theorem A (prefix mismatch):

   If user != nil AND user[0] is not NUL
         AND
   pat1 = the single character left bracket or blank (this theorem
      could apply to many other characters, but these are the only
      ones that arise)
         AND
   user[0] != pat1[0]
         THEN
   this procedure will do nothing.

Theorem B (prefix match):

   If user != nil AND user[0] is not NUL
         AND
   pat1 = the single character left bracket or blank
         AND
   user[0] = pat1[0]
         AND
   pat2 is not nil
         AND
   pat2->demand_a_call = false   (don't really need this, since it's doing nothing!)
         AND
   pat2->folks_to_restore = nil
         AND
   this procedure would do nothing if called with
      pat2->car in place of pat1 and the remaining characters of user.
         THEN
   this procedure will do nothing.

************************************************************************ */

void matcher_class::match_suffix_2(Cstring user, Cstring pat1, pat2_block *pat2, int patxi)
{
   match_result *save_current = m_current_result;
   const concept_descriptor *pat2_concept = (concept_descriptor *) 0;

   if (pat2->special_concept &&
       !(get_concparseflags(pat2->special_concept) & CONCPARSE_PARSE_DIRECT))
      pat2->special_concept = (concept_descriptor *) 0;

   pat2_block at_t_thing("");

   for (;;) {
      if (*pat1 == 0) {
         /* PAT1 has run out, get a string from PAT2 */
         if (pat2_concept) {
            if (user) {
               // We have processed a concept.  Scan for concepts and calls after same.
               m_current_result->match.concept_ptr = pat2_concept;
               scan_concepts_and_calls(user, " ", pat2,
                                       &m_current_result->real_next_subcall, patxi);
            }

            pat2 = (pat2_block *) 0;
            pat2_concept = (concept_descriptor *) 0;
         }
         else if (pat2) {
            // We don't allow a closing bracket after a concept.  That is,
            // stuff in brackets must be zero or more concepts PLUS A CALL.

            if (m_current_result->match.kind != ui_call_select && pat2->demand_a_call)
               goto getout;

            if (pat2->folks_to_restore) {
               // Be sure maximum yield depth gets propagated back.
               pat2->folks_to_restore->yield_depth = m_current_result->yield_depth;
               m_current_result = pat2->folks_to_restore;
            }

            pat1 = pat2->car;
            pat2_concept = pat2->special_concept;

            if (pat2->anythingers) {
               if (!user || !user[0]) goto yesyes;
               if (user[0] != ']') goto nono;
               if (foobar(&user[1])) goto yesyes;
               else goto nono;

            yesyes:
               pat1 = "] er's";
            nono: ;
            }

            pat2 = pat2->cdr;
            continue;
         }
      }

      if (user && (*user == '\0')) {
         // We have just reached the end of the user input.
         Cstring p = pat1;

         while (p[0] == '@') {
            switch (p[1]) {
               case 'S':
               case 'O':
                  m_space_ok = true;
                  // FALL THROUGH!
               case 'M':
               case 'I':
               case 'C':
               case 'G':
               case 'r':
               case 's':
                  // FELL THROUGH!
                  p += 2;
                  continue;
               case 'n':
                  p += 2;
                  while (*p++ != '@');
                  p++;
                  continue;
            }

            break;
         }

         while (p[0] == ',' || p[0] == '\'') p++;
         if (*p == ' ' || *p == '-')
            m_space_ok = true;

         if (!pat2 && *pat1 == '\0') {
            // Exact match.
            m_full_extension[patxi] = '\0';
            record_a_match();
            break;
         }

         // We need to look at the rest of the pattern because
         // if it contains wildcards, then there are multiple matches.
         user = (Cstring) 0;
      }
      else {
         char p = *pat1++;

         // Check for expansion of wildcards.  But if we are just listing
         // the matching commands, there is no point in expanding wildcards
         // that are past the part that matches the user input.

         if (p == '@') {
            match_wildcard(user, pat1, pat2, patxi, pat2_concept);

            if (user==0) {
               // User input has run out, just looking for more wildcards.
               Cstring ep = get_escape_string(*pat1++);

               if (ep && *ep) {
                  ::strcpy(&m_full_extension[patxi], ep);
                  patxi += ::strlen(ep);
               }
               else {
                  if (ep) {
                     while (*pat1++ != '@');
                     pat1++;
                  }

                  // Don't write duplicate blanks.
                  if (*pat1 == ' ' && patxi > 0 && m_full_extension[patxi-1] == ' ') pat1++;
               }
            }
            else {
               char u = *user++;
               char key = *pat1++;
               Cstring ep = get_escape_string(key);

               if (u == '<') {
                  if (ep && *ep) {
                     int i;

                     for (i=1; ep[i]; i++) {
                        if (!user[i-1]) {
                           while (ep[i]) { m_full_extension[patxi++] = ep[i] ; i++; }
                           user = 0;
                           if (key == 'T') goto yes;
                           goto cont;
                        }

                        if (user[i-1] != tolower(ep[i])) goto getout;
                     }

                     user += ::strlen((char *) ep)-1;

                     if (key == 'T' && (!user || foobar(user))) goto yes;
                     goto cont;

                  yes:
                     at_t_thing.car = pat1;
                     pat1 = " er's";
                     at_t_thing.cdr = pat2;
                     pat2 = &at_t_thing;

                  cont: ;
                  }
                  else
                     break;
               }
               else {
                  if (ep && *ep)
                     break;   // Pattern has "<...>" thing, but user didn't type "<".
                  else {
                     if (ep) {
                        while (*pat1++ != '@');
                        pat1++;
                     }

                     user--;    /* Back up! */

                     // Check for something that would cause the pattern effectively to have
                     // two consecutive blanks, and compress them to one.  This can happen
                     // if the pattern string has lots of confusing '@' escapes.
                     if (*pat1 == ' ' && user[-1] == ' ') pat1++;
                  }
               }
            }
         }
         else {
            if (user==0) {
               // User input has run out, just looking for more wildcards.

               if (p) {
                  // There is more pattern.
                  m_full_extension[patxi++] = tolower(p);
               }
               else {
                  // Reached the end of the pattern.

                  m_full_extension[patxi] = '\0';

                  if (!pat2) {
                     record_a_match();
                     break;
                  }
               }
            }
            else {
               char u = *user++;

               if (u != p && (p > 'Z' || p < 'A' || u != p+'a'-'A')) {
                  // If user said "wave based" instead of "wave-based", just continue.
                  if (p != '-' || u != ' ') {
                     // Also, if user left out apostrophe or comma, just continue.
                     if (p == '\'' || p == ',') user--;
                     else break;
                  }
               }
            }
         }
      }
   }

 getout:

   m_current_result = save_current;
}


#define SPIFFY_PARSER

#ifdef SPIFFY_PARSER
static int spiffy_parser = 1;
#else
static int spiffy_parser = 0;
#endif




void matcher_class::scan_concepts_and_calls(
   Cstring user,
   Cstring firstchar,
   pat2_block *pat2,
   const match_result **fixme,
   int patxi)
{
   match_result local_result = *m_current_result;
   match_result *saved_cur_res_ptr = m_current_result;
   if (firstchar[0] == '[')
      local_result.recursion_depth++;

   *fixme = &local_result;

   int i;
   int bucket;
   int new_depth;
   bool using_hash = false;

   /* We force any call invoked under a concept to yield if it is ambiguous.  This way,
      if the user types "cross roll", preference will be given to the call "cross roll",
      rather than to the modifier "cross" on the call "roll".  This whole thing depends,
      of course, on our belief that the language of square dance calling never contains
      an ambiguous call/concept utterance.  For example, we do not expect anyone to
      invent a call "and turn" that can be used with the "cross" modifier. */
   new_depth = m_current_result->yield_depth+1;
   local_result.indent = false;
   local_result.real_next_subcall = (match_result *) 0;
   local_result.real_secondary_subcall = (match_result *) 0;

   /* We know that user is never nil when this procedure is called.
      We also know the firstchar is just a single character, and that
      that character is blank or left bracket. */

   /* Given the above, we know that, in the next 2 loops, any iteration cycle will do nothing if:

      user[0] != NUL and user[0] != firstchar[0]

                  OR

      (call_or_concept_name has 1st 3 chars with no comma, quote, atsign, or NUL)
         AND
      (user[1,2,3] are not NUL or blank and don't match call_or_concept_name)

      */

   pat2_block p2b("", pat2);

   const match_result *save_stuff1 = (const match_result *) 0;
   const match_result *save_stuff2 = (const match_result *) 0;

   int matches_as_seen_by_me = m_match_count;

   if (user[0] && user[0] != firstchar[0])
      goto getout;

   if (m_current_result->match.kind == ui_call_select) {
      save_stuff1 = m_current_result->real_next_subcall;
      save_stuff2 = m_current_result->real_secondary_subcall;
   }

   // See if we can use the hash mechanism.

   if (user[0] || !m_showing) {
      // We now know that it will do nothing if:
      // (call_or_concept_name has 1st 3 chars with no comma, quote, atsign, or NUL)
      //                         AND
      // (user[1,2,3] are not NUL or blank and don't match call_or_concept_name)

      if (user[1] == '[' || get_hash(&user[1], &bucket) == 1) {
         if (user[1] == '[') bucket = BRACKET_HASH;

         /* Therefore, we should only search for those calls that either
            (1) have comma/quote/atsign/NULL in 1st 3 characters, OR
            (2) match user, that is, match the hash number we just computed.
         */

         using_hash = true;
      }
   }

   // Now figure out how to scan the concepts.

   index_list *list_to_use;

   if (using_hash)
      list_to_use = allowing_all_concepts ? &conc_hashers[bucket] : &conclvl_hashers[bucket];
   else
      list_to_use = allowing_all_concepts ? &m_concept_list : &m_level_concept_list;

   local_result.match.kind = ui_concept_select;

   for (i = 0; i < list_to_use->the_list_size; i++) {
      // Don't waste time after user stops us.
      if (m_showing && m_showing_has_stopped) break;

      const concept_descriptor *this_concept = access_concept_descriptor_table(list_to_use->the_list[i]);
      local_result.match.concept_ptr = this_concept;
      p2b.special_concept = this_concept;
      p2b.car = get_concept_name(this_concept);
      m_current_result = &local_result;
      m_current_result->yield_depth = new_depth;
      local_result.match.call_conc_options.initialize();
      match_suffix_2(user, firstchar, &p2b, patxi);
   }

   // And the calls.

   int menu_length;

   if (using_hash)
      menu_length = call_hashers[bucket].the_list_size;
   else
      menu_length = number_of_calls[call_list_any];

   p2b.special_concept = (concept_descriptor *) 0;
   local_result.match.kind = ui_call_select;

   {
      bool got_aborted_subcall = false;
      modifier_block *got_matched_subcall = (modifier_block *) 0;
      int my_patxi = 0;

      for (i = 0; i < menu_length; i++) {
         // Don't waste time after user stops us.
         if (m_showing && m_showing_has_stopped) break;

         call_with_name *this_call =
            main_call_lists[call_list_any][using_hash ? call_hashers[bucket].the_list[i] : i];
         local_result.match.call_conc_options = m_current_result->match.call_conc_options;
         m_current_result = &local_result;
         m_current_result->match.call_ptr = this_call;
         matches_as_seen_by_me = m_match_count;

         p2b.car = get_call_name(this_call);
         // By pushing the yield depth up by 2, we make it yield to things that are actually bettter by 1.
         // This means "6x2 acey deucey", while it is a single call that can be clicked from the menu,
         // will yield to "6x2" + "acey deucey", making it possible to parse "echo 6x2 acey deucey".
         m_current_result->yield_depth = ((get_yield_if_ambiguous_flag(this_call)) ? new_depth+2 : new_depth);

         if (spiffy_parser && get_call_name(this_call)[0] == '@' &&
             (get_call_name(this_call)[1] == '0' || get_call_name(this_call)[1] == 'T')) {
            if (got_aborted_subcall) {
               // We have seen another "@0" call after having seen one whose subcall
               // was incomplete.  There can't possibly be any benefit from parsing
               // further such calls.
               m_match_count++;
               continue;
            }
            else if (got_matched_subcall) {
               p2b.car += 2;    // Skip over the "@0".
               match_suffix_2((Cstring) 0, "", &p2b, patxi+my_patxi);
            }
            else {
               match_suffix_2(user, firstchar, &p2b, patxi);
            }
         }
         else {
            match_suffix_2(user, firstchar, &p2b, patxi);
         }

         // See if any new matches have come up that can allow us to curtail the scan.
         if (spiffy_parser && m_match_count > matches_as_seen_by_me) {
            if (m_only_extension &&
                get_call_name(this_call)[0] == '@' &&
                (get_call_name(this_call)[1] == '0' || get_call_name(this_call)[1] == 'T') &&
                m_user_input[0] == '[') {   // ***** This is probably wrong!!!!

               int full_bracket_depth = m_user_bracket_depth + m_extended_bracket_depth;

               if (local_result.recursion_depth < full_bracket_depth) {
                  // The subcall was aborted.  Processing future "@0" calls
                  // can't possibly get nontrivial results, other than to bump
                  // the match count.
                  if (!m_verify && !m_showing &&
                      m_final_result.match.packed_next_conc_or_subcall &&
                      !m_final_result.match.packed_secondary_subcall)
                     got_aborted_subcall = true;
               }
               else if (!got_matched_subcall) {
                  // The subcall was completely parsed.
                  // We should take the subcall (if there is one), and simply
                  // plug it in, without parsing it again, for all future "@0" calls.
                  if (!m_verify && !m_showing &&
                      m_final_result.match.packed_next_conc_or_subcall &&
                      !m_final_result.match.packed_secondary_subcall) {

                     // Let's try this.  We're only going to look at stuff for which
                     // the user input stopped in the middle of the subcall, for now.
                     if (m_user_bracket_depth-local_result.recursion_depth > 0) {
                        // Find out where, in the full extension, the bracket count
                        // went down to zero.
                        int jj = m_user_bracket_depth-local_result.recursion_depth;

                        for (my_patxi = 0 ; ; my_patxi++) {
                           if (!m_full_extension[my_patxi] || jj == 0) break;
                           else if (m_full_extension[my_patxi] == '[') jj++;
                           else if (m_full_extension[my_patxi] == ']') jj--;
                        }
                        // Now my_patxi tells where the close bracket was.

                        got_matched_subcall = m_final_result.match.packed_next_conc_or_subcall;
                     }
                  }
               }
            }
         }
      }
   }

 getout:

   m_current_result = saved_cur_res_ptr;
   m_current_result->real_next_subcall = save_stuff1;
   m_current_result->real_secondary_subcall = save_stuff2;
   m_current_result->indent = false;

   // Clear this stuff -- it points to our local_result.
   *fixme = (match_result *) 0;
}



/*
 * Match_wildcard tests for and handles pattern suffixes that begin with
 * a wildcard such as "<anyone>".  A wildcard is handled only if there is
 * room in the result struct to store the associated value.
 */

void matcher_class::match_wildcard(
   Cstring user,
   Cstring pat,
   pat2_block *pat2,
   int patxi,
   const concept_descriptor *special)
{
   Cstring prefix;
   Cstring *number_table;
   int i;
   uint32_t iu;
   char crossname[80];
   char *crossptr;
   int concidx;
   Cstring pattern;
   char key = *pat++;
   pat2_block p2b(pat, pat2);
   p2b.special_concept = special;

   // If we are just listing the matching commands, there is no point in expanding
   // wildcards that are past the part that matches the user input.  That is why we test
   // that "user" is not null.

   if (user) {
      switch (key) {
      case '6': case 'k': case 'K': case 'V':
         // Start recursion.
         m_current_result->match.call_conc_options.who.who_stack_ptr++;

         // Don't allow recursion deeper than 2 (though the array is big enough for 3.)
         // This would occur if someone did "girl-based triangle-based triangle circulate".
         if (m_current_result->match.call_conc_options.who.who_stack_ptr < who_list::who_stack_size) {
            for (i=1; i<selector_INVISIBLE_START; i++) {
               if (key != 'K' && i >= selector_SOME_START)
                  continue;
               if (key == 'V' && (i == selector_centers || i == selector_ends || i == selector_outsides))
                  continue;

               m_current_result->match.call_conc_options.who.who[m_current_result->match.call_conc_options.who.who_stack_ptr-1] = (selector_kind) i;
               {
                  match_result *save_current = m_current_result;
                  match_suffix_2(user,
                                 (key == 'k') ? selector_list[i].sing_name : selector_list[i].name,
                                 &p2b, patxi); m_current_result = save_current;
               }
            }

            m_current_result->match.call_conc_options.who.who[m_current_result->match.call_conc_options.who.who_stack_ptr-1] =
               selector_uninitialized;
         }

         // Restore.
         m_current_result->match.call_conc_options.who.who_stack_ptr--;
         return;
      case '0': case 'T': case 'm':
         if (*user == '[') {
            pat2_block p3b("]", &p2b);
            p3b.folks_to_restore = m_current_result;
            p3b.demand_a_call = true;
            p3b.anythingers = (key == 'T');

            scan_concepts_and_calls(user, "[", &p3b,
                                    ((key == 'm') ?
                                     &m_current_result->real_secondary_subcall :
                                     &m_current_result->real_next_subcall),
                                    patxi);
            return;
         }
         break;
      case 'h':
         if (m_current_result->match.call_conc_options.where == direction_uninitialized) {
            direction_kind save_where = m_current_result->match.call_conc_options.where;

            for (i=1; i<=last_direction_kind; ++i) {
               m_current_result->match.call_conc_options.where = (direction_kind) i;
               match_suffix_2(user, direction_names[i].name, &p2b, patxi);
            }

            m_current_result->match.call_conc_options.where = save_where;
            return;
         }
         break;
      case 'v': case 'w': case 'x': case 'y':

         // We don't allow this if we are already doing a tagger.  It won't happen
         // in any case, because we have taken out "revert <atc>" and "reflected <atc>"
         // as taggers.

         if (m_current_result->match.call_conc_options.tagger == 0) {
            int tagclass = 0;

            if (key == 'w') tagclass = 1;
            else if (key == 'x') tagclass = 2;
            else if (key == 'y') tagclass = 3;

            m_current_result->match.call_conc_options.tagger = tagclass << 5;

            for (iu=0; iu<number_of_taggers[tagclass]; iu++) {
               m_current_result->match.call_conc_options.tagger++;
               match_suffix_2(user, get_call_name(tagger_calls[tagclass][iu]), &p2b, patxi);
            }

            m_current_result->match.call_conc_options.tagger = 0;
            return;
         }
         break;
      case 'N':
         if (m_current_result->match.call_conc_options.circcer == 0) {
            char circname[80];
            uint32_t save_circcer = m_current_result->match.call_conc_options.circcer;

            for (iu=0; iu<number_of_circcers; ++iu) {
               const char *fromptr = get_call_name(circcer_calls[iu].the_circcer);
               char *toptr = circname;
               char c;
               do {
                  c = *fromptr++;
                  if (c == '@') {
                     if (*fromptr++ == 'O') {
                        while (*fromptr++ != '@') ;
                        fromptr++;
                     }
                  }
                  else
                     *toptr++ = c;
               } while (c);

               m_current_result->match.call_conc_options.circcer++;
               match_suffix_2(user, circname, &p2b, patxi);
            }

            m_current_result->match.call_conc_options.circcer = save_circcer;
            return;
         }
         break;
      case '9':
         if (*user < '0' || *user > '9') return;
         number_table = cardinals;
         goto do_number_stuff;
      case 'u':
         if (*user < '0' || *user > '9') return;
         number_table = ordinals;
         goto do_number_stuff;
      case 'a': case 'b': case 'B': case 'D':
         if ((*user >= '0' && *user <= '9') ||
             *user == 'q' || *user == 'h' ||
             *user == 't' || *user == 'f') {
            int save_howmanynumbers = m_current_result->match.call_conc_options.howmanynumbers;
            uint32_t save_number_fields = m_current_result->match.call_conc_options.number_fields;

            m_current_result->match.call_conc_options.howmanynumbers++;

            for (i=0 ; (prefix = n_4_patterns[i]) ; i++) {
               if (key != 'D' || (i&1) != 0)
                  match_suffix_2(user, prefix, &p2b, patxi);
               m_current_result->match.call_conc_options.number_fields +=
                  1 << (save_howmanynumbers*BITS_PER_NUMBER_FIELD);
            }

            // Special case: allow "quarter" for 1/4
            m_current_result->match.call_conc_options.number_fields =
               save_number_fields + (1 << (save_howmanynumbers*BITS_PER_NUMBER_FIELD));
            match_suffix_2(user, "quarter", &p2b, patxi);

            // Special case: allow "half" or "1/2" for 2/4
            if (key != 'D') {
               m_current_result->match.call_conc_options.number_fields =
                  save_number_fields + (2 << (save_howmanynumbers*BITS_PER_NUMBER_FIELD));
               match_suffix_2(user, "half", &p2b, patxi);
               match_suffix_2(user, "1/2", &p2b, patxi);
            }

            // Special case: allow "three quarter" for 3/4
            m_current_result->match.call_conc_options.number_fields =
               save_number_fields + (3 << (save_howmanynumbers*BITS_PER_NUMBER_FIELD));
            match_suffix_2(user, "three quarter", &p2b, patxi);

            // Special case: allow "full" for 4/4
            if (key != 'D') {
               m_current_result->match.call_conc_options.number_fields =
                  save_number_fields + (4 << (save_howmanynumbers*BITS_PER_NUMBER_FIELD));
               match_suffix_2(user, "full", &p2b, patxi);
            }

            m_current_result->match.call_conc_options.howmanynumbers = save_howmanynumbers;
            m_current_result->match.call_conc_options.number_fields = save_number_fields;
            return;
         }

         break;
      }
   }

   // The following escape codes are the ones that we print out
   // even if the user input has run out.

   switch (key) {
   case 'S':
      {
         bool saved_indent = m_current_result->indent;
         uint32_t save_number_fields = m_current_result->match.call_conc_options.star_turn_option;
         m_current_result->indent = true;

         m_current_result->match.call_conc_options.star_turn_option = -1;
         match_suffix_2(user, ", don't turn the star", &p2b, patxi);

         m_current_result->match.call_conc_options.star_turn_option = 1;
         match_suffix_2(user, ", turn the star 1/4", &p2b, patxi);
         m_current_result->match.call_conc_options.star_turn_option = 2;
         match_suffix_2(user, ", turn the star 1/2", &p2b, patxi);
         m_current_result->match.call_conc_options.star_turn_option = 3;
         match_suffix_2(user, ", turn the star 3/4", &p2b, patxi);

         m_current_result->match.call_conc_options.star_turn_option = save_number_fields;
         m_current_result->indent = saved_indent;
      }

      return;
   case 'j':
      crossptr = crossname;
      while ((*crossptr++ = *pat++) != '@');
      pat++;
      crossptr--;
      *crossptr = 0;
      pattern = crossname;
      concidx = useful_concept_indices[UC_cross];
      goto do_cross_stuff;
   case 'C':
      pattern = " cross";
      concidx = useful_concept_indices[UC_cross];
      goto do_cross_stuff;
   case 'J':
      crossptr = crossname;
      while ((*crossptr++ = *pat++) != '@');
      pat++;
      crossptr--;
      *crossptr = 0;
      pattern = crossname;
      concidx = useful_concept_indices[UC_magic];
      goto do_cross_stuff;
   case 'Q':
      crossptr = crossname;
      while ((*crossptr++ = *pat++) != '@');
      pat++;
      crossptr--;
      *crossptr = 0;
      pattern = crossname;
      concidx = useful_concept_indices[UC_grand];
      goto do_cross_stuff;
   case 'M':
      concidx = useful_concept_indices[UC_magic];
      pattern = " magic";
      goto do_cross_stuff;
   case 'G':
      concidx = useful_concept_indices[UC_grand];
      pattern = " grand";
      goto do_cross_stuff;
   case 'E':
      crossptr = crossname;
      while ((*crossptr++ = *pat++) != '@');
      pat++;
      crossptr--;
      *crossptr = 0;
      pattern = crossname;
      concidx = useful_concept_indices[UC_intlk];
      goto do_cross_stuff;
   case 'I':
      {
         char *p = m_full_extension;
         int idx = patxi;
         bool fixing_an_a = true;

         for (i=0 ; i<2 ; i++) {
            idx--;
            if (idx < 0) { idx = m_user_input_size-1 ; p = m_user_input; }
            if (p[idx] != "a "[i]) { fixing_an_a = false; break; }
         }

         if (fixing_an_a || (user && user[-1] == 'a' && user[-2] == ' '))
            pattern = "n interlocked";
         else
            pattern = " interlocked";

         concidx = useful_concept_indices[UC_intlk];
      }
      goto do_cross_stuff;
   case 'e':
      while (*pat++ != '@');
      pat++;
      pattern = "left";
      concidx = useful_concept_indices[UC_left];
      goto do_cross_stuff;
   }

   return;

   do_number_stuff:

   {
      int save_howmanynumbers = m_current_result->match.call_conc_options.howmanynumbers;
      uint32_t save_number_fields = m_current_result->match.call_conc_options.number_fields;

      m_current_result->match.call_conc_options.howmanynumbers++;

      for (i=0 ; (prefix = number_table[i]) ; i++) {
         match_suffix_2(user, prefix, &p2b, patxi);
         m_current_result->match.call_conc_options.number_fields +=
            1 << (save_howmanynumbers*BITS_PER_NUMBER_FIELD);
      }

      m_current_result->match.call_conc_options.howmanynumbers = save_howmanynumbers;
      m_current_result->match.call_conc_options.number_fields = save_number_fields;
      return;
   }

 do_cross_stuff:

   {
      match_result *saved_cross_ptr = m_current_result;
      match_result saved_cross_result = *m_current_result;

      m_current_result->match.kind = ui_concept_select;
      m_current_result->match.call_conc_options.initialize();
      m_current_result->match.concept_ptr = access_concept_descriptor_table(concidx);
      m_current_result->real_next_subcall = &saved_cross_result;
      m_current_result->indent = true;
      p2b.car = pat;

      m_current_result = &saved_cross_result;
      match_suffix_2(user, pattern, &p2b, patxi);
      m_current_result = saved_cross_ptr;
      *m_current_result = saved_cross_result;
   }
}


/*
 * Match_pattern tests the user input against a pattern (pattern)
 * that may contain wildcards (such as "<anyone>").  Pattern matching is
 * case-sensitive.  As currently used, the user input and the pattern
 * are always given in all lower case.
 * If the input is equivalent to a prefix of the pattern,
 * a match is recorded in the search state.
 */

void matcher_class::match_pattern(Cstring pattern)
{
   pat2_block p2b(pattern);
   match_suffix_2(m_user_input, "", &p2b, 0);
}

void matcher_class::search_menu(uims_reply_kind kind)
{
   unsigned int i, menu_length;
   Cstring *menu;
   char uch = m_user_input[0];
   bool input_is_null = uch == '\0' && !m_showing;

   m_current_result = &m_active_result;

   m_current_result->recursion_depth = 0;
   m_current_result->valid = true;
   m_current_result->exact = false;
   m_current_result->match.kind = kind;
   m_current_result->match.call_conc_options.initialize();
   m_current_result->indent = false;
   m_current_result->real_next_subcall = (match_result *) 0;
   m_current_result->real_secondary_subcall = (match_result *) 0;
   m_current_result->yield_depth = 0;

   if (kind == ui_call_select) {
      menu_length = number_of_calls[m_call_menu];

      if (input_is_null)
         m_match_count += menu_length;
      else {
         int matches_as_seen_by_me;
         bool got_aborted_subcall = false;
         modifier_block *got_matched_subcall = (modifier_block *) 0;
         int my_patxi = 0;

         for (i = 0; i < menu_length; i++) {
            // Don't waste time after user stops us.
            if (m_showing && m_showing_has_stopped) break;

            call_with_name *this_call = main_call_lists[m_call_menu][i];

            // Do a quick check for mismatch on first character.
            // Q: Why do we do it just at the top level?  Shouldn't
            //    we do it at all levels?
            // A: At deeper levels, the hashing mechanism has cut the
            //    list way down, so we don't need it.  But here we don't
            //    have hashing.
            // Q: Why not?
            // A: At the top level, we have many different menus to deal
            //    with, one for each possible starting setup.  Making a hash
            //    table for each of them is unwieldy.  At deeper levels,
            //    there is just the "call_list_any" menu to deal with, and
            //    that one is hashed.
            char pch = get_call_name(this_call)[0];

            if (!m_showing &&
                uch != pch &&
                (pch > 'Z' || pch < 'A' || uch != pch+'a'-'A') &&
                ((pch != '@') ||
                 ((get_call_name(this_call)[1] == '0' || get_call_name(this_call)[1] == 'T') && uch != '[' && uch != '<')))
               continue;

            parse_state.call_list_to_use = (call_list_kind) m_call_menu;
            m_active_result.match.call_ptr = this_call;
            m_active_result.yield_depth = (get_yield_if_ambiguous_flag(this_call)) ? 1 : 0;
            matches_as_seen_by_me = m_match_count;

            pat2_block p2b(get_call_name(this_call));

            if (spiffy_parser && get_call_name(this_call)[0] == '@' && (get_call_name(this_call)[1] == '0' || get_call_name(this_call)[1] == 'T')) {
               if (got_aborted_subcall) {
                  // We have seen another "@0" call after having seen one whose subcall
                  // was incomplete.  There can't possibly be any benefit from parsing
                  // further such calls.
                  m_match_count++;
                  continue;
               }
               else if (got_matched_subcall) {
                  p2b.car += 2;    // Skip over the "@0".
                  match_suffix_2((Cstring) 0, "", &p2b, my_patxi);
               }
               else {
                  match_suffix_2(m_user_input, "", &p2b, 0);
               }
            }
            else {
               match_suffix_2(m_user_input, "", &p2b, 0);
            }

            // See if any new matches have come up that can allow us to curtail the scan.
            if (spiffy_parser && m_match_count > matches_as_seen_by_me) {
               if (m_only_extension &&
                   get_call_name(this_call)[0] == '@' &&
                   (get_call_name(this_call)[1] == '0' || get_call_name(this_call)[1] == 'T') &&
                   uch == '[') {
                  int full_bracket_depth = m_user_bracket_depth + m_extended_bracket_depth;
                  if (0 < full_bracket_depth) {
                     // The subcall was aborted.  Processing future "@0" calls
                     // can't possibly get nontrivial results, other than to bump
                     // the match count.
                     if (!m_verify && !m_showing &&
                         m_final_result.match.packed_next_conc_or_subcall &&
                         !m_final_result.match.packed_secondary_subcall)
                        got_aborted_subcall = true;
                  }
                  else if (!got_matched_subcall) {
                     // The subcall was completely parsed.
                     // We should take the subcall (if there is one), and simply
                     // plug it in, without parsing it again, for all future "@0" calls.
                     if (!m_verify && !m_showing &&
                         m_final_result.match.packed_next_conc_or_subcall &&
                         !m_final_result.match.packed_secondary_subcall) {

                        // Let's try this.  We're only going to look at stuff for which
                        // the user input stopped in the middle of the subcall, for now.
                        if (m_user_bracket_depth > 0) {
                           // Find out where, in the full extension, the bracket count
                           // went down to zero.
                           int jj = m_user_bracket_depth;

                           for (my_patxi = 0 ; ; my_patxi++) {
                              if (!m_full_extension[my_patxi] || jj == 0) break;
                              else if (m_full_extension[my_patxi] == '[') jj++;
                              else if (m_full_extension[my_patxi] == ']') jj--;
                           }
                           // Now my_patxi tells where the close bracket was.

                           got_matched_subcall = m_final_result.match.packed_next_conc_or_subcall;
                        }
                     }
                  }
               }
            }
         }
      }
   }
   else if (kind == ui_concept_select) {
      index_list *list_to_use = allowing_all_concepts ? &m_concept_list : &m_level_concept_list;
      menu_length = list_to_use->the_list_size;
      short int *item = list_to_use->the_list;

      if (input_is_null)
         m_match_count += menu_length;
      else {
         for (i = 0; i < menu_length; i++) {
            // Don't waste time after user stops us.
            if (m_showing && m_showing_has_stopped) break;

            const concept_descriptor *this_concept = access_concept_descriptor_table(item[i]);

            // Another quick check -- there are hundreds of concepts.
            char pch = get_concept_name(this_concept)[0];

            if (!m_showing &&
                uch != pch &&
                (pch > 'Z' || pch < 'A' || uch != pch+'a'-'A') &&
                ((pch != '@') ||
                 (get_concept_name(this_concept)[1] == '0' && uch != '[' && uch != '<')))
               continue;

            parse_state.call_list_to_use = (call_list_kind) m_call_menu;
            m_active_result.match.concept_ptr = this_concept;
            m_active_result.yield_depth =
               (get_concparseflags(this_concept) & CONCPARSE_YIELD_IF_AMB) ? 1 : 0;

            pat2_block p2b(get_concept_name(this_concept));
            p2b.special_concept = this_concept;
            match_suffix_2(m_user_input, "", &p2b, 0);
         }
      }
   }
   else if (m_call_menu >= e_match_taggers &&
            m_call_menu < e_match_taggers+NUM_TAGGER_CLASSES) {
      int tagclass = m_call_menu - e_match_taggers;
      m_active_result.match.call_conc_options.tagger = tagclass << 5;

      if (input_is_null)
         m_match_count += number_of_taggers[tagclass];
      else {
         for (i = 0; i < number_of_taggers[tagclass]; i++) {
            m_active_result.match.call_conc_options.tagger++;
            match_pattern(get_call_name(tagger_calls[tagclass][i]));
         }
      }
   }
   else if (m_call_menu == e_match_circcer) {
      m_active_result.match.call_conc_options.circcer = 0;

      if (input_is_null)
         m_match_count += number_of_circcers;
      else {
         for (i = 0; i < number_of_circcers; i++) {
            m_active_result.match.call_conc_options.circcer++;
            match_pattern(get_call_name(circcer_calls[i].the_circcer));
         }
      }
   }
   else {
      if (kind == ui_command_select) {
         menu = command_commands;
         menu_length = num_command_commands;
      }
      else if (m_call_menu == e_match_directions) {
         menu = &direction_menu_list[1];
         menu_length = last_direction_kind;
      }
      else if (m_call_menu == e_match_number) {
         menu = cardinals;
         menu_length = NUM_CARDINALS;
      }
      else if (m_call_menu == e_match_selectors) {
         menu = selector_menu_list;
         // Menu is shorter than it appears, because we are skipping first item.
         menu_length = selector_INVISIBLE_START-1;
      }
      else if (m_call_menu == e_match_startup_commands) {
         kind = ui_start_select;
         menu = startup_commands;
         menu_length = num_startup_commands;
      }
      else if (m_call_menu == e_match_resolve_commands) {
         kind = ui_resolve_select;
         menu = resolve_command_strings;
         menu_length = number_of_resolve_commands;
      }

      m_active_result.match.kind = kind;

      if (input_is_null)
         m_match_count += menu_length;
      else {
         for (i = 0; i < menu_length; i++) {
            m_active_result.match.index = i;
            m_active_result.yield_depth = 0;
            match_pattern(menu[i]);
         }
      }
   }
}


/*
 * (These comments are extremely old, and are somewhat obsolete.)
 *
 * MATCH_USER_INPUT is the basic command matching function.
 *
 * The WHICH_COMMANDS parameter determines which list of commands
 * is matched against. Possibilities include:
 *
 * match_startup_commands - startup mode commands
 * match_resolve_commands - resolve mode commands
 * match_selectors        - selectors (e.g. "boys")
 * match_directions       - directions (e.g. "left")
 * <call menu index>      - normal commands, concepts, and calls
 *                          from specified call menu
 *
 * The USER_INPUT parameter is the user-specified string that is being
 * matched.
 *
 * MATCH_USER_INPUT returns the number of commands that are
 * compatible with the specified user input.  A command is compatible
 * if it is matches the user input as is or the user input could
 * be extended to match the command.
 *
 * If there is at least
 * one compatible command, then the maximal extension to the user
 * input that is compatible with all the compatible commands is returned
 * in EXTENSION; otherwise EXTENSION is cleared.
 * If there is at least one compatible command, then
 * a description of one such command is returned in MR.  EXTENSION
 * and MR can be NULL, if the corresponding information is not needed.
 *
 * MR->VALID is true if and only if there is at least one matching command.
 * MR->EXACT is true if and only if one or more commands exactly matched the
 * input (no extension required).  The remainder of MR is the
 * data to be returned to sd from one or more invocations of
 * user interface functions.
 *
 * The SF parameter is used to display a list of matching
 * commands. If SF is non-zero, then it must be a function pointer
 * that accepts two char * parameters and a match_result * parameter.
 * That function will be invoked
 * for each matching command.  The first parameter is the user input,
 * the second parameter is the extension corresponding to the command.
 * When showing matching commands,
 * wildcards are not expanded unless they are needed to match the
 * user input.  The extension may contain wildcards.  The third
 * parameter is the (volatile) match_result that describes the
 * matching command.  If SHOW_VERIFY is nonzero, then an attempt
 * will be made to verify that a call is possible in the current
 * setup before showing it.
 *
 * When searching for calls and concepts, it also searches for the special
 * commands provided by the "command_list" and "num_commands" arguments.
 * When such a command is found, we return "ui_command_select" as the kind,
 * and the index into that array as the index.
 *
 */

int matcher_class::match_user_input(
   int which_commands,
   bool show,
   bool show_verify,
   bool only_want_extension)
{
   initialize_for_parse(which_commands, show, only_want_extension);

   m_active_result.valid = false;
   m_active_result.exact = false;

   // Count the bracket depth of the user's part of the line.
   for (const char *p = m_user_input ; *p ; p++) {
      if (*p == '[') m_user_bracket_depth++;
      else if (*p == ']') m_user_bracket_depth--;
   }

   if (m_call_menu >= 0) {
      m_verify = show_verify;
      search_menu(ui_call_select);
      search_menu(ui_concept_select);
      m_verify = false;
      search_menu(ui_command_select);
   }
   else
      search_menu(ui_special_concept);

   return m_match_count;
}
