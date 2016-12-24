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

/* This defines the following functions:
   canonicalize_rotation
   reinstate_rotation
   remove_mxn_spreading
   remove_fudgy_2x3_2x6
   repair_fudgy_2x3_2x6
   do_1x3_type_expansion
   divide_for_magic
   do_simple_split
   do_call_in_series
   initialize_matrix_position_tables
   drag_someone_and_move
   anchor_someone_and_move
   process_number_insertion
   get_real_subcall
   gcd
   process_fractions
   fraction_info::get_fraction_info
   fraction_info::get_fracs_for_this_part
   fraction_info::query_instant_stop
   try_to_get_parts_from_parse_pointer
   fill_active_phantoms_and_move
   move_perhaps_with_active_phantoms
   impose_assumption_and_move
   really_inner_move
   move
*/

#include <stdio.h>
#include <string.h>

#include "sd.h"

extern void canonicalize_rotation(setup *result) THROW_DECL
{
   result->rotation &= 3;

   if (result->kind == s1x1) {
      result->rotate_person(0, (result->rotation) * 011);
      result->rotation = 0;
   }
   else if (result->kind == s_normal_concentric) {
      int i;

      if (result->inner.skind == s_normal_concentric ||
          result->outer.skind == s_normal_concentric ||
          result->inner.skind == s_dead_concentric ||
          result->outer.skind == s_dead_concentric)
         fail("Recursive concentric?????.");

      int save_rotation = result->rotation;

      result->kind = result->inner.skind;
      result->rotation += result->inner.srotation;
      canonicalize_rotation(result);    // Sorry!
      result->inner.srotation = result->rotation;

      result->kind = result->outer.skind;
      result->rotation = save_rotation + result->outer.srotation;
      for (i=0 ; i<12 ; i++) result->swap_people(i, i+12);
      canonicalize_rotation(result);    // Sorrier!
      for (i=0 ; i<12 ; i++) result->swap_people(i, i+12);
      result->outer.srotation = result->rotation;

      result->kind = s_normal_concentric;
      result->rotation = 0;
   }
   else if (result->kind == s_dead_concentric) {
      if (result->inner.skind == s_normal_concentric || result->inner.skind == s_dead_concentric)
         fail("Recursive concentric?????.");

      result->kind = result->inner.skind;
      result->rotation += result->inner.srotation;
      canonicalize_rotation(result);    // Sorry!
      result->inner.srotation = result->rotation;

      result->kind = s_dead_concentric;
      result->rotation = 0;
   }
   else if (setup_attrs[result->kind].four_way_symmetry) {
      // The setup has 4-way symmetry.  We can canonicalize it so the
      // result rotation is zero.
      int i, rot, rot11, delta, bigd, i0, i1, i2, i3, j0, j1, j2, j3;
      personrec x0, x1, x2, x3;

      rot = result->rotation;
      if (rot == 0) return;
      rot11 = rot * 011;
      bigd = attr::slimit(result) + 1;
      if (result->kind == s3x3) bigd = 8;
      delta = bigd >> 2;

      i0 = 1;
      i1 = i0 + delta;
      i2 = i1 + delta;
      i3 = i2 + delta;
      j0 = (rot-4)*delta+1;
      j1 = j0 + delta;
      j2 = j1 + delta;
      j3 = j2 + delta;
      for (i=0; i<delta; i++) {
         if ((--i0) < 0) i0 += bigd;
         if ((--i1) < 0) i1 += bigd;
         if ((--i2) < 0) i2 += bigd;
         if ((--i3) < 0) i3 += bigd;
         if ((--j0) < 0) j0 += bigd;
         if ((--j1) < 0) j1 += bigd;
         if ((--j2) < 0) j2 += bigd;
         if ((--j3) < 0) j3 += bigd;
         x0 = result->people[i0];
         x1 = result->people[i1];
         x2 = result->people[i2];
         x3 = result->people[i3];
         result->people[j0].id1 = rotperson(x0.id1, rot11);
         result->people[j0].id2 = x0.id2;
         result->people[j0].id3 = x0.id3;
         result->people[j1].id1 = rotperson(x1.id1, rot11);
         result->people[j1].id2 = x1.id2;
         result->people[j1].id3 = x1.id3;
         result->people[j2].id1 = rotperson(x2.id1, rot11);
         result->people[j2].id2 = x2.id2;
         result->people[j2].id3 = x2.id3;
         result->people[j3].id1 = rotperson(x3.id1, rot11);
         result->people[j3].id2 = x3.id2;
         result->people[j3].id3 = x3.id3;
      }

      if (result->kind == s3x3) result->people[8].id1 = rotperson(result->people[8].id1, rot11);

      result->rotation = 0;
   }
   else if (setup_attrs[result->kind].no_symmetry) {
   }
   else if (result->kind == s1x3) {
      if (result->rotation & 2) {
         // Must turn this setup upside-down.
         result->swap_people(0, 2);
         for (int i=0; i<3; i++)
            result->rotate_person(i, 022);
      }
      result->rotation &= 1;
   }
   else if (result->kind == s1x5) {
      if (result->rotation & 2) {
         // Must turn this setup upside-down.
         result->swap_people(0, 3);
         result->swap_people(1, 4);
         for (int i=0; i<5; i++)
            result->rotate_person(i, 022);
      }
      result->rotation &= 1;
   }
   else if (((attr::slimit(result) & ~07776) == 1)) {
      // We have a setup of an even number of people.  We know how to canonicalize
      // this.  The resulting rotation should be 0 or 1.

      if (result->rotation & 2) {

         // Must turn this setup upside-down.

         int offs = (attr::slimit(result)+1) >> 1;     // Half the setup size.

         for (int i=0; i<offs; i++) {
            result->swap_people(i, i+offs);
            result->rotate_person(i, 022);
            result->rotate_person(i+offs, 022);
         }
      }

      result->rotation &= 1;
   }
}


extern void reinstate_rotation(const setup *ss, setup *result) THROW_DECL
{
   int globalrotation;

   switch (ss->kind) {
      case s_dead_concentric:
      case s_normal_concentric:
         globalrotation = 0;
         break;
      default:
         globalrotation = ss->rotation;
   }

   switch (result->kind) {
      case s_normal_concentric:
         result->inner.srotation += globalrotation;
         result->outer.srotation += globalrotation;
         if ((globalrotation & 1) && ((result->concsetup_outer_elongation + 1) & 2))
            result->concsetup_outer_elongation ^= 3;
         break;
      case s_dead_concentric:
         result->inner.srotation += globalrotation;
         if ((globalrotation & 1) && ((result->concsetup_outer_elongation + 1) & 2))
            result->concsetup_outer_elongation ^= 3;
         break;
      case nothing:
         break;
      default:
         result->rotation += globalrotation;
         break;
   }

   if (ss->eighth_rotation) {
      result->eighth_rotation ^= 1;
      if (!result->eighth_rotation)
         result->rotation++;
   }

   // If we turned by 90 degrees, we have to to swap the "split_info" fields.

   if (globalrotation & 1)
      result->result_flags.swap_split_info_fields();

   canonicalize_rotation(result);
}


static const expand::thing exp72  = {{1, 3, 4, 5, 7, 9, 10, 11}, s2x4, s2x6, 0, 0, 07272};
static const expand::thing exp27  = {{0, 1, 2, 4, 6, 7, 8, 10},  s2x4, s2x6, 0, 0, 02727};
static const expand::thing exp52  = {{1, 3, 5, 7, 9, 11},        s2x3, s2x6, 0, 0, 05252};
static const expand::thing exp25  = {{0, 2, 4, 6, 8, 10},        s2x3, s2x6, 0, 0, 02525};
static const expand::thing exp56  = {{1, 2, 3, 5, 7, 8, 9, 11},  s2x4, s2x6, 0, 0, 05656};
static const expand::thing exp65  = {{0, 2, 4, 5, 6, 8, 10, 11}, s2x4, s2x6, 0, 0, 06565};
static const expand::thing exp35  = {{0, 2, 3, 4, 6, 8, 9, 10},  s2x4, s2x6, 0, 0, 03535};
static const expand::thing exp53  = {{0, 1, 3, 5, 6, 7, 9, 11},  s2x4, s2x6, 0, 0, 05353};
static const expand::thing exp3u  = {{1, 3, 4, 5, 6, 7, 8, 10},  s2x4, s2x6, 0, 0, 02772};
static const expand::thing exp3d  = {{0, 1, 2, 4, 7, 9, 10, 11}, s2x4, s2x6, 0, 0, 07227};


static const expand::thing exp70 = {{3, 4, 5, 9, 10, 11}, s2x3, s2x6, 0, 0, 07070};
static const expand::thing exp07 = {{0, 1, 2, 6, 7, 8}, s2x3, s2x6, 0, 0, 00707};



static const expand::thing expxF0 = {{4, 5, 6, 7, 12, 13, 14, 15}, s2x4, s2x8, 0, 0, 0xF0F0};
static const expand::thing expx0F = {{0, 1, 2, 3, 8, 9, 10, 11}, s2x4, s2x8, 0, 0, 0x0F0F};
static const expand::thing expxAA = {{1, 3, 5, 7, 9, 11, 13, 15}, s2x4, s2x8, 0, 0, 0xAAAA};
static const expand::thing expx55 = {{0, 2, 4, 6, 8, 10, 12, 14}, s2x4, s2x8, 0, 0, 0x5555};
static const expand::thing exp4x4x71 = {{12, 13, 14, 0, 4, 5, 6, 8}, s2x4, s4x4, 0, 0, 0x7171};
static const expand::thing exp4x4x8E = {{10, 15, 3, 1, 2, 7, 11, 9}, s2x4, s4x4, 0, 0, 0x8E8E};
static const expand::thing exp4x4x17 = {{8, 9, 10, 12, 0, 1, 2, 4}, s2x4, s4x4, 1, 0, 0x1717};
static const expand::thing exp4x4xE8 = {{6, 11, 15, 13, 14, 3, 7, 5}, s2x4, s4x4, 1, 0, 0xE8E8};

static const expand::thing exp12m = {{9, 10, 1, 3, 4, 7},        s_2x1dmd, s3dmd, 0, 0, 03232};
static const expand::thing exp21m = {{8, 11, 0, 2, 5, 6},        s2x3, s3dmd, 1, 0, 04545};

static const expand::thing exp46  = {{1, 2, 5, 7, 8, 11},        s2x3, s2x6, 0, 0, 04646};
static const expand::thing exp64  = {{2, 4, 5, 8, 10, 11},       s2x3, s2x6, 0, 0, 06464};
static const expand::thing exp54  = {{2, 3, 5, 8, 9, 11},        s2x3, s2x6, 0, 0, 05454};
static const expand::thing exp45  = {{0, 2, 5, 6, 8, 11},        s2x3, s2x6, 0, 0, 04545};
static const expand::thing exp51  = {{0, 3, 5, 6, 9, 11},        s2x3, s2x6, 0, 0, 05151};
static const expand::thing exp15  = {{0, 2, 3, 6, 8, 9},         s2x3, s2x6, 0, 0, 01515};
static const expand::thing exp31  = {{0, 3, 4, 6, 9, 10},        s2x3, s2x6, 0, 0, 03131};
static const expand::thing exp13  = {{0, 1, 3, 6, 7, 9},         s2x3, s2x6, 0, 0, 01313};

static const expand::thing expb31 = {{9, 10, 0, 3, 4, 6},        s2x3, s3x4, 1, 0, 03131};
static const expand::thing expb46 = {{8, 11, 1, 2, 5, 7},        s2x3, s3x4, 1, 0, 04646};


static const expand::thing expl52 = {{1, 3, 5, 7, 9, 11},        s1x6, s1x12, 0, 0, 05252};
static const expand::thing expl25 = {{0, 2, 4, 6, 8, 10},        s1x6, s1x12, 0, 0, 02525};
static const expand::thing expg72 = {{1, 3, 5, 4, 7, 9, 11, 10}, s1x8, s1x12, 0, 0, 07272};
static const expand::thing expg27 = {{0, 1, 4, 2, 6, 7, 10, 8},  s1x8, s1x12, 0, 0, 02727};
static const expand::thing expg56 = {{1, 2, 5, 3, 7, 8, 11, 9},  s1x8, s1x12, 0, 0, 05656};
static const expand::thing expg35 = {{0, 2, 4, 3, 6, 8, 10, 9},  s1x8, s1x12, 0, 0, 03535};

static const expand::thing exprig12a = {{9, 10, 11, 1, 3, 4, 5, 7}, s1x3dmd, srigger12, 0, 0, 07272};
static const expand::thing exprig12b = {{0, 1, 2, 4, 6, 7, 8, 10}, s_spindle, srigger12, 0, 0, 02727};

static const expand::thing expsp3 = {{1, 2, 3, 5, 7, 8, 9, 11}, s_spindle, sd3x4, 0, 0, 05656};
static const expand::thing expd34s6 = {{9, 11, 1, 3, 5, 7}, s_short6, sd3x4, 1, 0, 05252};
static const expand::thing expd3423 = {{0, 2, 4, 6, 8, 10}, s2x3, sd3x4, 0, 0, 02525};
static const expand::thing exp3d3 = {{10, 11, 0, -1, -1, 2, 4, 5, 6, -1, -1, 8}, s3dmd, sd3x4, 1};
static const expand::thing exp323 = {{10, 11, 0, 2, 4, 5, 6, 8}, s_323, sd3x4, 1, 0, 06565};
static const expand::thing exp303 = {{10, 11, 0, 4, 5, 6}, s2x3, sd3x4, 1, 0, 06161};
static const expand::thing exp030 = {{1, 2, 3, 7, 8, 9}, s2x3, sd3x4, 0, 0, 01616};
static const expand::thing exp4141 = {{17, 18, 11, 0, 5, 6, 23, 12}, s2x4, s4x6, 1, 0, 041414141};



static const expand::thing expb51 = {{0, 3, 5, 6, 9, 11},        s_bone6, s3x4, 0, 0, 05151};
static const expand::thing expb26 = {{8, 10, 1, 2, 4, 7},        s_short6, s3x4, 1, 0, 02626};
static const expand::thing expl65 = {{0, 2, 4, 5},               s1x4, s1x6, 0, 0, 065};
static const expand::thing expl56 = {{1, 2, 3, 5},               s1x4, s1x6, 0, 0, 056};
static const expand::thing expl72 = {{1, 5, 3, 4},               s1x4, s1x6, 0, 0, 072};
static const expand::thing expl27 = {{0, 1, 4, 2},               s1x4, s1x6, 0, 0, 027};

void fix_roll_transparency_stupidly(const setup *ss, setup *result)
{
   // Can only do this if we understand the setups, as indicated by the "slimit" being defined.
   if ((ss->cmd.cmd_misc3_flags & (CMD_MISC3__ROLL_TRANSP|CMD_MISC3__ROLL_TRANSP_IF_Z)) != 0 &&
       (attr::slimit(result) >= 0) &&
       (attr::slimit(ss) >= 0)) {

      // We may turn on the "moving_people_cant_roll" flag.  But only if the
      // the starting and ending setups are the same.  (If they aren't the same,
      // we can't tell whether people moved.)  In this case, we only perform
      // the special "roll_transparent" operation for people who didn't move
      // to a different location.  For people who moved, we leave their
      // roll-neutral status in place.
      //
      // This is required for "and cross" in such things as percolate.
      // Everyone has hinged and therefore has roll direction.  The "and
      // cross" is invoked with "roll_transparent" on, so that the people
      // who don't move on the "and cross" will still be able to roll.
      // But the people who crossed need to preserve their "ZM" status
      // and not have it reset to the previous call.

      bool moving_people_cant_roll = ss->cmd.callspec &&
         (ss->cmd.callspec->the_defn.callflagsf & CFLAG2_IF_MOVE_CANT_ROLL) != 0 &&
         result->kind == ss->kind;

      for (int u=0; u<=attr::slimit(result); u++) {
         uint32 *thispid1 = &result->people[u].id1;

         if (*thispid1) {
            uint32 rollinfo = *thispid1 & ROLL_DIRMASK;

            // Look for people marked "M", that is, roll-neutral.
            // But if in "DFM1_ROLL_TRANSPARENT_IF_Z" mode, also look for people marked undefined,
            // and force them to "M" rather than being transparent.
            if ((ss->cmd.cmd_misc3_flags & CMD_MISC3__ROLL_TRANSP_IF_Z) != 0 && rollinfo == 0) {
               *thispid1 |= ROLL_IS_M;
            }
            else if (rollinfo == ROLL_DIRMASK) {
               // This person is roll-neutral.  Reinstate his original roll info.
               // But do *NOT* restore his "this person moved" bit from its
               // previous state.  Leave it in its current state.

               // But we do *NOT* do this if the before and after setups
               // were the same, the call was marked "moving_people_cant_roll",
               // and the person finished on a different spot.

               // Find this person in the starting setup.
               int v;
               for (v=0; v<=attr::slimit(ss); v++) {
                  if (((*thispid1 ^ ss->people[v].id1) & (XPID_MASK|BIT_PERSON)) == 0) {
                     break;
                  }
               }

               if (v > attr::slimit(ss))
                  fail("INTERNAL ERROR: LOST SOMEONE!!!!!");

               *thispid1 &= ~ROLL_DIRMASK;

               if (moving_people_cant_roll && (u != v)) {
                  *thispid1 |= ROLL_IS_M;  // This person moved.  Take away his rollability.
               }
               else {
                  *thispid1 |= (ss->people[v].id1 & ROLL_DIRMASK);   // Restore rollability from original.
               }
            }
         }
      }
   }
}


extern void remove_mxn_spreading(setup *ss) THROW_DECL
{
   if (!(ss->result_flags.misc & RESULTFLAG__DID_MXN_EXPANSION))
      return;

   uint32 livemasklittle = little_endian_live_mask(ss);

   static const expand::thing *unwind_2x6_table[] = {
      &exp52, &exp25, &exp72, &exp27,
      &exp56, &exp65, &exp35, &exp53,
      &exp46, &exp64, &exp54, &exp45,
      &exp51, &exp15, &exp31, &exp13,
      &exp70, &exp07,
      (const expand::thing *) 0};

   static const expand::thing *unwind_2x8_table[] = {
      &expx0F, &expxF0, &expx55, &expxAA,
      (const expand::thing *) 0};

   static const expand::thing *unwind_4x4_table[] = {
      &exp4x4x71, &exp4x4x8E, &exp4x4x17, &exp4x4xE8,
      (const expand::thing *) 0};

   static const expand::thing *unwind_3dmd_table[] = {
      &exp12m, &exp21m,
      (const expand::thing *) 0};

   static const expand::thing *unwind_3x4_table[] = {
      &s_qtg_3x4, &expb51, &expb26, &expb31, &expb46,
      (const expand::thing *) 0};

   static const expand::thing *unwind_1x6_table[] = {
      &expl72, &expl27, &expl56, &expl65,
      (const expand::thing *) 0};

   static const expand::thing *unwind_d3x4_table[] = {
      &exp323, &exp303, &expsp3, &expd34s6, &expd3423, &exp030,
      (const expand::thing *) 0};

   static const expand::thing *unwind_1x12_table[] = {
      &expl52, &expl25, &expg72, &expg27, &expg56, &expg35,
      (const expand::thing *) 0};

   static const expand::thing *unwind_rigger12_table[] = {
      &exprig12a, &exprig12b,
      (const expand::thing *) 0};

   static const expand::thing *unwind_4x6_table[] = {
      &exp4141,
      (const expand::thing *) 0};


   const expand::thing **p = (const expand::thing **) 0;

   switch (ss->kind) {
   case s2x8:
      p = unwind_2x8_table;
      break;
   case s4x4:
      p = unwind_4x4_table;
      break;
   case s2x6:
      p = unwind_2x6_table;
      break;
   case s3dmd:
      p = unwind_3dmd_table;
      break;
   case s3x4:
      p = unwind_3x4_table;
      break;
   case s1x6:
      p = unwind_1x6_table;
      break;
   case sd3x4:
      p = unwind_d3x4_table;
      break;
   case s1x12:
      p = unwind_1x12_table;
      break;
   case srigger12:
      p = unwind_rigger12_table;
      break;
   case s4x6:
      p = unwind_4x6_table;
      break;
   }

   const expand::thing *final = (const expand::thing *) 0;

   if (p) {
      for ( ; *p ; p++) {
         if (livemasklittle == (*p)->biglivemask) {
            // Exact match, accept it immediately.  We know it's unambiguous.
            final = *p;
            break;
         }
         else if ((livemasklittle & ~(*p)->biglivemask) == 0) {
            if (final) return;    // Ambiguous.
            final = *p;
         }
      }

      if (final) {
         expand::compress_setup(*final, ss);

         if (final == &expg72)
            ss->result_flags.misc |= RESULTFLAG__VERY_ENDS_ODD;
         else if (final == &expg27)
            ss->result_flags.misc |= RESULTFLAG__VERY_CTRS_ODD;

         ss->result_flags.misc &= ~RESULTFLAG__DID_MXN_EXPANSION;
      }
   }
}


extern void remove_fudgy_2x3_2x6(setup *ss) THROW_DECL
{
   if (ss->kind == sfudgy2x3l) {
      const expand::thing compressfudgy2x3l = {
         {0, 1, -1, 4, 5, -1}, s2x3, sfudgy2x3l, 0};
      expand::compress_setup(compressfudgy2x3l, ss);
   }
   else if (ss->kind == sfudgy2x3r) {
      const expand::thing compressfudgy2x3r = {
         {-1, 2, 3, -1, 6, 7}, s2x3, sfudgy2x3r, 0};
      expand::compress_setup(compressfudgy2x3r, ss);
   }
   else if (ss->kind == sfudgy2x6l) {
      const expand::thing compressfudgy2x6l = {
         {0, 1, 2, 3, -1, -1, 8, 9, 10, 11, -1, -1}, s2x6, sfudgy2x6l, 0};
      expand::compress_setup(compressfudgy2x6l, ss);
   }
   else if (ss->kind == sfudgy2x6r) {
      const expand::thing compressfudgy2x6r = {
         {-1, -1, 4, 5, 6, 7, -1, -1, 12, 13, 14, 15}, s2x6, sfudgy2x6r, 0};
      expand::compress_setup(compressfudgy2x6r, ss);
   }
   else
      return;

   warn(warn_verycontroversial);
}


// This turns s1p5x4/s1p5x8 things into sfudgy things.
extern void repair_fudgy_2x3_2x6(setup *ss) THROW_DECL
{
   if (ss->kind == s1p5x4) {
      uint32 mask = little_endian_live_mask(ss);
      if (mask != 0 && (mask & 0xCC) == 0)
         ss->kind = sfudgy2x3l;
      else if (mask != 0 && (mask & 0x33) == 0)
         ss->kind = sfudgy2x3r;
      else
         fail("Can't go into a 50% offset 1x4.");
   }
   else if (ss->kind == s1p5x8) {
      uint32 mask = little_endian_live_mask(ss);
      if (mask != 0 && (mask & 0xF0F0) == 0)
         ss->kind = sfudgy2x6l;
      else if (mask != 0 && (mask & 0x0F0F) == 0)
         ss->kind = sfudgy2x6r;
      else
         fail("Can't go into a 50% offset 1x8.");
   }
   else
      return;

   warn(warn_verycontroversial);
}


extern bool do_1x3_type_expansion(setup *ss, uint32 heritflags_to_check) THROW_DECL
{
   struct Nx1_checker {
      uint32 directions;
      bool unsymm;
      const expand::thing *action_if_Nx1;
      const expand::thing *action_if_1xN;
   };

   static const Nx1_checker Nx1_checktable_2x4[] = {
      {0x2A80, false, &exp72, &exp72},  // These are independent of whether we said "1x3" or "3x1".
      {0x6AC0, false, &exp72, &exp72},
      {0xEA40, false, &exp72, &exp72},
      // Then swap
      {0x802A, false, &exp72, &exp72},
      {0xC06A, false, &exp72, &exp72},
      {0x40EA, false, &exp72, &exp72},

      {0x7FD5, false, &exp72, &exp72},
      {0x3F95, false, &exp72, &exp72},
      {0xBF15, false, &exp72, &exp72},
      // Then swap
      {0xD57F, false, &exp72, &exp72},
      {0x953F, false, &exp72, &exp72},
      {0x15BF, false, &exp72, &exp72},

      {0xA802, false, &exp27, &exp27},
      {0xA903, false, &exp27, &exp27},
      {0xAB01, false, &exp27, &exp27},
      // Then swap
      {0x02A8, false, &exp27, &exp27},
      {0x03A9, false, &exp27, &exp27},
      {0x01AB, false, &exp27, &exp27},

      {0xFD57, false, &exp27, &exp27},
      {0xFC56, false, &exp27, &exp27},
      {0xFE54, false, &exp27, &exp27},
      // Then swap
      {0x57FD, false, &exp27, &exp27},
      {0x56FC, false, &exp27, &exp27},
      {0x54FE, false, &exp27, &exp27},

      {0x208A, false, &exp56, &exp56},
      {0x8A20, false, &exp56, &exp56},
      {0x75DF, false, &exp56, &exp56},
      {0xDF75, false, &exp56, &exp56},
      {0xA208, false, &exp35, &exp35},
      {0x08A2, false, &exp35, &exp35},
      {0xF75D, false, &exp35, &exp35},
      {0x5DF7, false, &exp35, &exp35},
      {0x2AA8, true,  &exp3u, &exp3u},  // These 8 are unsymmetrical.
      {0x8002, true,  &exp3u, &exp3u},
      {0x7FFD, true,  &exp3u, &exp3u},
      {0xD557, true,  &exp3u, &exp3u},
      {0xA82A, true,  &exp3d, &exp3d},
      {0x0280, true,  &exp3d, &exp3d},
      {0xFD7F, true,  &exp3d, &exp3d},
      {0x57D5, true,  &exp3d, &exp3d},
      {0x55FF, false, &exp72, &exp27},  // These are specific to "1x3" or "3x1".
      {0x00AA, false, &exp72, &exp27},
      {0xFF55, false, &exp27, &exp72},
      {0xAA00, false, &exp27, &exp72},
      {0}};

   static const Nx1_checker Nx0_checktable_2x3[] = {
      {02577, false, &exp70, &exp07},
      {00052, false, &exp70, &exp07},
      {07725, false, &exp07, &exp70},
      {05200, false, &exp07, &exp70},
      {0}};

   static const Nx1_checker Nx0_checktable_2x4[] = {
      {0x55FF, false, &expxF0, &expx0F},
      {0x00AA, false, &expxF0, &expx0F},
      {0xFF55, false, &expx0F, &expxF0},
      {0xAA00, false, &expx0F, &expxF0},
      {0}};

   static const Nx1_checker Nx1_checktable_1x8[] = {
      {0x2A80, false, &expg72, &expg72},  // These are independent of whether we said "1x3" or "3x1".
      {0x802A, false, &expg72, &expg72},
      {0x7FD5, false, &expg72, &expg72},
      {0xD57F, false, &expg72, &expg72},
      {0xA208, false, &expg27, &expg27},
      {0x08A2, false, &expg27, &expg27},
      {0xF75D, false, &expg27, &expg27},
      {0x5DF7, false, &expg27, &expg27},
      {0x208A, false, &expg56, &expg56},
      {0x8A20, false, &expg56, &expg56},
      {0x75DF, false, &expg56, &expg56},
      {0xDF75, false, &expg56, &expg56},
      {0xA802, false, &expg35, &expg35},
      {0x02A8, false, &expg35, &expg35},
      {0xFD57, false, &expg35, &expg35},
      {0x57FD, false, &expg35, &expg35},
      {0x55FF, false, &expg72, &expg27},  // These are specific to "1x3" or "3x1".
      {0x00AA, false, &expg72, &expg27},
      {0xFF55, false, &expg27, &expg72},
      {0xAA00, false, &expg27, &expg72},
      {0}};

   static const Nx1_checker Nx1_checktable_2x3[] = {
      {01240, false, &exp52, &exp52},  // These are independent of whether we said "1x2" or "2x1".
      {04012, false, &exp52, &exp52},
      {03765, false, &exp52, &exp52},
      {06537, false, &exp52, &exp52},
      {05002, false, &exp25, &exp25},
      {00250, false, &exp25, &exp25},
      {07527, false, &exp25, &exp25},
      {02775, false, &exp25, &exp25},
      {02577, false, &exp52, &exp25},  // These are specific to "1x2" or "2x1".
      {00052, false, &exp52, &exp25},
      {07725, false, &exp25, &exp52},
      {05200, false, &exp25, &exp52},
      {0}};

   static const Nx1_checker Nx1_checktable_1x6[] = {
      {01240, false, &expl52, &expl52},  // These are independent of whether we said "1x2" or "2x1".
      {04012, false, &expl52, &expl52},
      {03765, false, &expl52, &expl52},
      {06537, false, &expl52, &expl52},
      {05002, false, &expl25, &expl25},
      {00250, false, &expl25, &expl25},
      {07527, false, &expl25, &expl25},
      {02775, false, &expl25, &expl25},
      {02577, false, &expl52, &expl25},  // These are specific to "1x2" or "2x1".
      {00052, false, &expl52, &expl25},
      {07725, false, &expl25, &expl52},
      {05200, false, &expl25, &expl52},
      {0}};

   uint32 directions;
   uint32 dblbitlivemask;
   const Nx1_checker *getin_search;
   uint32 full_occupation = (uint32) ((1U << ((attr::klimit(ss->kind)+1) << 1)) - 1);

   big_endian_get_directions(ss, directions, dblbitlivemask);

   if (heritflags_to_check == INHERITFLAGMXNK_3X1 ||
       heritflags_to_check == INHERITFLAGMXNK_1X3) {
      if (ss->kind == s2x4) {
         getin_search = Nx1_checktable_2x4;
         goto do_Nx1_search;
      }
      else if (ss->kind == s1x8) {
         getin_search = Nx1_checktable_1x8;
         goto do_Nx1_search;
      }
      else if (ss->kind == s3x4) {
         if (dblbitlivemask == 0x3CF3CF || dblbitlivemask == 0xC3FC3F ||
             dblbitlivemask == 0x33FCCF || dblbitlivemask == 0xCCF33F) return true;
      }
      else if (ss->kind == s2x6) {
         if (dblbitlivemask == 0x33F33F || dblbitlivemask == 0xFCCFCC ||
             dblbitlivemask == 0x30CFFF || dblbitlivemask == 0xFFF30C) return true;
      }
      else if (ss->kind == s_spindle) {
         if (dblbitlivemask == 0xFFFF) {
            expand::expand_setup(expsp3, ss);
            return true;
         }
      }
      else if (ss->kind == s3dmd) {
         if (dblbitlivemask == 0xFC3FC3) {
            expand::expand_setup(exp3d3, ss);
            return true;
         }
      }
      else if (ss->kind == s_323) {
         if (dblbitlivemask == 0xFFFF) {
            expand::expand_setup(exp323, ss);
            return true;
         }
      }
      else if (ss->kind == s2x3) {
         if (dblbitlivemask == 0x33F || dblbitlivemask == 0xFCC) return true;
      }
   }
   else if (heritflags_to_check == INHERITFLAGMXNK_2X1 ||
            heritflags_to_check == INHERITFLAGMXNK_1X2) {
      if (ss->kind == s2x3) {
         getin_search = Nx1_checktable_2x3;
         goto do_Nx1_search;
      }
      else if (ss->kind == s1x6) {
         getin_search = Nx1_checktable_1x6;
         goto do_Nx1_search;
      }
      else if (ss->kind == s_bone6) {
         if (directions == 01240 || directions == 04012 ||
             directions == 01042 || directions == 04210 ||
             directions == 03765 || directions == 06537 ||
             directions == 03567 || directions == 06735) {
            expand::expand_setup(expb51, ss);
            return true;
         }
      }
      else if (ss->kind == s_short6) {
         if (directions == 00052 || directions == 05200 ||
             directions == 07725 || directions == 02577 ||
             directions == 03567 || directions == 06735 ||
             directions == 01042 || directions == 04210) {
            expand::expand_setup(expb26, ss);
            return true;
         }
      }
   }
   else if (heritflags_to_check == INHERITFLAGMXNK_4X0 ||
            heritflags_to_check == INHERITFLAGMXNK_0X4) {
      if (ss->kind == s2x4) {
         getin_search = Nx0_checktable_2x4;
         goto do_Nx1_search;
      }
   }
   else if (heritflags_to_check == INHERITFLAGMXNK_3X0 ||
            heritflags_to_check == INHERITFLAGMXNK_0X3) {
      if (ss->kind == s2x3) {
         getin_search = Nx0_checktable_2x3;
         goto do_Nx1_search;
      }
   }

   return false;

 do_Nx1_search:

   // Search the table for a unique getin map.
   // *** We used to check only the direction bits that were masked on by dblbitlivemask,
   // and demand that the result be unique.  (That's what the "if (final) return false"
   // is about.)  We also used to try not to demand that dblbitlivemask have all bits
   // on.  But that really doesn't work.  We can't search for hard Nx1 and 1xN
   // formulations in the presence of phantoms.

   const expand::thing *highquality = (const expand::thing *) 0;
   const expand::thing *lowquality = (const expand::thing *) 0;
   bool lowqualityisambiguous = false;

   for ( ; getin_search->directions ; getin_search++) {
      if (((getin_search->directions ^ directions) & dblbitlivemask) == 0) {

         // Unsymmetrical maps require full occupation.
         if (getin_search->unsymm && dblbitlivemask != full_occupation)
            continue;

         if (getin_search->action_if_Nx1 != getin_search->action_if_1xN) {
            // This is a high quality map.  It can tell what to do without requiring lots of people.
            if (highquality) return false;    // If high quality maps are ambiguous, we lose.
            highquality = (heritflags_to_check == INHERITFLAGMXNK_3X1 ||
                           heritflags_to_check == INHERITFLAGMXNK_4X0 ||
                           heritflags_to_check == INHERITFLAGMXNK_3X0 ||
                           heritflags_to_check == INHERITFLAGMXNK_2X1) ?
               getin_search->action_if_Nx1 :
               getin_search->action_if_1xN;
         }
         else {
            // This is a low quality map.  Ambiguity is OK, up to two, if a high quality map is present.
            if (lowquality) lowqualityisambiguous = true;
            lowquality = getin_search->action_if_Nx1;
         }
      }
   }

   if (!highquality) {
      // No high quality map; must use an unambiguous low quality one.
      if (lowqualityisambiguous)
         return false;
      highquality = lowquality;
   }

   if (highquality) {
      expand::expand_setup(*highquality, ss);
      return true;
   }

   return false;
}


extern bool divide_for_magic(
   setup *ss,
   uint32 heritflags_to_check,
   setup *result) THROW_DECL
{
   warning_info saved_warnings;
   uint32 division_code;

   uint32 heritflags_to_use = ss->cmd.cmd_final_flags.herit;

   // If "magic" was specified, we have to be sure the level is
   // high enough (e.g. C3B).  At C1, only real magic columns,
   // and whatever calls have something explicit in the database,
   // are permitted.

   if (heritflags_to_check & INHERITFLAG_MAGIC) {
      bool booljunk;
      assumption_thing tt;

      tt.assumption = cr_magic_only;
      tt.assump_col = 1;
      tt.assump_both = 0;
      tt.assump_negate = 0;
      tt.assump_live = 0;
      tt.assump_cast = 0;
      if (ss->kind != s2x4 || verify_restriction(ss, tt, false, &booljunk) != restriction_passes) {
         if (calling_level < general_magic_level)
            warn_about_concept_level();
      }
   }

   switch (ss->kind) {
   case s2x4:
      if (heritflags_to_check == INHERITFLAG_MAGIC) {
         // "Magic" was specified.  Split it into 1x4's
         // in the appropriate magical way.
         division_code = MAPCODE(s1x4,2,MPKIND__MAGIC,1);
         goto divide_us;
      }
      break;
   case s_qtag:
      // Indicate that we have done a diamond division
      // and the concept name needs to be changed.
      ss->cmd.cmd_misc3_flags |= CMD_MISC3__NEED_DIAMOND;

      if (heritflags_to_check == INHERITFLAG_MAGIC) {
         division_code = MAPCODE(sdmd,2,MPKIND__MAGIC,1);
         goto divide_us;
      }
      else if (heritflags_to_check == INHERITFLAG_INTLK) {
         division_code = MAPCODE(sdmd,2,MPKIND__INTLKDMD,1);
         goto divide_us;
      }
      else if (heritflags_to_check == (INHERITFLAG_MAGIC | INHERITFLAG_INTLK)) {
         division_code = MAPCODE(sdmd,2,MPKIND__MAGICINTLKDMD,1);
         goto divide_us;
      }
      else if (heritflags_to_check == INHERITFLAGMXNK_3X1 ||
               heritflags_to_check == INHERITFLAGMXNK_1X3) {
         expand::expand_setup(s_qtg_3x4, ss);
         goto do_3x3;
      }
      break;
   case s_ptpd:
      ss->cmd.cmd_misc3_flags |= CMD_MISC3__NEED_DIAMOND;

      if (heritflags_to_check == INHERITFLAG_MAGIC) {
         division_code = spcmap_ptp_magic;
         goto divide_us;
      }
      else if (heritflags_to_check == INHERITFLAG_INTLK) {
         division_code = spcmap_ptp_intlk;
         goto divide_us;
      }
      else if (heritflags_to_check == (INHERITFLAG_MAGIC | INHERITFLAG_INTLK)) {
         division_code = spcmap_ptp_magic_intlk;
         goto divide_us;
      }
      break;
   }

   // Now check for 1x3 types of stuff.

   if (heritflags_to_check == INHERITFLAGMXNK_3X1 ||
       heritflags_to_check == INHERITFLAGMXNK_1X3 ||
       heritflags_to_check == INHERITFLAGMXNK_3X0 ||
       heritflags_to_check == INHERITFLAGMXNK_0X3 ||
       heritflags_to_check == INHERITFLAGMXNK_4X0 ||
       heritflags_to_check == INHERITFLAGMXNK_0X4 ||
       heritflags_to_check == INHERITFLAGMXNK_2X1 ||
       heritflags_to_check == INHERITFLAGMXNK_1X2) {

      // If we have already expanded, don't do it again.
      if (ss->cmd.cmd_heritflags_to_save_from_mxn_expansion != heritflags_to_check) {
         if (do_1x3_type_expansion(ss, heritflags_to_check))
            goto do_3x3;
      }
      else {
         goto do_3x3;
      }
   }

   return false;

 divide_us:

   ss->cmd.cmd_final_flags.herit = (heritflags) (heritflags_to_use & ~heritflags_to_check);
   divided_setup_move(ss, division_code, phantest_ok, true, result);
   return true;

 do_3x3:

   bool sixteen = (heritflags_to_use == INHERITFLAGMXNK_0X4 || heritflags_to_use == INHERITFLAGMXNK_4X0);

   ss->cmd.cmd_final_flags.herit = (heritflags)
      ((heritflags_to_use & ~(INHERITFLAG_MXNMASK|INHERITFLAG_NXNMASK)) |
       (sixteen ? INHERITFLAGNXNK_4X4 : INHERITFLAGNXNK_3X3));

   if (attr::slimit(ss) > 7)
      ss->cmd.cmd_final_flags.set_heritbit(sixteen ? INHERITFLAG_16_MATRIX : INHERITFLAG_12_MATRIX);

   saved_warnings = configuration::save_warnings();
   impose_assumption_and_move(ss, result);
   result->result_flags.misc |= RESULTFLAG__DID_MXN_EXPANSION;
   result->result_flags.res_heritflags_to_save_from_mxn_expansion =
      heritflags_to_use & (INHERITFLAG_MXNMASK|INHERITFLAG_NXNMASK);

   // Shut off "each 2x3" types of warnings -- they will arise spuriously
   // while the people do the calls in isolation.

   configuration::clear_multiple_warnings(dyp_each_warnings);
   configuration::set_multiple_warnings(saved_warnings);
   return true;
}


extern bool do_simple_split(
   setup *ss,
   split_command_kind split_command,
   setup *result) THROW_DECL
{
   uint32 mapcode;
   bool recompute_id = true;

   ss->cmd.cmd_misc_flags &= ~CMD_MISC__MUST_SPLIT_MASK;

   switch (ss->kind) {
   case s3x4:
      if (split_command == split_command_2x3) {
         mapcode = MAPCODE(s2x3,2,MPKIND__SPLIT,1);
         break;
      }
      else if (split_command == split_command_none) {
         if (ss->people[0].id1 & ss->people[1].id1 &
             ss->people[4].id1 & ss->people[5].id1 &
             ss->people[6].id1 & ss->people[7].id1 &
             ss->people[10].id1 & ss->people[11].id1) {
            mapcode = MAPCODE(s2x2,2,MPKIND__OFFS_R_HALF,0);
            break;
         }
         else if (ss->people[2].id1 & ss->people[3].id1 &
                  ss->people[4].id1 & ss->people[5].id1 &
                  ss->people[8].id1 & ss->people[9].id1 &
                  ss->people[10].id1 & ss->people[11].id1) {
            mapcode = MAPCODE(s2x2,2,MPKIND__OFFS_L_HALF,0);
            break;
         }
      }

      return true;
   case s2x4:
      mapcode = (split_command == split_command_none) ?
         MAPCODE(s2x2,2,MPKIND__SPLIT,0) : MAPCODE(s1x4,2,MPKIND__SPLIT,1);
      break;
   case s2x6:
      if (split_command == split_command_2x3) {
         mapcode = MAPCODE(s2x3,2,MPKIND__SPLIT,0);
         break;
      }
      else if (split_command == split_command_1x4) {
         if (ss->cmd.cmd_heritflags_to_save_from_mxn_expansion == INHERITFLAGMXNK_3X1 ||
             ss->cmd.cmd_heritflags_to_save_from_mxn_expansion == INHERITFLAGMXNK_1X3 ||
             ss->cmd.cmd_heritflags_to_save_from_mxn_expansion == INHERITFLAGMXNK_2X1 ||
             ss->cmd.cmd_heritflags_to_save_from_mxn_expansion == INHERITFLAGMXNK_1X2) {
            mapcode = MAPCODE(s1x6,2,MPKIND__SPLIT,1);
            break;
         }
         else if (ss->people[0].id1 & ss->people[1].id1 &
                  ss->people[2].id1 & ss->people[3].id1 &
                  ss->people[6].id1 & ss->people[7].id1 &
                  ss->people[8].id1 & ss->people[9].id1) {
            mapcode = MAPCODE(s1x4,2,MPKIND__OFFS_L_HALF,1);
            break;
         }
         else if (ss->people[2].id1 & ss->people[3].id1 &
                  ss->people[4].id1 & ss->people[5].id1 &
                  ss->people[8].id1 & ss->people[9].id1 &
                  ss->people[10].id1 & ss->people[11].id1) {
            mapcode = MAPCODE(s1x4,2,MPKIND__OFFS_R_HALF,1);
            break;
         }
      }

      fail("Can't figure out how to split this call.");
   case s2x8:
      mapcode = MAPCODE(s2x4,2,MPKIND__SPLIT,0);
      break;
   case s1x8:
      mapcode = MAPCODE(s1x4,2,MPKIND__SPLIT,0);
      if (split_command == split_command_1x8) recompute_id = false;
      break;
   case s4x4:
      {
         mapcode = MAPCODE(s2x2,4,MPKIND__SPLIT_OTHERWAY_TOO,0);
         /*
         uint32 mask = little_endian_live_mask(ss);

         if (mask == 0xB4B4)
            mapcode = MAPCODE(s2x4,1,MPKIND__OFFS_L_FULL,0);
         else if (mask == 0x4B4B)
            mapcode = MAPCODE(s2x4,1,MPKIND__OFFS_R_FULL,0);
         else
            return true;

         ss->cmd.cmd_misc_flags |= CMD_MISC__MUST_SPLIT_VERT;
         */
      }
      break;
   case s_qtag:
      mapcode = MAPCODE(sdmd,2,MPKIND__SPLIT,1);
      break;
   case s_ptpd:
      if (split_command == split_command_1x8) recompute_id = false;
      mapcode = MAPCODE(sdmd,2,MPKIND__SPLIT,0);
      break;
   default:
      return true;
   }

   divided_setup_move(ss, mapcode, phantest_ok, recompute_id, result);
   return false;
}


/* Do a call as part of a series.  The result overwrites the incoming setup.

   This handles the task of "turning around" the setup from the result left by
   one call to "move" to the input required by the next call.

   The result_flags are carried from one part to another through the "result_flags"
   word of the setup, even though (or especially because) that word is undefined
   prior to doing a call.  The client must seed that word with a huge "split_info"
   field (plus whatever "misc" bits are desired) prior to the beginning of the series, and not
   clobber it between parts.  At the end of the series, it will contain all the required
   result flags for the whole operation, including the final elongation.

   The elongation bits of the setup (in the "cmd.prior_elongation_bits" field)
   must be correct at the start of the series, and must preserved from one call
   to the next.  Since the entire "cmd" field of the setup typically gets changed
   for each call in the series (in order to do different calls) this will typically
   require specific action by the client. */

// This returns the "force" flags that it extracted from the call.

extern uint32 do_call_in_series(
   setup *sss,
   bool dont_enforce_consistent_split,
   bool normalize,
   bool qtfudged) THROW_DECL
{
   uint32 retval = 0;
   uint32 current_elongation = 0;
   resultflag_rec saved_result_flags = sss->result_flags;

   // Start the expiration mechanism, but only if we are really doing a call.
   if (sss->cmd.callspec)
      sss->cmd.prior_expire_bits |= sss->result_flags.misc & RESULTFLAG__EXPIRATION_BITS;

   setup qqqq = *sss;

   // Check for a concept that will need to be re-evaluated under "twice".
   // The test for this is [waves] initially twice initially once removed
   // hinge the lock.  We want the "once removed" to be re-evaluated.

   if (qqqq.cmd.cmd_misc3_flags & CMD_MISC3__RESTRAIN_CRAZINESS &&
       (qqqq.cmd.cmd_fraction.flags & CMD_FRAC_CODE_MASK) == CMD_FRAC_CODE_ONLY) {
      if (qqqq.cmd.restrained_concept->concept->kind == concept_n_times_const) {
         qqqq.cmd.cmd_misc3_flags &= ~CMD_MISC3__RESTRAIN_CRAZINESS;
         qqqq.cmd.restrained_fraction = qqqq.cmd.cmd_fraction;
         qqqq.cmd.cmd_fraction.set_to_null();

      }
   }

   setup tempsetup;

   // If we are forcing a split, and an earlier call in the series has responded
   // to that split by returning an unequivocal splitting axis (indicated by
   // one field being zero and the other nonzero), we continue to split
   // along the same axis.

    // We want one field nonzero and the other zero.

   if ((qqqq.cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK) &&
       !dont_enforce_consistent_split &&
       (saved_result_flags.split_info[0] | saved_result_flags.split_info[1]) != 0 &&
       (saved_result_flags.split_info[0] * saved_result_flags.split_info[1]) == 0) {
      int prefer_1x4;
      uint32 save_split = qqqq.cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK;

      if (saved_result_flags.split_info[0])
         prefer_1x4 = qqqq.rotation & 1;
      else
         prefer_1x4 = (~qqqq.rotation) & 1;

      if (qqqq.kind == s4x4) prefer_1x4 = 0;

      if (prefer_1x4 && qqqq.kind != s2x4 && qqqq.kind != s2x6)
         fail("Can't figure out how to split multiple part call.");

      if (do_simple_split(&qqqq,
                          prefer_1x4 ? split_command_1x4 : split_command_none,
                          &tempsetup))
         fail("Can't figure out how to split this multiple part call.");

      qqqq.cmd.cmd_misc_flags |= save_split;  // Put it back in.
   }
   else
      move(&qqqq, qtfudged, &tempsetup, false);

   if (tempsetup.kind == sfudgy2x6l || tempsetup.kind == sfudgy2x6r) {
      tempsetup.kind = s1p5x8;
   }
   else if (tempsetup.kind == sfudgy2x3l || tempsetup.kind == sfudgy2x3r) {
      tempsetup.kind = s1p5x4;
   }

   if (tempsetup.kind == s2x2) {
      switch (sss->kind) {
         case s1x4: case sdmd: case s2x2:
            current_elongation = tempsetup.result_flags.misc & 3;

            /* If just the ends were doing this, and it had some
               "force_lines" type of directive, honor same. */

            if (qqqq.cmd.cmd_misc3_flags & CMD_MISC3__DOING_ENDS) {
               if (sss->kind == s2x2 &&
                   (qqqq.cmd.cmd_misc_flags &
                    (DFM1_CONC_DEMAND_LINES | DFM1_CONC_DEMAND_COLUMNS))) {

                  int i;
                  uint32 tb = 0;

                  for (i=0; i<4; i++) tb |= sss->people[i].id1;
                  if ((tb & 011) == 011) fail("Can't figure out where people started.");

                  if (qqqq.cmd.cmd_misc_flags & DFM1_CONC_DEMAND_LINES)
                     tb++;

                  if (sss->cmd.prior_elongation_bits & ((tb & 1) + 1))
                     fail("Ends aren't starting in required position.");
               }

               if (qqqq.cmd.cmd_misc_flags & (DFM1_CONC_FORCE_LINES | DFM1_CONC_FORCE_COLUMNS)) {
                  int i;
                  uint32 tb = 0;

                  for (i=0; i<4; i++) tb |= tempsetup.people[i].id1;
                  if ((tb & 011) == 011) fail("Can't figure out where people finish.");

                  if (qqqq.cmd.cmd_misc_flags & DFM1_CONC_FORCE_COLUMNS)
                     tb++;

                  current_elongation = (tb & 1) + 1;
               }
               else if (qqqq.cmd.cmd_misc_flags & (DFM1_CONC_FORCE_OTHERWAY)) {
                  if ((sss->cmd.prior_elongation_bits+1) & 2)
                     current_elongation = (sss->cmd.prior_elongation_bits & 3) ^ 3;
               }
               else if (qqqq.cmd.cmd_misc_flags & (DFM1_CONC_FORCE_SPOTS)) {
                  if ((sss->cmd.prior_elongation_bits+1) & 2)
                     current_elongation = sss->cmd.prior_elongation_bits & 3;
               }

               retval = qqqq.cmd.cmd_misc_flags;
               qqqq.cmd.cmd_misc_flags &= ~(DFM1_CONC_FORCE_SPOTS | DFM1_CONC_FORCE_OTHERWAY |
                                            DFM1_CONC_FORCE_LINES | DFM1_CONC_FORCE_COLUMNS |
                                            DFM1_CONC_DEMAND_LINES | DFM1_CONC_DEMAND_COLUMNS);
            }

            break;

         /* Otherwise (perhaps the setup was a star) we have no idea how to elongate the setup. */
         default:
            break;
      }
   }

   uint32 save_expire = sss->cmd.prior_expire_bits;
   *sss = tempsetup;
   sss->cmd.prior_expire_bits = save_expire;
   sss->cmd.cmd_misc_flags = qqqq.cmd.cmd_misc_flags;   // But pick these up from the call.
   sss->cmd.cmd_misc_flags &= ~CMD_MISC__DISTORTED;     // But not this one!

   /* Remove outboard phantoms.
      It used to be that normalize_setup was not called
      here.  It was found that we couldn't do things like, from a suitable offset wave,
      [triple line 1/2 flip] back to a wave, that is, start offset and finish normally.
      So this has been added.  However, there may have been a reason for not normalizing.
      If any problems are found, it may be that a flag needs to be added to seqdef calls
      saying whether to remove outboard phantoms after each part. */

   /* But we *DON'T* remove the phantoms if "explicit matrix" is on, as of version 29.45.
      This is to make "1x12 matrix grand swing thru" work from a simple grand wave.
      The intention is to add outboard phantoms and then do the call in a gigantic way.
      We added two outboard phantoms on each end, then did the swing half, resulting
      in another 1x12 with only the center 8 spots occupied.  If outboard phantoms are
      removed, the effect of the explicit "1x12 matrix" modifier is lost.  The "grand
      1/2 by the left" is done on the 1x8.  So we leave the phantoms in.  Actually,
      making this call operate properly required a database change as well.  We had to
      add 1x12 and 1x16 starting setups for "1/2 by the left or right".  Otherwise,
      on "1x12 matrix grand swing thru", it would just do it in each 1x6.  Also, we
      don't want the warning "Do the call in each 1x6" to be given, though it is
      admittedly better to have the program warn us when it's about to do something
      wrong than to do the wrong thing silently.
      We also don't normalize if a "12 matrix" or "16 matrix" modifier is given, so that
      "12 matrix grand swing thru" will work also, along with "1x12 matrix grand swing thru".
      Only "1x12 matrix" turns on CMD_MISC__EXPLICIT_MATRIX.  Plain "12 matrix will appear
      in the "new_final_concepts" word. */

   if (normalize) normalize_setup(sss, plain_normalize, false);

   /* To be safe, we should take away the "did last part" bit for the second call,
      but we are fairly sure it won't be on. */

   sss->cmd.prior_elongation_bits = current_elongation;

   /* The computation of the new result flags is complex, since we are trying
      to accumulate results over a series of calls.  To that end, the incoming contents
      of the "result_flags" word are used.   (Normally, this word is undefined
      on input to "move", but "do_call_in_series" takes it is the accumulated
      stuff so far.  It follows that it must be "seeded" at the start of the series.)

      The manipulations that produce the final value differ among the various bits and fields:

         *  The RESULTFLAG__PART_COMPLETION_BITS bits:
            Set to the result of the part we just did -- discard incoming values.

         *  The "split_info" field:
            Set to the AND of the incoming values and what we just got --
            this has the effect of correctly keeping track of the splitting through all
            parts of the call.  Actually, it's a little more complicated than this,
            and we let minimize_splitting_info do it.

         *  The low 2 bits, which have the elongation:
            Set to "current_elongation", which typically has the incoming stuff,
            but it actually a little bit subtle.  Put this in the
            "cmd.prior_elongation_bits" field also.

         *  The other bits:
            Set to the OR of the incoming values and what we just got --
            this is believed to be the correct way to accumulate these bits.

      It follows from this that the correct way to "seed" the result_flags word
      at the start of a series is by initializing it to a huge split_info field. */

   sss->result_flags = saved_result_flags;

   sss->result_flags.misc &= ~RESULTFLAG__PART_COMPLETION_BITS;
   sss->result_flags.misc |= tempsetup.result_flags.misc;

   // Here is where we implement the policy that the internal boundaries between subcalls
   // are not checked for overcast.  But there is an exception:  If a subcall is marked
   // "roll transparent", the boundary between it and the following call is checked.
   // This is to handle the various transparent "check a wave" types of things that are
   // used as the first subcall.  Whatever follows them is considered the first subcall,
   // and is subject to checking.

   if (!(qqqq.cmd.cmd_misc3_flags & (CMD_MISC3__ROLL_TRANSP|CMD_MISC3__ROLL_TRANSP_IF_Z)))
      sss->result_flags.misc |= RESULTFLAG__STOP_OVERCAST_CHECK;

   sss->result_flags.misc &= ~3;

   sss->cmd.cmd_heritflags_to_save_from_mxn_expansion = tempsetup.result_flags.res_heritflags_to_save_from_mxn_expansion;
   sss->result_flags.misc |= current_elongation;
   sss->result_flags.copy_split_info(tempsetup.result_flags);

   canonicalize_rotation(sss);
   // If doing the special counter rotate 3/8 stuff, the result has all the information it needs.
   if (qqqq.kind != s4x4 || qqqq.eighth_rotation == 0 || sss->kind != s2x4 || sss->eighth_rotation != 0)
      minimize_splitting_info(sss, saved_result_flags);

   return retval;
}

// For each person, we remember the 3 best candidates with whom that person might Jaywalk.
enum { NUMBER_OF_JAYWALK_CANDIDATES = 4,
       NUMBER_OF_JAYWALK_VERT_CAND = 3 };

struct jaywalkpersonstruct {
   int jp;           // Person with whom to Jay Walk.
   int jp_lateral;
   int jp_vertical;
   int jp_eucdist;   // Jaywalk Euclidean distance.
};

struct matrix_rec {
   int x;              // This person's coordinates, calibrated so that a matrix
   int y;              //   position cooresponds to an increase by 4.
   int nicex;          // This person's "nice" coordinates, used for
   int nicey;          //   calculating jay walk legality.
   uint32 id1;         // The actual person, for error printing.
   bool sel;           // True if this person is selected.  (False if selectors not in use.)
   bool done;          // Used for loop control on each pass.
   bool realdone;      // Used for loop control on each pass.
   bool mirror_this_op;// On when a "press in" type of thing requires mirroring.
   // This list is sorted from best (smallest jp_eucdist) to worst.
   jaywalkpersonstruct jpersons[NUMBER_OF_JAYWALK_CANDIDATES];
   unsigned int jaywalktargetmask;   // Bits for jaywalkers point back at this person.
   int boybit;         // 1 if boy, 0 if not (might be neither).
   int girlbit;        // 1 if girl, 0 if not (might be neither).
   int dir;            // This person's initial facing direction, 0 to 3.
   int deltax;         // How this person will move, relative to his own facing
   int deltay;         //   direction, when call is finally executed.
   int nearestdrag;    // Something having to do with "drag".
   int deltarot;       // How this person will turn.
   uint32 roll_stability_info; // This person's slide, roll, & stability info, from call def'n.
   int orig_source_idx;
   matrix_rec *nextse; // Points to next person south (dir even) or east (dir odd.)
   matrix_rec *nextnw; // Points to next person north (dir even) or west (dir odd.)
   bool far_squeezer;  // This person's pairing is due to being far from someone.
   bool tbstopse;      // True if nextse/nextnw is zero because the next spot
   bool tbstopnw;      //   is occupied by a T-boned person (as opposed to being empty.)
};



static int start_matrix_call(
   const setup *ss,
   matrix_rec matrix_info[],
   int base,
   uint32 flags,
   setup *people)
{
   int i;
   int nump = base;

   if (base == 0) {
      *people = *ss;    /* Get the setup kind, so selectp will be happier. */
      people->clear_people();
   }

   const coordrec *nicethingyptr = setup_attrs[ss->kind].nice_setup_coords;
   const coordrec *thingyptr = setup_attrs[ss->kind].setup_coords;

   if (flags & (MTX_FIND_SQUEEZERS|MTX_FIND_SPREADERS)) {
      thingyptr = nicethingyptr;
      // Fix up a galaxy or hourglass so that points can squeeze.
      // They have funny coordinates so that they can't truck or loop.
      if (ss->kind == s_hrglass) thingyptr = &squeezethingglass;
      else if (ss->kind == s_galaxy) thingyptr = &squeezethinggal;
      else if (ss->kind == s_343) thingyptr = &squeezething343;
      else if (ss->kind == s_qtag) thingyptr = &squeezethingqtag;
      else if (ss->kind == s4dmd) thingyptr = &squeezething4dmd;
   }

   // Third clause in this is actually superfluous.
   if (!thingyptr || !nicethingyptr || attr::slimit(ss) < 0)
      fail("Can't do this in this setup.");

   for (i=0; i<=attr::slimit(ss); i++) {
      if (ss->people[i].id1) {
         if (nump == 8) fail("?????too many people????");
         copy_person(people, nump, ss, i);

         matrix_info[nump].nicex = nicethingyptr->xca[i];
         matrix_info[nump].nicey = nicethingyptr->yca[i];
         matrix_info[nump].x = thingyptr->xca[i];
         matrix_info[nump].y = thingyptr->yca[i];

         matrix_info[nump].done = false;
         matrix_info[nump].realdone = false;
         matrix_info[nump].mirror_this_op = false;

         if (flags & MTX_USE_SELECTOR)
            matrix_info[nump].sel = selectp(ss, i);
         else
            matrix_info[nump].sel = false;

         matrix_info[nump].id1 = people->people[nump].id1;
         matrix_info[nump].dir = people->people[nump].id1 & 3;

         if (flags & MTX_USE_VEER_DATA) {
            uint32 rollbits = people->people[nump].id1 & ROLL_DIRMASK;
            matrix_info[nump].girlbit = (rollbits == ROLL_IS_L) ? 1 : 0;
            matrix_info[nump].boybit = (rollbits == ROLL_IS_R) ? 1 : 0;
         }
         else {
            matrix_info[nump].girlbit = (people->people[nump].id3 & ID3_PERM_GIRL) ? 1 : 0;
            matrix_info[nump].boybit = (people->people[nump].id3 & ID3_PERM_BOY) ? 1 : 0;
         }

         matrix_info[nump].nextse = 0;
         matrix_info[nump].nextnw = 0;
         matrix_info[nump].deltax = 0;
         matrix_info[nump].deltay = 0;
         matrix_info[nump].nearestdrag = 100000;
         matrix_info[nump].deltarot = 0;
         // Set it to "roll is 'M'", no slide, and "stability is 'Z'".
         // "Z" is set with "stb_none" and "REV".
         matrix_info[nump].roll_stability_info =
            (DBSLIDEROLL_BIT * 3) | ((STB_NONE|STB_REVERSE) * DBSTAB_BIT);
         matrix_info[nump].orig_source_idx = i;
         matrix_info[nump].tbstopse = false;
         matrix_info[nump].tbstopnw = false;
         matrix_info[nump].far_squeezer = false;

         for (int i = 0 ; i < NUMBER_OF_JAYWALK_CANDIDATES ; i++) {
            matrix_info[nump].jpersons[i].jp = -1;
            matrix_info[nump].jpersons[i].jp_eucdist = 100000;
         }

         nump++;
      }
   }

   return nump;
}


struct checkitem {
   uint32 ypar;
   uint32 sigcheck;
   setup_kind new_setup;
   uint32 mask;
   uint32 new_rot;     // 0x100 bit means this fudging is severe.
   warning_index warning;
   const coordrec *new_checkptr;
   veryshort fixer[33];
};


static const checkitem checktable[] = {
   {0x00620046, 0x10808404, s_3223,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A60026, 0x08080104, s_nxtrglcw,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A60026, 0x0C008002, s_nxtrglccw, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00630095, 0x00840050, spgdmdcw,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00630095, 0x10800A00, spgdmdccw, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A20026, 0x08008404, s_rigger, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00770077, 0x00418004, s_galaxy, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   // Fudge this to a galaxy.  The center 2 did a squeeze or spread from a spindle.
   {0x00840066, 0x0C000108, s_galaxy, UINT32_C(~0), 0, warn__check_galaxy, (const coordrec *) 0,
    {0, 6, 0, 7,       0, -6, 0, -7,     8, 0, 7, 0,       -8, 0, -7, 0,
     4, 2, 2, 2,       4, -2, 2, -2,     -4, 2, -2, 2,     -4, -2, -2, -2}},
   // Fudge this to quadruple diamonds.  Someone trucked out from a wqtag.
   {0x00E70055, 0x0900A422, s4dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-4, 5, -5, 5, 4, -5, 5, -5, 127}},
   // Next two: The centers did a 1/2 truck from point-to-point diamonds.  Fudge to a 3x6.
   {0x00930044, 0x01018800, s3x6, UINT32_C(0522522), 0, warn__none, (const coordrec *) 0,
    {-9, 0, -10, 0,    9, 0, 10, 0,      -5, 0, -6, 0,
     -5, -4, -6, -4,   -5, 4, -6, 4,     5, -4, 6, -4,     5, 4, 6, 4}},
   {0x00930044, 0x21018800, s3x6, UINT32_C(0722722), 0, warn__none, (const coordrec *) 0,
    {-9, 0, -10, 0,    9, 0, 10, 0,      -5, 0, -6, 0,     5, 0, 6, 0,
     -5, -4, -6, -4,   -5, 4, -6, 4,     5, -4, 6, -4,     5, 4, 6, 4}},

   // The points did a 1/2 press ahead from triple diamonds.  Fudge to a qtag.
   {0x00630055, 0x01420421, s_qtag, 0, 0, warn__none, (const coordrec *) 0,
    {-2, 5, -4, 5,     -5, 5, -4, 5,     2, 5, 5, 5,
     -2, -5, -5, -5,   2, -5, 4, -5,     5, -5, 4, -5}},

   // The centers did a 1/2 truck from a 3x6.  Fudge to point-to-point diamonds.
   {0x00A20026, 0x09080002, nothing, UINT32_C(~0), 0, warn__none, &truck_to_ptpd, {127}},

   // Fudge to 2x4.  People pressed ahead from an alamo ring.
   {0x00530023, 0x00102090, s2x4,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-5, 2, -6, 2,     -2, 1, -2, 2,     2, 1, 2, 2,       5, 2, 6, 2,
     5, -2, 6, -2,     2, -1, 2, -2,     -2, -1, -2, -2,   -5, -2, -6, -2}},

   // OK to here (13).  This has something to do with "initialize_matrix_position_tables".
   // Not sure what that was about.

   {0x00230053, 0x00220009, s2x4,  UINT32_C(~0), 1, warn__none, (const coordrec *) 0,
    {2, -5, 2, -6,      1, -2, 2, -2,
     1, 2, 2, 2,        2, 5, 2, 6,
     -2, 5, -2, 6,      -1, 2, -2, 2,
     -1, -2, -2, -2,    -2, -5, -2, -6}},

   {0x00770073, 0x0001A005, sdeep2x1dmd,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {7, 2, 6, 2,        7, -2, 6, -2,
     -7, 2, -6, 2,      -7, -2, -6, -2, 127}},

   {0x00730077, 0x00408304, sdeep2x1dmd,  UINT32_C(~0), 1, warn__none, (const coordrec *) 0,
    {2, 7, 2, 6,        2, -7, 2, -6,
     -2, 7, -2, 6,      -2, -7, -2, -6, 127}},

   {0x00620073, 0x00088306, s4x4,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {2, 7, 2, 6,        2, -7, 2, -6,
     -2, 7, -2, 6,      -2, -7, -2, -6, 127}},

   {0x00730073, 0x0000A305, s4x4,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {2, 7, 2, 6,        2, -7, 2, -6,
     -2, 7, -2, 6,      -2, -7, -2, -6,
     7, 2, 6, 2,        7, -2, 6, -2,
     -7, 2, -6, 2,      -7, -2, -6, -2}},

   // 1/2 press ahead from LH star promenade.
   {0x00930093, 0x41020080, s_c1phan, UINT32_C(~0), 0, warn__check_c1_phan, (const coordrec *) 0,
    { -2, 9, -3, 13,    -2, 5, -3, 9,
      9, 2, 15, 4,      5, 2, 11, 4,
      2, -9, 4, -14,    2, -5, 4, -10,
      -9, -2, -14, -7,  -5, -2, -10, -7}},
   // 1/2 press ahead from RH star promenade.  Have to list them separately, because can only have 8 specifiers.
   {0x00930093, 0x00140041, s_c1phan, UINT32_C(~0), 0, warn__check_c1_phan, (const coordrec *) 0,
    { -9, 2, -6, 4,     -5, 2, -2, 4,
      2, 9, 4, 7,       2, 5, 4, 3,
      9, -2, 7, -5,     5, -2, 3, -5,
      -2, -9, -5, -6,   -2, -5, -5, -2}},
   // Full press ahead from LH star promenade.  Goes to same setup as 1/2 press ahead.
   {0x00950095, 0x00810404, s_c1phan, UINT32_C(~0), 0, warn__check_c1_phan, (const coordrec *) 0,
    { -4, 9, -3, 13,    -4, 5, -3, 9,      9, 4, 15, 4,      5, 4, 11, 4,
      4, -9, 4, -14,    4, -5, 4, -10,     -9, -4, -14, -7,  -5, -4, -10, -7}},
   // As above, RH.
   {0x00950095, 0x11000800, s_c1phan, UINT32_C(~0), 0, warn__check_c1_phan, (const coordrec *) 0,
    { -9, 4, -6, 4,     -5, 4, -2, 4,
      4, 9, 4, 7,       4, 5, 4, 3,
      9, -4, 7, -5,     5, -4, 3, -5,
      -4, -9, -5, -6,   -4, -5, -5, -2}},

   // Next 2 items: fudge to a galaxy.  The points got here by pressing and trucking.
   // Don't need to tell them to check a galaxy -- it's pretty obvious.
   {0x00660066, 0x08008404, s_galaxy, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {0, 6, 0, 7, 0, -6, 0, -7, 6, 0, 7, 0, -6, 0, -7, 0, 127}},
   {0x00660077, 0x00018404, s_galaxy, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {0, 6, 0, 7, 0, -6, 0, -7, 6, 0, 7, 0, -6, 0, -7, 0, 127}},
   {0x00660066, 0x0C000500, s_galaxy, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {0, 6, 0, 7,    0, -6, 0, -7,    6, 0, 7, 0,      -6, 0, -7, 0,
     4, 2, 2, 2,    4, -2, 2, -2,    -4, 2, -2, 2,    -4, -2, -2, -2, 127}},
   // Next 2 items: the trailing points pressed ahead from quadruple diamonds,
   // so that only the centers 2 diamonds are now occupied.  Fudge to diamonds.
   {0x00730055, 0x01008420, nothing, UINT32_C(~0), 0, warn__check_dmd_qtag, &press_4dmd_qtag1, {127}},
   {0x00710051, 0x01008420, nothing, UINT32_C(~0), 0, warn__check_dmd_qtag, &press_4dmd_qtag1, {127}},
   // Next 2 items: same, other way.
   {0x00730055, 0x21080400, nothing, UINT32_C(~0), 0, warn__check_dmd_qtag, &press_4dmd_qtag2, {127}},
   {0x00710051, 0x21080400, nothing, UINT32_C(~0), 0, warn__check_dmd_qtag, &press_4dmd_qtag2, {127}},

   // This must precede the "squeezefinalglass" stuff.
   {0x00620026, 0x01080002, s_bone6, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},

   // People 1/2 pressed ahead from quadruple 3/4 tags.  Fudge to a 4x4.
   {0x00B10071, 0x01806000, nothing, UINT32_C(~0), 0, warn__none, &press_4dmd_4x4, {127}},

   // Next 2 items: six poeple did a squeeze from a galaxy.  Fudge to an hourglass.
   {0x00260062, 0x10100600, nothing, UINT32_C(~0), 0, warn__none, &squeezefinalglass, {127}},
   {0x00660066, 0x10100600, nothing, UINT32_C(~0), 0, warn__none, &squeezefinalglass, {127}},
   // Next 2 items: same, other way.
   {0x00620026, 0x09080002, nothing, UINT32_C(~0), 1, warn__none, &squeezefinalglass, {127}},
   {0x00660066, 0x09080002, nothing, UINT32_C(~0), 1, warn__none, &squeezefinalglass, {127}},

   // Some points did a squeeze from a galaxy.  Fudge to a spindle.
   {0x00660026, 0x00008604, s_spindle, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {6, 0, 8, 0, -6, 0, -8, 0,
     2, 2, 4, 2, 2, -2, 4, -2, -2, 2, -4, 2, -2, -2, -4, -2, 127}},
   // Same, on other orientation.
   {0x00260066, 0x09008004, s_spindle, UINT32_C(~0), 1, warn__none, (const coordrec *) 0,
    {0, 6, 0, 8, 0, -6, 0, -8,
     2, 2, 2, 4, -2, 2, -2, 4, 2, -2, 2, -4, -2, -2, -2, -4, 127}},

   {0x01150026, 0x20048202, s1x4dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00D50026, 0x24009102, s1x4p2dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00D50026, 0x22009022, splinepdmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A60055, 0x09000400, splinedmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00670046, 0x109408C1, slinedmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00660062, 0x1810C244, slinepdmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00460086, 0x00242861, sdmdpdmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00660095, 0x40050031, s_trngl8,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},

   {0x00950062, 0x091002C0, sbigdmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00550062, 0x091002C0, sbigdmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00910022, 0x091002C0, sbigdmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},

   {0x00950095, 0x22008080, s_thar, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00530053, 0x00120081, s_alamo, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},

   // This is a "crosswave" on precise matrix spots.
   {0x00660084, 0x01040420, nothing, UINT32_C(~0), 1, warn__none, &acc_crosswave, {127}},

   {0x00950066, 0x28008200, s_crosswave, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A20026, 0x01040420, s_bone, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   // Pressed in from a bigh.
   {0x00930026, 0x01000440, s_bone, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-9, 2, -10, 2, -9, -2, -10, -2, 9, 2, 10, 2, 9, -2, 10, -2}},
   {0x00260062, 0x08008004, s_short6, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00840026, 0x04000308, s_spindle, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00840046, 0x04210308, sd3x4,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00840044, 0x04210308, sd3x4,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},

   {0x00C40026, 0x06109384, sd4x5,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00C40046, 0x06109384, sd4x5,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00C40086, 0x06109384, sd4x5,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},

   {0x00A200A2, 0x101CC4E6, s_bigblob, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   //   {0x00670055, 0x01000420, s_qtag, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00770055, 0x01400420, s_2stars, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},

   // Inner wing did a tow-truck-like operation from a rigger, for example,
   // after a sets in motion.  Fudge to a 1/4 tag.  This is serious fudging, and marked as such with the "0x100".
   {0x00620026, 0x01008404, s_qtag, UINT32_C(~0), 0x100, warn__none, (const coordrec *) 0,
    {-2, 2, -4, 5, 2, 2, 5, 5, 2, -2, 4, -5, -2, -2, -5, -5, 127}},

   // Similar to the above, but truck was from a deepxwv.
   {0x00620066, 0x11100400, s_qtag, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-2, 6, -4, 5, 2, 6, 5, 5, 2, -6, 4, -5, -2, -6, -5, -5, 127}},

   // Inner people did a 1/2 E.R.A. from a deepxwv.
   {0x00A60066, 0x08000600, s_crosswave, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {10, 0, 9, 0, 6, 0, 5, 0, -10, 0, -9, 0, -6, 0, -5, 0, 127}},

   // Inner wing did a 1/2 tow truck from a crosswave.  We want a thar.
   {0x00750066, 0x08400220, s_thar, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-7, 0, -9, 0, -3, 0, -5, 0, 0, 6, 0, 9, 0, 2, 0, 5,
    7, 0, 9, 0, 3, 0, 5, 0, 0, -6, 0, -9, 0, -2, 0, -5}},

   // Outer person did a 1/2 tow truck from a thar.  We want a crosswave.
   {0x00B50095, 0x02400180, s_crosswave, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-11, 0, -9, 0, -7, 0, -5, 0, 0, 9, 0, 6, 0, 5, 0, 2,
    11, 0, 9, 0, 7, 0, 5, 0, 0, -9, 0, -6, 0, -5, 0, -2}},

   {0x00570067, 0x03100084, s_hsqtag, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00570057, 0x03100084, s_hsqtag, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00750067, 0x28008200, s_dmdlndmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00950067, 0x28008200, s_dmdlndmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00550067, 0x08410200, s_qtag, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   {0x00620046, 0x01080842, sd2x5, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A20046, 0x010C0862, sd2x7, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00660055, 0x01000480, s_2x1dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00950026, 0x20008200, s_1x2dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00D50026, 0x20008202, s1x3dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A30055, 0x09000420, swqtag, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A60055, 0x09000420, swqtag, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A70055, 0x09000420, swqtag, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00950057, 0x20008620, swhrglass, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00660073, 0x00098006, sdeep2x1dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A60055, 0x09000480, s3x1dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A60044, 0x09040400, s_wingedstar, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A30055, 0x29008480, s3dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A70055, 0x29008480, s3dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00770055, 0x29008480, s3dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00730055, 0x29008480, s3dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00E30055, 0x0940A422, s4dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00B30055, 0x0940A422, s4dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00B10051, 0x0940A422, s4dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A30055, 0x0940A422, s4dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00D50057, 0x20008202, s_3mdmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00B50057, 0x20008202, s_3mdmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00B70057, 0x41022480, s_3mptpd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00E70057, 0x41022480, s_3mptpd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00D50066, 0x28048202, sbigx, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01150066, 0x28048202, sbigx, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00D10004, 0x28048202, sbigx, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00910004, 0x28048202, sbigx, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01110004, 0x28048202, sbigx, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A20066, 0x18108404, sdeepxwv, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   // Someone trucked from a deep2x1dmd to a deepxwv.
   {0x006600B3, 0x0008800E, nothing, UINT32_C(~0), 1, warn__none, &truck_to_deepxwv, {127}},
   {0x00F30066, 0x12148904, sbigbigx, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01130066, 0x12148904, sbigbigx, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01330066, 0x12148904, sbigbigx, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01130066, 0x09406600, sbigbigh, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01130026, 0x09406600, sbigbigh, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00550057, 0x20000620, s_hrglass, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   // The checkpointers squeezed or spread from a spindle.  Fudge to an hourglass.
   {0x00840066, 0x00202208, s_hrglass, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-4, 6, -4, 5, 4, 6, 5, 5, 4, -6, 4, -5, -4, -6, -5, -5, 8, 0, 5, 0, -8, 0, -5, 0, 127}},
   // The outsides cast back from a dhrglass.
   {0x00670066, 0x20004240, s_hrglass, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-6, -6, -5, -5, 6, -6, 4, -5, 6, 6, 5, 5, -6, 6, -4, 5, 127}},
   {0x00A70026, 0x20040220, s_dhrglass, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00930026, 0x01108080, s_ptpd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00910026, 0x01108080, s_ptpd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00530026, 0x01108080, s_ptpd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00620044, 0x11800C40, s3x4, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00440062, 0x0C202300, s3x4, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   {0x00670055, 0x31800420, s3x4, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-4, 5, -2, 4, 5, 5, 2, 4, -3, 5, -6, 4, 4, 5, 6, 4, 4, -5, 2, -4, -5, -5, -2, -4, 3, -5, 6, -4, -4, -5, -6, -4, 127}},
   {0x00840022, 0x06001300, s2x5, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00220084, 0x21080840, s2x5, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   {0x00E20004, 0x09002400, s1x8, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x000400E2, 0x08004202, s1x8, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   {0x01220004, 0x49002400, s1x10, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01620004, 0x49012400, s1x12, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01A20004, 0x49012404, s1x14, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x02E20004, 0x4B012404, s1x14, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01E20004, 0x49092404, s1x16, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00620022, 0x00088006, s2x4, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00220062, 0x10108004, s2x4, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   {0x00440022, 0x04000300, s2x3, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00220044, 0x01000840, s2x3, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   {0x00A20022, 0x000C8026, s2x6, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x002200A2, 0x10108484, s2x6, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   {0x00C40022, 0x26001B00, s2x7, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01040022, 0x36009B00, s2x9, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00E20022, 0x004C8036, s2x8, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x002200E2, 0x12908484, s2x8, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   {0x01220022, 0x006C8136, s2x10,UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01620022, 0x026C81B6, s2x12,UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A20044, 0x19804E40, s3x6, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00E20044, 0x1D806E41, s3x8, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00840062, 0x4E203380, s4x5, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00620084, 0x31888C60, s4x5, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   // Fudge this to a 4x5.  People were trucking in a qtag.
   {0x00670095, 0x10840C40, s4x5, UINT32_C(~0), 1, warn__none, (const coordrec *) 0,
    {-4, 1, -2, 0, 5, 9, 2, 8, 4, -1, 2, 0, -5, -9, -2, -8,
     -4, 9, -2, 8, 5, 1, 2, 0, 4, -9, 2, -8, -5, -1, -2, 0}},
   // And this, similar trucking.
   {0x00670085, 0x21088420, s4x5, UINT32_C(~0), 1, warn__none, (const coordrec *) 0,
    {-4, 5, -2, 4, 5, 5, 2, 4, 4, -5, 2, -4, -5, -5, -2, -4, 127}},
   // And this, similar trucking.
   {0x00870055, 0x09000420, s4x5, UINT32_C(~0), 1, warn__none, (const coordrec *) 0,
    {-8, 5, -6, 4, 5, 5, 2, 4, 8, -5, 6, -4, -5, -5, -2, -4, 127}},
   // And this, similar trucking.
   //             This replaces the stuff from line 1510.
   {0x00970055, 0x01400400, s4x5, UINT32_C(~0), 1, warn__none, (const coordrec *) 0,
    {-4, 5, -2, 4, 9, 5, 6, 4, 4, -5, 2, -4, -9, -5, -6, -4, 127}},

   // This is the aforesaid stuff, moved below.
   // The points pressed ahead from normal diamonds.  Fudge to quadruple diamonds.
   {0x00970055, 0x01400480, nothing, UINT32_C(~0), 0, warn__check_quad_dmds, &press_qtag_4dmd1, {127}},
   // Same, other way.
   {0x00870055, 0x09080400, nothing, UINT32_C(~0), 0, warn__check_quad_dmds, &press_qtag_4dmd2, {127}},


   {0x00A20062, 0x109CC067, s4x6, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x006200A2, 0x1918C4C6, s4x6, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   {0x00C40062, 0x6E001B80, s3oqtg, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00C40062, 0x6E001B80, s3oqtg, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00620062, 0x1018C046, s4x4, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01040026, 0x04100348, srigger12, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00C40026, 0x04100348, srigger12, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}}, // Only 7 wide, but no setup for that.
   {0x00930066, 0x01080C40, sbigh, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00910062, 0x01080C40, sbigh, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00910022, 0x01080C40, sbigh, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   // Pressed out from a bone.
   {0x00A20066, 0x01840421, sbigh, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-10, 6, -9, 6, -10, 2, -9, 2, -10, -6, -9, -6, -10, -2, -9, -2,
     10, 6, 9, 6, 10, 2, 9, 2, 10, -6, 9, -6, 10, -2, 9, -2}},
   {0x00E20026, 0x01440430, sbigbone, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01620026, 0x4A00A484, sdblbone, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},

   // Next 3 must follow the "bigbone" entry above.
   {0x00A20026, 0x090C0422, slinebox, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00570066, 0x18118A04, sboxdmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00260084, 0x20080861, sboxpdmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},

   {0x00840026, 0x06021308, sline2box, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00C40026, 0x20120809, sline6box, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A20026, 0x09048424, sdbltrngl4, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},

   {0x01620026, 0x41450430, sdblrig, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01220026, 0x41450430, sdblrig, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00E20026, 0x0800A404, sbigrig, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01220026, 0x4800A404, sbigrig, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01260055, 0x49002480, s5x1dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00E60055, 0x49002480, s5x1dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01150026, 0x20048212, s1x5dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01550026, 0x20048212, s1x5dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00E20026, 0x0808A006, swiderigger,UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00460044, 0x41040010, s_323, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00660044, 0x41040410, s_343, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00660066, 0x18100400, s_343, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-2, -6, -4, -4,    0, 6, 0, 4,    2, -6, 4, -4,    2, 6, 4, 4,    0, -6, 0, -4, -2, 6, -4, 4, 127}},
   {0x00840066, 0x08202008, s_343, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-4, -6, -4, -4,    0, 6, 0, 4,    4, -6, 4, -4,    4, 6, 4, 4,
     0, -6, 0, -4,      -4, 6, -4, 4,  8, 0, 6, 0,      -8, 0, -6, 0, 127}},
   {0x00660066, 0x00202600, s_hrglass, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-4, -6, -5, -5, 6, 0, 5, 0, 4, -6, 4, -5, 4, 6, 5, 5, -6, 0, -5, 0, -4, 6, -4, 5, 127}},
   {0x00860044, 0x49650044, s_525, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00440044, 0x49650044, s_525, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00860044, 0x41250410, s_545, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00860044, 0x41250018, sh545, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00840004, 0x00000008, sh545, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},

   {0x00A600A6, 0x09006602, sx1x8, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {2, 0, 6, 4, 127}},
   {0x00A600E6, 0x09006602, sx1x8, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {2, 0, 6, 4, 127}},
   {0x00E600A6, 0x09006602, sx1x8, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {2, 0, 6, 4, 127}},
   {0x00E600E6, 0x09006602, sx1x8, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {2, 0, 6, 4, 127}},
   {0x006600A6, 0x09004400, sx1x8, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {2, 0, 6, 4, 127}},
   {0x006600E6, 0x09006602, sx1x8, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {2, 0, 6, 4, 127}},
   {0x00E60066, 0x09006602, sx1x8, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {2, 0, 6, 4, 127}},

   {0x00860022, 0x02080300, s_ntrglccw,UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00860022, 0x04001202, s_ntrglcw, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},

   {0x00220022, 0x00008004, s2x2, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A20004, 0x09000400, s1x6, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   // Two colliding 1/2 circulates from as-couples T-bone.
   {0x00930004, 0x21008400, s1x6, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-9, 0, -10, 0, 9, 0, 10, 0, -5, 0, -6, 0, 5, 0, 6, 0, 127}},
   {0x00040093, 0x0A000280, s1x6, UINT32_C(~0), 1, warn__none, (const coordrec *) 0,
    {0, -9, 0, -10, 0, 9, 0, 10, 0, -5, 0, -6, 0, 5, 0, 6, 127}},
   {0x00620004, 0x01000400, s1x4, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00040062, 0x08000200, s1x4, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   {0x00550026, 0x20020200, sdmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00440004, 0x00020001, s1x3, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00040044, 0x00040001, s1x3, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   {0x00220004, 0x01000000, s1x2, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00040022, 0x00000200, s1x2, UINT32_C(~0), 1, warn__none, (const coordrec *) 0, {127}},
   {0x00A20066, 0x09084042, sbigptpd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A20026, 0x414C0032, sdblspindle, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x01220026, 0x414C0032, sdblspindle, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00950063, 0x01080C60, s_hqtag, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00930067, 0x01080C60, s_hqtag, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00970067, 0x01080C60, s_hqtag, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00970057, 0x01080C60, s_hqtag, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00930057, 0x01080C60, s_hqtag, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00970055, 0x114008A0, sbighrgl,  UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00E70026, 0x20440230, sbigdhrgl, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A60026, 0x03080020, s_nptrglcw, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00A60026, 0x01041002, s_nptrglccw, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0x00510062, 0x02000004, s2x4, UINT32_C(~0), 1, warn__none, (const coordrec *) 0,
    {-5, 6, -2, 6, 5, 6, 2, 6, -5, -6, -2, -6, 5, -6, 2, -6, 127}},
   {0x00770077, 0x22808044, s_c1phan, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {2, 2, 4, 3, -2, -2, -5, -2, 127}},
   {0x00770077, 0x02808084, s_c1phan, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-2, 2, -2, 4, 2, -2, 3, -5, 127}},
   {0x00F700E7, 0x2101180C, s_c1phan, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {2, 2, 11, 4, -2, -2, -10, -7, 127}},
   {0x00F700E7, 0x25019000, s_c1phan, UINT32_C(~0), 0, warn__none, (const coordrec *) 0,
    {-2, 2, -3, 9, 2, -2, 4, -10, 127}},
   {0x00840046, 0x42021210, s_23232, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}},
   {0}};


static checkitem c1fixup =
{0, 0, s_c1phan, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}};

static checkitem s4p2x1fixup =
{0, 0, s4p2x1dmd, UINT32_C(~0), 0, warn__none, (const coordrec *) 0, {127}};


void initialize_matrix_position_tables()
{
   const checkitem *p;

   for (p = checktable ; p < checktable+13 ; p++) {
      if (p->mask == 0)
         continue;
      if (p->new_checkptr)
         continue;
      if (p->new_setup == nothing)
         break;

      int i;
      int xmax, ymax, x, y, k;
      uint32 signature, xpar, ypar;
      xmax = xpar = ymax = ypar = signature = 0;
      const coordrec *checkptr = setup_attrs[p->new_setup].setup_coords;

      for (i=0; i<=attr::klimit(p->new_setup); i++) {
         if (!(p->mask & (1 << i)))
            continue;
         x = checkptr->xca[i];
         y = checkptr->yca[i];
         // Check for fudging.

         if (p->fixer[0] != 127) {
            for (k=0; k<32 && p->fixer[k]!=127; k+=4) {
               if (x == p->fixer[k+2] && y == p->fixer[k+3]) {
                  x = p->fixer[k];
                  y = p->fixer[k+1];
                  break;
               }
            }
         }

         // Compute new max, parity, and signature info.

         if ((x < 0) || ((x == 0) && (y < 0))) { x = -x; y = -y; }
         signature |= 1 << ((31000 + 12*x - 11*y) % 31);
         if (y < 0) y = -y;
         /* Now x and y have both had absolute values taken. */
         if (x > xmax) xmax = x;
         if (y > ymax) ymax = y;
         k = x | 4;
         xpar |= (k & (~(k-1)));
         k = y | 4;
         ypar |= (k & (~(k-1)));
      }

      ypar |= (xmax << 20) | (xpar << 16) | (ymax << 4);

      if (p->ypar != ypar || p->sigcheck != signature)
         gg77->iob88.fatal_error_exit(1, "Matrix position table initialization failed");
   }
}


static void finish_matrix_call(
   matrix_rec matrix_info[],
   int nump,
   bool do_roll_stability,
   collision_severity allow_collisions,
   bool allow_fudging,
   merge_action action,
   setup *people,
   setup *result) THROW_DECL
{
   int i;
   int xmax, ymax, x, y, k;
   uint32 signature, xpar, ypar;
   const coordrec *checkptr;
   result->clear_people();

   xmax = xpar = ymax = ypar = signature = 0;
   int xatnonzeroy = -1000;

   for (i=0; i<nump; i++) {

      /* If this person's position has low bit on, that means we consider his coordinates
         not sufficiently well-defined that we will allow him to do any pressing or
         trucking.  He is only allowed to turn.  That is, we will require deltax and
         deltay to be zero.  An example of this situation is the points of a galaxy. */

      /* ****** This seems to be too restrictive.  There may have been good reason for doing this
         at one time, but now it makes all press and truck calls illegal in C1 phantoms.  The
         table for C1 phantoms has been carefully chosen to make things legal only within one's
         own miniwave, but it requires odd numbers.  Perhaps we need to double the resolution
         of things in matrix_info[i].x or y, but that should wait until after version 28
         is released. */

      /* So this is patched out.  The same problem holds for bigdmds.
         if (((matrix_info[i].x | matrix_info[i].y) & 1) &&
             (matrix_info[i].deltax | matrix_info[i].deltay))
            fail("Someone's ending position is not well defined.");
      */

      switch (matrix_info[i].dir) {
      case 0:
         matrix_info[i].x += matrix_info[i].deltax;
         matrix_info[i].y += matrix_info[i].deltay;
         break;
      case 1:
         matrix_info[i].x += matrix_info[i].deltay;
         matrix_info[i].y -= matrix_info[i].deltax;
         break;
      case 2:
         matrix_info[i].x -= matrix_info[i].deltax;
         matrix_info[i].y -= matrix_info[i].deltay;
         break;
      case 3:
         matrix_info[i].x -= matrix_info[i].deltay;
         matrix_info[i].y += matrix_info[i].deltax;
         break;
      }

      x = matrix_info[i].x;
      y = matrix_info[i].y;

      // Compute new max, parity, and signature info.

      if ((x < 0) || ((x == 0) && (y < 0))) { x = -x; y = -y; }
      signature |= 1 << ((31000 + 12*x - 11*y) % 31);
      if (y < 0) y = -y;
      /* Now x and y have both had absolute values taken. */
      if (x > xmax) xmax = x;
      if (y > ymax) ymax = y;
      k = x | 4;
      xpar |= (k & (~(k-1)));
      k = y | 4;
      ypar |= (k & (~(k-1)));

      if (y != 0) xatnonzeroy = x;
   }

   ypar |= (xmax << 20) | (xpar << 16) | (ymax << 4);

   // First, take care of simple things from the table.

   const checkitem *p;

   for (p = checktable ; p->ypar ; p++) {
      if (p->ypar == ypar && (~p->sigcheck & signature) == 0)
         goto found_item;
   }

   // C1 phantoms are messy -- the max coordinates can vary a lot.
   // Depending on how the setup is actually occupied, xmax and ymax may vary.
   // For now, we only look at the signature.
   if ((signature & (~0x278198CC)) == 0)
      p = &c1fixup;
   else
      fail("Can't figure out result setup.");

 found_item:

   // The tables for splinedmd and s4p2x1dmd have the same ypar and signature!
   // Use the "xatnonzeroy" value to distinguish them.
   if (p->new_setup == splinedmd && xatnonzeroy == 4)
      p = &s4p2x1fixup;

   // Perform any required fixups, moving people around before
   // sending them to be placed in the final setup.  That final
   // setup placement can be very picky.
   if (p->fixer[0] != 127) {
      if ((p->new_rot & 0x100) && !allow_fudging)
         fail("Can't figure out result setup.");

      for (i=0; i<nump; i++) {
         for (k=0; k<32 && p->fixer[k]!=127; k+=4) {
            if (matrix_info[i].x == p->fixer[k] && matrix_info[i].y == p->fixer[k+1]) {
               matrix_info[i].x = p->fixer[k+2];
               matrix_info[i].y = p->fixer[k+3];
               break;
            }
         }
      }
   }

   warn(p->warning);

   if (p->new_setup == nothing)
      checkptr = p->new_checkptr;
   else
      checkptr = setup_attrs[p->new_setup].setup_coords;

   result->rotation = p->new_rot & 3;
   result->eighth_rotation = 0;
   result->kind = checkptr->result_kind;

   int doffset = 32 - (1 << (checkptr->xfactor-1));

   collision_collector CC(allow_collisions);

   for (i=0; i<nump; i++) {
      int mx, my;
      matrix_rec *mp = &matrix_info[i];

      if (result->rotation) { mx = -mp->y; my = mp->x; }
      else { mx = mp->x; my = mp->y; }

      int place = checkptr->diagram[doffset - ((my >> 2) << checkptr->xfactor) + (mx >> 2)];
      if (place < 0) fail("Person has moved into a grossly ill-defined location.");

      if ((checkptr->xca[place] != mx) || (checkptr->yca[place] != my))
         if (checkptr->result_kind != sx1x8)   // Know that these maps are OK, and some fudging took place.
            fail("Person has moved into a slightly ill-defined location.");

      int rot = ((mp->deltarot-result->rotation) & 3)*011;
      CC.install_with_collision(result, place, people, i, rot);

      if (do_roll_stability) {
         uint32 sliderollstuff = (mp->roll_stability_info * (NROLL_BIT/DBSLIDEROLL_BIT)) & NSLIDE_ROLL_MASK;
         // If just "L" or "R" (but not "M", that is, not both bits), turn on "moved".
         if ((sliderollstuff+NROLL_BIT) & (NROLL_BIT*2)) sliderollstuff |= PERSON_MOVED;
         result->people[place].id1 &= ~NSLIDE_ROLL_MASK;
         result->people[place].id1 |= sliderollstuff;

         if (result->people[place].id1 & STABLE_ENAB)
            do_stability(&result->people[place].id1,
                         mp->roll_stability_info/DBSTAB_BIT,
                         mp->deltarot, mp->mirror_this_op);
      }
   }

   // Pass the "action" in case it's merge_c1_phantom_real_couples.
   CC.fix_possible_collision(result, action);
}


static void set_matrix_info_from_calldef(matrix_rec & mi, uint32 datum)
{
   mi.deltax = (uncompress_position_number(datum) - 16) << 1;
   mi.deltay = (((datum >> 16) & 0x3F) - 16) << 1;   // This part isn't compressed.
   mi.deltarot = datum & 3;
   mi.roll_stability_info = datum;   // For slide, roll, and stability.
}


static void mirror_slide_roll(matrix_rec *ppp)
{
   // Switch the roll direction.
   ppp->roll_stability_info ^=
      (((ppp->roll_stability_info+DBSLIDEROLL_BIT) >> 1) & DBSLIDEROLL_BIT) * 3;
   // Switch the slide direction.
   ppp->roll_stability_info ^=
      (((ppp->roll_stability_info+DBSLIDE_BIT) >> 1) & DBSLIDE_BIT) * 3;
}

static void matrixmove(
   setup *ss,
   uint32 flags,
   const uint32 *callstuff,
   setup *result) THROW_DECL
{
   setup people;
   matrix_rec matrix_info[9];
   int i, nump, alldelta;

   alldelta = 0;

   nump = start_matrix_call(ss, matrix_info, 0, flags, &people);

   for (i=0; i<nump; i++) {
      matrix_rec *thisrec = &matrix_info[i];

      if (!(flags & MTX_USE_SELECTOR) || thisrec->sel) {
         // This is legal if girlbit or boybit is on (in which case we use
         // the appropriate datum) or if the two data are identical so
         // the sex doesn't matter.
         if ((thisrec->girlbit | thisrec->boybit) == 0 &&
             callstuff[0] != callstuff[1]) {
            if (flags & MTX_USE_VEER_DATA)
               fail("Can't determine lateral direction of this person.");
            else
               fail("Can't determine sex of this person.");
         }

         set_matrix_info_from_calldef(*thisrec, callstuff[thisrec->girlbit]);

         if (flags & MTX_MIRROR_IF_RIGHT_OF_CTR) {
            int relative_delta_x = (thisrec->dir & 1) ? thisrec->nicey : thisrec->nicex;
            if ((thisrec->dir+1) & 2) relative_delta_x = -relative_delta_x;

            if (relative_delta_x > 0) {
               thisrec->mirror_this_op = true;
               thisrec->deltarot = (-thisrec->deltarot) & 3;
               thisrec->deltax = -thisrec->deltax;
               mirror_slide_roll(thisrec);
               // The stability info is too hard to fix at this point.
               // It will be handled later.
            }
            else if (relative_delta_x == 0)
               fail("Person is on a center line.");
         }

         if (flags & MTX_USE_NUMBER) {
            int count = current_options.number_fields & NUMBER_FIELD_MASK;
            thisrec->deltax *= count;
            thisrec->deltay *= count;
         }

         alldelta |= thisrec->deltax | thisrec->deltay;
      }
   }

   if (alldelta != 0) {
      if (ss->cmd.cmd_misc_flags & CMD_MISC__DISTORTED)
         fail("This call not allowed in distorted or virtual setup.");

      if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
         fail("Can't split the setup.");
   }
   else
      // This call split the setup in every possible way.
      result->result_flags.maximize_split_info();

   finish_matrix_call(matrix_info, nump, true, collision_severity_no, true, merge_strict_matrix, &people, result);

   reinstate_rotation(ss, result);
   clear_result_flags(result);

   // If the call just kept a 2x2 in place, and they were the outsides, make
   // sure that the elongation is preserved.

   switch (ss->kind) {
   case s2x2: case s_short6:
      result->result_flags.misc |= ss->cmd.prior_elongation_bits & 3;
      break;
   case s1x2: case s1x4: case sdmd:
      result->result_flags.misc |= 2 - (ss->rotation & 1);
      break;
   }
}



static void do_part_of_pair(matrix_rec *thisrec, int base, const uint32 *callstuff) THROW_DECL
{
   if (thisrec->far_squeezer) base += 8;

   // This is legal if girlbit or boybit is on (in which case we use the appropriate datum)
   //   or if the two data are identical so the sex doesn't matter.
   if ((thisrec->girlbit | thisrec->boybit) == 0 && callstuff[base] != callstuff[base+1])
      fail("Can't determine sex of this person.");
   uint32 datum = callstuff[base+thisrec->girlbit];
   if (datum == 0) failp(thisrec->id1, "can't do this call.");
   set_matrix_info_from_calldef(*thisrec, datum);
   thisrec->realdone = true;
}


static void do_pair(
   matrix_rec *ppp,        /* Selected person */
   matrix_rec *qqq,        /* Unselected person */
   const uint32 *callstuff,
   uint32 flags,
   int flip,
   int filter) THROW_DECL             // 1 to do N/S facers, 0 for E/W facers.
{
   if (callstuff) {     // Doing normal matrix call.
      if ((!(flags & (MTX_IGNORE_NONSELECTEES | MTX_BOTH_SELECTED_OK))) && qqq->sel)
         fail("Two adjacent selected people.");

      // We know that either ppp is actually selected, or we are not using selectors.

      if ((filter ^ ppp->dir) & 1) {
         int base = (ppp->dir & 2) ? 6 : 4;
         if (!(flags & MTX_USE_SELECTOR)) base &= 3;
         do_part_of_pair(ppp, base^flip, callstuff);
      }

      if ((filter ^ qqq->dir) & 1) {
         int base = (qqq->dir & 2) ? 0 : 2;
         if ((flags & MTX_IGNORE_NONSELECTEES) || qqq->sel) base |= 4;
         do_part_of_pair(qqq, base^flip, callstuff);
      }

      if (flags & MTX_MIRROR_IF_RIGHT_OF_CTR) {
         int relative_ppp_x = (ppp->dir & 1) ? ppp->nicey : ppp->nicex;
         if ((ppp->dir+1) & 2) relative_ppp_x = -relative_ppp_x;
         int relative_qqq_x = (qqq->dir & 1) ? qqq->nicey : qqq->nicex;
         if ((qqq->dir+1) & 2) relative_qqq_x = -relative_qqq_x;

         if (relative_ppp_x > 0 && relative_qqq_x > 0) {
            ppp->mirror_this_op = true;
            qqq->mirror_this_op = true;
            // Make this pair of people anchor in the other direction, by
            // negating their rotation, swapping their Y motions, and swapping
            // and negating their X motions.
            ppp->deltarot = (-ppp->deltarot) & 3;
            qqq->deltarot = (-qqq->deltarot) & 3;
            int ttt = ppp->deltax;
            ppp->deltax = -qqq->deltax;
            qqq->deltax = -ttt;
            ttt = ppp->deltay;
            ppp->deltay = qqq->deltay;
            qqq->deltay = ttt;
            // Switch the roll directions.
            mirror_slide_roll(ppp);
            mirror_slide_roll(qqq);
            // The stability info is too hard to fix at this point.
            // It will be handled later.
         }
         else if (relative_ppp_x >= 0 || relative_qqq_x >= 0)
            fail("People are not anchoring consistently.");
      }
   }
   else {    // Doing "drag" concept.
      // ppp and qqq are a pair, independent of selection.
      // They may contain a dragger and a draggee.
      if (ppp->sel) {
         if (qqq->sel) fail("Two adjacent people being dragged.");
         ppp->realdone = true;
         ppp->deltax = qqq->x;
         ppp->deltay = qqq->y;
         ppp->deltarot = qqq->orig_source_idx;
         ppp->nearestdrag = qqq->dir;
      }
      else if (qqq->sel) {
         qqq->realdone = true;
         qqq->deltax = ppp->x;
         qqq->deltay = ppp->y;
         qqq->deltarot = ppp->orig_source_idx;
         qqq->nearestdrag = ppp->dir;
      }
   }

   ppp->done = true;
   qqq->done = true;
}



static void make_matrix_chains(
   matrix_rec matrix_info[],
   int nump,
   bool finding_far_squeezers,
   uint32 flags,
   int filter) THROW_DECL       // 1 for E/W chains, 0 for N/S chains.
{
   int i, j, k, l;

   // Find adjacency relationships, and fill in the "se"/"nw" pointers.

   for (i=0; i<nump; i++) {
      matrix_rec *mi = &matrix_info[i];
      if ((flags & MTX_IGNORE_NONSELECTEES) && (!mi->sel)) continue;

      for (j=0; j<nump; j++) {
         matrix_rec *mj = &matrix_info[j];
         if ((flags & MTX_IGNORE_NONSELECTEES) && (!mj->sel)) continue;

         // Find out if these people are adjacent in the right way.

         int ix, jx, iy, jy;
         if (flags & MTX_FIND_JAYWALKERS) {
            jx = mj->nicex;
            ix = mi->nicex;
            jy = mj->nicey;
            iy = mi->nicey;
         }
         else if (filter) {
            jx = - mj->y;
            ix = - mi->y;
            jy = - mj->x;
            iy = - mi->x;
         }
         else {
            jx = mj->x;
            ix = mi->x;
            jy = mj->y;
            iy = mi->y;
         }

         int delx = ix - jx;
         int dely = iy - jy;
         uint32 dirxor = mi->dir ^ mj->dir;

         if (flags & MTX_FIND_JAYWALKERS) {
            if (dirxor == 0) continue;   // Can't be facing same direction.

            // Vertdist is the forward distance to the jaywalkee.  We demand
            // that it be strictly positive.  We also consider only jaywalkees
            // that are either facing back toward us or are facing lateral but
            // are in such a position that they can see us (that is, their
            // vertdist, called "returndist" here, is also positive.)

            int vertdist = (mi->dir & 1) ? delx : dely;
            if (!(mi->dir & 2)) vertdist = -vertdist;
            if (vertdist <= 0) continue;

            int returndist = (mj->dir & 1) ? delx : dely;
            if ((mj->dir & 2)) returndist = -returndist;
            if (returndist <= 0) continue;

            // We know the jaywalkers can see each other.

            int latdist = (mi->dir & 1) ? dely : delx;
            if ((mi->dir + 1) & 2) latdist = -latdist;

            // Compute the Euclidean distance as a measure of badness.
            // (Actually, the square of the distance.)

            int euc_distance = latdist*latdist + vertdist*vertdist;

            // Find out whether this candidate is one of the best, and, if so,
            // where she should be in the list.

            for (k=NUMBER_OF_JAYWALK_CANDIDATES-1 ; k>=0 ; k--) {
               if (euc_distance >= mi->jpersons[k].jp_eucdist) break;
               if (k < NUMBER_OF_JAYWALK_CANDIDATES-1)
                  mi->jpersons[k+1] = mi->jpersons[k];
            }

            if (k < NUMBER_OF_JAYWALK_CANDIDATES-1) {
               mi->jpersons[k+1].jp = j;
               mi->jpersons[k+1].jp_lateral = latdist;
               mi->jpersons[k+1].jp_vertical = vertdist;
               mi->jpersons[k+1].jp_eucdist = euc_distance;
            }

            // But we only allow the top NUMBER_OF_JAYWALK_VERT_CAND items
            // when measured by vertical distance.  That is, we consider the best 3
            // overall candidates, but only the best 2 vertical distances.  Any
            // candidate that comes in third in vertical distance is just too
            // far to walk.

            int vert_cutoff = mi->jpersons[NUMBER_OF_JAYWALK_VERT_CAND-1].jp_vertical;

            for (k=NUMBER_OF_JAYWALK_VERT_CAND ; k<NUMBER_OF_JAYWALK_CANDIDATES ; k++) {
               if (mi->jpersons[k].jp_vertical > vert_cutoff) {
                  // Cut out person k.
                  for (l=k ; l<NUMBER_OF_JAYWALK_CANDIDATES-1 ; l++)
                     mi->jpersons[l] = mi->jpersons[l+1];
                  mi->jpersons[NUMBER_OF_JAYWALK_CANDIDATES-1].jp = -1;
               }
            }
         }
         else {
            if (dely != (finding_far_squeezers ? 12 : 4) || delx != 0) continue;

            // Now, if filter = 1, person j is just east of person i.
            // If filter = 0, person j is just south of person i.

            if (!(flags & MTX_TBONE_IS_OK)) {
               if ((mi->dir ^ filter) & 1) {
                  if (dirxor & 1) {
                     if (!(flags & MTX_STOP_AND_WARN_ON_TBONE)) fail("People are T-boned.");
                     mi->tbstopse = true;
                     mj->tbstopnw = true;
                     break;
                  }
                  else {
                     if ((flags & MTX_MUST_FACE_SAME_WAY) && dirxor)
                        continue;    // No adjacency here.
                     // fail("Paired people must face the same way.");
                  }
               }
               else
                  break;
            }

            if (mj->nextnw) fail("Adjacent to too many people.");

            mi->nextse = mj;
            mj->nextnw = mi;
            mi->far_squeezer = finding_far_squeezers;
            mj->far_squeezer = finding_far_squeezers;
            break;
         }
      }
   }
}


static int jaywalk_recurse(
   matrix_rec matrix_info[],
   int start_index,
   int nump,
   uint32 flags) THROW_DECL
{
   int i, j, k;
   matrix_rec best_info[9];
   int best_cost;

   // Pre-clean: Clear out any links that aren't bidirectional.
   // First, mark all targets.

   for (i=0; i<nump; i++)
      matrix_info[i].jaywalktargetmask = 0;

   for (i=0; i<nump; i++) {
      matrix_rec *mi = &matrix_info[i];
      if ((flags & MTX_IGNORE_NONSELECTEES) && !mi->sel) continue;

      for (k=0 ; k<NUMBER_OF_JAYWALK_CANDIDATES ; k++) {
         j = mi->jpersons[k].jp;
         if (j >= 0)
            matrix_info[j].jaywalktargetmask |= 1 << i;
      }
   }

   // Delete target pointers that aren't bidirectional.

   for (i=0; i<nump; i++) {
      matrix_rec *mi = &matrix_info[i];
      if ((flags & MTX_IGNORE_NONSELECTEES) && !mi->sel) continue;

      for (k=0 ; k<NUMBER_OF_JAYWALK_CANDIDATES ; k++) {
         j = mi->jpersons[k].jp;
         if (j<0) break;
         if (!(mi->jaywalktargetmask & (1<<j))) {
            // Delete this pointer from i to j, since no pointer from j to i exists.
            matrix_info[j].jaywalktargetmask &= ~(1 << i);
            for ( ; k<NUMBER_OF_JAYWALK_CANDIDATES-1 ; k++)
               mi->jpersons[k] = mi->jpersons[k+1];
            mi->jpersons[NUMBER_OF_JAYWALK_CANDIDATES-1].jp = -1;
         }
      }

      // If selected person has no jaywalkee, it is an error.
      if (mi->jpersons[0].jp < 0)
         return -i-1;
   }

   // Find first index with multiple choices.
   for (i=start_index; i<nump; i++) {
      matrix_rec *mi = &matrix_info[i];
      if ((flags & MTX_IGNORE_NONSELECTEES) && !mi->sel) continue;

      int choice_count = 0;

      for (k=0 ; k<NUMBER_OF_JAYWALK_CANDIDATES ; k++) {
         if (mi->jpersons[k].jp >= 0) choice_count++;
      }

      if (choice_count >= 2) {
         int cost_or_error_code;
         bool found_a_solution = false;

         for (k=0 ; k<NUMBER_OF_JAYWALK_CANDIDATES ; k++) {
            int target = mi->jpersons[k].jp;
            if (target >= 0) {

               // This person wants to jay walk with someone.  First, save a temporary
               // copy of the whole matrix_info array;

               matrix_rec temp_info[9];

               for (j=0; j<nump; j++) temp_info[j] = matrix_info[j];

               // Alter the temporary array so that this person has only that one choice.

               temp_info[i].jpersons[0] = mi->jpersons[k];
               for (j=1 ; j<NUMBER_OF_JAYWALK_CANDIDATES ; j++)
                  temp_info[i].jpersons[j].jp = -1;

               // The array is badly messed up, but the next pre-clean will fix them.
               // Recurse.  A returned value <0 says that this choice doesn't work.
               cost_or_error_code = jaywalk_recurse(temp_info, i+1, nump, flags);

               if (cost_or_error_code >= 0) {
                  // Found a solution.  Save the best solution.
                  if (!found_a_solution || cost_or_error_code < best_cost) {
                     for (j=0; j<nump; j++) best_info[j] = temp_info[j];
                     best_cost = cost_or_error_code;
                  }

                  found_a_solution = true;
               }
            }
         }

         // Finished the K scan.  Did any recursion find anything?
         // (If more than 1 thing was found, they were compared, and we have the best.)

         if (found_a_solution) {
            for (j=0; j<nump; j++) matrix_info[j] = best_info[j];
            return best_cost;
         }
         else {
            // No recursion worked; return last error code from a failed recursion.  It is negative.
            return cost_or_error_code;
         }
      }
   }

   // If did the whole scan without needing further recursion, the current
   // configuration is a solution.  By leaving it in matrix_info, it gets returned.
   // Calculate its cost.

   int this_cost = 0;

   for (j=0; j<nump; j++) {
      matrix_rec *mj = &matrix_info[j];
      if ((flags & MTX_IGNORE_NONSELECTEES) && !mj->sel) continue;
      this_cost += mj->jpersons[0].jp_eucdist;
   }

   return this_cost;
}

static void process_jaywalk_chains(
   matrix_rec matrix_info[],
   int nump,
   const uint32 *callstuff,
   uint32 flags) THROW_DECL
{
   int i, j;

   int errnum = jaywalk_recurse(matrix_info, 0, nump, flags);
   if (errnum < 0)
      failp(matrix_info[-errnum-1].id1, "can't find person to jaywalk with.");

   for (i=0; i<nump; i++) {
      matrix_rec *mi = &matrix_info[i];
      if ((flags & MTX_IGNORE_NONSELECTEES) && !mi->sel) continue;
      j = mi->jpersons[0].jp;
      matrix_rec *mj = &matrix_info[j];
      int delx = mj->x - mi->x;
      int dely = mj->y - mi->y;
      int deltarot;

      uint32 datum = callstuff[mi->girlbit];
      if (datum == 0) failp(mi->id1, "can't do this call.");

      if (mi->dir & 2) delx = -delx;
      if ((mi->dir+1) & 2) dely = -dely;

      set_matrix_info_from_calldef(*mi, datum);
      mi->realdone = true;

      deltarot = (mj->dir - mi->dir + 2) & 3;

      if (deltarot) {
         // This person went "around the corner" due to facing
         // direction of other dancer.
         if (mi->deltarot ||
             ((mi->roll_stability_info/DBSTAB_BIT) & STB_MASK) !=
             STB_NONE+STB_REVERSE) {   // Looking for "Z".
            // Call definition also had turning, as in "jay slide thru".
            // Just erase the stability info.
            mi->roll_stability_info &= ~(DBSTAB_BIT * STB_MASK);
         }
         else {
            mi->roll_stability_info &= ~(DBSTAB_BIT * STB_MASK);
            mi->roll_stability_info |= DBSTAB_BIT * STB_A;
            if ((deltarot & 3) == 1)
               mi->roll_stability_info |= DBSTAB_BIT * STB_REVERSE;
         }
      }

      mi->deltarot += deltarot;
      mi->deltarot &= 3;

      if (mi->dir & 1) { mi->deltax += dely; mi->deltay = +delx; }
      else             { mi->deltax += delx; mi->deltay = +dely; }
   }
}



static void process_nonjaywalk_chains(
   matrix_rec matrix_info[],
   int nump,
   const uint32 *callstuff,
   uint32 flags,
   int filter) THROW_DECL    // 1 for E/W chains, 0 for N/S chains.
{
   bool another_round = true;
   int i;

   // Pick out pairs of people and move them.

   while (another_round) {
      another_round = false;

      for (i=0; i<nump; i++) {
         matrix_rec *mi = &matrix_info[i];

         if (!mi->done) {
            if (mi->nextse) {
               // This person might be ready to be paired up with someone.
               if (mi->nextnw)
                  // This person has neighbors on both sides.  Can't do anything yet.
                  ;
               else {
                  // This person has a neighbor on south/east side only.

                  if (mi->tbstopnw) warn(warn__not_tbone_person);

                  if ((!(flags & MTX_USE_SELECTOR)) || mi->sel) {
                     // Do this pair.  First, chop the pair off from anyone else.

                     if (mi->nextse->nextse) mi->nextse->nextse->nextnw = 0;
                     mi->nextse->nextse = 0;
                     another_round = true;
                     do_pair(mi, mi->nextse, callstuff, flags, 0, filter);
                  }
               }
            }
            else if (mi->nextnw) {
               // This person has a neighbor on north/west side only.

               if (mi->tbstopse) warn(warn__not_tbone_person);

               if ((!(flags & MTX_USE_SELECTOR)) || mi->sel) {
                  // Do this pair.  First, chop the pair off from anyone else.

                  if (mi->nextnw->nextnw) mi->nextnw->nextnw->nextse = 0;
                  mi->nextnw->nextnw = 0;
                  another_round = true;
                  do_pair(mi, mi->nextnw, callstuff, flags, 2, filter);
               }
            }
            else {
               // Person is alone.  If this is his lateral axis,
               // mark him done and don't move him.

               if ((mi->dir ^ filter) & 1) {
                  if (mi->tbstopse || mi->tbstopnw) warn(warn__not_tbone_person);
                  if ((!(flags & MTX_USE_SELECTOR)) || mi->sel)
                     failp(mi->id1, "has no one to work with.");
                  mi->done = true;
               }
            }
         }
      }
   }
}



static void partner_matrixmove(
   setup *ss,
   uint32 flags,
   const uint32 *callstuff,
   setup *result) THROW_DECL
{
   setup people;
   matrix_rec matrix_info[9];
   int i, nump;

   if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
      fail("Can't split the setup.");

   // We allow stuff like "tandem jay walk".

   if ((ss->cmd.cmd_misc_flags & CMD_MISC__DISTORTED) &&
       !(flags & (MTX_FIND_JAYWALKERS|MTX_FIND_SQUEEZERS)))
      fail("This call not allowed in distorted or virtual setup.");

   // If this is "jay walk", we will need active phantoms.
   // The "find jaywalkers" code isn't smart enough to deal with phantoms.

   bool did_active_phantoms = false;
   if (ss->cmd.cmd_assume.assumption != cr_none &&
       (flags & (MTX_FIND_JAYWALKERS|MTX_FIND_SQUEEZERS))) {
      if (!check_restriction(ss, ss->cmd.cmd_assume, true, 99)) {
         // If check_restriction returned true, that means it couldn't do anything.
         ss->cmd.cmd_assume.assumption = cr_none;
         did_active_phantoms = true;
      }
   }

   nump = start_matrix_call(ss, matrix_info, 0, flags, &people);

   // Make the lateral chains first.

   make_matrix_chains(matrix_info, nump, false, flags, 1);
   if (flags & MTX_FIND_SQUEEZERS)
      make_matrix_chains(matrix_info, nump, true, flags, 1);

   if (flags & MTX_FIND_JAYWALKERS) {
      process_jaywalk_chains(matrix_info, nump, callstuff, flags);
   }
   else {
      process_nonjaywalk_chains(matrix_info, nump, callstuff, flags, 1);

      // Now clean off the pointers in preparation for the second pass.

      for (i=0; i<nump; i++) {
         matrix_info[i].done = false;
         matrix_info[i].nextse = 0;
         matrix_info[i].nextnw = 0;
         matrix_info[i].tbstopse = false;
         matrix_info[i].tbstopnw = false;
      }

      // Vertical chains next.

      make_matrix_chains(matrix_info, nump, false, flags, 0);
      if (flags & MTX_FIND_SQUEEZERS)
         make_matrix_chains(matrix_info, nump, true, flags, 0);
      process_nonjaywalk_chains(matrix_info, nump, callstuff, flags, 0);
   }

   // Scan for people who ought to have done something but didn't.

   for (i=0; i<nump; i++) {
      if (!matrix_info[i].realdone) {
         if ((!(flags & MTX_USE_SELECTOR)) || matrix_info[i].sel)
            failp(matrix_info[i].id1, "could not identify other person to work with.");
      }
   }

   finish_matrix_call(matrix_info, nump, true, collision_severity_no, true, merge_strict_matrix, &people, result);
   reinstate_rotation(ss, result);

   // Take out any active phantoms that we placed.

   if (did_active_phantoms) {
      for (i=0; i<=attr::slimit(result); i++) {
         if (result->people[i].id1 & BIT_ACT_PHAN)
            result->people[i].id1 = 0;
      }
   }

   clear_result_flags(result);
}


// This treats res2 as though it had rotation zero.
// Res1 is allowed to have rotation.
extern void brute_force_merge(const setup *res1, const setup *res2,
                              merge_action action, setup *result) THROW_DECL
{
   int i;
   int r = res1->rotation & 3;
   collision_severity allow_collisions = collision_severity_no;
   if (action > merge_for_own)
      allow_collisions = collision_severity_ok;
   else if (action == merge_for_own)
      allow_collisions = collision_severity_controversial;

   // First, check if the setups are nice enough for the matrix stuff,
   // as indicated by having both "setup_coords" and "nice_setup_coords".
   // If they match each other but aren't nice, we will do it the
   // extremely brute force way.

   if (res1->kind != res2->kind || r != 0 ||
       (setup_attrs[res1->kind].nice_setup_coords &&
        setup_attrs[res1->kind].setup_coords) ||
       res1->kind == s_trngl4 ||
       res1->kind == s_trngl) {

      // We know that start_matrix_call will raise an error if its incoming
      // setup is not of the sort that canonicalizes to rotation 0 or 1.
      // (This includes trngl and trngl4.)
      // Therefore, we know that r is 0 or 1.  That is, it will be if we
      // get past start_matrix_call.

      setup people;
      matrix_rec matrix_info[9];

      int nump = start_matrix_call(res1, matrix_info, 0, 0, &people);

      if (r) {
         for (i=0 ; i<nump ; i++) {
            matrix_info[i].deltarot++;
            int tt = matrix_info[i].y;
            matrix_info[i].y = -matrix_info[i].x;
            matrix_info[i].x = tt;
         }
      }

      nump = start_matrix_call(res2, matrix_info, nump, 0, &people);

      // Note that, because we set deltarot above, we must NOT turn on the
      // "do_roll_stability" argument here.  It would treat the rotation
      // as though the person had actually done a call.
      finish_matrix_call(matrix_info, nump, false, allow_collisions,
                         action > merge_for_own, action, &people, result);
      return;
   }

   // They really are the same setup.  Just stuff the people on top of each other,
   // and repair any collisions that occur.

   int rot = r * 011;
   int lim1 = attr::slimit(res1)+1;
   collision_collector CC(allow_collisions);

   if (lim1 <= 0) fail("Can't figure out result setup.");

   *result = *res2;

   CC.note_prefilled_result(result);

   for (i=0; i<lim1; i++) {
      if (res1->people[i].id1)
         CC.install_with_collision(result, i, res1, i, rot);
   }

   CC.fix_possible_collision(result);
}


extern void drag_someone_and_move(setup *ss, parse_block *parseptr, setup *result)
   THROW_DECL
{
   setup people, second_people;
   matrix_rec matrix_info[9];
   matrix_rec second_matrix_info[9];
   int i;
   bool fudged_start = false;
   uint32 flags = MTX_STOP_AND_WARN_ON_TBONE | MTX_IGNORE_NONSELECTEES;
   selector_kind saved_selector = current_options.who;
   current_options.who = parseptr->options.who;

   setup scopy = *ss;      // Will save rotation of this to the very end.
   scopy.rotation = 0;
   scopy.eighth_rotation = 0;

   if (scopy.kind == s_qtag) {
      expand::expand_setup(s_qtg_3x4, &scopy);
      fudged_start = true;
   }

   int nump = start_matrix_call(&scopy, matrix_info, 0,
      MTX_USE_SELECTOR | MTX_STOP_AND_WARN_ON_TBONE, &people);
   current_options.who = saved_selector;

   // Make the lateral chains first.

   make_matrix_chains(matrix_info, nump, false, MTX_STOP_AND_WARN_ON_TBONE, 1);
   process_nonjaywalk_chains(matrix_info, nump, (uint32 *) 0, flags, 1);

   /* Now clean off the pointers in preparation for the second pass. */

   for (i=0; i<nump; i++) {
      matrix_info[i].done = false;
      matrix_info[i].nextse = 0;
      matrix_info[i].nextnw = 0;
      matrix_info[i].tbstopse = false;
      matrix_info[i].tbstopnw = false;
   }

   // Vertical chains next.

   make_matrix_chains(matrix_info, nump, false, MTX_STOP_AND_WARN_ON_TBONE, 0);
   process_nonjaywalk_chains(matrix_info, nump, (uint32 *) 0, flags, 0);

   // Scan for people who ought to have done something but didn't.

   for (i=0; i<nump; i++) {
      if (matrix_info[i].sel)
         scopy.clear_person(matrix_info[i].orig_source_idx);
   }

   setup refudged = scopy;
   if (fudged_start)
      expand::compress_setup(s_qtg_3x4, &refudged);

   move(&refudged, false, result);

   // Expand again if it's another qtag.
   if (result->kind == s_qtag)
      expand::expand_setup(s_qtg_3x4, result);

   // Now figure out where the people who moved really are.

   int second_nump = start_matrix_call(result, second_matrix_info, 0,
                                       MTX_STOP_AND_WARN_ON_TBONE, &second_people);

   int final_2nd_nump = second_nump;

   // And scan the dragged people (who aren't in the result setup)
   // to find out how to glue them to the real result people.

   for (i=0; i<nump; i++) {
      if (matrix_info[i].sel) {
         // Get the actual dragger person id1 word.
         uint32 dragger_id = scopy.people[matrix_info[i].deltarot].id1;

         // Find the XY coords of the person's dragger.

         int kk;
         for (kk=0; kk<second_nump; kk++) {
            if (((second_matrix_info[kk].id1 ^ dragger_id) & (PID_MASK|BIT_PERSON)) == 0)
               goto found_dragger;
         }
         fail("Internal error: failed to find dragger coords.");
      found_dragger:
         // Original offset of draggee relative to dragger.
         int origdx = matrix_info[i].x - matrix_info[i].deltax;
         int origdy = matrix_info[i].y - matrix_info[i].deltay;
         // Find out how much the dragger turned while doing the call.
         // The "before" space is scopy and the "after" space is result,
         // so this doesn't necessarily relate to actual turning.
         int dragger_turn = (second_matrix_info[kk].dir - matrix_info[i].nearestdrag) & 3;
         if (dragger_turn & 2) {
            origdx = -origdx;
            origdy = -origdy;
         }
         if (dragger_turn & 1) {
            int temp = origdx;
            origdx = origdy;
            origdy = -temp;
         }
         // Now origdx/dy has offset of draggee from dragger in new space.
         // This is new info for draggee.
         copy_rot(&second_people, final_2nd_nump, &people, i, dragger_turn*011);
         second_matrix_info[final_2nd_nump] = matrix_info[i];
         second_matrix_info[final_2nd_nump].x = second_matrix_info[kk].x + origdx;
         second_matrix_info[final_2nd_nump++].y = second_matrix_info[kk].y + origdy;
      }
   }

   for (i=0; i<final_2nd_nump; i++) {
      second_matrix_info[i].deltax = 0;
      second_matrix_info[i].deltay = 0;
      second_matrix_info[i].deltarot = 0;
   }

   ss->rotation += result->rotation;
   finish_matrix_call(second_matrix_info, final_2nd_nump, true, collision_severity_no, true, merge_strict_matrix, &second_people, result);
   reinstate_rotation(ss, result);
   clear_result_flags(result);
}



extern void anchor_someone_and_move(
   setup *ss,
   parse_block *parseptr,
   setup *result)  THROW_DECL
{
   enum { MAX_GROUPS = 12 };
   setup people;
   matrix_rec before_matrix_info[9];
   matrix_rec after_matrix_info[9];
   int i, j, k, nump;
   int deltax[MAX_GROUPS], deltay[MAX_GROUPS];
   selector_kind saved_selector = current_options.who;
   setup saved_start_people = *ss;
   int Bindex[MAX_GROUPS];
   int Aindex[MAX_GROUPS];
   int Eindex[MAX_GROUPS];

   current_options.who = parseptr->options.who;

   if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
      fail("Can't split the setup.");

   if (ss->cmd.cmd_misc_flags & CMD_MISC__DISTORTED)
      fail("This call not allowed in distorted or virtual setup.");

   for (i=0 ; i<MAX_GROUPS ; i++) { Eindex[i] = Bindex[i] = Aindex[i] = -1; }

   ss->rotation = 0;
   ss->eighth_rotation = 0;

   if (ss->kind != s2x4 &&
       ss->kind != s1x8 &&
       ss->kind != s2x3 &&
       ss->kind != s2x6 &&
       ss->kind != s2x8 &&
       ss->kind != s2x12 &&
       ss->kind != s3x4 &&
       ss->kind != s4x4 &&
       ss->kind != s4x6 &&
       ss->kind != s_qtag &&
       ss->kind != s_ptpd)
      fail("Sorry, can't do this in this setup.");
   move(ss, false, result);

   nump = start_matrix_call(&saved_start_people, before_matrix_info, 0,
                            MTX_USE_SELECTOR, &people);
   current_options.who = saved_selector;

   int numgroups = 0;
   uint32 splitting_indicator = (result->result_flags.split_info[1] << 4) | result->result_flags.split_info[0];
   bool we_have_a_problem = false;

   switch (splitting_indicator) {
   case 1:       // Single split on X.
   case 0x10:    // Single split on Y.
      numgroups = 2;
      break;
   case 0x11:    // Single split on both X and Y.
   case 3:       // Triple split on X.
      numgroups = 4;
      break;
   case 0x12:    // Double split on X, single on Y.
   case 0x21:    // Double split on Y, single on X.
      numgroups = 6;
      break;
   case 0x13:    // Triple split on X, single on Y.
   case 0x31:    // Triple split on Y, single on X.
      numgroups = 8;
      break;
   case 0x15:    // Quintuple split on X, single on Y.
   case 0x51:    // Quintuple split on Y, single on X.
      numgroups = 12;
      break;
   default:
      we_have_a_problem = true;
   }

   if (we_have_a_problem) {
      if (ss->kind != s4x4)
         fail("Can't 'anchor' someone for an 8-person call.");

      // Needs 48-person setups!
      divided_setup_move(ss, MAPCODE(s1x4,4,MPKIND__SPLIT,1), phantest_ok, true, result);
      fail("Can't 'anchor' someone for an 8-person call.");
   }

   for (i=0 ; i<nump ; i++) {
      int x = before_matrix_info[i].x;
      int y = before_matrix_info[i].y;

      // Look at both fields simultaneously.

      switch (splitting_indicator) {
      case 1:       // Single split on X.
         if (x < 0) j = 0;
         else j = 1;
         break;
      case 0x10:    // Single split on Y.
         if (y < 0) j = 0;
         else j = 1;
         break;
      case 0x11:    // Single split on both X and Y.
         if (x < 0) j = 0;
         else j = 2;
         if (y < 0) j++;
         break;
      case 3:       // Triple split on X.
         // This is split into four groups in a row from left to right.
         if      (x < -setup_attrs[ss->kind].bounding_box[0]) j = 0;
         else if (x < 0) j = 1;
         else if (x < setup_attrs[ss->kind].bounding_box[0]) j = 2;
         else j = 3;
         break;
      case 0x12:    // Double split on X, single on Y.
         // This is split into 2 groups vertically and 3 groups laterally.
         if (y < 0) j = 0;
         else j = 3;
         if (x+x/2 > setup_attrs[ss->kind].bounding_box[0]) j += 2;
         else if (x+x/2 >= -setup_attrs[ss->kind].bounding_box[0]) j += 1;
         break;
      case 0x21:    // Double split on Y, single on X.
         // This is split into 3 groups vertically and 2 groups laterally.
         if (x < 0) j = 0;
         else j = 3;
         if (y+y/2 > setup_attrs[ss->kind].bounding_box[1]) j += 2;
         else if (y+y/2 >= -setup_attrs[ss->kind].bounding_box[1]) j += 1;
         break;
      case 0x13:    // Triple split on X, single on Y.
         // This is split into 2 groups vertically and 4 groups laterally.
         if (y < 0) j = 0;
         else j = 4;
         if      (x < -setup_attrs[ss->kind].bounding_box[0]) j += 3;
         else if (x < 0) j += 2;
         else if (x < setup_attrs[ss->kind].bounding_box[0]) j += 1;
         break;
      case 0x31:    // Triple split on Y, single on X.
         // This is split into 4 groups vertically and 2 groups laterally.
         if (x < 0) j = 0;
         else j = 4;
         if      (y < -setup_attrs[ss->kind].bounding_box[0]) j += 3;
         else if (y < 0) j += 2;
         else if (y < setup_attrs[ss->kind].bounding_box[0]) j += 1;
         break;
      case 0x15:    // Quintuple split on X, single on Y.
         // This is split into 2 groups vertically and 6 groups laterally.
         if (y < 0) j = 0;
         else j = 6;
         if      (x < -2*setup_attrs[ss->kind].bounding_box[0]) j += 5;
         if      (x < -setup_attrs[ss->kind].bounding_box[0]) j += 4;
         else if (x < 0) j += 3;
         else if (x < setup_attrs[ss->kind].bounding_box[0]) j += 2;
         else if (x < 2*setup_attrs[ss->kind].bounding_box[0]) j += 1;
         break;
      case 0x51:    // Quintuple split on Y, single on X.
         // This is split into 6 groups vertically and 2 groups laterally.
         if (x < 0) j = 0;
         else j = 6;
         if      (y < -2*setup_attrs[ss->kind].bounding_box[0]) j += 5;
         if      (y < -setup_attrs[ss->kind].bounding_box[0]) j += 4;
         else if (y < 0) j += 3;
         else if (y < setup_attrs[ss->kind].bounding_box[0]) j += 2;
         else if (y < 2*setup_attrs[ss->kind].bounding_box[0]) j += 1;
         break;
      }

      Eindex[j] = 99;    // We have someone in this group.

      // The "nearestdrag" field tells what anchored group the person is in.
      before_matrix_info[i].nearestdrag = j;

      if (before_matrix_info[i].sel) {
         if (Bindex[j] >= 0) fail("Need exactly one 'anchored' person in each group.");
         Bindex[j] = i;
      }
   }

   for (k=0 ; k<numgroups ; k++) {
      if (Eindex[k] > 0 && Bindex[k] < 0)
         fail("Need exactly one 'anchored' person in each group.");
   }

   // If the result is a "1p5x8", we do special stuff.
   if (result->kind == s1p5x8) result->kind = s2x8;
   else if (result->kind == s1p5x4) result->kind = s2x4;

   nump = start_matrix_call(result, after_matrix_info, 0, 0, &people);

   for (i=0 ; i<nump ; i++) {
      if (result->rotation) {
         int t = after_matrix_info[i].x;
         after_matrix_info[i].x = after_matrix_info[i].y;
         after_matrix_info[i].y = -t;
      }

      after_matrix_info[i].deltarot += result->rotation;

      for (k=0 ; k<numgroups ; k++) {
         if (((after_matrix_info[i].id1 ^ before_matrix_info[Bindex[k]].id1) & XPID_MASK) == 0)
            Aindex[k] = i;
      }
   }

   for (k=0 ; k<numgroups ; k++) {
      if (Eindex[k] > 0) {
         if (Aindex[k] < 0) fail("Sorry6.");
         deltax[k] = before_matrix_info[Bindex[k]].x - after_matrix_info[Aindex[k]].x;
         deltay[k] = before_matrix_info[Bindex[k]].y - after_matrix_info[Aindex[k]].y;
      }
   }

   for (i=0 ; i<nump ; i++) {
      // Find this person's group.

      for (k=0 ; k<nump ; k++) {
         if (((after_matrix_info[i].id1 ^ before_matrix_info[k].id1) & XPID_MASK) == 0) {
            j = before_matrix_info[k].nearestdrag;
            goto found_person;
         }
      }
      fail("Can't find where this person went.");

      found_person:

      after_matrix_info[i].deltax = deltax[j];
      after_matrix_info[i].deltay = deltay[j];
      after_matrix_info[i].dir = 0;
   }

   finish_matrix_call(after_matrix_info, nump, false, collision_severity_no, true, merge_strict_matrix, &people, result);
   reinstate_rotation(&saved_start_people, result);
   clear_result_flags(result);
}


static void rollmove(
   setup *ss,
   const calldefn *callspec,
   setup *result) THROW_DECL
{
   int i;

   if (attr::slimit(ss) < 0) fail("Can't roll in this setup.");

   result->kind = ss->kind;
   result->rotation = ss->rotation;
   result->eighth_rotation = ss->eighth_rotation;

   for (i=0; i<=attr::slimit(ss); i++) {
      if (ss->people[i].id1) {
         int rot = 0;
         uint32 st = STB_NONE+STB_REVERSE;

         if (!(callspec->callflagsf & CFLAGH__REQUIRES_SELECTOR) || selectp(ss, i)) {
            switch (ss->people[i].id1 & ROLL_DIRMASK) {
            case ROLL_IS_R:
               rot = 011;
               st = STB_A+STB_REVERSE;
               break;
            case ROLL_IS_L:
               rot = 033;
               st = STB_A;
               break;
            case ROLL_DIRMASK: break;
            default: fail("Roll not permitted after previous call.");
            }
         }

         install_rot(result, i, ss, i, rot);

         if (result->people[i].id1 & STABLE_ENAB) {
            do_stability(&result->people[i].id1, st, rot, false);
         }
      }
      else
         result->clear_person(i);
   }
}


static uint32 fix_gensting_weirdness(const setup_command *cmd, uint32 callflagsh)
{
   if (cmd->cmd_final_flags.test_heritbits(INHERITFLAG_YOYOETCMASK) == INHERITFLAG_YOYOETCK_STINGY) {
      // User gave "stingy".  Because we are cheating with these bits, special action is needed.
      if (callflagsh & INHERITFLAG_YOYOETCK_GENEROUS)
         callflagsh |= INHERITFLAG_YOYOETCK_STINGY;
   }
   return callflagsh;
}



static void do_inheritance(setup_command *cmd,
                           const calldefn *parent_call,
                           const by_def_item *defptr,
                           uint32 extra_heritmask_bits) THROW_DECL
{
   // Strip out those concepts that do not have the "dfm__xxx" flag set saying that
   // they are to be inherited to this part of the call.  BUT: the "INHERITFLAG_LEFT"
   // flag controls both "INHERITFLAG_REVERSE" and "INHERITFLAG_LEFT", turning the former
   // into the latter.  This makes reverse circle by, touch by, and clean sweep work.

   // If this subcall has "inherit_reverse" or "inherit_left" given, but the top-level call
   // doesn't permit the corresponding flag to be given, we should turn any "reverse" or
   // "left" modifier that was given into the other one, and cause that to be inherited.
   // This is what turns, for example, part 3 of "*REVERSE* clean sweep" into a "*LEFT*
   // 1/2 tag".

   uint32 callflagsh = parent_call->callflagsh;
   uint32 temp_concepts = cmd->cmd_final_flags.herit;
   uint32 forcing_concepts = defptr->modifiersh & ~callflagsh;

   if (forcing_concepts & (INHERITFLAG_REVERSE | INHERITFLAG_LEFT)) {
      if (cmd->cmd_final_flags.test_heritbits(INHERITFLAG_REVERSE | INHERITFLAG_LEFT))
         temp_concepts |= (INHERITFLAG_REVERSE | INHERITFLAG_LEFT);
   }

   // Pass any "inherit" flags.  That is, turn off any that are NOT to be inherited.
   // Flags to be inherited are indicated by "modifiersh" and "callflagsh" both on.
   // But we don't check "callflagsh", since, if it is off, we will force the bit
   // immediately below.

   // Fix special case of yoyo/generous/stingy.  HALF and LASTHALF are always considered to be heritable.
   uint32 hhhh = fix_gensting_weirdness(cmd, defptr->modifiersh | INHERITFLAG_HALF | INHERITFLAG_LASTHALF);

   temp_concepts &= (~cmd->cmd_final_flags.herit | hhhh | extra_heritmask_bits);

   // Now turn on any "force" flags.  These are indicated by "modifiersh" on
   // and "callflagsh" off.

   if (temp_concepts & defptr->modifiersh & ~callflagsh & (INHERITFLAG_HALF | INHERITFLAG_LASTHALF))
      fail("Can't do this with this fraction.");   // "force_half" was used when we already had "half" coming in.

   if (((INHERITFLAG_REVERSE | INHERITFLAG_LEFT) & callflagsh) == 0)
      // If neither of the "reverse_means_mirror" or "left_means_mirror" bits is on,
      // we allow forcing of left or reverse.
      temp_concepts |= forcing_concepts;
   else
      // Otherwise, we only allow the other bits.
      temp_concepts |= forcing_concepts & ~(INHERITFLAG_REVERSE | INHERITFLAG_LEFT);

   cmd->cmd_final_flags.herit = (heritflags) temp_concepts;
   cmd->callspec = base_calls[defptr->call_id];
}

extern void process_number_insertion(uint32 mod_word)
{
   if (mod_word & DFM1_NUM_INSERT_MASK) {
      uint32 insertion_num = (mod_word & DFM1_NUM_INSERT_MASK) / DFM1_NUM_INSERT_BIT;

      if ((mod_word & DFM1_FRACTAL_INSERT) && (insertion_num & ~2) == 1)
         insertion_num ^= 2;

      current_options.number_fields <<= BITS_PER_NUMBER_FIELD;
      current_options.number_fields += insertion_num;
      current_options.howmanynumbers++;
   }

   current_options.number_fields >>=
      ((DFM1_NUM_SHIFT_MASK & mod_word) / DFM1_NUM_SHIFT_BIT) * BITS_PER_NUMBER_FIELD;
   current_options.howmanynumbers -= ((DFM1_NUM_SHIFT_MASK & mod_word) / DFM1_NUM_SHIFT_BIT);
}



extern bool get_real_subcall(
   parse_block *parseptr,
   const by_def_item *item,
   const setup_command *cmd_in,
   const calldefn *parent_call,
   bool forbid_flip,
   uint32 extra_heritmask_bits,
   setup_command *cmd_out) THROW_DECL   // We fill in just the parseptr, callspec, cmd_final_flags fields.
{
   parse_block *search;
   parse_block **newsearch;
   int snumber;
   int item_id = item->call_id;
   // The flags, heritable and otherwise, with which the parent call was invoked.
   // Some of these may be inherited to the subcall.
   final_and_herit_flags new_final_concepts = cmd_in->cmd_final_flags;
   uint32 mods1 = item->modifiers1;
   call_with_name *orig_call = base_calls[item_id];
   bool this_is_tagger =
      item_id >= base_call_tagger0 && item_id < base_call_tagger0+NUM_TAGGER_CLASSES;
   bool this_is_tagger_circcer = this_is_tagger || item_id == base_call_circcer;
   call_conc_option_state save_state = current_options;

   if (!(new_final_concepts.test_heritbit(INHERITFLAG_FRACTAL)))
      mods1 &= ~DFM1_FRACTAL_INSERT;

   process_number_insertion(mods1);

   // Fill in defaults in case we choose not to get a replacement call.

   cmd_out->parseptr = parseptr;
   cmd_out->cmd_final_flags = new_final_concepts;

   // If this context requires a tagging or scoot call, pass that fact on.
   if (this_is_tagger) cmd_out->cmd_final_flags.set_finalbit(FINAL__MUST_BE_TAG);

   do_inheritance(cmd_out, parent_call, item, extra_heritmask_bits);

   // Do the substitutions called for by star turn replacements (from "@S" escape codes.)

   if (current_options.star_turn_option != 0 &&
       (orig_call->the_defn.callflagsf & CFLAG2_IS_STAR_CALL)) {
      parse_block *xx = get_parse_block();
      xx->concept = &concept_marker_concept_mod;
      xx->options = current_options;
      xx->options.star_turn_option = 0;
      xx->replacement_key = 0;
      xx->call = orig_call;
      xx->options.number_fields &= ~NUMBER_FIELD_MASK;
      xx->no_check_call_level = true;
      xx->call_to_print = xx->call;

      if (current_options.star_turn_option >= 0)
         xx->options.number_fields += current_options.star_turn_option;

      cmd_out->parseptr = xx;
      cmd_out->callspec = (call_with_name *) 0;
      goto ret_true;
   }

   /* If this subcall invocation does not permit modification under any value of the
      "allowing_modifications" switch, we do nothing.  Just return the default.
      We do not search the list.  Hence, such subcalls are always protected
      from being substituted, and, if the same call appears multiple times
      in the derivation tree of a compound call, it will never be replaced.
      What do we mean by that?  Suppose we did a
         "[tally ho but [2/3 recycle]] the difference".
      After we create the table entry saying to change cast off 3/4 into 2/3
      recycle, we wouldn't want the cast off's in the difference getting
      modified.  Actually, that is not the real reason.  The casts are in
      different sublists.  The real reason is that the final part of mixed-up
      square thru is defined as
         conc nullcall [mandatory_anycall] nullcall []
      and we don't want the unmodifiable nullcall that the ends are supposed to
      do getting modified, do we? */

   /* But if this is a tagging call substitution, we most definitely do
      proceed with the search. */

   if (!(mods1 & DFM1_CALL_MOD_MASK) && !this_is_tagger_circcer)
      goto ret_false;

   /* See if we had something from before.  This avoids embarassment if a call is actually
      done multiple times.  We want to query the user just once and use that result everywhere.
      We accomplish this by keeping a subcall list showing what modifications the user
      supplied when we queried. */

   snumber = (mods1 & DFM1_CALL_MOD_MASK) / DFM1_CALL_MOD_BIT;

   /* We ALWAYS search this list, if such exists.  Even if modifications are disabled.
      Why?  Because the reconciler re-executes calls after the point of insertion to test
      their fidelity with the new setup that results from the inserted calls.  If those
      calls have modified subcalls, we will find ourselves here, looking through the list.
      Modifications may have been enabled at the time the call was initially entered, but
      might not be now that we are being called from the reconciler. */

   newsearch = &parseptr->next;

   if (parseptr->concept->kind == concept_another_call_next_mod) {
      if (snumber == (DFM1_CALL_MOD_MAND_ANYCALL/DFM1_CALL_MOD_BIT) &&
          (cmd_in->cmd_misc3_flags & CMD_MISC3__NO_ANYTHINGERS_SUBST) &&
          item_id == base_call_circulate) {
         goto ret_false;
      }
      else if (snumber == (DFM1_CALL_MOD_ANYCALL/DFM1_CALL_MOD_BIT) &&
          (cmd_in->cmd_misc3_flags & CMD_MISC3__NO_ANYTHINGERS_SUBST) &&
               item_id == base_call_circcer) {
         cmd_out->callspec = base_calls[base_call_circulate];
         goto ret_true;
      }

      while ((search = *newsearch) != (parse_block *) 0) {
         if (orig_call == search->call ||
             (snumber == (DFM1_CALL_MOD_MAND_ANYCALL/DFM1_CALL_MOD_BIT) &&
              search->call == base_calls[base_call_null] && orig_call == base_calls[base_call_circulate]) ||
             (this_is_tagger && search->call == base_calls[base_call_tagger0])) {
            // Found a reference to this call.
            parse_block *subsidiary_ptr = search->subsidiary_root;

            // If the pointer is nil, we already asked about this call, and the reply was no.
            if (!subsidiary_ptr)
               goto ret_false;

            cmd_out->parseptr = subsidiary_ptr;

            // This makes it pick up the substituted call.
            cmd_out->callspec = (call_with_name *) 0;

            if (this_is_tagger_circcer) {
               if (forbid_flip && subsidiary_ptr->call == base_calls[base_call_flip])
                  fail_no_retry("Don't say 'flip back to a wave' -- put a comma"
                       " after the 'back' if that's what you want.");
            }
            else {
               // Substitutions are normal, and hence we clear the modifier bits,
               // unless this was a "mandatory_anycall" or "mandatory_secondary_call",
               // in which case we assume that the database tells just what bits
               // to inherit.
               if (snumber == (DFM1_CALL_MOD_MAND_ANYCALL/DFM1_CALL_MOD_BIT) ||
                   snumber == (DFM1_CALL_MOD_MAND_SECONDARY/DFM1_CALL_MOD_BIT)) {
                  if (search->concept->kind == concept_supercall) {
                     cmd_out->cmd_final_flags.herit = (heritflags) cmd_in->restrained_super8flags;
                     cmd_out->cmd_misc2_flags &= ~CMD_MISC2_RESTRAINED_SUPER;
                  }
               }
               else {
                  // ****** not right????.
                  cmd_out->cmd_final_flags.clear_all_herit_and_final_bits();
               }
            }

            goto ret_true;
         }

         newsearch = &search->next;
      }
   }
   else if (parseptr->concept->kind != marker_end_of_list)
      fail_no_retry("wrong marker in get_real_subcall???");

   if (do_subcall_query(snumber, parseptr, newsearch,
                        this_is_tagger, this_is_tagger_circcer, orig_call))
      goto ret_false;

   cmd_out->parseptr = (*newsearch)->subsidiary_root;
   // We THROW AWAY the alternate call, because we want our user
   // to get it from the concept list.
   cmd_out->callspec = (call_with_name *) 0;
   cmd_out->cmd_final_flags.clear_all_herit_and_final_bits();

 ret_true:
   current_options = save_state;
   return true;

 ret_false:

   current_options = save_state;
   return false;
}


static void divide_diamonds(setup *ss, setup *result) THROW_DECL
{
   uint32 ii;

   if (ss->kind == s3x4) expand::compress_setup(s_qtg_3x4, ss);

   if (ss->kind == s_qtag) ii = MAPCODE(sdmd,2,MPKIND__SPLIT,1);
   else if (ss->kind == s_ptpd) ii = MAPCODE(sdmd,2,MPKIND__SPLIT,0);
   else fail("Must have diamonds for this concept.");

   divided_setup_move(ss, ii, phantest_ok, false, result);
}


// a <= b
extern int gcd(int a, int b)
{
   for (;;) {
      int rem;
      if (a==0) return b;
      rem = b % a;
      if (rem==0) return a;
      b = a;
      a = rem;
   }
}


/* The fraction stuff is encoded into 6 hexadecimal digits, which we will call
   digits 3 through 8:

      Late-breaking news:  digit 2 is now used.  The bit 0x01000000 means
      "this is 'initially' or 'finally', and I promise to invoke all parts
      of the call, even though I appear at present to be invoking just a
      subset."  It is used to allow 'initially' and 'finally' to be stacked
      with themselves.

      digit 3 - the code (3 bits) and the reversal bit (1 bit)
                  Note that the code*2 is what is visible in this digit.
                  If this digit is odd, the reversal bit is on.  The code
                  indicates what kind of special job we are doing.
      digit 4 - the selected part, called "N".  Think of this as being 1-based,
                  even though the part numbering stuff that comes out of this
                  procedure will be zero-based.  This will always be nonzero
                  when some nontrivial job is being done.  the meaning of the
                  codes has been defined so that no job ever requires N to be zero.
                  The only time N is zero is when we are not doing anything, in
                  which case the code is zero.  Code = N = 0 means no special
                  job is being done.  (Though reversal and nontrivial fractions
                  may still be present.)
      digits 5 and 6 - the numerator and denominator, respectively, of the
                  start point of the call, always in reduced-fraction form.
                  The default is [0,1] meaning to start at 0.0000, that is, the
                  beginning.
      digits 7 and 8 - the numerator and denominator, respectively, of the
                  end point of the call, always in reduced-fraction form.
                  The default is [1,1] meaning to end at 1.0000, that is, the
                  end.

   The default configuration is 000111 hex, meaning do not reverse, go from
   0.0000 to 1.0000, and don't do any special job.  A word of zero means the same
   thing, and the fraction flag word is initialized to 0 if no fractions are being
   used.  This makes it easier to tell whether any fractions are being used.
   The zero is changed to 000111 hex at the start of any fraction-manipulation
   operation.

   The meaning of all this stuff is as follows:

   First, if the reversal bit is on, the target call (or target random crazy
   call, or whatever) has its parts reversed.

   Second, the fraction stuff, determined by the low 4 hexadecimal digits, is
   applied to the possibly reversed call.  This creates the "region".

   Third, the region is manipulated according to the key and the value of N.

   Before describing the key actions, here is how the region is computed.

      Suppose the call has 10 parts: A, B, C, D, E, F, G, H, I, and J, and that
      the low 4 digits of the fraction word are 1535.  This means that the start
      point is 1/5 and the end point is 3/5.  So the first 1/5 of the call (2 parts,
      A and B) are chopped off, and the last 2/5 of the call (4 parts, G, H, I, and J)
      are also chopped off.  The region is C, D, E, and F.  This effect could be
      obtained by saying "do the last 2/3, 3/5, <call>" or "1/2, do the last 4/5, <call>".

      Now suppose that the "reverse" bit is also on.  This reverses the parts BEFORE
      the fractions are computed.  The call is considered to consist of J, I, H, G, F,
      E, D, C, B, and A.  If the low 4 digits are 1535, this chops off J and I at
      the beginning, and D, C, B, and A at the end.  The region is thus H, G, F, and E.
      This could be obtained by saying "do the last 2/3, 3/5, reverse order, <call>",
      or "reverse order, 2/3, do the last 3/5, <call>", or "1/2, do the last 4/5,
      reverse order, <call>", etc.

   What happens next depends on the key and the value of "N", but always applies just
   to the region as though the region were the only thing under consideration.

   If "N" is zero (and the key is zero also, of course), the region is simply
   executed from beginning to end.

   Otherwise, the parts of the region are considered to be numbered in a 1-based
   fashion.  In the case above of the "reverse" bit being on, the low 4 digits being
   1535 hex, and the call having 10 parts, the region was H, G, F, and E.  These
   are numbered:
      H = 1
      G = 2
      F = 3
      E = 4

   Let T be the number of parts in the region.  In this example, T is 4.

   The meanings of the key values are as follows.  These must only be used
      when "N" (the selected part, the stuff in CMD_FRAC_PART_MASK) is nonzero!
      When "N" is zero, the key must be zero (which happens to be CMD_FRAC_CODE_ONLY,
      but it doesn't matter.)

      CMD_FRAC_CODE_ONLY
               Do part N only.  Do 1 part.  For a 5-part call ABCDE:
                  ONLY(1) means do A
                  ONLY(4) means do D
         (When N is zero, the whole word must be zero,
         and it means that this mechanism is not being used.)

      CMD_FRAC_CODE_ONLYREV
               Do part N from the end.  Do 1 part.  For a 5-part call ABCDE:
                  ONLYREV(1) means do E
                  ONLYREV(4) means do B

      CMD_FRAC_CODE_FROMTO
                  do parts from K+1 up through N, inclusive,
                  that is, K+1, K+2, K+3, .... N.  Relative to the region.
                  K is the "secondary part number".  If K=0, this does N parts:
                  from the beginning of the region up through N, inclusive.
                  For a 5-part call ABCDE:
                  FROMTO(K=0,N=1) means do A
                  FROMTO(K=0,N=4) means do ABCD
                  FROMTO(K=1,N=4) means do BCD

      CMD_FRAC_CODE_FROMTOREV
               Do parts from N up through size-K, inclusive.
               That is, skip N-1 parts at the start, do the main body in forward order,
               and skip K parts at the end.
               For a 5-part call ABCDE:
                  FROMTOREV(K=0,N=1) means do whole thing
                  FROMTOREV(K=0,N=2) means do BCDE
                  FROMTOREV(K=0,N=5) means do E
                  FROMTOREV(K=1,N=1) means do ABCD
                  FROMTOREV(K=1,N=2) means do BCD
                  FROMTOREV(K=3,N=2) means do B
                  FROMTOREV(K=4,N=1) means do A

      CMD_FRAC_CODE_FROMTOREVREV
               Do parts from size-N-1 up through size-K, inclusive.
               Do N-K parts -- the last N parts, but omitting the last K parts.
               For a 5-part call ABCDE:
                  FROMTOREVREV(K=0,N=5) means do whole thing
                  FROMTOREVREV(K=0,N=4) means do BCDE
                  FROMTOREVREV(K=0,N=1) means do E
                  FROMTOREVREV(K=1,N=5) means do ABCD
                  FROMTOREVREV(K=1,N=4) means do BCD
                  FROMTOREVREV(K=3,N=4) means do B
                  FROMTOREVREV(K=4,N=5) means do A

      CMD_FRAC_CODE_FROMTOMOST
                  Same as FROMTO, but leave early from the last part.
                  Do just the first half of it.
                  For a 5-part call ABCDE:
                  FROMTO(K=1,N=4) means do BC and the first half of D

      CMD_FRAC_CODE_LATEFROMTOREV
                  Same as FROMTOREV, but get a late start on the first part.
                  Do just the last half of it.
               For a 5-part call ABCDE:
                  LATEFROMTOREV(K=1,N=2) means do the last half of B, then CD

   The output of this procedure, after digesting the above, is a "fraction_info"
   structure, whose important items are:

      m_fetch_index - the first part of the target call that we are to execute.
         This is now in ZERO-BASED numbering, as befits the sequential-definition
         array.
      m_highlimit - if going forward, this is the (zero-based) part JUST AFTER the
         last part that we are to execute.  We will execute (m_highlimit-m_fetch_index)
         parts.  If going backward, this is the (zero-based) LAST PART that we will
         execute.  We will execute (m_fetch_index-m_highlimit+1) parts.  Note the
         asymmetry.  Sorry.

   So, to execute all of a 10 part call in forward order, we will have
      m_fetch_index = 0    and    m_highlimit = 10
   To execute it in reverse order, we will have
      m_fetch_index = 9    and    m_highlimit = 0

   The "m_instant_stop" and "m_do_half_of_last_part" flags do various things.
*/


// This computes "incoming_fracs of arg_fracs".  That is, it composes
// two fraction specs.  For example, if incoming_fracs=1st half (1,0,2,1)
// and arg_fracs=last half (2,1,1,1), this calculates the first half of the last
// half, which is the part from 1/2 to 3/4 (2,1,4,3).
// If the user nests fraction concepts as in
//          "1/2, do the last 1/2, split the difference",
// we will process the first concept (1/2) and set the cmd_frac_flags to 1,0,2,1.
// We will then see the second concept (do the last 1/2) and call this
// function with incoming_fracs = the cmd_frac_flags field we have so far,
// and start+end = this concept.
//
// If the incoming_fracs argument has the CMD_FRAC_REVERSE flag on, we
// presumably saw something like "1/2, reverse order", and are trying to
// do "1/2, reverse order, do the last 1/2, split the difference".
// In that case, this function returns
//      "incoming_fracs of reverse order of arg_fracs",
// that is, the first 1/4 (1,0,4,1).  If the client retains the
// CMD_FRAC_REVERSE flag on that, it will do the last 1/4 of the call.
extern uint32 process_fractions(int start, int end,
                                fraction_invert_flags invert_flags,
                                const fraction_command & incoming_fracs,
                                bool make_improper /* = false */,
                                bool *improper_p /* = 0 */) THROW_DECL
{
   int cn = start & NUMBER_FIELD_MASK;
   int cd = (start >> BITS_PER_NUMBER_FIELD) & NUMBER_FIELD_MASK;
   int dn = end & NUMBER_FIELD_MASK;
   int dd = (end >> BITS_PER_NUMBER_FIELD) & NUMBER_FIELD_MASK;

   if (invert_flags & FRAC_INVERT_START) cn = cd-cn;
   if (invert_flags & FRAC_INVERT_END) dn = dd-dn;

   int numer = dn*cd-cn*dd;
   int denom = dd*cd;
   int P = cn*dd;

   if (incoming_fracs.flags & CMD_FRAC_REVERSE)
      P = (dd-dn)*cd;

   // Check that the user isn't doing something stupid.
   if (numer <= 0 || numer >= denom)
      fail("Illegal fraction.");

   // If being asked to do "1-M/N", make the fraction improper.
   if (make_improper) numer += denom;

   int s_denom = (incoming_fracs.fraction >> (BITS_PER_NUMBER_FIELD*3)) & NUMBER_FIELD_MASK;
   int s_numer = (incoming_fracs.fraction >> (BITS_PER_NUMBER_FIELD*2)) & NUMBER_FIELD_MASK;
   int e_denom = (incoming_fracs.fraction >> BITS_PER_NUMBER_FIELD) & NUMBER_FIELD_MASK;
   int e_numer = (incoming_fracs.fraction & NUMBER_FIELD_MASK);

   s_numer = P*s_denom+numer*s_numer;
   e_numer = P*e_denom+numer*e_numer;

   s_denom *= denom;
   e_denom *= denom;

   if (make_improper && e_numer > e_denom) {
      if (improper_p) *improper_p = true;
      e_numer -= e_denom;
   }

   if (s_numer < 0 || s_numer >= s_denom || e_numer <= 0 || e_numer > e_denom)
      fail("Illegal fraction.");

   int divisor = gcd(s_numer, s_denom);
   s_numer /= divisor;
   s_denom /= divisor;

   divisor = gcd(e_numer, e_denom);
   e_numer /= divisor;
   e_denom /= divisor;

   if (s_numer > NUMBER_FIELD_MASK || s_denom > NUMBER_FIELD_MASK ||
       e_numer > NUMBER_FIELD_MASK || e_denom > NUMBER_FIELD_MASK)
      fail("Fractions are too complicated.");

   return (s_denom<<(BITS_PER_NUMBER_FIELD*3)) |
      (s_numer<<(BITS_PER_NUMBER_FIELD*2)) |
      (e_denom<<BITS_PER_NUMBER_FIELD) |
      e_numer;
}




void fraction_info::get_fraction_info(
   fraction_command frac_stuff,
   uint32 part_visibility_info,
   revert_weirdness_type doing_weird_revert,
   parse_block **restrained_concept_p /* = (parse_block **) 0 */ ) THROW_DECL
{
   uint32 last_half_stuff = 0;
   uint32 first_half_stuff = 0;

   int available_initial_fractions = 0;
   int available_final_fractions = 0;

   if (frac_stuff.flags & CMD_FRAC_FORCE_VIS) {
      available_initial_fractions = 1000;
      available_final_fractions = 1000;
   }

   switch (part_visibility_info) {
   case 0:
      break;
   case 1:    // first_part_visible
      available_initial_fractions = 1;
      break;
   case 2:    // first_two_parts_visible
      available_initial_fractions = 2;
      break;
   case 3:    // last_part_visible
      available_final_fractions = 1;
      break;
   case 4:    // last_two_parts_visible
      available_final_fractions = 2;
      break;
   case 5:    // first_and_last_parts_visible
      available_initial_fractions = 1;
      available_final_fractions = 1;
      break;
   default:   // visible_fractions
      available_initial_fractions = 1000;
      available_final_fractions = 1000;
      break;
   }

   m_reverse_order = false;
   m_instant_stop = 0;
   m_do_half_of_last_part = (fracfrac) 0;
   m_do_last_half_of_first_part = (fracfrac) 0;

   int this_part = (frac_stuff.flags & CMD_FRAC_PART_MASK) / CMD_FRAC_PART_BIT;
   int s_denom = (frac_stuff.fraction >> (BITS_PER_NUMBER_FIELD*3)) & NUMBER_FIELD_MASK;
   int s_numer = (frac_stuff.fraction >> (BITS_PER_NUMBER_FIELD*2)) & NUMBER_FIELD_MASK;
   int e_denom = (frac_stuff.fraction >> BITS_PER_NUMBER_FIELD) & NUMBER_FIELD_MASK;
   int e_numer = (frac_stuff.fraction & NUMBER_FIELD_MASK);

   if (s_numer >= s_denom) fail("Fraction must be proper.");
   int my_start_point = m_client_total * s_numer;
   int test_num = my_start_point / s_denom;

   if (test_num*s_denom != my_start_point) {
      int divisor = (test_num == 0) ?
         gcd(my_start_point, s_denom) :
         gcd(s_denom, my_start_point);
      s_denom /= divisor;
      my_start_point /= divisor;
      last_half_stuff = my_start_point - test_num * s_denom;   /* We will need this if we have
                                                                  to reverse the order. */
      m_do_last_half_of_first_part = (fracfrac)
         ((last_half_stuff << (BITS_PER_NUMBER_FIELD*2)) |
          (s_denom << (BITS_PER_NUMBER_FIELD*3)) |
          NUMBER_FIELDS_1_1);
      if (m_do_last_half_of_first_part != FRAC_FRAC_LASTHALF_VALUE)
         warn(warn_hairy_fraction);
   }

   my_start_point = test_num;

   if (e_numer <= 0) fail("Fraction must be proper.");
   m_highlimit = m_client_total * e_numer;
   test_num = m_highlimit / e_denom;

   if (test_num*e_denom != m_highlimit) {
      int divisor = (test_num == 0) ?
         gcd(m_highlimit, e_denom) :
         gcd(e_denom, m_highlimit);
      m_highlimit /= divisor;
      e_denom /= divisor;
      first_half_stuff = m_highlimit-e_denom*test_num;
      m_do_half_of_last_part = (fracfrac) (NUMBER_FIELDS_1_0_0_0 |
         (e_denom << BITS_PER_NUMBER_FIELD) | first_half_stuff);
      if (m_do_half_of_last_part != FRAC_FRAC_HALF_VALUE)
         warn(warn_hairy_fraction);
      test_num++;
   }

   m_highlimit = test_num;

   // Now my_start_point is the start point, and m_highlimit is the end point.

   if (my_start_point >= m_highlimit || m_highlimit > m_client_total)
      fail("Fraction must be proper.");

   // Check for "reverse order".
   if (frac_stuff.flags & CMD_FRAC_REVERSE) {
      uint32 orig_last = m_do_last_half_of_first_part;
      bool dont_clobber = false;

      m_reverse_order = true;
      my_start_point = m_client_total-1-my_start_point;
      m_highlimit = m_client_total-m_highlimit;

      if (m_do_half_of_last_part) {
         m_do_last_half_of_first_part = (fracfrac)
            ((e_denom << (BITS_PER_NUMBER_FIELD*3)) |
             ((e_denom - first_half_stuff) << (BITS_PER_NUMBER_FIELD*2)) |
             NUMBER_FIELDS_1_1);
         m_do_half_of_last_part = (fracfrac) 0;
         dont_clobber = true;
      }

      if (orig_last) {
         m_do_half_of_last_part = (fracfrac) (
            NUMBER_FIELDS_1_0_0_0 |
            (s_denom << BITS_PER_NUMBER_FIELD) |
            (s_denom - last_half_stuff));
         if (!dont_clobber) m_do_last_half_of_first_part = (fracfrac) 0;
      }
   }

   if (this_part != 0) {
      int highdel;
      uint32 kvalue = ((frac_stuff.flags & CMD_FRAC_PART2_MASK) / CMD_FRAC_PART2_BIT);

      // In addition to everything else, we are picking out a specific part
      // of whatever series we have decided upon.

      if (m_do_half_of_last_part | m_do_last_half_of_first_part)
         fail("This call can't be fractionalized with this fraction.");

      switch (frac_stuff.flags & CMD_FRAC_CODE_MASK) {
      case CMD_FRAC_CODE_ONLY:
         m_instant_stop = 1;
         my_start_point += m_reverse_order ? (1-this_part) : (this_part-1);

         // Be sure that enough parts are visible.
         if (my_start_point >= m_client_total)
            fail("The indicated part number doesn't exist.");

         if (m_reverse_order) {
            if (m_client_total-my_start_point > available_final_fractions)
               fail("This call can't be fractionalized.");

            if (m_highlimit > my_start_point)
               fail("The indicated part number doesn't exist.");
         }
         else {
            if (my_start_point >= available_initial_fractions)
               fail("This call can't be fractionalized.");

            if (m_highlimit <= my_start_point)
               fail("The indicated part number doesn't exist.");
         }

         break;
      case CMD_FRAC_CODE_ONLYREV:
         m_instant_stop = 1;
         my_start_point = m_reverse_order ?
            (m_highlimit-1+this_part) : (m_highlimit-this_part);

         // Be sure that enough parts are visible.
         if (my_start_point < m_client_total-available_final_fractions)
            fail("This call can't be fractionalized.");
         if (my_start_point >= m_client_total)
            fail("The indicated part number doesn't exist.");
         break;
      case CMD_FRAC_CODE_FROMTO:

         /* We are doing parts from (K+1) through N. */

         if (m_reverse_order) {
            highdel = my_start_point-this_part+1;

            if (highdel < m_highlimit   )
               fail("This call can't be fractionalized this way.");

            m_highlimit = highdel;
            my_start_point -= kvalue;

            if (my_start_point > available_initial_fractions)
               fail("This call can't be fractionalized.");
            if (my_start_point > m_client_total)
               fail("The indicated part number doesn't exist.");
         }
         else {
            highdel = m_highlimit-my_start_point-this_part;
            if (highdel < 0)
               fail("This call can't be fractionalized this way.");

            m_highlimit -= highdel;
            my_start_point += kvalue;

            if (m_highlimit > available_initial_fractions || my_start_point > available_initial_fractions)
               fail("This call can't be fractionalized.");
            if (m_highlimit > m_client_total || my_start_point > m_client_total)
               fail("The indicated part number doesn't exist.");
         }

         break;
      case CMD_FRAC_CODE_FROMTOREV:

         /* We are doing parts from N through (end-K). */

         if (m_reverse_order) {
            int lowdel = 1-this_part;

            m_highlimit += kvalue;
            my_start_point += lowdel;

            /* Be sure that enough parts are visible, and that we are within bounds.
               If lowdel is zero, we weren't cutting the upper limit, so it's
               allowed to be beyond the limit of part visibility. */

            if (my_start_point > m_client_total)
               fail("The indicated part number doesn't exist.");

            if ((my_start_point >= available_initial_fractions && lowdel != 0) ||
                m_highlimit > available_initial_fractions)
               fail("This call can't be fractionalized.");
         }
         else {
            m_highlimit -= kvalue;
            my_start_point += this_part-1;

            /* Be sure that enough parts are visible, and that we are within bounds.
               If kvalue is zero, we weren't cutting the upper limit, so it's
               allowed to be beyond the limit of part visibility. */

            if ((m_highlimit < m_client_total-available_final_fractions && kvalue != 0) ||
                my_start_point > available_initial_fractions)
               fail("This call can't be fractionalized.");
            if (m_highlimit > m_client_total || my_start_point > m_client_total)
               fail("The indicated part number doesn't exist.");
         }

         break;
      case CMD_FRAC_CODE_FROMTOREVREV:

         /* We are doing parts from (end-N+1 through (end-K). */

         if (m_reverse_order) {
            int highdel = my_start_point+1-m_highlimit-this_part;

            m_highlimit += kvalue;
            my_start_point -= highdel;

            /* Be sure that enough parts are visible, and that we are within bounds.
               If lowhdel is zero, we weren't cutting the upper limit, so it's
               allowed to be beyond the limit of part visibility. */

            if (my_start_point > m_client_total)
               fail("The indicated part number doesn't exist.");

            if ((my_start_point >= available_initial_fractions && highdel != 0) ||
                m_highlimit > available_initial_fractions)
               fail("This call can't be fractionalized.");
         }
         else {
            my_start_point = m_highlimit-this_part;
            m_highlimit -= kvalue;

            /* Be sure that enough parts are visible, and that we are within bounds.
               If kvalue is zero, we weren't cutting the upper limit, so it's
               allowed to be beyond the limit of part visibility. */

            if ((m_highlimit > available_initial_fractions && kvalue != 0) ||
                my_start_point > available_initial_fractions)
               fail("This call can't be fractionalized.");
            if (m_highlimit > m_client_total || my_start_point > m_client_total)
               fail("The indicated part number doesn't exist.");
         }

         break;
      case CMD_FRAC_CODE_FROMTOMOST:

         /* We are doing parts from (K+1) through N, but only doing half of part N. */

         if (m_reverse_order) {
            highdel = (my_start_point-this_part+1);

            if (highdel < m_highlimit)
               fail("This call can't be fractionalized this way.");

            m_highlimit = highdel;
            my_start_point -= kvalue;
            m_do_last_half_of_first_part = FRAC_FRAC_HALF_VALUE;

            if (my_start_point > available_initial_fractions)
               fail("This call can't be fractionalized.");
            if (my_start_point > m_client_total)
               fail("The indicated part number doesn't exist.");
            /* Be sure that we actually do the part that we take half of. */
            if (my_start_point < m_highlimit)
               fail("This call can't be fractionalized this way.");
         }
         else {
            highdel = m_highlimit-my_start_point-this_part;
            if (highdel < 0)
               fail("This call can't be fractionalized this way.");

            m_highlimit -= highdel;
            my_start_point += kvalue;
            m_do_half_of_last_part = FRAC_FRAC_HALF_VALUE;

            if (m_highlimit > available_initial_fractions || my_start_point > available_initial_fractions)
               fail("This call can't be fractionalized.");
            if (m_highlimit > m_client_total || my_start_point > m_client_total)
               fail("The indicated part number doesn't exist.");
            /* Be sure that we actually do the part that we take half of. */
            if (my_start_point >= m_highlimit)
               fail("This call can't be fractionalized this way.");
         }

         break;
      case CMD_FRAC_CODE_LATEFROMTOREV:

         /* Like FROMTOREV, but get a late start on the first part. */

         if (m_reverse_order) {
            int lowdel = 1-this_part;

            m_highlimit += kvalue;
            my_start_point += lowdel;
            m_do_half_of_last_part = FRAC_FRAC_LASTHALF_VALUE;

            /* Be sure that enough parts are visible, and that we are within bounds.
               If lowdel is zero, we weren't cutting the upper limit, so it's
               allowed to be beyond the limit of part visibility. */

            if (my_start_point > m_client_total)
               fail("The indicated part number doesn't exist.");

            if ((my_start_point >= available_initial_fractions && lowdel != 0) ||
                m_highlimit > available_initial_fractions)
               fail("This call can't be fractionalized.");
            /* Be sure that we actually do the part that we take half of. */
            if (my_start_point < m_highlimit)
               fail("This call can't be fractionalized this way.");
         }
         else {
            m_highlimit -= kvalue;
            my_start_point += this_part-1;
            m_do_last_half_of_first_part = FRAC_FRAC_LASTHALF_VALUE;

            /* Be sure that enough parts are visible, and that we are within bounds.
               If kvalue is zero, we weren't cutting the upper limit, so it's
               allowed to be beyond the limit of part visibility. */

            if ((m_highlimit > available_initial_fractions && kvalue != 0) ||
                my_start_point > available_initial_fractions)
               fail("This call can't be fractionalized.");
            if (m_highlimit > m_client_total || my_start_point > m_client_total)
               fail("The indicated part number doesn't exist.");
            /* Be sure that we actually do the part that we take half of. */
            if (my_start_point >= m_highlimit)
               fail("This call can't be fractionalized with this fraction.");
         }
         break;
      default:
         fail("Internal error: bad fraction code.");
      }
   }
   else if (!(frac_stuff.flags & CMD_FRAC_REVERSE) &&
            m_highlimit == 1 &&
            m_do_half_of_last_part) {
      // No action; it's OK even if call doesn't have visible fractions.
      // We would like to handle this properly when reverse order is on,
      // but we haven't gotten around to it.
   }
   else if (available_initial_fractions != 1000 && (m_client_total > 1 || (frac_stuff.flags & CMD_FRAC_REVERSE))) {
      // Unless all parts are visible, this is illegal.
      // However:   Calls with just one part can always be fractionalized.
      // But they can't be reversed.
      fail("This call can't be fractionalized.");
   }

   if (frac_stuff.fraction & CMD_FRAC_DEFER_HALF_OF_LAST) {
      if (m_do_half_of_last_part || m_do_last_half_of_first_part)
         fail("Can't do this nested fraction.");
      m_do_half_of_last_part = FRAC_FRAC_HALF_VALUE;
      frac_stuff.fraction &= ~CMD_FRAC_DEFER_HALF_OF_LAST;
   }

   if (frac_stuff.fraction & CMD_FRAC_DEFER_LASTHALF_OF_FIRST) {
      if (m_do_half_of_last_part || m_do_last_half_of_first_part)
         fail("Can't do this nested fraction.");
      m_do_last_half_of_first_part = FRAC_FRAC_LASTHALF_VALUE;
      frac_stuff.fraction &= ~CMD_FRAC_DEFER_LASTHALF_OF_FIRST;
   }

   if (frac_stuff.flags & CMD_FRAC_FIRSTHALF_ALL) {
      int diff = m_highlimit - my_start_point;

      if (m_reverse_order || m_do_half_of_last_part || m_do_last_half_of_first_part)
         fail("Can't do this nested fraction.");

      if (m_instant_stop && diff == 1)
         m_do_half_of_last_part = FRAC_FRAC_HALF_VALUE;
      else {
         if (m_instant_stop || diff <= 0 || (diff & 1))
            fail("Can't do this nested fraction.");
         m_highlimit -= diff >> 1;
      }
   }
   else if (frac_stuff.flags & CMD_FRAC_LASTHALF_ALL) {
      int diff = m_highlimit - my_start_point;

      if (m_reverse_order || m_do_half_of_last_part || m_do_last_half_of_first_part)
         fail("Can't do this nested fraction.");

      if (m_instant_stop && diff == 1)
         m_do_half_of_last_part = FRAC_FRAC_LASTHALF_VALUE;
      else {
         if (m_instant_stop || diff <= 0 || (diff & 1))
            fail("Can't do this nested fraction.");
         my_start_point += diff >> 1;
      }
   }

   if (m_reverse_order) {
      m_end_point = m_highlimit;
      m_highlimit = 1-m_highlimit;
   }
   else {
      m_end_point = m_highlimit-1;
   }

   // Here's how "restrained concepts" work.
   //
   // If the user said something like
   //      SECONDLY TANDEM swing the fractions
   // the main code for SECONDLY (in do_concept_meta) will do the work of picking out
   // the third part.  It will call this procedure, but with RESTRAIN_CRAZINESS turned
   // off, for the first part, then call this procedure with RESTRAIN_CRAZINESS turned
   // on, with some code like CMD_FRAC_CODE_ONLY, and then make a normal call for the
   // remaining parts.  But, for a concept that picks out a specific part of a call,
   // like SECONDLY, leading to a key of CMD_FRAC_CODE_ONLY, m_instant_stop will be
   // nonzero, so the code just below will not be executed.  That means that the
   // restraint will be lifted, and hence the TANDEM concept will be applied, *after*
   // picking out the specific second part.  So the restrained concept, TANDEM, will be
   // applied to the *subcall*, not to the actual "swing the fractions".
   //
   // But if the user gives some kind of "FIRST M/N <concept>" metaconcept, like
   //      FIRST 3/5 REVERSE ORDER swing the fractions
   // things are totally different.  Here the main concept selects a whole *range* of
   // the multi-part call.  (Other concepts like this are LAST M/N, MIDDLE M/N, M/N, and
   // DO THE LAST M/N.)  When one of those concepts comes in, the key will be something
   // like CMD_FRAC_CODE_FROMTO, which will leave m_instant_stop equal to zero, and
   // my_start_point and m_end_point delineating that range.  As before, do_concept_meta
   // will take care of the parts of the call not affected by the metaconcept (in this
   // case the last 2/5), and this code will be called with RESTRAIN_CRAZINESS, leading
   // to a nonzero restrained_concept_p.  That will trigger the code below.  In this
   // case the restrained concept will *not* be applied to individual subcalls.  It will
   // be applied to the indicated range of the "swing the fractions" itself.  This is
   // handled by directly taking care of the given concept (only REVERSE ORDER, M/N, and
   // DO THE LAST M/N are allowed), applying it to whatever range is specified.  We do
   // this by manipulating my_start_point and m_end_point directly.  We then indicate
   // that we have taken care of this by clearing the restrained concept pointer.  (The
   // RESTRAIN_CRAZINESS bit also needs to be cleared, but that will be done later.)

   if (restrained_concept_p && m_instant_stop == 0) {
      if ((frac_stuff.fraction & (CMD_FRAC_DEFER_HALF_OF_LAST|CMD_FRAC_DEFER_LASTHALF_OF_FIRST)) ||
          (frac_stuff.flags & (CMD_FRAC_FIRSTHALF_ALL|CMD_FRAC_LASTHALF_ALL)) ||
          m_do_last_half_of_first_part != 0 ||
          m_do_half_of_last_part != (fracfrac) 0)
         fail("Can't do this.");

      if ((*restrained_concept_p)->concept->kind == concept_meta &&
          (*restrained_concept_p)->concept->arg1 == meta_key_revorder) {
         // Swap my_start_point and m_end_point.  They are inclusive integer limits.
         int t = my_start_point;
         my_start_point = m_end_point;
         m_end_point = t;
         m_reverse_order = !m_reverse_order;     // Indicate that we are going the other way.
         *restrained_concept_p = (parse_block *) 0;  // Indicate that it's been taken care of.
      }
      else if ((*restrained_concept_p)->concept->kind == concept_fractional) {
         int span = m_end_point - my_start_point;
         if (span < 0) span = -span;
         span++;
         int num = (*restrained_concept_p)->options.number_fields & NUMBER_FIELD_MASK;
         int den = ((*restrained_concept_p)->options.number_fields >> BITS_PER_NUMBER_FIELD) & NUMBER_FIELD_MASK;
         int t = num * span;
         int shorter_span = t / den;

         if (num == 0 || den == 0 || num >= den || shorter_span * den != t || shorter_span >= span)
            fail("Can't do this.");

         if ((*restrained_concept_p)->concept->arg1 == 0) {
            // Use shorter span.
            if (m_reverse_order)
               m_end_point = my_start_point + 1 - shorter_span;
            else
               m_end_point = my_start_point + shorter_span - 1;
            *restrained_concept_p = (parse_block *) 0;
         }
         else if ((*restrained_concept_p)->concept->arg1 == 1) {
            // Use shorter span.
            if (m_reverse_order)
               my_start_point = m_end_point + shorter_span - 1;
            else
               my_start_point = m_end_point - shorter_span + 1;
            *restrained_concept_p = (parse_block *) 0;
         }
         else
            fail("Can't do this.");
      }
      else
         fail("Can't do this.");
   }

   m_subcall_incr = m_reverse_order ? -1 : 1;
   if (m_instant_stop != 0)
      m_instant_stop = my_start_point*m_subcall_incr+1;
   else
      m_instant_stop = 99;

   m_fetch_index = my_start_point;
   m_client_index = my_start_point;
   m_start_point = my_start_point;
}


fracfrac fraction_info::get_fracs_for_this_part()
{
   if (m_reverse_order) {
      if (m_do_half_of_last_part != 0 && m_client_index == m_start_point)
         return m_do_half_of_last_part;
      else if (m_do_last_half_of_first_part != 0 && m_client_index == m_end_point)
         return (fracfrac) m_do_last_half_of_first_part;
      else
         return FRAC_FRAC_NULL_VALUE;
   }
   else {
      if (m_do_half_of_last_part != 0 && (m_client_index == m_highlimit-1 || m_client_index == m_instant_stop-1))
         return m_do_half_of_last_part;
      else if (m_do_last_half_of_first_part != 0 && m_client_index == m_start_point)
         return (fracfrac) m_do_last_half_of_first_part;
      else
         return FRAC_FRAC_NULL_VALUE;
   }
}


bool fraction_info::query_instant_stop(uint32 & result_flag_wordmisc) const
{
   if (m_instant_stop != 99) {
      // Check whether we honored the last possible request.  That is,
      // whether we did the last part of the call in forward order, or
      // the first part in reverse order.
      result_flag_wordmisc |= RESULTFLAG__PARTS_ARE_KNOWN;
      if (m_instant_stop >= m_highlimit)
         result_flag_wordmisc |= RESULTFLAG__DID_LAST_PART;
      if (m_instant_stop == m_highlimit-1)
         result_flag_wordmisc |= RESULTFLAG__DID_NEXTTOLAST_PART;
      return true;
   }
   else return false;
}


int try_to_get_parts_from_parse_pointer(setup const *ss, parse_block const *pp) THROW_DECL
{
   if ((ss->cmd.cmd_misc3_flags &
        (CMD_MISC3__RESTRAIN_CRAZINESS | CMD_MISC3__RESTRAIN_MODIFIERS | CMD_MISC3__DOING_ENDS)) ||
       (ss->cmd.cmd_misc2_flags &
        (CMD_MISC2_RESTRAINED_SUPER | CMD_MISC2__ANY_WORK | CMD_MISC2__ANY_SNAG |
         CMD_MISC2__ANY_WORK_INVERT | CMD_MISC2__INVERT_CENTRAL | CMD_MISC2__INVERT_SNAG |
         CMD_MISC2__INVERT_MYSTIC | CMD_MISC2__CTR_END_MASK)) ||
       (!pp || pp->concept->kind != marker_end_of_list) ||
       (pp->call->the_defn.schema != schema_sequential) ||
       (ss->cmd.cmd_final_flags.herit & (INHERITFLAG_HALF | INHERITFLAG_REWIND | INHERITFLAG_LASTHALF)))
      return -1;
   return pp->call->the_defn.stuff.seq.howmanyparts;
}
/* This returns true if it can't do it because the assumption isn't specific enough.
   In such a case, the call was not executed.  If the user had said "with active phantoms",
   that is an error.  But if we are only doing this because the automatic active phantoms
   switch is on, we will just ignore it. */

bool fill_active_phantoms_and_move(setup *ss, setup *result, bool suppress_fudgy_2x3_2x6_fixup /*= false*/) THROW_DECL
{
   int i;

   if (check_restriction(ss, ss->cmd.cmd_assume, true, 99))
      return true;   /* We couldn't do it -- the assumption is not specific enough, like "general diamonds". */

   ss->cmd.cmd_assume.assumption = cr_none;
   move(ss, false, result, suppress_fudgy_2x3_2x6_fixup);

   // Take out the phantoms.

   for (i=0; i<=attr::slimit(result); i++) {
      if (result->people[i].id1 & BIT_ACT_PHAN)
         result->people[i].id1 = 0;
   }

   return false;
}



void move_perhaps_with_active_phantoms(setup *ss, setup *result, bool suppress_fudgy_2x3_2x6_fixup) THROW_DECL
{
   if (using_active_phantoms) {
      if (fill_active_phantoms_and_move(ss, result, suppress_fudgy_2x3_2x6_fixup)) {
         // Active phantoms couldn't be used.  Just do the call the way it is.
         // This does not count as a use of active phantoms, so don't set the flag.
         move(ss, false, result, suppress_fudgy_2x3_2x6_fixup);
      }
      else
         result->result_flags.misc |= RESULTFLAG__ACTIVE_PHANTOMS_ON;
   }
   else {
      check_restriction(ss, ss->cmd.cmd_assume, false, 99);
      move(ss, false, result, suppress_fudgy_2x3_2x6_fixup);
      result->result_flags.misc |= RESULTFLAG__ACTIVE_PHANTOMS_OFF;
   }
}



void impose_assumption_and_move(setup *ss, setup *result, bool suppress_fudgy_2x3_2x6_fixup /*= false*/) THROW_DECL
{
   if (ss->cmd.cmd_misc_flags & CMD_MISC__VERIFY_MASK) {
      assumption_thing t;

      /* **** actually, we want to allow the case of "assume waves" already in place. */
      if (ss->cmd.cmd_assume.assumption != cr_none)
         fail("Redundant or conflicting assumptions.");

      t.assump_col = 0;
      t.assump_both = 0;
      t.assump_cast = ss->cmd.cmd_assume.assump_cast;
      t.assump_live = 0;
      t.assump_negate = 0;

      switch (ss->cmd.cmd_misc_flags & CMD_MISC__VERIFY_MASK) {
      case CMD_MISC__VERIFY_WAVES:         t.assumption = cr_wave_only;     break;
      case CMD_MISC__VERIFY_2FL:           t.assumption = cr_2fl_only;      break;
      case CMD_MISC__VERIFY_DMD_LIKE:      t.assumption = cr_diamond_like;  break;
      case CMD_MISC__VERIFY_QTAG_LIKE:     t.assumption = cr_qtag_like;     break;
      case CMD_MISC__VERIFY_1_4_TAG:
         t.assumption = cr_qtag_like;
         t.assump_both = 1;
         break;
      case CMD_MISC__VERIFY_3_4_TAG:
         t.assumption = cr_qtag_like;
         t.assump_both = 2;
         break;
      case CMD_MISC__VERIFY_REAL_1_4_TAG:  t.assumption = cr_real_1_4_tag;  break;
      case CMD_MISC__VERIFY_REAL_3_4_TAG:  t.assumption = cr_real_3_4_tag;  break;
      case CMD_MISC__VERIFY_REAL_1_4_LINE: t.assumption = cr_real_1_4_line; break;
      case CMD_MISC__VERIFY_REAL_3_4_LINE: t.assumption = cr_real_3_4_line; break;
      case CMD_MISC__VERIFY_LINES:         t.assumption = cr_all_ns;        break;
      case CMD_MISC__VERIFY_COLS:          t.assumption = cr_all_ew;        break;
      case CMD_MISC__VERIFY_TALL6: t.assumption = cr_tall6; t.assump_col = 1; break;
      default:
         fail("Unknown assumption verify code.");
      }

      ss->cmd.cmd_assume = t;
      ss->cmd.cmd_misc_flags &= ~CMD_MISC__VERIFY_MASK;

      // If the assumption calls for a 1/4 tag sort of thing, and we are
      // in a (suitably populated) 3x4, fix same.

      switch (t.assumption) {
      case cr_real_1_4_tag:
      case cr_real_3_4_tag:
      case cr_real_1_4_line:
      case cr_real_3_4_line:
      case cr_diamond_like:
      case cr_qtag_like:
         if (ss->kind == s3x4 &&
             !(ss->people[0].id1 | ss->people[3].id1 |
               ss->people[6].id1 | ss->people[9].id1)) {
            expand::compress_setup(s_qtg_3x4, ss);
         }
         break;
      }

      move_perhaps_with_active_phantoms(ss, result, suppress_fudgy_2x3_2x6_fixup);
   }
   else
      move(ss, false, result, suppress_fudgy_2x3_2x6_fixup);
}




static void do_stuff_inside_sequential_call(
   setup *result,
   uint32 this_mod1,
   bool first_call,    // First call in logical definition.
   bool first_time,    // First thing we are doing, in temporal sequence.
   call_restriction *fix_next_assumption_p,
   int *fix_next_assump_col_p,
   int *fix_next_assump_both_p,
   int *remembered_2x2_elongation_p,
   final_and_herit_flags new_final_concepts,
   uint32 cmd_misc_flags,
   bool reverse_order,
   bool recompute_id,
   bool qtfudged,
   bool setup_is_elongated) THROW_DECL
{
   // We don't supply these; they get filled in by the call.
   result->cmd.cmd_misc_flags &= ~(DFM1_CONCENTRICITY_FLAG_MASK | CMD_MISC__NO_CHECK_MOD_LEVEL);

   if (this_mod1 & DFM1_NO_CHECK_MOD_LEVEL)
      result->cmd.cmd_misc_flags |= CMD_MISC__NO_CHECK_MOD_LEVEL;

   if (this_mod1 & DFM1_FINISH_THIS) {
      if (!result->cmd.cmd_fraction.is_null())
         fail("Can't fractionalize this call this way.");

      result->cmd.cmd_fraction.set_to_null_with_flags(FRACS(CMD_FRAC_CODE_FROMTOREV,2,0));
   }

   if (!first_call) {    // Is this right, or should we be using "first_time" here also?
      // Stop checking unless we are really serious.
      if (!setup_is_elongated && (result->kind == s2x2 || result->kind == s_short6))
         result->cmd.cmd_misc_flags |= CMD_MISC__NO_CHK_ELONG;

      result->cmd.cmd_misc2_flags &= ~CMD_MISC2__REQUEST_Z;
   }

   if (!first_time) {
      result->cmd.cmd_assume.assumption = *fix_next_assumption_p;

      if (*fix_next_assumption_p != cr_none) {
         result->cmd.cmd_assume.assump_col = *fix_next_assump_col_p;
         result->cmd.cmd_assume.assump_both = *fix_next_assump_both_p;
         result->cmd.cmd_assume.assump_cast = 0;
         result->cmd.cmd_assume.assump_live = 0;
         result->cmd.cmd_assume.assump_negate = 0;

         /* If we just put in an "assume 1/4 tag" type of thing, we presumably
            did a "scoot back to a wave" as part of a "scoot reaction".  Now, if
            there were phantoms in the center after the call, the result could
            have gotten changed (by the normalization stuff deep within
            "fix_n_results" or whatever) to a 2x4.  However, if we are doing a
            scoot reaction, we really want the 1/4 tag.  So change it back.
            It happens that code in "divide_the_setup" would do this anyway,
            but we don't like assumptions in place on setups for which they
            are meaningless. */

         if (*fix_next_assump_both_p == 2 &&
             (*fix_next_assumption_p == cr_jleft || *fix_next_assumption_p == cr_jright) &&
             result->kind == s2x4 &&
             (result->people[1].id1 | result->people[2].id1 |
              result->people[5].id1 | result->people[6].id1) == 0) {
            expand::expand_setup(s_qtg_2x4, result);
         }
      }
   }

   *fix_next_assumption_p = cr_none;
   *fix_next_assump_col_p = 0;
   *fix_next_assump_both_p = 0;

   call_restriction old_assumption = result->cmd.cmd_assume.assumption;
   int old_assump_col = result->cmd.cmd_assume.assump_col;
   int old_assump_both = result->cmd.cmd_assume.assump_both;

   setup_kind oldk = result->kind;

   if (oldk == s2x2 && (result->cmd.prior_elongation_bits & 3) != 0)
      *remembered_2x2_elongation_p = result->cmd.prior_elongation_bits & 3;

   // We need to manipulate some assumptions -- there are a few cases in
   // dancers really do track an awareness of the formation.

   if (result->cmd.cmd_fraction.is_null()) {
      if (!(result->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_HALF | INHERITFLAG_LASTHALF))) {
         if (result->cmd.callspec == base_calls[base_call_chreact_1]) {

            /* If we are starting a chain reaction, and the assumption was some form
               of 1/4 tag or 1/4 line (all of the above indicate such a thing --
               the outsides are a couple looking in), then, whether it was a plain
               chain reaction or a cross chain reaction, the result will have the
               checkpointers in miniwaves.  Pass that assumption on, so that they can hinge. */

            if (((old_assumption == cr_jleft ||
                  old_assumption == cr_ijleft ||
                  old_assumption == cr_jright ||
                  old_assumption == cr_ijright) && old_assump_both == 2) ||
                (old_assumption == cr_qtag_like && old_assump_both == 1) ||
                old_assumption == cr_real_1_4_tag ||
                old_assumption == cr_real_1_4_line)
               *fix_next_assumption_p = cr_ckpt_miniwaves;
         }
         else if (result->cmd.callspec == base_calls[base_call_scootback]) {
            if (result->kind == s_qtag) {
               if (old_assumption == cr_jright && old_assump_both == 2) {
                  // Jright:2 is left 1/4 tag, change to left 3/4 tag.
                  *fix_next_assumption_p = cr_jleft;
                  *fix_next_assump_both_p = 1;
               }
               else if (old_assumption == cr_jleft && old_assump_both == 2) {
                  // Jleft:2 is right 1/4 tag, change to right 3/4 tag.
                  *fix_next_assumption_p = cr_jright;
                  *fix_next_assump_both_p = 1;
               }
            }
         }
         else if (result->cmd.callspec == base_calls[base_call_scoottowave]) {
            if (result->kind == s2x4 &&
                !result->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_YOYOETCMASK)) {
               if ((result->people[0].id1 & d_mask) == d_north ||
                   (result->people[1].id1 & d_mask) == d_south ||
                   (result->people[2].id1 & d_mask) == d_north ||
                   (result->people[3].id1 & d_mask) == d_south ||
                   (result->people[4].id1 & d_mask) == d_south ||
                   (result->people[5].id1 & d_mask) == d_north ||
                   (result->people[6].id1 & d_mask) == d_south ||
                   (result->people[7].id1 & d_mask) == d_north) {
                  *fix_next_assumption_p = cr_jleft;
                  *fix_next_assump_both_p = 2;
               }
               else if ((result->people[0].id1 & d_mask) == d_south ||
                        (result->people[1].id1 & d_mask) == d_north ||
                        (result->people[2].id1 & d_mask) == d_south ||
                        (result->people[3].id1 & d_mask) == d_north ||
                        (result->people[4].id1 & d_mask) == d_north ||
                        (result->people[5].id1 & d_mask) == d_south ||
                        (result->people[6].id1 & d_mask) == d_north ||
                        (result->people[7].id1 & d_mask) == d_south) {
                  *fix_next_assumption_p = cr_jright;
                  *fix_next_assump_both_p = 2;
               }
            }
         }
         else if (result->cmd.callspec == base_calls[base_call_makepass_1] ||
                  result->cmd.callspec == base_calls[base_call_nuclear_1]) {

            // If we are starting a "make a pass", and the assumption was some form
            // of 1/4 tag, then we will have a 2-faced line in the center.  Pass that
            // assumption on, so that they can cast off 3/4.  If it was a 1/4 line,
            // the result will be a wave in the center.

            if (((old_assumption == cr_jleft || old_assumption == cr_jright) &&
                 old_assump_both == 2) ||
                old_assumption == cr_real_1_4_tag)
               *fix_next_assumption_p = cr_ctr_couples;
            else if (((old_assumption == cr_ijleft || old_assumption == cr_ijright) &&
                      old_assump_both == 2) ||
                     old_assumption == cr_real_1_4_line)
               *fix_next_assumption_p = cr_ctr_miniwaves;
            else if (old_assumption == cr_qtag_like &&
                     old_assump_both == 1 &&
                     oldk == s_qtag &&
                     (result->people[2].id1 & d_mask & ~2) == d_north &&
                     ((result->people[2].id1 ^ result->people[6].id1) & d_mask) == 2 &&
                     ((result->people[3].id1 ^ result->people[7].id1) & d_mask) == 2) {
               if (((result->people[2].id1 ^ result->people[3].id1) & d_mask) == 0)
                  *fix_next_assumption_p = cr_ctr_miniwaves;
               else if (((result->people[2].id1 ^ result->people[3].id1) & d_mask) == 2)
                  *fix_next_assumption_p = cr_ctr_couples;
            }
         }
         else if (result->cmd.callspec == base_calls[base_call_circulate]) {
            // If we are doing a circulate in columns, and the assumption was
            // "8 chain" or "trade by", change it to the other assumption.
            // Similarly for facing lines and back-to-back lines.

            if (old_assumption == cr_li_lo &&
                (old_assump_col & (~1)) == 0 &&
                ((old_assump_both - 1) & (~1)) == 0) {
               *fix_next_assumption_p = cr_li_lo;
               *fix_next_assump_col_p = old_assump_col;
               *fix_next_assump_both_p = old_assump_both ^ 3;
            }
         }
         else if ((result->cmd.callspec == base_calls[base_call_slither] ||
                   (result->cmd.callspec == base_calls[base_call_maybegrandslither] &&
                    !result->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_GRAND))) &&
                  old_assump_col == 0 &&
                  old_assump_both == 0) {
            switch (old_assumption) {
            case cr_2fl_only:
               *fix_next_assumption_p = cr_wave_only;
               break;
            case cr_wave_only:
               *fix_next_assumption_p = cr_2fl_only;
               break;
            case cr_miniwaves:
               *fix_next_assumption_p = cr_couples_only;
               break;
            case cr_couples_only:
               *fix_next_assumption_p = cr_miniwaves;
               break;
            }
         }
         else if (result->cmd.callspec == base_calls[base_base_prepare_to_drop]) {
            if (result->kind == sdmd && old_assump_col == 4) {
               if ((result->people[0].id1 & d_mask) == d_north ||
                   (result->people[2].id1 & d_mask) == d_south) {
                  if (old_assumption == cr_jright) {
                     *fix_next_assumption_p = cr_dmd_ctrs_mwv;
                     *fix_next_assump_col_p = 0;
                     *fix_next_assump_both_p = 1;
                  }
                  else if (old_assumption == cr_jleft) {
                     *fix_next_assumption_p = cr_dmd_ctrs_mwv;
                     *fix_next_assump_col_p = 0;
                     *fix_next_assump_both_p = 2;
                  }
               }
               else if ((result->people[0].id1 & d_mask) == d_south ||
                        (result->people[2].id1 & d_mask) == d_north) {
                  if (old_assumption == cr_jright) {
                     *fix_next_assumption_p = cr_dmd_ctrs_mwv;
                     *fix_next_assump_col_p = 0;
                     *fix_next_assump_both_p = 2;
                  }
                  else if (old_assumption == cr_jleft) {
                     *fix_next_assumption_p = cr_dmd_ctrs_mwv;
                     *fix_next_assump_col_p = 0;
                     *fix_next_assump_both_p = 1;
                  }
               }
            }
         }
         else if (result->cmd.callspec == base_calls[base_call_lockit] &&
                  old_assump_col == 0 &&
                  old_assump_both == 0) {
            switch (old_assumption) {
            case cr_2fl_only:
            case cr_wave_only:
               *fix_next_assumption_p = old_assumption;
               break;
            }
         }
         else if (result->cmd.callspec == base_calls[base_call_disband1] &&
                  result->kind == s2x4 &&
                  old_assump_col == 1 &&
                  old_assump_both == 0) {
            switch (old_assumption) {
            case cr_wave_only:
               *fix_next_assumption_p = cr_magic_only;
               *fix_next_assump_col_p = 1;
               break;
            case cr_magic_only:
               *fix_next_assumption_p = cr_wave_only;
               *fix_next_assump_col_p = 1;
               break;
            }
         }
         else if (result->cmd.callspec == base_calls[base_call_trade] &&
                  (result->kind == s2x4 || result->kind == s1x8 || result->kind == s1x4) &&
                  old_assumption == cr_wave_only &&
                  old_assump_col == 0) {
            *fix_next_assumption_p = cr_wave_only;
            *fix_next_assump_col_p = 0;
         }
         else if (result->cmd.callspec == base_calls[base_call_check_cross_counter]) {
            /* Just pass everything directly -- this call does nothing. */
            *fix_next_assumption_p = old_assumption;
            *fix_next_assump_col_p = old_assump_col;
            *fix_next_assump_both_p = old_assump_both;
         }
      }
      else if ((result->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_HALF))) {
         if (result->cmd.callspec == base_calls[base_call_circulate]) {
            // If we are doing a 1/2 circulate in a 2x2 that assumes lines facing in or out,
            // result is a wave.

            if (old_assumption == cr_li_lo && result->kind == s2x2 && old_assump_col == 0) {
               *fix_next_assumption_p = cr_wave_only;
               *fix_next_assump_col_p = 0;
               //               *fix_next_assump_both_p = 1;   // Right-handed.
               *fix_next_assump_both_p = 2;   // Left-handed.  *******
            }
         }
      }
   }

   if (DFM1_CPLS_UNLESS_SINGLE & this_mod1) {
      result->cmd.cmd_misc3_flags |= CMD_MISC3__DO_AS_COUPLES;
      result->cmd.do_couples_her8itflags = new_final_concepts.herit;
   }

   if ((this_mod1 & DFM1_ROLL_TRANSPARENT) != 0)
      result->cmd.cmd_misc3_flags |= CMD_MISC3__ROLL_TRANSP;
   if ((this_mod1 & DFM1_ROLL_TRANSPARENT_IF_Z) != 0)
      result->cmd.cmd_misc3_flags |= CMD_MISC3__ROLL_TRANSP_IF_Z;

   uint32 stuff = do_call_in_series(
                     result,
                     reverse_order,
                     ((cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX) == 0 &&
                      (new_final_concepts.test_heritbits(INHERITFLAG_12_MATRIX|INHERITFLAG_16_MATRIX)) == 0 &&
                      (recompute_id || (this_mod1 & DFM1_SEQ_NORMALIZE) != 0)),
                     qtfudged);

   if (stuff & (DFM1_CONC_FORCE_SPOTS | DFM1_CONC_FORCE_OTHERWAY |
                DFM1_CONC_FORCE_LINES | DFM1_CONC_FORCE_COLUMNS)) {
      *remembered_2x2_elongation_p = result->result_flags.misc & 3;
   }

   if (oldk != s2x2 && result->kind == s2x2 && *remembered_2x2_elongation_p != 0) {
      result->result_flags.misc = (result->result_flags.misc & ~3) | *remembered_2x2_elongation_p;
      result->cmd.prior_elongation_bits = (result->cmd.prior_elongation_bits & ~3) | *remembered_2x2_elongation_p;
   }
}


static void do_sequential_call(
   setup *ss,
   const calldefn *callspec,
   bool qtfudged,
   bool *mirror_p,
   uint32 extra_heritmask_bits,
   setup *result) THROW_DECL
{
   // We prefer fraction information in the fraction field rather than the herit bits.
   // (Under certain circumstances if might get changed back later.)
   if (ss->cmd.cmd_fraction.is_null() && ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_HALF)) {
      ss->cmd.cmd_fraction.set_to_firsthalf_with_flags(0);
      ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_HALF);
   }

   if (ss->cmd.cmd_fraction.is_null() && ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_LASTHALF)) {
      ss->cmd.cmd_fraction.set_to_lasthalf_with_flags(0);
      ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_LASTHALF);
   }

   bool forbid_flip = ss->cmd.callspec == base_calls[base_call_basetag0_noflip];
   final_and_herit_flags new_final_concepts = ss->cmd.cmd_final_flags;
   parse_block *parseptr = ss->cmd.parseptr;
   uint32 callflags1 = callspec->callflags1;
   calldef_schema this_schema = callspec->schema;
   bool this_schema_is_rem_or_alt =
      this_schema == schema_sequential_remainder ||
      this_schema == schema_sequential_alternate;
   bool first_call = true;    // First call in logical definition.
   bool first_time = true;    // First thing we are doing, in temporal sequence.
   call_restriction fix_next_assumption = cr_none;
   int fix_next_assump_col = 0;
   int fix_next_assump_both = 0;
   // This tells whether the setup was genuinely elongated when it came in.
   // We keep track of pseudo-elongation during the call even when it wasn't,
   // but sometimes we really need to know.
   bool setup_is_elongated =
      (ss->kind == s2x2 || ss->kind == s_short6) && (ss->cmd.prior_elongation_bits & 0x3F) != 0;

   // If this call has "half_is_inherited" or "lasthalf_is_inherited" on,
   // we don't use the fractions in the usual way.  Instead, we pass them,
   // exactly as they are, to all subcalls that want them.  Either flag will do it.
   // *All* fractions are passed through, not just half or last half.

   fraction_command saved_fracs = ss->cmd.cmd_fraction;
   bool feeding_fractions_through =
      (callspec->callflagsh & (INHERITFLAG_HALF|INHERITFLAG_LASTHALF)) != 0;

   // If rewinding, do the parts in reverse order.
   if (ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_REWIND)) {
      ss->cmd.cmd_fraction.flags ^= CMD_FRAC_REVERSE;
      ss->cmd.cmd_fraction.flags |= CMD_FRAC_FORCE_VIS;
   }

   bool distribute = (callflags1 & CFLAG1_DISTRIBUTE_REPETITIONS) != 0;

   fraction_info zzz(callspec->stuff.seq.howmanyparts);

   // These control the "repeat_n_alternate" and "repeat_n_remainder" things.
   bool fetch_other_call_for_this_cycle = false;
   bool fetching_remainder_for_this_cycle = false;

   if (distribute) {
      int ii;
      int delta = 0;

      for (ii=0 ; ii<callspec->stuff.seq.howmanyparts ; ii++) {
         by_def_item *this_item = &callspec->stuff.seq.defarray[ii];
         uint32 this_mod1 = this_item->modifiers1;

         if (this_mod1 & (DFM1_SEQ_REPEAT_N | DFM1_SEQ_REPEAT_NM1)) {
            uint32 this_count = current_options.number_fields & NUMBER_FIELD_MASK;

            delta += this_count-1;  // Why -1?  Because we're counting *extra* parts.

            if ((this_mod1 & DFM1_SEQ_REPEAT_NM1) || this_schema == schema_sequential_alternate) {
               delta--;
               if (this_count == 0) fail("Can't give number zero.");
            }

            if (this_mod1 & DFM1_SEQ_DO_HALF_MORE) {
               if (!(this_mod1 & DFM1_SEQ_REPEAT_NM1))
                  delta++;
               zzz.m_do_half_of_last_part = FRAC_FRAC_HALF_VALUE;
            }

            if (this_schema_is_rem_or_alt)
               break;
         }
      }

      zzz.fudge_client_total(delta);
   }

   // Check for special behavior of "sequential_with_fraction".

   if (this_schema == schema_sequential_with_fraction) {
      if (ss->cmd.cmd_fraction.fraction != FRAC_FRAC_NULL_VALUE)
         fail("Fractions have been specified in two places.");

      if (current_options.number_fields == 0 || current_options.number_fields > 4)
         fail("Illegal fraction.");

      if (ss->cmd.cmd_fraction.flags & CMD_FRAC_REVERSE) {
         ss->cmd.cmd_fraction.fraction =
            NUMBER_FIELDS_4_0_1_1 |
            ((4-current_options.number_fields) << (BITS_PER_NUMBER_FIELD*2));
      }
      else {
         ss->cmd.cmd_fraction.fraction =
            NUMBER_FIELDS_1_0_4_0 | current_options.number_fields;
      }
   }

   // If the "cmd_frac_flags" word is not null, we are being asked to do something special.
   // Otherwise, the defaults that we have placed into zzz will be used.

   revert_weirdness_type doing_weird_revert = weirdness_off;

   if (!ss->cmd.cmd_fraction.is_null()) {
      if ((zzz.m_do_half_of_last_part | zzz.m_do_last_half_of_first_part) != 0)
         fail("Sorry, can't fractionalize this.");

      // If we are not doing the whole call, then any "force_lines",
      // "demand_lines", "force_otherway", etc. flags are not used.
      // The starting and/or ending setups are not what the flags
      // are referring to.
      ss->cmd.cmd_misc_flags &= ~DFM1_CONCENTRICITY_FLAG_MASK;

      uint32 revertflags = ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_REVERTMASK);

      // Watch for "revert flip the line 1/2" stuff.
      // We look for a 3 part call, with fractions not visible, whose
      // 2nd part is "revert_if_needed" and whose 3rd part is "extend_n".
      // We further demand that the "n" for the extend is 2 (that is, there
      // will be no extend), and that a plain "revert" or "reflect" has been
      // given.  If this is found, we know that it is something like
      // "revert flip the line 1/2", and that it really has 2 parts -- the "flip 1/2"
      // and the "revert".  (The final "extend" isn't a part, since the tagging
      // amount is 1/2.)  In this case, we signal "get_fraction_info" to
      // do the right thing.

      // Also, look for call whose first part is "_real @v tag base".

      uint32 visibility_info = (callflags1 & CFLAG1_VISIBLE_FRACTION_MASK) / CFLAG1_VISIBLE_FRACTION_BIT;

      if (zzz.m_client_total == 3 &&    // Sorry, can't do tag the star.
          (revertflags == INHERITFLAGRVRTK_REVERT ||
           revertflags == INHERITFLAGRVRTK_REFLECT) &&
          (callflags1 & CFLAG1_NUMBER_MASK) == CFLAG1_NUMBER_BIT &&
          current_options.howmanynumbers == 1 &&
          current_options.number_fields == 2 &&
          visibility_info == 0 &&
          callspec->stuff.seq.defarray[1].call_id == base_call_revert_if_needed &&
          callspec->stuff.seq.defarray[2].call_id == base_call_extend_n) {
         zzz.m_client_total = 2;
         visibility_info |= CFLAG1_VISIBLE_FRACTION_MASK / CFLAG1_VISIBLE_FRACTION_BIT;    // Turn on all parts.
         doing_weird_revert = weirdness_flatten_from_3;
      }
      else if ((callspec->stuff.seq.defarray[0].call_id == base_call_basetag0 ||
                callspec->stuff.seq.defarray[0].call_id == base_call_basetag0_noflip) &&
          (revertflags == INHERITFLAGRVRTK_REVERT ||
           revertflags == INHERITFLAGRVRTK_REFLECT)) {
         // Treat it as though it had an extra part
         zzz.m_client_total++;

         // If call has lots of parts, make them all visible.
         visibility_info = (zzz.m_client_total >= 3) ? 7 : zzz.m_client_total;

         doing_weird_revert = weirdness_otherstuff;
      }

      parse_block **restrained_concept_p = (parse_block **) 0;
      if (ss->cmd.cmd_misc3_flags & CMD_MISC3__RESTRAIN_CRAZINESS)
         restrained_concept_p = &ss->cmd.restrained_concept;

      if (!feeding_fractions_through) zzz.get_fraction_info(ss->cmd.cmd_fraction,
                                                            visibility_info,
                                                            weirdness_off,
                                                            restrained_concept_p);

      // If distribution is on, we have to do some funny stuff.
      // We will scan the fetch array in its entirety, using the
      // client counts to control what we actually process.

      if (distribute || doing_weird_revert != weirdness_off) {
         if (zzz.m_reverse_order) {
            zzz.m_client_index = zzz.m_client_total-1;
            zzz.m_fetch_index = zzz.m_fetch_total-1;
         }
         else {
            zzz.m_client_index = 0;
            zzz.m_fetch_index = 0;
         }
      }

      if (zzz.m_reverse_order && zzz.m_instant_stop == 99) first_call = false;
   }

   // We will let "zzz.m_fetch_index" scan the actual call definition:
   //    forward - from 0 to zzz.m_fetch_total-1 inclusive
   //    reverse - from zzz.m_fetch_total-1 down to 0 inclusive.
   // While doing this, we will let "zzz.m_client_index" scan the parts of the
   // call as seen by the fracionalization stuff.  If we are not distributing
   // parts, "zzz.m_client_index" will be the same as "zzz.m_fetch_index".  Otherwise,
   // it will show the distributed subparts.

   if (new_final_concepts.test_finalbit(FINAL__SPLIT)) {
      if (callflags1 & CFLAG1_SPLIT_LIKE_SQUARE_THRU)
         new_final_concepts.set_finalbit(FINAL__SPLIT_SQUARE_APPROVED);
      else if (callflags1 & CFLAG1_SPLIT_LIKE_DIXIE_STYLE)
         new_final_concepts.set_finalbit(FINAL__SPLIT_DIXIE_APPROVED);
   }

   if (!first_time && ss->kind != s2x2 && ss->kind != s_short6)
      ss->cmd.prior_elongation_bits = 0;

   // Did we neglect to do the touch/rear back stuff because fractionalization was enabled?
   // If so, now is the time to correct that.  We only do it for the first part, and only if
   // doing parts in forward order.

   // Test for all this is "random left, swing thru".
   // The test cases for this stuff are such things as "left swing thru".
   // But don't do it if a restraint has just been lifted from a supercall-- in that case
   // Princess Leia is asking for help, and the call we are about to do is not what it seems.

   if (!(ss->cmd.cmd_misc_flags & (CMD_MISC__NO_STEP_TO_WAVE | CMD_MISC__ALREADY_STEPPED)) &&
       zzz.this_starts_at_beginning() &&
       (callflags1 & CFLAG1_STEP_REAR_MASK) &&
       !(ss->cmd.restrained_concept &&
         (ss->cmd.cmd_misc3_flags & CMD_MISC3__SUPERCALL))) {

      if (new_final_concepts.test_heritbit(INHERITFLAG_LEFT)) {
         if (!*mirror_p) mirror_this(ss);
         *mirror_p = true;
      }

      ss->cmd.cmd_misc_flags |= CMD_MISC__ALREADY_STEPPED;  // Can only do it once.
      touch_or_rear_back(ss, *mirror_p, callflags1);
   }

   // If a restrained concept is in place, it is waiting for the call to be pulled apart
   // into its pieces.  That is about to happen.  Turn off the restraint flag.
   // That will be the signal to "move" that it should act on the concept.

   ss->cmd.cmd_misc3_flags &= ~CMD_MISC3__RESTRAIN_CRAZINESS;

   int i;

   if (this_schema == schema_sequential_with_split_1x8_id) {
      if (ss->kind == s1x8) {
         for (i=0; i<8; i++) {
            if (ss->people[i].id1) ss->people[i].id2 |= (i&1) ? ID2_CENTER : ID2_END;
         }
      }
      else if (ss->kind == s3x4) {
         for (i=0; i<12; i++) {
            uint32 jjj = i+1;
            if (jjj>6) jjj+=2;
            if (ss->people[i].id1) ss->people[i].id2 |= (jjj&2) ? ID2_CENTER : ID2_END;
         }
      }
   }

   /* See comment earlier about mirroring.  For sequentially or concentrically defined
      calls, the "left" flag does not mean mirror; it is simply passed to subcalls.
      So we must put things into their normal state.  If we did any mirroring, it was
      only to facilitate the action of "touch_or_rear_back". */

   if (*mirror_p) mirror_this(ss);
   *mirror_p = false;

   prepare_for_call_in_series(result, ss);
   uint32 remember_elongation = 0;
   int remembered_2x2_elongation = 0;
   int subpart_count = 0;

   for (;;) {
      by_def_item *this_item = &callspec->stuff.seq.defarray[0];
      uint32 this_mod1 = this_item->modifiers1;
      by_def_item *alt_item = this_item;
      bool recompute_id = false;
      uint32 saved_number_fields = current_options.number_fields;
      int saved_num_numbers = current_options.howmanynumbers;
      uint32 herit_bits_to_clear = 0;

      /* Now the "index" values (zzz.m_fetch_index and zzz.m_client_index) contain the
         number of parts we have completed.  That is, they point (in 0-based
         numbering) to what we are about to do.  Also, if "subpart_count" is
         nonzero, it has the number of extra repetitions of what we just did
         that we must perform before we can go on to the next thing. */

      if (subpart_count) {
         subpart_count--;    // This is now the number of EXTRA repetitions
                             // of this that we will still have to do after
                             // we do the repetition that we are about to do.

         fetch_other_call_for_this_cycle =
            (this_schema == schema_sequential_remainder) ?
            fetching_remainder_for_this_cycle :
            !fetch_other_call_for_this_cycle;

         if (!distribute) zzz.m_client_index -= zzz.m_subcall_incr;

         // The client index moves forward, but the fetch index does not.
         // So we back up the fetch index to compensate for the incrementing
         // that will happen at the end.  Yuck.

         zzz.m_fetch_index -= zzz.m_subcall_incr;
         goto do_plain_call;
      }

      if (zzz.m_reverse_order) {
         if (zzz.m_fetch_index < 0) break;
         else if (zzz.m_fetch_index == 0) recompute_id = true;
      }
      else {
         if (zzz.m_fetch_index >= zzz.m_fetch_total) break;
      }

      zzz.demand_this_part_exists();

      setup_command foo1, foo2;

      {
         setup_command foobar = ss->cmd;
         this_item = &callspec->stuff.seq.defarray[zzz.m_fetch_index];
         this_mod1 = this_item->modifiers1;
         foobar.cmd_final_flags = new_final_concepts;

         // The no_anythingers_subst flag only applies to the first subcall.  This check shouldn't
         // be needed, but we want to be protected against an improperly formed database.
         if (zzz.m_fetch_index != 0)
            foobar.cmd_misc3_flags &= ~CMD_MISC3__NO_ANYTHINGERS_SUBST;

         if (zzz.m_reverse_order) {
            if (this_schema_is_rem_or_alt && zzz.m_fetch_index >= 1) {
               alt_item = this_item;
               this_item = &callspec->stuff.seq.defarray[zzz.m_fetch_index-1];
               zzz.m_fetch_index--;     // BTW, we require (in the database) that "distribute" be on.
               this_mod1 = this_item->modifiers1;
            }
         }
         else {
            if (this_schema_is_rem_or_alt) {
               alt_item = &callspec->stuff.seq.defarray[zzz.m_fetch_index+1];
               zzz.m_fetch_index++;     // BTW, we require (in the database) that "distribute" be on.
            }
         }

         // If we are not distributing, perform the range test now, so we don't
         // query the user needlessly about parts of calls that we won't do.

         if (!distribute) {
            if (zzz.not_yet_in_active_section()) {
               if (doing_weird_revert == weirdness_otherstuff && zzz.m_client_index == 0)
                  zzz.m_fetch_index--;

               goto go_to_next_cycle;
            }
            if (zzz.ran_off_active_section()) break;
         }

         result->result_flags.misc &= ~RESULTFLAG__NO_REEVALUATE;
         if ((this_mod1 & DFM1_SEQ_NO_RE_EVALUATE) &&
             !(result->cmd.cmd_misc2_flags & CMD_MISC2_RESTRAINED_SUPER))
            result->result_flags.misc |= RESULTFLAG__NO_REEVALUATE;

         // If an explicit substitution was made, we will recompute the ID bits for the setup.
         // Normally, we don't, which is why "patch the <anyone>" works.  The original
         // evaluation of the designees is retained after the first part of the call.
         // But if the user does something like "circle by 1/4 x [leads run]", we
         // want to re-evaluate who the leads are.

         // Turn on the expiration mechanism.
         result->cmd.prior_expire_bits |= RESULTFLAG__EXPIRATION_ENAB;

         {
            bool zzy = get_real_subcall(parseptr, this_item, &foobar,
                                        callspec, forbid_flip, extra_heritmask_bits, &foo1);
            if (zzy) {
               result->cmd.prior_expire_bits &= ~RESULTFLAG__EXPIRATION_ENAB;
               recompute_id = true;
            }
         }

         // We allow stepping (or rearing back) again.
         if (this_mod1 & DFM1_PERMIT_TOUCH_OR_REAR_BACK)
            ss->cmd.cmd_misc_flags &= ~CMD_MISC__ALREADY_STEPPED;

         if (this_schema_is_rem_or_alt)
            get_real_subcall(parseptr, alt_item, &foobar,
                             callspec, false, extra_heritmask_bits, &foo2);
      }

      // We also re-evaluate if the invocation flag "seq_re_evaluate" is on.

      if (recompute_id || (this_mod1 & DFM1_SEQ_RE_EVALUATE)) update_id_bits(result);

      // If this subcall invocation involves inserting or shifting the numbers, do so.

      if (!(new_final_concepts.test_heritbit(INHERITFLAG_FRACTAL)))
         this_mod1 &= ~DFM1_FRACTAL_INSERT;

      process_number_insertion(this_mod1);

      // Check for special repetition stuff.
      if (this_schema_is_rem_or_alt || (this_mod1 & (DFM1_SEQ_REPEAT_N | DFM1_SEQ_REPEAT_NM1))) {
         int count_to_use = current_options.number_fields & NUMBER_FIELD_MASK;

         number_used = true;
         if (this_mod1 & DFM1_SEQ_DO_HALF_MORE) count_to_use++;
         if (this_schema != schema_sequential_remainder && (this_mod1 & DFM1_SEQ_REPEAT_NM1)) count_to_use--;
         if (count_to_use < 0) fail("Can't give number zero.");

         bool just_use_half_of_count = false;

         if (zzz.m_do_half_of_last_part != 0 && !distribute &&
             zzz.m_fetch_index+zzz.m_subcall_incr == zzz.m_highlimit) {
            just_use_half_of_count = true;
         }
         else if (new_final_concepts.test_heritbit(INHERITFLAG_HALF)) {
            herit_bits_to_clear = INHERITFLAG_HALF;
            just_use_half_of_count = true;
         }
         else if (new_final_concepts.test_heritbit(INHERITFLAG_LASTHALF)) {
            herit_bits_to_clear = INHERITFLAG_LASTHALF;
            just_use_half_of_count = true;
         }

         if (just_use_half_of_count) {
            if (count_to_use & 1) fail("Can't fractionalize this call this way.");
            count_to_use >>= 1;
         }

         fetch_other_call_for_this_cycle = this_schema == schema_sequential_remainder ?
            fetching_remainder_for_this_cycle :
            zzz.m_reverse_order && (count_to_use & 1) == 0;

         subpart_count = count_to_use;
         if (subpart_count == 0) {
            goto done_with_big_cycle;
         }
         subpart_count--;
      }

      remember_elongation = result->cmd.prior_elongation_bits;

   do_plain_call:

      /* The index points to what we are about to do (0-based numbering, of course).
         Subpart_count has the number of ADDITIONAL repetitions of what we are about to do,
         after we finish the upcoming one. */

      if (zzz.not_yet_in_active_section()) {
         if (doing_weird_revert == weirdness_otherstuff && zzz.m_client_index == 1)
            zzz.m_fetch_index--;

         goto go_to_next_cycle;
      }
      if (zzz.ran_off_active_section()) break;

      {
         uint32 remember_expire = result->cmd.prior_expire_bits;
         uint32 remember_save_3x1_herit = result->cmd.cmd_heritflags_to_save_from_mxn_expansion;
         result->cmd = ss->cmd;
         result->cmd.prior_expire_bits = remember_expire;
         result->cmd.cmd_heritflags_to_save_from_mxn_expansion = remember_save_3x1_herit;
         if (result->result_flags.misc & RESULTFLAG__STOP_OVERCAST_CHECK)
            result->cmd.cmd_misc3_flags |= CMD_MISC3__STOP_OVERCAST_CHECK;
      }

      // If doing an explicit substitution, we allow another act of rearing
      // back or touching.  Also, clear the "can't do cross concentric" flag.  See rf02t.
      if ((this_mod1 & DFM1_CALL_MOD_MASK) == DFM1_CALL_MOD_MAND_ANYCALL ||
          (this_mod1 & DFM1_CALL_MOD_MASK) == DFM1_CALL_MOD_MAND_SECONDARY) {
         result->cmd.cmd_misc_flags &= ~CMD_MISC__ALREADY_STEPPED;
         result->cmd.cmd_misc3_flags &= ~CMD_MISC3__META_NOCMD;
      }

      result->cmd.prior_elongation_bits = remember_elongation;

      // If we are feeding fractions through, either "inherit_half" or "inherit_lasthalf"
      // causes the fraction info to be fed to this subcall.
      if (feeding_fractions_through) {
         if (this_item->modifiersh & (INHERITFLAG_HALF|INHERITFLAG_LASTHALF))
            result->cmd.cmd_fraction = saved_fracs;
         else
            result->cmd.cmd_fraction.set_to_null();
      }
      else {
         result->cmd.cmd_fraction.flags = 0;
         result->cmd.cmd_fraction.fraction = zzz.get_fracs_for_this_part();
      }

      if (doing_weird_revert == weirdness_otherstuff) {
         if (zzz.m_client_index == 0) {
            zzz.m_fetch_index--;
            result->cmd.cmd_fraction.set_to_firsthalf_with_flags(CMD_FRAC_FORCE_VIS);
         }
         else if (zzz.m_client_index == 1) {
            result->cmd.cmd_fraction.set_to_lasthalf_with_flags(CMD_FRAC_FORCE_VIS);
         }
      }

      setup_command *fooptr;
      fooptr = (this_schema_is_rem_or_alt && fetch_other_call_for_this_cycle) ? &foo2 : &foo1;

      result->cmd.parseptr = fooptr->parseptr;
      result->cmd.callspec = fooptr->callspec;
      result->cmd.cmd_final_flags = fooptr->cmd_final_flags;
      result->cmd.cmd_final_flags.clear_heritbits(herit_bits_to_clear);

      if (result->cmd.cmd_heritflags_to_save_from_mxn_expansion != 0 &&
          result->cmd.cmd_heritflags_to_save_from_mxn_expansion ==
          (((uint32) result->cmd.cmd_final_flags.herit) & (INHERITFLAG_MXNMASK|INHERITFLAG_NXNMASK))) {

         result->cmd.cmd_final_flags.herit = (heritflags)
            ((result->cmd.cmd_final_flags.herit & ~(INHERITFLAG_MXNMASK|INHERITFLAG_NXNMASK)) |
             (INHERITFLAGNXNK_3X3|INHERITFLAG_12_MATRIX));
      }

      do_stuff_inside_sequential_call(
         result, this_mod1, first_call, first_time,
         &fix_next_assumption,
         &fix_next_assump_col,
         &fix_next_assump_both,
         &remembered_2x2_elongation,
         new_final_concepts,
         ss->cmd.cmd_misc_flags,
         zzz.m_reverse_order,
         recompute_id,
         qtfudged,
         setup_is_elongated);

      remember_elongation = result->cmd.prior_elongation_bits;

      if (subpart_count && !distribute) goto go_to_next_cycle;

   done_with_big_cycle:

      /* This really isn't right.  It is done in order to make "ENDS FINISH set back" work.
         The flaw is that, if the sequentially defined call "FINISH set back" had more than one
         part, we would be OR'ing the bits from multiple parts.  What would it mean?  The bits
         we are interested in are the "demand_lines" and "force_lines" stuff.  I guess we should
         take the "demand" bits from the first part and the "force" bits from the last part.  Yuck! */

      /* The claim is that the following code removes the above problem.  The test is "ends 2/3 chisel thru".
         Below, we will pick up the concentricity flags from the last subcall. */

      ss->cmd.cmd_misc_flags |= result->cmd.cmd_misc_flags & ~DFM1_CONCENTRICITY_FLAG_MASK;

      if (DFM1_SEQ_REENABLE_ELONG_CHK & this_mod1)
         ss->cmd.cmd_misc_flags &= ~CMD_MISC__NO_CHK_ELONG;

      current_options.number_fields = saved_number_fields;
      current_options.howmanynumbers = saved_num_numbers;

      qtfudged = false;

      new_final_concepts.clear_finalbits(
         FINAL__SPLIT | FINAL__SPLIT_SQUARE_APPROVED | FINAL__SPLIT_DIXIE_APPROVED);

      first_call = false;
      first_time = false;

      // If we are being asked to do just one part of a call,
      // exit now.  Also, fill in bits in result->result_flags.

      if (zzz.query_instant_stop(result->result_flags.misc)) break;

   go_to_next_cycle:

      // Increment for next cycle.
      fetching_remainder_for_this_cycle = true;
      zzz.m_fetch_index += zzz.m_subcall_incr;
      zzz.m_client_index += zzz.m_subcall_incr;
   }

   // Pick up the concentricity command stuff from the last thing we did,
   // but take out the effect of "splitseq".

   ss->cmd.cmd_misc_flags |= result->cmd.cmd_misc_flags;
   ss->cmd.cmd_misc_flags &= ~CMD_MISC__MUST_SPLIT_MASK;
}


static bool do_misc_schema(
   setup *ss,
   calldef_schema the_schema,
   calldefn *callspec,
   uint32 callflags1,
   setup_command *foo1p,
   uint32 override_concentric_rules,
   selector_kind *special_selectorp,
   uint32 *special_modifiersp,
   selective_key *special_indicatorp,
   setup *result) THROW_DECL
{
   setup_command foo2 = ss->cmd;
   const by_def_item *innerdef = &callspec->stuff.conc.innerdef;
   const by_def_item *outerdef = &callspec->stuff.conc.outerdef;
   parse_block *parseptr = ss->cmd.parseptr;

   // Must be some form of concentric, or a "sel_XXX" schema.

   if (!(schema_attrs[the_schema].attrs & SCA_SPLITOK)) {
      if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
         fail("Can't split this call.");
   }

   get_real_subcall(parseptr, innerdef,
                    &ss->cmd, callspec, false, 0, foo1p);

   get_real_subcall(parseptr, outerdef,
                    &ss->cmd, callspec, false, 0, &foo2);

   foo1p->cmd_fraction = ss->cmd.cmd_fraction;
   foo2.cmd_fraction = ss->cmd.cmd_fraction;
   ss->cmd.cmd_misc3_flags |= CMD_MISC3__DOING_YOUR_PART;

   if (the_schema == schema_select_leads) {
      inner_selective_move(ss, foo1p, &foo2,
                           selective_key_plain, 1, 0, false, 0,
                           selector_leads,
                           innerdef->modifiers1,
                           outerdef->modifiers1,
                           result);
   }
   else if (the_schema == schema_select_headliners) {
      inner_selective_move(ss, foo1p, &foo2,
                           selective_key_plain, 1, 0, false, 0x80000008U,
                           selector_uninitialized,
                           innerdef->modifiers1,
                           outerdef->modifiers1,
                           result);
   }
   else if (the_schema == schema_select_sideliners) {
      inner_selective_move(ss, foo1p, &foo2,
                           selective_key_plain, 1, 0, false, 0x80000001U,
                           selector_uninitialized,
                           innerdef->modifiers1,
                           outerdef->modifiers1,
                           result);
   }
   else if (the_schema == schema_select_ctr2 || the_schema == schema_select_ctr4) {
      if ((ss->kind == s2x4 &&
           !(ss->people[1].id1 | ss->people[2].id1 |
             ss->people[5].id1 | ss->people[6].id1)) ||
          (ss->kind == s2x6 &&
           !(ss->people[2].id1 | ss->people[3].id1 |
             ss->people[8].id1 | ss->people[9].id1))) {
         *result = *ss;
      }
      else {
         // We have to do this -- the schema means the *current* centers.
         update_id_bits(ss);
         *special_selectorp = (the_schema == schema_select_ctr2) ?
            selector_center2 : selector_center4;
         *special_modifiersp = innerdef->modifiers1;
         return true;
      }
   }
   else if (the_schema == schema_select_ctr6) {
      // We have to do this -- the schema means the *current* centers.
      update_id_bits(ss);
      *special_selectorp = selector_center6;
      *special_modifiersp = innerdef->modifiers1;
      return true;
   }
   else if (the_schema == schema_select_who_can) {
      uint32 result_mask = 0xFFFFFF;   // Everyone.
      switch (ss->kind) {
      case sdmd: result_mask = 0xA; break;
      case s_short6: result_mask = 055; break;
      case s_2x1dmd: result_mask = 033; break;
      case s_qtag:
         // If this is like diamonds, only the centers hinge.
         // If it is like a 1/4 tag, everyone does.
         if ((ss->people[0].id1 | ss->people[1].id1 |
              ss->people[4].id1 | ss->people[5].id1) & 1)
            result_mask = 0xCC;
         break;
      case s3x1dmd: result_mask = 0x77; break;
      case s_spindle: result_mask = 0x77; break;
      case s_hrglass: result_mask = 0x88; break;
      case s_dhrglass: result_mask = 0xBB; break;
      case s_ptpd: result_mask = 0xEE; break;
      case s_galaxy: result_mask = 0xAA; break;
      }
      inner_selective_move(ss, foo1p, (setup_command *) 0,
                           selective_key_plain, 0, 0, false, result_mask | 0x400000,
                           selector_uninitialized,
                           innerdef->modifiers1,
                           outerdef->modifiers1,
                           result);
   }
   else if (the_schema == schema_select_who_did) {
      uint32 result_mask = 0;
      int i, j;

      for (i=0,j=1; i<=attr::slimit(ss); i++,j<<=1) {
         if (ss->people[i].id1 & PERSON_MOVED) result_mask |= j;
      }

      inner_selective_move(ss, foo1p, (setup_command *) 0,
                           selective_key_plain, 0, 0, false, result_mask | 0x400000,
                           selector_uninitialized,
                           innerdef->modifiers1,
                           outerdef->modifiers1,
                           result);
   }
   else if (the_schema == schema_select_who_didnt) {
      uint32 sourcemask = 0;
      uint32 result_mask = 0xFFFFFF;   // Everyone.
      int i, j;

      for (i=0,j=1; i<=attr::slimit(ss); i++,j<<=1) {
         if (ss->people[i].id1 & PERSON_MOVED) sourcemask |= j;
      }

      switch (ss->kind) {
      case s1x4:
         if (sourcemask == 0xA) result_mask = 0xF;
         else result_mask = 0xA;
         break;
      case s1x8:
         if (sourcemask == 0xEE) result_mask = 0xFF;
         else result_mask = 0xEE;
         break;
      case s1x6:
         if (sourcemask == 066) result_mask = 077;
         else result_mask = 066;
         break;
      case s2x4:
         if (sourcemask == 0x66) result_mask = 0xFF;
         else result_mask = 0x66;
         break;
      case s_qtag:
         if (sourcemask == 0x88) result_mask = 0xCC;
         else result_mask = 0x88;
         break;
         /*
      case s_2x1dmd: result_mask = 033; break;
      case s3x1dmd: result_mask = 0x77; break;
      case s_spindle: result_mask = 0x77; break;
      case s_hrglass: result_mask = 0x88; break;
      case s_dhrglass: result_mask = 0xBB; break;
      case s_ptpd: result_mask = 0xEE; break;
      case s_galaxy: result_mask = 0xAA; break;
         */
      default: fail("Can't do this call from this setup.");
      }

      inner_selective_move(ss, foo1p, (setup_command *) 0,
                           selective_key_plain, 0, 0, false, result_mask | 0x400000,
                           selector_uninitialized,
                           innerdef->modifiers1,
                           outerdef->modifiers1,
                           result);
   }
   else if (the_schema == schema_select_who_did_and_didnt) {
      uint32 result_mask = 0;
      int i, j;

      for (i=0,j=1; i<=attr::slimit(ss); i++,j<<=1) {
         if (ss->people[i].id1 & PERSON_MOVED) result_mask |= j;
      }

      inner_selective_move(ss, foo1p, &foo2,
                           selective_key_plain_no_live_subsets, 1, 0, false, result_mask | 0x400000,
                           selector_uninitialized,
                           innerdef->modifiers1,
                           outerdef->modifiers1,
                           result);
   }
   else if (the_schema == schema_select_original_rims) {
      *special_selectorp = ss->kind == s_crosswave ? selector_notctrdmd : selector_ends;
      *special_modifiersp = innerdef->modifiers1;
      *special_indicatorp = selective_key_plain_from_id_bits;
      return true;
   }
   else if (the_schema == schema_select_original_hubs) {
      *special_selectorp = ss->kind == s_crosswave ? selector_ctrdmd : selector_centers;
      *special_modifiersp = innerdef->modifiers1;
      *special_indicatorp = selective_key_plain_from_id_bits;
      return true;
   }
   else if (the_schema == schema_select_those_facing_both_live) {
      inner_selective_move(ss, foo1p, &foo2,
                           selective_key_plain_from_id_bits, 1, 0, true, 0,
                           selector_thosefacing,
                           innerdef->modifiers1,
                           outerdef->modifiers1,
                           result);
   }
   else {
      int rot = 0;
      bool normalize_strongly = false;
      warning_info saved_warnings = configuration::save_warnings();

      ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;     // We think this is the
                                                                // right thing to do.
      // Fudge a 3x4 into a 1/4-tag if appropriate.

      if (ss->kind == s3x4 && (callflags1 & CFLAG1_FUDGE_TO_Q_TAG) &&
          (the_schema == schema_concentric ||
           the_schema == schema_cross_concentric ||
           the_schema == schema_concentric_4_2_or_normal ||
           the_schema == schema_special_trade_by ||
           the_schema == schema_conc_o)) {
         if (ss->people[0].id1) {
            if (ss->people[1].id1) fail("Can't do this call from arbitrary 3x4 setup.");
         }
         else
            copy_person(ss, 0, ss, 1);

         if (ss->people[3].id1) {
            if (ss->people[2].id1) fail("Can't do this call from arbitrary 3x4 setup.");
            else copy_person(ss, 1, ss, 3);
         }
         else
            copy_person(ss, 1, ss, 2);

         copy_person(ss, 2, ss, 4);
         copy_person(ss, 3, ss, 5);

         if (ss->people[6].id1) {
            if (ss->people[7].id1) fail("Can't do this call from arbitrary 3x4 setup.");
            else copy_person(ss, 4, ss, 6);
         }
         else
            copy_person(ss, 4, ss, 7);

         if (ss->people[9].id1) {
            if (ss->people[8].id1) fail("Can't do this call from arbitrary 3x4 setup.");
            else copy_person(ss, 5, ss, 9);
         }
         else
            copy_person(ss, 5, ss, 8);

         copy_person(ss, 6, ss, 10);
         copy_person(ss, 7, ss, 11);

         ss->kind = s_qtag;
         ss->cmd.cmd_misc_flags |= CMD_MISC__DISTORTED;
      }

      if (ss->kind == s3x4 && the_schema == schema_concentric_or_diamond_line) {
         // If the schema is schema_concentric_or_diamond_line, and the 3x4 is occupied as in a
         // 1/4 tag, convert it.  Don't ask for CFLAG1_FUDGE_TO_Q_TAG, just do it.
         // The schema_concentric_or_diamond_line specifically looks for a 1x4 in the center, so a
         // suitably occupied 3x4 definitely has those centers and ends.  If we didn't do this,
         // it wouldn't see the centers and outsides.  Test is lo01.
         expand::fix_3x4_to_qtag(ss);
      }
      else if (ss->kind == s_qtag &&
               (callflags1 & CFLAG1_FUDGE_TO_Q_TAG) &&
               (the_schema == schema_in_out_triple ||
                the_schema == schema_in_out_triple_squash ||
                the_schema == schema_in_out_triple_dyp_squash)) {
         // Or change from qtag to 3x4 if schema requires same.
         copy_person(ss, 11, ss, 7);
         copy_person(ss, 10, ss, 6);
         copy_person(ss, 8, ss, 5);
         copy_person(ss, 7, ss, 4);
         copy_person(ss, 5, ss, 3);
         copy_person(ss, 4, ss, 2);
         copy_person(ss, 2, ss, 1);
         copy_person(ss, 1, ss, 0);
         ss->clear_person(0);
         ss->clear_person(3);
         ss->clear_person(6);
         ss->clear_person(9);

         ss->kind = s3x4;
      }

      switch (the_schema) {
      case schema_in_out_triple_squash:
      case schema_in_out_triple_dyp_squash:
      case schema_in_out_triple:
      case schema_in_out_quad:
         normalize_strongly = true;

         if (ss->kind == s2x4)
            do_matrix_expansion(ss, CONCPROP__NEEDK_4X4, false);

         if (ss->kind == s4x4) {

            /* We need to orient the setup so that it is vertical with respect
               to the way we want to pick out the outer 1x4's. */

            uint32 t1 = ss->people[1].id1 | ss->people[2].id1 | ss->people[9].id1 | ss->people[10].id1;
            uint32 t2 = ss->people[5].id1 | ss->people[6].id1 | ss->people[13].id1 | ss->people[14].id1;

            if (t1 && !t2)      rot = 1;              /* It has to be left-to-right. */
            else if (t2 && !t1) ;                     /* It has to be top-to-bottom. */
            else if (t2 && t1) fail("Can't pick out outside lines.");
            else {
               /* This "O" spots are unoccupied.  Try to figure it out from the orientation of the
                  people in the corners, so that we have lines. */
               uint32 t3 = ss->people[0].id1 | ss->people[4].id1 | ss->people[8].id1 | ss->people[12].id1;
               if ((t3 & 011) == 011) fail("Can't pick out outside lines.");
               else if (t3 & 1)        rot = 1;
               /* Otherwise, it either needs a vertical orientaion, or only the center 2x2 is occupied,
                  in which case it doesn't matter. */
            }

            ss->rotation += rot;
            canonicalize_rotation(ss);
         }
         break;
      case schema_3x3_concentric:
         if (!(ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_12_MATRIX)) &&
             !(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX))
            fail("You must specify a matrix.");

         if (ss->kind == s2x6)
            do_matrix_expansion(ss, CONCPROP__NEEDK_4X6, false);
         else
            do_matrix_expansion(ss, CONCPROP__NEEDK_3X4_D3X4, false);

         break;
      case schema_4x4_lines_concentric:
      case schema_4x4_cols_concentric:
         if (!(ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_16_MATRIX)) &&
             !(ss->cmd.cmd_misc_flags & CMD_MISC__EXPLICIT_MATRIX))
            fail("You must specify a matrix.");

         if (ss->kind == s2x4)
            do_matrix_expansion(ss, CONCPROP__NEEDK_4X4, false);

         break;
      }

      // If the schema is "reverse checkpoint", we leave the ID bits in place.
      // The database author is responsible for what ID bits mean in this case.
      concentric_move(ss, foo1p, &foo2, the_schema,
                      innerdef->modifiers1,
                      override_concentric_rules ? override_concentric_rules : outerdef->modifiers1,
                      the_schema != schema_rev_checkpoint, true, ~0U, result);

      result->rotation -= rot;   // Flip the setup back.

      /* If we expanded a 2x2 to a 4x4 and then chose an orientation at random,
         that orientation may have gotten frozen into a 2x4.  Cut it back to
         just the center 4, so that the illegal information won't affect anything. */

      if (normalize_strongly)
         normalize_setup(result, normalize_after_triple_squash, false);

      if (DFM1_SUPPRESS_ELONGATION_WARNINGS & outerdef->modifiers1)
         configuration::clear_multiple_warnings(conc_elong_warnings);

      configuration::set_multiple_warnings(saved_warnings);
   }

   return false;
}


static calldef_schema get_real_callspec_and_schema(setup *ss,
   uint32 herit_concepts,
   calldef_schema the_schema) THROW_DECL
{
   // Check for a schema that we weren't sure about,
   // and fix it up, using the specified modifiers.

   switch (the_schema) {
   case schema_maybe_single_concentric:
      return (herit_concepts & INHERITFLAG_SINGLE) ?
         schema_single_concentric : schema_concentric;
   case schema_maybe_single_cross_concentric:
      return (herit_concepts & INHERITFLAG_SINGLE) ?
         schema_single_cross_concentric : schema_cross_concentric;
   case schema_maybe_grand_single_concentric:
      if (herit_concepts & INHERITFLAG_GRAND) {
         if (herit_concepts & INHERITFLAG_SINGLE)
            return schema_grand_single_concentric;
         else
            fail("You must not use \"grand\" without \"single\".");
      }
      else {
         return (herit_concepts & INHERITFLAG_SINGLE) ?
            schema_single_concentric : schema_concentric;
      }
   case schema_grand_single_or_matrix_concentric:
      if (herit_concepts & INHERITFLAG_GRAND) {
         if (herit_concepts & (INHERITFLAG_12_MATRIX|INHERITFLAG_16_MATRIX))
            fail("You must not use \"grand\" with \"12 matrix\" or \"16 matrix\".");
         else if (herit_concepts & INHERITFLAG_SINGLE)
            return schema_grand_single_concentric;
         else
            fail("You must not use \"grand\" without \"single\".");
      }
      else {
         if (herit_concepts & INHERITFLAG_12_MATRIX)
            return schema_conc_12;
         else if (herit_concepts & INHERITFLAG_16_MATRIX)
            return schema_conc_16;
         else
            return (herit_concepts & INHERITFLAG_SINGLE) ?
               schema_single_concentric : schema_concentric;
      }
   case schema_maybe_grand_single_cross_concentric:
      if (herit_concepts & INHERITFLAG_GRAND) {
         if (herit_concepts & INHERITFLAG_SINGLE)
            return schema_grand_single_cross_concentric;
         else
            fail("You must not use \"grand\" without \"single\".");
      }
      else {
         if (herit_concepts & INHERITFLAG_SINGLE)
            return schema_single_cross_concentric;
         else
            return schema_cross_concentric;
      }
   case schema_maybe_special_single_concentric:
   case schema_maybe_special_trade_by:
      // "Single" has the usual meaning for this one.  But "grand single"
      // turns it into a "special concentric", which has the centers working
      // in three pairs.

      if (herit_concepts & INHERITFLAG_SINGLE) {
         if (herit_concepts & (INHERITFLAG_GRAND | INHERITFLAG_NXNMASK))
            return schema_concentric_others;
         else
            return schema_single_concentric;
      }
      else {
         if ((herit_concepts & (INHERITFLAG_GRAND | INHERITFLAG_NXNMASK)) == INHERITFLAG_GRAND)
            fail("You must not use \"grand\" without \"single\" or \"nxn\".");
         else if ((herit_concepts & (INHERITFLAG_GRAND | INHERITFLAG_NXNMASK)) == 0) {
            if (the_schema == schema_maybe_special_trade_by)
               return schema_special_trade_by;
            else
               return schema_concentric;
         }
         else if ((herit_concepts &
                   (INHERITFLAG_NXNMASK | INHERITFLAG_12_MATRIX | INHERITFLAG_16_MATRIX)) ==
                  (INHERITFLAGNXNK_4X4 | INHERITFLAG_16_MATRIX))
            return schema_4x4_lines_concentric;
         else if ((herit_concepts &
                   (INHERITFLAG_NXNMASK | INHERITFLAG_12_MATRIX | INHERITFLAG_16_MATRIX)) ==
                  (INHERITFLAGNXNK_3X3 | INHERITFLAG_12_MATRIX))
            return schema_3x3_concentric;
      }
      break;
   case schema_maybe_nxn_lines_concentric:
      switch (herit_concepts &
              (INHERITFLAG_SINGLE | INHERITFLAG_NXNMASK | INHERITFLAG_MXNMASK)) {
      case INHERITFLAG_SINGLE:
         return schema_single_concentric;
      case INHERITFLAGMXNK_1X3:
         return schema_concentric_6_2;
      case INHERITFLAGMXNK_3X1:
         return schema_concentric_2_6;
      case INHERITFLAGNXNK_3X3:
         return schema_3x3_concentric;
      case INHERITFLAGNXNK_4X4:
         return schema_4x4_lines_concentric;
      case INHERITFLAGMXNK_1X2:
      case INHERITFLAGMXNK_2X1:
         return schema_concentric_6p_or_normal;
      case 0:
         return schema_concentric;
      }
      break;
   case schema_maybe_nxn_cols_concentric:
      switch (herit_concepts &
              (INHERITFLAG_SINGLE | INHERITFLAG_NXNMASK | INHERITFLAG_MXNMASK)) {
      case INHERITFLAG_SINGLE:
         return schema_single_concentric;
      case INHERITFLAGMXNK_1X3:
         return schema_concentric_6_2;
      case INHERITFLAGMXNK_3X1:
         return schema_concentric_2_6;
      case INHERITFLAGNXNK_3X3:
         return schema_3x3_concentric;
      case INHERITFLAGNXNK_4X4:
         return schema_4x4_cols_concentric;
      case INHERITFLAGMXNK_1X2:
      case INHERITFLAGMXNK_2X1:
         return schema_concentric_6p_or_normal;
      case 0:
         return schema_concentric;
      }
      break;
   case schema_maybe_nxn_1331_lines_concentric:
      switch (herit_concepts &
              (INHERITFLAG_SINGLE | INHERITFLAG_NXNMASK | INHERITFLAG_MXNMASK)) {
      case INHERITFLAG_SINGLE:
         return schema_single_concentric;
      case INHERITFLAGNXNK_3X3:
         return schema_3x3_concentric;
      case INHERITFLAGNXNK_4X4:
         return schema_4x4_lines_concentric;
      case INHERITFLAGMXNK_1X3:
      case INHERITFLAGMXNK_3X1:
         return schema_1331_concentric;
      case INHERITFLAGMXNK_1X2:
      case INHERITFLAGMXNK_2X1:
         return schema_1221_concentric;
      case 0:
         return schema_concentric;
      }
      break;
   case schema_maybe_nxn_1331_cols_concentric:
      switch (herit_concepts &
              (INHERITFLAG_SINGLE | INHERITFLAG_NXNMASK | INHERITFLAG_MXNMASK)) {
      case INHERITFLAG_SINGLE:
         return schema_single_concentric;
      case INHERITFLAGNXNK_3X3:
         return schema_3x3_concentric;
      case INHERITFLAGNXNK_4X4:
         return schema_4x4_cols_concentric;
      case INHERITFLAGMXNK_1X3:
      case INHERITFLAGMXNK_3X1:
         return schema_1331_concentric;
      case INHERITFLAGMXNK_1X2:
      case INHERITFLAGMXNK_2X1:
         return schema_1221_concentric;
      case 0:
         return schema_concentric;
      }
      break;
   case schema_maybe_matrix_single_concentric_together:
      if ((herit_concepts & (INHERITFLAG_NXNMASK|INHERITFLAG_12_MATRIX)) ==
               (INHERITFLAGNXNK_3X3|INHERITFLAG_12_MATRIX))
         return schema_3x3_concentric;
      else if ((herit_concepts & (INHERITFLAG_NXNMASK|INHERITFLAG_16_MATRIX)) ==
               (INHERITFLAGNXNK_4X4|INHERITFLAG_16_MATRIX))
         return schema_4x4_lines_concentric;
      else if (herit_concepts & INHERITFLAG_12_MATRIX)
         return schema_conc_12;
      else if (herit_concepts & INHERITFLAG_16_MATRIX)
         return schema_conc_16;
      else if (herit_concepts & INHERITFLAG_GRAND)
         return schema_concentric_others;
      else if (herit_concepts & INHERITFLAG_DIAMOND)
         return schema_concentric_2_6;
      else if (herit_concepts & INHERITFLAG_SINGLE)
         return schema_single_concentric;
      else
         return schema_single_concentric_together;
   case schema_maybe_matrix_conc:
      if (herit_concepts & INHERITFLAG_12_MATRIX)
         return schema_conc_12;
      else if (herit_concepts & INHERITFLAG_16_MATRIX)
         return schema_conc_16;
      else
         return schema_concentric;
   case schema_maybe_matrix_conc_star:
      if (herit_concepts & INHERITFLAG_12_MATRIX)
         return schema_conc_star12;
      else if (herit_concepts & INHERITFLAG_16_MATRIX)
         return schema_conc_star16;
      else
         return schema_conc_star;
   case schema_maybe_matrix_conc_bar:
      if (herit_concepts & INHERITFLAG_12_MATRIX)
         return schema_conc_bar12;
      else if (herit_concepts & INHERITFLAG_16_MATRIX)
         return schema_conc_bar16;
      else
         return schema_conc_bar;
   case schema_maybe_in_out_triple_squash:
      switch (herit_concepts & (INHERITFLAG_SINGLE | INHERITFLAG_NXNMASK)) {
      case INHERITFLAG_SINGLE:
         return schema_sgl_in_out_triple_squash;
      case INHERITFLAGNXNK_3X3:
         return schema_3x3_in_out_triple_squash;
      case INHERITFLAGNXNK_4X4:
         return schema_4x4_in_out_triple_squash;
      case 0:
         return schema_in_out_triple_squash;
      }
   case schema_maybe_in_out_triple_dyp_squash:
      switch (herit_concepts & (INHERITFLAG_SINGLE | INHERITFLAG_NXNMASK)) {
      case INHERITFLAG_SINGLE:
         return schema_sgl_in_out_triple_squash;
      case INHERITFLAGNXNK_3X3:
         return schema_3x3_in_out_triple_squash;
      case INHERITFLAGNXNK_4X4:
         return schema_4x4_in_out_triple_squash;
      case 0:
         return schema_in_out_triple_dyp_squash;
      }
   default:
      return the_schema;
   }

   fail("Can't use this combination of modifiers.");
   return the_schema;   // Won't actually happen.
}



void really_inner_move(
   setup *ss,
   bool qtfudged,
   calldefn *callspec,
   calldef_schema the_schema,
   uint32 callflags1,
   uint32 callflagsf,
   uint32 override_concentric_rules,
   bool did_4x4_expansion,
   uint32 imprecise_rotation_result_flagmisc,
   bool mirror,
   setup *result) THROW_DECL
{
   if (callflagsf & CFLAG2_NO_RAISE_OVERCAST)
      ss->clear_all_overcasts();

   selector_kind special_selector = selector_none;
   selective_key special_indicator = selective_key_plain;
   uint32 special_modifiers = 0;
   // These two are always heritable.
   uint32 callflagsh = callspec->callflagsh|INHERITFLAG_HALF|INHERITFLAG_LASTHALF;

   // If the "matrix" concept is on and we get here,
   // that is, we haven't acted on a "split" command, it is illegal.

   if (ss->cmd.cmd_misc_flags & CMD_MISC__MATRIX_CONCEPT)
      fail("\"Matrix\" concept must be followed by applicable concept.");

   // Look for empty starting setup.  If definition is by array,
   // we go ahead with the call anyway.

   uint32 tbonetest = 0;
   if (attr::slimit(ss) >= 0) {
      tbonetest = or_all_people(ss);
      if (!(tbonetest & 011) && the_schema != schema_by_array) {
         result->kind = nothing;
         clear_result_flags(result);

         // We need to mark the result elongation, even though there aren't any people.
         switch (ss->kind) {
         case s2x2: case s_short6:
            result->result_flags.misc |= ss->cmd.prior_elongation_bits & 3;
            break;
         case s1x2: case s1x4: case sdmd:
            result->result_flags.misc |= 2 - (ss->rotation & 1);
            break;
         }

         return;
      }
   }

   // We can't handle the mirroring (or "Z" distortion) unless the schema is by_array, so undo it.

   if (the_schema != schema_by_array) {
      if (mirror) { mirror_this(ss); mirror = false; }
   }

   setup_command foo1;

   if ((callflags1 & CFLAG1_FUNNY_MEANS_THOSE_FACING) &&
       ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_FUNNY)) {
      ss->cmd.cmd_final_flags.clear_heritbit(INHERITFLAG_FUNNY);

      // We have to do this -- we need to know who is facing *now*.
      update_id_bits(ss);
      foo1 = ss->cmd;
      special_selector = selector_thosefacing;
      goto do_special_select_stuff;
   }

   switch (the_schema) {
   case schema_nothing:
   case schema_nothing_noroll:
   case schema_nothing_other_elong:
      if ((ss->cmd.cmd_final_flags.test_heritbits(~(INHERITFLAG_HALF|INHERITFLAG_LASTHALF|FINAL__UNDER_RANDOM_META))))
         fail("Illegal concept for this call.");
      *result = *ss;
      result->suppress_all_rolls(the_schema == schema_nothing);

      // This call is a 1-person call, so it can be presumed
      // to have split maximally both ways.
      result->result_flags.maximize_split_info();
      result->result_flags.misc = ss->cmd.prior_elongation_bits & 3;
      if (the_schema == schema_nothing_other_elong)
         result->result_flags.misc ^= 3;
      break;
   case schema_recenter:
      if ((ss->cmd.cmd_final_flags.test_heritbits(~(INHERITFLAG_HALF|INHERITFLAG_LASTHALF))) |
          (ss->cmd.cmd_final_flags.test_finalbits(~FINAL__UNDER_RANDOM_META)))
         fail("Illegal concept for this call.");
      *result = *ss;
      normalize_setup(result, normalize_recenter, false);
      if (ss->kind == result->kind)
         fail("This setup can't be recentered.");
      break;
   case schema_matrix:
   case schema_partner_matrix:
   case schema_partner_partial_matrix:
      {
         bool expanded = false;
         static const expand::thing exp_from_2x2_stuff = {{12, 0, 4, 8}, s2x2, s4x4, 0};
         static const expand::thing exp_back_to_2x6_stuff = {{0, 1, 4, 5, 6, 7, 10, 11}, s2x4, s2x6, 0};

         // The "reverse" concept might mean mirror, as in "reverse truck".
         // The "left" concept might also mean mirror, as in "left anchor".
         if (ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_REVERSE) &&
             (callflagsh & INHERITFLAG_REVERSE)) {
            mirror_this(ss);
            mirror = true;
            ss->cmd.cmd_final_flags.clear_heritbit(INHERITFLAG_REVERSE);
         }
         else if (ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_LEFT) &&
             (callflagsh & INHERITFLAG_LEFT)) {
            mirror_this(ss);
            mirror = true;
            ss->cmd.cmd_final_flags.clear_heritbit(INHERITFLAG_LEFT);
         }

         if (ss->kind == s_qtag &&
             ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_12_MATRIX)) {
            do_matrix_expansion(ss, CONCPROP__NEEDK_3X4, true);
            ss->cmd.cmd_final_flags.clear_heritbit(INHERITFLAG_12_MATRIX);
         }
         else if (ss->kind == s2x2 &&
                  // The elongation field can contain spurious stuff.  It is only
                  // meaningful if the setup is known to be distorted.
                  (ss->cmd.cmd_misc_flags & CMD_MISC__DISTORTED) &&
                  (ss->cmd.prior_elongation_bits & 3) != 0 &&
                  the_schema != schema_partner_partial_matrix) {
            expanded = true;
            if (ss->cmd.prior_elongation_bits == 3)
               expand::expand_setup(exp_from_2x2_stuff, ss);
            else if (ss->cmd.prior_elongation_bits == 1)
               expand::expand_setup(s_2x2_2x4_ends, ss);
            else
               expand::expand_setup(s_2x2_2x4_endsb, ss);

            // Since we are reconstructing the original setup,
            // we think it is reasonable to say that the setup is
            // no longer distorted.  Now, users might be able to
            // abuse this, but we hope they won't.
            ss->cmd.cmd_misc_flags &= ~CMD_MISC__DISTORTED;
         }

         remove_z_distortion(ss);

         // Not sure what this is about.  How can these flags appear?
         ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_16_MATRIX|INHERITFLAG_12_MATRIX);

         if (ss->cmd.cmd_final_flags.test_finalbits(~FINAL__UNDER_RANDOM_META))
            fail("Illegal concept for this call.");

         uint32 flags = callspec->stuff.matrix.matrix_flags;
         matrix_def_block *base_block = callspec->stuff.matrix.matrix_def_list;

         for ( ;; ) {
            if (base_block->alternate_def_flags == (uint32) ss->cmd.cmd_final_flags.herit)
               break;
            base_block = base_block->next;
            if (!base_block) fail("Illegal concept for this call.");
         }

         const uint32 *callstuff = base_block->matrix_def_items;

         if (the_schema == schema_partner_matrix || the_schema == schema_partner_partial_matrix)
            partner_matrixmove(ss, flags, callstuff, result);
         else
            matrixmove(ss, flags, callstuff, result);

         // If the invocation of this call is "roll transparent", restore roll info
         // from before the call for those people that are marked as roll-neutral.
         fix_roll_transparency_stupidly(ss, result);

         result->result_flags.misc |= RESULTFLAG__INVADED_SPACE;

         if (expanded) {
            result->result_flags.misc &= ~3;
            if (result->kind == s4x4 &&
                     !(result->people[15].id1 | result->people[3].id1 |
                       result->people[7].id1 | result->people[11].id1 |
                       result->people[13].id1 | result->people[14].id1 |
                       result->people[5].id1 | result->people[6].id1 |
                       result->people[10].id1 | result->people[9].id1 |
                       result->people[1].id1 | result->people[2].id1)) {
               result->result_flags.misc |= 3;
               expand::compress_setup(exp_from_2x2_stuff, result);
            }
            else if (result->kind == s2x4 &&
                     !(result->people[1].id1 | result->people[2].id1 |
                       result->people[5].id1 | result->people[6].id1)) {
               result->result_flags.misc |= (result->rotation & 1) + 1;
               expand::compress_setup(s_2x2_2x4_ends, result);
            }
            else if (result->kind == s2x6 &&
                     !(result->people[2].id1 | result->people[3].id1 |
                       result->people[8].id1 | result->people[9].id1)) {
               result->result_flags.misc |= (result->rotation & 1) + 1;
               expand::compress_setup(exp_back_to_2x6_stuff, result);
            }
         }
      }

      break;
   case schema_roll:
      if ((ss->cmd.cmd_final_flags.test_heritbits(~0)) |
          (ss->cmd.cmd_final_flags.test_finalbits(~FINAL__UNDER_RANDOM_META)))
         fail("Illegal concept for this call.");
      remove_z_distortion(ss);
      rollmove(ss, callspec, result);
      // This call is a 1-person call, so it can be presumed
      // to have split maximally both ways.
      result->result_flags.misc = ss->cmd.prior_elongation_bits & 3;
      result->result_flags.maximize_split_info();
      break;
   case schema_by_array:

      // Dispose of the "left" concept first -- it can only mean mirror.  If it is on,
      // mirroring may already have taken place.

      if (ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_LEFT)) {
         /* ***** why isn't this particular error test taken care of more generally elsewhere? */
         if (!(callflagsh & INHERITFLAG_LEFT))
            fail("Can't do this call 'left'.");
         if (!mirror) mirror_this(ss);
         mirror = true;
         ss->cmd.cmd_misc_flags |= CMD_MISC__DID_LEFT_MIRROR;
         ss->cmd.cmd_final_flags.clear_heritbit(INHERITFLAG_LEFT);
      }

      // The "reverse" concept might mean mirror, or it might be genuine.

      if ((ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_REVERSE)) &&
          (callflagsh & INHERITFLAG_REVERSE)) {
         // This "reverse" just means mirror.
         if (ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_LEFT))
            fail("Can't do this call 'left' and 'reverse'.");
         if (!mirror) mirror_this(ss);
         mirror = true;
         ss->cmd.cmd_final_flags.clear_heritbit(INHERITFLAG_REVERSE);
      }

      // If the "reverse" flag is still set in cmd_final_flags, it means a genuine
      // reverse as in reverse cut/flip the diamond or reverse change-O.

      basic_move(ss, callspec, tbonetest, qtfudged, mirror, result);
      break;
   default:
      /* Must be sequential or some form of concentric. */

      /* We demand that the final concepts that remain be only those in the following list,
         which includes all of the "heritable" concepts. */

      if (ss->cmd.cmd_final_flags.test_finalbits(
          ~(FINAL__SPLIT | FINAL__SPLIT_SQUARE_APPROVED |
            FINAL__SPLIT_DIXIE_APPROVED | FINAL__TRIANGLE |
            FINAL__LEADTRIANGLE | FINAL__UNDER_RANDOM_META)))
         fail("This concept not allowed here.");

      // Now we figure out how to dispose of "heritable" concepts.
      // In general, we will selectively inherit them to the call's children,
      // once we verify that the call accepts them for inheritance.
      // If it doesn't accept them, it is an error unless the concepts are
      // the special ones "magic" and/or "interlocked", which we can dispose
      // of by doing the call in the appropriate magic/interlocked setup.
      // We can also take care of things like "3x1" by expanding the setup and
      // letting the call deal with the resulting "3x3".  In both cases,
      // "divide_for_magic" will deal with it.

      uint32 extra_heritmask_bits = 0;

      {
         // Fix special case of yoyo/generous/stingy.
         uint32 hhhh = fix_gensting_weirdness(&ss->cmd, callflagsh);

         uint32 unaccepted_flags =
            ss->cmd.cmd_final_flags.test_heritbits(~hhhh);

         // Special case:  Some calls do not specify "magic" inherited
         // to their children, but can nevertheless be executed magically.
         // In such a case, the whole setup is divided into magic lines
         // or whatever, in the call to "divide_for_magic" below, and the
         // call itself never sees the magic.  It only sees the single
         // line that has been preprocessed.  Alter the Wave is such a call.
         // It wouldn't know how to do the star turns magically.
         // However, we might want to do a "finally magic alter the wave",
         // meaning that the Flip the Diamond is to be magic, which it can
         // certainly handle.  So we want to say that we can force the inheritance
         // for this part, even though the call itself doesn't want us to.
         // What is the justification for this?  Well, if we are doing a
         // "CMD_FRAC_CODE_ONLY" type of thing, we have effectively said
         // "Give me the Nth part of this call, and apply magic to it."
         // It is perfectly reasonable to say that, when I request
         // a specific part of a call, I want it unencumbered by its
         // context in the entire call.  When I request the 4th part of
         // Alter the Wave, I want a Flip the Diamond.  I take responsibility
         // for any concepts that I apply to it.
         //
         // So, in this case, we don't do the magic division here.  We
         // bypass it, and arrange for the picking out of the subcall
         // to act as though it were heritable, even if it normally
         // wouldn't be.
         //
         // This stuff applies to interlocked and similar things also.

         if (unaccepted_flags != 0) {
            if ((unaccepted_flags &
                 ~(INHERITFLAG_INTLK | INHERITFLAG_MAGIC |
                   INHERITFLAG_MXNMASK | INHERITFLAG_NXNMASK |
                   INHERITFLAG_SINGLEFILE)) == 0 &&
                the_schema == schema_sequential &&
                (ss->cmd.cmd_fraction.flags & CMD_FRAC_PART_MASK) != 0 &&
                (((ss->cmd.cmd_fraction.flags & CMD_FRAC_CODE_MASK) == CMD_FRAC_CODE_ONLY) ||
                 ((ss->cmd.cmd_fraction.flags & CMD_FRAC_CODE_MASK) == CMD_FRAC_CODE_ONLYREV))) {
               extra_heritmask_bits = unaccepted_flags;
            }
            else {
               if (!divide_for_magic(ss, unaccepted_flags, result))
                  fail("Can't do this call with this concept.");

               if (callflagsf & CFLAG2_NO_RAISE_OVERCAST)
                  result->clear_all_overcasts();
               return;
            }
         }
      }

      if (the_schema >= schema_sequential) {
         uint32 misc2 = ss->cmd.cmd_misc2_flags;

         if ((misc2 & CMD_MISC2__CENTRAL_SNAG) &&
             ((ss->cmd.cmd_fraction.flags & CMD_FRAC_PART_MASK) == 0 ||
              (((ss->cmd.cmd_fraction.flags & CMD_FRAC_CODE_MASK) != CMD_FRAC_CODE_ONLY) &&
               ((ss->cmd.cmd_fraction.flags & CMD_FRAC_CODE_MASK) != CMD_FRAC_CODE_ONLYREV)))) {
            if (!ss->cmd.cmd_fraction.is_null())
               fail("Can't fractionalize a call and use \"snag\" at the same time.");

            ss->cmd.cmd_misc2_flags &=
               ~(CMD_MISC2__CENTRAL_SNAG | CMD_MISC2__INVERT_SNAG);
            ss->cmd.cmd_fraction.set_to_firsthalf();

            // Note the uncanny similarity between the following and
            // "punt_centers_use_concept".
            {
               int i;
               int m, j;
               uint32 ssmask;
               setup the_setups[2], the_results[2];
               int sizem1 = attr::slimit(ss);
               int crossconc = (misc2 & CMD_MISC2__INVERT_SNAG) ? 0 : 1;

               ssmask = setup_attrs[ss->kind].setup_conc_masks.mask_normal;

               // Note who the original centers are.

               if (sizem1 < 0 || ssmask == 0) fail("Can't identify centers and ends.");

               setup orig_people = *ss;

               for (i=sizem1; i>=0; i--) {
                  orig_people.people[i].id2 = (ssmask ^ crossconc) & 1;
                  ssmask >>= 1;
               }

               move(ss, false, result);   /* Everyone does the first half of the call. */

               the_setups[0] = *result;              /* designees */
               the_setups[1] = *result;              /* non-designees */

               m = attr::slimit(result);
               if (m < 0) fail("Can't identify centers and ends.");

               for (j=0; j<=m; j++) {
                  uint32 p = result->people[j].id1;

                  if (p & BIT_PERSON) {
                     for (i=0; i<=sizem1; i++) {
                        if (((orig_people.people[i].id1 ^ p) & XPID_MASK) == 0) {
                           the_setups[orig_people.people[i].id2].clear_person(j);
                           goto did_it;
                        }
                     }
                     fail("Lost someone else during snag call.");
                  did_it: ;
                  }
               }

               /* Now the_setups[0] has original centers, after completion of first half
                     of the call, and the_setups[1] has original ends, also after completion.
                     We will have the_setups[0] proceed with the rest of the call. */

               normalize_setup(&the_setups[0], normalize_before_isolated_call, false);
               warning_info saved_warnings = configuration::save_warnings();

               the_setups[0].cmd = ss->cmd;
               the_setups[0].cmd.cmd_misc_flags |= CMD_MISC__PHANTOMS;
               the_setups[0].cmd.cmd_fraction.set_to_lasthalf();
               move(&the_setups[0], false, &the_results[0]);

               the_results[1] = the_setups[1];

               // Shut off "each 1x4" types of warnings -- they will arise spuriously
               // while the people do the calls in isolation.
               configuration::clear_multiple_warnings(dyp_each_warnings);
               configuration::set_multiple_warnings(saved_warnings);

               *result = the_results[1];
               result->result_flags = get_multiple_parallel_resultflags(the_results, 2);
               merge_table::merge_setups(&the_results[0], merge_c1_phantom, result);
            }

            /* ******* end of "punt_centers" junk. */

         }
         else {
            // Handle "mystic" before splitting the call into its parts.
            // But not if fractions are being used.
            if ((ss->cmd.cmd_misc2_flags & CMD_MISC2__CENTRAL_MYSTIC) && ss->cmd.cmd_fraction.is_null()) {
               ss->cmd.cmd_misc3_flags |= CMD_MISC3__DOING_YOUR_PART;
               setup_command conc_cmd = ss->cmd;

               inner_selective_move(ss, &conc_cmd, &conc_cmd, selective_key_dyp,
                                    1, 0, false, 0, selector_centers, 0, 0, result);
            }
            else
               do_sequential_call(ss, callspec, qtfudged, &mirror, extra_heritmask_bits, result);
         }

         if (the_schema == schema_split_sequential && result->kind == s2x6 &&
             ((ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_MXNMASK)) == INHERITFLAGMXNK_1X3 ||
              (ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_MXNMASK)) == INHERITFLAGMXNK_3X1)) {
            switch(little_endian_live_mask(result)) {
            case 02727:
               expand::compress_setup(exp27, result);
               break;
            case 07272:
               expand::compress_setup(exp72, result);
               break;
            }
         }
      }
      else {
         foo1 = ss->cmd;

         // *********** this is of course bogus LINES LINES LINES LINES
         if (ss->cmd.cmd_misc2_flags & CMD_MISC2_RESTRAINED_SUPER)
            foo1.extraspecialsuperduper_misc2flags |= ss->cmd.restrained_misc2flags;

         if (do_misc_schema(ss, the_schema, callspec, callflags1, &foo1, override_concentric_rules, &special_selector,
                            &special_modifiers, &special_indicator, result))
            goto do_special_select_stuff;
      }

      break;
   }

   goto foobarf;

 do_special_select_stuff:

   if (special_selector == selector_none) fail("Can't do this call in this formation.");

   inner_selective_move(ss, &foo1, (setup_command *) 0,
                        special_indicator, 0, 0, false, 0,
                        special_selector, special_modifiers, 0, result);

 foobarf:

   /* The definitions for things like "U-turn back" are sometimes
      in terms of setups larger than 1x1 for complex reasons related
      to roll direction, elongation checking, and telling which way
      "in" is.  But in fact they are treated as 1-person calls in
      terms of "stretch", "crazy", etc. */
   if (callflagsf & CFLAG2_ONE_PERSON_CALL)
      result->result_flags.maximize_split_info();

   result->result_flags.misc |= imprecise_rotation_result_flagmisc;
   result->result_flags.misc |=
      ((ss->cmd.cmd_misc3_flags & CMD_MISC3__DID_Z_COMPRESSMASK) /
       CMD_MISC3__DID_Z_COMPRESSBIT) *
      RESULTFLAG__DID_Z_COMPRESSBIT;

   // Reflect back if necessary.
   if (mirror) mirror_this(result);
   canonicalize_rotation(result);

   if (did_4x4_expansion) {
      setup outer_inners[2];
      outer_inners[0] = *result;
      outer_inners[1].kind = nothing;
      clear_result_flags(&outer_inners[1]);
      normalize_concentric(ss, schema_conc_o, 1, outer_inners, 1, 0, result);
      normalize_setup(result, simple_normalize, false);
      if (result->kind == s2x4) {
         if (result->people[1].id1 | result->people[2].id1 |
             result->people[5].id1 | result->people[6].id1)
            fail("Internal error: 'O' people wandered into middle.");
         result->swap_people(1, 3);
         result->swap_people(2, 4);
         result->swap_people(3, 7);
         result->kind = s2x2;
         result->result_flags.misc = (result->result_flags.misc & ~3) | (result->rotation+1);
         canonicalize_rotation(result);
      }
   }

   if (callflagsf & CFLAG2_NO_RAISE_OVERCAST)
      result->clear_all_overcasts();
}


static bool do_forced_couples_stuff(
   setup *ss,
   setup *result) THROW_DECL
{
   // If we have a pending "centers/ends work <concept>" concept,
   // we must dispose of it the crude way.

   if (ss->cmd.cmd_misc2_flags & (CMD_MISC2__ANY_WORK | CMD_MISC2__ANY_SNAG)) {
      punt_centers_use_concept(ss, result);
      return true;
   }

   ss->cmd.cmd_misc3_flags &= ~CMD_MISC3__DO_AS_COUPLES;
   uint32 mxnflags = ss->cmd.do_couples_her8itflags &
      (INHERITFLAG_SINGLE | INHERITFLAG_MXNMASK | INHERITFLAG_NXNMASK);

   // Mxnflags now has the "single" bit, or any "1x3" stuff.  If it is the "single"
   // bit alone, we do the call directly--we don't do "as couples".  Otherwise,
   // we the do call as couples, passing any modifiers.

   ss->cmd.do_couples_her8itflags &= ~mxnflags;

   if (mxnflags != INHERITFLAG_SINGLE) {
      tandem_couples_move(ss, selector_uninitialized, 0, 0, 0,
                          tandem_key_cpls, mxnflags, true, result);
      return true;
   }

   return false;
}


fraction_command::includes_first_part_enum fraction_command::includes_first_part()
{
   uint32 fracflags = flags;
   uint32 fracfrac = fraction;

   // If doing fractions, we can't (yet) be bothered to figure this out.
   if (fracfrac != FRAC_FRAC_NULL_VALUE)
      return notsure;

   if (fracflags == 0 ||                              // Whole thing
       fracflags == FRACS(CMD_FRAC_CODE_ONLY,1,0) ||  // First part
       (fracflags & ~CMD_FRAC_PART_MASK) == CMD_FRAC_CODE_FROMTO)  // N=any, K=0
      return yes;

   if (((fracflags & ~CMD_FRAC_PART_MASK) == CMD_FRAC_CODE_FROMTOREV) &&
       (fracflags & CMD_FRAC_PART_MASK) >= (CMD_FRAC_PART_BIT*2))
      return no;

   // No (we assume call has at least 2 parts)
   if (fracflags == FRACS(CMD_FRAC_CODE_ONLYREV,1,0))
      return no;

   return notsure;
}



// This leaves the split axis result bits in absolute orientation.

static void move_with_real_call(
   setup *ss,
   bool qtfudged,
   bool did_4x4_expansion,
   setup *result) THROW_DECL
{
   // We have a genuine call.  Presumably all serious concepts have been disposed of
   // (that is, nothing interesting will be found in parseptr -- it might be
   // useful to check that someday) and we just have the callspec and the final
   // concepts.

   if (ss->kind == nothing) {
      if (!ss->cmd.cmd_fraction.is_null())
         fail("Can't fractionalize a call if no one is doing it.");

      result->kind = nothing;
      clear_result_flags(result);   // Do we need this?
      return;
   }

   // If snag or mystic, or maybe just plain invert, was on,
   // we don't allow any final flags.
   if ((ss->cmd.cmd_misc2_flags & CMD_MISC2__DO_CENTRAL) && ss->cmd.cmd_final_flags.final)
         fail("This concept not allowed here.");

   uint32 herit_concepts = ss->cmd.cmd_final_flags.herit;

   calldefn *this_defn = &ss->cmd.callspec->the_defn;
   calldefn *deferred_array_defn = (calldefn *) 0;
   warning_info saved_warnings = configuration::save_warnings();
   call_conc_option_state saved_options = current_options;
   setup saved_ss = *ss;

 try_next_callspec:

   // Now try doing the call with this call definition.

   try {
      // Previous attempts may have messed things up.
      configuration::restore_warnings(saved_warnings);
      current_options = saved_options;
      *ss = saved_ss;
      result->clear_people();
      clear_result_flags(result, RESULTFLAG__REALLY_NO_REEVALUATE);   // In case we bail out.
      uint32 imprecise_rotation_result_flagmisc = 0;
      split_command_kind force_split = split_command_none;
      bool mirror = false;
      uint32 callflags1 = this_defn->callflags1;
      // These two are always heritable.
      uint32 callflagsh = this_defn->callflagsh|INHERITFLAG_HALF|INHERITFLAG_LASTHALF;
      uint32 callflagsf = this_defn->callflagsf;

      calldef_schema the_schema =
         get_real_callspec_and_schema(ss, herit_concepts, this_defn->schema);

      // If allowing modifications, the array version isn't what we want.
      if (the_schema == schema_by_array &&
          this_defn->compound_part &&
          allowing_modifications &&
          !deferred_array_defn) {
         deferred_array_defn = this_defn;     // Save the array definition for later.
         this_defn = this_defn->compound_part;
         goto try_next_callspec;
      }

      if ((callflags1 & CFLAG1_YOYO_FRACTAL_NUM)) {
         if (ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_FRACTAL)) {
            if ((current_options.number_fields & (NUMBER_FIELD_MASK ^ 2)) == 1)
               current_options.number_fields ^= 2;
            ss->cmd.cmd_final_flags.clear_heritbit(INHERITFLAG_FRACTAL);
         }
         else if ((ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_YOYOETCMASK)) == INHERITFLAG_YOYOETCK_YOYO) {
            if ((current_options.number_fields & NUMBER_FIELD_MASK) == 2)
               current_options.number_fields++;
            ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_YOYOETCMASK);
         }
         else if ((ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_YOYOETCMASK)) == INHERITFLAG_YOYOETCK_GENEROUS) {
            current_options.number_fields++;
            ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_YOYOETCMASK);
         }
         else if ((ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_YOYOETCMASK)) == INHERITFLAG_YOYOETCK_STINGY &&
                  current_options.number_fields > 0) {
            current_options.number_fields--;
            ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_YOYOETCMASK);
         }
      }

      // Check for "central" concept and its ilk, and pick up correct definition.

      if (ss->cmd.cmd_misc2_flags & CMD_MISC2__CTR_END_MASK) {
         ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX | CMD_MISC__DISTORTED;

         // If we invert centers and ends parts, we don't raise errors for
         // bad elongation if "suppress_elongation_warnings" was set for
         // the centers part.  This allows horrible "ends trade" on "invert
         // acey deucey", for example, since "acey deucey" has that flag
         // set for the trade that the centers do.

         if ((ss->cmd.cmd_misc2_flags & CMD_MISC2__SAID_INVERT) &&
             (schema_attrs[the_schema].attrs & SCA_INV_SUP_ELWARN) &&
             (DFM1_SUPPRESS_ELONGATION_WARNINGS & this_defn->stuff.conc.innerdef.modifiers1))
            ss->cmd.cmd_misc_flags |= CMD_MISC__NO_CHK_ELONG;

         // We shut off the "doing ends" stuff.  If we say "ends detour" we
         // mean "ends do the ends part of detour".  But if we say "ends
         // central detour" we mean "ends do the *centers* part of detour".
         ss->cmd.cmd_misc3_flags &= ~CMD_MISC3__DOING_ENDS;

         // Now we demand that, if a concept was given, the call had the
         // appropriate flag set saying that the concept is legal and will
         // be inherited to the children.  Unless it is defined by array.
         if (the_schema != schema_by_array) {
            callflagsh = fix_gensting_weirdness(&ss->cmd, callflagsh);

            if (ss->cmd.cmd_final_flags.test_heritbits(~callflagsh))
               fail("Can't do this call with this concept.");
         }

         if (ss->cmd.cmd_misc2_flags & CMD_MISC2_RESTRAINED_SUPER &&
             !(ss->cmd.cmd_misc3_flags & CMD_MISC3__DONE_WITH_REST_SUPER)) {
            // *********** LINES LINES LINES LINES LINES
            ss->cmd.cmd_misc_flags &= ~CMD_MISC__MUST_SPLIT_MASK;
            ss->cmd.cmd_misc2_flags &= ~CMD_MISC2__DO_CENTRAL;
            ss->cmd.cmd_misc3_flags |= CMD_MISC3__DONE_WITH_REST_SUPER;
         }

         if (ss->cmd.cmd_misc2_flags & CMD_MISC2__DO_CENTRAL) {
            // Set the appropriate "split h/v" bit.  It seems that we need to refrain
            // from forcing the split if 4x4 was given.  The test is [gwv]
            // finally 4x4 central stampede.

            // Also, we do not force the split if the "straight" modifier has been given.
            // In that case the dancers know what they are doing, and that the
            // usual rule that "central" means "do it on each side" is not
            // being followed.  The test is straight central interlocked little.

            if (attr::slimit(ss) == 7 &&
                ((ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_NXNMASK)) != INHERITFLAGNXNK_4X4) &&
                ((ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_STRAIGHT)) == 0)) {
               ss->cmd.cmd_misc_flags |=
                  (ss->rotation & 1) ? CMD_MISC__MUST_SPLIT_VERT : CMD_MISC__MUST_SPLIT_HORIZ;
            }

            // If it is sequential, we just pass it through.  Otherwise, we handle it here.

            if (the_schema != schema_sequential &&
                the_schema != schema_sequential_with_fraction &&
                the_schema != schema_sequential_with_split_1x8_id) {
               const by_def_item *defptr;
               uint32 inv_bits = ss->cmd.cmd_misc2_flags &
                  (CMD_MISC2__INVERT_CENTRAL | CMD_MISC2__SAID_INVERT);

               ss->cmd.cmd_misc2_flags &=
                  ~(CMD_MISC2__DO_CENTRAL | CMD_MISC2__INVERT_CENTRAL | CMD_MISC2__SAID_INVERT);

               if ((schema_attrs[the_schema].attrs &
                    (SCA_CENTRALCONC|SCA_CROSS)) == SCA_CENTRALCONC) {
                  // We used to include "schema_cross_concentric_6p_or_normal"
                  // in this clause.  It must have been related to some call definition
                  // that wasn't done correctly at that time.  It is no longer needed,
                  // and would cause "central disband" to be "legal" if it were still here.

                  // Normally, we get the centers' part of the definition.  But if the
                  // user said either "invert central" (the concept that means to get the
                  // ends' part) or "central invert" (the concept that says to get the
                  // centers' part of the inverted call) we get the ends' part.  If BOTH
                  // inversion bits are on, the user said "invert central invert", meaning
                  // to get the ends' part of the inverted call, so we just get the
                  // centers' part as usual.
                  if (inv_bits == CMD_MISC2__INVERT_CENTRAL ||
                      inv_bits == CMD_MISC2__SAID_INVERT)
                     defptr = &this_defn->stuff.conc.outerdef;
                  else
                     defptr = &this_defn->stuff.conc.innerdef;

                  if (ss->cmd.cmd_final_flags.test_finalbits(
                           ~(FINAL__SPLIT | FINAL__SPLIT_SQUARE_APPROVED |
                             FINAL__SPLIT_DIXIE_APPROVED | FINAL__UNDER_RANDOM_META)))
                     fail("This concept not allowed here.");

                  switch (defptr->modifiers1 & DFM1_CALL_MOD_MASK) {
                  case DFM1_CALL_MOD_MAND_ANYCALL:
                  case DFM1_CALL_MOD_MAND_SECONDARY:
                     fail("You can't select that part of the call.");
                  }

                  do_inheritance(&ss->cmd, this_defn, defptr, 0);
                  process_number_insertion(defptr->modifiers1);

                  if (ss->cmd.callspec == base_calls[base_call_null_second])
                     fail("You can't select that part of the call.");

                  move_with_real_call(ss, qtfudged, did_4x4_expansion, result);
                  return;
               }
               else if (the_schema == schema_select_ctr2 ||
                        the_schema == schema_select_ctr4) {
                  // Just leave the definition in place.  We will split the 8-person setup
                  // into two 4-person setups, and then pick out the center 2 from them.
                  force_split = split_command_1x4;
               }
               else
                  fail("Can't do \"central\" with this call.");
            }
         }
      }

      // We of course don't allow "mystic" or "snag" for things that are
      // *CROSS* concentrically defined.  That will be taken care of later,
      // in concentric_move.

      if (ss->cmd.cmd_misc2_flags & CMD_MISC2__CTR_END_MASK) {
         if (!(schema_attrs[the_schema].attrs & SCA_SNAGOK))
            fail("Can't do \"central/snag/mystic\" with this call.");
      }

      // Do some quick error checking for visible fractions.
      // For now, any flag is acceptable.  Later, we will
      // distinguish among the various flags.

      if (!ss->cmd.cmd_fraction.is_null()) {
         switch (the_schema) {
         case schema_by_array:
         case schema_matrix:
         case schema_partner_matrix:
         case schema_partner_partial_matrix:
            // We allow the fractions "1/2" and "last 1/2" to be given.
            // Basic_move or matrixmove will handle them.

            // We also allow any incoming fraction to alter the given number,
            // if the call is of the kind that allows that.

            // But we skip all of this if the incoming setup is empty.

            if (attr::slimit(ss) < 0 || or_all_people(ss) != 0) {
               heritflags bit_to_set = (heritflags) 0;

               if ((callflagsf & CFLAG2_FRACTIONAL_NUMBERS) &&
                   (ss->cmd.cmd_fraction.flags & ~CMD_FRAC_BREAKING_UP) == 0 &&
                   current_options.howmanynumbers == 1) {
                  fraction_info zzz(current_options.number_fields & NUMBER_FIELD_MASK);

                  zzz.get_fraction_info(ss->cmd.cmd_fraction,
                                        CFLAG1_VISIBLE_FRACTION_MASK / CFLAG1_VISIBLE_FRACTION_BIT,
                                        weirdness_off);

                  if ((zzz.m_do_half_of_last_part |
                       zzz.m_do_last_half_of_first_part |
                       zzz.m_fetch_index) == 0) {
                     current_options.number_fields = zzz.m_highlimit;
                     goto done;
                  }
               }

               if ((ss->cmd.cmd_fraction.flags & ~CMD_FRAC_BREAKING_UP) == 0) {
                  if (ss->cmd.cmd_fraction.fraction == FRAC_FRAC_HALF_VALUE)
                     bit_to_set = INHERITFLAG_HALF;
                  else if (ss->cmd.cmd_fraction.fraction == FRAC_FRAC_LASTHALF_VALUE) {
                     bit_to_set = INHERITFLAG_LASTHALF;
                  }
               }

               // Check for special case of swing the fractions with really hairy fraction.

               if (bit_to_set == 0 &&
                   ((callflags1 & CFLAG1_NUMBER_MASK) == CFLAG1_NUMBER_MASK) &&
                   !(ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_HALF|INHERITFLAG_LASTHALF))) {

                  int n = ss->cmd.cmd_fraction.fraction - NUMBER_FIELDS_1_0_4_0;
                  if (n >= 0 && n <= 3) {
                     current_options.howmanynumbers = 1;
                     current_options.number_fields = n;
                     ss->cmd.cmd_misc3_flags |= CMD_MISC3__SPECIAL_NUMBER_INVOKE;
                     goto done;
                  }
               }

               if (bit_to_set == 0 || ss->cmd.cmd_final_flags.test_heritbit(bit_to_set))
                  fail("This call can't be fractionalized this way.");
               ss->cmd.cmd_final_flags.set_heritbit(bit_to_set);

            done: ;
            }

            ss->cmd.cmd_fraction.set_to_null();

            break;
         case schema_recenter:
         case schema_roll:
            fail("This call can't be fractionalized.");
            break;
         case schema_nothing:
         case schema_nothing_noroll:
         case schema_nothing_other_elong:
         case schema_sequential:
         case schema_split_sequential:
         case schema_sequential_with_fraction:
         case schema_sequential_with_split_1x8_id:
         case schema_sequential_alternate:
         case schema_sequential_remainder:
            // These (except for "nothing") will be thoroughly checked later.
            break;
         default:

            // Must be some form of concentric.  We allow visible fractions,
            // and take no action in that case.  This means that any fractions
            // will be sent to constituent calls.

            if (!(callflags1 & CFLAG1_VISIBLE_FRACTION_MASK)) {
               // Otherwise, we allow the fraction "1/2" to be given, if the top-level
               // heritability flag allows it.  We turn the fraction into a "final concept".

               if (ss->cmd.cmd_fraction.is_firsthalf()) {
                  ss->cmd.cmd_fraction.set_to_null();
                  ss->cmd.cmd_final_flags.set_heritbit(INHERITFLAG_HALF);
               }
               else if (ss->cmd.cmd_fraction.is_lasthalf()) {
                  ss->cmd.cmd_fraction.set_to_null();
                  ss->cmd.cmd_final_flags.set_heritbit(INHERITFLAG_LASTHALF);
               }
               else {
                  fail("This call can't be fractionalized this way.");
               }
            }

            break;
         }
      }

      // If the "diamond" concept has been given and the call doesn't want it, we do
      // the "diamond single wheel" variety.

      if (ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_DIAMOND & ~callflagsh))  {
         // If the call is sequentially or concentrically defined, the top level flag is required
         // before the diamond concept can be inherited.  Since that flag is off, it is an error.
         if (the_schema != schema_by_array)
            fail("Can't do this call with the \"diamond\" concept.");

         if (ss->cmd.cmd_misc2_flags & CMD_MISC2__CTR_END_MASK)
            fail("Can't do \"invert/central/snag/mystic\" with the \"diamond\" concept.");

         ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;

         if (ss->kind == sdmd) {
            uint32 resflagsmisc = 0;
            if (ss->cmd.cmd_misc3_flags & CMD_MISC3__NEED_DIAMOND)
               resflagsmisc |= RESULTFLAG__NEED_DIAMOND;

            ss->cmd.cmd_final_flags.clear_heritbit(INHERITFLAG_DIAMOND);
            ss->clear_all_overcasts();
            divided_setup_move(ss, MAPCODE(s1x2,2,MPKIND__NONISOTROPDMD,0),
                               phantest_ok, true, result);
            result->result_flags.misc |= resflagsmisc;
            result->clear_all_overcasts();
            return;
         }
         else {
            // If in a qtag or point-to-points, perhaps we ought to divide
            // into single diamonds and try again.   BUT: if "magic" or "interlocked"
            // is also present, we don't.  We let basic_move deal with
            // it.  It will come back here after it has done what it needs to.

            if (!(ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_MAGIC|INHERITFLAG_INTLK))) {
               /* Divide into diamonds and try again.  Note that we do not clear the concept. */
               divide_diamonds(ss, result);
               return;
            }
         }
      }

      // It may be appropriate to step to a wave or rear back from one.
      // This is only legal if the flag forbidding same is off.
      // Furthermore, if certain modifiers have been given, we don't allow it.

      if ((ss->cmd.cmd_final_flags.test_heritbits(
            INHERITFLAG_MAGIC | INHERITFLAG_INTLK |
            INHERITFLAG_12_MATRIX | INHERITFLAG_16_MATRIX | INHERITFLAG_FUNNY)))
         ss->cmd.cmd_misc_flags |= CMD_MISC__NO_STEP_TO_WAVE;

      /* But, alas, if fractionalization is on, we can't do it yet, because we don't
         know whether we are starting at the beginning.  In the case of fractionalization,
         we will do it later.  We also can't do it yet if we are going
         to split the setup for "central" or "crazy", or if we are doing the call "mystic". */

      if ((!(ss->cmd.cmd_misc2_flags & CMD_MISC2__CENTRAL_MYSTIC) ||
           the_schema != schema_by_array) &&
          (callflags1 & (CFLAG1_STEP_REAR_MASK | CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK))) {

         // See if what we are doing includes the first part.
         switch (ss->cmd.cmd_fraction.fraction_command::includes_first_part()) {
         case fraction_command::yes:
            if (!(ss->cmd.cmd_misc_flags & (CMD_MISC__NO_STEP_TO_WAVE |
                                            CMD_MISC__ALREADY_STEPPED |
                                            CMD_MISC__MUST_SPLIT_MASK))) {
               if ((((callflagsh & INHERITFLAG_LEFT) || (callflags1 & CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK)) &&
                    ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_LEFT)) ||
                   ((callflagsh & INHERITFLAG_REVERSE) && ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_REVERSE))) {
                  mirror_this(ss);
                  mirror = true;
               }

               ss->cmd.cmd_misc_flags |= CMD_MISC__ALREADY_STEPPED;  // Can only do it once.
               touch_or_rear_back(ss, mirror, callflags1);

               // But, if the "left_means_touch_or_check" flag is set,
               // we only wanted the "left" flag for the purpose of what
               // "touch_or_rear_back" just did.  So, in that case,
               // we turn off the "left" flag and set things back to normal.

               if (callflags1 & CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK) {
                  if (mirror) mirror_this(ss);
                  mirror = false;
               }
            }

            // Actually, turning off the "left" flag is more global than that.

            if (callflags1 & CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK) {
               ss->cmd.cmd_final_flags.clear_heritbit(INHERITFLAG_LEFT);
            }
            break;
         case fraction_command::no:
            // If we're doing the rest of the call, just turn all that stuff off.
            if (callflags1 & CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK) {
               ss->cmd.cmd_final_flags.clear_heritbit(INHERITFLAG_LEFT);
            }
            break;
         }
      }

      if (callflagsf & CFLAG2_IMPRECISE_ROTATION)
         imprecise_rotation_result_flagmisc = RESULTFLAG__IMPRECISE_ROT;

      /* Check for a call whose schema is single (cross) concentric.
         If so, be sure the setup is divided into 1x4's or diamonds.
         But don't do it if something like "magic" is still unprocessed. */

      if ((ss->cmd.cmd_final_flags.test_heritbits(~callflagsh)) == 0) {
         // Some schemata change if the given number is odd.  For touch by N x <call>.
         if (current_options.howmanynumbers != 0 && (current_options.number_fields & 1)) {
            if (the_schema == schema_single_concentric_together_if_odd)
               the_schema = schema_single_concentric_together;
            else if (the_schema == schema_single_cross_concentric_together_if_odd)
               the_schema = schema_single_cross_concentric_together;
         }

         switch (the_schema) {
         case schema_single_concentric:
         case schema_single_cross_concentric:
            force_split = split_command_1x4;
            break;
         case schema_single_concentric_together_if_odd:
         case schema_single_cross_concentric_together_if_odd:
            force_split = split_command_1x4;
            break;
         case schema_single_concentric_together:
         case schema_single_cross_concentric_together:
            if (ss->kind == s2x6) {
               uint32 mask = little_endian_live_mask(ss);
               if (mask == 01717 || mask == 07474)
                  force_split = split_command_1x4;
            }
            // FALL THROUGH!!!!!
         case schema_concentric_6p_or_sgltogether:
            // FELL THROUGH!!
            switch (ss->kind) {
            case s1x8: case s_ptpd:
               force_split = split_command_1x4;
            case s2x4:
               // If this is "crazy" or "central" (i.e. some split bit is on)
               // and the schema is something like "schema_single_concentric_together"
               // (e.g. the call is "you all"), and the setup is a 2x4, we force a split
               // into 1x4's.  If that makes the call illegal, that's too bad.
               if (ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)
                  force_split = split_command_1x4;
            }
            break;
         case schema_sgl_in_out_triple_squash:
            switch (ss->kind) {
            case s3x4: case s2x6:
               force_split = split_command_2x3;
               break;
            }
            break;
         case schema_select_original_rims:
         case schema_select_original_hubs:
            switch (ss->kind) {
            case s1x8: case s_ptpd:
               force_split = split_command_1x8;     // This tells it not to recompute ID.
               break;
            }
            break;
         }
      }

      if (force_split != split_command_none)
         if (!do_simple_split(ss, force_split, result)) return;

      // At this point, we may have mirrored the setup and, of course, left the
      // switch "mirror" on.  We did it only as needed for the [touch / rear
      // back / check] stuff.  What we did doesn't actually count.  In
      // particular, if the call is defined concentrically or sequentially,
      // mirroring the setup in response to "left" is *NOT* the right thing to
      // do.  The right thing is to pass the "left" flag to all subparts that
      // have the "inherit_left" invocation flag, and letting events take their
      // course.  So we allow the "INHERITFLAG_LEFT" bit to remain in
      // "cmd_final_flags", because it is still important to know whether we
      // have been invoked with the "left" modifier.

      // Check for special case of ends doing a call like "detour" which
      // specifically allows just the ends part to be done.  If the call was
      // "central", this flag will be turned off.

      if (ss->cmd.cmd_misc3_flags & CMD_MISC3__DOING_ENDS) {
         if (ss->cmd.cmd_misc2_flags & CMD_MISC2__CTR_END_MASK)
            fail("Can't do \"invert/central/snag/mystic\" with a call for the ends only.");

         ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;

         if ((schema_attrs[the_schema].attrs & SCA_DETOUR) &&
             (DFM1_ENDSCANDO & this_defn->stuff.conc.outerdef.modifiers1)) {

            // Copy the concentricity flags from the call definition into the setup.
            // All the fuss in database.h about concentricity flags co-existing
            // with setupflags refers to this moment.
            ss->cmd.cmd_misc_flags |=
               (this_defn->stuff.conc.outerdef.modifiers1 & DFM1_CONCENTRICITY_FLAG_MASK);

            bool local_4x4_exp = false;

            if (the_schema == schema_conc_o) {
               static const expand::thing thing1 = {{10, 1, 2, 9},  s2x2, s4x4, 0};
               static const expand::thing thing2 = {{13, 14, 5, 6}, s2x2, s4x4, 0};

               if (ss->kind != s2x2) fail("Can't find outside 'O' spots.");

               if (ss->cmd.prior_elongation_bits == 1)
                  expand::expand_setup(thing1, ss);
               else if (ss->cmd.prior_elongation_bits == 2)
                  expand::expand_setup(thing2, ss);
               else
                  fail("Can't find outside 'O' spots.");
               local_4x4_exp = true;
            }

            do_inheritance(&ss->cmd, this_defn, &this_defn->stuff.conc.outerdef, 0);
            move_with_real_call(ss, qtfudged, local_4x4_exp, result);
            return;
         }
      }

      /* ******** We did this before, but maybe that was too early!!!!  Need to do it again
         after pulling out the "doing ends" stuff. */

      // Do some quick error checking for visible fractions.  For now,
      // any flag is acceptable.  Later, we will distinguish among the various flags.

      if (!ss->cmd.cmd_fraction.is_null()) {
         switch (the_schema) {
         case schema_by_array:
            // We allow the fractions "1/2" and "last 1/2" to be given.
            // Basic_move will handle them.
            if (ss->cmd.cmd_fraction.is_firsthalf()) {
               ss->cmd.cmd_fraction.set_to_null();
               ss->cmd.cmd_final_flags.set_heritbit(INHERITFLAG_HALF);
            }
            else if (ss->cmd.cmd_fraction.is_lasthalf()) {
               ss->cmd.cmd_fraction.set_to_null();
               ss->cmd.cmd_final_flags.set_heritbit(INHERITFLAG_LASTHALF);
            }
            else
               fail("This call can't be fractionalized this way.");

            break;
         }
      }

      // Enforce the restriction that only tagging calls are allowed in certain contexts.

      if (ss->cmd.cmd_final_flags.test_finalbit(FINAL__MUST_BE_TAG)) {
         if (!(callflags1 & CFLAG1_BASE_TAG_CALL_MASK))
            fail("Only a tagging call is allowed here.");
      }

      ss->cmd.cmd_final_flags.clear_finalbit(FINAL__MUST_BE_TAG);

      // If the "split" concept has been given and this call uses that concept
      // for a special meaning (split square thru, split dixie style), set the
      // special flag to determine that action, and remove the split concept.
      // Why remove it?  So that "heads split catch grand mix 3" will work.  If
      // we are doing a "split catch", we don't really want to split the setup
      // into 2x2's that are isolated from each other, or else the "grand mix"
      // won't work.

      if (ss->cmd.cmd_final_flags.test_finalbit(FINAL__SPLIT)) {
         bool starting = true;

         // Check for doing "split square thru" or "split dixie style" stuff.
         // But don't propagate the stuff if we aren't doing the first part of the call.
         // If the code is "ONLYREV", we assume, without checking, that the first part
         // isn't included.  That may not be right, but we can't check at present.

         if ((ss->cmd.cmd_fraction.fraction & NUMBER_FIELD_MASK_LEFT_TWO) !=
             NUMBER_FIELDS_1_0_0_0 ||
             (ss->cmd.cmd_fraction.flags & CMD_FRAC_CODE_MASK) == CMD_FRAC_CODE_ONLYREV ||
             ((ss->cmd.cmd_fraction.flags & CMD_FRAC_CODE_MASK) == CMD_FRAC_CODE_FROMTOREV &&
              (ss->cmd.cmd_fraction.flags & CMD_FRAC_PART_MASK) > CMD_FRAC_PART_BIT))
            starting = false;     // We aren't doing the first part.

         if (callflags1 & CFLAG1_SPLIT_LIKE_SQUARE_THRU) {
            if (starting) ss->cmd.cmd_final_flags.set_finalbit(FINAL__SPLIT_SQUARE_APPROVED);
            ss->cmd.cmd_final_flags.clear_finalbit(FINAL__SPLIT);

            if (current_options.howmanynumbers != 0 &&
                (current_options.number_fields & NUMBER_FIELD_MASK) <= 1)
               fail("Can't split square thru 1.");
         }
         else if (callflags1 & CFLAG1_SPLIT_LIKE_DIXIE_STYLE) {
            if (starting) ss->cmd.cmd_final_flags.set_finalbit(FINAL__SPLIT_DIXIE_APPROVED);
            ss->cmd.cmd_final_flags.clear_finalbit(FINAL__SPLIT);
         }

         // The entire rest of the program expects split calls to be done in a C1 phantom setup rather than a 4x4.
         turn_4x4_pinwheel_into_c1_phantom(ss);
      }

      // NOTE: We may have mirror-reflected the setup.  "Mirror" is true if so.
      // We may need to undo this.

      // If this is the "split sequential" schema and we have not already done so,
      // cause splitting to take place.

      if (the_schema == schema_split_sequential) {
         uint32 nxnflags = ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_NXNMASK);
         uint32 mxnflags = ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_MXNMASK);
         int limits = attr::slimit(ss);
         uint32 mask = little_endian_live_mask(ss);

         if ((limits == 7 && nxnflags != INHERITFLAGNXNK_3X3 && nxnflags != INHERITFLAGNXNK_4X4) ||
             (limits == 11 && (mxnflags == INHERITFLAGMXNK_1X3 ||
                               mxnflags == INHERITFLAGMXNK_3X1 ||
                               nxnflags == INHERITFLAGNXNK_3X3)) ||
             ((limits == 15 && nxnflags == INHERITFLAGNXNK_4X4))) {
            if (!(ss->cmd.cmd_misc_flags & CMD_MISC__MUST_SPLIT_MASK)) {
               if (ss->rotation & 1)
                  ss->cmd.cmd_misc_flags |= CMD_MISC__MUST_SPLIT_VERT;
               else
                  ss->cmd.cmd_misc_flags |= CMD_MISC__MUST_SPLIT_HORIZ;
            }
         }
         else if (ss->kind == s3x4) {
            // These setups and populations (clumps in a 3x4 or 4x4, Z's in a 2x6)
            // don't require and 3x3-like modifiers.
            if (mask == 0xF3C || mask == 0xCF3)
               ss->cmd.cmd_misc_flags |= CMD_MISC__MUST_SPLIT_HORIZ;
            else
               fail("Can't split this setup.");
         }
         else if (ss->kind == s4x4) {
            if (mask == 0x4B4B || mask == 0xB4B4)
               ss->cmd.cmd_misc_flags |= CMD_MISC__MUST_SPLIT_HORIZ;
            else
               fail("Can't split this setup.");
         }
         else if (ss->kind == s2x6) {
            if (mask == 0xDB6 || mask == 0x6DB)
               ss->cmd.cmd_misc_flags |= CMD_MISC__MUST_SPLIT_HORIZ;
            else
               fail("Can't split this setup.");
         }
         else if ((limits != 5 || nxnflags != INHERITFLAGNXNK_3X3)) {
            if (limits != 3 && limits != 1 && limits != 7) {
               fail("Need a 4 or 8 person setup for this.");
            }
         }
      }

      // If the split concept is still present, do it.

      if (ss->cmd.cmd_final_flags.test_finalbit(FINAL__SPLIT)) {
         uint32 split_map;

         if (ss->cmd.cmd_misc2_flags & CMD_MISC2__CTR_END_MASK)
            fail("Can't do \"invert/central/snag/mystic\" with the \"split\" concept.");

         ss->cmd.cmd_final_flags.clear_finalbit(FINAL__SPLIT);
         ss->cmd.cmd_misc_flags |= (CMD_MISC__SAID_SPLIT | CMD_MISC__NO_EXPAND_MATRIX);

         // We can't handle the mirroring, so undo it.
         if (mirror) { mirror_this(ss); mirror = false; }

         if      (ss->kind == s2x4)   split_map = MAPCODE(s2x2,2,MPKIND__SPLIT,0);
         else if (ss->kind == s1x8)   split_map = MAPCODE(s1x4,2,MPKIND__SPLIT,0);
         else if (ss->kind == s_ptpd) split_map = MAPCODE(sdmd,2,MPKIND__SPLIT,0);
         else if (ss->kind == s_qtag) split_map = MAPCODE(sdmd,2,MPKIND__SPLIT,1);
         else if (ss->kind == s2x2) {
            // "Split" was given while we are already in a 2x2?  The only way that
            // can be legal is if the word "split" was meant as a modifier for
            // "split square thru" etc., rather than as a virtual-setup concept,
            // or if the "split sequential" schema is in use.
            // In those cases, some "split approved" flag will still be on. */

            if (!(ss->cmd.cmd_final_flags.test_finalbits(
                       FINAL__SPLIT_SQUARE_APPROVED | FINAL__SPLIT_DIXIE_APPROVED)) &&
                !(ss->cmd.cmd_fraction.flags & CMD_FRAC_BREAKING_UP))
               // If "BREAKING_UP", caller presumably knows what she is doing.
               warn(warn__excess_split);

            if (ss->cmd.cmd_misc_flags & CMD_MISC__MATRIX_CONCEPT)
               fail("\"Matrix\" concept must be followed by applicable concept.");

            move(ss, qtfudged, result);
            result->result_flags.clear_split_info();
            return;
         }
         else
            fail("Can't do split concept in this setup.");

         /* If the user said "matrix split", the "matrix" flag will be on at this point,
         and the right thing will happen. */

         divided_setup_move(ss, split_map, phantest_ok, true, result);
         return;
      }

      // A "sequence_starter_promenade" call is only legal if really_inner_move is invoked
      // from the other place, in inner_selective_move, or at the start of a sequence.
      // Otherwise, there seem to be too many dangling loose ends in the logic.
      if ((callflags1 & CFLAG1_SEQUENCE_STARTER_PROM) != 0 && config_history_ptr != 1)
         fail("You must specify who is to do it.");

      really_inner_move(ss, qtfudged, this_defn, the_schema, callflags1, callflagsf,
                        0, did_4x4_expansion, imprecise_rotation_result_flagmisc, mirror, result);

      if ((callflagsf & CFLAG2_DO_EXCHANGE_COMPRESS))
         normalize_setup(result,
                         (result->kind == sbigdmd || result->kind == sbigptpd) ?
                         normalize_compress_bigdmd : normalize_after_exchange_boxes,
                         false);
   }
   catch(error_flag_type foo) {
      if (foo < error_flag_no_retry && this_defn != deferred_array_defn) {
         if (this_defn->compound_part) {
            // Don't take a sequential definition if there are no fractions or parts
            // specified and the call has the special flag.  This is for recycle.
            if (this_defn->compound_part->schema != schema_sequential ||
                !(this_defn->compound_part->callflagsf & CFLAG2_NO_SEQ_IF_NO_FRAC) ||
                !ss->cmd.cmd_fraction.is_null() || ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_HALF)) {
               this_defn = this_defn->compound_part;
               goto try_next_callspec;
            }
         }
         else if (deferred_array_defn) {
            this_defn = deferred_array_defn;
            goto try_next_callspec;
         }
      }

      throw foo;
   }

   current_options = saved_options;
}


static void handle_expiration(setup *ss, uint32 *bit_to_set)
{
   if (ss->cmd.prior_expire_bits & RESULTFLAG__EXPIRATION_ENAB) {
      if (ss->cmd.cmd_final_flags.test_heritbit(INHERITFLAG_TWISTED)) {
         if (ss->cmd.prior_expire_bits & RESULTFLAG__TWISTED_EXPIRED)
            ss->cmd.cmd_final_flags.clear_heritbit(INHERITFLAG_TWISTED);   // Already did that.
         *bit_to_set |= RESULTFLAG__TWISTED_EXPIRED;
      }

      // Take care of generous and stingy; they are complicated.

      switch (ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_YOYOETCMASK)) {
      case INHERITFLAG_YOYOETCK_YOYO:
         if (ss->cmd.prior_expire_bits & RESULTFLAG__YOYO_ONLY_EXPIRED)
            ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_YOYOETCMASK);
         *bit_to_set |= RESULTFLAG__YOYO_ONLY_EXPIRED;
         break;
      case INHERITFLAG_YOYOETCK_GENEROUS:
      case INHERITFLAG_YOYOETCK_STINGY:
         if (ss->cmd.prior_expire_bits & RESULTFLAG__GEN_STING_EXPIRED)
            ss->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_YOYOETCMASK);
         *bit_to_set |= RESULTFLAG__GEN_STING_EXPIRED;
         break;
      }

      // This used to be "FINAL_SPLIT" for some reason that we can't figure out now.
      // But that breaks tests nf25 and ng13. ([cheese] init triple boxes split turn the key
      // caused expiration on the "split" of the first part, which wasn't supposed
      // to have anything to do with split square thru stuff.)
      // So set it back to FINAL__SPLIT_SQUARE_APPROVED.
      if (ss->cmd.cmd_final_flags.test_finalbit(FINAL__SPLIT_SQUARE_APPROVED)) {
         if (ss->cmd.prior_expire_bits & RESULTFLAG__SPLIT_EXPIRED)
            ss->cmd.cmd_final_flags.clear_finalbit(FINAL__SPLIT_SQUARE_APPROVED);
         *bit_to_set |= RESULTFLAG__SPLIT_EXPIRED;
      }
   }
}


/* The current interpretation of the elongation flags, on input and output, is now
   as follows:

   Note first that, on input and output, the elongation bits are only meaningful in
      a 2x2 or short6 setup.
   On input, nonzero bits in "prior_elongation_bits" field with a 2x2 setup mean that
      the setup _was_ _actually_ _elongated_, and that the elongation is actually felt
      by the dancers.  In this case, the "move" routine is entitled to raise an error
      if the 2x2 call is awkward.  For example, a star thru from facing couples is
      illegal if the elongate bit is on that makes the people far away from the one
      they are facing.  It follows from this that, if we call concentric star thru,
      these bits will be cleared before calling "move", since the concentric concept
      forgives awkward elongation.  Of course, the "concentric_move" routine will
      remember the actual elongation in order to move people to the correct ending
      formation.
   On output, nonzero bits in the low two bits with a 2x2 setup mean that,
      IF RESULT ELONGATION IS REQUIRED, this is what it should be.  It does
      NOT mean that such elongation actually exists.  Whoever called "move" must
      make that judgement.  These bits are set when, for example, a 1x4 -> 2x2 call
      is executed, to indicate how to elongate the result, if it turns out that those
      people were working around the outside.  This determination is made not just on
      the "checkpoint rule" that says they go perpendicular to their original axis,
      but also on the basis of the "parallel_conc_end" flag in the call.  That is, these
      bits tell which way to go if the call had been "ends do <whatever>".  Of course,
      if the concentric concept was in use, this information is not used.  Instead,
      "concentric_move" overrides the bits we return with an absolute "checkpoint rule"
      application.

   It may well be that the goal described above is not actually implemented correctly.
*/

void move(
   setup *ss,
   bool qtfudged,
   setup *result,
   bool suppress_fudgy_2x3_2x6_fixup /*= false*/) THROW_DECL
{
   // Need this to check for dixie tag 1/4.
   if (current_options.number_fields == 1)
      ss->cmd.cmd_misc3_flags |= CMD_MISC3__PARENT_COUNT_IS_ONE;

   result->result_flags.res_heritflags_to_save_from_mxn_expansion = 0;
   parse_block *saved_magic_diamond = (parse_block *) 0;
   parse_block *parseptr = ss->cmd.parseptr;
   uint32 resultflags_to_put_inmisc = 0;
   final_and_herit_flags save_incoming_final;

   /* This shouldn't be necessary, but there have been occasional reports of the
      bigblock and stagger concepts getting confused with each other.  This would happen
      if the incoming 4x4 did not have its rotation field equal to zero, as is required
      when in canonical form.  So we check this. */

   if (setup_attrs[ss->kind].four_way_symmetry && ss->rotation != 0)
      fail("There is a bug in 4 way canonicalization -- please report this sequence.");

   // See if there is a restrained concept that has been released.
   // Tests for this are:
   //   rg04\3286   t14\702   nf17\1449   t14\739
   //   ni03\2061  vg03\3285  ci04\3066  yh07\4161

   if (ss->cmd.restrained_concept &&
       !(ss->cmd.cmd_misc3_flags & CMD_MISC3__RESTRAIN_CRAZINESS)) {
      parse_block *t = ss->cmd.restrained_concept;
      ss->cmd.restrained_concept = (parse_block *) 0;
      remove_z_distortion(ss);

      if (ss->cmd.cmd_misc2_flags & CMD_MISC2_RESTRAINED_SUPER)
         fail("Can't nest meta-concepts and supercalls.");

      if (t->concept->kind == concept_another_call_next_mod) {
         // This is a "supercall".

         if (ss->cmd.callspec == 0) {
            // We need to fill in the call.  This requires that things be nice.
            parse_block *parseptrtemp = process_final_concepts(parseptr, true, &ss->cmd.cmd_final_flags, true, false);

            if (parseptrtemp->concept->kind > marker_end_of_list)
               fail("Incomplete supercall.");

            ss->cmd.parseptr = parseptrtemp;
            ss->cmd.callspec = parseptrtemp->call;
         }

         parse_block p1 = *t;
         parse_block p2 = *p1.next;
         parse_block p3 = *(p2.subsidiary_root);

         p1.next = &p2;
         p2.concept = &concept_marker_concept_supercall;
         p2.subsidiary_root = &p3;
         p3.call = ss->cmd.callspec;
         p3.call_to_print = p3.call;

         if (p3.concept->kind != concept_another_call_next_mod) {
            p3.concept = &concept_mark_end_of_list;
            p3.next = (parse_block *) 0;
         }

         p3.no_check_call_level = true;
         p3.options = current_options;
         ss->cmd.parseptr = &p1;
         ss->cmd.callspec = (call_with_name *) 0;
         ss->cmd.restrained_super8flags = ss->cmd.cmd_final_flags.herit;
         ss->cmd.restrained_miscflags = ss->cmd.cmd_misc_flags;
         ss->cmd.restrained_misc2flags = ss->cmd.cmd_misc2_flags;
         //  ****************** change for the "init lines [central little more] thru" bug LINES LINES LINES LINES
         //         ss->cmd.cmd_misc2_flags &= ~CMD_MISC2__CTR_END_MASK;
         ss->cmd.cmd_misc2_flags |= CMD_MISC2_RESTRAINED_SUPER;
         ss->cmd.cmd_final_flags.clear_all_heritbits();
         move(ss, false, result);
      }
      else {
         // Find the end of the restraint chain, then splice that call in at the restraint tail.
         // The end of the restraint chain may be past the tail point.
         parse_block *z0 = t;
         bool fixme = true;

         for ( ; ; ) {
            // Clear "fixme" if we pass a meta-concept like "initially".
            if (get_meta_key_props(z0->concept) & MKP_RESTRAIN_2) fixme = false;

            if (z0->concept->kind == marker_end_of_list)
               break;

            if (z0->concept->kind < marker_end_of_list && fixme)
               break;

            if (z0->concept->kind < marker_end_of_list &&
                (z0->call->the_defn.callflagsf & CFLAGH__TAG_CALL_RQ_MASK) != 0) {
               // A tagger call stops immediately.
               break;
            }
            else if (z0->concept->kind < marker_end_of_list && z0->subsidiary_root) {
               // For "mod" pairs, watch for tracing downward.
               z0 = z0->subsidiary_root;
            }
            else if (concept_table[z0->concept->kind].concept_prop & CONCPROP__SECOND_CALL)
               // Concepts like "sandwich" also trace downward.
               z0 = z0->subsidiary_root;
            else
               z0 = z0->next;
         }

         parse_block *ssparseptrsave = ss->cmd.parseptr;
         call_with_name *sscallsave = ss->cmd.callspec;
         uint32 savemisc = ss->cmd.cmd_misc_flags;
         uint32 savemisc3 = ss->cmd.cmd_misc3_flags;
         final_and_herit_flags ssheritsave = ss->cmd.cmd_final_flags;
         parse_block **save_restr_fin = ss->cmd.restrained_final;
         parse_block *save_restr_fin_ptr = *ss->cmd.restrained_final;
         call_with_name *z0callsave = z0->call;
         bool z0levelsave = z0->no_check_call_level;
         final_and_herit_flags z0heritsave = z0->more_finalherit_flags;
         call_conc_option_state saved_options = z0->options;

         if (ss->cmd.callspec) {
            z0->call = ss->cmd.callspec;
            z0->options = current_options;
         }

         ss->cmd.callspec = 0;
         z0->no_check_call_level = true;
         z0->more_finalherit_flags = ss->cmd.cmd_final_flags;

         ss->cmd.parseptr = t;
         *ss->cmd.restrained_final = ssparseptrsave;
         ss->cmd.restrained_final = 0;
         ss->cmd.cmd_misc3_flags |= CMD_MISC3__RESTRAIN_MODIFIERS;
         ss->cmd.restrained_super8flags = ss->cmd.cmd_final_flags.herit;
         ss->cmd.restrained_do_as_couples =
            (ss->cmd.cmd_misc3_flags & CMD_MISC3__DO_AS_COUPLES) != 0;
         ss->cmd.cmd_misc3_flags &= ~CMD_MISC3__DO_AS_COUPLES;
         ss->cmd.restrained_super9flags = ss->cmd.do_couples_her8itflags;
         ss->cmd.cmd_final_flags.clear_all_herit_and_final_bits();

         // Preserved across a throw; must be volatile.
         volatile error_flag_type maybe_throw_this = error_flag_none;

         try {
            move(ss, false, result);
         }
         catch(error_flag_type foo) {
            // An error occurred.  We need to restore stuff, and then throw the saved error code.
            maybe_throw_this = foo;
         }

         ss->cmd.parseptr = ssparseptrsave;
         ss->cmd.callspec = sscallsave;
         ss->cmd.cmd_misc_flags = savemisc;
         ss->cmd.cmd_misc3_flags = savemisc3;
         ss->cmd.cmd_final_flags = ssheritsave;
         ss->cmd.restrained_final = save_restr_fin;
         *ss->cmd.restrained_final = save_restr_fin_ptr;
         z0->call = z0callsave;
         z0->options = saved_options;
         z0->no_check_call_level = z0levelsave;
         z0->more_finalherit_flags = z0heritsave;
         ss->cmd.restrained_selector_decoder[0] = 0;  // Don't need this any more.
         ss->cmd.restrained_selector_decoder[1] = 0;

         if (maybe_throw_this != error_flag_none)
            throw maybe_throw_this;
      }

      goto really_getout;
   }

   if (ss->cmd.cmd_misc3_flags & CMD_MISC3__DO_AS_COUPLES) {
      if (do_forced_couples_stuff(ss, result))
         goto really_getout;
   }

   if (ss->cmd.callspec) {
      /* This next thing shouldn't happen -- we shouldn't have a call in place
         when there is a pending "centers/ends work <concept>" concept,
         since that concept should be next. */
      if (ss->cmd.cmd_misc2_flags & (CMD_MISC2__ANY_WORK | CMD_MISC2__ANY_SNAG)) {
         punt_centers_use_concept(ss, result);
         goto really_getout;
      }

      // Handle expired concepts.
      handle_expiration(ss, &resultflags_to_put_inmisc);

      move_with_real_call(ss, qtfudged, false, result);
      goto getout;
   }

   // Scan the "final" concepts, remembering them and their end point.
   last_magic_diamond = 0;

   // But if we have a pending "centers/ends work <concept>" concept, don't.

   parse_block *parseptrcopy;

   if (ss->cmd.cmd_misc2_flags & CMD_MISC2__ANY_WORK) {
      skipped_concept_info foo(ss->cmd.parseptr);

      if (foo.m_heritflag != 0) {
         parseptrcopy = foo.m_concept_with_root;
         ss->cmd.skippable_heritflags = foo.m_heritflag;
      }
      else {
         if (!foo.m_root_of_result_of_skip || foo.m_skipped_concept->concept->kind == concept_supercall)
            fail("A concept is required.");

         parseptrcopy = *foo.m_root_of_result_of_skip;
      }
   }
   else
      parseptrcopy = parseptr;

   // We will merge the new concepts with whatever we already had.

   save_incoming_final = ss->cmd.cmd_final_flags;   // In case we need to punt.
   parseptrcopy = process_final_concepts(parseptrcopy, true, &ss->cmd.cmd_final_flags, true, false);
   saved_magic_diamond = last_magic_diamond;

   // Handle expired concepts.
   handle_expiration(ss, &resultflags_to_put_inmisc);

   if (ss->cmd.cmd_misc3_flags & CMD_MISC3__IMPOSE_Z_CONCEPT) {
      distorted_2x2s_move(ss, (parse_block *) 0, result);   // It will see the flag, do the right thing, and clear it.
      canonicalize_rotation(result);
      saved_magic_diamond = (parse_block *) 0;
      goto getout;
   }

   if (parseptrcopy->concept->kind <= marker_end_of_list) {
      call_conc_option_state saved_options = current_options;
      call_with_name *this_call = parseptrcopy->call;

      // There are no "big" concepts.  The only concepts are the "little" ones
      // that have been encoded into cmd_final_flags.

      // Find out whether there is a concentric-like schema for this call.  Other schemata
      // (e.g. array) might get in the way.  Skip them.  Of course, if we take action based
      // on the concentric schema, when it comes time to do the call, it will try the array
      // schema first.  That will fail, and it will eventually get to the correct schema.

      const calldefn *search_defn = &this_call->the_defn;
      while (search_defn->schema == schema_by_array && search_defn->compound_part)
         search_defn = search_defn->compound_part;

      ss->cmd.cmd_misc2_flags &= ~CMD_MISC2__ANY_WORK_CALL_CROSSED;
      if ((ss->cmd.cmd_misc2_flags & (CMD_MISC2__ANY_WORK | CMD_MISC2__ANY_SNAG))) {
         switch (search_defn->schema) {
         case schema_cross_concentric:
            ss->cmd.cmd_misc2_flags |= CMD_MISC2__ANY_WORK_CALL_CROSSED;
         case schema_concentric:
         case schema_concentric_4_2:
         case schema_concentric_4_2_or_normal:
         case schema_concentric_or_2_6:
         case schema_concentric_or_6_2:
         case schema_concentric_2_4_or_normal:
         case schema_concentric_or_6_2_line:
         case schema_concentric_6p:
         case schema_concentric_6p_or_sgltogether:
         case schema_concentric_6p_or_normal:
         case schema_concentric_6p_or_normal_or_2x6_2x3:
         case schema_1221_concentric:
            // If the schema under which we are operating is special (e.g. 6x2),
            // but the call definition has a vanilla schema, we can't just
            // use the call definition's parts, can we?  See test nf19.
            switch ((calldef_schema) (ss->cmd.cmd_misc2_flags & 0xFFF)) {
            case schema_concentric_6_2:
            case schema_cross_concentric_6_2:
            case schema_concentric_6_2_line:
            case schema_concentric_2_6:
            case schema_cross_concentric_2_6:
            case schema_concentric_2_4:
            case schema_cross_concentric_2_4:
            case schema_concentric_4_2:
            case schema_cross_concentric_4_2:
               goto punt;
            }

            break;
         case schema_maybe_nxn_1331_lines_concentric:
         case schema_maybe_nxn_1331_cols_concentric:
         case schema_conc_o:
            break;
         default:
            goto punt;
         }
      }
      else if (ss->cmd.cmd_misc2_flags & CMD_MISC2__CENTRAL_SNAG) {
         // Doing plain snag -- we are tolerant of calls like 6x2 acey deucey.
         // But if we are doing something like "initially snag" or "finally snag", don't.
         if ((ss->cmd.cmd_fraction.flags & CMD_FRAC_PART_MASK) == 0 ||
             (((ss->cmd.cmd_fraction.flags & CMD_FRAC_CODE_MASK) != CMD_FRAC_CODE_ONLY) &&
              ((ss->cmd.cmd_fraction.flags & CMD_FRAC_CODE_MASK) != CMD_FRAC_CODE_ONLYREV))) {
            FuckingThingToTryToKeepTheFuckingStupidMicrosoftCompilerFromScrewingUp();
            switch (search_defn->schema) {
            case schema_concentric:
            case schema_concentric_6_2:
            case schema_concentric_6_2_line:
            case schema_concentric_2_6:
            case schema_concentric_4_2:
            case schema_concentric_or_2_6:
            case schema_concentric_or_6_2:
            case schema_concentric_4_2_or_normal:
            case schema_concentric_or_6_2_line:
            case schema_concentric_6p:
            case schema_concentric_6p_or_sgltogether:
            case schema_concentric_6p_or_normal:
            case schema_concentric_6p_or_normal_or_2x6_2x3:
            case schema_1221_concentric:
            case schema_conc_o:
               FuckingThingToTryToKeepTheFuckingStupidMicrosoftCompilerFromScrewingUp();
               break;
            default:
               FuckingThingToTryToKeepTheFuckingStupidMicrosoftCompilerFromScrewingUp();
               goto punt;
            }
         }
      }

      // The "anyone work" stuff will need this, along with the
      // CMD_MISC2__ANY_WORK_CALL_CROSSED bit that we set above.
      if ((ss->cmd.cmd_misc2_flags & CMD_MISC2__ANY_WORK) && (ss->cmd.skippable_heritflags == 0))
         ss->cmd.skippable_concept = ss->cmd.parseptr;

      /* We must read the selector, direction, and number out of the concept list and use them
         for this call to "move".  We are effectively using them as arguments to "move",
         with all the care that must go into invocations of recursive procedures.  However,
         at their point of actual use, they must be in global variables.  Therefore, we
         explicitly save and restore those global variables (in dynamic variables local
         to this instance) rather than passing them as explicit arguments.  By saving
         them and restoring them in this way, we make things like "checkpoint bounce
         the beaus by bounce the belles" work. */

      ss->cmd.parseptr = parseptrcopy;
      ss->cmd.callspec = this_call;
      current_options = parseptrcopy->options;

      if (((dance_level) this_call->the_defn.level) > calling_level &&
          !parseptrcopy->no_check_call_level)
         warn(warn__bad_call_level);

      if (ss->cmd.cmd_misc3_flags & CMD_MISC3__RESTRAIN_MODIFIERS) {
         ss->cmd.cmd_misc3_flags &= ~CMD_MISC3__RESTRAIN_MODIFIERS;

         if (do_heritflag_merge((uint32 *) &ss->cmd.cmd_final_flags.herit, ss->cmd.restrained_super8flags))
            fail("Illegal combination of call modifiers.");

         ss->cmd.do_couples_her8itflags = ss->cmd.restrained_super9flags;

         ss->cmd.cmd_final_flags.set_heritbits(ss->cmd.parseptr->more_finalherit_flags.herit);
         ss->cmd.cmd_final_flags.set_finalbits(ss->cmd.parseptr->more_finalherit_flags.final);

         if (ss->cmd.restrained_do_as_couples) {
            // Maybe should just set ss->cmd.cmd_misc3_flags |= CMD_MISC3__DO_AS_COUPLES;
            if (do_forced_couples_stuff(ss, result))
               goto really_getout;
         }
      }

      move_with_real_call(ss, qtfudged, false, result);
      remove_mxn_spreading(result);
      remove_tgl_distortion(result);
      current_options = saved_options;
   }
   else {
      /* We now know that there are "non-final" (virtual setup) concepts present. */

      /* If we have a pending "centers/ends work <concept>" concept,
         we must dispose of it the crude way. */

      if (ss->cmd.cmd_misc2_flags & (CMD_MISC2__ANY_WORK | CMD_MISC2__ANY_SNAG))
         goto punt;

      ss->cmd.parseptr = parseptrcopy;
      clear_result_flags(result);

      /* Most concepts simply have no understanding of, or tolerance for, modifiers
         like "interlocked" in front of them.  So, in the general case, we check for
         those modifiers and raise an error if any are on.  Hence, most of the concept
         procedures don't check for these modifiers, we guarantee that they are off.
         (There are other modifiers bits than those listed here.  They are never
         tolerated under any circumstances by any concept, and have already been
         checked.)

         Now there are two contexts in which we allow modifiers:

         Some concepts (e.g. [reverse] crazy) are explictly coded to allow,
         and check for, modifier bits.  Those are marked with the
         "CONCPROP__PERMIT_MODIFIERS" bit.  For those concepts, we don't do
         the check, and the concept code is responsible for being sure that
         everything is legal.

         Some concepts (e.g. [interlocked] phantom lines) desire to allow the
         user to type a modifier on one line and then enter another concept,
         and have us figure out what concept was really chosen.  To handle
         these, we look through a table of such pairs. */

      /* If "CONCPROP__PERMIT_MODIFIERS" is on, let anything pass. */

      parse_block artificial_parse_block;

      uint32 extraheritmods = ss->cmd.cmd_final_flags.test_heritbits(
          INHERITFLAG_REVERSE|INHERITFLAG_LEFT|INHERITFLAG_GRAND|INHERITFLAG_CROSS|
          INHERITFLAG_SINGLE|INHERITFLAG_INTLK|INHERITFLAG_DIAMOND);
      uint32 extrafinalmods = ss->cmd.cmd_final_flags.test_finalbit(FINAL__SPLIT);

      if (extraheritmods | extrafinalmods) {
         // This can only be legal if we find a translation in the table.

         const concept_fixer_thing *p;

         for (p=concept_fixer_table ; p->newheritmods | p->newfinalmods ; p++) {
            if (p->newheritmods == extraheritmods && p->newfinalmods == extrafinalmods &&
                &concept_descriptor_table[useful_concept_indices[p->before]] == ss->cmd.parseptr->concept) {
               artificial_parse_block = *ss->cmd.parseptr;
               artificial_parse_block.concept = &concept_descriptor_table[useful_concept_indices[p->after]];
               ss->cmd.parseptr = &artificial_parse_block;
               parseptrcopy = ss->cmd.parseptr;
               ss->cmd.cmd_final_flags.clear_heritbits(extraheritmods);   /* Take out those mods. */
               ss->cmd.cmd_final_flags.clear_finalbits(extrafinalmods);
               goto found_new_concept;
            }
         }
      }

   found_new_concept: ;

      // These are the concepts that we are interested in.

      final_and_herit_flags check_concepts;
      check_concepts = ss->cmd.cmd_final_flags;
      check_concepts.clear_finalbit(FINAL__MUST_BE_TAG);

      // If concept does not accept "magic" or "interlocked", we have to take
      // such modifiers seriously, and divide the setup magically.
      // Otherwise, we just do the concept.
      uint32 foobar = 0;
      uint32 fooble = 0;

      const concept_descriptor *ddd = ss->cmd.parseptr->concept;

      if (!(concept_table[ddd->kind].concept_prop & CONCPROP__PERMIT_MODIFIERS)) {
         foobar = INHERITFLAG_HALF | INHERITFLAG_LASTHALF | INHERITFLAG_DIAMOND |
            INHERITFLAG_MAGIC | INHERITFLAG_INTLK | INHERITFLAG_REVERSE;
         fooble = ~0U;

         if (ddd->kind == concept_meta) {
             if (ddd->arg1 != meta_key_finally &&
                 ddd->arg1 != meta_key_nth_part_work &&
                 ddd->arg1 != meta_key_initially_and_finally)
            foobar &= ~(INHERITFLAG_MAGIC | INHERITFLAG_INTLK);
         }

         if (concept_table[ddd->kind].concept_prop & CONCPROP__PERMIT_REVERSE)
            foobar &= ~INHERITFLAG_REVERSE;
      }

      // If there are no modifier bits that the concept can't accept, do the concept.

      if (((check_concepts.test_heritbits(foobar)) | (check_concepts.test_finalbits(fooble))) == 0) {
         if (do_big_concept(ss, ss->cmd.parseptr, true, result)) {
            canonicalize_rotation(result);
            saved_magic_diamond = (parse_block *) 0;
            goto getout;
         }
      }

      result->clear_people();

      if (ss->cmd.cmd_misc2_flags & CMD_MISC2__CTR_END_MASK)
         fail("Can't do \"invert/central/snag/mystic\" followed by another concept or modifier.");

      /* Some final concept (e.g. "magic") is present in front of our virtual setup concept.
         We have to dispose of it.  This means that expanding the matrix (e.g. 2x4->2x6)
         and stepping to a wave or rearing back from one are no longer legal. */

      ss->cmd.cmd_misc_flags |= (CMD_MISC__NO_EXPAND_MATRIX | CMD_MISC__NO_STEP_TO_WAVE);

      /* There are a few "final" concepts that
         will not be treated as such if there are non-final concepts occurring
         after them.  Instead, they will be treated as virtual setup concepts.
         This is what makes "magic once removed trade" work, for
         example.  On the other hand, if there are no non-final concepts following,
         treat these as final.
         This is what makes "magic transfer" or "split square thru" work. */

      ss->cmd.parseptr = parseptrcopy;
      ss->cmd.callspec = (call_with_name *) 0;

      // We can tolerate the "matrix" flag if we are going to do "split".
      // For anything else, "matrix" is illegal.

      if (check_concepts.final == FINAL__SPLIT && check_concepts.herit == 0) {
         uint32 split_map;

         ss->cmd.cmd_misc_flags |= CMD_MISC__SAID_SPLIT;

         if      (ss->kind == s2x4)   split_map = MAPCODE(s2x2,2,MPKIND__SPLIT,0);
         else if (ss->kind == s1x8)   split_map = MAPCODE(s1x4,2,MPKIND__SPLIT,0);
         else if (ss->kind == s_ptpd) split_map = MAPCODE(sdmd,2,MPKIND__SPLIT,0);
         else if (ss->kind == s_qtag) split_map = MAPCODE(sdmd,2,MPKIND__SPLIT,1);
         else fail("Can't do split concept in this setup.");

         ss->cmd.cmd_final_flags.clear_finalbit(FINAL__SPLIT);
         divided_setup_move(ss, split_map, phantest_ok, true, result);
      }
      else {
         if (ss->cmd.cmd_misc_flags & CMD_MISC__MATRIX_CONCEPT)
            fail("\"Matrix\" concept must be followed by applicable concept.");

         if (divide_for_magic(
               ss,
               check_concepts.test_heritbits(~INHERITFLAG_DIAMOND),
               result)) {
         }
         else if (check_concepts.herit == INHERITFLAG_DIAMOND && check_concepts.final == 0) {
            ss->cmd.cmd_final_flags.clear_heritbit(INHERITFLAG_DIAMOND);

            if (ss->kind == sdmd) {
               uint32 resflagsmisc = 0;
               if (ss->cmd.cmd_misc3_flags & CMD_MISC3__NEED_DIAMOND)
                  resflagsmisc |= RESULTFLAG__NEED_DIAMOND;
               divided_setup_move(ss, MAPCODE(s1x2,2,MPKIND__NONISOTROPDMD,0),
                                  phantest_ok, true, result);
               result->result_flags.misc |= resflagsmisc;
            }
            else {
               // Divide into diamonds and try again.
               // (Note that we back up the concept pointer.)
               ss->cmd.parseptr = parseptr;
               ss->cmd.cmd_final_flags.clear_all_herit_and_final_bits();
               divide_diamonds(ss, result);
            }
         }
         else
            fail2("Can't do this concept with other concepts preceding it:",
                  parseptrcopy->concept->menu_name);
      }
   }

 getout:

   result->result_flags.misc |= resultflags_to_put_inmisc;

   // If execution of the call raised a request that we change a concept name from "magic" to
   // "magic diamond,", for example, do so.

   if (saved_magic_diamond &&
       (result->result_flags.misc & RESULTFLAG__NEED_DIAMOND) &&
       saved_magic_diamond->concept->arg1 == 0) {
      if (saved_magic_diamond->concept->kind == concept_magic)
         saved_magic_diamond->concept = &concept_special_magic;
      else if (saved_magic_diamond->concept->kind == concept_interlocked)
         saved_magic_diamond->concept = &concept_special_interlocked;
   }

 really_getout:

   if (!suppress_fudgy_2x3_2x6_fixup) {
      if (result->kind == sfudgy2x6l || result->kind == sfudgy2x6r) {
         result->kind = s1p5x8;
      }
      else if (result->kind == sfudgy2x3l || result->kind == sfudgy2x3r) {
         result->kind = s1p5x4;
      }
   }
   else if (!(ss->cmd.cmd_misc_flags & (CMD_MISC__DISTORTED|CMD_MISC__OFFSET_Z|CMD_MISC__SAID_PG_OFFSET))) {
      // If no parallelogram-like concepts were given at this level, we allow the
      // special "1p5" setups, and turn them, with a "controversial" warning,
      // into the appropriate "fudgy" setups.
      repair_fudgy_2x3_2x6(result);
   }

   return;

 punt:

   ss->cmd.cmd_final_flags = save_incoming_final;
   punt_centers_use_concept(ss, result);
   saved_magic_diamond = (parse_block *) 0;
   goto getout;
}
