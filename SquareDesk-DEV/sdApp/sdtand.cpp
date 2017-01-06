// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

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
//
//    This is for version 38.

// This file contains stuff for tandem and as-couples moves.

/* This defines the following functions:
   initialize_tandem_tables
   tandem_couples_move
   mimic_move
*/

#include <string.h>
#include "sd.h"



struct tm_thing {
   veryshort maps[32];

   // Laterally grouped people in inside numbering.
   // These are pairs; only low bit of each pair is used (except triangles.)

   // These are triads; usually only the middle bit of each triad is used.
   // (That is, the other bits are zero.)
   // For triangles the highest bit is also used.  For "1/8 twosome" things,
   // the lowest bit is used.
   uint32 ilatmask3high;
   uint32 ilatmask3low;

   uint32 olatmask;
   int limit;
   int rot;

   setup_kind insetup;
   setup_kind outsetup;

   // Things below here are not specified in the tables, but are filled in
   // by "initialize_one_table".

   // Relative to insetup numbering, those people that are NOT paired.
   // These are triads; only low bit of each triad is used; others are zero.
   uint32 insinglemaskhigh;
   uint32 insinglemasklow;

   // Relative to outsetup numbering, those people that are NOT paired.
   uint32 outsinglemask;

   uint32 outunusedmask;
   bool map_is_eighth_twosome;
};


class tandrec {
public:

   tandrec(bool phantom_pairing_ok, bool no_unit_symmetry, bool melded) :
      m_phantom_pairing_ok(phantom_pairing_ok),
      m_no_unit_symmetry(no_unit_symmetry),
      m_melded(melded)
   {}

   bool pack_us(personrec *s,
                const tm_thing *map_ptr,
                int fraction,    // This is in eighths.
                bool fraction_in_use,
                bool dynamic_in_use,
                int key) THROW_DECL;

   void unpack_us(const tm_thing *map_ptr,
                  uint32 orbitmask3high,
                  uint32 orbitmask3low,
                  setup *result) THROW_DECL;


   setup m_real_saved_people[8];
   int m_saved_rotations[MAX_PEOPLE];
   setup m_virtual_setup[2];       // If melded, use both.  Otherwise only m_virtual_setup[0].
   setup virtual_result;
   int m_vertical_people[MAX_PEOPLE];    // 1 if original people were near/far; 0 if lateral.
   uint32 single_mask;
   bool m_phantom_pairing_ok;
   bool m_no_unit_symmetry;
   bool m_melded;
   int m_people_per_group;
};



static tm_thing maps_isearch_twosome[] = {
   // PUT MAPS HERE
   //       maps                                                      ilatmaskH/L   olatmask  limit rot insetup outsetup
   // "2x4_4" - see below; this must be before the others.
   {{7, 6, 4, 5,                     0, 1, 3, 2},                          0,0,     0000,         4, 0,  s1x4,  s2x4},
   {{0, 2, 5, 7,                     1, 3, 4, 6},                      0,02222,     0xFF,         4, 0,  s2x2,  s2x4},
   {{2, 5, 7, 0,                     3, 4, 6, 1},                          0,0,     0xFF,         4, 1,  s2x2,  s2x4},

   {{0, 2, 4, 7, 10, 12, 14, 15, 17,     1, 3, 5, 6, 9, 11, 13, 16, 8}, 0,0222222222, 0x3FFFF,    9, 0,  s3x3,  s3x6},
   {{4, 7, 10, 12, 14, 15, 0, 2, 17,     5, 6, 9, 11, 13, 16, 1, 3, 8},      0,0,     0x3FFFF,    9, 1,  s3x3,  s3x6},

   // This one gets h1p, far box star thru, as couples touch
   {{7, 6, 2, 5,                     0, 1, 3, 4},                      0,00022,     0x3C,         4, 1, s_trngl4, s2x4},
   // This one gets h1p, near box star thru, as couples touch
   {{4, 5, 7, 0,                     3, 2, 6, 1},                      0,00022,     0xC3,         4, 3, s_trngl4, s2x4},

   // This one comes back from far box
   {{0, 2, 4, 6,                     1, 3, 5, 7},                         0,02200,  0xF0,         4, 0, s_trngl4, s_trngl8},

   // Special maps for couples 1/8 twosome stuff.
   // These need further work in the table initializer.
   {{12, 3, 7, 8,                    15, 0, 4, 11},                    0,01313,   0xFFFF,         4, 0,  s2x2,  s4x4},
   {{10, 14, 5, 9,                   13, 1, 2, 6},                     0,03131,   0xFFFF,         4, 0,  s2x2,  s4x4},
   {{12, 14, 7, 9,                   15, 1, 4, 6},                     0,03333,   0xFFFF,         4, 0,  s2x2,  s4x4},
   {{10, 3, 5, 8,                    13, 0, 2, 11},                    0,01111,   0xFFFF,         4, 0,  s2x2,  s4x4},
   {{15, 13, 9, 11,                  1, 3, 7, 5},                      0,01111,   0xFFFF,         4, 0,  s1x4,  s2x8},
   {{0, 2, 6, 4,                     14, 12, 8, 10},                   0,03333,   0xFFFF,         4, 0,  s1x4,  s2x8},
   {{15, 2, 9, 4,                    1, 12, 7, 10},                    0,03131,   0xFFFF,         4, 0,  s1x4,  s2x8},
   {{0, 13, 6, 11,                   14, 3, 8, 5},                     0,01313,   0xFFFF,         4, 0,  s1x4,  s2x8},
   {{0, 2,                           6, 4},                            0,033,       0xFF,         2, 0,  s1x2,  s2x4},
   {{7, 5,                           1, 3},                            0,011,       0xFF,         2, 0,  s1x2,  s2x4},
   {{7, 2,                           1, 4},                            0,031,       0xFF,         2, 0,  s1x2,  s2x4},
   {{0, 5,                           6, 3},                            0,013,       0xFF,         2, 0,  s1x2,  s2x4},


   {{3, 2,                           0, 1},                                0,0,     0000,         2, 0,  s1x2,  s2x2},
   {{0, 3,                           1, 2},                                0,0,      0xF,         2, 1,  s1x2,  s2x2},
   {{0, 3,                           1, 2},                              0,022,      0xF,         2, 0,  s1x2,  s1x4},
   {{0,                              1},                                  0,02,     0003,         1, 0,  s1x1,  s1x2},
   {{0,                              1},                                   0,0,     0003,         1, 1,  s1x1,  s1x2},
   {{0, 3, 5, 6,                     1, 2, 4, 7},                      0,02222,     0xFF,         4, 0,  s1x4,  s1x8},
   {{0, 2, 4, 7, 9, 11,              1, 3, 5, 6, 8, 10},             0,0222222,    0xFFF,         6, 0,  s1x6,  s1x12},
   {{0, 2, 6, 4, 9, 11, 15, 13,      1, 3, 7, 5, 8, 10, 14, 12},     0,022222222,  0xFFFF,        8, 0,  s1x8,  s1x16},
   {{15, 14, 12, 13, 8, 9, 11, 10,   0, 1, 3, 2, 7, 6, 4, 5},              0,0,     0000,         8, 0,  s1x8,  s2x8},
   {{11, 10, 9, 6, 7, 8,             0, 1, 2, 5, 4, 3},                    0,0,     0000,         6, 0,  s1x6,  s2x6},

   //       maps                                                      ilatmaskH/L   olatmask  limit rot insetup outsetup
   // PUT MAPS HERE

   // near line
   {{6, 11, 10, 3, 5, 7, 15, 1},                                       0,02222,   0x8CEA,         4, 0,s_trngl4,s4x4},
   {{12, 14, 0, 7, 9, 11, 2, 5},                                       0,02222,   0x5AA5,         4, 0,s_trngl4,s_c1phan},
   {{10, 3, 5, 6, 15, 1, 7, 11},                                       0,00022,   0x840A,         4, 0,  s2x2,  s4x4},
   {{0, 7, 9, 12, 2, 5, 11, 14},                                       0,00022,   0x00A5,         4, 0,  s2x2,  s_c1phan},
   // line on caller's left
   {{9, 11, 3, 5, 10, 15, 14, 7},                                      0,02222,   0x3157,         4, 1,s_trngl4,s4x4},
   {{13, 15, 6, 9, 0, 2, 4, 11},                                       0,02222,   0x55AA,         4, 1,s_trngl4,s_c1phan},
   {{10, 3, 5, 9, 15, 14, 7, 11},                                      0,02002,   0x8E00,         4, 0,  s2x2,  s4x4},
   {{0, 6, 9, 13, 2, 4, 11, 15},                                       0,02002,   0xA005,         4, 0,  s2x2,  s_c1phan},
   // far line
   {{13, 15, 7, 9, 14, 3, 2, 11},                                      0,02222,   0xEA8C,         4, 2,s_trngl4,s4x4},
   {{1, 3, 10, 13, 4, 6, 8, 15},                                       0,02222,   0xA55A,         4, 2,s_trngl4,s_c1phan},
   {{15, 3, 7, 9, 13, 14, 2, 11},                                      0,02200,   0x0A84,         4, 0,  s2x2,  s4x4},
   {{3, 6, 10, 13, 1, 4, 8, 15},                                       0,02200,   0xA500,         4, 0,  s2x2,  s_c1phan},
   // line on caller's right
   {{2, 7, 6, 15, 1, 3, 11, 13},                                       0,02222,   0x5731,         4, 3,s_trngl4,s4x4},
   {{8, 10, 12, 3, 5, 7, 14, 1},                                       0,02222,   0xAA55,         4, 3,s_trngl4,s_c1phan},
   {{15, 3, 7, 6, 13, 1, 2, 11},                                       0,00220,   0x008E,         4, 0,  s2x2,  s4x4},
   {{3, 7, 10, 12, 1, 5, 8, 14},                                       0,00220,   0x05A0,         4, 0,  s2x2,  s_c1phan},

   // Getouts for the above.
   {{5, 4, 1, 3, 6, 7, 0, 2},                                          0,02200,   0x000F,         4, 2,  s1x4,  slinebox},
   {{0, 2, 6, 7, 1, 3, 5, 4},                                          0,00022,   0x000F,         4, 0,  s1x4,  slinebox},

   {{10, 15, 3, 1, 4, 5, 6, 8,       12, 13, 14, 0, 2, 7, 11, 9},          0,0,     0000,         8, 0,  s2x4,  s4x4},
   {{14, 3, 7, 5, 8, 9, 10, 12,      0, 1, 2, 4, 6, 11, 15, 13},           0,0,   0xFFFF,         8, 1,  s2x4,  s4x4},
   {{-2, 15, 3, 1, -2, 5, 11, 9,     -1, 13, -1, -1, -1, 7, -1, -1},       0,0,     0000,         8, 0,  s2x4,  s4x4},
   {{10, 15, 3, -2, 2, 7, 6, -2,     -1, -1, 14, -1, -1, -1, 11, -1},      0,0,     0000,         8, 0,  s2x4,  s4x4},

   // When analyzing, we prefer the 4x6->3x4 formulation.  But we can synthesize
   // from a qtag.
   {{4, 7, 22, 13, 15, 20, 17, 18, 11, 0, 2, 9,
     5, 6, 23, 12, 14, 21, 16, 19, 10, 1, 3, 8},                           0,0, 0xFFFFFF,        12, 1,  s3x4,  s4x6},
   {{7, 22, 15, 20, 18, 11, 2, 9,    6, 23, 14, 21, 19, 10, 3, 8},         0,0, 0xFCCFCC,         8, 1,  s_qtag,s4x6},

   {{11, 10, 9, 8, 7, 6, 12, 13, 14, 15, 16, 17,
                      0, 1, 2, 3, 4, 5, 23, 22, 21, 20, 19, 18},           0,0,     0000,        12, 0,  s2x6,  s4x6},
   {{0, 2, 4, 6, 8, 10, 13, 15, 17, 19, 21, 23,
                      1, 3, 5, 7, 9, 11, 12, 14, 16, 18, 20, 22}, 04,022222222222, 0xFFFFFF,     12, 0,  s2x6,  s2x12},

   // This is for everyone as couples in a 3x4, making virtual columns of 6.
   {{2, 5, 7, 9, 10, 0,              3, 4, 6, 8, 11, 1},                   0,0,   0x0FFF,         6, 1,  s2x3,  s3x4},

   // There is an issue involving the order of the two pairs of items that follow.
   // In the order shown, (3x4 matrix stuff before c1phan), the program will opt
   // for a 3x4 if we say (normal columns; centers trail off)
   // centers are as couples, circulate.  In the other order, it would opt
   // for C1 phantoms.  We believe that having them arranged in 3 definite lines,
   // from which, for example, we could have the very center 2 trade, is better.

   // But we are nevertheless going to try it the other way, by swapping the
   // following two pairs with each other.

   // Next two are for various people as couples in a C1 phantom, making virtual columns of 6.
   {{3, 7, 5, 9, 15, 13,             1, -1, -1, 11, -1, -1},               0,0,     0000,         6, 0,  s2x3,  s_c1phan},
   {{0, 2, 6, 8, 10, 12,             -1, -1, 4, -1, -1, 14},               0,0,     0000,         6, 0,  s2x3,  s_c1phan},

   // Next two are for various people as couples in a 3x4 matrix, making virtual columns of 6.
   {{2, 5, 7, 8, 11, 0,              -1, -1, 6, -1, -1, 1},                0,0,   0x00C3,         6, 1,  s2x3,  s3x4},
   {{2, 5, 7, 9, 11, 1,              3, -1, -1, 8, -1, -1},                0,0,   0x030C,         6, 1,  s2x3,  s3x4},

   {{0, 2, 4, 6, 9, 11, 13, 15,      1, 3, 5, 7, 8, 10, 12, 14},   0,022222222,   0xFFFF,         8, 0,  s2x4,  s2x8},
   {{0, 2, 4, 6, 9, 11, 13, 15, 17, 19, 20, 22,
                      1, 3, 5, 7, 8, 10, 12, 14, 16, 18, 21, 23},  04,022222222222, 0xFFFFFF,    12, 0,  s3x4,  s3x8},
   {{2, 3, 5, 6, 7, 0,               -1, -1, 4, -1, -1, 1},                0,0,     0x33,         6, 1,  s_2x1dmd, s_crosswave},
   {{0, 1, 3, 4, 5, 6,               -1, -1, 2, -1, -1, 7},                0,0,        0,         6, 0,  s_1x2dmd, s_crosswave},
   {{6, 7, 0, 2, 3, 5,               -1, -1, 1, -1, -1, 4},          0,0200200,     0x33,         6, 0,  s_1x2dmd, s_rigger},
   {{0, 1, 3, 4, 6, 7,               -1, 2, -1, -1, 5, -1},          0,0020020,     0x66,         6, 0,  s_1x2dmd, s1x3dmd},
   {{0, 3, 2, 5, 7, 6,               1, -1, -1, 4, -1, -1},                0,0,     0x33,         6, 1,  s_2x1dmd, s_hrglass},
   {{0, 2, 3, 5, 6, 7,               1, -1, -1, 4, -1, -1},          0,0002002,     0x33,         6, 0,  s_2x1dmd, s3x1dmd},
   {{0, 1, 3, 4, 6, 7,               -1, 2, -1, -1, 5, -1},          0,0020020,     0x66,         6, 0,  s_2x1dmd, s3x1dmd},
   {{6, 7, 0, 2, 3, 5,               -1, -1, 1, -1, -1, 4},          0,0200200,     0x33,         6, 0,  s_2x1dmd, s_qtag},
   // Next one is for centers in tandem in lines, making a virtual bone6.
   {{0, 3, 5, 4, 7, 6,               -1, -1, 2, -1, -1, 1},                0,0,     0000,         6, 0,  s_bone6, s2x4},
   // Missing left half.
   {{-2, 3, 5, 4, -2, -2,            -2, -1, 2, -1, -2, -2},               0,0,     0000,         6, 0,  s_bone6, s2x4},
   // Missing right half.
   {{0, -2, -2, -2, 7, 6,            -1, -2, -2, -2, -1, 1},               0,0,     0000,         6, 0,  s_bone6, s2x4},

   // Must be after "2x4_4".
   {{7, 1, 4, 6,                     0, 2, 3, 5},                      0,02020,     0146,         4, 0,  sdmd,  s2x4},
   // Next one is for so-and-so in tandem in a bone6, making a virtual line of 4.
   {{4, 5, 3, 2,                     0, -1, 1, -1},                        0,0,     0000,         4, 0,  s1x4,  s_bone6},
   // Next one is for so-and-so in tandem in a short6, making a virtual line of 4.
   {{1, 0, 4, 5,                     -1, 2, -1, 3},                        0,0,     0055,         4, 1,  s1x4,  s_short6},
   {{1, 3, 4, 5,                     -1, 2, -1, 0},                    0,02020,     0000,         4, 1,  sdmd,  s_short6},
   {{5, 1, 3, 4,                     0, -1, 2, -1},                        0,0,     0000,         4, 0,  sdmd,  s2x3},

   // Next three are for so-and-so as couples in a line of 8, making a virtual line of 6.
   {{0, 1, 3, 4, 5, 6,               -1, -1, 2, -1, -1, 7},          0,0200200,     0xCC,         6, 0,  s1x6,  s1x8},
   {{0, 1, 2, 4, 7, 6,               -1, 3, -1, -1, 5, -1},          0,0020020,     0xAA,         6, 0,  s1x6,  s1x8},
   {{0, 3, 2, 5, 7, 6,               1, -1, -1, 4, -1, -1},          0,0002002,     0x33,         6, 0,  s1x6,  s1x8},
   // Next two are for so-and-so as couples in a line of 6, making a virtual line of 4.
   {{0, 1, 3, 5,                     -1, 2, -1, 4},                    0,02020,     0066,         4, 0,  s1x4,  s1x6},
   {{0, 2, 4, 5,                     1, -1, 3, -1},                    0,00202,     0033,         4, 0,  s1x4,  s1x6},

   // Next 4 are for so-and-so in tandem from a column of 6, making a virtual column of
   // 4.  The first two are the real maps, and the other two take care of the
   // reorientation that sometimes happens when coming out of a 2x2.

   {{0, 1, 3, 5,                     -1, 2, -1, 4},                    0,02020,     0066,         4, 0,  s2x2,  s2x3},
   {{0, 2, 4, 5,                     1, -1, 3, -1},                    0,00202,     0033,         4, 0,  s2x2,  s2x3},
   {{1, 3, 5, 0,                     2, -1, 4, -1},                        0,0,     0066,         4, 1,  s2x2,  s2x3},
   {{2, 4, 5, 0,                     -1, 3, -1, 1},                        0,0,     0033,         4, 1,  s2x2,  s2x3},
   // Next 2 are for similar situations, in "nonisotropic triangles".
   // We do not have the 3rd or 4th maps in the class, because they apply only
   // to unwinding, and we never unwind to these setups.  That's why
   // these 2 maps are placed after the 4 preceding ones.
   {{0, 1, 3, 5,                     -1, 2, -1, 4},                    0,02020,     0066,         4, 0,  s2x2,  s_ntrgl6cw},
   {{0, 2, 4, 5,                     1, -1, 3, -1},                    0,00202,     0033,         4, 0,  s2x2,  s_ntrgl6ccw},

   // Next 4 are for similar 8-person situations.
   {{1, 3, 4, 6, 7, 0,               2, -1, -1, 5, -1, -1},             0,0,        0x66,         6, 1,  s_short6,  s_nxtrglcw},
   {{2, 3, 5, 6, 7, 0,              -1, -1, 4, -1, -1, 1},              0,0,        0x33,         6, 1,  s_short6,  s_nxtrglccw},
   {{0, 2, 5, 4, 7, 1,              -1,  3, -1, -1,  6, -1},         0,0020020,     0xCC,         6, 0,  s_bone6,  s_nptrglcw},
   {{0, 3, 6, 5, 7, 2,               1, -1, -1,  4, -1, -1},         0,0002002,     0x33,         6, 0,  s_bone6,  s_nptrglccw},

   // Next two are for certain ends in tandem in an H, making a virtual bone6.
   {{10, 3, 5, 6, 9, 11,             0, -1, -1, 4, -1, -1},                0,0,     0000,         6, 0,  s_bone6, s3x4},
   {{0, 4, 5, 6, 9, 11,              -1, 3, -1, -1, 10, -1},               0,0,     0000,         6, 0,  s_bone6, s3x4},
   // Next one is for ends in tandem in lines, making a virtual short6.
   {{2, 4, 5, 6, 7, 1,               -1, 3, -1, -1, 0, -1},          0,0020020,     0000,         6, 1,  s_short6, s2x4},
   // Next two are for certain center column people in tandem in a 1/4 tag, making a virtual short6.
   {{3, 2, 4, 5, 6, 0,               1, -1, -1, 7, -1, -1},          0,0002002,     0000,         6, 1,  s_short6, s_qtag},
   {{1, 2, 4, 5, 6, 7,               -1, -1, 3, -1, -1, 0},          0,0200200,     0000,         6, 1,  s_short6, s_qtag},
   // Next two are for certain center column people in tandem in a spindle, making a virtual short6.
   {{2, 3, 5, 6, 7, 0,               -1, -1, 4, -1, -1, 1},                0,0,     0x33,      6, 1,  s_short6, s_spindle},
   {{1, 3, 4, 6, 7, 0,               2, -1, -1, 5, -1, -1},                0,0,     0x66,      6, 1,  s_short6, s_spindle},
   // Next three are for various people in tandem in columns of 8, making virtual columns of 6.
   {{0, 2, 3, 5, 6, 7,               1, -1, -1, 4, -1, -1},          0,0002002,     0063,         6, 0,  s2x3,  s2x4},
   {{0, 1, 3, 4, 6, 7,               -1, 2, -1, -1, 5, -1},          0,0020020,     0x66,         6, 0,  s2x3,  s2x4},
   {{0, 1, 2, 4, 5, 7,               -1, -1, 3, -1, -1, 6},          0,0200200,     0xCC,         6, 0,  s2x3,  s2x4},
   // Next three are for various people in tandem in a rigger/ptpd/bone, making a virtual line of 6.
   {{6, 7, 5, 2, 3, 4,               -1, -1, 0, -1, -1, 1},                0,0,     0000,         6, 0,  s1x6,  s_rigger},
   {{0, 3, 2, 4, 5, 6,               -1, 1, -1, -1, 7, -1},                0,0,     0000,         6, 0,  s1x6,  s_ptpd},
   {{5, 6, 7, 4, 2, 3,               0, -1, -1, 1, -1, -1},                0,0,     0000,         6, 0,  s1x6,  s_bone},
   {{0, 1, 3, 4, 5, 6,               -1, -1, 2, -1, -1, 7},          0,0200200,     0xCC,         6, 0,  s_bone6,s_bone},
   {{0, 2, 5, 7, 9, 11, 12, 14,      1, 3, 4, 6, 8, 10, 13, 15},   0,022222222,   0xFFFF,         8, 0,  s_qtag,s4dmd},
   {{0, 3, 5, 6,                     1, 2, 4, 7},                          0,0,     0xFF,         4, 1,  sdmd,  s_qtag},
   // This is for various people as couples in a 1/4 tag, making virtual columns of 6.
   // It must be after the map just above.
   {{1, 3, 4, 5, 6, 0,               -1, 2, -1, -1, 7, -1},                0,0,     0xCC,         6, 1,  s2x3,  s_qtag},

   {{0, 7, 2, 4, 5, 6,               -1, 1, -1, -1, 3, -1},                0,0,     0000,         6, 0,  s_2x1dmd, s_galaxy},
   {{2, 1, 4, 6, 7, 0,               -1, 3, -1, -1, 5, -1},                0,0,     0xAA,         6, 1,  s_2x1dmd, s_galaxy},
   {{3, 7, 9, 13,                    1, 5, 11, 15},                    0,02020,   0xA0A0,         4, 0,  s2x2,  s_c1phan},
   {{0, 6, 10, 12,                   2, 4, 8, 14},                     0,00202,   0x0505,         4, 0,  s2x2,  s_c1phan},

   // These 4 are unsymmetrical.
   {{0, 7, 9, 12,                    2, 5, 11, 14},                    0,00022,   0x00A5,         4, 0,  s2x2,  s_c1phan},
   {{3, 6, 10, 13,                   1, 4, 8, 15},                     0,02200,   0xA500,         4, 0,  s2x2,  s_c1phan},
   {{3, 7, 10, 12,                   1, 5, 8, 14},                     0,00220,   0x05A0,         4, 0,  s2x2,  s_c1phan},
   {{0, 6, 9, 13,                    2, 4, 11, 15},                    0,02002,   0xA005,         4, 0,  s2x2,  s_c1phan},

   // These do C1-phantom-like stuff from fudgy 4x4.
   // They must follow the pair just above.
   {{15, 3, 5, 9,                    13, 1, 7, 11},                    0,02020,   0x0A0A,         4, 0,  s2x2,  s4x4},
   {{10, 3, 7, 6,                    15, 14, 2, 11},                   0,00202,   0x8484,         4, 0,  s2x2,  s4x4},

   // These two do C1-phantom-like stuff from fudgy 3x4.
   {{11, 2, 7, 9,                    1, 3, 5, 8},                      0,02020,    01414,         4, 0,  s2x2,  s3x4},
   {{0, 5, 7, 8,                     1, 2, 6, 11},                     0,00202,    00303,         4, 0,  s2x2,  s3x4},

   {{1, 3, 4, 5, 6, 0,               -1, 2, -1, -1, 7, -1},                0,0,     0xCC,      6, 1,  s_short6, s_rigger},
   {{6, 0, 3, 5,                     7, 1, 2, 4},                      0,02222,     0xFF,         4, 0,  sdmd,  s_rigger},
   // Must be after "2x4_4".
   {{6, 5, 3, 4,                     7, 0, 2, 1},                      0,00202,     0xCC,         4, 0,  s1x4,  s_rigger},
   {{5, 6, 4, 3,                     0, 7, 1, 2},                      0,02020,     0xCC,         4, 0,  s1x4,  s_bone},
   {{0, 3, 5, 6,                     1, 2, 4, 7},                      0,00202,     0x33,      4, 0,  sdmd,  s_crosswave},
   {{0, 3, 5, 6,                     1, 2, 4, 7},                      0,00202,     0x33,         4, 0,  s_star, s_thar},

   {{2, 4, 5, 0,                     -1, 3, -1, 1},                        0,0,      033,         4, 1,  sdmd, s_2x1dmd},
   {{0, 2, 4, 5,                     1, -1, 3, -1},                    0,00202,      033,         4, 0,  sdmd, s_1x2dmd},

   {{0, 3, 5, 6, 9, 10, 12, 15,      1, 2, 4, 7, 8, 11, 13, 14},   0,002020202,   0x3333,         8, 0,  s_ptpd,  sdblxwave},

   {{15, 1, 12, 14, 8, 10, 11, 5,    0, 2, 3, 13, 7, 9, 4, 6},     0,020202020,   0x6666,         8, 0,  s_ptpd,  s2x8},

   {{6, 0, 3, 5,                     7, 1, 2, 4},                      0,02020,     0x33,         4, 0,  s_star, s_alamo},

   // Need versions of these with phantoms on each half.
   {{0, 1, 3, 4, 5, 6,               -1, -1, 2, -1, -1, 7},         0,0200200,     0xCC,       6, 0,  s_bone6,  s_bone},
   // Missing left half.
   {{-2, 1, 3, 4, -2, -2,            -2, -1, 2, -1, -2, -2},        0,0000200,     0x0C,       6, 0,  s_bone6,  s_bone},
   // Missing right half.
   {{0, -2, -2, -2, 5, 6,            -1, -2, -2, -2, -1, 7},        0,0200000,     0xC0,       6, 0,  s_bone6,  s_bone},

   {{0, 2, 4, 7, 9, 11,              1, 3, 5, 6, 8, 10},
    0,0222222,      0x0FFF,     6, 0,  s2x3,  s2x6},
   // The three maps just below must be after the map just above.
   {{-2, 7, 6, -2, 12, 15,           -2, 2, 5, -2, 17, 16},
    0,0200200,      0x18060,    6, 0,  s2x3,  s4x5},
   {{9, 7, -2, 18, 12, -2,           8, 2, -2, 19, 17, -2},
    0,0002002,      0xC0300,    6, 0,  s2x3,  s4x5},
   {{3, 6, 8, 10, 11, 1,             4, 5, 7, 9, 0, 2},
    0,0020020,      03636,      6, 1,  s_short6,  s2x6},

   // This must be after the 2x3/2x6 map above.
   {{0, 2, 3, 4, 7, 8, 9, 11,        1, -1, -1, 5, 6, -1, -1, 10},
    0,020022002,     06363,      8, 0,  s2x4, s2x6},

   {{10, 7, 8, 5, 0, 3,              11, 6, 9, 4, 1, 2},
    0,0202202,      00303,      6, 1,  s_short6,  sdeepxwv},

   {{-2, 3, 4, -2, 8, 11,           -2, 2, 5, -2, 9, 10},
    0,0200200,      0xC30,      6, 0,  s2x3,  sbigdmd},
   {{0, 3, -2, 7, 8, -2,           1, 2, -2, 6, 9, -2},
    0,0002002,      0x0C3,      6, 0,  s2x3,  sbigdmd},

   {{2, 0,                           3, 1},                         0,020,        0xC,        2, 1,  s1x2,  s_trngl4},
   {{1, 3,                           0, 2},                         0,02,         0xC,        2, 3,  s1x2,  s_trngl4},
   {{2, 1, 0,                        3, -1, -1},                    0,0,          0xC,        3, 1,  s1x3,  s_trngl4},
   {{0, 1, 3,                        -1, -1, 2},                    0,0,          0xC,        3, 3,  s1x3,  s_trngl4},
   {{0, 3, 2,                        -1, 1, -1},                    0,0,          0,          3, 0,  s1x3,  sdmd},

   {{1, 3, 4, 7, 9, 11,              -1, -1, 5, -1, -1, 10},    0,0200200,      0xC30,        6, 0,  s_ntrgl6cw,  s2x6},
   {{0, 2, 4, 7, 8, 10,              1, -1, -1, 6, -1, -1},     0,0002002,      0x0C3,        6, 0,  s_ntrgl6ccw, s2x6},
   {{1, 3, 5, 7, 10, 11,              -1, 4, -1, -1, 9, -1},    0,0020020,      0x618,        6, 0,  s_ntrgl6cw,  s2x6},
   {{0, 1, 4, 6, 8, 10,              -1, 2, -1, -1, 7, -1},     0,0020020,      0x186,        6, 0,  s_ntrgl6ccw, s2x6},

   // This map must be very late, after the two that do 2x4->4x4
   // and the one that does 2x4->2x8.
   {{3, 5, 14, 8, 9, 12, 7, 2,      1, 4, 15, 10, 11, 13, 6, 0},0,020022002,    0xF0F0,       8, 1,  s2x4,  sdeepbigqtg},

   {{9, 8, 23, 22, 14, 15, 18, 19,       2, 3, 6, 7, 21, 20, 11, 10},   0,0,         0,       8, 0,  s_rigger, s4x6},

   // These must be at the end.  They can partially restore setups
   // through intermediate setups that are too big.

   {{15, 14, 13, 12, 11, 10, 9, 8, 16, 17, 18, 19, 20, 21, 22, 23,
     0, 1, 2, 3, 4, 5, 6, 7, 31, 30, 29, 28, 27, 26, 25, 24},
    0,0,          0,          16, 0,  s2x8,  shyper4x8a},
   {{0, 2, 4, 6, 8, 10, 12, 14, 17, 19, 21, 23, 25, 27, 29, 31,
     1, 3, 5, 7, 9, 11, 13, 15, 16, 18, 20, 22, 24, 26, 28, 30},
    044444,022222222222, 0xFFFFFFFF, 16, 0,  s2x8,  shyper2x16},
   {{6, 9, 30, 11, 17, 19, 21, 28, 23, 24, 15, 26, 0, 2, 4, 13,
     7, 8, 31, 10, 16, 18, 20, 29, 22, 25, 14, 27, 1, 3, 5, 12},
    044444,022222222222, 0xFFFFFFFF, 16, 0,  s4x4,  shyper4x8a},
   {{17, 19, 21, 28, 23, 24, 15, 26, 0, 2, 4, 13, 6, 9, 30, 11,
     16, 18, 20, 29, 22, 25, 14, 27, 1, 3, 5, 12, 7, 8, 31, 10},
    0,0,          0xFFFFFFFF, 16, 1,  s4x4,  shyper4x8a},

   {{0}, 0,0, 0, 0, 0, nothing, nothing}};


static tm_thing maps_isearch_threesome[] = {
   //       maps                                                      ilatmaskH/L    olatmask   limit rot insetup outsetup

   {{0,                    1,                    2},                        0,02,        07,      1, 0,  s1x1,  s1x3},
   {{0,                    1,                    2},                        0,00,        07,      1, 1,  s1x1,  s1x3},
   {{0, 5,                 1, 4,                 2, 3},                    0,022,       077,      2, 0,  s1x2,  s1x6},
   {{0, 5,                 1, 4,                 2, 3},                    0,000,       077,      2, 1,  s1x2,  s2x3},

   {{1, 5,                 -1, 4,                -1, 3},                   0,002,       070,      2, 1,  s1x2,  s2x3},
   {{1, 5,                 -1, 4,                -1, 3},                   0,000,       070,      2, 1,  s1x2,  s2x3},
   {{0, 4,                 1, -1,                2, -1},                   0,020,       007,      2, 1,  s1x2,  s2x3},
   {{0, 4,                 1, -1,                2, -1},                   0,000,       007,      2, 1,  s1x2,  s2x3},

   {{0, 3, 8, 11,     1, 4, 7, 10,     2, 5, 6, 9},                      0,02222,     07777,      4, 0,  s2x2,  s2x6},
   {{3, 8, 11, 0,     4, 7, 10, 1,     5, 6, 9, 2},                          0,0,     07777,      4, 1,  s2x2,  s2x6},

   {{3, 8, 11, 14, 15, 0,   4, 7, 10, 13, 16, 1,   5, 6, 9, 12, 17, 2},      0,0,   0777777,      6, 1,  s2x3,  s3x6},

   // Ones with missing people:
   {{-2, 8, 10, -2, 15, 1,   -2, 7, -1, -2, 16, -1,   -2, 6, -1, -2, 17, -1}, 0,0,  0700700,      6, 1,  s2x3,  s3x6},

   {{4, 8, -2, 13, 15, -2,   -1, 7, -2, -1, 16, -2,   -1, 6, -2, -1, 17, -2}, 0,0,  0700700,      6, 1,  s2x3,  s3x6},

   {{0, 3, 6, 9, 14, 17, 20, 23,          1, 4, 7, 10, 13, 16, 19, 22,
                      2, 5, 8, 11, 12, 15, 18, 21},                  0,022222222, 077777777,      8, 0,  s2x4,  s2x12},
   {{0, 3, 6, 7,           1, -1, 5, -1,         2, -1, 4, -1},          0,00202,      0x77,      4, 0,  sdmd,  s1x3dmd},
   {{3, 6, 7, 0,           -1, 5, -1, 1,         -1, 4, -1, 2},              0,0,      0x77,      4, 1,  sdmd,  s3x1dmd},
   {{7, 0, 3, 6,           -1, 1, -1, 5,         -1, 2, -1, 4},          0,02020,      0x77,      4, 0,  sdmd,  s_spindle},
   {{0, 3, 6, 7,           1, -1, 5, -1,         2, -1, 4, -1},              0,0,      0x77,      4, 1,  sdmd,  s_323},
   {{0, 5, 8, 11,          1, -1, 7, -1,         2, -1, 6, -1},              0,0,      0707,      4, 1,  sdmd,  s3dmd},
   {{0, 5, 8, 9,           1, 4, 7, 10,          2, 3, 6, 11},               0,0,     07777,      4, 1,  sdmd,  s3dmd},
   {{0, 3, 8, 11,          1, 4, 7, 10,          2, 5, 6, 9},            0,02222,     07777,      4, 0,  s1x4,  s1x12},
   {{0, 2, 7, 6,           1, -1, 5, -1,         3, -1, 4, -1},          0,00202,      0xBB,      4, 0,  s1x4,  s1x8},
   {{0, 1, 4, 6,           -1, 3, -1, 7,         -1, 2, -1, 5},          0,02020,      0xEE,      4, 0,  s1x4,  s1x8},
   {{3, 8, 21, 14, 17, 18, 11, 0,         4, 7, 22, 13, 16, 19, 10, 1,
                                          5, 6, 23, 12, 15, 20, 9, 2},       0,0, 0x0FFFFFF,      8, 1,  s2x4,  s4x6},
   {{19, 18, 16, 17, 12, 13, 15, 14,      20, 21, 23, 22, 8, 9, 11, 10,
                                          0, 1, 3, 2, 7, 6, 4, 5},           0,0,      0000,      8, 0,  s1x8,  s3x8},
   {{9, 8, 6, 7,           10, 11, 4, 5,         0, 1, 3, 2},                0,0,      0000,      4, 0,  s1x4,  s3x4},
   {{9, 11, 6, 5,          10, -1, 4, -1,        0, -1, 3, -1},              0,0,      0000,      4, 0,  s1x4,  s3x4},
   {{10, 8, 4, 7,          -1, 11, -1, 5,        -1, 1, -1, 2},              0,0,      0000,      4, 0,  s1x4,  s3x4},
   {{0, 3, 6, 7,           1, -1, 5, -1,         2, -1, 4, -1},          0,00202,      0x77,      4, 0,  s2x2,  s2x4},
   {{0, 1, 4, 7,           -1, 2, -1, 6,         -1, 3, -1, 5},          0,02020,      0xEE,      4, 0,  s2x2,  s2x4},
   {{3, 6, 7, 0,           -1, 5, -1, 1,         -1, 4, -1, 2},              0,0,      0x77,      4, 1,  s2x2,  s2x4},
   {{1, 4, 7, 0,           2, -1, 6, -1,         3, -1, 5, -1},              0,0,      0xEE,      4, 1,  s2x2,  s2x4},
   {{6, 5, 2, 4,           -1, 7, -1, 3,         -1, 0, -1, 1},              0,0,      0000,      4, 0,  s1x4,  s_qtag},
   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};

static tm_thing maps_isearch_mimictwo[] = {
   //       maps                                                      ilatmaskH/L    olatmask   limit rot insetup outsetup
   {{0, 1,             3, 2},                                              0,022,      0xF,       2, 0,  s1x2,  s1x4},
   {{0, 1, 6, 7,       2, 3, 4, 5},                                      0,02222,     0xFF,       4, 0,  s2x2,  s2x4},
   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};

static tm_thing maps_isearch_mimicfour[] = {
   //       maps                                                      ilatmaskH/L    olatmask   limit rot insetup outsetup
   {{0, 1, 2, 3,       6, 7, 4, 5},                                      0,02222,     0xFF,       4, 0,  s1x4,  s1x8},
   {{0, 1, 2, 3, 12, 13, 14, 15,     4, 5, 6, 7, 8, 9, 10, 11},    0,  022222222,   0xFFFF,       8, 0,  s2x4,  s2x8},
   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};

static tm_thing maps_isearch_foursome[] = {
   //       maps                                                      ilatmaskH/L    olatmask   limit rot insetup outsetup
   {{0, 6,             1, 7,             3, 5,             2, 4},          0,022,    0x0FF,       2, 0,  s1x2,  s1x8},
   {{0, 7,             1, 6,             2, 5,             3, 4},            0,0,    0x0FF,       2, 1,  s1x2,  s2x4},
   {{0, 4, 11, 15,     1, 5, 10, 14,     2, 6, 9, 13,      3, 7, 8, 12}, 0,02222,  0x0FFFF,       4, 0,  s2x2,  s2x8},
   {{4, 11, 15, 0,     5, 10, 14, 1,     6, 9, 13, 2,      7, 8, 12, 3},     0,0,  0x0FFFF,       4, 1,  s2x2,  s2x8},
   {{0, 7, 11, 12,     1, 6, 10, 13,     2, 5, 9, 14,      3, 4, 8, 15},     0,0,  0x0FFFF,       4, 1,  sdmd,  s4dmd},


   {{0, 4, 11, 15,     1, 5, 10, 14,     2, 6, 9, 13,      3, 7, 8, 12}, 0,02222,  0x0FFFF,       4, 0,  s1x4,  s1x16},
   {{17, 16, 15, 12, 13, 14,         18, 19, 20, 23, 22, 21,
         11, 10, 9, 6, 7, 8,                 0, 1, 2, 5, 4, 3},              0,0,     0000,       6, 0,  s1x6,  s4x6},
   {{8, 6, 4, 5,       9, 11, 2, 7,      10, 15, 1, 3,     12, 13, 0, 14},   0,0,     0000,       4, 0,  s1x4,  s4x4},
   {{12, 10, 8, 9,     13, 15, 6, 11,    14, 3, 5, 7,      0, 1, 4, 2},      0,0,   0xFFFF,       4, 1,  s1x4,  s4x4},
   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};

static tm_thing maps_isearch_sixsome[] = {
//   maps  ilatmask olatmask    limit rot            insetup outsetup
   {{0, 11,   1, 10,   2, 9,    3, 8,    4, 7,    5, 6},
             0,022,    0x0FFF,       2, 0,  s1x2,  s1x12},
   {{0, 11,   1, 10,   2, 9,    3, 8,    4, 7,    5, 6},
               0,0,    0x0FFF,       2, 1,  s1x2,  s2x6},
   {{0, 6, 17, 23,   1, 7, 16, 22,   2, 8, 15, 21,    3, 9, 14, 20,    4, 10, 13, 19,    5, 11, 12, 18},
            0,02222, 0x0FFFFFF,       4, 0,  s2x2,  s2x12},
   {{0, 11, 17, 18,  1, 10, 16, 19,  2, 9, 15, 20,    3, 8, 14, 21,    4, 7, 13, 22,     5, 6, 12, 23},
               0,0, 0x0FFFFFF,       4, 1,  s1x4,  s4x6},
   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};

static tm_thing maps_isearch_eightsome[] = {
//   maps  ilatmask olatmask    limit rot            insetup outsetup
   {{0, 15,   1, 14,   2, 13,   3, 12,   4, 11,   5, 10,   6, 9,   7, 8},
             0,022,   0x0FFFF,       2, 0,  s1x2,  s1x16},
   {{0, 15,   1, 14,   2, 13,   3, 12,   4, 11,   5, 10,   6, 9,   7, 8},
               0,0,   0x0FFFF,       2, 1,  s1x2,  s2x8},
   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};

static tm_thing maps_isearch_boxsome[] = {

//   map1              map2              map3              map4               ilatmask olatmask    limit rot inset outset
   {{0,                1,                3,                2},                  0,02,      0xF,       1, 0,  s1x1,  s2x2},
   {{3,                0,                2,                1},                   0,0,        0,       1, 0,  s1x1,  s2x2},
   {{0, 2,             1, 3,             7, 5,             6, 4},              0,022,     0xFF,       2, 0,  s1x2,  s2x4},
   {{7, 5,             0, 2,             6, 4,             1, 3},                0,0,        0,       2, 0,  s1x2,  s2x4},
   {{0, 2, 6, 4,       1, 3, 7, 5,       15, 13, 9, 11,    14, 12, 8, 10},    0,02222,  0xFFFF,       4, 0,  s1x4,  s2x8},
   {{15, 13, 9, 11,    0, 2, 6, 4,       14, 12, 8, 10,    1, 3, 7, 5},          0,0,        0,       4, 0,  s1x4,  s2x8},
   {{15, 2, 9, 4,      0, 3, 6, 5,       14, 13, 8, 11,    1, 12, 7, 10},     0,02020,  0x3C3C,       4, 0,  s1x4,  s2x8},
   {{0, 13, 6, 11,     1, 2, 7, 4,       15, 12, 9, 10,    14, 3, 8, 5},      0,00202,  0xC3C3,       4, 0,  s1x4,  s2x8},
   {{12, 14, 7, 9,     13, 0, 2, 11,     10, 3, 5, 8,      15, 1, 4, 6},      0,02222,  0xFFFF,       4, 0,  s2x2,  s4x4},
   {{10, 3, 5, 8,      12, 14, 7, 9,     15, 1, 4, 6,      13, 0, 2, 11},        0,0,        0,       4, 0,  s2x2,  s4x4},


   // Special maps for couples 1/8 twosome stuff.
   // These need further work in the table initializer.
   {{3, 4, 28, 27,        13, 9, 30, 26,   14, 10, 29, 25,   12, 11, 19, 20},      0,03333,    0xFF,  4, 0,  s2x2,  shyper4x8b},
   {{14, 10, 29, 25,      3, 4, 28, 27,    12, 11, 19, 20,   13, 9, 30, 26},       0,01111,    0xFF,  4, 0,  s2x2,  shyper4x8b},
   {{14, 4, 29, 27,       3, 9, 28, 26,    12, 10, 19, 25,   13, 11, 30, 20},      0,03131,    0xFF,  4, 0,  s2x2,  shyper4x8b},
   {{3, 10, 28, 25,       13, 4, 30, 27,   14, 11, 29, 20,   12, 9, 19, 26},       0,01313,    0xFF,  4, 0,  s2x2,  shyper4x8b},
   {{20, 22, 9, 11,       2, 3, 5, 4,      17, 16, 14, 15,   21, 23, 8, 10},       0,01111,    0xFF,  4, 0,  s1x4,  shyper3x8},
   {{2, 3, 5, 4,          21, 23, 8, 10,   20, 22, 9, 11,    17, 16, 14, 15},      0,03333,    0xFF,  4, 0,  s1x4,  shyper3x8},
   {{20, 3, 9, 4,         2, 23, 5, 10,    17, 22, 14, 11,   21, 16, 8, 15},       0,03131,    0xFF,  4, 0,  s1x4,  shyper3x8},
   {{2, 22, 5, 11,        21, 3, 8, 4,     20, 16, 9, 15,    17, 23, 14, 10},      0,01313,    0xFF,  4, 0,  s1x4,  shyper3x8},
   {{6, 3,       0, 1,      5, 4,    7, 2},       0,011,       0xF,  2, 0,  s1x2,  slittlestars},
   {{0, 1,       7, 2,      6, 3,    5, 4},       0,033,       0xF,  2, 0,  s1x2,  slittlestars},
   {{6, 1,       0, 2,      5, 3,    7, 4},       0,031,       0xF,  2, 0,  s1x2,  slittlestars},
   {{0, 3,       7, 1,      6, 4,    5, 2},       0,013,       0xF,  2, 0,  s1x2,  slittlestars},


   {{11, 2, 7, 20,     10, 3, 6, 21,     18, 9, 22, 15,    19, 8, 23, 14},        0,02222,0x0FCCFCC,  4, 0,  sdmd,  s4x6},
   {{18, 9, 22, 15,    11, 2, 7, 20,     19, 8, 23, 14,    10, 3, 6, 21},              0,0,       0,  4, 0,  sdmd,  s4x6},
   {{0, 2, 4, 22, 20, 18,     1, 3, 5, 23, 21, 19,
     11, 9, 7, 13, 15, 17,    10, 8, 6, 12, 14, 16},                             0,0222222,0xFFFFFF,  6, 0,  s2x3,  s4x6},
   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};


static tm_thing maps_isearch_dmdsome[] = {

//   map1              map2              map3              map4               ilatmask olatmask    limit rot            insetup outsetup
   {{0,                1,                3,                2},                   0,02,      0xF,         1, 0,  s1x1,  sdmd},
   {{0,                1,                3,                2},                   0,0,      0xF,         1, 1,  s1x1,  sdmd},
   {{0, 6,             1, 7,             3, 5,             2, 4},              0,022,     0xFF,         2, 0,  s1x2,  s_ptpd},
   {{5, 4,             6, 3,             7, 2,             0, 1},                0,0,        0,         2, 0,  s1x2,  s_qtag},
   {{11, 10, 8, 9,     12, 14, 5, 7,     13, 15, 4, 6,     0, 1, 3, 2},          0,0,        0,         4, 0,  s1x4,  s4dmd},
   {{12, 14, 5, 7,     0, 1, 3, 2,       11, 10, 8, 9,     13, 15, 4, 6},     0,02222,   0xFFFF,         4, 0,  s1x4,  s4ptpd},
   {{-2, 4, -2, 5,     -2, 3, -2, 6,     -2, 2, -2, 7,     -2, 1, -2, 0},     0,02222,        0,         4, 1,  sdmd,  s_qtag},
   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};


static tm_thing maps_isearch_tglsome[] = {

//   map1              map2              map3              map4               ilatmask olatmask    limit rot            insetup outsetup
   {{6, 0, 2, 4,       -1, 7, -1, 3,     -1, 5, -1, 1},                       0,06020,     0xBB,         4, 0,  s1x4,  s_rigger},
   {{1, 2, 5, 6,       0, -1, 4, -1,     3, -1, 7, -1},                       0,00602,     0xBB,         4, 0,  s1x4,  s_ptpd},
   {{0, 3, 4, 7,       -1, 2, -1, 6,     -1, 1, -1, 5},                       0,02060,     0xEE,         4, 0,  s1x4,  s_ptpd},

   {{7, 1, 3, 5,       0, -1, 4, -1,     6, -1, 2, -1},                       0,00400,        0,         4, 0,  s2x2,  s_qtag},
   {{0, 2, 4, 6,       -1, 1, -1, 5,     -1, 3, -1, 7},                       0,04000,        0,         4, 0,  s2x2,  s_qtag},

   {{1, 3, 5, 7,       -1, 4, -1, 0,     -1, 2, -1, 6},                       0,02060,        0,         4, 1,  s2x2,  s_qtag},
   {{2, 4, 6, 0,       1, -1, 5, -1,     3, -1, 7, -1},                       0,00602,        0,         4, 1,  s2x2,  s_qtag},

   {{5, 7, 1, 3,       6, -1, 2, -1,     0, -1, 4, -1},                       0,00206,     0x77,         4, 0,  s1x4,  s_bone},
   {{0, 1, 4, 5,       7, -1, 3, -1,     6, -1, 2, -1},                       0,00602,     0xDD,         4, 0,  sdmd,  s_spindle},
   {{5, 3, 1, 7,       6, -1, 2, -1,     0, -1, 4, -1},                       0,00206,     0x77,         4, 0,  sdmd,  s_dhrglass},
   {{6, 0, 2, 4,       -1, 3, -1, 7,     -1, 1, -1, 5},                       0,00040,        0,         4, 0,  sdmd,  s_hrglass},
   {{0, 3, 4, 7,       6, -1, 2, -1,     5, -1, 1, -1},                       0,00602,     0x77,         4, 0,  sdmd,  s_hrglass},

   // These two ought to show a "fudgy" warning.
   {{6, 1, 2, 5,       0, -1, 4, -1,     7, -1, 3, -1},                       0,00400,        0,         4, 0,  sdmd,  s2x4},
   {{0, 2, 4, 6,       7, -1, 3, -1,     1, -1, 5, -1},                       0,00004,        0,         4, 0,  sdmd,  s2x4},

   {{7, 5, 3, 1,       -1, 0, -1, 4,     -1, 6, -1, 2},                       0,04000,        0,         4, 0,  s1x4,  s_nxtrglcw},
   {{7, 0, 3, 4,       -1, 6, -1, 2,     -1, 1, -1, 5},                       0,00040,        0,         4, 0,  s1x4,  s_nxtrglccw},
   {{6, 1, 2, 5,       0, -1, 4, -1,     7, -1, 3, -1},                       0,00400,        0,         4, 0,  s1x4,  s_nptrglcw},
   {{0, 2, 4, 6,       7, -1, 3, -1,     1, -1, 5, -1},                       0,00004,        0,         4, 0,  s1x4,  s_nptrglccw},

   {{0, 3, 4, 7,       -1, 2, -1, 6,     -1, 1, -1, 5},                       0,04000,        0,         4, 0,  sdmd,  s_galaxy},
   {{2, 5, 6, 1,       -1, 4, -1, 0,     -1, 3, -1, 7},                       0,00040,     0xBB,         4, 1,  sdmd,  s_galaxy},

   {{1, 7, 9, 15,      -1, 11, -1, 3,    -1, 5, -1, 13},                      0,00040,   0x0000,         4, 0,  s2x2,  s_c1phan},
   {{0, 4, 8, 12,      -1, 2, -1, 10,    -1, 6, -1, 14},                      0,06020,   0x5454,         4, 0,  s2x2,  s_c1phan},
   {{3, 5, 11, 13,     7, -1, 15, -1,    1, -1, 9, -1},                       0,00206,   0x8A8A,         4, 0,  s2x2,  s_c1phan},
   {{0, 4, 8, 12,      14, -1, 6, -1,    2, -1, 10, -1},                      0,00004,   0x0000,         4, 0,  s2x2,  s_c1phan},
   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};


static tm_thing maps_isearch_3x1tglsome[] = {
//   map1              map2              map3              map4               ilatmask olatmask    limit rot            insetup outsetup
   {{9, 3,             10, 4,            0, 6,             11, 5},             0,026,    07171,         2, 0,  s1x2,  s3x4},
   {{0, 4,             7, 3,             5, 1,             6, 2},              0,062,     0xFF,         2, 0,  s1x2,  s_qtag},
   {{9, 3,             10, 4,            11, 5,            1, 7},              0,040,     0000,         2, 0,  s1x2,  s2x6},
   {{0, 6,             1, 7,             2, 8,             10, 4},             0,004,     0000,         2, 0,  s1x2,  s2x6},

   {{9, 6, 21, 18,     10, 7, 22, 19,    11, 8, 23, 20,    1, 4, 13, 16},      0,00044,    0000,         4, 0,  s2x2,  s4x6},
   {{0, 3, 12, 15,     1, 4, 13, 16,     2, 5, 14, 17,     10, 7, 22, 19},     0,04400,    0000,         4, 0,  s2x2,  s4x6},

   {{19, 3, 7, 15,     20, 23, 8, 11,    0, 16, 12, 4,     21, 22, 9, 10},     0,02662,   0xF99F99,      4, 0,  s1x4,  s3x8},
   {{1, 17, 13, 5,     21, 22, 9, 10,    18, 2, 6, 14,     20, 23, 8, 11},     0,06226,   0xF66F66,      4, 0,  s1x4,  s3x8},

   {{0, 18, 12, 6,     1, 19, 13, 7,     2, 20, 14, 8,     22, 4, 10, 16},     0,00440,   0000,          4, 0,  s1x4,  s2x12},
   {{21, 3, 9, 15,     22, 4, 10, 16,    23, 5, 11, 17,    1, 19, 13, 7},      0,04004,   0000,          4, 0,  s1x4,  s2x12},

   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};

static tm_thing maps_isearch_ysome[] = {
//   map1              map2              map3              map4               ilatmask olatmask    limit rot            insetup outsetup
   {{5, 1,             0, 4,             6, 2,             7, 3},              0,026,     0xFF,         2, 0,  s1x2,  s_bone},
   {{0, 4,             5, 1,             7, 3,             6, 2},              0,062,     0xFF,         2, 0,  s1x2,  s_rigger},
   {{15, 7,            13, 5,            3, 11,            1, 9},              0,040,     0000,         2, 0,  s1x2,  s_c1phan},
   {{0, 8,             2, 10,            14, 6,            12, 4},             0,004,     0000,         2, 0,  s1x2,  s_c1phan},
   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};

static tm_thing maps_isearch_fudgy2x3[] = {
   {{1, 2},                                                    0,02,        006,          1, 0,  s1x1,  s2x3},
   {{4, 3},                                                    0,00,        030,          1, 1,  s1x1,  s2x3},
   {{5, 4},                                                    0,02,        060,          1, 0,  s1x1,  s2x3},
   {{0, 1},                                                    0,00,        003,          1, 1,  s1x1,  s2x3},
   {{0, 1},                                                    0,02,        003,          1, 0,  s1x1,  s2x3},
   {{1, 2},                                                    0,00,        006,          1, 1,  s1x1,  s2x3},
   {{4, 3},                                                    0,02,        030,          1, 0,  s1x1,  s2x3},
   {{5, 4},                                                    0,00,        060,          1, 1,  s1x1,  s2x3},
   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};

static tm_thing maps_isearch_fudgy2x6[] = {
   {{2, 3, 4, 5},                                              0,02,        00074,          1, 0,  s1x1,  s2x6},
   {{9, 8, 7, 6},                                              0,00,        01700,          1, 1,  s1x1,  s2x6},
   {{11, 10, 9, 8},                                            0,02,        07400,          1, 0,  s1x1,  s2x6},
   {{0, 1, 2, 3},                                              0,00,        00017,          1, 1,  s1x1,  s2x6},
   {{0, 1, 2, 3},                                              0,02,        00017,          1, 0,  s1x1,  s2x6},
   {{2, 3, 4, 5},                                              0,00,        00074,          1, 1,  s1x1,  s2x6},
   {{9, 8, 7, 6},                                              0,02,        01700,          1, 0,  s1x1,  s2x6},
   {{11, 10, 9, 8},                                            0,00,        07400,          1, 1,  s1x1,  s2x6},
   {{0}, 0,0, 0, 0, 0,  nothing, nothing}};



struct siamese_item {
   setup_kind testkind;
   uint32 testval; // High 16 = people facing E/W (little-endian), low 16 = people facing N/S.
   uint32 fixup;   // High bit means phantom pairing is OK.
   warning_index warning;
};

const siamese_item siamese_table_of_2[] = {
   {s2x4,        0x00FF0000U, 0x99U,   warn__ctrstand_endscpls},
   {s2x4,        0x00990066U, 0x99U,   warn__ctrstand_endscpls},
   {s2x4,        0x000000FFU, 0x66U,   warn__ctrscpls_endstand},
   {s2x4,        0x00660099U, 0x66U,   warn__ctrscpls_endstand},
   {s2x4,        0x00F0000FU, 0x0FU,   warn__none},  // unsymm
   {s2x4,        0x000F00F0U, 0xF0U,   warn__none},  // unsymm
   {s1x8,        0x00CC0033U, 0x33U,   warn__ctrstand_endscpls},
   {s1x8,        0x003300CCU, 0xCCU,   warn__ctrscpls_endstand},
   {s2x4,        0x003300CCU, 0xCCU,   warn__none},
   {s2x4,        0x00CC0033U, 0x33U,   warn__none},
   {s2x6,        0x030C0CF3U, 0xCF3U,  warn__none},
   {s2x6,        0x0CF3030CU, 0x30CU,  warn__none},
   {s_trngl4,    0x000F0000U, 0x03U,   warn__none},
   {s_trngl4,    0x0000000FU, 0x0CU,   warn__none},
   {s_c1phan,    0x0000AAAAU, 0xA0A0U, warn__none},
   {s_c1phan,    0x00005555U, 0x0505U, warn__none},
   {s_c1phan,    0xAAAA0000U, 0x0A0AU, warn__none},
   {s_c1phan,    0x55550000U, 0x5050U, warn__none},
   {s_c1phan,    0x00005AA5U, 0x00A5U, warn__none},  // These 8 are unsymmetrical.
   {s_c1phan,    0x0000A55AU, 0xA500U, warn__none},
   {s_c1phan,    0x000055AAU, 0x05A0U, warn__none},
   {s_c1phan,    0x0000AA55U, 0xA005U, warn__none},
   {s_c1phan,    0x5AA50000U, 0x5A00U, warn__none},
   {s_c1phan,    0xA55A0000U, 0x005AU, warn__none},
   {s_c1phan,    0x55AA0000U, 0x500AU, warn__none},
   {s_c1phan,    0xAA550000U, 0x0A50U, warn__none},

   {s4x4,        0x0000AAAAU, 0x0A0AU, warn__none},
   {s4x4,        0x0000CCCCU, 0x8484U, warn__none},
   {s4x4,        0xAAAA0000U, 0xA0A0U, warn__none},
   {s4x4,        0xCCCC0000U, 0x4848U, warn__none},

   {s4x4,        0x00008CEAU, 0x840AU, warn__none},   // near
   {s4x4,        0xCEA80000U, 0x40A8U, warn__none},   // **** need fixup left
   {s4x4,        0x0000EA8CU, 0x0A84U, warn__none},   // far
   {s4x4,        0xA8CE0000U, 0xA840U, warn__none},   // **** need fixup right

   {s4x5,        0x000090E4U, 0x18060U, warn__none},
   {s4x5,        0x00001384U, 0xC0300U, warn__none},
   {s4x5,        0x90E40000U, 0x21084U, warn__none},
   {s4x5,        0x13840000U, 0x21084U, warn__none},

   {sbigdmd,     0x00000F3CU, 0x18060U, warn__none},
   {sbigdmd,     0x000003CFU, 0xC0300U, warn__none},
   {sbigdmd,     0x0F3C0000U, 0x0030CU, warn__none},
   // ************************** THIS IS THE ONE ************************
   {sbigdmd,     0x03CF0000U, 0x0030CU, warn__none},
   // ************************** THIS IS THE ONE ************************

   {s4x4,        0x30304141U, 0x80004141U, warn__none},
   {s4x4,        0x41413030U, 0x80003030U, warn__none},
   {s4x4,        0x03031414U, 0x80000303U, warn__none},
   {s4x4,        0x14140303U, 0x80001414U, warn__none},

   {s3x4,        0x09E70000U, 0x0924U, warn__none},
   {s3x4,        0x000009E7U, 0x00C3U, warn__none},
   {s3x4,        0x0BAE0000U, 0x08A2U, warn__none},
   {s3x4,        0x00000BAEU, 0x030CU, warn__none},

   {s3x4,        0x0C30030CU, 0x030CU, warn__none},
   {s3x4,        0x030C0C30U, 0x0C30U, warn__none},
   {s3x4,        0x0C3000C3U, 0x00C3U, warn__none},
   {s3x4,        0x00C30C30U, 0x0C30U, warn__none},

   {s_qtag,      0x003300CCU, 0xCCU,   warn__ctrscpls_endstand},
   {s_qtag,      0x00CC0033U, 0x33U,   warn__ctrstand_endscpls},
   {s4dmd,       0x0F0FF0F0U, 0xF0F0U, warn__ctrscpls_endstand},
   {s4dmd,       0xF0F00F0FU, 0x0F0FU, warn__ctrstand_endscpls},
   {s_rigger,    0x00FF0000U, 0x33U,   warn__ctrscpls_endstand},
   {s_rigger,    0x00CC0033U, 0x33U,   warn__ctrscpls_endstand},
   {s_rigger,    0x000000FFU, 0xCCU,   warn__ctrstand_endscpls},
   {s_rigger,    0x003300CCU, 0xCCU,   warn__ctrstand_endscpls},
   {s_bone,      0x00FF0000U, 0x33U,   warn__ctrstand_endscpls},
   {s_bone,      0x000000FFU, 0xCCU,   warn__ctrscpls_endstand},
   {s_crosswave, 0x00FF0000U, 0xCCU,   warn__ctrscpls_endstand},
   {s_crosswave, 0x000000FFU, 0x33U,   warn__ctrstand_endscpls},
   {sdeepbigqtg, 0xFFFF0000U, 0x0F0FU, warn__none},
   {sdeepbigqtg, 0x0000FFFFU, 0xF0F0U, warn__none},
   {sdeepbigqtg, 0x3535CACAU, 0xC5C5U, warn__none},
   {sdeepbigqtg, 0xCACA3535U, 0x3A3AU, warn__none},
   {sdeepbigqtg, 0xC5C53A3AU, 0x3535U, warn__none},
   {sdeepbigqtg, 0x3A3AC5C5U, 0xCACAU, warn__none},
   {nothing,     0,            0,        warn__none}};

const siamese_item siamese_table_of_3[] = {
   {s2x6,        0x01C70E38U, 0xE38U,  warn__none},
   {s2x6,        0x0E3801C7U, 0x1C7U,  warn__none},
   {nothing,     0,            0,        warn__none}};

const siamese_item siamese_table_of_4[] = {
   {s2x8,        0x0F0FF0F0U, 0xF0F0U, warn__none},
   {s2x8,        0xF0F00F0FU, 0x0F0FU, warn__none},
   {nothing,     0,            0,        warn__none}};

static void initialize_one_table(tm_thing *map_start, int m_people_per_group)
{
   tm_thing *map_search;

   for (map_search = map_start; map_search->outsetup != nothing; map_search++) {
      int i, j;
      map_search->insinglemaskhigh = 0;
      map_search->insinglemasklow = 0;
      map_search->outsinglemask = 0;
      map_search->map_is_eighth_twosome = false;

      uint32 osidemask = 0;
      // All 1's for people in outer setup.
      int outsize = attr::klimit(map_search->outsetup)+1;
      // Yes, we could handle this better on a 64-bit system.
      map_search->outunusedmask = outsize >= 32 ? 0xFFFFFFFF : ((1U << outsize)-1);

      uint32 very_special = ((setup_attrs[map_search->outsetup].four_way_symmetry) &&
                             (setup_attrs[map_search->insetup].no_symmetry) &&
                             (map_search->rot & 1) != 0) ? ((1U << outsize)-1) : 0;

      for (i=0; i<map_search->limit; i++) {
         if (map_search->maps[i] == -2) continue;

         map_search->outunusedmask &= ~(1 << map_search->maps[i]);

         if (map_search->maps[i+map_search->limit] < 0) {
            int bitpos = i*3;
            // Obviously, we could handle this a *LOT* better on a 64-bit system.
            if (bitpos >= 32)
               map_search->insinglemaskhigh |= 1U << (bitpos-32);
            else
               map_search->insinglemasklow |= 1U << bitpos;

            map_search->outsinglemask |= 1 << map_search->maps[i];
         }
         else {
            for (j=1 ; j<m_people_per_group ; j++)
               map_search->outunusedmask &= ~(1 << map_search->maps[i+(map_search->limit*j)]);

            // ******** here is where we check for twosome map.

            int lowbitpos = i*3;
            uint32 ilatlowbit = (lowbitpos >= 32) ?
               map_search->ilatmask3high >> (lowbitpos-32) :
               map_search->ilatmask3low >> lowbitpos;

            // Once the "map_is_eighth_twosome" bit is set, "osidemask",
            // and the "map_search->olatmask" field, are undefined.
            if (ilatlowbit & 1)
               map_search->map_is_eighth_twosome = true;

            int midbitpos = lowbitpos+1;
            uint32 ilatmidbit = (midbitpos >= 32) ?
               map_search->ilatmask3high >> (midbitpos-32) :
               map_search->ilatmask3low >> midbitpos;

            if ((ilatmidbit ^ map_search->rot ^ very_special) & 1) {
               for (j=0 ; j<m_people_per_group ; j++)
                  osidemask |= 1U << map_search->maps[i+(map_search->limit*j)];
            }
         }
      }

      osidemask ^= very_special;

      if (map_search->limit != attr::klimit(map_search->insetup)+1)
         gg77->iob88.fatal_error_exit(1, "Tandem table initialization failed", "limit wrong");

      if (!map_search->map_is_eighth_twosome) {
         if (map_search->olatmask != osidemask)
            gg77->iob88.fatal_error_exit(1, "Tandem table initialization failed", "smask");
      }
   }
}


extern void initialize_tandem_tables()
{
   initialize_one_table(maps_isearch_twosome, 2);
   initialize_one_table(maps_isearch_threesome, 3);
   initialize_one_table(maps_isearch_foursome, 4);
   initialize_one_table(maps_isearch_sixsome, 6);
   initialize_one_table(maps_isearch_eightsome, 8);
   initialize_one_table(maps_isearch_boxsome, 4);
   initialize_one_table(maps_isearch_dmdsome, 4);
   initialize_one_table(maps_isearch_tglsome, 3);
   initialize_one_table(maps_isearch_3x1tglsome, 4);
   initialize_one_table(maps_isearch_ysome, 4);
   initialize_one_table(maps_isearch_mimictwo, 2);
   initialize_one_table(maps_isearch_mimicfour, 2);
   initialize_one_table(maps_isearch_fudgy2x3, 2);
   initialize_one_table(maps_isearch_fudgy2x6, 4);
}


void tandrec::unpack_us(
   const tm_thing *map_ptr,
   uint32 orbitmask3high,
   uint32 orbitmask3low,
   setup *result) THROW_DECL
{
   int i, j;
   uint32 sglhigh, sgllow, ohigh, olow, r;

   r = map_ptr->rot*011;

   // The result of the unpacking goes to an enormous "hyper" array.
   // This array may be bigger than the maximum allowed value of 24.
   // We will then reduce it to the actual setup.

   personrec hyperarray[32];
   ::memset(hyperarray, 0, sizeof(hyperarray));
   uint32 hyperarrayoccupation = 0;

   for (i=0,
           sglhigh=map_ptr->insinglemaskhigh,
           sgllow=map_ptr->insinglemasklow,
           ohigh=orbitmask3high,
           olow=orbitmask3low;
        i<map_ptr->limit;
        i++,
           sgllow >>= 3,
           sgllow |= sglhigh << 29,
           sglhigh >>= 3,
           olow >>= 3,
           olow |= ohigh << 29,
           ohigh >>= 3) {
      uint32 z = rotperson(virtual_result.people[i].id1, r);
      if (z != 0) {
         int ii = (z >> 6) & 7;

         bool invert_order =
            (((olow>>1) + (map_ptr->rot&1) + 1) & 2) && !m_no_unit_symmetry;

         // Figure out whether we are unpacking a single person or multiple people.
         int howmanytounpack = 1;

         if (!(sgllow & 1)) {
            howmanytounpack = m_people_per_group;
            if (map_ptr->maps[i+map_ptr->limit] < 0)
               fail("This would go to an impossible setup.");
         }

         personrec fb[8];

         for (j=0 ; j<howmanytounpack ; j++) {
            fb[j] = m_real_saved_people[j].people[ii];
            if (fb[j].id1) fb[j].id1 =
                              (fb[j].id1 & ~(NSLIDE_ROLL_MASK|STABLE_ALL_MASK|077)) |
                              (z & (NSLIDE_ROLL_MASK|STABLE_ALL_MASK|013));
         }

         for (j=0 ; j<howmanytounpack ; j++) {
            int unpack_order = invert_order ? howmanytounpack-j-1 : j;
            if (fb[unpack_order].id1 != 0) {
               int hyperarrayplace = map_ptr->maps[i+(map_ptr->limit*j)];
               // This could only arise in pathological uses of the 1/8 skewsome
               // maps.  (And probably not even in that case.)
               if (hyperarray[hyperarrayplace].id1)
                  fail("Can't do couples or tandem concepts in this setup.");
               hyperarray[hyperarrayplace] = fb[unpack_order];
               hyperarrayoccupation |= 1<< hyperarrayplace;
            }
         }
      }
   }

   static const veryshort fixer_4x8a_4x4[24] = {
      5, 10, 29, 11, 18, 19, 20, 28,
      21, 26, 13, 27, 2, 3, 4, 12,
      -1, -1, -1, -1, -1, -1, -1, -1};

   static const veryshort fixer_4x8a_2x8[24] = {
      15, 14, 13, 12, 11, 10, 9, 8,
      31, 30, 29, 28, 27, 26, 25, 24,
      -1, -1, -1, -1, -1, -1, -1, -1};

   static const veryshort fixer_4x8b_2x4h[24] = {
      14, 13, 10, 9, 30, 29, 26, 25,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1};

   static const veryshort fixer_4x8b_2x4v[24] = {
      4, 11, 28, 19, 20, 27, 12, 3,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1};

   static const veryshort fixer_2x4_3x8[24] = {
      2, 3, 4, 5, 14, 15, 16, 17,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1};

   static const veryshort fixer_1x8_3x8[24] = {
      20, 21, 23, 22, 8, 9, 11, 10,
      -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1};

   static const veryshort fixer_2x8_2x16[24] = {
      4, 5, 6, 7, 8, 9, 10, 11,
      20, 21, 22, 23, 24, 25, 26, 27,
      -1, -1, -1, -1, -1, -1, -1, -1};

   static const veryshort fixer_2x12_2x24[24] = {
      2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
      18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29};

   static const veryshort lilstar1[8] = {6, 7, 1, 4, 0, 0, 0, 0};
   static const veryshort lilstar2[8] = {2, 3, 5, 0, 0, 0, 0, 0};
   static const veryshort lilstar3[8] = {0, 1, 4, 5, 0, 0, 0, 0};
   static const veryshort lilstar4[8] = {6, 7, 2, 3, 0, 0, 0, 0};

   result->kind = map_ptr->outsetup;
   result->rotation = virtual_result.rotation - map_ptr->rot;
   result->eighth_rotation = virtual_result.eighth_rotation;
   result->result_flags = virtual_result.result_flags;

   const veryshort *my_huge_map = (const veryshort *) 0;
   int rot = 0;

   if (result->kind == shyper4x8a) {
      if ((hyperarrayoccupation & 0xC3C3C3C3) == 0) {
         result->kind = s4x4;
         my_huge_map = fixer_4x8a_4x4;
      }
      else if ((hyperarrayoccupation & 0x00FF00FF) == 0) {
         result->kind = s2x8;
         my_huge_map = fixer_4x8a_2x8;
      }
   }
   else if (result->kind == shyper4x8b) {
      if ((hyperarrayoccupation & 0x99FF99FF) == 0) {
         result->kind = s2x4;
         my_huge_map = fixer_4x8b_2x4h;
      }
      else if ((hyperarrayoccupation & 0xE7E7E7E7) == 0) {
         result->kind = s2x4;
         my_huge_map = fixer_4x8b_2x4v;
         result->rotation++;
         rot = 033;
      }
   }
   else if (result->kind == shyper3x8) {
      if ((hyperarrayoccupation & 0xFC3FC3) == 0) {
         result->kind = s2x4;
         my_huge_map = fixer_2x4_3x8;
      }
      else if ((hyperarrayoccupation & 0x0FF0FF) == 0) {
         result->kind = s1x8;
         my_huge_map = fixer_1x8_3x8;
      }
   }
   else if (result->kind == shyper2x16) {
      if ((hyperarrayoccupation & 0xF00FF00F) == 0) {
         result->kind = s2x8;
         my_huge_map = fixer_2x8_2x16;
      }
      else if ((hyperarrayoccupation & 0xC003C003) == 0) {
         result->kind = s2x12;
         my_huge_map = fixer_2x12_2x24;
      }
   }
   else if (result->kind == slittlestars) {
      if ((hyperarrayoccupation & 0xCC) == 0) {
         result->kind = s2x2;
         my_huge_map = lilstar3;
      }
      else if ((hyperarrayoccupation & 0x33) == 0) {
         result->kind = s1x4;
         my_huge_map = lilstar4;
      }
      else if ((hyperarrayoccupation & 0x2D) == 0) {
         result->kind = s_trngl4;
         my_huge_map = lilstar1;
         result->rotation--;
         rot = 011;
      }
      else if ((hyperarrayoccupation & 0xD2) == 0) {
         result->kind = s_trngl4;
         my_huge_map = lilstar2;
         result->rotation++;
         rot = 033;
      }
   }
   else
      my_huge_map = identity24;

   if (!my_huge_map)
      fail("This would go to an impossible setup.");

   for (i=0; i<MAX_PEOPLE; i++) {
      if (my_huge_map[i] >= 0) {
         personrec fb = hyperarray[my_huge_map[i]];

         if (m_melded || fb.id1 != 0) {
            result->people[i] = fb;
            result->people[i].id1 = rotperson(fb.id1, rot);
         }
      }
   }
}



// Lat = 0 means the people we collapsed, relative to the incoming geometry,
// were one behind the other.  Lat = 1 means they were lateral.  "Incoming
// geometry" does not include the incoming rotation field, since we ignore it.
// We are not responsible for the rotation field of the incoming setup.

// The canonical storage of the real people, while we are doing the virtual
// call, is as follows:
//    m_real_saved_people[0] gets person on left (lat=1) near person (lat=0).
//    m_real_saved_people[last] gets person on right (lat=1) or far person (lat=0).

// This returns true if it found people facing the wrong way.  This can happen
// if we are trying siamese and we shouldn't be.

bool tandrec::pack_us(
   personrec *s,
   const tm_thing *map_ptr,
   int fraction,    // This is in eighths.
   bool fraction_in_use,
   bool dynamic_in_use,
   int key) THROW_DECL
{
   int i, j;
   uint32 mhigh, mlow, sglhigh, sgllow;
   int virt_index = -1;

   m_virtual_setup[0].clear_people();
   m_virtual_setup[0].rotation = map_ptr->rot & 3;
   m_virtual_setup[0].eighth_rotation = 0;
   m_virtual_setup[0].kind = map_ptr->insetup;
   m_virtual_setup[1] = m_virtual_setup[0];   // Need this one also.

   if (m_melded) {
      m_real_saved_people[0].clear_people();
      m_real_saved_people[1].clear_people();
   }

   for (i=0,
           mhigh=map_ptr->ilatmask3high,
           mlow=map_ptr->ilatmask3low,
           sglhigh=map_ptr->insinglemaskhigh,
           sgllow=map_ptr->insinglemasklow;
        i<map_ptr->limit;
        i++,
           sgllow >>= 3,
           sgllow |= sglhigh << 29,
           mlow >>= 3,
           mlow |= mhigh << 29) {

      // Process a virtual person, put together from some number (1 or m_people_per_group)
      // of people working together.

      personrec fb[8];    // Will receive the people being assembled.
      uint32 vp1, vp2, vp3;
      uint32 vp1a, vp2a, vp3a;
      vp1a = 0;   // Do we need these?
      vp2a = 0;
      vp3a = 0;
      // We know (hopefully) that this map doesn't have the "1/8 twosome" stuff, so the bits will be AB0.
      // This will get an error, of course.  Where m was ......AB,
      // mlow is .....AB0.  Or maybe AB1, in which case this map must not be used.

      // Just in case...
      if (mlow & 1) return true;

      int vert = (1 + map_ptr->rot + (mlow >> 1)) & 3;

      fb[0].id1 = 0;
      fb[0].id2 = 0;
      fb[0].id3 = 0;
      int thingy = map_ptr->maps[i];
      if (thingy >= 0) fb[0] = s[thingy];

      if (!m_no_unit_symmetry) vert &= 1;

      if (sgllow & 1) {
         // This person is not paired, as in "boys are tandem" when we are a girl.
         vp1 = fb[0].id1;
         vp2 = fb[0].id2;
         vp3 = fb[0].id3;
         fb[1].id1 = ~0U;
      }
      else {
         // This person is paired; pick up the other real people.
         uint32 orpeople1 = fb[0].id1;
         uint32 andpeople1 = fb[0].id1;

         for (j=1 ; j<m_people_per_group ; j++) {
            fb[j].id1 = 0;
            fb[j].id2 = 0;
            int thingy2 = map_ptr->maps[i+(map_ptr->limit*j)];
            if (thingy2 >= 0) fb[j] = s[thingy2];
            orpeople1 |= fb[j].id1;
            andpeople1 &= fb[j].id1;
         }

         if (key == tandem_key_skew) {
            // If this is skew/skewsome, we require people paired in the appropriate way.
            // This means [fb[0], fb[3]] must match, [fb[1], fb[2]] must match, and
            // [fb[0], fb[1]] must not match.

            // But, if phantoms are allowed, and nobody is home, we allow it.
            // And we allow empty spots if the reduced setup is larger than 4,
            // since we couldn't possibly fill them.  This allows skewsome Z axle.

            if ((orpeople1 ||
                 !(m_virtual_setup[0].cmd.cmd_misc_flags & CMD_MISC__PHANTOMS)) &&
                (attr::slimit(&m_virtual_setup[0]) < 4 || orpeople1) &&
                (((fb[0].id1 ^ fb[3].id1) |
                  (fb[1].id1 ^ fb[2].id1) |
                  (~(fb[0].id1 ^ fb[1].id1))) & BIT_PERSON))
               fail("Can't find skew people.");
         }
         else if (!(m_virtual_setup[0].cmd.cmd_misc_flags & CMD_MISC__PHANTOMS) && !m_phantom_pairing_ok) {

            // Otherwise, we usually forbid phantoms unless some phantom concept was used
            // (either something like "phantom tandem" or some other phantom concept such
            // as "split phantom lines").  If we get here, such a concept was not used.
            // We forbid a live person paired with a phantom.  Additionally, we forbid
            // ANY PERSON AT ALL to be a phantom, even if paired with another phantom,
            // except in the special case of a virtual 2x3.

            if (!(andpeople1 & BIT_PERSON)) {
               if (orpeople1 ||
                   (m_virtual_setup[0].kind != s2x3 &&
                    key != tandem_key_siam))
                  fail("Use \"phantom\" concept in front of this concept.");
            }
         }

         if (orpeople1) {
            if ((fraction_in_use || dynamic_in_use) && (orpeople1 & STABLE_ALL_MASK))
               fail("Sorry, can't nest fractional stable/twosome.");

            vp1 = ~0U;
            vp2 = ~0U;
            vp3 = ~0U;

            if (m_melded) {
               vp1 = 0;
               vp2 = 0;
               vp3 = 0;
               vp1a = 0;
               vp2a = 0;
               vp3a = 0;
            }

            // Create the virtual person.  When both people are present, anding
            // the real peoples' id2 bits gets the right bits.  For example,
            // the virtual person will be a boy and able to do a tandem star thru
            // if both real people were boys.  Remove the identity field (700 bits)
            // from id1 and replace with a virtual person indicator.  Check that
            // direction, roll, and stability parts of id1 are consistent.

            for (j=0 ; j<m_people_per_group ; j++) {
               if (fb[j].id1) {
                  // **** Do this right.
                  if (m_melded) {
                     if (j == 0) {
                        vp1 = fb[j].id1;
                        vp2 = fb[j].id2;
                        vp3 = fb[j].id3;
                     }
                     else {
                        vp1a = fb[j].id1;
                        vp2a = fb[j].id2;
                        vp3a = fb[j].id3;
                     }
                  }
                  else {
                     vp1 &= fb[j].id1;
                     vp2 &= fb[j].id2;
                     vp3 &= fb[j].id3;

                     // If they have different fractional stability states,
                     // just clear them -- they can't do it.
                     if ((fb[j].id1 ^ orpeople1) & STABLE_ALL_MASK) vp1 &= ~STABLE_ALL_MASK;
                     // If they have different slide or roll states, just clear them -- they can't roll.
                     if ((fb[j].id1 ^ orpeople1) & ROLL_DIRMASK) vp1 &= ~ROLL_DIRMASK;
                     if ((fb[j].id1 ^ orpeople1) & NSLIDE_MASK) vp1 &= ~NSLIDE_MASK;
                     // Check that all real people face the same way.
                     if ((fb[j].id1 ^ orpeople1) & 077)
                        return true;
                  }
               }
            }
         }
         else {
            vp1 = 0;
            vp1a = 0;
         }
      }

      for (int meld_number = 0 ; meld_number < 2 ; meld_number++) {
         personrec *ptr = &m_virtual_setup[meld_number].people[i];

         // **** Do this right.
         if (meld_number == 1) {
            vp1 = vp1a;
            vp2 = vp2a;
            vp3 = vp3a;
         }

         if (vp1) {
            // Create a virtual person number.  Only 8 are allowed, because of the tightness
            // in the person representation.  That shouldn't be a problem, since each
            // virtual person came from a group of real people that had at least one
            // live person.  Unless active phantoms have been created, that is.

            if ((++virt_index) >= 8) fail("Sorry, too many tandem or as couples people.");

            ptr->id1 = (vp1 & ~0700) | (virt_index << 6) | BIT_TANDVIRT;
            ptr->id2 = vp2;
            ptr->id3 = vp3;

            if (!(sgllow & 1)) {
               if (fraction_in_use)
                  ptr->id1 |= STABLE_ENAB | (STABLE_RBIT * fraction);
               else if (dynamic_in_use)
                  ptr->id1 |= STABLE_ENAB;

               // 1 if original people were near/far; 0 if lateral.
               m_vertical_people[virt_index] = vert;
            }

            if (map_ptr->rot)   // Compensate for setup rotation.
               ptr->id1 = rotperson(ptr->id1, ((- map_ptr->rot) & 3) * 011);

            if (m_melded) {
               m_real_saved_people[meld_number].people[virt_index] = fb[meld_number];
            }
            else {
               for (j=0 ; j<m_people_per_group ; j++)
                  m_real_saved_people[j].people[virt_index] = fb[j];
            }

            m_saved_rotations[virt_index] = ptr->id1 + m_virtual_setup[meld_number].rotation;
         }
         else {
            ptr->id1 = 0;
            ptr->id2 = 0;
            ptr->id3 = 0;
         }

         if (!m_melded)
            break;
      }
   }

   return false;
}


extern void tandem_couples_move(
   setup *ss,
   selector_kind selector,
   int twosome,           // solid=0 / twosome=1 / solid-to-twosome=2 / twosome-to-solid=3
                          // also, 4 bit => dynamic, 8 bit => reverse dynamic
   int fraction_fields,   // number fields, if doing fractional twosome/solid
   int phantom,           // normal=0 phantom=1 general-gruesome=2 gruesome-with-wave-check=3
                          // "4" bit on --> this is a "melded (phantom)" thing
                          // "8" bit on --> this is a plain "melded" thing
   tandem_key key,
   uint32 mxn_bits,
   bool phantom_pairing_ok,
   setup *result) THROW_DECL
{
   ss->clear_all_overcasts();
   uint32 dynamic = twosome >> 2;
   twosome &= 3;

   if (ss->cmd.cmd_misc2_flags & CMD_MISC2__DO_NOT_EXECUTE) {
      result->kind = nothing;
      clear_result_flags(result);
      return;
   }

   clean_up_unsymmetrical_setup(ss);

   selector_kind saved_selector;
   const tm_thing *incoming_map;
   const tm_thing *map_search;
   int i, people_per_group;
   uint32 jbit;
   bool fractional = false;
   int fraction_in_eighths = 0;
   bool fraction_eighth_part = false;
   bool dead_conc = false;
   bool dead_xconc = false;
   int fudgy2x3count = 0;
   int fudgy2x3limit = 0;
   tm_thing *our_map_table;
   int finalcount;
   setup ttt[8];

   uint32 special_mask = 0;
   result->clear_people();
   remove_z_distortion(ss);

   bool no_unit_symmetry = false;
   bool melded = (key == tandem_key_overlap_siam) || (phantom & 0xC);
   phantom &= 3;    // Get rid of those bits.

   // Look for very special cases of selectors that specify triangles and concepts
   // that work with same.  The triangle designators are:
   //   selector_inside_tgl
   //   selector_outside_tgl
   //   selector_inpoint_tgl
   //   selector_outpoint_tgl
   // and the action designators are
   //   SOLID
   //   SOLID @9/@9 THREESOME
   //   THREESOME
   //   THREESOME @9/@9 SOLID

   if (key == tandem_key_special_triangles) {
      switch (selector) {
      case selector_inside_tgl:
         key = tandem_key_inside_tgls;
         break;
      case selector_outside_tgl:
         key = tandem_key_outside_tgls;
         break;
      case selector_inpoint_tgl:
         key = tandem_key_inpoint_tgls;
         break;
      case selector_outpoint_tgl:
         key = tandem_key_outpoint_tgls;
         break;
      default:
         fail("Can't use this designator.");   // This will happen if say "girls work threesome".
      }

      selector = selector_uninitialized;
   }
   else if (selector == selector_inside_tgl || selector == selector_outside_tgl ||
            selector == selector_inpoint_tgl || selector == selector_outpoint_tgl) {
      // If it wasn't a couples/tandem concept, the special selectors are not allowed.
      fail("Can't use this designator.");   // This will happen if say "inside triangles work tandem".
   }

   if (mxn_bits != 0) {
      tandem_key transformed_key = key;

      if (key == tandem_key_tand3 || key == tandem_key_tand4)
         transformed_key = tandem_key_tand;
      else if (key == tandem_key_cpls3 || key == tandem_key_cpls4)
         transformed_key = tandem_key_cpls;
      else if (key == tandem_key_siam3 || key == tandem_key_siam4)
         transformed_key = tandem_key_siam;

      if ((transformed_key & ~1) != 0) fail("Can't do this combination of concepts.");

      switch (mxn_bits) {
      case INHERITFLAGNXNK_3X3:
         people_per_group = 3;
         our_map_table = maps_isearch_threesome;
         goto foobarves;
      case INHERITFLAGNXNK_4X4:
         people_per_group = 4;
         our_map_table = maps_isearch_foursome;
         goto foobarves;
      case INHERITFLAGNXNK_6X6:
         people_per_group = 6;
         our_map_table = maps_isearch_sixsome;
         goto foobarves;
      case INHERITFLAGNXNK_8X8:
         people_per_group = 8;
         our_map_table = maps_isearch_eightsome;
         goto foobarves;
      }

      {
         // ****** Maybe all this could be done in terms of "do_1x3_type_expansion".

         uint32 livemaskl, livemaskr, directionsl, directionsr;
         big_endian_get_directions(ss, directionsr, livemaskr, &directionsl, &livemaskl);

         if (mxn_bits == INHERITFLAGMXNK_2X1 || mxn_bits == INHERITFLAGMXNK_1X2) {
            people_per_group = 2;
            our_map_table = maps_isearch_twosome;

            if (ss->kind == s2x3 || ss->kind == s1x6) {
               if (transformed_key == tandem_key_tand) directionsr ^= 0x555;

               if (((directionsr ^ 0x0A8) & livemaskr) == 0 ||
                   ((directionsr ^ 0xA02) & livemaskr) == 0)
                  special_mask |= 044;

               if (((directionsr ^ 0x2A0) & livemaskr) == 0 ||
                   ((directionsr ^ 0x80A) & livemaskr) == 0)
                  special_mask |= 011;

               if (mxn_bits == INHERITFLAGMXNK_2X1 && ((directionsr ^ 0x02A) & livemaskr) == 0)
                  special_mask |= 011;

               if (mxn_bits == INHERITFLAGMXNK_2X1 && ((directionsr ^ 0xA80) & livemaskr) == 0)
                  special_mask |= 044;

               if (mxn_bits == INHERITFLAGMXNK_1X2 && ((directionsr ^ 0x02A) & livemaskr) == 0)
                  special_mask |= 044;

               if (mxn_bits == INHERITFLAGMXNK_1X2 && ((directionsr ^ 0xA80) & livemaskr) == 0)
                  special_mask |= 011;

               if (special_mask != 011 && special_mask != 044) special_mask = 0;
            }
         }
         else if (mxn_bits == INHERITFLAGMXNK_3X1 || mxn_bits == INHERITFLAGMXNK_1X3) {
            people_per_group = 3;
            our_map_table = maps_isearch_threesome;
            if (transformed_key == tandem_key_tand) directionsr ^= 0x555555;

            if (ss->kind == s2x4) {
               if (((directionsr ^ 0x02A8) & livemaskr) == 0 ||
                   ((directionsr ^ 0xA802) & livemaskr) == 0)
                  special_mask |= 0x88;

               if (((directionsr ^ 0x2A80) & livemaskr) == 0 ||
                   ((directionsr ^ 0x802A) & livemaskr) == 0)
                  special_mask |= 0x11;

               if (mxn_bits == INHERITFLAGMXNK_3X1 && ((directionsr ^ 0x00AA) & livemaskr) == 0)
                  special_mask |= 0x11;

               if (mxn_bits == INHERITFLAGMXNK_3X1 && ((directionsr ^ 0xAA00) & livemaskr) == 0)
                  special_mask |= 0x88;

               if (mxn_bits == INHERITFLAGMXNK_1X3 && ((directionsr ^ 0x00AA) & livemaskr) == 0)
                  special_mask |= 0x88;

               if (mxn_bits == INHERITFLAGMXNK_1X3 && ((directionsr ^ 0xAA00) & livemaskr) == 0)
                  special_mask |= 0x11;

               if (special_mask != 0x11 && special_mask != 0x88) special_mask = 0;
            }
            else if (ss->kind == s3x4 && livemaskr == 0xC3FC3F) {
               // Don't look at facing directions; there's only one way it can be.
               special_mask = 0x820;
            }
            else if (ss->kind == s3x4 && livemaskr == 0x3CF3CF) {
               special_mask = 0x410;
            }
            else if (ss->kind == s3x6 && livemaskl == 0x3 && livemaskr == 0x00FCC03F) {
               special_mask = 0022022;
            }
            else if (ss->kind == s3x6 && livemaskl == 0x0 && livemaskr == 0x0CFC033F) {
               special_mask = 0022022;
            }
            else if (ss->kind == s3x6 && livemaskl == 0xF && livemaskr == 0xC033F00C) {
               special_mask = 0220220;
            }
            else if (ss->kind == s3x6 && livemaskl == 0x0 && livemaskr == 0x3F300FCC) {
               special_mask = 0202202;
            }
            else if (ss->kind == s3dmd && livemaskr == 0xFC3FC3) {
               special_mask = 0x820;
            }
            else if (ss->kind == s_qtag) {
               special_mask = 0x44;
            }
            else if (ss->kind == s1x8) {
               if (((directionsr ^ 0x08A2) & livemaskr) == 0 ||
                   ((directionsr ^ 0xA208) & livemaskr) == 0)
                  special_mask |= 0x44;

               if (((directionsr ^ 0x2A80) & livemaskr) == 0 ||
                   ((directionsr ^ 0x802A) & livemaskr) == 0)
                  special_mask |= 0x11;

               if (mxn_bits == INHERITFLAGMXNK_3X1 && ((directionsr ^ 0x00AA) & livemaskr) == 0)
                  special_mask |= 0x11;

               if (mxn_bits == INHERITFLAGMXNK_3X1 && ((directionsr ^ 0xAA00) & livemaskr) == 0)
                  special_mask |= 0x44;

               if (mxn_bits == INHERITFLAGMXNK_1X3 && ((directionsr ^ 0x00AA) & livemaskr) == 0)
                  special_mask |= 0x44;

               if (mxn_bits == INHERITFLAGMXNK_1X3 && ((directionsr ^ 0xAA00) & livemaskr) == 0)
                  special_mask |= 0x11;

               if (special_mask != 0x11 && special_mask != 0x44) special_mask = 0;
            }
            else if (ss->kind == s3x1dmd) {
               if (((directionsr ^ 0x00A8) & 0xFCFC & livemaskr) == 0 ||
                   ((directionsr ^ 0xA800) & 0xFCFC & livemaskr) == 0)
                  special_mask |= 0x88;
            }
         }
         else
            fail("Can't do this combination of concepts.");

         if (!special_mask) fail("Can't find 3x3/3x1/2x1 people.");
      }

      foobarves:

      // This will make it look like "as couples" or "tandem", as needed,
      // for swapping masks, but won't trip the assumption transformation stuff.
      // It will also turn "threesome" into "twosome", as required by the rest of the code.
      key = (tandem_key) (transformed_key | 64);
   }
   else if (key == tandem_key_ys) {
      people_per_group = 4;
      our_map_table = maps_isearch_ysome;
      no_unit_symmetry = true;
   }
   else if (key == tandem_key_3x1tgls) {
      people_per_group = 4;
      our_map_table = maps_isearch_3x1tglsome;
      no_unit_symmetry = true;
   }
   else if (key >= tandem_key_outpoint_tgls) {
      people_per_group = 3;
      our_map_table = maps_isearch_tglsome;
      no_unit_symmetry = true;
      if (key == tandem_key_outside_tgls)
         selector = selector_outer6;
      else if (key == tandem_key_inside_tgls)
         selector = selector_center6;
      else if (key == tandem_key_wave_tgls || key == tandem_key_tand_tgls) {
         if (ss->kind == s_hrglass) {
            uint32 tbonetest =
               ss->people[0].id1 | ss->people[1].id1 |
               ss->people[4].id1 | ss->people[5].id1;

            if ((tbonetest & 011) == 011 || ((key ^ tbonetest) & 1))
               fail("Can't find the indicated triangles.");

            special_mask = 0x44;   // The triangles have to be these.
         }
         else if (ss->kind == s_bone) {
            uint32 tbonetest =
               ss->people[0].id1 | ss->people[1].id1 |
               ss->people[4].id1 | ss->people[5].id1;

            if ((tbonetest & 011) == 011 || !((key ^ tbonetest) & 1))
               fail("Can't find the indicated triangles.");

            special_mask = 0x88;   // The triangles have to be these.
         }
         else if (ss->kind == s_rigger) {
            uint32 tbonetest =
               ss->people[0].id1 | ss->people[1].id1 |
               ss->people[4].id1 | ss->people[5].id1;

            if ((tbonetest & 011) == 011 || !((key ^ tbonetest) & 1))
               fail("Can't find the indicated triangles.");

            special_mask = 0x44;   // The triangles have to be these.
         }
         else if (ss->kind == s_dhrglass) {
            uint32 tbonetest =
               ss->people[0].id1 | ss->people[1].id1 |
               ss->people[4].id1 | ss->people[5].id1;

            if ((tbonetest & 011) == 011 || !((key ^ tbonetest) & 1))
               fail("Can't find the indicated triangles.");

            special_mask = 0x88;   // The triangles have to be these.
         }
         else if (ss->kind == s_galaxy) {
            uint32 tbonetest =
               ss->people[1].id1 | ss->people[3].id1 |
               ss->people[5].id1 | ss->people[7].id1;

            if ((tbonetest & 011) == 011)
               fail("Can't find the indicated triangles.");

            if ((key ^ tbonetest) & 1)
               special_mask = 0x44;
            else
               special_mask = 0x11;
         }
         else if (ss->kind == s_c1phan) {
            int t = key;

            uint32 tbonetest = or_all_people(ss);

            if ((tbonetest & 010) == 0) t ^= 1;
            else if ((tbonetest & 1) != 0)
               fail("Can't find the indicated triangles.");

            if (t&1) special_mask = 0x2121;
            else     special_mask = 0x1212;
         }
         else
            fail("Can't find these triangles.");
      }
      else if ((key == tandem_key_outpoint_tgls || key == tandem_key_inpoint_tgls) &&
               ss->kind == s_qtag) {
         if (key == tandem_key_inpoint_tgls) {
            if (     (ss->people[0].id1 & d_mask) == d_east &&
                     (ss->people[1].id1 & d_mask) != d_west &&
                     (ss->people[4].id1 & d_mask) == d_west &&
                     (ss->people[5].id1 & d_mask) != d_east)
               special_mask = 0x22;
            else if ((ss->people[0].id1 & d_mask) != d_east &&
                     (ss->people[1].id1 & d_mask) == d_west &&
                     (ss->people[4].id1 & d_mask) != d_west &&
                     (ss->people[5].id1 & d_mask) == d_east)
               special_mask = 0x11;
         }
         else {
            if (     (ss->people[0].id1 & d_mask) != d_west &&
                     (ss->people[1].id1 & d_mask) == d_east &&
                     (ss->people[4].id1 & d_mask) != d_east &&
                     (ss->people[5].id1 & d_mask) == d_west)
               special_mask = 0x11;
            else if ((ss->people[0].id1 & d_mask) == d_west &&
                     (ss->people[1].id1 & d_mask) != d_east &&
                     (ss->people[4].id1 & d_mask) == d_east &&
                     (ss->people[5].id1 & d_mask) != d_west)
               special_mask = 0x22;
         }

         if (special_mask == 0)
            fail("Can't find designated point.");
      }
      else if (key != tandem_key_anyone_tgls || ss->kind != s_c1phan)
         // For <anyone>-based triangles in C1-phantom,
         // we just use whatever selector was given.
         fail("Can't find these triangles.");
   }
   else if (key == tandem_key_diamond) {
      people_per_group = 4;
      our_map_table = maps_isearch_dmdsome;
   }
   else if (key == tandem_key_box || key == tandem_key_skew) {
      people_per_group = 4;
      our_map_table = maps_isearch_boxsome;
   }
   else if (key >= tandem_key_tand4) {
      people_per_group = 4;
      our_map_table = maps_isearch_foursome;
   }
   else if (key >= tandem_key_tand3) {
      people_per_group = 3;
      our_map_table = maps_isearch_threesome;
   }
   else {    // Includes tandem_key_overlap_siam.
      people_per_group = 2;
      our_map_table = maps_isearch_twosome;
   }

   if (attr::slimit(ss) < 0)
      fail("Can't do tandem/couples concept from this position.");

   tandrec tandstuff(phantom_pairing_ok, no_unit_symmetry, melded);
   tandstuff.single_mask = 0;

   uint32 nsmask = 0;
   uint32 ewmask = 0;
   uint32 allmask = 0;

   if (key == tandem_key_overlap_siam) {
      if ((ss->kind != s2x4 && ss->kind != s_qtag && ss->kind != s_bone && ss->kind != s1x8) || phantom != 0)
         fail("Can't do this concept in this setup.");
      phantom = 1;
   }

   // We use the phantom indicator to forbid an already-distorted setup.
   // The act of forgiving phantom pairing is based on the setting of the
   // CMD_MISC__PHANTOMS bit in the incoming setup, not on the phantom indicator.

   if ((ss->cmd.cmd_misc_flags & CMD_MISC__DISTORTED) && (phantom != 0))
      fail("Can't specify phantom tandem/couples in virtual or distorted setup.");

   // Find out who is selected, if this is a "so-and-so are tandem".
   saved_selector = current_options.who;

   if (selector != selector_uninitialized)
      current_options.who = selector;

   for (i=0, jbit=1; i<=attr::slimit(ss); i++, jbit<<=1) {
      uint32 p = ss->people[i].id1;
      if (p) {
         allmask |= jbit;
         // We allow a "special" mask to override the selector.
         // We also tell "selectp" that "some" is a legal selector.
         if ((selector != selector_uninitialized && !selectp(ss, i, people_per_group)) ||
             (jbit & special_mask) != 0)
            tandstuff.single_mask |= jbit;
         else {
            if (p & 1)
               ewmask |= jbit;
            else
               nsmask |= jbit;
         }
      }
   }

   current_options.who = saved_selector;

   if (!allmask) {
      result->kind = nothing;
      clear_result_flags(result);
      return;
   }

   if (twosome >= 2) {
      fractional = true;

      int num = (fraction_fields & NUMBER_FIELD_MASK) << 2;
      int fractional_twosome_part_den = fraction_fields >> BITS_PER_NUMBER_FIELD;

      fraction_in_eighths =
         num / fractional_twosome_part_den;
      int fractional_twosome_part_num =
         num - fraction_in_eighths * fractional_twosome_part_den;

      int fracgcd = gcd(fractional_twosome_part_num, fractional_twosome_part_den);
      fractional_twosome_part_num /= fracgcd;
      fractional_twosome_part_den /= fracgcd;

      fraction_in_eighths <<= 1;  // Put it into eighths temporarily.

      if (fractional_twosome_part_num != 0) {
         if (fractional_twosome_part_num != 1 || fractional_twosome_part_den != 2)
            fail("Fraction is too complicated.");
         fraction_in_eighths++;
         fraction_eighth_part = true;
      }

      if (fraction_in_eighths > 8)
         fail("Can't do fractional twosome more than 4/4.");
   }

   warning_index siamese_warning = warn__none;
   bool doing_siamese = false;
   uint32 saveew, savens;

   if (key == tandem_key_box || key == tandem_key_skew) {
      ewmask = allmask;
      nsmask = 0;
   }
   else if (key == tandem_key_diamond) {
      if (ss->kind == s_ptpd || ss->kind == s3ptpd || ss->kind == s4ptpd || ss->kind == sdmd) {
         ewmask = allmask;
         nsmask = 0;
      }
      else {
         ewmask = 0;
         nsmask = allmask;
      }
   }
   else if (tandstuff.m_no_unit_symmetry) {
      if (key == tandem_key_anyone_tgls) {
         // This was <anyone>-based triangles.  The setup must have been a C1-phantom.
         // The current mask shows just the base.  Expand it to include the apex.
         ewmask |= nsmask;     // Get the base bits regardless of facing direction.
         if (allmask == 0xAAAA) {
            tandstuff.single_mask &= 0x7777;
            if (ewmask == 0x0A0A) {
               ewmask |= 0x8888;
               nsmask = 0;
            }
            else if (ewmask == 0xA0A0) {
               ewmask |= 0x8888;
               nsmask = ewmask;
               ewmask = 0;
            }
            else
               fail("Can't find these triangles.");
         }
         else if (allmask == 0x5555) {
            tandstuff.single_mask &= 0xBBBB;
            if (ewmask == 0x0505) {
               ewmask |= 0x4444;
               nsmask = ewmask;
               ewmask = 0;
            }
            else if (ewmask == 0x5050) {
               ewmask |= 0x4444;
               nsmask = 0;
            }
            else
               fail("Can't find these triangles.");
         }
         else
            fail("Can't find these triangles.");
      }
      else if (key == tandem_key_3x1tgls) {
         if (ss->kind == s2x6 || ss->kind == s4x6 || ss->kind == s2x12) {
            ewmask = 0;
            nsmask = allmask;
         }
         else {
            ewmask = allmask;
            nsmask = 0;
         }
      }
      else if (key == tandem_key_ys) {
         if (ss->kind == s_c1phan) {
            if (!((ss->people[0].id1 ^ ss->people[1].id1 ^ ss->people[4].id1 ^ ss->people[5].id1) & 2)) {
               ss->rotation++;
               canonicalize_rotation(ss);
               ss->rotation--;   // Since we won't look at the ss->rotation again until the end,
                                 // this will have the effect of rotating it back.
                                 // Note that allmask, if it is acceptable at all, is still correct
                                 // for the new rotation.
            }
            ewmask = 0;
            nsmask = allmask;
         }
         else {
            ewmask = allmask;
            nsmask = 0;
         }
      }
      else if (ss->kind == s_galaxy) {
         if (special_mask == 0x44) {
            ewmask = allmask;
            nsmask = 0;
         }
         else if (special_mask == 0x11) {
            ewmask = 0;
            nsmask = allmask;
         }
         else
            fail("Can't find these triangles.");
      }
      else if ((special_mask == 0x44 && ss->kind != s_rigger) ||
               special_mask == 0x1212 ||
               special_mask == 0x22 ||
               special_mask == 0x11) {
         ewmask = 0;
         nsmask = allmask;
      }
      else if (special_mask == 0x2121) {
         ewmask = allmask;
         nsmask = 0;
      }
      else if (ss->kind == s_c1phan)
         fail("Sorry, don't know who is lateral.");
      else {
         ewmask = allmask;
         nsmask = 0;
      }
   }
   else if (key == tandem_key_overlap_siam) {
      ewmask = allmask;  // This works for 2x4, 1x8, and qtag

      if (ss->kind == s_bone)
         ewmask &= ~0x33;

      nsmask = allmask & ~ewmask;
      tandstuff.m_phantom_pairing_ok = true;
   }
   else if (key & 2) {

      // Siamese.  Usually, we will insist on a true siamese separation.
      // But, if we fail, it may be because we are doing things "random
      // siamese" or whatever.  In that case, we will allow a pure
      // couples or tandem separation.

      const siamese_item *ptr;
      doing_siamese = true;
      saveew = ewmask;
      savens = nsmask;

      if (key == tandem_key_siam3)
         ptr = siamese_table_of_3;
      else if (key == tandem_key_siam4)
         ptr = siamese_table_of_4;
      else
         ptr = siamese_table_of_2;

      uint32 A = allmask & 0xFFFF;
      uint32 AA = (A << 16) | A;
      uint32 EN = ((ewmask & 0xFFFF) << 16) | (nsmask & 0xFFFF);

      for (; ptr->testkind != nothing; ptr++) {
         if (ptr->testkind == ss->kind && ((EN ^ ptr->testval) & AA) == 0) {
            siamese_warning = ptr->warning;
            uint32 siamese_fixup = ptr->fixup & 0xFFFFFF;
            // We seem to have a match.  However, it still might be wrong.
            ewmask ^= (siamese_fixup & allmask);
            nsmask ^= (siamese_fixup & allmask);
            if (ptr->fixup & 0x80000000U) tandstuff.m_phantom_pairing_ok = true;
            goto try_this;
         }
      }

      goto siamese_failed;
   }
   else if (key & 1) {
      // Couples -- swap masks.  Tandem -- do nothing.
      uint32 temp = ewmask;
      ewmask = nsmask;
      nsmask = temp;
   }

 try_this:

   // If this is "tandem in a tall 6", be sure we get the map that assumes the ends
   // of the 2x6 are oriented differently.
   if ((ss->cmd.cmd_misc_flags & CMD_MISC__VERIFY_MASK) == CMD_MISC__VERIFY_TALL6) {
      if (ss->kind == s2x6) {
         ewmask |= 03636;
         nsmask |= 04141;
      }
      else if (ss->kind == sdeepxwv) {
         ewmask |= 00303;
         nsmask |= 07474;
      }
   }

   // Now ewmask and nsmask have the info about who is paired with whom.
   ewmask &= ~tandstuff.single_mask;         // Clear out unpaired people.
   nsmask &= ~tandstuff.single_mask;

   // Initiate the "fudgy 2x3" or "fudgy 2x6" mechanism.

   if (phantom == 0 && 
       (nsmask == 0 && ewmask == allmask) | (ewmask == 0 && nsmask == allmask)) {
      if ((key == tandem_key_cpls || key == tandem_key_tand) && ss->kind == s2x3 &&
          (allmask == 033 || allmask == 066)) {
         our_map_table = maps_isearch_fudgy2x3;
         incoming_map = &maps_isearch_fudgy2x3[(allmask == 033) ? 4 : 0];
         fudgy2x3limit = 1;
         goto fooy;
      }
      else if ((key == tandem_key_cpls4 || key == tandem_key_tand4) && ss->kind == s2x6 &&
          (allmask == 01717 || allmask == 07474)) {
         our_map_table = maps_isearch_fudgy2x6;
         incoming_map = &maps_isearch_fudgy2x6[(allmask == 01717) ? 4 : 0];
         fudgy2x3limit = 1;
         goto fooy;
      }
   }

   map_search = our_map_table;
   while (map_search->outsetup != nothing) {
      if ((map_search->outsetup == ss->kind) &&
          !map_search->map_is_eighth_twosome &&
          (allmask & map_search->outsinglemask) == tandstuff.single_mask &&
          (!(allmask & map_search->outunusedmask)) &&
          (!(ewmask & (~map_search->olatmask))) &&
          (!(nsmask & map_search->olatmask))) {
         incoming_map = map_search;
         goto fooy;
      }
      map_search++;
   }

   if (!doing_siamese)
      fail("Can't do this tandem or couples call in this setup or with these people selected.");

   goto siamese_failed;

 fooy:

   tandstuff.m_people_per_group = people_per_group;
   tandstuff.m_virtual_setup[0].cmd = ss->cmd;
   tandstuff.m_virtual_setup[0].cmd.cmd_assume.assumption = cr_none;

   // We also use the subtle aspects of the phantom indicator to tell what kind
   // of setup we allow, and whether pairings must be parallel to the long axis.

   if (phantom == 1 && !melded) {
      if (ss->kind != s2x8 && ss->kind != s4x4 && ss->kind != s3x4 && ss->kind != s2x6 &&
          ss->kind != s4x6 && ss->kind != s1x12 && ss->kind != s2x12 && ss->kind != s1x16 &&
          ss->kind != s3dmd && ss->kind != s_323 &&
          ss->kind != s4dmd && ss->kind != s3x8)
         fail("Can't do couples or tandem concepts in this setup.");
   }
   else if (phantom >= 2) {
      if (ss->kind != s2x8 || incoming_map->insetup != s2x4)
         fail("Can't do gruesome concept in this setup.");
   }

   // There are a small number of assumptions that we can transform.

   if (ss->cmd.cmd_assume.assump_col == 4) {
      if (ss->cmd.cmd_assume.assumption == cr_ijright)
         tandstuff.m_virtual_setup[0].cmd.cmd_assume.assumption = cr_jright;
      else if (ss->cmd.cmd_assume.assumption == cr_ijleft)
         tandstuff.m_virtual_setup[0].cmd.cmd_assume.assumption = cr_jleft;
   }

   if (ss->cmd.cmd_assume.assumption == cr_jright ||
       ss->cmd.cmd_assume.assumption == cr_jleft)
      fail("Couples or tandem concept is inconsistent with phantom facing direction.");

   if (key == tandem_key_cpls) {
      switch (ss->cmd.cmd_assume.assumption) {
      case cr_wave_only:
         fail("Couples or tandem concept is inconsistent with phantom facing direction.");
         break;
      case cr_li_lo:
         if (ss->kind == s2x2 || ss->kind == s2x4)
            tandstuff.m_virtual_setup[0].cmd.cmd_assume.assumption = ss->cmd.cmd_assume.assumption;
         break;
      case cr_1fl_only:
         if (ss->kind == s1x4 || ss->kind == s2x4)
            tandstuff.m_virtual_setup[0].cmd.cmd_assume.assumption = cr_couples_only;
         break;
      case cr_2fl_only:
         if (ss->kind == s1x4 || ss->kind == s2x4 || ss->kind == s1x8)
            tandstuff.m_virtual_setup[0].cmd.cmd_assume.assumption = cr_wave_only;
         break;
      }
   }

   if (phantom == 3) tandstuff.m_virtual_setup[0].cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

   // Give it the fraction in quarters, rounded down if an eighth part was specified.
   if (!tandstuff.pack_us(ss->people, incoming_map, fraction_in_eighths, fractional, dynamic != 0, key))
      goto got_good_separation;

   // Or failure to pack people properly may just be because we are taking
   // "siamese" too seriously.

   if (!doing_siamese)
      fail("People not facing same way for tandem or as couples.");

 siamese_failed:

   // We allow pointless siamese if this is a part of a call that is being done
   // "piecewise" or "random" or whatever.
   if (!(ss->cmd.cmd_fraction.flags & CMD_FRAC_BREAKING_UP))
      fail("Can't do Siamese in this setup.");

   nsmask = savens;
   ewmask = saveew;

   if (key & 2) {
      key = (tandem_key) (key & ~2);    // Turn off siamese -- it's now effectively as couples
      key = (tandem_key) (key | 1);
      uint32 temp(ewmask);
      ewmask = nsmask;
      nsmask = temp;
   }
   else if (key & 1)
      key = (tandem_key) (key & ~1);    // Try tandem
   else
      fail("Can't do this tandem or couples call in this setup or with these people selected.");

   goto try_this;

 got_good_separation:

   warn(siamese_warning);

   if (tandstuff.m_virtual_setup[0].kind == s_ntrgl6cw ||
       tandstuff.m_virtual_setup[0].kind == s_ntrgl6ccw)
      tandstuff.m_virtual_setup[0].cmd.cmd_misc3_flags |= CMD_MISC3__SAID_TRIANGLE;

   update_id_bits(&tandstuff.m_virtual_setup[0]);

   int tttcount = 1;

   if (melded) {
      tttcount = -1;
      uint32 rotstate, pointclip;

      update_id_bits(&tandstuff.m_virtual_setup[1]);

      for (int k=0 ; k<2 ; k++) {
         for (int j=0 ; j<=attr::klimit(incoming_map->insetup) ; j++) {
            setup sss = tandstuff.m_virtual_setup[k];
            sss.clear_people();
            if (copy_person(&sss, j, &tandstuff.m_virtual_setup[k], j)) {
               if ((++tttcount) >= 8) fail("Sorry, too many tandem or as couples people.");
               impose_assumption_and_move(&sss, &ttt[tttcount], true);
            }
         }
      }

      fix_n_results(tttcount+1, -1, ttt, rotstate, pointclip, 0);

      if (ttt[0].kind == nothing || !(rotstate & 0xF03))
         fail("Can't do this.");

      result->result_flags = get_multiple_parallel_resultflags(ttt, tttcount+1);
   }
   else {
      // If going to a 2x2, remember physical 2x4 elongation.  This may help in the case
      // of certain semi-bogus "crazy" stuff.  We always set this to indicate east-west.
      // Why?  Because we have dropped the orientation of the 2x4, and will reinstate it later.
      // So it's always east-west in our present view.
      if (ss->kind == s2x4 && tandstuff.m_virtual_setup[0].kind == s2x2)
         tandstuff.m_virtual_setup[0].cmd.prior_elongation_bits |= PRIOR_ELONG_BASE_FOR_TANDEM;
      impose_assumption_and_move(&tandstuff.m_virtual_setup[0], &ttt[0], true);
   }

   // This loop runs only once, unless melded, in which case it runs eight times.
   for (finalcount = 0 ; finalcount <= tttcount ; finalcount++) {
      tandstuff.virtual_result = ttt[finalcount];
      remove_fudgy_2x3_2x6(&tandstuff.virtual_result);
      remove_mxn_spreading(&tandstuff.virtual_result);
      remove_tgl_distortion(&tandstuff.virtual_result);

      // If this is a concentric setup with dead ends or centers, we can still handle it.
      // We have to remember to put dead folks back when we are done, in order
      // to make gluing illegal.

      if (tandstuff.virtual_result.kind == s_dead_concentric) {
         if (fraction_eighth_part) fail("Fraction is too complicated.");
         dead_conc = true;
         tandstuff.virtual_result.kind = tandstuff.virtual_result.inner.skind;
         tandstuff.virtual_result.rotation += tandstuff.virtual_result.inner.srotation;
      }
      else if (tandstuff.virtual_result.kind == s_normal_concentric &&
               tandstuff.virtual_result.inner.skind == nothing &&
               tandstuff.virtual_result.outer.skind == s1x2) {
         if (fraction_eighth_part) fail("Fraction is too complicated.");
         dead_xconc = true;
         tandstuff.virtual_result.kind = tandstuff.virtual_result.outer.skind;
         tandstuff.virtual_result.rotation += tandstuff.virtual_result.outer.srotation;
      }

      if (attr::slimit(&tandstuff.virtual_result) < 0)
         fail("Don't recognize ending position from this tandem or as couples call.");

      // Bits appear in this 64-bit item in triples!  All 3 bits are the same.
      uint32 sglmask3high = 0;
      uint32 sglmask3low = 0;

      // Bits appear in this 64-bit item in triples!  All 3 bits are the same.
      uint32 livemask3high = 0;
      uint32 livemask3low = 0;

      // Bits appear in this 64-bit item in triples!  The 3 bits give the orbit in eighths.
      uint32 orbitmask3high = 0;
      uint32 orbitmask3low = 0;

      // Compute orbitmask3, livemask3, and sglmask3.
      // Since we are synthesizing bit masks, we scan in reverse order to make things easier.
      // The resultant masks will be little-endian.

      for (i=attr::slimit(&tandstuff.virtual_result); i>=0; i--) {
         sglmask3high <<= 3;
         sglmask3high |= sglmask3low >> 29;
         sglmask3low <<= 3;

         livemask3high <<= 3;
         livemask3high |= livemask3low >> 29;
         livemask3low <<= 3;

         orbitmask3high <<= 3;
         orbitmask3high |= orbitmask3low >> 29;
         orbitmask3low <<= 3;

         uint32 p = tandstuff.virtual_result.people[i].id1;

         if (p) {
            int vpi = (p >> 6) & 7;
            livemask3low |= 7;

            if (!melded && tandstuff.m_real_saved_people[1].people[vpi].id1 == ~0U) {
               sglmask3low |= 7;
            }
            else {
               if (fractional || dynamic != 0) {
                  if (!(p & STABLE_ENAB))
                     fail("fractional twosome not supported for this call.");
               }

               int orbit_in_eighths = (p + tandstuff.virtual_result.rotation - tandstuff.m_saved_rotations[vpi]) << 1;

               uint32 stable_stuff_in_eighths = ((p & STABLE_VRMASK) / STABLE_VRBIT) - ((p & STABLE_VLMASK) / STABLE_VLBIT);

               if (twosome == 3) {
                  // This is "twosome to solid" -- they orbit by whatever the excess is after the stability expires.
                  orbit_in_eighths = 0;
                  if (fractional) {
                     if (dynamic == 1)
                        orbit_in_eighths += (p & STABLE_VRMASK) / STABLE_VRBIT;
                     else if (dynamic == 2)
                        orbit_in_eighths -= (p & STABLE_VLMASK) / STABLE_VLBIT;
                     else
                        orbit_in_eighths = stable_stuff_in_eighths;
                  }
               }
               else if (twosome == 2) {
                  // This is "solid to twosome" -- they orbit by whatever happened before the stability expired.
                  if (fractional) {
                     if (dynamic == 1)
                        orbit_in_eighths += (p & STABLE_VLMASK) / STABLE_VLBIT;
                     else if (dynamic == 2)
                        orbit_in_eighths -= (p & STABLE_VRMASK) / STABLE_VRBIT;
                     else
                        orbit_in_eighths -= stable_stuff_in_eighths;
                  }
               }
               else if (twosome == 1) {
                  // This is "twosome" -- they don't orbit (unless "dynamic" is on).
                  orbit_in_eighths = 0;
                  if (dynamic == 1)
                     orbit_in_eighths += (p & STABLE_VRMASK) / STABLE_VRBIT;
                  else if (dynamic == 2)
                     orbit_in_eighths -= (p & STABLE_VLMASK) / STABLE_VLBIT;
               }

               orbitmask3low |=
                  (orbit_in_eighths - ((tandstuff.virtual_result.rotation + tandstuff.m_vertical_people[vpi]) << 1)) & 7;
            }

            if (fractional || dynamic != 0)
               tandstuff.virtual_result.people[i].id1 &= ~STABLE_ALL_MASK;
         }
      }

      uint32 orbitcomhigh = orbitmask3high ^ 0155555U;
      uint32 orbitcomlow = orbitmask3low ^ 026666666666U;
      uint32 hmask3high = orbitcomhigh & livemask3high & ~sglmask3high;
      uint32 hmask3low = orbitcomlow & livemask3low & ~sglmask3low;

      // Bits appear here in triples!  Only low bit of each triple is used.
      uint32 sglmaskhigh = sglmask3high & 022222U;
      uint32 sglmasklow = sglmask3low & 011111111111U;

      // Bits appear here in triples!  Only low two bits of each triple are used.
      uint32 livemaskhigh = livemask3high & 066666U;
      uint32 livemasklow = livemask3low & 033333333333U;

      // Pick out only low two bits for map search, and only bits of live paired people.
      uint32 hmaskhigh = hmask3high & 066666U;
      uint32 hmasklow = hmask3low & 033333333333U;

      if (tandstuff.m_no_unit_symmetry) {
         livemaskhigh = livemask3high;
         livemasklow = livemask3low;
         hmaskhigh = hmask3high;
         hmasklow = hmask3low;
      }

      // If doing a "fudgy 2x3", we have to do two separate actions, with two different getin maps.
      if (fudgy2x3limit != 0) {
         map_search = incoming_map;
      }
      else {
         map_search = our_map_table;
      }

      // We don't accept the special 1/8-twosome maps from the "maps_isearch_boxsome"
      // table unless we are doing "skew".  If we are doing "box" (the only other
      // possibility) the maps are not used.
      while (map_search->outsetup != nothing) {
         if ((map_search->insetup == tandstuff.virtual_result.kind) &&
             (!map_search->map_is_eighth_twosome || key != tandem_key_box) &&
             (map_search->insinglemaskhigh & livemaskhigh) == sglmaskhigh &&
             (map_search->insinglemasklow & livemasklow) == sglmasklow &&
             (map_search->ilatmask3high & livemaskhigh) == hmaskhigh &&
             (map_search->ilatmask3low & livemasklow) == hmasklow) {
            break;
         }
         map_search++;
      }

      if (map_search->outsetup == nothing)
         fail("Don't recognize ending position from this tandem or as couples call.");

      tandstuff.unpack_us(map_search, orbitmask3high, orbitmask3low, result);

      if (fudgy2x3limit != 0) {
         // We should have gotten out either with the same map we went in with,
         // or (for orientation changer) the very next one.
         if (map_search != incoming_map && map_search != incoming_map+1)
            fail("Internal error: failed to find getout map for a 1x1.");

         // See whether to do another round.
         if (fudgy2x3count < fudgy2x3limit) {
            // Did the first one, go back and do the second.  Don't canonicalize the rotation.
            fudgy2x3count++;
            incoming_map += 2;
            goto fooy;
         }
      }

      canonicalize_rotation(result);
      reinstate_rotation(ss, result);

      if (fudgy2x3limit != 0 && ss->rotation != result->rotation)
         warn(warn_controversial);

      // When we fudge wrongly-oriented triangles to a 2x4, we need
      // to say something.
      if (our_map_table == maps_isearch_tglsome && result->kind == s2x4)
         warn(warn__check_hokey_2x4);

      if (dead_conc) {
         result->inner.skind = result->kind;
         result->inner.srotation = result->rotation;
         result->rotation = 0;
         result->eighth_rotation = 0;
         result->kind = s_dead_concentric;
      }
      else if (dead_xconc) {
         result->outer.skind = result->kind;
         result->outer.srotation = result->rotation;
         result->concsetup_outer_elongation = tandstuff.virtual_result.outer.srotation+1;
         result->rotation = 0;
         result->eighth_rotation = 0;
         result->inner.skind = nothing;
         result->kind = s_normal_concentric;
         for (i=0 ; i<MAX_PEOPLE/2 ; i++) result->swap_people(i, i+MAX_PEOPLE/2);
      }
      else if (ss->kind == s1x4 && result->kind == s2x2) {
         result->result_flags.misc &= ~3;
         result->result_flags.misc |= (ss->rotation & 1) + 1;
      }
      else if (ss->kind == s2x2 && result->kind == s2x2) {
         result->result_flags.misc &= ~3;
         if (ss->cmd.prior_elongation_bits & 3)
            result->result_flags.misc |= ss->cmd.prior_elongation_bits & 3;
      }

      // In the usual case (not melded) we just exit from the whole thing after one iteration.
      if (!melded)
         break;

      // If melded, we do everything eight times
      ttt[finalcount] = *result;   // Save the result of this run
      if (result->eighth_rotation != 0) fail("Can't do this.");  // Would be way too hairy.
   }

   if (melded) {
      // We can't just merge all 8 of the setups in a haphazard fashion.  We have to merge 2x4's,
      // of both orientation, with each other, and simplarly merge general 1x8's with each other.
      int horizontal_2x4_indices = -1;
      int vertical_2x4_indices = -1;
      int horizontal_1x8_indices = -1;
      int vertical_1x8_indices = -1;

      // Grab the various groups, in both orientations.
      // 1/4 tags with only the centers occupied, and riggers with only the wings occupied, count
      // as honorary 1x8's, as do 1x6's, 1x4's and 1x2's.  They will all merge just fine, even when
      // done in a haphazard fashion.
      for (i=0 ; i<=tttcount ; i++) {
         uint32 the_mask = little_endian_live_mask(&ttt[i]);

         if (ttt[i].kind == s2x4) {
            if (ttt[i].rotation & 1) {
               if (vertical_2x4_indices < 0)
                  vertical_2x4_indices = i;
               else {
                  merge_table::merge_setups(&ttt[i], merge_strict_matrix, &ttt[vertical_2x4_indices]);
                  ttt[i].kind = nothing;   // Be sure we don't see it in the next scan.
               }
            }
            else {
               if (horizontal_2x4_indices < 0)
                  horizontal_2x4_indices = i;
               else {
                  merge_table::merge_setups(&ttt[i], merge_strict_matrix, &ttt[horizontal_2x4_indices]);
                  ttt[i].kind = nothing;
               }
            }
         }
         else if (ttt[i].kind == s1x8 || ttt[i].kind == s1x6 || ttt[i].kind == s1x4 ||
                  (ttt[i].kind == s_qtag && (the_mask & 0x33) == 0) ||
                  (ttt[i].kind == s_rigger && (the_mask & 0x33) == 0)) {
            if (ttt[i].rotation & 1) {
               if (vertical_1x8_indices < 0)
                  vertical_1x8_indices = i;
               else {
                  merge_table::merge_setups(&ttt[i], merge_strict_matrix, &ttt[vertical_1x8_indices]);
                  ttt[i].kind = nothing;
               }
            }
            else {
               if (horizontal_1x8_indices < 0)
                  horizontal_1x8_indices = i;
               else {
                  merge_table::merge_setups(&ttt[i], merge_strict_matrix, &ttt[horizontal_1x8_indices]);
                  ttt[i].kind = nothing;
               }
            }
         }
      }

      // If we have both horizontal and vertical 2x4's, combine them now..
      if (horizontal_2x4_indices >= 0 && vertical_2x4_indices >= 0) {
         merge_table::merge_setups(&ttt[vertical_2x4_indices], merge_strict_matrix, &ttt[horizontal_2x4_indices]);
         ttt[vertical_2x4_indices].kind = nothing;
      }
      else if (vertical_2x4_indices >= 0) {
         horizontal_2x4_indices = vertical_2x4_indices;
      }

      // We now have all 2x4's, horizontal and vertical, in horizontal_2x4_indices.  So this might
      // be a 4x4.  We also have 1x8's, or honorary 1x8's, in horizontal_1x8_indices and vertical_1x8_indices.
      // And whatever is left over.
      //
      // Merge everything into horizontal_2x4_indices, and return that.

      // First, the horizontal 1x8's.
      if (horizontal_1x8_indices >= 0) {
         if (horizontal_2x4_indices < 0)
            horizontal_2x4_indices = horizontal_1x8_indices;
         else {
            merge_table::merge_setups(&ttt[horizontal_1x8_indices], merge_strict_matrix, &ttt[horizontal_2x4_indices]);
            ttt[horizontal_1x8_indices].kind = nothing;
         }
      }

      // Next, the vertical 1x8's.
      if (vertical_1x8_indices >= 0) {
         if (horizontal_2x4_indices < 0)
            horizontal_2x4_indices = vertical_1x8_indices;
         else {
            merge_table::merge_setups(&ttt[vertical_1x8_indices], merge_strict_matrix, &ttt[horizontal_2x4_indices]);
            ttt[vertical_1x8_indices].kind = nothing;
         }
      }

      // Finally, do the stragglers.
      for (i=0 ; i<=tttcount ; i++) {
         if (ttt[i].kind != nothing && i != horizontal_2x4_indices) {
            if (horizontal_2x4_indices < 0)
               horizontal_2x4_indices = i;
            else
               merge_table::merge_setups(&ttt[i], merge_strict_matrix, &ttt[horizontal_2x4_indices]);
         }
      }

      *result = ttt[horizontal_2x4_indices];
   }

   result->clear_all_overcasts();
}


static void fixup_mimic(setup *result, const uint16 split_info[2],
                 uint32 division_code, const setup *orig_before_press) THROW_DECL
{
   int orig_width = -1;
   int orig_height = -1;

   if (orig_before_press) {
      orig_width = setup_attrs[orig_before_press->kind].bounding_box[orig_before_press->rotation ^ result->rotation];
      orig_height = setup_attrs[orig_before_press->kind].bounding_box[orig_before_press->rotation ^ result->rotation ^ 1];

      // Diamonds don't have long-axis bounding box defined, for reasons that don't concern us here.
      if (orig_before_press->kind == sdmd) {
         if ((orig_before_press->rotation ^ result->rotation) == 1)
            orig_height = 3;
         else
            orig_width = 3;
      }
   }

   const expand::thing *compress_map = (const expand::thing *) 0;
   const veryshort *srclist = (veryshort *) 0;
   const veryshort *dstlist;
   const uint32 *census_ptr = (uint32 *) 0;

   static const veryshort listfor1x1[] = {(veryshort) s1x1, 0, 1,   0, -1};
   static const veryshort listfor1x2[] = {(veryshort) s1x2, 0, 2,   0, 1, -1};
   static const veryshort listfor1x2V[]= {(veryshort) s1x2, 1, 1,   0, 1, -1};
   static const veryshort listfor1x4[] = {(veryshort) s1x4, 0, 4,   0, 1, 3, 2, -1};
   static const veryshort listfor1x8[] = {(veryshort) s1x8, 0, 8,   0, 1, 3, 2, 6, 7, 5, 4, -1};
   static const veryshort listfor2x2[] = {(veryshort) s2x2, 0, 2,   0, 1, 3, 2, -1};
   static const veryshort listfor2x2V[]= {(veryshort) s2x2, 1, 2,   0, 3, 1, 2, -1};
   static const veryshort listfordmd[] = {(veryshort) sdmd, 0, 1,   0, 1, 2, 3, -1};
   static const veryshort listforrig[] = {(veryshort) s_rigger,0,2, 6, 7, 0, 1, 3, 2, 5, 4, -1};
   static const veryshort listfor2x4[] = {(veryshort) s2x4, 0, 4,   0, 1, 2, 3, 7, 6, 5, 4, -1};
   static const veryshort listfor1x4V[]= {(veryshort) s1x4, 1, 1,   0, 1, 3, 2, -1};
   static const veryshort listfor2x4V[]= {(veryshort) s2x4, 1, 2,   0, 7, 1, 6, 2, 5, 3, 4, -1};
   static const veryshort listfor2x8[] = {(veryshort) s2x8, 0, 8,
                                          0, 1, 2, 3, 4, 5, 6, 7,
                                          15, 14, 13, 12, 11, 10, 9, 8, -1};
   static const veryshort listfor4x4[] = {(veryshort) s4x4, 0, 4,
                                          12, 13, 14, 0, 10, 15, 3, 1,
                                          9, 11, 7, 2, 8, 6, 5, 4, -1};
   static const veryshort listfor4x4V[]= {(veryshort) s4x4, 1, 4,
                                          12, 10, 9, 8, 13, 15, 11, 6,
                                          14, 3, 7, 5, 0, 1, 2, 4, -1};

   static const expand::thing expAA_2x4_dmd = {{7, 1, 3, 5}, sdmd, s2x4, 0};
   static const expand::thing exp55_2x4_dmd = {{0, 2, 4, 6}, sdmd, s2x4, 0};

   static const expand::thing expCC_qtg_line = {{6, 7, 2, 3}, s1x4, s_qtag, 0};
   static const expand::thing exp33_qtg_box  = {{0, 1, 4, 5}, s2x2, s_qtag, 0};

   static const expand::thing exp55_xwv_dmd = {{0, 2, 4, 6}, sdmd, s_crosswave, 0};
   static const expand::thing expAA_xwv_dmd = {{1, 3, 5, 7}, sdmd, s_crosswave, 0};
   static const expand::thing exp99_xwv_dmd = {{0, 3, 4, 7}, sdmd, s_crosswave, 0};
   static const expand::thing exp66_xwv_dmd = {{1, 2, 5, 6}, sdmd, s_crosswave, 0};

   static const expand::thing exp55_qtg_dmd = {{4, 6, 0, 2}, sdmd, s_qtag, 1};
   static const expand::thing expAA_qtg_dmd = {{5, 7, 1, 3}, sdmd, s_qtag, 1};
   static const expand::thing exp99_qtg_dmd = {{4, 7, 0, 3}, sdmd, s_qtag, 1};
   static const expand::thing exp66_qtg_dmd = {{5, 6, 1, 2}, sdmd, s_qtag, 1};

   static const expand::thing exp_rig_ctr = {{0, 1, 4, 5}, s2x2, s_rigger, 0};
   static const expand::thing exp_rig_ctrR = {{5, 0, 1, 4}, s2x2, s_rigger, 1};

   static const expand::thing exp_bone_ctr = {{6, 7, 2, 3}, s1x4, s_bone, 0};
   static const expand::thing exp_bone_ctrR = {{2, 3, 6, 7}, s1x4, s_bone, 1};

   static const expand::thing exp_bone_end = {{0, 1, 4, 5}, s2x2, s_bone, 0};
   static const expand::thing exp_bone_endR = {{5, 0, 1, 4}, s2x2, s_bone, 1};

   // High 2 hex digits for top row and bottom row, next 4 nibbles for columns left to right.
   static const uint32 census_2x4[] = {0x1040, 0x1010, 0x1004, 0x1001,
                                       0x0101, 0x0104, 0x0110, 0x0140};

   // High 2 hex digits for rows top and bottom, next 2 for columns left and right.
   static const uint32 census_2x2[] = {
      0x1010, 0x1001, 0x0101, 0x0110};

   // High 4 hex digits for rows top to bottom, next 4 for columns left to right.
   static const uint32 census_4x4[] = {
      0x10000001, 0x01000001, 0x00100001, 0x01000010,
      0x00010001, 0x00010010, 0x00010100, 0x00100010,
      0x00011000, 0x00101000, 0x01001000, 0x00100100,
      0x10001000, 0x10000100, 0x10000010, 0x01000100};

   // High 2 hex digits for rows top and bottom, next 8 nibbles for columns left to right.
   static const uint32 census_2x8[] = {
      0x104000, 0x101000, 0x100400, 0x100100,
      0x100040, 0x100010, 0x100004, 0x100001,
      0x010001, 0x010004, 0x010010, 0x010040,
      0x010100, 0x010400, 0x011000, 0x014000};

   // High 4 hex digits for rows around the diamond, next 6 nibbles for columns left to right.
   static const uint32 census_rig[] = {
      0x0100040, 0x0100010, 0x0010001, 0x0010004,
      0x0001010, 0x0001040, 0x1000400, 0x1000100};

   // High 4 hex digits for pairs left to right, next 6 nibbles for columns left to right.
   static const uint32 census_bone[] = {
      0x1000400, 0x0001001, 0x0010004, 0x0010010,
      0x0001001, 0x1000400, 0x0100100, 0x0100040};

   uint32 finals = little_endian_live_mask(result);
   setup temp1 = *result;
   uint32 row_column_census = 0;

   switch (result->kind) {
   case s2x4:
      census_ptr = census_2x4;
      break;
   case s2x2:
      census_ptr = census_2x2;
      break;
   case s4x4:
      census_ptr = census_4x4;
      break;
   case s2x8:
      census_ptr = census_2x8;
      break;
   case s_rigger:
      census_ptr = census_rig;
      break;
   case s_bone:
      census_ptr = census_bone;
      break;
   }

   if (census_ptr) {
      for (int i=0 ; i<=attr::slimit(result); i++) {
         if (result->people[i].id1) row_column_census += census_ptr[i];
      }
   }

   switch (result->kind) {
   case s1x2:
      dstlist = listfor1x1;
      srclist = listfor1x2;
      break;
   case s1x4:
      dstlist = listfor1x2;
      srclist = listfor1x4;
      break;
   case s1x8:
      dstlist = listfor1x4;
      srclist = listfor1x8;
      break;
   case s2x4:
      if (orig_width == 2 && orig_height == 2) {
         // Came in as a 2x2, can it go out as a 2x2?
         if ((row_column_census & 0xFF00) == 0x2200) {
            // Yes.
            dstlist = listfor2x2;
            srclist = listfor2x4;
            break;
         }
         else if ((row_column_census & 0xFF) == 0x55) {
            // No, must go out as a 1x4.
            dstlist = listfor1x4V;
            srclist = listfor2x4V;
            break;
         }
      }
      else if (orig_width == 4 && orig_height == 1) {
         // Came in as a 1x4 oriented the same way, can it go out as a 1x4?
         if ((row_column_census & 0xFF) == 0x55) {
            // Yes.
            dstlist = listfor1x4V;
            srclist = listfor2x4V;
            break;
         }
         else if ((row_column_census & 0xFF00) == 0x2200) {
            // No, must go out as a 2x2.
            dstlist = listfor2x2;
            srclist = listfor2x4;
            break;
         }
      }
      else if (orig_width == 1 && orig_height == 4) {
         // Came in as a 1x4 oriented the other way, go out as a 2x2 if possible.
         if ((row_column_census & 0xFF00) == 0x2200) {
            dstlist = listfor2x2;
            srclist = listfor2x4;
            break;
         }
         else if ((row_column_census & 0xFF) == 0x55) {
            // Must go out as a 1x4.
            dstlist = listfor1x4V;
            srclist = listfor2x4V;
            break;
         }
      }
      else if (orig_width == 3 && orig_height == 2) {
         // Came in as a diamond oriented the same way, prefer a diamond.
         if (finals == 0xAA) {
            compress_map = &expAA_2x4_dmd;
            break;
         }
         else if (finals == 0x55) {
            compress_map = &exp55_2x4_dmd;
            break;
         }
      }
      else if (orig_width == 2 && orig_height == 3) {
         // Came in as a diamond oriented the other way, go out as a 2x2 if possible.
         if ((row_column_census & 0xFF00) == 0x2200) {
            dstlist = listfor2x2;
            srclist = listfor2x4;
            break;
         }
      }
      else if (orig_width < orig_height) {
         // Came in oriented the other way, go out as a 2x2 if possible.
         if ((row_column_census & 0xFF00) == 0x2200) {
            dstlist = listfor2x2;
            srclist = listfor2x4;
            break;
         }
      }
      else {
         // Came in oriented the same way, go out as a 2x2 if possible.
         if ((row_column_census & 0xFF00) == 0x2200) {
            dstlist = listfor2x2;
            srclist = listfor2x4;
            break;
         }
         else if ((row_column_census & 0xFF) == 0x55) {
            // No, must go out as a 1x4.
            dstlist = listfor1x4V;
            srclist = listfor2x4V;
            break;
         }
      }

      break;

   case s4x4:
      if (row_column_census == 0x22222222U) {
         // It can be compressed either way.  Use the incoming elongation to make a choice.
         if (orig_width > orig_height) {
            dstlist = listfor2x4V;
            srclist = listfor4x4V;
         }
         else if (orig_width < orig_height) {
            dstlist = listfor2x4V;
            srclist = listfor4x4;
         }
      }
      else if ((row_column_census & 0xFFFF0000U) == 0x22220000U) {
         dstlist = listfor2x4V;
         srclist = listfor4x4;
      }
      else if ((row_column_census & 0x0000FFFFU) == 0x00002222U) {
         dstlist = listfor2x4V;
         srclist = listfor4x4V;
      }

      break;
   case s2x2:
      if (orig_before_press && orig_before_press->kind == s1x2) {
         dstlist = listfor1x2V;
         if (row_column_census == 0x1111) {
            // People are in the corners, this could compress either way, choose the way that preserves elongation.
            srclist = (orig_height == 2) ? listfor2x2 : listfor2x2V;
         }
         else if ((row_column_census & 0xFF00U) == 0x1100U)
            srclist = listfor2x2;
         else if ((row_column_census & 0xFFU) == 0x11U)
            srclist = listfor2x2V;
      }

      break;
   case s_crosswave:
      switch (finals) {
      case 0x55: compress_map = &exp55_xwv_dmd; break;
      case 0xAA: compress_map = &expAA_xwv_dmd; break;
      case 0x66: compress_map = &exp66_xwv_dmd; break;
      case 0x99: compress_map = &exp99_xwv_dmd; break;
      }

      break;
   case s_rigger:
      if (orig_before_press) {
         if (orig_before_press->kind == sdmd &&
             orig_before_press->rotation == result->rotation &&
             (row_column_census & 0xFFFF000U) == 0x1111000U &&
             ((row_column_census & 0x0F0U) == 0x050U ||   // Handle various unsymmetrical things.
              (row_column_census & 0x0F0U) == 0x080U ||
              (row_column_census & 0x0F0U) == 0x020U) &&
             ((row_column_census & 0x00FU) == 0x001U || (row_column_census & 0x00FU) == 0x004U) &&
             ((row_column_census & 0xF00U) == 0x100U || (row_column_census & 0xF00U) == 0x400U)) {
            dstlist = listfordmd;
            srclist = listforrig;
         }
         else if (orig_before_press->kind == s2x2 && row_column_census == 0x02020A0U)
            compress_map = (orig_before_press->rotation == result->rotation) ? &exp_rig_ctr : &exp_rig_ctrR;
      }

      break;
   case s_bone:
      if (orig_before_press) {
         if (row_column_census == 0x0220154U)
            compress_map = (orig_before_press->rotation == result->rotation) ? &exp_bone_ctr : &exp_bone_ctrR;
         else if (row_column_census == 0x2002802U)
            compress_map = (orig_before_press->rotation == result->rotation) ? &exp_bone_end : &exp_bone_endR;
      }

      break;
   case s_qtag:
      if (finals == 0xCC) {
         compress_map = &expCC_qtg_line; break;  // It's just the center line.
      }
      else if (finals == 0x33) {
         compress_map = &exp33_qtg_box; break;   // It's just the outsides.
      }

      if (orig_before_press && orig_before_press->kind == sdmd && (orig_before_press->rotation ^ result->rotation) == 1) {
         switch (finals) {
         case 0x55: compress_map = &exp55_qtg_dmd; break;
         case 0xAA: compress_map = &expAA_qtg_dmd; break;
         case 0x66: compress_map = &exp66_qtg_dmd; break;
         case 0x99: compress_map = &exp99_qtg_dmd; break;
         }
      }

      break;
   case s2x8:
      if (orig_width == 4 && orig_height == 2 && (row_column_census & 0xFF0000) == 0x440000) {
         // Four on top row, four on bottom, and came from a properly oriented 2x4.
         // Collapse the result to a 2x4.
         dstlist = listfor2x4;
         srclist = listfor2x8;
         break;
      }
      else if (orig_width == 2 && orig_height == 4) {
         // Result must collapse to a 2x4.  ***** do something???
      }

      break;
   case s_dead_concentric:
      result->kind = result->inner.skind;
      result->rotation = result->inner.srotation;
      result->eighth_rotation = 0;
      canonicalize_rotation(result);
      return;
   default:
      fail("Sorry, can't handle this result setup.");
   }

   if (compress_map)
      expand::compress_setup(*compress_map, result);
   else if (srclist) {
      result->kind = (setup_kind) *dstlist++;
      srclist++;
      int rot = (*dstlist++) - (*srclist++);   // Get the rotations.
      result->clear_people();
      int srclim = *srclist++;    // Get the group sizes.
      int dstlim = *dstlist++;
      int wp;
      do {
         wp = 0;
         for (int rp = 0 ; rp < srclim ; rp++) {
            int rpp = rot ? srclim-1-rp : rp;
            if (temp1.people[srclist[rpp]].id1) {
               if (wp == dstlim)
                  fail("Sorry, can't recompress.");
               copy_rot(result, dstlist[wp], &temp1, srclist[rpp], 033*rot);
               wp++;
            }
         }
         if (wp != dstlim) fail("Can't recompress.");
         // Go to the next group.
         srclist += srclim;
         dstlist += dstlim;
      } while (*srclist >= 0);

      result->rotation += rot;
   }
   else
      fail("Can't recompress.");

   canonicalize_rotation(result);
}


static void whole_fixup_mimic(setup *result, const setup *ss) THROW_DECL
{
   if (result->rotation & 1)
      result->result_flags.swap_split_info_fields();

   fixup_mimic(result, result->result_flags.split_info, ~0U, ss);

   // Whatever was going on with splitting is no longer relevant.
   result->result_flags.clear_split_info();

   switch (ss->kind) {
   case s2x2: case s_short6:
      result->result_flags.misc &= ~3;
      result->result_flags.misc |= ss->cmd.prior_elongation_bits & 3;
      break;
   case s1x2: case s1x4: case sdmd:
      result->result_flags.misc &= ~3;
      result->result_flags.misc |= 2 - (ss->rotation & 1);
      break;
   }
}


struct mimic_info{
   int groupsize;
   int lateral;    // 1 if selector is beaus/belles/etc., 0 if leads/trailers/etc.
   int fwd;        // 1 if leads/belles/etc., 0 if beaus/trailers/etc.
   int setup_hint; // Zero, or exactly one bit like MIMIC_SETUP_LINES etc.
};


static void small_mimic_move(setup *ss,
                             mimic_info & MI,
                             uint32 division_code,
                             setup *result) THROW_DECL
{
   setup temp1 = *ss;
   uint32 ilatmask3low = 0;

   tandrec ttt(false, true, false);
   ttt.m_people_per_group = 2;
   ttt.virtual_result = temp1;

   // ****** Problem:  We should check consistency of facing direction
   // when doing leftmost/etc 2 or 4.

   for (int k=attr::slimit(ss); k>=0; k--) {
      ilatmask3low <<= 3;
      uint32 p = ss->people[k].id1;
      int the_real_index = ((p+MI.lateral+(MI.fwd<<1)) >> 1) & 1;

      // Set the person number fields to the identity map.
      ttt.virtual_result.people[k].id1 = (p & ~0700) | (k<<6);
      ilatmask3low |= (p & 1) << 1;
      ilatmask3low ^= (MI.lateral<<1);
      ttt.m_real_saved_people[the_real_index].people[k] = ss->people[k];
      ttt.m_real_saved_people[the_real_index^1].clear_person(k);
   }

   tm_thing *map_search =
      (MI.groupsize == 4) ? maps_isearch_mimicfour :
      (MI.groupsize == 2) ? maps_isearch_mimictwo :
      maps_isearch_twosome;

   while (map_search->outsetup != nothing) {
      if ((map_search->insetup == ss->kind) &&
          map_search->insinglemaskhigh == 0 &&
          map_search->insinglemasklow == 0 &&
          (map_search->ilatmask3high) == 0 &&
          (map_search->ilatmask3low) == ilatmask3low)
         break;

      map_search++;
   }

   if (map_search->outsetup == nothing)
      fail("Can't do this.");

   temp1.clear_people();
   ttt.unpack_us(map_search, 0, 0, &temp1);
   canonicalize_rotation(&temp1);
   if (MI.setup_hint & MIMIC_SETUP_WAVES) temp1.cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;
   temp1.cmd.cmd_misc_flags |= CMD_MISC__PHANTOMS;

   if (division_code != ~0U)
      divided_setup_move(&temp1, division_code, phantest_ok, true, result);
   else
      impose_assumption_and_move(&temp1, result);

   if (result->rotation & 1)
      result->result_flags.swap_split_info_fields();

   fixup_mimic(result, result->result_flags.split_info, division_code, ss);

   // Whatever was going on with splitting is no longer relevant.
   result->result_flags.clear_split_info();
}


void mimic_move(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   selector_kind who = parseptr->options.who;
   bool centers = false;
   int i;
   mimic_info MI;
   MI.lateral = 0; MI.groupsize = 1;
   MI.fwd = 0;

   // See if we have an incoming override of the assumed formation.

   if (!(ss->cmd.cmd_misc3_flags & (CMD_MISC3__TRY_MIMIC_LINES|CMD_MISC3__TRY_MIMIC_COLS)))
      MI.setup_hint = parseptr->concept->arg1;
   else if (ss->cmd.cmd_misc3_flags & CMD_MISC3__TRY_MIMIC_LINES)
      MI.setup_hint = MIMIC_SETUP_LINES;
   else
      MI.setup_hint = MIMIC_SETUP_COLUMNS;

   warning_info saved_warnings = configuration::save_warnings();

   ss->cmd.cmd_misc3_flags &= ~CMD_MISC3__DOING_ENDS;

   uint32 directions;
   uint32 livemask;
   big_endian_get_directions(ss, directions, livemask);
   directions &= 0x55555555;

   if (livemask != (uint32) (1U << ((attr::slimit(ss)<<1)+2)) - 1)
      fail_no_retry("Phantoms not allowed.");

   // What we do is very different for centers/ends vs. other designators.

   int orig_hint;
   int trial_number;
   setup trial_results[2];
   int trial_result_mask = 0;
   error_flag_type err_from_execution;

   switch (who) {
   case selector_centers:
      centers = true;
   case selector_ends:
      if (attr::slimit(ss) > 3) {
         // We need to divide the setup.  Stuff our concept block back into the parse tree.
         // Based on what we know "do_big_concept" does, the following will always be OK,
         // but we take no chances.
         if (ss->cmd.parseptr != parseptr->next)
            fail("Sorry, command tree is too complicated.");
         setup aa = *ss;
         aa.cmd.parseptr = parseptr;

         // This might throw an error; just let it do it, whether it's a "no_retry" or not.
         if (do_simple_split(&aa, split_command_none, result))
            fail("Can't split this.");
         return;
      }

      if (ss->kind == s2x2) {
         if ((MI.setup_hint & MIMIC_SETUP_BOXES) != 0)
            fail("Don't specify this setup.");

         uint32 tbonetest = or_all_people(ss);
         orig_hint = MI.setup_hint;

         for (trial_number=0 ; trial_number<2 ; trial_number++) {

            // If the hint is zero, do this with both LINES and COLUMNS, and demand that they match.
            // First, leave it at zero.  The code below will default to LINES, because it only looks at COLUMNS.

            if (MI.setup_hint & MIMIC_SETUP_COLUMNS)
               tbonetest ^= 011;

            setup a1 = *ss;

            try {
               if (centers) {
                  if ((tbonetest & 010) == 0) {
                     expand::expand_setup(s_2x2_2x4_ctrsb, &a1);
                  }
                  else {
                     if ((tbonetest & 1) != 0)
                        fail("Can't figure out how to make lines or columns.");
                     expand::expand_setup(s_2x2_2x4_ctrs, &a1);
                  }

                  move(&a1, false, result);
               }
               else {
                  if ((tbonetest & 010) == 0) {
                     expand::expand_setup(s_2x2_2x4_endsb, &a1);
                  }
                  else {
                     if ((tbonetest & 1) != 0)
                        fail("Can't figure out how to make lines or columns.");
                     expand::expand_setup(s_2x2_2x4_ends, &a1);
                  }

                  a1.cmd.cmd_misc_flags |= (MI.setup_hint & MIMIC_SETUP_COLUMNS) ?
                     CMD_MISC__VERIFY_COLS : CMD_MISC__VERIFY_LINES;

                  impose_assumption_and_move(&a1, result);
               }

               whole_fixup_mimic(result, ss);
               trial_results[trial_number] = *result;
               trial_result_mask |= 1 << trial_number;
            }
            catch(error_flag_type e) {
               err_from_execution = e;
               if (e == error_flag_no_retry || orig_hint != 0) throw e;
            }

            // If we're going to do it again, we did LINES, switch to COLUMNS.
            // Otherwise, this doesn't matter.
            MI.setup_hint = MIMIC_SETUP_COLUMNS;   // This will make it flip tbonetest.

            // Only loop if no hint was given.
            if (orig_hint != 0)
               break;
         }

         if (orig_hint != 0) {
            // We did it just once, and didn't get any errors--they wouldn't have been silenced.
            // The result is good.
            return;
         }

         // We have attempted it twice.  See what happened.

         if (trial_result_mask == 0) {
            // Both attempts failed.  Report whatever happened.  The text is still there.
            throw err_from_execution;
         }
         else if (trial_result_mask == 2) {
            // Second one only.  It is still in result.
            warn(warn__mimic_ambiguity_resolved);
         }
         else if (trial_result_mask == 1) {
            // First one only.
            *result = trial_results[0];
            warn(warn__mimic_ambiguity_resolved);
         }
         else {
            // We have now done it both ways, and the results are in trial_results.
            // Check them.

            if (trial_results[0].kind != trial_results[1].kind || trial_results[0].rotation != trial_results[1].rotation)
               fail_no_retry("Mimic is ambiguous.");

            for (i=0; i<=attr::slimit(result); i++) {
               if ((trial_results[0].people[i].id1 ^ trial_results[1].people[i].id1) & 0777)
                  fail_no_retry("Mimic is ambiguous.");
            }

            warn(warn__mimic_ambiguity_checked);
         }

         return;
      }
      else if (ss->kind == s1x4) {
         if (MI.setup_hint != 0)
            fail("Don't specify a setup.");

         setup a1 = *ss;

         if (centers)
            expand::expand_setup(s_1x4_1x8_ctrs, &a1);
         else
            expand::expand_setup(s_1x4_1x8_ends, &a1);

         move(&a1, false, result);
         whole_fixup_mimic(result, ss);
         return;
      }
      else
         fail("Can't do this.");
   }

   // Here is where we do the beau/belle stuff.

   switch (who) {
   case selector_beaus:
   case selector_belles:
      MI.lateral = 1;
   case selector_leads:
   case selector_trailers:
      break;
   case selector_leftmosttwo:
   case selector_rightmosttwo:
      MI.lateral = 1;
   case selector_firsttwo:
   case selector_lasttwo:
      MI.groupsize = 2;
      break;
   case selector_leftmostfour:
   case selector_rightmostfour:
      MI.lateral = 1;
   case selector_firstfour:
   case selector_lastfour:
      MI.groupsize = 4;
      break;
   default:
      fail("This designator is not allowed.");
   }

   switch (who) {
   case selector_belles:
   case selector_leads:
   case selector_rightmosttwo:
   case selector_firsttwo:
   case selector_rightmostfour:
   case selector_firstfour:
      MI.fwd = 1;
      break;
   }

   // If the user gave no hint and the setup is a s24, try lines and columns
   // and demand that, if both executions are legal, the results are the same.

   orig_hint = MI.setup_hint;
   if (orig_hint == 0 && ss->kind == s2x4) {
      for (trial_number=0 ; trial_number<2 ; trial_number++) {
         setup a1 = *ss;
         a1.cmd.cmd_misc3_flags &= !(CMD_MISC3__TRY_MIMIC_LINES|CMD_MISC3__TRY_MIMIC_COLS);
         a1.cmd.cmd_misc3_flags |= (trial_number==0) ? CMD_MISC3__TRY_MIMIC_LINES : CMD_MISC3__TRY_MIMIC_COLS;

         try {
            mimic_move(&a1, parseptr, result);
            trial_results[trial_number] = *result;
            trial_result_mask |= 1 << trial_number;
         }
         catch(error_flag_type e) {
            err_from_execution = e;
            if (e == error_flag_no_retry || orig_hint != 0) throw e;
         }
      }

      // We have attempted it twice.  See what happened.

      if (trial_result_mask == 0) {
         // Both attempts failed.  Report whatever happened.  The text is still there.
         throw err_from_execution;
      }
      else if (trial_result_mask == 1) {
         // First one only (lines).
         *result = trial_results[0];
         // User guessed right.  Don't complain.
         //         warn(warn__mimic_ambiguity_resolved);
      }
      else if (trial_result_mask == 2) {
         // Second one only (columns).
         *result = trial_results[1];
         // User guessed right.  Don't complain.
         //         warn(warn__mimic_ambiguity_resolved);
      }
      else {
         // We have now done it both ways, and the results are in trial_results.
         // Check them.

         if (trial_results[0].kind != trial_results[1].kind || trial_results[0].rotation != trial_results[1].rotation)
            fail_no_retry("Mimic is ambiguous.");

         for (i=0; i<=attr::slimit(&trial_results[0]); i++) {
            if ((trial_results[0].people[i].id1 ^ trial_results[1].people[i].id1) & 0777)
               fail_no_retry("Mimic is ambiguous.");
         }

         warn(warn__mimic_ambiguity_checked);
         *result = trial_results[1];
      }

      return;
   }

   // Find out whether this is a call like "counter rotate", which would give deceptive results
   // if we were allowed to do it in smaller setups.  Counter rotate can be done in a 1x2 miniwave,
   // but we don't want to be deceived by that.  Get the "flags2" word of the call definition.
   uint32 flags2 = 0;

   if (ss->cmd.callspec)
      flags2 = ss->cmd.callspec->the_defn.callflagsf;
   else if (ss->cmd.parseptr && ss->cmd.parseptr->concept && ss->cmd.parseptr->concept->kind == marker_end_of_list &&
       ss->cmd.parseptr->call)
      flags2 = ss->cmd.parseptr->call->the_defn.callflagsf;

   // Look for 2-person underlying calls that we can do with a single person.
   // Break up the incoming setup, whatever it is, into 1-person setups.
   // We don't have splitting maps for these, so do it by hand.

   int sizem1 = attr::slimit(ss);
   *result = *ss;
   result->clear_people();
   clear_result_flags(result);

   // But don't do it if user requested any setup, or if this is a [split] counter rotate.
   if (MI.groupsize <= 1 && MI.setup_hint == 0 && !(flags2 & CFLAG2_CAN_BE_ONE_SIDE_LATERAL)) {
      try {
         for (i=0 ; i<=sizem1; i++) {
            if (ss->people[i].id1) {
               setup tt = *ss;
               tt.kind = s1x1;
               tt.rotation = 0;
               copy_person(&tt, 0, ss, i);
               update_id_bits(&tt);
               setup uu;
               small_mimic_move(&tt, MI, ~0U, &uu);
               if (uu.kind != s1x1) fail("Catch me.");
               copy_person(result, i, &uu, 0);
            }
         }

         return;
      }
      catch(error_flag_type e) {
         if (e == error_flag_no_retry) throw e;
      }
   }

   // Well, that didn't work.  Look for 4-person underlying calls that we can do with 2 people.
   // That is, split the setup into 2-person setups, in which people face consistently.
   // The people in each group must not be T-boned.  Also, if the group size is 2 or more,
   // the people in each group must face the same way.

   // The way we will divide the setup and recurse requires that we stuff our concept block
   // back into the parse tree.  Based on what we know "do_big_concept" does, the following
   // will always be OK, but we take no chances.
   if (ss->cmd.parseptr != parseptr->next)
      fail("Sorry, command tree is too complicated.");

   setup aa = *ss;
   aa.cmd.parseptr = parseptr;
   aa.cmd.prior_elongation_bits = 0;
   uint32 division_code = ~0U;

   // If we're already in a 2-person setup, and subdivision failed, we must be able to do it directly.
   if (ss->kind == s1x2 && MI.groupsize <= 2) {
      small_mimic_move(ss, MI, ~0U, result);
      return;
   }

   configuration::restore_warnings(saved_warnings);

   if (MI.groupsize <= 2) {
      try {
         // Don't split into 1x2's if user requested C/L/W.
         if ((MI.setup_hint & (MIMIC_SETUP_LINES|MIMIC_SETUP_WAVES|MIMIC_SETUP_COLUMNS)) == 0 &&
             !((MI.setup_hint & MIMIC_SETUP_BOXES) == 0 && (flags2 & CFLAG2_CAN_BE_ONE_SIDE_LATERAL))) {
            switch (ss->kind) {
            case s1x4:
               division_code = MAPCODE(s1x2,2,MPKIND__SPLIT,0);
               break;
            case s1x8:
               division_code = MAPCODE(s1x2,4,MPKIND__SPLIT,0);
               break;
            case s2x2:
               {
                  int ew = MI.lateral;
                  if (directions == 0x55) ew ^= 1;
                  else if (directions != 0) fail("Can't figure out who should be working with whom.");
                  division_code = (ew & 1) ? spcmap_2x2v : MAPCODE(s1x2,2,MPKIND__SPLIT,1);
               }
               break;
            case s_crosswave:
               division_code = MAPCODE(s1x2,4,MPKIND__NONISOTROPDMD,1);
               break;
            case s_rigger:
               division_code = MAPCODE(s1x2,4,MPKIND__DMD_STUFF,0);
               break;
            case s_qtag:
               division_code = MAPCODE(s1x2,4,MPKIND__DMD_STUFF,1);
               break;
            case s2x4:
               if ((directions == 0x5555 && MI.lateral == 0) || (directions == 0x0000 && MI.lateral == 1)) {
                  division_code = MAPCODE(s1x2,4,MPKIND__SPLIT_OTHERWAY_TOO,0);
               }
               else if ((directions == 0x5555 && MI.lateral == 1) || (directions == 0x0000 && MI.lateral == 0)) {
                  division_code = MAPCODE(s1x2,4,MPKIND__SPLIT,1);
               }
               else if ((directions == 0x4141 && MI.lateral == 0) || (directions == 0x1414 && MI.lateral == 1)) {
                  division_code = MAPCODE(s1x2,4,MPKIND__NONISOTROPDMD,0);
               }

               break;
            }

            if (division_code != ~0U) {
               divided_setup_move(&aa, division_code, phantest_ok, true, result);
               warn(warn__each1x2);
               return;
            }
         }
      }
      catch(error_flag_type e) {
         if (e == error_flag_no_retry) throw e;
      }
   }

   configuration::restore_warnings(saved_warnings);

   if (MI.groupsize == 4) {
      // If we're already in a 4-person setup, do it directly.
      if ((attr::slimit(ss) == 3) && MI.groupsize <= 4) {
         small_mimic_move(ss, MI, ~0U, result);
         return;
      }

      // If user requested boxes, can't do a big division of a 2x4.
      if ((MI.setup_hint & MIMIC_SETUP_BOXES) == 0) {
         try {
            warning_index www = warn__none;

            if (ss->kind == s1x8) {
               www = warn__each1x4;
               division_code = MAPCODE(s1x4,2,MPKIND__SPLIT,0);
            }
            else if (ss->kind == s2x4) {
               www = warn__each1x4;
               division_code = MAPCODE(s1x4,2,MPKIND__SPLIT,1);
            }

            if (division_code != ~0U) {
               divided_setup_move(&aa, division_code, phantest_ok, true, result);
               warn(www);
               return;
            }
         }
         catch(error_flag_type e) {
            if (e == error_flag_no_retry) throw e;
         }
      }
   }

   configuration::restore_warnings(saved_warnings);

   // If we're already in a 4-person setup, and subdivision failed, we must be able to do it directly.
   if (attr::slimit(ss) <= 3) {
      small_mimic_move(ss, MI, ~0U, result);
      return;
   }

   // Well, that didn't work.  Look for 8-person underlying calls that we can do with 4 people.
   // That is, split the setup into 4-person setups, in which people face consistently.
   // The people in each group must not be T-boned.  Also, if the group size is 2 or more,
   // the people in each group must face the same way.

   aa = *ss;
   aa.cmd.parseptr = parseptr;
   aa.cmd.prior_elongation_bits = 0;
   division_code = ~0U;
   warning_index www = warn__none;

   if (MI.groupsize <= 2) {

      // We have the usual problem with splitting a 2x4.  Try this first,
      // with its own catch handler.
      try {
         bool dontdoit = false;

         if (ss->kind == s2x4) {
            // Honor any "leads of lines" specifications.
            if (MI.setup_hint & (MIMIC_SETUP_LINES|MIMIC_SETUP_WAVES)) {
               if (((directions ^ MI.lateral) & 1) == 0)
                  dontdoit = true;
            }
            else if (MI.setup_hint & MIMIC_SETUP_COLUMNS) {
               if (((directions ^ MI.lateral) & 1) != 0)
                  dontdoit = true;
            }

            if (!dontdoit) {
               division_code = MAPCODE(s2x2,2,MPKIND__SPLIT,0);
               www = warn__each2x2;
            }
         }

         if (division_code != ~0U) {
            divided_setup_move(&aa, division_code, phantest_ok, true, result);
            warn(www);
            return;
         }
      }
      catch(error_flag_type e) {
         if (e == error_flag_no_retry) throw e;
      }

      configuration::restore_warnings(saved_warnings);

      try {
         bool dontdoit = false;

         if (ss->kind == s1x8) {
            division_code = MAPCODE(s1x4,2,MPKIND__SPLIT,0);
            www = warn__each1x4;
         }
         else if (ss->kind == s2x4) {
            // Honor any "leads of lines" specifications.
            if (MI.setup_hint & (MIMIC_SETUP_LINES|MIMIC_SETUP_WAVES)) {
               if (((directions ^ MI.lateral) & 1) != 0)
                  dontdoit = true;
            }
            else if (MI.setup_hint & MIMIC_SETUP_COLUMNS) {
               if (((directions ^ MI.lateral) & 1) == 0)
                  dontdoit = true;
            }

            if (!dontdoit) {
               division_code = MAPCODE(s1x4,2,MPKIND__SPLIT,1);
               www = warn__each1x4;
            }
         }
         else if (ss->kind == s_qtag) {
            division_code = MAPCODE(sdmd,2,MPKIND__SPLIT,1);
            www = warn__eachdmd;
         }
         else if (ss->kind == s_ptpd) {
            division_code = MAPCODE(sdmd,2,MPKIND__SPLIT,0);
            www = warn__eachdmd;
         }

         if (division_code != ~0U) {
            divided_setup_move(&aa, division_code, phantest_ok, true, result);
            warn(www);
            return;
         }
      }
      catch(error_flag_type e) {
         if (e == error_flag_no_retry) throw e;
      }
   }

   configuration::restore_warnings(saved_warnings);

   // Just try the whole thing.

   if (MI.groupsize == 4 && ss->kind == s2x4) {
      // First, with no concepts.
      try {
         ss->cmd.cmd_misc_flags |=
            CMD_MISC__EXPLICIT_MATRIX|CMD_MISC__NO_EXPAND_AT_ALL|
            CMD_MISC__PHANTOMS|CMD_MISC__NO_STEP_TO_WAVE;
         small_mimic_move(ss, MI, ~0U, result);
         return;
      }
      catch(error_flag_type e) {
         if (e == error_flag_no_retry) throw e;
      }

      configuration::restore_warnings(saved_warnings);

      // Finally, with an implict "split phantom boxes".
      small_mimic_move(ss, MI, MAPCODE(s2x4,2,MPKIND__SPLIT,0), result);
      return;
   }

   fail("Can't do this \"mimic\" call.");
}


bool process_brute_force_mxn(
   setup *ss,
   parse_block *parseptr,
   void (*backstop)(setup *, parse_block *, setup *),
   setup *result) THROW_DECL
{
   if (ss->cmd.cmd_final_flags.test_finalbits(~0) != 0)
      return false;

   static const tm_thing map_getin_4x4_tidal_wave = {
      {0, 1, 3, 2, 6, 7, 5, 4},     0,02222,     0xFF,         2, 0,  s1x4,  s1x8};
   static const tm_thing map_getin_4x4_tidal_1fl = {
      {0, 6, 1, 7, 3, 5, 2, 4},     0,02222,     0xFF,         2, 0,  s1x4,  s1x8};
   static const tm_thing map_getin_4x4_2x4 = {
      {0, 7, 1, 6, 2, 5, 3, 4},     0,02222,     0xFF,         2, 0,  s2x2,  s2x4};
   static const tm_thing map_getin_4x4_2x8 = {
      {0, 4, 11, 15, 1, 5, 10, 14, 2, 6, 9, 13, 3, 7, 8, 12}, 0,022222222, 0xFFFF, 4, 0,  s2x4,  s2x8};
   static const tm_thing map_getin_2x3_2x6 = {
      {0, 3, 8, 11, 1, 4, 7, 10, 2, 5, 6, 9}, 0,0222222, 0xFFF, 4, 0,  s2x4,  s2x6};

   static const tm_thing map_getin_3x3_tidal_wave = {
      {0, 1, 2, 5, 4, 3},     0,0222,     0xFF,         2, 0,  s1x4,  s1x6};
   static const tm_thing map_getin_3x3_tidal_1fl = {
      {0, 5, 1, 4, 2, 3},     0,0222,     0xFF,         2, 0,  s1x4,  s1x6};
   static const tm_thing map_getin_3x3_2x4 = {
      {0, 5, 1, 4, 2, 3},     0,0222,     0xFF,         2, 0,  s2x2,  s2x3};

   static const tm_thing map_getout_2x2_list[2] = {
      {{3, 2, 4, 1, 5, 0},     0,0222,     0xFF,         2, 3,  s2x2,  s2x3},
      {{4, 3, 5, 2, 6, 1, 7, 0},     0,02222,     0xFF,         2, 3,  s2x2,  s2x4}};

   static const tm_thing map_getout_1x4_together_list[2] = {
      {{0, 5, 1, 4, 2, 3},     0,0222,     0xFF,         2, 0,  s1x4,  s1x6},
      {{0, 6, 1, 7, 3, 5, 2, 4},     0,02222,     0xFF,         2, 0,  s1x4,  s1x8}};

   static const tm_thing map_getout_1x4_apart_list[2] = {
      {{0, 1, 2, 5, 4, 3},           0,02222, 0xFF, 2, 0,  s1x4,  s1x6},
      {{0, 1, 3, 2, 6, 7, 5, 4},     0,02222, 0xFF, 2, 0,  s1x4,  s1x8}};

   static const tm_thing map_getout_2x3_together_normal_list[2] = {
      {{0, 1, 2, 7, 8, 3, 6, 5, 4},             0,0222222, 0xFFF, 3, 0,  s2x3,  s3x3},
      {{0, 10, 9, 1, 11, 8, 2, 5, 7, 3, 4, 6},  0,0222222, 0xFFF, 3, 1,  s2x3,  s3x4}};

   static const tm_thing map_getout_qtag_list[2] = {
      {{0, 5, 8, 9, 1, 4, 7, 10, 2, 3, 6, 11},                 0,022222222, 0xFFFF, 4, 0,  s_qtag,  s3dmd},
      {{0, 7, 11, 12, 1, 6, 10, 13, 2, 5, 9, 14, 3, 4, 8, 15}, 0,022222222, 0xFFFF, 4, 0,  s_qtag,  s4dmd}};

   tandrec ttt(true, false, false);
   const tm_thing *map_ptr;
   uint32 directions;
   uint32 livemask;
   big_endian_get_directions(ss, directions, livemask);

   int rotfix = 0;
   const tm_thing *getout_map;
   int reversal_bits = 0;

   ttt.m_virtual_setup[0].cmd = ss->cmd;
   ttt.m_virtual_setup[0].cmd.cmd_assume.assumption = cr_none;

   if (ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_MXNMASK|INHERITFLAG_NXNMASK) == INHERITFLAGNXNK_4X4) {
      ttt.m_people_per_group = 4;

      switch (ss->kind) {
      case s1x8:
         if (livemask == 0xFFFF && (directions == 0x2882 || directions == 0x8228)) {
            // Tidal wave.
            map_ptr = &map_getin_4x4_tidal_wave;

            if (ttt.pack_us(ss->people, map_ptr, 0, false, false, tandem_key_cpls))
               return false;

            // We now have a 1x4 wave with the first two people in place.  Each of these virtual
            // people represents 4 real people.  Spread them out to the full 4-person wave.

            ttt.m_virtual_setup[0].people[3] = ttt.m_virtual_setup[0].people[0];
            ttt.m_virtual_setup[0].people[3].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            ttt.m_virtual_setup[0].people[2] = ttt.m_virtual_setup[0].people[1];
            ttt.m_virtual_setup[0].people[2].id1 += 0100*map_ptr->limit;    // A new virtual person number.
         }
         else if (livemask == 0xFFFF && (directions == 0xAA00 || directions == 0x00AA)) {
            // Tidal 1FL.
            map_ptr = &map_getin_4x4_tidal_1fl;

            if (ttt.pack_us(ss->people, map_ptr, 0, false, false, tandem_key_cpls))
               return false;

            // We now have a 1x4 2fl with the first two people in place.  Each of these virtual
            // people represents 4 real people.  Spread them out to the full 4-person 2fl.

            ttt.m_virtual_setup[0].people[3] = ttt.m_virtual_setup[0].people[0];
            ttt.m_virtual_setup[0].people[3].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            ttt.m_virtual_setup[0].people[2] = ttt.m_virtual_setup[0].people[1];
            ttt.m_virtual_setup[0].people[2].id1 += 0100*map_ptr->limit;    // A new virtual person number.

            ttt.m_virtual_setup[0].swap_people(1, 3);
         }
         else
            return false;

         break;
      case s2x4:
         if (livemask == 0xFFFF && (directions == 0x55FF || directions == 0xFF55)) {
            map_ptr = &map_getin_4x4_2x4;

            if (ttt.pack_us(ss->people, map_ptr, 0, false, false, tandem_key_cpls))
               return false;

            // We now have a 2x2 box with the first two people in place.  Each of these virtual
            // people represents 4 real people.  Spread them out to the full 4-person box.

            ttt.m_virtual_setup[0].people[3] = ttt.m_virtual_setup[0].people[0];
            ttt.m_virtual_setup[0].people[3].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            ttt.m_virtual_setup[0].people[2] = ttt.m_virtual_setup[0].people[1];
            ttt.m_virtual_setup[0].people[2].id1 += 0100*map_ptr->limit;    // A new virtual person number.

            ttt.m_virtual_setup[0].swap_people(1, 3);
         }
         else
            return false;

         break;
      case s2x8:
         if (((directions ^ 0x5555FFFF) & livemask) == 0 ||
             ((directions ^ 0xFFFF5555) & livemask) == 0) {
            map_ptr = &map_getin_4x4_2x8;

            if (ttt.pack_us(ss->people, map_ptr, 0, false, false, tandem_key_cpls))
               return false;

            // We now have a 2x4 with two people in place.  Each of these virtual
            // people represents 4 real people.  Spread them out.

            ttt.m_virtual_setup[0].swap_people(2, 5);
            ttt.m_virtual_setup[0].swap_people(1, 2);
            ttt.m_virtual_setup[0].swap_people(3, 7);

            if (ttt.m_virtual_setup[0].people[0].id1) {
               ttt.m_virtual_setup[0].people[1] = ttt.m_virtual_setup[0].people[0];
               ttt.m_virtual_setup[0].people[1].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            }

            if (ttt.m_virtual_setup[0].people[2].id1) {
               ttt.m_virtual_setup[0].people[3] = ttt.m_virtual_setup[0].people[2];
               ttt.m_virtual_setup[0].people[3].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            }

            if (ttt.m_virtual_setup[0].people[5].id1) {
               ttt.m_virtual_setup[0].people[4] = ttt.m_virtual_setup[0].people[5];
               ttt.m_virtual_setup[0].people[4].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            }

            if (ttt.m_virtual_setup[0].people[7].id1) {
               ttt.m_virtual_setup[0].people[6] = ttt.m_virtual_setup[0].people[7];
               ttt.m_virtual_setup[0].people[6].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            }
         }
         else
            return false;

         break;
      default:
         return false;
      }
   }
   else if (ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_MXNMASK|INHERITFLAG_NXNMASK) == INHERITFLAGNXNK_3X3) {
      ttt.m_people_per_group = 3;

      switch (ss->kind) {
      case s1x6:
         if (((directions ^ 0x222) & livemask) == 0 || ((directions ^ 0x888) & livemask) == 0) {
            // Tidal wave.
            map_ptr = &map_getin_3x3_tidal_wave;

            if (ttt.pack_us(ss->people, map_ptr, 0, false, false, tandem_key_cpls))
               return false;

            // We now have a 1x4 wave with the first two people in place.  Each of these virtual
            // people represents 4 real people.  Spread them out to the full 4-person wave.

            ttt.m_virtual_setup[0].people[3] = ttt.m_virtual_setup[0].people[0];
            ttt.m_virtual_setup[0].people[3].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            ttt.m_virtual_setup[0].people[2] = ttt.m_virtual_setup[0].people[1];
            ttt.m_virtual_setup[0].people[2].id1 += 0100*map_ptr->limit;    // A new virtual person number.
         }
         else if (((directions ^ 0xA80) & livemask) == 0 || ((directions ^ 0x02A) & livemask) == 0) {
            // Tidal 1FL.
            map_ptr = &map_getin_3x3_tidal_1fl;

            if (ttt.pack_us(ss->people, map_ptr, 0, false, false, tandem_key_cpls))
               return false;

            // We now have a 1x4 2fl with the first two people in place.  Each of these virtual
            // people represents 4 real people.  Spread them out to the full 4-person 2fl.

            ttt.m_virtual_setup[0].people[3] = ttt.m_virtual_setup[0].people[0];
            ttt.m_virtual_setup[0].people[3].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            ttt.m_virtual_setup[0].people[2] = ttt.m_virtual_setup[0].people[1];
            ttt.m_virtual_setup[0].people[2].id1 += 0100*map_ptr->limit;    // A new virtual person number.

            ttt.m_virtual_setup[0].swap_people(1, 3);
         }
         else
            return false;

         break;
      case s2x3:
         if (((directions ^ 0x57F) & livemask) == 0 || ((directions ^ 0xFD5) & livemask) == 0) {
            map_ptr = &map_getin_3x3_2x4;

            if (ttt.pack_us(ss->people, map_ptr, 0, false, false, tandem_key_cpls))
               return false;

            // We now have a 2x2 box with the first two people in place.  Each of these virtual
            // people represents 4 real people.  Spread them out to the full 4-person box.

            ttt.m_virtual_setup[0].people[3] = ttt.m_virtual_setup[0].people[0];
            ttt.m_virtual_setup[0].people[3].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            ttt.m_virtual_setup[0].people[2] = ttt.m_virtual_setup[0].people[1];
            ttt.m_virtual_setup[0].people[2].id1 += 0100*map_ptr->limit;    // A new virtual person number.

            ttt.m_virtual_setup[0].swap_people(1, 3);
         }
         else
            return false;

         break;
      case s2x6:
         if (((directions ^ 0x555FFF) & livemask) == 0 ||
             ((directions ^ 0xFFF555) & livemask) == 0) {
            map_ptr = &map_getin_2x3_2x6;

            if (ttt.pack_us(ss->people, map_ptr, 0, false, false, tandem_key_cpls))
               return false;

            // We now have a 2x4 with two people in place.  Each of these virtual
            // people represents 3 real people.  Spread them out.

            ttt.m_virtual_setup[0].swap_people(2, 5);
            ttt.m_virtual_setup[0].swap_people(1, 2);
            ttt.m_virtual_setup[0].swap_people(3, 7);

            if (ttt.m_virtual_setup[0].people[0].id1) {
               ttt.m_virtual_setup[0].people[1] = ttt.m_virtual_setup[0].people[0];
               ttt.m_virtual_setup[0].people[1].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            }

            if (ttt.m_virtual_setup[0].people[2].id1) {
               ttt.m_virtual_setup[0].people[3] = ttt.m_virtual_setup[0].people[2];
               ttt.m_virtual_setup[0].people[3].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            }

            if (ttt.m_virtual_setup[0].people[5].id1) {
               ttt.m_virtual_setup[0].people[4] = ttt.m_virtual_setup[0].people[5];
               ttt.m_virtual_setup[0].people[4].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            }

            if (ttt.m_virtual_setup[0].people[7].id1) {
               ttt.m_virtual_setup[0].people[6] = ttt.m_virtual_setup[0].people[7];
               ttt.m_virtual_setup[0].people[6].id1 += 0100*map_ptr->limit;    // A new virtual person number.
            }
         }
         else
            return false;
         break;
      default:
         return false;
      }
   }
   else
      return false;

   // Do the call.

   // Clear whatever flag we are preocessing.
   ttt.m_virtual_setup[0].cmd.cmd_final_flags.clear_heritbits(
      ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_MXNMASK|INHERITFLAG_NXNMASK));

   update_id_bits(&ttt.m_virtual_setup[0]);
   backstop(&ttt.m_virtual_setup[0], parseptr, &ttt.virtual_result);
   normalize_setup(&ttt.virtual_result, normalize_before_merge, false);

   // Figure out what happened.

   uint32 aaa;
   uint32 bbb;

   switch (ttt.virtual_result.kind) {
   case s2x2:
      {
         int kid = ttt.virtual_result.people[0].id1 & 0700;
         bool left_column_reversed = (kid >= 0100*map_ptr->limit);
         uint32 otherid = left_column_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;

         if ((ttt.virtual_result.people[3].id1 & 0700) != otherid) {
            // Paired people are horizontal.  Just flip the setup around and recanonicalize.
            rotfix = 1;
            ttt.virtual_result.rotation += rotfix;
            canonicalize_rotation(&ttt.virtual_result);

            // And recompute stuff.
            kid = ttt.virtual_result.people[0].id1 & 0700;
            left_column_reversed = (kid >= 0100*map_ptr->limit);
            otherid = left_column_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;
         }

         // Paired people should now be vertical.

         if ((ttt.virtual_result.people[3].id1 & 0700) != otherid)
            return false;

         // The left column is paired; swap it into position.
         if (left_column_reversed) {
            reversal_bits |= 004;
            ttt.virtual_result.swap_people(0, 3);
         }

         // Check that the right column is also paired, and swap it into position.
         kid = ttt.virtual_result.people[1].id1 & 0700;
         bool right_column_reversed = (kid >= 0100*map_ptr->limit);
         otherid = right_column_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;
         if ((ttt.virtual_result.people[2].id1 & 0700) != otherid)
            return false;

         if (right_column_reversed) {
            reversal_bits |= 040;
            ttt.virtual_result.swap_people(1, 2);
         }

         // Check that the people in each column had similar turning motion and direction.
         aaa = ttt.virtual_result.people[0].id1;
         bbb = ttt.virtual_result.people[3].id1;
         if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
            return false;

         aaa = ttt.virtual_result.people[1].id1;
         bbb = ttt.virtual_result.people[2].id1;
         if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
            return false;

         getout_map = &map_getout_2x2_list[ttt.m_people_per_group-3];
      }

      break;

   case s1x4:
      {
         int kid = ttt.virtual_result.people[0].id1 & 0700;
         bool left_side_reversed = (kid >= 0100*map_ptr->limit);
         uint32 otherid = left_side_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;

         if ((ttt.virtual_result.people[1].id1 & 0700) == otherid) {
            // Paired people are together.

            // The left side is paired; swap it into position.
            if (left_side_reversed) {
               reversal_bits |= 002;
               ttt.virtual_result.swap_people(0, 1);
            }

            // Check that the right side is also paired, and swap it into position.
            kid = ttt.virtual_result.people[3].id1 & 0700;
            bool right_side_reversed = (kid >= 0100*map_ptr->limit);
            otherid = right_side_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;
            if ((ttt.virtual_result.people[2].id1 & 0700) != otherid)
               return false;

            if (right_side_reversed) {
               reversal_bits |= 020;
               ttt.virtual_result.swap_people(2, 3);
            }

            // Check that the people in each side had similar turning motion and direction.
            aaa = ttt.virtual_result.people[0].id1;
            bbb = ttt.virtual_result.people[1].id1;
            if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
               return false;

            aaa = ttt.virtual_result.people[2].id1;
            bbb = ttt.virtual_result.people[3].id1;
            if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
               return false;

            // Put the important people into slots 0 and 1.
            ttt.virtual_result.swap_people(1, 3);

            getout_map = &map_getout_1x4_together_list[ttt.m_people_per_group-3];
         }
         else {
            // Paired people are not together.

            // The left once-removed side is paired; swap it into position.
            if (left_side_reversed) {
               reversal_bits |= 002;
               ttt.virtual_result.swap_people(0, 3);
            }

            // Check that the right once-removed side is also paired, and swap it into position.
            kid = ttt.virtual_result.people[1].id1 & 0700;
            bool right_side_reversed = (kid >= 0100*map_ptr->limit);
            otherid = right_side_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;
            if ((ttt.virtual_result.people[2].id1 & 0700) != otherid)
               return false;

            if (right_side_reversed) {
               reversal_bits |= 020;
               ttt.virtual_result.swap_people(1, 2);
            }

            // Check that the people in each side had similar turning motion and direction.
            aaa = ttt.virtual_result.people[0].id1;
            bbb = ttt.virtual_result.people[3].id1;
            if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
               return false;

            aaa = ttt.virtual_result.people[2].id1;
            bbb = ttt.virtual_result.people[1].id1;
            if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
               return false;

            getout_map = &map_getout_1x4_apart_list[ttt.m_people_per_group-3];
         }
      }

      break;

   case s2x3:
      {
         int kid = ttt.virtual_result.people[0].id1 & 0700;
         bool left_side_reversed = (kid >= 0100*map_ptr->limit);
         uint32 otherid = left_side_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;

         if (!ttt.virtual_result.people[0].id1 || (ttt.virtual_result.people[5].id1 & 0700) == otherid) {
            // This is the only way people can be paired.

            // The left side is paired; swap it into position.
            if (ttt.virtual_result.people[0].id1 && left_side_reversed) {
               reversal_bits |= 004;
               ttt.virtual_result.swap_people(0, 5);
            }

            // Check that the middle group side is also paired, and swap it into position.
            if (ttt.virtual_result.people[1].id1) {
               kid = ttt.virtual_result.people[1].id1 & 0700;
               bool middle_group_reversed = (kid >= 0100*map_ptr->limit);
               otherid = middle_group_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;
               if ((ttt.virtual_result.people[4].id1 & 0700) != otherid)
                  return false;

               if (middle_group_reversed) {
                  reversal_bits |= 040;
                  ttt.virtual_result.swap_people(1, 4);
               }
            }

            // Check that the right side is also paired, and swap it into position.
            if (ttt.virtual_result.people[2].id1) {
               kid = ttt.virtual_result.people[2].id1 & 0700;
               bool right_side_reversed = (kid >= 0100*map_ptr->limit);
               otherid = right_side_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;
               if ((ttt.virtual_result.people[3].id1 & 0700) != otherid)
                  return false;

               if (right_side_reversed) {
                  reversal_bits |= 0400;
                  ttt.virtual_result.swap_people(2, 3);
               }
            }

            // Check that the people in each side had similar turning motion and direction.
            aaa = ttt.virtual_result.people[0].id1;
            bbb = ttt.virtual_result.people[5].id1;
            if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
               return false;

            aaa = ttt.virtual_result.people[1].id1;
            bbb = ttt.virtual_result.people[4].id1;
            if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
               return false;

            aaa = ttt.virtual_result.people[2].id1;
            bbb = ttt.virtual_result.people[3].id1;
            if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
               return false;

            getout_map = &map_getout_2x3_together_normal_list[ttt.m_people_per_group-3];
         }
         else
            return false;
      }

      break;

   case s_qtag:
      {
         int kid = ttt.virtual_result.people[0].id1 & 0700;
         bool top_group_reversed = (kid >= 0100*map_ptr->limit);
         uint32 otherid = top_group_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;

         if (!ttt.virtual_result.people[0].id1 || (ttt.virtual_result.people[1].id1 & 0700) == otherid) {
            // This is the only way people can be paired.

            // The top group is paired; swap it into position.
            if (ttt.virtual_result.people[0].id1 && top_group_reversed) {
               reversal_bits |= 00002;
               ttt.virtual_result.swap_people(0, 1);
            }

            // Check that the right side is also paired, and swap it into position.
            if (ttt.virtual_result.people[3].id1) {
               kid = ttt.virtual_result.people[3].id1 & 0700;
               bool right_side_reversed = (kid >= 0100*map_ptr->limit);
               otherid = right_side_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;
               if ((ttt.virtual_result.people[2].id1 & 0700) != otherid)
                  return false;

               if (right_side_reversed) {
                  reversal_bits |= 00020;
                  ttt.virtual_result.swap_people(3, 2);
               }
            }

            // Check that the bottom group is also paired, and swap it into position.
            if (ttt.virtual_result.people[5].id1) {
               kid = ttt.virtual_result.people[5].id1 & 0700;
               bool bottom_group_reversed = (kid >= 0100*map_ptr->limit);
               otherid = bottom_group_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;
               if ((ttt.virtual_result.people[4].id1 & 0700) != otherid)
                  return false;

               if (bottom_group_reversed) {
                  reversal_bits |= 00200;
                  ttt.virtual_result.swap_people(5, 4);
               }
            }

            // Check that the left side is also paired, and swap it into position.
            if (ttt.virtual_result.people[6].id1) {
               kid = ttt.virtual_result.people[6].id1 & 0700;
               bool left_side_reversed = (kid >= 0100*map_ptr->limit);
               otherid = left_side_reversed ? kid - 0100*map_ptr->limit : kid + 0100*map_ptr->limit;
               if ((ttt.virtual_result.people[7].id1 & 0700) != otherid)
                  return false;

               if (left_side_reversed) {
                  reversal_bits |= 02000;
                  ttt.virtual_result.swap_people(6, 7);
               }
            }

            // Check that the people in each side had similar turning motion and direction.
            aaa = ttt.virtual_result.people[0].id1;
            bbb = ttt.virtual_result.people[1].id1;
            if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
               return false;

            aaa = ttt.virtual_result.people[3].id1;
            bbb = ttt.virtual_result.people[2].id1;
            if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
               return false;

            aaa = ttt.virtual_result.people[5].id1;
            bbb = ttt.virtual_result.people[4].id1;
            if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
               return false;

            aaa = ttt.virtual_result.people[6].id1;
            bbb = ttt.virtual_result.people[7].id1;
            if (((aaa ^ bbb) & (STABLE_ALL_MASK | BIT_PERSON | 0x3F)) != 0)
               return false;

            // Put the important people into slots 0 through 3.
            ttt.virtual_result.swap_people(1, 3);
            ttt.virtual_result.swap_people(2, 5);
            ttt.virtual_result.swap_people(3, 6);

            getout_map = &map_getout_qtag_list[ttt.m_people_per_group-3];
         }
         else
            return false;
      }

      break;

   default:
      return false;
   }

   ttt.unpack_us(getout_map, 0, reversal_bits, result);
   canonicalize_rotation(result);
   warn(warn__brute_force_mxn);
   result->rotation -= rotfix;   // Flip the setup back.
   reinstate_rotation(ss, result);
   return true;
}
