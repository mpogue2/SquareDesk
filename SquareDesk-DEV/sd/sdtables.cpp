// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2015  William B. Ackerman.
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

/* This defines the following external variables:
   direction_names
   selector_list
   warning_strings
   s_2x2_2x4
   s_2x2_2x4b
   s_2x2_2x4_ends
   s_2x2_2x4_endsb
   s_qtg_2x4
   s_2x3_qtg
   s_4x4_4x6a
   s_4x4_4x6b
   s_4x4_4dma
   s_4x4_4dmb
   s_23232_4x5a
   s_23232_4x5b
   s_c1phan_4x4a
   s_c1phan_4x4b
   s_dmd_323
   s_1x4_dmd
   s_qtg_3x4
   s_short6_2x3
   s_bigrig_dblbone
   s_bigbone_dblrig
   identity24
   expand::init_table
   map::map_init_table
   * Try to do something sensible with these. *
   rear_1x2_pair
   rear_2x2_pair
   rear_bone_pair
   step_8ch_pair
   step_qtag_pair
   step_2x2h_pair
   step_2x2v_pair
   step_spindle_pair
   step_dmd_pair
   step_tgl_pair
   step_ptpd_pair
   step_qtgctr_pair
   touch_init_table1
   touch_init_table2
   touch_init_table3
   conc_tables::conc_init_table
   merge_table::merge_init_table
   tgl3_0
   tgl3_1
   tgl4_0
   tgl4_1
   squeezethingglass
   squeezethinggal
   squeezething343
   squeezethingqtag
   squeezething4dmd
   squeezefinalglass
   truck_to_ptpd
   truck_to_deepxwv
   press_4dmd_4x4
   press_4dmd_qtag1
   press_4dmd_qtag2
   press_qtag_4dmd1
   press_qtag_4dmd2
   acc_crosswave
   id_bit_table_2x5_z
   id_bit_table_2x5_ctr6
   id_bit_table_d2x7a
   id_bit_table_d2x7b
   id_bit_table_d2x5_ctr6
   id_bit_table_2x6_pg
   id_bit_table_bigdmd_wings
   id_bit_table_bigbone_wings
   id_bit_table_bighrgl_wings
   id_bit_table_bigdhrgl_wings
   id_bit_table_bigh_dblbent
   id_bit_table_3x4_h
   id_bit_table_3x4_ctr6
   id_bit_table_3x4_offset
   id_bit_table_3x4_corners
   id_bit_table_3x4_zs
   id_bit_table_butterfly
   id_bit_table_4x4_outer_pairs
   id_bit_table_525_nw
   id_bit_table_525_ne
   id_bit_table_343_outr
   id_bit_table_343_innr
   id_bit_table_545_outr
   id_bit_table_545_innr
   id_bit_table_3dmd_in_out
   id_bit_table_3dmd_ctr1x6
   id_bit_table_3dmd_ctr1x4
   id_bit_table_4dmd_cc_ee
   id_bit_table_3ptpd
   id_bit_table_wqtag_hollow
   id_bit_table_3x6_with_1x6
   setup_attrs
   schema_attrs
   map::spec_map_table
   dead_masks
   masks_for_3x4
   masks_for_3dmd_ctr2
   masks_for_3dmd_ctr4
   masks_for_bigh_ctr4
   masks_for_4x4
   clw3_table
   select::fixer_init_table
   select::sel_init_table
   tglmap::init_table
   c1tglmap1
   c1tglmap2
   dbqtglmap1
   dbqtglmap2
   qttglmap1
   qttglmap2
   bdtglmap1
   bdtglmap2
   rgtglmap1
   rgtglmap2
   rgtglmap3
   ritglmap1
   ritglmap2
   ritglmap3
*/

#include "sd.h"

/* BEWARE!!  This list is keyed to the definition of "direction_kind" in sd.h,
   and to the necessary stuff in SDUI. */
direction_item direction_names[] = {
   {"???",            "???"},
   {"(no direction)", "(NO DIRECTION)"},
   {"left",           "LEFT"},
   {"right",          "RIGHT"},
   {"in",             "IN"},
   {"out",            "OUT"},
   {"back",           "BACK"},
   {"zig-zag",        "ZIG-ZAG"},
   {"zag-zig",        "ZAG-ZIG"},
   {"zig-zig",        "ZIG-ZIG"},
   {"zag-zag",        "ZAG-ZAG"},
   {"the music",      "THE MUSIC"},
   {(Cstring) 0,      (Cstring) 0}};

// BEWARE!!  This list is keyed to the definition of "selector_kind" in sd.h .
selector_item selector_list[] = {
   {"???",          "???",         "???",          "???",         selector_uninitialized},
   {"boys",         "boy",         "BOYS",         "BOY",         selector_girls},
   {"girls",        "girl",        "GIRLS",        "GIRL",        selector_boys},
   {"heads",        "head",        "HEADS",        "HEAD",        selector_sides},
   {"sides",        "side",        "SIDES",        "SIDE",        selector_heads},
   {"head corners", "head corner", "HEAD CORNERS", "HEAD CORNER", selector_sidecorners},
   {"side corners", "side corner", "SIDE CORNERS", "SIDE CORNER", selector_headcorners},
   {"head boys",    "head boy",    "HEAD BOYS",    "HEAD BOY",    selector_uninitialized},
   {"head girls",   "head girl",   "HEAD GIRLS",   "HEAD GIRL",   selector_uninitialized},
   {"side boys",    "side boy",    "SIDE BOYS",    "SIDE BOY",    selector_uninitialized},
   {"side girls",   "side girl",   "SIDE GIRLS",   "SIDE GIRL",   selector_uninitialized},
   {"centers",      "center",      "CENTERS",      "CENTER",      selector_ends},
   {"ends",         "end",         "ENDS",         "END",         selector_centers},
   {"leads",        "lead",        "LEADS",        "LEAD",        selector_trailers},
   {"trailers",     "trailer",     "TRAILERS",     "TRAILER",     selector_leads},
   {"lead beaus",   "lead beau",   "LEAD BEAUS",   "LEAD BEAU",   selector_uninitialized},
   {"lead belles",  "lead belle",  "LEAD BELLES",  "LEAD BELLE",  selector_uninitialized},
   {"lead ends",    "lead end",    "LEAD ENDS",    "LEAD END",    selector_uninitialized},
   {"lead centers", "lead center", "LEAD CENTERS", "LEAD CENTER", selector_uninitialized},
   {"trailing beaus","trailing beau","TRAILING BEAUS","TRAILING BEAU",selector_uninitialized},
   {"trailing belles","trailing belle","TRAILING BELLES","TRAILING BELLE",selector_uninitialized},
   {"trailing ends","trailing end","TRAILING ENDS","TRAILING END",selector_uninitialized},
   {"trailing centers","trailing center","TRAILING CENTERS","TRAILING CENTER",selector_uninitialized},
   {"end boys",     "end boy",     "END BOYS",     "END BOY",     selector_uninitialized},
   {"end girls",    "end girl",    "END GIRLS",    "END GIRL",    selector_uninitialized},
   {"center boys",  "center boy",  "CENTER BOYS",  "CENTER BOY",  selector_uninitialized},
   {"center girls", "center girl", "CENTER GIRLS", "CENTER GIRL", selector_uninitialized},
   {"beaus",        "beau",        "BEAUS",        "BEAU",        selector_belles},
   {"belles",       "belle",       "BELLES",       "BELLE",       selector_beaus},
   {"center 2",     "center 2",    "CENTER 2",     "CENTER 2",    selector_outer6},
   {"very centers", "very center", "VERY CENTERS", "VERY CENTER", selector_outer6},
   {"center 6",     "center 6",    "CENTER 6",     "CENTER 6",    selector_outer2},
   {"outer 2",      "outer 2",     "OUTER 2",      "OUTER 2",     selector_center6},
   {"very ends",    "very end",    "VERY ENDS",    "VERY END",    selector_center6},
   {"outer 6",      "outer 6",     "OUTER 6",      "OUTER 6",     selector_center2},
   {"center diamond", "center diamond", "CENTER DIAMOND", "CENTER DIAMOND",    selector_uninitialized},
   {"center 1x4",   "center 1x4",  "CENTER 1X4",   "CENTER 1X4",  selector_uninitialized},
   {"center 1x6",   "center 1x6",  "CENTER 1X6",   "CENTER 1X6",  selector_uninitialized},
   {"outer 1x3s",   "outer 1x3s",  "OUTER 1X3s",   "OUTER 1X3s",  selector_uninitialized},
   {"center 4",     "center 4",    "CENTER 4",     "CENTER 4",    selector_outerpairs},
   {"center wave",  "center wave", "CENTER WAVE",  "CENTER WAVE", selector_outerpairs},
   {"center line",  "center line", "CENTER LINE",  "CENTER LINE", selector_outerpairs},
   {"center column","center column","CENTER COLUMN","CENTER COLUMN",selector_outerpairs},
   {"center box",   "center box",  "CENTER BOX",   "CENTER BOX",  selector_outerpairs},
   {"center wave of 6",  "center wave of 6", "CENTER WAVE OF 6",  "CENTER WAVE OF 6", selector_uninitialized},
   {"center line of 6",  "center line of 6", "CENTER LINE OF 6",  "CENTER LINE OF 6", selector_uninitialized},
   {"center column of 6","center column of 6","CENTER COLUMN OF 6","CENTER COLUMN OF 6",selector_uninitialized},
   {"outer pairs",  "outer pair",  "OUTER PAIRS",  "OUTER PAIR",  selector_center4},
   {"first 1",      "first 1",     "FIRST 1",      "FIRST 1",     selector_uninitialized},
   {"last 1",       "last 1",      "LAST 1",       "LAST 1",      selector_uninitialized},
   {"first 2",      "first 2",     "FIRST 2",      "FIRST 2",     selector_uninitialized},
   {"last 2",       "last 2",      "LAST 2",       "LAST 2",      selector_uninitialized},
   {"first 3",      "first 3",     "FIRST 3",      "FIRST 3",     selector_uninitialized},
   {"last 3",       "last 3",      "LAST 3",       "LAST 3",      selector_uninitialized},
   {"first 4",      "first 4",     "FIRST 4",      "FIRST 4",     selector_uninitialized},
   {"last 4",       "last 4",      "LAST 4",       "LAST 4",      selector_uninitialized},
   {"leftmost 1",   "leftmost 1",  "LEFTMOST 1",   "LEFTMOST 1",  selector_uninitialized},
   {"rightmost 1",  "rightmost 1", "RIGHTMOST 1",  "RIGHTMOST 1", selector_uninitialized},
   {"leftmost 2",   "leftmost 2",  "LEFTMOST 2",   "LEFTMOST 2",  selector_uninitialized},
   {"rightmost 2",  "rightmost 2", "RIGHTMOST 2",  "RIGHTMOST 2", selector_uninitialized},
   {"leftmost 3",   "leftmost 3",  "LEFTMOST 3",   "LEFTMOST 3",  selector_uninitialized},
   {"rightmost 3",  "rightmost 3", "RIGHTMOST 3",  "RIGHTMOST 3", selector_uninitialized},
   {"leftmost 4",   "leftmost 4",  "LEFTMOST 4",   "LEFTMOST 4",  selector_uninitialized},
   {"rightmost 4",  "rightmost 4", "RIGHTMOST 4",  "RIGHTMOST 4", selector_uninitialized},
   {"headliners",   "headliner",   "HEADLINERS",   "HEADLINER",   selector_sideliners},
   {"sideliners",   "sideliner",   "SIDELINERS",   "SIDELINER",   selector_headliners},
   {"those facing", "those facing","THOSE FACING", "THOSE FACING",selector_uninitialized},
   {"everyone",     "everyone",    "EVERYONE",     "EVERYONE",    selector_uninitialized},
   {"all",          "all",         "ALL",          "ALL",         selector_uninitialized},
   {"no one",       "no one",      "NO ONE",       "NO ONE",      selector_uninitialized},
   // Start of unsymmetrical selectors.
   {"near line",    "near line",   "NEAR LINE",    "NEAR LINE",   selector_uninitialized},
   {"far line",     "far line",    "FAR LINE",     "FAR LINE",    selector_uninitialized},
   {"near column",  "near column", "NEAR COLUMN",  "NEAR COLUMN", selector_uninitialized},
   {"far column",   "far column",  "FAR COLUMN",   "FAR COLUMN",  selector_uninitialized},
   {"near box",     "near box",    "NEAR BOX",     "NEAR BOX",    selector_uninitialized},
   {"far box",      "far box",     "FAR BOX",      "FAR BOX",     selector_uninitialized},
   {"line on the caller's left", "line on the caller's left",
    "LINE ON THE CALLER'S LEFT", "LINE ON THE CALLER'S LEFT", selector_uninitialized},
   {"line on the caller's right", "line on the caller's right",
    "LINE ON THE CALLER'S RIGHT", "LINE ON THE CALLER'S RIGHT", selector_uninitialized},
   {"column on the caller's left", "column on the caller's left",
    "COLUMN ON THE CALLER'S LEFT", "COLUMN ON THE CALLER'S LEFT", selector_uninitialized},
   {"column on the caller's right", "column on the caller's right",
    "COLUMN ON THE CALLER'S RIGHT", "COLUMN ON THE CALLER'S RIGHT", selector_uninitialized},
   {"box on the caller's left", "box on the caller's left",
    "BOX ON THE CALLER'S LEFT", "BOX ON THE CALLER'S LEFT", selector_uninitialized},
   {"box on the caller's right", "box on the caller's right",
    "BOX ON THE CALLER'S RIGHT", "BOX ON THE CALLER'S RIGHT", selector_uninitialized},
   {"near 4",       "near 4",      "NEAR 4",       "NEAR 4",      selector_farfour},
   {"far 4",        "far 4",       "FAR 4",        "FAR 4",       selector_nearfour},
   {"near 2",       "near 2",      "NEAR 2",       "NEAR 2",      selector_farsix},
   {"far 2",        "far 2",       "FAR 2",        "FAR 2",       selector_nearsix},
   {"near 6",       "near 6",      "NEAR 6",       "NEAR 6",      selector_fartwo},
   {"far 6",        "far 6",       "FAR 6",        "FAR 6",       selector_neartwo},
   {"near 3",       "near 3",      "NEAR 3",       "NEAR 3",      selector_farfive},
   {"far 3",        "far 3",       "FAR 3",        "FAR 3",       selector_nearfive},
   {"near 5",       "near 5",      "NEAR 5",       "NEAR 5",      selector_farthree},
   {"far 5",        "far 5",       "FAR 5",        "FAR 5",       selector_nearthree},
   {"the 2x3",      "the 2x3",     "THE 2X3",      "THE 2X3",     selector_uninitialized},
   {"the diamond",  "the diamond", "THE DIAMOND",  "THE DIAMOND", selector_uninitialized},
   {"the line",     "the line",    "THE LINE",     "THE LINE",    selector_uninitialized},
   {"the column",   "the column",  "THE COLUMN",   "THE COLUMN",  selector_uninitialized},
   {"those facing the caller", "those facing the caller",
    "THOSE FACING THE CALLER", "THOSE FACING THE CALLER",         selector_uninitialized},
   {"those facing away from the caller", "those facing away from the caller",
    "THOSE FACING AWAY FROM THE CALLER", "THOSE FACING AWAY FROM THE CALLER", selector_uninitialized},
   {"those facing the caller's left", "those facing the caller's left",
    "THOSE FACING THE CALLER'S LEFT", "THOSE FACING THE CALLER'S LEFT", selector_uninitialized},
   {"those facing the caller's right", "those facing the caller's right",
    "THOSE FACING THE CALLER'S RIGHT", "THOSE FACING THE CALLER'S RIGHT", selector_uninitialized},
   {"farthest person", "farthest person", "FARTHEST PERSON", "FARTHEST PERSON", selector_uninitialized},
   {"nearest person", "nearest person", "NEAREST PERSON", "NEAREST PERSON", selector_uninitialized},
   {"#1 boy",       "#1 boy",      "#1 BOY",       "#1 BOY",      selector_uninitialized},
   {"#1 girl",      "#1 girl",     "#1 GIRL",      "#1 GIRL",     selector_uninitialized},
   {"#1 couple",    "#1 couple",   "#1 COUPLE",    "#1 COUPLE",   selector_uninitialized},
   {"#2 boy",       "#2 boy",      "#2 BOY",       "#2 BOY",      selector_uninitialized},
   {"#2 girl",      "#2 girl",     "#2 GIRL",      "#2 GIRL",     selector_uninitialized},
   {"#2 couple",    "#2 couple",   "#2 COUPLE",    "#2 COUPLE",   selector_uninitialized},
   {"#3 boy",       "#3 boy",      "#3 BOY",       "#3 BOY",      selector_uninitialized},
   {"#3 girl",      "#3 girl",     "#3 GIRL",      "#3 GIRL",     selector_uninitialized},
   {"#3 couple",    "#3 couple",   "#3 COUPLE",    "#3 COUPLE",   selector_uninitialized},
   {"#4 boy",       "#4 boy",      "#4 BOY",       "#4 BOY",      selector_uninitialized},
   {"#4 girl",      "#4 girl",     "#4 GIRL",      "#4 GIRL",     selector_uninitialized},
   {"#4 couple",    "#4 couple",   "#4 COUPLE",    "#4 COUPLE",   selector_uninitialized},
   {"couples 1 and 2", "couple 1 and 2", "COUPLES 1 AND 2", "COUPLE 1 AND 2",
                                                                  selector_uninitialized},
   {"couples 2 and 3", "couple 2 and 3", "COUPLES 2 AND 3", "COUPLE 2 AND 3",
                                                                  selector_uninitialized},
   {"couples 3 and 4", "couple 3 and 4", "COUPLES 3 AND 4", "COUPLE 3 AND 4",
                                                                  selector_uninitialized},
   {"couples 1 and 4", "couple 1 and 4", "COUPLES 1 AND 4", "COUPLE 1 AND 4",
                                                                  selector_uninitialized},

   // These are in the "selector_SOME_START" section.  They are only parsed for "<any> are/work tandem" things.
   {"some",         "some",        "SOME",         "SOME",        selector_uninitialized},
   {"inside triangles","inside triangle","INSIDE TRIANGLES","INSIDE TRIANGLE",selector_uninitialized},
   {"outside triangles","outside triangle","OUTSIDE TRIANGLES","OUTSIDE TRIANGLE",selector_uninitialized},
   {"in point triangles","in point triangle","IN POINT TRIANGLES","IN POINT TRIANGLE",selector_uninitialized},
   {"out point triangles","out point triangle","OUT POINT TRIANGLES","OUT POINT TRIANGLE",selector_uninitialized},

   // These are in the "selector_INVISIBLE_START" section.  They are never parsed.
   {"Mbeaus???",    "Mbeaus???",   "MBEAUS???",    "MBEAUS???",   selector_uninitialized},
   {"Mbelles???",   "Mbelles???",  "MBELLES???",   "MBELLES???",  selector_uninitialized},
   {"notctr dmd???","notctr dmd???","NOTCTR DMD???","NOTCTR DMD???",selector_uninitialized},
   {(Cstring) 0,    (Cstring) 0,   (Cstring) 0,    (Cstring) 0,   selector_uninitialized}};

// BEWARE!!  These strings are keyed to the definition of "warning_index" in sdbase.h .
// A "*" as the first character means that this warning precludes acceptance while searching.
// A "+" as the first character means that this warning is cleared if a
//   concentric call was done and the "suppress_elongation_warnings" flag was on.
// A "=" as the first character means that this warning is cleared if it arises
//   during some kind of "do your part" call.
Cstring warning_strings[] = {
   /*  warn__none                */   " Unknown warning????",
   /*  warn__really_no_collision */   " Unknown no_collision warning????",
   /*  warn__really_no_eachsetup */   "=Unknown each-setup warning????",
   /*  warn__do_your_part        */   "*Do your part.",
   /*  warn__unusual_or_2faced   */   "*This is an unusual setup for this call.",
   /*  warn__tbonephantom        */   " This is a T-bone phantom setup call.  Everyone will do their own part.",
   /*  warn__awkward_centers     */   "*Awkward for centers.",
   /*  warn__overcast            */   "*Possible excessive turning.",
   /*  warn__bad_concept_level   */   "*This concept is not allowed at this level.",
   /*  warn__not_funny           */   "*That wasn't funny.",
   /*  warn__hard_funny          */   "*Very difficult funny concept.",
   /*  warn__rear_back           */   "*Rear back from the handhold.",
   /*  warn__awful_rear_back     */   "*Rear back from the handhold -- this is very unusual.",
   /*  warn__excess_split        */   "*Split concept seems to be superfluous here.",
   /*  warn__lineconc_perp       */   "+Ends should opt for setup perpendicular to their original line.",
   /*  warn__dmdconc_perp        */   "+Ends should opt for setup perpendicular to their original diamond points.",
   /*  warn__lineconc_par        */   "+Ends should opt for setup parallel to their original line -- concentric rule does not apply.",
   /*  warn__dmdconc_par         */   "+Ends should opt for setup parallel to their original diamond points -- concentric rule does not apply.",
   /*  warn__xclineconc_perpc    */   "+New ends should opt for setup perpendicular to their original (center) line.",
   /*  warn__xcdmdconc_perpc     */   "+New ends should opt for setup perpendicular to their original (center) diamond points.",
   /*  warn__xclineconc_perpe    */   "+New ends should opt for setup perpendicular to their original (center) line.  Beware:  This may be controversial.",
   /*  warn__ctrstand_endscpls   */   " Centers work in tandem, ends as couples.",
   /*  warn__ctrscpls_endstand   */   " Centers work as couples, ends in tandem.",
   /*  warn__each2x2             */   "=Each 2x2.",
   /*  warn__each1x4             */   "=Each 1x4.",
   /*  warn__each1x2             */   "=Each 1x2.",
   /*  warn__eachdmd             */   "=Each diamond.",
   /*  warn__take_right_hands    */   " Take right hands.",
   /*  warn__1_4_pgram           */   " 25% offset parallelogram.",
   /*  warn__full_pgram          */   " Completely offset parallelogram.",
   /*  warn__3_4_pgram           */   " 75% offset parallelogram.",
   /*  warn__offset_gone         */   " The offset goes away.",
   /*  warn__overlap_gone        */   " The overlap goes away.",
   /*  warn__to_o_spots          */   " Go back to 'O' spots.",
   /*  warn__to_x_spots          */   " Go back to butterfly spots.",
   /*  warn__check_butterfly     */   " Check a butterfly.",
   /*  warn__check_galaxy        */   " Check a galaxy.",
   /*  warn__some_rear_back      */   " Some people rear back.",
   /*  warn__centers_rear_back_staying_in_center */   " Centers rear back, working in the center.",
   /*  warn__not_tbone_person    */   " Work with the person to whom you are not T-boned.",
   /*  warn__check_c1_phan       */   " Check a 'C1 phantom' setup.",
   /*  warn__check_dmd_qtag      */   " Fudge to a diamond/quarter-tag setup.",
   /*  warn__check_quad_dmds     */   " Fudge to quadruple diamonds.",
   /*  warn__may_be_fudgy        */   " This may be slightly fudgy.",
   /*  warn__fudgy_half_offset   */   " This may be slightly fudgy -- assume 50% offset.",
   /*  warn__check_3x4           */   " Check a 3x4 setup.",
   /*  warn__check_2x4           */   " Check a 2x4 setup.",
   /*  warn__check_hokey_2x4     */   "*Fudge to a 2x4 -- the dancers might not like this.",
   /*  warn__check_4x4           */   "*Check a 4x4 setup.",
   /*  warn__check_hokey_4x5     */   "*Fudge to a 4x5 -- the dancers might not like this.",
   /*  warn__check_4x6           */   "*Check a 4x6 setup.",
   /*  warn__check_hokey_4x4     */   "*Check a center box and outer lines/columns.",
   /*  warn__check_4x4_start     */   "*Check a 4x4 setup at the start of this call.",
   /*  warn__check_4x4_ctrbox    */   "*Check a center box of 4 at the start of this call.",
   /*  warn__check_3dmd_is_wide  */   "*The people in the corners need to spread out to outer triple diamonds.",
   /*  warn__check_centered_qtag */   "*Each 8-person twin general 1/4 tag is centered, with the outsides directly adjacent.",
   /*  warn__check_pgram         */   " Opt for a parallelogram.",
   /*  warn__check_hokey_thar     */   "*Fudge to a thar -- the dancers might not like this.",
   /*  warn__ctrs_stay_in_ctr    */   " Centers stay in the center.",
   /*  warn__meta_on_xconc       */   "*Misleading meta-concept combination.",
   /*  warn__check_c1_stars      */   " Check 'stars'.",
   /*  warn__check_gen_c1_stars  */   " Check a generalized 'star' setup.",
   /*  warn__bigblock_feet       */   " Bigblock/stagger shapechanger -- go to footprints.",
   /*  warn__bigblockqtag_feet   */   " Adjust to bigblock/stagger 1/4 tag footprints.",
   /*  warn__diagqtag_feet       */   " Adjust to diagonal 1/4 tag footprints on other diagonal.",
   /*  warn__adjust_to_feet      */   " Adjust back to footprints.",
   /*  warn__some_touch          */   " Some people step to a wave.",
   /*  warn__some_touch_evil     */   " Some people step to a miniwave -- this may be problematical.",
   /*  warn__split_to_2x4s       */   "=Do the call in each 2x4.",
   /*  warn__split_to_2x3s       */   "=Do the call in each 2x3.",
   /*  warn__split_to_1x8s       */   "=Do the call in each 1x8.",
   /*  warn__split_to_1x6s       */   "=Do the call in each 1x6.",
   /*  warn__split_to_1x3s       */   "=Do the call in each 1x3.",
   /*  warn__split_to_1x3s_always*/   " Do the call in each 1x3.",
   /*  warn__take_left_hands     */   " Take left hands, since this call is being done mirror.",
   /*  warn__left_half_pass      */   " Take left hands due to left shoulder pass.",
   /*  warn__evil_interlocked    */   " Interlocked phantom shape-changers are very evil.",
   /*  warn__split_phan_in_pgram */   " The split phantom setups are directly adjacent to the real people.",
   /*  warn__bad_interlace_match */   "*The interlaced calls have mismatched lengths.",
   /*  warn__not_on_block_spots  */   " Generalized bigblock/stagger -- people are not on block spots.",
   /*  warn__stupid_phantom_clw  */   "#This use of phantom setups seems superfluous.",
   /*  warn__should_say_Z        */   "*You probably should say 'Z' lines/waves/columns.",
   /*  warn__bad_modifier_level  */   "*Use of this modifier on this call is not allowed at this level.",
   /*  warn__bad_call_level      */   "*This call is not really legal at this level.",
   /*  warn__did_not_interact    */   "*The setups did not interact with each other.",
   /*  warn__use_quadruple_setup_instead*/"*The setups did not interact; maybe should say 'quadruple formation' instead.",
   /*  warn__opt_for_normal_cast */   "*If in doubt, assume a normal cast.",
   /*  warn__opt_for_normal_hinge*/   "*If in doubt, assume a normal hinge.",
   /*  warn__opt_for_2fl         */   "*If in doubt, assume a two-faced line.",
   /*  warn__opt_for_no_collision*/   "*If in doubt, assume no collision.",
   /*  warn__opt_for_not_tboned_base*/"*If in doubt, assume triangle base is not T-boned.",
   /*  warn_partial_solomon      */   "*For the center line or column, the offset goes away.",
   /*  warn_same_z_shear         */   "*Make the outside Z's have the same shear as the center one.",
   /*  warn__like_linear_action  */   "*Ends start like a linear action -- this may be controversial.",
   /*  warn__phantoms_thinner    */   "*Phantoms may have gotten thinner -- go to outer triple boxes.",
   /*  warn__hokey_jay_shapechanger */"*This shapechanger in a jay may be controversial.",
   /*  warn_interlocked_to_6     */   "*This went from 4 interlocked groups to 6.",
   /*  warn__offset_hard_to_see  */   "*The offset setup is hard to see.",
   /*  warn__pg_hard_to_see      */   "*The parallelogram setup is hard to see.",
   /*  warn__phantom_judge       */   "*The judge or socker may be a phantom.",
   /*  warn__colocated_once_rem  */   " The once-removed setups have the same center.",
   /*  warn_big_outer_triangles  */   "*The outside triangles are very large.",
   /*  warn_hairy_fraction       */   " Fraction is very complicated.",
   /*  warn_bad_collision        */   "*This collision may be controversial.",
   /*  warn_very_bad_collision   */   "*This collision appears to be improper.",
   /*  warn_some_singlefile      */   " Some work single file.",
   /*  warn__dyp_resolve_ok      */   " Do your part.",
   /*  warn__unusual             */   "*This is an unusual setup for this call.",
   /*  warn_controversial        */   "*This may be controversial.",
   /*  warn_verycontroversial    */   "*Change offset to other axis, this may be seriously controversial.",
   /*  warn_serious_violation    */   "*This appears to be a serious violation of the definition.",
   /*  warn__4_circ_tracks       */   "*Original ends of lines stay outside of the others.",
   /*  warn__assume_dpt          */   "*Assume a starting DPT.",
   /*  warn_bogus_yoyo_rims_hubs */   "*Using incorrect definition of rims/hubs trade.",
   /*  warn__centers_are_diamond */   "*The centers are the diamond.",
   /*  warn_pg_in_2x6            */   "*Offset the resulting 2x6 by 50%, or 3 positions.",
   /*  warn__deprecate_pg_3box   */   "*Parallelogram triple boxes is deprecated.  Use offset triple boxes instead.",
   /*  warn_real_people_spots    */   "*The distorted setup is formed by the real people.",
   /*  warn__tasteless_com_spot  */   "*Not all common-spot people had right hands.",
   /*  warn__tasteless_junk      */   "*The algorithmic nondeterminism of this usage is truly extraordinary.",
   /*  warn__tasteless_slide_thru*/   "*Slide thru from left-handed miniwave may be controversial.",
   /*  warn__went_to_other_side  */   "*People went to the other side; is this really what you want?",
   /*  warn__horrible_conc_hinge */   "*Extremely awkward for outsides; is this really what you want?",
   /*  warn__this_is_tight       */   "*This is tight; it may not be a good combination.",
   /*  warn__compress_carefully  */   "*Preserve the phantom spots internal to the outer setups.",
   /*  warn__brute_force_mxn     */   "*Check that you are satisfied, and that the dancers will be satisfied, that this usage is correct, effective, and unambiguous.",
   /*  warn__two_faced           */   "*Not a wave -- maybe should say 'two-faced'.",
   /*  warn__cant_track_phantoms */   "*The phantoms may not have been tracked correctly.",
   /*  warn__crazy_tandem_interaction */  "*Using \"crazy\" on decomposed tandem-like operation.",
   /*  warn__mimic_ambiguity_checked */"*The mimic was ambiguous, but the result is the same either way.",
   /*  warn__mimic_ambiguity_resolved */"*The mimic was ambiguous, but only one assumption works.",
   /*  warn__diagnostic          */   "*This is a diagnostic warning and should never arise."};



const expand::thing s_2x2_2x4_ctrs = {{1, 2, 5, 6}, s2x2, s2x4, 0};
const expand::thing s_2x2_2x4_ctrsb = {{6, 1, 2, 5}, s2x2, s2x4, 1};
const expand::thing s_2x2_2x4_ends = {{0, 3, 4, 7}, s2x2, s2x4, 0};
const expand::thing s_2x2_2x4_endsb = {{7, 0, 3, 4}, s2x2, s2x4, 1};
const expand::thing s_1x4_1x8_ctrs = {{3, 2, 7, 6}, s1x4, s1x8, 0};
const expand::thing s_1x4_1x8_ends = {{0, 1, 4, 5}, s1x4, s1x8, 0};
const expand::thing s_qtg_2x4 = {{5, -1, -1, 0, 1, -1, -1, 4}, s2x4, s_qtag, 1};
const expand::thing s_4x4_4x6a = {
   {4, 7, 22, 8, 13, 14, 15, 21, 16, 19, 10, 20, 1, 2, 3, 9}, s4x4, s4x6, 0};
const expand::thing s_4x4_4x6b = {
   {1, 2, 3, 9, 4, 7, 22, 8, 13, 14, 15, 21, 16, 19, 10, 20}, s4x4, s4x6, 1};
const expand::thing s_4x4_4dma = {
   {0, 1, 2, -1, 3, -1, -1, -1, 8, 9, 10, -1, 11, -1, -1, -1}, s4x4, s4dmd, 1};
const expand::thing s_4x4_4dmb = {
   {3, -1, -1, -1, 8, 9, 10, -1, 11, -1, -1, -1, 0, 1, 2, -1}, s4x4, s4dmd, 0};

const expand::thing s_23232_4x5a = {
   {-1, 3, 9, 7, 5, 6, -1, 13, 19, 17, 15, 16}, s_23232, s4x5, 0};
const expand::thing s_23232_4x5b = {
   {1, -1, 9, 7, 5, 18, 11, -1, 19, 17, 15, 8}, s_23232, s4x5, 0};

const expand::thing s_c1phan_4x4a = {
   {-1, 13, -1, 15, -1, 1, -1, 3, -1, 5, -1, 7, -1, 9, -1, 11}, s_c1phan, s4x4, 0};
const expand::thing s_c1phan_4x4b = {
   {10, -1, 15, -1, 14, -1, 3, -1, 2, -1, 7, -1, 6, -1, 11, -1}, s_c1phan, s4x4, 0};
const expand::thing s_1x4_dmd = {{-1, 1, -1, 3}, sdmd, s1x4, 1};
/* s_qtg_3x4 is duplicated in the big table. */
const expand::thing s_qtg_3x4 = {{1, 2, 4, 5, 7, 8, 10, 11}, s_qtag, s3x4, 0, 0, 06666};
const expand::thing s_short6_2x3 = {{0, 1, 2, 3, 4, 5}, s2x3, s_short6, 0};
// This is duplicated in the big table.
const expand::thing s_bigrig_dblbone = {{6, 7, 3, 2, 1, 12, 14, 15, 11, 10, 9, 4},
                                        sbigrig, sdblbone, 0};
// This is duplicated in the big table.
const expand::thing s_bigbone_dblrig = {{0, 1, 3, 2, 12, 13, 8, 9, 11, 10, 4, 5},
                                        sbigbone, sdblrig, 0};


const veryshort identity24[24] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};


// This isn't declared const!  Why not?  Because there are pointers
// linking the hash chains, that get filled in during initialization.
// None of the data that you see here ever changes.
expand::thing expand::init_table[] = {

   {{5, 7, 0, 1, 3, 4},
    s_short6, s_rigger, 1, 0U, 0x44, false,
    warn__none, warn__none, normalize_before_merge, 0},

   // This thing compresses a short6 to a 2x2.
   {{0, 2, 3, 5},
    s2x2, s_short6, 0, 0U, 022U, false,
    warn__none, warn__none, normalize_to_4, 0},

   // And this one compresses a bigptpd.
   {{2, 4, 3, 1, 8, 10, 9, 7},
    s_ptpd, sbigptpd, 0, 0U, 04141U, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   {{3, 11},
    s1x2, sdblspindle, 0, 0U, 0xF7F7U, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   // Unsymmetrical recenter items.

   {{10, 15, 3, 1, 2, 7, 4, 5, 6, 8, 9, 11},
    s3x4, s4x4, 0, 0U, 0x7001, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{12, 13, 14, 0, 1, 3, 2, 7, 11, 9, 10, 15},
    s3x4, s4x4, 0, 0U, 0x0170, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{6, 11, 15, 13, 14, 3, 0, 1, 2, 4, 5, 7},
    s3x4, s4x4, 1, 0U, 0x1700, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{8, 9, 10, 12, 13, 15, 14, 3, 7, 5, 6, 11},
    s3x4, s4x4, 1, 0U, 0x0017, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{10, 11, 5, 4, 6, 7, 8, 9},
    s2x4, s3x4, 0, 0U, 0x00F, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{0, 1, 2, 3, 4, 5, 11, 10},
    s2x4, s3x4, 0, 0U, 0x3C0, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{0, 1, 3, 2, 7, 8, 4, 9},
    s1x8, s1x10, 0, 0U, 0x060, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{2, 3, 9, 4, 5, 6, 8, 7},
    s1x8, s1x10, 0, 0U, 0x003, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{0, 1, 3, 2, 10, 11, 4, 5},
    s1x8, s1x12, 0, 0U, 0x3C0, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{4, 5, 10, 11, 6, 7, 9, 8},
    s1x8, s1x12, 0, 0U, 0x00F, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{2, 3, 4, 5, 6, 7, 8, 9},
    s2x4, s2x6, 0, 0U, 0xC03, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{0, 1, 2, 3, 8, 9, 10, 11},
    s2x4, s2x6, 0, 0U, 0x0F0, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},
    s2x6, s2x8, 0, 0U, 0xC003, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{0, 1, 2, 3, 4, 5, 10, 11, 12, 13, 14, 15},
    s2x6, s2x8, 0, 0U, 0x03C0, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{15, 14, 12, 13, 8, 9, 11, 10},
    s1x8, s2x8, 0, 0U, 0x00FF, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{0, 1, 3, 2, 7, 6, 4, 5},
    s1x8, s2x8, 0, 0U, 0xFF00, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{0, 1, 2, 3, 4, 7, 8, 9},
    s1x4p2dmd, s1x4dmd, 0, 0U, 0x60, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{5, 6, 7, 8, 9, 2, 3, 4},
    s1x4p2dmd, s1x4dmd, 2, 0U, 0x03, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{0, -1, 1, -1, 2, -1, 3, -1, -1, 8, -1, 9, -1, 11, -1, 10},
    s_c1phan, sbigdmd, 0, 0U, 0x0F0, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{-1, 2, -1, 3, -1, 5, -1, 4, 6, -1, 7, -1, 8, -1, 9, -1},
    s_c1phan, sbigdmd, 0, 0U, 0xC03, false,
    warn__none, warn__none, normalize_recenter, 0},

   {{0, 1, 3, 2, 5, 6, -1, -1},
    s1x8, s1x4p2dmd, 0, 0U, 0x90, false,
    warn__none, warn__none, normalize_before_merge, 0},

   {{0, 1, -1, 2, 5, -1, -1, -1},
    s1x8, s4p2x1dmd, 0, 0U, 0xD8, false,
    warn__none, warn__none, normalize_before_merge, 0},

   {{0, 1, 2, 5, 6, 3},
    s1x6, s4p2x1dmd, 0, 0U, 0x90, true,      // This one requires exact action.
    warn__none, warn__none, normalize_to_6, 0},

   {{0, 1, 2, 6, 4, 3},
    s1x6, splinedmd, 0, 0U, 0xA0, true,      // This one requires exact action.
    warn__none, warn__none, normalize_to_6, 0},

   // This makes it possible to do "own the <points>, trade by flip the diamond"
   // from point-to-point diamonds.
   // These three must be in this order.
   {{0, -1, 2, 4, -1, 6},
    s1x6, s_ptpd, 0, 0U, 0xAA, true,
    warn__none, warn__none, normalize_before_isolated_callMATRIXMATRIXMATRIX, 0},
   {{0, 1, 2, 3, 4, 5, 6, 7},
    s1x8, s_ptpd, 0, 0U, 0xAA, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},
   {{0, -1, 2, 4, -1, 6},
    s1x6, s_ptpd, 0, 0U, 0xAA, false,
    warn__none, warn__none, normalize_strict_matrix, 0},

   {{-1, 1, -1, -1, 7, -1, -1, 5, -1, -1, 3, -1},
    s2x6, s_ptpd, 0, 0U, 0x55, true,
    warn__none, warn__none, normalize_before_isolated_callMATRIXMATRIXMATRIX, 0},

   // Some unsymmetrical things.
   {{-1, -1, 7, 4, 5, 6, -1, -1},
    s2x4, sboxdmd, 1, 0U, 0x0F, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   {{0, -1, -1, -1, -1, 2, 3, 1},
    s_qtag, sboxdmd, 1, 0U, 0xF0, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   {{4, 5, 7, 6, -1, -1, -1, -1},
    s2x4, slinepdmd, 0, 0U, 0x0F, false,
    warn__none, warn__none, normalize_before_merge, 0},

   {{4, 5, 7, 6, -1, -1, -1, -1},
    s2x4, slinedmd, 0, 0U, 0x0F, false,
    warn__none, warn__none, normalize_before_merge, 0},

   {{0, -1, -1, -1, -1, 2, 3, 1},
    s_qtag, slinedmd, 1, 0U, 0xF0, false,
    warn__none, warn__none, normalize_before_merge, 0},

   {{0, 1, 2, 3, -1, -1, -1, -1},
    s_ptpd, slinepdmd, 1, 0U, 0xF0, false,
    warn__none, warn__none, normalize_before_merge, 0},

   {{0, 1, 2, 3, -1, -1, -1, -1},
    s_ptpd, sboxpdmd, 1, 0U, 0xF0, false,
    warn__none, warn__none, normalize_before_merge, 0},

   {{-1, -1, 7, 4, 5, 6, -1, -1},
    s2x4, sboxpdmd, 1, 0U, 0x0F, false,
    warn__none, warn__none, normalize_before_merge, 0},

   // These next 4 must be in this order.
   {{1, 2, 3, 5, 6, 7},
    s_2x1dmd, s3x1dmd, 0, 0U, 0x11, false,
    warn__none, warn__none, normalize_to_6, 0},
   {{1, 2, 5, 6},
    s1x4,     s3x1dmd, 0, 0U, 0x99, false,
    warn__none, warn__none, normalize_to_4, 0},
   {{7, 2, 3, 6},
    sdmd,     s3x1dmd, 1, 0U, 0x33, false,
    warn__none, warn__none, normalize_to_4, 0},
   {{2, 6},
    s1x2,     s3x1dmd, 0, 0U, 0xBB, false,
    warn__none, warn__none, normalize_to_2, 0},

   // These next 2 must also be in this order.
   {{1, 2, 3, 5, 6, 7},
    s_wingedstar6, s_wingedstar, 0, 0U, 0x11, false,
    warn__none, warn__none, normalize_to_6, 0},
   {{1, 2, 5, 6},
    s1x4,     s_wingedstar, 0, 0U, 0x99, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{11, 5},
    s1x2, s_hsqtag, 0, 0U, 03737, false,
    warn__none, warn__none, normalize_to_2, 0},

   {{10, 11, 4, 5},
    sdmd, s_hsqtag, 1, 0U, 01717, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{2, 3, 6, 7},
    s2x2, sline2box, 0, 0U, 0x33, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{2, 3, 5, 4},
    s1x4, sline6box, 0, 0U, 0xC3, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{8, 2},
    s1x2, s_dmdlndmd, 0, 0U, 07373, false,
    warn__none, warn__none, normalize_to_2, 1},

   {{7, 8, 1, 2},
    s1x4, s_dmdlndmd, 1, 0U, 07171, false,
    warn__none, warn__none, normalize_to_4, 1},

   {{10, 11, 1, 2, 4, 5, 7, 8},
    s_crosswave, s_dmdlndmd, 0, 0U, 01111, false,
    warn__none, warn__none, simple_normalize, 1},

   {{1, 2, 3, 4, 5, 8, 9, 10, 11, 12},
    s2x5, s2x7, 0, 0U, 0x20C1, false,
    warn__none, warn__none, simple_normalize, 0},
   {{10, 3},
    s1x2, s2x7, 1, 0U, 0x3BF7, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   {{1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16},
    s2x7, s2x9, 0, 0U, 0x20301, false,
    warn__none, warn__none, simple_normalize, 0},
   {{13, 4},
    s1x2, s2x9, 1, 0U, 0x3BF7, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    s2x6, sbigdhrgl, 0, ~0U, 01414, false,
    warn__none, warn__none, normalize_before_isolated_call, NEEDMASK(CONCPROP__NEEDK_END_2X2)},

   // These three must be in this order.
   {{0, 1, -1, -1, -1, -1, 4, 5, 6, 7, -1, -1, -1, -1, 10, 11},
    s2x8, sbigbone, 0, 0x3C3C, 01414, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_END_2X2)},
   {{0, 1, -1, -1, 4, 5, 6, 7, -1, -1, 10, 11},
    s2x6, sbigbone, 0, 01414, 01414, false,
    warn__none, warn__none, normalize_after_disconnected, NEEDMASK(CONCPROP__NEEDK_END_2X2)},
   {{0, 1, -1, -1, -1, -1, 4, 5, 6, 7, -1, -1, -1, -1, 10, 11},
    s2x8, sbigbone, 0, 0x3C3C, 01414, false,
    warn__none, warn__none, normalize_strict_matrix, NEEDMASK(CONCPROP__NEEDK_END_2X2)},

   {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    s2x6, sbighrgl, 0, ~0U, 01414, false,
    warn__none, warn__none, normalize_before_isolated_call, NEEDMASK(CONCPROP__NEEDK_END_2X2)},

   /* If only the center diamond is present, turn it into a 3dmd.
            If only the "wings" are present, turn it into a 2x6. */
   {{-1, 2, -1, -1, -1, 3, -1, 8, -1, -1, -1, 9},
    s3dmd, sbighrgl, 0, 0U, 06363, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    s2x6, sbigdmd, 0, 0U, 01414, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   /* If only the center 1x4 is present, turn it into a 3x4.
      If only the "wings" are present, turn it into a 2x6. */
   {{-1, -1, -1, -1, 2, 3, -1, -1, -1, -1, 8, 9},
    s3x4, sbigdmd, 1, 0U, 06363, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   {{0, -1, -1, 1, 3, -1, -1, 4},
    s2x4, s_bone6, 0, 0U, 044, false,
    warn__none, warn__none, normalize_after_disconnected, 0},

   // These three must be in this order.
   {{0, -1, -1, -1, -1, 1, 4, -1, -1, -1, -1, 5},
    s2x6, s_bone, 0, 0U, 0xCC, true,
    warn__none, warn__none, normalize_before_isolated_callMATRIXMATRIXMATRIX, 0},
   {{0, -1, -1, 1, 4, -1, -1, 5},
    s2x4, s_bone, 0, 0U, 0xCC, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},
   {{0, -1, -1, -1, -1, 1, 4, -1, -1, -1, -1, 5},
    s2x6, s_bone, 0, 0U, 0xCC, false,
    warn__none, warn__none, normalize_strict_matrix, 0},

   // In this order.
   {{-1, 6, 7, -1, 2, 3},
    s1x6, s_bone, 0, 0U, 0x33, true,
    warn__none, warn__none, normalize_before_isolated_callMATRIXMATRIXMATRIX, 0},
   {{-1, -1, 7, 6, -1, -1, 3, 2},
    s1x8, s_bone, 0, 0U, 0x33, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   // In this order.
   {{-1, -1, 0, 1, -1, -1, -1, -1, 4, 5, -1, -1},
    s2x6, s_rigger, 0, 0U, 0xCC, true,
    warn__none, warn__none, normalize_before_isolated_callMATRIXMATRIXMATRIX, 0},
   {{-1, 0, 1, -1, -1, 4, 5, -1},
    s2x4, s_rigger, 0, 0U, 0xCC, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   // This makes it possible to do "ends explode" from a rigger.
   // These three must be in this order.
   {{6, 7, -1, 2, 3, -1},
    s1x6, s_rigger, 0, 0U, 0x33, true,
    warn__none, warn__none, normalize_before_isolated_callMATRIXMATRIXMATRIX, 0},
   {{6, 7, -1, -1, 2, 3, -1, -1},
    s1x8, s_rigger, 0, 0U, 0x33, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},
   {{6, 7, -1, 2, 3, -1},
    s1x6, s_rigger, 0, 0U, 0x33, false,
    warn__none, warn__none, normalize_after_disconnected, 0},

   /* This makes it possible to do "own the <points>, trade by flip the diamond" from
      a single diamond. */
   /* We do NOT compress to a 1x2 -- see comment above. */
   {{0, 1, 2, 3},
    s1x4, sdmd, 0, 0U, 0xA, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   // **** Have downgraded these next two, so that points of hourglass can do
   // disconnected peel to diamond or 1/2 circulate.

   {{0, -1, -1, 1, 4, -1, -1, 5},
    s2x4, s_dhrglass, 0, 0U, 0xCC, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},
   {{5, -1, -1, 0, 1, -1, -1, 4},
    s2x4, s_hrglass, 1, 0U, 0xCC, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   {{0, 1, 2, 4, 5, 6},
    s2x3, s_spindle, 0, 0U, 0x88, false,
    warn__none, warn__none, normalize_to_6, 0},

   {{0, 1, 2, 4, 5, 6},
    s_ntrgl6cw, s_nxtrglcw, 0, 0U, 0x88, false,
    warn__none, warn__none, normalize_to_6, 0},

   {{0, 1, 2, 4, 5, 6},
    s_ntrgl6ccw, s_nxtrglccw, 0, 0U, 0x88, false,
    warn__none, warn__none, normalize_to_6, 0},

   /* This makes it possible to do "own the <points>, trade by flip the diamond" from
      normal diamonds. */
   /* We do NOT compress to a 2x2 -- doing so might permit people to
      work with each other across the set when they shouldn't, as in
      "heads pass the ocean; heads recycle while the sides star thru". */
   // s_qtg_2x4
   {{5, -1, -1, 0, 1, -1, -1, 4},
    s2x4, s_qtag, 1, 0U, 0xCC, false,
    warn__none, warn__none, normalize_before_isolated_call, 0},

   {{5, 1},
    s1x2, s_spindle, 1, 0U, 0xDD, false,
    warn__none, warn__none, normalize_to_2, 0},

   // s_1x2_dmd
   {{3, 1},
    s1x2, sdmd, 1, 0U, 0x5, false,
    warn__none, warn__none, normalize_to_2, 0},

   {{1, 3},
    s1x2, s1x4, 0, 0U, 0x5, false,
    warn__none, warn__none, normalize_to_2, 0},

   {{4, 1},
    s1x2, s2x3, 1, 0U, 055, false,
    warn__none, warn__none, normalize_to_2, 0},

   // s_2x3_qtg
   {{5, 7, 0, 1, 3, 4},
    s2x3, s_qtag, 1, 0U, 0x44, false,
    warn__none, warn__none, normalize_after_triple_squash, 0},

   {{10, 11, 4, 5},
     s1x4, s3x4, 0, 0U, 01717, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{5, 1, 2, 4},
    sdmd, s_2x1dmd, 1, 0U, 011, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{1, 2, 4, 5},
    sdmd, s_1x2dmd, 0, 0U, 011, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{0, 1, 3, 4},
    s1x4, s_2x1dmd, 0, 0U, 044, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{1, 3, 5, 7},
    s2x2, s_galaxy, 0, 0U, 0x55, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{6, 7, 2, 3},
    s1x4, s_crosswave, 1, 0U, 0x33, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{7, 3},
    s1x2, s_crosswave, 1, 0U, 0x77, false,
    warn__none, warn__none, normalize_to_2, 0},

   {{0, 1, 4, 5},
    s2x2, s_rigger, 0, 0U, 0xCC, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{0, 1, 2, 5, 6, 7},
    s2x3, s_spindle12, 0, 0U, 0x318, false,
    warn__none, warn__none, simple_normalize, 0},

   {{6, 7, 2, 3},
    s1x4, s_bone, 0, 0U, 0x33, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{1, 2, 4, 5},
    s1x4, s1x6, 0, 0U, 011, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{3, 2, 7, 6},
    s1x4, s1x8, 0, 0U, 0x33, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{2, 6},
    s1x2, s1x8, 0, 0U, 0xBB, false,
    warn__none, warn__none, normalize_to_2, 0},

   // This is s_2x2_2x4.
   {{1, 2, 5, 6},
    s2x2, s2x4, 0, 0U, 0x99, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{1, 3, 2, 5, 7, 6},
    s1x6, s1x8, 0, 0U, 0x11, false,
    warn__none, warn__none, normalize_to_6, 0},

   {{1, 2, 3, 5, 6, 7},
    s_1x2dmd, s1x3dmd, 0, 0U, 0x11, false,
    warn__none, warn__none, normalize_to_6, 0},

   {{0, 1, 2, 4, 5, 6},
    s1x6, s3x1dmd, 0, 0U, 0x88, false,
    warn__none, warn__none, normalize_to_6, 0},

   {{0, 1, 2, 4, 5, 6},
    s1x6, s_wingedstar, 0, 0U, 0x88, false,
    warn__none, warn__none, normalize_to_6, 0},

   {{6, 7, 2, 3},
    s1x4, s_qtag, 0, 0U, 0x33, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{7, 3},
    s1x2, s_qtag, 0, 0U, 0x77, false,
    warn__none, warn__none, normalize_to_2, 0},

   {{3, 1},
    s1x2, s_star, 1, 0U, 0x5, false,
    warn__none, warn__none, normalize_to_2, 0},
   {{0, 2},
    s1x2, s_star, 0, 0U, 0xA, false,
    warn__none, warn__none, normalize_to_2, 0},

   {{9, 11, 13, 6, 2, 0, 1, 3, 5, 14, 10, 8},
    s2x6, sdeepbigqtg, 1, 0U, 0x9090, false,
    warn__none, warn__phantoms_thinner, simple_normalize, 0},

   {{2, 3, 7, 6, 5, 4, 10, 11, 15, 14, 13, 12},
    sbigh, sdblxwave, 0, 0U, 0x0303, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_DBLX)},

   {{13, 14, 1, 2, 5, 6, 9, 10},
    s_alamo, s4x4, 0, 0U, 0x9999, false,
    warn__none, warn__none, normalize_never, NEEDMASK(CONCPROP__NEEDK_4X4)},

   {{10, -1,-1, 1, 2, -1, -1, 9},
    s2x4, s4dmd, 1, 0x66, ~0U, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4D_4PTPD) |
                                              NEEDMASK(CONCPROP__NEEDK_4DMD)},

   {{0, 1, -1, -1, 2, 3, 5, 6, 7, 8, 9, -1, -1, 10, 11, 13, 14, 15},
    s3x6, s4dmd, 0, 0014014, ~0U, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4D_4PTPD) |
                                              NEEDMASK(CONCPROP__NEEDK_4DMD)},

   {{-1, 0, 1, -1, -1, 2, 3, -1, 4, 5, 6, 7, -1, 8, 9, -1, -1, 10, 11, -1, 12, 13, 14, 15},
    s3x8, s4dmd, 0, 0x099099, ~0U, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4D_4PTPD) |
                                              NEEDMASK(CONCPROP__NEEDK_4DMD)},

   {{1, -1, -1, 2, 6, 7, 9, -1, -1, 10, 14, 15},
    s3x4, s4dmd, 0, 00606, ~0U, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4D_4PTPD) |
                                              NEEDMASK(CONCPROP__NEEDK_4DMD)},

   {{0, 1, 2, 3, -1, -1, 8, 9, 10, 11, -1, -1},
    s3x4, s4dmd, 0, 06060, ~0U, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4D_4PTPD) |
                                              NEEDMASK(CONCPROP__NEEDK_4DMD)},

   {{-1, 2, 3, -1, -1, 6, 7, 8, -1, 11, 12, -1, -1, 15, 16, 17},
    s4dmd, s3x6, 0, 0x1919, ~0U, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_3X6)},

   {{13, 14, 1, -1, -1, 10, 5, 6, 9, -1, -1, 2},
    sdeepqtg, s4x4, 0, 03030, ~0U, false,
    warn__check_4x4_start, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4)},

   {{-1, -1, 13, 14, 1, -1, -1, 10, -1, -1, 5, 6, 9, -1, -1, 2},
    sdeepbigqtg, s4x4, 0, 0x6363, ~0U, false,
    warn__check_4x4_start, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4)},

   {{-1, -1, 13, 10, -1, -1, 1, 14, -1, -1, 5, 2, -1, -1, 9, 6},
    s_c1phan, s4x4, 0, 0x3333, ~0U, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4)},

   {{-1, 13, -1, 15, -1, 1, -1, 3, 2, -1, 7, -1, 6, -1, 11, -1},
    s_c1phan, s4x4, 0, 0xAA55, ~0U, false,
    warn__check_4x4_ctrbox, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4)},

   {{10, -1, 15, -1, -1, 1, -1, 3, -1, 5, -1, 7, 6, -1, 11, -1},
    s_c1phan, s4x4, 0, 0xA55A, ~0U, false,
    warn__check_4x4_ctrbox, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4)},

   {{10, -1, 15, -1, 14, -1, 3, -1, -1, 5, -1, 7, -1, 9, -1, 11},
    s_c1phan, s4x4, 0, 0x55AA, ~0U, false,
    warn__check_4x4_ctrbox, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4)},

   {{-1, 13, -1, 15, 14, -1, 3, -1, 2, -1, 7, -1, -1, 9, -1, 11},
    s_c1phan, s4x4, 0, 0x5AA5, ~0U, false,
    warn__check_4x4_ctrbox, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4)},

   {{10, 13, -1, 15, 14, 1, 3, -1, 2, 5, -1, 7, 6, 9, 11, -1},
    s_c1phan, s4x4, 0, 0x8484, ~0U, false,
    warn__check_4x4_start, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4) |
                                                         NEEDMASK(CONCPROP__NEEDK_QUAD_1X4) |
                                                         NEEDMASK(CONCPROP__NEEDK_BLOB) |
                                                         NEEDMASK(CONCPROP__NEEDK_4X6) |
                                                         NEEDMASK(CONCPROP__NEEDK_TWINDMD) |
                                                         NEEDMASK(CONCPROP__NEEDK_TWINQTAG)},
   {{10, 13, 15, -1, 14, 1, -1, 3, 2, 5, 7, -1, 6, 9, -1, 11},
    s_c1phan, s4x4, 0, 0x4848, ~0U, false,
    warn__check_4x4_start, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4) |
                                                         NEEDMASK(CONCPROP__NEEDK_QUAD_1X4) |
                                                         NEEDMASK(CONCPROP__NEEDK_BLOB) |
                                                         NEEDMASK(CONCPROP__NEEDK_4X6) |
                                                         NEEDMASK(CONCPROP__NEEDK_TWINDMD) |
                                                         NEEDMASK(CONCPROP__NEEDK_TWINQTAG)},
   {{10, 13, 15, -1, 14, 1, 3, -1, 2, 5, 7, -1, 6, 9, 11, -1},
    s_c1phan, s4x4, 0, 0x8888, ~0U, false,
    warn__check_4x4_start, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4) |
                                                         NEEDMASK(CONCPROP__NEEDK_QUAD_1X4) |
                                                         NEEDMASK(CONCPROP__NEEDK_BLOB) |
                                                         NEEDMASK(CONCPROP__NEEDK_4X6) |
                                                         NEEDMASK(CONCPROP__NEEDK_TWINDMD) |
                                                         NEEDMASK(CONCPROP__NEEDK_TWINQTAG)},

   {{10, 13, -1, 15, 14, 1, -1, 3, 2, 5, -1, 7, 6, 9, -1, 11},
    s_c1phan, s4x4, 0, 0x4444, ~0U, false,
    warn__check_4x4_start, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4) |
                                                         NEEDMASK(CONCPROP__NEEDK_QUAD_1X4) |
                                                         NEEDMASK(CONCPROP__NEEDK_BLOB) |
                                                         NEEDMASK(CONCPROP__NEEDK_4X6) |
                                                         NEEDMASK(CONCPROP__NEEDK_TWINDMD) |
                                                         NEEDMASK(CONCPROP__NEEDK_TWINQTAG)},


   {{12, 13, 14, 0, -1, -1, -1, -1, 4, 5, 6, 8, -1, -1, -1, -1},
    s4dmd, s4x4, 0, 0xF0F0, ~0U, false,
    warn__check_4x4_start, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4) |
                                                         NEEDMASK(CONCPROP__NEEDK_QUAD_1X4) |
                                                         NEEDMASK(CONCPROP__NEEDK_BLOB) |
                                                         NEEDMASK(CONCPROP__NEEDK_4X6) |
                                                         NEEDMASK(CONCPROP__NEEDK_TWINDMD) |
                                                         NEEDMASK(CONCPROP__NEEDK_TWINQTAG)},

   {{0, 1, 2, 3, -1, -1, 4, 5, 6, 7, 8, 9, -1, -1, 10, 11},
    s4dmd, s3x4, 0, 0x3030, ~0U, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_3X4) |
                                              NEEDMASK(CONCPROP__NEEDK_3X4_D3X4) |
                                              NEEDMASK(CONCPROP__NEEDK_TRIPLE_1X4)},

   {{0, 1, 4, 5, -1, 6, 7, 8, 9, 10, 13, 14, -1, 15, 16, 17},
    s4dmd, s3x6, 0, 0x1010, ~0U, false,
    warn__check_centered_qtag, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_3X6)},
   {{1, 2, 5, 6, 8, 9, 10, 11, 13, 14, 17, 18, 20, 21, 22, 23},
    s4dmd, s3x8, 0, 0U, ~0U, false,
    warn__check_centered_qtag, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_3X8)},

   {{1, 2, 3, 5, 7, 8, 9, 11},
    s_spindle, sd3x4, 0, 0U, 02121, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_3X4_D3X4)},
   {{1, 2, 3, 7, 8, 9},
    s2x3, sd3x4, 0, 0U, 06161, false,
    warn__none, warn__none, simple_normalize, 0},

   {{2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15},
    sdeepqtg, sdeepbigqtg, 0, 0, 0x0303, false,
    warn__none, warn__none, simple_normalize, 0},

   {{0, 1, -1, -1, 5, 6, 7, 8, -1, -1, 12, 13},
    s2x6, sd2x7, 0, 01414U, 0x0E1CU, false,
    warn__none, warn__none, simple_normalize, 0},

   {{4, 3, 2, 11, 10, 9},
    s2x3, sd2x7, 1, 0U, 0x31E3U, false,
    warn__none, warn__none, simple_normalize, 0},

   {{8, 11, 1, 2, 5, 7},
    s2x3, s3x4, 1, 0U, 03131, false,
    warn__none, warn__none, simple_normalize, 0},
   {{12, 14, 3, 1, 4, 6, 11, 9},
    s2x4, s_c1phan, 1, 0U, 0xA5A5, false,
    warn__none, warn__none, simple_normalize, 0},
   {{0, 2, 7, 5, 8, 10, 15, 13},
    s2x4, s_c1phan, 0, 0U, 0x5A5A, false,
    warn__none, warn__none, simple_normalize, 0},
   {{6, 11, 15, 13, 14, 3, 7, 5},
    s2x4, s4x4, 1, 0U, 0x1717, false,
    warn__none, warn__none, simple_normalize, 0},

   {{10, 15, 3, 1, 2, 7, 11, 9},
    s2x4, s4x4, 0, 0U, 0x7171, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4) |
                                              NEEDMASK(CONCPROP__NEEDK_QUAD_1X4) |
                                              NEEDMASK(CONCPROP__NEEDK_BLOB) |
                                              NEEDMASK(CONCPROP__NEEDK_4X6) |
                                              NEEDMASK(CONCPROP__NEEDK_TWINDMD) |
                                              NEEDMASK(CONCPROP__NEEDK_TWINQTAG)},

   // s_qtg_3x4
   /* ***** This is a kludge to make threesome work!!!! */
   {{1, 2, 4, 5, 7, 8, 10, 11},
    s_qtag, s3x4, 0, 0U, ~0U, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4X4) |
                                              NEEDMASK(CONCPROP__NEEDK_QUAD_1X4) |
                                              NEEDMASK(CONCPROP__NEEDK_BLOB) |
                                              NEEDMASK(CONCPROP__NEEDK_4X6) |
                                              NEEDMASK(CONCPROP__NEEDK_TWINDMD) |
                                              NEEDMASK(CONCPROP__NEEDK_TWINQTAG) |
                                              NEEDMASK(CONCPROP__NEEDK_3X4) |
                                              NEEDMASK(CONCPROP__NEEDK_3X4_D3X4) |
                                              NEEDMASK(CONCPROP__NEEDK_3X8) |
                                              NEEDMASK(CONCPROP__NEEDK_TRIPLE_1X4)},
   {{0, 1, 2, 4, 5, 6, 7, 9},
    s_323, s_343, 0, 0U, 0x108, false,
    warn__none, warn__none, simple_normalize, 0},
   {{1, 2, 3, 5, 7, 8, 9, 11},
    s_323, s_525, 0, 0U, 02121, false,
    warn__none, warn__none, simple_normalize, 0},
   {{1, 2, 3, 5, 6, 8, 9, 10, 12, 13},
    s_343, s_545, 0, 0U, 0x891, false,
    warn__none, warn__none, simple_normalize, 0},
   {{0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 13},
    s_525, s_545, 0, 0U, 0x1020, false,
    warn__none, warn__none, simple_normalize, 0},
   {{0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 13},
    s_525, sh545, 0, 0U, 0x1020, false,
    warn__none, warn__none, simple_normalize, 0},
   {{0, -1, 1, -1, 2, 4, 5, 6, 19, 18, 10, -1, 11, -1, 12, 14, 15, 16, 9, 8},
    s4x5, s3oqtg, 0, 0U, 0x22088, false,
    warn__none, warn__none, simple_normalize, 0},
   {{1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18},
    s2x8, s2x10, 0, 0U, 0x80601, false,
    warn__none, warn__none, simple_normalize, 0},
   {{-1, 15, 16, 20, 21, -1, 0, 22, 23, 17, 14, 13, -1, 3, 4, 8, 9, -1, 12, 10, 11, 5, 2, 1},
    s4x6, s_bigblob, 0, 0U, 0x0C00C0, false,
    warn__none, warn__none, simple_normalize, 0},
   {{-1, 21, 22, 2, 3, -1, 6, 4, 5, 23, 20, 19, -1, 9, 10, 14, 15, -1, 18, 16, 17, 11, 8, 7},
    s4x6, s_bigblob, 0, 0U, 0x003003, false,
    warn__none, warn__none, simple_normalize, 0},
   {{7, 6, 5, 4, 15, 14, 13, 12},
    s2x4, sdeepbigqtg, 0, 0U, 0xF0F, false,
    warn__none, warn__none, simple_normalize, 0},
   {{5, 4, 3, 2, 11, 10, 9, 8},
    s2x4, sdeepxwv, 1, 0U, 0303, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_DEEPXWV)},
   {{1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15},
    s1x14, s1x16, 0, 0U, 0x101, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_QUAD_1X4) |
                                              NEEDMASK(CONCPROP__NEEDK_1X16)},
   {{1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13},
    s1x12, s1x14, 0, 0U, 0x81, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_QUAD_1X4) |
                                              NEEDMASK(CONCPROP__NEEDK_1X16)},
   {{1, 2, 3, 4, 5, 7, 8, 9, 10, 11},
    s1x10, s1x12, 0, 0U, 0101, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_CTR_1X4) |
                                              NEEDMASK(CONCPROP__NEEDK_TRIPLE_1X4) |
                                              NEEDMASK(CONCPROP__NEEDK_END_1X4) |
                                              NEEDMASK(CONCPROP__NEEDK_QUAD_1X4) |
                                              NEEDMASK(CONCPROP__NEEDK_QUAD_1X3) |
                                              NEEDMASK(CONCPROP__NEEDK_1X12) |
                                              NEEDMASK(CONCPROP__NEEDK_1X16)},
   {{0, 1, 2, 3, 5, 14, 8, 9, 10, 11, 13, 6},
    sbigrig, sbigbigx, 0, 0U, 0x9090, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_QUAD_1X4)},
   {{3, 10, 6, 7, 9, 4, 0, 1},
    s_rigger, sdeepxwv, 0, 0U, 04444, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_QUAD_1X4)},

   {{2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15},
    sdeepxwv, sbigbigx, 0, 0U, 0x303, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_QUAD_1X4)},

   // order of these next 2 items must be as shown: must be 3x4, 1x6!!!!
   {{15, 16, 17, 6, 7, 8},
    s1x6, s3x6, 0, 0U, 0077077, false,
    warn__none, warn__none, simple_normalize, 0},
   {{1, 2, 3, 4, 7, 8, 10, 11, 12, 13, 16, 17},
    s3x4, s3x6, 0, 0U, 0141141, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_3X6)},

   {{12, 13, 15, 14, 4, 5, 7, 6},
    s1x8, s4dmd, 0, 0U, 0x0F0F, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4D_4PTPD) |
                                              NEEDMASK(CONCPROP__NEEDK_4DMD)},
   {{1, 2, 5, 6, 7, 9, 10, 13, 14, 15},
    swqtag, s4dmd, 0, 0U, 0x1919, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4D_4PTPD) |
                                              NEEDMASK(CONCPROP__NEEDK_4DMD)},
   {{1, 2, 6, 7, 9, 10, 14, 15},
    s_qtag, s4dmd, 0, 0U, 0x3939, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_4D_4PTPD) |
                                              NEEDMASK(CONCPROP__NEEDK_4DMD)},
   {{0, 1, 3, 4, 5, 6, 8, 9},
    s_qtag, swqtag, 0, 0U, 0x084, false,
    warn__none, warn__none, simple_normalize, 0},

   {{5, 4, 3, 6, 7, 8, 14, 13, 12, 15, 16, 17},
    s3dmd, sbig3dmd, 0, 0U, 0007007, false,
    warn__none, warn__none, simple_normalize, 0},

   {{7, 6, 5, 4, 8, 9, 10, 11, 19, 18, 17, 16, 20, 21, 22, 23},
    s4dmd, sbig4dmd, 0, 0U, 0x00F00F, false,
    warn__none, warn__none, simple_normalize, 0},

   {{7, 8, 9, 2, 3, 4},
    s1x6, swqtag, 0, 0U, 0x63, false,
    warn__none, warn__none, simple_normalize, 0},

   // Some things to compress an hqtag.
   {{0, 1, 2, 3, 14, 15, 8, 9, 10, 11, 6, 7},
    sbigh, s_hqtag, 0, 0U, 0x3030, false,
    warn__none, warn__none, simple_normalize, 0},
   {{0, 4, 5, 11, -1, -1, 6, 7, 8, 12, 13, 3, -1, -1, 14, 15},
    s4dmd, s_hqtag, 0, 0U, 0x0606, false,
    warn__none, warn__none, simple_normalize, 0},

   {{0, 1, 3, 4, 5, 6, 8, 9},
    s2x4, sdeep2x1dmd, 0, 0U, 0x084, false,
    warn__none, warn__none, simple_normalize, 0},
   {{5, 4, 1, 3, 2, 11, 10, 7, 9, 8},
    sdeep2x1dmd, sdeepxwv, 1, 0U, 00101, false,
    warn__none, warn__none, simple_normalize, 0},

   {{1, 2, 3, 6, 7, 8},
    s2x3, s2x5, 0, 0U, 0x231, false,
    warn__none, warn__none, normalize_to_6, 0},
   {{4, 3, 2, 9, 8, 7},
    s2x3, sd2x5, 1, 0U, 0x063, false,
    warn__none, warn__none, normalize_to_6, 0},

   {{11, 10, 9, 8, 7, 6, 23, 22, 21, 20, 19, 18},
    s2x6, s4x6, 0, 0U, 0x03F03F, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_4X6) |
                                             NEEDMASK(CONCPROP__NEEDK_TWINDMD) |
                                             NEEDMASK(CONCPROP__NEEDK_TWINQTAG)},

   {{11, -1, -1, 9, 8, -1, -1, 6, 23, -1, -1, 21, 20, -1, -1, 18},
    s2x8, s4x6, 0, 0x6666, ~0U, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_4X6) |
                                             NEEDMASK(CONCPROP__NEEDK_TWINDMD) |
                                             NEEDMASK(CONCPROP__NEEDK_TWINQTAG)},

   // order of these next 3 items must be as shown: must be 1x8, 3x6, 3x4!!!!
   {{20, 21, 23, 22, 8, 9, 11, 10},
    s1x8, s3x8, 0, 0U, 0x0FF0FF, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_3X8)},
   {{1, 2, 3, 4, 5, 6, 9, 10, 11, 13, 14, 15, 16, 17, 18, 21, 22, 23},
    s3x6, s3x8, 0, 0U, 0x181181, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_3X8)},
   {{2, 3, 4, 5, 10, 11, 14, 15, 16, 17, 22, 23},
    s3x4, s3x8, 0, 0U, 0x3C33C3, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_3X8)},
   {{1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18},
    s2x8, s2x10, 0, 0U, ~0U, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_2X10)},
   {{2, 3, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17},
    s2x6, s2x10, 0, 0U, ~0U, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_2X10)},
   {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22},
    s2x10, s2x12, 0, 0U, 0x801801, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_2X12)},
   {{2, 3, 4, 5, 6, 7, 8, 9, 14, 15, 16, 17, 18, 19, 20, 21},
    s2x8, s2x12, 0, 0U, ~0U, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_2X12)},
   {{3, 4, 5, 6, 7, 8, 15, 16, 17, 18, 19, 20},
    s2x6, s2x12, 0, 0U, ~0U, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_2X12)},
   {{4, 5, 6, 7, 16, 17, 18, 19},
    s2x4, s2x12, 0, 0U, ~0U, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_2X12)},
   {{14, 1, 15, 10, 6, 9, 7, 2},
    s_ptpd, s4ptpd,0, 0U, 0x3939, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_4D_4PTPD)},
   {{1, 2, 4, 3, 6, 7, 9, 8},
    s1x8, s1x10, 0, 0U, 0x21, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_1X10)},

   {{1, 8, 10, 11, 7, 2, 4, 5},
    s_bone, sbigh, 0, 0U, 01111, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_TRIPLE_1X4) |
                                             NEEDMASK(CONCPROP__NEEDK_END_1X4)},
   {{1, 2, 3, 5, 7, 8, 9, 11},
    s1x3dmd, sbigx, 0, 0U, 02121, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_CTR_1X4) |
                                             NEEDMASK(CONCPROP__NEEDK_TRIPLE_1X4) |
                                             NEEDMASK(CONCPROP__NEEDK_END_1X4)},
   {{2, 3, 4, 5, 8, 9, 10, 11},
    s_crosswave, sbigx, 0, 0U, 0303, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_CTR_1X4) |
                                             NEEDMASK(CONCPROP__NEEDK_TRIPLE_1X4) |
                                             NEEDMASK(CONCPROP__NEEDK_END_1X4)},

   {{1, 2, 6, 7, 9, 10, 14, 15},
    s_crosswave, sx1x8, 0, 0U, 0x3939, false,
    warn__none, warn__none, plain_normalize, 0},
   {{13, 14, 2, 3, 5, 6, 10, 11},
    s_crosswave, sx1x8, 1, 0U, 0x9393, false,
    warn__none, warn__none, plain_normalize, 0},

   {{2, 3, 5, 8, 9, 11},
    s_1x2dmd, sbigx, 0, 0U, 02323, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_CTR_1X4) |
                                             NEEDMASK(CONCPROP__NEEDK_TRIPLE_1X4) |
                                             NEEDMASK(CONCPROP__NEEDK_END_1X4)},
   {{2, 3, 5, 4, 8, 9, 11, 10},
    s1x8,    s1x12, 0, 0U, ~0U, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_CTR_1X4) |
                                             NEEDMASK(CONCPROP__NEEDK_TRIPLE_1X4) |
                                             NEEDMASK(CONCPROP__NEEDK_END_1X4) |
                                             NEEDMASK(CONCPROP__NEEDK_1X12) |
                                             NEEDMASK(CONCPROP__NEEDK_QUAD_1X3) |
                                             NEEDMASK(CONCPROP__NEEDK_QUAD_1X4) |
                                             NEEDMASK(CONCPROP__NEEDK_1X16)},
   {{1, 4, 8, 9, 7, 10, 2, 3},
    s_bone, sbigbone, 0, 0U, 04141, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_END_2X2) |
                                             NEEDMASK(CONCPROP__NEEDK_CTR_1X4)},
   {{4, 5, 8, 9, 10, 11, 2, 3},
    s_rigger, sbigrig, 0, 0U, 0303, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_CTR_2X2) |
                                             NEEDMASK(CONCPROP__NEEDK_END_1X4)},

   {{0, 1, 2, 4, 5, 6, 7, 8, 10, 11},
    s_spindle12, srigger12, 0, 0U, 01010, false,
    warn__none, warn__none, plain_normalize, 0},

   {{6, 7, 3, 2, 1, 12, 14, 15, 11, 10, 9, 4},
    sbigrig, sdblbone, 0, 0U, 0x2121, false,
    warn__none, warn__none, plain_normalize, 0},
   {{0, 1, 3, 2, 12, 13, 8, 9, 11, 10, 4, 5},
    sbigbone, sdblrig, 0, 0U, 0xC0C0, false,
    warn__none, warn__none, plain_normalize, 0},
   {{2, 3, 4, 5, 8, 9, 10, 11},
    s3x1dmd, s5x1dmd, 0, 0U, 0303, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_CTR_DMD) |
                                             NEEDMASK(CONCPROP__NEEDK_END_1X4)},
   {{2, 3, 4, 5, 8, 9, 10, 11},
    s1x3dmd, s1x5dmd,    0, 0U, 0303, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_CTR_DMD) |
                                             NEEDMASK(CONCPROP__NEEDK_END_1X4)},
   {{1, 2, 3, 4, 6, 7, 8, 9},
    s1x3dmd, s1x4dmd,    0, 0U, 0x021, false,
    warn__none, warn__none, plain_normalize, 0},
   {{1, 4, 3, 2, 7, 10, 9, 8},
    s_dhrglass,sbigdhrgl,0, 0U, 04141, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_END_2X2)},
   {{10, 1, 2, 9, 4, 7, 8, 3},
    s_hrglass, sbighrgl, 1, 0U, 04141, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_END_2X2)},
   {{8, 9, 2, 3},
    sdmd, sbighrgl, 1, 0U, 06363, false,
    warn__none, warn__none, normalize_to_4, 0},

   {{10, 1, 2, 3, 4, 7, 8, 9},
    s_qtag, sbigdmd, 1, 0U, 04141, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_END_2X2) |
                                              NEEDMASK(CONCPROP__NEEDK_CTR_1X4)},

   // This one is troublesome.
   {{1, 2, 3, 4, 7, 8, 9, 10},
    s2x4, s2x6, 0, 0U, 04141, false,
    warn__none, warn__none, simple_normalize, NEEDMASK(CONCPROP__NEEDK_2X6) |
                                              NEEDMASK(CONCPROP__NEEDK_CTR_2X2) |
                                              NEEDMASK(CONCPROP__NEEDK_END_2X2)},

   {{2, 3, 8, 9},
    s2x2, s2x6, 0, 0U, 06363, false,
    warn__none, warn__none, normalize_before_isolate_not_too_strict, 0},

   {{1, 9, 11, 8, 7, 3, 5, 2},
    s_rigger, sdblbone6, 0, 0U, 02121, false,
    warn__none, warn__none, plain_normalize, 0},
   {{11, 5},
    s1x2, s3dmd, 0, 0U, ~0U, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_3DMD)},
   {{0, 1, 2, 4, 5, 6, 7, 8, 10, 11},
    s_343, s3dmd, 0, 0U, ~0U, false,
    warn__check_3dmd_is_wide, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_3DMD)},
   {{0, 1, 2, 5, 6, 7, 8, 11},
    s_323, s3dmd, 0, 0U, ~0U, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_3DMD)},
   {{9, 10, 11, 1, 3, 4, 5, 7},
    s1x3dmd, s_3mdmd, 0, 0U, 0505, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_3DMD)},
   {{0, 2, 4, 5, 6, 8, 10, 11},
    s_bone, s_3mptpd, 0, 0U, 01212, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_3DMD)},
   {{9, 10, 11, 1, 3, 4, 5, 7},
    s3x1dmd, s3dmd,  0, 0U, 0505, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_END_DMD) |
                                             NEEDMASK(CONCPROP__NEEDK_CTR_DMD) |
                                             NEEDMASK(CONCPROP__NEEDK_3DMD)},

   {{1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14},
    s2x6,    s2x8,   0, 0U, 0x8181, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_2X8)},
   {{2, 3, 4, 5, 10, 11, 12, 13},
    s2x4,    s2x8,   0, 0U, ~0U, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_2X8)},
   {{3, 4, 8, 5, 9, 10, 14, 11, 15, 16, 20, 17, 21, 22, 2, 23},
    s4x4, s_bigblob, 0, 0U, ~0U, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_BLOB)},
   {{9, 8, 2, 7, 6, 5, 19, 18, 12, 17, 16, 15},
    sbigdmd, s4x5, 0, 0U, ~0U, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_4X5)},
   {{9, 8, 7, 6, 5, 19, 18, 17, 16, 15},
    s2x5, s4x5, 0, 0U, 0x07C1F, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_4X5)},
   {{13, 16, 8, 1, 2, 7, 3, 6, 18, 11, 12, 17},
    s3x4, s4x5, 1, 0U, 0x8C631, false,
    warn__none, warn__none, plain_normalize, NEEDMASK(CONCPROP__NEEDK_4X5)},

   {{0}, nothing, nothing}};



// Maps are identified by a "code" number, which is a uint32.
// Whenever a procedure needs to specify a "map_thing", it produces
// the code number for it.  The maps themselves are in tables
// "map_init_table" and "spec_map_table" in this file.
// The procedure "get_map_from_code" takes the code number and finds
// the indicated map in the tables.  The principal users of maps are
// "divided_setup_move" and "overlapped_setup_move".  They are simply
// given the map code, and they call "get_map_from_code" to get the
// map itself.  There are few other places that call "get_map_from_code"
// directly, but not many.
//
// The code number is one of two different types.  If the sign bit is on,
// the map is one of the "standard" maps, characterized by the inner setup kind,
// arity, map kind, and vertical bit.  A huge number of maps are characterized
// in this way.  They are in "map_init_table".  The macro
// MAPCODE(setupkind,num,mapkind,vert) packs the 4 values into the code number.
// The four arguments can be manifest constants (and the macro will be very
// efficient in that case), but they do not need to be.
//
// In the tables that contain the maps themselves, the inner_kind, arity,
// map_kind, and vert field are, by convention, all written together, with
// no spaces separating them.  All invocations of MAPCODE are written the
// same way.  The intention is that, if one sees a line that looks like
//         map_code = MAPCODE(s_qtag,1,MPKIND__OFFS_R_HALF,1);
// one can search this file for the string "s_qtag,1,MPKIND__OFFS_R_HALF,1"
// to find the actual map corresponding to this code.
//
// During program initialization (specifically, during "map::initialize"),
// "map_init_table" is scanned, the map codes numbers are computed from the
// 4 fields and placed in the "code" field, and hash tables are set up for
// "get_map_from_code" to use.
//
// There are a number of maps that don't fit into this scheme.  They are
// specified by the enumeration type "specmapkind".  The map code is simply
// the numeric value of some item in that enumeration.  Its sign bit is
// of course zero.  The array "spec_map_table" contains these, simply listed
// in the enumeration order.  When "get_map_from_code" is given a nonnegative
// argument, it simply indexes that table.
//
// To make sure that the map initializers in "spec_map_table" stay in step
// with the enumeration, the maps must have the "code" field explicitly filled
// in, and initialization code checks this.  So, if one sees a line of code like
//         map_code = spcmap_stw3b;
// one can search this file for the string "spcmap_stw3b"
// to find the actual map corresponding to this code.

// This table has the maps encoded by the "MAPCODE" macro.

map::map_thing map::map_init_table[] = {
   {{13, 15, 8, 9,     0, 1, 5, 7,
     12, 10, 6, 11,    14, 3, 4, 2},
    s1x4,4,MPKIND__OFFS_BOTH_SINGLEH,1, warn__none, s4x4, 0x055, 0},
   {{8, 6, 2, 7,    10, 15, 0, 14,
     9, 11, 4, 5,     12, 13, 1, 3},
    s1x4,4,MPKIND__OFFS_BOTH_SINGLEV,1, warn__none, s4x4, 0x000, 0},
   {{15, 14, 3, 2,     11, 10, 7, 6,
     0, 1, 12, 13,    4, 5, 8, 9},
    s1x4,4,MPKIND__OFFS_BOTH_SINGLEV,0, warn__none, s2x8, 0x000, 0},
   {{7, 6, 3, 2,        0, 1, 4, 5},
    s1x4,2,MPKIND__OFFS_BOTH_SINGLEV,1, warn__none, s2x4, 0x000, 0},
   {{0, 1, 4, 5,        2, 3, 6, 7},
    s2x2,2,MPKIND__OFFS_BOTH_SINGLEV,0, warn__none, s2x4, 0x000, 0},
   {{1, 4, 5, 0,        3, 6, 7, 2},
    s2x2,2,MPKIND__OFFS_BOTH_SINGLEV,1, warn__none, s2x4, 0x005, 0},
   {{9, 13, 15, 8,      7, 0, 1, 5,        12, 11, 6, 10,       14, 2, 4, 3},
    s2x2,4,MPKIND__OFFS_BOTH_SINGLEV,0, warn__none, s4x4, 0x000, 0},
   {{2, 3, 14, 15,      6, 7, 10, 11,      0, 1, 12, 13,        4, 5, 8, 9},
    s2x2,4,MPKIND__OFFS_BOTH_SINGLEH,0, warn__none, s2x8, 0x000, 0},

   {{1, 17, 12, 0, 3, 9, 10, 8},
    s2x2,2,MPKIND__BENT0CW,0,     warn__none,  s3x6,      0x000, 0x1441},
   {{14, 2, 17, 13, 8, 4, 5, 11},
    s2x2,2,MPKIND__BENT0CCW,0,    warn__none,  s3x6,      0x000, 0x3CC3},
   {{0, 11, 20, 19, 8, 7, 12, 23},
    s1x4,2,MPKIND__BENT0CW,0,     warn__none,  s4x6,      0x000, 0x5005},
   {{17, 18, 9, 10, 21, 22, 5, 6},
    s1x4,2,MPKIND__BENT0CCW,0,    warn__none,  s4x6,      0x000, 0xF00F},

   {{0, 12, 13, 15, 4, 6, 9, 3},
    s2x2,2,MPKIND__BENT1CW,0,     warn__none,  s3x6,      0x000, 0x4114},
   {{15, 1, 2, 14, 11, 5, 6, 10},
    s2x2,2,MPKIND__BENT1CCW,0,    warn__none,  s3x6,      0x000, 0xC33C},
   {{11, 10, 15, 20, 3, 8, 23, 22},
    s1x4,2,MPKIND__BENT1CW,0,     warn__none,  s4x6,      0x000, 0x0550},
   {{18, 19, 2, 9, 14, 21, 6, 7},
    s1x4,2,MPKIND__BENT1CCW,0,    warn__none,  s4x6,      0x000, 0x0FF0},

   {{0, 20, 19, 11, 7, 23, 12, 8},
    s2x2,2,MPKIND__BENT2CW,0,     warn__none,  s4x6,      0x000, 0x4114},
   {{18, 10, 9, 17, 21, 5, 6, 22},
    s2x2,2,MPKIND__BENT2CCW,0,    warn__none,  s4x6,      0x000, 0xC33C},
   {{0, 1, 12, 17, 3, 8, 9, 10},
    s1x4,2,MPKIND__BENT2CW,0,     warn__none,  s3x6,      0x000, 0x0550},
   {{14, 13, 2, 17, 11, 8, 5, 4},
    s1x4,2,MPKIND__BENT2CCW,0,    warn__none,  s3x6,      0x000, 0x0FF0},

   {{10, 20, 15, 11, 3, 23, 22, 8},
    s2x2,2,MPKIND__BENT3CW,0,     warn__none,  s4x6,      0x000, 0x1441},
   {{18, 2, 9, 19, 21, 7, 6, 14},
    s2x2,2,MPKIND__BENT3CCW,0,    warn__none,  s4x6,      0x000, 0x3CC3},
   {{0, 15, 12, 13, 3, 4, 9, 6},
    s1x4,2,MPKIND__BENT3CW,0,     warn__none,  s3x6,      0x000, 0x5005},
   {{14, 15, 2, 1, 11, 10, 5, 6},
    s1x4,2,MPKIND__BENT3CCW,0,    warn__none,  s3x6,      0x000, 0xF00F},

   {{1, 9, 20, 0, 8, 12, 13, 21},
    s2x2,2,MPKIND__BENT4CW,0,     warn__none,  s4x6,      0x000, 0x1441},
   {{17, 9, 20, 16, 8, 4, 5, 21},
    s2x2,2,MPKIND__BENT4CCW,0,    warn__none,  s4x6,      0x000, 0x3CC3},
   {{0, 1, 5, 4, 11, 10, 6, 7},
    s1x4,2,MPKIND__BENT4CW,0,     warn__none,  sbigh,     0x000, 0x5005},
   {{3, 2, 5, 4, 11, 10, 9, 8},
    s1x4,2,MPKIND__BENT4CCW,0,    warn__none,  sbigh,     0x000, 0xF00F},

   {{11, 15, 16, 18, 4, 6, 23, 3},
    s2x2,2,MPKIND__BENT5CW,0,     warn__none,  s4x6,      0x000, 0x4114},
   {{11, 1, 2, 18, 14, 6, 23, 13},
    s2x2,2,MPKIND__BENT5CCW,0,    warn__none,  s4x6,      0x000, 0xC33C},
   {{0, 1, 5, 4, 11, 10, 6, 7},
    s1x4,2,MPKIND__BENT5CW,0,     warn__none,  sdeepxwv,  0x000, 0x0550},
   {{0, 1, 2, 3, 8, 9, 6, 7},
    s1x4,2,MPKIND__BENT5CCW,0,    warn__none,  sdeepxwv,  0x000, 0x0FF0},

   {{0, 5, 4, 1, 10, 7, 6, 11},
    s2x2,2,MPKIND__BENT6CW,0,     warn__none,  sbigh,     0x000, 0x4114},
   {{2, 4, 5, 3, 11, 9, 8, 10},
    s2x2,2,MPKIND__BENT6CCW,0,    warn__none,  sbigh,     0x000, 0xC33C},
   {{0, 1, 20, 9, 8, 21, 12, 13},
    s1x4,2,MPKIND__BENT6CW,0,     warn__none,  s4x6,      0x000, 0x0550},
   {{17, 16, 9, 20, 21, 8, 5, 4},
    s1x4,2,MPKIND__BENT6CCW,0,    warn__none,  s4x6,      0x000, 0x0FF0},

   {{1, 4, 5, 0, 11, 6, 7, 10},
    s2x2,2,MPKIND__BENT7CW,0,     warn__none,  sdeepxwv,  0x000, 0x1441},
   {{0, 2, 3, 1, 9, 7, 6, 8},
    s2x2,2,MPKIND__BENT7CCW,0,    warn__none,  sdeepxwv,  0x000, 0x3CC3},
   {{11, 18, 15, 16, 3, 4, 23, 6},
    s1x4,2,MPKIND__BENT7CW,0,     warn__none,  s4x6,      0x000, 0x5005},
   {{18, 11, 2, 1, 14, 13, 6, 23},
    s1x4,2,MPKIND__BENT7CCW,0,    warn__none,  s4x6,      0x000, 0xF00F},

   // Stuff for bent CLW in unsymmetrical 4x4.

   {{8, 9, 14, 0, 1, 3, 11, 6},
    s2x4,1,MPKIND__BENT8NE,0,     warn__none,  s4x4,      0x000, 0xF00F},
   {{13, 15, 7, 2, 4, 5, 10, 12},
    s2x4,1,MPKIND__BENT8SE,0,     warn__none,  s4x4,      0x000, 0x5005},
   {{9, 11, 3, 14, 0, 1, 6, 8},
    s2x4,1,MPKIND__BENT8SW,0,     warn__none,  s4x4,      0x000, 0x0FF0},
   {{12, 13, 2, 4, 5, 7, 15, 10},
    s2x4,1,MPKIND__BENT8NW,0,     warn__none,  s4x4,      0x000, 0x0550},

   // Stuff for 1x8.

   {{1, 9, 8, 12, 13, 21, 20, 0},
    s2x4,1,MPKIND__BENT4CW,0,     warn__none,  s4x6,      0x000, 0x4141},
   {{17, 9, 8, 4, 5, 21, 20, 16},
    s2x4,1,MPKIND__BENT4CCW,0,    warn__none,  s4x6,      0x000, 0xC3C3},
   {{0, 1, 5, 4, 6, 7, 11, 10},
    s1x8,1,MPKIND__BENT4CW,0,     warn__none,  sbigh,     0x000, 0x0505},
   {{3, 2, 5, 4, 9, 8, 11, 10},
    s1x8,1,MPKIND__BENT4CCW,0,    warn__none,  sbigh,     0x000, 0x0F0F},

   // Stuff for 1x6.

   {{5, 4, 3, 11, 10, 9},
    s1x6,1,MPKIND__BENT0CW,0,     warn__none, sbigptpd,   0x000, 0x145},
   {{0, 1, 3, 6, 7, 9},
    s1x6,1,MPKIND__BENT0CCW,0,    warn__none, sbigptpd,   0x000, 0x3CF},

   {{0, 1, 3, 2,        4, 5, 10, 11,      9, 8, 6, 7},
    s1x4,3,MPKIND__HET_SPLIT,1,  warn__none,  sbigh,     (((veryshort) s1x4) << 24) | 0x011, 0},
   {{0, 1, 3, 2,        4, 5, 10, 11,      9, 8, 6, 7},
    s1x4,3,MPKIND__HET_SPLIT,0,  warn__none,  sbigx,     (((veryshort) s1x4) << 24) | 0x004, 0},
   {{0, 10, 8, 9,       11, 1, 5, 7,       2, 3, 6, 4},
    sdmd,3,MPKIND__HET_SPLIT,1,  warn__none,  s_3mdmd,   (((veryshort) sdmd) << 24) | 0x011, 0},
   {{9, 0, 10, 8,       1, 5, 7, 11,       4, 2, 3, 6},
    sdmd,3,MPKIND__HET_SPLIT,0,  warn__none,  s_3mptpd,  (((veryshort) sdmd) << 24) | 0x004, 0},

   {{0, 1, 3, 2,        4, 5, 7, 6,      11, 10, 8, 9,        15, 14, 12, 13},
    s1x4,4,MPKIND__HET_SPLIT,1,  warn__none, sbigbigh,  (((veryshort) s1x4) << 24) | 0x011, 0},
   {{0, 1, 3, 2,        4, 5, 7, 6,      11, 10, 8, 9,        15, 14, 12, 13},
    s1x4,4,MPKIND__HET_SPLIT,0,  warn__none, sbigbigx,  (((veryshort) s1x4) << 24) | 0x044, 0},
   {{0, 13, 11, 12,     14, 1, 15, 10,          3, 4, 8, 5,     7, 2, 6, 9},
    sdmd,4,MPKIND__HET_SPLIT,1,  warn__none, s_4mdmd,   (((veryshort) sdmd) << 24) | 0x011, 0},
   {{12, 0, 13, 11,     1, 15, 10, 14,          5, 3, 4, 8,     2, 6, 9, 7},
    sdmd,4,MPKIND__HET_SPLIT,0,  warn__none, s_4mptpd,  (((veryshort) sdmd) << 24) | 0x044, 0},

   // See t60@6140.
   {{11, 10, 7, 6, 23, 22, 19, 18,    15, 20, 9, 2, 3, 8, 21, 14},
    s2x4,2,MPKIND__HET_CONCPHAN,0, warn__none, s4x6,    (((veryshort) s2x4) << 24) | 0x10001, 0xFFFFFFFF},

   {{0, 3, 4, 5, 8, 11, 12, 13,       14, 1, 15, 10, 6, 9, 7, 2},
    s_qtag,2,MPKIND__HET_CONCPHAN,0, warn__none, s_4mdmd, (((veryshort) s_ptpd) << 24) | 0x000, 0},
   {{12, 0, 13, 11, 4, 8, 5, 3,       1, 2, 6, 7, 9, 10, 14, 15},
    s_ptpd,2,MPKIND__HET_CONCPHAN,0, warn__none, s_4mptpd, (((veryshort) s_qtag) << 24) | 0x000, 0},

   {{1, 0,                             2, 3},
    s1x2,2,MPKIND__HET_SPLIT,0,   warn__none, s_trngl4, (((veryshort) s1x2) << 24) | 0x0001, 0},
   {{2, 3,                             1, 0},
    s1x2,2,MPKIND__HET_SPLIT,1,   warn__none, s_trngl4, (((veryshort) s1x2) << 24) | 0x20004, 0},
   {{0, 1,                             2, 3},
    s1x2,2,MPKIND__HET_SPLIT,0,   warn__none, s_trngl4, (((veryshort) s1x2) << 24) | 0x10004, 0xFF},
   {{3, 2,                             1, 0},
    s1x2,2,MPKIND__HET_SPLIT,1,   warn__none, s_trngl4, (((veryshort) s1x2) << 24) | 0x30001, 0x55},

   {{6, 7,      0, 5,      3, 2,     1, 4},
    s1x2,4,MPKIND__HET_SPLIT,0,   warn__none, s_rigger, (((veryshort) s1x2) << 24) | 0x0044, 0},
   {{0, 5,      6, 7,      1, 4,     3, 2},
    s1x2,4,MPKIND__HET_SPLIT,1,   warn__none, s_bone,   (((veryshort) s1x2) << 24) | 0x0011, 0},

   {{3, 2, 0, 1,                       4, 5, 7, 6},
    s1x4,2,MPKIND__HET_SPLIT,0,   warn__none, s_trngl8, (((veryshort) s1x4) << 24) | 0x0001, 0},
   {{4, 5, 7, 6,                       3, 2, 0, 1},
    s1x4,2,MPKIND__HET_SPLIT,1,   warn__none, s_trngl8, (((veryshort) s1x4) << 24) | 0x20004, 0},

   {{0, 1, 3, 2,                       4, 5, 7, 6},
    s1x4,2,MPKIND__HET_SPLIT,0,   warn__none, s_trngl8, (((veryshort) s1x4) << 24) | 0x10004, 0xFFFF},
   {{7, 6, 4, 5,                       3, 2, 0, 1},
    s1x4,2,MPKIND__HET_SPLIT,1,   warn__none, s_trngl8, (((veryshort) s1x4) << 24) | 0x30001, 0x5555},

   {{2, 3, 0, 1,                       4, 5, 6, 7},
    sdmd,2,MPKIND__HET_SPLIT,0,   warn__none, sdmdpdmd, (((veryshort) sdmd) << 24) | 0x001, 0},
   {{4, 5, 6, 7,                       2, 3, 0, 1},
    sdmd,2,MPKIND__HET_SPLIT,1,   warn__none, sdmdpdmd, (((veryshort) sdmd) << 24) | 0x20004, 0},

   {{4, 5, 3,                          1, 2, 0},
    s_trngl,2,MPKIND__HET_SPLIT,1,      warn__none, s_short6, (((veryshort) s_trngl) << 24) | 0x108, 0},
   {{4, 5, 3,                          1, 2, 0},
    s_trngl,2,MPKIND__HET_SPLIT,1,      warn__none, s_short6, (((veryshort) s_trngl) << 24) | 0x1010D, 0xFFF},

   {{5, 4, 0,                          2, 1, 3},
    s_trngl,2,MPKIND__HET_SPLIT,1, warn__none, s_bone6, (((veryshort) s_trngl) << 24) | 0x107, 0},
   {{2, 1, 3,                          5, 4, 0},
    s_trngl,2,MPKIND__HET_SPLIT,1, warn__none, s_bone6, (((veryshort) s_trngl) << 24) | 0x10102, 0xFFF},

   {{5, 0, 1,                          2, 3, 4},
    s_trngl,2,MPKIND__HET_SPLIT,0,   warn__none, s_ntrgl6ccw, (((veryshort) s_trngl) << 24) | 0x30107, 0x555},
   {{0, 4, 5,                          3, 1, 2},
    s_trngl,2,MPKIND__HET_SPLIT,0,   warn__none, s_ntrgl6cw,  (((veryshort) s_trngl) << 24) | 0x102, 0},
   {{5, 0, 1,                          2, 3, 4},
    s_trngl,2,MPKIND__HET_SPLIT,0,   warn__none, s_ntrgl6ccw, (((veryshort) s_trngl) << 24) | 0x108, 0},
   {{0, 4, 5,                          3, 1, 2},
    s_trngl,2,MPKIND__HET_SPLIT,0,   warn__none, s_ntrgl6cw,  (((veryshort) s_trngl) << 24) | 0x3010D, 0x555},

   // Need both of these.  The first to keep "fix_n_results" from getting bent out of shape when
   // it sees that the setup isn't really symmetric, and the second because it will get turned
   // into MPKIND__SPLIT when it sees that the two triangles are the same after all.
   {{0, 1, 2, 3,                       4, 5, 6, 7},
    s_trngl4,2,MPKIND__HET_SPLIT,1,  warn__none, sdbltrngl4,  (((veryshort) s_trngl4) << 24) | 0x005, 0},
   //   {{0, 1, 2, 3,                       4, 5, 6, 7},
   //    s_trngl4,2,MPKIND__SPLIT,1,      0, sdbltrngl4,  0x005, 0},
   {{4, 5, 6, 7,                       0, 1, 2, 3},
    s_trngl4,2,MPKIND__HET_SPLIT,1,  warn__none, sdbltrngl4,  (((veryshort) s_trngl4) << 24) | 0x2000F, 0xAAAA},

   {{6, 7, 0, 5,                       2, 3, 4, 1},
    s_trngl4,2,MPKIND__HET_SPLIT,1,     warn__none, s_rigger, (((veryshort) s_trngl4) << 24) | 0x00D, 0},
   {{7, 6, 5, 0,                       3, 2, 1, 4},
    s_trngl4,2,MPKIND__HET_SPLIT,1,     warn__none, s_bone,   (((veryshort) s_trngl4) << 24) | 0x007, 0},
   {{13, 15, 11, 9,                    5, 7, 3, 1},
    s_trngl4,2,MPKIND__HET_SPLIT,0,   warn__none, s_c1phan,   (((veryshort) s_trngl4) << 24) | 0x00D, 0},
   {{8, 10, 12, 14,                    0, 2, 4, 6},
    s_trngl4,2,MPKIND__HET_SPLIT,0,   warn__none, s_c1phan,   (((veryshort) s_trngl4) << 24) | 0x007, 0},

   {{12, 14, 0, 2,                     4, 6, 8, 10},
    s_trngl4,2,MPKIND__HET_SPLIT,0,   warn__none, s_c1phan,   (((veryshort) s_trngl4) << 24) | 0x008, 0},
   {{1, 3, 15, 13,                     9, 11, 7, 5},
    s_trngl4,2,MPKIND__HET_SPLIT,0,   warn__none, s_c1phan,   (((veryshort) s_trngl4) << 24) | 0x002, 0},

   {{12, 14, 0, 2,                     9, 11, 7, 5},
    s_trngl4,2,MPKIND__HET_SPLIT,0,   warn__none, s_c1phan,   (((veryshort) s_trngl4) << 24) | 0x000, 0},
   {{1, 3, 15, 13,                     4, 6, 8, 10},
    s_trngl4,2,MPKIND__HET_SPLIT,0,   warn__none, s_c1phan,   (((veryshort) s_trngl4) << 24) | 0x00A, 0},
   {{13, 15, 11, 9,                    0, 2, 4, 6},
    s_trngl4,2,MPKIND__HET_SPLIT,0,   warn__none, s_c1phan,   (((veryshort) s_trngl4) << 24) | 0x005, 0},

   {{12, 15, 14, 1, 4, 7, 6, 9,        10, 13, 3, 0, 2, 5, 11, 8},
    s2x4,2,MPKIND__STAG,0,        warn__none,  s4x4,      0x000, 0},
   {{10, 13, 3, 0, 2, 5, 11, 8,        12, 15, 14, 1, 4, 7, 6, 9},
    s2x4,2,MPKIND__STAG,1,        warn__none,  s4x4,      0x000, 0},
   {{12, 14, 1, 7, 4, 6, 9, 15,        13, 0, 2, 3, 5, 8, 10, 11},
    s_qtag,2,MPKIND__STAG,0,        warn__none,  s4x4,    0x000, 0},
   {{13, 0, 2, 3, 5, 8, 10, 11,        12, 14, 1, 7, 4, 6, 9, 15},
    s_qtag,2,MPKIND__STAG,1,        warn__none,  s4x4,    0x000, 0},

   {{10, 13, 14, 1, 4, 7, 11, 8,       12, 15, 3, 0, 2, 5, 6, 9},
    s2x4,2,MPKIND__OX,0,        warn__none,  s4x4,      0x000, 0},
   {{10, 13, 3, 0, 4, 7, 6, 9,         12, 15, 14, 1, 2, 5, 11, 8},
    s2x4,2,MPKIND__OX,1,        warn__none,  s4x4,      0x000, 0},

   {{14, 1, 4, 7, 6, 9, 12, 15,        2, 5, 8, 11, 10, 13, 0, 3},
    s_qtag,2,MPKIND__DIAGQTAG,0,    warn__none,  s4x4,    0x000, 0},
   {{14, 1, 4, 7, 6, 9, 12, 15,        10, 13, 0, 3, 2, 5, 8, 11},
    s_qtag,2,MPKIND__DIAGQTAG,1,    warn__none,  s4x4,    0x000, 0},
   {{7, 23, 13, 21, 19, 11, 1, 9,      6, 22, 16, 20, 18, 10, 4, 8},
    s_qtag,2,MPKIND__DIAGQTAG4X6,0,    warn__none,  s4x6, 0x005, 0},

   {{7, 1, 4, 2,                       0, 6, 3, 5},
    s1x4,2,MPKIND__MAGIC,1,       warn__none,  s2x4,      0x000, 0},
   {{0, 2, 5, 3,                       1, 7, 4, 6},
    sdmd,2,MPKIND__MAGIC,1,       warn__none,  s_qtag,    0x005, 0},
   {{0, 3, 5, 6,                       1, 2, 4, 7},
    sdmd,2,MPKIND__INTLKDMD,1,    warn__none,  s_qtag,    0x005, 0},
   {{0, 2, 5, 7,                       1, 3, 4, 6},
    sdmd,2,MPKIND__MAGICINTLKDMD,1,warn__none, s_qtag,    0x005, 0},
   {{11, 10, 2, 6, 7, 3,               0, 1, 9, 5, 4, 8},
    s_1x2dmd,2,MPKIND__MAGIC,    1,warn__none,  sbigdmd,   0x000, 0},
   {{11, 10, 3, 6, 7, 8,               0, 1, 2, 5, 4, 9},
    s_1x2dmd,2,MPKIND__INTLKDMD,1,warn__none,  sbigdmd,   0x000, 0},
   {{11, 10, 2, 6, 7, 9,               0, 1, 3, 5, 4, 8},
    s_1x2dmd,2,MPKIND__MAGICINTLKDMD,1,warn__none,sbigdmd,0x000, 0},

   {{0, 3, 4, 7,           2, 5, 6, 1},
    sdmd,2,MPKIND__NONISOTROPDMD,0,   warn__none,  s_thar,    0x004, 0},
   {{0, 3, 4, 7,           2, 5, 6, 1},
    sdmd,2,MPKIND__NONISOTROPDMD,1,   warn__none,  s_thar,    0x104, 0},
   {{0, 2,                 1, 3},
    s1x2,2,MPKIND__NONISOTROPDMD,0,   warn__none,  sdmd,      0x004, 0},
   {{1, 3,                 0, 2},
    s1x2,2,MPKIND__NONISOTROPDMD,1,   warn__none,  sdmd,      0x001, 0},
   {{0, 1, 2, 3,           0, 1, 2, 3},
    s2x2,2,MPKIND__NONISOTROPDMD,0,   warn__none,  s2x2,      0x000, 0},
   {{0, 1, 4, 5,           2, 3, 6, 7},
    s1x4,2,MPKIND__NONISOTROPDMD,0,   warn__none,  s_thar,    0x004, 0},
   {{0, 1, 4, 5,           2, 3, 6, 7},
    s1x4,2,MPKIND__NONISOTROPDMD,1,   warn__none,  s_thar,    0x104, 0},
   {{1, 2,                 3, 4,                 6, 5,               0, 7},
    s1x2,4,MPKIND__NONISOTROPDMD,0, warn__none,    s2x4,      0x144, 0},
   {{2, 3,                 5, 4,               7, 6,                 0, 1},
    s1x2,4,MPKIND__NONISOTROPDMD,1, warn__none,  s_crosswave, 0x11, 0},
   {{0, 1,                 3, 2,                 5, 4,               6, 7},
    s1x2,4,MPKIND__DMD_STUFF,0, warn__none,    s_rigger,      0, 0},
   {{6, 7,                 0, 1,                 3, 2,               5, 4},
    s1x2,4,MPKIND__DMD_STUFF,1, warn__none,    s_qtag,      0, 0},
   // Same as nonisotropic one, above.  2x2's are always isotropic.
   {{0, 1, 2, 3,           0, 1, 2, 3},
    s2x2,2,MPKIND__DMD_STUFF,0,   warn__none,  s2x2,      0x000, 0},

   {{13, 14, 5, 6,         10, 1, 2, 9},
    s2x2,2,MPKIND__ALL_8,0,       warn__none,  s4x4,      0x000, 0},
   {{10, 1, 2, 9,          13, 14, 5, 6},
    s2x2,2,MPKIND__ALL_8,1,       warn__none,  s4x4,      0x000, 0},
   {{0, 3, 4, 7,           2, 5, 6, 1},
    sdmd,2,MPKIND__ALL_8,0,       warn__none,  s_thar,    0x004, 0},
   {{2, 5, 6, 1,           0, 3, 4, 7},
    sdmd,2,MPKIND__ALL_8,1,       warn__none,  s_thar,    0x001, 0},
   {{0, 1, 4, 5,           2, 3, 6, 7},
    s1x4,2,MPKIND__ALL_8,0,       warn__none,  s_thar,    0x004, 0},
   {{2, 3, 6, 7,           0, 1, 4, 5},
    s1x4,2,MPKIND__ALL_8,1,       warn__none,  s_thar,    0x001, 0},

   {{13, 14,               1, 2,                 6, 5,               10, 9},
    s1x2,4,MPKIND__4_EDGES_FROM_4X4,0, warn__none,s4x4,   0, 0x5050},
   {{0, 1,                 2, 3,                 5, 4,               7, 6},
    s1x2,4,MPKIND__4_EDGES_FROM_4X4,1, warn__none,s_thar, 0, 0x5050},

   {{0, 1,                 2, 3,                 5, 4,               7, 6},
    s1x2,4,MPKIND__4_EDGES,0,     warn__none,  s_alamo,   0, 0x5050},
   {{0, 1,                 2, 3,                 5, 4,               7, 6},
    s1x2,4,MPKIND__4_EDGES,1,     warn__none,  s_thar,    0, 0x5050},

   {{0, 2,                 4, 6,                 10, 8,              14, 12},
    s1x2,4,MPKIND__4_QUADRANTS,0, warn__none,  s_c1phan,  0x044, 0},
   {{1, 3,                 7, 5,                 11, 9,              13, 15},
    s1x2,4,MPKIND__4_QUADRANTS,1, warn__none,  s_c1phan,  0x011, 0},
   {{0, 1, 2, 3,        7, 4, 5, 6,        10, 11, 8, 9,    13, 14, 15, 12},
    s_star,4,MPKIND__4_QUADRANTS,0, warn__none, s_c1phan, 0x000, 0},

   {{0, 1,                 2, 3,                 5, 4,              7, 6},
    s1x2,4,MPKIND__4_QUADRANTS_WITH_45_ROTATION,0, warn__none,  s_thar,  0x044, 0},
   {{5, 4,                 6, 7,                 0, 1,              3, 2},
    s1x2,4,MPKIND__4_QUADRANTS_WITH_45_ROTATION,1, warn__none,  s_alamo, 0x0CC, 0},

   {{0, 1, 2, 3, 6, 7, 8, 9},
    s2x4,1,MPKIND__OFFS_L_HALF,1, warn__none,  s2x6,      0x000, 0},
   {{2, 3, 4, 5, 8, 9, 10, 11},
    s2x4,1,MPKIND__OFFS_R_HALF,1, warn__none,  s2x6,      0x000, 0},

   {{12, 15, 4, 7,   8, 11, 0, 3},  // need for NS swing thru
    s1x4,2,MPKIND__NS_CROSS_IN_4X4,0, warn__none, s4x4,      0x000, 0},
   {{0, 3, 8, 11,    12, 15, 4, 7}, // coming out of NS spin the top
    s1x4,2,MPKIND__NS_CROSS_IN_4X4,1, warn__none, s4x4,      0x005, 0},
   // Now switch rotation of EW maps:  1<->0
   {{12, 15, 4, 7,   0, 3, 8, 11},  // need for EW swing thru
    s1x4,2,MPKIND__EW_CROSS_IN_4X4,0, warn__none, s4x4,      0x005, 0},
   {{8, 11, 0, 3,    12, 15, 4, 7}, // coming out of EW spin the top
    s1x4,2,MPKIND__EW_CROSS_IN_4X4,1, warn__none, s4x4,      0x000, 0},

   {{0, 1, 2, 3, 5, 6, 7, 8},
    s2x4,1,MPKIND__OFFS_L_ONEQ,1, warn__none,  s2x5,      0x000, 0},
   {{1, 2, 3, 4, 6, 7, 8, 9},
    s2x4,1,MPKIND__OFFS_R_ONEQ,1, warn__none,  s2x5,      0x000, 0},

   {{0, 1, 2, 3, 7, 8, 9, 10},
    s2x4,1,MPKIND__OFFS_L_THRQ,1, warn__none,  s2x7,      0x000, 0},
   {{3, 4, 5, 6, 10, 11, 12, 13},
    s2x4,1,MPKIND__OFFS_R_THRQ,1, warn__none,  s2x7,      0x000, 0},

   {{15, 14, 12, 13, 7, 6, 4, 5},
    s1x8,1,MPKIND__OFFS_L_FULL,0, warn__none,  s2x8,      0x000, 0},
   {{0, 1, 3, 2, 8, 9, 11, 10},
    s1x8,1,MPKIND__OFFS_R_FULL,0, warn__none,  s2x8,      0x000, 0},

   {{0, 1, 2, 3, 8, 9, 10, 11},
    s2x4,1,MPKIND__OFFS_L_FULL,1, warn__none,  s2x8,      0x000, 0},
   {{4, 5, 6, 7, 12, 13, 14, 15},
    s2x4,1,MPKIND__OFFS_R_FULL,1, warn__none,  s2x8,      0x000, 0},

   {{0, 1, 2, 3, 4, 10, 11, 12, 13, 14},
    s2x5,1,MPKIND__OFFS_L_FULL,1, warn__none,  s2x10,     0x000, 0},
   {{5, 6, 7, 8, 9, 15, 16, 17, 18, 19},
    s2x5,1,MPKIND__OFFS_R_FULL,1, warn__none,  s2x10,     0x000, 0},

   {{0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17},
    s2x6,1,MPKIND__OFFS_L_FULL,1, warn__none,  s2x12,     0x000, 0},
   {{6, 7, 8, 9, 10, 11, 18, 19, 20, 21, 22, 23},
    s2x6,1,MPKIND__OFFS_R_FULL,1, warn__none,  s2x12,     0x000, 0},

   {{-1, -1, 0, 1, 2, 3, 4, 5, -1, -1, 12, 13, 14, 15, 16, 17},
    s2x8,1,MPKIND__OFFS_L_FULL,1, warn__none,  s2x12,     0x000, 0},
   {{6, 7, 8, 9, 10, 11, -1, -1, 18, 19, 20, 21, 22, 23, -1, -1},
    s2x8,1,MPKIND__OFFS_R_FULL,1, warn__none,  s2x12,     0x000, 0},

   {{0, 1, 2, 3, 7, 8, 9, 10, 11, 12, 16, 17},
    s3x4,1,MPKIND__OFFS_L_HALF,1, warn_partial_solomon,  s3x6,      0x000, 0},
   {{2, 3, 4, 5, 7, 8, 11, 12, 13, 14, 16, 17},
    s3x4,1,MPKIND__OFFS_R_HALF,1, warn_partial_solomon,  s3x6,      0x000, 0},

   {{3, 8, 23, 9, 12, 13, 14, 22, 15, 20, 11, 21, 0, 1, 2, 10},
    s4x4,1,MPKIND__OFFS_L_HALF,1, warn__none,  s4x6,      0x000, 0},
   {{5, 6, 21, 7, 14, 15, 16, 20, 17, 18, 9, 19, 2, 3, 4, 8},
    s4x4,1,MPKIND__OFFS_R_HALF,1, warn__none,  s4x6,      0x000, 0},

   {{-1, -1, 20, 6, 15, 16, 17, 19, -1, -1, 8, 18, 3, 4, 5, 7},
    s4x4,1,MPKIND__OFFS_R_FULL,1, warn__none,  s4x6,      0x000, 0},
   {{2, 9, -1, 10, -1, 12, 13, 23, 14, 21, -1, 22, -1, 0, 1, 11},
    s4x4,1,MPKIND__OFFS_L_FULL,1, warn__none,  s4x6,      0x000, 0},
   {{12, 13, 14, 22, 15, 20, 11, 21, 0, 1, 2, 10, 3, 8, 23, 9},
    s4x4,1,MPKIND__OFFS_L_HALF,0, warn__none,  s4x6,      0x001, 0},
   {{14, 15, 16, 20, 17, 18, 9, 19, 2, 3, 4, 8, 5, 6, 21, 7},
    s4x4,1,MPKIND__OFFS_R_HALF,0, warn__none,  s4x6,      0x001, 0},
   {{-1, -1, 20, 6, 15, 16, 17, 19, -1, -1, 8, 18, 3, 4, 5, 7},
    s4x4,1,MPKIND__OFFS_R_FULL,0, warn__none,  s4x6,      0x001, 0},
   {{-1, 12, 13, 23, 14, 21, -1, 22, -1, 0, 1, 11, 2, 9, -1, 10},
    s4x4,1,MPKIND__OFFS_L_FULL,0, warn__none,  s4x6,      0x001, 0},

   {{0, 1, 3, 4, 5, 6, 8, 9},
    s_qtag,1,MPKIND__OFFS_L_HALF,1, warn_partial_solomon,  s_343,   0x000, 0},
   {{1, 2, 3, 4, 6, 7, 8, 9},
    s_qtag,1,MPKIND__OFFS_R_HALF,1, warn_partial_solomon,  s_343,   0x000, 0},

   {{0, 1, 2, 3, 4, 5, 6, 7},
    s_qtag,1,MPKIND__OFFS_L_HALF,0, warn__none, spgdmdccw,0x000, 0},
   {{0, 1, 2, 3, 4, 5, 6, 7},
    s_qtag,1,MPKIND__OFFS_R_HALF,0, warn__none, spgdmdcw, 0x000, 0},

   {{3, 11, 6, 5, 10, 4, 13, 12},
    s_rigger,1,MPKIND__OFFS_L_HALF,0, warn__none, sd2x7,  0x000, 0},
   {{2, 10, 7, 8, 9, 3, 0, 1},
    s_rigger,1,MPKIND__OFFS_R_HALF,0, warn__none, sd2x7, 0x000, 0},

   {{0, 1, 2, 3, 4, 5, 9, 10, 11, 12, 13, 14},
    s2x6,1,MPKIND__OFFS_L_HALF,1, warn_pg_in_2x6,  s2x9,      0x000, 0},
   {{3, 4, 5, 6, 7, 8, 12, 13, 14, 15, 16, 17},
    s2x6,1,MPKIND__OFFS_R_HALF,1, warn_pg_in_2x6,  s2x9,      0x000, 0},

   {{0, 1, 2, 3, 4, 5, 6, 7},
    s1x8,1,MPKIND__OFFS_L_HALF,1, warn__offset_gone,  s1x8,      0x000, 0},
   {{0, 1, 2, 3, 4, 5, 6, 7},
    s1x8,1,MPKIND__OFFS_R_HALF,1, warn__offset_gone,  s1x8,      0x000, 0},

   {{0, 1, 2, 3, 4, 5, 6, 7},
    s1x8,1,MPKIND__OFFS_L_ONEQ,1, warn__offset_gone,  s1x8,      0x000, 0},
   {{0, 1, 2, 3, 4, 5, 6, 7},
    s1x8,1,MPKIND__OFFS_R_ONEQ,1, warn__offset_gone,  s1x8,      0x000, 0},

   {{0, 1, 2, 3, 4, 5, 6, 7},
    s1x8,1,MPKIND__OFFS_L_THRQ,1, warn__offset_gone,  s1x8,      0x000, 0},
   {{0, 1, 2, 3, 4, 5, 6, 7},
    s1x8,1,MPKIND__OFFS_R_THRQ,1, warn__offset_gone,  s1x8,      0x000, 0},

   {{0, 1, 2, 3, 4, 5, 6, 7},
    s1x8,1,MPKIND__OFFS_L_FULL,1, warn__offset_gone,  s1x8,      0x000, 0},
   {{0, 1, 2, 3, 4, 5, 6, 7},
    s1x8,1,MPKIND__OFFS_R_FULL,1, warn__offset_gone,  s1x8,      0x000, 0},

   {{10, 11, 2, 3, 4, 5, 8, 9},
    s2x4,1,MPKIND__OFFS_L_HALF,0, warn__none,  s3x4,      0x000, 0},
   {{0, 1, 5, 4, 6, 7, 11, 10},
    s2x4,1,MPKIND__OFFS_R_HALF,0, warn__none,  s3x4,      0x000, 0},

   {{15, 16, 17, 3, 4, 5, 6, 7, 8, 12, 13, 14},
    s2x6,1,MPKIND__OFFS_L_HALF,0, warn__none,  s3x6,      0x000, 0},
   {{0, 1, 2, 8, 7, 6, 9, 10, 11, 17, 16, 15},
    s2x6,1,MPKIND__OFFS_R_HALF,0, warn__none,  s3x6,      0x000, 0},
   {{13, 15, 2, 4, 5, 7, 10, 12},
    s2x4,1,MPKIND__OFFS_L_FULL,0, warn__none,  s4x4,      0x001, 0},
   {{0, 1, 11, 6, 8, 9, 3, 14},
    s2x4,1,MPKIND__OFFS_R_FULL,0, warn__none,  s4x4,      0x001, 0},

   {{9, 11, 14, 0, 1, 3, 6, 8,         12, 13, 7, 2, 4, 5, 15, 10},
    s2x4,2,MPKIND__OFFS_BOTH_FULL,0, warn__none, s4x4,    0x000, 0},
   {{0, 1, 2, 3, 8, 9, 10, 11,         4, 5, 6, 7, 12, 13, 14, 15},
    s2x4,2,MPKIND__OFFS_BOTH_FULL,1, warn__none, s2x8,    0x000, 0},

   {{10, 11, 8, 9,                     2, 3, 4, 5},
    s2x2,2,MPKIND__OFFS_L_HALF,0, warn__none,  s3x4,      0x000, 0},
   {{0, 1, 11, 10,                     5, 4, 6, 7},
    s2x2,2,MPKIND__OFFS_R_HALF,0, warn__none,  s3x4,      0x000, 0},
   {{11, 8, 9, 10,                     3, 4, 5, 2},
    s2x2,2,MPKIND__OFFS_L_HALF,1, warn__none,  s3x4,      0x005, 0},
   {{1, 11, 10, 0,                     4, 6, 7, 5},
    s2x2,2,MPKIND__OFFS_R_HALF,1, warn__none,  s3x4,      0x005, 0},

   {{10, 15, 0, 14, 9, 11, 1, 3, 8, 6, 2, 7},
    s1x4,3,MPKIND__OVLOFS_L_HALF,0, warn__none,  s4x4,      0x000, 0},
   {{12, 13, 1, 3, 10, 15, 2, 7, 9, 11, 4, 5},
    s1x4,3,MPKIND__OVLOFS_R_HALF,0, warn__none,  s4x4,      0x000, 0},

   {{0, 1, 3, 2,       4, 5, 10, 11,     9, 8, 6, 7},
    s1x4,3,MPKIND__OVLOFS_L_HALF,1, warn__offset_gone,  s1x12,     0x000, 0},
   {{0, 1, 3, 2,       4, 5, 10, 11,     9, 8, 6, 7},
    s1x4,3,MPKIND__OVLOFS_R_HALF,1, warn__offset_gone,  s1x12,     0x000, 0},

   {{5, 8, 9, 4,       3, 10, 11, 2,     1, 12, 13, 0},
    s2x2,3,MPKIND__OVLOFS_L_HALF,0, warn__none,  s2x8,      0x015, 0},
   {{7, 10, 11, 6,     5, 12, 13, 4,     3, 14, 15, 2},
    s2x2,3,MPKIND__OVLOFS_R_HALF,0, warn__none,  s2x8,      0x015, 0},
   {{0, 1, 12, 13,     2, 3, 10, 11,     4, 5, 8, 9},
    s2x2,3,MPKIND__OVLOFS_L_HALF,1, warn__none,  s2x8,      0x000, 0},
   {{2, 3, 14, 15,     4, 5, 12, 13,     6, 7, 10, 11},
    s2x2,3,MPKIND__OVLOFS_R_HALF,1, warn__none,  s2x8,      0x000, 0},

   {{10, 15, 14, 0, 1, 3, 11, 9,     9, 11, 3, 1, 2, 7, 6, 8},
    s2x4,2,MPKIND__OVLOFS_L_HALF,0, warn__none,s4x4,      0x000, 0},
   {{12, 13, 3, 1, 2, 7, 15, 10,     10, 15, 7, 2, 4, 5, 11, 9},
    s2x4,2,MPKIND__OVLOFS_R_HALF,0, warn__none,s4x4,      0x000, 0},
   {{0, 1, 2, 3, 10, 11, 12, 13,     2, 3, 4, 5, 8, 9, 10, 11},
    s2x4,2,MPKIND__OVLOFS_L_HALF,1, warn__none,s2x8,      0x000, 0},
   {{2, 3, 4, 5, 12, 13, 14, 15,     4, 5, 6, 7, 10, 11, 12, 13},
    s2x4,2,MPKIND__OVLOFS_R_HALF,1, warn__none,s2x8,      0x000, 0},
   {{0, 1, 3, 2, 10, 11, 4, 5,       4, 5, 10, 11, 6, 7, 9, 8},
    s1x8,2,MPKIND__OVLOFS_L_HALF,1, warn__offset_gone,  s1x12,     0x000, 0},
   {{0, 1, 3, 2, 10, 11, 4, 5,       4, 5, 10, 11, 6, 7, 9, 8},
    s1x8,2,MPKIND__OVLOFS_R_HALF,1, warn__offset_gone,  s1x12,     0x000, 0},

   {{10, 9,          1, 11,            5, 7,         3, 4},
    s1x2,4,MPKIND__OFFS_L_HALF_STAGGER,1, warn__none,  s3x4,      0x055, 0},
   {{0, 10,          11, 8,            2, 5,         4, 6},
    s1x2,4,MPKIND__OFFS_R_HALF_STAGGER,1, warn__none,  s3x4,      0x055, 0},

   {{15, 14,         2, 3,             11, 10,       6, 7},
    s1x2,4,MPKIND__OFFS_L_HALF_STAGGER,0, warn__none,  s1p5x8,    0x000, 0},
   {{0, 1,           13, 12,           4, 5,         9, 8},
    s1x2,4,MPKIND__OFFS_R_HALF_STAGGER,0, warn__none,  s1p5x8,    0x000, 0},

   {{9, 11, 6, 8,                      14, 0, 1, 3},
    s2x2,2,MPKIND__OFFS_L_FULL,0, warn__none,  s4x4,      0x000, 0},
   {{12, 13, 15, 10,                   7, 2, 4, 5},
    s2x2,2,MPKIND__OFFS_R_FULL,0, warn__none,  s4x4,      0x000, 0},
   {{11, 6, 8, 9,                      0, 1, 3, 14},
    s2x2,2,MPKIND__OFFS_L_FULL,1, warn__none,  s4x4,      0x005, 0},
   {{13, 15, 10, 12,                   2, 4, 5, 7},
    s2x2,2,MPKIND__OFFS_R_FULL,1, warn__none,  s4x4,      0x005, 0},

   {{5, 0, 1,                     2, 3, 4},
    s_trngl,2,MPKIND__HET_OFFS_L_HALF,0, warn__none,s_nftrgl6ccw, (((veryshort) s_trngl) << 24) | 0x8, 0},
   {{0, 4, 5,                     3, 1, 2},
    s_trngl,2,MPKIND__HET_OFFS_R_HALF,0, warn__none,s_nftrgl6cw, (((veryshort) s_trngl) << 24) | 0x2, 0},

   // For the 45 degree couples hinge stuff.

   {{0, 1, 3, 2,                       11, 10, 8, 9},
    s1x4,2,MPKIND__FUDGYOFFS_L_HALF,0, warn__none,  sfudgy2x6l, 0x000, 0},

   {{15, 14, 12, 13,                   4, 5, 7, 6},
    s1x4,2,MPKIND__FUDGYOFFS_R_HALF,0, warn__none,  sfudgy2x6r, 0x000, 0},

   {{15, 14, 12, 13,                   4, 5, 7, 6},
    s1x4,2,MPKIND__OFFS_L_HALF,0, warn__none,  s1p5x8,    0x000, 0},

   {{0, 1, 3, 2,                       11, 10, 8, 9},
    s1x4,2,MPKIND__OFFS_R_HALF,0, warn__none,  s1p5x8,    0x000, 0},

   {{0, 1,                             5, 4},
    s1x2,2,MPKIND__FUDGYOFFS_L_HALF,0, warn__none,  sfudgy2x3l, 0x000, 0},
   {{7, 6,                             2, 3},
    s1x2,2,MPKIND__FUDGYOFFS_R_HALF,0, warn__none,  sfudgy2x3r, 0x000, 0},

   {{7, 6,                             2, 3},
    s1x2,2,MPKIND__OFFS_L_HALF,0, warn__none,  s1p5x4,    0x000, 0},
   {{0, 1,                             5, 4},
    s1x2,2,MPKIND__OFFS_R_HALF,0, warn__none,  s1p5x4,    0x000, 0},


   {{15, 14, 12, 13,                   4, 5, 7, 6},
    s1x4,2,MPKIND__OFFS_L_FULL,0, warn__none,  s2x8,      0x000, 0},
   {{0, 1, 3, 2,                       11, 10, 8, 9},
    s1x4,2,MPKIND__OFFS_R_FULL,0, warn__none,  s2x8,      0x000, 0},

   {{7, 6, 3, 2},
    s1x4,1,MPKIND__OFFS_L_FULL,0, warn__none,  s2x4,      0x000, 0},
   {{0, 1, 4, 5},
    s1x4,1,MPKIND__OFFS_R_FULL,0, warn__none,  s2x4,      0x000, 0},

   {{10, 15, 0, 14, 9, 11, 1, 3, 8, 6, 2, 7},
    s1x4,3,MPKIND__OFFS_L_FULL,0, warn__none,  s4x4,      0x000, 0},
   {{12, 13, 1, 3, 10, 15, 2, 7, 9, 11, 4, 5},
    s1x4,3,MPKIND__OFFS_R_FULL,0, warn__none,  s4x4,      0x000, 0},

   {{0, 1, 4, 5},
    s2x2,1,MPKIND__OFFS_L_FULL,1, warn__none,  s2x4,      0x000, 0},
   {{2, 3, 6, 7},
    s2x2,1,MPKIND__OFFS_R_FULL,1, warn__none,  s2x4,      0x000, 0},

   {{1, 4, 5, 0},
    s2x2,1,MPKIND__OFFS_L_FULL,0, warn__none,  s2x4,      0x001, 0},
   {{3, 6, 7, 2},
    s2x2,1,MPKIND__OFFS_R_FULL,0, warn__none,  s2x4,      0x001, 0},

   {{0, 1, 12, 13, 2, 3, 10, 11, 4, 5, 8, 9},
    s2x2,3,MPKIND__OFFS_L_FULL,1, warn__none,  s2x8,      0x000, 0},
   {{2, 3, 14, 15, 4, 5, 12, 13, 6, 7, 10, 11},
    s2x2,3,MPKIND__OFFS_R_FULL,1, warn__none,  s2x8,      0x000, 0},

   {{5, 8, 9, 4, 3, 10, 11, 2, 1, 12, 13, 0},
    s2x2,3,MPKIND__OFFS_L_FULL,0, warn__none,  s2x8,      0x015, 0},
   {{7, 10, 11, 6, 5, 12, 13, 4, 3, 14, 15, 2},
    s2x2,3,MPKIND__OFFS_R_FULL,0, warn__none,  s2x8,      0x015, 0},

   {{9, 8, 6, 7,                       0, 1, 3, 2},
    s1x4,2,MPKIND__OFFS_L_HALF,1, warn__none,  s2x6,      0x000, 0},
   {{11, 10, 8, 9,                     2, 3, 5, 4},
    s1x4,2,MPKIND__OFFS_R_HALF,1, warn__none,  s2x6,      0x000, 0},

   {{4, 3,                             0, 1},
    s1x2,2,MPKIND__OFFS_L_HALF,1, warn__none,  s2x3,      0x000, 0},
   {{5, 4,                             1, 2},
    s1x2,2,MPKIND__OFFS_R_HALF,1, warn__none,  s2x3,      0x000, 0},

   {{11, 10,   17, 18,   8, 7,   0, 1},
    s1x2,4,MPKIND__OFFS_L_HALF,1, warn__none,  s4x5,      0x000, 0},
   {{14, 13,   16, 17,   7, 6,   3, 4},
    s1x2,4,MPKIND__OFFS_R_HALF,1, warn__none,  s4x5,      0x000, 0},

   // **** These are invoked after a call like "hinge".  They raise a warning.
   {{0, 1,   8, 7,   17, 18,   11, 10},
    s1x2,4,MPKIND__OFFS_L_HALF,0, warn__none,  s4x5,      0x000, 0},
   {{14, 13,   16, 17,   7, 6,   3, 4},
    s1x2,4,MPKIND__OFFS_R_HALF,0, warn__none,  s4x5,      0x000, 0},

   {{0, 7, 5, 6,                       1, 2, 4, 3},
    sdmd,2,MPKIND__OFFS_L_HALF,1, warn__none,  spgdmdccw, 0x005, 0},
   {{0, 7, 5, 6,                       1, 2, 4, 3},
    sdmd,2,MPKIND__OFFS_R_HALF,1, warn__none,  spgdmdcw,  0x005, 0},

   {{8, 7, 5, 6,                       0, 1, 3, 2},
    s1x4,2,MPKIND__OFFS_L_ONEQ,1, warn__none,  s2x5,      0x000, 0},
   {{9, 8, 6, 7,                       1, 2, 4, 3},
    s1x4,2,MPKIND__OFFS_R_ONEQ,1, warn__none,  s2x5,      0x000, 0},

   {{10, 9, 7, 8,                      0, 1, 3, 2},
    s1x4,2,MPKIND__OFFS_L_THRQ,1, warn__none,  s2x7,      0x000, 0},
   {{13, 12, 10, 11,                   3, 4, 6, 5},
    s1x4,2,MPKIND__OFFS_R_THRQ,1, warn__none,  s2x7,      0x000, 0},

   {{11, 10, 8, 9,                     0, 1, 3, 2},
    s1x4,2,MPKIND__OFFS_L_FULL,1, warn__none,  s2x8,      0x000, 0},
   {{15, 14, 12, 13,                   4, 5, 7, 6},
    s1x4,2,MPKIND__OFFS_R_FULL,1, warn__none,  s2x8,      0x000, 0},

   {{21, 22, 23, 13,                   11, 1, 9, 10},
    sdmd,2,MPKIND__OFFS_L_FULL,1, warn__none,  s4x6,      0x000, 0},
   {{18, 19, 20, 16,                   8, 4, 6, 7},
    sdmd,2,MPKIND__OFFS_R_FULL,1, warn__none,  s4x6,      0x000, 0},

   {{18, 19, 20, 16,                   8, 4, 6, 7},
    sdmd,2,MPKIND__OFFS_L_FULL,0, warn__none,  s4x6,      0x000, 0},
   {{11, 1, 9, 10,                   21, 22, 23, 13},
    sdmd,2,MPKIND__OFFS_R_FULL,0, warn__none,  s4x6,      0x000, 0},

   {{6, 7, 8, 9,                       0, 1, 2, 3},
    s_trngl4,2,MPKIND__HET_OFFS_L_HALF,0, warn__none,  sbigdmd, (((veryshort) s_trngl4) << 24) | 0x7, 0},
   {{11, 10, 9, 8,                     5, 4, 3, 2},
    s_trngl4,2,MPKIND__HET_OFFS_R_HALF,0, warn__none,  sbigdmd, (((veryshort) s_trngl4) << 24) | 0xD, 0},

   {{11, 7, 2, 4,                      3, 15, 10, 12},
    s_trngl4,2,MPKIND__HET_OFFS_L_HALF,0,warn__check_hokey_4x4,s4x4,(((veryshort) s_trngl4) << 24) | 0xD, 0},
   {{7, 11, 8, 9,                      15, 3, 0, 1},
    s_trngl4,2,MPKIND__HET_OFFS_R_HALF,0,warn__check_hokey_4x4,s4x4,(((veryshort) s_trngl4) << 24) | 0x7, 0},

   {{11, 7, 2, 4, 5, 6,                12, 13, 14, 3, 15, 10},
    s2x3,2,MPKIND__OFFS_L_THIRD,1, warn__none,  s4x4,      0x000, 0},
   {{9, 11, 7, 5, 6, 8,                13, 14, 0, 1, 3, 15},
    s2x3,2,MPKIND__OFFS_R_THIRD,1, warn__none,  s4x4,      0x000, 0},
   {{15, 14, 12, 13,                   4, 5, 7, 6},
    s1x4,2,MPKIND__OFFS_L_THIRD,0, warn__none,  s1p5x8,    0x000, 0},
   {{0, 1, 3, 2,                       11, 10, 8, 9},
    s1x4,2,MPKIND__OFFS_R_THIRD,0, warn__none,  s1p5x8,    0x000, 0},

   {{15, 16, 17, 12, 13, 14,           3, 4, 5, 6, 7, 8},
    s2x3,2,MPKIND__OFFS_L_HALF,0, warn__none,  s3x6,      0x000, 0},
   {{0, 1, 2, 17, 16, 15,              8, 7, 6, 9, 10, 11},
    s2x3,2,MPKIND__OFFS_R_HALF,0, warn__none,  s3x6,      0x000, 0},

   {{18, 19, 20, 15, 16, 17,           3, 4, 5, 6, 7, 8},
    s2x3,2,MPKIND__OFFS_L_FULL,0, warn__none,  s4x6,      0x000, 0},
   {{0, 1, 2, 9, 10, 11,               21, 22, 23, 12, 13, 14},
    s2x3,2,MPKIND__OFFS_R_FULL,0, warn__none,  s4x6,      0x000, 0},

   {{10, 19, 16, 17, 18, 11,           5, 6, 23, 22, 7, 4},
    s2x3,2,MPKIND__OFFS_L_FULL,1, warn__none,  s4x6,      0x005, 0},
   {{1, 10, 19, 18, 11, 0,             6, 23, 12, 13, 22, 7},
    s2x3,2,MPKIND__OFFS_R_FULL,1, warn__none,  s4x6,      0x005, 0},

   {{9, 20, 15, 19, -1, -1, -1, 16, -1, -1, -1, 17, -1, 11, 10, 18,
     -1, -1, -1, 5, -1, 23, 22, 6, 21, 8, 3, 7, -1, -1, -1, 4},
    s4x4,2,MPKIND__OFFS_L_HALF,0, warn__none,  s4x6,      0x000, 0},
   {{-1, 2, 9, 1, 20, 19, 18, 10, -1, -1, -1, 11, -1, -1, -1, 0,
     -1, -1, -1, 23, -1, -1, -1, 12, -1, 14, 21, 13, 8, 7, 6, 22},
    s4x4,2,MPKIND__OFFS_R_HALF,0, warn__none,  s4x6,      0x000, 0},
   {{23, 12, -1, 13, -1, -1, -1, -1, -1, -1, 15, -1, 20, 21, 22, 14,
     -1, -1, 3, -1, 8, 9, 10, 2, 11, 0, -1, 1, -1, -1, -1, -1},
    s4x4,2,MPKIND__OFFS_L_HALF,1, warn__none,  s4x6,      0x000, 0},
   {{21, 14, -1, 15, -1, -1, -1, -1, -1, -1, 17, -1, 18, 19, 20, 16,
     -1, -1, 5, -1, 6, 7, 8, 4, 9, 2, -1, 3, -1, -1, -1, -1},
    s4x4,2,MPKIND__OFFS_R_HALF,1, warn__none,  s4x6,      0x000, 0},

   {{20, 21, 22, 23, 16, 17, 18, 19,   4, 5, 6, 7, 8, 9, 10, 11},
    s2x4,2,MPKIND__OFFS_L_HALF,0, warn__none,  s3x8,      0x000, 0},
   {{0, 1, 2, 3, 23, 22, 21, 20,       11, 10, 9, 8, 12, 13, 14, 15},
    s2x4,2,MPKIND__OFFS_R_HALF,0, warn__none,  s3x8,      0x000, 0},

   {{16, 17, 18, 19, 10, 11, 12, 13,   0, 1, 2, 3, 6, 7, 8, 9},
    s2x4,2,MPKIND__OFFS_L_ONEQ,1, warn__none,  s4x5,      0x000, 0},
   {{15, 16, 17, 18, 11, 12, 13, 14,   1, 2, 3, 4, 5, 6, 7, 8},
    s2x4,2,MPKIND__OFFS_R_ONEQ,1, warn__none,  s4x5,      0x000, 0},

   {{17, 16, 14, 15, 10, 11, 13, 12,   0, 1, 3, 2, 7, 6, 4, 5},
    s1x8,2,MPKIND__OFFS_L_ONEQ,1, warn__none,  s2x10,     0x000, 0},
   {{19, 18, 16, 17, 12, 13, 15, 14,   2, 3, 5, 4, 9, 8, 6, 7},
    s1x8,2,MPKIND__OFFS_R_ONEQ,1, warn__none,  s2x10,     0x000, 0},

   {{10, 9, 8, 7, -1, -1, -1, -1,      -1, -1, -1, -1, 3, 2, 1, 0},
    s2x4,2,MPKIND__OFFS_L_THRQ,1, warn__none,  s2x7,      0x000, 0},
   {{13, 12, 11, 10, -1, -1, -1, -1,   -1, -1, -1, -1, 6, 5, 4, 3},
    s2x4,2,MPKIND__OFFS_R_THRQ,1, warn__none,  s2x7,      0x000, 0},

   {{18, 17, 15, 16, -1, 12, 14, 13,   -1, 0, 2, 1, 6, 5, 3, 4},
    s1x8,2,MPKIND__OFFS_L_THRQ,1, warn__none,  s2x12,     0x000, 0},
   {{-1, 23, 21, 22, 17, 18, 20, 19,   5, 6, 8, 7, -1, 11, 9, 10},
    s1x8,2,MPKIND__OFFS_R_THRQ,1, warn__none,  s2x12,     0x000, 0},

   {{20, 21, 22, 23, 12, 13, 14, 15,   0, 1, 2, 3, 8, 9, 10, 11},
    s2x4,2,MPKIND__OFFS_L_HALF,1, warn__none,  s4x6,      0x000, 0},
   {{18, 19, 20, 21, 14, 15, 16, 17,   2, 3, 4, 5, 6, 7, 8, 9},
    s2x4,2,MPKIND__OFFS_R_HALF,1, warn__none,  s4x6,      0x000, 0},

   {{2, -1, -1, -1, -1, 0, 1, 3,       -1, 4, 5, 7, 6, -1, -1, -1},
    s_qtag,2,MPKIND__OFFS_L_HALF,1, warn__none,  s_ptpd,  0x40005, 0},
   {{-1, 2, 3, 1, 0, -1, -1, -1,       4, -1, -1, -1, -1, 6, 7, 5},
    s_qtag,2,MPKIND__OFFS_R_HALF,1, warn__none,  s_ptpd,  0x40005, 0},

   {{-1, 0, 7, 6, 5, -1, -1, -1,       1, -1, -1, -1, -1, 4, 3, 2},
    s_qtag,2,MPKIND__OFFS_L_HALF,0, warn__none, spgdmdccw,0x000, 0},
   {{-1, 0, 7, 6, 5, -1, -1, -1,       1, -1, -1, -1, -1, 4, 3, 2},
    s_qtag,2,MPKIND__OFFS_R_HALF,0, warn__none, spgdmdcw, 0x000, 0},

   {{19, 18, 16, 17, 12, 13, 15, 14,   0, 1, 3, 2, 7, 6, 4, 5},
    s1x8,2,MPKIND__OFFS_L_HALF,1, warn__none,  s2x12,     0x000, 0},
   {{23, 22, 20, 21, 16, 17, 19, 18,   4, 5, 7, 6, 11, 10, 8, 9},
    s1x8,2,MPKIND__OFFS_R_HALF,1, warn__none,  s2x12,     0x000, 0},

   {{10, 1, 11, 9,                     5, 3, 4, 7},
    s2x2,2,MPKIND__LILZCCW,0,     warn__none,  s3x4,      0x000, 0},
   {{0, 11, 8, 10,                     2, 4, 6, 5},
    s2x2,2,MPKIND__LILZCW,0,      warn__none,  s3x4,      0x000, 0},
   {{15, 1, 16, 14,     17, 3, 8, 12,     7, 5, 6, 10},
    s2x2,3,MPKIND__LILZCCW,0,     warn__none,  s3x6,      0x000, 0},
   {{0, 16, 13, 15,     2, 8, 11, 17,     4, 6, 9, 7},
    s2x2,3,MPKIND__LILZCW,0,      warn__none,  s3x6,      0x000, 0},
   {{0, 16, 13, 15,     17, 3, 8, 12,     4, 6, 9, 7},
    s2x2,3,MPKIND__LILAZCCW,0,    warn__none,  s3x6,      0x000, 0},
   {{15, 1, 16, 14,     2, 8, 11, 17,     7, 5, 6, 10},
    s2x2,3,MPKIND__LILAZCW,0,     warn__none,  s3x6,      0x000, 0},

   {{0, 1, 6, 7, 8, 9, 14, 15,         2, 3, 4, 5, 10, 11, 12, 13},
    s2x4,2,MPKIND__CONCPHAN,0,    warn__none,  s2x8,      0x000, 0},
   {{0, 1, 4, 5, 10, 11, 14, 15,       2, 3, 6, 7, 8, 9, 12, 13},
    s2x4,2,MPKIND__INTLK,0,       warn__none,  s2x8,      0x000, 0},
   {{12, 13, 14, 0, 4, 5, 6, 8,        10, 15, 3, 1, 2, 7, 11, 9},
    s2x4,2,MPKIND__CONCPHAN,1,    warn__none,  s4x4,      0x000, 0},
   {{10, 15, 3, 1, 4, 5, 6, 8,         12, 13, 14, 0, 2, 7, 11, 9},
    s2x4,2,MPKIND__INTLK,1,       warn__none,  s4x4,      0x000, 0},
   {{0, 1, 3, 2, 8, 9, 11, 10,         4, 5, 7, 6, 12, 13, 15, 14},
    s1x8,2,MPKIND__CONCPHAN,0,    warn__none,  s1x16,     0x000, 0},
   {{0, 1, 3, 2, 12, 13, 15, 14,       4, 5, 7, 6, 8, 9, 11, 10},
    s1x8,2,MPKIND__INTLK,0,       warn__none,  s1x16,     0x000, 0},
   {{0, 3, 4, 5, 8, 11, 12, 13,        1, 2, 6, 7, 9, 10, 14, 15},
    s_qtag,2,MPKIND__CONCPHAN,0,  warn__none,  s4dmd,     0x000, 0},
   {{0, 2, 6, 7, 9, 11, 12, 13,        1, 3, 4, 5, 8, 10, 14, 15},
    s_qtag,2,MPKIND__INTLK,0,     warn__none,  s4dmd,     0x000, 0},
   {{12, 0, 13, 11, 4, 8, 5, 3,        14, 1, 15, 10, 6, 9, 7, 2},
    s_ptpd,2,MPKIND__CONCPHAN,0,  warn__none,  s4ptpd,    0x000, 0},
   {{12, 0, 13, 11, 6, 9, 7, 2,        14, 1, 15, 10, 4, 8, 5, 3},
    s_ptpd,2,MPKIND__INTLK,0,     warn__none,  s4ptpd,    0x000, 0},
   {{0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17,
     11, 10, 9, 8, 7, 6, 23, 22, 21, 20, 19, 18},
    s2x6,2,MPKIND__CONCPHAN,1,    warn__none,  s4x6,      0x000, 0},
   {{11, 10, 9, 8, 7, 6, 12, 13, 14, 15, 16, 17,
     0, 1, 2, 3, 4, 5, 23, 22, 21, 20, 19, 18},
    s2x6,2,MPKIND__INTLK,1,       warn__none,  s4x6,      0x000, 0},
   {{3, 4, 6, 9, 10, 0,                2, 5, 7, 8, 11, 1},
    s2x3,2,MPKIND__CONCPHAN,1,    warn__none,  s3x4,      0x005, 0},
   {{2, 5, 7, 9, 10, 0,                3, 4, 6, 8, 11, 1},
    s2x3,2,MPKIND__INTLK,1,       warn__none,  s3x4,      0x005, 0},

   {{0, 5, 6, 11,           1, 4, 7, 10,           2, 3, 8, 9},
    s2x2,3,MPKIND__CONCPHAN,0,    warn__none,  s2x6,      0x000, 0},
   {{5, 6, 11, 0,           4, 7, 10, 1,           3, 8, 9, 2},
    s2x2,3,MPKIND__CONCPHAN,1,    warn__none,  s2x6,      0x015, 0},
   {{0, 7, 8, 15,     1, 6, 9, 14,     2, 5, 10, 13,     3, 4, 11, 12},
    s2x2,4,MPKIND__CONCPHAN,0,    warn__none,  s2x8,      0x000, 0},

   {{10, 13, 14, 1, 2, 5, 6, 9,        12, 15, 3, 0, 4, 7, 11, 8},
    s2x4,2,MPKIND__O_SPOTS,1,     warn__none,  s4x4,      0x000, 0},
   {{10, 13, 14, 1, 2, 5, 6, 9,        12, 15, 3, 0, 4, 7, 11, 8},
    s2x4,2,MPKIND__X_SPOTS,1,     warn__none,  s4x4,      0x000, 0},
   {{10, 13, 14, 1, 2, 5, 6, 9,        12, 15, 3, 0, 4, 7, 11, 8},
    s2x4,2,MPKIND__O_SPOTS,0,     warn__none,  s4x4,      0x000, 0},
   {{10, 13, 14, 1, 2, 5, 6, 9,        12, 15, 3, 0, 4, 7, 11, 8},
    s2x4,2,MPKIND__X_SPOTS,0,     warn__none,  s4x4,      0x000, 0},

   {{0, 1, 2, 3, 8, 17, 11, 12, 13, 14, 15, 16,
     2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 17, 8},
    s3x4,2,MPKIND__OVERLAP,0,     warn__none,  s3x6,      0x000, 0},
   {{3, 8, 21, 9, 14, 15, 16, 20, 17, 18, 11, 19, 0, 1, 2, 10,
     5, 6, 23, 7, 12, 13, 14, 22, 15, 20, 9, 21, 2, 3, 4, 8},
    s4x4,2,MPKIND__OVERLAP,0,     warn__none,  s4x6,      0x000, 0},
   {{0, 1, 5, 11, 7, 8, 9, 10,         1, 2, 3, 4, 6, 7, 11, 5},
    s_qtag,2,MPKIND__OVERLAP,0,   warn__none,  s3dmd,     0x000, 0},
   {{0, 1, 2, 3, -1, -1, 5, 4,       -1, -1, 11, 10, 6, 7, 8, 9},
    s_crosswave,2,MPKIND__OVERLAP,0, warn__none, sdeepxwv, 0x000, 0},

   // This helps with things like triple boxes working together rotary spin.
   // We may have been confused about what "step to a left-handed wave and cast off 3/4"
   // meant for the absent centers, so we may have gone to 1/4 tags with absent centers,
   // rather than 2x4's with absent centers.  This makes it work anyway.
   // The "-2" instead of the usual "-1" makes it give the "Can't do shape-changer" error.
   {{3, 8, -2, -2, 11, 0, -2, -2,         5, 6, -2, -2, 9, 2, -2, -2},
    s_qtag,2,MPKIND__OVERLAP,1,   warn__none,  s2x6,      0x005, 0},

   {{9, 0, 10, 8, 5, 7, 11, 1,         11, 1, 5, 7, 3, 6, 4, 2},
    s_ptpd,2,MPKIND__OVERLAP,0,   warn__none,  s3ptpd,    0x000, 0},
   {{15, 16, 17, 8, 7, 6, 9, 10, 11, 12, 13, 14,
     0, 1, 2, 3, 4, 5, 6, 7, 8, 17, 16, 15},
    s2x6,2,MPKIND__OVERLAP,1,     warn__none,  s3x6,      0x000, 0},
   {{20, 21, 22, 23, 11, 10, 9, 8, 12, 13, 14, 15, 16, 17, 18, 19,
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 23, 22, 21, 20},
    s2x8,2,MPKIND__OVERLAP,1,     warn__none,  s3x8,      0x000, 0},
   {{0, 1, 3, 2, 10, 11, 4, 5,         4, 5, 10, 11, 6, 7, 9, 8},
    s1x8,2,MPKIND__OVERLAP,0,     warn__none,  s1x12,     0x000, 0},
   {{15, 14, 12, 13, 8, 9, 11, 10,     0, 1, 3, 2, 7, 6, 4, 5},
    s1x8,2,MPKIND__OVERLAP,1,     warn__overlap_gone,  s2x8,      0x000, 0},
   {{0, 1, 3, 2, 7, 8, 4, 9,           2, 3, 9, 4, 5, 6, 8, 7},
    s1x8,2,MPKIND__OVERLAP14,0,   warn__none,  s1x10,     0x000, 0},
   {{0, 1, 3, 2, 13, 6, 4, 5,          6, 13, 11, 12, 7, 8, 10, 9},
    s1x8,2,MPKIND__OVERLAP34,0,   warn__none,  s1x14,     0x000, 0},

   {{0, 1, 3, 2, 7, 6, 4, 5,           4, 5, 7, 6, 12, 13, 15, 14,
     15, 14, 12, 13, 8, 9, 11, 10},
    s1x8,3,MPKIND__OVERLAP,0,     warn__none,  s1x16,     0x000, 0},
   {{0, 1, 3, 2, 10, 11, 4, 5,         2, 3, 5, 4, 8, 9, 11, 10,
     4, 5, 10, 11, 6, 7, 9, 8},
    s1x8,3,MPKIND__OVERLAP14,0,    warn__none,  s1x12,    0x000, 0},
   {{19, 18, 16, 17, 12, 13, 15, 14,   20, 21, 23, 22, 8, 9, 11, 10,
     0, 1, 3, 2, 7, 6, 4, 5},
    s1x8,3,MPKIND__OVERLAP,1,     warn__overlap_gone,  s3x8,      0x000, 0},
   {{0, 1, 2, 3, 8, 9, 10, 11,         2, 3, 4, 5, 6, 7, 8, 9},
    s2x4,2,MPKIND__OVERLAP,0,     warn__none,  s2x6,      0x000, 0},
   {{10, 11, 5, 4, 6, 7, 8, 9,         0, 1, 2, 3, 4, 5, 11, 10},
    s2x4,2,MPKIND__OVERLAP,1,     warn__none,  s3x4,      0x000, 0},
   {{1, 11, 8, 9, 10, 0,    2, 5, 7, 8, 11, 1,     3, 4, 6, 7, 5, 2},
    s2x3,3,MPKIND__OVERLAP,1,     warn__none,  s3x4,      0x015, 0},

   {{0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17,
     3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
    s2x6,2,MPKIND__OVERLAP,0,     warn__none,  s2x9,      0x000, 0},

   {{0, 1, 3, -1, 7, 6, 2, -1,         3, 2, 6, -1, 4, 5, 7, -1},
    s3x1dmd,2,MPKIND__OVERLAP,0,     warn__none,  s1x8,     0x000, 0},

   // Overlapped 2x2's.
   {{0, 1, 4, 5,                       1, 2, 3, 4},
    s2x2,2,MPKIND__OVERLAP,0,     warn__none,  s2x3,      0x000, 0},
   {{1, 4, 5, 0,                       2, 3, 4, 1},
    s2x2,2,MPKIND__OVERLAP,1,     warn__none,  s2x3,      0x005, 0},

   {{0, 1, 6, 7,            1, 2, 5, 6,            2, 3, 4, 5},
    s2x2,3,MPKIND__OVERLAP,0,     warn__none,  s2x4,      0x000, 0},
   {{1, 6, 7, 0,            2, 5, 6, 1,            3, 4, 5, 2},
    s2x2,3,MPKIND__OVERLAP,1,     warn__none,  s2x4,      0x015, 0},

   {{0, 1, 8, 9,       1, 2, 7, 8,       2, 3, 6, 7,      3, 4, 5, 6},
    s2x2,4,MPKIND__OVERLAP,0,     warn__none,  s2x5,      0x000, 0},
   {{1, 8, 9, 0,       2, 7, 8, 1,       3, 6, 7, 2,      4, 5, 6, 3},
    s2x2,4,MPKIND__OVERLAP,1,     warn__none,  s2x5,      0x055, 0},

   {{0, 1, 10, 11,  1, 2, 9, 10,  2, 3, 8, 9,  3, 4, 7, 8,  4, 5, 6, 7},
    s2x2,5,MPKIND__OVERLAP,0,     warn__none,  s2x6,      0x000, 0},
   {{1, 10, 11, 0,  2, 9, 10, 1,  3, 8, 9, 2,  4, 7, 8, 3,  5, 6, 7, 4},
    s2x2,5,MPKIND__OVERLAP,1,     warn__none,  s2x6,      0x155, 0},

   {{0, 1, 12, 13,  1, 2, 11, 12,  2, 3, 10, 11,  3, 4, 9, 10,  4, 5, 8, 9,  5, 6, 7, 8},
    s2x2,6,MPKIND__OVERLAP,0,     warn__none,  s2x7,      0x000, 0},
   {{1, 12, 13, 0,  2, 11, 12, 1,  3, 10, 11, 2,  4, 9, 10, 3,  5, 8, 9, 4,  6, 7, 8, 5},
    s2x2,6,MPKIND__OVERLAP,1,     warn__none,  s2x7,      0x555, 0},

   {{0, 1, 14, 15,  1, 2, 13, 14,  2, 3, 12, 13,  3, 4, 11, 12,  4, 5, 10, 11,  5, 6, 9, 10,   6, 7, 8, 9},
    s2x2,7,MPKIND__OVERLAP,0,     warn__none,  s2x8,      0x000, 0},
   {{1, 14, 15, 0,  2, 13, 14, 1,  3, 12, 13, 2,  4, 11, 12, 3,  5, 10, 11, 4,  6, 9, 10, 5,   7, 8, 9, 6},
    s2x2,7,MPKIND__OVERLAP,1,     warn__none,  s2x8,      0x1555, 0},

   {{0, 1, 15, 14, 10, 11, 12, 13,     1, 2, 6, 7, 9, 10, 14, 15,
     2, 3, 4, 5, 8, 9, 7, 6},
    s_qtag,3,MPKIND__OVERLAP,0,   warn__none,  s4dmd,     0x000, 0},
   {{12, 0, 13, 11, 15, 10, 14, 1,     14, 1, 15, 10, 6, 9, 7, 2,
     7, 2, 6, 9, 4, 8, 5, 3},
    s_ptpd,3,MPKIND__OVERLAP,0,   warn__none,  s4ptpd,    0x000, 0},

   // Overlapped 1x4's.
   {{0, 1, 5, 2,                     2, 5, 3, 4},
    s1x4,2,MPKIND__OVERLAP,0,     warn__none,  s1x6,      0x000, 0},
   {{7, 6, 4, 5,                     0, 1, 3, 2},
    s1x4,2,MPKIND__OVERLAP,1,     warn__overlap_gone,  s2x4,      0x000, 0},
   {{0, 1, 2, 3,            3, 2, 7, 6,            6, 7, 4, 5},
    s1x4,3,MPKIND__OVERLAP,0,     warn__none,  s1x8,      0x000, 0},
   {{9, 8, 6, 7,            10, 11, 4, 5,          0, 1, 3, 2},
    s1x4,3,MPKIND__OVERLAP,1,     warn__overlap_gone,  s3x4,      0x000, 0},
   {{0, 1, 3, 2,     2, 3, 9, 4,    4, 9, 7, 8,    8, 7, 5, 6},
    s1x4,4,MPKIND__OVERLAP,0,     warn__none,  s1x10,      0x000, 0},
   {{8, 6, 4, 5,   9, 11, 2, 7,   10, 15, 1, 3,   12, 13, 0, 14},
    s1x4,4,MPKIND__OVERLAP,1,     warn__overlap_gone,  s4x4,      0x000, 0},
   {{0, 1, 3, 2,  2, 3, 5, 4,  4, 5, 10, 11,  11, 10, 8, 9,  9, 8, 6, 7},
    s1x4,5,MPKIND__OVERLAP,0,     warn__none,  s1x12,     0x000, 0},
   {{0, 9, 14, 15, 1, 8, 13, 16, 2, 7, 12, 17, 3, 6, 11, 18, 4, 5, 10, 19},
    s1x4,5,MPKIND__OVERLAP,1,     warn__overlap_gone,  s4x5,      0x155, 0},
   {{0, 1, 2, 3, 1, 3, 6, 2, 3, 2, 7, 6, 2, 6, 5, 7, 6, 7, 4, 5},
    s1x4,5,MPKIND__OVERLAP14,0,     warn__none,  s1x8,    0x000, 0},

   {{7, 0, -1, 6,           -1, 1, -1, 5,          -1, 2, 3, 4},
    sdmd,3,MPKIND__OVERLAP,0,     warn__none,s_spindle,   0x000, 0},
   {{0, 1, 2, 3, 12, 13, 14, 15,       2, 3, 4, 5, 10, 11, 12, 13,
     4, 5, 6, 7, 8, 9, 10, 11},
    s2x4,3,MPKIND__OVERLAP,0,     warn__none,  s2x8,      0x000, 0},
   {{9, 11, 7, 2, 4, 5, 6, 8,          10, 15, 3, 1, 2, 7, 11, 9,
     12, 13, 14, 0, 1, 3, 15, 10},
    s2x4,3,MPKIND__OVERLAP,1,     warn__none,  s4x4,      0x000, 0},
   {{1, 8, 16, 13, 14, 15, 9, 0,       2, 7, 17, 12, 13, 16, 8, 1,
     3, 6, 18, 11, 12, 17, 7, 2,      4, 5, 19, 10, 11, 18, 6, 3},
    s2x4,4,MPKIND__OVERLAP,1,     warn__none,  s4x5,      0x055, 0},
   {{1, 10, 19, 16, 17, 18, 11, 0,       2, 9, 20, 15, 16, 19, 10, 1,
     3, 8, 21, 14, 15, 20, 9, 2,         4, 7, 22, 13, 14, 21, 8, 3,
     5, 6, 23, 12, 13, 22, 7, 4},
    s2x4,5,MPKIND__OVERLAP,1,     warn__none,  s4x6,      0x155, 0},

   {{-1, -1, 5, 7, -1, -1, 0, 6,       -1, -1, 4, 2, -1, -1, 1, 3},
    s_hrglass,2,MPKIND__SPEC_MATRIX_OVERLAP,1,warn__none,  s_qtag,    0x005, 0},
   {{-1, 1, -1, 6, -1, 7, -1, 0,       -1, 3, -1, 4, -1, 5, -1, 2},
    s_galaxy,2,MPKIND__SPEC_MATRIX_OVERLAP,1, warn__none,  s2x4,      0x005, 0},
   {{19, 15, 12, 14, 13, 17, 0, 18,
     5, 9, 11, 16, 15, 19, 1, 6,
     3, 7, 10, 8, 9, 5, 2, 4},
    s_qtag,3,MPKIND__SPEC_MATRIX_OVERLAP,1,   warn__none,  s3oqtg,    0x015, 0},

   {{0, 3,                             1, 2},
    s1x2,2,MPKIND__REMOVED,0,     warn__none,  s1x4,      0x000, 0},

   {{0, 3,                             1, 2},
    s1x2,2,MPKIND__REMOVED,1,     warn__none,  s2x2,      0x005, 0},
   {{0, 2, 5, 7,                       1, 3, 4, 6},
    s2x2,2,MPKIND__REMOVED,0,     warn__none,  s2x4,      0x000, 0},
   {{0, 2, 4,                          1, 5, 3},
    s1x3,2,MPKIND__REMOVED,0,     warn__none,  s1x6,      0x000, 0},
   {{5, 4, 3,                          0, 1, 2},
    s1x3,2,MPKIND__REMOVED,1,     warn__none,  s2x3,      0x000, 0},
   {{0, 2, 4, 7, 9, 11,                1, 3, 5, 6, 8, 10},
    s2x3,2,MPKIND__REMOVED,0,     warn__none,  s2x6,      0x000, 0},
   {{2, 5, 7, 9, 10, 0,                3, 4, 6, 8, 11, 1},
    s2x3,2,MPKIND__REMOVED,1,     warn__none,  s3x4,      0x005, 0},
   {{0, 3, 5, 6,                       1, 2, 4, 7},
    s1x4,2,MPKIND__REMOVED,0,     warn__none,  s1x8,      0x000, 0},
   {{7, 6, 4, 5,                       0, 1, 3, 2},
    s1x4,2,MPKIND__REMOVED,1,     warn__none,  s2x4,      0x000, 0},
   {{0, 2, 6, 4, 9, 11, 15, 13,        1, 3, 7, 5, 8, 10, 14, 12},
    s1x8,2,MPKIND__REMOVED,0,     warn__none,  s1x16,      0x000, 0},
   {{15, 14, 12, 13, 8, 9, 11, 10,     0, 1, 3, 2, 7, 6, 4, 5},
    s1x8,2,MPKIND__REMOVED,1,     warn__none,  s2x8,      0x000, 0},
   {{0, 2, 4, 7, 10, 12, 14, 15, 17,   1, 3, 5, 6, 9, 11, 13, 16, 8},
    s3x3,2,MPKIND__REMOVED,0,     warn__none,  s3x6,      0x000, 0},
   {{9, 8, 7, 5, 6,                    0, 1, 2, 4, 3},
    s1x5,2,MPKIND__REMOVED,1,     warn__none,  s2x5,      0x000, 0},

   {{0, 5,                  1, 4,                  2, 3},
    s1x2,3,MPKIND__TWICE_REMOVED,0,warn__none,  s1x6,     0x000, 0},
   {{0, 5,                  1, 4,                  2, 3},
    s1x2,3,MPKIND__TWICE_REMOVED,1,warn__none,  s2x3,     0x015, 0},
   {{0, 3, 8, 11,           1, 4, 7, 10,           2, 5, 6, 9},
    s2x2,3,MPKIND__TWICE_REMOVED,0,warn__none,  s2x6,     0x000, 0},
   {{0, 3, 8, 11,           1, 4, 7, 10,           2, 5, 6, 9},
    s1x4,3,MPKIND__TWICE_REMOVED,0,warn__none,  s1x12,    0x000, 0},
   {{19, 18, 16, 17, 12, 13, 15, 14,     20, 21, 23, 22, 8, 9, 11, 10,
     0, 1, 3, 2, 7, 6, 4, 5},
    s1x8,3,MPKIND__TWICE_REMOVED,1,warn__none,  s3x8,     0x000, 0},
   {{0, 3, 6, 9, 14, 17, 20, 23,      1, 4, 7, 10, 13, 16, 19, 22,
     2, 5, 8, 11, 12, 15, 18, 21},
    s2x4,3,MPKIND__TWICE_REMOVED,0,warn__none,  s2x12,    0x000, 0},
   {{3, 8, 21, 14, 17, 18, 11, 0,     4, 7, 22, 13, 16, 19, 10, 1,
     5, 6, 23, 12, 15, 20, 9,  2},
    s2x4,3,MPKIND__TWICE_REMOVED,1,warn__none,  s4x6,     0x015, 0},

   {{0, 6,              1, 7,              3, 5,            2, 4},
    s1x2,4,MPKIND__THRICE_REMOVED,0,warn__none, s1x8,     0x000, 0},
   {{0, 7,              1, 6,              2, 5,            3, 4},
    s1x2,4,MPKIND__THRICE_REMOVED,1,warn__none, s2x4,     0x055, 0},
   {{0, 4, 11, 15,      1, 5, 10, 14,      2, 6, 9, 13,     3, 7, 8, 12},
    s2x2,4,MPKIND__THRICE_REMOVED,0,warn__none, s2x8,     0x000, 0},
   {{0, 4, 11, 15,      1, 5, 10, 14,      2, 6, 9, 13,     3, 7, 8, 12},
    s1x4,4,MPKIND__THRICE_REMOVED,0,warn__none, s1x16,    0x000, 0},

   {{6, 0, 3, 5,                       7, 1, 2, 4},
    sdmd,2,MPKIND__REMOVED,0,     warn__none,  s_rigger,  0x000, 0},
   {{0, 3, 5, 6,                       1, 2, 4, 7},
    sdmd,2,MPKIND__REMOVED,1,     warn__none,  s_qtag,    0x005, 0},
   {{0, 2, 4, 7, 9, 11,                1, 3, 5, 6, 8, 10},
    s1x6,2,MPKIND__REMOVED,0,     warn__none,  s1x12,     0x000, 0},
   {{11, 10, 9, 6, 7, 8,               0, 1, 2, 5, 4, 3},
    s1x6,2,MPKIND__REMOVED,1,     warn__none,  s2x6,      0x000, 0},
   {{0, 2, 4, 6, 9, 11, 13, 15,        1, 3, 5, 7, 8, 10, 12, 14},
    s2x4,2,MPKIND__REMOVED,0,     warn__none,  s2x8,      0x000, 0},
   {{10, 15, 3, 1, 4, 5, 6, 8,         12, 13, 14, 0, 2, 7, 11, 9},
    s2x4,2,MPKIND__REMOVED,1,     warn__none,  s4x4,      0x000, 0},
   {{4, 7, 22, 13, 15, 20, 17, 18, 11, 0, 2, 9,
     5, 6, 23, 12, 14, 21, 16, 19, 10, 1, 3, 8},
    s3x4,2,MPKIND__REMOVED,1,     warn__none,  s4x6,      0x005, 0},
   {{7, 22, 15, 20, 18, 11, 2, 9,
     6, 23, 14, 21, 19, 10, 3, 8},
    s_qtag,2,MPKIND__REMOVED,1,   warn__none,  s4x6,      0x005, 0},

   {{0, 1, 3, 2,             4, 5, 6, 7},
    s1x4,2,MPKIND__HET_SPLIT,0, warn__none, slinebox,  (((veryshort) s2x2) << 24) | 0x00, 0},
   {{6, 7, 4, 5,             3, 2, 0, 1},
    s2x2,2,MPKIND__HET_SPLIT,0, warn__none, slinebox,  (((veryshort) s1x4) << 24) | 0x20000, 0xAAAA},

   {{5, 6, 7, 4,             2, 3, 0, 1},
    s2x2,2,MPKIND__HET_SPLIT,0, warn__none, sboxdmd,   (((veryshort) sdmd) << 24) | 0x04, 0x5555},
   {{0, 1, 2, 3,             4, 5, 6, 7},
    sdmd,2,MPKIND__HET_SPLIT,1, warn__none, sboxdmd,   (((veryshort) s2x2) << 24) | 0x00, 0x0000},

   {{0, 1, 3, 2,             5, 6, 7, 4},
    s1x4,2,MPKIND__HET_SPLIT,0, warn__none, splinedmd, (((veryshort) sdmd) << 24) | 0x004, 0},
   {{7, 4, 5, 6,             3, 2, 0, 1},
    sdmd,2,MPKIND__HET_SPLIT,1, warn__none, splinedmd, (((veryshort) s1x4) << 24) | 0x20001, 0xAAAA},

   {{6, 7, 4, 5,             2, 3, 0, 1},
    s1x4,2,MPKIND__HET_SPLIT,1, warn__none, slinedmd, (((veryshort) sdmd) << 24) | 0x30005, 0x5555},
   {{0, 1, 2, 3,             4, 5, 6, 7},
    sdmd,2,MPKIND__HET_SPLIT,1, warn__none, slinedmd, (((veryshort) s1x4) << 24) | 0x10005, 0xFFFF},

   {{0, 2, 4, 6,             7, 1, 3, 5},
    s2x2,2,MPKIND__HET_CO_ONCEREM,0, warn__none, s_spindle, (((veryshort) sdmd) << 24) | 0x000, 0},
   {{7, 1, 3, 5,             0, 2, 4, 6},
    sdmd,2,MPKIND__HET_CO_ONCEREM,0, warn__none, s_spindle, (((veryshort) s2x2) << 24) | 0x000, 0},
   {{1, 3, 5, 7,                       0, 2, 4, 6},
    sdmd,2,MPKIND__HET_CO_ONCEREM,0, warn__none, s1x3dmd,   (((veryshort) s1x4) << 24) | 0x000, 0},
   {{0, 2, 4, 6,                       1, 3, 5, 7},
    s1x4,2,MPKIND__HET_CO_ONCEREM,0, warn__none, s1x3dmd,   (((veryshort) sdmd) << 24) | 0x000, 0},

   {{0, 2, 5, 3,                       1, 4, 6, 7},
    sdmd,2,MPKIND__HET_ONCEREM,0, warn__none, sdbltrngl4,   (((veryshort) s_trngl4) << 24) | 0x004, 0},

   {{1, 7, 5, 3,                       0, 6, 2, 4},
    s2x2,3,MPKIND__HET_TWICEREM,0,   warn__none,  s_ptpd,   (((veryshort) s1x2) << 24) | 0x000, 0},
   {{6, 7, 2, 3,                       5, 4, 0, 1},
    s1x4,3,MPKIND__HET_TWICEREM,1, warn__none,  s_qtag,     (((veryshort) s1x2) << 24) | 0x10015, 0xFFFF},

   {{2, 7, 5, 0,                       6, 3, 1, 4},
    s_trngl4,2,MPKIND__HET_ONCEREM,1, warn__none, s_bone, (((veryshort) s_trngl4) << 24) | 0x107, 0},
   {{0, 2, 7, 5,                       4, 6, 3, 1},
    s_trngl4,2,MPKIND__HET_ONCEREM,1, warn__none, s_ptpd, (((veryshort) s_trngl4) << 24) | 0x10D, 0},

   {{2, 4, 0,                          5, 1, 3},
    s_trngl,2,MPKIND__REMOVED,1,  warn__none,  s_bone6,   0x107, 0},

   {{0, 1, 2, 3, 8, 7},
    s2x3,1,MPKIND__OFFSET_UPWARD_1,0,  warn__none, s3x3,  0x000, 0},
   {{0, 1, 4, 2},
    s1x4,1,MPKIND__OFFSET_UPWARD_1,1,  warn__none, s1x5,  0x000, 0},

   {{0, 1,     2, 3,        4, 9,      8, 7,       6, 5},
    s1x2,5,MPKIND__SPLIT,0,       warn__none,  s1x10,     0x000, 0},
   {{0, 1,     2, 3,        4, 5,      11, 10,     9, 8,     7, 6},
    s1x2,6,MPKIND__SPLIT,0,       warn__none,  s1x12,     0x000, 0},
   {{0, 9,     1, 8,        2, 7,      3, 6,       4, 5},
    s1x2,5,MPKIND__SPLIT,1,       warn__none,  s2x5,      0x155, 0},
   {{0, 11,    1, 10,       2, 9,      3, 8,       4, 7,     5, 6},
    s1x2,6,MPKIND__SPLIT,1,       warn__none,  s2x6,      0x555, 0},

  {{0, 1, 2, 3, 4, 5, 18, 19, 20, 21, 22, 23,
    6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17},
   s2x6,2,MPKIND__SPLIT,0,        warn__none,  s2x12,     0x000, 0},
  {{18, 19, 20, 21, 22, 23, 12, 13, 14, 15, 16, 17,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
   s2x6,2,MPKIND__SPLIT,1,        warn__none,  s4x6,      0x000, 0},

   {{0, 1, 2, 3, 23, 22, 16, 17, 18, 19, 20, 21,
     4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 11, 10},
    s3x4,2,MPKIND__SPLIT,0,       warn__none,  s3x8,      0x000, 0},
   {{2, 9, 20, 15, 16, 19, 17, 18, 11, 0, 1, 10,
     5, 6, 23, 12, 13, 22, 14, 21, 8, 3, 4, 7},
    s3x4,2,MPKIND__SPLIT,1,       warn__none,  s4x6,      0x005, 0},

   {{6, 13, 9, 11, 12, 7, 0, 2,        4, 15, 8, 10, 14, 5, 1, 3},
    s_rigger,2,MPKIND__SPLIT,1,  warn__none, sdeepbigqtg, 0x005, 0},

   {{0, 1, 2, 3, 4, 15, 16, 17, 18, 19,
     5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
    s2x5,2,MPKIND__SPLIT,0,       warn__none,  s2x10,     0x000, 0},
   {{15, 16, 17, 18, 19, 10, 11, 12, 13, 14,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
    s2x5,2,MPKIND__SPLIT,1,       warn__none,  s4x5,      0x000, 0},

   {{0, 11, 18, -1, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   4, 7, 22, 8, 13, 14, 15, 21, 16, 19, 10, 20, 1, 2, 3, 9,
   -1, -1, -1, -1, -1, -1, -1, -1, 12, 23, 6, -1, 5, -1, -1, -1},
    s4x4,3,MPKIND__SPLIT,0,       warn__none,  s4x6,      0x000, 0},

   {{-1, -1, -1, 0, 7, 6, -1, 5,       3, 2, -1, 1, -1, -1, -1, 4},
    s_crosswave,2,MPKIND__SPLIT,0,warn__none,  s_bone,    0x000, 0},

   {{-1, -1, 2, 1, -1, -1, 0, 3,       -1, -1, 4, 7, -1, -1, 6, 5},
    s_hrglass,2,MPKIND__SPLIT,0,  warn__none,  s_ptpd,    0x000, 0},

   {{1, 11, 8, -1, 9, 10, 0, -1,       3, 4, 6, -1, 7, 5, 2, -1},
    s_spindle,2,MPKIND__SPLIT,1,  warn__none,  s3x4, 0x005, 0},
   {{0, 1, 2, 3, 4, 5, 6, 7,           12, 13, 14, 15, 8, 9, 10, 11},
    s_spindle,2,MPKIND__SPLIT,0,  warn__none, sdblspindle,0x000, 0},

   {{0, 1, 2, 3, 4, 5, 6, 7,           12, 13, 14, 15, 8, 9, 10, 11},
    s_bone,2,MPKIND__SPLIT,0,  warn__none,  sdblbone,     0x000, 0},

   {{0, 1, 2, 3, 4, 5, 6, 7,           12, 13, 14, 15, 8, 9, 10, 11},
    s_rigger,2,MPKIND__SPLIT,0,  warn__none,  sdblrig,    0x000, 0},

   {{5, 0, 4, 9, 7, 8,                 3, 1, 2, 11, 6, 10},
    s_short6,2,MPKIND__SPLIT,0,   warn__none, sdeepqtg,   0x000, 0},

   {{0, 1, 2, 3, 4, 5,                 9, 10, 11, 6, 7, 8},
    s_bone6,2,MPKIND__SPLIT,0,    warn__none,  sdblbone6, 0x000, 0},

   {{0, 1, 3, 2, 7, 6, 4, 5,           15, 14, 12, 13, 8, 9, 11, 10},
    s1x8,2,MPKIND__SPLIT,0,       warn__none,  s1x16,     0x000, 0},
   {{15, 14, 12, 13, 8, 9, 11, 10,     0, 1, 3, 2, 7, 6, 4, 5},
    s1x8,2,MPKIND__SPLIT,1,       warn__none,  s2x8,      0x000, 0},

   // For crossfire.
   {{7, 6, 5, 4, 4, 5, 6, 7,           0, 1, 2, 3, 3, 2, 1, 0},
    s1p5x4,2,MPKIND__SPLIT,1,     warn__none,  s2x4,      0x000, 0},
   {{0, 1, 2, 3, 12, 13, 14, 15,       4, 5, 6, 7, 8, 9, 10, 11},
    s1p5x4,2,MPKIND__SPLIT,0,     warn__none,  s2x8,      0x000, 0},

   {{-1, -1, -1, -1, 3, 2, 0, 1,
     4, 5, 7, 6, 12, 13, 15, 14,
     11, 10, 8, 9, -1, -1, -1, -1},
    s1x8,3,MPKIND__SPLIT,0,       warn__none,  s1x16,     0x000, 0},
   {{19, 18, 16, 17, 12, 13, 15, 14,
     20, 21, 23, 22, 8, 9, 11, 10,
     0, 1, 3, 2, 7, 6, 4, 5},
    s1x8,3,MPKIND__SPLIT,1,       warn__none,  s3x8,      0x000, 0},

   {{0, 1, 2, 3, 12, 13, 14, 15,       4, 5, 6, 7, 8, 9, 10, 11},
    s2x4,2,MPKIND__SPLIT,0,       warn__none,  s2x8,      0x000, 0},
   {{9, 11, 7, 2, 4, 5, 6, 8,          12, 13, 14, 0, 1, 3, 15, 10},
    s2x4,2,MPKIND__SPLIT,1,       warn__none,  s4x4,      0x000, 0},
   {{0, 1, 2, 3, 20, 21, 22, 23,
     4, 5, 6, 7, 16, 17, 18, 19,
     8, 9, 10, 11, 12, 13, 14, 15},
    s2x4,3,MPKIND__SPLIT,0,       warn__none,  s2x12,     0x000, 0},
   {{1, 10, 19, 16, 17, 18, 11, 0,
     3, 8, 21, 14, 15, 20, 9, 2,
     5, 6, 23, 12, 13, 22, 7, 4},
    s2x4,3,MPKIND__SPLIT,1,       warn__none,  s4x6,      0x015, 0},

   {{0, 1, 15, 14, 10, 11, 12, 13,     2, 3, 4, 5, 8, 9, 7, 6},
    s_qtag,2,MPKIND__SPLIT,0,     warn__none,  s4dmd,     0x000, 0},
   {{9, 20, 16, 19, 18, 11, 1, 10,     6, 23, 13, 22, 21, 8, 4, 7},
    s_qtag,2,MPKIND__SPLIT,1,     warn__none,  s4x6,      0x005, 0},

   {{12, 0, 13, 11, 15, 10, 14, 1,     7, 2, 6, 9, 4, 8, 5, 3},
    s_ptpd,2,MPKIND__SPLIT,0,     warn__none,  s4ptpd,    0x000, 0},
   {{18, 19, 20, 16, 23, 13, 21, 22,   11, 1, 9, 10, 6, 7, 8, 4},
    s_ptpd,2,MPKIND__SPLIT,1,     warn__none,  s4x6,      0x000, 0},

   {{-1, -1, 0, 7, 6, 5,               3, 2, 1, -1, -1, 4},
    s_1x2dmd,2,MPKIND__SPLIT,0,   warn__none,  s_bone,    0x000, 0},
   {{11, 10, 9, 6, 7, 8,               0, 1, 2, 5, 4, 3},
    s_1x2dmd,2,MPKIND__SPLIT,1,   warn__none,  sbigdmd,   0x000, 0},

   {{0, 1, 2, 9, 10, 11,               3, 4, 5, 6, 7, 8},
    s2x3,2,MPKIND__SPLIT,0,       warn__none,  s2x6,      0x000, 0},
   {{1, 11, 8, 9, 10, 0,               3, 4, 6, 7, 5, 2},
    s2x3,2,MPKIND__SPLIT,1,       warn__none,  s3x4,      0x005, 0},
   {{0, 1, 2, 15, 16, 17,   3, 4, 5, 12, 13, 14,   6, 7, 8, 9, 10, 11},
    s2x3,3,MPKIND__SPLIT,0,       warn__none,  s2x9,      0x000, 0},
   {{1, 16, 13, 14, 15, 0,  3, 8, 11, 12, 17, 2,   5, 6, 9, 10, 7, 4},
    s2x3,3,MPKIND__SPLIT,1,       warn__none,  s3x6,      0x015, 0},

   {{0, 1, 2, 5, 4, 3,                 11, 10, 9, 6, 7, 8},
    s1x6,2,MPKIND__SPLIT,0,       warn__none,  s1x12,     0x000, 0},
   {{11, 10, 9, 6, 7, 8,               0, 1, 2, 5, 4, 3},
    s1x6,2,MPKIND__SPLIT,1,       warn__none,  s2x6,      0x000, 0},
   {{14, 13, 12, 9, 10, 11,            15, 16, 17, 6, 7, 8,
     0, 1, 2, 5, 4, 3},
    s1x6,3,MPKIND__SPLIT,1,       warn__none,  s3x6,      0x000, 0},
   {{17, 16, 15, 12, 13, 14,           18, 19, 20, 23, 22, 21,
     0, 1, 2, 5, 4, 3,                 11, 10, 9, 6, 7, 8},
    s1x6,4,MPKIND__SPLIT,1,       warn__none,  s4x6,      0x000, 0},

   {{6, 0, 7, 5,                       3, 1, 2, 4},
    s_star,2,MPKIND__SPLIT,0,     warn__none,  s_2stars,  0x000, 0},

   {{0, 1, 2, 3,                       6, 7, 4, 5},
    sdmd,2,MPKIND__SPLIT,0,       warn__none,  s_ptpd,    0x000, 0},
   {{0, 7, 5, 6,                       1, 2, 4, 3},
    sdmd,2,MPKIND__SPLIT,1,       warn__none,  s_qtag,    0x005, 0},
   {{9, 0, 10, 8,           11, 1, 5, 7,           4, 2, 3, 6},
    sdmd,3,MPKIND__SPLIT,0,       warn__none,  s3ptpd,    0x000, 0},
   {{0, 10, 8, 9,           1, 5, 7, 11,           2, 3, 6, 4},
    sdmd,3,MPKIND__SPLIT,1,       warn__none,  s3dmd,     0x015, 0},
   {{12, 0, 13, 11,     14, 1, 15, 10,     5, 3, 4, 8,     7, 2, 6, 9},
    sdmd,4,MPKIND__SPLIT,0,       warn__none,  s4ptpd,    0x000, 0},
   {{0, 13, 11, 12,     1, 15, 10, 14,     3, 4, 8, 5,     2, 6, 9, 7},
    sdmd,4,MPKIND__SPLIT,1,       warn__none,  s4dmd,     0x055, 0},

   {{0, 1, 2, 3,                       6, 7, 4, 5},
    s1x4,2,MPKIND__SPLIT,0,       warn__none,  s1x8,      0x000, 0},
   {{7, 6, 4, 5,                       0, 1, 3, 2},
    s1x4,2,MPKIND__SPLIT,1,       warn__none,  s2x4,      0x000, 0},
   {{0, 1, 3, 2,            4, 5, 10, 11,          9, 8, 6, 7},
    s1x4,3,MPKIND__SPLIT,0,       warn__none,  s1x12,     0x000, 0},
   {{9, 8, 6, 7,            10, 11, 4, 5,          0, 1, 3, 2},
    s1x4,3,MPKIND__SPLIT,1,       warn__none,  s3x4,      0x000, 0},
   {{0, 1, 3, 2,           4, 5, 7, 6,          11, 10, 8, 9,     15, 14, 12, 13},
    s1x4,4,MPKIND__SPLIT,0,       warn__none,  s1x16,     0x000, 0},
   {{8, 6, 4, 5,           9, 11, 2, 7,         12, 13, 0, 14,    10, 15, 1, 3},
    s1x4,4,MPKIND__SPLIT,1,       warn__none,  s4x4,      0x000, 0},
   {{0, 9, 14, 15,          1, 8, 13, 16,          2, 7, 12, 17,
     3, 6, 11, 18,          4, 5, 10, 19},
    s1x4,5,MPKIND__SPLIT,1,       warn__none,  s4x5,      0x155, 0},
   {{0, 11, 17, 18,         1, 10, 16, 19,         2, 9, 15, 20,
     3, 8, 14, 21,          4, 7, 13, 22,          5, 6, 12, 23},
    s1x4,6,MPKIND__SPLIT,1,       warn__none,  s4x6,      0x555, 0},

   {{0, 1, 6, 7,                       2, 3, 4, 5},
    s2x2,2,MPKIND__SPLIT,0,       warn__none,  s2x4,      0x000, 0},
   {{1, 6, 7, 0,                       3, 4, 5, 2},
    s2x2,2,MPKIND__SPLIT,1,       warn__none,  s2x4,      0x005, 0},

   {{0, 1, 2, 17, 12, 13, 14, 15, 16,     3, 4, 5, 6, 9, 10, 11, 8, 7},
    s3x3,2,MPKIND__SPLIT,0,       warn__none,  s3x6,      0x000, 0},
   {{2, 17, 12, 13, 14, 15, 0, 1, 16,     5, 6, 9, 10, 11, 8, 3, 4, 7},
    s3x3,2,MPKIND__SPLIT,1,       warn__none,  s3x6,      0x005, 0},

   {{0, 1, 10, 11,          2, 3, 8, 9,            4, 5, 6, 7},
    s2x2,3,MPKIND__SPLIT,0,       warn__none,  s2x6,      0x000, 0},
   {{1, 10, 11, 0,          3, 8, 9, 2,            5, 6, 7, 4},
    s2x2,3,MPKIND__SPLIT,1,       warn__none,  s2x6,      0x015, 0},

   {{0, 1, 14, 15,         2, 3, 12, 13,        6, 7, 8, 9,       4, 5, 10, 11},
    s2x2,4,MPKIND__SPLIT,0,       warn__none,  s2x8,      0x000, 0},
   {{1, 14, 15, 0,         3, 12, 13, 2,        7, 8, 9, 6,       5, 10, 11, 4},
    s2x2,4,MPKIND__SPLIT,1,       warn__none,  s2x8,      0x055, 0},

   {{0, 1, 18, 19,         2, 3, 16, 17,        4, 5, 14, 15,
     6, 7, 12, 13,         8, 9, 10, 11},
    s2x2,5,MPKIND__SPLIT,0,       warn__none,  s2x10,     0x000, 0},
   {{1, 18, 19, 0,         3, 16, 17, 2,        5, 14, 15, 4,
     7, 12, 13, 6,         9, 10, 11, 8},
    s2x2,5,MPKIND__SPLIT,1,       warn__none,  s2x10,     0x155, 0},

   {{0, 1, 22, 23,         2, 3, 20, 21,        4, 5, 18, 19,
     6, 7, 16, 17,         8, 9, 14, 15,        10, 11, 12, 13},
    s2x2,6,MPKIND__SPLIT,0,       warn__none,  s2x12,     0x000, 0},
   {{1, 22, 23, 0,         3, 20, 21, 2,        5, 18, 19, 4,
     7, 16, 17, 6,         9, 14, 15, 8,        11, 12, 13, 10},
    s2x2,6,MPKIND__SPLIT,1,       warn__none,  s2x12,     0x555, 0},

   {{9, 11, 6, 8,       7, 2, 4, 5,      12, 13, 15, 10,     14, 0, 1, 3},
    s2x2,4,MPKIND__SPLIT_OTHERWAY_TOO,0, warn__none, s4x4,0x000, 0},

   // Don't seem to need this.  But I believe it's corrrect.
   //{{9, 11, 6, 8,       7, 2, 4, 5,      12, 13, 15, 10,     14, 0, 1, 3},
   // s2x2,4,MPKIND__SPLIT_WITH_45_ROTATION_OTHERWAY_TOO,0, 0, s4x4, 0x000, 0},

   {{18, 19, 16, 17,       20, 21, 14, 15,      22, 23, 12, 13,
     0, 1, 10, 11,         2, 3, 8, 9,          4, 5, 6, 7},
    s2x2,6,MPKIND__SPLIT_OTHERWAY_TOO,0, warn__none, s4x6,0x000, 0},
   {{1, 10, 11, 0,         3, 8, 9, 2,          5, 6, 7, 4,
     19, 16, 17, 18,       21, 14, 15, 20,      23, 12, 13, 22},
    s2x2,6,MPKIND__SPLIT_OTHERWAY_TOO,1, warn__none, s4x6,0x555, 0},

   {{23, 22, 20, 21,       19, 18, 16, 17,      15, 14, 12, 13,
     0, 1, 3, 2,           4, 5, 7, 6,          8, 9, 11, 10},
    s1x4,6,MPKIND__SPLIT_OTHERWAY_TOO,0, warn__none,s2x12,0x000, 0},
   {{15, 14, 12, 13,       11, 10, 8, 9,        4, 5, 7, 6,
     19, 18, 16, 17,       20, 21, 23, 22,      0, 1, 3, 2},
    s1x4,6,MPKIND__SPLIT_OTHERWAY_TOO,1, warn__none, s3x8,0x000, 0},

   {{0, 1, 2,                          5, 4, 3},
    s1x3,2,MPKIND__SPLIT,0,       warn__none,  s1x6,      0x000, 0},
   {{5, 4, 3,                          0, 1, 2},
    s1x3,2,MPKIND__SPLIT,1,       warn__none,  s2x3,      0x000, 0},
   {{0, 1, 2,              3, 4, 5,             8, 7, 6,          11, 10, 9},
    s1x3,4,MPKIND__SPLIT,0,       warn__none,  s1x12,     0x000, 0},
   {{0, 10, 9,             1, 11, 8,            3, 4, 6,          2, 5, 7},
    s1x3,4,MPKIND__SPLIT,1,       warn__none,  s3x4,      0x055, 0},
   {{11, 10, 9,            8, 7, 6,             0, 1, 2,          3, 4, 5},
    s1x3,4,MPKIND__SPLIT_OTHERWAY_TOO,0, warn__none,  s2x6,     0x000, 0},
   {{8, 7, 6,            3, 4, 5,          11, 10, 9,             0, 1, 2},
    s1x3,4,MPKIND__SPLIT_OTHERWAY_TOO,1, warn__none,  s2x6,     0x000, 0},

   {{0, 1,                             3, 2},
    s1x2,2,MPKIND__SPLIT,0,       warn__none,  s1x4,      0x000, 0},
   {{3, 2,                             0, 1},
    s1x2,2,MPKIND__SPLIT,1,       warn__none,  s2x2,      0x000, 0},
   {{0, 1,                  2, 5,                  4, 3},
    s1x2,3,MPKIND__SPLIT,0,       warn__none,  s1x6,      0x000, 0},
   {{0, 5,                  1, 4,                  2, 3},
    s1x2,3,MPKIND__SPLIT,1,       warn__none,  s2x3,      0x015, 0},
   {{0, 1,                  3, 2,                  5, 4,                  6, 7},
    s1x2,4,MPKIND__SPLIT,0,       warn__none,  s1x8,      0x000, 0},
   {{0, 7,                  1, 6,                  3, 4,                  2, 5},
    s1x2,4,MPKIND__SPLIT,1,       warn__none,  s2x4,      0x055, 0},

   {{7, 6,                  5, 4,                  0, 1,                  2, 3},
    s1x2,4,MPKIND__SPLIT_OTHERWAY_TOO,0, warn__none, s2x4, 0x000, 0},
   {{5, 4,                  2, 3,                  7, 6,                  0, 1},
    s1x2,4,MPKIND__SPLIT_OTHERWAY_TOO,1, warn__none, s2x4, 0x000, 0},
   {{3,                     2,                     0,                     1},
    s1x1,4,MPKIND__SPLIT_WITH_45_ROTATION_OTHERWAY_TOO,0, warn__none, sdmd, 0x000, 0},
   {{3,                     2,                     0,                     1},
    s1x1,4,MPKIND__SPLIT_WITH_45_ROTATION_OTHERWAY_TOO,1, warn__none, sdmd, 0x055, 0},
   {{3, 2,                  0, 1,                  5, 4,                  6, 7},
    s1x2,4,MPKIND__SPLIT_WITH_45_ROTATION_OTHERWAY_TOO,1, warn__none, s_qtag, 0x000, 0},
   {{5, 4,                  3, 2,                  6, 7,                  0, 1},
    s1x2,4,MPKIND__SPLIT_WITH_45_ROTATION_OTHERWAY_TOO,0, warn__none, s_rigger, 0x000, 0},

   {{5, 4,                  1, 2},
    s1x2,2,MPKIND__SPLIT_WITH_45_ROTATION,0, warn__none,  s2x3, 0x000, 0},
   {{4, 3,                  0, 1},
    s1x2,2,MPKIND__SPLIT_WITH_45_ROTATION,1, warn__none,  s2x3, 0x000, 0},
   {{5, 4,                  1, 2},
    s1x2,2,MPKIND__SPLIT_WITH_M45_ROTATION,0, warn__none, s2x3, 0x000, 0},

   {{9, 11, 6, 8,           14, 0, 1, 3},
    s2x2,2,MPKIND__SPLIT_WITH_45_ROTATION,0, warn__none,  s4x4, 0x000, 0},

   {{14, 13,                16, 17,                3, 4,                  7, 6},
    s1x2,4,MPKIND__SPLIT_WITH_45_ROTATION,0, warn__none, s4x5, 0x000, 0},
   {{11, 10,                17, 18,                0, 1,                  8, 7},
    s1x2,4,MPKIND__SPLIT_WITH_45_ROTATION,1, warn__none, s4x5, 0x000, 0},

   {{11, 10, 8, 9,                2, 3, 5, 4},
    s1x4,2,MPKIND__SPLIT_WITH_45_ROTATION,0, warn__none, s2x6, 0x000, 0},
   {{11, 10, 8, 9,                2, 3, 5, 4},
    s1x4,2,MPKIND__SPLIT_WITH_M45_ROTATION,0, warn__none, s2x6, 0x000, 0},
   {{9, 8, 6, 7,                  0, 1, 3, 2},
    s1x4,2,MPKIND__SPLIT_WITH_45_ROTATION,1, warn__none, s2x6, 0x000, 0},


   {{0, 7, 5, 6,                  1, 2, 4, 3},
    sdmd,2,MPKIND__SPLIT_WITH_45_ROTATION,0, warn__none, spgdmdcw, 0x005, 0},
   {{1, 2, 4, 3,                  0, 7, 5, 6},
    sdmd,2,MPKIND__SPLIT_WITH_M45_ROTATION,0, warn__none, spgdmdcw, 0x005, 0},
   {{0, 7, 5, 6,                  1, 2, 4, 3},
    sdmd,2,MPKIND__SPLIT_WITH_45_ROTATION,1, warn__none, spgdmdccw, 0x005, 0},


   {{7, 6, 5, 4, 0, 1, 2, 3},
    s1x1,8,MPKIND__SPLIT_OTHERWAY_TOO,0, warn__none, s2x4, 0x000, 0},
   {{7, 6, 5, 4, 0, 1, 2, 3},
    s1x1,8,MPKIND__SPLIT_OTHERWAY_TOO,1, warn__none, s2x4, 0x5555, 0},
   {{0, 1, 2, 3},
    s1x1,4,MPKIND__SPLIT,0, warn__none, s1x4, 0x000, 0},
   {{3, 2, 0, 1},
    s1x1,4,MPKIND__SPLIT_OTHERWAY_TOO,0, warn__none, s2x2, 0x000, 0},
   {{12, 15, 6, 5, 13, 14, 7, 4},
    s1x1,8,MPKIND__SPLIT_WITH_45_ROTATION_OTHERWAY_TOO,0, warn__none, s_c1phan, 0x0000, 0},

   {{0, 1, 2, 3, 4, 5, 6, 7},
    s1x1,8,MPKIND__QTAG8,0, warn__none, s_qtag, 0x0000, 0},
   {{0, 1, 2, 3, 4, 5, 6, 7},
    s1x1,8,MPKIND__QTAG8,1, warn__none, s_qtag, 0x5555, 0},
   {{10, 13, 0, 3, 2, 5, 8, 11},
    s1x1,8,MPKIND__QTAG8_WITH_45_ROTATION,0, warn__none, s4x4, 0x0000, 0},

   {{0,        1},
    s1x1,2,MPKIND__SPLIT,0,       warn__none,  s1x2,      0x000, 0},
   {{0,        1},
    s1x1,2,MPKIND__SPLIT,1,       warn__none,  s1x2,      0x005, 0},
   {{0,        1,        2},
    s1x1,3,MPKIND__SPLIT,0,       warn__none,  s1x3,      0x000, 0},
   {{0,        1,        2},
    s1x1,3,MPKIND__SPLIT,1,       warn__none,  s1x3,      0x015, 0},

   {{0, 1,                  2, 5,                  4, 3},
    s1x2,3,MPKIND__TRIPLETRADEINWINGEDSTAR6,0, warn__none,  s_wingedstar6,      0x004, 0},

   {{0}, nothing}};


// This table has the maps encoded by the "specmapkind" enumeration.


// Nonzero item in map3[1] for arity 1 map means do not reassemble.
// For maps with arity 1, a nonzero item in map3[0] means that the "offset goes away"
//    warning should be given in certain cases.
// For maps with arity 3, a value of 3 after the last map means that the
//    "overlap goes away" warning should be given in certain cases.

// Each setup is rotated clockwise before being concatenated ---|
// Setups are stacked on top                                    |
//       of each other --------------|                          |
//                                   V                          V
// maps       innerk,arity,map_kind,vert  warncode     outerk  rot perpersonrot */

const map::map_thing map::spec_map_table[] = {
   {{9, 13, 7, 0, 1, 5, 15, 8,         12, 11, 14, 2, 4, 3, 6, 10},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s4x4,      0x000, 0, spcmap_stairst},
   {{10, 15, 14, 0, 2, 7, 6, 8,        12, 13, 3, 1, 4, 5, 11, 9},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s4x4,      0x000, 0, spcmap_ladder},
   {{10, 13, 14, 1, 2, 5, 6, 9,        12, 15, 3, 0, 4, 7, 11, 8},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s4x4,      0x000, 0, spcmap_but_o},
   {{12, 14, 7, 9,         13, 0, 2, 11,      15, 1, 4, 6,      10, 3, 5, 8},
    s2x2,4,MPKIND__NONE,0,        warn__none,  s4x4,      0x000, 0, spcmap_blocks},
   {{1, 3, 5, 7,                       0, 2, 4, 6},
    s2x2,2,MPKIND__NONE,0,        warn__none,  s2x4,      0x000, 0, spcmap_2x4_int_pgram},
   {{1, 2, 4, 7,                       0, 3, 5, 6},
    s2x2,2,MPKIND__NONE,0,        warn__none,  s2x4,      0x000, 0, spcmap_2x4_trapezoid},
   {{1, 2, -1, 0},
    s2x2,1,MPKIND__NONE,0,        warn__none,  s_trngl, 0x40000, 0, spcmap_trngl_box1},
   {{1, 2, 0, -1},
    s2x2,1,MPKIND__NONE,0,        warn__none,  s_trngl, 0x40000, 0, spcmap_trngl_box2},
   {{2, 3, 4, 5, 10, 11, 12, 13},
    s2x4,1,MPKIND__NONE,0,        warn__none,  s2x8,      0x000, 0, spcmap_inner_box},
   {{0, 10, 2, 8, 12, 22, 14, 20,        11, 1, 9, 3, 23, 13, 21, 15},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s4x6,      0x000, 0, spcmap_lh_stag},
   {{2, 8, 4, 6, 14, 20, 16, 18,          9, 3, 7, 5, 21, 15, 19, 17},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s4x6,      0x000, 0, spcmap_rh_stag},
   {{0, 8, 2, 6, 10, 18, 12, 16,          9, 1, 7, 3, 19, 11, 17, 13},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s4x5,      0x000, 0, spcmap_lh_stag_1_4},
   {{1, 7, 3, 5, 11, 17, 13, 15,          8, 2, 6, 4, 18, 12, 16, 14},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s4x5,      0x000, 0, spcmap_rh_stag_1_4},
   {{11, 1, 2, 8, 23, 13, 14, 20,         0, 10, 9, 3, 12, 22, 21, 15},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s4x6,      0x000, 0, spcmap_lh_ox},
   {{9, 3, 4, 6, 21, 15, 16, 18,          2, 8, 7, 5, 14, 20, 19, 17},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s4x6,      0x000, 0, spcmap_rh_ox},
   {{0, -1, 1, -1, 2, -1, 3, -1, 6, -1, 7, -1, 8, -1, 9, -1},
    s_c1phan,1,MPKIND__OFFS_L_HALF,1, warn__none, sbigdmd,0x000, 0, spcmap_lh_c1phana},
   {{-1, 2, -1, 3, -1, 5, -1, 4, -1, 8, -1, 9, -1, 11, -1, 10},
    s_c1phan,1,MPKIND__OFFS_R_HALF,1, warn__none, sbigdmd,0x000, 0, spcmap_rh_c1phana},
   {{-1, 12, -1, 10, -1, 3, -1, 15, -1, 4, -1, 2, -1, 11, -1, 7},
    s_c1phan,1,MPKIND__OFFS_L_HALF,1, warn__check_hokey_4x4, s4x4, 0x000, 0, spcmap_lh_c1phanb},
   {{15, -1, 3, -1, 0, -1, 1, -1, 7, -1, 11, -1, 8, -1, 9, -1},
    s_c1phan,1,MPKIND__OFFS_R_HALF,1, warn__check_hokey_4x4, s4x4, 0x000, 0, spcmap_rh_c1phanb},
   {{0, 1, 3, 2,    8, 7, 5, 6},
    s1x4,2,MPKIND__NONE,0,        warn__none,  s1x10,     0x000, 0, spcmap_d1x10},
   {{15, 16, 17, 12, 13, 14,   2, 3, 4, 5, 6, 7},
    s2x3,2,MPKIND__NONE,0,        warn__none,  s4x5,      0x000, 0, spcmap_tgl451},
   {{17, 18, 19, 10, 11, 12,   0, 1, 2, 7, 8, 9},
    s2x3,2,MPKIND__NONE,0,        warn__none,  s4x5,      0x000, 0, spcmap_tgl452},
   {{0,   1,   2,   3},
    s1x1,4,MPKIND__NONE,0,        warn__none,  sdmd,      0x000, 0, spcmap_dmd_1x1},
   {{0,   1,   2,   3},
    s1x1,4,MPKIND__NONE,0,        warn__none,  s_star,    0x000, 0, spcmap_star_1x1},
   {{7, 3,   0, 0},
    s1x2,1,MPKIND__NONE,0,        warn__none,  s_qtag,    0x000, 0, spcmap_qtag_f0},
   {{1, 3,   7, 5},
    s1x2,2,MPKIND__NONE,1,        warn__none,  s_qtag,    0x005, 0, spcmap_qtag_f1},
   {{3, 4,   0, 7},
    s1x2,2,MPKIND__NONE,1,        warn__none,  s_qtag,    0x005, 0, spcmap_qtag_f2},
   {{5, 7, 21, 15, 17, 19, 9, 3},
    s2x4,1,MPKIND__NONE,1,        warn__none,  s4x6,      0x001, 0, spcmap_diag2a},
   {{2, 8, 22, 12, 14, 20, 10, 0},
    s2x4,1,MPKIND__NONE,1,        warn__none,  s4x6,      0x001, 0, spcmap_diag2b},
   {{13, 3, 2, 5, 11, 10},
    s2x3,1,MPKIND__NONE,0,        warn__none,  s4x4,      0x000, 0, spcmap_diag23a},
   {{13, 3, 2, 5, 11, 10},
    s2x3,1,MPKIND__NONE,1,        warn__none,  s4x4,      0x001, 0, spcmap_diag23b},
   {{9, 15, 14, 1, 7, 6},
    s2x3,1,MPKIND__NONE,0,        warn__none,  s4x4,      0x000, 0, spcmap_diag23c},
   {{1, 7, 6, 9, 15, 14},
    s2x3,1,MPKIND__NONE,1,        warn__none,  s4x4,      0x001, 0, spcmap_diag23d},
   {{-1, -1, -1, 13, -1, 15, 0, 14, -1, -1, -1, 1, -1, 3, 12, 2,
     -1, -1, -1,  9, -1, 11, 4, 10, -1, -1, -1, 5, -1, 7,  8, 6},
    s4x4,2,MPKIND__SPLIT,1,       warn__none,  sfat2x8,   0x005, 0, spcmap_f2x8_4x4},
   {{-1, 3, 12, 2, -1, -1, -1, 13, -1, 15, 0, 14, -1, -1, -1, 1,
     -1, 7,  8, 6, -1, -1, -1,  9, -1, 11, 4, 10, -1, -1, -1, 5},
    s4x4,2,MPKIND__SPLIT,0,       warn__none,  sfat2x8,   0x000, 0, spcmap_f2x8_4x4h},
   {{6, 8, -1, 9, -1, -1, -1, -1, -1, -1, 12, -1, 13, 15, 11, 10,
     -1, -1, 4, -1, 5, 7, 3, 2, 14, 0, -1, 1, -1, -1, -1, -1},
    s4x4,2,MPKIND__SPLIT,1,       warn__none,  swide4x4,  0x005, 0, spcmap_w4x4_4x4},
   {{13, 15, 11, 10, 6, 8, -1, 9, -1, -1, -1, -1, -1, -1, 12, -1,
     -1, -1, -1, -1, -1, -1, 4, -1, 5, 7, 3, 2, 14, 0, -1, 1},
    s4x4,2,MPKIND__SPLIT,0,       warn__none,  swide4x4,  0x000, 0, spcmap_w4x4_4x4h},
   {{15, 14, 13, 12, 11, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, 7, 6, 5, 4, 3, 2, 1, 0},
    s2x8,2,MPKIND__SPLIT,1,       warn__none,  sfat2x8,   0x000, 0, spcmap_f2x8_2x8},
   {{-1, -1, -1, -1, 0, 1, 2, 3, 12, 13, 14, 15, -1, -1, -1, -1,
     4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, 8, 9, 10, 11},
    s2x8,2,MPKIND__SPLIT,0,       warn__none,  sfat2x8,   0x000, 0, spcmap_f2x8_2x8h},
   {{-1, -1, 9, 11, 7, 2, -1, -1, -1, -1,  4, 5, 6, 8, -1, -1,
     -1, -1, 12, 13, 14, 0, -1, -1, -1, -1, 1, 3, 15, 10, -1, -1},
    s2x8,2,MPKIND__SPLIT,1,       warn__none,  swide4x4,  0x000, 0, spcmap_w4x4_2x8},
   {{17, 18, 19, -1, -1, 10, 11, 12,   -1, 0, 1, 2, 7, 8, 9, -1},
    s2x4,2,MPKIND__OFFS_L_THRQ,1, warn__none,  s4x5,      0x000, 0, spcmap_emergency1},
   {{-1, 15, 16, 17, 12, 13, 14, -1,   2, 3, 4, -1, -1, 5, 6, 7},
    s2x4,2,MPKIND__OFFS_R_THRQ,1, warn__none,  s4x5,      0x000, 0, spcmap_emergency2},
   {{-1, 0, 6, 5, -1, 3, -1, 7, 2, 1, -1, 4},
    sdmd,3,MPKIND__SPLIT, 0, warn__none,  s_dhrglass,     0x000, 0, spcmap_fix_triple_turnstyle},
   {{0, 3,                             1, 2},
    s1x2,2,MPKIND__SPLIT,1,       warn__none,  s2x2,      0x005, 0, spcmap_2x2v},
   {{6, 1, 4, 3,                       0, 7, 2, 5},
    sdmd,2,MPKIND__NONE,0,        warn__none,  s_ptpd,    0x000, 0, spcmap_ptp_magic},
   {{0, 1, 6, 3,                       2, 7, 4, 5},
    sdmd,2,MPKIND__NONE,0,        warn__none,  s_ptpd,    0x000, 0, spcmap_ptp_intlk},
   {{2, 1, 4, 3,                       0, 7, 6, 5},
    sdmd,2,MPKIND__NONE,0,        warn__none,  s_ptpd,    0x000, 0, spcmap_ptp_magic_intlk},
   {{12, 13, 0, 14,                    8, 6, 4, 5},
    s1x4,2,MPKIND__NONE,1,        warn__none,  s4x4,      0x000, 0, spcmap_4x4_ns},
   {{0, 1, 4, 2,                       12, 10, 8, 9},
    s1x4,2,MPKIND__NONE,0,        warn__none,  s4x4,      0x005, 0, spcmap_4x4_ew},
   {{13, 15,    3, 1,    7, 5,    9, 11},
    s1x2,4,MPKIND__NONE,1,        warn__none, s4x4,       0x011, 0, spcmap_4x4_spec0},
   {{10, 15,    14, 3,    7, 2,    11, 6},
    s1x2,4,MPKIND__NONE,0,        warn__none, s4x4,       0x044, 0, spcmap_4x4_spec1},
   {{0, 1,    5, 4,    9, 8,    12, 13},
    s1x2,4,MPKIND__NONE,1,        warn__none, s4x4,       0x011, 0, spcmap_4x4_spec2},
   {{8, 6,    12, 10,    14, 0,    2, 4},
    s1x2,4,MPKIND__NONE,0,        warn__none, s4x4,       0x044, 0, spcmap_4x4_spec3},
   {{14, 3,    5, 4,    11, 6,    12, 13},
    s1x2,4,MPKIND__NONE,1,        warn__none, s4x4,       0x011, 0, spcmap_4x4_spec4},
   {{8, 6,    13, 15,    14, 0,    7, 5},
    s1x2,4,MPKIND__NONE,0,        warn__none, s4x4,       0x044, 0, spcmap_4x4_spec5},
   {{12, 10, 8, 9,      13, 15, 6, 11,      0, 1, 4, 2,     14, 3, 5, 7},
    s1x4,4,MPKIND__SPLIT,1,       warn__none,  s4x4,      0x055, 0, spcmap_4x4v},
   {{0, 1, 2, 3, 4, 5, 6, 7,      8, 9, 10, 11, 12, 13, 14, 15},
    s1x1,16,MPKIND__NONE,0,       warn__none, s4x4,       0x000, 0, spcmap_4x4_1x1},
   {{0, 2, 7, 11,        1, 5, 6, 8},
    s2x2,2,MPKIND__NONE,0,        warn__none,  s3x4,      0x000, 0, spcmap_trglbox3x4a},
   {{11, 2, 7, 9,        1, 3, 5, 8},
    s2x2,2,MPKIND__NONE,0,        warn__none,  s3x4,      0x000, 0, spcmap_trglbox3x4b},
   {{0, 11, 7, 9,        1, 3, 6, 5},
    s2x2,2,MPKIND__NONE,0,        warn__none,  s3x4,      0x000, 0, spcmap_trglbox3x4c},
   {{0, 2, 11, 9,        5, 3, 6, 8},
    s2x2,2,MPKIND__NONE,0,        warn__none,  s3x4,      0x000, 0, spcmap_trglbox3x4d},
   {{12, 14, 15, 9,    13, 0, 2, 3,    7, 1, 4, 6,    10, 11, 5, 8},
    s2x2,4,MPKIND__NONE,0,        warn__none,  s4x4,      0x000, 0, spcmap_trglbox4x4},
   {{9, 7, 12, 16,        2, 6, 19, 17},
    s2x2,2,MPKIND__NONE,0,        warn__none,  s4x5,      0x000, 0, spcmap_trglbox4x5a},
   {{8, 2, 17, 15,        7, 5, 18, 12},
    s2x2,2,MPKIND__NONE,0,        warn__none,  s4x5,      0x000, 0, spcmap_trglbox4x5b},
   {{4, 3,                1, 2},
    s1x2,2,MPKIND__NONE,1,        warn__none,  s2x3,      0x000, 0, spcmap_2x3_1234},
   {{5, 4,                0, 1},
    s1x2,2,MPKIND__NONE,1,        warn__none,  s2x3,      0x000, 0, spcmap_2x3_0145},

   // Special maps for putting back end-to-end 1x6's, or 1x2 diamonds,
   // resulting from collisions at one end.
   {{0, 1, 3, -1, -1, 2,               -1, -1, 6, 4, 5, 7},
    s1x6,2,MPKIND__NONE,0,        warn__none,  s1x8,      0x000, 0, spcmap_1x8_1x6},
   {{6, 7, 0, -1, -1, 5,               -1, -1, 1, 2, 3, 4},
    s_1x2dmd,2,MPKIND__NONE,0,    warn__none,  s_rigger,  0x000, 0, spcmap_rig_1x6},

   {{0, 7, 5, -1, 6, -1,               -1, 2, -1, 4, 3, 1},
    s2x3,2,MPKIND__SPLIT,1,       warn__none,  s_qtag,    0x005, 0, spcmap_qtag_2x3},
   {{0, 1, 3, 2, 4, 5, 7, 6},
    s_qtag,1,MPKIND__NONE,0,      warn__none,  s2x4,      0x000, 0, spcmap_dbloff1},
   {{2, 3, 4, 5, 6, 7, 0, 1},
    s_qtag,1,MPKIND__NONE,0,      warn__none,  s2x4,      0x000, 0, spcmap_dbloff2},
   {{0, 1, 9, 6, 7, 3},
    s1x6,1,MPKIND__NONE,0,        warn__none,  sbighrgl,  0x000, 0, spcmap_dhrgl1},
   {{11, 10, 9, 5, 4, 3},
    s1x6,1,MPKIND__NONE,0,        warn__none,  sbighrgl,  0x000, 0, spcmap_dhrgl2},
   {{0, 1, 3, 2, 6, 7, 9, 8},
    s1x8,1,MPKIND__NONE,0,        warn__none,  sbigbone,  0x000, 0, spcmap_dbgbn1},
   {{11, 10, 3, 2, 5, 4, 9, 8},
    s1x8,1,MPKIND__NONE,0,        warn__none,  sbigbone,  0x000, 0, spcmap_dbgbn2},

   // Distorted 1/4 tags.

   {{0, 1, 4, 5, 6, 7, 10, 11},
    s_qtag,1,MPKIND__NONE,0,      warn__none,  s3dmd,     0x000, 0, spcmap_dqtag1},
   {{1, 2, 4, 5, 7, 8, 10, 11},
    s_qtag,1,MPKIND__NONE,0,      warn__none,  s3dmd,     0x000, 0, spcmap_dqtag2},
   {{13, 14, 2, 3, 5, 6, 10, 11},
    s_qtag,1,MPKIND__NONE,0,      warn__none,  s4x4,      0x000, 0, spcmap_dqtag3},
   {{13, 14, 1, 7, 5, 6, 9, 15},
    s_qtag,1,MPKIND__NONE,0,      warn__none,  s4x4,      0x000, 0, spcmap_dqtag4},
   {{7, 23, 15, 21, 19, 11, 3, 9},
    s_qtag,1,MPKIND__NONE,0,      warn__none,  s4x6,      0x001, 0, spcmap_dqtag5},
   {{6, 22, 14, 20, 18, 10, 2, 8},
    s_qtag,1,MPKIND__NONE,0,      warn__none,  s4x6,      0x001, 0, spcmap_dqtag6},

   // Maps for finding staggered C/L/W's of 3.

   {{12, 15, 14, 4, 7, 6},
    s2x3,1,MPKIND__NONE,0,        warn__none,  s4x4,      0x000, 0, spcmap_stw3a},
   {{13, 3, 0, 5, 11, 8},
    s2x3,1,MPKIND__NONE,0,        warn__none,  s4x4,      0x000, 0, spcmap_stw3b},


   // Maps for finding C/L/W's of 3.

   {{9, 11, 7, 15, 3, 1},
    s1x3,2,MPKIND__NONE,0,        warn__none,  s4x4,      0x000, 0, spcmap_3ri},
   {{11, 7, 2, 10, 15, 3},
    s1x3,2,MPKIND__NONE,0,        warn__none,  s4x4,      0x000, 0, spcmap_3li},
   {{6, 5, 4, 12, 13, 14},
    s1x3,2,MPKIND__NONE,0,        warn__none,  s4x4,      0x000, 0, spcmap_3ro},
   {{8, 6, 5, 13, 14, 0},
    s1x3,2,MPKIND__NONE,0,        warn__none,  s4x4,      0x000, 0, spcmap_3lo},
   {{0, 1, 2, 10, 9, 8},
    s1x3,2,MPKIND__NONE,0,        warn__none,  s2x8,      0x000, 0, spcmap_328a},
   {{5, 6, 7, 15, 14, 13},
    s1x3,2,MPKIND__NONE,0,        warn__none,  s2x8,      0x000, 0, spcmap_328b},
   {{1, 2, 3, 11, 10, 9},
    s1x3,2,MPKIND__NONE,0,        warn__none,  s2x8,      0x000, 0, spcmap_328c},
   {{4, 5, 6, 14, 13, 12},
    s1x3,2,MPKIND__NONE,0,        warn__none,  s2x8,      0x000, 0, spcmap_328d},
   {{2, 3, 4, 12, 11, 10},
    s1x3,2,MPKIND__NONE,0,        warn__none,  s2x8,      0x000, 0, spcmap_328e},
   {{3, 4, 5, 13, 12, 11},
    s1x3,2,MPKIND__NONE,0,        warn__none,  s2x8,      0x000, 0, spcmap_328f},
   {{0, 1, 2, 6, 5, 4},
    s1x3,2,MPKIND__NONE,0,        warn__none, s1x3dmd,    0x000, 0, spcmap_31x3d},
   {{0, 1, 2, 6, 5, 4},
    s1x3,2,MPKIND__NONE,0,        warn__none, s3x1dmd,    0x000, 0, spcmap_33x1d},
   {{4, 7, 22, 10, 19, 16},
    s1x3,2,MPKIND__NONE,1,        warn__none, s4x6,       0x005, 0, spcmap_34x6a},
   {{7, 22, 13, 1, 10, 19},
    s1x3,2,MPKIND__NONE,1,        warn__none, s4x6,       0x005, 0, spcmap_34x6b},
   {{1, 3, 4, 0, 7, 5},
    s1x3,2,MPKIND__NONE,1,        warn__none, s_qtag,     0x005, 0, spcmap_3lqtg},
   {{0, 1, 2, 6, 5, 4},
    s1x3,2,MPKIND__NONE,0,        warn__none, s_spindle,  0x000, 0, spcmap_3spn},
   {{0, 1, 2, 5, 4, 3},
    s1x3,2,MPKIND__NONE,0,        warn__none, s1x6,       0x000, 0, spcmap_31x6},
   {{13, 10, 6, 8,          15, 17, 3, 5,       18, 20, 1, 22},
    s1x4,3,MPKIND__NONE,1,        warn__none,  s_bigblob, 0x000, 0, spcmap_blob_1x4a},
   {{19, 16, 12, 14,        21, 23, 9, 11,      0, 2, 7, 4},
    s1x4,3,MPKIND__NONE,1,        warn__none,  s_bigblob, 0x000, 0, spcmap_blob_1x4b},
   {{1, 22, 18, 20,         3, 5, 15, 17,       6, 8, 13, 10},
    s1x4,3,MPKIND__NONE,1,        warn__none,  s_bigblob, 0x015, 0, spcmap_blob_1x4c},
   {{19, 16, 12, 14,        21, 23, 9, 11,      0, 2, 7, 4},
    s1x4,3,MPKIND__NONE,1,        warn__none,  s_bigblob, 0x015, 0, spcmap_blob_1x4d},
   {{15, 17, 5, 3, 6, 8, 10, 13,          18, 20, 22, 1, 3, 5, 17, 15},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s_bigblob, 0x000, 0, spcmap_wblob_1x4a},
   {{21, 23, 11, 9, 12, 14, 16, 19,       0, 2, 4, 7, 9, 11, 23, 21},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s_bigblob, 0x000, 0, spcmap_wblob_1x4b},
   {{3, 5, 17, 15, 18, 20, 22, 1,         6, 8, 10, 13, 15, 17, 5, 3},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s_bigblob, 0x005, 0, spcmap_wblob_1x4c},
   {{21, 23, 11, 9, 12, 14, 16, 19,       0, 2, 4, 7, 9, 11, 23, 21},
    s2x4,2,MPKIND__NONE,1,        warn__none,  s_bigblob, 0x005, 0, spcmap_wblob_1x4d},
   {{0, 1, 2, 3, 9, 8, 7, 6},
    s1x2,4,MPKIND__NONE,0,        warn__none,  sbigbone,  0x000, 0, spcmap_bigbone_cw},
   {{11, 10, 2, 3, 9, 8, 4, 5},
    s1x2,4,MPKIND__NONE,0,        warn__none,  sbigbone,  0x000, 0, spcmap_bigbone_ccw}};


static expand::thing rear_thar_stuff = {{9, 10, 13, 14, 1, 2, 5, 6}, s_thar, s4x4, 0};
static expand::thing rear_alamo_stuff = {{6, 7, 0, 1, 2, 3, 4, 5}, s_thar, s_alamo, 0};
static expand::thing rear_xwv_stuff = {{7, 0, 1, 2, 3, 4, 5, 6}, s_crosswave, s2x4, 0};
static expand::thing rear_ohh_stuff = {{-1, 5, 4, -1, -1, 7, 6, -1, -1, 1, 0, -1, -1, 3, 2, -1}, s4x4, s_thar, 0};
static expand::thing rear_bigd_stuff1 = {{-1, -1, 10, 11, 1, 0, -1, -1, 4, 5, 7, 6}, sbigdmd, s3x4, 1};
static expand::thing rear_bigd_stuff2 = {{8, 9, 10, 11, -1, -1, 2, 3, 4, 5, -1, -1}, sbigdmd, s3x4, 1};
static expand::thing rear_alamo_stuffa = {{0, 1, 3, 2, 4, 5, 7, 6}, s_qtag, s_alamo, 0};
static expand::thing rear_alamo_stuffb = {{6, 7, 1, 0, 2, 3, 5, 4}, s_qtag, s_alamo, 1};
static expand::thing rear_sqset_stuffa = {{10, 9, 13, -1, -1, 6, 2, 1, 5, -1, -1, 14}, sdeepxwv, s4x4, 0};
static expand::thing rear_sqset_stuffb = {{6, 5, 9, -1, -1, 2, 14, 13, 1, -1, -1, 10}, sdeepxwv, s4x4, 1};
static expand::thing rear_bone_stuffa = {{0, 5, 7, 6, 4, 1, 3, 2}, s1x8, s_bone, 0};
static expand::thing rear_bone_stuffb = {{0, 3, 2, 5, 4, 7, 6, 1}, s_bone, s2x4, 0};
static expand::thing rear_bone_stuffc = {{6, 3, 1, 4, 2, 7, 5, 0}, s_bone, s_rigger, 0};
static expand::thing rear_bone_stuffd = {{0, 3, 5, 2, 4, 7, 1, 6}, s_bone, s2x4, 0};
static expand::thing rear_rig_stuffa = {{1, 2, 3, 4, 5, 6, 7, 0}, s_rigger, s2x4, 0};
static expand::thing rear_rig_stuffb = {{3, 6, 4, 5, 7, 2, 0, 1}, s_rigger, s1x8, 0};
static expand::thing rear_rig_stuffc = {{6, 3, 1, 4, 2, 7, 5, 0}, s_rigger, s_bone, 0};
static expand::thing rear_rig_stuffd = {{1, 2, 4, 3, 5, 6, 0, 7}, s_rigger, s2x4, 0};
static expand::thing rear_funnydmd   = {{7, 0, 1, 2, 3, 4, 5, 6}, s_qtag, s2x4, 1};
static expand::thing rear_funny3x1   = {{5, 7, 6, 0, 1, 3, 2, 4}, s2x4, s_qtag, 1};

static expand::thing rear_tgl4a_stuff = {{2, 3, 0, 1}, s_trngl4, s2x2, 0};
static expand::thing rear_tgl4b_stuff = {{2, 3, 1, 0}, s_trngl4, s1x4, 1};

static expand::thing rear_dbt_stuff   = {{7, 0, 1, 6, 5, 2, 3, 4}, sdbltrngl4, s2x4, 0};

static expand::thing rear_c1a_stuff = {{0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1}, s_c1phan, s2x4, 0};
static expand::thing rear_44a_stuff = {{-1, -1, 4, 3, -1, -1, 6, 5, -1, -1, 0, 7, -1, -1, 2, 1}, s4x4, s2x4, 0};
static expand::thing rear_c1b_stuff = {{-1, 0, -1, 1, -1, 3, -1, 2, -1, 4, -1, 5, -1, 7, -1, 6}, s_c1phan, s2x4, 0};
static expand::thing rear_44b_stuff = {{-1, 3, -1, 2, -1, 4, -1, 5, -1, 7, -1, 6, -1, 0, -1, 1}, s4x4, s2x4, 0};
static expand::thing rear_c1c_stuff = {{6, -1, 7, -1, 0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1}, s_c1phan, s2x4, 1};
static expand::thing rear_44c_stuff = {{-1, -1, 2, 1, -1, -1, 4, 3, -1, -1, 6, 5, -1, -1, 0, 7}, s4x4, s2x4, 1};
static expand::thing rear_c1d_stuff = {{-1, 7, -1, 6, -1, 0, -1, 1, -1, 3, -1, 2, -1, 4, -1, 5}, s_c1phan, s2x4, 1};
static expand::thing rear_44d_stuff = {{-1, 0, -1, 1, -1, 3, -1, 2, -1, 4, -1, 5, -1, 7, -1, 6}, s4x4, s2x4, 1};

static expand::thing rear_c1e_stuff = {{3, -1, 1, -1, 7, -1, 5, -1, 11, -1, 9, -1, 15, -1, 13, -1}, s_c1phan, s_c1phan, 0};
static expand::thing rear_c1f_stuff = {{-1, 0, -1, 2, -1, 4, -1, 6, -1, 8, -1, 10, -1, 12, -1, 14}, s_c1phan, s_c1phan, 0};

static expand::thing rear_vrbox_stuff = {{1, 0, 3, 2}, s2x2, s1x4, 1};
static expand::thing rear_hrbox_stuff = {{0, 3, 2, 1}, s2x2, s1x4, 0};
static expand::thing rear_qtag_stuff = {{7, 0, 1, 2, 3, 4, 5, 6}, s_ptpd, s2x4, 1};
static expand::thing rear_ptpd_stuff = {{0, 1, 2, 3, 4, 5, 6, 7}, s_ptpd, s1x8, 0};
static expand::thing rear_sqtag_stuff = {{0, 1, 2, 3}, sdmd, s1x4, 0};
static expand::thing rear_twistqtag_stuff = {{0, 3, 2, 1}, s1x4, sdmd, 0};
static expand::thing rear_twist2x4c_stuff = {{5, 7, 6, 0, 1, 3, 2, 4}, s2x4, s_qtag, 1};
static expand::thing rear_twist1x8c_stuff = {{0, 3, 2, 1, 4, 7, 6, 5}, s1x8, s_ptpd, 0};
static expand::thing rear_3x1qtag_stuff = {{4, 5, 3, 6, 0, 1, 7, 2}, s3x1dmd, s_qtag, 1};
static expand::thing rear_1x3qtag_stuff = {{9, 10, 0, 11, 3, 4, 6, 5}, s_323, s3x4, 1};

static expand::thing step_8by_stuff = {{6, 7, 0, 1, 2, 3, 4, 5}, s_thar, s_qtag, 0};
static expand::thing step_sqs_stuff = {{7, 0, 1, 2, 3, 4, 5, 6}, s_thar, s2x4, 0};
static expand::thing step_sqsctr_stuff = {{7, 0, 1, 2, 3, 4, 5, 6}, s_qtag, s2x4, 1};
static expand::thing step_qtctr_stuff = {{5, 7, 6, 0, 1, 3, 2, 4}, s2x4, s_qtag, 1};
static expand::thing step_1x8ctr_stuff = {{0, 1, 2, 3, 4, 5, 6, 7}, s_ptpd, s1x8, 0};
static expand::thing step_ptpctr_stuff = {{0, 3, 2, 1, 4, 7, 6, 5}, s1x8, s_ptpd, 0};
static expand::thing step_1x4ctr_stuff = {{0, 1, 2, 3}, sdmd, s1x4, 0};
static expand::thing step_dmdctr_stuff = {{0, 3, 2, 1}, s1x4, sdmd, 0};

static expand::thing step_1x8_stuff = {{0, 7, 6, 1, 4, 3, 2, 5}, s1x8, s2x4, 0};
static expand::thing step_qbox_stuff = {{0, 3, 5, 2, 4, 7, 1, 6}, s_bone, s2x4, 0};
static expand::thing rear_3n1a_stuff = {{3, 1, 7, 5, 11, 9, 15, 13}, s2x4, s_c1phan, 0};
static expand::thing rear_3n1b_stuff = {{0, 2, 6, 4, 8, 10, 14, 12}, s2x4, s_c1phan, 0};
static expand::thing rear_3n1c_stuff = {{6, 0, 1, 3, 2, 4, 5, 7}, s2x4, s_rigger, 0};

static expand::thing step_1x4_side_stuff = {{0, 1, 2, 3}, s1x4, sdmd, 0};
static expand::thing step_1x4_stuff = {{0, 3, 2, 1}, s1x4, s2x2, 0};
static expand::thing step_1x2_stuff = {{0, 1}, s1x2, s1x2, 1};
static expand::thing step_offs1_stuff = {{-1, -1, 0, 1, 3, 2, -1, -1, 6, 7, 9, 8}, s3x4, s2x6, 1};
static expand::thing step_offs2_stuff = {{11, 10, -1, -1, 3, 2, 5, 4, -1, -1, 9, 8}, s3x4, s2x6, 1};
static expand::thing step_3x4_stuff = {{11, 10, 0, 1, 3, 2, 5, 4, 6, 7, 9, 8}, s3x4, s2x6, 1};
static expand::thing step_2x2v_stuff = {{1, 2, 3, 0}, s2x2, s1x4, 0};
static expand::thing step_2x2h_stuff = {{0, 1, 2, 3}, s2x2, s1x4, 1};
static expand::thing step_8ch_stuff = {{7, 6, 0, 1, 3, 2, 4, 5}, s2x4, s2x4, 1};
static expand::thing step_2x4nestuff = {{0, -1, 1, -1, 3, -1, 2, -1, 4, -1, 5, -1, 7, -1, 6, -1}, s_c1phan, s2x4, 0};
static expand::thing step_2x4nwstuff = {{-1, 1, -1, 0, -1, 3, -1, 2, -1, 5, -1, 4, -1, 7, -1, 6}, s_c1phan, s2x4, 0};
static expand::thing step_li_stuff = {{1, 2, 7, 4, 5, 6, 3, 0}, s2x4, s1x8, 0};
static expand::thing step_li6_stuff = {{1, 5, 3, 4, 2, 0}, s2x3, s1x6, 0};
static expand::thing step_liphan_stuff = {{-1, 1, -1, 2, -1, 3, -1, 0}, s2x4, s1x4, 0};
static expand::thing step_thar_stuff = {{7, 6, 1, 0, 3, 2, 5, 4}, s_thar, s_alamo, 0};
static expand::thing step_spindle_stuff = {{3, 6, 5, 4, 7, 2, 1, 0}, s_spindle, s1x8, 0};
static expand::thing step_bn_stuff = {{0, 7, 2, 1, 4, 3, 6, 5}, s2x4, s_bone, 0};
static expand::thing step_bn23_stuff = {{0, 2, 1, 3, 5, 4}, s2x3, s_bone6, 0};
static expand::thing step_24bn_stuff = {{0, 3, 5, 2, 4, 7, 1, 6}, s_bone, s2x4, 0};
static expand::thing step_23bn_stuff = {{0, 2, 4, 3, 5, 1}, s_bone6, s2x3, 0};
static expand::thing step_232a_stuff = {{-1, 0, 7, 6, -1, 1, -1, 4, 3, 2, -1, 5}, s_23232, s2x4, 1};
static expand::thing step_232b_stuff = {{7, -1, -1, 0, 1, 2, 3, -1, -1, 4, 5, 6}, s_23232, s2x4, 1};
static expand::thing step_tby_stuff = {{5, 6, 7, 0, 1, 2, 3, 4}, s2x4, s_qtag, 1};
static expand::thing step_2x4_rig_stuff = {{7, 0, 1, 2, 3, 4, 5, 6}, s2x4, s_rigger, 0};
static expand::thing step_bone_stuff = {{1, 4, 7, 6, 5, 0, 3, 2}, s_bone, s1x8, 0};
static expand::thing step_bone_rigstuff = {{7, 2, 4, 1, 3, 6, 0, 5}, s_bone, s_rigger, 0};
static expand::thing step_qtag_rigstuff = {{6, 7, 1, 0, 2, 3, 5, 4}, s_qtag, s_rigger, 1};
static expand::thing step_rig_bonestuff = {{7, 2, 4, 1, 3, 6, 0, 5}, s_rigger, s_bone, 0};
static expand::thing step_rig_stuff = {{2, 7, 4, 5, 6, 3, 0, 1}, s_rigger, s1x8, 0};
static expand::thing step_2x1d_stuff = {{0, 1, 5, 3, 4, 2}, s_1x2dmd, s1x6, 0};
static expand::thing step_short6_stuff = {{1, 0, 2, 4, 3, 5}, s_short6, s1x6, 1};

static expand::thing step_phan1_stuff = {{-1, 7, -1, 6, -1, 1, -1, 0, -1, 3, -1, 2, -1, 5, -1, 4},
                                        s_c1phan, s2x4, 1};
static expand::thing step_phan2_stuff = {{7, -1, 6, -1, 0, -1, 1, -1, 3, -1, 2, -1, 4, -1, 5, -1},
                                        s_c1phan, s2x4, 1};
static expand::thing step_phan3_stuff = {{0, -1, 1, -1, 3, -1, 2, -1, 4, -1, 5, -1, 7, -1, 6, -1},
                                         s_c1phan, s2x4, 0};
static expand::thing step_phan4_stuff = {{-1, 1, -1, 0, -1, 3, -1, 2, -1, 5, -1, 4, -1, 7, -1, 6},
                                        s_c1phan, s2x4, 0};

static expand::thing step_4x4_1_stuff = {{-1, 1, -1, 0, -1, 3, -1, 2, -1, 5, -1, 4, -1, 7, -1, 6},
                                        s4x4, s2x4, 1};
static expand::thing step_4x4_2_stuff = {{-1, -1, 3, 1, -1, -1, 4, 2, -1, -1, 7, 5, -1, -1, 0, 6},
                                        s4x4, s2x4, 1};
static expand::thing step_4x4_3_stuff = {{-1, -1, 4, 2, -1, -1, 7, 5, -1, -1, 0, 6, -1, -1, 3, 1},
                                        s4x4, s2x4, 0};
static expand::thing step_4x4_4_stuff = {{-1, 3, -1, 2, -1, 5, -1, 4, -1, 7, -1, 6, -1, 1, -1, 0},
                                        s4x4, s2x4, 0};

static expand::thing step_bigd_stuff1 = {{0, 1, 3, 2, -1, -1, 6, 7, 9, 8, -1, -1},
                                        sbigdmd, s2x6, 0};
static expand::thing step_bigd_stuff2 = {{-1, -1, 3, 2, 4, 5, -1, -1, 9, 8, 10, 11},
                                        sbigdmd, s2x6, 0};
static expand::thing step_tgl4_stuffa = {{2, 3, 0, 1}, s_trngl4, s1x4, 1};
static expand::thing step_tgl4_stuffb = {{3, 2, 0, 1}, s_trngl4, s2x2, 0};
static expand::thing step_dmd_stuff   = {{0, 3, 2, 1}, sdmd, s1x4, 0};
static expand::thing step_tgl_stuff   = {{2, 0, 1}, s_trngl, s1x3, 1};
static expand::thing step_ptpd_stuff  = {{0, 3, 2, 1, 4, 7, 6, 5}, s_ptpd, s1x8, 0};

full_expand::thing rear_1x2_pair      = {warn__rear_back,  8, &step_1x2_stuff};
full_expand::thing rear_2x2_pair      = {warn__rear_back,  8, &step_2x2v_stuff};
full_expand::thing rear_bone_pair     = {warn__some_rear_back, 0, &rear_bone_stuffb};
full_expand::thing step_8ch_pair      = {warn__none,       0, &step_8ch_stuff};
full_expand::thing step_qtag_pair     = {warn__none,       0, &step_tby_stuff};
full_expand::thing step_2x2h_pair     = {warn__none,    16+1, &step_2x2h_stuff};
full_expand::thing step_2x2v_pair     = {warn__none,    16+2, &step_2x2v_stuff};
full_expand::thing step_spindle_pair  = {warn__some_touch, 0, &step_spindle_stuff};
full_expand::thing step_dmd_pair      = {warn__some_touch, 0, &step_dmd_stuff};
full_expand::thing step_tgl_pair      = {warn__some_touch, 0, &step_tgl_stuff};
full_expand::thing step_ptpd_pair     = {warn__some_touch, 0, &step_ptpd_stuff};

full_expand::thing step_qtgctr_pair   = {warn__some_touch, 8, &step_qtctr_stuff};

full_expand::thing touch_init_table1[] = {
   // Rear back from a miniwave to facing people.
   {warn__rear_back,       8, &step_1x2_stuff,   s1x2,         0xFU,        0x2U, ~0U},
   // Rear back from a 4-person triangle to a "split square thru" setup.
   {warn__some_rear_back,  0, &rear_tgl4a_stuff, s_trngl4,    0xFFU,       0xDAU, ~0U},
   // Rear back from a 4-person triangle to a normal "square thru" setup.
   {warn__some_rear_back,  0, &rear_tgl4a_stuff, s_trngl4,    0xFFU,       0xD7U, ~0U},
   // Two similar ones with miniwave base for funny square thru.
   {warn__some_rear_back,  0, &rear_tgl4a_stuff, s_trngl4,    0xFFU,       0xD2U, ~0U},
   // The base couldn't want to rear back -- result would be stupid.)
   {warn__some_rear_back,  0, &rear_tgl4a_stuff, s_trngl4,    0xFFU,       0xD8U, ~0U},
   // Rear back from a 4-person triangle to a single 8 chain.
   {warn__awful_rear_back, 0, &rear_tgl4b_stuff, s_trngl4,    0xFFU,       0x22U, ~0U},
   // Rear back from a tidal wave to facing lines.
   {warn__rear_back,       8, &step_li_stuff,    s1x8,      0xFFFFU,     0x2882U, ~0U},
   // Ends rear back from a "bone" to grand 8-chain or whatever.
   {warn__some_rear_back,  8, &rear_bone_stuffa, s_bone,    0xFFFFU,     0x55F5U, 0xF5F5U},
   // Centers rear back from a "bone" to lines facing or "split square thru" setup.
   {warn__some_rear_back,  0, &rear_bone_stuffb, s_bone,    0xFFFFU,     0x0802U, 0x0F0FU},
   // All rear back from a "bone" to a "rigger".
   {warn__rear_back,       0, &rear_bone_stuffc, s_bone,    0xFFFFU,     0x58F2U, 0xFFFFU},
   // Rear back from an alamo wave to crossed single 8-chains.
   {warn__rear_back,       0, &rear_ohh_stuff, s4x4,    0x3C3C3C3CU, 0x1C203408U, ~0U},
   // Rear back from parallel waves to an 8 chain.
   {warn__rear_back,       8, &step_8ch_stuff, s2x4,        0xFFFFU,     0x2288U, ~0U},
   {warn__rear_back,       8, &step_8ch_stuff, s2x4,        0xF0F0U,     0x2080U, ~0U},
   {warn__rear_back,       8, &step_8ch_stuff, s2x4,        0x0F0FU,     0x0208U, ~0U},
   {warn__rear_back,       8, &step_8ch_stuff, s2x4,        0x0FF0U,     0x0280U, ~0U},
   {warn__rear_back,       8, &step_8ch_stuff, s2x4,        0xF00FU,     0x2008U, ~0U},
   // Rear back from 2x6 waves to a triple-column 8 chain (3x4.)
   {warn__rear_back,       8, &step_3x4_stuff, s2x6,      0xF00F00U,   0x200800U, ~0U},
   {warn__rear_back,       8, &step_3x4_stuff, s2x6,      0x00F00FU,   0x002008U, ~0U},
   // Some people rear back from parallel waves, while the others are column-like, to a C1 phantom.
   {warn__some_rear_back,  8, &step_2x4nestuff,s2x4,        0xFFFFU,     0x52F8U, ~0U},
   {warn__some_rear_back,  8, &step_2x4nestuff,s2x4,        0xFFFFU,     0x72D8U, ~0U},
   {warn__some_rear_back,  8, &step_2x4nestuff,s2x4,        0xFFFFU,     0xF258U, ~0U},
   {warn__some_rear_back,  8, &step_2x4nestuff,s2x4,        0xFFFFU,     0xD278U, ~0U},
   {warn__some_rear_back,  8, &step_2x4nwstuff,s2x4,        0xFFFFU,     0x258FU, ~0U},
   {warn__some_rear_back,  8, &step_2x4nwstuff,s2x4,        0xFFFFU,     0x278DU, ~0U},
   {warn__some_rear_back,  8, &step_2x4nwstuff,s2x4,        0xFFFFU,     0x2F85U, ~0U},
   {warn__some_rear_back,  8, &step_2x4nwstuff,s2x4,        0xFFFFU,     0x2D87U, ~0U},
   // Rear back from columns to end-to-end single 8-chains.
   {warn__awful_rear_back, 8, &step_1x8_stuff, s2x4,        0xFFFFU,     0x55FFU, ~0U},

   // Some rear back from ends of 2x4, to a rigger.
   {warn__some_rear_back,  8, &rear_rig_stuffd, s2x4,       0xFFFFU,     0x69C3U, ~0U},
   // Or from centers, to a bone.
   {warn__some_rear_back,  8, &rear_bone_stuffd, s2x4,      0xFFFFU,     0x963CU, ~0U},

   // Centers rear back from 1/4-box to triangles.
   {warn__some_rear_back,  8, &step_qbox_stuff, s2x4,       0xFFFFU,     0x57FDU, ~0U},

   // Some people rear back from 3&1 column to rigger.
   {warn__some_rear_back,  0, &rear_3n1c_stuff, s2x4,       0xFFFFU,     0x5DF7U, ~0U},

   // Some people rear back from 3&1 line to triangles.
   {warn__some_rear_back,  0, &rear_3n1a_stuff, s2x4,       0xFFFFU,     0x2A80U, ~0U},
   // Some people rear back from 3&1 line to triangles.
   {warn__some_rear_back,  0, &rear_3n1b_stuff, s2x4,       0xFFFFU,     0xA208U, ~0U},
   // Rear back from a right-hand box to a single 8 chain.
   {warn__awful_rear_back, 0, &rear_vrbox_stuff, s2x2,        0xFFU,       0x28U, ~0U},
   {warn__awful_rear_back, 0, &rear_hrbox_stuff, s2x2,        0xFFU,       0x5FU, ~0U},

   // Centers rear back from appropriate "diamonds" to T-boned pairs facing.
   {warn__some_rear_back,  0, &rear_funnydmd,  s_qtag,    0xFFFFU,     0x78D2U, ~0U},
   // Centers rear back 3&1 lines, ends facing, centers have right hands.
   {warn__centers_rear_back_staying_in_center, 0, &rear_funny3x1,  s2x4,      0xFFFFU,     0x8A20U, ~0U},

   // Ends rear back from a "rigger" to lines facing or "split square thru" setup.
   {warn__some_rear_back,  0, &rear_rig_stuffa,s_rigger,    0xFFFFU,     0x0802U, 0x0F0FU},
   // Centers rear back from a "rigger" to grand 8-chain or whatever.
   {warn__some_rear_back,  0, &rear_rig_stuffb,s_rigger,    0xFFFFU,     0x55F5U, 0xF5F5U},
   // All rear back from a "rigger" to a "bone".
   {warn__rear_back,       0, &rear_rig_stuffc,s_rigger,    0xFFFFU,     0x58F2U, 0xFFFFU},
   // Some people rear back from horrible "T"'s to couples facing or "split square thru" setup.
   {warn__some_rear_back,  0, &rear_bigd_stuff1,sbigdmd,  0x0FF0FFU,   0x0520F8U, ~0U},
   {warn__some_rear_back,  0, &rear_bigd_stuff1,sbigdmd,  0x0FF0FFU,   0x082028U, ~0U},
   // Some people rear back from horrible "T"'s to couples facing or "split square thru" setup.
   {warn__some_rear_back,  0, &rear_bigd_stuff2,sbigdmd,  0xFF0FF0U,   0x2F0850U, ~0U},
   {warn__some_rear_back,  0, &rear_bigd_stuff2,sbigdmd,  0xFF0FF0U,   0x280820U, ~0U},
   // Rear back from thar to alamo 8-chain.
   {warn__rear_back,       0, &rear_thar_stuff, s_thar,     0xFFFFU,     0x278DU, ~0U},
   // Same, in crosswave.
   {warn__rear_back,       0, &rear_xwv_stuff, s_crosswave, 0xFFFFU,     0x278DU, ~0U},

   // Some rear back from an alamo to a 1/4 tag.
   {warn__some_rear_back,  8, &rear_alamo_stuffa, s_alamo,  0xFFFFU,     0x57FDU, 0x5F5FU},
   {warn__some_rear_back,  8, &rear_alamo_stuffb, s_alamo,  0xFFFFU,     0x2080U, 0xF5F5U},

   // Some rear back from a squared set to a "deepxwv".
   {warn__some_rear_back,  8, &rear_sqset_stuffa, s4x4, 0x3C3C3C3CU, 0x1C143414U, 0x3C143C14U},
   {warn__some_rear_back,  8, &rear_sqset_stuffb, s4x4, 0x3C3C3C3CU, 0x00200008U, 0x143C143CU},

   // Check for certain people rearing back from C1 phantoms.
   {warn__some_rear_back,  0, &rear_c1a_stuff,s_c1phan, 0xCCCCCCCCU, 0x884C00C4U, ~0U},
   {warn__some_rear_back,  0, &rear_c1a_stuff,s_c1phan, 0xCCCCCCCCU, 0x4C4CC4C4U, ~0U},
   // Or from equivalent pinwheel.
   {warn__some_rear_back,  0, &rear_44a_stuff, s4x4,    0x0F0F0F0FU, 0x030C0906U, ~0U},
   {warn__some_rear_back,  0, &rear_44a_stuff, s4x4,    0x0F0F0F0FU, 0x0F0D0507U, ~0U},
   {warn__some_rear_back,  0, &rear_c1b_stuff,s_c1phan, 0x33333333U, 0x13223100U, ~0U},
   {warn__some_rear_back,  0, &rear_c1b_stuff,s_c1phan, 0x33333333U, 0x13313113U, ~0U},
   {warn__some_rear_back,  0, &rear_44b_stuff, s4x4,    0x33333333U, 0x22310013U, ~0U},
   {warn__some_rear_back,  0, &rear_44b_stuff, s4x4,    0x33333333U, 0x31311313U, ~0U},
   {warn__some_rear_back,  0, &rear_c1c_stuff,s_c1phan, 0xCCCCCCCCU, 0x08CC8044U, ~0U},
   {warn__some_rear_back,  0, &rear_c1c_stuff,s_c1phan, 0xCCCCCCCCU, 0x08808008U, ~0U},
   {warn__some_rear_back,  0, &rear_44c_stuff, s4x4,    0x0F0F0F0FU, 0x0B04010EU, ~0U},
   {warn__some_rear_back,  0, &rear_44c_stuff, s4x4,    0x0F0F0F0FU, 0x0800020AU, ~0U},
   {warn__some_rear_back,  0, &rear_c1d_stuff,s_c1phan, 0x33333333U, 0x11203302U, ~0U},
   {warn__some_rear_back,  0, &rear_c1d_stuff,s_c1phan, 0x33333333U, 0x20200202U, ~0U},
   {warn__some_rear_back,  0, &rear_44d_stuff, s4x4,    0x33333333U, 0x20330211U, ~0U},
   {warn__some_rear_back,  0, &rear_44d_stuff, s4x4,    0x33333333U, 0x20020220U, ~0U},

   {warn__rear_back,       0, &rear_c1e_stuff,s_c1phan, 0xCCCCCCCCU, 0x084C80C4U, ~0U},
   {warn__rear_back,       0, &rear_c1f_stuff,s_c1phan, 0x33333333U, 0x13203102U, ~0U},

   {warn__some_rear_back,  0, &rear_dbt_stuff,sdbltrngl4, 0xFFFFU,   0x2F2FU,     ~0U},

   // Rear back from a wave to facing couples.
   {warn__rear_back,     4+8, &step_2x2v_stuff, s1x4,         0xFFU,       0x28U, ~0U},
   {warn__none,            0, (expand::thing *) 0, nothing}
};

full_expand::thing touch_init_table2[] = {
   // Have the centers rear back from a 1/4 tag or 3/4 tag.
   {warn__rear_back,       0, &rear_qtag_stuff, s_qtag,     0xFFFFU,     0x08A2U, ~0U},
   {warn__rear_back,       0, &rear_qtag_stuff, s_qtag,     0xFFFFU,     0xA802U, ~0U},
   // Have the centers rear back from point-to-point 1/4 tags or 3/4 tags.
   {warn__rear_back,       0, &rear_ptpd_stuff, s_ptpd,     0xFFFFU,     0x5FF5U, ~0U},
   {warn__rear_back,       0, &rear_ptpd_stuff, s_ptpd,     0xFFFFU,     0xD77DU, ~0U},
   // Have the centers rear back from a single 1/4 tag or 3/4 tag.
   {warn__awful_rear_back, 0, &rear_sqtag_stuff, sdmd,        0xFFU,       0x5FU, ~0U},
   {warn__awful_rear_back, 0, &rear_sqtag_stuff, sdmd,        0xFFU,       0xD7U, ~0U},
   // As above, but centers are "twisted".
   {warn__awful_rear_back, 0, &rear_twistqtag_stuff, s1x4,    0xFFU,       0x4EU, ~0U},
   {warn__awful_rear_back, 0, &rear_twist2x4c_stuff, s2x4,  0xFFFFU,     0x0820U, 0x3C3CU},
   {warn__awful_rear_back, 0, &rear_twist1x8c_stuff, s1x8,  0xFFFFU,     0x0220U, 0x3333U},
   {warn__rear_back,       0, &rear_3x1qtag_stuff, s3x1dmd, 0xFFFFU,     0x2088U, 0xFCFCU},
   {warn__rear_back,       0, &rear_1x3qtag_stuff,   s_323, 0xFFFFU,     0x0200U, 0x0303U},

   {warn__none,            0, (expand::thing *) 0, nothing}
};

full_expand::thing touch_init_table3[] = {
   {warn__some_touch, 0, &step_phan1_stuff,   s_c1phan, 0x33333333U, 0x13313113U, ~0U},
   {warn__some_touch, 0, &step_phan2_stuff,   s_c1phan, 0xCCCCCCCCU, 0x4C4CC4C4U, ~0U},
   {warn__some_touch, 0, &step_phan3_stuff,   s_c1phan, 0xCCCCCCCCU, 0x08808008U, ~0U},
   {warn__some_touch, 0, &step_phan4_stuff,   s_c1phan, 0x33333333U, 0x20200202U, ~0U},

   {warn__some_touch, 0, &step_4x4_1_stuff,   s4x4, 0x33333333U, 0x31311313U, ~0U},
   {warn__some_touch, 0, &step_4x4_2_stuff,   s4x4, 0x0F0F0F0FU, 0x0F0D0507U, ~0U},
   {warn__some_touch, 0, &step_4x4_3_stuff,   s4x4, 0x0F0F0F0FU, 0x0800020AU, ~0U},
   {warn__some_touch, 0, &step_4x4_4_stuff,   s4x4, 0x33333333U, 0x20020220U, ~0U},

   // Some people touch from horrible "T"'s.
   {warn__some_touch, 0, &step_bigd_stuff1,   sbigdmd,    0xFF0FF0U,   0x280820U, ~0U},
   {warn__some_touch, 0, &step_bigd_stuff2,   sbigdmd,    0x0FF0FFU,   0x082028U, ~0U},

   // Check for stepping to a tidal wave from lines facing.
   {warn__none,      16, &step_li_stuff,      s2x4,         0xFFFFU,     0xAA00U, ~0U},
   {warn__none,      16, &step_li6_stuff,     s2x3,          0xFFFU,      0xA80U, ~0U},
   {warn__none,   64+16, &step_spindle_stuff, s_spindle,    0xFFFFU,     0xA800U, 0xFDFDU},
   // Same, with missing people.
   {warn__none,      16, &step_li_stuff,      s2x4,         0xC3C3U,     0x8200U, ~0U},
   {warn__none,      16, &step_li_stuff,      s2x4,         0x3C3CU,     0x2800U, ~0U},
   {warn__none,      16, &step_li_stuff,      s2x4,         0x0FF0U,     0x0A00U, ~0U},
   {warn__none,      16, &step_li_stuff,      s2x4,         0xF00FU,     0xA000U, ~0U},

   // Check for stepping to a wave from partially occupied lines facing, only beaus present.
   {warn__none,     128, &step_liphan_stuff,  s2x4,         0x3333U,     0x2200U, ~0U},

   // Check for stepping to an alamo wave from thar-like facing people.
   {warn__none,      16, &step_thar_stuff,    s_thar,       0xFFFFU,     0x78D2U, ~0U},

   // Check for stepping to a bone from a squared set or whatever.
   {warn__none,      16, &step_bn_stuff,      s2x4,         0xFFFFU,     0x6941U, 0x7D7DU},

   // Check for stepping to a bone6 from a 2x3.
   {warn__none,       0, &step_bn23_stuff,    s2x3,          07777U,      03121U,  03535U},

   // Check for centers stepping to a column from a bone.
   {warn__none,       0, &step_24bn_stuff,    s_bone,       0xFFFFU,     0x5D57U, 0x5F5FU},

   // Check for centers stepping to a column of 6 from a bone6.
   {warn__none,       0, &step_23bn_stuff,    s_bone6,       07777U,      02725U,  02727U},

   // Check for some people facing after a "1/2 make magic".
   {warn__may_be_fudgy,0,&step_232a_stuff,    s_23232,       0x003F33F3U, 0x001733D1U, 0x003F33F3U},
   {warn__may_be_fudgy,0,&step_232b_stuff,    s_23232,       0x00C3FC3FU, 0x0041DC37U, 0x00C3FC3FU},

   // Check for stepping to rigger from suitable T-bone.
   {warn__some_touch,16, &step_2x4_rig_stuff, s2x4,       0xFFFFU,     0x963CU, ~0U},
   {warn__none,       0, &step_offs1_stuff,   s3x4,     0x0FF0FFU,   0x07D0D7U, ~0U},
   {warn__none,       0, &step_offs2_stuff,   s3x4,     0xF0FF0FU,   0x70DD07U, ~0U},

   // Triangle base, who are facing, touch.
   {warn__some_touch, 0, &step_tgl4_stuffa,   s_trngl4,     0xFFU,       0xD7U, ~0U},
   // Same, with missing people.
   {warn__some_touch, 0, &step_tgl4_stuffa,   s_trngl4,     0xF0U,       0xD0U, ~0U},
   {warn__some_touch, 0, &step_tgl4_stuffa,   s_trngl4,     0x0FU,       0x07U, ~0U},

   // Triangle apexes, who are facing, touch.
   {warn__some_touch, 0, &step_tgl4_stuffb,   s_trngl4,     0xFFU,       0x22U, ~0U},
   // Same, with missing people.
   {warn__some_touch, 0, &step_tgl4_stuffb,   s_trngl4,     0xF0U,       0x20U, ~0U},
   {warn__some_touch, 0, &step_tgl4_stuffb,   s_trngl4,     0x0FU,       0x02U, ~0U},

   // Ends touch from a bone to a tidal wave.
   {warn__some_touch, 0, &step_bone_stuff,    s_bone,     0xFFFFU,     0xA802U, 0xFFFFU},
   {warn__some_touch, 0, &step_bone_stuff,    s_bone,     0xFFFFU,     0xA208U, 0xFFFFU},
   // Same, but we get a 3&1 or inverted line.
   {warn__some_touch, 32, &step_bone_stuff,   s_bone,     0xFFFFU,     0xAA00U, 0xFFFFU},
   {warn__some_touch, 32, &step_bone_stuff,   s_bone,     0xFFFFU,     0xA00AU, 0xFFFFU},

   // All touch from a bone to a rigger.
   {warn__none,       0, &step_bone_rigstuff, s_bone,     0xFFFFU,     0xAD07U, 0xFFFFU},
   // Same, with missing people.
   {warn__none,       0, &step_bone_rigstuff, s_bone,     0xF0F0U,     0xAD07U, 0xF0F0U},
   {warn__none,       0, &step_bone_rigstuff, s_bone,     0x0F0FU,     0xAD07U, 0x0F0FU},

   // All touch from a suitable qtag to a rigger.
   {warn__none,       0, &step_qtag_rigstuff, s_qtag,     0xFFFFU,     0x7DD7U, 0xFFFFU},
   // Same, with missing people.
   {warn__none,       0, &step_qtag_rigstuff, s_qtag,     0xF0F0U,     0x7DD7U, 0xF0F0U},
   {warn__none,       0, &step_qtag_rigstuff, s_qtag,     0x0F0FU,     0x7DD7U, 0x0F0FU},

   // All touch from a rigger to a bone.
   {warn__none,       0, &step_rig_bonestuff, s_rigger,   0xFFFFU,     0xAD07U, 0xFFFFU},
   // Same, with missing people.
   {warn__none,       0, &step_rig_bonestuff, s_rigger,   0xF0F0U,     0xAD07U, 0xF0F0U},
   {warn__none,       0, &step_rig_bonestuff, s_rigger,   0x0F0FU,     0xAD07U, 0x0F0FU},

   // Centers touch from a "rigger" to a tidal wave.
   {warn__some_touch, 0, &step_rig_stuff,     s_rigger,   0xFFFFU,     0xA802U, 0xFFFFU},
   // Same, but we get a 3&1 or inverted line, from which fan the top is legal.
   {warn__some_touch, 32, &step_rig_stuff,    s_rigger,   0xFFFFU,     0xA00AU, 0xFFFFU},

   // Same, with missing people.
   {warn__some_touch, 0, &step_rig_stuff,     s_rigger,   0xF0F0U,     0xA802U, 0xF0F0U},
   {warn__some_touch, 0, &step_rig_stuff,     s_rigger,   0x0F0FU,     0xA802U, 0x0F0FU},

   // Center 2 touch from a 1x2dmd to a wave of 6.
   {warn__some_touch, 0, &step_2x1d_stuff,    s_1x2dmd,   0xFFFU,      0x2A0U,  0xFFFU},

   // Center 4 touch from a short6 to a wave of 6.
   {warn__some_touch, 0, &step_short6_stuff,  s_short6,   0xFFFU,      0x7F5U,  0xFFFU},

   // Check for stepping to a miniwave from people facing.
   {warn__none,       0, &step_1x2_stuff,     s1x2,          0xFU,        0x7U, 0xFU},

   // Check for stepping to a box from a 1x4 single 8 chain -- we allow some phantoms.
   // This is what makes triple columns turn and weave legal in certain interesting cases.
   {warn__none,       0, &step_1x4_stuff,     s1x4,         0xFFU,       0x7DU, 0xFFU},
   {warn__none,       0, &step_1x4_stuff,     s1x4,         0xF0U,       0x7DU, 0xF0U},
   {warn__none,       0, &step_1x4_stuff,     s1x4,         0x0FU,       0x7DU, 0x0FU},

   // Check for stepping to a single 1/4 tag or 3/4 tag from a single-file DPT or trade-by --
   // we allow some phantoms, as above.
   {warn__none,       0, &step_1x4_side_stuff, s1x4,        0xFFU,       0xD7U, 0xFFU},
   {warn__none,       0, &step_1x4_side_stuff, s1x4,        0xFFU,       0x5FU, 0xFFU},
   {warn__none,       0, &step_1x4_side_stuff, s1x4,        0x33U,       0x13U, 0x33U},

   // Check for stepping to a column from a 1x8 single 8 chain.
   {warn__none,       0, &step_1x8_stuff,      s1x8,      0xFFFFU,     0x7DD7U, 0xFFFFU},
   // Same, with missing people.
   {warn__none,       0, &step_1x8_stuff,      s1x8,      0xF0F0U,     0x7DD7U, 0xF0F0U},
   {warn__none,       0, &step_1x8_stuff,      s1x8,      0x0F0FU,     0x7DD7U, 0x0F0FU},

   // Check for stepping to parallel waves from an 8 chain.
   {warn__none,       0, &step_8ch_stuff,      s2x4,      0xFFFFU,     0x77DDU, 0xFFFFU},
   {warn__none,       0, &step_8ch_stuff,      s2x4,      0xF0F0U,     0x77DDU, 0xF0F0U},
   {warn__none,       0, &step_8ch_stuff,      s2x4,      0x0F0FU,     0x77DDU, 0x0F0FU},
   {warn__none,       0, &step_8ch_stuff,      s2x4,      0x0FF0U,     0x77DDU, 0x0FF0U},
   {warn__none,       0, &step_8ch_stuff,      s2x4,      0xF00FU,     0x77DDU, 0xF00FU},

   // Ends touch from diamonds to thar.
   {warn__none,       8, &step_8by_stuff,      s_qtag,    0xFFFFU,     0x78D2U, ~0U},

   // Touch from alamo (as 4x4) 8-chain to thar.
   {warn__none,       8, &rear_thar_stuff,     s4x4,  0x3C3C3C3CU, 0x2034081CU, ~0U},

   // Touch from genuine alamo 8-chain to thar.
   {warn__none,       8, &rear_alamo_stuff,    s_alamo,   0xFFFFU,     0x78D2U, ~0U},

   // Touch from "general lines with centers 1/4ed in" 2x4 to qtag.
   // Touch from "facing lines with centers 1/4ed in" 2x4 to thar.
   // These two must be in the order shown.
   {warn__none,    64+8, &step_sqsctr_stuff,   s2x4,      0xFFFFU,     0x1C34U, 0x3C3CU},
   {warn__none,       8, &step_sqs_stuff,      s2x4,      0xFFFFU,     0x9E34U, ~0U},
   {warn__none,    64+8, &step_1x8ctr_stuff,   s1x8,      0xFFFFU,     0x1331U, 0x3333U},
   {warn__none,    64+8, &step_1x4ctr_stuff,   s1x4,        0xFFU,       0x13U, 0x33U},

   // Same, with different orientation of centers.
   // The 32 bit says these are allowed only for fan the top.
   {warn__some_touch,64+32+8,&step_qtctr_stuff, s_qtag,   0xFFFFU,     0x0D07U, 0x0F0FU},
   {warn__none,    64+32+8, &step_ptpctr_stuff, s_ptpd,   0xFFFFU,     0x2002U, 0x3333U},
   {warn__some_touch,64+32+8, &step_dmdctr_stuff, sdmd,     0xFFU,       0x20U, 0x33U},

   {warn__none,       0, (expand::thing *) 0, nothing}
};


// For elongrotallow, the bits are as follows:
// 800 bit: don't allow if setups have 1/8 rotation on; most maps have this
// 400 bit: don't allow if setups have 1/8 rotation off; for special 1/8 stuff.
// 200 bit: don't allow for analyze; that is, you can come out, but you can't go in.
// 100 bit: don't allow for synthesize; that is, you can go in, but you can't come out.
// 80 bit: forbid synthesize if came from schema_rev_checkpoint_concept.
// 40 bit: forbid synthesize if matrix concept was given.
// Low 4 bits forbid synthesize if
//   8: outside elongation is vertical and rotation different
//   4: outside elongation is vertical and rotation same
//   2: outside elongation is horizontal and rotation different
//   1: outside elongation is horizontal and rotation same
// 20 bit: forbid synthesize if outer_elongation = 3 and rotation different
// 10 bit: forbid synthesize if outer_elongation = 3 and rotation same

// First line:
//   bigsetup    lyzer      ... maps .............
// Second line:
//                    mapelong --------|  |---- center_arity
//                   outer_rot -----|  |  |   |---- elongrotallow
//               inner_rot ------|  |  |  |   |      |---- getout_schema
//          insetup  outsetup    |  |  |  |   |      |

conc_tables::cm_thing conc_tables::conc_init_table[] = {
   // These two for unsymmetrical strip.
   {s1x4dmd,        schema_nothing, {0, 1, 4, 5, 6, 9,    2, 3, 7, 8},
             s_1x2dmd, s1x4,     0, 0, 9, 1,  0xA7A, schema_rev_checkpoint},
   {s1x10,          schema_nothing, {0, 1, 4, 5, 6, 9,    2, 3, 7, 8},
             s1x6,     s1x4,     0, 0, 9, 1,  0xA7A, schema_rev_checkpoint},
   {s_ptpd,         schema_nothing, {0, 2, 4, 6,    3, 1, 7, 5},
             s1x4,     s2x2,     0, 1, 9, 1,  0xA75, schema_rev_checkpoint},
   {s_spindle,      schema_nothing, {7, 1, 3, 5,    6, 0, 2, 4},
             sdmd,     s2x2,     0, 1, 1, 1,  0xA75, schema_rev_checkpoint},
   {s_wingedstar,   schema_nothing, {0, 3, 4, 7,    1, 2, 5, 6},
             sdmd,     s1x4,     0, 0, 9, 1,  0xAFA, schema_ckpt_star},
   {s_wingedstar,   schema_nothing, {1, 2, 5, 6,    7, 0, 3, 4},
             s1x4,     sdmd,     0, 1, 9, 1,  0xAF5, schema_conc_star},
   {s_wingedstar,   schema_nothing, {1, 2, 5, 6,    0, 3, 4, 7},
             s1x4,     sdmd,     0, 0, 9, 1,  0xAFA, schema_conc_star},
   {s_wingedstar,   schema_nothing, {1, 2, 5, 6,    7, 0, 3, 4},
             s1x4,     s_star,   0, 1, 9, 1,  0xAF5, schema_conc_star},
   {s_wingedstar,   schema_nothing, {1, 2, 5, 6,    0, 3, 4, 7},
             s1x4,     s_star,   0, 0, 9, 1,  0xAFA, schema_conc_star},
   {s_wingedstar12, schema_nothing, {11, 2, 3, 4, 9, 10, 5, 8,     0, 1, 6, 7},
             s_star,   s1x4,     1, 0, 1, 2,  0xAF5, schema_conc_star12},
   {s_wingedstar16, schema_nothing, {4, 2, 3, 5, 15, 6, 7, 14, 11, 13, 12, 10,    0, 1, 8, 9},
             s_star,   s1x4,     1, 0, 1, 3,  0xAF5, schema_conc_star16},
   {s_barredstar,   schema_nothing, {6, 9, 1, 4,    7, 8, 0, 2, 3, 5},
             s_star,   s2x3,     1, 1, 2, 1,  0xAFB, schema_concentric},
   {s2x4,           schema_nothing, {6, 5, 1, 2,      7, 0, 3, 4},
             s1x2,     s2x2,     0, 1, 1, 2,  0xAF7, schema_concentric_others},
   {s_qtag,         schema_nothing, {6, 7, 3, 2,      5, 0, 1, 4},
             s1x2,     s2x2,     0, 1, 1, 2,  0xAFD, schema_concentric_others},
   {s2x6,           schema_nothing, {11, 0, 1, 10, 7, 4, 5, 6,           9, 2, 3, 8},
             s2x2,     s2x2,     1, 1, 2, 2,  0xAFB, schema_in_out_triple},
   {s2x9,           schema_nothing, {0, 1, 2, 15, 16, 17, 6, 7, 8, 9, 10, 11,    3, 4, 5, 12, 13, 14},
             s2x3,     s2x3,     0, 0, 2, 2,  0xAFA, schema_in_out_triple},
   {sbigdmd,        schema_nothing, {0, 1, 10, 11, 4, 5, 6, 7,           8, 9, 2, 3},
             s2x2,     s1x4,     0, 1, 2, 2,  0xAF7, schema_in_out_triple},
   {s_hrglass,      schema_nothing, {5, 0, 1, 4,       6, 3, 2, 7},
             s2x2,     sdmd,     1, 0, 1, 1,  0xAF7, schema_in_out_triple},
   {s_dhrglass,     schema_nothing, {0, 1, 4, 5,       6, 3, 2, 7},
             s2x2,     sdmd,     0, 0, 1, 1,  0xAFE, schema_in_out_triple},
   {sbigdhrgl,      schema_nothing, {11, 0, 1, 10, 7, 4, 5, 6,           9, 2, 3, 8},
             s2x2,     sdmd,     1, 0, 1, 2,  0xAFD, schema_in_out_triple},
   {sbighrgl,       schema_nothing, {11, 0, 1, 10, 7, 4, 5, 6,           8, 9, 2, 3},
             s2x2,     sdmd,     1, 1, 1, 2,  0xAFB, schema_in_out_triple},
   {s4x4,           schema_nothing, {8, 6, 4, 5, 12, 13, 0, 14,          11, 15, 3, 7},
             s1x4,     s2x2,     0, 1, 2, 2,  0xAFD, schema_in_out_triple},
   {s2x4,           schema_nothing, {7, 0, 4, 3,           6, 1, 2, 5},
             s1x2,     s2x2,     1, 1, 1, 2,  0xAFB, schema_in_out_triple},
   {s2x4,           schema_in_out_triple, {7, 0, 4, 3,           1, 2, 5, 6},
             s1x2,     s2x2,     1, 0, 1, 2,  0x8FD, schema_in_out_triple},
   {s_crosswave,    schema_in_out_triple, {0, 1, 5, 4,      6, 7, 2, 3},
             s1x2,     s1x4,     0, 1, 1, 2,  0x8F7, schema_in_out_triple},
   {s1x3dmd,        schema_in_out_triple, {-1, 0, -1, 1, -1, 5, -1, 4,     2, 3, 6, 7},
             sdmd,     sdmd,     1, 0, 1, 2,  0x9FD, schema_nothing},
   {s_3mdmd,        schema_in_out_triple, {8, 9, 0, 10, 6, 4, 2, 3,       11, 1, 5, 7},
             sdmd,     sdmd,     1, 0, 1, 2,  0x8FD, schema_in_out_triple},
   {s_4mdmd,        schema_in_out_quad, {11, 12, 0, 13, 8, 5, 3, 4,
                                         14, 1, 15, 10, 6, 9, 7, 2},
             sdmd,     s_ptpd,   1, 0, 1, 2,  0x8FD, schema_in_out_quad},
   {s_3mptpd,       schema_in_out_triple, {9, 0, 10, 8, 4, 2, 3, 6,       7, 11, 1, 5},
             sdmd,     sdmd,     0, 1, 1, 2,  0x8F7, schema_in_out_triple},
   {s_4mptpd,       schema_in_out_quad, {12, 0, 13, 11, 5, 3, 4, 8,
                                         1, 2, 6, 7, 9, 10, 14, 15},
             sdmd,     s_qtag,   0, 0, 1, 2,  0x8FE, schema_in_out_quad},
   {s3x4,           schema_nothing, {9, 8, 6, 7, 0, 1, 3, 2,             10, -1, 4, -1},
             s1x4,     sdmd,     0, 0, 2, 2,  0xAFB, schema_in_out_triple},
   {s3dmd,          schema_nothing, {8, -1, 0, -1, 6, -1, 2, -1,         7, 11, 1, 5},
             s1x4,     sdmd,     1, 1, 1, 2,  0xAFB, schema_in_out_triple},
   {s_3mdmd,        schema_nothing, {8, -1, 0, -1, 6, -1, 2, -1,         11, 1, 5, 7},
             s1x4,     sdmd,     1, 0, 1, 2,  0xAFD, schema_in_out_triple},
   {s_3mptpd,       schema_nothing, {9, -1, 10, -1, 4, -1, 3, -1,        7, 11, 1, 5},
             s1x4,     sdmd,     0, 1, 1, 2,  0xAF7, schema_in_out_triple},
   {s3x1dmd,        schema_nothing, {-1, -1, 1, 0, 5, 4, -1, -1,         7, 2, 3, 6},
             s1x4,     sdmd,     0, 1, 1, 2,  0xAF7, schema_in_out_triple},

   {s3x1dmd,        schema_intermediate_diamond, {7, 1, 3, 5,         0, 2, 4, 6},
             sdmd,     s1x4,     1, 0, 1, 1,  0x8FD, schema_intermediate_diamond},
   {s3x1dmd,        schema_outside_diamond, {7, 0, 3, 4,         1, 2, 5, 6},
             sdmd,     s1x4,     1, 0, 1, 1,  0x8FD, schema_outside_diamond},

   {s_wingedstar,   schema_intermediate_diamond, {1, 3, 5, 7,         0, 2, 4, 6},
             sdmd,     s1x4,     0, 0, 1, 1,  0x8FE, schema_intermediate_diamond},
   {s_wingedstar,   schema_outside_diamond, {0, 3, 4, 7,         1, 2, 5, 6},
             sdmd,     s1x4,     0, 0, 1, 1,  0x8FE, schema_outside_diamond},

   // For putting back shape-changers.
   {s1x3dmd,        schema_intermediate_diamond, {1, 3, 5, 7,         0, 2, 4, 6},
             sdmd,     s1x4,     0, 0, 1, 1,  0x8FE, schema_intermediate_diamond},
   {s1x3dmd,        schema_outside_diamond, {0, 3, 4, 7,         1, 2, 5, 6},
             sdmd,     s1x4,     0, 0, 1, 1,  0x8FE, schema_outside_diamond},
   {s_crosswave,    schema_nothing, {6, 7, 2, 3,         0, 1, 4, 5},
             s1x4,     s1x4,     1, 0, 1, 1,  0x8FD, schema_intermediate_diamond},
   {s_crosswave,    schema_nothing, {6, 7, 2, 3,         0, 1, 4, 5},
             s1x4,     s1x4,     1, 0, 1, 1,  0x8FD, schema_outside_diamond},
   {s_ptpd,         schema_nothing, {1, 7, 5, 3,         0, 2, 4, 6},
             s2x2,     s1x4,     0, 0, 1, 1,  0xAFE, schema_intermediate_diamond},
   {s_bone,         schema_nothing, {0, 1, 4, 5,         6, 7, 2, 3},
             s2x2,     s1x4,     0, 0, 1, 1,  0xAFE, schema_outside_diamond},
   {s1x8,           schema_nothing, {1, 3, 5, 7,         0, 2, 4, 6},
             s1x4,     s1x4,     0, 0, 1, 1,  0xAFE, schema_intermediate_diamond},
   {s1x8,           schema_nothing, {0, 1, 4, 5,         3, 2, 7, 6},
             s1x4,     s1x4,     0, 0, 1, 1,  0xAFE, schema_outside_diamond},

   {s3ptpd,         schema_nothing, {9, -1, 10, -1, 4, -1, 3, -1,        11, 1, 5, 7},
             s1x4,     sdmd,     0, 0, 1, 2,  0xAFE, schema_in_out_triple},
   {s3dmd,          schema_in_out_triple, {8, 9, 0, 10, 6, 4, 2, 3,         7, 11, 1, 5},
             sdmd,     sdmd,     1, 1, 1, 2,  0x8FB, schema_in_out_triple},
   {s_dhrglass,     schema_in_out_triple, {-1, 0, 6, 5, 2, 1, -1, 4,       -1, 7, -1, 3},
             sdmd,     s1x4,     0, 1, 1, 2,  0x8F7, schema_in_out_triple},
   {sd2x7,          schema_in_out_triple, {0, 1, 12, 13, 5, 6, 7, 8,       4, 3, 2, 11, 10, 9},
             s2x2,     s2x3,     0, 1, 2, 2,  0x8F7, schema_in_out_triple},
   {s2x7,           schema_in_out_triple, {0, 1, 12, 13, 5, 6, 7, 8,       2, 3, 4, 9, 10, 11},
             s2x2,     s2x3,     0, 0, 1, 2,  0x8FE, schema_in_out_triple},

   {s4dmd,          schema_in_out_quad, {11, 12, 0, 13, 8, 5, 3, 4,
                         1, 2, 6, 7, 9, 10, 14, 15},
             sdmd,     s_qtag,   1, 0, 1, 2,  0x8FD, schema_in_out_quad},
   // This one must not be used for analysis ("going in").  The reason is that we go in
   // with stuff separated left-to-right.  But this is top-to-bottom.  Wasn't the
   // elongation supposed to take care of all this????
   {s3x8,           schema_nothing, {17, 16, 14, 15, 2, 3, 5, 4,
                         20, 21, 23, 22, 8, 9, 11, 10},
             s1x4,     s1x8,     0, 0, 2, 2,  0xAFB, schema_in_out_quad},
   {s3x8,           schema_nothing, {19, 20, 0, 1, 21, 18,    13, 9, 6, 7, 8, 12,    22, 23, 10, 11},
             s2x3,     s1x4,     1, 0, 1, 2,  0xAFD, schema_in_out_triple},
   {s_343,          schema_nothing, {7, 6, 5, -1, -1, -1, -1, -1, -1, 2, 1, 0,    8, 9, 3, 4},
             s2x3,     s1x4,     0, 0, 2, 2,  0xAFB, schema_in_out_triple},

   {s_crosswave,    schema_nothing, {-1, 0, -1, -1, 1, -1,    -1, 5, -1, -1, 4, -1,    6, 7, 2, 3},
             s2x3,     s1x4,     1, 1, 1, 2,  0xAFB, schema_in_out_triple},
   {s3ptpd,         schema_in_out_triple, {9, 0, 10, 8, 4, 2, 3, 6,    11, 1, 5, 7},
             sdmd,     sdmd,     0, 0, 1, 2,  0x8FE, schema_in_out_triple},
   {s4ptpd,         schema_in_out_quad, {12, 0, 13, 11, 5, 3, 4, 8,
                                         14, 1, 15, 10, 6, 9, 7, 2},
             sdmd,     s_ptpd,   0, 0, 1, 2,  0x8FE, schema_in_out_quad},
   {s_crosswave,    schema_nothing, {-1, 0, -1, 1, -1, 5, -1, 4,     6, 7, 2, 3},
             sdmd,     s1x4,     1, 1, 1, 2,  0xAFB, schema_in_out_triple},
   {s1x8,           schema_nothing, {-1, 0, -1, 1, -1, 5, -1, 4,     3, 2, 7, 6},
             sdmd,     s1x4,     1, 0, 1, 2,  0xAFD, schema_in_out_triple},
   {s1x8,           schema_nothing, {0, 1,     5, 4,     3, 2, 7, 6},
             s1x2,     s1x4,     0, 0, 1, 2,  0xAFA, schema_in_out_triple},
   {s_rigger,       schema_nothing, {-1, 6, -1, 7, -1, 3, -1, 2,     0, 1, 4, 5},
             sdmd,     s2x2,     1, 0, 1, 2,  0xAF5, schema_in_out_triple},
   {s_rigger,       schema_nothing, {-1, 6, -1, -1, 7, -1, -1, 3, -1, -1, 2, -1,       0, 1, 4, 5},
             s2x3,     s2x2,     1, 0, 1, 2,  0xAFD, schema_in_out_triple},
   // Next two must be in the order shown.  Remember that they get reversed.
   // This one works with the "matrix" concept.
   {s4x4,           schema_nothing, {8, 9, 12, 10, 4, 2, 0, 1,    10, 15, 3, 1, 2, 7, 11, 9},
             s1x4,     s2x4,     1, 0, 1, 2,  0xABD, schema_in_out_quad},
   // This one doesn't.
   {s4x6,           schema_nothing, {17, 18, 0, 11, 12, 23, 5, 6,    10, 9, 8, 7, 22, 21, 20, 19},
             s1x4,     s2x4,     1, 0, 1, 2,  0xAFD, schema_in_out_quad},
   {s4x5,           schema_nothing, {14, 15, 0, 9, 10, 19, 4, 5,    16, 8, 2, 7, 6, 18, 12, 17},
             s1x4,     s_qtag,   1, 1, 1, 2,  0xAFB, schema_in_out_quad},
   // We would really like to have this raise the warning "warn__check_4x6" when
   // this is used, but there is no straightforward mechanism for doing this.
   // The warning "warn__check_4x6" was created specifically for this, and
   // is currently unused.
   {s4x6,           schema_nothing, {17, -1, 0, -1, 12, -1, 5,-1,    10, 9, 8, 7, 22, 21, 20, 19},
             sdmd,     s2x4,     1, 0, 1, 2,  0xAFD, schema_in_out_quad},

   {s4x6,           schema_nothing, {15, 20, 9, 2, 3, 8, 21, 14,
                                     5, 6, 23, 7, 12, 13, 16, 22,
                                     17, 18, 11, 19, 0, 1, 4, 10},
             s2x4,     s4x4,     1, 0, 1, 1,  0xAFD, schema_concentric},

   {sbigrig,        schema_nothing, {0, 1, 3, 2, 9, 8, 6, 7,    -1, 11, 4, -1, -1, 5, 10, -1},
             s1x4,     s2x4,     0, 1, 1, 2,  0xAF7, schema_in_out_quad},
   {s4x6,           schema_nothing, {18, 11, 10, 19, 22, 7, 6, 23,
                                     15, 20, 9, 2, 3, 8, 21, 14},
             s2x2,     s2x4,     1, 1, 1, 2,  0xAFB, schema_in_out_quad},
   {s4x6,           schema_nothing, {11, 10, 19, 18, 7, 6, 23, 22,
                                     15, 20, 9, 2, 3, 8, 21, 14},
             s2x2,     s2x4,     0, 1, 1, 2,  0xAF7, schema_in_out_quad},
   {s_qtag,         schema_nothing, {6, 7, 2, 3,
                        -1, 0, 1, -1, -1, -1, -1, -1,    -1, 4, 5, -1, -1, -1, -1, -1},
             s1x4,     s4x4,     0, 1, 1, 1,  0xAFD, schema_conc_o},
   {s_bone,         schema_nothing, {6, 7, 2, 3,
                        -1, -1, -1, -1, -1, 1, 4, -1, -1, -1, -1, -1, -1, 5, 0, -1},
             s1x4,     s4x4,     0, 1, 1, 1,  0xAF7, schema_conc_o},
   {s_hrglass,      schema_nothing, {6, 3, 2, 7,
                        -1, 0, 1, -1, -1, -1, -1, -1, -1, 4, 5, -1, -1, -1, -1, -1},
             sdmd,     s4x4,     0, 1, 1, 1,  0xAFD, schema_conc_o},
   {s_dhrglass,     schema_nothing, {6, 3, 2, 7,
                        -1, -1, -1, -1, -1, 1, 4, -1, -1, -1, -1, -1, -1, 5, 0, -1},
             sdmd,     s4x4,     0, 1, 1, 1,  0xAF7, schema_conc_o},
   {s_bone,         schema_nothing, {6, 7, 2, 3,    -1, 0, 1, -1, -1, 4, 5, -1},
             s1x4,     s2x4,      0, 0, 1, 1,  0xAFE, schema_concentric},
   {sbigdmd,        schema_nothing, {8, 9, 2, 3,    0, 1, 4, 5, 6, 7, 10, 11},
             s1x4,     s2x4,     1, 0, 1, 1,  0xAFD, schema_concentric},
   {s3x4,           schema_nothing, {10, 11, 4, 5,    0, 1, 2, 3, 6, 7, 8, 9},
             s1x4,     s2x4,     0, 0, 1, 1,  0xAFA, schema_concentric},
   {s3x6,           schema_nothing, {15, 16, 17, 6, 7, 8,    0, 1, 2, 3, 4, 5, 9, 10, 11, 12, 13, 14},
             s1x6,     s2x6,     0, 0, 1, 1,  0xAFB, schema_concentric},
   {s3x6,           schema_nothing, {12, 17, 2, 3, 8, 11,    15, 16, 6, 7},
             s2x3,     s1x4,     1, 0, 1, 1,  0xADD, schema_concentric},
   {s4x4,           schema_nothing, {10, 15, 3, 1, 2, 7, 11, 9,     13, 14, 5, 6},
             s2x4,     s2x2,     0, 0, 2, 1,  0xAFB, schema_concentric},
   {s4x4,           schema_nothing, {11, 15, 3, 7,    12, 13, 14, 0, 4, 5, 6, 8},
             s2x2,     s2x4,     1, 0, 2, 1,  0xAF7, schema_concentric},
   {s4x4,           schema_nothing, {15, 3, 7, 11,     12, 13, 14, 0, 4, 5, 6, 8},
             s2x2,     s2x4,     0, 0, 2, 1,  0xAFB, schema_concentric},
   {s2x6,           schema_nothing, {9, 2, 3, 8,    0, 1, 4, 5, 6, 7, 10, 11},
             s2x2,     s2x4,     1, 0, 9, 1,  0xAFD, schema_concentric},
   {s2x6,           schema_nothing, {2, 3, 8, 9,    0, 1, 4, 5, 6, 7, 10, 11},
             s2x2,     s2x4,     0, 0, 9, 1,  0xAFE, schema_concentric},
   {s2x6,           schema_nothing, {1, 2, 3, 4, 7, 8, 9, 10,   0, 5, 6, 11},
             s2x4,     s2x2,     0, 0, 9, 1,  0xAFE, schema_concentric},
   {s3dmd,          schema_3x3_concentric, {9, 10, 11, 3, 4, 5,    0, 1, 2, 6, 7, 8},
             s1x6,     s2x3,     0, 0, 2, 1,  0x8FB, schema_concentric},
   // Took out the analyzer part of this -- it is wrong and is doing damage.
   {s3x4,           schema_nothing, {11, 5,     0, 1, 2, 3, 6, 7, 8, 9},
             s1x2,     s2x4,     0, 0, 2, 1,  0xAFB, schema_concentric},
   // and added this one
   {s3x4,           schema_concentric, {8, 11, 1, 2, 5, 7,      0, 3, 6, 9},
             s2x3,     s2x2,     1, 0, 3, 1,  0x8DF, schema_concentric},
   {s3x8,           schema_3x3_concentric, {21, 22, 23, 9, 10, 11,      19, 20, 0, 7, 8, 12},
             s1x6,     s2x3,     0, 1, 1, 1,  0x8F7, schema_concentric},
   {s3x8,           schema_in_out_triple, {0, 1, 2, 3, 4, 5, 6, 7,
                                 12, 13, 14, 15, 16, 17, 18, 19,
                                 20, 21, 23, 22, 8, 9, 11, 10},
             s2x8, s1x8,         0, 0, 0, 1,  0x8FA, schema_in_out_triple},
   {s4dmd,          schema_in_out_triple, {0, 1, 2, 3, 8, 9, 10, 11,     12, 13, 15, 14, 4, 5, 7, 6},
             s2x4,   s1x8,       0, 0, 0, 1,  0x8FA, schema_in_out_triple},
   {s4dmd,          schema_4x4_cols_concentric, {12, 13, 15, 14, 4, 5, 7, 6,    0, 1, 2, 3, 8, 9, 10, 11},
             s1x8,     s2x4,     0, 0, 2, 1,  0x8FB, schema_concentric},
   {s4dmd,          schema_4x4_lines_concentric, {12, 13, 15, 14, 4, 5, 7, 6,    0, 1, 2, 3, 8, 9, 10, 11},
             s1x8,     s2x4,     0, 0, 2, 1,  0x900, schema_nothing},
   {sbigrig,        schema_nothing, {11, 4, 5, 10,     0, 1, 3, 2, 6, 7, 9, 8},
             s2x2,     s1x8,     1, 0, 9, 1,  0xAF5, schema_concentric},
   {sbigrig,        schema_nothing, {4, 5, 10, 11,     0, 1, 3, 2, 6, 7, 9, 8},
             s2x2,     s1x8,     0, 0, 9, 1,  0xAFA, schema_concentric},
   {s5x1dmd,        schema_nothing, {11, 4, 5, 10,     0, 1, 3, 2, 6, 7, 9, 8},
             sdmd,     s1x8,     1, 0, 9, 1,  0xAF5, schema_concentric},
   {s1x5dmd,        schema_nothing, {4, 5, 10, 11,     0, 1, 3, 2, 6, 7, 9, 8},
             sdmd,     s1x8,     0, 0, 9, 1,  0xAFA, schema_concentric},
   {s_bone6,        schema_nothing, {5, 2,    4, 0, 1, 3},
             s1x2,     s2x2,     0, 1, 2, 1,  0xAF7, schema_concentric},
   {s_bone6,        schema_concentric_6p, {5, 2,    0, 1, 3, 4},
             s1x2,     s2x2,     0, 0, 1, 1,  0x8FE, schema_concentric},
   {s_bone6,        schema_concentric_2_4, {5, 2,    0, 1, 3, 4},
             s1x2,     s2x2,     0, 0, 1, 1,  0x8FE, schema_concentric},
   {s1x6,           schema_concentric_2_4, {2, 5,    0, 1, 3, 4},
             s1x2,     s1x4,     0, 0, 1, 1,  0x8FE, schema_concentric},
   {s2x3,           schema_concentric, {4, 1,    5, 0, 2, 3},
             s1x2,     s2x2,     1, 1, 2, 1,  0x8FB, schema_concentric},
   {s_1x2dmd,       schema_concentric_6p, {5, 2,    0, 1, 3, 4},
             s1x2,     s1x4,     1, 0, 1, 1,  0x8F5, schema_concentric},
   {s_1x2dmd,       schema_concentric_2_4, {5, 2,    0, 1, 3, 4},
             s1x2,     s1x4,     1, 0, 1, 1,  0x8F5, schema_concentric},
   {s_2x1dmd,       schema_concentric_2_4, {1, 4,    0, 2, 3, 5},
             s1x2,     s_star,   0, 0, 1, 1,  0x8FA, schema_concentric},
   {s_2x1dmd,       schema_concentric_2_4, {1, 4,    5, 0, 2, 3},
             s1x2,     s_star,   0, 1, 1, 1,  0x8F5, schema_concentric},
   {s_1x2dmd,       schema_nothing, {1, 2, 4, 5,    0, 3},
             sdmd,     s1x2,     0, 0, 1, 1,  0xAFA, schema_concentric},
   {s1x6,           schema_nothing, {2, 5,    0, 1, 3, 4},
             s1x2,     s1x4,     0, 0, 1, 1,  0xAFA, schema_concentric},
   {s1x6,           schema_concentric_4_2, {1, 2, 4, 5,    0, 3},
             s1x4,     s1x2,     0, 0, 1, 1,  0x8FA, schema_concentric},

   {s_2x1dmd,       schema_concentric_4_2_prefer_1x4, {0, 1, 3, 4,    5, 2},
             s1x4,     s1x2,     0, 1, 1, 1,  0x8F5, schema_concentric},
   {s_2x1dmd,       schema_concentric_4_2, {5, 1, 2, 4,    0, 3},
             sdmd,     s1x2,     1, 0, 1, 1,  0x8F5, schema_concentric},

   // Next two are duplicate.
   {s_wingedstar,   schema_nothing, {7, 2, 3, 6,    0, 1, 4, 5},
             s_star,   s1x4,     1, 0, 1, 1,  0xAF5, schema_conc_star},
   {s_wingedstar,   schema_nothing, {7, 2, 3, 6,    0, 1, 4, 5},
             s_star,   s1x4,     1, 0, 1, 1,  0xAF5, schema_concentric},
   {s_rigger,       schema_nothing, {5, 0, 1, 4,    6, 7, 2, 3},
             s2x2,     s1x4,     1, 0, 1, 1,  0xAF5, schema_concentric},
   {s_qtag,         schema_nothing, {0, 1, 3, 4, 5, 7,    6, 2},
             s_bone6,  s1x2,     0, 0, 1, 1,  0xAFA, schema_concentric_6_2_tgl},
   {sbigdmd,        schema_nothing, {9, 3,     0, 1, 2, -1, 4, 5, 6, 7, 8, -1, 10, 11},
             s1x2,      sbigdmd, 1, 0, 2, 1,  0xAFD, schema_concentric},
   {sbighrgl,       schema_nothing, {9, 3,      0, 1, 2, -1, 4, 5, 6, 7, 8, -1, 10, 11},
             s1x2,      sbigdmd, 0, 0, 2, 1,  0xAFE, schema_concentric},
   {sbigdhrgl,      schema_nothing, {8, 2,      0, 1, 9, -1, 4, 5, 6, 7, 3, -1, 10, 11},
             s1x2,     sbigbone, 1, 0, 2, 1,  0xAFD, schema_concentric},
   {sdeepxwv,       schema_nothing, {5, 4, 1, 3, 2, 11, 10, 7, 9, 8,    0, 6},
             sdeep2x1dmd, s1x2,  1, 0, 2, 1,  0xAF5, schema_concentric},
   {sbigdmd,        schema_nothing, {9, 3,    0, 1, 2, 4, 5, 6, 7, 8, 10, 11},
             s1x2, sdeep2x1dmd,  1, 0, 2, 1,  0xAFD, schema_concentric},
   {sbighrgl,       schema_nothing, {9, 3,    0, 1, 2, 4, 5, 6, 7, 8, 10, 11},
             s1x2, sdeep2x1dmd,  0, 0, 2, 1,  0xAFE, schema_concentric},
   {sdeepxwv,       schema_concentric_8_4, {5, 4, 3, 2, 11, 10, 9, 8,    0, 1, 6, 7},
             s2x4, s1x4,         1, 0, 1, 1,  0x8FD, schema_concentric},
   {s_crosswave,    schema_nothing, {0, 1, 3, 4, 5, 7,    6, 2},
             s_1x2dmd, s1x2,     0, 1, 1, 1,  0xAF5, schema_concentric},
   {s2x4,           schema_nothing, {1, 2, 5, 6,    7, 0, 3, 4},
             s2x2,     s2x2,     0, 1, 9, 1,  0xAF7, schema_concentric},
   {s2x4,           schema_nothing, {6, 1, 2, 5,    0, 3, 4, 7},
             s2x2,     s2x2,     1, 0, 9, 1,  0xAFD, schema_concentric},
   {s2x4,           schema_nothing, {6, 1, 2, 5,    7, 0, 3, 4},
             s2x2,     s2x2,     1, 1, 9, 1,  0xAFB, schema_concentric},
   {s_bone,         schema_nothing, {6, 7, 2, 3,    5, 0, 1, 4},
             s1x4,     s2x2,     0, 1, 9, 1,  0xAF7, schema_concentric},
   {s_qtag,         schema_nothing, {6, 7, 2, 3,    5, 0, 1, 4},
             s1x4,     s2x2,     0, 1, 9, 1,  0xAFD, schema_concentric},
   {s_dhrglass,     schema_nothing, {6, 3, 2, 7,    5, 0, 1, 4},
             sdmd,     s2x2,     0, 1, 9, 1,  0xAF7, schema_concentric},
   {s_hrglass,      schema_nothing, {6, 3, 2, 7,    5, 0, 1, 4},
             sdmd,     s2x2,     0, 1, 9, 1,  0xAFD, schema_concentric},
   {s_hrglass,      schema_nothing, {5, -1, 0, 3, 1, -1, 4, 7,    6, 2},
             s_323,     s1x2,     1, 0, 9, 1, 0xAFD, schema_concentric},
   {s_crosswave,    schema_concentric_diamonds, {1, 3, 5, 7,    0, 2, 4, 6},
             sdmd,     sdmd,     0, 0, 1, 1,  0x8FA, schema_concentric},
   {s3x4,           schema_concentric_zs,       {8, 11, 1, 2, 5, 7,    9, 10, 0, 3, 4, 6},
             s2x3,     s2x3,     1, 1, 2, 1,  0x8FA, schema_concentric},
   // Very special maps for 2x3's that are both rotated 45 degrees.
   {s2x4,           schema_nothing,             {0, 1, -1, 4, 5, -1,    -1, 2, 3, -1, 6, 7},
             s2x3,     s2x3,     0, 0, 2, 1,  0x6FE, schema_concentric},
   {s2x4,           schema_nothing,             {-1, 2, 3, -1, 6, 7,   0, 1, -1, 4, 5, -1},
             s2x3,     s2x3,     0, 0, 2, 1,  0x6FB, schema_concentric},
   // This next map, with its loss of elongation information for the outer diamond,
   // is necessary to make the call "with confidence" work from the setup formed
   // by having the centers partner tag in left-hand waves.  This means that certain
   // Bakerisms involving concentric diamonds, in which each diamond must remember
   // its own elongation, are not possible.  Too bad.
   {s_crosswave,    schema_nothing, {1, 3, 5, 7,    6, 0, 2, 4},
             sdmd,     sdmd,     0, 1, 9, 1,  0xAF5, schema_concentric},
   {s3x1dmd,        schema_nothing, {1, 2, 5, 6,    7, 0, 3, 4},
             s1x4,     sdmd,     0, 1, 9, 1,  0xAF5, schema_concentric},
   {s3x1dmd,        schema_concentric_diamond_line, {1, 2, 5, 6,    0, 3, 4, 7},
             s1x4,     sdmd,     0, 0, 1, 1,  0x8FA, schema_concentric},
   {s_galaxy,       schema_nothing, {7, 1, 3, 5,    0, 2, 4, 6},
             s2x2,     sdmd,     1, 0, 9, 1,  0xAF5, schema_concentric},
   {s_galaxy,       schema_nothing, {1, 3, 5, 7,    0, 2, 4, 6},
             s2x2,     sdmd,     0, 0, 9, 1,  0xAFA, schema_concentric},
   {s_crosswave,    schema_nothing, {1, 3, 5, 7,    6, 0, 2, 4},
             sdmd,     s_star,   0, 1, 9, 1,  0xAF5, schema_concentric},
   {s_crosswave,    schema_nothing, {1, 3, 5, 7,    0, 2, 4, 6},
             sdmd,     s_star,   0, 0, 9, 1,  0xAFA, schema_concentric},
   {s3x1dmd,        schema_nothing, {1, 2, 5, 6,    7, 0, 3, 4},
             s1x4,     s_star,   0, 1, 9, 1,  0xAF5, schema_concentric},
   {s3x1dmd,        schema_nothing, {1, 2, 5, 6,    0, 3, 4, 7},
             s1x4,     s_star,   0, 0, 1, 1,  0xAFA, schema_concentric},
   {sbigh,          schema_concentric, {4, 5, 10, 11,    0, 9, 6, 3},
             s1x4,     s2x2,     0, 0, 3, 1,  0x8EF, schema_concentric},

   {s2x6,           schema_3x3k_concentric, {2, 3, 8, 9,   1, 4, 7, 10,  0, 5, 6, 11},
             s2x2,     s2x2,     0, 0, 1, 2,  0x8FE, schema_3x3k_concentric},
   {s2x6,           schema_3x3k_concentric, {9, 2, 3, 8,   10, 1, 4, 7,  11, 0, 5, 6},
             s2x2,     s2x2,     1, 1, 9, 2,  0xAFB, schema_3x3k_concentric},
   {s1x12,          schema_3x3k_concentric, {4, 5, 10, 11,   2, 3, 8, 9,   0, 1, 6, 7},
             s1x4,     s1x4,     0, 0, 1, 2,  0x8FA, schema_3x3k_concentric},

   {s2x8,           schema_4x4k_concentric, {3, 4, 11, 12,   2, 5, 10, 13,    1, 6, 9, 14,    0, 7, 8, 15},
             s2x2,     s2x2,     0, 0, 1, 3,  0x8FE, schema_4x4k_concentric},
   {s2x8,           schema_4x4k_concentric, {12, 3, 4, 11,   13, 2, 5, 10,    14, 1, 6, 9,    15, 0, 7, 8},
             s2x2,     s2x2,     1, 1, 9, 3,  0xAFB, schema_4x4k_concentric},
   {s1x16,          schema_4x4k_concentric, {6, 7, 14, 15,   4, 5, 12, 13,   2, 3, 10, 11,   0, 1, 8, 9},
             s1x4,     s1x4,     0, 0, 1, 3,  0x8FA, schema_4x4k_concentric},

   // The 3x4 items appear after the 3dmd items.  Since the search order is reversed,
   // the 3x4 will be preferred when putting together a 1x4 and 2x2.
   {s3dmd,          schema_concentric, {10, 11, 4, 5,    0, 2, 6, 8},
             s1x4,     s2x2,     0, 0, 3, 1,  0x8EF, schema_concentric},
   {s3dmd,          schema_nothing, {10, 11, 4, 5,    8, 0, 2, 6},
             s1x4,     s2x2,     0, 1, 3, 1,  0xADF, schema_concentric},
   {s3x4,           schema_concentric, {10, 11, 4, 5,    0, 3, 6, 9},
             s1x4,     s2x2,     0, 0, 3, 1,  0x8EF, schema_concentric},
   {s3x4,           schema_nothing, {10, 11, 4, 5,    9, 0, 3, 6},
             s1x4,     s2x2,     0, 1, 3, 1,  0xADF, schema_concentric},

   // These must be in this order, so it will prefer the 2x2 on the outside.
   {s4x4,           schema_concentric, {15, 3, 7, 11,    0, 1, 2, -1, 4, 5, 6, -1, 8, 9, 10, -1, 12, 13, 14, -1},
             s2x2,     s4x4,     0, 0, 3, 1,  0x900, schema_concentric},
   {s4x4,           schema_concentric, {15, 3, 7, 11,    12, 0, 4, 8},
             s2x2,     s2x2,     0, 0, 3, 1,  0x8CF, schema_concentric},

   {s1x3dmd,        schema_concentric_diamond_line, {1, 2, 5, 6,    0, 3, 4, 7},
             s1x4,     sdmd,     0, 0, 1, 1,  0x900, schema_nothing},
   {s_crosswave,    schema_concentric_diamond_line, {1, -1, 5, -1,    0, 2, 4, 6},
             s1x4,     sdmd,     0, 0, 1, 1,  0x900, schema_nothing},
   {s1x8,           schema_concentric_diamond_line, {1, -1, 5, -1,    0, -1, 4, -1},
             s1x4,     sdmd,     0, 0, 1, 1,  0x900, schema_nothing},
   {s3x4,           schema_concentric_lines_z, {8, 11, 1, 2, 5, 7,    0, 3, 6, 9},
             s2x3,     s2x2,     1, 0, 3, 1,  0x900, schema_nothing},

   // This one allows "finish" when the center "line" is actually a diamond
   // whose centers are empty.  This can happen because of the preference
   // given for lines over diamonds at the conclusion of certain calls.
   {s_wingedstar,   schema_concentric_diamond_line, {1, 2, 5, 6,    0, 3, 4, 7},
             s1x4,     sdmd,     0, 0, 1, 1,  0x900, schema_nothing},
   {s_wingedstar12, schema_concentric_diamond_line, {1, 2, 4, 7, 8, 10,     11, 0, 3, 5, 6, 9},
             s1x6,     s_short6, 0, 1, 1, 1,  0x8F5, schema_conc_star12},
   {s_wingedstar16, schema_concentric_diamond_line, {1, 2, 6, 5, 9, 10, 14, 13,    3, 7, 12, 8, 11, 15, 4, 0},
             s1x8,    s_spindle, 0, 0, 1, 1,  0x8FA, schema_conc_star16},
   {s_barredstar12, schema_concentric_diamond_line, {11, 12, 13, 4, 5, 6,     0, 1, 2, 3, 7, 8, 9, 10},
             s1x6,     s2x4,     0, 0, 2, 1,  0x8FA, schema_conc_bar12},
   {s_barredstar16, schema_concentric_diamond_line, {14, 15, 17, 16, 5, 6, 8, 7,     0, 1, 2, 3, 4, 9, 10, 11, 12, 13},
             s1x8,     s2x5,     0, 0, 2, 1,  0x8FA, schema_conc_bar16},
   {s_barredstar,   schema_concentric_diamond_line, {8, 9, 3, 4,     0, 1, 2, 5, 6, 7},
             s1x4,     s2x3,     0, 0, 2, 1,  0x8FB, schema_concentric},
   {s_qtag,         schema_concentric_others, {6, 7, 3, 2,    0, 1, 4, 5},
             s1x2,     s2x2,     0, 0, 2, 2,  0x900, schema_nothing},
   {s1x8,           schema_concentric_others, {1, 3, 2, 6, 7, 5,    0, 4},
             s1x2,     s1x2,     0, 0, 1, 3,  0x8FA, schema_concentric_others},
   {s1x6,           schema_concentric_others, {1, 2, 5, 4,    0, 3},
             s1x2,     s1x2,     0, 0, 1, 2,  0x8FA, schema_concentric_others},
   {s_spindle,      schema_nothing, {6, 0, 5, 1, 4, 2,    7, 3},
             s1x2,     s1x2,     1, 0, 1, 3,  0xAF5, schema_concentric_others},
   {s_spindle,      schema_concentric_others, {0, 1, 2, 4, 5, 6,    7, 3},
             s2x3,     s1x2,     0, 0, 1, 1,  0x900, schema_nothing},
   {s_short6,       schema_concentric_4_2, {0, 2, 3, 5,    4, 1},
             s2x2,     s1x2,     0, 1, 2, 1,  0x900, schema_nothing},
   {s_short6,       schema_concentric_6p, {0, 2, 3, 5,    4, 1},
             s2x2,     s1x2,     0, 1, 2, 1,  0x900, schema_nothing},
   {s_short6,       schema_concentric_others, {0, 2, 3, 5,    4, 1},
             s2x2,     s1x2,     0, 1, 2, 1,  0x8F5, schema_concentric},
   {s_short6,       schema_concentric, {5, 0, 2, 3,    4, 1},
             s2x2,     s1x2,     1, 1, 2, 1,  0x8FA, schema_concentric},
   {s_short6,       schema_nothing, {5, 3, 0, 2,    4, 1},
             s1x2,     s1x2,     0, 1, 2, 2,  0xAF5, schema_concentric_others},
   {s2x4,           schema_concentric_others, {1, 2, 6, 5,    0, 3, 4, 7},
             s1x2,     s2x2,     0, 0, 1, 2,  0x900, schema_nothing},
   {s_galaxy,       schema_intlk_lateral_6, {0, 3, 5, 4, 7, 1,    6, 2},
             s_trngl,  s1x2,     3, 1, 1, 2,  0x8FE, schema_intlk_lateral_6},
   {s_hrglass,      schema_intlk_vertical_6, {7, 0, 1, 3, 4, 5,    6, 2},
             s_trngl,  s1x2,     0, 0, 1, 2,  0x8FE, schema_intlk_vertical_6},
   {s_galaxy,       schema_intlk_vertical_6, {2, 5, 7, 6, 1, 3,    0, 4},
             s_trngl,  s1x2,     2, 0, 1, 2,  0x8FD, schema_intlk_vertical_6},
   {s_galaxy,       schema_lateral_6, {7, 0, 1, 3, 4, 5,    6, 2},
             s_short6, s1x2,     1, 1, 1, 1,  0x900, schema_nothing},
   {s_galaxy,       schema_vertical_6, {1, 2, 3, 5, 6, 7,    0, 4},
             s_short6, s1x2,     0, 0, 1, 1,  0x8FA, schema_concentric},
   {s2x8,           schema_in_out_quad, {0, 1, 14, 15, 6, 7, 8, 9,
                                         2, 3, 4, 5, 10, 11, 12, 13},
             s2x2,     s2x4,     0, 0, 1, 2,  0x8FE, schema_in_out_quad},
   {s4x4,           schema_in_out_quad, {8, 6, 4, 5, 12, 13, 0, 14,
                                        10, 15, 3, 1, 2, 7, 11, 9},
             s1x4,     s2x4,     0, 0, 2, 2,  0x8FB, schema_in_out_quad},
   {s3x4,           schema_in_out_12mquad, {9, 10, 0, 6, 4, 3,              8, 11, 1, 2, 5, 7},
             s1x3,     s2x3,     1, 1, 1, 2,  0x8FB, schema_in_out_12mquad},
   {sd3x4,         schema_in_out_12mquad, {10, 11, 0, 4, 5, 6,             1, 2, 3, 7, 8, 9},
             s1x3,     s2x3,     1, 0, 1, 2,  0x8FD, schema_in_out_12mquad},
   {s3dmd,          schema_in_out_12mquad, {8, 7, 6, 0, 1, 2,     9, 10, 11, 3, 4, 5},
             s1x3,     s1x6,     0, 0, 2, 2,  0x8FB, schema_in_out_12mquad},
   {s_hrglass,      schema_in_out_triple, {5, 4, -1, -1, -1, -1, 1, 0,     6, 3, 2, 7},
             s2x2,     sdmd,     0, 0, 2, 2,  0x900, schema_nothing},
   {s_dhrglass,     schema_in_out_triple, {-1, -1, 0, 5, 4, 1, -1, -1,     6, 3, 2, 7},
             s2x2,     sdmd,     1, 0, 1, 2,  0x900, schema_nothing},
   {s3x4,           schema_in_out_triple, {9, 8, 6, 7, 0, 1, 3, 2,    10, 11, 4, 5},
             s1x4,     s1x4,     0, 0, 2, 2,  0x8FB, schema_in_out_triple},


   {s3x4,           schema_in_out_triple_dyp_squash, {-1, -1, -1, -1, 6, 7, 8, 9,    0, 1, 2, 3, -1, -1, -1, -1,
                                                      10, 11, 4, 5},
             s2x4,     s1x4,     0, 0, 2, 2,  0x900, schema_nothing},


   {sbigh,          schema_in_out_triple_dyp_squash, {3, 2, 1, 0, -1, -1, -1, -1,    -1, -1, -1, -1, 9, 8, 7, 6,
                                                      4, 5, 10, 11},
             s2x4,     s1x4,     1, 0, 1, 2,  0x900, schema_nothing},

   {s_crosswave,    schema_in_out_triple_dyp_squash, {0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 5, -1, -1,
                                                      6, 7, 2, 3},
             s1x8,     s1x4,     0, 1, 1, 2,  0x900, schema_nothing},

   {sbigbone,       schema_in_out_triple_dyp_squash, {0, 1, -1, -1, -1, -1, 10, 11,    -1, -1, 4, 5, 6, 7, -1, -1,
                                                      2, 3, 8, 9},
             s2x4,     s1x4,     0, 0, 1, 2,  0x900, schema_nothing},

   // For incredibly evil unsymmetrical crossfire.
   {slinedmd,       schema_nothing, {3, 1, 5, 7,     2, 0, 4, 6},
             s_trngl4, s2x2,     0, 2, 1, 1,  0xAFD, schema_concentric},
   {slinedmd,       schema_nothing, {3, 1, 5, 7,     4, 6, 2, 0},
             s_trngl4, s2x2,     0, 0, 1, 1,  0xAFE, schema_concentric},

   {s_hsqtag,       schema_in_out_triple, {3, 2, 0, 1, 6, 7, 9, 8,         10, 11, 4, 5},
             s1x4,     sdmd,     1, 1, 1, 2,  0x8FB, schema_in_out_triple},
   {s_dmdlndmd,     schema_in_out_triple, {9, 10, 0, 11, 6, 5, 3, 4,         7, 8, 1, 2},
             sdmd,     s1x4,     1, 1, 1, 2,  0x8FB, schema_in_out_triple},
   {s2x3,           schema_sgl_in_out_triple, {5, 0, 3, 2,       4, 1},
             s1x2,     s1x2,     1, 1, 1, 2,  0x900, schema_nothing},
   {s3x6,           schema_3x3_in_out_triple, {14, 13, 12, 9, 10, 11, 0, 1, 2, 5, 4, 3,
                                               15, 16, 17, 6, 7, 8},
             s1x6,     s1x6,     0, 0, 2, 2,  0x900, schema_nothing},
   {sbig3dmd,       schema_nothing, {12, 13, 14, 9, 10, 11, 0, 1, 2, 3, 4, 5,     15, 16, 17, 6, 7, 8},
             s2x3,     s1x6,     0, 0, 2, 2,  0xAFB, schema_3x3_in_out_triple},
   {s3x8,           schema_4x4_in_out_triple, {19, 18, 16, 17, 12, 13, 15, 14,
                                               0, 1, 3, 2, 7, 6, 4, 5,
                                               20, 21, 23, 22, 8, 9, 11, 10},
             s1x8,     s1x8,     0, 0, 2, 2,  0x900, schema_nothing},
   {sbig4dmd,       schema_nothing, {16, 17, 18, 19, 12, 13, 14, 15,
                                     0, 1, 2, 3, 4, 5, 6, 7,
                                     20, 21, 23, 22, 8, 9, 11, 10},
             s2x4,     s1x8,     0, 0, 2, 2,  0xAFB, schema_4x4_in_out_triple},
   {s_1x2dmd,       schema_nothing, {0, 1, 4, 3,       5, 2},
             s1x2,     s1x2,     0, 1, 1, 2,  0xAF7, schema_sgl_in_out_triple},
   {s_qtag,         schema_in_out_triple, {5, 4, 0, 1,     6, 7, 2, 3},
             s1x2,     s1x4,     0, 0, 2, 2,  0x8FB, schema_in_out_triple},
   {s_rigger,       schema_in_out_triple, {6, 7, 3, 2,     5, 0, 1, 4},
             s1x2,     s2x2,     0, 1, 1, 2,  0x8F7, schema_in_out_triple},
   {s_rigger,       schema_nothing, {6, 7, 3, 2,     0, 1, 4, 5},
             s1x2,     s2x2,     0, 0, 1, 2,  0xAFE, schema_in_out_triple},

   {s4dmd,          schema_nothing, {11, 12, 0, 13, 8, 5, 3, 4,    10, -1, -1, 1, 2, -1, -1, 9},
             sdmd,     s2x4,     1, 1, 1, 2,  0xAFB, schema_in_out_quad},
   {s_hqtag,        schema_nothing, {3, 2, 0, 1, 8, 9, 11, 10,    4, 5, 6, 7, 12, 13, 14, 15},
             s1x4,     s_qtag,   1, 0, 1, 2,  0xAFD, schema_in_out_quad},
   {swqtag,         schema_nothing, {9, 4,     0, 1, 2, 3, 5, 6, 7, 8},
             s1x2, s_rigger,     0, 0, 1, 1,  0xAFB, schema_concentric},
   {swhrglass,      schema_nothing, {9, 4,     0, 1, 2, 3, 5, 6, 7, 8},
             s1x2, s_rigger,     1, 0, 1, 1,  0xAF7, schema_concentric},
   {sd2x5,          schema_concentric, {4, 3, 2, 9, 8, 7,     0, 6, 5, 1}, \
             s2x3,     s2x2,     1, 0, 1, 1,  0x8FD, schema_concentric},
   {sd2x5,          schema_concentric, {4, 3, 2, 9, 8, 7,     1, 0, 6, 5},
             s2x3,     s2x2,     1, 1, 2, 1,  0x8FB, schema_concentric},
   {s2x5,           schema_concentric, {1, 2, 3, 6, 7, 8,     9, 0, 4, 5},
             s2x3,     s2x2,     0, 1, 2, 1,  0x8F7, schema_concentric},
   {s2x5,           schema_concentric, {1, 2, 3, 6, 7, 8,     0, 4, 5, 9},
             s2x3,     s2x2,     0, 0, 1, 1,  0x8FE, schema_concentric},

   {s_spindle12,    schema_concentric, {0, 1, 2, 5, 6, 7,     8, 9, 3, 4},
             s2x3,     s1x4,     0, 0, 1, 1,  0x8FE, schema_concentric},

   {s2x5,           schema_checkpoint, {-1, 2, 4, -1, 7, 9,     1, 3, 6, 8},
             s2x3,     s2x2,     0, 0, 1, 1,  0x900, schema_nothing},
   {s2x5,           schema_checkpoint, {0, 2, -1, 5, 7, -1,     1, 3, 6, 8},
             s2x3,     s2x2,     0, 0, 1, 1,  0x900, schema_nothing},
   {s2x6,           schema_checkpoint, {-1, 2, -1, 5, -1, 8, -1, 11,     1, 4, 7, 10},
             s2x4,     s2x2,     0, 0, 1, 1,  0x900, schema_nothing},
   {s2x6,           schema_checkpoint, { 0, -1, 3, -1, 6, -1, 9, -1,     1, 4, 7, 10},
             s2x4,     s2x2,     0, 0, 1, 1,  0x900, schema_nothing},
   {s1x16,          schema_in_out_quad, {0, 1, 3, 2, 11, 10, 8, 9,
                                         4, 5, 7, 6, 12, 13, 15, 14},
             s1x4,     s1x8,     0, 0, 1, 2,  0x8FE, schema_in_out_quad},
   {sbigbigh,       schema_in_out_quad, {3, 2, 0, 1, 8, 9, 11, 10,
                                         4, 5, 7, 6, 12, 13, 15, 14},
             s1x4,     s1x8,     1, 0, 1, 2,  0x8FD, schema_in_out_quad},
   {sbigbigx,       schema_in_out_quad, {0, 1, 3, 2, 11, 10, 8, 9,         7, 6, 5, 4, 15, 14, 13, 12},
             s1x4,     s2x4,     0, 1, 1, 2,  0x8F7, schema_in_out_quad},
   {s1x12,          schema_in_out_triple, {0, 1, 3, 2, 9, 8, 6, 7,     4, 5, 10, 11},
             s1x4,     s1x4,     0, 0, 1, 2,  0x8FE, schema_in_out_triple},
   {sbigx,          schema_in_out_triple, {0, 1, 3, 2, 9, 8, 6, 7,     10, 11, 4, 5},
             s1x4,     s1x4,     0, 1, 1, 2,  0x8F7, schema_in_out_triple},
   {sbigh,          schema_in_out_triple, {3, 2, 0, 1, 6, 7, 9, 8,         4, 5, 10, 11},
             s1x4,     s1x4,     1, 0, 1, 2,  0x8FD, schema_in_out_triple},
   {sbigbone,       schema_nothing, {11, 0, 1, 10, 7, 4, 5, 6,          2, 3, 8, 9},
             s2x2,     s1x4,     1, 0, 1, 2,  0xAFD, schema_in_out_triple},
   {sbigbone,       schema_in_out_triple, {0, 1, 10, 11, 4, 5, 6, 7,       2, 3, 8, 9},
             s2x2,     s1x4,     0, 0, 1, 2,  0x8FE, schema_in_out_triple},
   {sbigrig,        schema_in_out_triple, {0, 1, 3, 2, 9, 8, 6, 7,         4, 5, 10, 11},
             s1x4,     s2x2,     0, 0, 1, 2,  0x8FE, schema_in_out_triple},
   {sbigrig,        schema_nothing, {0, 1, 3, 2, 9, 8, 6, 7,             11, 4, 5, 10},
             s1x4,     s2x2,     0, 1, 1, 2,  0xAF7, schema_in_out_triple},
   {s5x1dmd,        schema_in_out_triple, {0, 1, 3, 2, 9, 8, 6, 7,         11, 4, 5, 10},
             s1x4,     sdmd,     0, 1, 1, 2,  0x8F7, schema_in_out_triple},
   {s1x5dmd,        schema_in_out_triple, {0, 1, 3, 2, 9, 8, 6, 7,         4, 5, 10, 11},
             s1x4,     sdmd,     0, 0, 1, 2,  0x8FE, schema_in_out_triple},
   {sbigdmd,        schema_in_out_triple, {11, 0, 1, 10, 7, 4, 5, 6,       8, 9, 2, 3},
             s2x2,     s1x4,     1, 1, 1, 2,  0x8FB, schema_in_out_triple},


   {sbigdmd,        schema_in_out_triple_dyp_squash, {0, 1, -1, -1, -1, -1, 10, 11,    -1, -1, 4, 5, 6, 7, -1, -1,
                                                      8, 9, 2, 3},
             s2x4,     s1x4,     0, 1, 1, 2,  0x900, schema_nothing},



   {sbighrgl,       schema_in_out_triple, {0, 1, 10, 11, 4, 5, 6, 7,       8, 9, 2, 3},
             s2x2,     sdmd,     0, 1, 1, 2,  0x8F7, schema_in_out_triple},
   {sbigdhrgl,      schema_in_out_triple, {0, 1, 10, 11, 4, 5, 6, 7,       9, 2, 3, 8},
             s2x2,     sdmd,     0, 0, 1, 2,  0x8FE, schema_in_out_triple},
   {s2x6,           schema_in_out_triple, {0, 1, 10, 11, 4, 5, 6, 7,       2, 3, 8, 9},
             s2x2,     s2x2,     0, 0, 1, 2,  0x8FE, schema_in_out_triple},
   {s4x6,           schema_in_out_triple, {17, 18, 11, 0, 1, 10, 19, 16,
                                           13, 22, 7, 4, 5, 6, 23, 12,
                                           15, 20, 9, 2, 3, 8, 21, 14},
             s2x4,     s2x4,     1, 1, 1, 2,  0x8FB, schema_in_out_triple},
   {s3x6,           schema_in_out_triple, {14, 15, 0, 1, 16, 13,
                                           10, 7, 4, 5, 6, 9,
                                           12, 17, 2, 3, 8, 11},
             s2x3,     s2x3,     1, 1, 1, 2,  0x8FB, schema_in_out_triple},

   {s3x4,           schema_in_out_triple_zcom, {-1, -1, -1, 0, 10, 9,
                                                6, 4, 3, -1, -1, -1,
                                                8, 11, 1, 2, 5, 7},
             s2x3,     s2x3,     1, 1, 2, 2,  0x8FB, schema_in_out_triple},

   {sd3x4,          schema_in_out_triple_zcom, {-1, -1, -1, 0, 11, 10,
                                                6, 5, 4, -1, -1, -1,
                                                1, 2, 3, 7, 8, 9},
             s2x3,     s2x3,     1, 0, 1, 2,  0x8FD, schema_in_out_triple},


   // Two ways the outsides can surround the center z in a 4x5.
   // Not competely satisfactory, but it will have to do.
   {s4x5,           schema_in_out_triple_zcom, {14, 13, 12, 10, 11,
                                                0, 1, 2, 4, 3,
                                                8, 7, 6, 18, 17, 16},
             s1x5,     s2x3,     0, 0, 2, 2,  0x8FE, schema_in_out_triple},
   {s4x5,           schema_in_out_triple_zcom, {14, 15, 0, 9,
                                                10, 19, 4, 5,
                                                8, 7, 6, 18, 17, 16},
             s1x4,     s2x3,     1, 0, 1, 2,  0x8FD, schema_in_out_triple},

   {s_545,          schema_nothing, {11, 10, 9, 7, 8,
                                     0, 1, 2, 4, 3,
                                     12, 13, 5, 6},
             s1x5,     s1x4,     0, 0, 1, 2,  0xAFE, schema_in_out_triple},
   {swqtag,         schema_in_out_triple, {7, 8,     3, 2,     6, 9, 0, 1, 4, 5},
             s1x2,     s2x3,     0, 1, 2, 2,  0x900, schema_nothing},
   {s2x5,           schema_in_out_triple, {0, 4, 5, 9,     1, 2, 3, 6, 7, 8},
             s2x2,     s2x3,     0, 0, 1, 1,  0x8FE, schema_in_out_triple},
   {sd2x5,          schema_in_out_triple, {0, 6, 5, 1,     4, 3, 2, 9, 8, 7},
             s2x2,     s2x3,     0, 1, 1, 1,  0x8F7, schema_in_out_triple},
   {s4x5,           schema_nothing, {9, 8, 6, 5, 19, 18, 16, 15,     12, 17, 2, 7},
             s2x4,     s1x4,     0, 1, 1, 1,  0x8F7, schema_in_out_triple},
   {s_qtag,         schema_nothing, {5, 0, 1, 4,     6, 7, 2, 3},
             s2x2,     s1x4,     1, 0, 2, 1,  0xAF7, schema_in_out_triple},
   {s_bone,         schema_nothing, {0, 1, 4, 5,     6, 7, 2, 3},
             s2x2,     s1x4,     0, 0, 1, 1,  0xAFE, schema_in_out_triple},

   // Next two are duplicate, for multiple analysis.
   {s4x4,           schema_in_out_triple, {8, 9, 12, 10, 4, 2, 0, 1,     15, 3, 7, 11},
             s1x4,     s2x2,     1, 0, 1, 2,  0x900, schema_nothing},
   {s4x4,           schema_in_out_triple, {8, 6, 4, 5, 12, 13, 0, 14,     15, 3, 7, 11},
             s1x4,     s2x2,     0, 0, 2, 2,  0x8FB, schema_in_out_triple},

   {s1x8,           schema_grand_single_concentric, {2, 6, 3, 7, 1, 5,    0, 4},
             s1x2,     s1x2,     0, 0, 1, 3,  0x8FA, schema_grand_single_concentric},
   {s_spindle,      schema_grand_single_concentric, {5, 1, 6, 0, 4, 2,    7, 3},
             s1x2,     s1x2,     1, 0, 1, 3,  0x8F5, schema_grand_single_concentric},
   {s1x6,           schema_grand_single_concentric, {2, 5, 1, 4,    0, 3},
             s1x2,     s1x2,     0, 0, 1, 2,  0x8FA, schema_grand_single_concentric},
   {s1x4,           schema_single_concentric, {1, 3,          0, 2},
             s1x2,     s1x2,     0, 0, 1, 1,  0x8FA, schema_concentric},
   {sdmd,           schema_single_concentric, {3, 1,          0, 2},
             s1x2,     s1x2,     1, 0, 1, 1,  0x8F5, schema_concentric},
   {s_wingedstar16, schema_conc_star16, {2, 3, 5, 4, 6, 7, 14, 15, 13, 12, 10, 11,     0, 1, 8, 9},
             s_star,   s1x4,     0, 0, 1, 3,  0x8FA, schema_conc_star16},
   {s4x4,           schema_4x4_lines_concentric, {10, 15, 3, 1, 2, 7, 11, 9,
                                                  12, 13, 14, 0, 4, 5, 6, 8},
             s2x4,     s2x4,     0, 0, 2, 1,  0x8FB, schema_concentric},
   {s4x4,           schema_4x4_cols_concentric, {6, 11, 15, 13, 14, 3, 7, 5,
                                                 8, 9, 10, 12, 0, 1, 2, 4},
             s2x4,     s2x4,     1, 1, 2, 1,  0x900, schema_nothing},
   {s4x6,           schema_4x4_lines_concentric, {10, 9, 8, 7, 22, 21, 20, 19,
                                                  17, 18, 11, 0, 5, 6, 23, 12},
             s2x4,     s2x4,     0, 1, 2, 1,  0x8F7, schema_concentric},
   {s4x6,           schema_4x4_cols_concentric, {10, 9, 8, 7, 22, 21, 20, 19,
                                                 17, 18, 11, 0, 5, 6, 23, 12},
             s2x4,     s2x4,     0, 1, 2, 1,  0x900, schema_nothing},
   {s_wingedstar12, schema_conc_star12, {2, 3, 4, 11, 10, 5, 8, 9,     0, 1, 6, 7},
             s_star,   s1x4,     0, 0, 1, 2,  0x8FA, schema_conc_star12},
   {s3x4,           schema_3x3_concentric, {8, 11, 1, 2, 5, 7,    9, 10, 0, 3, 4, 6},
             s2x3,     s2x3,     1, 1, 2, 1,  0x8FA, schema_concentric},
   {sd3x4,         schema_3x3_concentric, {1, 2, 3, 7, 8, 9,     10, 11, 0, 4, 5, 6},
             s2x3,     s2x3,     0, 1, 2, 1,  0x8F5, schema_concentric},
   {s_barredstar16, schema_conc_bar16, {15, 1, 16, 12, 17, 2, 8, 11, 7, 3, 6, 10,     13, 14, 0, 4, 5, 9},
             s_star,   s2x3,     0, 1, 2, 3,  0x8F5, schema_conc_bar16},
   {s2x8,           schema_conc_bar16, {1, 2, 13, 14, 3, 4, 11, 12, 5, 6, 9, 10,     0, 7, 8, 15},
             s2x2,     s2x2,     0, 0, 1, 3,  0x900, schema_nothing},
   {s_barredstar12, schema_conc_bar12, {12, 1, 13, 9, 6, 2, 5, 8,     10, 11, 0, 3, 4, 7},
             s_star,   s2x3,     0, 1, 2, 2,  0x8F5, schema_conc_bar12},
   {s2x6,           schema_conc_bar12, {1, 2, 9, 10, 3, 4, 7, 8,     0, 5, 6, 11},
             s2x2,     s2x2,     0, 0, 1, 2,  0x900, schema_nothing},
   {s_barredstar,   schema_conc_bar, {9, 1, 4, 6,     7, 8, 0, 2, 3, 5},
             s_star,   s2x3,     0, 1, 2, 1,  0x8F7, schema_concentric},
   // Next two are duplicate, for multiple analysis.
   {s2x4,           schema_conc_bar, {1, 2, 5, 6,    0, 3, 4, 7},
             s2x2,     s2x2,     0, 0, 1, 1,  0x900, schema_nothing},
   {s2x4,           schema_concentric, {1, 2, 5, 6,    0, 3, 4, 7},
             s2x2,     s2x2,     0, 0, 1, 1,  0x8FE, schema_concentric},
   {s2x4,     schema_special_trade_by, {1, 2, 5, 6,    0, 3, 4, 7},
             s2x2,     s2x2,     0, 0, 1, 1,  0x9FE, schema_concentric},
   {s_qtag,         schema_conc_o, {6, 7, 2, 3,    -1, -1, -1, -1, -1, 4, 5, -1,
                         -1, -1, -1, -1, -1, 0, 1, -1},
             s1x4,     s4x4,     0, 0, 1, 1,  0x8FB, schema_conc_o},
   {s2x4,           schema_conc_o, {1, 2, 5, 6,    -1, 3, 4, -1, -1, -1, -1, -1,
                         -1, 7, 0, -1, -1, -1, -1, -1},
             s2x2,     s4x4,     0, 0, 1, 1,  0x900, schema_nothing},
   {s4x4,           schema_conc_o, {15, 3, 7, 11,    0, 1, 2, -1, 4, 5, 6, -1,
                         8, 9, 10, -1, 12, 13, 14, -1},
             s2x2,     s4x4,     0, 0, 1, 1,  0x8CA, schema_conc_o},
   {s_hrglass,      schema_conc_o, {6, 3, 2, 7,    -1, -1, -1, -1, -1, 4, 5, -1,
                         -1, -1, -1, -1, -1, 0, 1, -1},
             sdmd,     s4x4,     0, 0, 1, 1,  0x8FB, schema_conc_o},
   {s_dhrglass,     schema_conc_o, {6, 3, 2, 7,    -1, 1, 4, -1, -1, -1, -1, -1,
                         -1, 5, 0, -1, -1, -1, -1, -1},
             sdmd,     s4x4,     0, 0, 1, 1,  0x8FE, schema_conc_o},
   {s_bone,         schema_conc_o, {6, 7, 2, 3,    -1, 1, 4, -1, -1, -1, -1, -1,
                         -1, 5, 0, -1, -1, -1, -1, -1},
             s1x4,     s4x4,     0, 0, 1, 1,  0x8FE, schema_conc_o},
   {s_qtag,         schema_concentric_6_2_tgl, {0, 1, 3, 4, 5, 7,    6, 2},
             s_bone6,  s1x2,     0, 0, 1, 1,  0x900, schema_nothing},
   {s2x3,           schema_concentric_6p, {4, 1,    0, 2, 3, 5},
             s1x2,     s2x2,     1, 0, 1, 1,  0x8FD, schema_concentric},
   {s2x3,           schema_concentric_2_4, {4, 1,    0, 2, 3, 5},
             s1x2,     s2x2,     1, 0, 1, 1,  0x8FD, schema_concentric},
   {s_2x1dmd,       schema_concentric_6p, {0, 1, 3, 4,    5, 2},
             s1x4,     s1x2,     0, 1, 1, 1,  0x8F5, schema_concentric},
   {s3x1dmd,        schema_concentric_6_2, {1, 2, 3, 5, 6, 7,    0, 4},
             s_2x1dmd, s1x2,     0, 0, 1, 1,  0x8FA, schema_concentric},
   {s3x1dmd,        schema_concentric_6_2_line, {0, 1, 2, 4, 5, 6,    7, 3},
             s1x6,     s1x2,     0, 1, 1, 1,  0x8F5, schema_concentric},
   {s_wingedstar,   schema_concentric_6_2_line, {0, 1, 2, 4, 5, 6,    7, 3},
             s1x6,     s1x2,     0, 1, 1, 1,  0x900, schema_concentric},
   {s_wingedstar,   schema_concentric_6_2, {1, 2, 3, 5, 6, 7,    0, 4},
             s_wingedstar6,s1x2, 0, 0, 0, 1,  0x8FA, schema_concentric},
   {s_crosswave,    schema_concentric_6_2, {6, 7, 1, 2, 3, 5,    0, 4},
             s_2x1dmd, s1x2,     1, 0, 1, 1,  0x8F5, schema_concentric},
   {s1x3dmd,        schema_concentric_6_2, {1, 2, 3, 5, 6, 7,    0, 4},
             s_1x2dmd, s1x2,     0, 0, 1, 1,  0x8FA, schema_concentric},
   // Next two are duplicate, for multiple analysis.
   {s_hrglass,      schema_vertical_6, {5, 0, 3, 1, 4, 7,    6, 2},
             s_bone6,  s1x2,     1, 0, 1, 1,  0x900, schema_nothing},
   {s_hrglass,      schema_concentric_6_2, {5, 0, 3, 1, 4, 7,    6, 2},
             s_bone6,  s1x2,     1, 0, 1, 1,  0x8F5, schema_concentric},
   {s_ptpd,         schema_concentric_6_2, {1, 7, 6, 5, 3, 2,    0, 4},
             s_bone6,  s1x2,     0, 0, 1, 1,  0x8FA, schema_concentric},
   {s1x8,           schema_concentric_6_2, {1, 3, 2, 5, 7, 6,    0, 4},
             s1x6,     s1x2,     0, 0, 1, 1,  0x8FA, schema_concentric},
   {s_rigger,       schema_concentric_6_2, {5, 7, 0, 1, 3, 4,    6, 2},
             s_short6, s1x2,     1, 0, 1, 1,  0x8F5, schema_concentric},
   {s_qtag,         schema_concentric_6_2, {5, 7, 0, 1, 3, 4,    6, 2},
             s2x3,     s1x2,     1, 0, 1, 1,  0x8F5, schema_concentric},
   {s_2stars,       schema_concentric_6_2, {0, 1, 3, 4, 5, 7,    6, 2},
             s_bone6,  s1x2,     0, 0, 1, 1,  0x9F5, schema_concentric},
   {s4x5,          schema_nothing, {9, 8, 7, 6, 5, 19, 18, 17, 16, 15,    2, 12},
             s2x5,     s1x2,     0, 1, 1, 1,  0xAF5, schema_concentric},
   {s_spindle,      schema_concentric_6_2, {0, 1, 2, 4, 5, 6,    7, 3},
             s2x3,     s1x2,     0, 0, 1, 1,  0x8FA, schema_concentric},
   {s_crosswave,    schema_concentric_2_6, {7, 3,    0, 1, 2, 4, 5, 6},
             s1x2,     s_2x1dmd, 1, 0, 1, 1,  0x8F5, schema_concentric},
   {s3x1dmd,        schema_concentric_2_6, {2, 6,    0, 1, 3, 4, 5, 7},
             s1x2,     s_2x1dmd, 0, 0, 1, 1,  0x8FA, schema_concentric},
   {s1x3dmd,        schema_concentric_2_6, {7, 3,    0, 1, 2, 4, 5, 6},
             s1x2,     s1x6,     1, 0, 1, 1,  0x8F5, schema_concentric},
   {s_ptpd,         schema_concentric_2_6, {2, 6,    3, 0, 1, 7, 4, 5},
             s1x2,     s_short6, 0, 1, 2, 1,  0x8F7, schema_concentric},
   {s3x4,           schema_concentric_2_6, {11, 5,   9, 10, 0, 3, 4, 6},
             s1x2,     s2x3,     0, 1, 2, 1,  0x8F5, schema_concentric},
   {s2x5,           schema_concentric_2_6, {7, 2,   0, 1, 3, 4, 5, 6, 8, 9},
             s1x2,     s2x4,     1, 0, 2, 1,  0x8F5, schema_concentric},
   {s1x8,           schema_concentric_2_6, {2, 6,    0, 1, 3, 4, 5, 7},
             s1x2,     s1x6,     0, 0, 1, 1,  0x8FA, schema_concentric},
   {s_dhrglass,     schema_concentric_2_6, {7, 3,    0, 1, 2, 4, 5, 6},
             s1x2,     s_bone6,  1, 0, 1, 1,  0x8F5, schema_concentric},
   {s_3mdmd,        schema_concentric_big2_6, {7, 1,    0, 2, 5, 6, 8, 11},
             s1x2,     s_bone6,  1, 0, 1, 1,  0x8FD, schema_concentric_big2_6},
   {s4dmd,          schema_concentric_big2_6, {15, 7,   0, 3, 6, 8, 11, 14},
             s1x2,     s_bone6,  0, 0, 1, 1,  0x8FE, schema_concentric_big2_6},
   {s_bone,         schema_concentric_2_6, {7, 3,    0, 1, 2, 4, 5, 6},
             s1x2,     s_bone6,  0, 0, 1, 1,  0x8FA, schema_concentric},
   {s_qtag,         schema_concentric_2_6, {7, 3,    5, 6, 0, 1, 2, 4},
             s1x2,     s_short6, 0, 1, 1, 1,  0x8FD, schema_concentric},
   {s_hrglass,      schema_concentric_2_6, {7, 3,    5, 6, 0, 1, 2, 4},
             s1x2,     s_short6, 1, 1, 1, 1,  0x8FE, schema_concentric},
   {s_spindle,      schema_concentric_2_6, {5, 1,    6, 7, 0, 2, 3, 4},
             s1x2,     s_short6, 1, 1, 2, 1,  0x8FB, schema_concentric},
   {s_ntrglcw,      schema_concentric_2_6, {5, 1,    0, 2, 3, 4, 6, 7},
             s1x2,   s_ntrgl6cw, 1, 0, 1, 1,  0x8FD, schema_concentric},
   {s_ntrglccw,     schema_concentric_2_6, {6, 2,    0, 1, 3, 4, 5, 7},
             s1x2,  s_ntrgl6ccw, 1, 0, 1, 1,  0x8FD, schema_concentric},
   {s_nptrglcw,     schema_concentric_2_6, {1, 5,    0, 2, 3, 4, 6, 7},
             s1x2,   s_ntrgl6cw, 0, 0, 1, 1,  0x8FE, schema_concentric},
   {s_nptrglccw,    schema_concentric_2_6, {2, 6,    0, 1, 3, 4, 5, 7},
             s1x2,  s_ntrgl6ccw, 0, 0, 1, 1,  0x8FE, schema_concentric},
   {s_nxtrglcw,     schema_concentric_6_2, {0, 1, 2, 4, 5, 6,    7, 3},
             s_ntrgl6cw,   s1x2, 0, 0, 1, 1,  0x8FE, schema_concentric},
   {s_nxtrglccw,    schema_concentric_6_2, {0, 1, 2, 4, 5, 6,    7, 3},
             s_ntrgl6ccw,  s1x2, 0, 0, 1, 1,  0x8FE, schema_concentric},
   {s3dmd,          schema_concentric_2_6, {11, 5,    0, 1, 2, 6, 7, 8},
             s1x2,     s2x3,     0, 0, 2, 1,  0x900, schema_nothing},
   {s3dmd,          schema_concentric_2_6, {11, 5,    0, 4, 3, 6, 10, 9},
             s1x2,  s_nftrgl6cw, 0, 0, 1, 1,  0x8FE, schema_concentric},
   {s3dmd,          schema_concentric_2_6, {11, 5,    9, 10, 2, 3, 4, 8},
             s1x2,  s_nftrgl6ccw, 0, 0, 1, 1, 0x8FE, schema_concentric},
   {s_dmdlndmd,     schema_concentric_2_6, {2, 8,    0, 5, 4, 6, 11, 10},
             s1x2,  s_nftrgl6cw, 1, 0, 1, 1,  0x8FD, schema_concentric},
   {s_dmdlndmd,     schema_concentric_2_6, {2, 8,    10, 11, 3, 4, 5, 9},
             s1x2,  s_nftrgl6ccw, 1, 0, 1, 1, 0x8FD, schema_concentric},
   {s_323,          schema_concentric_2_6, {7, 3,     0, 1, 2, 4, 5, 6},
             s1x2,     s2x3,     0, 0, 2, 1,  0x8FA, schema_concentric},
   {s1x3dmd,        schema_checkpoint, {0, 3, 4, 7,    1, 2, 5, 6},
             sdmd,     s1x4,     0, 0, 1, 1,  0x87A, schema_rev_checkpoint},
   {s_ptpd,         schema_checkpoint, {0, 2, 4, 6,    1, 7, 5, 3},
             s1x4,     s2x2,     0, 0, 1, 1,  0x87A, schema_rev_checkpoint},
   {s1x8,           schema_checkpoint, {0, 2, 4, 6,    1, 3, 5, 7},
             s1x4,     s1x4,     0, 0, 1, 1,  0x87A, schema_rev_checkpoint},
   {s_dhrglass,     schema_checkpoint, {0, 3, 1, 4, 7, 5,    6, 2},
             s2x3,     s1x2,     0, 0, 1, 1,  0x900, schema_nothing},
   {s_spindle,      schema_checkpoint, {7, 1, 3, 5,    0, 2, 4, 6},
             sdmd,     s2x2,     0, 0, 1, 1,  0x87A, schema_rev_checkpoint},
   {s_rigger,       schema_checkpoint, {5, 6, 0, 1, 2, 4,    7, 3},
             s_short6, s1x2,     1, 0, 1, 1,  0x900, schema_nothing},
   {s_bone,         schema_checkpoint, {0, 1, 3, 4, 5, 7,    6, 2},
             s_bone6,  s1x2,     0, 0, 1, 1,  0x900, schema_nothing},
   {s_thar,         schema_concentric, {1, 3, 5, 7,    0, 2, 4, 6},
             s_star,   s_star,   0, 0, 1, 1,  0x8FA, schema_concentric},
   {s_galaxy,       schema_concentric, {1, 3, 5, 7,    0, 2, 4, 6},
             s2x2,     s_star,   0, 0, 1, 1,  0x8FA, schema_concentric},
   // Next two are duplicate, for multiple synthesis.
   {s_wingedstar,   schema_nothing, {2, 3, 6, 7,    0, 1, 4, 5},
             s_star,   s1x4,     0, 0, 1, 1,  0xAFA, schema_conc_star},
   {s_wingedstar,   schema_concentric, {2, 3, 6, 7,    0, 1, 4, 5},
             s_star,   s1x4,     0, 0, 1, 1,  0x8FA, schema_concentric},
   {s3x1dmd,        schema_concentric, {7, 2, 3, 6,    0, 1, 4, 5},
             sdmd,     s1x4,     1, 0, 1, 1,  0x8F5, schema_concentric},
   {s1x3dmd,        schema_concentric, {2, 3, 6, 7,    0, 1, 4, 5},
             sdmd,     s1x4,     0, 0, 1, 1,  0x8FA, schema_concentric},
   {s_crosswave,    schema_concentric, {6, 7, 2, 3,    0, 1, 4, 5},
             s1x4,     s1x4,     1, 0, 1, 1,  0x8F5, schema_concentric},
   {s_crosswave, schema_special_trade_by, {6, 7, 2, 3,    0, 1, 4, 5},
             s1x4,     s1x4,     1, 0, 1, 1,  0x9F5, schema_concentric},
   {s1x8,           schema_concentric, {3, 2, 7, 6,    0, 1, 4, 5},
             s1x4,     s1x4,     0, 0, 1, 1,  0x8EA, schema_concentric},
   {s1x8,     schema_special_trade_by, {3, 2, 7, 6,    0, 1, 4, 5},
             s1x4,     s1x4,     0, 0, 1, 1,  0x9EA, schema_concentric},

   // If doing a "finish" type of thing, and the outsides are box-like but phantoms,
   // assume they were elsewhere.  This shows up in (artificial) test t55t.  So this might
   // not be correct, and doing the really right thing might be quite difficult.
   {s_dhrglass,     schema_concentric_diamond_line, {-1, 7, -1, 3,      -1, 6, -1, 2},
             s1x4,     sdmd,     1, 1, 1, 1,  0x9FE, schema_concentric},

   {s_dhrglass,     schema_concentric, {6, 3, 2, 7,    0, 1, 4, 5},
             sdmd,     s2x2,     0, 0, 1, 1,  0x8FE, schema_concentric},
   {s_hrglass,      schema_concentric, {6, 3, 2, 7,    0, 1, 4, 5},
             sdmd,     s2x2,     0, 0, 2, 1,  0x8FB, schema_concentric},
   {s_rigger,       schema_concentric, {0, 1, 4, 5,    6, 7, 2, 3},
             s2x2,     s1x4,     0, 0, 1, 1,  0x8EA, schema_concentric},
   {s_rigger, schema_special_trade_by, {0, 1, 4, 5,    6, 7, 2, 3},
             s2x2,     s1x4,     0, 0, 1, 1,  0x9EA, schema_concentric},
   {s_bone,         schema_concentric, {6, 7, 2, 3,    0, 1, 4, 5},
             s1x4,     s2x2,     0, 0, 1, 1,  0x8FE, schema_concentric},
   {s_bone,   schema_special_trade_by, {6, 7, 2, 3,    0, 1, 4, 5},
             s1x4,     s2x2,     0, 0, 1, 1,  0x9FE, schema_concentric},
   {s_qtag,         schema_concentric, {6, 7, 2, 3,    0, 1, 4, 5},
             s1x4,     s2x2,     0, 0, 2, 1,  0x8FB, schema_concentric},
   {s_qtag,   schema_special_trade_by, {6, 7, 2, 3,    0, 1, 4, 5},
             s1x4,     s2x2,     0, 0, 2, 1,  0x9FB, schema_concentric},
   {s2x6,           schema_special_trade_by, {2, 3, 8, 9,    0, 1, 4, 5, 6, 7, 10, 11},
             s2x2,     s2x4,     0, 0, 1, 1,  0x9FE, schema_concentric},

   {nothing, schema_nothing, {0}, nothing, nothing},
};


// Rotmask is the mask of things that we will reject.  The low 4 bits are rotations
// that we will reject.  It is ANDed with "1 << r".  R is the rotation of res1,
// after localizing so that res2 has rotation zero.  Hence r=0 if the two setups
// have the same orientation, and, except in the case of things like triangles,
// r=1 if they are orthogonal.  Common values of the low hex digit of "rotmask"
// are therefore:
//    E demand same orientation
//    D demand orthogonal
//    C either way.
//
// Additionally:
//    The "10" bit means that action must be merge_without_gaps.
//    The "20" bit means that action must NOT be merge_strict_matrix or merge_for_own.
//    The "40" bit means only accept it if the setups, prior to cutting down,
//       were a 2x4 and a 1x8 that were perpendicular to each other.
//    The "80" bit means that action must NOT be merge_c1_phantom_real.
//    The "100" bit means that original action must NOT be merge_after_dyp.
//    The "200" bit means only accept it if the setups, prior to cutting down,
//       were a 2x4 and a ptpd that were perpendicular to each other. */
//    The "400" bit means that action must NOT be merge_without_gaps.

// swap_setups
//    1 bit - swap setups;
//    2 bit - change elongation;
//    4 bit - no take right hands
//    8 bit - force outer_elong to 3, so people will go to corners of 4x4;
// 0x10 bit - reject map if it leads to a collision;


//                             rotmask |     | swap_setups
merge_table::concmerge_thing merge_table::merge_init_table[] = {
   {s_thar, s_alamo,     0xCC,  0xCC, 0xAE, 0x0, schema_matrix,         s_qtag,      nothing,  warn__none, 0, 0, {6, 7, -1, -1, 2, 3, -1, -1}, {0, 1, -1, -1, 4, 5, -1, -1}},
   {s_thar, s_alamo,     0x33,  0x33, 0xAE, 0x0, schema_matrix,         s_qtag,      nothing,  warn__none, 0, 1, {-1, -1, 6, 7, -1, -1, 2, 3}, {-1, -1, 0, 1, -1, -1, 4, 5}},
   {s_spindle, s_spindle, 0xAA, 0x55, 0x0D, 0x1, schema_concentric,     sdmd,        s2x2,     warn__none, 0, 0, {7, 1, 3, 5},               {0, 2, 4, 6}},
   {s_spindle, s_spindle, 0x55, 0xAA, 0x0D, 0x0, schema_concentric,     sdmd,        s2x2,     warn__none, 0, 0, {7, 1, 3, 5},               {0, 2, 4, 6}},
   {s_spindle,     s2x4, 0x55,  0x66, 0x0D, 0x0, schema_concentric,     sdmd,        s2x2,     warn__none, 0, 0, {7, 1, 3, 5},               {0, 3, 4, 7}},
   {s_ptpd,     sbigptpd, 0,  0,    0x0E, 0x0, schema_nothing,          nothing,     nothing,  warn__none, 0, 0, {2, 4, 3, 1, 8, 10, 9, 7}, {0}},

   {s1x3p1dmd,     s1x8,  0,  0x0C, 0x0B, 0x0, schema_matrix,         s1x4p2dmd,     nothing,  warn__none, 2, 2, {1, 2, 6, 7, 3, 4}, {5, 6, -1, -1, 0, 1, 3, 2}},
   {s1x3p1dmd,     s1x8,  0,  0xC0, 0x0E, 0x0, schema_matrix,         s1x4p2dmd,     nothing,  warn__none, 0, 0, {1, 2, 3, 4, 6, 7}, {0, 1, 3, 2, 5, 6, -1, -1}},

   {s1x4p2dmd, s4p2x1dmd,   0, 0xD8, 0x0E, 0x0, schema_matrix,        s1x4p2dmd,     nothing,  warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 2, -1, -1, 5, -1, -1}},
   {s1x4p2dmd, s4p2x1dmd, 0xD8,   0, 0x0E, 0x0, schema_matrix,        s4p2x1dmd,     nothing,  warn__none, 0, 0, {0, 1, 2, -1, -1, 5, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},

   {s1x6,    sdbltrngl4,  010, 0x0C, 0x0E, 0x0, schema_matrix,         sline6box,     nothing,  warn__none, 0, 0, {1, 2, 3, -1, 5, 4}, {0, 1, -1, -1, 4, 5, 6, 7}},

   {s3p1x1dmd,     s1x8, 0x03, 0x4E, 0x0B, 0x0, schema_matrix,        s4p2x1dmd,     nothing,  warn__none, 2, 2, {-1, -1, 6, 7, 3, 4}, {5, -1, -1, -1, 0, 1, -1, 2}},
   {s3p1x1dmd,     s1x8, 0x03, 0xE4, 0x0E, 0x0, schema_matrix,        s4p2x1dmd,     nothing,  warn__none, 0, 0, {-1, -1, 3, 4, 6, 7}, {0, 1, -1, 2, 5, -1, -1, -1}},

   {s1x8,     s1x4p2dmd, 0xC0,    0, 0x0E, 0x0, schema_matrix,        s1x4p2dmd,     nothing,  warn__none, 0, 0, {0, 1, 3, 2, 5, 6, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {s1x8,     s4p2x1dmd, 0xE4,    0, 0x0E, 0x0, schema_matrix,        s4p2x1dmd,     nothing,  warn__none, 0, 0, {0, 1, -1, 2, 5, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},

   {s1x6,     s4p2x1dmd,    0, 0xD8, 0x0E, 0x0, schema_matrix,        s1x8,          nothing,  warn__none, 0, 0, {1, 3, 2, 5, 7, 6}, {0, 1, 3, -1, -1, 4, -1, -1}},

   {s1x8,     splinepdmd,  0xF0,   0, 0x0E, 0x0, schema_matrix,       splinepdmd,    nothing,  warn__none, 0, 0, {0, 1, 3, 2, -1, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {s1x8,     splinedmd,  0xF0,    0, 0x0E, 0x0, schema_matrix,       splinedmd,     nothing,  warn__none, 0, 0, {0, 1, 3, 2, -1, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},

   {s1x8,       s2x4,     0x30, 0x6F, 0x2D, 0x0, schema_matrix,       splinedmd,     nothing,  warn__none, 0, 1, {0, 1, 3, 2, -1, -1, 4, 6}, {-1, -1, -1, -1, 5, -1, -1, 7}},
   {s1x8,       s2x4,     0x03, 0xF6, 0x2D, 0x0, schema_matrix,       splinedmd,     nothing,  warn__none, 0, 3, {-1, -1, 4, 6, 0, 1, 3, 2}, {5, -1, -1, 7, -1, -1, -1, -1}},

   {s1x8,       s2x4,     0xF0, 0xC3, 0x0E, 0x0, schema_matrix,        slinebox,     nothing,  warn__none, 0, 0, {0, 1, 3, 2, -1, -1, -1, -1}, {-1, -1, 4, 5, 6, 7, -1, -1}},
   {s1x8,       s2x4,     0x0F, 0x3C, 0x0E, 0x0, schema_matrix,        slinebox,     nothing,  warn__none, 0, 2, {-1, -1, -1, -1, 0, 1, 3, 2}, {6, 7, -1, -1, -1, -1, 4, 5}},

   {s1x8,       s2x4,     0xFC, 0x81, 0x0E, 0x0, schema_matrix,        sline2box,    nothing,  warn__none, 0, 0, {0, 1, -1, -1, -1, -1, -1, -1}, {-1, 2, 3, 4, 5, 6, 7, -1}},
   {s1x8,       s2x4,     0xCF, 0x18, 0x0E, 0x0, schema_matrix,        sline2box,    nothing,  warn__none, 0, 2, {-1, -1, -1, -1, 0, 1, -1, -1}, {5, 6, 7, -1, -1, 2, 3, 4}},
   {s2x3,   s_rigger,      063, 0x0C, 0x0E, 0x0, schema_matrix,        sline2box,    nothing,  warn__none, 0, 0, {-1, -1, 4, 5, -1, -1}, {2, 3, -1, -1, 6, 7, 0, 1}},
   {s2x3,   s_rigger,      036, 0xC0, 0x0E, 0x0, schema_matrix,        sline2box,    nothing,  warn__none, 0, 2, {5, -1, -1, -1, -1, 4}, {6, 7, 0, 1, 2, 3, -1, -1}},
   {s1x8,  sline2box,     0x03, 0xFC, 0x0E, 0x1, schema_nothing,         nothing,    nothing,  warn__none, 0, 0, {0, 1, -1, -1, -1, -1, -1, -1}},
   {s1x8,  sline6box,     0xCF, 0xC0, 0x0E, 0x1, schema_nothing,         nothing,    nothing,  warn__none, 0, 0, {0, 1, 3, 2, 6, 7, -1, -1}},
   {s1x4,  sline6box,        0, 0x3C, 0x0E, 0x0, schema_nothing,         nothing,    nothing,  warn__none, 0, 0, {2, 3, 5, 4}},
   {s2x2,  sline6box,        0, 0x3C, 0x0E, 0x0, schema_matrix,        sline2box,    nothing,  warn__none, 0, 0, {2, 3, 6, 7}, {0, 1, -1, -1, -1, -1, 4, 5}},

   {s2x4,  sline2box,     0x7E, 0x03, 0x0E, 0x1, schema_nothing,         nothing,    nothing,  warn__none, 0, 0, {-1, -1, 1, 2, 3, 4, 5, 6}},
   {s1x4,  sline2box,        0, 0xCC, 0x0E, 0x0, schema_matrix,        sline6box,    nothing,  warn__none, 0, 0, {2, 3, 5, 4}, {0, 1, -1, -1, 6, 7, -1, -1}},
   {s1x6,  sline2box,      047, 0x30, 0x0E, 0x0, schema_matrix,         s_rigger,    nothing,  warn__none, 0, 0, {-1, -1, -1, 2, 3, -1}, {6, 7, 0, 1, -1, -1, 4, 5}},
   {s2x2,  sline2box,        0, 0xCC, 0x0E, 0x0, schema_nothing,         nothing,    nothing,  warn__none, 0, 0, {2, 3, 6, 7}},
   {s2x2,  sline2box,      0x9, 0x48, 0x0E, 0x0, schema_nothing,         nothing,    nothing,  warn__none, 0, 0, {-1, 3, 6, -1}},
   {s2x4,  sline6box,     0x7E, 0x03, 0x0E, 0x0, schema_matrix,           s_bone,    nothing,  warn__none, 0, 0, {0, -1, -1, -1, -1, -1, -1, 5}, {-1, -1, 6, 7, 3, 2, 1, 4}},
   {s2x4,  sline6box,     0x18, 0x3F, 0x0E, 0x1, schema_nothing,         nothing,    nothing,  warn__none, 0, 0, {-1, -1, -1, -1, -1, -1, 3, 4}},

   {s1x8,       s2x4,     0x30, 0xE7, 0x0E, 0x0, schema_matrix,        sline6box,    nothing,  warn__none, 0, 0, {0, 1, 3, 2, -1, -1, 4, 5}, {-1, -1, -1, 6, 7, -1, -1, -1}},
   {s1x8,       s2x4,     0x03, 0x7E, 0x0E, 0x0, schema_matrix,        sline6box,    nothing,  warn__none, 0, 2, {-1, -1, 4, 5, 0, 1, 3, 2}, {7, -1, -1, -1, -1, -1, -1, 6}},
   {s_bone,     s1x8,     0x21, 0xFC, 0x0E, 0x0, schema_matrix,        sline6box,    nothing,  warn__none, 0, 0, {-1, 6, 5, 4, 7, -1, 2, 3}, {0, 1, -1, -1, -1, -1, -1, -1}},
   {s_bone,     s1x8,     0x12, 0xCF, 0x0E, 0x0, schema_matrix,        sline6box,    nothing,  warn__none, 0, 2, {7, -1, 2, 3, -1, 6, 5, 4}, {-1, -1, -1, -1, 0, 1, -1, -1}},

   {s1x4,  sline2box,      0x3, 0x48, 0x0E, 0x0, schema_matrix,        sdbltrngl4,   nothing,  warn__none, 0, 0, {-1, -1, 5, 4}, {0, 1, 2, -1, 6, 7, -1, 3}},
   {s1x4,  sline6box,        0, 0,    0x0E, 0x0, schema_nothing,         nothing,    nothing,  warn__none, 0, 0, {2, 3, 5, 4}},
   {s2x2,  sline6box,      0x6, 0x0C, 0x0E, 0x0, schema_matrix,        sdbltrngl4,   nothing,  warn__none, 0, 0, {2, -1, -1, 3}, {0, 1, -1, -1, 4, 5, 6, 7}},

   {s2x2,  sdbltrngl4,     0x9, 0x30, 0x0E, 0x0, schema_matrix,        sline2box,   nothing,  warn__none, 0, 0, {-1, 3, 6, -1}, {0, 1, 2, 7, -1, -1, 4, 5}},
   {s1x4,  sdbltrngl4,     0xC, 0x0C, 0x0E, 0x0, schema_matrix,        sline6box,   nothing,  warn__none, 0, 0, {2, 3, -1, -1}, {0, 1, -1, -1, 4, 5, 6, 7}},
   {s2x4,  sdbltrngl4,    0x5A, 0x33, 0x0E, 0x1, schema_nothing,         nothing,    nothing,  warn__none, 0, 0, {-1, -1, 1, 6, -1, -1, 3, 4}},
   {s1x8,  sdbltrngl4,    0xC3, 0xCC, 0x0E, 0x1, schema_nothing,         nothing,    nothing,  warn__none, 0, 0, {0, 1, -1, -1, 6, 7, -1, -1}},

   {s1x8,       s2x4,     0x3C, 0xA5, 0x0E, 0x0, schema_matrix,        sdbltrngl4,   nothing,  warn__none, 0, 0, {0, 1, -1, -1, -1, -1, 4, 5}, {-1, 2, -1, 6, 7, -1, 3, -1}},
   {s1x8,       s2x4,     0xC3, 0x5A, 0x0E, 0x0, schema_matrix,        sdbltrngl4,   nothing,  warn__none, 0, 2, {-1, -1,  4,  5,  0,  1, -1, -1}, {7, -1, 3, -1, -1, 2, -1, 6}},

   {s1x8,     splinepdmd,    0, 0xF0, 0x0E, 0x0, schema_matrix,         s1x8,        nothing,  warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 3, 2, -1, -1, -1, -1}},
   {s1x8,     splinedmd,     0, 0xF0, 0x0E, 0x0, schema_matrix,         s1x8,        nothing,  warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 3, 2, -1, -1, -1, -1}},

   {s2x4,     splinepdmd, 0xF0, 0xF0, 0x0D, 0x0, schema_matrix,      s_trngl8,       nothing,  warn__none, 0, 1, {4, 5, 6, 7, -1, -1, -1, -1}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {s2x4,     sdmdpdmd,   0xF0, 0xF0, 0x0E, 0x0, schema_matrix,     slinepdmd,       nothing,  warn__none, 0, 0, {4, 5, 7, 6, -1, -1, -1, -1}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {s2x4,       slinedmd, 0x3C, 0xF0, 0x0D, 0x0, schema_matrix,       sboxdmd,       nothing,  warn__none, 0, 0, {5, 6, -1, -1, -1, -1, 7, 4}, {0, 1, 2, 3, -1, -1, -1, -1}},

   {s2x4,      splinedmd, 0x3C, 0x0F, 0x0E, 0x0, schema_matrix,       sboxdmd,       nothing,  warn__none, 0, 3, {5, 6, -1, -1, -1, -1, 7, 4}, {-1, -1, -1, -1, 1, 2, 3, 0}},
   {s2x4,     splinepdmd, 0x3C, 0x0F, 0x0E, 0x0, schema_matrix,      sboxpdmd,       nothing,  warn__none, 0, 3, {5, 6, -1, -1, -1, -1, 7, 4}, {-1, -1, -1, -1, 2, 3, 0, 1}},
   {s2x4,       slinebox,    0, 0x0F, 0x0E, 0x0, schema_matrix,          s2x4,       nothing,  warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {-1, -1, -1, -1, 2, 3, 4, 5}},

   {s2x4,       slinebox, 0xF0, 0xF0, 0x0D, 0x0, schema_matrix,          s_trngl8,       nothing,  warn__none, 0, 1, {4, 5, 6, 7, -1, -1, -1, -1}, {0, 1, 2, 3, -1, -1, -1, -1}},

   {s1x8,       slinebox, 0x0F, 0xF0, 0x0E, 0x0, schema_matrix,          s1x8,       nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 4, 5, 6, 7}, {0, 1, 3, 2, -1, -1, -1, -1}},

   {s2x4,       slinebox, 0x0F, 0x0F, 0x0D, 0x0, schema_matrix,          s4x4,       nothing,  warn__none, 0, 1, {-1, -1, -1, -1, 2, 7, 11, 9}, {-1, -1, -1, -1, 15, 13, 14, 3}},

   {s2x4,       s2x4,     0x3C, 0x0F, 0x0D, 0x0, schema_matrix,      s_c1phan,       nothing,  warn__none, 0, 1, {0, 2, -1, -1, -1, -1, 15, 13}, {-1, -1, -1, -1, 4, 6, 11, 9}},
   {s2x4,       s2x4,     0xC3, 0xF0, 0x0D, 0x0, schema_matrix,      s_c1phan,       nothing,  warn__none, 0, 1, {-1, -1, 7, 5, 8, 10, -1, -1}, {12, 14, 3, 1, -1, -1, -1, -1}},
   {s2x4,       s2x4,     0x0F, 0xC3, 0x0D, 0x0, schema_matrix,      s_c1phan,       nothing,  warn__none, 0, 2, {-1, -1, -1, -1, 4, 6, 11, 9}, {-1, -1, 15, 13, 0, 2, -1, -1}},
   {s2x4,       s2x4,     0xF0, 0x3C, 0x0D, 0x0, schema_matrix,      s_c1phan,       nothing,  warn__none, 0, 0, {4, 6, 11, 9, -1, -1, -1, -1}, {0, 2, -1, -1, -1, -1, 15, 13}},

   {s2x4,      slinepdmd, 0x3C, 0xF0, 0x0D, 0x0, schema_matrix,      sboxpdmd,       nothing,  warn__none, 0, 0, {5, 6, -1, -1, -1, -1, 7, 4}, {0, 1, 2, 3, -1, -1, -1, -1}},

   {s_qtag,      s2x4,    0xE1, 0x3C, 0x0E, 0x0, schema_matrix,       sboxdmd,       nothing,  warn__none, 0, 3, {-1, 2, 3, 1, 0, -1, -1, -1}, {5, 6, -1, -1, -1, -1, 7, 4}},
   {s_qtag,      s2x4,    0xE1, 0xF0, 0x0D, 0x0, schema_matrix,      slinedmd,       nothing,  warn__none, 0, 0, {-1, 2, 3, 1, 0, -1, -1, -1}, {4, 5, 7, 6, -1, -1, -1, -1}},
   {s_qtag,      s_ptpd,  0xE1, 0xF0, 0x0E, 0x0, schema_matrix,      sdmdpdmd,       nothing,  warn__none, 0, 1, {-1, 4, 5, 7, 6, -1, -1, -1}, {0, 1, 2, 3, -1, -1, -1, -1}},

   {s2x4,       s_trngl8, 0x3C, 0xF0, 0x0D, 0x0, schema_matrix,      slinebox,       nothing,  warn__none, 0, 3, {6, 7, -1, -1, -1, -1, 4, 5}, {0, 1, 2, 3, -1, -1, -1, -1}},

   {s_ptpd,   slinedmd,   0xF0, 0xF0, 0x0D, 0x0, schema_matrix,       sdmdpdmd,      nothing,  warn__none, 0, 2, {0, 1, 2, 3, -1, -1, -1, -1}, {6, 7, 4, 5, -1, -1, -1, -1}},
   {s_ptpd,   splinedmd,  0xF0, 0x0F, 0x0E, 0x0, schema_matrix,       sdmdpdmd,      nothing,  warn__none, 0, 1, {0, 1, 2, 3, -1, -1, -1, -1}, {-1, -1, -1, -1, 7, 4, 5, 6}},
   {s_ptpd,   slinepdmd,     0, 0xF0, 0x0D, 0x0, schema_matrix,         s_ptpd,      nothing,  warn__none, 0, 1, {0, 1, 2, 3, 4, 5, 6, 7}, {4, 5, 6, 7, -1, -1, -1, -1}},
   {s_ptpd,   splinepdmd,    0, 0x0F, 0x0E, 0x0, schema_matrix,         s_ptpd,      nothing,  warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {-1, -1, -1, -1, 6, 7, 4, 5}},

   {s1x8,     slinepdmd,  0xF0, 0xF0, 0x0D, 0x0, schema_matrix,        splinepdmd,  nothing,  warn__none, 0, 1, {0, 1, 3, 2, -1, -1, -1, -1}, {6, 7, 4, 5, -1, -1, -1, -1}},

   {s_ptpd,   s_trngl8,   0x0F, 0x0F, 0x0D, 0x0, schema_matrix,      slinepdmd,      nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 0, 1, 2, 3}, {-1, -1, -1, -1, 4, 5, 7, 6}},

   {s_qtag,   splinedmd,     0, 0x0F, 0x0E, 0x0, schema_matrix,         s_qtag,      nothing,  warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {-1, -1, -1, -1, 3, 1, 2, 4}},
   {s_qtag,   slinedmd,      0, 0xF0, 0x0D, 0x0, schema_matrix,         s_qtag,      nothing,  warn__none, 0, 1, {0, 1, 2, 3, 4, 5, 6, 7}, {4, 3, 1, 2, -1, -1, -1, -1}},
   {s_qtag,   s_trngl8,   0x1E, 0xF0, 0x0D, 0x0, schema_matrix,      splinedmd,      nothing,  warn__none, 0, 3, {7, -1, -1, -1, -1, 5, 6, 4}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {s_qtag,   s_trngl8,   0xE1, 0x0F, 0x0D, 0x0, schema_matrix,       slinedmd,      nothing,  warn__none, 0, 0, {-1, 2, 3, 1, 0, -1, -1, -1}, {-1, -1, -1, -1, 4, 5, 7, 6}},
   {s_qtag,  slinepdmd,   0x1E,    0, 0x0D, 0x0, schema_matrix,       sdmdpdmd,      nothing,  warn__none, 0, 0, {6, -1, -1, -1, -1, 4, 5, 7}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {s_qtag, splinepdmd,   0x1E, 0x0F, 0x0E, 0x0, schema_matrix,       sdmdpdmd,      nothing,  warn__none, 0, 3, {6, -1, -1, -1, -1, 4, 5, 7}, {-1, -1, -1, -1, 2, 3, 0, 1}},
   {s_qtag,   slinebox,   0x1E, 0x0F, 0x0E, 0x0, schema_matrix,        sboxdmd,      nothing,  warn__none, 0, 1, {0, -1, -1, -1, -1, 2, 3, 1}, {-1, -1, -1, -1, 7, 4, 5, 6}},
   {s_qtag, sboxdmd,      0xE1,    0, 0x0D, 0x0, schema_matrix,        sboxdmd,      nothing,  warn__none, 0, 0, {-1, 2, 3, 1, 0, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {s_qtag, sboxpdmd,     0x1E, 0xF0, 0x0D, 0x0, schema_matrix,       sdmdpdmd,      nothing,  warn__none, 0, 0, {6, -1, -1, -1, -1, 4, 5, 7}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {s_qtag, sboxdmd,      0,    0xF0, 0x0D, 0x0, schema_matrix,         s_qtag,      nothing,  warn__none, 0, 1, {0, 1, 2, 3, 4, 5, 6, 7}, {4, 3, 1, 2, -1, -1, -1, -1}},

   {s_ptpd, sboxpdmd,     0x0F,    0, 0x0D, 0x0, schema_matrix,       sboxpdmd,      nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 0, 1, 2, 3}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {slinepdmd, sboxpdmd,  0xF0,    0, 0x0E, 0x0, schema_matrix,       sboxpdmd,      nothing,  warn__none, 0, 0, {0, 1, 2, 3, -1, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {slinedmd,  sboxpdmd,  0xF0, 0x0F, 0x0E, 0x0, schema_matrix,        sboxdmd,      nothing,  warn__none, 0, 0, {0, 1, 2, 3, -1, -1, -1, -1}, {-1, -1, -1, -1, 4, 5, 6, 7}},
   {s1x8,      sboxpdmd,  0x0F, 0x0B, 0x0D, 0x0, schema_matrix,       slinebox,      nothing,  warn__none, 0, 3, {-1, -1, -1, -1, 0, 1, 3, 2}, {-1, -1, 3, -1, 5, 6, 7, 4}},
   {s1x8,      sboxpdmd,  0xF0, 0xF0, 0x0D, 0x0, schema_matrix,       splinepdmd,    nothing,  warn__none, 0, 1, {0, 1, 3, 2, -1, -1, -1, -1}, {6, 7, 4, 5, -1, -1, -1, -1}},
   {s1x8,       sboxdmd,  0x0F, 0x0F, 0x0D, 0x0, schema_matrix,       slinebox,      nothing,  warn__none, 0, 3, {-1, -1, -1, -1, 0, 1, 3, 2}, {-1, -1, -1, -1, 5, 6, 7, 4}},

   {s2x4,   sboxpdmd,     0x0F, 0x0F, 0x0E, 0x0, schema_matrix,         s4x4,        nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 2, 7, 11, 9}, {-1, -1, -1, -1, 13, 14, 3, 15}},
   {s2x4,   sboxpdmd,     0xF0, 0xF0, 0x0E, 0x0, schema_matrix,       slinepdmd,     nothing,  warn__none, 0, 0, {4, 5, 7, 6, -1, -1, -1, -1}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {s2x4,   sboxpdmd,     0x3C, 0xF0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {5, 6, -1, -1, -1, -1, 7, 4}, {0}},
   {sboxpdmd, sboxpdmd,     0xF0, 0xF0, 0x0B, 0x0, schema_matrix,        s_ptpd,     nothing,  warn__none, 0, 1, {0, 1, 2, 3, -1, -1, -1, -1}, {4, 5, 6, 7, -1, -1, -1, -1}},


   {s2x4,   slinedmd,     0xC3, 0x0F, 0x0D, 0x0, schema_matrix,         s4x4,        nothing,  warn__none, 0, 0, {-1, -1, 7, 5, 6, 11, -1, -1}, {-1, -1, -1, -1, 10, 15, 1, 3}},
   {s2x4,   slinepdmd,    0xC3, 0x0F, 0x0D, 0x0, schema_matrix,         s4x4,        nothing,  warn__none, 0, 0, {-1, -1, 7, 5, 6, 11, -1, -1}, {-1, -1, -1, -1, 10, 15, 1, 3}},
   {s2x4,   splinedmd,    0xC3, 0xF0, 0x0E, 0x0, schema_matrix,     slinebox,        nothing,  warn__none, 0, 0, {-1, -1, 4, 5, 6, 7, -1, -1}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {s2x4,   splinedmd,    0x0F, 0x0F, 0x0D, 0x0, schema_matrix,     slinedmd,        nothing,  warn__none, 0, 3, {-1, -1, -1, -1, 4, 5, 7, 6}, {-1, -1, -1, -1, 1, 2, 3, 0}},
   {s2x4,   splinepdmd,   0xC3, 0xF0, 0x0E, 0x0, schema_matrix,     slinebox,        nothing,  warn__none, 0, 0, {-1, -1, 4, 5, 6, 7, -1, -1}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {s2x4,   splinepdmd,   0x0F, 0x0F, 0x0D, 0x0, schema_matrix,     slinepdmd,       nothing,  warn__none, 0, 3, {-1, -1, -1, -1, 4, 5, 7, 6}, {-1, -1, -1, -1, 2, 3, 0, 1}},
   {s2x4,   sboxpdmd,        0, 0x0F, 0x0D, 0x0, schema_matrix,         s2x4,        nothing,  warn__none, 0, 1, {0, 1, 2, 3, 4, 5, 6, 7}, {-1, -1, -1, -1, 7, 0, 1, 6}},

   {s_qtag,      s2x4,    0x1E, 0xC3, 0x0E, 0x0, schema_matrix,      sboxdmd,        nothing,  warn__none, 0, 1, {0, -1, -1, -1, -1, 2, 3, 1}, {-1, -1, 7, 4, 5, 6, -1, -1}},

   {s2x4,   sdmdpdmd,     0x3C, 0xF0, 0x0D, 0x0, schema_matrix,     sboxpdmd,        nothing,  warn__none, 0, 0, {5, 6, -1, -1, -1, -1, 7, 4}, {0, 1, 2, 3, -1, -1, -1, -1}},

   {s2x4,   sdmdpdmd,     0xC3, 0x0F, 0x0D, 0x0, schema_matrix,      sboxdmd,        nothing,  warn__none, 0, 2, {-1, -1, 7, 4, 5, 6, -1, -1}, {-1, -1, -1, -1, 2, 3, 0, 1}},

   {s2x4,     s_ptpd,     0xC3, 0xF0, 0x0E, 0x0, schema_matrix,     sboxpdmd,        nothing,  warn__none, 0, 1, {-1, -1, 7, 4, 5, 6, -1, -1}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {s2x4,     s_ptpd,     0x3C, 0x0F, 0x0E, 0x0, schema_matrix,     sboxpdmd,        nothing,  warn__none, 0, 3, {5, 6, -1, -1, -1, -1, 7, 4}, {-1, -1, -1, -1, 0, 1, 2, 3}},

   {s2x3,    splinedmd,      0, 0x5F, 0x0E, 0x0, schema_matrix,         s4x4,        nothing,  warn__none, 0, 0, {10, 15, 3, 7, 11, 9}, {-1, -1, -1, -1, -1, 14, -1, 5}},

   {slinedmd,  sdmdpdmd,  0xF0,    0, 0x0B, 0x0, schema_matrix,        sdmdpdmd,     nothing,  warn__none, 0, 0, {6, 7, 4, 5, -1, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {slinedmd,  sdmdpdmd,  0xF0, 0x0F, 0x0E, 0x0, schema_matrix,          s_qtag,     nothing,  warn__none, 0, 1, {4, 3, 1, 2, -1, -1, -1, -1}, {-1, -1, -1, -1, 5, 6, 0, 7}},

   {slinedmd,  sboxdmd,   0xF0,    0, 0x0E, 0x0, schema_matrix,         sboxdmd,     nothing,  warn__none, 0, 0, {0, 1, 2, 3, -1, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {slinedmd,  sboxdmd,   0,    0xF0, 0x0E, 0x0, schema_matrix,        slinedmd,     nothing,  warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 2, 3, -1, -1, -1, -1}},

   {slinedmd,  slinebox,  0x0F, 0x0F, 0x07, 0x0, schema_matrix,         s4x4,        nothing,  warn__none, 0, 3, {-1, -1, -1, -1, 10, 15, 1, 3}, {-1, -1, -1, -1, 7, 5, 6, 11}},

   {slinepdmd,  sdmdpdmd, 0,    0xF0, 0x0E, 0x0, schema_matrix,       slinepdmd,     nothing,  warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {slinepdmd,  sdmdpdmd, 0xF0,    0, 0x0E, 0x0, schema_matrix,       sdmdpdmd,      nothing,  warn__none, 0, 0, {0, 1, 2, 3, -1, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},

   {slinepdmd,  sdmdpdmd, 0xF0, 0xF0, 0x0B, 0x0, schema_matrix,         s_ptpd,      nothing,  warn__none, 0, 1, {0, 1, 2, 3, -1, -1, -1, -1}, {4, 5, 6, 7, -1, -1, -1, -1}},

   {slinepdmd,  slinedmd,  0,   0x0F, 0x0E, 0x0, schema_matrix,     slinepdmd,       nothing,  warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {-1, -1, -1, -1, 4, 5, 6, 7}},
   {slinepdmd,  slinedmd,  0x0F,   0, 0x0E, 0x0, schema_matrix,      slinedmd,       nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 4, 5, 6, 7}, {0, 1, 2, 3, 4, 5, 6, 7}},

   {slinepdmd,  sboxdmd,  0xF0, 0x0F, 0x0E, 0x0, schema_matrix,       sboxpdmd,      nothing,  warn__none, 0, 0, {0, 1, 2, 3, -1, -1, -1, -1}, {-1, -1, -1, -1, 4, 5, 6, 7}},

   {s_trngl8,   sboxdmd,  0xF0, 0x0F, 0x0E, 0x0, schema_matrix,       slinebox,      nothing,  warn__none, 0, 3, {0, 1, 2, 3, -1, -1, -1, -1}, {-1, -1, -1, -1, 5, 6, 7, 4}},

   {splinedmd,  slinedmd,  0x0F,   0, 0x0D, 0x0, schema_matrix,       slinedmd,      nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 1, 2, 3, 0}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {splinedmd,  slinedmd,  0,   0xF0, 0x0D, 0x0, schema_matrix,       splinedmd,     nothing,  warn__none, 0, 1, {0, 1, 2, 3, 4, 5, 6, 7}, {7, 4, 5, 6, -1, -1, -1, -1}},
   {splinedmd,  sdmdpdmd,  0,   0x0F, 0x07, 0x0, schema_matrix,       splinedmd,     nothing,  warn__none, 0, 3, {0, 1, 2, 3, 4, 5, 6, 7}, {-1, -1, -1, -1, 5, 6, 7, 4}},
   {splinedmd,  sdmdpdmd,  0x0F,   0, 0x07, 0x1, schema_matrix,       sdmdpdmd,      nothing,  warn__none, 0, 1, {0, 1, 2, 3, 4, 5, 6, 7}, {-1, -1, -1, -1, 7, 4, 5, 6}},
   {splinedmd,  sdmdpdmd,  0x0F, 0x0F, 0x0D, 0x0, schema_matrix,        s_qtag,      nothing,  warn__none, 0, 1, {-1, -1, -1, -1, 3, 1, 2, 4}, {-1, -1, -1, -1, 5, 6, 0, 7}},
   {splinedmd, splinedmd, 0x0F, 0x0F, 0x0B, 0x0, schema_matrix,         s_qtag,      nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 7, 5, 6, 0}, {-1, -1, -1, -1, 3, 1, 2, 4}},

   // This one isn't needed (for the regression tests, at least), now that these setups are
   // normalized more aggressively.  There are probably many other maps in the same situation.
   {slinepdmd, slinedmd,  0xF0, 0xF0, 0x0B, 0x0, schema_matrix,       sdmdpdmd,      nothing,  warn__none, 0, 2, {0, 1, 2, 3, -1, -1, -1, -1}, {6, 7, 4, 5, -1, -1, -1, -1}},

   {splinedmd, slinepdmd, 0xF0, 0xF0, 0x0D, 0x0, schema_matrix,     splinepdmd,      nothing,  warn__none, 0, 1, {0, 1, 2, 3, -1, -1, -1, -1}, {6, 7, 4, 5, -1, -1, -1, -1}},
   {splinedmd, slinepdmd,   0x0F, 0x0F, 0x0D, 0x0, schema_matrix,       slinedmd,    nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 1, 2, 3, 0}, {-1, -1, -1, -1, 4, 5, 6, 7}},
   {splinedmd,  sboxpdmd, 0x0F, 0x0F, 0x0D, 0x0, schema_matrix,        sboxdmd,      nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 1, 2, 3, 0}, {-1, -1, -1, -1, 4, 5, 6, 7}},
   {splinedmd,  sboxpdmd, 0x0F, 0xF0, 0x07, 0x0, schema_matrix,       sdmdpdmd,      nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 7, 4, 5, 6}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {splinedmd,   sboxdmd, 0x0F,    0, 0x0D, 0x0, schema_matrix,        sboxdmd,      nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 1, 2, 3, 0}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {splinepdmd, splinepdmd, 0x0F, 0x0F, 0x0B, 0x0, schema_matrix,       s_ptpd,      nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 2, 3, 0, 1}, {-1, -1, -1, -1, 6, 7, 4, 5}},
   {splinepdmd, splinedmd,  0, 0xF0, 0x0E, 0x0, schema_matrix,        splinepdmd,    nothing,  warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {splinepdmd, splinedmd,0xF0,  0, 0x0E, 0x0, schema_matrix,          splinedmd,    nothing,  warn__none, 0, 0, {0, 1, 2, 3, -1, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {splinepdmd, sdmdpdmd, 0x0F,  0, 0x0D, 0x0, schema_matrix,           sdmdpdmd,    nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 2, 3, 0, 1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {splinepdmd, sdmdpdmd, 0x0F, 0xF0, 0x07, 0x0, schema_matrix,           s_ptpd,    nothing,  warn__none, 0, 3, {-1, -1, -1, -1, 6, 7, 4, 5}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {splinepdmd, slinepdmd,  0, 0xF0, 0x0D, 0x0, schema_matrix,        splinepdmd,    nothing,  warn__none, 0, 1, {0, 1, 2, 3, 4, 5, 6, 7}, {6, 7, 4, 5, -1, -1, -1, -1}},
   {splinepdmd, slinepdmd,  0x0F, 0, 0x0D, 0x0, schema_matrix,          slinepdmd,   nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 2, 3, 0, 1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {splinepdmd, slinedmd, 0xF0, 0xF0, 0x0D, 0x0, schema_matrix,        splinedmd,    nothing,  warn__none, 0, 1, {0, 1, 2, 3, -1, -1, -1, -1}, {7, 4, 5, 6, -1, -1, -1, -1}},
   {splinepdmd, slinedmd, 0x0F, 0x0F, 0x0D, 0x0, schema_matrix,        slinepdmd,    nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 2, 3, 0, 1}, {-1, -1, -1, -1, 4, 5, 6, 7}},
   {splinepdmd,  sboxdmd, 0x0F, 0x0F, 0x0D, 0x0, schema_matrix,         sboxpdmd,    nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 2, 3, 0, 1}, {-1, -1, -1, -1, 4, 5, 6, 7}},
   {splinepdmd, sboxpdmd, 0x0F,    0, 0x0D, 0x0, schema_matrix,         sboxpdmd,    nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 2, 3, 0, 1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {splinepdmd, sboxpdmd, 0x0F, 0xF0, 0x07, 0x0, schema_matrix,           s_ptpd,    nothing,  warn__none, 0, 1, {-1, -1, -1, -1, 2, 3, 0, 1}, {4, 5, 6, 7, -1, -1, -1, -1}},
   {splinepdmd, slinebox, 0xF0,    0, 0x0E, 0x0, schema_matrix,         slinebox,    nothing,  warn__none, 0, 0, {0, 1, 2, 3, -1, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {splinepdmd, slinebox, 0x0F, 0xF0, 0x0E, 0x1, schema_nothing,         nothing,    nothing,  warn__none, 0, 0, {0, 1, 2, 3, -1, -1, -1, -1}, {0}},
   {slinepdmd,  slinebox, 0x0F, 0x0F, 0x07, 0x0, schema_matrix,             s4x4,    nothing,  warn__none, 0, 3, {-1, -1, -1, -1, 10, 15, 1, 3}, {-1, -1, -1, -1, 7, 5, 6, 11}},
   {slinebox,   sdmdpdmd, 0x0F, 0x0F, 0x0D, 0x1, schema_matrix,          sboxdmd,    nothing,  warn__none, 0, 1, {-1, -1, -1, -1, 2, 3, 0, 1}, {-1, -1, -1, -1, 7, 4, 5, 6}},
   {slinebox,   sdmdpdmd, 0x0F, 0xF0, 0x07, 0x1, schema_matrix,         sboxpdmd,    nothing,  warn__none, 0, 1, {0, 1, 2, 3, -1, -1, -1, -1}, {-1, -1, -1, -1, 7, 4, 5, 6}},
   {slinebox,    sboxdmd, 0x0F, 0x0F, 0x0D, 0x0, schema_matrix,             s2x4,    nothing,  warn__none, 0, 1, {-1, -1, -1, -1, 2, 3, 4, 5}, {-1, -1, -1, -1, 7, 0, 1, 6}},
   {slinebox,   sboxpdmd, 0x0F, 0x0F, 0x0D, 0x0, schema_matrix,             s2x4,    nothing,  warn__none, 0, 1, {-1, -1, -1, -1, 2, 3, 4, 5}, {-1, -1, -1, -1, 7, 0, 1, 6}},
   {slinebox,   sboxpdmd, 0x0F, 0xF0, 0x07, 0x0, schema_matrix,         sboxpdmd,    nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 7, 4, 5, 6}, {0, 1, 2, 3, -1, -1, -1, -1}},

   {s_trngl8,   sboxpdmd, 0x0F, 0x0F, 0x0B, 0x0, schema_matrix,             s4x4,    nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 2, 7, 11, 9}, {-1, -1, -1, -1, 13, 14, 3, 15}},
   {s_trngl8,    sboxdmd, 0x0F, 0x0F, 0x0B, 0x0, schema_matrix,             s4x4,    nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 2, 7, 11, 9}, {-1, -1, -1, -1, 13, 14, 3, 15}},
   {s_ptpd,     slinebox, 0xF0, 0x0F, 0x0E, 0x0, schema_matrix,         sboxpdmd,    nothing,  warn__none, 0, 1, {0, 1, 2, 3, -1, -1, -1, -1}, {-1, -1, -1, -1, 7, 4, 5, 6}},

   {s2x4,       s_23232,  0x33, 06363, 0x0E, 0x1, schema_nothing,        nothing,    nothing,  warn__may_be_fudgy, 0, 0, {-1, -1, 0, 1, -1, -1, -1, -1, 4, 5, -1, -1}, {0}},
   {s2x4,       s_23232,  0xCC, 04747, 0x0E, 0x1, schema_nothing,        nothing,    nothing,  warn__may_be_fudgy, 0, 0, {-1, -1, -1, 2, 3, -1, -1, -1, -1, 6, 7, -1}, {0}},
   {s2x4,       s_23232,  0xCC, 03535, 0x0E, 0x0, schema_matrix,        s_c1phan,    nothing,  warn__may_be_fudgy, 0, 0, {0, 2, -1, -1, 8, 10, -1, -1}, {-1, 4, -1, -1, -1, 6, -1, 12, -1, -1, -1, 14}},
   {s2x4,       s_23232,  0x33, 03636, 0x0E, 0x0, schema_matrix,        s_c1phan,    nothing,  warn__may_be_fudgy, 0, 0, {-1, -1, 7, 5, -1, -1, 15, 13}, {1, -1, -1, -1, -1, 11, 9, -1, -1, -1, -1, 3}},
   {s2x4,       s_23232,  0x33, 03535, 0x0D, 0x0, schema_matrix,        s_c1phan,    nothing,  warn__may_be_fudgy, 0, 0, {-1, -1, 11, 9, -1, -1, 3, 1}, {-1, 4, -1, -1, -1, 6, -1, 12, -1, -1, -1, 14}},
   {s2x4,       s_23232,  0xCC, 03636, 0x0D, 0x0, schema_matrix,        s_c1phan,    nothing,  warn__may_be_fudgy, 0, 0, {4, 6, -1, -1, 12, 14, -1, -1}, {1, -1, -1, -1, -1, 11, 9, -1, -1, -1, -1, 3}},
   {s1x2,        s_ptpd,     0, 0x44, 0x0C, 0x0, schema_concentric_2_6,     s1x2,   s_short6,  warn__none, 0, 1, {0, 1}, {7, 4, 5, 3, 0, 1}},
   {s1x8,        s_ptpd,  0xA0, 0x0A, 0x0E, 0x0, schema_matrix,       splinepdmd,    nothing,  warn__none, 0, 0, {0, 1, 3, 2, 6, -1, 4, -1}, {0, -1, 3, -1, 6, 7, 4, 5}},
   {s1x8,        s_ptpd,  0x0A, 0xA0, 0x0E, 0x0, schema_matrix,       splinepdmd,    nothing,  warn__none, 0, 2, {6, -1, 4, -1, 0, 1, 3, 2}, {6, 7, 4, 5, 0, -1, 3, -1}},

   {s1x8,        s3ptpd,  0xAA, 00707, 0x0E, 0x0, schema_matrix,       s1x10,        nothing,  warn__none, 0, 0, {1, -1, 4, -1, 6, -1, 9, -1}, {-1, -1, -1, 5, 7, 8, -1, -1, -1, 0, 2, 3}},

   {s2x4,        s_ptpd,  0xF0, 0xF0, 0x0D, 0x0, schema_matrix,        slinepdmd,    nothing,  warn__none, 0, 1, {4, 5, 7, 6, -1, -1, -1, -1}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {s2x4,        s_ptpd,  0x0F, 0x0F, 0x0D, 0x0, schema_matrix,        slinepdmd,    nothing,  warn__none, 0, 3, {-1, -1, -1, -1, 4, 5, 7, 6}, {-1, -1, -1, -1, 0, 1, 2, 3}},

   {s_qtag,        s1x8,  0xE1, 0xF0, 0x0E, 0x0, schema_matrix,       splinedmd,     nothing,  warn__none, 0, 0, {-1, 5, 6, 4, 7, -1, -1, -1}, {0, 1, 3, 2, -1, -1, -1, -1}},
   {s_qtag,        s1x8,  0x1E, 0x0F, 0x0E, 0x0, schema_matrix,       splinedmd,     nothing,  warn__none, 0, 2, {7, -1, -1, -1, -1, 5, 6, 4}, {-1, -1, -1, -1, 0, 1, 3, 2}},
   {s_qtag,        s_ptpd,  0x1E, 0x0F, 0x0E, 0x0, schema_matrix,      sdmdpdmd,     nothing,  warn__none, 0, 3, {6, -1, -1, -1, -1, 4, 5, 7}, {-1, -1, -1, -1, 0, 1, 2, 3}},

   {s_1x2dmd,      s1x8,     0, 0x44, 0x0E, 0x0, schema_matrix,         s1x3dmd,     nothing,  warn__none, 0, 0, {1, 2, 3, 5, 6, 7}, {0, 1, -1, 2, 4, 5, -1, 6}},
   {s_1x2dmd,      s1x8,   044,    0, 0x0E, 0x0, schema_matrix,         s1x8,        nothing,  warn__none, 0, 0, {1, 3, -1, 5, 7, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {s_1x2dmd,  s_galaxy,   022, 0xAA, 0x1E, 0x0, schema_matrix,         s_crosswave, nothing,  warn__none, 0, 0, {0, -1, 3, 4, -1, 7}, {1, -1, 2, -1, 5, -1, 6, -1}},
   {s_1x2dmd,      s2x4,   022, 0x66, 0x0C, 0x0, schema_concentric,     sdmd,        s2x2,     warn__none, 0, 0, {0, 2, 3, 5},               {0, 3, 4, 7}},
   {sdmd,          s2x4,     0, 0x66, 0x0C, 0x0, schema_concentric,     sdmd,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 3, 4, 7}},
   {sdmd,          s2x3,     0,  022, 0x0C, 0x0, schema_concentric,     sdmd,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 2, 3, 5}},
   {s1x4,      s_galaxy,     0, 0xAA, 0x0C, 0x0, schema_concentric,     s1x4,        sdmd,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 2, 4, 6}},
   {s2x4,        s_thar,  0x66, 0x33, 0x0E, 0x1, schema_concentric,     s1x4,        s2x2,     warn__none, 1, 0, {2, 3, 6, 7},               {0, 3, 4, 7}},
   {s2x4,        s_thar,  0x66, 0xCC, 0x0D, 0x1, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {0, 1, 4, 5},               {0, 3, 4, 7}},
   {s2x3,        s_thar,   022, 0x33, 0x0E, 0x1, schema_concentric,     s1x4,        s2x2,     warn__none, 1, 0, {2, 3, 6, 7},               {0, 2, 3, 5}},
   {s2x3,        s_thar,   022, 0xCC, 0x0D, 0x1, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {0, 1, 4, 5},               {0, 2, 3, 5}},
   {s1x2,       s1x3dmd,     0, 0x88, 0x0C, 0x0, schema_concentric_2_6, s1x2,        s1x6,     warn__none, 0, 0, {0, 1}, {0, 1, 2, 4, 5, 6}},
   {s1x2,        s_hrglass,  0, 0x88, 0x0C, 0x2, schema_concentric_2_6, s1x2,        s_short6, warn__none, 0, 1, {0, 1},                     {1, 2, 4, 5, 6, 0}},
   {s1x2,        s_qtag,     0, 0x88, 0x0C, 0x2, schema_concentric_2_6, s1x2,        s_short6, warn__none, 0, 1, {0, 1},                     {1, 2, 4, 5, 6, 0}},
   {s_qtag,        s2x4,  0x33, 0x66, 0x0C, 0x0, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {6, 7, 2, 3},               {0, 3, 4, 7}},
   {s1x4,          s2x3,     0,  022, 0x0C, 0x0, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 2, 3, 5}},
   {s2x2,          s2x3,     0,  022, 0x0C, 0x0, schema_concentric,     s2x2,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 2, 3, 5}},

   {s_qtag,        s2x4, 0x33,    0,  0x0D, 0x0, schema_concentric,     s1x4,        s2x4,     warn__none, 0, 0, {6, 7, 2, 3},               {0, 1, 2, 3, 4, 5, 6, 7}},

   // This one is for vi16t.
   {s2x4,          s3x4,    0, 00360, 0x0E, 0x2, schema_concentric,     s2x4,        s2x4,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7},   {0, 1, 2, 3, 6, 7, 8, 9}},

   {s2x4,          s3x4,    0, 01717, 0x0D, 0x1, schema_concentric,     s1x4,        s2x4,     warn__none, 0, 0, {10, 11, 4, 5},             {0, 1, 2, 3, 4, 5, 6, 7}},
   {s2x4,          s3x4, 0x66, 01717, 0x0D, 0x1, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {10, 11, 4, 5},             {0, 3, 4, 7}},
   {s1x4,          s2x4, 0,     0xCC, 0x0C, 0x0, schema_concentric,     s1x4,        s2x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 2, 3, 4, 5, 6, 7}},
   {s1x4,          s2x4, 0,     0x33, 0x0C, 0x0, schema_concentric,     s1x4,        s2x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 2, 3, 4, 5, 6, 7}},
   {s1x8,          s2x4, 0x33,  0x66, 0x0C, 0x0, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {3, 2, 7, 6},               {0, 3, 4, 7}},

   {s1x8,          s2x4, 0x0F,  0xF0, 0xAD, 0x0, schema_matrix,         s_trngl8, nothing,     warn__none, 0, 0, {-1, -1, -1, -1, 0, 1, 3, 2}, {4, 5, 6, 7, -1, -1, -1, -1}},
   {s1x8,          s2x4, 0xF0,  0x0F, 0xAD, 0x0, schema_matrix,         s_trngl8, nothing,     warn__none, 0, 2, {0, 1, 3, 2, -1, -1, -1, -1}, {-1, -1, -1, -1, 4, 5, 6, 7}},
   {s2x4,      s_trngl8, 0x0F,  0x0F, 0x0E, 0x0, schema_matrix,         s2x4,     nothing,     warn__none, 0, 0, {-1, -1, -1, -1, 4, 5, 6, 7}, {-1, -1, -1, -1, 0, 1, 2, 3}},
   {s1x8,      s_trngl8, 0xF0,  0xF0, 0x0D, 0x0, schema_matrix,         s1x8,     nothing,     warn__none, 0, 1, {0, 1, 2, 3, -1, -1, -1, -1}, {4, 5, 7, 6, -1, -1, -1, -1}},

   {s2x4,      s_trngl8, 0xC3,  0x0F, 0x0D, 0x0, schema_matrix,     s_c1phan,     nothing,     warn__none, 0, 0, {-1, -1, 11, 9, 12, 14, -1, -1}, {-1, -1, -1, -1, 0, 2, 7, 5}},

   {s_trngl8,  splinepdmd,   0, 0xF0, 0x0D, 0x0, schema_matrix,      s_trngl8,    nothing,     warn__none, 0, 1, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {s_trngl8,  splinepdmd,   0xF0, 0, 0x0D, 0x0, schema_matrix,    splinepdmd,    nothing,     warn__none, 0, 0, {0, 1, 2, 3, -1, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},

   {s_trngl8,  splinedmd, 0xF0, 0xF0, 0x07, 0x0, schema_matrix,          s1x8,    nothing,     warn__none, 0, 0, {4, 5, 7, 6, -1, -1, -1, -1}, {0, 1, 3, 2, -1, -1, -1, -1}},

   {s_trngl8,  slinepdmd, 0x0F, 0x0F, 0x0B, 0x0, schema_matrix,          s2x4,    nothing,     warn__none, 0, 0, {-1, -1, -1, -1, 4, 5, 6, 7}, {-1, -1, -1, -1, 0, 1, 3, 2}},

   {s_trngl8,  sdmdpdmd,  0xF0, 0xF0, 0x0B, 0x0, schema_matrix,    splinepdmd,    nothing,     warn__none, 0, 1, {0, 1, 2, 3, -1, -1, -1, -1}, {6, 7, 4, 5, -1, -1, -1, -1}},
   {s_trngl8,  sdmdpdmd,  0x0F, 0x0F, 0x0B, 0x0, schema_matrix,      slinedmd,    nothing,     warn__none, 0, 2, {-1, -1, -1, -1, 4, 5, 7, 6}, {-1, -1, -1, -1, 2, 3, 0, 1}},
   {s_trngl8,  sdmdpdmd,  0x0F, 0xF0, 0x0E, 0x0, schema_matrix,     slinepdmd,    nothing,  warn__none, 0, 0, {-1, -1, -1, -1, 4, 5, 7, 6}, {0, 1, 2, 3, -1, -1, -1, -1}},


   {s2x4,      splinedmd, 0xF0, 0xF0, 0x0D, 0x0, schema_matrix,      s_trngl8,    nothing,     warn__none, 0, 1, {4, 5, 6, 7, -1, -1, -1, -1}, {0, 1, 2, 3, -1, -1, -1, -1}},

   {s2x4,      sdmdpdmd, 0x0F, 0x0F, 0x0E, 0x0, schema_matrix,      slinedmd,    nothing,     warn__none, 0, 2, {-1, -1, -1, -1, 4, 5, 7, 6}, {-1, -1, -1, -1, 2, 3, 0, 1}},

   {s_qtag,    s2x4,      0xF0, 0xF0, 0x0D, 0x0, schema_matrix,      s_trngl8,    nothing,     warn__none, 0, 1, {4, 5, 6, 7, -1, -1, -1, -1}, {0, 1, 2, 3, -1, -1, -1, -1}},

   {s_qtag,    s2x4,      0x1E, 0x0F, 0x0D, 0x0, schema_matrix,      slinedmd,    nothing,     warn__none, 0, 2, {0, -1, -1, -1, -1, 2, 3, 1}, {-1, -1, -1, -1, 4, 5, 7, 6}},

   {s_trngl8,  slinedmd,  0x0F, 0,    0x07, 0x0, schema_matrix,      slinedmd,    nothing,     warn__none, 0, 0, {-1, -1, -1, -1, 3, 2, 1, 0}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {s_trngl8,  slinedmd,     0, 0x0F, 0x0E, 0x0, schema_matrix,      s_trngl8,    nothing,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {-1, -1, -1, -1, 4, 5, 7, 6}},

   {s_trngl8,  slinedmd,  0x0F, 0x0F, 0x0B, 0x0, schema_matrix,          s2x4,    nothing,     warn__none, 0, 0, {-1, -1, -1, -1, 4, 5, 6, 7}, {-1, -1, -1, -1, 0, 1, 3, 2}},

   {s2x4,      slinedmd,     0xF0, 0, 0x0E, 0x0, schema_matrix,      slinedmd,    nothing,     warn__none, 0, 0, {4, 5, 7, 6, -1, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {s2x4,      slinedmd,     0, 0x0F, 0x0E, 0x0, schema_matrix,      s2x4,        nothing,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {-1, -1, -1, -1, 0, 1, 3, 2}},
   {s2x4,      slinepdmd,    0xF0, 0, 0x0E, 0x0, schema_matrix,     slinepdmd,    nothing,     warn__none, 0, 0, {4, 5, 7, 6, -1, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},

   {s2x4,      slinepdmd,     0, 0x0F, 0x0E, 0x0, schema_matrix,     s2x4,        nothing,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7}, {-1, -1, -1, -1, 0, 1, 3, 2}},

   {s1x8,      slinepdmd,  0x0F, 0x0F, 0x0D, 0x0, schema_matrix,    s_trngl8,     nothing,     warn__none, 0, 0, {-1, -1, -1, -1, 0, 1, 3, 2}, {-1, -1, -1, -1, 4, 5, 7, 6}},
   {s1x8,      slinedmd,   0x0F, 0x0F, 0x0D, 0x0, schema_matrix,    s_trngl8,     nothing,     warn__none, 0, 0, {-1, -1, -1, -1, 0, 1, 3, 2}, {-1, -1, -1, -1, 4, 5, 7, 6}},

   {s1x8,      sdmdpdmd,   0x0F, 0x0F, 0x0D, 0x0, schema_matrix,   splinedmd,     nothing,     warn__none, 0, 3, {-1, -1, -1, -1, 0, 1, 3, 2}, {-1, -1, -1, -1, 5, 6, 7, 4}},
   {s1x8,      sdmdpdmd,   0xF0, 0xF0, 0x0D, 0x0, schema_matrix,  splinepdmd,     nothing,     warn__none, 0, 1, {0, 1, 3, 2, -1, -1, -1, -1}, {6, 7, 4, 5, -1, -1, -1, -1}},

   {s_trngl8,  slinepdmd,    0x0F, 0, 0x0E, 0x0, schema_matrix,    slinepdmd,     nothing,     warn__none, 0, 0, {-1, -1, -1, -1, 4, 5, 7, 6}, {0, 1, 2, 3, 4, 5, 6, 7}},
   {s_trngl8,  splinedmd,    0, 0xF0, 0x0D, 0x0, schema_matrix,     s_trngl8,     nothing,     warn__none, 0, 1, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 2, 3, -1, -1, -1, -1}},
   {s_trngl8,  splinedmd,    0xF0, 0, 0x0D, 0x0, schema_matrix,    splinedmd,     nothing,     warn__none, 0, 0, {0, 1, 2, 3, -1, -1, -1, -1}, {0, 1, 2, 3, 4, 5, 6, 7}},

   // When all else fails, go to a sx1x8 (gigantic thar).
   {s1x8,          s1x8,        0, 0, 0x0D, 0x0, schema_matrix,        sx1x8,     nothing,     warn__none, 0, 1, {0, 1, 3, 2, 8, 9, 11, 10},      {12, 13, 15, 14, 4, 5, 7, 6}},
   // Go to a thar.
   {s1x8,          s1x8, 0x33,  0x33, 0x0D, 0x0, schema_matrix,       s_thar,     nothing,     warn__none, 0, 1, {-1, -1, 1, 0, -1, -1, 5, 4},               {-1, -1, 7, 6, -1, -1, 3, 2}},

   // Need both of these because they won't canonicalize.
   {s1x8,          s1x8, 0xCC,  0x33, 0x0D, 0x1, schema_concentric,     s1x4,        s1x4,     warn__none, 0, 0, {3, 2, 7, 6},               {0, 1, 4, 5}},
   {s1x8,          s1x8, 0x33,  0xCC, 0x0D, 0x0, schema_concentric,     s1x4,        s1x4,     warn__none, 0, 0, {3, 2, 7, 6},               {0, 1, 4, 5}},

   // Even newer!  This one works for "do your part".
   // This allows [diamonds] point DYP spin the top while ctrs dmd circ.
   {s1x8,          s2x4, 0x55,  0x66, 0x0D, 0x0, schema_matrix,       s_qtag,     nothing,     warn__none, 0, 1, {-1, 6, -1, 7, -1, 2, -1, 3},{5, -1, -1, 0, 1, -1, -1, 4}},

   // These two need to be in this order for now.  Cf ng33t.
   // Note that this map now won't be triggered for "do your part".
   {s1x8,          s2x4, 0x11,  0x66, 0x10D,0x0, schema_concentric,     s1x6,        s2x6,     warn__none, 0, 1, {1, 3, 2, 5, 7, 6},{-1, -1, 3, 4, -1, -1, -1, -1, 7, 0, -1, -1}},
   {s1x8,          s2x4, 0x33,  0x66, 0x0C, 0x0, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {3, 2, 7, 6},               {0, 3, 4, 7}},

   // Next 2 must be in this order.  Actually, can we just use one?
   {s2x3,          s2x4, 022,      0, 0x0D, 0x0, schema_matrix,         s4x4,     nothing,     warn__none, 0, 0, {14, -1, 5, 6, -1, 13}, {10, 15, 3, 1, 2, 7, 11, 9}},
   {s2x3,          s2x4, 022,   0x99, 0x2E, 0x1, schema_concentric,     s2x2,        s2x2,     warn__none, 0, 0, {1, 2, 5, 6},               {0, 2, 3, 5}},

   {s2x2,       s_bone6, 0,      044, 0x0C, 0x0, schema_concentric,     s2x2,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 3, 4}},
   {s1x4,       s_bone6, 0,      044, 0x0E, 0x0, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 3, 4}},
   {s1x4,       s_bone6, 0xA,      0, 0x0D, 0x0,     schema_matrix,     s_hrglass,   nothing,  warn__none, 0, 1, {6, -1, 2, -1},             {5, 0, 3, 1, 4, 7}},
   {s1x4,       s_bone6, 0xA,      0, 0x0E, 0x0,     schema_matrix,        s_ptpd,   nothing,  warn__none, 0, 0, {0, -1, 4, -1},             {1, 7, 6, 5, 3, 2}},

   {s1x4,          s2x4, 0,     0x66, 0x0C, 0x0, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 3, 4, 7}},
   {s2x4,        s_ptpd, 0x66,  0x55, 0xAD, 0x1, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {-1, 6, -1, 5, -1, 2, -1, 1}, {0}},

   // These two must be in the order shown.
   {s1x6,        s_ptpd, 0,     0xAA, 0x0E, 0x1, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {0, -1, 2, -1, 3, -1, 5, -1}, {0}},
   {s1x8,        s_ptpd, 0,     0xAA, 0x2E, 0x1, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {0, -1, 2, -1, 4, -1, 6, -1}, {0}},

   {s_bone6,       s2x4, 0,     0x66, 0x0E, 0x1, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {0, -1, -1, 1, 3, -1, -1, 4}, {0}},
   {s_bone,        s2x4, 0,     0x66, 0x0E, 0x1, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {0, -1, -1, 1, 4, -1, -1, 5}, {0}},
   {s_qtag,        s2x4, 0,     0x66, 0x0D, 0x1, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {5, -1, -1, 0, 1, -1, -1, 4}, {0}},
   {s_qtag,        s2x3, 0,      022, 0x0D, 0x1, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {5, -1, 0, 1, -1, 4},       {0}},
   {s1x4,          s2x3, 0xA,      0, 0x0C, 0x1, schema_concentric,     s2x3,        s1x2,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5},         {0, 2}},
   {s1x6,          s2x3, 0,        0, 0x0D, 0x0, schema_matrix,         s3x6,        nothing,  warn__none, 0, 1, {15, 16, 17, 6, 7, 8}, {12, 17, 2, 3, 8, 11}},
   {s1x6,          s2x3, 044,      0, 0x0E, 0x0, schema_matrix,         s_spindle12, nothing,  warn__none, 0, 0, {8, 9, -1, 3, 4, -1}, {0, 1, 2, 5, 6, 7}},
   {s2x3,   s_spindle12, 0,     0xE7, 0x0D, 0x0, schema_matrix,         s3x6,        nothing,  warn__none, 0, 0, {3, 8, 11, 12, 17, 2}, {-1, -1, -1, 6, 7, -1, -1, -1, 15, 16}},
   {s1x6,          s2x6, 044,  02222, 0x0E, 0x0, schema_matrix,         sdblbone6,   nothing,  warn__none, 0, 0, {5, 2, -1, 11, 8, -1}, {0, -1, 1, 9, -1, 10, 6, -1, 7, 3, -1, 4}},
   {s1x6,          s1x6, 0,      066, 0x2D, 0x0, schema_concentric,     s1x6,        s1x2,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5},         {0, 3}},
   {s1x6,          s1x6, 066,      0, 0x2D, 0x1, schema_concentric,     s1x6,        s1x2,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5},         {0, 3}},
   {s1x4,          s1x6, 0xA,      0, 0x0D, 0x1, schema_concentric,     s1x6,        s1x2,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5},         {0, 2}},
   {s2x4,          s2x8, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {2, 3, 4, 5, 10, 11, 12, 13},{0}},
   {s2x4,          s4x6, 0,        0, 0x0E, 0x0, schema_nothing,        s4x6,        nothing,  warn__none, 0, 0, {10, 9, 8, 7, 22, 21, 20, 19}, {0}},
   {s2x4,          s4x6, 0,        0, 0x0D, 0x0, schema_nothing,        s4x6,        nothing,  warn__none, 0, 0, {3, 8, 21, 14, 15, 20, 9, 2}, {0}},
   {s2x6,          s4x6, 0,        0, 0x0E, 0x0, schema_nothing,        s4x6,        nothing,  warn__none, 0, 0, {11, 10, 9, 8, 7, 6, 23, 22, 21, 20, 19, 18}, {0}},
   {s2x6,          s4x4, 0,        0, 0x0D, 0x0, schema_matrix,         s4x6,        nothing,  warn__none, 0, 1, {11, 10, 9, 8, 7, 6, 23, 22, 21, 20, 19, 18}, {1, 2, 3, 9, 4, 7, 22, 8, 13, 14, 15, 21, 16, 19, 10, 20}},
   {s2x6,          s4x4, 0,        0, 0x0E, 0x0, schema_matrix,         s4x6,        nothing,  warn__none, 0, 0, {11, 10, 9, 8, 7, 6, 23, 22, 21, 20, 19, 18}, {4, 7, 22, 8, 13, 14, 15, 21, 16, 19, 10, 20, 1, 2, 3, 9}},

   {s3x4,         sd2x7, 07474,0x31E3,0xAD, 0x1, schema_concentric,     s2x3,        s2x3,     warn__none, 1, 0, {11, 10, 9, 4, 3, 2},         {0, 1, -1, 6, 7, -1}},
   {s3x4,         sd2x7, 06363,0x31E3,0xAD, 0x1, schema_concentric,     s2x3,        s2x3,     warn__none, 1, 0, {11, 10, 9, 4, 3, 2},         {-1, 2, 3, -1, 8, 9}},

   // These six must be in this order.
   {s2x4,          s2x6, 0,        0, 0x0D, 0x0, schema_matrix,         s4x6,        nothing,  warn__none, 0, 0, {3, 8, 21, 14, 15, 20, 9, 2}, {11, 10, 9, 8, 7, 6, 23, 22, 21, 20, 19, 18}},


   // This one is troublesome!

   // This is what it used to be:

   {s2x4,          s2x6, 0,    0x30C,0x5AE, 0x0, schema_matrix,         s2x8,        nothing,  warn__none, 0, 0, {2, 3, 4, 5, 10, 11, 12, 13}, {0, 1, -1, -1, 6, 7, 8, 9, -1, -1, 14, 15}},


   // Want this one NOT TO BE USED in the girls hinge case, but OK in the girls pass thru case.
   {s2x4,          s2x6, 0,        0, 0x0E, 0x10,schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 2, 3, 4, 7, 8, 9, 10},  {0}},


   {s2x4,          s2x6, 0x99,     0, 0x0D, 0x0, schema_nothing,        nothing,     nothing, warn__none, 0, 0, {-1, 3, 8, -1, -1, 9, 2, -1}, {0}},
   {s2x4,          s2x6, 0x33, 06666, 0x2D, 0x0, schema_matrix,         spgdmdccw,   nothing, warn__none, 0, 1, {-1, -1, 3, 2, -1, -1, 7, 6}, {5, -1, -1, 0, -1, -1, 1, -1, -1, 4, -1, -1}},
   {s2x4,          s2x6, 0xCC, 03333, 0x2D, 0x0, schema_matrix,         spgdmdcw,    nothing, warn__none, 0, 1, {6, 7, -1, -1, 2, 3, -1, -1},{-1, -1, 5, -1, -1, 0, -1, -1, 1, -1, -1, 4}},

   {s2x3,          s2x6, 0,    0x30C, 0x0E, 0x0, schema_matrix,         s2x7,        nothing, warn__none, 0, 0, {2, 3, 4, 9, 10, 11},{0, 1, -1, -1, 5, 6, 7, 8, -1, -1, 12, 13}},
   {s2x3,          s2x7, 0,   0x0E1C, 0x0D, 0x0, schema_matrix,        sd2x7,        nothing, warn__none, 0, 0, {11, 10, 9, 4, 3, 2},{0, 1, -1, -1, -1, 5, 6, 7, 8, -1, -1, -1, 12, 13}},

   {sdmd,          s2x8, 0,   0x3C3C, 0x0E, 0x0, schema_matrix,         sbigdhrgl,   nothing,  warn__none, 0, 0, {9, 2, 3, 8},               {0, 1, -1, -1, -1, -1, 4, 5, 6, 7, -1, -1, -1, -1, 10, 11}},
   {sdmd,          s2x8, 0,   0x3C3C, 0x0D, 0x0, schema_matrix,         sbighrgl,    nothing,  warn__none, 0, 0, {2, 3, 8, 9},               {0, 1, -1, -1, -1, -1, 4, 5, 6, 7, -1, -1, -1, -1, 10, 11}},

   {sdmd,          s2x6, 0,    0x30C, 0x0E, 0x0, schema_matrix,         sbigdhrgl,   nothing,  warn__none, 0, 0, {9, 2, 3, 8},               {0, 1, -1, -1, 4, 5, 6, 7, -1, -1, 10, 11}},
   {sdmd,          s2x6, 0,    0x30C, 0x0D, 0x0, schema_matrix,         sbighrgl,    nothing,  warn__none, 0, 0, {2, 3, 8, 9},               {0, 1, -1, -1, 4, 5, 6, 7, -1, -1, 10, 11}},

   // These two must be in this order.
   {s1x4,          s2x6, 0,    0x30C, 0x2E, 0x0, schema_matrix,         sbigbone,    nothing,  warn__none, 0, 0, {2, 3, 8, 9}, {0, 1, -1, -1, 4, 5, 6, 7, -1, -1, 10, 11}},
   {s1x4,          s2x6, 0,    0x79E, 0x0E, 0x0, schema_matrix,         s_bone,      nothing,  warn__none, 0, 0, {6, 7, 2, 3}, {0, -1, -1, -1, -1, 1, 4, -1, -1, -1, -1, 5}},

   {s_spindle,     s2x5, 0x77,     0, 0x0D, 0x0, schema_matrix,         s4x5,        nothing,  warn__none, 0, 0, {-1, -1, -1, 12, -1, -1, -1, 2},               {9, 8, 7, 6, 5, 19, 18, 17, 16, 15}},

   {s1x4,          s2x8, 0,   0x3C3C, 0x0D, 0x0, schema_matrix,         sbigdmd,     nothing,  warn__none, 0, 0, {2, 3, 8, 9},               {0, 1, -1, -1, -1, -1, 4, 5, 6, 7, -1, -1, -1, -1, 10, 11}},

   {s1x4,          s2x6, 0,    0x30C, 0x0D, 0x0, schema_matrix,         sbigdmd,     nothing,  warn__none, 0, 0, {2, 3, 8, 9},               {0, 1, -1, -1, 4, 5, 6, 7, -1, -1, 10, 11}},
   {s_qtag,        s2x6, 0x33, 0x30C, 0x0D, 0x0, schema_matrix,         sbigdmd,     nothing,  warn__none, 0, 0, {-1, -1, 8, 9, -1, -1, 2, 3}, {0, 1, -1, -1, 4, 5, 6, 7, -1, -1, 10, 11}},
   {s2x2,         s4dmd, 0,   0xF0F0, 0x0E, 0x0, schema_matrix,         s4x4,        nothing,  warn__none, 0, 0, {15, 3, 7, 11},             {12, 13, 14, 0, -1, -1, -1, -1, 4, 5, 6, 8, -1, -1, -1, -1}},
   {s_crosswave,s_crosswave, 0x99,0x66,0x0D,0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {-1, 2, 5, -1, -1, 6, 1, -1}, {0}},
   {s_crosswave,s_crosswave, 0x66,0x99,0x0D,0x1, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {-1, 6, 1, -1, -1, 2, 5, -1}, {0}},
   {s_crosswave,s_crosswave, 0x88,0x11,0x0D,0x0, schema_matrix,         s3x1dmd,     nothing,  warn__none, 0, 1, {0, 1, 3, -1, 4, 5, 7, -1}, {-1, 7, 1, 2, -1, 3, 5, 6}},
   {s1x2,   s_crosswave, 0,        0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {3, 7},                     {0}},
   {s1x8,   s_crosswave, 0xCC,  0x55, 0x2D, 0x1, schema_concentric,     sdmd,        s1x4,     warn__none, 0, 0, {1, 3, 5, 7},{0, 1, 4, 5}},
   {s1x4,      s_rigger, 0xA,      0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {7, -1, 3, -1},{0}},

   // These two must be in the order shown.
   {s1x6,      s_rigger, 044,      0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {6, 7, -1, 2, 3, -1},{0}},
   {s1x6,      s_rigger, 066,      0, 0x2E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {7, -1, -1, 3, -1, -1},{0}},

   {s1x8,      s_rigger, 0xCC,     0, 0x2E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {6, 7, -1, -1, 2, 3, -1, -1},{0}},
   {s1x8,     s_spindle, 0xAA,  0xAA, 0x0E, 0x0, schema_rev_checkpoint, s1x4,        s2x2,     warn__none, 0, 0, {0, 2, 4, 6},               {0, 2, 4, 6}},
   {s1x8,       s3x1dmd, 0xAA,  0x66, 0x1E, 0x0, schema_matrix,         s3x1dmd,     nothing,  warn__none, 0, 0, {0, -1, 2, -1, 4, -1, 6, -1},{1, -1, -1, 3, 5, -1, -1, 7}},
   {s1x6,       s1x3dmd, 044,   0x66, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 2, -1, 5, 6, -1},{0}},
   {s1x8,       s1x3dmd, 0x55,  0x66, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {-1, 1, -1, 2, -1, 5, -1, 6},{0}},
   {s1x3dmd,    s2x4,    0x66,  0x66, 0X1BE,0x0, schema_matrix,       s_spindle,     nothing,  warn__none, 0, 0, {7, -1, -1, 1, 3, -1, -1, 5},{0, -1, -1, 2, 4, -1, -1, 6}},
   {s1x3dmd,  s_spindle, 0x66,  0xAA, 0x0E, 0x0, schema_rev_checkpoint,    sdmd,     s2x2,     warn__none, 0, 0, {0, 3, 4, 7},               {0, 2, 4, 6}},
   {s1x3dmd,     s_ptpd, 0x66,  0x55, 0x0E, 0x0, schema_rev_checkpoint,    sdmd,     s2x2,     warn__none, 0, 0, {0, 3, 4, 7},               {1, 7, 5, 3}},
   {s_1x2dmd, s_2x1dmd, 022,     022, 0x1D, 0x0, schema_matrix,      s_crosswave,    nothing,  warn__none, 0, 1, {0, -1, 3, 4, -1, 7}, {6, -1, 1, 2, -1, 5}},
   {s_1x2dmd, s1x8,    022,     0x66, 0x1E, 0x0, schema_matrix,         s1x3dmd,     nothing,  warn__none, 0, 0, {1, -1, 3, 5, -1, 7}, {0, -1, -1, 2, 4, -1, -1, 6}},
   {s_1x2dmd, s1x8,    044,     0xAA, 0x1E, 0x0, schema_matrix,         s1x8,        nothing,  warn__none, 0, 0, {1, 3, -1, 5, 7, -1}, {0, -1, 2, -1, 4, -1, 6, -1}},
   {s_1x2dmd, s_spindle, 0,     0x55, 0x2E, 0x0, schema_matrix,         s1x3dmd,     nothing,  warn__none, 0, 0, {1, 2, 3, 5, 6, 7}, {-1, 3, -1, 4, -1, 7, -1, 0}},
   {s_1x2dmd,   s1x3dmd, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 2, 3, 5, 6, 7},         {0}},

   // These two must be in the order shown.
   {s1x6,        s_ptpd, 022,      0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {0, -1, 2, 4, -1, 6}, {0}},
   {s1x8,        s_ptpd, 0xAA,     0, 0x2E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {0, -1, 2, -1, 4, -1, 6, -1}, {0}},

   {s_bone6,       s1x8, 0,     0xAA, 0x0E, 0x0, schema_matrix,         s_ptpd,      nothing,  warn__none, 0, 0, {1, 7, 6, 5, 3, 2},         {0, -1, 2, -1, 4, -1, 6, -1}},

   // Special one for merging a perpendicular 2x4 that was cut down to a 2x2.
   // Is this one used???
   // I hope not.  It allows "own the ends zip the top by regroup" from (2FL; ctrs ctr rot).
   // I don't think we should allow that, because people from the zip the top are
   // intervening between the people who traded on the regroup.  So take it out.
   /*
   {s2x2,          s1x8, 0,     0xAA, 0x0E, 0x1, schema_rev_checkpoint, s1x4,        s2x2,     warn__none, 0, 0, {0, 2, 4, 6},               {0, 1, 2, 3}},
   */

   {s1x8,          s2x4, 0xAA,  0x99, 0x4D, 0x0, schema_rev_checkpoint, s1x4,        s2x2,     warn__none, 0, 0, {0, 2, 4, 6},               {1, 2, 5, 6}},
   {s2x2,          s1x8, 0,     0xAA, 0x4E, 0x1, schema_rev_checkpoint, s1x4,        s2x2,     warn__none, 0, 0, {0, 2, 4, 6},               {0, 1, 2, 3}},
   {s2x2,          s1x8, 0,     0x66, 0x1E, 0x0, schema_concentric,     s2x2,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 3, 4, 7}},

   // These two must be in this order.
   {s2x2,          s1x6, 0,      044, 0x0E, 0x0, schema_concentric,     s2x2,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 3, 4}},
   {s2x2,          s1x6, 0,      066, 0x0E, 0x0, schema_concentric,     s2x2,        s1x2,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 3}},

   {s2x2,          s1x8, 0,     0x55, 0x0E, 0x0, schema_concentric,     s2x2,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3},               {1, 3, 5, 7}},
   {s1x8,          s2x4, 0xCC,  0x99, 0x0E, 0x1, schema_concentric,     s2x2,        s1x4,     warn__none, 0, 0, {1, 2, 5, 6},               {0, 1, 4, 5}},
   {s1x8,          s2x4, 0xCC,  0x99, 0x0D, 0x1, schema_concentric,     s2x2,        s1x4,     warn__none, 0, 0, {1, 2, 5, 6}, {0, 1, 4, 5}},
   {s2x2,          s1x8, 0,     0xCC, 0x0E, 0x0, schema_concentric,     s2x2,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 4, 5}},
   {s2x2,          s1x4, 0x9,    0xC, 0x2E, 0x0, schema_matrix,         s_trngl4,    nothing,  warn__none, 0, 1, {-1, 2, 3, -1},               {0, 1, -1, -1}},
   {s2x2,          s1x4, 0x6,    0x3, 0x2E, 0x0, schema_matrix,         s_trngl4,    nothing,  warn__none, 0, 3, {3, -1, -1, 2},               {-1, -1, 0, 1}},
   {s2x2,       s3x1dmd, 0,     0x66, 0x0E, 0x0, schema_concentric,     s2x2,        sdmd,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 3, 4, 7}},
   {s2x2,      s_galaxy, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 3, 5, 7},               {0}},
   {s2x2,        s_ptpd, 0,     0x55, 0x0E, 0x0, schema_concentric,     s2x2,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},               {1, 7, 5, 3}},
   {sbigh,         s4x6, 0,036363636, 0x0E, 0x0, schema_matrix,         sbigh,       nothing,  warn__none, 0, 0, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},{0, -1, -1, -1, -1, 9, 8, -1, -1, -1, -1, 1, 6, -1, -1, -1, -1, 3, 2, -1, -1, -1, -1, 7}},
   {s2x6,         sbigh, 03636,    0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, -1, -1, -1, -1, 8, 7, -1, -1, -1, -1, 2},{0}},
   {s2x6,      sbigbone, 03636,    0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, -1, -1, -1, -1, 4, 7, -1, -1, -1, -1, 10},{0}},
   {s_bone,        s2x8, 0,   0x3C3C, 0x0E, 0x0, schema_matrix,         sbigbone,    nothing,  warn__none, 0, 0, {1, 4, 8, 9, 7, 10, 2, 3},{0, 1, -1, -1, -1, -1, 4, 5, 6, 7, -1, -1, -1, -1, 10, 11}},
   {s_bone,        s4x6, 0,036363636, 0x0E, 0x0, schema_matrix,         sbigh,       nothing,  warn__none, 0, 0, {1, 8, 10, 11, 7, 2, 4, 5},{0, -1, -1, -1, -1, 9, 8, -1, -1, -1, -1, 1, 6, -1, -1, -1, -1, 3, 2, -1, -1, -1, -1, 7}},
   {s_bone,    sbigbone, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 4, 8, 9, 7, 10, 2, 3},{0}},
   {s_bone,        s1x8, 0x33,     0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {-1, -1, 7, 6, -1, -1, 3, 2},{0}},
   {s_qtag,        s3x4, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 2, 4, 5, 7, 8, 10, 11}, {0}},
   {s_qtag,   s_hrglass, 0x88,  0xBB, 0x0D, 0x0, schema_concentric,     s_short6,    s1x2,     warn__check_galaxy, 1, 0, {1, 2, 4, 5, 6, 0}, {6, 2}},
   {s1x2,   s_spindle,   0,     0x22, 0x0C, 0x0, schema_concentric_2_6, s1x2,        s_short6, warn__none, 0, 1, {0, 1}, {2, 3, 4, 6, 7, 0}},
   {s1x8,   s_spindle,   0x44,  0xDD, 0x0E, 0x1, schema_concentric,     s1x2,        s1x6,     warn__none, 1, 0, {1, 5}, {0, 1, 3, 4, 5, 7}},
   {sdmd,   s_crosswave, 0,     0xCC, 0x0D, 0x0, schema_concentric,     sdmd,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3}, {0, 1, 4, 5}},
   {s2x2,   s_crosswave, 0,     0xAA, 0x0E, 0x0, schema_concentric,     s2x2,        sdmd,     warn__none, 0, 0, {0, 1, 2, 3}, {0, 2, 4, 6}},
   {s_crosswave,   s2x4, 0x55,  0x66, 0x0D, 0x0, schema_concentric,     sdmd,        s2x2,     warn__none, 0, 0, {1, 3, 5, 7}, {0, 3, 4, 7}},
   {s1x4,        s_bone, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {6, 7, 2, 3},               {0}},
   {s2x3,         s3dmd, 0,    07070, 0x0E, 0x0, schema_concentric,     s2x3,        s2x3,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5},         {0, 1, 2, 6, 7, 8}},
   {sdmd,          s4x4, 0,   0x8E8E, 0x0E, 0x0, schema_matrix,         s_hsqtag,    nothing,  warn__none, 0, 1, {10, 11, 4, 5}, {0, -1, -1, -1, 9, 8, 7, -1, 6, -1, -1, -1, 3, 2, 1, -1}},
   {sdmd,          s3x4, 0,    04040, 0x0E, 0x0, schema_matrix,         s_hsqtag,    nothing,  warn__none, 0, 1, {10, 11, 4, 5}, {3, 2, 1, 0, 4, -1, 9, 8, 7, 6, 10, -1}},

   {s1x4,          s3x4, 0,        0, 0x0E, 0x10, schema_matrix,        s3x6,        nothing,  warn__none, 0, 0, {16, 17, 7, 8},             {0, -1, -1, 5, 6, -1, 9, -1, -1, 14, 15, -1}},
   {s1x4,          s3x4, 0,        0, 0x0E, 0x10, schema_nothing,       nothing,     nothing,  warn__none, 0, 0, {10, 11, 4, 5},             {0}},

   {s1x4,          s3x4, 0,    04646, 0x0D, 0x0, schema_matrix,         s_343,       nothing,  warn__none, 0, 1, {8, 9, 3, 4},             {7, -1, -1, 0, 1, -1, 2, -1, -1, 5, 6, -1}},


   {s1x4,         sd3x4, 0,    01616, 0x0E, 0x0, schema_matrix,         s3x6,        nothing,  warn__none, 0, 0, {16, 17, 7, 8},             {0, -1, -1, -1, 5, 6, 9, -1, -1, -1, 14, 15}},



   {sdmd,       s_hsqtag, 0,       0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 1, 0, {4, 5, 10, 11},             {0}},
   {s1x4,       s_hsqtag, 0,   04040, 0x0D, 0x0, schema_matrix,         s3x4,        nothing,  warn__none, 0, 1, {10, 11, 4, 5}, {9, 8, 7, 6, 10, -1, 3, 2, 1, 0, 4, -1}},
   {s1x4,       s_dmdlndmd, 0,     0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 1, 0, {1, 2, 7, 8},             {0}},
   {sdmd,       s_dmdlndmd, 0, 00606, 0x0D, 0x0, schema_matrix,         s3dmd,       nothing,  warn__none, 0, 0, {1, 5, 7, 11}, {0, 1, -1, 2, 3, 4, 6, 7, -1, 8, 9, 10}},
   {s1x2,          s3x4, 0,    04040, 0x0D, 0x0, schema_matrix,         s_hsqtag,    nothing,  warn__none, 0, 1, {11, 5}, {3, 2, 1, 0, 4, -1, 9, 8, 7, 6, 10, -1}},
   {s1x2,       s_hsqtag, 0,   04040, 0x0D, 0x0, schema_matrix,         s3x4,        nothing,  warn__none, 0, 1, {11, 5}, {9, 8, 7, 6, 10, -1, 3, 2, 1, 0, 4, -1}},
   {s2x2,       s_hsqtag, 0,   06060, 0x0E, 0x0, schema_matrix,         s4x4,        nothing,  warn__none, 0, 0, {15, 3, 7, 11},                {12, 10, 9, 8, -1, -1, 4, 2, 1, 0, -1, -1}},
   {s1x4,         s4dmd, 0,   0xF6F6, 0x2D, 0x0, schema_concentric,     s1x4,        s2x4,     warn__check_3x4, 0, 1, {0, 1, 2, 3},{3, -1, -1, 8, 11, -1, -1, 0}},
   {s1x4,         s4dmd, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {14, 15, 6, 7},{0}},
   {s1x6,         s4dmd, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {13, 14, 15, 5, 6, 7},{0}},
   {s1x8,         s4dmd, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {12, 13, 15, 14, 4, 5, 7, 6},{0}},
   {s_qtag,       s4dmd, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 2, 6, 7, 9, 10, 14, 15},{0}},
   {s2x3,         s4dmd, 0,        0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {2, 7, 9, 10, 15, 1},{0}},
   {s1x6,         s3dmd, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {9, 10, 11, 3, 4, 5},       {0}},
   {s1x4,         s3dmd, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {10, 11, 4, 5},             {0}},

   {s1x2,         s3dmd, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {11, 5},                    {0}},
   {s1x2,         s3dmd, 0,    04242, 0x0D, 0x0, schema_matrix,         s_dmdlndmd,  nothing,  warn__none, 0, 0, {2, 8}, {0, -1, 3, 4, 5, -1, 6, -1, 9, 10, 11, -1}},
   {s1x2,         s_dmdlndmd, 0,   0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 1, 0, {2, 8},                     {0}},
   {s1x2,         s_dmdlndmd, 0, 00404, 0x0E, 0x0, schema_matrix,       s3dmd,       nothing,  warn__none, 0, 0, {11, 5}, {0, 1, -1, 2, 3, 4, 6, 7, -1, 8, 9, 10}},

   {s1x4,         s3dmd, 0,    07272, 0x0D, 0x0, schema_matrix,         s3dmd,       nothing,  warn__none, 0, 1, {10, 11, 4, 5},{8, -1, 0, -1, -1, -1, 2, -1, 6, -1, -1, -1}},
   {s1x4,         sbigh, 0,    06060, 0x0D, 0x0, schema_matrix,         s3x4,        nothing,  warn__none, 0, 1, {10, 11, 4, 5},{9, 8, 7, 6, -1, -1, 3, 2, 1, 0, -1, -1}},
   {s1x4,          s3x4, 0,    06060, 0x0D, 0x0, schema_matrix,         sbigh,       nothing,  warn__none, 0, 1, {4, 5, 10, 11},{3, 2, 1, 0, -1, -1, 9, 8, 7, 6, -1, -1}},
   {s2x2,         sbigh, 0,    06060, 0x0E, 0x0, schema_matrix,         s4x4,        nothing,  warn__none, 0, 0, {15, 3, 7, 11},{12, 10, 9, 8, -1, -1, 4, 2, 1, 0, -1, -1}},

   {s2x4,          spgdmdcw, 0, 0xCC, 0x2D, 0x0, schema_matrix,         s2x6,        nothing,  warn__none, 0, 1, {1, 2, 3, 4, 7, 8, 9, 10}, {11, 2, -1, -1, 5, 8, -1, -1}},
   {s2x4,         spgdmdccw, 0, 0xCC, 0x2D, 0x0, schema_matrix,         s2x6,        nothing,  warn__none, 0, 1, {1, 2, 3, 4, 7, 8, 9, 10}, {9, 0, -1, -1, 3, 6, -1, -1}},

   {s1x4,          s4x4, 0,   0x8E8E, 0x0E, 0x0, schema_matrix,         s3x4,        nothing,  warn__none, 0, 0, {10, 11, 4, 5},{3, -1, -1, -1, 6, 7, 8, -1, 9, -1, -1, -1, 0, 1, 2, -1}},
   {s1x4,          s4x4, 0,   0xE8E8, 0x0D, 0x0, schema_matrix,         s3x4,        nothing,  warn__none, 0, 1, {10, 11, 4, 5},{0, 1, 2, -1, 3, -1, -1, -1, 6, 7, 8, -1, 9, -1, -1, -1}},
   {s1x4,          s4x4, 0,   0xE8E8, 0x0E, 0x0, schema_matrix,         sbigh,       nothing,  warn__none, 0, 0, {4, 5, 10, 11},{9, 8, 7, -1, 6, -1, -1, -1, 3, 2, 1, -1, 0, -1, -1, -1}},
   {s1x4,          s4x4, 0,   0x8E8E, 0x0D, 0x0, schema_matrix,         sbigh,       nothing,  warn__none, 0, 1, {4, 5, 10, 11},{0, -1, -1, -1, 9, 8, 7, -1, 6, -1, -1, -1, 3, 2, 1, -1}},

   // This one must be after the four that precede.
   {s1x4,          s4x4, 0,   0xEEEE, 0x0C, 0x8, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 1, {0, 1, 2, 3},{0, 4, 8, 12}},

   {s2x3,          s4x5, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {8, 7, 6, 18, 17, 16},{0}},

   {s1x4,          s4x5, 0,  0xF83E0, 0xAE, 0x0, schema_matrix,         s_545,       nothing,  warn__none, 0, 0, {12, 13, 5, 6},               {0, 1, 2, 3, 4, -1, -1, -1, -1, -1, 7, 8, 9, 10, 11, -1, -1, -1, -1, -1}},

   {s1x4,          s4x5, 0,  0x739CE, 0xAD, 0x0, schema_matrix,         s3x4,        nothing,  warn__none, 0, 1, {10, 11, 4, 5},               {9, -1, -1, -1, 0, 1, -1, -1, -1, 8, 3, -1, -1, -1, 6, 7, -1, -1, -1, 2}},
   {s1x4,          s4x5, 0,  0x739CE, 0xAE, 0x0, schema_matrix,         sbigh,       nothing,  warn__none, 0, 0, {4, 5, 10, 11},               {0, -1, -1, -1, 9, 8, -1, -1, -1, 1, 6, -1, -1, -1, 3, 2, -1, -1, -1, 7}},

   {s1x4,          s3x6, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {16, 17, 7, 8},{0}},
   {s_short6,   s1x8,    0,     0xEE, 0x0E, 0x0, schema_matrix,         s_galaxy,    nothing,  warn__none, 0, 0, {1, 2, 3, 5, 6, 7},       {0, -1, -1, -1, 4, -1, -1, -1}},
   {s_short6,   s1x6,    0,      066, 0x0E, 0x0, schema_matrix,         s_galaxy,    nothing,  warn__none, 0, 0, {1, 2, 3, 5, 6, 7},       {0, -1, -1, 4, -1, -1}},
   {s1x6,   s_spindle,   0,     0x77, 0x0E, 0x0, schema_concentric,     s1x6,        s1x2,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5},       {7, 3}},
   {s1x6,   s_spindle,   044,   0x55, 0x0E, 0x0, schema_matrix,         s1x3dmd,     nothing,  warn__none, 0, 0, {1, 2, -1, 5, 6, -1},       {-1, 3, -1, 4, -1, 7, -1, 0}},
   {s1x4,      s_2x1dmd, 0,      066, 0x0D, 0x0, schema_matrix,         s_2x1dmd,    nothing,  warn__none, 0, 1, {0, 1, 3, 4},       {5, -1, -1, 2, -1, -1}},
   {s1x4,      s_2x1dmd, 0xA,      0, 0x0D, 0x0, schema_matrix,         s_crosswave, nothing,  warn__none, 0, 1, {0, -1, 4, -1},     {6, 7, 1, 2, 3, 5}},
   {s1x4,          sdmd, 0xA,      0, 0x0D, 0x0, schema_matrix,         s_2x1dmd,    nothing,  warn__none, 0, 1, {0, -1, 3, -1},     {5, 1, 2, 4}},
   {sdmd,      s_2x1dmd, 0,        0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {2, 4, 5, 1},       {0}},
   {s1x6,      s_2x1dmd, 044,      0, 0x0D, 0x0, schema_matrix,         s_crosswave, nothing,  warn__none, 0, 1, {0, 1, -1, 4, 5, -1},       {6, 7, 1, 2, 3, 5}},
   {s1x6,      s_2x1dmd, 0,        0, 0x0E, 0x0, schema_matrix,         s3x1dmd,     nothing,  warn__none, 0, 0, {0, 1, 2, 4, 5, 6},       {1, 2, 3, 5, 6, 7}},
   {s1x6, s_wingedstar6, 0,        0, 0x0E, 0x0, schema_matrix,         s_wingedstar,nothing,  warn__none, 0, 0, {0, 1, 2, 4, 5, 6},       {1, 2, 3, 5, 6, 7}},
   {s1x4,      s_1x2dmd, 0,      044, 0x0D, 0x0, schema_matrix,         s_crosswave, nothing,  warn__none, 0, 0, {2, 3, 6, 7},               {0, 1, -1, 4, 5, -1}},
   {s1x4,      s_1x2dmd, 0,      044, 0x0E, 0x0, schema_matrix,         s1x8,        nothing,  warn__none, 0, 0, {3, 2, 7, 6},               {0, 1, -1, 4, 5, -1}},
   {s1x6,       s3x1dmd, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {0, 1, 2, 4, 5, 6},         {0}},
   {s1x6,  s_wingedstar, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {0, 1, 2, 4, 5, 6},         {0}},
   {s1x4,       s3x1dmd, 0,     0x66, 0x2D, 0x1, schema_matrix,         s3x1dmd,     nothing,  warn__none, 0, 0, {7, -1, -1, 0, 3, -1, -1, 4}, {1, 2, 5, 6}},
   {s1x4,       s3x1dmd, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 2, 5, 6},               {0}},

/*    not yet
   {s1x2,     sbigdhrgl, 0,        0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {2, 8},                     {0}},
   {s1x2,      sbigbone, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {3, 9},                     {0}},
*/

   {s1x2,      sbighrgl, 0,    01010, 0x0D, 0x0, schema_by_array,       sbigdmd,     nothing,  warn__none, 0, 0, {3, 9},                     {0}},
   {s1x2,       sbigdmd, 0,    01010, 0x0E, 0x0, schema_by_array,       sbighrgl,    nothing,  warn__none, 0, 0, {9, 3},                     {0}},
   {s1x2,      sbighrgl, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {9, 3},                     {0}},
   {s1x2,       sbigdmd, 0,        0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {3, 9},                     {0}},
   {s1x2,   s_crosswave, 0,     0x88, 0x0E, 0x0, schema_matrix,         s3x1dmd,     nothing,  warn__none, 0, 0, {2, 6},                     {0, 1, 3, -1, 4, 5, 7, -1}},
   {s1x2,       s3x1dmd, 0,     0x44, 0x0D, 0x0, schema_matrix,         s_crosswave, nothing,  warn__none, 0, 0, {3, 7},                     {0, 1, -1, 2, 4, 5, -1, 6}},
   {s1x2,       s3x1dmd, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {2, 6},                     {0}},
   {s_dhrglass,    s2x4, 0x33,  0x66, 0x0C, 0x0, schema_concentric,     sdmd,        s2x2,     warn__none, 0, 0, {6, 3, 2, 7},            {0, 3, 4, 7}},
   {s_hrglass,     s2x4, 0x33,  0x66, 0x0C, 0x0, schema_concentric,     sdmd,        s2x2,     warn__none, 0, 0, {6, 3, 2, 7},            {0, 3, 4, 7}},
   {s_bone6,   s_galaxy, 0,     0xBB, 0x0E, 0x0, schema_concentric,     s_bone6,     s1x2,     warn__none, 0, 1, {0, 1, 2, 3, 4, 5},         {2, 6}},
   {s_bone6,   s_galaxy, 0,     0xEE, 0x0D, 0x0, schema_concentric,     s_bone6,     s1x2,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5},         {0, 4}},
   {s_hrglass, s_galaxy, 0x44,  0xBB, 0x0D, 0x0, schema_concentric,     s_bone6,     s1x2,     warn__none, 1, 1, {1, 4, 7, 5, 0, 3},         {2, 6}},
   {s_hrglass, s_galaxy, 0x44,  0xEE, 0x0E, 0x0, schema_concentric,     s_bone6,     s1x2,     warn__none, 1, 0, {1, 4, 7, 5, 0, 3},         {0, 4}},
   {s1x2,          s2x4, 0,     0x66, 0x0C, 0x0, schema_concentric,     s1x2,        s2x2,     warn__none, 0, 0, {0, 1},                     {0, 3, 4, 7}},
   {s1x2,          s1x8, 0,     0x44, 0x0C, 0x0, schema_concentric_2_6, s1x2,        s1x6,     warn__none, 0, 0, {0, 1},                     {0, 1, 3, 4, 5, 7}},
   {s1x2,          s1x6, 0,      044, 0x0C, 0x0, schema_concentric,     s1x2,        s1x4,     warn__none, 0, 0, {0, 1},                     {0, 1, 3, 4}},
   {s1x2,          s1x4, 0,      0xA, 0x0C, 0x0, schema_concentric,     s1x2,        s1x2,     warn__none, 0, 0, {0, 1},                     {0, 2}},
   {s1x2,          s1x2, 0,        0, 0x0D, 0x0, schema_matrix,         s_star,      nothing,  warn__none, 0, 1, {0, 2},                     {3, 1}},
   {sdmd,         s3dmd, 0,        0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 5, 7, 11},              {0}},
   {sdmd,       s_3mdmd, 0,    04242, 0x0D, 0x0, schema_matrix,          s3dmd,      nothing,  warn__none, 0, 0, {1, 5, 7, 11},         {0, -1, 2, 3, 4, -1, 6, -1, 8, 9, 10, -1}},
   {sdmd,         s3dmd, 0,    04242, 0x0E, 0x0, schema_matrix,        s_3mdmd,      nothing,  warn__none, 0, 0, {11, 1, 5, 7},         {0, -1, 2, 3, 4, -1, 6, -1, 8, 9, 10, -1}},
   {s1x2,         s3dmd, 0,    07070, 0x0C, 0x0, schema_concentric,     s1x2,        s2x3,     warn__none, 0, 0, {0, 1},                     {0, 1, 2, 6, 7, 8}},

   // Need both of these because they won't canonicalize.
   {s1x4,          s1x4, 0xA,      0, 0x2E, 0x0, schema_matrix,         s1x6,        nothing,  warn__none, 0, 0, {0, -1, 3, -1},       {1, 2, 4, 5}},
   {s1x4,          s1x4, 0,      0xA, 0x2E, 0x0, schema_matrix,         s1x6,        nothing,  warn__none, 0, 0, {1, 2, 4, 5},       {0, -1, 3, -1}},

   // Need both of these because they won't canonicalize.
   {s1x4,          s1x4, 0xA,      0, 0x0D, 0x1, schema_concentric,     s1x4,        s1x2,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 2}},
   {s1x4,          s1x4, 0,      0xA, 0x0D, 0x0, schema_concentric,     s1x4,        s1x2,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 2}},

   // These two must be in this order.
   {s1x6,          s1x8, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 3, 2, 5, 7, 6},         {0}},
   {s1x6,          s1x8, 066,   0x88, 0x1E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {3, -1, -1, 7, -1, -1},         {0}},

   {s1x6,          s1x6, 022,    044, 0x1E, 0x4, schema_matrix,         s1x8,        nothing,  warn__none, 0, 0, {0, -1, 2, 4, -1, 6},    {1, 3, -1, 5, 7, -1}},
   {s1x6,          s1x6, 044,    022, 0x1E, 0x4, schema_matrix,         s1x8,        nothing,  warn__none, 0, 0, {1, 3, -1, 5, 7, -1},    {0, -1, 2, 4, -1, 6}},
   {sdmd,          s1x6, 0xA,      0, 0x1D, 0x1, schema_concentric,     s1x6,        s1x2,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5},      {0, 2}},
   {s1x4,          s1x8, 0xA,   0x22, 0x1D, 0x1, schema_concentric,     s1x6,        s1x2,     warn__none, 0, 0, {0, 3, 2, 4, 7, 6},      {0, 2}},
   /* Can we get rid of this one? */
   {sdmd,          s1x8, 0xA,   0x22, 0x1D, 0x1, schema_concentric,     s1x6,        s1x2,     warn__none, 0, 0, {0, 3, 2, 4, 7, 6},      {0, 2}},
   {s1x4,          s1x8, 0xA,   0x88, 0x1D, 0x1, schema_concentric,     s1x6,        s1x2,     warn__none, 0, 0, {0, 1, 2, 4, 5, 6},      {0, 2}},
   // Can we get rid of this one?
   {sdmd,          s1x8, 0xA,   0x88, 0x1D, 0x1, schema_concentric,     s1x6,        s1x2,     warn__none, 0, 0, {0, 1, 2, 4, 5, 6},      {0, 2}},
   {sdmd,          s1x8, 0,     0xCC, 0x0C, 0x0, schema_concentric,     sdmd,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 4, 5}},
   {s3x4,         s3dmd, 06666,    0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {2, -1, -1, 6, -1, -1, 8, -1, -1, 0, -1, -1}, {0}},
   {s1x2,         s_525, 0,    04040, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {11, 5}, {0}},
   {s3x4,         s_525, 06666,    0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {3, -1, -1, 7, -1, -1, 9, -1, -1, 1, -1, -1}, {0}},
   {s1x4,         s_525, 0,    04444, 0x0D, 0x0, schema_matrix,         sbigdmd,     nothing,  warn__none, 0, 0, {2, 3, 8, 9},               {0, 1, -1, 4, 5, -1, 6, 7, -1, 10, 11, -1}},

   {sdmd,         s_525, 0,    04444, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {2, 5, 8, 11},         {0}},
   {s1x2,          s2x3, 0,      022, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 4},               {0}},
   {s1x2,         s_323, 0,     0x88, 0x0D, 0x2, schema_concentric,     s1x2,        s2x3,     warn__none, 0, 0, {0, 1},               {0, 1, 2, 4, 5, 6}},
   {s1x2,         s_323, 0,     0x88, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {7, 3},               {0}},
   {s2x2,         s_323, 0,     0xAA, 0x0C, 0x0, schema_matrix,         s4x4,        nothing,  warn__check_butterfly, 0, 0, {15, 3, 7, 11},             {12, -1, 0, -1, 4, -1, 8, -1}},
   {s1x4,         s_323, 0,     0xAA, 0x0D, 0x0, schema_concentric,     s1x4,        s2x4,     warn__none, 0, 1, {0, 1, 2, 3},               {2, 1, 3, 4, 6, 5, 7, 0}},
   {sdmd,         s_323, 0,     0xAA, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 3, 5, 7},         {0}},
   {sdmd,       s1x3dmd, 0,     0xAA, 0x1D, 0x0, schema_concentric,     sdmd,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 2, 4, 6}},
   {sdmd,       s1x3dmd, 0,     0xCC, 0x0C, 0x0, schema_concentric,     sdmd,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 4, 5}},
   {sdmd,          s_galaxy, 0, 0xAA, 0x0C, 0x0, schema_concentric,     sdmd,        s_star,   warn__none, 0, 0, {0, 1, 2, 3},               {0, 2, 4, 6}},
   {sdmd,          s1x6, 0,      044, 0x0C, 0x0, schema_concentric,     sdmd,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 3, 4}},
   {s1x6,         s1x10, 0,    0x39C, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {2, 3, 4, 7, 8, 9},         {0}},
   {s1x4,         s1x10, 0,    0x318, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {3, 4, 8, 9},               {0}},
   {s1x4,          s1x8, 0,     0x66, 0x1D, 0x0, schema_concentric,     s1x4,          s1x4,   warn__none, 0, 0, {0, 1, 2, 3},               {0, 3, 4, 7}},
   {s1x4,          s1x8, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {3, 2, 7, 6},               {0}},
   {s1x4,   s_crosswave, 0,        0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {2, 3, 6, 7},               {0}},
   {s_crosswave,   s2x4, 0xCC,  0x99, 0x0C, 0x1, schema_concentric,     s2x2,        s1x4,     warn__none, 0, 0, {1, 2, 5, 6},               {0, 1, 4, 5}},
   {s2x2,   s_crosswave, 0,     0xCC, 0x0C, 0x0, schema_concentric,     s2x2,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 4, 5}},

   /* This is apparently no longer needed.  Is that really true?
   {s2x5,         s_323, 0x1CE,    0, 0x0D, 0x0, schema_matrix,         s_23232,     nothing,  warn__none, 0, 1, {2, -1, -1, -1, 4, 8, -1, -1, -1, 10},   {7, 11, 0, 3, 1, 5, 6, 9}},
   */

   // The 80 bit in rotmaskreject causes this map to be rejected
   // if the action is merge_c1_phantom_real.
   {s1x4,   s_crosswave, 0,     0xCC, 0x8E, 0x0, schema_concentric,     s1x4,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 4, 5}},

   {s_qtag,        s1x8, 0x33,  0xCC, 0x0C, 0x0, schema_concentric,     s1x4,        s1x4,     warn__none, 0, 0, {6, 7, 2, 3},               {0, 1, 4, 5}},
   {s1x4,          s1x8, 0,     0xCC, 0x0C, 0x0, schema_concentric,     s1x4,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 4, 5}},
   {s_qtag,        s1x8, 0x33,  0x55, 0x0C, 0x0, schema_concentric,     s1x4,        s1x4,     warn__none, 0, 0, {6, 7, 2, 3},               {1, 3, 5, 7}},
   {s1x4,          s1x6, 0,      044, 0x2C, 0x0, schema_concentric,     s1x4,        s1x4,     warn__none, 0, 0, {0, 1, 2, 3},               {0, 1, 3, 4}},
   {s1x4,       s1x3dmd, 0,     0xCC, 0x0E, 0x0, schema_matrix,         s1x8,        nothing,  warn__none, 0, 0, {3, 2, 7, 6},               {0, 1, -1, -1, 4, 5, -1, -1}},
   {s1x4,       s1x3dmd, 0,     0xAA, 0x1E, 0x0, schema_matrix,         s1x8,        nothing,  warn__none, 0, 0, {3, 2, 7, 6},               {0, -1, 1, -1, 4, -1, 5, -1}},
   {s1x2,       s1x3dmd, 0,     0x88, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 1, 0, {3, 7},               {0}},
   {s2x2,          s4x4, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {15, 3, 7, 11},             {0}},
   {s2x2,          s2x8, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {3, 4, 11, 12},             {0}},
   {s2x2,          s2x6, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {2, 3, 8, 9},               {0}},
   /*
   {s2x3,          s2x5, 0,    0x1CE, 0x0C, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 2, 3, 6, 7, 8},              {0}},
   */
   {s2x3,          s2x5, 0,    0x1CE, 0x0C, 0x0, schema_concentric,     s2x3,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5},              {0, 4, 5, 9}},
   {s2x2,          s2x5, 0,    0x1CE, 0x2C, 0x0, schema_concentric,     s2x2,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},              {0, 4, 5, 9}},
   {s1x4,          s2x5, 0,    0x1CE, 0x2C, 0x0, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},              {0, 4, 5, 9}},
   {s2x3,         sd2x5, 0,    0x39C, 0x0C, 0x0, schema_concentric,     s2x3,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5},              {0, 6, 5, 1}},
   {s2x2,         sd2x5, 0,    0x39C, 0x2C, 0x0, schema_concentric,     s2x2,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},              {0, 6, 5, 1}},
   {s1x4,         sd2x5, 0,    0x39C, 0x2C, 0x0, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},              {0, 6, 5, 1}},
   {s1x2,   sdblspindle, 0,   0x8888, 0x0D, 0x0, schema_matrix,         s2x7,        nothing,  warn__none, 0, 0, {3, 10},{0, 1, 2, -1, 11, 12, 13, -1, 7, 8, 9, -1, 4, 5, 6, -1}},
   {s1x2,          s2x7, 0,    0x408, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 1, 0, {3, 10},{0}},
   {s1x8,          s2x4, 0xCC,     0, 0x2D, 0x0, schema_matrix,         sdeepxwv,    nothing,  warn__none, 0, 1, {0, 1, -1, -1, 6, 7, -1, -1},{5, 4, 3, 2, 11, 10, 9, 8}},
   {s1x8,          s2x4, 0xAA,  0x66, 0x0E, 0x0, schema_matrix,         s_ptpd,      nothing,  warn__none, 0, 0, {0, -1, 2, -1, 4, -1, 6, -1},{1, -1, -1, 7, 5, -1, -1, 3}},
   {s1x8,          s2x4, 0x99,  0x66, 0x1E, 0x0, schema_matrix,         s_ptpd,      nothing,  warn__none, 0, 0, {-1, 0, 2, -1, -1, 4, 6, -1},{1, -1, -1, 7, 5, -1, -1, 3}},

   // These 4 need to be in this order.
   {s1x6,          s2x4, 0,     0x66, 0x0D, 0x0, schema_concentric,     s1x6,        s2x6,     warn__none, 0, 1, {0, 1, 2, 3, 4, 5},{-1, -1, 3, 4, -1, -1, -1, -1, 7, 0, -1, -1}},
   {s1x6,          s2x4, 044,   0x66, 0x0D, 0x0, schema_matrix,         sdeepxwv,    nothing,  warn__none, 0, 1, {0, 1, -1, 6, 7, -1},{5, -1, -1, 2, 11, -1, -1, 8}},
   {s1x6,          s2x4, 044,   0x66, 0x1ED,0x0, schema_matrix,         s_qtag,      nothing,  warn__none, 0, 1, {6, 7, -1, 2, 3, -1},{5, -1, -1, 0, 1, -1, -1, 4}},
   {s1x6,          s2x4, 022,   0x66, 0x1E, 0x0, schema_matrix,         s_ptpd,      nothing,  warn__none, 0, 0, {0, -1, 2, 4, -1, 6},{1, -1, -1, 7, 5, -1, -1, 3}},

   {s1x8,          s2x4, 0xAA,  0x66, 0x1D, 0x0, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {0, 2, 4, 6},            {0, 3, 4, 7}},
   {s2x2,         s3dmd, 0,    07272, 0x0C, 0x0, schema_matrix,         s4x4,        nothing,  warn__check_butterfly, 0, 0, {15, 3, 7, 11},             {12, -1, 0, -1, -1, -1, 4, -1, 8, -1, -1, -1}},
   {s1x6,        sd3x4, 0,    01616, 0x0C, 0x0, schema_concentric,     s1x6,        s2x3,     warn__none, 0, 1, {0, 1, 2, 3, 4, 5},      {4, 5, 6, 10, 11, 0}},
   {s2x3,        sd3x4, 0,    01616, 0x0C, 0x0, schema_concentric,     s2x3,        s2x3,     warn__none, 0, 1, {0, 1, 2, 3, 4, 5},      {4, 5, 6, 10, 11, 0}},

   {s1x4,        sd3x4, 0,    01616, 0x0D, 0x0, schema_matrix,         s_343,       nothing,  warn__none, 0, 1, {8, 9, 3, 4},      {7, -1, -1, -1, 0, 1, 2, -1, -1, -1, 5, 6}},


   // These next two must be in this sequence.
   {s1x2,          s3x4, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {11, 5},{0}},
   {s1x2,          s3x4, 0,    04646, 0x0C, 0x0, schema_concentric,     s1x2,        s2x3,     warn__none, 0, 1, {0, 1},                  {3, 4, 6, 9, 10, 0}},

   // These next two must be in this sequence.
   {s1x6,          s3x4, 0,        0, 0x0E, 0x0, schema_matrix,         s3x6,        nothing,  warn__none, 0, 0, {15, 16, 17, 6, 7, 8},   {1, 2, 3, 4, 7, 8, 10, 11, 12, 13, 16, 17}},
   {s1x6,          s3x4, 0,    04646, 0x2C, 0x0, schema_concentric,     s1x6,        s2x3,     warn__none, 0, 1, {0, 1, 2, 3, 4, 5},      {3, 4, 6, 9, 10, 0}},

   // These next three must be in this sequence.
   {s2x3,          s3x4, 0,    06161, 0x0E, 0x0, schema_concentric,     s2x3,        s2x3,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5},      {1, 2, 3, 7, 8, 9}},
   {s2x3,          s3x4, 0,    07070, 0x0E, 0x0, schema_concentric,     s2x3,        s2x3,     warn__none, 0, 0, {0, 1, 2, 3, 4, 5},      {0, 1, 2, 6, 7, 8}},
   {s2x3,          s3x4, 0,    04646, 0x0C, 0x0, schema_concentric,     s2x3,        s2x3,     warn__none, 0, 1, {0, 1, 2, 3, 4, 5},      {3, 4, 6, 9, 10, 0}},

   {s2x2,        s_bone, 0,     0xCC, 0x2C, 0x0, schema_concentric,     s2x2,        s2x2,     warn__none, 0, 0, {0, 1, 2, 3},            {0, 1, 4, 5}},
   {s2x2,          s2x4, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 2, 5, 6},               {0}},
   {s2x4,          s4x4, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {10, 15, 3, 1, 2, 7, 11, 9},{0}},

   // These next three must be in this sequence.
   // Use this one last.  (This used to be the only one.)  Place the 2x4 directly on top of the 4x4.
   // If there is a conflict (and, at this point, it seems that there is), we lose.
   {s2x4,          s4x4, 0,        0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {14, 3, 7, 5, 6, 11, 15, 13},{0}},
   // Use this one next.  If the schema isn't too strict, push the outer part of the 4x4 out to a 4x6.
   // But not all the way out to a 4x8.  If there's still a conflict on the 4x6, we lose.
   {s2x4,          s4x4, 0,        0, 0xAD, 0x0, schema_matrix,            s4x6,     nothing,  warn__none, 0, 1, {10, 9, 8, 7, 22, 21, 20, 19}, {0, 1, 4, 10, 5, 6, 23, 7, 12, 13, 16, 22, 17, 18, 11, 19}},
   // This one is tried first.  If no conlict, just place the 2x4 on top of the 4x4.
   {s2x4,          s4x4, 0,        0, 0xD, 0x10, schema_nothing,       nothing,     nothing,  warn__none, 0, 0, {14, 3, 7, 5, 6, 11, 15, 13},{0}},

   {s2x4,      s_c1phan, 0,        0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {0, 2, 7, 5, 8, 10, 15, 13},{0}},
   {s2x4,      s_c1phan, 0,        0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {4, 6, 11, 9, 12, 14, 3, 1},{0}},
   {s1x6,       sbigx,    044,      0, 0x0E, 0x0, schema_nothing,       nothing,     nothing,  warn__none, 0, 0, {2, 3, -1, 8, 9, -1},        {0}},
   {s1x6,      s_1x2dmd, 044,    022, 0x1D, 0x0, schema_matrix,         s3x1dmd,     nothing,  warn__none, 0, 1, {0, 1, -1, 4, 5, -1},        {7, -1, 2, 3, -1, 6}},
   {s1x6,      s_1x2dmd, 044,    022, 0x1E, 0x0, schema_matrix,         s1x3dmd,     nothing,  warn__none, 0, 0, {1, 2, -1, 5, 6, -1},        {0, -1, 3, 4, -1, 7}},
   {s2x3,      s_1x2dmd, 022,    022, 0xBE, 0x0, schema_matrix,         s_spindle,  nothing,  warn__none, 0, 0, {0, -1, 2, 4, -1, 6},        {7, -1, 1, 3, -1, 5}},
   {s1x6,       s1x3dmd, 022,   0xAA, 0x1E, 0x0, schema_matrix,         s1x8,       nothing,   warn__none, 0, 0, {1, -1, 2, 5, -1, 6},        {0, -1, 3, -1, 4, -1, 7, -1}},
   {s1x4,       s1x3dmd, 0,     0xAA, 0x1D, 0x0, schema_matrix,         s_crosswave, nothing,  warn__none, 0, 0, {2, 3, 6, 7},                {0, -1, 1, -1, 4, -1, 5, -1}},
   {s1x4,       s1x3dmd, 0,     0xCC, 0x0D, 0x0, schema_matrix,         s_crosswave, nothing,  warn__none, 0, 0, {2, 3, 6, 7},                {0, 1, -1, -1, 4, 5, -1, -1}},
   {s2x2,       s1x3dmd, 0,     0xAA, 0x1E, 0x0, schema_matrix,         s_rigger,   nothing,   warn__none, 0, 0, {0, 1, 4, 5},                {6, -1, 7, -1, 2, -1, 3, -1}},
   {s2x3,       s1x3dmd, 0,     0x66, 0x2E, 0x0, schema_matrix,         s_spindle,   nothing,  warn__none, 0, 0, {0, 1, 2, 4, 5, 6},         {7, -1, -1, 1, 3, -1, -1, 5}},
   {s1x4,    s_dhrglass, 0xA,   0x44, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {6, -1, 2, -1},             {0}},
   {s1x2,    s_dhrglass, 0x0,   0x88, 0x0C, 0x0, schema_concentric,     s1x2,     s_bone6,     warn__none, 0, 0, {0, 1},            {0, 1, 2, 4, 5, 6}},
   {s2x3,    s_dhrglass, 022,   0x33, 0x0C, 0x1, schema_concentric,     sdmd,        s2x2,     warn__none, 0, 0, {6, 3, 2, 7},            {0, 2, 3, 5}},
   {s2x3,     s_hrglass, 022,   0x33, 0x0C, 0x1, schema_concentric,     sdmd,        s2x2,     warn__none, 0, 0, {6, 3, 2, 7},            {0, 2, 3, 5}},

   // This one is used for finishing right wing follow to a diamond from
   // suitable T-bones.  It seems that it should be useful in its own right
   // for merging diamondlike things, but no other test seems to need it.
   {s_qtag,        s1x8, 0x33,  0xAA, 0xAE, 0x1, schema_rev_checkpoint, s1x4,       s1x4,  warn__none, 0, 0, {0, 2, 4, 6},    {6, 7, 2, 3}},

   // And this one is used instead.
   {s_qtag,        s1x8, 0x44,  0xCC, 0x0E, 0x0, schema_matrix,         s4dmd,       nothing,  warn__none, 0, 0, {1, 2, -1, 7, 9, 10, -1, 15},    {13, 14, -1, -1, 5, 6, -1, -1}},
   // The 100 bit in rotmaskreject causes this map to be rejected
   // if the action is merge_after_dyp.
   // See vi19t / vi07t.
   {s_qtag,        s1x8, 0x44,  0xCC, 0x10E, 0x0, schema_matrix,         s4dmd,      nothing,  warn__none, 0, 0, {1, 2, -1, 7, 9, 10, -1, 15},    {12, 13, -1, -1, 4, 5, -1, -1}},

   {s2x3,          s1x8, 0,     0xCC, 0x0D, 0x0, schema_matrix,         s4dmd,       nothing,  warn__none, 0, 0, {2, 7, 9, 10, 15, 1},    {12, 13, -1, -1, 4, 5, -1, -1}},
   {s2x3,          s1x8, 022,   0xAA, 0x1D, 0x1, schema_concentric,     s1x4,        s2x2,     warn__none, 0, 0, {0, 2, 4, 6},            {0, 2, 3, 5}},
   {s2x3,          s1x8, 022,   0xAA, 0x2E, 0x0, schema_matrix,         s_ptpd,      nothing,  warn__none, 0, 0, {1, -1, 7, 5, -1, 3},       {0, -1, 2, -1, 4, -1, 6, -1}},
   {s2x3,          s1x8, 022,   0x99, 0x1E, 0x0, schema_matrix,         s_ptpd,      nothing,  warn__none, 0, 0, {1, -1, 7, 5, -1, 3},       {-1, 0, 2, -1, -1, 4, 6, -1}},
   {s2x3,        s_qtag, 0,        0, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {1, 3, 4, 5, 7, 0},         {0}},
   {s1x4,     s_hrglass, 0xA,   0x88, 0x0D, 0x1, schema_concentric,     s_short6,    s1x2,     warn__check_galaxy, 1, 0, {1, 2, 4, 5, 6, 0},      {0, 2}},
   {s1x4,        s_qtag, 0xA,   0x88, 0x0D, 0x1, schema_concentric,     s_short6,    s1x2,     warn__check_galaxy, 1, 0, {1, 2, 4, 5, 6, 0},      {0, 2}},
   {s1x4,     s_hrglass, 0xA,   0x44, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {6, -1, 2, -1},             {0}},
   {s1x4,          sdmd, 0xA,    0x5, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {0, -1, 2, -1},             {0}},
   {s_ntrgl6ccw, s_nxtrglccw, 0,   0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {0, 1, 2, 4, 5, 6},               {0}},
   {s_ntrgl6cw,   s_nxtrglcw, 0,   0, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {0, 1, 2, 4, 5, 6},               {0}},

   {s_ntrglcw,    s_323,  0x33, 0x33, 0x0D, 0x0, schema_matrix,         s_ntrglcw,   nothing,  warn__none, 0, 1, {-1, -1, 2, 3, -1, -1, 6, 7},               {-1, -1, 0, 1, -1, -1, 4, 5}},
   {s_ntrglccw,   s_323,  0xCC, 0x66, 0x0D, 0x0, schema_matrix,         s_ntrglccw,  nothing,  warn__none, 0, 1, {0, 1, -1, -1, 4, 5, -1, -1},               {7, -1, -1, 2, 3, -1, -1, 6}},
   {s_ntrglcw,    s_323,  0xCC, 0xCC, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {2, 3, -1, -1, 6, 7, -1, -1},               {0}},
   {s_ntrglccw,   s_323,  0x33, 0x99, 0x0D, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {-1, -1, 3, 4, -1, -1, 7, 0},               {0}},

   {s1x4,      s_short6, 0,      055, 0x2D, 0x0, schema_concentric,     s1x4,        s1x2,     warn__none, 0, 1, {0, 1, 2, 3},            {1, 4}},
   {s1x4,      s_short6, 0xA,      0, 0x0D, 0x0,     schema_matrix,      s_rigger,   nothing,  warn__none, 0, 1, {6, -1, 2, -1},             {5, 7, 0, 1, 3, 4}},

   // These 2 must be in the order shown.
   {s1x4,        s_qtag, 0,     0x88, 0x0E, 0x0, schema_matrix,         s3x6,        nothing,  warn__none, 0, 0, {16, 17, 7, 8},               {2, 3, 6, -1, 11, 12, 15, -1}},
   {s1x4,        s_qtag, 0,     0xCC, 0x0E, 0x0, schema_nothing,        nothing,     nothing,  warn__none, 0, 0, {6, 7, 2, 3},               {0}},
   {nothing, nothing}};



// These are used in mirror_this, for handling difficult triangles.

const coordrec tgl3_0 = {s_trngl, 3,
   {  0,  -2,   2},
   {  0,   4,   4}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  1,  2, -1, -1, -1,
      -1, -1, -1, -1,  0, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

const coordrec tgl3_1 = {s_trngl, 3,
   {  0,   4,   4},
   {  0,   2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  0,  1, -1, -1,
      -1, -1, -1, -1, -1,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

const coordrec tgl4_0 = {s_trngl4, 3,
   {  0,   0,  -2,   2},
   { -4,   0,   4,   4}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  2,  3, -1, -1, -1,
      -1, -1, -1, -1,  1, -1, -1, -1,
      -1, -1, -1, -1,  0, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

const coordrec tgl4_1 = {s_trngl4, 3,
   { -4,   0,   4,   4},
   {  0,   0,   2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2, -1, -1,
      -1, -1, -1, -1, -1,  3, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

// These are used in sdmoves to fudge matrix calls.

const coordrec squeezethingglass = {s_hrglass, 3,
   { -4,   4,   8,   0,   4,  -4,  -8,   0},
   {  6,   6,   0,   2,  -6,  -6,   0,  -2}, {0}};

const coordrec squeezethinggal = {s_galaxy, 3,
   { -6,  -2,   0,   2,   6,   2,   0,  -2},
   {  0,   2,   6,   2,   0,  -2,  -6,  -2}, {0}};

const coordrec squeezething343 = {s_343, 3,
   { -4,   0,   4,   6,   2,   4,   0,  -4,  -6,  -2},
   {  6,   6,   6,   0,   0,  -6,  -6,  -6,   0,   0}, {0}};

const coordrec squeezethingqtag = {s_qtag, 3,
   { -2,   2,   6,   2,   2,  -2,  -6,  -2},
   {  4,   4,   0,   0,  -4,  -4,   0,   0}, {0}};

const coordrec squeezething4dmd = {s4dmd, 3,
   {-10,  -5,   5,  10,  14,  10,   6,   2,  10,   5,  -5, -10, -14, -10,  -6,  -2},
   {  6,   6,   6,   6,   0,   0,   0,   0,  -6,  -6,  -6,  -6,   0,   0,   0,   0}, {0}};

const coordrec squeezefinalglass = {s_hrglass, 3,
   { -2,   2,   6,   0,   2,  -2,  -6,   0},
   {  6,   6,   0,   2,  -6,  -6,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1, -1, -1, -1,
      -1, -1,  6, -1,  3, -1,  2, -1,
      -1, -1, -1, -1,  7, -1, -1, -1,
      -1, -1, -1,  5,  4, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

const coordrec truck_to_ptpd = {s_ptpd, 3,
   {-10,  -6,  -2,  -6,  10,   6,   2,   6},
   {  0,   2,   0,  -2,   0,  -2,   0,   2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1,  2,  6,  7,  4, -1,
      -1, -1,  3, -1, -1,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

const coordrec truck_to_deepxwv = {sdeepxwv, 4,
   {-11,  -7,  -2,  -2,  -2,  -2,  11,   7,   2,   2,   2,   2},
   {  0,   0,   6,   2,  -2,  -6,   0,   0,  -6,  -2,   2,   6}, {
      -1, -1, -1, -1, -1, -1, -1,  2, 11, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  0,  1,  3, 10,  7,  6, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1,  4,  9, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1,  5,  8, -1, -1, -1, -1, -1, -1, -1}};

const coordrec press_4dmd_4x4 = {s4x4, 3,
   { 11,   9,   9,   1,  11,   5,  -5,   1, -11,  -9,  -9,  -1, -11,  -5,   5,  -1},
   {  7,   1,  -1,   1,  -7,  -7,  -7,  -1,  -7,  -1,   1,  -1,   7,   7,   7,   1}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 12, 13, -1, -1, 14,  0, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  8,  6, -1, -1,  5,  4, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

const coordrec press_4dmd_qtag1 = {s_qtag, 3,
   { -7,  -1,   6,   2,   7,   1,  -6,  -2},
   {  5,   5,   0,   0,  -5,  -5,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1, -1, -1, -1, -1,
      -1, -1,  6,  7,  3,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  5,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

const coordrec press_4dmd_qtag2 = {s_qtag, 3,
   {  1,   7,   6,   2,  -1,   -7,  -6,  -2},
   {  5,   5,   0,   0,  -5,  -5,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  0,  1, -1, -1,
      -1, -1,  6,  7,  3,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  5,  4, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

const coordrec press_qtag_4dmd1 = {s4dmd, 3,
   {-11,  -5,   0,   9,  14,  10,   6,   2,  11,   5,   0,  -9, -14, -10,  -6,  -2},
   {  5,   5,   5,   5,   0,   0,   0,   0,  -5,  -5,  -5,  -5,   0,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1, -1,  2, -1,  3, -1,
      12, 13, 14, 15,  7,  6,  5,  4,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 11, -1, -1, 10,  9,  8, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

const coordrec press_qtag_4dmd2 = {s4dmd, 3,
   { -8,   1,   5,  11,  14,  10,   6,   2,   8,  -1,  -5, -11, -14, -10,  -6,  -2},
   {  5,   5,   5,   5,   0,   0,   0,   0,  -5,  -5,  -5,  -5,   0,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1,  1,  2,  3, -1,
      12, 13, 14, 15,  7,  6,  5,  4,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 11, 10,  9, -1, -1,  8, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

const coordrec acc_crosswave = {s_crosswave, 3,
   { -8,  -4,   0,   0,   8,   4,   0,   0},
   {  0,   0,   6,   2,   0,   0,  -6,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  2, -1, -1, -1,
      -1, -1,  0,  1,  3,  5,  4, -1,
      -1, -1, -1, -1,  7, -1, -1, -1,
      -1, -1, -1, -1,  6, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

// These are used in setup_coords.

static const coordrec thing1x1 = {s1x1, 3,
   {  0},
   {  0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  0, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x2 = {s1x2, 3,
   { -2,   2},
   {  0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x3 = {s1x3, 3,
   { -4,   0,   4},
   {  0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x4 = {s1x4, 3,
   { -6,  -2,   6,   2},
   {  0,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  3,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x5 = {s1x5, 3,
   { -8,  -4,   0,   4,   8},
   {  0,   0,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  2,  4,  3, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x6 = {s1x6, 3,
   {-10,  -6,  -2,  10,   6,   2},
   {  0,   0,   0,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1,  2,  5,  4,  3, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x8 = {s1x8, 3,
   {-14, -10,  -2,  -6,  14,  10,   2,   6},
   {  0,   0,   0,   0,   0,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
       0,  1,  3,  2,  6,  7,  5,  4,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x10 = {s1x10, 4,
   {-18, -14, -10,  -6,  -2,  18,  14,  10,   6,   2},
   {  0,   0,   0,   0,   0,   0,   0,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2,  3,  4,  9,  8,  7,  6,  5, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x12 = {s1x12, 4,
   {-22, -18, -14, -10,  -6,  -2,  22,  18,  14,  10,   6,   2},
   {  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  2,  3,  4,  5, 11, 10,  9,  8,  7,  6, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x14 = {s1x14, 4,
   {-26, -22, -18, -14, -10,  -6,  -2,  26,  22,  18,  14,  10,   6,   2},
   {  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1,  2,  3,  4,  5,  6, 13, 12, 11, 10,  9,  8,  7, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x16 = {s1x16, 4,
   {-30, -26, -22, -18, -14, -10,  -6,  -2,  30,  26,  22,  18,  14,  10,   6,   2},
   {  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       0,  1,  2,  3,  4,  5,  6,  7, 15, 14, 13, 12, 11, 10,  9,  8,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingdmd = {sdmd, 3,
   { -5,   0,   4,   0},
   {  0,   2,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1,  1,  2, -1, -1,
      -1, -1, -1, -1,  3, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec nicethingdmd = {sdmd, 3,
   { -5,   0,   5,   0},
   {  0,   2,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1,  1,  2, -1, -1,
      -1, -1, -1, -1,  3, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing2x2 = {s2x2, 3,
   { -2,   2,   2,  -2},
   {  2,   2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1, -1, -1, -1,
      -1, -1, -1,  3,  2, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing2x3 = {s2x3, 3,
   { -4,   0,   4,   4,   0,  -4},
   {  2,   2,   2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2, -1, -1,
      -1, -1, -1,  5,  4,  3, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing2x4 = {s2x4, 3,
   { -6,  -2,   2,   6,   6,   2,  -2,  -6},
   {  2,   2,   2,   2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  2,  3, -1, -1,
      -1, -1,  7,  6,  5,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing2x5 = {s2x5, 3,
   { -8,  -4,   0,   4,   8,   8,   4,   0,  -4,  -8},
   {  2,   2,   2,   2,   2,  -2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  2,  3,  4, -1,
      -1, -1,  9,  8,  7,  6,  5, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing2x6 = {s2x6, 3,
   {-10,  -6,  -2,   2,   6,  10,  10,   6,   2,  -2,  -6,  -10},
   {  2,   2,   2,   2,   2,   2,  -2,  -2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1,  2,  3,  4,  5, -1,
      -1, 11, 10,  9,  8,  7,  6, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing2x7 = {s2x7, 3,
   { -12, -8,  -4,   0,   4,   8,  12,  12,   8,   4,   0,  -4,  -8, -12},
   {  2,   2,   2,   2,   2,   2,   2,  -2,  -2,  -2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1,  2,  3,  4,  5,  6,
      -1, 13, 12, 11, 10,  9,  8,  7,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing2x8 = {s2x8, 3,
   {-14, -10,  -6,  -2,   2,   6,  10,  14,  14,  10,   6,   2,  -2,  -6, -10, -14},
   {  2,   2,   2,   2,   2,   2,   2,   2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
       0,  1,  2,  3,  4,  5,  6,  7,
      15, 14, 13, 12, 11, 10,  9,  8,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

// Used only by mirror_this.
static const coordrec thing1p5x8 = {nothing, 3,
   {-14, -10,  -6,  -2,   2,   6,  10,  14,  14,  10,   6,   2,  -2,  -6, -10, -14},
   {  3,   3,   3,   3,   3,   3,   3,   3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
       0,  1,  2,  3,  4,  5,  6,  7,
      15, 14, 13, 12, 11, 10,  9,  8,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing2x9 = {s2x9, 4,
   { -16, -12, -8,  -4,   0,   4,   8,  12,  16,  16,  12,   8,   4,   0,  -4,  -8, -12, -16},
   {  2,   2,   2,   2,   2,   2,   2,   2,   2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8, -1, -1, -1,
      -1, -1, -1, -1, 17, 16, 15, 14, 13, 12, 11, 10,  9, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing2x10 = {s2x10, 4,
   {-18, -14, -10,  -6,  -2,   2,   6,  10,  14,  18,  18,  14,  10,   6,   2,  -2,  -6, -10, -14, -18},
   {  2,   2,   2,   2,   2,   2,   2,   2,   2,   2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1,
      -1, -1, -1, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing2x12 = {s2x12, 4,
   {-22, -18, -14, -10,  -6,  -2,   2,   6,  10,  14,  18,  22,
     22,  18,  14,  10,   6,   2,  -2,  -6, -10, -14, -18, -22},
   {  2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
     -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, -1, -1,
      -1, -1, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing3x6 = {s3x6, 3,
   {-10,  -6,  -2,   2,   6,  10,  10,   6,   2,  10,   6,   2,  -2,  -6, -10, -10,  -6,  -2},
   {  4,   4,   4,   4,   4,   4,   0,   0,   0,  -4,  -4,  -4,  -4,  -4,  -4,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1,  2,  3,  4,  5, -1,
      -1, 15, 16, 17,  8,  7,  6, -1,
      -1, 14, 13, 12, 11, 10,  9, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing3x8 = {s3x8, 3,
   {-14, -10,  -6,  -2,   2,   6,  10,  14,
     14,  10,   6,   2,  14,  10,   6,   2,
     -2,  -6, -10, -14, -14, -10,  -6,  -2},
   {  4,   4,   4,   4,   4,   4,   4,   4,
      0,   0,   0,   0,  -4,  -4,  -4,  -4,
     -4,  -4,  -4,  -4,   0,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
       0,  1,  2,  3,  4,  5,  6,  7,
      20, 21, 22, 23, 11, 10,  9,  8,
      19, 18, 17, 16, 15, 14, 13, 12,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing3x4 = {s3x4, 3,
   { -6,  -2,   2,   6,   6,   2,   6,   2,  -2,  -6,  -6,  -2},
   {  4,   4,   4,   4,   0,   0,  -4,  -4,  -4,  -4,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  2,  3, -1, -1,
      -1, -1, 10, 11,  5,  4, -1, -1,
      -1, -1,  9,  8,  7,  6, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingd3x4 = {sd3x4, 3,
   { -8,  -4,   0,   4,   8,   8,   8,   4,   0,  -4,  -8,  -8},
   {  4,   2,   2,   2,   4,   0,  -4,  -2,  -2,  -2,  -4,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1, -1, -1,  4, -1,
      -1, -1, 11,  1,  2,  3,  5, -1,
      -1, -1, 10,  9,  8,  7,  6, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingd4x5 = {sd4x5, 3,
   {-12, -12, -12, -12, -12,  -8,  -4,   0,   4,   8,  12,  12,  12,  12,  12,   8,   4,   0,  -4,  -8},
   {  8,   4,   0,  -4,  -8,   2,   2,   2,   2,   2,  -8,  -4,   0,   4,   8,  -2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0, -1, -1, -1, -1, -1, 14,
      -1,  1, -1, -1, -1, -1, -1, 13,
      -1,  2,  5,  6,  7,  8,  9, 12,
      -1,  3, 19, 18, 17, 16, 15, 11,
      -1,  4, -1, -1, -1, -1, -1, 10,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing_323 = {s_323, 3,
   { -4,   0,   4,   2,   4,   0,  -4,  -2},
   {  4,   4,   4,   0,  -4,  -4,  -4,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2, -1, -1,
      -1, -1, -1,  7,  3, -1, -1, -1,
      -1, -1, -1,  6,  5,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing_343 = {s_343, 3,
   { -4,   0,   4,   6,   2,   4,   0,  -4,  -6,  -2},
   {  4,   4,   4,   0,   0,  -4,  -4,  -4,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2, -1, -1,
      -1, -1,  8,  9,  4,  3, -1, -1,
      -1, -1, -1,  7,  6,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing_3223 = {s_3223, 3,
   { -6,   6,   6,   2,  -2,   6,  -6,  -6,  -2,   2},
   {  4,   4,   0,   2,   2,  -4,  -4,   0,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1, -1,  1, -1, -1,
      -1, -1,  7,  4,  3,  2, -1, -1,
      -1, -1,  6,  8,  9,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing_525 = {s_525, 3,
   { -8,  -4,   0,   4,   8,   2,   8,   4,   0,  -4,  -8,  -2},
   {  4,   4,   4,   4,   4,   0,  -4,  -4,  -4,  -4,  -4,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  2,  3,  4, -1,
      -1, -1, -1, 11,  5, -1, -1, -1,
      -1, -1, 10,  9,  8,  7,  6, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing_545 = {s_545, 3,
   { -8,  -4,   0,   4,   8,   6,   2,   8,   4,   0,  -4,  -8,  -6,  -2},
   {  4,   4,   4,   4,   4,   0,   0,  -4,  -4,  -4,  -4,  -4,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  2,  3,  4, -1,
      -1, -1, 12, 13,  6,  5, -1, -1,
      -1, -1, 11, 10,  9,  8,  7, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing_h545 = {sh545, 3,
   { -8,  -4,   0,   4,   8,   8,   2,   8,   4,   0,  -4,  -8,  -8,  -2},
   {  4,   4,   4,   4,   4,   0,   0,  -4,  -4,  -4,  -4,  -4,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  2,  3,  4, -1,
      -1, -1, 12, 13,  6, -1,  5, -1,
      -1, -1, 11, 10,  9,  8,  7, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thinggal = {s_galaxy, 3,
   { -7,  -2,   0,   2,   7,   2,   0,  -2},
   {  0,   2,   7,   2,   0,  -2,  -7,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  2, -1, -1, -1,
      -1, -1,  0,  1,  3,  4, -1, -1,
      -1, -1, -1,  7,  5, -1, -1, -1,
      -1, -1, -1, -1,  6, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingthar = {s_thar, 3,
   { -9,  -5,   0,   0,   9,   5,   0,   0},
   {  0,   0,   9,   5,   0,   0,  -9,  -5}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  2, -1, -1, -1,
      -1, -1, -1, -1,  3, -1, -1, -1,
      -1,  0,  1, -1, -1,  5,  4, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  7, -1, -1, -1,
      -1, -1, -1, -1,  6, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingalamo = {s_alamo, 3,
   { -2,   2,   5,   5,   2,  -2,  -5,  -5},
   {  5,   5,   2,  -2,  -5,  -5,  -2,   2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1, -1, -1, -1,
      -1, -1,  7, -1, -1,  2, -1, -1,
      -1, -1,  6, -1, -1,  3, -1, -1,
      -1, -1, -1,  5,  4, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingx1x8 = {sx1x8, 3,
   {-14, -10,  -6,  -2,   0,   0,   0,   0,  14,  10,   6,   2,   0,   0,   0,   0},
   {  0,   0,   0,   0,  14,  10,   6,   2,   0,   0,   0,   0, -14, -10,  -6,  -2}, {
      -1, -1, -1, -1,  4, -1, -1, -1,
      -1, -1, -1, -1,  5, -1, -1, -1,
      -1, -1, -1, -1,  6, 11, -1, -1,   // Note funny placement of 11.
       0,  1,  2,  3,  7, 10,  9,  8,
      -1, -1, -1, -1, 15, -1, -1, -1,
      -1, -1, -1, -1, 14, -1, -1, -1,
      -1, -1, -1, -1, 13, -1, -1, -1,
      -1, -1, -1, -1, 12, -1, -1, -1}};

static const coordrec thingxwv = {s_crosswave, 3,
   { -9,  -5,   0,   0,   9,   5,   0,   0},
   {  0,   0,   6,   2,   0,   0,  -6,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  2, -1, -1, -1,
      -1,  0,  1, -1,  3,  5,  4, -1,
      -1, -1, -1, -1,  7, -1, -1, -1,
      -1, -1, -1, -1,  6, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingrigger = {s_rigger, 3,
   { -2,   2,  10,   6,   2,  -2, -10,  -6},
   {  2,   2,   0,   0,  -2,  -2,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  6,  7,  0,  1,  3,  2, -1,
      -1, -1, -1,  5,  4, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing3x3 = {s3x3, 3,
   { -4,   0,   4,   4,   4,   0,  -4,  -4,   0},
   {  4,   4,   4,   0,  -4,  -4,  -4,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2, -1, -1,
      -1, -1, -1,  7,  8,  3, -1, -1,
      -1, -1, -1,  6,  5,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingd2x5 = {sd2x5, 3,
   { -6,  -6,  -2,  -2,  -2,   6,   6,   2,   2,   2},
   {  2,  -2,   4,   0,  -4,  -2,   2,  -4,   0,   4}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  2,  9, -1, -1, -1,
      -1, -1,  0,  3,  8,  6, -1, -1,
      -1, -1,  1,  4,  7,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingd2x7 = {sd2x7, 3,
   {-10,  -6,  -2,  -2,  -2,   6,  10,  10,   6,   2,   2,   2,  -6, -10},
   {  2,   2,   4,   0,  -4,   2,   2,  -2,  -2,  -4,   0,   4,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  2, 11, -1, -1, -1,
      -1,  0,  1,  3, 10,  5,  6, -1,
      -1, 13, 12,  4,  9,  8,  7, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingntrgl6cw = {s_ntrgl6cw, 3,
   { -4,   2,   6,   4,  -2,  -6},
   {  2,   2,   2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2, -1, -1,
      -1, -1,  5,  4, -1,  3, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingntrgl6ccw = {s_ntrgl6ccw, 3,
   { -6,  -2,   4,   6,   2,  -4},
   {  2,   2,   2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1, -1,  2, -1, -1,
      -1, -1, -1,  5,  4,  3, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingnftrgl6cw = {s_nftrgl6cw, 3,
   { -5,   2,   6,   5,  -2,  -6},
   {  5,   0,   0,  -5,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1, -1, -1, -1, -1,
      -1, -1,  5,  4,  1,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  3, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingnftrgl6ccw = {s_nftrgl6ccw, 3,
   { -6,  -2,   5,   6,   2,  -5},
   {  0,   0,   5,   0,   0,  -5}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  2, -1, -1,
      -1, -1,  0,  1,  4,  3, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  5, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingntrglcw = {s_ntrglcw, 3,
   { -6,   0,   4,   8,   6,   0,  -4,  -8},
   {  2,   2,   2,   2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1,  1,  2,  3, -1,
      -1, -1,  7,  6,  5,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingntrglccw = {s_ntrglccw, 3,
   { -8,  -4,   0,   6,   8,   4,   0,  -6},
   {  2,   2,   2,   2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  2,  3, -1, -1,
      -1, -1,  7, -1,  6,  5,  4, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingnxtrglcw = {s_nxtrglcw, 3,
   { -4,   2,   6,  10,   4,  -2,  -6, -10},
   {  2,   2,   2,   0,  -2,  -2,  -2,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  7, -1,  0,  1,  2,  3, -1,
      -1, -1,  6,  5, -1,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingnxtrglccw = {s_nxtrglccw, 3,
   { -6,  -2,   4,  10,   6,   2,  -4, -10},
   {  2,   2,   2,   0,  -2,  -2,  -2,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  7,  0,  1, -1,  2,  3, -1,
      -1, -1, -1,  6,  5,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingspindle = {s_spindle, 3,
   { -4,   0,   4,   8,   4,   0,  -4,  -8},
   {  2,   2,   2,   0,  -2,  -2,  -2,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  7,  0,  1,  2,  3, -1,
      -1, -1, -1,  6,  5,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingspindle12 = {s_spindle12, 3,
   { -4,   0,   4,  12,   8,   4,   0,  -4, -12,  -8},
   {  2,   2,   2,   0,   0,  -2,  -2,  -2,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  8,  9,  0,  1,  2,  4,  3,
      -1, -1, -1,  7,  6,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingdblspindle = {sdblspindle, 4,
   {-14, -10,  -6,  -2,  -6, -10, -14, -18,  14,  10,   6,   2,   6,  10,  14,  18},
   {  2,   2,   2,   0,  -2,  -2,  -2,   0,  -2,  -2,  -2,   0,   2,   2,   2,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  7,  0,  1,  2,  3, 11, 12, 13, 14, 15, -1, -1, -1,
      -1, -1, -1, -1,  6,  5,  4, -1, -1, 10,  9,  8, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingdblbone = {sdblbone, 4,
   {-22,  -2,  -6, -10,  -2, -22, -18, -14,  22,   2,   6,  10,   2,  22,  18,  14},
   {  2,   2,   0,   0,  -2,  -2,   0,   0,  -2,  -2,   0,   0,   2,   2,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  6,  7,  3,  2,  1, 12, 10, 11, 15, 14, 13, -1, -1,
      -1, -1,  5, -1, -1, -1, -1,  4,  9, -1, -1, -1, -1,  8, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingdblbone6 = {sdblbone6, 4,
   {-14,  -2,  -6,  -2, -14, -10,  14,   2,   6,   2,  14,  10},
   {  2,   2,   0,  -2,  -2,   0,  -2,  -2,   0,   2,   2,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  0,  5,  2,  1,  9,  8, 11, 10, -1, -1, -1, -1,
      -1, -1, -1, -1,  4, -1, -1,  3,  7, -1, -1,  6, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingdblrig = {sdblrig, 4,
   {-14, -10,  -2,  -6, -10, -14, -22, -18,  14,  10,   2,   6,  10,  14,  22,  18},
   {  2,   2,   0,   0,  -2,  -2,   0,   0,  -2,  -2,   0,   0,   2,   2,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  6,  7,  0,  1,  3,  2, 10, 11, 12, 13, 15, 14, -1, -1,
      -1, -1, -1, -1,  5,  4, -1, -1, -1, -1,  9,  8, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingnptrglcw = {s_nptrglcw, 3,
   { -8,  -2,   6,  10,   8,   2,  -6, -10},
   {  2,   0,   2,   2,  -2,   0,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  5,  2,  3, -1,
      -1,  7,  6, -1, -1, -1,  4, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingnptrglccw = {s_nptrglccw, 3,
   {-10,  -6,  -2,   8,  10,   6,   2,  -8},
   {  2,   2,   0,   2,  -2,  -2,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1,  2,  6, -1,  3, -1,
      -1, -1,  7, -1, -1,  5,  4, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingbone = {s_bone, 3,
   {-10,  10,   6,   2,  10, -10,  -6,  -2},
   {  2,   2,   0,   0,  -2,  -2,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  6,  7,  3,  2,  1, -1,
      -1,  5, -1, -1, -1, -1,  4, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingbone6 = {s_bone6, 3,
   { -6,   6,   2,   6,  -6,  -2},
   {  2,   2,   0,  -2,  -2,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  5,  2,  1, -1, -1,
      -1, -1,  4, -1, -1,  3, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingqtag = {s_qtag, 3,       /* "fudged" coordinates */
   { -4,   5,   6,   2,   4,  -5,  -6,  -2},
   {  5,   5,   0,   0,  -5,  -5,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0, -1,  1, -1, -1,
      -1, -1,  6,  7,  3,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  5, -1, -1,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec nicethingqtag = {s_qtag, 3,   /* "true" coordinates */
   { -5,   5,   6,   2,   5,  -5,  -6,  -2},
   {  5,   5,   0,   0,  -5,  -5,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1, -1,  1, -1, -1,
      -1, -1,  6,  7,  3,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  5, -1, -1,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingspgdmdcw = {spgdmdcw, 3,
   { -5,   5,   6,   2,   5,  -5,  -6,  -2},
   {  9,   1,  -4,  -4,  -9,  -1,   4,   4}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1, -1, -1, -1, -1,
      -1, -1,  6,  7, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  1, -1, -1,
      -1, -1,  5, -1,  3,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingspgdmdccw = {spgdmdccw, 3,
   { -5,   5,   6,   2,   5,  -5,  -6,  -2},
   {  1,   9,   4,   4,  -1,  -9,  -4,  -4}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  1, -1, -1,
      -1, -1, -1, -1,  3,  2, -1, -1,
      -1, -1,  0, -1, -1, -1, -1, -1,
      -1, -1,  6,  7, -1,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  5, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingwqtag = {swqtag, 3,       // "fudged" coordinates
   { -4,   5,  10,   6,   2,   4,  -5, -10,  -6,  -2},
   {  5,   5,   0,   0,   0,  -5,  -5,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0, -1,  1, -1, -1,
      -1,  7,  8,  9,  4,  3,  2, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  6, -1, -1,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec nicethingwqtag = {swqtag, 3,   // "true" coordinates
   { -5,   5,  10,   6,   2,   5,  -5, -10,  -6,  -2},
   {  5,   5,   0,   0,   0,  -5,  -5,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1, -1,  1, -1, -1,
      -1,  7,  8,  9,  4,  3,  2, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  6, -1, -1,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thinghsqtag = {s_hsqtag, 3,
   { -5,  -5,  -5,  -5,   0,   2,   5,   5,   5,   5,   0,  -2},
   {  6,   2,  -2,  -6,   5,   0,  -6,  -2,   2,   6,  -5,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1,  4,  9, -1, -1,
      -1, -1,  1, 11,  5,  8, -1, -1,
      -1, -1,  2, -1, -1,  7, -1, -1,
      -1, -1,  3, -1, 10,  6, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingdmdlndmd = {s_dmdlndmd, 3,
   { -7,   0,   0,   7,   9,   5,   7,   0,   0,  -7,  -9,  -5},
   {  5,   6,   2,   5,   0,   0,  -5,  -6,  -2,  -5,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1,  1,  3, -1, -1,
      -1, 10, 11, -1,  2,  5,  4, -1,
      -1, -1, -1, -1,  8, -1, -1, -1,
      -1, -1,  9, -1,  7,  6, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thinghqtag = {s_hqtag, 3,       // "fudged" coordinates
   { -9,  -9,  -9,  -9,  -4,   5,   6,   2,   9,   9,   9,   9,   4,  -5,  -6,  -2},
   {  6,   2,  -2,  -6,   5,   5,   0,   0,  -6,  -2,   2,   6,  -5,  -5,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0, -1,  4, -1,  5, 11, -1,
      -1,  1, 14, 15,  7,  6, 10, -1,
      -1,  2, -1, -1, -1, -1,  9, -1,
      -1,  3, 13, -1, -1, 12,  8, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec nicethinghqtag = {s_hqtag, 3,   // "true" coordinates
   { -9,  -9,  -9,  -9,  -5,   5,   6,   2,   9,   9,   9,   9,   5,  -5,  -6,  -2},
   {  6,   2,  -2,  -6,   5,   5,   0,   0,  -6,  -2,   2,   6,  -5,  -5,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  4, -1, -1,  5, 11, -1,
      -1,  1, 14, 15,  7,  6, 10, -1,
      -1,  2, -1, -1, -1, -1,  9, -1,
      -1,  3, 13, -1, -1, 12,  8, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing2stars = {s_2stars, 3,       /* "fudged" coordinates */
   { -4,   5,   7,   2,   4,  -5,  -7,  -2},
   {  5,   5,   0,   0,  -5,  -5,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0, -1,  1, -1, -1,
      -1, -1,  6,  7,  3,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  5, -1, -1,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec nicething2stars = {s_2stars, 3,   /* "true" coordinates */
   { -5,   5,   7,   2,   5,  -5,  -7,  -2},
   {  5,   5,   0,   0,  -5,  -5,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1, -1,  1, -1, -1,
      -1, -1,  6,  7,  3,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  5, -1, -1,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingptpd = {s_ptpd, 3,       /* "fudged" coordinates */
   { -9,  -5,  -2,  -5,   9,   5,   2,   5},
   {  0,   2,   0,  -2,   0,  -2,   0,   2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1,  2,  6,  7,  4, -1,
      -1, -1,  3, -1, -1,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec nicethingptpd = {s_ptpd, 3,       /* "true" coordinates */
   {-10,  -6,  -2,  -6,  10,   6,   2,   6},
   {  0,   2,   0,  -2,   0,  -2,   0,   2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1,  2,  6,  7,  4, -1,
      -1, -1,  3, -1, -1,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingbigdmd = {sbigdmd, 3,   /* used for both --
                                                symmetric and safe for press/truck */
   { -9,  -5,   0,   0,   5,   9,   9,   5,   0,   0,  -5,  -9},
   {  2,   2,   6,   2,   2,   2,  -2,  -2,  -6,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  2, -1, -1, -1,
      -1,  0,  1, -1,  3,  4,  5, -1,
      -1, 11, 10, -1,  9,  7,  6, -1,
      -1, -1, -1, -1,  8, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingbigptpd = {sbigptpd, 3,
   { -6,  -6, -10,  -2,  -6,  -6,   6,   6,  10,   2,   6,   6},
   { -6,  -2,   0,   0,   2,   6,   6,   2,   0,   0,  -2,  -6}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  5, -1, -1,  6, -1, -1,
      -1,  2,  4,  3,  9,  7,  8, -1,
      -1, -1,  1, -1, -1, 10, -1, -1,
      -1, -1,  0, -1, -1, 11, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x2dmd = {s_1x2dmd, 3,   /* used for both --
                                                symmetric and safe for press/truck */
   { -9,  -5,   0,   9,   5,   0},
   {  0,   0,   2,   0,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1, -1,  2,  4,  3, -1,
      -1, -1, -1, -1,  5, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x3p1dmd = {s1x3p1dmd, 3,     // Used for both.
   { -9,  -5,  -1,   4,   9,   4},
   {  0,   0,   0,   2,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1,  2, -1,  3,  4, -1,
      -1, -1, -1, -1, -1,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x3dmd = {s1x3dmd, 3,   // Used for both -- symmetric and safe for press/truck.
   {-13,  -9,  -5,   0,  13,   9,   5,   0},
   {  0,   0,   0,   2,   0,   0,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
       0,  1,  2, -1,  3,  6,  5,  4,
      -1, -1, -1, -1,  7, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing3x1dmd = {s3x1dmd, 3,   // Used for both -- symmetric and safe for press/truck.
   {-10,  -6,  -2,   0,  10,   6,   2,   0},
   {  0,   0,   0,   5,   0,   0,   0,  -5}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  3, -1, -1, -1,
      -1,  0,  1,  2,  6,  5,  4, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  7, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingwingstar = {s_wingedstar, 3,   // Used for both -- symmetric and safe for press/truck.
   {-10,  -6,  -2,   0,  10,   6,   2,   0},
   {  0,   0,   0,   4,   0,   0,   0,  -4}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  3, -1, -1, -1,
      -1,  0,  1,  2,  6,  5,  4, -1,
      -1, -1, -1, -1,  7, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing4p2x1dmd = {s4p2x1dmd, 3,   // Used for both -- symmetric and safe for press/truck.
   {-10,  -6,  -2,   2,   4,  10,   6,   4},
   {  0,   0,   0,   0,   5,   0,   0,  -5}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  4, -1, -1,
      -1,  0,  1,  2,  3,  6,  5, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  7, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing3p1x1dmd = {s3p1x1dmd, 3,   // Used for both -- symmetric and safe for press/truck.
   { -6,  -2,   2,   4,   6,   4},
   {  0,   0,   0,   5,   0,  -5}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  3, -1, -1,
      -1, -1,  0,  1,  2,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing2x1dmd = {s_2x1dmd, 3,   // Used for both -- symmetric and safe for press/truck.
   { -6,  -2,   0,   6,   2,   0},
   {  0,   0,   5,   0,   0,  -5}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  2, -1, -1, -1,
      -1, -1,  0,  1,  4,  3, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  5, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingdeep2x1dmd = {sdeep2x1dmd, 3,   // used for both --
                                                     // symmetric and safe for press/truck
   { -6,  -2,   0,   2,   6,   6,   2,   0,  -2,  -6},
   {  2,   2,   7,   2,   2,  -2,  -2,  -7,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  2, -1, -1, -1,
      -1, -1,  0,  1,  3,  4, -1, -1,
      -1, -1,  9,  8,  6,  5, -1, -1,
      -1, -1, -1, -1,  7, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing3dmd = {s3dmd, 3,   /* used for both --
                                            symmetric and safe for press/truck */
   { -7,   0,   7,  10,   6,   2,   7,   0,  -7, -10,  -6,  -2},
   {  5,   5,   5,   0,   0,   0,  -5,  -5,  -5,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1,  1,  2, -1, -1,
      -1,  9, 10, 11,  5,  4,  3, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  8, -1,  7,  6, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing4dmd = {s4dmd, 3,   /* used for both --
                                            symmetric and safe for press/truck */
   {-11,  -5,   5,  11,  14,  10,   6,   2,  11,   5,  -5, -11, -14, -10,  -6,  -2},
   {  5,   5,   5,   5,   0,   0,   0,   0,  -5,  -5,  -5,  -5,   0,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1, -1, -1,  2,  3, -1,
      12, 13, 14, 15,  7,  6,  5,  4,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 11, 10, -1, -1,  9,  8, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing3mdmd = {s_3mdmd, 3,   /* used for both --
                                               symmetric and safe for press/truck */
   {-11,   0,  11,  13,   9,   5,  11,   0, -11, -13,  -9,  -5},
   {  5,   2,   5,   0,   0,   0,  -5,  -2,  -5,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0, -1, -1, -1, -1,  2, -1,
       9, 10, 11, -1,  1,  5,  4,  3,
      -1, -1, -1, -1,  7, -1, -1, -1,
      -1,  8, -1, -1, -1, -1,  6, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing3mptpd = {s_3mptpd, 3,   /* used for both --
                                                 symmetric and safe for press/truck */
   {-11,   0,  11,  14,   6,   2,  11,   0, -11, -14,  -6,  -2},
   {  2,   5,   2,   0,   0,   0,  -2,  -5,  -2,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  1,  2, -1, -1,
       9,  0, 10, 11,  5,  4,  2,  3,
      -1,  8, -1, -1, -1, -1,  6, -1,
      -1, -1, -1, -1,  7,  6, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingshort6 = {s_short6, 3,   /* used for both --
                                                 symmetric and safe for press/truck */
   { -2,   0,   2,   2,   0,  -2},
   {  2,   6,   2,  -2,  -6,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  1, -1, -1, -1,
      -1, -1, -1,  0,  2, -1, -1, -1,
      -1, -1, -1,  5,  3, -1, -1, -1,
      -1, -1, -1, -1,  4, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec nicethingphan = {s_c1phan, 3,   /* "true" coordinates for mirroring --
                                   actually, the truck distances are greatly exaggerated. */
   {-10,  -6,  -2,  -6,   6,  10,   6,   2,  10,   6,   2,   6,  -6, -10,  -6,  -2},
   {  6,  10,   6,   2,  10,   6,   2,   6,  -6, -10,  -6,  -2, -10,  -6,  -2,  -6}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  1, -1, -1,  4, -1, -1,
      -1,  0, -1,  2,  7, -1,  5, -1,
      -1, -1,  3, -1, -1,  6, -1, -1,
      -1, -1, 14, -1, -1, 11, -1, -1,
      -1, 13, -1, 15, 10, -1,  8, -1,
      -1, -1, 12, -1, -1,  9, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingphan = {s_c1phan, 3,   /* "fudged" coordinates -- only truck within each 1x2 is allowed */
   { -6,  -3,  -2,  -3,   4,  15,   4,  11,   7,   4,   3,   4,  -5, -14,  -5, -10},
   {  4,  13,   4,   9,   7,   4,   3,   4,  -5, -14,  -5, -10,  -6,  -7,  -2,  -7}, {
      -1, -1, -1,  1, -1, -1, -1, -1,
      -1, -1, -1,  3, -1, -1, -1, -1,
      -1, -1,  0,  2, -1,  4,  7,  5,
      -1, -1, -1, -1, -1,  6, -1, -1,
      -1, -1, 14, -1, -1, -1, -1, -1,
      13, 15, 12, -1, 10,  8, -1, -1,
      -1, -1, -1, -1, -1, 11, -1, -1,
      -1, -1, -1, -1, -1,  9, -1, -1}};

static const coordrec nicethingglass = {s_hrglass, 3,   // "True" coordinates for mirroring --
                                        // actually, the truck distances are greatly exaggerated.
   { -5,   5,   6,   0,   5,  -5,  -6,   0},
   {  5,   5,   0,   2,  -5,  -5,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1, -1,  1, -1, -1,
      -1, -1,  6, -1,  3,  2, -1, -1,
      -1, -1, -1, -1,  7, -1, -1, -1,
      -1, -1,  5, -1, -1,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingglass = {s_hrglass, 3,   // "Fudged" coordinates -- only truck by center 2 is allowed.
   { -4,   5,   5,   0,   4,  -5,  -5,   0},
   {  5,   5,   0,   2,  -5,  -5,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0, -1,  1, -1, -1,
      -1, -1,  6, -1,  3,  2, -1, -1,
      -1, -1, -1, -1,  7, -1, -1, -1,
      -1, -1,  5, -1, -1,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingdglass = {s_dhrglass, 3,   // Used for both -- symmetric and safe for press/truck.
   {-10,  10,   5,   0,  10, -10,  -5,   0},
   {  2,   2,   0,   2,  -2,  -2,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  6, -1,  3,  2,  1, -1,
      -1,  5, -1, -1,  7, -1,  4, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingbigdglass = {sbigdhrgl, 3,   // Used for both -- symmetric and safe for press/truck.
   {-14, -10,   0,   5,  10,  14,  14,  10,  0,  -5,  -10, -14},
   {  2,   2,   2,   0,   2,   2,  -2,  -2, -2,   0,   -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
       0,  1,  9, -1,  2,  3,  4,  5,
      11, 10, -1, -1,  8, -1,  7,  6,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec nicethingbigglass = {sbighrgl, 3,   // "True" coordinates for mirroring --
                                           // actually, the truck distances are greatly exaggerated.
   { -9,  -5,   0,   2,   5,   9,   9,   5,   0,  -2,  -5,  -9},
   {  5,   5,   5,   0,   5,   5,  -5,  -5,  -5,   0,  -5,  -5}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1, -1,  2,  4,  5, -1,
      -1, -1, -1,  9,  3, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 11, 10, -1,  8,  7,  6, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingbigglass = {sbighrgl, 3,   // "Fudged" coordinates -- only a few trucks are allowed.
   { -9,  -5,   0,   2,   5,   9,   9,   5,   0,  -2,  -5,  -9},
   {  4,   4,   5,   0,   5,   5,  -4,  -4,  -5,   0,  -5,  -5}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1, -1,  2,  4,  5, -1,
      -1, -1, -1,  9,  3, -1, -1, -1,
      -1, -1, -1, -1, -1,  7,  6, -1,
      -1, 11, 10, -1,  8, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec nicethingwglass = {swhrglass, 3,   /* "true" coordinates for mirroring --
                                   actually, the truck distances are greatly exaggerated. */
   { -5,   5,   9,   5,   0,   5,  -5,  -9,  -5,   0},
   {  5,   5,   0,   0,   2,  -5,  -5,   0,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1, -1,  1, -1, -1,
      -1,  7,  8, -1,  4,  3,  2, -1,
      -1, -1, -1, -1,  9, -1, -1, -1,
      -1, -1,  6, -1, -1,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingwglass = {swhrglass, 3,   /* "fudged" coordinates -- only truck by center 2 is allowed */
   { -4,   5,   9,   5,   0,   4,  -5,  -9,  -5,   0},
   {  5,   5,   0,   0,   2,  -5,  -5,   0,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0, -1,  1, -1, -1,
      -1,  7,  8, -1,  4,  3,  2, -1,
      -1, -1, -1, -1,  9, -1, -1, -1,
      -1, -1,  6, -1, -1,  5, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing4x4 = {s4x4, 3,
   {  6,   6,   6,   2,   6,   2,  -2,   2,  -6,  -6,  -6,  -2,  -6,  -2,   2,  -2},
   {  6,   2,  -2,   2,  -6,  -6,  -6,  -2,  -6,  -2,   2,  -2,   6,   6,   6,   2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, 12, 13, 14,  0, -1, -1,
      -1, -1, 10, 15,  3,  1, -1, -1,
      -1, -1,  9, 11,  7,  2, -1, -1,
      -1, -1,  8,  6,  5,  4, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing4x5 = {s4x5, 3,
   { -8,  -4,   0,   4,   8,   8,   4,   0,  -4,  -8,   8,   4,   0,  -4,  -8,  -8,  -4,   0,   4,   8},
   {  6,   6,   6,   6,   6,   2,   2,   2,   2,   2,  -6,  -6,  -6,  -6,  -6,  -2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  2,  3,  4, -1,
      -1, -1,  9,  8,  7,  6,  5, -1,
      -1, -1, 15, 16, 17, 18, 19, -1,
      -1, -1, 14, 13, 12, 11, 10, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing23232 = {s_23232, 3,
   { -4,   4,  -8,   0,   8,   4,   4,  -4,   8,   0,  -8,  -4},
   {  4,   4,   2,   2,   2,   0,  -4,  -4,  -2,  -2,  -2,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0, -1,  1, -1, -1,
      -1, -1,  2, 11,  3,  5,  4, -1,
      -1, -1, 10,  7,  9,  6,  8, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing4x6 = {s4x6, 3,
   {-10,  -6,  -2,   2,   6,  10,  10,   6,   2,  -2,  -6, -10,  10,   6,   2,  -2,  -6, -10, -10,  -6,  -2,   2,   6,  10},
   {  6,   6,   6,   6,   6,   6,   2,   2,   2,   2,   2,   2,  -6,  -6,  -6,  -6,  -6,  -6,  -2,  -2,  -2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  1,  2,  3,  4,  5, -1,
      -1, 11, 10,  9,  8,  7,  6, -1,
      -1, 18, 19, 20, 21, 22, 23, -1,
      -1, 17, 16, 15, 14, 13, 12, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing3oqtg = {s3oqtg, 3,
   { -8,   0,   8,  12,   8,   4,   0,  12,   8,   4,   8,   0,  -8, -12,  -8,  -4,   0, -12,  -8,  -4},
   {  6,   6,   6,   2,   2,   2,   2,  -2,  -2,  -2,  -6,  -6,  -6,  -2,  -2,  -2,  -2,   2,   2,   2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0, -1,  1, -1,  2, -1,
      -1, 17, 18, 19,  6,  5,  4,  3,
      -1, 13, 14, 15, 16,  9,  8,  7,
      -1, -1, 12, -1, 11, -1, 10, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingbigh = {sbigh, 3,
   { -9,  -9,  -9,  -9,  -6,  -2,   9,   9,   9,   9,   6,   2},
   {  6,   2,  -2,  -6,   0,   0,  -6,  -2,   2,   6,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0, -1, -1, -1, -1,  9, -1,
      -1,  1,  4,  5, 11, 10,  8, -1,
      -1,  2, -1, -1, -1, -1,  7, -1,
      -1,  3, -1, -1, -1, -1,  6, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingbigbigx = {sbigbigx, 4,
   {-19, -15, -11,  -7,  -2,  -2,  -2,  -2,  19,  15,  11,   7,   2,   2,   2,   2},
   {  0,   0,   0,   0,   6,   2,  -2,  -6,   0,   0,   0,   0,  -6,  -2,   2,   6}, {
      -1, -1, -1, -1, -1, -1, -1,  4, 15, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2,  3,  5, 14, 11, 10,  9,  8, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1,  6, 13, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1,  7, 12, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingbigbigh = {sbigbigh, 4,
   {-17, -17, -17, -17, -14, -10,  -6,  -2,  17,  17,  17,  17,  14,  10,   6,   2},
   {  6,   2,  -2,  -6,   0,   0,   0,   0,  -6,  -2,   2,   6,   0,   0,   0,   0}, {
      -1, -1, -1,  0, -1, -1, -1, -1, -1, -1, -1, -1, 11, -1, -1, -1,
      -1, -1, -1,  1,  4,  5,  6,  7, 15, 14, 13, 12, 10, -1, -1, -1,
      -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1,  9, -1, -1, -1,
      -1, -1, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1,  8, -1, -1, -1}};


static const coordrec thingbigbone = {sbigbone, 3,
   {-14, -10,  -6,  -2,  10,  14,  14,  10,   6,   2, -10, -14},
   {  2,   2,   0,   0,   2,   2,  -2,  -2,   0,   0,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
       0,  1,  2,  3,  9,  8,  4,  5,
      11, 10, -1, -1, -1, -1,  7,  6,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingbigrig = {sbigrig, 4,
   {-18, -14, -10,  -6,  -2,   2,  18,  14,  10,   6,   2,  -2},
   {  0,   0,   0,   0,   2,   2,   0,   0,   0,   0,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2,  3,  4,  5,  9,  8,  7,  6, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 11, 10, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing5x1dmd = {s5x1dmd, 4,
   {-18, -14, -10,  -6,  -2,   0,  18,  14,  10,   6,   2,   0},
   {  0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,  -5}, {
      -1, -1, -1, -1, -1, -1, -1, -1,  5, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2,  3,  4, 10,  9,  8,  7,  6, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 11, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x5dmd = {s1x5dmd, 4,
   {-21, -17, -13,  -9,  -5,   0,  21,  17,  13,   9,   5,   0},
   {  0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  0,  1,  2,  3,  4, -1,  5, 10,  9,  8,  7,  6, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 11, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x4dmd = {s1x4dmd, 4,
   {-17, -13,  -9,  -5,   0,  17,  13,   9,   5,   0},
   {  0,   0,   0,   0,   2,   0,   0,   0,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2,  3, -1,  4,  8,  7,  6,  5, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,  9, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thing1x4p2dmd = {s1x4p2dmd, 4,
   {-13,  -9,  -5,  -1,  4,  13,   9,   4},
   {  0,   0,   0,   0,  2,   0,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  0,  1,  2,  3, -1,  4,  6,  5, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1,  7, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thinglinepdmd = {slinepdmd, 3,
   {  0,  -2,   0,   2, -6,  -2,   6,   2},
   { -6,  -2,   2,  -2,  6,   6,   6,   6}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  4,  5,  7,  6, -1, -1,
      -1, -1, -1, -1,  2, -1, -1, -1,
      -1, -1, -1,  1,  3, -1, -1, -1,
      -1, -1, -1, -1,  0, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingdmdpdmd = {sdmdpdmd, 3,
   {  0,  -2,   0,   2, -4,   0,   4,   0},
   { -8,  -4,   0,  -4,  6,   8,   6,   4}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  5, -1, -1, -1,
      -1, -1, -1,  4,  7,  6, -1, -1,
      -1, -1, -1, -1,  2, -1, -1, -1,
      -1, -1, -1,  1,  3, -1, -1, -1,
      -1, -1, -1, -1,  0, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingplinepdmd = {splinepdmd, 4,
   {-13,  -9,  -5,  -1,  3,   8,  13,   8},
   {  0,   0,   0,   0,  0,   2,   0,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  0,  1,  2,  3,  4, -1,  5,  6, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  7, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingplinedmd = {splinedmd, 3,
   {-10,  -6,  -2,   2,   6,   8,  10,   8},
   {  0,   0,   0,   0,   0,   5,   0,  -5}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1,  5, -1,
      -1,  0,  1,  2,  3,  4,  6, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1,  7, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thinglinedmd = {slinedmd, 3,
   { -5,   0,   5,   0,  -6,  -2,   6,   2},
   { -2,   0,  -2,  -4,   4,   4,   4,   4}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  4,  5,  7,  6, -1, -1,
      -1, -1, -1, -1,  1, -1, -1, -1,
      -1, -1,  0, -1,  3,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thinglinebox = {slinebox, 4,
   {-10,  -6,  -2,   2,   6,  10,  10,   6},
   {  0,   0,   0,   0,   2,   2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1,  7,  6, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingline2box = {sline2box, 4,
   { -8,  -4,   0,   4,   8,   8,   4,   0},
   {  0,   0,   2,   2,   2,  -2,  -2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1,  0,  1,  2,  3,  4, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,  7,  6,  5, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingline6box = {sline6box, 4,
   {-12,  -8,  -4,   0,   4,   8,  12,  12},
   {  0,   0,   0,   0,   0,   0,   2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  7, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingdbltrngl4 = {sdbltrngl4, 4,
   {-10,  -6,  -2,  -2,   2,   6,  10,  10},
   {  0,   0,   2,  -2,   0,   0,   2,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  0,  1,  2,  4,  5,  6, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1,  3, -1, -1,  7, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingboxdmd = {sboxdmd, 3,
   { -5,   0,   5,   0,  -2,   2,   2,  -2},
   { -4,  -2,  -4,  -6,   6,   6,   2,   2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  4,  5, -1, -1, -1,
      -1, -1, -1,  7,  6, -1, -1, -1,
      -1, -1,  0, -1,  1,  2, -1, -1,
      -1, -1, -1, -1,  3, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingboxpdmd = {sboxpdmd, 3,
   {  0,  -2,   0,   2,  -2,   2,   2,  -2},
   { -8,  -4,   0,  -4,   8,   8,   4,   4}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  4,  5, -1, -1, -1,
      -1, -1, -1,  7,  6, -1, -1, -1,
      -1, -1, -1, -1,  2, -1, -1, -1,
      -1, -1, -1,  1,  3, -1, -1, -1,
      -1, -1, -1, -1,  0, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingtrngl8 = {s_trngl8, 3,
   {  0,   0,   0,   0,  -6,  -2,   2,   6},
   { -8,  -4,   0,   4,   9,   9,   9,   9}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1,  4,  5,  6,  7, -1, -1,
      -1, -1, -1, -1,  3, -1, -1, -1,
      -1, -1, -1, -1,  2, -1, -1, -1,
      -1, -1, -1, -1,  1, -1, -1, -1,
      -1, -1, -1, -1,  0, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingbigx = {sbigx, 4,
   {-17, -13,  -9,  -5,   0,   0,  17,  13,   9,   5,   0,   0},
   {  0,   0,   0,   0,   6,   2,   0,   0,   0,   0,  -6,  -2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,  4, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1,  2,  3, -1,  5,  9,  8,  7,  6, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 11, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 10, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingwiderigger = {swiderigger, 4,
   { -6,  -2,   2,   6,  14,  10,   6,   2,  -2,  -6, -14, -10},
   {  2,   2,   2,   2,   0,   0,  -2,  -2,  -2,  -2,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 10, 11,  0,  1,  2,  3,  5,  4, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1,  9,  8,  7,  6, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingrigger12 = {srigger12, 4,
   { -4,   0,   4,  16,  12,   8,   4,   0,  -4, -16, -12,  -8},
   {  2,   2,   2,   0,   0,   0,  -2,  -2,  -2,   0,   0,   0}, {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,  9, 10, 11,  0,  1,  2,  5,  4,  3, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1,  8,  7,  6, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingdeepxwv = {sdeepxwv, 4,
   {-10,  -6,  -2,  -2,  -2,  -2,  10,   6,   2,   2,   2,   2},
   {  0,   0,   6,   2,  -2,  -6,   0,   0,  -6,  -2,   2,   6}, {
      -1, -1, -1, -1, -1, -1, -1,  2, 11, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  0,  1,  3, 10,  7,  6, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1,  4,  9, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1,  5,  8, -1, -1, -1, -1, -1, -1, -1}};

static const coordrec thingblob = {s_bigblob, 3,
   { -2,   2,   2,   6,   6,   2,  10,  10,   6,   6,   2,   2,   2,  -2,  -2,  -6,  -6,  -2, -10, -10,  -6,  -6,  -2,  -2},
   { 10,  10,   6,   6,   2,   2,   2,  -2,  -2,  -6,  -6,  -2, -10, -10,  -6,  -6,  -2,  -2,  -2,   2,   2,   6,   6,   2}, {
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  0,  1, -1, -1, -1,
      -1, -1, 21, 22,  2,  3, -1, -1,
      -1, 19, 20, 23,  5,  4,  6, -1,
      -1, 18, 16, 17, 11,  8,  7, -1,
      -1, -1, 15, 14, 10,  9, -1, -1,
      -1, -1, -1, 13, 12, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1}};



#define NOBIT(otherbits) { otherbits, otherbits, otherbits, otherbits }

#define WESTBIT(otherbits) { ID2_BEAU | otherbits, ID2_TRAILER | otherbits, ID2_BELLE | otherbits, ID2_LEAD | otherbits }

#define EASTBIT(otherbits) { ID2_BELLE | otherbits, ID2_LEAD | otherbits, ID2_BEAU | otherbits, ID2_TRAILER | otherbits }

#define NORTHBIT(otherbits) { ID2_LEAD | otherbits, ID2_BEAU | otherbits, ID2_TRAILER | otherbits, ID2_BELLE | otherbits }

#define SOUTHBIT(otherbits) { ID2_TRAILER | otherbits, ID2_BELLE | otherbits, ID2_LEAD | otherbits, ID2_BEAU | otherbits }

#define NWBITS(otherbits) { ID2_LEAD   |ID2_BEAU  | otherbits, ID2_TRAILER|ID2_BEAU  | otherbits, ID2_TRAILER|ID2_BELLE | otherbits, ID2_LEAD   |ID2_BELLE | otherbits }

#define SWBITS(otherbits) { ID2_TRAILER|ID2_BEAU  | otherbits, ID2_TRAILER|ID2_BELLE | otherbits, ID2_LEAD   |ID2_BELLE | otherbits, ID2_LEAD   |ID2_BEAU  | otherbits }

#define SEBITS(otherbits) { ID2_TRAILER|ID2_BELLE | otherbits, ID2_LEAD   |ID2_BELLE | otherbits, ID2_LEAD   |ID2_BEAU  | otherbits, ID2_TRAILER|ID2_BEAU  | otherbits }

#define NEBITS(otherbits) { ID2_LEAD   |ID2_BELLE | otherbits, ID2_LEAD   |ID2_BEAU  | otherbits, ID2_TRAILER|ID2_BEAU  | otherbits, ID2_TRAILER|ID2_BELLE | otherbits }



static const id_bit_table id_bit_table_1x2[] = {
   WESTBIT(0),
   EASTBIT(0)};

static const id_bit_table id_bit_table_2x2[] = {
   NWBITS(0),
   NEBITS(0),
   SEBITS(0),
   SWBITS(0)};

static const id_bit_table id_bit_table_thar[] = {
   WESTBIT( ID2_END),
   EASTBIT( ID2_CENTER),
   NORTHBIT(ID2_END),
   SOUTHBIT(ID2_CENTER),
   EASTBIT( ID2_END),
   WESTBIT( ID2_CENTER),
   SOUTHBIT(ID2_END),
   NORTHBIT(ID2_CENTER)};

static const id_bit_table id_bit_table_2x4[] = {
   NWBITS(ID2_END),
   NEBITS(ID2_CENTER|ID2_CTR4),
   NWBITS(ID2_CENTER|ID2_CTR4),
   NEBITS(ID2_END),
   SEBITS(ID2_END),
   SWBITS(ID2_CENTER|ID2_CTR4),
   SEBITS(ID2_CENTER|ID2_CTR4),
   SWBITS(ID2_END)};

static const id_bit_table id_bit_table_2x5[] = {
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0)};

// This table is only accepted if the population is a "Z" between pairs.
id_bit_table id_bit_table_2x5_z[] = {
   NORTHBIT(ID2_OUTRPAIRS | ID2_OUTR6 | ID2_END),
   NORTHBIT(ID2_CTR4      | ID2_OUTR6 | ID2_CENTER),
   NORTHBIT(ID2_CTR4      | ID2_CTR2  | ID2_CENTER),
   NORTHBIT(ID2_CTR4      | ID2_OUTR6 | ID2_CENTER),
   NORTHBIT(ID2_OUTRPAIRS | ID2_OUTR6 | ID2_END),
   SOUTHBIT(ID2_OUTRPAIRS | ID2_OUTR6 | ID2_END),
   SOUTHBIT(ID2_CTR4      | ID2_OUTR6 | ID2_CENTER),
   SOUTHBIT(ID2_CTR4      | ID2_CTR2  | ID2_CENTER),
   SOUTHBIT(ID2_CTR4      | ID2_OUTR6 | ID2_CENTER),
   SOUTHBIT(ID2_OUTRPAIRS | ID2_OUTR6 | ID2_END)};

// This table is only accepted if the center 2x3 is fully occupied.
id_bit_table id_bit_table_2x5_ctr6[] = {
   NORTHBIT(ID2_OUTR2 | ID2_OUTR6),
   NORTHBIT(ID2_CTR6  | ID2_OUTR6),
   NORTHBIT(ID2_CTR6  | ID2_CTR2),
   NORTHBIT(ID2_CTR6  | ID2_OUTR6),
   NORTHBIT(ID2_OUTR2 | ID2_OUTR6),
   SOUTHBIT(ID2_OUTR2 | ID2_OUTR6),
   SOUTHBIT(ID2_CTR6  | ID2_OUTR6),
   SOUTHBIT(ID2_CTR6  | ID2_CTR2),
   SOUTHBIT(ID2_CTR6  | ID2_OUTR6),
   SOUTHBIT(ID2_OUTR2 | ID2_OUTR6)};

static const id_bit_table id_bit_table_2x7[] = {
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(ID2_CTR2),
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(ID2_CTR2),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0)};

static const id_bit_table id_bit_table_2x9[] = {
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(ID2_CTR2),
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(ID2_CTR2),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0)};

static const id_bit_table id_bit_table_dblspindle[] = {
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(ID2_CTR2),
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(0),
   NORTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(ID2_CTR2),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0),
   SOUTHBIT(0)};

/* This table is accepted in any circumstances. */
/* Note that the people marked "ID2_END" won't actually identify
   themselves as ends.  That is just to make "center 4" work. */
static const id_bit_table id_bit_table_2x6[] = {
   NORTHBIT(ID2_END),
   NORTHBIT(ID2_END),
   NORTHBIT(ID2_CTR4),
   NORTHBIT(ID2_CTR4),
   NORTHBIT(ID2_END),
   NORTHBIT(ID2_END),
   SOUTHBIT(ID2_END),
   SOUTHBIT(ID2_END),
   SOUTHBIT(ID2_CTR4),
   SOUTHBIT(ID2_CTR4),
   SOUTHBIT(ID2_END),
   SOUTHBIT(ID2_END)};

/* This table is only accepted if the population is a parallelogram. */
id_bit_table id_bit_table_2x6_pg[] = {
   NWBITS(ID2_OUTRPAIRS),
   NEBITS(ID2_OUTRPAIRS),
   NWBITS(ID2_CTR4),
   NEBITS(ID2_CTR4),
   NWBITS(ID2_OUTRPAIRS),
   NEBITS(ID2_OUTRPAIRS),
   SEBITS(ID2_OUTRPAIRS),
   SWBITS(ID2_OUTRPAIRS),
   SEBITS(ID2_CTR4),
   SWBITS(ID2_CTR4),
   SEBITS(ID2_OUTRPAIRS),
   SWBITS(ID2_OUTRPAIRS)};

static const id_bit_table id_bit_table_2x8[] = {
   NORTHBIT(ID2_END),
   NORTHBIT(ID2_END),
   NORTHBIT(ID2_END),
   NORTHBIT(ID2_CTR4),
   NORTHBIT(ID2_CTR4),
   NORTHBIT(ID2_END),
   NORTHBIT(ID2_END),
   NORTHBIT(ID2_END),
   SOUTHBIT(ID2_END),
   SOUTHBIT(ID2_END),
   SOUTHBIT(ID2_END),
   SOUTHBIT(ID2_CTR4),
   SOUTHBIT(ID2_CTR4),
   SOUTHBIT(ID2_END),
   SOUTHBIT(ID2_END),
   SOUTHBIT(ID2_END)};

static const id_bit_table id_bit_table_1x4[] = {
   WESTBIT(ID2_END),
   EASTBIT(ID2_CENTER),
   EASTBIT(ID2_END),
   WESTBIT(ID2_CENTER)};

static const id_bit_table id_bit_table_dmd[] = {
   NOBIT(ID2_END),
   NORTHBIT(ID2_CENTER),
   NOBIT(ID2_END),
   SOUTHBIT(ID2_CENTER)};

// This table won't be used unless the actual setup is
// a center "Z" between outer pairs.
static const id_bit_table id_bit_table_d2x5[] = {
   NORTHBIT(ID2_OUTRPAIRS | ID2_END),
   SOUTHBIT(ID2_OUTRPAIRS | ID2_END),
   NOBIT(   ID2_CTR4      | ID2_CENTER),
   NOBIT(   ID2_CTR4      | ID2_CENTER),
   NOBIT(   ID2_CTR4      | ID2_CENTER),
   SOUTHBIT(ID2_OUTRPAIRS | ID2_END),
   NORTHBIT(ID2_OUTRPAIRS | ID2_END),
   NOBIT(   ID2_CTR4      | ID2_CENTER),
   NOBIT(   ID2_CTR4      | ID2_CENTER),
   NOBIT(   ID2_CTR4      | ID2_CENTER)};

// This table is used only if the center 6 are fully occupied.
static const id_bit_table id_bit_table_d2x7[] = {
   NOBIT(ID2_OUTR2),
   NOBIT(ID2_OUTR2),
   WESTBIT(ID2_CTR6),
   WESTBIT(ID2_CTR6),
   WESTBIT(ID2_CTR6),
   NOBIT(ID2_OUTR2),
   NOBIT(ID2_OUTR2),
   NOBIT(ID2_OUTR2),
   NOBIT(ID2_OUTR2),
   EASTBIT(ID2_CTR6),
   EASTBIT(ID2_CTR6),
   EASTBIT(ID2_CTR6),
   NOBIT(ID2_OUTR2),
   NOBIT(ID2_OUTR2)};

// These 2 tables will be used if the actual setup is a center "Z" between outer pairs.
// There are 4 cases that we are interested in, that require 2 tables.
id_bit_table id_bit_table_d2x7a[] = {
   WESTBIT(ID2_OUTRPAIRS | ID2_END),
   EASTBIT(ID2_OUTRPAIRS | ID2_END),
   NORTHBIT(ID2_CTR4     | ID2_CENTER),
   NORTHBIT(ID2_CTR4     | ID2_CENTER),
   SOUTHBIT(ID2_CTR4     | ID2_CENTER),
   WESTBIT(ID2_OUTRPAIRS | ID2_END),
   EASTBIT(ID2_OUTRPAIRS | ID2_END),
   EASTBIT(ID2_OUTRPAIRS | ID2_END),
   WESTBIT(ID2_OUTRPAIRS | ID2_END),
   SOUTHBIT(ID2_CTR4     | ID2_CENTER),
   SOUTHBIT(ID2_CTR4     | ID2_CENTER),
   NORTHBIT(ID2_CTR4     | ID2_CENTER),
   EASTBIT(ID2_OUTRPAIRS | ID2_END),
   WESTBIT(ID2_OUTRPAIRS | ID2_END)};

id_bit_table id_bit_table_d2x7b[] = {
   WESTBIT(ID2_OUTRPAIRS | ID2_END),
   EASTBIT(ID2_OUTRPAIRS | ID2_END),
   NORTHBIT(ID2_CTR4     | ID2_CENTER),
   SOUTHBIT(ID2_CTR4     | ID2_CENTER),
   SOUTHBIT(ID2_CTR4     | ID2_CENTER),
   WESTBIT(ID2_OUTRPAIRS | ID2_END),
   EASTBIT(ID2_OUTRPAIRS | ID2_END),
   EASTBIT(ID2_OUTRPAIRS | ID2_END),
   WESTBIT(ID2_OUTRPAIRS | ID2_END),
   SOUTHBIT(ID2_CTR4     | ID2_CENTER),
   NORTHBIT(ID2_CTR4     | ID2_CENTER),
   NORTHBIT(ID2_CTR4     | ID2_CENTER),
   EASTBIT(ID2_OUTRPAIRS | ID2_END),
   WESTBIT(ID2_OUTRPAIRS | ID2_END)};

// This one will be used if the center 2x3 is fully occupied.
id_bit_table id_bit_table_d2x5_ctr6[] = {
   NORTHBIT(ID2_OUTR2 | ID2_OUTR6),
   SOUTHBIT(ID2_OUTR2 | ID2_OUTR6),
   NOBIT   (ID2_CTR6  | ID2_OUTR6),
   NOBIT   (ID2_CTR6  | ID2_CTR2),
   NOBIT   (ID2_CTR6  | ID2_OUTR6),
   SOUTHBIT(ID2_OUTR2 | ID2_OUTR6),
   NORTHBIT(ID2_OUTR2 | ID2_OUTR6),
   NOBIT   (ID2_CTR6  | ID2_OUTR6),
   NOBIT   (ID2_CTR6  | ID2_CTR2),
   NOBIT   (ID2_CTR6  | ID2_OUTR6)};

static const id_bit_table id_bit_table_bone6[] = {
   NORTHBIT(0),
   NORTHBIT(0),
   /* We mark triangle points as trailers, in case we need to identify them in a 2x2 box,
       as in "leads start wheel the ocean". */
   {ID2_BEAU|ID2_TRAILER,     ID2_TRAILER,             ID2_BELLE|ID2_TRAILER,  ID2_LEAD},
   SOUTHBIT(0),
   SOUTHBIT(0),
   {ID2_BELLE|ID2_TRAILER,    ID2_LEAD,                ID2_BEAU|ID2_TRAILER,   ID2_TRAILER}};

static const id_bit_table id_bit_table_short6[] = {
   NWBITS(ID2_CENTER),
   NOBIT(ID2_END),
   NEBITS(ID2_CENTER),
   SEBITS(ID2_CENTER),
   NOBIT(ID2_END),
   SWBITS(ID2_CENTER)};

static const id_bit_table id_bit_table_1x6[] = {
   WESTBIT(ID2_OUTRPAIRS| ID2_OUTR2| ID2_NCTR1X4),
   EASTBIT(ID2_OUTRPAIRS| ID2_CTR4 | ID2_CTR1X4),
   WESTBIT(ID2_CTR2     | ID2_CTR4 | ID2_CTR1X4),
   EASTBIT(ID2_OUTRPAIRS| ID2_OUTR2| ID2_NCTR1X4),
   WESTBIT(ID2_OUTRPAIRS| ID2_CTR4 | ID2_CTR1X4),
   EASTBIT(ID2_CTR2     | ID2_CTR4 | ID2_CTR1X4)};

static const id_bit_table id_bit_table_2x3[] = {
   NORTHBIT(ID2_OUTRPAIRS),
   NORTHBIT(ID2_CTR2),
   NORTHBIT(ID2_OUTRPAIRS),
   SOUTHBIT(ID2_OUTRPAIRS),
   SOUTHBIT(ID2_CTR2),
   SOUTHBIT(ID2_OUTRPAIRS)};

static const id_bit_table id_bit_table_1x8[] = {
   WESTBIT(ID2_OUTR6| ID2_OUTR2| ID2_NCTR1X6| ID2_NCTR1X4| ID2_OUTRPAIRS),
   EASTBIT(ID2_OUTR6| ID2_CTR6 | ID2_CTR1X6 | ID2_NCTR1X4| ID2_OUTRPAIRS),
   EASTBIT(ID2_CTR2 | ID2_CTR6 | ID2_CTR1X6 | ID2_CTR1X4 | ID2_CTR4),
   WESTBIT(ID2_OUTR6| ID2_CTR6 | ID2_CTR1X6 | ID2_CTR1X4 | ID2_CTR4),
   EASTBIT(ID2_OUTR6| ID2_OUTR2| ID2_NCTR1X6| ID2_NCTR1X4| ID2_OUTRPAIRS),
   WESTBIT(ID2_OUTR6| ID2_CTR6 | ID2_CTR1X6 | ID2_NCTR1X4| ID2_OUTRPAIRS),
   WESTBIT(ID2_CTR2 | ID2_CTR6 | ID2_CTR1X6 | ID2_CTR1X4 | ID2_CTR4),
   EASTBIT(ID2_OUTR6| ID2_CTR6 | ID2_CTR1X6 | ID2_CTR1X4 | ID2_CTR4)};

/* Used only if center 6 are contiguous, then a gap, then isolated people. */
static const id_bit_table id_bit_table_1x10[] = {
   NOBIT(ID2_OUTR2| ID2_NCTR1X6| ID2_NCTR1X4),
   NOBIT(0),
   NOBIT(ID2_CTR6 | ID2_CTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_CTR6 | ID2_CTR1X6 | ID2_CTR1X4  | ID2_CTR4),
   NOBIT(ID2_CTR6 | ID2_CTR1X6 | ID2_CTR1X4  | ID2_CTR4),
   NOBIT(ID2_OUTR2| ID2_NCTR1X6| ID2_NCTR1X4),
   NOBIT(0),
   NOBIT(ID2_CTR6 | ID2_CTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_CTR6 | ID2_CTR1X6 | ID2_CTR1X4  | ID2_CTR4),
   NOBIT(ID2_CTR6 | ID2_CTR1X6 | ID2_CTR1X4  | ID2_CTR4)};

static const id_bit_table id_bit_table_qtag[] = {
   WESTBIT(ID2_END    | ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_CTR6  | ID2_OUTR6),
   EASTBIT(ID2_END    | ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_CTR6  | ID2_OUTR6),
   EASTBIT(ID2_CENTER | ID2_CTR4      | ID2_CTR1X4  | ID2_OUTR2 | ID2_OUTR6),
   WESTBIT(ID2_CENTER | ID2_CTR4      | ID2_CTR1X4  | ID2_CTR6  | ID2_CTR2),
   EASTBIT(ID2_END    | ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_CTR6  | ID2_OUTR6),
   WESTBIT(ID2_END    | ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_CTR6  | ID2_OUTR6),
   WESTBIT(ID2_CENTER | ID2_CTR4      | ID2_CTR1X4  | ID2_OUTR2 | ID2_OUTR6),
   EASTBIT(ID2_CENTER | ID2_CTR4      | ID2_CTR1X4  | ID2_CTR6  | ID2_CTR2)};

static const id_bit_table id_bit_table_2stars[] = {
   NOBIT(ID2_OUTR6|ID2_CTR6),
   NOBIT(ID2_OUTR6|ID2_CTR6),
   NOBIT(ID2_OUTR6|ID2_OUTR2),
   NOBIT(ID2_CTR2|ID2_CTR6),
   NOBIT(ID2_OUTR6|ID2_CTR6),
   NOBIT(ID2_OUTR6|ID2_CTR6),
   NOBIT(ID2_OUTR6|ID2_OUTR2),
   NOBIT(ID2_CTR2|ID2_CTR6)};

static const id_bit_table id_bit_table_ptpd[] = {
   {ID2_OUTR6|ID2_OUTR2,            ID2_OUTR6|ID2_OUTR2,           ID2_OUTR6|ID2_OUTR2,            ID2_OUTR6|ID2_OUTR2},
   {ID2_OUTR6|ID2_CTR6|ID2_LEAD,    ID2_OUTR6|ID2_CTR6|ID2_BEAU,   ID2_OUTR6|ID2_CTR6|ID2_TRAILER, ID2_OUTR6|ID2_CTR6|ID2_BELLE},
   {ID2_CTR2|ID2_CTR6|ID2_BEAU,     ID2_CTR2|ID2_CTR6|ID2_TRAILER, ID2_CTR2|ID2_CTR6|ID2_BELLE,    ID2_CTR2|ID2_CTR6|ID2_LEAD},
   {ID2_OUTR6|ID2_CTR6|ID2_TRAILER, ID2_OUTR6|ID2_CTR6|ID2_BELLE,  ID2_OUTR6|ID2_CTR6|ID2_LEAD,    ID2_OUTR6|ID2_CTR6|ID2_BEAU},
   {ID2_OUTR6|ID2_OUTR2,            ID2_OUTR6|ID2_OUTR2,           ID2_OUTR6|ID2_OUTR2,            ID2_OUTR6|ID2_OUTR2},
   {ID2_OUTR6|ID2_CTR6|ID2_TRAILER, ID2_OUTR6|ID2_CTR6|ID2_BELLE,  ID2_OUTR6|ID2_CTR6|ID2_LEAD,    ID2_OUTR6|ID2_CTR6|ID2_BEAU},
   {ID2_CTR2|ID2_CTR6|ID2_BELLE,    ID2_CTR2|ID2_CTR6|ID2_LEAD,    ID2_CTR2|ID2_CTR6|ID2_BEAU,     ID2_CTR2|ID2_CTR6|ID2_TRAILER},
   {ID2_OUTR6|ID2_CTR6|ID2_LEAD,    ID2_OUTR6|ID2_CTR6|ID2_BEAU,   ID2_OUTR6|ID2_CTR6|ID2_TRAILER, ID2_OUTR6|ID2_CTR6|ID2_BELLE}};

static const id_bit_table id_bit_table_crosswave[] = {
   WESTBIT( ID2_END   |ID2_OUTRPAIRS|ID2_OUTR6|ID2_OUTR2|ID2_NCTRDMD| ID2_NCTR1X4),
   EASTBIT( ID2_END   |ID2_OUTRPAIRS|ID2_OUTR6|ID2_CTR6 |ID2_CTRDMD | ID2_NCTR1X4),
   NORTHBIT(ID2_CENTER|ID2_CTR4     |ID2_OUTR6|ID2_CTR6 |ID2_NCTRDMD| ID2_CTR1X4),
   SOUTHBIT(ID2_CENTER|ID2_CTR4     |ID2_CTR2 |ID2_CTR6 |ID2_CTRDMD | ID2_CTR1X4),
   EASTBIT( ID2_END   |ID2_OUTRPAIRS|ID2_OUTR6|ID2_OUTR2|ID2_NCTRDMD| ID2_NCTR1X4),
   WESTBIT( ID2_END   |ID2_OUTRPAIRS|ID2_OUTR6|ID2_CTR6 |ID2_CTRDMD | ID2_NCTR1X4),
   SOUTHBIT(ID2_CENTER|ID2_CTR4     |ID2_OUTR6|ID2_CTR6 |ID2_NCTRDMD| ID2_CTR1X4),
   NORTHBIT(ID2_CENTER|ID2_CTR4     |ID2_CTR2 |ID2_CTR6 |ID2_CTRDMD | ID2_CTR1X4)};

static const id_bit_table id_bit_table_c1phan[] = {
   WESTBIT(0),
   NORTHBIT(0),
   EASTBIT(0),
   SOUTHBIT(0),
   NORTHBIT(0),
   EASTBIT(0),
   SOUTHBIT(0),
   WESTBIT(0),
   EASTBIT(0),
   SOUTHBIT(0),
   WESTBIT(0),
   NORTHBIT(0),
   SOUTHBIT(0),
   WESTBIT(0),
   NORTHBIT(0),
   EASTBIT(0)};

static const id_bit_table id_bit_table_gal[] = {
   NOBIT(ID2_END),
   NWBITS(ID2_CENTER|ID2_CTR4),
   NOBIT(ID2_END),
   NEBITS(ID2_CENTER|ID2_CTR4),
   NOBIT(ID2_END),
   SEBITS(ID2_CENTER|ID2_CTR4),
   NOBIT(ID2_END),
   SWBITS(ID2_CENTER|ID2_CTR4)};

static const id_bit_table id_bit_table_hrglass[] = {
   NOBIT(ID2_END            |ID2_OUTR6|ID2_NCTRDMD| ID2_CTR6),
   NOBIT(ID2_END            |ID2_OUTR6|ID2_NCTRDMD| ID2_CTR6),
   NOBIT(ID2_CENTER|ID2_CTR4|ID2_OUTR6|ID2_CTRDMD | ID2_OUTR2),
   NOBIT(ID2_CENTER|ID2_CTR4|ID2_CTR2 |ID2_CTRDMD | ID2_CTR6),
   NOBIT(ID2_END            |ID2_OUTR6|ID2_NCTRDMD| ID2_CTR6),
   NOBIT(ID2_END            |ID2_OUTR6|ID2_NCTRDMD| ID2_CTR6),
   NOBIT(ID2_CENTER|ID2_CTR4|ID2_OUTR6|ID2_CTRDMD | ID2_OUTR2),
   NOBIT(ID2_CENTER|ID2_CTR4|ID2_CTR2 |ID2_CTRDMD | ID2_CTR6)};

static const id_bit_table id_bit_table_dhrglass[] = {
   NORTHBIT(ID2_END            |ID2_NCTRDMD| ID2_OUTR6|ID2_OUTR1X3),
   NORTHBIT(ID2_END            |ID2_NCTRDMD| ID2_OUTR6|ID2_OUTR1X3),
   NOBIT(   ID2_CENTER|ID2_CTR4|ID2_CTRDMD | ID2_OUTR6|ID2_NOUTR1X3),
   NORTHBIT(ID2_CENTER|ID2_CTR4|ID2_CTRDMD | ID2_CTR2 |ID2_OUTR1X3),
   SOUTHBIT(ID2_END            |ID2_NCTRDMD| ID2_OUTR6|ID2_OUTR1X3),
   SOUTHBIT(ID2_END            |ID2_NCTRDMD| ID2_OUTR6|ID2_OUTR1X3),
   NOBIT(   ID2_CENTER|ID2_CTR4|ID2_CTRDMD | ID2_OUTR6|ID2_NOUTR1X3),
   SOUTHBIT(ID2_CENTER|ID2_CTR4|ID2_CTRDMD | ID2_CTR2 |ID2_OUTR1X3)};

static const id_bit_table id_bit_table_323[] = {
   NOBIT(  ID2_NCTRDMD| ID2_OUTR6|ID2_OUTR1X3),
   NOBIT(  ID2_CTRDMD | ID2_OUTR6|ID2_OUTR1X3),
   NOBIT(  ID2_NCTRDMD| ID2_OUTR6|ID2_OUTR1X3),
   EASTBIT(ID2_CTRDMD | ID2_CTR2 |ID2_NOUTR1X3),
   NOBIT(  ID2_NCTRDMD| ID2_OUTR6|ID2_OUTR1X3),
   NOBIT(  ID2_CTRDMD | ID2_OUTR6|ID2_OUTR1X3),
   NOBIT(  ID2_NCTRDMD| ID2_OUTR6|ID2_OUTR1X3),
   WESTBIT(ID2_CTRDMD | ID2_CTR2 |ID2_NOUTR1X3)};

static const id_bit_table id_bit_table_23232[] = {
   NOBIT(   ID2_NCTRDMD),
   NOBIT(   ID2_NCTRDMD),
   NORTHBIT(ID2_NCTRDMD),
   NORTHBIT(ID2_CTRDMD|ID2_CTR2),
   NORTHBIT(ID2_NCTRDMD),
   NOBIT(   ID2_CTRDMD),
   NOBIT(   ID2_NCTRDMD),
   NOBIT(   ID2_NCTRDMD),
   SOUTHBIT(ID2_NCTRDMD),
   SOUTHBIT(ID2_CTRDMD|ID2_CTR2),
   SOUTHBIT(ID2_NCTRDMD),
   NOBIT(   ID2_CTRDMD)};

// If the population is "common spot diamonds", the richer table below is used instead.
static const id_bit_table id_bit_table_bigdmd[] = {
   WESTBIT(ID2_NCTR1X4 | ID2_OUTR6),
   EASTBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NORTHBIT(ID2_CTR1X4 | ID2_OUTR6),
   SOUTHBIT(ID2_CTR1X4 | ID2_CTR2),
   WESTBIT(ID2_NCTR1X4 | ID2_OUTR6),
   EASTBIT(ID2_NCTR1X4 | ID2_OUTR6),
   EASTBIT(ID2_NCTR1X4 | ID2_OUTR6),
   WESTBIT(ID2_NCTR1X4 | ID2_OUTR6),
   SOUTHBIT(ID2_CTR1X4 | ID2_OUTR6),
   NORTHBIT(ID2_CTR1X4 | ID2_CTR2),
   EASTBIT(ID2_NCTR1X4 | ID2_OUTR6),
   WESTBIT(ID2_NCTR1X4 | ID2_OUTR6)};

// This table is only accepted if the population is "common spot diamonds".
id_bit_table id_bit_table_bigdmd_wings[] = {
   WESTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   EASTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   NORTHBIT(ID2_CTR4     | ID2_CTR1X4  | ID2_OUTR6),
   SOUTHBIT(ID2_CTR4     | ID2_CTR1X4  | ID2_CTR2),
   WESTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   EASTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   EASTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   WESTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   SOUTHBIT(ID2_CTR4     | ID2_CTR1X4  | ID2_OUTR6),
   NORTHBIT(ID2_CTR4     | ID2_CTR1X4  | ID2_CTR2),
   EASTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   WESTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6)};

static const id_bit_table id_bit_table_bigptpd[] = {
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_CTR2),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_CTR2),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_OUTR6)};

/* If the population is a distorted tidal line (2-4-2), the richer table below is used instead. */
static const id_bit_table id_bit_table_bigbone[] = {
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_CTR1X4  | ID2_OUTR6),
   NOBIT(ID2_CTR1X4  | ID2_CTR2),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_CTR1X4  | ID2_OUTR6),
   NOBIT(ID2_CTR1X4  | ID2_CTR2),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6)};

// This table is only accepted if the population is a distorted tidal line (2-4-2).
id_bit_table id_bit_table_bigbone_wings[] = {
   WESTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   EASTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   WESTBIT(ID2_CTR4      | ID2_CTR1X4  | ID2_OUTR6),
   EASTBIT(ID2_CTR4      | ID2_CTR1X4  | ID2_CTR2),
   WESTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   EASTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   EASTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   WESTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   EASTBIT(ID2_CTR4      | ID2_CTR1X4  | ID2_OUTR6),
   WESTBIT(ID2_CTR4      | ID2_CTR1X4  | ID2_CTR2),
   EASTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6),
   WESTBIT(ID2_OUTRPAIRS | ID2_NCTR1X4 | ID2_OUTR6)};

static const id_bit_table id_bit_table_bigrig[] = {
   NOBIT(0),
   NOBIT(0),
   NOBIT(0),
   NOBIT(0),
   NOBIT( ID2_CENTER|ID2_CTR4),
   NOBIT( ID2_CENTER|ID2_CTR4),
   NOBIT(0),
   NOBIT(0),
   NOBIT(0),
   NOBIT(0),
   NOBIT( ID2_CENTER|ID2_CTR4),
   NOBIT( ID2_CENTER|ID2_CTR4)};

/* If the population is a "common spot hourglass", the richer table below is used instead. */
static const id_bit_table id_bit_table_bighrgl[] = {
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_CTRDMD  | ID2_OUTR6),
   NOBIT(ID2_CTRDMD  | ID2_CTR2),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_CTRDMD  | ID2_OUTR6),
   NOBIT(ID2_CTRDMD  | ID2_CTR2),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6)};

/* This table is only accepted if the population is a "common spot hourglass". */
id_bit_table id_bit_table_bighrgl_wings[] = {
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_CTR4      | ID2_CTRDMD  | ID2_OUTR6),
   NOBIT(ID2_CTR4      | ID2_CTRDMD  | ID2_CTR2),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_CTR4      | ID2_CTRDMD  | ID2_OUTR6),
   NOBIT(ID2_CTR4      | ID2_CTRDMD  | ID2_CTR2),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6)};

/* If the population is a "common spot distorted hourglass", the richer table below is used instead. */
static const id_bit_table id_bit_table_bigdhrgl[] = {
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_CTRDMD  | ID2_CTR2),
   NOBIT(ID2_CTRDMD  | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_CTRDMD  | ID2_CTR2),
   NOBIT(ID2_CTRDMD  | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTR6)};

/* This table is only accepted if the population is a "common spot distorted hourglass". */
id_bit_table id_bit_table_bigdhrgl_wings[] = {
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_CTR4      | ID2_CTRDMD  | ID2_CTR2),
   NOBIT(ID2_CTR4      | ID2_CTRDMD  | ID2_OUTR6),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_CTR4      | ID2_CTRDMD  | ID2_CTR2),
   NOBIT(ID2_CTR4      | ID2_CTRDMD  | ID2_OUTR6),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6),
   NOBIT(ID2_OUTRPAIRS | ID2_NCTRDMD | ID2_OUTR6)};

// If the population of a "bigh" is a "double bent tidal line", use this.
id_bit_table id_bit_table_bigh_dblbent[] = {
   NORTHBIT(ID2_OUTR6| ID2_NCTR1X4| ID2_OUTRPAIRS),
   SOUTHBIT(ID2_OUTR6| ID2_NCTR1X4| ID2_OUTRPAIRS),
   NORTHBIT(ID2_OUTR6| ID2_NCTR1X4| ID2_OUTRPAIRS),
   SOUTHBIT(ID2_OUTR6| ID2_NCTR1X4| ID2_OUTRPAIRS),
   WESTBIT( ID2_OUTR6| ID2_CTR1X4 | ID2_CTR4),
   EASTBIT( ID2_CTR2 | ID2_CTR1X4 | ID2_CTR4),
   SOUTHBIT(ID2_OUTR6| ID2_NCTR1X4| ID2_OUTRPAIRS),
   NORTHBIT(ID2_OUTR6| ID2_NCTR1X4| ID2_OUTRPAIRS),
   SOUTHBIT(ID2_OUTR6| ID2_NCTR1X4| ID2_OUTRPAIRS),
   NORTHBIT(ID2_OUTR6| ID2_NCTR1X4| ID2_OUTRPAIRS),
   EASTBIT( ID2_OUTR6| ID2_CTR1X4 | ID2_CTR4),
   WESTBIT( ID2_CTR2 | ID2_CTR1X4 | ID2_CTR4)};

// If the population of a 3x4 is an "H", use this.
id_bit_table id_bit_table_3x4_h[] = {
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4|ID2_OUTR1X3),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4|ID2_OUTR1X3),
   NOBIT(ID2_OUTR6 | ID2_CTR1X4 |ID2_OUTR1X3),
   NOBIT(ID2_CTR2  | ID2_CTR1X4 |ID2_NOUTR1X3),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4|ID2_OUTR1X3),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4|ID2_OUTR1X3),
   NOBIT(ID2_OUTR6 | ID2_CTR1X4 |ID2_OUTR1X3),
   NOBIT(ID2_CTR2  | ID2_CTR1X4 |ID2_NOUTR1X3)};

// Else if the center 2x3 of a 3x4 is full, use this.
id_bit_table id_bit_table_3x4_ctr6[] = {
   NOBIT(ID2_OUTR6 | ID2_OUTR2),
   NOBIT(ID2_OUTR6 | ID2_CTR6),
   NOBIT(ID2_OUTR6 | ID2_CTR6),
   NOBIT(ID2_OUTR6 | ID2_OUTR2),
   NOBIT(ID2_OUTR6 | ID2_OUTR2),
   NOBIT(ID2_CTR2  | ID2_CTR6),
   NOBIT(ID2_OUTR6 | ID2_OUTR2),
   NOBIT(ID2_OUTR6 | ID2_CTR6),
   NOBIT(ID2_OUTR6 | ID2_CTR6),
   NOBIT(ID2_OUTR6 | ID2_OUTR2),
   NOBIT(ID2_OUTR6 | ID2_OUTR2),
   NOBIT(ID2_CTR2  | ID2_CTR6)};

// Else if the population is offset C/L/W, use this.
id_bit_table id_bit_table_3x4_offset[] = {
   NOBIT(ID2_OUTR6 | ID2_OUTRPAIRS | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_OUTRPAIRS | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_OUTRPAIRS | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_OUTRPAIRS | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6                 | ID2_CTR1X4),
   NOBIT(ID2_CTR2                  | ID2_CTR1X4),
   NOBIT(ID2_OUTR6 | ID2_OUTRPAIRS | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_OUTRPAIRS | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_OUTRPAIRS | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_OUTRPAIRS | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6                 | ID2_CTR1X4),
   NOBIT(ID2_CTR2                  | ID2_CTR1X4)};

// Else if the "corners" are sensibly occupied, use this.
id_bit_table id_bit_table_3x4_corners[] = {
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4 | ID2_END),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4 | ID2_CENTER),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4 | ID2_CENTER),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4 | ID2_END),
   NOBIT(0),
   NOBIT(ID2_CTR2  | ID2_CTR1X4  | ID2_CENTER),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4 | ID2_END),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4 | ID2_CENTER),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4 | ID2_CENTER),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4 | ID2_END),
   NOBIT(0),
   NOBIT(ID2_CTR2  | ID2_CTR1X4  | ID2_CENTER)};

// Else if the setup is occupied as Z's, use this.
id_bit_table id_bit_table_3x4_zs[] = {
   NOBIT(ID2_NCTR1X4 | ID2_OUTRPAIRS),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4 | ID2_OUTRPAIRS),
   NOBIT(ID2_CTR1X4  | ID2_OUTRPAIRS),
   NOBIT(ID2_CTR1X4|ID2_CTR2),
   NOBIT(ID2_NCTR1X4 | ID2_OUTRPAIRS),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4 | ID2_OUTRPAIRS),
   NOBIT(ID2_CTR1X4  | ID2_OUTRPAIRS),
   NOBIT(ID2_CTR1X4|ID2_CTR2)};

// If all else fails, use this.
static const id_bit_table id_bit_table_3x4[] = {
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_CTR1X4),
   NOBIT(ID2_CTR2  | ID2_CTR1X4),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_NCTR1X4),
   NOBIT(ID2_OUTR6 | ID2_CTR1X4),
   NOBIT(ID2_CTR2  | ID2_CTR1X4)};

/* This is only used if the center 2x3 is full. */
static const id_bit_table id_bit_table_d3x4[] = {
   NOBIT(ID2_OUTR6 | ID2_OUTR2),
   NOBIT(ID2_OUTR6 | ID2_CTR6),
   NOBIT(ID2_OUTR6 | ID2_CTR6),
   NOBIT(ID2_OUTR6 | ID2_CTR6),
   NOBIT(ID2_OUTR6 | ID2_OUTR2),
   NOBIT(ID2_CTR2  | ID2_OUTR2),
   NOBIT(ID2_OUTR6 | ID2_OUTR2),
   NOBIT(ID2_OUTR6 | ID2_CTR6),
   NOBIT(ID2_OUTR6 | ID2_CTR6),
   NOBIT(ID2_OUTR6 | ID2_CTR6),
   NOBIT(ID2_OUTR6 | ID2_OUTR2),
   NOBIT(ID2_CTR2  | ID2_CTR6)};

// Use only if center 1x4 is full.
id_bit_table id_bit_table_bigh[] = {
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4),
   NOBIT(ID2_CTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4),
   NOBIT(ID2_CTR1X4)};

// This table is used when the outer 1x4's are yes-yes-no-no.
id_bit_table id_bit_table_s_hsqtag[] = {
   NOBIT(ID2_NCTRDMD | ID2_OUTRPAIRS | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTRPAIRS | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTRPAIRS | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTRPAIRS | ID2_OUTR6),
   NOBIT(ID2_CTRDMD  | ID2_CTR4      | ID2_OUTR6),
   NOBIT(ID2_CTRDMD  | ID2_CTR4      | ID2_CTR2),
   NOBIT(ID2_NCTRDMD | ID2_OUTRPAIRS | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTRPAIRS | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTRPAIRS | ID2_OUTR6),
   NOBIT(ID2_NCTRDMD | ID2_OUTRPAIRS | ID2_OUTR6),
   NOBIT(ID2_CTRDMD  | ID2_CTR4      | ID2_OUTR6),
   NOBIT(ID2_CTRDMD  | ID2_CTR4      | ID2_CTR2)};

id_bit_table id_bit_table_sdmdlndmd[] = {
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_CTR1X4  | ID2_OUTR6),
   NOBIT(ID2_CTR1X4  | ID2_CTR2),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_CTR1X4  | ID2_OUTR6),
   NOBIT(ID2_CTR1X4  | ID2_CTR2),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X4 | ID2_OUTR6)};

// If the population is a butterfly, this table is used.
id_bit_table id_bit_table_butterfly[] = {
   NOBIT(ID2_END),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_END),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_END),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_END),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_CENTER | ID2_CTR4)};

// If the population of a 4x4 is a center box with outer pairs somewhere, this table is used.
id_bit_table id_bit_table_4x4_outer_pairs[] = {
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_CENTER | ID2_CTR4)};

// Otherwise, this table is used for 4x4's.
// The use of "ID2_NCTR1X4" is a crock, of course.
id_bit_table id_bit_table_4x4[] = {
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR4)};

// This table is used for 4x6's if the center 2x2 box is full.
// The use of "ID2_NCTR1X4" is a crock, of course.
id_bit_table id_bit_table_4x6[] = {
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR4),
   NOBIT(ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR4),
   NOBIT(ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4)};

id_bit_table id_bit_table_525_nw[] = {
   NOBIT(ID2_OUTR6 | ID2_NCTRDMD | ID2_OUTR1X3),
   NOBIT(ID2_OUTR6 | ID2_NCTRDMD | ID2_OUTR1X3),
   NOBIT(ID2_OUTR6 | ID2_CTRDMD  | ID2_OUTR1X3),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_CTR2  | ID2_CTRDMD  | ID2_NOUTR1X3),
   NOBIT(ID2_OUTR6 | ID2_NCTRDMD | ID2_OUTR1X3),
   NOBIT(ID2_OUTR6 | ID2_NCTRDMD | ID2_OUTR1X3),
   NOBIT(ID2_OUTR6 | ID2_CTRDMD  | ID2_OUTR1X3),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_CTR2  | ID2_CTRDMD  | ID2_NOUTR1X3)};

id_bit_table id_bit_table_525_ne[] = {
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_OUTR6 | ID2_CTRDMD  | ID2_OUTR1X3),
   NOBIT(ID2_OUTR6 | ID2_NCTRDMD | ID2_OUTR1X3),
   NOBIT(ID2_OUTR6 | ID2_NCTRDMD | ID2_OUTR1X3),
   NOBIT(ID2_CTR2  | ID2_CTRDMD  | ID2_NOUTR1X3),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_OUTR6 | ID2_CTRDMD  | ID2_OUTR1X3),
   NOBIT(ID2_OUTR6 | ID2_NCTRDMD | ID2_OUTR1X3),
   NOBIT(ID2_OUTR6 | ID2_NCTRDMD | ID2_OUTR1X3),
   NOBIT(ID2_CTR2  | ID2_CTRDMD  | ID2_NOUTR1X3)};

id_bit_table id_bit_table_343_outr[] = {
   NOBIT(ID2_OUTR1X3),
   NOBIT(ID2_OUTR1X3),
   NOBIT(ID2_OUTR1X3),
   NOBIT(ID2_NOUTR1X3),
   NOBIT(ID2_NOUTR1X3),
   NOBIT(ID2_OUTR1X3),
   NOBIT(ID2_OUTR1X3),
   NOBIT(ID2_OUTR1X3),
   NOBIT(ID2_NOUTR1X3),
   NOBIT(ID2_NOUTR1X3)};

id_bit_table id_bit_table_343_innr[] = {
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4)};

id_bit_table id_bit_table_545_outr[] = {
   NOBIT(ID2_OUTR1X3 |ID2_NCTR1X4),
   NOBIT(ID2_OUTR1X3 |ID2_NCTR1X4),
   NOBIT(ID2_OUTR1X3 |ID2_NCTR1X4),
   NOBIT(ID2_OUTR1X3 |ID2_NCTR1X4),
   NOBIT(ID2_OUTR1X3 |ID2_NCTR1X4),
   NOBIT(ID2_NOUTR1X3|ID2_CTR1X4),
   NOBIT(ID2_NOUTR1X3|ID2_CTR1X4),
   NOBIT(ID2_OUTR1X3 |ID2_NCTR1X4),
   NOBIT(ID2_OUTR1X3 |ID2_NCTR1X4),
   NOBIT(ID2_OUTR1X3 |ID2_NCTR1X4),
   NOBIT(ID2_OUTR1X3 |ID2_NCTR1X4),
   NOBIT(ID2_OUTR1X3 |ID2_NCTR1X4),
   NOBIT(ID2_NOUTR1X3|ID2_CTR1X4),
   NOBIT(ID2_NOUTR1X3|ID2_CTR1X4)};

id_bit_table id_bit_table_545_innr[] = {
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4)};

id_bit_table id_bit_table_3dmd_in_out[] = {
   NOBIT(ID2_END | ID2_NCTR1X4),
   NOBIT(0),
   NOBIT(ID2_END | ID2_NCTR1X4),
   NOBIT(0),
   NOBIT(ID2_CENTER | ID2_CTR4 | ID2_CTR1X4),
   NOBIT(ID2_CENTER | ID2_CTR4 | ID2_CTR1X4),
   NOBIT(ID2_END | ID2_NCTR1X4),
   NOBIT(0),
   NOBIT(ID2_END | ID2_NCTR1X4),
   NOBIT(0),
   NOBIT(ID2_CENTER | ID2_CTR4 | ID2_CTR1X4),
   NOBIT(ID2_CENTER | ID2_CTR4 | ID2_CTR1X4)};

static const id_bit_table id_bit_table_s4p2x1dmd[] = {
   NOBIT(ID2_CTR1X6),
   NOBIT(ID2_CTR1X6),
   NOBIT(ID2_CTR1X6),
   NOBIT(ID2_CTR1X6),
   NOBIT(ID2_NCTR1X6),
   NOBIT(ID2_CTR1X6),
   NOBIT(ID2_CTR1X6),
   NOBIT(ID2_NCTR1X6)};

static const id_bit_table id_bit_table_sline2box[] = {
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_CENTER | ID2_CTR4)};

static const id_bit_table id_bit_table_sline6box[] = {
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_CENTER | ID2_CTR4),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS)};

static const id_bit_table id_bit_table_splinedmd[] = {
   NOBIT(ID2_CTR1X6),
   NOBIT(ID2_CTR1X6),
   NOBIT(ID2_CTR1X6),
   NOBIT(ID2_CTR1X6),
   NOBIT(ID2_CTR1X6),
   NOBIT(ID2_NCTR1X6),
   NOBIT(ID2_CTR1X6),
   NOBIT(ID2_NCTR1X6)};

static const id_bit_table id_bit_table_dmdpdmd[] = {
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_CTR2),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_OUTR6),
   NOBIT(ID2_CTR2)};

static const id_bit_table id_bit_table_swingedstar[] = {
   NOBIT(ID2_OUTR2| ID2_OUTRPAIRS | ID2_END | ID2_CTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_CTR6 | ID2_OUTRPAIRS| ID2_END| ID2_CTR1X6 | ID2_CTR1X4),
   NOBIT(ID2_CTR6|  ID2_CTR4|   ID2_CENTER| ID2_CTR1X6 | ID2_CTR1X4),
   NOBIT(ID2_CTR6 | ID2_CTR4|   ID2_CENTER| ID2_NCTR1X6  | ID2_NCTR1X4),
   NOBIT(ID2_OUTR2| ID2_OUTRPAIRS | ID2_END | ID2_CTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_CTR6 | ID2_OUTRPAIRS| ID2_END| ID2_CTR1X6 | ID2_CTR1X4),
   NOBIT(ID2_CTR6|  ID2_CTR4|   ID2_CENTER| ID2_CTR1X6 | ID2_CTR1X4),
   NOBIT(ID2_CTR6 | ID2_CTR4|   ID2_CENTER| ID2_NCTR1X6  | ID2_NCTR1X4)};

static const id_bit_table id_bit_table_spindle[] = {
   NORTHBIT(ID2_CTR6 |ID2_OUTR6),
   NORTHBIT(ID2_CTR6 |ID2_CTR2),
   NORTHBIT(ID2_CTR6 |ID2_OUTR6),
   NOBIT(   ID2_OUTR2|ID2_OUTR6),
   SOUTHBIT(ID2_CTR6 |ID2_OUTR6),
   SOUTHBIT(ID2_CTR6 |ID2_CTR2),
   SOUTHBIT(ID2_CTR6 |ID2_OUTR6),
   NOBIT(   ID2_OUTR2|ID2_OUTR6)};

static const id_bit_table id_bit_table_nxtrglcw[] = {
   NOBIT(  ID2_CTR6),
   WESTBIT(ID2_CTR6),
   EASTBIT(ID2_CTR6),
   NOBIT(  ID2_OUTR2),
   NOBIT(  ID2_CTR6),
   EASTBIT(ID2_CTR6),
   WESTBIT(ID2_CTR6),
   NOBIT(  ID2_OUTR2)};

static const id_bit_table id_bit_table_nxtrglccw[] = {
   WESTBIT(ID2_CTR6),
   EASTBIT(ID2_CTR6),
   NOBIT(  ID2_CTR6),
   NOBIT(  ID2_OUTR2),
   EASTBIT(ID2_CTR6),
   WESTBIT(ID2_CTR6),
   NOBIT(  ID2_CTR6),
   NOBIT(  ID2_OUTR2)};

static const id_bit_table id_bit_table_nptrglcw[] = {
   NOBIT(  ID2_OUTR6),
   WESTBIT(ID2_CTR2),
   WESTBIT(ID2_OUTR6),
   EASTBIT(ID2_OUTR6),
   NOBIT(  ID2_OUTR6),
   EASTBIT(ID2_CTR2),
   EASTBIT(ID2_OUTR6),
   WESTBIT(ID2_OUTR6)};

static const id_bit_table id_bit_table_nptrglccw[] = {
   WESTBIT(ID2_OUTR6),
   EASTBIT(ID2_OUTR6),
   WESTBIT(ID2_CTR2),
   NOBIT(  ID2_OUTR6),
   EASTBIT(ID2_OUTR6),
   WESTBIT(ID2_OUTR6),
   EASTBIT(ID2_CTR2),
   NOBIT(  ID2_OUTR6)};

// This is used only if the center line of 6 is full.
static const id_bit_table id_bit_table_wqtag[] = {
   NOBIT(  ID2_NCTR1X6),
   NOBIT(  ID2_NCTR1X6),
   EASTBIT(ID2_CTR1X6),
   WESTBIT(ID2_CTR1X6),
   EASTBIT(ID2_CTR1X6),
   NOBIT(  ID2_NCTR1X6),
   NOBIT(  ID2_NCTR1X6),
   WESTBIT(ID2_CTR1X6),
   EASTBIT(ID2_CTR1X6),
   WESTBIT(ID2_CTR1X6)};

// This is used only if the center 2 people are absent.
id_bit_table id_bit_table_wqtag_hollow[] = {
   WESTBIT(ID2_CTR6),
   EASTBIT(ID2_CTR6),
   EASTBIT(ID2_OUTR2),
   WESTBIT(ID2_CTR6),
   NOBIT(0),
   EASTBIT(ID2_CTR6),
   WESTBIT(ID2_CTR6),
   WESTBIT(ID2_OUTR2),
   EASTBIT(ID2_CTR6),
   NOBIT(0)};

// This is only accepted if the outer pairs are occupied.
static const id_bit_table id_bit_table_deepxwv[] = {
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_CENTER),
   NOBIT(ID2_CENTER),
   NOBIT(ID2_CENTER),
   NOBIT(ID2_CENTER),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_OUTRPAIRS),
   NOBIT(ID2_CENTER),
   NOBIT(ID2_CENTER),
   NOBIT(ID2_CENTER),
   NOBIT(ID2_CENTER)};

static const id_bit_table id_bit_table_rigger[] = {
   NWBITS( ID2_CTR6 |ID2_CENTER|ID2_CTR4),
   NEBITS( ID2_CTR6 |ID2_CENTER|ID2_CTR4),
   EASTBIT(ID2_OUTR2|ID2_END   |ID2_OUTRPAIRS),
   WESTBIT(ID2_CTR6 |ID2_END   |ID2_OUTRPAIRS),
   SEBITS( ID2_CTR6 |ID2_CENTER|ID2_CTR4),
   SWBITS( ID2_CTR6 |ID2_CENTER|ID2_CTR4),
   WESTBIT(ID2_OUTR2|ID2_END   |ID2_OUTRPAIRS),
   EASTBIT(ID2_CTR6 |ID2_END   |ID2_OUTRPAIRS)};

static const id_bit_table id_bit_table_1x3dmd[] = {
   NOBIT(   ID2_END|    ID2_OUTRPAIRS| ID2_OUTR6| ID2_OUTR2| ID2_NCTRDMD | ID2_OUTR1X3),
   NOBIT(   ID2_END|    ID2_OUTRPAIRS| ID2_OUTR6| ID2_CTR6|  ID2_NCTRDMD | ID2_OUTR1X3),
   NOBIT(   ID2_CENTER| ID2_CTR4|      ID2_OUTR6| ID2_CTR6|  ID2_CTRDMD  | ID2_OUTR1X3),
   NORTHBIT(ID2_CENTER| ID2_CTR4|      ID2_CTR2|  ID2_CTR6|  ID2_CTRDMD  | ID2_NOUTR1X3),
   NOBIT(   ID2_END|    ID2_OUTRPAIRS| ID2_OUTR6| ID2_OUTR2| ID2_NCTRDMD | ID2_OUTR1X3),
   NOBIT(   ID2_END|    ID2_OUTRPAIRS| ID2_OUTR6| ID2_CTR6|  ID2_NCTRDMD | ID2_OUTR1X3),
   NOBIT(   ID2_CENTER| ID2_CTR4|      ID2_OUTR6| ID2_CTR6|  ID2_CTRDMD  | ID2_OUTR1X3),
   SOUTHBIT(ID2_CENTER| ID2_CTR4|      ID2_CTR2|  ID2_CTR6|  ID2_CTRDMD  | ID2_NOUTR1X3)};

static const id_bit_table id_bit_table_2x1dmd[] = {
   WESTBIT(ID2_NCTRDMD | ID2_CTR1X4),
   EASTBIT(ID2_CTRDMD  | ID2_CTR1X4),
   NOBIT(  ID2_CTRDMD  | ID2_NCTR1X4),
   EASTBIT(ID2_NCTRDMD | ID2_CTR1X4),
   WESTBIT(ID2_CTRDMD  | ID2_CTR1X4),
   NOBIT(  ID2_CTRDMD  | ID2_NCTR1X4)};

/*   Used to be this ...
static const id_bit_table id_bit_table_3x1dmd[] = {
   WESTBIT(ID2_OUTR2| ID2_OUTR6| ID2_OUTRPAIRS| ID2_END| ID2_CTR1X6|  ID2_NCTRDMD | ID2_NCTR1X4),
   EASTBIT(ID2_CTR6|  ID2_OUTR6| ID2_OUTRPAIRS| ID2_END| ID2_CTR1X6|  ID2_NCTRDMD | ID2_CTR1X4),
   WESTBIT(ID2_CTR6|  ID2_CTR2|  ID2_CTR4|   ID2_CENTER| ID2_CTR1X6|  ID2_CTRDMD  | ID2_CTR1X4),
   NOBIT(  ID2_CTR6|  ID2_OUTR6| ID2_CTR4|   ID2_CENTER| ID2_NCTR1X6| ID2_CTRDMD  | ID2_NCTR1X4),
   EASTBIT(ID2_OUTR2| ID2_OUTR6| ID2_OUTRPAIRS| ID2_END| ID2_CTR1X6|  ID2_NCTRDMD | ID2_NCTR1X4),
   WESTBIT(ID2_CTR6|  ID2_OUTR6| ID2_OUTRPAIRS| ID2_END| ID2_CTR1X6|  ID2_NCTRDMD | ID2_CTR1X4),
   EASTBIT(ID2_CTR6|  ID2_CTR2|  ID2_CTR4|   ID2_CENTER| ID2_CTR1X6|  ID2_CTRDMD  | ID2_CTR1X4),
   NOBIT(  ID2_CTR6|  ID2_OUTR6| ID2_CTR4|   ID2_CENTER| ID2_NCTR1X6| ID2_CTRDMD  | ID2_NCTR1X4)};
*/

// Now changed to this, so the "center 6" are the 1x6.
static const id_bit_table id_bit_table_3x1dmd[] = {
   WESTBIT(ID2_OUTR2| ID2_OUTR6| ID2_OUTRPAIRS| ID2_END| ID2_CTR1X6|  ID2_NCTRDMD | ID2_NCTR1X4),
   EASTBIT(ID2_CTR6|  ID2_OUTR6| ID2_OUTRPAIRS| ID2_END| ID2_CTR1X6|  ID2_NCTRDMD | ID2_CTR1X4),
   WESTBIT(ID2_CTR6|  ID2_CTR2|  ID2_CTR4|   ID2_CENTER| ID2_CTR1X6|  ID2_CTRDMD  | ID2_CTR1X4),
   NOBIT(  ID2_CTR6|  ID2_OUTR6| ID2_CTR4|   ID2_CENTER| ID2_NCTR1X6| ID2_CTRDMD  | ID2_NCTR1X4),
   EASTBIT(ID2_OUTR2| ID2_OUTR6| ID2_OUTRPAIRS| ID2_END| ID2_CTR1X6|  ID2_NCTRDMD | ID2_NCTR1X4),
   WESTBIT(ID2_CTR6|  ID2_OUTR6| ID2_OUTRPAIRS| ID2_END| ID2_CTR1X6|  ID2_NCTRDMD | ID2_CTR1X4),
   EASTBIT(ID2_CTR6|  ID2_CTR2|  ID2_CTR4|   ID2_CENTER| ID2_CTR1X6|  ID2_CTRDMD  | ID2_CTR1X4),
   NOBIT(  ID2_CTR6|  ID2_OUTR6| ID2_CTR4|   ID2_CENTER| ID2_NCTR1X6| ID2_CTRDMD  | ID2_NCTR1X4)};


// If center diamond is fully occupied and outer diamonds have only points, use this.
static const id_bit_table id_bit_table_3dmd[] = {
   NOBIT(ID2_OUTR6| ID2_NCTRDMD           |ID2_OUTR1X3),
   NOBIT(ID2_OUTR6| ID2_CTRDMD | ID2_CTR4 |ID2_OUTR1X3),
   NOBIT(ID2_OUTR6| ID2_NCTRDMD           |ID2_OUTR1X3),
   NOBIT(           ID2_NCTRDMD),
   NOBIT(           ID2_NCTRDMD),
   NOBIT(ID2_CTR2|  ID2_CTRDMD | ID2_CTR4 |ID2_NOUTR1X3),
   NOBIT(ID2_OUTR6| ID2_NCTRDMD           |ID2_OUTR1X3),
   NOBIT(ID2_OUTR6| ID2_CTRDMD | ID2_CTR4 |ID2_OUTR1X3),
   NOBIT(ID2_OUTR6| ID2_NCTRDMD           |ID2_OUTR1X3),
   NOBIT(           ID2_NCTRDMD),
   NOBIT(           ID2_NCTRDMD),
   NOBIT(ID2_CTR2|  ID2_CTRDMD | ID2_CTR4 |ID2_NOUTR1X3)};

// If center 1x6 is fully occupied, use this.  It is external.
id_bit_table id_bit_table_3dmd_ctr1x6[] = {
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_CTR1X6  | ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_CTR1X6  | ID2_CTR1X4  | ID2_OUTR6),
   NOBIT(ID2_CTR1X6  | ID2_CTR1X4  | ID2_CTR2),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_CTR1X6  | ID2_NCTR1X4 | ID2_OUTR6),
   NOBIT(ID2_CTR1X6  | ID2_CTR1X4  | ID2_OUTR6),
   NOBIT(ID2_CTR1X6  | ID2_CTR1X4  | ID2_CTR2)};

/* If only center 1x4 is fully occupied, use this.  It is external. */
id_bit_table id_bit_table_3dmd_ctr1x4[] = {
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4),
   NOBIT(ID2_CTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4),
   NOBIT(ID2_CTR1X4)};

static const id_bit_table id_bit_table_3mdmd[] = {
   NOBIT(           ID2_NCTRDMD),
   NOBIT(ID2_CTR2|  ID2_CTRDMD | ID2_CTR4),
   NOBIT(           ID2_NCTRDMD),
   NOBIT(           ID2_NCTRDMD),
   NOBIT(           ID2_NCTRDMD),
   NOBIT(           ID2_CTRDMD | ID2_CTR4),
   NOBIT(           ID2_NCTRDMD),
   NOBIT(ID2_CTR2|  ID2_CTRDMD | ID2_CTR4),
   NOBIT(           ID2_NCTRDMD),
   NOBIT(           ID2_NCTRDMD),
   NOBIT(           ID2_NCTRDMD),
   NOBIT(           ID2_CTRDMD | ID2_CTR4)};

// If center 1x4 is occupied, use this.
static const id_bit_table id_bit_table_4dmd[] = {
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4|ID2_CTR4),
   NOBIT(ID2_CTR1X4|ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4|ID2_CTR4),
   NOBIT(ID2_CTR1X4|ID2_CTR4)};

// If center diamond has only centers and outer diamonds have only points, use this.
id_bit_table id_bit_table_4dmd_cc_ee[] = {
   NOBIT(ID2_END|ID2_NCTR1X4|ID2_OUTRPAIRS),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_END|ID2_NCTR1X4|ID2_OUTRPAIRS),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_CENTER|ID2_CTR1X4|ID2_CTR4),
   NOBIT(ID2_CENTER|ID2_CTR1X4|ID2_CTR4),
   NOBIT(ID2_END|ID2_NCTR1X4|ID2_OUTRPAIRS),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_END|ID2_NCTR1X4|ID2_OUTRPAIRS),
   NOBIT(0),
   NOBIT(0),
   NOBIT(ID2_CENTER|ID2_CTR1X4|ID2_CTR4),
   NOBIT(ID2_CENTER|ID2_CTR1X4|ID2_CTR4)};

/* If center diamond of triple point-to-point diamonds is fully occupied,
   along with inboard points of other diamonds, this lets us do "triple trade".
   It is external. */
id_bit_table id_bit_table_3ptpd[] = {
   NORTHBIT(ID2_OUTR2| ID2_NCTRDMD),
   NORTHBIT(ID2_CTR6|  ID2_CTRDMD),
   NORTHBIT(ID2_OUTR2| ID2_NCTRDMD),
   NOBIT(   ID2_OUTR2| ID2_NCTRDMD),
   EASTBIT( ID2_CTR6|  ID2_NCTRDMD),
   WESTBIT( ID2_CTR6|  ID2_CTRDMD),
   SOUTHBIT(ID2_OUTR2| ID2_NCTRDMD),
   SOUTHBIT(ID2_CTR6|  ID2_CTRDMD),
   SOUTHBIT(ID2_OUTR2| ID2_NCTRDMD),
   NOBIT(   ID2_OUTR2| ID2_NCTRDMD),
   WESTBIT( ID2_CTR6|  ID2_NCTRDMD),
   EASTBIT( ID2_CTR6|  ID2_CTRDMD)};

static const id_bit_table id_bit_table_bone[] = {
   NORTHBIT(ID2_END  |ID2_NCTR1X4  |ID2_OUTRPAIRS|ID2_OUTR6),
   NORTHBIT(ID2_END  |ID2_NCTR1X4  |ID2_OUTRPAIRS|ID2_OUTR6),
   EASTBIT(ID2_CENTER|ID2_CTR1X4   |ID2_CTR4     |ID2_OUTR6),
   WESTBIT(ID2_CENTER|ID2_CTR1X4   |ID2_CTR4     |ID2_CTR2),
   SOUTHBIT(ID2_END  |ID2_NCTR1X4  |ID2_OUTRPAIRS|ID2_OUTR6),
   SOUTHBIT(ID2_END  |ID2_NCTR1X4  |ID2_OUTRPAIRS|ID2_OUTR6),
   WESTBIT(ID2_CENTER|ID2_CTR1X4   |ID2_CTR4     |ID2_OUTR6),
   EASTBIT(ID2_CENTER|ID2_CTR1X4   |ID2_CTR4     |ID2_CTR2)};

// Use this only if center 1x6 is fully occupied.
const id_bit_table id_bit_table_3x6_with_1x6[] = {
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_CTR1X6  | ID2_NCTR1X4),
   NOBIT(ID2_CTR1X6  | ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_CTR1X6  | ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X6 | ID2_NCTR1X4),
   NOBIT(ID2_CTR1X6  | ID2_NCTR1X4),
   NOBIT(ID2_CTR1X6  | ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_CTR1X6  | ID2_CTR1X4 | ID2_CTR4)};

// Use this one if center 1x4 only.
static const id_bit_table id_bit_table_3x6[] = {
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4)};

static const id_bit_table id_bit_table_4x5[] = {
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_CTR1X4 | ID2_CTR4),
   NOBIT(ID2_NCTR1X4),
   NOBIT(ID2_NCTR1X4)};


// The meaning of the special numeric characters in the display strings
// is as follows:
//
// 5 - space equivalent to 1/2 glyph.
// 6 - space equivalent to 1 full glyph.
// 7 - ONLY AFTER @ SIGN - @7 means 1/2 line feed instead of full line feed for @.
// 8 - Like 5, but one space less if doing ASCII.
//     (Exactly same as 5 if doing checkers.)
// 9 - space equivalent to 3/4 glyph.

// BEWARE!!  This list is keyed to the definition of "setup_kind" in database.h .
const setup_attr setup_attrs[] = {
   {-1,                       // setup_limits
    (const coordrec *) 0,     // setup_coords
    (const coordrec *) 0,     // nice_setup_coords
    {0, 0, 0, 0},             // mask_normal, mask_6_2, mask_2_6, mask_ctr_dmd
    {b_nothing, b_nothing},   // keytab
    {0, 0},                   // bounding_box
    false, false,             // four_way_symmetry, no_symmetry
    (const id_bit_table *) 0, // id_bit_table_ptr
    {(Cstring) 0,             // print_strings
     (Cstring) 0}},
   {0,                      // s1x1
    &thing1x1,
    &thing1x1,
    {0, 0, 0, 0},
    {b_1x1, b_1x1},
    {1, 1},
    true, false,
    (const id_bit_table *) 0,
    {"a@",
     (Cstring) 0}},
   {1,                      // s1x2
    &thing1x2,
    &thing1x2,
    {0, 0, 0, 0},
    {b_1x2, b_2x1},
    {2, 1},
    false, false,
    id_bit_table_1x2,
    {"a  b@",
     "a@b@"}},
   {2,                      // s1x3
    &thing1x3,
    &thing1x3,
    {0, 0, 0, 0},
    {b_1x3, b_3x1},
    {3, 1},
    false, false,
    (const id_bit_table *) 0,
    {"a  b  c@",
     "a@b@c@"}},
   {3,                      // s2x2
    &thing2x2,
    &thing2x2,
    {0, 0, 0, 0},
    {b_2x2, b_2x2},
    {2, 2},
    true, false,
    id_bit_table_2x2,
    {(Cstring) 0,
     (Cstring) 0}},
   {3,                      // s1x4
    &thing1x4,
    &thing1x4,
    {0x5, 0, 0, 0},
    {b_1x4, b_4x1},
    {4, 1},
    false, false,
    id_bit_table_1x4,
    {"a  b  d  c@",
     "a@b@d@c@"}},
   {3,                      // sdmd
    &thingdmd,
    &nicethingdmd,
    {0x5, 0, 0, 0},
    {b_dmd, b_pmd},
    {0, 2},
    false, false,
    id_bit_table_dmd,
    {"6 b@7a 6 c@76 d@",
     " 5a@@ db@@ 5c@"}},
   {3,                      // s_star
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_star, b_star},
    {0, 0},
    true, false,
    (const id_bit_table *) 0,
    {"5 b@a  c@5 d@",
     (Cstring) 0}},
   {2,                      // s_trngl
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_trngl, b_ptrngl},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {3,                      // s_trngl4
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_trngl4, b_ptrngl4},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {4,                      // sbeehive
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_beehive, b_pbeehive},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {4,                      // svee
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_vee, b_pvee},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {5,                      // s_bone6
     &thingbone6,
     &thingbone6,
     {0, 0, 0, 0},
     {b_bone6, b_pbone6},
     {0, 0},
     false, false,
     id_bit_table_bone6,
     {"a6 6b@76f c@7e6 6d@",
      "ea@5f@5c@db@"}},
   {5,                      // s_short6
    &thingshort6,
    &thingshort6,
    {0, 0, 0, 0},
    {b_short6, b_pshort6},
    {0, 0},
    false, false,
    id_bit_table_short6,
    {"5 b@a  c@f  d@5 e@",
     "5 fa@e 6 b@5 dc@"}},
   {4,                      // s1x5
    &thing1x5,
    &thing1x5,
    {0, 0, 0, 0},
    {b_1x5, b_5x1},
    {5, 1},
    false, false,
    (const id_bit_table *) 0,
    {"a  b  c  e  d@",
     "a@b@c@e@d@"}},
   {5,                      // s1x6
    &thing1x6,
    &thing1x6,
    {0, 0, 0, 0},
    {b_1x6, b_6x1},
    {6, 1},
    false, false,
    id_bit_table_1x6,
    {"a  b  c  f  e  d@",
     "a@b@c@f@e@d@"}},
   {5,                      // s2x3
    &thing2x3,
    &thing2x3,
    {0, 0, 0, 0},
    {b_2x3, b_3x2},
    {3, 2},
    false, false,
    id_bit_table_2x3,
    {"a  b  c@f  e  d@",
     "f  a@e  b@d  c@"}},
   {5,                      // s_1x2dmd
    &thing1x2dmd,
    &thing1x2dmd,
    {0, 0, 0, 0},
    {b_1x2dmd, b_p1x2dmd},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6  6 c@7a  b 6 e  d@76  6 f@",
     "5 a@@5 b@@f  c@@5 e@@5 d@"}},
   {5,                      // s_2x1dmd
    &thing2x1dmd,
    &thing2x1dmd,
    {0, 0, 0, 0},
    {b_2x1dmd, b_p2x1dmd},
    {0, 0},
    false, false,
    id_bit_table_2x1dmd,
    {"6  5 c@@a  b  e  d@@6  5 f",
     "6  a@@6  b@7f  6  c@76  e@@6  d"}},
   {5,                      // s_wingedstar6
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_wingedstar6, b_pwingedstar6},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6  5 c@7a  b  e  d@76  5 f@",
     "5 a@@5 b@f  c@5 e@@5 d@"}},
   {5,                     // s1x3p1dmd
    &thing1x3p1dmd,
    &thing1x3p1dmd,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {5,                     // s3p1x1dmd
    &thing3p1x1dmd,
    &thing3p1x1dmd,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                      // s_qtag
    &thingqtag,
    &nicethingqtag,
    {0x33, 0xDD, 0x11, 0},
    {b_qtag, b_pqtag},
    {4, 0},
    false, false,
    id_bit_table_qtag,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                      // s_bone
    &thingbone,
    &thingbone,
    {0x33, 0, 0x11, 0},
    {b_bone, b_pbone},
    {0, 0},
    false, false,
    id_bit_table_bone,
    {"a6 6 6 6b@76g h d c@7f6 6 6 6e",
     "fa@5g@5h@5d@5c@eb"}},
   {7,                      // s1x8
    &thing1x8,
    &thing1x8,
    {0x33, 0x77, 0x22, 0},
    {b_1x8, b_8x1},
    {8, 1},
    false, false,
    id_bit_table_1x8,
    {"a b d c g h f e",
     "a@b@d@c@g@h@f@e"}},
   {7,                      // slittlestars
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                      // s_2stars
    &thing2stars,
    &nicething2stars,
    {0, 0, 0, 0},
    {b_2stars, b_p2stars},
    {0, 0},
    false, false,
    id_bit_table_2stars,
    {"  8a6 b@g8hd8c@  8f6 e",
     "9g@f8a@9h@@9d@e8b@9c"}},
   {7,                      // s1x3dmd
    &thing1x3dmd,
    &thing1x3dmd,
    {0x33, 0x77, 0x11, 0},
    {b_1x3dmd, b_p1x3dmd},
    {0, 0},
    false, false,
    id_bit_table_1x3dmd,
    {"6 6 6 d@7a b c 6 g f e@76 6 6 h",
     " 5a@@ 5b@@ 5c@@ hd@@ 5g@@ 5f@@ 5e@"}},
   {7,                      // s3x1dmd
    &thing3x1dmd,
    &thing3x1dmd,
    {0x33, 0x77, 0x22, 0},
    {b_3x1dmd, b_p3x1dmd},
    {0, 0},
    false, false,
    id_bit_table_3x1dmd,
    {"6 6 9d@@a b c g f e@@6 6 9h",  // Not quite symmetrical, unfortunately.
     "6  a@@6  b@@6  c@7h  6  d@76  g@@6  f@@6  e"}},
   {7,                      // s_spindle
    &thingspindle,
    &thingspindle,
    {0, 0xEE, 0x44, 0},
    {b_spindle, b_pspindle},
    {0, 0},
    false, false,
    id_bit_table_spindle,
    {"6a b c@7h6 6 6d@76g f e",
     "5h@ga@fb@ec@5d"}},
   {7,                      // s_hrglass
    &thingglass,
    &nicethingglass,
    {0x33, 0xDD, 0x11, 0},
    {b_hrglass, b_phrglass},
    {0, 0},
    false, false,
    id_bit_table_hrglass,
    {"9a5b@56d@7g66c@756h@9f5e",
     "6 g@7f 6 a@5 hd@e 6 b@76 c"}},
   {7,                      // s_dhrglass
    &thingdglass,
    &thingdglass,
    {0x33, 0, 0x11, 0},
    {b_dhrglass, b_pdhrglass},
    {0, 0},
    false, false,
    id_bit_table_dhrglass,
    {"a 6 d 6 b@76 g 6 c@7f 6 h 6 e",
     "fa@@5g@@hd@@5c@@eb"}},
   {11,                     // s_hyperglass
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    true, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                      // s_crosswave
    &thingxwv,
    &thingxwv,
    {0x33, 0x77, 0x11, 0x55},
    {b_crosswave, b_pcrosswave},
    {0, 0},
    false, false,
    id_bit_table_crosswave,
    {"66  c@66  d@7ab  6  fe@766  h@66  g",
     "65a@65b@@ghdc@@65f@65e"}},
   {7,                      // s2x4
    &thing2x4,
    &thing2x4,
    {0x66, 0, 0, 0},
    {b_2x4, b_4x2},
    {4, 2},
    false, false,
    id_bit_table_2x4,
    {"a  b  c  d@@h  g  f  e",
     "h  a@@g  b@@f  c@@e  d"}},
   {9,                      // s2x5
    &thing2x5,
    &thing2x5,
    {0, 0, 0, 0},
    {b_2x5, b_5x2},
    {5, 2},
    false, false,
    id_bit_table_2x5,
    {"a  b  c  d  e@@j  i  h  g  f",
     "j  a@@i  b@@h  c@@g  d@@f  e"}},
   {9,                      // sd2x5
    &thingd2x5,
    &thingd2x5,
    {0, 0, 0, 0},
    {b_d2x5, b_5x2},
    {4, 3},
    false, false,
    id_bit_table_d2x5,
    {"58c  j@7a6 6g@758d  i@7b6 6f@758e  h",
     "6  b  a@@5 e  d  c@@5 h  i  j@@6  f  g"}},
   {5,                      // s_ntrgl6cw
    &thingntrgl6cw,
    &thingntrgl6cw,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {4, 2},
    false, false,
    (const id_bit_table *) 0,
    {"9a5   b  c@@f  e5   d",
     "f@76  a@7e@@6  b@7d@76  c"}},
   {5,                      // s_ntrgl6ccw
    &thingntrgl6ccw,
    &thingntrgl6ccw,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {4, 2},
    false, false,
    (const id_bit_table *) 0,
    {"a  b5   c@@9f5   e  d",
     "6  a@7f@76  b@@e@76  c@7d"}},
   {5,                      // s_nftrgl6cw
    &thingnftrgl6cw,
    &thingnftrgl6cw,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {4, 3},
    false, false,
    (const id_bit_table *) 0,
    {"5 a@@f e b c@@5 6 6 d",
     "6  f@76  6  a@76  e@@6  b@7d@76  c"}},
   {5,                      // s_nftrgl6ccw
    &thingnftrgl6ccw,
    &thingnftrgl6ccw,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {4, 3},
    false, false,
    (const id_bit_table *) 0,
    {"5 6 6 c@@a b e d@@5 f",
     "6  a@7f@76  b@@6  e@76  6  c@76  d"}},
   {7,                      // s_ntrglcw
    &thingntrglcw,
    &thingntrglcw,
    {0, 0, 0x44, 0},
    {b_nothing, b_nothing},
    {5, 2},
    false, false,
    (const id_bit_table *) 0,
    {"9a5   b  c  d@@h  g  f5   e",
     "h@76  a@7g@@f  b@@6  c@7e@76  d"}},
   {7,                      // s_ntrglccw
    &thingntrglccw,
    &thingntrglccw,
    {0, 0, 0x22, 0},
    {b_nothing, b_nothing},
    {5, 2},
    false, false,
    (const id_bit_table *) 0,
    {"a  b  c5   d@@9h5   g  f  e",
     "6  a@7h@76  b@@g  c@@f@76  d@7e"}},
   {7,                      // s_nptrglcw
    &thingnptrglcw,
    &thingnptrglcw,
    {0, 0, 0x44, 0},
    {b_nothing, b_nothing},
    {6, 2},
    false, false,
    id_bit_table_nptrglcw,
    {"9a5   6  6  c  d@7965   b  f@7h  g  65  6   e",
     "h@76a@7g@@5b@@5f@@6c@7e@76d"}},
   {7,                      // s_nptrglccw
    &thingnptrglccw,
    &thingnptrglccw,
    {0, 0, 0x22, 0},
    {b_nothing, b_nothing},
    {6, 2},
    false, false,
    id_bit_table_nptrglccw,
    {"a  b  65  6   d@7965   c  g@79h5   6  6  f  e",
     "6a@7h@76b@@5c@@5g@@f@76d@7e"}},
   {7,                      // s_nxtrglcw
    &thingnxtrglcw,
    &thingnxtrglcw,
    {0, 0xEE, 0, 0},
    {b_nothing, b_nothing},
    {6, 2},
    false, false,
    id_bit_table_nxtrglcw,
    {"6  9a5   b  c@7h  6  656  6  d@76  g  f5   e",  // Not quite symmetrical, unfortunately.
     "5h@@g@76a@7f@@6b@7e@76c@@5d"}},
   {7,                      // s_nxtrglccw
    &thingnxtrglccw,
    &thingnxtrglccw,
    {0, 0xEE, 0, 0},
    {b_nothing, b_nothing},
    {6, 2},
    false, false,
    id_bit_table_nxtrglccw,
    {"6  a  b5   c@7h  6  656  6  d@76  9g5   f  e",  // Not quite symmetrical, unfortunately.
     "5h@@6a@7g@76b@@f@76c@7e@@5d"}},
   {7,                      // spgdmdcw
    &thingspgdmdcw,
    &thingspgdmdcw,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {4, 0},
    false, false,
    (const id_bit_table *) 0,
    {"5 a@@g h5 b@@5 f5 d c@@5 66 e",
     "66  g@76f  6  a@766  h@@6  d@7e  6  b@76  c"}},
   {7,                      // spgdmdccw
    &thingspgdmdccw,
    &thingspgdmdccw,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {4, 0},
    false, false,
    (const id_bit_table *) 0,
    {"5 66 b@@5 a5 d c@@g h5 e@@5 f",
     "6  g@7f  6  a@76  h@@66  d@76e  6  b@766  c"}},
   {9,                      // s1x4dmd
    &thing1x4dmd,
    &thing1x4dmd,
    {0, 0, 0, 0},
    {b_1x4dmd, b_p1x4dmd},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6 6 6 6 e@7a b c d 6 i h g f@76 6 6 6 j",
     " 5a@@ 5b@@ 5c@@ 5d@@ je@@ 5i@@ 5h@@ 5g@@ 5f@"}},
   {9,                      // swqtag
    &thingwqtag,
    &nicethingwqtag,
    {0, 0, 0, 0},
    {b_wqtag, b_pwqtag},
    {6, 0},
    false, false,
    id_bit_table_wqtag,
    {"6 5 a6 b@@h i j e d c@@6 5 g6 f",
     "6  h@@6  i@7g  6  a@76  j@@6  e@7f  6  b@76  d@@6  c"}},
   {9,                      // sdeep2x1dmd
    &thingdeep2x1dmd,
    &thingdeep2x1dmd,
    {0, 0, 0, 0},
    {b_deep2x1dmd, b_pdeep2x1dmd},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"65c@@abde@@jigf@@65h",
     "6ja@@6ib@7h6 6c@76gd@@6fe"}},
   {9,                      // swhrglass
    &thingwglass,
    &nicethingwglass,
    {0, 0, 0, 0},
    {b_whrglass, b_pwhrglass},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6 9a5b@6 56e@7h i66d c@76 56j@6 9g5f",
     "6 h@6 i@7g 6 a@5 je@f 6 b@76 d@6 c"}},
   {7,                      //s_rigger
    &thingrigger,
    &thingrigger,
    {0xCC, 0xDD, 0, 0},
    {b_rigger, b_prigger},
    {0, 0},
    false, false,
    id_bit_table_rigger,
    {"66a b@7gh6 6dc@766f e",
     "5g@5h@fa@eb@5d@5c"}},
   {8,                      // s3x3
    &thing3x3,
    &thing3x3,
    {0, 0, 0, 0},
    {b_3x3, b_3x3},
    {3, 3},
    true, false,
    (const id_bit_table *) 0,
    {"a  b  c@h  i  d@g  f  e",
     (Cstring) 0}},
   {11,                     // s3x4
    &thing3x4,
    &thing3x4,
    {0, 0, 0x041, 0},         // Only used if occupied as "H"
    {b_3x4, b_4x3},
    {4, 3},
    false, false,
    id_bit_table_3x4,
    {"a  b  c  d@@k  l  f  e@@j  i  h  g",
     "j  k  a@@i  l  b@@h  f  c@@g  e  d"}},
   {11,                     // s2x6
    &thing2x6,
    &thing2x6,
    {0, 0, 0, 0},
    {b_2x6, b_6x2},
    {6, 2},
    false, false,
    id_bit_table_2x6,
    {"a  b  c  d  e  f@@l  k  j  i  h  g",
     "l  a@@k  b@@j  c@@i  d@@h  e@@g  f"}},
   {13,                     // s2x7
    &thing2x7,
    &thing2x7,
    {0, 0, 0, 0},
    {b_2x7, b_7x2},
    {7, 2},
    false, false,
    id_bit_table_2x7,
    {"a  b  c  d  e  f  g@@n  m  l  k  j  i  h",
     "n  a@@m  b@@l  c@@k  d@@j  e@@i  f@@h  g"}},
   {13,                     // sd2x7
    &thingd2x7,
    &thingd2x7,
    {0, 0, 0, 0},
    {b_d2x7, b_7x2},
    {6, 3},
    false, false,
    id_bit_table_d2x7,
    {"6 58c  l@7a b6 6f g@76 58d  k@7n m6 6i h@76 58e  j",
     "6  n  a@@6  m  b@@5 e  d  c@@5 j  k  l@@6  i  f@@6  h  g"}},
   {17,                     // s2x9
    &thing2x9,
    &thing2x9,
    {0, 0, 0, 0},
    {b_2x9, b_9x2},
    {9, 2},
    false, false,
    id_bit_table_2x9,
    {"a  b  c  d  e  f  g  h  i@@r  q  p  o  n  m  l  k  j",
     "r  a@@q  b@@p  c@@o  d@@n  e@@m  f@@l  g@@k  h@@j  i"}},
   {11,                     // d3x4
    &thingd3x4,
    &thingd3x4,
    {0, 0, 0, 0},
    {b_d3x4, b_d4x3},
    {5, 3},
    false, false,
    id_bit_table_d3x4,
    {"a6 6 6e@758b  c  d@7l6 6 6f@758j  i  h@7k6 6 6g",
     "k  l  a@@5 j  b@@5 i  c@@5 h  d@@g  f  e"}},
   {19,                     // sd4x5
    &thingd4x5,
    &thingd4x5,
    {0, 0, 0, 0},
    {b_d4x5, b_d5x4},
    {7, 5},
    false, false,
    (const id_bit_table *) 0,
    {"a 6 6 6 6 6 o@b 6 6 6 6 6 n@76 f g h i j@7c 6 6 6 6 6 m@76 t s r q p@7d 6 6 6 6 6 l@e 6 6 6 6 6 k",
     "e d c b a@@56 t f@@56 s g@@56 r h@@56 q i@@56 p j@@k l m n o"}},

   {9,                      // s_spindle12
    &thingspindle12,
    &thingspindle12,
    {0, 0, 0, 0},
    {b_spindle12, b_pspindle12},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6 6a b c@7i j6 6 6e d@76 6h g f",
     "5i@5j@ha@gb@fc@5e@5d"}},

   {15,                     // s1p5x8
    (const coordrec *) 0,   // Code in anchor_someone_and_move requires that this
    (const coordrec *) 0,   // setup looks a lot like a 2x8.
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"a  b  c  d  e  f  g  h@p  o  n  m  l  k  j  i",
     "pa@@ob@@nc@@md@@le@@kf@@jg@@ih"}},
   {7,                      // s1p5x4
    (const coordrec *) 0,   // Code in anchor_someone_and_move requires that this
    (const coordrec *) 0,   // setup looks a lot like a 2x4.
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"a  b  c  d@h  g  f  e",
     "ha@@gb@@fc@@ed"}},
   {15,                     // sfudgy2x6l
    &thing1p5x8,
    &thing1p5x8,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"a  b  c  d  e  f  g  h@p  o  n  m  l  k  j  i",
     "pa@@ob@@nc@@md@@le@@kf@@jg@@ih"}},
   {15,                     // sfudgy2x6r
    &thing1p5x8,
    &thing1p5x8,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"a  b  c  d  e  f  g  h@p  o  n  m  l  k  j  i",
     "pa@@ob@@nc@@md@@le@@kf@@jg@@ih"}},
   {7,                      // sfudgy2x3l
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"a  b  c  d@h  g  f  e",
     "ha@@gb@@fc@@ed"}},
   {7,                      // sfudgy2x3r
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"a  b  c  d@h  g  f  e",
     "ha@@gb@@fc@@ed"}},
   {15,                     // s2x8
    &thing2x8,
    &thing2x8,
    {0, 0, 0, 0},
    {b_2x8, b_8x2},
    {8, 2},
    false, false,
    id_bit_table_2x8,
    {"a  b  c  d  e  f  g  h@@p  o  n  m  l  k  j  i",
     "p  a@@o  b@@n  c@@m  d@@l  e@@k  f@@j  g@@i  h"}},
   {15,                     // s4x4
    &thing4x4,
    &thing4x4,
    {0x1111, 0, 0, 0},        // Only used if occupied as butterfly.
    {b_4x4, b_4x4},
    {4, 4},
    true, false,
    id_bit_table_4x4,
    {"m  n  o  a@@k  p  d  b@@j  l  h  c@@i  g  f  e",
     (Cstring) 0}},
   {9,                      // s1x10
    &thing1x10,
    &thing1x10,
    {0, 0, 0, 0},
    {b_1x10, b_10x1},
    {10, 1},
    false, false,
    id_bit_table_1x10,
    {"a b c d e j i h g f",
     "a@b@c@d@e@j@i@h@g@f"}},
   {11,                     // s1x12
    &thing1x12,
    &thing1x12,
    {0, 0, 0x041, 0},
    {b_1x12, b_12x1},
    {12, 1},
    false, false,
    (const id_bit_table *) 0,
    {"a b c d e f l k j i h g",
     "a@b@c@d@e@f@l@k@j@i@h@g"}},
   {13,                     // s1x14
    &thing1x14,
    &thing1x14,
    {0, 0, 0, 0},
    {b_1x14, b_14x1},
    {14, 1},
    false, false,
    (const id_bit_table *) 0,
    {"abcdefgnmlkjih",
     "a@b@c@d@e@f@g@n@m@l@k@j@i@h"}},
   {15,                     // s1x16
    &thing1x16,
    &thing1x16,
    {0, 0, 0, 0},
    {b_1x16, b_16x1},
    {16, 1},
    false, false,
    (const id_bit_table *) 0,
    {"abcdefghponmlkji",
     "a@b@c@d@e@f@g@h@p@o@n@m@l@k@j@i"}},
   {15,                     // s_c1phan
    &thingphan,
    &nicethingphan,
    {0, 0, 0, 0},
    {b_c1phan, b_c1phan},
    {0, 0},
    true, false,
    id_bit_table_c1phan,
    {(Cstring) 0,
     (Cstring) 0}},
   {23,                     // s_bigblob
    &thingblob,
    &thingblob,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    true, false,
    (const id_bit_table *) 0,
    {"6  6  a  b@@6  v  w  c  d@@t  u  x  f  e  g@@s  q  r  l  i  h@@6  p  o  k  j@@6  6  n  m",
     (Cstring) 0}},
   {7,                      // s_ptpd
    &thingptpd,
    &nicethingptpd,
    {0, 0x77, 0x22, 0},
    {b_ptpd, b_pptpd},
    {0, 0},
    false, false,
    id_bit_table_ptpd,
    {"6b6   6h@7a6c   g6e@76d6   6f",
     "5a@@db@@5c@@5g@@fh@@5e"}},
   {11,                     // s3dmd
    &thing3dmd,
    &thing3dmd,
    {00303, 0, 00101, 0},        // Only used for certain occupations.
    {b_3dmd, b_p3dmd},
    {0, 0},
    false, false,
    id_bit_table_3dmd,
    {"5 a 6 b 6 c@@j k l f e d@@5 i 6 h 6 g",
     "6  j@7i  6  a@76  k@@6  l@7h  6  b@76  f@@6  e@7g  6  c@76  d"}},
   {15,                     // s4dmd
    &thing4dmd,
    &thing4dmd,
    {0, 0, 0, 0},
    {b_4dmd, b_p4dmd},
    {0, 0},
    false, false,
    id_bit_table_4dmd,
    {"5 a 6 b 6 c 6 d@@m n o p h g f e@@5 l 6 k 6 j 6 i",
     "6  m@7l  6  a@76  n@@6  o@7k  6  b@76  p@@6  h@7j  6  c@76  g@@6  f@7i  6  d@76  e"}},
   {11,                     // s3ptpd
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_3ptpd, b_p3ptpd},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6a696b696c@7j6k9l6f9e6d@76i696h696g",
     "5j@@ia@@5k@@5l@@hb@@5f@@5e@@gc@@5d"}},
   {15,                     // s4ptpd
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_4ptpd, b_p4ptpd},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6a696b696c696d@7m6n9o6p9h6g9f6e@76l696k696j696i",
     "5m@@la@@5n@@5o@@kb@@5p@@5h@@jc@@5g@@5f@@id@@5e"}},
   {7,                     // s_trngl8
    &thingtrngl8,
    &thingtrngl8,
    {0, 0, 0, 0},
    {b_trngl8, b_ptrngl8},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // s1x4p2dmd
    &thing1x4p2dmd,
    &thing1x4p2dmd,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // s4p2x1dmd
    &thing4p2x1dmd,
    &thing4p2x1dmd,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    id_bit_table_s4p2x1dmd,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // splinepdmd
    &thingplinepdmd,
    &thingplinepdmd,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // splinedmd
    &thingplinedmd,
    &thingplinedmd,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    id_bit_table_splinedmd,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // slinepdmd
    &thinglinepdmd,
    &thinglinepdmd,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // slinedmd
    &thinglinedmd,
    &thinglinedmd,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // slinebox
    &thinglinebox,
    &thinglinebox,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // sline2box
    &thingline2box,
    &thingline2box,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    id_bit_table_sline2box,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // sline6box
    &thingline6box,
    &thingline6box,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    id_bit_table_sline6box,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // sdbltrngl4
    &thingdbltrngl4,
    &thingdbltrngl4,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // sboxdmd
    &thingboxdmd,
    &thingboxdmd,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // sboxpdmd
    &thingboxpdmd,
    &thingboxpdmd,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                     // sdmdpdmd
    &thingdmdpdmd,
    &thingdmdpdmd,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, true,
    id_bit_table_dmdpdmd,
    {(Cstring) 0,
     (Cstring) 0}},
   {11,                     // s_hsqtag
    &thinghsqtag,
    &thinghsqtag,
    {0, 0, 00101, 00303},
    {b_hsqtag, b_phsqtag},
    {0, 0},
    false, false,
    id_bit_table_s_hsqtag,
    {"a6 6j@76 5e@7b6 6i@76l f@7c6 6h@76 5k@7d6 6g",
     "dcba@65l@75k6e@765f@ghij"}},
   {11,                     // s_dmdlndmd
    &thingdmdlndmd,
    &thingdmdlndmd,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    id_bit_table_sdmdlndmd,
    {"66 b@75 a6 6d@766 c@7k l6f e@766 i@75 j6 6g@766 h",
     "65k@75j6a@765l@@hicb@@65f@75g6d@765e"}},
   {15,                     // s_hqtag
    &thinghqtag,
    &nicethinghqtag,
    {0, 0, 0, 0},
    {b_hqtag, b_phqtag},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"a6 6 6 6l@76 5e 6 f@7b6 6 6 6k@76o p h g@7c6 6 6 6j@76 5n 6 m@7d6 6 6 6i",
     "dcba@65o@75n6e@765p@65h@75m6f@765g@ijkl"}},
   {7,                      // s_wingedstar
    &thingwingstar,
    &thingwingstar,
    {0x33, 0x77, 0, 0},
    {b_wingedstar, b_pwingedstar},
    {0, 0},
    false, false,
    id_bit_table_swingedstar,
    {"665   d@a b c  g f e@665   h",
     "9a@@9b@@9c@h5d@9g@@9f@@9e"}},
   {11,                     // s_wingedstar12
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"665   d6   f@a b c  e k  i h g@665   l6   j",
     "9a@@9b@@9c@l5d@9e@9k@j5f@9i@@9h@@9g"}},
   {15,                     // s_wingedstar16
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"665   d6   h6   m@a b c  f g  o n  k j i@665   e6   p6   l",
     "9a@@9b@@9c@e5d@9f@9g@p5h@9o@9n@l5m@9k@@9j@@9i"}},
   {9,                      // s_barredstar
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {" a 66 c@ 65 b@i j  e d@ 65 g@ h 66 f",
     "h  i  a@@6  j@5 g  b@6  e@@f  d  c"}},
   {13,                     // s_barredstar12
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {" a6  6 6  6d@6 5 b 6  c@l m  n g  f e@6 5 j 6  i@ k6  6 6  6h",
     "k  l  a@6  m@5 j  b@6  n@@6  g@5 i  c@6  f@@h  e  d"}},
   {17,                     // s_barredstar16
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {" a6  6 6  6 6  6e@6 5 b 6  c 6  d@o p  q r  i h  g f@6 5 m 6  l 6  k@ n6  6 6  6 6  6j",
     "n  o  a@6  p@5 m  b@6  q@6  r@5 l  c@6  i@@6  h@5 k  d@6  g@@j  f  e"}},
   {7,                      // s_galaxy
    &thinggal,
    &thinggal,
    {0x55, 0, 0, 0},
    {b_galaxy, b_galaxy},
    {0, 0},
    true, false,
    id_bit_table_gal,
    {"68c@58bd@7a688e@758hf@68g",
     (Cstring) 0}},
   {11,                     // sbigh
    &thingbigh,
    &thingbigh,
    {00303, 0, 00101, 0},        // Only used for certain occupations.
    {b_bigh, b_pbigh},
    {0, 0},
    false, false,
    id_bit_table_bigh,
    {"a6666   j@b6666   i@76e f l k@7c6666   h@d6666   g",
     "dcba@65e@65f@65l@65k@ghij"}},
   {11,                     // sbigx
    &thingbigx,
    &thingbigx,
    {0, 0, 0x041, 0},
    {b_bigx, b_pbigx},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6666e@6666f@7abcd6jihg@76666l@6666k",
     "65a@65b@65c@65d@klfe@65j@65i@65h@65g"}},
   {17,                     // s3x6
    &thing3x6,
    &thing3x6,
    {0, 0, 0, 0},
    {b_3x6, b_6x3},
    {6, 3},
    false, false,
    id_bit_table_3x6,
    {"a  b  c  d  e  f@@p  q  r  i  h  g@@o  n  m  l  k  j",
     "o  p  a@@n  q  b@@m  r  c@@l  i  d@@k  h  e@@j  g  f"}},
   {23,                     // s3x8
    &thing3x8,
    &thing3x8,
    {0, 0, 0, 0},
    {b_3x8, b_8x3},
    {8, 3},
    false, false,
    (const id_bit_table *) 0,
    {"a  b  c  d  e  f  g  h@@u  v  w  x  l  k  j  i@@t  s  r  q  p  o  n  m",
     "t  u  a@@s  v  b@@r  w  c@@q  x  d@@p  l  e@@o  k  f@@n  j  g@@m  i  h"}},
   {19,                     // s4x5
    &thing4x5,
    &thing4x5,
    {0, 0, 0, 0},
    {b_4x5, b_5x4},
    {5, 4},
    false, false,
    id_bit_table_4x5,
    {"a  b  c  d  e@@j  i  h  g  f@@p  q  r  s  t@@o  n  m  l  k",
     "o  p  j  a@@n  q  i  b@@m  r  h  c@@l  s  g  d@@k  t  f  e"}},
   {23,                     // s4x6
    &thing4x6,
    &thing4x6,
    {0, 0, 0, 0},
    {b_4x6, b_6x4},
    {6, 4},
    false, false,
    id_bit_table_4x6,
    {"a  b  c  d  e  f@@l  k  j  i  h  g@@s  t  u  v  w  x@@r  q  p  o  n  m",
     "r  s  l  a@@q  t  k  b@@p  u  j  c@@o  v  i  d@@n  w  h  e@@m  x  g  f"}},
   {19,                     // s2x10
    &thing2x10,
    &thing2x10,
    {0, 0, 0, 0},
    {b_2x10, b_10x2},
    {10, 2},
    false, false,
    (const id_bit_table *) 0,
    {"a  b  c  d  e  f  g  h  i  j@@t  s  r  q  p  o  n  m  l  k",
     "t  a@@s  b@@r  c@@q  d@@p  e@@o  f@@n  g@@m  h@@l  i@@k  j"}},
   {23,                     // s2x12
    &thing2x12,
    &thing2x12,
    {0, 0, 0, 0},
    {b_2x12, b_12x2},
    {12, 2},
    false, false,
    (const id_bit_table *) 0,
    {"a  b  c  d  e  f  g  h  i  j  k  l@@x  w  v  u  t  s  r  q  p  o  n  m",
     "x  a@@w  b@@v  c@@u  d@@t  e@@s  f@@r  g@@q  h@@p  i@@o  j@@n  k@@m  l"}},
   {11,                     // sdeepqtg
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_deepqtg, b_pdeepqtg},
    {4, 4},
    false, false,
    (const id_bit_table *) 0,
    {"5 a  6  b@f  e  d  c@@i  j  k  l@5 h  6  g",
     "6i  f@7h6  6a@76j  e@@6k  d@7g6  6b@76l  c"}},
   {15,                     // sdeepbigqtg
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_deepbigqtg, b_pdeepbigqtg},
    {4, 6},
    false, false,
    (const id_bit_table *) 0,
    {"5 a  6  b@5 c  6  d@h  g  f  e@@m  n  o  p@5 l  6  k@5 j  6  i",
     "6 6m  h@7j l6  6c a@76 6n  g@@6 6o  f@7i k6  6d b@6 6p  e"}},
   {11,                     // swiderigger
    &thingwiderigger,
    &thingwiderigger,
    {0, 0, 0, 0},
    {b_widerigger, b_pwiderigger},
    {8, 2},
    false, false,
    (const id_bit_table *) 0,
    {"66a b c d@7kl6 6 6 6fe@766j i h g",
     "5k@5l@ja@ib@hc@gd@5f@5e"}},
   {11,                     // srigger12
    &thingrigger12,
    &thingrigger12,
    {0, 0, 0, 0},
    {b_rigger12, b_prigger12},
    {9, 2},
    false, false,
    (const id_bit_table *) 0,
    {"666a b c@7jkl6 6 6fed@7666i h g",
     "5j@5k@5l@ia@hb@gc@5f@5e@5d"}},
   {11,                     // sdeepxwv
    &thingdeepxwv,
    &thingdeepxwv,
    {0, 0, 0, 0},
    {b_deepxwv, b_pdeepxwv},
    {6, 4},
    false, false,
    id_bit_table_deepxwv,
    {"66c l@@66d k@7ab6 6hg@766e j@@66f i",
     "65a@65b@fedc@ijkl@65h@65g@"}},
   {19,                     // s3oqtg
    &thing3oqtg,
    &thing3oqtg,
    {0, 0, 0, 0},
    {b_3oqtg, b_p3oqtg},
    {7, 4},
    false, false,
    (const id_bit_table *) 0,
    {"6  a  6  b  6  c@@r  s  t  g  f  e  d@@n  o  p  q  j  i  h@@6  m  6  l  6  k",
     "6  n  r@@m  o  s  a@@6  p  t@@l  q  b  g@@6  j  f@@k  i  e  c@@6  h  d"}},
   {7,                      // s_thar
    &thingthar,
    &thingthar,
    {0x55, 0, 0, 0},
    {b_thar, b_thar},
    {0, 0},
    true, false,
    id_bit_table_thar,
    {"66c@66d@ab6fe@66h@66g",
     (Cstring) 0}},
   {7,                      // s_alamo
    &thingalamo,
    &thingalamo,
    {0, 0, 0, 0},
    {b_alamo, b_alamo},
    {0, 0},
    true, false,
    (const id_bit_table *) 0,
    {"6ab@h66c@g66d@6fe",
     (Cstring) 0}},
   {31,                     // sx4dmd
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {31,                     // sx4dmdbone
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {15,                     // s_hyperbone
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {23,                     // s_hyper3x4
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {15,                     // s_tinyhyperbone
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {63,                     // s8x8
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {31,                     // sxequlize
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {11,                     // sx1x6
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {15,                     // sx1x8
    &thingx1x8,
    &thingx1x8,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    true, false,
    (const id_bit_table *) 0,
    {"6666e@6666f@6666g@6666h@abcd6lkji@6666p@6666o@6666n@6666m",
     (Cstring) 0}},
   {31,                     // sx1x16
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {15,                     // shypergal
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {-1,                     // shyper4x8a
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {8, 4},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {-1,                     // shyper4x8b
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {8, 4},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {-1,                     // shyper3x8
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {8, 3},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {31,                     // shyper2x16
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {16, 2},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {15,                     // sfat2x8
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {8, 4},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {15,                     // swide4x4
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {8, 4},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {7,                      // s_323
    &thing_323,
    &thing_323,
    {0, 0, 0x11, 0},
    {b_323, b_p323},
    {0, 0},
    false, false,
    id_bit_table_323,
    {"   a  b  c@@   5 h  d@@   g  f  e",
     "g6a@76h@7f6b@76d@7e6c"}},
   {9,                      // s_343
    &thing_343,
    &thing_343,
    {0, 0, 0, 0},
    {b_343, b_p343},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"   5 a  b  c@@   i  j  e  d@@   5 h  g  f",
     "6i@7h6a@76j@7g6b@76e@7f6c@76d"}},
   {9,                      // s_3223
    &thing_3223,
    &thing_3223,
    {0, 0, 0, 0},
    {b_3223, b_p3223},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"a  6  6  b@76  e  d@7h  6  6  c@76  i  j@7g  6  6  f",
     "g  h  a@@5 i  e@@5 j  d@@f  c  b"}},
   {11,                     // s_525
    &thing_525,
    &thing_525,
    {0, 0, 0, 0},
    {b_525, b_p525},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"   a  b  c  d  e@@   6  5 l  f@@   k  j  i  h  g",
     "k6a@@j6b@76l@7i6c@76f@7h6d@@g6e"}},
   {13,                     // s_545
    &thing_545,
    &thing_545,
    {0, 0, 0, 0},
    {b_545, b_p545},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"   a  b  c  d  e@@   5 m  n  g  f@@   l  k  j  i  h",
     "l6a@76m@7k6b@76n@7j6c@76g@7i6d@76f@7h6e"}},
   {13,                     // sh545
    &thing_h545,
    &thing_h545,
    {0, 0, 0, 0},
    {bh545, bhp545},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"   a  b  c  d  e@@   m  5 n  g 5  f@@   l  k  j  i  h",
     "lma@@k6b@76n@7j6c@76g@7i6d@@hfe"}},
   {11,                     // s_23232
    &thing23232,
    &thing23232,
    {0, 0, 00404, 0},
    {b_23232, b_p23232},
    {0, 0},
    false, false,
    id_bit_table_23232,
    {"6a6b@7c6d6e@76l6f@7k6j6i@76h6g",
     "5 k  c@@h  l  a@@5 j  d@@g  f  b@@5 i  e"}},
   {11,                     // s_3mdmd
    &thing3mdmd,
    &thing3mdmd,
    {0, 0, 0, 0},
    {b_3mdmd, b_p3mdmd},
    {0, 0},
    false, false,
    id_bit_table_3mdmd,
    {"5 a6666     c@7666   b@7j k l6  f e d@7666   h@75 i6666     g",
     "6  j@7i6    a@76  k@@6  l@@5  hb@@6  f@@6  e@7g6    c@76  d"}},
   {11,                     // s_3mptpd
    &thing3mptpd,
    &thing3mptpd,
    {0, 0, 0, 0},
    {b_3mptpd, b_p3mptpd},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6665    b@76 a6666     c@7j6  k l f e6  d@76 i6666     g@76665    h",
     "6  j@@5  ia@@6  k@@6  l@7h6    b@76  f@@6  e@@5  gc@@6  d"}},
   {15,                     // s_4mdmd
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_4mdmd, b_p4mdmd},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"5 a6666666       d@7666   b66   c@7m n o 6 p h 6 g f e@7666   k66   j@75 l6666666       i",
     "6  m@7l  6  a@76  n@@6  o@@5  kb@@6  p@@6  h@@5  jc@@6  g@@6  f@7i  6  d@76  e"}},
   {15,                     // s_4mptpd
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_4mptpd, b_p4mptpd},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6 6 6 5 b 6 c@76 a 6 6 6 6 6 6 d@7m 6 n o p h g f 6 e@76 l 6 6 6 6 6 6 i@76 6 6 5 k 6 j",
     "6  m@@5  la@@6  n@@6  o@7k  6  b@76  p@@6  h@7j  6  c@76  g@@6  f@@5  id@@6  e"}},
   {15,                     // sbigbigh
    &thingbigbigh,
    &thingbigbigh,
    {0, 0, 0, 0},
    {b_bigbigh, b_pbigbigh},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"a66666666l@b66666666k@76efghponm@7c66666666j@d66666666i",
     "dcba@65e@65f@65g@65h@65p@65o@65n@65m@ijkl"}},
   {15,                     // sbigbigx
    &thingbigbigx,
    &thingbigbigx,
    {0, 0, 0, 0},
    {b_bigbigx, b_pbigbigx},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6666e p@6666f o@abcd6 6lkji@76666g n@6666h m",
     "65a@65b@65c@65d@hgfe@mnop@65l@65k@65j@65i"}},
   {11,                     // sbigrig
    &thingbigrig,
    &thingbigrig,
    {0, 0, 0, 0},
    {b_bigrig, b_pbigrig},
    {0, 0},
    false, false,
    id_bit_table_bigrig,
    {"6666e f@7abcd6 6jihg@76666l k",
     "5a@5b@5c@5d@le@kf@5j@5i@5h@5g"}},
   {11,                     // sbighrgl
    &thingbigglass,
    &nicethingbigglass,
    {0, 0, 0x104, 0},
    {b_bighrgl, b_pbighrgl},
    {0, 0},
    false, false,
    id_bit_table_bighrgl,
    {"6 6 c@7a b 6 e f@6 5 jd@l k 6 h g@76 6 i",
     "9l5a@@9k5b@56j@7i66c@756d@9h5e@@9g5f"}},
   {11,                     // sbigdhrgl
    &thingbigdglass,
    &thingbigdglass,
    {0, 0, 0x104, 0},
    {b_bigdhrgl, b_pbigdhrgl},
    {0, 0},
    false, false,
    id_bit_table_bigdhrgl,
    {"a  b 6 c 6 e  f@6  6 j 6 d@l  k 6 i 6 h  g",
     "l  a@@k  b@@5 j@@i  c@@5 d@@h  e@@g  f"}},
   {11,                     // sbigbone
    &thingbigbone,
    &thingbigbone,
    {0, 0, 0x104, 0},
    {b_bigbone, b_pbigbone},
    {0, 0},
    false, false,
    id_bit_table_bigbone,
    {"a  b6666   e  f@766  c d j i@7l  k6666   h  g",
     "la@kb@5c@5d@5j@5i@he@gf"}},
   {11,                     // sdblbone6
    &thingdblbone6,
    &thingdblbone6,
    {0, 0, 0, 0},
    {b_dblbone6, b_pdblbone6},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"a6 6b j6 6k@76f c6 6i l@7e6 6d h6 6g",
     "ea@5f@5c@db@hj@5i@5l@gk"}},
   {11,                     // sbigdmd
    &thingbigdmd,
    &thingbigdmd,
    {0, 0, 0x104, 0},
    {b_bigdmd, b_pbigdmd},
    {0, 0},
    false, false,
    id_bit_table_bigdmd,
    {"6 6  c@7a b  6  e f@76 6  d@@6 6  j@7l k  6  h g@76 6  i",
     "5 l6  a@5 k6  b@i j d c@5 h6  e@5 g6  f"}},
   {11,                     // sbigptpd
    &thingbigptpd,
    &thingbigptpd,
    {0, 0, 0x104, 0},
    {b_bigptpd, b_pbigptpd},
    {0, 0},
    false, false,
    id_bit_table_bigptpd,
    {"85f 8858 g@85e 8858 h@7c88d j88i@785b 8858 k@85a 8858 l",
     "65c@@abef@@65d@@65j@@lkhg@@65i"}},
   {11,                     // s5x1dmd
    &thing5x1dmd,
    &thing5x1dmd,
    {0, 0, 0, 0},
    {b_5x1dmd, b_p5x1dmd},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6 6 6 6 9f@@a b c d e k j i h g@@6 6 6 6 9l",  // Not quite symmetrical, unfortunately.
     "6  a@@6  b@@6  c@@6  d@@6  e@7l  6  f@76  k@@6  j@@6  i@@6  h@@6  g"}},
   {11,                     // s1x5dmd
    &thing1x5dmd,
    &thing1x5dmd,
    {0, 0, 0, 0},
    {b_1x5dmd, b_p1x5dmd},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6 6 6 6 6 f@7a b c d e 6 k j i h g@76 6 6 6 6 l",
     " 5a@@ 5b@@ 5c@@ 5d@@ 5e@@ lf@@ 5k@@ 5j@@ 5i@@ 5h@@ 5g@"}},
   {17,                     // sbig3dmd
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_big3dmd, b_pbig3dmd},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"5 a 6 b 6 c@@5 f 6 e 6 d@@p q r i h g@@5 m 6 n 6 o@@5 l 6 k 6 j",
     "6 6  p@7l m  6  f a@76 6  q@@6 6  r@7k n  6  e b@76 6  i@@6 6  h@7j o  6  d c@76 6  g"}},
   {23,                     // sbig4dmd
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_big4dmd, b_pbig4dmd},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"5 a 6 b 6 c 6 d@@5 h 6 g 6 f 6 e@@u v w x l k j i@@5 q 6 r 6 s 6 t@@5 p 6 o 6 n 6 m",
     "6 6  u@7p q  6  h a@76 6  v@@6 6  w@7o r  6  g b@76 6  x@@6 6  l@7n s  6  f c@76 6  k@@6 6  j@7m t  6  e d@76 6  i"}},
   {15,                     // sdblxwave
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_dblxwave, b_pdblxwave},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6 6c6 6 6 6o@6 6d6 6 6 6p@7a b6f e m n6j i@76 6h6 6 6 6l@6 6g6 6 6 6k",
     "65a@65b@ghdc@65f@65e@65m@65n@klpo@65j@65i"}},
   {15,                     // sdblspindle
    &thingdblspindle,
    &thingdblspindle,
    {0, 0, 0, 0},
    {b_dblspindle, b_pdblspindle},
    {0, 0},
    false, false,
    id_bit_table_dblspindle,
    {"6a b c6 6m n o@7h6 6 6d l6 6 6p@76g f e6 6k j i",
     "5h@ga@fb@ec@5d@5l@km@jn@io@5p"}},
   {15,                     // sdblbone
    &thingdblbone,
    &thingdblbone,
    {0, 0, 0, 0},
    {b_dblbone, b_pdblbone},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"a6 6 6 6b m6 6 6 6n@76g h d c6 6k l p o@7f6 6 6 6e j6 6 6 6i",
     "fa@5g@5h@5d@5c@eb@jm@5k@5l@5p@5o@in"}},
   {15,                     // sdblrig
    &thingdblrig,
    &thingdblrig,
    {0, 0, 0, 0},
    {b_dblrig, b_pdblrig},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {"6 6a b6 6 6 6m n@7g h6 6d c k l6 6p o@76 6f e6 6 6 6j i",
     "5g@5h@fa@eb@5d@5c@5k@5l@jm@in@5p@5o"}},
   {-1,                     // s_dead_concentric
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}},
   {-1,                     // s_normal_concentric
    (const coordrec *) 0,
    (const coordrec *) 0,
    {0, 0, 0, 0},
    {b_nothing, b_nothing},
    {0, 0},
    false, false,
    (const id_bit_table *) 0,
    {(Cstring) 0,
     (Cstring) 0}}};


// BEWARE!!  This list is keyed to the definition of "calldef_schema" in database.h .
const schema_attr schema_attrs[] = {
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_DETOUR | SCA_INV_SUP_ELWARN | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric
   {SCA_CENTRALCONC | SCA_CROSS | SCA_COPY_LYZER | SCA_SNAGOK | SCA_REMOVE_VERIFY,
    schema_concentric},                  // schema_cross_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_3x3k_concentric
   {SCA_CROSS | SCA_COPY_LYZER,
    schema_3x3k_concentric},             // schema_3x3k_cross_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_4x4k_concentric
   {SCA_CROSS | SCA_COPY_LYZER,
    schema_4x4k_concentric},             // schema_4x4k_cross_concentric
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_SPLITOK | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_single_concentric
   {SCA_CENTRALCONC | SCA_CROSS | SCA_COPY_LYZER | SCA_SNAGOK | SCA_REMOVE_VERIFY,
    schema_single_concentric},           // schema_single_cross_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_grand_single_concentric
   {SCA_CENTRALCONC | SCA_CROSS | SCA_COPY_LYZER,
    schema_grand_single_concentric},     // schema_grand_single_cross_concentric
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_SPLITOK | SCA_NO_OVERCAST,
    schema_nothing},                     // schema_single_concentric_together
   {SCA_CENTRALCONC | SCA_CROSS | SCA_SNAGOK | SCA_SPLITOK,
    schema_nothing},                     // schema_single_cross_concentric_together
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_maybe_matrix_single_concentric_together
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_maybe_single_concentric
   {SCA_CROSS,
    schema_nothing},                     // schema_maybe_single_cross_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_maybe_grand_single_concentric
   {SCA_CROSS,
    schema_nothing},                     // schema_maybe_grand_single_cross_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_maybe_special_single_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_maybe_special_trade_by
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_special_trade_by
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_grand_single_or_matrix_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_3x3_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_4x4_lines_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_4x4_cols_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_maybe_nxn_lines_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_maybe_nxn_cols_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_maybe_nxn_1331_lines_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_maybe_nxn_1331_cols_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_1331_concentric
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_1313_concentric
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_DETOUR | SCA_INV_SUP_ELWARN | SCA_NO_OVERCAST,
    schema_nothing},                     // schema_1221_concentric
   {0,
    schema_nothing},                     // schema_concentric_diamond_line
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_concentric_lines_z
   {0,
    schema_nothing},                     // schema_concentric_diamonds
   {SCA_CROSS | SCA_COPY_LYZER,
    schema_concentric_diamonds},         // schema_cross_concentric_diamonds
   {SCA_NO_OVERCAST,
    schema_nothing},                     // schema_concentric_zs
   {SCA_CROSS | SCA_COPY_LYZER,
    schema_concentric_zs},               // schema_cross_concentric_zs
   // *************************** Don't lots more need the "SCA_CENTRALCONC" bit?
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_or_diamond_line
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_DETOUR | SCA_INV_SUP_ELWARN | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_or_6_2_line
   {SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_6_2
   {SCA_CROSS | SCA_REMOVE_VERIFY,
    schema_concentric_6_2},              // schema_cross_concentric_6_2
   {SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_6_2_line
   {SCA_SNAGOK | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_2_6
   {SCA_CROSS | SCA_REMOVE_VERIFY,
    schema_concentric_2_6},              // schema_cross_concentric_2_6
   {SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_2_4
   {SCA_CROSS | SCA_REMOVE_VERIFY,
    schema_concentric_2_4},              // schema_cross_concentric_2_4
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_DETOUR | SCA_INV_SUP_ELWARN | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_2_4_or_normal
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_DETOUR | SCA_INV_SUP_ELWARN | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_4_2
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_DETOUR | SCA_INV_SUP_ELWARN | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_4_2_prefer_1x4
   {SCA_CENTRALCONC | SCA_CROSS | SCA_REMOVE_VERIFY,
    schema_concentric_4_2},              // schema_cross_concentric_4_2
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_DETOUR | SCA_INV_SUP_ELWARN | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_4_2_or_normal
   {0 | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_or_2_6
   {0 | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_or_6_2
   {0 | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_8_4
   {0 | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_big2_6
   {0 | SCA_REMOVE_VERIFY,
    schema_nothing},                     // schema_concentric_2_6_or_2_4
   {SCA_CROSS | SCA_REMOVE_VERIFY,
    schema_concentric_2_6_or_2_4},       // schema_cross_concentric_2_6_or_2_4
   {0,
    schema_nothing},                     // schema_concentric_innermost
   {0,
    schema_nothing},                     // schema_concentric_touch_by_1_of_3
   {SCA_CROSS | SCA_COPY_LYZER,
    schema_concentric_innermost},        // schema_cross_concentric_innermost
   {SCA_CROSS | SCA_COPY_LYZER,
    schema_concentric_touch_by_1_of_3},  // schema_cross_concentric_touch_by_1_of_3
   {0,
    schema_nothing},                     // schema_concentric_touch_by_2_of_3
   {0,
    schema_nothing},                     // schema_single_concentric_together_if_odd
   {SCA_CROSS | SCA_COPY_LYZER,
    schema_single_concentric_together_if_odd},  // schema_single_cross_concentric_together_if_odd
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_DETOUR | SCA_INV_SUP_ELWARN,
    schema_nothing},                     // schema_concentric_6p
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_DETOUR | SCA_INV_SUP_ELWARN,
    schema_nothing},                     // schema_concentric_6p_or_normal
   {SCA_CENTRALCONC | SCA_SNAGOK | SCA_DETOUR | SCA_INV_SUP_ELWARN,
    schema_nothing},                     // schema_concentric_6p_or_normal_or_2x6_2x3
   {SCA_CENTRALCONC | SCA_SNAGOK,
    schema_nothing},                     // schema_concentric_6p_or_sgltogether
   {SCA_CROSS | SCA_COPY_LYZER | SCA_SNAGOK,
    schema_concentric_6p_or_normal},     // schema_cross_concentric_6p_or_normal
   {0,
    schema_nothing},                     // schema_concentric_others
   {0,
    schema_nothing},                     // schema_concentric_6_2_tgl
   {0,
    schema_nothing},                     // schema_concentric_to_outer_diamond
   {0,
    schema_nothing},                     // schema_concentric_no31dwarn
   {0,
    schema_nothing},                     // schema_concentric_specialpromenade
   {0,
    schema_nothing},                     // schema_cross_concentric_specialpromenade
   {0,
    schema_nothing},                     // schema_conc_12
   {0,
    schema_nothing},                     // schema_conc_16
   {0,
    schema_nothing},                     // schema_conc_star
   {0,
    schema_nothing},                     // schema_conc_star12
   {0,
    schema_nothing},                     // schema_conc_star16
   {0,
    schema_nothing},                     // schema_conc_bar
   {0,
    schema_nothing},                     // schema_conc_bar12
   {0,
    schema_nothing},                     // schema_conc_bar16
   {SCA_SNAGOK | SCA_DETOUR | SCA_INV_SUP_ELWARN,
    schema_nothing},                     // schema_conc_o
   {0,
    schema_nothing},                     // schema_intermediate_diamond
   {0,
    schema_nothing},                     // schema_outside_diamond
   {0,
    schema_nothing},                     // schema_maybe_matrix_conc
   {0,
    schema_nothing},                     // schema_maybe_matrix_conc_star
   {0,
    schema_nothing},                     // schema_maybe_matrix_conc_bar
   {0,
    schema_nothing},                     // schema_checkpoint
   {0,
    schema_nothing},                     // schema_checkpoint_spots
   {SCA_SNAGOK,
    schema_nothing},                     // schema_checkpoint_mystic_ok
   {SCA_CROSS | SCA_COPY_LYZER,
    schema_checkpoint},                  // schema_cross_checkpoint
   {SCA_DETOUR,
    schema_nothing},                     // schema_rev_checkpoint
   {SCA_DETOUR,
    schema_nothing},                     // schema_rev_checkpoint_concept
   {0,
    schema_checkpoint},                  // schema_ckpt_star
   {0,
    schema_in_out_triple_dyp_squash},    // schema_maybe_in_out_triple_dyp_squash
   {SCA_CONC_REV_ORDER,
    schema_in_out_triple_dyp_squash},    // schema_in_out_triple_dyp_squash
   {0,
    schema_nothing},                     // schema_maybe_in_out_triple_squash
   {SCA_CONC_REV_ORDER,
    schema_in_out_triple},               // schema_in_out_triple_squash
   {SCA_CONC_REV_ORDER,
    schema_sgl_in_out_triple},           // schema_sgl_in_out_triple_squash
   {SCA_CONC_REV_ORDER,
    schema_3x3_in_out_triple},           // schema_3x3_in_out_triple_squash
   {SCA_CONC_REV_ORDER,
    schema_4x4_in_out_triple},           // schema_4x4_in_out_triple_squash
   {SCA_CONC_REV_ORDER,
    schema_nothing},                     // schema_in_out_triple
   {SCA_CONC_REV_ORDER,
    schema_nothing},                     // schema_sgl_in_out_triple
   {SCA_CONC_REV_ORDER,
    schema_nothing},                     // schema_3x3_in_out_triple
   {SCA_CONC_REV_ORDER,
    schema_nothing},                     // schema_4x4_in_out_triple
   {SCA_CONC_REV_ORDER,
    schema_nothing},                     // schema_in_out_quad
   {SCA_CONC_REV_ORDER,
    schema_nothing},                     // schema_in_out_12mquad
   {0,
    schema_nothing},                     // schema_in_out_triple_zcom
   {0,
    schema_nothing},                     // schema_in_out_center_triple_z
   {0,
    schema_nothing},                     // schema_select_leads
   {0,
    schema_nothing},                     // schema_select_headliners
   {0,
    schema_nothing},                     // schema_select_sideliners
   {SCA_SNAGOK | SCA_SPLITOK,
    schema_nothing},                     // schema_select_original_rims
   {SCA_SNAGOK | SCA_SPLITOK,
    schema_nothing},                     // schema_select_original_hubs
   {0,
    schema_nothing},                     // schema_select_those_facing_both_live
   {SCA_SNAGOK | SCA_SPLITOK,
    schema_nothing},                     // schema_select_ctr2
   {SCA_SNAGOK | SCA_SPLITOK,
    schema_nothing},                     // schema_select_ctr4
   {SCA_SNAGOK | SCA_SPLITOK,
    schema_nothing},                     // schema_select_ctr6
   {SCA_SNAGOK | SCA_SPLITOK,
    schema_nothing},                     // schema_select_who_can
   {SCA_SNAGOK | SCA_SPLITOK,
    schema_nothing},                     // schema_select_who_did
   {SCA_SNAGOK | SCA_SPLITOK,
    schema_nothing},                     // schema_select_who_didnt
   {0,
    schema_nothing},                     // schema_select_who_did_and_didnt
   {0,
    schema_nothing},                     // schema_lateral_6
   {0,
    schema_nothing},                     // schema_vertical_6
   {0,
    schema_nothing},                     // schema_intlk_lateral_6
   {0,
    schema_nothing},                     // schema_intlk_vertical_6
   {SCA_SNAGOK,
    schema_nothing},                     // schema_by_array
   {0,
    schema_nothing},                     // schema_nothing
   {0,
    schema_nothing},                     // schema_nothing_noroll
   {0,
    schema_nothing},                     // schema_nothing_other_elong
   {0,
    schema_nothing},                     // schema_matrix
   {0,
    schema_nothing},                     // schema_partner_matrix
   {0,
    schema_nothing},                     // schema_partner_partial_matrix
   {0,
    schema_nothing},                     // schema_roll
   {0,
    schema_nothing},                     // schema_recenter
   {SCA_SNAGOK,
    schema_nothing},                     // schema_sequential
   {0,
    schema_nothing},                     // schema_split_sequential
   {SCA_SNAGOK,
    schema_nothing},                     // schema_sequential_with_fraction
   {SCA_SNAGOK,
    schema_nothing},                     // schema_sequential_with_split_1x8_id
   {SCA_SNAGOK,
    schema_nothing},                     // schema_sequential_alternate
   {SCA_SNAGOK,
    schema_nothing}                      // schema_sequential_remainder
};


// BEWARE!!  This list is keyed to the definition of "meta_key_kind" in sd.h .
const uint32 meta_key_props[] = {
   MKP_RESTRAIN_1 | MKP_RESTRAIN_2,   // meta_key_random
   MKP_RESTRAIN_1 | MKP_RESTRAIN_2,   // meta_key_rev_random
   MKP_RESTRAIN_1 | MKP_RESTRAIN_2,   // meta_key_piecewise
   MKP_RESTRAIN_1,                    // meta_key_finish
   MKP_RESTRAIN_1,                    // meta_key_revorder
   MKP_RESTRAIN_1 | MKP_RESTRAIN_2,   // meta_key_roundtrip
   0,                                 // meta_key_like_a
   MKP_RESTRAIN_1 | MKP_RESTRAIN_2 | MKP_COMMA_NEXT,   // meta_key_finally
   MKP_RESTRAIN_1 | MKP_RESTRAIN_2 | MKP_COMMA_NEXT,   // meta_key_initially_and_finally
   MKP_RESTRAIN_1 | MKP_RESTRAIN_2 | MKP_COMMA_NEXT,   // meta_key_nth_part_work
   MKP_RESTRAIN_1 | MKP_RESTRAIN_2 | MKP_COMMA_NEXT,   // meta_key_first_frac_work
   0,                                 // meta_key_skip_nth_part
   0,                                 // meta_key_skip_last_part
   0,                                 // meta_key_shift_n
   MKP_RESTRAIN_1 | MKP_RESTRAIN_2,   // meta_key_echo
   MKP_RESTRAIN_1 | MKP_RESTRAIN_2,   // meta_key_rev_echo
   0};                                // meta_key_shift_half


/* In the print_strings tables below, characters have the following meanings:
   a-z  draw a person, this will be 4 characters wide in ASCII
   6    blank person space, whether in ASCII (4 blanks) or checkers
   5    1/2 person space (2 blanks if in ASCII)
   9    3/4 person space (3 blanks if in ASCII)
   8    1/2 person space with checkers, but only one blank (not 2) if ASCII
   @7   only 1/2 line feed if using checkers, must be careful not to overlap
*/

const ctr_end_mask_rec dead_masks = {0, 0, 0, 0};
const ctr_end_mask_rec masks_for_3x4 = {0xC3, 0, 0x041, 0};     // Only used if occupied as "H".
const ctr_end_mask_rec masks_for_3dmd_ctr2 = {0, 0, 00101, 0};  // Only if ctr 2 / outer 1x3's
const ctr_end_mask_rec masks_for_3dmd_ctr4 = {00303, 0, 0, 0};  // Only if ctr 1x4 / outer points
const ctr_end_mask_rec masks_for_bigh_ctr4 = {00303, 0, 0, 0};  // Only if ctr 1x4 / outer points
const ctr_end_mask_rec masks_for_4x4 = {0x1111, 0, 0, 0};       // Only used if occupied as butterfly



const clw3_thing clw3_table[] = {
   {s4x4, 0x8E8E, 0x8A8A, spcmap_3ri,  0, {12, 13, 14, 0, 8, 6,  5, 4, -1}},
   {s4x4, 0x8E8E, 0x8C8C, spcmap_3li,  0, {12, 13, 14, 0, 8, 6,  5, 4, -1}},
   {s4x4, 0x7171, 0x7070, spcmap_3ro,  0, {10, 15,  3, 1, 9, 11, 7, 2, -1}},
   {s4x4, 0x7171, 0x6161, spcmap_3lo,  0, {10, 15,  3, 1, 9, 11, 7, 2, -1}},
   {s4x4, 0xE8E8, 0xA8A8, spcmap_3ri,  1, {12, 13, 14, 0, 8, 6,  5, 4, -1}},
   {s4x4, 0xE8E8, 0xC8C8, spcmap_3li,  1, {12, 13, 14, 0, 8, 6,  5, 4, -1}},
   {s4x4, 0x1717, 0x0707, spcmap_3ro,  1, {10, 15,  3, 1, 9, 11, 7, 2, -1}},
   {s4x4, 0x1717, 0x1616, spcmap_3lo,  1, {10, 15,  3, 1, 9, 11, 7, 2, -1}},
   {s2x8, 0x0F0F, 0x0707, spcmap_328a, 0, {4, 5, 6, 7, 12, 13, 14, 15, -1}},
   {s2x8, 0xF0F0, 0xE0E0, spcmap_328b, 0, {0, 1, 2, 3, 8, 9, 10, 11, -1}},
   {s2x8, 0x1F1F, 0x0E0E, spcmap_328c, 0, {5, 6, 7, 13, 14, 15, -1}},
   {s2x8, 0xF8F8, 0x7070, spcmap_328d, 0, {0, 1, 2, 8, 9, 10, -1}},
   {s2x8, 0x3E3E, 0x1C1C, spcmap_328e, 0, {0, 6, 7, 8, 14, 15, -1}},
   {s2x8, 0x7C7C, 0x3838, spcmap_328f, 0, {0, 1, 7, 8, 9, 15, -1}},
   {s1x3dmd, 0x77,  0x77, spcmap_31x3d,0, {3, 7, -1}},
   {s3x1dmd, 0x77,  0x77, spcmap_33x1d,0, {3, 7, -1}},
   {s4x6, 0036363636, 022202220, spcmap_34x6a,0, {0, 5, 6, 11, 12, 17, 18, 23, -1}},
   {s4x6, 0036363636, 022022202, spcmap_34x6b,0, {0, 5, 6, 11, 12, 17, 18, 23, -1}},
   {s_qtag, 0xBB,   0xBB, spcmap_3lqtg,0, {2, 6, -1}},
   {s_spindle, 0x77,  0x77, spcmap_3spn,0, {3, 7, -1}},
   {s1x6, 077,  077, spcmap_31x6,0, {-1}},
   {nothing, 0}};


// Because these items need to "point" to each other in arbitrary order, and
// C++ initialization abhors such things, we use an enumeration type of indicators,
// and use those indicators as indices into the "fixer_ptr_table", which is initialized
// by "initialize_fix_tables".  The first field in each item below is its own
// tag.  It is the intention that the items below stay synchronized with the
// enumeration type, but it is not essential.  But if the items below fail to
// have exactly one item for every required enumeration tag, a run-time
// initialization error will occur.

// The "rot" field can have lots of bits (though usually it doesn't) because it has
// incremental bits per person.  So leave plenty of room.
// The 0x80000000 bit of "rot" means some complicated thing is going on; I don't
// remember the details.  The 0x40000000 bit means that the "nextdmdrot" field actually
// holds a vertical 2x3 map.
//
//                          prior_elong----|
//                                         |
//                                rot---\  |  /---numsetups
//                                      |  |  |
//                                      V  V  V
select::fixer select::fixer_init_table[] = {
   {fx_foo33a, s1x2, s2x4,              0, 0, 2, {5, 4, 0, 1},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_foocc, s1x2, s2x4,               UINT32_C(0x40000000), 0, 2, {2, 3, 7, 6},
    fx0, fx_foo33a,             fx_foocct, fx0,             fx0, fx_fooccd,             fx0, fx0},
   {fx_foo33, s1x2, s2x4,               UINT32_C(0x40000000), 0, 2, {0, 1, 5, 4},
    fx0, fx_foocc,              fx_foo33t, fx0,             fx0, fx_foo33d,             fx0, fx0},

   {fx_foocct, s1x4, s2x6,              0, 0, 2, {2, 3, 5, 4, 11, 10, 8, 9},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_foo33t, s1x4, s2x6,              0, 0, 2, {0, 1, 3, 2, 9, 8, 6, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fooccd, s2x3, s4x4,              1, 0, 2, {0, 1, 2, 7, 3, 14, 15, 11, 6, 8, 9, 10},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_foo33d, s2x3, s4x4,              1, 0, 2, {13, 15, 11, 9, 10, 12, 1, 2, 4, 5, 7, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_foo55, s1x1, s2x4,               0, 0, 4, {0, 2, 4, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fooaa, s1x1, s2x4,               0, 0, 4, {1, 3, 5, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_foo11, s1x1, s2x4,               0, 0, 2, {0, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_foo22, s1x1, s2x4,               0, 0, 2, {1, 5},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_foo44, s1x1, s2x4,               0, 0, 2, {2, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_foo88, s1x1, s2x4,               0, 0, 2, {3, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_foo77, s1x3, s2x4,               0, 0, 2, {0, 1, 2, 6, 5, 4},
    fx0, fx_fooEEprime,         fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fooEE, s1x3, s2x4,               0, 0, 2, {7, 6, 5, 1, 2, 3},
    fx0, fx_foo77,              fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fooEEprime, s1x3, s2x4,          0, 0, 2, {1, 2, 3, 7, 6, 5},
    fx0, fx_foo77,              fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_1x4p2d, sdmd, s1x4p2dmd,         0, 0, 1, {3, 4, 6, 7},
    fx0, fx0,                   fx_1x4p2l, fx0,       fx0, fx_4p2x1d,                   fx0, fx0},
   {fx_1x4p2l, s1x4, s1x8,              0, 0, 1, {2, 6, 5, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_4p2x1d, sdmd, s4p2x1dmd,         1, 0, 1, {4, 6, 7, 3},
    fx0, fx0,             fx0, fx_1x4p2l,                   fx0, fx_1x4p2d,             fx0, fx0},
   {fx_1x6lowf, s1x4, s1x6,             0, 0, 1, {0, 1, 5, 2},
    fx0, fx0,                         fx0, fx0,    fx_1x3p1lowf, fx_3p1x1lowf,          fx0, fx0},
   {fx_1x6hif, s1x4, s1x6,              0, 0, 1, {2, 5, 3, 4},
    fx0, fx0,                         fx0, fx0,    fx_1x3p1lhif, fx_3p1x1lhif,          fx0, fx0},
   {fx_1x3p1lowf, sdmd, s1x3p1dmd,      2, 0, 1, {4, 5, 2, 3},
    fx0, fx0,            fx_1x6lowf, fx0,                   fx0, fx_3p1x1lowf,          fx0, fx0},
   {fx_1x3p1lhif, sdmd, s1x3p1dmd,      0, 0, 1, {2, 3, 4, 5},
    fx0, fx0,             fx_1x6hif, fx0,                   fx0, fx_3p1x1lhif,          fx0, fx0},
   {fx_3p1x1lowf, sdmd, s3p1x1dmd, UINT32_C(0x800002AB), 0, 1, {3, 4, 5, 2},
    fx0, fx0,                   fx0, fx_1x6lowf,            fx0, fx_1x3p1lowf,          fx0, fx0},
   {fx_3p1x1lhif, sdmd, s3p1x1dmd,      1, 0, 1, {3, 4, 5, 2},
    fx0, fx0,                   fx0, fx_1x6hif,             fx0, fx_1x3p1lhif,          fx0, fx0},

   {fx_n1x43, s1x2, s1x4,               0, 0, 1, {0, 1},
    fx0, fx_box9c,              fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_n1x4c, s1x2, s1x4,               0, 0, 1, {3, 2},
    fx0, fx_box6c,              fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_n1x45, s1x1, s1x4,               0, 0, 2, {0, 2},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_n1x4a, s1x2, s1x4,           0, 0, 1, {1, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_n1x3a, s1x2, s1x3,           0, 0, 1, {0, 1},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_n1x3b, s1x2, s1x3,           0, 0, 1, {1, 2},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f1x6aa, s1x2, s1x6,          0, 0, 2, {0, 1, 4, 3},
    fx0, fx_fo6zz,              fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f1x8aa, s1x2, s1x8,          0, 0, 2, {1, 3, 7, 5},
    fx0, fx_foozz,              fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f1x855, s1x1, s1x8,          0, 0, 4, {0, 2, 4, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fo6zz, s1x2, s_bone6,        1, 0, 2, {0, 4, 1, 3},
    fx0, fx_f1x6aa,             fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_foozz, s1x2, s_ptpd,         1, 0, 2, {1, 3, 7, 5},
    fx0, fx_f1x8aa,             fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fo6zzd, s2x2, s_bone6,       0, 1, 1, {0, 1, 3, 4},
    fx0, fx0,                   fx_f1x6aad, fx0,            fx0, fx0,                   fx_fo6zzd, fx0},
   {fx_fo6zze, s1x2, s_bone6,       0, 1, 1, {5, 2},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_foozzd, s2x2, s_ptpd,        0, 1, 1, {1, 7, 5, 3},
    fx0, fx0,                   fx_f1x8aad, fx0,            fx0, fx0,                   fx_foozzd, fx_fqtgend},
   {fx_f3x4east, s1x2, s3x4,        1, 0, 2, {10, 9, 3, 4},
    fx0, fx_fd2x7d1,            fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f3x4west, s1x2, s3x4,        1, 0, 2, {0, 10, 4, 6},
    fx0, fx_fd2x7d2,            fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f3x4left, s1x2, s3x4,        0, 0, 2, {0, 1, 7, 6},
    fx0, fx_f3x4rzz,            fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f3x4right, s1x2, s3x4,       0, 0, 0x100+2, {2, 3, 9, 8},
    fx0, fx_f3x4lzz,            fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f3x4down,  s1x2, s3x4,        0, 0, 3, {0, 1, 11, 5, 7, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f3x4up,    s1x2, s3x4,        0, 0, 3, {2, 3, 11, 5, 9, 8},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x5down,  s1x2, s4x5,        0, 0, 4, {0, 1, 8, 7, 17, 18, 11, 10},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x5up,    s1x2, s4x5,        0, 0, 4, {3, 4, 7, 6, 16, 17, 14, 13},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f3x4lzz, s1x2, s2x6,         0, 0, 2, {0, 1, 7, 6},
    fx0, fx_f3x4right,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f3x4rzz, s1x2, s2x6,         0, 0, 0x100+2, {4, 5, 11, 10},
    fx0, fx_f3x4left,           fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x4nw, s1x2, s4x4,          0, 0, 2, {12, 13, 5, 4},
    fx0, fx_f4x4rzza,           fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x4ne, s1x2, s4x4,          0, 0, 0x100+2, {14, 0, 8, 6},
    fx0, fx_f4x4lzza,           fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x4wn, s1x2, s4x4,          1, 0, 2, {12, 10, 2, 4},
    fx0, fx_f4x4lzza,           fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x4en, s1x2, s4x4,          1, 0, 0x100+2, {0, 1, 9, 8},
    fx0, fx_f4x4rzza,           fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x4lzz, s1x2, s2x8,         0, 0, 2, {0, 1, 9, 8},
    fx0, fx_f4x4ne,             fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x4rzz, s1x2, s2x8,         0, 0, 0x100+2, {6, 7, 15, 14},
    fx0, fx_f4x4nw,             fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x4lzza, s1x2, s2x6,        0, 0, 2, {0, 1, 7, 6},
    fx0,   fx_f4x4ne,           fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x4rzza, s1x2, s2x6,        0, 0, 0x100+2, {4, 5, 11, 10},
    fx0,   fx_f4x4nw,           fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_f2x4tt0, s2x2, s2x4,         0, 0, 1, {1, 2, 4, 7},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4tt1, s2x2, s2x4,         0, 0, 1, {0, 3, 5, 6},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_f2x8qq0, s2x2, s2x8,         0, 0, 1, {0, 1, 14, 15},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x8qq1, s2x2, s2x8,         0, 0, 1, {6, 7, 8, 9},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_f2x8tt0, s2x2, s2x8,         0, 0, 1, {1, 2, 12, 15},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x8tt1, s2x2, s2x8,         0, 0, 1, {0, 3, 13, 14},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x8tt2, s2x2, s2x8,         0, 0, 1, {5, 6, 8, 11},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x8tt3, s2x2, s2x8,         0, 0, 1, {4, 7, 9, 10},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_f4x4neq, s2x2, s4x4,         0, 0, 1, {14, 0, 1, 3},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x4seq, s2x2, s4x4,         0, 0, 1, {7, 2, 4, 5},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x4swq, s2x2, s4x4,         0, 0, 1, {9, 11, 6, 8},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x4nwq, s2x2, s4x4,         0, 0, 1, {12, 13, 15, 10},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_f2x6qq0, s2x2, s2x6,         0, 0, 1, {0, 1, 10, 11},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x6qq1, s2x2, s2x6,         0, 0, 1, {4, 5, 6, 7},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x6tt0, s2x2, s2x6,         0, 0, 1, {1, 2, 8, 11},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x6tt1, s2x2, s2x6,         0, 0, 1, {0, 3, 9, 10},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x6tt2, s2x2, s2x6,         0, 0, 1, {2, 5, 7, 8},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x6tt3, s2x2, s2x6,         0, 0, 1, {3, 4, 6, 9},
    fx0,          fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_f3x4outer, s1x3, s3x4,       1, 0, 2, {0, 10, 9, 3, 4, 6},
    fx_f3x4outer, fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f3dmouter, s1x3, s3dmd,      0, 0, 2, {8, 7, 6, 0, 1, 2},
    fx_f3dmouter, fx0,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f3ptpdin, s1x2, s3ptpd,      0, 0, 2, {10, 11, 5, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_fpgdmdcw, s1x2, spgdmdcw,    0, 0, 2, {6, 7, 3, 2},
    fx0, fx_f2x6cw,             fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fpgdmdccw, s1x2, spgdmdccw,  0, 0, 2, {6, 7, 3, 2},
    fx0, fx_f2x6ccw,            fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x6cw, s1x2, s2x6,          0, 0, 2, {3, 4, 10, 9},
    fx0, fx_fpgdmdcw,           fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x6ccw, s1x2, s2x6,         0, 0, 2, {1, 2, 8, 7},
    fx0, fx_fpgdmdccw,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_fdhrgl, s_trngl, s_dhrglass, 0x2A03, 0, 2, {6, 5, 0, 2, 1, 4},
    fx_f323, fx0,               fx0, fx0,                   fx0, fx_specspindle,        fx0, fx0},
   {fx_specspindle, s_trngl, s_spindle, 0x2A01, 0, 2, {7, 0, 6, 3, 4, 2},
    fx_specspindle, fx0,        fx0, fx0,                   fx0, fx_fdhrgl,             fx0, fx0},
   {fx_specfix3x40, s_trngl, s_ptpd, 0x00AB, 0, 2, {0, 1, 3, 4, 5, 7},
    fx_specfix3x40, fx0,        fx0, fx0,                   fx0, fx_specfix3x41,        fx0, fx0},
   {fx_specfix3x41, s_trngl, s_bone, 0x00A9, 0, 2, {6, 5, 0, 2, 1, 4},
    fx_specfix3x41, fx0,        fx0, fx0,                   fx0, fx_specfix3x40,        fx0, fx0},
   {fx_f525nw, s1x3, s_525,         0, 0, 2, {8, 7, 6, 0, 1, 2},
    fx_f525nw, fx0,             fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f525ne, s1x3, s_525,         0, 0, 2, {10, 9, 8, 2, 3, 4},
    fx_f525ne, fx0,             fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fdpx44, s1x2, s4x4,          0, 0, 2, {10, 9, 1, 2},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fdpxwve, s1x2, sdeepxwv,      0, 0, 2, {0, 1, 7, 6},
    fx0, fx_fdpx44,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_f1x12outer, s1x3, s1x12,      0, 0, 2, {0, 1, 2, 8, 7, 6},
    fx_f1x12outer, fx_f3x4outer,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_f3x4outrd, s2x3, s3x4,        1, 0, 1, {3, 4, 6, 9, 10, 0},
    fx0, fx0,    fx0, fx0,             fx0, fx0,    fx_f3x4outrd, fx_f3dmoutrd},
   {fx_f3dmoutrd, s2x3, s3dmd,       0, 0, 1, {0, 1, 2, 6, 7, 8},
    fx0, fx0,    fx0, fx0,             fx0, fx0,    fx_f3dmoutrd, fx_f3x4outrd},
   {fx_fdhrgld, s_bone6, s_dhrglass, 0, 0, 1, {0, 1, 2, 4, 5, 6},
    fx0, fx_fdhrgld,    fx0, fx0,             fx0, fx_fspindlod,    fx0, fx_f323d},
   {fx_f1x12outrd, s1x6, s1x12,      0, 0, 1, {0, 1, 2, 6, 7, 8},
    fx0, fx0,    fx_f1x12outrd, fx0,   fx0, fx0,             fx0, fx0},
   {fx_f1x12outre, s1x8, s1x12,      0, 0, 1, {0, 1, 5, 4, 6, 7, 11, 10},
    fx0, fx0,    fx_f1x12outre, fx0,   fx0, fx0,             fx0, fx0},
   {fx_f1x3d6, s1x3, s1x3dmd,        0, 0, 2, {0, 1, 2, 6, 5, 4},
    fx_f1x3d6, fx_f323,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_f323, s1x3, s_323,            0, 0, 2, {6, 5, 4, 0, 1, 2},
    fx_f323, fx_f1x3d6,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_f323d, s2x3, s_323,           0, 0, 1, {0, 1, 2, 4, 5, 6},
    fx0, fx0,    fx0, fx0,             fx0, fx0,        fx_f323d, fx_f3x4outrd},
   {fx_f3x1zzd, sdmd, s3x1dmd,       0, 0, 1, {0, 3, 4, 7},
    fx0, fx0,    fx_f1x8endd, fx0, fx_f3x1zzd, fx0,          fx0, fx0},
   {fx_f1x3zzd, sdmd, s1x3dmd,       0, 0, 1, {0, 3, 4, 7},
    fx0, fx0,    fx_foo55d, fx0, fx_f1x3zzd, fx0,            fx0, fx0},
   {fx_f3x1yyd, sdmd, s3x1dmd,       0, 0, 1, {1, 3, 5, 7},
    fx0, fx0,    fx_f1x8aad, fx0, fx_f3x1yyd, fx0,           fx0, fx0},
   {fx_f2x1yyd, s_star, s_2x1dmd,    0, 0, 1,          {0, 2, 3, 5},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fwstyyd, s_star, s_wingedstar6,0, 0, 1,         {0, 2, 3, 5},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_f1x4xv, s1x4, s_crosswave,    1, 0, 1,          {2, 3, 6, 7},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_f1x3yyd, sdmd, s1x3dmd,       0, 0, 1, {1, 3, 5, 7},
    fx0, fx0,    fx_foo66d, fx_f1x4xv, fx_f1x3yyd, fx0, fx_frigctr, fx_frigctr},
   {fx_f1x6aad, s1x4, s1x6,          0, 0, 1,    {0, 1, 3, 4},
    fx0, fx0,    fx_f1x6aad, fx0, fx_f3x1zzd, fx0,     fx_fo6zzd, fx_fo6zzd},
   {fx_f1x4qv, s1x4, s_2x1dmd,       0, 0, 1,          {0, 1, 3, 4},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fo6qqd, s2x2, s_short6,      1, 0, 1,          {2, 3, 5, 0},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_f21dabd, sdmd, s_2x1dmd,     1, 0, 1,          {2, 4, 5, 1},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_f12dabd, sdmd, s_1x2dmd,     0, 0, 1, {0, 2, 3, 5},
    fx0, fx0,    fx_f1x6abd, fx_f1x4xv, fx_f12dabd, fx_f21dabd, fx_fo6qqd, fx_fo6qqd},
   {fx_f1x6abd, s1x4, s1x6,         0, 0, 1, {0, 2, 3, 5},
    fx0, fx0,  fx0, fx_f1x4qv,     fx_f12dabd, fx_f21dabd, fx_fo6qqd, fx_fo6qqd},

   {fx_f1x10abd, s1x4, s1x10,        0, 0, 1, {0, 3, 5, 8},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},

   {fx_f2x1ded, s1x2, s_2x1dmd,     1, 0, 1,          {2, 5},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_f1x6ed, s1x2, s1x6,          0, 0, 1,          {0, 3},
    fx0, fx_f2x1ded,    fx0, fx0,             fx0, fx0,             fx0, fx0},

   {fx_f3ptp,   s2x3, s3ptpd,       0, 0, 1,    {0, 1, 2, 6, 7, 8},
    fx0, fx0,      fx_f3dmdln, fx0,           fx0, fx0,             fx0, fx0},
   {fx_f3dmdln, s1x4, s1x8,         0, 0, 1,    {0, 2, 4, 6},
    fx0, fx0,           fx0, fx0,             fx0, fx0,             fx0, fx0},

   {fx_f1x8aad, s1x4, s1x8,         0, 0, 1,    {1, 3, 5, 7},
    fx0, fx0,    fx_f1x8aad, fx0, fx_f3x1zzd, fx0,     fx_foozzd, fx_foozzd},
   {fx_fxwv1d, sdmd, s_crosswave,   0, 0, 1,          {0, 2, 4, 6},
    fx0, fx0,    fx0, fx0,       fx_fxwv1d, fx0,             fx0, fx0},
   {fx_fxwv2d, sdmd, s_crosswave,   0, 0, 1,          {0, 3, 4, 7},
    fx0, fx0,    fx0, fx0,       fx_fxwv2d, fx0,             fx0, fx0},
   {fx_fxwv3d, s_star, s_crosswave,  0, 0, 1,          {1, 2, 5, 6},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fqtgns, s1x2, s_qtag,        0, 0, 2,          {0, 1, 5, 4},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_ftharns, s1x2, s_thar,       1, 0, 2,          {2, 3, 7, 6},
    fx0, fx_fqtgns,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_ftharew, s1x2, s_thar,       0, 0, 2,          {0, 1, 5, 4},
    fx0, fx_fqtgns,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_falamons, s1x2, s_alamo,     0, 0, 2,          {0, 1, 5, 4},
    fx0, fx_ftharns,   fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_falamoew, s1x2, s_alamo,     1, 0, 2,          {7, 6, 2, 3},
    fx0, fx_ftharew,   fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fqtgj1, s1x2, s_qtag,        1, 0, 2,          {1, 3, 7, 5},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fqtgj2, s1x2, s_qtag,        1, 0, 2,          {0, 7, 3, 4},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_f2x3j1, s1x2, s2x3,          0, 0, 2,            {0, 1, 4, 3},
    fx0, fx_foocc,    fx0, fx0,             fx0, fx0,    fx0, fx0},
   {fx_f2x3j2, s1x2, s2x3,          0, 0, 2,            {1, 2, 5, 4},
    fx0, fx_foo33a,    fx0, fx0,             fx0, fx0,    fx0, fx0},
   {fx_fqtgjj1, s2x2, s_qtag,       0, 0, 1, {7, 1, 3, 5},
    fx0, fx0,    fx0, fx0,             fx0, fx0, fx_fqtgjj1, fx_fqtgjj1},
   {fx_fqtgjj2, s2x2, s_qtag,       0, 0, 1, {0, 3, 4, 7},
    fx0, fx0,    fx0, fx0,             fx0, fx0, fx_fqtgjj2, fx_fqtgjj2},
   {fx_f2x3jj1, s2x2, s2x3,         0, 0, 1, {1, 2, 4, 5},
    fx0, fx0,    fx0, fx0,             fx0, fx0, fx_f2x3jj1, fx_f2x3jj1},
   {fx_f2x3jj2, s2x2, s2x3,         0, 0, 1, {0, 1, 3, 4},
    fx0, fx0,    fx0, fx0,             fx0, fx0, fx_f2x3jj2, fx_f2x3jj2},
   {fx_f2x3jj3, s2x2, s2x3,         0, 0, 1, {0, 2, 3, 5},
    fx0, fx0,    fx_f1x6aad, fx0,      fx0, fx0, fx_f2x3jj3, fx_f2x3jj3},
   {fx_fgalcv, s1x2, s_galaxy,      1, 0, 1,          {2, 6},
    fx0, fx_fgalch,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fgalch, s1x2, s_galaxy,      0, 0, 1,          {0, 4},
    fx0, fx_fgalcv,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fdblbentcw, s1x2, sbigh,     1, 0, 2,          {0, 1, 7, 6},
    fx0, fx_fcrookedcw,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fdblbentccw, s1x2, sbigh,    1, 0, 2,          {2, 3, 9, 8},
    fx0, fx_fcrookedccw,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fcrookedcw, s1x2, sbigbone,  0, 0, 2,          {0, 1, 7, 6},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fcrookedccw, s1x2, sbigbone, 0, 0, 2,          {11, 10, 4, 5},
    fx0, fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fspindlc, s1x2, s_spindle,   1, 0, 2,          {0, 6, 2, 4},
    fx0, fx_f1x3aad,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fspindlf, s1x2, s_spindle,   0, 0, 2,          {1, 2, 6, 5},
    fx0,  fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fspindlg, s1x2, s_spindle,   0, 0, 2,          {0, 1, 5, 4},
    fx0,  fx0,    fx0, fx0,             fx0, fx0,             fx0, fx0},
   {fx_fspindlfd, s2x2, s_spindle,   0, 0, 1,          {1, 2, 5, 6},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_fspindlfd, fx0},
   {fx_fspindlgd, s2x2, s_spindle,   0, 0, 1,          {0, 1, 4, 5},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_fspindlgd, fx0},
   {fx_ftgl6cwd, s2x2, s_ntrgl6cw,   0, 0, 1,          {0, 1, 3, 4},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_ftgl6ccwd, s2x2, s_ntrgl6ccw, 0, 0, 1,          {1, 2, 4, 5},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_ftgl6cld, s1x4, s_ntrgl6cw,   0, 0, 1,          {5, 4, 2, 1},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_ftgl6ccld, s1x4, s_ntrgl6ccw, 0, 0, 1,          {0, 1, 3, 4},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_ftgl6ccd, s1x2, s_ntrgl6cw,   0, 0, 2,          {5, 4, 1, 2},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_ftgl6cccd, s1x2, s_ntrgl6ccw, 0, 0, 2,          {0, 1, 4, 3},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f1x3aad, s1x2, s1x3dmd,     0, 0, 2,          {1, 2, 6, 5},
    fx0,   fx_fspindlc,  fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f2x3c, s1x2, s2x3,          1, 0, 2,          {0, 5, 2, 3},
    fx0,     fx_f1x2aad,   fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f2x3a41, s1x2, s2x3,        1, 0, 1,          {0, 5},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f2x3a14, s1x2, s2x3,        1, 0, 1,          {2, 3},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_box3c, s1x2, s2x2,          0, 0, 1,          {0, 1},
    fx0, fx_n1x4c,          fx0, fx0,        fx0, fx0,    fx0, fx0},
   {fx_box6c, s1x2, s2x2,          1, 0, 1,          {1, 2},
    fx0,     fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_box9c, s1x2, s2x2,          1, 0, 1,          {0, 3},
    fx0,     fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_boxcc, s1x2, s2x2,          0, 0, 1,          {3, 2},
    fx0,     fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_box55, s1x1, s2x2,        0, 0, 2,          {0, 2},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_boxaa, s1x1, s2x2,        0, 0, 2,          {1, 3},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f2x5c, s1x2, s2x5,        0, 0, 2,          {3, 4, 9, 8},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f2x5d, s1x2, s2x5,        0, 0, 2,          {0, 1, 6, 5},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f2x5e, s1x2, s2x5,        1, 0, 3,          {0, 9, 2, 7, 4, 5},
    fx0,          fx_f4ptpd,    fx0,          fx0, fx0,          fx0,    fx0,          fx0},

   {fx_f2x5f, s1x2, s2x5,        1, 0, 2,          {0, 9, 4, 5},
    fx0,          fx_sp12wing,  fx0,          fx0, fx0,          fx0,    fx0,          fx0},

   {fx_sp12wing, s1x2, s_spindle12, 0, 0, 2,       {8, 9, 4, 3},
    fx0,          fx_f2x5f,     fx0,          fx0, fx0,          fx0,    fx0,          fx0},

   {fx_f4ptpd, s1x2, s4ptpd,     0, 0, 3,          {13, 14, 15, 7, 6, 5},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_fd2x5d, s2x3, sd2x5,      1, 0, 1,          {9, 8, 7, 4, 3, 2},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_fd2x7d1, s1x2, sd2x7,      0, 0, 2,          {13, 12, 5, 6},
    fx0,          fx_f3x4east,  fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_fd2x7d2, s1x2, sd2x7,      0, 0, 2,          {0, 1, 8, 7},
    fx0,          fx_f3x4west,  fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_fd2x7d3, s1x4, sd2x7,      0, 0, 1,          {13, 12, 6, 5},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_d3x4ub6,   fx0},
   {fx_fd2x7d4, s1x4, sd2x7,      0, 0, 1,          {0, 1, 7, 8},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_d3x4ub5,   fx0},
   {fx_fd2x7q1, s1x2, sd2x7,      1, 0, 2,          {11, 10, 3, 4},
    fx0,          fx_foocc,     fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_fd2x7q2, s1x2, sd2x7,      1, 0, 2,          {10, 9, 2, 3},
    fx0,          fx_foo33a,    fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_fd2x7q3, s1x4, s2x6,       0, 0, 1,          {11, 10, 5, 4},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_d3x4ub6,   fx0},
   {fx_fd2x7q4, s1x4, s2x6,       0, 0, 1,          {0, 1, 6, 7},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_d3x4ub5,   fx0},
   {fx_fd2x7za, s2x3, sd2x7,       1, 0, 1,          {11, 10, 9, 4, 3, 2},
    fx0,          fx0,         fx0,          fx0, fx0,         fx0,     fx0,           fx0},
   {fx_fd2x7zb, s2x3, sd2x7,       1, 0, 1,          {11, 10, 9, 4, 3, 2},
    fx0,          fx0,         fx0,          fx0, fx0,         fx0,     fx0,           fx0},
   {fx_bghrgla, s1x2, sbighrgl,  0, 0, 2,          {11, 10, 4, 5},
    fx0,   fx_shsqtga,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_bghrglb, s1x2, sbighrgl,  0, 0, 2,          {0, 1, 7, 6},
    fx0,   fx_shsqtgb,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_shsqtga, s1x2, s_hsqtag,  1, 0, 2,          {2, 3, 9, 8},
    fx0,   fx_bghrgla,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_shsqtgb, s1x2, s_hsqtag,  1, 0, 2,          {0, 1, 7, 6},
    fx0,   fx_bghrglb,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f4x6a, s1x2, s4x6,        0, 0, 2,          {13, 12, 0, 1},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f4x6b, s1x2, s4x6,        0, 0, 2,          {22, 23, 11, 10},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f4x6c, s1x2, s4x6,        0, 0, 2,          {7, 6, 18, 19},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f4x6d, s1x2, s4x6,        0, 0, 2,          {4, 5, 17, 16},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f4x6e, s1x2, s4x6,        1, 0, 2,          {0, 11, 23, 12},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f4x6f, s1x2, s4x6,        1, 0, 2,          {18, 17, 5, 6},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f1x2aad, s1x2, s_1x2dmd,  0, 0, 2,          {0, 1, 4, 3},
    fx0,   fx_f2x3c,     fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f2x166d, sdmd, s_2x1dmd,  0, 0, 1,          {2, 4, 5, 1},
    fx0,          fx0,          fx0,          fx0, fx_f2x166d,   fx0,    fx0,          fx0},
   {fx_f1x3bbd, s1x4, s1x3dmd,   0, 0, 1,  {1, 2, 5, 6},
    fx0,          fx0,          fx_f1x3bbd,   fx0, fx0,          fx0,    fx_fspindld,  fx_fspindld},
   {fx_f1x3wwd, s1x4, s1x3dmd,   0, 0, 1,  {0, 2, 4, 6},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_fspindld,  fx0},
   {fx_fhrglassd, s2x2, s_hrglass,   0, 2, 1,          {0, 1, 4, 5},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_fspindld, s2x2, s_spindle,   0, 1, 1, {0, 2, 4, 6},
    fx0,          fx0,          fx_f1x3bbd,   fx0, fx0,          fx0,    fx_fspindld,  fx_fhrglassd},
   {fx_fptpzzd, s1x4, s_ptpd,    0, 0, 1,          {0, 2, 4, 6},
    fx0,          fx0,          fx_fptpzzd,   fx0, fx_fspindlbd, fx0,    fx0,          fx0},
   {fx_f3ptpo6, s_1x2dmd, s3ptpd,  0, 0, 1,          {10, 11, 1, 4, 5, 7},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_fspindlbd, sdmd, s_spindle,   0, 0, 1,          {7, 1, 3, 5},
    fx0,          fx0,          fx_fptpzzd,   fx0, fx_fspindlbd, fx0,    fx0,          fx0},
   {fx_fspindlod, s_short6, s_spindle,1, 0, 1,        {2, 3, 4, 6, 7, 0},
    fx0,          fx_fdhrgld,   fx0,          fx0, fx0,          fx_fspindlod, fx_f323d, fx0},
   {fx_d2x4b1, s2x2, s4x4,       0, 0, 1,          {13, 7, 5, 15},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_d2x4b2, s2x2, s4x4,       0, 0, 1,          {11, 14, 3, 6},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_d2x4w1, s1x4, s2x4,       0, 0, 1,    {0, 1, 4, 5},
    fx0,          fx0,          fx_d2x4w1,    fx0, fx0,          fx0,    fx_d2x4b1,    fx_d2x4b1},
   {fx_d2x4w2, s1x4, s2x4,        0, 0, 1,    {7, 6, 3, 2},
    fx0,          fx0,          fx_d2x4w2,    fx0, fx0,          fx0,    fx_d2x4b2,    fx_d2x4b2},
   {fx_d2x4q1, s1x4, s2x4,        0, 0, 1,          {0, 1, 4, 5},
    fx0,          fx0,          fx_d2x4q1,    fx0, fx0,          fx0,    fx0,          fx0},
   {fx_d2x4q2, s1x4, s2x4,        0, 0, 1,          {7, 6, 3, 2},
    fx0,          fx0,          fx_d2x4q2,    fx0, fx0,          fx0,    fx0,          fx0},
   {fx_d2x4d1, sdmd, s2x4,        0, 0, 1,    {7, 1, 3, 5},
    fx0,          fx0,          fx_d2x4x1,    fx0, fx_d2x4d1,    fx0,    fx_d2x4c1,    fx_d2x4c1},
   {fx_d2x4d2, sdmd, s2x4,        0, 0, 1,    {0, 2, 4, 6},
    fx0,          fx0,          fx_d2x4x2,    fx0, fx_d2x4d2,    fx0,    fx_d2x4c2,    fx_d2x4c2},
   {fx_d2x4c1, s2x2, s2x4,        0, 0, 1,    {1, 3, 5, 7},
    fx0,          fx0,          fx_d2x4x1,    fx0, fx_d2x4d1,    fx0,    fx_d2x4c1,    fx_d2x4c1},
   {fx_d2x4c2, s2x2, s2x4,        0, 0, 1,    {0, 2, 4, 6},
    fx0,          fx0,          fx_d2x4x2,    fx0, fx_d2x4d2,    fx0,    fx_d2x4c2,    fx_d2x4c2},
   {fx_d2x4z1, s2x2, s4x4,        0, 0, 1,          {9, 11, 1, 3},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_d2x4z2, s2x2, s4x4,        0, 0, 1,          {10, 15, 2, 7},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_d2x4y1, s2x2, s2x4,        0, 0, 1,    {0, 1, 4, 5},
    fx0,          fx0,          fx_d2x4w1,    fx_d2x4z1, fx0,    fx0,    fx_d2x4y1,    fx_d2x4y1},
   {fx_d2x4y2, s2x2, s2x4,        0, 0, 1,          {2, 3, 6, 7},
    fx0,          fx0,          fx_d2x4w2,    fx_d2x4z2, fx0,    fx0,    fx0,          fx0},
   {fx_d3x4ul1, s1x4, s3x4,        0, 0, 1,          {10, 8, 6, 5},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx_d3x4ub1,   fx0},
   {fx_d3x4ul2, s1x4, s3x4,        0, 0, 1,          {9, 11, 4, 7},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx_d3x4ub2,   fx0},
   {fx_d3x4ul3, s1x4, s3x4,        0, 0, 1,          {10, 1, 3, 5},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx_d3x4ub3,   fx0},
   {fx_d3x4ul4, s1x4, s3x4,        0, 0, 1,          {0, 11, 4, 2},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx_d3x4ub4,   fx0},
   {fx_d3x4ul5, s1x4, s3x4,        0, 0, 1,          {9, 11, 3, 5},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d3x4ul6, s1x4, s3x4,        0, 0, 1,          {0, 11, 6, 5},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d3x4ub1, s2x2, s3x4,        0, 0, 1,          {10, 5, 6, 8},
    fx0,          fx0,          fx_d3x4ul1,    fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d3x4ub2, s2x2, s3x4,        0, 0, 1,          {11, 4, 7, 9},
    fx0,          fx0,          fx_d3x4ul2,    fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d3x4ub3, s2x2, s3x4,        0, 0, 1,          {1, 3, 5, 10},
    fx0,          fx0,          fx_d3x4ul3,    fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d3x4ub4, s2x2, s3x4,        0, 0, 1,          {0, 2, 4, 11},
    fx0,          fx0,          fx_d3x4ul4,    fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d3x4ub5, s2x2, s3x4,        0, 0, 1,          {0, 4, 6, 10},
    fx0,          fx0,          fx_fd2x7d4,    fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d3x4ub6, s2x2, s3x4,        0, 0, 1,          {10, 3, 4, 9},
    fx0,          fx0,          fx_fd2x7d3,    fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d3x4vl1, s1x4, s3x4,        0, 0, 1,          {10, 11, 6, 7},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx_d3x4vb1,   fx0},
   {fx_d3x4vl2, s1x4, s3x4,        0, 0, 1,          {9, 8, 4, 5},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx_d3x4vb2,   fx0},
   {fx_d3x4vl3, s1x4, s3x4,        0, 0, 1,          {10, 11, 3, 2},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx_d3x4vb3,   fx0},
   {fx_d3x4vl4, s1x4, s3x4,        0, 0, 1,          {0, 1, 4, 5},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx_d3x4vb4,   fx0},
   {fx_d3x4vb1, s2x2, s3x4,        0, 0, 1,          {10, 11, 6, 7},
    fx0,          fx0,          fx_d3x4vl1,    fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d3x4vb2, s2x2, s3x4,        0, 0, 1,          {5, 4, 8, 9},
    fx0,          fx0,          fx_d3x4vl2,    fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d3x4vb3, s2x2, s3x4,        0, 0, 1,          {2, 3, 11, 10},
    fx0,          fx0,          fx_d3x4vl3,    fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d3x4vb4, s2x2, s3x4,        0, 0, 1,          {0, 1, 4, 5},
    fx0,          fx0,          fx_d3x4vl4,    fx0,      fx0,    fx0,    fx0,          fx0},

   {fx_d4x4b1,  s2x2, s4x4,        0, 0, 1,          {10, 14, 2, 6},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d4x4b2,  s2x2, s4x4,        0, 0, 1,          {13, 1, 5, 9},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d4x4b3,  s2x2, s4x4,        0, 0, 1,          {12, 3, 4, 11},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx0,          fx0},
   {fx_d4x4b4,  s2x2, s4x4,        0, 0, 1,          {15, 0, 7, 8},
    fx0,          fx0,          fx0,           fx0,      fx0,    fx0,    fx0,          fx0},

   {fx_d2x4x1, s1x4, s2x4,         0, 0, 1,    {7, 1, 3, 5},
    fx0,          fx0,          fx_d2x4x1,    fx0, fx_d2x4d1,    fx0,    fx_d2x4c1,    fx_d2x4c1},
   {fx_d2x4x2, s1x4, s2x4,        0, 0, 1,    {0, 6, 4, 2},
    fx0,          fx0,          fx_d2x4x2,    fx0, fx_d2x4d2,    fx0,    fx_d2x4c2,    fx_d2x4c2},
   {fx_dgalw1, s1x4, s_galaxy,    1, 0, 1,          {2, 1, 6, 5},
    fx0,          fx0,          fx_dgalw1,    fx0, fx_dgald1,    fx0,    fx0,          fx0},
   {fx_dgalw2, s1x4, s_galaxy,    1, 0, 1,          {2, 3, 6, 7},
    fx0,          fx0,          fx_dgalw2,    fx0, fx_dgald2,    fx0,    fx0,          fx0},
   {fx_dgalw3, s1x4, s_galaxy,    0, 0, 1,          {0, 1, 4, 5},
    fx0,          fx0,          fx_dgalw3,    fx0, fx_dgald3,    fx0,    fx0,          fx0},
   {fx_dgalw4, s1x4, s_galaxy,    0, 0, 1,          {0, 7, 4, 3},
    fx0,          fx0,          fx_dgalw4,    fx0, fx_dgald4,    fx0,    fx0,          fx0},
   {fx_dgald1, sdmd, s_galaxy,    1, 0, 1,          {2, 5, 6, 1},
    fx0,          fx0,          fx_dgalw1,    fx0, fx_dgald1,    fx0,    fx0,          fx0},
   {fx_dgald2, sdmd, s_galaxy,    1, 0, 1,          {2, 3, 6, 7},
    fx0,          fx0,          fx_dgalw2,    fx0, fx_dgald2,    fx0,    fx0,          fx0},
   {fx_dgald3, sdmd, s_galaxy,    0, 0, 1,          {0, 1, 4, 5},
    fx0,          fx0,          fx_dgalw3,    fx0, fx_dgald3,    fx0,    fx0,          fx0},
   {fx_dgald4, sdmd, s_galaxy,    0, 0, 1,          {0, 3, 4, 7},
    fx0,          fx0,          fx_dgalw4,    fx0, fx_dgald4,    fx0,    fx0,          fx0},
   {fx_ddmd1, sdmd, s_qtag,      0, 0, 1,          {6, 1, 2, 5},
    fx0,          fx0,          fx0,          fx0, fx_ddmd1,     fx0,    fx0,          fx0},
   {fx_ddmd2, sdmd, s_qtag,      0, 0, 1,          {6, 0, 2, 4},
    fx0,          fx0,          fx0,          fx0, fx_ddmd2,     fx0,    fx0,          fx0},
   {fx_distbone1, s1x4, s_bone,      0, 0, 1,          {0, 6, 4, 2},
    fx0,          fx0,          fx_distbone1, fx0, fx0,          fx0,    fx0,          fx0},
   {fx_distbone2, s1x4, s_bone,      0, 0, 1,          {0, 7, 4, 3},
    fx0,          fx0,          fx_distbone2, fx0, fx0,          fx0,    fx0,          fx0},
   {fx_distbone5, s1x4, s_bone,      0, 0, 1,          {5, 6, 1, 2},
    fx0,          fx0,          fx_distbone5, fx0, fx0,          fx0,    fx0,          fx0},
   {fx_distbone6, s1x4, s_bone,      0, 0, 1,          {5, 7, 1, 3},
    fx0,          fx0,          fx_distbone6, fx0, fx0,          fx0,    fx0,          fx0},
   {fx_distpgdmd1, s1x4, spgdmdcw,   0, 0, 1,          {6, 7, 2, 3},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_distpgdmd2, s1x4, spgdmdccw,  0, 0, 1,          {6, 7, 2, 3},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_distrig3, sdmd, s_rigger,    0, 0, 1,          {7, 0, 3, 4},
    fx0,          fx0,          fx_distrig1,  fx0, fx_distrig3,  fx0,    fx0,          fx0},
   {fx_distrig1, s1x4, s_rigger,    0, 0, 1,          {7, 0, 3, 4},
    fx0,          fx0,          fx_distrig1,  fx0, fx_distrig3,  fx0,    fx0,          fx0},
   {fx_distrig4, sdmd, s_rigger,    0, 0, 1,          {6, 1, 2, 5},
    fx0,          fx0,          fx_distrig2,  fx0, fx_distrig4,  fx0,    fx0,          fx0},
   {fx_distrig2, s1x4, s_rigger,    0, 0, 1,          {6, 5, 2, 1},
    fx0,          fx0,          fx_distrig2,  fx0, fx_distrig4,  fx0,    fx0,          fx0},
   {fx_distrig7, sdmd, s_rigger,    0, 0, 1,          {6, 0, 2, 4},
    fx0,          fx0,          fx_distrig5,  fx0, fx_distrig7,  fx0,    fx0,          fx0},
   {fx_distrig5, s1x4, s_rigger,    0, 0, 1,          {6, 0, 2, 4},
    fx0,          fx0,          fx_distrig5,  fx0, fx_distrig7,  fx0,    fx0,          fx0},
   {fx_distrig8, sdmd, s_rigger,    0, 0, 1,          {7, 1, 3, 5},
    fx0,          fx0,          fx_distrig6,  fx0, fx_distrig8,  fx0,    fx0,          fx0},
   {fx_distrig6, s1x4, s_rigger,    0, 0, 1,          {7, 5, 3, 1},
    fx0,          fx0,          fx_distrig6,  fx0, fx_distrig8,  fx0,    fx0,          fx0},
   {fx_distptp1, s1x4, s_ptpd,      0, 0, 1,          {3, 2, 7, 6},
    fx0,          fx0,          fx_distptp1,  fx0, fx0,          fx0,    fx0,          fx0},
   {fx_distptp2, s1x4, s_ptpd,      0, 0, 1,          {1, 2, 5, 6},
    fx0,          fx0,          fx_distptp2,  fx0, fx0,          fx0,    fx0,          fx0},
   {fx_distptp3, s1x4, s_ptpd,      0, 0, 1,          {0, 1, 4, 5},
    fx0,          fx0,          fx_distptp3,  fx0, fx_distptp7,  fx0,    fx0,          fx0},
   {fx_distptp4, s1x4, s_ptpd,      0, 0, 1,          {0, 3, 4, 7},
    fx0,          fx0,          fx_distptp4,  fx0, fx_distptp8,  fx0,    fx0,          fx0},
   {fx_distptp7, sdmd, s_ptpd,      0, 0, 1,          {0, 1, 4, 5},
    fx0,          fx0,          fx_distptp3,  fx0, fx_distptp7,  fx0,    fx0,          fx0},
   {fx_distptp8, sdmd, s_ptpd,      0, 0, 1,          {0, 7, 4, 3},
    fx0,          fx0,          fx_distptp4,  fx0, fx_distptp8,  fx0,    fx0,          fx0},
   {fx_disthrg1, s1x4, s_hrglass,   1, 0, 1,          {1, 3, 5, 7},
    fx0,          fx0,          fx_disthrg1,  fx0, fx0,          fx0,    fx0,          fx0},
   {fx_disthrg2, s1x4, s_hrglass,   1, 0, 1,          {0, 3, 4, 7},
    fx0,          fx0,          fx_disthrg2,  fx0, fx0,          fx0,    fx0,          fx0},
   {fx_d4x4l1, s1x4, s4x4,        1, 0, 1,          {0, 3, 8, 11},
    fx0,          fx0,          fx_d4x4l1, fx_d4x4l4, fx_d4x4d1, fx_d4x4d4, fx0,       fx0},
   {fx_d4x4l2, s1x4, s4x4,        0, 0, 1,          {8, 11, 0, 3},
    fx0,          fx0,          fx_d4x4l2, fx_d4x4l3, fx_d4x4d2, fx_d4x4d3, fx0,       fx0},
   {fx_d4x4l3, s1x4, s4x4,        1, 0, 1,          {12, 15, 4, 7},
    fx0,          fx0,          fx_d4x4l3, fx_d4x4l2, fx_d4x4d3, fx_d4x4d2, fx0,       fx0},
   {fx_d4x4l4, s1x4, s4x4,        0, 0, 1,          {12, 15, 4, 7},
    fx0,          fx0,          fx_d4x4l4, fx_d4x4l1, fx_d4x4d4, fx_d4x4d1, fx0,       fx0},
   {fx_d4x4d1, sdmd, s4x4,        1, 0, 1,          {0, 7, 8, 15},
    fx0,          fx0,          fx_d4x4l1, fx_d4x4l4, fx_d4x4d1, fx_d4x4d4, fx0,       fx0},
   {fx_d4x4d2, sdmd, s4x4,        0, 0, 1,          {8, 15, 0, 7},
    fx0,          fx0,          fx_d4x4l2, fx_d4x4l3, fx_d4x4d2, fx_d4x4d3, fx0,       fx0},
   {fx_d4x4d3, sdmd, s4x4,        1, 0, 1,          {12, 3, 4, 11},
    fx0,          fx0,          fx_d4x4l3, fx_d4x4l2, fx_d4x4d3, fx_d4x4d2, fx0,       fx0},
   {fx_d4x4d4, sdmd, s4x4,        0, 0, 1,          {12, 3, 4, 11},
    fx0,          fx0,          fx_d4x4l4, fx_d4x4l1, fx_d4x4d4, fx_d4x4d1, fx0,       fx0},
   {fx_dqtgdbb1, s1x4, s2x3,   0x110, 0, 1,          {0, 1, 3, 4},
    fx0, fx0,                   fx0, fx0,    fx0, fx0,                      fx0, fx0},
   {fx_dqtgdbb2, s1x4, s2x3,   0x330, 0, 1,          {5, 4, 2, 1},
    fx0, fx0,                   fx0, fx0,    fx0, fx0,                      fx0, fx0},
   {fx_hrgminb, s2x3, s_hrglass,  0, 0, 1,          {0, 3, 1, 4, 7, 5},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_ptpminb, s2x3, s_ptpd,     1, 0, 1,          {7, 6, 5, 3, 2, 1},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_rigmino, s2x3, s_rigger,   1, 0, 1,          {1, 3, 4, 5, 7, 0},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_galmino, s2x3, s_galaxy,   1, 0, 1,          {3, 4, 5, 7, 0, 1},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_qtgminb, s2x3, s_qtag,     1, 0, 1,          {1, 2, 4, 5, 6, 0},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_fcpl12, s2x2, s4x4,     0x14, 0, 1,          {1, 2, 5, 6},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_fcpl12,    fx0},
   {fx_fcpl23, s2x2, s4x4,    0x3C0, 0, 1,          {13, 14, 1, 2},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_fcpl23,    fx0},
   {fx_fcpl34, s2x2, s4x4,    0x140, 0, 1,          {13, 14, 9, 10},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_fcpl34,    fx0},
   {fx_fcpl41, s2x2, s4x4,     0x3C, 0, 1,          {9, 10, 5, 6},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_fcpl41,    fx0},
   {fx_foo55d, s1x4, s1x8,        0, 0, 1,    {0, 2, 4, 6},
    fx0,          fx0,          fx_foo55d,    fx0, fx_f1x3zzd,   fx0,    fx_bar55d,    fx_bar55d},
   {fx_fgalctb, s2x2, s_galaxy,    0, 0, 1,   {1, 3, 5, 7},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_fgalctb,   fx_fgalctb},
   {fx_f3x1ctl, s1x4, s3x1dmd,     0, 0, 1,   {1, 2, 5, 6},
    fx0,          fx0,          fx_f3x1ctl,   fx0, fx0,          fx0,    fx_fgalctb,   fx_fgalctb},
   {fx_f2x2pl, s2x2, s2x2,        0, 0, 1,         {0, 1, 2, 3},
    fx0, fx0,             fx_f1x4pl, fx_f1x4pl,    fx_fdmdpl, fx_fdmdpl,    fx_f2x2pl, fx_f2x2pl},
   {fx_f1x4pl, s1x4, s1x4,        0, 0, 1,         {0, 1, 2, 3},
    fx0, fx0,             fx_f1x4pl, fx_f1x4pl,    fx_fdmdpl, fx_fdmdpl,    fx_f2x2pl, fx_f2x2pl},
   {fx_fdmdpl, sdmd, sdmd,        0, 0, 1,         {0, 1, 2, 3},
    fx0, fx0,             fx_f1x4pl, fx_f1x4pl,    fx_fdmdpl, fx_fdmdpl,    fx_f2x2pl, fx_f2x2pl},
   {fx_f1x2pl, s1x2, s1x2,        0, 0, 1,               {0, 1},
    fx0, fx_f1x2pl,             fx0, fx0,    fx0, fx0,                        fx0, fx0},
   {fx_f3x1d_2, s1x2, s3x1dmd,     1, 0, 1,          {3, 7},
    fx0, fx0,                   fx0, fx0,    fx0, fx0,                        fx0, fx0},
   {fx_f1x8_88, s1x2, s1x8,        0, 0, 1,          {3, 7},
    fx0, fx_f3x1d_2,            fx0, fx0,    fx0, fx0,                        fx0, fx0},
   {fx_f1x8_22, s1x2, s1x8,        0, 0, 1,          {1, 5},
    fx0, fx_f3x1d_2,            fx0, fx0,    fx0, fx0,                        fx0, fx0},
   {fx_f1x8_11, s1x2, s1x8,        0, 0, 1,          {0, 4},
    fx0, fx_f3x1d_2,            fx0, fx0,    fx0, fx0,                        fx0, fx0},
   {fx_fdqtagzzz, s2x3, s_qtag,    1, 0, 1, {1, 3, 4, 5, 7, 0},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f1x8_77_3, s1x2, s1x8,      0, 0, 3,          {0, 1, 2, 6, 5, 4},
    fx0, fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f1x8_77, s1x6, s1x8,        0, 0, 1,          {0, 1, 2, 4, 5, 6},
    fx0,          fx0,          fx_f1x8_77,   fx0, fx0,          fx0,    fx_fdrhgl_bb, fx0},
   {fx_fdrhgl_bb, s2x3, s_dhrglass,  0, 1, 1, {0, 3, 1, 4, 7, 5},
    fx0,          fx0,          fx_f1x8_77,   fx_f3x1d77, fx0,   fx0,    fx_fdrhgl_bb, fx_fdqtagzzz},
   {fx_f3x1d77, s1x6, s3x1dmd,     0, 1, 1, {0, 1, 2, 4, 5, 6},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f1x8_bb, s1x6, s1x8,        0, 0, 1,          {0, 1, 3, 4, 5, 7},
    fx0,          fx0,          fx_f1x8_bb,   fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f1x8_dd, s1x6, s1x8,        0, 0, 1,          {0, 3, 2, 4, 7, 6},
    fx0,          fx0,          fx_f1x8_dd,   fx0, fx0,          fx0,    fx0,          fx0},
   {fx_foo99d, s1x4, s1x8,         0, 0, 1,  {0, 3, 4, 7},
    fx0,          fx0,          fx_foo99d,    fx0, fx0,          fx0,    fx_f2x4endd,  fx_f2x4endd},
   {fx_foo66d, s1x4, s1x8,         0, 0, 1,    {1, 2, 5, 6},
    fx0,          fx0,          fx_foo66d,    fx_f1x4xv, fx_f1x3yyd, fx0, fx_bar55d,   fx_bar55d},
   {fx_f1x8ctr, s1x4, s1x8,        0, 0, 1,    {3, 2, 7, 6},
    fx0,          fx0,          fx_f1x8ctr,   fx0, fx0,          fx0,    fx_bar55d,    fx_bar55d},
   {fx_fqtgctr, s1x4, s_qtag,      0, 0, 1,    {6, 7, 2, 3},
    fx0,          fx0,          fx_fqtgctr,   fx0, fx0,          fx0,    fx_bar55d,    fx_bar55d},
   {fx_fxwve, s1x4, s_crosswave, 0, 0, 1,  {0, 1, 4, 5},
    fx0,          fx0,          fx_fxwve,     fx_f1x8endd, fx0,  fx0,    fx_f2x4endd,  fx_f2x4endd},
   {fx_fboneendd, s2x2, s_bone,    0, 1, 1,   {0, 1, 4, 5},
    fx0,          fx0,          fx_f1x8endd,  fx0, fx0,          fx0,    fx_fboneendd, fx_fqtgend},
   {fx_fqtgend, s2x2, s_qtag,      0, 2, 1, {0, 1, 4, 5},
    fx0,          fx0,          fx_f1x8endd,  fx0, fx0,          fx0,    fx_fqtgend,   fx_fboneendd},
   {fx_f1x6endd, s1x4, s1x6,        0, 0, 1,          {0, 1, 3, 4},
    fx0, fx0,           fx_f1x6endd, fx_f1x2dd,    fx0, fx0,                      fx0, fx0},
   {fx_f1x2dd, s1x4, s_1x2dmd,    0, 0, 1,   {0, 1, 3, 4},
    fx0, fx0,             fx_f1x2dd, fx_f1x6endd,    fx0, fx0,                fx_f2x3od, fx_fbn6ndd},
   {fx_fbn6ndd, s2x2, s_bone6,     0, 1, 1,    {0, 1, 3, 4},
    fx0, fx0,                   fx0, fx0,    fx0, fx0,               fx_fbn6ndd, fx_f2x3od},
   {fx_f2x3od, s2x2, s2x3,        0, 1, 1,   {0, 2, 3, 5},
    fx0, fx0,             fx_f1x2dd, fx_f1x6endd,    fx0, fx0,                fx_f2x3od, fx_fbn6ndd},
   {fx_fdmded, s1x2, sdmd,        0, 1, 1,   {0, 2},
    fx0, fx0,                   fx0, fx0,    fx0, fx0,                fx0, fx0},
   {fx_f1x4ed, s1x2, s1x4,        0, 1, 1,   {0, 2},
    fx0, fx_fdmded,       fx0, fx0,    fx0, fx0,                fx0, fx0},
   {fx_fdrhgl1, s2x2, s_dhrglass,  0, 1, 1,   {0, 1, 4, 5},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx_fdrhgl1,   fx_fqtgend},
   {fx_f1x8endd, s1x4, s1x8,        0, 0, 1,  {0, 1, 4, 5},
    fx0,          fx0,          fx_f1x8endd,  fx_fxwve, fx0,     fx0,    fx_f2x4endd,  fx_f2x4endd},
   {fx_f1x8endo, s1x2, s1x8,        0, 0, 2,          {0, 1, 5, 4},
    fx0,  fx_fboneendo, fx0,          fx0, fx0,          fx0,    fx0,          fx0},

   {fx_fbonectr, s1x4, s_bone,      0, 0, 1,    {6, 7, 2, 3},
    fx0,          fx0,          fx_fbonectr,  fx0, fx0,          fx0,    fx_bar55d,    fx_bar55d},
   {fx_fbonetgl, s_bone6, s_bone,   0, 0, 1,          {0, 1, 3, 4, 5, 7},
    fx0,          fx0,          fx_f1x8_77,   fx0, fx0,          fx0,    fx0,          fx0},
   {fx_frigtgl, s_short6, s_rigger,1, 0, 1,          {1, 2, 4, 5, 6, 0},
    fx0,          fx0,          fx0,          fx_f1x8_77, fx0,   fx0,    fx0,          fx0},
   {fx_fboneendo, s1x2, s_bone,     1, 0, 2,          {0, 5, 1, 4},
    fx0, fx_f1x8endo,  fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_frigendd, s1x4, s_rigger,    0, 0, 1,  {6, 7, 2, 3},
    fx0,          fx0,          fx_frigendd,  fx0, fx0,          fx0,    fx_f2x4endd,  fx_f2x4endd},
   {fx_frigctr, s2x2, s_rigger,     0, 0, 1,   {0, 1, 4, 5},
    fx0,          fx0,          fx_f1x8ctr,   fx0, fx0,          fx0,    fx_frigctr,   fx_frigctr},
   {fx_f2x4ctr, s2x2, s2x4,         0, 0, 1,   {1, 2, 5, 6},
    fx0,          fx0,          fx_fbonectr,  fx0, fx0,          fx0,    fx_f2x4ctr,   fx_f2x4ctr},
   {fx_f2x4lr, s2x2, s2x4,        1, 0, 1,          {3, 4, 5, 2},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f2x4rl, s2x2, s2x4,        1, 0, 1,          {1, 6, 7, 0},
    fx0,          fx0,          fx0,          fx0, fx0,          fx0,    fx0,          fx0},
   {fx_f4dmdiden, s4dmd, s4dmd,     0, 0, 1, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   // The next 50 or so are unsymmetrical.

   // In a dmd+line, doing something to the dmd.
   {fx_1x5p1d, sdmd, splinepdmd,    0, 0, 0x101, {4, 5, 6, 7},
    fx0, fx0,                   fx_f1x8hif, fx_trngl8a,     fx0, fx_5p1x1d,             fx0, fxlinboxa},
   // In a dmd+line, doing something to the line.
   {fx_1x5p1e, s1x4, splinepdmd,    0, 0, 1, {0, 1, 3, 2},
    fx0, fx0,                   fx0, fx_linpdma,    fx_fdmdlowf, fxdmdpdmda,            fx0, fxboxpdmda},
   // In a box+line, doing something to the box.
   {fx_1x5p1f, s2x2, slinebox,      0, 0, 1, {4, 5, 6, 7},
    fx0, fx0,            fx_f1x8hif, fx_f2x4near,     fx_1x5p1y, fx_5p1x1d,             fx0, fx0},
   // In a box+line, doing something to the line.
   {fx_1x5p1g, s1x4, slinebox,      0, 0, 1, {0, 1, 3, 2},
    fx0, fx0,                   fx0, fx_f2x4far,     fxboxpdmdd, fx_boxdmd,     fx_f2x4left, fx_f2x4left},


   // In a line2box, doing something to the box of 6.
   {fx_l2b1,   s2x3, sline2box,     0, 0, 1, {2, 3, 4, 5, 6, 7},
    fx0, fx0,            fx_f1x8hi6, fx0,                   fx0, fx0,                   fx0, fx0},

   // In a line2box, doing something to the line of 2.
   {fx_l2b2,   s1x2, sline2box,     0, 0, 1, {0, 1},
    fx0, fx_f2x4pos1,           fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   // In a line2box, doing something to the far 2 in the box of 6.
   {fx_l2b3,   s1x2, sline2box,     1, 0, 1, {4, 5},
    fx0, fx_l2b4,               fx0, fx0,                   fx0, fx0,                   fx0, fx0},




   // In a line6box, doing something to the line of 6.
   {fx_l6b1,   s1x6, sline6box,     0, 0, 1, {0, 1, 2, 5, 4, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,           fx_f2x4pos8, fx0},

   // In a line6box, doing something to the line of 2.
   {fx_l6b2,   s1x2, sline6box,     1, 0, 1, {6, 7},
    fx0, fx_f1x8hi2,            fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   // In a line6box, doing something to the near 2 in the line of 6.
   {fx_l6b3,   s1x2, sline6box,     0, 0, 1, {0, 1},
    fx0, fx_l6b4,               fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_l6b4,   s1x2, s_bone,        1, 0, 1, {0, 5},
    fx0, fx_l6b4,               fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_l2b4,   s1x2, s1x6,      0, 0, 1, {4, 3},
    fx0, fx_f2x3a14,                fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_l2b5,   s1x2, s1x6,      0, 0, 1, {0, 1},
    fx0, fx_f2x3a41,                fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_dbt1,   s1x2, sdbltrngl4, 0, 0, 1, {4, 5},
    fx0, fx_dbt1a,              fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_dbt2,   s1x2, sdbltrngl4, 1, 0, 1, {2, 3},
    fx0, fx_dbt2a,              fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_dbt3,   s1x2, sdbltrngl4, 0, 0, 1, {0, 1},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_dbt4,   s1x2, sdbltrngl4, 1, 0, 1, {6, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_dbt1a,  s1x2, sline2box,  1, 0, 1, {3, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_dbt2a,  s1x2, sline6box,  0, 0, 1, {2, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_dbt1b,  s1x2, sdbltrngl4, 1, 0, 2, {2, 3, 6, 7},
    fx0, fx_f1x8lodbt4,         fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_dbt2b,  s1x2, sdbltrngl4, 0, 0, 2, {0, 1, 4, 5},
    fx0, fx_f2x4pos02,          fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_dbt1d,  sdmd, sdbltrngl4, 0, 0, 1, {1, 2, 4, 3},
    fx0, fx0,                   fx_dbt1l, fx0,              fx0, fx0,                   fx0, fx0},
   {fx_dbt3t, s_trngl, sdbltrngl4, 1, 0, 1, {5, 6, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_dbt6t, s_bone6, sdbltrngl4, 0, 0, 1, {2, 6, 5, 7, 3, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_dbt1l,  s1x4, s1x8,       0, 0, 1, {1, 3, 6, 2},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_1x5p1z, sdmd, splinepdmd, UINT32_C(0x80000002), 0, 1, {6, 7, 4, 5},
    fx0, fx0,                   fx_1x5p1d, fx0,             fx0, fx0,                   fx0, fx0},
   {fx_1x5p1y, sdmd, splinepdmd, UINT32_C(0x80000000), 0, 1, {4, 5, 6, 7},
    fx0, fx0,                   fx_1x5p1d, fx0,             fx0, fx0,                   fx0, fx0},

   {fx_5p1x1d, sdmd, splinedmd,  UINT32_C(0x80000001), 0, 1, {5, 6, 7, 4},
    fx0, fx0,                   fx_trngl8a, fx_f1x8hif,     fx0, fx_1x5p1d,             fx0, fxlinboxb},
   {fx_plndmda, s1x4, splinedmd,    0, 0, 1, {0, 1, 3, 2},
    fx0, fx0,                   fx0, fx_lndmda,      fxdmdpdmdb, fx_fqtglowf,           fx0, fxboxdmdc},
   {fx_5p1x1z, sdmd, splinedmd,  UINT32_C(0x80000003), 0, 1, {7, 4, 5, 6},
    fx0, fx0,                   fx_5p1x1d, fx0,             fx0, fx0,                   fx0, fx0},

   {fx_linpdma, s1x4, slinepdmd,    0, 0, 0x101, {4, 5, 6, 7},
    fx0, fx0,                   fx0, fx_1x5p1e,      fxdmdpdmda, fx_fdmdlowf,           fx0, fxboxpdmdc},
   {fx_lndmda, s1x4, slinedmd,      0, 0, 0x101, {4, 5, 6, 7},
    fx0, fx0,                   fx0, fx_plndmda,    fx_fqtglowf, fxdmdpdmdc,            fx0, fxboxdmda},
   {fx_linpdmd, sdmd, slinepdmd,    1, 0, 1, {2, 3, 0, 1},
    fx0, fx0,           fx_f1x8lowf, fx_f2x4near,           fx0, fx_lndmd9,             fx0, fx_f2x4left},
   {fx_lndmd9, sdmd, slinedmd,            0, 0, 1, {0, 1, 2, 3},
    fx0, fx0,           fx_f2x4near, fx_trngl8b,            fx0, fx_linpdmd,            fx0, fx_f2x4lr},
   {fx_linpdm8, sdmd, slinepdmd, UINT32_C(0x80000003), 0, 1, {0, 1, 2, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_lndmd8, sdmd, slinedmd,   UINT32_C(0x80000002),   0, 1, {2, 3, 0, 1},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_boxdma, s2x2, sboxdmd,    UINT32_C(0x80000000), 0, 1, {4, 5, 6, 7},
    fx0, fx0,                   fx_lndmda, fx0,     fx_fqtglowf, fx0,                   fx0, fx0},
   {fx_boxdmb, sdmd, sboxdmd,       0, 0, 1, {0, 1, 2, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx_f2x4lr},
   {fx_boxdmc, sdmd, sboxdmd,    UINT32_C(0x80000000), 0, 1, {0, 1, 2, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_boxdmd, sdmd, sboxdmd,       0, 0, 0x203, {0, 1, 2, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_boxpdma, s2x2, sboxpdmd,  UINT32_C(0x80000000), 0, 1, {4, 5, 6, 7},
    fx0, fx0,                   fx_linpdma, fx0,            fx0, fx0,                   fx0, fx0},
   {fx_boxpdmb, sdmd, sboxpdmd,      0, 0, 1, {0, 1, 2, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_beehive1, sbeehive, sboxpdmd, 2, 0, 1, {6, 2, 7, 4, 5},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_beehive2, sbeehive, s_spindle, 3, 0, 1, {6, 7, 0, 1, 5},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_beehive3, sbeehive, s_spindle, 1, 0, 1, {2, 3, 4, 5, 1},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_spnnrtgl, s_trngl, s_spindle, 3, 0, 1, {3, 4, 2},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_spnfrtgl, s_trngl, s_spindle, 1, 0, 1, {7, 0, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_galnrtgl, s_trngl, s_galaxy, 0, 0, 1, {6, 7, 5},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_galfrtgl, s_trngl, s_galaxy, 2, 0, 1, {2, 3, 1},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_galnrvee, svee, s_galaxy, 2, 0, 1, {4, 5, 6, 7, 0},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_galfrvee, svee, s_galaxy, 0, 0, 1, {0, 1, 2, 3, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_trngl8a, s1x4, s_trngl8,  UINT32_C(0x80000000), 0, 1, {4, 5, 7, 6},
    fx0, fx0,                   fx0, fx_f1x8hif,      fx_5p1x1d, fx_1x5p1d,             fx0, fx0},
   {fx_trngl8b, s1x4, s_trngl8,     1, 0, 1, {3, 2, 0, 1},
    fx0, fx0,                   fx0, fx_f2x4near,    fx_linpdmd, fx_lndmd9,     fx_f2x4left, fx_f2x4left},

   {fxdmdpdmda, sdmd, sdmdpdmd,  UINT32_C(0x80000000), 0, 1, {4, 5, 6, 7},
    fx0, fx0,            fx_linpdma, fx_f1x8hif,            fx0, fx_1x5p1d,             fx0, fx0},
   {fxdmdpdmdb, sdmd, sdmdpdmd,     1, 0, 1, {2, 3, 0, 1},
    fx0, fx0,            fx_plndmda, fx0,                   fx0, fx_lndmd9,     fx_f2x4left, fx_f2x4left},
   {fxdmdpdmdc, sdmd, sdmdpdmd,  UINT32_C(0x80000003), 0, 1, {0, 1, 2, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fxboxdmda,  s2x2, sboxdmd,      0, 0, 1, {4, 5, 6, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fxboxdmdb,  s2x2, sboxdmd,   UINT32_C(0x80000000), 0, 1, {4, 5, 6, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fxboxdmdc,  s2x2, sboxdmd,   UINT32_C(0x80000001), 0, 1, {5, 6, 7, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fxboxpdmda, s2x2, sboxpdmd,  UINT32_C(0x80000001), 0, 1, {5, 6, 7, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fxboxpdmdb, s2x2, sboxpdmd,     1, 0, 1, {5, 6, 7, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fxboxpdmdc, s2x2, sboxpdmd,     0, 0, 1, {4, 5, 6, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fxboxpdmdd, sdmd, sboxpdmd, 0x2AB, 0, 1, {2, 3, 0, 1},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fxboxpdmde, sdmd, sboxpdmd,  UINT32_C(0x80000001), 0, 1, {2, 3, 0, 1},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fxlinboxa,  s2x2, slinebox,     0, 0, 1, {4, 5, 6, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fxlinboxb,  s2x2, slinebox,     1, 0, 1, {5, 6, 7, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx323a,   s1x2, s_323,          0, 0, 2, {0, 1, 5, 4},
    fx0, fxntrgla,              fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx323b,   s1x2, s_323,          0, 0, 2, {1, 2, 6, 5},
    fx0, fxntrglb,              fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fxntrgla, s1x2, s_ntrglcw,      0, 0, 2, {2, 3, 7, 6},
    fx0, fx323c,                fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fxntrglb, s1x2, s_ntrglccw,     0, 0, 2, {5, 4, 0, 1},
    fx0, fx323d,                fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   // For unwinding the above.  That is, need to get out with different maps.
   {fx323c,   s1x2, s_323,          0, 0, 2, {5, 4, 0, 1},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx323d,   s1x2, s_323,          0, 0, 2, {6, 5, 1, 2},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx23232a,   s1x2, s_23232,     1, 0, 2, {1, 5, 11, 7},
    fx0, fx_foocc,              fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx23232b,   s1x2, s_23232,     1, 0, 2, {0, 11, 5, 6},
    fx0, fx_foo33,              fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx23232c,   s1x2, s_23232,     1, 0, 1, {3, 9},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx23232d,   s1x2, s_23232,     0, 0, 2, {2, 3, 9, 8},
    fx23232u, fx23232w,         fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx23232e,   s1x2, s_23232,     0, 0, 2, {3, 4, 10, 9},
    fx23232v, fx23232x,         fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   // For unwinding the above.
   {fx23232u,   s1x2, s_c1phan,     0, 0, 2, {0, 2, 10, 8},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx23232v,   s1x2, s_c1phan,     0, 0, 2, {7, 5, 13, 15},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx23232w,   s1x2, s_c1phan,     1, 0, 2, {1, 3, 11, 9},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx23232x,   s1x2, s_c1phan,     1, 0, 2, {4, 6, 14, 12},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_f1x8lowf, s1x4, s1x8,        0, 0, 1,  {0, 1, 2, 3},
    fx0, fx0,                   fx0, fx_f2x4far,          fx_1x5p1z,fx_5p1x1z,  fx_f2x4left, fx_f2x4left},
   {fx_f1x8hif,  s1x4, s1x8,        0, 0, 1, {6, 7, 4, 5},
    fx0, fx0,                   fx0, fx_f2x4near,         fx_1x5p1d,fx_5p1x1d, fx_f2x4right, fx_f2x4right},


   // New stuff.
   {fx_f1x8low2, s1x2, s1x8,        0, 0, 1, {0, 1},
    fx0, fx_f2x4pos1,           fx0, fx0,                   fx0, fx0,           fx0, fx0},
   {fx_f1x8hi2,  s1x2, s1x8,        0, 0, 1, {5, 4},
    fx0, fx_f2x4pos4,           fx0, fx0,                   fx0, fx0,           fx0, fx0},

   {fx_f1x8low6, s1x6, s1x8,        0, 0, 1, {0, 1, 3, 7, 6, 2},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,           fx_f2x4pos8, fx0},
   {fx_f1x8hi6,  s1x6, s1x8,        0, 0, 1, {3, 2, 6, 4, 5, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,           fx_f2x4pos5, fx0},

   {fx_f1x8lodbt4, s1x2, s1x8,      0, 0, 2, {3, 2, 5, 4},
    fx0, fx_f2x4pos13,          fx0, fx0,                   fx0, fx0,           fx0, fx0},
   {fx_f1x8hidbt4,  s1x2, s1x8,     0, 0, 2, {0, 1, 6, 7},
    fx0, fx_f2x4pos02,          fx0, fx0,                   fx0, fx0,           fx0, fx0},



   {fx_fqtglowf, sdmd, s_qtag,      1, 0, 1, {0, 7, 5, 6},
    fx0, fx0,            fx_f2x4far, fx_f1x8lowf,           fx0, fx_fdmdlowf,           fx0, fxboxdmda},
   {fx_fqtghif,  sdmd, s_qtag,      1, 0, 1, {1, 2, 4, 3},
    fx0, fx0,           fx_f2x4near, fx_f1x8hif,            fx0, fx_fdmdhif,            fx0, fxboxdmdb},

   {fx_fdmdlowf, sdmd, s_ptpd,      0, 0, 1, {0, 1, 2, 3},
    fx0, fx0,           fx_f1x8lowf, fx_f2x4far,            fx0, fx_fqtglowf,           fx0, fxboxpdmda},
   {fx_fdmdhif,  sdmd, s_ptpd,      0, 0, 1, {6, 7, 4, 5},
    fx0, fx0,            fx_f1x8hif, fx_f2x4near,           fx0, fx_fqtghif,            fx0, fxboxpdmdb},

   {fx_fdmdlow3, s_trngl, s_ptpd,   1, 0, 1, {0, 1, 3},
    fx0, fx_f1x8low3,           fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fdmdhi3,  s_trngl, s_ptpd,   3, 0, 1, {4, 5, 7},
    fx0, fx_f1x8hi3,            fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f1x8low3, s1x3,    s1x8,     0, 0, 1, {0, 1, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f1x8hi3, s1x3,     s1x8,  0xA8, 0, 1, {4, 5, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_f2x4far, s1x4, s2x4,         0, 0, 1, {0, 1, 3, 2},
    fx0, fx0,                   fx0, fx_f1x8hif,      fx_lndmd8, fx_linpdm8,             fx_f2x4rl, fx_f2x4rl},
   {fx_f2x4near, s1x4, s2x4,        0, 0, 1, {7, 6, 4, 5},
    fx0, fx0,                   fx0, fx_f1x8lowf,     fx_lndmd9, fx_linpdmd,      fx_f2x4lr, fx_f2x4lr},
   {fx_f2x4pos1, s1x2, s2x4,        1, 0, 1, {0, 7},
    fx0, fx_f1x8low2,           fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4pos2, s1x2, s2x4,        1, 0, 1, {1, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4pos3, s1x2, s2x4,        1, 0, 1, {2, 5},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4pos4, s1x2, s2x4,        1, 0, 1, {3, 4},
    fx0, fx_f1x8hi2,            fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4pos5, s2x3, s2x4,        0, 0, 1, {1, 2, 3, 4, 5, 6},
    fx0, fx0,                   fx_f1x8hi6, fx0,            fx0, fx0,                   fx0, fx0},
   {fx_f2x4pos6, s1x2, s2x4,        1, 0, 3, {0, 7, 2, 5, 3, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4pos7, s1x2, s2x4,        1, 0, 3, {0, 7, 1, 6, 3, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4pos8, s2x3, s2x4,        0, 0, 1, {0, 1, 2, 5, 6, 7},
    fx0, fx0,                   fx_f1x8low6, fx0,           fx0, fx0,                   fx0, fx0},

   {fx_f2x4pos02, s1x2, s2x4,       1, 0, 2, {0, 7, 2, 5},
    fx0, fx_f1x8hidbt4,         fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4pos13, s1x2, s2x4,       1, 0, 2, {1, 6, 3, 4},
    fx0, fx_f1x8lodbt4,         fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_f2x4posa, s1x2, s2x4,        0, 0, 1, {0, 1},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posb, s1x2, s2x4,        0, 0, 1, {1, 2},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posc, s1x2, s2x4,        0, 0, 1, {2, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posd, s1x2, s2x4,        0, 0, 1, {5, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4pose, s1x2, s2x4,        0, 0, 1, {6, 5},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posf, s1x2, s2x4,        0, 0, 1, {7, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posp, s1x2, s2x4,        0, 0, 2, {0, 1, 6, 5},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posq, s1x2, s2x4,        0, 0, 2, {2, 3, 6, 5},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posr, s1x2, s2x4,        0, 0, 2, {1, 2, 7, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4poss, s1x2, s2x4,        0, 0, 2, {1, 2, 5, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posu, s1x2, s2x4,        0, 0, 3, {0, 1, 2, 3, 5, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posv, s1x2, s2x4,        0, 0, 3, {0, 1, 2, 3, 7, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posw, s1x2, s2x4,        0, 0, 3, {0, 1, 5, 4, 7, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posx, s1x2, s2x4,        0, 0, 3, {2, 3, 5, 4, 7, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posy, s1x2, s2x4,        0, 0, 3, {0, 1, 2, 3, 6, 5},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4posz, s1x2, s2x4,        0, 0, 3, {1, 2, 7, 6, 5, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_f2x4left, s2x2, s2x4,        0, 0, 1, {0, 1, 6, 7},
    fx0, fx0,           fx_f1x8lowf, fx_f2x4far,     fxboxpdmdd, fx_boxdmd,     fx_f2x4left, fx_f2x4left},
   {fx_f2x4right, s2x2, s2x4,       0, 0, 1, {2, 3, 4, 5},
    fx0, fx0,            fx_f1x8hif, fx_f2x4near,    fxboxpdmde, fx_boxdmc,    fx_f2x4right, fx_f2x4right},

   {fx_f2x4dleft, s2x2, s2x4,       0, 0, 1, {0, 2, 5, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx_f2x4dleft, fx_f2x4dleft},
   {fx_f2x4dright, s2x2, s2x4,      0, 0, 1, {1, 3, 4, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx_f2x4dright, fx_f2x4dright},

   {fx_f2zzrdsc, s2x4, s2x4,        0, 0, 1, {0, 1, 2, 3, 4, 5, 6, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2zzrdsd, s2x4, s4x4,        0, 0, 1, {12, 13, 14, 0, 4, 5, 6, 8},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f288rdsd, s1x8, s1x10,       0, 0, 1, {0, 1, 3, 2, 5, 6, 8, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2yyrdsc, s1x8, s1x8,        0, 0, 1, {0, 1, 2, 3, 4, 5, 6, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x5rdsd, s2x4, s2x5,        0, 0, 1, {0, 1, 3, 4, 5, 6, 8, 9},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2qt1dsc, s_qtag, s3x6,       0, 0, 1, {2, 3, 6, 8, 11, 12, 15, 17},
    fx0, fx0,                   fx0, fx0,                   fx_f2qt1dsc, fx0,           fx0, fx0},
   {fx_f2qt1dsd, s_qtag, s3x6,       0, 0, 1, {1, 4, 6, 7, 10, 13, 15, 16},
    fx0, fx0,                   fx0, fx0,                   fx_f2qt1dsd, fx0,           fx0, fx0},
   {fx_f2qt2dsc, s_qtag, s3x8,       0, 0, 1, {3, 4, 8, 11, 15, 16, 20, 23},
    fx0, fx0,                   fx0, fx0,                   fx_f2qt2dsc, fx0,           fx0, fx0},
   {fx_f2qt2d99, s_qtag, s3x8,       0, 0, 1, {2, 5, 8, 10, 14, 17, 20, 22},
    fx0, fx0,                   fx0, fx0,                   fx_f2qt2d99, fx0,           fx0, fx0},
   {fx_f2x6ndsc, s2x4, s2x6,        0, 0, 1, {0, 2, 3, 5, 6, 8, 9, 11},
    fx0, fx0,                   fx0, fx_f2yyrdsc,           fx_f2qt1dsc, fx0,           fx0, fx_f2zzrdsc},
   {fx_f2x6ndsd, s2x4, s2x6,        0, 0, 1, {0, 1, 4, 5, 6, 7, 10, 11},
    fx0, fx0,                   fx_f288rdsd, fx0,           fx_f2qt1dsd, fx0,           fx0, fx_f2zzrdsd},
   {fx_fdmdndx, s3x4, s3x6,         0, 0, 1, {0, -1, -1, 5, 6, 8, 9, -1, -1, 14, 15, 17},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fdmdnd, s3x4, s4dmd,         0, 0, 1, {0, -1, -1, 3, 5, 7, 8, -1, -1, 11, 13, 15},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx_fdmdndx, fx0},
   {fx_f3x4ndsd, s2x4, s3x4,        0, 0, 1, {0, 1, 2, 3, 6, 7, 8, 9},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx_f2x5rdsd},
   {fx_f2x8nd99, s2x4, s2x8,        0, 0, 1, {0, 3, 4, 7, 8, 11, 12, 15},
    fx0, fx0,                   fx0, fx_f2yyrdsc,           fx_f2qt2dsc, fx0,           fx0, fx_f2zzrdsc},
   {fx_f2x8nda5, s2x4, s2x8,        0, 0, 1, {0, 2, 5, 7, 8, 10, 13, 15},
    fx0, fx0,                   fx0, fx0,                   fx_f2qt2d99, fx0,           fx0, fx_f2zzrdsd},
   {fx_f4x6ndoo, s4x4, s4x6,        0, 0, 1, {5, 6, 23, 8, 12, 14, 15, 21, 17, 18, 11, 20, 0, 2, 3, 9},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f4x6ndxx, s4x4, s4x6,        0, 0, 1, {5, 6, 23, 8, 12, 14, 15, 21, 17, 18, 11, 20, 0, 2, 3, 9},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f1x8nd96, s1x4, s1x8,        0, 0, 1, {1, 2, 4, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f1x8nd69, s1x4, s1x8,        0, 0, 1, {0, 3, 5, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f1x8nd41, s1x2, s1x8,        0, 0, 1, {0, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f1x8nd82, s1x2, s1x8,        0, 0, 1, {1, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f1x8nd28, s1x2, s1x8,        0, 0, 1, {3, 5},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f1x8nd14, s1x2, s1x8,        0, 0, 1, {2, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f1x10ndsc, s1x8, s1x10,      0, 0, 1, {0, 1, 4, 3, 5, 6, 9, 8},
    fx0, fx0,                   fx0, fx_f2yyrdsc,           fx0, fx0,                   fx_f2zzrdsc, fx_f2zzrdsc},
   {fx_f1x10ndsd, s1x8, s1x10,      0, 0, 1, {0, 2, 4, 3, 5, 7, 9, 8},
    fx0, fx0,                   fx0, fx_f2yyrdsc,           fx0, fx0,                   fx_f2zzrdsc, fx_f2zzrdsc},
   {fx_f1x10ndse, s1x8, s1x10,      0, 0, 1, {0, 1, 4, 2, 5, 6, 9, 7},
    fx0, fx0,                   fx0, fx_f2yyrdsc,           fx0, fx0,                   fx_f2x6ndsc, fx_f2zzrdsc},
   {fx_z1x4u, s1x4, s1x4,               0, 0, 1, {0, 1, 2, 3},
    fx0, fx0,                   fx_z1x4u, fx0,              fx0, fx0,                   fx0, fx0},
   {fx_z2x3a, s2x3, s2x3,               0, 0, 1, {0, 1, 2, 3, 4, 5},
    fx0, fx0,                   fx_z1x4u, fx_z1x4u,         fx0, fx0,                   fx_z2x3a, fx_z2x3a},
   {fx_z2x3b, s2x3, s2x3,               0, 0, 1, {0, 1, 2, 3, 4, 5},
    fx0, fx0,                   fx_z1x4u, fx_z1x4u,         fx0, fx0,                   fx_z2x3b, fx_z2x3b},
   {fx_fgalpts, sdmd, s_galaxy,         0, 0, 1, {0, 2, 4, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4endd, s2x2, s2x4,            0, 1, 1, {0, 3, 4, 7},
    fx0, fx0,                   fx_frigendd, fx_frigendd,   fx_fgalpts, fx_fgalpts,     fx_f2x4endd, fx_fqtgend},
   {fx_fhrgl1, sdmd, s_2x1dmd,          1, 0, 1, {2, 3, 5, 0},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fhrgl2, sdmd, s_2x1dmd,          0, 0, 1,          {0, 2, 3, 5},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fhrgle, s2x2, s_hrglass,         0, 2, 1, {0, 1, 4, 5},
    fx0, fx0,                   fx_frigendd, fx_frigendd,   fx_fhrgl1, fx_fhrgl2,       fx_fhrgle, fx_fqtgend},
   {fx_fptpdid, s_ptpd, s_ptpd,         0, 1, 1,          {0, 1, 2, 3, 4, 5, 6, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_phanigna, s_ntrgl6ccw, s_c1phan, 0, 0, 1,     {0, 2, 4, 8, 10, 12},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_phanignb, s_ntrgl6ccw, s_c1phan, 0, 0, 1,     {0, 2, 6, 8, 10, 14},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_phanignc, s_ntrgl6ccw, s_c1phan, 1, 0, 1,     {4, 6, 8, 12, 14, 0},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_phanignd, s_ntrgl6ccw, s_c1phan, 1, 0, 1,     {4, 6, 10, 12, 14, 2},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_phanigne, s_ntrgl6cw, s_c1phan,  0, 0, 1,      {1, 7, 5, 9, 15, 13},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_phanignf, s_ntrgl6cw, s_c1phan,  0, 0, 1,      {3, 7, 5, 11, 15, 13},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_phanigng, s_ntrgl6cw, s_c1phan,  1, 0, 1,      {5, 11, 9, 13, 3, 1},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_phanignh, s_ntrgl6cw, s_c1phan,  1, 0, 1,      {7, 11, 9, 15, 3, 1},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},

   {fx_f2x477, s2x3, s2x4,              0, 1, 1,          {0, 1, 2, 4, 5, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx_f2x477, fx0},
   {fx_f2x4ee, s2x3, s2x4,              0, 1, 1,          {1, 2, 3, 5, 6, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx_f2x4ee, fx0},
   {fx_f2x4bb, s2x3, s2x4,              0, 1, 1,          {0, 1, 3, 4, 5, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx_f2x4bb, fx0},
   {fx_f2x4dd, s2x3, s2x4,              0, 1, 1,          {0, 2, 3, 4, 6, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx_f2x4dd, fx0},
   {fx_fdhrgld1, s1x2, s_dhrglass,      0, 1, 2,          {0, 3, 7, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fdhrgld2, s1x2, s_dhrglass,      0, 1, 2,          {3, 1, 5, 7},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_f2x4endo, s1x2, s2x4,            1, 0, 2,          {0, 7, 3, 4},
    fx0, fx_f1x8endo,           fx_f4x4endo, fx0,           fx0, fx0,                   fx0, fx0},
   {fx_f4x4endo, s1x4, s4x4,            1, 0, 2,          {12, 10, 8, 9, 0, 1, 4, 2},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_bar55d, s2x2, s2x4,              0, 0, 1,          {1, 2, 5, 6},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fppaad, s1x2, s2x4,              0, 0, 2,          {1, 3, 7, 5},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_fpp55d, s1x2, s2x4,              0, 0, 2,          {0, 2, 6, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},


   {fx_4x5eight0, s1x2, s1x8,           0, 0, 4, {0, 1, 3, 2, 6, 7, 5, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_4x5eight1, s1x2, s2x4,           1, 0, 4, {0, 7, 1, 6, 2, 5, 3, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_4x5eight2, s1x2, s2x4,           3, 0, 4, {7, 0, 6, 1, 5, 2, 4, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_4x5eight3, s1x2, s1x8,           0, 0, 4, {0, 1, 3, 2, 6, 7, 5, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},


   {fx0}};


/*
   {fx_4x5eight0, s1x2, s1x8,           0, 0, 4, {0, 1, 3, 2, 6, 7, 5, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_4x5eight1, s1x2, s2x4,           1, 0, 4, {0, 7, 1, 6, 2, 5, 3, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_4x5eight2, s1x2, s2x4,           3, 0, 4, {7, 0, 6, 1, 5, 2, 4, 3},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},
   {fx_4x5eight3, s1x2, s1x8,           0, 0, 4, {0, 1, 3, 2, 6, 7, 5, 4},
    fx0, fx0,                   fx0, fx0,                   fx0, fx0,                   fx0, fx0},




*/


select::sel_item select::sel_init_table[] = {
   {LOOKUP_Z,                  s2x3,        066,    fx_z2x3a,      fx0, -1},
   {LOOKUP_Z,                  s2x3,        033,    fx_z2x3b,      fx0, -1},
   {LOOKUP_Z,                  sd2x7,      0x0C18,  fx_fd2x7za,    fx0, -1},
   {LOOKUP_Z,                  sd2x7,      0x060C,  fx_fd2x7zb,    fx0, -1},
   {LOOKUP_DIST_DMD,           s_rigger,    0x99,   fx_distrig3,   fx0, -1},
   {LOOKUP_DIST_DMD,           s_rigger,    0x66,   fx_distrig4,   fx0, -1},
   {LOOKUP_DIST_DMD,           s_rigger,    0x55,   fx_distrig7,   fx0, -1},
   {LOOKUP_DIST_DMD,           s_rigger,    0xAA,   fx_distrig8,   fx0, -1},
   {LOOKUP_DIST_DMD,           s_ptpd,      0x33,   fx_distptp7,   fx0, -1},
   {LOOKUP_DIST_DMD,           s_ptpd,      0x99,   fx_distptp8,   fx0, -1},
   {LOOKUP_DIST_DMD,           s_galaxy,    0x66,   fx_dgald1,     fx0, -1},
   {LOOKUP_DIST_DMD,           s_galaxy,    0xCC,   fx_dgald2,     fx0, -1},
   {LOOKUP_DIST_DMD,           s_galaxy,    0x33,   fx_dgald3,     fx0, -1},
   {LOOKUP_DIST_DMD,           s_galaxy,    0x99,   fx_dgald4,     fx0, -1},
   {LOOKUP_DIST_DMD,           s2x4,        0xAA,   fx_d2x4d1,     fx0, -1},
   {LOOKUP_DIST_DMD,           s2x4,        0x55,   fx_d2x4d2,     fx0, -1},
   {LOOKUP_DIST_DMD,           s4x4,        0x8181, fx_d4x4d1,     fx_d4x4d2,     0},
   {LOOKUP_DIST_DMD,           s4x4,        0x1818, fx_d4x4d3,     fx_d4x4d4,     4},
   {LOOKUP_DIST_DMD,           s_qtag,      0x66,   fx_ddmd1,      fx0, -1},
   {LOOKUP_DIST_DMD,           s_qtag,      0x55,   fx_ddmd2,      fx0, -1},
   {LOOKUP_DIST_BOX|LOOKUP_STAG_BOX, s2x4,  0xAA,   fx_d2x4c1,     fx0, -1},
   {LOOKUP_DIST_BOX|LOOKUP_STAG_BOX, s2x4,  0x55,   fx_d2x4c2,     fx0, -1},
   {LOOKUP_DIST_BOX|LOOKUP_DIAG_BOX, s2x4,  0x33,   fx_d2x4y1,     fx0, -1},
   {LOOKUP_DIST_BOX|LOOKUP_DIAG_BOX, s2x4,  0xCC,   fx_d2x4y2,     fx0, -1},
   {LOOKUP_DIST_BOX,           s3x4,       0x560,   fx_d3x4ub1,    fx0, -1},
   {LOOKUP_DIST_BOX,           s3x4,       0xA90,   fx_d3x4ub2,    fx0, -1},
   {LOOKUP_DIST_BOX,           s3x4,       0x42A,   fx_d3x4ub3,    fx0, -1},
   {LOOKUP_DIST_BOX,           s3x4,       0x815,   fx_d3x4ub4,    fx0, -1},
   {LOOKUP_DIST_BOX,           s3x4,       0xCC0,   fx_d3x4vb1,    fx0, -1},
   {LOOKUP_DIST_BOX,           s3x4,       0x330,   fx_d3x4vb2,    fx0, -1},
   {LOOKUP_DIST_BOX,           s3x4,       0xC0C,   fx_d3x4vb3,    fx0, -1},
   {LOOKUP_DIST_BOX,           s3x4,       0x033,   fx_d3x4vb4,    fx0, -1},
   {LOOKUP_DIST_BOX,           s4x4,      0x4444,   fx_d4x4b1,     fx0, -1},
   {LOOKUP_DIST_BOX,           s4x4,      0x2222,   fx_d4x4b2,     fx0, -1},
   {LOOKUP_DIST_BOX,           s4x4,      0x1818,   fx_d4x4b3,     fx0, -1},
   {LOOKUP_DIST_BOX,           s4x4,      0x8181,   fx_d4x4b4,     fx0, -1},
   {LOOKUP_DIST_BOX,           s_qtag,      0xAA,   fx_fqtgjj1,    fx0, -1},
   {LOOKUP_DIST_BOX,           s_qtag,      0x99,   fx_fqtgjj2,    fx0, -1},
   {LOOKUP_DIST_BOX,           s2x3,         066,   fx_f2x3jj1,    fx0, -1},
   {LOOKUP_DIST_BOX,           s2x3,         033,   fx_f2x3jj2,    fx0, -1},
   {LOOKUP_DIST_BOX,           s2x3,         055,   fx_f2x3jj3,    fx0, -1},
   {LOOKUP_DIST_BOX,           s_spindle,   0x66,   fx_fspindlfd,  fx0, -1},
   {LOOKUP_DIST_BOX,           s_spindle,   0x33,   fx_fspindlgd,  fx0, -1},
   {LOOKUP_DIST_BOX,           s_ntrgl6cw,   033,   fx_ftgl6cwd,   fx0, -1},
   {LOOKUP_DIST_BOX,           s_ntrgl6ccw,  066,   fx_ftgl6ccwd,  fx0, -1},
   {LOOKUP_DIST_CLW,           s_ntrgl6cw,   066,   fx_ftgl6cld,   fx0, -1},
   {LOOKUP_DIST_CLW,           s_ntrgl6ccw,  033,   fx_ftgl6ccld,  fx0, -1},
   {LOOKUP_DIAG_CLW|LOOKUP_DIST_CLW, s4x4,  0x0909, fx_d4x4l1,     fx_d4x4l2,     0},
   {LOOKUP_DIAG_CLW|LOOKUP_DIST_CLW, s4x4,  0x9090, fx_d4x4l3,     fx_d4x4l4,     4},
   {LOOKUP_DIST_CLW,           s_rigger,    0x99,   fx_distrig1,   fx0, -1},
   {LOOKUP_DIST_CLW,           s_rigger,    0x66,   fx_distrig2,   fx0, -1},
   {LOOKUP_DIST_CLW,           s_rigger,    0x55,   fx_distrig5,   fx0, -1},
   {LOOKUP_DIST_CLW,           s_rigger,    0xAA,   fx_distrig6,   fx0, -1},
   {LOOKUP_DIST_CLW,           s_ptpd,      0xCC,   fx_distptp1,   fx0, -1},
   {LOOKUP_DIST_CLW,           s_ptpd,      0x66,   fx_distptp2,   fx0, -1},
   {LOOKUP_DIST_CLW,           s_ptpd,      0x33,   fx_distptp3,   fx0, -1},
   {LOOKUP_DIST_CLW,           s_ptpd,      0x99,   fx_distptp4,   fx0, -1},
   {LOOKUP_OFFS_CLW|LOOKUP_DIST_CLW, spgdmdcw,  0xCC, fx_distpgdmd1, fx0, -1},
   {LOOKUP_OFFS_CLW|LOOKUP_DIST_CLW, spgdmdccw, 0xCC, fx_distpgdmd2, fx0, -1},
   {LOOKUP_DIST_CLW,           s_bone,      0x55,   fx_distbone1,  fx0, -1},
   {LOOKUP_DIST_CLW,           s_bone,      0x99,   fx_distbone2,  fx0, -1},
   {LOOKUP_DIST_CLW,           s_bone,      0x66,   fx_distbone5,  fx0, -1},
   {LOOKUP_DIST_CLW,           s_bone,      0xAA,   fx_distbone6,  fx0, -1},
   {LOOKUP_DIST_CLW,           s_hrglass,   0xAA,   fx_disthrg1,   fx0, -1},
   {LOOKUP_DIST_CLW,           s_hrglass,   0x99,   fx_disthrg2,   fx0, -1},
   {LOOKUP_DIST_CLW,           s_galaxy,    0x66,   fx_dgalw1,     fx0, -1},
   {LOOKUP_DIST_CLW,           s_galaxy,    0xCC,   fx_dgalw2,     fx0, -1},
   {LOOKUP_DIST_CLW,           s_galaxy,    0x33,   fx_dgalw3,     fx0, -1},
   {LOOKUP_DIST_CLW,           s_galaxy,    0x99,   fx_dgalw4,     fx0, -1},
   {LOOKUP_DIST_CLW,           s2x4,        0x33,   fx_d2x4q1,     fx0, -1},
   {LOOKUP_DIST_CLW,           s2x4,        0xCC,   fx_d2x4q2,     fx0, -1},
   {LOOKUP_OFFS_CLW,           s2x4,        0x33,   fx_d2x4w1,     fx0, -1},
   {LOOKUP_OFFS_CLW,           s2x4,        0xCC,   fx_d2x4w2,     fx0, -1},
   {LOOKUP_DIST_CLW|LOOKUP_STAG_CLW, s2x4,  0xAA,   fx_d2x4x1,     fx0, -1},
   {LOOKUP_DIST_CLW|LOOKUP_STAG_CLW, s2x4,  0x55,   fx_d2x4x2,     fx0, -1},
   {LOOKUP_DIST_CLW,           s3x4,       0x560,   fx_d3x4ul1,    fx0, -1},
   {LOOKUP_DIST_CLW,           s3x4,       0xA90,   fx_d3x4ul2,    fx0, -1},
   {LOOKUP_DIST_CLW,           s3x4,       0x42A,   fx_d3x4ul3,    fx0, -1},
   {LOOKUP_DIST_CLW,           s3x4,       0x815,   fx_d3x4ul4,    fx0, -1},
   {LOOKUP_DIST_CLW,           s3x4,       0xA28,   fx_d3x4ul5,    fx0, -1},
   {LOOKUP_DIST_CLW,           s3x4,       0x861,   fx_d3x4ul6,    fx0, -1},
   {LOOKUP_OFFS_CLW|LOOKUP_DIST_CLW, s3x4, 0xCC0,   fx_d3x4vl1,    fx0, -1},
   {LOOKUP_OFFS_CLW|LOOKUP_DIST_CLW, s3x4, 0x330,   fx_d3x4vl2,    fx0, -1},
   {LOOKUP_OFFS_CLW|LOOKUP_DIST_CLW, s3x4, 0xC0C,   fx_d3x4vl3,    fx0, -1},
   {LOOKUP_OFFS_CLW|LOOKUP_DIST_CLW, s3x4, 0x033,   fx_d3x4vl4,    fx0, -1},
   {LOOKUP_DBL_BENT,           s2x3,         033,   fx_dqtgdbb1,   fx0, -1},
   {LOOKUP_DBL_BENT,           s2x3,         066,   fx_dqtgdbb2,   fx0, -1},
   {LOOKUP_MINI_B,             s_hrglass,  0x0BB,   fx_hrgminb,    fx0, -1},
   {LOOKUP_MINI_B,             s_ptpd,     0x0EE,   fx_ptpminb,    fx0, -1},
   {LOOKUP_MINI_O,             s_rigger,   0x0BB,   fx_rigmino,    fx0, -1},
   {LOOKUP_MINI_O,             s_galaxy,   0x0BB,   fx_galmino,    fx0, -1},
   {LOOKUP_MINI_O,             s_qtag,     0x077,   fx_qtgminb,    fx0, -1},

   {LOOKUP_DISC|LOOKUP_IGNORE, s1x6,         033,   fx_f1x6aad,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x6,         055,   fx_f1x6abd,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x6,         011,   fx_f1x6ed,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s2x2,         0xF,   fx_f2x2pl,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x4,         0xF,   fx_f1x4pl,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, sdmd,         0xF,   fx_fdmdpl,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x2,         0x3,   fx_f1x2pl,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x4,         0x5,   fx_f1x4ed,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s2x3,         055,   fx_f2x3od,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_1x2dmd,     055,   fx_f12dabd,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_1x2dmd,     033,   fx_f1x2dd,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s3ptpd,     00303,   fx_f3ptp,      fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s3ptpd,     00606,   fx_f3ptp,      fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x8,        0xAA,   fx_f1x8aad,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x8,        0x55,   fx_foo55d,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x8,        0x99,   fx_foo99d,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x8,        0x66,   fx_foo66d,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x8,        0x33,   fx_f1x8endd,   fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x8,        0x88,   fx_f1x8_88,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x8,        0x22,   fx_f1x8_22,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x8,        0x11,   fx_f1x8_11,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x8,        0x77,   fx_f1x8_77,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x8,        0xBB,   fx_f1x8_bb,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x8,        0xDD,   fx_f1x8_dd,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x10,      0x129,   fx_f1x10abd,   fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s3x4,       03131,   fx_f3x4outrd,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s3dmd,      00707,   fx_f3dmoutrd,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_dhrglass,  0x77,   fx_fdhrgld,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x12,      00707,   fx_f1x12outrd, fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x12,      06363,   fx_f1x12outre, fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_323,       0x77,   fx_f323d,      fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_bone,      0x33,   fx_fboneendd,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_bone,      0xBB,   fx_fbonetgl,   fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_rigger,    0x77,   fx_frigtgl,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_qtag,      0x33,   fx_fqtgend,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_bone6,      033,   fx_fo6zzd,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_bone6,      044,   fx_fo6zze,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_ptpd,      0xAA,   fx_foozzd,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_ptpd,      0x55,   fx_fptpzzd,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_dhrglass,  0x33,   fx_fdrhgl1,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s3x1dmd,     0x99,   fx_f3x1zzd,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s3x1dmd,     0xAA,   fx_f3x1yyd,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_2x1dmd,     055,   fx_f2x1yyd,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_wingedstar6,055,   fx_fwstyyd,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x3dmd,     0x99,   fx_f1x3zzd,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x3dmd,     0xAA,   fx_f1x3yyd,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x3dmd,     0x66,   fx_f1x3bbd,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s1x3dmd,     0x55,   fx_f1x3wwd,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_crosswave, 0x55,   fx_fxwv1d,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_crosswave, 0x99,   fx_fxwv2d,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_crosswave, 0x66,   fx_fxwv3d,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_crosswave, 0x33,   fx_fxwve,      fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_spindle,   0x55,   fx_fspindld,   fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_spindle,   0xAA,   fx_fspindlbd,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_spindle,   0xDD,   fx_fspindlod,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_dhrglass,  0xBB,   fx_fdrhgl_bb,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s2x4,        0xAA,   fx_fppaad,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s2x4,        0x55,   fx_fpp55d,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s2x4,        0xA5,   fx_f2x4dleft,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s2x4,        0x5A,   fx_f2x4dright, fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s2x4,        0x99,   fx_f2x4endd,   fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_hrglass,   0x33,   fx_fhrgle,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_dhrglass,  0x99,   fx_fdhrgld1,   fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s_dhrglass,  0xAA,   fx_fdhrgld2,   fx0, -1},
   {LOOKUP_IGNORE,             s2x4,        0x77,   fx_f2x477,     fx0, -1},  /* 2x4 with 2 corners ignored.  These 2 are *NOT* disconnected. */
   {LOOKUP_IGNORE,             s2x4,        0xEE,   fx_f2x4ee,     fx0, -1},  /* 2x4 with 2 centers ignored.  These 2 *ARE* disconnected (or ignored). */
   {LOOKUP_IGNORE,             s_ptpd,      0x77,   fx_fptpdid,    fx0, -1},
   {LOOKUP_IGNORE,             s_ptpd,      0xDD,   fx_fptpdid,    fx0, -1},
   {LOOKUP_IGNORE,             s_c1phan,  0x1515,   fx_phanigna,    fx0, -1},
   {LOOKUP_IGNORE,             s_c1phan,  0x4545,   fx_phanignb,    fx0, -1},
   {LOOKUP_IGNORE,             s_c1phan,  0x5151,   fx_phanignc,    fx0, -1},
   {LOOKUP_IGNORE,             s_c1phan,  0x5454,   fx_phanignd,    fx0, -1},
   {LOOKUP_IGNORE,             s_c1phan,  0xA2A2,   fx_phanigne,    fx0, -1},
   {LOOKUP_IGNORE,             s_c1phan,  0xA8A8,   fx_phanignf,    fx0, -1},
   {LOOKUP_IGNORE,             s_c1phan,  0x2A2A,   fx_phanigng,    fx0, -1},
   {LOOKUP_IGNORE,             s_c1phan,  0x8A8A,   fx_phanignh,    fx0, -1},

   {LOOKUP_DISC|LOOKUP_IGNORE, s2x4,        0xBB,   fx_f2x4bb,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE, s2x4,        0xDD,   fx_f2x4dd,     fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s1x8,     0xCC, fx_f1x8ctr,   fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s_qtag,   0xCC, fx_fqtgctr,   fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s_bone,   0xCC, fx_fbonectr,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s_rigger, 0x33, fx_frigctr,   fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s3x1dmd,  0x66, fx_f3x1ctl,   fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s1x4,     0x0F, fx_f1x4pl,    fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x66, fx_f2x4ctr,   fx0, -1},

   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x0F, fx_f2x4far,   fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0xF0, fx_f2x4near,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x81, fx_f2x4pos1,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x42, fx_f2x4pos2,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x24, fx_f2x4pos3,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x18, fx_f2x4pos4,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x7E, fx_f2x4pos5,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0xBD, fx_f2x4pos6,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0xDB, fx_f2x4pos7,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0xE7, fx_f2x4pos8,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x03, fx_f2x4posa,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x06, fx_f2x4posb,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x0C, fx_f2x4posc,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x30, fx_f2x4posd,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x60, fx_f2x4pose,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0xC0, fx_f2x4posf,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x63, fx_f2x4posp,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x6C, fx_f2x4posq,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0xC6, fx_f2x4posr,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x36, fx_f2x4poss,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x3F, fx_f2x4posu,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0xCF, fx_f2x4posv,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0xF3, fx_f2x4posw,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0xFC, fx_f2x4posx,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x6F, fx_f2x4posy,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0xF6, fx_f2x4posz,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0xC3, fx_f2x4left,  fx0, -1},
   {LOOKUP_DISC|LOOKUP_IGNORE|LOOKUP_NONE, s2x4,     0x3C, fx_f2x4right, fx0, -1},
   {LOOKUP_DISC,                           s2x6,    05555, fx_f2x6ndsc,  fx0, -1},
   {LOOKUP_DISC,                           s2x6,    06363, fx_f2x6ndsd,  fx0, -1},
   {LOOKUP_DISC,                           s3x4,    01717, fx_f3x4ndsd,  fx0, -1},
   {LOOKUP_DISC,                           s2x8,   0x9999, fx_f2x8nd99,  fx0, -1},
   {LOOKUP_DISC,                           s2x8,   0xA5A5, fx_f2x8nda5,  fx0, -1},
   {LOOKUP_DISC,                           s4x6, 0x84C84C, fx_f4x6ndoo,  fx0, -1},
   {LOOKUP_DISC,                           s4x6, 0x321321, fx_f4x6ndxx,  fx0, -1},
   {LOOKUP_DISC,                           s1x8,     0x96, fx_f1x8nd96,  fx0, -1},
   {LOOKUP_DISC,                           s1x8,     0x69, fx_f1x8nd69,  fx0, -1},
   {LOOKUP_DISC,                           s1x8,     0x41, fx_f1x8nd41,  fx0, -1},
   {LOOKUP_DISC,                           s1x8,     0x82, fx_f1x8nd82,  fx0, -1},
   {LOOKUP_DISC,                           s1x8,     0x28, fx_f1x8nd28,  fx0, -1},
   {LOOKUP_DISC,                           s1x8,     0x14, fx_f1x8nd14,  fx0, -1},
   {LOOKUP_DISC,                          s1x10,    0x37B, fx_f1x10ndsc, fx0, -1},
   {LOOKUP_DISC,                          s1x10,    0x3BD, fx_f1x10ndsd, fx0, -1},
   {LOOKUP_DISC,                          s1x10,    0x2F7, fx_f1x10ndse, fx0, -1},
   {LOOKUP_DISC,                          s4dmd,   0xA9A9, fx_fdmdnd,    fx0, -1},
   {LOOKUP_DISC,                           s3x6,  0x2C361, fx_fdmdndx,   fx0, -1},
   {LOOKUP_NONE,                           s4x4,   0x0066, fx_fcpl12,     fx0, -1},
   {LOOKUP_NONE,                           s4x4,   0x6006, fx_fcpl23,     fx0, -1},
   {LOOKUP_NONE,                           s4x4,   0x6600, fx_fcpl34,     fx0, -1},
   {LOOKUP_NONE,                           s4x4,   0x0660, fx_fcpl41,     fx0, -1},
   {LOOKUP_NONE,                           s_thar, 0x00CC, fx_ftharns,    fx0, -1},
   {LOOKUP_NONE,                           s_thar, 0x0033, fx_ftharew,    fx0, -1},
   {LOOKUP_NONE,                          s_alamo, 0x0033, fx_falamons,   fx0, -1},
   {LOOKUP_NONE,                          s_alamo, 0x00CC, fx_falamoew,   fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0xCC,   fx_foocc,      fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0x33,   fx_foo33,      fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0x99,   fx_f2x4endo,   fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0x5A,   fx_f2x4pos13,  fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0xA5,   fx_f2x4pos02,  fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0x55,   fx_foo55,      fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0xAA,   fx_fooaa,      fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0x88,   fx_foo88,      fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0x44,   fx_foo44,      fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0x22,   fx_foo22,      fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0x11,   fx_foo11,      fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0x77,   fx_foo77,      fx0, -1},
   {LOOKUP_NONE,                             s2x4, 0xEE,   fx_fooEE,      fx0, -1},
   {LOOKUP_NONE,                      s1x4p2dmd,   0xD8,   fx_1x4p2d,     fx0, -1},
   {LOOKUP_NONE,                      s4p2x1dmd,   0xD8,   fx_4p2x1d,     fx0, -1},
   {LOOKUP_NONE,                      splinepdmd,  0xF0,   fx_1x5p1d,     fx0, -1},
   {LOOKUP_NONE,                      splinepdmd,  0x0F,   fx_1x5p1e,     fx0, -1},
   {LOOKUP_NONE,                      slinebox,    0xF0,   fx_1x5p1f,     fx0, -1},
   {LOOKUP_NONE,                      slinebox,    0x0F,   fx_1x5p1g,     fx0, -1},
   {LOOKUP_NONE,                      sline2box,   0xFC,   fx_l2b1,       fx0, -1},
   {LOOKUP_NONE,                      sline2box,   0x03,   fx_l2b2,       fx0, -1},
   {LOOKUP_NONE,                      sline2box,   0x30,   fx_l2b3,       fx0, -1},
   {LOOKUP_NONE,                      sline6box,   0x3F,   fx_l6b1,       fx0, -1},
   {LOOKUP_NONE,                      sline6box,   0xC0,   fx_l6b2,       fx0, -1},
   {LOOKUP_NONE,                      sline6box,   0x03,   fx_l6b3,       fx0, -1},

   {LOOKUP_NONE,                     sdbltrngl4,   0x30,   fx_dbt1,       fx0, -1},
   {LOOKUP_NONE,                     sdbltrngl4,   0x0C,   fx_dbt2,       fx0, -1},
   {LOOKUP_NONE,                     sdbltrngl4,   0x03,   fx_dbt3,       fx0, -1},
   {LOOKUP_NONE,                     sdbltrngl4,   0xC0,   fx_dbt4,       fx0, -1},
   {LOOKUP_NONE,                     sdbltrngl4,   0xCC,   fx_dbt1b,      fx0, -1},
   {LOOKUP_NONE,                     sdbltrngl4,   0x33,   fx_dbt2b,      fx0, -1},
   {LOOKUP_NONE,                     sdbltrngl4,   0x1E,   fx_dbt1d,      fx0, -1},
   {LOOKUP_NONE,                     sdbltrngl4,   0xE0,   fx_dbt3t,      fx0, -1},
   {LOOKUP_NONE,                     sdbltrngl4,   0xFC,   fx_dbt6t,      fx0, -1},
   {LOOKUP_NONE,                           s1x6,   0x18,   fx_l2b4,       fx0, -1},
   {LOOKUP_NONE,                           s1x6,   0x03,   fx_l2b5,       fx0, -1},
   {LOOKUP_NONE,                      splinedmd,   0xF0,   fx_5p1x1d,     fx0, -1},
   {LOOKUP_NONE,                      splinedmd,   0x0F,   fx_plndmda,    fx0, -1},
   {LOOKUP_NONE,                       slinedmd,   0xF0,   fx_lndmda,     fx0, -1},
   {LOOKUP_NONE,                       slinedmd,   0x0F,   fx_lndmd9,     fx0, -1},
   {LOOKUP_NONE,                      slinepdmd,   0xF0,   fx_linpdma,    fx0, -1},
   {LOOKUP_NONE,                      slinepdmd,   0x0F,   fx_linpdmd,    fx0, -1},
   {LOOKUP_NONE,                        sboxdmd,   0xF0,   fx_boxdma,     fx0, -1},
   {LOOKUP_NONE,                        sboxdmd,   0x0F,   fx_boxdmb,     fx0, -1},
   {LOOKUP_NONE,                       sboxpdmd,   0xF0,   fx_boxpdma,    fx0, -1},
   {LOOKUP_NONE,                       sboxpdmd,   0x0F,   fx_boxpdmb,    fx0, -1},
   {LOOKUP_NONE,                       sboxpdmd,   0xF4,   fx_beehive1,   fx0, -1},

   {LOOKUP_NONE,                      s_spindle,   0xE3,   fx_beehive2,   fx0, -1},
   {LOOKUP_NONE,                      s_spindle,   0x3E,   fx_beehive3,   fx0, -1},
   {LOOKUP_NONE,                      s_spindle,   0x1C,   fx_spnnrtgl,   fx0, -1},
   {LOOKUP_NONE,                      s_spindle,   0xC1,   fx_spnfrtgl,   fx0, -1},
   {LOOKUP_NONE,                      s_galaxy,    0xE0,   fx_galnrtgl,   fx0, -1},
   {LOOKUP_NONE,                      s_galaxy,    0x0E,   fx_galfrtgl,   fx0, -1},
   {LOOKUP_NONE,                      s_galaxy,    0xF1,   fx_galnrvee,   fx0, -1},
   {LOOKUP_NONE,                      s_galaxy,    0x1F,   fx_galfrvee,   fx0, -1},

   {LOOKUP_NONE,                      s_323,       0x33,   fx323a,        fx0, -1},
   {LOOKUP_NONE,                      s_323,       0x66,   fx323b,        fx0, -1},
   {LOOKUP_NONE,                      s_ntrglcw,   0xCC,   fxntrgla,      fx0, -1},
   {LOOKUP_NONE,                      s_ntrglccw,  0x33,   fxntrglb,      fx0, -1},

   {LOOKUP_NONE,                      s_trngl8,    0xF0,   fx_trngl8a,    fx0, -1},
   {LOOKUP_NONE,                      s_trngl8,    0x0F,   fx_trngl8b,    fx0, -1},
   {LOOKUP_NONE,                      sdmdpdmd,    0xF0,   fxdmdpdmda,    fx0, -1},
   {LOOKUP_NONE,                      sdmdpdmd,    0x0F,   fxdmdpdmdb,    fx0, -1},

   {LOOKUP_NONE,                      s_23232,    04242,   fx23232a,      fx0, -1},
   {LOOKUP_NONE,                      s_23232,    04141,   fx23232b,      fx0, -1},
   {LOOKUP_NONE,                      s_23232,    01010,   fx23232c,      fx0, -1},

   {LOOKUP_NONE,                      s_23232,    01414,   fx23232d,      fx0, -1},
   {LOOKUP_NONE,                      s_23232,    03030,   fx23232e,      fx0, -1},

   {LOOKUP_NONE,                      s1x6,         074,   fx_1x6hif,     fx0, -1},
   {LOOKUP_NONE,                      s1x6,         047,   fx_1x6lowf,    fx0, -1},
   {LOOKUP_NONE,                      s1x4,        0x3,    fx_n1x43,      fx0, -1},
   {LOOKUP_NONE,                      s1x4,        0xC,    fx_n1x4c,      fx0, -1},
   {LOOKUP_NONE,                      s1x4,        0x5,    fx_n1x45,      fx0, -1},
   {LOOKUP_NONE,                      s1x4,        0xA,    fx_n1x4a,      fx0, -1},
   {LOOKUP_NONE,                      s1x3,        0x3,    fx_n1x3a,      fx0, -1},
   {LOOKUP_NONE,                      s1x3,        0x6,    fx_n1x3b,      fx0, -1},
   {LOOKUP_NONE,                      s1x6,         033,   fx_f1x6aa,     fx0, -1},
   {LOOKUP_NONE,                      s1x8,        0xAA,   fx_f1x8aa,     fx0, -1},
   {LOOKUP_NONE,                      s1x8,        0x55,   fx_f1x855,     fx0, -1},
   {LOOKUP_NONE,                      s3x4,        03131,  fx_f3x4outer,  fx0, -1},
   {LOOKUP_NONE,                      s3dmd,       00707,  fx_f3dmouter,  fx0, -1},
   {LOOKUP_NONE,                      s3ptpd,      06060,  fx_f3ptpdin,   fx0, -1},
   {LOOKUP_NONE,                      s_spindle12, 0x318,  fx_sp12wing,   fx0, -1},
   {LOOKUP_NONE,                      spgdmdcw,    0xCC,   fx_fpgdmdcw,   fx0, -1},
   {LOOKUP_NONE,                      spgdmdccw,   0xCC,   fx_fpgdmdccw,  fx0, -1},
   {LOOKUP_NONE,                      s_dhrglass,   0x77,  fx_fdhrgl,     fx0, -1},
   {LOOKUP_NONE,                      s_ptpd,       0xBB,  fx_specfix3x40,fx0, -1},
   {LOOKUP_NONE,                      s_bone,       0x77,  fx_specfix3x41,fx0, -1},
   {LOOKUP_NONE,                      s_spindle,    0xDD,  fx_specspindle,fx0, -1},
   {LOOKUP_NONE,                      s_525,       00707,  fx_f525nw,     fx0, -1},
   {LOOKUP_NONE,                      s_525,       03434,  fx_f525ne,     fx0, -1},
   {LOOKUP_NONE,                      s_323,        0x77,  fx_f323,       fx0, -1},
   {LOOKUP_NONE,                      sdeepxwv,    00303,  fx_fdpxwve,    fx0, -1},
   {LOOKUP_NONE,                      s_ntrgl6cw,    066,  fx_ftgl6ccd,   fx0, -1},
   {LOOKUP_NONE,                      s_ntrgl6ccw,   033,  fx_ftgl6cccd,  fx0, -1},
   {LOOKUP_NONE,                      s1x3dmd,      0x77,  fx_f1x3d6,     fx0, -1},
   {LOOKUP_NONE,                      s1x12,       00707,  fx_f1x12outer, fx0, -1},
   {LOOKUP_NONE,                      s3x4,        00303,  fx_f3x4left,   fx0, -1},
   {LOOKUP_NONE,                      s3x4,        01414,  fx_f3x4right,  fx0, -1},
   {LOOKUP_NONE,                      s3x4,        03030,  fx_f3x4east,   fx0, -1},
   {LOOKUP_NONE,                      s3x4,        02121,  fx_f3x4west,   fx0, -1},
   {LOOKUP_NONE,                      s3x4,        04343,  fx_f3x4down,   fx0, -1},
   {LOOKUP_NONE,                      s3x4,        05454,  fx_f3x4up,     fx0, -1},
   {LOOKUP_NONE,                      s4x5,      0x60D83,  fx_f4x5down,   fx0, -1},
   {LOOKUP_NONE,                      s4x5,      0x360D8,  fx_f4x5up,     fx0, -1},
   {LOOKUP_DIST_BOX|LOOKUP_DIAG_BOX,  s3x4,        03030,  fx_d3x4ub6,    fx0, -1},
   {LOOKUP_DIST_BOX|LOOKUP_DIAG_BOX,  s3x4,        02121,  fx_d3x4ub5,    fx0, -1},
   {LOOKUP_NONE,                      s4x4,       0x3030,  fx_f4x4nw,     fx0, -1},
   {LOOKUP_NONE,                      s4x4,       0x4141,  fx_f4x4ne,     fx0, -1},
   {LOOKUP_NONE,                      s4x4,       0x0303,  fx_f4x4en,     fx0, -1},
   {LOOKUP_NONE,                      s4x4,       0x1414,  fx_f4x4wn,     fx0, -1},
   {LOOKUP_NONE,                      s2x6,        00303,  fx_f3x4lzz,    fx0, -1},
   {LOOKUP_NONE,                      s2x6,        06060,  fx_f3x4rzz,    fx0, -1},
   {LOOKUP_NONE,                      s2x8,       0x0303,  fx_f4x4lzz,    fx0, -1},
   {LOOKUP_NONE,                      s2x8,       0xC0C0,  fx_f4x4rzz,    fx0, -1},

   {LOOKUP_TRAPEZOID,                 s2x4,         0x96,  fx_f2x4tt0,    fx0, -1},
   {LOOKUP_TRAPEZOID,                 s2x4,         0x69,  fx_f2x4tt1,    fx0, -1},

   // Next 6 are unsymmetrical in a 2x8 - boxes and trapezoids.
   {LOOKUP_NONE,                      s2x8,       0xC003,  fx_f2x8qq0,    fx0, -1},
   {LOOKUP_NONE,                      s2x8,       0x03C0,  fx_f2x8qq1,    fx0, -1},
   {LOOKUP_TRAPEZOID,                 s2x8,       0x9006,  fx_f2x8tt0,    fx0, -1},
   {LOOKUP_TRAPEZOID,                 s2x8,       0x6009,  fx_f2x8tt1,    fx0, -1},
   {LOOKUP_TRAPEZOID,                 s2x8,       0x0960,  fx_f2x8tt2,    fx0, -1},
   {LOOKUP_TRAPEZOID,                 s2x8,       0x0690,  fx_f2x8tt3,    fx0, -1},

   // Unsymmetrical in a 4x4 - near/far box.
   {LOOKUP_NONE,                      s4x4,       0x400B,  fx_f4x4neq,    fx0, -1},
   {LOOKUP_NONE,                      s4x4,       0x00B4,  fx_f4x4seq,    fx0, -1},
   {LOOKUP_NONE,                      s4x4,       0x0B40,  fx_f4x4swq,    fx0, -1},
   {LOOKUP_NONE,                      s4x4,       0xB400,  fx_f4x4nwq,    fx0, -1},

   // Next 6 are unsymmetrical in a 2x6 - boxes and trapezoids.
   {LOOKUP_NONE,                      s2x6,        0xC03,  fx_f2x6qq0,    fx0, -1},
   {LOOKUP_NONE,                      s2x6,        0x0F0,  fx_f2x6qq1,    fx0, -1},
   {LOOKUP_TRAPEZOID,                 s2x6,        0x906,  fx_f2x6tt0,    fx0, -1},
   {LOOKUP_TRAPEZOID,                 s2x6,        0x609,  fx_f2x6tt1,    fx0, -1},
   {LOOKUP_TRAPEZOID,                 s2x6,        0x1A4,  fx_f2x6tt2,    fx0, -1},
   {LOOKUP_TRAPEZOID,                 s2x6,        0x258,  fx_f2x6tt3,    fx0, -1},

   {LOOKUP_NONE,                      s1x8,        0x33,   fx_f1x8endo,   fx0, -1},
   {LOOKUP_NONE,                      s1x8,        0x77,   fx_f1x8_77_3,  fx0, -1},
   {LOOKUP_NONE,                      s1x8,        0x0F,   fx_f1x8lowf,   fx0, -1},
   {LOOKUP_NONE,                      s1x8,        0xF0,   fx_f1x8hif,    fx0, -1},
   {LOOKUP_NONE,                      s1x8,        0x03,   fx_f1x8low2,   fx0, -1},
   {LOOKUP_NONE,                      s1x8,        0x30,   fx_f1x8hi2,    fx0, -1},
   {LOOKUP_NONE,                      s1x8,        0xCF,   fx_f1x8low6,   fx0, -1},
   {LOOKUP_NONE,                      s1x8,        0xFC,   fx_f1x8hi6,    fx0, -1},
   {LOOKUP_NONE,                      s1x8,        0x3C,   fx_f1x8lodbt4, fx0, -1},
   {LOOKUP_NONE,                      s1x8,        0xC3,   fx_f1x8hidbt4, fx0, -1},
   {LOOKUP_NONE,                      s_qtag,      0xE1,   fx_fqtglowf,   fx0, -1},
   {LOOKUP_NONE,                      s_qtag,      0x1E,   fx_fqtghif,    fx0, -1},
   {LOOKUP_NONE,                      s_ptpd,      0x0F,   fx_fdmdlowf,   fx0, -1},
   {LOOKUP_NONE,                      s_ptpd,      0xF0,   fx_fdmdhif,    fx0, -1},
   {LOOKUP_NONE,                      s_ptpd,      0x0B,   fx_fdmdlow3,   fx0, -1},
   {LOOKUP_NONE,                      s_ptpd,      0xB0,   fx_fdmdhi3,    fx0, -1},
   {LOOKUP_NONE,                      s_bone,      0x33,   fx_fboneendo,  fx0, -1},
   {LOOKUP_NONE,                      s_ptpd,      0xAA,   fx_foozz,      fx0, -1},
   {LOOKUP_NONE,                      sbigh,       00303,  fx_fdblbentcw, fx0, -1},
   {LOOKUP_NONE,                      sbigh,       01414,  fx_fdblbentccw,fx0, -1},
   {LOOKUP_NONE,                      sbigbone,    00303,  fx_fcrookedcw, fx0, -1},
   {LOOKUP_NONE,                      sbigbone,    06060,  fx_fcrookedccw,fx0, -1},
   {LOOKUP_NONE,                      s_spindle,   0x55,   fx_fspindlc,   fx0, -1},
   {LOOKUP_NONE,                      s_spindle,   0x66,   fx_fspindlf,   fx0, -1},
   {LOOKUP_NONE,                      s_spindle,   0x33,   fx_fspindlg,   fx0, -1},
   {LOOKUP_NONE,                      s_galaxy,    0x44,   fx_fgalcv,     fx0, -1},
   {LOOKUP_NONE,                      s_galaxy,    0x11,   fx_fgalch,     fx0, -1},
   {LOOKUP_NONE,                      s1x3dmd,     0x66,   fx_f1x3aad,    fx0, -1},
   {LOOKUP_NONE,                      s_2x1dmd,    066,    fx_f2x166d,    fx0, -1},
   {LOOKUP_NONE,                      s_1x2dmd,    033,    fx_f1x2aad,    fx0, -1},
   {LOOKUP_NONE,                      s3ptpd,      06262,  fx_f3ptpo6,    fx0, -1},
   {LOOKUP_NONE,                      s2x3,        055,    fx_f2x3c,      fx0, -1},
   {LOOKUP_NONE,                      s2x3,        041,    fx_f2x3a41,    fx0, -1},
   {LOOKUP_NONE,                      s2x3,        014,    fx_f2x3a14,    fx0, -1},
   {LOOKUP_NONE,                      s2x5,        0x318,  fx_f2x5c,      fx0, -1},
   {LOOKUP_NONE,                      s2x5,        0x063,  fx_f2x5d,      fx0, -1},
   {LOOKUP_NONE,                      s2x5,        0x2B5,  fx_f2x5e,      fx0, -1},
   {LOOKUP_NONE,                      sd2x5,       0x18C,  fx_fd2x5d,     fx0, -1},
   {LOOKUP_NONE,                      sd2x5,       0x318,  fx_fd2x5d,     fx0, -1},
   {LOOKUP_NONE,                      sd2x5,       0x198,  fx_fd2x5d,     fx0, -1},
   {LOOKUP_NONE,                      sd2x5,       0x30C,  fx_fd2x5d,     fx0, -1},
   {LOOKUP_NONE,                      sd2x5,       0x294,  fx_fd2x5d,     fx0, -1},
   {LOOKUP_NONE,                      sd2x7,      0x3060,  fx_fd2x7d1,    fx0, -1},
   {LOOKUP_NONE,                      sd2x7,      0x0183,  fx_fd2x7d2,    fx0, -1},
   {LOOKUP_DIST_CLW|LOOKUP_OFFS_CLW,  sd2x7,      0x3060,  fx_fd2x7d3,    fx0, -1},
   {LOOKUP_DIST_CLW|LOOKUP_OFFS_CLW,  sd2x7,      0x0183,  fx_fd2x7d4,    fx0, -1},
   {LOOKUP_NONE,                      sd2x7,      0x0C18,  fx_fd2x7q1,    fx0, -1},
   {LOOKUP_NONE,                      sd2x7,      0x060C,  fx_fd2x7q2,    fx0, -1},
   {LOOKUP_DIST_CLW|LOOKUP_OFFS_CLW,  s2x6,        06060,  fx_fd2x7q3,    fx0, -1},
   {LOOKUP_DIST_CLW|LOOKUP_OFFS_CLW,  s2x6,        00303,  fx_fd2x7q4,    fx0, -1},
   {LOOKUP_NONE,                      sbighrgl,    0xC30,  fx_bghrgla,    fx0, -1},
   {LOOKUP_NONE,                      sbighrgl,    0x0C3,  fx_bghrglb,    fx0, -1},
   {LOOKUP_NONE,                      s_hsqtag,    0x30C,  fx_shsqtga,    fx0, -1},
   {LOOKUP_NONE,                      s_hsqtag,    0x0C3,  fx_shsqtgb,    fx0, -1},
   {LOOKUP_NONE,                      s4x6,      0x003003, fx_f4x6a,      fx0, -1},
   {LOOKUP_NONE,                      s4x6,      0xC00C00, fx_f4x6b,      fx0, -1},
   {LOOKUP_NONE,                      s4x6,      0x0C00C0, fx_f4x6c,      fx0, -1},
   {LOOKUP_NONE,                      s4x6,      0x030030, fx_f4x6d,      fx0, -1},
   {LOOKUP_NONE,                      s4x6,      0x801801, fx_f4x6e,      fx0, -1},
   {LOOKUP_NONE,                      s4x6,      0x060060, fx_f4x6f,      fx0, -1},

   {LOOKUP_NONE,                      s2x2,        0x3,    fx_box3c,      fx0, -1},
   {LOOKUP_NONE,                      s2x2,        0x6,    fx_box6c,      fx0, -1},
   {LOOKUP_NONE,                      s2x2,        0x9,    fx_box9c,      fx0, -1},
   {LOOKUP_NONE,                      s2x2,        0xC,    fx_boxcc,      fx0, -1},
   {LOOKUP_NONE,                      s2x2,        0x5,    fx_box55,      fx0, -1},
   {LOOKUP_NONE,                      s2x2,        0xA,    fx_boxaa,      fx0, -1},
   {LOOKUP_NONE,                      s_qtag,      0xAA,   fx_fqtgj1,     fx0, -1},
   {LOOKUP_NONE,                      s_qtag,      0x99,   fx_fqtgj2,     fx0, -1},
   {LOOKUP_NONE,                      s2x3,         033,   fx_f2x3j1,     fx0, -1},
   {LOOKUP_NONE,                      s2x3,         066,   fx_f2x3j2,     fx0, -1},
   {LOOKUP_NONE, nothing}};

// This has the same issues as the table above.  These items are supposed
// to reflect the enumeration type "tglmapkey".  It is not essential
// that this table be in the same order as the enumeration type.
// But it is essential that it correspond exactly to the enumeration
// in some order.  That will be checked during initialization.

const tglmap::map tglmap::init_table[] = {

   // mykey     kind   kind1x3 otherkey
   //                                   nointlkshapechange
   //                                      switchtgls

   // In C1 phantom: first triangle (inverted),
   // then second triangle (upright), then idle.
   {tglmap1b, s_c1phan, s2x4, tglmap2b, 0, 0,
    {4, 3, 2,   0, 7, 6,   1, 5},     // mapqt1
    {6, 8, 10,  14,0, 2,   4, 12},    // mapcp1
    {10, 9, 8,  4, 3, 2,   5, 11},    // mapbd1
    {6, 5, 4, 2, 1, 0, 3, 7},         // map241
    {10, 9, 8, 4, 3, 2, 5, 11}},      // map261

   {tglmap2b, s_qtag, s2x4, tglmap1b, 0, 1,
    {5, 6, 7,   1, 2, 3,   0, 4},
    {3, 15, 13, 11,7, 5,   1,  9},
    {1, 2, 3,   7, 8, 9,   0, 6},
    {7, 6, 5, 3, 2, 1, 0, 4},
    {1, 2, 3, 7, 8, 9, 0, 6}},

   {tglmap1i, nothing, nothing, tglmap2i, 0, 0,   // Interlocked
    {0, 0, 0,   0, 0, 0,   0, 0},
    {4, 8, 10,  12,0, 2,   6, 14},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}},

   {tglmap2i, nothing, nothing,  tglmap1i, 0, 0,   // Interlocked
    {0, 0, 0,   0, 0, 0,   0, 0},
    {1, 15, 13, 9, 7, 5,   3, 11},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}},

   // Maps for deepbigqtg setups.
   {tglmap1d, s2x8, s2x8, tglmap2d, 0, 0,
    {4, 3, 2,   0, 7, 6,     1, 5},
    {2, 6, 7,   10,14, 15,   0, 8},
    {10, 9, 8,  4, 3, 2,     5, 11},
    {1, 2, 3,   9, 10, 11,   8, 0},
    {10, 9, 8, 4, 3, 2, 5, 11}},

   {tglmap2d, s2x8, s2x8, tglmap1d, 0, 0,
    {4, 3, 2,   0, 7, 6,   1, 5},
    {3, 4, 5,   11,12,13,  1, 9},
    {10, 9, 8,  4, 3, 2,   5, 11},
    {14,13,12,  6, 5, 4,   7, 15},
    {10, 9, 8, 4, 3, 2, 5, 11}},

   {tglmap1m, nothing, s2x8, tglmap2m, 1, 0,   // Interlocked
    {5, 6, 7,   1, 2, 3,   0, 4},
    {0, 6, 7,   8,14,15,  2, 10},
    {1, 2, 3,   7, 8, 9,   0, 6},
    {7, 6, 5, 3, 2, 1, 0, 4},
    {1, 2, 3, 7, 8, 9, 0, 6}},

   {tglmap2m, nothing, s2x8, tglmap1m, 1, 0,   // Interlocked
    {5, 6, 7,   1, 2, 3,   0, 4},
    {1, 4, 5,   9,12,13,  3, 11},
    {1, 2, 3,   7, 8, 9,   0, 6},
    {7, 6, 5, 3, 2, 1, 0, 4},
    {1, 2, 3, 7, 8, 9, 0, 6}},

   // Interlocked triangles in quarter-tag:
   {tglmap1j, s_qtag, nothing, tglmap2j, 1, 1,   // In/out point.
    {4, 7, 2,   0, 3, 6,   1, 5},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}},

   {tglmap2j, s_qtag, nothing, tglmap1j, 1, 1,   // In/out point.
    {5, 6, 3,   1, 2, 7,   0, 4},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}},

   {tglmap3j, s_qtag, nothing, tglmap3j, 1, 17,   // Inside, in qtag.
    {7, 1, 4,   3, 5, 0,   2, 6},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}},

   // Magic triangles in quarter-tag:
   {tglmap1x, s_qtag, nothing, tglmap2x, 1, 1,
    {4, 6, 7,   0, 2, 3,   1, 5},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}},

   {tglmap2x, s_qtag, nothing, tglmap1x, 1, 1,
    {5, 3, 2,   1, 7, 6,   0, 4},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}},

   // Magic interlocked triangles in quarter-tag:
   {tglmap1y, s_qtag, nothing, tglmap2y, 1, 1,
    {4, 6, 3,   0, 2, 7,   1, 5},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}},

   {tglmap2y, s_qtag, nothing, tglmap1y, 1, 1,
    {5, 7, 2,   1, 3, 6,   0, 4},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}},

   // Interlocked triangles in bigdmd:
   {tglmap1k, nothing, nothing, tglmap2k, 1, 0,
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {10, 3, 8,  4, 9, 2,   5, 11},
    {0},
    {0}},

   {tglmap2k, nothing, nothing, tglmap1k, 1, 0,
    {0, 0, 0,   0, 0, 0,   0, 0},
    {0, 0, 0,   0, 0, 0,   0, 0},
    {1, 2, 9,   7, 8, 3,   0, 6},
    {0},
    {0}},

   {tglmap2r, nothing, nothing, tglmap2r, 1, 0,   // Inside interlock, in rigger.
    {0},
    {0},
    {7, 1, 4,   3, 5, 0,   2, 6},
    {0},
    {0}},

   {tglmap2p, nothing, nothing, tglmap2p, 1, 0x10,// Inside interlock, in ptpd.
    {2, 7, 5,   6, 3, 1,   0, 4},
    {0},
    {0},
    {0},
    {0}},

   {tglmap1t, nothing, nothing, tglmap1t, 1, 0x10,// Outside interlock, in rigger.
    {0},
    {0},
    {6, 1, 4,   2, 5, 0,   7, 3},
    {0},
    {0}},

   {tglmap2t, nothing, nothing, tglmap2t, 1, 0x10,// Outside interlock, in ptpd.
    {0, 7, 5,   4, 3, 1,   2, 6},
    {0},
    {0},
    {0},
    {0}},

   {tglmap3t, nothing, nothing, tglmap3t, 1, 0x10,// Outside interlock, in qtag.
    {6, 1, 4,   2, 5, 0,   7, 3},
    {0},
    {0},
    {0},
    {0}},

   {tglmaps6, s_bone6, nothing, tg6mapcw, 0, 1,
    {5, 4, 0,   2, 1, 3,   -1, -1},   // qt1 -- come out if rotate 180.
    {0},
    {1, 2, 0,   4, 5, 3,   -1, -1},   // bd1 -- go in with this, come out if no rotate.
    {0},
    {0}},

   {tglmapb6, s_short6, nothing, tg6mapccw, 0, 1,
    {1, 2, 0,   4, 5, 3,   -1, -1},   // qt1 -- come out if rotate 180.
    {0},
    {5, 4, 0,   2, 1, 3,   -1, -1},   // bd1 -- go in with this, come out if no rotate.
    {0},
    {0}},

   // In d2x7.

   // mykey     kind   kind1x3 otherkey
   //                                    nointlkshapechange
   //                                       randombits
   {tglmapd71, s2x8, nothing, tglmapd71, 0, 1,
    {0},
    {0},
    {12, 3, 4,   5, 10, 11,   13, 6},
    {0},
    {14, 13, 12,   6, 5, 4,   15, 7}},

   {tglmapd72, s2x8, nothing, tglmapd72, 0, 1,
    {0},
    {0},
    {1, 2, 3,   8, 9, 10,   0, 7},
    {0},
    {1, 2, 3,    9, 10, 11,    0, 8}},

   // In s_ntrgl6cw or s_ntrgl6ccw.  The "2" bit in "randombits" tells how to choose short6 vs. bone6, or whatever.
   // The "4" bit says this is one of these not-mirror-symmetric setups.
   // The "8" bit says this is an 8-person version.

   {tg6mapcw, s_ntrgl6ccw, nothing, tg6mapccw, 0, 5,
    {5, 0, 1,   2, 3, 4,     -1, -1},   // qt1 -- come out if rotate 180.
    {0, 4, 5,   3, 1, 2,     -1, -1},   // cp1 -- go in with this, come out if no rotate.
    {0},
    {5, 4, 0,   2, 1, 3,     -1, -1},   // 241 -- come out to bone6
    {1, 2, 0,   4, 5, 3,     -1, -1}},  // 261 -- come out to short6

   {tg6mapccw, s_ntrgl6cw, nothing, tg6mapcw, 0, 7,
    {3, 1, 2,   0, 4, 5,     -1, -1},
    {2, 3, 4,   5, 0, 1,     -1, -1},
    {0},
    {5, 4, 0,   2, 1, 3,     -1, -1},
    {1, 2, 0,   4, 5, 3,     -1, -1}},

   {tgl0}
};


const tglmap::tglmapkey tglmap::t6cwtglmap1[1] = {tg6mapcw};
const tglmap::tglmapkey tglmap::t6ccwtglmap1[1] = {tg6mapccw};

const tglmap::tglmapkey tglmap::s6tglmap1[1] = {tglmaps6};
const tglmap::tglmapkey tglmap::b6tglmap1[1] = {tglmapb6};

const tglmap::tglmapkey tglmap::c1tglmap1[2] = {tglmap1b, tglmap1i};
const tglmap::tglmapkey tglmap::c1tglmap2[2] = {tglmap2b, tglmap2i};

const tglmap::tglmapkey tglmap::dbqtglmap1[2] = {tglmap1d, tglmap1m};
const tglmap::tglmapkey tglmap::dbqtglmap2[2] = {tglmap2d, tglmap2m};

const tglmap::tglmapkey tglmap::qttglmap1[4] = {tglmap1b, tglmap1j, tglmap1x, tglmap1y};
const tglmap::tglmapkey tglmap::qttglmap2[4] = {tglmap2b, tglmap2j, tglmap2x, tglmap2y};
const tglmap::tglmapkey tglmap::bdtglmap1[2] = {tglmap1b, tglmap1k};
const tglmap::tglmapkey tglmap::bdtglmap2[2] = {tglmap2b, tglmap2k};
const tglmap::tglmapkey tglmap::rgtglmap1[2] = {tglmap2r, tglmap2r};
const tglmap::tglmapkey tglmap::rgtglmap2[2] = {tglmap2p, tglmap2p};
const tglmap::tglmapkey tglmap::rgtglmap3[2] = {tglmap3j, tglmap3j};
const tglmap::tglmapkey tglmap::ritglmap1[2] = {tglmap1t, tglmap1t};
const tglmap::tglmapkey tglmap::ritglmap2[2] = {tglmap2t, tglmap2t};
const tglmap::tglmapkey tglmap::ritglmap3[2] = {tglmap3t, tglmap3t};

const tglmap::tglmapkey tglmap::d7tglmap1[4] = {tglmapd71};
const tglmap::tglmapkey tglmap::d7tglmap2[4] = {tglmapd72};
