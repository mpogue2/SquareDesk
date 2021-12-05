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
   get_multiple_parallel_resultflags
   initialize_sel_tables
   initialize_fix_tables
   conc_tables::initialize
   conc_tables::analyze_this
   conc_tables::synthesize_this
   normalize_concentric
   concentric_move
   merge_table::map_tgl4l
   merge_table::map_tgl4b
   merge_table::map_2x3short
   merge_table::map_2234b
   merge_table::map_24r24a
   merge_table::map_24r24b
   merge_table::map_24r24c
   merge_table::map_24r24d
   merge_table::initialize
   merge_table::lookup
   merge_table::merge_setups
   on_your_own_move
   punt_centers_use_concept
   selective_move
   inner_selective_move
*/

#include "sd.h"


#define CONTROVERSIAL_CONC_ELONG 0x200

extern resultflag_rec get_multiple_parallel_resultflags(setup outer_inners[], int number) THROW_DECL
{
   resultflag_rec result_flags;

   bool clear_split_fields = false;   // In case we have to clear both fields.

   // If a call was being done "piecewise" or "random", we demand that both
   // calls run out of parts at the same time, and, when that happens, we
   // report it to the higher level in the recursion.

   for (int i=0 ; i<number ; i++) {
      if (!(outer_inners[i].result_flags.misc & RESULTFLAG__PARTS_ARE_KNOWN))
         outer_inners[i].result_flags.misc &= ~(RESULTFLAG__DID_LAST_PART|RESULTFLAG__SECONDARY_DONE);

      if (((outer_inners[i].result_flags.misc & outer_inners[0].result_flags.misc) & RESULTFLAG__PARTS_ARE_KNOWN) &&
            ((outer_inners[i].result_flags.misc ^ outer_inners[0].result_flags.misc) & (RESULTFLAG__DID_LAST_PART|RESULTFLAG__SECONDARY_DONE)))
         fail("Two calls must use the same number of fractions.");

      result_flags.misc |= outer_inners[i].result_flags.misc;

      if (i == 0)
         result_flags.copy_split_info(outer_inners[i].result_flags);
      else {
         // For each field (X or Y), if both setups have nonzero values but
         // those values don't match, that field gets cleared.
         if (result_flags.split_info[0] != outer_inners[i].result_flags.split_info[0] &&
             result_flags.split_info[0] != 0 &&
             outer_inners[i].result_flags.split_info[0] != 0)
            clear_split_fields = true;
         if (result_flags.split_info[1] != outer_inners[i].result_flags.split_info[1] &&
             result_flags.split_info[1] != 0 &&
             outer_inners[i].result_flags.split_info[1] != 0)
            clear_split_fields = true;

         result_flags.split_info[0] |= outer_inners[i].result_flags.split_info[0];
         result_flags.split_info[1] |= outer_inners[i].result_flags.split_info[1];
      }
   }

   if (clear_split_fields) result_flags.clear_split_info();

   return result_flags;
}


select::sel_item *select::sel_hash_table[select::NUM_SEL_HASH_BUCKETS];
select::fixer *select::fixer_ptr_table[fx_ENUM_EXTENT];

void select::initialize()
{
   sel_item *selp;
   int i;

   for (i=0 ; i<NUM_SEL_HASH_BUCKETS ; i++) sel_hash_table[i] = (sel_item *) 0;

   for (selp = sel_init_table ; selp->kk != nothing ; selp++) {
      uint32_t hash_num = (5*selp->kk) & (NUM_SEL_HASH_BUCKETS-1);
      selp->next = sel_hash_table[hash_num];
      sel_hash_table[hash_num] = selp;
   }

   fixer *fixp;

   for (i=fx0 ; i<fx_ENUM_EXTENT ; i++) fixer_ptr_table[i] = (fixer *) 0;

   for (fixp = fixer_init_table ; fixp->mykey != fx0 ; fixp++) {
      if (fixer_ptr_table[fixp->mykey])
         gg77->iob88.fatal_error_exit(1, "Fixer table initialization failed", "dup");
      fixer_ptr_table[fixp->mykey] = fixp;
   }

   for (i=fx0+1 ; i<fx_ENUM_EXTENT ; i++) {
      if (!fixer_ptr_table[i])
         gg77->iob88.fatal_error_exit(1, "Fixer table initialization failed", "undef");
   }
}


const select::fixer *select::hash_lookup(setup_kind kk, uint32_t thislivemask,
                                         bool allow_phantoms,
                                         uint32_t key, uint32_t arg, const setup *ss)
{
   uint32_t hash_num = (5*kk) & (NUM_SEL_HASH_BUCKETS-1);

   for (const sel_item *p = sel_hash_table[hash_num] ; p ; p = p->next) {
      // The livemask must match exactly unless "allow_phantoms" is on.
      if ((p->key & key) && p->kk == kk &&
          (~p->thislivemask & thislivemask) == 0 &&
          (allow_phantoms || p->thislivemask == thislivemask)) {
         const fixer *fixp = fixer_ptr_table[p->fixp];

         // We make an extremely trivial test here to see which way the distortion goes.
         // It will be checked thoroughly later.

         if (p->use_fixp2 >= 0 && ((ss->people[p->use_fixp2].id1 ^ arg) & 1))
            fixp = fixer_ptr_table[p->fixp2];

         return fixp;
      }
   }

   return (const fixer *) 0;
}



conc_tables::cm_thing *conc_tables::conc_hash_synthesize_table[conc_tables::NUM_CONC_HASH_BUCKETS];
conc_tables::cm_thing *conc_tables::conc_hash_analyze_table[conc_tables::NUM_CONC_HASH_BUCKETS];


void conc_tables::initialize()
{
   cm_thing *tabp;
   int i;

   for (i=0 ; i<NUM_CONC_HASH_BUCKETS ; i++) conc_hash_synthesize_table[i] = (cm_thing *) 0;
   for (i=0 ; i<NUM_CONC_HASH_BUCKETS ; i++) conc_hash_analyze_table[i] = (cm_thing *) 0;

   for (tabp = conc_init_table ; tabp->bigsetup != nothing ; tabp++) {

      // For synthesize.

      if ((tabp->elongrotallow & 0x100) == 0) {
         uint32_t hash_num = ((tabp->outsetup + (5*(tabp->insetup + 5*tabp->getout_schema))) * 25) & (NUM_CONC_HASH_BUCKETS-1);
         tabp->next_synthesize = conc_hash_synthesize_table[hash_num];
         conc_hash_synthesize_table[hash_num] = tabp;
      }

      // For analyze.

      if ((tabp->elongrotallow & 0x200) == 0) {
         uint32_t hash_num = ((tabp->bigsetup + (5*tabp->lyzer)) * 25) & (NUM_CONC_HASH_BUCKETS-1);

         tabp->next_analyze = conc_hash_analyze_table[hash_num];
         conc_hash_analyze_table[hash_num] = tabp;
      }

      tabp->used_mask_analyze = 0;
      tabp->used_mask_synthesize = 0;

      int mapsize = (attr::klimit(tabp->insetup)+1)*tabp->center_arity +
         (attr::klimit(tabp->outsetup)+1);

      for (i=0 ; i<mapsize ; i++) {
         if (tabp->maps[i] >= 0) {
            tabp->used_mask_analyze |= 1 << tabp->maps[i];
            tabp->used_mask_synthesize |= 1 << i;
         }
      }
   }
}


static void fix_missing_centers(
   setup *inners,
   setup *outers,
   setup_kind kki,
   setup_kind kko,
   int center_arity,
   bool enforce_kk) THROW_DECL
{
   int i;

   /* Fix up nonexistent centers, in a rather inept way. */

   for (i=0 ; i<center_arity ; i++) {
      if (inners[i].kind == nothing) {
         inners[i].kind = kki;
         inners[i].rotation = 0;
         inners[i].eighth_rotation = 0;
         inners[i].result_flags = outers->result_flags;
         inners[i].clear_people();
      }
      else if (inners[i].kind != kki && enforce_kk)
         fail("Don't recognize concentric ending setup.");
   }

   if (outers->kind != kko && enforce_kk)
      fail("Don't recognize concentric ending setup.");
}


bool conc_tables::analyze_this(
   setup *ss,
   setup *inners,
   setup *outers,
   int *center_arity_p,
   int & mapelong,
   int & inner_rot,
   int & outer_rot,
   calldef_schema analyzer)
{
   if (analyzer == schema_in_out_center_triple_z)
      analyzer = schema_in_out_triple_zcom;

   uint32_t hash_num = ((ss->kind + (5*analyzer)) * 25) &
      (conc_tables::NUM_CONC_HASH_BUCKETS-1);

   const conc_tables::cm_thing *lmap_ptr;

   for (lmap_ptr = conc_tables::conc_hash_analyze_table[hash_num] ;
        lmap_ptr ;
        lmap_ptr = lmap_ptr->next_analyze) {
      if (lmap_ptr->bigsetup == ss->kind && lmap_ptr->lyzer == analyzer) {

         for (int k=0; k<=attr::slimit(ss); k++) {
            if (ss->people[k].id1 && !(lmap_ptr->used_mask_analyze & (1<<k)))
               goto not_this_one;
         }

         int inlim = attr::klimit(lmap_ptr->insetup)+1;
         int outlim = attr::klimit(lmap_ptr->outsetup)+1;

         *center_arity_p = lmap_ptr->center_arity;
         outers->kind = lmap_ptr->outsetup;
         outers->rotation = (-lmap_ptr->outer_rot) & 3;
         outers->eighth_rotation = 0;

         gather(outers, ss, &lmap_ptr->maps[inlim*lmap_ptr->center_arity],
                outlim-1, lmap_ptr->outer_rot * 011);

         for (int m=0; m<lmap_ptr->center_arity; m++) {
            uint32_t rr = lmap_ptr->inner_rot;

            // Need to flip alternating triangles upside down.
            if (lmap_ptr->insetup == s_trngl && (m&1)) rr ^= 2;

            inners[m].clear_people();
            inners[m].kind = lmap_ptr->insetup;
            inners[m].rotation = (0-rr) & 3;
            inners[m].eighth_rotation = 0;
            gather(&inners[m], ss, &lmap_ptr->maps[m*inlim], inlim-1, rr * 011);
         }

         mapelong = lmap_ptr->mapelong;
         inner_rot = lmap_ptr->inner_rot;
         outer_rot = lmap_ptr->outer_rot;
         return true;
      }
   not_this_one: ;
   }

   return false;
}


bool conc_tables::synthesize_this(
   setup *inners,
   setup *outers,
   int center_arity,
   uint32_t orig_elong_is_controversial,
   int relative_rotation,
   uint32_t matrix_concept,
   int outer_elongation,
   calldef_schema synthesizer,
   calldef_schema orig_synthesizer,
   setup *result)
{
   int index;

   if (inners[0].kind == s_trngl || inners[0].kind == s_trngl4) {
      // For triangles, we use the "2" bit of the rotation, demanding that it be even.
      if (relative_rotation&1) fail("Ends can't figure out what spots to finish on.");
      index = (relative_rotation&2) >> 1;
   }
   else
      index = relative_rotation&1;

   // Select maps depending on 1/8 rotation stuff.  It's generally off, so get 0x400 here.
   uint32_t allowmask = ((outers->eighth_rotation&1) + 1) << 10;

   if (outer_elongation == 3)
      allowmask |= 0x10 << index;
   else if (outer_elongation <= 0 || outer_elongation > 3)
      allowmask |= 5 << index;
   else
      allowmask |= 1 << (index + ((((outer_elongation-1) ^ outers->rotation) & 1) << 1));

   if (matrix_concept) allowmask |= 0x40;

   if (orig_synthesizer == schema_rev_checkpoint_concept) allowmask |= 0x80;

   uint32_t hash_num =
      ((outers->kind + (5*(inners[0].kind + 5*synthesizer))) * 25) &
      (conc_tables::NUM_CONC_HASH_BUCKETS-1);

   const conc_tables::cm_thing *lmap_ptr;

   int q;
   int inlim, outlim;
   bool reverse_centers_order = false;

   for (lmap_ptr = conc_tables::conc_hash_synthesize_table[hash_num] ;
        lmap_ptr ;
        lmap_ptr = lmap_ptr->next_synthesize) {
      if (lmap_ptr->outsetup == outers->kind &&
          lmap_ptr->insetup == inners[0].kind &&
          lmap_ptr->center_arity == center_arity &&
          lmap_ptr->getout_schema == synthesizer &&
          (lmap_ptr->elongrotallow & allowmask) == 0) {
         // Find out whether inners need to be flipped around.
         q = relative_rotation + lmap_ptr->inner_rot - lmap_ptr->outer_rot;

         if (q & 1) fail("Sorry, bug 1 in normalize_concentric.");

         reverse_centers_order = false;

         if (synthesizer != schema_concentric_others &&
             synthesizer != schema_3x3k_concentric &&
             synthesizer != schema_3x3k_cross_concentric &&
             synthesizer != schema_4x4k_concentric &&
             synthesizer != schema_4x4k_cross_concentric) {   // This test is a crock!!!!!
            if ((outers->rotation + lmap_ptr->outer_rot + outer_elongation-1) & 2)
               reverse_centers_order = true;
         }

         inlim = attr::klimit(lmap_ptr->insetup)+1;
         outlim = attr::klimit(lmap_ptr->outsetup)+1;

         // If inner (that is, outer) setups are triangles, we won't be able to
         // turn them upside-down.  So accept it.  We don't have multiple maps
         // for such things in any case.

         if (inlim & 1) goto gotit;

         int inners_xorstuff = 0;
         if (q & 2) inners_xorstuff = inlim >> 1;

         uint32_t synth_mask = 1;

         for (int k=0; k<lmap_ptr->center_arity; k++) {
            int k_reorder = reverse_centers_order ? lmap_ptr->center_arity-k-1 : k;
            for (int j=0; j<inlim; j++) {
               if (inners[k_reorder].people[(j+inners_xorstuff)%inlim].id1 &&
                   !(lmap_ptr->used_mask_synthesize & synth_mask))
                  goto not_this_one;
               synth_mask <<= 1;
            }
         }

         for (int m=0; m<outlim; m++) {
            if (outers->people[m].id1 &&
                !(lmap_ptr->used_mask_synthesize & synth_mask))
               goto not_this_one;
            synth_mask <<= 1;
         }

         goto gotit;
      }
   not_this_one: ;
   }

   return false;

 gotit:

   q = relative_rotation + lmap_ptr->inner_rot - lmap_ptr->outer_rot;

   if (orig_elong_is_controversial) {
      // See if the table selection depended on knowing the actual elongation.
      if (lmap_ptr->elongrotallow & (5 << index))
         warn(warn_controversial);
   }

   if (reverse_centers_order) {
      if (lmap_ptr->center_arity == 2) {
         setup tt = inners[0];
         inners[0] = inners[1];
         inners[1] = tt;
      }
      else if (lmap_ptr->center_arity == 3) {
         setup tt = inners[0];
         inners[0] = inners[2];
         inners[2] = tt;
      }
   }

   const int8_t *map_indices = lmap_ptr->maps;

   for (int k=0; k<lmap_ptr->center_arity; k++) {
      uint32_t rr = lmap_ptr->inner_rot;

      // Need to flip alternating triangles upside down.
      if (lmap_ptr->insetup == s_trngl && (k&1)) rr ^= 2;

      if (q & 2) {
         inners[k].rotation += 2;
         canonicalize_rotation(&inners[k]);
      }

      if (k != 0) {
         if (((inners[k].rotation ^ inners[k-1].rotation ^ lmap_ptr->inner_rot ^ rr) & 3) != 0)
            fail("Sorry, bug 2 in normalize_concentric.");
      }

      install_scatter(result, inlim, map_indices, &inners[k], ((0-rr) & 3) * 011);
      map_indices += inlim;
   }

   install_scatter(result, outlim, map_indices, outers,
                   ((0-lmap_ptr->outer_rot) & 3) * 011);

   result->kind = lmap_ptr->bigsetup;
   result->rotation = outers->rotation + lmap_ptr->outer_rot;
   return true;
}



// This overwrites its "outer_inners" argument setups.
extern void normalize_concentric(
   const setup *ss,        // The "dyp_squash" schemata need to see this; it's allowed to be null.
   calldef_schema synthesizer,
   int center_arity,
   setup outer_inners[],   // Outers in position 0, inners follow.
   int outer_elongation,
   uint32_t matrix_concept,
   setup *result) THROW_DECL
{
   // If "outer_elongation" < 0, the outsides can't deduce their ending spots on
   // the basis of the starting formation.  In this case, it is an error unless
   // they go to some setup for which their elongation is obvious, like a 1x4.
   // The "CONTROVERSIAL_CONC_ELONG" is similar, but says that the low 2 bits
   // are sort of OK, and that a warning needs to be raised.

   int j;
   setup *inners = &outer_inners[1];
   setup *outers = &outer_inners[0];
   calldef_schema table_synthesizer = synthesizer;
   if (synthesizer == schema_rev_checkpoint_concept) table_synthesizer = schema_rev_checkpoint;
   uint32_t orig_elong_is_controversial = outer_elongation & CONTROVERSIAL_CONC_ELONG;
   outer_elongation &= ~CONTROVERSIAL_CONC_ELONG;

   result->clear_people();
   result->result_flags = get_multiple_parallel_resultflags(outer_inners, center_arity+1);

   // Only the first setup (centers) counts when check for space invasion.
   if (!(inners->result_flags.misc & RESULTFLAG__INVADED_SPACE))
       result->result_flags.misc &= ~RESULTFLAG__INVADED_SPACE;

   if (outers->kind == s2x3 && (outers->result_flags.misc & RESULTFLAG__DID_SHORT6_2X3)) {
      expand::expand_setup(s_short6_2x3, outers);
      outer_elongation = (outers->rotation & 1) + 1;
   }

   if (inners[0].kind == nothing && outers->kind == nothing) {
      result->kind = nothing;
      return;
   }

   compute_rotation_again:

   int relative_rotation = (inners[0].rotation - outers->rotation) & 3;

   // Handle the special "do your part" mechanism for "slant [] and []".
   // Then do the normal squash processing.

   if (synthesizer == schema_in_out_triple_dyp_squash) {
      setup *i0p = &inners[0];
      setup *i1p = &inners[1];

      uint32_t mask0 = little_endian_live_mask(i0p);
      uint32_t mask1 = little_endian_live_mask(i1p);

      // Convert from the outsides of a qtag to the outsides of a 2x4.
      // The mask check is so that we won't ever get into a situation in which
      // one of the formations was compressed but the other wasn't.
      // (Compress_from_hash_table does its own checking.)
      if (i0p->kind == s_qtag && i1p->kind == s_qtag && ((mask0 | mask1) & 0xCC) == 0) {
         expand::compress_from_hash_table(i0p, plain_normalize, mask0, false);
         expand::compress_from_hash_table(i1p, plain_normalize, mask1, false);
         mask0 = little_endian_live_mask(i0p);
         mask1 = little_endian_live_mask(i1p);
      }

      bool originals_were_on_left = false;
      bool originals_were_on_right = false;

      // Find out whether the active dancers were on the left side or the right side,
      // in order to check for moving to the far side.

      if (ss) {
         uint32_t ssmask = little_endian_live_mask(ss);
         if (ss->kind == s3x4) {
            if ((ssmask & 0x30C) == 0) originals_were_on_left = true;
            else if ((ssmask & 0x0C3) == 0) originals_were_on_right = true;
         }
         else if (ss->kind == sbigh) {
            if ((ssmask & 0x0C3) == 0) originals_were_on_left = true;
            else if ((ssmask & 0x30C) == 0) originals_were_on_right = true;
         }
         else if (ss->kind == sbigdmd || ss->kind == sbigbone) {
            if ((ssmask & 0x0C3) == 0) originals_were_on_left = true;
            else if ((ssmask & 0xC30) == 0) originals_were_on_right = true;
         }
      }

      bool swap_setups = false;
      bool swap_fix_ptrs = false;
      enum { fix_no_action, fix_bad, fix_1x8_to_1x4, fix_2x4_to_1x4, fix_2x4_to_2x2, fix_qtag_to_gal } todo = fix_bad;

      if (outer_elongation == 1) {
         if (i0p->kind == s2x4 && i0p->rotation == 1) {
            if ((mask0 & 0x6F) == 0 && (mask1 & 0xF6) == 0) {
               copy_person(i0p, 0, i1p, 0);
               i1p->clear_person(0);
               copy_person(i1p, 4, i0p, 4);
               i0p->clear_person(4);
               swap_setups = true;
               outer_elongation = 2;
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0x0F) == 0 && (mask1 & 0xF0) == 0) {
               todo = fix_2x4_to_1x4;
            }
            else if ((mask0 & 0xC3) == 0 && (mask1 & 0x3C) == 0) {
               if (originals_were_on_right) {
                  warn(warn__went_to_other_side);
               }
               if ((mask0 & 0x24) != 0 || (mask1 & 0x42) != 0)
                  warn(warn__this_is_tight);
               outer_elongation = 2;
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0x3C) == 0 && (mask1 & 0xC3) == 0) {
               if (originals_were_on_left) {
                  warn(warn__went_to_other_side);
               }
               if ((mask0 & 0x42) != 0 || (mask1 & 0x24) != 0)
                  warn(warn__this_is_tight);
               swap_setups = true;
               outer_elongation = 2;
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0xF0) == 0 && (mask1 & 0x0F) == 0) {
               warn(warn__went_to_other_side);
               swap_setups = true;
               todo = fix_2x4_to_1x4;
            }
         }
         else if (i0p->kind == s2x4 && i0p->rotation == 0) {
            if ((mask0 & 0x6F) == 0 && (mask1 & 0xF6) == 0) {
               copy_person(i0p, 0, i1p, 0);
               i1p->clear_person(0);
               copy_person(i1p, 4, i0p, 4);
               i0p->clear_person(4);
               swap_fix_ptrs = true;
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0xF6) == 0 && (mask1 & 0x6F) == 0) {
               copy_person(i1p, 0, i0p, 0);
               i0p->clear_person(0);
               copy_person(i0p, 4, i1p, 4);
               i1p->clear_person(4);
               swap_setups = true;
               swap_fix_ptrs = true;
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0x3C) == 0 && (mask1 & 0xC3) == 0) {
               swap_fix_ptrs = true;
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0xC3) == 0 && (mask1 & 0x3C) == 0) {
               warn(warn__went_to_other_side);
               swap_setups = true;
               swap_fix_ptrs = true;
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0x0F) == 0 && (mask1 & 0xF0) == 0) {
               if (originals_were_on_right) {
                  warn(warn__went_to_other_side);
               }
               outer_elongation = 2;
               todo = fix_2x4_to_1x4;
            }
            else if ((mask0 & 0xF0) == 0 && (mask1 & 0x0F) == 0) {
               outer_elongation = 2;
               if (originals_were_on_left) {
                  warn(warn__went_to_other_side);
               }
               swap_setups = true;
               todo = fix_2x4_to_1x4;
            }
         }
         else if (i0p->kind == s1x8 && i0p->rotation == 0) {
            if ((mask0 & 0xF0) == 0 && (mask1 & 0x0F) == 0) {
               swap_fix_ptrs = true;
               todo = fix_1x8_to_1x4;
            }
            else if ((mask0 & 0x0F) == 0 && (mask1 & 0xF0) == 0) {
               warn(warn__went_to_other_side);
               swap_setups = true;
               swap_fix_ptrs = true;
               todo = fix_1x8_to_1x4;
            }
         }
         else if (i0p->kind == s1x8 && i0p->rotation == 1) {
            if ((mask0 & 0x0F) == 0 && (mask1 & 0xF0) == 0) {
               if (originals_were_on_right) {
                  warn(warn__went_to_other_side);
               }
               outer_elongation = 2;
               todo = fix_1x8_to_1x4;
            }
            else if ((mask0 & 0xF0) == 0 && (mask1 & 0x0F) == 0) {
               if (originals_were_on_left) {
                  warn(warn__went_to_other_side);
               }
               outer_elongation = 2;
               swap_setups = true;
               todo = fix_1x8_to_1x4;
            }
         }
         else if (i0p->kind == s_qtag && i0p->rotation == 0) {
            if (mask0 == 0x41 && mask1 == 0x14) {
               todo = fix_qtag_to_gal;
            }
            else if (mask0 == 0x60 && mask1 == 0x06) {
               todo = fix_qtag_to_gal;
            }
            else if (mask0 == 0x81 && mask1 == 0x18) {
               warn(warn__this_is_tight);
               todo = fix_qtag_to_gal;
            }
            else if (mask0 == 0xA0 && mask1 == 0x0A) {
               warn(warn__this_is_tight);
               todo = fix_qtag_to_gal;
            }
         }
         else {
            // If get here, there probably were phantoms that got
            // turned into 2x2's or something.  Take no action, other than setting to schema_in_out_triple_squash.
            todo = fix_no_action;
         }
      }
      else if (outer_elongation == 2) {
         if (i0p->kind == s2x4 && i0p->rotation == 0) {
            if ((mask0 & 0x6F) == 0 && (mask1 & 0xF6) == 0) {
               copy_person(i0p, 0, i1p, 0);
               i1p->clear_person(0);
               copy_person(i1p, 4, i0p, 4);
               i0p->clear_person(4);
               outer_elongation = 1;
               swap_fix_ptrs = true;
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0x0F) == 0 && (mask1 & 0xF0) == 0) {
               todo = fix_2x4_to_1x4;
            }
            else if ((mask0 & 0xC3) == 0 && (mask1 & 0x3C) == 0) {
               if (originals_were_on_right) {
                  warn(warn__went_to_other_side);
               }
               if ((mask0 & 0x24) != 0 || (mask1 & 0x42) != 0)
                  warn(warn__this_is_tight);
               swap_setups = true;
               outer_elongation = 1;
               swap_fix_ptrs = true;
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0x3C) == 0 && (mask1 & 0xC3) == 0) {
               if (originals_were_on_left) {
                  warn(warn__went_to_other_side);
               }
               if ((mask0 & 0x42) != 0 || (mask1 & 0x24) != 0)
                  warn(warn__this_is_tight);
               outer_elongation = 1;
               swap_fix_ptrs = true;
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0xF0) == 0 && (mask1 & 0x0F) == 0) {
               warn(warn__went_to_other_side);
               swap_setups = true;
               todo = fix_2x4_to_1x4;
            }
         }
         else if (i0p->kind == s2x4 && i0p->rotation == 1) {
            if ((mask0 & 0xF6) == 0 && (mask1 & 0x6F) == 0) {
               copy_person(i1p, 0, i0p, 0);
               i0p->clear_person(0);
               copy_person(i0p, 4, i1p, 4);
               i1p->clear_person(4);
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0x6F) == 0 && (mask1 & 0xF6) == 0) {
               copy_person(i0p, 0, i1p, 0);
               i1p->clear_person(0);
               copy_person(i1p, 4, i0p, 4);
               i0p->clear_person(4);
               swap_setups = true;
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0xC3) == 0 && (mask1 & 0x3C) == 0) {
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0x3C) == 0 && (mask1 & 0xC3) == 0) {
               warn(warn__went_to_other_side);
               swap_setups = true;
               todo = fix_2x4_to_2x2;
            }
            else if ((mask0 & 0xF0) == 0 && (mask1 & 0x0F) == 0) {
               outer_elongation = 1;
               swap_setups = true;
               if (originals_were_on_right) {
                  warn(warn__went_to_other_side);
               }
               todo = fix_2x4_to_1x4;
            }
            else if ((mask0 & 0x0F) == 0 && (mask1 & 0xF0) == 0) {
               outer_elongation = 1;
               if (originals_were_on_left) {
                  warn(warn__went_to_other_side);
               }
               todo = fix_2x4_to_1x4;
            }
         }
         else if (i0p->kind == s1x8 && i0p->rotation == 1) {
            if ((mask0 & 0x0F) == 0 && (mask1 & 0xF0) == 0) {
               todo = fix_1x8_to_1x4;
            }
            else if ((mask0 & 0xF0) == 0 && (mask1 & 0x0F) == 0) {
               warn(warn__went_to_other_side);
               swap_setups = true;
               todo = fix_1x8_to_1x4;
            }
         }
         else if (i0p->kind == s1x8 && i0p->rotation == 0) {
            if ((mask0 & 0x0F) == 0 && (mask1 & 0xF0) == 0) {
               if (originals_were_on_right) {
                  warn(warn__went_to_other_side);
               }
               outer_elongation = 1;
               swap_setups = true;
               swap_fix_ptrs = true;
               todo = fix_1x8_to_1x4;
            }
            else if ((mask0 & 0xF0) == 0 && (mask1 & 0x0F) == 0) {
               if (originals_were_on_left) {
                  warn(warn__went_to_other_side);
               }
               outer_elongation = 1;
               swap_fix_ptrs = true;
               todo = fix_1x8_to_1x4;
            }
         }
         else if (i0p->kind == s_qtag && i0p->rotation == 1) {
            if (mask0 == 0x14 && mask1 == 0x41) {
               todo = fix_qtag_to_gal;
            }
            else if (mask0 == 0x06 && mask1 == 0x60) {
               todo = fix_qtag_to_gal;
            }
            else if (mask0 == 0x18 && mask1 == 0x81) {
               warn(warn__this_is_tight);
               todo = fix_qtag_to_gal;
            }
            else if (mask0 == 0x0A && mask1 == 0xA0) {
               warn(warn__this_is_tight);
               todo = fix_qtag_to_gal;
            }
         }
         else {
            // If get here, there probably were phantoms that got
            // turned into 2x2's or something.  Take no action, other than setting to schema_in_out_triple_squash.
            todo = fix_no_action;
         }
      }

      if (swap_setups) {
         setup t = *i0p;
         *i0p = *i1p;
         *i1p = t;
      }

      if (swap_fix_ptrs) {
         setup *t = i0p;
         i0p = i1p;
         i1p = t;
      }

      switch (todo) {
      case fix_1x8_to_1x4:
         i0p->swap_people(0, 6);
         i0p->swap_people(1, 7);
         i0p->swap_people(3, 5);
         i0p->swap_people(2, 4);
         i0p->kind = s1x4;
         i1p->kind = s1x4;
         break;
      case fix_2x4_to_1x4:
         i0p->swap_people(0, 7);
         i0p->swap_people(1, 6);
         i0p->swap_people(3, 5);
         i0p->swap_people(2, 4);
         i1p->swap_people(2, 3);
         i0p->kind = s1x4;
         i1p->kind = s1x4;
         break;
      case fix_2x4_to_2x2:
         i0p->swap_people(0, 2);
         i0p->swap_people(1, 3);
         i0p->swap_people(2, 4);
         i0p->swap_people(3, 5);
         i1p->swap_people(2, 6);
         i1p->swap_people(3, 7);
         i0p->kind = s2x2;
         i1p->kind = s2x2;
         break;
      case fix_qtag_to_gal:
         // Create just one outside setup, which is a diamond.
         for (j=0 ; j<8 ; j++) install_person(i1p, j, i0p, j);

         install_person(i1p, 2, i1p, 3);
         install_person(i1p, 6, i1p, 7);
         install_person(i1p, 1, i1p, 0);
         install_person(i1p, 4, i1p, 5);

         copy_rot(i0p, 0, i1p, 4, 011);
         copy_rot(i0p, 1, i1p, 6, 011);
         copy_rot(i0p, 2, i1p, 1, 011);
         copy_rot(i0p, 3, i1p, 2, 011);

         i0p->kind = sdmd;
         i0p->rotation--;
         canonicalize_rotation(i0p);

         // Change schema to normal, and compensate for the fact that the "triple"
         // schemata have reversed the centers and ends.

         center_arity = 1;
         synthesizer = schema_concentric;
         warn(warn__may_be_fudgy);

         {
            setup t = *inners;
            *inners = *outers;
            *outers = t;
         }

         goto compute_rotation_again;

      case fix_bad:
         fail("Can't figure out what to do.");
         break;
      }

      canonicalize_rotation(i0p);
      canonicalize_rotation(i1p);
      synthesizer = schema_in_out_triple_squash;
      goto compute_rotation_again;
   }

   if (synthesizer == schema_in_out_triple_squash) {
      // Do special stuff to put setups back properly for squashed schema.

      setup *i0p = &inners[0];
      setup *i1p = &inners[1];

      if (i0p->kind == s2x2) {
         // Move people to the closer parts of 2x2 setups.
         if (outer_elongation == 2) {
            if (!(i1p->people[2].id1 | i1p->people[3].id1)) {
               i1p->swap_people(0, 3);
               i1p->swap_people(1, 2);
            }
            if (!(i0p->people[0].id1 | i0p->people[1].id1)) {
               i0p->swap_people(0, 3);
               i0p->swap_people(1, 2);
            }
         }
         else if (outer_elongation == 1) {
            if (!(i1p->people[0].id1 | i1p->people[3].id1)) {
               i1p->swap_people(0, 1);
               i1p->swap_people(3, 2);
            }
            if (!(i0p->people[2].id1 | i0p->people[1].id1)) {
               i0p->swap_people(0, 1);
               i0p->swap_people(3, 2);
            }
         }

         center_arity = 2;
         table_synthesizer = schema_in_out_triple;
      }
      else if (i0p->kind == s1x4) {
         // Move people to the closer parts of 1x4 setups.
         if (i0p->rotation == 1 && outer_elongation == 2) {
            if (!(i1p->people[2].id1 | i1p->people[3].id1)) {
               i1p->swap_people(0, 3);
               i1p->swap_people(1, 2);
            }
            else if (!(i1p->people[2].id1)) {
               i1p->swap_people(2, 3);
               i1p->swap_people(1, 3);
               i1p->swap_people(1, 0);
               warn(warn__compress_carefully);
            }
            if (!(i0p->people[0].id1 | i0p->people[1].id1)) {
               i0p->swap_people(0, 3);
               i0p->swap_people(1, 2);
            }
            else if (!(i0p->people[0].id1)) {
               i0p->swap_people(1, 0);
               i0p->swap_people(1, 3);
               i0p->swap_people(2, 3);
               warn(warn__compress_carefully);
            }
         }
         else if (i0p->rotation == 0 && outer_elongation == 1) {
            if (!(i0p->people[2].id1 | i0p->people[3].id1)) {
               i0p->swap_people(0, 3);
               i0p->swap_people(1, 2);
            }
            else if (!(i0p->people[2].id1)) {
               i0p->swap_people(2, 3);
               i0p->swap_people(1, 3);
               i0p->swap_people(1, 0);
               warn(warn__compress_carefully);
            }
            if (!(i1p->people[0].id1 | i1p->people[1].id1)) {
               i1p->swap_people(0, 3);
               i1p->swap_people(1, 2);
            }
            else if (!(i1p->people[0].id1)) {
               i1p->swap_people(1, 0);
               i1p->swap_people(1, 3);
               i1p->swap_people(2, 3);
               warn(warn__compress_carefully);
            }
         }

         center_arity = 2;
         table_synthesizer = schema_in_out_triple;
      }
      else if (i0p->kind == s1x2 && i0p->rotation == 0 && outer_elongation == 2) {
         setup temp = *outers;
         const int8_t v1[] = {3, 2};
         const int8_t v2[] = {0, 1};
         scatter(outers, i0p, v1, 1, 0);
         scatter(outers, i1p, v2, 1, 0);
         outers->rotation = 0;
         outers->eighth_rotation = 0;
         outers->kind = s2x2;
         *i0p = temp;
         relative_rotation = (i0p->rotation - outers->rotation) & 3;
         center_arity = 1;
      }
      else if (i0p->kind == s1x2 && i0p->rotation == 1 && outer_elongation == 1) {
         setup temp = *outers;
         const int8_t v1[] = {0, 3};
         const int8_t v2[] = {1, 2};
         scatter(outers, i0p, v1, 1, 0);
         scatter(outers, i1p, v2, 1, 0);
         outers->rotation = 0;
         outers->eighth_rotation = 0;
         outers->kind = s2x2;
         *i0p = temp;
         relative_rotation = (i0p->rotation - outers->rotation) & 3;
         center_arity = 1;
      }
      else if (i0p->kind == s1x2 && i0p->rotation == 0 && outer_elongation == 1) {
         setup temp = *outers;
         const int8_t v1[] = {0, 1};
         const int8_t v2[] = {3, 2};
         scatter(outers, i0p, v1, 1, 0);
         scatter(outers, i1p, v2, 1, 0);
         outers->rotation = 0;
         outers->eighth_rotation = 0;
         outers->kind = s1x4;
         *i0p = temp;
         // Compute the rotation again.
         relative_rotation = (i0p->rotation - outers->rotation) & 3;
         center_arity = 1;
      }
      else if (i0p->kind == s1x2 && i0p->rotation == 1 && outer_elongation == 2) {
         setup temp = *outers;
         const int8_t v1[] = {3, 2};
         const int8_t v2[] = {0, 1};
         scatter(outers, i0p, v1, 1, 0);
         scatter(outers, i1p, v2, 1, 0);
         outers->rotation = 1;
         outers->eighth_rotation = 0;
         outers->kind = s1x4;
         *i0p = temp;
         // Compute the rotation again.
         relative_rotation = (i0p->rotation - outers->rotation) & 3;
         center_arity = 1;
      }
      else if (i0p->kind == sdmd) {
         table_synthesizer = schema_in_out_triple;
      }
      else if (i0p->kind == nothing) {
         if (outers->kind == sdmd) {
            i0p->clear_people();
            i0p->kind = s2x2;
            i0p->rotation = 0;
            i0p->eighth_rotation = 0;
            *i1p = *i0p;
            center_arity = 2;
            table_synthesizer = schema_in_out_triple;
            // Compute the rotation again.
            relative_rotation = (i0p->rotation - outers->rotation) & 3;
         }
         else {
            *i0p = *outers;
            outers->kind = nothing;
            outers->rotation = 0;
            outers->eighth_rotation = 0;
            // Compute the rotation again.
            relative_rotation = (i0p->rotation - outers->rotation) & 3;
            center_arity = 1;
         }
      }
      else
         fail("Can't figure out what to do.");
   }
   else if (synthesizer == schema_sgl_in_out_triple_squash) {

      // Do special stuff to put setups back properly for squashed schema.

      setup *i0p = &inners[0];
      setup *i1p = &inners[1];

      if (i0p->kind == s1x2) {
         // Move people to the closer parts of 1x2 setups.

         if (i0p->rotation == 1 && outer_elongation == 2) {
            if (!(i0p->people[0].id1))
               i0p->swap_people(0, 1);
            if (!(i1p->people[1].id1))
               i1p->swap_people(0, 1);
         }
         else if (i0p->rotation == 0 && outer_elongation == 1) {
            if (!(i0p->people[1].id1))
               i0p->swap_people(0, 1);
            if (!(i1p->people[0].id1))
               i1p->swap_people(0, 1);
         }

         center_arity = 2;
         table_synthesizer = schema_sgl_in_out_triple;
      }
      else
         fail("Can't figure out what to do.");
   }
   else if (synthesizer == schema_3x3_in_out_triple_squash) {

      // Do special stuff to put setups back properly for squashed schema.

      setup *i0p = &inners[0];
      setup *i1p = &inners[1];

      if (i0p->kind == s2x3) {
         // Move people to the closer parts of 2x3 setups.

         if (i0p->rotation == 0 && outer_elongation == 2) {
            if (!(i0p->people[0].id1 | i0p->people[1].id1 | i0p->people[2].id1)) {
               i0p->swap_people(0, 5);
               i0p->swap_people(1, 4);
               i0p->swap_people(2, 3);
            }
            if (!(i1p->people[3].id1 | i1p->people[4].id1 | i1p->people[5].id1)) {
               i1p->swap_people(0, 5);
               i1p->swap_people(1, 4);
               i1p->swap_people(2, 3);
            }
         }
      }
      else if (i0p->kind == nothing && outer_elongation == 2) {
         i0p->kind = s2x3;
         i0p->rotation = 0;
         i0p->eighth_rotation = 0;
         i0p->clear_people();
         i1p->kind = s2x3;
         i1p->rotation = 0;
         i1p->eighth_rotation = 0;
         i1p->clear_people();
         // Compute the rotation again.
         relative_rotation = (i0p->rotation - outers->rotation) & 3;
      }
      else
         fail("Can't figure out what to do.");

      center_arity = 2;
      table_synthesizer = schema_3x3_in_out_triple;
   }
   else if (synthesizer == schema_4x4_in_out_triple_squash) {

      // Do special stuff to put setups back properly for squashed schema.

      setup *i0p = &inners[0];
      setup *i1p = &inners[1];

      if (i0p->kind == s2x4) {
         // Move people to the closer parts of 2x4 setups.

         if (i0p->rotation == 0 && outer_elongation == 2) {
            if (!(i0p->people[0].id1 | i0p->people[1].id1 |
                  i0p->people[2].id1 | i0p->people[3].id1)) {
               i0p->swap_people(0, 7);
               i0p->swap_people(1, 6);
               i0p->swap_people(2, 5);
               i0p->swap_people(3, 4);
            }
            if (!(i1p->people[4].id1 | i1p->people[5].id1 |
                  i1p->people[6].id1 | i1p->people[7].id1)) {
               i1p->swap_people(0, 7);
               i1p->swap_people(1, 6);
               i1p->swap_people(2, 5);
               i1p->swap_people(3, 4);
            }
         }
      }
      else if (i0p->kind == nothing && outer_elongation == 2) {
         i0p->kind = s2x4;
         i0p->rotation = 0;
         i0p->eighth_rotation = 0;
         i0p->clear_people();
         i1p->kind = s2x4;
         i1p->rotation = 0;
         i1p->eighth_rotation = 0;
         i1p->clear_people();
      }
      else
         fail("Can't figure out what to do.");

      center_arity = 2;
      table_synthesizer = schema_4x4_in_out_triple;
      // Compute the rotation again.
      relative_rotation = (inners[0].rotation - outers->rotation) & 3;
   }

   if (table_synthesizer == schema_conc_o) {
      if (outers->kind != s4x4)
         fail("Outsides are not on 'O' spots.");

      if (!(outers->people[5].id1 | outers->people[6].id1 |
            outers->people[13].id1 | outers->people[14].id1))
         outer_elongation = 1;
      else if (!(outers->people[1].id1 | outers->people[2].id1 |
                 outers->people[9].id1 | outers->people[10].id1))
         outer_elongation = 2;
      else
         outer_elongation = 3;
   }

   switch (synthesizer) {
   case schema_first_only:
   case schema_second_only:
      table_synthesizer = synthesizer;
      break;
   case schema_checkpoint_spots:
      table_synthesizer = schema_rev_checkpoint;  // Yes, reverse_checkpoint tells how to go back.
      break;
   case schema_rev_checkpoint:
   case schema_rev_checkpoint_concept:
      // Fix up nonexistent centers or ends, in a rather inept way.
      if (inners[0].kind == nothing) {
         inners[0].kind = outers->kind;
         inners[0].rotation = outers->rotation;
         inners[0].eighth_rotation = 0;
         inners[0].result_flags = outers->result_flags;
         inners[0].clear_people();
         relative_rotation = 0;
      }
      else if (outers->kind == nothing) {
         outers->kind = inners[0].kind;
         outers->rotation = inners[0].rotation;
         outers->eighth_rotation = 0;
         outers->result_flags = inners[0].result_flags;
         outers->clear_people();
         relative_rotation = 0;
      }
      break;
   case schema_ckpt_star:
      // There are a few cases of centers or ends being phantoms, in which
      // we nevertheless know what to do, since we know that the setup should
      // be some kind of "winged star".
      if (inners[0].kind == nothing && outers->kind == s1x4) {
         inners[0].kind = s_star;
         inners[0].rotation = 0;
         inners[0].eighth_rotation = 0;
         inners[0].result_flags = outers->result_flags;
         inners[0].clear_people();
         goto compute_rotation_again;
      }
      else if (inners[0].kind == sdmd && outers->kind == nothing) {
         // The test case for this is: RWV:intlkphanbox relay top;splitphanbox flip reaction.
         outers->kind = s1x4;
         outers->rotation = inners[0].rotation;
         outers->eighth_rotation = 0;
         outers->result_flags = inners[0].result_flags;
         outers->clear_people();
         goto compute_rotation_again;
      }
      break;
   case schema_conc_star:
      // There are a few cases of centers or ends being phantoms, in which
      // we nevertheless know what to do, since we know that the setup should
      // be some kind of "winged star".
      if (outers->kind == nothing && inners[0].kind == s1x4) {
         outers->kind = s_star;
         outers->rotation = 0;
         outers->eighth_rotation = 0;
         outers->result_flags = inners[0].result_flags;
         outers->clear_people();
         goto compute_rotation_again;
      }
      else if (outers->kind == nothing && inners[0].kind == s_star) {
         outers->kind = s1x4;
         outers->rotation = outer_elongation-1;
         outers->eighth_rotation = 0;
         outers->result_flags = inners[0].result_flags;
         outers->clear_people();
         goto compute_rotation_again;
      }
      else if (outers->kind == s1x4 && inners[0].kind == nothing) {
         inners[0].kind = s_star;
         inners[0].rotation = 0;
         inners[0].eighth_rotation = 0;
         inners[0].result_flags = outers->result_flags;
         inners[0].clear_people();
         goto compute_rotation_again;
      }
      else if (outers->kind == sdmd && inners[0].kind == nothing) {
         inners[0].kind = s1x4;
         inners[0].rotation = 1;
         inners[0].eighth_rotation = 0;
         inners[0].result_flags = outers->result_flags;
         inners[0].clear_people();
         goto compute_rotation_again;
      }

      switch (outers->kind) {
         case s1x4:
            switch (inners[0].kind) {
               case sdmd:
                  // Just turn it into a star.
                  inners[0].kind = s_star;
                  canonicalize_rotation(&inners[0]);  // Need to do this; it has 4-way symmetry now.
                  goto compute_rotation_again;
               case s1x4:
                  /* In certain phantom cases, what should have been a diamond
                     around the outside, resulting from a 1/2 circulate, will be
                     a line with the two centers missing, since the basic_move
                     routine gives preference to a line when it is ambiguous.
                     If this happens, we have to turn it back into a diamond. */
                  if (!(outers->people[1].id1 | outers->people[3].id1)) {
                     outers->kind = sdmd;  /* That's all that it takes to fix it. */
                     goto compute_rotation_again;
                  }
                  // Or the two ends missing.
                  else if (!(outers->people[0].id1 | outers->people[2].id1)) {
                     expand::compress_setup(s_1x4_dmd, outers);
                     goto compute_rotation_again;
                  }
                  break;
            }
            break;
      }

      break;
   case schema_conc_star12:
      if (center_arity == 2) {
         fix_missing_centers(inners, outers, s_star, s1x4, center_arity, true);
      }

      break;
   case schema_conc_star16:
      if (center_arity == 3) {
         fix_missing_centers(inners, outers, s_star, s1x4, center_arity, true);
      }

      break;
   case schema_conc_12:
      if (center_arity == 2) {
         fix_missing_centers(inners, outers, s1x6, s2x3, center_arity, true);
      }
      table_synthesizer = schema_concentric;

      break;
   case schema_conc_16:
      if (center_arity == 3) {
         fix_missing_centers(inners, outers, s1x8, s2x4, center_arity, true);
      }
      table_synthesizer = schema_concentric;

      break;
   case schema_conc_bar12:
      if (center_arity == 2)
         fix_missing_centers(inners, outers, s_star, s2x3, center_arity, true);

      break;
   case schema_conc_bar16:
      if (center_arity == 3)
         fix_missing_centers(inners, outers, s_star, s2x3, center_arity, true);

      break;
   case schema_concentric_others:
      if (center_arity == 2) {
         fix_missing_centers(inners, outers, s1x2, s2x2, center_arity, false);
      }
      else if (center_arity == 3) {
         fix_missing_centers(inners, outers, s1x2, s1x2, center_arity, false);
      }
      else
         table_synthesizer = schema_concentric;

      break;
   case schema_grand_single_concentric:
      if (center_arity == 3) {
         fix_missing_centers(inners, outers, s1x2, s1x2, center_arity, true);
      }

      break;
   default:
      // Fix up nonexistent centers or ends, in a rather inept way.
      if (inners[0].kind == nothing) {
         if (table_synthesizer == schema_conc_o) {
            inners[0].kind = s2x2;
            inners[0].rotation = 0;
            inners[0].eighth_rotation = 0;
         }
         else {
            inners[0].kind = outers->kind;
            inners[0].rotation = outers->rotation;
            inners[0].eighth_rotation = 0;
         }

         inners[0].result_flags = outers->result_flags;
         inners[0].clear_people();
         for (int j=1 ; j<center_arity ; j++) inners[j] = inners[0];
         relative_rotation = 0;
      }
      else if (outers->kind == nothing) {
         if (table_synthesizer == schema_conc_o) {
            outers->kind = s4x4;
            outers->rotation = 0;
            outers->eighth_rotation = 0;
            outer_elongation = 3;
         }
         else if (table_synthesizer == schema_concentric) {
            // This makes vi01 and vi03 work.

            *result = inners[0];
            result->turn_into_deadconc();
            canonicalize_rotation(result);
            clear_result_flags(result);
            return;
         }
         else {
            outers->kind = inners[0].kind;
            outers->rotation = inners[0].rotation;
            outers->eighth_rotation = 0;
         }
         outers->result_flags = inners[0].result_flags;
         outers->clear_people();
         relative_rotation = 0;
      }

      if (table_synthesizer != schema_in_out_triple &&
          table_synthesizer != schema_sgl_in_out_triple &&
          table_synthesizer != schema_3x3_in_out_triple &&
          table_synthesizer != schema_4x4_in_out_triple &&
          table_synthesizer != schema_in_out_quad &&
          table_synthesizer != schema_in_out_12mquad &&
          table_synthesizer != schema_concentric_zs &&
          table_synthesizer != schema_concentric_big2_6 &&
          table_synthesizer != schema_concentric_6_2_tgl &&
          table_synthesizer != schema_intlk_vertical_6 &&
          table_synthesizer != schema_intlk_lateral_6 &&
          table_synthesizer != schema_3x3k_concentric &&
          table_synthesizer != schema_3x3k_cross_concentric &&
          table_synthesizer != schema_4x4k_concentric &&
          table_synthesizer != schema_4x4k_cross_concentric &&
          table_synthesizer != schema_intermediate_diamond &&
          table_synthesizer != schema_outside_diamond &&
          table_synthesizer != schema_conc_o) {
         // Nonexistent center or ends have been taken care of.
         // Now figure out how to put the setups together.

         switch (outers->kind) {
         case sbigdmd:
            switch (inners[0].kind) {
            case s1x2:
               if ((outers->people[3].id1 | outers->people[9].id1)) goto anomalize_it;
               break;
            }
            break;
         case s4x4:
            switch (inners[0].kind) {
            case s2x2:
               table_synthesizer = schema_conc_o;
               goto compute_rotation_again;
            }
            break;
         }

         table_synthesizer = schema_concentric;   // They are all in the hash table this way.
      }

      break;
   }

   // We don't allow different eighths-rotation under any circumstances (for now).

   result->eighth_rotation = outers->eighth_rotation;

   if (center_arity >= 2) {
      for (j=0; j<center_arity; j++) {
         if (inners[j].eighth_rotation != outers->eighth_rotation)
            fail("Inconsistent rotation.");
      }
   }
   else {
      if (inners[0].eighth_rotation != outers->eighth_rotation) {
         // use inners->eighth_rotation and outers->eighth_rotation directly
         goto anomalize_it;
      }

      // If the schema is plain concentric and the sub-setups are 1/8 rotated, take out
      // that rotation for the purposes of synthesize_this.  The local geometry is taken
      // care of by the lines-to-lines etc. stuff.  Note that the result rotation will
      // have the 1/8 indicator.  Note also that the consistency among the sub-setups
      // has already been taken care of.  But if the inner and outer setups are 2x3's,
      // something strange is going on, which by convention does what it does.  See t62.

      if (synthesizer == schema_concentric &&
          !(outers->kind == s2x3 && inners[0].kind == s2x3))
         outers->eighth_rotation = 0;   // Trick synthesize_this into not getting excited.
      else if (result->eighth_rotation & 1)
         warn(warn_controversial);
   }

   if (!conc_tables::synthesize_this(
         inners,
         outers,
         center_arity,
         orig_elong_is_controversial,
         relative_rotation,
         matrix_concept,
         outer_elongation,
         table_synthesizer,
         synthesizer,
         result))
      goto anomalize_it;

   if (table_synthesizer == schema_conc_o ||
       synthesizer == schema_in_out_triple_squash ||
       synthesizer == schema_in_out_triple_dyp_squash)
      normalize_setup(result, simple_normalize, qtag_compress);
   if (table_synthesizer == schema_sgl_in_out_triple)
      normalize_setup(result, normalize_to_4, qtag_compress);

   canonicalize_rotation(result);
   return;

 anomalize_it:            // Failed, just leave it as it is.

   switch (synthesizer) {
   case schema_rev_checkpoint_concept:
      fail("Sorry, can't figure out this reverse checkpoint result.");
   case schema_single_concentric:
      fail("Can't figure out this single concentric result.");
   case schema_grand_single_concentric:
      fail("Can't figure out this grand single concentric result.");
   case schema_intermediate_diamond: case schema_outside_diamond:
      fail("Can't figure out how diamond should be considered to have been oriented.");
   case schema_concentric: case schema_concentric_diamond_line:
      break;
   default:
      fail("Can't figure out this concentric result.");
   }

   if (outer_elongation <= 0 || outer_elongation > 2)
      fail("Ends can't figure out what spots to finish on.");

   if (attr::slimit(&inners[0])+1 >= MAX_PEOPLE/2 || attr::slimit(outers)+1 >= MAX_PEOPLE/2)
      fail("Can't go into this result setup.");

   result->kind = s_normal_concentric;
   result->rotation = 0;
   result->eighth_rotation = 0;
   result->inner.skind = inners[0].kind;
   result->inner.srotation = inners[0].rotation;
   result->inner.seighth_rotation = inners[0].eighth_rotation;
   result->outer.skind = outers->kind;
   result->outer.srotation = outers->rotation;
   result->outer.seighth_rotation = outers[0].eighth_rotation;
   result->concsetup_outer_elongation = outer_elongation;
   if (outers->rotation & 1) result->concsetup_outer_elongation ^= 3;
   for (j=0; j<(MAX_PEOPLE/2); j++) {
      copy_person(result, j, &inners[0], j);
      copy_person(result, j+(MAX_PEOPLE/2), outers, j);
   }
   canonicalize_rotation(result);
   clear_result_flags(result);
}


/* This sets "outer_elongation" to the absolute elongation of the
   outsides.  If the outsides are in a 2x2, this, along with individual
   facing directions, will permit enforcement of the "lines-to-lines/
   columns-to-columns" rule.  Otherwise, this will permit enforcement
   of the Hodson checkpoint rule.

   There are a few cases in which this result may seem wrong:
      (1) If we have triple diamonds with points in only the center
         diamond (that is, a line of 6 with some points hanging off
         the center 2), and we ask for the center 6/outer 2, the
         lonesome points become the ends, and "outer_elongation"
         reflects their elongation, even though the line of 6 is longer.
         This doesn't seem to affect any checkpoint or concentric cases.
         Perhaps the phrases "center 6" and "outer 2" aren't really
         correct here.
      (2) If we have a quarter tag, and we ask for the center 6/outer 2,
         the center 6 are, of course, a 2x3, and the ends of the line
         are the outer 2.  We set "outer_elongation" to reflect
         the elongation of the outer 2, which may not be what people
         would think.  Once again, this does not arise in any actual
         checkpoint or concentric case.
      (3) If we have an "H", and we ask for the center 2/outer 6, the
         outer 6 are the 2x3, and "outer_elongation" will show
         their elongation, even though that is not the elongation of
         the 3x4 matrix.  Once again, this does not arise in any actual
         checkpoint or concentric case. */

static calldef_schema concentrify(
   setup *ss,
   calldef_schema & analyzer,
   int & crossing,   // This is int (0 or 1), not bool.
   int & inverting,  // This too.
   setup_command *cmdout,
   bool enable_3x1_warn,
   bool impose_z_on_centers,
   setup inners[],
   setup *outers,
   int *center_arity_p,
   int *outer_elongation,    // Set to elongation of original outers, except if
                             // center 6 and outer 2, in which case, if centers
                             // are a bone6, it shows their elongation.
   int *xconc_elongation) THROW_DECL    // If cross concentric, set to
                                        // elongation of original ends.
{
   int i;
   calldef_schema analyzer_result = analyzer;

   *outer_elongation = 1;   //  **** shouldn't these be -1????
   *xconc_elongation = 1;
   outers->clear_people();
   inners[0].clear_people();

   // First, translate the analyzer into a form that encodes only what we
   //  need to know.  Extract the "cross concentric" info while doing this.

   crossing = (schema_attrs[analyzer_result].attrs & SCA_CROSS) ? 1 : 0;

   // Sometimes we want to copy the new schema only to "analyzer_result",
   // and sometimes we also want to copy it to "analyzer".  The purpose
   // of this distinction is lost in the mists of ancient history.
   // But it makes the proram work.

   if (schema_attrs[analyzer_result].uncrossed != schema_nothing) {
      if (schema_attrs[analyzer_result].attrs & SCA_COPY_LYZER)
         analyzer = schema_attrs[analyzer_result].uncrossed;
      analyzer_result = schema_attrs[analyzer_result].uncrossed;
   }

   // No concentric operation can operate on C1 phantoms, but a few can operate on a 4x4.
   if (ss->kind == s_c1phan) do_matrix_expansion(ss, CONCPROP__NEEDK_4X4, false);

   // It will be helpful to have a mask of where the live people are.
   uint32_t livemask = little_endian_live_mask(ss);

   // Need to do this now, so that the "schema_concentric_big2_6" stuff below will be triggered.
   if (analyzer_result == schema_concentric_6p_or_normal_or_2x6_2x3) {
      if (ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_MXNMASK) == INHERITFLAGMXNK_6X2 ||
          ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_MXNMASK) == INHERITFLAGMXNK_3X2)
         analyzer_result = schema_concentric_2_6;
      else if (attr::slimit(ss) == 5)
         analyzer_result = schema_concentric_6p;
      else
         analyzer_result = schema_concentric;
   }

   switch (analyzer_result) {
   case schema_concentric_diamond_line:
      if ((ss->kind == s_crosswave && (livemask & 0x88)) ||
          (ss->kind == s1x8 && (livemask & 0xCC)))
         fail("Can't find centers and ends in this formation.");
      break;
   case schema_concentric_6_2_line:
      if (ss->kind != s3x1dmd && ss->kind != s_wingedstar && ss->kind != s_dead_concentric)
         analyzer_result = schema_concentric_6_2;
      break;
   case schema_concentric_2_6:
      switch (ss->kind) {
      case s3x4:
         if (livemask != 07171)
            fail("Can't find centers and ends in this formation.");
         break;
      case s4dmd:
         warn(warn_big_outer_triangles);
         analyzer = schema_concentric_big2_6;
         analyzer_result = schema_concentric_big2_6;
         if (livemask != 0xC9C9)
            fail("Can't find centers and ends in this formation.");
         break;
      case s_3mdmd:
         warn(warn_big_outer_triangles);
         analyzer = schema_concentric_big2_6;
         analyzer_result = schema_concentric_big2_6;
         if (livemask != 04747)
            fail("Can't find centers and ends in this formation.");
         break;
      case s3dmd:
         // Occupations 7171 and 7474 will go to s_nftrgl6cw or s_nftrgl6ccw, respectively.
         if (livemask != 04747 && livemask != 07171 && livemask != 07474)
            fail("Can't find centers and ends in this formation.");
         break;
      case s_dmdlndmd:
         // Occupations 6565 and 7474 will go to s_nftrgl6cw or s_nftrgl6ccw, respectively.
         if (livemask != 06565 && livemask != 07474)
            fail("Can't find centers and ends in this formation.");
         break;
      default:
         break;
      }

      break;
   case schema_concentric_2_6_or_2_4:
      if (attr::slimit(ss) == 5) {
         warn(warn__unusual);
         analyzer_result = schema_concentric_2_4;
      }
      else
         analyzer_result = schema_concentric_2_6;
      break;
   case schema_concentric_2_6_or_2_4_or_2_2:
      switch (attr::slimit(ss)) {
      case 5:
         warn(warn__unusual);
         analyzer_result = schema_concentric_2_4;
         break;
      case 3:
         warn(warn__unusual);
         analyzer_result = schema_single_concentric;
         break;
      default:
         analyzer_result = schema_concentric_2_6;
         break;
      }

      break;
   case schema_concentric_6_2_or_4_2:
      if (attr::slimit(ss) == 5) {
         warn(warn__unusual);
         analyzer_result = schema_concentric_4_2;
      }
      else
         analyzer_result = schema_concentric_6_2;
      break;
   case schema_concentric_6_2_or_4_2_line:
      if (attr::slimit(ss) == 5) {
         warn(warn__unusual);
         analyzer_result = schema_concentric_4_2;
      }
      else if (ss->kind == s3x1dmd || ss->kind == s_wingedstar || ss->kind == s_dead_concentric)
         analyzer_result = schema_concentric_6_2_line;
      else
         analyzer_result = schema_concentric_6_2;
      break;
   case schema_concentric_innermost:
      if (ss->kind == s_short6) {
         warn(warn__unusual);
         analyzer_result = schema_concentric_4_2;
      }
      else if (ss->kind == s2x4 || ss->kind == s_rigger || ss->kind == s_galaxy)
         analyzer_result = schema_concentric;
      else if (attr::slimit(ss) == 5)
         analyzer_result = schema_concentric_2_4;
      else if (attr::slimit(ss) == 3)
         analyzer_result = schema_single_concentric;
      else
         analyzer_result = schema_concentric_2_6;
      break;
   case schema_concentric_touch_by_1_of_3:
      if (ss->kind == s_short6) {
         warn(warn__unusual);
         analyzer_result = schema_concentric_4_2;
      }
      else if (attr::slimit(ss) == 5)
         analyzer_result = schema_concentric_2_4;
      else if (ss->kind == s_ptpd) {
         // Check for point-to-point diamonds all facing toward other diamond.
         uint32_t dir, live;
         big_endian_get_directions32(ss, dir, live);
         if (((dir ^ 0x55FF) & live) == 0)
            analyzer_result = schema_concentric_2_6;
         else
            fail("Can't find centers and ends in this formation.");
      }
      else
         fail("Can't find centers and ends in this formation.");
      break;
   case schema_concentric_touch_by_2_of_3:
      if (ss->kind == s_galaxy) {
         // This code is duplicated in triangle_move.  Make schemata "tall6" and "short6"
         uint32_t tbonetest = ss->people[1].id1 | ss->people[3].id1 |
            ss->people[5].id1 | ss->people[7].id1;

         if ((tbonetest & 011) == 011) fail("Can't find tall 6.");
         else if (tbonetest & 1)
            analyzer_result = schema_lateral_6;
         else
            analyzer_result = schema_vertical_6;
      }
      else if (ss->kind == s_rigger)
         analyzer_result = schema_concentric_6_2;
      else if (ss->kind == s_2x1dmd)
         analyzer_result = schema_concentric_4_2_prefer_1x4;
      else if (attr::slimit(ss) == 5)
         analyzer_result = schema_concentric_4_2;
      else
         fail("Can't find centers and ends in this formation.");
      break;
   case schema_concentric:
      if (crossing && ss->kind == s4x4 && livemask != 0x9999)
         fail("Can't find centers and ends in this formation.");

      if ((ss->cmd.cmd_misc_flags & CMD_MISC__REDUCED_BY_TANDEM) != 0 && attr::klimit(ss->kind) < 4)
         analyzer_result = schema_single_concentric;
      break;
   case schema_cross_concentric:
      if ((ss->cmd.cmd_misc_flags & CMD_MISC__REDUCED_BY_TANDEM) != 0 && attr::klimit(ss->kind) < 4)
         analyzer_result = schema_single_cross_concentric;
      break;
   case schema_rev_checkpoint:
   case schema_rev_checkpoint_concept:
   case schema_conc_star:
   case schema_concentric_to_outer_diamond:
      analyzer_result = schema_concentric;
      enable_3x1_warn = false;
      if (ss->kind == s4x4 && livemask != 0x9999)
         fail("Can't find centers and ends in this formation.");
      break;
   case schema_concentric_or_diamond_line:
      if (ss->kind == s3x1dmd)
         analyzer_result = schema_concentric_diamond_line;
      else if (ss->kind == s3x4 && (livemask == 0xAEB || livemask == 0xB6D))
         analyzer_result = schema_concentric_lines_z;
      else
         analyzer_result = schema_concentric;
      break;
   case schema_concentric_or_2_6_line:
      if (ss->kind == s3x1dmd) {
         if ((livemask & 0xCC) == 0 && two_couple_calling) {
            normalize_setup(ss, normalize_to_4, qtag_no_compress);     // Get rid of the outlier.
            analyzer_result = schema_concentric_2_4;       // The pt04 case.
         }
         else
            analyzer_result = schema_concentric_6_2_line;  // The vg05 case.
      }
      else
         analyzer_result = schema_concentric;
      break;
   case schema_concentric_2_4_or_normal:
      if (attr::slimit(ss) == 5)
         analyzer_result = schema_concentric_2_4;
      else if (attr::slimit(ss) == 3)
         analyzer_result = schema_single_concentric;
      else if (ss->kind == s3x4 && (livemask == 06363 || livemask == 07474)) {
         analyzer_result = schema_in_out_triple;
         analyzer = schema_in_out_triple;
         inverting ^= 1;
      }
      else
         analyzer_result = schema_concentric;
      break;
   case schema_concentric_2_4_or_single:
      if (ss->kind == s_bone6)
         analyzer_result = schema_concentric_2_4;
      else if (ss->kind == s1x4 || ss->kind == s1x8 || ss->kind == sdmd)
         analyzer_result = schema_single_concentric;
      else
         analyzer_result = schema_concentric;
      break;
   case schema_special_trade_by:
      if (attr::slimit(ss) == 5)
         analyzer_result = schema_concentric_2_4;
      else if (attr::slimit(ss) == 3)
         analyzer_result = schema_single_concentric;
      else if (ss->kind == s3x4 && (livemask == 06363 || livemask == 07474)) {
         analyzer_result = schema_in_out_triple;
         analyzer = schema_in_out_triple;
         inverting ^= 1;
      }
      break;
   case schema_concentric_4_2_or_normal:
      if (ss->kind == s2x3)
         analyzer_result = schema_concentric_2_4;
      else if (attr::slimit(ss) == 5)
         analyzer_result = schema_concentric_4_2;
      else if (attr::slimit(ss) == 3)
         analyzer_result = schema_single_concentric;
      else
         analyzer_result = schema_concentric;
      break;
   case schema_concentric_or_2_6:
      if (ss->kind == s_323)
         analyzer_result = schema_concentric_2_6;
      else
         analyzer_result = schema_concentric;
      break;
   case schema_concentric_with_number:
      if (ss->kind == s_spindle || ss->kind == s_qtag ||
          (ss->kind == s3x4 && livemask == 06666)) {
         analyzer_result = schema_concentric_6_2;
         if (current_options.howmanynumbers == 1 && current_options.number_fields == 1)
            if (cmdout) cmdout->cmd_final_flags.set_heritbits(INHERITFLAG_SINGLE);
      }
      else if (ss->kind == s_short6) {
         analyzer_result = schema_concentric_4_2;
         if (current_options.howmanynumbers == 1 && current_options.number_fields == 1)
            if (cmdout) cmdout->cmd_final_flags.set_heritbits(INHERITFLAG_SINGLE);
      }
      else if (ss->kind == s1x8) {
         analyzer_result = schema_concentric_2_6;
         if (current_options.howmanynumbers == 1 && current_options.number_fields == 3) {
            if (cmdout) cmdout->cmd_final_flags.set_heritbits(INHERITFLAGNXNK_3X3);
         }
         else if (current_options.howmanynumbers == 1 && current_options.number_fields == 4) {
            if (cmdout) cmdout->cmd_final_flags.set_heritbits(INHERITFLAGNXNK_4X4);
            analyzer = analyzer_result = schema_first_only;
         }
      }
      else if (ss->kind == s1x6) {
         if (current_options.howmanynumbers == 1 && current_options.number_fields == 3) {
            if (cmdout) cmdout->cmd_final_flags.set_heritbits(INHERITFLAGNXNK_3X3);
            analyzer = analyzer_result = schema_first_only;
         }
         else if (current_options.howmanynumbers == 1 && current_options.number_fields == 2) {
            analyzer_result = schema_concentric_2_4;
         }
      }
      else if (ss->kind == s2x4 || ss->kind == s2x3) {
         if (current_options.howmanynumbers == 1 && current_options.number_fields == 0) {
            analyzer = analyzer_result = schema_second_only;
         }
      }
      else if (ss->kind == s_rigger) {
         if (current_options.howmanynumbers == 1 && current_options.number_fields == 2) {
            analyzer = analyzer_result = schema_concentric;
         }
      }
      break;
   case schema_concentric_6p_or_normal:
      if (attr::slimit(ss) == 5)
         analyzer_result = schema_concentric_6p;
      else
         analyzer_result = schema_concentric;
      break;
   case schema_concentric_6p_or_sgltogether:
      if (attr::slimit(ss) == 5)
         analyzer_result = schema_concentric_2_4;
      else if (ss->kind == s1x8 || ss->kind == s_ptpd ||
               attr::slimit(ss) == 3)
         analyzer_result = schema_single_concentric;
      else
         analyzer_result = schema_concentric;
      break;

   case schema_1331_concentric:
      if (ss->kind == s3x4 && (livemask & 01111) == 0) {
         // Compress to a 1/4 tag.
         ss->kind = s_qtag;
         ss->swap_people(1, 0);
         ss->swap_people(2, 1);
         ss->swap_people(4, 2);
         ss->swap_people(5, 3);
         ss->swap_people(7, 4);
         ss->swap_people(8, 5);
         ss->swap_people(10, 6);
         ss->swap_people(11, 7);
      }
      else if (ss->kind != s_qtag && ss->kind != s_spindle && ss->kind != s3x1dmd)
         analyzer_result = schema_1313_concentric;
      break;
   case schema_checkpoint_mystic_ok:
      analyzer_result = schema_checkpoint;
      break;
   case schema_single_concentric:      // Completely straightforward ones.
   case schema_3x3k_concentric:
   case schema_4x4k_concentric:
   case schema_concentric_4_2:
   case schema_grand_single_concentric:
   case schema_lateral_6:
   case schema_vertical_6:
   case schema_intlk_lateral_6:
   case schema_intlk_vertical_6:
   case schema_checkpoint:
   case schema_checkpoint_spots:
   case schema_ckpt_star:
   case schema_intermediate_diamond:
   case schema_outside_diamond:
   case schema_conc_12:
   case schema_conc_16:
   case schema_conc_star12:
   case schema_conc_star16:
   case schema_conc_bar:
   case schema_conc_bar12:
   case schema_conc_bar16:
   case schema_3x3_concentric:
   case schema_4x4_lines_concentric:
   case schema_4x4_cols_concentric:
   case schema_in_out_triple:
   case schema_inner_2x4:
   case schema_inner_2x6:
   case schema_3x3_in_out_triple:
   case schema_4x4_in_out_triple:
   case schema_sgl_in_out_triple:
   case schema_in_out_triple_squash:
   case schema_in_out_triple_dyp_squash:
   case schema_sgl_in_out_triple_squash:
   case schema_3x3_in_out_triple_squash:
   case schema_4x4_in_out_triple_squash:
   case schema_in_out_triple_zcom:
   case schema_in_out_center_triple_z:
   case schema_in_out_quad:
   case schema_in_out_12mquad:
   case schema_concentric_6_2:
   case schema_concentric_6p_or_normal_maybe_single:
   case schema_concentric_6p:
   case schema_1221_concentric:
   case schema_concentric_others:
   case schema_concentric_6_2_tgl:
   case schema_concentric_diamonds:
   case schema_concentric_2_4:
   case schema_concentric_8_4:
   case schema_concentric_zs:
   case schema_conc_o:
   case schema_concentric_ctrbox:
      break;
   case schema_concentric_specialpromenade:
   case schema_cross_concentric_specialpromenade:
      fail("You must specify who is to do it.");
   default:
      fail("Internal error: Don't understand this concentricity type.");
   }

   // Next, deal with the "normal_concentric" special case.
   // We need to be careful here.  The setup was not able to be normalized, but
   // we are being asked to pick out centers and ends.  There are very few
   // non-normal concentric setups for which we can do that correctly.  For example,
   // if we have concentric diamonds whose points are along different axes from each
   // other, who are the 4 centers of the total setup?  (If the axes of the diamonds
   // had been consistent, the setup would have been normalized to crossed lines,
   // and we wouldn't be here.)
   // If we don't take action here and the setup is normal_concentric, an error will
   // be raised, since the "concthing" entries are zero.

   if (ss->kind == s_normal_concentric || ss->kind == s_dead_concentric) {
      *center_arity_p = 1;

      if (ss->kind == s_dead_concentric) {
         ss->outer.skind = nothing;
         ss->outer.srotation = 0;
         ss->outer.seighth_rotation = 0;
      }

      setup linetemp;
      setup qtagtemp;
      setup dmdtemp;

      switch (analyzer_result) {
      case schema_conc_bar:
      case schema_concentric_diamond_line:
         if (ss->inner.skind == nothing && ss->outer.skind == s2x3) {
            inners[0].kind = s_star;
            inners[0].rotation = 0;
            inners[0].eighth_rotation = 0;
            outers->kind = s2x3;
            for (i=0; i<(MAX_PEOPLE/2); i++)
               copy_person(outers, i, ss, i+(MAX_PEOPLE/2));
            *outer_elongation = ss->concsetup_outer_elongation;
            if ((ss->outer.srotation & 1) && (*outer_elongation) != 0) *outer_elongation ^= 3;
            goto finish;
         }

         break;
      case schema_concentric_6_2_line:
         try_to_expand_dead_conc(*ss, (const call_with_name *) 0, linetemp, qtagtemp, dmdtemp);
         *ss = dmdtemp;
         break;

      case schema_concentric_2_6:
         outers->rotation = ss->outer.srotation;
         outers->eighth_rotation = 0;
         inners[0].rotation = ss->outer.srotation;   // Yes, this looks wrong, but it isn't.
         inners[0].eighth_rotation = 0;

         if (ss->outer.skind == nothing) {
            inners[0].kind = ss->inner.skind;
            inners[0].rotation = ss->inner.srotation;
            inners[0].eighth_rotation = 0;
            outers->kind = nothing;
            for (i=0; i<MAX_PEOPLE; i++)
               copy_person(&inners[0], i, ss, i);
            *outer_elongation = ss->concsetup_outer_elongation;
            goto finish;
         }
         else if (ss->inner.skind == nothing) {
            outers[0].kind = ss->outer.skind;
            outers[0].rotation = ss->outer.srotation;
            outers[0].eighth_rotation = 0;
            inners->kind = nothing;
            for (i=0; i<MAX_PEOPLE/2; i++)
               copy_person(&outers[0], i, ss, i+MAX_PEOPLE/2);
            *outer_elongation = ss->concsetup_outer_elongation;
            goto finish;
         }
         break;

      case schema_concentric:
         outers->rotation = ss->outer.srotation;
         outers->eighth_rotation = 0;
         inners[0].rotation = ss->outer.srotation;   // Yes, this looks wrong, but it isn't.
         inners[0].eighth_rotation = 0;

         if (ss->outer.skind == nothing) {
            inners[0].kind = ss->inner.skind;
            inners[0].rotation = ss->inner.srotation;
            inners[0].eighth_rotation = 0;
            outers->kind = nothing;
            for (i=0; i<MAX_PEOPLE; i++)
               copy_person(&inners[0], i, ss, i);
            *outer_elongation = ss->concsetup_outer_elongation;
            goto finish;
         }
         else if (ss->inner.skind == sdmd && ss->inner.srotation == ss->outer.srotation) {
            inners[0].kind = sdmd;
            inners[0].rotation = 0;
            inners[0].eighth_rotation = 0;
            outers->kind = ss->outer.skind;
            for (i=0; i<(MAX_PEOPLE/2); i++) {
               copy_person(&inners[0], i, ss, i);
               copy_person(outers, i, ss, i+(MAX_PEOPLE/2));
            }
         }
         else if (ss->inner.skind == s1x2 &&
                  ss->outer.skind == s1x6 &&
                  ss->inner.srotation != ss->outer.srotation) {
            static const int8_t map44[4] = {12, 13, 15, 16};

            inners[0].kind = sdmd;
            outers->kind = s1x4;

            if ((ss->inner.srotation - ss->outer.srotation) & 2) {
               copy_rot(&inners[0], 1, ss, 1, 033);
               copy_rot(&inners[0], 3, ss, 0, 033);
            }
            else {
               copy_rot(&inners[0], 1, ss, 0, 011);
               copy_rot(&inners[0], 3, ss, 1, 011);
            }

            copy_person(&inners[0], 0, ss, 14);
            copy_person(&inners[0], 2, ss, 17);
            gather(outers, ss, map44, 3, 0);
            goto finish;
         }
         break;
      }
   }

   // Next, do the 3x4 -> qtag fudging.  Don't ask permission, just do it.

   if (analyzer_result == schema_concentric && ss->kind == s3x4) {
      if ((livemask & 03131U) != 01111U) {
         *center_arity_p = 1;
         inners[0].kind = s1x4;
         inners[0].rotation = 0;
         inners[0].eighth_rotation = 0;
         outers->kind = s2x2;
         outers->rotation = 0;
         outers->eighth_rotation = 0;

         if (livemask == 07171U)
            *outer_elongation = 3;   // If occupied as "H", put them in the corners.
         else
            *outer_elongation = ((~outers->rotation) & 1) + 1;

         copy_person(&inners[0], 0, ss, 10);
         copy_person(&inners[0], 1, ss, 11);
         copy_person(&inners[0], 2, ss, 4);
         copy_person(&inners[0], 3, ss, 5);

         if (!ss->people[0].id1 && ss->people[1].id1)
            copy_person(outers, 0, ss, 1);
         else if (!ss->people[1].id1 && ss->people[0].id1)
            copy_person(outers, 0, ss, 0);
         else fail("Can't find centers and ends in this formation.");

         if (!ss->people[2].id1 && ss->people[3].id1)
            copy_person(outers, 1, ss, 3);
         else if (!ss->people[3].id1 && ss->people[2].id1)
            copy_person(outers, 1, ss, 2);
         else fail("Can't find centers and ends in this formation.");

         if (!ss->people[6].id1 && ss->people[7].id1)
            copy_person(outers, 2, ss, 7);
         else if (!ss->people[7].id1 && ss->people[6].id1)
            copy_person(outers, 2, ss, 6);
         else fail("Can't find centers and ends in this formation.");

         if (!ss->people[8].id1 && ss->people[9].id1)
            copy_person(outers, 3, ss, 9);
         else if (!ss->people[9].id1 && ss->people[8].id1)
            copy_person(outers, 3, ss, 8);
         else fail("Can't find centers and ends in this formation.");

         goto finish;
      }
   }

   if ((ss->kind == s4x4 || ss->kind == s4dmd) &&
       (analyzer_result == schema_conc_16 ||
        analyzer_result == schema_conc_star16 ||
        analyzer_result == schema_conc_bar16))
      analyzer_result = schema_4x4_cols_concentric;

   if ((ss->kind == s3x4 || ss->kind == s3dmd) &&
       (analyzer_result == schema_conc_12 ||
        analyzer_result == schema_conc_star12 ||
        analyzer_result == schema_conc_bar12))
      analyzer_result = schema_3x3_concentric;

   if (analyzer_result == schema_4x4_cols_concentric ||
       analyzer_result == schema_4x4_lines_concentric) {

      // There is a minor kludge in the tables.  For "4x4_cols" and "4x4_lines",
      // the tables don't know anything about people's facing direction,
      // so the two analyzers are used to split vertically or laterally.
      // Therefore, we translate, based on facing direction.

      if (ss->kind == s4x4) {
         uint32_t tbone1 =
            ss->people[1].id1 | ss->people[2].id1 | ss->people[9].id1 | ss->people[10].id1 |
            ss->people[3].id1 | ss->people[7].id1 | ss->people[11].id1 | ss->people[15].id1;

         uint32_t tbone2 =
            ss->people[5].id1 | ss->people[6].id1 | ss->people[13].id1 | ss->people[14].id1 |
            ss->people[3].id1 | ss->people[7].id1 | ss->people[11].id1 | ss->people[15].id1;

         if (analyzer_result == schema_4x4_cols_concentric) {
            if (!(tbone2 & 011) && (tbone1 & 001))
               analyzer_result = schema_4x4_lines_concentric;
            else if (!(tbone1 & 010) && (tbone2 & 001))
               analyzer_result = schema_4x4_lines_concentric;
            else if (tbone2 & 001)
               fail("Can't do this from T-bone setup.");
         }
         else {
            if (!(tbone1 & 011) && (tbone2 & 001))
               analyzer_result = schema_4x4_cols_concentric;
            else if (!(tbone2 & 010) && (tbone1 & 001))
               analyzer_result = schema_4x4_cols_concentric;
            else if (tbone1 & 001)
               fail("Can't do this from T-bone setup.");
         }
      }
   }

   if (analyzer_result == schema_1331_concentric)
      analyzer_result = schema_concentric_6_2;
   else if (analyzer_result == schema_1313_concentric)
      analyzer_result = schema_concentric_2_6;
   else if (analyzer_result == schema_1221_concentric)
      analyzer_result = schema_concentric_6p;
   else if (analyzer_result == schema_in_out_triple_zcom || analyzer_result == schema_in_out_center_triple_z) {
      if (impose_z_on_centers)
         analyzer_result = schema_concentric_zs;
      else if (ss->kind != s3x4 && ss->kind != s4x5/* && ss->kind != s2x5*/ && ss->kind != sd3x4)
         analyzer_result = schema_in_out_triple;
   }
   else if (analyzer_result == schema_checkpoint_spots)
      analyzer_result = schema_checkpoint;

   if (ss->kind == s_2x1dmd && analyzer_result == schema_concentric_4_2 && enable_3x1_warn)
      warn(warn__centers_are_diamond);

   if (ss->kind == s3x1dmd && analyzer_result == schema_concentric && enable_3x1_warn)
      warn(warn__centers_are_diamond);

   {
      int mapelong;
      int inner_rot;
      int outer_rot;

      if (!conc_tables::analyze_this(
            ss,
            inners,
            outers,
            center_arity_p,
            mapelong,
            inner_rot,
            outer_rot,
            analyzer_result)) {
         switch (analyzer_result) {
         case schema_concentric:
            if (ss->kind != s_normal_concentric)
               fail("Can't find centers and ends in this formation.");
            *outer_elongation = ss->concsetup_outer_elongation;
            *center_arity_p = 1;
            if (crossing)
               fail("Can't find centers and ends in this formation.");

            inners[0].kind = ss->inner.skind;
            inners[0].rotation = ss->inner.srotation;
            inners[0].eighth_rotation = ss->inner.seighth_rotation;

            outers->kind = ss->outer.skind;
            outers->rotation = ss->outer.srotation;
            outers->eighth_rotation = ss->outer.seighth_rotation;

            for (i=0; i<(MAX_PEOPLE/2); i++) {
               copy_person(&inners[0], i, ss, i);
               copy_person(outers, i, ss, i+(MAX_PEOPLE/2));
            }

            goto finish;
         case schema_special_trade_by:
         case schema_conc_bar:
         case schema_inner_2x4:
         case schema_inner_2x6:
            fail("Can't find centers and ends in this formation.");
         case schema_checkpoint:
            fail("Can't find checkpoint people in this formation.");
         case schema_concentric_2_6:
         case schema_concentric_big2_6:
            fail("Can't find 2 centers and 6 ends in this formation.");
         case schema_concentric_2_4:
            fail("Can't find 2 centers and 4 ends in this formation.");
         case schema_concentric_4_2:
            fail("Can't find 4 centers and 2 ends in this formation.");
         case schema_concentric_6_2:
         case schema_concentric_6_2_line:
            fail("Can't find 6 centers and 2 ends in this formation.");
         case schema_concentric_6p:
         case schema_1221_concentric:
            fail("Can't find centers and ends in this 6-person formation.");
         case schema_concentric_6_2_tgl:
            fail("Can't find inside triangles in this formation.");
         case schema_conc_o:
            fail("Can't find outside 'O' spots.");
         case schema_conc_12:
         case schema_conc_star12:
         case schema_conc_bar12:
            fail("Can't find 12 matrix centers and ends in this formation.");
         case schema_conc_16:
         case schema_conc_star16:
         case schema_conc_bar16:
            fail("Can't find 16 matrix centers and ends in this formation.");
         case schema_3x3_concentric:
            fail("Can't find 3x3 centers and ends in this formation.");
         case schema_4x4_lines_concentric:
            fail("Can't find 16 matrix center and end lines in this formation.");
         case schema_4x4_cols_concentric:
            fail("Can't find 16 matrix center and end columns in this formation.");
         case schema_1331_concentric:
         case schema_1313_concentric:
            fail("Can't find 3x1 concentric formation.");
         case schema_single_concentric:
            fail("Can't do single concentric in this formation.");
         case schema_grand_single_concentric:
            fail("Can't do grand single concentric in this formation.");
         case schema_in_out_triple:
         case schema_in_out_triple_dyp_squash:
         case schema_sgl_in_out_triple:
         case schema_3x3_in_out_triple:
         case schema_4x4_in_out_triple:
            fail("Can't find triple lines/columns/boxes/diamonds in this formation.");
         case schema_in_out_quad:
         case schema_in_out_12mquad:
            fail("Can't find phantom lines/columns/boxes/diamonds in this formation.");
         case schema_concentric_diamonds:
            fail("Can't find concentric diamonds.");
         case schema_concentric_zs:
            fail("Can't find concentric Z's.");
         case schema_in_out_triple_zcom:
            fail("Can't find center Z.");
         case schema_concentric_diamond_line:
            fail("Can't find center line and outer diamond.");
         default:
            fail("Wrong formation.");
         }
      }

      // Set the outer elongation to whatever elongation the outsides really had, as indicated
      // by the map.

      *outer_elongation = mapelong;
      if (outers->rotation & 1) *outer_elongation ^= 3;

      // If the concept is cross-concentric, we have to set the elongation to what
      // the centers (who will, of course, be going to the outside) had.
      // If the original centers are in a 2x2, we set it according to the orientation
      // of the entire 2x4 they were in, so that they can think about whether they were
      // in lines or columns and act accordingly.  If they were not in a 2x4, that is,
      // the setup was a wing or galaxy, we set the elongation to -1 to indicate an
      // error.  In such a case the centers won't be able to decide whether they were
      // in lines or columns.

      // The following additional comment used to be present, back when we obeyed a misguided
      // notion of what cross concentric means:
      // "But if the outsides started in a 1x4, they override the centers' own axis."

      switch (analyzer) {
      case schema_concentric:
      case schema_concentric_diamonds:
      case schema_concentric_zs:   // This may not be right.
      case schema_concentric_6p_or_normal:
      case schema_concentric_6p_or_normal_or_2x6_2x3:
         if (crossing) {
            *xconc_elongation = inner_rot+1;

            switch (ss->kind) {
            case s4x4:
               if (inners[0].kind == s2x2) *xconc_elongation = 3;
               break;
            case s_galaxy:
               *xconc_elongation = -1;    // Can't do this!
               break;
            case s_rigger:
               *xconc_elongation = -1;
               if (outers->kind == s1x4)
                  *xconc_elongation = (outer_rot+1) | CONTROVERSIAL_CONC_ELONG;
               break;
            }
         }
         break;

      case schema_in_out_triple:
      case schema_sgl_in_out_triple:
      case schema_3x3_in_out_triple:
      case schema_4x4_in_out_triple:
      case schema_in_out_triple_squash:
      case schema_in_out_triple_dyp_squash:
      case schema_sgl_in_out_triple_squash:
      case schema_3x3_in_out_triple_squash:
      case schema_4x4_in_out_triple_squash:
      case schema_in_out_quad:
      case schema_in_out_12mquad:
      case schema_in_out_triple_zcom:
      case schema_conc_o:
         *outer_elongation = mapelong;   // The map defines it completely.
         break;
      case schema_concentric_6_2_tgl:
         if (inners[0].kind == s_bone6)
            *outer_elongation = ((~outers->rotation) & 1) + 1;
         break;
      case schema_concentric_6_2:
         if (inners[0].kind == s_bone6)
            *outer_elongation = (outers->rotation & 1) + 1;
         break;
      }
   }

 finish:

   canonicalize_rotation(outers);
   canonicalize_rotation(&inners[0]);
   if (*center_arity_p >= 2)
      canonicalize_rotation(&inners[1]);
   if (*center_arity_p == 3)
      canonicalize_rotation(&inners[2]);

   return analyzer_result;
}




warning_index concwarneeetable[] = {warn__lineconc_perp, warn__xclineconc_perpe, warn__lineconc_par};
warning_index concwarn1x4table[] = {warn__lineconc_perp, warn__xclineconc_perpc, warn__lineconc_par};
warning_index concwarndmdtable[] = {warn__dmdconc_perp, warn__xcdmdconc_perpc, warn__dmdconc_par};


static bool fix_empty_outers(
   setup_kind sskind,
   setup_kind & final_outers_start_kind,
   uint32_t & localmods1,
   int crossing,
   int begin_outer_elongation,
   int center_arity,
   calldef_schema & analyzer,
   setup_command *cmdin,
   setup_command *cmdout,
   setup *begin_outer,
   setup *result_outer,
   setup *result_inner,
   setup *result)
{
   // If the schema is one of the special ones, we will know what to do.
   if (analyzer == schema_conc_star ||
       analyzer == schema_ckpt_star ||
       analyzer == schema_conc_star12 ||
       analyzer == schema_conc_star16) {

      // This is what makes 12 matrix relay the top work when everyone is
      // in the stars.

      result_outer->kind = s1x4;
      result_outer->clear_people();
      clear_result_flags(result_outer);
      result_outer->rotation = 0;
      result_outer->eighth_rotation = 0;
   }
   else if (analyzer == schema_conc_bar ||
            analyzer == schema_conc_bar12 ||
            analyzer == schema_conc_bar16) {

      // This is what makes 12 matrix quarter the deucey work
      // when everyone is in the stars.

      result_outer->kind = s2x3;
      result_outer->clear_people();
      clear_result_flags(result_outer);
      result_outer->result_flags.misc |= 1;
      result_outer->rotation = 1;
      result_outer->eighth_rotation = 0;
   }
   else if (analyzer == schema_checkpoint || analyzer == schema_checkpoint_mystic_ok) {
      result_outer->kind = s2x2;
      result_outer->clear_people();
      clear_result_flags(result_outer);
      result_outer->result_flags.misc = 1;
      result_outer->rotation = 0;
      result_outer->eighth_rotation = 0;
   }
   else if (analyzer == schema_rev_checkpoint) {
      result_outer->kind = s1x4;
      result_outer->clear_people();
      clear_result_flags(result_outer);
      result_outer->rotation = result_inner->rotation;
      result_outer->eighth_rotation = 0;
   }
   else if (analyzer == schema_in_out_triple_squash) {
      result_outer->kind = s2x2;
      result_outer->clear_people();
      clear_result_flags(result_outer);
      result_outer->rotation = 0;
      result_outer->eighth_rotation = 0;
   }
   else if (analyzer == schema_in_out_triple_dyp_squash) {
      result_outer->kind = s2x2;
      result_outer->clear_people();
      clear_result_flags(result_outer);
      result_outer->rotation = 0;
      result_outer->eighth_rotation = 0;
   }
   else if (analyzer == schema_concentric_diamond_line) {
      switch (sskind) {
      case s_wingedstar:
      case s_wingedstar12:
      case s_wingedstar16:
      case s_barredstar:
      case s_barredstar12:
      case s_barredstar16:
      case s3x1dmd:
         result_outer->kind = s2x2;
         result_outer->rotation = 0;
         result_outer->eighth_rotation = 0;
         result_outer->clear_people();
         // Set their "natural" elongation perpendicular to their original diamond.
         // The test for this is 1P2P; touch 1/4; column circ; boys truck; split phantom
         // lines tag chain thru reaction.  They should finish in outer triple boxes,
         // not a 2x4.
         result_outer->result_flags = result_inner->result_flags;
         result_outer->result_flags.misc &= ~3;
         result_outer->result_flags.misc |= 2;
         break;
      default:
         fail("Can't figure out ending setup for concentric call -- no ends.");
      }
   }
   else {
      uint32_t result_outer_result_flags_misc = result_outer->result_flags.misc;
      uint32_t orig_elong_flags = result_outer->result_flags.misc & 3;

      // We may be in serious trouble -- we have to figure out what setup the ends
      // finish in, and they are all phantoms.
      *result_outer = *begin_outer;          // Restore the original bunch of phantoms.
      clear_result_flags(result_outer);
      // If setup is 2x2 and a command "force spots" or "force otherway" was given, we can
      // honor it, even though there are no people present in the outer setup.
      if (final_outers_start_kind == s2x2 &&
          result_outer->kind == s2x2 &&
          (localmods1 & (DFM1_CONC_FORCE_SPOTS | DFM1_CONC_FORCE_OTHERWAY))) {
         ;        // Take no further action.
      }
      else if (result_outer->kind == s4x4 && analyzer == schema_conc_o) {
         if (sskind == s2x4 &&
             final_outers_start_kind == s4x4 &&
             cmdout &&
             cmdout->cmd_fraction.is_null() &&
             cmdout->callspec == base_calls[base_call_two_o_circs]) {
            result_outer->kind = s2x2;
            analyzer = schema_concentric;
            final_outers_start_kind = s2x2;
            localmods1 |= DFM1_CONC_FORCE_OTHERWAY;
         }

         // Otherwise, take no action.
      }
      else if (final_outers_start_kind == s1x4 &&
               result_outer->kind == s1x4 &&
               ((localmods1 & (DFM1_CONC_FORCE_SPOTS | DFM1_CONC_FORCE_OTHERWAY)) != 0)) {
         // If a call starts in a 1x4 and has "force spots" or "force otherway" indicated,
         // it must go to a 2x2 with same elongation.
         result_outer->kind = s2x2;    // Take no further action.
      }
      else if (final_outers_start_kind == s1x4 &&
               result_outer->kind == s1x4 &&
               (result_outer_result_flags_misc & RESULTFLAG__EMPTY_1X4_TO_2X2) != 0) {
         // If a call started in a 1x4 and would have gone to a 2x2 and had
         // "parallel_conc_end" indicated, it must go to a 2x2 with same elongation.
         result_outer->kind = s2x2;
         result_outer->result_flags.misc = result_outer_result_flags_misc & 3;
      }
      else if (final_outers_start_kind == s1x4 &&
               !crossing &&
               (orig_elong_flags ^ begin_outer_elongation) == 3 &&
               (localmods1 & DFM1_CONC_CONCENTRIC_RULES)) {
         // If a call starts in a 1x4 but tries to set the result elongation to the
         // opposite of the starting elongation, it must have been trying to go to a 2x2.
         // In that case, the "opposite elongation" rule applies.
         result_outer->kind = s2x2;    // Take no further action.
      }
      else if (final_outers_start_kind == s1x2 &&
               result_outer->kind == s1x2 &&
               ((orig_elong_flags+1) & 2)) {
         // Note that the "desired elongation" is the opposite
         // of the rotation, because, for 1x2 calls, the final
         // elongation is opposite of what it should be.
         // (Why?  So that, if we specify "parallel_conc_end", it
         // will be correct.  Why?  Because counter rotate has that
         // flag set in order to get the right 2x2 behavior, and
         // this way we get right 1x2 behavior also.  Isn't that
         // a stupid reason?  Yes.)  (See the documentation
         // of the "parallel_conc_end" flag.)  So we do what
         // seems to be the wrong thing.
         result_outer->rotation = orig_elong_flags & 1;
         result_outer->eighth_rotation = 0;
      }
      else {
         // Otherwise, we can save the day only if we can convince
         // ourselves that they did the call "nothing", or a very few
         // other calls.  We make use of the fact that "concentrify"
         // did NOT flush them, so we still know what their starting
         // setup was.  This is what makes split phantom diamonds
         // diamond chain through work from columns far apart, among
         // other things.

         call_with_name *the_call = begin_outer->cmd.callspec;   // Might be null pointer.

         // ***** We have a bug (well, a not completely correct situation) here.
         // From a split phantom boxes with everyone as ends of waves in their
         // split phantom boxes, if we say "split phantom boxes change lanes",
         // it should refuse to do it unless we said "assume waves" or something
         // like that.  Otherwise, how does it know that the phantom centers
         // aren't T-boned, and hence go into the other spaces when they spread?
         // (BTW, if we are in columns far apart and we say "split phantom waves
         // change lanes", there is an implicit "assume waves", so we should win
         // anyway.)
         //
         // Now the correct way to handle this is *NOT* to change localmods1 in
         // the way it is done just below, in the cases of "nothing", "nothing_noroll",
         // "trade", or "remake".  Leave it as it is in those cases.  (And probably
         // other cases too.)  We then need to have "fix_empty_outers" (that's us) set
         // "final_outers_finish_dirs" with the directions those phantoms would have
         // had if (a) we know that the call was one of the four that we are discussing,
         // and (b) we know, from an assumption, what direction those phantoms should
         // have been known to have at the start.
         // (Note that "final_outers_finish_dirs"  was computed to be zero just before
         // we got called.  It would need to be passed in to us as a writeable variable.)
         //
         // But this is no worse than lots of other places where this program, and
         // humans, make assumptions about "normal" situations.

         // Make sure these people go to the same spots, and remove possibly misleading info.
         localmods1 |= DFM1_CONC_FORCE_SPOTS;
         localmods1 &= ~(DFM1_CONC_FORCE_LINES |
                         DFM1_CONC_FORCE_COLUMNS |
                         DFM1_CONC_FORCE_OTHERWAY);

         if (the_call && (the_call->the_defn.schema == schema_nothing ||
                          the_call->the_defn.schema == schema_nothing_noroll ||
                          the_call->the_defn.schema == schema_nothing_other_elong))
            ;        // It's OK, the call was "nothing".
         else if (the_call == base_calls[base_call_trade] ||
                  the_call == base_calls[base_call_passin] ||
                  the_call == base_calls[base_call_passout] ||
                  the_call == base_calls[base_call_any_hand_remake])
            ;        // It's OK, the call was "trade" or "any hand remake".
         else if (the_call == base_calls[base_call_circulate] &&
                  cmdout && cmdout->cmd_final_flags.bool_test_heritbits(INHERITFLAG_HALF) &&
                  cmdout->cmd_fraction.is_null()) {
            // The call is "1/2 circulate".  We assume it goes to a diamond.
            // We don't check that the facing directions are such that this is
            // actually true.  Too bad.
            // Well, it actually goes to a star.  It's the best we can do, given that
            // we don't know facing directions.  In practice, that's good enough.
            localmods1 &= ~DFM1_CONC_FORCE_SPOTS;  // We don't want this after all.
            result_outer->kind = s_star;
         }
         else if ((the_call == base_calls[base_base_hinge] ||
                   the_call == base_calls[base_base_hinge_for_nicely] ||
                   the_call == base_calls[base_base_hinge_with_warn] ||
                   the_call == base_calls[base_base_hinge_for_breaker] ||
                   the_call == base_calls[base_base_hinge_and_then_trade] ||
                   the_call == base_calls[base_base_hinge_and_then_trade_for_breaker]) &&
                  ((orig_elong_flags+1) & 2)) {
            if (result_outer->kind == s1x4) {
               result_outer->kind = s2x2;
               result_outer->rotation = 0;
               result_outer->eighth_rotation = 0;
            }
            else {
               result_outer->kind = s1x4;
               result_outer->rotation = (orig_elong_flags & 1) ^ 1;
               result_outer->eighth_rotation = 0;
            }
         }
         else if (center_arity > 1)
            ;        // It's OK.
         else {
            // We simply have no idea where the outsides should be.  We
            // simply contract the setup to a 4-person setup (or whatever),
            // throwing away the outsides completely.  If this was an
            // "on your own", it may be possible to put things back together.
            // This is what makes "1P2P; pass thru; ENDS leads latch on;
            // ON YOUR OWN disband & snap the lock" work.  But if we try to glue
            // these setups together, "fix_n_results" will raise an error, since
            // it won't know whether to leave room for the phantoms.
            *result = *result_inner;  // This gets all the inner people, and the result_flags.
            result->turn_into_deadconc();

            // This used to set concsetup_outer_elongation to begin_outer_elongation
            // instead of zero.  It was explained with a comment:
            //     "We remember a vague awareness of where the outside would have been."
            // It is no longer done that way.  We enforce precise rules, with no notions
            // of a "vague awareness".

            result->concsetup_outer_elongation = 0;
            return true;
         }
      }
   }

   // If we are doing something like "triple boxes",
   // we really want the setups to look alike.
   if (cmdin == cmdout && analyzer == schema_in_out_triple) {
      result_outer->kind = result_inner[0].kind;
      result_outer->rotation = result_inner[0].rotation;
      result->eighth_rotation = 0;
      result_outer->clear_people();  // Do we need this?  Maybe the result setup got bigger.
   }

   return false;
}


static bool this_call_preserves_shape(const setup *begin,
                                      const call_conc_option_state & call_options,
                                      setup_kind sss)
{
   if (!begin->cmd.callspec) return false;

   const calldefn *def = &begin->cmd.callspec->the_defn;

   switch (def->schema) {
   case schema_nothing: case schema_nothing_noroll: case schema_nothing_other_elong: return true;
   case schema_by_array:
      {
         // Calls like "arm turn N/4" are special.  Look at the number that it was given.
         // It preserves shape if that number is even.  These calls are characterized
         // by having "yoyo_fractal_numbers".
         if (def->callflags1 & CFLAG1_YOYO_FRACTAL_NUM)
            return sss == s1x2 && (call_options.number_fields & 1) == 0;

         if ((def->stuff.arr.def_list->callarray_list->callarray_flags & CAF__ROT) != 0 ||
             def->stuff.arr.def_list->callarray_list->get_end_setup() != sss)
            return false;

         begin_kind bb = def->stuff.arr.def_list->callarray_list->get_start_setup();

         if (bb == setup_attrs[sss].keytab[0] || bb == setup_attrs[sss].keytab[1]) return true;
         else return false;
      }
   default: return false;
   }
}



static bool fix_empty_inners(
   setup_kind orig_inners_start_kind,
   int center_arity,
   int begin_outer_elongation,
   calldef_schema analyzer,
   calldef_schema analyzer_result,
   setup *begin_inner,
   setup *result_outer,
   setup *result_inner,
   const call_conc_option_state & call_options,
   setup *result)
{
   for (int i=0 ; i<center_arity ; i++) {
      result_inner[i].clear_people();    // This is always safe.
      clear_result_flags(&result_inner[i]);
   }

   if (analyzer == schema_conc_star &&
       analyzer_result == schema_concentric &&
       result_outer->kind == sdmd &&
       center_arity == 1 &&
       begin_outer_elongation == 1 &&
       begin_inner->kind == s2x2 &&
       begin_inner->cmd.callspec &&
       begin_inner->cmd.callspec == base_calls[base_call_armturn_n4] &&
       (call_options.number_fields & 1) != 0) {

      // This is the "centers cast 3/4" as the second part of relay the top.
      // This isn't really correct, because we don't know that the center phantom 2x2
      // isn't T-boned to the others except by virtue of the trade that preceded.
      // But it's good enough--the human outsides will assume that they are facing appropriately.

      // Restore the original bunch of phantoms.
      *result_inner = *begin_inner;
      result_inner->kind = s1x4;
      result_inner->rotation = 1;
      result_inner->eighth_rotation = 0;
      clear_result_flags(result_inner);
   }
   else if (analyzer == schema_conc_star ||   // If the schema is one of the special ones, we will know what to do.
            analyzer == schema_ckpt_star ||
            analyzer == schema_conc_star12 ||
            analyzer == schema_conc_star16 ||
            analyzer == schema_in_out_triple ||
            analyzer == schema_sgl_in_out_triple_squash ||
            analyzer == schema_sgl_in_out_triple ||
            analyzer == schema_3x3_in_out_triple_squash ||
            analyzer == schema_3x3_in_out_triple ||
            analyzer == schema_4x4_in_out_triple_squash ||
            analyzer == schema_4x4_in_out_triple ||
            analyzer == schema_in_out_quad ||
            analyzer == schema_in_out_12mquad) {
      // Take no action.
   }
   else if (analyzer == schema_in_out_triple_squash && center_arity == 2) {
      result_inner[0].kind = s2x2;
      result_inner[0].rotation = 0;
      result_inner[0].eighth_rotation = 0;
      result_inner[1].kind = s2x2;
      result_inner[1].rotation = 0;
      result_inner[1].eighth_rotation = 0;
   }
   else if (analyzer == schema_in_out_triple_dyp_squash && center_arity == 2) {
      result_inner[0].kind = s2x2;
      result_inner[0].rotation = 0;
      result_inner[0].eighth_rotation = 0;
      result_inner[1].kind = s2x2;
      result_inner[1].rotation = 0;
      result_inner[1].eighth_rotation = 0;
   }
   else if (analyzer == schema_conc_bar && result_outer->kind == s2x3) {
      // Fix some quarter the deucey stuff.
      switch (orig_inners_start_kind) {
      case s2x2:
         result_inner->kind = s1x4;
         result_inner->rotation = result_outer->rotation;
         result_inner->eighth_rotation = 0;
         break;
      case s_star:
         result_inner->kind = s_star;
         result_inner->rotation = 0;
         result_inner->eighth_rotation = 0;
         break;
      }
   }
   else if (center_arity == 1 &&
            (result_outer->kind == s2x2 ||
             (result_outer->kind == s4x4 && analyzer == schema_conc_o))) {
      // If the ends are a 2x2, we just set the missing centers to a 2x2.
      // The ends had better know their elongation, of course.  It shouldn't
      // matter to the ends whether the phantoms in the center did something
      // that leaves the whole setup as diamonds or as a 2x4.  (Some callers
      // might think it matters (Hi, Clark!) but it doesn't matter to this program.)
      // This is what makes split phantom diamonds diamond chain through work
      // from a tidal wave.
      // Also, if doing "O" stuff, it's easy.  Do the same thing.
      // Also, if the call was "nothing", or something for which we know the
      // ending setup and orientation, follow same.
      uint32_t orig_elong_flags = result_outer->result_flags.misc & 3;

      if (analyzer_result == schema_concentric_6p && ((orig_elong_flags+1) & 2)) {
         result_inner->kind = s1x2;
         result_inner->rotation = orig_elong_flags & 1;
         result_inner->eighth_rotation = 0;
      }
      else {
         if (this_call_preserves_shape(begin_inner, call_options, s1x2) ||
             this_call_preserves_shape(begin_inner, call_options, s2x2) ||
             this_call_preserves_shape(begin_inner, call_options, s1x4)) {
            // Restore the original bunch of phantoms.
            *result_inner = *begin_inner;
            clear_result_flags(result_inner);
         }
         else {
            result_inner->kind = s2x2;
            result_inner->rotation = 0;
            result_inner->eighth_rotation = 0;
         }
      }
   }
   else if (result_outer->kind == s_bone6 && center_arity == 1) {
      // If the ends are a bone6, we just set the missing centers to a 1x2,
      // unless the missing centers did "nothing", in which case they
      // retain their shape.
      if (this_call_preserves_shape(begin_inner, call_options, s1x2)) {
         // Restore the original bunch of phantoms.
         *result_inner = *begin_inner;
         clear_result_flags(result_inner);
      }
      else {
         result_inner->kind = s1x2;
         result_inner->rotation = result_outer->rotation;
         result_inner->eighth_rotation = 0;
      }
   }
   else if (result_outer->kind == s1x4 && center_arity == 1) {
      // If the ends are a 1x4, we just set the missing centers to a 1x4,
      // unless the missing centers did "nothing", in which case they
      // retain their shape.
      setup_kind inner_start = (analyzer_result == schema_concentric_6p) ? s1x2 : s1x4;

      if (this_call_preserves_shape(begin_inner, call_options, inner_start)) {
         // Restore the original bunch of phantoms.
         *result_inner = *begin_inner;
         clear_result_flags(result_inner);
      }
      else {
         result_inner->kind = inner_start;
         result_inner->rotation = result_outer->rotation;
         result_inner->eighth_rotation = 0;
      }
   }
   else if (result_outer->kind == s1x2 &&
            (analyzer == schema_single_concentric)) {
      // A similar thing, for single concentric.
      result_inner->kind = s1x2;
      result_inner->rotation = result_outer->rotation;
      result_inner->eighth_rotation = 0;
   }
   else if (result_outer->kind == s1x6 && analyzer == schema_concentric_2_6) {
      // If the ends are a 1x6, we just set the missing centers to a 1x2,
      // so the entire setup is a 1x8.  Maybe the phantoms went the other way,
      // so the setup is really a 1x3 diamond, but we don't care.  See the comment
      // just above.
      result_inner->kind = s1x2;
      result_inner->rotation = result_outer->rotation;
      result_inner->eighth_rotation = 0;
   }
   else if (result_outer->kind == s_short6 &&
            analyzer_result == schema_concentric_2_6 &&
            begin_inner->kind == s1x2) {
      // If the ends are a short6, (presumably the whole setup was a qtag or hrglass),
      // and the missing centers were an empty 1x2, we just restore that 1x2.
      *result_inner = *begin_inner;
      clear_result_flags(result_inner);
   }
   else if (result_outer->kind == s_star &&
            analyzer_result == schema_concentric_2_4 &&
            begin_inner->kind == s1x2) {
      // Similarly, for 6-person arrangements.
      *result_inner = *begin_inner;
      clear_result_flags(result_inner);
   }
   else if (center_arity == 1) {
      // The centers are just gone!

      // First, look for special case of outers are a 4x4 or galaxy with no one in the center.
      if ((result_outer->kind == s4x4 && (little_endian_live_mask(result_outer) & 0x8888) == 0) ||
          (result_outer->kind == s_galaxy && (little_endian_live_mask(result_outer) & 0xAA) == 0)) {
         *result = *result_outer;
         return true;
      }

      // It is quite possible that "fix_n_results" may be able to repair this damage by
      // copying some info from another setup.  Missing centers are not as serious as
      // missing ends, because they won't lead to indecision about whether to leave
      // space for the phantoms.

      int j;
      *result = *result_outer;   // This gets the result_flags.
      clear_result_flags(result);

      // This should, among other things, make "run away" more palatable.
      if (analyzer_result == schema_single_concentric &&
          center_arity == 1 &&
          begin_inner->kind == s1x2) {
         return false;
      }

      // Otherwise, we have to set this to an unknown concentric formulation,
      // which doesn't make anyone happy.
      result->kind = s_normal_concentric;
      result->rotation = 0;
      result->eighth_rotation = 0;
      result->outer.skind = result_outer->kind;
      result->outer.srotation = result_outer->rotation;
      result->outer.seighth_rotation = result_outer->eighth_rotation;
      result->inner.skind = nothing;
      result->inner.srotation = 0;
      result->inner.seighth_rotation = 0;
      result->concsetup_outer_elongation = 0;

      for (j=0; j<MAX_PEOPLE/2; j++)
         copy_person(result, j+MAX_PEOPLE/2, result_outer, j);

      return true;
   }

   return false;
}


static void inherit_conc_assumptions(
   setup_kind sskind,
   setup_kind beginkind,
   calldef_schema analyzer,
   int really_doing_ends,
   assumption_thing *this_assumption)
{
   if (analyzer == schema_concentric ||
       analyzer == schema_concentric_6p_or_normal ||
       analyzer == schema_concentric_4_2_or_normal ||
       analyzer == schema_concentric_2_4_or_normal) {
      if (sskind == s2x4 && beginkind == s2x2) {
         switch (this_assumption->assumption) {
         case cr_wave_only:
            /* Waves [wv/0/0] or normal columns [wv/1/0] go to normal boxes [wv/0/0]. */
            this_assumption->assump_col = 0;
            goto got_new_assumption;
         case cr_magic_only:
            if (this_assumption->assump_col == 0) {
               /* Inv lines [mag/0/0] go to couples_only [cpl/0/0]. */
               this_assumption->assumption = cr_couples_only;
            }
            else {
               /* Magic cols [mag/1/0] go to normal boxes [wv/0/0]. */
               this_assumption->assumption = cr_wave_only;
               this_assumption->assump_col = 0;
            }
            goto got_new_assumption;
         case cr_2fl_only:
            if (this_assumption->assump_col == 0) {
               /* 2FL [2fl/0/0] go to normal boxes [wv/0/0]. */
               this_assumption->assumption = cr_wave_only;
            }
            else {
               /* DPT/CDPT [2fl/1/x] go to facing/btb boxes [lilo/0/x]. */
               this_assumption->assumption = cr_li_lo;
               this_assumption->assump_col = 0;
            }
            goto got_new_assumption;
         case cr_li_lo:
            if (this_assumption->assump_col == 0) {
               /* facing/btb lines [lilo/0/x] go to facing/btb boxes [lilo/0/x]. */
            }
            else {
               /* 8ch/tby [lilo/1/x] go to facing/btb boxes [lilo/0/y], */
               this_assumption->assumption = cr_li_lo;
               this_assumption->assump_col = 0;
               /* Where calculation of whether facing or back-to-back is complicated. */
               if (!really_doing_ends)
                  this_assumption->assump_both ^= 3;
            }
            goto got_new_assumption;
         case cr_1fl_only:
            if (this_assumption->assump_col == 0) {
               // 1-faced lines [1fl/0/0] go to couples_only [cpl/0/0].
               this_assumption->assumption = cr_couples_only;
               goto got_new_assumption;
            }
            break;
         case cr_quarterbox:
         case cr_threequarterbox:
            if (really_doing_ends) {
               // Ends go to back-to-back couples [lilo/0/2]
               this_assumption->assumption = cr_li_lo;
               this_assumption->assump_col = 0;
               this_assumption->assump_both = 2;
            }
            else {
               // Centers go to normal box [wv/0/x]
               this_assumption->assumption = cr_wave_only;
               this_assumption->assump_col = 0;
            }
            break;
         }
      }
      if (sskind == s1x8 && beginkind == s1x4) {
         switch (this_assumption->assumption) {
         case cr_wave_only:
         case cr_2fl_only:
         case cr_1fl_only:
            goto got_new_assumption;
         }
      }
      else if (sskind == s_qtag &&
               beginkind == s2x2 &&
               this_assumption->assump_col == 0) {
         switch (this_assumption->assumption) {
         case cr_jright:
         case cr_jleft:
         case cr_ijright:
         case cr_ijleft:
            /* 1/4 tag or line [whatever/0/2] go to facing in [lilo/0/1]. */
            this_assumption->assumption = cr_li_lo;
            /* 3/4 tag or line [whatever/0/1] go to facing out [lilo/0/2]. */
            this_assumption->assump_both ^= 3;
            goto got_new_assumption;

         case cr_qtag_like:
            // Outsides facing in (B=1) or out (B=2), centers in unknown line,
            // though we don't casre about that just now.
            if (((this_assumption->assump_both + 1) & 2) != 0) {   // 1 or 2
               this_assumption->assumption = cr_li_lo;
               goto got_new_assumption;
            }
            else
               break;

         case cr_real_1_4_tag:
         case cr_real_1_4_line:
            this_assumption->assumption = cr_li_lo;
            this_assumption->assump_both = 1;
            goto got_new_assumption;
         case cr_real_3_4_tag:
         case cr_real_3_4_line:
            this_assumption->assumption = cr_li_lo;
            this_assumption->assump_both = 2;
            goto got_new_assumption;

         case cr_ctr_miniwaves:
         case cr_ctr_couples:
            /* Either of those special assumptions means that the outsides
               are in a normal box. */
            this_assumption->assumption = cr_wave_only;
            this_assumption->assump_col = 0;
            this_assumption->assump_both = 0;
            goto got_new_assumption;
         }
      }
      else if (sskind == s_qtag && beginkind == s1x4) {
         switch (this_assumption->assumption) {
         case cr_ctr_miniwaves:
            this_assumption->assumption = cr_wave_only;
            this_assumption->assump_col = 0;
            this_assumption->assump_both = 0;
            goto got_new_assumption;
         case cr_ctr_couples:
            this_assumption->assumption = cr_2fl_only;
            this_assumption->assump_col = 0;
            this_assumption->assump_both = 0;
            goto got_new_assumption;
         }
      }
   }
   else if (analyzer == schema_concentric_2_6 || analyzer == schema_concentric_6p_or_normal_or_2x6_2x3) {
      if ((sskind == s_qtag || sskind == s_ptpd) &&
          beginkind == s_short6 && really_doing_ends) {
         // We want to preserve "assume diamond" stuff to the outer 6,
         // so 6x2 and 3x2 acey deucey will work.
         // Of course, it will have a different meaning.  "Assume
         // normal diamonds" will mean "this is a wave-based triangle,
         // with consistent direction".  "Assume facing diamonds" will
         // mean "this is a wave-based triangle, with the base in a
         // real mini-wave, but the apex pointing inconsistently".
         goto got_new_assumption;
      }
   }
   else if (analyzer == schema_ckpt_star &&
            really_doing_ends == 1 &&
            sskind == s_spindle &&
            this_assumption->assumption == cr_ckpt_miniwaves) {
      // The box is a real box.  This makes the hinge win on chain reaction.
      this_assumption->assumption = cr_wave_only;
      goto got_new_assumption;
   }
   else if (analyzer == schema_checkpoint &&
            sskind == s1x8 &&
            this_assumption->assumption == cr_wave_only &&
            this_assumption->assump_col == 0) {
      if (really_doing_ends == 0)
         this_assumption->assump_both = 0;   // Handedness changed -- if we weren't lazy, we would do this right.
      goto got_new_assumption;
   }
   else if (analyzer == schema_single_concentric) {
      if (sskind == s1x4 && beginkind == s1x2 &&
          (this_assumption->assumption == cr_2fl_only ||
           this_assumption->assumption == cr_wave_only)) {
         this_assumption->assumption = cr_wave_only;
         goto got_new_assumption;
      }
   }

   this_assumption->assumption = cr_none;

 got_new_assumption: ;
}

static const expand::thing fix_cw  = {{1, 2, 4, 5}, s2x2, s2x3, 0};
static const expand::thing fix_ccw = {{0, 1, 3, 4}, s2x2, s2x3, 0};
static const expand::thing fix_2x3_r = {{1, 2, 3, 4}, s2x2, s2x3, 0};
static const expand::thing fix_2x3_l = {{0, 1, 4, 5}, s2x2, s2x3, 0};


extern void concentric_move(
   setup *ss,
   setup_command *cmdin,
   setup_command *cmdout,
   calldef_schema analyzer,
   uint32_t modifiersin1,
   uint32_t modifiersout1,
   bool recompute_id,
   bool enable_3x1_warn,
   uint32_t specialoffsetmapcode,
   setup *result) THROW_DECL
{
   if (ss->cmd.cmd_misc2_flags & CMD_MISC2__DO_NOT_EXECUTE) {
      clear_result_flags(result);
      result->kind = nothing;
      return;
   }

   clean_up_unsymmetrical_setup(ss);

   if (analyzer == schema_concentric_no31dwarn) {
      analyzer = schema_concentric;
      enable_3x1_warn = false;
   }

   setup begin_inner[3];
   setup begin_outer;
   int begin_outer_elongation;
   int begin_xconc_elongation;
   int center_arity;
   uint32_t rotstate, pointclip;
   calldef_schema analyzer_result;
   int rotate_back = 0;
   int i, k, klast;
   bool ends_are_in_situ = false;
   int crossing;       // This is an int (0 or 1) rather than a bool,
                       // because we will index with it.

   // The original info about the people who STARTED on the inside.
   setup_kind orig_inners_start_kind;
   uint32_t orig_inners_start_dirs;
   uint32_t orig_inners_start_directions[32];

   // The original info about the people who STARTED on the outside.
   setup_kind orig_outers_start_kind;
   uint32_t orig_outers_start_dirs;
   uint32_t orig_outers_start_directions[32];

   // The original info about the people who will FINISH on the outside.
   setup_kind final_outers_start_kind;
   uint32_t *final_outers_start_directions;

   // The final info about the people who FINISHED on the outside.
   int final_outers_finish_dirs;
   uint32_t final_outers_finish_directions[32];
   uint32_t ccmask, eemask;

   const call_conc_option_state save_state = current_options;
   uint32_t save_cmd_misc2_flags = ss->cmd.cmd_misc2_flags;

   parse_block *save_skippable_concept = ss->cmd.skippable_concept;
   ss->cmd.skippable_concept = (parse_block *) 0;
   heritflags save_skippable_heritflags = ss->cmd.skippable_heritflags;
   ss->cmd.skippable_heritflags = 0ULL;

   const uint64_t scrnxn =
      ss->cmd.cmd_final_flags.test_heritbits(INHERITFLAG_NXNMASK | INHERITFLAG_MXNMASK);

   ss->cmd.cmd_misc2_flags &= ~(0xFFF | CMD_MISC2__ANY_WORK_INVERT |
                                CMD_MISC2__ANY_WORK | CMD_MISC2__ANY_SNAG);

   // It is clearly too late to expand the matrix -- that can't be what is wanted.
   ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;

   // But, if we thought we weren't sure enough of where people were to allow stepping
   // to a wave, we are once again sure.
   ss->cmd.cmd_misc_flags &= ~CMD_MISC__NO_STEP_TO_WAVE;

   // If there is a restrained concept that will presumably be lifted during the execution
   // of the centers' and ends' part of the call, and that concept refers to a selector
   // that refers to the setup, take special action.  The selector refers to the setup
   // *now*, not the setups that will result from the splitting.
   //
   // The cases of this are things like "finally snag the ends, bits and pieces".
   //
   // Some calls are hierarchically defined as sequential-over-concentric, and some are
   // the other way around.  There are good (messy, but good) reasons why this has to be so.
   // Bits and pieces is concentric-over-sequential.  If invoked with something like
   // "finally snag <someone>", it will come here first with no restraint or other special
   // stuff, and we will do the first part.  We will be called again with fraction info
   // saying to do the last part, lifting a restraint.  But, since this is concentric-over-sequential
   // we will divide into centers and ends first, and then go to do_sequential_call separately
   // for the centers and ends.  If the selector was "girls", there will be no problem.  But if
   // it was "ends", we need to evaluate that right now, because the user wanted it to refer
   // to the whole setup.

   if (ss->cmd.restrained_concept && (ss->cmd.cmd_misc3_flags & CMD_MISC3__RESTRAIN_CRAZINESS)) {
      // We have a restrained concept for which the restraint has not been lifted.
      selector_kind sel = ss->cmd.restrained_concept->options.who.who[0];
      if (sel != selector_uninitialized) {
         if ((ss->cmd.restrained_selector_decoder[0] | ss->cmd.restrained_selector_decoder[1]) != 0)
            fail("Concept nesting is too complicated.");    // Really shouldn't happen.

         who_list little_saved_selector = current_options.who;
         current_options.who.who[0] = sel;

         int nump = 0;
         for (i=0; i<=attr::slimit(ss); i++) {
            if (ss->people[i].id1) {
               if (nump == 8) fail("?????too many people????");

               bool fubb = selectp(ss, i);
               ss->cmd.restrained_selector_decoder[0] <<= 8;
               ss->cmd.restrained_selector_decoder[0] |= ss->cmd.restrained_selector_decoder[1] >> 24;
               ss->cmd.restrained_selector_decoder[1] <<= 8;
               // Set each byte to 4 bits of full person id (that's 6 bits), 0 bit, selection bit.
               ss->cmd.restrained_selector_decoder[1] |= ((ss->people[i].id1 & (BIT_PERSON|XPID_MASK)) >> 4) |
                  (fubb ? 1 : 0);
               nump++;
            }
         }

         current_options.who = little_saved_selector;
      }
   }

   if (ss->kind == s_qtag &&
       analyzer != schema_concentric_6_2 &&
       (ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_12_MATRIX) ||
        scrnxn == INHERITFLAGMXNK_1X3 ||
        scrnxn == INHERITFLAGMXNK_3X1 ||
        scrnxn == INHERITFLAGNXNK_3X3))
      do_matrix_expansion(ss, CONCPROP__NEEDK_3X4, true);

   for (i=0; i<32; i++) {
      orig_inners_start_directions[i] =
      orig_outers_start_directions[i] =
      final_outers_finish_directions[i] = 0;
   }

   if (save_cmd_misc2_flags & CMD_MISC2__CTR_END_MASK) {
      if (save_cmd_misc2_flags & CMD_MISC2__CENTRAL_SNAG) {
         if (!ss->cmd.cmd_fraction.is_null())
            fail("Can't do fractional \"snag\".");
      }

      if (!(save_cmd_misc2_flags & CMD_MISC2__DO_CENTRAL))
         ss->cmd.cmd_misc2_flags &= ~CMD_MISC2__CTR_END_MASK;
   }

   if (analyzer == schema_single_cross_concentric_together_if_odd) {
      analyzer = schema_single_cross_concentric;     // Setup was already split.
   }
   else if (analyzer == schema_single_concentric_together) {
      if (ss->kind == s1x8 || ss->kind == s_ptpd || ss->kind == s4x4 || attr::slimit(ss) == 3)
         analyzer = schema_single_concentric;
      else if (ss->kind == s_bone6)
         analyzer = schema_concentric_6p;
      else
         analyzer = schema_concentric;
   }
   else if (analyzer == schema_single_cross_concentric_together) {
      if (ss->kind == s1x8 || ss->kind == s_ptpd || ss->kind == s4x4 || attr::slimit(ss) == 3)
         analyzer = schema_single_cross_concentric;
      else
         analyzer = schema_cross_concentric;
   }

   if (schema_attrs[analyzer].attrs & SCA_REMOVE_VERIFY) {
      // These "verify" things can't possibly be meaningful after this point.
      ss->cmd.cmd_misc_flags &= ~CMD_MISC__VERIFY_MASK;
   }

   begin_inner[0].cmd = ss->cmd;
   begin_inner[1].cmd = ss->cmd;
   begin_inner[2].cmd = ss->cmd;
   begin_outer.cmd = ss->cmd;

   int inverting = 0;

   bool imposing_z = cmdin && ((cmdin->cmd_misc3_flags & CMD_MISC3__IMPOSE_Z_CONCEPT) != 0);

   // This reads and writes to "analyzer" and "inverting", and writes to "crossing".
   analyzer_result = concentrify(ss, analyzer, crossing, inverting, cmdout, enable_3x1_warn,
                                 imposing_z,
                                 begin_inner, &begin_outer, &center_arity,
                                 &begin_outer_elongation, &begin_xconc_elongation);

   // But reverse them if doing "invert".
   if (save_cmd_misc2_flags & CMD_MISC2__SAID_INVERT)
      inverting ^= 1;

   uint32_t localmodsin1 = modifiersin1;
   uint32_t localmodsout1 = modifiersout1;

   if (inverting) {
      localmodsin1 = modifiersout1;
      localmodsout1 = modifiersin1;
   }

   // Get initial info for the original ends.
   orig_outers_start_dirs = 0;
   for (i=0, k=1, eemask=0;
        i<=attr::klimit(begin_outer.kind);
        i++, k<<=1) {
      uint32_t q = begin_outer.people[i].id1;
      orig_outers_start_dirs |= q;
      if (q) {
         eemask |= k;
         orig_outers_start_directions[(q >> 6) & 037] = q;
      }
   }
   orig_outers_start_kind = begin_outer.kind;

   // Get initial info for the original centers.
   orig_inners_start_dirs = 0;
   for (i=0, k=1, ccmask=0;
        i<=attr::klimit(begin_inner[0].kind);
        i++, k<<=1) {
      uint32_t q = begin_inner[0].people[i].id1;
      orig_inners_start_dirs |= q;
      if (q) {
         ccmask |= k;
         orig_inners_start_directions[(q >> 6) & 037] = q;
      }
   }
   orig_inners_start_kind = begin_inner[0].kind;

   bool suppress_overcasts = crossing != 0 || (schema_attrs[analyzer].attrs & SCA_NO_OVERCAST) != 0;

   if (crossing) {
      setup temptemp = begin_inner[0];
      begin_inner[0] = begin_outer;
      begin_outer = temptemp;

      if ((analyzer == schema_grand_single_concentric || analyzer == schema_4x4k_concentric) && center_arity == 3) {
         temptemp = begin_inner[2];
         begin_inner[2] = begin_inner[1];
         begin_inner[1] = temptemp;
      }

      uint32_t ttt = ccmask;
      ccmask = eemask;
      eemask = ttt;
      final_outers_start_kind = orig_inners_start_kind;
      final_outers_start_directions = orig_inners_start_directions;
   }
   else {
      final_outers_start_kind = orig_outers_start_kind;
      final_outers_start_directions = orig_outers_start_directions;
   }

   // If the call turns out to be "detour", this will make it do just the ends part.

   if (!(schema_attrs[analyzer].attrs & SCA_CONC_REV_ORDER))
      begin_outer.cmd.cmd_misc3_flags |= CMD_MISC3__DOING_ENDS;

   // There are two special pieces of information we now have that will help us decide
   // where to put the outsides.  "Orig_outers_start_kind" tells what setup the outsides
   // were originally in, and "begin_outer_elongation" tells how the outsides were
   // oriented (1=horiz, 2=vert).  "begin_outer_elongation" refers to absolute orientation,
   // that is, "our" view of the setups, taking all rotations into account.
   // "final_outers_start_directions" gives the individual orientations (absolute)
   // of the people who are finishing on the outside.  Later, we will compute
   // "final_outers_finish_dirs", telling how the individual people were oriented.
   // How we use all this information depends on many things that we will attend to below.

   // Giving one of the concept descriptor pointers as nil indicates that
   // we don't want those people to do anything.

   // We will now do the parts, controlled by the counter k.  There are multiple
   // instances of doing the centers' parts, indicated by k = 0, 1, ..... center_arity-1.
   // (Normally, center_arity=1, so we have just k=0).  There is just one instance
   // of doing the ends' part, indicated by k not in [0..center_arity-1].

   // Now this is made complicated by the fact that we sometimes want to do
   // things in a different order.  Usually, we want to do the centers first and
   // the ends later, but, when the schema is triple lines or something similar,
   // we want to do the ends first and the centers later.  This has to do with
   // the order in which we query the user for "slant <anything> and
   // <anything>".  So, in the normal case, we let k run from 0 to center_arity,
   // inclusive, with the final value for the ends.  When the schema is triple
   // lines, we let k run from -1 to center_arity-1, with the initial value for
   // the ends.

   k = 0;
   klast = center_arity+1;

   if (schema_attrs[analyzer].attrs & SCA_CONC_REV_ORDER) {
      k = -1;
      klast = center_arity;

      if (save_cmd_misc2_flags & CMD_MISC2__CENTRAL_MYSTIC) {
         // Need to switch this, because we are reversing
         // who we think are "centers" and "ends".
         save_cmd_misc2_flags ^= CMD_MISC2__INVERT_MYSTIC;
      }
   }

   // We only allow the "matrix" concept for the "REV_ORDER" schemata, and only
   // if "cmdin" (which contains the *outsides'* command in such a case) is
   // null.  That is we allow it only for stuff like "center phantom columns".

   uint32_t matrix_concept = ss->cmd.cmd_misc_flags & CMD_MISC__MATRIX_CONCEPT;
   begin_outer.cmd.cmd_misc_flags &= ~CMD_MISC__MATRIX_CONCEPT;

   if (matrix_concept && (cmdin || k == 0))
      fail("\"Matrix\" concept must be followed by applicable concept.");

   // Slot 0 has outers; Slot 1 has inners.  If arity > 1, slots 2 and 3 have extra inners.
   setup outer_inners[4];
   call_conc_option_state outer_inner_options[4];
   for (i=0 ; i<4 ; i++) outer_inner_options[i].initialize();

   warning_info saved_ctr_warnings = configuration::save_warnings();
   warning_info saved_end_warnings = configuration::save_warnings();
   warning_info saved_all_warnings = configuration::save_warnings();

   for (; k<klast; k++) {
      int doing_ends = (k<0) || (k==center_arity);
      setup *begin_ptr = doing_ends ? &begin_outer : &begin_inner[k];
      setup *result_ptr = doing_ends ? &outer_inners[0] : &outer_inners[k+1];
      call_conc_option_state *option_ptr = doing_ends ? &outer_inner_options[0] : &outer_inner_options[k+1];
      uint32_t modifiers1 = doing_ends ? localmodsout1 : localmodsin1;
      setup_command *cmdptr = (doing_ends ^ inverting) ? cmdout : cmdin;
      uint32_t ctr_use_flag = CMD_MISC2__ANY_WORK;

      if (doing_ends ^ (((save_cmd_misc2_flags & CMD_MISC2__ANY_WORK_CALL_CROSSED) != 0) ? 1 : 0))
         ctr_use_flag |= CMD_MISC2__ANY_WORK_INVERT;

      current_options = save_state;

      // We allow "quick so-and-so shove off"!

      if (analyzer != schema_in_out_triple_squash &&
          analyzer != schema_in_out_triple &&
          analyzer != schema_in_out_quad &&
          analyzer != schema_in_out_12mquad &&
          ((analyzer != schema_concentric &&
            analyzer != schema_concentric_6_2 &&
            analyzer != schema_concentric_2_6) ||
           doing_ends || center_arity != 1))
         begin_ptr->cmd.cmd_misc_flags |= CMD_MISC__DISTORTED;

      if (cmdptr) {
         uint32_t mystictest;
         bool demand_no_z_stuff = false;
         begin_ptr->cmd.parseptr = cmdptr->parseptr;
         begin_ptr->cmd.callspec = cmdptr->callspec;
         begin_ptr->cmd.cmd_final_flags = cmdptr->cmd_final_flags;
         begin_ptr->cmd.cmd_fraction = cmdptr->cmd_fraction;
         begin_ptr->cmd.restrained_fraction = cmdptr->restrained_fraction;
         begin_ptr->cmd.cmd_misc_flags &= ~CMD_MISC__VERIFY_MASK;
         begin_ptr->cmd.cmd_misc_flags |= (CMD_MISC__VERIFY_MASK & cmdptr->cmd_misc_flags);

         // If a restrained fraction got promoted, turn off this flag.
         if (begin_ptr->cmd.restrained_fraction.fraction != 0 && cmdptr->restrained_fraction.fraction == 0)
            begin_ptr->cmd.cmd_misc2_flags &= ~CMD_MISC2_RESTRAINED_SUPER;

         // If doing something under a "3x1" (or "1x3") concentric schema,
         // put the "3x3" flag into the 6-person call, whichever call that is,
         // and "single" into the other one.
         if (analyzer == schema_1331_concentric) {
            if (attr::slimit(begin_ptr) == 5) {
               begin_ptr->cmd.cmd_final_flags.clear_heritbits(INHERITFLAG_NXNMASK);
               begin_ptr->cmd.cmd_final_flags.set_heritbits(INHERITFLAGNXNK_3X3);
            }
            else
               begin_ptr->cmd.cmd_final_flags.set_heritbits(INHERITFLAG_SINGLE);
         }
         else if (analyzer == schema_1221_concentric) {
            if (attr::slimit(begin_ptr) == 1)
               begin_ptr->cmd.cmd_final_flags.set_heritbits(INHERITFLAG_SINGLE);
         }

         // Check for operating on a Z.

         uint32_t z_compress_direction99rot = begin_ptr->rotation;
         uint32_t z_compress_direction99skew;

         if (begin_ptr->kind == s2x3 &&
             (analyzer == schema_concentric_zs ||
              analyzer == schema_in_out_triple_zcom ||
              analyzer == schema_in_out_center_triple_z)) {
            z_compress_direction99skew = doing_ends ? eemask : ccmask;
            begin_ptr->cmd.cmd_misc2_flags |= CMD_MISC2__REQUEST_Z;
            demand_no_z_stuff = (analyzer == schema_concentric);
         }

         if (doing_ends) {
            configuration::restore_warnings(saved_end_warnings);

            // If the operation isn't being done with "DFM1_CONC_CONCENTRIC_RULES"
            // (that is, this is some implicit operation), we allow the user
            // to give the explicit "concentric" concept.  We respond to that concept
            // simply by turning on "DFM1_CONC_CONCENTRIC_RULES".  This makes
            // things like "shuttle [concentric turn to a line]" work.

            if (!inverting && !begin_ptr->cmd.callspec &&
                !(localmodsout1 & DFM1_CONC_CONCENTRIC_RULES)) {
               final_and_herit_flags junk_concepts;
               junk_concepts.clear_all_herit_and_final_bits();
               parse_block *next_parseptr;

               next_parseptr = process_final_concepts(begin_ptr->cmd.parseptr,
                                                      false, &junk_concepts, true, false);

               if (!junk_concepts.test_for_any_herit_or_final_bit() &&
                   next_parseptr->concept->kind == concept_concentric) {
                  localmodsout1 |= DFM1_CONC_CONCENTRIC_RULES;
                  begin_ptr->cmd.parseptr = next_parseptr->next;
               }

               if (next_parseptr->concept->kind == marker_end_of_list &&
                   next_parseptr->call == base_calls[base_call_cloverleaf]) {
                  next_parseptr->call = base_calls[base_call_clover];
                  localmodsout1 |= DFM1_CONC_FORCE_COLUMNS;
                  localmodsout1 |= DFM1_CONC_DEMAND_COLUMNS;
                  next_parseptr->no_check_call_level = true;
               }
            }

            // If the ends' starting setup is a 2x2, and we did not say
            // "concentric" (indicated by the DFM1_CONC_CONCENTRIC_RULES
            // flag being off), we mark the setup as elongated.  If the
            // call turns out to be a 2-person call, the elongation will be
            // checked against the pairings of people, and an error will be
            // given if it isn't right.  This is what makes "cy-kick"
            // illegal from diamonds, and "ends hinge" illegal from waves.
            // The reason this is turned off when the "concentric" concept
            // is given is so that "concentric hinge" from waves, obnoxious
            // as it may be, will be legal.

            // We also turn it off if this is reverse checkpoint.  In that
            // case, the ends know exactly where they should go.  This is
            // what makes "reverse checkpoint recycle by star thru" work
            // from a DPT setup.

            if (analyzer != schema_in_out_triple &&
                analyzer != schema_sgl_in_out_triple &&
                analyzer != schema_3x3_in_out_triple &&
                analyzer != schema_4x4_in_out_triple &&
                analyzer != schema_in_out_triple_zcom &&
                analyzer != schema_in_out_center_triple_z &&
                analyzer != schema_in_out_triple_squash &&
                analyzer != schema_in_out_triple_dyp_squash &&
                analyzer != schema_sgl_in_out_triple_squash &&
                analyzer != schema_3x3_in_out_triple_squash &&
                analyzer != schema_4x4_in_out_triple_squash &&
                analyzer != schema_in_out_quad &&
                analyzer != schema_in_out_12mquad &&
                analyzer != schema_conc_o &&
                analyzer != schema_rev_checkpoint &&
                analyzer != schema_rev_checkpoint_concept) {
               if ((begin_ptr->kind == s2x2 ||
                    begin_ptr->kind == s2x3 || begin_ptr->kind == s_star ||
                    begin_ptr->kind == s_short6) &&
                   begin_outer_elongation > 0) {      // We demand elongation be 1 or more.
                  begin_ptr->cmd.prior_elongation_bits = begin_outer_elongation;

                  // If "demand lines" or "demand columns" has been given, we suppress
                  // elongation checking.  In that case, the database author knows what
                  // elongation is required and is taking responsibility for it.
                  // This is what makes "scamper" and "divvy up" work.
                  // We also do this if the concept is cross concentric.
                  // In that case the people doing the "ends" call actually did it in
                  // the center (the way they were taught in C2 class) before moving
                  // to the outside.

                  if (((DFM1_CONC_CONCENTRIC_RULES | DFM1_CONC_DEMAND_LINES |
                        DFM1_CONC_DEMAND_COLUMNS) & localmodsout1) ||
                      crossing ||
                      analyzer == schema_checkpoint)
                     begin_ptr->cmd.cmd_misc_flags |= CMD_MISC__NO_CHK_ELONG;

                  // Let later code see that this is under explicit "concentric"; watch for
                  // extremely unesthetic things for ends, like "concentric hinge" from waves;
                  // will raise horrible warning if so.

                  if ((ss->kind == s2x4 || ss->kind == s_qtag) && !crossing && analyzer == schema_concentric &&
                      (localmodsout1 & DFM1_CONC_CONCENTRIC_RULES))
                     begin_ptr->cmd.prior_elongation_bits |= PRIOR_ELONG_CONC_RULES_CHECK_HORRIBLE;
               }
               else if (begin_ptr->kind == s1x2 || begin_ptr->kind == s1x4 || begin_ptr->kind == s1x6) {
                  // Indicate that these people are working around the outside.
                  begin_ptr->cmd.prior_elongation_bits |= PRIOR_ELONG_IS_DISCONNECTED;

                  if ((DFM1_CONC_CONCENTRIC_RULES & localmodsout1) ||
                      crossing ||
                      analyzer == schema_checkpoint)
                     begin_ptr->cmd.cmd_misc_flags |= CMD_MISC__NO_CHK_ELONG;

                  // Do some more stuff.  This makes "fan the gating [recycle]" work.
                  if ((DFM1_SUPPRESS_ELONGATION_WARNINGS & localmodsout1) &&
                      analyzer_result == schema_concentric)
                     begin_ptr->cmd.cmd_misc_flags |= CMD_MISC__NO_CHK_ELONG;
               }
            }
         }
         else {
            configuration::restore_warnings(saved_ctr_warnings);
            if ((begin_ptr->kind == s2x2 || begin_ptr->kind == s_short6) &&
                cmdout &&
                begin_outer_elongation > 0) {
               // Needed by t49\3045.  Something about triple percolate.
               begin_ptr->cmd.prior_elongation_bits = begin_outer_elongation << 8;
            }
         }

         process_number_insertion(modifiers1);
         // Save the numbers, in case we have to figure out whether "cast off N/4" changed shape.
         *option_ptr = current_options;

         if (recompute_id &&
             !(save_cmd_misc2_flags & CMD_MISC2__CTR_END_KMASK) &&
             !(ss->result_flags.misc & RESULTFLAG__REALLY_NO_REEVALUATE))
            update_id_bits(begin_ptr);

         // Inherit certain assumptions to the child setups.  This is EXTREMELY incomplete.

         if (begin_ptr->cmd.cmd_assume.assumption != cr_none)
            inherit_conc_assumptions(ss->kind, begin_ptr->kind,
                                     analyzer, (doing_ends ^ crossing),
                                     &begin_ptr->cmd.cmd_assume);

         // This call to "move" will fill in good stuff (viz. the DFM1_CONCENTRICITY_FLAG_MASK)
         // into begin_ptr->cmd.cmd_misc_flags, which we will use below to do various
         // "force_lines", "demand_columns", etc. things.

         if (doing_ends) {
            // If cross concentric, we are looking for plain "mystic"
            mystictest = crossing ? CMD_MISC2__CENTRAL_MYSTIC :
               (CMD_MISC2__CENTRAL_MYSTIC | CMD_MISC2__INVERT_MYSTIC);

            // Handle "invert snag" for ends.
            if ((save_cmd_misc2_flags & (CMD_MISC2__CENTRAL_SNAG | CMD_MISC2__INVERT_SNAG)) ==
                (CMD_MISC2__CENTRAL_SNAG | CMD_MISC2__INVERT_SNAG)) {
               if (mystictest == CMD_MISC2__CENTRAL_MYSTIC)
                  fail("Can't do \"central/snag/mystic\" with this call.");
               begin_ptr->cmd.cmd_fraction.set_to_firsthalf();
            }
            else if ((save_cmd_misc2_flags & (CMD_MISC2__ANY_SNAG | CMD_MISC2__ANY_WORK_INVERT))
                     == (CMD_MISC2__ANY_SNAG | CMD_MISC2__ANY_WORK_INVERT)) {
               begin_ptr->cmd.cmd_fraction.set_to_firsthalf();
            }

            // This makes it not normalize the setup between parts -- the 4x4 stays around.
            if (analyzer == schema_conc_o)
               begin_ptr->cmd.cmd_misc_flags |= CMD_MISC__EXPLICIT_MATRIX;
         }
         else {
            // If cross concentric, we are looking for "invert mystic".

            mystictest = crossing ?
               (CMD_MISC2__CENTRAL_MYSTIC | CMD_MISC2__INVERT_MYSTIC) :
               CMD_MISC2__CENTRAL_MYSTIC;

            // Handle "snag" for centers.
            if ((save_cmd_misc2_flags & (CMD_MISC2__CENTRAL_SNAG | CMD_MISC2__INVERT_SNAG))
                == CMD_MISC2__CENTRAL_SNAG) {
               if (mystictest == (CMD_MISC2__CENTRAL_MYSTIC | CMD_MISC2__INVERT_MYSTIC))
                  fail("Can't do \"central/snag/mystic\" with this call.");
               begin_ptr->cmd.cmd_fraction.set_to_firsthalf();
            }
            else if ((save_cmd_misc2_flags & (CMD_MISC2__ANY_SNAG | CMD_MISC2__ANY_WORK_INVERT))
                     == CMD_MISC2__ANY_SNAG) {
               begin_ptr->cmd.cmd_fraction.set_to_firsthalf();
            }
         }

         // Handle "invert mystic" for ends, or "mystic" for centers.
         // Also handle the "mystify_split" stuff.

         bool we_are_mirroring =
            (save_cmd_misc2_flags & (CMD_MISC2__CENTRAL_MYSTIC | CMD_MISC2__INVERT_MYSTIC)) ==
            mystictest;

         if (we_are_mirroring) {
            mirror_this(begin_ptr);
            begin_ptr->cmd.cmd_misc_flags ^= CMD_MISC__EXPLICIT_MIRROR;
         }

         if ((save_cmd_misc2_flags &
              (CMD_MISC2__ANY_WORK|CMD_MISC2__ANY_WORK_INVERT)) == ctr_use_flag) {

            if (save_skippable_heritflags != 0) {
               begin_ptr->cmd.cmd_final_flags.set_heritbits(save_skippable_heritflags);
               impose_assumption_and_move(begin_ptr, result_ptr);
            }
            else {
               if (!save_skippable_concept)
                  fail("Internal error in centers/ends work, please report this.");

               if (!begin_ptr->cmd.callspec)
                  fail("No callspec, centers/ends!!!!!!");

               // We are going to alter the list structure while executing
               // the subject call, and then restore same when finished.

               parse_block *z1 = save_skippable_concept;
               while (z1->concept->kind > marker_end_of_list) z1 = z1->next;

               call_with_name *savecall = z1->call;
               call_with_name *savecall_to_print = z1->call_to_print;
               bool savelevelcheck = z1->no_check_call_level;
               parse_block *savebeginparse = begin_ptr->cmd.parseptr;

               z1->call = begin_ptr->cmd.callspec;
               z1->call_to_print = begin_ptr->cmd.callspec;
               z1->no_check_call_level = true;
               begin_ptr->cmd.callspec = (call_with_name *) 0;
               begin_ptr->cmd.parseptr = save_skippable_concept;

               // Preserved across a throw; must be volatile.
               volatile error_flag_type maybe_throw_this = error_flag_none;

               try {
                  impose_assumption_and_move(begin_ptr, result_ptr);
               }
               catch(error_flag_type foo) {
                  // An error occurred.  We need to restore stuff.
                  maybe_throw_this = foo;
               }

               // Restore.

               begin_ptr->cmd.callspec = z1->call;
               begin_ptr->cmd.parseptr = savebeginparse;
               z1->call = savecall;
               z1->call_to_print = savecall_to_print;
               z1->no_check_call_level = savelevelcheck;

               if (maybe_throw_this != error_flag_none)
                  throw maybe_throw_this;
            }
         }
         else if (doing_ends &&
                  analyzer_result == schema_concentric &&
                  begin_ptr->kind == s4x4 &&
                  (little_endian_live_mask(begin_ptr) & 0x9999) == 0) {
            // Make two 2x4's.
            setup setup1 = *begin_ptr;
            setup setup2 = *begin_ptr;
            setup1.kind = s2x2;
            setup2.kind = s2x2;
            setup1.rotation = 0;
            setup1.eighth_rotation = 0;
            setup2.rotation = 0;
            setup2.eighth_rotation = 0;
            setup1.clear_people();
            setup2.clear_people();

            static const int8_t sss1[4] = {10, 1, 2, 9};
            static const int8_t sss2[4] = {13, 14, 5, 6};
            gather(&setup1, begin_ptr, sss1, 3, 0);
            gather(&setup2, begin_ptr, sss2, 3, 0);
            setup1.cmd.prior_elongation_bits = 1;
            setup2.cmd.prior_elongation_bits = 2;

            if (localmodsout1 & DFM1_CONC_DEMAND_LINES) {
               if ((or_all_people(&setup1) & 1) ||
                   (or_all_people(&setup2) & 010))
                  fail("Outsides must be as if in lines at start of this call.");
            }
            else if (localmodsout1 & DFM1_CONC_DEMAND_COLUMNS) {
               if ((or_all_people(&setup1) & 010) ||
                   (or_all_people(&setup2) & 1))
                  fail("Outsides must be as if in columns at start of this call.");
            }

            setup the_results[2];
            impose_assumption_and_move(&setup1, &the_results[0]);
            impose_assumption_and_move(&setup2, &the_results[1]);

            if (the_results[0].kind != s2x2 || the_results[1].kind != s2x2)
               fail("Can't do this call.");

            *result_ptr = the_results[1];
            result_ptr->result_flags = get_multiple_parallel_resultflags(the_results, 2);

            result_ptr->kind = s4x4;
            result_ptr->rotation = 0;
            result_ptr->eighth_rotation = 0;
            result_ptr->clear_people();

            for (int k=0; k<4; k++) {
               const int8_t *place0;
               const int8_t *place1;

               if (localmodsout1 & DFM1_CONC_FORCE_SPOTS) {
                  place0 = sss1;
                  place1 = sss2;
               }
               else if (localmodsout1 & DFM1_CONC_FORCE_OTHERWAY) {
                  place0 = sss2;
                  place1 = sss1;
               }
               else if (localmodsout1 & DFM1_CONC_FORCE_LINES) {
                  place0 = (the_results[0].people[k].id1 & 1) ? sss2 : sss1;
                  place1 = (the_results[1].people[k].id1 & 1) ? sss2 : sss1;
               }
               else if (localmodsout1 & DFM1_CONC_FORCE_COLUMNS) {
                  place0 = (the_results[0].people[k].id1 & 1) ? sss1 : sss2;
                  place1 = (the_results[1].people[k].id1 & 1) ? sss1 : sss2;
               }
               else
                  fail("Can't do this call.");

               install_person(result_ptr, place0[k], &the_results[0], k);
               install_person(result_ptr, place1[k], &the_results[1], k);
            }
         }
         else {
            if (doing_ends || suppress_overcasts)
               begin_ptr->clear_all_overcasts();

            if (specialoffsetmapcode < ~1U) {
               divided_setup_move(begin_ptr, specialoffsetmapcode,
                                  phantest_only_one, true, result_ptr);
            }
            else {
               if (doing_ends && center_arity == 1 && !crossing &&
                   (analyzer == schema_concentric || analyzer == schema_single_concentric)) {
                  call_with_name *trythiscall = begin_ptr->cmd.callspec;

                  if (!trythiscall)
                     trythiscall = begin_ptr->cmd.parseptr->call;

                  if (trythiscall &&
                      trythiscall->the_defn.schema == schema_matrix &&
                      !(trythiscall->the_defn.stuff.matrix.matrix_flags &
                        (MTX_FIND_TRADERS | MTX_FIND_SQUEEZERS | MTX_FIND_SPREADERS))) {
                     if (begin_ptr->kind == s1x2) {
                        static const expand::thing fix_1x2 = {{0, 2}, s1x2, s1x4, 0, 0, 0};
                        expand::expand_setup(fix_1x2, begin_ptr);
                        ends_are_in_situ = true;
                        begin_ptr->cmd.cmd_misc_flags &= ~CMD_MISC__DISTORTED;
                     }
                     else if (begin_ptr->kind == s2x2 && (begin_outer_elongation & 3) == 1) {
                        expand::expand_setup(s_2x2_2x4_ends, begin_ptr);
                        ends_are_in_situ = true;
                        begin_ptr->cmd.cmd_misc_flags &= ~CMD_MISC__DISTORTED;
                     }
                     else if (begin_ptr->kind == s2x2 && (begin_outer_elongation & 3) == 2) {
                        expand::expand_setup(s_2x2_2x4_endsb, begin_ptr);
                        ends_are_in_situ = true;
                        begin_ptr->cmd.cmd_misc_flags &= ~CMD_MISC__DISTORTED;
                     }
                  }
               }

               impose_assumption_and_move(begin_ptr, result_ptr);
            }

            if (doing_ends || suppress_overcasts)
               result_ptr->clear_all_overcasts();
         }

         if (modifiers1 & DFM1_SUPPRESS_ROLL)
            result_ptr->suppress_all_rolls(false);

         uint32_t z_compress_direction =
            (result_ptr->result_flags.misc & RESULTFLAG__DID_Z_COMPRESSMASK) /
            RESULTFLAG__DID_Z_COMPRESSBIT;

         if (z_compress_direction) {
            if (demand_no_z_stuff && !(begin_ptr->cmd.cmd_misc3_flags & CMD_MISC3__SAID_Z))
               fail("Can't do this call from a \"Z\".");

            if (result_ptr->kind == s2x2) {
               const expand::thing *fixp;
               result_ptr->rotation -= z_compress_direction99rot;
               canonicalize_rotation(result_ptr);

               if (z_compress_direction99skew == 066)
                  fixp = &fix_cw;
               else if (z_compress_direction99skew == 033)
                  fixp = &fix_ccw;
               else
                  fail("Internal error: Can't figure out how to unwind Z.");

               expand::expand_setup(*fixp, result_ptr);
               result_ptr->rotation += z_compress_direction99rot;
            }
         }

         if (analyzer == schema_in_out_triple_zcom || analyzer == schema_in_out_center_triple_z) {
            analyzer = schema_in_out_triple;
         }
         else if (analyzer == schema_in_out_triple_squash && !doing_ends &&
                  (result_ptr->result_flags.misc & RESULTFLAG__EXPAND_TO_2X3) &&
                  begin_ptr->kind == s1x4 && begin_ptr->rotation == 0 &&
                  result_ptr->kind == s2x3 && result_ptr->rotation == 1) {
            // This handles things like "quick [rip the line]" or
            // "quick [step and slide]", getting rid of paradoxical phantoms.
            static const expand::thing fix_2x3_23 = {{0, 1, 4, 5}, s2x2, s2x3, 0};
            static const expand::thing fix_2x3_05 = {{1, 2, 3, 4}, s2x2, s2x3, 0};

            if (!(result_ptr->people[2].id1 | result_ptr->people[3].id1)) {
               expand::compress_setup(fix_2x3_23, result_ptr);
            }
            else if (!(result_ptr->people[0].id1 | result_ptr->people[5].id1)) {
               expand::compress_setup(fix_2x3_05, result_ptr);
            }

            canonicalize_rotation(result_ptr);
         }

         remove_mxn_spreading(result_ptr);
         remove_tgl_distortion(result_ptr);

         if (we_are_mirroring)
            mirror_this(result_ptr);

         current_options = save_state;

         if (doing_ends)
            saved_end_warnings.setmultiple(configuration::save_warnings());
         else
            saved_ctr_warnings.setmultiple(configuration::save_warnings());
      }
      else {
         begin_ptr->cmd.callspec = (call_with_name *) 0;
         *result_ptr = *begin_ptr;
         clear_result_flags(result_ptr);

         if (doing_ends) {
            if (begin_outer_elongation <= 0 || begin_outer_elongation > 2) {
               // Outer people have unknown elongation and aren't moving.  Not good.
            }
            else {
               result_ptr->result_flags.misc = begin_outer_elongation;
            }

            // Make sure these people go to the same spots.
            localmodsout1 |= DFM1_CONC_FORCE_SPOTS;
         }

         // Strip out the roll bits -- people who didn't move can't roll.  Unless this is roll-transparent.
         result_ptr->suppress_all_rolls((begin_ptr->cmd.cmd_misc3_flags & CMD_MISC3__ROLL_TRANSP) != 0);
      }

      if (analyzer == schema_concentric_to_outer_diamond &&
          doing_ends &&
          result_ptr->kind != sdmd) {
         if (result_ptr->kind == s1x4 &&
             !(result_ptr->people[1].id1 | result_ptr->people[3].id1)) {
         }
         else if (result_ptr->kind == nothing) {
            result_ptr->clear_people();
         }
         else
            fail("Can't make a diamond out of this.");

         result_ptr->kind = sdmd;
      }
   }

   // Put the warnings back.  Watch for some people getting "rear back" and others not; change to "some rear back".

   configuration::restore_warnings(saved_all_warnings);

   // But don't do that if only one group was told to do anything.
   if (cmdout != 0 && cmdin != 0) {
      if (saved_ctr_warnings.testbit(warn__rear_back) && !saved_end_warnings.testbit(warn__rear_back)) {
         saved_ctr_warnings.clearbit(warn__rear_back);
         saved_ctr_warnings.setbit(warn__some_rear_back);
      }

      if (saved_end_warnings.testbit(warn__rear_back) && !saved_ctr_warnings.testbit(warn__rear_back)) {
         saved_end_warnings.clearbit(warn__rear_back);
         saved_end_warnings.setbit(warn__some_rear_back);
      }
   }

   configuration::set_multiple_warnings(saved_ctr_warnings);
   configuration::set_multiple_warnings(saved_end_warnings);
   configuration::clear_one_warning(warn_suspect_destroyline);

   // Now, if some command (centers or ends) didn't exist, we pick up the needed result flags
   // from the other part.
   // Grab the "did_last_part" flags from the call that was actually done.

   if (inverting) {
      if (!cmdin)
         outer_inners[0].result_flags.misc |= outer_inners[1].result_flags.misc &
            (RESULTFLAG__DID_LAST_PART | RESULTFLAG__SECONDARY_DONE | RESULTFLAG__PARTS_ARE_KNOWN);

      if (!cmdout) {
         for (k=0; k<center_arity; k++)
            outer_inners[k+1].result_flags.misc |= outer_inners[0].result_flags.misc &
               (RESULTFLAG__DID_LAST_PART | RESULTFLAG__SECONDARY_DONE | RESULTFLAG__PARTS_ARE_KNOWN);
      }
   }
   else {
      if (!cmdout)
         outer_inners[0].result_flags.misc |= outer_inners[1].result_flags.misc &
            (RESULTFLAG__DID_LAST_PART | RESULTFLAG__SECONDARY_DONE | RESULTFLAG__PARTS_ARE_KNOWN);

      if (!cmdin) {
         for (k=0; k<center_arity; k++)
            outer_inners[k+1].result_flags.misc |= outer_inners[0].result_flags.misc &
               (RESULTFLAG__DID_LAST_PART | RESULTFLAG__SECONDARY_DONE | RESULTFLAG__PARTS_ARE_KNOWN);
      }
   }

   int final_elongation = crossing ? begin_xconc_elongation : begin_outer_elongation;

   // Note: final_elongation might be -1 now, meaning that the people on the outside
   // cannot determine their elongation from the original setup.  Unless their
   // final setup is one that does not require knowing the value of final_elongation,
   // it is an error.
   // It might also have the "CONTROVERSIAL_CONC_ELONG" bit set, meaning that we should
   // raise a warning if we use it.

   // At this point, "final_elongation" actually has the INITIAL elongation of the
   // people who finished on the outside.  That is, if they went from a wave or diamond
   // to a 2x2, it has the elongation of their initial wave or diamond points.

   // Exception: if the schema was conc_6_2 or conc_6_2_tri, and the centers are in a bone6,
   // "final_elongation" has the elongation of that bone6.

   // The elongation bits in their setup tells how they "naturally" wanted to end,
   // based on the call they did, how it got divided up, whether it had the "parallel_conc_end"
   // flag on, etc.

   if (analyzer == schema_in_out_triple_zcom) analyzer = schema_concentric_zs;
   else if (analyzer == schema_in_out_triple && imposing_z) analyzer = schema_concentric;
   else if (analyzer == schema_inner_2x4 || analyzer == schema_inner_2x6) analyzer = schema_in_out_triple;
   else if (analyzer == schema_concentric_ctrbox) analyzer = schema_concentric;

   if (analyzer == schema_concentric ||
       analyzer == schema_concentric_6p_or_normal ||
       analyzer == schema_in_out_triple ||
       analyzer == schema_concentric_zs ||
       analyzer == schema_sgl_in_out_triple ||
       analyzer == schema_3x3_in_out_triple ||
       analyzer == schema_4x4_in_out_triple ||
       analyzer == schema_in_out_triple_squash ||
       analyzer == schema_in_out_triple_dyp_squash ||
       analyzer == schema_sgl_in_out_triple_squash ||
       analyzer == schema_3x3_in_out_triple_squash ||
       analyzer == schema_4x4_in_out_triple_squash ||
       analyzer == schema_in_out_quad ||
       analyzer == schema_in_out_12mquad ||
       analyzer == schema_concentric_others) {
      fix_n_results(center_arity, -1, &outer_inners[1], rotstate, pointclip, 0);
      if (outer_inners[1].kind != nothing && !(rotstate & 0xF03)) fail("Sorry, can't do this orientation changer.");

      // Try to turn inhomogeneous diamond/wave combinations into all diamonds,
      // if the waves are missing their centers or ends.  If the resulting diamonds
      // are nonisotropic (elongated differently), that's OK.

      if (analyzer == schema_in_out_triple && ((final_elongation+1) & 2) != 0) {
         if (outer_inners[0].kind == sdmd && outer_inners[1].kind == s1x4) {
            if (!(outer_inners[1].people[0].id1 | outer_inners[1].people[2].id1 |
                  outer_inners[2].people[0].id1 | outer_inners[2].people[2].id1)) {
               expand::compress_setup(s_1x4_dmd, &outer_inners[1]);
               expand::compress_setup(s_1x4_dmd, &outer_inners[2]);
            }
            else if (!(outer_inners[1].people[1].id1 | outer_inners[1].people[3].id1 |
                       outer_inners[2].people[1].id1 | outer_inners[2].people[3].id1)) {
               outer_inners[1].kind = sdmd;
               outer_inners[2].kind = sdmd;
            }
         }
         else if (outer_inners[0].kind == s1x4 && outer_inners[1].kind == sdmd) {
            if (!(outer_inners[0].people[1].id1 | outer_inners[0].people[3].id1)) {
               outer_inners[0].kind = sdmd;
            }
            else if (!(outer_inners[0].people[0].id1 | outer_inners[0].people[2].id1) &&
                     ((outer_inners[0].rotation ^ final_elongation) & 1)) {
               // Occurs in t00t@1462.
               // The case we avoid with the rotation check above is bi02t@1045.
               expand::compress_setup(s_1x4_dmd, &outer_inners[0]);
            }
            else if (!(outer_inners[1].people[0].id1 | outer_inners[1].people[2].id1 |
                       outer_inners[2].people[0].id1 | outer_inners[2].people[2].id1) &&
                     ((outer_inners[1].rotation ^ final_elongation) & 1)) {
               expand::expand_setup(s_1x4_dmd, &outer_inners[1]);
               expand::expand_setup(s_1x4_dmd, &outer_inners[2]);
            }
         }
      }
   }
   else if (analyzer == schema_4x4k_concentric ||
            analyzer == schema_3x3k_concentric) {
      // Put all 3, or all 4, results into the same formation.
      fix_n_results(center_arity+1, -1, outer_inners, rotstate, pointclip, 0);
      if (outer_inners[0].kind == nothing)
         fail("This is an inconsistent shape or orientation changer.");
   }

   // If the call was something like "ends detour", the concentricity info was left in the
   // cmd_misc_flags during the execution of the call, so we have to pick it up to make sure
   // that the necessary "demand" and "force" bits are honored.
   localmodsout1 |= (begin_outer.cmd.cmd_misc_flags & DFM1_CONCENTRICITY_FLAG_MASK);

   // If doing half of an acey deucey, don't force spots--it messes up short6 orientation.
   if (analyzer == schema_concentric_6p_or_normal_or_2x6_2x3 &&
       ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_HALF))
      localmodsout1 &= ~DFM1_CONC_FORCE_SPOTS;

   // If the outsides did "emulate", they stay on the same spots no matter what
   // anyone says.
   if (outer_inners[0].result_flags.misc & RESULTFLAG__FORCE_SPOTS_ALWAYS) {
      localmodsout1 &= ~DFM1_CONCENTRICITY_FLAG_MASK;
      localmodsout1 |= DFM1_CONC_FORCE_SPOTS;
   }

   // Check whether the necessary "demand" conditions are met.  First, set "localmods1"
   // to the demand info for the call that the original ends did.  Where this comes from
   // depends on whether the schema is cross concentric.

   uint32_t localmods1 = crossing ? localmodsin1 : localmodsout1;

   // Will only be used if begin_outer_elongation = 1 or 2.
   uint32_t elongshift = 3*(begin_outer_elongation - 1);    // So this will be 0 or 3.
   uint32_t linesfailbit = 1 << elongshift;                 // And these will be 1 or 8.
   uint32_t columnsfailbit = 8 >> elongshift;

   setup_command *outercmd = (inverting) ? cmdin : cmdout;

   if (outercmd &&
       (outercmd->cmd_final_flags.bool_test_heritbits(INHERITFLAG_TWISTED) != 0 ||
       outercmd->cmd_final_flags.test_heritbits(INHERITFLAG_REVERTMASK) == INHERITFLAGRVRTK_REFLECT)) {
      columnsfailbit = 1 << elongshift;
      linesfailbit = 8 >> elongshift;
   }

   if (orig_outers_start_kind == s2x2) {
      fraction_command::includes_first_part_enum foob = ss->cmd.cmd_fraction.fraction_command::includes_first_part();

      switch (localmods1 & (DFM1_CONC_DEMAND_LINES | DFM1_CONC_DEMAND_COLUMNS)) {
      case DFM1_CONC_DEMAND_LINES:
         // We make use of the fact that the setup, being a 2x2, is canonicalized.
         if (foob == fraction_command::yes) {
            if (begin_outer_elongation <= 0 || begin_outer_elongation > 2 || (orig_outers_start_dirs & linesfailbit) != 0)
               fail("Outsides must be as if in lines at start of this call.");
         }
         break;
      case DFM1_CONC_DEMAND_COLUMNS:
         if (foob == fraction_command::yes) {
            if (begin_outer_elongation <= 0 || begin_outer_elongation > 2 || (orig_outers_start_dirs & columnsfailbit) != 0)
               fail("Outsides must be as if in columns at start of this call.");
         }
         break;
      case DFM1_CONC_DEMAND_LINES | DFM1_CONC_DEMAND_COLUMNS:
         if (begin_outer_elongation > 2)
            fail("Can't do this.");
         break;
      }
   }

   // Now check whether there are any demands on the original centers.  The interpretation
   // of "lines" and "columns" is slightly different in this case.  We apply the test only if
   // the centers are in a 2x2, but we don't care about the outsides' setup, as long as it
   // has a measurable elongation.  If the outsides are also in a 2x2, so that the whole setup
   // is a 2x4, these tests will do just what they say -- they will check whether the centers
   // believe they are in lines or columns.  However, if the outsides are in a 1x4, so the
   // overall setup is a "rigger", we simply test the outsides' elongation.  In such a case
   // "demand lines" means "demand outsides lateral to me".

   // But we don't do this if we are inverting the centers and ends.

   if (!inverting) {
      uint32_t mymods = crossing ? localmodsout1 : localmodsin1;

      if (orig_inners_start_kind == s2x2) {
         switch (mymods & (DFM1_CONC_DEMAND_LINES | DFM1_CONC_DEMAND_COLUMNS)) {
         case DFM1_CONC_DEMAND_LINES:
            if (begin_outer_elongation <= 0 || begin_outer_elongation > 2 ||
                (orig_inners_start_dirs & (1 << elongshift)))
               fail("Centers must be as if in lines at start of this call.");
            break;
         case DFM1_CONC_DEMAND_COLUMNS:
            if (begin_outer_elongation <= 0 || begin_outer_elongation > 2 ||
                (orig_inners_start_dirs & (8 >> elongshift)))
               fail("Centers must be as if in columns at start of this call.");
            break;
         case DFM1_CONC_DEMAND_LINES | DFM1_CONC_DEMAND_COLUMNS:
            if (begin_outer_elongation > 2)
               fail("Can't do this.");
            break;
         }
      }
   }

   localmods1 = localmodsout1;

   final_outers_finish_dirs = 0;
   for (i=0; i<=attr::slimit(&outer_inners[0]); i++) {
      int q = outer_inners[0].people[i].id1;
      final_outers_finish_dirs |= q;
      if (q) final_outers_finish_directions[(q >> 6) & 037] = q;
   }

   // Now final_outers_finish_dirs tells whether outer peoples' orientations
   // changed.  This is only meaningful if outer setup is 2x2.  Note that, if
   // the setups are 2x2's, canonicalization sets their rotation to zero, so the
   // tbonetest quantities refer to absolute orientation.

   // Deal with empty setups.

   if (outer_inners[0].kind == nothing) {
      if (outer_inners[1].kind == nothing) {
         result->kind = nothing;    // If everyone is a phantom, it's simple.
         clear_result_flags(result);
         return;
      }

      // No action if schema_first/second_only.
      if (analyzer_result != schema_first_only && analyzer_result != schema_second_only) {
         if (fix_empty_outers(ss->kind, final_outers_start_kind, localmods1,
                              crossing, begin_outer_elongation, center_arity,
                              analyzer, cmdin, cmdout, &begin_outer, &outer_inners[0],
                              &outer_inners[1], result)) {
            if (crossing &&
                begin_outer.cmd.callspec == base_calls[base_call_plan_ctrtoend] &&
                final_outers_finish_dirs == 0 &&
                ss->cmd.cmd_assume.assumption == cr_li_lo &&
                ss->cmd.cmd_assume.assump_col == 0) {

               // This is "plan ahead" with an "assume facing lines".
               // If no live people in the center, we infer their direction
               // from the overall assumption, and set the final direction
               // to what would have resulted.  We have only the assumption
               // to tell us what to do.
               //
               // Q: wouldn't a smarter "inherit_conc_assumptions" have taken
               //    care of this?
               // A: No.  It correctly inherited the "facing lines" to "facing couples"
               //    in each 2x2 before doing the call, but just knowing that the center
               //    2x2 was in facing couples doesn't tell us what we really need to know --
               //    that those couples were parallel to the overall 2x4.

               localmods1 = localmodsout1;   // Set it back to DFM1_CONC_FORCE_COLUMNS.
               final_outers_finish_dirs = (~ss->cmd.cmd_assume.assump_col) & 1;
            }
            else
               goto getout;
         }
      }
   }
   else if (outer_inners[1].kind == nothing) {
      // No action if schema_first/second_only.
      if (analyzer_result != schema_first_only && analyzer_result != schema_second_only) {
         if (fix_empty_inners(orig_inners_start_kind, center_arity, begin_outer_elongation,
                              analyzer, analyzer_result, &begin_inner[0],
                              &outer_inners[0], &outer_inners[1],
                              outer_inner_options[1],
                              result))
            goto getout;
      }
   }

   // The time has come to compute the elongation of the outsides in the final setup.
   // This gets complicated if the outsides' final setup is a 2x2.  Among the
   // procedures we could use are:
   //    (1) if the call is "checkpoint", go to spots with opposite elongation
   //    // from the original outsides' elongation.  This is the "Hodson checkpoint
   //    // rule", named after the caller who first used a consistent, methodical,
   //    // and universal rule for the checkpoint concept.
   //    (2) if the call is "concentric", use the Hodson rule if the original setup
   //    // was a 1x4 or diamond, or the "lines-to-lines, columns-to-columns" rule
   //    // if the original setup was a 2x2.
   //    (3) if we have various definition flags, such as "force_lines" or
   //    // "force_otherway", obey them.
   // We will use information from several sources in carrying out these rules.
   // The concentric concept will signify itself by turning on the "lines_lines"
   // flag.  The checkpoint concept will signify itself by turning on the
   // "force_otherway" flag.  The "parallel_conc_end" flag in the outsides' setup
   // indicates that, if "concentric" or "checkpoint" are NOT being used, the call
   // wants the outsides to maintain the same elongation as they had at the beginning.
   // This is what makes "ends hinge" and "ends recycle" do their respective
   // right things when called from a grand wave.

   // Default: the ends just keep their original elongation.  This will often
   // mean that they stay on their spots.

   // We will use both pieces of information to figure out how to elongate the outsides at
   // the conclusion of the call.  For example, if the word "concentric" was not spoken,
   // we will just use their "natural" elongation from the setup.  This is what makes
   // "ends hinge" work from a grand wave.  If the word "concentric" was spoken, their
   // natural elongation is discarded, and we will set them perpendicular to their
   // original 1x4 or diamond, using the value in "final_elongation"  If invocation
   // flags like "force lines" or "force columns" are present, we will use those.

   // When we are done, our final judgement will be put back into the variable
   // "final_elongation".

   if (analyzer != schema_in_out_triple &&
       analyzer != schema_in_out_triple_dyp_squash &&
       analyzer != schema_in_out_triple_squash &&
       analyzer != schema_sgl_in_out_triple_squash &&
       analyzer != schema_3x3_in_out_triple_squash &&
       analyzer != schema_4x4_in_out_triple_squash &&
       analyzer != schema_sgl_in_out_triple &&
       analyzer != schema_3x3_in_out_triple &&
       analyzer != schema_4x4_in_out_triple &&
       analyzer != schema_in_out_quad &&
       analyzer != schema_in_out_12mquad &&
       analyzer != schema_concentric_zs &&
       analyzer != schema_intermediate_diamond &&
       analyzer != schema_outside_diamond &&
       analyzer != schema_first_only &&
       analyzer != schema_second_only &&
       analyzer != schema_conc_o &&
       analyzer != schema_conc_bar12 &&
       analyzer != schema_conc_bar16) {
      if (outer_inners[0].kind == s2x2 || outer_inners[0].kind == s2x3 || outer_inners[0].kind == sdeep2x1dmd ||
          outer_inners[0].kind == s2x4 || outer_inners[0].kind == s_short6) {
         warning_index *concwarntable;

         if (final_outers_start_kind == s1x4) {
            // Watch for special case of cross concentric that some people may not agree with.
            if (orig_outers_start_kind == s1x4 &&
                ((begin_inner[0].rotation ^ begin_outer.rotation) & 1))
               concwarntable = concwarneeetable;
            else
               concwarntable = concwarn1x4table;
         }
         else
            concwarntable = concwarndmdtable;

         switch (final_outers_start_kind) {
         case s1x4: case sdmd: case s_star:

            // Outers' call has gone from a 1x4 or diamond to a 2x2.  The rules are:
            // (1) The "force_columns" or "force_lines" flag in the invocation takes
            //    precedence over anything else.
            // (2) If the "concentric rules" flag is on (that flag is a euphemism for "the
            //    concentric or checkpoint concept is explicitly in use here"), we set the
            //    elongation perpendicular to the original 1x4 or diamond.
            // (3) If the "force_otherway" invocation flag is on, meaning the database
            //    really wants us to, we set the elongation perpendicular to the original
            //    1x4 or diamond.
            // (4) If the "force_spots" invocation flag is on, meaning the database
            //    really wants us to, we set the elongation parallel to the original
            //    1x4 or diamond.
            // (5) Otherwise, we set the elongation to the natural elongation that the
            //    people went to.  This uses the result of the "par_conc_end" flag for
            //    1x4/dmd -> 2x2 calls, or the manner in which the setup was divided
            //    for calls that were put together from 2-person calls, or whatever.
            //    (For 1x4->2x2 calls, the "par_conc_end" flag means the call prefers
            //    the SAME elongation in the resulting 2x2.)  The default, absent
            //    this flag, is to change the elongation.  In any case, the result
            //    of all that has been encoded into the elongation of the 2x2 setup
            //    that the people went to; we just have to obey.

            if ((DFM1_CONC_FORCE_LINES & localmods1) && final_outers_finish_dirs) {
               if ((final_outers_finish_dirs & 011) == 011)
                  fail("Can't force ends to be as in lines - they are T-boned.");
               final_elongation = (final_outers_finish_dirs & 1) + 1;
            }
            else if ((DFM1_CONC_FORCE_COLUMNS & localmods1) && final_outers_finish_dirs) {
               if ((final_outers_finish_dirs & 011) == 011)
                  fail("Can't force ends to be as in columns - they are T-boned.");
               final_elongation = ((~final_outers_finish_dirs) & 1) + 1;
            }
            else if (DFM1_CONC_CONCENTRIC_RULES & localmods1) {
               if (outer_inners[0].kind != s2x3)
                  warn(concwarntable[crossing]);  // Don't give warning that would obviously break Solomon.
               final_elongation ^= 3;
            }
            else if (DFM1_CONC_FORCE_OTHERWAY & localmods1) {
               // But we don't obey this flag unless we did the whole call.  (Or it's a checkpoint.)
               if (analyzer == schema_checkpoint || begin_outer.cmd.cmd_fraction.is_null())
                  final_elongation ^= 3;
            }
            else if (DFM1_CONC_FORCE_SPOTS & localmods1) {
               // It's OK the way it is.
            }
            else {
               // Get the elongation from the result setup, if possible.

               int newelong = outer_inners[0].result_flags.misc & 3;

               if (newelong != 0 && final_outers_start_kind != s_star &&
                   !(DFM1_SUPPRESS_ELONGATION_WARNINGS & localmods1)) {
                  if ((final_elongation & (~CONTROVERSIAL_CONC_ELONG)) == newelong)
                     warn(concwarntable[2]);
                  else
                     warn(concwarntable[crossing]);
               }

               if (newelong != 0)
                  final_elongation = newelong;
            }

            break;

         case s_short6: case s_bone6: case s_spindle: case s1x6:
         case s2x4: case s2x3: case sdeep2x1dmd:
         case s_ntrgl6cw: case s_ntrgl6ccw: case s_ntrglcw: case s_ntrglccw:
         case s_nptrglcw: case s_nptrglccw: case s_nxtrglcw: case s_nxtrglccw:

            // In these cases we honor the "concentric rules" and "force_????" flags.

            // Otherwise, if the call did not split the setup, we take the elongation info
            // that it generates -- this is what makes "circulate" and "counter rotate"
            // behave differently, as they must.  But if the call split the setup into
            // parts, its own elongation info can't be meaningful, so we use our own
            // notion of how to elongate.  We also use our notion if the call didn't
            // provide one.

            if ((DFM1_CONC_CONCENTRIC_RULES | DFM1_CONC_FORCE_OTHERWAY) & localmods1) {
               if (((final_elongation+1) & 2) != 0)
                  final_elongation ^= 3;
            }
            else if (DFM1_CONC_FORCE_SPOTS & localmods1) {
               // It's OK the way it is.
            }
            else if ((outer_inners[0].result_flags.misc & 3) != 0)
               final_elongation = (outer_inners[0].result_flags.misc & 3);

            break;

         case s2x2:

            // If call went from 2x2 to 2x2, the rules are:
            //
            // First, check for "force_columns" or "force_lines" in the invocation.
            // This is not a property of the call that we did, but of the way its parent
            // (or the concept) invoked it.
            //
            // Second, check for "force_spots" or "force_otherway" in the invocation.
            // This is not a property of the call that we did, but of the way its parent
            // (or the concept) invoked it.
            //
            // Third, check for "lines_lines" in the invocation.  This is not a property
            // of the call that we did, but of the way its parent (or the concept) invoked it.
            // If the concept was "concentric", it will be on, of course.
            //
            // Finally, check the elongation bits in the result flags left over
            // from the call.  These tell whether to work to spots, or antispots,
            // or whatever, based what the call was, and whether it, or various
            // sequential parts of it, had the "parallel_conc_end" flag on.
            // If there are no elongation bits, we simply don't know what to do.
            //
            // Note that the "ends do thus-and-so" concept does NOT set the
            // lines_lines flag in the invocation, so we work to spots unless
            // the call says "parallel_conc_end".  Counter-rotate, for example,
            // says "parallel_conc_end", so it works to antispots.

            if ((DFM1_CONC_FORCE_LINES & localmods1) && final_outers_finish_dirs) {
               if ((final_outers_finish_dirs & 011) == 011)
                  fail("Can't force ends to be as in lines - they are T-boned.");
               final_elongation = (final_outers_finish_dirs & 1) + 1;
            }
            else if ((DFM1_CONC_FORCE_COLUMNS & localmods1) && final_outers_finish_dirs) {
               if ((final_outers_finish_dirs & 011) == 011)
                  fail("Can't force ends to be as in columns - they are T-boned.");
               final_elongation = ((~final_outers_finish_dirs) & 1) + 1;
            }
            else if (DFM1_CONC_FORCE_OTHERWAY & localmods1) {
               if (((final_elongation-1) & (~1)) == 0) {

                  if (DFM1_ONLY_FORCE_ELONG_IF_EMPTY & localmods1) {
                     // If the setup is nonempty, and the setup seems to know
                     // what elongation it should have, pre-undo it.
                     if (final_outers_finish_dirs != 0 &&
                         outer_inners[0].kind == s2x2 &&
                         (outer_inners[0].result_flags.misc & 3) != 0 &&
                         ((final_elongation ^ outer_inners[0].result_flags.misc) & 3) == 0) {
                        final_elongation ^= 3;
                     }
                  }
                  final_elongation ^= 3;
               }

            }
            else if (DFM1_CONC_FORCE_SPOTS & localmods1) {
               // It's OK the way it is.
            }
            else if (DFM1_CONC_CONCENTRIC_RULES & localmods1) {
               // Do "lines-to-lines / columns-to-columns".
               int new_elongation = -1;

               if (final_elongation < 0)
                  fail("People who finish on the outside can't tell whether they started in line-like or column-like orientation.");

               // If they are butterfly points, leave them there.
               if ((final_elongation & ~CONTROVERSIAL_CONC_ELONG) <= 2) {
                  // Otherwise, search among all possible people,
                  // including virtuals and phantoms.
                  for (i=0; i<32; i++) {
                     if (final_outers_finish_directions[i]) {
                        int t = ((final_outers_start_directions[i] ^
                                  final_outers_finish_directions[i] ^
                                  (final_elongation-1)) & 1) + 1;
                        if (t != new_elongation) {
                           if (new_elongation >= 0)
                              fail("Sorry, outsides would have to go to a 'pinwheel', can't handle that.");
                           new_elongation = t;
                        }
                     }
                  }

                  // Preserve the "controversial" bit.
                  final_elongation &= CONTROVERSIAL_CONC_ELONG;
                  final_elongation |= new_elongation;
               }
            }
            else
               final_elongation = (outer_inners[0].result_flags.misc & 3);

            break;
         case s1x2:
            break;   // Nothing to deal with.
         default:
            if (analyzer != schema_concentric_diamond_line &&
                analyzer != schema_conc_star &&
                analyzer != schema_ckpt_star &&
                analyzer != schema_conc_star12 &&
                analyzer != schema_conc_star16 &&
                analyzer != schema_concentric_others)
               fail("Don't recognize starting setup.");
         }
      }
   }

   // In general, if we sandwich a 1x4 between two parallel 1x4's,
   // with all 3 in parallel, we want a 3x4.  It may happen that
   // the outer setups are actually occupied only in the corners
   // ("butterfly spots"), and the center 1x4 is perpendicular to
   // the outer ones.  In that case, we still want a 3x4, and we
   // have to reformulate the outer 1x4's to go the other way
   // in order to get this result.  Doing this is necessary to get
   // consistent behavior if, in a butterfly, we say "center triple
   // box hinge" vs. "center triple box peel and trail".  (Note that
   // saying "center triple box" requested more precise positioning
   // than if we just said "centers" or "center 4".)  Cf. test t48t.

   if (analyzer == schema_in_out_triple &&
       center_arity == 2 &&
       cmdout &&
       outer_inners[1].kind == s1x4 &&
       outer_inners[0].kind == s1x4 &&
       begin_outer.kind == s2x2 &&
       (outer_inners[1].rotation ^ outer_inners[0].rotation) == 1 &&
       final_elongation == (1 << (outer_inners[0].rotation & 1)) &&
       !(outer_inners[1].people[1].id1 | outer_inners[1].people[3].id1 |
         outer_inners[2].people[1].id1 | outer_inners[2].people[3].id1)) {
      outer_inners[2].swap_people(0, 1);
      copy_rot(&outer_inners[2], 0, &outer_inners[2], 2, 033);
      copy_rot(&outer_inners[2], 2, &outer_inners[1], 2, 033);
      copy_rot(&outer_inners[1], 2, &outer_inners[1], 0, 033);
      copy_rot(&outer_inners[1], 0, &outer_inners[2], 1, 033);
      outer_inners[2].clear_person(1);
      outer_inners[0].rotation--;
      rotate_back++;
   }

   // Now lossage in "final_elongation" may have been repaired.  If it is still
   // negative, there may be trouble ahead.

   if (analyzer == schema_concentric_zs)
      analyzer = schema_in_out_triple_zcom;


   if (ends_are_in_situ) {
      *result = outer_inners[1];
      merge_table::merge_setups(&outer_inners[0], merge_strict_matrix, result);
   }
   else {
      normalize_concentric(ss, analyzer, center_arity, outer_inners,
                           final_elongation, matrix_concept, result);
   }

   getout:

   // Tentatively clear the splitting info.

   result->result_flags.clear_split_info();

   if (analyzer == schema_concentric && center_arity == 1) {
      // Set the result fields to the minimum of the result fields of the
      // two components.  Start by setting to the outers, then bring in the inners.
      result->result_flags.copy_split_info(outer_inners[0].result_flags);
      minimize_splitting_info(result, outer_inners[1].result_flags);
   }
   else if (analyzer == schema_in_out_triple && center_arity == 2 &&
            result->kind == s2x9 && (result->result_flags.misc & RESULTFLAG__EXPAND_TO_2X3)) {
      static const expand::thing fix_2x9 = {{0, 1, 3, 4, 5, 7, 8, 9, 10, 12, 13, 14, 16, 17}, s2x7, s2x9, 0};
      if ((little_endian_live_mask(result) & 0104104) == 0)
         expand::compress_setup(fix_2x9, result);
   }
   result->rotation += rotate_back;
   reinstate_rotation(ss, result);
}



const merge_table::concmerge_thing merge_table::map_tgl4l = {
   nothing, nothing, 0, 0, 0, 0x0, schema_by_array, s1x4, nothing,
   warn__none, 0, 0, {0, 1, -1, -1}, {0}};
const merge_table::concmerge_thing merge_table::map_tgl4b = {
   nothing, nothing, 0, 0, 0, 0x0, schema_by_array, s2x2, nothing,
   warn__none, 0, 0, {-1, -1, 2, 3}, {0}};
const merge_table::concmerge_thing merge_table::map_2x3short = {
   nothing, nothing, 0, 0, 0, 0x1, schema_concentric, s1x2, s1x2,
   warn__none, 1, 1, {1, 4}, {1, 4}};
const merge_table::concmerge_thing merge_table::map_2234b = {
   nothing, nothing, 0, 0, 0, 0x0, schema_matrix, s4x4, nothing,
   warn__none, 0, 0, {15, 3, 7, 11}, {12, 13, 14, 0, -1, -1, 4, 5, 6, 8, -1, -1}};
const merge_table::concmerge_thing merge_table::map_24r24a = {
   nothing, nothing, 0, 0, 0, 0x0, schema_matrix, s2x4, nothing,
   warn__check_2x4, 0, 1, {0, 1, 2, -1, 4, 5, 6, -1}, {7, -1, -1, -1, 3, -1, -1, -1}};
const merge_table::concmerge_thing merge_table::map_24r24b = {
   nothing, nothing, 0, 0, 0, 0x0, schema_matrix, s2x4, nothing,
   warn__check_2x4, 0, 1, {-1, 1, 2, 3, -1, 5, 6, 7}, {-1, -1, -1, 0, -1, -1, -1, 4}};
const merge_table::concmerge_thing merge_table::map_24r24c = {
   nothing, nothing, 0, 0, 0, 0x0, schema_matrix, s2x4, nothing,
   warn__check_2x4, 0, 0, {3, -1, -1, -1, 7, -1, -1, -1}, {0, 1, 2, -1, 4, 5, 6, -1}};
const merge_table::concmerge_thing merge_table::map_24r24d = {
   nothing, nothing, 0, 0, 0, 0x0, schema_matrix, s2x4, nothing,
   warn__check_2x4, 0, 0, {-1, -1, -1, 4, -1, -1, -1, 0}, {-1, 1, 2, 3, -1, 5, 6, 7}};


merge_table::concmerge_thing *merge_table::merge_hash_tables[merge_table::NUM_MERGE_HASH_BUCKETS];


void merge_table::initialize()
{
   concmerge_thing *tabp;
   int i;

   for (i=0 ; i<NUM_MERGE_HASH_BUCKETS ; i++) merge_hash_tables[i] = (concmerge_thing *) 0;

   for (tabp = merge_init_table ; tabp->k1 != nothing ; tabp++) {
      uint32_t hash_num = ((tabp->k1 + (5*tabp->k2)) * 25) & (NUM_MERGE_HASH_BUCKETS-1);

      tabp->next = merge_hash_tables[hash_num];
      merge_hash_tables[hash_num] = tabp;
   }
}



// This overwrites its first argument setup.
void merge_table::merge_setups(setup *ss,
                               merge_action action,
                               setup *result,
                               call_with_name *maybe_the_call /* = (call_with_name *) 0 */) THROW_DECL
{
   int i, r, rot;
   setup outer_inners[2];
   int reinstatement_rotation = 0;
   int reinstatement_eighth = 0;
   bool rose_from_dead = false;
   bool perp_2x4_1x8 = false;
   bool perp_2x4_ptp = false;
   normalize_action na = normalize_before_merge;

   setup res2copy = *result;
   setup *res1 = ss;
   setup *res2 = &res2copy;

   if (res1->kind == s_dead_concentric || res2->kind == s_dead_concentric)
      rose_from_dead = true;
   else if ((res1->kind == s_qtag && res2->kind == s_qtag) ||
            (res1->kind == sdmd && res2->kind == sdmd)) {
      // The situation with 1/4 tags is difficult.  Don't normalize the
      // outsides away.  Cf. vq9t.  Diamonds too.
      canonicalize_rotation(res1);
      canonicalize_rotation(res2);
      if (res1->rotation == res2->rotation) {
         *result = *res2;
         bool what_do_we_put_in = action >= merge_c1_phantom;
         collision_collector CC(result, what_do_we_put_in ? collision_severity_ok : collision_severity_no);
         CC.note_prefilled_result();

         for (i=0; i<=attr::slimit(res1); i++) {
            if (res1->people[i].id1)
               CC.install_with_collision(i, res1, i, 0);
         }

         CC.fix_possible_collision();
         canonicalize_rotation(result);
         return;
      }
   }

   merge_action orig_action = action;
   // Only use the special triangle stuff if doing brute force merge.
   if (action == merge_c1_phantom_from_triangle_call)
      action = merge_c1_phantom;

   if (action == merge_after_dyp) {
      action = merge_c1_phantom;
      na = normalize_before_isolate_not_too_strict;
   }
   else if (action == merge_c1_phantom_real)
      na = normalize_before_isolate_not_too_strict;
   else if (action == merge_strict_matrix || action == merge_for_own)
      na = normalize_strict_matrix;
   else if (action == merge_without_gaps)
      na = normalize_after_disconnected;

   // If either incoming setup is big, opt for a 4x4 rather than C1 phantoms.
   // The test for this is, from a squared set, everyone phantom column wheel thru.
   // We want a 4x4.

   if ((res1->rotation ^ res2->rotation) & 1) {
      if ((res1->kind == s2x4 && res2->kind == s1x8) ||
          (res1->kind == s1x8 && res2->kind == s2x4))
         perp_2x4_1x8 = true;
      else if ((res1->kind == s2x4 && res2->kind == s_ptpd) ||
          (res1->kind == s_ptpd && res2->kind == s2x4))
         perp_2x4_ptp = true;  // Test for this is T-boned right wing follow to a diamond.
   }
   else {
      // If similarly oriented 2x3 and qtag ends, move the qtag to a 2x4.  First, canonicalize.
      if (res1->kind == s2x3 && res2->kind == s_qtag) {
         setup *temp = res2;
         res2 = res1;
         res1 = temp;
      }

      uint32_t mask1 = little_endian_live_mask(res1);
      uint32_t mask2 = little_endian_live_mask(res2);

      if (res1->kind == s_qtag && res2->kind == s2x3 && !(mask1 & 0xCC)) {
         expand::compress_from_hash_table(res1, plain_normalize, mask1, false);
      }
      else if (orig_action == merge_c1_phantom_from_triangle_call &&
               res1->kind == s2x8 && !(mask1 & 0x8181) &&
               res2->kind == sd2x7 && !(mask2 & 0x1F3E)) {
         static const expand::thing sd2x7_fixer = {
            {0, -1, -1, -1, -1, -1, 7, 8, -1, -1, -1, -1, -1, 15}, sd2x7, s2x8, 0};
         expand::expand_setup(sd2x7_fixer, res2);
      }
   }

   canonicalize_rotation(res1);    // Do we really need to do this before normalize_setup?
   normalize_setup(res1, na, res2->kind == nothing ? qtag_no_compress : qtag_compress);
   canonicalize_rotation(res1);    // We definitely need to do it now --
                                   // a 2x2 might have been created.

 tryagain:

   canonicalize_rotation(res2);
   normalize_setup(res2, na, res1->kind == nothing ? qtag_no_compress : qtag_compress);
   canonicalize_rotation(res2);

   // Canonicalize the setups according to their kind.  This is a bit sleazy, since
   // the enumeration order of setup kinds is supposed to be insignificant.  We depend in
   // general on small setups being before larger ones.  In particular, we seem to require:
   //    s2x2 < s2x4 < s2x6
   //    s2x2 < s1x8
   //    s1x4 > s1x6 < s1x8
   //    s1x4 < s2x4
   //    s2x4 < s_c1phan
   // You get the idea.

   if (res2->kind < res1->kind) {
      setup *temp = res2;
      res2 = res1;
      res1 = temp;
   }

   // This one looks ugly here.
   configuration::clear_one_warning(warn__phantoms_thinner);

   // If one of the setups was a "concentric" setup in which there are no ends,
   // we can still handle it.

   if ((res2->kind == s_normal_concentric && res2->outer.skind == nothing) ||
       res2->kind == s_dead_concentric) {
      res2->kind = res2->inner.skind;
      res2->rotation += res2->inner.srotation;
      res2->eighth_rotation += res2->inner.seighth_rotation;
      goto tryagain;    // Need to recanonicalize setup order.
   }

   if (rose_from_dead && res1->kind == s1x4 && res2->kind == s4x4) {
      uint32_t mask = little_endian_live_mask(res2);

      if (res1->rotation == 0 && (mask & 0x8E8E) == 0) {
         expand::expand_setup(s_4x4_4dmb, res2);
         goto tryagain;
      }
      else if (res1->rotation == 1 && (mask & 0xE8E8) == 0) {
         expand::expand_setup(s_4x4_4dma, res2);
         goto tryagain;
      }
   }

   // If one of the setups was a "concentric" setup in which there are no centers,
   // merge concentrically.

   if (res2->kind == s_normal_concentric &&
       res2->inner.skind == nothing &&
       action != merge_strict_matrix &&
       action != merge_for_own) {
      res2->kind = res2->outer.skind;
      res2->rotation += res2->outer.srotation;
      res2->eighth_rotation += res2->outer.seighth_rotation;
      canonicalize_rotation(res2);
      for (i=0 ; i<(MAX_PEOPLE/2) ; i++) res2->swap_people(i, i+(MAX_PEOPLE/2));
      int outer_elong = res2->concsetup_outer_elongation;
      outer_inners[0] = *res2;
      outer_inners[1] = *res1;
      normalize_concentric(ss, schema_concentric, 1, outer_inners, outer_elong, 0, result);
   }
   else if (res1->kind == nothing) {
      *result = *res2;
      canonicalize_rotation(result);
      return;
   }
   else {
      if (res1->eighth_rotation != res2->eighth_rotation)
         fail("Rotation is inconsistent.");

      reinstatement_rotation = res2->rotation;
      reinstatement_eighth = res2->eighth_rotation;

      res1->rotation -= res2->rotation;
      res2->rotation = 0;
      result->rotation = 0;
      result->eighth_rotation = 0;
      res1->eighth_rotation = 0;
      res2->eighth_rotation = 0;
      canonicalize_rotation(res1);
      canonicalize_rotation(res2);
      uint32_t mask1 = little_endian_live_mask(res1);
      uint32_t mask2 = little_endian_live_mask(res2);

      r = res1->rotation & 3;
      rot = r * 011;

      uint32_t rotmaskreject = (1<<r);
      if (action != merge_without_gaps) rotmaskreject |= 0x10;
      if (action == merge_without_gaps) rotmaskreject |= 0x400;
      if (action == merge_strict_matrix || action == merge_for_own) rotmaskreject |= 0x20;
      if (!perp_2x4_1x8) rotmaskreject |= 0x40;
      if (!perp_2x4_ptp) rotmaskreject |= 0x200;
      if (action == merge_c1_phantom_real) rotmaskreject |= 0x80;
      if (orig_action == merge_after_dyp) rotmaskreject |= 0x100;

      setup_kind res1k = res1->kind;
      setup_kind res2k = res2->kind;

      // Look for case of simple direct merge.
      if (res1k == res2k && (mask1 & mask2) == 0 && res1->rotation == 0) {
         *result = *res2;
         for (i=0; i<=attr::slimit(result); i++)
            install_person(result, i, res1, i);
         goto final_getout;
      }

      uint32_t hash_num = ((res1k + (5*res2k)) * 25) & (merge_table::NUM_MERGE_HASH_BUCKETS-1);

      const merge_table::concmerge_thing *the_map;

      for (the_map = merge_table::merge_hash_tables[hash_num] ; the_map ; the_map = the_map->next) {
         if (res1k == the_map->k1 &&
             res2k == the_map->k2 &&
             (!(rotmaskreject & the_map->rotmask)) &&
             (mask1 & the_map->m1) == 0 &&
             (mask2 & the_map->m2) == 0) {

            // The 0x10 bit means reject collisions.

            bool reject_this = false;

            if (the_map->swap_setups & 0x10) {
               for (i=0; i<=attr::slimit(res1); i++) {
                  if (res1->people[i].id1 != 0 && res2->people[the_map->innermap[i]].id1 != 0)
                     reject_this = true;
               }
            }

            if (!reject_this) goto one_way_or_another_we_have_a_map;
         }
      }

      // Didn't find a map through the normal table search.  We may be able to
      // do the merge anyway, either by direct action or by coming up with a
      // concocted map.

      if (res1->kind == s2x4 && res2->kind == s2x4 && (r&1)) {
         bool going_to_stars;
         bool going_to_o;
         bool go_to_4x4_anyway;
         bool conflict_at_4x4;
         bool action_suggests_4x4;

         conflict_at_4x4 = ((res2->people[1].id1 & res1->people[6].id1) |
                            (res2->people[2].id1 & res1->people[1].id1) |
                            (res2->people[5].id1 & res1->people[2].id1) |
                            (res2->people[6].id1 & res1->people[5].id1)) != 0;

         going_to_stars =
            ((mask1 == 0x33) && (mask2 == 0xCC)) ||
            ((mask1 == 0xCC) && (mask2 == 0x33));
         going_to_o = ((mask1 | mask2) & 0x66) == 0;
         go_to_4x4_anyway = (mask1 == 0x99) || (mask2 == 0x99);

         // This stuff affects tests t33, t35, and vg06.  The point is that,
         // if we have two groups do calls in distorted setups out of a 4x4,
         // and each group goes to what amounts to a 1x4 with a shear in the
         // middle, then we want either stars or a C1 phantom, as needed.
         // but if the two sets of people are more random (e.g. the messy
         // couple up in vg06), we want a pinwheel (or whatever) on a 4x4.
         // Of course, if the action is merge_strict_matrix, we always go to
         // a 4x4 if possible.

         action_suggests_4x4 =
            action == merge_strict_matrix ||
            action == merge_for_own ||
            (action == merge_without_gaps &&
             ((mask1 != 0x33 && mask1 != 0xCC) || (mask2 != 0x33 && mask2 != 0xCC)));

         if ((action_suggests_4x4 && !going_to_stars && !conflict_at_4x4) ||
             go_to_4x4_anyway || going_to_o) {
            static const int8_t matrixmap1[8] = {14, 3, 7, 5, 6, 11, 15, 13};
            static const int8_t matrixmap2[8] = {10, 15, 3, 1, 2, 7, 11, 9};
            // Make this an instance of expand_setup.
            result->kind = s4x4;
            result->clear_people();
            scatter(result, res1, matrixmap1, 7, 011);
            install_scatter(result, 8, matrixmap2, res2, 0);
            goto final_getout;
         }
         else if (mask1 == 0x77 && mask2 == 0x11) {
            the_map = &merge_table::map_24r24a;
         }
         else if (mask1 == 0xEE && mask2 == 0x88) {
            the_map = &merge_table::map_24r24b;
         }
         else if (mask1 == 0x11 && mask2 == 0x77) {
            the_map = &merge_table::map_24r24c;
         }
         else if (mask1 == 0x88 && mask2 == 0xEE) {
            the_map = &merge_table::map_24r24d;
         }
         else if (mask1 == 0x66 && mask2 == 0x66) {
            static const int8_t alamomap1[8] = {-1, 2, 3, -1, -1, 6, 7, -1};
            static const int8_t alamomap2[8] = {-1, 0, 1, -1, -1, 4, 5, -1};
            result->kind = s_alamo;
            result->clear_people();
            scatter(result, res1, alamomap1, 7, 011);
            install_scatter(result, 8, alamomap2, res2, 0);
            goto final_getout;
         }
         else {
            static const int8_t phanmap1[8] = {4, 6, 11, 9, 12, 14, 3, 1};
            static const int8_t phanmap2[8] = {0, 2, 7, 5, 8, 10, 15, 13};
            uint32_t t1 = mask2 & 0x33;
            uint32_t t2 = mask2 & 0xCC;
            uint32_t t3 = mask1 & 0x33;
            uint32_t t4 = mask1 & 0xCC;

            result->kind = s_c1phan;
            result->clear_people();
            scatter(result, res1, phanmap1, 7, 011);
            scatter(result, res2, phanmap2, 7, 0);

            // See if we have a "classical" C1 phantom setup, and give the appropriate warning.
            if (action != merge_c1_phantom_real) {
               if ((t1 | t3) == 0 || (t2 | t4) == 0)
                  warn(warn__check_c1_phan);
               else if ((t1 | t4) == 0 || (t2 | t3) == 0)
                  warn(warn__check_c1_stars);
               else
                  warn(warn__check_gen_c1_stars);
            }

            goto final_getout;
         }
      }
      else if (res2->kind == s_trngl4 && res1->kind == s_trngl4 && r == 2 &&
               (mask1 & 0xC) == 0 && (mask2 & 0xC) == 0) {
         copy_rot(res2, 0, res2, 0, 011);
         copy_rot(res2, 1, res2, 1, 011);
         res2->rotation = 3;
         res2->eighth_rotation = 0;
         the_map = &merge_table::map_tgl4l;
         r = 1;
      }
      else if (res2->kind == s_trngl4 && res1->kind == s_trngl4 && r == 2 &&
               (mask1 & 0x3) == 0 && (mask2 & 0x3) == 0) {
         res1->swap_people(0, 2);
         res1->swap_people(1, 3);
         res1->kind = s2x2;
         canonicalize_rotation(res1);
         res2->swap_people(0, 2);
         res2->swap_people(1, 3);
         the_map = &merge_table::map_tgl4b;
         r = 0;
      }
      else if (res1->kind == s_short6 && res2->kind == s2x3 && two_couple_calling && r == 1 &&
               (mask1 & 055) == 0 && (mask2 & 055) == 0) {
         the_map = &merge_table::map_2x3short;
      }
      else if (res2->kind == s3x4 && res1->kind == s2x2 && ((mask2 & 06060) == 0)) {
         the_map = &merge_table::map_2234b;
         warn((mask2 & 06666) ? warn__check_4x4 : warn__check_butterfly);
      }
      else {
         // The only remaining hope is that the setups match and we can blindly combine them.
         // Our 180 degree rotation wouldn't work for triangles.

         // But first, watch for case of a 4x4 vs. a C1 phantom.  Just throw people onto the 4x4 and hope for the best.
         if (res1->kind == s4x4 && res2->kind == s_c1phan) {
            static int8_t fixup[16] = {10, 13, 15, 15, 14, 1, 3, 3, 2, 5, 7, 7, 6, 9, 11, 11};
            setup temp = *res2;
            res2->kind = s4x4;
            res2->clear_people();
            install_scatter(res2, 16, fixup, &temp, 0);
         }

         if (action == merge_c1_phantom_real &&
             maybe_the_call &&
             (maybe_the_call->the_defn.callflags1 & CFLAG1_TAKE_RIGHT_HANDS_AS_COUPLES))
            action = merge_c1_phantom_real_couples;

         brute_force_merge(res1, res2, action, result);
         goto final_getout;
      }

   one_way_or_another_we_have_a_map:

      if (the_map->swap_setups & 1) {
         setup *temp = res2;
         res2 = res1;
         res1 = temp;
         r = -r;
      }

      rot = (r & 3) * 011;

      int outer_elongation;

      if (the_map->swap_setups & 8)
         outer_elongation = 3;
      else
         outer_elongation = ((res2->rotation ^ (the_map->swap_setups >> 1)) & 1) + 1;

      warn(the_map->warning);

      if (the_map->conc_type == schema_nothing) {
         *result = *res2;
      }
      else if (the_map->conc_type == schema_by_array) {
         *result = *res2;
         result->kind = the_map->innerk;
         canonicalize_rotation(result);
      }
      else if (the_map->conc_type == schema_matrix) {
         *result = *res2;
         result->rotation += the_map->orot;
         result->kind = the_map->innerk;
         result->clear_people();
         scatter(result, res2, the_map->outermap, attr::slimit(res2),
                 ((-the_map->orot) & 3) * 011);
         r += the_map->irot - the_map->orot;
         rot = (r & 3) * 011;
      }
      else {
         rot = 0;
         res2->kind = the_map->outerk;
         if (the_map->orot) {
            res2->rotation++;
            rot = 033;
         }
         outer_inners[0] = *res2;
         outer_inners[0].clear_people();
         gather(&outer_inners[0], res2, the_map->outermap, attr::slimit(res2), rot);
         canonicalize_rotation(&outer_inners[0]);

         rot = 0;
         res1->kind = the_map->innerk;
         if (the_map->irot) {
            res1->rotation++;
            rot = 033;
         }
         outer_inners[1] = *res1;
         outer_inners[1].clear_people();
         gather(&outer_inners[1], res1, the_map->innermap, attr::slimit(res1), rot);
         canonicalize_rotation(&outer_inners[1]);
         normalize_concentric(ss, the_map->conc_type, 1, outer_inners, outer_elongation, 0, result);
         goto final_getout;
      }

      bool what_do_we_put_in = action >= merge_c1_phantom && !(the_map->swap_setups & 4);
      collision_collector CC(result, what_do_we_put_in ? collision_severity_ok : collision_severity_no);
      CC.note_prefilled_result();

      for (i=0; i<=attr::slimit(res1); i++) {
         if (res1->people[i].id1)
            CC.install_with_collision(the_map->innermap[i], res1, i, rot);
      }

      CC.fix_possible_collision();
   }

 final_getout:

   result->rotation += reinstatement_rotation;
   result->eighth_rotation = reinstatement_eighth;
   canonicalize_rotation(result);
}


extern void on_your_own_move(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL
{
   setup setup1, setup2, res1;
   setup outer_inners[2];

   if (ss->kind != s2x4) fail("Must have 2x4 setup for 'on your own'.");

   warning_info saved_warnings = configuration::save_warnings();

   setup1 = *ss;              // Get outers only.
   setup1.clear_person(1);
   setup1.clear_person(2);
   setup1.clear_person(5);
   setup1.clear_person(6);
   setup1.cmd = ss->cmd;
   setup1.cmd.cmd_misc_flags |= CMD_MISC__DISTORTED | CMD_MISC__PHANTOMS;

   if ((setup1.cmd.cmd_misc3_flags &
        (CMD_MISC3__PUT_FRAC_ON_FIRST|CMD_MISC3__RESTRAIN_CRAZINESS)) ==
       CMD_MISC3__PUT_FRAC_ON_FIRST) {
      // Curried meta-concept.  Take the fraction info off the first call.
      setup1.cmd.cmd_misc3_flags &= ~CMD_MISC3__PUT_FRAC_ON_FIRST;
      setup1.cmd.cmd_fraction.set_to_null();
   }

   move(&setup1, false, &res1);

   setup2 = *ss;              // Get inners only.
   setup2.clear_person(0);
   setup2.clear_person(3);
   setup2.clear_person(4);
   setup2.clear_person(7);
   setup2.cmd = ss->cmd;
   setup2.cmd.cmd_misc_flags |= CMD_MISC__DISTORTED | CMD_MISC__PHANTOMS;
   setup2.cmd.parseptr = parseptr->subsidiary_root;

   move(&setup2, false, result);

   outer_inners[0] = res1;
   outer_inners[1] = *result;

   result->result_flags = get_multiple_parallel_resultflags(outer_inners, 2);
   merge_table::merge_setups(&res1, merge_strict_matrix, result);

   // Shut off "superfluous phantom setups" warnings.

   configuration::clear_multiple_warnings(useless_phan_clw_warnings);
   configuration::set_multiple_warnings(saved_warnings);
}


// This places an assumption into a command structure, if the setup
// is fully occupied and supports that assumption.
void infer_assumption(setup *ss)
{
   int sizem1 = attr::slimit(ss);

   if (ss->cmd.cmd_assume.assumption == cr_none && sizem1 >= 0) {
      uint32_t directions, livemask;
      big_endian_get_directions32(ss, directions, livemask);
      if (livemask == (uint32_t) (1<<((sizem1+1)<<1))-1) {
         assumption_thing tt;
         tt.assump_col = 0;
         tt.assump_both = 0;
         tt.assump_cast = 0;
         tt.assump_live = 0;
         tt.assump_negate = 0;

         if (ss->kind == s_qtag) {
            switch (directions) {
            case 0x58F2:
               tt.assump_both = 1;
               tt.assump_col = 4;
               tt.assumption = cr_jright;
               break;
            case 0x52F8:
               tt.assump_both = 1;
               tt.assump_col = 4;
               tt.assumption = cr_jleft;
               break;
            case 0xF852:
               tt.assump_both = 2;
               tt.assump_col = 4;
               tt.assumption = cr_jleft;
               break;
            case 0xF258:
               tt.assump_both = 2;
               tt.assump_col = 4;
               tt.assumption = cr_jright;
               break;
            case 0x5AF0:
               tt.assump_both = 1;
               tt.assump_col = 4;
               tt.assumption = cr_ijright;
               break;
            case 0x50FA:
               tt.assump_both = 1;
               tt.assump_col = 4;
               tt.assumption = cr_ijleft;
               break;
            case 0xFA50:
               tt.assump_both = 2;
               tt.assump_col = 4;
               tt.assumption = cr_ijleft;
               break;
            case 0xF05A:
               tt.assump_both = 2;
               tt.assump_col = 4;
               tt.assumption = cr_ijright;
               break;
            case 0x08A2:
               tt.assump_both = 1;
               tt.assumption = cr_jright;
               break;
            case 0x02A8:
               tt.assump_both = 1;
               tt.assumption = cr_jleft;
               break;
            case 0xA802:
               tt.assump_both = 2;
               tt.assumption = cr_jleft;
               break;
            case 0xA208:
               tt.assump_both = 2;
               tt.assumption = cr_jright;
               break;
            case 0x0AA0:
               tt.assump_both = 1;
               tt.assumption = cr_ijright;
               break;
            case 0x00AA:
               tt.assump_both = 1;
               tt.assumption = cr_ijleft;
               break;
            case 0xAA00:
               tt.assump_both = 2;
               tt.assumption = cr_ijleft;
               break;
            case 0xA00A:
               tt.assump_both = 2;
               tt.assumption = cr_ijright;
               break;
            default:
               return;
            }

            ss->cmd.cmd_assume = tt;
         }
      }
   }
}



// We know that the setup has well-defined size, and that the conctable masks are good.
extern void punt_centers_use_concept(setup *ss, setup *result) THROW_DECL
{
   int i, setupcount;
   setup the_setups[2], the_results[2];
   int sizem1 = attr::slimit(ss);
   uint32_t cmd2word = ss->cmd.cmd_misc2_flags;
   int crossconc = (cmd2word & CMD_MISC2__ANY_WORK_INVERT) ? 1 : 0;
   bool doing_yoyo = false;
   bool doing_do_last_frac = false;
   parse_block *parseptrcopy;

   remove_z_distortion(ss);

   // Clear the stuff out of the cmd_misc2_flags word.

   ss->cmd.cmd_misc2_flags &=
      ~(0xFFF |
        CMD_MISC2__ANY_WORK_INVERT |
        CMD_MISC2__ANY_WORK | CMD_MISC2__ANY_SNAG |
        CMD_MISC2__SAID_INVERT | CMD_MISC2__CENTRAL_SNAG | CMD_MISC2__INVERT_SNAG);

   // Before we separate the two groups from each other, it may be helpful
   // to infer an assumption from the whole setup.  That way, each group
   // will have a recollection of what the facing directions of the other group
   // were, which will lead to more realistic execution.  This helps test lg01t.

   infer_assumption(ss);

   the_setups[0] = *ss;              // designees
   the_setups[1] = *ss;              // non-designees

   uint32_t ssmask = setup_attrs[ss->kind].setup_conc_masks.mask_normal;

   if (cmd2word & (CMD_MISC2__ANY_WORK | CMD_MISC2__ANY_SNAG)) {
      switch ((calldef_schema) (cmd2word & 0xFFF)) {
      case schema_concentric_2_6:
         ssmask = setup_attrs[ss->kind].setup_conc_masks.mask_2_6;
         break;
      case schema_concentric_6_2:
         ssmask = setup_attrs[ss->kind].setup_conc_masks.mask_6_2;
         break;
      case schema_concentric_diamonds:
         ssmask = setup_attrs[ss->kind].setup_conc_masks.mask_ctr_dmd;
         break;
      }
   }

   for (i=sizem1; i>=0; i--) {
      the_setups[(ssmask & 1) ^ crossconc].clear_person(i);
      ssmask >>= 1;
   }

   normalize_action normalizer = normalize_before_isolated_call;

   // Check for "someone work yoyo".  If call is sequential and yoyo is consumed by
   // first part, then just do this stuff on the first part.  After that, merge
   // the setups and finish the call normally.

   if ((cmd2word & CMD_MISC2__ANY_WORK) &&
       (ss->cmd.parseptr->concept->kind == concept_yoyo || ss->cmd.parseptr->concept->kind == concept_generous) &&
       ss->cmd.parseptr->next->concept->kind == marker_end_of_list &&
       (ss->cmd.parseptr->next->call->the_defn.schema == schema_sequential ||
        ss->cmd.parseptr->next->call->the_defn.schema == schema_sequential_alternate ||
        ss->cmd.parseptr->next->call->the_defn.schema == schema_sequential_remainder) &&
       (ss->cmd.parseptr->next->call->the_defn.callflagsherit & INHERITFLAG_YOYOETCMASK) != 0ULL &&
       (ss->cmd.parseptr->next->call->the_defn.stuff.seq.defarray[0].modifiersh & INHERITFLAG_YOYOETCMASK) != 0ULL &&
       ss->cmd.cmd_fraction.is_null()) {
      doing_yoyo = true;
      ss->cmd.cmd_fraction.set_to_null_with_flags(
         CMD_FRAC_BREAKING_UP | CMD_FRAC_FORCE_VIS | CMD_FRAC_PART_BIT);
   }
   else if ((cmd2word & CMD_MISC2__ANY_WORK) &&
            ss->cmd.parseptr->concept->kind == concept_fractional &&
            ss->cmd.parseptr->concept->arg1 == 1 &&
            ss->cmd.cmd_fraction.is_null()) {
      doing_do_last_frac = true;
   }
   else if ((cmd2word & CMD_MISC2__ANY_WORK) && ss->cmd.parseptr->concept->kind == concept_tandem) {
      normalizer = normalize_before_isolated_callMATRIXMATRIXMATRIX;
   }

   normalize_setup(&the_setups[0], normalizer, qtag_compress);
   normalize_setup(&the_setups[1], normalizer, qtag_compress);
   warning_info saved_warnings = configuration::save_warnings();

   for (setupcount=0; setupcount<2; setupcount++) {
      setup *this_one = &the_setups[setupcount];
      this_one->cmd = ss->cmd;

      if (doing_do_last_frac) {
         if (setupcount == 0) {
            the_results[0] = *this_one;
            continue;                                 // Designees do nothing.
         }
         else {
            // Non-designees do first part only.
            this_one->cmd.cmd_fraction.process_fractions(NUMBER_FIELDS_1_0,
                                                         ss->cmd.parseptr->options.number_fields,
                                                         FRAC_INVERT_END,
                                                         this_one->cmd.cmd_fraction);
            this_one->cmd.cmd_fraction.flags = 0;
         }
      }

      this_one->cmd.cmd_misc_flags |= CMD_MISC__PHANTOMS;

      if (setupcount == 1 && (cmd2word & CMD_MISC2__ANY_WORK)) {
         skipped_concept_info foo(ss->cmd.parseptr);

         this_one->cmd.parseptr = foo.m_result_of_skip;
         parseptrcopy = foo.m_old_retval;

         if (foo.m_skipped_concept->concept->kind == concept_supercall)
            fail("A concept is required.");
      }
      else if (setupcount == 0 &&
               ((cmd2word & CMD_MISC2__ANY_SNAG) ||
                (cmd2word &
                 (CMD_MISC2__SAID_INVERT|CMD_MISC2__CENTRAL_SNAG|CMD_MISC2__INVERT_SNAG)) ==
                CMD_MISC2__CENTRAL_SNAG)) {
         if (this_one->cmd.cmd_fraction.is_null())
            this_one->cmd.cmd_fraction.set_to_firsthalf();
         else if (this_one->cmd.cmd_fraction.is_null_with_exact_flags(CMD_FRAC_REVERSE))
            this_one->cmd.cmd_fraction.set_to_lasthalf();
         else
            this_one->cmd.cmd_fraction.flags |= CMD_FRAC_FIRSTHALF_ALL;
      }
      else if (setupcount == 1 &&
               (cmd2word &
                (CMD_MISC2__SAID_INVERT|CMD_MISC2__CENTRAL_SNAG|CMD_MISC2__INVERT_SNAG)) ==
               (CMD_MISC2__CENTRAL_SNAG|CMD_MISC2__INVERT_SNAG)) {
         if (this_one->cmd.cmd_fraction.is_null())
            this_one->cmd.cmd_fraction.set_to_firsthalf();
         else if (this_one->cmd.cmd_fraction.is_null_with_exact_flags(CMD_FRAC_REVERSE))
            this_one->cmd.cmd_fraction.set_to_lasthalf();
         else
            this_one->cmd.cmd_fraction.flags |= CMD_FRAC_FIRSTHALF_ALL;
      }

      move(this_one, false, &the_results[setupcount]);
   }

   // Shut off "each 1x4" types of warnings -- they will arise spuriously
   // while the people do the calls in isolation.
   configuration::clear_multiple_warnings(dyp_each_warnings);
   configuration::set_multiple_warnings(saved_warnings);

   *result = the_results[0];

   if (!doing_do_last_frac) {
      if (!(result->result_flags.misc & RESULTFLAG__EXPAND_TO_2X3) &&
          (the_results[1].result_flags.misc & RESULTFLAG__EXPAND_TO_2X3) &&
          result->rotation == the_results[1].rotation) {
         if (result->kind == s2x4 &&
             the_results[1].kind == s2x4 &&
             ((result->people[1].id1 & the_results[1].people[1].id1) |
              (result->people[2].id1 & the_results[1].people[2].id1) |
              (result->people[5].id1 & the_results[1].people[5].id1) |
              (result->people[6].id1 & the_results[1].people[6].id1))) {
            // We are doing something like "snag pair the line".  The centers
            // didn't know which way the ends went, so they punted, moved into
            // the center, and cleared the bit to say they really didn't know
            // that this was right.  We now have the information, and it isn't
            // good -- there is a collision.  We need to move the_results[0] outward.
            result->swap_people(0, 1);
            result->swap_people(3, 2);
            result->swap_people(4, 5);
            result->swap_people(7, 6);
         }
         else if (result->kind == s2x4 &&
                  the_results[1].kind == s2x6 &&
                  ((result->people[1].id1 & the_results[1].people[2].id1) |
                   (result->people[2].id1 & the_results[1].people[3].id1) |
                   (result->people[5].id1 & the_results[1].people[8].id1) |
                   (result->people[6].id1 & the_results[1].people[9].id1))) {
            result->swap_people(0, 1);
            result->swap_people(3, 2);
            result->swap_people(4, 5);
            result->swap_people(7, 6);
         }
         else if (result->kind == s1x6 &&
                  the_results[1].kind == s2x4 &&
                  (((result->people[0].id1 | result->people[1].id1) &
                    (the_results[1].people[0].id1 | the_results[1].people[7].id1)) |
                   ((result->people[3].id1 | result->people[4].id1) &
                    (the_results[1].people[3].id1 | the_results[1].people[4].id1)))) {
            result->swap_people(1, 2);
            result->swap_people(0, 1);
            result->swap_people(4, 5);
            result->swap_people(3, 4);
         }
      }
      else if ((result->result_flags.misc & RESULTFLAG__EXPAND_TO_2X3) &&
          (the_results[1].result_flags.misc & RESULTFLAG__EXPAND_TO_2X3) &&
          result->rotation == the_results[1].rotation) {
         if (result->kind == s_rigger &&
             the_results[1].kind == s2x4 &&
             (result->people[0].id1 |
              result->people[1].id1 |
              result->people[4].id1 |
              result->people[5].id1 |
              the_results[1].people[1].id1 |
              the_results[1].people[2].id1 |
              the_results[1].people[5].id1 |
              the_results[1].people[6].id1) == 0) {
            // We are doing something like "snag pair the line".  The centers
            // didn't know which way the ends went, so they left room for them.
            // The ends moved to the outside.
            result->kind = s_bone;   // This is all it takes -- merge_setups will do the rest.
         }
      }

      result->result_flags = get_multiple_parallel_resultflags(the_results, 2);
   }

   merge_table::merge_setups(&the_results[1], merge_c1_phantom, result);

   if (doing_yoyo) {
      the_setups[0] = *result;
      the_setups[0].cmd = ss->cmd;    // Restore original command stuff (though we clobbered fractionalization info).
      the_setups[0].cmd.cmd_assume.assumption = cr_none;  // Assumptions don't carry through.
      the_setups[0].cmd.cmd_fraction.set_to_null_with_flags(
         CMD_FRAC_BREAKING_UP | CMD_FRAC_FORCE_VIS |
         FRACS(CMD_FRAC_CODE_FROMTOREV,2,0));
      the_setups[0].cmd.parseptr = parseptrcopy->next;      // Skip over the concept.
      uint32_t finalresultflagsmisc = the_setups[0].result_flags.misc;
      move(&the_setups[0], false, result);
      finalresultflagsmisc |= result->result_flags.misc;
      normalize_setup(result, simple_normalize, qtag_compress);
      result->result_flags.misc = finalresultflagsmisc & ~3;
   }
   else if (doing_do_last_frac) {
      the_setups[0] = *result;
      the_setups[0].cmd = ss->cmd;    // Restore original command stuff.
      the_setups[0].cmd.cmd_assume.assumption = cr_none;  // Assumptions don't carry through.

      the_setups[0].cmd.cmd_fraction.flags ^= CMD_FRAC_REVERSE;
      the_setups[0].cmd.cmd_fraction.process_fractions(NUMBER_FIELDS_1_0,
                                                       ss->cmd.parseptr->options.number_fields,
                                                       FRAC_INVERT_NONE,
                                                       the_setups[0].cmd.cmd_fraction);
      the_setups[0].cmd.cmd_fraction.flags = 0;
      the_setups[0].cmd.parseptr = parseptrcopy->next;    // Skip over the concept.
      uint32_t finalresultflagsmisc = the_setups[0].result_flags.misc;
      move(&the_setups[0], false, result);
      finalresultflagsmisc |= result->result_flags.misc;
      normalize_setup(result, simple_normalize, qtag_compress);
      result->result_flags.misc = finalresultflagsmisc & ~3;
   }
}



// This does the various types of "so-and-so do this, perhaps while the others
// do that" concepts.

/* indicator =
   selective_key_dyp                   -  <> do your part
   selective_key_dyp + others          -  <> do your part while the others ....
   selective_key_own + others          -  own the <>, .... by ....
   selective_key_plain                 -  <>
   selective_key_plain + others        -  <> while the others ....
   selective_key_disc_dist             -  <> disconnected   or   <> distorted
   selective_key_disc_dist + others    -  <> disconnected .... while the others ....
   selective_key_ignore                -  <> ignored
   selective_key_work_concept + others -  <> work <concept> (the others do the call,
                                           but without the concept)
   selective_key_lead_for_a, selective_key_promenade_and_lead_for_a
                                       -  <> lead for a .... (as in cast a shadow
                                           from a promenade)
   selective_key_work_no_concentric    -  like selective_key_work_concept, but don't
                                           use concentric_move
   selective_key_snag_anyone + others  -  <> work 1/2, the others do the whole call
                                           (i.e. snag the <>)
   selective_key_plain_from_id_bits    -  like selective_key_plain, but don't use concentric_move


   arg2 = 0 - not doing distorted setup, this is the usual case, people work in the
                       actual setup that they have
   arg2 nonzero is used ONLY when indicator = selective_key_disc_dist:
      1/17/33 - distorted line -- used only with indicator = selective_key_disc_dist
                (16 bit means user said "diagonal", 32 bit means user said "offset")
      2/18/34 - distorted column (same as above)
      3/19/35 - distorted wave   (same as above)
      4       - distorted box
      5       - distorted diamond
      6       - Z */


extern void selective_move(
   setup *ss,
   parse_block *parseptr,
   selective_key indicator,
   int others,  // -1 - only selectees do the call, others can still roll
                //  0 - only selectees do the call, others can't roll
                //  1 - both sets
                //  9 - same sex disconnected - both sets, same call, there is no selector
   uint32_t arg2,
   uint32_t override_selector,
   const who_list &selector_to_use,
   bool concentric_rules,
   setup *result) THROW_DECL
{
   setup_command cmd1thing, cmd2thing;

   cmd1thing = ss->cmd;
   cmd2thing = ss->cmd;

   if (indicator == selective_key_work_concept) {
      // This is "<anyone> work <concept>".  Pick out the concept to be skipped
      // by the unselected people.

      // First, be sure these things are really correct.  Removal of
      // restrained concepts could have messed them up.
      ss->cmd.parseptr = parseptr->next;
      cmd1thing.parseptr = parseptr->next;
      cmd2thing.parseptr = parseptr->next;

      skipped_concept_info foo(parseptr->next);

      cmd2thing.parseptr = foo.m_result_of_skip;

      const parse_block *kkk = foo.m_skipped_concept;
      const concept_descriptor *kk = kkk->concept;
      concept_kind k = kk->kind;

      if (k == concept_supercall)
         fail("A concept is required.");

      // If this is a concept like "split phantom diamonds", we want to
      // refrain from the "centers" optimization.  That is, we don't want
      // to do this in terms of concentric_move.  Change the indicator
      // to indicate this.

      if (concept_table[k].concept_prop & CONCPROP__SET_PHANTOMS)
         indicator = selective_key_work_no_concentric;

      // The "<anyone> WORK" concept is set to allow selectors in the "some" region
      // (e.g. "some", "inside triangles"), because it has a control key letter of 'K'.
      // But these special selectors are only allowed if the concept is a couples/tandem
      // concept, in which case the "<anyone> work tandem" will be changed to
      // "<anyone> are tandem".

      if ((k == concept_tandem || k == concept_frac_tandem) &&
          kk->arg1 == 0 &&
          kk->arg2 == 0 &&
          (kk->arg3 & ~0xF0) == 0 &&
          (kk->arg4 == tandem_key_cpls || kk->arg4 == tandem_key_tand ||
           kk->arg4 == tandem_key_cpls3 || kk->arg4 == tandem_key_tand3 ||
           kk->arg4 == tandem_key_special_triangles) &&
          !ss->cmd.cmd_final_flags.bool_test_heritbits(INHERITFLAG_SINGLE | INHERITFLAG_MXNMASK |
                                                       INHERITFLAG_NXNMASK | INHERITFLAG_TWISTED)) {
         // Save a few things in case we have to try a second theory.
         uint32_t save_flags = ss->cmd.cmd_misc_flags;
         parse_block *save_parse = ss->cmd.parseptr;
         ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;
         ss->cmd.parseptr = cmd2thing.parseptr;  // Skip the concept.

         try {
            tandem_couples_move(ss,
                                parseptr->options.who,
                                kk->arg3 >> 4,
                                kkk->options.number_fields,
                                0,
                                (tandem_key) kk->arg4,
                                0ULL,
                                false,
                                result);
            return;
         }
         catch(error_flag_type) {
            ss->cmd.cmd_misc_flags = save_flags;
            ss->cmd.parseptr = save_parse;
            // Fall into regular "work" code.
         }
      }
      else {
         // If it wasn't a couples/tandem concept, the special selectors are not allowed.
         if (selector_to_use.who[0] >= selector_SOME_START)
            fail("Not a legal designator with 'WORK'.");

         if ((k == concept_stable || k == concept_frac_stable) && kk->arg1 == 0) {
            ss->cmd.parseptr = cmd2thing.parseptr;  // Skip the concept.
            stable_move(ss,
                        kk->arg2 != 0,
                        false,
                        kkk->options.number_fields,
                        parseptr->options.who.who[0],
                        result);
            return;
         }
         else if (k == concept_nose) {
            ss->cmd.parseptr = cmd2thing.parseptr;  // Skip the concept.
            nose_move(ss, false, parseptr->options.who.who[0], kkk->options.where, result);
            return;
         }
      }
   }
   else if (indicator != selective_key_snag_anyone && others != 9)
      cmd2thing.parseptr = parseptr->subsidiary_root;

   inner_selective_move(
      ss,
      &cmd1thing,
      (others > 0) ? &cmd2thing : (setup_command *) 0,
      indicator,
      others,
      arg2,
      false,
      override_selector,
      selector_to_use,
      0,
      (concentric_rules ? DFM1_CONC_CONCENTRIC_RULES : 0),
      result);
}


extern void inner_selective_move(
   setup *ss,
   setup_command *cmd1,
   setup_command *cmd2,
   selective_key indicator,
   int others,  // -1 - only selectees do the call, others can still roll
                //  0 - only selectees do the call, others can't roll
                //  1 - both sets
                //  9 - same sex disconnected - both sets, same call, there is no selector
   uint32_t arg2,
   bool demand_both_setups_live,
   uint32_t override_selector,
   const who_list &selector_to_use,
   uint32_t modsa1,
   uint32_t modsb1,
   setup *result) THROW_DECL
{
   int i, k;
   int setupcount;
   bool crossconc;
   uint32_t livemask[2];
   uint32_t j;
   uint32_t rotstate, pointclip;
   warning_info saved_warnings;
   calldef_schema schema;
   bool enable_3x1_warn = true;
   setup the_setups[2], the_results[2];
   uint32_t bigend_ssmask, bigend_llmask;
   int sizem1 = attr::slimit(ss);
   selective_key orig_indicator = indicator;
   normalize_action action = normalize_before_isolated_call;
   bool force_matrix_merge = false;
   bool inner_shape_change = false;
   bool doing_special_promenade_thing = false;
   int tglindicator = 0;
   bool special_tgl_ignore = false;
   uint32_t mask = ~(~0 << (sizem1+1));
   const ctr_end_mask_rec *ctr_end_masks_to_use = &dead_masks;
   selector_kind local_selector = selector_to_use.who[0];
   bool special_unsym_dyp = false;

   if (local_selector == selector_outsides)
      local_selector = selector_ends;

   if (ss->cmd.cmd_misc2_flags & CMD_MISC2__DO_NOT_EXECUTE) {
      clear_result_flags(result);
      result->kind = nothing;
      return;
   }

   if (indicator == selective_key_plain ||
       indicator == selective_key_plain_from_id_bits ||
       indicator == selective_key_plain_no_live_subsets) {

      switch (local_selector) {
      case selector_ctrdmd:
      case selector_thediamond:
         ss->cmd.cmd_misc3_flags |= CMD_MISC3__SAID_DIAMOND;
         break;
      case selector_magic_inpoint_tgl:
      case selector_magic_outpoint_tgl:
      case selector_magic_beaupoint_tgl:
      case selector_magic_bellepoint_tgl:
         if (calling_level < magic_triangle_level)
            warn_about_concept_level();
         ss->cmd.cmd_misc3_flags |= CMD_MISC3__SAID_TRIANGLE;
         break;
      case selector_intlk_magic_inpoint_tgl:
      case selector_magic_intlk_inpoint_tgl:
      case selector_intlk_magic_outpoint_tgl:
      case selector_magic_intlk_outpoint_tgl:
      case selector_intlk_magic_beaupoint_tgl:
      case selector_magic_intlk_beaupoint_tgl:
      case selector_intlk_magic_bellepoint_tgl:
      case selector_magic_intlk_bellepoint_tgl:
         if (calling_level < magic_triangle_level)
            warn_about_concept_level();
         // FALL THROUGH
      case selector_inside_intlk_tgl:
      case selector_intlk_inside_tgl:
      case selector_outside_intlk_tgl:
      case selector_intlk_outside_tgl:
      case selector_inpoint_intlk_tgl:
      case selector_intlk_inpoint_tgl:
      case selector_outpoint_intlk_tgl:
      case selector_intlk_outpoint_tgl:
      case selector_intlk_beaupoint_tgl:
      case selector_bellepoint_tgl:
      case selector_bellepoint_intlk_tgl:
      case selector_intlk_bellepoint_tgl:
      case selector_wave_base_intlk_tgl:
      case selector_intlk_wave_base_tgl:
      case selector_tand_base_intlk_tgl:
      case selector_intlk_tand_base_tgl:
      case selector_anyone_base_intlk_tgl:
      case selector_intlk_anyone_base_tgl:
         // FELL THROUGH!
         if (calling_level < intlk_triangle_level)
            warn_about_concept_level();
         // FALL THROUGH
      case selector_thetriangle:
      case selector_neartriangle:
      case selector_fartriangle:
      case selector_outside_tgl:
      case selector_inside_tgl:
      case selector_inpoint_tgl:
      case selector_outpoint_tgl:
      case selector_beaupoint_tgl:
      case selector_beaupoint_intlk_tgl:
      case selector_wave_base_tgl:
      case selector_tand_base_tgl:
      case selector_anyone_base_tgl:
         // FELL THROUGH!
         ss->cmd.cmd_misc3_flags |= CMD_MISC3__SAID_TRIANGLE;
         break;
      }

      action = normalize_before_merge;
      if (others <= 0 && sizem1 == 3) {
         switch (local_selector) {
         case selector_center2:
         case selector_verycenters:
            local_selector = selector_centers;
            break;
         case selector_center4:
         case selector_ctrdmd:
         case selector_ctr_1x4:
         case selector_center_wave:
         case selector_center_line:
         case selector_center_col:
         case selector_center_box:
            local_selector = selector_everyone;
            break;
         }
      }
   }

   if (others == 9) {
      local_selector = selector_boys;
      others = 1;
   }

   // Special case for picking out center Z for crazy Z's.
   if ((ss->cmd.cmd_misc3_flags & CMD_MISC3__IMPOSE_Z_CONCEPT) &&
       attr::slimit(ss) > 7 &&
       indicator == selective_key_plain &&
       local_selector == selector_center4 &&
       others <= 0) {
      ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;

      if (ss->kind == s4x5)
         override_selector = 0x701C0;
      else {
         concentric_move(ss, cmd1, cmd2, schema_in_out_triple_zcom, modsa1, modsb1, true, enable_3x1_warn, ~0U, result);
         return;
      }
   }

   // Nonzero means we are not using the given selector, but are overriding it
   // with specific "decoder" information.
   uint32_t not_using_given_selector = ss->cmd.restrained_selector_decoder[0] | ss->cmd.restrained_selector_decoder[1];
   who_list saved_selector = current_options.who;

   if (ss->kind == s_normal_concentric && indicator == selective_key_plain) {
      schema = schema_concentric;
      if (local_selector == selector_centers)
         goto do_concentric_ctrs;
      else if (local_selector == selector_ends)
         goto do_concentric_ends;
   }

   if (sizem1 < 0) fail("Can't identify people in this setup.");

   current_options.who.who[0] = local_selector;

   // If the primary selector is a recursive one (and nothing special is interfering
   // with this; otherwise we just let it fail on the special selector), switch over
   // to the secondary selector.  That is, if the selector is "girl-based triangles",
   // find the girls.  We will finish the job later.
   if (local_selector >= selector_RECURSIVE_START && local_selector < selector_SOME_START &&
       not_using_given_selector == 0 && override_selector == 0) {
      current_options.who.who[0] = selector_to_use.who[1];
   }

   the_setups[0] = *ss;              // designees
   the_setups[1] = *ss;              // non-designees

   for (i=0, bigend_ssmask=0, bigend_llmask=0, livemask[0] = 0, livemask[1] = 0, j=1;
        i<=sizem1;
        i++, j<<=1) {
      bigend_ssmask <<= 1;
      bigend_llmask <<= 1;
      if (ss->people[i].id1) {
         int q = 0;

         // Handle the "finally snag the <anyone>" restrained concept when the call is
         // defined as concentric-over-sequential.  When we get to this point, the concentric
         // decomposition has already occurred, so we are not in the same setup as when the
         // selector was actually evaluated.  The result of that evaluation has been stored
         // in the restrained_selector_decoder words, and we have to look each person up in
         // those words to find out whether they were actually selected.
         //
         // Also, we allow the designators "centers" and "ends" while in a 1x8, which
         // would otherwise not be allowed.  The reason we don't allow it in
         // general is that people would carelessly say "centers kickoff" in a
         // 1x8 when they should really say "each 1x4, centers kickoff".  But we
         // assume that they will not misuse the term here.

         uint32_t t0 = ss->cmd.restrained_selector_decoder[0];
         uint32_t t1 = ss->cmd.restrained_selector_decoder[1];

         if ((t0 | t1) != 0) {
            while (true) {
               if ((t0 | t1) == 0)
                  fail("INTERNAL ERROR: LOST SOMEONE!!!!!");

               if (((ss->people[i].id1 ^ (t1 << 4)) & (XPID_MASK|BIT_PERSON)) == 0) {
                  q = t1 & 1;   // Found it.
                  break;
               }

               t1 >>= 8;
               t1 |= t0 << 24;
               t0 >>= 8;
            }
         }
         else if (ss->kind == s1x8 && current_options.who.who[0] == selector_centers) {
            if (i&2) q = 1;
         }
         else if (ss->kind == s1x8 && (current_options.who.who[0] == selector_ends ||
                                       current_options.who.who[0] == selector_outsides)) {
            if (!(i&2)) q = 1;
         }
         else if (override_selector) {
            if (override_selector & 0x80000000U) {
               // It is a headliner/sideliner mask.
               if (override_selector & 011 & ss->people[i].id1) q = 1;
            }
            else {
               // It is a little-endian mask of the people to select.
               if (override_selector & j) q = 1;
            }
         }
         else if (selectp(ss, i))
            q = 1;

         // Indicator selective_key_ignore is like selective_key_disc_dist, but inverted.
         if (orig_indicator == selective_key_ignore) q ^= 1;

         bigend_ssmask |= q;
         bigend_llmask |= 1;
         the_setups[q].clear_person(i);
         livemask[q^1] |= j;
      }
   }

   if (local_selector >= selector_RECURSIVE_START && local_selector < selector_SOME_START) {
      // The livemasks tell where the real selected people and the real unselected people, respectively, are.
      // But that's where the girls are.  We need to know where the girl-based triangles are.
      // That requires moving some bits between the two masks, to bring in the apex.
      uint32_t bothlivemask = livemask[0] << 16 | livemask[1];
      uint32_t LEdelta = 0;
      uint32_t BEdelta = 0;
      uint32_t apex_people = 0;
      if (local_selector == selector_anyone_base_tgl) {
         // Anyone-based triangle, not interlocked.
         switch (ss->kind) {
         case s_c1phan:
            switch (bothlivemask) {
            case 0xA0A00A0A: LEdelta = 0x0808; break;
            case 0x0A0AA0A0: LEdelta = 0x8080; break;
            case 0x50500505: LEdelta = 0x0404; break;
            case 0x05055050: LEdelta = 0x4040; break;
            }
            break;
         case s_323:
            switch (bothlivemask) {
            case 0x003300CC: case 0x00660099: LEdelta = 0x88; break;
            default:
               fail("Can't find the indicated triangles.");
            }
            break;
         case s_343:
            switch (bothlivemask) {
            case 0x031800C6: LEdelta = 0x84; break;
            case 0x03180063: LEdelta = 0x21; break;
            }
            break;
         case sd2x5:
            if ((bothlivemask & ~0x294) == 0x00630108) {
               LEdelta = 0x108; BEdelta = 0x042;
            }
            else if ((bothlivemask & ~0x252) == 0x018c0021) {
               LEdelta = 0x021; BEdelta = 0x210;
            }
            else if ((bothlivemask & ~0xA5) == 0x03180042) {
               LEdelta = 0x042; BEdelta = 0x108;
            }
            else if ((bothlivemask & ~0x41) == 0x01980022) {
               LEdelta = 0x022; BEdelta = 0x110;
            }
            else if ((bothlivemask & ~0x22) == 0x030C0041) {
               LEdelta = 0x041; BEdelta = 0x208;
            }
            break;
         case s_3223:
            if ((bothlivemask & ~0x14A) == 0x00A50210) {
               LEdelta = 0x210; BEdelta = 0x021;
            }
            else if ((bothlivemask & ~0x231) == 0x00C60108) {
               LEdelta = 0x108; BEdelta = 0x042;
            }
            else if ((bothlivemask & ~0x360) == 0x00870018) {
               LEdelta = 0x018; BEdelta = 0x060;
            }
            else if ((bothlivemask & ~0x01B) == 0x00E40300) {
               LEdelta = 0x300; BEdelta = 0x003;
            }
            else if ((bothlivemask & ~0x063) == 0x03180084) {
               LEdelta = 0x084; BEdelta = 0x084;
            }
            break;
         case s_434:
            if ((bothlivemask & ~0x18C) == 0x00630210) {
               LEdelta = 0x210; BEdelta = 0x021;
            }
            else if ((bothlivemask & ~0x063) == 0x018C0210) {
               LEdelta = 0x210; BEdelta = 0x021;
            }
            break;
         case s_rigger:
            if ((livemask[0] & ~0x33) == 0) {
               LEdelta = 0x88; BEdelta = 0x11;
            }
            break;
         case s_short6:
            if ((livemask[0] & ~055) == 0) {
               LEdelta = 022; BEdelta = 022;
            }
            break;
         case s_bone6:
            if ((livemask[0] & ~033) == 0) {
               LEdelta = 044; BEdelta = 011;
            }
            break;
         case s_bone:
            if ((livemask[0] & ~0x33) == 0) {
               LEdelta = 0x44; BEdelta = 0x22;
            }
            break;
         case s_ntrgl6cw:
            if ((livemask[0] & ~066) == 0)
               LEdelta = 011;
            break;
         case s_ntrgl6ccw:
            if ((livemask[0] & ~033) == 0)
               LEdelta = 044;
            break;
         case s_spindle:
            if ((livemask[0] & ~0x55) == 0)
               LEdelta = 0x88;
            break;
         case s_ntrglcw: case s_nptrglcw:
            if ((livemask[0] & ~0xCC) == 0)
               LEdelta = 0x11;
            break;
         case s_ntrglccw: case s_nptrglccw:
            if ((livemask[0] & ~0x33) == 0)
               LEdelta = 0x88;
            break;
         case s_nxtrglcw:
            if ((livemask[0] & ~0x66) == 0)
               LEdelta = 0x11;
            break;
         case s_nxtrglccw:
            if ((livemask[0] & ~0x33) == 0)
               LEdelta = 0x44;
            break;
         case sdeepbigqtg:
            if (livemask[0] == 0x3030)
               LEdelta = 0x0808;
            else if (livemask[0] == 0xC0C0)
               LEdelta = 0x0404;
            break;
         default:
            fail("Can't find the indicated triangles.");
         }
      }
      else if ((local_selector == selector_anyone_apex_of_tandem_tgl ||
               local_selector == selector_anyone_apex_of_wave_tgl) && ss->kind == s2x4) {

         ss->cmd.cmd_misc3_flags |= CMD_MISC3__SAID_TRIANGLE;
         apex_people = or_all_people(&the_setups[0]) & 011;

         if (apex_people == 010) {
            switch (livemask[0]) {
            case 0x88:
               LEdelta = 0x66;
               tglindicator = tglmap::TGL_GROUP_400;
               break;
            case 0x11:
               LEdelta = 0x66;
               tglindicator = tglmap::TGL_GROUP_400+1;
               break;
            case 0x44:
               LEdelta = 0x99;
               tglindicator = tglmap::TGL_GROUP_400+2;
               break;
            case 0x22:
               LEdelta = 0x99;
               tglindicator = tglmap::TGL_GROUP_400+3;
               break;
            }
         }
         else if (apex_people == 001) {
            switch (livemask[0]) {
            case 0x88: 
               LEdelta = 0x33;
               tglindicator = tglmap::TGL_GROUP_500;
               break;
            case 0x44: 
               LEdelta = 0x33;
               tglindicator = tglmap::TGL_GROUP_500+1;
               break;
            case 0x22:
               LEdelta = 0xCC;
               tglindicator = tglmap::TGL_GROUP_500+2;
               break;
            case 0x11:
               LEdelta = 0xCC;
               tglindicator = tglmap::TGL_GROUP_500+3;
               break;
            }
         }
      }
      else {
         // Anyone-based triangle, interlocked.
         switch (ss->kind) {
         case s_c1phan:
            switch (bothlivemask) {
            case 0x0A0AA0A0: LEdelta = 0x2020; BEdelta = 0x0404; break;
            case 0xA0A00A0A: LEdelta = 0x0202; BEdelta = 0x4040; break;
            case 0x50500505: LEdelta = 0x0101; BEdelta = 0x8080; break;
            case 0x05055050: LEdelta = 0x1010; BEdelta = 0x0808; break;
            default:
               fail("Can't find the indicated triangles.");
            }
            break;
         case sd2x5:
            if ((bothlivemask & ~0x294) == 0x00630108) {
               LEdelta = 0x108; BEdelta = 0x042;
            }
            else if ((bothlivemask & ~0x41) == 0x01980022) {
               LEdelta = 0x022; BEdelta = 0x110;
            }
            else if ((bothlivemask & ~0x22) == 0x030C0041) {
               LEdelta = 0x041; BEdelta = 0x208;
            }
            break;
         case s_3223:
            if ((bothlivemask & ~0x252) == 0x00A50108) {
               LEdelta = 0x108; BEdelta = 0x042;
            }
            else if ((bothlivemask & ~0x129) == 0x00C60210) {
               LEdelta = 0x210; BEdelta = 0x021;
            }
            else if ((bothlivemask & ~0x360) == 0x00870018) {
               LEdelta = 0x018; BEdelta = 0x060;
            }
            else if ((bothlivemask & ~0x01B) == 0x00E40300) {
               LEdelta = 0x300; BEdelta = 0x003;
            }
            else if ((bothlivemask & ~0x063) == 0x03180084) {
               LEdelta = 0x084; BEdelta = 0x084;
            }
            break;
         case s_rigger:
            if ((livemask[0] & ~0x33) == 0) {
               LEdelta = 0x88; BEdelta = 0x11;
            }
            break;
         default:
            fail("Can't find the indicated triangles.");
         }
      }

      if (LEdelta == 0)
         fail("Can't find the indicated triangles.");

      livemask[0] ^= LEdelta;
      livemask[1] ^= LEdelta;
      bigend_ssmask ^= BEdelta;

      // Now go through the setup again, separating it into the new designees and non-designees.
      // The designees (the_setups[0]) are the entire triangle, including the apex,
      // and the non-designees (the_setups[1]) are just the idle people.
      the_setups[0] = *ss;              // designees
      the_setups[1] = *ss;              // non-designees

      uint32_t base_people = 0;

      for (i=0, j=1; i<=sizem1; i++, j<<=1) {
         if (ss->people[i].id1) {
            if (livemask[0] & j)
               the_setups[1].clear_person(i);
            if (livemask[1] & j)
               the_setups[0].clear_person(i);
            if (LEdelta & j)
               base_people |= ss->people[i].id1;
         }
      }

      base_people &= 011;

      if (local_selector == selector_anyone_apex_of_wave_tgl)
         base_people ^= 011;

      if ((local_selector == selector_anyone_apex_of_tandem_tgl ||
           local_selector == selector_anyone_apex_of_wave_tgl) && base_people != apex_people)
         fail("Can't find the indicated triangles.");
   }

   current_options.who = saved_selector;

   // If we are doing the special stuff instead of an actual selector,
   // turn off the given selector to prevent the code below from being led astray.
   if (not_using_given_selector != 0) local_selector = selector_uninitialized;

   if (demand_both_setups_live && (livemask[0] == 0 || livemask[1] == 0))
      fail("Can't do this call in this formation.");

   // If this is "ignored", we invert the selector
   // and change the operation to "disconnected".

   if (orig_indicator == selective_key_ignore) {
      local_selector = selector_list[local_selector].opposite;
      indicator = selective_key_disc_dist;
   }

   if (indicator == selective_key_disc_dist)
      action = normalize_to_2;

   // If the call is a "space-invader", and we are simply doing it under a selector, and
   // the call takes no further selector, and "others" is <=0, that means the user
   // simply said, for example, "boys" and "press ahead" as two seperate actions, rather
   // than using the single call "boys press ahead".  In that case, we simply do
   // whatever "boys press ahead" would have done -- we have the designated (or
   // non-ignored) people do their part in a strict matrix.  We don't do any of the
   // clever stuff ("find the largest contiguous undistorted subsetups") that this
   // procedure generally tries to do.  But if "others" is >0, things are more
   // complicated, and the designees have to interact with the non-designees, so we
   // don't take this shortcut.
   //
   // Similarly, if the call was "girls circulate", we turn on "do your part".  But not
   // if it was "ends circulate".  It needs to be a "who you are" selector, not "where
   // you are".
   //
   if (others <= 0 &&
       (orig_indicator == selective_key_ignore ||
        orig_indicator == selective_key_plain ||
        orig_indicator == selective_key_dyp)) {
      parse_block *foo = cmd1->parseptr;

      // We see through any "N times" concepts.
      while (foo && foo->concept &&
             (foo->concept->kind == concept_n_times || foo->concept->kind == concept_n_times_const)) {
         foo = foo->next;
      }

      // Now watch for matrix calls, or circulate in 2x2/2x4 where selector is who-you-are,
      // with no concepts other than "N times".
      // And the call must not itself require a selector.  We are already acting on the selector.
      // We require a "who-you-are" selector, such as "side boys", rather than a positional selector,
      // such as "ends", so that the people can just do it, without getting hung up on all the
      // complex interactions involved with the "select::hash_lookup" mechanism below.
      if (foo && foo->concept && foo->call && foo->concept->kind == marker_end_of_list) {
         if ((foo->call->the_defn.callflagsf & CFLAGH__REQUIRES_SELECTOR) == 0 &&
             (foo->call->the_defn.schema == schema_matrix || 
              (foo->call == base_calls[base_call_circulate] &&
               (ss->kind == s2x4 || ss->kind == s2x2) &&
               // The "who-you-are" selectors are in two groups, because some have to be in the
               // unsymmetrical "no_resolve" section near the end.
               (local_selector < selector_WHO_YOU_ARE_END ||
                (local_selector >= selector_WHO_YOU_ARE_2_START && local_selector < selector_TGL_START))))) {
            indicator = selective_key_dyp;
            orig_indicator = selective_key_dyp;
            action = normalize_strict_matrix;
            force_matrix_merge = true;
         }
      }
   }

   if (orig_indicator == selective_key_lead_for_a || orig_indicator == selective_key_promenade_and_lead_for_a) {
      // This is "so-and-so lead for a cast a shadow".

      static const int8_t map_prom_1[16] =
      {6, 7, 1, 0, 2, 3, 5, 4, 011, 011, 022, 022, 011, 011, 022, 022};
      static const int8_t map_prom_2[16] =
      {4, 5, 7, 6, 0, 1, 3, 2, 022, 022, 033, 033, 022, 022, 033, 033};
      static const int8_t map_prom_3[16] =
      {0, 1, 3, 2, 4, 5, 7, 6, 000, 000, 011, 011, 000, 000, 011, 011};
      static const int8_t map_prom_4[16] =
      {6, 7, 1, 0, 2, 3, 5, 4, 011, 011, 022, 022, 011, 011, 022, 022};

      static const int8_t map_2fl_1[16] =
      {4, 5, 6, 7, 0, 1, 2, 3, 022, 022, 022, 022, 022, 022, 022, 022};
      static const int8_t map_2fl_2[16] =
      {7, 6, 1, 0, 3, 2, 5, 4, 000, 000, 022, 022, 000, 000, 022, 022};

      static const int8_t map_phan_1[16] =
      {13, 15, 3, 1, 5, 7, 11, 9, 000, 000, 011, 011, 000, 000, 011, 011};
      static const int8_t map_phan_2[16] =
      {9, 11, 15, 13, 1, 3, 7, 5, 011, 011, 022, 022, 011, 011, 022, 022};
      static const int8_t map_phan_3[16] =
      {8, 10, 14, 12, 0, 2, 6, 4, 022, 022, 033, 033, 022, 022, 033, 033};
      static const int8_t map_phan_4[16] =
      {12, 14, 2, 0, 4, 6, 10, 8, 011, 011, 022, 022, 011, 011, 022, 022};

      static const int8_t map_4x4_1[16] =
      {9, 11, 7, 5, 9, 11, 15, 13, 000, 000, 011, 011, 000, 000, 011, 011};
      static const int8_t map_4x4_2[16] =
      {13, 15, 11, 9, 5, 7, 11, 9, 011, 011, 022, 022, 011, 011, 022, 022};
      static const int8_t map_4x4_3[16] =
      {2, 7, 11, 6, 10, 15, 3, 14, 022, 022, 033, 033, 022, 022, 033, 033};
      static const int8_t map_4x4_4[16] =
      {6, 11, 15, 10, 14, 3, 7, 2, 011, 011, 022, 022, 011, 011, 022, 022};

      the_setups[0] = *ss;        // Use this all over again.
      the_setups[0].clear_people();
      the_setups[0].kind = s2x4;

      const int8_t *map_prom;
      uint32_t dirmask, junk;
      big_endian_get_directions32(ss, dirmask, junk);

      if (ss->kind == s2x4 && orig_indicator == selective_key_promenade_and_lead_for_a) {
         if (bigend_ssmask == 0xCC) {
            if (dirmask == 0x0AA0) {
               map_prom = map_2fl_1;
               goto got_map;
            }
            else if (dirmask == 0xA00A) {
               map_prom = map_2fl_2;
               the_setups[0].rotation++;
               goto got_map;
            }
         }
         else if (bigend_ssmask == 0x33) {
            if (dirmask == 0xA00A) {
               map_prom = map_2fl_1;
               goto got_map;
            }
            else if (dirmask == 0x0AA0) {
               map_prom = map_2fl_2;
               the_setups[0].rotation++;
               goto got_map;
            }
         }
      }
      if (ss->kind == s_crosswave || ss->kind == s_thar) {
         if (bigend_ssmask == 0xCC) {
            if (dirmask == 0xAF05) {
               map_prom = map_prom_1;
               goto got_map;
            }
            else if (dirmask == 0x05AF) {
               map_prom = map_prom_2;
               goto got_map;
            }
         }
         else if (bigend_ssmask == 0x33) {
            if (dirmask == 0xAF05) {
               map_prom = map_prom_3;
               the_setups[0].rotation++;
               goto got_map;
            }
            else if (dirmask == 0x05AF) {
               map_prom = map_prom_4;
               the_setups[0].rotation++;
               goto got_map;
            }
         }
      }
      else if (ss->kind == s_c1phan) {
         if (bigend_llmask == 0x5555) {
            if (bigend_ssmask == 0x5050) {
               if (dirmask == 0x33001122U) {
                  map_prom = map_phan_1;
                  the_setups[0].rotation++;
                  goto got_map;
               }
               else if (dirmask == 0x11223300U) {
                  map_prom = map_phan_2;
                  goto got_map;
               }
            }
            else if (bigend_ssmask == 0x0505) {
               if (dirmask == 0x33001122U) {
                  map_prom = map_phan_2;
                  goto got_map;
               }
               else if (dirmask == 0x11223300U) {
                  map_prom = map_phan_1;
                  the_setups[0].rotation++;
                  goto got_map;
               }
            }
         }
         else if (bigend_llmask == 0xAAAA) {
            if (bigend_ssmask == 0x0A0A) {
               if (dirmask == 0x88CC0044U) {
                  map_prom = map_phan_3;
                  goto got_map;
               }
               else if (dirmask == 0x004488CCU) {
                  map_prom = map_phan_4;
                  the_setups[0].rotation++;
                  goto got_map;
               }
            }
            else if (bigend_ssmask == 0xA0A0) {
               if (dirmask == 0x88CC0044U) {
                  map_prom = map_phan_4;
                  the_setups[0].rotation++;
                  goto got_map;
               }
               else if (dirmask == 0x004488CCU) {
                  map_prom = map_phan_3;
                  goto got_map;
               }
            }
         }
      }
      else if (ss->kind == s4x4) {
         if (bigend_llmask == 0x5555) {
            if (bigend_ssmask == 0x0505) {
               if (dirmask == 0x00112233U) {
                  map_prom = map_4x4_1;
                  the_setups[0].rotation++;
                  goto got_map;
               }
               else if (dirmask == 0x22330011U) {
                  map_prom = map_4x4_2;
                  goto got_map;
               }
            }
            else if (bigend_ssmask == 0x5050) {
               if (dirmask == 0x00112233U) {
                  map_prom = map_4x4_2;
                  goto got_map;
               }
               else if (dirmask == 0x22330011U) {
                  map_prom = map_4x4_1;
                  the_setups[0].rotation++;
                  goto got_map;
               }
            }
         }
         else if (bigend_llmask == 0x3333) {
            if (bigend_ssmask == 0x1212) {
               if (dirmask == 0x0304090EU) {
                  map_prom = map_4x4_3;
                  goto got_map;
               }
               else if (dirmask == 0x090E0304U) {
                  map_prom = map_4x4_4;
                  the_setups[0].rotation++;
                  goto got_map;
               }
            }
            else if (bigend_ssmask == 0x2121) {
               if (dirmask == 0x0304090EU) {
                  map_prom = map_4x4_4;
                  the_setups[0].rotation++;
                  goto got_map;
               }
               else if (dirmask == 0x090E0304U) {
                  map_prom = map_4x4_3;
                  goto got_map;
               }
            }
         }
      }

      fail("Can't do this in this setup or with these people designated.");

      got_map:

      for (i=0 ; i<8 ; i++)
         copy_rot(&the_setups[0], i, ss, map_prom[i], map_prom[i+8]);

      update_id_bits(&the_setups[0]);
      move(&the_setups[0], false, result);
      result->result_flags.misc |= RESULTFLAG__IMPRECISE_ROT;
      return;
   }

   // Look for special case of user saying something like "heads bring us together" from squared-set spots.
   // Just do it, as though sequence had started that way.

   if (orig_indicator == selective_key_plain || orig_indicator == selective_key_plain_no_live_subsets) {
      static const expand::thing compress_heads4x4 = {{10, 13, 14, 1, 2, 5, 6, 9}, s2x4, s4x4, 0};
      static const expand::thing compress_sides4x4 = {{6, 9, 10, 13, 14, 1, 2, 5}, s2x4, s4x4, 1};
      static const expand::thing compress_headsalamo = {{7, 0, 1, 2, 3, 4, 5, 6}, s2x4, s_alamo, 0};
      static const expand::thing compress_sidesalamo = {{5, 6, 7, 0, 1, 2, 3, 4}, s2x4, s_alamo, 1};

      struct promenader_thing {
         const expand::thing *compressor;
         const expand::thing *compressor_backout;
         calldef_schema schema1;
         calldef_schema xschema;
         int rotation_forbid;
      };

      promenader_thing const *whattodo = (promenader_thing const *) 0;

      static const promenader_thing thing4x4_0606 = {
         &compress_heads4x4,
         &compress_sides4x4,
         schema_concentric,
         schema_cross_concentric,
         2};

      static const promenader_thing thing4x4_6060 = {
         &compress_sides4x4,
         &compress_heads4x4,
         schema_concentric,
         schema_cross_concentric,
         2};

      static const promenader_thing thingalamocc = {
         &compress_headsalamo,
         &compress_sidesalamo,
         schema_concentric,
         schema_cross_concentric,
         0};

      static const promenader_thing thingalamo33 = {
         &compress_sidesalamo,
         &compress_headsalamo,
         schema_concentric,
         schema_cross_concentric,
         0};

      static const promenader_thing thingspindle = {
         (const expand::thing *) 0,
         (const expand::thing *) 0,
         schema_concentric_6_2,
         schema_cross_concentric_6_2,
         2};

      static const promenader_thing thingqtag22 = {
         (const expand::thing *) 0,
         (const expand::thing *) 0,
         schema_concentric_6_2,
         schema_cross_concentric_6_2,
         1};

      static const promenader_thing thingqtag11 = {
         (const expand::thing *) 0,
         (const expand::thing *) 0,
         schema_concentric,
         schema_cross_concentric,
         2};

      static const promenader_thing thingbone = {
         (const expand::thing *) 0,
         (const expand::thing *) 0,
         schema_concentric,
         schema_cross_concentric,
         1};

      static const promenader_thing thinghrglass = {
         (const expand::thing *) 0,
         (const expand::thing *) 0,
         schema_concentric,
         schema_cross_concentric,
         3};

      static const promenader_thing thing2x4 = {
         (const expand::thing *) 0,
         (const expand::thing *) 0,
         schema_concentric,
         schema_cross_concentric,
         0};

      if (ss->kind == s4x4 && bigend_llmask == 0x6666) {
         if (bigend_ssmask == 0x0606)
            whattodo = &thing4x4_0606;
         else if (bigend_ssmask == 0x6060)
            whattodo = &thing4x4_6060;
      }
      else if (bigend_llmask == 0xFF) {
         if (ss->kind == s_alamo && bigend_ssmask == 0xCC)
            whattodo = &thingalamocc;
         else if (ss->kind == s_alamo && bigend_ssmask == 0x33)
            whattodo = &thingalamo33;
         else if (ss->kind == s_spindle && bigend_ssmask == 0x11)
            whattodo = &thingspindle;
         else if (ss->kind == s_qtag && bigend_ssmask == 0x22)
            whattodo = &thingqtag22;
         else if (ss->kind == s_qtag && bigend_ssmask == 0xCC)
            whattodo = &thingqtag11;
         else if (ss->kind == s_bone && bigend_ssmask == 0xCC)
            whattodo = &thingbone;
         else if (ss->kind == s_hrglass && bigend_ssmask == 0xCC)
            whattodo = &thinghrglass;
         else if (ss->kind == s_dhrglass && bigend_ssmask == 0xCC)
            whattodo = &thinghrglass;
         else if (ss->kind == s2x4 && bigend_ssmask == 0x99)
            whattodo = &thing2x4;
         else if (ss->kind == s_rigger && bigend_ssmask == 0x33)
            whattodo = &thing2x4;
         else if (ss->kind == s1x8 && bigend_ssmask == 0xCC)
            whattodo = &thing2x4;
         else if (ss->kind == s_crosswave && bigend_ssmask == 0xCC)
            whattodo = &thing2x4;
      }

      final_and_herit_flags local_flags;
      local_flags.clear_all_herit_and_final_bits();
      parse_block *parseptrcopy = process_final_concepts(ss->cmd.parseptr, false, &local_flags, false, false);

      if (parseptrcopy->call) {
         calldefn *callspec = &parseptrcopy->call->the_defn;

         if (callspec->callflags1 & CFLAG1_SEQUENCE_STARTER) {
            if (whattodo && whattodo->compressor) {
               expand::compress_setup(*whattodo->compressor, ss);
               move(ss, false, result);
               return;
            }
         }
         else if (callspec->schema == schema_concentric_specialpromenade) {
            if (whattodo) {
               setup local_setup = *ss;
               local_setup.cmd.parseptr = parseptrcopy;
               local_setup.cmd.cmd_final_flags = local_flags;

               if (whattodo->compressor_backout) expand::compress_setup(*whattodo->compressor_backout, &local_setup);
               call_conc_option_state saved_options = current_options;
               current_options = parseptrcopy->options;

               uint32_t override =
                  (callspec->stuff.conc.outerdef.call_id == base_call_plainprom ||
                   callspec->stuff.conc.outerdef.call_id == base_call_plainpromeighths) ?
                  DFM1_CONC_FORCE_COLUMNS : 0;

               really_inner_move(&local_setup, false, callspec, whattodo->schema1,
                                 callspec->callflags1, callspec->callflagsf,
                                 override, false, 0, false, result);
               current_options = saved_options;
               return;
            }
         }
         else if (callspec->schema == schema_cross_concentric_specialpromenade) {
            if (whattodo) {
               if (whattodo->compressor_backout) expand::compress_setup(*whattodo->compressor_backout, ss);
               call_conc_option_state saved_options = current_options;
               current_options = parseptrcopy->options;
               // Be sure people don't come in to the middle while others are in the way.
               if ((1 << (current_options.number_fields & 1)) & whattodo->rotation_forbid)
                  fail("These people can't come into the middle gracefully.");

               really_inner_move(ss, false, callspec, whattodo->xschema, callspec->callflags1, callspec->callflagsf,
                                 DFM1_CONC_FORCE_OTHERWAY, false, 0, false, result);
               current_options = saved_options;
               return;
            }
         }
      }
   }

   switch (ss->kind) {
   case s3x4:
      if (bigend_llmask == 04747) {
         ctr_end_masks_to_use = &masks_for_3x4;
         mask = bigend_llmask;
      }
      break;
   case s3dmd:
      if (bigend_llmask == 07171) {
         ctr_end_masks_to_use = &masks_for_3dmd_ctr2;
         mask = bigend_llmask;
      }
      else if (bigend_llmask == 05353) {
         ctr_end_masks_to_use = &masks_for_3dmd_ctr4;
         mask = bigend_llmask;
      }
      break;
   case sbigh:
      if (bigend_llmask == 04747) {
         ctr_end_masks_to_use = &masks_for_bigh_ctr4;
         mask = bigend_llmask;
      }
      break;
   case s4x4:
      if (bigend_llmask == 0x9999) {
         ctr_end_masks_to_use = &masks_for_4x4;
         mask = bigend_llmask;
      }
      break;
   default:
      ctr_end_masks_to_use = &setup_attrs[ss->kind].setup_conc_masks;
   }

   // See if the user requested "centers" (or the equivalent people under some
   // other designation), and just do it with the concentric_move stuff if so.
   // The concentric_move stuff is much more sophisticated about a lot of things
   // than what we would otherwise do.

   if (force_matrix_merge) {
   }
   else if (orig_indicator == selective_key_plain_from_id_bits) {
      // With this indicator, we don't require an exact match.
      if (sizem1 == 3)
         schema = schema_single_concentric;
      else
         schema = schema_concentric;

      if (ctr_end_masks_to_use != &dead_masks &&
          bigend_ssmask != 0 &&
          setup_attrs[ss->kind].setup_conc_masks.mask_normal != 0) {
         if ((bigend_ssmask & ~setup_attrs[ss->kind].setup_conc_masks.mask_normal) == 0)
            goto do_concentric_ctrs;
         else if ((bigend_ssmask & ~(mask-setup_attrs[ss->kind].setup_conc_masks.mask_normal)) == 0)
            goto do_concentric_ends;
      }

      if (ctr_end_masks_to_use != &dead_masks &&
          bigend_ssmask != 0 &&
          setup_attrs[ss->kind].setup_conc_masks.mask_ctr_dmd != 0) {
         if ((bigend_ssmask & ~setup_attrs[ss->kind].setup_conc_masks.mask_ctr_dmd) == 0) {
            schema = schema_concentric_diamonds;
            goto do_concentric_ctrs;
         }
         else if ((bigend_ssmask & ~(mask-setup_attrs[ss->kind].setup_conc_masks.mask_ctr_dmd)) == 0) {
            schema = schema_concentric_diamonds;
            goto do_concentric_ends;
         }
      }
   }
   else if (orig_indicator == selective_key_plain ||
            orig_indicator == selective_key_plain_no_live_subsets ||
            orig_indicator == selective_key_ignore ||
            orig_indicator == selective_key_work_concept ||
            orig_indicator == selective_key_snag_anyone) {
      if (sizem1 == 3) {
         schema = schema_single_concentric;
      }
      else {
         if (ss->kind == s3x4) {             /* **** BUG  what a crock -- fix this right. */
            if (bigend_llmask == 03333) {         // Qtag occupation.
               schema = schema_concentric_6_2;
               if (local_selector == selector_center6)
                  goto do_concentric_ctrs;
               if (local_selector == selector_outer2 || local_selector == selector_veryends)
                  goto do_concentric_ends;
            }
            else if (bigend_llmask == 04747) {    // "H" occupation.
               schema = schema_concentric_2_6;
               if (local_selector == selector_center2 ||
                   local_selector == selector_verycenters)
                  goto do_concentric_ctrs;
               if (local_selector == selector_outer6)
                  goto do_concentric_ends;
            }
            else if (orig_indicator == selective_key_plain &&
                     !cmd2 &&
                     local_selector != selector_thosefacing &&
                     (bigend_ssmask == 02121 || bigend_ssmask == 01111) &&
                     bigend_ssmask == (bigend_llmask & 03131)) {
               // Look for center Z.  See comment below for 4x5.
               schema = schema_select_ctr4;
               action = normalize_to_4;
               goto back_here;
            }
         }
         else if (ss->kind == sd3x4) {
            if (bigend_llmask == 03535) {    // Spindle occupation.
               schema = schema_concentric_6_2;
               if (local_selector == selector_center6)
                  goto do_concentric_ctrs;
               if (local_selector == selector_outer2 || local_selector == selector_veryends)
                  goto do_concentric_ends;
            }
            else if (orig_indicator == selective_key_plain &&
                     !cmd2 &&
                     local_selector != selector_thosefacing &&

                     (bigend_ssmask == 0x618 || bigend_ssmask == 0x30C) &&
                     bigend_ssmask == (bigend_llmask & 0x71C)) {
               // Look for center Z.  See comment below for 4x5.
               schema = schema_select_ctr4;
               action = normalize_to_4;
               goto back_here;
            }
         }
         else if (ss->kind == s4x5 &&
                  orig_indicator == selective_key_plain &&
                  !cmd2 &&
                  local_selector != selector_thosefacing &&
                  (bigend_ssmask == 0x0300C || bigend_ssmask == 0x01806) &&
                  bigend_ssmask == (bigend_llmask & 0x0380E)) {
            // Look for center Z.  But if the selected people are "those facing", we can't just
            // do the call in a 2x3, because there are missing people.  In that case, just ignore this,
            // and the situation will be taken care of by the normal "back_here" code, eventually leading
            // to the call being done in 2 1x2 setups.
            schema = schema_select_ctr4;
            action = normalize_to_4;
            goto back_here;
         }
         else if (ss->kind == swqtag) {
            // Check for center 6 in "hollow" occupation.
            // Turn it into a "mini O", which we know how to handle.
            if (bigend_llmask == 0x3DE &&
                bigend_ssmask == 0x35A &&
                orig_indicator == selective_key_plain &&
                local_selector == selector_center6) {
                  action = normalize_before_isolated_call;
                  indicator = selective_key_mini_but_o;
                  arg2 = LOOKUP_MINI_O;
                  goto back_here;
            }
         }
         else if (ss->kind == sd2x5) {
            if (local_selector < selector_RECURSIVE_START && (bigend_llmask == 0x37B || bigend_llmask == 0x3DE)) {
               schema = schema_concentric;
               if (bigend_ssmask == 0x063 || bigend_ssmask == 0x0C6)
                  goto do_concentric_ctrs;
               else if (bigend_ssmask == 0x318)
                  goto do_concentric_ends;
            }
         }
         else if (ss->kind == s2x5) {
            if (bigend_llmask == 0x2F7 || bigend_llmask == 0x3BD) {
               schema = schema_concentric;
               if (bigend_ssmask == 0x18C || bigend_ssmask == 0x0C6)
                  goto do_concentric_ctrs;
               else if (bigend_ssmask == 0x231)
                  goto do_concentric_ends;
            }
         }
         else if (ss->kind != s2x7 && ss->kind != sd2x7) {    // Default action, but not if 2x7 or d2x7.
            schema = schema_concentric_6_2;
            if (local_selector == selector_center6)
               goto do_concentric_ctrs;
            if (local_selector == selector_outer2 || local_selector == selector_veryends)
               goto do_concentric_ends;
            schema = schema_concentric_2_6;
            if (local_selector == selector_center2 || local_selector == selector_verycenters)
               goto do_concentric_ctrs;
            if (local_selector == selector_outer6)
               goto do_concentric_ends;
            if (ss->kind == s3x1dmd) {
               schema = schema_concentric_diamond_line;

               switch (local_selector) {
               case selector_ctr_1x4:
                  goto do_concentric_ctrs;
               case selector_center_line:
                  cmd1->cmd_misc_flags |= CMD_MISC__VERIFY_LINES;
                  goto do_concentric_ctrs;
               case selector_center_wave:
                  cmd1->cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;
                  goto do_concentric_ctrs;
               case selector_center_col:
                  cmd1->cmd_misc_flags |= CMD_MISC__VERIFY_COLS;
                  goto do_concentric_ctrs;
               }

               schema = schema_concentric_6_2_line;
               switch (local_selector) {
               case selector_ctr_1x6:
                  goto do_concentric_ctrs;
               case selector_center_line_of_6:
                  cmd1->cmd_misc_flags |= CMD_MISC__VERIFY_LINES;
                  goto do_concentric_ctrs;
               case selector_center_wave_of_6:
                  cmd1->cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;
                  goto do_concentric_ctrs;
               case selector_center_col_of_6:
                  cmd1->cmd_misc_flags |= CMD_MISC__VERIFY_COLS;
                  goto do_concentric_ctrs;
               }
            }
         }

         schema = schema_concentric;
      }

      if (local_selector == selector_centers) goto do_concentric_ctrs;
      if (local_selector == selector_ends) goto do_concentric_ends;

      // If the designator selected the centers and ends but was something other than
      // "centers" or "ends", don't raise the warning.  That is, if the caller says
      // "girls step and fold", the dancers don't need to consider just what "centers"
      // would have meant.
      enable_3x1_warn = false;

      if (orig_indicator == selective_key_plain) {
         // These encodings are an historical artifact from triangle_move;
         // They were encoded into the triangle concepts in sdctables.
         switch (local_selector) {
         case selector_inside_tgl:
            // Plain inside and outside triangles go directly to the concentric mechanism, which is very powerful.
            if (ss->kind == sd2x7 || ss->kind == sbigdmd) {   // Except these.  They have "inside triangles".
               tglindicator = tglmap::TGL_TYPE_INSIDE;
               goto back_here;
            }

            schema = schema_concentric_6_2;
            if (ss->kind == s_qtag)
               schema = schema_concentric_6_2_tgl;
            else if (ss->kind == s_hrglass)
               schema = schema_vertical_6;
            goto do_concentric_ctrs;
         case selector_outside_tgl:
            schema = schema_concentric_2_6;
            goto do_concentric_ends;
         case selector_inside_intlk_tgl:
         case selector_intlk_inside_tgl:
            tglindicator = tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_INSIDE;
            goto back_here;
         case selector_outside_intlk_tgl:
         case selector_intlk_outside_tgl:
            tglindicator = tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_OUTSIDE;
            goto back_here;
         case selector_inpoint_tgl:
            tglindicator = tglmap::TGL_TYPE_INPOINT;
            goto back_here;
         case selector_inpoint_intlk_tgl:
         case selector_intlk_inpoint_tgl:
            tglindicator = tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_INPOINT;
            goto back_here;
         case selector_magic_inpoint_tgl:
            tglindicator = tglmap::TGL_CLASS_MAGIC | tglmap::TGL_TYPE_INPOINT;
            goto back_here;
         case selector_magic_intlk_inpoint_tgl:
         case selector_intlk_magic_inpoint_tgl:
            tglindicator = tglmap::TGL_CLASS_MAGIC | tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_INPOINT;
            goto back_here;
         case selector_outpoint_tgl:
            tglindicator = tglmap::TGL_TYPE_OUTPOINT;
            goto back_here;
         case selector_outpoint_intlk_tgl:
         case selector_intlk_outpoint_tgl:
            tglindicator = tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_OUTPOINT;
            goto back_here;
         case selector_magic_outpoint_tgl:
            tglindicator = tglmap::TGL_CLASS_MAGIC | tglmap::TGL_TYPE_OUTPOINT;
            goto back_here;
         case selector_magic_intlk_outpoint_tgl:
         case selector_intlk_magic_outpoint_tgl:
            tglindicator = tglmap::TGL_CLASS_MAGIC | tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_OUTPOINT;
            goto back_here;
         case selector_beaupoint_tgl:
            tglindicator = tglmap::TGL_TYPE_BEAUPOINT;
            goto back_here;
         case selector_beaupoint_intlk_tgl:
         case selector_intlk_beaupoint_tgl:
            tglindicator = tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_BEAUPOINT;
            goto back_here;
         case selector_magic_beaupoint_tgl:
            tglindicator = tglmap::TGL_CLASS_MAGIC | tglmap::TGL_TYPE_BEAUPOINT;
            goto back_here;
         case selector_magic_intlk_beaupoint_tgl:
         case selector_intlk_magic_beaupoint_tgl:
            tglindicator = tglmap::TGL_CLASS_MAGIC | tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_BEAUPOINT;
            goto back_here;
         case selector_bellepoint_tgl:
            tglindicator = tglmap::TGL_TYPE_BELLEPOINT;
            goto back_here;
         case selector_bellepoint_intlk_tgl:
         case selector_intlk_bellepoint_tgl:
            tglindicator = tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_BELLEPOINT;
            goto back_here;
         case selector_magic_bellepoint_tgl:
            tglindicator = tglmap::TGL_CLASS_MAGIC | tglmap::TGL_TYPE_BELLEPOINT;
            goto back_here;
         case selector_magic_intlk_bellepoint_tgl:
         case selector_intlk_magic_bellepoint_tgl:
            tglindicator = tglmap::TGL_CLASS_MAGIC | tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_BELLEPOINT;
            goto back_here;
         case selector_wave_base_tgl:
            tglindicator = tglmap::TGL_TYPE_WAVEBASE;
            goto wv_tand;
         case selector_wave_base_intlk_tgl:
         case selector_intlk_wave_base_tgl:
            tglindicator = tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_WAVEBASE;
            goto wv_tand;
         case selector_tand_base_tgl:
            tglindicator = tglmap::TGL_TYPE_TANDBASE;
            goto wv_tand;
         case selector_tand_base_intlk_tgl:
         case selector_intlk_tand_base_tgl:
            tglindicator = tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_TANDBASE;
         // FALL THROUGH
         wv_tand:
         // FELL THROUGH!
            if (ss->kind == s_galaxy) {
               // When these are done from a galaxy, use the concentric mechanism,
               // albeit in a not-very-concentric way.
               j = ss->people[1].id1 | ss->people[3].id1 | ss->people[5].id1 | ss->people[7].id1;

               if ((j & 011) == 011)
                  goto back_here;   // Will raise an error.
               else if ((tglindicator ^ j) & 1)
                  schema = (tglindicator & tglmap::TGL_CLASS_INTERLOCKED) ? schema_intlk_lateral_6 : schema_lateral_6;
               else
                  schema = (tglindicator & tglmap::TGL_CLASS_INTERLOCKED) ? schema_intlk_vertical_6 : schema_vertical_6;

               // For galaxies, the schema is now in terms of the absolute orientation.
               // We know that the original setup rotation was canonicalized.
               goto do_concentric_ctrs;
            }
            else if (ss->kind == s_bone && !(tglindicator & tglmap::TGL_CLASS_INTERLOCKED)) {
               schema = schema_concentric_2_6;
               goto do_concentric_ends;
            }
            else if (ss->kind == s_rigger && !(tglindicator & tglmap::TGL_CLASS_INTERLOCKED)) {
               schema = schema_concentric_6_2;
               goto do_concentric_ctrs;
            }
            else if (ss->kind == s_hrglass) {
               // The same for an hourglass.
               schema = (tglindicator & tglmap::TGL_CLASS_INTERLOCKED) ? schema_intlk_vertical_6 : schema_vertical_6;
               goto do_concentric_ctrs;
            }
            else if (ss->kind == s_rigger && !(tglindicator & tglmap::TGL_CLASS_INTERLOCKED)) {
               schema = schema_concentric_6_2;
               goto do_concentric_ctrs;
            }
            else if (ss->kind == s_ntrglcw && !(tglindicator & tglmap::TGL_CLASS_INTERLOCKED)) {
               schema = schema_concentric_2_6;
               goto do_concentric_ends;
            }
            else if (ss->kind == s_ntrglccw && !(tglindicator & tglmap::TGL_CLASS_INTERLOCKED)) {
               schema = schema_concentric_2_6;
               goto do_concentric_ends;
            }
            else if (ss->kind == s_nptrglcw && !(tglindicator & tglmap::TGL_CLASS_INTERLOCKED)) {
               schema = schema_concentric_2_6;
               goto do_concentric_ends;
            }
            else if (ss->kind == s_nptrglccw && !(tglindicator & tglmap::TGL_CLASS_INTERLOCKED)) {
               schema = schema_concentric_2_6;
               goto do_concentric_ends;
            }
            else if (ss->kind == s_nxtrglcw && !(tglindicator & tglmap::TGL_CLASS_INTERLOCKED)) {
               schema = schema_concentric_6_2;
               goto do_concentric_ctrs;
            }
            else if (ss->kind == s_nxtrglccw && !(tglindicator & tglmap::TGL_CLASS_INTERLOCKED)) {
               schema = schema_concentric_6_2;
               goto do_concentric_ctrs;
            }
            else {
               goto back_here;
            }

            break;
         case selector_anyone_base_tgl:
            if (ss->kind == s_bone && (livemask[1] & 0x77) == 0) {
               schema = schema_concentric_2_6;
               goto do_concentric_ends;
            }
            else if (ss->kind == s_rigger && (livemask[1] & 0xBB) == 0) {
               schema = schema_concentric_6_2;
               goto do_concentric_ctrs;
            }
            else if (ss->kind == s_spindle && (livemask[1] & 0xDD) == 0) {
               schema = schema_concentric_2_6;
               goto do_concentric_ends;
            }
            else if ((ss->kind == s_ntrglcw || ss->kind == s_nptrglcw) && (livemask[1] & 0xDD) == 0) {
               schema = schema_concentric_2_6;
               goto do_concentric_ends;
            }
            else if ((ss->kind == s_ntrglccw || ss->kind == s_nptrglccw) && (livemask[1] & 0xBB) == 0) {
               schema = schema_concentric_2_6;
               goto do_concentric_ends;
            }
            else if ((ss->kind == s_nxtrglcw || ss->kind == s_nxtrglccw) && (livemask[1] & 0x77) == 0) {
               schema = schema_concentric_6_2;
               goto do_concentric_ctrs;
            }

            tglindicator = tglmap::TGL_TYPE_ANYBASE;
            goto back_here;
         case selector_anyone_base_intlk_tgl:
         case selector_intlk_anyone_base_tgl:
            tglindicator = tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_ANYBASE;
            goto back_here;
         }
      }

      // This stuff is needed, with the livemask test, for rf01t and rd01t.

      if (others <= 0 && livemask[1] != 0) {
         switch (local_selector) {
         case selector_center2:
         case selector_verycenters:
            schema = schema_select_ctr2;
            action = normalize_to_2;
            goto back_here;
         case selector_center4:
         case selector_ctr_1x4:
         case selector_center_wave:
         case selector_center_line:
         case selector_center_col:
         case selector_center_box:
            schema = schema_select_ctr4;
            action = normalize_to_4;
            goto back_here;
         case selector_center6:
         case selector_ctr_1x6:
         case selector_center_wave_of_6:
         case selector_center_line_of_6:
         case selector_center_col_of_6:
            schema = schema_select_ctr6;
            action = normalize_to_6;
            goto back_here;
         }
      }

      if (ctr_end_masks_to_use->mask_normal) {
         if (bigend_ssmask == ctr_end_masks_to_use->mask_normal) goto do_concentric_ctrs;
         else if (bigend_ssmask == mask - ctr_end_masks_to_use->mask_normal) goto do_concentric_ends;
      }

      if (ctr_end_masks_to_use->mask_6_2) {
         schema = schema_concentric_6_2;
         if (bigend_ssmask == ctr_end_masks_to_use->mask_6_2) {
            // Don't do this if qtag and are being told to ignore the outer 2.
            // It would set the other 6 to a 2x3, from which a triangle circulate would be impossible.
            if (ss->kind == s_qtag && orig_indicator == selective_key_ignore)
               special_tgl_ignore = true;
            else
               goto do_concentric_ctrs;
         }
         else if (bigend_ssmask == mask - ctr_end_masks_to_use->mask_6_2)
            goto do_concentric_ends;
      }

      // We don't do this if the selector is "outer 1x3's", because that
      // designates the 1x3's separately, not as all 6 people.
      if (ctr_end_masks_to_use->mask_2_6 && local_selector != selector_outer1x3s) {
         schema = schema_concentric_2_6;
         if (bigend_ssmask == ctr_end_masks_to_use->mask_2_6) goto do_concentric_ctrs;
         else if (bigend_ssmask == mask - ctr_end_masks_to_use->mask_2_6) goto do_concentric_ends;
      }

      if (ctr_end_masks_to_use->mask_ctr_dmd) {
         if (bigend_ssmask == ctr_end_masks_to_use->mask_ctr_dmd) {
            schema = schema_concentric_diamonds;
            goto do_concentric_ctrs;
         }
      }

      if (ss->kind == s3x1dmd) {
         schema = schema_concentric_6_2_line;
         if (bigend_ssmask == 0xEE) goto do_concentric_ctrs;
         else if (bigend_ssmask == 0x11) goto do_concentric_ends;
      }
      else if (ss->kind == s_ptpd) {
         schema = schema_checkpoint_spots;
         if (bigend_ssmask == 0x55) goto do_concentric_ends;
      }
      else if (ss->kind == s_spindle) {
         schema = schema_checkpoint_spots;
         if (bigend_ssmask == 0xAA) goto do_concentric_ends;
      }

      schema = schema_lateral_6;
      if (ss->kind == s_galaxy && bigend_ssmask == 0xDD) goto do_concentric_ctrs;
      schema = schema_vertical_6;
      if (ss->kind == s_galaxy && bigend_ssmask == 0x77) goto do_concentric_ctrs;
   }
   else if (orig_indicator == selective_key_disc_dist) {
      uint32_t mask = ~(~0 << (sizem1+1));

      if (setup_attrs[ss->kind].setup_conc_masks.mask_ctr_dmd) {
         if (bigend_ssmask == mask-setup_attrs[ss->kind].setup_conc_masks.mask_ctr_dmd) {
            schema = schema_concentric_diamonds;
            goto do_concentric_ends;
         }
      }
      else if (ss->kind == s_galaxy && setup_attrs[ss->kind].setup_conc_masks.mask_normal) {
         if (bigend_ssmask == mask-setup_attrs[ss->kind].setup_conc_masks.mask_normal) {
            schema = schema_concentric;
            goto do_concentric_ends;
         }
      }
   }
   else if (orig_indicator == selective_key_dyp_for_mystic) {
      action = normalize_to_4;
   }

 back_here:

   if (doing_special_promenade_thing)
      fail("Can't do this with these people designated.");

   // Check for special case of "<anyone> work tandem", and fix the normalization action if so.
   if (orig_indicator == selective_key_work_concept && cmd1->parseptr && cmd1->parseptr->concept &&
       cmd1->parseptr->concept->kind == concept_tandem)
      action = normalize_before_isolated_callMATRIXMATRIXMATRIX;

   // Don't normalize if doing triangle stuff.
   if (!tglindicator && !special_tgl_ignore)
      normalize_setup(&the_setups[0], action, qtag_compress);
   if (others > 0)
      normalize_setup(&the_setups[1], action, qtag_compress);

   saved_warnings = configuration::save_warnings();

   // It will be helpful to have masks of where the live people are.
   livemask[0] = little_endian_live_mask(&the_setups[0]);
   livemask[1] = little_endian_live_mask(&the_setups[1]);

   // If one of the calls is something like "girls zoom" (indicated with the "one_side_lateral"
   // flag), from a normal starting DPT, we treat it as if we had said
   // "girls do your part zoom".  This effectively fills in the rest of the setup
   // so that they know how to do the call.  Similarly for things like "zing"
   // and "peel off".  Also, "spread" is marked this way, so we can say
   // "boys spread" from a BBGG wave.  And "counter rotate" too, but for a different reason.
   // ("Counter rotate" doesn't want to get mired down in all the "lilsetup" nonsense.
   // "Counter rotate" knows how to do the call all by itself.)
   // In any case, if either call is one of these, change to DYP.

   // So we do a prescan, iterating 1 or 2 times, depending on whether the "other" people do a call.
   for (setupcount=0; setupcount <= others ; setupcount++) {
      setup_kind kk = the_setups[setupcount].kind;
      setup_command *cmdp = (setupcount == 1) ? cmd2 : cmd1;
      uint32_t thislivemask = livemask[setupcount];

      if (indicator == selective_key_plain) {
         const call_with_name *callspec = cmdp->callspec;

         if (!callspec &&
             cmdp->parseptr &&
             cmdp->parseptr->concept &&
             cmdp->parseptr->concept->kind <= marker_end_of_list)
            callspec = cmdp->parseptr->call;

         if (callspec) {
            // If this has a "one_side_lateral" flag with a suitable setup,
            // change to "Do Your Part".  But not if it's a counter rotate --
            // those will be taken care of elsewhere, and may or may not involve
            // turning into a DYP operation.

            if ((callspec->the_defn.callflagsf & CFLAG2_CAN_BE_ONE_SIDE_LATERAL) &&
                callspec->the_defn.schema != schema_counter_rotate &&
                ((kk == s2x4 && (thislivemask == 0xCC || thislivemask == 0x33)) ||
                 (kk == s1x6 && (thislivemask == 033)) ||
                 (kk == s_spindle12 && ((thislivemask & 0xE7) == 0)) ||
                 (kk == s_trngl && two_couple_calling && (thislivemask == 0x06)) ||
                 (kk == s_trngl4 && two_couple_calling && (thislivemask == 0x0C)) ||
                 (kk == s2x2 && two_couple_calling &&
                  (thislivemask == 0x3 || thislivemask == 0x6 || thislivemask == 0xC || thislivemask == 0x9)) ||
                 (kk == s2x6 && (thislivemask == 00303 || thislivemask == 06060)))) {
               indicator = selective_key_dyp;
               special_unsym_dyp = true;
            }
         }
      }
   }

   // Iterate 1 or 2 times, depending on whether the "other" people do a call.
   for (setupcount=0; ; setupcount++) {

      // Not clear that this is really right.
      uint32_t svd_number_fields = current_options.number_fields;
      int svd_num_numbers = current_options.howmanynumbers;
      uint32_t thislivemask = livemask[setupcount];
      uint32_t otherlivemask = livemask[setupcount^1];
      setup *this_one = &the_setups[setupcount];
      setup *this_result = &the_results[setupcount];
      setup_kind kk = this_one->kind;
      setup_command *cmdp = (setupcount == 1) ? cmd2 : cmd1;

      process_number_insertion((setupcount == 1) ? modsb1 : modsa1);
      this_one->cmd = ss->cmd;
      this_one->cmd.parseptr = cmdp->parseptr;
      this_one->cmd.callspec = cmdp->callspec;
      this_one->cmd.cmd_final_flags = cmdp->cmd_final_flags;

      this_one->cmd.cmd_misc_flags |= CMD_MISC__PHANTOMS;

      if (indicator == selective_key_snag_anyone) {
         // Snag the <anyone>.
         if (setupcount == 0) {
            if (!this_one->cmd.cmd_fraction.is_null())
               fail("Can't do fractional \"snag\".");
            this_one->cmd.cmd_fraction.set_to_firsthalf();
         }

         move(this_one, false, this_result);
         // Is this the right thing???????????????
         repair_fudgy_2x3_2x6(this_result);
         remove_fudgy_2x3_2x6(this_result);
      }
      else if (indicator >= selective_key_plain &&
               indicator != selective_key_work_concept &&
               indicator != selective_key_work_no_concentric) {
         int lilcount;
         int numsetups;
         uint32_t key;
         uint32_t frot, vrot;
         setup lilsetup[4], lilresult[4];
         int map_scanner;
         bool feet_warning = false;
         bool doing_iden = false;

         // Check for special cases of no one or everyone.
         // For the "everyone" test, we include cases of "ignore" if the setup
         // is larger than 8 people.
         // The general reasoning is this:  Under normal circumstances "ignore"
         // means "disconnected with the other people", and "disconnected" means
         // "close the gaps".  So, under normal circumstances, we do not want to
         // pick out "everyone" here and just have them do the call.  But, if there
         // are more than 8 people, there are already gaps, and we can't close them.
         // So we interpret it as "other people just do the call where you stand"
         // in that case, which is what this code does.
         // The specific reason for this is to make
         // "ignore <anyone>, staggered waves of 3 <call>" work.

         if (indicator < selective_key_disc_dist ||
             orig_indicator == selective_key_ignore ||
             orig_indicator == selective_key_plain_from_id_bits) {
            if (thislivemask == 0) {
               // No one.
               this_result->kind = nothing;
               clear_result_flags(this_result);
               goto done_with_this_one;
            }
            else if (thislivemask == (uint32_t) ((1U << (attr::klimit(kk)+1)) - 1) ||
                     (otherlivemask == 0 && kk != s4x5) ||
                     orig_indicator == selective_key_plain_from_id_bits ||
                     (orig_indicator == selective_key_ignore && kk != s_c1phan && attr::klimit(kk) > 7) ||
                     ((schema == schema_select_ctr2 ||
                       schema == schema_select_ctr4 ||
                       schema == schema_select_ctr6) && others <= 0)) {
               // Everyone.
               update_id_bits(this_one);
               this_one->cmd.cmd_misc_flags &= ~CMD_MISC__VERIFY_MASK;
               switch (local_selector) {
               case selector_center_wave: case selector_center_wave_of_6:
                  this_one->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;
                  break;
               case selector_center_line: case selector_center_line_of_6:
                  this_one->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_LINES;
                  break;
               case selector_center_col: case selector_center_col_of_6:
                  this_one->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_COLS;
                  break;
               }

               // If we did something just above, change a wingedstar to a 1x6.
               if ((this_one->cmd.cmd_misc_flags & CMD_MISC__VERIFY_MASK) &&
                   this_one->kind == s_wingedstar &&
                   (thislivemask & 0x88) == 0) {
                  static const expand::thing foo = {{0, 1, 2, 4, 5, 6}, s1x6, s_wingedstar, 0};
                  expand::compress_setup(foo, this_one);
               }

               impose_assumption_and_move(this_one, this_result);
               // Is this the right thing???????????????
               repair_fudgy_2x3_2x6(this_result);
               remove_fudgy_2x3_2x6(this_result);
               goto done_with_this_one;
            }
         }

         if (setupcount == 0 && tglindicator) {
            const tglmap::tglmapkey *map_key_table = (const tglmap::tglmapkey *) 0;
            int rotfix = 0;

            if (kk == sbigdmd) {
               if (thislivemask == 03434)
                  map_key_table = tglmap::bdtglmap1;
               else if (thislivemask == 01616)
                  map_key_table = tglmap::bdtglmap2;
            }
            else if (tglindicator == tglmap::TGL_TYPE_INSIDE && kk == sd2x7) {
               if (thislivemask == 0x1C38U)
                  map_key_table = tglmap::d7tglmap1;
               else if (thislivemask == 0x070EU)
                  map_key_table = tglmap::d7tglmap2;
            }
            else if (tglindicator == (tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_INSIDE) && kk == s_rigger) {
               map_key_table = tglmap::rgtglmap1;
            }
            else if (tglindicator == (tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_INSIDE) && kk == s_ptpd) {
               map_key_table = tglmap::rgtglmap2;
            }
            else if (tglindicator == (tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_INSIDE) && kk == s_qtag) {
               map_key_table = tglmap::rgtglmap3;
            }
            else if (tglindicator == (tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_OUTSIDE) && kk == s_rigger) {
               map_key_table = tglmap::ritglmap1;
            }
            else if (tglindicator == (tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_OUTSIDE) && kk == s_ptpd) {
               map_key_table = tglmap::ritglmap2;
            }
            else if (tglindicator == (tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_OUTSIDE) && kk == s_qtag) {
               map_key_table = tglmap::ritglmap3;
            }
            else if (kk == s_qtag &&
                     ((tglindicator & tglmap::TGL_IGNORE_MAG_INTLK_LOWBIT) == tglmap::TGL_TYPE_OUTPOINT ||
                      (tglindicator & tglmap::TGL_IGNORE_MAG_INTLK_LOWBIT) == tglmap::TGL_TYPE_BEAUPOINT)) {
               // In/out/beau/belle-point.
               // Ignoring the magic/interlocked bits for now; will get taken care of later.
               if ((thislivemask & 0x22) == 0)
                  map_key_table = tglmap::qttglmap1;
               else if ((thislivemask & 0x11) == 0)
                  map_key_table = tglmap::qttglmap2;
            }
            else if (kk == s_ptpd && ((tglindicator & ~1) == tglmap::TGL_TYPE_BEAUPOINT)) {
               // Beau/belle-point in point-to-point diamonds.
               if ((thislivemask & 0x11) == 0)
                  map_key_table = tglmap::ptptglmap1;
               else if ((thislivemask & 0x44) == 0)
                  map_key_table = tglmap::ptptglmap2;
            }
            else if (kk == sdmd && ((tglindicator & tglmap::TGL_IGNORE_MAG_INTLK_LOWBIT) == tglmap::TGL_TYPE_BEAUPOINT)) {
               // Beau/belle-point in single diamond.
               // Ignoring the magic/interlocked bits for now; will get taken care of later.
               if ((thislivemask & 0x4) == 0)
                  map_key_table = tglmap::dmtglmap1;
               else if ((thislivemask & 0x1) == 0)
                  map_key_table = tglmap::dmtglmap2;
            }
            else if (kk == s_rigger &&
                     (tglindicator & ~1) == (tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_WAVEBASE)) {
               map_key_table = tglmap::rgtglmap1;
            }
            else if (kk == s_rigger &&
                     tglindicator == (tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_ANYBASE)) {
               map_key_table = tglmap::rgtglmap1;
            }
            else if (kk == s_bone6 && (tglindicator & ~1) == tglmap::TGL_TYPE_WAVEBASE) {
               map_key_table = tglmap::b6tglmap1;
            }
            else if (kk == s_short6 && (tglindicator & ~1) == tglmap::TGL_TYPE_WAVEBASE) {
               map_key_table = tglmap::s6tglmap1;
            }
            else if (kk == s_c1phan && tglindicator == tglmap::TGL_TYPE_ANYBASE) {
               switch (thislivemask) {
               case 0xA8A8:
                  map_key_table = tglmap::c1tglmap2;
                  break;
               case 0x8A8A:
                  map_key_table = tglmap::c1tglmap2;
                  rotfix = 1;
                  break;
               case 0x4545:
                  map_key_table = tglmap::c1tglmap1;
                  break;
               case 0x5454:
                  map_key_table = tglmap::c1tglmap1;
                  rotfix = 1;
                  break;
               }

               this_one->rotation += rotfix;
               canonicalize_rotation(this_one);
            }
            else if (kk == s_323 && tglindicator == tglmap::TGL_TYPE_ANYBASE) {
               if (thislivemask == 0xBB) {
                  map_key_table = tglmap::s323map33;
               }
               else if (thislivemask == 0xEE) {
                  map_key_table = tglmap::s323map66;
               }
            }
            else if (kk == sd2x5 && tglindicator == tglmap::TGL_TYPE_ANYBASE) {
               if (thislivemask == 0x34D) {
                  map_key_table = tglmap::sd25map33;
               }
               else if (thislivemask == 0x1BA) {
                  map_key_table = tglmap::sd25map66;
               }
               else if (thislivemask == 0x16B) {
                  map_key_table = tglmap::sd25map16b;
               }
               else if (thislivemask == 0x1AD) {
                  map_key_table = tglmap::sd25map1ad;
               }
               else if (thislivemask == 0x35A) {
                  map_key_table = tglmap::sd25map35a;
               }
            }
            else if (kk == sd2x5 && tglindicator == (tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_ANYBASE)) {
               if (thislivemask == 0x34D) {
                  map_key_table = tglmap::sd25map33;
               }
               else if (thislivemask == 0x1BA) {
                  map_key_table = tglmap::sd25map66;
               }
               else if (thislivemask == 0x16B) {
                  map_key_table = tglmap::sd25map16b;
               }
            }
            else if (kk == s_434 && tglindicator == (tglmap::TGL_TYPE_ANYBASE)) {
               if (thislivemask == 0x273) {
                  map_key_table = tglmap::s434map73;
               }
               else if (thislivemask == 0x39C) {
                  map_key_table = tglmap::s434map14;
               }
            }
            else if (kk == s_3223 && tglindicator == (tglmap::TGL_TYPE_ANYBASE)) {
               if (thislivemask == 0x2B5) {
                  map_key_table = tglmap::s3223map2b;
               }
               else if (thislivemask == 0x1CE) {
                  map_key_table = tglmap::s3223map1c;
               }
               else if (thislivemask == 0x3E4) {
                  map_key_table = tglmap::s3223map3e;
               }
               else if (thislivemask == 0x09F) {
                  map_key_table = tglmap::s3223map09;
               }
               else if (thislivemask == 0x39C) {
                  map_key_table = tglmap::s3223map39;
               }
            }
            else if (kk == s_3223 && tglindicator == (tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_ANYBASE)) {
               if (thislivemask == 0x1AD) {
                  map_key_table = tglmap::s3223map2b;
               }
               else if (thislivemask == 0x2D6) {
                  map_key_table = tglmap::s3223map1c;
               }
               else if (thislivemask == 0x3E4) {
                  map_key_table = tglmap::s3223map3e;
               }
               else if (thislivemask == 0x09F) {
                  map_key_table = tglmap::s3223map09;
               }
               else if (thislivemask == 0x39C) {
                  map_key_table = tglmap::s3223map39;
               }
            }
            else if (kk == s_343 && tglindicator == tglmap::TGL_TYPE_ANYBASE) {
               if (thislivemask == 0x39C) {
                  map_key_table = tglmap::s343map33;
               }
               else if (thislivemask == 0x339) {
                  map_key_table = tglmap::s343map66;
               }
            }
            else if (kk == s_c1phan && tglindicator == (tglmap::TGL_CLASS_INTERLOCKED | tglmap::TGL_TYPE_ANYBASE)) {
               if (thislivemask == 0x2A2A) {
                  map_key_table = tglmap::c1tglmap2;
                  rotfix = 1;
               }
               else if (thislivemask == 0xA2A2) {
                  map_key_table = tglmap::c1tglmap2;
               }
               else if (thislivemask == 0x5151) {
                  map_key_table = tglmap::c1tglmap1;
                  rotfix = 1;
               }
               else if (thislivemask == 0x1515) {
                  map_key_table = tglmap::c1tglmap1;
               }

               this_one->rotation += rotfix;
               canonicalize_rotation(this_one);
            }
            else if (kk == s_c1phan && (tglindicator & tglmap::TGL_IGNORE_INTLK_LOWBIT) == tglmap::TGL_TYPE_WAVEBASE) {
               if (tglindicator & tglmap::TGL_CLASS_INTERLOCKED) {
                  i = 0x8484;   // Test for interlocked with vertical alignment of bases.
                  k = 0x4848;   // Test for interlocked with horizontal alignment of bases.
               }
               else {
                  i = 0x2121;   // Test for plain with vertical alignment of bases.
                  k = 0x1212;   // Test for plain with horizontal alignment of bases.
               }

               if ((thislivemask & i) == 0 && (thislivemask & k) != 0) {
                  rotfix = 1;     // we are vertical and not horizontal.
                  // Still need to check whether we are both, or neither.  That would be an error.
                  this_one->rotation += rotfix;   // Just flip the setup around and recanonicalize.
                  canonicalize_rotation(this_one);
                  thislivemask = little_endian_live_mask(this_one);
               }

               // Now rotfix is 1 to select the triangles whose bases are
               // vertically aligned, and 0 for the horizontally aligned bases.

               if ((thislivemask & 0xAAAA) == 0)
                  map_key_table = tglmap::c1tglmap1;
               else if ((thislivemask & 0x5555) == 0)
                  map_key_table = tglmap::c1tglmap2;

               if ((thislivemask & k) == 0 && (thislivemask & i) != 0) {
                  // We are horizontal and not vertical, OK.
               }
               else if (rotfix) {
                  // We are vertical and not horizontal, OK.
               }
               else
                  map_key_table = 0;    // Error.
            }
            else if (kk == sdeepbigqtg &&
                     (tglindicator & tglmap::TGL_IGNORE_INTLK_LOWBIT) == tglmap::TGL_TYPE_WAVEBASE) {
               if ((thislivemask & 0x3A3A) == 0)
                  map_key_table = tglmap::dbqtglmap1;
               else if ((thislivemask & 0xC5C5) == 0)
                  map_key_table = tglmap::dbqtglmap2;
            }
            else if (kk == sdeepbigqtg && tglindicator == tglmap::TGL_TYPE_ANYBASE) {
               if ((thislivemask & 0x3838) == 0)
                  map_key_table = tglmap::dbqtglmap1;
               else if ((thislivemask & 0xC4C4) == 0)
                  map_key_table = tglmap::dbqtglmap2;
            }
            else if ((tglindicator & tglmap::TGL_IGNORE_INTLK_LOWBIT) == tglmap::TGL_TYPE_WAVEBASE && kk == s_ntrgl6cw) {
               map_key_table = tglmap::t6cwtglmap1;
            }
            else if ((tglindicator & tglmap::TGL_IGNORE_INTLK_LOWBIT) == tglmap::TGL_TYPE_WAVEBASE && kk == s_ntrgl6ccw) {
               map_key_table = tglmap::t6ccwtglmap1;
            }
            else if ((tglindicator & ~3) == 0x400 && kk == s2x4) {
               tglindicator <<= 6;
               map_key_table = tglmap::rtglmap400;
            }
            else if ((tglindicator & ~3) == 0x500 && kk == s2x4) {
               tglindicator <<= 6;
               map_key_table = tglmap::rtglmap500;
            }

            if (!map_key_table)
               fail("Can't find the indicated triangles.");

            this_result->clear_people();
            tglmap::do_glorious_triangles(this_one, map_key_table, tglindicator, this_result);
            this_result->rotation -= rotfix;   // Flip the setup back.
            reinstate_rotation(this_one, this_result);
            goto done_with_this_one;
         }

         // By the way, if it is LOOKUP_DIST_DMD, there will be a "7" in the low bits
         // to make the "fixp2" stuff behave correctly.

         if (arg2 & LOOKUP_GEN_MASK)
            key = arg2 & LOOKUP_GEN_MASK;
         else if (arg2 != 0)
            key = LOOKUP_DIST_CLW;
         else if (orig_indicator == selective_key_ignore)
            key = LOOKUP_IGNORE;
         else if (indicator >= selective_key_disc_dist &&
                  indicator != selective_key_plain_from_id_bits)
            key = LOOKUP_DISC;
         else
            key = LOOKUP_NONE;

         bool allow_phantoms =
            (ss->cmd.cmd_misc_flags & (CMD_MISC__EXPLICIT_MATRIX|CMD_MISC__PHANTOMS)) ==
            (CMD_MISC__EXPLICIT_MATRIX|CMD_MISC__PHANTOMS);

         if ((key & (LOOKUP_STAG_BOX|LOOKUP_STAG_CLW)) && ss->kind != s4x4) {
            if (calling_level < stagger_not_in_2x4_level)
               warn_about_concept_level();
         }

         const select::fixer *fixp =
            select::hash_lookup(kk, thislivemask, allow_phantoms, key, arg2, this_one);

         if ((!fixp || (fixp->numsetups & 0xFF) != 1) &&
             this_one->cmd.parseptr &&
             this_one->cmd.parseptr->call &&
             (this_one->cmd.parseptr->call->the_defn.schema == schema_matrix ||
              this_one->cmd.parseptr->call->the_defn.schema == schema_partner_matrix ||
              this_one->cmd.parseptr->call->the_defn.schema == schema_partner_partial_matrix ||
              this_one->cmd.parseptr->call->the_defn.schema == schema_counter_rotate)) {
            // Do it as though we had said "do your part".
            move(this_one, false, this_result);
            goto done_with_this_one;
         }

         if (!fixp) {
            // These two have a looser livemask criterion.
            if (key & (LOOKUP_IGNORE|LOOKUP_DISC|LOOKUP_NONE)) {
               if (kk == s2x4 && the_setups[setupcount^1].kind == s2x4) {
                  // Used in rd03\1213:  Selector is girls, call is spread.  (Yes, if just say
                  // "girls spread")
                  if ((thislivemask & ~0x0F) == 0 && (otherlivemask & 0x0F) == 0)
                     fixp = select::fixer_ptr_table[select::fx_f2x4far];
                  else if ((thislivemask & ~0xF0) == 0  && (otherlivemask & 0xF0) == 0)
                     fixp = select::fixer_ptr_table[select::fx_f2x4near];
               }
               else if (kk == s4dmd && the_setups[setupcount^1].kind == s1x4) {
                  if ((thislivemask & ~0x0F0F) == 0) {
                     doing_iden = true;
                     fixp = select::fixer_ptr_table[select::fx_f4dmdiden];
                  }
               }
            }
         }

         if (!fixp) {
            // We don't have maps for all possible combinations of subsets of calls,
            // but maybe we can save the day by one of the following tricks.
            //
            // (1) If the call is a simple one-person space-invader, like "press ahead",
            //   that would normally take a selector, as in "boys press ahead", and is
            //   given here without a selector, just do it.
            // (2) Otherwise, just split the setup into separate 1-person setups and
            //   do the call for each one.  It has to be a one-person call like "1/4 right".
            if (setupcount == 1 &&
                this_one->cmd.parseptr &&
                this_one->cmd.parseptr->concept &&
                this_one->cmd.parseptr->concept->kind == marker_end_of_list &&
                this_one->cmd.parseptr->call &&
                this_one->cmd.parseptr->call->the_defn.schema == schema_matrix &&
                !(this_one->cmd.parseptr->call->the_defn.callflagsf & CFLAGH__REQUIRES_SELECTOR) &&
                orig_indicator == selective_key_plain) {
               move(this_one, false, this_result);
            }
            else {
               // This code actually never gets executed by any of the regression tests.
               // Whatever situation it was trying to address has since been dealt with
               // more elegantly.
               *this_result = *this_one;
               this_result->clear_people();
               clear_result_flags(this_result);
               int sizem1 = attr::slimit(this_one);

               try {
                  for (i=0 ; i<=sizem1; i++) {
                     if (this_one->people[i].id1) {
                        setup tt = *this_one;
                        tt.kind = s1x1;
                        tt.rotation = 0;
                        update_id_bits(&tt);
                        copy_person(&tt, 0, this_one, i);
                        setup uu;
                        move(&tt, false, &uu);
                        if (uu.kind != s1x1)
                           fail("Can't do this with these people designated.");
                        copy_person(this_result, i, &uu, 0);
                     }
                  }
               }
               catch(error_flag_type) {
                  // This is the error message we want.
                  fail("Can't do this with these people designated.");
               }
            }

            goto done_with_this_one;
         }

         numsetups = fixp->numsetups & 0xFF;
         map_scanner = 0;
         frot = fixp->rot;  // This stays fixed.
         vrot=frot>>2;      // This shifts down.

         for (lilcount=0; lilcount<numsetups; lilcount++) {
            uint32_t tbone = 0;
            setup *lilss = &lilsetup[lilcount];
            setup *lilres = &lilresult[lilcount];

            lilss->cmd = this_one->cmd;
            lilss->cmd.prior_elongation_bits = fixp->prior_elong;
            lilss->cmd.prior_expire_bits = 0;
            lilss->cmd.cmd_assume.assumption = cr_none;
            if (indicator == selective_key_disc_dist)
               lilss->cmd.cmd_misc_flags |= CMD_MISC__DISTORTED;
            lilss->kind = fixp->ink;
            lilss->rotation = 0;
            lilss->eighth_rotation = 0;
            if (fixp->ink == s_trngl && lilcount == 1) {
               lilss->rotation = 2;
               lilss->eighth_rotation = 0;
            }

            for (k=0;
                 k<=attr::klimit(fixp->ink);
                 k++,map_scanner++,vrot>>=2)
               tbone |= copy_rot(lilss, k, this_one, fixp->indices[map_scanner],
                                 011*((0-frot-vrot) & 3));

            // If we are picking out a distorted diamond from a 4x4, we can't tell
            // unambiguously how to do it unless all 4 people are facing in a
            // sensible way, that is, as if in real diamonds.  We did an extremely
            // cursory test to see which map to use, now we have to test whether
            // everyone agrees that the distortion direction was correct,
            // by checking whether they are in fact in real diamonds.

            if (kk == s4x4 && key == LOOKUP_DIST_DMD) {
               if (((lilss->people[0].id1) |
                    (~lilss->people[1].id1) |
                    (lilss->people[2].id1) |
                    (~lilss->people[3].id1)) & 1)
                  fail("Can't determine direction of diamond distortion.");
            }

            if ((arg2&7) == 2 && (tbone & 010) != 0) fail("There are no columns here.");
            if ((arg2&5) == 1 && (tbone & 001) != 0) fail("There are no lines here.");
            if ((arg2&7) == 3) lilss->cmd.cmd_misc_flags |= CMD_MISC__VERIFY_WAVES;

            if (indicator == selective_key_disc_dist)
               lilss->cmd.cmd_misc_flags |= CMD_MISC__NO_CHK_ELONG;

            lilss->cmd.cmd_misc_flags &= ~CMD_MISC__MUST_SPLIT_MASK;

            // Request Z compression.
            if (key == LOOKUP_Z)
               lilss->cmd.cmd_misc2_flags |= CMD_MISC2__REQUEST_Z;

            update_id_bits(lilss);
            impose_assumption_and_move(lilss, lilres);

            // There are a few cases in which we handle shape-changers in a distorted setup.
            // Print a warning if so.  Of course, it may not be allowed, in which case an
            // error will occur later.  But we won't give the warning if we went to a 4x4,
            // so we just set a flag.  See below.

            if (arg2 != 0 && key != LOOKUP_Z) {
               if (lilss->kind != lilres->kind ||
                   lilss->rotation != lilres->rotation)
                  feet_warning = true;
            }
         }

         if (doing_iden) {
            *this_result = lilresult[0];
            goto fooble;
         }

         fix_n_results(numsetups, -1, lilresult, rotstate, pointclip, 0);

         if (lilresult[0].kind == nothing) goto lose;
         if (!(rotstate & 0xF03)) fail("Sorry, can't do this orientation changer.");

         this_result->clear_people();

         this_result->result_flags =
            get_multiple_parallel_resultflags(lilresult, numsetups);
         this_result->result_flags.misc &= ~3;

         // If the call just kept a 2x2 in place, and they were the outsides, make
         // sure that the elongation is preserved.

         switch (this_one->kind) {
         case s2x2: case s_short6:
            this_result->result_flags.misc |= this_one->cmd.prior_elongation_bits & 3;
            break;
         case s1x2: case s1x4: case sdmd:
            this_result->result_flags.misc |= 2 - (this_one->rotation & 1);
            break;
         }

         this_result->rotation = 0;
         this_result->eighth_rotation = 0;

         // Ought to make this a method of "select".

         {
            const select::fixer *nextfixp = (const select::fixer *) 0;

            if (lilresult[0].kind == s2x2 && key == LOOKUP_Z) {
               expand::expand_setup((thislivemask == 066) ? fix_cw : fix_ccw, &lilresult[0]);
            }

            if (numsetups == 1 && lilresult[0].kind == s2x3) {
               if ((lilresult[0].people[0].id1 | lilresult[0].people[5].id1) == 0) {
                  expand::compress_setup(fix_2x3_r, &lilresult[0]);
               }
               else if ((lilresult[0].people[2].id1 | lilresult[0].people[3].id1) == 0) {
                  expand::compress_setup(fix_2x3_l, &lilresult[0]);
               }
            }

            if (lilresult[0].eighth_rotation != 0) {
               this_result->eighth_rotation = 1;

               if (lilresult[0].kind == s1x2 && fixp->mykey == select::fx_f4x5down && lilresult[0].rotation == 0) {
                  nextfixp = select::fixer_ptr_table[select::fx_4x5eight0];
               }
               else if (lilresult[0].kind == s1x2 && fixp->mykey == select::fx_f4x5down && lilresult[0].rotation == 1) {
                  nextfixp = select::fixer_ptr_table[select::fx_4x5eight1];
               }
               else if (lilresult[0].kind == s1x2 && fixp->mykey == select::fx_f4x5up && lilresult[0].rotation == 0) {
                  nextfixp = select::fixer_ptr_table[select::fx_4x5eight2];
                  this_result->rotation = 1;
               }
               else if (lilresult[0].kind == s1x2 && fixp->mykey == select::fx_f4x5up && lilresult[0].rotation == 1) {
                  nextfixp = select::fixer_ptr_table[select::fx_4x5eight3];
                  this_result->rotation = 1;
               }
               else
                  goto lose;
            }
            else if (numsetups == 2 && lilresult[0].kind == s_trngl) {
               if (((lilresult[0].rotation ^ lilresult[1].rotation) != 2) ||
                   ((lilresult[0].rotation & ~2) != 0))
                  goto lose;

               if (fixp == select::fixer_ptr_table[select::fx_f323]) {
                  this_result->rotation = 3;
                  this_result->eighth_rotation = 0;
                  nextfixp = select::fixer_ptr_table[select::fx_specspindle];
               }
               else if (fixp == select::fixer_ptr_table[select::fx_f3x4outer]) {
                  nextfixp = select::fixer_ptr_table[select::fx_specfix3x40];
               }
               else if (fixp == select::fixer_ptr_table[select::fx_fdhrgl] ||
                        fixp == select::fixer_ptr_table[select::fx_specspindle] ||
                        fixp == select::fixer_ptr_table[select::fx_specfix3x40] ||
                        fixp == select::fixer_ptr_table[select::fx_specfix3x41] ||
                        fixp == select::fixer_ptr_table[select::fx_fqtgitgl] ||
                        fixp == select::fixer_ptr_table[select::fx_fqtgctgl] ||
                        fixp == select::fixer_ptr_table[select::fx_fqtgatgl] ||
                        fixp == select::fixer_ptr_table[select::fx_fc1fctgl] ||
                        fixp == select::fixer_ptr_table[select::fx_fc1fatgl]) {
                  nextfixp = fixp;
               }
               else
                  goto lose;

               if (lilresult[0].rotation != 0 && !(nextfixp->rot & 0x40000000))
                  nextfixp = select::fixer_ptr_table[nextfixp->nextdmdrot];
            }
            else if (lilresult[0].rotation != 0) {
               if (attr::klimit(fixp->ink) == 5) {
                  if (lilresult[0].kind == s1x6 ||
                      (lilresult[0].kind == s1x4 && key == LOOKUP_Z))
                     nextfixp = select::fixer_ptr_table[fixp->next1x4rot];
                  else if (lilresult[0].kind == s2x3)
                     nextfixp = select::fixer_ptr_table[fixp->next2x2v];
                  else if (lilresult[0].kind == s_short6 && !(fixp->rot & 0x40000000))
                     nextfixp = select::fixer_ptr_table[fixp->nextdmdrot];
                  else if (lilresult[0].kind == s_bone6)
                     nextfixp = select::fixer_ptr_table[fixp->next1x2rot];
               }
               else if (attr::klimit(fixp->ink) == 7) {
                  if (lilresult[0].kind == s1x8)
                     nextfixp = select::fixer_ptr_table[fixp->next1x4rot];
                  else if (lilresult[0].kind == s2x4)
                     nextfixp = select::fixer_ptr_table[fixp->next2x2v];
               }
               else if (attr::klimit(fixp->ink) == 2) {
                  if (lilresult[0].kind == s1x3)
                     nextfixp = select::fixer_ptr_table[fixp->next1x2rot];
                  else if (lilresult[0].kind == s_trngl && !(fixp->rot & 0x40000000))
                     nextfixp = select::fixer_ptr_table[fixp->nextdmdrot];
               }
               else if (attr::klimit(fixp->ink) == 0) {
                  if (lilresult[0].kind == s1x3)
                     nextfixp = select::fixer_ptr_table[fixp->next1x4rot];
                  else if (lilresult[0].kind == s1x5)
                     nextfixp = select::fixer_ptr_table[fixp->nextdmdrot];
               }
               else if (attr::klimit(fixp->ink) == 11) {
                  if (lilresult[0].kind == s3x4)
                     nextfixp = select::fixer_ptr_table[fixp->next2x2v];
               }
               else {
                  if (lilresult[0].kind == s1x2)
                     nextfixp = select::fixer_ptr_table[fixp->next1x2rot];
                  else if (lilresult[0].kind == s1x4)
                     nextfixp = select::fixer_ptr_table[fixp->next1x4rot];
                  else if (lilresult[0].kind == s2x3 && (fixp->rot & 0x40000000))
                     nextfixp = select::fixer_ptr_table[fixp->nextdmdrot];
                  else if (lilresult[0].kind == s_trngl4 && lilresult[0].rotation == 2 && (fixp->rot & 0x40000000))
                     nextfixp = select::fixer_ptr_table[fixp->next2x2v];
                  else if (lilresult[0].kind == sdmd && !(fixp->rot & 0x40000000))
                     nextfixp = select::fixer_ptr_table[fixp->nextdmdrot];
               }

               inner_shape_change = true;

               if (!nextfixp) goto lose;

               if (((nextfixp->rot - fixp->rot) & 3) == 0) {
                  this_result->rotation--;

                  // Not happy about having to do this.  Gotta straighten this stuff out someday.
                  if ((fixp->numsetups & 0x100) || (nextfixp->numsetups & 0x200)) {
                     this_result->rotation += 2;

                     for (lilcount=0; lilcount<numsetups; lilcount++) {
                        lilresult[lilcount].rotation += 2;
                        canonicalize_rotation(&lilresult[lilcount]);
                     }
                  }
               }
               else if (((nextfixp->rot - fixp->rot) & 3) == 1) {
                  if (nextfixp->rot & 0x08000000) {
                     this_result->rotation += 1;
                  }
               }
               else if (((nextfixp->rot - fixp->rot) & 3) == 2) {
                  if (nextfixp->rot & 0x80000000) {
                     this_result->rotation += 2;
                  }
               }
               else if (((nextfixp->rot - fixp->rot) & 3) == 3) {
                  if (nextfixp->rot & 0x80000000) {
                     this_result->rotation += 2;
                  }
                  if (nextfixp->rot & 0x08000000) {
                     this_result->rotation += 3;
                  }
               }

               if ((nextfixp->rot & 3) == 0) {
                  if (!(nextfixp->rot & 0x20000000)) {
                     for (lilcount=0; lilcount<numsetups; lilcount++) {
                        lilresult[lilcount].rotation += 2;
                        canonicalize_rotation(&lilresult[lilcount]);
                     }
                  }
               }
            }
            else {
               if (attr::klimit(fixp->ink) == 5) {
                  if (lilresult[0].kind == s1x4 || lilresult[0].kind == s1x6)
                     nextfixp = select::fixer_ptr_table[fixp->next1x4];
                  else if (lilresult[0].kind == s2x3)
                     nextfixp = select::fixer_ptr_table[fixp->next2x2];
                  else if (lilresult[0].kind == s_short6)
                     nextfixp = select::fixer_ptr_table[fixp->nextdmd];
                  else if (lilresult[0].kind == s_bone6)
                     nextfixp = select::fixer_ptr_table[fixp->next1x2];
                  else if (lilresult[0].kind == s_1x2dmd)
                     nextfixp = select::fixer_ptr_table[fixp->next1x2rot];
               }
               else if (attr::klimit(fixp->ink) == 7) {
                  if (lilresult[0].kind == s1x8)
                     nextfixp = select::fixer_ptr_table[fixp->next1x4];
                  else if (lilresult[0].kind == s2x4)
                     nextfixp = select::fixer_ptr_table[fixp->next2x2];
                  else if (lilresult[0].kind == s_qtag)
                     nextfixp = select::fixer_ptr_table[fixp->nextdmd];
               }
               else if (attr::klimit(fixp->ink) == 2) {
                  if (lilresult[0].kind == s1x3)
                     nextfixp = select::fixer_ptr_table[fixp->next1x2];
                  else if (lilresult[0].kind == s_trngl)
                     nextfixp = select::fixer_ptr_table[fixp->nextdmd];
               }
               else if (attr::klimit(fixp->ink) == 0) {
                  if (lilresult[0].kind == s1x3)
                     nextfixp = select::fixer_ptr_table[fixp->next1x4];
                  else if (lilresult[0].kind == s1x5)
                     nextfixp = select::fixer_ptr_table[fixp->nextdmd];
               }
               else if (attr::klimit(fixp->ink) == 11) {
                  if (lilresult[0].kind == s3x4)
                     nextfixp = select::fixer_ptr_table[fixp->next2x2];
               }
               else {
                  if (lilresult[0].kind == s1x2)
                     nextfixp = select::fixer_ptr_table[fixp->next1x2];
                  else if (lilresult[0].kind == s1x4)
                     nextfixp = select::fixer_ptr_table[fixp->next1x4];
                  else if (lilresult[0].kind == sdmd)
                     nextfixp = select::fixer_ptr_table[fixp->nextdmd];
                  else if (lilresult[0].kind == s2x2) {
                     // If points are counter rotating in an hourglass, preserve
                     // elongation (by going to a 2x3) so they will go to the outside.
                     // But if they are doing it disconnected, always go to spots.
                     // See Application Note 3.
                     if (indicator != selective_key_disc_dist &&
                         (lilresult[0].result_flags.misc & 3) == 2)
                        nextfixp = select::fixer_ptr_table[fixp->next2x2v];
                     else
                        nextfixp = select::fixer_ptr_table[fixp->next2x2];
                  }
               }

               if (nextfixp) {
                  inner_shape_change = (nextfixp != fixp);
               }
               else {
                  if (lilresult[0].kind == fixp->ink)
                     nextfixp = fixp;
                  else if (indicator == selective_key_disc_dist &&
                           key == LOOKUP_DIST_CLW)
                     fail("Can't find distorted lines/columns, perhaps you mean offset.");
                  else
                     goto lose;
               }

               if ((fixp->rot ^ nextfixp->rot) & 3) {
                  if ((fixp->rot ^ nextfixp->rot) & 1) {
                     if (fixp->rot & 1)
                        this_result->rotation = 3;
                     else
                        this_result->rotation = 1;

                     this_result->eighth_rotation = 0;

                     if (nextfixp->rot & 0x80000000)
                        this_result->rotation += 2;
                  }
                  else {
                     this_result->rotation = 2;
                     this_result->eighth_rotation = 0;
                  }

                  if (!(nextfixp->rot & 0x80000000)) {
                     for (lilcount=0; lilcount<numsetups; lilcount++) {
                        lilresult[lilcount].rotation += 2;
                        canonicalize_rotation(&lilresult[lilcount]);
                     }
                  }
               }
            }

            fixp = nextfixp;
         }

         if (!fixp)
            goto lose;

         this_result->kind = fixp->outk;
         map_scanner = 0;
         frot = fixp->rot;  // This stays fixed.
         vrot=frot>>2;      // This shifts down.

         for (lilcount=0; lilcount<numsetups; lilcount++) {
            for (k=0;
                 k<=attr::klimit(lilresult[0].kind);
                 k++,map_scanner++,vrot>>=2)
               install_rot(this_result, fixp->indices[map_scanner],
                           &lilresult[lilcount], k, 011*((frot+vrot) & 3));
         }

         // We only give the warning if they in fact went to spots.  Some of the
         // maps create a result setup of 4x4.  For these maps, the dancers are
         // not actually going to spots, but are going back to the quadrants the
         // belong in.  This is a "put the offset back" type of adjustment.
         // There don't seem to be any generally recognized words that one says
         // to cause this to happen.  We hope the dancers will know what to do.

         // But if the overall call wasn't a shape-changer, *do* give the warning.
         // That is, the call performed on the little setups was a shape-changer,
         // but they nevertheless went back to spots in the same larger setup.
         // Except in 4x4's.  They are just too unpredictable.  See t36t, t37t, t50t.

         if (feet_warning &&
             fixp->outk != s4x4 &&
             (fixp->outk == kk || (fixp->outk != s3x4 && fixp->outk != sd2x7)))
            warn(warn__adjust_to_feet);

       fooble:

         reinstate_rotation(this_one, this_result);
      }
      else {
         uint32_t doing_mystic = this_one->cmd.cmd_misc2_flags &
            (CMD_MISC2__CTR_END_MASK & ~CMD_MISC2__SAID_INVERT);
         bool mirror = false;

         this_one->cmd.cmd_misc2_flags &= ~doing_mystic;

         if (setupcount == 1)
            doing_mystic ^= (CMD_MISC2__INVERT_MYSTIC|CMD_MISC2__INVERT_SNAG);

         if ((doing_mystic & (CMD_MISC2__CENTRAL_MYSTIC|CMD_MISC2__INVERT_MYSTIC)) ==
             CMD_MISC2__CENTRAL_MYSTIC) {
            mirror = true;
         }
         else if ((doing_mystic & (CMD_MISC2__CENTRAL_SNAG|CMD_MISC2__INVERT_SNAG)) ==
                  CMD_MISC2__CENTRAL_SNAG) {
            if (!this_one->cmd.cmd_fraction.is_null())
               fail("Can't do fractional \"snag\".");
            this_one->cmd.cmd_fraction.set_to_firsthalf();
         }

         if (mirror) {
            mirror_this(this_one);
            this_one->cmd.cmd_misc_flags ^= CMD_MISC__EXPLICIT_MIRROR;
         }

         if (others > 0 && setupcount == 0) {
            if ((this_one->cmd.cmd_misc3_flags &
                 (CMD_MISC3__PUT_FRAC_ON_FIRST|CMD_MISC3__RESTRAIN_CRAZINESS)) ==
                CMD_MISC3__PUT_FRAC_ON_FIRST) {
               // Curried meta-concept.  Take the fraction info off the first call.
               // This should only be legal for "own the <anyone>".
               this_one->cmd.cmd_misc3_flags &= ~CMD_MISC3__PUT_FRAC_ON_FIRST;
               this_one->cmd.cmd_fraction.set_to_null();
            }
         }

         // Look for special cases like two people peeling off from the base of a triangle.
         if (special_unsym_dyp && this_one->kind == s_trngl4 && (thislivemask & 3) == 0) {
            setup trythis = *this_one;
            trythis.kind = s2x2;
            trythis.clear_people();
            copy_person(&trythis, 0, this_one, 2);
            copy_person(&trythis, 1, this_one, 3);
            canonicalize_rotation(&trythis);
            move(&trythis, false, this_result);
         }
         else if (special_unsym_dyp && this_one->kind == s_trngl && (thislivemask & 1) == 0) {
            setup trythis = *this_one;
            trythis.kind = s2x2;
            trythis.clear_people();
            copy_person(&trythis, 0, this_one, 1);
            copy_person(&trythis, 1, this_one, 2);
            canonicalize_rotation(&trythis);
            move(&trythis, false, this_result);
         }
         else {
            move(this_one, false, this_result);
         }

         if (mirror) mirror_this(this_result);
      }

   done_with_this_one:

      current_options.number_fields = svd_number_fields;
      current_options.howmanynumbers = svd_num_numbers;

      if (setupcount >= others) break;
   }

   if (others <= 0) {      // The non-designees did nothing.
      the_results[1] = the_setups[1];
      // Give the people who didn't move the same result flags as those who did.
      // This is important for the "did last part" check.
      the_results[1].result_flags = the_results[0].result_flags;
      if (livemask[1] == 0) the_results[1].kind = nothing;

      // Strip out the roll bits -- people who didn't move can't roll.  Unless this is roll-transparent.
      if (others == 0) {
         the_results[1].suppress_all_rolls((the_setups[1].cmd.cmd_misc3_flags & CMD_MISC3__ROLL_TRANSP) != 0);
      }
   }

   // Shut off "each 1x4" types of warnings -- they will arise spuriously while
   // the people do the calls in isolation.
   // Also, shut off "superfluous phantom setups" warnings if this was "own the
   // <anyone> or <anyone> do your part".
   // But note that "warn__split_to_1x3s_always" warnings do not get shut off--this means
   // that "center 1x6 swing thru" in the "1/2 acey deucey" setup will raise the warning.
   configuration::clear_multiple_warnings(dyp_each_warnings);
   if (indicator < selective_key_plain_no_live_subsets)
      configuration::clear_multiple_warnings(useless_phan_clw_warnings);
   if (indicator == selective_key_own)
      configuration::clear_multiple_warnings(conc_elong_warnings);

   configuration::set_multiple_warnings(saved_warnings);

   *result = the_results[1];
   result->result_flags = get_multiple_parallel_resultflags(the_results, 2);

   {
      merge_action ma = merge_c1_phantom;

      if (ss->kind == s4x4 && indicator == selective_key_plain && !inner_shape_change)
         ma = merge_strict_matrix;

      if (force_matrix_merge) {
         // If we are doing an "anyone truck" type of call from
         // a C1 phantom, go back to same.
         if (ss->kind != s_c1phan)
            ma = merge_strict_matrix;
      }
      else if (indicator == selective_key_own)
         ma = merge_for_own;
      else if (indicator == selective_key_mini_but_o)
         ma = merge_strict_matrix;
      else if (indicator == selective_key_dyp &&
               ss->cmd.parseptr &&
               ss->cmd.parseptr->concept->kind == concept_do_phantom_2x4)
         ma = merge_strict_matrix;
      else if (indicator == selective_key_dyp ||
               indicator == selective_key_plain_from_id_bits)
         ma = merge_after_dyp;
      else if (indicator == selective_key_work_concept && cmd1->parseptr && cmd1->parseptr->concept &&
               cmd1->parseptr->concept->kind == concept_tandem) {
         livemask[0] = little_endian_live_mask(&the_results[0]);
         livemask[1] = little_endian_live_mask(&the_results[1]);
         if (the_results[0].kind == s2x8 && the_results[1].kind == s2x4 &&
             (livemask[0] & 0x3C3C) == 0 &&
             (livemask[1] & 0x99) == 0) {
            // Some people working as couples or tandem went way to the outside, under
            // the impression that the others must be getting wider also.  If the others
            // weren't getting wider (since they were working normally), clear out some
            // of the interior space in the first result.
            static const expand::thing fix_big_2x8 =
            {{0, 1, -1, -1, 6, 7, 8, 9, -1, -1, 14, 15}, s2x6, s2x8, 0};
            expand::compress_setup(fix_big_2x8, &the_results[0]);
         }
      }
      else if (the_results[0].kind == s1x6 &&
               the_results[1].kind == s3x6 &&
               the_results[0].rotation == the_results[1].rotation)
         ma = merge_strict_matrix;
      else if (the_results[0].kind == s3x4 &&
               the_results[1].kind == s3x6 &&
               the_results[0].rotation == the_results[1].rotation)
         ma = merge_strict_matrix;
      else if (indicator == selective_key_disc_dist)
         ma = merge_without_gaps;
      else if (tglindicator)
         ma = merge_c1_phantom_from_triangle_call;

      merge_table::merge_setups(&the_results[0], ma, result);
   }

   return;

 lose: fail("Can't do this call with these people.");

 do_concentric_ends:
   crossconc = true;
   goto do_concentric_something;

 do_concentric_ctrs:
   crossconc = false;

   // If the setup is bigger than 8 people, concentric_move won't be able to handle it.
   // The only hope is that we are just having the center 2 or center 4 do something.
   // There are a few exceptions.  In those cases we presume that the centers make a Z,
   // and that concentric_move will handle it.

   if (attr::slimit(ss) > 7 &&
       ss->kind != sd2x5 &&
       ss->kind != s2x5 &&
       indicator == selective_key_plain &&
       others <= 0)
      goto back_here;

 do_concentric_something:

   if (indicator == selective_key_work_concept || indicator == selective_key_snag_anyone)
      goto use_punt_stuff;

   ss->cmd.cmd_misc_flags |= CMD_MISC__NO_EXPAND_MATRIX;

   // We may need to back out of this.  If either call is a space-invader,
   // we do *not* do the normal concentric formulation.  We have everyone do
   // their call in the actual setup, and merge them directly.  The test for this
   // is something like "centers truck while the ends reverse truck".
   saved_warnings = configuration::save_warnings();

   // We abuse the "specialoffsetmapcode" argument in order to communicate that we are doing
   // a "promenade" type of call.  It's OK; no map code comes anywhere near -1.

   {
      uint32_t specialoffsetmapcode = doing_special_promenade_thing ? ~1U : ~0U;

      concentric_move(ss,
                      crossconc ? cmd2 : cmd1,
                      crossconc ? cmd1 : cmd2,
                      schema, modsa1, modsb1, true, enable_3x1_warn, specialoffsetmapcode, result);
   }

   return;

   use_punt_stuff:

   if (doing_special_promenade_thing)
      fail("Can't do this with these people designated.");

   switch (schema) {
      case schema_concentric_2_6:
         bigend_ssmask = setup_attrs[ss->kind].setup_conc_masks.mask_2_6;
         break;
      case schema_concentric_6_2:
         bigend_ssmask = setup_attrs[ss->kind].setup_conc_masks.mask_6_2;
         break;
      default:
         bigend_ssmask = setup_attrs[ss->kind].setup_conc_masks.mask_normal;
         break;
   }

   if (!bigend_ssmask) goto back_here;    // We don't know how to find centers and ends.

   if (ss->cmd.cmd_misc2_flags & (CMD_MISC2__ANY_WORK | CMD_MISC2__ANY_SNAG))
      fail("Can't stack \"<anyone> work <concept>\" concepts.");

   ss->cmd.cmd_misc2_flags |=
      (indicator == selective_key_snag_anyone) ? CMD_MISC2__ANY_SNAG : CMD_MISC2__ANY_WORK;
   ss->cmd.cmd_misc2_flags &= ~(0xFFF | CMD_MISC2__ANY_WORK_INVERT);
   ss->cmd.cmd_misc2_flags |= (0xFFF & ((int) schema));
   if (crossconc) ss->cmd.cmd_misc2_flags |= CMD_MISC2__ANY_WORK_INVERT;

   // If we aren't already looking for something,
   // check whether to put on a new assumption.

   if (ss->kind == s2x4 && ss->cmd.cmd_assume.assumption == cr_none) {
      assumption_thing restr;
      bool booljunk;

      restr.assumption = cr_wave_only;
      restr.assump_col = 0;
      restr.assump_both = 0;
      restr.assump_negate = 0;
      // Only do this if all are live -- otherwise we would be imposing
      // a stronger restriction on the whole setup than we ought to.
      restr.assump_live = 1;

      if (verify_restriction(ss, restr, false, &booljunk) == restriction_passes)
         ss->cmd.cmd_assume = restr;
      else {
         restr.assumption = cr_li_lo;
         restr.assump_col = 1;
         restr.assump_both = 1;
         if (verify_restriction(ss, restr, false, &booljunk) == restriction_passes)
            ss->cmd.cmd_assume = restr;
      }
   }

   move(ss, false, result);
}
