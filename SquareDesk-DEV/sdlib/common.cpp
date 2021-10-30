// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2021  William B. Ackerman.
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
//    You should have received a copy of the GNU General Public License,
//    in the file COPYING.txt, along with Sd.  See
//    http://www.gnu.org/licenses/
//
//    ===================================================================

#include "sd.h"


// These combinations are not allowed.

#define FORBID1R (INHERITFLAG_FRACTAL|INHERITFLAG_YOYOETCMASK)
#define FORBID2R (INHERITFLAG_SINGLEFILE|INHERITFLAG_SINGLE)
#define FORBID3R (INHERITFLAG_MXNMASK|INHERITFLAG_NXNMASK)
#define FORBID4R (INHERITFLAG_12_MATRIX|INHERITFLAG_16_MATRIX)


bool do_heritflag_merge(heritflags & dest, heritflags source)
{
   uint64_t revertsource = source & INHERITFLAG_REVERTMASK;

   if (revertsource != 0ULL) {
      // If the source is a revert/reflect bit, things are complicated.

      uint64_t revertdest = dest & INHERITFLAG_REVERTMASK;

      if (revertdest == 0) {
         goto good;
      }
      else if (revertsource == INHERITFLAGRVRTK_REVERT &&
               revertdest == INHERITFLAGRVRTK_REFLECT) {
         dest &= ~INHERITFLAG_REVERTMASK;
         dest |= INHERITFLAGRVRTK_RFV;
         return false;
      }
      else if (revertsource == INHERITFLAGRVRTK_REFLECT &&
               revertdest == INHERITFLAGRVRTK_REVERT) {
         dest &= ~INHERITFLAG_REVERTMASK;
         dest |= INHERITFLAGRVRTK_RVF;
         return false;
      }
      else if (revertsource == INHERITFLAGRVRTK_REFLECT &&
               revertdest == INHERITFLAGRVRTK_REFLECT) {
         dest &= ~INHERITFLAG_REVERTMASK;
         dest |= INHERITFLAGRVRTK_RFF;
         return false;
      }
      else if (revertsource == INHERITFLAGRVRTK_REVERT &&
               revertdest == INHERITFLAGRVRTK_RVF) {
         dest &= ~INHERITFLAG_REVERTMASK;
         dest |= INHERITFLAGRVRTK_RVFV;
         return false;
      }
      else if (revertsource == INHERITFLAGRVRTK_REFLECT &&
               revertdest == INHERITFLAGRVRTK_RFV) {
         dest &= ~INHERITFLAG_REVERTMASK;
         dest |= INHERITFLAGRVRTK_RFVF;
         return false;
      }
      else
         return true;
   }

   // Check for plain redundancy.  If this is a bit in one of the complex
   // fields, this simple test may not catch the error, but the next one will.

   if ((dest & source) != 0ULL)
      return true;

   if (((dest & FORBID1R) != 0ULL && (source & FORBID1R) != 0ULL) ||
       ((dest & FORBID2R) != 0ULL && (source & FORBID2R) != 0ULL) ||
       ((dest & FORBID3R) != 0ULL && (source & FORBID3R) != 0ULL) ||
       ((dest & FORBID4R) != 0ULL && (source & FORBID4R) != 0ULL))
      return true;

   good:

   dest |= source;

   return false;
}


// BEWARE!!  This list is keyed to the definition of "begin_kind" in sd.h .
int begin_sizes[] = {
   0,          // b_nothing
   1,          // b_1x1
   2,          // b_1x2
   2,          // b_2x1
   3,          // b_1x3
   3,          // b_3x1
   4,          // b_2x2
   4,          // b_dmd
   4,          // b_pmd
   4,          // b_star
   6,          // b_trngl
   6,          // b_ptrngl
   8,          // b_trngl4
   8,          // b_ptrngl4
   10,         // b_beehive,
   10,         // b_pbeehive,
   10,         // b_vee,
   10,         // b_pvee,
   16,         // b_trngl8
   16,         // b_ptrngl8
   8,          // b_linebox
   8,          // b_plinebox
   8,          // b_linejbox
   8,          // b_plinejbox
   8,          // b_linevbox
   8,          // b_plinevbox
   8,          // b_lineybox
   8,          // b_plineybox
   8,          // b_linefbox
   8,          // b_plinefbox
   6,          // b_bone6
   6,          // b_pbone6
   6,          // b_short6
   6,          // b_pshort6
   6,          // b_1x2dmd
   6,          // b_p1x2dmd
   6,          // b_2x1dmd
   6,          // b_p2x1dmd
   6,          // b_wingedstar6
   6,          // b_pwingedstar6
   8,          // b_qtag
   8,          // b_pqtag
   8,          // b_bone
   8,          // b_pbone
   8,          // b_rigger
   8,          // b_prigger
   9,          // b_3x3
   8,          // b_2stars
   8,          // b_p2stars
   8,          // b_spindle
   8,          // b_pspindle
   8,          // b_hrglass
   8,          // b_phrglass
   8,          // b_dhrglass
   8,          // b_pdhrglass
   8,          // b_crosswave
   8,          // b_pcrosswave
   4,          // b_1x4
   4,          // b_4x1
   8,          // b_1x8
   8,          // b_8x1
   8,          // b_2x4
   8,          // b_4x2
   6,          // b_2x3
   6,          // b_3x2
  10,          // b_2x5
  10,          // b_5x2
  10,          // b_d2x5
  10,          // b_d5x2
  10,          // b_wqtag
  10,          // b_pwqtag
  10,          // b_deep2x1dmd
  10,          // b_pdeep2x1dmd
  10,          // b_whrglass
  10,          // b_pwhrglass
   5,          // b_1x5
   5,          // b_5x1
   6,          // b_1x6
   6,          // b_6x1
   12,         // b_3x4
   12,         // b_4x3
   12,         // b_2x6
   12,         // b_6x2
   14,         // b_2x7
   14,         // b_7x2
   14,         // b_d2x7
   14,         // b_d7x2
   18,         // b_2x9
   18,         // b_9x2
   12,         // b_d3x4
   12,         // b_d4x3
   20,         // b_d4x5
   20,         // b_d5x4
   10,         // b_spindle12
   10,         // b_pspindle12
   16,         // b_2x8
   16,         // b_8x2
   16,         // b_4x4
   10,         // b_1x10
   10,         // b_10x1
   12,         // b_1x12
   12,         // b_12x1
   14,         // b_1x14
   14,         // b_14x1
   16,         // b_1x16
   16,         // b_16x1
   16,         // b_c1phan
   8,          // b_galaxy
   18,         // b_3x6
   18,         // b_6x3
   24,         // b_3x8
   24,         // b_8x3
   20,         // b_4x5
   20,         // b_5x4
   24,         // b_4x6
   24,         // b_6x4
   20,         // b_2x10
   20,         // b_10x2
   24,         // b_2x12
   24,         // b_12x2
   12,         // b_deepqtg
   12,         // b_pdeepqtg
   16,         // b_deepbigqtg
   16,         // b_pdeepbigqtg
   12,         // b_widerigger
   12,         // b_pwiderigger
   12,         // b_rigger12
   12,         // b_prigger12
   12,         // b_deepxwv
   12,         // b_pdeepxwv
   20,         // b_3oqtg
   20,         // b_p3oqtg
   8,          // b_thar
   8,          // b_alamo
   8,          // b_ptpd
   8,          // b_pptpd
   8,          // b_1x3dmd
   8,          // b_p1x3dmd
   8,          // b_3x1dmd
   8,          // b_p3x1dmd
   12,         // b_3dmd
   12,         // b_p3dmd
   16,         // b_4dmd
   16,         // b_p4dmd
   12,         // b_3ptpd
   12,         // b_p3ptpd
   16,         // b_4ptpd
   16,         // b_p4ptpd
   16,         // b_2x2dmd
   16,         // b_p2x2dmd
   16,         // b_hqtag
   16,         // b_phqtag
   12,         // b_hsqtag
   12,         // b_phsqtag
   8,          // b_wingedstar
   8,          // b_pwingedstar
   8,          // ntrgl6cw
   8,          // pntrgl6cw
   8,          // ntrgl6ccw
   8,          // pntrgl6ccw
   8,          // nftrgl6cw
   8,          // pnftrgl6cw
   8,          // nftrgl6ccw
   8,          // pnftrgl6ccw
   8,          // ntrglcw
   8,          // pntrglcw
   8,          // ntrglccw
   8,          // pntrglccw
   8,          // nptrglcw
   8,          // pnptrglcw
   8,          // nptrglccw
   8,          // pnptrglccw
   8,          // nxtrglcw
   8,          // pnxtrglcw
   8,          // nxtrglccw
   8,          // pnxtrglccw
   8,          // b_323
   8,          // b_p323
   10,         // b_343
   10,         // b_p343
   10,         // b_3223
   10,         // b_p3223
   12,         // b_525
   12,         // b_p525
   14,         // b_545
   14,         // b_p545
   15,         // b_3x5
   15,         // b_p3x5
   11,         // b_434
   11,         // b_p434
   14,         // bh545
   14,         // bhp545
   12,         // b_23232
   12,         // b_p23232
   12,         // b_3mdmd
   12,         // b_p3mdmd
   12,         // b_3mptpd
   12,         // b_p3mptpd
   16,         // b_4mdmd
   16,         // b_p4mdmd
   16,         // b_4mptpd
   16,         // b_p4mptpd
   10,         // b_1x4dmd
   10,         // b_p1x4dmd
   12,         // b_bigh
   12,         // b_pbigh
   12,         // b_bigx
   12,         // b_pbigx
   16,         // b_bigbigh
   16,         // b_pbigbigh
   16,         // b_bigbigx
   16,         // b_pbigbigx
   12,         // b_bigrig
   12,         // b_pbigrig
   12,         // b_bighrgl
   12,         // b_pbighrgl
   12,         // b_bigdhrgl
   12,         // b_pbigdhrgl
   12,         // b_bigbone
   12,         // b_pbigbone
   12,         // b_dblbone6
   12,         // b_pdblbone6
   12,         // b_bigdmd
   12,         // b_pbigdmd
   12,         // b_bigptpd
   12,         // b_pbigptpd
   12,         // b_5x1dmd
   12,         // b_p5x1dmd
   12,         // b_1x5dmd
   12,         // b_p1x5dmd
   18,         // b_big3dmd
   18,         // b_pbig3dmd
   24,         // b_big4dmd
   24,         // b_pbig4dmd
   16,         // b_dblxwave
   16,         // b_pdblxwave
   16,         // b_dblspindle
   16,         // b_pdblspindle
   16,         // b_dblbone
   16,         // b_pdblbone
   16,         // b_dblrig
   16};        // b_pdblrig
