// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2024  William B. Ackerman.
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
   do_big_concept
   nose_move
   stable_move

and the following external variables:
   global_tbonetest
   global_livemask
   global_selectmask
   global_tboneselect
   concept_table
*/

// For sprintf.
#include <stdio.h>

#include "sd.h"

static uint32_t orig_tbonetest;
uint32_t global_tbonetest;
uint32_t global_livemask;
uint32_t global_selectmask;
uint32_t global_tboneselect;



static void do_concept_expand_some_matrix(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // We used to turn on the "FINAL__16_MATRIX" call modifier for 2x8 matrix,
   // but that makes tandem stuff not work (it doesn't like
   // call modifiers preceding it) and 4x4 stuff not work
   // (it wants the matrix expanded, but doesn't want you to say
   // "16 matrix").  So we need to let the CMD_MISC__EXPLICIT_MATRIX
   // bit control the desired effects.
   if (  ss->kind != ((setup_kind) parseptr->concept->arg1) &&
         // "16 matrix of parallel diamonds" needs to accept 4dmd or 4ptpd.
         (ss->kind != s4ptpd || ((setup_kind) parseptr->concept->arg1) != s4dmd))
      fail("Can't make the required matrix out of this.");
   ss->cmd.cmd_misc_flags |= CMD_MISC__EXPLICIT_MATRIX;
   move(ss, false, result);
}


typedef struct {
   int sizem1;
   setup_kind ikind;
   int8_t map1[8];
   int8_t map2[8];
} phan_map;

static const phan_map map_c1_phan   = {7, s2x4,   {0, 2, 7, 5, 8, 10, 15, 13},       {4, 6, 11, 9, 12, 14, 3, 1}};

static const phan_map map_diag_box  = {3, s2x2,   {2, 3, 6, 7},                      {0, 1, 4, 5}};
static const phan_map map_diag_hinged={3, nothing,{0, 1, 4, 5},                      {7, 6, 3, 2}};
static const phan_map map_diag_phan1= {3, nothing,{3, 1, 11, 9},                     {13, 15, 5, 7}};
static const phan_map map_diag_phan2= {3, nothing,{4, 6, 12, 14},                    {0, 2, 8, 10}};

static const phan_map map_pinwheel1 = {7, s2x4,   {10, 15, -1, -1,  2,  7, -1, -1},  {14,  3, -1, -1,  6, 11, -1, -1}};
static const phan_map map_pinwheel2 = {7, s2x4,   {-1, -1,  3,  1, -1, -1, 11,  9},  {-1, -1,  7,  5, -1, -1, 15, 13}};
static const phan_map map_pinwheel3 = {7, s2x4,   {12, 13, -1, -1,  4, 5, -1, -1},   {14,  3, -1, -1,  6, 11, -1, -1}};
static const phan_map map_pinwheel4 = {7, s2x4,   {-1, -1,  14,  0, -1, -1, 6,  8},  {-1, -1,  7,  5, -1, -1, 15, 13}};
static const phan_map map_pinwheel5 = {7, s2x4,   {10, 15, -1, -1,  2,  7, -1, -1},  {0, 1, -1, -1,  8, 9, -1, -1}};
static const phan_map map_pinwheel6 = {7, s2x4,   {-1, -1,  3,  1, -1, -1, 11,  9},  {-1, -1,  2, 4, -1, -1, 10, 12}};

static const phan_map map_tophat1   = {7, s2x4,   {-1, -1,  3,  1,  2,  7, -1, -1},  {-1, -1, -1, -1,  6, 11, 15, 13}};
static const phan_map map_tophat2   = {7, s2x4,   {10, 15,  3,  1, -1, -1, -1, -1},  {-1, -1,  7,  5,  6, 11, -1, -1}};
static const phan_map map_tophat3   = {7, s2x4,   {10, 15, -1, -1, -1, -1, 11,  9},  {14,  3,  7,  5, -1, -1, -1, -1}};
static const phan_map map_tophat4   = {7, s2x4,   {-1, -1, -1, -1,  2,  7, 11,  9},  {14,  3, -1, -1, -1, -1, 15, 13}};

static const phan_map map_pinwheel7 = {7, s2x4,   {0, 1, -1, -1, 6, 7, -1, -1},      {2, 5, -1, -1, 8, 11, -1, -1}};
static const phan_map map_pinwheel8 = {7, s2x4,   {-1, -1, 2, 3, -1, -1, 8, 9},      {-1, -1, 5, 7, -1, -1, 11, 1}};

static const phan_map map_o_spots   = {7, s2x4,   {10, -1, -1, 1, 2, -1, -1, 9},     {14, -1, -1, 5, 6, -1, -1, 13}};
static const phan_map map_qt_phan   = {7, s_qtag, {-1, -1, 2, 3, -1, -1, 6, 7},      {1, 4, -1, -1, 5, 0, -1, -1}};
static const phan_map map_hrg_phan  = {7, s_hrglass, {-1, -1, 2, 3, -1, -1, 6, 7},      {1, 4, -1, -1, 5, 0, -1, -1}};

static void do_concept_tandem(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // The "arg3" field of the concept descriptor contains bit fields as follows:
   // "100" bit:  this takes a selector
   // "200" bit:  this takes an implicit selector of "some"
   // "F0" field: (fractional) twosome info --
   //    0=solid all the way
   //    1=twosome all the way
   //    2=solid-frac-twosome
   //    3=twosome-frac-solid
   //    "8" bit: any twosome is actually "dynamic"
   //    "4" bit: any twosome is actually "reverse dynamic"
   // "0F" field:
   //    0=normal
   //    2=plain-gruesome
   //    3=gruesome-with-wave-assumption
   //    4 bit=this is a "melded (phantom)" thing.
   //    8 bit=this is a plain "melded" thing.

   if (ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_TWISTED))
      fail("Improper concept order.");

   // Look for things like "tandem in a 1/4 tag".
   if (parseptr->next && parseptr->next->concept->kind == concept_tandem_in_setup) {
      tandem_key master_key = (tandem_key) parseptr->concept->arg4;

      // Demand that this not be "gruesome" or "anyone are tandem" or "skew" or
      // "triangles are solid" or whatever.

      if ((master_key != tandem_key_cpls && master_key != tandem_key_tand) ||
          (parseptr->concept->arg3 & ~0x0F0) != 0)
         fail("Can do this only with \"as couples\" or \"tandem\".");

      // Find out how the matrix is to be expanded.
      uint32_t orig_bits = parseptr->next->concept->arg2;
      if (master_key == tandem_key_tand) {
         switch (orig_bits & CONCPROP__NEED_MASK) {
         case CONCPROP__NEEDK_4DMD:
            orig_bits ^= CONCPROP__NEEDK_TWINQTAG ^ CONCPROP__NEEDK_4DMD;
            break;
         case CONCPROP__NEEDK_DBLX:
            orig_bits ^= CONCPROP__NEEDK_2X8 ^ CONCPROP__NEEDK_DBLX;
            break;
         case CONCPROP__NEEDK_DEEPXWV:
            orig_bits ^= CONCPROP__NEEDK_2X6 ^ CONCPROP__NEEDK_DEEPXWV;
            break;
         case CONCPROP__NEEDK_1X16:
         case CONCPROP__NEEDK_2X8:
            orig_bits ^= CONCPROP__NEEDK_2X8 ^ CONCPROP__NEEDK_1X16;
            break;
         }
      }

      if (!(ss->cmd.cmd_misc_flags & CMD_MISC__NO_EXPAND_MATRIX))
         ss->do_matrix_expansion(orig_bits, false);

      // Put in the "VERIFY" bits stating just what type of setup to expect.
      ss->cmd.cmd_misc_flags |= CMD_MISC__PHANTOMS | parseptr->next->concept->arg1;

      // Skip the "in a whatever" concept.
      ss->cmd.parseptr = parseptr->next->next;
   }

   // The table said this concept was matrix-oblivious.  Now that we have
   // expanded for "in a whatever", we have to take action.

   // Not so!!!!   We used to do:
   //    ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;
   // Which prevented any further matrix expansion (e.g. the 2x4-4x4 that
   // happens on split phantom lines) from happening after any "phantom tandem"
   // type of call.  We no longer do that, which means that things like
   // "phantom as couples split phantom lines square the bases" are now legal.

   uint64_t mxnflags = ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_SINGLE |
                                                              INHERITFLAG_MXNMASK |
                                                              INHERITFLAG_NXNMASK);

   if (parseptr->concept->arg2 == CONCPROP__NEEDK_4DMD ||
       parseptr->concept->arg2 == CONCPROP__NEEDK_4D_4PTPD ||
       parseptr->concept->arg2 == CONCPROP__NEEDK_TWINQTAG)
      ss->cmd.cmd_misc_flags |= CMD_MISC__PHANTOMS;

   ss->cmd.cmd_misc_flags |= parseptr->concept->arg1;

   ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_SINGLE |
                                           INHERITFLAG_MXNMASK |
                                           INHERITFLAG_NXNMASK);

   if (parseptr->concept->arg3 & 0x4) {
      // Expand for "phantom tandem" etc.  First priority is a 4x4.
      ss->do_matrix_expansion(CONCPROP__NEEDK_4X4, true);
      if (ss->kind != s4x4) ss->do_matrix_expansion(CONCPROP__NEEDK_2X8, true);
      if (ss->kind != s2x8) ss->do_matrix_expansion(CONCPROP__NEEDK_1X16, true);

      ss->cmd.cmd_misc_flags |= CMD_MISC__PHANTOMS;
   }

   tandem_couples_move(
     ss,
     (parseptr->concept->arg3 & 0x100) ? parseptr->options.who :
     ((parseptr->concept->arg3 & 0x200) ? who_some_thing : who_uninit_thing),
     (parseptr->concept->arg3 & 0xF0) >> 4, // (fractional) twosome info
     parseptr->options.number_fields,
     parseptr->concept->arg3 & 0xF,          // normal/phantom/gruesome etc.
     (tandem_key) parseptr->concept->arg4,   // key
     mxnflags,
     false,
     result);
}



static void do_c1_phantom_move(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // If arg2 is nonzero, this is actually the "diagonal box" concept.

   parse_block *next_parseptr;
   final_and_herit_flags junk_concepts;
   setup the_setups[2];
   const phan_map *map_ptr = (const phan_map *) 0;

   // See if this is a "phantom tandem" (or whatever) by searching ahead,
   // skipping comments of course.  This means we must skip modifiers too,
   // so we check that there weren't any.

   junk_concepts.clear_all_herit_and_final_bits();

   next_parseptr = process_final_concepts(parseptr->next, false, &junk_concepts, true, false);

   if (!parseptr->concept->arg2 && (next_parseptr->concept->kind == concept_tandem ||
                                    next_parseptr->concept->kind == concept_frac_tandem)) {

      // Find out what kind of tandem call this is.

      uint32_t what_we_need = 0;
      uint64_t mxnflags = ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_SINGLE |
                                                                 INHERITFLAG_MXNMASK |
                                                                 INHERITFLAG_NXNMASK);

      if (junk_concepts.test_for_any_herit_or_final_bit())
         fail("Phantom couples/tandem must not have intervening concepts.");

      // "Phantom tandem" has a higher level than either "phantom" or "tandem".
      if (phantom_tandem_level > calling_level) warn_about_concept_level();

      switch (next_parseptr->concept->arg4) {
      case tandem_key_siam:
      case tandem_key_skew:
         fail("Phantom not allowed with skew or siamese.");
      case tandem_key_box:
         // We do not expand the matrix.  The caller must say
         // "2x8 matrix", or whatever, to get that effect.
         break;
      case tandem_key_diamond:
         // We do not expand the matrix.  The caller must say
         // "16 matrix or parallel diamonds", or whatever, to get that effect.
         break;
      case tandem_key_tand3:
      case tandem_key_cpls3:
      case tandem_key_siam3:
      case tandem_key_tand4:
      case tandem_key_cpls4:
      case tandem_key_siam4:
         // We do not expand the matrix.  The caller must say
         // "2x8 matrix", or whatever, to get that effect.
         break;
      default:
         // This is plain "phantom tandem", or whatever.  Expand to whatever is in
         // the "arg2" field, or to a 4x4.  The "arg2" check allows the user to say
         // "phantom as couples in a 1/4 tag".  "as couples in a 1/4 tag"
         // would have worked also.
         // But we don't do this if stuff like "1x3" came in.

         if (!mxnflags) {
            what_we_need = next_parseptr->concept->arg2;
            if (what_we_need == 0) what_we_need = ~0U;
         }

         break;
      }

      // Look for things like "tandem in a 1/4 tag".  If so, skip the expansion stuff.

      if (next_parseptr->next && next_parseptr->next->concept->kind == concept_tandem_in_setup) {
         // No matrix expand.
      }
      else if (!(ss->cmd.cmd_misc_flags & CMD_MISC__NO_EXPAND_MATRIX)) {
         if (what_we_need == ~0U) {
            // Expand for "phantom tandem" etc.  First priority is a 4x4.
            ss->do_matrix_expansion(CONCPROP__NEEDK_4X4, true);
            if (ss->kind != s4x4) ss->do_matrix_expansion(CONCPROP__NEEDK_2X8, true);
            if (ss->kind != s2x8) ss->do_matrix_expansion(CONCPROP__NEEDK_1X16, true);
         }
         else if (what_we_need != 0) {
            ss->do_matrix_expansion(what_we_need, true);
         }
      }

      ss->cmd.cmd_misc_flags |= CMD_MISC__PHANTOMS;
      ss->cmd.parseptr = next_parseptr->next;
      do_concept_tandem(ss, next_parseptr, result);
      return;
   }

   ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;   // We didn't do this before.

   if (parseptr->concept->arg2) {
      if (ss->kind != s2x4)
         fail("Need a 2x4 setup to do this concept.");
      map_ptr = &map_diag_box;
      ss->cmd.cmd_misc_flags &= ~CMD_MISC__MUST_SPLIT_MASK;
      goto use_map;
   }

   if (ss->kind == s4x4 && global_livemask == 0x6666) {
      // First, check for everyone on "O" spots.  If so, treat them as though
      // in equivalent C1 phantom spots.
      map_ptr = &map_o_spots;
   }
   else if (ss->kind == s_c1phan)
      // This is a vanilla C1 phantom setup.
      map_ptr = &map_c1_phan;
   else if (ss->kind == s_bone)
      // We allow "phantom" in a bone setup to mean two "dunlap" quarter tags.
      map_ptr = &map_qt_phan;
   else if (ss->kind == s_dhrglass)
      // Or in a dunlap hourglass.
      map_ptr = &map_hrg_phan;
   else if (ss->kind == s3x4) {

      // Check for a 3x4 occupied as a distorted "pinwheel", and treat it as phantoms.

      if (global_livemask == 04747)
         map_ptr = &map_pinwheel7;
      else if (global_livemask == 05656)
         map_ptr = &map_pinwheel8;
   }
   else if (ss->kind == s4x4) {
      setup temp;

      // Check for a 4x4 occupied as a "pinwheel" or "tophat", and treat it as phantoms.

      if (global_livemask == 0xCCCC) {
         map_ptr = &map_pinwheel1;
         goto use_map;
      }
      else if (global_livemask == 0xAAAA) {
         map_ptr = &map_pinwheel2;
         goto use_map;
      }
      else if (global_livemask == 0x7878) {
         map_ptr = &map_pinwheel3;
         goto use_map;
      }
      else if (global_livemask == 0xE1E1) {
         map_ptr = &map_pinwheel4;
         goto use_map;
      }
      else if (global_livemask == 0x8787) {
         map_ptr = &map_pinwheel5;
         goto use_map;
      }
      else if (global_livemask == 0x1E1E) {
         map_ptr = &map_pinwheel6;
         goto use_map;
      }
      else if (global_livemask == 0xA8CE) {
         map_ptr = &map_tophat1;
         goto use_map;
      }
      else if (global_livemask == 0x8CEA) {
         map_ptr = &map_tophat2;
         goto use_map;
      }
      else if (global_livemask == 0xCEA8) {
         map_ptr = &map_tophat3;
         goto use_map;
      }
      else if (global_livemask == 0xEA8C) {
         map_ptr = &map_tophat4;
         goto use_map;
      }

      // Next, check for a "phantom turn and deal" sort of thing from stairsteps.
      // Do the call in each line, them remove resulting phantoms carefully.

      if (global_livemask == 0x5C5C || global_livemask == 0xA3A3) {
         // Split into 4 vertical strips.  Flip the setup around.
         ss->rotation++;
         canonicalize_rotation(ss);
         divided_setup_move(ss, MAPCODE(s1x4,4,MPKIND__SPLIT,1), phantest_ok, true, result);
         result->rotation--;
         canonicalize_rotation(ss);
      }
      else if (global_livemask == 0xC5C5 || global_livemask == 0x3A3A) {
         // Split into 4 horizontal strips.
         divided_setup_move(ss, MAPCODE(s1x4,4,MPKIND__SPLIT,1), phantest_ok, true, result);
      }
      else
         fail("Inappropriate setup for phantom concept.");

      if (result->kind != s2x8)
         fail("This call is not appropriate for use with phantom concept.");

      temp = *result;
      result->clear_people();

      if (!(temp.people[0].id1 | temp.people[7].id1 | temp.people[8].id1 | temp.people[15].id1)) {
         copy_person(result, 0, &temp, 1);
         copy_person(result, 3, &temp, 6);
         copy_person(result, 4, &temp, 9);
         copy_person(result, 7, &temp, 14);
      }
      else if (!(temp.people[1].id1 | temp.people[6].id1 | temp.people[9].id1 | temp.people[14].id1)) {
         copy_person(result, 0, &temp, 0);
         copy_person(result, 3, &temp, 7);
         copy_person(result, 4, &temp, 8);
         copy_person(result, 7, &temp, 15);
      }
      else
         fail("This call is not appropriate for use with phantom concept.");

      if (!(temp.people[2].id1 | temp.people[5].id1 | temp.people[10].id1 | temp.people[13].id1)) {
         copy_person(result, 1, &temp, 3);
         copy_person(result, 2, &temp, 4);
         copy_person(result, 5, &temp, 11);
         copy_person(result, 6, &temp, 12);
      }
      else if (!(temp.people[3].id1 | temp.people[4].id1 | temp.people[11].id1 | temp.people[12].id1)) {
         copy_person(result, 1, &temp, 2);
         copy_person(result, 2, &temp, 5);
         copy_person(result, 5, &temp, 10);
         copy_person(result, 6, &temp, 13);
      }
      else
         fail("This call is not appropriate for use with phantom concept.");

      result->kind = s2x4;
      return;
   }

   use_map:

   if (!map_ptr)
      fail("Inappropriate setup for phantom concept.");

   setup setup1 = *ss;
   setup setup2 = *ss;

   setup1.kind = map_ptr->ikind;
   setup2.kind = map_ptr->ikind;

   setup1.rotation = ((parseptr->concept->arg2) ? 0 : ss->rotation);
   setup2.rotation = ((parseptr->concept->arg2) ? 0 : ss->rotation+1);
   setup1.eighth_rotation = 0;
   setup2.eighth_rotation = 0;
   setup1.clear_people();
   setup2.clear_people();

   gather(&setup1, ss, map_ptr->map1, map_ptr->sizem1, 0);
   gather(&setup2, ss, map_ptr->map2, map_ptr->sizem1, ((parseptr->concept->arg2) ? 0 : 033));

   normalize_setup(&setup1, simple_normalize, qtag_compress);
   normalize_setup(&setup2, simple_normalize, qtag_compress);

   setup1.cmd.cmd_misc_flags |= CMD_MISC__DISTORTED;
   setup2.cmd.cmd_misc_flags |= CMD_MISC__DISTORTED;
   move(&setup1, false, &the_setups[0]);
   move(&setup2, false, &the_setups[1]);

   if (parseptr->concept->arg2) {
      result->clear_people();
      result->result_flags = get_multiple_parallel_resultflags(the_setups, 2);

      if (the_setups[0].kind == nothing) {
         if (the_setups[1].kind == nothing) {
            result->kind = nothing;
            clear_result_flags(result);   // Do we need this?
            return;
         }
         else {
            the_setups[0] = the_setups[1];
            the_setups[0].clear_people();
         }
      }
      else if (the_setups[1].kind == nothing) {
         the_setups[1] = the_setups[0];
         the_setups[1].clear_people();
      }

      if (the_setups[0].kind == s2x2 && the_setups[1].kind == s2x2) {
         result->rotation = 0;
         result->eighth_rotation = 0;
         result->kind = s2x4;
         install_scatter(result, map_ptr->sizem1+1, map_ptr->map1, &the_setups[0], 0);
         install_scatter(result, map_ptr->sizem1+1, map_ptr->map2, &the_setups[1], 0);
      }
      else if (the_setups[0].kind == s1x4 && the_setups[1].kind == s1x4 &&
          the_setups[0].rotation == 1 && the_setups[1].rotation == 1) {
         map_ptr = &map_diag_hinged;
         result->kind = s2x4;
         result->rotation = 1;
         result->eighth_rotation = 0;
         install_scatter(result, map_ptr->sizem1+1, map_ptr->map1, &the_setups[0], 0);
         install_scatter(result, map_ptr->sizem1+1, map_ptr->map2, &the_setups[1], 0);
      }
      else if (the_setups[0].kind == s2x2 && the_setups[1].kind == s1x4 &&
          the_setups[1].rotation == 1) {
         map_ptr = &map_diag_phan1;
         result->kind = s_c1phan;
         result->rotation = 1;
         result->eighth_rotation = 0;
         install_scatter(result, map_ptr->sizem1+1, map_ptr->map1, &the_setups[0], 033);
         install_scatter(result, map_ptr->sizem1+1, map_ptr->map2, &the_setups[1], 0);
      }
      else if (the_setups[0].kind == s1x4 && the_setups[0].rotation == 1 &&
               the_setups[1].kind == s2x2) {
         map_ptr = &map_diag_phan2;
         result->kind = s_c1phan;
         result->rotation = 0;
         result->eighth_rotation = 0;
         install_scatter(result, map_ptr->sizem1+1, map_ptr->map1, &the_setups[0], 011);
         install_scatter(result, map_ptr->sizem1+1, map_ptr->map2, &the_setups[1], 0);
      }
      else {
         fail("Can't figure out result setup.");
      }

      canonicalize_rotation(result);
      reinstate_rotation(ss, result);
      return;
   }

   *result = the_setups[1];
   result->result_flags = get_multiple_parallel_resultflags(the_setups, 2);

   merge_table::merge_setups(&the_setups[0],
                             merge_c1_phantom_real,
                             result,
                             (ss->cmd.parseptr &&
                              ss->cmd.parseptr->concept &&
                              ss->cmd.parseptr->concept->kind == marker_end_of_list) ?
                             ss->cmd.parseptr->call : (call_with_name *) 0);
}



static void do_concept_single_diagonal(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   if (ss->kind != s4x4) fail("Need a 4x4 for this.");

   if ((global_livemask != 0x2D2D && global_livemask != 0xD2D2) &&
       (!two_couple_calling || (global_livemask != 0x9090 && global_livemask != 0x0909)))
      fail("People must be in blocks -- try specifying the people who should do the call.");

   selective_move(ss, parseptr, selective_key_disc_dist, 0,
                  parseptr->concept->arg1,
                  global_livemask & 0x9999, null_options.who, false, result);
}


static void do_concept_double_diagonal(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL

/* This concept is "standard", which means that it can look at global_tbonetest
   and global_livemask, but may not look at anyone's facing direction other
   than through global_tbonetest. */

{
   uint32_t tbonetest;
   uint32_t map_code;

   if (parseptr->concept->arg2 == 2) {

      // This is "distorted CLW of 6".

      setup ssave = *ss;

      if (ss->kind != sbighrgl) global_livemask = 0;   // Force error.

      if (global_livemask == 0x3CF) { map_code = spcmap_dhrgl1; }
      else if (global_livemask == 0xF3C) { map_code = spcmap_dhrgl2; }
      else fail("Can't find distorted 1x6.");

      if (parseptr->concept->arg1 == 3)
         ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;
      else if (parseptr->concept->arg1 == 1)
         ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_LINES;
      else
         ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_COLS;

      divided_setup_move(ss, map_code, phantest_ok, true, result);

      // I wish this weren't so sleazy, but a new concentricity schema seems excessive.

      if (result->kind != sbighrgl) fail("Can't figure out result setup.");

      copy_person(result, 2, &ssave, 2);
      copy_person(result, 8, &ssave, 8);
   }
   else if (parseptr->concept->arg2) {

      // This is "diagonal CLW's of 3".

      setup ssave = *ss;
      int switcher = (parseptr->concept->arg1 ^ global_tbonetest) & 1;

      if (ss->kind != s4x4 || (global_tbonetest & 011) == 011) global_livemask = 0;   // Force error.

      if (     global_livemask == 0x2D2D)
         map_code = switcher ? spcmap_diag23a : spcmap_diag23b;
      else if (global_livemask == 0xD2D2)
         map_code = switcher ? spcmap_diag23c : spcmap_diag23d;
      else
         fail("There are no diagonal lines or columns of 3 here.");

      if (parseptr->concept->arg1 == 3)
         ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

      divided_setup_move(ss, map_code, phantest_ok, true, result);

      // I wish this weren't so sleazy, but a new concentricity schema seems excessive.

      if (result->kind != s4x4) fail("Can't figure out result setup.");

      copy_person(result, 0, &ssave, 0);
      copy_person(result, 4, &ssave, 4);
      copy_person(result, 8, &ssave, 8);
      copy_person(result, 12, &ssave, 12);
   }
   else {
      tbonetest = global_tbonetest;

      if (ss->kind == s4x4) {
         if (global_livemask == 0x9999) {
            if ((parseptr->concept->arg1 ^ tbonetest) & 1) {
               map_code = MAPCODE(s1x4,2,MPKIND__NS_CROSS_IN_4X4,0);
               tbonetest = ~tbonetest;  // Trick the line/column test below, so it does the right thing.
            }
            else
               map_code = MAPCODE(s1x4,2,MPKIND__EW_CROSS_IN_4X4,0);
         }
         else
            tbonetest = ~0U;   // Force error.
      }
      else if (ss->kind == s4x6) {
         if (     global_livemask == 0x2A82A8) map_code = spcmap_diag2a;
         else if (global_livemask == 0x505505) map_code = spcmap_diag2b;
         else
            tbonetest = ~0U;   // Force error.
      }
      else
         tbonetest = ~0U;   // Force error.

      if (parseptr->concept->arg1 & 1) {
         if (tbonetest & 010) fail("There are no diagonal lines here.");
      }
      else {
         if (tbonetest & 1) fail("There are no diagonal columns here.");
      }

      if (parseptr->concept->arg1 == 3)
         ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

      divided_setup_move(ss, map_code, phantest_ok, true, result);
   }
}


static void do_concept_double_offset(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   if (ss->kind != s2x4) fail("Must have a 2x4 setup to do this concept.");

   uint32_t topmask, botmask, ctrmask;
   uint32_t directions, livemask, map_code;
   ss->big_endian_get_directions32(directions, livemask);  // Get big-endian bit-pair masks.

   if (global_selectmask == (global_livemask & 0xCC)) {
      topmask = 0xF000 & livemask;
      botmask = 0x00F0 & livemask;
      ctrmask = 0x0F0F & livemask;
      map_code = spcmap_dbloff1;
   }
   else if (global_selectmask == (global_livemask & 0x33)) {
      topmask = 0x0F00 & livemask;
      botmask = 0x000F & livemask;
      ctrmask = 0xF0F0 & livemask;
      map_code = spcmap_dbloff2;
   }
   else
      fail("The designated centers are improperly placed.");

   // Check that the concept is correctly named.

   uint32_t errmask = 0;
   uint32_t case01 = 0;
   uint32_t case23 = 0;
   if (((directions ^ 0x8822) & ctrmask) != 0 &&
       ((directions ^ 0x2288) & ctrmask) != 0)
      case01 = 1;

   if (((directions ^ 0xAA00) & ctrmask) != 0 &&
       ((directions ^ 0x00AA) & ctrmask) != 0)
      case23 = 1;

   switch (parseptr->concept->arg1) {
   case 0:
      // Double-offset quarter tag.
      errmask = (directions & ctrmask & 0x5555) |
         case01 |
         ((directions ^ 0xAAAA) & topmask) |
         (directions & botmask);
      break;
   case 1:
      // Double-offset three-quarter tag.
      errmask = (directions & ctrmask & 0x5555) |
         case01 |
         ((directions ^ 0xAAAA) & botmask) |
         (directions & topmask);
      break;
   case 2:
      // Double-offset quarter line.
      errmask = (directions & ctrmask & 0x5555) |
         case23 |
         ((directions ^ 0xAAAA) & topmask) |
         (directions & botmask);
      break;
   case 3:
      // Double-offset three-quarter line.
      errmask = (directions & ctrmask & 0x5555) |
         case23 |
         ((directions ^ 0xAAAA) & botmask) |
         (directions & topmask);
      break;
   case 4:
      // Double-offset general quarter tag.
      errmask = directions & livemask & 0x5555;
      break;
   case 5:
      // Double-offset diamonds.
      errmask = (~directions & (topmask | botmask) & 0x5555) |
         (directions & ctrmask & 0x5555);
      break;
   default:
      // Case 6 -- double-offset diamond spots -- anything goes.
      break;
   }

   if (errmask != 0)
      fail("Facing directions are incorrect for this concept.");

   divided_setup_move(ss, map_code, phantest_ok, true, result);
}


static void do_4x4_quad_working(setup *ss, int cstuff, setup *result) THROW_DECL
{
   uint32_t masks[8];

   canonicalize_rotation(ss);

   // Initially assign the centers to the upper (masks[1] or masks[2]) group.
   masks[0] = 0xF0; masks[1] = 0xF0; masks[2] = 0xFF;

   // Look at the center 8 people and put each one in the correct group.

   if (cstuff == 8) {
      masks[0] = 0xFC; masks[1] = 0xCC; masks[2] = 0xCF;   // clockwise
   }
   else if (cstuff == 9) {
      masks[0] = 0xF3; masks[1] = 0x33; masks[2] = 0x3F;   // counterclockwise
   }
   else {                        // forward/back/left/right
      if ((ss->people[10].id1 ^ cstuff) & 2) { masks[1] |= 0x01; masks[2] &= ~0x80; };
      if ((ss->people[15].id1 ^ cstuff) & 2) { masks[1] |= 0x02; masks[2] &= ~0x40; };
      if ((ss->people[3].id1  ^ cstuff) & 2) { masks[1] |= 0x04; masks[2] &= ~0x20; };
      if ((ss->people[1].id1  ^ cstuff) & 2) { masks[1] |= 0x08; masks[2] &= ~0x10; };
      if ((ss->people[9].id1  ^ cstuff) & 2) { masks[0] |= 0x01; masks[1] &= ~0x80; };
      if ((ss->people[11].id1 ^ cstuff) & 2) { masks[0] |= 0x02; masks[1] &= ~0x40; };
      if ((ss->people[7].id1  ^ cstuff) & 2) { masks[0] |= 0x04; masks[1] &= ~0x20; };
      if ((ss->people[2].id1  ^ cstuff) & 2) { masks[0] |= 0x08; masks[1] &= ~0x10; };
   }

   overlapped_setup_move(ss, MAPCODE(s2x4,3,MPKIND__OVERLAP,1), masks, result);
}




static void do_concept_multiple_lines_tog(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // This can only be standard for
   // together/apart/clockwise/counterclockwise/toward-the-center,
   // not for forward/back/left/right, because we look at
   // individual facing directions to determine which other
   // line/column the people in the center lines/columns must
   // work in.

   int rotfix = 0;
   setup_kind base_setup = s2x4;
   int base_vert = 1;
   uint32_t map_code = ~0U;
   uint32_t masks[8];

   // Arg4 = number of C/L/W.

   int cstuff = parseptr->concept->arg1;
   // cstuff =
   // forward (lines) or left (cols)     : 0
   // backward (lines) or right (cols)   : 2
   // clockwise                          : 8
   // counterclockwise                   : 9
   // together (must be end-to-end)      : 10
   // apart (must be end-to-end)         : 11
   // toward the center (quadruple only) : 12

   int linesp = parseptr->concept->arg3;

   // If this was multiple columns, we allow stepping to a wave.  This makes it
   // possible to do interesting cases of turn and weave, when one column
   // is a single 8 chain and another is a single DPT.  But if it was multiple
   // lines, we forbid it.

   if (linesp & 1)
      ss->cmd.cmd_misc_flags |= CMD_MISC__NO_STEP_TO_WAVE;

   if (linesp == 3)
      ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

   if (parseptr->concept->arg4 == 3) {
      // Triple C/L/W working.

      if (cstuff >= 10) {
         if (ss->kind != s1x12) fail("Must have a 1x12 setup for this concept.");

         if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

         if (linesp & 1) {
            if (global_tbonetest & 1) fail("There are no lines of 4 here.");
         }
         else {
            if (!(global_tbonetest & 1)) fail("There are no columns of 4 here.");
         }

         if (cstuff == 10) {     // Working together.
            masks[0] = 0xCF; masks[1] = 0xFC;
         }
         else {                 // Working apart.
            masks[0] = 0x3F; masks[1] = 0xF3;
         }

         base_setup = s1x8;
         base_vert = 0;
      }
      else {
         if (ss->kind != s3x4) fail("Must have a 3x4 setup for this concept.");

         uint32_t tbonetest = (cstuff < 8) ? ss->or_all_people() : global_tbonetest;

         if ((tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

         if (linesp & 1) {
            if (tbonetest & 1) fail("There are no lines of 4 here.");
         }
         else {
            if (!(tbonetest & 1)) fail("There are no columns of 4 here.");
         }

         // Initially assign the centers to the upper (masks[1]) group.
         masks[0] = 0xF0; masks[1] = 0xFF;

         // Look at the center line people and put each one in the correct group.

         if (cstuff == 8) {
            masks[0] = 0xFC; masks[1] = 0xCF;   // clockwise
         }
         else if (cstuff == 9) {
            masks[0] = 0xF3; masks[1] = 0x3F;   // counterclockwise
         }
         else {                        // forward/back/left/right
            if ((ss->people[10].id1 ^ cstuff) & 2) { masks[1] &= ~0x80 ; masks[0] |= 0x1; };
            if ((ss->people[11].id1 ^ cstuff) & 2) { masks[1] &= ~0x40 ; masks[0] |= 0x2; };
            if ((ss->people[5].id1  ^ cstuff) & 2) { masks[1] &= ~0x20 ; masks[0] |= 0x4; };
            if ((ss->people[4].id1  ^ cstuff) & 2) { masks[1] &= ~0x10 ; masks[0] |= 0x8; };
         }
      }
   }
   else if (parseptr->concept->arg4 == 16+3) {
      // Offset triple C/L/W working.

      if (ss->kind != s4x4) fail("Must have a 4x4 setup for this concept.");

      uint32_t livemask = (cstuff < 8) ? ss->little_endian_live_mask() : global_livemask;
      uint32_t tbonetest = (cstuff < 8) ? ss->or_all_people() : global_tbonetest;
      int map_table_key = 0;

      if ((tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

      if (!((linesp ^ tbonetest) & 1)) {
         rotfix = 1;
         ss->rotation++;
         canonicalize_rotation(ss);
         livemask = ((livemask << 4) & 0xFFFF) | (livemask >> 12);
      }

      if ((livemask & 0x3030) == 0) map_table_key |= 1;
      if ((livemask & 0x4141) == 0) map_table_key |= 2;

      static const uint32_t offs_triple_clw_working_table[4] = {
         ~0U, MAPCODE(s2x4,2,MPKIND__OVLOFS_L_HALF,0), MAPCODE(s2x4,2,MPKIND__OVLOFS_R_HALF,0), ~0U};

      map_code = offs_triple_clw_working_table[map_table_key];
      if (map_code == ~0U) fail("Can't find offset triple 1x4's.");

      // Look at the center line people and put each one in the correct group.

      // Initially assign the centers to the upper (masks[0]) group.
      masks[0] = 0xFF; masks[1] = 0xF0;

      if (cstuff == 8) {
         masks[0] = 0xCF; masks[1] = 0xFC;   // clockwise, no matter which way the offset goes.
      }
      else if (cstuff == 9) {
         masks[0] = 0x3F; masks[1] = 0xF3;   // counterclockwise, no matter which way the offset goes.
      }
      else {
         // Forward/back/left/right.  What we do depends on which way the offset goes.
         if (map_table_key == 2) {
            // Offset is "clockwise".
            if ((ss->people[10].id1 ^ cstuff) & 2) { masks[0] &= ~0x80 ; masks[1] |= 0x1; };
            if ((ss->people[15].id1 ^ cstuff) & 2) { masks[0] &= ~0x40 ; masks[1] |= 0x2; };
            if ((ss->people[7].id1  ^ cstuff) & 2) { masks[0] &= ~0x20 ; masks[1] |= 0x4; };
            if ((ss->people[2].id1  ^ cstuff) & 2) { masks[0] &= ~0x10 ; masks[1] |= 0x8; };
         }
         else {
            // Offset is "counterclockwise".
            if ((ss->people[9].id1  ^ cstuff) & 2) { masks[0] &= ~0x80 ; masks[1] |= 0x1; };
            if ((ss->people[11].id1 ^ cstuff) & 2) { masks[0] &= ~0x40 ; masks[1] |= 0x2; };
            if ((ss->people[3].id1  ^ cstuff) & 2) { masks[0] &= ~0x20 ; masks[1] |= 0x4; };
            if ((ss->people[1].id1  ^ cstuff) & 2) { masks[0] &= ~0x10 ; masks[1] |= 0x8; };
         }
      }
   }
   else if (parseptr->concept->arg4 == 4) {
      // Quadruple C/L/W working.

      if (cstuff >= 12) {
         if (ss->kind != s4x4) fail("Must have a 4x4 setup to do this concept.");

         if ((global_tbonetest & 011) == 011) fail("Sorry, can't do this from T-bone setup.");
         rotfix = (global_tbonetest ^ linesp ^ 1) & 1;
         ss->rotation += rotfix;   // Just flip the setup around and recanonicalize.
         canonicalize_rotation(ss);

         masks[0] = 0xF0; masks[1] = 0xFF; masks[2] = 0x0F;
      }
      else if (cstuff >= 10) {
         if (ss->kind != s1x16) fail("Must have a 1x16 setup for this concept.");

         if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

         if (linesp & 1) {
            if (global_tbonetest & 1) fail("There are no lines of 4 here.");
         }
         else {
            if (!(global_tbonetest & 1)) fail("There are no columns of 4 here.");
         }

         if (cstuff == 10) {    // Working together end-to-end.
            masks[0] = 0xCF; masks[1] = 0xCC; masks[2] = 0xFC;
         }
         else {                 // Working apart end-to-end.
            masks[0] = 0x3F; masks[1] = 0x33; masks[2] = 0xF3;
         }

         base_setup = s1x8;
         base_vert = 0;
      }
      else {
         int tbonetest;
         setup hpeople = *ss;
         setup vpeople = *ss;

         if (ss->kind != s4x4) fail("Must have a 4x4 setup to do this concept.");

         if (cstuff >= 8) {
            // Clockwise/counterclockwise can use "standard".
            tbonetest = global_tbonetest;
            if ((tbonetest & 011) == 011)  // But we can't have the standard people inconsistent.
               fail("Standard people are inconsistent.");
         }
         else {
            // Others can't use "standard", but can have multiple setups T-bioned to each other.
            tbonetest = 0;
            for (int i=0; i<16; i++) {
               uint32_t person = ss->people[i].id1;
               tbonetest |= person;
               if (person & 1)
                  vpeople.clear_person(i);
               else
                  hpeople.clear_person(i);
            }
         }

         if ((tbonetest & 011) == 011) {
            setup the_setups[2];

            // We now have nonempty setups in both hpeople and vpeople.

            rotfix = (linesp ^ 1) & 1;
            vpeople.rotation += rotfix;
            do_4x4_quad_working(&vpeople, cstuff, &the_setups[0]);
            the_setups[0].rotation -= rotfix;   // Flip the setup back.

            rotfix = (linesp) & 1;
            hpeople.rotation += rotfix;
            do_4x4_quad_working(&hpeople, cstuff, &the_setups[1]);
            the_setups[1].rotation -= rotfix;   // Flip the setup back.

            *result = the_setups[1];
            result->result_flags = get_multiple_parallel_resultflags(the_setups, 2);
            merge_table::merge_setups(&the_setups[0], merge_strict_matrix, result);
            warn(warn__tbonephantom);
         }
         else {
            rotfix = (tbonetest ^ linesp ^ 1) & 1;
            ss->rotation += rotfix;
            do_4x4_quad_working(ss, cstuff, result);
            result->rotation -= rotfix;   // Flip the setup back.
         }

         return;
      }
   }
   else if (parseptr->concept->arg4 == 5) {
      // Quintuple C/L/W working.

      if (ss->kind != s4x5) fail("Must have a 4x5 setup for this concept.");

      uint32_t tbonetest = (cstuff < 8) ? ss->or_all_people() : global_tbonetest;

      if ((tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

      if (linesp & 1) {
         if (!(tbonetest & 1)) fail("There are no lines of 4 here.");
      }
      else {
         if (tbonetest & 1) fail("There are no columns of 4 here.");
      }

      // Initially assign the center 3 lines to the right (masks[1]/masks[2]/masks[3]) group.
      masks[0] = 0xF0; masks[1] = 0xF0; masks[2] = 0xF0; masks[3] = 0xFF;

      // Look at the center 3 lines of people and put each one in the correct group.

      if (cstuff == 8) {
         // clockwise
         masks[0] = 0xFC; masks[1] = 0xCC; masks[2] = 0xCC; masks[3] = 0xCF;
      }
      else if (cstuff == 9) {
         // counterclockwise
         masks[0] = 0xF3; masks[1] = 0x33; masks[2] = 0x33; masks[3] = 0x3F;
      }
      else {                        // forward/back/left/right
         if ((ss->people[3].id1  + 3 + cstuff) & 2) { masks[2] |= 0x01; masks[3] &= ~0x80; };
         if ((ss->people[6].id1  + 3 + cstuff) & 2) { masks[2] |= 0x02; masks[3] &= ~0x40; };
         if ((ss->people[18].id1 + 3 + cstuff) & 2) { masks[2] |= 0x04; masks[3] &= ~0x20; };
         if ((ss->people[11].id1 + 3 + cstuff) & 2) { masks[2] |= 0x08; masks[3] &= ~0x10; };
         if ((ss->people[2].id1  + 3 + cstuff) & 2) { masks[1] |= 0x01; masks[2] &= ~0x80; };
         if ((ss->people[7].id1  + 3 + cstuff) & 2) { masks[1] |= 0x02; masks[2] &= ~0x40; };
         if ((ss->people[17].id1 + 3 + cstuff) & 2) { masks[1] |= 0x04; masks[2] &= ~0x20; };
         if ((ss->people[12].id1 + 3 + cstuff) & 2) { masks[1] |= 0x08; masks[2] &= ~0x10; };
         if ((ss->people[1].id1  + 3 + cstuff) & 2) { masks[0] |= 0x01; masks[1] &= ~0x80; };
         if ((ss->people[8].id1  + 3 + cstuff) & 2) { masks[0] |= 0x02; masks[1] &= ~0x40; };
         if ((ss->people[16].id1 + 3 + cstuff) & 2) { masks[0] |= 0x04; masks[1] &= ~0x20; };
         if ((ss->people[13].id1 + 3 + cstuff) & 2) { masks[0] |= 0x08; masks[1] &= ~0x10; };
      }
   }
   else {
      // Sextuple C/L/W working.

      // Expanding to a 4x6 is tricky.  See the extensive comments in the
      // function "triple_twin_move" in sdistort.c .

      uint32_t tbonetest = (cstuff < 8) ? ss->or_all_people() : global_tbonetest;

      if ((tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

      tbonetest ^= linesp;

      if (ss->kind == s4x4) {
         expand::expand_setup(
            ((tbonetest & 1) ? s_4x4_4x6b : s_4x4_4x6a), ss);
         tbonetest = 0;
      }

      if (ss->kind != s4x6) fail("Must have a 4x6 setup for this concept.");

      if (tbonetest & 1) {
         if (linesp) fail("Can't find the required lines.");
         else fail("Can't find the required columns.");
      }

      // Initially assign the center 3 lines to the right
      // (masks[1]/masks[2]/masks[3]/masks[4]) group.
      masks[0] = 0xF0; masks[1] = 0xF0; masks[2] = 0xF0; masks[3] = 0xF0; masks[4] = 0xFF;

      // Look at the center 4 lines of people and put each one in the correct group.

      if (cstuff == 8) {
         // clockwise
         masks[0] = 0xFC; masks[1] = 0xCC; masks[2] = 0xCC; masks[3] = 0xCC; masks[4] = 0xCF;
      }
      else if (cstuff == 9) {
         // counterclockwise
         masks[0] = 0xF3; masks[1] = 0x33; masks[2] = 0x33; masks[3] = 0x33; masks[4] = 0x3F;
      }
      else {
         // forward/back/left/right
         if ((ss->people[ 4].id1 + 3 + cstuff) & 2) { masks[3] |= 0x01; masks[4] &= ~0x80; };
         if ((ss->people[ 7].id1 + 3 + cstuff) & 2) { masks[3] |= 0x02; masks[4] &= ~0x40; };
         if ((ss->people[22].id1 + 3 + cstuff) & 2) { masks[3] |= 0x04; masks[4] &= ~0x20; };
         if ((ss->people[13].id1 + 3 + cstuff) & 2) { masks[3] |= 0x08; masks[4] &= ~0x10; };
         if ((ss->people[ 3].id1 + 3 + cstuff) & 2) { masks[2] |= 0x01; masks[3] &= ~0x80; };
         if ((ss->people[ 8].id1 + 3 + cstuff) & 2) { masks[2] |= 0x02; masks[3] &= ~0x40; };
         if ((ss->people[21].id1 + 3 + cstuff) & 2) { masks[2] |= 0x04; masks[3] &= ~0x20; };
         if ((ss->people[14].id1 + 3 + cstuff) & 2) { masks[2] |= 0x08; masks[3] &= ~0x10; };
         if ((ss->people[ 2].id1 + 3 + cstuff) & 2) { masks[1] |= 0x01; masks[2] &= ~0x80; };
         if ((ss->people[ 9].id1 + 3 + cstuff) & 2) { masks[1] |= 0x02; masks[2] &= ~0x40; };
         if ((ss->people[20].id1 + 3 + cstuff) & 2) { masks[1] |= 0x04; masks[2] &= ~0x20; };
         if ((ss->people[15].id1 + 3 + cstuff) & 2) { masks[1] |= 0x08; masks[2] &= ~0x10; };
         if ((ss->people[ 1].id1 + 3 + cstuff) & 2) { masks[0] |= 0x01; masks[1] &= ~0x80; };
         if ((ss->people[10].id1 + 3 + cstuff) & 2) { masks[0] |= 0x02; masks[1] &= ~0x40; };
         if ((ss->people[19].id1 + 3 + cstuff) & 2) { masks[0] |= 0x04; masks[1] &= ~0x20; };
         if ((ss->people[16].id1 + 3 + cstuff) & 2) { masks[0] |= 0x08; masks[1] &= ~0x10; };
      }
   }

   if (map_code == ~0U) map_code = MAPCODE(base_setup,(parseptr->concept->arg4 & 0xF)-1,MPKIND__OVERLAP,base_vert);

   overlapped_setup_move(ss,
                         map_code,
                         masks,
                         result,
                         CMD_MISC__NO_EXPAND_1);
   result->rotation -= rotfix;   // Flip the setup back.
}


static uint32_t get_standard_people(setup *ss, selector_kind who,
                                  uint32_t & tbonetest, uint32_t & stdtest)
{
   int i, j;
   tbonetest = 0;
   stdtest = 0;
   uint32_t livemask = 0;
   who_list saved_selector = current_options.who;

   current_options.who.who[0] = who;

   for (i=0, j=1; i<=attr::slimit(ss); i++, j<<=1) {
      int p = ss->people[i].id1;
      tbonetest |= p;
      if (p) {
         livemask |= j;
         if (selectp(ss, i)) stdtest |= p;
      }
   }

   current_options.who = saved_selector;
   return livemask;
}


static void do_concept_parallelogram(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   ss->cmd.cmd_misc_flags |= CMD_MISC__SAID_PG_OFFSET;

   // First, deal with "parallelogram diamonds".

   if (parseptr->concept->arg1) {
      uint32_t map_code;

      if (ss->kind == spgdmdcw)
         map_code = MAPCODE(s_qtag,1,MPKIND__OFFS_R_HALF,0);
      else if (ss->kind == spgdmdccw)
         map_code = MAPCODE(s_qtag,1,MPKIND__OFFS_L_HALF,0);
      else
         fail("Can't find parallelogram diamonds.");

      divided_setup_move(ss, map_code, phantest_ok, true, result);

      // The split-axis bits are gone.  If someone needs them, we have work to do.
      result->result_flags.clear_split_info();
      return;
   }

   // See if it is followed by "split phantom C/L/W" or "split phantom boxes",
   // in which case we do something esoteric.

   const parse_block *next_parseptr;
   final_and_herit_flags junk_concepts;
   junk_concepts.clear_all_herit_and_final_bits();

   next_parseptr = process_final_concepts(parseptr->next, false, &junk_concepts,
                                          true, false);

   const parse_block *standard_concept = (parse_block *) 0;

   // But skip over "standard"
   if (next_parseptr->concept->kind == concept_standard &&
       !junk_concepts.test_for_any_herit_or_final_bit()) {
      standard_concept = next_parseptr;
      junk_concepts.clear_all_herit_and_final_bits();
      next_parseptr = process_final_concepts(next_parseptr->next, false, &junk_concepts,
                                             true, false);
   }

   // We are only interested in a few concepts, and only if there
   // are no intervening modifiers.  Shut off the concept if there
   // are modifiers.

   concept_kind kk = next_parseptr->concept->kind;

   if (junk_concepts.test_for_any_herit_or_final_bit())
      kk = concept_comment;

   phantest_kind phancontrol = phantest_ok;
   uint32_t map_code = ~0U;
   bool is_pgram = false;
   bool no_overcast = false;

   int linesp;
   mpkind mk;

   if (ss->kind == s2x6) {
      if ((global_livemask & ~01717) == 0 && (global_livemask & ~07474) != 0)
         mk = MPKIND__OFFS_L_HALF;
      else if ((global_livemask & ~07474) == 0 && (global_livemask & ~01717) != 0)
         mk = MPKIND__OFFS_R_HALF;
      else fail("Can't find a parallelogram.");
      is_pgram = true;
   }
   else if (ss->kind == s2x5) {
      warn(warn__1_4_pgram);
      if ((global_livemask & ~0x1EF) == 0 && (global_livemask & ~0x3DE) != 0)
         mk = MPKIND__OFFS_L_ONEQ;
      else if ((global_livemask & ~0x3DE) == 0 && (global_livemask & ~0x1EF) != 0)
         mk = MPKIND__OFFS_R_ONEQ;
      else fail("Can't find a parallelogram.");
      is_pgram = true;
   }
   else if (ss->kind == s2x7) {
      warn(warn__3_4_pgram);
      if ((global_livemask & ~0x078F) == 0 && (global_livemask & ~0x3C78) != 0)
         mk = MPKIND__OFFS_L_THRQ;
      else if ((global_livemask & ~0x3C78) == 0 && (global_livemask & ~0x078F) != 0)
         mk = MPKIND__OFFS_R_THRQ;
      else fail("Can't find a parallelogram.");
      is_pgram = true;
   }
   else if (ss->kind == s2x8) {
      warn(warn__full_pgram);
      if ((global_livemask & ~0x0F0F) == 0 && (global_livemask & ~0xF0F0) != 0)
         mk = MPKIND__OFFS_L_FULL;
      else if ((global_livemask & ~0xF0F0) == 0 && (global_livemask & ~0x0F0F) != 0)
         mk = MPKIND__OFFS_R_FULL;
      else fail("Can't find a parallelogram.");
      is_pgram = true;
   }

   if (is_pgram) {
      if (kk == concept_multiple_boxes &&
          next_parseptr->concept->arg4 == 3 &&
          next_parseptr->concept->arg5 == MPKIND__SPLIT) {
         // This is "parallelogram triple boxes".  It is deprecated.
         warn(warn__deprecate_pg_3box);
         if (standard_concept) fail("Don't use \"standard\" with triple boxes.");
         goto whuzzzzz;
      }
      else if (kk == concept_do_phantom_boxes &&
               next_parseptr->concept->arg3 == MPKIND__SPLIT) {
         // This is "parallelogram split phantom boxes".
         if (standard_concept) fail("Don't use \"standard\" with split phantom boxes.");
         goto whuzzzzz;
      }
   }
   else if (ss->kind == s4x6 && kk == concept_do_phantom_2x4) {
      // See whether people fit unambiguously
      // into one parallelogram or the other.
      if ((global_livemask & 003600360) == 0 && (global_livemask & 060036003) != 0)
         mk = MPKIND__OFFS_L_HALF;
      else if ((global_livemask & 060036003) == 0 && (global_livemask & 003600360) != 0)
         mk = MPKIND__OFFS_R_HALF;
      else fail("Can't find a parallelogram.");
      warn(warn__pg_hard_to_see);
      no_overcast = true;
   }
   else if (ss->kind == s4x5 && kk == concept_do_phantom_2x4) {
      // See whether people fit unambiguously
      // into one parallelogram or the other.
      if ((global_livemask & 0x0C030) == 0 && (global_livemask & 0x80601) != 0)
         mk = MPKIND__OFFS_L_ONEQ;
      else if ((global_livemask & 0x80601) == 0 && (global_livemask & 0x0C030) != 0)
         mk = MPKIND__OFFS_R_ONEQ;
      else fail("Can't find a parallelogram.");
      warn(warn__1_4_pgram);
      warn(warn__pg_hard_to_see);
      no_overcast = true;
   }
   else
      fail("Can't do parallelogram concept from this position.");

   if (kk == concept_do_phantom_2x4 &&
       (ss->kind == s2x6 || ss->kind == s2x5 || ss->kind == s2x7 ||
        ss->kind == s4x5 || ss->kind == s4x6)) {

      no_overcast = true;
      ss->cmd.cmd_misc_flags |= CMD_MISC__PHANTOMS;

      if (ss->kind == s2x5) {
         ss->do_matrix_expansion(CONCPROP__NEEDK_4X5, false);
         if (ss->kind != s4x5) fail("Must have a 4x5 setup for this concept.");
      }
      if (ss->kind == s2x6) {
         ss->do_matrix_expansion(CONCPROP__NEEDK_4X6, false);
         if (ss->kind != s4x6) fail("Must have a 4x6 setup for this concept.");
      }

      linesp = next_parseptr->concept->arg2 & 7;

      if (standard_concept) {
         ss->cmd.cmd_misc_flags |= CMD_MISC__NO_STEP_TO_WAVE;
         uint32_t tbonetest;
         global_livemask = get_standard_people(ss, standard_concept->options.who.who[0],
                                               tbonetest, global_tbonetest);

         if (!tbonetest) {
            result->kind = nothing;
            clear_result_flags(result);
            return;
         }

         if ((tbonetest & 011) != 011)
            fail("People are not T-boned -- 'standard' is meaningless.");

         if (!global_tbonetest)
            fail("No one is standard.");
         if ((global_tbonetest & 011) == 011)
            fail("The standard people are not facing consistently.");
      }

      if (linesp & 1) {
         if (global_tbonetest & 1) fail("There are no lines of 4 here.");
      }
      else {
         if (global_tbonetest & 010) fail("There are no columns of 4 here.");
      }

      if (linesp == 3)
         ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

      // Look for "parallelogram split phantom C/L/W"
      // or "parallelogram stagger/big block/O/butterfly".

      switch (next_parseptr->concept->arg3) {
      case MPKIND__SPLIT:
         map_code = MAPCODE(s2x4,2,mk,1);
         break;
      case MPKIND__STAG:
         if (mk == MPKIND__OFFS_L_HALF)
            map_code = spcmap_lh_stag;
         else if (mk == MPKIND__OFFS_R_HALF)
            map_code = spcmap_rh_stag;
         else if (mk == MPKIND__OFFS_L_ONEQ)
            map_code = spcmap_lh_stag_1_4;
         else if (mk == MPKIND__OFFS_R_ONEQ)
            map_code = spcmap_rh_stag_1_4;
         break;
      case MPKIND__O_SPOTS:
      case MPKIND__X_SPOTS:
         if (mk == MPKIND__OFFS_L_HALF)
            map_code = spcmap_lh_ox;
         else if (mk == MPKIND__OFFS_R_HALF)
            map_code = spcmap_rh_ox;
         break;
      }

      phancontrol = (phantest_kind) next_parseptr->concept->arg1;
      ss->cmd.parseptr = next_parseptr->next;
   }
   else
      map_code = MAPCODE(s2x4,1,mk,1);   // Plain parallelogram.

   if (map_code == ~0U)
      fail("Can't do this concept with parallelogram.");

   if (no_overcast) ss->clear_all_overcasts();
   divided_setup_move(ss, map_code, phancontrol, true, result);
   if (no_overcast) result->clear_all_overcasts();

   // The split-axis bits are gone.  If someone needs them, we have work to do.
   result->result_flags.clear_split_info();
   return;

 whuzzzzz:

   ss->cmd.cmd_misc_flags |= CMD_MISC__PHANTOMS;
   ss->cmd.parseptr = next_parseptr->next;

   whuzzisthingy zis;
   zis.k = kk;
   zis.rot = 0;

   ss->clear_all_overcasts();
   divided_setup_move(ss, MAPCODE(s2x4,1,mk,1), phancontrol, true, result, 0, &zis);
   result->clear_all_overcasts();

   // The split-axis bits are gone.  If someone needs them, we have work to do.
   result->result_flags.clear_split_info();
   return;
}


static void do_concept_quad_boxes_tog(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   int cstuff;
   uint32_t masks[3];

   if (ss->kind != s2x8) fail("Must have a 2x8 setup to do this concept.");

   cstuff = parseptr->concept->arg1;
   /* cstuff =
      forward          : 0
      left             : 1
      back             : 2
      right            : 3
      together         : 6
      apart            : 7
      clockwise        : 8
      counterclockwise : 9
      toward the center : 12 */

   if (cstuff < 4) {      /* Working forward/back/right/left. */
      int tbonetest = ss->people[2].id1 |
                      ss->people[3].id1 |
                      ss->people[4].id1 |
                      ss->people[5].id1 |
                      ss->people[10].id1 |
                      ss->people[11].id1 |
                      ss->people[12].id1 |
                      ss->people[13].id1;

      if ((tbonetest & 010) && (!(cstuff & 1))) fail("Must indicate left/right.");
      if ((tbonetest & 01) && (cstuff & 1)) fail("Must indicate forward/back.");

      masks[0] = 0xC3 ; masks[1] = 0xC3; masks[2] = 0xFF;
      cstuff <<= 2;

      /* Look at the center 8 people and put each one in the correct group. */

      if (((ss->people[2].id1  + 6) ^ cstuff) & 8) { masks[0] |= 0x04; masks[1] &= ~0x01; }
      if (((ss->people[3].id1  + 6) ^ cstuff) & 8) { masks[0] |= 0x08; masks[1] &= ~0x02; }
      if (((ss->people[4].id1  + 6) ^ cstuff) & 8) { masks[1] |= 0x04; masks[2] &= ~0x01; }
      if (((ss->people[5].id1  + 6) ^ cstuff) & 8) { masks[1] |= 0x08; masks[2] &= ~0x02; }
      if (((ss->people[10].id1 + 6) ^ cstuff) & 8) { masks[1] |= 0x10; masks[2] &= ~0x40; }
      if (((ss->people[11].id1 + 6) ^ cstuff) & 8) { masks[1] |= 0x20; masks[2] &= ~0x80; }
      if (((ss->people[12].id1 + 6) ^ cstuff) & 8) { masks[0] |= 0x10; masks[1] &= ~0x40; }
      if (((ss->people[13].id1 + 6) ^ cstuff) & 8) { masks[0] |= 0x20; masks[1] &= ~0x80; }
   }
   else if (cstuff == 6) {   /* Working together. */
      masks[0] = 0xE7 ; masks[1] = 0x66; masks[2] = 0x7E;
   }
   else if (cstuff == 7) {   /* Working apart. */
      masks[0] = 0xDB ; masks[1] = 0x99; masks[2] = 0xBD;
   }
   else if (cstuff == 8) {   /* Working clockwise. */
      masks[0] = 0xF3 ; masks[1] = 0x33; masks[2] = 0x3F;
   }
   else if (cstuff == 9) {   /* Working counterclockwise. */
      masks[0] = 0xCF ; masks[1] = 0xCC; masks[2] = 0xFC;
   }
   else {                    /* Working toward the center. */
      masks[0] = 0xC3 ; masks[1] = 0xFF; masks[2] = 0x3C;
   }

   overlapped_setup_move(ss, MAPCODE(s2x4,3,MPKIND__OVERLAP,0), masks, result, CMD_MISC__NO_EXPAND_2);
}


static void do_concept_triple_diamonds_tog(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   int cstuff;
   uint32_t m1, m2;
   uint32_t masks[2];

   if (ss->kind != s3dmd) fail("Must have a triple diamond setup to do this concept.");

   cstuff = parseptr->concept->arg1;
   /* cstuff =
      together         : 0
      right            : 1
      left             : 2 */

   if (cstuff == 0) {
      if ((ss->people[1].id1 | ss->people[7].id1) & 010)
         fail("Can't tell where points of center diamond should work.");

      m1 = 0xE9; m2 = 0xBF;

      /* Look at the center diamond points and put each one in the correct group. */

      if (ss->people[1].id1 & 2) { m1 |= 0x02; m2 &= ~0x01; }
      if (ss->people[7].id1 & 2) { m1 |= 0x10; m2 &= ~0x20; }
   }
   else {      /* Working right or left. */
      if ((ss->people[1].id1 | ss->people[7].id1 | ss->people[5].id1 | ss->people[11].id1) & 001)
         fail("Can't tell where center 1/4 tag should work.");

      m1 = 0xE1; m2 = 0xFF;
      cstuff &= 2;

      /* Look at the center 1/4 tag and put each one in the correct group. */

      if ((ss->people[1].id1  ^ cstuff) & 2) { m1 |= 0x02; m2 &= ~0x01; }
      if ((ss->people[7].id1  ^ cstuff) & 2) { m1 |= 0x10; m2 &= ~0x20; }
      if ((ss->people[5].id1  ^ cstuff) & 2) { m1 |= 0x04; m2 &= ~0x80; }
      if ((ss->people[11].id1 ^ cstuff) & 2) { m1 |= 0x08; m2 &= ~0x40; }
   }

   masks[0] = m1; masks[1] = m2;
   overlapped_setup_move(ss, MAPCODE(s_qtag,2,MPKIND__OVERLAP,0), masks, result);
}


static void do_concept_quad_diamonds_tog(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   int cstuff;
   uint32_t m1, m2, m3;
   uint32_t masks[3];

   if (ss->kind != s4dmd) fail("Must have a quadruple diamond setup to do this concept.");

   cstuff = parseptr->concept->arg1;
   /* cstuff =
      together          : 0
      right             : 1
      left              : 2
      toward the center : 12 */

   if (cstuff == 12) {      /* Working toward the center. */
      m1 = 0xE1; m2 = 0xFF; m3 = 0x1E;
   }
   else if (cstuff == 0) {  /* Working together. */
      if ((ss->people[1].id1 | ss->people[2].id1 | ss->people[9].id1 | ss->people[10].id1) & 010)
         fail("Can't tell where points of center diamonds should work.");

      m1 = 0xE9; m2 = 0xA9; m3 = 0xBF;

      /* Look at the center diamond points and put each one in the correct group. */

      if (ss->people[1].id1 & 2)  { m1 |= 0x02; m2 &= ~0x01; }
      if (ss->people[2].id1 & 2)  { m2 |= 0x02; m3 &= ~0x01; }
      if (ss->people[9].id1 & 2)  { m2 |= 0x10; m3 &= ~0x20; }
      if (ss->people[10].id1 & 2) { m1 |= 0x10; m2 &= ~0x20; }
   }
   else {      /* Working right or left. */
      if ((    ss->people[1].id1 | ss->people[2].id1 | ss->people[6].id1 | ss->people[7].id1 |
               ss->people[9].id1 | ss->people[10].id1 | ss->people[14].id1 | ss->people[15].id1) & 001)
         fail("Can't tell where center 1/4 tags should work.");

      m1 = 0xE1; m2 = 0xE1; m3 = 0xFF;
      cstuff &= 2;

      /* Look at the center 1/4 tags and put each one in the correct group. */

      if ((ss->people[1].id1  ^ cstuff) & 2) { m1 |= 0x02; m2 &= ~0x01; }
      if ((ss->people[10].id1 ^ cstuff) & 2) { m1 |= 0x10; m2 &= ~0x20; }
      if ((ss->people[15].id1 ^ cstuff) & 2) { m1 |= 0x04; m2 &= ~0x80; }
      if ((ss->people[14].id1 ^ cstuff) & 2) { m1 |= 0x08; m2 &= ~0x40; }
      if ((ss->people[2].id1  ^ cstuff) & 2) { m2 |= 0x02; m3 &= ~0x01; }
      if ((ss->people[9].id1  ^ cstuff) & 2) { m2 |= 0x10; m3 &= ~0x20; }
      if ((ss->people[6].id1  ^ cstuff) & 2) { m2 |= 0x04; m3 &= ~0x80; }
      if ((ss->people[7].id1  ^ cstuff) & 2) { m2 |= 0x08; m3 &= ~0x40; }
   }

   masks[0] = m1; masks[1] = m2; masks[2] = m3;
   overlapped_setup_move(ss, MAPCODE(s_qtag,3,MPKIND__OVERLAP,0), masks, result);
}


static void do_concept_triple_boxes_tog(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   uint32_t m1, m2;

   if (ss->kind != s2x6) fail("Must have a 2x6 setup to do this concept.");

   // If doing two-couple calling, this 2x6 might have been expanded from a
   // simple 2x2.  The expansion code had to make a guess, and may have guessed wrong.
   // If the concept was together/apart/clockwise/counterclockwise, we can't figure it out,
   // and this usage is illegal.

   bool special_ambiguous_expand = ss->little_endian_live_mask() == 01414 && two_couple_calling;

   int cstuff = parseptr->concept->arg1;
   // cstuff =
   // forward          : 0
   // left             : 1
   // back             : 2
   // right            : 3
   // together         : 6
   // apart            : 7
   // clockwise        : 8
   // counterclockwise : 9

   if (cstuff < 4) {         // Working forward/back/right/left.
      int tbonetest = ss->people[2].id1 | ss->people[3].id1 |
                      ss->people[8].id1 | ss->people[9].id1;

      if ((tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

      if (((tbonetest & 010) && (!(cstuff & 1))) || ((tbonetest & 01) && (cstuff & 1))) {
         // People's facing directions are inconsistent with the way the 2x6 is oriented.
         // if we did a special_ambiguous_expand, we have to fix the expansion.
         // Otherwise, the user requested something impossible.

         if (special_ambiguous_expand) {
            expand::thing correct_2x6_orientation =
               {{-1, -1, 9, 2, -1, -1, -1, -1, 3, 8, -1, -1},
                s2x6, s2x6, 1, 0U, ~0U, false,
                warn__none, warn__none, simple_normalize,
                0};

            expand::compress_setup(correct_2x6_orientation, ss);
         }
         else {
            fail((cstuff & 1) ? "Must indicate forward/back." : "Must indicate left/right.");
         }
      }

      // Look at the center 4 people and put each one in the correct group.

      m1 = 0xC3; m2 = 0xFF;
      cstuff <<= 2;

      if (((ss->people[2].id1 + 6) ^ cstuff) & 8) { m1 |= 0x04 ; m2 &= ~0x01; };
      if (((ss->people[3].id1 + 6) ^ cstuff) & 8) { m1 |= 0x08 ; m2 &= ~0x02; };
      if (((ss->people[8].id1 + 6) ^ cstuff) & 8) { m1 |= 0x10 ; m2 &= ~0x40; };
      if (((ss->people[9].id1 + 6) ^ cstuff) & 8) { m1 |= 0x20 ; m2 &= ~0x80; };
   }
   else {
      if (special_ambiguous_expand)
         fail("This is ambiguous in two-couple calling.  Must indicate left/right/forward/back.");

      if (cstuff == 6) {        // Working together.
         m1 = 0xE7; m2 = 0x7E;
      }
      else if (cstuff == 7) {   // Working apart.
         m1 = 0xDB; m2 = 0xBD;
      }
      else if (cstuff == 8) {   // Working clockwise.
         m1 = 0xF3; m2 = 0x3F;
      }
      else {                    // Working counterclockwise.
         m1 = 0xCF; m2 = 0xFC;
      }
   }

   uint32_t masks[2];
   masks[0] = m1; masks[1] = m2;
   overlapped_setup_move(ss, MAPCODE(s2x4,2,MPKIND__OVERLAP,0), masks, result, CMD_MISC__NO_EXPAND_2);
}


static void do_concept_offset_triple_boxes_tog(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   static const uint32_t offs_triple_boxes_map_code_table[4] = {
      ~0U, MAPCODE(s2x4,2,MPKIND__OVLOFS_L_HALF,1), MAPCODE(s2x4,2,MPKIND__OVLOFS_R_HALF,1), ~0U};

   int cstuff = parseptr->concept->arg1;
   // cstuff =
   // forward          : 0
   // left             : 1
   // back             : 2
   // right            : 3
   // together         : 6
   // apart            : 7
   // clockwise        : 8
   // counterclockwise : 9

   uint32_t m0, m1;

   if (ss->kind != s2x8) fail("Must have a 2x8 setup to do this concept.");
   int map_table_key = 0;
   uint32_t map_code = ~0U;
   uint32_t livemask = ss->little_endian_live_mask();

   if ((livemask & 0xC0C0) == 0) map_table_key |= 1;
   if ((livemask & 0x0303) == 0) map_table_key |= 2;

   map_code = offs_triple_boxes_map_code_table[map_table_key];
   if (map_code == ~0U) fail("Can't find offset triple boxes.");

   // These 4 specs are independent of which way the offset goes.
   if (cstuff == 6) {        // Working together.
      m0 = 0xE7; m1 = 0x7E;
   }
   else if (cstuff == 7) {   // Working apart.
      m0 = 0xDB; m1 = 0xBD;
   }
   else if (cstuff == 8) {   // Working clockwise.
      m0 = 0xF3; m1 = 0x3F;
   }
   else if (cstuff == 9) {   // Working counterclockwise.
      m0 = 0xCF; m1 = 0xFC;
   }
   else {                    // Working forward/back/right/left.
      uint32_t A, B, C, D;

      // Need to check which way the offset goes in order to pick up
      // the logical center box.  All else is the same.
      if (map_table_key == 2) {
         A = ss->people[4].id1;
         B = ss->people[5].id1;
         C = ss->people[12].id1;
         D = ss->people[13].id1;
      }
      else {
         A = ss->people[2].id1;
         B = ss->people[3].id1;
         C = ss->people[10].id1;
         D = ss->people[11].id1;
      }

      uint32_t tbonetest = A | B | C | D;

      if ((tbonetest & 010) && (!(cstuff & 1))) fail("Must indicate left/right.");
      if ((tbonetest & 01) && (cstuff & 1)) fail("Must indicate forward/back.");

      // Look at the center 4 people and put each one in the correct group.
      // Tentatively put them all in the m1 group; will selectively remove them and put them in m0.

      m0 = 0xC3; m1 = 0xFF;
      cstuff <<= 2;
      if (((A + 6) ^ cstuff) & 8) { m0 |= 0x04 ; m1 &= ~0x01; };
      if (((B + 6) ^ cstuff) & 8) { m0 |= 0x08 ; m1 &= ~0x02; };
      if (((C + 6) ^ cstuff) & 8) { m0 |= 0x10 ; m1 &= ~0x40; };
      if (((D + 6) ^ cstuff) & 8) { m0 |= 0x20 ; m1 &= ~0x80; };
   }

   uint32_t masks[2];
   masks[0] = m0; masks[1] = m1;
   overlapped_setup_move(ss, map_code, masks, result, CMD_MISC__NO_EXPAND_2);
}


static void do_triple_formation(
   setup *ss,
   parse_block *parseptr,
   uint32_t code,
   setup *result) THROW_DECL
{
   // Concepts that use this must have:
   // arg5 = a map_kind, typically MPKIND_SPLIT.  It may be MPKIND_CONCPHAN
   //   for "concentric triple boxes".  Among other things, this will force
   //   the use of "divided_setup_move" to force spots or whatever.
   // arg3 = assumption stuff to put into misc bits:
   //
   //   CMD_MISC__VERIFY_DMD_LIKE   "diamonds" -- require diamond-like, i.e.
   //      centers in some kind of line, ends are line-like.
   //   CMD_MISC__VERIFY_1_4_TAG    "1/4 tags" -- centers in some kind of line,
   //      ends are a couple looking in (includes 1/4 line, etc.)
   //      If this isn't specific enough for you, use the "ASSUME LEFT 1/4 LINES" concept,
   //      or whatever.
   //   CMD_MISC__VERIFY_3_4_TAG    "3/4 tags" -- centers in some kind of line,
   //      ends are a couple looking out (includes 3/4 line, etc.)
   //      If this isn't specific enough for you, use the "ASSUME LEFT 3/4 LINES" concept,
   //      or whatever.
   //   CMD_MISC__VERIFY_REAL_1_4_LINE  "1/4 lines" -- centers in 2FL.
   //   CMD_MISC__VERIFY_REAL_3_4_LINE  "3/4 lines" -- centers in 2FL.
   //   CMD_MISC__VERIFY_QTAG_LIKE  "general 1/4 tags" -- all facing same orientation --
   //      centers in some kind of line, ends are column-like.
   //   0                           "diamond spots" -- any facing direction is allowed.

   ss->cmd.cmd_misc_flags |= parseptr->concept->arg3;

   // Mystic only allowed with "triple boxes", not "concentric triple boxes".

   if ((ss->cmd.cmd_misc2_flags & CMD_MISC2__CENTRAL_MYSTIC) &&
       parseptr->concept->arg5 != MPKIND__SPLIT)
      fail("Mystic not allowed with this concept.");

   // If concept was "matrix triple boxes", we can't use the elegant "concentric" method.
   if (parseptr->concept->arg5 == MPKIND__SPLIT &&
       !(ss->cmd.cmd_misc_flags & CMD_MISC__MATRIX_CONCEPT))
      concentric_move(ss, &ss->cmd, &ss->cmd, schema_in_out_triple,
                      0, DFM1_CONC_CONCENTRIC_RULES,
                      true, false, ~0U, result);
   else {
      // Gotta do this by hand.  Sigh.
      if (ss->cmd.cmd_misc2_flags & CMD_MISC2__CENTRAL_MYSTIC) {
         ss->cmd.cmd_misc2_flags |= CMD_MISC2__MYSTIFY_SPLIT;
         if (ss->cmd.cmd_misc2_flags & CMD_MISC2__INVERT_MYSTIC)
            ss->cmd.cmd_misc2_flags |= CMD_MISC2__MYSTIFY_INVERT;
         ss->cmd.cmd_misc2_flags &= ~(CMD_MISC2__CENTRAL_MYSTIC|CMD_MISC2__INVERT_MYSTIC);
      }

      divided_setup_move(ss, code, phantest_ok, true, result);
   }
}


static void do_concept_multiple_lines(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL

/* This concept is "standard", which means that it can look at global_tbonetest
   and global_livemask, but may not look at anyone's facing direction other
   than through global_tbonetest. */

{
   // Arg1 = the C/L/W indicator.  For "triple 1x4's" it has the special value 2.
   // Arg2 = the matrix that we need, with CONCPROP__NEED_ARG2_MATRIX.  It has
   //    already been taken care of.
   // Arg3 = 1 for "quadruple C/L/W of 3".  Zero otherwise.
   // Arg4 = the number of items.
   // Arg5 = stuff used by "do_triple_formation", which we might call.
   //    It is MPKIND__SPLIT.  We don't look at it here.

   uint32_t code;
   int clw_indicator = parseptr->concept->arg1;
   int rot = 0;

   if (parseptr->concept->arg4 == 4) {
      if (parseptr->concept->arg3) {
         // This is "quadruple C/L/W OF 3".
         if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

         if (ss->kind == s3x4) {
            if ((clw_indicator ^ global_tbonetest) & 1) {
               if (clw_indicator & 1) fail("There are no lines of 3 here.");
               else                   fail("There are no columns of 3 here.");
            }

            code = MAPCODE(s1x3,4,MPKIND__SPLIT,1);
         }
         else if (ss->kind == s1x12) {
            if (!((clw_indicator ^ global_tbonetest) & 1)) {
               if (clw_indicator & 1) fail("There are no lines of 3 here.");
               else                   fail("There are no columns of 3 here.");
            }

            code = MAPCODE(s1x3,4,MPKIND__SPLIT,0);
         }
         else
            fail("Must have quadruple 1x3's for this concept.");
      }
      else {
         // This is plain "quadruple C/L/W" (of 4).

         if (ss->kind == s4x4) {
            rot = (global_tbonetest ^ clw_indicator ^ 1) & 1;
            if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

            ss->rotation += rot;   // Just flip the setup around and recanonicalize.
            canonicalize_rotation(ss);
            code = MAPCODE(s1x4,4,MPKIND__SPLIT,1);
         }
         else if (ss->kind == s1x16) {
            if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

            if (!((clw_indicator ^ global_tbonetest) & 1)) {
               if (clw_indicator & 1) fail("There are no lines of 4 here.");
               else                   fail("There are no columns of 4 here.");
            }

            code = MAPCODE(s1x4,4,MPKIND__SPLIT,0);
         }
         else if (ss->kind == sbigbigh)
            code = HETERO_MAPCODE(s1x4,4,MPKIND__HET_SPLIT,1,s1x4,0x1);
         else if (ss->kind == sbigbigx)
            code = HETERO_MAPCODE(s1x4,4,MPKIND__HET_SPLIT,0,s1x4,0x4);
         else
            fail("Must have quadruple 1x4's for this concept.");
      }
   }
   else if (parseptr->concept->arg4 == 3) {
      if (ss->kind == s1x6 || ss->kind == s1x8 || ss->kind == s1x10)
         ss->do_matrix_expansion(CONCPROP__NEEDK_1X12, false);

      if (ss->kind == s3x4)
         code = MAPCODE(s1x4,3,MPKIND__SPLIT,1);
      else if (ss->kind == s1x12)
         code = MAPCODE(s1x4,3,MPKIND__SPLIT,0);
      else if (ss->kind == sbigh)
         code = HETERO_MAPCODE(s1x4,3,MPKIND__HET_SPLIT,1,s1x4,0x1);
      else if (ss->kind == sbigx)
         code = HETERO_MAPCODE(s1x4,3,MPKIND__HET_SPLIT,0,s1x4,0x4);
      else
         fail("Must have triple 1x4's for this concept.");

      // Clw_indicator = 2 is the special case of "triple 1x4's".
      if (clw_indicator != 2 && ss->kind != sbigh && ss->kind != sbigx) {
         if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

         if (!((clw_indicator ^ global_tbonetest) & 1)) {
            if (clw_indicator & 1) fail("There are no lines of 4 here.");
            else                   fail("There are no columns of 4 here.");
         }
      }
   }
   else {
      if (parseptr->concept->arg4 == 5) {
         if (ss->kind == s4x5)
            code = MAPCODE(s1x4,5,MPKIND__SPLIT,1);
         else
            fail("Must have quintuple 1x4's for this concept.");
      }
      else {
         if (ss->kind == s4x6)
            code = MAPCODE(s1x4,6,MPKIND__SPLIT,1);
         else
            fail("Must have sextuple 1x4's for this concept.");
      }

      // Clw_indicator = 2 is the special case of "quintuple 1x4's".
      if (clw_indicator != 2) {
         if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

         if ((clw_indicator ^ global_tbonetest) & 1) {
            if (clw_indicator & 1) fail("There are no lines of 4 here.");
            else                   fail("There are no columns of 4 here.");
         }
      }
   }

   // If this was multiple columns, we allow stepping to a wave.  This makes it
   // possible to do interesting cases of turn and weave, when one column
   // is a single 8 chain and another is a single DPT.  But if it was multiple
   // lines, we forbid it.

   if (clw_indicator & 1)
      ss->cmd.cmd_misc_flags |= CMD_MISC__NO_STEP_TO_WAVE;

   if (clw_indicator == 3)
      ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

   // Sigh....  This clearly isn't the right way to do this.
   // How about CMD_MISC__VERIFY_LINES?

   if (parseptr->concept->arg4 == 3) {
      do_triple_formation(ss, parseptr, code, result);
   }
   else {
      divided_setup_move(ss, code, phantest_ok, true, result);
      result->rotation -= rot;   // Flip the setup back.
  }
}




static void do_concept_triple_1x8_tog(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   uint32_t m1, m2;
   uint32_t masks[2];

   int cstuff = parseptr->concept->arg1;
   /* cstuff =
      forward (lines) or left (cols)   : 0
      backward (lines) or right (cols) : 2 */

   int linesp = parseptr->concept->arg2;

   if (linesp & 1)
      ss->cmd.cmd_misc_flags |= CMD_MISC__NO_STEP_TO_WAVE;

   if (linesp == 3)
      ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

   if (ss->kind != s3x8) fail("Must have a 3x8 setup for this concept.");

   uint32_t tbonetest = ss->or_all_people();

   if ((tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

   if (linesp & 1) {
      if (tbonetest & 1) fail("There are no lines of 8 here.");
   }
   else {
      if (!(tbonetest & 1)) fail("There are no columns of 8 here.");
   }

   /* Initially assign the centers to the upper (m2) group. */
   m1 = 0xFF00; m2 = 0xFFFF;

   /* Look at the center line people and put each one in the correct group. */

   if ((ss->people[20].id1 ^ cstuff) & 2) { m2 &= ~0x8000 ; m1 |= 0x1; };
   if ((ss->people[21].id1 ^ cstuff) & 2) { m2 &= ~0x4000 ; m1 |= 0x2; };
   if ((ss->people[22].id1 ^ cstuff) & 2) { m2 &= ~0x2000 ; m1 |= 0x4; };
   if ((ss->people[23].id1 ^ cstuff) & 2) { m2 &= ~0x1000 ; m1 |= 0x8; };
   if ((ss->people[11].id1 ^ cstuff) & 2) { m2 &= ~0x800  ; m1 |= 0x10; };
   if ((ss->people[10].id1 ^ cstuff) & 2) { m2 &= ~0x400  ; m1 |= 0x20; };
   if ((ss->people[9].id1  ^ cstuff) & 2) { m2 &= ~0x200  ; m1 |= 0x40; };
   if ((ss->people[8].id1  ^ cstuff) & 2) { m2 &= ~0x100  ; m1 |= 0x80; };

   masks[0] = m1; masks[1] = m2;
   overlapped_setup_move(ss, MAPCODE(s2x8,2,MPKIND__OVERLAP,1),
                         masks, result);
}


static void do_concept_triple_diag(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL

/* This concept is "standard", which means that it can look at global_tbonetest
   and global_livemask, but may not look at anyone's facing direction other
   than through global_tbonetest. */

{
   int cstuff = parseptr->concept->arg1;
   int q;

   if (ss->kind != s_bigblob) fail("Must have a rather large setup for this concept.");

   if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

   if (cstuff == 3)
      ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

   if ((global_livemask & ~0x56A56A) == 0) q = 0;
   else if ((global_livemask & ~0xA95A95) == 0) q = 2;
   else fail("Can't identify triple diagonal setup.");

   static specmapkind maps_3diag[4]   = {spcmap_blob_1x4c, spcmap_blob_1x4a,
                                         spcmap_blob_1x4d, spcmap_blob_1x4b};

   divided_setup_move(ss, maps_3diag[q + ((cstuff ^ global_tbonetest) & 1)],
                      phantest_ok, true, result);
}


static void do_concept_triple_diag_tog(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   int q;
   uint32_t m1, m2;
   uint32_t masks[2];

   int cstuff = parseptr->concept->arg1;
   /* cstuff =
      forward  : 0
      left     : 1
      back     : 2
      right    : 3
      together : 8 (doesn't really exist)
      apart    : 9 (doesn't really exist) */

   if (ss->kind != s_bigblob) fail("Must have a rather large setup for this concept.");

   if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

   if (parseptr->concept->arg2 == 3)
      ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

   /* Initially assign the centers to the right or upper (m2) group. */
   m1 = 0xF0; m2 = 0xFF;

   if ((global_livemask & ~0x56A56A) == 0) q = 0;
   else if ((global_livemask & ~0xA95A95) == 0) q = 2;
   else fail("Can't identify triple diagonal setup.");

   // Look at the center line/column people and put each one in the correct group.

   static specmapkind maps_3diagwk[4] = {spcmap_wblob_1x4a, spcmap_wblob_1x4c,
                                         spcmap_wblob_1x4b, spcmap_wblob_1x4d};

   uint32_t map_code = maps_3diagwk[q+((cstuff ^ global_tbonetest) & 1)];
   const map::map_thing *map_ptr = map::get_map_from_code(map_code);

   if ((cstuff + 1 - ss->people[map_ptr->maps[0]].id1) & 2) { m2 &= ~0x80 ; m1 |= 0x1; };
   if ((cstuff + 1 - ss->people[map_ptr->maps[1]].id1) & 2) { m2 &= ~0x40 ; m1 |= 0x2; };
   if ((cstuff + 1 - ss->people[map_ptr->maps[2]].id1) & 2) { m2 &= ~0x20 ; m1 |= 0x4; };
   if ((cstuff + 1 - ss->people[map_ptr->maps[3]].id1) & 2) { m2 &= ~0x10 ; m1 |= 0x8; };

   masks[0] = m1; masks[1] = m2;
   overlapped_setup_move(ss, map_code, masks, result);
}


static void do_concept_grand_working(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   int cstuff;
   uint32_t tbonetest;
   uint32_t m0(0), m1(0), m2(0), m3(0), m4(0);
   uint32_t masks[8];
   setup_kind kk;
   int arity = 2;

   cstuff = parseptr->concept->arg1;
   /* cstuff =
      forward          : 0
      left             : 1
      back             : 2
      right            : 3
      clockwise        : 8
      counterclockwise : 9
      as centers       : 10
      as ends          : 11 */

   if (ss->kind == s2x4 &&
       ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_12_MATRIX))
      ss->do_matrix_expansion(CONCPROP__NEEDK_2X6, false);
   else if ((ss->kind == s2x4 || ss->kind == s2x6) &&
            ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_16_MATRIX))
      ss->do_matrix_expansion(CONCPROP__NEEDK_2X8, false);

   if (ss->kind == s2x4) {
      if (cstuff < 4) {      /* Working forward/back/right/left. */
         tbonetest =
            ss->people[1].id1 | ss->people[2].id1 |
            ss->people[5].id1 | ss->people[6].id1;
         if ((tbonetest & 010) && (!(cstuff & 1))) fail("Must indicate left/right.");
         if ((tbonetest & 001) && (cstuff & 1)) fail("Must indicate forward/back.");

         /* Look at the center 4 people and put each one in the correct group. */

         m0 = 0x9; m1 = 0x9; m2 = 0xF;
         cstuff <<= 2;

         if (((ss->people[1].id1 + 6) ^ cstuff) & 8) { m0 |= 0x2 ; m1 &= ~0x1; };
         if (((ss->people[6].id1 + 6) ^ cstuff) & 8) { m0 |= 0x4 ; m1 &= ~0x8; };
         if (((ss->people[2].id1 + 6) ^ cstuff) & 8) { m1 |= 0x2 ; m2 &= ~0x1; };
         if (((ss->people[5].id1 + 6) ^ cstuff) & 8) { m1 |= 0x4 ; m2 &= ~0x8; };
      }
      else if (cstuff < 10) {      /* Working clockwise/counterclockwise. */
         /* Put each of the center 4 people in the correct group, no need to look. */

         if (cstuff & 1) {
            m0 = 0xB; m1 = 0xA; m2 = 0xE;
         }
         else {
            m0 = 0xD; m1 = 0x5; m2 = 0x7;
         }
      }
      else        /* Working as-ends or as-centers. */
         fail("May not specify as-ends/as-centers here.");

      kk = s2x2;
   }
   else if (ss->kind == s2x6 &&
            ((ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) ||
             (ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_12_MATRIX)))) {

      ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_12_MATRIX);

      if (cstuff < 4) {      // Working forward/back/right/left.
         tbonetest =
            ss->people[1].id1 | ss->people[2].id1 |
            ss->people[3].id1 | ss->people[4].id1 |
            ss->people[7].id1 | ss->people[8].id1 |
            ss->people[9].id1 | ss->people[10].id1;
         if ((tbonetest & 010) && (!(cstuff & 1))) fail("Must indicate left/right.");
         if ((tbonetest & 001) && (cstuff & 1)) fail("Must indicate forward/back.");

         /* Look at the center 8 people and put each one in the correct group. */

         m0 = m1 = m2 = m3 = 0x9; m4 = 0xF;
         cstuff <<= 2;

         if (((ss->people[1].id1  + 6) ^ cstuff) & 8) { m0 |= 0x2 ; m1 &= ~0x1; };
         if (((ss->people[10].id1 + 6) ^ cstuff) & 8) { m0 |= 0x4 ; m1 &= ~0x8; };
         if (((ss->people[2].id1  + 6) ^ cstuff) & 8) { m1 |= 0x2 ; m2 &= ~0x1; };
         if (((ss->people[9].id1  + 6) ^ cstuff) & 8) { m1 |= 0x4 ; m2 &= ~0x8; };
         if (((ss->people[3].id1  + 6) ^ cstuff) & 8) { m2 |= 0x2 ; m3 &= ~0x1; };
         if (((ss->people[8].id1  + 6) ^ cstuff) & 8) { m2 |= 0x4 ; m3 &= ~0x8; };
         if (((ss->people[4].id1  + 6) ^ cstuff) & 8) { m3 |= 0x2 ; m4 &= ~0x1; };
         if (((ss->people[7].id1  + 6) ^ cstuff) & 8) { m3 |= 0x4 ; m4 &= ~0x8; };
      }
      else if (cstuff < 10) {      /* Working clockwise/counterclockwise. */
         /* Put each of the center 4 people in the correct group, no need to look. */

         if (cstuff & 1) {
            m0 = 0xB; m1 = m2 = m3 = 0xA; m4 = 0xE;
         }
         else {
            m0 = 0xD; m1 = m2 = m3 = 0x5; m4 = 0x7;
         }
      }
      else        /* Working as-ends or as-centers. */
         fail("May not specify as-ends/as-centers here.");

      masks[3] = m3; masks[4] = m4;
      kk = s2x2;
      arity = 4;
   }
   else if (ss->kind == s2x8 &&
            ((ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) ||
             (ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_16_MATRIX)))) {
      uint32_t m5, m6;

      ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_16_MATRIX);

      if (cstuff < 4) {      // Working forward/back/right/left.
         tbonetest =
            ss->people[1].id1 | ss->people[2].id1 |
            ss->people[3].id1 | ss->people[4].id1 |
            ss->people[5].id1 | ss->people[6].id1 |
            ss->people[9].id1 | ss->people[10].id1 |
            ss->people[11].id1 | ss->people[12].id1 |
            ss->people[13].id1 | ss->people[14].id1;
         if ((tbonetest & 010) && (!(cstuff & 1))) fail("Must indicate left/right.");
         if ((tbonetest & 001) && (cstuff & 1)) fail("Must indicate forward/back.");

         /* Look at the center 8 people and put each one in the correct group. */

         m0 = m1 = m2 = m3 = m4 = m5 = 0x9; m6 = 0xF;
         cstuff <<= 2;

         if (((ss->people[1].id1  + 6) ^ cstuff) & 8) { m0 |= 0x2 ; m1 &= ~0x1; };
         if (((ss->people[14].id1 + 6) ^ cstuff) & 8) { m0 |= 0x4 ; m1 &= ~0x8; };
         if (((ss->people[2].id1  + 6) ^ cstuff) & 8) { m1 |= 0x2 ; m2 &= ~0x1; };
         if (((ss->people[13].id1 + 6) ^ cstuff) & 8) { m1 |= 0x4 ; m2 &= ~0x8; };
         if (((ss->people[3].id1  + 6) ^ cstuff) & 8) { m2 |= 0x2 ; m3 &= ~0x1; };
         if (((ss->people[12].id1 + 6) ^ cstuff) & 8) { m2 |= 0x4 ; m3 &= ~0x8; };
         if (((ss->people[4].id1  + 6) ^ cstuff) & 8) { m3 |= 0x2 ; m4 &= ~0x1; };
         if (((ss->people[11].id1 + 6) ^ cstuff) & 8) { m3 |= 0x4 ; m4 &= ~0x8; };
         if (((ss->people[5].id1  + 6) ^ cstuff) & 8) { m4 |= 0x2 ; m5 &= ~0x1; };
         if (((ss->people[10].id1 + 6) ^ cstuff) & 8) { m4 |= 0x4 ; m5 &= ~0x8; };
         if (((ss->people[6].id1  + 6) ^ cstuff) & 8) { m5 |= 0x2 ; m6 &= ~0x1; };
         if (((ss->people[9].id1  + 6) ^ cstuff) & 8) { m5 |= 0x4 ; m6 &= ~0x8; };
      }
      else if (cstuff < 10) {      /* Working clockwise/counterclockwise. */
         /* Put each of the center 4 people in the correct group, no need to look. */

         if (cstuff & 1) {
            m0 = 0xB; m1 = m2 = m3 = m4 = m5 = 0xA; m6 = 0xE;
         }
         else {
            m0 = 0xD; m1 = m2 = m3 = m4 = m5 = 0x5; m6 = 0x7;
         }
      }
      else        /* Working as-ends or as-centers. */
         fail("May not specify as-ends/as-centers here.");

      masks[3] = m3; masks[4] = m4; masks[5] = m5; masks[6] = m6;
      kk = s2x2;
      arity = 6;
   }
   else if (ss->kind == s2x3) {
      if (cstuff < 4) {      /* Working forward/back/right/left. */
         tbonetest = ss->people[1].id1 | ss->people[4].id1;
         if ((tbonetest & 010) && (!(cstuff & 1))) fail("Must indicate left/right.");
         if ((tbonetest & 001) && (cstuff & 1)) fail("Must indicate forward/back.");

         /* Look at the center 2 people and put each one in the correct group. */

         m0 = 0x9; m1 = 0xF;
         cstuff <<= 2;

         if (((ss->people[1].id1 + 6) ^ cstuff) & 8) { m0 |= 0x2 ; m1 &= ~0x1; };
         if (((ss->people[4].id1 + 6) ^ cstuff) & 8) { m0 |= 0x4 ; m1 &= ~0x8; };
      }
      else if (cstuff < 10) {      /* Working clockwise/counterclockwise. */
         m2 = 0;
         if (cstuff & 1) {
            m0 = 0xB; m1 = 0xE;
         }
         else {
            m0 = 0xD; m1 = 0x7;
         }
      }
      else        /* Working as-ends or as-centers. */
         fail("May not specify as-ends/as-centers here.");

      kk = s2x2;
      arity = 1;
   }
   else if (ss->kind == s2x5) {
      /* **** Should actually put in test for explicit matrix or "10 matrix",
         but don't have the latter. */
      if (cstuff < 4) {      /* Working forward/back/right/left. */
         tbonetest =
            ss->people[2].id1 | ss->people[7].id1;
         if ((tbonetest & 010) && (!(cstuff & 1))) fail("Must indicate left/right.");
         if ((tbonetest & 001) && (cstuff & 1)) fail("Must indicate forward/back.");

         /* Look at the center 6 people and put each one in the correct group. */

         m0 = m1 = m2 = 0x9; m3 = 0xF;
         cstuff <<= 2;

         if (((ss->people[1].id1 + 6) ^ cstuff) & 8) { m0 |= 0x2 ; m1 &= ~0x1; };
         if (((ss->people[8].id1 + 6) ^ cstuff) & 8) { m0 |= 0x4 ; m1 &= ~0x8; };
         if (((ss->people[2].id1 + 6) ^ cstuff) & 8) { m1 |= 0x2 ; m2 &= ~0x1; };
         if (((ss->people[7].id1 + 6) ^ cstuff) & 8) { m1 |= 0x4 ; m2 &= ~0x8; };
         if (((ss->people[3].id1 + 6) ^ cstuff) & 8) { m2 |= 0x2 ; m3 &= ~0x1; };
         if (((ss->people[6].id1 + 6) ^ cstuff) & 8) { m2 |= 0x4 ; m3 &= ~0x8; };
      }
      else if (cstuff < 10) {      /* Working clockwise/counterclockwise. */
         /* Put each of the center 4 people in the correct group, no need to look. */

         if (cstuff & 1) {
            m0 = 0xB; m1 = m2 = 0xA; m3 = 0xE;
         }
         else {
            m0 = 0xD; m1 = m2 = 0x5; m3 = 0x7;
         }
      }
      else        /* Working as-ends or as-centers. */
         fail("May not specify as-ends/as-centers here.");

      masks[3] = m3;
      kk = s2x2;
      arity = 3;
   }
   else if (ss->kind == s1x8) {
      if (cstuff < 4) {      /* Working forward/back/right/left. */
         tbonetest = ss->people[2].id1 | ss->people[3].id1 | ss->people[6].id1 | ss->people[7].id1;
         if ((tbonetest & 010) && (!(cstuff & 1))) fail("Must indicate left/right.");
         if ((tbonetest & 001) && (cstuff & 1)) fail("Must indicate forward/back.");

         /* Look at the center 4 people and put each one in the correct group. */

         m0 = 0x3; m1 = 0x3; m2 = 0xF;
         cstuff <<= 2;

         if (((ss->people[2].id1 + 6) ^ cstuff) & 8) { m0 |= 0x4 ; m1 &= ~0x2; };
         if (((ss->people[3].id1 + 6) ^ cstuff) & 8) { m0 |= 0x8 ; m1 &= ~0x1; };
         if (((ss->people[7].id1 + 6) ^ cstuff) & 8) { m1 |= 0x4 ; m2 &= ~0x2; };
         if (((ss->people[6].id1 + 6) ^ cstuff) & 8) { m1 |= 0x8 ; m2 &= ~0x1; };
      }
      else if (cstuff < 10) {      /* Working clockwise/counterclockwise. */
         fail("Must have a 2x3 or 2x4 setup for this concept.");
      }
      else {      /* Working as-ends or as-centers. */
         /* Put each of the center 4 people in the correct group, no need to look. */

         if (cstuff & 1) {
            m0 = 0x7; m1 = 0x5; m2 = 0xD;
         }
         else {
            m0 = 0xB; m1 = 0xA; m2 = 0xE;
         }
      }

      kk = s1x4;
   }
   else if (ss->kind == s1x6) {
      if (cstuff < 4) {      /* Working forward/back/right/left. */
         tbonetest = ss->people[2].id1 | ss->people[5].id1;
         if ((tbonetest & 010) && (!(cstuff & 1))) fail("Must indicate left/right.");
         if ((tbonetest & 001) && (cstuff & 1)) fail("Must indicate forward/back.");

         /* Look at the center 2 people and put each one in the correct group. */

         m0 = 0x3; m1 = 0xF; m2 = 0;
         cstuff <<= 2;

         if (((ss->people[2].id1 + 6) ^ cstuff) & 8) { m0 |= 0x8 ; m1 &= ~0x1; };
         if (((ss->people[5].id1 + 6) ^ cstuff) & 8) { m0 |= 0x4 ; m1 &= ~0x2; };
      }
      else if (cstuff < 10) {      /* Working clockwise/counterclockwise. */
         fail("Must have a 2x3 or 2x4 setup for this concept.");
      }
      else {      /* Working as-ends or as-centers. */
         m2 = 0;
         if (cstuff & 1) {
            m0 = 0x7; m1 = 0xD;
         }
         else {
            m0 = 0xB; m1 = 0xE;
         }
      }

      arity = 1;
      kk = s1x4;
   }
   else
      fail("Must have a 2x3, 2x4, 1x6, or 1x8 setup for this concept.");

   masks[0] = m0; masks[1] = m1; masks[2] = m2;
   overlapped_setup_move(ss, MAPCODE(kk,arity+1,MPKIND__OVERLAP,0), masks, result);
}


static void do_concept_do_phantom_2x2(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // Do "blocks" or "4 phantom interlocked blocks", etc.

   if (ss->kind != s4x4) fail("Must have a 4x4 setup for this concept.");
   divided_setup_move(ss, parseptr->concept->arg1,
                      (phantest_kind) parseptr->concept->arg2,
                      true, result);
}


static void do_concept_do_triangular_boxes(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   uint32_t division_map_code = ~0U;
   phantest_kind phant = (phantest_kind) parseptr->concept->arg1;

   if (ss->kind == sbigdmd) {
      ss->do_matrix_expansion(CONCPROP__NEEDK_4X5, true);
      // Need to compute the livemask again.
      int i, j;
      global_livemask = 0;
      for (i=0, j=1; i<=attr::slimit(ss); i++, j<<=1) {
         if (ss->people[i].id1) {
            global_livemask |= j;
         }
      }
   }

   // We only allow it in a 3x4 if it was real, not phantom.  Arg1 tells which.
   if (ss->kind == s3x4 && phant == phantest_2x2_only_two) {
      if (global_livemask == 04747)
         division_map_code = spcmap_trglbox3x4a;
      else if (global_livemask == 05656)
         division_map_code = spcmap_trglbox3x4b;
      else if (global_livemask == 05353)
         division_map_code = spcmap_trglbox3x4c;
      else if (global_livemask == 05555)
         division_map_code = spcmap_trglbox3x4d;
      phant = phantest_ok;
   }
   else if (ss->kind == s4x5 && phant == phantest_2x2_only_two) {
      if (global_livemask == 0xB12C4)
         division_map_code = spcmap_trglbox4x5a;
      if (global_livemask == 0x691A4)
         division_map_code = spcmap_trglbox4x5b;
      phant = phantest_ok;
   }
   else if (ss->kind == s4x4)
      division_map_code = spcmap_trglbox4x4;

   if (division_map_code == ~0U)
      fail("Must have a 3x4, 4x4, or 4x5 setup for this concept.");

   divided_setup_move(ss, division_map_code, phant, true, result);
}


static void do_concept_do_phantom_boxes(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   if (ss->kind != s2x8) fail("Must have a 2x8 setup for this concept.");

   // We specify the CMD_MISC__NO_EXPAND_2 bit, to allow split phantom boxes split phantom CLW.
   // But only if this is split phantom, not interlocked phantom or plain phantom.
   divided_setup_move(ss, MAPCODE(s2x4,2,parseptr->concept->arg3,0),
                      (phantest_kind) parseptr->concept->arg1, true, result,
                      parseptr->concept->arg3 == MPKIND__SPLIT ?
                      CMD_MISC__NO_EXPAND_2 :
                      CMD_MISC__NO_EXPAND_1 | CMD_MISC__NO_EXPAND_2);
}


static void do_concept_do_phantom_diamonds(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   uint32_t map_code;

   // Arg2 is assumption stuff, described in "do_triple_formation".
   // Arg3 is an MPKIND: SPLIT, INTLK, or CONCPHAN.

   mpkind map_kind = (mpkind) parseptr->concept->arg3;
   mpkind het_map_kind = (map_kind == MPKIND__CONCPHAN) ? MPKIND__HET_CONCPHAN : map_kind;

   if (two_couple_calling && ss->kind == s2x3 && parseptr->concept->arg3 == MPKIND__CONCPHAN) {
      // This code copied from line 3643 or so.
      copy_person(ss, 6, ss, 5);
      copy_rot(ss, 5, ss, 0, 033);
      copy_person(ss, 7, ss, 4);
      copy_rot(ss, 4, ss, 6, 033);
      copy_person(ss, 6, ss, 1);
      copy_rot(ss, 1, ss, 3, 033);
      copy_rot(ss, 0, ss, 2, 033);
      copy_rot(ss, 3, ss, 7, 033);
      copy_rot(ss, 7, ss, 6, 033);
      ss->clear_person(2);
      ss->clear_person(6);
      ss->rotation++;
      ss->kind = s_qtag;
   }

   if (ss->kind == s4dmd)
      map_code = MAPCODE(s_qtag,2,map_kind,0);
   else if (ss->kind == s4ptpd)
      map_code = MAPCODE(s_ptpd,2,map_kind,0);
   else if (ss->kind == s_4mdmd)
      map_code = HETERO_MAPCODE(s_qtag,2,het_map_kind,0,s_ptpd,0);
   else if (ss->kind == s_4mptpd)
      map_code = HETERO_MAPCODE(s_ptpd,2,het_map_kind,0,s_qtag,0);
   else
      fail("Must have a quadruple diamond/quarter-tag setup for this concept.");

   ss->cmd.cmd_misc_flags |= parseptr->concept->arg2;

   divided_setup_move(ss, map_code, (phantest_kind) parseptr->concept->arg1, true, result);
}


static void do_concept_do_twinphantom_diamonds(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // See "do_triple_formation" for meaning of arg3.

   if (ss->kind != s2x2dmd)
      fail("Must have twin phantom diamond or 1/4 tag setup for this concept.");

   ss->cmd.cmd_misc_flags |= parseptr->concept->arg3;

   // If arg4 is nonzero, this is point-to-point diamonds.

   divided_setup_move(ss,
                      parseptr->concept->arg4 ?
                      MAPCODE(s_ptpd,2,MPKIND__SPLIT,1) :
                      MAPCODE(s_qtag,2,MPKIND__SPLIT,1),
                      (phantest_kind) parseptr->concept->arg1, true, result);
}


static void do_concept_do_divided_bones(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   setup tempsetup = *ss;

   if (parseptr->concept->arg2) {
      // Expand, first to a bigbone, and then to a dblrig.
      // Either or both of these may be unnecessary or may fail.

      tempsetup.do_matrix_expansion(CONCPROP__NEEDK_END_2X2, false);
      if (tempsetup.kind == sbigbone) expand::expand_setup(s_bigbone_dblrig, &tempsetup);

      divided_setup_move(&tempsetup,
                         MAPCODE(s_rigger,2,MPKIND__SPLIT,0),
                         (phantest_kind) parseptr->concept->arg1, true, result);
   }
   else {
      // Expand, first to a bigrig, and then to a dblbone.
      // Either or both of these may be unnecessary or may fail.

      tempsetup.do_matrix_expansion(CONCPROP__NEEDK_END_1X4, false);
      if (tempsetup.kind == sbigrig) expand::expand_setup(s_bigrig_dblbone, &tempsetup);

      divided_setup_move(&tempsetup,
                         MAPCODE(s_bone,2,MPKIND__SPLIT,0),
                         (phantest_kind) parseptr->concept->arg1, true, result);
   }
}


static void do_concept_distorted(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   ss->cmd.cmd_misc_flags |= CMD_MISC__SAID_PG_OFFSET;
   distorted_move(ss, parseptr,
                  (disttest_kind) parseptr->concept->arg1, parseptr->concept->arg4, result);
}


static void do_concept_dblbent(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{

/*
   Args from the concept are as follows:
   arg1 =
      0 - user claims this is some kind of columns
      1 - user claims this is some kind of lines
      3 - user claims this is waves
   8 bit -- this is double bent tidal
   16 bit -- this is bent box
*/

   uint32_t map_code = 0;
   uint32_t arg1 = parseptr->concept->arg1;
   setup otherfolks = *ss;
   setup *otherfolksptr = (setup *) 0;


   if (arg1 & 16) {
      //Bent boxes.
      if (ss->kind == s3x6) {
         if (global_livemask == 0x2170B)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT0CW,0);
         else if (global_livemask == 0x26934)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT0CCW,0);
         else if (global_livemask == 0x0B259)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT1CW,0);
         else if (global_livemask == 0x0CC66)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT1CCW,0);
      }
      else if (ss->kind == s4x6) {
         if (global_livemask == 0x981981)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT2CW,0);
         else if (global_livemask == 0x660660)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT2CCW,0);
         else if (global_livemask == 0xD08D08)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT3CW,0);
         else if (global_livemask == 0x2C42C4)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT3CCW,0);
         else if (global_livemask == 0x303303)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT4CW,0);
         else if (global_livemask == 0x330330)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT4CCW,0);
         else if (global_livemask == 0x858858)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT5CW,0);
         else if (global_livemask == 0x846846)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT5CCW,0);
      }
      else if (ss->kind == sbigh) {
         if (global_livemask == 0xCF3)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT6CW,0);
         else if (global_livemask == 0xF3C)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT6CCW,0);
      }
      else if (ss->kind == sdeepxwv) {
         if (global_livemask == 0xCF3)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT7CW,0);
         else if (global_livemask == 0x3CF)
            map_code = MAPCODE(s2x2,2,MPKIND__BENT7CCW,0);
      }
   }
   else if (!(arg1 & 8)) {
      //Bent C/L/W's.
      if (ss->kind == s3x6) {
         if (global_livemask == 0x2170B)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT2CW,0);
         else if (global_livemask == 0x26934)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT2CCW,0);
         else if (global_livemask == 0x0B259)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT3CW,0);
         else if (global_livemask == 0x0CC66)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT3CCW,0);
      }
      else if (ss->kind == s4x6) {
         if (global_livemask == 0x981981)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT0CW,0);
         else if (global_livemask == 0x660660)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT0CCW,0);
         else if (global_livemask == 0xD08D08)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT1CW,0);
         else if (global_livemask == 0x2C42C4)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT1CCW,0);
         else if (global_livemask == 0x303303)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT6CW,0);
         else if (global_livemask == 0x330330)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT6CCW,0);
         else if (global_livemask == 0x858858)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT7CW,0);
         else if (global_livemask == 0x846846)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT7CCW,0);
      }
      else if (ss->kind == sbigh) {
         if (global_livemask == 0xCF3)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT4CW,0);
         else if (global_livemask == 0xF3C)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT4CCW,0);
      }
      else if (ss->kind == sdeepxwv) {
         if (global_livemask == 0xCF3)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT5CW,0);
         else if (global_livemask == 0x3CF)
            map_code = MAPCODE(s1x4,2,MPKIND__BENT5CCW,0);
      }
      else if (ss->kind == s4x4) {
         if (global_livemask == 0x4B4B) {
            if (!((ss->people[0].id1 ^ ss->people[8].id1) & 1))
               fail("Setup must be unsymmetrical for this concept.");
            map_code = ((ss->people[0].id1 ^ arg1) & 1) ?
               MAPCODE(s2x4,1,MPKIND__BENT8NE,0) :
               MAPCODE(s2x4,1,MPKIND__BENT8SW,0);
         }
         else if (global_livemask == 0xB4B4) {
            if (!((ss->people[4].id1 ^ ss->people[12].id1) & 1))
               fail("Setup must be unsymmetrical for this concept.");
            map_code = ((ss->people[4].id1 ^ arg1) & 1) ?
               MAPCODE(s2x4,1,MPKIND__BENT8SE,0) :
               MAPCODE(s2x4,1,MPKIND__BENT8NW,0);
         }
      }
   }
   else {
      // Double bent tidal C/L/W.
      if (ss->kind == sbigh) {
         if (global_livemask == 0xCF3)
            map_code = MAPCODE(s1x8,1,MPKIND__BENT4CW,0);
         else if (global_livemask == 0xF3C)
            map_code = MAPCODE(s1x8,1,MPKIND__BENT4CCW,0);
      }
      else if (ss->kind == sbigptpd) {
         // We need to save a few people.  Maybe the "selective_move"
         // mechanism, with "override_selector", would be a better way.
         otherfolksptr = &otherfolks;

         if (global_livemask == 0xF3C)
            map_code = MAPCODE(s1x6,1,MPKIND__BENT0CW,0);
         else if (global_livemask == 0x3CF)
            map_code = MAPCODE(s1x6,1,MPKIND__BENT0CCW,0);
      }
   }

   if (!map_code)
      fail("Can't do this concept in this formation.");

   switch (arg1 & 7) {
   case 0: ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_COLS; break;
   case 1: ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_LINES; break;
   case 3: ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES; break;
   }

   divided_setup_move(ss, map_code, phantest_ok, true, result);

   if (otherfolksptr) {
      if (otherfolksptr->kind != result->kind ||
          otherfolksptr->rotation != result->rotation)
         fail("Can't do this concept in this formation.");
      install_person(result, 2, otherfolksptr, 2);
      install_person(result, 8, otherfolksptr, 8);
   }
}


static void do_concept_omit(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // We need to go through the motions of doing the call, at least
   // through any sequential definition, so that the number of parts
   // will be known.
   ss->cmd.cmd_misc2_flags |= CMD_MISC2__DO_NOT_EXECUTE;
   move(ss, false, result);
   // But that left the result as "nothing".  So now we
   // copy the incoming setup to the result, so that we will actually
   // have done the null call.  But of course we save our hard-won
   // result flags.
   resultflag_rec save_flags = result->result_flags;
   *result = *ss;
   result->result_flags = save_flags;
}


static void do_concept_once_removed(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   ss->clear_all_overcasts();
   uint32_t map_code = ~0U;
   ss->cmd.cmd_assume.assumption = cr_none;    // Not any more.

   // We allow "3x1" and the like only with plain "once removed".
   if (parseptr->concept->arg1 &&
       ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_MXNMASK | INHERITFLAG_NXNMASK))
      fail("Illegal modifier before a concept.");

   if (parseptr->concept->arg1 == 2) {
      switch (ss->kind) {
      case s1x6:
         map_code = MAPCODE(s1x2,3,MPKIND__TWICE_REMOVED,0);
         break;
      case s1x12:
         map_code = MAPCODE(s1x4,3,MPKIND__TWICE_REMOVED,0);
         break;
      case s2x6:
         map_code = MAPCODE(s2x2,3,MPKIND__TWICE_REMOVED,0);
         break;
      case s2x12:
         map_code = MAPCODE(s2x4,3,MPKIND__TWICE_REMOVED,0);
         break;
      case s_ptpd:
         map_code = HETERO_MAPCODE(s2x2,3,MPKIND__HET_TWICEREM,0,s1x2,0);
         break;
      case s_bone:      // Figure these out -- they are special.  Really?  What calls could you do?
      case s_rigger:
      default:
         fail("Can't do 'twice removed' from this setup.");
      }
   }
   else if (parseptr->concept->arg1 == 3) {
      switch (ss->kind) {
      case s1x8:
         map_code = MAPCODE(s1x2,4,MPKIND__THRICE_REMOVED,0);
         break;
      case s1x16:
         map_code = MAPCODE(s1x4,4,MPKIND__THRICE_REMOVED,0);
         break;
      case s2x8:
         map_code = MAPCODE(s2x2,4,MPKIND__THRICE_REMOVED,0);
         break;
      default:
         fail("Can't do 'thrice removed' from this setup.");
      }
   }
   else {

      // We allow "3x1" or "1x3".  That's all.
      // Well, we also allow "3x3" etc.

      switch (ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_MXNMASK | INHERITFLAG_NXNMASK)) {
      case 0:
         break;
      case INHERITFLAGMXNK_1X3:
      case INHERITFLAGMXNK_3X1:
         warn(warn__tasteless_junk);
         // We allow "12 matrix", but do not require it.  We have no
         // idea whether it should be required.
         ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_12_MATRIX);
         if (ss->kind == s_qtag) ss->do_matrix_expansion(CONCPROP__NEEDK_3X4, true);
         switch (ss->kind) {
         case s3x4:
            map_code = MAPCODE(s2x3,2,MPKIND__REMOVED,1);
            goto doit;
         default:
            fail("Can't do this concept in this formation.");
         }
         break;
      case INHERITFLAGNXNK_3X3:
         ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_12_MATRIX);
         switch (ss->kind) {
         case s1x12:
            map_code = MAPCODE(s1x6,2,MPKIND__REMOVED,0);
            goto doit;
         default:
            fail("Can't do this concept in this formation.");
         }
         break;
      case INHERITFLAGNXNK_4X4:
         ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_16_MATRIX);
         switch (ss->kind) {
         case s1x16:
            map_code = MAPCODE(s1x8,2,MPKIND__REMOVED,0);
            goto doit;
         default:
            fail("Can't do this concept in this formation.");
         }
         break;
      default:
         fail("Illegal modifier before a concept.");
      }

      if (parseptr->concept->arg1) {
         // If this is the "once removed diamonds" concept, we only allow diamonds.
         if (ss->kind != s_qtag && ss->kind != s_rigger)
            fail("There are no once removed diamonds here.");
      }
      else {

         // If this is just the "once removed" concept, we do NOT allow the splitting of a
         // quarter-tag into diamonds -- although there is only one splitting axis that
         // will work, it is not generally accepted usage.
         if (ss->kind == s_qtag)
            fail("You must use the \"once removed diamonds\" concept here.");
      }

      switch (ss->kind) {
      case s2x4:
         map_code = MAPCODE(s2x2,2,MPKIND__REMOVED,0);
         break;
      case s2x6:
         map_code = MAPCODE(s2x3,2,MPKIND__REMOVED,0);
         break;
      case s2x8:
         map_code = MAPCODE(s2x4,2,MPKIND__REMOVED,0);
         break;
      case s1x8:
         map_code = MAPCODE(s1x4,2,MPKIND__REMOVED,0);
         break;
      case s3x6:
         map_code = MAPCODE(s3x3,2,MPKIND__REMOVED,0);
         break;
      case s1x12:
         map_code = MAPCODE(s1x6,2,MPKIND__REMOVED,0);
         break;
      case s1x16:
         map_code = MAPCODE(s1x8,2,MPKIND__REMOVED,0);
         break;
      case s1x6:
         map_code = MAPCODE(s1x3,2,MPKIND__REMOVED,0);
         break;
      case s1x4:
         map_code = MAPCODE(s1x2,2,MPKIND__REMOVED,0);
         break;
      case s_rigger:
         map_code = MAPCODE(sdmd,2,MPKIND__REMOVED,0);
         break;
      case s_bone:
         map_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_ONCEREM,1,s_trngl4,0x7);
         break;
      case s_bone6:
         map_code = MAPCODE(s_trngl,2,MPKIND__REMOVED,1);
         break;
      case s_ptpd:
         map_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_ONCEREM,1,s_trngl4,0xD);
         break;
      case s_spindle:
         map_code = HETERO_MAPCODE(s2x2,2,MPKIND__HET_CO_ONCEREM,0,sdmd,0);
         break;
      case s1x3dmd:
         map_code = HETERO_MAPCODE(s1x4,2,MPKIND__HET_CO_ONCEREM,0,sdmd,0);
         break;
      case s_qtag:
         map_code = MAPCODE(sdmd,2,MPKIND__REMOVED,1);
         break;
      case slinebox:
         map_code = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_ONCEREM,0,s_trngl4,0x5);
         break;
      case sdbltrngl4:
         map_code = HETERO_MAPCODE(sdmd,2,MPKIND__HET_ONCEREM,0,s_trngl4,0x4);
         break;
      default:
         fail("Can't do 'once removed' from this setup.");
      }
   }

 doit:

   divided_setup_move(ss, map_code, phantest_ok, true, result);

   result->clear_all_overcasts();
}



static void do_concept_diagnose(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   char junk[200];

   move(ss, false, result);

   sprintf(junk, "Command flags: 0x%08X, result flags misc/split: 0x%08X/%d/%d.",
           (unsigned int) ss->cmd.cmd_misc_flags,
           (unsigned int) result->result_flags.misc,
           (unsigned int) result->result_flags.split_info[0],
           (unsigned int) result->result_flags.split_info[1]);
   fail(junk);
}


static void do_concept_stretch(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   ss->clear_all_overcasts();
   uint64_t mxnbits = ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_MXNMASK);
   bool mxnstuff = mxnbits == INHERITFLAGMXNK_1X3 || mxnbits == INHERITFLAGMXNK_3X1;

   setup tempsetup = *ss;
   move(&tempsetup, false, result);

   // If the fraction info indicates that the call is being broken up and we are being
   // asked to do a specific part, it's a good bet that it is not the last part.
   // Why?  Because the mechanism generally uses CMD_FRAC_CODE_FROMTOREV or
   // CMD_FRAC_CODE_FROMTOREVREV to get the last part.  It's not an iron-clad bet,
   // but it's reasonable.  If this is so, don't do the stretch.

   if (!tempsetup.cmd.cmd_fraction.is_null_with_masked_flags(CMD_FRAC_BREAKING_UP|CMD_FRAC_CODE_MASK,
                                                       CMD_FRAC_BREAKING_UP|CMD_FRAC_CODE_ONLY) ||
       parseptr->more_finalherit_flags.test_finalbit(FINAL__UNDER_RANDOM_META) != 0) {

      if (!mxnstuff) {
         // We don't do the splitting check if this was 3x1 -- too complicated for now.

         uint32_t field_to_check = (result->rotation & 1);
         // Short6 is geometrically strange.  Maybe ought to check bounding box instead?
         if (result->kind == s_short6)
            field_to_check ^= 1;

         // Because of obscure issues relating to whether "swing thru" in a 1x6 is grand (it isn't)
         // or done in each 1x3 (it is) we can get into messy situations deciding how the setup was
         // split.  2 1x3's?  3 1x2's?  Search the calls database for "each_1x3" to get an idea of
         // what is going on.  So the information we depend on in order to check the stretching
         // direction might not be present.  The warning "Do the call in each 1x3" will tell us
         // if this is happening.

         bool b = configuration::test_one_warning(warn__split_to_1x3s_always);

         if (!b && result->result_flags.split_info[field_to_check] == 0) {
            // It failed.  We will try again with a forced split.
            // This means that, if we said "stretch counter rotate" from waves, we will try
            // again with the 2x2 version of the call, that is, split counter rotate.
            // This may be controversial.

            if (ss->kind == s2x4 || ss->kind == s1x8) {   // Only do it for nice cases.
               tempsetup = *ss;

               tempsetup.cmd.cmd_misc_flags |=
                  (tempsetup.rotation & 1) ?
                  CMD_MISC__MUST_SPLIT_VERT :
                  CMD_MISC__MUST_SPLIT_HORIZ;

               // Do this in a catch block, so that, if the call isn't legal in the cut down
               // setup (e.g. trade circulate), we blame it on the stretch, not on the cutting down.

               try {
                  move(&tempsetup, false, result);
               }
               catch(error_flag_type) {
                  goto this_is_bad;
               }

               uint32_t field_to_check = (result->rotation & 1);
               // Short6 is geometrically strange.  Maybe ought to check bounding box instead?
               if (result->kind == s_short6)
                  field_to_check ^= 1;

               // Check again.
               if (result->result_flags.split_info[field_to_check] == 0) {
                  goto this_is_bad;
               }

               warn(warn__did_weird_stretch_response);
            }
            else {
               goto this_is_bad;
            }
         }
      }

      if (mxnstuff && (result->kind == s1x8)) {
         // This is a bit sleazy.

         if (((result->people[0].id1 ^ result->people[1].id1) & 2) ||
             (result->result_flags.misc & RESULTFLAG__VERY_ENDS_ODD)) {
            result->swap_people(1, 6);
            result->swap_people(2, 5);
            result->swap_people(3, 7);
         }
         else if (((result->people[2].id1 ^ result->people[3].id1) & 2) ||
                  (result->result_flags.misc & RESULTFLAG__VERY_CTRS_ODD)) {
            result->swap_people(2, 6);
         }
         else {
            fail("Sorry, can't figure this out.");
         }
      }
      else {
         if (mxnstuff)
            fail("1x3 or 3x1 Stretch call not permitted here.");

         if (result->kind == s2x4) {
            result->swap_people(1, 2);
            result->swap_people(5, 6);
         }
         else if (result->kind == s1x8) {
            result->swap_people(3, 6);
            result->swap_people(2, 7);
         }
         else if (result->kind == s1x6) {
            result->swap_people(2, 5);
         }
         else if (result->kind == s_qtag) {
            result->swap_people(3, 7);
         }
         else if (result->kind == s_ptpd) {
            result->swap_people(2, 6);
         }
         else if (result->kind == s1x4) {
            result->swap_people(1, 3);
         }
         else if (result->kind == s3x4) {
            uint32_t tt = result->little_endian_live_mask();
            if (tt == 07171)
               result->swap_people(5, 11);
            else if (tt == 06666) {
               result->swap_people(5, 11);
               result->swap_people(1, 2);
               result->swap_people(7, 8);
               warn(warn__6peoplestretched);
            }
            else
               goto fail1;
         }
         else if (result->kind == s_short6) {
            result->swap_people(5, 0);
            result->swap_people(2, 3);
         }
         else if (result->kind == s_rigger) {
            result->swap_people(5, 4);
            result->swap_people(0, 1);
         }
         else if (result->kind == s_bone) {
            result->swap_people(3, 6);
            result->swap_people(2, 7);
         }
         else if (result->kind == s_bone6) {
            result->swap_people(2, 5);
         }
         else
            goto fail1;
      }
   }

   result->clear_all_overcasts();
   return;

 fail1:
   fail("Stretch call didn't go to a legal setup.");

 this_is_bad:
   fail("Stretch call was not a 4 person call divided along stretching axis.");
}


static void do_concept_stretched_setup(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   ss->clear_all_overcasts();
   uint32_t maps;
   setup tempsetup = *ss;
   int linesp = parseptr->concept->arg1;

   // linesp =
   //  0x10 : any setup
   //  1    : line
   //  3    : wave
   //  4    : column
   //  0x12 : box
   //  0x13 : diamond spots
   //  0x14 : just "stretched", to be used with triangles.

   if ((linesp == 0x12 && tempsetup.kind != s2x4) ||
       (linesp == 0x13 && tempsetup.kind != s_qtag && tempsetup.kind != s_ptpd) ||
       (!(linesp & 0x10) && tempsetup.kind != s1x4 && tempsetup.kind != s1x8))
      fail("Not in correct formation for this concept.");

   if (!(linesp & 0x10)) {
      if (linesp & 1) {
         if (global_tbonetest & 1) fail("There is no line of 8 here.");
      }
      else {
         if (global_tbonetest & 010) fail("There is no column of 8 here.");
      }
   }

   if (linesp == 3)
      ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

   tempsetup.cmd.cmd_misc_flags |= CMD_MISC__DISTORTED;

   if (linesp == 0x14) {
      // This was just "stretched".  Demand that the next concept
      // be some kind of triangle designation.
      // Search ahead, skipping comments of course.  This means we
      // must skip modifiers too, so we check that there weren't any.

      final_and_herit_flags junk_concepts;
      junk_concepts.clear_all_herit_and_final_bits();
      parse_block *next_parseptr = process_final_concepts(parseptr->next, false,
                                                          &junk_concepts, true, false);

      if ((next_parseptr->concept->kind == concept_so_and_so_only) &&
          next_parseptr->options.who.who[0] >= selector_TGL_START &&
          next_parseptr->options.who.who[0] < selector_SOME_START &&
          !junk_concepts.test_for_any_herit_or_final_bit()) {
         if (tempsetup.kind == s_hrglass) {
            tempsetup.swap_people(3, 7);
         }
         else if (tempsetup.kind == s_ptpd &&
                  next_parseptr->concept->kind == concept_so_and_so_only &&
                  next_parseptr->options.who.who[0] == selector_inside_tgl) {
            // We require "inside triangles".
            tempsetup.swap_people(2, 6);
         }
         else
            fail("Stretched setup call didn't start in appropriate setup.");
      }
      else if (next_parseptr->concept->kind == concept_do_phantom_2x4 &&
               next_parseptr->concept->arg3 == MPKIND__SPLIT &&
               !junk_concepts.test_for_any_herit_or_final_bit()) {
         if (tempsetup.kind == s4x4 && ((global_tbonetest & 011) != 011)) {
            if ((global_tbonetest ^ next_parseptr->concept->arg2) & 1) {
               tempsetup.swap_people(1, 2);
               tempsetup.swap_people(3, 7);
               tempsetup.swap_people(15, 11);
               tempsetup.swap_people(10, 9);
            }
            else {
               tempsetup.swap_people(13, 14);
               tempsetup.swap_people(15, 3);
               tempsetup.swap_people(11, 7);
               tempsetup.swap_people(6, 5);
            }
         }
         else
            fail("Stretched setup call didn't start in appropriate setup.");
      }
      else
         fail("\"Stretched\" concept must be followed by triangle or split phantom concept.");

      move(&tempsetup, false, result);
      return;
   }

   if (tempsetup.kind == s2x4) {
      tempsetup.swap_people(1, 2);
      tempsetup.swap_people(5, 6);
      maps = MAPCODE(s2x2,2,MPKIND__SPLIT,0);
   }
   else if (tempsetup.kind == s1x4) {
      tempsetup.swap_people(1, 3);
      maps = MAPCODE(s1x2,2,MPKIND__SPLIT,0);
   }
   else if (tempsetup.kind == s1x8) {
      tempsetup.swap_people(3, 6);
      tempsetup.swap_people(2, 7);
      maps = MAPCODE(s1x4,2,MPKIND__SPLIT,0);
   }
   else if (tempsetup.kind == s_qtag) {
      tempsetup.swap_people(3, 7);
      maps = MAPCODE(sdmd,2,MPKIND__SPLIT,1);
   }
   else if (tempsetup.kind == s_ptpd) {
      tempsetup.swap_people(2, 6);
      maps = MAPCODE(sdmd,2,MPKIND__SPLIT,0);
   }
   else if (tempsetup.kind == s_bone6) {
      tempsetup.swap_people(2, 5);
      maps = HETERO_MAPCODE(s_trngl,2,MPKIND__HET_SPLIT,1,s_trngl,0x7);
   }
   else if (tempsetup.kind == s_rigger) {
      tempsetup.swap_people(0, 1);
      tempsetup.swap_people(4, 5);
      maps = HETERO_MAPCODE(s_trngl4,2,MPKIND__HET_SPLIT,1,s_trngl4,0xD);
   }
   else if (tempsetup.kind == s3x4 && tempsetup.little_endian_live_mask() == 07171) {
      tempsetup.swap_people(5, 11);
      maps = MAPCODE(s2x3,2,MPKIND__SPLIT,1);
   }
   else
      fail("Stretched setup call didn't start in appropriate setup.");

   divided_setup_move(&tempsetup, maps, phantest_ok, true, result);

   result->clear_all_overcasts();
}


static void do_concept_mirror(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   /* This is treated differently from "left" or "reverse" in that the code in sdmoves.cpp
      that handles the latter communicates it to the "take right hands" and "rear back
      from right hands" stuff, and we do not.  This means that people will physically
      take left hands when they meet.  This seems to be the consensus of what "mirror"
      means, as distinguished from "left" or "reverse".

      We may also need to be careful about unsymmetrical selectors.  It happens that
      there are no selectors that are sensitive to left-right position ("box to my
      left peel off"), and that unsymmetrical selectors are loaded at the start of
      the entire call, when absolute orientation is manifest, so we luck out.
      That may change in the future.  We have to be careful never to load unsymmetrical
      selectors unless we are in absolute orientation space. */

   mirror_this(ss);
   ss->update_id_bits();
   // We xor it of course -- these can be nested! 
   ss->cmd.cmd_misc_flags ^= CMD_MISC__EXPLICIT_MIRROR;
   move(ss, false, result);
   mirror_this(result);
}



static void do_concept_assume_waves(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   assumption_thing t;
   assumption_thing *e = &ss->cmd.cmd_assume;   /* Existing assumption. */
   setup sss;

   // "Assume normal casts" is special.

   if (parseptr->concept->arg1 == cr_alwaysfail) {
      if (e->assump_cast)
         fail("Redundant or conflicting assumptions.");
      e->assump_cast = 1;
      move(ss, false, result);
      return;
   }

   // We wish it were possible to encode this entire word neatly in the concept
   // table, but, unfortunately, the way C struct and union initializers work
   // makes it impossible.

   t.assumption = (call_restriction) parseptr->concept->arg1;
   t.assump_col = parseptr->concept->arg2;
   t.assump_both = parseptr->concept->arg3;
   t.assump_cast = e->assump_cast;
   t.assump_live = 0;
   t.assump_negate = 0;

   // Need to check any pre-existing assumption.

   if (e->assumption == cr_none) ;   // If no pre-existing assumption, OK.
   else {     // We have something, and must check carefully.
      // First, an exact match is allowed.
      if (e->assumption == t.assumption &&
          e->assump_col == t.assump_col &&
          e->assump_both == t.assump_both) ;

      // We also allow certain tightenings of existing assumptions.

      else if ((t.assumption == cr_jleft || t.assumption == cr_jright) &&
               ((e->assumption == cr_diamond_like && t.assump_col == 4) ||
                (e->assumption == cr_qtag_like &&
                 t.assump_col == 0 &&
                 (e->assump_both == 0 || e->assump_both == (t.assump_both ^ 3))) ||
                (e->assumption == cr_real_1_4_tag && t.assump_both == 2) ||
                (e->assumption == cr_real_3_4_tag && t.assump_both == 1))) ;
      else if ((t.assumption == cr_ijleft || t.assumption == cr_ijright) &&
               ((e->assumption == cr_diamond_like && t.assump_col == 4) ||
                (e->assumption == cr_qtag_like &&
                 t.assump_col == 0 &&
                 (e->assump_both == 0 || e->assump_both == (t.assump_both ^ 3))) ||
                (e->assumption == cr_real_1_4_line && t.assump_both == 2) ||
                (e->assumption == cr_real_3_4_line && t.assump_both == 1))) ;
      else if ((t.assumption == cr_real_1_4_tag || t.assumption == cr_real_1_4_line) &&
               e->assumption == cr_qtag_like &&
               e->assump_both == 1) ;
      else if ((t.assumption == cr_real_3_4_tag || t.assumption == cr_real_3_4_line) &&
               e->assumption == cr_qtag_like &&
               e->assump_both == 2) ;
      else
         fail("Redundant or conflicting assumptions.");
   }

   if (t.assumption == cr_miniwaves || t.assumption == cr_couples_only ||
       ((t.assumption == cr_magic_only || t.assumption == cr_wave_only) &&
        t.assump_col == 2)) {
      switch (ss->kind) {
      case s2x4:
      case s2x6:
      case s2x8:
      case s2x3:
         if (!(ss->or_all_people() & 010)) t.assump_col++;
         break;
      }
   }

   ss->cmd.cmd_assume = t;

   // The restrictions mean different things in different setups.  In some setups, they
   // mean things that are unsuitable for the "assume" concept.  In some setups they
   // take no action at all.  So we must check the setups on a case-by-case basis.

   if (     t.assumption == cr_jleft  || t.assumption == cr_jright ||
            t.assumption == cr_ijleft || t.assumption == cr_ijright ||
            t.assumption == cr_real_1_4_tag  || t.assumption == cr_real_3_4_tag ||
            t.assumption == cr_real_1_4_line || t.assumption == cr_real_3_4_line) {
      // These assumptions work independently of the "assump_col" number.
      goto fudge_diamond_like;
   }
   else if ((t.assump_col & 2) == 2) {
      // These are special assumptions -- "assume normal boxes", or "assume inverted boxes".
      if (t.assumption == cr_wave_only || t.assumption == cr_magic_only) {
         switch (ss->kind) {
            case s2x2: goto check_it;
            case s2x4: goto check_it;
         }
      }
      else if (t.assumption == cr_split_square_setup || t.assumption == cr_liftoff_setup) {
         if (ss->kind == s2x4)
            goto check_it;
         else
            goto check_for_2x2;
      }
      else if (t.assumption == cr_i_setup) {
         if (ss->kind == s_bone)
            goto check_it;
         else
            goto check_for_1x4_to_bone;
      }
   }
   else if (t.assump_col == 1) {
      // This is a "column-like" assumption.

      if (((t.assumption == cr_wave_only ||
            t.assumption == cr_magic_only ||
            t.assumption == cr_2fl_only ||
            t.assumption == cr_li_lo ||
            t.assumption == cr_quarterbox ||
            t.assumption == cr_threequarterbox) &&
           (ss->kind == s2x2 || ss->kind == s2x4)) ||
          ((t.assumption == cr_couples_only ||
            t.assumption == cr_miniwaves) &&
           (ss->kind == s2x2 || ss->kind == s2x3 || ss->kind == s2x4 || ss->kind == s2x6 || ss->kind == s2x8))) {
         if (ss->kind == s2x2) {
            if (two_couple_calling) {
               // expand to 2x4
               if ((ss->or_all_people() & 010) == 0)
                  expand::expand_setup(s_2x2_2x4_ctrs, ss);
               else
                  expand::expand_setup(s_2x2_2x4_ctrsb, ss);
            }
            else
               goto bad_assume;
         }

         goto check_it;
      }
      else if (t.assumption == cr_tidal_line) {
         switch (ss->kind) {
         case s1x4: case s1x6: case s1x8: goto check_for_1x4_1x6;
         }
      }
   }
   else {
      // This is a "line-like" assumption.

      switch (t.assumption) {
      case cr_tidal_wave:
      case cr_tidal_line:
      case cr_tidal_2fl:
         switch (ss->kind) {     // "assume tidal wave/line", 2-couple only
         case s1x4: case s1x6: case s1x8: goto check_for_1x4_1x6;
         }
         break;
      case cr_wave_only:
         switch (ss->kind) {     // "assume waves"
         case s2x2: case s2x4: case s3x4:  case s2x8:  case s2x6:
         case s1x8: case s1x10: case s1x12: case s1x14:
         case s1x16: case s1x6: case s1x4: goto check_for_2x2;
         }
         break;
      case cr_2fl_only:
         switch (ss->kind) {     // "assume two-faced lines" 
         case s2x2: case s2x4:  case s3x4:  case s1x8:  case s1x4: goto check_for_2x2;
         }
         break;
      case cr_couples_only:
      case cr_miniwaves:
         switch (ss->kind) {     // "assume couples" or "assume miniwaves"
         case s2x2: case s1x2: case s1x8: case s1x16:
         case s2x8: case s2x4: case s1x4: goto check_it;  // 2x2 is completely acceptable.
         }
         break;
      case cr_magic_only:
         switch (ss->kind) {     // "assume inverted lines"
         case s2x2: case s2x4:  case s1x4: goto check_for_2x2;
         }
         break;
      case cr_1fl_only:
         switch (ss->kind) {     // "assume one-faced lines"
         case s2x2: case s1x4: case s2x4: goto check_for_2x2;
         }
         break;
      case cr_li_lo:
         switch (ss->kind) {     // "assume lines in" or "assume lines out"
         case s2x2: case s2x3: case s2x4: goto check_for_2x2;
         }
         break;
      case cr_diamond_like:
      case cr_qtag_like:
      case cr_pu_qtag_like:
         goto fudge_diamond_like;
      case cr_hourglass:
         if (ss->kind == s_hrglass)
            goto check_it;
         else if (two_couple_calling) {
            if (ss->kind == s2x4 &&
                (ss->people[1].id1 | ss->people[2].id1 | ss->people[5].id1 | ss->people[6].id1) == 0) {
               expand::expand_setup(s_2x4_hrgl_pts, ss);
               goto check_it;
            }
            else if (ss->kind == sdmd) {
               expand::expand_setup(s_dmd_hrgl_ctrs, ss);
               goto check_it;
            }
            else if (ss->kind == s2x3) {
               expand::expand_setup(s2x3_hrgl, ss);
               goto check_it;
            }
            else if (ss->kind == s_bone6) {
               expand::expand_setup(s_bone6_hrgl, ss);
               goto check_it;
            }
         }
      case cr_galaxy:
         if (ss->kind == s_galaxy)
            goto check_it;
         else if (ss->kind == s2x2) {
            if (two_couple_calling) {
               expand::expand_setup(s_2x2_gal_ctrs, ss);
               goto check_it;
            }
            else
               goto bad_assume;
         }
      }
   }

   goto bad_assume;

 check_for_2x2:

   if (ss->kind == s2x2) {
      if (two_couple_calling) {
         // expand to 2x4
         if ((ss->or_all_people() & 010) == 0)
            expand::expand_setup(s_2x2_2x4_ctrsb, ss);
         else
            expand::expand_setup(s_2x2_2x4_ctrs, ss);
      }
      else
         goto bad_assume;
   }

   goto check_it;

 check_for_1x4_to_bone:

   if (ss->kind == s1x4) {
      if (two_couple_calling) {
         expand::expand_setup(s_1x4_bone_ctrs, ss);
      }
      else
         goto bad_assume;
   }

   goto check_it;

 check_for_1x4_1x6:
   // expand to 1x8

   if (two_couple_calling) {
      if (ss->kind != s1x8)
         expand::expand_setup((ss->kind == s1x4) ? s_1x4_1x8_ctrs : s_1x6_1x8_ctrs, ss);
   }
   else
      goto bad_assume;

   goto check_it;

 fudge_diamond_like:

   switch (ss->kind) {
   case s1x4:
      copy_person(ss, 6, ss, 0);
      copy_person(ss, 7, ss, 1);
      ss->clear_person(0);
      ss->clear_person(1);
      ss->clear_person(4);
      ss->clear_person(5);
      ss->kind = s_qtag;
      goto check_it;
   case s2x4:
      if (ss->people[1].id1 || ss->people[2].id1 || ss->people[5].id1 || ss->people[6].id1)
         fail("Can't do this assumption.");
      copy_rot(ss, 5, ss, 0, 033);
      copy_rot(ss, 1, ss, 4, 033);
      copy_rot(ss, 0, ss, 3, 033);
      copy_rot(ss, 4, ss, 7, 033);
      ss->clear_person(3);
      ss->clear_person(7);
      ss->rotation++;
      ss->kind = s_qtag;
      goto check_it;
   case s2x3:
      copy_person(ss, 6, ss, 5);
      copy_rot(ss, 5, ss, 0, 033);
      copy_person(ss, 7, ss, 4);
      copy_rot(ss, 4, ss, 6, 033);
      copy_person(ss, 6, ss, 1);
      copy_rot(ss, 1, ss, 3, 033);
      copy_rot(ss, 0, ss, 2, 033);
      copy_rot(ss, 3, ss, 7, 033);
      copy_rot(ss, 7, ss, 6, 033);
      ss->clear_person(2);
      ss->clear_person(6);
      ss->rotation++;
      ss->kind = s_qtag;
      goto check_it;
   case s3x4:
      sss = *ss;
      copy_person(ss, 0, &sss, 1);
      copy_person(ss, 1, &sss, 2);
      copy_person(ss, 2, &sss, 4);
      copy_person(ss, 3, &sss, 5);
      copy_person(ss, 4, &sss, 7);
      copy_person(ss, 5, &sss, 8);
      copy_person(ss, 6, &sss, 10);
      copy_person(ss, 7, &sss, 11);

      if (sss.people[0].id1) {
         if (sss.people[1].id1) fail("Can't do this assumption.");
         else copy_person(ss, 0, &sss, 0);
      }
      if (sss.people[3].id1) {
         if (sss.people[2].id1) fail("Can't do this assumption.");
         else copy_person(ss, 1, &sss, 3);
      }
      if (sss.people[6].id1) {
         if (sss.people[7].id1) fail("Can't do this assumption.");
         else copy_person(ss, 4, &sss, 6);
      }
      if (sss.people[9].id1) {
         if (sss.people[8].id1) fail("Can't do this assumption.");
         else copy_person(ss, 5, &sss, 9);
      }

      ss->kind = s_qtag;
      goto check_it;
   case sdmd: case s_qtag: case s_ptpd: case s3dmd: case s4dmd: goto check_it;
   }

 bad_assume:

   fail("This assumption is not legal from this formation.");

   check_it:

   move_perhaps_with_active_phantoms(ss, result);
}




static void do_concept_active_phantoms(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   if (ss->cmd.cmd_assume.assump_cast)
      fail("Don't use \"active phantoms\" with \"assume normal casts\".");

   if (fill_active_phantoms_and_move(ss, result))
      fail("This assumption is not specific enough to fill in active phantoms.");
}


static void do_concept_rectify(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // Check that we aren't setting the bit twice.

   if (ss->cmd.cmd_misc3_flags & CMD_MISC3__RECTIFY)
      fail("You can't give this concept twice.");

   ss->cmd.cmd_misc3_flags |= CMD_MISC3__RECTIFY;

   move(ss, false, result);
}


static void do_concept_central(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   if (parseptr->concept->arg1 == CMD_MISC2__SAID_INVERT) {
      // If this is "invert", just flip the bit.  They can stack, of course.
      ss->cmd.cmd_misc2_flags ^= CMD_MISC2__SAID_INVERT;
   }
   else {
      uint32_t this_concept = parseptr->concept->arg1;

      // Otherwise, if the "invert" bit was on, we assume that means that
      // the user really wanted "invert snag" or whatever.

      if (ss->cmd.cmd_misc2_flags & CMD_MISC2__SAID_INVERT) {
         if (this_concept &
            (CMD_MISC2__INVERT_CENTRAL|CMD_MISC2__INVERT_SNAG|CMD_MISC2__INVERT_MYSTIC))
            fail("You can't invert a concept twice.");
         // Take out the "invert" bit".
         ss->cmd.cmd_misc2_flags &= ~CMD_MISC2__SAID_INVERT;
         // Put in the "this concept is inverted" bit.
         if (this_concept == CMD_MISC2__DO_CENTRAL)
            this_concept |= CMD_MISC2__INVERT_CENTRAL;
         else if (this_concept == CMD_MISC2__CENTRAL_SNAG)
            this_concept |= CMD_MISC2__INVERT_SNAG;
         else if (this_concept == CMD_MISC2__CENTRAL_MYSTIC)
            this_concept |= CMD_MISC2__INVERT_MYSTIC;
      }

      // Check that we aren't setting the bit twice.

      if (ss->cmd.cmd_misc2_flags & this_concept)
         fail("You can't give this concept twice.");

      ss->cmd.cmd_misc2_flags |= this_concept;
   }

   move(ss, false, result);
}


static void do_concept_crazy(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   uint32_t finalresultflagsmisc = 0;
   int this_part;

   setup tempsetup = *ss;

   int reverseness = parseptr->concept->arg1;

   if (tempsetup.cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_REVERSE)) {
      if (reverseness) fail("Redundant 'REVERSE' modifiers.");
      if (parseptr->concept->arg2) fail("Don't put 'reverse' in front of the fraction.");
      reverseness = 1;
   }

   tempsetup.cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_REVERSE);

   // We don't allow other flags, like "cross", but we do allow "MxN".
   if ((tempsetup.cmd.cmd_final_flags.herit & ~(INHERITFLAG_MXNMASK|INHERITFLAG_NXNMASK |
                                                INHERITFLAG_INROLL|INHERITFLAG_OUTROLL|
                                                INHERITFLAG_SPLITTRADE|INHERITFLAG_BIAS|
                                                INHERITFLAG_BIASTRADE|INHERITFLAG_ORBIT|
                                                INHERITFLAG_TWINORBIT|INHERITFLAG_ROTARY|
                                                INHERITFLAG_SCATTER | INHERITFLAG_ZOOMROLL)) != 0 ||
       tempsetup.cmd.cmd_final_flags.final != 0)
      fail("Illegal modifier before \"crazy\".");

   // We will modify these flags, and, in any case, we need to rematerialize them at each step.
   setup_command cmd = tempsetup.cmd;

   int craziness_integer = 4;
   int craziness_fraction_num = 0;
   int craziness_fraction_den = 1;

   if (parseptr->concept->arg2 == 2) {
      int num = (parseptr->options.number_fields & NUMBER_FIELD_MASK) << 2;
      craziness_fraction_den = parseptr->options.number_fields >> BITS_PER_NUMBER_FIELD;
      craziness_integer = num/craziness_fraction_den;
      craziness_fraction_num = num - craziness_integer*craziness_fraction_den;
   }
   else if (parseptr->concept->arg2)
      craziness_integer = parseptr->options.number_fields & NUMBER_FIELD_MASK;

   fraction_command incomingfracs = cmd.cmd_fraction;
   incomingfracs.flags &= ~CMD_FRAC_THISISLAST;

   if (incomingfracs.flags & CMD_FRAC_REVERSE)
      reverseness ^= (craziness_integer ^ 1);    // That's all it takes!

   // We now have the overall number of crazy parts to do (that is, the fraction as a number of quarters)
   // and an additional fraction of quarters in craziness_fraction.  Turn the whole thing into one
   // big fraction, in quarters.  That is, "5/8 crazy" gives 5/2 here.  "Crazy" gives 4.

   craziness_fraction_num += craziness_fraction_den*craziness_integer;

   int s_denom = incomingfracs.start_denom();
   int s_numer = incomingfracs.start_numer();
   int e_denom = incomingfracs.end_denom();
   int e_numer = incomingfracs.end_numer();

   s_numer *= craziness_fraction_num;
   s_denom *= craziness_fraction_den;
   e_numer *= craziness_fraction_num;
   e_denom *= craziness_fraction_den;

   // Now s_numer/s_denom give start point, calibrated in parts.
   // and e_numer/e_denom give end point.

   reduce_fraction(s_numer, s_denom);
   reduce_fraction(e_numer, e_denom);

   int i = s_numer / s_denom;           // The start point.  It must be an integer.
   int highlimit = e_numer / e_denom;   // The end point.
   e_numer -= highlimit * e_denom;      // This now has just the fractional remainder.

   // If end isn't an integral number of parts, bump the count.
   // We will apply the fractional part to the last cycle.
   if (e_numer != 0) highlimit++;
   int craziness = highlimit;

   // The start point must be an integral number of parts.
   if (i*s_denom != s_numer || i >= highlimit)
      fail("Illegal fraction for \"crazy\".");

   // We have *not* checked for highlimit <= 4.  We allow 5/4 crazy.
   // Use tastefully.

   this_part = (incomingfracs.flags & CMD_FRAC_PART_MASK) / CMD_FRAC_PART_BIT;

   if (this_part > 0) {
      switch (incomingfracs.flags & (CMD_FRAC_CODE_MASK | CMD_FRAC_PART2_MASK)) {
      case CMD_FRAC_CODE_ONLY:
         // Request is to do just part this_part.
         if (e_numer != 0 && this_part >= highlimit)
            fail("Can't select fractional part of \"crazy\".");
         e_numer = 0;
         i = this_part-1;
         highlimit = this_part;
         finalresultflagsmisc |= RESULTFLAG__PARTS_ARE_KNOWN;
         if (highlimit == craziness) finalresultflagsmisc |= RESULTFLAG__DID_LAST_PART;
         break;
      case CMD_FRAC_CODE_FROMTO:
         // Request is to do everything up through part this_part.
         if (e_numer != 0 && this_part >= highlimit)
            fail("Can't select fractional part of \"crazy\".");
         e_numer = 0;
         highlimit = this_part;
         break;
      case CMD_FRAC_CODE_FROMTOREV: case CMD_FRAC_CODE_SKIP_K_MINUS_HALF:
         // Request is to do everything strictly after part this_part-1.
         i = this_part-1;
         break;
      case CMD_FRAC_CODE_ONLYREV:
         // Request is to do just part this_part, counting from the end.
         if (e_numer != 0 && this_part >= highlimit)
            fail("Can't select fractional part of \"crazy\".");
         e_numer = 0;
         i = highlimit-this_part;
         highlimit += 1-this_part;
         finalresultflagsmisc |= RESULTFLAG__PARTS_ARE_KNOWN;
         if (highlimit == craziness) finalresultflagsmisc |= RESULTFLAG__DID_LAST_PART;
         break;
      default:
         // If the "K" field ("part2") is nonzero, maybe we can still do something.
         switch (incomingfracs.flags & CMD_FRAC_CODE_MASK) {
         case CMD_FRAC_CODE_FROMTOREV: case CMD_FRAC_CODE_SKIP_K_MINUS_HALF:
            i = this_part-1;
            highlimit -= (incomingfracs.flags & CMD_FRAC_PART2_MASK) / CMD_FRAC_PART2_BIT;
            break;
         default:
            fail("\"crazy\" is not allowed after this concept.");
         }
      }
   }

   if (highlimit <= i || highlimit > craziness) fail("Illegal fraction for \"crazy\".");

   // No fractions for subject, we have consumed them.
   cmd.cmd_fraction.set_to_null();

   // Lift the craziness restraint from before -- we are about to pull things apart.
   cmd.cmd_misc3_flags &= ~CMD_MISC3__RESTRAIN_CRAZINESS;
   cmd.promote_restrained_fraction();

   if (parseptr->concept->arg3 == 1)   // Handle "crazy Z's".
      cmd.cmd_misc3_flags |= CMD_MISC3__IMPOSE_Z_CONCEPT;

   // If we didn't check for an 8-person setup, and we had a 1x4, the "do it on each side"
   // stuff would just do it without splitting or thinking anything was unusual,
   // while the "do it in the center" code would catch it at present, but might
   // not in the future if we add the ability of the concentric schema to mean
   // pick out the center 2 from a 1x4.  In any case, if we didn't do this
   // check, "1/4 reverse crazy bingo" would be legal from a 2x2.
   // But the check is actually done a little below, after taking care of the
   // "PRIOR_ELONG_BASE_FOR_TANDEM" stuff.

   for ( ; i<highlimit; i++) {
      tempsetup.cmd = cmd;    // Get a fresh copy of the command.

      tempsetup.update_id_bits();

      // If craziness isn't an integral multiple of 1/4 and this is the last time,
      // put in the fraction.
      if (i == highlimit-1 && e_numer != 0) {
         tempsetup.cmd.cmd_fraction.flags = 0;
         tempsetup.cmd.cmd_fraction.fraction =
            (e_denom << BITS_PER_NUMBER_FIELD) + e_numer + NUMBER_FIELDS_1_0_0_0;
      }

      if ((i ^ reverseness) & 1) {
         // Do it in the center.
         // If this is crazy Z's, CMD_MISC3__IMPOSE_Z_CONCEPT will be on, and the selector will be changed later.
         who_list sel;
         sel.initialize();
         sel.who[0] = selector_center4;

         if (attr::klimit(tempsetup.kind) < 7)
            sel.who[0] = selector_center2;

         // We might be doing a "finally 1/2 crazy central little more".
         // So we need to clear the splitting bits when doing it in the center.
         tempsetup.cmd.cmd_misc_flags &= ~CMD_MISC__MUST_SPLIT_MASK;

         // The -1 for arg 4 makes it preserve roll information for the inactives.
         selective_move(&tempsetup, parseptr, selective_key_plain,
                        -1, 0, 0, sel, false, result);
      }
      else {
         // Do it on each side.
         if (attr::klimit(tempsetup.kind) < 7) {
            if (tempsetup.kind == s2x2) {
               // If we have a clue about splitting info, fill it in.
               if (tempsetup.cmd.prior_elongation_bits & PRIOR_ELONG_BASE_FOR_TANDEM) {
                  tempsetup.cmd.cmd_misc_flags |= CMD_MISC__MUST_SPLIT_HORIZ;
               }
            }
            else {
               if (tempsetup.kind != s1x4)
                  fail("Need an 8-person setup for this.");

               tempsetup.cmd.cmd_misc_flags |=
                  (tempsetup.rotation & 1) ? CMD_MISC__MUST_SPLIT_VERT : CMD_MISC__MUST_SPLIT_HORIZ;
            }
         }
         else {
            tempsetup.cmd.cmd_misc_flags |=
               (tempsetup.rotation & 1) ? CMD_MISC__MUST_SPLIT_VERT : CMD_MISC__MUST_SPLIT_HORIZ;
         }

         move(&tempsetup, false, result);
      }

      finalresultflagsmisc |= result->result_flags.misc;

      tempsetup = *result;
   }

   result->result_flags.misc = finalresultflagsmisc;
}


static void do_concept_phan_crazy(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   if (process_brute_force_mxn(ss, parseptr, do_concept_phan_crazy, result)) return;

   int i;
   setup_kind kk = s4x4;

   int reverseness = (parseptr->concept->arg1 >> 3) & 1;

   if (ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_REVERSE)) {
      if (reverseness) fail("Redundant 'REVERSE' modifiers.");
      reverseness = 1;
   }

   ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_REVERSE);

   // We don't allow other flags, like "cross".
   if (ss->cmd.cmd_final_flags.test_for_any_herit_or_final_bit())
      fail("Illegal modifier before \"crazy\".");

   int craziness = (parseptr->concept->arg1 & 16) ?
      parseptr->options.number_fields : 4;

   setup tempsetup = *ss;

   setup_command cmd = tempsetup.cmd;    // We will modify these flags, and, in any case,
                                         // we need to rematerialize them at each step.

   uint32_t offsetmapcode;
   uint32_t specialmapcode = ~0U;

   phantest_kind phanstuff = phantest_ok;

   // Turn on "verify waves" or whatever, for the first time only.
   tempsetup.cmd.cmd_misc_flags |= parseptr->concept->arg3;

   if (parseptr->concept->arg1 & 64) {
      // This is crazy offset.  A 2x8 setup is reasonable.
      // Unfortunately, there is no concept property flag
      // that calls for a 4x4 or a 2x8.  The flag for this
      // concept just called for a 4x4.  That did no harm, but
      // we have to consider expanding to a 2x8.  If the current
      // setup is not a 4x4, we try to make a 2x8.  If that fails,
      // there will be trouble ahead.
      if (tempsetup.kind != s4x4)
         tempsetup.do_matrix_expansion(CONCPROP__NEEDK_2X8, false);
   }

   int rot = tempsetup.rotation & 1;
   int spec_conc_rot = 1;

   // After taking care of crazy diamonds, decide which way we are going to divide, once only.

   if (tempsetup.kind == s4dmd) {
      kk = s4dmd;
      offsetmapcode = MAPCODE(s_qtag,2,MPKIND__SPLIT,0);
   }
   else if (tempsetup.kind == s4ptpd) {
      kk = s4ptpd;
      offsetmapcode = MAPCODE(s_ptpd,2,MPKIND__SPLIT,0);
   }
   else if ((parseptr->concept->arg1 & 7) < 4) {
      // This is {crazy phantom / crazy offset} C/L/W.  64 bit tells which.

      if ((global_tbonetest & 011) == 011) fail("People are T-boned -- try using 'standard'.");

      if (tempsetup.kind == s4x4) {
         rot = (global_tbonetest ^ parseptr->concept->arg1) & 1;
      }
      else
         kk = s2x8;

      // We have now processed any "standard" information to determine how to split the setup.
      // If this was done nontrivially, we now have to shut off the test that we will do in the
      // divided setup -- that test would fail.  Also, we do not allow "waves"; only "lines" or
      // "columns".

      if ((orig_tbonetest & 011) == 011) {
         tempsetup.cmd.cmd_misc_flags &= ~CMD_MISC__VERIFY_MASK;
         if ((parseptr->concept->arg1 & 7) == 3)
            fail("Don't use 'crazy waves' with standard; use 'crazy lines'.");
      }

      if (parseptr->concept->arg1 & 64) {
         // Crazy offset C/L/W.
         phanstuff = phantest_only_one_pair;
         specialmapcode = MAPCODE(s1x4,2,MPKIND__OFFS_BOTH_SINGLEV,1);

         // Look for the case of crazy offset CLW in a 2x8.
         if (tempsetup.kind != s4x4)
            offsetmapcode = MAPCODE(s1x4,4,MPKIND__OFFS_BOTH_SINGLEV,0);
         else
            offsetmapcode = MAPCODE(s1x4,4,MPKIND__OFFS_BOTH_SINGLEH,1);
      }
      else {
         // Crazy phantom C/L/W.  We will use a map that stacks the setups vertically.
         offsetmapcode = MAPCODE(s2x4,2,MPKIND__SPLIT,1);
         spec_conc_rot = 0;   // Undo the effect of this for the center CLW.
         rot++;
      }
   }
   else if ((parseptr->concept->arg1 & 7) == 4) {
      // This is {crazy phantom / crazy diagonal} boxes.  64 bit tells which.

      if (parseptr->concept->arg1 & 64) {   // Crazy diagonal boxes.
         specialmapcode = MAPCODE(s2x2,2,MPKIND__OFFS_BOTH_SINGLEV,0);

         if (tempsetup.kind != s4x4) {
            kk = s2x8;
            offsetmapcode = MAPCODE(s2x2,4,MPKIND__OFFS_BOTH_SINGLEH,0);
         }
         else {
            // We do an incredibly simple test of which way the 4x4 is oriented.
            // A rigorous test of the exact occupation will be made later.
            rot = ((global_livemask == 0x3A3A) || (global_livemask == 0xC5C5)) ?
               1 : 0;
            offsetmapcode = MAPCODE(s2x2,4,MPKIND__OFFS_BOTH_SINGLEV,0);
         }
      }
      else {
         kk = s2x8;
         offsetmapcode = MAPCODE(s2x4,2,MPKIND__SPLIT,0);
      }
   }
   else if ((parseptr->concept->arg1 & 7) == 5)
      fail("Can't use crazy diamonds here.");

   uint32_t finalresultflagsmisc = 0;

   // This setup rotation stuff is complicated.  What we do may be
   // different for "each side" and "centers".

   // Flip the setup around and recanonicalize.
   // We have to do it in pieces like this, because of the test
   // of tempsetup.rotation that will happen a few lines below.
   tempsetup.rotation += rot;

   for (i=0 ; i<craziness; i++) {
      int ctrflag = (i ^ reverseness) & 1;
      canonicalize_rotation(&tempsetup);

      // Check the validity of the setup each time for boxes/diamonds,
      // or first time only for C/L/W.
      // But allow 2x8 -> 4x4 transition for later parts.
      if ((i==0 ||
           !((parseptr->concept->arg1 & 7) < 4 ||
             (parseptr->concept->arg1 & 64) != 0 ||
             (kk == s2x8 && tempsetup.kind == s4x4) ||
             (kk == s4dmd && tempsetup.kind == s4ptpd) ||
             (kk == s4ptpd && tempsetup.kind == s4dmd))) &&
          (tempsetup.kind != kk || tempsetup.rotation != 0))
         fail("Can't do crazy phantom or offset in this setup.");

      if (ctrflag) {
         // Do it in the center.
         // Do special check for crazy offset.
         tempsetup.rotation += spec_conc_rot;
         canonicalize_rotation(&tempsetup);
         if ((parseptr->concept->arg1 & 64) &&
             tempsetup.kind != s2x8 && tempsetup.kind != s4x4)
            fail("Can't do crazy offset with this shape-changer.");
         concentric_move(&tempsetup, (setup_command *) 0, &tempsetup.cmd,
                         schema_in_out_quad, 0, 0, true, false, specialmapcode, result);

         // Concentric_move sometimes gets carried away.  Maybe should fix same.
         if (result->kind == s4x6) {
            normalize_setup(result, simple_normalize, qtag_compress);
            result->do_matrix_expansion(CONCPROP__NEEDK_2X8, true);
         }

         result->rotation -= spec_conc_rot;
      }
      else                              // Do it on each side.
         divided_setup_move(&tempsetup, offsetmapcode, phanstuff, true, result);

      finalresultflagsmisc |= result->result_flags.misc;
      tempsetup = *result;
      tempsetup.cmd = cmd;    // Get a fresh copy of the command for next time.
   }

   // Flip the setup back.  No need to canonicalize.
   result->rotation -= rot;
   // The split-axis bits are gone.  If someone needs them, we have work to do.
   result->result_flags.misc = finalresultflagsmisc;
   result->result_flags.clear_split_info();
}



static void do_concept_fan(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   uint32_t finalresultflagsmisc = 0;
   /* This is a huge amount of kludgy stuff shoveled in from a variety of sources.
      It needs to be cleaned up and thought about. */

   final_and_herit_flags new_final_concepts;
   new_final_concepts.clear_all_herit_and_final_bits();
   const parse_block *parseptrcopy;
   call_with_name *callspec;

   parseptrcopy = process_final_concepts(parseptr->next, true, &new_final_concepts, true, false);

   if (new_final_concepts.test_for_any_herit_or_final_bit() ||
       parseptrcopy->concept->kind > marker_end_of_list)
      fail("Can't do \"fan\" followed by another concept or modifier.");

   callspec = parseptrcopy->call;

   if (!callspec || !(callspec->the_defn.callflagsf & CFLAG2_CAN_BE_FAN))
      fail("Can't do \"fan\" with this call.");

   // Step to a wave if necessary.  This is actually only needed for the "yoyo" concept.
   // The "fan" concept could take care of itself later.  However, we do them both here.

   if (!(ss->cmd.cmd_misc_flags & (CMD_MISC__NO_STEP_TO_WAVE | CMD_MISC__ALREADY_STEPPED)) &&
       (callspec->the_defn.callflags1 & CFLAG1_STEP_REAR_MASK) == CFLAG1_STEP_TO_WAVE) {
      ss->cmd.cmd_misc_flags |= CMD_MISC__ALREADY_STEPPED;  // Can only do it once.
      ss->touch_or_rear_back(false, callspec->the_defn.callflags1);
   }

   setup tempsetup = *ss;

   // Normally, set the fractionalize field to start with the second part.
   // But if we have been requested to do a specific part number of "fan <call>",
   // just add one to the part number and do the call.

   tempsetup.cmd = ss->cmd;

   if (tempsetup.cmd.cmd_fraction.is_null()) {
      tempsetup.cmd.cmd_fraction.set_to_null_with_flags(
         FRACS(CMD_FRAC_CODE_FROMTOREV,2,0));
   }
   else if (tempsetup.cmd.cmd_fraction.is_null_with_masked_flags(
               CMD_FRAC_CODE_MASK | CMD_FRAC_REVERSE, 0))
      tempsetup.cmd.cmd_fraction.flags += CMD_FRAC_PART_BIT;
   else
      fail("Sorry, this concept can't be fractionalized this way.");

   tempsetup.cmd.prior_elongation_bits = 0;
   tempsetup.cmd.prior_expire_bits = 0;
   move(&tempsetup, false, result);
   finalresultflagsmisc |= result->result_flags.misc;
   result->result_flags.misc = finalresultflagsmisc & ~3;
}

void nose_move(
   setup *ss,
   bool everyone,
   selector_kind who,
   direction_kind where,
   setup *result) THROW_DECL
{
   who_list saved_selector = current_options.who;
   current_options.who.who[0] = who;

   int n = attr::slimit(ss);
   if (n < 0) fail("Sorry, can't do nose starting in this setup.");
   const coordrec *coordptr = setup_attrs[ss->kind].nice_setup_coords;

   int initial_turn[32];

   int i;
   for (i=0; i<=n; i++) {           // Execute the required turning, and remember same.
      uint32_t p = ss->people[i].id1;
      if (p & BIT_PERSON) {
         int turn;
         int my_coord;
         bool select = everyone || selectp(ss, i);
         switch (where)
         {
         case direction_left:
            turn = 3;
            break;
         case direction_right:
            turn = 1;
            break;
         case direction_back:
            turn = 2;
            break;
         case direction_in:
         case direction_out:
            if (!coordptr) fail("Can't do this in this formation.");
            my_coord = (p & 1) ? coordptr->yca[i] : coordptr->xca[i];
            if (my_coord == 0) fail("Person is in center column.");
            if (where == direction_out) my_coord = -my_coord;
            if ((p+1) & 2) my_coord = -my_coord;
            turn = my_coord < 0 ? 1 : 3;
            break;
         default: fail("Illegal direction.");
         }

         if (select) ss->people[i].id1 = rotperson(p, turn * 011);
         initial_turn[(p >> 6) & 037] = select ? turn : 0;
      }
   }

   current_options.who = saved_selector;

   ss->update_id_bits();
   move(ss, false, result);

   setup_kind kk = result->kind;

   if (kk == s_dead_concentric)
      kk = result->inner.skind;

   n = attr::klimit(kk);
   if (n < 0) fail("Sorry, can't do nose going to this setup.");

   for (i=0; i<=n; i++) {      // Undo the turning.
      uint32_t p = result->people[i].id1;
      if (p & BIT_PERSON) {
         result->people[i].id1 = rotperson(p, ((- initial_turn[(p >> 6) & 037]) & 3) * 011);
      }
   }
}

void stable_move(
   setup *ss,
   bool fractional,
   bool everyone,
   int howfar,
   selector_kind who,
   setup *result) THROW_DECL
{
   int orig_rotation = ss->rotation;
   setup_kind kk;
   int rot;
   int n = attr::slimit(ss);

   who_list saved_selector = current_options.who;
   current_options.who.who[0] = who;

   if (ss->kind == nothing)
      goto dust_to_dust;

   if (n < 0) fail("Sorry, can't do stable starting in this setup.");

   if (fractional && howfar > 4)
      fail("Can't do fractional stable more than 4/4.");

   howfar <<= 1;    // Calibrated in eighths.

   uint32_t directions[32];
   bool selected[32];

   int i;
   for (i=0; i<=n; i++) {           // Save current facing directions.
      uint32_t p = ss->people[i].id1;
      if (p & BIT_PERSON) {
         directions[(p >> 6) & 037] = p;
         selected[(p >> 6) & 037] = everyone || selectp(ss, i);
         if (fractional) {
            if (p & STABLE_ALL_MASK)
               fail("Sorry, can't nest fractional stable/twosome.");
            ss->people[i].id1 |= STABLE_ENAB | (STABLE_REMBIT * howfar);
         }
      }
   }

   current_options.who = saved_selector;

   move(ss, false, result);
   rot = ((orig_rotation - result->rotation) & 3) * 011;

   kk = result->kind;

   if (kk == s_dead_concentric)
      kk = result->inner.skind;

   if (kk == nothing)
      goto dust_to_dust;

   n = attr::klimit(kk);
   if (n < 0) fail("Sorry, can't do stable going to this setup.");

   for (i=0; i<=n; i++) {      // Restore facing directions of selected people.
      uint32_t p = result->people[i].id1;
      if (p & BIT_PERSON) {
         if (selected[(p >> 6) & 037]) {
            uint32_t stop_amount;
            if (fractional) {
               stop_amount = ((p & STABLE_VLMASK) / STABLE_VLBIT) - ((p & STABLE_VRMASK) / STABLE_VRBIT);

               if (stop_amount & 1)
                  fail("Sorry, can't do 1/8 stable.");

               if (!(p & STABLE_ENAB))
                  fail("fractional stable not supported for this call.");
               p = rotperson(p, ((stop_amount/2) & 3) * 011);
            }
            else {
               stop_amount = 1;   // So that roll will be turned off.

               p = rotperson(
                  (p & ~(d_mask | STABLE_ALL_MASK)) |
                  (directions[(p >> 6) & 037] & (d_mask | STABLE_ALL_MASK)),
                  rot);
            }

            // If this was fractional stable, roll is turned off
            // only if the person was actually stopped from turning.
            if (stop_amount != 0) p = (p & ~ROLL_DIRMASK) | ROLL_IS_M;
         }

         if (fractional) p &= ~STABLE_ALL_MASK;

         result->people[i].id1 = p;
      }
   }

   return;

 dust_to_dust:
   clear_result_flags(result);
   result->kind = nothing;
}

static void do_concept_nose(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   if (process_brute_force_mxn(ss, parseptr, do_concept_nose, result)) return;

   if (ss->cmd.cmd_final_flags.test_for_any_herit_or_final_bit())
      fail("Illegal modifier before \"nose\".");

   nose_move(ss,
             parseptr->concept->arg1 == 0,
             parseptr->options.who.who[0],
             parseptr->options.where,
             result);
}


static void do_concept_stable(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   if (process_brute_force_mxn(ss, parseptr, do_concept_stable, result)) return;

   if (ss->cmd.cmd_final_flags.test_for_any_herit_or_final_bit())
      fail("Illegal modifier before \"stable\".");

   stable_move(ss,
               parseptr->concept->arg2 != 0,
               parseptr->concept->arg1 == 0,
               parseptr->options.number_fields,
               parseptr->options.who.who[0],
               result);
}



static void do_concept_emulate(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   int i, j, n, m, rot;
   setup res1;

   setup a1 = *ss;

   move(&a1, false, &res1);

   *result = res1;      // Get all the other fields.

   result->result_flags.misc |= RESULTFLAG__FORCE_SPOTS_ALWAYS;
   result->kind = ss->kind;
   result->rotation = ss->rotation;
   result->eighth_rotation = 0;

   n = attr::slimit(ss);
   m = attr::klimit(res1.kind);
   if (n < 0 || m < 0) fail("Sorry, can't do emulate in this setup.");

   rot = ((res1.rotation-result->rotation) & 3) * 011;

   for (i=0; i<=n; i++) {
      uint32_t p = copy_person(result, i, ss, i);

      if (p & BIT_PERSON) {
         for (j=0; j<=m; j++) {
            uint32_t q = res1.people[j].id1;
            if ((q & BIT_PERSON) && ((q ^ p) & XPID_MASK) == 0) {
               result->people[i].id1 &= ~(NSLIDE_ROLL_MASK | STABLE_ALL_MASK | 0x3F);
               result->people[i].id1 |= rotperson(q, rot) & (NSLIDE_ROLL_MASK | STABLE_ALL_MASK | 0x3F);
               goto did_it;
            }
         }
         fail("Lost someone during emulate call.");
         did_it: ;
      }
   }
}

static void do_concept_checkerboard(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // arg1 = setup (1x4/2x2/dmd)
   // arg2 = 0 : checkersetup
   //        1 : shadow setup
   //        2 : orbitsetup
   //        3 : twin orbitsetup
   //        4 : left orbitsetup
   //        add 8 for selected people (checker only)

   struct CKBDtabletype {
      setup_kind K;
      uint32_t L;   // the actual livemask must not intersect this
                    // (only to pick out two types of C1 phantoms.)
      uint32_t M;   // 11 means direction is required (and person must be live);
                    // 00 means direction is forbidden.
      uint32_t V;   // what the required/forbidden direction is.
      uint32_t dup; // Direction of 1st 2 trading people, typically north
      uint32_t ddn; // Direction of last 2 trading people, typically south
      uint32_t rot; // People are rotated.
      int8_t mape[4];   // The people who will trade.
      int8_t mapl[4];   // Where the others go, checkerboard.
      int8_t mapb[4];   // Where the others go, checkerbox.
      int8_t mapd[4];   // Where the others go, checkerdiamond.
      int8_t mapr[4];   // Alternate mapl for orientation changer.
   };

   static const CKBDtabletype CKBDtable[] = {
      // First 5 must be as shown, because "so-and-so preferred" depends on this.
      {s2x4, 0,              0xCCCC,     0x00AA,      // outfacers are as if in RWV
       d_north, d_south, 0, {0, 2, 4, 6}, {7, 1, 3, 5}, {1, 3, 5, 7}, {7, 1, 3, 5}, {-1}},
      {s2x4, 0,              0x3333,     0x00AA,      // outfacers are as if in LWV
       d_north, d_south, 0, {1, 3, 5, 7}, {0, 6, 4, 2}, {0, 2, 4, 6}, {0, 2, 4, 6}, {-1}},
      {s2x4, 0,              0xF0F0,     0x00AA,      // outfacers are as if in R2FL
       d_north, d_south, 0, {0, 1, 4, 5}, {7, 6, 3, 2}, {2, 3, 6, 7}, {7, 2, 3, 6}, {14, 4, 6, 12}},
      {s2x4, 0,              0x0F0F,     0x00AA,      // outfacers are as if in L2FL
       d_north, d_south, 0, {2, 3, 6, 7}, {0, 1, 4, 5}, {0, 1, 4, 5}, {0, 1, 4, 5}, {11, 1, 3, 9}},
      {s2x4, 0,              0xC3C3,     0x00AA,      // outfacers are ends
       d_north, d_south, 0, {0, 3, 4, 7}, {-1, -1, -1, -1}, {1, 2, 5, 6}, {-1, -1, -1, -1}, {-1}},
      {s2x4, 0,              0xF00F,     0x00AA,      // unsymmetrical, outfacers on left
       d_north, d_south, 0, {0, 1, 6, 7}, {-1, -1, -1, -1}, {2, 3, 4, 5}, {-1, -1, -1, -1}, {-1}},
      {s2x4, 0,              0x0FF0,     0x00AA,      // unsymmetrical, outfacers on right
       d_north, d_south, 0, {2, 3, 4, 5}, {-1, -1, -1, -1}, {0, 1, 6, 7}, {-1, -1, -1, -1}, {-1}},

      // 2X6, left offset
      {s2x6, 0,              0xCC0CC0,     0x000AA0,      // outfacers are as if in RWV
       d_north, d_south, 0, {0, 2, 6, 8}, {1, 9, 7, 3}, {1, 3, 7, 9}, {1, 3, 7, 9}, {-1}},
      {s2x6, 0,              0x330330,     0x000AA0,      // outfacers are as if in LWV
       d_north, d_south, 0, {1, 3, 7, 9}, {0, 2, 6, 8}, {0, 2, 6, 8}, {0, 2, 6, 8}, {-1}},
      {s2x6, 0,              0xF00F00,     0x000AA0,      // outfacers are as if in R2FL
       d_north, d_south, 0, {0, 1, 6, 7}, {-1, -1, -1, -1}, {2, 3, 8, 9}, {-1, -1, -1, -1}, {-1}},
      {s2x6, 0,              0x0F00F0,     0x000AA0,      // outfacers are as if in L2FL
       d_north, d_south, 0, {2, 3, 8, 9}, {0, 1, 6, 7}, {0, 1, 6, 7}, {-1, -1, -1, -1}, {-1}},
      {s2x6, 0,              0xC30C30,     0x000AA0,      // outfacers are ends
       d_north, d_south, 0, {0, 3, 6, 9}, {1, 2, 7, 8}, {1, 2, 7, 8}, {-1, -1, -1, -1}, {-1}},

      // 2X6, right offset
      {s2x6, 0,              0x0CC0CC,     0x0000AA,      // outfacers are as if in RWV
       d_north, d_south, 0, {2, 4, 8, 10}, {11, 9, 5, 3}, {3, 5, 9, 11}, {11, 3, 5, 9}, {-1}},
      {s2x6, 0,              0x033033,     0x0000AA,      // outfacers are as if in LWV
       d_north, d_south, 0, {3, 5, 9, 11}, {10, 2, 4, 8}, {2, 4, 8, 10}, {10, 2, 4, 8}, {-1}},
      {s2x6, 0,              0x0F00F0,     0x0000AA,      // outfacers are as if in R2FL
       d_north, d_south, 0, {2, 3, 8, 9}, {11, 10, 5, 4}, {4, 5, 10, 11}, {-1, -1, -1, -1}, {-1}},
      {s2x6, 0,              0x00F00F,     0x0000AA,      // outfacers are as if in L2FL
       d_north, d_south, 0, {4, 5, 10, 11}, {-1, -1, -1, -1}, {2, 3, 8, 9}, {-1, -1, -1, -1}, {-1}},
      {s2x6, 0,              0x0C30C3,     0x0000AA,      // outfacers are ends
       d_north, d_south, 0, {2, 5, 8, 11}, {10, 9, 4, 3}, {3, 4, 9, 10}, {-1, -1, -1, -1}, {-1}},

      {s_c1phan, 0x33333333, 0xCC00CC00, 0x004488CC,  // C1 phantoms, outfacers N/S
       d_north, d_south, 033, {0, 2, 8, 10}, {4, 6, 12, 14}, {4, 6, 12, 14}, {4, 6, 12, 14}, {15, 5, 7, 13}},
      {s_c1phan, 0x33333333, 0x00CC00CC, 0x004488CC,  // C1 phantoms, outfacers E/W
       d_east, d_west, 0, {4, 6, 12, 14}, {0, 2, 8, 10}, {0, 2, 8, 10}, {0, 2, 8, 10}, {11, 1, 3, 9}},
      {s_c1phan, 0xCCCCCCCC, 0x00330033, 0x33001122,  // other C1 phantoms, N/S
       d_north, d_south, 033, {7, 5, 15, 13}, {1, 3, 9, 11}, {11, 9, 3, 1}, {1, 11, 9, 3}, {2, 8, 10, 0}},
      {s_c1phan, 0xCCCCCCCC, 0x33003300, 0x33001122,  // other C1 phantoms, E/W
       d_west, d_east, 0, {3, 1, 11, 9}, {13, 15, 5, 7}, {7, 5, 15, 13}, {13, 7, 5, 15}, {14, 4, 6, 12}},
      {nothing}};

   int i;
   int offset = -1;
   ss->clear_all_overcasts();

   setup_kind kn = (setup_kind) parseptr->concept->arg1;

   if (ss->cmd.cmd_misc3_flags & CMD_MISC3__META_NOCMD)
      warn(warn__meta_on_xconc);

   result->clear_people();

   if (parseptr->concept->arg2 == 1) {
      // This is "shadow <setup>"

      if ((kn != s2x2 || ss->kind != s2x4) &&
          (kn != s1x4 || (ss->kind != s_qtag && ss->kind != s_bone)) &&
          (kn != sdmd || (ss->kind != s_hrglass && ss->kind != s_dhrglass)))
         fail("Not in correct setup for 'shadow line/box/diamond' concept.");

      setup_command subsid_cmd = ss->cmd;
      subsid_cmd.parseptr = (parse_block *) 0;
      subsid_cmd.callspec = base_calls[base_call_ends_shadow];
      subsid_cmd.cmd_fraction.set_to_null();
      subsid_cmd.cmd_final_flags.clear_all_herit_and_final_bits();
      concentric_move(ss, &ss->cmd, &subsid_cmd, schema_concentric, 0,
                      DFM1_CONC_DEMAND_LINES | DFM1_CONC_FORCE_COLUMNS, true, false, ~0U, result);
      result->clear_all_overcasts();
      return;
   }

   // This is "checker/orbit <setup>"

   // First, we allow these to be done in a C1 phantom setup under certain
   // conditions.  If it's the equivalent "pinwheel" 4x4, fix it.

   turn_4x4_pinwheel_into_c1_phantom(ss);

   if (parseptr->concept->arg2 & 8) {
      // This is "so-and-so preferred for the trade, checkerboard".
      if (ss->kind != s2x4) fail("Must have a 2x4 setup for 'checker' concept.");

      if      (global_selectmask == 0x55)
         offset = 0;
      else if (global_selectmask == 0xAA)
         offset = 1;
      else if (global_selectmask == 0x33)
         offset = 2;
      else if (global_selectmask == 0xCC)
         offset = 3;
      else if (global_selectmask == 0x99)
         offset = 4;
      else fail("Can't select these people.");
   }
   else {
      uint32_t D, L;
      ss->big_endian_get_directions32(D, L);

      for (i=0; CKBDtable[i].K != nothing ; i++) {
         if (CKBDtable[i].K != ss->kind ||
             CKBDtable[i].L & L) continue;
         uint32_t M = CKBDtable[i].M;
         // Find out who is facing out for the trade, by XOR'ing the person's
         // direction (in D) with the required direction from the table
         // (the V field).  The result will be 00 for people facing out.
         // If the spot is unoccupied, set the bits to 11 by OR'ing
         // the complement of L.  That way, such a phantom will be
         // considered not to be facing out for the trade.
         // (That means that people trading must be real.  We will
         // allow exceptions to this below, where we check for
         // "assume waves".)
         uint32_t Q = ~L | (CKBDtable[i].V ^ D);

         // Now "M" is 11 for people who must be facing out for the trade,
         // and 00 for the other people, who are therefore forbidden to
         // be facing out.  The first part of the next conditional tests
         // that the M=11 spots all have Q=00, which means that they are
         // live and facing out.  The second part tests that the M=00
         // spots all have Q != 00, which means that they are phantoms
         // or are facing some other way.
         if ((M & Q) == 0 &&
             (((Q & 0x55555555U) << 1) | Q | M | 0x55555555U) == ~0U) {
            offset = i;
            break;
         }
      }

      // If we don't have the required outfacers, but a suitable
      // "assume" command was given, that's good enough.

      if (offset < 0 &&
          ss->kind == s2x4 &&
          ss->cmd.cmd_assume.assumption == cr_wave_only &&
          ss->cmd.cmd_assume.assump_col == 0 &&
          ss->cmd.cmd_assume.assump_negate == 0) {
         if (((L & ~D) | 0xAAAAAAAA) != 0xAAAAAAAA) {
            uint32_t DR = D ^ 0x8822;
            uint32_t DL = D ^ 0x2288;
            if (((DR | ~L | ((DR & 0x55555555) << 1)) & 0xAAAAAAAA) == 0xAAAAAAAA)
               offset = 0;
            else if (((DL | ~L | ((DL & 0x55555555) << 1)) & 0xAAAAAAAA) == 0xAAAAAAAA)
               offset = 1;
         }
      }
   }

   if (offset < 0)
      fail("Can't identify checkerboard people.");

   const CKBDtabletype *CKBDthing = &CKBDtable[offset];

   const int8_t *mapeptr = CKBDthing->mape;

   const int8_t *map_ptr =
      (kn == s1x4) ? CKBDthing->mapl :
      (kn == s2x2) ? CKBDthing->mapb :
      CKBDthing->mapd;

   // If we have inverted lines with the ends facing out,
   // it is only legal with checkerbox.
   if (*map_ptr < 0) fail("Can't identify checkerboard people.");

   uint32_t t;

   if (((t = ss->people[mapeptr[0]].id1) && (t & d_mask) != CKBDthing->dup) ||
       ((t = ss->people[mapeptr[1]].id1) && (t & d_mask) != CKBDthing->dup) ||
       ((t = ss->people[mapeptr[2]].id1) && (t & d_mask) != CKBDthing->ddn) ||
       ((t = ss->people[mapeptr[3]].id1) && (t & d_mask) != CKBDthing->ddn))
      fail("Selected people are not facing out.");

   struct ck_thing {
      int src;
      int dir;
      int stability_info;
      int roll_info;
   };

   ck_thing *board_table;

   ck_thing checkerboard_table[] = {
      {1, 2, STB_A,             PERSON_MOVED | ROLL_IS_L},
      {0, 2, STB_A+STB_REVERSE, PERSON_MOVED | ROLL_IS_R},
      {3, 2, STB_A,             PERSON_MOVED | ROLL_IS_L},
      {2, 2, STB_A+STB_REVERSE, PERSON_MOVED | ROLL_IS_R}
   };

   ck_thing orbitboard_table[] = {
      {3, 0, STB_A+STB_REVERSE, PERSON_MOVED | ROLL_IS_R},
      {0, 2, STB_A+STB_REVERSE, PERSON_MOVED | ROLL_IS_R},
      {1, 0, STB_A+STB_REVERSE, PERSON_MOVED | ROLL_IS_R},
      {2, 2, STB_A+STB_REVERSE, PERSON_MOVED | ROLL_IS_R}
   };

   ck_thing leftorbitboard_table[] = {
      {1, 2, STB_A,             PERSON_MOVED | ROLL_IS_L},
      {2, 0, STB_A,             PERSON_MOVED | ROLL_IS_L},
      {3, 2, STB_A,             PERSON_MOVED | ROLL_IS_L},
      {0, 0, STB_A,             PERSON_MOVED | ROLL_IS_L}
   };

   ck_thing twinorbitboard_table[] = {
      {3, 0, STB_A+STB_REVERSE, PERSON_MOVED | ROLL_IS_R},
      {2, 0, STB_A,             PERSON_MOVED | ROLL_IS_L},
      {1, 0, STB_A+STB_REVERSE, PERSON_MOVED | ROLL_IS_R},
      {0, 0, STB_A,             PERSON_MOVED | ROLL_IS_L}
   };

   // Move the people who simply orbit, filling in their roll and stability info.

   switch (parseptr->concept->arg2 & 7) {
   case 2:         // orbitboard
      if (offset > 1) fail("Can't find orbiting people.");
      board_table = orbitboard_table;
      break;
   case 4:         // left orbitboard
      if (offset > 1) fail("Can't find orbiting people.");
      board_table = leftorbitboard_table;
      break;
   case 3:         // twin orbitboard
      if (offset > 1) fail("Can't find orbiting people.");
      board_table = twinorbitboard_table;
      break;
   default:        // 0 = checkerboard/box/diamond
      board_table = checkerboard_table;
      break;
   }

   for (int j=0 ; j<4 ; j++) {
      ck_thing *this_item = &board_table[j];

      copy_rot(result, mapeptr[j], ss, mapeptr[this_item->src], this_item->dir * 011);
      uint32_t *personp = &result->people[mapeptr[j]].id1;
      if (*personp) {
         if (*personp & STABLE_ENAB)
            do_stability(personp, this_item->stability_info, this_item->dir, false);
         *personp = (*personp & (~NSLIDE_ROLL_MASK)) | this_item->roll_info;
      }
   }

   setup a1 = *ss;
   for (i=0; i<4; i++) copy_rot(&a1, i, ss, map_ptr[i], CKBDthing->rot);

   a1.kind = kn;
   a1.cmd.cmd_misc_flags |= CMD_MISC__DISTORTED;
   a1.rotation = 0;
   a1.eighth_rotation = 0;
   a1.cmd.cmd_assume.assumption = cr_none;
   a1.update_id_bits();
   setup res1;
   move(&a1, false, &res1);

   map_ptr =
      (res1.kind == s1x4) ? CKBDthing->mapl :
      (res1.kind == s2x2) ? CKBDthing->mapb :
      (res1.kind == sdmd) ? CKBDthing->mapd : 0;

   if (!map_ptr) fail("Don't recognize ending setup after 'checker' call.");

   result->kind = ss->kind;
   result->rotation = 0;
   result->eighth_rotation = 0;
   result->result_flags = res1.result_flags;

   if (res1.rotation != 0) {
      // Look at the rotation coming out of the move.  We know res1 is a 1x4 or a
      // diamond.  If it's a 1x4, we can handle incoming phantom setups--just fill
      // the result as a phantom setup that happens to be a 2x4, and it will get trimmed
      // later.  Otherwise, allow any rotation.  But we give a warning for peculiarly
      // oriented diamonds.

      if (res1.kind == sdmd) {
         // Effectively do a "slim down" on the diamond and hope that's what the caller wants.
         warn(warn_controversial);
      }
      else if (ss->kind == s_c1phan) {
         // If incoming setup was a phantom setup, just put people back, using a modified table.
         // The result will be occupied as a 2x4, and will get reduced to same.
         map_ptr = CKBDthing->mapr;
      }
      else {
         // Incoming setup was a 2x4; turn it into a c1phan.  The 2x4 result has the
         // people who traded; move them into their proper place in the c1phan.  The
         // final result will not be reduced; it will stay as a c1phan.
         setup temp = *result;
         temp.kind = s_c1phan;
         temp.clear_people();
         scatter(&temp, result, map_c1_phan.map1, 7, 0);
         *result = temp;
         map_ptr = CKBDthing->mapr;
         // Only legal for 2FL cases.
         if (*map_ptr < 0) fail("'Checker' call went to 1x4 setup oriented the wrong way.");
      }
   }

   if (*map_ptr < 0) fail("Can't do this.");
   int rot = ((res1.rotation - CKBDthing->rot) & 3) * 011;

   for (i=0; i<4; i++) copy_rot(result, map_ptr[i], &res1, (i-res1.rotation) & 3, rot);

   reinstate_rotation(ss, result);
   result->clear_all_overcasts();
}


static void do_concept_checkpoint(
   setup *ss,
   parse_block *parseptr,
   setup *result)
{
   int reverseness = parseptr->concept->arg1;

   // Don't all this fancy stuff for "checkpoint it by it".
   if (reverseness != 2) {
      if (process_brute_force_mxn(ss, parseptr, do_concept_checkpoint, result)) return;

      if (ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_REVERSE)) {
         if (reverseness) fail("Redundant 'REVERSE' modifiers.");
         reverseness = 1;
      }

      ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_REVERSE);
      // We don't allow other flags, like "cross".
      if (ss->cmd.cmd_final_flags.test_for_any_herit_or_final_bit())
         fail("Illegal modifier before \"checkpoint\".");
   }

   if (ss->cmd.cmd_misc3_flags & CMD_MISC3__META_NOCMD)
      warn(warn__meta_on_xconc);

   setup_command this_cmd = ss->cmd;

   if ((this_cmd.cmd_misc3_flags &
        (CMD_MISC3__PUT_FRAC_ON_FIRST|CMD_MISC3__RESTRAIN_CRAZINESS)) ==
       CMD_MISC3__PUT_FRAC_ON_FIRST) {

      // Curried meta-concept, as in "finally checkpoint recycle
      // by 1/4 thru".  Take the fraction info off the first
      // call.  In this example, the 1/4 thru is affected but the
      // recycle is not.
      this_cmd.cmd_misc3_flags &= ~CMD_MISC3__PUT_FRAC_ON_FIRST;
      if (reverseness != 2)
         this_cmd.cmd_fraction.set_to_null();
   }
   else {
      // If not under a meta-concept, we don't allow fractionalization.
      // You can't do 1/2 of the moving of the checkpointers to the outside.
      if (!this_cmd.cmd_fraction.is_null())
         fail("Can't do this.");
   }

   setup_command subsid_cmd = ss->cmd;
   subsid_cmd.parseptr = parseptr->subsidiary_root;

   // The "dfm_conc_force_otherway" flag forces Callerlab interpretation:
   // If checkpointers go from 2x2 to 2x2, this is clear.
   // If checkpointers go from 1x4 to 2x2, "dfm_conc_force_otherway" forces
   //    the Callerlab rule in preference to the "parallel_concentric_end" property
   //    on the call.

   if (reverseness == 2) {
      concentric_move(ss, &this_cmd, &this_cmd, schema_checkpoint,
                      0, DFM1_CONC_FORCE_OTHERWAY, true, false, ~0U, result);
   }
   else if (reverseness)
      concentric_move(ss, &this_cmd, &subsid_cmd, schema_rev_checkpoint_concept,
                      0, 0, true, false, ~0U, result);
   else
      concentric_move(ss, &subsid_cmd, &this_cmd, schema_checkpoint,
                      0, DFM1_CONC_FORCE_OTHERWAY, true, false, ~0U, result);
}


static void copy_cmd_preserve_elong_and_expire(const setup *ss, setup *result, bool only_elong = false)
{
   uint32_t save_elongation = result->cmd.prior_elongation_bits;
   uint32_t save_expire = result->cmd.prior_expire_bits;
   result->cmd = ss->cmd;
   result->cmd.prior_elongation_bits = save_elongation;
   if (!only_elong) result->cmd.prior_expire_bits = save_expire;
}


static bool prepare_for_call_under_repetition(
   const fraction_info *yyy,
   int & fetch_number,
   setup *ss,
   setup *result) THROW_DECL
{
   if (yyy->m_reverse_order) {
      if (yyy->m_fetch_index < 0) return true;
   }
   else {
      if (yyy->m_fetch_index >= yyy->m_fetch_total) return true;
   }

   yyy->demand_this_part_exists();

   copy_cmd_preserve_elong_and_expire(ss, result);

   if (!(result->result_flags.misc & (RESULTFLAG__NO_REEVALUATE|RESULTFLAG__REALLY_NO_REEVALUATE)))
      result->update_id_bits();

   //   result->result_flags.misc &= ~RESULTFLAG__REALLY_NO_REEVALUATE;   NO!!!!!

   // We don't supply these; they get filled in by the call.
   result->cmd.cmd_misc_flags &= ~DFM1_CONCENTRICITY_FLAG_MASK;
   if (!yyy->m_first_call) {
      result->cmd.cmd_misc_flags |= CMD_MISC__NO_CHK_ELONG;
      result->cmd.cmd_assume.assumption = cr_none;
   }

   // Move this stuff out, after the increment.

   if (yyy->m_do_half_of_last_part != 0 &&
       yyy->m_fetch_index+1 == yyy->m_highlimit) {
      result->cmd.cmd_fraction.flags = 0;
      result->cmd.cmd_fraction.fraction = yyy->m_do_half_of_last_part;
   }
   else if (result->cmd.restrained_fraction.fraction) {
      result->cmd.promote_restrained_fraction();
   }
   else
      result->cmd.cmd_fraction.set_to_null();  // No fractions to constituent call.

   fetch_number = yyy->m_fetch_index;
   return false;
}


static void do_call_in_series_simple(setup *result) THROW_DECL
{
   do_call_in_series(result, false, true, false);
}


static void do_call_in_series_and_update_bits(setup *result) THROW_DECL
{
   do_call_in_series(result, false, true, false);
   if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
      result->update_id_bits();
}



static void do_concept_sequential(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   fraction_info zzz(2);

   if (!ss->cmd.cmd_fraction.is_null()) {
      zzz.get_fraction_info(ss->cmd.cmd_fraction,
                            CFLAG1_VISIBLE_FRACTION_MASK / CFLAG1_VISIBLE_FRACTION_BIT,
                            weirdness_off);
   }

   zzz.m_first_call = !zzz.m_reverse_order;

   prepare_for_call_in_series(result, ss);

   assumption_thing fix_next_assumption;

   int remembered_2x2_elongation = 0;
   bool use_incoming_assumption = true;  // Normally turned off after first round; only 1st call get the assumption.

   for (;;) {
      int fetch_number;

      if (prepare_for_call_under_repetition(&zzz, fetch_number, ss, result)) break;
      if (zzz.not_yet_in_active_section()) goto go_to_next_cycle;
      if (zzz.ran_off_active_section()) break;

      if (!use_incoming_assumption) {
         pre_process_seq_assumptions(result, &fix_next_assumption);
      }

      fix_next_assumption.clean();

      final_and_herit_flags finalheritzero;
      finalheritzero.herit = 0ULL;
      finalheritzero.final = (finalflags) 0;

      // The fetch number is 0 or 1.  Depending on which it is, get the proper parse pointer.
      if (fetch_number != 0) result->cmd.parseptr = parseptr->subsidiary_root;

      // And now get the call.  Though it might seem that do_stuff_inside_sequential_call
      // will get it, the assumption-propagating code requires that it already be in place.
      result->cmd.callspec = (result->cmd.parseptr->concept == &concept_mark_end_of_list) ? result->cmd.parseptr->call : 0;

      {
         call_conc_option_state saved_options = current_options;
         current_options = result->cmd.parseptr->options;

         do_stuff_inside_sequential_call(result, 0,
                                         &fix_next_assumption,
                                         &remembered_2x2_elongation,
                                         finalheritzero, 0, false, true, false, false);

         current_options = saved_options;
      }

      zzz.m_first_call = false;

      use_incoming_assumption = false;

      // If we are being asked to do just one part of a call,
      // exit now.  Also, fill in bits in result->result_flags.

      if (zzz.query_instant_stop(result->result_flags.misc)) break;

   go_to_next_cycle:

      // Are we someplace where absolute location bits are known?  That is, are we not
      // under some other operation that could get the bits wrong?  If so, clear those
      // bits and put in new ones.  Just sample one person.

      if (attr::slimit(result) >= 0 && (result->people[0].id3 & ID3_ABSOLUTE_PROXIMITY_BITS) != 0) {
         result->clear_absolute_proximity_and_facing_bits();
         result->put_in_absolute_proximity_and_facing_bits();
      }
      else
         result->clear_absolute_proximity_and_facing_bits();

      // Increment for next cycle.
      zzz.m_fetch_index += zzz.m_subcall_incr;
      zzz.m_client_index += zzz.m_subcall_incr;
   }
}


static void do_concept_special_sequential(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // Values of arg1 are:
   //    part_key_follow_by     (0) - follow it by (call)
   //    part_key_precede_by    (1) - precede it by (call)
   //    part_key_start_with    (2) - start with (call)
   //    part_key_use_nth_part  (3) - use (call) for <Nth> part
   //    part_key_use           (4) - use (call) in
   //    part_key_half_and_half (5) - half and half
   //    part_key_frac_and_frac (6) - I/J and M/N
   //    part_key_use_last_part (7) - use (call) for the last part
   //    part_key_paranoid      (8) - paranoid

   if (parseptr->concept->arg1 == part_key_paranoid) {

      // This is "paranoid".

      struct paranoid_thing {
         setup *presult;
         call_with_name *the_utb;
         uint32_t psaved_last_flagmisc;
         fraction_info pzzz;

         paranoid_thing(setup *result) : presult(result), pzzz(2)
         {};

         void do_subject_call() {
            psaved_last_flagmisc = presult->result_flags.misc &
               (RESULTFLAG__DID_LAST_PART|RESULTFLAG__PARTS_ARE_KNOWN);
            presult->cmd.cmd_fraction.flags &= ~CMD_FRAC_REVERSE;
            if (!(presult->cmd.cmd_fraction.flags & CMD_FRAC_BREAKING_UP))
               presult->cmd.cmd_fraction.fraction = pzzz.get_fracs_for_this_part();
            do_call_in_series_simple(presult);
            presult->result_flags.misc |= psaved_last_flagmisc;
         }

         void do_the_utb() {
            presult->cmd.initialize();
            presult->cmd.parseptr = (parse_block *) 0;
            presult->cmd.callspec = the_utb;
            psaved_last_flagmisc = presult->result_flags.misc &
               (RESULTFLAG__DID_LAST_PART|RESULTFLAG__PARTS_ARE_KNOWN);
            presult->cmd.cmd_fraction.flags &= ~CMD_FRAC_REVERSE;
            if (!(presult->cmd.cmd_fraction.flags & CMD_FRAC_BREAKING_UP))
               presult->cmd.cmd_fraction.fraction = pzzz.get_fracs_for_this_part();
            do_call_in_series_simple(presult);
            presult->result_flags.misc |= psaved_last_flagmisc;
         }
      };

      paranoid_thing P(result);

      if (parseptr->concept->arg2) current_options.who = parseptr->options.who;

      P.the_utb = result->cmd.callspec = (parseptr->concept->arg2) ?
         base_calls[base_call_anyoneuturnback] : base_calls[base_call_uturnback];

      call_with_name *save_subject_call = ss->cmd.parseptr->call;
      who_list saved_selector = current_options.who;

      if (process_brute_force_mxn(ss, parseptr, do_concept_special_sequential, result))
         return;

      if (ss->cmd.cmd_final_flags.test_for_any_herit_or_final_bit())
         fail("Illegal modifier before \"paranoid\".");

      ss->cmd.cmd_misc3_flags |= CMD_MISC3__PUT_FRAC_ON_FIRST;

      if (!ss->cmd.cmd_fraction.is_null() &&
          !(ss->cmd.cmd_misc3_flags & CMD_MISC3__PUT_FRAC_ON_FIRST))
         fail("Can't stack meta or fractional concepts.");

      if (ss->cmd.cmd_fraction.flags & CMD_FRAC_REVERSE)
         P.pzzz.m_reverse_order = true;

      P.pzzz.m_first_call = !P.pzzz.m_reverse_order;

      prepare_for_call_in_series(result, ss);

      if (ss->cmd.cmd_fraction.flags == 0) {
         // Pure fractionalization (or nothing) coming in.

         P.pzzz.get_fraction_info(ss->cmd.cmd_fraction, 7, weirdness_off, (parse_block **) 0);

         P.psaved_last_flagmisc = result->result_flags.misc &
            (RESULTFLAG__DID_LAST_PART|RESULTFLAG__PARTS_ARE_KNOWN);

         result->cmd.cmd_fraction.flags &= ~CMD_FRAC_REVERSE;
         goto try_this;
      }
      else {
         // Special part-analyzing thing coming in.
         // Find out whether we are doing something like "initially stable paranoid swing thru",
         // with an intervening concept, or just "initially paranoid swing thru".

         if (!(ss->cmd.cmd_misc3_flags & CMD_MISC3__PARTS_OVER_THIS_CONCEPT)) {
            // Yes, so the part number applies to the "paranoid" concept itself.

            P.pzzz.get_fraction_info(ss->cmd.cmd_fraction, 7, weirdness_off, (parse_block **) 0);

            if ((ss->cmd.cmd_fraction.fraction == FRAC_FRAC_NULL_VALUE) ||   // User said "finally or secondly".
                ((ss->cmd.cmd_fraction.flags & ~CMD_FRAC_BREAKING_UP) == 0)) // User said "last 1/2".
                result->cmd.cmd_fraction.set_to_null();
         }

      try_this:

         setup_command save_cmd;

         for (;;) {
            if (P.pzzz.not_yet_in_active_section()) goto paranoid_next_cycle;
            if (P.pzzz.ran_off_active_section()) break;

            save_cmd = result->cmd;

            if (P.pzzz.m_fetch_index == 0) {
               P.do_subject_call();
            }
            else {
               P.do_the_utb();
            }

            result->cmd = save_cmd;

            if (P.pzzz.query_instant_stop(result->result_flags.misc)) break;

         paranoid_next_cycle:

            // Increment for next cycle.
            P.pzzz.m_fetch_index += P.pzzz.m_subcall_incr;
            P.pzzz.m_client_index += P.pzzz.m_subcall_incr;
         }
      }

      // Repair the damage.

      result->cmd.callspec = save_subject_call;
      ss->cmd.parseptr->call = save_subject_call;
      ss->cmd.parseptr->call_to_print = save_subject_call;
      current_options.who = saved_selector;
   }
   else if (parseptr->concept->arg1 == part_key_half_and_half ||
       parseptr->concept->arg1 == part_key_frac_and_frac) {

      // This is "half and half", or "frac and frac".

      uint32_t incoming_numerical_arg = (parseptr->concept->arg1 == part_key_frac_and_frac) ?
         parseptr->options.number_fields : NUMBER_FIELDS_2_1_2_1;

      fraction_info zzz(2);

      if (!ss->cmd.cmd_fraction.is_null()) {
         zzz.get_fraction_info(ss->cmd.cmd_fraction,
                               CFLAG1_VISIBLE_FRACTION_MASK / CFLAG1_VISIBLE_FRACTION_BIT,
                               weirdness_off);
      }

      zzz.m_first_call = !zzz.m_reverse_order;

      prepare_for_call_in_series(result, ss);

      for (;;) {
         int fetch_number;
         fraction_command corefracs;

         if (prepare_for_call_under_repetition(&zzz, fetch_number, ss, result)) break;
         if (zzz.not_yet_in_active_section()) goto go_to_next_cycle;
         if (zzz.ran_off_active_section()) break;

         corefracs.flags = 0;
         corefracs.fraction = zzz.get_fracs_for_this_part();

         // The fetch number is 0 or 1.  Depending on which it is,
         // get the proper parse pointer.
         if (fetch_number == 0) {
            // Doing the first call.  Apply the fraction "first 1/2", or whatever.
            result->cmd.cmd_fraction.flags = 0;
            result->cmd.cmd_fraction.process_fractions(NUMBER_FIELDS_1_0, incoming_numerical_arg,
                                                       FRAC_INVERT_NONE, corefracs);
         }
         else {
            // Doing the second call.  Apply the fraction "last 1/2", or whatever.
            result->cmd.cmd_fraction.flags = 0;
            result->cmd.cmd_fraction.process_fractions(incoming_numerical_arg >> (BITS_PER_NUMBER_FIELD*2), NUMBER_FIELDS_1_1,
                                                       FRAC_INVERT_START, corefracs);
            result->cmd.parseptr = parseptr->subsidiary_root;
         }

         do_call_in_series_simple(result);
         zzz.m_first_call = false;

         // If we are being asked to do just one part of a call,
         // exit now.  Also, fill in bits in result->result_flags.

         if (zzz.query_instant_stop(result->result_flags.misc)) break;

      go_to_next_cycle:

         // Increment for next cycle.
         zzz.m_fetch_index += zzz.m_subcall_incr;
         zzz.m_client_index += zzz.m_subcall_incr;
      }
   }
   else if (parseptr->concept->arg1 == part_key_use_nth_part) {
      // This is "use (call) for <Nth> part", which is the same as "replace the <Nth> part".

      if (!ss->cmd.cmd_fraction.is_null())
         fail("Can't stack meta or fractional concepts.");

      prepare_for_call_in_series(result, ss);
      int stopindex = parseptr->options.number_fields;

      // Do the early part, if any, of the main call.

      if (stopindex > 1) {
         copy_cmd_preserve_elong_and_expire(ss, result);
         result->cmd.prior_expire_bits |= RESULTFLAG__EXPIRATION_ENAB;
         result->cmd.parseptr = parseptr->subsidiary_root;
         result->cmd.cmd_fraction.set_to_null_with_flags(
            FRACS(CMD_FRAC_CODE_FROMTO,stopindex-1,0) | CMD_FRAC_BREAKING_UP);
         result->cmd.prior_expire_bits |=
            result->result_flags.misc & RESULTFLAG__EXPIRATION_BITS;
         do_call_in_series(result, true, true, false);
      }

      // Do the replacement call.

      result->cmd = ss->cmd;
      if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
         result->update_id_bits();    // So you can interrupt with "leads run", etc.
      result->cmd.cmd_misc_flags &= ~CMD_MISC__NO_EXPAND_MATRIX;

      // Give this call a clean start with respect to expiration stuff.
      uint32_t suspended_expiration_bits = result->result_flags.misc & RESULTFLAG__EXPIRATION_BITS;
      result->result_flags.misc &= ~RESULTFLAG__EXPIRATION_BITS;
      do_call_in_series(result, true, true, false);

      // Put back the expiration bits for the resumed call.
      result->result_flags.misc &= ~RESULTFLAG__EXPIRATION_BITS;
      result->result_flags.misc |= suspended_expiration_bits|RESULTFLAG__EXPIRATION_ENAB;

      // Do the remainder of the main call, if there is more.

      copy_cmd_preserve_elong_and_expire(ss, result);
      result->cmd.parseptr = parseptr->subsidiary_root;
      result->cmd.cmd_fraction.set_to_null_with_flags(
         FRACS(CMD_FRAC_CODE_FROMTOREV,stopindex+1,0) | CMD_FRAC_BREAKING_UP);
      result->cmd.prior_expire_bits |= result->result_flags.misc & RESULTFLAG__EXPIRATION_BITS;
      do_call_in_series(result, true, true, false);
   }
   else if (parseptr->concept->arg1 == part_key_use_last_part) {
      // This is "use (call) for last part".

      if (!ss->cmd.cmd_fraction.is_null())
         fail("Can't stack meta or fractional concepts.");

      prepare_for_call_in_series(result, ss);

      // Do all of the main call except the last.

      copy_cmd_preserve_elong_and_expire(ss, result);
      result->cmd.prior_expire_bits |= RESULTFLAG__EXPIRATION_ENAB;
      result->cmd.parseptr = parseptr->subsidiary_root;
      result->cmd.cmd_fraction.set_to_null_with_flags(
         FRACS(CMD_FRAC_CODE_FROMTOREV,1,1) | CMD_FRAC_BREAKING_UP);
      result->cmd.prior_expire_bits |=
         result->result_flags.misc & RESULTFLAG__EXPIRATION_BITS;
      do_call_in_series(result, true, true, false);

      // Do the replacement call.

      result->cmd = ss->cmd;
      if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
         result->update_id_bits();    // So you can interrupt with "leads run", etc.
      result->cmd.cmd_misc_flags &= ~CMD_MISC__NO_EXPAND_MATRIX;

      // Give this call a clean start with respect to expiration stuff.
      result->result_flags.misc &= ~RESULTFLAG__EXPIRATION_BITS;
      do_call_in_series(result, true, true, false);
   }
   else if (parseptr->concept->arg1 == part_key_start_with) {
      // This is "start with (call)", which is the same as "replace the 1st part".

      if (!ss->cmd.cmd_fraction.is_null())
         fail("Can't stack meta or fractional concepts.");

      setup tttt = *ss;
      uint32_t finalresultflagsmisc = 0;

      // Do the special first part.

      if (!(tttt.result_flags.misc & RESULTFLAG__NO_REEVALUATE))
         tttt.update_id_bits();           /* So you can use "leads run", etc. */
      move(&tttt, false, result);
      finalresultflagsmisc |= result->result_flags.misc;
      normalize_setup(result, simple_normalize, qtag_compress);

      // Do the rest of the original call, if there is more.

      tttt = *result;
      // Skip over the concept.
      tttt.cmd = ss->cmd;
      tttt.cmd.parseptr = parseptr->subsidiary_root;
      tttt.cmd.cmd_fraction.set_to_null_with_flags(FRACS(CMD_FRAC_CODE_FROMTOREV,2,0));
      move(&tttt, false, result);
      finalresultflagsmisc |= result->result_flags.misc;
      normalize_setup(result, simple_normalize, qtag_compress);

      result->result_flags.misc = finalresultflagsmisc & ~3;
   }
   else {
      // This is
      //    replace with (part_key_use)
      //    follow by (part_key_follow_by)
      //    precede with (part_key_precede_by)

      // We allow fractionalization commands only for the special case of "piecewise"
      // or "random", in which case we will apply them to the first call only.

      if (!ss->cmd.cmd_fraction.is_null() &&
          !(ss->cmd.cmd_misc3_flags & CMD_MISC3__PUT_FRAC_ON_FIRST))
         fail("Can't stack meta or fractional concepts.");

      prepare_for_call_in_series(result, ss);

      if (parseptr->concept->arg1 == part_key_use) {
         // Replace with this call.

         // We need to do a "test execution" of the thing being replaced, to find out
         // whether we are at the last part.

         copy_cmd_preserve_elong_and_expire(ss, result);
         result->cmd.parseptr = parseptr->subsidiary_root;
         result->cmd.cmd_misc2_flags |= CMD_MISC2__DO_NOT_EXECUTE;
         do_call_in_series_simple(result);
         uint32_t saved_last_flagmisc = result->result_flags.misc &
            (RESULTFLAG__DID_LAST_PART|RESULTFLAG__PARTS_ARE_KNOWN);  // The info we want.

         // Now do the actual replacement call.  It gets no fractions.
         prepare_for_call_in_series(result, ss);
         result->cmd.cmd_fraction.set_to_null();
         do_call_in_series_simple(result);
         result->result_flags.misc &= ~RESULTFLAG__PART_COMPLETION_BITS;
         result->result_flags.misc |= saved_last_flagmisc;
      }
      else {
         // This is follow (part_key_follow_by,0) or precede (part_key_precede_by,1).

         for (int call_index=0; call_index<2; call_index++) {
            copy_cmd_preserve_elong_and_expire(ss, result);

            if ((call_index ^ parseptr->concept->arg1) != 0) {
               // The interloper call.  It gets no fractions.
               result->cmd.cmd_fraction.set_to_null();
               uint32_t saved_last_flagmisc = result->result_flags.misc &
                  (RESULTFLAG__DID_LAST_PART|RESULTFLAG__PARTS_ARE_KNOWN);
               do_call_in_series_simple(result);
               result->result_flags.misc &= ~RESULTFLAG__PART_COMPLETION_BITS;
               result->result_flags.misc |= saved_last_flagmisc;
            }
            else {
               // The base call, or the part of it.
               result->cmd.parseptr = parseptr->subsidiary_root;
               do_call_in_series_simple(result);
            }
         }
      }
   }
}


static void do_concept_n_times(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // This includes "twice", "thrice", and "N times", which means it can
   // either take a hard-wired number or a number from the current options.
   // arg1 = 0 :  number of repetitions is hardwired and is in arg2.
   // arg1 = 1 :  number of repetitions was specified by user.

   // Lift the craziness restraint from before -- we are about to pull things apart.
   ss->cmd.cmd_misc3_flags &= ~CMD_MISC3__RESTRAIN_CRAZINESS;

   setup offline_split_info;

   // We do not accumulate the splitting information as the calls progress.
   // Instead, we accumulate it into an offline setup, and put the final
   // splitting info into the result at the end.

   prepare_for_call_in_series(&offline_split_info, ss);

   fraction_info zzz((parseptr->concept->arg1) ?
                     parseptr->options.number_fields : parseptr->concept->arg2);

   // If fractions come in but the craziness is restrained, just pass the fractions on.
   // This is what makes "oddly twice mix" work.  The "random" concept wants to reach through
   // the "twice" concept and do the numbered parts of the mix, not the numbered parts of the
   // twiceness.  But if the craziness is unrestrained, which is the usual case, we act on
   // the fractions.  This makes "interlace twice this with twice that" work.

   zzz.get_fraction_info(ss->cmd.cmd_fraction,
                         CFLAG1_VISIBLE_FRACTION_MASK / CFLAG1_VISIBLE_FRACTION_BIT,
                         weirdness_off);

   zzz.m_first_call = !zzz.m_reverse_order;

   *result = *ss;
   clear_result_flags(result, RESULTFLAG__REALLY_NO_REEVALUATE);

   for (;;) {
      int fetch_number;

      if (prepare_for_call_under_repetition(&zzz, fetch_number, ss, result)) break;
      if (zzz.not_yet_in_active_section()) goto go_to_next_cycle;
      if (zzz.ran_off_active_section()) break;

      // Do *NOT* try to maintain consistent splitting across repetitions when doing "twice".
      result->result_flags.maximize_split_info();

      // We do *NOT* remember the yoyo/twisted expiration stuff.
      result->result_flags.misc &= ~RESULTFLAG__EXPIRATION_BITS;

      // If doing a series of extends, and this is not first in the series, and
      // the setup is a general 1/4 tag, it must be a 3/4 tag; it can't be a 1/4 tag.
      if (!zzz.m_first_call && (result->kind == s_qtag || result->kind == sdmd)) {
         if (result->cmd.callspec == base_calls[base_call_extend] ||
             (result->cmd.parseptr && result->cmd.parseptr->call == base_calls[base_call_extend])) {
            result->cmd.cmd_assume.assumption = cr_jright;
            result->cmd.cmd_assume.assump_both = 1;
         }
      }

      do_call_in_series_simple(result);

      // Record the minimum in each direction.
      minimize_splitting_info(&offline_split_info, result->result_flags);
      zzz.m_first_call = false;

      // If we are being asked to do just one part of a call,
      // exit now.  Also, fill in bits in result->result_flags.

      if (zzz.query_instant_stop(result->result_flags.misc)) break;

   go_to_next_cycle:

      // Increment for next cycle.
      zzz.m_fetch_index += zzz.m_subcall_incr;
      zzz.m_client_index += zzz.m_subcall_incr;
   }

   result->result_flags.misc |= offline_split_info.result_flags.misc;  //******* needed?
   result->result_flags.copy_split_info(offline_split_info.result_flags);
}


static void do_concept_trace(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   if (process_brute_force_mxn(ss, parseptr, do_concept_trace, result)) return;

   bool interlock = ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_INTLK);

   ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_INTLK);
   // We don't allow other flags, like "cross".
   if (ss->cmd.cmd_final_flags.test_for_any_herit_or_final_bit())
      fail("Illegal modifier before \"trace\".");

   int i, r[4], rot[4];
   setup a[4], res[4];
   setup outer_inners[2];
   const int8_t *tracearray;

   static const int8_t tracearray1[16] =
   {-1, -1, 7, 6, -1, -1, 4, 5, 3, 2, -1, -1, 0, 1, -1, -1};
   static const int8_t tracearray2[16] =
   {0, 1, -1, -1, 6, 7, -1, -1, -1, -1, 4, 5, -1, -1, 2, 3};

   if (ss->kind != s_qtag) fail("Must have a 1/4-tag-like setup for trace.");

   // We handle "interlocked trace" in a rather simple-minded way.
   if (interlock) ss->swap_people(3, 7);

   ss->cmd.cmd_misc_flags |= CMD_MISC__DISTORTED|CMD_MISC__QUASI_PHANTOMS;

   for (i=0 ; i<4 ; i++) {
      a[i] = *ss;
      a[i].clear_people();
      a[i].kind = s2x2;
      a[i].rotation = 0;
      a[i].eighth_rotation = 0;
   }

   if ((ss->people[6].id1 & d_mask) == d_north && (ss->people[2].id1 & d_mask) == d_south) {
      a[1].cmd.parseptr = parseptr->subsidiary_root;
      a[3].cmd.parseptr = parseptr->subsidiary_root;
      tracearray = tracearray1;
   }
   else if ((ss->people[6].id1 & d_mask) == d_south && (ss->people[2].id1 & d_mask) == d_north) {
      a[0].cmd.parseptr = parseptr->subsidiary_root;
      a[2].cmd.parseptr = parseptr->subsidiary_root;
      tracearray = tracearray2;
   }
   else
      fail("Can't determine which box people should work in.");

   for (i=0 ; i<4 ; i++) {
      a[i].cmd.cmd_assume.assumption = cr_none;
      gather(&a[i], ss, tracearray, 3, 0);
      a[i].update_id_bits();
      move(&a[i], false, &res[i]);
      tracearray += 4;
   }

   resultflag_rec finalresultflags = get_multiple_parallel_resultflags(res, 4);
   outer_inners[1].clear_people();
   outer_inners[0].clear_people();

   // Check that everyone is in a 2x2 or vertically oriented 1x4.

   for (i=0 ; i<4 ; i++) {
      if ((res[i].kind != s2x2 && res[i].kind != nothing && (res[i].kind != s1x4 || (!(res[i].rotation&1)))))
         fail("You can't do this.");
   }

   // Process people going into the center.

   outer_inners[1].rotation = 0;
   outer_inners[1].eighth_rotation = 0;
   clear_result_flags(&outer_inners[1]);

   if   ((res[0].kind == s2x2 && (res[0].people[2].id1 | res[0].people[3].id1)) ||
         (res[1].kind == s2x2 && (res[1].people[0].id1 | res[1].people[1].id1)) ||
         (res[2].kind == s2x2 && (res[2].people[0].id1 | res[2].people[1].id1)) ||
         (res[3].kind == s2x2 && (res[3].people[2].id1 | res[3].people[3].id1)))
      outer_inners[1].kind = s1x4;
   else
      outer_inners[1].kind = nothing;

   for (i=0 ; i<4 ; i++) {
      r[i] = res[i].rotation & 2;
      rot[i] = (res[i].rotation & 3) * 011;
   }

   if   ((res[0].kind == s1x4 && (res[0].people[2 ^ r[0]].id1 | res[0].people[3 ^ r[0]].id1)) ||
         (res[1].kind == s1x4 && (res[1].people[0 ^ r[1]].id1 | res[1].people[1 ^ r[1]].id1)) ||
         (res[2].kind == s1x4 && (res[2].people[0 ^ r[2]].id1 | res[2].people[1 ^ r[2]].id1)) ||
         (res[3].kind == s1x4 && (res[3].people[2 ^ r[3]].id1 | res[3].people[3 ^ r[3]].id1))) {
      if (outer_inners[1].kind != nothing) fail("You can't do this.");
      outer_inners[1].kind = s2x2;
   }

   if (res[0].kind == s2x2) {
      install_person(&outer_inners[1], 1, &res[0], 2);
      install_person(&outer_inners[1], 0, &res[0], 3);
   }
   else {
      install_rot(&outer_inners[1], 3, &res[0], 2^r[0], rot[0]);
      install_rot(&outer_inners[1], 0, &res[0], 3^r[0], rot[0]);
   }

   if (res[1].kind == s2x2) {
      install_person(&outer_inners[1], 0, &res[1], 0);
      install_person(&outer_inners[1], 1, &res[1], 1);
   }
   else {
      install_rot(&outer_inners[1], 0, &res[1], 0^r[1], rot[1]);
      install_rot(&outer_inners[1], 3, &res[1], 1^r[1], rot[1]);
   }

   if (res[2].kind == s2x2) {
      install_person(&outer_inners[1], 3, &res[2], 0);
      install_person(&outer_inners[1], 2, &res[2], 1);
   }
   else {
      install_rot(&outer_inners[1], 1, &res[2], 0^r[2], rot[2]);
      install_rot(&outer_inners[1], 2, &res[2], 1^r[2], rot[2]);
   }

   if (res[3].kind == s2x2) {
      install_person(&outer_inners[1], 2, &res[3], 2);
      install_person(&outer_inners[1], 3, &res[3], 3);
   }
   else {
      install_rot(&outer_inners[1], 2, &res[3], 2^r[3], rot[3]);
      install_rot(&outer_inners[1], 1, &res[3], 3^r[3], rot[3]);
   }

   // Process people going to the outside.

   outer_inners[0].rotation = 0;
   outer_inners[0].eighth_rotation = 0;
   clear_result_flags(&outer_inners[0]);

   for (i=0 ; i<4 ; i++) {
      r[i] = res[i].rotation & 2;
   }

   if   ((res[0].kind == s2x2 && (res[0].people[0].id1 | res[0].people[1].id1)) ||
         (res[1].kind == s2x2 && (res[1].people[2].id1 | res[1].people[3].id1)) ||
         (res[2].kind == s2x2 && (res[2].people[2].id1 | res[2].people[3].id1)) ||
         (res[3].kind == s2x2 && (res[3].people[0].id1 | res[3].people[1].id1)))
      outer_inners[0].kind = s2x2;
   else
      outer_inners[0].kind = nothing;

   if   ((res[0].kind == s1x4 && (res[0].people[0 ^ r[0]].id1 | res[0].people[1 ^ r[0]].id1)) ||
         (res[1].kind == s1x4 && (res[1].people[2 ^ r[1]].id1 | res[1].people[3 ^ r[1]].id1)) ||
         (res[2].kind == s1x4 && (res[2].people[2 ^ r[2]].id1 | res[2].people[3 ^ r[2]].id1)) ||
         (res[3].kind == s1x4 && (res[3].people[0 ^ r[3]].id1 | res[3].people[1 ^ r[3]].id1))) {
      if (outer_inners[0].kind != nothing) fail("You can't do this.");
      outer_inners[0].kind = s1x4;
      outer_inners[0].rotation = 1;
      outer_inners[0].eighth_rotation = 0;
   }

   for (i=0 ; i<4 ; i++) {
      int ind = (i+1) & 2;

      if (res[i].kind == s2x2) {
         install_person(&outer_inners[0], ind,     &res[i], ind);
         install_person(&outer_inners[0], ind ^ 1, &res[i], ind ^ 1);
      }
      else {
         install_rot(&outer_inners[0], ind,     &res[i], ind ^ (res[i].rotation&2),     ((res[i].rotation-1)&3)*011);
         install_rot(&outer_inners[0], ind ^ 1, &res[i], ind ^ (res[i].rotation&2) ^ 1, ((res[i].rotation-1)&3)*011);
      }
   }

   normalize_concentric(ss, schema_concentric, 1, outer_inners,
                        ((~outer_inners[0].rotation) & 1) + 1, 0, result);
   result->result_flags = finalresultflags;

   if (interlock) {
      if (result->kind != s_qtag) fail("Can't do this interlocked trace.");
      result->swap_people(3, 7);
   }

   reinstate_rotation(ss, result);
}


static void do_concept_move_in_and(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   int rotfix = 0;
   int eighthrot = 0;
   calldef_schema the_schema = schema_concentric;

   if (ss->kind == s_alamo && global_livemask == 0xFF) {
      switch (global_selectmask) {
      case 0x99:
         rotfix--;
         ss->rotation++;
         canonicalize_rotation(ss);
         // FALL THROUGH!!!
      case 0x66:
         // FELL THROUGH!!!
         // Move them counterclockwise one eighth.
         eighthrot = 1;
         copy_person(ss, 8, ss, 2);
         copy_person(ss, 2, ss, 3);
         copy_rot(ss, 3, ss, 4, 033);
         copy_person(ss, 4, ss, 5);
         copy_rot(ss, 5, ss, 6, 033);
         copy_person(ss, 6, ss, 7);
         copy_rot(ss, 7, ss, 0, 033);
         copy_person(ss, 0, ss, 1);
         copy_rot(ss, 1, ss, 8, 033);
         break;
      case 0xCC:
         rotfix--;
         ss->rotation++;
         canonicalize_rotation(ss);
         break;
      case 0x33:
         break;
      default:
         fail("Can't do this with these people selected.");
      }

      ss->swap_people(6, 7);
      ss->swap_people(5, 6);
      ss->swap_people(4, 5);
      ss->swap_people(3, 4);
      ss->swap_people(2, 3);
      ss->swap_people(1, 2);
      ss->swap_people(0, 1);
      ss->kind = s2x4;
   }
   else if (ss->kind == s4x4 && global_livemask == 0x6666) {
      switch (global_selectmask) {
      case 0x2424:
         rotfix--;
         ss->rotation++;
         canonicalize_rotation(ss);
         // FALL THROUGH!!!
      case 0x4242:
         // FELL THROUGH!!!
         // Move them counterclockwise one eighth.
         eighthrot = 1;
         copy_person(ss, 0, ss, 1);
         copy_person(ss, 1, ss, 2);
         copy_rot(ss, 2, ss, 5, 033);
         copy_person(ss, 5, ss, 6);
         copy_rot(ss, 6, ss, 9, 033);
         copy_person(ss, 9, ss, 10);
         copy_rot(ss, 10, ss, 13, 033);
         copy_person(ss, 13, ss, 14);
         copy_rot(ss, 14, ss, 0, 033);
         break;
      case 0x0606:
         rotfix--;
         ss->rotation++;
         canonicalize_rotation(ss);
         break;
      case 0x6060:
         break;
      default:
         fail("Can't do this with these people selected.");
      }

      ss->swap_people(1, 3);
      ss->swap_people(2, 4);
      ss->swap_people(7, 9);
      ss->swap_people(0, 10);
      ss->swap_people(1, 13);
      ss->swap_people(2, 14);
      ss->kind = s2x4;
   }
   else if (ss->kind == s_spindle && global_livemask == 0xFF && global_selectmask == 0x88) {
      ss->kind = s_323;   // That's all!
      the_schema = schema_concentric_2_6;
   }
   else
      fail("Must be on squared-set spots.");

   // We need to find out whether the subject call has an implicit "centers" concept.

   uint32_t bogus_topcallflags1 = 0;
   parse_block *bogus_parse_block = ss->cmd.parseptr;
   if (check_for_centers_concept(bogus_topcallflags1, bogus_parse_block, &ss->cmd)) {
      ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;
      concentric_move(ss, &ss->cmd, (setup_command *) 0, the_schema, 0, 0, true, false, ~0U, result);
   }
   else {
      move(ss, false, result);
   }

   if (eighthrot) {
      result->eighth_rotation ^= 1;
      if (!result->eighth_rotation) rotfix++;
   }

   result->rotation += rotfix;
   canonicalize_rotation(result);
}


static void do_concept_outeracting(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   setup temp;

   static const int8_t mapo1[8] = {7, 0, 2, 5, 3, 4, 6, 1};
   static const int8_t mapo2[8] = {7, 0, 1, 6, 3, 4, 5, 2};

   temp = *ss;
   temp.kind = s2x4;
   temp.rotation++;
   temp.clear_people();

   if (ss->kind != s_qtag)
      fail("Must have 1/4 tag to do this concept.");

   if ((((ss->people[2].id1 ^ d_south) | (ss->people[6].id1 ^ d_north)) & d_mask) == 0) {
      scatter(&temp, ss, mapo1, 7, 033);
   }
   else if ((((ss->people[2].id1 ^ d_north) | (ss->people[6].id1 ^ d_south)) & d_mask) == 0) {
      scatter(&temp, ss, mapo2, 7, 033);
   }
   else
      fail("Incorrect facing directions.");

   divided_setup_move(&temp, MAPCODE(s2x2,2,MPKIND__SPLIT,0), phantest_ok, true, result);
}


static void do_concept_multiple_boxes(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // Arg2 = the matrix that we need, with CONCPROP__NEED_ARG2_MATRIX.  It has
   //    already been taken care of.
   // Arg4 = the number of items.
   // Arg5 = stuff used by "do_triple_formation", which we might call.
   //    It is MPKIND__SPLIT.  We don't look at it here.

   if (parseptr->concept->arg4 == 3) {
      if (ss->kind != s2x6) fail("Must have a 2x6 setup for this concept.");
      do_triple_formation(ss, parseptr, MAPCODE(s2x2,3,parseptr->concept->arg5,0), result);
   }
   else {
      if (parseptr->concept->arg4 == 4) {
         if (ss->kind != s2x8) fail("Must have a 2x8 setup for this concept.");
      }
      else if (parseptr->concept->arg4 == 5) {
         if (ss->kind != s2x10) fail("Must have a 2x10 setup for this concept.");
      }
      else if (parseptr->concept->arg4 == 6) {
         if (ss->kind != s2x12) fail("Must have a 2x12 setup for this concept.");
      }

      divided_setup_move(ss, MAPCODE(s2x2,parseptr->concept->arg4,MPKIND__SPLIT,0),
                         phantest_ok, true, result);
   }
}


static void do_concept_inner_outer(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   uint32_t livemask;
   calldef_schema sch;
   int rot = 0;
   int arg1 = parseptr->concept->arg1;

   // Low octal digit of arg1 gives the basic inner type, as a CS_* enumeration.
   // The 8 bit indicates the outer ones (as opposed to the inner ones):
   // Next hex digit says how they are put together:
   //
   // 00 (center/outside) plain triple whatever
   // 10 (center/outside) phantom whatever
   // 20 (center/outside) triple twin whatever
   // 30 (center/outside) triple twin whatever of 3
   // 40 (center/outside) 12 matrix phantom whatever
   // 50 (center) tidal whatever

   switch (arg1 & 0x70) {
   case 0:      // triple CLWBDZ
   case 0x20:   // triple twin CLW
   case 0x30:   // triple twin CLW of 3
   case 0x50:   // triple tidal CLW
      sch = schema_in_out_triple;
      break;
   case 0x10:
      sch = schema_in_out_quad;
      break;
   case 0x40:
      sch = schema_in_out_12mquad;
      break;
   case 0x60:
      sch = schema_inner_2x4;
      break;
   case 0x70:
      sch = schema_inner_2x6;
      break;
   }

   switch (arg1) {
   case CS_C: case CS_L: case CS_W:
      // Center triple line/wave/column.
      switch (ss->kind) {
      case s3x4: case s1x12:
         if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

         if (!((arg1 ^ global_tbonetest) & 1)) {
            if (global_tbonetest & 1) fail("There are no triple lines here.");
            else                      fail("There are no triple columns here.");
         }
         goto ready;
      case sbigh: case sbigx: case s_dmdlndmd: case sbigdmd: case sbigbone:
         goto verify_clw;
      }

      fail("Need a triple line/column setup for this.");
   case 8+CS_C: case 8+CS_L: case 8+CS_W:
      // Outside triple lines/waves/columns.
      switch (ss->kind) {
      case s3x4: case s1x12:
         if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

         if (!((arg1 ^ global_tbonetest) & 1)) {
            if (global_tbonetest & 1) fail("There are no triple lines here.");
            else                      fail("There are no triple columns here.");
         }
         goto ready;
      case sbigh: case sbigx: case sbigrig: case s_hsqtag: case s5x1dmd: case s1x5dmd:
         goto verify_clw;
      }

      fail("Need a triple line/column setup for this.");
   case 0x20+CS_C: case 0x20+CS_L: case 0x20+CS_W:
   case 0x20+8+CS_C: case 0x20+8+CS_L: case 0x20+8+CS_W:
      // Center/outside triple twin lines/waves/columns.
      if (ss->kind != s4x6) fail("Need a 4x6 setup for this.");

      if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

      if ((arg1 ^ global_tbonetest) & 1) {
         if (global_tbonetest & 1) fail("There are no triple twin lines here.");
         else                      fail("There are no triple twin columns here.");
      }

      goto ready;
   case 0x80+CS_C: case 0x80+CS_L: case 0x80+CS_W:
      // Center tidal line/wave/column.
      // This concept has the "CONCPROP__NEEDK_3X8" property set,
      // which will fail for quadruple diamonds.
      if (ss->kind != s3x8 && ss->kind != s4dmd)
         fail("Need center tidal setup for this.");
      // Unfortunately, we can't readily test facing direction.
      // Too lazy to do it right.
      goto ready;
   case 0x30+CS_C: case 0x30+CS_L: case 0x30+CS_W:
   case 0x38+CS_C: case 0x38+CS_L: case 0x38+CS_W:
      // Center/outside triple twin lines/waves/columns of 3.
      if (ss->kind != s3x6) fail("Need a 3x6 setup for this.");

      if ((global_tbonetest & 011) == 011) fail("Can't do this from T-bone setup.");

      if ((arg1 ^ global_tbonetest) & 1) {
         if (global_tbonetest & 1) fail("There are no triple twin lines of 3 here.");
         else                      fail("There are no triple twin columns of 3here.");
      }

      goto ready;
   case 0x10+CS_C: case 0x10+CS_L: case 0x10+CS_W:
   case 0x18+CS_C: case 0x18+CS_L: case 0x18+CS_W:
      // Center or outside phantom lines/waves/columns.

      if (ss->kind == s4x4) {
         uint32_t tbone = global_tbonetest;
         // If everyone is consistent (or "standard" was used
         // to make it appear so), it's easy.
         if ((tbone & 011) == 011) {
            // If not, we need to look carefully.
            tbone =
               ss->people[1].id1 | ss->people[2].id1 |
               ss->people[5].id1 | ss->people[6].id1 |
               ss->people[9].id1 | ss->people[10].id1 |
               ss->people[13].id1 | ss->people[14].id1;

            if (arg1 & 8) {
               // This is outside phantom C/L/W.
               tbone |=
                  ss->people[0].id1 | ss->people[4].id1 |
                  ss->people[8].id1 | ss->people[12].id1;
            }
            else {
               // This is center phantom C/L/W.
               tbone |=
                  ss->people[3].id1 | ss->people[7].id1 |
                  ss->people[11].id1 | ss->people[15].id1;
            }
            if ((tbone & 011) == 011) fail("Can't do this from T-bone setup.");
         }

         rot = (tbone ^ arg1 ^ 1) & 1;

         ss->rotation += rot;   // Just flip the setup around and recanonicalize.
         canonicalize_rotation(ss);
      }
      else if (ss->kind == s1x16 || ss->kind == sbigbigh || ss->kind == sbigbigx) {
         /* Shouldn't we do this for 4x4 also?  It seems that one ought to say
            "standard in outside phantom 1x4's" rather than "... in lines"
            if they are T-boned.  There is something in t34t that would break, however. */
         goto verify_clw;
      }
      else
         fail("Need quadruple 1x4's for this.");

      break;
   case 0x40+CS_C: case 0x40+CS_L:
   case 0x48+CS_C: case 0x48+CS_L:
      // Center or outside phantom lines/columns (no wave).
      if (ss->kind == s3x4 || ss->kind == sd3x4 || ss->kind == s3dmd)
         goto verify_clw;
      else
         fail("Need quadruple 1x3's for this.");
      break;
   case CS_B:
      // Center triple box.
      switch (ss->kind) {
      case s2x6: case sbigrig:
         goto ready;
      case s4x4:
         if (ss->people[1].id1 | ss->people[2].id1 | ss->people[9].id1 | ss->people[10].id1) {
            // The outer lines/columns are to the left and right.  That's not canonical.
            // We want them at top and bottom.  So we flip the setup.
            rot = 1;
            ss->rotation++;
            canonicalize_rotation(ss);
         }

         // Now the people had better be clear from the side centers.

         if (!(ss->people[1].id1 | ss->people[2].id1 | ss->people[9].id1 | ss->people[10].id1))
            goto ready;
      }

      fail("Need center triple box for this.");
   case 8+CS_B:
      // Outside triple boxes.
      switch (ss->kind) {
      case s2x6: case sbigbone: case sbigdmd:
      case sbighrgl: case sbigdhrgl: case s4x5:
         goto ready;
      }

      fail("Need outer triple boxes for this.");
   case 0x10+CS_B:
   case 0x18+CS_B:
      // Center or outside quadruple boxes.
      if (ss->kind != s2x8) fail("Need a 2x8 setup for this.");
      goto ready;
   case CS_D:
      // Center triple diamond.
      switch (ss->kind) {
      case s3dmd: case s3ptpd: case s_3mdmd: case s_3mptpd: case s3x1dmd: case s1x3dmd:
      case s_hsqtag: case s_hrglass: case s_dhrglass: case sbighrgl: case sbigdhrgl:
      case s5x1dmd: case s1x5dmd:
         goto ready;
      }

      fail("Need center triple diamond for this.");
   case 8+CS_D:
      // Outside triple diamonds.
      switch (ss->kind) {
      case s3dmd: case s3ptpd: case s_3mdmd: case s_3mptpd: case s_dmdlndmd: case s3x1dmd: case s1x3dmd:
         goto ready;
      }

      fail("Need outer triple diamonds for this.");
   case 0x10+CS_D:
   case 8+0x10+CS_D:
      // Center or outside quadruple diamonds.
      if (ss->kind != s4dmd &&
          ss->kind != s4ptpd &&
          ss->kind != s_4mptpd &&
          ss->kind != s_4mdmd) fail("Need quadruple diamonds for this.");
      ss->cmd.cmd_misc_flags |= parseptr->concept->arg3;
      break;
   case CS_Z:
   case 8+CS_Z:
      // Center/outside triple Z's.
      sch = schema_in_out_triple_zcom;
      livemask = ss->little_endian_live_mask();

      switch (ss->kind) {
      case s3x4:
         if (arg1 & 8) break;   // Can't say "outside triple Z's", only "center Z".

         // Demand that the center Z be properly filled.
         switch (livemask & 04646) {
         case 04242: case 04444:
            sch = schema_in_out_center_triple_z;
            goto do_real_z_stuff;
         }
         break;
      case s3x6:
         if ((livemask & 0063063) == 0) {
            // Of course, this is kind of stupid.  Why would you say "outer
            // triple Z's" if the outer Z's weren't recognizable, and you
            // simply wanted them to be imputed from the center Z?
            if (arg1 & 8)
               warn(warn_same_z_shear);  // Outer Z's are ambiguous --
                                         // make them look like inner ones.
         }

         // Demand that the center Z be solidly filled.

         switch (livemask & 0414414) {
         case 0404404:   // Center Z is CW.
            if ((livemask & 0042042) == 0 || (livemask & 0021021) == 0) {
               goto do_real_z_stuff;
            }

            break;
         case 0410410:   // Center Z is CCW.
            if ((livemask & 0021021) == 0 || (livemask & 0042042) == 0) {
               goto do_real_z_stuff;
            }

            break;
         }
         break;
      case swqtag:
         if (arg1 & 8) break;   // Can't say "outside triple Z's".
         switch (livemask & 0x273) {
         case 0x231: case 0x252:
            sch = schema_in_out_center_triple_z;
            goto do_real_z_stuff;
         }
         break;
      case s2x5:
         if (arg1 & 8) break;   // Can't say "outside triple Z's".
         switch (livemask) {
         case 0x3BD: case 0x2F7:
            sch = schema_in_out_center_triple_z;
            goto do_real_z_stuff;
         }
         break;
      case s2x7:
         if (arg1 & 8) break;   // Can't say "outside triple Z's".
         switch (livemask & 0x0E1C) {
         case 0x0C18: case 0x060C:
            sch = schema_in_out_center_triple_z;
            goto do_real_z_stuff;
         }
         break;
      case sd2x7:
         if (arg1 & 8) break;   // Can't say "outside triple Z's".
         switch (livemask & 0x0E1C) {
         case 0x060C: case 0x0C18:
            sch = schema_in_out_center_triple_z;
            goto do_real_z_stuff;
         }
         break;
      case sd2x5:
         if (arg1 & 8) break;   // Can't say "outside triple Z's".
         switch (livemask) {
         case 0x1EF: case 0x37B:
            goto do_real_z_stuff;
         }
         break;
      case s4x5:
         if (arg1 & 8) break;   // Can't say "outside triple Z's".

         // Demand that the center Z be solidly filled.

         switch (livemask & 0x701C0) {
         case 0x300C0: case 0x60180:
            goto do_real_z_stuff;
         }
         break;
      case sd3x4:
         if (arg1 & 8) break;   // Can't say "outside triple Z's".

         // Demand that the center Z be solidly filled.

         switch (livemask & 0x38E) {
         case 0x30C: case 0x186:
            goto do_real_z_stuff;
         }
         break;
      }

      fail("Can't find the indicated formation.");

   case 0x60:
   case 0x70:;
   }

 ready:

   if ((arg1 & 7) == 3)
      ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

   if (arg1 & 8)
      concentric_move(ss, &ss->cmd, (setup_command *) 0, sch, 0, 0, true, false, ~0U, result);
   else
      concentric_move(ss, (setup_command *) 0, &ss->cmd, sch, 0, 0, true, false, ~0U, result);

   result->rotation -= rot;   // Flip the setup back.
   return;

 verify_clw:

   switch (arg1 & 7) {
   case 0: ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_COLS; break;
   case 1: ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_LINES; break;
   }
   goto ready;

 do_real_z_stuff:

   ss->cmd.cmd_misc2_flags |= CMD_MISC2__REQUEST_Z;
   goto ready;
}


static void do_concept_two_faced(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   ss->cmd.cmd_misc3_flags |= CMD_MISC3__TWO_FACED_CONCEPT;
   move(ss, false, result);
}


static void do_concept_do_both_boxes(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   if (ss->kind == s2x4)
      divided_setup_move(ss, parseptr->concept->arg1, phantest_ok, true, result);
   else if (ss->kind == s3x4 && parseptr->concept->arg3)
      // distorted_2x2s_move will notice that concept is funny and will do the right thing.
      distorted_2x2s_move(ss, parseptr, result);
   else if (ss->kind == s2x6 && (ss->cmd.cmd_final_flags.herit & INHERITFLAG_NXNMASK) == INHERITFLAGNXNK_3X3) {
      ss->cmd.cmd_final_flags.herit |= parseptr->concept->arg3 ? INHERITFLAG_INTPGRAM : INHERITFLAG_TRAP;
      move(ss, false, result);
   }
   else if (ss->kind == s2x8 && (ss->cmd.cmd_final_flags.herit & INHERITFLAG_NXNMASK) == INHERITFLAGNXNK_4X4) {
      ss->cmd.cmd_final_flags.herit |= parseptr->concept->arg3 ? INHERITFLAG_INTPGRAM : INHERITFLAG_TRAP;
      move(ss, false, result);
   }
   else
      fail("Not in proper setup for this concept.");
}


static void do_concept_do_each_1x4(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   uint32_t map_code;
   uint32_t tbonetest_fixer = 0;
   int rot = 0;
   int arg1 = parseptr->concept->arg1;
   int arg2 = parseptr->concept->arg2;

   if (arg2 == 1) {
      switch (ss->kind) {
         case s_qtag: case s_ptpd: break;
         default: fail("Need diamonds for this concept.");
      }
   }
   else if (arg2 == 2) {
      switch (ss->kind) {
         case s2x4: break;
         case s3x4:
            if (global_livemask == 0xCF3) {
               map_code = MAPCODE(s2x2,2,MPKIND__OFFS_R_HALF, 0);
               goto split_big;
            }
            else if (global_livemask == 0xF3C) {
               map_code = MAPCODE(s2x2,2,MPKIND__OFFS_L_HALF, 0);
               goto split_big;
            }
            fail("Need boxes for this concept.");
            break;
         case s4x4:
            if (global_livemask == 0xB4B4) {
               map_code = MAPCODE(s2x2,2,MPKIND__OFFS_R_FULL, 1);
               goto split_big;
            }
            else if (global_livemask == 0x4B4B) {
               map_code = MAPCODE(s2x2,2,MPKIND__OFFS_L_FULL, 1);
               goto split_big;
            }
         /* !!!!!! FALL THROUGH !!!!!! */
         default: fail("Need boxes for this concept.");
      }
   }
   else {
      if (arg1 == 3)
         ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

      switch (ss->kind) {
      case s2x4: case s1x8:
         goto split_small;
      case s4x4:
         if (global_livemask == 0x857A || global_livemask == 0x7A85 || global_livemask == 0x7171) {
            map_code = MAPCODE(s1x4,4,MPKIND__SPLIT,1);
            goto split_big;
         }

         else if (global_livemask == 0xA857 || global_livemask == 0x57A8 || global_livemask == 0x1717) {
            map_code = MAPCODE(s1x4,4,MPKIND__SPLIT,1);
            rot = 1;
            tbonetest_fixer = 0xFFFF;
            goto split_big;
         }

         break;
         // Future project:  do this for lots of interesting things in 4x5 or 4x6.
      case s_trngl8:
         map_code = HETERO_MAPCODE(s1x4,2,MPKIND__HET_SPLIT,0,s1x4,0x1);
         tbonetest_fixer = 0xF;
         goto split_big;
      case s3x4:
         if (global_livemask == 01717) {
            map_code = MAPCODE(s1x4,3,MPKIND__SPLIT,1);
            goto split_big;
         }
         break;
      case s2x6:
         if (global_livemask == 07474) {
            map_code = MAPCODE(s1x4,2,MPKIND__OFFS_R_HALF,1);
            goto split_big;
         }
         else if (global_livemask == 01717) {
            map_code = MAPCODE(s1x4,2,MPKIND__OFFS_L_HALF,1);
            goto split_big;
         }
         break;
      case s2x8:
         if (global_livemask == 0xF0F0) {
            map_code = MAPCODE(s1x4,2,MPKIND__OFFS_R_FULL,1);
            goto split_big;
         }
         else if (global_livemask == 0x0F0F) {
            map_code = MAPCODE(s1x4,2,MPKIND__OFFS_L_FULL,1);
            goto split_big;
         }
         break;
      case s1x10:
         if (global_livemask == 0x1EF) {
            map_code = spcmap_d1x10;
            goto split_big;
         }
         break;
      }

      fail("Can't find the indicated setups.");
   }

   split_small:

   if (arg2 == 0 && arg1 != 0 && ((arg1 ^ global_tbonetest) & 1) == 0)
      fail("People are not in the required line, column or wave.");

   do_simple_split(ss, (arg2 != 2) ? split_command_1x4_dmd : split_command_none, result);
   return;

   split_big:

   // May need to fudge the global_tbonetest value to deal with non-straightforward setups.
   if (tbonetest_fixer != 0) {
      global_tbonetest = 0;
      for (int i=0; i<=attr::slimit(ss); i++, tbonetest_fixer>>=1) {
         uint32_t p = ss->people[i].id1;
         if (p)
            global_tbonetest |= p ^ tbonetest_fixer;
      }
   }

   if (arg2 == 0 && arg1 != 0 && ((arg1 ^ global_tbonetest) & 1) == 0)
      fail("People are not in the required line, column or wave.");

   ss->rotation += rot;   // Just flip the setup around and recanonicalize.
   canonicalize_rotation(ss);
   divided_setup_move(ss, map_code, phantest_ok, true, result);
   result->rotation -= rot;   // Flip the setup back.
}


static void do_concept_centers_and_ends(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   who_list sel;
   sel.initialize();
   sel.who[0] = (selector_kind) parseptr->concept->arg1;

   selective_move(ss, parseptr, selective_key_plain, 1, 0, 0, sel, parseptr->concept->arg2 != 0, result);
}


static void do_concept_centers_or_ends(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   who_list sel;
   sel.initialize();
   sel.who[0] = (selector_kind) parseptr->concept->arg1;

   selective_move(ss, parseptr, selective_key_plain, 0, 0, 0, sel, parseptr->concept->arg2 != 0, result);
}


static void do_concept_mini_but_o(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   int rot = 0;
   uint32_t is_an_o = parseptr->concept->arg1;
   uint32_t mask = 0;

   switch (ss->kind) {
   case s_galaxy:
      if (is_an_o) {
         rot = ss->people[1].id1 | ss->people[3].id1 |
            ss->people[5].id1 | ss->people[7].id1;

         if ((rot & 011) == 011) fail("mini-O is ambiguous.");
         rot &= 1;
         ss->rotation += rot;
         canonicalize_rotation(ss);
         mask = 0xBB;
      }

      break;
   case s_hrglass:
      if (!is_an_o) mask = 0xBB;
      break;
   case s_ptpd:
      if (!is_an_o) mask = 0xEE;
      break;
   case s_rigger:
      if (is_an_o) mask = 0xBB;
      break;
   case swqtag:
      // We require the center two missing.
      if (is_an_o && (ss->people[4].id1 | ss->people[9].id1) == 0) mask = 0x16B;
      break;
   case s3x4:
      if (is_an_o) mask = 02626;
      break;
   }

   if (mask == 0) fail("Can't do this concept in this setup.");

   who_list sel;
   sel.initialize();
   sel.who[0] = selector_none;

   // Adding 2 to the lookup key makes it check for columns.
   selective_move(ss, parseptr, selective_key_mini_but_o, 0,
                  is_an_o ? LOOKUP_MINI_O+2 : LOOKUP_MINI_B+2,
                  mask, sel, false, result);

   result->rotation -= rot;   // Flip the setup back.
   return;
}



static void do_concept_multiple_diamonds(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // Arg4 = the number of items.
   uint32_t code;

   if (parseptr->concept->arg4 == 3) {
      switch (ss->kind) {
      case s3dmd:
         code = MAPCODE(sdmd,3,MPKIND__SPLIT,1); break;
      case s3ptpd:
         code = MAPCODE(sdmd,3,MPKIND__SPLIT,0); break;
      case s_3mdmd:
         code = HETERO_MAPCODE(sdmd,3,MPKIND__HET_SPLIT,1,sdmd,0x1); break;
      case s_3mptpd:
         code = HETERO_MAPCODE(sdmd,3,MPKIND__HET_SPLIT,1,sdmd,0x4); break;
      default:
         fail("Must have a triple diamond or 1/4 tag setup for this concept.");
      }

      do_triple_formation(ss, parseptr, code, result);
   }
   else {
      // See "do_triple_formation" for meaning of arg3.

      switch (ss->kind) {
      case s4dmd:
         code = MAPCODE(sdmd,4,MPKIND__SPLIT,1); break;
      case s4ptpd:
         code = MAPCODE(sdmd,4,MPKIND__SPLIT,0); break;
      case s_4mdmd:
         code = HETERO_MAPCODE(sdmd,4,MPKIND__HET_SPLIT,1,sdmd,0x1); break;
      case s_4mptpd:
         code = HETERO_MAPCODE(sdmd,4,MPKIND__HET_SPLIT,0,sdmd,0x4); break;
      default:
         fail("Must have a quadruple diamond or 1/4 tag setup for this concept.");
      }

      ss->cmd.cmd_misc_flags |= parseptr->concept->arg3;
      divided_setup_move(ss, code, phantest_ok, true, result);
   }
}


static void do_concept_multiple_formations(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // Meaning of arg3 is:
   //   0 - triple lines or boxes
   //   1 - triple lines or diamonds
   //   2 - triple boxes or diamonds

   // Arg4 = number of setups; always 3 at present.

   setup tempsetup = *ss;

   // We need to expand certain setups, of course.
   //
   // Why don't we do it the usual way, with "NEEDK" bits like
   // "NEEDK_TRIPLE_1X4_OR_BOX", setting those bits in the concept
   // properties?  Because we would run out of bits!  We have used
   // up too many bits doing other (worthwhile) things.  Unless we
   // change the format of the "NEEDK" mechanism to use 64 bits
   // (which isn't worth it), we have to do these by hand.  It's
   // not actually a serious problem -- the "NEEDK" mechanism does
   // a lot of powerful and useful things elsewhere in the program.
   // We don't need that power and flexibility here.  It's not much
   // work to take care of the individual cases.

   uint32_t need_prop = 0;

   switch (parseptr->concept->arg3) {
   case 0:
      switch (tempsetup.kind) {
      case s_rigger:            need_prop = CONCPROP__NEEDK_END_1X4; break;
      case s2x4:                need_prop = CONCPROP__NEEDK_4X4; break;
      case s_bone: case s_qtag: need_prop = CONCPROP__NEEDK_END_2X2; break;
      }

      tempsetup.do_matrix_expansion(need_prop, false);

      // The 4x4 case is for a box sandwiched between two parallel 1x4's.
      // The 4x5 case is for a line sandwiched between two boxes.
      // Concentric_move knows how to deal with these.
      if (tempsetup.kind != sbigbone && tempsetup.kind != sbigrig &&
          tempsetup.kind != sbigdmd && tempsetup.kind != s4x4 &&
          tempsetup.kind != s4x5)
         fail("Can't do this concept in this setup.");
      break;
   case 1:
      switch (tempsetup.kind) {
      case s3x1dmd: case s1x3dmd: case s_hrglass: need_prop = CONCPROP__NEEDK_END_1X4; break;
      }

      tempsetup.do_matrix_expansion(need_prop, false);

      if (tempsetup.kind != s5x1dmd && tempsetup.kind != s1x5dmd &&
          tempsetup.kind != s_hsqtag && tempsetup.kind != s_dmdlndmd)
         fail("Can't do this concept in this setup.");
      break;
   default:

      switch (tempsetup.kind) {
      case s_dhrglass: case s_hrglass: need_prop = CONCPROP__NEEDK_END_2X2; break;
      }

      tempsetup.do_matrix_expansion(need_prop, false);

      if (tempsetup.kind != sbighrgl && tempsetup.kind != sbigdhrgl)
         fail("Can't do this concept in this setup.");
      break;
   }

   do_triple_formation(&tempsetup, parseptr, ~0U, result);
}


static void do_concept_ferris(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   const expand::thing *map_ptr = (const expand::thing *) 0;

   if (parseptr->concept->arg1) {
      // This is "release".

      static const expand::thing mapr1 = {{10, 2, 3, 5, 4,  8, 9, 11}, s_qtag, s3x4, 0};
      static const expand::thing mapr2 = {{1,  4, 6, 5, 7, 10, 0, 11}, s_qtag, s3x4, 0};
      static const expand::thing mapr4 = {{1, 10, 0, 5, 7,  4, 6, 11}, s_qtag, s3x4, 0};
      static const expand::thing mapr8 = {{4,  2, 9, 5,10,  8, 3, 11}, s_qtag, s3x4, 0};
      static const expand::thing mapr10 = {{10, 1, 0, 11,  4,  7, 6, 5}, s_hrglass, s_hsqtag, 1};
      static const expand::thing mapr20 = {{2,  4, 9, 11,  8, 10, 3, 5}, s_hrglass, s_hsqtag, 1};
      static const expand::thing mapr40 = {{2, 10, 3, 11,  8,  4, 9, 5}, s_hrglass, s_hsqtag, 1};
      static const expand::thing mapr80 = {{4,  1, 6, 11, 10,  7, 0, 5}, s_hrglass, s_hsqtag, 1};
      static const expand::thing mapr100 = {{-1, -1, 2, 3, -1, -1, 8, 9}, s_spindle, s3x4, 0};
      static const expand::thing mapr200 = {{1, -1, -1, 6, 7, -1, -1, 0}, s_spindle, s3x4, 0};

      uint32_t directions;
      uint32_t livemask;
      ss->big_endian_get_directions32(directions, livemask);

      if ((((ss->kind != s_qtag) && (ss->kind != s_hrglass)) || ((directions & 0x4444) != 0)) &&
          (((ss->kind != s_spindle)) || ((directions & 0x4545) != 0)))
         fail("Must have quarter-tag or hourglass-like setup to do this concept.");

      uint32_t whatcanitbe = ~0;
      uint32_t t;

      if (ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_MAGIC)) { // qtag and hrglass only.
         if ((t = ss->people[0].id1) && ((t ^ d_south) & d_mask)) whatcanitbe &= 0x4;
         if ((t = ss->people[4].id1) && ((t ^ d_north) & d_mask)) whatcanitbe &= 0x4;
         if ((t = ss->people[2].id1) && ((t ^ d_south) & d_mask)) whatcanitbe &= 0x4;
         if ((t = ss->people[6].id1) && ((t ^ d_north) & d_mask)) whatcanitbe &= 0x4;
         if ((t = ss->people[1].id1) && ((t ^ d_south) & d_mask)) whatcanitbe &= 0x8;
         if ((t = ss->people[5].id1) && ((t ^ d_north) & d_mask)) whatcanitbe &= 0x8;
         if ((t = ss->people[2].id1) && ((t ^ d_north) & d_mask)) whatcanitbe &= 0x8;
         if ((t = ss->people[6].id1) && ((t ^ d_south) & d_mask)) whatcanitbe &= 0x8;
      }
      else if (ss->kind == s_spindle) {
         if ((t = ss->people[2].id1) && ((t ^ d_south) & d_mask)) whatcanitbe &= 0x1;
         if ((t = ss->people[6].id1) && ((t ^ d_north) & d_mask)) whatcanitbe &= 0x1;
         if ((t = ss->people[3].id1) && ((t ^ d_south) & d_mask)) whatcanitbe &= 0x1;
         if ((t = ss->people[7].id1) && ((t ^ d_north) & d_mask)) whatcanitbe &= 0x1;
         if ((t = ss->people[0].id1) && ((t ^ d_south) & d_mask)) whatcanitbe &= 0x2;
         if ((t = ss->people[4].id1) && ((t ^ d_north) & d_mask)) whatcanitbe &= 0x2;
         if ((t = ss->people[3].id1) && ((t ^ d_north) & d_mask)) whatcanitbe &= 0x2;
         if ((t = ss->people[7].id1) && ((t ^ d_south) & d_mask)) whatcanitbe &= 0x2;
      }
      else {
         if ((t = ss->people[1].id1) && ((t ^ d_south) & d_mask)) whatcanitbe &= 0x1;
         if ((t = ss->people[5].id1) && ((t ^ d_north) & d_mask)) whatcanitbe &= 0x1;
         if ((t = ss->people[2].id1) && ((t ^ d_south) & d_mask)) whatcanitbe &= 0x1;
         if ((t = ss->people[6].id1) && ((t ^ d_north) & d_mask)) whatcanitbe &= 0x1;
         if ((t = ss->people[0].id1) && ((t ^ d_south) & d_mask)) whatcanitbe &= 0x2;
         if ((t = ss->people[4].id1) && ((t ^ d_north) & d_mask)) whatcanitbe &= 0x2;
         if ((t = ss->people[2].id1) && ((t ^ d_north) & d_mask)) whatcanitbe &= 0x2;
         if ((t = ss->people[6].id1) && ((t ^ d_south) & d_mask)) whatcanitbe &= 0x2;
      }

      if (ss->kind == s_hrglass) whatcanitbe <<= 4;
      else if (ss->kind == s_spindle) whatcanitbe <<= 8;

      switch (whatcanitbe) {
      case 0x1: map_ptr = &mapr1; break;
      case 0x2: map_ptr = &mapr2; break;
      case 0x4: map_ptr = &mapr4; break;
      case 0x8: map_ptr = &mapr8; break;
      case 0x10: map_ptr = &mapr10; break;
      case 0x20: map_ptr = &mapr20; break;
      case 0x40: map_ptr = &mapr40; break;
      case 0x80: map_ptr = &mapr80; break;
      case 0x100: map_ptr = &mapr100; break;
      case 0x200: map_ptr = &mapr200; break;
      }

      ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_MAGIC);
   }
   else {
      // This is "ferris".

      static const expand::thing mapf1 = {{0, 1, 5, 4, 6, 7, 11, 10}, s2x4, s3x4, 0};
      static const expand::thing mapf2 = {{10, 11, 2, 3, 4, 5, 8, 9}, s2x4, s3x4, 0};

      if ((ss->kind != s2x4) || ((global_tbonetest & 1) != 0))
         fail("Must have lines to do this concept.");

      bool retvaljunk;
      assumption_thing t;
      t.assumption = cr_2fl_only;
      t.assump_col = 0;
      t.assump_both = 1;    // right 2FL
      t.assump_cast = 0;
      t.assump_live = 0;
      t.assump_negate = 0;

      if (verify_restriction(ss, t, false, &retvaljunk) == restriction_passes)
         map_ptr = &mapf1;
      else {
         t.assump_both = 2;    // left 2FL
         if (verify_restriction(ss, t, false, &retvaljunk) == restriction_passes)
            map_ptr = &mapf2;
      }
   }

   if (!map_ptr) fail("Incorrect facing directions.");

   // Be sure there aren't any modifiers other than "magic release".
   if (ss->cmd.cmd_final_flags.test_for_any_herit_or_final_bit())
      fail("Illegal modifier before \"ferris\" or \"release\".");

   expand::expand_setup(*map_ptr, ss);

   // If the next thing is an "offset CLW" concept or a "triple CLW" concept,
   // just do it, directly from the current 3x4 setup.
   if (ss->cmd.parseptr && ss->cmd.parseptr->concept &&
       ((ss->cmd.parseptr->concept->kind == concept_distorted &&
        ss->cmd.parseptr->concept->arg1 == disttest_offset &&
        (ss->cmd.parseptr->concept->arg4 >> 4) == DISTORTKEY_DIST_CLW) ||
       (ss->cmd.parseptr->concept->kind == concept_multiple_lines &&
        ss->cmd.parseptr->concept->arg4 == 3))) {
      move(ss, false, result);
   }
   else
      concentric_move(ss, &ss->cmd, &ss->cmd, schema_in_out_triple_squash, 0, 0, false, false, ~0U, result);
}


static void do_concept_overlapped_diamond(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   uint32_t mapcode;
   const expand::thing *scatterlist;
   static const expand::thing list1x4    = {{0, 1, 4, 5}, s1x4, s_thar, 0};
   static const expand::thing list1x4rot = {{2, 3, 6, 7}, s1x4, s_thar, 3};
   static const expand::thing listdmd    = {{0, 3, 4, 7}, sdmd, s_thar, 0};
   static const expand::thing listdmdrot = {{2, 5, 6, 1}, sdmd, s_thar, 3};

   // Split an 8 person setup.
   if (attr::slimit(ss) == 7) {
      // Reset it to execute this same concept again, until it doesn't have to split any more.
      ss->cmd.parseptr = parseptr;
      if (do_simple_split(ss, split_command_1x4_dmd, result))
         fail("Not in correct setup for overlapped diamond/line concept.");
      return;
   }

   switch (ss->kind) {
   case s1x4:
      if (parseptr->concept->arg1 != 2)
         fail("Must be in a diamond.");

      scatterlist = &list1x4;
      mapcode = MAPCODE(sdmd,2,MPKIND__NONISOTROPDMD,0);
      break;
   case sdmd:
      if (parseptr->concept->arg1 == 2)
         fail("Must be in a 1x4.");

      switch (parseptr->concept->arg1) {
      case 0: ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_COLS; break;
      case 1: ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_LINES; break;
      case 3: ss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES; break;
      }

      scatterlist = &listdmd;
      mapcode = MAPCODE(s1x4,2,MPKIND__NONISOTROPDMD,0);
      break;
   default:
      fail("Not in correct setup for overlapped diamond/line concept.");
   }

   expand::expand_setup(*scatterlist, ss);
   divided_setup_move(ss, mapcode, phantest_ok, true, result);

   if (result->kind == s2x2)
      return;
   else if (result->kind != s_thar)
      fail("Something horrible happened during overlapped diamond call.");

   if ((result->people[2].id1 | result->people[3].id1 |
        result->people[6].id1 | result->people[7].id1) == 0)
      scatterlist = &list1x4;
   else if ((result->people[0].id1 | result->people[1].id1 |
             result->people[4].id1 | result->people[5].id1) == 0)
      scatterlist = &list1x4rot;
   else if ((result->people[1].id1 | result->people[2].id1 |
             result->people[5].id1 | result->people[6].id1) == 0)
      scatterlist = &listdmd;
   else if ((result->people[0].id1 | result->people[3].id1 |
             result->people[4].id1 | result->people[7].id1) == 0)
      scatterlist = &listdmdrot;
   else
      fail("Can't put the setups back together.");

   expand::compress_setup(*scatterlist, result);
}



static void do_concept_all_8(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   static const int8_t expander[8] = {10, 13, 14, 1, 2, 5, 6, 9};

   int key = parseptr->concept->arg1;

   /* key =
      all 4 couples    : 0
      all 8            : 1
      all 8 (diamonds) : 2 */

   // First, turn an alamo into squared-set spots.
   if (ss->kind == s_alamo) {
      ss->do_matrix_expansion(CONCPROP__NEEDK_4X4, false);
   }

   if (key == 0 && ss->kind == s_thar && (ss->cmd.cmd_fraction.flags & CMD_FRAC_PART_BIT) != 0)
      key = 1;

   if (key == 0) {
      // This is "all 4 couples".
      if (  ss->kind != s4x4 ||
            (( ss->people[0].id1 | ss->people[3].id1 | ss->people[4].id1 | ss->people[7].id1 |
               ss->people[8].id1 | ss->people[11].id1 | ss->people[12].id1 | ss->people[15].id1) != 0) ||
            (((ss->people[ 1].id1 ^ ss->people[ 2].id1) & d_mask) != 0) ||
            (((ss->people[ 5].id1 ^ ss->people[ 6].id1) & d_mask) != 0) ||
            (((ss->people[ 9].id1 ^ ss->people[10].id1) & d_mask) != 0) ||
            (((ss->people[13].id1 ^ ss->people[14].id1) & d_mask) != 0) ||
            (ss->people[ 1].id1 != 0 && ((ss->people[ 1].id1) & 1) == 0) ||
            (ss->people[ 5].id1 != 0 && ((ss->people[ 5].id1) & 1) != 0) ||
            (ss->people[ 9].id1 != 0 && ((ss->people[ 9].id1) & 1) == 0) ||
            (ss->people[13].id1 != 0 && ((ss->people[13].id1) & 1) != 0))
         fail("Must be in squared set formation.");

      divided_setup_move(ss, MAPCODE(s2x2,2,MPKIND__ALL_8,0), phantest_ok, true, result);
   }
   else {
      // This is "all 8" or "all 8 (diamond)".

      // Turn a 2x4 into a 4x4, if people are facing reasonably.  If the centers are all
      // facing directly across the set, it might not be a good idea to allow this.
      if (ss->kind == s2x4 && key == 1) {
         uint32_t tbctrs =
            ss->people[1].id1 | ss->people[2].id1 | ss->people[5].id1 | ss->people[6].id1;
         uint32_t tbends =
            ss->people[0].id1 | ss->people[3].id1 | ss->people[4].id1 | ss->people[7].id1;

         if ((((ss->people[1].id1 ^ d_south) |   /* Don't allow it if centers are */
               (ss->people[2].id1 ^ d_south) |   /* facing directly accross the set. */
               (ss->people[5].id1 ^ d_north) |
               (ss->people[6].id1 ^ d_north)) & d_mask) &&
             (((tbends & 010) == 0 && (tbctrs & 1) == 0) ||    /* Like an alamo line. */
              ((tbends & 1) == 0 && (tbctrs & 010) == 0))) {   /* Like an alamo column. */
            setup temp = *ss;
            ss->kind = s4x4;
            ss->clear_people();
            scatter(ss, &temp, expander, 7, 0);
            canonicalize_rotation(ss);
         }
      }

      if (ss->kind == s_thar) {
         // Either one is legal in a thar.
         if (key == 1)
            divided_setup_move(ss, MAPCODE(s1x4,2,MPKIND__ALL_8,0), phantest_ok, true, result);
         else
            divided_setup_move(ss, MAPCODE(sdmd,2,MPKIND__ALL_8,0), phantest_ok, true, result);

         // The above stuff did an "elongate perpendicular to the long axis of the 1x4 or diamond"
         // operation, also known as an "ends' part of concentric" operation.  Some people believe
         // (perhaps as part of a confusion between "all 8" and "all 4 couples", or some other mistaken
         // notion) that they should always end on column spots.  Now it is true that "all 4 couples"
         // always ends on columns spots, but that's because it can only begin on column spots.
         // To avoid undue controversy or bogosity, we only allow the call if both criteria are met.
         // The "opposite elongation" criterion was already met, so we check for the mistaken
         // "end on column spots" criterion.  If someone really wants to hinge from a thar, they can
         // just say "hinge".

         /*  Not any longer.  Always go to columns.
         if (result->kind == s4x4) {
            if (((result->people[1].id1 | result->people[2].id1 | result->people[9].id1 | result->people[10].id1) & 010) ||
                ((result->people[14].id1 | result->people[5].id1 | result->people[6].id1 | result->people[13].id1) & 1))
               fail("Ending position is not defined.");
         }
         return;
         */
      }
      else if (key == 2)
         fail("Must be in a thar.");   // Can't do "all 8 (diamonds)" from squared-set spots.
      else if (ss->kind == s_crosswave) {
         ss->kind = s_thar;
         divided_setup_move(ss, MAPCODE(s1x4,2,MPKIND__ALL_8,0), phantest_ok, true, result);
      }
      else if (ss->kind == s4x4) {
         // This is "all 8" in a squared-set-type of formation.  This concept isn't really formally
         // defined here, except for the well-known cases like "all 8 spin the top", in which they
         // would step to a wave and then proceed from the resulting thar.  That is, it is known
         // to be legal in facing line-like elongation if everyone steps to a wave.  But it is
         // also called from column-like elongation, with everyone facing out, for things like
         // "all 8 shakedown", so we want to allow that.
         //
         // Perhaps, if people are in line-like elongation, we should have a cmd_misc bit saying
         // "must step to a wave" along with the bit saying "must not step to a wave".
         // To make matters worse, it might be nice to allow only some people to step to a wave
         // in a suitable rigger setup.
         //
         // Basically, no one knows exactly how this concept is supposed to work in all the
         // cases.  This isn't a problem for anyone except those people who have the misfortune
         // to try to write a computer program to do this stuff.

         // First, it's clearly only legal if on squared-set spots.

         if ((    ss->people[0].id1 | ss->people[3].id1 | ss->people[4].id1 | ss->people[7].id1 |
                  ss->people[8].id1 | ss->people[11].id1 | ss->people[12].id1 | ss->people[15].id1) != 0)
            fail("Must be on squared-set spots.");

         // If people started out line-like, we demand that they be facing in.  That is, we do not allow
         // "all quarter away from your partner, then all 8 shakedown", though we do allow "all
         // face your partner and all 8 square chain thru to a wave".  We are just being as conservative
         // as possible while allowing those cases that are commonly used.

         divided_setup_move(ss, MAPCODE(s2x2,2,MPKIND__ALL_8,0), phantest_ok, true, result);
      }
      else
         fail("Must be in a thar or squared-set spots.");
   }

   // If this ended in a thar, we accept it.  If not, we have the usual lines-to-lines/
   // columns-to-columns problem.  We don't know whether to enforce column spots, line spots,
   // perpendicular to the lines they had after stepping to a wave (if indeed they did so;
   // we don't know), to footprints from before stepping to a wave, or what.  So the only case
   // we allow is columns-to-columns.

   switch (result->kind) {
   case s_thar:
      break;   // No action needed.
   case s4x4:
      // Make sure the "anything-to-columns" rule is applied.
      if ((result->people[1].id1 & 010) || (result->people[14].id1 & 1))
         result->swap_people(1, 14);
      if ((result->people[2].id1 & 010) || (result->people[5].id1 & 1))
         result->swap_people(2, 5);
      if ((result->people[9].id1 & 010) || (result->people[6].id1 & 1))
         result->swap_people(9, 6);
      if ((result->people[10].id1 & 010) || (result->people[13].id1 & 1))
         result->swap_people(10, 13);

      // Check that we succeeded.  If the call ended T-boned, our zeal to get people
      // out of each others' way may have left people incorrect.

      if (((result->people[1].id1 | result->people[2].id1 | result->people[9].id1 | result->people[10].id1) & 010) ||
          ((result->people[14].id1 | result->people[5].id1 | result->people[6].id1 | result->people[13].id1) & 1))
         fail("People must end as if on column spots.");

      break;
   case s_bone:
      // Make sure the "anything-to-columns" rule is applied for the ends.
      if ((result->people[0].id1 & 010) & (result->people[1].id1 & 010) &
          (result->people[4].id1 & 010) & (result->people[5].id1 & 010)) {
         result->kind = s_qtag;   // Change it to a qtag.  This is all that's required.
      }
      else if ((result->people[0].id1 & 1) & (result->people[1].id1 & 1) &
               (result->people[4].id1 & 1) & (result->people[5].id1 & 1)) {
         // No action needed.
      }

      break;
   default:
      fail("Ending position is not defined.");
   }
}


static void do_concept_meta(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   parse_block *result_of_skip;
   uint32_t expirations_to_clearmisc = 0;
   uint32_t finalresultflagsmisc = 0;
   meta_key_kind key = (meta_key_kind) parseptr->concept->arg1;
   // Will point to things in case of something like "echo in roll circulate"
   parse_block *specialfirstptr = (parse_block *) 0;
   parse_block *specialsecondptr = (parse_block *) 0;

   prepare_for_call_in_series(result, ss);

   // We hardly ever care about the "thisislast" bit.
   fraction_command corefracs = ss->cmd.cmd_fraction;
   corefracs.flags &= ~CMD_FRAC_THISISLAST;
   uint32_t nfield = (corefracs.flags & CMD_FRAC_PART_MASK) / CMD_FRAC_PART_BIT;
   uint32_t kfield = (corefracs.flags & CMD_FRAC_PART2_MASK) / CMD_FRAC_PART2_BIT;

   if ((key == meta_key_random || key == meta_key_echo) &&
       ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_REVERSE)) {
      // "reverse" and "random"  ==>  "reverse random".
      // "reverse" and "echo"  ==>  "reverse echo".
      key = (meta_key_kind) ((int) key+1);
      ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_REVERSE);
   }

   // Now demand that no flags remain.
   if (ss->cmd.cmd_final_flags.final)
      fail("Illegal modifier for this concept.");

   if (key != meta_key_finally && key != meta_key_initially_and_finally &&
       key != meta_key_piecewise && key != meta_key_nth_part_work &&
       key != meta_key_first_frac_work &&
       key != meta_key_echo && key != meta_key_rev_echo)
      ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;   // We didn't do this before.

   // Some meta-concepts take a number, which might be wired into the concept ("shifty"),
   // or might be entered explicitly by the user ("shift <N>").

   uint32_t shiftynum =
      (concept_table[parseptr->concept->kind].concept_prop & CONCPROP__USE_NUMBER) ?
      parseptr->options.number_fields : parseptr->concept->arg2;

   // Some meta-concepts have a numeric parameter, such as "multiple echo",
   // or "do the first/last/middle M/N <concept>".
   int concept_option_code = parseptr->concept->arg2;

   // First, try simple ones, that don't take a concept.

   switch (key) {

   case meta_key_finish:
      if (corefracs.is_null())
         ss->cmd.cmd_fraction.flags = FRACS(CMD_FRAC_CODE_FROMTOREV,2,0);
      else if (corefracs.is_null_with_masked_flags(
                  CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK | CMD_FRAC_PART2_MASK,
                  CMD_FRAC_REVERSE))
         ss->cmd.cmd_fraction.flags += FRACS(CMD_FRAC_CODE_FROMTOREV,1,1);
      else if (corefracs.is_null_with_masked_flags(
                  CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK,
                  CMD_FRAC_CODE_FROMTOREV))
         // If we are already doing just parts N and later, just bump N.
         ss->cmd.cmd_fraction.flags += CMD_FRAC_PART_BIT;
      else if (corefracs.is_null_with_masked_flags(
                  CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK,
                  CMD_FRAC_CODE_SKIP_K_MINUS_HALF))
         // Same as above.
         ss->cmd.cmd_fraction.flags += CMD_FRAC_PART_BIT;
      else if (corefracs.is_null_with_masked_flags(
                  CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK,
                  CMD_FRAC_REVERSE | CMD_FRAC_CODE_FROMTOREV))
         // If we are already doing just parts N and later while reversed, just bump K.
         ss->cmd.cmd_fraction.flags += CMD_FRAC_PART2_BIT;
      else if (corefracs.is_null_with_masked_flags(
                  CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK,
                  CMD_FRAC_CODE_FROMTO))
         ss->cmd.cmd_fraction.flags += CMD_FRAC_PART_BIT+CMD_FRAC_PART2_BIT;
      else if (corefracs.is_null_with_masked_flags(
                  CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK,
                  CMD_FRAC_CODE_ONLY))
         // If we are already doing just part N only, just bump N.
         ss->cmd.cmd_fraction.flags += CMD_FRAC_PART_BIT;
      else if (corefracs.is_null_with_masked_flags(
                  CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK,
                  CMD_FRAC_CODE_ONLYREV))
         // If we are already doing just part N only in reverse order, do nothing.
         ;
      else if (corefracs.is_null_with_masked_flags(
                  CMD_FRAC_BREAKING_UP | CMD_FRAC_REVERSE |
                  CMD_FRAC_CODE_MASK | CMD_FRAC_PART_MASK,
                  CMD_FRAC_BREAKING_UP | FRACS(CMD_FRAC_CODE_FROMTOREV,1,0)))
         // If we are already doing up to some part, just "finish" that.
         ss->cmd.cmd_fraction.flags += CMD_FRAC_PART_BIT;
      else {
         // If there were incoming fractions (as opposed to incoming parts), this violates the general mechanism
         // of the program, that says that part concepts must come before fraction concepts.  Here is where we
         // fix that, so that we can do 3/4 finish swing the fractions as well as finish 3/5 swing the fractions.
         //
         // This constructor will only give us an OK indication if we are looking at a naked underlying call,
         // with no further concepts.  It also requires that there be no incoming parts concepts prior
         // to this concept.
         fraction_info ff(ss->cmd, 1);
         if (ff.m_instant_stop < 0)  // This is the error indication.
            fail("Can't stack meta or fractional concepts this way.");
         ss->cmd.cmd_fraction.set_to_null_with_flags(FRACS(CMD_FRAC_CODE_FROMTOREV,
                                                           ff.m_fetch_index+2,
                                                           ff.m_fetch_total-ff.m_highlimit));
      }

      // The "CMD_FRAC_THISISLAST" bit, if still set, might not tell the truth at this point.
      ss->cmd.cmd_fraction.flags &= ~CMD_FRAC_THISISLAST;

      if (ss->cmd.restrained_fraction.fraction) {
         if (ss->cmd.cmd_fraction.fraction != FRAC_FRAC_NULL_VALUE)
            fail("Fraction nesting is too complicated.");
         ss->cmd.cmd_fraction.fraction = ss->cmd.restrained_fraction.fraction;
         ss->cmd.restrained_fraction.fraction = 0;
      }

      move(ss, false, result);
      normalize_setup(result, simple_normalize, qtag_compress);
      return;

   case meta_key_like_a:

      // This is "like a".  Do the last part of the call.
      ss->cmd.cmd_fraction.set_to_null_with_flags(FRACS(CMD_FRAC_CODE_ONLYREV,1,0));

      // We generally don't allow any incoming fraction stuff.  However,
      // there are two cases that we allow.  If the incoming info was just "1/2",
      // we are being asked to do 1/2 of "like a whatever".  The order in which
      // parts and fractions are specified can't handle that correctly.  (If we
      // just set a fraction field of
      //        FRACS(CMD_FRAC_CODE_ONLYREV,1,0) + FRAC_FRAC_HALF_VALUE
      // it would have us do the last part whatever the first half of the call is.)
      // We want the first half of the last part.  But we can do it by using
      // the "CMD_FRAC_FIRSTHALF_ALL" flag, which says to apply the "first half"
      // mechanism *outside* of the part-selecting mechanism.

      if (corefracs.is_firsthalf())
         ss->cmd.cmd_fraction.flags |= CMD_FRAC_FIRSTHALF_ALL;
      else if (corefracs.is_lasthalf())
         ss->cmd.cmd_fraction.flags |= CMD_FRAC_LASTHALF_ALL;
      else if (!corefracs.is_null())
         fail("Can't stack meta or fractional concepts.");

      move(ss, false, result);
      normalize_setup(result, simple_normalize, qtag_compress);
      return;

   case meta_key_roundtrip:
      {

         // The "CMD_FRAC_THISISLAST" bit, if still set, might not tell the truth at this point.
         ss->cmd.cmd_fraction.flags &= ~CMD_FRAC_THISISLAST;

         if (ss->cmd.restrained_fraction.fraction)
            fail("Fraction nesting is too complicated.");

         // Handling nested roundtrips is both very complicated and very simple!

         // Convert the various fraction codes to a universal form that is useful for this concept.

         int NN = 0;
         int KK = 0;

         // If we are operating under some concept that directs specific parts to do, do so.
         if (corefracs.is_null_with_masked_flags(CMD_FRAC_CODE_MASK, CMD_FRAC_CODE_FROMTOREV)) {
            NN = nfield-1;
            KK = kfield;
         }
         else if (!corefracs.is_null()) {
            if (corefracs.is_null_with_masked_flags(CMD_FRAC_CODE_MASK | CMD_FRAC_PART2_MASK, CMD_FRAC_CODE_ONLY)) {
               // Being asked to do just one particular part of roundtrip XYZ.
               // First, do just that part of XYZ.  If the part is off the end, this won't do anything.

            // ****** This stuff may not be finished.  We are simply doing the indicated part of
            // the subject call.  What if the indicated part is in the second part of the roundtrip?

               result->cmd = ss->cmd;
               result->cmd.cmd_fraction.set_to_null_with_flags(FRACS(CMD_FRAC_CODE_ONLY, nfield, 0));
               goto finish_it;
            }

            // Otherwise, this must be a fraction concept that we can turn into specific start and end points,
            // which requires that the call can be found without any other concepts or flags that would make
            // the determination incorrect.

            parse_block *pp = ss->cmd.parseptr;
            // We allow nested roundtrips, of course.
            int rountrip_nesting_count = 1;
            /* Nested things may not work just yet.  Probably related to going off the end of the subject call.
         while (pp && pp->concept->kind == concept_meta && pp->concept->arg1 == meta_key_roundtrip) {
            pp = pp->next;
            rountrip_nesting_count++;
         }
         */

            int howmanyparts = try_to_get_parts_from_parse_pointer(ss, pp);

            if (howmanyparts < 0)
               fail("Sorry, fraction is too complicated.");

            while (--rountrip_nesting_count > 0)
               howmanyparts = howmanyparts*2-1;

            howmanyparts = howmanyparts*2-1;

            // We now have the number of parts for the complete roundtrip.

            if ((corefracs.flags & (CMD_FRAC_LASTHALF_ALL | CMD_FRAC_FIRSTHALF_ALL)) ||
                (corefracs.fraction & (CMD_FRAC_DEFER_HALF_OF_LAST | CMD_FRAC_DEFER_LASTHALF_OF_FIRST)))
               fail("Sorry, fraction is too complicated.");

            fraction_info zzz(howmanyparts);
            zzz.get_fraction_info(corefracs,
                                  CFLAG1_VISIBLE_FRACTION_MASK / CFLAG1_VISIBLE_FRACTION_BIT,
                                  weirdness_off);

            if (zzz.m_reverse_order || zzz.m_do_half_of_last_part != 0 || zzz.m_do_last_half_of_first_part)
               fail("Sorry, fraction is too complicated.");

            NN = zzz.m_fetch_index;
            KK = howmanyparts-zzz.m_highlimit;
         }

         // Now do the roundtrip, skipping the first NN parts and the last KK parts.  Of the whole roundtrip.
         // The incoming CMD_FRAC_REVERSE bit is ignored, since this is a palindrome.

         // Do the first part of the trip, that is, the whole call, skipping the first NN parts.

         if (NN==0 && KK==0)
            // We can do this with with a completely null specifier (that is, CMD_FRAC_CODE_ONLY[0,0],
            // which may make other parts of the code happier.
            result->cmd.cmd_fraction.set_to_null_with_flags(FRACS(CMD_FRAC_CODE_ONLY, 0, 0));
         else
            result->cmd.cmd_fraction.set_to_null_with_flags(FRACS(CMD_FRAC_CODE_FROMTOREV, NN+1, 0));

         do_call_in_series_simple(result);

         // Do the last part of the trip, skipping the last KK parts.  That means reversing the order of the
         // call parts, and skipping the first part (that's the last part of the subject call), and the last
         // KK parts (that's the first KK parts of the subject call.)

         result->cmd = ss->cmd;
         result->cmd.cmd_fraction.set_to_null_with_flags(FRACS(CMD_FRAC_REVERSE|CMD_FRAC_CODE_FROMTOREV, 2, KK));
         goto finish_it;
      }

   case meta_key_revorder:
      result->cmd.cmd_fraction.flags ^= CMD_FRAC_REVERSE;
      goto finish_it;

   case meta_key_skip_nth_part:
      if (!corefracs.is_null())
         fail("Can't stack meta or fractional concepts.");

      // Do the initial part, if any.

      if (parseptr->options.number_fields > 1) {
         // Set the fractionalize field to do the first few parts of the call.
         result->cmd.cmd_fraction.set_to_null_with_flags(
            FRACS(CMD_FRAC_CODE_FROMTO,(parseptr->options.number_fields-1),0));

         do_call_in_series_and_update_bits(result);
         result->cmd = ss->cmd;
      }

      // Do the final part.
      result->cmd.cmd_fraction.set_to_null_with_flags(
         FRACS(CMD_FRAC_CODE_FROMTOREV,parseptr->options.number_fields+1,0));
      goto finish_it;

   case meta_key_skip_last_part:
      if (!corefracs.is_null())
         fail("Can't stack meta or fractional concepts.");

      result->cmd.cmd_fraction.set_to_null_with_flags(
         FRACS(CMD_FRAC_CODE_FROMTOREV,1,1));
      goto finish_it;

   case meta_key_shift_n:
   case meta_key_shift_half:

      // Some form of shift <N>.

      if (key == meta_key_shift_half) {
         shiftynum++;

         if (!corefracs.is_null())
            fail("Fractional shift doesn't have parts.");

         // Do the last (shifted) part.

         result->cmd.cmd_fraction.set_to_null_with_flags(
            FRACS(CMD_FRAC_CODE_LATEFROMTOREV,shiftynum,0) | CMD_FRAC_BREAKING_UP);

         do_call_in_series_and_update_bits(result);
         result->cmd = ss->cmd;

         // Do the initial part up to the shift point.

         result->cmd.cmd_fraction.set_to_null_with_flags(
            FRACS(CMD_FRAC_CODE_FROMTOMOST,shiftynum,0) | CMD_FRAC_BREAKING_UP);

         goto rollover_and_finish_it;
      }
      else {         // Not fractional shift.
         uint32_t incoming_break_flags = corefracs.flags & CMD_FRAC_BREAKING_UP;

         // We allow "reverse order".
         if (corefracs.is_null_with_exact_flags(CMD_FRAC_REVERSE)) {
            result->cmd.cmd_fraction.set_to_null_with_flags(
               CMD_FRAC_BREAKING_UP | CMD_FRAC_REVERSE |
               FRACS(CMD_FRAC_CODE_FROMTOREVREV,shiftynum,0));

            // Do the last (shifted) part.
            do_call_in_series_and_update_bits(result);
            result->cmd = ss->cmd;

            // Do the initial part up to the shift point.
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_FROMTOREV,1,shiftynum) |
               CMD_FRAC_BREAKING_UP | CMD_FRAC_REVERSE);

            goto rollover_and_finish_it;
         }
         else if (corefracs.is_null_with_masked_flags(
                     ~(CMD_FRAC_BREAKING_UP|CMD_FRAC_PART_MASK|CMD_FRAC_PART2_MASK),
                     CMD_FRAC_CODE_FROMTOREV)) {
            result->cmd.cmd_fraction.set_to_null_with_flags(
               incoming_break_flags |
               CMD_FRAC_CODE_FROMTOREV |
               ((shiftynum * CMD_FRAC_PART_BIT) +
                (corefracs.flags & CMD_FRAC_PART_MASK)));

            // See whether we will do the early part.
            if (shiftynum == kfield) {
               goto finish_it;    // That's all.
            }
            else if (shiftynum <= kfield)
               fail("Can't stack these meta or fractional concepts.");

            do_call_in_series_and_update_bits(result);

            result->cmd = ss->cmd;
            // Do the initial part up to the shift point.
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_FROMTO,shiftynum-kfield,0) | incoming_break_flags);

            goto rollover_and_finish_it;
         }
         else if (corefracs.is_null_with_masked_flags(
                     ~(CMD_FRAC_BREAKING_UP|CMD_FRAC_PART_MASK),
                     CMD_FRAC_CODE_ONLYREV)) {
            if (shiftynum < nfield)
               fail("Can't stack these meta or fractional concepts.");
            result->cmd.cmd_fraction.set_to_null_with_flags(
               incoming_break_flags | FRACS(CMD_FRAC_CODE_ONLY,shiftynum+1-nfield,0));
         }
         else if (corefracs.is_null_with_masked_flags(
                     ~(CMD_FRAC_PART_MASK | CMD_FRAC_BREAKING_UP),
                     CMD_FRAC_CODE_ONLY) &&
                  nfield != 0) {
            result->cmd.cmd_fraction.set_to_null_with_flags(
               incoming_break_flags | FRACS(CMD_FRAC_CODE_ONLY,shiftynum+nfield,0));
         }
         else if (corefracs.is_null()) {
            result->cmd.cmd_fraction.set_to_null_with_flags(
               CMD_FRAC_BREAKING_UP | FRACS(CMD_FRAC_CODE_FROMTOREV,shiftynum+1,0));
            do_call_in_series_and_update_bits(result);
            result->cmd = ss->cmd;

            // Do the initial part up to the shift point.
            result->cmd.cmd_fraction.set_to_null_with_flags(
               CMD_FRAC_BREAKING_UP | FRACS(CMD_FRAC_CODE_FROMTO,shiftynum,0));

            goto rollover_and_finish_it;
         }
         else {
            fraction_info ff(ss->cmd, 0);
            if (ff.m_instant_stop < 0)  // This is the error indication.
               fail("Can't stack meta or fractional concepts this way.");

            int S = shiftynum;          // Shift amount.  We defer this many parts of subject call.
            int N = ff.m_fetch_total;   // Number of parts of call, same as number of things we will do.
            int A = ff.m_fetch_index;   // Starting point, due to incoming fraction.
            int Z = ff.m_highlimit;     // Ending point, due to incoming fraction.

            if (S < 1 || S >= N || A >= N)
               fail("Can't do this.");

            if (A < N-S) {
               if (Z > N-S) {
                  // Do S+A+1 ... N, followed by 1 ... Z+S-N.
                  result->cmd.cmd_fraction.set_to_null_with_flags(
                     CMD_FRAC_BREAKING_UP | FRACS(CMD_FRAC_CODE_FROMTOREV,S+A+1,0));
                  do_call_in_series_and_update_bits(result);

                  result->cmd = ss->cmd;
                  result->cmd.cmd_fraction.set_to_null_with_flags(
                     CMD_FRAC_BREAKING_UP | FRACS(CMD_FRAC_CODE_FROMTO,Z+S-N,0));

                  goto rollover_and_finish_it;
               }
               else {
                  // Do S+A+1 ... S+Z
                  result->cmd.cmd_fraction.set_to_null_with_flags(
                     CMD_FRAC_BREAKING_UP | FRACS(CMD_FRAC_CODE_FROMTOREV,S+A+1,N-Z-S));

                  goto rollover_and_finish_it;
               }
            }
            else {
               // Do A+S-N+1 ... Z+S-N.
               result->cmd.cmd_fraction.set_to_null_with_flags(
                  CMD_FRAC_BREAKING_UP | FRACS(CMD_FRAC_CODE_FROMTOREV,A+S-N+1,2*N-Z-S));

               goto rollover_and_finish_it;
            }
         }
      }

      goto finish_it;
   }

   // Special cases:  Watch for things like "echo in roll circulate".  The call "circulate"
   // can take an "in roll" modifier, so that, if "in roll" and "circulate" are typed separately,
   // the right things will happen.  But "in roll circulate" is also a call in its own right
   // because of stuff like "in roll motivate".  There's a huge amount of mechanism for dealing
   // with that, that we can't really change.  So we might see the subject of "echo" being just
   // the call "in roll circulate", and we have to act as though it were actually
   // "in roll" + "circulate".  These special cases are actually very simple.  We just pull them
   // out and do them, by rewriting the parse chain.  Figuring out how to do that is not simple.

   if (key == meta_key_echo || key == meta_key_rev_echo) {
      if (ss->cmd.parseptr->concept->kind == marker_end_of_list) {
         if (ss->cmd.parseptr->call && ss->cmd.parseptr->call->the_defn.callflags1 & CFLAG1_BASE_CIRC_CALL) {
            if (ss->cmd.parseptr->call != base_calls[base_call_ctrrot] &&
                ss->cmd.parseptr->call != base_calls[base_call_splctrrot]) {
               specialfirstptr = parse_block::get_parse_block();
               specialsecondptr = parse_block::get_parse_block();
               specialfirstptr->call = (call_with_name *) 0;
               specialfirstptr->call_to_print = (call_with_name *) 0;
               specialfirstptr->next = specialsecondptr;
               specialfirstptr->concept = circcer_calls[ss->cmd.parseptr->call->the_defn.circcer_index].the_concept;
               specialsecondptr->concept = &concept_mark_end_of_list;
               specialsecondptr->call = base_calls[base_call_circulate];
               specialsecondptr->call_to_print = base_calls[base_call_circulate];
               // This isn't supposed to have failed, but future database changes, done carelessly,
               // could lead to failure, and we don't want the program to crash.
               if (!specialfirstptr->concept) {
                  // Just abandon the objects.  They will get garbage-collected.
                  // This willmake the "echo" operation fail, of course.
                  specialfirstptr = (parse_block *) 0;
                  specialsecondptr = (parse_block *) 0;
               }
            }
         }
      }
   }

   // The remaining ones take one (or more, in the case of randomize or multiple echo) subconcept.
   // Do some initial preparation.

   {
      // Scan the modifiers, remembering the and their end point.  The reason for this is to
      // avoid getting screwed up by a commentf stuff to do it really right.  It isn't
      // worth it, and isn't worth holding up "ra, which counts as a modifier.  YUK!!!!!!
      // This code used to have the beginnings ondom left" for.  (Previous sentence written several
      // years ago; I wonder what on Earth I was talking about?)  In any case, the stupid handling
      // of comments will go away soon.  (Yeah, right!)

      setup_command yescmd = ss->cmd;
      setup_command nocmd = ss->cmd;

      if (specialfirstptr) {
         yescmd.parseptr = specialfirstptr;
         nocmd.parseptr = specialfirstptr;
      }

      // The CMD_MISC3__PUT_FRAC_ON_FIRST bit tells the "special_sequential" concept
      // (if that is the subject concept) that fractions are allowed, and they
      // are to be applied to the first call only.
      yescmd.cmd_misc3_flags |= CMD_MISC3__PUT_FRAC_ON_FIRST;
      yescmd.parseptr->more_finalherit_flags.set_finalbit(FINAL__UNDER_RANDOM_META);

      // This makes "paranoid", if used directly after this concept, pass this concept
      // (initially, thirdly, etc.) on to the subject call.  So "secondly paranoid swing
      // the fractions" will pick out the second part of swing the fractions.  If the
      // flag is off, which will happen if there is an intervening concept, this concept
      // will be taken directly by the "paranoid" concept.  So "secondly stable paranoid
      // swing the fractions" will make the second part of the "paranoid" concept itself
      // be picked out for application of "stable".
      yescmd.cmd_misc3_flags |= CMD_MISC3__PARTS_OVER_THIS_CONCEPT;

      // This makes it complain about things like "initially stable cross concentric mix".
      nocmd.cmd_misc3_flags |= CMD_MISC3__META_NOCMD;

      // Foo1 finds the boundaries of the first subconcept.  (Foo2 wil show the second subconcept.)
      skipped_concept_info foo1(specialfirstptr ? specialfirstptr : parseptr->next);

      if (foo1.m_heritflag != 0ULL) {
         result_of_skip = foo1.m_concept_with_root;
         yescmd.parseptr = result_of_skip;
         yescmd.cmd_misc3_flags |= CMD_MISC3__RESTRAIN_MODIFIERS;
         yescmd.restrained_final = 0;
         yescmd.restrained_super8flags = foo1.m_heritflag;
      }
      else {
         result_of_skip = foo1.m_result_of_skip;
         yescmd.parseptr = foo1.m_old_retval;

         if ((foo1.m_need_to_restrain & 2) ||
             ((foo1.m_need_to_restrain & 1) &&
              (key != meta_key_rev_echo && key != meta_key_echo))) {
            yescmd.restrained_concept = foo1.m_old_retval;
            yescmd.cmd_misc3_flags |= CMD_MISC3__RESTRAIN_CRAZINESS;
            if (foo1.m_skipped_concept->concept->kind == concept_supercall)
               yescmd.cmd_misc3_flags |= CMD_MISC3__SUPERCALL;
            yescmd.restrained_final = foo1.m_root_of_result_of_skip;
            yescmd.parseptr = result_of_skip;
         }
      }

      if (key != meta_key_echo && foo1.m_nocmd_misc3_bits != 0)
         fail("Can't perform this substitution.");

      nocmd.parseptr = result_of_skip;
      nocmd.cmd_misc3_flags |= foo1.m_nocmd_misc3_bits;

      // If the skipped concept is "twisted" or "yoyo", get ready to clear
      // the expiration bit for same, if we do it "piecewise" or whatever.

      switch (foo1.m_skipped_concept->concept->kind) {
      case concept_yoyo:
         expirations_to_clearmisc = RESULTFLAG__YOYO_ONLY_EXPIRED;
         break;
      case concept_generous:
      case concept_stingy:
         expirations_to_clearmisc = RESULTFLAG__GEN_STING_EXPIRED;
         break;
      case concept_twisted:
         expirations_to_clearmisc = RESULTFLAG__TWISTED_EXPIRED;
         break;
      case concept_rectify:
         expirations_to_clearmisc = RESULTFLAG__RECTIFY_EXPIRED;
         break;
      }

      yescmd.parseptr->more_finalherit_flags.set_finalbit(FINAL__UNDER_RANDOM_META);

      switch (key) {

      case meta_key_echo:
         if (!(yescmd.cmd_misc3_flags & CMD_MISC3__RESTRAIN_CRAZINESS)) {
            if (corefracs.is_null_with_masked_flags(
                   CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK | CMD_FRAC_PART_MASK,
                   FRACS(CMD_FRAC_CODE_ONLY,1,0))) {
               if (concept_option_code != 1)
                  fail("Can't stack meta or fractional concepts this way.");
               finalresultflagsmisc |= RESULTFLAG__PARTS_ARE_KNOWN;
               result->cmd = yescmd;
               result->cmd.cmd_fraction.set_to_null();
               goto finish_it;
            }
            else if (corefracs.is_null_with_masked_flags(
                        CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK | CMD_FRAC_PART_MASK,
                        FRACS(CMD_FRAC_CODE_ONLY,2,0))) {
               if (concept_option_code != 1)
                  fail("Can't stack meta or fractional concepts this way.");
               finalresultflagsmisc |= RESULTFLAG__PARTS_ARE_KNOWN;
               finalresultflagsmisc |= RESULTFLAG__DID_LAST_PART;
               result->cmd = nocmd;
               result->cmd.cmd_fraction.set_to_null();
               goto finish_it;
            }
            else if (corefracs.is_null_with_masked_flags(
                        CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK | CMD_FRAC_PART_MASK,
                        FRACS(CMD_FRAC_CODE_FROMTOREV,2,0))) {
               if (concept_option_code != 1)
                  fail("Can't stack meta or fractional concepts this way.");
               finalresultflagsmisc |= RESULTFLAG__PARTS_ARE_KNOWN | RESULTFLAG__DID_LAST_PART;
               result->cmd = nocmd;
               result->cmd.cmd_fraction.set_to_null();
               goto finish_it;
            }
            else if (!result->cmd.cmd_fraction.is_null() && result->cmd.cmd_fraction.flags == 0) {
               if (concept_option_code != 1)
                  fail("Can't stack meta or fractional concepts this way.");

               int sn = result->cmd.cmd_fraction.start_numer();
               int sd = result->cmd.cmd_fraction.start_denom();
               int en = result->cmd.cmd_fraction.end_numer();
               int ed = result->cmd.cmd_fraction.end_denom();

               if (ed >= 2*en) {
                  // This ends halfway through or less.  Just do (part of) the "yes" stuff.
                  result->cmd = yescmd;
                  result->cmd.cmd_fraction.set_to_null();
                  result->cmd.cmd_fraction.repackage(sn<<1, sd, en<<1, ed);
                  goto finish_it;
               }
               else if (sd <= 2*sn) {
                  // This begins halfway through or more.  Just do (part of) the "no" stuff.
                  result->cmd = nocmd;
                  result->cmd.cmd_fraction.set_to_null();
                  result->cmd.cmd_fraction.repackage((sn<<1)-sd, sd, (en<<1)-ed, ed);
                  goto finish_it;
               }
               else {
                  // Some in each half.
                  result->cmd = yescmd;
                  result->cmd.cmd_fraction.set_to_null();
                  result->cmd.cmd_fraction.repackage(sn<<1, sd, 1, 1);
                  do_call_in_series_and_update_bits(result);
                  result->cmd = nocmd;
                  // Assumptions don't carry through.
                  result->cmd.cmd_assume.assumption = cr_none;
                  result->cmd.cmd_fraction.set_to_null();
                  result->cmd.cmd_fraction.repackage(0, 1, (en<<1)-ed, ed);
                  goto finish_it;
               }
            }
         }

         if (corefracs.is_null_with_masked_flags(
                CMD_FRAC_CODE_MASK | CMD_FRAC_PART_MASK | CMD_FRAC_PART2_MASK,
                FRACS(CMD_FRAC_CODE_FROMTOREV,1,1))) {
            if (concept_option_code != 1)
               fail("Can't stack meta or fractional concepts this way.");
            result->cmd = yescmd;
            result->cmd.cmd_fraction.set_to_null();
            goto finish_it;
         }
         else if (corefracs.is_null_with_masked_flags(
                     CMD_FRAC_CODE_MASK | CMD_FRAC_PART_MASK,
                     FRACS(CMD_FRAC_CODE_ONLYREV,1,0))) {
            if (concept_option_code != 1)
               fail("Can't stack meta or fractional concepts this way.");
            result->cmd = nocmd;
            result->cmd.cmd_fraction.set_to_null();
            goto finish_it;
         }

         if (!result->cmd.cmd_fraction.is_null())
            fail("Can't stack meta or fractional concepts this way.");

         // Do the call with the concept.
         result->cmd = yescmd;

         for (;;) {
            do_call_in_series_and_update_bits(result);

            // Set up so that next round will be without the concept.
            result->cmd = nocmd;

            // Assumptions don't carry through.
            result->cmd.cmd_assume.assumption = cr_none;

            if (--concept_option_code <= 0) break;   // Do it, the last time.

            // There will be future rounds; set nocmd so that the following round will be correct.
            skipped_concept_info foo2(nocmd.parseptr);
            nocmd.parseptr = foo2.get_next();
            nocmd.cmd_misc3_flags |= foo2.m_nocmd_misc3_bits;
         }

         goto finish_it;

      case meta_key_rev_echo:
         if (!(yescmd.cmd_misc3_flags & CMD_MISC3__RESTRAIN_CRAZINESS)) {
            if (result->cmd.cmd_fraction.is_firsthalf()) {
               result->cmd = nocmd;
               result->cmd.cmd_fraction.set_to_null();
               goto finish_it;
            }
            else if (result->cmd.cmd_fraction.is_lasthalf()) {
               result->cmd = yescmd;
               result->cmd.cmd_fraction.set_to_null();
               goto finish_it;
            }
         }

         if (!corefracs.is_null() || !result->cmd.cmd_fraction.is_null())
            fail("Can't stack meta or fractional concepts this way.");

         {
            parse_block *nosave = nocmd.parseptr;

            while (concept_option_code > 0) {
               for (int nn = concept_option_code; nn > 1 ; nn--) {
                  skipped_concept_info foo2(nocmd.parseptr);
                  nocmd.parseptr = foo2.m_result_of_skip;
               }

               result->cmd = nocmd;
               do_call_in_series_and_update_bits(result);
               nocmd.parseptr = nosave;

               concept_option_code--;
            }

            result->cmd = yescmd;

            // Assumptions don't carry through.
            result->cmd.cmd_assume.assumption = cr_none;
         }

         goto finish_it;

      case meta_key_randomize:
         {
            if (concept_option_code <= 1)
               fail("Need two concepts.");

            if (!result->cmd.cmd_fraction.is_null())
               fail("Can't stack meta or fractional concepts this way.");

            int index = 0;
            fraction_command frac_stuff = corefracs;
            frac_stuff.flags &= ~(CMD_FRAC_CODE_MASK|CMD_FRAC_PART_MASK|CMD_FRAC_PART2_MASK);

            uint32_t saved_last_flagmisc = 0;

            foo1.get_next()->say_and = true;

            skipped_concept_info foo2(foo1.get_next());
            if (concept_option_code == 3) foo2.get_next()->say_and = true;

            // If randomizing among 3, get a third "skipped_concept_info" item.  If only
            // between two, just make it an unused copy of earlier one.
            skipped_concept_info foo3((concept_option_code == 3) ? foo2.get_next() : foo1.get_next());

            do {
               copy_cmd_preserve_elong_and_expire(ss, result);

               switch (index % concept_option_code) {
               case 0:
                  // First concept.
                  {
                     result->cmd = yescmd;
                     result->cmd.restrained_super8flags = 0ULL;

                     // Set the fractionalize field to do the indicated part.
                     result->cmd.cmd_fraction = frac_stuff;
                     result->cmd.cmd_fraction.flags |=
                        FRACS(CMD_FRAC_CODE_ONLY,index+1,0) | CMD_FRAC_BREAKING_UP;
                     result->result_flags.misc &= ~expirations_to_clearmisc;
                     result->cmd.cmd_misc_flags &= ~CMD_MISC__NO_EXPAND_MATRIX;

                     // Splice out the second concept group.

                     // If the call that we are doing has the RESULTFLAG__NO_REEVALUATE flag
                     // on (meaning we don't re-evaluate under *any* circumstances, particularly
                     // circumstances like these), we do not re-evaluate the ID bits.
                     if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
                        result->update_id_bits();

                     // Skip the second concept.  If the first is an actual concept,
                     // it requires careful splicing of pointers.  But for heritflag stuff,
                     // just leave the restrained_super8flags stuff in place and start at
                     // the 3rd concept.

                     if (foo1.m_heritflag != 0ULL) {
                        result->cmd.restrained_super8flags = foo1.m_heritflag;
                        result->cmd.parseptr = (concept_option_code == 3) ?
                           foo3.m_result_of_skip :
                           foo2.m_result_of_skip;
                        result->cmd.cmd_misc3_flags |= CMD_MISC3__RESTRAIN_MODIFIERS;
                        result->cmd.restrained_final = 0;
                        do_call_in_series_and_update_bits(result);
                     }
                     else {
                        // Do the actual pointer splicing.
                        // Preserved across a throw; must be volatile.
                        if (!foo1.m_root_of_result_of_skip)
                           fail("Need a concept.");
                        volatile error_flag_type maybe_throw_this = error_flag_none;
                        parse_block *save_it = *foo1.m_root_of_result_of_skip;
                        *foo1.m_root_of_result_of_skip = (concept_option_code == 3) ?
                           foo3.m_result_of_skip :
                           foo2.m_result_of_skip;
                        try {
                           do_call_in_series_and_update_bits(result);
                        }
                        catch(error_flag_type foo) {
                           // An error occurred.  We need to restore stuff.
                           maybe_throw_this = foo;
                        }

                        // Restore what was spliced out.
                        *foo1.m_root_of_result_of_skip = save_it;

                        // If an error occurred, throw it now.
                        if (maybe_throw_this != error_flag_none)
                           throw maybe_throw_this;
                     }
                  }

                  break;
               case 1:
                  // Second concept.
                  result->cmd = nocmd;
                  result->cmd.restrained_super8flags = 0ULL;

                  // Set the fractionalize field to do the indicated part.
                  result->cmd.cmd_fraction = frac_stuff;
                  result->cmd.cmd_fraction.flags |=
                     FRACS(CMD_FRAC_CODE_ONLY,index+1,0) | CMD_FRAC_BREAKING_UP;
                  result->cmd.cmd_misc_flags &= ~CMD_MISC__NO_EXPAND_MATRIX;

                  // If the call that we are doing has the RESULTFLAG__NO_REEVALUATE flag
                  // on (meaning we don't re-evaluate under *any* circumstances, particularly
                  // circumstances like these), we do not re-evaluate the ID bits.
                  if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
                     result->update_id_bits();

                  // If there are 3 concepts, this is the middle one, and more splicing is required.
                  if (concept_option_code == 3) {
                     if (foo2.m_heritflag != 0) {
                        result->cmd.parseptr = foo3.m_result_of_skip;
                        result->cmd.cmd_misc3_flags |= CMD_MISC3__RESTRAIN_MODIFIERS;
                        result->cmd.restrained_final = 0;
                        result->cmd.restrained_super8flags = foo2.m_heritflag;
                        do_call_in_series_and_update_bits(result);
                     }
                     else {
                        // Preserved across a throw; must be volatile.
                        if (!foo2.m_root_of_result_of_skip)
                           fail("Need a concept.");
                        volatile error_flag_type maybe_throw_this = error_flag_none;
                        parse_block *save_it = *foo2.m_root_of_result_of_skip;
                        *foo2.m_root_of_result_of_skip = foo3.m_result_of_skip;
                        try {
                           do_call_in_series_and_update_bits(result);
                        }
                        catch(error_flag_type foo) {
                           // An error occurred.  We need to restore stuff.
                           maybe_throw_this = foo;
                        }

                        // Restore what was spliced out.
                        *foo2.m_root_of_result_of_skip = save_it;

                        // If an error occurred, throw it now.
                        if (maybe_throw_this != error_flag_none)
                           throw maybe_throw_this;
                     }
                  }
                  else {
                     do_call_in_series_and_update_bits(result);
                  }

                  break;
               case 2:
                  // Third concept.
                  result->cmd = nocmd;
                  result->cmd.parseptr = foo2.m_result_of_skip;
                  result->cmd.restrained_super8flags = 0ULL;

                  // Set the fractionalize field to do the indicated part.
                  result->cmd.cmd_fraction = frac_stuff;
                  result->cmd.cmd_fraction.flags |=
                     FRACS(CMD_FRAC_CODE_ONLY,index+1,0) | CMD_FRAC_BREAKING_UP;
                  result->cmd.cmd_misc_flags &= ~CMD_MISC__NO_EXPAND_MATRIX;

                  // If the call that we are doing has the RESULTFLAG__NO_REEVALUATE flag
                  // on (meaning we don't re-evaluate under *any* circumstances, particularly
                  // circumstances like these), we do not re-evaluate the ID bits.
                  if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
                     result->update_id_bits();


                  if (foo3.m_heritflag != 0ULL) {
                     result->cmd.parseptr = foo3.m_result_of_skip;
                     result->cmd.cmd_misc3_flags |= CMD_MISC3__RESTRAIN_MODIFIERS;
                     result->cmd.restrained_final = 0;
                     result->cmd.restrained_super8flags = foo3.m_heritflag;
                  }

                  do_call_in_series_and_update_bits(result);
                  break;
               }

               index++;

               if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
                  result->update_id_bits();

               // Now look at the "DID_LAST_PART" bits of the call we just executed,
               // to see whether we should break out.
               if (!(result->result_flags.misc & RESULTFLAG__PARTS_ARE_KNOWN)) {
                  // Problem.  The call doesn't know what it did.
                  // Perhaps the previous part knew.  This could arise if we
                  // do something like "random use flip back in swing the fractions" --
                  // various parts aren't actually done, but enough parts are done
                  // for us to piece together the required information.

                  result->result_flags.misc &= ~RESULTFLAG__PART_COMPLETION_BITS;

                  if ((saved_last_flagmisc & (RESULTFLAG__PARTS_ARE_KNOWN|RESULTFLAG__DID_LAST_PART))
                      == RESULTFLAG__PARTS_ARE_KNOWN) {
                     result->result_flags.misc |= RESULTFLAG__PARTS_ARE_KNOWN;
                     if (saved_last_flagmisc & RESULTFLAG__DID_NEXTTOLAST_PART)
                        result->result_flags.misc |= RESULTFLAG__DID_LAST_PART;
                  }

                  saved_last_flagmisc = 0;   // Need to recharge before can use it again.
               }
               else
                  saved_last_flagmisc = result->result_flags.misc;

               if (!(result->result_flags.misc & RESULTFLAG__PARTS_ARE_KNOWN))
                  fail("Can't have 'no one' do a call.");

               // Assumptions don't carry through.
               result->cmd.cmd_assume.assumption = cr_none;
            }
            while (!(result->result_flags.misc & RESULTFLAG__DID_LAST_PART));

            goto final_fixup;
         }

      case meta_key_first_frac_work:
         {
            // This is "do the first/last/middle M/N <concept>", or "halfway".

            if (!corefracs.is_null())
               fail("Can't stack meta or fractional concepts.");

            uint32_t stuff = (concept_option_code == 3) ? NUMBER_FIELDS_2_1 : parseptr->options.number_fields;
            int numer = stuff & NUMBER_FIELD_MASK;
            int denom = (stuff >> BITS_PER_NUMBER_FIELD) & NUMBER_FIELD_MASK;

            // Check that user isn't doing something stupid.
            if (numer <= 0 || numer >= denom)
               fail("Illegal fraction.");

            fraction_command afracs;
            fraction_command bfracs;
            fraction_command cfracs;

            afracs.fraction = ~0U;
            bfracs.fraction = ~0U;
            cfracs.fraction = ~0U;

            switch (concept_option_code) {
            case 1:
               // This is "last M/N".
               afracs.process_fractions(NUMBER_FIELDS_1_0, stuff, FRAC_INVERT_END, corefracs);
               bfracs.process_fractions(stuff, NUMBER_FIELDS_1_1, FRAC_INVERT_START, corefracs);
               break;
            case 2:
               // This is "middle M/N".
               {
                  uint32_t middlestuff = stuff + denom*((1<<BITS_PER_NUMBER_FIELD)+1);
                  afracs.process_fractions(NUMBER_FIELDS_1_0, middlestuff, FRAC_INVERT_END, corefracs);
                  bfracs.process_fractions(middlestuff, middlestuff, FRAC_INVERT_START, corefracs);
                  cfracs.process_fractions(middlestuff, NUMBER_FIELDS_1_1, FRAC_INVERT_NONE, corefracs);
               }
               break;
            default:
               // This is "halfway" (3) or "first M/N" (0).
               bfracs.process_fractions(NUMBER_FIELDS_1_0, stuff, FRAC_INVERT_NONE, corefracs);
               cfracs.process_fractions(stuff, NUMBER_FIELDS_1_1, FRAC_INVERT_NONE, corefracs);
               break;
            }

            // Do afracs without.
            if (afracs.fraction != ~0U) {
               result->cmd = nocmd;
               result->cmd.parseptr = result_of_skip;      // Skip over the concept.
               result->cmd.cmd_fraction.flags = CMD_FRAC_BREAKING_UP;
               result->cmd.cmd_fraction.fraction = afracs.fraction;
               do_call_in_series_and_update_bits(result);
            }

            // Do bfracs with.
            if (bfracs.fraction != ~0U) {
               result->cmd = yescmd;
               result->cmd.cmd_fraction.flags = corefracs.flags | CMD_FRAC_BREAKING_UP;
               result->cmd.cmd_fraction.fraction = bfracs.fraction;
               result->cmd.prior_expire_bits |= RESULTFLAG__PRESERVE_INCOMING_EXPIRATIONS;
               do_call_in_series_and_update_bits(result);
            }

            // Do cfracs without.
            if (cfracs.fraction != ~0U) {
               result->cmd = nocmd;
               result->cmd.parseptr = result_of_skip;      // Skip over the concept.
               result->cmd.cmd_fraction.flags = CMD_FRAC_BREAKING_UP;
               result->cmd.cmd_fraction.fraction = cfracs.fraction;
               result->cmd.prior_expire_bits |= RESULTFLAG__PRESERVE_INCOMING_EXPIRATIONS;
               do_call_in_series_simple(result);
            }

            goto final_fixup;
         }
      case meta_key_nth_part_work:

         if (corefracs.is_null_with_exact_flags(
            FRACS(CMD_FRAC_CODE_ONLY,shiftynum,0) | CMD_FRAC_BREAKING_UP)) {
            // We are being asked to do just the selected part, because of another
            // "initially"/"secondly", etc.  Just pass it through.
            result->cmd = yescmd;
         }
         else if (corefracs.is_null()) {
            // Simple case; no incoming fractions.
            // Do the initial part, if any, without the concept.

            if (shiftynum > 1) {
               result->cmd = nocmd;
               // Set the fractionalize field to do the first few parts of the call.
               result->cmd.cmd_fraction.set_to_null_with_flags(
                  FRACS(CMD_FRAC_CODE_FROMTO,shiftynum-1,0));
               do_call_in_series_and_update_bits(result);
            }

            // Do the part of the call that needs the concept.

            result->cmd = yescmd;
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_ONLY,shiftynum,0) | CMD_FRAC_BREAKING_UP);
            result->result_flags.misc |= RESULTFLAG__EXPIRATION_ENAB;
            do_call_in_series_and_update_bits(result);

            // Is this the right thing to do?
            // Cf. tests pt00, pt01, and pt02.
            // We really don't completely know how two-couple stuff is supposed to work.
            if (two_couple_calling &&
                (yescmd.parseptr->concept->kind == concept_meta ||
                 yescmd.parseptr->concept->kind == concept_each_1x4))
               normalize_setup(result, normalize_to_4, qtag_no_compress);

            // And the rest of the call without it.
            // Try to figure out whether there is more.

            if (shiftynum != 1 &&   // Not sure why this shiftynum test is required, but it is.
                (result->result_flags.misc & RESULTFLAG__PARTS_ARE_KNOWN) &&
                (result->result_flags.misc & RESULTFLAG__DID_LAST_PART))
               goto get_out;

            // If the parts were not known, go ahead anyway, and hope for the best.
            // It is very likely that the code below will respond correctly
            // to being told to do the rest of the call even if there isn't
            // any more.  It's only for really pathological stuff like
            // "thirdly use hinge, 3/4 crazy mix", which makes no sense anyway,
            // that it would lose.
            // We need this for "secondly use acey deucey in swing the fractions".

            result->cmd = nocmd;
            result->cmd.cmd_assume.assumption = cr_none;  // Assumptions don't carry through.
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_FROMTOREV,shiftynum+1,0) | CMD_FRAC_BREAKING_UP);
         }
         else if (shiftynum != 1) {
            // This is "do the Nth part <concept>".
            fraction_info ff(ss->cmd, 0, true);    // Allow half parts.
            // For now, we allow only m_do_half_of_last_part = "1/2".
            if ((ff.m_do_half_of_last_part != 0 && ff.m_do_half_of_last_part != FRAC_FRAC_HALF_VALUE) ||
                ff.m_do_last_half_of_first_part != 0 ||
                ff.m_instant_stop < 0)  // This is the error indication.
               fail("Can't stack meta or fractional concepts this way.");

            int S = shiftynum;          // Shift amount.  We defer this many parts of subject call.
            int N = ff.m_fetch_total;   // Number of parts of call, same as number of things we will do.
            int A = ff.m_fetch_index;   // Starting point, due to incoming fraction.
            int Z = ff.m_highlimit;     // Ending point, due to incoming fraction.

            if (S <= A || S > Z) {
               // The part we are to apply the concept to is outside of the selected range.
               // Just do whatever fractions were given.
               result->cmd = nocmd;
               goto rollover_and_finish_it;
            }
            else {
               // The selected part is in the active region.  It might be the whole thing,
               // or it might be at the beginning, or at the end, or somewhere else.  That is,
               // there might or might not be something in the region before the selected part,
               // then there is the selected part, then there might or might not be something
               // in the region after the selected part.
               if (S > A+1) {
                  // The active region begins with something non-selected.
                  result->cmd = nocmd;
                  // Set the fractionalize field to do the first few parts of the call.
                  result->cmd.cmd_fraction.set_to_null_with_flags(
                                                                  FRACS(CMD_FRAC_CODE_FROMTO,S-1,A));
                  do_call_in_series_and_update_bits(result);
               }

               // Do the selected part, that is, the part that needs the concept.
               result->cmd = yescmd;
               result->cmd.cmd_fraction.set_to_null_with_flags(
                  FRACS(CMD_FRAC_CODE_ONLY,S,0) | CMD_FRAC_BREAKING_UP);

               // See if there is a remaining part.
               if (S < Z) {
                  // There is, so do the selected part and then set up the remaining part.
                  do_call_in_series_and_update_bits(result);
                  result->cmd = nocmd;
                  result->cmd.cmd_assume.assumption = cr_none;  // Assumptions don't carry through.
                  result->cmd.cmd_fraction.set_to_null_with_flags(
                     FRACS(CMD_FRAC_CODE_FROMTOREV, S+1, N-Z) | CMD_FRAC_BREAKING_UP);
               }

               if (ff.m_do_half_of_last_part == FRAC_FRAC_HALF_VALUE)
                  result->cmd.cmd_fraction.fraction |= CMD_FRAC_DEFER_HALF_OF_LAST;
               else if (ff.m_do_half_of_last_part != 0)
                  fail("Can't stack meta or fractional concepts this way.");  // Needs more thought.

               goto rollover_and_finish_it;
            }
         }
         else {
            if (
               // Being asked to do all but the first part.
               corefracs.is_null_with_exact_flags(FRACS(CMD_FRAC_CODE_FROMTOREV,2,0) | CMD_FRAC_BREAKING_UP) ||
               // Being asked to do some specific part other than the first.
               (corefracs.is_null_with_masked_flags(~CMD_FRAC_PART_MASK, CMD_FRAC_BREAKING_UP | CMD_FRAC_CODE_ONLY) &&
                nfield >= 2) ||
               // Being asked to do only the last part -- it's a safe bet that that isn't the first part.
               corefracs.is_null_with_exact_flags(FRACS(CMD_FRAC_CODE_ONLYREV,1,0) | CMD_FRAC_BREAKING_UP)) {

               // In any case, just pass it through.
               result->cmd.parseptr = result_of_skip;
            }
            else if (corefracs.is_null_with_exact_flags(FRACS(CMD_FRAC_CODE_FROMTOREV,1,1) | CMD_FRAC_BREAKING_UP)) {

               // We are being asked to do all but the last part.  Do the first part
               // with the concept, then all but first and last without it.

               result->cmd = yescmd;
               result->cmd.cmd_fraction.set_to_null_with_flags(
                  FRACS(CMD_FRAC_CODE_ONLY,1,0) | CMD_FRAC_BREAKING_UP);

               do_call_in_series_and_update_bits(result);

               result->cmd = nocmd;
               // Assumptions don't carry through.
               result->cmd.cmd_assume.assumption = cr_none;
               result->cmd.cmd_fraction.set_to_null_with_flags(
                  FRACS(CMD_FRAC_CODE_FROMTOREV,2,1) | CMD_FRAC_BREAKING_UP);
            }
            else if (corefracs.is_null_with_exact_flags(
               FRACS(CMD_FRAC_CODE_FROMTOREV,2,1) | CMD_FRAC_BREAKING_UP)) {

               // We are being asked to do just the inner parts, presumably because of an
               // "initially" and "finally".  Just pass it through.
               result->cmd = nocmd;
            }
            else if (corefracs.is_null_with_masked_flags(~CMD_FRAC_PART_MASK, CMD_FRAC_BREAKING_UP | CMD_FRAC_CODE_FROMTO)) {

               // We are being asked to do an initial subset that includes the first part.
               // Do the first part, then do the rest of the subset.

               result->cmd = yescmd;
               result->cmd.cmd_fraction.set_to_null_with_flags(
                  FRACS(CMD_FRAC_CODE_ONLY,1,0) | CMD_FRAC_BREAKING_UP);

               // The first part, with the concept.
               do_call_in_series_and_update_bits(result);

               result->cmd = nocmd;
               result->cmd.cmd_assume.assumption = cr_none;   // Assumptions don't carry through.
               // nfield = incoming end point; skip one at start.
               result->cmd.cmd_fraction.set_to_null_with_flags(
                  FRACS(CMD_FRAC_CODE_FROMTO,nfield,1) | CMD_FRAC_BREAKING_UP);
            }
            else {
               fraction_info ff(ss->cmd, 0, true);    // Allow half parts.
               // For now, we allow only m_do_half_of_last_part = "1/2".
               if ((ff.m_do_half_of_last_part != 0 && ff.m_do_half_of_last_part != FRAC_FRAC_HALF_VALUE) ||
                   ff.m_do_last_half_of_first_part != 0 ||
                   ff.m_instant_stop < 0)  // This is the error indication.
                  fail("Can't stack meta or fractional concepts this way.");

               int N = ff.m_fetch_total;   // Number of parts of call, same as number of things we will do.
               int A = ff.m_fetch_index;   // Starting point, due to incoming fraction.
               int Z = ff.m_highlimit;     // Ending point, due to incoming fraction.

               if (A == 0) {   // Does the given fraction include the first part?
                  if (ff.m_do_half_of_last_part != 0)
                     fail("Can't stack meta or fractional concepts this way.");  // Needs more thought; probably quite simple.


                  // Do the first part, with the concept.
                  result->cmd = yescmd;
                  result->cmd.cmd_fraction.set_to_null_with_flags(
                     FRACS(CMD_FRAC_CODE_ONLY,1,0) | CMD_FRAC_BREAKING_UP);
                  do_call_in_series_and_update_bits(result);

                  // Do the curtailed rest of the call, with the concept.
                  result->cmd = nocmd;
                  result->cmd.cmd_assume.assumption = cr_none;   // Assumptions don't carry through.
                  result->cmd.cmd_fraction.set_to_null_with_flags(
                     FRACS(CMD_FRAC_CODE_FROMTO, Z, 1) | CMD_FRAC_BREAKING_UP);
               }
               else {
                  // The fraction excludes the first part, so "initially" doesn't make much sense.  Whatever.
                  result->cmd = nocmd;
                  result->cmd.cmd_assume.assumption = cr_none;  // Assumptions don't carry through.
                  result->cmd.cmd_fraction.set_to_null_with_flags(
                     FRACS(CMD_FRAC_CODE_FROMTOREV, A+1, N-Z) | CMD_FRAC_BREAKING_UP);

                  if (ff.m_do_half_of_last_part != 0)
                     result->cmd.cmd_fraction.fraction |= CMD_FRAC_DEFER_HALF_OF_LAST;
               }
            }
         }

         goto finish_it;

      case meta_key_finally:

         // This is "finally": we select all but the last part without the concept,
         // and then the last part with the concept.

         if (corefracs.is_null_with_exact_flags(
                FRACS(CMD_FRAC_CODE_ONLYREV,1,0) | CMD_FRAC_BREAKING_UP)) {

            // We are being asked to do just the last part, because of another "finally".
            // Just pass it through.

            result->cmd = yescmd;
         }
         else if (corefracs.is_null_with_exact_flags(
                     FRACS(CMD_FRAC_CODE_FROMTOREV,1,1) | CMD_FRAC_BREAKING_UP)) {

            // We are being asked to do all but the last part,
            // because of another "finally".  Just pass it through.

            result->cmd = nocmd;
         }
         else if (corefracs.is_null_with_exact_flags(
                     FRACS(CMD_FRAC_CODE_ONLY,1,0) | CMD_FRAC_BREAKING_UP)) {

            // We are being asked to do just the first part,
            // presumably because of an "initially".  Just pass it through.

            result->cmd = nocmd;
         }
         else if (corefracs.is_null_with_exact_flags(
                     FRACS(CMD_FRAC_CODE_FROMTOREV,2,1) | CMD_FRAC_BREAKING_UP)) {

            // We are being asked to do just the inner parts, presumably because of
            // an "initially" and "finally".  Just pass it through.

            result->cmd = nocmd;
         }
         else if (corefracs.is_null_with_exact_flags(
                     FRACS(CMD_FRAC_CODE_FROMTOREV,2,0) | CMD_FRAC_BREAKING_UP)) {

            // We are being asked to do all but the first part, presumably because of
            // an "initially".  Do all but first and last normally, then do last
            // with the concept.

            // Change it to do all but first and last.

            result->cmd = nocmd;
            result->cmd.cmd_fraction.flags += CMD_FRAC_PART2_BIT;
            do_call_in_series_and_update_bits(result);
            result->cmd = yescmd;
            result->result_flags.misc &= ~expirations_to_clearmisc;
            // Assumptions don't carry through.
            result->cmd.cmd_assume.assumption = cr_none;
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_ONLYREV,1,0) | CMD_FRAC_BREAKING_UP);
         }
         else if (ss->cmd.cmd_fraction.is_null_with_masked_flags(
                     ~CMD_FRAC_PART_MASK,
                     CMD_FRAC_BREAKING_UP | CMD_FRAC_CODE_ONLY | CMD_FRAC_THISISLAST)) {

            // We are being asked to do some part, and we normally wouldn't know
            // whether it is last, so we would be screwed.  However, we have been
            // specifically told that it is last.  So we just pass it through.

            result->cmd = yescmd;
         }
         else if (corefracs.is_null()) {

            // Do the call without the concept.
            // Set the fractionalize field to execute all but the last part of the call.

            result->cmd = nocmd;

            // Look for "finally 1/2 ...".  If so, we can do something direct, without the need to
            // break things up.  Breaking things up could be injurious to concepts like "once
            // removed".  Cf. test t14t.
            if ((yescmd.cmd_misc3_flags & (CMD_MISC3__RESTRAIN_CRAZINESS|CMD_MISC3__PUT_FRAC_ON_FIRST)) ==
                (CMD_MISC3__RESTRAIN_CRAZINESS|CMD_MISC3__PUT_FRAC_ON_FIRST) &&
                yescmd.restrained_concept->concept->kind == concept_fractional &&
                yescmd.restrained_concept->concept->arg1 == 0 &&
                yescmd.restrained_concept->options.number_fields == NUMBER_FIELDS_2_1) {
               result->cmd.cmd_fraction.set_to_null_with_flags(
                  FRACS(CMD_FRAC_CODE_SKIP_K_MINUS_HALF,1,0));
               goto finish_it;
            }

            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_FROMTOREV,1,1) | CMD_FRAC_BREAKING_UP);
            do_call_in_series_and_update_bits(result);

            // Do the call with the concept.
            // Set the fractionalize field to execute the last part of the call.
            result->cmd = yescmd;
            result->result_flags.misc &= ~expirations_to_clearmisc;
            // Assumptions don't carry through.
            result->cmd.cmd_assume.assumption = cr_none;
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_ONLYREV,1,0) | CMD_FRAC_BREAKING_UP);
            result->result_flags.misc |= RESULTFLAG__EXPIRATION_ENAB | RESULTFLAG__REALLY_NO_REEVALUATE;
         }
         else
            fail("Can't stack meta or fractional concepts.");

         goto finish_it;

      case meta_key_initially_and_finally:

         // This is "initially and finally": we select the first part with the concept,
         // the interior part without the concept,
         // and then the last part with the concept.

         if (corefracs.is_null_with_exact_flags(
            FRACS(CMD_FRAC_CODE_ONLY,1,0) | CMD_FRAC_BREAKING_UP)) {
            // We're doing only the first part.
            result->cmd = yescmd;
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_ONLY,1,0) | CMD_FRAC_BREAKING_UP);
            result->result_flags.misc |= RESULTFLAG__EXPIRATION_ENAB;
         }
         else if (corefracs.is_null_with_exact_flags(
            FRACS(CMD_FRAC_CODE_ONLYREV,1,0) | CMD_FRAC_BREAKING_UP)) {
            // We're doing only the last part.
            result->cmd = yescmd;
            result->result_flags.misc &= ~expirations_to_clearmisc;
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_ONLYREV,1,0) | CMD_FRAC_BREAKING_UP);
            result->result_flags.misc |= RESULTFLAG__EXPIRATION_ENAB;
         }
         else if (corefracs.is_null_with_masked_flags(
            ~CMD_FRAC_PART_MASK,
            FRACS(CMD_FRAC_CODE_FROMTOREV,0,0) | CMD_FRAC_BREAKING_UP)) {
            // We're doing some subset, presumably not including the first part,
            // up through the last part.

            // Do the interior of the call, as originally specified
            // but without the last part,  without the concept.

            result->cmd = nocmd;
            // Assumptions don't carry through.
            result->cmd.cmd_assume.assumption = cr_none;
            result->cmd.cmd_fraction.flags += CMD_FRAC_PART2_BIT;
            result->result_flags.misc |= RESULTFLAG__EXPIRATION_ENAB;
            do_call_in_series_and_update_bits(result);

            // and the last part with it.  Be sure to reset the "twisted"/"yoyo" expiration.

            result->cmd = yescmd;
            result->result_flags.misc &= ~expirations_to_clearmisc;
            // Assumptions don't carry through.
            result->cmd.cmd_assume.assumption = cr_none;
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_ONLYREV,1,0) | CMD_FRAC_BREAKING_UP);
            result->result_flags.misc |= RESULTFLAG__EXPIRATION_ENAB;
         }
         else if (corefracs.is_null()) {
            // The usual case -- do the first part with the concept,

            result->cmd = yescmd;
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_ONLY,1,0) | CMD_FRAC_BREAKING_UP);
            result->result_flags.misc |= RESULTFLAG__EXPIRATION_ENAB;
            do_call_in_series_and_update_bits(result);

            // the interior of the call without it,

            result->cmd = nocmd;
            // Assumptions don't carry through.
            result->cmd.cmd_assume.assumption = cr_none;
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_FROMTOREV,2,1) | CMD_FRAC_BREAKING_UP);
            result->result_flags.misc |= RESULTFLAG__EXPIRATION_ENAB;
            do_call_in_series_and_update_bits(result);

            // and the last part with it.  Be sure to reset the "twisted"/"yoyo" expiration.

            result->cmd = yescmd;
            result->result_flags.misc &= ~expirations_to_clearmisc;
            // Assumptions don't carry through.
            result->cmd.cmd_assume.assumption = cr_none;
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_ONLYREV,1,0) | CMD_FRAC_BREAKING_UP);
            result->result_flags.misc |= RESULTFLAG__EXPIRATION_ENAB;
         }
         else
            fail("Can't stack meta or fractional concepts.");

         goto finish_it;

      default:

         // Otherwise, this is the "random", "reverse random", or "piecewise" concept.
         // Repeatedly execute parts of the call, skipping the concept where required.

         {
            fraction_command frac_stuff;
            uint32_t index = 0;
            uint32_t shortenhighlim = 0;
            uint32_t code_to_use_for_only = CMD_FRAC_CODE_ONLY;
            bool doing_just_one = false;

            // We allow picking a specific part, and we allow "finishing" from a specific
            // part, but we allow nothing else.

            if (corefracs.is_null_with_masked_flags(
                   CMD_FRAC_BREAKING_UP | CMD_FRAC_IMPROPER_BIT |
                   CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK | CMD_FRAC_PART2_MASK,
                   CMD_FRAC_BREAKING_UP | CMD_FRAC_CODE_ONLY)) {
               index = nfield - 1;
               doing_just_one = true;
            }
            else if (corefracs.is_null_with_masked_flags(
                        CMD_FRAC_BREAKING_UP | CMD_FRAC_IMPROPER_BIT |
                        CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK | CMD_FRAC_PART2_MASK,
                        CMD_FRAC_BREAKING_UP | CMD_FRAC_CODE_ONLYREV)) {
               index = nfield - 1;
               doing_just_one = true;
               code_to_use_for_only = CMD_FRAC_CODE_ONLYREV;
            }
            else if (corefracs.is_null_with_masked_flags(
                        CMD_FRAC_BREAKING_UP | CMD_FRAC_IMPROPER_BIT |
                        CMD_FRAC_REVERSE | CMD_FRAC_CODE_MASK,
                        CMD_FRAC_BREAKING_UP | CMD_FRAC_CODE_FROMTOREV)) {
               index = nfield - 1;
               shortenhighlim = kfield;
            }
            else if (corefracs.flags != 0)  // If flags=0, we allow incoming fraction; we pass it on.
               fail("Can't stack meta or fractional concepts.");

            frac_stuff = corefracs;
            frac_stuff.flags &= ~(CMD_FRAC_CODE_MASK|CMD_FRAC_PART_MASK|CMD_FRAC_PART2_MASK);

            uint32_t saved_last_flagmisc = 0;

            do {
               // Here is where we make use of actual numerical assignments.
               uint32_t revrand = key-meta_key_random;

               index++;
               if (index > 7) fail("Sorry, can't handle this number.");
               copy_cmd_preserve_elong_and_expire(ss, result);

               // If concept is "[reverse] random" and this is an even/odd-numbered part,
               // as the case may be, skip over the concept.
               if (((revrand & ~1) == 0) && ((index & 1) == revrand)) {
                  // But how do we skip the concept?  If it an ordinary single-call concept,
                  // it's easy.  But, if the concept takes a second call (the only legal case
                  // of this being "concept_special_sequential") we use its second subject call
                  // instead of the first.  This is part of the peculiar behavior of this
                  // particular combination.

                  result->cmd = nocmd;
               }
               else {
                  result->cmd = yescmd;
                  result->result_flags.misc &= ~expirations_to_clearmisc;
               }

               // Set the fractionalize field to do the indicated part.
               result->cmd.cmd_fraction = frac_stuff;
               result->cmd.cmd_fraction.flags |=
                  FRACS(code_to_use_for_only,index,shortenhighlim) | CMD_FRAC_BREAKING_UP;

               // If the call that we are doing has the RESULTFLAG__NO_REEVALUATE flag
               // on (meaning we don't re-evaluate under *any* circumstances, particularly
               // circumstances like these), we do not re-evaluate the ID bits.
               if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
                  result->update_id_bits();

               result->cmd.cmd_misc_flags &= ~CMD_MISC__NO_EXPAND_MATRIX;

               do_call_in_series(result, true, true, false);

               if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
                  result->update_id_bits();

               if (result->result_flags.misc & RESULTFLAG__SECONDARY_DONE) {
                  index = 0;
                  frac_stuff.flags |= CMD_FRAC_IMPROPER_BIT;
               }

               result->result_flags.misc &= ~RESULTFLAG__SECONDARY_DONE;

               // Now look at the "DID_LAST_PART" bits of the call we just executed,
               // to see whether we should break out.
               if (!(result->result_flags.misc & RESULTFLAG__PARTS_ARE_KNOWN)) {
                  // Problem.  The call doesn't know what it did.
                  // Perhaps the previous part knew.  This could arise if we
                  // do something like "random use flip back in swing the fractions" --
                  // various parts aren't actually done, but enough parts are done
                  // for us to piece together the required information.

                  result->result_flags.misc &= ~RESULTFLAG__PART_COMPLETION_BITS;

                  if ((saved_last_flagmisc & (RESULTFLAG__PARTS_ARE_KNOWN|RESULTFLAG__DID_LAST_PART))
                      == RESULTFLAG__PARTS_ARE_KNOWN) {
                     result->result_flags.misc |= RESULTFLAG__PARTS_ARE_KNOWN;
                     if (saved_last_flagmisc & RESULTFLAG__DID_NEXTTOLAST_PART)
                        result->result_flags.misc |= RESULTFLAG__DID_LAST_PART;
                  }

                  saved_last_flagmisc = 0;   // Need to recharge before can use it again.
               }
               else
                  saved_last_flagmisc = result->result_flags.misc;

               if (!(result->result_flags.misc & RESULTFLAG__PARTS_ARE_KNOWN))
                  fail("Can't have 'no one' do a call.");

               // We will pass all those bits back to our client.
               if (doing_just_one) break;
            }

            while (!(result->result_flags.misc & RESULTFLAG__DID_LAST_PART));
         }
      }
   }

   goto get_out;

   // This is used for things like "shift".  By doing the rollover, we make this concept
   // look like any other compound call.  Compound calls are immune to overcast warnings
   // on their internal parts.  So this makes "shift" calls immune also.  "Shift 1 Remake
   // the Wave" will therefore not raise a warning, because it is treated as though it were
   // a call defined as "1/2 left, 1/4 right, 1/4 right".  The interface between the two
   // "1/4 right"s is immune from the warning.  We're not sure this is the right thing to do.
 rollover_and_finish_it:

   if (result->result_flags.misc & RESULTFLAG__STOP_OVERCAST_CHECK)
      result->cmd.cmd_misc3_flags |= CMD_MISC3__STOP_OVERCAST_CHECK;

 finish_it:

   // This seems to make t50 work.
   result->cmd.prior_expire_bits |= result->result_flags.misc & RESULTFLAG__EXPIRATION_BITS;
   do_call_in_series_simple(result);
 final_fixup:
   result->result_flags.misc |= finalresultflagsmisc;

 get_out: ;
}


static void do_concept_replace_nth_part(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // Values of arg1 are:
   //    0 - replace last part
   //    1 - interrupt before last part
   //    2 - interrupt after given fraction
   //    3 - "sandwich" - interrupt after 1/2
   //    8 - replace Nth part
   //    9 = interrupt after Nth part

   int concept_key = parseptr->concept->arg1;
   fraction_command incoming_fracs = ss->cmd.cmd_fraction;

   ss->cmd.cmd_fraction.set_to_null();

   // We will modify these flags, and, in any case,
   // we need to rematerialize them at each step.
   setup_command cmd = ss->cmd;

   prepare_for_call_in_series(result, ss);

   fraction_command fracs_for_middle_part;
   fracs_for_middle_part.set_to_null();

   // Fill in some default values that might be useful.

   fraction_command frac_key1;
   frac_key1.set_to_null_with_flags(
      FRACS(CMD_FRAC_CODE_FROMTOREV,1,1) | CMD_FRAC_BREAKING_UP);

   fraction_command frac_key2;
   frac_key2.set_to_null_with_flags(
      FRACS(CMD_FRAC_CODE_FROMTOREV,parseptr->options.number_fields+1,0) |
      CMD_FRAC_BREAKING_UP);

   bool skip_replacement_call = false;

   switch (concept_key) {
   case 0:
      frac_key2.fraction = 0;
      break;
   case 1:
      frac_key2.flags = FRACS(CMD_FRAC_CODE_ONLY,1,0) |
         CMD_FRAC_BREAKING_UP | CMD_FRAC_REVERSE;
      break;
   case 2:
      // "Interrupt after M/N".
      frac_key1.flags = 0;
      frac_key1.process_fractions(NUMBER_FIELDS_1_0, parseptr->options.number_fields,
                                  FRAC_INVERT_NONE, ss->cmd.cmd_fraction);

      frac_key2.flags = 0;
      frac_key2.fraction =
         ((frac_key1.fraction & NUMBER_FIELD_MASK_RIGHT_TWO) << (BITS_PER_NUMBER_FIELD*2)) |
         NUMBER_FIELDS_0_0_1_1;
      break;
   case 3:
      // "Sandwich".
      frac_key1.flags = 0;
      frac_key1.process_fractions(NUMBER_FIELDS_1_0, NUMBER_FIELDS_2_1,
                                  FRAC_INVERT_NONE, ss->cmd.cmd_fraction);

      frac_key2.flags = 0;
      frac_key2.fraction =
         ((frac_key1.fraction & NUMBER_FIELD_MASK_RIGHT_TWO) << (BITS_PER_NUMBER_FIELD*2)) |
         NUMBER_FIELDS_0_0_1_1;
      break;
   case 8:
      // Replace Nth part.
      frac_key1.set_to_null_with_flags(
         FRACS(CMD_FRAC_CODE_FROMTO,parseptr->options.number_fields-1,0) |
         CMD_FRAC_BREAKING_UP);
      break;
   case 9:
      // Interrupt after Nth part.
      frac_key1.set_to_null_with_flags(
         FRACS(CMD_FRAC_CODE_FROMTO,parseptr->options.number_fields,0) |
         CMD_FRAC_BREAKING_UP);
      break;
   }

   // Handle Curried meta-concept on "sandwich".  "Sandwich" is the only concept
   // we will see that can use a Curried meta-concept.

   if (concept_key == 3 &&
       (cmd.cmd_misc3_flags & (CMD_MISC3__PUT_FRAC_ON_FIRST|CMD_MISC3__RESTRAIN_CRAZINESS)) ==
       CMD_MISC3__PUT_FRAC_ON_FIRST) {
      cmd.cmd_misc3_flags &= ~CMD_MISC3__PUT_FRAC_ON_FIRST;
      fracs_for_middle_part = incoming_fracs;
      incoming_fracs.set_to_null();
   }

   // Handle special fractionalization commands on "sandwich".  These are the
   // only instances of fractionalization that we allow on these concepts.

   if (concept_key == 3 &&
       incoming_fracs.is_null_with_masked_flags(
          CMD_FRAC_CODE_MASK | CMD_FRAC_PART_MASK | CMD_FRAC_PART2_MASK,
          FRACS(CMD_FRAC_CODE_ONLYREV,1,0))) {
      // Do the last part only.
      frac_key1.set_to_null_with_flags(FRACS(CMD_FRAC_CODE_FROMTO,0,0));
      skip_replacement_call = true;
   }
   else if (concept_key == 3 &&
            incoming_fracs.is_null_with_masked_flags(
               CMD_FRAC_CODE_MASK | CMD_FRAC_PART_MASK | CMD_FRAC_PART2_MASK,
               FRACS(CMD_FRAC_CODE_FROMTOREV,1,1))) {
      // Don't do the last part.
      frac_key2.fraction = 0;
   }
   else if (concept_key == 3 &&
            incoming_fracs.is_null_with_masked_flags(
               CMD_FRAC_CODE_MASK | CMD_FRAC_PART_MASK | CMD_FRAC_PART2_MASK,
               FRACS(CMD_FRAC_CODE_SKIP_K_MINUS_HALF,1,0))) {
      // Do only half of the last part.
      frac_key2.repackage(1, 2, 3, 4);
   }
   else if (!incoming_fracs.is_null())
      fail("Can't stack meta or fractional concepts.");

   // Do the initial part, if any, without the concept.  Use frac_key1.
   // If frac_key1 is CMD_FRAC_CODE_FROMTO with K=0 and N=0, there is no first part.

   if (!frac_key1.is_null_with_masked_flags(
           CMD_FRAC_CODE_MASK | CMD_FRAC_PART_MASK | CMD_FRAC_PART2_MASK,
           FRACS(CMD_FRAC_CODE_FROMTO,0,0))) {
      cmd.cmd_fraction = frac_key1;

      ss->cmd = cmd;

      // Lift the craziness restraint from before -- we are about to pull things apart.
      // But only if it is "sandwich".
      if (concept_key == 3) {
         if (cmd.cmd_misc3_flags & CMD_MISC3__RESTRAIN_CRAZINESS) {
            cmd.cmd_misc3_flags &= ~CMD_MISC3__RESTRAIN_CRAZINESS;
            cmd.restrained_fraction = cmd.cmd_fraction;
            cmd.cmd_fraction.set_to_null();
         }
      }

      copy_cmd_preserve_elong_and_expire(ss, result);
      result->cmd.prior_expire_bits |= RESULTFLAG__EXPIRATION_ENAB;
      // This seems to make t50 work.
      result->cmd.prior_expire_bits |=
         result->result_flags.misc & RESULTFLAG__EXPIRATION_BITS;
      do_call_in_series(result, true, true, false);
   }

   // Do the interruption/replacement call.

   if (!skip_replacement_call) {
      cmd.cmd_fraction = fracs_for_middle_part;
      ss->cmd = cmd;
      result->cmd = ss->cmd;
      result->cmd.parseptr = parseptr->subsidiary_root;
      if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
         result->update_id_bits();    // So you can interrupt with "leads run", etc.
      result->cmd.cmd_misc_flags &= ~CMD_MISC__NO_EXPAND_MATRIX;

      // Give this call a clean start with respect to expiration stuff.
      uint32_t suspended_expiration_bitsmisc =
         result->result_flags.misc & RESULTFLAG__EXPIRATION_BITS;
      result->result_flags.misc &= ~RESULTFLAG__EXPIRATION_BITS;
      do_call_in_series(result, true, true, false);

      // Put back the expiration bits for the resumed call.
      result->result_flags.misc &= ~RESULTFLAG__EXPIRATION_BITS;
      result->result_flags.misc |= suspended_expiration_bitsmisc|RESULTFLAG__EXPIRATION_ENAB;
   }

   // Do the final part, if there is more.  Use frac_key2.

   if (frac_key2.fraction != 0) {
      cmd.cmd_fraction = frac_key2;
      ss->cmd = cmd;

      // Lift the craziness restraint from before -- we are about to pull things apart.
      // But only if it is "sandwich".
      if (concept_key == 3) {
         if (cmd.cmd_misc3_flags & CMD_MISC3__RESTRAIN_CRAZINESS) {
            cmd.cmd_misc3_flags &= ~CMD_MISC3__RESTRAIN_CRAZINESS;
            cmd.restrained_fraction = cmd.cmd_fraction;
            cmd.cmd_fraction.set_to_null();
         }

         ss->cmd = cmd;
         // Re-enable setup expansion for final part of "sandwich".
         ss->cmd.cmd_misc_flags &= ~CMD_MISC__NO_EXPAND_MATRIX;
      }

      copy_cmd_preserve_elong_and_expire(ss, result);
      // This seems to make t50 work.
      result->cmd.prior_expire_bits |=
         result->result_flags.misc & RESULTFLAG__EXPIRATION_BITS;
      do_call_in_series(result, true, true, false);
   }
}


static void do_concept_interlace(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // We don't allow any other fractional stuff.

   if (!ss->cmd.cmd_fraction.is_null())
      fail("Can't stack meta or fractional concepts.");

   uint32_t first_doneflagmisc = 0;
   uint32_t second_doneflagmisc = 0;
   uint32_t indexa = 0;
   uint32_t indexb = 0;

   prepare_for_call_in_series(result, ss);
   uint32_t a_improper = 0;
   uint32_t b_improper = 0;
   uint32_t calla_expiration_bitsmisc = 0;
   uint32_t callb_expiration_bitsmisc = 0;

   // If yoyo/generous/twisted coming in, set callb_expiration, so only calla will do it.

   if ((ss->cmd.cmd_final_flags.herit & INHERITFLAG_YOYOETCMASK) == INHERITFLAG_YOYOETCK_YOYO)
      callb_expiration_bitsmisc |= RESULTFLAG__YOYO_ONLY_EXPIRED;
   if ((ss->cmd.cmd_final_flags.herit & INHERITFLAG_YOYOETCMASK) == INHERITFLAG_YOYOETCK_GENEROUS)
      callb_expiration_bitsmisc |= RESULTFLAG__GEN_STING_EXPIRED;
   if (ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_TWISTED))
      callb_expiration_bitsmisc |= RESULTFLAG__TWISTED_EXPIRED;
   if (ss->cmd.cmd_final_flags.test_finalbit(FINAL__SPLIT_SQUARE_APPROVED))
      callb_expiration_bitsmisc |= RESULTFLAG__SPLIT_EXPIRED;
   if (ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_RECTIFY))
      callb_expiration_bitsmisc |= RESULTFLAG__RECTIFY_EXPIRED;

   // Well, actually, what we want to do it not carry the stuff around,
   // that is, not clear and restore.

   do {
      indexa++;

      if (!(first_doneflagmisc & RESULTFLAG__DID_LAST_PART)) {
         // Do the indicated part of the first call.
         copy_cmd_preserve_elong_and_expire(ss, result, true);
         result->result_flags.misc &= ~RESULTFLAG__EXPIRATION_BITS;
         result->result_flags.misc |= calla_expiration_bitsmisc & RESULTFLAG__EXPIRATION_BITS;

         // If we know that this part is in fact last, we say so.  That way,
         // if the subject call is an instance of "finally", it will work.

         result->cmd.cmd_fraction.set_to_null_with_flags(
            FRACS(CMD_FRAC_CODE_ONLY,indexa,0) | CMD_FRAC_BREAKING_UP | a_improper);

         if (first_doneflagmisc & RESULTFLAG__DID_NEXTTOLAST_PART)
            result->cmd.cmd_fraction.flags |= CMD_FRAC_THISISLAST;

         if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
            result->update_id_bits();
         result->cmd.cmd_misc_flags &= ~CMD_MISC__NO_EXPAND_MATRIX;

         // This seems to make t50 work.
         result->cmd.prior_expire_bits |= result->result_flags.misc & RESULTFLAG__EXPIRATION_BITS;

         do_call_in_series(result, true, true, false);
         calla_expiration_bitsmisc = result->result_flags.misc;

         if (!(result->result_flags.misc & RESULTFLAG__PARTS_ARE_KNOWN))
            fail("Can't have 'no one' do a call.");

         if (result->result_flags.misc & RESULTFLAG__SECONDARY_DONE) {
            indexa = 0;
            a_improper |= CMD_FRAC_IMPROPER_BIT;
         }

         first_doneflagmisc = result->result_flags.misc;
         result->result_flags.misc &= ~RESULTFLAG__SECONDARY_DONE;  /* **** need this? */
      }
      else if (!(second_doneflagmisc & RESULTFLAG__DID_LAST_PART))
         warn(warn__bad_interlace_match);

      indexb++;

      if (!(second_doneflagmisc & RESULTFLAG__DID_LAST_PART)) {
         // Do the indicated part of the second call.
         copy_cmd_preserve_elong_and_expire(ss, result, true);
         result->cmd.parseptr = parseptr->subsidiary_root;
         result->result_flags.misc &= ~RESULTFLAG__EXPIRATION_BITS;
         result->result_flags.misc |= callb_expiration_bitsmisc & RESULTFLAG__EXPIRATION_BITS;

         // If other part has run out, and we aren't doing an improper fraction,
         // just finish this.
         if ((first_doneflagmisc & RESULTFLAG__DID_LAST_PART) &&
             !(second_doneflagmisc & RESULTFLAG__SECONDARY_DONE)) {
            result->cmd.cmd_fraction.set_to_null_with_flags(
               FRACS(CMD_FRAC_CODE_FROMTOREV,indexb,0) | CMD_FRAC_BREAKING_UP | b_improper);

            // This seems to make t50 work.
            result->cmd.prior_expire_bits |=
               result->result_flags.misc & RESULTFLAG__EXPIRATION_BITS;

            do_call_in_series(result, true, true, false);
            return;
         }

         result->cmd.cmd_fraction.set_to_null_with_flags(
            FRACS(CMD_FRAC_CODE_ONLY,indexb,0) | CMD_FRAC_BREAKING_UP | b_improper);

         // This seems to make t50 work.
         result->cmd.prior_expire_bits |= result->result_flags.misc & RESULTFLAG__EXPIRATION_BITS;

         do_call_in_series(result, true, true, false);
         callb_expiration_bitsmisc = result->result_flags.misc;

         if (!(result->result_flags.misc & RESULTFLAG__PARTS_ARE_KNOWN))
            fail("Can't have 'no one' do a call.");

         if (result->result_flags.misc & RESULTFLAG__SECONDARY_DONE) {
            indexb = 0;
            b_improper |= CMD_FRAC_IMPROPER_BIT;
         }

         second_doneflagmisc = result->result_flags.misc;
         result->result_flags.misc &= ~RESULTFLAG__SECONDARY_DONE;  /* **** need this? */
      }
      else if (!(first_doneflagmisc & RESULTFLAG__DID_LAST_PART))
         warn(warn__bad_interlace_match);
   }
   while ((first_doneflagmisc & second_doneflagmisc & RESULTFLAG__DID_LAST_PART) == 0);
}


static void do_concept_fractional(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   // Note: if we ever implement something that omits the first fraction, that
   //   concept would have to have "CONCPROP__NO_STEP" set in concept_table, and
   //   things might get ugly.
   // Actually, we have implemented exactly that -- "do last fraction", and we
   //   have not set CONCPROP__NO_STEP in the concept table.  The user is responsible
   //   for the consequences of using this.

   // The meaning of arg1 is as follows:
   // 0 - "M/N" or "M/N OF" - do first fraction.
   // 1 - "LAST M/N OF"
   // 2 - "1-M/N" do the whole call and then some.
   // 3 - "MIDDLE M/N OF"

   uint32_t ddnn = parseptr->options.number_fields;

   int dd = ddnn >> BITS_PER_NUMBER_FIELD;
   int nn = ddnn & NUMBER_FIELD_MASK;
   uint32_t FOO;

   switch (parseptr->concept->arg1) {
   case 1:
      FOO = (dd << (BITS_PER_NUMBER_FIELD*3)) + ((dd-nn) << (BITS_PER_NUMBER_FIELD*2)) + NUMBER_FIELDS_1_1;
      break;
   case 3:
      FOO = dd << (BITS_PER_NUMBER_FIELD+1);      // dd * 2.
      FOO += FOO << (BITS_PER_NUMBER_FIELD*2);    // Both denominators.
      FOO += ((dd-nn) << (BITS_PER_NUMBER_FIELD*2)) + dd + nn;
      break;
   default:    // 0 or 2.
      FOO = ddnn+NUMBER_FIELDS_1_0_0_0;
      break;
   }

   fraction_command ARG4 = ss->cmd.cmd_fraction;

   if (ss->cmd.restrained_fraction.fraction) {
      if (ss->cmd.cmd_fraction.fraction != FRAC_FRAC_NULL_VALUE)
         fail("Fraction nesting is too complicated.");
      ARG4.fraction = FOO;
      ss->cmd.cmd_fraction.fraction = ss->cmd.restrained_fraction.fraction;
      ss->cmd.restrained_fraction.fraction = 0;
      FOO = ss->cmd.cmd_fraction.fraction;
   }

   bool improper = false;
   fraction_command new_fracs;

   new_fracs.process_fractions((FOO >> (BITS_PER_NUMBER_FIELD*2)) & NUMBER_FIELD_MASK_RIGHT_TWO,
                               FOO & NUMBER_FIELD_MASK_RIGHT_TWO,
                               FRAC_INVERT_NONE,
                               ARG4,
                               parseptr->concept->arg1 == 2,
                               &improper);

   if (improper) {
      // Do the whole call first, then part of it again.
      if (ss->cmd.cmd_misc2_flags & (CMD_MISC2__CENTRAL_SNAG | CMD_MISC2__INVERT_SNAG))
         fail("Can't do \"snag\" followed by this concept or modifier.");

      if ((ss->cmd.cmd_fraction.flags & CMD_FRAC_PART_MASK) != 0 &&
          ((ss->cmd.cmd_fraction.flags & (CMD_FRAC_CODE_MASK|CMD_FRAC_BREAKING_UP)) ==
           (CMD_FRAC_CODE_ONLY|CMD_FRAC_BREAKING_UP))) {

         // We are being asked to do just one part of "1-3/5 swing the fractions".

         ss->cmd.cmd_fraction.fraction = 0;

         if ((ss->cmd.cmd_fraction.flags & CMD_FRAC_IMPROPER_BIT) == 0) {
            // First time around -- Do the indicated part of a whole swing the fractions.

            ss->cmd.cmd_fraction.fraction = FRAC_FRAC_NULL_VALUE;
            move(ss, false, result);
            // And, if it reported that it did the last part, change it to
            // "secondary_done" to indicate that we're not finished.
            if (result->result_flags.misc & RESULTFLAG__DID_LAST_PART) {
               result->result_flags.misc &= ~RESULTFLAG__DID_LAST_PART;
               result->result_flags.misc |= RESULTFLAG__SECONDARY_DONE;
            }
         }
         else {
            // Second time around -- just do the fractional part.
            ss->cmd.cmd_fraction.fraction = new_fracs.fraction;
            move(ss, false, result);
         }
      }
      else if ((ss->cmd.cmd_fraction.flags & CMD_FRAC_PART_MASK) >= (CMD_FRAC_PART_BIT*2) &&
               (ss->cmd.cmd_fraction.flags & (CMD_FRAC_CODE_MASK|CMD_FRAC_BREAKING_UP)) ==
               (CMD_FRAC_CODE_FROMTOREV|CMD_FRAC_BREAKING_UP)) {

         // We are being asked to do "1-3/5 swing the fractions", but skipping some number of initial parts,
         // and perhaps some final parts as well.  We will take the chance that the number of initial parts
         // to skip does not exceed the first full swing the fractions.

         // We are being asked to do everything beyond some part of
         // "1-3/5 swing the fractions".

         // First, we do the full call, skipping the initial parts that need to be skipped.

         prepare_for_call_in_series(result, ss, true);   // Preserve the "NO_REEVALUATE" flag.

         result->cmd.cmd_fraction.set_to_null_with_flags(
            FRACS(CMD_FRAC_CODE_FROMTOREV,ss->cmd.cmd_fraction.flags & CMD_FRAC_PART_MASK,0) | CMD_FRAC_BREAKING_UP);

         do_call_in_series(result, false,
            !(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX),
            false);

         copy_cmd_preserve_elong_and_expire(ss, result);
         result->cmd.cmd_fraction.flags = ss->cmd.cmd_fraction.flags;
         result->cmd.cmd_fraction.fraction = new_fracs.fraction;

         if ((ss->cmd.cmd_fraction.flags & CMD_FRAC_PART2_MASK) != 0) {
            result->cmd.cmd_fraction.flags &= ~CMD_FRAC_PART_MASK;  // Force "N" to 1.
            result->cmd.cmd_fraction.flags |= CMD_FRAC_PART_BIT;
         }
         else {
            result->cmd.cmd_fraction.flags = 0;   // Do the whole 3/5.
         }

         if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
            result->update_id_bits();
         result->cmd.cmd_misc_flags &= ~CMD_MISC__NO_EXPAND_MATRIX;

         do_call_in_series(result, false,
            !(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX),
            false);
      }
      else if ((ss->cmd.cmd_fraction.flags & CMD_FRAC_PART_MASK) == CMD_FRAC_PART_BIT &&
               (ss->cmd.cmd_fraction.flags & (CMD_FRAC_CODE_MASK|CMD_FRAC_BREAKING_UP)) ==
           (CMD_FRAC_CODE_ONLYREV|CMD_FRAC_BREAKING_UP)) {

         // We are being asked to do just the last few parts of
         // "1-3/5 swing the fractions".  We are going to hope that those parts
         // lie only in the final "3/5".
         // Actually, we take no chances.  If it isn't the very last part, we don't allow it.

         prepare_for_call_in_series(result, ss, true);   // Preserve the "NO_REEVALUATE" flag.
         result->cmd.cmd_fraction.flags = ss->cmd.cmd_fraction.flags;
         result->cmd.cmd_fraction.fraction = new_fracs.fraction;
         do_call_in_series(result, false,
            !(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX),
            false);
      }
      else {
         prepare_for_call_in_series(result, ss, true);   // Preserve the "NO_REEVALUATE" flag.
         result->cmd.cmd_fraction.set_to_null_with_flags(ss->cmd.cmd_fraction.flags);
         do_call_in_series(result, false,
            !(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX),
            false);

         copy_cmd_preserve_elong_and_expire(ss, result);
         result->cmd.cmd_fraction.flags = ss->cmd.cmd_fraction.flags;
         result->cmd.cmd_fraction.fraction = new_fracs.fraction;

         if ((ss->cmd.cmd_fraction.flags & CMD_FRAC_PART_MASK) >= (CMD_FRAC_PART_BIT*2) &&
                  ((ss->cmd.cmd_fraction.flags & (CMD_FRAC_CODE_MASK|CMD_FRAC_BREAKING_UP)) ==
                   (CMD_FRAC_CODE_FROMTOREV|CMD_FRAC_BREAKING_UP))) {

            // We are being asked to do everything beyond some part of
            // "1-3/5 swing the fractions".
            result->cmd.cmd_fraction.flags = 0;   // Do the whole 3/5.
         }
         else {
            if ((ss->cmd.cmd_fraction.flags & CMD_FRAC_PART_MASK) == CMD_FRAC_PART_BIT &&
                (ss->cmd.cmd_fraction.flags & (CMD_FRAC_CODE_MASK|CMD_FRAC_BREAKING_UP)) ==
                 (CMD_FRAC_CODE_FROMTOREV|CMD_FRAC_BREAKING_UP)) {
                  // No action; this is OK.
            }
            else if ((ss->cmd.cmd_fraction.flags & CMD_FRAC_CODE_MASK) == CMD_FRAC_CODE_ONLY) {
               // No action; this is OK.
            }
            else {
               fail("Sorry, can't do this.");
            }
         }

         if (!(result->result_flags.misc & RESULTFLAG__NO_REEVALUATE))
            result->update_id_bits();
         result->cmd.cmd_misc_flags &= ~CMD_MISC__NO_EXPAND_MATRIX;

         // Roll over the overcast stuff, so that "center 2 work 1-1/2, 6x2 acey deucey won't raise the warning.
         if (result->result_flags.misc & RESULTFLAG__STOP_OVERCAST_CHECK)
            result->cmd.cmd_misc3_flags |= CMD_MISC3__STOP_OVERCAST_CHECK;

         do_call_in_series(result, false,
            !(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX),
            false);
      }
   }
   else {
      ss->cmd.cmd_fraction.fraction = new_fracs.fraction;
      move(ss, false, result);
   }
}


static void do_concept_so_and_so_begin(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   who_list saved_selector;
   int i;
   setup setup1, setup2;
   setup the_setups[2];

   // See if we are being requested to do only stuff that does not
   // include the first part.  If so, we just do it.

   // We check for either part selection indicating the first part isn't involved ...
   if ((((ss->cmd.cmd_fraction.flags & CMD_FRAC_CODE_MASK) == CMD_FRAC_CODE_ONLY ||
        (ss->cmd.cmd_fraction.flags & CMD_FRAC_CODE_MASK) == CMD_FRAC_CODE_FROMTOREV) &&
       (ss->cmd.cmd_fraction.flags & CMD_FRAC_PART_MASK) >= CMD_FRAC_PART_BIT*2)
       ||
       // or fraction selection indicating the same.
       // That is, code="only", part=0, reverse off, and starting frac != 0.
       (((ss->cmd.cmd_fraction.flags &
          (CMD_FRAC_CODE_MASK|CMD_FRAC_PART_MASK|CMD_FRAC_REVERSE))
         == CMD_FRAC_CODE_ONLY)
        && (ss->cmd.cmd_fraction.fraction & NUMBER_FIELD_MASK_SECOND_ONE) != 0)) {
      move(ss, false, result);
      return;
   }

   // At this point we know that we will do stuff including the first part,
   // which is the part that requires special action.

   saved_selector = current_options.who;
   current_options.who = parseptr->options.who;

   setup1 = *ss;              /* designees */
   setup2 = *ss;              /* non-designees */

   if (attr::slimit(ss) < 0) fail("Can't identify people in this setup.");
   for (i=0; i<=attr::slimit(ss); i++) {
      if (ss->people[i].id1) {
         if (selectp(ss, i))
            setup2.clear_person(i);
         else
            setup1.clear_person(i);
      }
   }

   current_options.who = saved_selector;

   normalize_setup(&setup1, plain_normalize, qtag_compress);
   normalize_setup(&setup2, plain_normalize, qtag_compress);

   // We just feed the "reverse" bit and the fractional stuff (low 16 bits)
   // through.  They have no effect on what we are doing.

   uint32_t incoming_code_and_parts = ss->cmd.cmd_fraction.flags;
   fraction_command incoming_everything_else = ss->cmd.cmd_fraction;
   incoming_everything_else.flags &=
      ~(CMD_FRAC_CODE_MASK|CMD_FRAC_PART_MASK|CMD_FRAC_PART2_MASK);

   // Set the fractionalize field to execute the first part of the call.

   setup1.cmd.cmd_fraction = incoming_everything_else;
   setup1.cmd.cmd_fraction.flags |= FRACS(CMD_FRAC_CODE_ONLY,1,0);

   // The selected people execute the first part of the call.

   move(&setup1, false, &the_setups[0]);
   the_setups[1] = setup2;

   // Give the people who didn't move the same result flags as those who did.
   // This is important for the "did last part" check.
   the_setups[1].result_flags = the_setups[0].result_flags;
   the_setups[1].result_flags = get_multiple_parallel_resultflags(the_setups, 2);

   merge_table::merge_setups(&the_setups[0], merge_c1_phantom, &the_setups[1]);
   uint32_t finalresultflagsmisc = the_setups[1].result_flags.misc;
   normalize_setup(&the_setups[1], simple_normalize, qtag_compress);

   if ((incoming_code_and_parts & CMD_FRAC_CODE_MASK) != CMD_FRAC_CODE_ONLY)
      fail("Can't stack meta or fractional concepts.");

   // Now we know that the code is "ONLY".  If N=0, this is the null operation;
   // just finish the call.  If N=1, we were doing only the first part; exit now.
   // If N>=2, we already exited.

   if ((incoming_code_and_parts & CMD_FRAC_PART_MASK) == CMD_FRAC_PART_BIT) {
      *result = the_setups[1];
      result->result_flags.misc |= finalresultflagsmisc;
      result->result_flags.misc &= ~3;
      return;
   }

   the_setups[1].cmd = ss->cmd; // Just in case it got messed up, which shouldn't have happened.

   // Set the fractionalize field to execute the rest of the call.
   the_setups[1].cmd.cmd_fraction = incoming_everything_else;
   the_setups[1].cmd.cmd_fraction.flags |= FRACS(CMD_FRAC_CODE_FROMTOREV,2,0);
   move(&the_setups[1], false, result);
   result->result_flags.misc = finalresultflagsmisc;
   result->result_flags.misc &= ~3;
   normalize_setup(result, simple_normalize, qtag_compress);
}


static void so_and_so_only_move(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   ss->cmd.cmd_misc3_flags |= CMD_MISC3__DOING_YOUR_PART;

   // The general principle is "reevaluate often".  The thing that the selected people
   // are being asked to do might have multiple parts, such that those parts can't normally
   // be done in the manner stated, but can be done if reevaluation takes place.  So, if
   // this concept fails while doing a "two calls in succession", put an implicit "piecewise"
   // around it to pick out the individual calls.

   // Preserved across a throw; must be volatile.
   volatile saved_error_info err;
   volatile error_flag_type save_throw_this = error_flag_none;

   try {
      selective_move(ss, parseptr, (selective_key) parseptr->concept->arg1,
                     parseptr->concept->arg2, parseptr->concept->arg3,
                     0, parseptr->options.who, false, result);
   }
   catch(error_flag_type foo) {
      save_throw_this = foo;
      ((saved_error_info *) &err)->collect(foo);
   }

   if (save_throw_this != error_flag_none) {

      // Try the special stuff, reevaluating through a "two calls in succession".
      // Do this only under carefully controlled circumstances.  Failure to check
      // could lead to infinite recursion.

      // Don't do it if this is "concept_some_vs_others" or "concept_same_sex_disconnected",
      // or if it is something like "own the <anyone>" or "snag <anyone>".
      // We do it only for plain "<anyone>".

      if (parseptr->concept->kind != concept_so_and_so_only ||
          (selective_key) parseptr->concept->arg1 != selective_key_plain)
         throw save_throw_this;

      // If we are going to do a complete "two calls in succession",
      // or some similar thing like "<anything> with the flow",
      // see if we can succeed by putting an implicit "piecewise" around this.

      if (!parseptr->next ||
          !parseptr->next->concept ||
          (parseptr->next->concept->kind != concept_sequential &&
           parseptr->next->concept->kind != concept_another_call_next_mod) ||
          !ss->cmd.cmd_fraction.is_null())
         throw save_throw_this;

      parse_block meta_thing(concept_special_piecewise);
      meta_thing.next = parseptr;

      // Now do the "piecewise <anyone> ( X ; Y )", watching for errors again.

      try {
         do_concept_meta(ss, &meta_thing, result);
      }
      catch(error_flag_type) {
         // This failed too?  Throw the *original* error.
         ((saved_error_info *) &err)->throw_saved_error();
      }
   }
}


static void do_concept_concentric(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   setup_command sscmd = ss->cmd;
   calldef_schema schema = (calldef_schema) parseptr->concept->arg1;

   if (schema == schema_cross_concentric && two_couple_calling && attr::klimit(ss->kind) < 7) {
      schema = schema_single_cross_concentric;
   }

   if (schema == schema_concentric) {
      uint64_t herits =
         ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_GRAND|INHERITFLAG_SINGLE|INHERITFLAG_CROSS);

      if (two_couple_calling && attr::klimit(ss->kind) < 7) {
         schema = schema_single_concentric;
      }

      switch (herits) {
      case INHERITFLAG_CROSS:
         schema = schema_cross_concentric;
         sscmd.cmd_final_flags.clear_heritbits(herits);
         break;
      case INHERITFLAG_SINGLE:
         schema = schema_single_concentric;
         sscmd.cmd_final_flags.clear_heritbits(herits);
         break;
      case INHERITFLAG_SINGLE|INHERITFLAG_CROSS:
         schema = schema_single_cross_concentric;
         sscmd.cmd_final_flags.clear_heritbits(herits);
         break;
      case INHERITFLAG_GRAND|INHERITFLAG_SINGLE:
         schema = schema_grand_single_concentric;
         sscmd.cmd_final_flags.clear_heritbits(herits);
         break;
      case INHERITFLAG_GRAND|INHERITFLAG_SINGLE|INHERITFLAG_CROSS:
         schema = schema_grand_single_cross_concentric;
         sscmd.cmd_final_flags.clear_heritbits(herits);
         break;
      }
   }

   switch (schema) {
   case schema_3x3k_concentric:
   case schema_3x3k_cross_concentric:
      switch (ss->kind) {
      case s1x8: case s1x10:
         ss->do_matrix_expansion(CONCPROP__NEEDK_1X12, false);
         break;
      case s2x4:
         ss->do_matrix_expansion(CONCPROP__NEEDK_2X6, false);
         break;
      }
      break;
   case schema_4x4k_concentric:
   case schema_4x4k_cross_concentric:
      switch (ss->kind) {
      case s1x8: case s1x10: case s1x12: case s1x14:
         ss->do_matrix_expansion(CONCPROP__NEEDK_1X16, false);
         break;
      case s2x4: case s2x6:
         ss->do_matrix_expansion(CONCPROP__NEEDK_2X8, false);
         break;
      }
      break;
   }

   if ((ss->cmd.cmd_misc3_flags & CMD_MISC3__META_NOCMD) &&
       (schema_attrs[schema].attrs & SCA_CROSS))
      warn(warn__meta_on_xconc);

   switch (schema) {
      uint32_t map_code;
   case schema_single_concentric:
   case schema_single_cross_concentric:
      switch (ss->kind) {
      case s2x4:   map_code = MAPCODE(s1x4,2,MPKIND__SPLIT,1); break;
      case s1x8:   map_code = MAPCODE(s1x4,2,MPKIND__SPLIT,0); break;
      case s_qtag: map_code = MAPCODE(sdmd,2,MPKIND__SPLIT,1); break;
      case s_ptpd: map_code = MAPCODE(sdmd,2,MPKIND__SPLIT,0); break;
      case s1x4: case sdmd:
         concentric_move(ss, &sscmd, &sscmd, schema, 0,
                         DFM1_CONC_CONCENTRIC_RULES, true, false, ~0U, result);
         return;
      default:
         fail("Can't figure out how to do single concentric here.");
      }

      // Reset it to execute this same concept again, until it doesn't have to split any more.
      ss->cmd.parseptr = parseptr;
      divided_setup_move(ss, map_code, phantest_ok, true, result);
      break;
   case schema_intermediate_diamond: case schema_outside_diamond:
      ss->cmd.cmd_misc3_flags |= CMD_MISC3__SAID_DIAMOND;
      concentric_move(ss, &sscmd, (setup_command *) 0, schema, 0,
                      DFM1_CONC_CONCENTRIC_RULES, true, false, ~0U, result);

      // 8-person concentric operations do not show the split.
      result->result_flags.clear_split_info();
      break;
   default:
      // Check for "CONCENTRIC, Z".
      if (ss->cmd.parseptr->concept->kind == concept_misc_distort && ss->cmd.parseptr->concept->arg1 == 0)
         fail("Use the \"CONCENTRIC Z's\" concept.");

      concentric_move(ss, &sscmd, &sscmd, schema, 0,
                      DFM1_CONC_CONCENTRIC_RULES, true, false, ~0U, result);

      // 8-person concentric operations do not show the split.
      result->result_flags.clear_split_info();
      break;
   }
}



/* ARGSUSED */
static void do_concept_standard(
   setup *ss,
   parse_block *parseptr,
   setup *result)
{
   /* This should never be called this way -- "standard" is treated specially.
      But we do need a nonzero entry in the dispatch table. */
   fail("Huh? -- standard concept out of place.");
}

static void complain_about_in_setup(
   setup *ss,
   parse_block *parseptr,
   setup *result)
{
   fail("This concept must be preceded by \"as couples\" or \"tandem\" type of concept.");
}



/* ARGSUSED */
static void do_concept_matrix(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   ss->cmd.cmd_misc_flags |= CMD_MISC__MATRIX_CONCEPT;
   // The above flag will raise an error in all contexts except
   // those few to which this concept may be applied.
   move(ss, false, result);
}


static bool ok_for_expand_1(const concept_descriptor *this_concept)
{
   concept_kind this_kind = this_concept->kind;

   return
      (this_kind == concept_do_phantom_boxes && this_concept->arg3 == MPKIND__SPLIT) ||
      this_kind == concept_multiple_boxes ||
      this_kind == concept_triple_boxes_together ||
      this_kind == concept_quad_boxes_together;
}
static bool ok_for_expand_2(const concept_descriptor *this_concept)
{
   concept_kind this_kind = this_concept->kind;

   return
      (this_kind == concept_do_phantom_2x4 && this_concept->arg3 == MPKIND__SPLIT) ||
      this_kind == concept_multiple_lines_tog ||
      this_kind == concept_multiple_lines_tog_std;
}


extern bool do_big_concept(
   setup *ss,
   parse_block *the_concept_parse_block,
   bool handle_concept_details,
   setup *result) THROW_DECL
{
   remove_z_distortion(ss);

   void (*concept_func)(setup *, parse_block *, setup *);
   parse_block *this_concept_parse_block = the_concept_parse_block;
   const concept_descriptor *this_concept = this_concept_parse_block->concept;
   concept_kind this_kind = this_concept->kind;
   const concept_table_item *this_table_item = &concept_table[this_kind];
   const uint32_t prop_bits = this_table_item->concept_prop;

   if (this_kind == concept_multiple_diamonds ||
       this_kind == concept_in_out_nostd ||
       this_kind == concept_do_twinphantom_diamonds ||
       this_kind == concept_phan_crazy ||
       this_kind == concept_frac_phan_crazy ||
       this_kind == concept_do_phantom_stag_qtg ||
       this_kind == concept_do_phantom_diag_qtg ||
       this_kind == concept_distorted ||
       this_kind == concept_once_removed ||
       this_kind == concept_concentric) {
      if (this_concept->arg3 == CMD_MISC__VERIFY_DMD_LIKE)
         ss->cmd.cmd_misc3_flags |= CMD_MISC3__SAID_DIAMOND;
   }
   else if (this_kind == concept_do_phantom_diamonds) {
      if (this_concept->arg2 == CMD_MISC__VERIFY_DMD_LIKE)
         ss->cmd.cmd_misc3_flags |= CMD_MISC3__SAID_DIAMOND;
   }

   // Take care of combinations like "mystic triple waves".

   if (handle_concept_details) {
      if (prop_bits & CONCPROP__PERMIT_MYSTIC &&
          ss->cmd.cmd_misc2_flags & CMD_MISC2__CENTRAL_MYSTIC) {

         // Turn on the good bits.
         ss->cmd.cmd_misc2_flags |= CMD_MISC2__MYSTIFY_SPLIT;
         if (ss->cmd.cmd_misc2_flags & CMD_MISC2__INVERT_MYSTIC)
            ss->cmd.cmd_misc2_flags |= CMD_MISC2__MYSTIFY_INVERT;

         // And turn off the old ones.
         ss->cmd.cmd_misc2_flags &= ~(CMD_MISC2__CENTRAL_MYSTIC|CMD_MISC2__INVERT_MYSTIC);
      }

      if (ss->cmd.cmd_misc2_flags & CMD_MISC2__CTR_END_MASK) {

         // If we have an "invert", "central", "mystic" or "snag" concept in place,
         // we have to check whether the current concept can deal with it.

         // The following concepts are always acceptable with invert/snag/etc in place.

         if (this_kind == concept_snag_mystic ||
             this_kind == concept_central ||
             this_kind == concept_fractional ||
             this_kind == concept_concentric ||
             this_kind == concept_some_vs_others ||
             (this_kind == concept_meta &&
              (this_concept->arg1 == meta_key_finish ||
               this_concept->arg1 == meta_key_revorder)))
            goto this_is_ok;

         // Otherwise, if "central" is selected, it must be one of the following ones.

         if (ss->cmd.cmd_misc2_flags & CMD_MISC2__DO_CENTRAL) {
            if (this_kind != concept_fractional &&
                this_kind != concept_fan &&
                this_kind != concept_dbl_frac_crazy &&
                this_kind != concept_frac_crazy &&
                (this_kind != concept_meta ||
                 (this_concept->arg1 != meta_key_like_a &&
                  this_concept->arg1 != meta_key_skip_last_part &&
                  this_concept->arg1 != meta_key_shift_n)) &&
                (this_kind != concept_meta_one_arg ||
                 (this_concept->arg1 != meta_key_skip_nth_part &&
                  this_concept->arg1 != meta_key_shift_n)))
               goto this_is_bad;
         }

         // But if "snag" or "invert" is selected, we lose.

         if (ss->cmd.cmd_misc2_flags & (CMD_MISC2__CENTRAL_SNAG|CMD_MISC2__SAID_INVERT))
            goto this_is_bad;

         goto this_is_ok;

      this_is_bad:

         fail("Can't do \"invert/central/snag/mystic\" followed by this concept or modifier.");

      this_is_ok: ;
      }

      if ((ss->cmd.cmd_misc_flags & CMD_MISC__MATRIX_CONCEPT) &&
          !(prop_bits & CONCPROP__PERMIT_MATRIX))
         fail("\"Matrix\" concept must be followed by applicable concept.");
   }

   concept_func = this_table_item->concept_action;
   ss->cmd.parseptr = ss->cmd.parseptr->next;
   if (!ss->cmd.parseptr)
      fail("Incomplete parse.");

   /* When we invoke one of the functions, we will have:
      2ndarg = the concept we are doing
      1starg->cmd.parseptr = the next concept after that */

   if (concept_func == 0) return false;

   if (this_concept->level > calling_level) warn_about_concept_level();

   result->clear_people();

   if (prop_bits & CONCPROP__SET_PHANTOMS)
      ss->cmd.cmd_misc_flags |= CMD_MISC__PHANTOMS;

   if (prop_bits & CONCPROP__NO_STEP)
      ss->cmd.cmd_misc_flags |= CMD_MISC__NO_STEP_TO_WAVE;

   // If the "arg2_matrix" bit is on, pick up additional
   // matrix descriptor bits from the arg2 word.

   uint32_t prop_bits_for_expansion = prop_bits;
   if (prop_bits & CONCPROP__NEED_ARG2_MATRIX)
      prop_bits_for_expansion |= this_concept->arg2;

   // "Standard" is special -- process it now.
   if (this_kind == concept_standard) {
      parse_block *substandard_concptptr;
      final_and_herit_flags junk_concepts;

      // Skip to the phantom-line (or whatever) concept
      // by going over the "standard" and skipping comments.

      junk_concepts.clear_all_herit_and_final_bits();
      substandard_concptptr = process_final_concepts(the_concept_parse_block->next,
                                                     true, &junk_concepts,
                                                     true, false);

      // If we hit "matrix", do a little extra stuff and continue.

      if (!junk_concepts.test_for_any_herit_or_final_bit() &&
          substandard_concptptr->concept->kind == concept_matrix) {
         ss->cmd.cmd_misc_flags |= CMD_MISC__MATRIX_CONCEPT;
         substandard_concptptr = process_final_concepts(substandard_concptptr->next,
                                                        true, &junk_concepts,
                                                        true, false);
      }

      const concept_table_item *sub_table_item =
         &concept_table[substandard_concptptr->concept->kind];
      uint32_t sub_prop_bits = sub_table_item->concept_prop;

      // If the "arg2_matrix" bit is on, pick up additional
      // matrix descriptor bits from the arg2 word.
      uint32_t sub_prop_bits_for_expansion = sub_prop_bits;
      if (sub_prop_bits & CONCPROP__NEED_ARG2_MATRIX)
         sub_prop_bits_for_expansion |= substandard_concptptr->concept->arg2;

      if (junk_concepts.test_for_any_herit_or_final_bit() ||
          (!(sub_prop_bits & CONCPROP__STANDARD)))
         fail("This concept must be used with some offset/distorted/phantom concept.");

      /* We don't think stepping to a wave is ever a good idea if standard is used.
         Most calls that permit standard (CONCPROP__STANDARD is on) forbid it anyway
         (CONCPROP__NO_STEP is on also), but a few (e.g. concept_triple_lines)
         permit standard but don't necessarily forbid stepping to a wave.
         This is so that interesting cases of triple columns turn and weave will work.
         However, we think that this should be disallowed if "so-and-so are standard
         in triple lines" is given.  At least, that is the theory behind
         this next line of code. */

      ss->cmd.cmd_misc_flags |= CMD_MISC__NO_STEP_TO_WAVE;

      if (sub_prop_bits & CONCPROP__SET_PHANTOMS)
         ss->cmd.cmd_misc_flags |= CMD_MISC__PHANTOMS;

      if (!(ss->cmd.cmd_misc_flags & CMD_MISC__NO_EXPAND_MATRIX))
         ss->do_matrix_expansion(sub_prop_bits_for_expansion, false);

      ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;

      if (attr::slimit(ss) < 0) fail("Can't do this concept in this setup.");

      uint32_t tbonetest;
      uint32_t stdtest;
      int livemask = get_standard_people(ss, this_concept_parse_block->options.who.who[0],
                                         tbonetest, stdtest);

      if (!tbonetest) {
         result->kind = nothing;
         clear_result_flags(result);
         return true;
      }

      if ((tbonetest & 011) != 011)
         fail("People are not T-boned -- 'standard' is meaningless.");

      if (!stdtest) fail("No one is standard.");
      if ((stdtest & 011) == 011) fail("The standard people are not facing consistently.");

      global_tbonetest = stdtest;
      global_livemask = livemask;
      orig_tbonetest = tbonetest;

      ss->cmd.parseptr = substandard_concptptr->next;
      (sub_table_item->concept_action) (ss, substandard_concptptr, result);
      remove_tgl_distortion(result);
      // Beware -- result is not necessarily canonicalized.
      result->result_flags.clear_split_info();  // **** For now.
      return true;
   }

   // If matrix expansion has already occurred, don't do it again,
   // except for the special cases: only the CMD_MISC__NO_EXPAND_1 bit is set,
   // and this concept is some kind of "triple boxes" or "split phantom boxes".
   // Also, only the CMD_MISC__NO_EXPAND_2 bit is set,
   // and this concept is split phantom CLW.

   if (!(ss->cmd.cmd_misc_flags & CMD_MISC__NO_EXPAND_AT_ALL)) {
      if ((!(ss->cmd.cmd_misc_flags & CMD_MISC__NO_EXPAND_1) || ok_for_expand_1(this_concept)) &&
          (!(ss->cmd.cmd_misc_flags & CMD_MISC__NO_EXPAND_2) || ok_for_expand_2(this_concept)))
         ss->do_matrix_expansion(prop_bits_for_expansion, false);
   }

   // We can no longer do any matrix expansion, unless this is "phantom" and "tandem",
   // in which case we continue to allow it.  The code for the "C1 phantom" concept
   // will check whether it is being used with some tandem-like concept, and expand to
   // the matrix that it really wants if so, or set the NO_EXPAND_MATRIX flag if not.
   // We also don't set the flag if this concept was "fractional" or stable or
   // fractional stable.  Those concepts can be effectively oblivious to matrix expansion.
   // The tests for these are (from lines out) "stable 2x8 matrix roll em" and
   // "1/4 (fractional) split phantom boxes split the difference".

   // However, we allow one case of "stacked phantom expansion":
   //   split phantom C/L/W (expanding to a 4x4 and thence a pair of 2x4's)
   //   followed by triple boxes (expanding each 2x4 to a 2x6).
   // We do this by turning on only CMD_MISC__NO_EXPAND_1 if the concept is
   // split phantom C/L/W.  Most other parts of the code will reject any
   // further expansion if either flag is on.
   //
   // Also want split phantom boxes (expand to a 2x8 and thence a pair of 2x4's)
   // followed by split phantom C/L/W.  We turn on CMD_MISC__NO_EXPAND_2.

   if (!(prop_bits & CONCPROP__MATRIX_OBLIVIOUS)) {
      if (ok_for_expand_2(this_concept))
         ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_1;
      else if (ok_for_expand_1(this_concept))
         ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_2;
      else
         ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_AT_ALL;
   }

   // Various concepts are allergic to s2x2dmd setups.
   if ((this_kind == concept_do_phantom_2x4  || this_kind == concept_distorted  || this_kind == concept_misc_distort) &&
       ss->kind == s2x2dmd)
      ss->do_matrix_expansion(CONCPROP__NEEDK_4X6, false);

   // See if this concept can be invoked with "standard".  If so, it wants
   // tbonetest and livemask computed, and expects the former to indicate
   // only the standard people.

   if (prop_bits & (CONCPROP__STANDARD | CONCPROP__GET_MASK)) {
      int i;
      uint32_t j;
      bool doing_select;
      who_list saved_selector = current_options.who;

      if (attr::slimit(ss) < 0) fail("Can't do this concept in this setup.");

      global_tbonetest = 0;
      global_livemask = 0;
      global_selectmask = 0;
      global_tboneselect = 0;
      doing_select = (prop_bits & CONCPROP__USE_SELECTOR) != 0;

      if (doing_select) {
         current_options.who = this_concept_parse_block->options.who;
      }

      for (i=0, j=1; i<=attr::slimit(ss); i++, j<<=1) {
         uint32_t p = ss->people[i].id1;
         global_tbonetest |= p;
         if (p) {
            global_livemask |= j;
            if (doing_select && selectp(ss, i)) {
               global_selectmask |= j; global_tboneselect |= p;
            }
         }
      }

      current_options.who = saved_selector;

      orig_tbonetest = global_tbonetest;

      if (!global_tbonetest) {
         result->kind = nothing;
         clear_result_flags(result);
         return true;
      }
   }

   // We want the "paranoid" concept to know, if it was called with a concept
   // like "initially", whether some other concept (like "stable") intervened.
   if (!(this_kind == concept_special_sequential_no_2nd ||
         this_kind == concept_special_sequential_sel_no_2nd))
      ss->cmd.cmd_misc3_flags &= ~CMD_MISC3__PARTS_OVER_THIS_CONCEPT;

   (*concept_func)(ss, this_concept_parse_block, result);
   remove_tgl_distortion(result);
   // Beware -- result is not necessarily canonicalized.
   if (!(prop_bits & CONCPROP__SHOW_SPLIT))
      result->result_flags.clear_split_info();
   return true;
}


enum {
   Standard_matrix_phantom = (CONCPROP__SET_PHANTOMS | CONCPROP__PERMIT_MATRIX | CONCPROP__STANDARD),
   Nostandard_matrix_phantom = (CONCPROP__SET_PHANTOMS | CONCPROP__PERMIT_MATRIX),
   Nostep_phantom = (CONCPROP__NO_STEP | CONCPROP__SET_PHANTOMS)
};


// Beware!!  This table must be keyed to definition of "concept_kind" in sd.h .
const concept_table_item concept_table[] = {
   {CONCPROP__USES_PARTS, 0},                               // concept_another_call_next_mod
   {0, 0},                                                  // concept_mod_declined
   {0, 0},                                                  // marker_end_of_list
   {0, 0},                                                  // concept_comment
   {CONCPROP__SHOW_SPLIT | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__PERMIT_MODIFIERS,
    do_concept_concentric},                                 // concept_concentric
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT,
    do_concept_tandem},                                     // concept_tandem
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__MATRIX_OBLIVIOUS |
    CONCPROP__SHOW_SPLIT | CONCPROP__USE_SELECTOR,
    do_concept_tandem},                                     // concept_some_are_tandem
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__MATRIX_OBLIVIOUS |
    CONCPROP__SHOW_SPLIT | CONCPROP__USE_TWO_NUMBERS,
    do_concept_tandem},                                     // concept_frac_tandem
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__MATRIX_OBLIVIOUS |
    CONCPROP__SHOW_SPLIT | CONCPROP__USE_TWO_NUMBERS |
    CONCPROP__USE_SELECTOR,
    do_concept_tandem},                                     // concept_some_are_frac_tandem
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__MATRIX_OBLIVIOUS |
    CONCPROP__SHOW_SPLIT | CONCPROP__SET_PHANTOMS,
    do_concept_tandem},                                     // concept_gruesome_tandem
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__MATRIX_OBLIVIOUS |
    CONCPROP__SHOW_SPLIT | CONCPROP__USE_TWO_NUMBERS |
    CONCPROP__SET_PHANTOMS,
    do_concept_tandem},                                     // concept_gruesome_frac_tandem
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__SHOW_SPLIT,
    complain_about_in_setup},                               // concept_tandem_in_setup
   {0, do_concept_checkerboard},                            // concept_checkerboard
   {CONCPROP__USE_SELECTOR | CONCPROP__GET_MASK,
    do_concept_checkerboard},                               // concept_sel_checkerboard
   {CONCPROP__USE_SELECTOR | CONCPROP__GET_MASK,
    anchor_someone_and_move},                               // concept_anchor
   {0, 0},                                                  // concept_reverse
   {0, 0},                                                  // concept_left
   {0, 0},                                                  // concept_grand
   {0, 0},                                                  // concept_magic
   {0, 0},                                                  // concept_cross
   {0, 0},                                                  // concept_single
   {0, 0},                                                  // concept_singlefile
   {0, 0},                                                  // concept_interlocked
   {0, 0},                                                  // concept_yoyo
   {0, 0},                                                  // concept_generous
   {0, 0},                                                  // concept_stingy
   {0, 0},                                                  // concept_fractal
   {0, 0},                                                  // concept_fast
   {0, 0},                                                  // concept_straight
   {0, 0},                                                  // concept_twisted
   {0, 0},                                                  // concept_rewind
   {0, 0},                                                  // concept_crossover
   {0, 0},                                                  // concept_inroll
   {0, 0},                                                  // concept_outroll
   {0, 0},                                                  // concept_splittrade
   {0, 0},                                                  // concept_bias
   {0, 0},                                                  // concept_biastrade
   {0, 0},                                                  // concept_orbit
   {0, 0},                                                  // concept_twinorbit
   {0, 0},                                                  // concept_rotary
   {0, 0},                                                  // concept_scatter
   {0, 0},                                                  // concept_intpgram,
   {0, 0},                                                  // concept_trap,
   {0, 0},                                                  // concept_zoomroll
   {0, 0},                                                  // concept_12_matrix
   {0, 0},                                                  // concept_16_matrix
   {0, 0},                                                  // concept_revert
   {0, 0},                                                  // concept_1x2
   {0, 0},                                                  // concept_2x1
   {0, 0},                                                  // concept_2x2
   {0, 0},                                                  // concept_1x3
   {0, 0},                                                  // concept_3x1
   {0, 0},                                                  // concept_3x0
   {0, 0},                                                  // concept_0x3
   {0, 0},                                                  // concept_4x0
   {0, 0},                                                  // concept_0x4
   {0, 0},                                                  // concept_6x2
   {0, 0},                                                  // concept_3x2
   {0, 0},                                                  // concept_3x5
   {0, 0},                                                  // concept_5x3
   {0, 0},                                                  // concept_3x3
   {0, 0},                                                  // concept_4x4
   {0, 0},                                                  // concept_5x5
   {0, 0},                                                  // concept_6x6
   {0, 0},                                                  // concept_7x7
   {0, 0},                                                  // concept_8x8
   {CONCPROP__NEED_ARG2_MATRIX | Nostep_phantom,
    do_concept_expand_some_matrix},                         // concept_create_matrix
   {0, do_concept_two_faced},                               // concept_two_faced
   {0, 0},                                                  // concept_funny
   {CONCPROP__NO_STEP | CONCPROP__GET_MASK | CONCPROP__PERMIT_MODIFIERS,
    do_tallshort6_move},                                    // concept_tallshort6
   {0, 0},                                                  // concept_split
   {CONCPROP__SHOW_SPLIT | CONCPROP__PERMIT_MATRIX | CONCPROP__GET_MASK,
    do_concept_do_each_1x4},                                // concept_each_1x4
   {0, 0},                                                  // concept_diamond
   {0, 0},                                                  // concept_triangle
   {0, 0},                                                  // concept_leadtriangle
   {0, do_concept_do_both_boxes},                           // concept_do_both_boxes
   {CONCPROP__SHOW_SPLIT, do_concept_once_removed},         // concept_once_removed
   {CONCPROP__NEEDK_4X4 | Nostep_phantom,
    do_concept_do_phantom_2x2},                             // concept_do_phantom_2x2
   {CONCPROP__NEEDK_4X4,
    do_concept_do_phantom_2x2},                             // concept_do_blocks_2x2
   {CONCPROP__NEEDK_2X8 | Nostandard_matrix_phantom,
    do_concept_do_phantom_boxes},                           // concept_do_phantom_boxes
   {CONCPROP__NEEDK_4D_4PTPD | CONCPROP__NO_STEP | Nostandard_matrix_phantom,
    do_concept_do_phantom_diamonds},                        // concept_do_phantom_diamonds
   {CONCPROP__NEEDK_QUAD_1X4 | Standard_matrix_phantom | CONCPROP__PERMIT_MYSTIC,
    do_phantom_2x4_concept},                                // concept_do_phantom_2x4
   {CONCPROP__NEEDK_4X4 | CONCPROP__NO_STEP | Nostandard_matrix_phantom,
    do_phantom_stag_qtg_concept},                           // concept_do_phantom_stag_qtg
   {CONCPROP__NEEDK_4X4 | CONCPROP__NO_STEP | Nostandard_matrix_phantom,
    do_phantom_diag_qtg_concept},                           // concept_do_phantom_diag_qtg
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__NO_STEP | CONCPROP__GET_MASK |
    Nostandard_matrix_phantom,
    do_concept_do_twinphantom_diamonds},                    // concept_do_twinphantom_diamonds
   {CONCPROP__NO_STEP | CONCPROP__GET_MASK |
    Nostandard_matrix_phantom,
    do_concept_do_divided_bones},                           // concept_do_divided_bones
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__STANDARD,
    do_concept_distorted},                                  // concept_distorted
   {CONCPROP__NO_STEP | CONCPROP__GET_MASK,
    do_concept_single_diagonal},                            // concept_single_diagonal
   {CONCPROP__NO_STEP | CONCPROP__STANDARD,
    do_concept_double_diagonal},                            // concept_double_diagonal
   {CONCPROP__GET_MASK,
    do_concept_parallelogram},                              // concept_parallelogram
   {CONCPROP__NEED_ARG2_MATRIX | Standard_matrix_phantom,
    do_concept_multiple_lines},                             // concept_multiple_lines
   {CONCPROP__NEED_ARG2_MATRIX | Nostandard_matrix_phantom,
    do_concept_multiple_lines_tog},                         // concept_multiple_lines_tog
   {CONCPROP__NEED_ARG2_MATRIX | Standard_matrix_phantom,
    do_concept_multiple_lines_tog},                         // concept_multiple_lines_tog_std
   {CONCPROP__NEEDK_3X8 | Nostandard_matrix_phantom,
    do_concept_triple_1x8_tog},                             // concept_triple_1x8_tog
   {CONCPROP__NEED_ARG2_MATRIX | Nostandard_matrix_phantom,
    do_concept_multiple_boxes},                             // concept_multiple_boxes
   {CONCPROP__NEEDK_2X8 | CONCPROP__NO_STEP | Nostandard_matrix_phantom,
    do_concept_quad_boxes_tog},                             // concept_quad_boxes_together
   {CONCPROP__NEEDK_2X6 | Nostandard_matrix_phantom,
    do_concept_triple_boxes_tog},                           // concept_triple_boxes_together
   {CONCPROP__NEEDK_2X8 | Nostandard_matrix_phantom,
    do_concept_offset_triple_boxes_tog},                    // concept_offset_triple_boxes_tog
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__NO_STEP | Nostandard_matrix_phantom,
    do_concept_multiple_diamonds},                          // concept_multiple_diamonds
   {CONCPROP__NO_STEP | Nostandard_matrix_phantom,
    do_concept_multiple_formations},                        // concept_multiple_formations
   {CONCPROP__NEEDK_3DMD | CONCPROP__NO_STEP | Nostandard_matrix_phantom,
    do_concept_triple_diamonds_tog},                        // concept_triple_diamonds_together
   {CONCPROP__NEEDK_4D_4PTPD | CONCPROP__NO_STEP | Nostandard_matrix_phantom,
    do_concept_quad_diamonds_tog},                          // concept_quad_diamonds_together
   {CONCPROP__NEED_ARG2_MATRIX | Nostep_phantom | CONCPROP__GET_MASK,
    do_concept_do_triangular_boxes},                        // concept_triangular_boxes
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__STANDARD | CONCPROP__PERMIT_MATRIX | Nostep_phantom,
    do_concept_inner_outer},                                // concept_in_out_std
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__PERMIT_MATRIX | Nostep_phantom,
    do_concept_inner_outer},                                // concept_in_out_nostd
   {CONCPROP__NEEDK_BLOB | Nostep_phantom | CONCPROP__STANDARD,
    do_concept_triple_diag},                                // concept_triple_diag
   {CONCPROP__NEEDK_BLOB | Nostep_phantom | CONCPROP__GET_MASK,
    do_concept_triple_diag_tog},                            // concept_triple_diag_together
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__NO_STEP | Standard_matrix_phantom |
    CONCPROP__PERMIT_MYSTIC,
    triple_twin_move},                                      // concept_triple_twin
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__NO_STEP | Standard_matrix_phantom,
    triple_twin_move},                                      // concept_triple_twin_nomystic
   {CONCPROP__NEED_ARG2_MATRIX, distorted_2x2s_move},       // concept_misc_distort
   {CONCPROP__NEED_ARG2_MATRIX | CONCPROP__PERMIT_MATRIX,
    distorted_2x2s_move},                                   // concept_misc_distort_matrix
   {0, do_concept_stretch},                                 // concept_stretch
   {CONCPROP__GET_MASK, do_concept_stretched_setup},        // concept_stretched_setup
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT,
    do_concept_assume_waves},                               // concept_assume_waves
   {0, do_concept_active_phantoms},                         // concept_active_phantoms
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT,
    do_concept_mirror},                                     // concept_mirror
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT | CONCPROP__PERMIT_MODIFIERS,
    do_concept_central},                                    // concept_central
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT | CONCPROP__PERMIT_MODIFIERS,
    do_concept_rectify},                                    // concept_rectify
   {CONCPROP__MATRIX_OBLIVIOUS, do_concept_central},        // concept_snag_mystic
   {CONCPROP__PERMIT_MODIFIERS | CONCPROP__USES_PARTS,
    do_concept_crazy},                                      // concept_crazy
   {CONCPROP__USE_NUMBER | CONCPROP__PERMIT_MODIFIERS | CONCPROP__USES_PARTS,
    do_concept_crazy},                                      // concept_frac_crazy
   {CONCPROP__USE_TWO_NUMBERS | CONCPROP__PERMIT_MODIFIERS | CONCPROP__USES_PARTS,
    do_concept_crazy},                                      // concept_dbl_frac_crazy
   {CONCPROP__PERMIT_MODIFIERS | CONCPROP__NEED_ARG2_MATRIX |
    CONCPROP__SET_PHANTOMS | CONCPROP__STANDARD | CONCPROP__USES_PARTS,
    do_concept_phan_crazy},                                 // concept_phan_crazy
   {CONCPROP__PERMIT_MODIFIERS | CONCPROP__NEED_ARG2_MATRIX | CONCPROP__USE_NUMBER |
    CONCPROP__SET_PHANTOMS | CONCPROP__STANDARD | CONCPROP__USES_PARTS,
    do_concept_phan_crazy},                                 // concept_frac_phan_crazy
   {0, do_concept_fan},                                     // concept_fan
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__GET_MASK,
    do_c1_phantom_move},                                    // concept_c1_phantom
   {CONCPROP__PERMIT_MATRIX | CONCPROP__SET_PHANTOMS,
    do_concept_grand_working},                              // concept_grand_working
   {0, do_concept_centers_or_ends},                         // concept_centers_or_ends
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__USE_SELECTOR,
    so_and_so_only_move},                                   // concept_so_and_so_only
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__USE_SELECTOR | CONCPROP__SECOND_CALL,
    so_and_so_only_move},                                   // concept_some_vs_others
   {CONCPROP__MATRIX_OBLIVIOUS,
    so_and_so_only_move},                                   // concept_same_sex_disconnected
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT,
    do_concept_stable},                                     // concept_stable
   {CONCPROP__USE_SELECTOR | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT,
    do_concept_stable},                                     // concept_so_and_so_stable
   {CONCPROP__USE_NUMBER | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS,
    do_concept_stable},                                     // concept_frac_stable
   {CONCPROP__USE_SELECTOR | CONCPROP__USE_NUMBER |
    CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS,
    do_concept_stable},                                     // concept_so_and_so_frac_stable
   {CONCPROP__USE_DIRECTION | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT,
    do_concept_nose},                                       // concept_nose
   {CONCPROP__USE_DIRECTION | CONCPROP__USE_SELECTOR | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT,
    do_concept_nose},                                       // concept_so_and_so_nose
   {CONCPROP__MATRIX_OBLIVIOUS,
    do_concept_emulate},                                    // concept_emulate
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__USE_SELECTOR,
    mimic_move},                                            // concept_mimic
   {CONCPROP__USE_SELECTOR | CONCPROP__NO_STEP | CONCPROP__PERMIT_MATRIX,
    do_concept_standard},                                   // concept_standard
   {CONCPROP__MATRIX_OBLIVIOUS, do_concept_matrix},         // concept_matrix
   {CONCPROP__NO_STEP | CONCPROP__GET_MASK | CONCPROP__USE_SELECTOR,
    do_concept_double_offset},                              // concept_double_offset
   {CONCPROP__SECOND_CALL | CONCPROP__PERMIT_MODIFIERS,
    do_concept_checkpoint},                                 // concept_checkpoint
   {0,
    do_concept_checkpoint},                                 // concept_checkpoint_it_it
   {CONCPROP__SECOND_CALL | CONCPROP__NO_STEP,
    on_your_own_move},                                      // concept_on_your_own
   {CONCPROP__SECOND_CALL | CONCPROP__PERMIT_MODIFIERS | CONCPROP__NO_STEP,
    do_concept_trace},                                      // concept_trace
   {CONCPROP__USE_SELECTOR | CONCPROP__GET_MASK,
    do_concept_move_in_and},                                // concept_move_in_and
   {CONCPROP__NO_STEP,
    do_concept_outeracting},                                // concept_outeracting
   {CONCPROP__NO_STEP | CONCPROP__GET_MASK | CONCPROP__PERMIT_MODIFIERS,
    do_concept_ferris},                                     // concept_ferris
   {CONCPROP__NO_STEP, do_concept_overlapped_diamond},      // concept_overlapped_diamond
   {0, do_concept_all_8},                                   // concept_all_8
   {CONCPROP__SECOND_CALL, do_concept_centers_and_ends},    // concept_centers_and_ends
   {0, do_concept_mini_but_o},                              // concept_mini_but_o
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS | CONCPROP__PERMIT_MODIFIERS,
    do_concept_n_times},                                    // concept_n_times_const
   {CONCPROP__USE_NUMBER | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT |
    CONCPROP__USES_PARTS | CONCPROP__PERMIT_MODIFIERS,
    do_concept_n_times},                                    // concept_n_times
   {CONCPROP__SECOND_CALL | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS,
    do_concept_sequential},                                 // concept_sequential
   {CONCPROP__SECOND_CALL | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS,
    do_concept_special_sequential},                         // concept_special_sequential
   {CONCPROP__USE_SELECTOR | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT |
    CONCPROP__USES_PARTS,
    do_concept_special_sequential},                         // concept_special_sequential_sel
   {CONCPROP__SECOND_CALL | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT |
    CONCPROP__USE_NUMBER | CONCPROP__USES_PARTS,
    do_concept_special_sequential},                         // concept_special_sequential_num
   {CONCPROP__SECOND_CALL | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT |
    CONCPROP__USE_FOUR_NUMBERS | CONCPROP__USES_PARTS,
    do_concept_special_sequential},                         // concept_special_sequential_4num
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS,
    do_concept_special_sequential},                         // concept_special_sequential_no_2nd
   {CONCPROP__USE_SELECTOR | CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS,
    do_concept_special_sequential},                         // concept_special_sequential_sel_no_2nd
   {CONCPROP__MATRIX_OBLIVIOUS |
    CONCPROP__SHOW_SPLIT | CONCPROP__PERMIT_REVERSE |
    CONCPROP__IS_META | CONCPROP__USES_PARTS,
    do_concept_meta},                                       // concept_meta
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__USE_NUMBER |
    CONCPROP__SHOW_SPLIT | CONCPROP__PERMIT_REVERSE |
    CONCPROP__IS_META | CONCPROP__USES_PARTS,
    do_concept_meta},                                       // concept_meta_one_arg
   {CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__USE_TWO_NUMBERS |
    CONCPROP__SHOW_SPLIT | CONCPROP__PERMIT_REVERSE |
    CONCPROP__IS_META | CONCPROP__USES_PARTS,
    do_concept_meta},                                       // concept_meta_two_args
   {CONCPROP__USE_SELECTOR | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS,
    do_concept_so_and_so_begin},                            // concept_so_and_so_begin
   {CONCPROP__USE_NUMBER | CONCPROP__SECOND_CALL | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS,
    do_concept_replace_nth_part},                           // concept_replace_nth_part
   {CONCPROP__SECOND_CALL | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS,
    do_concept_replace_nth_part},                           // concept_replace_last_part
   {CONCPROP__USE_TWO_NUMBERS |
    CONCPROP__SECOND_CALL | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS,
    do_concept_replace_nth_part},                           // concept_interrupt_at_fraction
   {CONCPROP__SECOND_CALL | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS,
    do_concept_replace_nth_part},                           // concept_sandwich
   {CONCPROP__SECOND_CALL | CONCPROP__SHOW_SPLIT | CONCPROP__USES_PARTS,
    do_concept_interlace},                                  // concept_interlace
   {CONCPROP__USE_TWO_NUMBERS |
    CONCPROP__MATRIX_OBLIVIOUS | CONCPROP__SHOW_SPLIT,
    do_concept_fractional},                                 // concept_fractional
   {CONCPROP__NO_STEP, do_concept_rigger},                  // concept_rigger
   {CONCPROP__NO_STEP | CONCPROP__MATRIX_OBLIVIOUS,
    do_concept_wing},                                       // concept_wing
   {CONCPROP__NO_STEP | CONCPROP__MATRIX_OBLIVIOUS,
    common_spot_move},                                      // concept_common_spot
   {CONCPROP__USE_SELECTOR | CONCPROP__SHOW_SPLIT,
    drag_someone_and_move},                                 // concept_drag
   {CONCPROP__NO_STEP | CONCPROP__GET_MASK,
    do_concept_dblbent},                                    // concept_dblbent
   {CONCPROP__USES_PARTS, do_concept_omit},                 // concept_omit
   {CONCPROP__USES_PARTS, 0},                               // concept_supercall
   {0, do_concept_diagnose}};                               // concept_diagnose
