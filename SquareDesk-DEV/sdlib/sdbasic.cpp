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

/* This defines the following functions:
   collision_collector::install_with_collision
   collision_collector::fix_possible_collision
   mirror_this
   do_stability
   check_restriction
   basic_move
*/

#include "sd.h"


/* This file uses a few bogus setups.  They are never allowed to escape:
   s8x8 =
     0   1   2   3  28  24  20  16
     4   5   6   7  29  25  21  17
     8   9  10  11  30  26  22  18
    12  13  14  15  31  27  23  19
    51  55  59  63  47  46  45  44
    50  54  58  62  43  42  41  40
    49  53  57  61  39  38  37  36
    48  52  56  60  35  34  33  32

   sx4dmd or sx4dmdbone =
      4        8       15
   7     5     9    14    12
      6       10       13
              11
   0  1  2  3    19 18 17 16
              27
     29       26       22
  28    30    25    21    23
     31       24       20


   sxequlize =
      4                15
   7     5     9    14    12
      6       10       13
            0 11  8
      1  2  3    19 18 17
           24 27 16
     29       26       22
  28    30    25    21    23
     31                20


   shypergal =

              4 5

               6
      1       3 7      8
           2      10
      0      15 11     9
               14

             13 12


   sx1x6 =
            3
            4
            5
   0  1  2     8  7  6
           11
           10
            9

   sx1x8 =   (actually, this is now a legitimate setup)
               4
               5
               6
               7
   0  1  2  3    11 10  9  8
              15
              14
              13
              12

   sx1x16 =
                     8
                     9
                    ...
                    14
                    15
   0  1  ....  6  7     23 22 ... 17 16
                    31
                    30
                    ...
                    25
                    24

s_tinyhyperbone -- like s_hyperbone, but all 4 trngl4's are on top of each other.

*/





typedef struct {
   int size;                 // how many people in the maps
   // These masks are little-endian.
   uint32 lmask;             // which people are facing E-W in original double-length setup
   uint32 rmask;             /* where in original setup can people be found */
   uint32 cmask;             /* where in original setup have people collided */
   veryshort source[12];     /* where to get the people */
   veryshort map0[12];       /* where to put the north (or east)-facer */
   veryshort map1[12];       /* where to put the south (or west)-facer */
   setup_kind initial_kind;  /* what setup they are collided in */
   setup_kind final_kind;    /* what setup to change it to */
   int rot;                  /* whether to rotate final setup CW */
   warning_index warning;    /* an optional warning to give */
   uint32 assume_key;        // special stuff for checking assumptions in low 16 bits, plus these:
                             // 0x80000000 bit -> dangerous
                             // 0x40000000 bit -> allow partial setup
                             // 0x20000000 bit -> reject if action = merge_c1_phantom_real_couples
                             // 0x10000000 bit -> reject if incoming setup was diamond
                             // 0x08000000 bit -> fail if NO_CUTTING_THROUGH is on
} collision_map;

static collision_map collision_map_table[] = {
   // These items handle various types of "1/2 circulate" calls from 2x4's.

   {4, 0x000000, 0x33, 0x33, {0, 1, 4, 5},         {0, 3, 5, 6},          {1, 2, 4, 7},
    s_crosswave, s1x8,        0, warn__really_no_collision, 0x20000000},   // from lines out
   {4, 0x000000, 0x33, 0x33, {0, 1, 4, 5},         {0, 1, 7, 6},          {3, 2, 4, 5},
    s_crosswave, s1x8,        0, warn__really_no_collision, 0},            // alternative if couples
   {2, 0x000000, 0x11, 0x11, {0, 4},               {0, 5},                {1, 4},
    s_crosswave, s1x8,        0, warn__really_no_collision, 0},   // from lines out, only ends exist
   {2, 0x000000, 0x22, 0x22, {1, 5},               {3, 6},                {2, 7},
    s_crosswave, s1x8,        0, warn__really_no_collision, 3},   // from lines out, only centers exist
   {2, 0x000000, 0x30, 0x30, {4, 5},               {5, 6},                {4, 7},
    s_crosswave, s1x8,        0, warn__really_no_collision, 0},   // more of same
   {2, 0x000000, 0x03, 0x03, {0, 1},               {0, 3},                {1, 2},
    s_crosswave, s1x8,        0, warn__really_no_collision, 0},

   {4, 0x0CC0CC, 0xCC, 0xCC, {2, 3, 6, 7},         {0, 3, 5, 6},          {1, 2, 4, 7},
    s_crosswave, s1x8,        1, warn__really_no_collision, 0x20000000},   // from lines in
   {4, 0x0CC0CC, 0xCC, 0xCC, {2, 3, 6, 7},         {0, 1, 7, 6},          {3, 2, 4, 5},
    s_crosswave, s1x8,        1, warn__really_no_collision, 0},            // alternative if couples
   {2, 0x044044, 0x44, 0x44, {2, 6},               {0, 5},                {1, 4},
    s_crosswave, s1x8,        1, warn__really_no_collision, 0},   // from lines in, only ends exist
   {2, 0x088088, 0x88, 0x88, {3, 7},               {3, 6},                {2, 7},
    s_crosswave, s1x8,        1, warn__really_no_collision, 2},   // from lines in, only centers exist
   {2, 0x0C00C0, 0xC0, 0xC0, {6, 7},               {5, 6},                {4, 7},
    s_crosswave, s1x8,        1, warn__really_no_collision, 0},   // more of same
   {2, 0x00C00C, 0x0C, 0x0C, {2, 3},               {0, 3},                {1, 2},
    s_crosswave, s1x8,        1, warn__really_no_collision, 0},

   {4, 0x000000, 0x0F, 0x0F, {0, 1, 2, 3},         {0, 3, 5, 6},          {1, 2, 4, 7},
    s1x4,        s1x8,        0, warn__really_no_collision, 0},   // more of same

   {4, 0x022022, 0xAA, 0xAA, {1, 3, 5, 7},         {2, 5, 7, 0},          {3, 4, 6, 1},
    s_spindle,   s_crosswave, 0, warn__none, 0},   // from trade by
   {2, 0x022022, 0x22, 0x22, {1, 5},               {2, 7},                {3, 6},
    s_spindle,   s_crosswave, 0, warn__none, 1},   // from trade by with no ends
   {2, 0x000000, 0x88, 0x88, {3, 7},               {5, 0},                {4, 1},
    s_spindle,   s_crosswave, 0, warn__none, 0},   // from trade by with no centers
   {3, 0x000022, 0x2A, 0x08, {1, 3, 5},            {3, 5, 7},             {3, 4, 7},
    s_spindle,   s_crosswave, 0, warn__none, 0},   // from trade by with no left half
   {3, 0x000022, 0xA2, 0x80, {1, 5, 7},            {3, 7, 0},             {3, 7, 1},
    s_spindle,   s_crosswave, 0, warn__none, 0},   // from trade by with no right half

   {3, 0x000000, 0x7, 0x2, {0, 1, 2},              {0, 1, 2},             {0, 3, 2},
    s1x3,   s1x4, 0, warn__none, 0},

   {6, 0x0880CC, 0xDD, 0x88, {0, 2, 3, 4, 6, 7},   {7, 0, 1, 3, 4, 6},    {7, 0, 2, 3, 4, 5},
    s_crosswave, s3x1dmd,     1, warn__none, 0},   // from 3&1 lines w/ centers in
   {6, 0x000044, 0x77, 0x22, {0, 1, 2, 4, 5, 6},   {0, 1, 3, 4, 6, 7},    {0, 2, 3, 4, 5, 7},
    s_crosswave, s3x1dmd,     0, warn__none, 0},   // from 3&1 lines w/ centers out
   {6, 0x0440CC, 0xEE, 0x44, {1, 2, 3, 5, 6, 7},   {7, 0, 2, 3, 5, 6},    {7, 1, 2, 3, 4, 6},
    s_crosswave, s3x1dmd,     1, warn__none, 0},   // from 3&1 lines w/ ends in
   {6, 0x000088, 0xBB, 0x11, {0, 1, 3, 4, 5, 7},   {0, 2, 3, 5, 6, 7},    {1, 2, 3, 4, 6, 7},
    s_crosswave, s1x3dmd,     0, warn__none, 0},   // from 3&1 lines w/ ends out
   {4, 0x088088, 0x99, 0x99, {0, 3, 4, 7},         {0, 2, 5, 7},          {1, 3, 4, 6},
    s_crosswave, s_crosswave, 0, warn__none, 0},   // from inverted lines w/ centers in
   {4, 0x044044, 0x66, 0x66, {1, 2, 5, 6},         {6, 0, 3, 5},          {7, 1, 2, 4},
    s_crosswave, s_crosswave, 1, warn__ctrs_stay_in_ctr, 0}, // from inverted lines w/ centers out

   // Collision after 1/2 circulate from C1 phantoms, going to a thar.
   {4, 0x044044, 0x55, 0x55, {0, 2, 4, 6},         {0, 2, 5, 7},          {1, 3, 4, 6},
    s_thar, s_thar, 0, warn__none, 0},

   // Collision after circulate from lines all facing same way.
   {4, 0x000000, 0x0F, 0x0F,  {0, 1, 2, 3},         {0, 2, 4, 6},          {1, 3, 5, 7},
    s2x4,        s2x8,        0, warn__none, 0},
   {4, 0x000000, 0xF0, 0xF0,  {4, 5, 6, 7},         {9, 11, 13, 15},       {8, 10, 12, 14},
    s2x4,        s2x8,        0, warn__none, 0},
   // Collision after checkmate from a 8-chain
   {4, 0x000000, 0x66, 0x66,  {1, 2, 5, 6},         {0, 2, 5, 7},          {1, 3, 4, 6},
    s2x4,        s2x4,        0, warn__none, 0},

   // Collision after column circulate from 3/4 box or similar 3x1-column-type things.
   {6, 0x0880DD, 0xDD, 0x88,  {0, 2, 3, 4, 6, 7},   {10, 3, 0, 2, 11, 9},  {10, 3, 1, 2, 11, 8},
    s2x4,        s4x4,        0, warn__none, 0},
   {6, 0x0110BB, 0xBB, 0x11,  {0, 1, 3, 4, 5, 7},   {12, 15, 1, 2, 7, 9},  {10, 15, 1, 4, 7, 9},
    s2x4,        s4x4,        0, warn__none, 0},
   {6, 0x022077, 0x77, 0x22,  {0, 1, 2, 4, 5, 6},   {10, 13, 3, 2, 7, 11}, {10, 15, 3, 2, 5, 11},
    s2x4,        s4x4,        0, warn__none, 0},
   {6, 0x0440EE, 0xEE, 0x44,  {1, 2, 3, 5, 6, 7},   {15, 14, 1, 7, 11, 9}, {15, 3, 1, 7, 6, 9},
    s2x4,        s4x4,        0, warn__none, 0},
   // These are used for exchange the boxes 1/4 from magic columns, and for unwrap from interlocked diamonds.
   {6, 0x011077, 0x77, 0x11,  {0, 1, 2, 4, 5, 6},   {12, 15, 3, 2, 7, 11}, {10, 15, 3, 4, 7, 11},
    s2x4,        s4x4,        0, warn__none, 0},
   {6, 0x0880EE, 0xEE, 0x88,  {1, 2, 3, 5, 6, 7},   {15, 3, 0, 7, 11, 9},  {15, 3, 1, 7, 11, 8},
    s2x4,        s4x4,        0, warn__none, 0},
   {6, 0x0220BB, 0xBB, 0x22,  {0, 1, 3, 4, 5, 7},   {10, 13, 1, 2, 7, 9},  {10, 15, 1, 2, 5, 9},
    s2x4,        s4x4,        0, warn__none, 0},
   {6, 0x0440DD, 0xDD, 0x44,  {0, 2, 3, 4, 6, 7},   {10, 14, 1, 2, 11, 9}, {10, 3, 1, 2, 6, 9},
    s2x4,        s4x4,        0, warn__none, 0},
   // These arise in common spot circulate from columns in which the ends are trucked.
   {4, 0x0880CC, 0xCC, 0x88,  {2, 3, 6, 7},         {3, 0, 11, 9},         {3, 1, 11, 8},
    s2x4,        s4x4,        0, warn__none, 0},
   {4, 0x011033, 0x33, 0x11,  {0, 1, 4, 5},         {12, 15, 2, 7},        {10, 15, 4, 7},
    s2x4,        s4x4,        0, warn__none, 0},
   // These arise in common spot exchange 1/4 from columns in which the ends are trucked.
   {4, 0x0880AA, 0xAA, 0x88,  {1, 3, 5, 7},         {15, 0, 7, 9},         {15, 1, 7, 8},
    s2x4,        s4x4,        0, warn__none, 0},
   {4, 0x011055, 0x55, 0x11,  {0, 2, 4, 6},         {12, 3, 2, 11},        {10, 3, 4, 11},
    s2x4,        s4x4,        0, warn__none, 0},

   // This one is marked as dangerous, meaning it can't be used from
   // merge_setups, but can be used when normalizing the result of basic_move.
   {6, 0x044044, 0x77, 0x44, {0, 1, 2, 4, 5, 6},   {0, 1, 2, 4, 5, 7},   {0, 1, 3, 4, 5, 6},
    s_crosswave, s_crosswave, 0, warn__ctrs_stay_in_ctr, UINT32_C(0x80000000)}, // From inverted lines w/ centers out.

   {4, 0x044044, 0x55, 0x55, {0, 2, 4, 6},         {0, 2, 5, 7},          {1, 3, 4, 6},
    s_crosswave, s_crosswave, 0, warn__ctrs_stay_in_ctr, 0},      // from trade-by w/ ctrs 1/4 out
   // This was put in so that "1/2 circulate" would work from lines in with centers counter rotated.
   {4, 0x088088, 0xAA, 0xAA, {1, 3, 5, 7},         {0, 2, 5, 7},          {1, 3, 4, 6},
    s_crosswave, s_crosswave, 0, warn__ctrs_stay_in_ctr, 0},      // from lines in and centers face
   {6, 0x000082, 0xAAA, 0x820, {1, 3, 5, 7, 9, 11},{3, 4, 6, 7, 0, 1},    {3, 4, 5, 7, 0, 2},
    s3dmd,       s3x1dmd,     0, warn__none, 0},                  // from trade by with some outside quartered out
   {6, 0x000088, 0x0DD, 0x044, {3, 4, 6, 7, 0, 2}, {3, 4, 6, 7, 0, 1},    {3, 4, 5, 7, 0, 2},
    s3x1dmd,     s3x1dmd,     0, warn__none, 0},                  // Same.
   {6, 0x000088, 0x0EE, 0x022, {3, 5, 6, 7, 1, 2}, {3, 5, 6, 7, 0, 2},    {3, 4, 6, 7, 1, 2},
    s3x1dmd,     s3x1dmd,     0, warn__none, 0},                  // Same.
   // Phantom 1/2 circulate.
   {6, 0x000000, 0x077, 0x022, {0, 1, 2, 4, 5, 6}, {0, 1, 2, 4, 7, 6},    {0, 3, 2, 4, 5, 6},
    s3x1dmd,     s1x8,        0, warn__none, 0},
   {6, 0x088088, 0xBB, 0x88, {0, 1, 3, 4, 5, 7},   {0, 1, 2, 4, 5, 7},    {0, 1, 3, 4, 5, 6},
    s_crosswave, s_crosswave, 0, warn__none, 0},
   {6, 0x0000CC, 0xDD, 0x11, {0, 2, 3, 4, 6, 7},   {0, 2, 3, 5, 6, 7},    {1, 2, 3, 4, 6, 7},
    s_crosswave, s_crosswave, 0, warn__none, 0},
   {6, 0x0000CC, 0xEE, 0x22, {1, 2, 3, 5, 6, 7},   {0, 2, 3, 5, 6, 7},    {1, 2, 3, 4, 6, 7},
    s_crosswave, s_crosswave, 0, warn__none, 0},
   {6, 0x000000,  077, 011,  {0, 1, 2, 3, 4, 5},   {0, 3, 2, 5, 7, 6},    {1, 3, 2, 4, 7, 6},
    s1x6,        s1x8,        0, warn__none, 0},
   {6, 0x000000,  077, 022,  {0, 1, 2, 3, 4, 5},   {0, 1, 2, 4, 7, 6},    {0, 3, 2, 4, 5, 6},
    s1x6,        s1x8,        0, warn__none, 0x20000000},
   {6, 0x000000,  077, 022,  {0, 1, 2, 3, 4, 5},   {0, 3, 2, 4, 5, 6},    {0, 1, 2, 4, 7, 6},
    s1x6,        s1x8,        0, warn__really_no_collision, 0},            // alternative if couples
   {6, 0x000000,  077, 044,  {0, 1, 2, 3, 4, 5},   {0, 1, 3, 4, 5, 6},    {0, 1, 2, 4, 5, 7},
    s1x6,        s1x8,        0, warn__none, 0},
   {4, 0x000000,  055, 055,  {0, 2, 3, 5},         {0, 3, 5, 6},          {1, 2, 4, 7},
    s1x6,        s1x8,        0, warn__none, 0},
   // For tally ho from 5&3 lines.
   {5, 0x000000,  067, 064,  {0, 1, 2, 4, 5},      {0, 1, 3, 5, 6},       {0, 1, 2, 4, 7},
    s1x6,        s1x8,        0, warn__none, 0},
   {5, 0x000000,  076, 046,  {1, 2, 3, 4, 5},      {0, 3, 4, 5, 6},       {1, 2, 4, 5, 7},
    s1x6,        s1x8,        0, warn__none, 0},

   // These items handle parallel lines with people wedged on one end, and hence handle flip or cut the hourglass.
   {6, 0x000000, 0x77, 0x11, {0, 1, 2, 4, 5, 6},   {0, 2, 3, 7, 8, 9},    {1, 2, 3, 6, 8, 9},
    s2x4,        s2x6,        0, warn__none, 0},
   {6, 0x000000, 0xEE, 0x88, {1, 2, 3, 5, 6, 7},   {2, 3, 4, 8, 9, 11},   {2, 3, 5, 8, 9, 10},
    s2x4,        s2x6,        0, warn__none, 0},
   {6, 0x000000, 0x7E, 0x18, {1, 2, 3, 4, 5, 6},   {2, 3, 4, 7, 8, 9},    {2, 3, 5, 6, 8, 9},
    s2x4,        s2x6,        0, warn__none, 0},
   {6, 0x000000, 0xE7, 0x81, {0, 1, 2, 5, 6, 7},   {0, 2, 3, 8, 9, 11},   {1, 2, 3, 8, 9, 10},
    s2x4,        s2x6,        0, warn__none, 0},

   // These items handle single lines with people wedged on one end, and hence handle flip or cut the diamond.
   {3, 0x000000, 0x0B, 0x01, {0, 1, 3},            {0, 2, 5},             {1, 2, 5},
    s1x4,        s1x6,        0, warn__none, 0},
   {3, 0x000000, 0x0E, 0x04, {1, 2, 3},            {2, 4, 5},             {2, 3, 5},
    s1x4,        s1x6,        0, warn__none, 0},
   // These items handle single diamonds with people wedged on one end, and hence handle diamond circulate.
   {3, 0x00000A, 0x0E, 0x04, {1, 2, 3},            {2, 4, 5},             {2, 3, 5},
    sdmd,    s_1x2dmd,        0, warn__none, 0},
   {3, 0x00000A, 0x0B, 0x01, {0, 1, 3},            {0, 2, 5},             {1, 2, 5},
    sdmd,    s_1x2dmd,        0, warn__none, 0},
   // These items handle columns with people wedged everywhere, and hence handle unwraps of facing diamonds etc.
   {4, 0x055055, 0x55, 0x55, {0, 2, 4, 6},         {12, 14, 2, 11},       {10, 3, 4, 6},
    s2x4,        s4x4,        0, warn__none, 0x40000000},
   {4, 0x0AA0AA, 0xAA, 0xAA, {1, 3, 5, 7},         {13, 0, 7, 9},         {15, 1, 5, 8},
    s2x4,        s4x4,        0, warn__none, 0x40000000},
   // These items handle columns with people wedged in clumps, and hence handle gravitate from lefties etc.
   {4, 0x033033, 0x33, 0x33, {0, 1, 4, 5},         {12, 13, 2, 7},        {10, 15, 4, 5},
    s2x4,        s4x4,        0, warn__none, 0x40000000},
   {4, 0x0CC0CC, 0xCC, 0xCC, {2, 3, 6, 7},         {14, 0, 11, 9},        {3, 1, 6, 8},
    s2x4,        s4x4,        0, warn__none, 0x40000000},
   // Collision on ends of an "erase".
   {1, 0x000000, 0x02, 0x02, {1},                  {3},                   {2},
    s1x2,        s1x4,        0, warn__none, 0},
   {1, 0x000000, 0x01, 0x01, {0},                  {0},                   {1},
    s1x2,        s1x4,        0, warn__none, 0},

   // These items handle colliding 2FL-type circulates.
   {2, 0x000000, 0x03, 0x03, {0, 1},               {0, 2},                {1, 3},
    s2x4,        s2x8,        0, warn__none, 0},
   {2, 0x000000, 0x0C, 0x0C, {2, 3},               {4, 6},                {5, 7},
    s2x4,        s2x8,        0, warn__none, 0},
   {2, 0x000000, 0x30, 0x30, {5, 4},               {11, 9},               {10, 8},
    s2x4,        s2x8,        0, warn__none, 0},
   {2, 0x000000, 0xC0, 0xC0, {7, 6},               {15, 13},              {14, 12},
    s2x4,        s2x8,        0, warn__none, 0},

   // The warning "warn_bad_collision" in the warning field is special -- it means give that warning if it appears illegal.
   // If it doesn't appear illegal, don't say anything at all (other than the usual "take right hands".)
   // If anything appears illegal but does not have "warn_bad_collision" in this field, it is an ERROR.
   {4, 0x000000, 0xAA, 0xAA, {1, 3, 5, 7},        {2, 6, 11, 15},        {3, 7, 10, 14},
    s2x4,        s2x8,        0, warn_bad_collision, 0},
   {4, 0x000000, 0x55, 0x55, {0, 2, 4, 6},        {0, 4, 9, 13},         {1, 5, 8, 12},
    s2x4,        s2x8,        0, warn_bad_collision, 0},
   {4, 0x000000, 0x33, 0x33, {0, 1, 4, 5},        {0, 2, 9, 11},         {1, 3, 8, 10},
    s2x4,        s2x8,        0, warn_bad_collision, 0},
   {4, 0x000000, 0xCC, 0xCC, {2, 3, 6, 7},        {4, 6, 13, 15},        {5, 7, 12, 14},
    s2x4,        s2x8,        0, warn_bad_collision, 0},
   // Next two are unsymmetrical.
   {4, 0x000000, 0xC3, 0xC3, {0, 1, 6, 7},        {0, 2, 13, 15},        {1, 3, 12, 14},
    s2x4,        s2x8,        0, warn_bad_collision, 0},
   {4, 0x000000, 0x3C, 0x3C, {2, 3, 4, 5},        {4, 6, 9, 11},         {5, 7, 8, 10},
    s2x4,        s2x8,        0, warn_bad_collision, 0},

   // These items handle various types of "1/2 circulate" calls from 2x2's.
   {2, 0x000000, 0x05, 0x05, {0, 2},               {0, 3},                {1, 2},
    sdmd,        s1x4,        0, warn__none, 0},                  // From couples out if it went to diamond.
   {1, 0x000000, 0x01, 0x01, {0},                  {0},                   {1},
    sdmd,        s1x4,        0, warn__none, 0},                  // Same, but with missing people.
   {1, 0x000000, 0x04, 0x04, {2},                  {3},                   {2},
    sdmd,        s1x4,        0, warn__none, 0},                  // same, but with missing people.
   {2, 0x000000, 0x05, 0x05, {0, 2},               {0, 3},                {1, 2},
    s1x4,        s1x4,        0, warn__really_no_collision, 0},   // From couples out if it went to line.
   {1, 0x000000, 0x01, 0x01, {0},                  {0},                   {1},
    s1x4,        s1x4,        0, warn__really_no_collision, 0},   // Same, but with missing people.
   {1, 0x000000, 0x04, 0x04, {2},                  {3},                   {2},
    s1x4,        s1x4,        0, warn__really_no_collision, 0},   // Same, but with missing people.
   {2, 0x00A00A, 0x0A, 0x0A, {1, 3},               {0, 3},                {1, 2},
    sdmd,        s1x4,        1, warn__none, 0},                  // From couples in if it went to diamond.
   {2, 0x000000, 0x0A, 0x0A, {1, 3},               {0, 3},                {1, 2},
    s1x4,        s1x4,        0, warn__really_no_collision, 0},   // From couples in if it went to line.
   {2, 0x000000, 0x06, 0x06, {1, 2},               {0, 3},                {1, 2},
    s1x4,        s1x4,        0, warn__none, 0},                  // From "head pass thru, all split circulate".
   {2, 0x000000, 0x09, 0x09, {0, 3},               {0, 3},                {1, 2},
    s1x4,        s1x4,        0, warn__none, 0},                  // From "head pass thru, all split circulate".

   // These items handle single-spot collisions in a 1x4 from a T-boned 2x2.
   {3, 0x000000, 0x07, 0x04, {0, 1, 2},            {0, 1, 3},             {0, 1, 2},
    s1x4,        s1x4,        0, warn__none, 0},                  // From nasty T-bone.
   {3, 0x000000, 0x0D, 0x01, {0, 2, 3},            {0, 2, 3},             {1, 2, 3},
    s1x4,        s1x4,        0, warn__none, 0},                  // From nasty T-bone.
   {3, 0x000000, 0x0B, 0x08, {0, 1, 3},            {0, 1, 3},             {0, 1, 2},
    s1x4,        s1x4,        0, warn__none, 0},                  // From nasty T-bone.
   {3, 0x000000, 0x0E, 0x02, {1, 2, 3},            {0, 2, 3},             {1, 2, 3},
    s1x4,        s1x4,        0, warn__none, 0},                  // From nasty T-bone.

   // These items handle "1/2 split trade circulate" from 2x2's.
   // They also do "switch to a diamond" when the ends come to the same spot in the center.
   // They don't allow NO_CUTTING_THROUGH.
   {3, 0x008008, 0x0D, 0x08, {0, 2, 3},            {0, 2, 1},             {0, 2, 3},
    sdmd,        sdmd,        0, warn_bad_collision, UINT32_C(0x08000000)},
   {3, 0x002002, 0x07, 0x02, {0, 2, 1},            {0, 2, 1},             {0, 2, 3},
    sdmd,        sdmd,        0, warn_bad_collision, UINT32_C(0x08000000)},

   // If phantoms, and only diamond centers of result are present (and colliding), use these.
   // Tests are t62 and lo03.
   // Reject these two if incoming setup is diamond.
   {1, 0x008008, 0x08, 0x08, {3},            {3},             {2},
    sdmd,        s1x4,        1, warn_bad_collision, UINT32_C(0x10000000)},
   {1, 0x002002, 0x02, 0x02, {1},            {0},             {1},
    sdmd,        s1x4,        1, warn_bad_collision, UINT32_C(0x10000000)},
   // Use these instead.
   {1, 0x008008, 0x08, 0x08, {3},            {1},             {3},
    sdmd,        sdmd,        0, warn_bad_collision, 0},
   {1, 0x002002, 0x02, 0x02, {1},            {1},             {3},
    sdmd,        sdmd,        0, warn_bad_collision, 0},

   // These items handle various types of "circulate" calls from 2x2's.
   {2, 0x009009, 0x09, 0x09, {0, 3},               {7, 5},                {6, 4},
    s2x2,        s2x4,        1, warn_bad_collision, 0},   // from box facing all one way
   {2, 0x006006, 0x06, 0x06, {1, 2},               {0, 2},                {1, 3},
    s2x2,        s2x4,        1, warn_bad_collision, 0},   // we need all four cases
   {2, 0x000000, 0x0C, 0x0C, {3, 2},               {7, 5},                {6, 4},
    s2x2,        s2x4,        0, warn_bad_collision, 0},   // sigh
   {2, 0x000000, 0x03, 0x03, {0, 1},               {0, 2},                {1, 3},
    s2x2,        s2x4,        0, warn_bad_collision, 0},   // sigh
   {2, 0x000000, 0x09, 0x09, {0, 3},               {0, 7},                {1, 6},
    s2x2,        s2x4,        0, warn_bad_collision, 0},   // from "inverted" box
   {2, 0x000000, 0x06, 0x06, {1, 2},               {2, 5},                {3, 4},
    s2x2,        s2x4,        0, warn_bad_collision, 0},   // we need all four cases
   {2, 0x003003, 0x03, 0x03, {1, 0},               {0, 7},                {1, 6},
    s2x2,        s2x4,        1, warn_bad_collision, 0},   // sigh
   {2, 0x00C00C, 0x0C, 0x0C, {2, 3},               {2, 5},                {3, 4},
    s2x2,        s2x4,        1, warn_bad_collision, 0},   // sigh

   // This handles circulate from a starting DPT.
   {4, 0x066066, 0x66, 0x66, {1, 2, 5, 6},         {7, 0, 2, 5},          {6, 1, 3, 4},
    s2x4,        s2x4,        1, warn_bad_collision, 0},

   // These items handle horrible lockit collisions in the middle
   // (from inverted lines, for example).
   // Of course, such a collision is ILLEGAL in the center of a lockit or fan the top.
   // (This sort of thing was called at NACC in 1996, but is simply bogus now.)
   // However, we *DO* allow it when reassembling "own the anyone" operations.
   {2, 0x000000, 0x06, 0x06, {1, 2},               {3, 5},                {2, 4},
    s1x4,        s1x8,        0, warn_bad_collision, 0},
   {2, 0x000000, 0x09, 0x09, {0, 3},               {0, 6},                {1, 7},
    s1x4,        s1x8,        0, warn_bad_collision, 0},
   {2, 0x000000, 0x03, 0x03, {0, 1},               {0, 3},                {1, 2},
    s1x4,        s1x8,        0, warn_bad_collision, 0},
   {2, 0x000000, 0x0C, 0x0C, {3, 2},               {6, 5},                {7, 4},
    s1x4,        s1x8,        0, warn_bad_collision, 0},

   {3, 0x000000, 0x0D, 0x08, {0, 3, 2},               {0, 5, 3},                {0, 4, 3},
    s1x4,        s1x6,        0, warn_bad_collision, 0},
   {3, 0x000000, 0x07, 0x02, {0, 1, 2},               {0, 1, 3},                {0, 2, 3},
    s1x4,        s1x6,        0, warn_bad_collision, 0},

   {6, 0x000033, 0xBB, 0x88, {0, 1, 3, 4, 5, 7},   {0, 1, 3, 4, 5, 6},    {0, 1, 2, 4, 5, 7},
    s_qtag,      s_qtag,      0, warn_bad_collision, 0},

   // These items handle circulate in a short6, and hence handle collisions in 6X2 acey deucey.

   {6, 022,  077,  055,  {0, 1, 2, 3, 4, 5},       {0, 2, 3, 6, 7, 9},    {1, 2, 4, 5, 7, 8},
    s_short6,    sdeep2x1dmd, 0, warn__none, 0x40000000},
   {4, 000,  055,  044,  {0, 2, 3, 5},       {1, 2, 5, 7},    {1, 3, 5, 6},
    s_short6,    s2x4,        0, warn__none, 0x40000000},
   {4, 000,  055,  011,  {0, 2, 3, 5},       {0, 2, 5, 6},    {1, 2, 4, 6},
    s_short6,    s2x4,        0, warn__none, 0x40000000},

   {5, 022,  073,  001,  {0, 1, 3, 4, 5},          {0, 2, 6, 7, 8},       {1, 2, 6, 7, 8},
    s_short6,    sdeep2x1dmd, 0, warn__none, 0},
   {5, 022,  067,  040,  {0, 1, 2, 4, 5},          {1, 2, 3, 7, 9},       {1, 2, 3, 7, 8},
    s_short6,    sdeep2x1dmd, 0, warn__none, 0},
   {5, 022,  076,  004,  {1, 2, 3, 4, 5},          {2, 3, 6, 7, 8},       {2, 4, 6, 7, 8},
    s_short6,    sdeep2x1dmd, 0, warn__none, 0},
   {5, 022,  037,  010,  {0, 1, 2, 3, 4},          {1, 2, 3, 6, 7},       {1, 2, 3, 5, 7},
    s_short6,    sdeep2x1dmd, 0, warn__none, 0},

   {4, 0x012012, 0x36, 0x12, {1, 2, 4, 5},         {6, 0, 3, 4},          {7, 0, 2, 4},
    s_short6,    s_rigger,    1, warn__none, 0},
   {4, 0x012012, 0x1B, 0x12, {1, 0, 4, 3},         {6, 5, 3, 1},          {7, 5, 2, 1},
    s_short6,    s_rigger,    1, warn__none, 0},

   // These 4 items handle more 2x2 stuff, including the "special drop in"
   // that makes chain reaction/motivate etc. work.
   {2, 0x005005, 0x05, 0x05, {0, 2},               {7, 2},                {6, 3},
    s2x2,        s2x4,        1, warn__none, 0},
   {2, 0x00A00A, 0x0A, 0x0A, {1, 3},               {0, 5},                {1, 4},
    s2x2,        s2x4,        1, warn__none, 0},
   {3, 0x00800B, 0x0B, 0x08, {0, 1, 3},            {6, 1, 5},             {6, 1, 4},
    s2x2,        s2x4,        1, warn__none, 0},
   {3, 0x00200E, 0x0E, 0x02, {1, 2, 3},            {0, 2, 5},             {1, 2, 5},
    s2x2,        s2x4,        1, warn__none, 0},
   {3, 0x00100B, 0x0B, 0x01, {0, 1, 3},            {7, 1, 5},             {6, 1, 5},
    s2x2,        s2x4,        1, warn__none, 0},
   {3, 0x00400E, 0x0E, 0x04, {1, 2, 3},            {1, 2, 5},             {1, 3, 5},
    s2x2,        s2x4,        1, warn__none, 0},

   // These 2 in particular are what make a crossfire from waves go to a 2x4 instead of a 2x3.
   // If they are changed to give a 2x3, test lg02 fails on a "single presto" from a tidal wave.
   {2, 0x000000, 0x05, 0x05, {0, 2},               {0, 5},                {1, 4},
    s2x2,        s2x4,        0, warn__none, 0},
   {2, 0x000000, 0x0A, 0x0A, {1, 3},               {2, 7},                {3, 6},
    s2x2,        s2x4,        0, warn__none, 0},

   // Same, but with missing people.
   {1, 0x004004, 0x04, 0x04, {2},                  {2},                   {3},
    s2x2,        s2x4,        1, warn__none, 0},
   {1, 0x001001, 0x01, 0x01, {0},                  {7},                   {6},
    s2x2,        s2x4,        1, warn__none, 0},
   {1, 0x008008, 0x08, 0x08, {3},                  {5},                   {4},
    s2x2,        s2x4,        1, warn__none, 0},
   {1, 0x002002, 0x02, 0x02, {1},                  {0},                   {1},
    s2x2,        s2x4,        1, warn__none, 0},
   {1, 0x000000, 0x04, 0x01, {2},                  {5},                   {4},
    s2x2,        s2x4,        0, warn__none, 0},
   {1, 0x000000, 0x01, 0x01, {0},                  {0},                   {1},
    s2x2,        s2x4,        0, warn__none, 0},
   {1, 0x000000, 0x08, 0x08, {3},                  {7},                   {6},
    s2x2,        s2x4,        0, warn__none, 0},
   {1, 0x000000, 0x02, 0x02, {1},                  {2},                   {3},
    s2x2,        s2x4,        0, warn__none, 0},

   // Unsymmetrical collisions.
   {3, 0x00200B, 0x0B, 0x02, {0, 1, 3},            {6, 0, 5},             {6, 1, 5},
    s2x2,        s2x4,        1, warn__none, 0},
   {3, 0x00400D, 0x0D, 0x04, {0, 2, 3},            {6, 2, 5},             {6, 3, 5},   
    s2x2,        s2x4,        1, warn__none, 0},

   // Same spot at ends of bone (first choice from 2FL.)
   {6, 0x000000, 0xEE, 0x22, {1, 2, 3, 5, 6, 7},   {4, 8, 9, 11, 2, 3},   {5, 8, 9, 10, 2, 3},
    s_bone,      sbigbone,     0, warn__none, 0},
   {6, 0x000000, 0xDD, 0x11, {0, 2, 3, 4, 6, 7},   {0, 8, 9, 7, 2, 3},    {1, 8, 9, 6, 2, 3},
    s_bone,      sbigbone,     0, warn__none, 0},

   // Collision at ends of center line during exchange the diamonds.
   {6, 0x33, 0x77, 0x44, {0, 1, 2, 4, 5, 6},   {0, 1, 3, 5, 6, 7},    {0, 1, 2, 5, 6, 8},
    s_qtag,      swqtag,     0, warn__none, 0},

   // Same spot as points of diamonds.
   {6, 0x022022, 0xEE, 0x22, {1, 2, 3, 5, 6, 7},   {0, 2, 3, 7, 8, 9},    {1, 2, 3, 6, 8, 9},
    s_qtag,      sbigdmd,     1, warn__none, 0x40000000},
   {6, 0x011011, 0xDD, 0x11, {0, 2, 3, 4, 6, 7},   {11, 2, 3, 4, 8, 9},   {10, 2, 3, 5, 8, 9},
    s_qtag,      sbigdmd,     1, warn__none, 0x40000000},
   // Same spot as points of hourglass.
   {6, 0x0220AA, 0xEE, 0x22, {1, 2, 3, 5, 6, 7},   {0, 2, 9, 7, 8, 3},    {1, 2, 9, 6, 8, 3},
    s_hrglass,   sbighrgl,    1, warn__none, 0x40000000},
   {6, 0x011099, 0xDD, 0x11, {0, 2, 3, 4, 6, 7},   {11, 2, 9, 4, 8, 3},   {10, 2, 9, 5, 8, 3},
    s_hrglass,   sbighrgl,    1, warn__none, 0x40000000},

   // Collisions at the points of a galaxy.
   {6, 0x0440EE, 0xEE, 0x44, {1, 2, 3, 5, 6, 7},   {5, 6, 0, 1, 3, 4},   {5, 7, 0, 1, 2, 4},
    s_galaxy,    s_rigger,    1, warn_bad_collision, 0x40000000},
   {6, 0x044044, 0xEE, 0x44, {1, 2, 3, 5, 6, 7},   {5, 6, 0, 1, 3, 4},   {5, 7, 0, 1, 2, 4},
    s_galaxy,    s_rigger,    1, warn_bad_collision, 0x40000000},
   {6, 0x0000AA, 0xBB, 0x11, {0, 1, 3, 4, 5, 7},   {6, 0, 1, 3, 4, 5},   {7, 0, 1, 2, 4, 5},
    s_galaxy,    s_rigger,    0, warn_bad_collision, 0x40000000},
   {6, 0x000000, 0xBB, 0x11, {0, 1, 3, 4, 5, 7},   {6, 0, 1, 3, 4, 5},   {7, 0, 1, 2, 4, 5},
    s_galaxy,    s_rigger,    0, warn_bad_collision, 0x40000000},

   {6, 0x000024, 0x3F, 0x09, {0, 1, 2, 3, 4, 5},   {0, 2, 3, 5, 6, 7},   {1, 2, 3, 4, 6, 7},
    s_wingedstar6, s_wingedstar, 0, warn__none, 0x40000000},

   {-1}};


void collision_collector::install_with_collision(
   setup *result, int resultplace,
   const setup *sourcepeople, int sourceplace,
   int rot) THROW_DECL
{
   if (resultplace < 0) fail("This would go into an excessively large matrix.");
   m_result_mask |= 1<<resultplace;
   int destination = resultplace;

   if (result->people[resultplace].id1) {
      // We have a collision.
      destination += 12;
      // Prepare the error message, in case it is needed.
      collision_person1 = result->people[resultplace].id1;
      collision_person2 = sourcepeople->people[sourceplace].id1;
      error_message1[0] = '\0';
      error_message2[0] = '\0';

      // ****** but if m_allow_collisions is "collision_severity_controversial", we want to give a fairly serious warning.

      if (m_allow_collisions == collision_severity_no ||
          attr::slimit(result) >= 12 ||
          result->people[destination].id1 != 0)
         throw error_flag_type(error_flag_collision);
      // Collisions are legal.  Store the person in the overflow area
      // (12 higher than the main area, which is why we only permit
      // this if the result setup size is <= 12) and record the fact
      // in the collision_mask so we can figure out what to do.
      m_collision_mask |= (1 << resultplace);
      m_collision_index = resultplace;   // In case we need to report a mundane collision.

      // Under certain circumstances (if callflags1 was manually overridden),
      // we need to keep track of who collides, and set things appropriately.
      // If we have "ends take right hands" and the setup is a 2x2, we consider it
      // to be perfectly legal.  This makes 2/3 recycle work from an inverted line.

      if (m_allow_collisions == collision_severity_controversial)
         m_collision_appears_illegal |= 0x100;
      else if ((m_callflags1 & CFLAG1_TAKE_RIGHT_HANDS) ||
          (m_callflags1 & CFLAG1_TAKE_RIGHT_HANDS_AS_COUPLES) ||
          ((m_callflags1 & CFLAG1_ENDS_TAKE_RIGHT_HANDS) &&
           (((result->kind == s1x4 || result->kind == sdmd) && !(resultplace&1)) ||
            (result->kind == s2x4 && !((resultplace+1)&2)) ||
            (result->kind == s_qtag && !(resultplace&2)) ||
            result->kind == s2x2))) {
         // Collisions are legal.
      }
      else if (m_callflags1 & CFLAG1_ENDS_TAKE_RIGHT_HANDS)
         // If the specific violation was that "ends take right hands" was on, but
         // the centers are taking right hands, it is extremely serious.
         m_collision_appears_illegal |= 4;
      else
         // Otherwise it is serious.
         m_collision_appears_illegal |= 2;
   }

   copy_rot(result, destination, sourcepeople, sourceplace, rot);
}



void collision_collector::fix_possible_collision(setup *result,
                                                 merge_action action /*= merge_strict_matrix*/,
                                                 uint32 callarray_flags /*= 0*/,
                                                 setup *ss /*= (setup *) 0*/) THROW_DECL
{
   if (!m_collision_mask) return;

   int i;
   setup spare_setup = *result;
   bool kill_ends = false;
   uint32 lowbitmask = 0;
   collision_map *c_map_ptr;

   result->clear_people();

   for (i=0; i<MAX_PEOPLE; i++) lowbitmask |= ((spare_setup.people[i].id1) & 1) << i;

   for (c_map_ptr = collision_map_table ; c_map_ptr->size >= 0 ; c_map_ptr++) {
      if (result->kind != c_map_ptr->initial_kind) continue;

      bool yukk = ((lowbitmask == c_map_ptr->lmask)) &&
         (m_result_mask == c_map_ptr->rmask) &&
         (m_collision_mask == c_map_ptr->cmask);

      // Under certain conditions (absence of people doesn't change the overall setup),
      // we allow setups in which people are absent.  Of course, there has to be at least
      // one genuine collision.
      if (!yukk &&
          (c_map_ptr->assume_key & 0x40000000) != 0 &&
          (m_result_mask & ~c_map_ptr->rmask) == 0 &&
          ((m_result_mask | (m_result_mask << (MAX_PEOPLE/2))) & c_map_ptr->lmask) == lowbitmask &&
          (m_result_mask & c_map_ptr->cmask) == m_collision_mask) {
         yukk = true;
      }

      if (!yukk)
         continue;

      if ((c_map_ptr->assume_key & 0x10000000) && ss && ss->kind == sdmd)
         continue;

      if (m_assume_ptr) {
         switch (c_map_ptr->assume_key & 0xFFFF) {
         case 1:
            if (m_assume_ptr->assumption != cr_li_lo ||
                m_assume_ptr->assump_col != 1 ||
                m_assume_ptr->assump_both != 2)
               kill_ends = true;
            break;
         case 2:
            if (m_assume_ptr->assumption != cr_li_lo ||
                m_assume_ptr->assump_col != 0 ||
                m_assume_ptr->assump_both != 1)
               kill_ends = true;
            break;
         case 3:
            if (m_assume_ptr->assumption != cr_li_lo ||
                m_assume_ptr->assump_col != 0 ||
                m_assume_ptr->assump_both != 2)
                  kill_ends = true;
               break;
            }
         }

         // In some cases, we want to ignore a table entry and keep searching,
         // rather than accept the table entry and deal with the warnings that it raises.
         // So we only do the "goto win" in certain circumstances.

         if (c_map_ptr->warning == warn_bad_collision)
            goto win;   // Entries with this warning always know exactly what they are doing.

         // If doing real as_couples call, we reject certain maps and pick up the next one.
         if (action == merge_c1_phantom_real_couples && (c_map_ptr->assume_key & 0x20000000))
            continue;

         if ((m_collision_appears_illegal & 6) ||
             ((m_collision_appears_illegal & 1) &&
              (c_map_ptr->assume_key & 0x80000000)))
            continue;
         else
            goto win;
   }

   // Don't recognize the pattern, report this as normal collision.
   throw error_flag_type(error_flag_collision);

 win:

   if ((callarray_flags & CAF__NO_CUTTING_THROUGH) && (c_map_ptr->assume_key & 0x08000000))
      fail("Call's collision has outsides cutting through the middle of the set.");

   if (m_collision_appears_illegal & 0x100)
      warn(warn_bad_collision);

   if ((m_collision_appears_illegal & 4) &&
       c_map_ptr->warning == warn_bad_collision)
      warn(warn_very_bad_collision);

   if ((m_collision_appears_illegal & 2) ||
       ((m_collision_appears_illegal & 1) && (c_map_ptr->assume_key & 0x80000000)) ||
       c_map_ptr->warning != warn_bad_collision)
      warn(c_map_ptr->warning);

   if (m_doing_half_override) {
      if (m_cmd_misc_flags & CMD_MISC__EXPLICIT_MIRROR)
         warn(warn__take_right_hands);
      else
         warn(warn__left_half_pass);
   }
   else {
      if (m_cmd_misc_flags & CMD_MISC__EXPLICIT_MIRROR)
         warn(warn__take_left_hands);
      else if (c_map_ptr->warning != warn__really_no_collision)
         warn(warn__take_right_hands);
   }

   int temprot = ((-c_map_ptr->rot) & 3) * 011;
   result->kind = c_map_ptr->final_kind;
   result->rotation += c_map_ptr->rot;

   // If this is under an implicit mirror image operation,
   // make them take left hands, by swapping the maps.

   uint32 flip = m_force_mirror_warn ? 2 : 0;

   for (i=0; i<c_map_ptr->size; i++) {
      int oldperson;

      oldperson = spare_setup.people[c_map_ptr->source[i]].id1;
      install_rot(result,
                  (((oldperson ^ flip) & 2) ? c_map_ptr->map1 : c_map_ptr->map0)[i],
                  &spare_setup,
                  c_map_ptr->source[i],
                  temprot);

      oldperson = spare_setup.people[c_map_ptr->source[i]+12].id1;
      install_rot(result,
                  (((oldperson ^ flip) & 2) ? c_map_ptr->map1 : c_map_ptr->map0)[i],
                  &spare_setup,
                  c_map_ptr->source[i]+12,
                  temprot);
   }

   if (kill_ends) {
      const veryshort m3276[] = {3, 2, 7, 6};
      const veryshort m2367[] = {2, 3, 6, 7};

      // The centers are colliding, but the ends are absent, and we have
      // no assumptions to guide us about where they should go.
      if ((result->kind != s_crosswave && result->kind != s1x8) ||
          (result->people[0].id1 |
           result->people[1].id1 |
           result->people[4].id1 |
           result->people[5].id1))
         fail("Need an assumption in order to take right hands at collision.");

      spare_setup = *result;

      if (result->kind == s_crosswave) {
         gather(result, &spare_setup, m2367, 3, 033);
         result->rotation++;
      }
      else {
         gather(result, &spare_setup, m3276, 3, 0);
      }

      result->kind = s_dead_concentric;
      result->inner.srotation = result->rotation;
      result->inner.skind = s1x4;
      result->rotation = 0;
      result->eighth_rotation = 0;
      result->concsetup_outer_elongation = 0;
   }
}


static void install_mirror_person_in_matrix(int x, int y,
                                            setup *s, const personrec *temp_p,
                                            const coordrec *optr,
                                            uint32 zmask)
{
   int place = optr->get_index_from_coords(x, y);
   if (place < 0)
      fail("Don't recognize ending setup for this call; not able to do it mirror.");

   // Switch the stable bits.
   uint32 n = temp_p->id1;

   uint32 tl = (n & STABLE_VLMASK) / STABLE_VLBIT;
   uint32 tr = (n & STABLE_VRMASK) / STABLE_VRBIT;
   uint32 z = (n & ~(STABLE_VLMASK|STABLE_VRMASK)) | (tl*STABLE_VRBIT) | (tr*STABLE_VLBIT);

   // Switch the slide and roll bits.
   z &= ~((3*NSLIDE_BIT) | (3*NROLL_BIT));
   z |= ((n >> 1) & (NSLIDE_BIT|NROLL_BIT)) | ((n & (NSLIDE_BIT|NROLL_BIT)) << 1);

   s->people[place].id1 = (z & zmask) ? (z ^ 2) : z;
   s->people[place].id2 = temp_p->id2;
   s->people[place].id3 = temp_p->id3;
}



void mirror_this(setup *s) THROW_DECL
{
   int i;

   if (s->kind == nothing) return;

   setup temp = *s;

   const coordrec *cptr = setup_attrs[s->kind].nice_setup_coords;

   switch (s->kind) {
   case s_ntrglcw:   s->kind = s_ntrglccw; break;
   case s_ntrglccw:  s->kind = s_ntrglcw; break;
   case s_nptrglcw:  s->kind = s_nptrglccw; break;
   case s_nptrglccw: s->kind = s_nptrglcw; break;
   case s_ntrgl6cw:  s->kind = s_ntrgl6ccw; break;
   case s_ntrgl6ccw: s->kind = s_ntrgl6cw; break;
   case s_nxtrglcw:  s->kind = s_nxtrglccw; break;
   case s_nxtrglccw: s->kind = s_nxtrglcw; break;
   case spgdmdcw:    s->kind = spgdmdccw; break;
   case spgdmdccw:   s->kind = spgdmdcw; break;
   }

   const coordrec *optr = setup_attrs[s->kind].nice_setup_coords;

   if (!cptr) {
      if (s->kind == s_trngl4) {
         if (s->rotation & 1) {
            s->rotation += 2;
            for (i=0; i<4; i++) copy_rot(&temp, i, &temp, i, 022);
            cptr = &tgl4_1;
         }
         else
            cptr = &tgl4_0;
      }
      else if (s->kind == s_trngl) {
         if (s->rotation & 1) {
            s->rotation += 2;
            for (i=0; i<3; i++) copy_rot(&temp, i, &temp, i, 022);
            cptr = &tgl3_1;
         }
         else
            cptr = &tgl3_0;
      }
      else if (s->kind == s_normal_concentric) {
         if (s->inner.skind == s_normal_concentric ||
             s->outer.skind == s_normal_concentric ||
             s->inner.skind == s_dead_concentric ||
             s->outer.skind == s_dead_concentric)
            fail("Recursive concentric?????.");

         s->kind = s->inner.skind;
         s->rotation = s->inner.srotation;
         s->eighth_rotation = 0;
         mirror_this(s);    // Sorry!
         s->inner.srotation = s->rotation;

         s->kind = s->outer.skind;
         s->rotation = s->outer.srotation;
         s->eighth_rotation = 0;
         for (i=0 ; i<12 ; i++) s->swap_people(i, i+12);
         mirror_this(s);    // Sorrier!
         for (i=0 ; i<12 ; i++) s->swap_people(i, i+12);
         s->outer.srotation = s->rotation;

         s->kind = s_normal_concentric;
         s->rotation = 0;
         s->eighth_rotation = 0;
         return;
      }
      else if (s->kind == s_dead_concentric) {
         if (s->inner.skind == s_normal_concentric || s->inner.skind == s_dead_concentric)
            fail("Recursive concentric?????.");

         s->kind = s->inner.skind;
         s->rotation += s->inner.srotation;
         mirror_this(s);    // Sorry!
         s->inner.srotation = s->rotation;

         s->kind = s_dead_concentric;
         s->rotation = 0;
         s->eighth_rotation = 0;
         return;
      }
      else
         fail("Don't recognize ending setup for this call; not able to do it mirror.");

      optr = cptr;
   }

   int limit = attr::slimit(s);

   if (s->rotation & 1) {
      for (i=0; i<=limit; i++)
         install_mirror_person_in_matrix(cptr->xca[i], -cptr->yca[i],
                                         s, &temp.people[i], optr, 010);
   }
   else {
      for (i=0; i<=limit; i++)
         install_mirror_person_in_matrix(-cptr->xca[i], cptr->yca[i],
                                         s, &temp.people[i], optr, 1);
   }

   if (s->eighth_rotation) {
      s->rotation = (s->rotation-1) & 3;
      canonicalize_rotation(s);
   }
}


static const veryshort ftc2x4[8] = {10, 15, 3, 1, 2, 7, 11, 9};
static const veryshort ftl2x4[12] = {6, 11, 15, 13, 14, 3, 7, 5, 6, 11, 15, 13};

static const veryshort ftqthbv[8] = {0, 1, 6, 7, 8, 9, 14, 15};
static const veryshort ftqthbh[12] = {12, 13, 2, 3, 4, 5, 10, 11, 12, 13, 2, 3};

static const veryshort ftc4x4[24] = {10, 15, 3, 1, 2, 7, 11, 9, 2, 7, 11, 9,
                                     10, 15, 3, 1, 10, 15, 3, 1, 2, 7, 11, 9};

static const veryshort ftrig12[12] = {9, 10, 11, -1, -1, -1, 3, 4, 5, -1, -1, -1,};

static const veryshort ftcphan[24] = {0, 2, 7, 5, 8, 10, 15, 13, 8, 10, 15, 13,
                                      0, 2, 7, 5, 0, 2, 7, 5, 8, 10, 15, 13};
static const veryshort ft2x42x6[8] = {1, 2, 3, 4, 7, 8, 9, 10};

static const veryshort ftcspn[8] = {6, 11, 13, 17, 22, 27, 29, 1};
static const veryshort ftcbone[8] = {6, 13, 18, 19, 22, 29, 2, 3};
static const veryshort ftc2x1dmd[6] = {1, 2, 4, 7, 8, 10};
static const veryshort ftl2x1dmd[9] = {10, 1, 2, 4, 7, 8, 10, 1, 2};
static const veryshort qhypergall[8] = {1, 8, 10, -1, 9, 0, 2, -1};
static const veryshort qhypergalc[8] = {-1, 3, 7, -1, -1, 11, 15, -1};

static const veryshort ftequalize[8] = {6, 0, 8, 13, 22, 16, 24, 29};
static const veryshort ftlcwv[12] = {25, 26, 2, 3, 9, 10, 18, 19, 25, 26, 2, 3};
static const veryshort ftlqtg[12] = {29, 6, 10, 11, 13, 22, 26, 27, 29, 6, 10, 11};
static const veryshort ftlbigqtg[12] = {28, 7, 10, 11, 12, 23, 26, 27, 28, 7, 10, 11};
static const veryshort ftlshort6dmd[9] = {4, -1, -1, 1, -1, -1, 4, -1, -1};
static const veryshort qtlqtg[12] = {1, -1, -1, 4, 5, -1, -1, 0, 1, -1, -1, 4};
static const veryshort qtlbone[12] = {0, 3, -1, -1, 4, 7, -1, -1, 0, 3, -1, -1};
static const veryshort qtlbone2[12] = {0, -1, -1, 1, 4, -1, -1, 5, 0, -1, -1, 1};
static const veryshort qtlxwv[12] = {0, 1, -1, -1, 4, 5, -1, -1, 0, 1, -1, -1};
static const veryshort qtl1x8[12] = {-1, -1, 5, 7, -1, -1, 1, 3, -1, -1, 5, 7};
static const veryshort qtlrig[12] = {6, 7, -1, -1, 2, 3, -1, -1, 6, 7, -1, -1};
static const veryshort qtlgls[12] = {2, 5, 6, 7, 8, 11, 0, 1, 2, 5, 6, 7};
static const veryshort qtg2x4[12] = {7, 0, -1, -1, 3, 4, -1, -1, 7, 0, -1, -1};
static const veryshort f2x4qtg[12] = {5, -1, -1, 0, 1, -1, -1, 4, 5, -1, -1, 0};
static const veryshort f2x4phan[24] = {12, 14, 3, 1, 4, 6, 11, 9, 4, 6, 11, 9,
                                       12, 14, 3, 1, 12, 14, 3, 1, 4, 6, 11, 9};
static const veryshort ft4x4bh[16] = {9, 8, 7, -1, 6, -1, -1, -1, 3, 2, 1, -1, 0, -1, -1, -1};
static const veryshort ftqtgbh[8] = {-1, -1, 10, 11, -1, -1, 4, 5};
static const veryshort ft3x4bb[12] = {-1, -1, -1, -1, 8, 9, -1, -1, -1, -1, 2, 3};
static const veryshort ft4x446[16] = {4, 7, 22, 8, 13, 14, 15, 21, 16, 19, 10, 20, 1, 2, 3, 9};
static const veryshort ft2646[12] = {11, 10, 9, 8, 7, 6, 23, 22, 21, 20, 19, 18};
static const veryshort galtranslateh[16]  = {-1,  3,  4,  2, -1, -1, -1,  5,
                                             -1,  7,  0,  6, -1, -1, -1,  1};
static const veryshort galtranslatev[16]  = {-1, -1, -1,  1, -1,  3,  4,  2,
                                             -1, -1, -1,  5, -1,  7,  0,  6};
static const veryshort phantranslateh[16] = { 0, -1,  1,  1, -1,  3,  2,  2,
                                              4, -1,  5,  5, -1,  7,  6,  6};
static const veryshort phantranslatev[16] = {-1,  7,  6,  6,  0, -1,  1,  1,
                                             -1,  3,  2,  2,  4, -1,  5,  5};
static const veryshort sdmdtranslateh[8] = {0, 0, 0, 1, 2, 0, 0, 3};
static const veryshort sdmdtranslatev[8] = {0, 3, 0, 0, 0, 1, 2, 0};
static const veryshort stharlinetranslateh[8] = {0, 1, 0, 0, 2, 3, 0, 0};
static const veryshort stharlinetranslatev[8] = {0, 0, 0, 1, 0, 0, 2, 3};

static const veryshort octtranslatev[80] = {
   0,  0,  0, 15,  0,  0,  0, 14,  0,  0,  0, 13,  0,  0,  0, 12,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,
   0,  0,  0,  7,  0,  0,  0,  6,  0,  0,  0,  5,  0,  0,  0,  4,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  9, 10, 11,
   0,  0,  0, 15,  0,  0,  0, 14,  0,  0,  0, 13,  0,  0,  0, 12};

static const veryshort octt4x6latev[80] = {
   0,  0,  0,  0,  0,  0, 17, 18,  0,  0, 16, 19,  0,  0, 15, 20,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  0, 11, 10,  9,
   0,  0,  0,  0,  0,  0,  5,  6,  0,  0,  4,  7,  0,  0,  3,  8,
   0,  0,  0,  0,  0,  0,  0,  0,  0, 12, 13, 14,  0, 23, 22, 21,
   0,  0,  0,  0,  0,  0, 17, 18,  0,  0, 16, 19,  0,  0, 15, 20};

static const veryshort hextranslatev[40] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,
   0,  0,  0,  0,  0,  0,  0,  0,  8,  9, 10, 11, 12, 13, 14, 15,
   0,  0,  0,  0,  0,  0,  0,  0};

static const veryshort hxwtranslatev[40] = {
   0,  0,  0,  0,  0,  0,  6,  7,  0,  0,  0,  0,  0,  0,  1,  0,
   0,  0,  0,  0,  0,  0,  2,  3,  0,  0,  0,  0,  0,  4,  5,  0,
   0,  0,  0,  0,  0,  0,  6,  7};

static const veryshort hthartranslate[32] = {
   0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  2,  3,
   0,  0,  0,  0,  0,  0,  4,  5,  0,  0,  0,  0,  0,  0,  6,  7};

static const veryshort h1x6translatev[15] = {
   0,  0,  0,  0, 1, 2,
   0,  0,  0,  3, 4, 5,
   0,  0,  0};

static const veryshort h1x8translatev[20] = {
   0,  0,  0,  0,  0, 1, 3, 2,
   0,  0,  0,  0,  4, 5, 7, 6,
   0,  0,  0,  0};

static const veryshort hxwvdmdtranslate3012[20] = {
   3,  3,  3,  3,  0,  0,  0,  0,
   1,  1,  1,  1,  2,  2,  2,  2,
   3,  3,  3,  3};

static const veryshort h1x6thartranslate[12] = {
   0,  0,  1,  0,  2,  3,
   0,  4,  5,  0,  6,  7};

static const veryshort h1x6translate2x1d[12] = {
   0,  0,  1,  0,  2,  0,
   0,  3,  4,  0,  5,  0};

static const veryshort h1x8thartranslate9999[16] = {
   0,  0,  1,  0,  0,  2,  3,  0,
   0,  4,  5,  0,  0,  6,  7,  0};

static const veryshort h1x8thartranslatec3c3[20] = {
   0,  0,  6,  7,  0,  1,  0,  0,
   0,  0,  2,  3,  4,  5,  0,  0,
   0,  0,  6,  7};

static const veryshort dmdhyperv[15] = {0, 3, 0, 0, 0, 0,
                                        0, 1, 0, 2, 0, 0,
                                        0, 3, 0};

static const veryshort linehyperh[12] = {0, 1, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0};
static const veryshort linehyperv[12] = {0, 0, 0, 0, 1, 0, 0, 0, 0, 2, 3, 0};

static const veryshort hyperbonev[20] = {5, 0, 6, 7, -1, -1, -1, -1,
                                         1, 4, 2, 3, -1, -1, -1, -1,
                                         5, 0, 6, 7};

static const veryshort hyper3x4v[30] = {3, 2, 1, 0, 4, 5, -1, -1, -1, -1, -1, -1,
                                        9, 8, 7, 6, 10, 11, -1, -1, -1, -1, -1, -1,
                                        3, 2, 1, 0, 4, 5};

static const veryshort tinyhyperbonet[20] = {-1, -1, -1, -1, -1, -1, -1, -1,
                                             -1, -1, -1, -1,  2,  3,  1,  0,
                                             -1, -1, -1, -1};

static const veryshort tinyhyperbonel[20] = {-1, -1,  3,  2, -1, -1, -1, -1,
                                             -1, -1,  1,  0, -1, -1, -1, -1,
                                             -1, -1,  3,  2};

static const veryshort tinyhyperboneb[20] = { 3,  0, -1, -1, -1, -1, -1, -1,
                                              1,  2, -1, -1, -1, -1, -1, -1,
                                              3,  0, -1, -1};

static const veryshort galhyperv[15] = {0, 7, 5, 6, 0, 0, 0, 3, 1, 2, 0, 4, 0, 7, 5};
static const veryshort qtghyperh[12] = {6, 7, 0, 0, 0, 1, 2, 3, 4, 0, 0, 5};
static const veryshort qtghyperv[12] = {0, 0, 5, 6, 7, 0, 0, 0, 1, 2, 3, 4};
static const veryshort starhyperh[12] =  {0, 0, 0, 0, 1, 0, 0, 2, 0, 0, 3, 0};
static const veryshort fstarhyperh[12] = {0, 0, 0, 1, 0, 0, 2, 0, 0, 3, 0, 0};
static const veryshort lilstar1[8] = {0, 2, 0, 0, 3, 0, 0, 1};
static const veryshort lilstar2[8] = {3, 0, 0, 1, 0, 2, 0, 0};
static const veryshort lilstar3[8] = {0, 1, 0, 0, 2, 3, 0, 0};
static const veryshort lilstar4[8] = {0, 0, 2, 3, 0, 0, 0, 1};


static const veryshort qtbd1[12] = {5, 9, 6, 7, 9, 0, 1, 9, 2, 3, 9, 4};
static const veryshort qtbd2[12] = {9, 5, 6, 7, 0, 9, 9, 1, 2, 3, 4, 9};
static const veryshort qtbd3[12] = {9, 5, 6, 7, 9, 0, 9, 1, 2, 3, 9, 4};
static const veryshort qtbd4[12] = {5, 9, 6, 7, 0, 9, 1, 9, 2, 3, 4, 9};
static const veryshort q3x4xx1[12] = {9, 5, 0, 9, 9, 1, 9, 2, 3, 9, 9, 4};
static const veryshort q3x4xx2[12] = {9, 9, 9, 9, 2, 3, 9, 9, 9, 9, 0, 1};
static const veryshort q3x4xx3[12] = {9, 9, 2, 2, 9, 9, 3, 3, 9, 9, 0, 1};
static const veryshort q3x4xx4[12] = {3, 3, 9, 9, 0, 1, 9, 9, 2, 2, 9, 9};



extern void do_stability(uint32 *personp,
                         int field,
                         int turning,
                         bool mirror) THROW_DECL
{
   turning &= 3;

   switch (field & STB_MASK) {
   case STB_NONE:
      *personp &= ~STABLE_ALL_MASK;
      return;
   case STB_NONE+STB_REVERSE:
      // "None" + REV = "Z".
      if (turning != 0)
         fail("Person turned but was marked 'Z' stability.");
      break;
   case STB_A:
      turning -= 4;
      break;
   case STB_A+STB_REVERSE:
      if (turning == 0) turning = 4;
      break;
   case STB_AC:
      // Turn one quadrant anticlockwise.
      do_stability(personp, STB_A, 3, mirror);
      turning += 1;                    // And the rest clockwise.
      break;
   case STB_AC+STB_REVERSE:
      // Turn one quadrant clockwise.
      do_stability(personp, STB_A+STB_REVERSE, 1, mirror);
      if (turning == 0) turning = 4;   // And the rest anticlockwise.
      turning -= 5;
      break;
   case STB_AAC:
      // Turn two quadrants anticlockwise.
      do_stability(personp, STB_A, 2, mirror);
      if (turning == 3) turning = -1;  // And the rest clockwise.
      turning += 2;
      break;
   case STB_AAC+STB_REVERSE:
      // Turn two quadrants clockwise.
      do_stability(personp, STB_A+STB_REVERSE, 2, mirror);
      if (turning <= 1) turning += 4;  // And the rest anticlockwise.
      turning -= 6;
      break;
   case STB_AAAC:
      // Turn three quadrants anticlockwise.
      do_stability(personp, STB_A, 1, mirror);
      if (turning >= 2) turning -= 4;  // And the rest clockwise.
      turning += 3;
      break;
   case STB_AAAC+STB_REVERSE:
      // Turn three quadrants clockwise.
      do_stability(personp, STB_A+STB_REVERSE, 3, mirror);
      if (turning == 3) turning = -1;  // And the rest anticlockwise.
      turning -= 3;
      break;
   case STB_AAAAC:
      // Turn four quadrants anticlockwise.
      do_stability(personp, STB_A, 0, mirror);
      if (turning == 0) turning = 4;   // And the rest clockwise.
      break;
   case STB_AAAAC+STB_REVERSE:
      // Turn four quadrants clockwise.
      do_stability(personp, STB_A+STB_REVERSE, 0, mirror);
      turning -= 4;                    // And the rest anticlockwise.
      break;
   case STB_AA:
      // Turn four quadrants anticlockwise.
      do_stability(personp, STB_A, 0, mirror);
      break;      // And keep turning.
   case STB_AA+STB_REVERSE:
      // Turn four quadrants clockwise.
      do_stability(personp, STB_A+STB_REVERSE, 0, mirror);
      break;      // And keep turning.
   default:
      *personp &= ~STABLE_ALL_MASK;
      return;
   }

   // Now turning has the number of quadrants the person turned, clockwise or anticlockwise.

   if (mirror) turning = -turning;

   int absturning_in_eighths = turning * 2;     // Measured in eighths!
   if (absturning_in_eighths < 0) absturning_in_eighths = -absturning_in_eighths;

   int abs_overturning_in_eighths = absturning_in_eighths -
      ((*personp & STABLE_RMASK) / STABLE_RBIT);

   if (abs_overturning_in_eighths <= 0)
      *personp -= absturning_in_eighths*STABLE_RBIT;
   else {
      if (turning < 0) {
         *personp =
            (*personp & ~(STABLE_RMASK|STABLE_VLMASK)) |
            ((*personp + (STABLE_VLBIT * abs_overturning_in_eighths)) & STABLE_VLMASK);
      }
      else {
         *personp =
            (*personp & ~(STABLE_RMASK|STABLE_VRMASK)) |
            ((*personp + (STABLE_VRBIT * abs_overturning_in_eighths)) & STABLE_VRMASK);
      }
   }
}


extern bool check_restriction(
   setup *ss,
   assumption_thing restr,
   bool instantiate_phantoms,
   uint32 flags) THROW_DECL
{
   uint32 q0, q1, q2, q3;
   uint32 z, t;
   int idx;
   const veryshort *mp;
   bool only_because_of_2faced = false;
   bool retval = true;   // True means we were not able to instantiate phantoms.
                         // It is only meaningful if instantiation was requested.

   // First, check for nice things.

   // If this successfully instantiates phantoms, it will clear retval.
   switch (verify_restriction(ss, restr, instantiate_phantoms, &retval)) {
   case restriction_passes:
      if (restr.assumption == cr_wave_unless_say_2faced &&
          (ss->cmd.cmd_misc3_flags & CMD_MISC3__TWO_FACED_CONCEPT)) {
         // User said "two-faced", and the call is marked "wave_unless_say_2faced",
         // but the "wave" test passed.  Have to do it this way.  See si03t.
         assumption_thing second_restr = restr;
         second_restr.assumption = cr_2fl_only;
         bool junk;
         restriction_test_result fff = verify_restriction(ss, second_restr, false, &junk);
         if (fff != restriction_passes)
            fail("Two-faced concept used when not in 2-faced line.");
      }
      goto getout;
   case restriction_fails_on_2faced:
      {
         // Test again; see whether we have a real 2-faced line.
         assumption_thing second_restr = restr;
         second_restr.assumption = cr_2fl_only;
         bool junk;
         restriction_test_result fff = verify_restriction(ss, second_restr, false, &junk);

         if (ss->cmd.cmd_misc3_flags & CMD_MISC3__TWO_FACED_CONCEPT) {
            if (fff != restriction_passes)
               fail("Two-faced concept used when not in 2-faced line.");
            goto getout;
         }
         else {
            if (fff == restriction_passes) warn(warn__two_faced);
            only_because_of_2faced = true;
            goto restr_failed;
         }
      }
   case restriction_bad_level:
      if (allowing_all_concepts) {
         warn(warn__bad_call_level);
         goto getout;
      }
      else if (flags == CAF__RESTR_UNUSUAL)
         // If it has been marked unusual below some level, just give that warning;
         // don't complain about the level itself.
         goto restr_failed;
      else
         fail("This call is not legal from this formation at this level.");
   case restriction_fails:
      goto restr_failed;
   }

   // Now check all the other stuff.

   switch (restr.assumption) {
   case cr_real_1_4_tag: case cr_real_3_4_tag:
   case cr_real_1_4_line: case cr_real_3_4_line:
   case cr_tall6:
      // These are not legal if they weren't handled already.
      goto restr_failed;
   }

   if (!(restr.assump_col & 1)) {
      // Restriction is "line-like" or special.

      static const veryshort mapwkg8[3] = {2, 2, 6};
      static const veryshort mapwkg6[3] = {2, 2, 5};
      static const veryshort mapwkg4[3] = {2, 1, 3};
      static const veryshort mapwkg2[3] = {2, 0, 1};
      static const veryshort mapwk24[5] = {4, 1, 2, 6, 5};

      switch (restr.assumption) {
      case cr_awkward_centers:       /* check for centers not having left hands */
         switch (ss->kind) {
         case s2x4: mp = mapwk24; goto check_wk;
         case s1x8: mp = mapwkg8; goto check_wk;
         case s1x6: mp = mapwkg6; goto check_wk;
         case s1x4: mp = mapwkg4; goto check_wk;
         case s1x2: mp = mapwkg2; goto check_wk;
         }
         break;
      case cr_ends_are_peelable:
         /* check for ends in a "peelable" (everyone in genuine tandem somehow) box */
         if (ss->kind == s2x4) {
            q1 = 0; q0 = 0; q3 = 0; q2 = 0;
            if ((t = ss->people[0].id1) != 0) { q1 |= t; q0 |= (t^2); }
            if ((t = ss->people[3].id1) != 0) { q3 |= t; q2 |= (t^2); }
            if ((t = ss->people[4].id1) != 0) { q3 |= t; q2 |= (t^2); }
            if ((t = ss->people[7].id1) != 0) { q1 |= t; q0 |= (t^2); }
            if (((q1&3) && (q0&3)) || ((q3&3) && (q2&3)))
               goto restr_failed;
         }
         break;
      case cr_explodable:
         if (ss->kind == s1x4) {
            t = ss->people[1].id1;
            z = ss->people[3].id1;
            if ((z & t) && ((z ^ t) & 2)) goto restr_failed;
         }
         break;
      case cr_rev_explodable:
         if (ss->kind == s1x4) {
            t = ss->people[0].id1;
            z = ss->people[2].id1;
            if ((z & t) && ((z ^ t) & 2)) goto restr_failed;
         }
         break;
      case cr_not_tboned:
         if ((or_all_people(ss) & 011) == 011) goto restr_failed;
         break;
      }
   }

   goto getout;

   check_wk:   /* check the "awkward_centers" restriction. */

   for (idx=0 ; idx < mp[0] ; idx+=2) {
      t = ss->people[mp[idx+1]].id1;
      z = ss->people[mp[idx+2]].id1;
      if ((z&t) && !((z | (~t)) & 2)) warn(warn__awkward_centers);
   }

   goto getout;

   restr_failed:

   switch (flags) {
   case CAF__RESTR_RESOLVE_OK:
      warn(warn__dyp_resolve_ok);
      break;
   case CAF__RESTR_UNUSUAL:
      // want to suppress the "unusual" if it's already giving the "warn__two_faced".
      if (only_because_of_2faced)
         warn(warn__unusual_or_2faced);
      else
         warn(warn__unusual);
      break;
   case CAF__RESTR_CONTROVERSIAL:
      warn(warn_controversial);
      break;
   case CAF__RESTR_EACH_1X3:
      warn(warn__split_to_1x3s_always); // This one is not subject to being suppressed in inner_selective_move.
      break;
   case CAF__RESTR_BOGUS:
      warn(warn_serious_violation);
      break;
   case CAF__RESTR_ASSUME_DPT:
      warn(warn__assume_dpt);
      break;
   case CAF__RESTR_FORBID:
      fail("This call is not legal from this formation.");
   case 99:
      if (restr.assumption == cr_qtag_like && restr.assump_both == 1)
         fail("People are not facing as in 1/4 tags.");
      else if (restr.assumption == cr_qtag_like && restr.assump_both == 2)
         fail("People are not facing as in 3/4 tags.");
      else if (restr.assumption == cr_wave_only && restr.assump_col == 0)
         fail("People are not in waves.");
      else if (restr.assumption == cr_all_ns)
         fail("People are not in lines.");
      else if (restr.assumption == cr_all_ew)
         fail("People are not in columns.");
      else
         fail("The people do not satisfy the assumption.");
   default:
      warn(warn__do_your_part);
      break;
   }

   getout:

   // One final check.  If there is an assumption in place that is inconsistent
   // with the restriction being enforced, then it is an error, even though
   // no live people violate the restriction.

   switch (ss->cmd.cmd_assume.assumption) {
   case cr_qtag_like:
      switch (restr.assumption) {
      case cr_diamond_like: case cr_pu_qtag_like:
         fail("An assumed facing direction for phantoms is illegal for this call.");
         break;
      }
      break;
   case cr_diamond_like:
      switch (restr.assumption) {
      case cr_qtag_like: case cr_pu_qtag_like:
         fail("An assumed facing direction for phantoms is illegal for this call.");
         break;
      }
      break;
   case cr_pu_qtag_like:
      switch (restr.assumption) {
      case cr_qtag_like: case cr_diamond_like:
         fail("An assumed facing direction for phantoms is illegal for this call.");
         break;
      }
      break;
   case cr_couples_only: case cr_3x3couples_only: case cr_4x4couples_only:
      switch (restr.assumption) {
      case cr_wave_only: case cr_miniwaves:
         fail("An assumed facing direction for phantoms is illegal for this call.");
         break;
      }
      break;
   case cr_wave_only: case cr_miniwaves:
      switch (restr.assumption) {
      case cr_couples_only: case cr_3x3couples_only: case cr_4x4couples_only:
         fail("An assumed facing direction for phantoms is illegal for this call.");
         break;
      }
      break;
   }

   return retval;
}


// The 32 bit word returned by this is an unpacked/uncompressed version of the 16 bit
// word in the database.  See comments at the end of database.h of a description of the latter.

// Left half:
//       unused         slide and roll info          unused
//                   (2 for slide, 3 for roll)
//       2 bits              5 bits                   9 bits
//
// The slide and roll info are in the proper position for the "id1" field of a person.
// See NSLIDE_MASK, NSLIDE_BIT, SLIDE_IS_L, SLIDE_IS_R,
//    NROLL_MASK, NROLL_BIT, PERSON_MOVED, ROLL_IS_L, and ROLL_IS_R.

// Right half:
//     stability info        unused         where to go     direction to face
//                                        (uncompressed!)
//         4 bits            4 bits            6 bits            2 bits
//
// The stability info is in the same location as in the 16 bit word from the database,
// and is also indicated by DBSTAB_BIT.  The location might be zero.

static uint32 find_calldef(
   callarray *tdef,
   setup *scopy,
   int real_index,
   int real_direction,
   int northified_index) THROW_DECL
{
   if (!tdef) crash_print(__FILE__, __LINE__, 0, (setup *) 0);

   unsigned short *calldef_array;
   uint32 z;

   if (tdef->callarray_flags & CAF__PREDS) {
      for (predptr_pair *predlistptr = tdef->stuff.prd.predlist ;
           predlistptr ;
           predlistptr = predlistptr->next) {
         if ((*(predlistptr->pred->predfunc))
             (scopy, real_index, real_direction,
              northified_index, predlistptr->pred->extra_stuff)) {
            calldef_array = predlistptr->array_pred_def;
            goto got_it;
         }
      }
      fail(tdef->stuff.prd.errmsg);
   }
   else
      calldef_array = tdef->stuff.array_no_pred_def;

got_it:

   z = (uint32) calldef_array[northified_index];
   if (!z) failp(scopy->people[real_index].id1, "can't execute their part of this call.");

   // Uncompress the destination position.
   int field = uncompress_position_number(z);

   // Shift the slide/roll data up into the right place.
   uint32 rollstuff = (z * (NROLL_BIT/DBSLIDEROLL_BIT)) & (NSLIDE_MASK|NROLL_MASK);

   // Preserve stability and direction from original.
   return (z & 0xF003) | (field << 2) | rollstuff;
}


static uint32 do_slide_roll(uint32 person_in, uint32 z, int direction)
{
   uint32 sliderollstuff = z & (NSLIDE_MASK|NROLL_MASK);
   // If just "L" or "R" (but not "M", that is, not both bits), turn on "moved".
   if ((sliderollstuff+NROLL_BIT) & (NROLL_BIT*2)) sliderollstuff |= PERSON_MOVED;

   return (person_in & ~(NSLIDE_MASK | NROLL_MASK | 077)) |
      (((((z + direction) & 3) * 011) ^ 010) & 013) | sliderollstuff;
}


static void special_4_way_symm(
   callarray *tdef,
   setup *scopy,
   setup *destination,
   int newplacelist[],
   uint32 lilresult_mask[],
   setup *result) THROW_DECL
{
   static const veryshort table_2x4[8] = {10, 15, 3, 1, 2, 7, 11, 9};

   static const veryshort table_2x8[16] = {
      12, 13, 14, 15, 31, 27, 23, 19,
      44, 45, 46, 47, 63, 59, 55, 51};

   static const veryshort table_3x1d[8] = {
      1, 2, 3, 9, 17, 18, 19, 25};

   static const veryshort table_2x6[12] = {
      13, 14, 15, 31, 27, 23,
      45, 46, 47, 63, 59, 55};

   static const veryshort table_4x6[24] = {
       9, 10, 11, 30, 26, 22,
      23, 27, 31, 15, 14, 13,
      41, 42, 43, 62, 58, 54,
      55, 59, 63, 47, 46, 45};

   static const veryshort table_1x16[16] = {
       0,  1,  2,  3,  4,  5,  6,  7,
      16, 17, 18, 19, 20, 21, 22, 23};

   static const veryshort table_x1x8_from_xwv[8] = {0, 2, 5, 7, 8, 10, 13, 15};

   static const veryshort table_4dmd[16] = {
      7, 5, 14, 12, 16, 17, 18, 19,
      23, 21, 30, 28, 0, 1, 2, 3};

   static const veryshort table_hyperbone[8] = {13, 4, 6, 7, 5, 12, 14, 15};

   static const veryshort table_hyper3x4[12] = {0, 1, 2, 3, 10, 11, 12, 13, 14, 15, 22, 23};

   static const veryshort table_tinyhyperbone[8] = {3, 2, 0, 1, 11, 10, 8, 9};

   static const veryshort table_2x3_4dmd[6] = {6, 11, 13, 22, 27, 29};

   static const veryshort table_bigd_x4dmd[12] = {7, 5, 10, 11, 14, 12, 23, 21, 26, 27, 30, 28};

   static const veryshort line_table[4] = {0, 1, 6, 7};

   static const veryshort dmd_table[4] = {0, 4, 6, 10};

   int begin_size;
   int real_index;
   int k, result_size, result_quartersize;
   const veryshort *the_table = (const veryshort *) 0;

   switch (result->kind) {
   case s2x2: case s_galaxy:
   case s_c1phan: case s4x4:
   case s_hyperglass: case s_thar:
   case s_star: case s1x1: case s_alamo:
      break;
   case s1x4:
      result->kind = s_hyperglass;
      the_table = line_table;
      break;
   case sdmd:
      result->kind = s_hyperglass;
      the_table = dmd_table;
      break;
   case s2x4:
      result->kind = s4x4;
      the_table = table_2x4;
      break;
   case s2x8:
      result->kind = s8x8;
      the_table = table_2x8;
      break;
   case s3x1dmd:
      result->kind = sx4dmd;
      the_table = table_3x1d;
      break;
   case s4x6:
      result->kind = s8x8;
      the_table = table_4x6;
      break;
   case s2x6:
      result->kind = s8x8;
      the_table = table_2x6;
      break;
   case s1x16:
      result->kind = sx1x16;
      the_table = table_1x16;
      break;
   case s_crosswave:
      result->kind = sx1x8;
      the_table = table_x1x8_from_xwv;
      break;
   case s4dmd:
      result->kind = sx4dmd;
      the_table = table_4dmd;
      break;
   case s_bone:
      result->kind = s_hyperbone;
      the_table = table_hyperbone;
      break;
   case s3x4:
      result->kind = s_hyper3x4;
      the_table = table_hyper3x4;
      break;
   case s_trngl4:
      result->kind = s_tinyhyperbone;
      the_table = table_tinyhyperbone;
      break;
   case s2x3:
      result->kind = sx4dmd;
      the_table = table_2x3_4dmd;
      break;
   case sbigdmd:
      result->kind = sx4dmd;
      the_table = table_bigd_x4dmd;
      break;
   default:
      fail("Don't recognize ending setup for this call.");
   }

   begin_size = attr::slimit(scopy)+1;
   result_size = attr::slimit(result)+1;
   result_quartersize = result_size >> 2;
   lilresult_mask[0] = 0;
   lilresult_mask[1] = 0;

   for (real_index=0; real_index<begin_size; real_index++) {
      personrec this_person = scopy->people[real_index];
      destination->clear_person(real_index);
      if (this_person.id1) {
         int real_direction = this_person.id1 & 3;
         int northified_index = (real_index + (((4-real_direction)*begin_size) >> 2)) % begin_size;
         uint32 z = find_calldef(tdef, scopy, real_index, real_direction, northified_index);
         k = (z >> 2) & 0x3F;
         if (the_table) k = the_table[k];
         k = (k + real_direction*result_quartersize) % result_size;
         destination->people[real_index].id1 = do_slide_roll(this_person.id1, z, real_direction);

         if (this_person.id1 & STABLE_ENAB)
            do_stability(&destination->people[real_index].id1,
                         z/DBSTAB_BIT,
                         z + result->rotation, false);

         destination->people[real_index].id2 = this_person.id2;
         destination->people[real_index].id3 = this_person.id3;
         newplacelist[real_index] = k;
         lilresult_mask[k>>5] |= (1 << (k&037));
      }
   }
}



static void special_triangle(
   callarray *cdef,
   callarray *ldef,
   setup *scopy,
   setup *destination,
   int newplacelist[],
   int num,
   uint32 lilresult_mask[],
   setup *result) THROW_DECL
{
   int real_index;
   int numout = attr::slimit(result)+1;
   bool is_triangle = setup_attrs[scopy->kind].no_symmetry;
   bool result_is_triangle = setup_attrs[result->kind].no_symmetry;

   for (real_index=0; real_index<num; real_index++) {
      personrec this_person = scopy->people[real_index];
      destination->clear_person(real_index);
      newplacelist[real_index] = -1;
      if (this_person.id1) {
         int k;
         uint32 z;
         int northified_index = real_index;
         int real_direction = this_person.id1 & 3;

         if (real_direction & 2) {
            if (is_triangle) {
               northified_index += num;
            }
            else if (scopy->kind == s1x3) {
               northified_index = 2-northified_index;
            }
            else {
               int d2 = num >> 1;
               northified_index = (northified_index + d2) % num;
            }
         }

         z = find_calldef((real_direction & 1) ? cdef : ldef, scopy, real_index, real_direction, northified_index);
         k = (z >> 2) & 0x3F;

         if (real_direction & 2) {
            if (result_is_triangle) {
               k-=num;
               if (k<0) k += (num << 1);
            }
            else if (result->kind == s1x3)
               k = numout-1-k;
            else {
               k -= (numout >> 1);
               if (k<0) k+=numout;
            }
         }

         destination->people[real_index].id1 = do_slide_roll(this_person.id1, z, real_direction);

         if (this_person.id1 & STABLE_ENAB)
            do_stability(&destination->people[real_index].id1,
                         z/DBSTAB_BIT,
                         z+result->rotation, false);

         destination->people[real_index].id2 = this_person.id2;
         destination->people[real_index].id3 = this_person.id3;
         newplacelist[real_index] = k;
         lilresult_mask[0] |= (1 << k);
      }
   }

   // Check whether the call went into the other triangle.  If so, it
   // must have done so completely.

   if (result_is_triangle) {
      int i;

      for (i=0; i<num; i++) {
         if (newplacelist[i] >= numout) {
            result->rotation += 2;

            for (i=0; i<num; i++) {
               if (newplacelist[i] >=0) {
                  newplacelist[i] -= numout;
                  if (newplacelist[i] < 0)
                     fail("Inconsistent orientation of triangle result.");
                  destination->people[i].id1 = rotperson(destination->people[i].id1, 022);
               }
            }

            break;
         }
      }
   }
}


static bool handle_3x4_division(
   setup *ss, uint32 callflags1, uint32 newtb, uint32 livemask,
   uint32 & division_code,            // We write over this.
   callarray *calldeflist, bool matrix_aware, setup *result)

{
   bool forbid_little_stuff;
   uint32 nxnbits =
      ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_NXNMASK|INHERITFLAG_MXNMASK);

   /* The call has no applicable 3x4 or 4x3 definition. */
   /* First, check whether it has 2x3/3x2 definitions, and divide the setup if so,
      and if the call permits it.  This is important for permitting "Z axle" from
      a 3x4 but forbidding "circulate" (unless we give a concept like 12 matrix
      phantom columns.)  We also enable this if the caller explicitly said
      "3x4 matrix". */

   if ((callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) ||
       ((callflags1 & CFLAG1_SPLIT_IF_Z) &&
        nxnbits != INHERITFLAGNXNK_3X3 &&
        (livemask == 06565 || livemask == 07272)) ||
       (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) ||
       nxnbits == INHERITFLAGMXNK_1X3 ||
       nxnbits == INHERITFLAGMXNK_3X1) {

      if ((!(newtb & 010) || assoc(b_3x2, ss, calldeflist)) &&
          (!(newtb & 001) || assoc(b_2x3, ss, calldeflist))) {
         division_code = MAPCODE(s2x3,2,MPKIND__SPLIT,1);
         return true;
      }
      else if ((!(newtb & 001) || assoc(b_1x3, ss, calldeflist)) &&
               (!(newtb & 010) || assoc(b_3x1, ss, calldeflist))) {
         division_code = MAPCODE(s1x3,4,MPKIND__SPLIT,1);
         return true;
      }
   }

   /* Search for divisions into smaller things: 2x2, 1x2, 2x1, or 1x1,
      in setups whose occupations make it clear what is meant.
      We do not allow this if the call has a larger definition.
      The reason is that we do not actually use "divided_setup_move"
      to perform the division that we want in one step.  This could require
      maps with excessive arity.  Instead, we use existing maps to break it
      down a little, and depend on recursion.  If larger definitions existed,
      the recursion might not do what we have in mind.  For example,
      we could get here on "checkmate" from offset columns, since that call
      has a 2x2 definition but no 4x3 definition.  However, all we do here
      is use the map that the "offset columns" concept uses.  We would then
      recurse on a virtual 4x2 column, and pick up the 8-person version of
      "checkmate", which would be wrong.  Similarly, this could lead to an
      "offset wave circulate" being done when we didn't say "offset wave".
      For the call "circulate", it might be considered more natural to do
      a 12 matrix circulate.  But if the user doesn't say "12 matrix",
      we want to refuse to do it.  The only legal circulate if no concept
      is given is the 2x2 circulate in each box.  So we disallow this if
      any possibly conflicting definition could be seen during the recursion. */

   forbid_little_stuff =
      !(ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK) &&
      (assoc(b_2x4, ss, calldeflist) ||
       assoc(b_4x2, ss, calldeflist) ||
       assoc(b_2x3, ss, calldeflist) ||
       assoc(b_3x2, ss, calldeflist));

   switch (livemask) {
   case 07474: case 06363:
      // We are in offset lines/columns, i.e. "clumps".
      // See if we can do the call in 2x2 or smaller setups.
      if (forbid_little_stuff ||
          (!assoc(b_2x2, ss, calldeflist) &&
           !assoc(b_1x2, ss, calldeflist) &&
           !assoc(b_2x1, ss, calldeflist) &&
           !assoc(b_1x1, ss, calldeflist)))
         fail("Don't know how to do this call in this setup.");
      if (!matrix_aware) warn(warn__each2x2);
      division_code = (livemask == 07474) ?
         MAPCODE(s2x2,2,MPKIND__OFFS_L_HALF,0) :
            MAPCODE(s2x2,2,MPKIND__OFFS_R_HALF,0);
      return true;
   case 07272: case 06565:
      // We are in "Z"'s.  See if we can do the call in 1x2, 2x1, or 1x1 setups.
      // We do not allow 2x2 definitions.
      // We do now!!!!  It is required to allow utb in "1/2 press ahead" Z lines.
      if (!forbid_little_stuff &&
          /* See also 32 lines below. */
          /*                        !assoc(b_2x2, ss, calldeflist) &&*/
          (((!(newtb & 001) || assoc(b_1x2, ss, calldeflist)) &&
            (!(newtb & 010) || assoc(b_2x1, ss, calldeflist))) ||
           assoc(b_1x1, ss, calldeflist))) {
         warn(warn__each1x2);
         division_code = (livemask == 07272) ?
            MAPCODE(s1x2,4,MPKIND__OFFS_L_HALF_STAGGER,1) :
            MAPCODE(s1x2,4,MPKIND__OFFS_R_HALF_STAGGER,1);

         return true;
      }
   }

   // Check for everyone in either the outer 2 triple lines or the
   // center triple line, and able to do the call in 1x4 or smaller setups.

   if (((livemask & 01717) == 0 || (livemask & 06060) == 0) &&
       !forbid_little_stuff &&
       (assoc(b_1x4, ss, calldeflist) ||
        assoc(b_4x1, ss, calldeflist) ||
        assoc(b_1x2, ss, calldeflist) ||
        assoc(b_2x1, ss, calldeflist) ||
        assoc(b_1x1, ss, calldeflist))) {
      if (!matrix_aware) warn(warn__each1x4);
      division_code = MAPCODE(s1x4,3,MPKIND__SPLIT,1);
      return true;
   }

   // Setup is randomly populated.  See if we have 1x2/1x1 definitions.
   // If so, divide the 3x4 into 3 1x4's.  We accept 1x2/2x1 definitions if the call is
   // "matrix aware" (it knows that 1x4's are what it wants) or if the facing directions
   // are such that that would win anyway.
   // We are also more lenient about "run", which we detect through
   // CAF__LATERAL_TO_SELECTEES.  See test nf35.
   //
   // This is not the right way to do this.  The right way would be,
   // in general, to try various recursive splittings and check that the call
   // could be done on the various *fully occupied* subparts.  So, for example,
   // in the case that arises in nf35, we have a 3x4 that consists of a center
   // column of 6 and two outlyers, who are in fact the runners.  We split
   // into 1x4's and then into 1x2's and 1x1's, guided by the live people.
   // For those in the centers column not adjacent to the outlyers, we have
   // to divide to 1x1's.  The call "anyone run" is arranged to be legal on
   // a 1x1 as long as that person isn't selected.  Now the outlyers and
   // their adjacent center form a fully live 1x2.  In that 1x2, the "run"
   // call is legal, when one selectee and one non-selectee.
   //
   // But that's a pretty ambitious change.

   if (assoc(b_1x1, ss, calldeflist) ||
       ((!forbid_little_stuff &&
        (((ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) ||
          (calldeflist->callarray_flags & CAF__LATERAL_TO_SELECTEES)) &&
         ((!(newtb & 010) ||
           assoc(b_1x2, ss, calldeflist) ||
           assoc(b_1x4, ss, calldeflist)) &&
          (!(newtb & 001) ||
           assoc(b_2x1, ss, calldeflist) ||
           assoc(b_4x1, ss, calldeflist))))) ||
        (matrix_aware &&
         (assoc(b_1x2, ss, calldeflist) ||
          assoc(b_2x1, ss, calldeflist))))) {
      division_code = MAPCODE(s1x4,3,MPKIND__SPLIT,1);
      return true;
   }
   else if (!forbid_little_stuff &&
       (((!(newtb & 010) || assoc(b_3x1, ss, calldeflist)) &&
         (!(newtb & 001) || assoc(b_1x3, ss, calldeflist))))) {
      division_code = MAPCODE(s1x3,4,MPKIND__SPLIT,1);
      return true;
   }

   // Now check for something that can be fudged into a "quarter tag"
   // (which includes diamonds).  Note whether we fudged,
   // since some calls do not tolerate it.
   // But don't do this if the "points" are all outside and their orientation
   // is in triple lines.  Or if we would have to move someone and the call
   // does not permit the fudging.

   if ((!(callflags1 & CFLAG1_FUDGE_TO_Q_TAG) && livemask != 06666) ||
       (livemask == 07171 &&
        ((ss->people[0].id1 & ss->people[3].id1 &
          ss->people[6].id1 & ss->people[9].id1) & 1)))
      fail("Can't do this call from arbitrary 3x4 setup.");

   bool really_fudged = false;
   setup sss = *ss;

   expand::compress_setup(s_qtg_3x4, &sss);

   if (ss->people[0].id1) {
      if (ss->people[1].id1) fail("Can't do this call from arbitrary 3x4 setup.");
      else copy_person(&sss, 0, ss, 0);
      really_fudged = true;
   }

   if (ss->people[3].id1) {
      if (ss->people[2].id1) fail("Can't do this call from arbitrary 3x4 setup.");
      else copy_person(&sss, 1, ss, 3);
      really_fudged = true;
   }

   if (ss->people[6].id1) {
      if (ss->people[7].id1) fail("Can't do this call from arbitrary 3x4 setup.");
      else copy_person(&sss, 4, ss, 6);
      really_fudged = true;
   }

   if (ss->people[9].id1) {
      if (ss->people[8].id1) fail("Can't do this call from arbitrary 3x4 setup.");
      else copy_person(&sss, 5, ss, 9);
      really_fudged = true;
   }

   sss.cmd.cmd_misc_flags |= CMD_MISC__DISTORTED;
   move(&sss, really_fudged, result);
   ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;
   result->result_flags.clear_split_info();
   return false;
}


static bool handle_4x4_division(
   setup *ss, uint32 callflags1, uint32 newtb, uint32 livemask,
   uint32 & division_code,            // We write over this.
   int & finalrot,                    // We write over this.
   callarray *calldeflist, bool matrix_aware)
{
   bool forbid_little_stuff;
   uint32 nxnbits =
      ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_NXNMASK|INHERITFLAG_MXNMASK);

   // The call has no applicable 4x4 definition.
   // First, check whether it has 2x4/4x2 definitions, and divide the setup if so,
   // and if the call permits it.

   if ((callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) ||
       (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
      if ((!(newtb & 010) || assoc(b_4x2, ss, calldeflist)) &&
          (!(newtb & 001) || assoc(b_2x4, ss, calldeflist))) {
         // Split to left and right halves.
         finalrot++;
         division_code = MAPCODE(s2x4,2,MPKIND__SPLIT,1);
         return true;
      }
      else if ((!(newtb & 001) || assoc(b_4x2, ss, calldeflist)) &&
               (!(newtb & 010) || assoc(b_2x4, ss, calldeflist))) {
         // Split to bottom and top halves.
         division_code = MAPCODE(s2x4,2,MPKIND__SPLIT,1);
         return true;
      }
   }

   // Look for 3x1 triangles.

   if ((callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) ||
       (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) ||
       nxnbits == INHERITFLAGMXNK_1X3 ||
       nxnbits == INHERITFLAGMXNK_3X1 ||
       nxnbits == INHERITFLAGNXNK_3X3) {
      uint32 assocstuff = 0;
      if (assoc(b_3x2, ss, calldeflist)) assocstuff |= 001;
      if (assoc(b_2x3, ss, calldeflist)) assocstuff |= 010;

      if (livemask == 0x6969 &&
          (!(newtb & 001) || (assocstuff&001)) &&
          (!(newtb & 010) || (assocstuff&010))) {
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_R_THIRD,1);
         return true;
      }
      else if (livemask == 0x9696 &&
          (!(newtb & 010) || (assocstuff&001)) &&
          (!(newtb & 001) || (assocstuff&010))) {
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_R_THIRD,1);
         finalrot++;
         return true;
      }
      else if (livemask == 0xF0F0 &&
          (!(newtb & 001) || (assocstuff&001)) &&
          (!(newtb & 010) || (assocstuff&010))) {
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_L_THIRD,1);
         return true;
      }
      else if (livemask == 0x0F0F &&
          (!(newtb & 010) || (assocstuff&001)) &&
          (!(newtb & 001) || (assocstuff&010))) {
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_L_THIRD,1);
         finalrot++;
         return true;
      }
      else if (livemask == 0xACAC &&
               (((newtb & 011) == 001 && (assocstuff==001)) ||
                ((newtb & 011) == 010 && (assocstuff==010)))) {
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_L_THIRD,1);
         return true;
      }
      else if (livemask == 0xCACA &&
               (((newtb & 011) == 001 && (assocstuff==001)) ||
                ((newtb & 011) == 010 && (assocstuff==010)))) {
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_R_THIRD,1);
         return true;
      }
      else if (livemask == 0xACAC &&
               (((newtb & 011) == 010 && (assocstuff==001)) ||
                ((newtb & 011) == 001 && (assocstuff==010)))) {
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_R_THIRD,1);
         finalrot++;
         return true;
      }
      else if (livemask == 0xCACA &&
               (((newtb & 011) == 010 && (assocstuff==001)) ||
                ((newtb & 011) == 001 && (assocstuff==010)))) {
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_L_THIRD,1);
         finalrot++;
         return true;
      }
   }

   // The only other way this can be legal is if we can identify
   // smaller setups of all real people and can do the call on them.
   // For example, if the outside phantom lines are fully occupied
   // and the inside ones empty, we could do a swing-thru.
   // We also identify Z's from which we can do "Z axle".

   // But if user said "16 matrix" or something, do don't do implicit division.

   if (!(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
      switch (livemask) {
      case 0xD2D2: finalrot++;   // The 8-way map can only take one occupation.
      case 0x2D2D:
         division_code = MAPCODE(s1x1,8,MPKIND__QTAG8_WITH_45_ROTATION,0);
         return true;
      case 0x6666:
         division_code = MAPCODE(s1x2,4,MPKIND__4_EDGES_FROM_4X4,0);
         return true;
      case 0xAAAA:
         division_code = spcmap_4x4_spec0;
         return true;
      case 0xCCCC:
         division_code = spcmap_4x4_spec1;
         return true;
      case 0x3333:
         division_code = spcmap_4x4_spec2;
         return true;
      case 0x5555:
         division_code = spcmap_4x4_spec3;
         return true;
      case 0x8787: finalrot++;
      case 0x7878:
         division_code = spcmap_4x4_spec4;
         return true;
      case 0x1E1E: finalrot++;
      case 0xE1E1:
         division_code = spcmap_4x4_spec5;
         return true;
      case 0xA3A3: case 0x5C5C: finalrot++;
      case 0x3A3A: case 0xC5C5:
         division_code = MAPCODE(s1x4,4,MPKIND__SPLIT,1);
         return true;
      case 0x7171:
         division_code = spcmap_4x4_ns;
         warn(warn__each1x4);
         return true;
      case 0x1717:
         division_code = spcmap_4x4_ew;
         warn(warn__each1x4);
         return true;
      case 0x4E4E: case 0x8B8B:
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_L_THIRD,1);
         finalrot++;
         goto handle_z;
      case 0xE4E4: case 0xB8B8:
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_L_THIRD,1);
         goto handle_other_z;
      case 0xA6A6: case 0x9C9C:
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_R_THIRD,1);
         finalrot++;
         goto handle_z;
      case 0x6A6A: case 0xC9C9:
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_R_THIRD,1);
         goto handle_other_z;
      case 0x4B4B: case 0xB4B4:
         // We are in "clumps".  See if we can do the call in 2x2 or smaller setups.

         if (!assoc(b_1x1, ss, calldeflist) &&
             !assoc(b_2x2, ss, calldeflist) &&
             !assoc(b_1x2, ss, calldeflist) &&
             !assoc(b_2x1, ss, calldeflist) &&
             !assoc(b_1x1, ss, calldeflist))
            fail("Don't know how to do this call in this setup.");

         if (!matrix_aware) warn(warn__each2x2);
         division_code = (livemask == 0x4B4B) ?
            MAPCODE(s2x2,2,MPKIND__OFFS_L_FULL,0) : MAPCODE(s2x2,2,MPKIND__OFFS_R_FULL,0);
         return true;
      }
   }

 dont_handle_z:

   // Setup is randomly populated.  See if we have 1x2/1x1 definitions, but no 2x2.
   // If so, divide the 4x4 into 2 2x4's.

   forbid_little_stuff =
      assoc(b_2x4, ss, calldeflist) ||
      assoc(b_4x2, ss, calldeflist) ||
      assoc(b_2x3, ss, calldeflist) ||
      assoc(b_3x2, ss, calldeflist) ||
      assoc(b_dmd, ss, calldeflist) ||
      assoc(b_pmd, ss, calldeflist) ||
      assoc(b_qtag, ss, calldeflist) ||
      assoc(b_pqtag, ss, calldeflist);

   if (!forbid_little_stuff && !assoc(b_2x2, ss, calldeflist) &&
       (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
      if ((assoc(b_1x2, ss, calldeflist) ||
           assoc(b_2x1, ss, calldeflist) ||
           assoc(b_1x1, ss, calldeflist))) {
         // Without a lot of examination of facing directions, and whether the call
         // has 1x2 vs. 2x1 definitions, and all that, we don't know which axis
         // to use when dividing it.  But any division into 2 2x4's is safe,
         // and code elsewhere will make the tricky decisions later.
         division_code = MAPCODE(s2x4,2,MPKIND__SPLIT,1);
         return true;
      }
      else if ((callflags1 & CFLAG1_12_16_MATRIX_MEANS_SPLIT) &&
               ((!(newtb & 010) || assoc(b_1x4, ss, calldeflist)) &&
                (!(newtb & 001) || assoc(b_4x1, ss, calldeflist)))) {
         division_code = MAPCODE(s1x4,4,MPKIND__SPLIT,1);
         return true;
      }
      else if ((callflags1 & CFLAG1_12_16_MATRIX_MEANS_SPLIT) &&
               ((!(newtb & 001) || assoc(b_1x4, ss, calldeflist)) &&
                (!(newtb & 010) || assoc(b_4x1, ss, calldeflist)))) {
         finalrot++;
         division_code = MAPCODE(s1x4,4,MPKIND__SPLIT,1);
         return true;
      }
   }

   // If the call has a 1x1 definition, and we can't do anything else,
   // divide it up into all 16 people.
   if (assoc(b_1x1, ss, calldeflist)) {
      division_code = spcmap_4x4_1x1;
      return true;
   }

   fail("You must specify a concept.");

 handle_z:

   // If this changes shape (as it will in the only known case
   // of this -- Z axle), divided_setup_move will give a warning
   // about going to a parallelogram, since we did not start
   // with 50% offset, though common practice says that a
   // parallelogram is the correct result.  If the call turns out
   // not to be a shape-changer, no warning will be given.  If
   // the call is a shape changer that tries to go into a setup
   // other than a parallelogram, divided_setup_move will raise
   // an error.
   ss->cmd.cmd_misc_flags |= CMD_MISC__OFFSET_Z;
   if ((callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) &&
       (!(newtb & 010) || assoc(b_3x2, ss, calldeflist)) &&
       (!(newtb & 001) || assoc(b_2x3, ss, calldeflist)))
      return true;
   goto dont_handle_z;

 handle_other_z:
   ss->cmd.cmd_misc_flags |= CMD_MISC__OFFSET_Z;
   if ((callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) &&
       (!(newtb & 010) || assoc(b_2x3, ss, calldeflist)) &&
       (!(newtb & 001) || assoc(b_3x2, ss, calldeflist)))
      return true;
   goto dont_handle_z;
}


static bool handle_4x6_division(
   setup *ss, uint32 callflags1, uint32 newtb, uint32 livemask,
   uint32 & division_code,            // We write over this.
   callarray *calldeflist, bool matrix_aware)
{
   // The call has no applicable 4x6 definition.
   // First, check whether it has 2x6/6x2 definitions,
   // and divide the setup if so, and if the call permits it.

   if ((callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) ||
       (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
      // If the call wants a 2x6 or 3x4, do it.
      if ((!(newtb & 010) || assoc(b_2x6, ss, calldeflist)) &&
          (!(newtb & 001) || assoc(b_6x2, ss, calldeflist))) {
         division_code = MAPCODE(s2x6,2,MPKIND__SPLIT,1);
         return true;
      }
      else if ((!(newtb & 010) || assoc(b_4x3, ss, calldeflist)) &&
               (!(newtb & 001) || assoc(b_3x4, ss, calldeflist))) {
         division_code = MAPCODE(s3x4,2,MPKIND__SPLIT,1);
         return true;
      }

      // Look for special Z's.
      if (!(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
         switch (livemask) {
         case 043204320: case 023402340:
            division_code = MAPCODE(s2x3,2,MPKIND__OFFS_L_FULL,1);
            if ((!(newtb & 010) || assoc(b_3x2, ss, calldeflist)) &&
                (!(newtb & 001) || assoc(b_2x3, ss, calldeflist))) {
               ss->cmd.cmd_misc_flags |= CMD_MISC__OFFSET_Z;
               return true;
            }
            break;
         case 061026102: case 062016201:
            division_code = MAPCODE(s2x3,2,MPKIND__OFFS_R_FULL,1);
            if ((!(newtb & 010) || assoc(b_3x2, ss, calldeflist)) &&
                (!(newtb & 001) || assoc(b_2x3, ss, calldeflist))) {
               ss->cmd.cmd_misc_flags |= CMD_MISC__OFFSET_Z;
               return true;
            }
            break;
         }
      }

      // If the call wants a 2x3 and we didn't find one of the special Z's do it.

      if ((!(newtb & 010) || assoc(b_2x3, ss, calldeflist)) &&
          (!(newtb & 001) || assoc(b_3x2, ss, calldeflist))) {
         division_code = MAPCODE(s3x4,2,MPKIND__SPLIT,1);
         return true;
      }
   }

   // The only other way this can be legal is if we can identify
   // smaller setups of all real people and can do the call on them.

   // But if user said "16 matrix" or something, do don't do implicit division.

   if (!(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
      bool forbid_little_stuff;

      switch (livemask) {
      case 0xC03C03: case 0x0F00F0:
         // We are in "clumps".  See if we can do the call in 2x2 or smaller setups.
         if (!assoc(b_2x2, ss, calldeflist) &&
             !assoc(b_1x2, ss, calldeflist) &&
             !assoc(b_2x1, ss, calldeflist) &&
             !assoc(b_1x1, ss, calldeflist))
            fail("Don't know how to do this call in this setup.");
         if (!matrix_aware) warn(warn__each2x2);
         division_code = MAPCODE(s2x2,6,MPKIND__SPLIT_OTHERWAY_TOO,0);
         return true;
      case 0xA05A05: case 0x168168:
         // We are in "offset stairsteps".  See if we can do the call in 1x2 or smaller setups.
         forbid_little_stuff =
            !(ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK) &&
            (assoc(b_2x4, ss, calldeflist) ||
             assoc(b_4x2, ss, calldeflist) ||
             assoc(b_dmd, ss, calldeflist) ||
             assoc(b_pmd, ss, calldeflist) ||
             assoc(b_qtag, ss, calldeflist) ||
             assoc(b_pqtag, ss, calldeflist));

         if (forbid_little_stuff ||
             (!assoc(b_1x2, ss, calldeflist) &&
              !assoc(b_2x1, ss, calldeflist) &&
              !assoc(b_1x1, ss, calldeflist)))
            fail("Don't know how to do this call in this setup.");
         if (!matrix_aware) warn(warn__each1x2);
         // This will do.
         division_code = MAPCODE(s1x4,6,MPKIND__SPLIT,1);
         return true;
      case 0x1D01D0: case 0xE02E02:
         // We are in "diamond clumps".  See if we can do the call in diamonds.
         forbid_little_stuff =
            !(ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK) &&
            (assoc(b_2x4, ss, calldeflist) || assoc(b_4x2, ss, calldeflist));

         if (forbid_little_stuff ||
             (!assoc(b_dmd, ss, calldeflist) &&
              !assoc(b_pmd, ss, calldeflist)))
            fail("Don't know how to do this call in this setup.");
         if (!matrix_aware) warn(warn__eachdmd);

         division_code = (livemask == 0x1D01D0) ?
            MAPCODE(sdmd,2,MPKIND__OFFS_R_FULL,1) : MAPCODE(sdmd,2,MPKIND__OFFS_L_FULL,1);
         return true;
      }
   }

   return false;
}


static bool handle_3x8_division(
   setup *ss, uint32 callflags1, uint32 newtb, uint32 livemask,
   uint32 & division_code,            // We write over this.
   callarray *calldeflist, bool matrix_aware)
{
   // The call has no applicable 3x8 definition.
   // First, check whether it has 3x4/4x3/2x3/3x2 definitions,
   // and divide the setup if so, and if the call permits it.

   if ((callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) ||
       (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
      if ((!(newtb & 010) ||
           assoc(b_3x4, ss, calldeflist) ||
           assoc(b_3x2, ss, calldeflist)) &&
          (!(newtb & 001) ||
           assoc(b_4x3, ss, calldeflist) ||
           assoc(b_2x3, ss, calldeflist))) {
         division_code = MAPCODE(s3x4,2,MPKIND__SPLIT,0);
         return true;
      }
   }

   // The only other way this can be legal is if we can identify
   // smaller setups of all real people and can do the call on them.

   // But if user said "16 matrix" or something, do don't do implicit division.

   if (!(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
      switch (livemask) {
      case 0x00F00F: case 0x0F00F0:
         // We are in 1x4's in the corners.  See if we can do the call in 1x4
         // or smaller setups.
         if (!assoc(b_1x4, ss, calldeflist) &&
             !assoc(b_4x1, ss, calldeflist) &&
             !assoc(b_1x2, ss, calldeflist) &&
             !assoc(b_2x1, ss, calldeflist) &&
             !assoc(b_1x1, ss, calldeflist))
            fail("Don't know how to do this call in this setup.");
         if (!matrix_aware) warn(warn__each1x4);
         division_code = MAPCODE(s1x4,6,MPKIND__SPLIT_OTHERWAY_TOO,1);
         return true;
      }
   }

   return false;
}


static bool handle_2x12_division(
   setup *ss, uint32 callflags1, uint32 newtb, uint32 livemask,
   uint32 & division_code,            // We write over this.
   callarray *calldeflist, bool matrix_aware)
{
   bool forbid_little_stuff;

   // The call has no applicable 2x12 definition.
   // First, check whether it has 2x6/6x2 definitions,
   // and divide the setup if so, and if the call permits it.

   if ((callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) ||
       (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
      // If the call wants a 2x6, do it.
      if ((!(newtb & 010) || assoc(b_6x2, ss, calldeflist)) &&
          (!(newtb & 001) || assoc(b_2x6, ss, calldeflist))) {
         division_code = MAPCODE(s2x6,2,MPKIND__SPLIT,0);
         return true;
      }
   }

   // The only other way this can be legal is if we can identify
   // smaller setups of all real people and can do the call on them.

   // But if user said "16 matrix" or something, do don't do implicit division.

   if (!(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
      switch (livemask) {
      case 0x00F00F: case 0xF00F00:
         forbid_little_stuff =
            !(ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK) &&
            (assoc(b_2x4, ss, calldeflist) ||
             assoc(b_4x2, ss, calldeflist) ||
             assoc(b_dmd, ss, calldeflist) ||
             assoc(b_pmd, ss, calldeflist) ||
             assoc(b_qtag, ss, calldeflist) ||
             assoc(b_pqtag, ss, calldeflist));

         // We are in 1x4's in the corners.  See if we can do the call in 1x4
         // or smaller setups.
         if (forbid_little_stuff ||
             (!assoc(b_1x4, ss, calldeflist) &&
              !assoc(b_4x1, ss, calldeflist) &&
              !assoc(b_1x2, ss, calldeflist) &&
              !assoc(b_2x1, ss, calldeflist) &&
              !assoc(b_1x1, ss, calldeflist)))
            fail("Don't know how to do this call in this setup.");
         if (!matrix_aware) warn(warn__each1x4);
         // This will do.
         division_code = MAPCODE(s2x4,3,MPKIND__SPLIT,0);
         return true;
      }
   }

   return false;
}


static int divide_the_setup(
   setup *ss,
   uint32 *newtb_p,
   callarray *calldeflist,
   int *desired_elongation_p,
   setup *result) THROW_DECL
{
   int i;
   callarray *have_1x2, *have_2x1;
   uint32 division_code = ~0U;
   uint32 newtb = *newtb_p;
   uint32 callflags1 = ss->cmd.callspec->the_defn.callflags1;
   final_and_herit_flags final_concepts = ss->cmd.cmd_final_flags;
   setup_command conc_cmd;
   uint32 must_do_mystic = ss->cmd.cmd_misc2_flags & CMD_MISC2__CTR_END_KMASK;
   calldef_schema conc_schema = schema_concentric;
   bool matrix_aware =
         (callflags1 & CFLAG1_12_16_MATRIX_MEANS_SPLIT) &&
         (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX);
   int finalrot = 0;
   bool maybe_horrible_hinge = false;

   uint32 nxnbits =
      ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_NXNMASK|INHERITFLAG_MXNMASK);

   // It will be helpful to have a mask of where the live people are.
   uint32 livemask = little_endian_live_mask(ss);

   // Take care of "snag" and "mystic".  "Central" is illegal, and was already caught.
   // We first limit it to just the few setups for which it can possibly be legal, to make
   // it easier to test later.

   if (must_do_mystic) {
      switch (ss->kind) {
      case s_rigger:
      case s_qtag:
      case s_bone:
      case s1x8:
      case s2x4:
      case s_crosswave:
         break;
      default:
         fail("Can't do \"snag/mystic\" with this call.");
      }
   }

   bool specialpass;

   switch (ss->kind) {
      uint32 tbi, tbo;    // Many clauses will use these.
      bool temp;

   case s_thar:
      if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
         fail("Can't split the setup.");
      division_code = MAPCODE(s1x2,4,MPKIND__4_EDGES,1);
      goto divide_us_no_recompute;
   case s_alamo:
      if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
         fail("Can't split the setup.");
      division_code = MAPCODE(s1x2,4,MPKIND__4_EDGES,0);
      goto divide_us_no_recompute;
   case s2x8:
      // The call has no applicable 2x8 or 8x2 definition.

      // Check whether it has 2x4/4x2/1x8/8x1 definitions, and divide the setup if so,
      // or if the caller explicitly said "2x8 matrix".

      temp =
         (callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) &&
         (ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_NXNMASK)) != INHERITFLAGNXNK_4X4;

      if (temp ||
          ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_16_MATRIX) ||
          (ss->cmd.cmd_misc_flags & (CMD_MISC__EXPLICIT_MATRIX|CMD_MISC__PHANTOMS))) {
         if ((!(newtb & 010) || assoc(b_2x4, ss, calldeflist)) &&
             (!(newtb & 001) || assoc(b_4x2, ss, calldeflist))) {
            division_code = MAPCODE(s2x4,2,MPKIND__SPLIT,0);

            // I consider it an abomination to call such a thing as "2x8 matrix
            // swing-o-late" from 2x8 lines, expecting people to do the call
            // in split phantom boxes, or "2x8 matrix grand mix", expecting
            // twin tidal lines.  However, we allow it for consistency with such
            // things as "2x6 matrix double play" from 2x6 columns, expecting
            // 12 matrix divided columns.  The correct usage should involve the
            // explicit concepts "split phantom boxes", "phantom tidal lines",
            // or "12 matrix divided columns", as appropriate.

            // If database said to split, don't give warning, unless said "4x4".
            if (!temp) warn(warn__split_to_2x4s);
            goto divide_us_no_recompute;
         }
         else if ((!(newtb & 010) ||
                   assoc(b_1x4, ss, calldeflist) ||
                   assoc(b_1x8, ss, calldeflist)) &&
                  (!(newtb & 001) ||
                   assoc(b_4x1, ss, calldeflist) ||
                   assoc(b_8x1, ss, calldeflist))) {
            division_code = MAPCODE(s1x8,2,MPKIND__SPLIT,1);
            // See comment above about abomination.

            // If database said to split, don't give warning, unless said "4x4".
            if (!temp) warn(warn__split_to_1x8s);
            goto divide_us_no_recompute;
         }
      }

      // Setup is randomly populated.  See if we have 1x2/1x1 definitions, but no 2x2,
      // 2x3, or other unseemly things.
      // If so, divide the 2x8 into 2 2x4's and proceed from there.
      // But only if user specified phantoms or matrix.

      if ((ss->cmd.cmd_misc_flags & (CMD_MISC__EXPLICIT_MATRIX|CMD_MISC__PHANTOMS)) &&
          !assoc(b_2x4, ss, calldeflist) &&
          !assoc(b_4x2, ss, calldeflist) &&
          !assoc(b_2x3, ss, calldeflist) &&
          !assoc(b_3x2, ss, calldeflist) &&
          !assoc(b_dmd, ss, calldeflist) &&
          !assoc(b_pmd, ss, calldeflist) &&
          !assoc(b_qtag, ss, calldeflist) &&
          !assoc(b_pqtag, ss, calldeflist) &&
          !assoc(b_2x2, ss, calldeflist) &&
          (assoc(b_1x2, ss, calldeflist) ||
           assoc(b_2x1, ss, calldeflist) ||
           assoc(b_1x1, ss, calldeflist))) {
         // Without a lot of examination of facing directions, and whether the call
         // has 1x2 vs. 2x1 definitions, and all that, we don't know which axis
         // to use when dividing it.  But any division into 2 2x4's is safe,
         // and code elsewhere will make the tricky decisions later.
         division_code = MAPCODE(s2x4,2,MPKIND__SPLIT,0);
         goto divide_us_no_recompute;
      }

      // Otherwise, the only way this can be legal is if we can identify
      // smaller setups of all real people and can do the call on them.  For
      // example, we will look for 1x4 setups, so we could do things like
      // swing thru from a totally offset parallelogram.

      switch (livemask) {
      case 0xF0F0:    // A parallelogram.
         division_code = MAPCODE(s1x4,2,MPKIND__OFFS_R_FULL,1);
         warn(warn__each1x4);
         break;
      case 0x0F0F:    // A parallelogram.
         division_code = MAPCODE(s1x4,2,MPKIND__OFFS_L_FULL,1);
         warn(warn__each1x4);
         break;
      case 0xC3C3:    // The outer quadruple boxes.
         division_code = MAPCODE(s2x2,4,MPKIND__SPLIT,0);
         warn(warn__each2x2);
         break;
      }

      goto divide_us_no_recompute;
   case s2x6:
      // The call has no applicable 2x6 or 6x2 definition.

      // See if this call has applicable 2x8 definitions and matrix expansion is permitted.
      // If so, that is what we must do.

      if (!(ss->cmd.cmd_misc_flags & (CMD_MISC__NO_EXPAND_MATRIX|CMD_MISC__MUST_SPLIT_MASK)) &&
          (!(newtb & 010) || assoc(b_2x8, ss, calldeflist)) &&
          (!(newtb & 001) || assoc(b_8x2, ss, calldeflist))) {
         do_matrix_expansion(ss, CONCPROP__NEEDK_2X8, true);
         // Should never fail, but we don't want a loop.
         if (ss->kind != s2x8) fail("Failed to expand to 2X8.");
         return 2;        // And try again.
      }

      // Next, check whether it has 1x3/3x1/2x3/3x2/1x6/6x1 definitions,
      // and divide the setup if so, and if the call permits it.  This is important
      // for permitting "Z axle" from a 2x6 but forbidding "circulate".
      // We also enable this if the caller explicitly said "2x6 matrix".

      temp =
         (callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) ||
         ((callflags1 & CFLAG1_SPLIT_IF_Z) &&
          nxnbits != INHERITFLAGNXNK_3X3 &&
          (livemask == 03333 || livemask == 06666));

      if (temp ||
          ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_12_MATRIX) ||
          (ss->cmd.cmd_misc_flags & (CMD_MISC__EXPLICIT_MATRIX|CMD_MISC__PHANTOMS))) {
         if ((!(newtb & 010) || assoc(b_2x3, ss, calldeflist)) &&
             (!(newtb & 001) || assoc(b_3x2, ss, calldeflist))) {
            division_code = MAPCODE(s2x3,2,MPKIND__SPLIT,0);
            // See comment above about abomination.
            // If database said to split, don't give warning, unless said "3x3".
            if (!temp) warn(warn__split_to_2x3s);
            goto divide_us_no_recompute;
         }
         else if ((!(newtb & 010) || assoc(b_1x6, ss, calldeflist)) &&
                  (!(newtb & 001) || assoc(b_6x1, ss, calldeflist))) {
            division_code = MAPCODE(s1x6,2,MPKIND__SPLIT,1);
            // See comment above about abomination.
            // If database said to split, don't give warning, unless said "3x3".
            if (!temp) warn(warn__split_to_1x6s);
            goto divide_us_no_recompute;
         }
         else if ((!(newtb & 010) || assoc(b_1x3, ss, calldeflist)) &&
                  (!(newtb & 001) || assoc(b_3x1, ss, calldeflist))) {
            division_code = MAPCODE(s1x3,4,MPKIND__SPLIT_OTHERWAY_TOO,0);
            // See comment above about abomination.
            // If database said to split, don't give warning, unless said "3x3".
            if (!temp) warn(warn__split_to_1x3s);
            goto divide_us_no_recompute;
         }
      }

      // Next, check whether it has 1x2/2x1/2x2/1x1 definitions,
      // and we are doing some phantom concept.
      // Divide the setup into 3 boxes if so.

      if ((ss->cmd.cmd_misc_flags & CMD_MISC__PHANTOMS) ||
          (callflags1 & CFLAG1_SPLIT_LARGE_SETUPS)) {
         if (assoc(b_2x2, ss, calldeflist) ||
             assoc(b_1x2, ss, calldeflist) ||
             assoc(b_2x1, ss, calldeflist) ||
             assoc(b_1x1, ss, calldeflist)) {
            division_code = MAPCODE(s2x2,3,MPKIND__SPLIT,0);
            if (matrix_aware)                     // ***** Maybe this should be done more generally.
               warn(warn__really_no_eachsetup);
            goto divide_us_no_recompute;
         }
      }

      // Otherwise, the only way this can be legal is if we can identify
      // smaller setups of all real people and can do the call on them.  For
      // example, we will look for 1x4 setups, so we could do things like
      // swing thru from a parallelogram.

      switch (livemask) {
      case 07474:    // a parallelogram
         division_code = MAPCODE(s1x4,2,MPKIND__OFFS_R_HALF,1);
         warn(warn__each1x4);
         break;
      case 01717:    // a parallelogram
         division_code = MAPCODE(s1x4,2,MPKIND__OFFS_L_HALF,1);
         warn(warn__each1x4);
         break;
      case 06060: case 00303:
      case 06363:    // the outer triple boxes
         division_code = MAPCODE(s2x2,3,MPKIND__SPLIT,0);
         warn(warn__each2x2);
         break;
      case 05555: case 04141: case 02222:
         // Split into 6 stacked 1x2's.
         division_code = MAPCODE(s1x2,6,MPKIND__SPLIT,1);
         warn(warn__each1x2);
         break;
      case 0xDB6: case 0x6DB:
         warn(warn__split_to_2x3s);
         division_code = MAPCODE(s2x3,2,MPKIND__SPLIT,0);
         break;
      }

      goto divide_us_no_recompute;
   case spgdmdcw:
      division_code = MAPCODE(sdmd,2,MPKIND__OFFS_R_HALF,1);
      warn(warn__eachdmd);
      goto divide_us_no_recompute;
   case spgdmdccw:
      division_code = MAPCODE(sdmd,2,MPKIND__OFFS_L_HALF,1);
      warn(warn__eachdmd);
      goto divide_us_no_recompute;
   case s2x7:
      // The call has no applicable 2x7 or 7x2 definition.
      // Check for a 75% offset parallelogram,

      switch (livemask) {
      case 0x3C78:
         division_code = MAPCODE(s1x4,2,MPKIND__OFFS_R_THRQ,1);
         warn(warn__each1x4);
         goto divide_us_no_recompute;
      case 0x078F:
         division_code = MAPCODE(s1x4,2,MPKIND__OFFS_L_THRQ,1);
         warn(warn__each1x4);
         goto divide_us_no_recompute;
      }

      break;
   case sdeepqtg:
         /* Check whether it has short6/pshort6 definitions, and divide the setup if so,
            and if the call permits it. */

         if ((callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) ||
             (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
            if ((!(newtb & 010) || assoc(b_short6, ss, calldeflist)) &&
                (!(newtb & 001) || assoc(b_pshort6, ss, calldeflist))) {
               division_code = MAPCODE(s_short6,2,MPKIND__SPLIT,0);
               goto divide_us_no_recompute;
            }
         }
         break;
   case s4x5:
      // **** actually want to test that call says "occupied_as_3x1tgl".

      if ((callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) ||
          (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
         if ((!(newtb & 010) || assoc(b_2x3, ss, calldeflist)) &&
             (!(newtb & 001) || assoc(b_3x2, ss, calldeflist))) {
            if (livemask == 0x3A0E8 || livemask == 0x1705C) {
               division_code = spcmap_tgl451;
               goto divide_us_no_recompute;
            }
            else if (livemask == 0x41D07 || livemask == 0xE0B82) {
               division_code = spcmap_tgl452;
               goto divide_us_no_recompute;
            }
         }
      }
      else {
         if ((!(newtb & 010) || assoc(b_1x2, ss, calldeflist)) &&
             (!(newtb & 001) || assoc(b_2x1, ss, calldeflist))) {
            if (livemask == 0x360D8) {
               division_code = MAPCODE(s1x2,4,MPKIND__OFFS_R_HALF,1);
               goto divide_us_no_recompute;
            }
            else if (livemask == 0x60D83) {
               division_code = MAPCODE(s1x2,4,MPKIND__OFFS_L_HALF,1);
               goto divide_us_no_recompute;
            }
         }
      }

      break;
   case s1p5x8:
      // This is a phony setup, allowed only so that we can have people temporarily
      // in 50% offset 1x4's that are offset the impossible way.

      switch (livemask) {
      case 0xF0F0:
         division_code = MAPCODE(s1x4,2,MPKIND__OFFS_L_HALF,0);
         break;
      case 0x0F0F:
         division_code = MAPCODE(s1x4,2,MPKIND__OFFS_R_HALF,0);
         break;
      case 0xCCCC:
         division_code = MAPCODE(s1x2,4,MPKIND__OFFS_L_HALF_STAGGER,0);
         break;
      case 0x3333:
         division_code = MAPCODE(s1x2,4,MPKIND__OFFS_R_HALF_STAGGER,0);
         break;
      }

      goto divide_us_no_recompute;
   case s1x12:
      // The call has no applicable 1x12 or 12x1 definition.

      // See if this call has applicable 1x16 definitions and matrix expansion is permitted.
      // If so, that is what we must do.

      if (  !(ss->cmd.cmd_misc_flags & CMD_MISC__NO_EXPAND_MATRIX) &&
            (!(newtb & 010) || assoc(b_1x16, ss, calldeflist)) &&
            (!(newtb & 001) || assoc(b_16x1, ss, calldeflist))) {
         do_matrix_expansion(ss, CONCPROP__NEEDK_1X16, true);
         if (ss->kind != s1x16) fail("Failed to expand to 1X16.");  // Should never fail, but we don't want a loop.
         return 2;        // And try again.
      }

      // Check whether it has 1x6/6x1 definitions, and divide the setup if so,
      // and if the caller explicitly said "1x12 matrix".

      temp = (callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) != 0;

      if (temp ||
          ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_12_MATRIX) ||
          (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
         if ((!(newtb & 010) || assoc(b_1x6, ss, calldeflist)) &&
             (!(newtb & 001) || assoc(b_6x1, ss, calldeflist))) {
            division_code = MAPCODE(s1x6,2,MPKIND__SPLIT,0);
            // See comment above about abomination.
            // If database said to split, don't give warning.
            if (!temp) warn(warn__split_to_1x6s);
            goto divide_us_no_recompute;
         }
         else if ((!(newtb & 010) || assoc(b_1x3, ss, calldeflist)) &&
                  (!(newtb & 001) || assoc(b_3x1, ss, calldeflist))) {
            division_code = MAPCODE(s1x3,4,MPKIND__SPLIT,0);
            // See comment above about abomination.
            warn(warn__split_to_1x3s);
            goto divide_us_no_recompute;
         }
      }

      // Otherwise, the only way this can be legal is if we can identify
      // smaller setups of all real people and can do the call on them.

      switch (livemask) {
      case 01717:    /* outer 1x4's */
         division_code = MAPCODE(s1x4,3,MPKIND__SPLIT,0);
         warn(warn__each1x4);
         break;
      case 06363:    /* center 1x4 and outer 1x2's */
         division_code = MAPCODE(s1x2,6,MPKIND__SPLIT,0);
         warn(warn__each1x2);
         break;
      }

      goto divide_us_no_recompute;
   case s1x16:
      // The call has no applicable 1x16 or 16x1 definition.

      // Check whether it has 1x8/8x1 definitions, and divide the setup if so,
      // and if the caller explicitly said "1x16 matrix".

      temp = (callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) != 0;

      if (temp ||
          ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_16_MATRIX) ||
          (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) {
         if ((!(newtb & 010) || assoc(b_1x8, ss, calldeflist)) &&
             (!(newtb & 001) || assoc(b_8x1, ss, calldeflist))) {
            division_code = MAPCODE(s1x8,2,MPKIND__SPLIT,0);
            // See comment above about abomination.
            // If database said to split, don't give warning.
            if (!temp) warn(warn__split_to_1x8s);
            goto divide_us_no_recompute;
         }
      }
      break;
   case s1x10:
      // See if this call has applicable 1x12 or 1x16 definitions and
      // matrix expansion is permitted.  If so, that is what we must do.
      // These two cases are required to make things like 12 matrix
      // grand swing thru work from a 1x10.

      if (!(ss->cmd.cmd_misc_flags & CMD_MISC__NO_EXPAND_MATRIX) &&
          (!(newtb & 010) || assoc(b_1x12, ss, calldeflist)) &&
          (!(newtb & 001) || assoc(b_12x1, ss, calldeflist))) {
         do_matrix_expansion(ss, CONCPROP__NEEDK_1X12, true);
         if (ss->kind != s1x12) fail("Failed to expand to 1X12.");  // Should never fail, but we don't want a loop.
         return 2;        // And try again.
      }
      else if (livemask == 0x1EF) {
         division_code = MAPCODE(s1x4,2,MPKIND__UNDERLAP,0);
         warn(warn__each1x4);
         goto divide_us_no_recompute;
      }

      // FALL THROUGH!!!!!
   case s1x14:      // WARNING!!  WE FELL THROUGH!!
      /* See if this call has applicable 1x16 definitions and matrix expansion is permitted.
            If so, that is what we must do. */

      if (  !(ss->cmd.cmd_misc_flags & CMD_MISC__NO_EXPAND_MATRIX) &&
            (!(newtb & 010) || assoc(b_1x16, ss, calldeflist)) &&
            (!(newtb & 001) || assoc(b_16x1, ss, calldeflist))) {
         do_matrix_expansion(ss, CONCPROP__NEEDK_1X16, true);
         if (ss->kind != s1x16) fail("Failed to expand to 1X16.");  // Should never fail, but we don't want a loop.
         return 2;        // And try again.
      }

      if (ss->kind == s1x10) {   /* Can only do this in 1x10, for now. */
         division_code = MAPCODE(s1x2,5,MPKIND__SPLIT,0);

         if (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) {
            if (((newtb & 001) == 0 && assoc(b_1x2, ss, calldeflist)) ||
                ((newtb & 010) == 0 && assoc(b_2x1, ss, calldeflist)))
               goto divide_us_no_recompute;
         }
         else if (assoc(b_1x1, ss, calldeflist))
            goto divide_us_no_recompute;
      }

      break;
   case s3x6:
      /* Check whether it has 2x3/3x2/1x6/6x1 definitions, and divide the setup if so,
            or if the caller explicitly said "3x6 matrix" (not that "3x6 matrix"
            exists at present.) */

         /* We do *NOT* use the "CFLAG1_SPLIT_LARGE_SETUPS" flag.
            We are willing to split to a 12 matrix, but not an 18 matrix. */

      if (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) {
         if ((!(newtb & 010) || assoc(b_3x2, ss, calldeflist)) &&
             (!(newtb & 001) || assoc(b_2x3, ss, calldeflist))) {
            division_code = MAPCODE(s2x3,3,MPKIND__SPLIT,1);
            warn(warn__split_to_2x3s);
            goto divide_us_no_recompute;
         }
         else if ((!(newtb & 010) || assoc(b_1x6, ss, calldeflist)) &&
                  (!(newtb & 001) || assoc(b_6x1, ss, calldeflist))) {
            division_code = MAPCODE(s1x6,3,MPKIND__SPLIT,1);
            warn(warn__split_to_1x6s);
            goto divide_us_no_recompute;
            /* YOW!!  1x3's are hard!  We need a 3x3 formation. */
         }
      }

      /* Otherwise, the only way this can be legal is if we can identify
         smaller setups of all real people and can do the call on them.  For
         example, we will look for 1x2 setups, so we could trade in
         individual couples scattered around. */

      switch (livemask) {
      case 0505505:
      case 0702702:
      case 0207207:
         warn(warn__each1x2);
         // FALL THROUGH!!!!!
      case 0603603:      // WARNING!!  WE FELL THROUGH!!
      case 0306306:
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_R_HALF,0);
         break;
      case 0550550:
      case 0720720:
      case 0270270:
         warn(warn__each1x2);
         // FALL THROUGH!!!!!
      case 0660660:      // WARNING!!  WE FELL THROUGH!!
      case 0330330:
         division_code = MAPCODE(s2x3,2,MPKIND__OFFS_L_HALF,0);
         break;
      }

      goto divide_us_no_recompute;
   case s_c1phan:

      // Check for "twisted split" stuff.

      if (ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_TWISTED) &&
          (ss->cmd.cmd_final_flags.test_finalbits(FINAL__SPLIT_SQUARE_APPROVED |
                                                  FINAL__SPLIT_DIXIE_APPROVED)) &&
          (livemask == 0xAAAA || livemask == 0x5555)) {
         finalrot = newtb & 1;
         division_code = (livemask & 1) ?
            HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,0,s_trngl4,0x8) :
            HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,0,s_trngl4,0x2);
         goto divide_us_no_recompute;
      }

      // Check for division into trngl4's.  This is ambiguous, so we have
      // to peek at the call to see whether it is qualifying on something
      // that tells us how to divide.  The only such thing we know about
      // is "ntbone" along with the few listed below.  The only
      // call at present that uses this is "<anyone> start vertical tag".
      // And "dixie style", etc.

      // If that fails, the only way this can be legal is if people are in
      // genuine C1 phantom spots and the call can be done from 1x2's or 2x1's.
      // *** Actually, that isn't so.  We ought to be able to do 1x1 calls from
      // any population at all.

      {
         bool looking_for_lateral_triangle_split = false;
         callarray *t = calldeflist;
         while (t) {
            if ((t->qualifierstuff & QUALBIT__NTBONE) &&
                (((t->qualifierstuff & QUALBIT__QUAL_CODE) == cr_dmd_ctrs_mwv) ||
                 ((t->qualifierstuff & QUALBIT__QUAL_CODE) == cr_dmd_ctrs_mwv_no_mirror) ||
                 ((t->qualifierstuff & QUALBIT__QUAL_CODE) == cr_not_split_dixie) ||
                 ((t->qualifierstuff & QUALBIT__QUAL_CODE) == cr_dmd_ctrs_1f))) {
               looking_for_lateral_triangle_split = true;
               break;
            }

            t = t->next;
         }

         if (looking_for_lateral_triangle_split) {
            if (!(newtb & 010)) {
               // Split vertically, by rotating the setup.
               finalrot++;
            }
            else if (newtb & 001)
               break;    // Can't do it.

            if ((livemask & 0xAAAA) == 0)
               division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,0,s_trngl4,0x8);
            else if ((livemask & 0x5555) == 0)
               division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,0,s_trngl4,0x2);
            else if ((livemask & 0xA55A) == 0)
               division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,0,s_trngl4,0x0);
            else if ((livemask & 0x5AA5) == 0)
               division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,0,s_trngl4,0xA);
            else if ((livemask & 0x55AA) == 0)
               division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,0,s_trngl4,0x5);
            else if ((livemask & 0xAA55) == 0)
               division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,0,s_trngl4,0x7);
         }
         else if ((livemask & 0xAAAA) == 0)
            division_code = MAPCODE(s1x2,4,MPKIND__4_QUADRANTS,0);
         else if ((livemask & 0x5555) == 0)
            division_code = MAPCODE(s1x2,4,MPKIND__4_QUADRANTS,1);
         else if (livemask == 0x0F0F || livemask == 0xF0F0) {
            // Occupied as stars.  Check for a real 8-way split to 1x1's.
            specialpass = false;
            have_1x2 = assoc(b_1x2, ss, calldeflist, &specialpass);
            have_2x1 = assoc(b_2x1, ss, calldeflist, &specialpass);
            if (ss->cmd.cmd_assume.assumption != cr_none) specialpass = true;
            if (have_1x2 || have_2x1) specialpass = true;
            if (livemask == 0x0F0F) finalrot++;   // The 8-way map can only take one occupation.

            // If the planets are auspicious, do a direct 8-way division.
            if (!specialpass) division_code = MAPCODE(s1x1,8,MPKIND__SPLIT_WITH_45_ROTATION_OTHERWAY_TOO,0);
            else division_code = MAPCODE(s_star,4,MPKIND__4_QUADRANTS,0);
         }
         else if ((livemask & 0x55AA) == 0 || (livemask & 0xAA55) == 0 ||
                  (livemask & 0x5AA5) == 0 || (livemask & 0xA55A) == 0) {
            setup scopy;
            setup the_results[2];

            // This is an unsymmetrical thing.  Do each quadrant (a 1x2) separately by
            // using both maps, and then merge the result and hope for the best.

            ss->cmd.cmd_misc_flags &= ~CMD_MISC__MUST_SPLIT_MASK;
            scopy = *ss;    // "Move" can write over its input.
            divided_setup_move(ss, MAPCODE(s1x2,4,MPKIND__4_QUADRANTS,0),
                               phantest_ok, false, &the_results[0]);
            divided_setup_move(&scopy, MAPCODE(s1x2,4,MPKIND__4_QUADRANTS,1),
                               phantest_ok, false, &the_results[1]);
            *result = the_results[1];
            result->result_flags = get_multiple_parallel_resultflags(the_results, 2);
            merge_table::merge_setups(&the_results[0], merge_c1_phantom, result);
            return 1;
         }
      }

      goto divide_us_no_recompute;
   case sbigbone:

      switch (livemask) {
      case 01717:
         division_code = spcmap_bigbone_cw;
         break;
      case 07474:
         division_code = spcmap_bigbone_ccw;
         break;
      }

      goto divide_us_no_recompute;
   case sbigdmd:

      // The only way this can be legal is if people are in genuine "T" spots.

      switch (livemask) {
      case 01717:
         division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_OFFS_L_HALF,0,s_trngl4,0x7);
         break;
      case 07474:
         division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_OFFS_R_HALF,0,s_trngl4,0xD);
         break;
      }

      goto divide_us_no_recompute;
   case s2x2:
      ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;

      // Any 2x2 -> 2x2 call that acts by dividing itself into 1x2's
      // is presumed to want the people in each 1x2 to stay near each other.
      // We signify that by reverting to the original elongation,
      // overriding anything that may have been in the call definition.

      // Tentatively choose a map.  We may change it later to "spcmap_2x2v".
      division_code = MAPCODE(s1x2,2,MPKIND__SPLIT,1);

      if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_HORIZ) {
         division_code = spcmap_2x2v;
         goto divide_us_no_recompute;
      }
      else if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_VERT) {
         goto divide_us_no_recompute;
      }
      else {
         if ((newtb & 011) == 011) {
            // The situation is serious.  If the call has both 1x2 and 2x1 definitions,
            // we can do it, by guessing correctly which way to divide the setup.
            // Otherwise, if it has either a 1x2 or a 2x1 definition, but not both,
            // we lose, because the call presumably wants to use same.
            // But if the call has neither 1x2 nor 2x1 definitions, but does have
            // a 1x1 definition, we can do it.  Just divide the setup arbitrarily.

            // If the "lateral_to_selectees" flag is on (that is, the call is "run"),
            // We decide what to do according to the direction of the selectees.
            // There must be at least one, they must be collectively consistent.
            // Otherwise, we look at the manner in which the setup is T-boned
            // in order to figure out how to divide the setup.

            if (calldeflist->callarray_flags & CAF__LATERAL_TO_SELECTEES) {
               uint32 selmask = 0;

               for (i=0 ; i<4 ; i++) if (selectp(ss, i)) selmask |= ss->people[i].id1;

               if (selmask == 0 || (selmask & 011) == 011)
                  fail("People are not working with each other in a consistent way.");
               else if (selmask & 1)
               { division_code = spcmap_2x2v; }
            }
            else {
               if ((((ss->people[0].id1 | ss->people[3].id1) & 011) != 011) &&
                   (((ss->people[1].id1 | ss->people[2].id1) & 011) != 011))
               { division_code = spcmap_2x2v; }
               else if ((((ss->people[0].id1 | ss->people[1].id1) & 011) == 011) ||
                        (((ss->people[2].id1 | ss->people[3].id1) & 011) == 011))
                  fail("Can't figure out who should be working with whom.");
            }

            goto divide_us_no_recompute;
         }
         else {
            // People are not T-boned.  Check for a 2x1 or 1x2 definition.
            // If found, use it as a guide.  If both are present, we use
            // the setup elongation flag to tell us what to do.  In any
            // case, the setup elongation flag, if present, must not be
            // inconsistent with our decision.

            uint32 elong = 0;

            // If this is "run" and people aren't T-boned, just ignore the 2x1 definition.

            if (!(calldeflist->callarray_flags & CAF__LATERAL_TO_SELECTEES) &&
                assoc(b_2x1, ss, calldeflist))
               elong |= (2 - (newtb & 1));

            if (assoc(b_1x2, ss, calldeflist)) {
               // If "split_to_box" is on, prefer the 2x1 definition.
               if (!(calldeflist->callarray_flags & CAF__SPLIT_TO_BOX) || elong == 0)
                  elong |= (1 + (newtb & 1));
            }

            if (elong == 0) {
               // Neither 2x1 or 1x2 definition existed.  Check for 1x1.
               if (assoc(b_1x1, ss, calldeflist)) {
                  division_code = MAPCODE(s1x1,4,MPKIND__SPLIT_OTHERWAY_TOO,0);
                  goto divide_us_no_recompute;
               }
            }
            else {
               uint32 foo = (ss->cmd.prior_elongation_bits | ~elong) & 3;

               if (foo == 0) {
                  fail("Can't figure out who should be working with whom.");
               }
               else if (foo == 3) {
                  // We are in trouble if CMD_MISC__NO_CHK_ELONG is off.
                  // But, if there was a 1x1 definition, we allow it anyway.
                  // This is what makes "you all" and "the K" legal from lines.
                  // The "U-turn-back" is done in a 2x2 that is elongated laterally.
                  // "U-turn-back" has a 1x2 definition (so that you can roll) but
                  // no 2x1 definition (because, from a 2x2, it might overtake the 1x2
                  // definition, in view of the fact that there is no definite priority
                  // for searching for definitions, which could make people unable to
                  // roll during certain phases of the moon.)  So, when we are having the
                  // ends U-turn-back while in parallel lines, their 1x2's appear to be
                  // illegally separated.  Since they could have done it in 1x1's,
                  // we allow it.  And, incidentally, we allow a roll afterwards.

                  if (!assoc(b_1x1, ss, calldeflist)) {
                     if (!(ss->cmd.cmd_misc_flags & CMD_MISC__NO_CHK_ELONG))
                        fail("People are too far apart to work with each other on this call.");
                     else if (ss->cmd.prior_elongation_bits & PRIOR_ELONG_CONC_RULES_CHECK_HORRIBLE) {
                        // Only complain if anyone is line-like and facing in.

                        if ((((ss->cmd.prior_elongation_bits & 3) == 1) &&
                             ((ss->people[0].id1 & 0xF) == 0xA ||
                              (ss->people[1].id1 & 0xF) == 0xA ||
                              (ss->people[2].id1 & 0xF) == 0x8 ||
                              (ss->people[3].id1 & 0xF) == 0x8)) ||
                            (((ss->cmd.prior_elongation_bits & 3) == 2) &&
                             ((ss->people[0].id1 & 0xF) == 0x1 ||
                              (ss->people[1].id1 & 0xF) == 0x3 ||
                              (ss->people[2].id1 & 0xF) == 0x3 ||
                              (ss->people[3].id1 & 0xF) == 0x1)))
                           maybe_horrible_hinge = true;  // If result is 1x4, will raise the warning.
                     }
                  }

                  foo ^= elong;
               }

               if (foo == 1)
                  division_code = spcmap_2x2v;

               goto divide_us_no_recompute;
            }
         }
      }

      break;
   case s_rigger:
      if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
         fail("Can't split the setup.");

      if ((final_concepts.test_finalbits(FINAL__SPLIT_SQUARE_APPROVED |
                                         FINAL__SPLIT_DIXIE_APPROVED)) ||
          assoc(b_trngl4, ss, calldeflist) ||
          assoc(b_ptrngl4, ss, calldeflist)) {
         division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,1,s_trngl4,0xD);
         goto divide_us_no_recompute;
      }

      if (must_do_mystic)
         goto do_mystically;

      {
         uint32 tinytb =
            ss->people[2].id1 | ss->people[3].id1 |
            ss->people[6].id1 | ss->people[7].id1;

         // See if this call has applicable 1x2 or 2x1 definitions,
         // and the people in the wings are facing appropriately.
         // Then do it concentrically, which will break it into 4-person triangles
         // first and 1x2/2x1's later.  If it has a 1x1 definition,
         // do it no matter how people are facing.

         if ((!(tinytb & 010) || assoc(b_1x2, ss, calldeflist)) &&
             (!(tinytb & 1) || assoc(b_2x1, ss, calldeflist)))
            goto do_concentrically;

         if (assoc(b_1x1, ss, calldeflist))
            goto do_concentrically;
      }
      break;
   case s3x4:
      if (handle_3x4_division(ss, callflags1, newtb, livemask,
                              division_code,
                              calldeflist, matrix_aware, result))
         goto divide_us_no_recompute;
      return 1;
   case s4x4:
      if (handle_4x4_division(ss, callflags1, newtb, livemask,
                              division_code,
                              finalrot,
                              calldeflist, matrix_aware))
         goto divide_us_no_recompute;
      break;
   case s4x6:
      if (handle_4x6_division(ss, callflags1, newtb, livemask,
                              division_code,
                              calldeflist, matrix_aware))
         goto divide_us_no_recompute;
      break;
   case s3x8:
      if (handle_3x8_division(ss, callflags1, newtb, livemask,
                              division_code,
                              calldeflist, matrix_aware))
         goto divide_us_no_recompute;
      break;
   case s2x12:
      if (handle_2x12_division(ss, callflags1, newtb, livemask,
                              division_code,
                              calldeflist, matrix_aware))
         goto divide_us_no_recompute;
      break;
   case s_nftrgl6cw:
      // The only legal thing we can do here is split into two triangles.
      division_code = HETERO_MAPCODE(s_trngl,2,MPKIND__HET_OFFS_R_HALF,0,s_trngl,0x2);
      goto divide_us_no_recompute;
   case s_nftrgl6ccw:
      // The only legal thing we can do here is split into two triangles.
      division_code = HETERO_MAPCODE(s_trngl,2,MPKIND__HET_OFFS_L_HALF,0,s_trngl,0x8);
      goto divide_us_no_recompute;
   case s_wingedstar6:
      // **** Not really right.  Ought to check facing directions of the people that will be
      // involved in each 2-person subcall, with the appropriate definition for that subcall.
      if (assoc(b_1x2, ss, calldeflist) || assoc(b_2x1, ss, calldeflist)) {
         warn(warn__unusual);
         warn(warn_controversial);
         division_code = MAPCODE(s1x2,3,MPKIND__TRIPLETRADEINWINGEDSTAR6,0);
         goto divide_us_no_recompute;
      }

      break;
   case s_qtag:
      if (assoc(b_dmd, ss, calldeflist) ||
          assoc(b_pmd, ss, calldeflist)) {
         division_code = MAPCODE(sdmd,2,MPKIND__SPLIT,1);
         goto divide_us_no_recompute;
      }

      // Check whether it has 2x3/3x2 definitions, and divide the setup if so,
      // and if the call permits it.  This is important for permitting "Z axle" from
      // a 3x4 but forbidding "circulate" (unless we give a concept like 12 matrix
      // phantom columns.)  We also enable this if the caller explicitly said
      // "3x4 matrix".

      if (((callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) ||
           (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) ||
           nxnbits == INHERITFLAGMXNK_1X3 ||
           nxnbits == INHERITFLAGMXNK_3X1 ||
           nxnbits == INHERITFLAGNXNK_3X3) &&
          (!(newtb & 010) || assoc(b_3x2, ss, calldeflist)) &&
          (!(newtb & 001) || assoc(b_2x3, ss, calldeflist))) {
         division_code = spcmap_qtag_2x3;
         goto divide_us_no_recompute;
      }

      if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
         fail("Can't split the setup.");

      if (must_do_mystic)
         goto do_mystically;

      specialpass = false;
      have_1x2 = assoc(b_1x2, ss, calldeflist, &specialpass);
      have_2x1 = assoc(b_2x1, ss, calldeflist, &specialpass);
      if (ss->cmd.cmd_assume.assumption != cr_none) specialpass = true;
      if (have_1x2 && have_2x1) specialpass = true;

      if ((!(newtb & 010) || have_1x2) && (!(newtb & 1) || have_2x1)) {
         if (!specialpass && (newtb & 011) != 011) {
            division_code = MAPCODE(s1x2,4,MPKIND__SPLIT_WITH_45_ROTATION_OTHERWAY_TOO,1);
            goto divide_us_no_recompute;
         }
         else
            goto do_concentrically;
      }
      else if ((livemask & 0x55) == 0) {
         // Check for stuff like "heads pass the ocean; side corners only slide thru".
         division_code = spcmap_qtag_f1;
         goto divide_us_no_recompute;
      }
      else if ((livemask & 0x66) == 0) {
         division_code = spcmap_qtag_f2;
         goto divide_us_no_recompute;
      }
      else if ((livemask & 0x77) == 0) {
         // Check for stuff like "center two slide thru".
         division_code = spcmap_qtag_f0;
         goto divide_us_no_recompute;
      }
      else if (assoc(b_1x1, ss, calldeflist, &specialpass) && !specialpass) {
         division_code = MAPCODE(s1x1,8,MPKIND__QTAG8,0);
         goto divide_us_no_recompute;
      }

      break;
   case s_2stars:
      if (assoc(b_star, ss, calldeflist)) {
         division_code = MAPCODE(s_star,2,MPKIND__SPLIT,0);
         goto divide_us_no_recompute;
      }

      break;
   case s_bone:
      division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,1,s_trngl4,0x7);

      // If being forced to split in an impossible way, it's an error.
      if (((ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_HORIZ) && (ss->rotation & 1)) ||
          ((ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_VERT) && !(ss->rotation & 1)))
         fail("Can't split the setup.");

      // If being forced to split the right way, do so.  Also split it if it has a triangle definition.
      // Facing directions will be tested in the triangles.
      if ((ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK) ||
          assoc(b_trngl4, ss, calldeflist) ||
          assoc(b_ptrngl4, ss, calldeflist))
         goto divide_us_no_recompute;

      // See if this call is being done "split" as in "split square thru" or
      // "split dixie style", in which case split into triangles.
      // (Presumably there is a "twisted" somewhere.)
      if ((final_concepts.test_finalbits(FINAL__SPLIT_SQUARE_APPROVED | FINAL__SPLIT_DIXIE_APPROVED)))
         goto divide_us_no_recompute;

      if (must_do_mystic)
         goto do_mystically;

      {
         tbi = ss->people[2].id1 | ss->people[3].id1 |
            ss->people[6].id1 | ss->people[7].id1;
         tbo = ss->people[0].id1 | ss->people[1].id1 |
            ss->people[4].id1 | ss->people[5].id1;

         // See if this call has applicable 1x2 or 2x1 definitions,
         // and the people in the center 1x4 are facing appropriately.
         // Then do it concentrically, which will break it into 4-person
         // triangles first and 1x2/2x1's later.  If it has a 1x1 definition,
         // do it no matter how people are facing.
         if ((!((tbi & 010) | (tbo & 001)) || assoc(b_1x2, ss, calldeflist)) &&
             (!((tbi & 001) | (tbo & 010)) || assoc(b_2x1, ss, calldeflist)))
            goto do_concentrically;

         if (assoc(b_1x1, ss, calldeflist))
            goto do_concentrically;
      }

      // Turn a bone with only the center line occupied into a 1x8.
      if (livemask == 0xCC &&
          (!(newtb & 010) || assoc(b_1x8, ss, calldeflist)) &&
          (!(newtb & 1) || assoc(b_8x1, ss, calldeflist))) {
         ss->kind = s1x8;
         ss->swap_people(2, 7);
         ss->swap_people(3, 6);
         return 2;                        // And try again.
      }
      break;
   case s_ptpd:
      if (assoc(b_dmd, ss, calldeflist) ||
          assoc(b_pmd, ss, calldeflist) ||
          assoc(b_1x1, ss, calldeflist)) {
         division_code = MAPCODE(sdmd,2,MPKIND__SPLIT,0);
         goto divide_us_no_recompute;
      }
      if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
         fail("Can't split the setup.");
      break;
   case s3x3:
      // We can offset this to a 2x3 if we can see an unambiguous direction of the offset.

      finalrot = -1;

      if ((livemask & 0x70) == 0) {
         finalrot = 0;
      }

      if ((livemask & 0x1C) == 0) {
         if (finalrot >= 0) break;
         finalrot = 1;
      }

      if ((livemask & 0x7) == 0) {
         if (finalrot >= 0) break;
         finalrot = 2;
      }

      if ((livemask & 0xC1) == 0) {
         if (finalrot >= 0) break;
         finalrot = 3;
      }

      if (finalrot < 0) break;
      if ((newtb & 011) == 011) break;  // Can't allow T-boned people.

      // See if this call has applicable 2x3 or 3x2 definitions.
      if (assoc(((finalrot ^ newtb) & 1) ? b_3x2 : b_2x3, ss, calldeflist)) {
         division_code = MAPCODE(s2x3,1,MPKIND__OFFSET_UPWARD_1,0);
         goto divide_us_no_recompute;
      }

      break;
   case s2x3:
      // See if this call has applicable 1x2 or 2x1 definitions,
      // in which case split it 3 ways.
      if (((!(newtb & 010) || assoc(b_2x1, ss, calldeflist)) &&
           (!(newtb & 001) || assoc(b_1x2, ss, calldeflist))) ||
          assoc(b_1x1, ss, calldeflist)) {
         division_code = MAPCODE(s1x2,3,MPKIND__SPLIT,1);
         goto divide_us_no_recompute;
      }

      // See if this call has applicable 1x3 or 3x1 definitions,
      // in which case split it 2 ways.
      if ((!(newtb & 010) || assoc(b_1x3, ss, calldeflist)) &&
          (!(newtb & 001) || assoc(b_3x1, ss, calldeflist))) {
         division_code = MAPCODE(s1x3,2,MPKIND__SPLIT,1);
         goto divide_us_no_recompute;
      }

      // See if people only occupy Z-like spots.

      if (livemask == 033) {
         division_code = MAPCODE(s1x2,2,MPKIND__OFFS_L_HALF,1);
         goto divide_us_no_recompute;
      }
      else if (livemask == 066) {
         division_code = MAPCODE(s1x2,2,MPKIND__OFFS_R_HALF,1);
         goto divide_us_no_recompute;
      }
      else if (livemask == 036) {
         division_code = spcmap_2x3_1234;
         goto divide_us_no_recompute;
      }
      else if (livemask == 063) {
         division_code = spcmap_2x3_0145;
         goto divide_us_no_recompute;
      }

      break;
   case s_short6:
      if (assoc(b_trngl, ss, calldeflist) || assoc(b_ptrngl, ss, calldeflist) ||
          assoc(b_1x1, ss, calldeflist) || assoc(b_2x2, ss, calldeflist)) {
         division_code = HETERO_MAPCODE(s_trngl,2,MPKIND__HET_SPLIT,1,s_trngl,0x008);
         goto divide_us_no_recompute;
      }

      // Maybe we should fudge to a 2x3.
      if (callflags1 & CFLAG1_FUDGE_TO_Q_TAG) {
         setup sss = *ss;
         expand::compress_setup(s_short6_2x3, &sss);
         sss.cmd.cmd_misc_flags |= CMD_MISC__DISTORTED;
         move(&sss, true, result);
         ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;
         result->result_flags.clear_split_info();
         result->result_flags.misc |= RESULTFLAG__DID_SHORT6_2X3;
         return 1;
      }

      break;
   case s_ntrgl6cw:
   case s_ntrgl6ccw:
      if (assoc(b_trngl, ss, calldeflist) || assoc(b_ptrngl, ss, calldeflist) ||
          assoc(b_1x1, ss, calldeflist) || assoc(b_2x2, ss, calldeflist)) {
         division_code = (ss->kind == s_ntrgl6cw) ?
            HETERO_MAPCODE(s_trngl,2,MPKIND__HET_SPLIT,0,s_trngl,0x2) :
            HETERO_MAPCODE(s_trngl,2,MPKIND__HET_SPLIT,0,s_trngl,0x8);
         goto divide_us_no_recompute;
      }
      break;
   case s_bone6:
      if (assoc(b_trngl, ss, calldeflist) || assoc(b_ptrngl, ss, calldeflist) ||
          assoc(b_1x1, ss, calldeflist) || assoc(b_2x2, ss, calldeflist)) {
         division_code = HETERO_MAPCODE(s_trngl,2,MPKIND__HET_SPLIT,1,s_trngl,0x7);
         goto divide_us_no_recompute;
      }

      {
         tbi = ss->people[2].id1 | ss->people[5].id1;
         tbo = ss->people[0].id1 | ss->people[1].id1 |
            ss->people[3].id1 | ss->people[4].id1;

         if (assoc(b_1x1, ss, calldeflist) ||
             ((!((tbi & 010) | (tbo & 001)) || assoc(b_1x2, ss, calldeflist)) &&
              (!((tbi & 001) | (tbo & 010)) || assoc(b_2x1, ss, calldeflist)))) {
            conc_schema = schema_concentric_6p;
            goto do_concentrically;
         }
      }

      break;
   case s_1x2dmd:
      {
         tbi = ss->people[2].id1 | ss->people[5].id1;
         tbo = ss->people[0].id1 | ss->people[1].id1 |
            ss->people[3].id1 | ss->people[4].id1;

         if (assoc(b_1x1, ss, calldeflist) ||
             ((!((tbi & 010) | (tbo & 001)) || assoc(b_2x1, ss, calldeflist)) &&
              (!((tbi & 001) | (tbo & 010)) || assoc(b_1x2, ss, calldeflist)))) {
            conc_schema = schema_concentric_6p;
            goto do_concentrically;
         }
      }
      break;
   case sdeepxwv:
      if (assoc(b_1x1, ss, calldeflist) ||
          assoc(b_2x1, ss, calldeflist) ||
          assoc(b_1x2, ss, calldeflist)) {
         conc_schema = schema_concentric_8_4;
         goto do_concentrically;
      }
      break;
   case s_trngl:
      if (assoc(b_2x2, ss, calldeflist)) {
         if (calling_level < triangle_in_box_level)
            warn_about_concept_level();

         uint32 leading = final_concepts.final;

         if (ss->cmd.cmd_misc3_flags & CMD_MISC3__SAID_TRIANGLE) {
            if (final_concepts.test_finalbit(FINAL__TRIANGLE))
               fail("'Triangle' concept is redundant.");
         }
         else {
            if (!(final_concepts.test_finalbits(FINAL__TRIANGLE|FINAL__LEADTRIANGLE)))
               fail("You must give the 'triangle' concept.");
         }

         if ((ss->people[0].id1 & d_mask) == d_east)
            leading = ~leading;
         else if ((ss->people[0].id1 & d_mask) != d_west)
            fail("Can't figure out which way triangle point is facing.");

         division_code = (leading & FINAL__LEADTRIANGLE) ? spcmap_trngl_box1 : spcmap_trngl_box2;

         final_concepts.clear_finalbits(FINAL__TRIANGLE|FINAL__LEADTRIANGLE);
         ss->cmd.cmd_final_flags = final_concepts;
         divided_setup_move(ss, division_code, phantest_ok, false, result);
         result->result_flags.misc |= RESULTFLAG__DID_TGL_EXPANSION;
         result->result_flags.clear_split_info();
         return 1;
      }
      break;
   case s1x6:
      // See if this call has a 1x2, 2x1, or 1x1 definition, in which case split it 3 ways.
      if (assoc(b_1x2, ss, calldeflist) ||
          assoc(b_2x1, ss, calldeflist) ||
          assoc(b_1x1, ss, calldeflist)) {
         division_code = MAPCODE(s1x2,3,MPKIND__SPLIT,0);
         goto divide_us_no_recompute;
      }
      // If it has 1x3 or 3x1 definitions, split it 2 ways.
      if (assoc(b_1x3, ss, calldeflist) || assoc(b_3x1, ss, calldeflist)) {
         division_code = MAPCODE(s1x3,2,MPKIND__SPLIT,0);
         // We want to be sure that the operator knows what we are doing, and why,
         // if we divide a 1x6 into 1x3's.  We allow "swing thru" in a wave of
         // 3 or 4 people.  If the operator wants to do a swing thru with
         // all 6 people, use "grand swing thru".
         warn(warn__split_to_1x3s);
         goto divide_us_no_recompute;
      }
      break;
   case s1x2:
      // See if the call has a 1x1 definition, in which case split it and do each part.
      if (assoc(b_1x1, ss, calldeflist)) {
         division_code = MAPCODE(s1x1,2,MPKIND__SPLIT,0);
         goto divide_us_no_recompute;
      }
      break;
   case s1x3:
      /* See if the call has a 1x1 definition, in which case split it and do each part. */
      if (assoc(b_1x1, ss, calldeflist)) {
         division_code = MAPCODE(s1x1,3,MPKIND__SPLIT,0);
         goto divide_us_no_recompute;
      }
      break;
   case sdmd:
      /* See if the call has a 1x1 definition, in which case split it and do each part. */
      if (assoc(b_1x1, ss, calldeflist)) {
         division_code = spcmap_dmd_1x1;
         goto divide_us_no_recompute;
      }
      break;
   case s_star:
      /* See if the call has a 1x1 definition, in which case split it and do each part. */
      if (assoc(b_1x1, ss, calldeflist)) {
         division_code = spcmap_star_1x1;
         goto divide_us_no_recompute;
      }
      break;
   case s1x8:
      if (must_do_mystic)
         goto do_mystically;

      // See if the call has a 1x4, 4x1, 1x2, 2x1, or 1x1 definition, in which case split it and do each part.
      division_code = MAPCODE(s1x4,2,MPKIND__SPLIT,0);
      if (     assoc(b_1x4, ss, calldeflist) || assoc(b_4x1, ss, calldeflist)) {
         goto divide_us_no_recompute;
      }

      division_code = MAPCODE(s1x2,4,MPKIND__SPLIT,0);
      if (     assoc(b_1x2, ss, calldeflist) || assoc(b_2x1, ss, calldeflist) ||
               assoc(b_1x1, ss, calldeflist)) {
         goto divide_us_no_recompute;
      }

      if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
         fail("Can't split the setup.");

      /* See if this call has applicable 1x12 or 1x16 definitions and matrix expansion is permitted.
         If so, that is what we must do. */

      if (  !(ss->cmd.cmd_misc_flags & CMD_MISC__NO_EXPAND_MATRIX) &&
            (!(newtb & 010) || assoc(b_1x12, ss, calldeflist) || assoc(b_1x16, ss, calldeflist)) &&
            (!(newtb & 1) || assoc(b_12x1, ss, calldeflist) || assoc(b_16x1, ss, calldeflist))) {
         do_matrix_expansion(ss, CONCPROP__NEEDK_1X12, true);
         if (ss->kind != s1x12) fail("Failed to expand to 1X12.");  /* Should never fail, but we don't want a loop. */
         return 2;                        /* And try again. */
      }

      // We might be doing some kind of "own the so-and-so" operation in which people who are ends of
      // each wave in a 1x8 want to think they are points of diamonds instead.  This could happen,
      // for example, with point-to-point diamonds if we say "own the <points>, flip the diamond by
      // flip the diamond".  Yes, it's stupid.  Now normalize_setup turned the centerless diamonds
      // into a 1x8 (it needs to do that in order for "own the <points>, trade by flip the diamond"
      // to work.  We must turn that 1x8 back into diamonds.  The "own the so-and-so" concept turns
      // on CMD_MISC__PHANTOMS.  If this flag weren't on, we would have no business saying "I see
      // phantoms in the center 2 spots of my wave, I'm allowed to think of this as a diamond."
      // The same thing is done below for 2x4's and 1x4's.
      // We only do this if some kind of "do your part" operation is going on:  own the <anyone>,
      // mystic, etc.

      if ((ss->cmd.cmd_misc_flags & CMD_MISC__PHANTOMS) &&
          (ss->cmd.cmd_misc3_flags & CMD_MISC3__DOING_YOUR_PART)) {
         if ((ss->people[1].id1 | ss->people[3].id1 |
              ss->people[5].id1 | ss->people[7].id1) == 0) {
            setup sstest = *ss;
            sstest.kind = s_ptpd;

            if ((!(newtb & 010) ||
                 assoc(b_ptpd, &sstest, calldeflist) ||
                 assoc(b_dmd, &sstest, calldeflist)) &&
                (!(newtb & 001) ||
                 assoc(b_pptpd, &sstest, calldeflist) ||
                 assoc(b_pmd, &sstest, calldeflist))) {
               *ss = sstest;
               return 2;
            }
         }
         else if ((ss->people[0].id1 | ss->people[1].id1 |
                   ss->people[4].id1 | ss->people[5].id1) == 0) {
            setup sstest = *ss;
            sstest.swap_people(2, 7);
            sstest.swap_people(3, 6);
            sstest.kind = s_qtag;

            if ((!(newtb & 010) ||
                 assoc(b_qtag, &sstest, calldeflist) ||
                 assoc(b_pmd, &sstest, calldeflist)) &&
                (!(newtb & 001) ||
                 assoc(b_pqtag, &sstest, calldeflist) ||
                 assoc(b_dmd, &sstest, calldeflist))) {
               *ss = sstest;
               return 2;
            }
         }
      }

      break;
   case s_crosswave:
      if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
         fail("Can't split the setup.");

      // If we do not have a 1x4 or 4x1 definition, but we have 1x2, 2x1, or 1x1 definitions,
      // do the call concentrically.  This will have the effect of having each miniwave do it.
      // If we did this when a 1x4 or 4x1 definition existed, it would have the effect of
      // having the people in the outer, disconnected, 1x4 work with each other across
      // the set, which we do not want.

      if (must_do_mystic)
         goto do_mystically;

      if (!assoc(b_4x1, ss, calldeflist) &&
          !assoc(b_1x4, ss, calldeflist) &&
          (assoc(b_2x1, ss, calldeflist) ||
           assoc(b_1x2, ss, calldeflist) ||
           assoc(b_1x1, ss, calldeflist)))
         goto do_concentrically;

      break;
   case s2x4:
      division_code = MAPCODE(s2x2,2,MPKIND__SPLIT,0);    // The map we will probably use.

      // See if this call is being done "split" as in "split square thru" or
      // "split dixie style", in which case split into boxes.

      if (final_concepts.test_finalbits(FINAL__SPLIT_SQUARE_APPROVED |
                                        FINAL__SPLIT_DIXIE_APPROVED))
         goto divide_us_no_recompute;

      // If this is "run", always split it into boxes.  If they are T-boned,
      // they will figure it out, we hope.

      if (calldeflist->callarray_flags & CAF__LATERAL_TO_SELECTEES)
         goto divide_us_no_recompute;

      // See if this call has applicable 2x6 or 2x8 definitions and matrix expansion
      // is permitted.  If so, that is what we must do.  But if it has a 4x4 definition
      // also, it is ambiguous, so we can't do it.

      if (!(ss->cmd.cmd_misc_flags & CMD_MISC__NO_EXPAND_MATRIX) &&
          !assoc(b_4x4, ss, calldeflist) &&
          (!(newtb & 010) ||
           assoc(b_2x6, ss, calldeflist) ||
           assoc(b_2x8, ss, calldeflist)) &&
          (!(newtb & 1) ||
           assoc(b_6x2, ss, calldeflist) ||
           assoc(b_8x2, ss, calldeflist))) {

         if (must_do_mystic)
            fail("Can't do \"snag/mystic\" with this call.");

         do_matrix_expansion(ss, CONCPROP__NEEDK_2X6, true);

         // Should never fail, but we don't want a loop.
         if (ss->kind != s2x6) fail("Failed to expand to 2X6.");

         return 2;                        // And try again.
      }

      // If we are splitting for "central", "crazy", or "splitseq",
      // give preference to 2x2 splitting.  Also give preference
      // if the "split_to_box" flag was given.

      if (((ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_HORIZ) && !(ss->rotation & 1)) ||
          ((ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_VERT) && (ss->rotation & 1)) ||
          (calldeflist->callarray_flags & CAF__SPLIT_TO_BOX)) {
         if (assoc(b_2x2, ss, calldeflist) ||
             ((newtb & 001) == 0 && assoc(b_2x1, ss, calldeflist)) ||
             ((newtb & 010) == 0 && assoc(b_1x2, ss, calldeflist)))
            goto divide_us_no_recompute;
      }

      // See if this call has applicable 1x4 or 4x1 definitions,
      // in which case split it that way.

      if ((!(newtb & 010) || assoc(b_1x4, ss, calldeflist)) &&
          (!(newtb & 1) || assoc(b_4x1, ss, calldeflist))) {
         division_code = MAPCODE(s1x4,2,MPKIND__SPLIT,1);
         goto divide_us_no_recompute;
      }

      if (must_do_mystic)
         goto do_mystically;

      // See if this call has applicable 2x2 definition, in which case split into boxes.

      if (assoc(b_2x2, ss, calldeflist)) goto divide_us_no_recompute;

      // See long comment above for s1x8.  The test cases for this are
      // "own the <points>, trade by flip the diamond", and
      // "own the <points>, flip the diamond by flip the diamond".
      //
      // But it's more complicated than that.  While we must do this for flip/cut
      // the diamond, we must *not* do it for "N/4 by the right".  Complicated cases
      // of phantom waves swing the fractions depend on this.  The test is rh06.
      // The call N/4 by the right has lots of complicated behavior in diamond/qtag spots,
      // and we must not trigger that behavior.  So we test for the call "N/4 by the right"
      // by an incredibly sleazy and shameful method.  We check whether the call has
      // a definition in a short6, which N/4 by the right does.  We pass a null second
      // argument so it won't do any special qualifier tests; we just want to know
      // whether short6 is on the association list.

      if ((ss->cmd.cmd_misc_flags & CMD_MISC__PHANTOMS) &&
          (ss->people[1].id1 | ss->people[2].id1 |
           ss->people[5].id1 | ss->people[6].id1) == 0 && !assoc(b_short6, (setup *) 0, calldeflist)) {
         setup sstest = *ss;

         expand::expand_setup(s_qtg_2x4, &sstest);

         uint32 tbtest =
            sstest.people[0].id1 | sstest.people[1].id1 |
            sstest.people[4].id1 | sstest.people[5].id1;

         if ((!(tbtest & 010) ||
              assoc(b_qtag, &sstest, calldeflist) ||
              assoc(b_pmd, &sstest, calldeflist)) &&
             (!(tbtest & 001) ||
              assoc(b_pqtag, &sstest, calldeflist) ||
              assoc(b_dmd, &sstest, calldeflist))) {
            *ss = sstest;
            *newtb_p = tbtest;
            return 2;                        // And try again.
         }
      }

      // Look very carefully at how we split this, so we get the
      // RESULTFLAG__SPLIT_AXIS_MASK stuff right.

      specialpass = false;
      have_1x2 = assoc(b_1x2, ss, calldeflist, &specialpass);
      have_2x1 = assoc(b_2x1, ss, calldeflist, &specialpass);
      if (ss->cmd.cmd_assume.assumption != cr_none) specialpass = true;
      if (have_1x2 && have_2x1) specialpass = true;

      // This call does not have an applicable 2x2 definition.  Check for appropriate
      // 1x2 or 2x1 definitions that call for a twofold split in each direction.  If so,
      // do a direct fourfold split.  Do this only if the setup is not T-boned, so that
      // it is clear that this is the right thing.  Also, don't do direct fourfold
      // splits if "specialpass" was set, or both definitions existed, or if any
      // assumption was used.  See t02t, t36t and t40t.

      if ((!(newtb & 1) && have_1x2) || (!(newtb & 010) && have_2x1)) {
         if (!specialpass) division_code = MAPCODE(s1x2,4,MPKIND__SPLIT_OTHERWAY_TOO,1);
         goto divide_us_no_recompute;
      }

      // The other way.  Do a direct fourfold split along one axis.

      if ((!(newtb & 1) && have_2x1) || (!(newtb & 010) && have_1x2)) {
         if (!specialpass) division_code = MAPCODE(s1x2,4,MPKIND__SPLIT,1);
         goto divide_us_no_recompute;
      }

      // If we are T-boned and have 1x2 or 2x1 definitions, we need to be careful.

      if ((newtb & 011) == 011) {
         tbi = ss->people[1].id1 | ss->people[2].id1 | ss->people[5].id1 | ss->people[6].id1;
         tbo = ss->people[0].id1 | ss->people[3].id1 | ss->people[4].id1 | ss->people[7].id1;

         if (((tbi & 011) != 011) && ((tbo & 011) != 011)) {

            // The centers and ends are T-boned to each other but each is individually
            // consistent.  We can do the call concentrically *IF* the appropriate type of
            // definition exists for the ends to work with the near person rather than the
            // far one.  This is what makes "heads into the middle and everbody partner
            // trade" work, and forbids "heads into the middle and everbody star thru".
            //
            // Or if we have a 1x1 definition, we can divide it.  Otherwise, we lose.

            static const expand::thing s_1x2_2x4_leftends = {{7, 0}, s1x2, s2x4, 1};
            static const expand::thing s_1x2_2x4_rightends = {{4, 3}, s1x2, s2x4, 1};

            setup leftends = *ss;
            setup rightends = *ss;
            expand::compress_setup(s_1x2_2x4_leftends, &leftends);
            expand::compress_setup(s_1x2_2x4_rightends, &rightends);

            if (((tbo & 1) != 0 &&      // Handle ends in line-like 1x2's,
                 assoc(b_1x2, &leftends, calldeflist) != 0 &&
                 assoc(b_1x2, &rightends, calldeflist) != 0) ||
                ((tbo & 1) == 0 &&     // or in column-like 1x2's.
                 assoc(b_2x1, &leftends, calldeflist) != 0 &&
                 assoc(b_2x1, &rightends, calldeflist) != 0)) {
               if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
                  fail("Can't split the setup.");

               goto do_concentrically;
            }

            if (assoc(b_1x1, ss, calldeflist))
               goto divide_us_no_recompute;
         }

         // If the centers and ends are not separately consistent, we should just
         // split it into 2x2's.  Perhaps the call has both 1x2 and 2x1 definitions,
         // and will be done sort of siamese in each quadrant.  Another possibility
         // is that the call has just (say) 1x2 definitions, but everyone can do their
         // part and miraculously not hit each other.

         else goto divide_us_no_recompute;
      }

      // If some phantom concept has been used and there are 1x2 or 2x1
      // definitions, we also split it into boxes even if people are T-boned.
      // This is what makes everyone do their part if we say "heads into the middle
      // and heads are standard in split phantom lines, partner trade".
      // But we don't turn on both splitting bits in this case.  Note that,
      // since the previous test failed, the setup must be T-boned if this test passes.

      if ((ss->cmd.cmd_misc_flags & CMD_MISC__PHANTOMS) && (have_1x2 != 0 || have_2x1 != 0))
         goto divide_us_no_recompute;

      // Maybe we should divide concentrically not because the centers and ends
      // are T-boned, but because they have different properties that are needed
      // to satisfy qualifiers.  First, check that we failed because of unsatisfied
      // qualifiers.  This is indicated by failure to get 1x2 or 2x1 definitions
      // while looking at the setup, but success when we don't look at the setup.
      // (Recall that qualifiers automatically pass if a null setup pointer is given.)
      // The test for this is "turn thru" in a 2x4 when some are facing and some
      // are in miniwaves.  See vg06t.

      if (have_1x2 == 0 && have_2x1 == 0) {
         // It needs to have both definitions, both failing because of
         // qualifiers, for a concentric division to be the right thing.
         if (assoc(b_1x2, (setup *) 0, calldeflist) &&
             assoc(b_2x1, (setup *) 0, calldeflist) &&
             (calldeflist->callarray_flags & CAF__SPLIT_TO_BOX) &&
             !(ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)) {
               goto do_concentrically;
         }
      }

      // We are not T-boned, and there is no 1x2 or 2x1 definition.
      // The only possibility is that there is a 1x1 definition.

      if (assoc(b_1x1, ss, calldeflist)) {
         // If the planets are auspicious, do a direct 8-way division.
         // Otherwise, divide to two 2x2's, and the right things will happen.
         if (!specialpass) division_code = MAPCODE(s1x1,8,MPKIND__SPLIT_OTHERWAY_TOO,0);
         goto divide_us_no_recompute;
      }

      break;
   case s2x5:
      division_code = MAPCODE(s1x2,5,MPKIND__SPLIT,1);

      if (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) {
         if (((newtb & 001) == 0 && assoc(b_2x1, ss, calldeflist)) ||
             ((newtb & 010) == 0 && assoc(b_1x2, ss, calldeflist)))
            goto divide_us_no_recompute;
      }
      else if (assoc(b_1x1, ss, calldeflist))
         goto divide_us_no_recompute;

      switch (livemask) {
      case 0x3DE:
         division_code = MAPCODE(s1x4,2,MPKIND__OFFS_R_ONEQ,1);
         warn(warn__each1x4);
         break;
      case 0x1EF:
         division_code = MAPCODE(s1x4,2,MPKIND__OFFS_L_ONEQ,1);
         warn(warn__each1x4);
         break;
      case 0x37B:    // the outer boxes
         division_code = MAPCODE(s2x2,2,MPKIND__UNDERLAP,0);
         warn(warn__each2x2);
         break;
      }

      goto divide_us_no_recompute;

   case s1x4:
      /* See if the call has a 1x2, 2x1, or 1x1 definition,
         in which case split it and do each part. */
      if ((assoc(b_1x2, ss, calldeflist) ||
           assoc(b_2x1, ss, calldeflist) ||
           assoc(b_1x1, ss, calldeflist))) {

         /* The following makes "ends hinge" work from a grand wave.  Any 1x4 -> 2x2 call
            that acts by dividing itself into 1x2's is presumed to want the people in each 1x2
            to stay near each other.  We signify that by flipping the elongation, which we
            had previously set perpendicular to the 1x4 axis, overriding anything that may
            have been in the call definition. */

         *desired_elongation_p ^= 3;
         /* If the flags were zero and we complemented them so that
            both are set, that's not good. */
         if (*desired_elongation_p == 3)
            *desired_elongation_p = 0;

         division_code = MAPCODE(s1x2,2,MPKIND__SPLIT,0);
         goto divide_us_no_recompute;
      }

      /* See long comment above for s1x8.  The test cases for this are "tandem own the <points>, trade
         by flip the diamond", and "tandem own the <points>, flip the diamond by flip the diamond",
         both from a tandem diamond (the point being that there will be only one of them.) */

      if ((ss->cmd.cmd_misc_flags & CMD_MISC__PHANTOMS) &&
          (ss->people[1].id1 | ss->people[3].id1) == 0) {
         setup sstest = *ss;

         sstest.kind = sdmd;   /* It makes assoc happier if we do this now. */

         if (
             (!(newtb & 010) || assoc(b_dmd, &sstest, calldeflist)) &&
             (!(newtb & 001) || assoc(b_pmd, &sstest, calldeflist))) {
            *ss = sstest;
            return 2;                        /* And try again. */
         }
      }

      break;
   case s_trngl4:
      // See if the call has a 1x2, 2x1, or 1x1 definition, in which case
      // split it and do each part.
      if ((assoc(b_1x2, ss, calldeflist) ||
           assoc(b_2x1, ss, calldeflist) ||
           assoc(b_1x1, ss, calldeflist))) {
         division_code = HETERO_MAPCODE(s1x2,2,MPKIND__HET_SPLIT,0,s1x2,0x1);
         goto divide_us_no_recompute;
      }

      break;
   case sdmdpdmd:
      // This is the only way we can divide it for these things.
      division_code = HETERO_MAPCODE(sdmd,2,MPKIND__HET_SPLIT,0,sdmd,0x1);
      goto divide_us_no_recompute;
   case s_trngl8:
      division_code = HETERO_MAPCODE(s1x4,2,MPKIND__HET_SPLIT,0,s1x4,0x1);
      goto divide_us_no_recompute;
   case slinebox:
      division_code = HETERO_MAPCODE(s1x4,2,MPKIND__HET_SPLIT,0,s2x2,0x0);
      goto divide_us_no_recompute;
   case slinedmd:
      division_code = HETERO_MAPCODE(s1x4,2,MPKIND__HET_SPLIT,1,sdmd,0x5);
      goto divide_us_no_recompute;
   case sboxdmd:
      division_code = HETERO_MAPCODE(sdmd,2,MPKIND__HET_SPLIT,1,s2x2,0x0);
      goto divide_us_no_recompute;
   case sdbltrngl4:
      division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,1,s_trngl4,0x5);
      goto divide_us_no_recompute;
   case slinejbox:
      division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,1,s2x2,0x1);
      goto divide_us_no_recompute;
   case slinevbox:
      division_code = HETERO_MAPCODE(s1x4,2,MPKIND__HET_SPLIT,0,s_trngl4,0x4);
      goto divide_us_no_recompute;
   case slineybox:
      division_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,1,s2x2,0x3);
      goto divide_us_no_recompute;
   case slinefbox:
      division_code = HETERO_MAPCODE(s1x4,2,MPKIND__HET_SPLIT,0,s_trngl4,0xC);
      goto divide_us_no_recompute;
   }

   if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
      fail("Can't split the setup.");

   result->result_flags.clear_split_info();
   return 0;    // We did nothing.  An error will presumably result.

   divide_us_no_recompute:

   if (division_code == ~0U)
      fail("You must specify a concept.");

   if (must_do_mystic)
      fail("Can't do \"snag/mystic\" with this call.");

   ss->cmd.prior_elongation_bits = 0;
   ss->cmd.prior_expire_bits = 0;
   ss->rotation += finalrot;   // Flip the setup around and recanonicalize.
   canonicalize_rotation(ss);
   divided_setup_move(ss, division_code, phantest_ok,
                      (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK) != 0, result);

   // Flip the setup back if necessary.  It will get canonicalized.
   result->rotation -= finalrot;
   // And note that the splitting has changed.
   if (finalrot & 1)
      result->result_flags.swap_split_info_fields();

   // If a "horrible hinge" went to a 1x4, it's horrible.
   if (maybe_horrible_hinge && result->kind == s1x4)
      warn(warn__horrible_conc_hinge);

   // If expansion to a 2x3 occurred (because the call was, for example, a "pair the line"),
   // and the two 2x3's are end-to-end in a 2x6, see if we can squash phantoms.  We squash both
   // internal (the center triple box) and external ones.  The external ones would probably have
   // been squashed anyway due to the top level normalization, but we want this to occur
   // immediately, not just at the top level, though we can't think of a concrete example
   // in which it makes a difference.

   if (result->result_flags.misc & RESULTFLAG__EXPAND_TO_2X3) {
      static const expand::thing inner_2x6 = {{0, 1, 4, 5, 6, 7, 10, 11}, s2x4, s2x6, 0};
      static const expand::thing inner_dblbone6 = {{1, 9, 11, 8, 7, 3, 5, 2}, s_rigger, sdblbone6, 0};
      static const expand::thing outer_dblbone6 = {{0, 10, 11, 8, 6, 4, 5, 2}, s_bone, sdblbone6, 0};
      static const expand::thing inner8_2x10 = {{0, 1, 2, 7, 8, 9, 10, 11, 12, 17, 18, 19}, s2x6, s2x10, 0};
      static const expand::thing inner_2x10 = {
         {0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19}, s2x8, s2x10, 0};
      static const expand::thing outer8_2x10 = {{2, 3, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17}, s2x6, s2x10, 0};
      static const expand::thing outer_2x10 = {
         {1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18}, s2x8, s2x10, 0};
      static const expand::thing inner_rig = {{6, 7, -1, 2, 3, -1}, s1x6, s_rigger, 0};
      static const expand::thing inner_4x6 = {{4, 7, 22, 8, 13, 14, 15, 21, 16, 19, 10, 20, 1, 2, 3, 9},
                                              s4x4, s4x6, 0};
      static const expand::thing outer_4x6 = {{5, 6, 23, 7, 12, 13, 16, 22, 17, 18, 11, 19, 0, 1, 4, 10},
                                              s4x4, s4x6, 0};
      static const expand::thing inner_3x6 = {{0, 1, 4, 5, 6, 7, 9, 10, 13, 14, 15, 16}, s3x4, s3x6, 0};
      static const expand::thing outer_3x6 = {{1, 2, 3, 4, 7, 8, 10, 11, 12, 13, 16, 17}, s3x4, s3x6, 0};

      const expand::thing *expand_ptr = (const expand::thing *) 0;

      switch (result->kind) {
      case s2x6:
         if (!(result->people[2].id1 | result->people[3].id1 |
               result->people[8].id1 | result->people[9].id1)) {
            // Inner spots are empty.

            // If the outer ones are empty also, we don't know what to do.
            // This is presumably a "snag pair the line", or something like that.
            // Clear the inner spots away, and turn off the flag, so that
            // "punt_centers_use_concept" will know what to do.
            if (!(result->people[0].id1 | result->people[5].id1 |
                  result->people[6].id1 | result->people[11].id1))
               result->result_flags.misc &= ~RESULTFLAG__EXPAND_TO_2X3;

            expand_ptr = &inner_2x6;
         }
         break;
      case sdblbone6:
         if (!(result->people[1].id1 | result->people[3].id1 |
               result->people[7].id1 | result->people[9].id1)) {
            // Inner spots are empty.

            // If the outer ones are empty also, trim the outer ones,
            // resulting in a rigger, and leave the flag on.
            if (!(result->people[0].id1 | result->people[4].id1 |
                  result->people[6].id1 | result->people[10].id1))
               expand_ptr = &inner_dblbone6;
            else
               expand_ptr = &outer_dblbone6;    // Remove the inner ones, and leave the flag on.
         }
         break;
      case s2x10:
         if (!(result->people[4].id1 | result->people[5].id1 |
               result->people[14].id1 | result->people[15].id1)) {
            // Innermost 4 spots are empty.
            if (!(result->people[3].id1 | result->people[6].id1 | result->people[13].id1 | result->people[16].id1))
               expand_ptr = &inner8_2x10;   // Actually, inner 8 spots are empty.
            else
               expand_ptr = &inner_2x10;
         }
         else if (!(result->people[0].id1 | result->people[9].id1 |
                    result->people[10].id1 | result->people[19].id1)) {
            // Outermost 4 spots are empty.
            if (!(result->people[1].id1 | result->people[8].id1 | result->people[11].id1 | result->people[18].id1))
               expand_ptr = &outer8_2x10;   // Actually, outer 8 spots are empty.
            else
               expand_ptr = &outer_2x10;
         }
         break;
      case s_rigger:
         // The outer spots are already known to be empty and have been cleaned up.
         // So we just have to deal with the inner spots.  This means that both
         // inner and outer spots are empty, so we have to do the same thing
         // that we do above in the 2x6.
         if (!(result->people[0].id1 | result->people[1].id1 |
               result->people[4].id1 | result->people[5].id1)) {
            result->result_flags.misc &= ~RESULTFLAG__EXPAND_TO_2X3;
            expand_ptr = &inner_rig;
         }
         break;
      case s4x6:
         // We do the same for two concatenated 3x4's.
         // This could happen if the people folding were not the ends.
         if (!(result->people[2].id1 | result->people[3].id1 |
               result->people[8].id1 | result->people[9].id1 |
               result->people[20].id1 | result->people[21].id1 |
               result->people[14].id1 | result->people[15].id1)) {
            // Inner spots are empty.
            expand_ptr = &outer_4x6;
         }
         else if (!( result->people[0].id1 | result->people[5].id1 |
                     result->people[6].id1 | result->people[11].id1 |
                     result->people[18].id1 | result->people[23].id1 |
                     result->people[12].id1 | result->people[17].id1)) {
            // Outer spots are empty.
            expand_ptr = &inner_4x6;
         }
         break;
      case s3x6:
         if (result->result_flags.split_info[result->rotation & 1] == 1 &&
             result->result_flags.split_info[(result->rotation ^ 1) & 1] == 0) {
            // These were offset 2x3's.
            if (!(result->people[2].id1 | result->people[3].id1 | result->people[8].id1 |
                  result->people[11].id1 | result->people[12].id1 | result->people[17].id1)) {
               // Inner spots are empty.
               expand_ptr = &inner_3x6;
            }
            else if (!(result->people[0].id1 | result->people[5].id1 | result->people[6].id1 |
                       result->people[9].id1 | result->people[14].id1 | result->people[15].id1)) {
               // Outer spots are empty.
               expand_ptr = &outer_3x6;
            }
         }
         break;
      }

      if (expand_ptr) expand::compress_setup(*expand_ptr, result);
   }

   return 1;

   do_concentrically:

   conc_cmd = ss->cmd;
   concentric_move(ss, &conc_cmd, &conc_cmd, conc_schema,
                   0, DFM1_SUPPRESS_ELONGATION_WARNINGS, false, true, ~0U, result);
   return 1;

   do_mystically:

   conc_cmd = ss->cmd;

   // It really isn't clear whether this is the right thing.  Mystic appears
   // to mean "centers do the full call, in the full setup, mirror, while the
   // ends do it normally".  There is definitely a notion of doing your part,
   // in the whole setup, of the 8-person call.  But it appears that the call
   // "trade" means to find someone with whom to trade, without getting too
   // caught up in the "do your part in the whole setup" notion.  In particular,
   // if we are in a (1/4 tag; trade and roll) setup, and we call "1/2 mystic
   // spin the top", we want everyone to step to right or left hands mystically,
   // and then trade mystically.  The centers do the trade in the center.  They
   // don't "do their part".  So we check for the call "trade".  Perhaps there
   // are other calls in this catagory, and it's quite complicated.  Perhaps
   // the "selective_key_dyp_for_mystic" should only be used if the setup is
   // a rigger or otherwise something for which the centers and ends trade
   // by themselves.  Trade is a complicated call.

   ss->cmd.cmd_misc3_flags |= CMD_MISC3__DOING_YOUR_PART;

   inner_selective_move(ss, &conc_cmd, &conc_cmd,
                        ss->cmd.callspec == base_calls[base_call_trade] ?
                        selective_key_dyp_for_mystic : selective_key_dyp,
                        1, 0, false, 0, selector_centers, 0, 0, result);
   return 1;
}




static veryshort s1x6translateh[32] = {
   -1, 0, 1, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

static veryshort s1x6translatev[32] = {
   -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 4, 5, -1, -1, -1, -1};

static veryshort sxwvtranslatev[40] = {
   -1, -1, 6, 7, -1, -1, -1, -1, -1, 0, 1, -1, -1, -1, -1, -1,
   -1, -1, 2, 3, -1, -1, -1, -1, -1, 4, 5, -1, -1, -1, -1, -1,
   -1, -1, 6, 7, -1, -1, -1, -1};

static veryshort shrgltranslatev[40] = {
   -1, -1, -1, 7, -1, -1, 5, -1, -1, -1, 6, -1, -1, 0, -1, -1,
   -1, -1, -1, 3, -1, -1, 1, -1, -1, -1, 2, -1, -1, 4, -1, -1,
   -1, -1, -1, 7, -1, -1, 5, -1};

static veryshort sptpdtranslatev[40] = {
   -1, -1, -1, -1, -1,  3, -1, -1, -1,  0, -1,  2, -1, -1,  1, -1,
   -1, -1, -1, -1, -1,  7, -1, -1, -1,  4, -1,  6, -1, -1,  5, -1,
   -1, -1, -1, -1, -1,  3, -1, -1};

static veryshort shypergalv[20] = {
   -1, -1, -1, 6, 7, 0, -1, 1, -1, -1, -1, 2, 3, 4, -1, 5, -1, -1, -1, 6};

static veryshort shypergaldhrglv[20] = {
   -1, -1, -1, 7, 5, 0, 6, 3, -1, -1, -1, 3, 1, 4, 2, 7, -1, -1, -1, 7};

static veryshort s3dmftranslateh[32] = {
   -1, 0, 1, 2, -1, -1, -1, -1, -1, 3, -1, -1, -1, -1, -1, -1,
   -1, 4, 5, 6, -1, -1, -1, -1, -1, 7, -1, -1, -1, -1, -1, -1};

static veryshort s3dmftranslatev[32] = {
   -1, 7, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, -1, -1, -1, -1,
   -1, 3, -1, -1, -1, -1, -1, -1, -1, 4, 5, 6, -1, -1, -1, -1};

static veryshort s3dmntranslateh[32] = {
   -1, 0, 1, 2, -1, -1, -1, -1, -1, -1, 3, -1, -1, -1, -1, -1,
   -1, 4, 5, 6, -1, -1, -1, -1, -1, -1, 7, -1, -1, -1, -1, -1};

static veryshort s3dmntranslatev[32] = {
   -1, -1, 7, -1, -1, -1, -1, -1, -1, 0, 1, 2, -1, -1, -1, -1,
   -1, -1, 3, -1, -1, -1, -1, -1, -1, 4, 5, 6, -1, -1, -1, -1};

static veryshort s_wingedstartranslate[40] = {
   -1, -1, -1, 7, -1, -1, -1, -1, -1, 0, 1, 2, -1, -1, -1, -1,
   -1, -1, -1, 3, -1, -1, -1, -1, -1, 4, 5, 6, -1, -1, -1, -1,
   -1, -1, -1, 7, -1, -1, -1, -1};

static veryshort jqttranslatev[40] = {
   -1, -1, -1, -1, -1, -1, 5, -1, -1, 6, -1, 7, -1, 0, -1, -1,
   -1, -1, -1, -1, -1, -1, 1, -1, -1, 2, -1, 3, -1, 4, -1, -1,
   -1, -1, -1, -1, -1, -1, 5, -1};

static veryshort bigdtranslatev[40] = {
   -1, -1, 8, 9, 11, -1, 10, -1, -1, -1, -1, -1, -1, 1, -1, 0,
   -1, -1, 2, 3,  5, -1,  4, -1, -1, -1, -1, -1, -1, 7, -1, 6,
   -1, -1, 8, 9, 11, -1, 10, -1};

static veryshort j23translatev[40] = {
   0,  0,  0,  4,  0,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  1,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,
   0,  0,  0,  4,  0,  5,  0,  0};

static veryshort qdmtranslatev[40] = {
   0,  0,  0,  0,  11, 0,  10, 0,   12, 13, 14, 15,  0,  1,  0,  0,
   0,  0,  0,  0,  3,  0,  2,  0,    4,  5,  6,  7,  0,  9,  0,  8,
   0,  0,  0,  0, 11,  0, 10,  0};

static veryshort bonetranslatev[40] = {
   0,  0,  0,  0,  5,  0, 10,  0,   0,  0,  6,  7,  0,  0,  0,  0,
   0,  0,  0,  0,  1,  0,  0,  0,   0,  0,  2,  3,  0,  0,  0,  4,
   0,  0,  0,  0,  5,  0, 10,  0};

static veryshort qtgtranslateh[40] = {
   -1, -1, -1, -1, -1, -1, 5, -1,   -1, -1, 6, 7, -1, 0, -1, -1,
   -1, -1, -1, -1, -1, -1, 1, -1,   -1, -1, 2, 3, -1, 4, -1, -1,
   -1, -1, -1, -1, -1, -1, 5, -1};

static veryshort shrgltranseqlh[40] = {
   7, -1, -1, -1, -1, -1, 5, -1,   3, -1, 6, -1, -1, 0, -1, -1,
   3, -1, -1, -1, -1, -1, 1, -1,   7, -1, 2, -1, -1, 4, -1, -1,
   7, -1, -1, -1, -1, -1, 5, -1};

static veryshort eqlizr[40] = {
   6, -1, -1, 5, -1, 7, -1, -1,   -1, -1, -1, -1, -1, -1, 0, -1,
   2, -1, -1, 1, -1, 3, -1, -1,   -1, -1, -1, -1, -1, -1, 4, -1,
   6, -1, -1, 5, -1, 7, -1, -1};

static veryshort eqlizl[40] = {
   -1, -1, -1, 6, -1, 7, -1, -1,   1, -1, -1, -1, -1, -1, 0, -1,
   -1, -1, -1, 2, -1, 3, -1, -1,   5, -1, -1, -1, -1, -1, 4, -1,
   -1, -1, -1, 6, -1, 7, -1, -1};

static veryshort starstranslatev[40] = {
   -1, -1, -1, -1, -1, 5, -1, -1,   -1, -1, 6, 7, -1, -1, 0, -1,
   -1, -1, -1, -1, -1, 1, -1, -1,   -1, -1, 2, 3, -1, -1, 4, -1,
   -1, -1, -1, -1, -1, 5, -1, -1};


// Two people are "casting partners" if they are adjacent in a miniwave relationship
// and their roll directions are toward each other.  This is a necessary (but not sufficient)
// condition for them to have engaged in some kind of casting with each other.
//
// This routine finds the casting partner, if any, of a given person.  Well, actually,
// that's true only if we give it the subject's actual roll direction.  In some cases
// we give a different roll direction (3rd argument) in order to make it find a different
// casting partner.  The written-over 4th argument encodes the location where the turning took place.

static int find_casting_partner(int i, const setup *s, uint32 roll_info_to_use, int & octantmask)
{
   const coordrec *thingptr = setup_attrs[s->kind].nice_setup_coords;
   if (!thingptr) {
      return -1;
   }

   int dir = s->people[i].id1 & 3;

   if ((roll_info_to_use & ROLL_DIRMASK) == ROLL_IS_L)
      dir ^= 2;
   else if ((roll_info_to_use & ROLL_DIRMASK) != ROLL_IS_R) {
      return -1;
   }

   int x = thingptr->xca[i];
   int y = thingptr->yca[i];
   int oldx = x;
   int oldy = y;

   switch (dir) {
   case 0: x += 4; break;
   case 1: y -= 4; break;
   case 2: x -= 4; break;
   default: y += 4; break;
   }

   // Find the person with whom he would have been casting.
   // Don't check whether that person agrees that casting is taking place;
   // that will be checked elsewhere.

   int place = thingptr->get_index_from_coords(x, y);
   if (place < 0)
      return -1;

   // Get the center point between them, and its octant.

   oldx += x;
   oldy += y;

   octantmask = 0;

   if (oldx > 0) octantmask |= 8;
   if (oldx < 0) octantmask |= 4;
   if (oldy > 0) octantmask |= 2;
   if (oldy < 0) octantmask |= 1;

   for (int j=s->rotation & 3 ; j>0 ; j--)
      octantmask = ((octantmask & 3) << 2) | (octantmask >> 3) | ((octantmask & 4) >> 1);

   return place;
}



static uint32 do_actual_array_call(
   setup *ss,
   const calldefn *callspec,
   callarray *linedefinition,
   callarray *coldefinition,
   uint32 newtb,
   uint32 funny,
   bool mirror,
   bool four_way_startsetup,
   int orig_elongation,
   int & desired_elongation,
   bool & check_peeloff_migration,
   setup *result) THROW_DECL
{
   int inconsistent_rotation = 0;
   uint32 resultflagsmisc = 0;
   int inconsistent_setup = 0;
   bool funny_ok1 = false;
   bool funny_ok2 = false;
   bool other_elongate = false;
   callarray *goodies;
   int real_index, northified_index;
   int num, halfnum;
   int i, j, k;
   setup newpersonlist;
   int newplacelist[MAX_PEOPLE];
   bool need_to_normalize = false;

   newpersonlist.clear_people();

   // This shouldn't happen, but we are being very careful here.
   if (ss->cmd.cmd_misc2_flags & CMD_MISC2__CTR_END_MASK)
      fail("Can't do \"snag/mystic\" with this call.");

   ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;
   result->eighth_rotation = 0;

   if (coldefinition && (coldefinition->callarray_flags & CAF__PLUSEIGHTH_ROTATION)) {
      result->eighth_rotation = 1;
      if (linedefinition && !(linedefinition->callarray_flags & CAF__PLUSEIGHTH_ROTATION))
         fail("Inconsistent rotation.");
   }
   else if (linedefinition && (linedefinition->callarray_flags & CAF__PLUSEIGHTH_ROTATION))
      result->eighth_rotation = 1;

   if ((callspec->callflags1 & CFLAG1_PARALLEL_CONC_END) ||
       (coldefinition && (coldefinition->callarray_flags & CAF__OTHER_ELONGATE)) ||
       (linedefinition && (linedefinition->callarray_flags & CAF__OTHER_ELONGATE)))
      other_elongate = true;

   if (other_elongate && ((desired_elongation+1) & 2))
      desired_elongation ^= 3;

   if (!newtb) {
      result->kind = nothing;   // Note that we get the benefit of the
                                // "CFLAG1_PARALLEL_CONC_END" stuff here.
      return 0;                 // This means that a counter rotate in
                                // an empty 1x2 will still change shape.
   }

   // Check that "linedefinition" has been set up if we will need it.

   goodies = (callarray *) 0;

   if ((newtb & 010) || four_way_startsetup) {
      assumption_thing t;

      if (!linedefinition) {
         switch (ss->kind) {
         case sdmd:
            fail("Can't handle people in diamond or PU quarter-tag for this call.");
         case s_trngl:
            fail("Can't handle people in triangle for this call.");
         case s_qtag:
            fail("Can't handle people in quarter tag for this call.");
         case s_ptpd:
            fail("Can't handle people in point-to-point diamonds for this call.");
         case s3x4:
            fail("Can't handle people in triple lines for this call.");
         case s2x3: case s2x4:
            fail("Can't handle people in lines for this call.");
         case s_short6:
            fail("Can't handle people in tall/short 6 for this call.");
         case s2x2:
            fail("Can't handle people in box of 4 for this call.");
         case s1x2:
            fail("Can't handle people in line of 2 for this call.");
         case s1x3:
            fail("Can't handle people in line of 3 for this call.");
         case s1x4:
            fail("Can't handle people in line of 4 for this call.");
         case s1x6:
            fail("Can't handle people in line of 6 for this call.");
         case s1x8:
            fail("Can't handle people in line of 8 for this call.");
         case s_galaxy:
            fail("Can't handle people in galaxy for this call.");
         default:
            fail("Can't do this call.");
         }
      }

      t.assumption = linedefinition->restriction;
      t.assump_col = 0;
      t.assump_both = 0;
      t.assump_cast = 0;
      t.assump_live = 0;
      t.assump_negate = 0;

      if (t.assumption != cr_none)
         check_restriction(ss, t, false,
                           linedefinition->callarray_flags & CAF__RESTR_MASK);
      goodies = linedefinition;
   }

   // Check that "coldefinition" has been set up if we will need it.

   if ((newtb & 1) && (!four_way_startsetup)) {
      assumption_thing t;

      if (!coldefinition) {
         switch (ss->kind) {
         case sdmd:
            fail("Can't handle people in diamond or normal quarter-tag for this call.");
         case s_trngl:
            fail("Can't handle people in triangle for this call.");
         case s_qtag: case s3x4:
            fail("Can't handle people in triple columns for this call.");
         case s2x3: case s2x4:
            fail("Can't handle people in columns for this call.");
         case s_short6:
            fail("Can't handle people in tall/short 6 for this call.");
         case s1x2:
            fail("Can't handle people in column of 2 for this call.");
         case s1x4:
            fail("Can't handle people in column of 4 for this call.");
         case s1x6:
            fail("Can't handle people in column of 6 for this call.");
         case s1x8:
            fail("Can't handle people in column of 8 for this call.");
         case s_galaxy:
            fail("Can't handle people in galaxy for this call.");
         default:
            fail("Can't do this call.");
         }
      }

      t.assumption = coldefinition->restriction;
      t.assump_col = 1;
      t.assump_both = 0;
      t.assump_cast = 0;
      t.assump_live = 0;
      t.assump_negate = 0;

      if (t.assumption != cr_none)
         check_restriction(ss, t, false,
                           coldefinition->callarray_flags & CAF__RESTR_MASK);

      // If we have linedefinition also, check for consistency.

      if (goodies) {
         // ***** should also check the other stupid fields!
         inconsistent_rotation =
            (goodies->callarray_flags ^ coldefinition->callarray_flags) & CAF__ROT;
         if (goodies->get_end_setup() != coldefinition->get_end_setup())
            inconsistent_setup = 1;
      }

      goodies = coldefinition;
   }

   if (!goodies) crash_print(__FILE__, __LINE__, newtb, ss);

   result->kind = goodies->get_end_setup();

   if (result->kind == s_normal_concentric) {
      // ***** this requires an 8-person call definition
      setup outer_inners[2];
      int outer_elongation;
      setup p1;

      if (inconsistent_rotation | inconsistent_setup)
         fail("This call is an inconsistent shape-changer.");

      if (funny) fail("Sorry, can't do this call 'funny'");

      p1.clear_people();

      for (real_index=0; real_index<8; real_index++) {
         personrec this_person = ss->people[real_index];
         if (this_person.id1) {
            uint32 z;
            int real_direction = this_person.id1 & 3;
            int d2 = (this_person.id1 << 1) & 4;
            northified_index = (real_index ^ d2);
            z = find_calldef((real_direction & 1) ? coldefinition : linedefinition,
                             ss, real_index, real_direction, northified_index);
            k = ((z >> 2) & 0x3F) ^ (d2 >> 1);
            install_person(&p1, k, ss, real_index);
            p1.people[k].id1 = do_slide_roll(p1.people[k].id1, z, real_direction);

            // For now, can't do fractional stable on this kind of call.
            p1.people[k].id1 &= ~STABLE_ALL_MASK;
         }
      }

      for (k=0; k<4; k++) {
         copy_person(&outer_inners[1], k, &p1, k);
         copy_person(&outer_inners[0], k, &p1, k+4);
      }

      clear_result_flags(&outer_inners[0]);
      clear_result_flags(&outer_inners[1]);
      outer_inners[0].kind = goodies->get_end_setup_out();
      outer_inners[1].kind = goodies->get_end_setup_in();
      outer_inners[0].rotation = (goodies->callarray_flags & CAF__ROT_OUT) ? 1 : 0;
      outer_inners[1].rotation = goodies->callarray_flags & CAF__ROT;
      outer_inners[0].eighth_rotation = 0;
      outer_inners[1].eighth_rotation = 0;

      // For calls defined by array with concentric end setup, the "other_elongate" flag,
      // which comes from "CFLAG1_PARALLEL_CONC_END" or "CAF__OTHER_ELONGATE",
      // turns on the outer elongation.
      outer_elongation = outer_inners[0].rotation;
      if (other_elongate) outer_elongation ^= 1;

      normalize_concentric(ss, schema_concentric, 1, outer_inners,
                           (outer_elongation & 1) + 1, 0, result);

      goto fixup;
   }
   else {
      uint32 lilresult_mask[2];
      setup_kind tempkind;

      result->rotation = goodies->callarray_flags & CAF__ROT;
      num = attr::slimit(ss)+1;
      halfnum = num >> 1;
      tempkind = result->kind;
      lilresult_mask[0] = 0;
      lilresult_mask[1] = 0;

      if (funny) {
         if ((ss->kind != result->kind) || result->rotation ||
             inconsistent_rotation || inconsistent_setup)
            fail("Can't do 'funny' shape-changer.");
      }

      // Check for a 1x4 call around the outside that
      // sends people far away without permission.
      if ((ss->kind == s1x4 || ss->kind == s1x6) &&
          ss->cmd.prior_elongation_bits & 0x40 &&
          !(ss->cmd.cmd_misc_flags & CMD_MISC__NO_CHK_ELONG)) {
         if (goodies->callarray_flags & CAF__NO_CUTTING_THROUGH) {
            fail_no_retry("Call has outsides going too far around the set.");
         }
      }

      // Check for people cutting through or working around an elongated 2x2 setup.

      if (ss->kind == s2x2) {
         uint32 groovy_elongation = orig_elongation >> 8;

         if ((groovy_elongation & 0x3F) != 0 &&
             (goodies->callarray_flags & CAF__NO_FACING_ENDS)) {
            for (int i=0; i<4; i++) {
               uint32 p = ss->people[i].id1;
               if (p != 0 && ((p-i-1) & 2) != 0 && ((p ^ groovy_elongation) & 1) == 0)
                  fail("Centers aren't staying in the center.");
            }
         }
      }

      if ((ss->kind == s2x2 || ss->kind == s2x3) &&
          (orig_elongation & 0x3F) != 0 &&
          !(ss->cmd.cmd_misc_flags & CMD_MISC__NO_CHK_ELONG)) {
         if (callspec->callflagsf & CFLAG2_NO_ELONGATION_ALLOWED)
            fail_no_retry("Call can't be done around the outside of the set.");

         if (goodies->callarray_flags & CAF__NO_CUTTING_THROUGH) {
            if (result->kind == s1x4)
               check_peeloff_migration = true;
            else if (ss->kind == s2x2) {
               for (int i=0; i<4; i++) {
                  uint32 p = ss->people[i].id1;
                  if (p != 0 && ((p-i-1) & 2) == 0 && ((p ^ orig_elongation) & 1) == 0)
                     fail("Call has outsides cutting through the middle of the set.");
               }
            }
            else if ((orig_elongation & 0x3F) == 2 - (ss->rotation & 1)) {
               for (int i=0; i<6; i++) {
                  uint32 p = ss->people[i].id1;
                  if (p != 0) {
                     if (i<3) p ^= 2;
                     if ((p&3) == 0)
                        fail("Call has outsides cutting through the middle of the set.");
                  }
               }
            }
         }
      }

      if (four_way_startsetup) {
         special_4_way_symm(linedefinition, ss, &newpersonlist, newplacelist,
                            lilresult_mask, result);
      }
      else if (setup_attrs[ss->kind].no_symmetry || ss->kind == s1x3 ||
               setup_attrs[result->kind].no_symmetry || result->kind == s1x3) {
         if (inconsistent_rotation | inconsistent_setup)
            fail("This call is an inconsistent shape-changer.");
         special_triangle(coldefinition, linedefinition, ss, &newpersonlist, newplacelist,
                          num, lilresult_mask, result);
      }
      else {
         int halfnumoutl, halfnumoutc, numoutl, numoutc;
         const veryshort *final_translatec = identity24;
         const veryshort *final_translatel = identity24;
         int rotfudge_line = 0;
         int rotfudge_col = 0;
         numoutl = attr::slimit(result)+1;
         numoutc = numoutl;
         // Handle a result setup with an odd size, e.g. s3x3.
         uint32 oddness = (numoutl & 1) ? numoutl-1 : 0;

         if (inconsistent_setup) {
            setup_kind other_kind = linedefinition->get_end_setup();

            if (inconsistent_rotation) {

               struct arraycallfixer {
                  setup_kind reskind;
                  setup_kind otherkind;
                  setup_kind finalkind;
                  const veryshort *final_c;
                  const veryshort *final_l;
                  bool onlyifequalize;
               };

               arraycallfixer arraycallfixtable[] = {
                  {s_spindle, s_crosswave, sx4dmd, ftcspn, ftlcwv, false},
                  {s_bone, s_qtag, sx4dmdbone, ftcbone, ftlbigqtg, false},
                  {s_short6, s_2x1dmd, s_short6, identity24, ftlshort6dmd, false},
                  {s_2x1dmd, s1x6, sx1x6, ftc2x1dmd, ftl2x1dmd, false},
                  {s2x4, s_qtag, sxequlize, ftequalize, ftlqtg, true}, // Complicated T-boned "transfer and []".
                  {s2x4, s_qtag, s2x4, identity24, qtg2x4, false},
                  {s_qtag, s2x4, s_qtag, identity24, f2x4qtg, false},
                  {s_c1phan, s2x4, s_c1phan, identity24, f2x4phan, false},
                  {nothing},
               };

               for (arraycallfixer *p = arraycallfixtable ; p->reskind != nothing ; p++) {
                  if (result->kind == p->reskind && other_kind == p->otherkind &&
                      (!p->onlyifequalize || (callspec->callflagsf & CFLAG2_EQUALIZE))) {
                     result->kind = p->finalkind;
                     tempkind = result->kind;
                     final_translatec = p->final_c;
                     final_translatel = p->final_l;
                     rotfudge_line = 3;

                     if (!(goodies->callarray_flags & CAF__ROT)) {
                        final_translatel += (attr::klimit(p->reskind) + 1) >> 1;
                        rotfudge_line += 2;
                     }

                     goto donewiththis;
                  }
               }

               if (result->kind == s2x4 && other_kind == s_hrglass) {
                  result->rotation = linedefinition->callarray_flags & CAF__ROT;
                  result->kind = s_hrglass;
                  tempkind = s_hrglass;
                  final_translatec = qtlqtg;

                  if (goodies->callarray_flags & CAF__ROT) {
                     rotfudge_col = 1;
                  }
                  else {
                     final_translatec += (attr::klimit(s_hrglass) + 1) >> 1;
                     rotfudge_col = 3;
                  }
               }
               else if (result->kind == s_qtag && other_kind == s_bone) {
                  result->rotation = linedefinition->callarray_flags & CAF__ROT;
                  result->kind = sx4dmdbone;
                  tempkind = sx4dmdbone;
                  rotfudge_col = 1;
                  final_translatel = ftcbone;

                  if (goodies->callarray_flags & CAF__ROT) {
                     final_translatec = &ftlbigqtg[4];
                  }
                  else {
                     final_translatec = &ftlbigqtg[0];
                     rotfudge_line = 2;
                  }
               }
               else
                  fail("T-bone call went to a weird setup.");
            }
            else {
               if (result->kind == s4x4 && other_kind == s2x4) {
                  numoutl = attr::klimit(other_kind)+1;
                  final_translatel = ftc4x4;
               }
               else if (result->kind == srigger12 && other_kind == s1x12) {
                  final_translatel = ftrig12;
               }
               else if (result->kind == s4x4 && other_kind == s_qtag) {
                  numoutl = attr::klimit(other_kind)+1;
                  result->kind = sbigh;
                  tempkind = sbigh;
                  final_translatec = ft4x4bh;
                  final_translatel = ftqtgbh;
               }
               else if (result->kind == s4x4 && other_kind == s2x6) {
                  numoutl = attr::klimit(other_kind)+1;
                  result->kind = s4x6;
                  tempkind = s4x6;
                  final_translatec = ft4x446;
                  final_translatel = ft2646;
               }
               else if (result->kind == sbigbone && other_kind == s3x4) {
                  tempkind = result->kind;
                  final_translatel = ft3x4bb;
               }
               else if (result->kind == s2x4 && other_kind == s2x6) {
                  numoutl = attr::klimit(other_kind)+1;
                  result->kind = s2x6;
                  tempkind = s2x6;
                  final_translatec = ft2x42x6;
               }
               else if (result->kind == s_c1phan && other_kind == s2x4) {
                  numoutl = attr::klimit(other_kind)+1;
                  final_translatel = ftcphan;
               }
               else if (result->kind == s2x4 && other_kind == s_bone) {
                  // We seem to be doing a complicated T-boned "busy []".
                  // Check whether we have been requested to "equalize",
                  // in which case we can do glorious things like going into
                  // a center diamond.
                  if ((callspec->callflagsf & CFLAG2_EQUALIZE)) {
                     result->kind = shypergal;
                     tempkind = shypergal;
                     final_translatec = qhypergalc;
                     final_translatel = qhypergall;
                  }
                  else {
                     final_translatel = qtlbone;
                  }
               }
               else if (result->kind == s_bone && other_kind == s2x4) {
                  final_translatel = qtlbone2;
               }
               else if (result->kind == s1x8 && other_kind == s_crosswave) {
                  final_translatel = qtlxwv;
               }
               else if (result->kind == s1x8 && other_kind == s_qtag) {
                  final_translatel = qtl1x8;
               }
               else if (result->kind == s_rigger && other_kind == s1x8) {
                  final_translatel = qtlrig;
               }
               else if (result->kind == s_hyperglass && other_kind == s_qtag) {
                  numoutl = attr::klimit(other_kind)+1;
                  final_translatel = qtlgls;
               }
               else
                  fail("T-bone call went to a weird setup.");
            }
         }
         else if (inconsistent_rotation) {
            if (result->kind == s2x4) {
               result->kind = s4x4;
               tempkind = s4x4;
               final_translatec = ftc2x4;

               if (goodies->callarray_flags & CAF__ROT) {
                  final_translatel = &ftl2x4[0];
                  rotfudge_line = 3;
               }
               else {
                  final_translatel = &ftl2x4[4];
                  rotfudge_line = 1;
               }
            }
            else if (result->kind == s_qtag) {
               result->kind = s_hyperbone;
               tempkind = s_hyperbone;
               final_translatec = ftqthbv;

               if (goodies->callarray_flags & CAF__ROT) {
                  final_translatel = &ftqthbh[0];
                  rotfudge_line = 3;
               }
               else {
                  final_translatel = &ftqthbh[4];
                  rotfudge_line = 1;
               }
            }
            else
               fail("This call is an inconsistent shape-changer.");
         }

      donewiththis:

         halfnumoutl = numoutl >> 1;
         halfnumoutc = numoutc >> 1;

         for (real_index=0; real_index<num; real_index++) {
            const veryshort *final_translate;
            int kt;
            callarray *the_definition;
            personrec this_person = ss->people[real_index];
            newpersonlist.clear_person(real_index);
            if (this_person.id1) {
               int final_direction, d2out, thisnumout;
               int real_direction = this_person.id1 & 3;
               int d2 = ((this_person.id1 >> 1) & 1) * halfnum;
               northified_index = (real_index + d2) % num;

               if (real_direction & 1) {
                  d2out = ((this_person.id1 >> 1) & 1) * halfnumoutc;
                  thisnumout = numoutc;
                  final_translate = final_translatec;
                  final_direction = rotfudge_col;
                  the_definition = coldefinition;
               }
               else {
                  d2out = ((this_person.id1 >> 1) & 1) * halfnumoutl;
                  thisnumout = numoutl;
                  final_translate = final_translatel;
                  final_direction = rotfudge_line;
                  the_definition = linedefinition;
               }

               final_direction = (final_direction+real_direction) & 3;

               uint32 z = find_calldef(the_definition, ss, real_index, real_direction, northified_index);

               uint32 where = (z >> 2) & 0x3F;

               if (oddness != 0) {
                  if (where < oddness) {
                     where += d2out;
                     if (where >= oddness) where -= oddness;
                  }
               }
               else {
                  where += d2out;
                  where = where % thisnumout;
               }

               newpersonlist.people[real_index].id1 =
                  do_slide_roll(this_person.id1, z, final_direction);

               if (this_person.id1 & STABLE_ENAB)
                  do_stability(&newpersonlist.people[real_index].id1,
                               z/DBSTAB_BIT,
                               z + final_direction - real_direction + result->rotation,
                               false);

               newpersonlist.people[real_index].id2 = this_person.id2;
               newpersonlist.people[real_index].id3 = this_person.id3;
               kt = final_translate[where];
               if (kt < 0) fail("T-bone call went to a weird and confused setup.");
               newplacelist[real_index] = kt;
               lilresult_mask[0] |= (1 << kt);
            }
         }
      }

      // If the result setup size is larger than the starting setup size, we assume that this call
      // has a concocted result setup (e.g. squeeze the galaxy, unwrap the galaxy, change your
      // image), and we try to compress it.  We claim that, if the result size given
      // in the calls database is bigger than the starting size, as it is for those calls, we
      // don't really want that big setup, but want to compress it immediately if possible.
      // Q: Why don't we just let the natural setup normalization that will occur later do this for us?
      // A: That only happens at the top level.  In this case, we consider the compression
      //          to be part of doing the call.  If we someday were able to do a reverse flip of split
      // phantom galaxies, we would want each galaxy to compress itself to a 2x4 before
      // reassembling them.
      //
      // When we do a 16-matrix circulate in a 4x4 start setup, we do NOT want to compress
      //  the 4x4 to a 2x4!!!!!  This is why we compare the beginning and ending setup sizes.

      if (!(goodies->callarray_flags & CAF__NO_COMPRESS) &&
          (result->kind == s_hyperglass ||
           result->kind == s_hyperbone ||
           result->kind == sx1x6 ||
           result->kind == sx1x8 ||
           result->kind == shypergal ||
           attr::slimit(ss) < attr::slimit(result))) {
         const veryshort *permuter = (const veryshort *) 0;
         int rotator = 0;

         switch (result->kind) {
         case s4x4:
            // See if people landed on 2x4 spots.
            if ((lilresult_mask[0] & 0x7171) == 0) {
               result->kind = s2x4;
               permuter = galtranslateh;
            }
            else if ((lilresult_mask[0] & 0x1717) == 0) {
               result->kind = s2x4;
               permuter = galtranslatev;
               rotator = 1;
            }
            else {
               // If fakery occurred and we were not able to compress it, that is an error.
               // It means the points got confused on a reverse flip the galaxy.
               // If we were not able to compress a 4x4 but no fakery occurred,
               // we let it pass.  That simply means that the unwrappers were T-boned
               // in an unwrap the galaxy, so they led out in strange directions.
               // Is this the right thing to do?  Do we want to allow
               // T-boned reverse flip?  Probably not.

               if (tempkind != s4x4) fail("Galaxy call went to improperly-formed setup.");
            }
            break;
         case s_c1phan:
            // See if people landed on 2x4 spots.
            if ((lilresult_mask[0] & 0x7B7B) == 0) {
               result->kind = s2x4;
               permuter = phantranslateh;
            }
            else if ((lilresult_mask[0] & 0xB7B7) == 0) {
               result->kind = s2x4;
               permuter = phantranslatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0x5A5A) == 0 || (lilresult_mask[0] & 0x9696) == 0) {
               result->kind = s2x4;
               permuter = phantranslateh;
            }
            else if ((lilresult_mask[0] & 0xA5A5) == 0 || (lilresult_mask[0] & 0x6969) == 0) {
               result->kind = s2x4;
               permuter = phantranslatev;
               rotator = 1;
            }
            break;
         case s_thar:
            if ((lilresult_mask[0] & 0x66) == 0) {         // Check horiz dmd spots.
               result->kind = sdmd;
               permuter = sdmdtranslateh;
            }
            else if ((lilresult_mask[0] & 0x99) == 0) {    // Check vert dmd spots.
               result->kind = sdmd;
               permuter = sdmdtranslatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xCC) == 0) {    // Check horiz line spots.
               result->kind = s1x4;
               permuter = stharlinetranslateh;
            }
            else if ((lilresult_mask[0] & 0x33) == 0) {    // Check vert line spots.
               result->kind = s1x4;
               permuter = stharlinetranslatev;
               rotator = 1;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case s8x8:
            // See if people landed on 2x8 or 4x6 spots.
            result->kind = s2x8;
            permuter = octtranslatev+16;

            if ((lilresult_mask[0] & 0x333F11FFU) == 0 &&
                (lilresult_mask[1] & 0x333F11FFU) == 0) {
               result->kind = s4x6;
               permuter = octt4x6latev+16;
            }
            else if ((lilresult_mask[0] & 0x11FF333FU) == 0 &&
                     (lilresult_mask[1] & 0x11FF333FU) == 0) {
               result->kind = s4x6;
               permuter = octt4x6latev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0x0FFF7777U) == 0 &&
                     (lilresult_mask[1] & 0x0FFF7777U) == 0) {
               permuter = octtranslatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0x77770FFFU) != 0 ||
                     (lilresult_mask[1] & 0x77770FFFU) != 0)
               fail("Call went to improperly-formed setup.");

            need_to_normalize = true;
            break;
         case sx1x16:
            if ((lilresult_mask[0] & 0x00FF00FFU) == 0) {
               permuter = hextranslatev;  // 1x16 spots, vertical.
               result->kind = s1x16;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xFF00FF00U) == 0) {
               permuter = hextranslatev+8;  // 1x16 spots, horizontal.
               result->kind = s1x16;
            }
            else if ((lilresult_mask[0] & 0x3F3F3F3FU) == 0) {
               permuter = hthartranslate;   // thar spots.
               result->kind = s_thar;
            }
            else if ((lilresult_mask[0] & 0x9F3F9F3FU) == 0) {
               permuter = hxwtranslatev;  // crosswave spots, vertical.
               result->kind = s_crosswave;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0x3F9F3F9FU) == 0) {
               permuter = hxwtranslatev+8;  // crosswave spots, horizontal.
               result->kind = s_crosswave;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case sx1x6:
            if ((lilresult_mask[0] & 00707U) == 0) {
               permuter = h1x6translatev;  // 1x6 spots, vertical.
               result->kind = s1x6;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 07070U) == 0) {
               permuter = h1x6translatev+3;  // 1x6 spots, horizontal.
               result->kind = s1x6;
            }
            else if ((lilresult_mask[0] & 05151U) == 0) {
               permuter = h1x6translate2x1d;  // 2x1 diamond
               result->kind = s_2x1dmd;
            }
            else if ((lilresult_mask[0] & 01111U) == 0) {
               permuter = h1x6thartranslate;   // thar spots.
               result->kind = s_thar;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case sx1x8:
            if (attr::slimit(ss) == 3) {
               // This is for "1/2 ripoff".  We have to figure out how to orient the diamond spots,
               // even in the presence of T-bones and various things that are not locally symmetric.
               // Anyone deep in the center (that would be a trailing beau, they walk directly into
               // the center) defines it, as long as no one else is inconsistent with that.
               // Anyone far to the outside (that would be a lead beau, they always take the
               // outside track) defines it, as long as no one else is inconsistent with that.
               // The people we aren't sure about are the lead belles, doing the 1/2 zoom.  They
               // will sometimes be in close, and sometimes out.  They have to take their cues from
               // the other people.  Similarly for the trailing belles.  They slide over and may be
               // facing directly in the center, or may be far apart, depending on what the others do.
               if ((lilresult_mask[0] & ~0x7F78U) == 0 ||  // Anyone deep inside, with no one else conflicting
                   (lilresult_mask[0] & ~0x787FU) == 0 ||
                   (lilresult_mask[0] & ~0xFE1EU) == 0 ||  // Anyone far outside
                   (lilresult_mask[0] & ~0x1EFEU) == 0 ||
                   (lilresult_mask[0] & ~0x3C3CU) == 0) {  // Generally inside and outside
                  permuter = hxwvdmdtranslate3012;
                  result->kind = sdmd;
                  rotator = 1;
               }
               else if ((lilresult_mask[0] & ~0xF787U) == 0 ||  // Anyone deep inside, with no one else conflicting
                        (lilresult_mask[0] & ~0x87F7U) == 0 ||
                        (lilresult_mask[0] & ~0xE1EFU) == 0 ||  // Anyone far outside
                        (lilresult_mask[0] & ~0xEFE1U) == 0 ||
                        (lilresult_mask[0] & ~0xC3C3U) == 0) {  // Generally inside and outside
                  permuter = hxwvdmdtranslate3012+4;
                  result->kind = sdmd;
               }
               else
                  fail("Call went to improperly-formed setup.");
            }
            else if ((lilresult_mask[0] & 0x0F0FU) == 0) {
               warn(warn__4_circ_tracks);
               permuter = h1x8translatev;    // 1x8 spots, vertical.
               result->kind = s1x8;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xF0F0U) == 0) {
               warn(warn__4_circ_tracks);
               permuter = h1x8translatev+4;  // 1x8 spots, horizontal.
               result->kind = s1x8;
            }
            else if ((lilresult_mask[0] & 0x9999U) == 0) {
               warn(warn__4_circ_tracks);
               permuter = h1x8thartranslate9999;   // thar spots.
               result->kind = s_thar;
            }
            else if ((lilresult_mask[0] & 0x3C3CU) == 0) {
               permuter = h1x8thartranslatec3c3+4;  // crosswave spots, horizontal.
               result->kind = s_crosswave;
            }
            else if ((lilresult_mask[0] & 0xC3C3U) == 0) {
               permuter = h1x8thartranslatec3c3;   // crosswave spots, vertical.
               result->kind = s_crosswave;
               rotator = 1;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case sxequlize:
            if ((lilresult_mask[0] & 0xD6BFD6BFU) == 0) {
               result->kind = s2x4;
               permuter = eqlizr+8;
            }
            else if ((lilresult_mask[0] & 0xBFD6BFD6U) == 0) {
               result->kind = s2x4;
               permuter = eqlizr;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xD7BED7BEU) == 0) {
               result->kind = s2x4;
               permuter = eqlizl+8;
            }
            else if ((lilresult_mask[0] & 0xBED7BED7U) == 0) {
               result->kind = s2x4;
               permuter = eqlizl;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xBFD3BFD3U) == 0) {
               result->kind = s_qtag;
               permuter = qtgtranslateh+8;
            }
            else if ((lilresult_mask[0] & 0xD3BFD3BFU) == 0) {
               result->kind = s_qtag;
               permuter = qtgtranslateh;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xBEDBBEDB) == 0 || (lilresult_mask[0] & 0xBFDABFDA) == 0) {
               result->kind = s_hrglass;
               permuter = shrgltranseqlh+8;
            }
            else if ((lilresult_mask[0] & 0xDBBEDBBE) == 0 || (lilresult_mask[0] & 0xDABFDABF) == 0) {
               result->kind = s_hrglass;
               permuter = shrgltranseqlh;
               rotator = 1;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case sx4dmd:
            if ((lilresult_mask[0] & 0xD7BFD7BFU) == 0) {
               result->kind = s2x3;
               permuter = j23translatev+8;
            }
            else if ((lilresult_mask[0] & 0xBFD7BFD7U) == 0) {
               result->kind = s2x3;
               permuter = j23translatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xFFF1FFF1) == 0) {
               // Check horiz 1x6 spots.
               result->kind = s1x6;
               permuter = s1x6translateh;
            }
            else if ((lilresult_mask[0] & 0xF1FFF1FF) == 0) {
               // Check vert 1x6 spots.
               result->kind = s1x6;
               permuter = s1x6translatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xBFD5BFD5U) == 0) {
               result->kind = s_qtag;
               permuter = jqttranslatev+8;
            }
            else if ((lilresult_mask[0] & 0xD5BFD5BFU) == 0) {
               result->kind = s_qtag;
               permuter = jqttranslatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xBFD3BFD3U) == 0) {
               result->kind = s_qtag;
               permuter = qtgtranslateh+8;
            }
            else if ((lilresult_mask[0] & 0xD3BFD3BFU) == 0) {
               result->kind = s_qtag;
               permuter = qtgtranslateh;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & ~0x204C204CU) == 0) {
               result->kind = s_2stars;
               permuter = starstranslatev+8;
            }
            else if ((lilresult_mask[0] & ~0x4C204C20U) == 0) {
               result->kind = s_2stars;
               permuter = starstranslatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & ~0xA05CA05CU) == 0) {
               result->kind = sbigdmd;
               permuter = bigdtranslatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & ~0x5CA05CA0U) == 0) {
               result->kind = sbigdmd;
               permuter = bigdtranslatev+8;
            }
            else if ((lilresult_mask[0] & 0xF3F9F3F9) == 0) {
               // Check horiz xwv spots.
               result->kind = s_crosswave;
               permuter = sxwvtranslatev+8;
            }
            else if ((lilresult_mask[0] & 0xF9F3F9F3) == 0) {
               // Check vert xwv spots.
               result->kind = s_crosswave;
               permuter = sxwvtranslatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xFDF1FDF1) == 0) {
               // Check horiz 3x1dmd spots w/points out far.
               result->kind = s3x1dmd;
               permuter = s3dmftranslateh;
            }
            else if ((lilresult_mask[0] & 0xF1FDF1FD) == 0) {
               // Check vert 3x1dmd spots w/points out far.
               result->kind = s3x1dmd;
               permuter = s3dmftranslatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xFBF1FBF1) == 0) {
               // Check horiz 3x1dmd spots w/points in close.
               result->kind = s3x1dmd;
               permuter = s3dmntranslateh;
            }
            else if ((lilresult_mask[0] & 0xF1FBF1FB) == 0) {
               // Check vert 3x1dmd spots w/points in close.
               result->kind = s3x1dmd;
               permuter = s3dmntranslatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xF7F1F7F1) == 0) {
               result->kind = s_wingedstar;
               permuter = s_wingedstartranslate+8;
            }
            else if ((lilresult_mask[0] & 0xF1F7F1F7) == 0) {
               result->kind = s_wingedstar;
               permuter = s_wingedstartranslate;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xAF50AF50U) == 0) {
               result->kind = s4dmd;
               permuter = qdmtranslatev+8;
            }
            else if ((lilresult_mask[0] & 0x50AF50AFU) == 0) {
               result->kind = s4dmd;
               permuter = qdmtranslatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xB7DBB7DB) == 0) {
               // Check horiz hourglass spots.
               result->kind = s_hrglass;
               permuter = shrgltranslatev+8;
            }
            else if ((lilresult_mask[0] & 0xDBB7DBB7) == 0) {
               // Check vert hourglass spots.
               result->kind = s_hrglass;
               permuter = shrgltranslatev;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xDFB5DFB5) == 0) {
               // Check horiz ptpd spots.
               result->kind = s_ptpd;
               permuter = sptpdtranslatev+8;
            }
            else if ((lilresult_mask[0] & 0xB5DFB5DF) == 0) {
               // Check vert ptpd spots.
               result->kind = s_ptpd;
               permuter = sptpdtranslatev;
               rotator = 1;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case sx4dmdbone:
            if ((lilresult_mask[0] & 0xEF73EF73U) == 0) {
               result->kind = s_bone;
               permuter = bonetranslatev+8;
            }
            else if ((lilresult_mask[0] & 0x73EF73EFU) == 0) {
               result->kind = s_bone;
               permuter = bonetranslatev;
               rotator = 1;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case shypergal:
            if ((lilresult_mask[0] & 0xFFFF7474) == 0) {
               result->kind = s2x4;
               permuter = shypergalv+4;
            }
            else if ((lilresult_mask[0] & 0xFFFF4747) == 0) {
               result->kind = s2x4;
               permuter = shypergalv;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xFFFF7878) == 0) {
               result->kind = s_dhrglass;
               permuter = &shypergaldhrglv[4];
            }
            else if ((lilresult_mask[0] & 0xFFFF8787) == 0) {
               result->kind = s_dhrglass;
               permuter = shypergaldhrglv;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xFFFFF0F0) == 0) {
               result->kind = s_dhrglass;
               permuter = &shypergaldhrglv[4];
            }
            else if ((lilresult_mask[0] & 0xFFFF0F0F) == 0) {
               result->kind = s_dhrglass;
               permuter = shypergaldhrglv;
               rotator = 1;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case s_hyperglass:

            // Investigate possible diamonds and 1x4's.  If enough centers
            // and ends (points) are present to determine unambiguously what
            // result setup we must create, we of course do so.  Otherwise, if
            // the centers are missing but points are present, we give preference
            // to making a 1x4, no matter what the call definition said the ending
            // setup was.  But if centers are present and the points are missing,
            // we go do diamonds if the original call definition wanted diamonds.

            if ((lilresult_mask[0] & 05757) == 0 && tempkind == sdmd) {
               result->kind = sdmd;     // Only centers present, and call wanted diamonds.
               permuter = dmdhyperv+3;
            }
            else if ((lilresult_mask[0] & 07575) == 0 && tempkind == sdmd) {
               result->kind = sdmd;     // Only centers present, and call wanted diamonds.
               permuter = dmdhyperv;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 05656) == 0 &&
                     (goodies->callarray_flags & CAF__REALLY_WANT_DIAMOND)) {
               result->kind = sdmd;     // Setup is consistent with diamonds,
                                        //  though maybe centers are absent,
                                        // but user specifically requested diamonds.
               permuter = dmdhyperv+3;
            }
            else if ((lilresult_mask[0] & 06565) == 0 &&
                     (goodies->callarray_flags & CAF__REALLY_WANT_DIAMOND)) {
               result->kind = sdmd;     // Setup is consistent with diamonds,
                                        // though maybe centers are absent,
                                        // but user specifically requested diamonds.
               permuter = dmdhyperv;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 07474) == 0) {
               result->kind = s1x4;     // Setup is consistent with lines,
                                        // though maybe centers are absent.
               permuter = linehyperh;
            }
            else if ((lilresult_mask[0] & 04747) == 0) {
               result->kind = s1x4;     // Setup is consistent with lines,
                                        // though maybe centers are absent.
               permuter = linehyperv;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 05656) == 0) {
               result->kind = sdmd;     // Setup is consistent with diamonds,
                                        // though maybe centers are absent.
               permuter = dmdhyperv+3;
            }
            else if ((lilresult_mask[0] & 06565) == 0) {
               result->kind = sdmd;     // Setup is consistent with diamonds,
                                        // though maybe centers are absent.
               permuter = dmdhyperv;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 01212) == 0) {
               result->kind = s_hrglass;    // Setup is an hourglass.
               permuter = galhyperv+3;
            }
            else if ((lilresult_mask[0] & 02121) == 0) {
               result->kind = s_hrglass;    // Setup is an hourglass.
               permuter = galhyperv;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 03030) == 0) {
               result->kind = s_qtag;    // Setup is qtag/diamonds.
               permuter = qtghyperh;
            }
            else if ((lilresult_mask[0] & 00303) == 0) {
               result->kind = s_qtag;    // Setup is qtag/diamonds.
               permuter = qtghyperv;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 05555) == 0) {
               result->kind = s_star;
               permuter = starhyperh;
            }
            else if ((lilresult_mask[0] & 06666) == 0) {
               result->kind = s_star;        // Actually, this is a star with all people
                                             // sort of far away from the center.  We may
                                             // need to invent a new setup, "farstar".
               permuter = fstarhyperh;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case s_hyperbone:
            if ((lilresult_mask[0] & 0x0F0F) == 0) {
               result->kind = s_bone;
               permuter = hyperbonev+4;
            }
            else if ((lilresult_mask[0] & 0xF0F0) == 0) {
               result->kind = s_bone;
               permuter = hyperbonev;
               rotator = 1;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case s_hyper3x4:
            if ((lilresult_mask[0] & ~0xFC0FC0) == 0) {
               result->kind = sbigh;
               permuter = hyper3x4v+6;
            }
            else if ((lilresult_mask[0] & ~0x03F03F) == 0) {
               result->kind = sbigh;
               permuter = hyper3x4v;
               rotator = 1;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case s_tinyhyperbone:
            if ((lilresult_mask[0] & 0x0FFF) == 0) {       // These 4 go to a trngl4
               result->kind = s_trngl4;
               permuter = tinyhyperbonet;
               rotator = 3;
            }
            else if ((lilresult_mask[0] & 0xF0FF) == 0) {
               result->kind = s_trngl4;
               permuter = tinyhyperbonet+4;
               rotator = 2;
            }
            else if ((lilresult_mask[0] & 0xFF0F) == 0) {
               result->kind = s_trngl4;
               permuter = tinyhyperbonet+8;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xFFF0) == 0) {
               result->kind = s_trngl4;
               permuter = tinyhyperbonet+12;
            }
            else if ((lilresult_mask[0] & 0xF3F3) == 0) {  // Next 2 go to a 1x4!
               result->kind = s1x4;
               permuter = tinyhyperbonel;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0x3F3F) == 0) {
               result->kind = s1x4;
               permuter = tinyhyperbonel+4;
            }
            else if ((lilresult_mask[0] & 0xCFCF) == 0) {  // Next 2 go to a 2x2!
               result->kind = s2x2;
               permuter = tinyhyperboneb+4;
            }
            else if ((lilresult_mask[0] & 0xFCFC) == 0) {
               result->kind = s2x2;
               permuter = tinyhyperboneb;
               rotator = 1;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case slittlestars:
            if ((lilresult_mask[0] & 0xCC) == 0) {
               result->kind = s2x2;
               permuter = lilstar3;
            }
            else if ((lilresult_mask[0] & 0x33) == 0) {
               result->kind = s1x4;
               permuter = lilstar4;
            }
            else if ((lilresult_mask[0] & 0x2D) == 0) {
               result->kind = s_trngl4;
               permuter = lilstar1;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 0xD2) == 0) {
               result->kind = s_trngl4;
               permuter = lilstar2;
               rotator = 3;
            }
            else
               fail("Call went to improperly-formed setup.");
            break;
         case sbigdmd:
            if ((lilresult_mask[0] & 02222) == 0) {        // All outside.
               result->kind = s_qtag;
               permuter = qtbd1;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 04141) == 0) {   // All inside.
               result->kind = s_qtag;
               permuter = qtbd2;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 02121) == 0) {   // Some inside, some outside.
               result->kind = s_qtag;
               permuter = qtbd3;
               rotator = 1;
            }
            else if ((lilresult_mask[0] & 04242) == 0) {   // Some inside, some outside.
               result->kind = s_qtag;
               permuter = qtbd4;
               rotator = 1;
            }
            break;
         case s2x3:
         case s2x5:
         case s3x3:
         case s1p5x4:
         case s_bone6:
            // This call turned a smaller setup (like a 1x4) into a 2x3 or something similar.
            // It is presumably a call like "pair the line" or "step and slide".
            // Flag the result setup so that the appropriate phantom-squashing
            // will take place if two of these results are placed end-to-end.
            // We also do this if a 3x3 pair the line went to a 2x5.
            resultflagsmisc |= RESULTFLAG__EXPAND_TO_2X3;
            break;
         case s3x4:
            if (ss->kind == s1x4) {

               // This could be the even more glorious form of the above, for "fold".
               // If we can strip it down to a 2x3 (because the ends were the ones doing
               // the fold), do so now.  In any case, set the flag so that the 3-to-2
               // squashing can take place later.

               if ((lilresult_mask[0] & 03131) == 0) {
                  result->kind = s2x3;
                  permuter = q3x4xx1;
                  rotator = 1;
               }
               else if ((lilresult_mask[0] & 01717) == 0) {
                  result->kind = s1x4;
                  permuter = q3x4xx2;
                  rotator = 0;
               }
               else if ((lilresult_mask[0] & 01567) == 0 || (lilresult_mask[0] & 01673) == 0) {
                  result->kind = s_trngl4;
                  permuter = q3x4xx3;
                  rotator = 1;
               }
               else if ((lilresult_mask[0] & 06715) == 0 || (lilresult_mask[0] & 07316) == 0) {
                  result->kind = s_trngl4;
                  permuter = q3x4xx4;
                  rotator = 3;
               }

               resultflagsmisc |= RESULTFLAG__EXPAND_TO_2X3;
            }
            break;
         }

         if (permuter) {
            uint32 r = 011*((-rotator) & 3);

            for (real_index=0; real_index<num; real_index++) {
               if (ss->people[real_index].id1) {
                  newplacelist[real_index] = permuter[newplacelist[real_index]];
                  if (rotator)
                     newpersonlist.people[real_index].id1 =
                        rotperson(newpersonlist.people[real_index].id1, r);
               }
            }

            result->rotation += rotator;
         }
      }
   }

   // Install all the people.

   {
      // We create an especially glorious collision collector here, with all
      // the stuff for handling the nuances of the call definition and the
      // assumption.

      collision_collector CC(mirror, &ss->cmd, callspec);

      for (real_index=0; real_index<num; real_index++) {
         personrec newperson = newpersonlist.people[real_index];
         if (newperson.id1) {
            if (funny) {
               if (newperson.id1 != ~0U) {  // We only handle people who haven't been erased.
                  k = real_index;
                  j = real_index;    // J will move twice as fast as k, looking for a loop not containing starting point.
                  do {
                     j = newplacelist[j];
                     // If hit a phantom, we can't proceed.
                     if (!newpersonlist.people[j].id1) fail("Can't do 'funny' call with phantoms.");
                     // If hit an erased person, we have clearly hit a loop not containing starting point.
                     else if (newpersonlist.people[j].id1 == ~0U) break;
                     j = newplacelist[j];
                     if (!newpersonlist.people[j].id1) fail("Can't do 'funny' call with phantoms.");
                     else if (newpersonlist.people[j].id1 == ~0U) break;
                     k = newplacelist[k];
                     if (k == real_index) goto funny_win;
                  } while (k != j);

                  // This person can't move, because he moves into a loop
                  // not containing his starting point.
                  k = real_index;
                  newperson.id1 = (ss->people[real_index].id1 & ~NSLIDE_ROLL_MASK) | (3*NROLL_BIT);
                  newperson.id2 = ss->people[real_index].id2;
                  newperson.id3 = ss->people[real_index].id3;
                  result->people[k] = newperson;
                  newpersonlist.people[k].id1 = ~0U;
                  funny_ok1 = true;    // Someone decided not to move.  Hilarious.
                  goto funny_end;

               funny_win:
                  /* Move the entire loop, replacing people with -1. */
                  k = real_index;
                  j = 0;      /* See how long the loop is. */
                  do {
                     newperson = newpersonlist.people[k];
                     newpersonlist.people[k].id1 = ~0U;
                     k = newplacelist[k];
                     result->people[k] = newperson;
                     j++;
                  } while (k != real_index);

                  if (j > 2) warn(warn__hard_funny);
                  funny_ok2 = true;    // Someone was able to move.  Hysterical.
                  // Actually, I don't see how this test can fail.
                  // Someone can always move!

               funny_end: ;
               }
            }
            else {              // Un-funny move.
               CC.install_with_collision(result, newplacelist[real_index],
                                         &newpersonlist, real_index, 0);
            }
         }
      }

      if (funny && (!funny_ok1 || !funny_ok2))
         warn(warn__not_funny);

      call_with_name *maybe_call = (ss->cmd.parseptr &&
                                    ss->cmd.parseptr->concept &&
                                    ss->cmd.parseptr->concept->kind == marker_end_of_list) ?
         ss->cmd.parseptr->call : (call_with_name *) 0;

      merge_action action = (maybe_call && (maybe_call->the_defn.callflags1 & CFLAG1_TAKE_RIGHT_HANDS_AS_COUPLES)) ?
         merge_c1_phantom_real_couples : merge_strict_matrix;

      CC.fix_possible_collision(result, action, goodies->callarray_flags, ss);
   }

   // Fix up "dixie tag" result if fraction was 1/4.
   if ((ss->cmd.cmd_misc3_flags & CMD_MISC3__PARENT_COUNT_IS_ONE) &&
       ss->cmd.callspec == base_calls[base_call_dixiehalftag] &&
       result->kind == s2x2) {
      // We just did a "dixie 1/2 tag" but will want to back up to the 1/4 position.
      // Need to change handedness.
      uint32 newtb99 = or_all_people(result);
      if (!(newtb99 & 001)) {
         result->swap_people(0, 1);
         result->swap_people(2, 3);
      }
      else if (!(newtb99 & 010)) {
         result->swap_people(0, 3);
         result->swap_people(2, 1);
      }
      else
         fail("Can't figure out what you want.");
   }

   fixup:

   if (need_to_normalize || result->kind == s_c1phan)
      normalize_setup(result, plain_normalize, true);

   reinstate_rotation(ss, result);

   // Handle the "casting overflow" calculation.
   //
   // The "OVERCAST_BIT" on a person means:
   //   This person has an adjacent casting partner, their roll directions are
   //   toward each other, and, prior to the just completed call they were in an
   //   adjacent casting partner relationship, though perhaps not with correct
   //   roll direction.
   //
   // So, for example, the centers of the wave resulting from "criss cross your neighbor"
   //   are adjacent, with roll direction toward each other, but the bit is not on, because
   //   they were not adjacent before the call.

   if (!ss->cmd.callspec || (ss->cmd.callspec->the_defn.callflagsf & CFLAG2_OVERCAST_TRANSPARENT) == 0) {
      // Save the original locations.
      // ****** make this a nice routine, and use same at sdmoves/1100.
      // Also, use the full XPID_MASK bits, not just 3 bits.

      veryshort where_they_came_from[8];
      ::memset(where_they_came_from, -1, sizeof(where_they_came_from));
      for (i=0; i<=attr::slimit(ss); i++) {
         int j = (ss->people[i].id1 >> 6) & 7;
         if (where_they_came_from[j] != -1) {
            result->clear_all_overcasts();
            goto overcast_done;
         }
         where_they_came_from[j] = i;
      }

      int octantmask1, octantmask2;

      for (i=0; i<=attr::slimit(result); i++) {
         uint32 p = result->people[i].id1;
         if (p) {
            int place = find_casting_partner(i, result, result->people[i].id1, octantmask1);
            // Check whether both people agree that casting is taking place.  This requires that they
            // have the same roll direction and are facing opposite directions.
            if (place < 0 || ((p ^ result->people[place].id1) & (ROLL_DIRMASK | 3)) != 2) {
               result->people[i].id1 &= ~OVERCAST_BIT;
               continue;
            }

            // We now know that the two people are adjacent, in a miniwave, and rolling
            // toward each other at the end of the call.
            // Were they adjacent and in a miniwave before the call?
            // In the same general area?

            int f2 = where_they_came_from[(p >> 6) & 7];
            int f8 = where_they_came_from[(result->people[place].id1 >> 6) & 7];
            if (f2 >= 0) {
               int zz1 = find_casting_partner(f2, ss, result->people[i].id1, octantmask2);
               if (zz1 >= 0 &&
                   zz1 == f8 &&
                   octantmask1 == octantmask2 &&
                   ((ss->people[f2].id1 ^ ss->people[zz1].id1) & 3) == 2) {
                  // They were also adjacent prior to the call, and in a miniwave.
                  // So we will turn on both of their overcast bits now.  But we raise the warning
                  // only if they both had their overcast bits on before, and their roll directions
                  // matched their current roll directions.
                  if ((ss->cmd.cmd_misc3_flags & CMD_MISC3__STOP_OVERCAST_CHECK) == 0) {
                     if (ss->people[f2].id1 & ss->people[zz1].id1 & OVERCAST_BIT) {
                        // They both had the flag on before.  But for each other?  That is,
                        // are their roll directions back then same as they are now?

                        if (((p ^ ss->people[f2].id1) & ROLL_DIRMASK) == 0) {
                           // My roll dir now is same as it was.
                           if (enforce_overcast_warning &&
                               ((result->people[place].id1 ^ ss->people[f8].id1) & ROLL_DIRMASK) == 0) {
                              // Other person -- same.  Check whether the call prevents this.
                              if (!ss->cmd.callspec ||
                                  (ss->cmd.callspec->the_defn.callflagsf & CFLAG2_NO_RAISE_OVERCAST) == 0)
                                 warn(warn__overcast);
                           }
                        }
                     }
                  }
                  result->people[i].id1 |= OVERCAST_BIT;
               }
               else {
                  result->people[i].id1 &= ~OVERCAST_BIT;
               }
            }
            else {
               result->people[i].id1 &= ~OVERCAST_BIT;
            }
         }
      }
   }
   else
      return resultflagsmisc;

 overcast_done:

   return resultflagsmisc | RESULTFLAG__STOP_OVERCAST_CHECK;
}


// For this routine, we know that callspec is a real call, with an array definition schema.
// Also, result->people have been cleared.
extern void basic_move(
   setup *ss,
   calldefn *the_calldefn,
   int tbonetest,
   bool fudged,
   bool mirror,
   setup *result) THROW_DECL
{
   int j;
   callarray *calldeflist;
   uint32 funny;
   uint32 division_code = ~0U;
   callarray *linedefinition;
   callarray *coldefinition;
   uint32 matrix_check_flag = 0;
   uint32 search_concepts_without_funny,
      search_temp_without_funny, search_temp_with_funny;
   int orig_elongation = 0;
   bool four_way_startsetup;
   uint32 newtb = tbonetest;
   uint32 resultflagsmisc = 0;
   int desired_elongation = 0;
   calldef_block *qq;
   const calldefn *callspec = the_calldefn;

   bool check_peeloff_migration = false;

   // We don't allow "central" or "invert" with array-defined calls.
   // We might allow "snag" or "mystic".

   if (ss->cmd.cmd_misc2_flags & (CMD_MISC2__SAID_INVERT | CMD_MISC2__DO_CENTRAL))
      fail("Can't do \"central\" or \"invert\" with this call.");

   // Do this now, for 2 reasons:
   // (1) We want it to be zero in case we bail out.
   // (2) we want the RESULTFLAG__SPLIT_AXIS_MASK stuff to be clear
   //     for the normal case, and have bits only if splitting actually occurs.
   clear_result_flags(result);

   if (ss->cmd.cmd_misc2_flags & CMD_MISC2__DO_NOT_EXECUTE) {
      result->kind = nothing;
      return;
   }

   // We demand that the final concepts that remain be only the following ones.

   if (ss->cmd.cmd_final_flags.test_finalbits(
         ~(FINAL__SPLIT_SQUARE_APPROVED | FINAL__SPLIT_DIXIE_APPROVED |
           FINAL__TRIANGLE | FINAL__LEADTRIANGLE | FINAL__UNDER_RANDOM_META)))
      fail("This concept not allowed here.");

   /* Set up the result elongation that we will need if the result is
      a 2x2 or short6.  This comes from the original 2x2 elongation, or is set
      perpendicular to the original 1x4/diamond elongation.  If the call has the
      "parallel_conc_end" flag set, invert that elongation.

      We will override all this if we divide a 1x4 or 2x2 into 1x2's.

      What this means is that the default for a 2x2->2x2 or short6->short6 call
      is to work to "same absolute elongation".  For a 2x2 that is of course the same
      as working to footprints.  For a short6 it works to footprints if the call
      doesn't rotate, and does something a little bit weird if it does.  In either
      case the "parallel_conc_end" flag inverts that.

      The reason for putting up with this rather weird behavior is to make counter
      rotate work for 2x2 and short6 setups.  The call has the "parallel_conc_end" flag
      set, so that "ends counter rotate" will do the correct (anti-footprint) thing.
      The "parallel_conc_end" flag applies only to an entire call, not to a specific
      by-array definition, so it is set for counter rotate from a short6 also.
      Counter rotate for a short6 rotates the setup, of course, and we want it to do
      the correct shape-preserving thing, which requires that the absolute elongation
      change when "parallel_conc_end" is set.  So the default is that, in the absence
      of "parallel_conc_end", the absolute elongation stays the same, even though that
      changes the shape for a rotating short6 call. */

   switch (ss->kind) {
   case s2x2: case s2x3: case s_short6: case s_bone6:
      orig_elongation = ss->cmd.prior_elongation_bits;
      break;
   case s1x2: case s1x4: case sdmd:
      orig_elongation = 2 - (ss->rotation & 1);
      break;
   }

   desired_elongation = orig_elongation & 3;

   // Attend to a few details, but only if there are real people present.

   if (newtb) {
      switch (ss->kind) {
      case s2x2:
         // Now we do a special check for split-square-thru or
         // split-dixie-style types of things.

         if (ss->cmd.cmd_final_flags.test_finalbits(FINAL__SPLIT_SQUARE_APPROVED | FINAL__SPLIT_DIXIE_APPROVED)) {
            static uint32 startmasks[4] = {0xAD, 0xBC, 0x70, 0x61};

            ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;

            // Find out what orientation of the split call is consistent with the
            // directions of the live people.  Demand that it be unambiguous.

            uint32 directions, livemask;
            big_endian_get_directions(ss, directions, livemask);
            int i1 = -1;

            for (int i=0 ; i<4 ; i++) {
               if (((directions ^ startmasks[i]) & livemask) == 0) {
                  // Be sure it isn't ambiguous.
                  if (i1 >= 0) fail("People are not in correct position for split call.");
                  i1 = i;
               }
            }

            if (i1 < 0) fail("People are not in correct position for split call.");

            // Now do the required transformation, if it is a "split square thru" type.
            // For "split dixie style" stuff, do nothing -- the database knows what to do.

            if (!(ss->cmd.cmd_final_flags.test_finalbit(FINAL__SPLIT_DIXIE_APPROVED))) {
               int i2 = (i1 + 1) & 3;
               ss->swap_people(i1, i2);
               copy_rot(ss, i1, ss, i1, 033);
               copy_rot(ss, i2, ss, i2, 011);

               // Repair the damage.
               newtb = ss->people[0].id1 | ss->people[1].id1 | ss->people[2].id1 | ss->people[3].id1;
            }
         }
         break;
      case s_trngl4:
         // The same, with twisted.

         if (ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_TWISTED) &&
             (ss->cmd.cmd_final_flags.test_finalbits(FINAL__SPLIT_SQUARE_APPROVED | FINAL__SPLIT_DIXIE_APPROVED))) {
            ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;

            if (((ss->people[0].id1 & d_mask) != d_north) ||
                ((ss->people[1].id1 & d_mask) != d_south) ||
                ((ss->people[2].id1 & d_mask) != d_south) ||
                ((ss->people[3].id1 & d_mask) != d_south))
               fail("People are not in correct position for split call.");

            // Now do the required transformation, if it is a "split square thru" type.
            // For "split dixie style" stuff, do nothing -- the database knows what to do.

            if (!(ss->cmd.cmd_final_flags.test_finalbit(FINAL__SPLIT_DIXIE_APPROVED))) {
               copy_rot(ss, 0, ss, 0, 011);
               copy_rot(ss, 1, ss, 1, 011);
               copy_rot(ss, 2, ss, 2, 011);
               copy_rot(ss, 3, ss, 3, 033);
               ss->rotation--;
               ss->kind = s1x4;
               canonicalize_rotation(ss);

               // Repair the damage.
               newtb = ss->people[0].id1 | ss->people[1].id1 | ss->people[2].id1 | ss->people[3].id1;
            }
         }
         break;
      case s_qtag:
         if (fudged && !(callspec->callflags1 & CFLAG1_FUDGE_TO_Q_TAG))
            fail("Can't do this call from arbitrary 3x4 setup.");
         break;
      }
   }

   // Many of the remaining final concepts (all of the heritable ones
   // except "funny" and "left", but "left" has been taken care of)
   // determine what call definition we will get.

   uint32 given_funny_flag = ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_FUNNY);

   search_concepts_without_funny = ss->cmd.cmd_final_flags.test_heritbits(~INHERITFLAG_FUNNY);
   search_temp_without_funny = 0;

foobar:

   search_temp_without_funny |= search_concepts_without_funny;
   search_temp_with_funny = search_temp_without_funny | given_funny_flag;

   funny = 0;   // Will have the bit if we are supposed to use the "CAF__FACING_FUNNY" stuff.

   for (qq = callspec->stuff.arr.def_list; qq; qq = qq->next) {
      // First, see if we have a match including any incoming "funny" flag.
      if (qq->modifier_seth == search_temp_with_funny) {
         goto use_this_calldeflist;
      }
      // Search again, for a plain definition that has the "CAF__FACING_FUNNY" flag.
      else if (given_funny_flag &&
               qq->modifier_seth == search_temp_without_funny &&
               (qq->callarray_list->callarray_flags & CAF__FACING_FUNNY)) {
         funny = given_funny_flag;
         goto use_this_calldeflist;
      }
   }

   /* We didn't find anything. */

   if (matrix_check_flag != 0) goto need_to_divide;

   if (ss->cmd.cmd_misc2_flags & CMD_MISC2__CTR_END_MASK)
      fail("Can't do \"snag/mystic\" with this call.");

   // Perhaps the concept "magic" or "interlocked" was given, and the call
   // has no special definition for same, but wants us to divide the setup
   // magically or interlockedly.  Or a similar thing with 12 matrix.

   // First, check for "magic" and "interlocked" stuff, and do those divisions if so.
   result->result_flags.res_heritflags_to_save_from_mxn_expansion = 0;
   if (divide_for_magic(ss,
                        search_concepts_without_funny & ~(INHERITFLAG_DIAMOND | INHERITFLAG_SINGLE |
                                                          INHERITFLAG_SINGLEFILE | INHERITFLAG_CROSS |
                                                          INHERITFLAG_GRAND | INHERITFLAG_REVERTMASK),
                        result))
      goto un_mirror;

   // Now check for 12 matrix or 16 matrix things.  If the call has the
   // "12_16_matrix_means_split" flag, and that (plus possible 3x3/4x4 stuff)
   // was what we were looking for, remove those flags and split the setup.

   if (callspec->callflags1 & CFLAG1_12_16_MATRIX_MEANS_SPLIT) {
      uint32 z = search_concepts_without_funny & ~INHERITFLAG_NXNMASK;

      switch (ss->kind) {
      case s3x4:
         if (z == INHERITFLAG_12_MATRIX ||
             (z == 0 && (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX))) {
            // "12 matrix" was specified.  Split it into 1x4's in the appropriate way.
            division_code = MAPCODE(s1x4,3,MPKIND__SPLIT,1);
         }
         break;
      case s1x12:
         if (z == INHERITFLAG_12_MATRIX ||
             (z == 0 && (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX))) {
            // "12 matrix" was specified.  Split it into 1x4's in the appropriate way.
            division_code = MAPCODE(s1x4,3,MPKIND__SPLIT,0);
         }
         break;
      case s3dmd:
         if (z == INHERITFLAG_12_MATRIX ||
             (z == 0 && (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX))) {
            // "12 matrix" was specified.  Split it into diamonds in the appropriate way.
            division_code = MAPCODE(sdmd,3,MPKIND__SPLIT,1);

         }
         break;
      case s4dmd:
         if (z == INHERITFLAG_16_MATRIX ||
             (z == 0 && (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX))) {
            // "16 matrix" was specified.  Split it into diamonds in the appropriate way.
            division_code = MAPCODE(sdmd,4,MPKIND__SPLIT,1);
         }
         break;
      case s2x6:
         if (z == INHERITFLAG_12_MATRIX ||
             (z == 0 && (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX))) {
            // "12 matrix" was specified.  Split it into 2x2's in the appropriate way.
            division_code = MAPCODE(s2x2,3,MPKIND__SPLIT,0);
         }
         break;
      case s2x8:
         if (z == INHERITFLAG_16_MATRIX ||
             (z == 0 && (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX))) {
            // "16 matrix" was specified.  Split it into 2x2's in the appropriate way.
            division_code = MAPCODE(s2x2,4,MPKIND__SPLIT,0);
         }
         break;
      case s4x4:
         if (z == INHERITFLAG_16_MATRIX ||
             (z == 0 && (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX))) {
            /* "16 matrix" was specified.  Split it into 1x4's in the appropriate way. */
            /* But which way is appropriate?  A 4x4 is ambiguous. */
            /* Being rather lazy, we just look to see whether the call is "pass thru",
               which is the only one that wants tandems rather than couples.
               Really ought to try splitting to 2x2's and see what happens. */

            int zxy = (callspec == &base_calls[base_call_passthru]->the_defn) ? 1 : 0;

            if ((newtb ^ zxy) & 1) {
               // If the setup is empty and newtb is zero, it doesn't matter what we do.
               division_code = spcmap_4x4v;
            }
            else
               division_code = MAPCODE(s1x4,4,MPKIND__SPLIT,1);
         }
         break;
      }

      if (division_code != ~0U) {
         warn(warn__really_no_eachsetup);   // This will shut off all future "do it in each XYZ" warnings.
         goto divide_us;
      }
   }

   calldeflist = 0;

   goto try_to_find_deflist;

 divide_us:

   ss->cmd.cmd_final_flags.clear_heritbits(search_concepts_without_funny);
   divided_setup_move(ss, division_code, phantest_ok, true, result);
   goto un_mirror;

 use_this_calldeflist:

   calldeflist = qq->callarray_list;
   if (qq->modifier_level > calling_level &&
       !(ss->cmd.cmd_misc_flags & CMD_MISC__NO_CHECK_MOD_LEVEL)) {
      if (allowing_all_concepts)
         warn(warn__bad_modifier_level);
      else
         fail("Use of this modifier on this call is not allowed at this level.");
   }

   /* We now have an association list (setups ==> definition arrays) in calldeflist.
      Search it for an entry matching the setup we have, or else divide the setup
         until we find something.
      If we can't handle the starting setup, perhaps we need to look for "12 matrix" or
         "16 matrix" call definitions. */

   search_for_call_def:

   linedefinition = (callarray *) 0;
   coldefinition = (callarray *) 0;

   /* If we have to split the setup for "crazy", "central", or "splitseq", we
      specifically refrain from finding a call definition.  The same is true if
      we have "snag" or "mystic". */

   if (calldeflist &&
       !(ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK) &&
       !(ss->cmd.cmd_misc2_flags & CMD_MISC2__CTR_END_MASK)) {

      /* If we came in with a "dead concentric" or its equivalent, try to turn it
         into a real setup, depending on what the call is looking for.  If we fail
         to do so, setup_limits will be negative and an error will arise shortly. */

      if ((ss->kind == s_dead_concentric) ||
          (ss->kind == s_normal_concentric && ss->outer.skind == nothing)) {
         newtb = 0;
         for (j=0; j<=attr::klimit(ss->inner.skind); j++)
            newtb |= ss->people[j].id1;

         setup linetemp;
         setup qtagtemp;
         setup dmdtemp;
         setup_kind k = try_to_expand_dead_conc(*ss, (const call_with_name *) 0, linetemp, qtagtemp, dmdtemp);

         if (k == s1x4) {
            if ((!(newtb & 010) || assoc(b_1x8, &linetemp, calldeflist)) &&
                (!(newtb & 1) || assoc(b_8x1, &linetemp, calldeflist))) {
               *ss = linetemp;
            }
            else {
               if ((!(newtb & 010) || assoc(b_qtag, &qtagtemp, calldeflist)) &&
                   (!(newtb & 1) || assoc(b_pqtag, &qtagtemp, calldeflist))) {
                  *ss = qtagtemp;
               }
            }

            newtb = or_all_people(ss);
         }
         else if (k == s2x2) {
            *ss = linetemp;
            newtb = or_all_people(ss);
         }
      }

      begin_kind key1 = setup_attrs[ss->kind].keytab[0];
      begin_kind key2 = setup_attrs[ss->kind].keytab[1];

      four_way_startsetup = false;

      if (key1 != b_nothing && key2 != b_nothing) {
         if (key1 == key2) {     // This is for things like 2x2 or 1x1.
            linedefinition = assoc(key1, ss, calldeflist);
            coldefinition = linedefinition;
            four_way_startsetup = true;
         }
         else {
            // If the setup is empty, get whatever definitions we can get, so that
            // we can find the "CFLAG1_PARALLEL_CONC_END" bit,
            // also known as the "other_elongate" bit.

            if (!newtb || (newtb & 010)) linedefinition = assoc(key1, ss, calldeflist);
            if (!newtb || (newtb & 1)) coldefinition = assoc(key2, ss, calldeflist);

            if (ss->cmd.cmd_misc2_flags & CMD_MISC2__REQUEST_Z && ss->kind == s2x3) {
               // See if the call has a 2x3 definition that goes to a setup of size 4.
               // That is, see if this is "Z axle".  If so, turn off the special "Z" flags
               // and forget about it.  Otherwise, change to a 2x2 and try again.
               //
               // But if an actual "Z" concept has been specified (as opposed to "EACH Z"
               // or some implicit thing like "Z axle"), always turn it into a 2x2.

               bool whuzzis2 = (linedefinition && (attr::klimit(linedefinition->get_end_setup()) == 3 ||
                                                   (callspec->callflags1 & CFLAG1_PRESERVE_Z_STUFF))) ||
                  (coldefinition && (attr::klimit(coldefinition->get_end_setup()) == 3 ||
                                     (callspec->callflags1 & CFLAG1_PRESERVE_Z_STUFF)));

               if (!(ss->cmd.cmd_misc3_flags & CMD_MISC3__ACTUAL_Z_CONCEPT) && whuzzis2) {
                  ss->cmd.cmd_misc2_flags &= ~CMD_MISC2__REQUEST_Z;
               }
               else {
                  remove_z_distortion(ss);
                  if (!whuzzis2)
                     result->result_flags.misc |= RESULTFLAG__COMPRESSED_FROM_2X3;
                  newtb = or_all_people(ss);
                  linedefinition = assoc(b_2x2, ss, calldeflist);
                  coldefinition = linedefinition;
                  four_way_startsetup = true;
               }
            }
         }
      }
   }

   if (attr::slimit(ss) < 0) fail("Setup is extremely bizarre.");

   switch (ss->kind) {
   case s_short6:
   case s_bone6:
   case s_trngl:
   case s_ntrgl6cw:
   case s_ntrgl6ccw:
      break;
   default:
      if (ss->cmd.cmd_final_flags.test_finalbits(FINAL__TRIANGLE|FINAL__LEADTRIANGLE))
         fail("Triangle concept not allowed here.");
   }

   /* If we found a definition for the setup we are in, we win.
      This is true even if we only found a definition for the lines version
      of the setup and not the columns version, or vice-versa.
      If we need both and don't have both, we will lose. */

   if (linedefinition || coldefinition) {
      /* Raise a warning if the "split" concept was explicitly used but the
         call would have naturally split that way anyway. */

      /* ******* we have patched this out, because we aren't convinced that it really
         works.  How do we make it not complain on "split sidetrack" even though some
         parts of the call, like the "zig-zag", would complain?  And how do we account
         for the fact that it is observed not to raise the warning on split sidetrack
         even though we don't understand why?  By the way, uncertainty about this is what
         led us to make this a warning.  It was originally an error.  Which is correct?
         It is probably best to leave it as a warning of the "don't use in resolve" type.

      if (ss->setupflags & CMD_MISC__SAID_SPLIT) {
         switch (ss->kind) {
            case s2x2:
               if (!assoc(b_2x4, ss, calldeflist) && !assoc(b_4x2, ss, calldeflist))
                  warn(warn__excess_split);
               break;
            case s1x4:
               if (!assoc(b_1x8, ss, calldeflist) && !assoc(b_8x1, ss, calldeflist))
                  warn(warn__excess_split);
               break;
            case sdmd:
               if (!assoc(b_qtag, ss, calldeflist) && !assoc(b_pqtag, ss, calldeflist) &&
                        !assoc(b_ptpd, ss, calldeflist) && !assoc(b_pptpd, ss, calldeflist))
                  warn(warn__excess_split);
               break;
         }
      }
      */

      if (ss->cmd.cmd_final_flags.test_finalbits(FINAL__TRIANGLE|FINAL__LEADTRIANGLE))
         fail("Triangle concept not allowed here.");

      /* We got here if either linedefinition or coldefinition had something.
         If only one of them had something, but both needed to (because the setup
         was T-boned) the normal intention is that we proceed anyway, which will
         raise an error.  However, we check here for the case of a 1x2 setup
         that has one definition but not the other, and has a 1x1 definition as well.
         In that case, we split the setup.  This allows T-boned "quarter <direction>".
         The problem is that "quarter <direction>" has a 1x2 definition (for
         "quarter in") and a 1x1 definition, but no 2x1 definition.  (If it had
         a 2x1 definition, then the splitting from a 2x2 would be ambiguous.)
         So we have to fix that. */

      if (ss->kind == s1x2 && (newtb & 011) == 011 && (!linedefinition || !coldefinition))
         goto need_to_divide;

      goto do_the_call;
   }

 try_to_find_deflist:

   /* We may have something in "calldeflist" corresponding to the modifiers on this call,
      but there was nothing listed under that definition matching the starting setup. */

   /* First, see if adding a "12 matrix" or "16 matrix" modifier to the call will help.
      We need to be very careful about this.  The code below will divide, for example,
      a 2x6 into 2x3's if CMD_MISC__EXPLICIT_MATRIX is on (that is, if the caller
      said "2x6 matrix") and the call has a 2x3 definition.  This is what makes
      "2x6 matrix double play" work correctly from parallelogram columns, while
      just "double play" is not legal.  We take the position that the division of the
      2x6 into 2x3's only occurs if the caller said "2x6 matrix".  But the call
      "circulate" has a 2x3 column definition for the case with no 12matrix modifier
      (so that "center 6, circulate" will work), and a 2x6 definition if the 12matrix
      modifier is given.  We want the 12-person version of the circulate to happen
      if the caller said either "12 matrix" or "2x6 matrix".  If "2x6 matrix" was
      used, we will get here with nothing in coldefinition.  We must notice that
      doing the call search again with the "12 matrix" modifier set will get us the
      12-person call definition.

      If we didn't do this check, then a 2x6 parallelogram column, for example,
      would be split into 2x3's in the code below, if a 2x3 definition existed
      for the call.  This would mean that, if we said "2x6 matrix circulate",
      we would split the setup and do the circulate in each 2x3, which is not
      what people want.

      What actually goes on here is even more complicated now.  We want to allow the
      concept "12 matrix" to cause division of the set.  This means that we not only
      allow an explicit matrix ("2x6 matrix") to be treated as a "12 matrix", the way
      we always have, but we allow it to go the other way.  That is, under certain
      conditions we will turn a "12 matrix" to get turned into an explicit matrix
      concept.  We do this special action only when all else has failed.

      We do this only once.  Because of all the goto's that we execute as we
      try various things, there is danger of looping.  To make sure that this happens
      only once, we set "matrix_check_flag" nonzero when we do it, and only do it if
      that flag is zero.

      We don't do this if "snag" or "mystic" was given.  In those cases, we know exactly
      why there is no definition, and we need to call "divide_the_setup" to fix it. */

   if (matrix_check_flag == 0 && !(ss->cmd.cmd_misc2_flags & CMD_MISC2__CTR_END_MASK)) {
      if (!(search_concepts_without_funny & INHERITFLAG_16_MATRIX) &&
          ((ss->kind == s2x6 || ss->kind == s3x4 || ss->kind == sd3x4 ||
            ss->kind == s1x12 || ss->kind == sdeepqtg) ||
           (((callspec->callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) ||
             (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) &&
            !(search_concepts_without_funny & INHERITFLAG_16_MATRIX) &&
            (ss->kind == s2x3 || ss->kind == s1x6))))
         matrix_check_flag = INHERITFLAG_12_MATRIX;
      else if (!(search_concepts_without_funny & INHERITFLAG_12_MATRIX) &&
               ((ss->kind == s2x8 || ss->kind == s4x4 || ss->kind == s1x16) ||
                (((callspec->callflags1 & CFLAG1_SPLIT_LARGE_SETUPS) ||
                  (ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX)) &&
                 !(search_concepts_without_funny & INHERITFLAG_12_MATRIX) &&
                 (ss->kind == s2x4 || ss->kind == s1x8))))
         matrix_check_flag = INHERITFLAG_16_MATRIX;

      /* But we might not have set "matrix_check_flag" nonzero!  How are we going to
         prevent looping?  The answer is that we won't execute the goto unless we did
         set set it nonzero. */

      if (!(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) &&
          calldeflist == 0 &&
          (matrix_check_flag & ~search_concepts_without_funny) == 0) {

         /* Here is where we do the very special stuff.  We turn a "12 matrix" concept
            into an explicit matrix.  Note that we only do it if we would have lost
            anyway about 25 lines below (note that "calldeflist" is zero, and
            the search again stuff won't be executed unless CMD_MISC__EXPLICIT_MATRIX is on,
            which it isn't.)  So we turn on the CMD_MISC__EXPLICIT_MATRIX bit,
            and we turn off the INHERITFLAG_12_MATRIX or INHERITFLAG_16_MATRIX bit. */

         if (matrix_check_flag == 0) {
            /* We couldn't figure out from the setup what the matrix is,
               so we have to expand it. */

            if (!(ss->cmd.cmd_misc_flags & CMD_MISC__NO_EXPAND_MATRIX)) {
               if (search_concepts_without_funny & INHERITFLAG_12_MATRIX) {
                  do_matrix_expansion(
                     ss,
                     (ss->kind == s2x4) ? CONCPROP__NEEDK_2X6 : CONCPROP__NEEDK_TRIPLE_1X4,
                     true);

                  if (ss->kind != s2x6 && ss->kind != s3x4 && ss->kind != s1x12)
                     fail("Can't expand to a 12 matrix.");
                  matrix_check_flag = INHERITFLAG_12_MATRIX;
               }
               else if (search_concepts_without_funny & INHERITFLAG_16_MATRIX) {
                  if (ss->kind == s2x6)
                     do_matrix_expansion(ss, CONCPROP__NEEDK_2X8, true);
                  else if (ss->kind == s_alamo)
                     do_matrix_expansion(ss, CONCPROP__NEEDK_4X4, true);
                  else if (ss->kind != s2x4)
                     do_matrix_expansion(ss, CONCPROP__NEEDK_1X16, true);

                  // Take no action (and hence cause an error) if the setup was a 2x4.
                  // If someone wants to say "16 matrix 4x4 peel off" from normal columns,
                  // that person needs more help than we can give.

                  if (ss->kind != s2x8 && ss->kind != s4x4 && ss->kind != s1x16)
                     fail("Can't expand to a 16 matrix.");
                  matrix_check_flag = INHERITFLAG_16_MATRIX;
               }
            }
         }

         search_concepts_without_funny &= ~matrix_check_flag;
         ss->cmd.cmd_final_flags.clear_heritbits(matrix_check_flag);
         search_temp_without_funny = 0;
         ss->cmd.cmd_misc_flags |= CMD_MISC__EXPLICIT_MATRIX;
      }
      else {
         uint32 sc = search_concepts_without_funny & INHERITFLAG_NXNMASK;

         if (((matrix_check_flag & INHERITFLAG_12_MATRIX) &&
              (search_concepts_without_funny & INHERITFLAG_12_MATRIX) &&
              (sc == INHERITFLAGNXNK_3X3 || sc == INHERITFLAGNXNK_6X6)) ||
             ((matrix_check_flag & INHERITFLAG_16_MATRIX) &&
              (search_concepts_without_funny & INHERITFLAG_16_MATRIX) &&
              (sc == INHERITFLAGNXNK_4X4 || sc == INHERITFLAGNXNK_8X8))) {
            search_concepts_without_funny &= ~matrix_check_flag;
            ss->cmd.cmd_final_flags.clear_heritbits(matrix_check_flag);
            search_temp_without_funny = 0;
         }
         else
            search_temp_without_funny = matrix_check_flag;
      }

      if ((ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) && matrix_check_flag != 0)
         goto foobar;
   }

   // We need to divide the setup.

 need_to_divide:

   if (!newtb) {
      if (callspec->callflags1 & CFLAG1_PARALLEL_CONC_END) {
         if ((desired_elongation+1) & 2)
            desired_elongation ^= 3;
      }

      result->kind = nothing;
      goto un_mirror;
   }

   if (!calldeflist)
      fail("Can't do this call with this modifier.");

   j = divide_the_setup(ss, &newtb, calldeflist, &desired_elongation, result);
   if (j == 1)
      goto un_mirror;     // It divided and did the call.  We are done.
   else if (j == 2) goto search_for_call_def;      /* It did some necessary expansion
                                                      or transformation, but did not
                                                      do the call.  Try again. */

   // If doing an "own the anyone", we might have to fix up a bizarre unsymmetrical setup.

   if (clean_up_unsymmetrical_setup(ss))
      goto search_for_call_def;

   if (ss->kind == s_ntrgl6cw || ss->kind == s_ntrgl6ccw) {
      // If the problem was that we had a "nonisotropic triangle" setup
      // and neither the call definition nor the splitting can handle it,
      // we give a warning and fudge to a 2x3.
      // Note that the "begin_kind" fields for these setups are "nothing".
      // It is simply not possible for a call definition to specify a
      // starting setup of nonisotropic triangles.
      ss->kind = s2x3;
      warn(warn__may_be_fudgy);
      goto search_for_call_def;
   }

   /* If we get here, linedefinition and coldefinition are both zero, and we will fail. */

   /* We are about to do the call by array! */

 do_the_call:

   resultflagsmisc = do_actual_array_call(
      ss, callspec, linedefinition, coldefinition, newtb,
      funny, mirror, four_way_startsetup, orig_elongation, desired_elongation,
      check_peeloff_migration, result);

   // If the invocation of this call is "roll transparent", restore roll info
   // from before the call for those people that are marked as roll-neutral.
   fix_roll_transparency_stupidly(ss, result);

 un_mirror:

   canonicalize_rotation(result);

   if (check_peeloff_migration) {
      // Check that the resultant 1x4 is parallel to the original 2x2 elongation,
      // and that everyone stayed on their own side of the split.
      if (((orig_elongation ^ result->rotation) & 1) == 0)
         fail("People are too far apart to work with each other on this call.");

      for (int i=0; i<4; i++) {
         int z = (i-result->rotation+1) & 2;
         uint32 p = ss->people[i].id1;
         if ((((p ^ result->people[z  ].id1) & PID_MASK) != 0) &&
             (((p ^ result->people[z+1].id1) & PID_MASK) != 0))
            fail_no_retry("People are too far apart to work with each other on this call.");
      }
   }

   // We take out any elongation info that divided_setup_move may have put in
   // and override it with the correct info.

   result->result_flags.misc =
      (result->result_flags.misc & (~3)) |
      resultflagsmisc |
      (desired_elongation & 3);
}
