// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

#ifndef SDMATCH_H
#define SDMATCH_H

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2021  William B. Ackerman.
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


// This class is a singleton object.  It gets "cleaned", but not reconstructed, for every parse,
// by initialize_for_parse, except for s_modifier_active_list and s_modifier_inactive_list,
// which get initialized just once and get manipulated through the life of the program.

// Constructor.
class SDLIB_API matcher_class {
public:

   // Must be a power of 2.
   enum {
      NUM_NAME_HASH_BUCKETS = 128,
      BRACKET_HASH = (NUM_NAME_HASH_BUCKETS+1)
   };

   // These negative values to the call menu type, which tells what menu we are to pick from.
   enum {
      e_match_startup_commands = -1,
      e_match_resolve_commands = -2,
      e_match_selectors = -3,
      e_match_directions = -4,
      e_match_taggers = -8,      // This embraces 4 (NUM_TAGGER_CLASSES) numbers: -8, -7, -6, and -5.
      e_match_circcer = -9,
      e_match_number = -10       // Only used by sdui-win; sdui-tty just reads it and uses atoi.
   };

   void initialize_for_parse(int which_commands, bool show, bool only_want_extension);

   void copy_sublist(const match_result *outbar, modifier_block *tails);

   void copy_to_user_input(const char *stuff);

   void erase_matcher_input();

   int delete_matcher_word();

   bool process_accel_or_abbrev(modifier_block & mb, char linebuff[]);

   void do_accelerator_spec(Cstring inputline, bool is_accelerator);

   bool verify_call();

   void record_a_match();

   void match_pattern(Cstring pattern);

   void match_suffix_2(Cstring user, Cstring pat1, pat2_block *pat2, int patxi);

   void scan_concepts_and_calls(
      Cstring user,
      Cstring firstchar,
      pat2_block *pat2,
      const match_result **fixme,
      int patxi);

   void match_wildcard(
      Cstring user,
      Cstring pat,
      pat2_block *pat2,
      int patxi,
      const concept_descriptor *special);

   void search_menu(uims_reply_kind kind);

   int match_user_input(
      int which_commands,
      bool show,
      bool show_verify,
      bool only_want_extension);

   // These are used for allocating and "garbage collecting" blocks dynamically.
   // If this were not a singleton object, these two fields would have to be declared static,
   // because they are program-static.  So we use the "s_" prefix.
   modifier_block *s_modifier_active_list;
   modifier_block *s_modifier_inactive_list;

   // This is the structure that gets manipulated as parsing and matching proceed.
   match_result m_active_result;

   // This usually points to m_active_result, and is used for manipulations deep inside
   // the matching code.  It occasionally get pointed at other things in the matching code,
   // but is always set back.
   match_result *m_current_result;

   // When a parse is completed, the result information for the user is copied out to this structure.
   // Things in m_active_result, including the all-important "match.kind" and "match.index" fields,
   // were manipulated as menu searches were performed, and aren't left with the right result.
   // But if a unique match is found, it is copied out to this field.
   match_result m_final_result;

   bool m_only_extension;          // Only want extension, short-circuit the search.
   int m_user_bracket_depth;
   int m_match_count;              // The number of matches so far.
   int m_exact_count;              // The number of exact matches so far.
   bool m_showing;                 // We are only showing the matching patterns.
   bool m_showing_has_stopped;
   bool m_verify;                  // True => verify calls before showing.
   int m_lowest_yield_depth;
   int m_call_menu;       // The call menu (or special negative command) that we are searching.

   int m_extended_bracket_depth;
   bool m_space_ok;
   int m_yielding_matches;
   char m_user_input[INPUT_TEXTLINE_SIZE+1];     // the current user input
   char m_full_extension[INPUT_TEXTLINE_SIZE+1]; // the extension for the current pattern
   char m_echo_stuff[INPUT_TEXTLINE_SIZE+1];     // the maximal common extension
   int m_user_input_size;                        // This is always equal to strlen(m_user_input).

   // Things below here are effectively "static constants".  They are filled in by
   // matcher_initialize and open_session at program startup.

   index_list m_concept_list;        // indices of all concepts
   index_list m_level_concept_list;  // indices of concepts valid at current level

   index_list *call_hashers;
   index_list *conc_hashers;
   index_list *conclvl_hashers;

   modifier_block **m_fcn_key_table_normal;
   modifier_block **m_fcn_key_table_start;
   modifier_block **m_fcn_key_table_resolve;

   abbrev_block *m_abbrev_table_normal;
   abbrev_block *m_abbrev_table_start;
   abbrev_block *m_abbrev_table_resolve;

   matcher_class();      // Constructor and destructor are in sdmatch.cpp.
   ~matcher_class();
};


#endif   /* SDMATCH_H */
