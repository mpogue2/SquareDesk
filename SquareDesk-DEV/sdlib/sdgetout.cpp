// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

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

/* This defines the following functions:
   configuration::null_resolve_thing
   configuration::null_resolve_ptr
   configuration::calculate_resolve
   write_resolve_text
   full_resolve
   create_resolve_menu_title
   initialize_getout_tables
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "sd.h"


struct resolve_rec {
   configuration stuph[MAX_RESOLVE_SIZE];
   int size;
   int insertion_point;
   int insertion_width;
   personrec permutepersoninfo[8];
   int rotchange;
};



struct reconcile_descriptor {
   int perm[8];
   bool allow_eighth_rotation;
};



static const reconcile_descriptor promperm =  {{1, 0, 6, 7, 5, 4, 2, 3}, false};
static const reconcile_descriptor rpromperm = {{0, 1, 7, 6, 4, 5, 3, 2}, false};
static const reconcile_descriptor rlgperm =   {{1, 0, 6, 7, 5, 4, 2, 3}, false};
static const reconcile_descriptor qtagperm =  {{1, 0, 7, 6, 5, 4, 3, 2}, false};
static const reconcile_descriptor homeperm =  {{6, 5, 4, 3, 2, 1, 0, 7}, true};
static const reconcile_descriptor sglperm =   {{7, 6, 5, 4, 3, 2, 1, 0}, true};
static const reconcile_descriptor crossperm = {{5, 4, 3, 2, 1, 0, 7, 6}, false};
static const reconcile_descriptor crossplus = {{5, 4, 3, 2, 1, 0, 7, 6}, false};
static const reconcile_descriptor laperm =    {{1, 7, 6, 4, 5, 3, 2, 0}, false};


static configuration *huge_history_save = (configuration *) 0;
static int huge_history_allocation = 0;
static int huge_history_ptr;

static resolve_rec *all_resolves = (resolve_rec *) 0;
static int resolve_allocation = 0;

static int *avoid_list = (int *) 0;
static int avoid_list_size;
static int avoid_list_allocation = 0;
static uint32_t perm_array[8];
static setup_kind goal_kind;
static int goal_rotation;
static uint32_t goal_directions[8];
static const reconcile_descriptor *current_reconciler;

// BEWARE!!  This must be keyed to the enumeration "command_kind" in sd.h .
static Cstring title_string[] = {
   "Resolve: ",
   "Normalize: ",
   "Standardize: ",
   "Reconcile: ",
   "Pick Random Call: ",
   "Pick Simple Call: ",
   "Pick Concept Call: ",
   "Pick Level Call: ",
   "Pick 8 Person Level Call: ",
   "Create Setup: ",
};




static const char *resolve_distances[] = {
   "0",
   "1/8",
   "1/4",
   "3/8",
   "1/2",
   "5/8",
   "3/4",
   "7/8",
   "0"};

static const char *circ_to_home_distances[] = {
   "",
   "couples 1/2 circulate, ",
   "couples circulate, ",
   "couples circulate 1-1/2, ",
   "couples circulate twice, ",
   "couples circulate 2-1/2, ",
   "couples circulate 3 times, ",
   "couples circulate 3-1/2, "};


// BEWARE!!  This enum must track the table "resolve_first_parts".
enum first_part_kind {
   first_part_none,
   first_part_ext,
   first_part_slcl,
   first_part_circ,
   first_part_pthru,
   first_part_trby,
   first_part_xby,
   first_part_dixie
};

// Beware!!  This table must track the definition of enum "first_part_kind".
static Cstring resolve_first_parts[] = {
   (Cstring) 0,
   "extend",
   "slip the clutch",
   "circulate",
   "pass thru",
   "trade by",
   "cross by",
   "dixie grand",};

// BEWARE!!  This enum must track the table "resolve_main_parts".
enum main_part_kind {
   main_part_none,
   main_part_rlg,
   main_part_la,
   main_part_dixgnd,
   main_part_minigrand,
   main_part_prom,
   main_part_revprom,
   main_part_sglprom,
   main_part_rsglprom,
   main_part_circ,
   main_part_swing
};

// Beware!!  This table must track the definition of enum "main_part_kind".
static Cstring resolve_main_parts[] = {
   "???",
   "right and left grand",
   "left allemande",
   "dixie grand, left allemande",
   "mini-grand",
   "promenade",
   "reverse promenade",
   "single file promenade",
   "reverse single file promenade",
   "circle right",
   "swing and promenade"};


struct resolve_descriptor {
   // 0 means accept all such resolves.
   // Each increase cuts the acceptance probability in half when searching
   // randomly.  For example, 4 means accept only 1/16 of such resolves,
   // throwing away the other 15/16.
   // Also, 4 or more means it will not be accepted in the "initial scan" parts of a resolve.
   short int how_bad;
   short int not_in_reconcile;   // Nonzero => do not accept in a reconcile.
   first_part_kind first_part;
   main_part_kind main_part;
};

// BEWARE!!  This list must track the array "resolve_table".
enum resolve_kind {
   resolve_none,
   resolve_rlg,
   resolve_la,
   resolve_ext_rlg,
   resolve_ext_la,
   resolve_bad_ext_la,
   resolve_slipclutch_rlg,
   resolve_slipclutch_la,
   resolve_circ_rlg,
   resolve_circ_la,
   resolve_pth_rlg,
   resolve_pth_la,
   resolve_tby_rlg,
   resolve_tby_la,
   resolve_xby_rlg,
   resolve_xby_la,
   resolve_dixie_grand,
   resolve_bad_dixie_grand,
   resolve_singing_dixie_grand,
   resolve_minigrand,
   resolve_prom,
   resolve_revprom,
   resolve_sglfileprom,
   resolve_revsglfileprom,
   resolve_circle,
   resolve_circle_from_facing_lines
};

// BEWARE!!  This list is keyed to the definition of "resolve_kind".
static const resolve_descriptor resolve_table[] = {
   {2, 1, first_part_none,  main_part_none},     // resolve_none
   {0, 0, first_part_none,  main_part_rlg},      // resolve_rlg
   {0, 0, first_part_none,  main_part_la},       // resolve_la
   {1, 1, first_part_ext,   main_part_rlg},      // resolve_ext_rlg
   {1, 1, first_part_ext,   main_part_la},       // resolve_ext_la
   {3, 1, first_part_ext,   main_part_la},       // resolve_bad_ext_la
   {2, 1, first_part_slcl,  main_part_rlg},      // resolve_slipclutch_rlg
   {2, 1, first_part_slcl,  main_part_la},       // resolve_slipclutch_la
   {3, 1, first_part_circ,  main_part_rlg},      // resolve_circ_rlg
   {3, 1, first_part_circ,  main_part_la},       // resolve_circ_la
   {3, 1, first_part_pthru, main_part_rlg},      // resolve_pth_rlg
   {3, 1, first_part_pthru, main_part_la},       // resolve_pth_la
   {4, 1, first_part_trby,  main_part_rlg},      // resolve_tby_rlg
   {4, 1, first_part_trby,  main_part_la},       // resolve_tby_la
   {1, 1, first_part_xby,   main_part_rlg},      // resolve_xby_rlg
   {1, 1, first_part_xby,   main_part_la},       // resolve_xby_la
   {1, 0, first_part_none,  main_part_dixgnd},   // resolve_dixie_grand
   {4, 1, first_part_none,  main_part_dixgnd},   // resolve_bad_dixie_grand
   {0, 0, first_part_none,  main_part_dixgnd},   // resolve_singing_dixie_grand
   {2, 1, first_part_none,  main_part_minigrand},// resolve_minigrand
   {0, 0, first_part_none,  main_part_prom},     // resolve_prom
   {1, 0, first_part_none,  main_part_revprom},  // resolve_revprom
   {3, 0, first_part_none,  main_part_sglprom},  // resolve_sglfileprom
   {4, 0, first_part_none,  main_part_rsglprom}, // resolve_revsglfileprom
   {2, 0, first_part_none,  main_part_circ},     // resolve_circle
   {2, 0, first_part_none,  main_part_circ}};    // resolve_circle_from_facing_lines


// Some resolves are only legal at certain levels, so there is a "level" field
// in a resolve_tester.  We define these short names to keep the table entries
// from being unwieldy.
enum level_abbreviation {
   MS = l_mainstream,
   XB = cross_by_level,
   DX = dixie_grand_level,
   EX = extend_34_level
};

struct resolve_tester {
   resolve_kind k;
   level_abbreviation level_needed;
   // Notes for "distance" field:
   // Add 0x10 bit for singer-only; these must be last.
   // Also, last item in each table has 0x10 only.
   // Add 0x20 bit -- same as 0x40 if distance is zero, otherwise OK.
   // Add 0x40 bit to make the resolver never find this, though
   //    we will display it if user gets here.
   uint32_t distance;
   int8_t locations[8];
   uint32_t directions;
};

bool configuration::sequence_is_resolved()
{
   return current_resolve().the_item->k != resolve_none;
}


const resolve_tester null_resolve_thing = {
   resolve_none, MS, 0, {0,0,0,0,0,0,0,0}, 0};

const resolve_tester *configuration::null_resolve_ptr = &null_resolve_thing;

// The list is:
// where in the setup to find B1, where in the setup to find G1,
// where in the setup to find B2, where in the setup to find G2,
// where in the setup to find B3, where in the setup to find G3,
// where in the setup to find B4, where in the setup to find G4,
//
// The direction word is, left to right:
// direction of B1, direction of G1,
// direction of B2, direction of G2,
// direction of B3, direction of G3,
// direction of B4, direction of G4,
//
// The assignment of couple number may be rotated freely, of course.  But the person
// in slot 0 of the list (B1 in the example) must be a boy.


static const resolve_tester test_2x2_stuff[] = {
   // Look for at-home getout in two-couple material.
   {resolve_circle,         MS, 4,   {1, 0, -1, -1, 3, 2, -1, -1},  0xAA008800},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_thar_stuff[] = {
   {resolve_rlg,            MS, 2,   {5, 4, 3, 2, 1, 0, 7, 6},     0x8A31A813},
   {resolve_minigrand,      MS, 4,   {5, 0, 3, 6, 1, 4, 7, 2},     0x8833AA11},
   {resolve_prom,           MS, 6,   {5, 4, 3, 2, 1, 0, 7, 6},     0x8833AA11},
   {resolve_slipclutch_rlg, MS, 1,   {5, 2, 3, 0, 1, 6, 7, 4},     0x8138A31A},
   {resolve_la,             MS, 5,   {5, 2, 3, 0, 1, 6, 7, 4},     0xA31A8138},
   {resolve_slipclutch_la,  MS, 6,   {5, 4, 3, 2, 1, 0, 7, 6},     0xA8138A31},
   {resolve_xby_rlg,        XB, 1,   {4, 3, 2, 1, 0, 7, 6, 5},     0x8138A31A},
   {resolve_revprom,        MS, 4,   {2, 3, 0, 1, 6, 7, 4, 5},     0x118833AA},
   {resolve_xby_la,         XB, 4,   {2, 3, 0, 1, 6, 7, 4, 5},     0x138A31A8},
   {resolve_bad_dixie_grand,DX, 0,   {4, 1, 2, 7, 0, 5, 6, 3},     0xAA118833},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_rigger_stuff[] = {
   {resolve_rlg,            MS, 2,   {3, 2, 1, 0, 7, 6, 5, 4},     0x8A31A813},
   {resolve_minigrand,      MS, 4,   {3, 6, 1, 4, 7, 2, 5, 0},     0x8833AA11},
   {resolve_la,             MS, 5,   {3, 1, 0, 6, 7, 5, 4, 2},     0xA31A8138},
   {resolve_bad_dixie_grand,DX, 0,   {2, 7, 0, 5, 6, 3, 4, 1},     0xAA118833},

   // From a strange  rigger.  This rates about
   // 500 milli-Tersoffs, on a scale named after Mike Tersoff, a devotee
   // of extremely strange getout positions.
   {resolve_rlg,            MS, 2,   {3, 2, 1, 0, 7, 6, 5, 4},     0x8AA8A88A},
   {resolve_minigrand,      MS, 4,   {3, 6, 1, 4, 7, 2, 5, 0},     0x88AAAA88},
   {resolve_la,             MS, 5,   {3, 1, 0, 6, 7, 5, 4, 2},     0xA8AA8A88},

   {resolve_none, MS, 0x10}};

static const resolve_tester test_4x4_stuff[] = {
   // "Circle left/right", etc. from squared-set.
   {resolve_circle,         MS, 6,   {2, 1, 14, 13, 10, 9, 6, 5},  0x33AA1188},
   {resolve_circle,         MS, 7,   {5, 2, 1, 14, 13, 10, 9, 6},  0x833AA118},
   {resolve_sglfileprom,    MS, 6,   {2, 1, 14, 13, 10, 9, 6, 5},  0x8833AA11},
   {resolve_sglfileprom,    MS, 7,   {5, 2, 1, 14, 13, 10, 9, 6},  0x18833AA1},
   {resolve_revsglfileprom, MS, 6,   {2, 1, 14, 13, 10, 9, 6, 5},  0xAA118833},
   {resolve_revsglfileprom, MS, 7,   {5, 2, 1, 14, 13, 10, 9, 6},  0x3AA11883},

   // From vertical 8-chain in "O".
   {resolve_rlg,            MS, 3,   {5, 2, 1, 14, 13, 10, 9, 6},  0x8A8AA8A8},
   {resolve_la,             MS, 6,   {2, 1, 14, 13, 10, 9, 6, 5},  0xA8AA8A88},

   // From horizontal 8-chain in "O".
   {resolve_rlg,            MS, 3,   {5, 2, 1, 14, 13, 10, 9, 6},  0x13313113},
   {resolve_la,             MS, 6,   {2, 1, 14, 13, 10, 9, 6, 5},  0x33131131},

   // Weird ones from squared set.
   {resolve_rlg,            MS, 3,   {5, 2, 1, 14, 13, 10, 9, 6},  0x8A31A813},
   {resolve_rlg,            MS, 3,   {5, 2, 1, 14, 13, 10, 9, 6},  0x138A31A8},
   {resolve_la,             MS, 6,   {2, 1, 14, 13, 10, 9, 6, 5},  0x38A31A81},
   {resolve_la,             MS, 6,   {2, 1, 14, 13, 10, 9, 6, 5},  0xA31A8138},

   // From squared set, around the corner.
   {resolve_rlg,            MS, 3,   {5, 2, 1, 14, 13, 10, 9, 6},  0x1A8138A3},
   {resolve_la,             MS, 6,   {2, 1, 14, 13, 10, 9, 6, 5},  0xA8138A31},

   // From squared set, facing directly.
   {resolve_rlg,            MS, 2,   {2, 1, 14, 13, 10, 9, 6, 5},  0x8A31A813},
   {resolve_la,             MS, 7,   {5, 2, 1, 14, 13, 10, 9, 6},  0x38A31A81},

   // From ladder columns, facing directly.
   {resolve_rlg,            MS, 3,   {7, 2, 0, 14, 15, 10, 8, 6},  0x13313113},
   {resolve_rlg,            MS, 1,   {3, 14, 12, 10, 11, 6, 4, 2}, 0x8AA8A88A},
   {resolve_rlg,            MS, 3,   {5, 4, 1, 3, 13, 12, 9, 11},  0x13313113},
   {resolve_rlg,            MS, 1,   {1, 0, 13, 15, 9, 8, 5, 7},   0x8AA8A88A},
   {resolve_la,             MS, 6,   {2, 0, 14, 15, 10, 8, 6, 7},  0x33131131},
   {resolve_la,             MS, 4,   {14, 12, 10, 11, 6, 4, 2, 3}, 0xAA8A88A8},
   {resolve_la,             MS, 6,   {4, 1, 3, 13, 12, 9, 11, 5},  0x33131131},
   {resolve_la,             MS, 4,   {0, 13, 15, 9, 8, 5, 7, 1},   0xAA8A88A8},

   // From pinwheel, facing directly.
   {resolve_rlg,            MS, 3,   {7, 2, 3, 14, 15, 10, 11, 6}, 0x138A31A8},
   {resolve_rlg,            MS, 3,   {5, 7, 1, 3, 13, 15, 9, 11},  0x8A31A813},

   // From pinwheel, all in miniwaves.
   {resolve_rlg,            MS, 3,   {7, 5, 3, 1, 15, 13, 11, 9},  0x138A31A8},
   {resolve_rlg,            MS, 3,   {7, 2, 3, 14, 15, 10, 11, 6}, 0x8A31A813},

   // From pinwheel, some of each.
   {resolve_rlg,            MS, 3,   {7, 2, 3, 14, 15, 10, 11, 6}, 0x13313113},
   {resolve_rlg,            MS, 3,   {5, 7, 3, 1, 13, 15, 11, 9},  0x8A8AA8A8},

   // From pinwheel, others of each.
   {resolve_rlg,            MS, 3,   {7, 2, 3, 14, 15, 10, 11, 6}, 0x8A8AA8A8},
   {resolve_rlg,            MS, 3,   {7, 5, 1, 3, 15, 13, 9, 11},  0x13313113},

   // From clumps.
   {resolve_rlg,            MS, 2,   {3, 1, 14, 0, 11, 9, 6, 8},   0x8A8AA8A8},
   {resolve_rlg,            MS, 4,   {5, 4, 7, 2, 13, 12, 15, 10}, 0x8A8AA8A8},
   {resolve_rlg,            MS, 2,   {1, 0, 3, 14, 9, 8, 11, 6},   0x31311313},
   {resolve_rlg,            MS, 4,   {7, 5, 2, 4, 15, 13, 10, 12}, 0x13133131},
   {resolve_la,             MS, 5,   {3, 0, 14, 9, 11, 8, 6, 1},   0xA8AA8A88},
   {resolve_la,             MS, 7,   {5, 2, 7, 12, 13, 10, 15, 4}, 0xA8AA8A88},
   {resolve_la,             MS, 5,   {1, 14, 3, 8, 9, 6, 11, 0},   0x13113133},
   {resolve_la,             MS, 7,   {7, 4, 2, 13, 15, 12, 10, 5}, 0x31331311},

   // From stairstep lines.
   {resolve_rlg,            MS, 3,   {7, 2, 14, 0, 15, 10, 6, 8},  0x8A8AA8A8},
   {resolve_rlg,            MS, 3,   {5, 4, 3, 1, 13, 12, 11, 9},  0x8A8AA8A8},
   {resolve_rlg,            MS, 1,   {1, 0, 15, 13, 9, 8, 7, 5},   0x31311313},
   {resolve_rlg,            MS, 5,   {11, 6, 2, 4, 3, 14, 10, 12}, 0x13133131},
   {resolve_la,             MS, 6,   {7, 0, 14, 10, 15, 8, 6, 2},  0xA8AA8A88},
   {resolve_la,             MS, 6,   {5, 1, 3, 12, 13, 9, 11, 4},  0xA8AA8A88},
   {resolve_la,             MS, 4,   {1, 13, 15, 8, 9, 5, 7, 0},   0x13113133},
   {resolve_la,             MS, 0,   {11, 4, 2, 14, 3, 12, 10, 6}, 0x31331311},
   {resolve_prom,           MS, 7,   {7, 2, 14, 0, 15, 10, 6, 8},  0x8888AAAA},
   {resolve_prom,           MS, 7,   {5, 4, 3, 1, 13, 12, 11, 9},  0x8888AAAA},
   {resolve_prom,           MS, 5,   {1, 0, 15, 13, 9, 8, 7, 5},   0x33331111},
   {resolve_prom,           MS, 1,   {11, 6, 2, 4, 3, 14, 10, 12}, 0x11113333},
   {resolve_revprom,        MS, 7,   {2, 7, 0, 14, 10, 15, 8, 6},  0xAAAA8888},
   {resolve_revprom,        MS, 7,   {4, 5, 1, 3, 12, 13, 9, 11},  0xAAAA8888},
   {resolve_revprom,        MS, 5,   {0, 1, 13, 15, 8, 9, 5, 7},   0x11113333},
   {resolve_revprom,        MS, 1,   {6, 11, 4, 2, 14, 3, 12, 10}, 0x33331111},

   // From blocks.
   {resolve_rlg,            MS, 3,   {5, 2, 3, 0, 13, 10, 11, 8},  0x8A8AA8A8},
   {resolve_rlg,            MS, 1,   {1, 14, 15, 12, 9, 6, 7, 4},  0x31311313},
   {resolve_la,             MS, 7,   {5, 2, 3, 0, 13, 10, 11, 8},  0xA8A88A8A},
   {resolve_la,             MS, 5,   {1, 14, 15, 12, 9, 6, 7, 4},  0x13133131},

   // From weird stuff
   {resolve_rlg,            MS, 6,   {15, 11, 5, 4, 7, 3, 13, 12}, 0xA8138A31},
   {resolve_rlg,            MS, 0,   {3, 15, 9, 8, 11, 7, 1, 0},   0x31A8138A},
   {resolve_rlg,            MS, 6,   {15, 11, 8, 6, 7, 3, 0, 14},  0xA8138A31},
   {resolve_rlg,            MS, 0,   {3, 15, 12, 10, 11, 7, 4, 2}, 0x31A8138A},

   {resolve_none, MS, 0x10}};

static const resolve_tester test_alamo_stuff[] = {
   // "Circle left/right", etc.
   {resolve_circle,         MS, 6,   {3, 2, 1, 0, 7, 6, 5, 4},  0x33AA1188},
   {resolve_circle,         MS, 7,   {4, 3, 2, 1, 0, 7, 6, 5},  0x833AA118},
   {resolve_sglfileprom,    MS, 6,   {3, 2, 1, 0, 7, 6, 5, 4},  0x8833AA11},
   {resolve_sglfileprom,    MS, 7,   {4, 3, 2, 1, 0, 7, 6, 5},  0x18833AA1},
   {resolve_revsglfileprom, MS, 6,   {3, 2, 1, 0, 7, 6, 5, 4},  0xAA118833},
   {resolve_revsglfileprom, MS, 7,   {4, 3, 2, 1, 0, 7, 6, 5},  0x3AA11883},

   // Around the corner.
   {resolve_rlg,            MS, 3,   {4, 3, 2, 1, 0, 7, 6, 5},  0x1A8138A3},
   {resolve_la,             MS, 6,   {3, 2, 1, 0, 7, 6, 5, 4},  0xA8138A31},

   // Facing directly.
   {resolve_rlg,            MS, 2,   {3, 2, 1, 0, 7, 6, 5, 4},  0x8A31A813},
   {resolve_pth_rlg,        MS, 1,   {3, 0, 1, 6, 7, 4, 5, 2},  0x8138A31A},
   {resolve_la,             MS, 7,   {4, 3, 2, 1, 0, 7, 6, 5},  0x38A31A81},
   {resolve_pth_la,         MS, 6,   {2, 3, 0, 1, 6, 7, 4, 5},  0xA8138A31},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_4x6_stuff[] = {
   {resolve_rlg,            MS, 2,   {23, 6, 3, 2, 11, 18, 15, 14},0x8A31A813},
   {resolve_la,             MS, 7,   {14, 23, 6, 3, 2, 11, 18, 15},0x38A31A81},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_c1phan_stuff[] = {
   // Various circle/RLG/LA from phantom-type squared-set.
   {resolve_circle,         MS, 6,   {11, 6, 7, 2, 3, 14, 15, 10}, 0x33AA1188},
   {resolve_circle,         MS, 7,   {10, 11, 6, 7, 2, 3, 14, 15}, 0x833AA118},
   {resolve_la,             MS, 6,   {11, 6, 7, 2, 3, 14, 15, 10}, 0xA8138A31},
   {resolve_la,             MS, 7,   {10, 11, 6, 7, 2, 3, 14, 15}, 0x38A31A81},
   {resolve_rlg,            MS, 2,   {11, 6, 7, 2, 3, 14, 15, 10}, 0x8A31A813},
   {resolve_rlg,            MS, 3,   {10, 11, 6, 7, 2, 3, 14, 15}, 0x1A8138A3},
   {resolve_sglfileprom,    MS, 6,   {11, 6, 7, 2, 3, 14, 15, 10}, 0x8833AA11},
   {resolve_sglfileprom,    MS, 7,   {10, 11, 6, 7, 2, 3, 14, 15}, 0x18833AA1},
   {resolve_revsglfileprom, MS, 6,   {11, 6, 7, 2, 3, 14, 15, 10}, 0xAA118833},
   {resolve_revsglfileprom, MS, 7,   {10, 11, 6, 7, 2, 3, 14, 15}, 0x3AA11883},
   // From phantoms, all facing.
   {resolve_rlg,            MS, 3,   {10, 8, 6, 4, 2, 0, 14, 12},  0x138A31A8},
   {resolve_rlg,            MS, 3,   {9, 11, 5, 7, 1, 3, 13, 15},  0x8A31A813},
   // From phantoms, all in miniwaves.
   {resolve_rlg,            MS, 3,   {11, 9, 7, 5, 3, 1, 15, 13},  0x138A31A8},
   {resolve_rlg,            MS, 3,   {10, 8, 6, 4, 2, 0, 14, 12},  0x8A31A813},
   // From phantoms, some of each.
   {resolve_rlg,            MS, 3,   {10, 8, 6, 4, 2, 0, 14, 12},  0x13313113},
   {resolve_rlg,            MS, 3,   {9, 11, 7, 5, 1, 3, 15, 13},  0x8A8AA8A8},
   // From phantoms, others of each.
   {resolve_rlg,            MS, 3,   {10, 8, 6, 4, 2, 0, 14, 12},  0x8A8AA8A8},
   {resolve_rlg,            MS, 3,   {11, 9, 5, 7, 3, 1, 13, 15},  0x13313113},
   // From phantoms, all promenade.
   {resolve_prom,           MS, 7,   {11, 9, 7, 5, 3, 1, 15, 13},  0x118833AA},
   {resolve_prom,           MS, 7,   {10, 8, 6, 4, 2, 0, 14, 12},  0x8833AA11},
   {resolve_revprom,        MS, 7,   {9, 11, 5, 7, 1, 3, 13, 15},  0x33AA1188},
   {resolve_revprom,        MS, 7,   {8, 10, 4, 6, 0, 2, 12, 14},  0xAA118833},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_galaxy_stuff[] = {
   {resolve_rlg,            MS, 2,   {5, 4, 3, 2, 1, 0, 7, 6},     0x1A313813},
   {resolve_rlg,            MS, 2,   {5, 4, 3, 2, 1, 0, 7, 6},     0x8A81A8A3},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_qtag_stuff[] = {
   // From 1/4 tag.
   {resolve_dixie_grand,    DX, 2,   {5, 0, 2, 7, 1, 4, 6, 3},     0x8AAAA888},
   {resolve_dixie_grand,    DX, 2,   {4, 1, 2, 7, 0, 5, 6, 3},     0x33AA1188},
   {resolve_ext_rlg,        MS, 5,   {7, 5, 4, 2, 3, 1, 0, 6},     0xA88A8AA8},
   {resolve_ext_la,         MS, 6,   {3, 2, 1, 0, 7, 6, 5, 4},     0xA8AA8A88},
   // From 3/4 tag.
   {resolve_rlg,            MS, 4,   {5, 4, 3, 2, 1, 0, 7, 6},     0xAA8A88A8},
   {resolve_minigrand,      MS, 6,   {5, 0, 3, 6, 1, 4, 7, 2},     0xA8888AAA},
   {resolve_la,             MS, 7,   {4, 2, 3, 1, 0, 6, 7, 5},     0xA8A88A8A},

   // From diamonds with points facing each other.
   {resolve_rlg,            MS, 4,   {5, 4, 3, 2, 1, 0, 7, 6},     0x138A31A8},
   {resolve_minigrand,      MS, 6,   {5, 0, 3, 6, 1, 4, 7, 2},     0x118833AA},
   {resolve_la,             MS, 7,   {4, 2, 3, 1, 0, 6, 7, 5},     0x38A31A81},

   // From a strange "6x2 acey deucey" situation.  This rates about
   // 500 milli-Tersoffs, on a scale named after Mike Tersoff, a devotee
   // of extremely strange getout positions.
   {resolve_rlg,            MS, 4,   {5, 4, 3, 2, 1, 0, 7, 6},     0x8A8AA8A8},
   {resolve_minigrand,      MS, 6,   {5, 0, 3, 6, 1, 4, 7, 2},     0x8888AAAA},
   {resolve_la,             MS, 7,   {4, 2, 3, 1, 0, 6, 7, 5},     0x88A8AA8A},

   // From a really weird 1/4-tag-like setup, about 900 milli-Tersoffs.
   {resolve_rlg,            MS, 3,   {4, 2, 3, 1, 0, 6, 7, 5},     0x1A313813},

   // Singers only.
   // Swing/prom from 3/4 tag, ends sashayed (normal case is above).
   {resolve_rlg,            MS, 0x14,{4, 5, 3, 2, 0, 1, 7, 6},     0xAA8A88A8},
   // Swing/prom from 3/4 tag, centers traded, ends normal.
   {resolve_rlg,            MS, 0x14,{5, 4, 2, 3, 1, 0, 6, 7},     0xAAA8888A},
   // Swing/prom from 3/4 tag, centers traded, ends sashayed.
   {resolve_rlg,            MS, 0x14,{4, 5, 2, 3, 0, 1, 6, 7},     0xAAA8888A},
   // From diamonds with points facing each other but switched from the 0x138A31A8 case above.
   {resolve_rlg,            MS, 4,   {4, 5, 3, 2, 0, 1, 7, 6},     0x318A13A8},
   // More diamonds with points facing each other.
   {resolve_rlg,            MS, 4,   {5, 4, 2, 3, 1, 0, 6, 7},     0x13A8318A},
   // More diamonds with points facing each other.
   {resolve_rlg,            MS, 4,   {4, 5, 2, 3, 0, 1, 6, 7},     0x31A8138A},
   // 1/4-tag-like spots, but ends in RH miniwave.  Various milli-Tersoff scores.
   {resolve_rlg,            MS, 4,   {4, 5, 3, 2, 0, 1, 7, 6},     0xA88A8AA8},
   {resolve_rlg,            MS, 4,   {4, 5, 2, 3, 0, 1, 6, 7},     0xA8A88A8A},
   {resolve_rlg,            MS, 4,   {5, 4, 2, 3, 1, 0, 6, 7},     0x8AA8A88A},
   {resolve_rlg,            MS, 4,   {5, 4, 3, 2, 1, 0, 7, 6},     0x8A8AA8A8},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_2x6_stuff[] = {
   // From "Z" 8-chain.
   {resolve_rlg,            MS, 3,   {7, 6, 4, 3, 1, 0, 10, 9},    0x13313113},
   {resolve_rlg,            MS, 3,   {8, 7, 5, 4, 2, 1, 11, 10},   0x13313113},
   {resolve_la,             MS, 6,   {6, 4, 3, 1, 0, 10, 9, 7},    0x33131131},
   {resolve_la,             MS, 6,   {7, 5, 4, 2, 1, 11, 10, 8},   0x33131131},
   // From outer triple boxes.
   {resolve_rlg,            MS, 3,   {7, 6, 4, 5, 1, 0, 10, 11},   0x8A8AA8A8},
   {resolve_la,             MS, 6,   {7, 5, 4, 0, 1, 11, 10, 6},   0xA8AA8A88},
   {resolve_prom,           MS, 7,   {7, 6, 4, 5, 1, 0, 10, 11},   0x8888AAAA},
   // From parallelogram waves.
   {resolve_rlg,            MS, 3,   {7, 6, 2, 3, 1, 0, 8, 9},     0x8A8AA8A8},
   {resolve_la,             MS, 6,   {7, 3, 2, 0, 1, 9, 8, 6},     0xA8AA8A88},
   {resolve_rlg,            MS, 3,   {9, 8, 4, 5, 3, 2, 10, 11},   0x8A8AA8A8},
   {resolve_la,             MS, 6,   {9, 5, 4, 2, 3, 11, 10, 8},   0xA8AA8A88},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_2x8_stuff[] = {
   // From outer quadruple boxes.
   {resolve_rlg,            MS, 3,   {9, 8, 6, 7, 1, 0, 14, 15},   0x8A8AA8A8},
   {resolve_la,             MS, 6,   {9, 7, 6, 0, 1, 15, 14, 8},   0xA8AA8A88},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_3x4_stuff[] = {
   // From offset lines.
   {resolve_rlg,            MS, 3,   {7, 6, 5, 4, 1, 0, 11, 10},   0x8A8AA8A8},
   {resolve_rlg,            MS, 3,   {5, 4, 2, 3, 11, 10, 8, 9},   0x8A8AA8A8},
   {resolve_la,             MS, 6,   {7, 4, 5, 0, 1, 10, 11, 6},   0xA8AA8A88},
   {resolve_la,             MS, 6,   {5, 3, 2, 10, 11, 9, 8, 4},   0xA8AA8A88},
   {resolve_prom,           MS, 7,   {7, 6, 5, 4, 1, 0, 11, 10},   0x8888AAAA},
   {resolve_prom,           MS, 7,   {5, 4, 2, 3, 11, 10, 8, 9},   0x8888AAAA},
   {resolve_revprom,        MS, 7,   {6, 7, 4, 5, 0, 1, 10, 11},   0xAAAA8888},
   {resolve_revprom,        MS, 7,   {4, 5, 3, 2, 10, 11, 9, 8},   0xAAAA8888},

   // Far apart lines.
   {resolve_rlg,            MS, 3,   {7, 6, 2, 3, 1, 0, 8, 9},     0x8A8AA8A8},
   {resolve_la,             MS, 6,   {7, 3, 2, 0, 1, 9, 8, 6},     0xA8AA8A88},

   // From sort of skewed offset 8-chain.
   // It may have been a mistake to write these.
   //   {resolve_rlg,       MS, 2,   {6, 4, 11, 1, 0, 10, 5, 7},   0x8A8AA8A8},
   //   {resolve_rlg,       MS, 2,   {4, 3, 5, 2, 11, 8, 10, 9},   0x8A8AA8A8},
   //   {resolve_la,        MS, 5,   {4, 11, 1, 0, 10, 5, 7, 6},   0xA8AA8A88},
   //   {resolve_la,        MS, 5,   {3, 2, 5, 10, 9, 8, 11, 4},   0xAA8A88A8},
   {resolve_rlg,            MS, 1,   {4, 5, 1, 0, 10, 11, 7, 6},   0x31311313},
   {resolve_rlg,            MS, 1,   {3, 2, 11, 10, 9, 8, 5, 4},   0x31311313},
   {resolve_la,             MS, 4,   {5, 1, 0, 10, 11, 7, 6, 4},   0x13113133},
   {resolve_la,             MS, 4,   {2, 11, 10, 9, 8, 5, 4, 3},   0x13113133},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_4dmd_stuff[] = {
   // These test for people in a miniwave as centers of the outer diamonds,
   // while the others are facing each other as points.  There are three of each
   // to allow for the outsides to be symmetrical (points of the center diamonds)
   // or unsymmetrical (points of a center diamond and the adjacent outer diamond).
   {resolve_la,             MS, 7,   {8, 4, 5, 1, 0, 12, 13, 9},   0x38A31A81},
   {resolve_la,             MS, 7,   {9, 4, 5, 2, 1, 12, 13, 10},  0x38A31A81},
   {resolve_la,             MS, 7,   {10, 4, 5, 3, 2, 12, 13, 11}, 0x38A31A81},
   {resolve_rlg,            MS, 4,   {9, 8, 5, 4, 1, 0, 13, 12},   0x138A31A8},
   {resolve_rlg,            MS, 4,   {10, 9, 5, 4, 2, 1, 13, 12},  0x138A31A8},
   {resolve_rlg,            MS, 4,   {11, 10, 5, 4, 3, 2, 13, 12}, 0x138A31A8},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_3dmd_stuff[] = {
   // Some points facing each other, others around the corner to a center.
   {resolve_la,             MS, 7,   {6, 3, 2, 1, 0, 9, 8, 7},     0x38131A31},
   {resolve_la,             MS, 7,   {7, 6, 3, 2, 1, 0, 9, 8},     0x31A31381},
   {resolve_rlg,            MS, 3,   {7, 6, 3, 2, 1, 0, 9, 8},     0x138131A3},
   {resolve_rlg,            MS, 5,   {8, 7, 6, 3, 2, 1, 0, 9},     0x131A3138},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_bigdmd_stuff[] = {
   // From  miniwaves in "common point diamonds".
   {resolve_rlg,            MS, 2,   {7, 6, 3, 2, 1, 0, 9, 8},     0x8A31A813},
   {resolve_rlg,            MS, 2,   {4, 5, 3, 2, 10, 11, 9, 8},   0x8A31A813},
   {resolve_la,             MS, 5,   {7, 2, 3, 0, 1, 8, 9, 6},     0xA31A8138},
   {resolve_la,             MS, 5,   {4, 2, 3, 11, 10, 8, 9, 5},   0xA31A8138},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_deepqtg_stuff[] = {
   {resolve_circle,         MS, 6,   {11, 2, 1, 0, 5, 8, 7, 6},    0x33AA1188},
   {resolve_circle,         MS, 7,   {6, 11, 2, 1, 0, 5, 8, 7},    0x833AA118},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_deepxwv_stuff[] = {
   {resolve_rlg,            MS, 4,   {5, 8, 7, 6, 11, 2, 1, 0},    0x138A31A8},
   {resolve_la,             MS, 7,   {8, 6, 7, 11, 2, 0, 1, 5},    0x38A31A81},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_d2x7_stuff[] = {
   {resolve_rlg,            MS, 4,   {10, 9, 8, 7, 3, 2, 1, 0},    0x138A31A8},
   {resolve_rlg,            MS, 4,   {3, 4, 5, 6, 10, 11, 12, 13}, 0x138A31A8},
   {resolve_prom,           MS, 0,   {10, 9, 8, 7, 3, 2, 1, 0},    0x118833AA},
   {resolve_prom,           MS, 0,   {3, 4, 5, 6, 10, 11, 12, 13}, 0x118833AA},
   {resolve_la,             MS, 7,   {10, 7, 8, 2, 3, 0, 1, 9},    0x38A31A81},
   {resolve_la,             MS, 7,   {12, 4, 3, 6, 5, 11, 10, 13}, 0x8138A31A},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_3x6_stuff[] = {
   {resolve_rlg,            MS, 4,   {12, 11, 7, 6, 3, 2, 16, 15}, 0x138A31A8},
   {resolve_la,             MS, 7,   {11, 6, 7, 3, 2, 15, 16, 12}, 0x38A31A81},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_spindle_stuff[] = {
   // These test for people looking around the corner.
   {resolve_rlg,            MS, 3,   {4, 3, 2, 1, 0, 7, 6, 5},     0x13313113},
   {resolve_rlg,            MS, 3,   {4, 3, 2, 1, 0, 7, 6, 5},     0x1A313813},
   {resolve_la,             MS, 7,   {4, 3, 2, 1, 0, 7, 6, 5},     0x33131131},
   {resolve_la,             MS, 7,   {4, 3, 2, 1, 0, 7, 6, 5},     0x38131A31},
   {resolve_rlg,            MS, 2,   {3, 2, 1, 0, 7, 6, 5, 4},     0x31311313},
   {resolve_rlg,            MS, 2,   {3, 2, 1, 0, 7, 6, 5, 4},     0x8131A313},
   {resolve_la,             MS, 6,   {3, 2, 1, 0, 7, 6, 5, 4},     0x33131131},
   {resolve_la,             MS, 6,   {3, 2, 1, 0, 7, 6, 5, 4},     0xA3138131},
   {resolve_circle,         MS, 7,   {4, 3, 2, 1, 0, 7, 6, 5},     0x83AAA188},
   {resolve_circle,         MS, 5,   {3, 2, 1, 0, 7, 6, 5, 4},     0x3AAA1888},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_2x4_stuff[] = {
   // 8-chain.
   {resolve_rlg,            MS, 3,   {5, 4, 3, 2, 1, 0, 7, 6},     0x13313113},
   {resolve_minigrand,      MS, 5,   {5, 0, 3, 6, 1, 4, 7, 2},     0x11333311},
   {resolve_la,             MS, 6,   {4, 3, 2, 1, 0, 7, 6, 5},     0x33131131},
   {resolve_pth_rlg,        MS, 2,   {5, 2, 3, 0, 1, 6, 7, 4},     0x11313313},
   {resolve_pth_la,         MS, 5,   {2, 3, 0, 1, 6, 7, 4, 5},     0x13133131},
   {resolve_bad_dixie_grand,DX, 1,   {4, 1, 2, 7, 0, 5, 6, 3},     0x33111133},

   // "Centers touch 1/4".
   {resolve_la,             MS, 6,   {4, 3, 2, 1, 0, 7, 6, 5},     0x33A8118A},

   // Trade-by.
   {resolve_rlg,            MS, 2,   {4, 3, 2, 1, 0, 7, 6, 5},     0x11313313},
   {resolve_minigrand,      MS, 4,   {4, 7, 2, 5, 0, 3, 6, 1},     0x13333111},
   {resolve_la,             MS, 7,   {5, 4, 3, 2, 1, 0, 7, 6},     0x31131331},
   {resolve_tby_rlg,        MS, 3,   {6, 3, 4, 1, 2, 7, 0, 5},     0x11113333},
   {resolve_tby_la,         MS, 0,   {5, 6, 3, 4, 1, 2, 7, 0},     0x31111333},

   // Waves.
   {resolve_rlg,            MS, 3,   {5, 4, 2, 3, 1, 0, 6, 7},     0x8A8AA8A8},
   {resolve_minigrand,      MS, 5,   {5, 0, 2, 7, 1, 4, 6, 3},     0x8888AAAA},
   {resolve_la,             MS, 6,   {5, 3, 2, 0, 1, 7, 6, 4},     0xA8AA8A88},
   {resolve_ext_rlg,        EX, 2,   {5, 3, 2, 0, 1, 7, 6, 4},     0x8A88A8AA},
   {resolve_bad_ext_la,     EX, 7,   {5, 4, 2, 3, 1, 0, 6, 7},     0xA8A88A8A},
   {resolve_circ_rlg,       MS, 1,   {5, 0, 2, 7, 1, 4, 6, 3},     0x8888AAAA},
   {resolve_circ_la,        MS, 0,   {5, 7, 2, 4, 1, 3, 6, 0},     0xAAA8888A},
   {resolve_xby_rlg,        XB, 2,   {4, 2, 3, 1, 0, 6, 7, 5},     0x8A88A8AA},
   {resolve_xby_la,         XB, 5,   {3, 2, 0, 1, 7, 6, 4, 5},     0xA88A8AA8},
   {resolve_bad_dixie_grand,DX, 0x47,{3, 6, 0, 5, 7, 2, 4, 1},     0xAA8888AA},

   // From T-bone setup, ends facing.
   {resolve_rlg,            MS, 2,   {4, 3, 2, 1, 0, 7, 6, 5},     0x8A31A813},
   {resolve_la,             MS, 7,   {5, 4, 3, 2, 1, 0, 7, 6},     0x38A31A81},
   {resolve_bad_dixie_grand,DX, 2,   {5, 2, 3, 0, 1, 6, 7, 4},     0x33AA1188},

   // RLG from centers facing and ends in miniwaves.
   {resolve_rlg,            MS, 2,   {4, 3, 2, 1, 0, 7, 6, 5},     0x31311313},
   // LA from lines-out.  This has been decreed to be a horrible resolve.
   {resolve_la,             MS, 0x46,{4, 3, 2, 1, 0, 7, 6, 5},     0xA8888AAA},
   // Same thing, other gender.
   {resolve_rlg,            MS, 0x41,{3, 2, 1, 0, 7, 6, 5, 4},     0x8888AAAA},
   // From wacky T-bone.
   {resolve_la,             MS, 6,   {4, 3, 2, 1, 0, 7, 6, 5},     0x38131A31},

   // From 2FL.
   {resolve_prom,           MS, 7,   {5, 4, 2, 3, 1, 0, 6, 7},     0x8888AAAA},
   {resolve_revprom,        MS, 5,   {3, 2, 0, 1, 7, 6, 4, 5},     0xAA8888AA},

   // "circle left/right" from pseudo squared-set, normal.
   {resolve_circle,         MS, 6,   {4, 3, 2, 1, 0, 7, 6, 5},     0x33AA1188},
   // "circle left/right" from pseudo squared-set, sashayed.
   // If the circling distance is zero, that's fine.  Resolve is "at home".
   {resolve_circle,         MS, 7,   {5, 4, 3, 2, 1, 0, 7, 6},     0x833AA118},

   // "circle left/right" from lines-in, normal.  The circling distance will always be nonzero.
   {resolve_circle_from_facing_lines,         MS, 7,   {5, 4, 3, 2, 1, 0, 7, 6},     0x88AAAA88},
   // "circle left/right" from lines-in, sashayed.  If the circling distance is zero,
   // we don't actively seek this, because they aren't really squared up.
   {resolve_circle_from_facing_lines,         MS, 0x26,{4, 3, 2, 1, 0, 7, 6, 5},     0x8AAAA888},

   // From DPT.
   {resolve_dixie_grand,    DX, 2,   {5, 2, 4, 7, 1, 6, 0, 3},     0x33311113},

   // From magic column.
   {resolve_dixie_grand,    DX, 3,   {6, 3, 5, 0, 2, 7, 1, 4},     0x33331111},
   {resolve_dixie_grand,    DX, 1,   {4, 1, 3, 6, 0, 5, 7, 2},     0x33111133},

   // From columns.
   {resolve_sglfileprom,    MS, 7,   {5, 4, 3, 2, 1, 0, 7, 6},     0x11333311},
   {resolve_sglfileprom,    MS, 6,   {4, 3, 2, 1, 0, 7, 6, 5},     0x13333111},
   {resolve_revsglfileprom, MS, 7,   {5, 4, 3, 2, 1, 0, 7, 6},     0x33111133},
   {resolve_revsglfileprom, MS, 6,   {4, 3, 2, 1, 0, 7, 6, 5},     0x31111333},

   // From T-bone.
   {resolve_sglfileprom,    MS, 7,   {5, 4, 3, 2, 1, 0, 7, 6},     0x18833AA1},
   {resolve_sglfileprom,    MS, 6,   {4, 3, 2, 1, 0, 7, 6, 5},     0x8833AA11},
   {resolve_revsglfileprom, MS, 7,   {5, 4, 3, 2, 1, 0, 7, 6},     0x3AA11883},
   {resolve_revsglfileprom, MS, 6,   {4, 3, 2, 1, 0, 7, 6, 5},     0xAA118833},

   // From T-bone mixed 8-chain and waves.
   {resolve_rlg,            MS, 3,   {5, 4, 2, 3, 1, 0, 6, 7},     0x138A31A8},
   {resolve_rlg,            MS, 3,   {5, 4, 3, 2, 1, 0, 7, 6},     0x8A31A813},
   {resolve_la,             MS, 6,   {4, 3, 2, 1, 0, 7, 6, 5},     0x38A31A81},
   {resolve_la,             MS, 6,   {5, 3, 2, 0, 1, 7, 6, 4},     0xA31A8138},

   // Singers only.
   // Swing/prom from waves, boys looking in.
   {resolve_rlg,            MS,   0x13,   {5, 4, 3, 2, 1, 0, 7, 6},     0x8AA8A88A},
   // Swing/prom from waves, girls looking in.
   {resolve_rlg,            MS,   0x13,   {4, 5, 2, 3, 0, 1, 6, 7},     0xA88A8AA8},
   // Swing/prom from lines-out.
   {resolve_rlg,            MS,   0x13,   {4, 5, 2, 3, 0, 1, 6, 7},     0xAA8888AA},
   // Same as cross-by-LA from waves (above), but it's mainstream here.
   {resolve_rlg,            MS,   0x11,   {3, 2, 0, 1, 7, 6, 4, 5},     0xA88A8AA8},
   // 8-chain, boys in center.
   {resolve_rlg,            MS,   0x13,   {5, 4, 2, 3, 1, 0, 6, 7},     0x13133131},
   // 8-chain, girls in center.
   {resolve_rlg,            MS,   0x11,   {3, 2, 0, 1, 7, 6, 4, 5},     0x31131331},
   // Trade-by, ends sashayed.
   {resolve_rlg,            MS,   0x12,   {4, 3, 1, 2, 0, 7, 5, 6},     0x11133331},
   // trade-by, centers sashayed.
   {resolve_rlg,            MS,   0x14,   {6, 5, 3, 4, 2, 1, 7, 0},     0x13113133},
   // Dixie grand/swing/prom from DPT.
   {resolve_singing_dixie_grand,DX,011,   {4, 2, 1, 7, 0, 6, 5, 3},     0x33111133},
   // From T-bone setup, ends facing.
   {resolve_rlg,            MS, 2,   {3, 4, 2, 1, 7, 0, 6, 5},     0xA8318A13},
   {resolve_rlg,            MS, 2,   {4, 3, 1, 2, 0, 7, 5, 6},     0x8A13A831},
   {resolve_rlg,            MS, 2,   {3, 4, 1, 2, 7, 0, 5, 6},     0xA8138A31},
   {resolve_none, MS, 0x10}};

static const resolve_tester test_hrgl_stuff[] = {
   {resolve_rlg,            MS, 5,   {6, 5, 7, 4, 2, 1, 3, 0},     0xA3138131},
   {resolve_la,             MS, 1,   {6, 5, 7, 4, 2, 1, 3, 0},     0x8131A313},
   {resolve_dixie_grand,    DX, 4,   {6, 4, 5, 3, 2, 0, 1, 7},     0x8133A311},
   {resolve_dixie_grand,    DX, 4,   {5, 2, 7, 0, 1, 6, 3, 4},     0x38331A11},
   {resolve_none, MS, 0x10}};


void configuration::calculate_resolve()
{
   const resolve_tester *testptr;
   int i;
   uint32_t singer_offset = 0;

   if (ui_options.singing_call_mode == 1) singer_offset = 0600;
   else if (ui_options.singing_call_mode == 2) singer_offset = 0200;

   switch (state.kind) {
   case s2x2:
      if (two_couple_calling)
         testptr = test_2x2_stuff;
      else
         goto no_resolve;
      break;
   case s2x4:
      testptr = test_2x4_stuff; break;
   case s3x4:
      testptr = test_3x4_stuff; break;
   case s2x6:
      testptr = test_2x6_stuff; break;
   case s2x8:
      testptr = test_2x8_stuff; break;
   case s_qtag:
      testptr = test_qtag_stuff; break;
   case s_hrglass:
      testptr = test_hrgl_stuff; break;
   case s3dmd:
      testptr = test_3dmd_stuff; break;
   case s4dmd:
      testptr = test_4dmd_stuff; break;
   case sbigdmd:
      testptr = test_bigdmd_stuff; break;
   case sdeepqtg:
      testptr = test_deepqtg_stuff; break;
   case sdeepxwv:
      testptr = test_deepxwv_stuff; break;
   case sd2x7:
      testptr = test_d2x7_stuff; break;
   case s3x6:
      testptr = test_3x6_stuff; break;
   case s4x4:
      testptr = test_4x4_stuff; break;
   case s4x6:
      testptr = test_4x6_stuff; break;
   case s_c1phan:
      testptr = test_c1phan_stuff; break;
   case s_galaxy:
      testptr = test_galaxy_stuff; break;
   case s_crosswave: case s_thar:
      // This makes use of the fact that the person numbering
      // in crossed lines and thars is identical.
      testptr = test_thar_stuff; break;
   case s_rigger:
      testptr = test_rigger_stuff; break;
   case s_spindle:
      testptr = test_spindle_stuff; break;
   case s_alamo:
      testptr = test_alamo_stuff; break;
   default: goto no_resolve;
   }

   do {
      uint32_t directionword;
      uint32_t firstperson = state.people[testptr->locations[0]].id1 & 0700;
      if (firstperson & 0100) goto not_this_one;

      // We run the tests in descending order, because the test for i=0 is especially
      // likely to pass (since the person ID is known to match), and we want to find
      // failures as quickly as possible.

      for (i=7,directionword=testptr->directions ; i>=0 ; i--,directionword>>=4) {
         uint32_t expected_id = (i << 6) + ((i&1) ? singer_offset : 0);

         // The adds of "expected_id" and "firstperson" may overflow out of the "700" bits
         // into the next 2 bits.  (One bit for each add.)

         if (((state.people[testptr->locations[i]].id1 ^
              (expected_id + firstperson + (directionword & 0xF))) &
              0777) && (directionword & 0xF) != 0)  // Skip some tests if doing two couple.
            goto not_this_one;
      }

      if (calling_level < (dance_level) testptr->level_needed ||
          (testptr->k == resolve_minigrand && !allowing_minigrand)) goto not_this_one;

      resolve_flag.distance =
         ((state.rotation << 1) + (firstperson >> 6) + testptr->distance) & 7;
      if (two_couple_calling && resolve_flag.distance != 0)
         goto not_this_one;    // Must be at home.
      resolve_flag.the_item = testptr;
      return;

      not_this_one: ;
   }
   while (
          // always do next one if it doesn't have the singer-only mark.
          !((++testptr)->distance & 0x10) ||
          // Even if it has the mark, do it if this is a singer
          // and it isn't really the end of the table.
          (ui_options.singing_call_mode != 0 && testptr->k != resolve_none));

   // Too bad.

 no_resolve:

   init_resolve();   // Set it to the null resolve.
}



uint32_t random_recent_history[128];
int random_count;
int random_top_level_start;


void ui_utils::write_aproximately()
{
   if (configuration::current_config().state.result_flags.misc & RESULTFLAG__IMPRECISE_ROT)
      writestuff("approximately ");
}


// This assumes that "sequence_is_resolved" passes.
void ui_utils::write_resolve_text(bool doing_file)
{
   resolve_indicator & r = configuration::current_resolve();
   int distance = r.distance;
   resolve_kind index = r.the_item->k;

   if (configuration::current_config().state.eighth_rotation)
      distance++;

   distance &= 7;

   if (doing_file && !ui_options.singlespace_mode) doublespace_file();

   if (index == resolve_circle || index == resolve_circle_from_facing_lines) {
      if (distance == 0) {
         if (index == resolve_circle_from_facing_lines) {
            writestuff("in resolved facing lines with ");
            write_aproximately();
            writestuff("zero circling distance");
         }
         else {
            write_aproximately();
            writestuff("at home");
         }
      }
      else {
         writestuff("circle left ");
         write_aproximately();
         writestuff(resolve_distances[8 - distance]);
         writestuff(" or right ");
         write_aproximately();
         writestuff(resolve_distances[distance]);
      }
   }
   else {
      first_part_kind first = resolve_table[index].first_part;
      main_part_kind mainpart = resolve_table[index].main_part;

      // In a singer, "pass thru, allemande left", "trade by, allemande left", or
      // "cross by, allemande left" can be just "swing and promenade".

      if (ui_options.singing_call_mode != 0) {
         if (index == resolve_pth_la ||
             index == resolve_tby_la ||
             index == resolve_xby_la) {
            first = first_part_none;
            mainpart = main_part_swing;
         }
         else if (index == resolve_singing_dixie_grand) {
            first = first_part_dixie;
            mainpart = main_part_swing;
         }
      }

      if (first != first_part_none) {
         writestuff(resolve_first_parts[first]);
         if (doing_file) {
            newline();
            if (!ui_options.singlespace_mode) doublespace_file();
         }
         else
            writestuff(", ");
      }

      if (ui_options.singing_call_mode != 0 && mainpart == main_part_rlg) {
         mainpart = main_part_swing;
         distance ^= 4;
      }

      if (index == resolve_revprom || index == resolve_revsglfileprom)
         distance = (-distance) & 7;

      writestuff(resolve_main_parts[mainpart]);
      writestuff("  (");
      write_aproximately();

      if (distance == 0) {
         writestuff("at home");
      }
      else {
         writestuff(resolve_distances[distance]);
         writestuff(" promenade");
      }

      if (allow_bend_home_getout) {
         if ((mainpart == main_part_prom || mainpart == main_part_revprom) &&
             (configuration::current_config().state.kind == s2x4 || distance == 0) &&
             !(configuration::current_config().state.result_flags.misc & RESULTFLAG__IMPRECISE_ROT)) {
            writestuff(", or ");
            writestuff(circ_to_home_distances[distance]);
            writestuff("bend the line, you're home");
         }
      }

      writestuff(")");
   }
}



// These variables are actually local to inner_search, but they are
// expected to be preserved across the throw, so they must be static.

static int perm_indices[8];
static int hashed_random_list[5];
static parse_block *inner_parse_mark, *outer_parse_mark;
static int history_insertion_point;    // Where our resolve should lie in the history.
                                       // This is normally the end of the history, but
                                       // may be earlier if doing a reconcile.  We clobber
                                       // everything in the given history past this point.
static int history_save;               // Where we are inserting calls now.  This moves
                                       // forward as we build multiple-call resolves.

static bool inner_search(command_kind goal,
                         resolve_rec *new_resolve,
                         int insertion_depth,
                         int insertion_width)
{
   int i, j;
   uint32_t directions, p, q;
   double CLOCKS_TO_RESOLVE;

   if (ui_options.resolve_test_minutes > 0)
      CLOCKS_TO_RESOLVE = (double) ui_options.resolve_test_minutes * 60.0 * ((double) CLOCKS_PER_SEC);
   else
      CLOCKS_TO_RESOLVE = 5.0 * ((double) CLOCKS_PER_SEC);

   history_insertion_point = huge_history_ptr;

   if (goal == command_reconcile) {
      history_insertion_point -= insertion_depth;    // This now points to the end of the insertion region.

      const setup & insertion_start_setup = configuration::history[history_insertion_point].state;

      goal_rotation = insertion_start_setup.rotation;
      goal_kind = insertion_start_setup.kind;
      if (attr::klimit(goal_kind) != 7) return false;
      for (j=0; j<8; j++)
         goal_directions[j] = insertion_start_setup.people[j].id1 & d_mask;

      for (j=0; j<8; j++) {
         perm_indices[j] = -1;
         for (i=0; i<8; i++)
            if ((insertion_start_setup.people[i].id1 &
                 PID_MASK) ==
                perm_array[j])
               perm_indices[j] = i;
         if (perm_indices[j] < 0) return false;      // Didn't find the person????
      }

      history_insertion_point -= insertion_width;    // Now it points to the beginning of the insertion region.
   }

   history_save = history_insertion_point;

   // Since these variables are expected to be preserved
   // across the throw, they must be volatile.
   volatile int little_count = 0;
   volatile int attempt_count = 0;

   int32_t air_start_time = clock();
   double big_resolve_time = 0.0;
   hashed_random_list[0] = 0;

   // Mark the parse block allocation, so that we throw away the garbage created by failing attempts.
   inner_parse_mark = outer_parse_mark = get_parse_block_mark();

   // This loop searches through a group of twenty single-call resolves, then a group
   // of twenty two-call resolves, then a group of twenty three-call resolves,
   // repeatedly.  Any time it finds a resolve in less than the length of the sequence
   // it is looking for, it of course accepts it.  Why don't we simply always search
   // for three call resolves and accept shorter ones that it might stumble upon?
   // Because this might make the program "lazy": it would settle for long resolves
   // rather than looking harder for short ones.  We bias it in favor of short
   // resolves by making it sometimes search only for short ones, refusing to look
   // deeper when an attempt fails.  The searches are in groups of twenty in order
   // to save time: once we decide to search for some two-call resolves, we re-use
   // the setup left by the same initial call.

   // If a call analysis gets here, this call doesn't work, either as an actual solution,
   // or as something we can build on to make a multi-call solution.

 cant_consider_this_call:

   try {
      // Throw away garbage from last attempt.
      release_parse_blocks_to_mark(inner_parse_mark);
      testing_fidelity = false;
      config_history_ptr = history_save;
      random_top_level_start = random_count;
      random_number = (int) rand();
      srand(random_number);
      random_recent_history[(random_count++) & 127] = random_number*1000;
      attempt_count++;

      // Check whether we have been trying too long.  If so, give up and report failure.
      // The user can try again by giving the "find another" command.  We use the actual
      // clock for this test, and give up after 5 seconds.  But we only do the test
      // every 4096 tries.  Part of the reason is so that we won't waste a lot of time
      // in the "clock" library call.  Another reason is to capture a reasonably
      // accurate value for the clock() call when this happens.  Measurements show that,
      // on a 1.6 GHz processor, it does about 40,000 attempts per second, so it will
      // come up for air about 10 times per second.
      //
      // This also has the nice property that, when debugging, clock expirations won't
      // interfere until 4095 things have been tried.
      //
      // But there is a much more serious problem that needs to be addressed.  On
      // Windows, CLOCKS_PER_SEC is 1000, that is, the clock ticks count milliseconds.
      // But on Linux, CLOCKS_PER_SEC is 1000000, that is, the clock ticks count
      // microseconds.  We want to allow really long tests. like 10 hours.  That's 36
      // billion ticks on Linux.  The returned value of the clock() call is a signed
      // 32-bit integer--it was clearly not intended to measure intervals this long.
      //
      // So we effectively reset the clock manipulation every time we come up for
      // air--10 times per second.  That's still very accurate on Windows, a resolution
      // of 1 percent.

      if (!(attempt_count & 4095)) {
         // Come up for air; see how much clock time has elapsed.  It will be about 100
         // ticks on Windows, and 100,000 ticks on Linux.  Tally that in a
         // doubleprecision floating variable.  36 billion is trivial for such a thing.

         int32_t air_time = clock() - air_start_time;
         air_start_time += air_time; // Reset it for the next time we come up for air.

         // But what if the system clock wrapped around (overflowed) during this time?
         // No problem; the signed 32-bit subtract will do the right thing.  That is,
         // assuming the system clock didn't advance by more than 2 billion during this
         // time.

         big_resolve_time += (double) air_time;

         if (big_resolve_time > CLOCKS_TO_RESOLVE) {
            // Too many tries -- too bad.
            config_history_ptr = huge_history_ptr;

            // We shouldn't have to do the following stuff.  The searcher should be written
            // such that it doesn't get stuck on a call with any iterator nonzero, because,
            // if the iterator is ever set to zero, the current call should continue
            // cycling that iterator until it goes back to zero.  However, if there were
            // a bug in that code, the consequences would be extremely embarrassing.
            // The resolver would just be stuck, repeatedly reporting failure until the
            // entire resolve operation is manually aborted.  To be sure that never happens,
            // we do this.  This could have the effect of prematurely terminating an
            // iteration search, but no one will notice.

            reset_internal_iterators();
            return false;
         }
      }

      // Now clear any concepts if we are not on the first call of the series.

      if (config_history_ptr != history_insertion_point || goal == command_reconcile)
         initialize_parse();
      else
         restore_parse_state();

      // Generate the concepts and call.

      hashed_randoms = hashed_random_list[config_history_ptr - history_insertion_point];

      // Put in a special initial concept if needed to normalize.

      if (goal == command_normalize && !configuration::concepts_in_place()) {
         int k, l;
         useful_concept_enum zzzc;

         for (k=0 ; k < NUM_NICE_START_KINDS ; k++) {
            if (nice_setup_info[k].kind == configuration::current_config().state.kind) {
               l = nice_setup_info[k].number_available_now;
               if (l != 0) goto found_k_and_l;
               else goto cant_consider_this_call;  // This shouldn't happen, because we are screening setups carefully.
            }
         }

         goto cant_consider_this_call;   // This shouldn't happen.

      found_k_and_l:

         zzzc = nice_setup_info[k].array_to_use_now
            [generate_random_number(l)];

         // If the concept is a tandem or as couples type, we really want "phantom"
         // or "2x8 matrix"" in front of it.

         if (concept_descriptor_table[useful_concept_indices[zzzc]].kind == concept_tandem) {
            if (configuration::current_config().state.kind == s4x4)
               deposit_concept(&concept_descriptor_table[useful_concept_indices[UC_phan]]);
            else
               deposit_concept(&concept_descriptor_table[useful_concept_indices[UC_2x8matrix]]);
         }

         deposit_concept(&concept_descriptor_table[useful_concept_indices[zzzc]]);
      }

      // Select the call.  Selecting one that says "don't use in resolve"
      // will signal and go to cant_consider_this_call.
      // This may, of course, add more concepts.

      query_for_call();

      // Do the call.  An error will signal and go to cant_consider_this_call.

      toplevelmove();
      finish_toplevelmove();

      // Check that there are no unfilled parts of the parse tree, as in
      // "detract [ignore everyone line to line but [***]]".
      // We make the check strict, so that all subcalls have been filled in.
      // Since filling in is done lazily, when the subcall is actually
      // executed, things like the above example will never get filled in,
      // and the test will fail.
      check_concept_parse_tree(configuration::next_config().command_root, true);

      // We don't like certain warnings either.
      if (warnings_are_unacceptable(goal != command_standardize)) goto cant_consider_this_call;

      // See if we have already seen this sequence.

      for (i=0; i<avoid_list_size; i++) {
         if (hashed_randoms == avoid_list[i]) goto cant_consider_this_call;
      }

      // The call was legal, see if it satisfies our criterion.

      const setup *ns = &configuration::next_config().state;

      // But if we are doing a resolve test, make everything fail, so that the test
      // will run forever (well, until the specified time limit), just looking for crashes.

      if (ui_options.resolve_test_minutes > 0) goto not_a_solution_but_maybe_can_build_on_it;

      // We used to use an "if" statement here instead of a "switch", because
      // of a compiler bug.  We no longer take pity on buggy compilers.

      switch (goal) {
      case command_resolve:
         {
            resolve_indicator & r = configuration::next_resolve();
            resolve_kind index = r.the_item->k;

            if (index == resolve_none) goto not_a_solution_but_maybe_can_build_on_it;

            // Some resolves are so bad we never search for them.
            // This is indicated by the 40 bit in the distance word.
            if (r.the_item->distance & 0x40) goto not_a_solution_but_maybe_can_build_on_it;

            // Also, the 20 bit says that we don't actively search for this if the promenade distance is zero.
            // The operator might want to accept a "circle left" type of resolve, but would not be very
            // interested in a "circle left zero", because that would actually be a "you're home".  But,
            // from facing lines, they aren't home.  The ends would need to quarter in.  If we find something
            // in which the ends are quartered in, we will accept that whether the circling distance is zero
            // or not.

            if ((r.the_item->distance & 0x20) &&
                (((ns->rotation << 1) +
                  ((ns->people[r.the_item->locations[0]].id1 & 0700) >> 6) +
                  r.the_item->distance) & 7) == 0)
               goto not_a_solution_but_maybe_can_build_on_it;

            // Here we bias the search against resolves with circulates (which we
            // consider to be of lower quality) by only sometimes accepting them.
            //
            // As the "how_bad" indicator gets higher, we ignore a larger
            // fraction of the resolves.  For example, we bias the search VERY HEAVILY
            // against reverse single file promenades, accepting only 1 in 16.

            int badness = resolve_table[index].how_bad;

            if (badness >= 4 && in_exhaustive_search())
               goto not_a_solution_but_maybe_can_build_on_it;

            switch (get_resolve_goodness_info()) {
            case resolve_goodness_only_nice:
               // Only accept things with "how_bad" = 0, that is, RLG, LA, and prom.
               // Furthermore, at C2 and above, only accept if promenade distance
               // is 0 to 3/8.
               if (badness != 0 ||
                   (calling_level >= l_c2 &&
                    ((r.distance & 7) > 3)))
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case resolve_goodness_always:
               // Accept any one-call resolve.
               break;
            case resolve_goodness_maybe:
               // Accept resolves randomly.  The probability that we reject a
               // resolve increases as the resolve quality goes down.
               if (~(~0 << badness) & attempt_count) goto not_a_solution_but_maybe_can_build_on_it;
               break;
            }
         }
         break;

      case command_normalize:

         // We accept any setup with 8 people in it.  This could conceivably give
         // somewhat unusual setups like dogbones or riggers, but they might be
         // sort of interesting if they arise.  (Actually, it is highly unlikely,
         // given the concepts that we use.)
         if (attr::slimit(ns) != 7) goto not_a_solution_but_maybe_can_build_on_it;
         break;

      case command_standardize:
         {
            uint32_t tb = 0;
            uint32_t tbtb = 0;
            uint32_t tbpt = 0;

            for (i=0 ; i<8 ; i++) {
               tb |= ns->people[i].id1;
               tbtb |= ns->people[i].id1 ^ ((i & 2) << 2);
               tbpt |= ns->people[i].id1 ^ (i & 1);
            }

            if (ns->kind == s2x4 || ns->kind == s1x8) {
               if ((tb & 011) == 011) goto not_a_solution_but_maybe_can_build_on_it;
            }
            else if (ns->kind == s_qtag) {
               if ((tb & 1) != 0 && (tbtb & 010) != 0) goto not_a_solution_but_maybe_can_build_on_it;
            }
            else if (ns->kind == s_ptpd) {
               if ((tb & 010) != 0 && (tbpt & 1) != 0) goto not_a_solution_but_maybe_can_build_on_it;
            }
            else
               goto not_a_solution_but_maybe_can_build_on_it;
         }
         break;

      case command_reconcile:
         {
            if (ns->kind != goal_kind) goto not_a_solution_but_maybe_can_build_on_it;

            for (j=0; j<8; j++) {
               if ((ns->people[j].id1 & d_mask) != goal_directions[j]) goto not_a_solution_but_maybe_can_build_on_it; }

            int p0 = ns->people[perm_indices[0]].id1 & PID_MASK;
            int p1 = ns->people[perm_indices[1]].id1 & PID_MASK;
            int p2 = ns->people[perm_indices[2]].id1 & PID_MASK;
            int p3 = ns->people[perm_indices[3]].id1 & PID_MASK;
            int p4 = ns->people[perm_indices[4]].id1 & PID_MASK;
            int p5 = ns->people[perm_indices[5]].id1 & PID_MASK;
            int p6 = ns->people[perm_indices[6]].id1 & PID_MASK;
            int p7 = ns->people[perm_indices[7]].id1 & PID_MASK;

            // Test for absolute sex correctness if required.

            if (!current_reconciler->allow_eighth_rotation && (p0 & 0100)) goto not_a_solution_but_maybe_can_build_on_it;

            p7 = (p7 - p6) & PID_MASK;
            p6 = (p6 - p5) & PID_MASK;
            p5 = (p5 - p4) & PID_MASK;
            p4 = (p4 - p3) & PID_MASK;
            p3 = (p3 - p2) & PID_MASK;
            p2 = (p2 - p1) & PID_MASK;
            p1 = (p1 - p0) & PID_MASK;

            // Test each sex individually for uniformity of offset around the ring.
            if (p1 != p3 || p3 != p5 || p5 != p7 || p2 != p4 || p4 != p6) goto not_a_solution_but_maybe_can_build_on_it;

            // Test for each sex in sequence.
            if (((p1 + p2) & PID_MASK) != 0200) goto not_a_solution_but_maybe_can_build_on_it;

            // Test for alternating sex.
            if ((p2 & 0100) == 0) goto not_a_solution_but_maybe_can_build_on_it;
         }

         break;

      case command_8person_level_call:
         // We demand that no splitting have taken place along either axis.
         if (ns->result_flags.split_info[0] || ns->result_flags.split_info[1])
            goto not_a_solution_but_maybe_can_build_on_it;
         break;

      default:
         if (goal >= command_create_any_lines) {
            directions = 0;
            for (i=0 ; i<8 ; i++) {
               directions <<= 2;
               directions |= ns->people[i].id1 & 3;
            }

            switch (goal) {
            case command_create_any_lines:
               if (ns->kind != s2x4 || (directions & 0x5555) != 0)
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_any_col:
               if (ns->kind != s2x4 || (directions & 0x5555) != 0x5555)
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_any_qtag:
               if (ns->kind != s_qtag || (directions & 0x5555) != 0)
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_any_tidal:
               if (ns->kind != s1x8)
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_waves:
               if (ns->kind != s2x4 || (directions != 0x2288 && directions != 0x8822))
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_2fl:
               if (ns->kind != s2x4 || (directions != 0x0AA0 && directions != 0xA00A))
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_inv_lines:
               if (ns->kind != s2x4 || (directions != 0x2882 && directions != 0x8228))
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_3and1_lines:
               p = (directions ^ (directions >> 6)) & 0x202;
               q = ((directions ^ (directions >> 2)) >> 2) & 0x202;
               if (ns->kind != s2x4 || (directions & 0x5555) != 0 || (p | q) == 0 || p == q)
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_tidal_wave:
               if (ns->kind != s1x8 || (directions != 0x2882 && directions != 0x8228))
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_col:
               if (ns->kind != s2x4 || (directions != 0x55FF && directions != 0xFF55))
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_magic_col:
               if (ns->kind != s2x4 || (directions != 0x7DD7 && directions != 0xD77D))
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_qtag:
               if (ns->kind != s_qtag || (directions & 0xF0F0) != 0xA000 ||
                   ((directions & 0x0F0F) != 0x0802 && (directions & 0x0F0F) != 0x0208))
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_3qtag:
               if (ns->kind != s_qtag || (directions & 0xF0F0) != 0x00A0 ||
                   ((directions & 0x0F0F) != 0x0802 && (directions & 0x0F0F) != 0x0208))
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_qline:
               if (ns->kind != s_qtag || (directions & 0xF0F0) != 0xA000 ||
                   ((directions & 0x0F0F) != 0x0A00 && (directions & 0x0F0F) != 0x000A))
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_3qline:
               if (ns->kind != s_qtag || (directions & 0xF0F0) != 0x00A0 ||
                   ((directions & 0x0F0F) != 0x0A00 && (directions & 0x0F0F) != 0x000A))
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_dmd:
               if (ns->kind != s_qtag || (directions & 0x5555) != 0x5050)
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_li:
               if (ns->kind != s2x4 || directions != 0xAA00)
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_lo:
               if (ns->kind != s2x4 || directions != 0x00AA)
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_dpt:
               if (ns->kind != s2x4 || directions != 0x5FF5)
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_cdpt:
               if (ns->kind != s2x4 || directions != 0xF55F)
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_tby:
               if (ns->kind != s2x4 || directions != 0xDD77)
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            case command_create_8ch:
               if (ns->kind != s2x4 || directions != 0x77DD)
                  goto not_a_solution_but_maybe_can_build_on_it;
               break;
            }
         }
         break;
      }

      // The call (or sequence thereof) seems to satisfy our criterion.  Just to be
      // sure, we have to examine all future calls (for a reconcile -- for other stuff
      // there are no future calls), to make sure that, aside from the permutation
      // that gets performed, they will be executed the same way.

      // But first, we make the dynamic part of the parse state be a copy of what we
      // had, since we are repeatedly overwriting existing blocks.

      // The solution that we have found consists of the parse blocks hanging off of
      // huge_history_ptr+1 ... history_ptr inclusive.  We have to make sure that they will
      // be safe forever.  (That is, until we have exited the entire resolve operation.)
      // For the most part, this follows from the fact that we will not re-use any
      // already-in-use parse blocks.  But the tree hanging off of huge_history_ptr+1
      // gets destructively reset to the initial state by restore_parse_state, so we must
      // protect it.

      configuration::history[huge_history_ptr+1].command_root =
         copy_parse_tree(configuration::history[huge_history_ptr+1].command_root);

      // Save the entire resolve, that is, the calls we inserted, and where we inserted them.

      config_history_ptr++;
      new_resolve->size = config_history_ptr - history_insertion_point;

      if (goal == command_reconcile) {
         for (j=0; j<8; j++)
            new_resolve->permutepersoninfo[perm_array[j] >> 6] = ns->people[perm_indices[j]];

         new_resolve->rotchange = ns->rotation - goal_rotation;
         new_resolve->insertion_point = insertion_depth;
         new_resolve->insertion_width = insertion_width;
      }
      else {
         new_resolve->insertion_point = 0;
         new_resolve->insertion_width = 0;
      }

      // Now test the "fidelity" of the pre-existing calls after the insertion point,
      // to be sure they still behave the way we expect, that is, that they move the
      // permuted people around in the same way.  (If one of those calls uses a predicate
      // like "heads" or "boys" it will likely fail this test until we get around to
      // doing something clever.  Oh well.)

      testing_fidelity = true;

      for (j=0; j<new_resolve->insertion_point; j++) {
         // Copy the whole thing into the history, chiefly to get the call and concepts.
         written_history_items = -1;

         configuration::next_config() = huge_history_save[j+huge_history_ptr+1-new_resolve->insertion_point];

         // Now execute the call again, from the new starting configuration.
         // This might signal and go to cant_consider_this_call.
         toplevelmove();
         finish_toplevelmove();
         configuration this_state = configuration::next_config();
         this_state.state.rotation -= new_resolve->rotchange;
         canonicalize_rotation(&this_state.state);

         if (this_state.state.rotation !=
             huge_history_save[j+huge_history_ptr+1-new_resolve->insertion_point].state.rotation)
            goto cant_consider_this_call;

         if (this_state.warnings_are_different(huge_history_save[j+huge_history_ptr+1-new_resolve->insertion_point]))
            goto cant_consider_this_call;

         for (int k=0; k<=attr::klimit(this_state.state.kind); k++) {
            personrec t = huge_history_save[j+huge_history_ptr+1-new_resolve->insertion_point].state.people[k];

            if (t.id1) {
               const personrec & thispermuteperson = new_resolve->permutepersoninfo[(t.id1 & PID_MASK) >> 6];

               if (this_state.state.people[k].id1 != ((t.id1 & ~PID_MASK) | (thispermuteperson.id1 & PID_MASK)))
                  goto cant_consider_this_call;
               if (this_state.state.people[k].id2 !=
                   ((t.id2 & ~ID2_PERM_ALLBITS) | (thispermuteperson.id2 & ID2_PERM_ALLBITS)))
                  goto cant_consider_this_call;
               if (this_state.state.people[k].id3 != t.id3)
                  goto cant_consider_this_call;
            }
            else {
               if (this_state.state.people[k].id1)
                  goto cant_consider_this_call;
            }
         }

         config_history_ptr++;
      }

      testing_fidelity = false;

      // One more check.  If this was a "reconcile", demand that we
      // have an acceptable resolve.  Specifically, we reject anything
      // with "circulate", "extend", etc.  Why?  Because reconcile endings
      // are supposed to be "clever" RLG's or LA's.  Also, if the ending
      // is a "circle left", we demand that the circling distance be zero.
      // Why?  Because squared-set reconcile endings are supposed to be
      // clever "you're home" types of things.

      if (goal == command_reconcile) {
         resolve_indicator & rr = configuration::current_resolve();
         resolve_kind rk = rr.the_item->k;
         if (resolve_table[rk].not_in_reconcile != 0)
            goto cant_consider_this_call;
         // Fudge the meaning of "at home" if there is a 1/8 rotation.
         if (rk == resolve_circle && rr.distance != (configuration::current_config().state.eighth_rotation ? 7 : 0))
            goto cant_consider_this_call;
      }

      // We win.  Really save it and exit.  History_ptr has been clobbered.

      for (j=0; j<MAX_RESOLVE_SIZE; j++) {
         new_resolve->stuph[j] = configuration::history[j+history_insertion_point+1];
         if (j < new_resolve->size) {
            if (new_resolve->stuph[j].command_root == 0 || new_resolve->stuph[j].command_root->concept == 0) {
               gg77->iob88.serious_error_print("BUG IN RESOLVER!\n");
               goto cant_consider_this_call;   // What????  Some kind of bug, apparently.
            }
         }
      }

      // Grow the "avoid_list" array as needed.

      if (avoid_list_allocation <= avoid_list_size) {
         int new_allocation = avoid_list_size*2+5;
         int *new_list = new int[new_allocation];
         memcpy(new_list, avoid_list, avoid_list_allocation * sizeof(int));
         delete [] avoid_list;
         avoid_list = new_list;
         avoid_list_allocation = new_allocation;
      }

      avoid_list[avoid_list_size++] = hashed_randoms;   // It's now safe to do this.

      return true;

   not_a_solution_but_maybe_can_build_on_it:

      if (!pick_allow_multiple_items()) goto cant_consider_this_call;

      if (++little_count == 60) {
         // Revert back to beginning.
         history_save = history_insertion_point;
         inner_parse_mark = outer_parse_mark;
         little_count = 0;
      }
      else if (little_count == 20 || little_count == 40) {
         // Save current state as a base for future calls.

         // But first, if doing a "normalize" operation, we verify that the setup
         // we have arrived at is one from which we know how to do something.  Otherwise,
         // there is no point in trying to build on the setup at which we have arrived.
         // Also, if the setup has gotten bigger, do not proceed.

         if (goal == command_normalize) {
            int k;

            if (attr::slimit(ns) > attr::klimit(configuration::current_config().state.kind))
               goto cant_consider_this_call;

            for (k=0 ; k < NUM_NICE_START_KINDS ; k++) {
               if (nice_setup_info[k].kind == ns->kind && nice_setup_info[k].number_available_now != 0)
                  goto ok_to_save_this;
            }

            goto cant_consider_this_call;

         ok_to_save_this: ;
         }

         history_save = config_history_ptr + 1;
         inner_parse_mark = get_parse_block_mark();
         hashed_random_list[history_save - history_insertion_point] = hashed_randoms;
      }
   }
   catch(error_flag_type) {
   }

   goto cant_consider_this_call;
}


static bool reconcile_command_ok()
{
   int k;
   int dirmask = 0;
   personrec *current_people = configuration::current_config().state.people;
   setup_kind current_kind = configuration::current_config().state.kind;
   current_reconciler = (reconcile_descriptor *) 0;

   // Since we are going to go back 1 call, demand we have at least 3. *****
   // Also, demand no concepts already in place.
   if ((config_history_ptr < 3) || configuration::concepts_in_place()) return false;

   for (k=0; k<8; k++)
      dirmask = (dirmask << 2) | (current_people[k].id1 & 3);

   switch (current_kind) {
   case s2x4:
      if (dirmask == 0xA00A)
         current_reconciler = &promperm;   // L2FL, looking for promenade.
      else if (dirmask == 0x0AA0)
         current_reconciler = &rpromperm;  // R2FL, looking for reverse promenade.
      else if (dirmask == 0x6BC1)
         current_reconciler = &homeperm;   // pseudo-squared-set, looking for circle left/right.
      else if (dirmask == 0xFF55)
         current_reconciler = &sglperm;    // Lcol, looking for single file promenade.
      else if (dirmask == 0x55FF)
         current_reconciler = &sglperm;    // Rcol, looking for reverse single file promenade.
      else if (dirmask == 0xBC16)
         current_reconciler = &sglperm;    // L Tbone, looking for single file promenade.
      else if (dirmask == 0x16BC)
         current_reconciler = &sglperm;    // R Tbone, looking for reverse single file promenade.
      else if (dirmask == 0x2288)
         current_reconciler = &rlgperm;    // Rwave, looking for RLG.
      else if (dirmask == 0x8822)
         current_reconciler = &laperm;     // Lwave, looking for LA.
      break;
   case s_qtag:
      if (dirmask == 0x08A2)
         current_reconciler = &qtagperm;   // Rqtag, looking for RLG.
      else if (dirmask == 0x78D2)
         current_reconciler = &qtagperm;   // diamonds with points facing, looking for RLG.
      break;
   case s_crosswave: case s_thar:
      if (dirmask == 0x278D)
         current_reconciler = &crossplus;  // crossed waves or thar, looking for RLG, allow slip the clutch.
      else if (dirmask == 0x8D27)
         current_reconciler = &crossplus;  // crossed waves or thar, looking for LA, allow slip the clutch.
      else if (dirmask == 0xAF05)
         current_reconciler = &crossperm;  // crossed waves or thar, looking for promenade.
      break;
   }

   return (current_reconciler != 0);
}

static bool resolve_command_ok()
{
   int n = attr::klimit(configuration::current_config().state.kind);
   return n == 7 || (two_couple_calling && n == 3);
}

static bool nice_setup_command_ok()
{
   int i, k;
   bool setup_ok = false;
   setup_kind current_kind = configuration::current_config().state.kind;

   // Decide which arrays we will use, depending on the current setting of the
   // "allow all concepts" flag, and see if we are in one of the known setups
   // and there are concepts available for that setup.

   for (k=0 ; k < NUM_NICE_START_KINDS ; k++) {
      // Select the correct concept array.
      nice_setup_info[k].array_to_use_now =
         (allowing_all_concepts) ? nice_setup_info[k].thing->zzzfull_list : nice_setup_info[k].thing->zzzon_level_list;

      // Note how many concepts are in it.  If there are zero in some of them,
      // we may still be able to proceed, but we must have concepts available
      // for the current setup.

      for (i=0 ; ; i++) {
         if (nice_setup_info[k].array_to_use_now[i] == UC_none) break;
      }

      nice_setup_info[k].number_available_now = i;

      if (nice_setup_info[k].kind == current_kind && nice_setup_info[k].number_available_now != 0) setup_ok = true;
   }

   return setup_ok || configuration::concepts_in_place();
}


uims_reply_thing ui_utils::full_resolve()
{
   int j, k;
   uims_reply_thing reply(ui_user_cancel, 99);
   int current_resolve_index, max_resolve_index;
   bool show_resolve;
   int current_depth = 0;
   int current_width = 0;
   bool find_another_resolve = true;
   resolver_display_state big_state; // for display to the user.

   // Allocate or reallocate the huge_history_save save array if needed.

   if (huge_history_allocation < config_history_ptr+MAX_RESOLVE_SIZE+2) {
      int new_history_allocation = config_history_ptr+MAX_RESOLVE_SIZE+2;
      // Increase by 50% beyond what we have now.
      new_history_allocation += new_history_allocation >> 1;
      configuration *new_history_save = new configuration[new_history_allocation];
      memcpy(new_history_save, huge_history_save, huge_history_allocation);
      delete [] huge_history_save;
      huge_history_save = new_history_save;
      huge_history_allocation = new_history_allocation;
   }

   // Do the resolve array.

   if (all_resolves == 0) {
      resolve_allocation = 10;
      all_resolves = new resolve_rec[resolve_allocation];
   }

   // Be sure the extra 5 slots in the history array are clean.

   for (j=0; j<MAX_RESOLVE_SIZE; j++) {
      configuration::history[config_history_ptr+j+2].command_root = (parse_block *) 0;
      configuration::history[config_history_ptr+j+2].init_centersp_specific();
   }

   // See if we are in a reasonable position to do the search.

   switch (search_goal) {
      case command_resolve:
         if (!resolve_command_ok())
            specialfail("Not in acceptable setup for resolve.");
         break;
      case command_standardize:
         if (!resolve_command_ok() || two_couple_calling)
            specialfail("Not in acceptable setup for standardize.");
         break;
      case command_reconcile:
         if (!reconcile_command_ok() || two_couple_calling)
            specialfail("Not in acceptable setup for reconcile, or sequence is too short, or concepts are selected.");

         {
            personrec *current_people = configuration::current_config().state.people;

            for (j=0; j<8; j++)
               perm_array[j] = current_people[current_reconciler->perm[j]].id1 & PID_MASK;
         }

         current_depth = 1;
         current_width = 0;
         find_another_resolve = false;       // We initially don't look for resolves;
                                             // we wait for the user to set the depth.
         break;
      case command_normalize:
         if (!nice_setup_command_ok() || two_couple_calling)
            specialfail("Sorry, can't do this: concepts are already selected, or no applicable concepts are available.");
         break;
   }

   for (j=0; j<=config_history_ptr+1; j++)
      huge_history_save[j] = configuration::history[j];

   huge_history_ptr = config_history_ptr;
   save_parse_state();

   restore_parse_state();
   current_resolve_index = 0;
   show_resolve = true;
   max_resolve_index = 0;
   avoid_list_size = 0;

   if (search_goal == command_reconcile) show_resolve = false;

   start_pick();   // This sets interactivity, among other stuff.

   for (;;) {
      // We know the history is restored at this point.
      if (find_another_resolve) {
         // Put up the resolve title showing that we are searching.

         gg77->iob88.update_resolve_menu(search_goal, current_resolve_index, max_resolve_index, resolver_display_searching);

         restore_parse_state();

         if (inner_search(search_goal, &all_resolves[max_resolve_index], current_depth, current_width)) {
            // Search succeeded, save it.
            max_resolve_index++;
            // Make it the current one.
            current_resolve_index = max_resolve_index;

            // Put up the resolve title showing this resolve,
            // but without saying "searching".
            big_state = resolver_display_ok;
         }
         else {
            // Put up a resolve title indicating failure.
            big_state = resolver_display_failed;
         }

         written_history_items = -1;
         config_history_ptr = huge_history_ptr;

         for (j=0; j<=config_history_ptr+1; j++)
            configuration::history[j] = huge_history_save[j];

         find_another_resolve = false;
      }
      else {
         // Just display the sequence with the current resolve inserted.
         // Put up a neutral resolve title.
         big_state = resolver_display_ok;
      }

      // Modify the history to show the current resolve.
      // Note that the current history has been restored to its saved state.

      if ((current_resolve_index != 0) && show_resolve) {
         // Display the current resolve.
         resolve_rec *this_resolve = &all_resolves[current_resolve_index-1];

         // Copy the inserted calls.
         written_history_items = -1;
         for (j=0; j<this_resolve->size; j++)
            configuration::history[j+huge_history_ptr+1-this_resolve->insertion_point-this_resolve->insertion_width] =
               this_resolve->stuph[j];

         // Copy and repair the calls after the insertion.
         for (j=0; j<this_resolve->insertion_point; j++) {
            configuration *this_state =
               &configuration::history[j+huge_history_ptr+1-
                                      this_resolve->insertion_point-this_resolve->insertion_width+this_resolve->size];
            *this_state = huge_history_save[j+huge_history_ptr+1-this_resolve->insertion_point];
            this_state->state.rotation += this_resolve->rotchange;
            canonicalize_rotation(&this_state->state);

            // Repair this setup by permuting all the people.

            for (k=0; k<=attr::klimit(this_state->state.kind); k++) {
               personrec & t = this_state->state.people[k];

               if (t.id1) {
                  const personrec & thispermuteperson = this_resolve->permutepersoninfo[(t.id1 & PID_MASK) >> 6];
                  t.id2 = (t.id2 & ~ID2_PERM_ALLBITS) | (thispermuteperson.id2 & ID2_PERM_ALLBITS);
                  t.id1 = (t.id1 & ~PID_MASK) | (thispermuteperson.id1 & PID_MASK);
               }
            }

            this_state->calculate_resolve();
         }

         config_history_ptr = huge_history_ptr + this_resolve->size - this_resolve->insertion_width;

         // Show the history up to the start of the resolve, forcing a picture on the last item (unless reconciling).

         display_initial_history(huge_history_ptr-this_resolve->insertion_point-this_resolve->insertion_width,
                                 search_goal != command_reconcile);

         // If doing a reconcile, show the begin mark.
         if (search_goal == command_reconcile) {
            writestuff("------------------------------------");
            newline();
         }

         // Show the resolve itself, without its last item.

         for (j=huge_history_ptr-this_resolve->insertion_point-this_resolve->insertion_width+1;
              j<config_history_ptr-this_resolve->insertion_point;
              j++)
            write_history_line(j, false, false, file_write_no);

         // Show the last item of the resolve, with a forced picture.
         write_history_line(config_history_ptr-this_resolve->insertion_point,
                            search_goal != command_reconcile,
                            false,
                            file_write_no);

         // If doing a reconcile, show the end mark.
         if (search_goal == command_reconcile) {
            writestuff("------------------------------------");
            newline();
         }

         // Show whatever comes after the resolve.
         for (j=config_history_ptr-this_resolve->insertion_point+1; j<=config_history_ptr; j++)
            write_history_line(j, j==config_history_ptr-this_resolve->insertion_point,
                               false, file_write_no);
      }
      else if (show_resolve) {
         // We don't have any resolve to show.  Just draw the usual picture.
         display_initial_history(huge_history_ptr, 2);
      }
      else {
         // Don't show any resolve, because we want to display the current insertion point.
         display_initial_history(huge_history_ptr-current_depth-current_width, 0);
         if (current_depth+current_width > 0) {
            writestuff("------------------------------------");
            newline();

            if (current_width > 0) {
                for (j=huge_history_ptr-current_depth-current_width+1; j<=huge_history_ptr-current_depth; j++)
                  write_history_line(j, false, false, file_write_no);
                writestuff("------------------------------------");
                newline();
            }

            for (j=huge_history_ptr-current_depth+1; j<=huge_history_ptr; j++)
               write_history_line(j, false, false, file_write_no);
         }

         newline();   // Write a blank line.
      }

      if (show_resolve && (configuration::sequence_is_resolved())) {
         newline();
         writestuff("     resolve is:");
         newline();
         write_resolve_text(false);
         newline();
         newline();
      }

      gg77->iob88.update_resolve_menu(search_goal, current_resolve_index, max_resolve_index, big_state);

      show_resolve = true;

      for (;;) {          // We ignore any "undo" or "erase" clicks.
         reply = gg77->iob88.get_resolve_command();
         if (reply.majorpart != ui_command_select ||
             (reply.minorpart != command_undo && reply.minorpart != command_erase))
            break;
      }

      if (reply.majorpart == ui_resolve_select) {
         switch ((resolve_command_kind) reply.minorpart) {
         case resolve_command_find_another:
            // Increase allocation if necessary.
            if (max_resolve_index >= resolve_allocation) {
               int new_allocation = resolve_allocation*2+5;
               resolve_rec *new_list = new resolve_rec[new_allocation];
               memcpy(new_list, all_resolves, resolve_allocation * sizeof(resolve_rec));
               delete [] all_resolves;
               all_resolves = new_list;
               resolve_allocation = new_allocation;
            }

            find_another_resolve = true;             // Will get it next time around.
            break;
         case resolve_command_goto_next:
            if (current_resolve_index < max_resolve_index)
               current_resolve_index++;
            break;
         case resolve_command_goto_previous:
            if (current_resolve_index > 1)
               current_resolve_index--;
            break;
         case resolve_command_raise_rec_point:
            if (current_depth+current_width < huge_history_ptr-2)
               current_depth++;
            show_resolve = false;
            break;
         case resolve_command_lower_rec_point:
            if (current_depth+current_width > 0)
               current_depth--;
            show_resolve = false;
            break;
         case resolve_command_grow_rec_region:
            if (current_depth+current_width < huge_history_ptr-2)
               current_width++;
            show_resolve = false;
            break;
         case resolve_command_shrink_rec_region:
            if (current_depth+current_width > 0 && current_width > 0)
               current_width--;
            show_resolve = false;
            break;
         case resolve_command_abort:
            written_history_items = -1;
            config_history_ptr = huge_history_ptr;

            for (j=0; j<=config_history_ptr+1; j++)
               configuration::history[j] = huge_history_save[j];

            goto getout;
         case resolve_command_write_this:
            reply.majorpart = ui_command_select;
            reply.minorpart = command_getout;
            goto getout;
         default:
            // Clicked on "accept choice", or something not on this submenu.
            goto getout;
         }
      }
      else if ((reply.majorpart == ui_command_select) && (reply.minorpart == command_refresh)) {
         // Fall through to redisplay.
         ;
      }
      else {
         // Clicked on "accept choice", or something not on this submenu.
         goto getout;
      }

      // Restore history for next cycle.
      written_history_items = -1;
      config_history_ptr = huge_history_ptr;

      for (j=0; j<=config_history_ptr+1; j++)
         configuration::history[j] = huge_history_save[j];
   }

   getout:

   interactivity = interactivity_normal;
   end_pick();
   return reply;
}



// Create a string representing the search state.  Search_goal indicates which user command
// is being performed.  If there is no current solution,
// then M and N are both 0.  If there is a current
// solution, the M is the solution index (minimum value 1) and N is the maximum
// solution index (N>0).  State indicates whether a search is in progress or not, and
// if not, whether the most recent search failed.

void create_resolve_menu_title(
   command_kind goal,
   int cur, int max,
   resolver_display_state state,
   char *title)
{
   char junk[MAX_TEXT_LINE_LENGTH];
   char *titleptr = title;
   if (goal > command_create_any_lines) goal = command_create_any_lines;

   string_copy(&titleptr, title_string[goal-command_resolve]);

   if (max > 0) {
      sprintf(junk, "%d out of %d", cur, max);
      string_copy(&titleptr, junk);
   }
   switch (state) {
      case resolver_display_ok:
         break;
      case resolver_display_searching:
         if (max > 0) string_copy(&titleptr, " ");
         string_copy(&titleptr, "searching ...");
         break;
      case resolver_display_failed:
         if (max > 0) string_copy(&titleptr, " ");
         string_copy(&titleptr, "failed");
         break;
   }
}


void initialize_getout_tables()
{
   int i, j, k;

   for (k=0 ; k < NUM_NICE_START_KINDS ; k++) {
      nice_setup_thing *nice = nice_setup_info[k].thing;

      // Create the "on level" lists if not already created.
      // Since we re-use some stuff (e.g. 1x10 and 1x12 both use
      // the 1x12 things), it might not be necessary.

      if (!nice->zzzon_level_list) {
         nice->zzzon_level_list = (useful_concept_enum *) ::operator new(nice->full_list_size);

         // Copy those concepts that are on the level.
         for (i=0,j=0 ; ; i++) {
            if (nice->zzzfull_list[i] == UC_none) break;

            if (concept_descriptor_table[useful_concept_indices[nice->zzzfull_list[i]]].level <=
                   calling_level)
               nice->zzzon_level_list[j++] = nice->zzzfull_list[i];
         }

         // Put in the end mark.
         nice->zzzon_level_list[j] = UC_none;
      }
   }
}
