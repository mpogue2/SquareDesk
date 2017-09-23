// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2015  William B. Ackerman.
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

#include <stdio.h>
#include <errno.h>

// We used to take pity on those poor souls who are compelled to use
// troglodyte development environments.  So we used to have the conditional
// "#if __STDC__ || defined(sun)" on the "#include <stdlib.h> line just below.
// Prior to that, the conditional had been
// "#if defined(__STDC__) && !defined(athena_rt) && !defined(athena_vax)"
//
// If that conditional failed, we used to put in explicit prototypes for
// malloc, realloc, free, and exit.  Explicitly in the C namespace, of course.
// The brokenness of some Unix-like development environments used to be
// positively staggering!  You were expected to manually type in prototypes
// instead of including the appropriate header file.

// Of course, we no longer take pity on troglodyte development environments.

#include <stdlib.h>
#include <string.h>
#include "paths.h"
#include "database.h"

// Some systems can't be bothered to adhere to the standards for
// stdlib.h.  We name no names, but the company that inflicted this
// particular version of Unix on the world recently acquired Compaq.
// So we used to have the following code to define SEEK_SET explicitly.
// To zero.  That was apparently the value one was supposed to use on
// that manufacturer's systems.  I don't remember how one was supposed
// to figure that out.  But it certainly wasn't by including stdlib.h.
//
// #ifndef SEEK_SET
// #define SEEK_SET 0
// #endif

// Of course, we no longer take pity on troglodyte development environments.


FILE *db_input = NULL;
FILE *db_output = NULL;
#define FILENAME_LEN 200
char db_input_filename[FILENAME_LEN];
char db_output_filename[FILENAME_LEN];


typedef enum {
   tok_string, tok_symbol, tok_lbkt, tok_rbkt, tok_number} toktype;


/* Several of the top-level flags have different meanings for calls
defined by array or defined by (concentric or sequential) definition.

    flag                   meaning by array                      meaning by (concentric or
                                                                   sequential) definition

left_means_mirror        The "left" modifier is legal          The "left" modifier is legal
                         and causes the call to be             and will be passed on to those
                         done mirror.                          subcalls that have the
                                                               "inherit_left" property set in
                                                               the subcall invocation.  Its
                                                               meaning in that subcall is
                                                               determined according to the way
                                                               that subcall is defined, by
                                                               applying this rule recursively.
                                                               Presumably at least one subcall
                                                               has that property set.

reverse_means_mirror     The "reverse" modifier is legal       The "reverse" modifier is legal
                         and causes the call to be             and will be passed on to those
                         done mirror.                          subcalls that have the
                                                               "inherit_left" property set in
                                                               the subcall invocation.  Its
                                                               meaning in that subcall is
                                                               determined according to the way
                                                               that subcall is defined, by
                                                               applying this rule recursively.
                                                               Presumably at least one subcall
                                                               has that property set.

diamond_is_inherited     The "diamond" modifier is legal,      The "diamond" modifier is legal
                         and its meaning is determined by an   and will be passed on to those
                         item in the array list that has the   subcalls that have the
                         "alternate_definition [diamond]"      "inherit_diamond" property set in
                         flag.                                 the subcall invocation.  Its
                                                               meaning in that subcall is
                                                               determined according to the way
                                                               that subcall is defined, by
                                                               applying this rule recursively.
                                                               Presumably at least one subcall
                                                               has that property set.

magic_is_inherited       Has no meaning.  If the "magic"       The "magic" modifier is legal
                         modifier is given, its legality       and will be passed on to those
                         and meaning are determined by an      subcalls that have the
                         item in the array list that has the   "inherit_magic" property set in
                         "alternate_definition [magic]"        the subcall invocation.  Its
                         flag.                                 meaning in that subcall is
                                                               determined according to the way
                                                               that subcall is defined, by
                                                               applying this rule recursively.
                                                               Presumably at least one subcall
                                                               has that property set.

intlk_is_inherited       Has no meaning.  If the "interlocked" The "interlocked" modifier is legal
                         modifier is given, its legality       and will be passed on to those
                         and meaning are determined by an      subcalls that have the
                         item in the array list that has the   "inherit_intlk" property set in
                         "alternate_definition [interlocked]"  the subcall invocation.  Its
                         flag.                                 meaning in that subcall is
                                                               determined according to the way
                                                               that subcall is defined, by
                                                               applying this rule recursively.
                                                               Presumably at least one subcall
                                                               has that property set.

12_matrix_is_inherited   Has no meaning.  If the "12 matrix"   The "12 matrix" modifier is legal
                         modifier is given, its legality       and will be passed on to those
                         and meaning are determined by an      subcalls that have the
                         item in the array list that has the   "inherit_12_matrix" property set in
                         "alternate_definition [12matrix]"     the subcall invocation.  Its
                         flag.                                 meaning in that subcall is
                                                               determined according to the way
                                                               that subcall is defined, by
                                                               applying this rule recursively.
                                                               Presumably at least one subcall
                                                               has that property set.

funny_is_inherited       Has no meaning.  Use the              The "funny" modifier is legal
                         "simple_funny" flag at the            and will be passed on to those
                         head of each array list.              subcalls that have the
                         This will make "funny" legal          "inherit_funny" property set in
                         for that call from that setup.        the subcall invocation.  Its
                                                               meaning in that subcall is
                                                               determined according to the way
                                                               that subcall is defined, by
                                                               applying this rule recursively.
                                                               Presumably at least one subcall
                                                               has that property set.

Properties applied to a call are only legal (whether they arose from being clicked on at
the top level or being inherited from a higher level call) if they can be interpreted according
to one of the mechanisms described above.  For example, if we have:

call "scoot and plenty" c1
            left_means_mirror
            intlk_is_inherited
   seq scootback [inherit_intlk]
   seq plenty [inherit_left inherit_intlk]

then "left scoot and plenty" is only legal if "left plenty" is legal.
(The legality of "left scoot back" is irrelevant, since the "inherit_left"
property is not present on that invocation.)

Now if we have:

call "plenty" c1 tag plenty
            left_means_mirror
            intlk_is_inherited
   seq startplenty [inherit_left inherit_intlk]
   seq turnstar2 [or_anycall]
   seq finishplenty []

then "left scoot and plenty" is only legal if "left _start plenty" is legal.

Now if we have:

call "_start plenty" dontshow tag startplenty
            fudge_to_q_tag left_means_mirror
   setup qtag 2x4
      array [3S 4S 2S 2N 0 0 1S 1N]
               intlk_coming_next
   setup qtag 2x4
      array [3S 4S 1S 2N 0 0 2S 1N]

then "left scoot and plenty" is legal.  The "_start plenty" will be
done mirror, but the scoot back, star turn, and finish will not.
*/

/* A few notes about "definition modifiers":

These appear within the brackets after each call in a concdefine or seqdefine or whatever.
They are only meaningful as indicated in the table in database.h.  For example, repeat_n
is meaningful only in a "seq" definition, not in a "conc" definition.

Full documentation of these doesn't exist (and, when it does, it will belong elsewhere),
but the following notes need to be written down.

When "seq_alternate" is used to start of a "seq" definition, there MUST be two parts.
The only meaningful flags are those on the first part.

When "roll_transparent" is used on a part of a "seq" definition, that call must
have the property that anyone who is marked roll-neutral (that is, with an "M" roll
letter), must not have moved at all, and must consider this part not to have applied
to him, so that the call "roll" will direct him to use the roll direction arising
from what he did previously.  The call "centers hinge" obeys this property, and so
the "roll_transparent" flag is appropriate.  The purpose of this flag is to make
the centers hinge part of peel/trail to a diamond work correctly, so that the
new points can roll.
*/



struct tagtabitem {
   int def;
   const char *s;
};


// This table is keyed to "level".
const char *leveltab[] = {
   "mainstream",
   "plus",
   "a1",
   "a2",
   "c1",
   "c2",
   "c3a",
   "c3",
   "c3x",
   "c4a",
   "c4",
   "c4x",
   "dontshow",
   ""};

// This table is keyed to "begin_kind".
const char *sstab[] = {
   "??",
   "1x1",
   "1x2",
   "2x1",
   "1x3",
   "3x1",
   "2x2",
   "dmd",
   "pmd",
   "star",
   "trngl",
   "ptrngl",
   "trngl4",
   "ptrngl4",
   "beehive",
   "pbeehive",
   "vee",
   "pvee",
   "trngl8",
   "ptrngl8",
   "linebox",
   "plinebox",
   "linejbox",
   "plinejbox",
   "linevbox",
   "plinevbox",
   "lineybox",
   "plineybox",
   "linefbox",
   "plinefbox",
   "bone6",
   "pbone6",
   "short6",
   "pshort6",
   "1x2dmd",
   "p1x2dmd",
   "2x1dmd",
   "p2x1dmd",
   "wingedstar6",
   "pwingedstar6",
   "qtag",
   "pqtag",
   "bone",
   "pbone",
   "rigger",
   "prigger",
   "3x3",
   "2stars",
   "p2stars",
   "spindle",
   "pspindle",
   "hrglass",
   "phrglass",
   "dhrglass",
   "pdhrglass",
   "crosswave",
   "pcrosswave",
   "1x4",
   "4x1",
   "1x8",
   "8x1",
   "2x4",
   "4x2",
   "2x3",
   "3x2",
   "2x5",
   "5x2",
   "d2x5",
   "d5x2",
   "wqtag",
   "pwqtag",
   "deep2x1dmd",
   "pdeep2x1dmd",
   "whrglass",
   "pwhrglass",
   "1x5",
   "5x1",
   "1x6",
   "6x1",
   "3x4",
   "4x3",
   "2x6",
   "6x2",
   "2x7",
   "7x2",
   "d2x7",
   "d7x2",
   "2x9",
   "9x2",
   "d3x4",
   "d4x3",
   "d4x5",
   "d5x4",
   "spindle12",
   "pspindle12",
   "2x8",
   "8x2",
   "4x4",
   "1x10",
   "10x1",
   "1x12",
   "12x1",
   "1x14",
   "14x1",
   "1x16",
   "16x1",
   "c1phan",
   "galaxy",
   "3x6",
   "6x3",
   "3x8",
   "8x3",
   "4x5",
   "5x4",
   "4x6",
   "6x4",
   "2x10",
   "10x2",
   "2x12",
   "12x2",
   "deepqtg",
   "pdeepqtg",
   "deepbiqtg",
   "pdeepbigqtg",
   "widerigger",
   "pwiderigger",
   "rigger12",
   "prigger12",
   "deepxwv",
   "pdeepxwv",
   "3oqtg",
   "p3oqtg",
   "thar",
   "alamo",
   "ptpd",
   "pptpd",
   "1x3dmd",
   "p1x3dmd",
   "3x1dmd",
   "p3x1dmd",
   "3dmd",
   "p3dmd",
   "4dmd",
   "p4dmd",
   "3ptpd",
   "p3ptpd",
   "4ptpd",
   "p4ptpd",
   "hqtag",
   "phqtag",
   "hsqtag",
   "phsqtag",
   "wingedstar",
   "pwingedstar",
   "3x23",
   "p3x23",
   "3x43",
   "p3x43",
   "3x223",
   "p3x223",
   "5x25",
   "p5x25",
   "5x45",
   "p5x45",
   "5h45",
   "p5h45",
   "2x3232",
   "p2x3232",
   "3mdmd",
   "p3mdmd",
   "3mptpd",
   "p3mptpd",
   "4mdmd",
   "p4mdmd",
   "4mptpd",
   "p4mptpd",
   "1x4dmd",
   "p1x4dmd",
   "bigh",
   "pbigh",
   "bigx",
   "pbigx",
   "bigbigh",
   "pbigbigh",
   "bigbigx",
   "pbigbigx",
   "bigrig",
   "pbigrig",
   "bighrgl",
   "pbighrgl",
   "bigdhrgl",
   "pbigdhrgl",
   "bigbone",
   "pbigbone",
   "dblbone6",
   "pdblbone6",
   "bigdmd",
   "pbigdmd",
   "bigptpd",
   "pbigptpd",
   "5x1dmd",
   "p5x1dmd",
   "1x5dmd",
   "p1x5dmd",
   "big3dmd",
   "pbig3dmd",
   "big4dmd",
   "pbig4dmd",
   "dblxwave",
   "pdblxwave",
   "dblspindle",
   "pdblspindle",
   "dblbone",
   "pdblbone",
   "dblrig",
   "pdblrig",
   ""};

// This table is keyed to "setup_kind".
const char *estab[] = {
   "nothing",
   "1x1",
   "1x2",
   "1x3",
   "2x2",
   "1x4",
   "dmd",
   "star",
   "trngl",
   "trngl4",
   "beehive",
   "vee",
   "bone6",
   "short6",
   "1x5",
   "1x6",
   "2x3",
   "1x2dmd",
   "2x1dmd",
   "wingedstar6",
   "1x3p1dmd",
   "3p1x1dmd",
   "qtag",
   "bone",
   "1x8",
   "littlestars",
   "2stars",
   "1x3dmd",
   "3x1dmd",
   "spindle",
   "hrglass",
   "dhrglass",
   "hyperglass",
   "crosswave",
   "2x4",
   "2x5",
   "d2x5",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "pgdmdcw",
   "pgdmdccw",
   "1x4dmd",
   "wqtag",
   "deep2x1dmd",
   "whrglass",
   "rigger",
   "3x3",
   "3x4",
   "2x6",
   "2x7",
   "d2x5",
   "2x9",
   "d3x4",
   "d4x5",
   "spindle12",
   "???",
   "1p5x4",
   "???",
   "???",
   "???",
   "???",
   "2x8",
   "4x4",
   "1x10",
   "1x12",
   "1x14",
   "1x16",
   "c1phan",
   "bigblob",
   "ptpd",
   "3dmd",
   "4dmd",
   "3ptpd",
   "4ptpd",
   "trngl8",
   "4p2x1dmd",
   "plinepdmd",
   "plinedmd",
   "linepdmd",
   "linedmd",
   "linebox",
   "linejbox",
   "linevbox",
   "lineybox",
   "linefbox",
   "dbltrgngl4",
   "boxdmd",
   "boxpdmd",
   "dmdpdmd",
   "hsqtag",
   "dmdlndmd",
   "hqtag",
   "wingedstar",
   "wingedstar12",
   "wingedstar16",
   "barredstar",
   "barredstar12",
   "barredstar16",
   "galaxy",
   "bigh",
   "bigx",
   "3x6",
   "3x8",
   "4x5",
   "4x6",
   "2x10",
   "2x12",
   "deepqtg",
   "deepbigqtg",
   "widerigger",
   "rigger12",
   "deepxwv",
   "3oqtg",
   "thar",
   "alamo",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "x1x8",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "3x23",
   "3x43",
   "3x223",
   "5x25",
   "5x45",
   "5h45",
   "2x3232",
   "3mdmd",
   "3mptpd",
   "4mdmd",
   "4mptpd",
   "bigbigh",
   "bigbigx",
   "bigrig",
   "bighrgl",
   "bigdhrgl",
   "bigbone",
   "dblbone6",
   "bigdmd",
   "bigptpd",
   "5x1dmd",
   "1x5dmd",
   "big3dmd",
   "big4dmd",
   "dblxwave",
   "dblspindle",
   "dblbone",
   "dblrig",
   "???",
   "normal_concentric",
   ""};

// This table is keyed to "calldef_schema".
const char *schematab[] = {
   "conc",
   "crossconc",
   "3x3kconc",
   "3x3kcrossconc",
   "4x4kconc",
   "4x4kcrossconc",
   "singleconc",
   "singlecrossconc",
   "grandsingleconc",
   "grandsinglecrossconc",
   "singleconc_together",
   "singlecrossconc_together",
   "maybematrix_singleconc_together",
   "maybesingleconc",
   "maybesinglecrossconc",
   "maybegrandsingleconc",
   "maybegrandsinglecrossconc",
   "maybespecialsingleconc",
   "maybespecialtradeby",
   "specialtradeby",
   "maybegrandsingleormatrixconc",
   "3x3_conc",
   "4x4_lines_conc",
   "4x4_cols_conc",
   "maybe_nxn_lines_conc",
   "maybe_nxn_cols_conc",
   "maybe_nxn_1331_lines_conc",
   "maybe_nxn_1331_cols_conc",
   "1331_conc",
   "???",
   "1221_conc",
   "conc_diamond_line",
   "conc_lines_z",
   "conc_diamonds",
   "crossconc_diamonds",
   "conc_zs",
   "crossconc_zs",
   "conc_or_diamond_line",
   "conc_or_6_2_line",
   "conc6_2",
   "crossconc6_2",
   "conc6_2_line",
   "conc2_6",
   "crossconc2_6",
   "conc2_4",
   "crossconc2_4",
   "conc2_4_or_normal",
   "conc4_2",
   "conc4_2_prefer_1x4",
   "crossconc4_2",
   "conc4_2_or_normal",
   "conc_or_2_6",
   "conc_or_6_2",
   "???",
   "???",
   "conc2_6_or_2_4",
   "crossconc2_6_or_2_4",
   "conc_innermost",
   "conc_touch_by_1_of_3",
   "crossconc_innermost",
   "crossconc_touch_by_1_of_3",
   "conc_touch_by_2_of_3",
   "singleconc_together_if_odd",
   "singlecrossconc_together_if_odd",
   "conc6p",
   "conc6p_or_normal",
   "conc6p_or_normal_or_2x6_2x3",
   "conc6p_or_singletogether",
   "crossconc6p_or_normal",
   "conc_others",
   "conc6_2_tgl",
   "conc_to_outer_dmd",
   "conc_no31dwarn",
   "conc_specialpromenade",
   "crossconc_specialpromenade",
   "conc_12",
   "conc_16",
   "conc_star",
   "conc_star12",
   "conc_star16",
   "conc_bar",
   "conc_bar12",
   "conc_bar16",
   "conc_o",
   "conc_intermediate_diamond",
   "conc_outside_diamond",
   "maybematrix_conc",
   "maybematrix_conc_star",
   "maybematrix_conc_bar",
   "checkpoint",
   "checkpoint_spots",
   "checkpoint_mystic_ok",
   "cross_checkpoint",
   "reverse_checkpoint",
   "???",
   "ckpt_star",
   "maybe_in_out_triple_dyp_squash",
   "in_out_triple_dyp_squash",
   "maybe_in_out_triple_squash",
   "in_out_triple_squash",
   "sgl_in_out_triple_squash",
   "3x3_in_out_triple_squash",
   "4x4_in_out_triple_squash",
   "in_out_triple",
   "sgl_in_out_triple",
   "3x3_in_out_triple",
   "4x4_in_out_triple",
   "in_out_quad",
   "in_out_12mquad",
   "???",
   "???",
   "select_leads",
   "select_headliners",
   "select_sideliners",
   "select_original_rims",
   "select_original_hubs",
   "select_those_facing_both_sets_live",
   "select_center2",
   "select_center4",
   "select_center6",
   "select_who_can",
   "select_who_did",
   "select_who_didnt",
   "select_who_did_and_didnt",
   "???",
   "???",
   "???",
   "???",
   "setup",
   "nulldefine",
   "nulldefine_noroll",
   "nulldefine_other_elong",
   "matrix",
   "partnermatrix",
   "partnerpartialmatrix",
   "rolldefine",
   "recenter",
   "seq",
   "splitseq",
   "seq_with_fraction",
   "seq_with_split_1x8_id",
   "seq_alternate",
   "seq_remainder",
   "alias",
   ""};

// This table is keyed to "call_restriction".
const char *qualtab[] = {
   "none",
   "alwaysfail",
   "give_fudgy_warn",
   "wave_only",
   "wave_unless_say_2faced",
   "all_facing_same",
   "1fl_only",
   "2fl_only",
   "2fl_per_1x4",
   "ctr_2fl_only",
   "3x3_2fl_only",
   "4x4_2fl_only",
   "leads_only",
   "trailers_only",
   "couples_only",
   "3x3couples_only",
   "4x4couples_only",
   "ckpt_miniwaves",
   "ctr_miniwaves",
   "ctr_couples",
   "awkward_centers",
   "dmd_same_point",
   "dmd_facing",
   "diamond_like",
   "qtag_like",
   "qtag_like_anisotropic",
   "pu_qtag_like",
   "conc_cpls_same",
   "conc_cpls_diff",
   "regular_tbone",
   "gen_qbox",
   "nice_diamonds",
   "nice_wv_triangles",
   "nice_tnd_triangles",
   "magic_only",
   "in_or_out",
   "centers_in_or_out",
   "independent_in_or_out",
   "miniwaves",
   "not_miniwaves",
   "tgl_tandbase",
   "true_Z_cw",
   "true_Z_ccw",
   "true_PG_cw",
   "true_PG_ccw",
   "lateral_columns_empty",
   "ctrwv_end2fl",
   "ctr2fl_endwv",
   "split_dixie",
   "not_split_dixie",
   "dmd_ctrs_mwv",
   "dmd_ctrs_mwv_no_mirror",
   "dmd_ctrs_mwv_change_to_34_tag",
   "spd_base_mwv",
   "qtag_mwv",
   "qtag_mag_mwv",
   "dmd_ctrs_1f",
   "dmd_pts_mwv",
   "dmd_pts_1f",
   "dmd_intlk",
   "dmd_not_intlk",
   "tall_6",
   "ctr_pts_rh",
   "ctr_pts_lh",
   "extend_inroutl",
   "extend_inloutr",
   "said_diamond",
   "said_triangle",
   "didnt_say_triangle",
   "said_galaxy",
   "occupied_as_stars",
   "occupied_as_clumps",
   "occupied_as_blocks",
   "occupied_as_h",
   "occupied_as_qtag",
   "occupied_as_3x1tgl",
   "line_ends_looking_out",
   "col_ends_looking_in",
   "ripple_one_end",
   "ripple_both_ends",
   "ripple_both_ends_1x4_only",
   "ripple_both_centers",
   "ripple_any_centers",
   "people_1_and_5_real",
   "centers_sel",
   "ends_sel",
   "all_sel",
   "not_all_sel",
   "some_sel",
   "none_sel",
   "normal_unwrap_sel",
   "ptp_unwrap_sel",
   "explodable",
   "reverse_explodable",
   "peelable_box",
   "ends_are_peelable",
   "siamese_in_quad",
   "not_tboned_in_quad",
   "inroller_is_cw",
   "inroller_is_ccw",
   "outroller_is_cw",
   "outroller_is_ccw",
   "judge_is_cw",
   "judge_is_ccw",
   "socker_is_cw",
   "socker_is_ccw",
   "levelplus",
   "levela1",
   "levela2",
   "levelc1",
   "levelc2",
   "levelc3",
   "levelc4",
   "not_tboned",
   "opposite_sex",
   "quarterbox_or_col",
   "quarterbox_or_magic_col",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   "???",
   ""};

// This table is keyed to the constants "DFM1_***".  These are the general
// definition-modifier flags.  They go in the "modifiers1" word of a by_def_item.
const char *defmodtab1[] = {
   "conc_demand_lines",
   "conc_demand_columns",
   "conc_force_lines",
   "conc_force_columns",
   "conc_force_otherway",
   "conc_force_spots",
   "conc_concentric_rules",
   "suppress_elongation_warnings",
   "or_anycall",
   "mandatory_anycall",
   "allow_forced_mod",
   "only_force_elong_if_empty",
   "roll_transparent_if_z",
   "endscando",
   "finish_this_part",
   "roll_transparent",
   "permit_touch_or_rear_back",
   "cpls_unless_single",
   "shift_one_number",
   "shift_two_numbers",     // The constant "shift_three_numbers" is elsewhere.
   "???",                   // We get numbers by other means.
   "???",
   "???",
   "no_check_mod_level",
   "???",
   "suppress_roll",
   ""};

// This table is keyed to the constants "DFM1_SEQ***".  These are the general
// definition-modifier flags.  They go in the "modifiers1" word of a by_def_item.
const char *seqmodtab1[] = {
   "seq_re_evaluate",
   "do_half_more",
   "seq_never_re_evaluate",
   "seq_re_enable_elongation_check",
   "repeat_n",
   "repeat_nm1",
   "normalize",
   ""};

// This table is keyed to the constants "CFLAG1_***" (first 32) and "CFLAG2_***" (next 15).
// These are the general top-level call flags.  They go into the "callflags1" word and
// part of the "callflagsh" word.

const char *flagtab1f[] = {
   "first_part_visible",
   "first_two_parts_visible",
   "last_two_parts_visible",
   "12_16_matrix_means_split",
   "preserve_z_stuff",
   "split_like_dixie_style",
   "parallel_conc_end",
   "take_right_hands",
   "take_right_hands_as_couples",
   "yoyo_fractal_numbers",
   "fudge_to_q_tag",
   "step_to_wave",
   "rear_back_from_r_wave",
   "rear_back_from_qtag",
   "distribute_repetitions",
   "neednumber",
   "need_two_numbers",      // The constants "need_three_numbers" and
   "need_four_numbers",     // "optional_special_number" are elsewhere.
   "left_means_touch_or_check",
   "sequence_starter",
   "sequence_starter_promenade",
   "dont_use_in_resolve",
   "dont_use_in_nice_resolve",
   "split_large_setups",
   "split_if_z",
   "base_tag_call_0",
   "base_tag_call_1",      // The constant "base_tag_call_2" is elsewhere.
   "base_tag_call_3",
   "base_circ_call",
   "ends_take_right_hands",
   "funny_means_those_facing",
   "split_like_square_thru",

   "no_seq_if_no_frac",        // The overflow (into CFLAG2_) items start here.
   "can_be_one_side_lateral",  //    There is space for 16 of them.
   "no_elongation_allowed",
   "imprecise_rotation",
   "can_be_fan",
   "equalize",
   "one_person_call",
   "yield_if_ambiguous",
   "do_exchange_compress",
   "anyone_who_moves_cant_roll",
   "fractional_numbers",
   "dont_raise_overcast",
   "overcast_transparent",
   "is_star_call",
   "accept_in_all_menus",
   ""};

// The next three tables are all in step with each other, and with the "heritable" flags.

// This table is keyed to the constants "INHERITFLAG_???".  The bits indicated by it
// are encoded into the "callflags" word of the top-level call descriptor.
const char *flagtabh[] = {
   "diamond_is_legal",
   "reverse_means_mirror",
   "left_means_mirror",
   "funny_is_inherited",
   "intlk_is_inherited",
   "magic_is_inherited",
   "grand_is_inherited",
   "12_matrix_is_inherited",
   "16_matrix_is_inherited",
   "cross_is_inherited",
   "single_is_inherited",
   "singlefile_is_inherited",
   "half_is_inherited",
   "rewind_is_inherited",
   "straight_is_inherited",
   "twisted_is_inherited",
   "lasthalf_is_inherited",
   "fractal_is_inherited",
   "fast_is_inherited",
   ""};

// This table is keyed to the constants "INHERITFLAG_???".
// Notice that it looks like flagtabh.
const char *altdeftabh[] = {
   "diamond",
   "reverse",
   "left",
   "funny",
   "interlocked",
   "magic",
   "grand",
   "12matrix",
   "16matrix",
   "cross",
   "single",
   "singlefile",
   "half",
   "rewind",
   "straight",
   "twisted",
   "lasthalf",
   "fractal",
   "fast",
   ""};

// See INHERITFLAG_YOYOETCMASK in database.h
const char *yoyotabforce[] = {
   "force_yoyo",
   "force_generous",
   "force_stingy",
   ""};

// See INHERITFLAG_YOYOETCMASK in database.h
const char *yoyotabplain[] = {
   "yoyo",
   "generous",
   "stingy",
   ""};

// See INHERITFLAG_MXNMASK in database.h
const char *mxntabforce[] = {
   "force_1x2",
   "force_2x1",
   "force_1x3",
   "force_3x1",
   "force_0x3",
   "force_3x0",
   "force_0x4",
   "force_4x0",
   "force_6x2",
   "force_3x2",
   ""};

// See INHERITFLAG_MXNMASK in database.h
const char *mxntabplain[] = {
   "1x2",
   "2x1",
   "1x3",
   "3x1",
   "0x3",
   "3x0",
   "0x4",
   "4x0",
   "6x2",
   "3x2",
   ""};

// See INHERITFLAG_NXNMASK in database.h
const char *nxntabforce[] = {
   "force_2x2",
   "force_3x3",
   "force_4x4",
   "force_5x5",
   "force_6x6",
   "force_7x7",
   "force_8x8",
   ""};

// See INHERITFLAG_NXNMASK in database.h
const char *nxntabplain[] = {
   "2x2",
   "3x3",
   "4x4",
   "5x5",
   "6x6",
   "7x7",
   "8x8",
   ""};

// See INHERITFLAG_REVERTMASK in database.h
const char *reverttabforce[] = {
   "force_revert",
   "force_reflect",
   "force_revertreflect",
   "force_reflectrevert",
   "force_revertreflectrevert",
   "force_reflectrevertreflect",
   "force_reflectreflect",
   ""};

// See INHERITFLAG_REVERTMASK in database.h
const char *reverttabplain[] = {
   "revert",
   "reflect",
   "revertreflect",
   "reflectrevert",
   "revertreflectrevert",
   "reflectrevertreflect",
   "reflectreflect",
   ""};

// This table is keyed to the constants "dfm_***".  These are the heritable
// definition-modifier flags.  They go in the "modifiersh" word of a by_def_item.
// Notice that it looks like flagtabh.
const char *defmodtabh[] = {
   "inherit_diamond",
   "inherit_reverse",
   "inherit_left",
   "inherit_funny",
   "inherit_intlk",
   "inherit_magic",
   "inherit_grand",
   "???",
   "???",
   "inherit_cross",
   "inherit_single",
   "inherit_singlefile",
   "inherit_half",
   "inherit_rewind",
   "inherit_straight",
   "inherit_twisted",
   "inherit_lasthalf",
   "inherit_fractal",
   "inherit_fast",
   ""};

// This table is keyed to the constants "dfm_***".  These are the heritable
// definition-modifier force flags.  They go in the "modifiersh" word of a by_def_item.
// These are the words that one uses if the top-level enabling bit is OFF.  The
// bit in the "modifiersh" is the same in either case -- the different interpretation
// simply depends on whether the top level bit is on or off.
// Notice that it looks like flagtabh.
const char *forcetabh[] = {
   "force_diamond",
   "force_reverse",  // Careful with "reverse" and "left" -- the bits move around during inheritance.
   "force_left",
   "force_funny",
   "force_intlk",
   "force_magic",
   "force_grand",
   "force_12_matrix",
   "force_16_matrix",
   "force_cross",
   "force_single",
   "force_singlefile",
   "force_half",
   "force_rewind",
   "force_straight",
   "force_twisted",
   "force_lasthalf",
   "force_fractal",
   "force_fast",
   ""};


// This table is keyed to the constants "MTX_???".
const char *matrixcallflagtab[] = {
   "use_selector",
   "stop_and_warn_on_tbone",
   "tbone_is_ok",
   "ignore_nonselectees",
   "must_face_same_way",
   "find_jaywalkers",
   "both_selected_ok",
   "find_squeezers",
   "find_spreaders",
   "use_veer_data",
   "use_number",
   "lateral_mirror_if_right_of_center",
   "only_in",
   "only_out",
   "add_2n",
   "include_phantoms",
   "not_true_invader",
   ""};

// BEWARE!!  This list must track the array "pred_table" in sdpreds.cpp .
const char *predtab[] = {
   "select",
   "unselect",
   "select_near_select",
   "select_near_select_or_phantom",
   "select_near_unselect",
   "unselect_near_select",
   "unselect_near_unselect",
   "select_once_rem_from_select",
   "conc_from_select",
   "other_spindle_cw_select",
   "grand_conc_from_select",
   "other_diamond_point_select",
   "other_spindle_ckpt_select",
   "pair_person_select",
   "person_select_sum5",
   "person_select_sum8",
   "person_select_sum9",
   "person_select_sum11",
   "person_select_sum13",
   "person_select_sum15",
   "person_select_plus1",
   "person_select_plus2",
   "person_select_plus3",
   "person_select_plus4",
   "person_select_plus5",
   "person_select_plus6",
   "person_select_plus7",
   "person_select_plus8",
   "person_select_plus9",
   "person_select_plus10",
   "person_select_plus11",
   "person_select_plus12",
   "person_select_real_plus1",
   "person_select_real_plus2",
   "person_select_real_plus3",
   "person_select_real_plus4",
   "person_select_real_plus5",
   "person_select_real_plus6",
   "person_select_real_plus7",
   "person_select_real_plus8",
   "person_select_real_plus9",
   "person_select_real_plus10",
   "person_select_real_plus11",
   "person_select_nocross_plus1",
   "person_select_nocross_plus2",
   "person_select_nocross_plus3",
   "person_select_nocross_plus4",
   "person_select_nocross_plus5",
   "person_select_nocross_plus6",
   "person_select_nocross_plus7",
   "person_select_nocross_plus8",
   "person_select_nocross_plus9",
   "person_select_nocross_plus10",
   "person_select_nocross_plus11",
   "select_4x3_on_left",
   "select_4x3_on_right",
   "select_w_adj_4x4",
   "select_w_or_4x4",
   "select_w_ctr_4x4",
   "select_w_end_4x4",
   "semi_squeezer_select",
   "select_once_rem_from_unselect",
   "unselect_once_rem_from_select",
   "select_and_roll_is_cw",
   "select_and_roll_is_ccw",
   "1x2_selectee_is_linelike_facing_cw",
   "1x2_selectee_is_linelike_facing_ccw",
   "1x4_selectee_of_far_side_is_linelike_facing_cw",
   "1x4_selectee_of_far_side_is_linelike_facing_ccw",
   "kicker_coming",
   "always",
   "2x2_miniwave",
   "2x2_couple",
   "2x2_tandem_with_someone",
   "2x2_antitandem",
   "2x2_facing_someone",
   "2x4_tandem_with_someone",
   "2x4_antitandem",
   "2x4_facing_someone",
   "1x4_end_of_this_side_is_linelike_facing_cw",
   "1x4_end_of_far_side_is_linelike_facing_cw",
   "1x4_end_of_this_side_is_linelike_facing_ccw",
   "1x4_end_of_far_side_is_linelike_facing_ccw",
   "1x4_center_of_this_side_is_linelike_facing_cw",
   "1x4_center_of_far_side_is_linelike_facing_cw",
   "1x4_center_of_this_side_is_linelike_facing_ccw",
   "1x4_center_of_far_side_is_linelike_facing_ccw",
   "columns_someone_in_front",
   "x14_once_rem_miniwave",
   "x14_once_rem_couple",
   "lines_miniwave",
   "lines_couple",
   "miniwave_side_of_in_3n1_line",
   "couple_side_of_in_3n1_line",
   "miniwave_side_of_out_3n1_line",
   "couple_side_of_out_3n1_line",
   "antitandem_side_of_in_3n1_col",
   "tandem_side_of_in_3n1_col",
   "antitandem_side_of_out_3n1_col",
   "tandem_side_of_out_3n1_col",
   "miniwave_side_of_2n1_line",
   "couple_side_of_2n1_line",
   "antitandem_side_of_2n1_col",
   "tandem_side_of_2n1_col",
   "cast_normal",
   "cast_pushy",
   "cast_normal_or_warn",
   "cast_normal_or_nowarn",
   "intlk_cast_normal_or_warn",
   "lines_magic_miniwave",
   "lines_magic_couple",
   "lines_once_rem_miniwave",
   "lines_once_rem_couple",
   "lines_tandem",
   "lines_antitandem",
   "columns_tandem",
   "columns_antitandem",
   "columns_magic_tandem",
   "columns_magic_antitandem",
   "columns_once_rem_tandem",
   "columns_once_rem_antitandem",
   "columns_couple",
   "columns_miniwave",
   "1x2_beau_or_miniwave",
   "1x2_beau_miniwave_or_warn",
   "1x2_beau_miniwave_for_breaker",
   "1x2_beau_miniwave_or_ok",
   "can_swing_left",
   "can_swing_right",
   "1x4_wheel_and_deal",
   "1x4_wheel_and_deal_or_1fl",
   "1x6_wheel_and_deal",
   "1x8_wheel_and_deal",
   "cycle_and_wheel_1",
   "cycle_and_wheel_2",
   "vert1",
   "vert2",
   "inner_active_lines",
   "outer_active_lines",
   "judge_is_right",
   "judge_is_left",
   "socker_is_right",
   "socker_is_left",
   "judge_is_right_1x3",
   "judge_is_left_1x3",
   "socker_is_right_1x3",
   "socker_is_left_1x3",
   "judge_is_right_1x6",
   "judge_is_left_1x6",
   "socker_is_right_1x6",
   "socker_is_left_1x6",
   "judge_is_right_1x8",
   "judge_is_left_1x8",
   "socker_is_right_1x8",
   "socker_is_left_1x8",
   "inroller_is_cw",
   "magic_inroller_is_cw",
   "outroller_is_cw",
   "magic_outroller_is_cw",
   "inroller_is_cw_2x3",
   "magic_inroller_is_cw_2x3",
   "outroller_is_cw_2x3",
   "magic_outroller_is_cw_2x3",
   "inroller_is_cw_2x6",
   "outroller_is_cw_2x6",
   "inroller_is_cw_2x8",
   "outroller_is_cw_2x8",
   "outposter_is_cw",
   "outposter_is_ccw",
   "raise_some_sglfile",
   "zero_cw_people",
   "one_cw_person",
   "two_cw_people",
   "three_cw_people",
   "quad_person_cw",
   "quad_person_ccw",
   "next_dmd_spot_is_facing",
   "next_dmd_spot_is_normal",
   "next_qtag_spot_faces_me",
   "next_qtag_spot_faces_away",
   "nexttrnglspot_is_tboned",
   "nextinttrnglspot_is_tboned",
   "next62spot_is_tboned",
   "next_magic62spot_is_tboned",
   "next_galaxyspot_is_tboned",
   "column_double_down",
   "apex_test_1",
   "apex_test_2",
   "apex_test_3",
   "boyp",
   "girlp",
   "boyp_rh_slide_thru",
   "girlp_rh_slide_thru",
   "roll_is_cw",
   "roll_is_ccw",
   "slide_or_roll_is_cw",
   "slide_or_roll_is_ccw",
   "x12_boy_with_girl",
   "x12_girl_with_boy",
   "x22_boy_facing_girl",
   "x22_girl_facing_boy",
   "lateral_to_same_sex",
   "lateral_to_opposite_sex",
   "leftp",
   "rightp",
   "inp",
   "outp",
   "backp",
   "zigzagp",
   "zagzigp",
   "zigzigp",
   "zagzagp",
   "no_dir_p",
   "dmd_ctrs_rh",
   "dmd_ctrs_lh",
   "trngl_pt_rh",
   "q_tag_front",
   "q_tag_back",
   "q_line_front",
   "q_line_back",
   "base_is_across",
   "base_is_right",
   "base_is_here",
   "base_is_left",
   ""};

// The "tag table" is the table that we use to bind together things like
//
//            "seq flipdiamond []"
//
// in a definition, and
//
//            call "flip the diamond" plus tag flipdiamond
//
// in another definition.  We keep a list of the strings, and turn
// each tag into a number that is its index in the list.  The binary
// database deals with these numbers.  The main sd program will make
// its own copy of the table, containing pointers to the actual call
// descriptors.

// BEWARE!!  These must track the enumeration "base_call_index" in database.h
tagtabitem tagtabinit[num_base_call_indices] = {
   {1, "+++"},            // Must be unused -- call #0 signals end of list
   // in sequential encoding.
   {0, "nullcall"},       // Must be #1 -- the database initializer uses call #1
                          // for any mandatory modifier, e.g. "clover and [anything]"
                          // is executed as "clover and [call #1]".
   {0, "nullsecond"},          // Base call for mandatory secondary modification.
   {0, "real_base_0"},         // General thing that takes a tagging call
   {0, "real_base_0_noflip"},  // Same, but must not be "flip"
   {0, "base_tag_call_flip"},  // "flip"
   {0, "armturn_n4"},     // To tell where phantoms went.
   {0, "endsshadow"},     // This is used for "shadow <setup>".
   {0, "chreact_1"},      // This is used for propagating the hinge info for part 2 of chain reaction.
   {0, "makepass_1"},     // This is used for propagating the cast off 3/4 info for part 2 of make a pass.
   {0, "nuclear_1"},      // Same, for part 2 of nuclear reaction.
   {0, "scootback"},
   {0, "scootbacktowave"},
   {0, "backemup"},       // This is used for remembering the handedness.
   {0, "circulate"},
   {0, "trade"},
   {0, "plainprom"},
   {0, "plainpromeighths"},
   {0, "any_hand_remake_start_with_n"},
   {0, "passthru"},       // To tell how to do "12_16_matrix_means_split".
   {0, "check_cross_counter"},
   {0, "lockit"},
   {0, "disband1"},
   {0, "slither"},
   {0, "maybegrandslither"},
   {0, "dixie_half_tag"},
   {0, "plan_ctrtoend"},
   {0, "prepare_to_drop"},
   {0, "hinge"},
   {0, "hinge_for_nicely"},
   {0, "hinge_with_warn"},
   {0, "hinge_for_breaker"},
   {0, "hinge_then_trade"},
   {0, "hinge_then_trade_for_breaker"},
   {0, "two_o_circs_for_frac"},
   {0, "cloverleaf"},
   {0, "clover"},
   // The next "NUM_TAGGER_CLASSES" (that is, 4) must be a consecutive group.
   {0, "tagnullcall0"},
   {0, "tagnullcall1"},
   {0, "tagnullcall2"},
   {0, "tagnullcall3"},
   {0, "circnullcall"},
   {0, "turnstarn"},
   {0, "revert_if_needed"},
   {0, "extend_n"}};

int tagtabsize;          // Number of items we currently have in tagtab.
int tagtabmax;           // Amount of space allocated for tagtab;
                         // must be >= tagtabsize at all times, obviously.
tagtabitem *tagtab;      // The dynamically allocated tag list.


int errnum1 = -1;   // These may get set >= when raising a fatal error.
int errnum2 = -1;
int eof;
int chars_left;
char *lineptr;
int linelen;
int lineno;
int error_is_fatal;
int tok_value;
int letcount;
toktype tok_kind;
char tok_str[100];
int char_ct;

char line[200];
char ch;
char *return_ptr;
int callcount;
int filecount;
int dumbflag;
uint32 call_flags1;
uint32 call_flagsh;
uint32 call_flags1overflow;
uint32 call_tag;
char call_name[100];
int call_namelen;
int call_level;
int call_startsetup;
uint32 qual_stuff;
int call_endsetup;
int call_endsetup_in;
int call_endsetup_out;
int bmatrix, gmatrix;
int restriction;





static void errexit(const char s[])
{
   char my_line[1000];
   int i;

   if (error_is_fatal)
      printf("Error");
   else
      printf("Warning");

   if (lineno == 0) {
      printf(":\n%s.\n", s);
   }
   else {
      printf(" at line %d:\n%s.", lineno, s);

      if (!call_namelen)
         printf("\n");
      else
         printf("  Last call was: %s\n", call_name);

      if (!eof) {
         strncpy(my_line, lineptr, linelen-1);
         my_line[linelen-1] = '\0';
         printf("%s\n", my_line);
         for (i = 1; i <= linelen-chars_left-1; i++) printf(" ");
         printf("|\n");
      }
   }

   if (errnum1 >= 0 || errnum2 >= 0)
      printf("Error data are:   %d   %d\n", errnum1, errnum2);

   if (error_is_fatal) {
      // We have, of course, leaked all of tagtab and the strings therein,
      // and left some files open.
      exit(1);
   }
}



static int get_char()
{
   if (!chars_left) {
      lineno++;

      lineptr = fgets(line, 199, db_input);

      if (!lineptr) {
         if (!feof(db_input)) {
            fprintf(stderr, "Error reading input file ");
            perror(db_input_filename);
            exit(1);
         }

         eof = 1;
         return 1;
      }

      linelen = strlen(line);

      // Strip off any superfluous "return" or "newline" characters at the end.
      // If things are going well, there will just be a single '\n', because the
      // definition of "fgets" says so.  But, if the database text file is in a
      // non-native format, e.g. a Windows file was sent in binary form to a
      // Linux system, things could look a little funny.  So we repair the damage.

      while (linelen >= 1 && (lineptr[linelen-1] == '\r' || lineptr[linelen-1] == '\n'))
         linelen--;

      // Put it back;
      lineptr[linelen++] = '\n';

      return_ptr = lineptr;
      chars_left = linelen;
   }

   ch = *return_ptr++;
   chars_left--;
   return 0;
}

static int symchar(void)
{
   if (ch == '[' || ch == ']' || ch == ',' ||
       ch == '/' || ch == ':' || (int)ch <= 32) return 0;
   else return 1;
}

static void get_tok_or_eof()
{
   int digit;

   char_ct = 0;
   tok_value = 0;
   letcount = 0;
   while ((ch == ' ') || (ch == '/') || (ch == '\n')) {
      if (ch == '/') {
         char starter;
         if (get_char()) errexit("End of file in comment starter");
         starter = ch;
         if (ch != '*' && ch != '/') errexit("Incorrect comment starter");
         for (;;) {
            if (get_char()) errexit("End of file inside comment");
            if (starter == '*') {
               if (ch == '*') {
                  for (;;) {
                     if (get_char()) errexit("End of file inside comment");
                     if (ch != '*') break;
                  }
                  if (ch == '/') break;
               }
            }
            else if (ch == '\n') break;
         }
      }
      if (get_char()) return;
   }

   // Now have a real character.

   switch (ch) {
   case '[': tok_kind = tok_lbkt; ch = ' '; break;
   case ']': tok_kind = tok_rbkt; ch = ' '; break;
   case '"':
      for (;;) {
         if (get_char())
            errexit("End of file inside symbol\n");

         if (ch == '"') break;
         else if (ch == '\\') {
            if (get_char())
               errexit("End of file inside symbol\n");
         }

         if (char_ct > 100)
            errexit("String too long\n");

         tok_str[char_ct++] = ch;
      }

      ch = ' ';
      tok_kind = tok_string;

      // Pack a null.

      if (char_ct > 100)
         errexit("String too long\n");

      tok_str[char_ct] = '\0';
      break;
   default:
      for ( ;; ) {
         if (char_ct > 100)
            errexit("Symbol too long\n");

         tok_str[char_ct++] = ch;

         digit = ch - '0';
         if (digit < 0 || digit > 9) letcount++;
         else tok_value = tok_value*10 + digit;

         if (get_char())
            errexit("End of file inside symbol\n");

         if (!symchar()) break;
      }

      // Pack a null.

      if (char_ct > 100)
         errexit("Symbol too long\n");

      tok_str[char_ct] = '\0';

      if (letcount)
         tok_kind = tok_symbol;
      else
         tok_kind = tok_number;
      break;
   }
}


// This returns -1 if the item is not found.

static int search(const char *table[])
{
   int i;

   i = -1;
   while (*table[++i]) {
      if (!strcmp(tok_str, table[i]))
         return i;
   }
   return -1;
}


// The returned value fits into 13 bits.
static uint32 tagsearch(int def)
{
   int i;

   for (i = 0; i < tagtabsize; i++) {
      if (!strcmp(tok_str, tagtab[i].s)) goto done;
   }

   i = tagtabsize++;
   // Independently of the way we reallocate memory, the tag field must be able
   // to fit into 13 bits in order to be packed into the binary database file.
   // Also, we can't allow a tag of 8191, because that would be taken
   // as a compound call definition.
   if (i >= 8190) errexit("Sorry, too many tagged calls");

   if (i >= tagtabmax) {
      tagtabitem *tempsave = tagtab;
      tagtab = new tagtabitem [tagtabmax * 2];
      memcpy(tagtab, tempsave, tagtabmax * sizeof(tagtabitem));
      tagtabmax <<= 1;
      delete [] tempsave;
   }

   {
      char *tempstring = new char [strlen(tok_str)+1];
      strcpy(tempstring, tok_str);
      tagtab[i].s = tempstring;
      tagtab[i].def = 0;
   }

 done:

   if (def) {
      if (tagtab[i].def) errexit("Multiple definition of a call tag");
      tagtab[i].def = 1;
   }
   return i;
}


static void get_tok(void)
{
   get_tok_or_eof();
   if (eof) errexit("Unexpected end of file");
}


static int get_num(const char s[])
{
   get_tok();
   if ((tok_kind != tok_number)) errexit(s);
   return tok_value;
}


void db_output_error()
{
    fprintf(stderr, "Error writing output file ");
    perror(db_output_filename);
    exit(1);
}


// There is a gross bug in the DJGPP library.  When figuring the return value,
// fputc sign-extends the input datum and returns that as an int.  This means
// that, if we try to write 0xFF, we get back -1, which is the error condition.
// So, under DJGPP, we used to have to use errno as the sole means of telling
// whether an error occurred.  Of course, we no longer compile with DJGPP, and
// no longer take pity on buggy compilers.
void db_putc(char ch)
{
   if (fputc(ch, db_output) == EOF)
      db_output_error();
}

static void write_byte(uint32 n)
{
   db_putc((char) ((n) & 0xFF));
   filecount += 1;
}

static void write_halfword(uint32 n)
{
   db_putc((char) ((n>>8) & 0xFF));
   db_putc((char) ((n) & 0xFF));
   filecount += 2;
}

static void write_fullword(uint32 n)
{
   db_putc((char) ((n>>24) & 0xFF));
   db_putc((char) ((n>>16) & 0xFF));
   db_putc((char) ((n>>8) & 0xFF));
   db_putc((char) ((n) & 0xFF));
   filecount += 4;
}


// Do_heritflag_merge is in common.cpp.


static void write_defmod_flags(int is_seq)
{
   int i;
   uint32 rr1 = 0;
   uint32 rrh = 0;

   get_tok();
   if (tok_kind != tok_lbkt)
      errexit("Missing left bracket in defmod list");

   get_tok();
   if (tok_kind != tok_rbkt) {
      for (;;) {
         if (tok_kind != tok_symbol)
            errexit("Improper defmod key");

         if ((i = search(defmodtab1)) >= 0)
            rr1 |= (1U << i);
         else if (is_seq && (i = search(seqmodtab1)) >= 0)
            rr1 |= (1U << i);
         else if (!strcmp(tok_str, "allow_plain_mod"))
            rr1 |= (DFM1_CALL_MOD_ALLOW_PLAIN_MOD);
         else if (!strcmp(tok_str, "or_secondary_call"))
            rr1 |= (DFM1_CALL_MOD_OR_SECONDARY);
         else if (!strcmp(tok_str, "mandatory_secondary_call"))
            rr1 |= (DFM1_CALL_MOD_MAND_SECONDARY);
         else if (!strcmp(tok_str, "shift_three_numbers"))
            rr1 |= (3*DFM1_NUM_SHIFT_BIT);
         else if (!strcmp(tok_str, "insert_number")) {
            int nnn;
            if (rr1 & DFM1_NUM_INSERT_MASK) errexit("Only one number insertion is allowed");
            nnn = get_num("Need a number here");
            if (nnn <= 0 || nnn >= 8) errexit("bad number");
            rr1 |= nnn*DFM1_NUM_INSERT_BIT;
         }
         else if (!strcmp(tok_str, "insert_fractal")) {
            int nnn;
            if (rr1 & DFM1_NUM_INSERT_MASK) errexit("Only one number insertion is allowed");
            nnn = get_num("Need a number here");
            if (nnn <= 0 || nnn >= 8) errexit("bad number");
            rr1 |= (nnn*DFM1_NUM_INSERT_BIT) | DFM1_FRACTAL_INSERT;
         }
         else if (!strcmp(tok_str, "inherit_nxn")) {
            if (INHERITFLAG_NXNMASK & ~call_flagsh)
               errexit("Can't use an \"inherit\" flag unless corresponding top level flag is on");

            rrh |= INHERITFLAG_NXNMASK;
         }
         else if (!strcmp(tok_str, "inherit_mxn")) {
            if (INHERITFLAG_MXNMASK & ~call_flagsh)
               errexit("Can't use an \"inherit\" flag unless corresponding top level flag is on");

            rrh |= INHERITFLAG_MXNMASK;
         }
         else if (!strcmp(tok_str, "inherit_bigmatrix")) {
            if ((INHERITFLAG_12_MATRIX|INHERITFLAG_16_MATRIX) & ~call_flagsh)
               errexit("Can't use an \"inherit\" flag unless corresponding top level flag is on");

            rrh |= INHERITFLAG_12_MATRIX|INHERITFLAG_16_MATRIX;
         }
         else if (!strcmp(tok_str, "inherit_revert")) {
            if ((INHERITFLAG_REVERTMASK) & ~call_flagsh)
               errexit("Can't use an \"inherit\" flag unless corresponding top level flag is on");

            rrh |= INHERITFLAG_REVERTMASK;
         }
         else if (!strcmp(tok_str, "inherit_yoyo")) {
            if (INHERITFLAG_YOYOETCK_YOYO & ~call_flagsh)
               errexit("Can't use an \"inherit\" flag unless corresponding top level flag is on");

            rrh |= INHERITFLAG_YOYOETCK_YOYO;
         }
         else if (!strcmp(tok_str, "inherit_gensting")) {
            if (INHERITFLAG_YOYOETCK_GENEROUS & ~call_flagsh)
               errexit("Can't use an \"inherit\" flag unless corresponding top level flag is on");

            rrh |= INHERITFLAG_YOYOETCK_GENEROUS;
         }
         else if ((i = search(defmodtabh)) >= 0) {
            uint32 bit = 1U << i;

            // Don't check the left/reverse flags -- they are complicated,
            // so there is no "force" word for them.
            if (bit & ~(call_flagsh | INHERITFLAG_REVERSE | INHERITFLAG_LEFT))
               errexit("Can't use an \"inherit\" flag unless corresponding top level flag is on");

            rrh |= bit;
         }
         else {
            uint32 bit;

            if ((i = search(forcetabh)) >= 0) bit = (1U << i);
            else if ((i = search(yoyotabforce)) >= 0) bit = INHERITFLAG_YOYOETCBIT * (i+1);
            else if ((i = search(mxntabforce)) >= 0) bit = INHERITFLAG_MXNBIT * (i+1);
            else if ((i = search(nxntabforce)) >= 0) bit = INHERITFLAG_NXNBIT * (i+1);
            else if ((i = search(reverttabforce)) >= 0) bit = INHERITFLAG_REVERTBIT * (i+1);
            else errexit("Unknown defmod key");

            // Don't check the left/reverse flags -- they are complicated,
            // so there is no "force" word for them.
            if (bit & call_flagsh & ~(INHERITFLAG_REVERSE | INHERITFLAG_LEFT))
               errexit("Can't use a \"force\" flag unless corresponding top level flag is off");

            if (do_heritflag_merge(&rrh, bit))
               errexit("Redundant \"force\" flags");
         }

         get_tok();
         if (tok_kind == tok_rbkt) break;
      }
   }

   write_fullword(rr1);
   write_fullword(rrh);
}



static void write_callarray(int num, bool doing_matrix)
{
   int count;

   if (tok_kind != tok_lbkt)
      errexit("Missing left bracket in callarray list");

   for (count=0; ; count++) {
      get_tok();
      if (tok_kind == tok_rbkt) break;
      else if (tok_kind == tok_number && tok_value == 0)
         write_halfword(0);
      else if (tok_kind == tok_symbol) {
         int p;
         uint32 stab = STB_NONE;
         int repetition = 0;

         for (p=0; letcount-p >= 2; p++) {
            switch (tok_str[p]) {
            case 'Z': case 'z':
               // "Z" is "none" with reversal on.
               if (stab == STB_NONE) stab = STB_NONE+STB_REVERSE;
               else errexit("Improper callarray specifier");
               break;
            case 'A': case 'a':
               switch (stab) {
               case STB_NONE: stab = STB_A; break;
               case STB_A: stab = STB_AA; break;
               case STB_AA: repetition++; break;
               case STB_A+STB_REVERSE: stab = STB_AC+STB_REVERSE; break;
               case STB_AA+STB_REVERSE:
                  if (repetition == 0)
                     stab = STB_AAC+STB_REVERSE;
                  else if (repetition == 1)
                     { stab = STB_AAAC+STB_REVERSE; repetition = 0; }
                  else if (repetition == 2)
                     { stab = STB_AAAAC+STB_REVERSE; repetition = 0; }
                  break;
               default: errexit("Improper callarray specifier");
               }
               break;
            case 'C': case 'c':
               switch (stab) {
               case STB_NONE: stab = STB_A+STB_REVERSE; break;
               case STB_A: stab = STB_AC; break;
               case STB_AA:
                  if (repetition == 0)
                     stab = STB_AAC;
                  else if (repetition == 1)
                     { stab = STB_AAAC; repetition = 0; }
                  else if (repetition == 2)
                     { stab = STB_AAAAC; repetition = 0; }
                  break;

               case STB_A+STB_REVERSE: stab = STB_AA+STB_REVERSE; break;
               case STB_AA+STB_REVERSE: repetition++; break;
               default: errexit("Improper callarray specifier");
               }
               break;
            default:
               goto stability_done;
            }
         }

      stability_done:

         uint32 dat = 0;

         // Read the "slide" indicator.

         if (letcount-p >= 2) {
            switch (tok_str[p]) {
            case 'F': case 'f': dat |= 16 * DBSLIDEROLL_BIT;
               p++;
               break;
            case 'G': case 'g': dat |= 8 * DBSLIDEROLL_BIT;
               p++;
               break;
            }
         }

         if (repetition != 0) errexit("Improper callarray specifier");

         if (letcount-p == 2) {
            switch (tok_str[p]) {
            case 'R': case 'r': dat |= 1 * DBSLIDEROLL_BIT; break;
            case 'L': case 'l': dat |= 2 * DBSLIDEROLL_BIT; break;
            case 'M': case 'm': dat |= 3 * DBSLIDEROLL_BIT; break;
            default:
               errexit("Improper callarray specifier");
            }
         }
         else if (letcount-p != 1)
            errexit("Improper callarray specifier");

         // Compress tok_value into a 5 bit field.
         // Everything up to 24 is natural.  Beyond that, only even numbers 26-36 are allowed.
         // They are compressed to 25-30.

         if (tok_value > 24) {
            if (tok_value > 36 || (tok_value & 1))
               errexit("Callarray number out of range");
            tok_value = (tok_value >> 1) + 12;
         }

         // It's now in the range 0 ... 30.  Shift it to 1 ... 31 so that the word will never be zero.

         tok_value++;

         dat |= (stab * DBSTAB_BIT) | (tok_value << 2);

         // We now have slide/roll indicator and position, need to get direction.
         switch (tok_str[char_ct-1]) {
         case 'N': case 'n': break;
         case 'E': case 'e': dat |= 1; break;
         case 'S': case 's': dat |= 2; break;
         case 'W': case 'w': dat |= 3; break;
         default:
            errexit("Improper callarray direction specifier");
         }

         write_halfword(dat);

         if (doing_matrix)
            write_halfword(get_num("Improper y coordinate"));
      }
      else
         errexit("Improper callarray element");
   }

   if (count != num) {
      errnum1 = count;
      errnum2 = num;
      if (count < num) errexit("Callarray list is too short for this call");
      else             errexit("Callarray list is too long for this call");
   }
}


static void write_call_header(calldef_schema schema)
{
   int j;

   if (call_namelen == 0) {
      write_halfword(0x3FFF);
   }
   else {
      write_halfword(0x2000 | call_tag );
      write_byte(call_level);
   }

   write_halfword(call_flags1overflow);
   write_fullword(call_flags1);
   write_fullword(call_flagsh);
   write_halfword((call_namelen << 8) | (uint32) schema);

   for (j=0; j<call_namelen; j++)
      write_byte((uint32) call_name[j]);

   callcount++;
}


static void write_conc_stuff(calldef_schema schema)
{
   // Write two level 2 concdefine records.

   get_tok();
   if (tok_kind != tok_symbol) errexit("Improper conc symbol");

   write_halfword(0x4000 | tagsearch(0));
   write_defmod_flags(0);

   get_tok();
   if (tok_kind != tok_symbol) errexit("Improper conc symbol");

   write_halfword(0x4000 | tagsearch(0));
   write_defmod_flags(0);
}


static void write_seq_stuff(void)
{
   get_tok();
   if (tok_kind != tok_symbol) errexit("Improper seq symbol");
   write_halfword(0x4000 | tagsearch(0));
   write_defmod_flags(1);
}


static void write_pred_clauses()
{
   for (;;) {
      get_tok_or_eof();
      if (eof) break;

      if (tok_kind != tok_symbol) break;    // Will give an error.

      if (!strcmp(tok_str, "if")) {
         int iii;

         get_tok();
         if (tok_kind != tok_symbol) errexit("Improper predicate");
         if ((iii = search(predtab)) < 0) errexit("Unknown predicate");

         // Write a level 4 group.
         write_halfword(0x6000);
         write_halfword(iii);
         get_tok();
         write_callarray(begin_sizes[call_startsetup], false);
      }
      else
         break;
   }
}


static void write_array_def_block(uint32 callarrayflags)
{
   // Bits in specifying an array def are very precious, so we use only a single "1" bit
   // to indicate an array def block.
   // The other indicators use 3 bits:
   //    000 - end of file
   //    001 (0x2000) - top level call
   //    010 (0x4000) - compound call def
   //    011 (0x6000) - predicate group
   if (callarrayflags & 0xFFC00000) errexit("Internal error -- too many callarray flags");
   write_halfword(0x8000 | (callarrayflags>>7));

   write_fullword(((callarrayflags & 0x7F) << 25) | (restriction << 17) | qual_stuff);

   if (call_startsetup >= 256) errexit("Internal error -- too many start setups");

   if (callarrayflags & CAF__CONCEND) {
      write_halfword(call_endsetup_in | (call_startsetup << 8));
      write_halfword(call_endsetup_out);
   }
   else {
      write_halfword(call_endsetup | (call_startsetup << 8));
   }
}


static int scan_for_per_array_def_flags(void)
{
   int result = 0;

   for ( ; ; ) {
      if (!strcmp(tok_str, "simple_funny"))
         result |= CAF__FACING_FUNNY;
      else if (!strcmp(tok_str, "lateral_to_selectees"))
         result |= CAF__LATERAL_TO_SELECTEES;
      else if (!strcmp(tok_str, "split_to_box"))
         result |= CAF__SPLIT_TO_BOX;
      else break;

      get_tok();
      if (tok_kind != tok_symbol) errexit("Missing indicator");
   }

   return result;
}

static void process_alt_def_header()
{
   uint32 rrr = 0;

   get_tok();
   if (tok_kind != tok_lbkt)
      errexit("Missing left bracket in alternate_definition list");

   get_tok();
   if (tok_kind != tok_rbkt) {
      for (;;) {
         int i;
         uint32 bit;

         if (tok_kind != tok_symbol)
            errexit("Improper alternate_definition key");

         if ((i = search(altdeftabh)) >= 0) bit = (1U << i);
         else if ((i = search(yoyotabplain)) >= 0) bit = INHERITFLAG_YOYOETCBIT * (i+1);
         else if ((i = search(mxntabplain)) >= 0) bit = INHERITFLAG_MXNBIT * (i+1);
         else if ((i = search(nxntabplain)) >= 0) bit = INHERITFLAG_NXNBIT * (i+1);
         else if ((i = search(reverttabplain)) >= 0) bit = INHERITFLAG_REVERTBIT * (i+1);
         else errexit("Unknown alternate_definition key");

         if (do_heritflag_merge(&rrr, bit))
            errexit("Can't specify this combination of flags");

         get_tok();
         if (tok_kind == tok_rbkt) break;
      }
   }

   get_tok();
   if (tok_kind != tok_symbol) errexit("Improper alternate_definition level");
   int alt_level = search(leveltab);
   if (alt_level < 0) errexit("Unknown alternate_definition level");
   if (alt_level >= 16) errexit("Too many levels");

   write_halfword(0x4000 | alt_level);
   write_fullword(rrr);
}



static void write_array_def(uint32 incoming)
{
   int jjj;
   uint32 callarray_flags1, callarray_flags2;

   callarray_flags1 = incoming;

def2:
   restriction = 0;
   callarray_flags2 = 0;
   qual_stuff = 0;

   get_tok();
   if (tok_kind != tok_symbol) errexit("Improper starting setup");
   if ((call_startsetup = search(sstab)) < 0) errexit("Unknown start setup");

   get_tok();
   if (tok_kind != tok_symbol) errexit("Improper ending setup");
   if ((call_endsetup = search(estab)) < 0) errexit("Unknown ending setup");

   // Should actually look for anomalous concentric and do it specially.

   for (;;) {
      get_tok();
      if (tok_kind != tok_symbol) errexit("Missing indicator");

      if (!strcmp(tok_str, "array")) {
         write_array_def_block(callarray_flags1 | callarray_flags2);              // Pred flag off.
         get_tok();
         write_callarray(begin_sizes[call_startsetup], false);
         get_tok_or_eof();
         break;
      }
      else if (!strcmp(tok_str, "preds")) {
         write_array_def_block(CAF__PREDS | callarray_flags1 | callarray_flags2); // Pred flag on.
         get_tok();
         if (tok_kind != tok_string) errexit("Missing string");

         write_halfword(char_ct);
         for (jjj = 1; jjj <= (char_ct >> 1); jjj++)
            write_halfword((((int)(tok_str[2*jjj-2])) << 8) | ((int)(tok_str[2*jjj-1])));
         if (char_ct&1)
            write_halfword(((int)(tok_str[char_ct-1])) << 8);

         write_pred_clauses();
         break;
      }
      else if (!strcmp(tok_str, "predscantdo")) {
         // See prederrmgstable in sdinit.cpp .
         write_array_def_block(CAF__PREDS | callarray_flags1 | callarray_flags2);
         write_halfword(0x8000+0);
         write_pred_clauses();
         break;
      }
      else if (!strcmp(tok_str, "predscantdocallinsetup")) {
         write_array_def_block(CAF__PREDS | callarray_flags1 | callarray_flags2);
         write_halfword(0x8000+1);
         write_pred_clauses();
         break;
      }
      else if (!strcmp(tok_str, "predsnoselect")) {
         write_array_def_block(CAF__PREDS | callarray_flags1 | callarray_flags2);
         write_halfword(0x8000+2);
         write_pred_clauses();
         break;
      }
      else if (!strcmp(tok_str, "qualifier")) {
         int t;

         if (qual_stuff) errexit("Only one qualifier is allowed");
         get_tok();

         // Look for other indicators.
         for (;;) {
            if (tok_kind == tok_symbol && (!strcmp(tok_str, "left") || !strcmp(tok_str, "out"))) {
               qual_stuff |= QUALBIT__LEFT;
               get_tok();
            }
            else if (tok_kind == tok_symbol && (!strcmp(tok_str, "right") || !strcmp(tok_str, "in"))) {
               qual_stuff |= QUALBIT__RIGHT;
               get_tok();
            }
            else if (tok_kind == tok_symbol && !strcmp(tok_str, "live")) {
               qual_stuff |= QUALBIT__LIVE;
               get_tok();
            }
            else if (tok_kind == tok_symbol && !strcmp(tok_str, "tbone")) {
               if (qual_stuff & (QUALBIT__TBONE|QUALBIT__NTBONE))
                  errexit("Can't specify both \"tbone\" and \"ntbone\"");
               qual_stuff |= QUALBIT__TBONE;
               get_tok();
            }
            else if (tok_kind == tok_symbol && !strcmp(tok_str, "ntbone")) {
               if (qual_stuff & (QUALBIT__TBONE|QUALBIT__NTBONE))
                  errexit("Can't specify both \"tbone\" and \"ntbone\"");
               qual_stuff |= QUALBIT__NTBONE;
               get_tok();
            }
            else if (tok_kind == tok_symbol && !strcmp(tok_str, "explicit")) {
               if (qual_stuff & (QUALBIT__TBONE|QUALBIT__NTBONE))
                  errexit("Can't specify \"explicit\" and \"tbone\"");
               qual_stuff |= QUALBIT__TBONE|QUALBIT__NTBONE;
               get_tok();
            }
            else if (tok_kind == tok_symbol && !strcmp(tok_str, "num")) {
               int n = get_num("Need a qualifier number here");
               if (n > 14)
                  errexit("Number may not be greater than 14");
               qual_stuff |= (n+1) * QUALBIT__NUM_BIT;
               get_tok();
            }
            else
               break;
         }

         if (tok_kind != tok_symbol) errexit("Improper qualifier");
         if ((t = search(qualtab)) < 0) errexit("Unknown qualifier");
         if (t > 255) errexit("Too many qualifiers");
         qual_stuff |= t;
      }
      else if (!strcmp(tok_str, "restriction")) {
         get_tok();
         if (tok_kind != tok_symbol) errexit("Improper restriction specifier");

         if (!strcmp(tok_str, "unusual")) {
            callarray_flags2 |= CAF__RESTR_UNUSUAL;
            get_tok();
            if (tok_kind != tok_symbol) errexit("Improper restriction specifier");
         }
         else if (!strcmp(tok_str, "forbidden")) {
            callarray_flags2 |= CAF__RESTR_FORBID;
            get_tok();
            if (tok_kind != tok_symbol) errexit("Improper restriction specifier");
         }
         else if (!strcmp(tok_str, "resolve_ok")) {
            callarray_flags2 |= CAF__RESTR_RESOLVE_OK;
            get_tok();
            if (tok_kind != tok_symbol) errexit("Improper restriction specifier");
         }
         else if (!strcmp(tok_str, "controversial")) {
            callarray_flags2 |= CAF__RESTR_CONTROVERSIAL;
            get_tok();
            if (tok_kind != tok_symbol) errexit("Improper restriction specifier");
         }
         else if (!strcmp(tok_str, "serious_violation")) {
            callarray_flags2 |= CAF__RESTR_BOGUS;
            get_tok();
            if (tok_kind != tok_symbol) errexit("Improper restriction specifier");
         }
         else if (!strcmp(tok_str, "assume_dpt")) {
            callarray_flags2 |= CAF__RESTR_ASSUME_DPT;
            get_tok();
            if (tok_kind != tok_symbol) errexit("Improper restriction specifier");
         }
         else if (!strcmp(tok_str, "each_1x3")) {
            callarray_flags2 |= CAF__RESTR_EACH_1X3;
            get_tok();
            if (tok_kind != tok_symbol) errexit("Improper restriction specifier");
         }

         if ((restriction = search(qualtab)) < 0) errexit("Unknown restriction specifier");
         if (restriction > 255) errexit("Too many restrictions");
      }
      else if (!strcmp(tok_str, "rotate"))
         callarray_flags1 |= CAF__ROT;
      else if (!strcmp(tok_str, "no_cutting_through"))
         callarray_flags1 |= CAF__NO_CUTTING_THROUGH;
      else if (!strcmp(tok_str, "no_facing_ends"))
         callarray_flags1 |= CAF__NO_FACING_ENDS;
      else if (!strcmp(tok_str, "other_elongate"))
         callarray_flags1 |= CAF__OTHER_ELONGATE;
      else if (!strcmp(tok_str, "really_want_diamond"))
         callarray_flags1 |= CAF__REALLY_WANT_DIAMOND;
      else if (!strcmp(tok_str, "no_compress"))
         callarray_flags1 |= CAF__NO_COMPRESS;
      else if (!strcmp(tok_str, "plus_eighth_rotation"))
         callarray_flags1 |= CAF__PLUSEIGHTH_ROTATION;
      else if ((!(callarray_flags1 & CAF__CONCEND)) && (!strcmp(tok_str, "concendsetup"))) {
         if (call_endsetup != (int) s_normal_concentric)
            errexit("concendsetup with wrong end_setup");
         get_tok();
         if (tok_kind != tok_symbol) errexit("Improper setup specifier");
         if ((call_endsetup_in = search(estab)) < 0) errexit("Unknown setup specifier");

         get_tok();
         if (tok_kind != tok_symbol) errexit("Improper setup specifier");
         if ((call_endsetup_out = search(estab)) < 0) errexit("Unknown setup specifier");

         jjj = get_num("Improper second rotation");
         if (jjj != (jjj & 1))
            errexit("Improper second rotation");
         if (jjj) callarray_flags1 |= CAF__ROT_OUT;

         callarray_flags1 |= CAF__CONCEND;
      }
      else
         errexit("Item in illegal context while parsing setups/preds/arrays");
   }

   if (eof) return;
   if (tok_kind != tok_symbol) errexit("Missing indicator");

   // If see something other than "setup", it's either an alternate definition
   // to start another group of arrays, or it's the end of the whole call.

   callarray_flags1 = 0;

   if (strcmp(tok_str, "setup")) {
      if (strcmp(tok_str, "alternate_definition")) {
         return;       // Must have seen next 'call' indicator.
      }

      process_alt_def_header();

      // Now do another group of arrays.

      get_tok();
      if (tok_kind != tok_symbol) errexit("Missing indicator");
      callarray_flags1 |= scan_for_per_array_def_flags();

      if (strcmp(tok_str, "setup"))
         errexit("Need \"setup\" indicator");
   }

   // Must have seen "setup" -- do another basic array.

   goto def2;
}


int main(int argc, char *argv[])
{
   if (argc == 2)
       strncpy(db_input_filename, argv[1], FILENAME_LEN);
   else
       strncpy(db_input_filename, CALLS_FILENAME, FILENAME_LEN);

   strncpy(db_output_filename, DATABASE_FILENAME, FILENAME_LEN);

   db_input = fopen(db_input_filename, "r");
   if (!db_input) {
      fprintf(stderr, "Can't open input file ");
      perror(db_input_filename);
      exit(1);
   }

   if (remove(db_output_filename)) {
      if (errno != ENOENT) {
	 fprintf(stderr, "trouble deleting old output file ");
	 perror(db_output_filename);
         // This one does NOT abort.
      }
   }

   // The "b" in the mode is meaningless and harmless in POSIX.  Some systems,
   // however, require it for correct handling of binary data.
   db_output = fopen(db_output_filename, "wb");
   if (!db_output) {
      fprintf(stderr, "Can't open output file ");
      perror(db_output_filename);
      exit(1);
   }

   int i, iii;
   uint32 funnyflag;

   tagtabsize = num_base_call_indices;
   tagtabmax = 100;
   if (tagtabsize > tagtabmax) tagtabmax = tagtabsize;
   tagtab = new tagtabitem [tagtabmax];
   lineno = 0;
   chars_left = 0;
   error_is_fatal = 1;
   ch = ' ';
   call_namelen = 0;   // So won't print error first time around.

   memcpy(tagtab, tagtabinit, sizeof(tagtabinit));   // Initialize first few entries in tagtab.

   filecount = 0;

   write_halfword(DATABASE_MAGIC_NUM);
   write_halfword(DATABASE_FORMAT_VERSION);
   // Next two halfwords in the file will be the call count and the tag table size.
   // The latter tells sd how much memory to allocate internally for its reconstruction
   // of the tag table.
   write_halfword(0);
   write_halfword(0);

   get_tok();
   if (tok_kind != tok_symbol) errexit("Improper \"version\" indicator");
   if (strcmp(tok_str, "version")) errexit("Missing version specification");
   get_tok();
   if (tok_kind != tok_string) errexit("Improper version string -- must be in quotes");
   write_halfword(char_ct);
   for (i=0; i<char_ct; i++)
      db_putc((char) (((uint32) tok_str[i]) & 0xFF));
   filecount += char_ct;

   callcount = 0;
   for (;;) {
      uint32 matrixflags;
      int bit;
      calldef_schema ccc;

      get_tok_or_eof();
   startagain:
      if (eof) break;
      else if (tok_kind != tok_symbol) errexit("Missing indicator");
      else if (strcmp(tok_str, "call")) errexit("Item in illegal context");

      get_tok();
      if (tok_kind != tok_string) errexit("Improper call name");

      strcpy(call_name, tok_str);
      call_namelen = char_ct;

      get_tok();
      if (tok_kind != tok_symbol) errexit("Improper level");
      call_level = search(leveltab);
      if (call_level < 0) errexit("Unknown level");
      if (call_level >= 16) errexit("Too many levels");

      call_tag = 0;

      get_tok();
      if (tok_kind != tok_symbol) errexit("Missing indicator");

      // Get tag, if present.

      if (!strcmp(tok_str, "tag")) {
         get_tok();
         if (tok_kind != tok_symbol) errexit("Improper tag");
         call_tag = tagsearch(1);
         get_tok();
         if (tok_kind != tok_symbol) errexit("Missing indicator");
      }

   restart_compound_call:

      // Get toplevel options.

      call_flagsh = 0;
      call_flags1 = 0;
      call_flags1overflow = 0;

      for (;;) {
         uint32 flagh_to_set = 0;
         uint32 flag1_to_set = 0;

         if ((iii = search(flagtab1f)) >= 0) {
            if (iii >= 32) {
               call_flags1overflow |= (1U << (iii-32));
               // We only have room for 16 overflow call flags.
               if (call_flags1overflow & ~0xFFFF)
                  errexit("Too many secondary flags");
            }
            else {
               uint32 bit = 1U << iii;
               if ((call_flags1 & CFLAG1_STEP_REAR_MASK) &&
                   (bit & CFLAG1_STEP_REAR_MASK))
                  errexit("Too many touch/rear flags");
               call_flags1 |= bit;
            }
         }
         else if (!strcmp(tok_str, "step_to_nonphantom_box")) {
            if (call_flags1 & CFLAG1_STEP_REAR_MASK)
               errexit("Too many touch/rear flags");
            call_flags1 |= CFLAG1_STEP_TO_NONPHAN_BOX;
         }
         else if (!strcmp(tok_str, "rear_back_from_wave_or_qtag")) {
            if (call_flags1 & CFLAG1_STEP_REAR_MASK)
               errexit("Too many touch/rear flags");
            call_flags1 |= CFLAG1_REAR_BACK_FROM_EITHER;
         }
         else if (!strcmp(tok_str, "step_to_qtag")) {
            if (call_flags1 & CFLAG1_STEP_REAR_MASK)
               errexit("Too many touch/rear flags");
            call_flags1 |= CFLAG1_STEP_TO_QTAG;
         }
         else if (!strcmp(tok_str, "visible_fractions"))
            call_flags1 |= (7*CFLAG1_VISIBLE_FRACTION_BIT);
         else if (!strcmp(tok_str, "last_part_visible"))
            call_flags1 |= (3*CFLAG1_VISIBLE_FRACTION_BIT);
         else if (!strcmp(tok_str, "first_and_last_parts_visible"))
            call_flags1 |= (5*CFLAG1_VISIBLE_FRACTION_BIT);
         else if (!strcmp(tok_str, "need_three_numbers"))
            call_flags1 |= (3*CFLAG1_NUMBER_BIT);
         else if (!strcmp(tok_str, "optional_special_number"))
            call_flags1 |= (CFLAG1_NUMBER_MASK);
         else if (!strcmp(tok_str, "base_tag_call_2"))
            flag1_to_set = (3*CFLAG1_BASE_TAG_CALL_BIT);
         else if (!strcmp(tok_str, "yoyo_is_inherited"))        // These two sort of cheat.
            flagh_to_set = INHERITFLAG_YOYOETCK_YOYO;           // Set the "yoyo" bit.
         else if (!strcmp(tok_str, "gen_sting_is_inherited"))
            flagh_to_set = INHERITFLAG_YOYOETCK_GENEROUS;       // Set the other bit, which is called "generous".
         else if (!strcmp(tok_str, "mxn_is_inherited"))
            flagh_to_set = INHERITFLAG_MXNMASK;
         else if (!strcmp(tok_str, "nxn_is_inherited"))
            flagh_to_set = INHERITFLAG_NXNMASK;
         else if (!strcmp(tok_str, "bigmatrix_is_inherited"))
            flagh_to_set = INHERITFLAG_12_MATRIX|INHERITFLAG_16_MATRIX;
         else if (!strcmp(tok_str, "revert_is_inherited"))
            flagh_to_set = INHERITFLAG_REVERTMASK;
         else if ((iii = search(flagtabh)) >= 0)
            flagh_to_set = (1U << iii);
         else
            break;

         if ((call_flags1 & flag1_to_set) != 0)
            errexit("Redundant indicator");

         call_flags1 |= flag1_to_set;

         if ((call_flagsh & flagh_to_set) != 0)
            errexit("Redundant indicator");

         call_flagsh |= flagh_to_set;

         get_tok();
         if (tok_kind != tok_symbol) errexit("Missing indicator");
      }

      // Process the actual definition.  First, check for the "simple_funny" or "lateral_to_selectees" indicator.

      funnyflag = scan_for_per_array_def_flags();

      // Find out what kind of call it is.
      iii = search(schematab);

      if (iii < 0) errexit("Can't determine call definition type");

      ccc = (calldef_schema) iii;

      if (funnyflag != 0 && ccc != schema_by_array)
         errexit("Simple_funny or lateral_to_selectees out of place");

      write_call_header(ccc);

      switch (ccc) {
      case schema_by_array:
         write_array_def(funnyflag);
         break;
      case schema_nothing:
      case schema_nothing_noroll:
      case schema_nothing_other_elong:
      case schema_roll:
      case schema_recenter:
         get_tok_or_eof();
         break;
      case schema_matrix:
      case schema_partner_matrix:
      case schema_partner_partial_matrix:
         matrixflags = 0;

         for (;;) {     // Get matrix call options.
            get_tok();
            if (tok_kind != tok_symbol) break;
            if ((bit = search(matrixcallflagtab)) < 0) errexit("Unknown matrix call flag");
            matrixflags |= (1U << bit);
         }

         write_fullword(matrixflags);

         for ( ;; ) {
            write_callarray((ccc == schema_matrix) ? 2 : 16, true);
            get_tok_or_eof();

            if (tok_kind != tok_symbol || strcmp(tok_str, "alternate_definition"))
               break;

            process_alt_def_header();
            get_tok();
         }

         break;
      case schema_sequential:
      case schema_split_sequential:
      case schema_sequential_with_fraction:
      case schema_sequential_with_split_1x8_id:
      case schema_sequential_alternate:
      case schema_sequential_remainder:
         write_seq_stuff();

         for (;;) {               // Write a level 2 seqdefine group.
            get_tok_or_eof();
            if (eof) goto startagain;
            else if ((tok_kind == tok_symbol) && (!strcmp(tok_str, "seq"))) {
               // Write a level 2 seqdefine group.
               write_seq_stuff();
            }
            else break;
         }

         break;
      case schema_alias:
         if (call_flagsh|call_flags1|call_flags1overflow|call_tag)
            errexit("Flags not allowed with alias");
         get_tok();
         if (tok_kind != tok_symbol) errexit("Improper alias symbol");
         write_halfword(0x4000 | tagsearch(0));
         get_tok();
         break;
      default:
         write_conc_stuff(ccc);
         get_tok_or_eof();
         break;
      }

      // End of call definition.  See if there is another definition for this call.

      if ((tok_kind == tok_symbol) && (strcmp(tok_str, "call"))) {
         // Yes.  Process the next definition.
         if (ccc == schema_alias) errexit("Compound definitions not allowed on alias");
         call_namelen = 0;
         goto restart_compound_call;
      }
      else
         goto startagain;       // Must have seen next 'call' indicator.
   }

   write_halfword(0);           // Final end mark.

   dumbflag = 0;

   for (i = 0; i < tagtabsize; i++) {
      if (!tagtab[i].def) {
         if (!dumbflag++) printf("Tags not defined:\n");
         printf("   %s\n", tagtab[i].s);
      }
   }

   int result = fclose(db_input);
   db_input = NULL;
   if (result != 0) {
      fprintf(stderr, "Error reading input file ");
      perror(db_input_filename);
      exit(1);
   }

   printf("%d bytes written, %d calls\n", filecount, callcount);

   if (fseek(db_output, 4, SEEK_SET)) {
      db_output_error();
   }

   write_halfword(callcount);
   write_halfword(tagtabsize);

   result = fclose(db_output);
   db_output = NULL;
   if (result != 0)
      db_output_error();

   // We have, of course, leaked all of tagtab and the strings therein.

   return 0;
}
