// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2021  William B. Ackerman.
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

/* This defines the following functions:
   expand::compress_setup
   expand::expand_setup
   expand::fix_3x4_to_qtag
   update_id_bits
   clear_bits_for_update
   clear_absolute_proximity_bits
   clear_absolute_proximity_and_facing_bits
   put_in_absolute_proximity_and_facing_bits
   expand::initialize
   full_expand::initialize_touch_tables
   full_expand::search_table_1
   full_expand::search_table_2
   full_expand::search_table_3
   big_endian_get_directions
   touch_or_rear_back
   do_matrix_expansion
   initialize_sdlib
   check_for_concept_group
   crash_print
   fail
   fail_no_retry
   fail2
   failp
   specialfail
   saved_error_info::collect
   saved_error_info::throw_saved_error
   warn
   verify_restriction
   assoc
   uncompress_position_number
   clear_result_flags
   setup::setup    // big constructor.
   clear_result
   rotperson
   rotcw
   rotccw
   copy_person
   copy_rot
   install_person
   install_rot
   scatter
   gather
   install_scatter
   warn_about_concept_level
   turn_4x4_pinwheel_into_c1_phantom
   clean_up_unsymmetrical_setup
   process_final_concepts
   really_skip_one_concept
   fix_n_results
   warnings_are_unacceptable
   normalize_setup
   check_concept_parse_tree
   check_for_centers_concept
   toplevelmove
   finish_toplevelmove
   deposit_call_tree
   do_subcall_query
   find_proper_call_list
and the following external variables:
   text_line_count
   error_message1
   error_message2
   collision_person1
   collision_person2
   last_magic_diamond
   configuration::history
   config_history_ptr
   configuration::whole_sequence_low_lim
   history_allocation
   written_history_items
   no_erase_before_this
   written_history_nopic
   the_topcallflags
   there_is_a_call
   base_calls
   ui_options
   enable_file_writing
   cardinals
   ordinals
   getout_strings
   writechar_block
   num_command_commands
   command_commands
   command_command_values
   num_startup_commands
   startup_commands
   startup_command_values
   resolve_command_values
   number_of_resolve_commands
   resolve_command_strings
   glob_abridge_mode
   abs_max_calls
   max_base_calls
   tagger_menu_list
   selector_menu_list
   direction_menu_list
   circcer_menu_list
   tagger_calls
   circcer_calls
   number_of_taggers
   number_of_taggers_allocated
   number_of_circcers
   number_of_circcers_allocated
   parse_state
   current_options
   no_search_warnings
   conc_elong_warnings
   dyp_each_warnings
   useless_phan_clw_warnings
   suspect_destroyline_warnings
   allowing_all_concepts
   allowing_minigrand
   allow_bend_home_getout
   enforce_overcast_warning
   using_active_phantoms
   last_direction_kind
   interactivity
   database_version
   testing_fidelity
   level_threshholds_for_pick
   allowing_modifications
   hashed_randoms
   concept_sublist_sizes
   concept_sublists
   good_concept_sublist_sizes
   good_concept_sublists
   verify_options
   verify_used_number
   verify_used_direction
   verify_used_selector
*/


// For memset, memcpy, and strncpy.
#include <string.h>

#include "sd.h"


// *** There are more globals proclaimed at line 235.
ui_utils *gg77 = 0;
int text_line_count = 0;
char error_message1[MAX_ERR_LENGTH];
char error_message2[MAX_ERR_LENGTH];
uint32_t collision_person1;
uint32_t collision_person2;
int config_history_ptr;
parse_block *last_magic_diamond;
int history_allocation;

/* This tells how much of the history text written to the UI is "safe".  If this
   is positive, the history items up to that number, inclusive, have valid
   "text_line" fields, and have had their text written to the UI.  Furthermore,
   the local variable "text_line_count" is >= any of those "text_line" fields,
   and each "text_line" field shows the number of lines of text that were
   written to the UI when that history item was written.  This variable is
   -1 when none of the history is safe or the state of the text in the UI
   is unknown. */
int written_history_items;

// Normally zero.  When this becomes nonzero, the next clear_screen will
// consider the "screen" to go back only that many lines, so that anything
// before that will never be subject to being erased.
int no_erase_before_this;

// When written_history_items is positive, this tells how many of the initial lines
// did not have pictures forced on (other than having been drawn as a natural consequence
// of the "draw_pic" flag being on.)  If this number ever becomes greater than
// written_history_items, that's OK.  It just means that none of the lines had
// forced pictures.
int written_history_nopic;

// This list tells what level calls will be accepted for the "pick level call"
// operation.  When doing a "pick level call, we don't actually require calls
// to be exactly on the indicated level, as long as it's plausibly close.
//
// BEWARE!!  This list is keyed to the definition of "dance_level" in database.h .
dance_level level_threshholds_for_pick[] = {
   l_mainstream,
   l_plus,
   l_a1,
   l_a1,      // If a2 is given, an a1 call is OK.
   l_c1,
   l_c2,
   l_c3a,
   l_c3a,     // If c3 is given, a c3a call is OK.
   l_c3a,     // If c3x is given, a c3a call is OK.
   l_c3x,     // If c4a is given, a c3x call is OK (unless the "no_c3x" switch has been given.)
   l_c3x,     // Same.
   l_c3x,     // Same.
   l_dontshow,
   l_nonexistent_concept};

// BEWARE!!  This list is keyed to the definition of "dance_level" in database.h .
Cstring getout_strings[] = {
   "Mainstream",
   "Plus",
   "A1",
   "A2",
   "C1",
   "C2",
   "C3A",
   "C3",
   "C3X",
   "C4A",
   "C4",
   "C4X",
   "all",
   ""};

// *** There are more globals proclaimed at line 164.
uint32_t the_topcallflags;
bool there_is_a_call;
call_with_name **base_calls;        // Gets allocated as array of pointers in sdinit.

int configuration::whole_sequence_low_lim;
configuration *configuration::history = (configuration *) 0; // Will be allocated in sdmain.
bool enable_file_writing;


// The variable "last_direction_kind" below, gets manipulated
// at startup in order to remove the "zig-zag" and "the music" at certain levels.

int num_command_commands;     // Size of the command menu.
Cstring *command_commands;
command_kind *command_command_values;
int num_startup_commands;     // Size of the startup menu.
Cstring *startup_commands;
start_select_kind *startup_command_values;
int number_of_resolve_commands;
Cstring *resolve_command_strings;
resolve_command_kind *resolve_command_values;
int abs_max_calls;
int max_base_calls;
Cstring *tagger_menu_list[NUM_TAGGER_CLASSES];
Cstring *selector_menu_list;
Cstring *direction_menu_list;
Cstring *circcer_menu_list;
circcer_object *circcer_calls;
call_with_name **tagger_calls[NUM_TAGGER_CLASSES];
uint32_t number_of_taggers[NUM_TAGGER_CLASSES];
uint32_t number_of_taggers_allocated[NUM_TAGGER_CLASSES];
uint32_t number_of_circcers;
uint32_t number_of_circcers_allocated;
parse_state_type parse_state;
call_conc_option_state current_options;
warning_info no_search_warnings;
warning_info conc_elong_warnings;
warning_info dyp_each_warnings;
warning_info useless_phan_clw_warnings;
warning_info suspect_destroyline_warnings;
bool allowing_minigrand = false;
bool allow_bend_home_getout = false;
bool enforce_overcast_warning = false;
bool using_active_phantoms = false;
bool two_couple_calling = false;
bool expanding_database = false;
int trace_progress = 0;
bool allowing_all_concepts = false;
int allowing_modifications = 0;

int last_direction_kind = direction_ENUM_EXTENT-1;
interactivity_state interactivity = interactivity_normal;
char database_version[81];
bool testing_fidelity = false;

heritflags simple_herit_bits_table[] = {
   INHERITFLAG_CROSSOVER,
   INHERITFLAG_INROLL,
   INHERITFLAG_OUTROLL,
   INHERITFLAG_SPLITTRADE,
   INHERITFLAG_BIAS,
   INHERITFLAG_BIASTRADE,
   INHERITFLAG_ORBIT,
   INHERITFLAG_TWINORBIT,
   INHERITFLAG_ROTARY,
   INHERITFLAG_SCATTER,
   INHERITFLAG_ZOOMROLL
};


int hashed_randoms;


/* These two direct the generation of random concepts when we are searching.
   We make an attempt to generate somewhat plausible concepts, depending on the
   setup we are in.  If we just generated evenly weighted concepts from the entire
   concept list, we would hardly ever get a legal one. */
int concept_sublist_sizes[call_list_extent];
short int *concept_sublists[call_list_extent];
/* These two are similar, but contain only "really nice" concepts that
   we are willing to use in exhaustive searches.  Concepts in these
   lists are very "expensive", in that each one causes an exhaustive search
   through all calls (with nearly-exhaustive enumeration of numbers/selectors, etc).
   Also, these concepts will appear in suggested resolves very early on.  So
   we don't want anything the least bit ugly in these lists. */
int good_concept_sublist_sizes[call_list_extent];
short int *good_concept_sublists[call_list_extent];
call_conc_option_state verify_options;
bool verify_used_number;
bool verify_used_direction;
bool verify_used_selector;


// A few accessors to let the UI stuff survive.
uint32_t get_concparseflags(const concept_descriptor *foo) { return foo->concparseflags; }
const char *get_call_name(const call_with_name *foo) { return foo->name; }
const char *get_call_menu_name(const call_with_name *foo) { return foo->menu_name; }
dance_level get_concept_level(const concept_descriptor *foo) { return foo->level; }
Cstring get_concept_name(const concept_descriptor *foo) { return foo->name; }
Cstring get_concept_menu_name(const concept_descriptor *foo) { return foo->menu_name; }
concept_kind get_concept_kind(const concept_descriptor *foo) { return foo->kind; }
const concept_descriptor *access_concept_descriptor_table(int i) { return &concept_descriptor_table[i]; }
bool get_yield_if_ambiguous_flag(call_with_name *foo) {return (foo->the_defn.callflagsf & CFLAG2_YIELD_IF_AMBIGUOUS) != 0; }
call_with_name *access_base_calls(int i) { return base_calls[i]; }

void set_parse_block_concept(parse_block *p, const concept_descriptor *concept) { p->concept = concept; }
void set_parse_block_next(parse_block *p, parse_block *next) { p->next = next; }
void set_parse_block_call(parse_block *p, call_with_name *call) { p->call = call; }
void set_parse_block_call_to_print(parse_block *p, call_with_name *call) { p->call_to_print = call; }
void set_parse_block_replacement_key(parse_block *p, short int key) { p->replacement_key = key; }
parse_block **get_parse_block_subsidiary_root_addr(parse_block *p) { return &p->subsidiary_root; }

const concept_kind constant_with_concept_diagnose = concept_diagnose;
const concept_kind constant_with_marker_end_of_list = marker_end_of_list;


parse_block *get_parse_block_mark() { return parse_block::parse_active_list; }
parse_block *get_parse_block()
{
   parse_block *item;

   if (parse_block::parse_inactive_list) {
      item = parse_block::parse_inactive_list;
      parse_block::parse_inactive_list = item->gc_ptr;
   }
   else {
      item = new parse_block;
   }

   item->gc_ptr = parse_block::parse_active_list;
   parse_block::parse_active_list = item;
   item->initialize((concept_descriptor *) 0);
   return item;
}



warning_info config_save_warnings()
{ return configuration::save_warnings(); }

void config_restore_warnings(const warning_info & rhs)
{ configuration::restore_warnings(rhs); }


void expand::compress_setup(const expand::thing & thing, setup *stuff) THROW_DECL
{
   setup temp = *stuff;

   stuff->kind = thing.inner_kind;
   stuff->clear_people();
   gather(stuff, &temp, thing.source_indices, attr::klimit(thing.inner_kind), thing.rot * 011);
   stuff->rotation -= thing.rot;
   canonicalize_rotation(stuff);
}


void expand::expand_setup(const expand::thing & thing, setup *stuff) THROW_DECL
{
   setup temp = *stuff;

   stuff->kind = thing.outer_kind;
   stuff->clear_people();
   scatter(stuff, &temp, thing.source_indices, attr::klimit(thing.inner_kind), thing.rot * 033);
   stuff->rotation += thing.rot;
   canonicalize_rotation(stuff);
}

// Turn a 3x4 into a 1/4 tag if the spots are occupied appropriately.  Otherwise do nothing.
void expand::fix_3x4_to_qtag(setup *stuff) THROW_DECL
{
   static const expand::thing foo = {{1, 2, 4, 5, 7, 8, 10, 11}, s_qtag, s3x4, 0};
   if ((little_endian_live_mask(stuff) & 01111) == 0)
      expand::compress_setup(foo, stuff);
}


extern void update_id_bits(setup *ss)
{
   int i;
   unsigned short int *face_list = (uint16_t *) 0;

   static uint16_t face_qtg[] = {
      3, d_west, 7, d_east,
      2, d_west, 3, d_east,
      7, d_west, 6, d_east,
      1, d_west, 0, d_east,
      4, d_west, 5, d_east,
      1, d_south, 3, d_north,
      3, d_south, 4, d_north,
      0, d_south, 7, d_north,
      7, d_south, 5, d_north,
      UINT16_C(~0)};

   static uint16_t face_spindle[] = {
      0, d_south, 6, d_north,
      1, d_south, 5, d_north,
      2, d_south, 4, d_north,
      0, d_east, 1, d_west,
      1, d_east, 2, d_west,
      6, d_east, 5, d_west,
      5, d_east, 7, d_west,
      UINT16_C(~0)};

   static uint16_t face_1x4[] = {
      2, d_west, 3, d_east,
      3, d_west, 1, d_east,
      1, d_west, 0, d_east,
      UINT16_C(~0)};

   static uint16_t face_2x4[] = {
      3, d_west, 2, d_east,
      2, d_west, 1, d_east,
      1, d_west, 0, d_east,
      4, d_west, 5, d_east,
      5, d_west, 6, d_east,
      6, d_west, 7, d_east,
      0, d_south, 7, d_north,
      1, d_south, 6, d_north,
      2, d_south, 5, d_north,
      3, d_south, 4, d_north,
      UINT16_C(~0)};

   static uint16_t face_2x2[] = {
      1, d_west, 0, d_east,
      2, d_west, 3, d_east,
      0, d_south, 3, d_north,
      1, d_south, 2, d_north,
      UINT16_C(~0)};

   static uint16_t face_1x3[] = {
      2, d_west, 1, d_east,
      1, d_west, 0, d_east,
      UINT16_C(~0)};

   static uint16_t face_2x3[] = {
      2, d_west, 1, d_east,
      1, d_west, 0, d_east,
      3, d_west, 4, d_east,
      4, d_west, 5, d_east,
      0, d_south, 5, d_north,
      1, d_south, 4, d_north,
      2, d_south, 3, d_north,
      UINT16_C(~0)};

   static uint16_t face_2x6[] = {
      5, d_west, 4, d_east,
      4, d_west, 3, d_east,
      3, d_west, 2, d_east,
      2, d_west, 1, d_east,
      1, d_west, 0, d_east,
      6, d_west, 7, d_east,
      7, d_west, 8, d_east,
      8, d_west, 9, d_east,
      9, d_west, 10, d_east,
      10, d_west, 11, d_east,
      0, d_south, 11, d_north,
      1, d_south, 10, d_north,
      2, d_south, 9, d_north,
      3, d_south, 8, d_north,
      4, d_south, 7, d_north,
      5, d_south, 6, d_north,
      UINT16_C(~0)};

   static uint16_t face_2x8[] = {
      7, d_west, 6, d_east,
      6, d_west, 5, d_east,
      5, d_west, 4, d_east,
      4, d_west, 3, d_east,
      3, d_west, 2, d_east,
      2, d_west, 1, d_east,
      1, d_west, 0, d_east,
      8, d_west, 9, d_east,
      9, d_west, 10, d_east,
      10, d_west, 11, d_east,
      11, d_west, 12, d_east,
      12, d_west, 13, d_east,
      13, d_west, 14, d_east,
      14, d_west, 15, d_east,
      0, d_south, 15, d_north,
      1, d_south, 14, d_north,
      2, d_south, 13, d_north,
      3, d_south, 12, d_north,
      4, d_south, 11, d_north,
      5, d_south, 10, d_north,
      6, d_south, 9, d_north,
      7, d_south, 8, d_north,
      UINT16_C(~0)};

   static uint16_t face_1x8[] = {
      1, d_west, 0, d_east,
      2, d_west, 3, d_east,
      4, d_west, 5, d_east,
      7, d_west, 6, d_east,
      3, d_west, 1, d_east,
      5, d_west, 7, d_east,
      6, d_west, 2, d_east,
      UINT16_C(~0)};

   static uint16_t face_3x4[] = {
      1,  d_west, 0, d_east,
      2,  d_west, 1, d_east,
      3,  d_west, 2, d_east,
      11, d_west, 10, d_east,
      5,  d_west, 11, d_east,
      4,  d_west, 5, d_east,
      8,  d_west, 9, d_east,
      7,  d_west, 8, d_east,
      6,  d_west, 7, d_east,
      0,  d_south, 10, d_north,
      1,  d_south, 11, d_north,
      2,  d_south, 5, d_north,
      3,  d_south, 4, d_north,
      10, d_south, 9, d_north,
      11, d_south, 8, d_north,
      5,  d_south, 7, d_north,
      4,  d_south, 6, d_north,
      UINT16_C(~0)};

   static uint16_t face_434[] = {
      1,  d_west, 0,  d_east,
      2,  d_west, 1,  d_east,
      3,  d_west, 2,  d_east,
      10, d_west, 9,  d_east,
      4,  d_west, 10, d_east,
      7,  d_west, 8,  d_east,
      6,  d_west, 7,  d_east,
      5,  d_west, 6,  d_east,
      UINT16_C(~0)};

   static uint16_t face_3x5[] = {
      1,  d_west, 0, d_east,
      2,  d_west, 1, d_east,
      3,  d_west, 2, d_east,
      4,  d_west, 3, d_east,
      13, d_west, 12, d_east,
      14, d_west, 13, d_east,
      6,  d_west, 14, d_east,
      5,  d_west, 6,  d_east,
      10, d_west, 11, d_east,
      9,  d_west, 10, d_east,
      8,  d_west, 9, d_east,
      7,  d_west, 8, d_east,
      0,  d_south, 12, d_north,
      1,  d_south, 13, d_north,
      2,  d_south, 14, d_north,
      3,  d_south, 6,  d_north,
      4,  d_south, 5,  d_north,
      12, d_south, 11, d_north,
      13, d_south, 10, d_north,
      14, d_south, 9,  d_north,
      6,  d_south, 8,  d_north,
      5,  d_south, 7,  d_north,
      UINT16_C(~0)};

   static uint16_t face_4x4[] = {
      13, d_west, 12,  d_east,
      14, d_west, 13,  d_east,
      0,  d_west, 14,  d_east,
      15, d_west, 10,  d_east,
      3,  d_west, 15,  d_east,
      1,  d_west,  3,  d_east,
      11, d_west,  9,  d_east,
      7,  d_west, 11,  d_east,
      2,  d_west,  7,  d_east,
      6,  d_west,  8,  d_east,
      5,  d_west,  6,  d_east,
      4,  d_west,  5,  d_east,
      12, d_south, 10, d_north,
      13, d_south, 15, d_north,
      14, d_south,  3, d_north,
      0,  d_south,  1, d_north,
      10, d_south,  9, d_north,
      15, d_south, 11, d_north,
      3,  d_south,  7, d_north,
      1,  d_south,  2, d_north,
      9,  d_south,  8, d_north,
      11, d_south,  6, d_north,
      7,  d_south,  5, d_north,
      2,  d_south,  4, d_north,
      UINT16_C(~0)};

   static uint16_t face_c1phan[] = {
      2,  d_west,   0, d_east,
      5,  d_west,   7, d_east,
      8,  d_west,  10, d_east,
      15, d_west,  13, d_east,
      1,  d_south,  3, d_north,
      4,  d_south,  6, d_north,
      11, d_south,  9, d_north,
      14, d_south, 12, d_north,
      UINT16_C(~0)};

   static uint16_t face_4x5[] = {
      1, d_west, 0, d_east,
      2, d_west, 1, d_east,
      3, d_west, 2, d_east,
      4, d_west, 3, d_east,
      8, d_west, 9, d_east,
      7, d_west, 8, d_east,
      6, d_west, 7, d_east,
      5, d_west, 6, d_east,
      16, d_west, 15, d_east,
      17, d_west, 16, d_east,
      18, d_west, 17, d_east,
      19, d_west, 18, d_east,
      13, d_west, 14, d_east,
      12, d_west, 13, d_east,
      11, d_west, 12, d_east,
      10, d_west, 11, d_east,
      0, d_south, 9, d_north,
      1, d_south, 8, d_north,
      2, d_south, 7, d_north,
      3, d_south, 6, d_north,
      4, d_south, 5, d_north,
      9, d_south, 15, d_north,
      8, d_south, 16, d_north,
      7, d_south, 17, d_north,
      6, d_south, 18, d_north,
      5, d_south, 19, d_north,
      15, d_south, 14, d_north,
      16, d_south, 13, d_north,
      17, d_south, 12, d_north,
      18, d_south, 11, d_north,
      19, d_south, 10, d_north,
      UINT16_C(~0)};

   clear_bits_for_update(ss);

   uint32_t livemask = little_endian_live_mask(ss);

   const id_bit_table *ptr = setup_attrs[ss->kind].id_bit_table_ptr;

   switch (ss->kind) {
   case s_qtag:
      face_list = face_qtg; break;
   case s_spindle:
      face_list = face_spindle; break;
   case s1x4:
      face_list = face_1x4; break;
   case s2x4:
      face_list = face_2x4; break;
   case s1x8:
      face_list = face_1x8; break;
   case s2x2:
      face_list = face_2x2; break;
   case s2x3:
      face_list = face_2x3; break;
   case s3x4:
      face_list = face_3x4; break;
   case s3x5:
      face_list = face_3x5; break;
   case s_434:
      face_list = face_434; break;
   case s4x4:
      face_list = face_4x4; break;
   case s_c1phan:
      // This doesn't get all legal cases, but ...
      if ((livemask & 0x5555) == 0 || (livemask & 0xAAAA) == 0 || (livemask & 0xA55A) == 0 || (livemask & 0x5AA5) == 0)
         face_list = face_c1phan;
      break;
   case s4x5:
      face_list = face_4x5; break;
   case s2x6:
      face_list = face_2x6; break;
   case s2x8:
      face_list = face_2x8; break;
   case s1x3:
      face_list = face_1x3; break;
   }

   if (face_list) {
      for ( ; *face_list != UINT16_C(~0) ; ) {
         short idx1 = *face_list++;

         if ((ss->people[idx1].id1 & d_mask) == *face_list++) {
            short idx2 = *face_list++;
            if ((ss->people[idx2].id1 & d_mask) == *face_list++) {
               ss->people[idx1].id2 |= ID2_FACING;
               ss->people[idx2].id2 |= ID2_FACING;
            }
         }
         else
            face_list += 2;
      }

      // Those that were not found to be facing get set to ID2_NOTFACING, except that if
      // the height of the setup was 1, that is, it's a 1x4 or such, any person facing
      // up or down is deemed not to be facing anyone.
      //
      // This means that, in a 1x4 line, people will have neither ID2_FACING nor
      // ID2_NOTFACING set, so the designation "those facing" will not just return
      // false; it will raise an error.  If we are in a 1/4 tag and the call is
      // "[those facing pass thru] and circle 1/4", it tries the 4-person call with just
      // the centers first, and, if that raises an error, it tries it with the whole
      // 8-person setup.  So this code makes the 4-person version illegal, so it will go
      // to the 8-person version.

      uint32_t cant_do_it_mask = (setup_attrs[ss->kind].bounding_box[1] == 1) ? 0x8 : 0;

      for (i=0 ; i<=attr::slimit(ss) ; i++) {
         if (ss->people[i].id1 && !(ss->people[i].id2 & ID2_FACING) &&
             !(ss->people[i].id1 & cant_do_it_mask)) {
            ss->people[i].id2 |= ID2_NOTFACING;
         }
      }
   }

   // Some setups are only recognized for ID bits with certain patterns of population.
   //  The bit tables make those assumptions, so we have to use the bit tables
   //  only if those assumptions are satisfied.

   switch (ss->kind) {
   case s2x5:
      // We recognize "centers" or "center 4" if they are a Z within the center 6.
      if (livemask == 0x3BDU || livemask == 0x2F7U)
         ptr = id_bit_table_2x5_z;
      // We recognize "center 6"/"outer 2" and "center 2"/"outer 6" if the center 2x3
      // is fully occupied.
      else if (livemask == 0x3DEU || livemask == 0x1EFU)
         ptr = id_bit_table_2x5_ctr6;
      break;
   case sd2x5:
      // We recognize "center 6"/"outer 2" and "center 2"/"outer 6" if the center 2x3
      // is fully occupied.
      if (livemask == 0x3DEU || livemask == 0x3BDU)
         ptr = id_bit_table_d2x5_ctr6;
      // We recognize "centers" or "center 4" if they are a Z within the center 6.
      else if (livemask != 0x37BU && livemask != 0x1EFU)
         ptr = (id_bit_table *) 0;
      break;
   case sd2x7:
      // We recognize "centers" or "center 4" if they are a Z with outer wings.
      if (livemask == 0x3C78U || livemask == 0x0D9BU)
         ptr = id_bit_table_d2x7a;
      else if (livemask == 0x366CU || livemask == 0x078FU)
         ptr = id_bit_table_d2x7b;
      // Otherwise, the center 6 must be fully occupied, and we
      // recognize "center 6"/"outer 2" and "center 2"/"outer 6".
      else if ((livemask & 0x0E1CU) != 0x0E1CU)
         ptr = (id_bit_table *) 0;
      break;
   case s2x6:
      /* **** This isn't really right -- it would allow "outer pairs bingo".
         We really should only allow 2-person calls, unless we say
         "outer triple boxes".  So we're not completely sure what the right thing is. */
      if (livemask == 07474U || livemask == 0x3CFU)
         /* Setup is parallelogram, accept slightly stronger table. */
         ptr = id_bit_table_2x6_pg;
      break;
   case s1x10:
      /* We recognize center 4 and center 6 if this has center 6 filled, then a gap,
         then isolated people. */
      if (livemask != 0x3BDU) ptr = (id_bit_table *) 0;
      break;
   case s3x6:
      // If the center 1x6 is fully occupied, use this better table.
      if ((livemask & 0700700U) == 0700700U) ptr = id_bit_table_3x6_with_1x6;
      // Or, if center 1x4 is occupied, use the standard table.
      // Otherwise, fail.
      if ((livemask & 0600600U) != 0600600U) ptr = (id_bit_table *) 0;
      break;
   case sbigdmd:
      // If this is populated appropriately, we can identify "outer pairs".
      if (livemask == 07474U || livemask == 0x3CFU) ptr = id_bit_table_bigdmd_wings;
      break;
   case sbigbone:
      // If this is populated appropriately, we can identify "outer pairs".
      if (livemask == 07474U || livemask == 0x3CFU) ptr = id_bit_table_bigbone_wings;
      break;
   case sbigdhrgl:
      // If this is populated appropriately, we can identify "outer pairs".
      if (livemask == 07474U || livemask == 0x3CFU) ptr = id_bit_table_bigdhrgl_wings;
      break;
   case sbighrgl:
      // If this is populated appropriately, we can identify "outer pairs".
      if (livemask == 07474U || livemask == 0x3CFU) ptr = id_bit_table_bighrgl_wings;
      break;
   case sbigh:
      // If it's a "double bent tidal line", we can recognize the "center 4"
      // and "outer pairs".
      if (livemask == 06363U || livemask == 07474U) ptr = id_bit_table_bigh_dblbent;
      // Otherwise, we recognize only the center 1x4, and only if it is full.
      else if ((livemask & 06060U) != 06060U) ptr = (id_bit_table *) 0;
      break;
   case s_525:
      if (livemask == 04747U) ptr = id_bit_table_525_nw;
      else if (livemask == 07474U) ptr = id_bit_table_525_ne;
      break;
   case s_343:
      if ((livemask & 0xE7) == 0xE7) ptr = id_bit_table_343_outr;
      break;
   case s_545:
      if ((livemask & 0xF9F) == 0x387 || (livemask & 0xF9F) == 0xE1C)
         ptr = id_bit_table_545_outr;
      else if ((livemask & 0x3060) == 0x3060)
         ptr = id_bit_table_545_innr;
      break;
   case s3x4:

      // There are three special things we can recognize from here.
      // If the setup is populated as an "H", we use a special table
      // (*NOT* the usual one picked up from the setup_attrs list)
      // that knows about the center 2 and the outer 6 and all that.
      // If the center 2x3 is occupied, we use a table for that.
      // If the setup is populated as offset lines/columns/whatever,
      // we use the table from the setup_attrs list, that knows about
      // the "outer pairs".
      // If the corners are occupied but the ends of the center line
      // are not, use a table that recognizes those corners as "ends".
      // If occupied as Z's, recognize "center 4" as the center Z.
      // If all else fails, we use the default table. */

      if (livemask == 07171U) ptr = id_bit_table_3x4_h;
      else if ((livemask & 04646U) == 04646U) ptr = id_bit_table_3x4_ctr6;
      else if (livemask == 07474U || livemask == 06363U) ptr = id_bit_table_3x4_offset;
      else if ((livemask & 03131U) == 01111U) ptr = id_bit_table_3x4_corners;
      else if (livemask == 07272U || livemask == 06565U) ptr = id_bit_table_3x4_zs;
      break;
   case sd3x4:
      if ((livemask & 01616U) != 01616U) ptr = (id_bit_table *) 0;
      break;
   case s4x4:
      // We recognize centers and ends if this is populated as a butterfly.
      // Otherwise, we recognize "center 4" only if those 4 spots are occupied.
      if (livemask == 0x9999U)
         ptr = id_bit_table_butterfly;
      else if (livemask == 0xC9C9U || livemask == 0x9C9CU ||
               livemask == 0xB8B8U || livemask == 0x8B8BU)
         ptr = id_bit_table_4x4_outer_pairs;
      else if ((livemask & 0x8888U) != 0x8888U)
         ptr = (id_bit_table *) 0;
      break;
   case s4x6:
      // The only selector we recognize is "center 4", when the center
      // 2x2 box is full.  Unfortunately, the table we get when this is so
      // is something of a crock.  It will recognize stupid things like
      // "center line", "center wave", "center column", and "center 1x4".
      // We expect users not to specify such things in a 4x6.
      if ((livemask & 014001400U) != 014001400U)
         ptr = (id_bit_table *) 0;
      break;
   case sdeepxwv:
      // We recognize outer pairs if they are fully populated.
      if ((livemask & 00303U) != 00303U) ptr = (id_bit_table *) 0;
      break;
   case swqtag:
      // We recognize "center 6" if the center 2 spots are empty,
      // so that there is a "short 6".
      // Otherwise, we recognize the center 1x6 if it is fully populated.
      if (livemask == 0x1EF)
         ptr = id_bit_table_wqtag_hollow;
      else if ((livemask & 0x39CU) != 0x39CU)
         ptr = (id_bit_table *) 0;
      break;
   case s3dmd:
      // The standard table requires all points, and centers of center diamond only, occupied.
      // But first we look for a few other configurations.

      // Look for center 1x6 occupied.
      if ((livemask & 0xE38U) == 0xE38U) ptr = id_bit_table_3dmd_ctr1x6;
      // Look for center 1x6 having center 1x4 occupied.
      else if ((livemask & 0xC30U) == 0xC30U) ptr = id_bit_table_3dmd_ctr1x4;
      // Look for center 1x4 and outer points.
      else if (livemask == 06565U) ptr = id_bit_table_3dmd_in_out;
      // Otherwise, see whether to accept default or reject everything.
      else if (livemask != 04747U) ptr = (id_bit_table *) 0;
      break;
   case s4dmd:
      // If center diamonds have only centers and outer ones only points, use this.
      if (livemask == 0xC9C9U) ptr = id_bit_table_4dmd_cc_ee;
      // Or if center 1x4 is occupied, use standard map.
      else if ((livemask & 0xC0C0U) != 0xC0C0U) ptr = (id_bit_table *) 0;
      break;
   case s3ptpd:
      // If the center diamond is full and the inboard points of each outer diamond
      // is present, we can do a "triple trade".
      if (livemask == 06666U || livemask == 06363U || livemask == 07272U)
         ptr = id_bit_table_3ptpd;
      break;
   case s_hsqtag:
      // Only allow it if outer pairs are together.
      if (livemask != 0xF3CU && livemask != 0xCF3U) ptr = (id_bit_table *) 0;
      break;
   }

   if (!ptr) return;

   for (i=0; i<=attr::slimit(ss); i++) {
      if (ss->people[i].id1 & BIT_PERSON)
         ss->people[i].id2 |= ptr[i][ss->people[i].id1 & 3];
   }
}


extern void clear_bits_for_update(setup *ss)
{
   for (int i=0; i<MAX_PEOPLE; i++) {
      ss->people[i].id2 &= ~ID2_BITS_TO_CLEAR_FOR_UPDATE;
   }
}

extern void clear_absolute_proximity_bits(setup *ss)
{
   for (int i=0; i<MAX_PEOPLE; i++) {
      ss->people[i].id3 &= ~ID3_ABSOLUTE_PROXIMITY_BITS;
   }
}

extern void clear_absolute_proximity_and_facing_bits(setup *ss)
{
   for (int i=0; i<MAX_PEOPLE; i++) {
      ss->people[i].id3 &= ~(ID3_ABSOLUTE_FACING_BITS|ID3_ABSOLUTE_PROXIMITY_BITS);
   }
}


full_expand::thing *full_expand::touch_hash_table1[full_expand::NUM_TOUCH_HASH_BUCKETS];
full_expand::thing *full_expand::touch_hash_table2[full_expand::NUM_TOUCH_HASH_BUCKETS];
full_expand::thing *full_expand::touch_hash_table3[full_expand::NUM_TOUCH_HASH_BUCKETS];

expand::thing *expand::expand_hash_table[expand::NUM_EXPAND_HASH_BUCKETS];
expand::thing *expand::compress_hash_table[expand::NUM_EXPAND_HASH_BUCKETS];

void expand::initialize()
{
   thing *tabp;
   int i;

   for (i=0 ; i<NUM_EXPAND_HASH_BUCKETS ; i++) {
      expand_hash_table[i] = (thing *) 0;
      compress_hash_table[i] = (thing *) 0;
   }

   for (tabp = expand_init_table ; tabp->inner_kind != nothing ; tabp++) {
      uint32_t hash_num = (tabp->inner_kind * 25) & (NUM_EXPAND_HASH_BUCKETS-1);
      tabp->next_expand = expand_hash_table[hash_num];
      expand_hash_table[hash_num] = tabp;

      hash_num = (tabp->outer_kind * 25) & (NUM_EXPAND_HASH_BUCKETS-1);
      tabp->next_compress = compress_hash_table[hash_num];
      compress_hash_table[hash_num] = tabp;
   }
}

void full_expand::initialize_touch_tables()
{
   thing *tabp;
   int i;

   for (i=0 ; i<NUM_TOUCH_HASH_BUCKETS ; i++) {
      touch_hash_table1[i] = (thing *) 0;
      touch_hash_table2[i] = (thing *) 0;
      touch_hash_table3[i] = (thing *) 0;
   }

   for (tabp = touch_init_table1 ; tabp->kind != nothing ; tabp++) {
      uint32_t hash_num = ((tabp->kind + (5*tabp->live)) * 25) & (NUM_TOUCH_HASH_BUCKETS-1);
      tabp->next = touch_hash_table1[hash_num];
      touch_hash_table1[hash_num] = tabp;
   }

   for (tabp = touch_init_table2 ; tabp->kind != nothing ; tabp++) {
      uint32_t hash_num = ((tabp->kind + (5*tabp->live)) * 25) & (NUM_TOUCH_HASH_BUCKETS-1);
      tabp->next = touch_hash_table2[hash_num];
      touch_hash_table2[hash_num] = tabp;
   }

   for (tabp = touch_init_table3 ; tabp->kind != nothing ; tabp++) {
      uint32_t hash_num = ((tabp->kind + (5*tabp->live)) * 25) & (NUM_TOUCH_HASH_BUCKETS-1);
      tabp->next = touch_hash_table3[hash_num];
      touch_hash_table3[hash_num] = tabp;
   }
}


full_expand::thing *full_expand::search_table_1(setup_kind kind,
                                                uint32_t livemask,
                                                uint32_t directions)
{
   uint32_t hash_num = ((kind + (5*livemask)) * 25) & (NUM_TOUCH_HASH_BUCKETS-1);

   for (thing *tptr = touch_hash_table1[hash_num] ; tptr ; tptr = tptr->next) {
      if (tptr->kind == kind &&
          tptr->live == livemask &&
          ((tptr->dir ^ directions) & tptr->dirmask) == 0) return tptr;
   }

   return (thing *) 0;
}


full_expand::thing *full_expand::search_table_2(setup_kind kind,
                                                uint32_t livemask,
                                                uint32_t directions)
{
   uint32_t hash_num = ((kind + (5*livemask)) * 25) & (NUM_TOUCH_HASH_BUCKETS-1);

   for (thing *tptr = touch_hash_table2[hash_num] ; tptr ; tptr = tptr->next) {
      if (tptr->kind == kind &&
          tptr->live == livemask &&
          ((tptr->dir ^ directions) & tptr->dirmask) == 0) return tptr;
   }

   return (thing *) 0;
}


full_expand::thing *full_expand::search_table_3(setup_kind kind,
                                                uint32_t livemask,
                                                uint32_t directions,
                                                uint32_t touchflags)
{
   uint32_t hash_num = ((kind + (5*livemask)) * 25) & (NUM_TOUCH_HASH_BUCKETS-1);

   for (thing *tptr = touch_hash_table3[hash_num] ; tptr ; tptr = tptr->next) {
      // If it has the evil "32" bit on, don't allow it except for special
      // fan-the-top calls, indicated by CFLAG1_STEP_TO_WAVE.
      // Well, not that way any longer.
      if (tptr->kind == kind &&
          tptr->live == livemask &&
          ((tptr->dir ^ directions) & tptr->dirmask) == 0 &&
          (!(tptr->forbidden_elongation & 32) || touchflags == CFLAG1_STEP_TO_WAVE))
         return tptr;
   }

   return (thing *) 0;
}


// This gets a ===> BIG-ENDIAN <=== mask of people's facing directions.
// Each person occupies 2 bits in the resultant masks.  The "livemask"
// bits are both on if the person is live.
extern void big_endian_get_directions64(
   const setup *ss,
   uint64_t & directions,
   uint64_t & livemask)
{
   directions = 0ULL;
   livemask = 0ULL;

   for (int i=0; i<=attr::slimit(ss); i++) {
      uint32_t p = ss->people[i].id1;
      directions = (directions<<2) | (p&3);
      livemask <<= 2;
      if (p) { livemask |= 3 ; }
   }
}

extern void big_endian_get_directions32(
   const setup *ss,
   uint32_t & directions,
   uint32_t & livemask)
{
   uint64_t local_directions;
   uint64_t local_livemask;

   big_endian_get_directions64(ss, local_directions, local_livemask);

   directions = (uint32_t) local_directions;
   livemask = (uint32_t) local_livemask;
}


extern void touch_or_rear_back(
   setup *scopy,
   bool did_mirror,
   int callflags1) THROW_DECL
{
   uint32_t directions, livemask;
   const full_expand::thing *tptr;
   const expand::thing *zptr;

   // We don't understand absurd setups.
   if (attr::slimit(scopy) < 0) return;

   // We don't do this if doing the last half of a call.
   if (scopy->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_LASTHALF)) return;

   if (!(callflags1 & (CFLAG1_STEP_REAR_MASK | CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK)))
      return;

   big_endian_get_directions32(scopy, directions, livemask);

   uint32_t touchflags = (callflags1 & CFLAG1_STEP_REAR_MASK);
   call_restriction new_assume = cr_none;

   switch (touchflags) {
   case CFLAG1_REAR_BACK_FROM_QTAG:
   case CFLAG1_REAR_BACK_FROM_R_WAVE:
   case CFLAG1_REAR_BACK_FROM_EITHER:
      if (scopy->cmd.cmd_final_flags.test_finalbit(FINAL__SPLIT_DIXIE_APPROVED) ||
          (touchflags != CFLAG1_REAR_BACK_FROM_QTAG &&
           !scopy->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_MXNMASK))) {
         // Check for rearing back from a wave.
         tptr = full_expand::search_table_1(scopy->kind, livemask, directions);
         if (tptr) goto found_tptr;

         // A few setups are special -- we allow any combination at all in livemask.

         if (livemask != 0) {
            switch (scopy->kind) {
            case s1x2:
               if (((directions ^ 0x2U) & livemask) == 0) {
                  tptr = &rear_1x2_pair;
                  goto found_tptr;
               }
               break;
            case s1x4:
               if (((directions ^ 0x28U) & livemask) == 0) {
                  tptr = &rear_2x2_pair;
                  goto found_tptr;
               }
               break;
            case s_bone:
               if (((directions ^ 0xA802U) & livemask) == 0) {
                  tptr = &rear_bone_pair;
                  goto found_tptr;
               }
               break;
            }
         }
      }

      if (touchflags != CFLAG1_REAR_BACK_FROM_R_WAVE) {
         // Check for rearing back from a 1/4 tag.
         tptr = full_expand::search_table_2(scopy->kind, livemask, directions);
         if (tptr) goto found_tptr;
      }

      break;

   case CFLAG1_STEP_TO_WAVE:
   case CFLAG1_STEP_TO_NONPHAN_BOX:
   case CFLAG1_STEP_TO_QTAG:

      // Special stuff:  If lines facing, but people are incomplete,
      // we honor an "assume facing lines" command.
      // Or if columns, we honor "assume dpt".

      if (scopy->cmd.cmd_assume.assumption == cr_li_lo &&
          scopy->cmd.cmd_assume.assump_col == 0 &&
          scopy->cmd.cmd_assume.assump_both == 1) {
         if (scopy->kind == s2x4 && directions == (livemask & 0xAA00)) {
            new_assume = cr_wave_only;  // Turn into "assume right-handed waves" --
            livemask = 0xFFFF;          // assump_col and assump_both are still OK.
            directions = 0xAA00;
         }
         else if (scopy->kind == s2x3 && directions == (livemask & 0xA80)) {
            livemask = 0xFFF;
            directions = 0xA80;
         }
      }
      else if (scopy->cmd.cmd_assume.assumption == cr_2fl_only &&
               scopy->cmd.cmd_assume.assump_col == 1 &&
               scopy->cmd.cmd_assume.assump_both == 1) {
         if (scopy->kind == s2x4 && directions == (livemask & 0x5FF5)) {
            livemask = 0xFFFF;
            directions = 0x5FF5;
         }
      }

      tptr = full_expand::search_table_3(scopy->kind, livemask, directions, touchflags);
      if (tptr) {
         if (!(tptr->forbidden_elongation & 0x80))
            goto found_tptr;

         // If we just have beaus in what might be facing lines, but the incoming assumption
         // says the others aren't facing us, don't do anything.
         if (scopy->cmd.cmd_assume.assumption == cr_wave_only ||
             (scopy->kind == s2x4 && livemask == 0x3333)) {
            return;
         }

         goto found_tptr;
      }

      // A few setups are special -- we allow any combination at all in livemask,
      // though we are careful.

      bool step_ok =
         touchflags == CFLAG1_STEP_TO_WAVE || touchflags == CFLAG1_STEP_TO_QTAG;

      switch (scopy->kind) {
      case s2x4:
         if (livemask != 0) {
            if ((step_ok || livemask == 0xFFFFU) &&
                ((directions ^ 0x77DDU) & livemask) == 0) {
               // Check for stepping to parallel waves from an 8-chain.
               tptr = &step_8ch_pair;
               goto found_tptr;
            }
            else if (((0x003C & ~livemask) == 0 || (0x3C00 & ~livemask) == 0) &&
                     (livemask & 0xC3C3) != 0 &&
                     ((directions ^ 0x5D75U) & 0x7D7DU & livemask) == 0) {
               // Check for stepping to some kind of 1/4 tag
               // from a DPT or trade-by or whatever.
               // But we require at least one person on the outside.  See test t57.
               tptr = &step_qtag_pair;
               goto found_tptr;
            }
            else if (touchflags == CFLAG1_STEP_TO_QTAG &&
                     ((directions ^ 0x5D75U) & 0x7D7DU & livemask) == 0 &&
                     (livemask & 0x3C3C) != 0) {
               // If the command is specifically to step to a qtag, we are
               // more accepting relative to missing people.
               // There must be at least 1 person in the middle.  Everyone in
               // the middle must be as if in a DPT or trade-by.  Everyone
               // else must be in generalized columns.
               tptr = &step_qtag_pair;
               goto found_tptr;
            }
         }
         break;
      case s2x2:
         if (livemask != 0 &&
             (step_ok || livemask == 0xFF)) {
            if (((directions ^ 0x7DU) & livemask) == 0) {
               tptr = &step_2x2h_pair;
               goto found_tptr;
            }
            else if (((directions ^ 0xA0U) & livemask) == 0) {
               tptr = &step_2x2v_pair;
               goto found_tptr;
            }
         }
         break;
      case s_spindle:
         if (livemask != 0 &&
             ((step_ok || livemask == 0xFFFFU) &&
              ((directions ^ 0xA802U) & livemask) == 0)) {
            tptr = &step_spindle_pair;
            goto found_tptr;
         }
         break;
      case s_trngl:
         if ((0x0FU & ~livemask) == 0 &&
             ((directions ^ 0x17U) & livemask) == 0 &&
             !(callflags1 & CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK)) {
            tptr = &step_tgl_pair;
            goto found_tptr;
         }
         break;
      case sdmd:
         if ((0x33U & ~livemask) == 0 &&
             ((directions ^ 0xA0U) & livemask) == 0 &&
             !(callflags1 & CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK)) {
            tptr = &step_dmd_pair;
            goto found_tptr;
         }
         break;
      case s_ptpd:
         if ((0x3333U & ~livemask) == 0 &&
             ((directions ^ 0xA00AU) & livemask) == 0 &&
             !(callflags1 & CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK)) {
            tptr = &step_ptpd_pair;
            goto found_tptr;
         }
         break;
      case s_qtag:
         if ((0x0F0FU & ~livemask) == 0 &&
             ((directions ^ 0xFD57U) & livemask) == 0 &&
             !(callflags1 & CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK)) {
            tptr = &step_qtgctr_pair;
            goto found_tptr;
         }
         break;
      }

      break;
   }

 do_the_leftie_test:

   // We need to raise an error if the caller said "left spin the top" when we were in a right-hand wave.

   if (callflags1 & CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK) {
      big_endian_get_directions32(scopy, directions, livemask);    // Need to do this again.

      uint32_t rtest = ~0U;
      uint32_t rothertest = ~0U;

      switch (scopy->kind) {
      case s2x2:
         rtest = 0x28U;
         rothertest = 0x5FU;
         break;
      case s2x4:
         rtest = 0x2288U;
         rothertest = 0x55FFU;
         break;
      case s_bone:
         rtest = 0x58F2U;
         break;
      case s_rigger:
         rtest = 0x58F2U;
         break;
      case s1x2:
         rtest = 0x2U;
         break;
      case s1x4:
         rtest = 0x28U;
         break;
      case s1x8:
         rtest = 0x2882U;
         break;
      case s_qtag:
         rtest = 0x0802U;
         livemask &= 0x0F0F;
         break;
      }

      if (rtest != ~0U) {
         bool rfail = (((directions) ^ rtest) & livemask) != 0;
         if (rothertest != ~0U)
            rfail = rfail && ((directions ^ rothertest) & livemask) != 0;

         if (did_mirror && rfail) fail("Setup is not left-handed.");
      }
   }

   return;

   found_tptr:

   // Make sure we alert the user (Hi, Clark!) if we call "Fan the Top" where only
   // the centers would touch.  Case is a starting DPT with ends 1/4 left.
   // People are supposed to step to right hands, but the centers on a Fan the Top
   // normally step to left hands.  What are the dancers supposed to do?
   if ((tptr->forbidden_elongation & 64) && (callflags1 & CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK) &&
       touchflags == CFLAG1_STEP_TO_WAVE)
      warn(warn__some_touch_evil);
   else
      warn(tptr->warning);  // Or give whatever warning the table says, but not both.

   // The 128 bit says that we have a setup with phantoms, but we insist that no
   // phantom concept was given.  If we have everyone 1/2 press back from a
   // right-hand wave, we have a 2x4 from which we allow "swing thru", as long
   // as no phantom concept was given.  (If we said "split phantom waves", we
   // would definitely not want to step to a single wave from this 2x4.)
   if ((tptr->forbidden_elongation & 128) && (scopy->cmd.cmd_misc_flags & CMD_MISC__PHANTOMS))
      return;

   if ((tptr->forbidden_elongation & 4) && (scopy->cmd.cmd_misc3_flags & CMD_MISC3__DOING_ENDS))
      scopy->cmd.prior_elongation_bits =
         (scopy->cmd.prior_elongation_bits & (~3)) | ((scopy->rotation+1) & 3);

   if ((scopy->cmd.prior_elongation_bits & tptr->forbidden_elongation & 3) &&
       (!(scopy->cmd.cmd_misc_flags & CMD_MISC__NO_CHK_ELONG)))
      fail("People are too far away to work with each other on this call.");

   zptr = tptr->expand_lists;
   scopy->cmd.cmd_misc_flags |= CMD_MISC__DISTORTED;
   setup stemp = *scopy;
   scopy->clear_people();

   if (tptr->forbidden_elongation & 8) {
      gather(scopy, &stemp, zptr->source_indices, attr::klimit(zptr->inner_kind), zptr->rot * 011);
      scopy->rotation -= zptr->rot;
      scopy->kind = zptr->inner_kind;
   }
   else {
      scatter(scopy, &stemp, zptr->source_indices, attr::klimit(zptr->inner_kind), zptr->rot * 033);
      scopy->rotation += zptr->rot;
      scopy->kind = zptr->outer_kind;
   }

   // Check for doing this under the "mystic" concept.
   if ((scopy->cmd.cmd_misc2_flags & CMD_MISC2__CENTRAL_MYSTIC) && touchflags == CFLAG1_STEP_TO_WAVE) {
      if (scopy->kind == s1x8 && livemask == 0xFFFF && directions == 0xAA00) {
         // We stepped to a tidal wave.
         if (scopy->cmd.cmd_misc2_flags & CMD_MISC2__INVERT_MYSTIC) {
            scopy->swap_people(0, 1);
            scopy->swap_people(4, 5);
         }
         else {
            scopy->swap_people(2, 3);
            scopy->swap_people(6, 7);
         }
      }
      else if (scopy->kind == s2x4 && livemask == 0xFFFF && directions == 0x7DD7) {
         // We stepped to columns.
         if (scopy->cmd.cmd_misc2_flags & CMD_MISC2__INVERT_MYSTIC) {
            scopy->swap_people(0, 7);
            scopy->swap_people(3, 4);
         }
         else {
            scopy->swap_people(2, 5);
            scopy->swap_people(1, 6);
         }
      }
      else if (scopy->kind == s_bone && livemask == 0xFFFF && directions == 0xAD07) {
         // We stepped to a bone.
         if (scopy->cmd.cmd_misc2_flags & CMD_MISC2__INVERT_MYSTIC) {
            scopy->swap_people(0, 5);
            scopy->swap_people(1, 4);
         }
         else {
            scopy->swap_people(2, 3);
            scopy->swap_people(6, 7);
         }
      }
      else if (scopy->kind == s_rigger && livemask == 0xFFFF && directions == 0xAD07) {
         // We stepped to a rigger from a bone.
         if (scopy->cmd.cmd_misc2_flags & CMD_MISC2__INVERT_MYSTIC) {
            scopy->swap_people(2, 3);
            scopy->swap_people(6, 7);
         }
         else {
            scopy->swap_people(0, 5);
            scopy->swap_people(1, 4);
         }
      }
      else if (scopy->kind == s_rigger && livemask == 0xFFFF && directions == 0x7DD7) {
         // We stepped to a rigger from a suitable qtag.
         if (scopy->cmd.cmd_misc2_flags & CMD_MISC2__INVERT_MYSTIC) {
            scopy->swap_people(2, 3);
            scopy->swap_people(6, 7);
         }
         else {
            scopy->swap_people(0, 1);
            scopy->swap_people(4, 5);
         }
      }

      update_id_bits(scopy);    // Centers and ends are meaningful now.
   }

   // Assumptions are no longer valid, except for a few special cases.
   scopy->cmd.cmd_assume.assumption = new_assume;
   canonicalize_rotation(scopy);
   scopy->clear_all_overcasts();

   goto do_the_leftie_test;
}



bool expand::compress_from_hash_table(setup *ss,
                                      normalize_action action,
                                      uint32_t livemask,
                                      bool noqtagcompress) THROW_DECL
{
   uint32_t hash_num = (ss->kind * 25) & (expand::NUM_EXPAND_HASH_BUCKETS-1);

   const thing *cptr;

   for (cptr=compress_hash_table[hash_num] ; cptr ; cptr=cptr->next_compress) {
      if (cptr->outer_kind == ss->kind &&
          action >= cptr->action_level &&
          (cptr->biglivemask & livemask) == 0) {

         // Do not compress qtag to 2x3 or 2x4 if switch is on.
         if (noqtagcompress && cptr->outer_kind == s_qtag && (cptr->inner_kind == s2x3))
            continue;

         if (action != cptr->action_level && cptr->must_be_exact_level) {
            continue;
         }

         warn(cptr->norwarning);
         expand::compress_setup(*cptr, ss);
         return true;
      }
   }

   return false;
}



bool expand::expand_from_hash_table(setup *ss,
                                    uint64_t needpropbits,
                                    uint32_t livemask) THROW_DECL
{
   uint32_t hash_num = (ss->kind * 25) & (NUM_EXPAND_HASH_BUCKETS-1);
   const thing *eptr;

   for (eptr=expand_hash_table[hash_num] ; eptr ; eptr=eptr->next_expand) {
      if (eptr->inner_kind == ss->kind &&
          ((eptr->expandconcpropmask & needpropbits)) &&
          (livemask & eptr->lillivemask) == 0) {
         warn(eptr->expwarning);
         expand_setup(*eptr, ss);
         return true;
      }
   }

   return false;
}





extern void do_matrix_expansion(
   setup *ss,
   uint32_t concprops,
   bool recompute_id) THROW_DECL
{
   uint32_t needprops = concprops & CONCPROP__NEED_MASK;
   if (needprops == 0) return;
   uint64_t needpropbits = NB(needprops);

   for (;;) {
      uint32_t livemask = little_endian_live_mask(ss);

      // Search for simple things in the hash table.

      if (expand::expand_from_hash_table(ss, needpropbits, livemask))
         goto expanded;

      if (ss->kind == s4x4) {
         if (needpropbits & (NB(CONCPROP__NEEDK_4D_4PTPD) | NB(CONCPROP__NEEDK_4DMD))) {
            if (livemask == 0x1717U) {
               expand::expand_setup(s_4x4_4dma, ss);
               goto expanded;
            }
            else if (livemask == 0x7171U) {
               expand::expand_setup(s_4x4_4dmb, ss);
               goto expanded;
            }
         }


         else if (needpropbits & (NB(CONCPROP__NEEDK_TWINDMD) | NB(CONCPROP__NEEDK_TWINQTAG))) {
            // Egads!  It turns out that the "CONCPROP__NEEDK_TWINQTAG"
            // indicator is used not just for "twin phantom 1/4 tags", but for
            // "tandem in a 1/4 tag"!!!!  Both require a 4x6, but with very
            // different philosophies of how to figure it out.  The code under
            // WOULDLIKETODOTHISBUTCANT handles the "twin phantom 1/4 tags"
            // concept the way we would like, but fails to do the other.  The
            // later code, which we have to use, is a sort of sleazy compromise
            // that can handle both actions.
#ifdef WOULDLIKETODOTHISBUTCANT
            // If the occupation is such that one or the other orientations is
            // impossible, we use the other.  If that turns out to be impossible
            // also, or the facing directions of the "points" are inapropriate
            // for the twin qtag / twin diamond nature of the concept, an error
            // will be raised when the concept is executed.
            if ((livemask & 0x6060U) != 0) {
               expand::expand_setup(s_4x4_4x6b, ss);
               goto expanded;
            }
            else if ((livemask & 0x0606U) != 0) {
               expand::expand_setup(s_4x4_4x6a, ss);
               goto expanded;
            }

            // If we get no clue from the occupation, try to deduce the
            // answer from the facing directions of the "points".

            uint32_t ctrs = ss->people[3].id1 | ss->people[7].id1 |
               ss->people[11].id1 | ss->people[15].id1;

            if (ctrs != 0 && (ctrs & 011) != 011) {
               if (needprops == CONCPROP__NEEDK_TWINQTAG) ctrs ^= 1;
               expand::expand_setup((ctrs & 1) ? s_4x4_4x6b : s_4x4_4x6a, ss);
               goto expanded;
            }
#else
            uint32_t ctrs = ss->people[3].id1 | ss->people[7].id1 |
               ss->people[11].id1 | ss->people[15].id1;

            if (ctrs != 0 && (ctrs & 011) != 011) {
               if (needprops == CONCPROP__NEEDK_TWINQTAG) ctrs ^= 1;
               expand::expand_setup((ctrs & 1) ? s_4x4_4x6b : s_4x4_4x6a, ss);
               goto expanded;
            }
            else if (livemask == 0x1717U) {
               expand::expand_setup(s_4x4_4x6a, ss);
               goto expanded;
            }
            else if (livemask == 0x7171U) {
               expand::expand_setup(s_4x4_4x6b, ss);
               goto expanded;
            }
#endif
         }
      }

      if (ss->kind == s_23232) {
         if (needpropbits & NB(CONCPROP__NEEDK_4X5)) {
            // Have to figure out where to move the people in the lines of 3.
            if ((livemask & 04343) == 04242) {
               warn(warn__check_hokey_4x5);
               expand::expand_setup(s_23232_4x5a, ss);
               goto expanded;
            }
            else if ((livemask & 04343) == 04141) {
               warn(warn__check_hokey_4x5);
               expand::expand_setup(s_23232_4x5b, ss);
               goto expanded;
            }
         }
      }

      // If get here, we did NOT see any concept that requires a setup expansion.

      return;

      expanded:

      // Can't ask for things like "near box" if we expanded the matrix.
      clear_absolute_proximity_bits(ss);

      // Put in position-identification bits (leads/trailers/beaus/belles/centers/ends etc.)
      if (recompute_id) update_id_bits(ss);
   }
}


class restriction_tester {

private:

   enum chk_type {
      chk_none,
      chk_wave,
      chk_groups,
      chk_line_col_aspect,
      chk_groups_cpls_in_tbone,
      chk_groups_cpls_in_tbone_either_way,
      chk_anti_groups,
      chk_box,
      chk_box_dbl,
      chk_indep_box,
      chk_star,
      chk_dmd_qtag,
      chk_dmd_qtag_new,
      chk_qtag,
      chk_qbox,
      chk_peelable,
      chk_spec_directions,
      chk_sex,
      chk_inroller
   };

   // We make this a struct inside the class, rather than having its
   // fields just comprise the class itself (note that there are no
   // fields in this class, and it is never instantiated) so that
   // we can make the initialization arrays be statically initialized.

   struct restr_initializer {
      setup_kind k;
      call_restriction restr;
      int size;
      int8_t map1[17];
      int8_t map2[17];
      int8_t map3[8];
      int8_t map4[8];
      bool ok_for_assume;
      chk_type check;
      restr_initializer *next;
   };

   // Must be a power of 2.
   enum { NUM_RESTR_HASH_BUCKETS = 32 };

   static restr_initializer restr_init_table0[];
   static restr_initializer restr_init_table1[];
   static restr_initializer restr_init_table4[];
   static restr_initializer restr_init_table9[];

   static restr_initializer *restr_hash_table0[NUM_RESTR_HASH_BUCKETS];
   static restr_initializer *restr_hash_table1[NUM_RESTR_HASH_BUCKETS];
   static restr_initializer *restr_hash_table4[NUM_RESTR_HASH_BUCKETS];
   static restr_initializer *restr_hash_table9[NUM_RESTR_HASH_BUCKETS];

public:

   static restr_initializer *get_restriction_thing(setup_kind k, assumption_thing t);
   static void initialize_tables();

   friend restriction_test_result verify_restriction(
      setup *ss,
      assumption_thing tt,
      bool instantiate_phantoms,
      bool *failed_to_instantiate) THROW_DECL;

};

restriction_tester::restr_initializer
   *restriction_tester::restr_hash_table0[restriction_tester::NUM_RESTR_HASH_BUCKETS];
restriction_tester::restr_initializer
   *restriction_tester::restr_hash_table1[restriction_tester::NUM_RESTR_HASH_BUCKETS];
restriction_tester::restr_initializer
   *restriction_tester::restr_hash_table4[restriction_tester::NUM_RESTR_HASH_BUCKETS];
restriction_tester::restr_initializer
   *restriction_tester::restr_hash_table9[restriction_tester::NUM_RESTR_HASH_BUCKETS];


restriction_tester::restr_initializer restriction_tester::restr_init_table0[] = {
   {s2x4, cr_inroller_is_cw,   99,    {3, 7, 0, 4, 0, 4, 3, 7},
    {012, 010, 012, 010, 010, 012, 0, 0},
    {0}, {0}, false,  chk_inroller},
   {s2x4, cr_inroller_is_ccw,   99,    {4, 0, 7, 3, 7, 3, 4, 0},
    {010, 012, 010, 012, 012, 010, 0, 0},
    {0}, {0}, false,  chk_inroller},
   {s2x4, cr_outroller_is_cw,   99,    {3, 7, 0, 4, 0, 4, 3, 7},
    {010, 012, 010, 012, 012, 010, 0, 0},
    {0}, {0}, false,  chk_inroller},
   {s2x4, cr_outroller_is_ccw,   99,    {4, 0, 7, 3, 7, 3, 4, 0},
    {012, 010, 012, 010, 010, 012, 0, 0},
    {0}, {0}, false,  chk_inroller},

   {s_qtag, cr_inroller_is_cw,   99,    {4, 0, 1, 5, 1, 5, 4, 0},
    {003, 001, 003, 001, 001, 003, 0, 0},
    {0}, {0}, false,  chk_inroller},
   {s_qtag, cr_inroller_is_ccw,   99,    {5, 1, 0, 4, 0, 4, 5, 1},
    {001, 003, 001, 003, 003, 001, 0, 0},
    {0}, {0}, false,  chk_inroller},
   {s_qtag, cr_outroller_is_cw,   99,    {4, 0, 1, 5, 1, 5, 4, 0},
    {001, 003, 001, 003, 003, 001, 0, 0},
    {0}, {0}, false,  chk_inroller},
   {s_qtag, cr_outroller_is_ccw,   99,    {5, 1, 0, 4, 0, 4, 5, 1},
    {003, 001, 003, 001, 001, 003, 0, 0},
    {0}, {0}, false,  chk_inroller},

   {s2x4, cr_judge_is_cw,   0,    {7, 3, 0, 4, 0, 4, 3, 7},
    {012, 010, 012, 010, 010, 012, 0, 0},
    {0}, {0}, false,  chk_inroller},
   {s2x4, cr_judge_is_ccw,   0,    {0, 4, 7, 3, 7, 3, 0, 4},
    {012, 010, 012, 010, 010, 012, 0, 0},
    {0}, {0}, false,  chk_inroller},
   {s2x4, cr_socker_is_cw,   0,    {3, 7, 4, 0, 4, 0, 3, 7},
    {012, 010, 012, 010, 010, 012, 0, 0},
    {0}, {0}, false,  chk_inroller},
   {s2x4, cr_socker_is_ccw,   0,    {4, 0, 3, 7, 3, 7, 4, 0},
    {012, 010, 012, 010, 010, 012, 0, 0},
    {0}, {0}, false,  chk_inroller},

   {s_qtag, cr_judge_is_cw,   0,    {0, 4, 1, 5, 1, 5, 0, 4},
    {003, 001, 003, 001, 001, 003, 0, 0},
    {0}, {0}, false,  chk_inroller},
   {s_qtag, cr_judge_is_ccw,   0,    {1, 5, 0, 4, 0, 4, 1, 5},
    {003, 001, 003, 001, 001, 003, 0, 0},
    {0}, {0}, false,  chk_inroller},
   {s_qtag, cr_socker_is_cw,   0,    {4, 0, 5, 1, 5, 1, 0, 4},
    {003, 001, 003, 001, 001, 003, 0, 0},
    {0}, {0}, false,  chk_inroller},
   {s_qtag, cr_socker_is_ccw,   0,    {5, 1, 4, 0, 4, 0, 1, 5},
    {003, 001, 003, 001, 001, 003, 0, 0},
    {0}, {0}, false,  chk_inroller},

   {s1x2, cr_opposite_sex, 2, {0}, {0}, {0}, {1}, false, chk_sex},
   {s4x4,      cr_wave_only,    1,
    {0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 2, 0, 2, 0, 2, 0},
    {2, 0, 2, 0, 0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 2, 0}, {0}, {0}, true,  chk_box},
   {s4x4,      cr_2fl_only,     1,
    {0, 0, 0, 0, 0, 0, 2, 0, 2, 2, 2, 2, 2, 2, 0, 2},
    {2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 2, 0, 2, 2, 2, 2}, {0}, {0}, true,  chk_box},
   {s_qtag,    cr_qtag_mwv,     8, {0, 1, 3, 2, 5, 4, 6, 7, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s_qtag,    cr_qtag_mag_mwv, 8, {0, 1, 2, 3, 5, 4, 7, 6, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},

   {s_spindle, cr_dmd_ctrs_mwv, 6, {0, 6, 1, 5, 2, 4, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_spindle, cr_spd_base_mwv, 4, {0, 6, 2, 4, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s1x3dmd, cr_spd_base_mwv, 4, {1, 2, 6, 5, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {sdmd,      cr_dmd_ctrs_mwv, 2, {1, 3, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {sdmd,      cr_dmd_pts_mwv, 2, {0, 2, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s_star,    cr_dmd_ctrs_mwv, 2, {1, 1}, {1, 3},
    {1, 0}, {1, 2},              false, chk_star},
   {s_2stars,  cr_dmd_ctrs_mwv, 2, {2, 0, 1}, {2, 4, 5},
    {2, 3, 6}, {2, 2, 7},         false, chk_star},
   {s_short6,  cr_dmd_ctrs_mwv, 4, {0, 2, 5, 3, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s_trngl,   cr_dmd_ctrs_mwv, 2, {1, 2, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s_trngl4,  cr_dmd_ctrs_mwv, 6, {2, 3, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s_bone6,   cr_dmd_ctrs_mwv, 4, {0, 4, 1, 3, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_bone,    cr_dmd_ctrs_mwv, 4, {0, 5, 1, 4, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_rigger,  cr_dmd_ctrs_mwv, 4, {0, 5, 1, 4, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},

   {s_short6,  cr_dmd_ctrs_1f, 4, {0, 2, 5, 3, -1},
    {0, 2, 1}, {0}, {0}, false, chk_spec_directions},
   {s_trngl,   cr_dmd_ctrs_1f, 2, {1, 2, -1},
    {0, 2, 1}, {0}, {0}, false, chk_spec_directions},
   {s_trngl4,  cr_dmd_ctrs_1f, 6, {2, 3, -1},
    {0, 2, 1}, {0}, {0}, false, chk_spec_directions},
   {s_bone6,   cr_dmd_ctrs_1f, 4, {0, 4, 1, 3, -1},
    {1, 3, 1}, {0}, {0}, false, chk_spec_directions},
   {s_bone,    cr_dmd_ctrs_1f, 4, {0, 5, 1, 4, -1},
    {1, 3, 1}, {0}, {0}, false, chk_spec_directions},
   {s_rigger,  cr_dmd_ctrs_1f, 4, {0, 5, 1, 4, -1},
    {1, 3, 1}, {0}, {0}, false, chk_spec_directions},

   {s1x4,      cr_dmd_ctrs_mwv, 2, {1, 3, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s1x8,      cr_dmd_ctrs_mwv, 4, {1, 3, 7, 5, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s2x4,      cr_dmd_ctrs_mwv, 4, {1, 2, 6, 5, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s2x3,      cr_dmd_ctrs_mwv, 2, {1, 4, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_qtag,    cr_dmd_ctrs_mwv, 4, {3, 2, 6, 7, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s_ptpd,    cr_dmd_ctrs_mwv, 4, {1, 3, 7, 5, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s2x2dmd,    cr_dmd_ctrs_mwv, 8, {1, 3, 7, 5, 11, 9, 13, 15, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_qtag,    cr_dmd_pts_mwv, 4, {0, 5, 1, 4, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_ptpd,    cr_dmd_pts_mwv, 4, {0, 2, 6, 4, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s2x2dmd,    cr_dmd_pts_mwv, 8, {0, 2, 6, 4, 10, 18, 12, 14, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s3dmd,     cr_dmd_ctrs_mwv, 6, {4, 3, 11, 5, 9, 10, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s4dmd,     cr_dmd_ctrs_mwv, 8, {5, 4, 7, 6, 14, 15, 12, 13, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s4x6,      cr_dmd_ctrs_mwv, 8, {1, 10, 4, 7, 22, 13, 19, 16, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {swqtag,    cr_dmd_ctrs_mwv, 6, {3, 2, 9, 4, 7, 8, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {sdeep2x1dmd, cr_dmd_ctrs_mwv, 8, {0, 1, 3, 4, 6, 5, 9, 8, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {sdmd,      cr_dmd_ctrs_1f, 2, {1, 3, -1},
    {1, 3, 1}, {0}, {0}, false, chk_spec_directions},
   {sdmd,      cr_dmd_pts_1f, 2, {0, 2, -1},
    {0, 2, 1}, {0}, {0}, false, chk_spec_directions},
   {s_qtag,    cr_dmd_pts_1f, 2, {0, 1, 5, 4, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_ptpd,    cr_dmd_pts_1f, 2, {0, 6, 2, 4, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s1x4,      cr_dmd_ctrs_1f, 2, {1, 3, -1},
    {0, 2, 1}, {0}, {0}, false, chk_spec_directions},
   {s1x8,      cr_dmd_ctrs_1f, 4, {1, 3, 7, 5, -1},
    {0, 2, 1}, {0}, {0}, false, chk_spec_directions},
   {s_short6,  cr_extend_inroutl, 6, {0, 2, 5, 3, 4, 1, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s_short6,  cr_extend_inloutr, 6, {2, 0, 3, 5, 4, 1, -1},
    {0, 2, 0}, {0}, {0}, false, chk_spec_directions},
   {s_1x2dmd,  cr_extend_inroutl, 6, {2, 5, 0, 4, 1, 3, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_1x2dmd,  cr_extend_inloutr, 6, {5, 2, 0, 4, 1, 3, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s1x3dmd,   cr_extend_inroutl, 8, {3, 7, 0, 6, 1, 5, 2, 4, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s1x3dmd,   cr_extend_inloutr, 8, {7, 3, 0, 6, 1, 5, 2, 4, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_spindle, cr_extend_inroutl, 8, {0, 6, 1, 5, 2, 4, 7, 3, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_spindle, cr_extend_inloutr, 8, {6, 0, 5, 1, 4, 2, 7, 3, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_rigger,  cr_extend_inroutl, 8, {0, 5, 1, 4, 6, 3, 7, 2, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_rigger,  cr_extend_inloutr, 8, {5, 0, 4, 1, 6, 3, 7, 2, -1},
    {1, 3, 0}, {0}, {0}, false, chk_spec_directions},
   {s_hrglass, cr_magic_only, 4, {6, 2, 7, 3},
    {0}, {0}, {0}, false, chk_wave},
   {s1x8, cr_wave_only, 8, {0, 1, 3, 2, 6, 7, 5, 4},
    {0}, {0}, {0}, true, chk_wave},
   {s1x8, cr_tidal_wave, 8, {0, 1, 3, 2, 6, 7, 5, 4},
    {0}, {0}, {0}, true, chk_wave},
   {s1x8, cr_tidal_2fl, 8, {0, 3, 1, 2, 6, 5, 7, 4},
    {0}, {0}, {0}, true, chk_wave},
   {s1x8, cr_tidal_line, 8, {0, 1, 3, 2, 6, 7, 5, 4},
    {0}, {0}, {0}, true, chk_line_col_aspect},
   {s1x8, cr_1fl_only, 4, {0, 4, 1, 5, 2, 6, 3, 7},
    {2}, {0}, {0}, true, chk_groups},
   {s1x8, cr_all_facing_same, 8, {0, 1, 2, 3, 4, 5, 6, 7},
    {1}, {0}, {0}, true,  chk_groups},
   {s1x8, cr_2fl_only, 8, {0, 3, 1, 2, 6, 5, 7, 4},
    {0}, {0}, {0}, false, chk_wave},
   {s1x8, cr_2fl_per_1x4, 2, {0, 1, 4, 5, 2, 3, 6, 7},
    {2}, {0}, {0}, false, chk_anti_groups},
   {s1x8, cr_2fl_per_1x4, 8, {0, 3, 1, 2, 6, 5, 7, 4},
    {0}, {0}, {0}, false, chk_wave},
   {s2x4, cr_4x4couples_only, 4, {0, 4, 1, 5, 2, 6, 3, 7},
    {2}, {0}, {0}, true, chk_groups},
   {s1x8, cr_4x4couples_only, 4, {0, 4, 1, 5, 2, 6, 3, 7},
    {2}, {0}, {0}, true, chk_groups},
   {s1x8, cr_4x4_2fl_only, 8, {0, 4, 1, 5, 2, 6, 3, 7},
    {0}, {0}, {0}, false, chk_wave},
   {s1x8, cr_couples_only, 2, {0, 2, 4, 6, 1, 3, 5, 7},
    {4}, {0}, {0}, true, chk_groups},
   {s1x8, cr_miniwaves, 1, {0, 3, 5, 6, 1, 2, 4, 7},
    {4}, {0}, {0}, true, chk_anti_groups},
   {s1x3, cr_1fl_only, 3, {0, 1, 2},
    {1}, {0}, {0}, true, chk_groups},
   {s1x3, cr_3x3couples_only, 3, {0, 1, 2},
    {1}, {0}, {0}, true, chk_groups},
   {s1x3, cr_wave_only, 3, {0, 1, 2},
    {0}, {0}, {0}, true, chk_wave},
   {s1x4, cr_wave_only, 4, {0, 1, 3, 2},
    {0}, {0}, {0}, true, chk_wave},
   {s1x4, cr_2fl_only, 4, {0, 2, 1, 3},
    {0}, {0}, {0}, true, chk_wave},
   {s1x4, cr_1fl_only, 4, {0, 1, 2, 3},
    {1}, {0}, {0}, true, chk_groups},
   {s1x4, cr_4x4couples_only, 4, {0, 1, 2, 3},
    {1}, {0}, {0}, true, chk_groups},
   {s1x4, cr_all_facing_same, 4, {0, 1, 2, 3},
    {1}, {0}, {0}, true, chk_groups},
   {s1x4, cr_couples_only, 2, {0, 2, 1, 3},
    {2}, {0}, {0}, true, chk_groups},
   {s1x4, cr_miniwaves, 1, {0, 3, 1, 2},
    {2}, {0}, {0}, true, chk_anti_groups},
   {s1x4, cr_magic_only, 4, {0, 1, 2, 3},
    {0}, {0}, {0}, true, chk_wave},
   {s1x4, cr_all_ns, 4, {4, 0, 1, 2, 3},
    {0}, {0}, {0}, false, chk_dmd_qtag},
   {s1x4, cr_all_ew, 4, {0},
    {4, 0, 1, 2, 3}, {0}, {0}, false, chk_dmd_qtag},
   {s2x3, cr_all_ns, 4, {6, 0, 1, 2, 3, 4, 5},
    {0}, {0}, {0}, false, chk_dmd_qtag},
   {s2x3, cr_all_ew, 4, {0},
    {6, 0, 1, 2, 3, 4, 5}, {0}, {0}, false, chk_dmd_qtag},
   {s2x4, cr_all_ns, 4, {8, 0, 1, 2, 3, 4, 5, 6, 7},
    {0}, {0}, {0}, false, chk_dmd_qtag},
   {s2x4, cr_all_ew, 4, {0},
    {8, 0, 1, 2, 3, 4, 5, 6, 7}, {0}, {0}, false, chk_dmd_qtag},
   {s1x6, cr_all_ns, 4, {6, 0, 1, 2, 3, 4, 5},
    {0}, {0}, {0}, false, chk_dmd_qtag},
   {s1x6, cr_all_ew, 4, {0},
    {6, 0, 1, 2, 3, 4, 5}, {0}, {0}, false, chk_dmd_qtag},
   {s1x8, cr_all_ns, 4, {8, 0, 1, 2, 3, 4, 5, 6, 7},
    {0}, {0}, {0}, false, chk_dmd_qtag},
   {s1x8, cr_all_ew, 4, {0},
    {8, 0, 1, 2, 3, 4, 5, 6, 7}, {0}, {0}, false, chk_dmd_qtag},
   {s1x6, cr_miniwaves, 1, {0, 2, 4, 1, 5, 3},
    {3}, {0}, {0}, true, chk_anti_groups},
   {s1x6, cr_wave_only, 6, {0, 1, 2, 5, 4, 3},
    {0}, {0}, {0}, true, chk_wave},
   {s1x6, cr_1fl_only, 3, {0, 3, 1, 4, 2, 5},
    {2}, {0}, {0}, true, chk_groups},
   {s1x6, cr_all_facing_same, 6, {0, 1, 2, 3, 4, 5},
    {1}, {0}, {0}, true, chk_groups},
   {s1x6, cr_3x3_2fl_only, 6, {0, 3, 1, 4, 2, 5},
    {0}, {0}, {0}, false, chk_wave},
   {s1x6, cr_3x3couples_only, 3, {0, 3, 1, 4, 2, 5},
    {2}, {0}, {0}, true, chk_groups},
   {s2x3, cr_3x3couples_only, 3, {0, 3, 1, 4, 2, 5},
    {2}, {0}, {0}, true, chk_groups},
   {s1x10, cr_wave_only, 10, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
    {0}, {0}, {0}, true, chk_wave},
   {s1x12, cr_wave_only, 12, {0, 1, 2, 3, 4, 5, 7, 6, 9, 8, 11, 10},
    {0}, {0}, {0}, true, chk_wave},
   {s1x14, cr_wave_only, 14, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},
    {0}, {0}, {0}, true, chk_wave},
   {s1x16, cr_wave_only, 16, {0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 11, 10, 13, 12, 15, 14},
    {0}, {0}, {0}, true, chk_wave},
   {s1x16, cr_couples_only, 2, {0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15},
    {8}, {0}, {0}, true, chk_groups},
   {s1x16, cr_miniwaves, 1, {0, 2, 4, 6, 9, 11, 13, 15, 1, 3, 5, 7, 8, 10, 12, 14},
    {8}, {0}, {0}, true, chk_anti_groups},
   {s2x3, cr_all_facing_same, 6, {0, 1, 2, 3, 4, 5},
    {1}, {0}, {0}, true, chk_groups},
   {s2x3, cr_1fl_only, 3, {0, 3, 1, 4, 2, 5},
    {2}, {0}, {0}, true, chk_groups},
   {s2x3, cr_li_lo, 6, {3, 0, 4, 1, 5, 2},
    {0}, {0}, {0}, true, chk_wave},
   {s2x4, cr_2fl_only, 8, {0, 3, 1, 2, 6, 5, 7, 4},
    {0}, {0}, {0}, true, chk_wave},
   {s2x4, cr_2fl_per_1x4, 2, {0, 1, 6, 7, 2, 3, 4, 5},
    {2}, {0}, {0}, false, chk_anti_groups},
   {s3x4, cr_ctr_2fl_only, 4, {10, 4, 11, 5},
    {0}, {0}, {0}, true, chk_wave},
   {s_crosswave, cr_ctr_2fl_only, 4, {2, 6, 3, 7},
    {8}, {0}, {0}, true, chk_wave},
   {s2x4, cr_wave_only, 8, {0, 1, 2, 3, 5, 4, 7, 6},
    {0}, {0}, {0}, true, chk_wave},
   {s2x3, cr_wave_only, 6, {0, 1, 2, 3, 4, 5},
    {0}, {0}, {0}, false, chk_wave},
   {s2x3, cr_magic_only, 6, {0, 1, 2, 3, 4, 5},
    {0}, {0}, {0}, true, chk_wave},
   {s2x4, cr_magic_only, 8, {0, 1, 3, 2, 5, 4, 6, 7},
    {0}, {0}, {0}, true, chk_wave},
   {sbigdmd, cr_magic_only, 12, {0, 1, 4, 5, 7, 6, 11, 10, 3, 2, 8, 9},
    {0}, {0}, {0}, false, chk_wave},
   {s1x8, cr_magic_only, 8, {0, 1, 2, 3, 5, 4, 7, 6},
    {0}, {0}, {0}, true, chk_wave},
   {s2x4, cr_li_lo, 8, {4, 0, 5, 1, 6, 2, 7, 3},
    {0}, {0}, {0}, true, chk_wave},
   {s2x4, cr_ctrs_in_out, 4, {5, 1, 6, 2},
    {0}, {0}, {0}, false, chk_wave},
   {s2x4, cr_indep_in_out, 0, {3, 2, 3, 2, 1, 0, 1, 0},
    {0}, {0}, {0}, false, chk_indep_box},
   {s2x4, cr_all_facing_same, 8, {0, 1, 2, 3, 4, 5, 6, 7},
    {1}, {0}, {0}, true, chk_groups},
   {s2x4, cr_1fl_only, 4, {0, 4, 1, 5, 2, 6, 3, 7},
    {2}, {0}, {0}, true, chk_groups},
   {s2x4, cr_couples_only, 2, {0, 2, 4, 6, 1, 3, 5, 7},
    {4}, {0}, {0}, true, chk_groups},
   {s3x4, cr_couples_only, 2, {0, 2, 4, 6, 8, 10, 1, 3, 5, 7, 9, 11},
    {6}, {0}, {0}, true, chk_groups},
   {s2x4, cr_miniwaves, 1, {0, 2, 5, 7, 1, 3, 4, 6},
    {4}, {0}, {0}, true, chk_anti_groups},
   {s_qtag, cr_wave_only, 4, {6, 7, 3, 2},
    {0}, {0}, {0}, true, chk_wave},
   {s_qtag, cr_2fl_only, 4, {6, 2, 7, 3},
    {0}, {0}, {0}, true, chk_wave},
   {s_qtag, cr_dmd_not_intlk, 4, {6, 7, 3, 2},
    {0}, {0}, {0}, true, chk_wave},
   {s_qtag, cr_dmd_intlk, 4, {6, 2, 7, 3},
    {0}, {0}, {0}, true, chk_wave},
   {s_ptpd, cr_dmd_not_intlk, 4, {0, 2, 6, 4},
    {0}, {0}, {0}, true, chk_wave},
   {s_ptpd, cr_dmd_intlk, 4, {0, 6, 2, 4},
    {0}, {0}, {0}, true, chk_wave},
   {s_qtag, cr_miniwaves, 1, {6, 3, 7, 2},
    {2, 1}, {0}, {0}, true, chk_anti_groups},
   {s_qtag, cr_couples_only, 2, {6, 2, 7, 3},
    {2}, {0}, {0}, true, chk_groups},
   {sbigdmd, cr_2fl_only, 4, {3, 9, 2, 8},
    {8}, {0}, {0}, true, chk_wave},
   {s4x5, cr_2fl_only, 4, {7, 17, 2, 12},
    {8}, {0}, {0}, true, chk_wave},
   {s_galaxy, cr_wave_only, 4, {1, 3, 7, 5},
    {0}, {0}, {0}, true, chk_wave},
   {s_bone, cr_miniwaves, 1, {6, 3, 7, 2},
    {2}, {0}, {0}, true, chk_anti_groups},
   {s_bone, cr_couples_only, 2, {6, 2, 7, 3},
    {2}, {0}, {0}, true, chk_groups},
   {s_bone6, cr_wave_only, 6, {0, 1, 2, 3, 4, 5},
    {0}, {0}, {0}, true, chk_wave},
   {s_bone, cr_wave_only, 8, {0, 1, 2, 3, 5, 4, 7, 6},
    {0}, {0}, {0}, true, chk_wave},
   {s2x8, cr_wave_only, 16, {0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 11, 10, 13, 12, 15, 14},
    {0}, {0}, {0}, true, chk_wave},
   {s2x8, cr_4x4_2fl_only, 16, {0, 4, 1, 5, 2, 6, 3, 7, 12, 8, 13, 9, 14, 10, 15, 11},
    {0},{0},{0}, false,chk_wave},
   {s2x8, cr_1fl_only, 8, {0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15},
    {2}, {0}, {0}, true, chk_groups},
   {s1x16, cr_4x4couples_only, 4, {0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15},
    {4}, {0}, {0}, true, chk_groups},
   {s2x8, cr_4x4couples_only, 4, {0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15},
    {4}, {0}, {0}, true, chk_groups},
   {s4x4, cr_4x4couples_only, 4, {12, 13, 14, 0, 10, 15, 3, 1, 9, 11, 7, 2, 8, 6, 5, 4},
    {4}, {0}, {0}, false, chk_groups_cpls_in_tbone_either_way},
   {s2x8, cr_couples_only, 2, {0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15},
    {8}, {0}, {0}, true, chk_groups},
   {s2x8, cr_miniwaves, 1, {0, 2, 4, 6, 9, 11, 13, 15, 1, 3, 5, 7, 8, 10, 12, 14},
    {8}, {0}, {0}, false, chk_anti_groups},
   {s2x6, cr_wave_only, 12, {0, 1, 2, 3, 4, 5, 7, 6, 9, 8, 11, 10},
    {0}, {0}, {0}, true, chk_wave},
   {s2x6, cr_3x3_2fl_only, 12, {0, 3, 1, 4, 2, 5, 9, 6, 10, 7, 11, 8},
    {0}, {0}, {0}, false, chk_wave},
   {s2x6, cr_1fl_only, 6, {0, 6, 1, 7, 2, 8, 3, 9, 4, 10, 5, 11},
    {2}, {0}, {0}, true, chk_groups},
   {s2x6, cr_3x3couples_only, 3, {0, 3, 6, 9, 1, 4, 7, 10, 2, 5, 8, 11},
    {4}, {0}, {0}, true, chk_groups},
   {s1x12, cr_3x3couples_only, 3, {0, 3, 6, 9, 1, 4, 7, 10, 2, 5, 8, 11},
    {4}, {0}, {0}, true, chk_groups},
   {s1x2, cr_wave_only, 2, {0, 1},
    {0}, {0}, {0}, true, chk_wave},
   {s1x2, cr_2fl_only, 2, {0, 1},
    {1}, {0}, {0}, true, chk_groups},
   {s1x2, cr_couples_only, 2, {0, 1},
    {1}, {0}, {0}, true, chk_groups},
   {s1x2, cr_all_facing_same, 2, {0, 1},
    {1}, {0}, {0}, true, chk_groups},
   {s1x2, cr_miniwaves, 1, {0, 1},
    {1}, {0}, {0}, true, chk_anti_groups},
   {s2x2, cr_couples_only, 0, {1, 0, 3, 2},
    {3, 2, 1, 0}, {0}, {0}, false, chk_box_dbl},
   {s2x2, cr_miniwaves, 2, {1, 0, 3, 2, 3, 0, 0, 3},
    {3, 2, 1, 0, 3, 3, 0, 0}, {0}, {0}, false, chk_box_dbl},
   {s2x2, cr_peelable_box, 0, {3, 2, 1, 0},
    {1, 0, 3, 2}, {0}, {0}, false, chk_box_dbl},
   {s2x4, cr_peelable_box, 2, {0, 1, 2, 3, 7, 6, 5, 4},
    {4}, {0}, {0}, false, chk_groups},
   {s2x4, cr_reg_tbone, 1, { 3, 2, 0, 1, 1, 0, 2, 3},
    {0, 3, 1, 2, 2, 1, 3, 0}, {0}, {0}, false, chk_box},
   {s2x2, cr_reg_tbone, 1, {2, 1, 0, 3},
    {2, 3, 0, 1}, {0}, {0}, false, chk_box},
   {s1x4, cr_reg_tbone, 1, {3, 2, 1, 0},
    {0, 3, 2, 1}, {0}, {0}, false, chk_box},
   {s3x4, cr_wave_only, 12, {0, 1, 2, 3, 5, 4, 7, 6, 9, 8, 10, 11},
    {0}, {0}, {0}, true, chk_wave},
   {s3x4, cr_2fl_only, 12, {0, 2, 1, 3, 8, 4, 9, 5, 10, 6, 11, 7},
    {0}, {0}, {0}, true, chk_wave},
   {s_thar, cr_wave_only, 8, {0, 1, 2, 3, 5, 4, 7, 6},
    /* NOTE THE 4 --> */{4}, {0}, {0}, true, chk_wave},
   {s_thar, cr_2fl_only, 8, {0, 5, 2, 7, 1, 4, 3, 6},
    /* NOTE THE 4 --> */{4}, {0}, {0}, true, chk_wave},
   {s_thar, cr_1fl_only, 8, {0, 3, 1, 2, 6, 5, 7, 4},
    {0}, {0}, {0}, false, chk_wave},
   {s_thar, cr_miniwaves, 1, {0, 2, 5, 7, 1, 3, 4, 6},
    {4}, {0}, {0}, false,  chk_anti_groups},
   {s_thar, cr_magic_only, 8, {0, 1, 3, 2, 5, 4, 6, 7},
    /* NOTE THE 4 --> */{4}, {0}, {0}, true, chk_wave},
   {s_crosswave, cr_wave_only, 8, {0, 1, 2, 3, 5, 4, 7, 6},
    /* NOTE THE 4 --> */{4}, {0}, {0}, true, chk_wave},
   {s_crosswave, cr_2fl_only, 8, {0, 5, 2, 7, 1, 4, 3, 6},
    /* NOTE THE 4 --> */{4}, {0}, {0}, true, chk_wave},
   {s_crosswave, cr_1fl_only, 8, {0, 3, 1, 2, 6, 5, 7, 4},
    {0}, {0}, {0}, false, chk_wave},
   {s_crosswave, cr_magic_only, 8, {0, 1, 3, 2, 5, 4, 6, 7},
    /* NOTE THE 4 --> */{4}, {0}, {0}, true, chk_wave},
   {s3x1dmd, cr_wave_only, 6, {0, 1, 2, 6, 5, 4},
    {0}, {0}, {0}, true, chk_wave},
   {s_2x1dmd, cr_wave_only, 4, {0, 1, 4, 3},
    {0}, {0}, {0}, true, chk_wave},
   {s_qtag, cr_real_1_4_tag, 4, {6, 7, 3, 2},
    {4, 4, 0, 5, 1}, {01}, {0}, true, chk_qtag},
   {s_qtag, cr_real_3_4_tag, 4, {6, 7, 3, 2},
    {4, 0, 4, 1, 5}, {01}, {0}, true, chk_qtag},
   {s_qtag, cr_real_1_4_line, 4, {6, 3, 7, 2},
    {4, 4, 0, 5, 1}, {01}, {0}, true, chk_qtag},
   {s_qtag, cr_real_3_4_line, 4, {6, 3, 7, 2},
    {4, 0, 4, 1, 5}, {01}, {0}, true, chk_qtag},
   {s_alamo, cr_couples_only, 2, {0, 2, 5, 7, 1, 3, 4, 6},
    {4}, {0}, {0}, false, chk_groups_cpls_in_tbone},
   {s_alamo, cr_magic_only, 8, {0, 1, 3, 2, 5, 4, 6, 7},  // Line-like, miniwaves of alternating handedness.
    /* NOTE THE 4 --> */{4}, {0}, {0}, true, chk_wave},
   {s_alamo, cr_li_lo, 8, {0, 1, 3, 2, 5, 4, 6, 7},       // Column-like, each pair facing or back-to-back.
    {0}, {0}, {0}, false, chk_wave},
   {s4x4, cr_li_lo, 8, {13, 14, 2, 1, 6, 5, 9, 10},  // Same, on "O" spots.
    {0}, {0}, {0}, false, chk_wave},
   {s_alamo, cr_1fl_only, 8, {0, 4, 1, 5, 2, 6, 3, 7},
    {0}, {0}, {0}, false, chk_wave},
   {s4x4, cr_1fl_only, 8, {13, 5, 14, 6, 1, 9, 2, 10},  // Same, on "O" spots.
    {0}, {0}, {0}, false, chk_wave},
   {s_alamo, cr_miniwaves, 8, {0, 1, 2, 3, 5, 4, 7, 6},
    {0}, {0}, {0}, false, chk_wave},
   {s4x4, cr_miniwaves, 8, {13, 14, 1, 2, 6, 5, 10, 9},  // Same, on "O" spots.
    {0}, {0}, {0}, false, chk_wave},
   {s_trngl, cr_wave_only, 2, {1, 2},
    {0}, {0}, {0}, true, chk_wave},
   {s_trngl, cr_miniwaves, 2, {1, 2},
    {0}, {0}, {0}, true, chk_wave},
   {s_trngl, cr_couples_only, 2, {1, 2},
    {1}, {0}, {0}, true, chk_groups},
   {s_trngl, cr_2fl_only, 3, {0, 1, 2},
    {1}, {0}, {0}, true, chk_groups},
   {s_trngl, cr_tgl_tandbase, 0, {0},
    {2, 1, 2}, {2, 1, 2}, {0}, true, chk_dmd_qtag},
   {s_short6, cr_nice_wv_triangles, 0, {4, 0, 2, 3, 5},
    {2, 1, 4}, {3, 0, 1, 5}, {3, 2, 3, 4}, false,  chk_dmd_qtag_new},
   {s_bone6, cr_nice_wv_triangles, 0, {2, 2, 5},
    {4, 0, 1, 3, 4}, {3, 0, 1, 2}, {3, 3, 4, 5}, false,  chk_dmd_qtag_new},
   {s_trngl, cr_nice_wv_triangles, 0, {2, 1, 2},
    {1, 0}, {1, 1}, {2, 0, 2}, false,  chk_dmd_qtag_new},
   {s_short6, cr_nice_tnd_triangles, 0, {0},
    {6, 0, 1, 2, 3, 4, 5}, {3, 1, 3, 5}, {3, 0, 2, 4}, false,  chk_dmd_qtag_new},
   {s_bone6, cr_nice_tnd_triangles, 0, {6, 0, 1, 2, 3, 4, 5},
    {0}, {3, 0, 2, 4}, {3, 1, 3, 5}, false,  chk_dmd_qtag_new},
   {s_trngl, cr_nice_tnd_triangles, 0, {0},
    {3, 0, 1, 2}, {2, 1, 2}, {1, 0}, false,  chk_dmd_qtag_new},
   {nothing}};

restriction_tester::restr_initializer restriction_tester::restr_init_table1[] = {
   {s1x2, cr_opposite_sex, 2, {0},
    {0}, {0}, {0}, false, chk_sex},
   {s2x4, cr_quarterbox, 4, {1, 5, 2, 6},
    {4, 0, 3, 7, 4}, {010}, {0}, true, chk_qtag},
   {s2x4, cr_threequarterbox, 4, {1, 5, 2, 6},
    {4, 3, 0, 4, 7}, {010}, {0}, true, chk_qtag},
   {s2x4, cr_quarterbox_or_col, 0, {3, 0, 1, 2},
    {3, 4, 5, 6}, {3, 1, 2, 3}, {3, 5, 6, 7}, false, chk_qbox},
   {s2x4, cr_quarterbox_or_magic_col, 0, {3, 1, 2, 7},
    {3, 3, 5, 6}, {3, 1, 2, 4}, {3, 0, 5, 6}, false, chk_qbox},
   {s2x3, cr_quarterbox_or_col, 0, {2, 0, 1},
    {2, 3, 4}, {2, 1, 2}, {2, 4, 5}, false, chk_qbox},
   {s2x3, cr_quarterbox_or_magic_col, 0, {2, 1, 5},
    {2, 2, 4}, {2, 1, 3}, {2, 0, 4}, false, chk_qbox},
   {s2x3, cr_wave_only, 6, {0, 3, 1, 4, 2, 5},
    {0}, {0}, {0}, true, chk_wave},
   {s2x3, cr_magic_only, 6, {0, 1, 2, 3, 4, 5},
    {0}, {0}, {0}, true, chk_wave},
   {s2x3, cr_peelable_box, 3, {0, 1, 2},
    {3, 4, 5}, {0}, {0}, false, chk_peelable},
   {s2x3, cr_all_facing_same, 6, {0, 1, 2, 3, 4, 5},
    {1}, {0}, {0}, true, chk_groups},
   {s1x4, cr_all_facing_same, 4, {0, 1, 2, 3},
    {1}, {0}, {0}, true, chk_groups},
   {s1x4, cr_2fl_only, 4, {0, 2, 1, 3},
    {0}, {0}, {0}, true, chk_wave},
   {s1x4, cr_li_lo, 4, {0, 1, 3, 2},
    {0}, {0}, {0}, true, chk_wave},
   {s1x6, cr_all_facing_same, 6, {0, 1, 2, 3, 4, 5},
    {1}, {0}, {0}, true, chk_groups},
   {s1x8, cr_all_facing_same, 8, {0, 1, 2, 3, 4, 5, 6, 7},
    {1}, {0}, {0}, true, chk_groups},
   {s1x8, cr_tidal_line, 8, {0, 1, 3, 2, 6, 7, 5, 4},
    {0}, {0}, {0}, true, chk_line_col_aspect},
   {s2x4, cr_li_lo, 8, {0, 1, 2, 3, 5, 4, 7, 6},
    {0}, {0}, {0}, true, chk_wave},
   {s2x4, cr_ctrs_in_out, 4, {1, 2, 6, 5},
    {0}, {0}, {0}, false, chk_wave},
   {s2x4, cr_indep_in_out, 0, {3, 2, 3, 2, 1, 0, 1, 0},
    {0}, {0}, {0}, false, chk_indep_box},
   {s2x4, cr_2fl_only, 8, {0, 3, 1, 2, 6, 5, 7, 4},
    {0}, {0}, {0}, true, chk_wave},
   {s2x4, cr_wave_only, 8, {0, 4, 1, 5, 2, 6, 3, 7},
    {0}, {0}, {0}, true, chk_wave},
   {s_galaxy, cr_wave_only, 4, {1, 7, 3, 5},
    {0}, {0}, {0}, true, chk_wave},
   {s2x4, cr_magic_only, 8, {0, 1, 3, 2, 5, 4, 6, 7},
    {0}, {0}, {0}, true, chk_wave},
   {s2x4, cr_peelable_box, 4, {0, 1, 2, 3},
    {4, 5, 6, 7}, {0}, {0}, false, chk_peelable},
   {s2x4, cr_couples_only, 2, {0, 1, 2, 3, 7, 6, 5, 4},
    {4}, {0}, {0}, true, chk_groups},
   {s2x4, cr_miniwaves, 1, {0, 1, 2, 3, 7, 6, 5, 4},
    {4}, {0}, {0}, true, chk_anti_groups},
   {s2x4, cr_all_facing_same, 8, {0, 1, 2, 3, 4, 5, 6, 7},
    {1}, {0}, {0}, true, chk_groups},
   {s1x2, cr_all_facing_same, 2, {0, 1},
    {1}, {0}, {0}, true, chk_groups},
   {s1x2, cr_li_lo, 2, {0, 1},
    {0}, {0}, {0}, true, chk_wave},
   {s2x4, cr_reg_tbone, 1, {2, 1, 2, 1, 0, 3, 0, 3},
    {2, 3, 2, 3, 0, 1, 0, 1}, {0}, {0}, false, chk_box},
   {s2x6, cr_wave_only, 12, {0, 6, 1, 7, 2, 8, 3, 9, 4, 10, 5, 11},
    {0}, {0}, {0}, true, chk_wave},
   {s2x6, cr_peelable_box, 6, {0, 1, 2, 3, 4, 5},
    {6, 7, 8, 9, 10, 11}, {0}, {0}, false, chk_peelable},
   {s2x6, cr_couples_only, 3, {0, 1, 2, 3, 4, 5, 11, 10, 9, 8, 7, 6},
    {4}, {0}, {0}, true, chk_groups},
   {s3x4, cr_3x3couples_only, 3, {0, 1, 2, 3, 10, 11, 5, 4, 9, 8, 7, 6},
    {4}, {0}, {0}, true, chk_groups},
   {s2x5, cr_wave_only, 10, {0, 5, 1, 6, 2, 7, 3, 8, 4, 9},
    {0}, {0}, {0}, true, chk_wave},
   {s2x5, cr_peelable_box, 5, {0, 1, 2, 3, 4},
    {5, 6, 7, 8, 9}, {0}, {0}, false, chk_peelable},
   {s2x8, cr_wave_only, 16, {0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15},
    {0}, {0}, {0}, true, chk_wave},
   {s2x8, cr_peelable_box, 8, {0, 1, 2, 3, 4, 5, 6, 7},
    {8, 9, 10, 11, 12, 13, 14, 15}, {0}, {0}, false, chk_peelable},
   {s2x8, cr_couples_only, 2, {0, 1, 2, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8},
    {8}, {0}, {0}, true, chk_groups},
   {s_qtag, cr_wave_only, 4, {2, 3, 7, 6},
    {0}, {0}, {0}, false, chk_wave},
   {s_short6, cr_tall6, 6, {4, 0, 2, 3, 5},
    {2, 1, 4}, {3, 0, 1, 5}, {3, 2, 3, 4}, true, chk_dmd_qtag},
   {s_short6, cr_wave_only, 6, {1, 0, 3, 2, 5, 4},
    {0}, {0}, {0}, true, chk_wave},
   {s_trngl, cr_wave_only, 4, {0, 1, 0, 2},
    {0}, {0}, {0}, true, chk_wave},
   {sdmd, cr_wave_only, 2, {1, 3},
    {0}, {0}, {0}, true, chk_wave},
   {sdmd, cr_miniwaves, 2, {1, 3},
    {0}, {0}, {0}, true, chk_wave},
   {s_ptpd, cr_wave_only, 4, {1, 3, 7, 5},
    {0}, {0}, {0}, true, chk_wave},
   {s_ptpd, cr_miniwaves, 1, {1, 7, 3, 5},
    {2}, {0}, {0}, true, chk_anti_groups},
   {nothing}};

restriction_tester::restr_initializer restriction_tester::restr_init_table4[] = {
   {sdmd, cr_jright, 4, {0, 2, 1, 3},                     {0}, {0}, {0}, true, chk_wave},
   {sdmd, cr_jleft,  4, {0, 2, 3, 1},                     {0}, {0}, {0}, true, chk_wave},
   {s2x4, cr_conc_iosame, 8, {6, 1, 0, 3, 5, 2, 7, 4},    {0}, {0}, {0}, true, chk_wave},
   {s2x4, cr_conc_iodiff, 8, {1, 6, 0, 3, 2, 5, 7, 4},    {0}, {0}, {0}, true, chk_wave},
   {s_ptpd, cr_jright, 8, {0, 2, 1, 3, 6, 4, 7, 5},       {0}, {0}, {0}, true, chk_wave},
   {s_ptpd, cr_jleft, 8,  {0, 2, 3, 1, 6, 4, 5, 7},       {0}, {0}, {0}, true, chk_wave},
   {s_hrglass, cr_jleft, 4, {6, 2, 7, 3},                 {0}, {0}, {0}, true, chk_wave},
   {nothing}};

restriction_tester::restr_initializer restriction_tester::restr_init_table9[] = {
   {s_qtag, cr_jleft, 8, {2, 3, 0, 4, 7, 6, 1, 5},
    {0}, {0}, {0}, true, chk_wave},
   {s_qtag, cr_jright, 8, {3, 2, 0, 4, 6, 7, 1, 5},
    {0}, {0}, {0}, true, chk_wave},
   {s_c1phan,cr_jleft,16, {2, 0, 1, 3, 5, 7, 4, 6, 8, 10, 11, 9, 15, 13, 14, 12},
    {0}, {0}, {0}, false, chk_wave},
   {s_c1phan,cr_jright,16,{0, 2, 1, 3, 7, 5, 4, 6, 10, 8, 11, 9, 13, 15, 14, 12},
    {0}, {0}, {0}, false, chk_wave},
   {s_qtag, cr_ijleft, 8, {2, 6, 0, 4, 3, 7, 1, 5},
    {0}, {0}, {0}, true, chk_wave},
   {s_qtag, cr_ijright, 8, {6, 2, 0, 4, 7, 3, 1, 5},
    {0}, {0}, {0}, true, chk_wave},
   {s_qtag, cr_diamond_like, 4, {4, 2, 3, 6, 7},
    {4, 0, 1, 4, 5}, {0}, {0}, false, chk_dmd_qtag},
   {s_qtag, cr_qtag_like, 4, {8, 0, 1, 2, 3, 4, 5, 6, 7},
    {0}, {2, 4, 5}, {2, 0, 1}, false, chk_dmd_qtag},
   {s_qtag, cr_qline_like_l, 8, {2, 0, 3, 1, 4, 6, 5, 7},
    {0}, {0}, {0}, false, chk_wave},
   {s_qtag, cr_qline_like_r, 8, {4, 0, 5, 1, 6, 2, 7, 3},
    {0}, {0}, {0}, false, chk_wave},
   {s_qtag, cr_qtag_like_anisotropic, 4, {8, 0, 1, 2, 3, 4, 5, 6, 7},
    {0}, {2, 0, 5}, {2, 4, 1}, false, chk_dmd_qtag_new},
   {s_qtag, cr_pu_qtag_like, 0, {0},
    {8, 0, 1, 2, 3, 4, 5, 6, 7}, {2, 0, 1}, {2, 4, 5}, false, chk_dmd_qtag},
   {s3dmd, cr_diamond_like, 0, {6, 3, 4, 5, 9, 10, 11},
    {6, 0, 1, 2, 6, 7, 8}, {0}, {0}, false, chk_dmd_qtag},
   {s3dmd, cr_qtag_like, 0, {12, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    {0}, {3, 6, 7, 8}, {3, 0, 1, 2}, false, chk_dmd_qtag},
   {s3dmd, cr_pu_qtag_like, 0, {0},
    {12, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    {3, 0, 1, 2}, {3, 6, 7, 8}, false, chk_dmd_qtag},
   {s4dmd, cr_jleft, 16, {4, 5, 0, 8, 6, 7, 1, 9, 15, 14, 2, 10, 13, 12, 3, 11},
    {0}, {0}, {0}, true, chk_wave},
   {s4dmd, cr_jright, 16, {5, 4, 0, 8, 7, 6, 1, 9, 14, 15, 2, 10, 12, 13, 3, 11},
    {0}, {0}, {0}, true, chk_wave},
   {s4dmd, cr_diamond_like, 8, {8, 4, 5, 6, 7, 12, 13, 14, 15},
    {8, 0, 1, 2, 3, 8, 9, 10, 11}, {0}, {0}, false, chk_dmd_qtag},
   {s4dmd, cr_qtag_like, 4, {16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {0}, {4, 8, 9, 10, 11}, {4, 0, 1, 2, 3}, false, chk_dmd_qtag},
   {s4dmd, cr_pu_qtag_like, 0, {0},
    {16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {4, 0, 1, 2, 3}, {4, 8, 9, 10, 11}, false, chk_dmd_qtag},
   {sdmd, cr_jright, 4, {2, 0, 1, 3},
    {8}, {0}, {0}, true, chk_wave},
   {sdmd, cr_jleft, 4, {2, 0, 3, 1},
    {8}, {0}, {0}, true, chk_wave},
   {s_star, cr_jleft, 4, {2, 0, 1, 3},
    {8}, {0}, {0}, true, chk_wave},
   {sdmd, cr_diamond_like, 4, {2, 0, 2},
    {2, 1, 3}, {0}, {0}, false, chk_dmd_qtag},
   {sdmd, cr_qtag_like, 4, {0},
    {4, 0, 1, 2, 3}, {1, 0}, {1, 2}, false, chk_dmd_qtag},
   {sdmd, cr_qtag_like_anisotropic, 4, {0},
    {4, 0, 1, 2, 3}, {2, 0, 2}, {0}, false, chk_dmd_qtag_new},
   {sdmd, cr_pu_qtag_like, 0, {4, 0, 1, 2, 3},
    {0}, {1, 0}, {1, 2}, false, chk_dmd_qtag},
   {s_ptpd, cr_jright, 8, {2, 0, 1, 3, 4, 6, 7, 5},
    {8}, {0}, {0}, true, chk_wave},
   {s_ptpd, cr_jleft, 8, {2, 0, 3, 1, 4, 6, 5, 7},
    {8}, {0}, {0}, true, chk_wave},
   {s_ptpd, cr_diamond_like, 4, {4, 0, 2, 4, 6},
    {4, 1, 3, 5, 7}, {0}, {0}, false, chk_dmd_qtag},
   {s_ptpd, cr_qtag_like, 4, {0},
    {8, 0, 1, 2, 3, 4, 5, 6, 7}, {2, 0, 6}, {2, 2, 4}, false, chk_dmd_qtag},
   {s_ptpd, cr_qtag_like_anisotropic, 4, {0},
    {8, 0, 1, 2, 3, 4, 5, 6, 7}, {2, 0, 2}, {2, 6, 4}, false, chk_dmd_qtag},
   {s4x6, cr_qtag_like, 8, {0},
    {16, 11, 1, 9, 10, 8, 4, 6, 7, 23, 13, 21, 22, 20, 16, 19, 18},
    {4, 11, 8, 18, 21}, {4, 9, 6, 23, 20}, false, chk_dmd_qtag},
   {s_ptpd, cr_pu_qtag_like, 0, {8, 0, 1, 2, 3, 4, 5, 6, 7},
    {0}, {2, 0, 6}, {2, 2, 4}, false, chk_dmd_qtag},
   {s2x4, cr_gen_qbox, 4, {0},
    {8, 0, 1, 2, 3, 4, 5, 6, 7}, {2, 1, 2}, {2, 5, 6}, false, chk_dmd_qtag},
   {s2x4, cr_split_square_setup, 4,
    {4, 1, 5, 6, 2}, {4, 0, 7, 3, 4}, {4, 0, 7, 5, 6}, {4, 3, 4, 1, 2}, false, chk_dmd_qtag},
   {s2x4, cr_liftoff_setup, 4,
    {4, 1, 5, 6, 2}, {4, 0, 7, 3, 4}, {4, 0, 7, 1, 2}, {4, 3, 4, 5, 6}, false, chk_dmd_qtag},
   {s_bone, cr_i_setup, 0, {2, 0, 5}, {2, 1, 4},
    {2, 3, 6}, {2, 7, 2},         false, chk_star},
   {s2x2, cr_wave_only, 1, {2, 0, 0, 2},
    {0, 0, 2, 2},         {0}, {0}, true, chk_box},
   {s2x2, cr_all_facing_same, 1, {2, 2, 2, 2},
    {0, 0, 0, 0},   {0}, {0}, true, chk_box},
   {s2x2, cr_2fl_only, 1, {2, 2, 2, 2},
    {0, 0, 0, 0},          {0}, {0}, true, chk_box},
   {s2x2, cr_trailers_only, 1, {0, 0, 2, 2},
    {0, 2, 2, 0},     {0}, {0}, true, chk_box},
   {s2x2, cr_leads_only, 1, {0, 0, 2, 2},
    {0, 2, 2, 0},        {0}, {0}, true, chk_box},
   {s2x2, cr_li_lo, 1, {0, 0, 2, 2},
    {0, 2, 2, 0},             {0}, {0}, true, chk_box},
   {s2x2, cr_indep_in_out, 0, {3, 2, 1, 0},
    {0}, {0}, {0}, false, chk_indep_box},
   {s2x2, cr_magic_only, 1, {2, 0, 2, 0},
    {0, 2, 0, 2},        {0}, {0}, true, chk_box},
   {nothing}};



void restriction_tester::initialize_tables()
{
   restr_initializer *tabp;
   int i;

   for (i=0 ; i<NUM_RESTR_HASH_BUCKETS ; i++) {
      restr_hash_table0[i] = (restr_initializer *) 0;
      restr_hash_table1[i] = (restr_initializer *) 0;
      restr_hash_table4[i] = (restr_initializer *) 0;
      restr_hash_table9[i] = (restr_initializer *) 0;
   }

   for (tabp = restr_init_table0 ; tabp->k != nothing ; tabp++) {
      uint32_t hash_num = ((tabp->k + (5*tabp->restr)) * 25) & (NUM_RESTR_HASH_BUCKETS-1);
      tabp->next = restr_hash_table0[hash_num];
      restr_hash_table0[hash_num] = tabp;
   }

   for (tabp = restr_init_table1 ; tabp->k != nothing ; tabp++) {
      uint32_t hash_num = ((tabp->k + (5*tabp->restr)) * 25) & (NUM_RESTR_HASH_BUCKETS-1);
      tabp->next = restr_hash_table1[hash_num];
      restr_hash_table1[hash_num] = tabp;
   }

   for (tabp = restr_init_table4 ; tabp->k != nothing ; tabp++) {
      uint32_t hash_num = ((tabp->k + (5*tabp->restr)) * 25) & (NUM_RESTR_HASH_BUCKETS-1);
      tabp->next = restr_hash_table4[hash_num];
      restr_hash_table4[hash_num] = tabp;
   }

   for (tabp = restr_init_table9 ; tabp->k != nothing ; tabp++) {
      uint32_t hash_num = ((tabp->k + (5*tabp->restr)) * 25) & (NUM_RESTR_HASH_BUCKETS-1);
      tabp->next = restr_hash_table9[hash_num];
      restr_hash_table9[hash_num] = tabp;
   }
}


restriction_tester::restr_initializer *restriction_tester::get_restriction_thing(
   setup_kind k, assumption_thing t)
{
   restr_initializer **hash_table;
   uint32_t hash_num = ((k + (5*t.assumption)) * 25) & (NUM_RESTR_HASH_BUCKETS-1);
   uint32_t col = t.assump_col;

   // Check for a few special cases that need to be translated.

   if (k == s2x4 && t.assumption == cr_magic_only && (col & 2)) {
      // "assume inverted boxes", people are in general lines (col=2) or cols (col=3)
      col = 0;   /* This is wrong, need something that allows
                    real mag col or unsymm congruent inv boxes.
                    This is a "check mixed groups" type of thing!!!! */
   }

   // Use a specific hash table to search for easy cases.
   // Otherwise, search the general table.

   bool can_try_gen_table = true;

   do {
      switch (col) {
      case 0:  hash_table = restr_hash_table0; break;
      case 1:  hash_table = restr_hash_table1; break;
      case 4:  hash_table = restr_hash_table4; break;
      default: hash_table = restr_hash_table9;
         can_try_gen_table = false;
         break;
      }

      for ( restr_initializer *restr_hash_bucket = hash_table[hash_num] ;
            restr_hash_bucket ;
            restr_hash_bucket = restr_hash_bucket->next) {
         if (restr_hash_bucket->k == k && restr_hash_bucket->restr == t.assumption)
            return restr_hash_bucket;
      }

      col = 99;   // If we go back, only do the general table.
   }
   while (can_try_gen_table);   // Go back one more time if did a specific table.

   return (restr_initializer *) 0;
}


void create_active_phantom(personrec *person_p, uint32_t dir, int *phantom_count_p)
{
   if (!person_p->id1) {
      if (*phantom_count_p >= 16) fail("Too many phantoms.");
      person_p->id1 = dir | BIT_ACT_PHAN | (((*phantom_count_p)++) << 6);
      person_p->id2 = 0U;
      person_p->id3 = 0U;
   }
   else if (person_p->id1 & BIT_ACT_PHAN)
      fail("Active phantoms may only be used once.");
}


restriction_test_result verify_restriction(
   setup *ss,
   assumption_thing tt,
   bool instantiate_phantoms,
   bool *failed_to_instantiate) THROW_DECL
{
   int idx, limit, i, j;
   uint32_t t = 0;
   uint32_t qa0, qa1, qa2, qa3;
   uint32_t qaa[4];
   uint32_t pdir, qdir, pdirodd, qdirodd;
   uint32_t dirtest2[2];
   const int8_t *p, *q;
   int phantom_count = 0;
   unsigned int live_test = tt.assump_live;  // We might clear this later.
   restriction_tester::restr_initializer *rr;
   int k = 0;
   int local_negate = tt.assump_negate;

   if (tt.assumption == cr_not_all_sel || tt.assumption == cr_some_sel)
      local_negate = 1;

   call_restriction orig_assumption = tt.assumption;

   switch (tt.assumption) {
   case cr_alwaysfail:
      return restriction_fails;
   case cr_give_fudgy_warn:
      warn(warn__may_be_fudgy);
      return restriction_passes;

   case cr_wave_unless_say_2faced:
      tt.assumption = cr_wave_only;
      break;

   case cr_ptp_unwrap_sel:
      k ^= (ss->kind == s2x2dmd) ? (0x2828 ^ 0x4444) : (022002200 ^ 014001400);
   case cr_nor_unwrap_sel:
      k ^= (ss->kind == s2x2dmd) ? 0x4444 : 014001400;
      goto cr_none_sel_label;

   case cr_ends_sel:
      k = ~k;
      /* FALL THROUGH!!!!!! */
   case cr_ctrs_sel:
      /* FELL THROUGH!!!!!! */

      switch (ss->kind) {
      case s1x4: k ^= 0x5; break;
      case s2x4: k ^= 0x99; break;
      case s_qtag: case s_bone: case s_hrglass: case s_dhrglass: k ^= 0x33; break;
      case s_rigger: k ^= 0xCC; break;
      default: goto bad;
      }
      /* FALL THROUGH!!!!!! */
   case cr_not_all_sel:
   case cr_all_sel:
      /* FELL THROUGH!!!!!! */
      k = ~k;
      /* FALL THROUGH!!!!!! */

   case cr_some_sel:
   case cr_none_sel:
   cr_none_sel_label:
      /* FELL THROUGH!!!!!! */
      for (idx=0 ; idx<=attr::slimit(ss) ; idx++,k>>=1) {
         if (!ss->people[idx].id1) {
            if (tt.assump_live) goto bad;
         }
         else if (selectp(ss, idx)) {
            if (!(k&1)) goto bad;
         }
         else {
            if (k&1) goto bad;
         }
      }
      goto good;

   case cr_nice_diamonds:
      // B=1 => right-handed; B=2 => left-handed.
      tt.assumption = cr_jright;
      tt.assump_col = 4;
      break;
   case cr_dmd_facing:
      // This isn't documented, but it should work.
      // If it does, can get rid of some special code in assoc.
      // B=1 => pts are right-handed; B=2 => pts are left-handed.
      tt.assumption = cr_jleft;
      tt.assump_col = 4;
      break;
   case cr_conc_iosame:
   case cr_conc_iodiff:
      tt.assump_col = 4;
      break;
   case cr_leads_only:
      tt.assump_both = 2;
      break;
   case cr_trailers_only:
      tt.assump_both = 1;
      break;
   case cr_levelplus:
      if (calling_level < l_plus) return restriction_bad_level;
      goto good;
   case cr_levela1:
      if (calling_level < l_a1) return restriction_bad_level;
      goto good;
   case cr_levela2:
      if (calling_level < l_a2) return restriction_bad_level;
      goto good;
   case cr_levelc1:
      if (calling_level < l_c1) return restriction_bad_level;
      goto good;
   case cr_levelc2:
      if (calling_level < l_c2) return restriction_bad_level;
      goto good;
   case cr_levelc3a:
      if (calling_level < l_c3a) return restriction_bad_level;
      goto good;
   case cr_levelc3:
      if (calling_level < l_c3) return restriction_bad_level;
      goto good;
   case cr_levelc4a:
      if (calling_level < l_c4a) return restriction_bad_level;
      goto good;
   case cr_levelc4:
      if (calling_level < l_c4) return restriction_bad_level;
      goto good;
   case cr_siamese_in_quad:
      t ^= 2;
      // FALL THROUGH!!!!!
   case cr_not_tboned_in_quad:
      // FELL THROUGH!!
      t ^= 1;
      // This is independent of whether we are line-like or column-like.
      for (idx=0 ; idx<4 ; idx++) {
         int ii;
         int perquad = (attr::slimit(ss)+1) >> 2;
         qa0 = 0; qa1 = 0; qa2 = 0; qa3 = 0;
         // Test one quadrant.
         for (ii=0 ; ii<perquad ; ii++) {
            uint32_t tp;
            if ((tp = ss->people[perquad*idx+ii].id1) != 0) {
               qa0 |= (tp^1);
               qa1 |= (tp^3);
               qa2 |= (tp^2);
               qa3 |= (tp);
            }
         }
         if ((qa0&t) && (qa1&t) && (qa2&t) && (qa3&t)) goto bad;
      }
      goto good;
   }

   rr = restriction_tester::get_restriction_thing(ss->kind, tt);
   if (!rr) return restriction_no_item;

   dirtest2[0] = 0;
   dirtest2[1] = 0;

   *failed_to_instantiate = true;

   int8_t *map1item;
   int szlim;

   switch (rr->check) {
   case restriction_tester::chk_line_col_aspect:
      qa1 = 0;

      for (idx=0; idx<rr->size; idx++) {
         qa1 |=  ss->people[rr->map1[idx]].id1;
      }

      if ((qa1 >> (tt.assump_col*3)) & 1)
         goto bad;

      goto good;
   case restriction_tester::chk_spec_directions:
      qa1 = 0;
      qa0 = 3 & (~tt.assump_both);

      p = rr->map1;
      qa2 = rr->map2[0];
      qa3 = rr->map2[1];

      while (*p>=0) {
         uint32_t t1 = ss->people[*(p++)].id1;
         uint32_t t2 = ss->people[*(p++)].id1;
         qa1 |= t1 | t2;
         if (t1 && (t1 & 3)!=qa2) qa0 &= ~2;
         if (t2 && (t2 & 3)!=qa3) qa0 &= ~2;
         if (t1 && (t1 & 3)!=qa3) qa0 &= ~1;
         if (t2 && (t2 & 3)!=qa2) qa0 &= ~1;
         if ((t1|t2) == 0 && tt.assump_live) goto bad;
      }

      if (qa1) {
         if (rr->map2[2]) {
            if (!qa0) goto good;
         }
         else {
            if (qa0) goto good;
         }
      }

      goto bad;
   case restriction_tester::chk_wave:
      // Fix grand chain 8.  If the "assume dpt" command
      // was given, don't issue the warning.
      if (ss->kind == s2x4 &&
          ss->cmd.cmd_assume.assump_both == 1 &&
          ss->cmd.cmd_assume.assumption == cr_2fl_only &&
          ss->cmd.cmd_assume.assump_col == 1)
         live_test = 0;

      qaa[0] = tt.assump_both;
      qaa[1] = tt.assump_both << 1;

      for (idx=0; idx<rr->size; idx++) {
         if ((t = ss->people[rr->map1[idx]].id1) != 0) {
            qaa[idx&1] |=  t;
            qaa[(idx&1)^1] |= t^2;
         }
         else if (local_negate || live_test) goto bad;    // All live people were demanded.
      }

      if ((qaa[0] & qaa[1] & 2) != 0)
         goto bad;

      if (rr->ok_for_assume) {
         uint32_t qab[4];
         qab[0] = 0;
         qab[2] = 0;

         for (idx=0; idx<rr->size; idx++)
            qab[idx&2] |= ss->people[rr->map1[idx]].id1;

         if ((tt.assump_col | rr->map2[0]) & 4) {
            qab[2] >>= 3;
         }
         else if (tt.assump_col == 1 || (rr->map2[0] & 8)) {
            qab[0] >>= 3;
            qab[2] >>= 3;
         }

         if ((qab[0]|qab[2]) & 1) goto bad;

         if (instantiate_phantoms) {
            *failed_to_instantiate = false;

            if (qaa[0] == 0) fail("Need live person to determine handedness.");

            if ((tt.assump_col | rr->map2[0]) & 4) {
               if ((qaa[0] & 2) == 0) { pdir = d_north; qdir = d_south; }
               else                   { pdir = d_south; qdir = d_north; }
               pdirodd = rotcw(pdir); qdirodd = rotcw(qdir);
            }
            else if (tt.assump_col == 1 || (rr->map2[0] & 8)) {
               if ((qaa[0] & 2) == 0) { pdir = d_east; qdir = d_west; }
               else                   { pdir = d_west; qdir = d_east; }
               pdirodd = pdir; qdirodd = qdir;
            }
            else {
               if ((qaa[0] & 2) == 0) { pdir = d_north; qdir = d_south; }
               else                   { pdir = d_south; qdir = d_north; }
               pdirodd = pdir; qdirodd = qdir;
            }

            for (i=0; i<rr->size; i++) {
               uint32_t dir = (i&1) ? ((i&2) ? qdirodd : qdir) : ((i&2) ? pdirodd : pdir);
               create_active_phantom(&ss->people[rr->map1[i]], dir, &phantom_count);
            }
         }
      }

      goto good;
   case restriction_tester::chk_box:
      p = rr->map1;
      q = rr->map2;

      for (i=0; i<rr->size; i++) {
         qa0 = (tt.assump_both << 1) & 2;
         qa1 = tt.assump_both & 2;
         qa2 = qa1;
         qa3 = qa0;

         for (idx=0 ; idx<=attr::slimit(ss) ; idx++) {
            if ((t = ss->people[idx].id1) != 0) {
               qa0 |= t^(*p)^0;
               qa1 |= t^(*p)^2;
               qa2 |= t^(*q)^1;
               qa3 |= t^(*q)^3;
            }
            else if (tt.assump_live) goto bad;

            p++;
            q++;
         }

         if ((qa1&3) == 0 || (qa0&3) == 0 || (qa3&3) == 0 || (qa2&3) == 0)
            goto check_box_assume;
      }

      goto bad;

   check_box_assume:
      if (rr->ok_for_assume) {    /* Will not be true unless size = 1 */
         if (instantiate_phantoms) {
            if (!(qa0 & BIT_PERSON))
               fail("Need live person to determine handedness.");

            for (i=0 ; i<=attr::slimit(ss) ; i++) {
               pdir = (qa0&1) ?
                  (d_east ^ ((rr->map2[i] ^ qa2) & 2)) :
                  (d_north ^ ((rr->map1[i] ^ qa0) & 2));
               create_active_phantom(&ss->people[i], pdir, &phantom_count);
            }

            *failed_to_instantiate = false;
         }
      }

      goto good;
   case restriction_tester::chk_box_dbl:
      // Check everyone's lateral partner, independently of headlinerness.
      for (idx=0 ; idx<=attr::slimit(ss) ; idx++) {
         uint32_t u;

         if ((t = ss->people[idx].id1) & 010) p = rr->map1;
         else if ((t = ss->people[idx].id1) & 1) p = rr->map2;
         else continue;

         p += idx;

         if ((u = ss->people[p[0]].id1) && ((t ^ u ^ rr->size) & 3)) goto bad;

         if (rr->restr == cr_miniwaves) {
            if (((((t>>1)&1)+1) ^ p[4]) & tt.assump_both) goto bad;
         }
      }

      goto good;
   case restriction_tester::chk_indep_box:
      qa0 = (tt.assump_both << 1) & 2;
      qa1 = tt.assump_both & 2;
      qa2 = 0;
      qa3 = 0;

      for (idx=0 ; idx<=attr::slimit(ss) ; idx++) {
         if ((t = ss->people[idx].id1) != 0) {
            qa2 |= t + rr->map1[idx];
            qa3 |= t + rr->map1[idx] + 2;
         }
      }

      if (((qa0 & qa2) | (qa1 & qa3)) != 0) goto bad;

      goto good;
   case restriction_tester::chk_groups:
      limit = rr->map2[0];

      for (idx=0; idx<limit; idx++) {
         qaa[idx&1] = (tt.assump_both<<1) & 2;
         qaa[(idx&1)^1] = tt.assump_both & 2;

         for (i=0,j=idx; i<rr->size; i++,j+=limit) {
            if ((t = ss->people[rr->map1[j]].id1) != 0) { qaa[0] |= t; qaa[1] |= t^2; }
            else if (local_negate || tt.assump_live) goto bad;    // All live people were demanded.
         }

         if ((qaa[0] & qaa[1] & 2) != 0) goto bad;

         if (rr->ok_for_assume) {
            if (tt.assump_col == 1) {
               if ((qaa[0] & 2) == 0) { pdir = d_east; }
               else                   { pdir = d_west; }

               qaa[0] >>= 3;
               qaa[1] >>= 3;
            }
            else {
               if ((qaa[0] & 2) == 0) { pdir = d_north; }
               else                   { pdir = d_south; }
            }

            if ((qaa[0]|qaa[1])&1) goto bad;

            if (instantiate_phantoms) {
               if (qaa[0] == 0) fail("Need live person to determine handedness.");

               for (i=0,k=0 ; k<rr->size ; i+=limit,k++) {
                  create_active_phantom(&ss->people[rr->map1[idx+i]], pdir, &phantom_count);
               }

               *failed_to_instantiate = false;
            }
         }
      }
      goto good;
   case restriction_tester::chk_groups_cpls_in_tbone:
      limit = rr->map2[0];

      for (idx=0; idx<limit; idx++) {
         qa0 = 0; qa1 = 0;

         for (i=0; i<rr->size; i++) {
            if ((t = ss->people[rr->map1[idx+i*limit]].id1) != 0) { qa0 |= t; qa1 |= ~t; }
            else if (local_negate || tt.assump_live) goto bad;    // All live people were demanded.
         }

         if ((qa0 & qa1 & 2) != 0) goto bad;
         if ((qa0 >> (idx&1 ? 3 : 0)) & 1) goto bad;
      }
      goto good;
   case restriction_tester::chk_groups_cpls_in_tbone_either_way:
      limit = rr->map2[0];

      {
         uint32_t allqa0 = 0;
         uint32_t allqa2 = 0;
         uint32_t allcol = 0;
         uint32_t allrow = 0;
         for (idx=0; idx<limit; idx++) {
            qa0 = 0; qa1 = 0; qa2 = 0; qa3 = 0;

            for (i=0; i<rr->size; i++) {
               if ((t = ss->people[rr->map1[idx+i*limit]].id1) != 0) { qa0 |= t; qa1 |= ~t; }
               else if (local_negate || tt.assump_live) goto bad;    // All live people were demanded.
               if ((t = ss->people[rr->map1[idx*limit+i]].id1) != 0) { qa2 |= t; qa3 |= ~t; }
            }

            if ((qa0 & 9) == 9 || (qa2 & 9) == 9) goto bad;  // People are T-boned.

            allqa0 |= qa0;          // If allqa0 & 8, some column fails (but rows might be OK.)
            allqa2 |= qa2;          // If allqa2 & 1, some row fails (but columns might be OK.)
            allcol |= (qa0 & qa1);  // If allcol & 2, some column fails (but rows might be OK.)
            allrow |= (qa2 & qa3);  // If allrow & 2, some row fails (but columns might be OK.)
         }

         if ((allcol | (allqa0 >> 2)) & (allrow | (allqa2 << 1)) & 2) goto bad;
      }
      goto good;
   case restriction_tester::chk_anti_groups:
      limit = rr->map2[0];
      // Under certain circumstances (dividing a qtag made of different-handedness single qtags)
      // cr_miniwaves has no handedness.  (Though after division, in the qingle qtags, it will.)
      if (rr->map2[1] != 0) tt.assump_both = 0;
      map1item = rr->map1;
      szlim = rr->size*limit;

      for (idx=0; idx<limit; idx++) {
         qa0 = 0; qa1 = 0;

         for (int jjj=0 ; jjj<rr->size ; jjj++) {
            if ((t = ss->people[map1item[0]].id1) != 0)     { qa0 |= t;   qa1 |= t^2; }
            else if (local_negate) goto bad;    // All live people were demanded.
            if ((t = ss->people[map1item[szlim]].id1) != 0) { qa0 |= t^2; qa1 |= t;   }
            else if (local_negate) goto bad;    // All live people were demanded.
            map1item++;
         }

         if ((qa0 & qa1 & 2) != 0) goto bad;

         if ((((tt.assump_both & qa1) | ((tt.assump_both<<1) & qa0)) & 2) != 0)
            goto bad;

         if (rr->ok_for_assume) {
            if (tt.assump_col == 1) {
               if ((qa0 & 2) == 0) { pdir = d_east; qdir = d_west; }
               else                { pdir = d_west; qdir = d_east; }

               qa0 >>= 3;
               qa1 >>= 3;
            }
            else {
               if ((qa0 & 2) == 0) { pdir = d_north; qdir = d_south; }
               else                { pdir = d_south; qdir = d_north; }
            }

            if ((qa0|qa1)&1) goto bad;

            if (instantiate_phantoms) {
               if (qa0 == 0)
                  fail("Need live person to determine handedness.");

               create_active_phantom(&ss->people[rr->map1[idx]], pdir, &phantom_count);
               create_active_phantom(&ss->people[rr->map1[idx+limit]], qdir, &phantom_count);
               *failed_to_instantiate = false;
            }
         }
      }
      goto good;
   case restriction_tester::chk_peelable:
      qa0 = 3; qa1 = 3;
      qa2 = 3; qa3 = 3;

      for (j=0; j<rr->size; j++) {
         if ((t = ss->people[rr->map1[j]].id1) != 0)  { qa0 &= t; qa1 &= t^2; }
         if ((t = ss->people[rr->map2[j]].id1) != 0)  { qa2 &= t; qa3 &= t^2; }
      }

      if ((((~qa0)&3) && ((~qa1)&3)) ||
          (((~qa2)&3) && ((~qa3)&3)))
         goto bad;

      goto good;
   case restriction_tester::chk_qbox:
      qa0 = 0;
      qa1 = ~0;

      for (idx=0; idx<rr->map1[0]; idx++) {
         if ((t = ss->people[rr->map1[idx+1]].id1) & 1) qa0 |= t;
      }

      for (idx=0; idx<rr->map2[0]; idx++) {
         if ((t = ss->people[rr->map2[idx+1]].id1) & 1) qa0 |= ~t;
      }

      for (idx=0; idx<rr->map3[0]; idx++) {
         if ((t = ss->people[rr->map3[idx+1]].id1) & 1) qa1 &= t;
      }

      for (idx=0; idx<rr->map4[0]; idx++) {
         if ((t = ss->people[rr->map4[idx+1]].id1) & 1) qa1 &= ~t;
      }

      if (qa0 & ~qa1 & 2)
         goto bad;

      goto good;
   case restriction_tester::chk_star:
      qa0 = tt.assump_both;
      qa1 = ~(tt.assump_both << 1);

      for (idx=0; idx<rr->map1[0]; idx++) {
         if ((t = ss->people[rr->map1[idx+1]].id1) & 1) { qa0 |= t; qa1 &= t; }
      }

      for (idx=0; idx<rr->map2[0]; idx++) {
         if ((t = ss->people[rr->map2[idx+1]].id1) & 1) { qa0 |= ~t; qa1 &= ~t; }
      }

      for (idx=0; idx<rr->map3[0]; idx++) {
         if ((t = ss->people[rr->map3[idx+1]].id1) & 010) { qa0 |= t; qa1 &= t; }
      }

      for (idx=0; idx<rr->map4[0]; idx++) {
         if ((t = ss->people[rr->map4[idx+1]].id1) & 010) { qa0 |= ~t; qa1 &= ~t; }
      }

      if (qa0 & ~qa1 & 2)
         goto bad;

      goto good;
   case restriction_tester::chk_dmd_qtag:
   case restriction_tester::chk_dmd_qtag_new:
      qa0 = qa1 = qa2 = qa3 = 0;

      for (idx=0; idx<rr->map1[0]; idx++)
         qa1 |= ss->people[rr->map1[idx+1]].id1;

      for (idx=0; idx<rr->map2[0]; idx++)
         qa0 |= ss->people[rr->map2[idx+1]].id1;

      for (idx=0; idx<rr->map3[0]; idx++) {
         if ((t = ss->people[rr->map3[idx+1]].id1) != 0)
            { qa2 |= t; qa3 |= ~t; }
      }

      for (idx=0; idx<rr->map4[0]; idx++) {
         if ((t = ss->people[rr->map4[idx+1]].id1) != 0)
            { qa2 |= ~t; qa3 |= t; }
      }

      if ((qa1 & 001) != 0 || (qa0 & 010) != 0)
         goto bad;

      // If "both" is on, we demand correct facing directions for points.
      if (tt.assump_both) {
         if ((qa2 & (tt.assump_both << 1) & 2) != 0 ||
             (qa3 & tt.assump_both & 2) != 0)
            goto bad;
      }

      if (rr->check == restriction_tester::chk_dmd_qtag) {
         if (tt.assump_both) {
            // The "live" modifier means that we need a definitive person
            // to distinguish "in" or "out".
            if (tt.assump_live && !(qa2 | qa3))
               goto bad;
         }
      }
      else {
         // The "live" flag means exactly what it says.
         if (tt.assump_live) {
            for (idx=0 ; idx<=attr::slimit(ss) ; idx++) {
               if (ss->people[idx].id1 == 0) goto bad;
            }
         }

         // If "both" is off, we demand consistent handedness.
         if (!tt.assump_both && (qa2 & qa3 & 2) != 0)
            goto bad;
      }

      goto good;
   case restriction_tester::chk_qtag:
      // If what we are searching for has handedness contrary to an incoming assumption, fail.
      if (((tt.assump_both | ss->cmd.cmd_assume.assump_both) & 3) == 3)
         goto bad;

      qaa[0] = tt.assump_both;
      qaa[1] = tt.assump_both << 1;

      for (idx=0; idx<rr->size; idx++) {
         if ((t = ss->people[rr->map1[idx]].id1) != 0) {
            qaa[idx&1] |=  t;
            qaa[(idx&1)^1] |= t^2;
         }
      }

      if ((qaa[0] & qaa[1] & 2) != 0)
         goto bad;

      for (idx=0; idx<rr->map2[0]; idx++) {
         if ((t = ss->people[rr->map2[idx+1]].id1) != 0 && ((t ^ (idx << 1)) & 2) != 0)
            goto bad;
      }

      // Check all NS for 1/4 tag, all EW for 1/4 box.
      for (idx=0 ; idx<8 ; idx++) { if (ss->people[idx].id1 & rr->map3[0]) goto bad; }

      if (rr->ok_for_assume) {
         uint32_t qab[4];
         qab[0] = 0;
         qab[2] = 0;

         for (idx=0; idx<rr->size; idx++)
            qab[idx&2] |= ss->people[rr->map1[idx]].id1;

         if ((qab[0]|qab[2]) & rr->map3[0]) goto bad;

         if (instantiate_phantoms) {
            *failed_to_instantiate = false;

            if (qaa[0] == 0)
               fail("Need live person to determine handedness.");

            if ((qaa[0] & 2) == 0) { pdir = d_north; qdir = d_south; }
            else                   { pdir = d_south; qdir = d_north; }

            if (rr->map3[0] & 010) { pdir = rotcw(pdir); qdir = rotcw(qdir); }

            for (i=0; i<rr->size; i++) {
               uint32_t dir = (i&1) ? qdir : pdir;
               create_active_phantom(&ss->people[rr->map1[i]], dir, &phantom_count);
            }

            pdir = d_south; qdir = d_north;

            if (rr->map3[0] & 010) { pdir = rotcw(pdir); qdir = rotcw(qdir); }

            for (i=0; i<rr->map2[0]; i++) {
               uint32_t dir = (i&1) ? pdir : qdir;
               create_active_phantom(&ss->people[rr->map2[i+1]], dir, &phantom_count);
            }
         }
      }

      goto good;

   case restriction_tester::chk_sex:
      qaa[0] = tt.assump_both;
      qaa[1] = tt.assump_both << 1;

      for (i=0; i<2; i++) {
         if ((t = ss->people[i].id1)) {
            uint32_t t2 = ss->people[i].id2;
            uint32_t northified = (i ^ (t>>1)) & 1;
            dirtest2[i] = t2;
            if (t2 & ID2_PERM_BOY) qaa[northified] |= 2;
            else if (t2 & ID2_PERM_GIRL) qaa[northified^1] |= 2;
            else goto bad;
         }
      }

      if ((dirtest2[0] & dirtest2[1]) & (ID2_PERM_BOY | ID2_PERM_GIRL))
         goto bad;

      // If this is a couple, check the "left" or "right" bits.
      if (rr->map4[0] && (qaa[0] & qaa[1] & 2)) goto bad;

      goto good;

   case restriction_tester::chk_inroller:

      // This "size" field tells whether this is an inroll/outroll
      // type (size != 0) or a judge/socker type (size == 0).

      if (ss->kind == s2x4 && rr->size != 0) {
         // For these assumptions, finding a single usable person
         // will suffice to determine the outcome.
         // Now it happens that, if assump_both has something,
         // we could determine the handedness, and hence the
         // in/out roll direction, without even one person.
         // But we won't be called if there isn't even one person.
         // ***** Not so!  We might be called with one person in the
         // center, but no ends.  Too bad.
         switch (ss->cmd.cmd_assume.assumption) {
         case cr_wave_only:
            if ((ss->people[0].id1 & 013) == (uint32_t) rr->map2[1] ||
                (ss->people[1].id1 & 013) == (uint32_t) rr->map2[0] ||
                (ss->people[2].id1 & 013) == (uint32_t) rr->map2[1] ||
                (ss->people[3].id1 & 013) == (uint32_t) rr->map2[0] ||
                (ss->people[4].id1 & 013) == (uint32_t) rr->map2[0] ||
                (ss->people[5].id1 & 013) == (uint32_t) rr->map2[1] ||
                (ss->people[6].id1 & 013) == (uint32_t) rr->map2[0] ||
                (ss->people[7].id1 & 013) == (uint32_t) rr->map2[1])
               goto good;
            break;
         case cr_2fl_only:
            if ((ss->people[0].id1 & 013) == (uint32_t) rr->map2[1] ||
                (ss->people[2].id1 & 013) == (uint32_t) rr->map2[0] ||
                (ss->people[1].id1 & 013) == (uint32_t) rr->map2[1] ||
                (ss->people[3].id1 & 013) == (uint32_t) rr->map2[0] ||
                (ss->people[4].id1 & 013) == (uint32_t) rr->map2[0] ||
                (ss->people[6].id1 & 013) == (uint32_t) rr->map2[1] ||
                (ss->people[5].id1 & 013) == (uint32_t) rr->map2[0] ||
                (ss->people[7].id1 & 013) == (uint32_t) rr->map2[1])
               goto good;
            break;
         }
      }

      if ((ss->people[rr->map1[0]].id1 & 013) == (uint32_t) rr->map2[0] &&
          (ss->people[rr->map1[1]].id1 & 013) == (uint32_t) rr->map2[1] &&
          (ss->people[rr->map1[2]].id1 & 013) != (uint32_t) rr->map2[2] &&
          (ss->people[rr->map1[3]].id1 & 013) != (uint32_t) rr->map2[3])
         goto good;

      if ((ss->people[rr->map1[4]].id1 & 013) == (uint32_t) rr->map2[4] &&
          (ss->people[rr->map1[5]].id1 & 013) == (uint32_t) rr->map2[5] &&
          (ss->people[rr->map1[6]].id1 & 013) == 0 &&
          (ss->people[rr->map1[7]].id1 & 013) == 0)
         goto good;

      goto bad;

   default:
      goto bad;    // Shouldn't happen.
   }

 good:
   if (local_negate) return restriction_fails;
   else return restriction_passes;

 bad:
   if (local_negate) return restriction_passes;
   else if (orig_assumption == cr_wave_unless_say_2faced) return restriction_fails_on_2faced;
   else return restriction_fails;
}


enum {
   MASK_CTR_2 = ((1 << call_list_any) | (1 << call_list_1x8) |
                 (1 << call_list_l1x8) | (1 << call_list_l1x8) |
                 (1 << call_list_gcol) | (1 << call_list_qtag)) >> 1,

   MASK_QUAD_D = ((1 << call_list_any) | (1 << call_list_1x8) |
                  (1 << call_list_l1x8) | (1 << call_list_l1x8) |
                  (1 << call_list_gcol)) >> 1,

   MASK_CPLS = ((1 << call_list_any) | (1 << call_list_dpt) |
                (1 << call_list_cdpt) | (1 << call_list_8ch) |
                (1 << call_list_tby) | (1 << call_list_lin) |
                (1 << call_list_lout) | (1 << call_list_r2fl) |
                (1 << call_list_l2fl) | (1 << call_list_qtag)) >> 1,

   MASK_GOODCPLS = ((1 << call_list_dpt) | (1 << call_list_cdpt) |
                    (1 << call_list_8ch) | (1 << call_list_tby) |
                    (1 << call_list_lin) | (1 << call_list_lout) |
                    (1 << call_list_r2fl) | (1 << call_list_l2fl)) >> 1,

   MASK_TAND = ((1 << call_list_any) | (1 << call_list_dpt) |
                (1 << call_list_cdpt) | (1 << call_list_rcol) |
                (1 << call_list_lcol) | (1 << call_list_rwv) |
                (1 << call_list_lwv) | (1 << call_list_r2fl) |
                (1 << call_list_l2fl) | (1 << call_list_gcol) |
                (1 << call_list_qtag)) >> 1,

   MASK_GOODTAND = ((1 << call_list_dpt) | (1 << call_list_cdpt) |
                    (1 << call_list_rcol) | (1 << call_list_lcol) |
                    (1 << call_list_rwv) | (1 << call_list_lwv) |
                    (1 << call_list_r2fl) | (1 << call_list_l2fl)) >> 1,

   MASK_GOODCONC = ((1 << call_list_1x8) | (1 << call_list_l1x8) |
                    (1 << call_list_dpt) | (1 << call_list_cdpt) |
                    (1 << call_list_rcol) | (1 << call_list_lcol) |
                    (1 << call_list_8ch) | (1 << call_list_tby) |
                    (1 << call_list_lin) | (1 << call_list_lout) |
                    (1 << call_list_rwv) | (1 << call_list_lwv) |
                    (1 << call_list_r2fl) | (1 << call_list_l2fl)) >> 1,

   MASK_GOODRMVD = ((1 << call_list_dpt) | (1 << call_list_cdpt) |
                    (1 << call_list_rcol) | (1 << call_list_lcol) |
                    (1 << call_list_8ch) | (1 << call_list_tby) |
                    (1 << call_list_lin) | (1 << call_list_lout) |
                    (1 << call_list_rwv) | (1 << call_list_lwv) |
                    (1 << call_list_r2fl) | (1 << call_list_l2fl)) >> 1,

   MASK_2X4 = ((1 << call_list_any) | (1 << call_list_dpt) |
               (1 << call_list_cdpt) | (1 << call_list_rcol) |
               (1 << call_list_lcol) | (1 << call_list_8ch) |
               (1 << call_list_tby) | (1 << call_list_lin) |
               (1 << call_list_lout) | (1 << call_list_rwv) |
               (1 << call_list_lwv) | (1 << call_list_r2fl) |
               (1 << call_list_l2fl)) >> 1,

   MASK_SIAM = ((1 << call_list_any) | (1 << call_list_qtag)) >> 1
};


// This fills in concept_sublist_sizes and concept_sublists.
static void initialize_concept_sublists()
{
   int test_call_list_kind;

   // Decide whether we allow the "diagnose" concept, by deciding
   // when we will stop the concept list scan.
   concept_kind end_marker = ui_options.diagnostic_mode ?
      marker_end_of_list : concept_diagnose;

   // First, just count up all the available concepts.

   int all_legal_concepts = 0;
   const concept_descriptor *p;

   for (p = concept_descriptor_table ; p->kind != end_marker ; p++) {
      if (p->level <= calling_level) all_legal_concepts++;
   }

   concept_sublists[call_list_any] = new short int[all_legal_concepts];
   good_concept_sublists[call_list_any] = new short int[all_legal_concepts];

   // Make the concept sublists, one per setup.  We do them in downward order,
   // with "any setup" last.  This is because we put our results into the
   // "any setup" slot while we are working, and then copy them to the
   // correct slot for each setup other than "any".

   for (test_call_list_kind = (int) call_list_qtag;
        test_call_list_kind >= (int)call_list_any;
        test_call_list_kind--) {
      int concept_index, concepts_in_this_setup, good_concepts_in_this_setup;

      for (concept_index=0,concepts_in_this_setup=0,good_concepts_in_this_setup=0;
           ;
           concept_index++) {
         uint32_t setup_mask = ~0;      // Default is to accept the concept.
         uint32_t good_setup_mask = 0;  // Default for this is not to.
         const concept_descriptor *p = &concept_descriptor_table[concept_index];
         if (p->kind == end_marker) break;
         if (p->kind == concept_omit) continue;  // Totally stupid concept!!!!
         if (p->level > calling_level) continue;

         // This concept is legal at this level.  See if it is appropriate for this setup.
         // If we don't know, the default value of setup_mask will make it legal.

         switch (p->kind) {
         case concept_centers_or_ends:
         case concept_centers_and_ends:
            switch (p->arg1) {
            case selector_center6:
            case selector_outer6:
            case selector_center2:
            case selector_verycenters:
            case selector_outer2:
            case selector_veryends:
               setup_mask = MASK_CTR_2;    // This is a 6 and 2 type of concept.
               break;
            default:
               break;
            }

            break;
         case concept_concentric:
            switch (p->arg1) {
            case schema_concentric:
            case schema_cross_concentric:
            case schema_single_concentric:
            case schema_single_cross_concentric:
               good_setup_mask = MASK_GOODCONC;
            }

            break;
         case concept_once_removed:
            if (p->arg1 == 0)
               good_setup_mask = MASK_GOODRMVD;

            break;
         case concept_tandem:
         case concept_some_are_tandem:
         case concept_frac_tandem:
         case concept_some_are_frac_tandem:
            switch (p->arg4) {
            case tandem_key_tand:
               setup_mask = MASK_TAND;

               // We allow <anyone> tandem, and we allow twosome.  That's all.
               // We specifically exclude all the "tandem in a 1/4 tag" stuff.
               if (p->arg2 != CONCPROP__NEEDK_TWINQTAG &&
                   ((p->arg3 & 0xFF) == 0 || (p->arg3 & 0xFF) == 0x10))
                  good_setup_mask = MASK_GOODTAND;

               break;
            case tandem_key_cpls:
               setup_mask = MASK_CPLS;

               // We allow <anyone> tandem, and we allow twosome.  That's all.
               // We specifically exclude all the "tandem in a 1/4 tag" stuff.
               if (p->arg2 != CONCPROP__NEEDK_TWINQTAG &&
                   ((p->arg3 & 0xFF) == 0 || (p->arg3 & 0xFF) == 0x10))
                  good_setup_mask = MASK_GOODCPLS;

               break;
            case tandem_key_siam:
               setup_mask = MASK_SIAM; break;
            default:
               setup_mask = 0; break;    // Don't waste time on the others.
            }
            break;
         case concept_multiple_lines_tog_std:
         case concept_multiple_lines_tog:
            // Test for quadruple C/L/W working.
            if (p->arg4 == 4) setup_mask = MASK_2X4;
            break;
         case concept_multiple_diamonds:
            // Test for quadruple diamonds.
            if (p->arg4 == 4) setup_mask = MASK_QUAD_D;
            break;
         case concept_quad_diamonds_together:
         case concept_do_phantom_diamonds:
            setup_mask = MASK_QUAD_D;
            break;
         case concept_triple_twin_nomystic:
            // Test for divided C/L/W.
            if (p->arg3 == 10) setup_mask = MASK_2X4;
            break;
         case concept_do_phantom_2x4:
            setup_mask = MASK_2X4;          // Can actually improve on this.
            break;
         case concept_multiple_lines:
            // Check specifically for quadruple C/L/W.
            if (p->arg3 == 0 && p->arg4 == 4) setup_mask = MASK_2X4;
            break;
         case concept_assume_waves:
            // We never allow any "assume_waves" concept.  In the early days,
            // it was actually dangerous.  It isn't dangerous any more,
            // but it's a fairly stupid thing to generate in a search.
            setup_mask = 0;
            break;
         }

         // Now we can determine whether this concept is appropriate for this setup.

         if ((1 << (test_call_list_kind - ((int) call_list_empty))) & setup_mask)
            concept_sublists[call_list_any][concepts_in_this_setup++] = concept_index;

         // And we can determine whether this concept is really nice for this setup.

         if ((1 << (test_call_list_kind - ((int) call_list_empty))) & good_setup_mask)
            good_concept_sublists[call_list_any][good_concepts_in_this_setup++] = concept_index;
      }

      concept_sublist_sizes[test_call_list_kind] = concepts_in_this_setup;
      good_concept_sublist_sizes[test_call_list_kind] = good_concepts_in_this_setup;

      if (test_call_list_kind != (int) call_list_any) {
         if (concepts_in_this_setup != 0) {
            concept_sublists[test_call_list_kind] = new short int[concepts_in_this_setup];
            memcpy(concept_sublists[test_call_list_kind],
                   concept_sublists[call_list_any],
                   concepts_in_this_setup*sizeof(short int));
         }
         if (good_concepts_in_this_setup != 0) {
            good_concept_sublists[test_call_list_kind] = new short int[good_concepts_in_this_setup];
            memcpy(good_concept_sublists[test_call_list_kind],
                   good_concept_sublists[call_list_any],
                   good_concepts_in_this_setup*sizeof(short int));
         }
      }
   }

   // "Any" is not considered a good setup.
   good_concept_sublist_sizes[call_list_any] = 0;
}

void initialize_sdlib()
{
   configuration::initialize();
   initialize_tandem_tables();
   initialize_matrix_position_tables();
   restriction_tester::initialize_tables();
   select::initialize();
   conc_tables::initialize();
   merge_table::initialize();
   tglmap::initialize();
   initialize_commonspot_tables();
   map::initialize();
   full_expand::initialize_touch_tables();
   expand::initialize();
   initialize_concept_sublists();

   int i;

   for (i=0 ; i<warn__NUM_WARNINGS ; i++) {
      switch (warning_strings[i][0]) {
      case '%':
         suspect_destroyline_warnings.setbit((warning_index) i);
         no_search_warnings.setbit((warning_index) i); break;
      case '#':
         useless_phan_clw_warnings.setbit((warning_index) i);
         // FALL THROUGH!!!!
      case '*':
         // FELL THROUGH!!!!
         no_search_warnings.setbit((warning_index) i); break;
      case '+':
         conc_elong_warnings.setbit((warning_index) i); break;
      case '=':
         dyp_each_warnings.setbit((warning_index) i); break;
      }
   }
}

void finalize_sdlib()
{
   // In case someone runs a some kind of global memory leak detector, this releases memory.

   int i;

   delete [] base_calls;

   for (i=0 ; i<NUM_TAGGER_CLASSES ; i++)
      delete [] tagger_menu_list[i];

   delete [] selector_menu_list;
   delete [] direction_menu_list;
   delete [] circcer_menu_list;
}

static bool check_for_supercall(parse_block *parseptrcopy)
{
   concept_kind kk = parseptrcopy->concept->kind;
   heritflags zeroherit = 0ULL;

   if (kk <= marker_end_of_list) {
      // Only calls with "@0" or "@m" in their name may be supercalls.
      // But we don't allow calls that have both.
      // What would "finally [reverse the top] an anchor but [recoil]" mean?
      if (kk == marker_end_of_list &&
          !parseptrcopy->next &&
          ((parseptrcopy->call->the_defn.callflagsf &
            (CFLAGH__HAS_AT_ZERO|CFLAGH__HAS_AT_M)) == CFLAGH__HAS_AT_ZERO ||
           (parseptrcopy->call->the_defn.callflagsf &
            (CFLAGH__HAS_AT_ZERO|CFLAGH__HAS_AT_M)) == CFLAGH__HAS_AT_M)) {
         by_def_item innerdef;

         if (parseptrcopy->call->the_defn.callflagsf & CFLAGH__HAS_AT_ZERO) {
            innerdef.call_id = base_call_null;
            innerdef.modifiers1 = DFM1_CALL_MOD_MAND_ANYCALL;
         }
         else {
            innerdef.call_id = base_call_null_second;
            innerdef.modifiers1 = DFM1_CALL_MOD_MAND_SECONDARY;
         }

         innerdef.modifiersh = 0ULL;
         setup_command bar;
         bar.cmd_final_flags.clear_all_herit_and_final_bits();
         calldefn this_defn = base_calls[innerdef.call_id]->the_defn;
         setup_command foo2;
         get_real_subcall(parseptrcopy, &innerdef, &bar, &this_defn, false, zeroherit, &foo2);
      }

      if (parseptrcopy->concept->kind == concept_another_call_next_mod &&
          parseptrcopy->next &&
          (parseptrcopy->next->call == base_calls[base_call_null] ||
           parseptrcopy->next->call == base_calls[base_call_null_second] ||
           parseptrcopy->next->call == base_calls[base_call_circcer]) &&
          !parseptrcopy->next->next &&
          parseptrcopy->next->subsidiary_root) {
         return true;
      }
      else
         fail("A concept is required.");
   }

   return false;
}

static parse_block pbend(concept_mark_end_of_list);
static parse_block pbsuper(concept_marker_concept_supercall);

bool check_for_concept_group(
   parse_block *parseptrcopy,
   skipped_concept_info & retstuff,
   bool want_result_root) THROW_DECL
{
   concept_kind k;
   parse_block *kk;
   bool retval = false;
   parse_block *parseptr_skip = (parse_block *) 0;
   parse_block *next_parseptr = (parse_block *) 0;

   parse_block *first_arg = parseptrcopy;

   retstuff.m_skipped_concept = &pbend;
   retstuff.m_need_to_restrain = 0;

 try_again:

   if (!parseptrcopy) fail("A concept is required.");

   parseptr_skip = parseptrcopy->next;

   if (parseptrcopy->concept) {
      kk = parseptrcopy;

      if (check_for_supercall(parseptrcopy))
         kk = &pbsuper;
   }
   else
      kk = &pbend;

   const concept_descriptor *this_concept = kk->concept;
   k = this_concept->kind;

   if (!retval) {
      retstuff.m_skipped_concept = kk;

      if (k == concept_crazy || k == concept_frac_crazy || k == concept_dbl_frac_crazy)
         retstuff.m_need_to_restrain |= 1;
      else if (k == concept_n_times_const || k == concept_n_times)
         retstuff.m_need_to_restrain |= 2;
   }

   // We do these even if we aren't the first concept.

   if (k == concept_supercall ||
       k == concept_fractional ||
       (k == concept_snag_mystic && (this_concept->arg1 & CMD_MISC2__CENTRAL_SNAG)) ||
       (k == concept_so_and_so_only && (this_concept->arg1 == selective_key_snag_anyone)) ||
       (get_meta_key_props(this_concept) & MKP_RESTRAIN_1))
      retstuff.m_need_to_restrain |= 1;

   parse_block *skip_a_pair = (parse_block *) 0;
   final_and_herit_flags junk_concepts;

   junk_concepts.clear_all_herit_and_final_bits();
   parse_block *temp = process_final_concepts(parseptrcopy, false, &junk_concepts, false, false);

   if (temp && temp != parseptrcopy && temp->concept->kind == concept_concentric &&
       !junk_concepts.bool_test_heritbits(~(INHERITFLAG_GRAND|INHERITFLAG_SINGLE|INHERITFLAG_CROSS))) {
         skip_a_pair = temp;
   }
   else {
      junk_concepts.clear_all_herit_and_final_bits();
      temp = process_final_concepts(parseptr_skip, false, &junk_concepts, false, false);

      if (temp) {
         if (k == concept_c1_phantom) {
            // Look for combinations like "phantom tandem".
            // If skipping "phantom", maybe it's "phantom tandem", so we need to skip both.
            if ((temp->concept->kind == concept_tandem ||
                 temp->concept->kind == concept_frac_tandem) &&
                (!junk_concepts.test_for_any_herit_or_final_bit())) {
               skip_a_pair = temp;
            }
         }
         else if (k == concept_snag_mystic && (this_concept->arg1 & CMD_MISC2__CENTRAL_MYSTIC)) {
            // Look for combinations like "mystic triple boxes".
            if ((temp->concept->kind == concept_multiple_lines ||
                 temp->concept->kind == concept_multiple_diamonds ||
                 temp->concept->kind == concept_multiple_formations ||
                 temp->concept->kind == concept_multiple_boxes) &&
                temp->concept->arg4 == 3 &&
                (!junk_concepts.test_for_any_herit_or_final_bit())) {
               skip_a_pair = temp;
            }
         }
         else if (k == concept_parallelogram ||
                  (k == concept_distorted &&
                   parseptrcopy->concept->arg1 == disttest_offset &&
                   parseptrcopy->concept->arg3 == 0 &&
                   (parseptrcopy->concept->arg4 & ~0xF) == DISTORTKEY_DIST_CLW*16)) {
            // Look for combinations like "parallelogram split phantom C/L/W/B".
            // Similarly with "offset C/L/W split phantom C/L/W/B".
            if ((temp->concept->kind == concept_do_phantom_2x4 ||
                 temp->concept->kind == concept_do_phantom_boxes) &&
                temp->concept->arg3 == MPKIND__SPLIT &&
                (!junk_concepts.test_for_any_herit_or_final_bit())) {
               skip_a_pair = temp;
            }
         }
         else if (get_meta_key_props(this_concept) & MKP_RESTRAIN_2) {
            // Look for combinations like "random/initially/echo/nth-part-work <concept>".
            skip_a_pair = parseptr_skip;
         }
         else if (k == concept_so_and_so_only &&
                  ((selective_key) parseptrcopy->concept->arg1) == selective_key_work_concept) {
            // Look for combinations like "<anyone> work <concept>".
            skip_a_pair = parseptr_skip;
         }
         else if (k == concept_matrix) {
            skip_a_pair = parseptr_skip;
         }
      }
   }

   if (skip_a_pair) {
      parseptrcopy = skip_a_pair;
      next_parseptr = skip_a_pair;
      retval = true;
      goto try_again;
   }

   if (want_result_root) {
      if (k == concept_supercall) {
         retstuff.m_concept_with_root = parseptrcopy->next;
         retstuff.m_root_of_result_of_skip = &retstuff.m_concept_with_root->subsidiary_root;
      }
      else {
         if (retval)
            retstuff.m_concept_with_root = next_parseptr;
         else
            retstuff.m_concept_with_root = first_arg;

         if (concept_table[retstuff.m_concept_with_root->concept->kind].concept_prop & CONCPROP__SECOND_CALL)
            retstuff.m_root_of_result_of_skip = &retstuff.m_concept_with_root->subsidiary_root;
         else
            retstuff.m_root_of_result_of_skip = &retstuff.m_concept_with_root->next;
      }
   }

   return retval;
}



static void debug_print_parse_block(int level, const parse_block *p, char *tempstring_text, int &n)
{
   for ( ; p ; p = p->next) {
      if (level > 0) n += sprintf(tempstring_text+n, "(%d) ", level);
      if (p->concept) n += sprintf(tempstring_text+n, "  concept %s  ", p->concept->name);
      if (p->call) n += sprintf(tempstring_text+n, "  call %s  ", p->call->name);
      n += sprintf(tempstring_text+n, " %d %d %d %d %d \n",
             p->options.who.who[0],
             p->options.where,
             p->options.howmanynumbers,
             (int) p->options.number_fields,
             (int) p->options.tagger);
      if (p->subsidiary_root)
         debug_print_parse_block(level+1, p->subsidiary_root, tempstring_text, n);
   }
}

extern void crash_print(const char *filename, int linenum, int newtb, setup *ss) THROW_DECL
{
   char tempstring_text[10000];  // If this isn't big enough, I guess we lose.

   int n = sprintf(tempstring_text, "++++++++ CRASH - PLEASE REPORT THIS ++++++++\n");
   n += sprintf(tempstring_text+n, "%s %d 0x%08x\n", filename, linenum, newtb);

   if (ss && attr::slimit(ss) <= 24) {
      n += sprintf(tempstring_text+n, "%d\n", ss->kind);
      for (int q = 0 ; q <= attr::slimit(ss) ; q++) {
         n += sprintf(tempstring_text+n, "  0x%08x\n", ss->people[q].id1);
      }
   }

   int hsize = config_history_ptr+1;
   for (int jjj = 2 ; jjj <= hsize ; jjj++) {
      debug_print_parse_block(0, configuration::history[jjj].command_root, tempstring_text, n);
   }

   gg77->iob88.serious_error_print(tempstring_text);
   fail("Crash.");
}





extern void fail(const char s[]) THROW_DECL
{
   strncpy(error_message1, s, MAX_ERR_LENGTH);
   error_message1[MAX_ERR_LENGTH-1] = '\0';
   error_message2[0] = '\0';
   throw error_flag_type(error_flag_1_line);
}


extern void fail_no_retry(const char s[]) THROW_DECL
{
   strncpy(error_message1, s, MAX_ERR_LENGTH);
   error_message1[MAX_ERR_LENGTH-1] = '\0';
   error_message2[0] = '\0';
   throw error_flag_type(error_flag_no_retry);
}


extern void fail2(const char s1[], const char s2[]) THROW_DECL
{
   strncpy(error_message1, s1, MAX_ERR_LENGTH);
   error_message1[MAX_ERR_LENGTH-1] = '\0';
   strncpy(error_message2, s2, MAX_ERR_LENGTH);
   error_message2[MAX_ERR_LENGTH-1] = '\0';
   throw error_flag_type(error_flag_2_line);
}


extern void failp(uint32_t id1, const char s[]) THROW_DECL
{
   collision_person1 = id1;
   strncpy(error_message1, s, MAX_ERR_LENGTH);
   error_message1[MAX_ERR_LENGTH-1] = '\0';
   throw error_flag_type(error_flag_cant_execute);
}


extern void specialfail(const char s[]) THROW_DECL
{
   strncpy(error_message1, s, MAX_ERR_LENGTH);
   error_message1[MAX_ERR_LENGTH-1] = '\0';
   error_message2[0] = '\0';
   throw error_flag_type(error_flag_wrong_resolve_command);
}


void saved_error_info::collect(error_flag_type flag)
{
   save_error_flag = flag;
   strncpy((char *) save_error_message1, error_message1, MAX_ERR_LENGTH);
   strncpy((char *) save_error_message2, error_message2, MAX_ERR_LENGTH);
   save_collision_person1 = collision_person1;
   save_collision_person2 = collision_person2;
}

void saved_error_info::throw_saved_error() THROW_DECL
{
   strncpy(error_message1, (char *) save_error_message1, MAX_ERR_LENGTH);
   strncpy(error_message2, (char *) save_error_message2, MAX_ERR_LENGTH);
   collision_person1 = save_collision_person1;
   collision_person2 = save_collision_person2;
   throw save_error_flag;
}


extern void warn(warning_index w)
{
   if (w == warn__none || w == warn__really_no_collision) return;

   // If this is an "each 1x4" type of warning, and we already have
   // such a warning, don't enter the new one.  The first one takes precedence.

   if (dyp_each_warnings.testbit(w) && configuration::test_multiple_warnings(dyp_each_warnings))
         return;

   configuration::set_one_warning(w);
}


extern callarray *assoc(
   begin_kind key,
   setup *ss,
   callarray *spec,
   bool *specialpass /* = (bool *) 0 */) THROW_DECL
{
   for (callarray *p = spec ; p ; p = p->next) {
      uint32_t k, t, u, w, mask;
      assumption_thing tt;
      int plaini;
      bool booljunk;

      // First, we demand that the starting setup be correct.
      // Also, if a qualifier number was specified, it must match.

      if (p->get_start_setup() != key) continue;

      tt.assump_negate = 0;

      // During initialization, we will be called with a null pointer for ss.
      // We need to be careful, and err on the side of acceptance.

      if (!ss) return p;

      // The bits of the "qualifierstuff" field have the following meaning
      //          (see definitions in database.h):
      // 10000  left/out only (put 2 into assump_both)
      //  8000  right/in only (put 1 into assump_both)
      //  4000  must be live
      //  2000  must be T-boned
      //  1000  must not be T-boned (both together means explicit assumption)
      //  0F00  if these 4 bits are nonzero, they must match the number plus 1
      //  00FF  the qualifier itself (we allow 127 qualifiers)

      // The handling of clauses with numeric tests (e.g. "qualifier num 2") is made
      // complicated by the existence of special calls that can *optionally* be given a
      // number.
      //
      // These special calls have the "optional_special_number" top-level flag.  Their
      // definition has a mixture of clauses that take a number and clauses that do not.
      // On any given usage of the call, only one or the other type of clause is to be
      // used; the other type is ignored.
      //
      //    If the CMD_MISC3__SPECIAL_NUMBER_INVOKE is on, only numeric clauses are
      //    considered; non-numeric clauses are skipped.  The call is being invoked with
      //    the special mechanism, such as "3/4 swing the fractions".  The required
      //    number will be present, in current_options.number_fields, and needs to be
      //    checked.
      //
      //    If CMD_MISC3__SPECIAL_NUMBER_INVOKE is off, only non-numeric clauses are
      //    considered, even if a number is present.  (Current_options.howmanynumbers
      //    might be nonzero for unrelated reasons; that number is not to be used.)  The
      //    call is being invoked without the special mechanism, such as plain "swing
      //    the fractions".
      //
      // Other calls (the vast majority) consider all clauses.  Such calls typically
      // either have all numeric clauses (and have something like "@b" in their name to
      // get the required number), or all non-numeric clauses.  But they are not
      // required to conform to that.
      //
      //    If a clause does not take a number, just proceed with it.
      //
      //    If a clause takes a number, demand that it be present, and check it.

      if ((p->qualifierstuff & QUALBIT__NUM_MASK) != 0) {
         // Qualifier wants a number.

         if (ss->cmd.callspec && (ss->cmd.callspec->the_defn.callflags1 & CFLAG1_NUMBER_MASK) == CFLAG1_NUMBER_MASK) {
            // It's one of the special calls that optionally takes a number.  Proceed
            // and check the number only if CMD_MISC3__SPECIAL_NUMBER_INVOKE is on.  If
            // CMD_MISC3__SPECIAL_NUMBER_INVOKE is off, dismiss it immediately.
            if (!(ss->cmd.cmd_misc3_flags & CMD_MISC3__SPECIAL_NUMBER_INVOKE) ||
                current_options.howmanynumbers != 1)
               continue;
         }
         else {
            // Qualifier wants a number, call is not special.  Demand that number be
            // present, and check same.
            number_used = true;   // Turn this on whether the number matched or not.
            if (current_options.howmanynumbers == 0)
               continue;
         }

         // Either way, demand that it be the correct number.
         if (((unsigned int) (p->qualifierstuff & QUALBIT__NUM_MASK) / QUALBIT__NUM_BIT) !=
             (current_options.number_fields & NUMBER_FIELD_MASK)+1)
            continue;
      }
      else {
         // Qualifier does not want a number.  Accept the clause if the call is not
         // special, or if it is special but CMD_MISC3__SPECIAL_NUMBER_INVOKE is off.
         // If CMD_MISC3__SPECIAL_NUMBER_INVOKE is on, only numeric clauses, which this
         // isn't, are accepted,
         if ((ss->cmd.cmd_misc3_flags & CMD_MISC3__SPECIAL_NUMBER_INVOKE) &&
             ss->cmd.callspec && (ss->cmd.callspec->the_defn.callflags1 & CFLAG1_NUMBER_MASK) == CFLAG1_NUMBER_MASK)
            continue;
      }

      bool require_explicit = false;

      if ((p->qualifierstuff & (QUALBIT__TBONE|QUALBIT__NTBONE)) != 0) {
         if ((p->qualifierstuff & (QUALBIT__TBONE|QUALBIT__NTBONE)) == (QUALBIT__TBONE|QUALBIT__NTBONE)) {
            require_explicit = true;
         }
         else {
            if ((or_all_people(ss) & 011) == 011) {
               // They are T-boned.  The "QUALBIT__NTBONE" bit says to reject.
               if ((p->qualifierstuff & QUALBIT__NTBONE) != 0) continue;
            }
            else {
               // They are not T-boned.  The "QUALBIT__TBONE" bit says to reject.
               if ((p->qualifierstuff & QUALBIT__TBONE) != 0) continue;
            }
         }
      }

      call_restriction ssA = ss->cmd.cmd_assume.assumption;
      uint32_t ssB = ss->cmd.cmd_assume.assump_both;
      setup_kind ssK = ss->kind;

      call_restriction this_qualifier = (call_restriction) (p->qualifierstuff & QUALBIT__QUAL_CODE);

      if (this_qualifier == cr_dmd_ctrs_mwv_no_mirror) {
         if (ss->cmd.cmd_misc_flags & CMD_MISC__DID_LEFT_MIRROR) goto bad;
         this_qualifier = cr_dmd_ctrs_mwv;
      }

      if (this_qualifier == cr_none) {
         if ((p->qualifierstuff / QUALBIT__LIVE) & 1) {   // All live people were demanded.
            for (plaini=0; plaini<=attr::slimit(ss); plaini++) {
               if ((ss->people[plaini].id1) == 0) goto bad;
            }
         }

         goto good;
      }

      // Note that we have to examine setups larger than the setup the
      // qualifier is officially defined for.  If a qualifier were defined
      // as being legal only on 1x4's (so that, in the database, we only had
      // specifications of the sort "setup 1x4 1x4 qualifier wave_only") we could
      // still find ourselves here with ss->kind equal to s2x4.  Why?  Because
      // the setup could be a 2x4 and the splitter could be trying to decide
      // whether to split the setup into parallel 1x4's.  This happens when
      // trying to figure out whether to split a 2x4 into 1x4's or 2x2's for
      // the call "recycle".

      k = 0;   // Many tests will find these values useful.
      mask = 0;
      uint32_t livemask;
      tt.assumption = this_qualifier;
      tt.assump_col = 0;
      tt.assump_cast = 0;
      tt.assump_live = (p->qualifierstuff / QUALBIT__LIVE) & 1;
      tt.assump_both = (p->qualifierstuff / QUALBIT__RIGHT) & 3;

      u = or_all_people(ss);

      switch (this_qualifier) {
      case cr_wave_only:
         goto do_wave_stuff;
      case cr_magic_only:
         switch (ssA) {
         case cr_1fl_only:
         case cr_2fl_only:
         case cr_wave_only:
         case cr_li_lo:
            goto bad;
         }

         switch (ssK) {
         case s2x4: case s1x8:
            if (key == b_1x4) tt.assumption = cr_miniwaves;
            break;
         }

         goto fix_col_line_stuff;

      case cr_quarterbox:
         if (ssA == cr_threequarterbox)
            goto bad;
         goto fix_col_line_stuff;
      case cr_threequarterbox:
         if (ssA == cr_quarterbox)
            goto bad;
         goto fix_col_line_stuff;
      case cr_li_lo:
         switch (ssA) {
         case cr_magic_only: case cr_wave_only:
            goto bad;
         }

         switch (ssK) {
         case s1x2: case s1x4: case s2x2: case s2x3: case s2x4: case s_alamo:
            goto fix_col_line_stuff;
         case s4x4:
            // Check for special case of 4x4 occupied on "O" spots--cr_li_lo has a meaning here.
            if (little_endian_live_mask(ss) == 0x6666)
               goto fix_col_line_stuff;
         // FALL THROUGH!!!!
         default:
            goto good;           // We don't understand the setup -- we'd better accept it.
         }
      case cr_opposite_sex:
         switch (ssK) {
         case s1x2:
            goto fix_col_line_stuff;
         default:
            if (setup_attrs[ssK].keytab[0] != key && setup_attrs[ssK].keytab[1] != key) {
               if (specialpass) *specialpass = true;
            }
            goto good;           // Accept this all the way down to 1x2.
         }
      case cr_indep_in_out:
         switch (ssK) {
         case s2x2: case s2x4:
            goto fix_col_line_stuff;
         default:
            goto good;           // We don't understand the setup -- we'd better accept it.
         }
      case cr_ctrs_in_out:
         switch (ssK) {
         case s2x4:
            if (!((ss->people[1].id1 | ss->people[2].id1 |
                   ss->people[5].id1 | ss->people[6].id1)&010))
               tt.assump_col = 1;
            goto check_tt;
         default:
            goto good;           // We don't understand the setup -- we'd better accept it.
         }

      case cr_all_facing_same:
         switch (ssA) {
         case cr_wave_only:
         case cr_magic_only:
            goto bad;
         }

         // If we are not looking at the whole setup (that is, we are deciding
         // whether to split the setup into smaller ones), let it pass.

         if (setup_attrs[ssK].keytab[0] != key &&
             setup_attrs[ssK].keytab[1] != key)
            goto good;

         goto fix_col_line_stuff;
      case cr_1fl_only:        // 1x3/1x4/1x6/1x8 - a 1FL, that is, all 3/4/6/8 facing same;
                               // 2x3/2x4 - individual 1FL's
         switch (ssA) {
         case cr_1fl_only:
            goto good;
         case cr_wave_only:
         case cr_magic_only:
            goto bad;
         }

         goto check_tt;
      case cr_2fl_only:
         goto do_2fl_stuff;

      case cr_ctr_2fl_only:
         switch (ssA) {
         case cr_li_lo: case cr_1fl_only: case cr_wave_only:
         case cr_miniwaves: case cr_magic_only: goto bad;
         }

         goto check_tt;
      case cr_3x3_2fl_only:
         switch (ssA) {
         case cr_1fl_only: case cr_wave_only:
         case cr_miniwaves: case cr_magic_only: goto bad;
         }

         tt.assumption = cr_3x3_2fl_only;
         goto fix_col_line_stuff;
      case cr_4x4_2fl_only:
         switch (ssA) {
         case cr_1fl_only: case cr_wave_only:
         case cr_miniwaves: case cr_magic_only: goto bad;
         }

         tt.assumption = cr_4x4_2fl_only;
         goto fix_col_line_stuff;
      case cr_couples_only:
         switch (ssA) {
         case cr_1fl_only:
         case cr_2fl_only:
            goto good;
         case cr_wave_only:
         case cr_miniwaves:
         case cr_magic_only:
            goto bad;
         }

         tt.assumption = cr_couples_only;  /* Don't really need this, unless implement
                                              "not_couples" (like "cr_not_miniwaves")
                                              in the future. */

         switch (ssK) {
         case s1x2: case s1x4: case s1x6: case s3x4: case s1x8: case s1x16: case s2x4:
         case sdmd: case s_trngl: case s_qtag: case s_ptpd: case s_bone:
         case s2x2: case s_alamo:
            goto fix_col_line_stuff;
         default:
            goto good;           // We don't understand the setup -- we'd better accept it.
         }
      case cr_3x3couples_only:      // 1x3/1x6/2x3/2x6/1x12 - each group of 3 people
                                    // are facing the same way
         switch (ssA) {
         case cr_1fl_only:
            goto good;
         case cr_wave_only:
         case cr_magic_only:
            goto bad;
         }

         goto check_tt;
      case cr_4x4couples_only:      // 1x4/1x8/2x4/2x8/1x16 - each group of 4 people
                                    // are facing the same way
         switch (ssA) {
         case cr_1fl_only:
            goto good;
         case cr_wave_only:
         case cr_magic_only:
            goto bad;
         }

         goto check_tt;
      case cr_not_miniwaves:
         tt.assump_negate = 1;
         /* **** FALL THROUGH!!!! */
      case cr_miniwaves:                    // miniwaves everywhere
         /* **** FELL THROUGH!!!!!! */
         switch (ssA) {
         case cr_li_lo:
         case cr_1fl_only:
         case cr_2fl_only:
            goto bad;
         case cr_wave_only:
         case cr_miniwaves:
         case cr_magic_only:
            goto good;
         }

         tt.assumption = cr_miniwaves;

         switch (ssK) {
         case s1x2: case s1x4: case s1x6: case s1x8: case s1x16: case s2x4:
         case sdmd: case s_trngl: case s_qtag: case s_ptpd: case s_bone:
         case s2x2: case s_thar: case s_alamo:
            goto fix_col_line_stuff;
         case s4x4:
            // Check for special case of 4x4 occupied on "O" spots--cr_miniwaves has a meaning here.
            if (little_endian_live_mask(ss) == 0x6666)
               goto fix_col_line_stuff;
         // FALL THROUGH!!!!
         default:
            goto good;           // We don't understand the setup -- we'd better accept it.
         }
      case cr_tgl_tandbase:
         tt.assumption = cr_tgl_tandbase;
         switch (ssK) {
         case s_trngl:
            goto fix_col_line_stuff;
         default:
            goto good;
         }
      case cr_ctrwv_end2fl:
         /* Note that this qualifier is kind of strict.  We won't permit the call "with
            confidence" do be done unless everyone can trivially determine which
            part to do. */
         if (ssK == s_crosswave) {
            if (((ss->people[0].id1 ^ ss->people[1].id1) & d_mask) == 0 &&
                ((ss->people[4].id1 ^ ss->people[5].id1) & d_mask) == 0 &&
                ((ss->people[2].id1 | ss->people[3].id1) == 0 ||
                 ((ss->people[2].id1 ^ ss->people[3].id1) & d_mask) == 2) &&
                ((ss->people[6].id1 | ss->people[7].id1) == 0 ||
                 ((ss->people[6].id1 ^ ss->people[7].id1) & d_mask) == 2))
               goto good;
            goto bad;
         }
         else if (ssK == s1x6) {
            if (((ss->people[0].id1 ^ ss->people[1].id1) & d_mask) == 0 &&
                ((ss->people[3].id1 ^ ss->people[4].id1) & d_mask) == 0 &&
                ((ss->people[1].id1 ^ ss->people[2].id1) & d_mask) == 2 &&
                ((ss->people[4].id1 ^ ss->people[5].id1) & d_mask) == 2)
               goto good;
            goto bad;
         }
         else if (ssK == s1x8) {
            if (((ss->people[0].id1 ^ ss->people[1].id1) & d_mask) == 0 &&
                ((ss->people[4].id1 ^ ss->people[5].id1) & d_mask) == 0 &&
                ((ss->people[2].id1 ^ ss->people[3].id1) & d_mask) == 2 &&
                ((ss->people[6].id1 ^ ss->people[7].id1) & d_mask) == 2)
               goto good;
            goto bad;
         }
         else
            goto good;        /* We don't understand the setup -- we'd better accept it. */
      case cr_ctr2fl_endwv:
         /* Note that this qualifier is kind of strict.  We won't permit the call "with
               confidence" do be done unless everyone can trivially determine which
               part to do. */
         if (ssK == s_crosswave) {
            if (((ss->people[2].id1 ^ ss->people[3].id1) & d_mask) == 0 &&
                ((ss->people[6].id1 ^ ss->people[7].id1) & d_mask) == 0 &&
                ((ss->people[0].id1 | ss->people[1].id1) == 0 ||
                 ((ss->people[0].id1 ^ ss->people[1].id1) & d_mask) == 2) &&
                ((ss->people[4].id1 | ss->people[5].id1) == 0 ||
                 ((ss->people[4].id1 ^ ss->people[5].id1) & d_mask) == 2))
               goto good;
            goto bad;
         }
         else if (ssK == s1x6) {
            if (((ss->people[0].id1 ^ ss->people[1].id1) & d_mask) == 2 &&
                ((ss->people[3].id1 ^ ss->people[4].id1) & d_mask) == 2 &&
                ((ss->people[1].id1 ^ ss->people[2].id1) & d_mask) == 0 &&
                ((ss->people[4].id1 ^ ss->people[5].id1) & d_mask) == 0)
               goto good;
            goto bad;
         }
         else if (ssK == s1x8) {
            if (((ss->people[0].id1 ^ ss->people[1].id1) & d_mask) == 2 &&
                ((ss->people[4].id1 ^ ss->people[5].id1) & d_mask) == 2 &&
                ((ss->people[2].id1 ^ ss->people[3].id1) & d_mask) == 0 &&
                ((ss->people[6].id1 ^ ss->people[7].id1) & d_mask) == 0)
               goto good;
            goto bad;
         }
         else
            goto good;         /* We don't understand the setup -- we'd better accept it. */
      case cr_siamese_in_quad:
      case cr_not_tboned_in_quad:
         goto check_tt;
      case cr_true_Z_cw:
         if (ssK == s2x3) {
            if ((ss->people[2].id1 | ss->people[5].id1) && !ss->people[0].id1 && !ss->people[3].id1)
               goto good;
            else
               goto bad;
         }
         else if (ssK == s_qtag) {
            if ((ss->people[0].id1 | ss->people[4].id1) && !ss->people[5].id1 && !ss->people[1].id1)
               goto good;
            else
               goto bad;
         }

         // We don't understand the setup -- we'd better accept it.
         goto good;
      case cr_true_Z_ccw:
         if (ssK == s2x3) {
            if ((ss->people[0].id1 | ss->people[3].id1) && !ss->people[2].id1 && !ss->people[5].id1)
               goto good;
            else
               goto bad;
         }
         else if (ssK == s_qtag) {
            if ((ss->people[5].id1 | ss->people[1].id1) && !ss->people[0].id1 && !ss->people[4].id1)
               goto good;
            else
               goto bad;
         }

         // We don't understand the setup -- we'd better accept it.
         goto good;
      case cr_true_PG_cw:
         k ^= 01717U ^ 07474U;
         // **** FALL THROUGH!!!!
      case cr_true_PG_ccw:
         // **** FELL THROUGH!!!!!!
         k ^= ~01717U;

         if (ssK == s2x6) {
            // In this case, we actually check the shear direction of the parallelogram.

            mask = little_endian_live_mask(ss);
            if ((mask & k) == 0 && (mask & (k^01717U^07474U)) != 0) goto good;
            goto bad;
         }
         else if (ssK == s4x6 || ssK == s2x12)
            goto good;         // We don't understand the setup, and it's one from which we
                               // could subdivide into 2x6's -- we'd better accept it.

         goto bad;   // If it's a 2x4, for example, it can't be a parallelogram.
      case cr_lateral_cols_empty:
         t = or_all_people(ss);
         mask = little_endian_live_mask(ss);

         if (ssK == s3x4 && (t & 1) == 0 &&
             ((mask & 04646) == 0 ||
              (mask & 04532) == 0 || (mask & 03245) == 0 ||
              (mask & 02525) == 0 || (mask & 03232) == 0 ||
              (mask & 04651) == 0 || (mask & 05146) == 0))
            goto good;
         else if (ssK == s3x6 && (t & 1) == 0 &&
                  ((mask & 0222222) == 0))
            goto good;
         else if (ssK == s4x4 && (t & 1) == 0 &&
                  ((mask & 0xE8E8) == 0 ||
                   (mask & 0xA3A3) == 0 || (mask & 0x5C5C) == 0 ||
                   (mask & 0xA857) == 0 || (mask & 0x57A8) == 0))
            goto good;
         else if (ssK == s4x4 && (t & 010) == 0 &&
                  ((mask & 0x8E8E) == 0 ||
                   (mask & 0x3A3A) == 0 || (mask & 0xC5C5) == 0 ||
                   (mask & 0x857A) == 0 || (mask & 0x7A85) == 0))
            goto good;
         else if (ssK == s4x6 && (t & 010) == 0) goto good;
         else if (ssK == s3x8 && (t & 001) == 0) goto good;
         goto bad;
      case cr_dmd_same_pt:                   // dmd or pdmd - centers would circulate to same point
         if (((ss->people[1].id1 & (BIT_PERSON | 011U)) == d_east) &&         // faces either east or west
             (!((ss->people[3].id1 ^ ss->people[1].id1) & d_mask)))            // and both face same way
            goto good;
         goto bad;
      case cr_nice_diamonds:
      case cr_dmd_facing:
      case cr_conc_iosame:
      case cr_conc_iodiff:
      case cr_nice_wv_triangles:
      case cr_spd_base_mwv:
      case cr_dmd_pts_mwv:
      case cr_dmd_pts_1f:
         goto check_tt;
      case cr_dmd_ctrs_mwv:
         if ((tt.assump_both+1) & 2) {
            switch (ssA) {
            case cr_dmd_ctrs_mwv:
               if (ssB == tt.assump_both)
                  goto good;
               break;
            case cr_jright: case cr_ijright:
               if (ssB == (tt.assump_both ^ 3))
                  goto bad;
               else if (ssB == tt.assump_both)
                  goto good;
               break;
            case cr_jleft: case cr_ijleft:
               if (ssB == (tt.assump_both ^ 3))
                  goto good;
               else if (ssB == tt.assump_both)
                  goto bad;
               break;
            }
         }

         goto check_tt;
      case cr_qtag_like:
      case cr_qtag_like_anisotropic:
      case cr_qline_like_l:
      case cr_qline_like_r:
         switch (ssA) {
         case cr_diamond_like: case cr_pu_qtag_like:
            goto bad;
         case cr_jleft: case cr_jright: case cr_ijleft: case cr_ijright:
            // If tt.B is 1 or 2, demand ss.B be the other.
            if (((tt.assump_both-1) & ~1) == 0) {
               if (ssB == (tt.assump_both ^ 3))
                  goto good;
               else
                  goto bad;
            }
         }
         goto check_tt;
      case cr_diamond_like:
         switch (ssA) {
         case cr_qtag_like: case cr_pu_qtag_like:
            goto bad;
         }
         goto check_tt;
      case cr_pu_qtag_like:
         switch (ssA) {
         case cr_qtag_like: case cr_diamond_like:
            goto bad;
         }
         goto check_tt;
      case cr_dmd_intlk:
         switch (ssA) {
         case cr_ijright: case cr_ijleft: case cr_real_1_4_line: case cr_real_3_4_line:
            goto good;
         case cr_jright: case cr_jleft: case cr_real_1_4_tag: case cr_real_3_4_tag:
            goto bad;
         }

         goto check_tt;
      case cr_dmd_not_intlk:
         switch (ssA) {
         case cr_ijright: case cr_ijleft: case cr_real_1_4_line: case cr_real_3_4_line:
            goto bad;
         case cr_jright: case cr_jleft: case cr_real_1_4_tag: case cr_real_3_4_tag:
            goto good;
         }
         goto check_tt;
      case cr_not_split_dixie:
         tt.assump_negate = 1;
         /* **** FALL THROUGH!!!! */
      case cr_split_dixie:
         /* **** FELL THROUGH!!!!!! */
         if (ss->cmd.cmd_final_flags.test_finalbit(FINAL__SPLIT_DIXIE_APPROVED)) goto good;
         goto bad;
      case cr_said_dmd:
         if (ss->cmd.cmd_misc3_flags & CMD_MISC3__SAID_DIAMOND) goto good;
         goto bad;
      case cr_said_gal:
         if (ss->cmd.cmd_misc3_flags & CMD_MISC3__SAID_GALAXY) goto good;
         goto bad;
      case cr_didnt_say_tgl:
         tt.assump_negate = 1;
         /* **** FALL THROUGH!!!! */
      case cr_said_tgl:
         /* **** FELL THROUGH!!!!!! */
         if (ss->cmd.cmd_misc3_flags & CMD_MISC3__SAID_TRIANGLE) goto good;
         goto bad;
      case cr_didnt_say_matrix:
         if (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) goto bad;
         goto good;
      case cr_occupied_as_h:
         if (ssK != s3x4 ||
             (ss->people[1].id1 | ss->people[2].id1 |
              ss->people[7].id1 | ss->people[8].id1)) goto bad;
         goto good;
      case cr_occupied_as_stars:
         if (ssK != s_c1phan ||
             ((ss->people[0].id1 | ss->people[1].id1 |
               ss->people[2].id1 | ss->people[3].id1 |
               ss->people[8].id1 | ss->people[9].id1 |
               ss->people[10].id1 | ss->people[11].id1) &&
              (ss->people[4].id1 | ss->people[5].id1 |
               ss->people[6].id1 | ss->people[7].id1 |
               ss->people[12].id1 | ss->people[13].id1 |
               ss->people[14].id1 | ss->people[15].id1)))
            goto bad;
         goto good;
      case cr_occupied_as_clumps:
         if (ssK != s4x4 ||
             ((ss->people[0].id1 | ss->people[1].id1 |
               ss->people[14].id1 | ss->people[3].id1 |
               ss->people[8].id1 | ss->people[9].id1 |
               ss->people[6].id1 | ss->people[11].id1) &&
              (ss->people[4].id1 | ss->people[5].id1 |
               ss->people[2].id1 | ss->people[7].id1 |
               ss->people[12].id1 | ss->people[13].id1 |
               ss->people[10].id1 | ss->people[15].id1)))
            goto bad;
         goto good;
      case cr_occupied_as_blocks:
         livemask = little_endian_live_mask(ss);
         if (ssK == s4x4 &&
             ((livemask & 0x2D2D) == 0 || (livemask & 0xD2D2) == 0))
            goto good;
         goto bad;
      case cr_occupied_as_traps:
         livemask = little_endian_live_mask(ss);
         if (ssK == s4x4 &&
             ((livemask & 0x6666) == 0 || (livemask & 0x9999) == 0 ||
              (livemask & 0x6D92) == 0 || (livemask & 0x926D) == 0 ||
              (livemask & 0xD926) == 0 || (livemask & 0x26D9) == 0))
            goto good;
         else if (ssK == s2x8 &&
             ((livemask & 0x6969) == 0 || (livemask & 0x9696) == 0 ||
              (livemask & 0x6699) == 0 || (livemask & 0x9966) == 0))
            goto good;
         else if (ssK == s2x4 &&
             ((livemask & 0x69) == 0 || (livemask & 0x96) == 0))
            goto good;
         goto bad;
      case cr_occupied_as_qtag:
         if (ssK != s3x4 ||
             (ss->people[0].id1 | ss->people[3].id1 |
              ss->people[6].id1 | ss->people[9].id1)) goto bad;
         goto good;
      case cr_occupied_as_3x1tgl:
         mask = little_endian_live_mask(ss);

         switch (ssK) {
         case s_qtag:
            goto good;
         case s3x4:
            if (mask == 07171 || mask == 00170 || mask == 07001 ||
                mask == 06666 || mask == 06402 || mask == 00264 ||
                mask == 07265 || mask == 06572) goto good;
            else goto bad;
         case s2x6:
            if (mask == 07272 || mask == 07002 || mask == 00270 ||
                mask == 02727 || mask == 02007 || mask == 00720 ||
                mask == 07722 || mask == 02277) goto good;
            else goto bad;
         case s2x3:
            if (mask == 072 || mask == 027) goto good;
            else goto bad;
         default:
            goto good;    // Must be evaluating a division of a 4x5 or whatever.
         }
      case cr_occupied_as_o:
         if (ssK != s4x4 || ((little_endian_live_mask(ss) & 0x9999) != 0))
            goto bad;
         goto good;
      case cr_reg_tbone:
         if (ssK == s2x4 && key == b_2x2) tt.assump_col = 1;
         /* **** FALL THROUGH!!!! */
      case cr_qtag_mwv:
      case cr_qtag_mag_mwv:
      case cr_dmd_ctrs_1f:
      case cr_gen_qbox:
      case cr_inroller_is_cw:
      case cr_inroller_is_ccw:
      case cr_outroller_is_cw:
      case cr_outroller_is_ccw:
      case cr_judge_is_cw:
      case cr_judge_is_ccw:
      case cr_socker_is_cw:
      case cr_socker_is_ccw:
      case cr_extend_inroutl:
      case cr_extend_inloutr:
         /* **** FELL THROUGH!!!!!! */
         goto check_tt;
      case cr_ctr_pts_rh:
      case cr_ctr_pts_lh:
         {
            uint32_t z = 1;

            switch (ssA) {
            case cr_jright: case cr_ijright:
               z = 2;
            case cr_jleft: case cr_ijleft:
               if (z & ssB) {
                  if (this_qualifier == cr_ctr_pts_rh) goto bad;
                  else goto good;
               }
               else if ((z ^ 3) & ssB) {
                  if (this_qualifier == cr_ctr_pts_rh) goto good;
                  else goto bad;
               }
            }

            uint32_t t1;
            uint32_t t2;
            uint32_t ndir = d_north;
            uint32_t sdir = d_south;
            uint32_t tb;
            bool b1 = true;
            bool b2 = true;

            switch (ssK) {
            case s_qtag:
            case s_hrglass:
            case s_dhrglass:
            case s_bone:
               t1 = 6; t2 = 2; break;
            case s_2x1dmd:
               t1 = 0; t2 = 3; break;
            case s_bone6:
               t1 = 5; t2 = 2; break;
            case s_short6:
               // This one has the people facing sideways.
               ndir = d_east; sdir = d_west;
               t1 = 1; t2 = 4; break;
            case s_star:
               // This has to look at headliner-sideliner-ness.  It
               // will screw up if people are T-boned.
               tb = (ss->people[0].id1 | ss->people[1].id1 |
                  ss->people[2].id1 | ss->people[3].id1) & 011;
               if (tb == 1) {
                  ndir = d_east; sdir = d_west;
                  t1 = 1; t2 = 3; break;
               }
               else if (tb == 010) {
                  t1 = 0; t2 = 2; break;
               }
               else goto bad;
            default:
               goto bad;
            }

            t1 = ss->people[t1].id1;
            t2 = ss->people[t2].id1;

            if (t1 && (t1 & d_mask)!=ndir) b1 = false;
            if (t2 && (t2 & d_mask)!=sdir) b1 = false;
            if (t1 && (t1 & d_mask)!=sdir) b2 = false;
            if (t2 && (t2 & d_mask)!=ndir) b2 = false;

            if (b1 == b2) goto bad;

            if (this_qualifier ==
                (b1 ? cr_ctr_pts_rh : cr_ctr_pts_lh))
               goto good;
         }
         goto bad;
      case cr_line_ends_looking_out:
         if (ssK != s2x4) goto bad;
         if ((t = ss->people[0].id1) && (t & d_mask) != d_north) goto bad;
         if ((t = ss->people[3].id1) && (t & d_mask) != d_north) goto bad;
         if ((t = ss->people[4].id1) && (t & d_mask) != d_south) goto bad;
         if ((t = ss->people[7].id1) && (t & d_mask) != d_south) goto bad;
         goto good;
      case cr_col_ends_lookin_in:
         if (ssK != s2x4) goto bad;
         if ((t = ss->people[0].id1) && (t & d_mask) != d_east) goto bad;
         if ((t = ss->people[3].id1) && (t & d_mask) != d_west) goto bad;
         if ((t = ss->people[4].id1) && (t & d_mask) != d_west) goto bad;
         if ((t = ss->people[7].id1) && (t & d_mask) != d_east) goto bad;
         goto good;
      case cr_ripple_both_centers:
         k ^= (0xAAA ^ 0xA82);
         /* **** FALL THROUGH!!!! */
      case cr_ripple_any_centers:
         /* **** FELL THROUGH!!!!!! */
         k ^= (0xA82 ^ 0x144);
         /* **** FALL THROUGH!!!! */
      case cr_ripple_one_end:
         /* **** FELL THROUGH!!!!!! */
         k ^= (0x144 ^ 0x555);
         /* **** FALL THROUGH!!!! */
      case cr_ripple_both_ends:
      case cr_ripple_both_ends_1x4_only:
         /* **** FELL THROUGH!!!!!! */
         k ^= 0x11090555;

         // Make a little-endian mask of the selected people.
         mask = 0;
         for (plaini=0, w=1; plaini<=attr::slimit(ss); plaini++, w<<=1) {
            if (selectp(ss, plaini)) mask |= w;
         }

         if (ssK == s1x4) {
            if (mask == (k & 0xF) || mask == ((k>>4) & 0xF) || mask == ((k>>8) & 0xF))
               goto good;
            goto bad;
         }
         else if (ssK == s2x4 && this_qualifier == cr_ripple_one_end) {
            // Only "cr_ripple_one_end" is supported in a 2x4.
            // In fact, we don't even look at k.
            // 32 bits in k aren't enough to do everything we want
            // in a completely general way.
            if (mask == 0x11 || mask == 0x88) goto good;
            goto bad;
         }
         else if (ssK == s1x6 && this_qualifier == cr_ripple_both_ends) {
            if (mask == ((k>>16) & 0x3F)) goto good;
            goto bad;
         }
         else if (ssK == s1x6 && this_qualifier == cr_ripple_one_end) {
            if (mask == 010 || mask == 001) goto good;
            goto bad;
         }
         else if (ssK == s1x8 && this_qualifier == cr_ripple_both_ends) {
            if (mask == ((k>>24) & 0xFF)) goto good;
            goto bad;
         }

         // If the actual setup is a 2x4 and we are testing a 1x4,
         // we have to accept it.  After it gets split to 1x4's,
         // we will test it more thoroughly.
         if (ssK == s2x4)
            goto good;
      case cr_people_1_and_5_real:
         if (ss->people[1].id1 & ss->people[5].id1) goto good;
         goto bad;

      case cr_ptp_unwrap_sel:
      case cr_nor_unwrap_sel:
      case cr_ends_sel:
      case cr_ctrs_sel:
      case cr_all_sel:
      case cr_not_all_sel:
      case cr_some_sel:
      case cr_none_sel:

         // If we are not looking at the whole setup (that is, we are deciding
         // whether to split the setup into smaller ones), let it pass.

         if (setup_attrs[ssK].keytab[0] != key && setup_attrs[ssK].keytab[1] != key) {
            if (specialpass) *specialpass = true;
            goto good;
         }

         goto check_tt;

      case cr_levelplus:
      case cr_levela1:
      case cr_levela2:
      case cr_levelc1:
      case cr_levelc2:
      case cr_levelc3:
      case cr_levelc4:
         goto check_tt;

      case cr_ends_didnt_move:
         if (ss->kind == s1x4)
            k = ss->people[0].id1 & ss->people[2].id1;
         else if (ss->kind == s2x4)
            k = ss->people[0].id1 & ss->people[3].id1 & ss->people[4].id1 & ss->people[7].id1;
         else if (ss->kind == s1x8)
            k = ss->people[0].id1 & ss->people[2].id1 & ss->people[2].id1 & ss->people[2].id1;
         else
            goto bad;

         if (k & PERSON_MOVED)
            goto bad;
         else
            goto good;

      default:
         break;
      }

      goto bad;

   do_2fl_stuff:
      tt.assumption = cr_2fl_only;

      switch (ssA) {
      case cr_li_lo: case cr_1fl_only: case cr_wave_only:
      case cr_miniwaves: case cr_magic_only: goto bad;
      }

      switch (ssK) {
      case s2x4:
      case s1x8:
         if (key == b_1x4) tt.assumption = cr_2fl_per_1x4;
         break;
      }

      goto fix_col_line_stuff;

   do_wave_stuff:
      switch (ssA) {
      case cr_1fl_only: case cr_2fl_only: case cr_couples_only: case cr_magic_only: goto bad;
      case cr_li_lo: goto bad;
      case cr_ijright: case cr_ijleft: case cr_real_1_4_line: case cr_real_3_4_line: goto bad;
      case cr_wave_only: case cr_jright:
         if (ssB == 2 && tt.assump_both == 1) goto bad;
         if (ssB == 1 && tt.assump_both == 2) goto bad;
         break;
      case cr_jleft:
         if (ssB == 2 && tt.assump_both == 2) goto bad;
         if (ssB == 1 && tt.assump_both == 1) goto bad;
         break;
      }

      tt.assumption = cr_wave_only;

      switch (ssK) {
         uint32_t vv;
      case s3x4:         // This only handles lines; not columns --
                         // we couldn't have "wavy" columns that were symmetric.
      case s_bone6:
      case s_qtag:
      case s3x1dmd:
         goto check_tt;
      case s_trngl:
         // See if this is tandem-based.  If so, we are presumably checking
         // for something like single ferris wheel from a triangle.
         if ((ss->people[1].id1 | ss->people[2].id1) & 1) tt.assump_col = 1;
         goto check_tt;
      case s_galaxy:
         vv = ss->people[1].id1 | ss->people[3].id1 |
            ss->people[5].id1 | ss->people[7].id1;
         if (!(vv&010)) tt.assump_col = 1;
         else if (vv&001) goto bad;
         goto check_tt;
      case sdmd:
      case s_ptpd:
      case s_short6:
         tt.assump_col = 1;
         goto check_tt;
      case s2x4:
         // If the setup is a T-boned 2x4, but we are deciding whether
         // to split into 2x2's, let it pass.
         if (key == b_2x2 && (u & 011) == 011) goto good;
         else if (key == b_1x4) tt.assumption = cr_miniwaves;
         break;
      }

   fix_col_line_stuff:

      // This is a horrible kludge to handle "stroll explode the wave" and the evil
      // "1/2" hinge from a 4x5 occupied as 4 "stairstep" minwaves (going to a tidal
      // wave).  We have to make "wave_only" and "2fl_only" do two different things
      // (check the 1x4 down the middle in the former case and check each of the 4
      // miniwaves in the latter case), and we don't want further explosion of the
      // "cr_wave_only" and "cr_2fl_only" variants.  Thank you, Bryan Clark.

      if (ssK == s4x5 && (little_endian_live_mask(ss) & 0x06C1B) == 0)
         goto check_tt;

      switch (ssK) {
      case s1x3:
         if (tt.assump_both) goto bad;   // We can't check a 1x3 for right-or-left-handedness.
         /* FALL THROUGH!!! */
      case s1x6: case s1x8: case s1x10:
      case s1x12: case s1x14: case s1x16:
      case s2x2: case s3x4: case s4x4: case s_thar: case s_crosswave: case s_qtag:
      case s3x1dmd: case s_2x1dmd: case sbigdmd:
      case s_trngl: case s_bone: case s_alamo: case s_hrglass:
         /* FELL THROUGH!!! */
         goto check_tt;
      case sdmd: case s_ptpd:
         tt.assump_col = 1;
         goto check_tt;
      case s1x4:  // Note that 1x6, 1x8, etc should be here also.  This will
                  // make "cr_2fl_only" and such things work in 4x1.
      case s1x2:
      case s2x4:
      case s2x6:
      case s2x8:
      case s2x3:
         if (!(u&010)) tt.assump_col = 1;

         // This line commented out.  We now recognize "handedness" of the waves of 3
         // in a 2x3.  OK?  This is needed for 1/2 circulate from lines of 3 with
         // centers facing and ends back-to-back.  See test vg05/vg02.
         //         else if (tt.assump_both && ssK == s2x3) goto bad;
         //    /* We can't check 2x3 lines for right-or-left-handedness. */
         goto check_tt;
      default:
         /* ****** Try to change this (and other things) to "goto bad". */
         goto good;                 /* We don't understand the setup -- we'd better accept it. */
      }

   check_tt:

      // If the "explicit" flag was on, we don't accept anything unless an assumption was in place.

      if (require_explicit && ssA == cr_none) goto bad;

      if (verify_restriction(ss, tt, false, &booljunk) == restriction_passes) return p;
      continue;

   bad:

      if (tt.assump_negate) return p;
      else continue;

   good:

      if (!tt.assump_negate) return p;
      else continue;
   }

   return (callarray *) 0;
}



uint32_t uncompress_position_number(uint32_t datum)
{
   int field = ((datum >> 2) & 0x1F) - 1;
   return (field <= 24) ? field : (field-12) << 1;
}



extern void clear_result_flags(setup *z, uint32_t preserve_these /* = 0 */)
{
   z->result_flags.misc &= preserve_these;
   z->result_flags.res_heritflags_to_save_from_mxn_expansion = 0ULL;
   z->result_flags.clear_split_info();
}

setup::setup(setup_kind k, int r,
             uint32_t P0, uint32_t I0,
             uint32_t P1, uint32_t I1,
             uint32_t P2, uint32_t I2,
             uint32_t P3, uint32_t I3,
             uint32_t P4, uint32_t I4,
             uint32_t P5, uint32_t I5,
             uint32_t P6, uint32_t I6,
             uint32_t P7, uint32_t I7,
             uint32_t little_endian_wheretheygo /* = 0x76543210 */) :
   kind(k), rotation(r), eighth_rotation(0)
{
   clear_people();
   clear_result_flags(this);
   int i;

   // This is kind of klutzy and inefficient, but this constructor is only
   // used at program startup.
   i = little_endian_wheretheygo & 0xF;
   people[i].id1 = P0; people[i].id2 = I0; people[i].id3 = 0;
   i = (little_endian_wheretheygo >> 4) & 0xF;
   people[i].id1 = P1; people[i].id2 = I1; people[i].id3 = 0;
   i = (little_endian_wheretheygo >> 8) & 0xF;
   people[i].id1 = P2; people[i].id2 = I2; people[i].id3 = 0;
   i = (little_endian_wheretheygo >> 12) & 0xF;
   people[i].id1 = P3; people[i].id2 = I3; people[i].id3 = 0;

   if (attr::klimit(k) < 4) return;   // Just calling to the heads.

   i = (little_endian_wheretheygo >> 16) & 0xF;
   people[i].id1 = P4; people[i].id2 = I4; people[i].id3 = 0;
   i = (little_endian_wheretheygo >> 20) & 0xF;
   people[i].id1 = P5; people[i].id2 = I5; people[i].id3 = 0;
   i = (little_endian_wheretheygo >> 24) & 0xF;
   people[i].id1 = P6; people[i].id2 = I6; people[i].id3 = 0;
   i = (little_endian_wheretheygo >> 28) & 0xF;
   people[i].id1 = P7; people[i].id2 = I7; people[i].id3 = 0;
}




extern uint32_t copy_person(setup *resultpeople, int resultplace, const setup *sourcepeople, int sourceplace)
{
   resultpeople->people[resultplace] = sourcepeople->people[sourceplace];
   return resultpeople->people[resultplace].id1;
}


extern uint32_t copy_rot(setup *resultpeople, int resultplace, const setup *sourcepeople, int sourceplace, int rotamount)
{
   uint32_t newperson = sourcepeople->people[sourceplace].id1;

   if (newperson) newperson = (newperson + rotamount) & ~064;
   resultpeople->people[resultplace].id2 = sourcepeople->people[sourceplace].id2;
   resultpeople->people[resultplace].id3 = sourcepeople->people[sourceplace].id3;
   return resultpeople->people[resultplace].id1 = newperson;
}


extern void install_person(setup *resultpeople, int resultplace, const setup *sourcepeople, int sourceplace)
{
   uint32_t newperson = sourcepeople->people[sourceplace].id1;

   if (resultpeople->people[resultplace].id1 == 0)
      resultpeople->people[resultplace] = sourcepeople->people[sourceplace];
   else if (newperson) {
      collision_person1 = resultpeople->people[resultplace].id1;
      collision_person2 = newperson;
      error_message1[0] = '\0';
      error_message2[0] = '\0';
      throw error_flag_type(error_flag_collision);
   }
}


extern void install_rot(setup *resultpeople, int resultplace, const setup *sourcepeople, int sourceplace, int rotamount) THROW_DECL
{
   uint32_t newperson = sourcepeople->people[sourceplace].id1;

   if (newperson) {
      if (resultplace < 0) fail(resultplace == -2 ?
                                "Can't do this shape-changing call with this concept." :
                                "This would go into an excessively large matrix.");

      if (resultpeople->people[resultplace].id1 == 0) {
         resultpeople->people[resultplace].id1 = (newperson + rotamount) & ~064;
         resultpeople->people[resultplace].id2 = sourcepeople->people[sourceplace].id2;
         resultpeople->people[resultplace].id3 = sourcepeople->people[sourceplace].id3;
      }
      else {
         collision_person1 = resultpeople->people[resultplace].id1;
         collision_person2 = newperson;
         error_message1[0] = '\0';
         error_message2[0] = '\0';
         throw error_flag_type(error_flag_collision);
      }
   }
}


extern void scatter(setup *resultpeople, const setup *sourcepeople,
                    const int8_t *resultplace, int countminus1, int rotamount) THROW_DECL
{
   int k, idx;
   for (k=0; k<=countminus1; k++) {
      idx = resultplace[k];

      if (idx < 0) {
         // This could happen in "touch_or_rear_back".
         if (sourcepeople->people[k].id1) fail("Don't understand this setup.");
      }
      else
         copy_rot(resultpeople, idx, sourcepeople, k, rotamount);
   }
}


extern void gather(setup *resultpeople, const setup *sourcepeople,
                   const int8_t *resultplace, int countminus1, int rotamount)
{
   int k, idx;
   for (k=0; k<=countminus1; k++) {
      idx = resultplace[k];

      if (idx >= 0)
         copy_rot(resultpeople, k, sourcepeople, idx, rotamount);
   }
}


extern void install_scatter(setup *resultpeople, int num, const int8_t *placelist,
                            const setup *sourcepeople, int rot) THROW_DECL
{
   for (int j=0; j<num; j++)
      install_rot(resultpeople, placelist[j], sourcepeople, j, rot);
}


void warn_about_concept_level()
{
   if (allowing_all_concepts)
      warn(warn__bad_concept_level);
   else if (!two_couple_calling)
      fail("This concept is not allowed at this level.");
}


void turn_4x4_pinwheel_into_c1_phantom(setup *ss)
{
   if (ss->kind == s4x4) {
      switch(little_endian_live_mask(ss)) {
      case 0xAAAA:
         expand::compress_setup(s_c1phan_4x4a, ss);
         break;
      case 0xCCCC:
         expand::compress_setup(s_c1phan_4x4b, ss);
         break;
      }
   }
}



bool clean_up_unsymmetrical_setup(setup *ss)
{
   static expand::thing thing_splinedmd_1x8 = {{0, 1, 3, 2, -1, -1, -1, -1}, splinedmd, s1x8, 0};
   static expand::thing thing_splinedmd_qtag = {{-1, -1, -1, -1, 3, 1, 2, 4}, splinedmd, s_qtag, 0};
   uint32_t livemask = little_endian_live_mask(ss);

   switch (ss->kind) {
   case splinedmd:
      if ((livemask & 0xF0) == 0) {
         expand::expand_setup(thing_splinedmd_1x8, ss);
         return true;
      }
      else if ((livemask & 0x0F) == 0) {
         expand::expand_setup(thing_splinedmd_qtag, ss);
         return true;
      }
   }

   return false;
}


static const expand::thing s_2x4_qtg = {{3, 4, -1, -1, 7, 0, -1, -1}, s_qtag, s2x4, 3};


setup_kind try_to_expand_dead_conc(const setup & result,
                                   const call_with_name *call,
                                   setup & lineout, setup & qtagout, setup & dmdout)
{
   lineout = result;
   lineout.rotation += lineout.inner.srotation;
   qtagout = result;
   qtagout.rotation += qtagout.inner.srotation;
   dmdout = result;
   dmdout.rotation += dmdout.inner.srotation;

   if (result.inner.skind == s1x4) {
      static const expand::thing exp_conc_1x8 = {{3, 2, 7, 6}, s1x4, s1x8, 0};
      static const expand::thing exp_conc_qtg = {{6, 7, 2, 3}, s1x4, s_qtag, 0};
      static const expand::thing exp_conc_dmd = {{1, 2, 5, 6}, s1x4, s3x1dmd, 0};
      expand::expand_setup(exp_conc_1x8, &lineout);
      expand::expand_setup(exp_conc_qtg, &qtagout);
      expand::expand_setup(exp_conc_dmd, &dmdout);
   }
   else if (result.inner.skind == s2x2) {
      if (result.concsetup_outer_elongation == 1)
         expand::expand_setup(s_2x2_2x4_ctrs, &lineout);
      else if (result.concsetup_outer_elongation == 2)
         expand::expand_setup(s_2x2_2x4_ctrsb, &lineout);
      else fail("Can't figure out where the ends went.");
   }

   return result.inner.skind;
}


// Do_heritflag_merge is in common.cpp.


// Take a concept pointer and scan for all "final" concepts,
// returning an updated concept pointer and a mask of all such concepts found.
// "Final" concepts are those that modify the execution of a call but
// do not cause it to be executed in a virtual or distorted setup.
// This has a side-effect that is occasionally used:  When it passes over
// any "magic" or "interlocked" concept, it drops a pointer to where the
// last such occurred into the external variable "last_magic_diamond". */
//
// If not checking for errors, we set the "final" bits correctly
// (we can always do that, since the complex bit field stuff isn't
// used in this), and we just set anything in the "herit" field.
// In fact, we set something in the "herit" field even if this item
// was "final".  Doing all this gives the required behavior, which is:
//
// (1) If any concept at all, final or herit, the herit field
//    will be nonzero.
// (2) The herit field is otherwise messed up.  Only zero/nonzero matters.
// (3) In any case the "final" field will be correct. */

parse_block *process_final_concepts(
   parse_block *cptr,
   bool check_errors,
   final_and_herit_flags *final_concepts,
   bool forbid_unfinished_parse,
   bool only_one) THROW_DECL
{
   for ( ; cptr ; cptr=cptr->next) {
      finalflags the_final_bit = (finalflags) 0;
      uint32_t forbidfinalbit = 0;
      heritflags heritsetbit = 0ULL;
      heritflags forbidheritbit = 0ULL;

      if (cptr->concept->kind >= FIRST_SIMPLE_HERIT_CONCEPT &&
          cptr->concept->kind <= LAST_SIMPLE_HERIT_CONCEPT) {
         heritsetbit = simple_herit_bits_table[cptr->concept->kind - FIRST_SIMPLE_HERIT_CONCEPT];
      }
      else {
         switch (cptr->concept->kind) {
         case concept_comment:
            continue;               // Skip comments.
         case concept_triangle:
            the_final_bit = FINAL__TRIANGLE;
            forbidfinalbit = FINAL__LEADTRIANGLE;
            goto new_final;
         case concept_leadtriangle:
            the_final_bit = FINAL__LEADTRIANGLE;
            forbidfinalbit = FINAL__TRIANGLE;
            goto new_final;
         case concept_magic:
            last_magic_diamond = cptr;
            heritsetbit = INHERITFLAG_MAGIC;
            forbidheritbit = INHERITFLAG_SINGLE | INHERITFLAG_DIAMOND;
            break;
         case concept_interlocked:
            last_magic_diamond = cptr;
            heritsetbit = INHERITFLAG_INTLK;
            forbidheritbit = INHERITFLAG_SINGLE | INHERITFLAG_DIAMOND;
            break;
         case concept_grand:
            heritsetbit = INHERITFLAG_GRAND;
            forbidheritbit = INHERITFLAG_SINGLE;
            break;
         case concept_cross:
            heritsetbit = INHERITFLAG_CROSS; break;
         case concept_reverse:
            heritsetbit = INHERITFLAG_REVERSE; break;
         case concept_fast:
            heritsetbit = INHERITFLAG_FAST; break;
         case concept_left:
            heritsetbit = INHERITFLAG_LEFT; break;
         case concept_yoyo:
            heritsetbit = INHERITFLAG_YOYOETCK_YOYO;
            forbidheritbit = INHERITFLAG_SCATTER;
            break;
         case concept_generous:
            heritsetbit = INHERITFLAG_YOYOETCK_GENEROUS;
            forbidheritbit = INHERITFLAG_SCATTER;
            break;
         case concept_stingy:
            heritsetbit = INHERITFLAG_YOYOETCK_STINGY;
            forbidheritbit = INHERITFLAG_SCATTER;
            break;
         case concept_fractal:
            heritsetbit = INHERITFLAG_FRACTAL;
            forbidheritbit = INHERITFLAG_SCATTER;
            break;
         case concept_straight:
            heritsetbit = INHERITFLAG_STRAIGHT; break;
         case concept_rewind:
            heritsetbit = INHERITFLAG_REWIND; break;
         case concept_twisted:
            heritsetbit = INHERITFLAG_TWISTED; break;
         case concept_single:
            heritsetbit = INHERITFLAG_SINGLE; break;
         case concept_singlefile:
            heritsetbit = INHERITFLAG_SINGLEFILE; break;
         case concept_1x2:
            heritsetbit = INHERITFLAGMXNK_1X2; break;
         case concept_2x1:
            heritsetbit = INHERITFLAGMXNK_2X1; break;
         case concept_1x3:
            heritsetbit = INHERITFLAGMXNK_1X3; break;
         case concept_3x1:
            heritsetbit = INHERITFLAGMXNK_3X1; break;
         case concept_3x0:
            heritsetbit = INHERITFLAGMXNK_3X0; break;
         case concept_0x3:
            heritsetbit = INHERITFLAGMXNK_0X3; break;
         case concept_4x0:
            heritsetbit = INHERITFLAGMXNK_4X0; break;
         case concept_0x4:
            heritsetbit = INHERITFLAGMXNK_0X4; break;
         case concept_6x2:
            heritsetbit = INHERITFLAGMXNK_6X2; break;
         case concept_3x2:
            heritsetbit = INHERITFLAGMXNK_3X2; break;
         case concept_3x5:
            heritsetbit = INHERITFLAGMXNK_3X5; break;
         case concept_5x3:
            heritsetbit = INHERITFLAGMXNK_5X3; break;
         case concept_2x2:
            heritsetbit = INHERITFLAGNXNK_2X2; break;
         case concept_3x3:
            heritsetbit = INHERITFLAGNXNK_3X3; break;
         case concept_4x4:
            heritsetbit = INHERITFLAGNXNK_4X4; break;
         case concept_5x5:
            heritsetbit = INHERITFLAGNXNK_5X5; break;
         case concept_6x6:
            heritsetbit = INHERITFLAGNXNK_6X6; break;
         case concept_7x7:
            heritsetbit = INHERITFLAGNXNK_7X7; break;
         case concept_8x8:
            heritsetbit = INHERITFLAGNXNK_8X8; break;
         case concept_revert:
            heritsetbit = (heritflags) cptr->concept->arg1; break;
         case concept_split:
            the_final_bit = FINAL__SPLIT;
            goto new_final;
         case concept_12_matrix:
            if (check_errors && final_concepts->test_for_any_herit_or_final_bit())
               fail("Matrix modifier must appear first.");
            heritsetbit = INHERITFLAG_12_MATRIX;
            break;
         case concept_16_matrix:
            if (check_errors && final_concepts->test_for_any_herit_or_final_bit())
               fail("Matrix modifier must appear first.");
            heritsetbit = INHERITFLAG_16_MATRIX;
            break;
         case concept_diamond:
            heritsetbit = INHERITFLAG_DIAMOND;
            forbidheritbit = INHERITFLAG_SINGLE;
            break;
         case concept_funny:
            heritsetbit = INHERITFLAG_FUNNY; break;
         default:
            return cptr;
         }
      }

      // At this point we have a "herit" concept.

      if (check_errors) {
         if (final_concepts->bool_test_heritbits(forbidheritbit))
            fail("Illegal order of call modifiers.");

         if (do_heritflag_merge(final_concepts->herit, heritsetbit))
            fail("Illegal combination of call modifiers.");
      }
      else {
         // If not checking for errors, we just have to set the "herit" field nonzero.
         final_concepts->herit |= heritsetbit;
      }

      goto check_level;

   new_final:

      // This is a "final" concept.  It is more straightforward, because the word
      // is not broken into fields that need special checking.

      if (check_errors) {
         if ((final_concepts->final & (the_final_bit|forbidfinalbit)))
            fail("Redundant call modifier.");
      }
      else {
         // Put the bit into the "herit" stuff also, so that "herit" will tell
         // whether any modifier at all was seen.  Only the zero/nonzero nature
         // of the "herit" word will be looked at if we are not checking for errors.

         final_concepts->herit |= INHERITFLAG_DIAMOND;
      }

      final_concepts->set_finalbit(the_final_bit);

   check_level:

      if (check_errors && cptr->concept->level > calling_level)
         warn(warn__bad_concept_level);

      // Stop now if we have been asked to process only one concept.
      if (only_one && (the_final_bit != 0 || heritsetbit != 0ULL)) return cptr->next;
   }

   if (forbid_unfinished_parse) fail_no_retry("Incomplete parse.");

   return cptr;
}


skipped_concept_info::skipped_concept_info(parse_block *incoming) THROW_DECL
{
   if (!incoming)
      fail("Need a concept.");

   while (incoming->concept->kind == concept_comment)
      incoming = incoming->next;

   m_nocmd_misc3_bits = 0;
   m_heritflag = 0ULL;
   m_old_retval = incoming;
   m_skipped_concept = incoming;
   m_result_of_skip = m_skipped_concept->next;
   m_need_to_restrain = 0;
   m_root_of_result_of_skip = (parse_block **) 0;

   final_and_herit_flags junk_concepts;
   junk_concepts.clear_all_herit_and_final_bits();

   // We tell it to process only one concept.
   parse_block *parseptrcopy = process_final_concepts(incoming, false, &junk_concepts, true, true);

   m_concept_with_root = parseptrcopy;

   // Find out whether the next concept (the one that will be "random" or whatever)
   // is a modifier or a "real" concept.

   if (junk_concepts.final == 0 &&
       (junk_concepts.bool_test_heritbits(INHERITFLAG_YOYOETCMASK | INHERITFLAG_LEFT | INHERITFLAG_FRACTAL))) {
      m_heritflag = junk_concepts.herit;
      return;
   }
   else if (junk_concepts.test_for_any_herit_or_final_bit()) {
      parseptrcopy = incoming;
   }
   else if (parseptrcopy->concept) {
      concept_kind kk = parseptrcopy->concept->kind;

      if (check_for_supercall(parseptrcopy)) {
         if (parseptrcopy->call->the_defn.callflagsf & (CFLAGH__HAS_AT_ZERO | CFLAGH__HAS_AT_M)) {
            // This gets "busy [...]".
            // This gets "tally ho but [...]".
            // We skip over the supercall, and use the "but" subcall.
            m_concept_with_root = parseptrcopy->next;
            m_root_of_result_of_skip = &m_concept_with_root->subsidiary_root;
            m_result_of_skip = *m_root_of_result_of_skip;
            m_skipped_concept = &pbsuper;

            // We don't restrain for echo with supercalls, because echo doesn't pull parts
            // apart, and supercalls don't work with multiple-part calls as the target
            // for which they are restraining the concept.

            m_need_to_restrain = 1;
            m_old_retval = parseptrcopy;
            return;
         }
         else {
            // This gets "[...] motivate".
            // We don't skip over anything -- we just return the motivate, but with an indicator
            // that the "anythinger's" subcall is not to be used.
            m_result_of_skip = m_skipped_concept;
            m_need_to_restrain = 1;
            m_old_retval = parseptrcopy;
            m_nocmd_misc3_bits = CMD_MISC3__NO_ANYTHINGERS_SUBST;
            return;
         }
      }

      if (concept_table[kk].concept_action == 0)
         fail("Sorry, can't do this with this concept.");

      if ((concept_table[kk].concept_prop & CONCPROP__SECOND_CALL) &&
          parseptrcopy->concept->kind != concept_checkpoint &&
          parseptrcopy->concept->kind != concept_sandwich &&
          parseptrcopy->concept->kind != concept_on_your_own &&
          (parseptrcopy->concept->kind != concept_some_vs_others ||
           parseptrcopy->concept->arg1 != selective_key_own) &&
          parseptrcopy->concept->kind != concept_special_sequential)
         fail("Can't use a concept that takes a second call.");
   }

   check_for_concept_group(parseptrcopy, *this, true);

   m_old_retval = parseptrcopy;
   m_result_of_skip = *m_root_of_result_of_skip;
}



// Prepare several setups to be assembled into one, by making them all have
// the same kind and rotation.  If there is a question about what ending
// setup to opt for (because of lots of phantoms), use "goal".

// Goal of +9 means opt for dmd.
// Goal of +7 means make the rotations alternate.

// One of the hard test cases is, from a tidal wave,
// Split Phantom Diamonds Diamond Chain Thru.  It's in t13t.
// There is no code to track the phantoms through the cast off 3/4.
// Instead, merge_setups notices that it is merging "nothing" with
// diamond points, and changes the diamond points to ends of lines.
// It's not clear that this is the wisest way to do this.

// If the result is empty, put "nothing" into all of the z items.

bool fix_n_results(
   int arity,
   int goal,
   setup z[],
   uint32_t & rotstates,
   uint32_t & pointclip,
   uint32_t fudgystupidrot,
   bool allow_hetero_and_notify /* = false */) THROW_DECL
{
   int i;

   int lineflag = 0;
   bool dmdflag = false;
   bool qtflag = false;
   bool boxrectflag = false;
   int setfinal = 0;
   bool miniflag = false;
   int deadconcindex = -1;
   setup_kind kk = nothing;
   rotstates = 0xFFF;
   pointclip = 0;

   // The "rotstates" word collects information about the rotations
   // of the various setups.  The individual bits correspond to various
   // theories about what the rotations are.  The word starts out with
   // lots of bits set, and bits are cleared as the theories are rejected.
   // At the end of the scan there should still be one (or more) bits set.
   //
   // The meaning is (sort of) as follows.
   //
   //      even setups (we start counting with 0,
   //            which is even) have rot=1, odd ones have rot=0
   //                             |
   //                             |   even have rot=0, odd have rot=1
   //                             |   |
   //                             |   |   all have rot=3
   //                             |   |   |   all have rot=2
   //                             |   |   |   |   all have rot=1
   //                             |   |   |   |   |   all have rot=0
   //                             |   |   |   |   |   |
   //                             V   V   V   V   V   V
   //    <for triangles>
   //   | ?   ?   ?   ? | ?   ?         |               |
   //   |___|___|___|___|___|___|___|___|___|___|___|___|

   static uint16_t rotstate_table[8] = {
      UINT16_C(0x111), UINT16_C(0x222), UINT16_C(0x404), UINT16_C(0x808),
      UINT16_C(0x421), UINT16_C(0x812), UINT16_C(0x104), UINT16_C(0x208)};

   // There are 3 things that make this task nontrivial.  First, some setups could
   // be "nothing", in which case we turn them into the same type of setup as
   // their neighbors, with no people.  Second, some types of "grand working"
   // can leave setups confused about whether they are lines or diamonds,
   // because only the ends/points are occupied.  We turn those setups into
   // whatever matches their more-fully-occupied neighbors.  (If completely in
   // doubt, we opt for 1x4's.)  Third, some 1x4's may have been shrunk to 1x2's.
   // This can happen in some cases of "triple box patch the so-and-so", which lead
   // to triple lines in which the outer lines collapsed from 1x4's to 1x2's while
   // the center line is fully occupied.  In this case we repopulate the outer lines
   // to 1x4's.

   uint32_t eighth_rot_flag = ~0U;

   for (i=0; i<arity; i++) {
      // First, check that all setups have the same "eighth rotation" stuff,
      // and don't have any "imprecise rotation".

      if (z[i].result_flags.misc & RESULTFLAG__IMPRECISE_ROT)
         goto lose;

      // This guides us in following differing rotations from one subsetup to the
      // next.  (The whole point of the rotstates and the rotstate_table is to track this.)
      // But, if rotations change from one subsetup to the next and there are 4 or more
      // of them, we don't expect them to rotate uniformly from left to right.
      // We want to track a rotation set of {0, 1, 0, 1}, which would happen if we did
      // a quadruple boxes reach out in a 2x8 with the center boxes T-boned to the outer
      // ones.  So we swap the last two subsetups.  This is tested in t46t.
      // If the number is higher than that, we have no effective way of dealing with
      // nonuniform rotations.

      if (z[i].kind == s_normal_concentric) {

         // We definitely have a problem.  A common way for this to happen
         // is if a concentric call was done and there were no ends,
         // so we don't know what the setup really is.
         // Example:  from normal columns, do a split phantom lines hocus-pocus.
         // Do we leave space for the phantoms?  Humans would probably say yes
         // because they know where the phantoms went, but this program has no
         // idea in general.  If a call is concentrically executed and the
         // outsides are all phantoms, we don't know what the setup is.
         // Concentric_move signifies this by creating a "concentric" setup
         // with "nothing" for the outer setup.  So we raise an error that is
         // somewhat descriptive.

         if (z[i].outer.skind == nothing)
            continue;  // Defer this until later; we may be able to figure something out.
         else if (z[i].inner.skind == nothing && z[i].outer.skind == s1x2) {

            // We can fix this up.  Just turn it into a 1x4 with the ends missing.
            // (If a diamond is actually required, that will get fixed up below.)
            // The test case for this is split phantom lines cross to a diamond from 2FL.

            z[i].kind = s1x4;
            z[i].rotation = z[i].outer.srotation;
            z[i].eighth_rotation = 0;
            copy_person(&z[i], 0, &z[i], 12);
            copy_person(&z[i], 2, &z[i], 13);
            z[i].clear_person(1);
            z[i].clear_person(3);
         }
         else
            fail("Don't recognize ending setup for this call.");
      }
      else if (z[i].kind == s_dead_concentric) {
         continue;  // Defer this until later; we may be able to figure something out.
      }

      if (z[i].kind != nothing) {
         canonicalize_rotation(&z[i]);

         if (eighth_rot_flag == ~0U)
            eighth_rot_flag = z[i].eighth_rotation;
         else if (eighth_rot_flag ^ z[i].eighth_rotation)
            goto lose;

         uint32_t dmdqtagfudge = 0;

         // Unfortunately, this code wants all setups to be oriented the
         // same way, so that it will know how to make empty setups look like their brethren.
         // For example, if we are in an alamo ring that somehow is missing one of its
         // miniwaves, and the call is "hinge", "fix_n_results" wants to see all resulting
         // miniwaves have rotation = 1.  That way, it can fill in the missing miniwave to
         // look like the others, resulting in a thar.

         // So, until I figure out how to do this correctly, we need to undo the rotation
         // that we put in above, making it look like the old code, which forced each subsetup
         // x[i] to have rotation zero before doing the call.

         // Except for the 3 maps noted in the next sentence, all maps with the "100" bit on
         // have inner kind = s_trngl/s_trngl4 and rotation = 102, 108, 107, or 10D,
         // and no maps have just 02, 08, 07, or 0D.
         // The 3 exceptions have MPKIND__NONISOTROPDMD and rot = 104.

         if (z[i].kind == s1x2)
            miniflag = true;
         else if (z[i].kind == s1x4 && (z[i].people[1].id1 | z[i].people[3].id1) == 0)
            lineflag |= 1;
         else if (z[i].kind == sdmd && (z[i].people[1].id1 | z[i].people[3].id1) == 0)
            lineflag |= 2;
         else {
            if (kk == nothing) kk = z[i].kind;

            if (kk != z[i].kind) {
               // We may have a minor problem -- differently oriented
               // 2x4's and qtag's with just the ends present might want to be
               // like each other.  We will turn them into qtags.
               // Or 1x4's with just the centers present might want
               // to become diamonds.

               if (((kk == s2x4 && z[i].kind == s_qtag) ||
                    (kk == s_qtag && z[i].kind == s2x4))) {
                  qtflag = true;
                  dmdqtagfudge = 1;
               }
               else if (((kk == s2x4 && z[i].kind == s2x2) ||
                         (kk == s2x2 && z[i].kind == s2x4))) {
                  boxrectflag = true;
               }
               else if (((kk == s1x4 && z[i].kind == sdmd) ||
                         (kk == sdmd && z[i].kind == s1x4))) {
                  dmdflag = true;
                  dmdqtagfudge = 1;
               }
               else if (arity == 2 && fudgystupidrot == 5 &&
                        z[0].rotation == 1 && z[1].rotation == 1 &&
                        ((kk == s1x4 && z[i].kind == s1x6) ||
                         (kk == s1x6 && z[i].kind == s1x4))) {

                  // We have a 1x4 parallel to a 1x6.  Someone is doing a horrible unsymmetrical
                  // somewhat-colliding flip the diamond.
                  static const expand::thing exp_1x4_1x6 = {{1, 2, 4, 5}, s1x4, s1x6, 0};
                  expand::expand_setup(exp_1x4_1x6, &z[(z[i].kind == s1x4) ? i : 1-i]);
                  kk = s1x6;
               }
               else
                  goto maybelose;
            }
         }

         // If the setups are "trngl" or "trngl4", the rotations have
         // to alternate by 180 degrees.

         int zirot = z[i].rotation;

         if (z[i].kind == s_trngl || z[i].kind == s_trngl4) {
            // Except that, if there are two of them, and they are both triangles with the
            // SAME rotation, we treat them normally.  The "hetero" maps can now deal with
            // this situation.
            if (arity == 2 && z[0].kind == z[1].kind && z[0].rotation == z[1].rotation) {
               zirot = 0;  // The rotstate_table table won't understand what we are doing.
               rotstates &= 0x033;
            }
            else
               rotstates &= 0xF00;
         }
         else
            rotstates &= 0x033;

         canonicalize_rotation(&z[i]);
         rotstates &= rotstate_table[((i << 2) & 4) | ((zirot^dmdqtagfudge) & 3)];
      }
   }

   if (boxrectflag) kk = s2x4;

   if (kk == nothing) {
      // If client really needs a diamond, return a diamond.
      if (lineflag != 0) {
         if (goal == 9) kk = sdmd;
         else if (lineflag == 1)
            kk = s1x4;
         else if (lineflag == 2)
            kk = sdmd;
         else fail("Can't do this: don't know where the phantoms went.");
      }
      else if (miniflag) kk = s1x2;
   }
   else if (kk == s2x2) {
      rotstates &= 0x011;
      // If the arity is 3 or more, we shut off the second digit.
      // The second digit is for things that are presumed to be nonisotropic.
      // A bunch of 2x2's can't be nonisotropic.
      if (arity >=2) rotstates &= 1;
   }

   if (arity == 1 && kk != s_trngl && kk != s_trngl4) rotstates &= 0x3;
   if (!rotstates) goto lose;

   // Now deal with any setups that we may have deferred.

   if (dmdflag && kk == s1x4) {
      rotstates ^= 3;
      kk = sdmd;
   }
   else if (qtflag && kk == s2x4) {
      rotstates ^= 3;
      kk = s_qtag;
   }

   for (i=0; i<arity; i++) {
      if (z[i].kind == s_dead_concentric ||
          (z[i].kind == s_normal_concentric && z[i].outer.skind == nothing)) {
         int rr;

         if (z[i].inner.skind == s2x2 && kk == s2x4) {
            // Turn the 2x2 into a 2x4.  Need to make it have same rotation as the others;
            // that is, rotation = rr.  (We know that rr has something in it by now.)

            // We might have a situation with alternating rotations, or we
            // might have homogeneous rotations.  If the state says we could
            // have both, change it to just homogeneous.  That is, we default
            // to same rotation unless live people force mixed rotation.

            if (rotstates & 0x0F) rotstates &= 0x03;     // That does the defaulting.
            if (!rotstates) goto lose;
            rr = (((rotstates & 0x0F0) ? (rotstates >> 4) : rotstates) >> 1) & 1;

            z[i].inner.srotation -= rr;
            canonicalize_rotation(&z[i]);
            z[i].kind = kk;
            z[i].rotation = rr;
            z[i].eighth_rotation = 0;
            z[i].swap_people(3, 6);
            z[i].swap_people(2, 5);
            z[i].swap_people(2, 1);
            z[i].swap_people(1, 0);
            z[i].clear_person(4);
            z[i].clear_person(7);
            z[i].clear_person(0);
            z[i].clear_person(3);
            canonicalize_rotation(&z[i]);
         }
         else if (z[i].inner.skind == s1x4 && kk == s_qtag) {
            // Turn the 1x4 into a qtag.
            if (rotstates & 0x0F) rotstates &= 0x03;     // That does the defaulting.
            if (!rotstates) goto lose;
            rr = (((rotstates & 0x0F0) ? (rotstates >> 4) : rotstates) >> 1) & 1;
            if (z[i].inner.srotation != rr) goto lose;

            z[i].kind = kk;
            z[i].rotation = rr;
            z[i].eighth_rotation = 0;
            z[i].swap_people(0, 6);
            z[i].swap_people(1, 7);
            z[i].clear_person(0);
            z[i].clear_person(1);
            z[i].clear_person(4);
            z[i].clear_person(5);
            canonicalize_rotation(&z[i]);
         }
         else if (kk == nothing &&
                  (deadconcindex < 0 ||
                   (z[i].inner.skind == z[deadconcindex].inner.skind &&
                    z[i].inner.srotation == z[deadconcindex].inner.srotation &&
                    z[i].inner.seighth_rotation == z[deadconcindex].inner.seighth_rotation))) {
            deadconcindex = i;
         }
         else
            fail("Can't do this: don't know where the phantoms went.");
      }
      else if (qtflag && z[i].kind == s2x4) {
         // Turn the 2x4 into a qtag.
         if (z[i].people[1].id1 | z[i].people[2].id1 |
             z[i].people[5].id1 | z[i].people[6].id1) goto lose;
         expand::compress_setup(s_2x4_qtg, &z[i]);
      }
      else if (dmdflag && z[i].kind == s1x4) {
         // Turn the 1x4 into a diamond.
         if (z[i].people[0].id1 | z[i].people[2].id1) goto maybelose;
         expand::compress_setup(s_1x4_dmd, &z[i]);
         pointclip |= 1 << i;
      }
      else if (boxrectflag && z[i].kind == s2x2) {
         canonicalize_rotation(&z[i]);

         if (rotstates & 0x20) {
            if (i&1) {
               expand::expand_setup(s_2x2_2x4_ctrsb, &z[i]);
               setfinal = 2;
            }
            else rotstates = 0;   // fail.
         }
         else if (rotstates & 0x10) {
            if (!(i&1)) {
               expand::expand_setup(s_2x2_2x4_ctrsb, &z[i]);
               setfinal = 1;
            }
            else rotstates = 0;   // fail.
         }
         else {
            expand::expand_setup((rotstates & 2) ? s_2x2_2x4_ctrsb : s_2x2_2x4_ctrs, &z[i]);
         }
      }

      canonicalize_rotation(&z[i]);
   }

   if (setfinal != 0)
      rotstates = setfinal;

   if (deadconcindex >= 0) {
      kk = z[deadconcindex].kind;
      rotstates = 0x001;
   }

   if (kk == nothing) {
      for (i=0; i<arity; i++)
         z[i].kind = nothing;
      return false;
   }

   // If something wasn't sure whether it was points of a diamond or
   // ends of a 1x4, that's OK if something else had a clue.
   if (lineflag != 0 && kk != s1x4 && kk != sdmd) goto lose;

   // If something was a 1x2, that's OK if something else was a 1x4.
   if (miniflag && kk != s1x4 && kk != s1x2) goto lose;

   // We know rotstates has a nonzero bit in an appropriate field.
   // This will guide us in setting the rotations of empty setups.

   // Goal=7 means that we prefer alternating rotations, if we have a choice.
   // (That is, if some setups are "nothing".)  If we have bits on in both the
   // 0x03 region and the 0x30 region, we have a choice.
   if (goal == 7 && (rotstates & 0x30) != 0)
      rotstates &= 0x30;

   for (i=0; i<arity; i++) {
      if (z[i].kind == nothing) {
         z[i].kind = kk;
         z[i].clear_people();
         z[i].eighth_rotation = 0;

         if (rotstates & 0x1)
            z[i].rotation = 0;
         else if (rotstates & 0x2)
            z[i].rotation = 1;
         else if (rotstates & 0x4)
            z[i].rotation = 2;
         else if (rotstates & 0x8)
            z[i].rotation = 3;
         else if (rotstates & 0x10)
            z[i].rotation = i & 1;
         else if (rotstates & 0x20)
            z[i].rotation = ~i & 1;
         else if (rotstates & 0x100)
            z[i].rotation = 2*(i & 1);
         else if (rotstates & 0x200)
            z[i].rotation = 1+2*(i & 1);
         else if (rotstates & 0x400)
            z[i].rotation = 2-2*(i & 1);
         else if (rotstates & 0x800)
            z[i].rotation = 3-2*(i & 1);
         else
            fail("Don't recognize ending setup for this call.");
      }
      else if (z[i].kind == s1x2 && kk == s1x4) {
         // We have to expand a 1x2 to the center spots of a 1x4.
         copy_person(&z[i], 3, &z[i], 1);
         z[i].clear_person(2);
         copy_person(&z[i], 1, &z[i], 0);
         z[i].clear_person(0);
      }

      z[i].kind = kk;

      if (kk == s_dead_concentric || kk == s_normal_concentric) {
         z[i].inner.skind = z[deadconcindex].inner.skind;
         z[i].inner.srotation = z[deadconcindex].inner.srotation;
         z[i].inner.seighth_rotation = z[deadconcindex].inner.seighth_rotation;
      }
   }

   return false;

   maybelose:

   if (allow_hetero_and_notify)
      // It's a major problem, but we have been told to allow it.
      return true;

   lose:

   fail("This is an inconsistent shape or orientation changer.");
   return false;
}




bool warnings_are_unacceptable(bool strict)
{
   // If we are doing a "standardize", we let ALL warnings pass.
   // We particularly want weird T-bones and other unusual sort of things.

   if (!strict) return false;

   // We only care about "bad" warnings.
   if (!configuration::test_multiple_warnings(no_search_warnings)) return false;

   // If the "allow all concepts" switch isn't on, we lose.
   if (!allowing_all_concepts) return true;

   // But if "allow all concepts" was given, and the only bad warning is
   // "bad_concept_level", we let it pass.

   // So we test for any bad warnings other than "warn__bad_concept_level".
   warning_info otherthanbadconc = configuration::save_warnings();
   otherthanbadconc.clearbit(warn__bad_concept_level);
   return otherthanbadconc.testmultiple(no_search_warnings);
}


const expand::thing s_dmd_hrgl = {{6, 3, 2, 7}, sdmd, s_hrglass, 0};
const expand::thing s_dmd_hrgl_disc = {{6, -1, 3, 2, -1, 7}, s_1x2dmd, s_hrglass, 0};
/* s_1x2_dmd is duplicated in the big table. */
const expand::thing s_1x2_dmd = {{3, 1}, s1x2, sdmd, 1};
const expand::thing s_1x2_hrgl = {{7, 3}, s1x2, s_hrglass, 1};
const expand::thing s_dmd_323 = {{5, 7, 1, 3}, sdmd, s_323, 1};

// The "action" argument tells how hard we work to remove the outside phantoms.
// When merging the results of "on your own" or "own the so-and-so",
// we set action=normalize_before_merge to work very hard at stripping away
// outside phantoms, so that we can see more readily how to put things together.
// When preparing for an isolated call, that is, "so-and-so do your part, whatever",
// we work at it a little, so we set action=normalize_before_isolated_call.
// For normal usage, we set action=simple_normalize.
void normalize_setup(setup *ss, normalize_action action, qtag_compress_choice noqtagcompress) THROW_DECL
{
   uint32_t livemask, tbonetest;
   bool did_something = false;

 startover:

   tbonetest = or_all_people(ss);
   livemask = little_endian_live_mask(ss);

   if (ss->kind == sfat2x8)
      ss->kind = s2x8;     /* That's all it takes! */
   else if (ss->kind == swide4x4)
      ss->kind = s4x4;     /* That's all it takes! */
   else if (ss->kind == s_dead_concentric && action > plain_normalize) {
      ss->kind = ss->inner.skind;
      ss->rotation += ss->inner.srotation;
      ss->eighth_rotation += ss->inner.seighth_rotation;
      did_something = true;
      goto startover;
   }

   if (action == normalize_compress_bigdmd) {
      if (ss->kind == sbigdmd || ss->kind == sbigptpd) {
         // They're the same!
         if ((livemask & 00003) == 00001) ss->swap_people(0, 1);
         if ((livemask & 00060) == 00040) ss->swap_people(4, 5);
         if ((livemask & 00300) == 00100) ss->swap_people(6, 7);
         if ((livemask & 06000) == 04000) ss->swap_people(10, 11);
         action = simple_normalize;
         goto startover;
      }
      else return;   // Huh?
   }

   // A few difficult cases.

   // **** We really require that the second clause not happen unless the first did!!!
   if (ss->kind == s_323) {
      if (!(ss->people[0].id1 | ss->people[2].id1 |
            ss->people[4].id1 | ss->people[6].id1)) {
         expand::compress_setup(s_dmd_323, ss);

         if (action >= normalize_before_isolated_call) {
            if (ss->kind == sdmd && !(ss->people[0].id1 | ss->people[2].id1)) {
               expand::compress_setup(s_1x2_dmd, ss);
            }
         }

         did_something = true;
         goto startover;
      }
   }

   // Next, search for simple things in the hash table.

   setup_kind oldk = ss->kind;

   // decide whether to compress a qtag to a 2x3.  If people are T-boned, don't,
   // even if the ends of the center 1x4 are missing.
   bool b = expand::compress_from_hash_table(ss, action, livemask,
                                             noqtagcompress == qtag_no_compress ||
                                             (noqtagcompress == qtag_compress_unless_tboned && (tbonetest & 011) == 011));

   if (!b) goto difficult;

   did_something = true;
   // If we compressed a "swqtag" to a "q_tag", take no further action.
   // Do not further compress to a 2x3.
   if (oldk == swqtag && ss->kind == s_qtag) goto kinda_done;
   goto startover;

 difficult:

   // A few difficult cases.

   // **** this map is externally visible
   if (ss->kind == s4x6) {
      if (!(ss->people[0].id1 | ss->people[11].id1 |
            ss->people[18].id1 | ss->people[17].id1 |
            ss->people[5].id1 | ss->people[6].id1 |
            ss->people[23].id1 | ss->people[12].id1)) {
         expand::compress_setup(s_4x4_4x6a, ss);
         did_something = true;
         goto startover;
      }
   }

   if (!did_something && ss->kind == s4x4 && action == normalize_after_exchange_boxes) {
      // We have no idea how to deal with compression after exchange the boxes in a T-bone.
      if (!(tbonetest & 0x1)) {
         if ((livemask & 0x0030) == 0x0010 && (ss->people[4].id1 & 2) != 0) ss->swap_people(4, 5);
         if ((livemask & 0x3000) == 0x1000 && (ss->people[12].id1 & 2) == 0) ss->swap_people(12, 13);
         if ((livemask & 0x0140) == 0x0100 && (ss->people[8].id1 & 2) != 0) ss->swap_people(8, 6);
         if ((livemask & 0x4001) == 0x0001 && (ss->people[0].id1 & 2) == 0) ss->swap_people(0, 14);
         if ((livemask & 0x0084) == 0x0004 && (ss->people[2].id1 & 2) != 0) ss->swap_people(2, 7);
         if ((livemask & 0x8400) == 0x0400 && (ss->people[10].id1 & 2) == 0) ss->swap_people(10, 15);
         if ((livemask & 0x0A00) == 0x0200 && (ss->people[9].id1 & 2) != 0) ss->swap_people(9, 11);
         if ((livemask & 0x000A) == 0x0002 && (ss->people[1].id1 & 2) == 0) ss->swap_people(1, 3);
         action = simple_normalize;
         goto startover;
      }
      else if (!(tbonetest & 0x8)) {
         if ((livemask & 0x0003) == 0x0001 && (ss->people[0].id1 & 2) == 0) ss->swap_people(0, 1);
         if ((livemask & 0x0300) == 0x0100 && (ss->people[8].id1 & 2) != 0) ss->swap_people(8, 9);
         if ((livemask & 0x0014) == 0x0010 && (ss->people[4].id1 & 2) == 0) ss->swap_people(4, 2);
         if ((livemask & 0x1400) == 0x1000 && (ss->people[12].id1 & 2) != 0) ss->swap_people(12, 10);
         if ((livemask & 0x4008) == 0x4000 && (ss->people[14].id1 & 2) == 0) ss->swap_people(14, 3);
         if ((livemask & 0x0840) == 0x0040 && (ss->people[6].id1 & 2) != 0) ss->swap_people(6, 11);
         if ((livemask & 0x00A0) == 0x0020 && (ss->people[5].id1 & 2) == 0) ss->swap_people(5, 7);
         if ((livemask & 0xA000) == 0x2000 && (ss->people[13].id1 & 2) != 0) ss->swap_people(13, 15);
         action = simple_normalize;
         goto startover;
      }
   }
   else if (!did_something && ss->kind == s2x6 && action == normalize_after_exchange_boxes) {
      if ((livemask & 0xC00) == 0x800 && !(ss->people[11].id1 & 1)) ss->swap_people(11, 10);
      if ((livemask & 0x0C0) == 0x040 && !(ss->people[ 6].id1 & 1)) ss->swap_people(6, 7);
      if ((livemask & 0x030) == 0x020 && !(ss->people[ 5].id1 & 1)) ss->swap_people(4, 5);
      if ((livemask & 0x003) == 0x001 && !(ss->people[ 0].id1 & 1)) ss->swap_people(0, 1);
      action = simple_normalize;
      goto startover;
   }
   else if (!did_something && ss->kind == s2x8 && action == normalize_after_exchange_boxes) {
      if ((livemask & 0xE000) == 0x8000 && !(ss->people[15].id1 & 1)) ss->swap_people(15, 13);
      if ((livemask & 0xE000) == 0x4000 && !(ss->people[14].id1 & 1)) ss->swap_people(14, 13);
      if ((livemask & 0x0700) == 0x0100 && !(ss->people[ 8].id1 & 1)) ss->swap_people(8, 10);
      if ((livemask & 0x0700) == 0x0200 && !(ss->people[ 9].id1 & 1)) ss->swap_people(9, 10);
      if ((livemask & 0x00E0) == 0x0080 && !(ss->people[ 7].id1 & 1)) ss->swap_people(7, 5);
      if ((livemask & 0x00E0) == 0x0040 && !(ss->people[ 6].id1 & 1)) ss->swap_people(6, 5);
      if ((livemask & 0x0007) == 0x0001 && !(ss->people[ 0].id1 & 1)) ss->swap_people(0, 2);
      if ((livemask & 0x0007) == 0x0002 && !(ss->people[ 1].id1 & 1)) ss->swap_people(1, 2);

      livemask = little_endian_live_mask(ss);

      if ((livemask & 0xF000) == 0xA000 && !(ss->people[15].id1 & 1) && !(ss->people[13].id1 & 1)) {
         ss->swap_people(13, 12); ss->swap_people(15, 13); }
      if ((livemask & 0x0F00) == 0x0500 && !(ss->people[ 8].id1 & 1) && !(ss->people[10].id1 & 1)) {
         ss->swap_people(10, 11); ss->swap_people(8, 10); }
      if ((livemask & 0x00F0) == 0x00A0 && !(ss->people[ 7].id1 & 1) && !(ss->people[ 5].id1 & 1)) {
         ss->swap_people(5, 4); ss->swap_people(7, 5); }
      if ((livemask & 0x000F) == 0x0005 && !(ss->people[ 0].id1 & 1) && !(ss->people[ 2].id1 & 1)) {
         ss->swap_people(2, 3); ss->swap_people(0, 2); }

      if ((livemask & 0xF000) == 0xC000 && !(ss->people[15].id1 & 1) && !(ss->people[14].id1 & 1)) {
         ss->swap_people(14, 12); ss->swap_people(15, 13); }
      if ((livemask & 0x0F00) == 0x0300 && !(ss->people[ 8].id1 & 1) && !(ss->people[ 9].id1 & 1)) {
         ss->swap_people(9, 11); ss->swap_people(8, 10); }
      if ((livemask & 0x00F0) == 0x00C0 && !(ss->people[ 7].id1 & 1) && !(ss->people[ 6].id1 & 1)) {
         ss->swap_people(6, 4); ss->swap_people(7, 5); }
      if ((livemask & 0x000F) == 0x0003 && !(ss->people[ 0].id1 & 1) && !(ss->people[ 1].id1 & 1)) {
         ss->swap_people(1, 3); ss->swap_people(0, 2); }

      if ((livemask & 0xF000) == 0xD000 && !(ss->people[15].id1 & 1) && !(ss->people[14].id1 & 1)) {
         ss->swap_people(14, 13); ss->swap_people(15, 14); }
      if ((livemask & 0x0F00) == 0x0B00 && !(ss->people[ 8].id1 & 1) && !(ss->people[ 9].id1 & 1)) {
         ss->swap_people(9, 10); ss->swap_people(8, 9); }
      if ((livemask & 0x00F0) == 0x00D0 && !(ss->people[ 7].id1 & 1) && !(ss->people[ 6].id1 & 1)) {
         ss->swap_people(6, 5); ss->swap_people(7, 6); }
      if ((livemask & 0x000F) == 0x000B && !(ss->people[ 0].id1 & 1) && !(ss->people[ 1].id1 & 1)) {
         ss->swap_people(1, 2); ss->swap_people(0, 1); }

      if ((livemask & 0xF000) == 0xE000 && !(ss->people[15].id1 & 1) && !(ss->people[14].id1 & 1) && !(ss->people[13].id1 & 1)) {
         ss->swap_people(13, 12); ss->swap_people(14, 13); ss->swap_people(15, 14); }
      if ((livemask & 0x0F00) == 0x0700 && !(ss->people[ 8].id1 & 1) && !(ss->people[ 9].id1 & 1) && !(ss->people[10].id1 & 1)) {
         ss->swap_people(10, 11); ss->swap_people(9, 10); ss->swap_people(8, 9); }
      if ((livemask & 0x00F0) == 0x00E0 && !(ss->people[ 7].id1 & 1) && !(ss->people[ 6].id1 & 1) && !(ss->people[ 5].id1 & 1)) {
         ss->swap_people(5, 4); ss->swap_people(6, 5); ss->swap_people(7, 6); }
      if ((livemask & 0x000F) == 0x0007 && !(ss->people[ 0].id1 & 1) && !(ss->people[ 1].id1 & 1) && !(ss->people[ 2].id1 & 1)) {
         ss->swap_people(2, 3); ss->swap_people(1, 2); ss->swap_people(0, 1); }

      livemask = little_endian_live_mask(ss);

      if ((livemask & 0x7000) == 0x6000 && !(ss->people[14].id1 & 1) && !(ss->people[13].id1 & 1)) {
         ss->swap_people(13, 12); ss->swap_people(14, 13); }
      if ((livemask & 0x0E00) == 0x0600 && !(ss->people[ 9].id1 & 1) && !(ss->people[10].id1 & 1)) {
         ss->swap_people(10, 11); ss->swap_people(9, 10); }
      if ((livemask & 0x0070) == 0x0060 && !(ss->people[ 6].id1 & 1) && !(ss->people[ 5].id1 & 1)) {
         ss->swap_people(5, 4); ss->swap_people(6, 5); }
      if ((livemask & 0x000E) == 0x0006 && !(ss->people[ 1].id1 & 1) && !(ss->people[ 2].id1 & 1)) {
         ss->swap_people(2, 3); ss->swap_people(1, 2); }

      action = plain_normalize;
      goto startover;
   }
   else if (!did_something && ss->kind == s4x6 && action == normalize_after_exchange_boxes) {
      // Any line-like people that were way out get pulled in.
      if ((livemask & 0xC00000) == 0x800000 && !(ss->people[11].id1 & 1)) ss->swap_people(11, 10);
      if ((livemask & 0x0C0000) == 0x040000 && !(ss->people[ 6].id1 & 1)) ss->swap_people(6, 7);
      if ((livemask & 0xC00000) == 0x800000 && !(ss->people[23].id1 & 1)) ss->swap_people(23, 22);
      if ((livemask & 0x0C0000) == 0x040000 && !(ss->people[18].id1 & 1)) ss->swap_people(18, 19);
      // Any column-like people that were past the exchange point get pulled in.
      if ((livemask & 0x000090) == 0x000010 && (ss->people[4].id1 & 0xA) == 0) ss->swap_people(4, 7);
      if ((livemask & 0x090000) == 0x010000 && (ss->people[16].id1 & 0xA) == 2) ss->swap_people(16, 19);
      if ((livemask & 0x402000) == 0x002000 && (ss->people[13].id1 & 0xA) == 0) ss->swap_people(13, 22);
      if ((livemask & 0x000402) == 0x000002 && (ss->people[1].id1 & 0xA) == 2) ss->swap_people(1, 10);
      if ((livemask & 0x000108) == 0x000008 && (ss->people[3].id1 & 0xA) == 0) ss->swap_people(3, 8);
      if ((livemask & 0x108000) == 0x008000 && (ss->people[15].id1 & 0xA) == 2) ss->swap_people(15, 20);
      if ((livemask & 0x204000) == 0x004000 && (ss->people[14].id1 & 0xA) == 0) ss->swap_people(14, 21);
      if ((livemask & 0x000204) == 0x000004 && (ss->people[2].id1 & 0xA) == 2) ss->swap_people(2, 9);
      action = simple_normalize;
      goto startover;
   }
   else if (ss->kind == s3x4 && (livemask & 01111) == 0) {
      expand::compress_setup(s_qtg_3x4, ss);
      did_something = true;
      goto startover;
   }

   if (action >= normalize_to_4 &&
       action != normalize_after_triple_squash &&
       (ss->kind == s_hrglass || ss->kind == s_dhrglass)) {
      if (action >= normalize_to_2 && (livemask & 0x77) == 0) {
         // Be sure we match what the map says -- that might get checked someday.
         ss->kind = s_hrglass;
         expand::compress_setup(s_1x2_hrgl, ss);
         did_something = true;
         goto startover;
      }
      else if ((livemask & 0x33) == 0)  {
         // If normalizing before a merge (which might be from a "disconnected"),
         // and it is a real hourglass, be sure we leave space.
         const expand::thing & t = (action == normalize_after_disconnected && ss->kind == s_hrglass) ?
            s_dmd_hrgl_disc : s_dmd_hrgl;

         // Be sure we match what the map says -- that might get checked someday.
         ss->kind = s_hrglass;
         expand::compress_setup(t, ss);
         did_something = true;
         goto startover;
      }
   }

 kinda_done:

   canonicalize_rotation(ss);
}



void check_concept_parse_tree(parse_block *conceptptr, bool strict) THROW_DECL
{
   for (;;) {

      if (!conceptptr)
         fail("Incomplete parse.");

      if (conceptptr->concept->kind <= marker_end_of_list) {
         if (!conceptptr->call)
            fail("Incomplete parse.");

         // If it is "strict" (that is, we are checking the result of a random search),
         // we check that all subcalls have been filled in.
         if (strict &&
             (conceptptr->call->the_defn.callflagsf &
              (CFLAGH__HAS_AT_ZERO | CFLAGH__HAS_AT_M))) {

            // This call requires subcalls.
            if (conceptptr->concept->kind != concept_another_call_next_mod)
               fail("Incomplete parse.");

            bool subst1_in_use = false;
            bool subst2_in_use = false;

            parse_block *search = conceptptr->next;
            while (search) {
               parse_block *subsidiary_ptr = search->subsidiary_root;
               if (!subsidiary_ptr)
                  fail("Incomplete parse.");

               check_concept_parse_tree(subsidiary_ptr, strict);

               switch (search->replacement_key) {
               case DFM1_CALL_MOD_ANYCALL/DFM1_CALL_MOD_BIT:
               case DFM1_CALL_MOD_MAND_ANYCALL/DFM1_CALL_MOD_BIT:
                  subst1_in_use = true;
                  break;
               case DFM1_CALL_MOD_OR_SECONDARY/DFM1_CALL_MOD_BIT:
               case DFM1_CALL_MOD_MAND_SECONDARY/DFM1_CALL_MOD_BIT:
                  subst2_in_use = true;
                  break;
               }

               search = search->next;
            }

            if ((!subst1_in_use &&
                 (conceptptr->call->the_defn.callflagsf & CFLAGH__HAS_AT_ZERO)) ||
                (!subst2_in_use &&
                 (conceptptr->call->the_defn.callflagsf & CFLAGH__HAS_AT_M)))
               fail("Incomplete parse.");
         }

         break;
      }
      else {
         if (concept_table[conceptptr->concept->kind].concept_prop & CONCPROP__SECOND_CALL) {
            check_concept_parse_tree(conceptptr->subsidiary_root, strict);
         }

         conceptptr = conceptptr->next;
      }
   }
}

bool check_for_centers_concept(uint32_t & callflags1_to_examine,   // We rewrite this.
                               parse_block * & parse_scan,         // This too.
                               const setup_command *the_cmd) THROW_DECL
{
   // If the call is a special sequence starter (e.g. spin a pulley) remove the implicit
   // "centers" concept and just do it.  The setup in this case will be a squared set
   // with so-and-so moved into the middle, which is what the encoding of these calls
   // wants.
   //
   // If the call is a "split square thru" or "split dixie style" type of call, and the
   // "split" concept has been given, possibly preceded by "left" we do the same.
   // We leave the "split" concept in place.  Other mechanisms will do the rest.
   //
   // This is made much more complicated by the fact that we want the examination
   // of the call to be transparent to modifiers, certain concepts (fractional,
   // stretch, maybe others will be added to this list in the future) and to
   // "<anything> and roll" types of suffixes.

   final_and_herit_flags finaljunk;
   finaljunk.clear_all_herit_and_final_bits();
   bool did_something = true;
   heritflags zeroherit = 0ULL;

   // Here's the loop that strips off the stuff that we need to strip off,
   // and checks the call.
   while (did_something) {
      did_something = false;

      while (parse_scan->concept->kind == concept_comment)
         parse_scan = parse_scan->next;

      if (parse_scan->call)
         callflags1_to_examine = parse_scan->call->the_defn.callflags1;

      if (callflags1_to_examine & (CFLAG1_SEQUENCE_STARTER|CFLAG1_SEQUENCE_STARTER_PROM)) {
         // The subject call is a sequence starter, or some kind of "outsides promenade" thing.
         return false;
      }
      else if ((callflags1_to_examine &
               (CFLAG1_SPLIT_LIKE_SQUARE_THRU | CFLAG1_SPLIT_LIKE_DIXIE_STYLE)) &&
               finaljunk.test_finalbit(FINAL__SPLIT)) {
         // The subject call is a conditional sequence starter with the "split" modifier.
         return false;
      }

      // Now skip things.
      // First, skip any fractional concept.  Only simple "M/N".
      // Also skip "stretch" and "once removed", and so on.
      // And "add <call>", which requires tracing the subsidiary_root.
      for ( ;; ) {
         if ((parse_scan->concept->kind == concept_fractional &&
              parse_scan->concept->arg1 == 0) ||
             parse_scan->concept->kind == concept_meta ||
             parse_scan->concept->kind == concept_once_removed ||
             parse_scan->concept->kind == concept_stable ||
             parse_scan->concept->kind == concept_frac_stable ||
             parse_scan->concept->kind == concept_mirror ||
             parse_scan->concept->kind == concept_concentric ||
             parse_scan->concept->kind == concept_tandem ||
             parse_scan->concept->kind == concept_frac_tandem ||
             parse_scan->concept->kind == concept_stretched_setup ||
             parse_scan->concept->kind == concept_stretch) {
            parse_scan = parse_scan->next;
         }
         else if ((parse_scan->concept->kind == concept_special_sequential &&
                   parse_scan->concept->arg1 == 0)) {
            parse_scan = parse_scan->subsidiary_root;
         }
         else
            break;

         did_something = true;
      }

      // Skip all simple modifiers.  If we pass a "split" modifier, remember same.
      finaljunk.clear_all_herit_and_final_bits();
      parse_block *new_parse_scan =
         process_final_concepts(parse_scan, false, &finaljunk, true, false);

      if (parse_scan != new_parse_scan) {
         parse_scan = new_parse_scan;
         did_something = true;
      }

      // If the subject call is something like "<anything> and roll",
      // look at the <anything> to see whether it is a sequence starter.
      // The test for this is "heads [split grand chain 8] and roll".
      // The subcall may have already been filled in (the user typed
      // "heads [split grand chain 8] and roll"), or it may need to be
      // filled in (the user typed "heads <anything> and roll".)
      // The concept kind will be "concept_another_call_next_mod" in the
      // former case, or "marker_end_of_list" in the latter.
      // In the latter case, "get_real_subcall" will perform the
      // query.  Either way, "thingout" will have the subcall.
      // We do this if the call is something like "<anything> and roll".
      // The test is that it is sequentially defined, and part 1 is
      // a mandatory substitution, replacing the call "nothing".
      if ((parse_scan->concept->kind == concept_another_call_next_mod ||
           parse_scan->concept->kind == marker_end_of_list) &&
          parse_scan->call &&
          parse_scan->call->the_defn.schema == schema_sequential) {
         calldefn *seqdef = &parse_scan->call->the_defn;
         by_def_item *part0item = &seqdef->stuff.seq.defarray[0];
         setup_command thingout;

         if (part0item->call_id == base_call_null &&
             (part0item->modifiers1 & DFM1_CALL_MOD_MASK) == DFM1_CALL_MOD_MAND_ANYCALL &&
             get_real_subcall(parse_scan,
                              part0item,
                              the_cmd,
                              seqdef,
                              false,
                              zeroherit,
                              &thingout)) {
            parse_scan = thingout.parseptr;
            // If there is no call, then process_final_concepts will do something
            // on the next round, and we will get something on the round after that.
            callflags1_to_examine = parse_scan->call ?
               parse_scan->call->the_defn.callflags1 : 0;
            did_something = true;
         }
      }
   }

   return true;
}


static void check_near_far(setup *ss, int Vsize, uint32_t *Vtable,
                           int howmany, bool speciallateral, uint32_t flag, bool do_near)
{
   if (howmany >= Vsize)
      return;

   int j, k, lowlim, hilim;
   uint32_t h_boxlist[8], v_boxlist[8];

   int h_boxpop = 0;
   int v_boxpop = 0;
   int dirs = 0;

   if (do_near) {
      // Do the "near" people, in the higher-numbered slots of Vtable.
      if ((Vtable[Vsize-howmany-1] >> 16) >= (Vtable[Vsize-howmany] >> 16))
         return;
      lowlim = Vsize-howmany;
      hilim = Vsize;
   }
   else {
      // Do the "far" people, in the lower-numbered slots of Vtable.
      if ((Vtable[howmany-1] >> 24) >= (Vtable[howmany] >> 24))
         return;
      lowlim = 0;
      hilim = howmany;
   }

   for (k=lowlim; k < hilim  ; k++) {
      // Get direction in our "absolute" space.
      dirs |= (ss->people[Vtable[k] & 0xFF].id1 + ss->rotation * 011) & ~064;
      if (!speciallateral) ss->people[Vtable[k] & 0xFF].id3 |= flag;

      for (j=0; j < v_boxpop ; j++) {
         if ((v_boxlist[j] >> 8) == (Vtable[k] >> 24)) {
            v_boxlist[j]++;   // Bump the count.
            goto v_out;
         }
      }

      v_boxlist[j] = ((Vtable[k] >> 24) << 8) + 1;
      v_boxpop++;

   v_out: ;

      for (j=0; j < h_boxpop ; j++) {
         if ((h_boxlist[j] >> 8) == ((Vtable[k] << 8) >> 24)) {
            h_boxlist[j]++;   // Bump the count.
            goto h_out;
         }
      }

      h_boxlist[j] = ((Vtable[k] << 8) >> 16) + 1;
      h_boxpop++;

   h_out: ;
   }

   uint32_t colbits = do_near ? ID3_NEARCOL : ID3_FARCOL;
   uint32_t linebits = do_near ? ID3_NEARLINE : ID3_FARLINE;

   if (speciallateral) {
      if (ss->rotation & 1) {
         colbits = do_near ? ID3_LEFTLINE : ID3_RIGHTLINE;
         linebits = do_near ? ID3_LEFTCOL : ID3_RIGHTCOL;
      }
      else {
         colbits = do_near ? ID3_RIGHTLINE : ID3_LEFTLINE;
         linebits = do_near ? ID3_RIGHTCOL : ID3_LEFTCOL;
      }
   }

   // The C1 phantom setup has unusually wide press/truck distances.
   uint32_t htrucksize = (ss->kind == s_c1phan) ? 0xC00 : 0x400;
   uint32_t vtrucksize = (ss->kind == s_c1phan) ? 0x800 : 0x400;

   // If the designated 4 people are in a 2x2 box, say so.
   // Two people vertically and two horizontally, and, since
   // the arrays are sorted, the coordinate differences are 4.
   if (Vsize == 8 && howmany == 4 && h_boxpop == 2 && v_boxpop == 2 &&
       (h_boxlist[0] & 0xFF) == 2 && (h_boxlist[1] & 0xFF) == 2 &&
       (v_boxlist[0] & 0xFF) == 2 && (v_boxlist[1] & 0xFF) == 2 &&
       v_boxlist[1] - v_boxlist[0] == vtrucksize &&
       h_boxlist[1] - h_boxlist[0] == htrucksize) {
      for (k=lowlim; k < hilim  ; k++)
         ss->people[Vtable[k] & 0xFF].id3 |=
            speciallateral ? flag : (do_near ? ID3_NEARBOX : ID3_FARBOX);
   }

   // We have one position horizontally and four vertically;
   // coordinate distances are sorted.
   // A vertical 1x4 (or horizontal if speciallateral is on.)
   if (Vsize == 8 && howmany == 4 && h_boxpop == 1 && v_boxpop == 4 &&
       (h_boxlist[0] & 0xFF) == 4 &&
       (v_boxlist[0] & 0xFF) == 1 && (v_boxlist[1] & 0xFF) == 1 &&
       (v_boxlist[2] & 0xFF) == 1 && (v_boxlist[3] & 0xFF) == 1 &&
       v_boxlist[1] - v_boxlist[0] == vtrucksize &&
       v_boxlist[2] - v_boxlist[1] == 0x400 &&
       v_boxlist[3] - v_boxlist[2] == vtrucksize) {

      // It's a line if (dirs & 010) == 0
      // It's a col  if (dirs & 001) == 0
      uint32_t bits = ((dirs & 001) == 0) ? colbits : (((dirs & 010) == 0) ? linebits : 0);
      for (k=lowlim; k < hilim  ; k++)
         ss->people[Vtable[k] & 0xFF].id3 |= bits;
   }

   // We have one position vertically and four horizontally;
   // coordinate distances are sorted.
   // A horizontal 1x4 (or vertical if speciallateral is on.)
   if (Vsize == 8 && howmany == 4 && h_boxpop == 4 && v_boxpop == 1 &&
       (v_boxlist[0] & 0xFF) == 4 &&
       (h_boxlist[0] & 0xFF) == 1 && (h_boxlist[1] & 0xFF) == 1 &&
       (h_boxlist[2] & 0xFF) == 1 && (h_boxlist[3] & 0xFF) == 1 &&
       h_boxlist[1] - h_boxlist[0] == vtrucksize &&
       h_boxlist[2] - h_boxlist[1] == 0x400 &&
       h_boxlist[3] - h_boxlist[2] == vtrucksize) {

      // It's a line if (dirs & 001) == 0
      // It's a col  if (dirs & 010) == 0
      uint32_t bits = ((dirs & 010) == 0) ? colbits : (((dirs & 001) == 0) ? linebits : 0);
      for (k=lowlim; k < hilim  ; k++)
         ss->people[Vtable [k] & 0xFF].id3 |= bits;
   }
}


static void check_near_far_wrapper(setup *ss, int Vsize, uint32_t *Vtable,
                                   int howmany, bool speciallateral, uint32_t nearflag, uint32_t farflag)
{
   check_near_far(ss, Vsize, Vtable, howmany, speciallateral, nearflag, true);
   check_near_far(ss, Vsize, Vtable, howmany, speciallateral, farflag, false);
}


extern void put_in_absolute_proximity_and_facing_bits(setup *ss)
{
   // Can't do it if rotation is not known.
   if (ss->result_flags.misc & RESULTFLAG__IMPRECISE_ROT) return;

   int i;

   if (attr::slimit(ss) >= 0) {
      // Put in headliner/sideliner stuff if possible.
      for (i=0; i<=attr::slimit(ss); i++) {
         if (ss->people[i].id1 & BIT_PERSON) {
            switch ((ss->people[i].id1 + ss->rotation) & 3) {
            case 0:
               ss->people[i].id3 |= ID3_FACEBACK;
               break;
            case 1:
               ss->people[i].id3 |= ID3_FACERIGHT;
               break;
            case 2:
               ss->people[i].id3 |= ID3_FACEFRONT;
               break;
            case 3:
               ss->people[i].id3 |= ID3_FACELEFT;
               break;
            }
         }
      }
   }

   if (setup_attrs[ss->kind].setup_props & SPROP_FIND_NEAR_PEOPLE) {
      if (ss->kind == s_c1phan) {
         // We don't allow random populations; dancers might find it ambiguous.
         // So, in each quadrant, we require 2 people, in local opposite places.
         uint32_t livemask = little_endian_live_mask(ss);
         uint32_t q1 = livemask & 0xF;
         uint32_t q2 = (livemask >> 4) & 0xF;
         uint32_t q3 = (livemask >> 8) & 0xF;
         uint32_t q4 = (livemask >> 12) & 0xF;
         if (q1 != 5 && q1 != 0xA) return;
         if (q2 != 5 && q2 != 0xA) return;
         if (q3 != 5 && q3 != 0xA) return;
         if (q4 != 5 && q4 != 0xA) return;
      }

      int k;

      // In these tables,
      //    "V" = front-to-back location.  More positive is near us; negative is far.
      //       This is used primarily for designating things like "near 3".
      //    "H" = left-to-right location.  More positive is to our right; negative is left.
      //       This is used primarily for designating things like "line on the caller's left".
      // But each of them is always involved, to tell whether some people are in an actual 2x2 box or whatever.
      //
      // Table word layout of Vtable:
      //    8 bits of V.     These are arranged so they are concatenated, and it sorts primarily on V
      //    8 bits of H.     and secondarily on H.
      //    8 bits of zero (because person indices fit in 8 bits.)
      //    "N": 8 bits.  The person index.
      //
      // Htable has H and V fields reversed:
      //    8 bits of H.     So it sorts primarily on H and secondarily on V.
      //    8 bits of V.
      //    8 bits of zero.
      //    "N": 8 bits.
      //
      // The tables are sorted from lower to higher, so far people are in lower slots.
      //
      uint32_t Vtable[MAX_PEOPLE], Htable[MAX_PEOPLE];
      int Vsize = 0;
      int Hsize = 0;

      for (i=0; i <= attr::slimit(ss) ; i++) {
         if (ss->people[i].id1 & BIT_PERSON) {
            // By adding 64 and truncating, we are turning them into unsigned
            // 8-bit numbers, so we can just concatenate them and compare both at once.
            uint32_t Vcoord = 
               ((ss->rotation & 1) ?
                setup_attrs[ss->kind].nice_setup_coords->xca[i] :
                -setup_attrs[ss->kind].nice_setup_coords->yca[i]) + 128;
            uint32_t Hcoord = 
               ((ss->rotation & 1) ?
                -setup_attrs[ss->kind].nice_setup_coords->yca[i] :
                setup_attrs[ss->kind].nice_setup_coords->xca[i]) + 128;

            if (ss->rotation & 2) {
               Vcoord = ~Vcoord;
               Hcoord = ~Hcoord;
            }

            Vcoord &= 0xFF;
            Hcoord &= 0xFF;

            int j;

            // V table.
            // Sort primarily on the coordinate we are interested in ("V", or front-to-back)
            // and secondarily on the other coordinate ("H", or side-to-side).

            for (j=0; j < Vsize ; j++) {
               if ((Vtable[j] >> 16) > ((Vcoord << 8) | Hcoord)) {
                  // Push the rest of the table up.
                  for (k=Vsize; k > j ; k--)
                     Vtable[k] = Vtable[k-1];
                  goto Vdone;
               }
            }

            j = Vsize;

         Vdone:

            Vtable[j] = (Vcoord << 24) | (Hcoord << 16) | i;
            Vsize++;

            // H table.
            // Sort primarily on "H", and secondarily on "V".
            // Note swapping of Vcoord and Hcoord here.

            for (j=0; j < Hsize ; j++) {
               if ((Htable[j] >> 16) > ((Hcoord << 8) | Vcoord)) {
                  // Push the rest of the table up.
                  for (k=Hsize; k > j ; k--)
                     Htable[k] = Htable[k-1];
                  goto Hdone;
               }
            }

            j = Hsize;

         Hdone:

            Htable[j] = (Hcoord << 24) | (Vcoord << 16) | i;
            Hsize++;
         }
      }

      check_near_far_wrapper(ss, Vsize, Vtable, 1, false, ID3_NEAREST1, ID3_FARTHEST1);
      check_near_far_wrapper(ss, Vsize, Vtable, 2, false, ID3_NEARTWO, ID3_FARTWO);
      check_near_far_wrapper(ss, Vsize, Vtable, 3, false, ID3_NEARTHREE, ID3_FARTHREE);
      check_near_far_wrapper(ss, Vsize, Vtable, 4, false, ID3_NEARFOUR, ID3_FARFOUR);
      check_near_far_wrapper(ss, Hsize, Htable, 4, true,  ID3_RIGHTBOX, ID3_LEFTBOX);
      check_near_far_wrapper(ss, Vsize, Vtable, 5, false, ID3_NEARFIVE, ID3_FARFIVE);
      check_near_far_wrapper(ss, Vsize, Vtable, 6, false, ID3_NEARSIX, ID3_FARSIX);
      check_near_far_wrapper(ss, Vsize, Vtable, 7, false, ID3_NOTFARTHEST1, ID3_NOTNEAREST1);
   }
}


// Top level move routine.

void toplevelmove() THROW_DECL
{
   current_options.initialize();

   // Must copy this out of the history; it may be modified.
   setup starting_setup = configuration::current_config().state;
   configuration & newhist = configuration::next_config();
   parse_block *conceptptr = newhist.command_root;

   // Check for an incomplete parse.  This could happen if we do
   // something like "make a pass but [?".  There's just no way to make sure
   // that the parse tree is complete.

   check_concept_parse_tree(conceptptr, false);

   /* Be sure that the amount of written history that we consider to be safely
      written is less than the item we are about to change. */
   if (written_history_items > config_history_ptr)
      written_history_items = config_history_ptr;

   starting_setup.cmd.cmd_misc_flags = 0;
   starting_setup.cmd.cmd_misc2_flags = 0;
   starting_setup.cmd.cmd_misc3_flags = 0;
   starting_setup.cmd.do_couples_her8itflags = 0ULL;
   starting_setup.cmd.cmd_fraction.set_to_null();
   starting_setup.cmd.cmd_assume.assumption = cr_none;
   starting_setup.cmd.cmd_assume.assump_cast = 0;
   starting_setup.cmd.prior_elongation_bits = 0;
   starting_setup.cmd.prior_expire_bits = 0;
   starting_setup.cmd.skippable_concept = (parse_block *) 0;
   starting_setup.cmd.skippable_heritflags = 0ULL;
   starting_setup.cmd.cmd_heritflags_to_save_from_mxn_expansion = 0ULL;
   starting_setup.cmd.restrained_concept = (parse_block *) 0;
   starting_setup.cmd.restrained_super8flags = 0ULL;
   starting_setup.cmd.restrained_super9flags = 0ULL;
   starting_setup.cmd.restrained_do_as_couples = false;
   starting_setup.cmd.restrained_miscflags = 0;
   starting_setup.cmd.restrained_misc2flags = 0;
   starting_setup.cmd.restrained_selector_decoder[0] = 0;
   starting_setup.cmd.restrained_selector_decoder[1] = 0;
   starting_setup.cmd.restrained_fraction.flags = 0;
   starting_setup.cmd.restrained_fraction.fraction = 0;

   newhist.init_warnings_specific();

   // If we are starting a sequence with the "so-and-so into the center and do whatever"
   // flag on, and this call is a "sequence starter", take special action.

   if (configuration::current_config().get_startinfo_specific()->into_the_middle) {

      // claim that config_history_ptr == 1

      parse_state.topcallflags1 = 0;
      parse_block *cnext = conceptptr->next;

      if (!check_for_centers_concept(parse_state.topcallflags1, cnext, &starting_setup.cmd)) {
         if (cnext->call &&
             (cnext->call->the_defn.schema == schema_concentric_specialpromenade ||
              cnext->call->the_defn.schema == schema_cross_concentric_specialpromenade)) {
            conceptptr->concept = (configuration::current_config().startinfoindex == start_select_sides_start) ?
               &concept_sides_concept : &concept_heads_concept;
            conceptptr->options.who.who[0] = (selector_kind) conceptptr->concept->arg1;

            starting_setup.kind = configuration::startinfolist[start_select_as_they_are].the_setup_p->kind;
            starting_setup.rotation = configuration::startinfolist[start_select_as_they_are].the_setup_p->rotation;
            starting_setup.eighth_rotation = 0;
            memcpy(starting_setup.people,
                   configuration::startinfolist[start_select_as_they_are].the_setup_p->people,
                   sizeof(personrec)*MAX_PEOPLE);
         }
         else {
            conceptptr = conceptptr->next;
         }
      }
   }

   // Clear a few things.  We do NOT clear the warnings, because some (namely the
   // "concept not allowed at this level" warning) may have already been logged.
   // Set the selectors to "uninitialized", so that, if we do a call like "run", we
   // will query the user to find out who is selected.
   newhist.init_centersp_specific();
   newhist.draw_pic = false;
   newhist.state_is_valid = false;
   newhist.init_resolve();

   // Put in identification bits for global/unsymmetrical stuff, if possible.
   clear_absolute_proximity_and_facing_bits(&starting_setup);
   put_in_absolute_proximity_and_facing_bits(&starting_setup);

   // Put in position-identification bits (leads/trailers/beaus/belles/centers/ends etc.)
   update_id_bits(&starting_setup);
   starting_setup.cmd.parseptr = conceptptr;
   starting_setup.cmd.callspec = (call_with_name *) 0;
   starting_setup.cmd.cmd_final_flags.clear_all_herit_and_final_bits();
   starting_setup.result_flags.misc &= ~RESULTFLAG__DID_MXN_EXPANSION;
   starting_setup.cmd.cmd_heritflags_to_save_from_mxn_expansion = 0ULL;
   move(&starting_setup, false, &newhist.state, true);
   newhist.state_is_valid = true;
   remove_mxn_spreading(&newhist.state);
   remove_tgl_distortion(&newhist.state);
   remove_fudgy_2x3_2x6(&newhist.state);

   // If did a Cycle and Wheel with the 2x2 facing recycle as both parts,
   // It isn't legal.  If it was done under a concentric concept, the error was cleared.
   if (newhist.test_one_warning_specific(warn_suspect_destroyline))
      fail("This isn't a legitimate cycle and wheel.");

   if (newhist.state.kind == s1p5x8)
      fail("Can't go into a 50% offset 1x8.");
   else if (newhist.state.kind == s1p5x4)
      fail("Can't go into a 50% offset 1x4.");
   else if (newhist.state.kind == s_dead_concentric) {
      newhist.state.kind = newhist.state.inner.skind;
      newhist.state.rotation += newhist.state.inner.srotation;
      newhist.state.eighth_rotation += newhist.state.inner.seighth_rotation;
   }

   // Once rotation is imprecise, it is always imprecise.  Same for the other flags copied here.
   newhist.state.result_flags.misc |= starting_setup.result_flags.misc &
      (RESULTFLAG__IMPRECISE_ROT|RESULTFLAG__ACTIVE_PHANTOMS_ON|RESULTFLAG__ACTIVE_PHANTOMS_OFF);
}


// Do the extra things that a call requires, that are not required
// when only testing for legality.

void finish_toplevelmove() THROW_DECL
{
   configuration & newhist = configuration::next_config();

   // Remove outboard phantoms from the resulting setup.
   normalize_setup(&newhist.state,
                   two_couple_calling ? normalize_to_4 : plain_normalize, 
                   two_couple_calling ? qtag_compress_unless_tboned : qtag_no_compress);
   // Resolve needs to know what "near 4" means right now.
   clear_absolute_proximity_and_facing_bits(&newhist.state);
   newhist.calculate_resolve();
}


// This stuff is duplicated in verify_call in sdmatch.c .
bool deposit_call_tree(modifier_block *anythings, parse_block *save1, int key)
{
   // First, if we have already deposited a call, and we see more stuff, it must be
   // concepts or calls for an "anything" subcall.

   if (save1) {
      parse_block *tt = get_parse_block();
      // Run to the end of any already-deposited things.  This could happen if the
      // call takes a tagger -- it could have a search chain before we even see it.
      while (save1->next) save1 = save1->next;
      save1->next = tt;
      save1->concept = &concept_marker_concept_mod;
      tt->concept = &concept_marker_concept_mod;
      tt->call = base_calls[(key == DFM1_CALL_MOD_MAND_SECONDARY/DFM1_CALL_MOD_BIT) ?
                           base_call_null_second: base_call_null];
      tt->call_to_print = tt->call;
      tt->replacement_key = key;
      parse_state.concept_write_ptr = &tt->subsidiary_root;
   }

   save1 = (parse_block *) 0;
   gg77->matcher_p->m_final_result.match.call_conc_options = anythings->call_conc_options;

   if (anythings->kind == ui_call_select) {
      if (deposit_call(anythings->call_ptr, &anythings->call_conc_options)) return true;
      save1 = *parse_state.concept_write_ptr;
      if (!there_is_a_call) the_topcallflags = parse_state.topcallflags1;
      there_is_a_call = true;
   }
   else if (anythings->kind == ui_concept_select) {
      if (deposit_concept(anythings->concept_ptr)) return true;
   }
   else return true;   // Huh?

   if (anythings->packed_next_conc_or_subcall) {
      if (deposit_call_tree(anythings->packed_next_conc_or_subcall, save1,
                            DFM1_CALL_MOD_MAND_ANYCALL/DFM1_CALL_MOD_BIT))
         return true;
   }

   if (anythings->packed_secondary_subcall) {
      if (deposit_call_tree(anythings->packed_secondary_subcall, save1,
                            DFM1_CALL_MOD_MAND_SECONDARY/DFM1_CALL_MOD_BIT))
         return true;
   }

   return false;
}



bool do_subcall_query(
   int snumber,
   parse_block *parseptr,
   parse_block **newsearch,
   bool this_is_tagger,
   bool this_is_tagger_circcer,
   call_with_name *orig_call)
{
   char tempstring_text[MAX_TEXT_LINE_LENGTH];

   // Note whether we are using any mandatory substitutions, so that the menu
   // initialization will always accept this call.

   if (snumber == (DFM1_CALL_MOD_MAND_ANYCALL/DFM1_CALL_MOD_BIT) ||
       snumber == (DFM1_CALL_MOD_MAND_SECONDARY/DFM1_CALL_MOD_BIT)) {
      // In some types of pick operations, the picker simply doesn't know how
      // to choose a mandatory subcall.  In that case, the call requiring the
      // mandatory subcall (e.g. "wheel and <anything>") is simply rejected.
      if (forbid_call_with_mandatory_subcall())
         fail_no_retry("Mandatory subcall fail.");
      mandatory_call_used = true;
   }

   // Now we know that the list doesn't say anything about this call.  Perhaps we should
   // query the user for a replacement and add something to the list.  First, decide whether
   // we should consider doing so.  If we are initializing the database, the answer is
   // always "no", even for calls that require a replacement call, such as
   // "clover and anything".  This means that, for the purposes of database initialization,
   // "clover and anything" is tested as "clover and nothing", since "nothing" is the subcall
   // that appears in the database.

   // Also, when doing pick operations, the picker might not want to do a random pick.
   // It might just want to leave the default call ("clover and [nothing]") in place.
   // So we ask the picker.

   // Of course, if we are testing the fidelity of later calls during a reconcile
   // operation, we DO NOT EVER add any modifiers to the list, even if the user
   // clicked on "allow modification" before clicking on "reconcile".  It is perfectly
   // legal to click on "allow modification" before clicking on "reconcile".  It means
   // we want modifications (chosen by random number generator, since we won't be
   // interactive) for the calls that we randomly choose, but not for the later calls
   // that we test for fidelity.

   if (!(interactivity == interactivity_normal ||
         allow_random_subcall_pick()) ||
       testing_fidelity)
      return true;

   // When we are searching for resolves and the like, the situation
   // is different.  In this case, the interactivity state is set for
   // a search.  We do perform mandatory modifications, so we will
   // generate things like "clover and shakedown".  Of course, no
   // querying actually takes place.  Instead, get_subcall just uses
   // the random number generator.  Therefore, whether resolving or in
   // normal interactive mode, we are guided by the call modifier
   // flags and the "allowing_modifications" global variable. */

   // Depending on what type of substitution is involved and what the "allowing modifications"
   // level is, we may choose not to query about this subcall, but just return the default.

   switch (snumber) {
   case DFM1_CALL_MOD_ANYCALL/DFM1_CALL_MOD_BIT:
   case DFM1_CALL_MOD_ALLOW_PLAIN_MOD/DFM1_CALL_MOD_BIT:
   case DFM1_CALL_MOD_OR_SECONDARY/DFM1_CALL_MOD_BIT:
      if (!allowing_modifications) return true;
      break;
   case DFM1_CALL_MOD_ALLOW_FORCED_MOD/DFM1_CALL_MOD_BIT:
      if (allowing_modifications <= 1) return true;
      break;
   }

   // At this point, we know we should query the user about this call.

   // Set ourselves up for modification by making the null modification list
   // if necessary.  ***** Someday this null list will always be present.

   if (parseptr->concept->kind == marker_end_of_list)
      parseptr->concept = &concept_marker_concept_mod;

   // Create a reference on the list.  "search" points to the null item at the end.

   tempstring_text[0] = '\0';           // Null string, just to be safe.

   // If doing a tagger, just get the call.

   if (snumber == 0 && this_is_tagger_circcer)
      ;

   // If the replacement is mandatory, or we are not interactive,
   // don't present the popup.  Just get the replacement call.

   else if (interactivity != interactivity_normal)
      ;
   else if (snumber == (DFM1_CALL_MOD_MAND_ANYCALL/DFM1_CALL_MOD_BIT))
      sprintf(tempstring_text, "SUBSIDIARY CALL");
   else if (snumber == (DFM1_CALL_MOD_MAND_SECONDARY/DFM1_CALL_MOD_BIT))
      sprintf(tempstring_text, "SECOND SUBSIDIARY CALL");
   else {

      // Need to present the popup to the operator
      // and find out whether modification is desired.

      char pretty_call_name[MAX_TEXT_LINE_LENGTH];

      // Star turn calls can have funny names like "nobox".

      gg77->unparse_call_name(
         (orig_call->the_defn.callflagsf & CFLAG2_IS_STAR_CALL) ?
         "turn the star @b" : orig_call->name,
         pretty_call_name, &current_options);

      const char *line_format;

      if (this_is_tagger)
         line_format = "The \"%s\" can be replaced with a tagging call.";
      else if (this_is_tagger_circcer)
         line_format = "The \"%s\" can be replaced with a modified circulate-like call.";
      else
         line_format = "The \"%s\" can be replaced.";

      char tempstuff[200];
      sprintf(tempstuff, line_format, pretty_call_name);
      if (gg77->iob88.yesnoconfirm("Replacement", tempstuff, "Do you want to replace it?", false, false)) {
         // User accepted the modification.
         // Set up the prompt and get the concepts and call.
         sprintf(tempstring_text, "REPLACEMENT FOR THE %s", pretty_call_name);
      }
      else {
         // User declined the modification.  Create a null entry
         // so that we don't query again.
         *newsearch = get_parse_block();
         (*newsearch)->concept = &concept_marker_concept_mod;
         (*newsearch)->options = current_options;
         (*newsearch)->replacement_key = snumber;
         (*newsearch)->call = orig_call;
         (*newsearch)->call_to_print = orig_call;
         return true;
      }
   }

   *newsearch = get_parse_block();
   (*newsearch)->concept = &concept_marker_concept_mod;
   (*newsearch)->options = current_options;
   (*newsearch)->replacement_key = snumber;
   (*newsearch)->call = orig_call;
   (*newsearch)->call_to_print = orig_call;

   /* Set stuff up for reading subcall and its concepts. */

   /* Create a new parse block, point concept_write_ptr at its contents. */
   /* Create the new root at the start of the subsidiary list. */

   parse_state.concept_write_base = &(*newsearch)->subsidiary_root;
   parse_state.concept_write_ptr = parse_state.concept_write_base;

   parse_state.parse_stack_index = 0;
   parse_state.call_list_to_use = call_list_any;
   strncpy(parse_state.specialprompt, tempstring_text, MAX_TEXT_LINE_LENGTH);

   // Search for special case of "must_be_tag_call" with no other modification bits.
   // That means it is a new-style tagging call.

   if (snumber == 0 && this_is_tagger_circcer) {
      throw error_flag_type(error_flag_wrong_command);
   }
   else {
      if (query_for_call()) {
         // User clicked on something unusual like "exit" or "undo".
         // These are generally not allowed.
         // But it might be "pick random call", which we can handle.
         if (global_reply.majorpart == ui_command_select &&
             ((command_kind) global_reply.minorpart) == command_random_call)
            throw error_flag_type(error_flag_user_wants_to_resolve);

         throw error_flag_type(error_flag_wrong_command);
      }
   }

   return false;
}



call_list_kind find_proper_call_list(setup *s)
{
   if (s->kind == s1x8) {
      if      ((s->people[0].id1 & 017) == 010 &&
               (s->people[1].id1 & 017) == 012 &&
               (s->people[2].id1 & 017) == 012 &&
               (s->people[3].id1 & 017) == 010 &&
               (s->people[4].id1 & 017) == 012 &&
               (s->people[5].id1 & 017) == 010 &&
               (s->people[6].id1 & 017) == 010 &&
               (s->people[7].id1 & 017) == 012)
         return call_list_1x8;
      else if ((s->people[0].id1 & 017) == 012 &&
               (s->people[1].id1 & 017) == 010 &&
               (s->people[2].id1 & 017) == 010 &&
               (s->people[3].id1 & 017) == 012 &&
               (s->people[4].id1 & 017) == 010 &&
               (s->people[5].id1 & 017) == 012 &&
               (s->people[6].id1 & 017) == 012 &&
               (s->people[7].id1 & 017) == 010)
         return call_list_l1x8;
      else if ((s->people[0].id1 & 015) == 1 &&
               (s->people[1].id1 & 015) == 1 &&
               (s->people[2].id1 & 015) == 1 &&
               (s->people[3].id1 & 015) == 1 &&
               (s->people[4].id1 & 015) == 1 &&
               (s->people[5].id1 & 015) == 1 &&
               (s->people[6].id1 & 015) == 1 &&
               (s->people[7].id1 & 015) == 1)
         return call_list_gcol;
   }
   else if (s->kind == s2x4) {
      if      ((s->people[0].id1 & 017) == 1 &&
               (s->people[1].id1 & 017) == 1 &&
               (s->people[2].id1 & 017) == 3 &&
               (s->people[3].id1 & 017) == 3 &&
               (s->people[4].id1 & 017) == 3 &&
               (s->people[5].id1 & 017) == 3 &&
               (s->people[6].id1 & 017) == 1 &&
               (s->people[7].id1 & 017) == 1)
         return call_list_dpt;
      else if ((s->people[0].id1 & 017) == 3 &&
               (s->people[1].id1 & 017) == 3 &&
               (s->people[2].id1 & 017) == 1 &&
               (s->people[3].id1 & 017) == 1 &&
               (s->people[4].id1 & 017) == 1 &&
               (s->people[5].id1 & 017) == 1 &&
               (s->people[6].id1 & 017) == 3 &&
               (s->people[7].id1 & 017) == 3)
         return call_list_cdpt;
      else if ((s->people[0].id1 & 017) == 1 &&
               (s->people[1].id1 & 017) == 1 &&
               (s->people[2].id1 & 017) == 1 &&
               (s->people[3].id1 & 017) == 1 &&
               (s->people[4].id1 & 017) == 3 &&
               (s->people[5].id1 & 017) == 3 &&
               (s->people[6].id1 & 017) == 3 &&
               (s->people[7].id1 & 017) == 3)
         return call_list_rcol;
      else if ((s->people[0].id1 & 017) == 3 &&
               (s->people[1].id1 & 017) == 3 &&
               (s->people[2].id1 & 017) == 3 &&
               (s->people[3].id1 & 017) == 3 &&
               (s->people[4].id1 & 017) == 1 &&
               (s->people[5].id1 & 017) == 1 &&
               (s->people[6].id1 & 017) == 1 &&
               (s->people[7].id1 & 017) == 1)
         return call_list_lcol;
      else if ((s->people[0].id1 & 017) == 1 &&
               (s->people[1].id1 & 017) == 3 &&
               (s->people[2].id1 & 017) == 1 &&
               (s->people[3].id1 & 017) == 3 &&
               (s->people[4].id1 & 017) == 3 &&
               (s->people[5].id1 & 017) == 1 &&
               (s->people[6].id1 & 017) == 3 &&
               (s->people[7].id1 & 017) == 1)
         return call_list_8ch;
      else if ((s->people[0].id1 & 017) == 3 &&
               (s->people[1].id1 & 017) == 1 &&
               (s->people[2].id1 & 017) == 3 &&
               (s->people[3].id1 & 017) == 1 &&
               (s->people[4].id1 & 017) == 1 &&
               (s->people[5].id1 & 017) == 3 &&
               (s->people[6].id1 & 017) == 1 &&
               (s->people[7].id1 & 017) == 3)
         return call_list_tby;
      else if ((s->people[0].id1 & 017) == 012 &&
               (s->people[1].id1 & 017) == 012 &&
               (s->people[2].id1 & 017) == 012 &&
               (s->people[3].id1 & 017) == 012 &&
               (s->people[4].id1 & 017) == 010 &&
               (s->people[5].id1 & 017) == 010 &&
               (s->people[6].id1 & 017) == 010 &&
               (s->people[7].id1 & 017) == 010)
         return call_list_lin;
      else if ((s->people[0].id1 & 017) == 010 &&
               (s->people[1].id1 & 017) == 010 &&
               (s->people[2].id1 & 017) == 010 &&
               (s->people[3].id1 & 017) == 010 &&
               (s->people[4].id1 & 017) == 012 &&
               (s->people[5].id1 & 017) == 012 &&
               (s->people[6].id1 & 017) == 012 &&
               (s->people[7].id1 & 017) == 012)
         return call_list_lout;
      else if ((s->people[0].id1 & 017) == 010 &&
               (s->people[1].id1 & 017) == 012 &&
               (s->people[2].id1 & 017) == 010 &&
               (s->people[3].id1 & 017) == 012 &&
               (s->people[4].id1 & 017) == 012 &&
               (s->people[5].id1 & 017) == 010 &&
               (s->people[6].id1 & 017) == 012 &&
               (s->people[7].id1 & 017) == 010)
         return call_list_rwv;
      else if ((s->people[0].id1 & 017) == 012 &&
               (s->people[1].id1 & 017) == 010 &&
               (s->people[2].id1 & 017) == 012 &&
               (s->people[3].id1 & 017) == 010 &&
               (s->people[4].id1 & 017) == 010 &&
               (s->people[5].id1 & 017) == 012 &&
               (s->people[6].id1 & 017) == 010 &&
               (s->people[7].id1 & 017) == 012)
         return call_list_lwv;
      else if ((s->people[0].id1 & 017) == 010 &&
               (s->people[1].id1 & 017) == 010 &&
               (s->people[2].id1 & 017) == 012 &&
               (s->people[3].id1 & 017) == 012 &&
               (s->people[4].id1 & 017) == 012 &&
               (s->people[5].id1 & 017) == 012 &&
               (s->people[6].id1 & 017) == 010 &&
               (s->people[7].id1 & 017) == 010)
         return call_list_r2fl;
      else if ((s->people[0].id1 & 017) == 012 &&
               (s->people[1].id1 & 017) == 012 &&
               (s->people[2].id1 & 017) == 010 &&
               (s->people[3].id1 & 017) == 010 &&
               (s->people[4].id1 & 017) == 010 &&
               (s->people[5].id1 & 017) == 010 &&
               (s->people[6].id1 & 017) == 012 &&
               (s->people[7].id1 & 017) == 012)
         return call_list_l2fl;
   }
   else if (s->kind == s_qtag)
      return call_list_qtag;

   return call_list_any;
}
