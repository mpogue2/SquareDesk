// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

#ifndef SDUI_H
#define SDUI_H

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2013  William B. Ackerman.
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
//
//    This is for version 38.

#include <stdio.h>
#include <string.h>


#include "database.h"
#include "sdbase.h"


enum {
   // Probability (out of 8) that a concept will be placed on a randomly generated call.
   CONCEPT_PROBABILITY = 2,
   // We use lots more concepts for "standardize", since it is much less likely (though
   // by no means impossible) that a plain call will do the job.
   STANDARDIZE_CONCEPT_PROBABILITY = 6
};

// Actually, we don't make a resolve bigger than 3.  This is how much space
// we allocate for things.  Just being careful.
enum { MAX_RESOLVE_SIZE = 5 };

/* The Sd program reads this binary file for its calls database */
#ifndef DATABASE_FILENAME
//#define DATABASE_FILENAME "sd_calls2.dat"  // FIX FIX FIX
#define DATABASE_FILENAME "/Users/mpogue/clean3/SquareDesk/build-SquareDesk-Desktop_Qt_5_9_3_clang_64bit-Debug/test123/Install/SquareDeskPlayer.app/Contents/MacOS/sd_calls.dat"
#endif

/* The source form of the calls database.  The mkcalls program compiles it. */
#ifndef CALLS_FILENAME
#define CALLS_FILENAME "sd_calls.txt"
#endif

/* The output filename prefix.  ".level" is added to the name. */
#ifndef SEQUENCE_FILENAME
#define SEQUENCE_FILENAME "sequence"
#endif

/* The file containing the user's current working sessions. */
#ifndef SESSION_FILENAME
#define SESSION_FILENAME "sd.ini"
#endif

/* The temporary file used when rewriting the above. */
#ifndef SESSION2_FILENAME
#define SESSION2_FILENAME "sd2.ini"
#endif


// This has to be 80 more than you might think, because we append system
// error codes to what this would otherwise be, and to keep GCC's extremely
// picky warning checking happy, we subtract 80 before letting the system
// library append an error message to it.
#define MAX_FILENAME_LENGTH 440
#define INPUT_TEXTLINE_SIZE 400
// Absolute maximum length we can handle in text operations, including
// writing to file.  If a call gets more complicated than this, stuff
// will simply not be written to the file.  Too bad.
#define MAX_TEXT_LINE_LENGTH 300

// INPUT_TEXTLINE_SIZE must be >= MAX_TEXT_LINE_LENGTH, or calls to copy_to_user_input could fail.

// "max_print_length", in the ui-options class, is the length at which we will wrap a line
// when printing.  It defaults to 59 (for printing in 14-point Courier on 8.5 by 11 paper),
// but can be overridden by the user at startup.


// Codes for special accelerator keystrokes.

// Function keys can be plain, shifted, control, alt, or control-alt.

enum {
   FKEY = 128,
   SFKEY = 144,
   CFKEY = 160,
   AFKEY = 176,
   CAFKEY = 192,

   // "Enhanced" keys can be plain, shifted, control, alt, or control-alt.
   // e1 = page up
   // e2 = page down
   // e3 = end
   // e4 = home
   // e5 = left arrow
   // e6 = up arrow
   // e7 = right arrow
   // e8 = down arrow
   // e13 = insert
   // e14 = delete

   EKEY = 208,
   SEKEY = 224,
   CEKEY = 240,
   AEKEY = 256,
   CAEKEY = 272,

   // Digits can be control, alt, or control-alt.

   CTLDIG = 288,
   ALTDIG = 298,
   CTLALTDIG = 308,

   // Numeric keypad can be control, alt, or control-alt.

   CTLNKP = 318,
   ALTNKP = 328,
   CTLALTNKP = 338,

   // Letters can be control, alt, or control-alt.

   CTLLET = (348-'A'),
   ALTLET = (374-'A'),
   CTLALTLET = (400-'A'),

   CLOSEPROGRAMKEY = 426,

   FCN_KEY_TAB_LOW = (FKEY+1),
   FCN_KEY_TAB_LAST = CLOSEPROGRAMKEY
};

// This allows numbers from 0 to 36, inclusive.
enum {
   NUM_CARDINALS = 37
};


enum mode_kind {
   mode_none,     /* Not a real mode; used only for fictional purposes
                        in the user interface; never appears in the rest of the program. */
   mode_normal,
   mode_startup,
   mode_resolve
};

struct abbrev_block {
   Cstring key;
   modifier_block value;
   abbrev_block *next;
};

enum {
    special_index_lineup = -1,
    special_index_linedown = -2,
    special_index_pageup = -3,
    special_index_pagedown = -4,
    special_index_deleteline = -5,
    special_index_deleteword = -6,
    special_index_quote_anything = -7,
    special_index_copytext = -8,
    special_index_cuttext = -9,
    special_index_pastetext = -10
};


// BEWARE!!  Order is important.  Various comparisons are made.
enum abridge_mode_t {
   abridge_mode_none,             // Running, no abridgement.
   abridge_mode_deleting_abridge, // Like the above, but user has explicitly
                                  // requested that the abridgement specified for
                                  // the current session be removed.
   abridge_mode_abridging,        // Running, with abridgement list.
   abridge_mode_writing_only,     // Just writing the list; don't run the program itself.
   abridge_mode_writing_full      // Same, but write all lower lists as well.
};

class SDLIB_API index_list {
public:
   short int *the_list;
   int the_list_allocation;
   int the_list_size;

   index_list() : the_list((short int *) 0),
                  the_list_allocation(0),
                  the_list_size(0) {}

   void add_one(int datum)
   {
      if (the_list_size >= the_list_allocation) {
         the_list_allocation = the_list_allocation*2 + 5;
         short int *new_list = new short int[the_list_allocation];
         if (the_list) {
            memcpy(new_list, the_list, the_list_size*sizeof(short int));
            delete [] the_list;
         }
         the_list = new_list;
      }

      the_list[the_list_size++] = datum;
   }
};



enum color_scheme_type {
   color_by_gender,
   no_color,
   color_by_couple,
   color_by_corner,
   color_by_couple_rgyb,
   color_by_couple_ygrb,
   color_by_couple_random
};

class ui_option_type {
 public:
   color_scheme_type color_scheme;
   int force_session;
   int sequence_num_override;
   int no_graphics;       // 1 = "no_checkers"; 2 = "no_graphics"
   bool no_c3x;
   bool no_intensify;
   bool reverse_video;    // T = white-on-black; F = black-on-white.
   bool pastel_color;     // T = use pastel red/blue for color by gender.
                          // F = bold colors.  Color by couple or color by corner
                          // are always done with bold colors.
   bool use_magenta;      // These two override the above on a case-by-case basis.
   bool use_cyan;
   bool singlespace_mode;
   bool nowarn_mode;
   bool keep_all_pictures;
   bool accept_single_click;
   bool hide_glyph_numbers;
   bool diagnostic_mode;
   bool no_sound;
   bool tab_changes_focus;

   // This is the line length beyond which we will take pity on
   // whatever device has to print the result, and break the text line.
   // It is presumably smaller than our internal text capacity.
   // It is an observed fact that, with the default font (14 point Courier),
   // one can print 66 characters on 8.5 x 11 inch paper.
   // This is 59 by default.
   int max_print_length;

   // This is for the hidden command-line switch "resolve_test <N>".  Any
   // nonzero argument will seed the random number generator with that value,
   // thereby making all search operations deterministic.  (The random number
   // generator is normally seeded with the clock, of course.)
   //
   // Also, if the number is positive, it makes all search operations fail, and
   // sets the timeout to that many minutes.  This can be used for testing for
   // crashes in the resolve searcher.  Give an argument of 60, for example, and
   // any search command ("resolve", "pick random call", etc.) will generate
   // random solutions for an hour, rejecting them all.  Doing this on multiple
   // processors, with slightly different arguments, will run separate
   // deterministic tests on each processor.
   int resolve_test_minutes;

   int singing_call_mode;

   // This gets set if a user interface (e.g. sdui-tty/sdui-win) wants escape sequences
   // for drawing people, so that it can fill in funny characters, or draw in color.
   // This applies only to calls to add_new_line with a nonzero second argument.
   //
   // 0 means don't use any funny stuff.  The text strings transmitted when drawing
   // setups are completely plain ASCII.
   //
   // 1 means use escapes for the people themselves (13 octal followed by a byte of
   // person identifier followed by a byte of direction) but don't use the special
   // spacing characters.  All spacing and formatting is done with spaces.
   //
   // 2 means use escapes and other special characters.  Whenever the second arg to
   // add_new_line is nonzero, then in addition to the escape sequences for the
   // people themselves, we have an escape sequence for a phantom, and certain
   // characters have special meaning:  5 means space 1/2 of a glyph width, etc.
   // See the definition of newline for details.
   int use_escapes_for_drawing_people;

   // These could get changed if the user requests special naming.  See "alternate_glyphs_1"
   // in the command-line switch parser in sdsi.cpp.
   const char *pn1;       // 1st char (1/2/3/4) of what we use to print person.
   const char *pn2;       // 2nd char (B/G) of what we use to print person.
   const char *direc;     // 3rd char (direction arrow) of what we use to print person.
   const char *stddirec;  // the "standard" directions, for transcript files.
                          // Doesn't get overridden by any options.
   int squeeze_this_newline;  // randomly used by printing stuff.
   int drawing_picture;       // randomly used by printing stuff.

   ui_option_type();      // Constructor is in sdmain.
};



struct parse_stack_item {
   parse_block **concept_write_save_ptr;
   concept_kind save_concept_kind;
};

struct parse_state_type {
   parse_stack_item parse_stack[40];
   int parse_stack_index;
   parse_block **concept_write_ptr;
   parse_block **concept_write_base;
   char specialprompt[MAX_TEXT_LINE_LENGTH];
   uint32 topcallflags1;
   call_list_kind call_list_to_use;
   call_list_kind base_call_list_to_use;
};


struct pat2_block {
   Cstring car;
   pat2_block *cdr;
   const concept_descriptor *special_concept;
   match_result *folks_to_restore;
   bool demand_a_call;
   bool anythingers;

   pat2_block(Cstring carstuff, pat2_block *cdrstuff = (pat2_block *) 0) :
      car(carstuff),
      cdr(cdrstuff),
      special_concept((concept_descriptor *) 0),
      folks_to_restore((match_result *) 0),
      demand_a_call(false),
      anythingers(false)
   {}
};


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

   // This class is a singleton object.  It gets effectively recreated for every parse,
   // by initialize_for_parse, except for these two program-static items, which get initialized
   // just once and get manipulated through the life of the program.

   matcher_class() : s_modifier_active_list((modifier_block *) 0),
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

      ::memset(m_fcn_key_table_normal, 0,
               sizeof(modifier_block *) * (FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1));
      ::memset(m_fcn_key_table_start, 0,
               sizeof(modifier_block *) * (FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1));
      ::memset(m_fcn_key_table_resolve, 0,
               sizeof(modifier_block *) * (FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1));
   }

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

   modifier_block *m_fcn_key_table_normal[FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1];
   modifier_block *m_fcn_key_table_start[FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1];
   modifier_block *m_fcn_key_table_resolve[FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1];

   abbrev_block *m_abbrev_table_normal;
   abbrev_block *m_abbrev_table_start;
   abbrev_block *m_abbrev_table_resolve;
};


extern SDLIB_API parse_state_type parse_state;                      /* in SDTOP */
extern SDLIB_API ui_option_type ui_options;                         /* in SDTOP */

extern SDLIB_API int *color_index_list;                             /* in SDINIT */
extern SDLIB_API int color_randomizer[4];                           /* in SDINIT */

extern SDLIB_API const concept_kind constant_with_concept_diagnose; /* in SDTOP */
extern SDLIB_API const concept_kind constant_with_marker_end_of_list;/* in SDTOP */
extern SDLIB_API int last_direction_kind;                           /* in SDTOP */
extern SDLIB_API char database_version[81];                         /* in SDTOP */
extern SDLIB_API bool testing_fidelity;                             /* in SDTOP */
extern SDLIB_API bool allowing_minigrand;                           /* in SDTOP */
extern SDLIB_API bool allow_bend_home_getout;                       /* in SDTOP */
extern SDLIB_API call_conc_option_state verify_options;             /* in SDTOP */
extern SDLIB_API bool verify_used_number;                           /* in SDTOP */
extern SDLIB_API bool verify_used_direction;                        /* in SDTOP */
extern SDLIB_API bool verify_used_selector;                         /* in SDTOP */


struct comment_block {
   char txt[MAX_TEXT_LINE_LENGTH];
   comment_block *nxt;
};





// A few accessors to let the UI stuff survive.  They are implemented, for now, in SDTOP.
extern SDLIB_API uint32 get_concparseflags(const concept_descriptor *foo);
extern SDLIB_API const char *get_call_name(const call_with_name *foo);
extern SDLIB_API const char *get_call_menu_name(const call_with_name *foo);
extern SDLIB_API dance_level get_concept_level(const concept_descriptor *foo);
extern SDLIB_API Cstring get_concept_name(const concept_descriptor *foo);
extern SDLIB_API Cstring get_concept_menu_name(const concept_descriptor *foo);
extern SDLIB_API concept_kind get_concept_kind(const concept_descriptor *foo);
extern SDLIB_API const concept_descriptor *access_concept_descriptor_table(int i);
extern SDLIB_API bool get_yield_if_ambiguous_flag(call_with_name *foo);
extern SDLIB_API call_with_name *access_base_calls(int i);

void set_parse_block_concept(parse_block *p, const concept_descriptor *concept);
void set_parse_block_next(parse_block *p, parse_block *next);
void set_parse_block_call(parse_block *p, call_with_name *call);
void set_parse_block_call_to_print(parse_block *p, call_with_name *call);
void set_parse_block_replacement_key(parse_block *p, short int key);
parse_block **get_parse_block_subsidiary_root_addr(parse_block *p);

// Well, these are more than just accessors.
warning_info config_save_warnings();
void config_restore_warnings(const warning_info & rhs);


extern selector_kind selector_for_initialize;                       /* in SDINIT */
extern direction_kind direction_for_initialize;                     /* in SDINIT */
extern int number_for_initialize;                                   /* in SDINIT */

/* in SDUTIL */
void release_parse_blocks_to_mark(parse_block *mark_point);


/* in SDUTIL */
SDLIB_API const char *get_escape_string(char c);
SDLIB_API extern void save_parse_state();
SDLIB_API extern void restore_parse_state();


/* in SDCTABLE */

extern SDLIB_API modifier_block *fcn_key_table_normal[FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1];
extern SDLIB_API modifier_block *fcn_key_table_start[FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1];
extern SDLIB_API modifier_block *fcn_key_table_resolve[FCN_KEY_TAB_LAST-FCN_KEY_TAB_LOW+1];
extern SDLIB_API abbrev_block *abbrev_table_normal;
extern SDLIB_API abbrev_block *abbrev_table_start;
extern SDLIB_API abbrev_block *abbrev_table_resolve;

extern SDLIB_API command_kind search_goal;                          /* in SDPICK */

extern SDLIB_API Cstring menu_names[];                              /* in SDMAIN */
extern SDLIB_API command_list_menu_item command_menu[];             /* in SDMAIN */
extern SDLIB_API resolve_list_menu_item resolve_menu[];             /* in SDMAIN */
extern SDLIB_API startup_list_menu_item startup_menu[];             /* in SDMAIN */
extern int last_file_position;                                      /* in SDMAIN */
extern SDLIB_API const char *sd_version_string();                   /* In SDMAIN */
extern SDLIB_API bool query_for_call();                             /* In SDMAIN */

extern int sdtty_screen_height;                                     /* in SDUI-TTY */
extern bool sdtty_no_console;                                       /* in SDUI-TTY */
extern bool sdtty_no_line_delete;                                   /* in SDUI-TTY */


extern SDLIB_API bool showing_has_stopped;                    // in SDMATCH
extern SDLIB_API bool GLOB_doing_frequency;                   // in SDMATCH
extern SDLIB_API char GLOB_stats_filename[MAX_TEXT_LINE_LENGTH];   // in SDMATCH
extern SDLIB_API char GLOB_decorated_stats_filename[MAX_TEXT_LINE_LENGTH];   // in SDMATCH

extern SDLIB_API int session_index;                           // in SDSI
extern SDLIB_API bool rewrite_with_new_style_filename;        // in SDSI
extern SDLIB_API int random_number;                           // in SDSI
extern SDLIB_API const char *database_filename;               // in SDSI
extern SDLIB_API const char *new_outfile_string;              // in SDSI
extern SDLIB_API char abridge_filename[MAX_TEXT_LINE_LENGTH]; // in SDSI

extern SDLIB_API bool wrote_a_sequence;                             /* in SDUTIL */
extern SDLIB_API int sequence_number;                               /* in SDUTIL */
extern SDLIB_API int starting_sequence_number;                      /* in SDUTIL */
extern SDLIB_API const Cstring old_filename_strings[];              /* in SDUTIL */
extern SDLIB_API const Cstring new_filename_strings[];              /* in SDUTIL */
extern SDLIB_API const Cstring *filename_strings;                   /* in SDUTIL */
extern SDLIB_API char outfile_string[MAX_FILENAME_LENGTH];          /* in SDUTIL */
extern SDLIB_API char outfile_prefix[MAX_FILENAME_LENGTH];          /* in SDUTIL */
extern SDLIB_API char header_comment[MAX_TEXT_LINE_LENGTH];         /* in SDUTIL */
extern SDLIB_API bool creating_new_session;                         /* in SDUTIL */

extern SDLIB_API int text_line_count;                               /* in SDTOP */
extern SDLIB_API bool there_is_a_call;                              /* in SDTOP */
extern SDLIB_API int no_erase_before_this;                          /* in SDTOP */
extern SDLIB_API int written_history_nopic;                         /* in SDTOP */
extern SDLIB_API uint32 the_topcallflags;                           /* in SDTOP */

/* In SDTOP */

SDLIB_API bool deposit_call_tree(modifier_block *anythings, parse_block *save1, int key);

/* In SDGETOUT */
SDLIB_API void create_resolve_menu_title(
   command_kind goal,
   int cur,
   int max,
   resolver_display_state state,
   char *title);


/* In SDMATCH */

SDLIB_API bool process_accel_or_abbrev(modifier_block & mb, char linebuff[]);
SDLIB_API void erase_matcher_input();
SDLIB_API int delete_matcher_word();
void matcher_initialize();
SDLIB_API void matcher_setup_call_menu(call_list_kind cl);

extern SDLIB_API interactivity_state interactivity;                 /* in SDTOP */
extern SDLIB_API bool enable_file_writing;                          /* in SDTOP */
extern SDLIB_API Cstring cardinals[NUM_CARDINALS+1];                /* in SDTOP */
extern SDLIB_API Cstring ordinals[NUM_CARDINALS+1];                 /* in SDTOP */
extern SDLIB_API Cstring getout_strings[];                          /* in SDTOP */
extern SDLIB_API command_kind *command_command_values;              /* in SDTOP */
extern SDLIB_API int num_startup_commands;                          /* in SDTOP */
extern SDLIB_API Cstring *startup_commands;                         /* in SDTOP */
extern SDLIB_API start_select_kind *startup_command_values;         /* in SDTOP */
extern SDLIB_API int number_of_resolve_commands;                    /* in SDTOP */
extern SDLIB_API Cstring* resolve_command_strings;                  /* in SDTOP */
extern SDLIB_API resolve_command_kind *resolve_command_values;      /* in SDTOP */
extern SDLIB_API abridge_mode_t glob_abridge_mode;                  /* in SDTOP */

/* In SDINIT */
SDLIB_API bool parse_level(Cstring s, Cstring *break_ptr = 0);
SDLIB_API bool iterate_over_sel_dir_num(
   bool enable_selector_iteration,
   bool enable_direction_iteration,
   bool enable_number_iteration);
SDLIB_API void start_sel_dir_num_iterator();
SDLIB_API bool install_outfile_string(const char newstring[]);
SDLIB_API bool get_first_session_line();
SDLIB_API bool get_next_session_line(char *dest);
SDLIB_API void prepare_to_read_menus();
SDLIB_API int process_session_info(Cstring *error_msg);
SDLIB_API void close_init_file();
SDLIB_API void general_final_exit(int code);
SDLIB_API void start_stats_file_from_GLOB_stats_filename();
SDLIB_API bool open_session(int argc, char **argv);
SDLIB_API void close_session();

/* in SDMAIN */
SDLIB_API bool deposit_call(call_with_name *call, const call_conc_option_state *options);
SDLIB_API bool deposit_concept(const concept_descriptor *conc);

/* In SDTOP */
SDLIB_API void toplevelmove() THROW_DECL;
SDLIB_API void finish_toplevelmove() THROW_DECL;

/* In SDUI */

// Change the title bar (or whatever it's called) on the window.
extern void ttu_set_window_title(const char *string);

// Initialize this package.
extern void ttu_initialize();

// The opposite.
extern void ttu_terminate();

// Get number of lines to use for "more" processing.  This number is
// not used for any other purpose -- the rest of the program is not concerned
// with the "screen" size.

extern int get_lines_for_more();

// Return true for 'tty-like' devices which don't require 'more' processing;
// i.e. they have an unlimited scrollback buffer.
extern bool ttu_unlimited_scrollback();

// Clear the current line, leave cursor at left edge.
extern void clear_line();

// Backspace the cursor and clear the rest of the line, presumably
// erasing the last character.
extern void rubout();

// Move cursor up "n" lines and then clear rest of screen.
extern void erase_last_n(int n);

// Write a line.  The text may or may not have a newline at the end.
// This may or may not be after a prompt and/or echoed user input.
// This is in sdui-wincon.cpp only.  It is used in sdtty only, not sd.
extern void put_line(const char the_line[]);

// Write a single character on the current output line.
extern void put_char(int c);

/* Get one character from input, no echo, no waiting for <newline>.
   Return large number for function keys and alt alphabetics:
      128+N for plain function key         (F1 = 129)
      144+N for shifted                   (sF1 = 145)
      160+N for control                   (cF1 = 161)
      176+N for alt function key          (aF1 = 177)
      192+N for control-alt function key (caF1 = 193)
      348..373 for ctl letter             (c-A = 348)
      374..399 for alt letter             (a-A = 374)
      400..425 for ctl-alt letter        (ca-A = 400) */

extern int get_char();

/* Get string from input, up to <newline>, with echoing and editing.
   Return it without the final <newline>. */
extern void get_string(char *dest, int max);

/* Ring the bell, or whatever. */
extern void ttu_bell();

extern void refresh_input();

/* in SDSI */

extern void general_initialize();
SDLIB_API int generate_random_number(int modulus);
SDLIB_API void hash_nonrandom_number(int number);
extern char *get_errstring();

/* in SDMAIN */
SDLIB_API int sdmain(int argc, char *argv[], iobase & ggg);

#endif   /* SDUI_H */
