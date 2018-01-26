// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

#ifndef SDBASE_H
#define SDBASE_H

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

// Figure out how to do dll linkage.  If the source file that includes this
// had "SDLIB_EXPORTS" set (which it will if it's a file for sdlib.dll),
// make symbols "export" type.  Otherwise, make them "import" type.
// Unless this isn't for the WIN32 API at all, in which case make the
// "SDLIB_API" symbol do nothing.

//#if defined(WIN32)
//#if defined(SDLIB_EXPORTS)
//#define SDLIB_API __declspec(dllexport)
//#else
//#define SDLIB_API __declspec(dllimport)
//#endif
//#else
#define SDLIB_API
//#endif

#define THROW_DECL throw(error_flag_type)

// Not so!  Throw is now in effect.
//
// It seems that proper handling of throw clauses is just too hard to
// implement.  GCC does it, and presumably does an excellent job of
// diagnosing programs that violate throw-correctness.  However, the
// program runs something like 13 times slower.  This is unacceptable.
// So we turn it off for production builds, and use throws in the dumb
// setjmp-like way that everyone else does.  Too bad.  They were a nice
// idea.  Microsoft doesn't even try, and raises a warning (How thoughtful!
// See flaming below.) if we use them.

#if defined(_MSC_VER)
// Microsoft can't be bothered to support C++ exception declarations
// properly, but this pragma at least makes the compiler not complain.
// Geez!  If I wanted to use compilers that whine and wail
// piteously about the fact that I'm actually using the language, I'd
// use the HP-UX compilers!  They did a wonderful job of pointing out,
// in warning messages, that function prototypes are an ANSI C feature.
// As though this dangerous fact (that I was using ANSI C, of all things)
// were something that I didn't know and needed to to warned about.
// The HP-UX compilers came into the 20th century in the nick of time.
// It's now the 21st.
#pragma warning(disable: 4290)
#endif

// We customize the necessary declaration for functions that don't return.
#if defined(__GNUC__)
#define NORETURN2 __attribute__ ((noreturn))
#else
#define NORETURN2
#endif



struct small_setup;
struct setup;
class parse_block;
struct call_conc_option_state;
struct call_with_name;
struct resolve_tester;
class matcher_class;
class iobase;


enum { MAX_ERR_LENGTH = 200 };


// BEWARE!!  This list must track the array "concept_table" in sdconcpt.cpp .
enum concept_kind {

   // These next few are not concepts.  Their appearance marks the end of a parse tree.

   concept_another_call_next_mod,         // calla modified by callb
   concept_mod_declined,                  // user was queried about modification, and said no.
   marker_end_of_list,                    // normal case

   // This is not a concept.  Its appearance indicates a comment is to be placed here.

   concept_comment,

   // Everything after this is a real concept.

   concept_concentric,
   concept_tandem,
   concept_some_are_tandem,
   concept_frac_tandem,
   concept_some_are_frac_tandem,
   concept_gruesome_tandem,
   concept_gruesome_frac_tandem,
   concept_tandem_in_setup,
   concept_checkerboard,
   concept_sel_checkerboard,
   concept_anchor,
   concept_reverse,
   concept_left,
   concept_grand,
   concept_magic,
   concept_cross,
   concept_single,
   concept_singlefile,
   concept_interlocked,
   concept_yoyo,
   concept_generous,
   concept_stingy,
   concept_fractal,
   concept_fast,
   concept_straight,
   concept_twisted,
   concept_rewind,
   concept_12_matrix,
   concept_16_matrix,
   concept_revert,
   concept_1x2,
   concept_2x1,
   concept_2x2,
   concept_1x3,
   concept_3x1,
   concept_3x0,
   concept_0x3,
   concept_4x0,
   concept_0x4,
   concept_6x2,
   concept_3x2,
   concept_3x3,
   concept_4x4,
   concept_5x5,
   concept_6x6,
   concept_7x7,
   concept_8x8,
   concept_create_matrix,
   concept_two_faced,
   concept_funny,
   concept_randomtrngl,
   concept_selbasedtrngl,
   concept_split,
   concept_each_1x4,
   concept_diamond,
   concept_triangle,
   concept_leadtriangle,
   concept_do_both_boxes,
   concept_once_removed,
   concept_do_phantom_2x2,
   concept_do_blocks_2x2,
   concept_do_phantom_boxes,
   concept_do_phantom_diamonds,
   concept_do_phantom_2x4,
   concept_do_phantom_stag_qtg,
   concept_do_phantom_diag_qtg,
   concept_do_twinphantom_diamonds,
   concept_do_divided_bones,
   concept_distorted,
   concept_single_diagonal,
   concept_double_diagonal,
   concept_parallelogram,
   concept_multiple_lines,
   concept_multiple_lines_tog,
   concept_multiple_lines_tog_std,
   concept_triple_1x8_tog,
   concept_multiple_boxes,
   concept_quad_boxes_together,
   concept_triple_boxes_together,
   concept_offset_triple_boxes_tog,
   concept_multiple_diamonds,
   concept_multiple_formations,
   concept_triple_diamonds_together,
   concept_quad_diamonds_together,
   concept_triangular_boxes,
   concept_in_out_std,
   concept_in_out_nostd,
   concept_triple_diag,
   concept_triple_diag_together,
   concept_triple_twin,
   concept_triple_twin_nomystic,
   concept_misc_distort,
   concept_misc_distort_matrix,
   concept_old_stretch,
   concept_new_stretch,
   concept_assume_waves,
   concept_active_phantoms,
   concept_mirror,
   concept_central,
   concept_snag_mystic,
   concept_crazy,
   concept_frac_crazy,
   concept_dbl_frac_crazy,
   concept_phan_crazy,
   concept_frac_phan_crazy,
   concept_fan,
   concept_c1_phantom,
   concept_grand_working,
   concept_centers_or_ends,
   concept_so_and_so_only,
   concept_some_vs_others,
   concept_same_sex_disconnected,
   concept_stable,
   concept_so_and_so_stable,
   concept_frac_stable,
   concept_so_and_so_frac_stable,
   concept_paranoid,
   concept_so_and_so_paranoid,
   concept_nose,
   concept_so_and_so_nose,
   concept_emulate,
   concept_mimic,
   concept_standard,
   concept_matrix,
   concept_double_offset,
   concept_checkpoint,
   concept_on_your_own,
   concept_trace,
   concept_move_in_and,
   concept_outeracting,
   concept_ferris,
   concept_overlapped_diamond,
   concept_all_8,
   concept_centers_and_ends,
   concept_mini_but_o,
   concept_n_times_const,
   concept_n_times,
   concept_sequential,
   concept_special_sequential,
   concept_special_sequential_num,
   concept_special_sequential_4num,
   concept_meta,
   concept_meta_one_arg,
   concept_meta_two_args,
   concept_so_and_so_begin,
   concept_replace_nth_part,
   concept_replace_last_part,
   concept_interrupt_at_fraction,
   concept_sandwich,
   concept_interlace,
   concept_fractional,
   concept_rigger,
   concept_wing,
   concept_common_spot,
   concept_drag,
   concept_dblbent,
   concept_omit,
   concept_supercall,
   concept_diagnose
};


// These enumerate the "useful" concepts -- concepts that we will automatically
// generate for the "normalize" command or when we see something like
// "switch to an interlocked diamond".  The order of these is not important,
// though they generally follow the order of the concepts in the main table.
//
// The concepts in the main table ("unsealed_concept_descriptor_table") have
// one of these in their "useful" field to register themselves as available
// for service as a "useful" concept.

enum useful_concept_enum {
  UC_none,
  UC_spl,
  UC_ipl,
  UC_pl,
  UC_pl8,
  UC_pl6,
  UC_tl,
  UC_tlwt,
  UC_tlwa,
  UC_tlwf,
  UC_tlwb,
  UC_trtl,
  UC_qlwt,
  UC_qlwa,
  UC_qlwf,
  UC_qlwb,
  UC_spw,
  UC_ipw,
  UC_pw,
  UC_spc,
  UC_ipc,
  UC_pc,
  UC_pc8,
  UC_pc6,
  UC_tc,
  UC_tcwt,
  UC_tcwa,
  UC_tcwr,
  UC_tcwl,
  UC_trtc,
  UC_qcwt,
  UC_qcwa,
  UC_qcwr,
  UC_qcwl,
  UC_spb,
  UC_ipb,
  UC_pb,
  UC_tb,
  UC_tbwt,
  UC_tbwa,
  UC_tbwf,
  UC_tbwb,
  UC_tbwr,
  UC_tbwl,
  UC_qbwt,
  UC_qbwa,
  UC_qbwf,
  UC_qbwb,
  UC_qbwr,
  UC_qbwl,
  UC_spd,
  UC_ipd,
  UC_pd,
  UC_spds,
  UC_ipds,
  UC_pds,
  UC_td,
  UC_tdwt,
  UC_qd,
  UC_qdwt,
  UC_sp1,
  UC_ip1,
  UC_p1,
  UC_sp3,
  UC_ip3,
  UC_p3,
  UC_spgt,
  UC_ipgt,
  UC_pgt,
  UC_cpl,
  UC_tnd,
  UC_cpl2s,
  UC_tnd2s,
  UC_pofl,
  UC_pofc,
  UC_pob,
  UC_magic,
  UC_pibl,
  UC_left,
  UC_cross,
  UC_grand,
  UC_intlk,
  UC_phan,
  UC_3x3,
  UC_4x4,
  UC_2x8matrix,
  UC_extent    // Not an item; indicates extent of the enum.
};


// In several places in this program, we use C-style static aggregate
// initializers.  Specifically, sdtables.cpp and sdctable.cpp are full of them.
// Some of them are absolutely enormous.  The C++ language sort of frowns on
// that style of initialization.  The C++ way would be to use constructors,
// writing the appropriate code to invoke the constructors in the proper way.
// While we have some sympathy with this style in many cases, we prefer the
// C-style direct initializers for most of the big tables.  The reason is that
// the benefits of C++ constructors seem infinitesimal for these tables, and
// they would sacrifice transparency: with the direct initializers, you can
// look at the struct definition in sd.h and look at the initializer in
// sdtables.cpp and know for sure, without worrying about what clever things
// the constructor might do, which constant belongs in which field.
//
// Because we are initializing things in ways that C++ frowns on, the
// C++ compiler punishes us by not letting us declare fields "const".
// This is unfortunate.  We attach great importance to the ability of
// the C/C++ type mechanism to prevent table overwrites.
//
// For a while, we solved this problem by putting the initializers in
// plain C files (sdtables.c and sdctable.c) and linking with the other
// C++ files.  Not surprisingly, this created difficulties.  In the end,
// it became unworkable, and now all files are C++.
//
// By the way, one of the problems was that the Intel compiler saw what
// we were doing in the C++ files, realized that we could not possibly
// initialize the structures legally, and gave a warning.  What it
// overlooked was that we were linking with C files to do what would
// have been impossible in C++.  For reference, the incantation to
// shut off the warning was:
//     #if defined(WIN32) && defined(__ICL)
//     #pragma warning(disable: 411)
//     #endif
//
// Because what we really want is the protection against overwriting
// that the "const" declaration would provide, we define the initialized
// tables, and the definition of their structure, in classes.  The tables
// themselves are private.  So that is why you see all these classes
// that have no members, but have statically initialized structs.


// We used to make this a struct inside the class, rather than having its
// fields just comprise the class itself (note that there are no
// fields in this class, and it is never instantiated) so that
// we can make "unsealed_concept_descriptor_table" be a statically initialized array.
//
// But it now has to be free-standing, so that sdui.h can use it.

struct concept_descriptor {
   Cstring name;
   concept_kind kind;
   uint32 concparseflags;   // See above.
   dance_level level;
   useful_concept_enum useful;
   uint32 arg1;
   uint32 arg2;
   uint32 arg3;
   uint32 arg4;
   uint32 arg5;
   mutable int frequency;  // For call/concept-use statistics.
   Cstring menu_name;
};


// These are in SDCTABLE.
extern SDLIB_API const concept_descriptor concept_centers_concept;
extern SDLIB_API const concept_descriptor concept_heads_concept;
extern SDLIB_API const concept_descriptor concept_sides_concept;
extern SDLIB_API const concept_descriptor concept_special_magic;
extern SDLIB_API const concept_descriptor concept_special_interlocked;
extern SDLIB_API const concept_descriptor concept_mark_end_of_list;
extern SDLIB_API const concept_descriptor concept_marker_decline;
extern SDLIB_API const concept_descriptor concept_marker_concept_mod;
extern SDLIB_API const concept_descriptor concept_marker_concept_comment;
extern SDLIB_API const concept_descriptor concept_marker_concept_supercall;
extern SDLIB_API const concept_descriptor concept_special_piecewise;
extern SDLIB_API const concept_descriptor concept_special_z;


// ****** It would be nice if we didn't need the warning stuff here.
// but things in sdmatch.cpp (like verify_call) need to save an actual warning_info object
// by copying it, so it needs to know how big it is.
// BEWARE!!  This list must track the array "warning_strings" in sdtables.cpp
enum warning_index {
   warn__none,
   warn__really_no_collision,
   warn__really_no_eachsetup,
   warn__do_your_part,
   warn__unusual_or_2faced,
   warn__tbonephantom,
   warn__awkward_centers,
   warn__overcast,
   warn__bad_concept_level,
   warn__not_funny,
   warn__hard_funny,
   warn__rear_back,
   warn__awful_rear_back,
   warn__excess_split,
   warn__lineconc_perp,
   warn__dmdconc_perp,
   warn__lineconc_par,
   warn__dmdconc_par,
   warn__xclineconc_perpc,
   warn__xcdmdconc_perpc,
   warn__xclineconc_perpe,
   warn__ctrstand_endscpls,
   warn__ctrscpls_endstand,
   warn__each2x2,
   warn__each1x4,
   warn__each1x2,
   warn__eachdmd,
   warn__take_right_hands,
   warn__1_4_pgram,
   warn__full_pgram,
   warn__3_4_pgram,
   warn__offset_gone,
   warn__overlap_gone,
   warn__to_o_spots,
   warn__to_x_spots,
   warn__check_butterfly,
   warn__check_galaxy,
   warn__some_rear_back,
   warn__centers_rear_back_staying_in_center,
   warn__not_tbone_person,
   warn__check_c1_phan,
   warn__check_dmd_qtag,
   warn__check_quad_dmds,
   warn__may_be_fudgy,
   warn__fudgy_half_offset,
   warn__check_3x4,
   warn__check_2x4,
   warn__check_hokey_2x4,
   warn__check_4x4,
   warn__check_hokey_4x5,
   warn__check_4x6,
   warn__check_hokey_4x4,
   warn__check_4x4_start,
   warn__check_4x4_ctrbox,
   warn__check_3dmd_is_wide,
   warn__check_centered_qtag,
   warn__check_pgram,
   warn__check_hokey_thar,
   warn__ctrs_stay_in_ctr,
   warn__meta_on_xconc,
   warn__check_c1_stars,
   warn__check_gen_c1_stars,
   warn__bigblock_feet,
   warn__bigblockqtag_feet,
   warn__diagqtag_feet,
   warn__adjust_to_feet,
   warn__some_touch,
   warn__some_touch_evil,
   warn__split_to_2x4s,
   warn__split_to_2x3s,
   warn__split_to_1x8s,
   warn__split_to_1x6s,
   warn__split_to_1x3s,
   warn__split_to_1x3s_always,
   warn__take_left_hands,
   warn__left_half_pass,
   warn__evil_interlocked,
   warn__split_phan_in_pgram,
   warn__bad_interlace_match,
   warn__not_on_block_spots,
   warn__stupid_phantom_clw,
   warn__should_say_Z,
   warn__bad_modifier_level,
   warn__bad_call_level,
   warn__did_not_interact,
   warn__use_quadruple_setup_instead,
   warn__opt_for_normal_cast,
   warn__opt_for_normal_hinge,
   warn__opt_for_2fl,
   warn__opt_for_no_collision,
   warn__opt_for_not_tboned_base,
   warn_partial_solomon,
   warn_same_z_shear,
   warn__like_linear_action,
   warn__phantoms_thinner,
   warn__hokey_jay_shapechanger,
   warn_interlocked_to_6,
   warn__offset_hard_to_see,
   warn__pg_hard_to_see,
   warn__phantom_judge,
   warn__colocated_once_rem,
   warn_big_outer_triangles,
   warn_hairy_fraction,
   warn_bad_collision,
   warn_very_bad_collision,
   warn_some_singlefile,
   warn__dyp_resolve_ok,
   warn__unusual,
   warn_controversial,
   warn_verycontroversial,
   warn_no_internal_phantoms,
   warn_serious_violation,
   warn__4_circ_tracks,
   warn__assume_dpt,
   warn_bogus_yoyo_rims_hubs,
   warn__centers_are_diamond,
   warn_pg_in_2x6,
   warn__deprecate_pg_3box,
   warn_real_people_spots,
   warn__tasteless_com_spot,
   warn__tasteless_junk,
   warn__tasteless_slide_thru,
   warn__went_to_other_side,
   warn__horrible_conc_hinge,
   warn__this_is_tight,
   warn__compress_carefully,
   warn__brute_force_mxn,
   warn__two_faced,
   warn__cant_track_phantoms,
   warn__did_weird_stretch_response,
   warn__crazy_tandem_interaction,
   warn__mimic_ambiguity_checked,
   warn__mimic_ambiguity_resolved,
   warn__diagnostic,
   warn__NUM_WARNINGS       // Not a real warning; just used for counting.
};

class warning_info {
 public:

   // Zero-arg constructor, clears the words.
   warning_info()
      { for (int i=0 ; i<WARNING_WORDS ; i++) bits[i] = 0; }

   bool operator != (const warning_info & rhs) const
      {
         for (int i=0 ; i<WARNING_WORDS ; i++) { if ((bits[i] != rhs.bits[i])) return true; }
         return false;
      }

   bool operator == (const warning_info & rhs) const
      {
         for (int i=0 ; i<WARNING_WORDS ; i++) { if ((bits[i] != rhs.bits[i])) return false; }
         return true;
      }

   void setbit(warning_index i)
      { bits[i>>5] |= 1 << (i & 0x1F); }

   void clearbit(warning_index i)
      { bits[i>>5] &= ~(1 << (i & 0x1F)); }

   bool testbit(warning_index i) const
      { return (bits[i>>5] & (1 << (i & 0x1F))) != 0; }

   bool testmultiple(const warning_info & rhs) const
      {
         for (int i=0 ; i<WARNING_WORDS ; i++) { if ((bits[i] & rhs.bits[i])) return true; }
         return false;
      }

   void setmultiple(const warning_info & rhs)
      { for (int i=0 ; i<WARNING_WORDS ; i++) bits[i] |= rhs.bits[i]; }

   void clearmultiple(const warning_info & rhs)
      { for (int i=0 ; i<WARNING_WORDS ; i++) bits[i] &= ~rhs.bits[i]; }

 private:
   enum { WARNING_WORDS = (warn__NUM_WARNINGS+31)>>5 };

   uint32 bits[WARNING_WORDS];
};


enum error_flag_type {
   error_flag_none = 0,      // Must be zero because setjmp returns this.
                             // (Of course, we haven't used setjmp since March, 2000.)
   error_flag_1_line,        // 1-line error message, text is in error_message1.
   error_flag_2_line,        // 2-line error message, text is in error_message1 and
                             // error_message2.
   error_flag_collision,     // collision error, message is that people collided, they are in
                             // collision_person1 and collision_person2.
   error_flag_cant_execute,  // unable-to-execute error, person is in collision_person1,
                             // text is in error_message1.

   // Errors after this can't be restarted by the mechanism that goes on to the
   // call's next definition when a call execution fails.
   // "Error_flag_no_retry" is the indicator for this.

   error_flag_no_retry,      // Like error_flag_1_line, but it is instantly fatal.

   error_flag_user_wants_to_resolve, // User typed "pick random call" while querying
                                     // for a subcall -- do a resolve.

   // Errors after this did not arise from call execution, so we don't
   // show the ending formation.  "Error_flag_wrong_command" is the indicator for this.

   error_flag_wrong_command,     // clicked on something inappropriate in subcall reader.
   error_flag_wrong_resolve_command, // "resolve" or similar command was called
                                     // in inappropriate context, text is in error_message1.
   error_flag_selector_changed,  // warn that selector was changed during clipboard paste.
   error_flag_formation_changed, // warn that formation changed during clipboard paste.
   error_flag_OK_but_dont_erase  // We have finished a "frequency" operation -- don't clear the screen.
};


// BEWARE!!  This list must track the array "selector_list" in sdtables.cpp
enum selector_kind {
   selector_uninitialized,
   selector_boys,
   selector_girls,
   selector_heads,
   selector_sides,
   selector_headcorners,
   selector_sidecorners,
   selector_headboys,
   selector_headgirls,
   selector_sideboys,
   selector_sidegirls,
   selector_centers,
   selector_ends,
   selector_leads,
   selector_trailers,
   selector_lead_beaus,
   selector_lead_belles,
   selector_lead_ends,
   selector_lead_ctrs,
   selector_trail_beaus,
   selector_trail_belles,
   selector_trail_ends,
   selector_trail_ctrs,
   selector_end_boys,
   selector_end_girls,
   selector_center_boys,
   selector_center_girls,
   selector_beaus,
   selector_belles,
   selector_center2,
   selector_verycenters,
   selector_center6,
   selector_outer2,
   selector_veryends,
   selector_outer6,
   selector_ctrdmd,
   selector_ctr_1x4,
   selector_ctr_1x6,
   selector_outer1x3s,
   selector_center4,
   selector_center_wave,
   selector_center_line,
   selector_center_col,
   selector_center_box,
   selector_center_wave_of_6,
   selector_center_line_of_6,
   selector_center_col_of_6,
   selector_outerpairs,
   selector_firstone,
   selector_lastone,
   selector_firsttwo,
   selector_lasttwo,
   selector_firstthree,
   selector_lastthree,
   selector_firstfour,
   selector_lastfour,
   selector_leftmostone,
   selector_rightmostone,
   selector_leftmosttwo,
   selector_rightmosttwo,
   selector_leftmostthree,
   selector_rightmostthree,
   selector_leftmostfour,
   selector_rightmostfour,
   selector_headliners,
   selector_sideliners,
   selector_thosefacing,
   selector_everyone,
   selector_all,
   selector_none,
   // Selectors below this point will not be used in random generation.
   // They are mostly unsymmetrical ones.
   noresolve_SELECTOR_START,    selector_nearline = noresolve_SELECTOR_START,
   selector_farline,
   selector_nearcolumn,
   selector_farcolumn,
   selector_nearbox,
   selector_farbox,
   selector_lineleft,
   selector_lineright,
   selector_columnleft,
   selector_columnright,
   selector_boxleft,
   selector_boxright,
   selector_nearfour,
   selector_farfour,
   selector_neartwo,
   selector_fartwo,
   selector_nearsix,
   selector_farsix,
   selector_nearthree,
   selector_farthree,
   selector_nearfive,
   selector_farfive,
   selector_the2x3,
   selector_thediamond,
   selector_theline,
   selector_thecolumn,
   selector_facingfront,
   selector_facingback,
   selector_facingleft,
   selector_facingright,
   selector_farthest1,
   selector_nearest1,
   selector_boy1,
   selector_girl1,
   selector_cpl1,
   selector_boy2,
   selector_girl2,
   selector_cpl2,
   selector_boy3,
   selector_girl3,
   selector_cpl3,
   selector_boy4,
   selector_girl4,
   selector_cpl4,
   selector_cpls1_2,
   selector_cpls2_3,
   selector_cpls3_4,
   selector_cpls4_1,
   // Start of selectors used only in "some are tandem" operations, with a control key of 'K'.
   selector_some,          selector_SOME_START = selector_some,
   selector_inside_tgl,
   selector_outside_tgl,
   selector_inpoint_tgl,
   selector_outpoint_tgl,
   // Start of invisible selectors.
   selector_mysticbeaus,   selector_INVISIBLE_START = selector_mysticbeaus,
   selector_mysticbelles,
   selector_notctrdmd,
   selector_ENUM_EXTENT   // Not a selector; indicates extent of the enum.
};

// BEWARE!!  This list must track the array "direction_names" in sdtables.cpp .
// Note also that the "zig-zag" items will get disabled below A2, and "the music" below C3A.
// The keys for this are "direction_zigzag" and "direction_the_music".
enum direction_kind {
   direction_uninitialized,
   direction_no_direction,
   direction_left,
   direction_right,
   direction_in,
   direction_out,
   direction_back,
   direction_zigzag,
   direction_zagzig,
   direction_zigzig,
   direction_zagzag,
   direction_the_music,
   direction_ENUM_EXTENT   // Not a direction; indicates extent of the enum.
};


struct resolve_indicator {

   // Without this next thing, we get this error in Visual C++ Professional,
   // version 5.0, service pack 3:
   //      sdgetout.cpp
   //      C:\wba\sd\sdgetout.cpp(542) : fatal error C1001: INTERNAL COMPILER ERROR
   //        (compiler file 'E:\utc\src\\P2\main.c', line 379)
   //          Please choose the Technical Support command on the Visual C++
   //          Help menu, or open the Technical Support help file for more information
   //
   // Isn't that cool?
   //
   // Of course, if one were to take it out now, the error probably wouldn't
   // happen, because many other things have changed, and the error has a
   // serious phase-of-the-moon dependency.  But I have saved the files
   // that can provoke it.  July 2002.

   float ICantBelieveWhatABunchOfDunderheadsTheyHaveAtMicrosoft;

   // We only look at the "k" field and the 0x40 bit of the distance
   // (which tells us not to accept it in any resolve search.)
   // Note in particular that the low bits of the distance do *not*
   // contain useful information, because it's just a table item --
   // It doesn't know anything about the current setup and the
   // actual person identities.
   const resolve_tester *the_item;
   // Use this instead.
   int distance;
};


struct startinfo {
   const char *name;
   bool into_the_middle;
   setup *the_setup_p;
};


// In each case, an integer or enum is deposited into the global_reply minor part.
// Its interpretation depends on which of the replies above was given.
// For some of the replies, it gives the index into a menu.  For
// "ui_start_select" it is a start_select_kind.  For other replies, it is one of
// the following constants:

/* BEWARE!!!!!!!!  Lots of implications for "centersp" and all that! */
/* BEWARE!!  If change this next definition, be sure to update the definition of
   "startinfolist" in sdinit.cpp, and also necessary stuff in the user interfaces. */
enum start_select_kind {
   start_select_exit,        /* Don't start a sequence; exit from the program. */
   start_select_h1p2p,       /* Start with Heads 1P2P. */
   start_select_s1p2p,       /* Etc. */
   start_select_heads_start,
   start_select_sides_start,
   start_select_as_they_are,    // End of items that are keyed to startinfolist.
   start_select_toggle_conc,
   start_select_toggle_singlespace,
   start_select_toggle_minigrand,
   start_select_toggle_bend_home,
   start_select_toggle_overflow_warn,
   start_select_toggle_act,
   start_select_toggle_retain,
   start_select_toggle_nowarn_mode,
   start_select_toggle_keepallpic_mode,
   start_select_toggle_singleclick_mode,
   start_select_toggle_singer,
   start_select_toggle_singer_backward,
   start_select_select_print_font,
   start_select_print_current,
   start_select_print_any,
   start_select_init_session_file,
   start_select_change_to_new_style_filename,
   start_select_randomize_couple_colors,
   start_select_change_outfile,
   start_select_change_outprefix,
   start_select_change_title,

   start_select_freq_show,
   start_select_freq_show_level,
   start_select_freq_show_nearlevel,
   start_select_freq_show_sort,
   start_select_freq_show_sort_level,
   start_select_freq_show_sort_nearlevel,
   start_select_freq_reset,
   start_select_freq_start,
   start_select_freq_delete,

   start_select_kind_enum_extent    // Not a start_select kind; indicates extent of the enum.
};


/* For ui_resolve_select: */
/* BEWARE!!  This list must track the array "resolve_resources" in sdui-x11.c . */
enum resolve_command_kind {
   resolve_command_abort,
   resolve_command_find_another,
   resolve_command_goto_next,
   resolve_command_goto_previous,
   resolve_command_accept,
   resolve_command_raise_rec_point,
   resolve_command_lower_rec_point,
   resolve_command_grow_rec_region,
   resolve_command_shrink_rec_region,
   resolve_command_write_this,
   resolve_command_kind_enum_extent    // Not a resolve kind; indicates extent of the enum.
};



/* BEWARE!!  There is a static initializer for this, "null_options", in sdtop.cpp
   that must be kept up to date. */
struct call_conc_option_state {
   selector_kind who;        /* selector, if any, used by concept or call */
   direction_kind where;     /* direction, if any, used by concept or call */
   uint32 tagger;            /* tagging call indices, if any, used by call.
                                If nonzero, this is 3 bits for the 0-based tagger class
                                and 5 bits for the 1-based tagger call */
   uint32 circcer;           /* circulating call index, if any, used by call */
   uint32 number_fields;     /* number, if any, used by concept or call */
   int howmanynumbers;       /* tells how many there are */
   int star_turn_option;     /* For calls with "@S" star turn stuff. */
};

parse_block *get_parse_block_mark();
parse_block *get_parse_block();

/* in SDPREDS */
extern bool selector_used;
extern bool direction_used;
extern bool number_used;
extern bool mandatory_call_used;

extern SDLIB_API int useful_concept_indices[UC_extent];             /* in SDINIT */



// These are the values returned in user_match.match.kind
// by "uims_get_call_command" and similar functions.
enum uims_reply_kind {
   ui_special_concept,  // Not a real return; used only for fictional purposes
                        //    in the user interface; never appears in the rest of the program.
   ui_command_select,   // (normal/resolve) User chose one of the special buttons
                        //    like "resolve" or "quit".
   ui_resolve_select,   // (resolve only) User chose one of the various actions
                        //    peculiar to resolving.
   ui_start_select,     // (startup only) User chose something.
                        //    This is the only outcome in startup mode.
   ui_concept_select,   // (normal only) User selected a concept.
   ui_call_select,      // (normal only) User selected a call from the current call menu.
   ui_user_cancel,      // (normal only) user canceled call entry (waved the mouse away, etc.)
   ui_help_simple,      // (any) user selected "help"
   ui_help_manual       // (any) user selected "help manual"
};

class uims_reply_thing {

public:

   uims_reply_thing(uims_reply_kind major, int16 minor) : majorpart((int16) major), minorpart(minor) {}

   uint16 majorpart;
   uint16 minorpart;
};

// BEWARE!!  There may be tables in the user interface file keyed to this enumeration.
// In particular, this list must track the array "menu_names" in sdtables.cpp
// BEWARE!!  This list may be keyed to some messy stuff in the procedure "initialize_concept_sublists".
// Changing these items is not recommended.

enum call_list_kind {
   call_list_none, call_list_empty, /* Not real call list kinds; used only for
                                       fictional purposes in the user interface;
				       never appear in the rest of the program. */
   call_list_any,                   /* This is the "universal" call list; used
                                       whenever the setup isn't one of the known ones. */
   call_list_1x8, call_list_l1x8,
   call_list_dpt, call_list_cdpt,
   call_list_rcol, call_list_lcol,
   call_list_8ch, call_list_tby,
   call_list_lin, call_list_lout,
   call_list_rwv, call_list_lwv,
   call_list_r2fl, call_list_l2fl,
   call_list_gcol, call_list_qtag,

   call_list_extent    // Not a start call_list kind; indicates extent of the enum.
};

struct modifier_block {
   uims_reply_kind kind;
   int32 index;
   call_conc_option_state call_conc_options;  /* Has numbers, selectors, etc. */
   call_with_name *call_ptr;
   const concept_descriptor *concept_ptr;
   modifier_block *packed_next_conc_or_subcall;  /* next concept, or, if this is end mark, points to substitution list */
   modifier_block *packed_secondary_subcall; // points to substitution list for secondary subcall
   modifier_block *gc_ptr;                /* used for reclaiming dead blocks */
};


/*
 * A match_result describes the result of matching a string against
 * a set of acceptable commands. A match_result is effectively a
 * sequence of values to be returned to the main program. A sequence
 * is required because the main program asks for information in
 * bits and pieces.  For example, the call "touch 1/4" first returns
 * "touch <N/4>" and then returns "1".  This structure for the
 * interaction between the main program and the UI reflects the
 * design of the original Domain Dialog UI.
 *
 */

struct match_result {
   bool valid;               // Set to true if a match was found.
   bool exact;               // Set to true if an exact match was found.
   bool indent;              // This is a subordinate call; indent it in listing.
   modifier_block match;     // The little thing we actually return.
   const match_result *real_next_subcall;
   const match_result *real_secondary_subcall;
   int recursion_depth;      // How deep in "@0" or "@m" things.
   int yield_depth;          // If nonzero, this yields by that amount.
};

enum interactivity_state {
   interactivity_database_init,
   interactivity_no_query_at_all,    /* Used when pasting from clipboard.  All subcalls,
                                        selectors, numbers, etc. must be filled in already.
                                        If not, it is a bug. */
   interactivity_verify,
   interactivity_normal,
   interactivity_picking
};

enum resolve_goodness_test {
   resolve_goodness_only_nice,
   resolve_goodness_always,
   resolve_goodness_maybe
};

struct selector_item {
   Cstring name;
   Cstring sing_name;
   Cstring name_uc;
   Cstring sing_name_uc;
   selector_kind opposite;
};

struct direction_item {
   Cstring name;
   Cstring name_uc;
};

void fail(const char s[]) THROW_DECL NORETURN2;

void fail_no_retry(const char s[]) THROW_DECL NORETURN2;

extern void fail2(const char s1[], const char s2[]) THROW_DECL NORETURN2;

extern void failp(uint32 id1, const char s[]) THROW_DECL NORETURN2;

void specialfail(const char s[]) THROW_DECL NORETURN2;

extern void warn(warning_index w);

int gcd(int a, int b);    // In sdmoves

void reduce_fraction(int &a, int &b);    // In sdmoves

// There are two different contexts in which we deal with collections of
// numbers.  In each case the numbers are packed into 6 bit fields, so they can
// theoretically go up to 63.  We could therefore handle up to 5 numbers, though
// we never do more than 4.
// Helpful hint:  when debugging, display the word in octal.
//
//  (1) The "number_fields" part of a call_conc_option_state.
//         This has the numbers (up to four of them) that the user entered
//         with a call or concept, as in "3/4 crazy" or "cast off 1/2".  Those
//         numbers are packed in right-to-left order.  So, for example, "circle
//         by 1/2 by 3/4" will have 0/0/3/2 in those fields (this call takes its
//         numeric arguments in quarters).  The "do the last 3/5" concept will
//         have 0/0/5/3 in those fields.  The parser will never put a number
//         larger than 36 (NUM_CARDINALS-1) in the "number_fields" word.
//
//  (2) The "fraction" field of a "fraction_command".  This also takes four
//         numbers, though there is no direct correspondence with the other
//         usage described above.  The four numbers are the numerators and
//         denominators of the starting and ending points of the desired part of
//         the call.  The layout is
//         <start den> / <start num> / <end den> / <end num>.  To do the whole
//         call, we use 1/0/1/1, which is the constant NUMBER_FIELDS_1_0_1_1 or
//         FRAC_FRAC_NULL_VALUE.

enum {
   BITS_PER_NUMBER_FIELD = 6,
   NUMBER_FIELD_MASK = (1<<BITS_PER_NUMBER_FIELD)-1,
   NUMBER_FIELD_MASK_SECOND_ONE = NUMBER_FIELD_MASK << (BITS_PER_NUMBER_FIELD*2),
   NUMBER_FIELD_MASK_RIGHT_TWO = (1<<(BITS_PER_NUMBER_FIELD*2))-1,
   NUMBER_FIELD_MASK_LEFT_TWO = NUMBER_FIELD_MASK_RIGHT_TWO << (BITS_PER_NUMBER_FIELD*2),
   NUMBER_FIELDS_1_0 = 00100U,          // A few useful canned values.
   NUMBER_FIELDS_2_1 = 00201U,
   NUMBER_FIELDS_1_1 = 00101U,
   NUMBER_FIELDS_0_0_1_1 = 000000101U,
   NUMBER_FIELDS_1_0_0_0 = 001000000U,
   NUMBER_FIELDS_1_0_1_1 = 001000101U,
   NUMBER_FIELDS_1_0_2_1 = 001000201U,
   NUMBER_FIELDS_1_0_4_0 = 001000400U,
   NUMBER_FIELDS_2_1_1_1 = 002010101U,
   NUMBER_FIELDS_2_1_2_1 = 002010201U,
   NUMBER_FIELDS_4_0_1_1 = 004000101U
};

// The following enumeration and struct encode the fraction/parts information
// for a call to be executed.

// This enumeration lists the things that go into the "fraction" word of a
// fraction_command.  The low 24 bits give the fractions values, in 4 6-bit fields,
// which are the numerators and denominators of the starting and ending points of the
// desired part of the call.  The layout is
//
//      <start den> / <start num> / <end den> / <end num>.
//
// To do the whole call, we use 1/0/1/1, which is the constant
// NUMBER_FIELDS_1_0_1_1 or FRAC_FRAC_NULL_VALUE.
// Helpful hint:  when debugging, display the word in octal.

enum fracfrac {
   // These refer to the "fraction" field.
   FRAC_FRAC_NULL_VALUE      = NUMBER_FIELDS_1_0_1_1,
   FRAC_FRAC_HALF_VALUE      = NUMBER_FIELDS_1_0_2_1,
   FRAC_FRAC_LASTHALF_VALUE  = NUMBER_FIELDS_2_1_1_1,

   // Special flags for the top of the "fraction" field.
   // Not to be confused with the "flags" field, which has most of the information.
   // There is actually room for 8 bits here, though we only use 2.
   CMD_FRAC_DEFER_LASTHALF_OF_FIRST   = 0x40000000,
   CMD_FRAC_DEFER_HALF_OF_LAST        = 0x80000000
};


// This enumeration lists the things that go into the "flags" word of a
// fraction_command.  This is largely flags and the "code" info, though there are also
// two 6-bit items at the bottom, called "N" and "K", that are associated with the
// codes.  We encode those fields in 6 bits, as usual.  They are independent of the
// four 6-bit fields in the "fraction" word.
//
// For the meaning of this, see the long block of comments before
// "process_fractions" in sdmoves.cpp.
// But note that much of that (the bit layout) is out of date.
// The documentation of the codes is correct, but the "N" and "K" fields
// are now 6 bits, in the "flags" word.

enum {
   CMD_FRAC_PART_BIT        = 00001U,  // This is "N".
   CMD_FRAC_PART_MASK       = 00077U,
   CMD_FRAC_PART2_BIT       = 00100U,  // This is "K".
   CMD_FRAC_PART2_MASK      = 07700U,

   CMD_FRAC_IMPROPER_BIT    = 0x00200000U,
   CMD_FRAC_THISISLAST      = 0x00400000U,
   CMD_FRAC_REVERSE         = 0x00800000U,
   CMD_FRAC_CODE_MASK       = 0x07000000U,    // This is a 3 bit field.

   // Here are the codes that can be inside.  We require that CMD_FRAC_CODE_ONLY be zero.
   // We require that the PART_MASK field be nonzero (we use 1-based part numbering)
   // when these are in use.  If the PART_MASK field is zero, the code must be zero
   // (that is, CMD_FRAC_CODE_ONLY), and this stuff is not in use.

   CMD_FRAC_CODE_ONLY           = 0x00000000U,
   CMD_FRAC_CODE_ONLYREV        = 0x01000000U,
   CMD_FRAC_CODE_FROMTO         = 0x02000000U,
   CMD_FRAC_CODE_FROMTOREV      = 0x03000000U,
   CMD_FRAC_CODE_FROMTOREVREV   = 0x04000000U,
   CMD_FRAC_CODE_FROMTOMOST     = 0x05000000U,
   CMD_FRAC_CODE_LATEFROMTOREV  = 0x06000000U,

   CMD_FRAC_BREAKING_UP     = 0x10000000U,
   CMD_FRAC_FORCE_VIS       = 0x20000000U,
   CMD_FRAC_LASTHALF_ALL    = 0x40000000U,
   CMD_FRAC_FIRSTHALF_ALL   = 0x80000000U
};

// The "flags" word has special information, like "do parts 5 through 2 in
// reverse order".  The "fraction" word has 4 numbers, encoding information like
// "do from 1/3 to 7/8".  The interaction of these things is quite complicated.
// See the comments in front of "get_fraction_info" in sdmoves.cpp for details
// about this.
//
// The default value ("do the whole call") is zero in the flags word and
// FRAC_FRAC_NULL_VALUE in the fraction word.  Note that FRAC_FRAC_NULL_VALUE is
// not zero.  Under normal circumstances, the fraction word is never zero,
// because it has fraction denominators.  There are a few special situations in
// which zero is stored in the fractions word.  For example, the
// "restrained_fraction" field of a command may have its fraction word zero.
// That means that the restrained fraction mechanism is turned off.

// Helpful macro for assembling the code and its two numeric arguments.
#define FRACS(code,n,k) (code|((n)*CMD_FRAC_PART_BIT)|((k)*CMD_FRAC_PART2_BIT))

// These control inversion of the start and end args.  That is, the position
// fraction F is turned into 1-F.
enum fraction_invert_flags {
   FRAC_INVERT_NONE = 0,
   FRAC_INVERT_START = 1,
   FRAC_INVERT_END = 2
};


struct fraction_command {
   uint32 flags;
   uint32 fraction;  // The fraction info, packed into 4 fields.

   enum includes_first_part_enum {
      yes,
      no,
      notsure};

   inline void set_to_null()     { flags = 0; fraction = FRAC_FRAC_NULL_VALUE; }
   inline void set_to_firsthalf(){ flags = 0; fraction = FRAC_FRAC_HALF_VALUE; }
   inline void set_to_lasthalf() { flags = 0; fraction = FRAC_FRAC_LASTHALF_VALUE; }
   inline void set_to_null_with_flags(uint32 newflags)
   { flags = newflags; fraction = FRAC_FRAC_NULL_VALUE; }
   inline void set_to_firsthalf_with_flags(uint32 newflags)
   { flags = newflags; fraction = FRAC_FRAC_HALF_VALUE; }
   inline void set_to_lasthalf_with_flags(uint32 newflags)
   { flags = newflags; fraction = FRAC_FRAC_LASTHALF_VALUE; }

   inline bool is_null() { return flags == 0 && fraction == FRAC_FRAC_NULL_VALUE; }
   inline bool is_firsthalf() { return flags == 0 && fraction == FRAC_FRAC_HALF_VALUE; }
   inline bool is_lasthalf() { return flags == 0 && fraction == FRAC_FRAC_LASTHALF_VALUE; }

   inline bool is_null_with_exact_flags(uint32 testflags)
   { return flags == testflags && fraction == FRAC_FRAC_NULL_VALUE; }

   inline bool is_null_with_masked_flags(uint32 testmask, uint32 testflags)
   { return (flags & testmask) == testflags && fraction == FRAC_FRAC_NULL_VALUE; }

   inline static int start_denom(uint32 f) { return (f >> (BITS_PER_NUMBER_FIELD*3)) & NUMBER_FIELD_MASK; }
   inline static int start_numer(uint32 f) { return (f >> (BITS_PER_NUMBER_FIELD*2)) & NUMBER_FIELD_MASK; }
   inline static int end_denom(uint32 f) { return (f >> BITS_PER_NUMBER_FIELD) & NUMBER_FIELD_MASK; }
   inline static int end_numer(uint32 f) { return (f & NUMBER_FIELD_MASK); }
   inline int start_denom() const { return start_denom(fraction); }
   inline int start_numer() const { return start_numer(fraction); }
   inline int end_denom() const { return end_denom(fraction); }
   inline int end_numer() const { return end_numer(fraction); }

   inline void repackage(int sn, int sd, int en, int ed)
   {
      if (sn < 0 || sn >= sd || en <= 0 || en > ed)
         fail("Illegal fraction.");

      reduce_fraction(sn, sd);
      reduce_fraction(en, ed);

      if (sn > NUMBER_FIELD_MASK || sd > NUMBER_FIELD_MASK ||
          en > NUMBER_FIELD_MASK || ed > NUMBER_FIELD_MASK)
         fail("Fractions are too complicated.");

      fraction = (sd<<(BITS_PER_NUMBER_FIELD*3)) | (sn<<(BITS_PER_NUMBER_FIELD*2)) | (ed<<BITS_PER_NUMBER_FIELD) | en;
   }

   // This is in sdmoves.cpp.
   void process_fractions(int start, int end,
                          fraction_invert_flags invert_flags,
                          const fraction_command & incoming_fracs,
                          bool make_improper = false,
                          bool *improper_p = 0) THROW_DECL;

   includes_first_part_enum includes_first_part();
};


// For ui_command_select:

// BEWARE!!  The resolve part of this next definition must be keyed
// to the array "title_string" in sdgetout.cpp, and maybe stuff in the UI.
// For example, see "command_menu" in sdmain.cpp.

// BEWARE!!  The order is slightly significant -- all search-type commands
// are >= command_resolve, and all "create some setup" commands
// are >= command_create_any_lines.  Certain tests are made easier by this.
enum command_kind {
   command_quit,
   command_undo,
   command_erase,
   command_abort,
   command_create_comment,
   command_change_outfile,
   command_change_outprefix,
   command_change_title,
   command_getout,
   command_cut_to_clipboard,
   command_delete_entire_clipboard,
   command_delete_one_call_from_clipboard,
   command_paste_one_call,
   command_paste_all_calls,
   command_save_pic,
   command_help,
   command_help_manual,
   command_help_faq,
   command_simple_mods,
   command_all_mods,
   command_toggle_conc_levels,
   command_toggle_minigrand,
   command_toggle_bend_home,
   command_toggle_overflow_warn,
   command_toggle_act_phan,
   command_toggle_retain_after_error,
   command_toggle_nowarn_mode,
   command_toggle_keepallpic_mode,
   command_toggle_singleclick_mode,
   command_toggle_singer,
   command_toggle_singer_backward,
   command_select_print_font,
   command_print_current,
   command_print_any,
   command_refresh,

   command_freq_show,
   command_freq_show_level,
   command_freq_show_nearlevel,
   command_freq_show_sort,
   command_freq_show_sort_level,
   command_freq_show_sort_nearlevel,
   command_freq_reset,
   command_freq_start,
   command_freq_delete,

   command_resolve,            // Search commands start here.
   command_normalize,
   command_standardize,
   command_reconcile,
   command_random_call,
   command_simple_call,
   command_concept_call,
   command_level_call,
   command_8person_level_call,
   command_create_any_lines,   // Create setup commands start here.
   command_create_waves,
   command_create_2fl,
   command_create_li,
   command_create_lo,
   command_create_inv_lines,
   command_create_3and1_lines,
   command_create_any_col,
   command_create_col,
   command_create_magic_col,
   command_create_dpt,
   command_create_cdpt,
   command_create_tby,
   command_create_8ch,
   command_create_any_qtag,
   command_create_qtag,
   command_create_3qtag,
   command_create_qline,
   command_create_3qline,
   command_create_dmd,
   command_create_any_tidal,
   command_create_tidal_wave,
   command_kind_enum_extent    // Not a command kind; indicates extent of the enum.
};

struct command_list_menu_item {
   Cstring command_name;
   command_kind action;
   int resource_id;
};

struct startup_list_menu_item {
   Cstring startup_name;
   start_select_kind action;
   int resource_id;
};

struct resolve_list_menu_item {
   Cstring command_name;
   resolve_command_kind action;
};

enum resolver_display_state {
   resolver_display_ok,
   resolver_display_searching,
   resolver_display_failed
};

// Values returned by the various popup routines.
enum popup_return {
   POPUP_DECLINE = 0,
   POPUP_ACCEPT  = 1,
   POPUP_ACCEPT_WITH_STRING = 2
};


enum file_write_flag {
   file_write_no,
   file_write_double
};


// During initialization, the main program makes a number of callbacks
// to the user interface stuff, through procedure "init_step".
// This is done to do things like put up and take down dialog boxes,
// change the status bar, and manipulate the progress bar.
// The first argument is one of these keys.  They are listed in the
// order in which they occur.  A few of them take a second argument.

enum init_callback_state {
   get_session_info,   // Query user about the session.
   final_level_query,  // Maybe query the user for the level --
                       // didn't get it from the command line or the session.
   init_database1,     // Got level, about to open database.
   init_database2,     // Starting the big database scan to create menus.
   calibrate_tick,     // Takes arg, calibrate the progress bar.
   do_tick,            // Takes arg, called repeatedly to advance the progress bar.
   tick_end,           // End the progress bar.
   do_accelerator      // Starting the processing of accelerator keys.
};


class SDLIB_API ui_utils {

   enum { PRINT_RECURSE_STAR=1, PRINT_RECURSE_CIRC=2 };

public:
   ui_utils(matcher_class *ma, iobase & iob) : m_clipboard_allocation(0), matcher_p(ma), iob88(iob)
   { m_writechar_block.usurping_writechar = false; }

   void write_header_stuff(bool with_ui_version, uint32 act_phan_flags);
   bool write_sequence_to_file() THROW_DECL;
   popup_return do_header_popup(char *dest);
   void display_initial_history(int upper_limit, int num_pics);
   void write_history_line(int history_index,
                           bool picture,
                           bool leave_missing_calls_blank,
                           file_write_flag write_to_file);
   void write_nice_number(char indicator, int num);
   void writestuff_with_decorations(const call_conc_option_state *cptr, Cstring f, bool is_concept);
   uims_reply_thing full_resolve();
   void write_aproximately();
   void write_resolve_text(bool doing_file);
   void writestuff(const char *s);
   void show_match_item(int frequency_to_show);
   void print_error_person(unsigned int person, bool example);  // In sdmain
   void printperson(uint32 x);
   void printsetup(setup *x);
   void print_4_person_setup(int ps, small_setup *s, int elong);
   void do_write(Cstring s);
   void do_change_outfile(bool signal);
   void do_change_outprefix(bool signal);
   void open_text_line();
   void clear_screen();
   void writechar(char src);
   void write_blank_if_needed();
   void newline();
   uint32_t get_number_fields(int nnumbers, bool odd_number_only, bool forbid_zero);
   bool look_up_abbreviations(int which);
   void unparse_call_name(Cstring name, char *s, const call_conc_option_state *options);
   void print_recurse(parse_block *thing, int print_recurse_arg);
   void doublespace_file();
   void do_freq_reset();
   void do_freq_start();
   void do_freq_delete();
   void do_freq_show(int options);
   void run_program(iobase & ggg);

   // These 4 are in SDSI.
   void open_file();
   void close_file();
   void write_file(const char line[]);
   void get_date(char dest[]);

   // These variables are used by printsetup/print_4_person_setup/do_write.
   int offs, roti, modulus, personstart;
   setup *printarg;

   struct writechar_block_type {
      char *destcurr;
      char lastchar;
      char lastlastchar;
      char *lastblank;
      bool usurping_writechar;
   } m_writechar_block;

   bool m_leave_missing_calls_blank;
   bool m_reply_pending;
   int m_clipboard_allocation;

   matcher_class *matcher_p;

   iobase & iob88;
};

class iobase {
 public:
   virtual int do_abort_popup() = 0;
   virtual void prepare_for_listing() = 0;
   virtual uims_reply_thing get_startup_command() = 0;
   virtual void set_window_title(char s[]) = 0;
   virtual void add_new_line(const char the_line[], uint32 drawing_picture) = 0;
   virtual void no_erase_before_n(int n) = 0;
   virtual void reduce_line_count(int n) = 0;
   virtual void update_resolve_menu(command_kind goal, int cur, int max,
                                    resolver_display_state state) = 0;
   virtual void show_match(int frequency_to_show) = 0;
   virtual const char *version_string() = 0;
   virtual uims_reply_thing get_resolve_command() = 0;
   virtual bool choose_font() = 0;
   virtual bool print_this() = 0;
   virtual bool print_any() = 0;
   virtual bool help_manual() = 0;
   virtual bool help_faq() = 0;
   virtual popup_return get_popup_string(Cstring prompt1, Cstring prompt2, Cstring final_inline_prompt,
                                         Cstring seed, char *dest) = 0;
   virtual void fatal_error_exit(int code, Cstring s1=0, Cstring s2=0) = 0;
   virtual void serious_error_print(Cstring s1) = 0;
   virtual void create_menu(call_list_kind cl) = 0;
   virtual selector_kind do_selector_popup(matcher_class &matcher) = 0;
   virtual direction_kind do_direction_popup(matcher_class &matcher) = 0;
   virtual int do_circcer_popup() = 0;
   virtual int do_tagger_popup(int tagger_class) = 0;
   virtual int yesnoconfirm(Cstring title, Cstring line1, Cstring line2, bool excl, bool info) = 0;
   virtual uint32 get_one_number(matcher_class &matcher) = 0;
   virtual uims_reply_thing get_call_command() = 0;
   virtual void dispose_of_abbreviation(const char *linebuff) = 0;
   virtual void set_pick_string(Cstring string) = 0;
   virtual void display_help() = 0;
   virtual void terminate(int code) = 0;
   virtual void process_command_line(int *argcp, char ***argvp) = 0;
   virtual void bad_argument(Cstring s1, Cstring s2, Cstring s3) = 0;
   virtual void final_initialize() = 0;
   virtual bool init_step(init_callback_state s, int n) = 0;
   virtual void set_utils_ptr(ui_utils *utils_ptr) = 0;
   virtual ui_utils *get_utils_ptr() = 0;
};

class iofull : public iobase {
 public:
   int do_abort_popup();
   void prepare_for_listing();
   uims_reply_thing get_startup_command();
   void set_window_title(char s[]);
   void add_new_line(const char the_line[], uint32 drawing_picture);
   void no_erase_before_n(int n);
   void reduce_line_count(int n);
   void update_resolve_menu(command_kind goal, int cur, int max,
                            resolver_display_state state);
   void show_match(int frequency_to_show);
   const char *version_string();
   uims_reply_thing get_resolve_command();
   bool choose_font();
   bool print_this();
   bool print_any();
   bool help_manual();
   bool help_faq();
   popup_return get_popup_string(Cstring prompt1, Cstring prompt2, Cstring final_inline_prompt,
                                 Cstring seed, char *dest);
   void fatal_error_exit(int code, Cstring s1=0, Cstring s2=0);
   void serious_error_print(Cstring s1);
   void create_menu(call_list_kind cl);
   selector_kind do_selector_popup(matcher_class &matcher);
   direction_kind do_direction_popup(matcher_class &matcher);
   int do_circcer_popup();
   int do_tagger_popup(int tagger_class);
   int yesnoconfirm(Cstring title, Cstring line1, Cstring line2, bool excl, bool info);
   void set_pick_string(Cstring string);
   uint32 get_one_number(matcher_class &matcher);
   uims_reply_thing get_call_command();
   void dispose_of_abbreviation(const char *linebuff);
   void display_help();
   void terminate(int code);
   void process_command_line(int *argcp, char ***argvp);
   void bad_argument(Cstring s1, Cstring s2, Cstring s3);
   void final_initialize();
   bool init_step(init_callback_state s, int n);
   void set_utils_ptr(ui_utils *utils_ptr);
   ui_utils *get_utils_ptr();

   ui_utils *m_ui_utils_ptr;
};

// This is the number of tagger classes.  It must not be greater than 7,
// because class numbers, in 1-based form, are put into the CFLAGH__TAG_CALL_RQ_MASK
// and CFLAG1_BASE_TAG_CALL fields, and because the tagger class is stored
// (albeit in 0-based form) in the high 3 bits (along with the tagger call number
// in the low 5 bits) in an 8-bit field that is replicated 4 times in the
// "tagger" field of a call_conc_option_state.
enum { NUM_TAGGER_CLASSES = 4 };


// These bits appear in the "concparseflags" word.
//
// If the parse turns out to be ambiguous, don't use this one --
// yield to the other one.
#define CONCPARSE_YIELD_IF_AMB   0x00000002U
// Parse directly.  It directs the parser to allow this concept
// (and similar concepts) and the following call to be typed
// on one line.  One needs to be very careful about avoiding
// ambiguity when setting this flag.
#define CONCPARSE_PARSE_DIRECT   0x00000004U
// These are used by "print_recurse" in sdutil.cpp to control the printing.
// They govern the placement of commas.
#define CONCPARSE_PARSE_L_TYPE 0x8
#define CONCPARSE_PARSE_F_TYPE 0x10
#define CONCPARSE_PARSE_G_TYPE 0x20



/* in SDCTABLE, shouldn't be */
extern SDLIB_API dance_level calling_level;



extern SDLIB_API direction_item direction_names[];                            /* in SDTABLES */
extern SDLIB_API selector_item selector_list[];                               /* in SDTABLES */
extern SDLIB_API Cstring warning_strings[];                                   /* in SDTABLES */

extern SDLIB_API call_with_name **main_call_lists[call_list_extent];
extern SDLIB_API int number_of_calls[call_list_extent];


extern SDLIB_API bool using_active_phantoms;                        /* in SDTOP */
extern SDLIB_API int allowing_modifications;                        /* in SDTOP */
extern SDLIB_API const call_conc_option_state null_options;         /* in SDTOP */
extern SDLIB_API int config_history_ptr;                            /* in SDTOP */
extern SDLIB_API bool allowing_all_concepts;                        /* in SDTOP */
extern SDLIB_API int abs_max_calls;                                 /* in SDTOP */
extern SDLIB_API int max_base_calls;                                /* in SDTOP */
extern SDLIB_API Cstring *tagger_menu_list[NUM_TAGGER_CLASSES];     /* in SDTOP */
extern SDLIB_API Cstring *selector_menu_list;                       /* in SDTOP */
extern SDLIB_API Cstring *direction_menu_list;                      /* in SDTOP */
extern SDLIB_API Cstring *circcer_menu_list;                        /* in SDTOP */
extern SDLIB_API int num_command_commands;                          /* in SDTOP */
extern SDLIB_API Cstring *command_commands;                         /* in SDTOP */
extern SDLIB_API ui_utils *gg77;                                    /* in SDTOP */


extern SDLIB_API call_with_name **tagger_calls[NUM_TAGGER_CLASSES]; /* in SDTOP */
extern SDLIB_API call_with_name **circcer_calls;                    /* in SDTOP */
extern SDLIB_API uint32 number_of_taggers[NUM_TAGGER_CLASSES];      /* in SDTOP */
extern SDLIB_API uint32 number_of_taggers_allocated[NUM_TAGGER_CLASSES]; /* in SDTOP */
extern SDLIB_API uint32 number_of_circcers;                         /* in SDTOP */
extern SDLIB_API uint32 number_of_circcers_allocated;               /* in SDTOP */
extern SDLIB_API call_conc_option_state current_options;            /* in SDTOP */

#endif   /* SDBASE_H */
