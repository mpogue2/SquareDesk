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


/* This defines the following function:
   selectp

and the following external variables:
   selector_used
   direction_used
   number_used
   mandatory_call_used
   pred_table     which is filled with pointers to the predicate functions
   selector_preds
*/

#include "sd.h"


// These variables are external.

bool selector_used;
bool direction_used;
bool number_used;
bool mandatory_call_used;


extern bool selectp(const setup *ss, int place, int allow_some /*= 0*/) THROW_DECL
{
   uint32_t p1, p2, p2bg, p3;
   selector_kind s;
   int thing_to_test = 0;
   int other_thing_to_test = 0;
   int other_other_thing_to_test = 0;
   int tand_base = 0;
   uint32_t selected_person_mask = ~0U;

   uint32_t directions;
   uint32_t livemask;
   big_endian_get_directions32(ss, directions, livemask);

   selector_used = true;
   selector_kind local_selector = current_options.who.who[0];
   if (local_selector == selector_outsides)
      local_selector = selector_ends;
   if (local_selector == selector_verycenters)
      local_selector = selector_center2;

   // Pull out the cases that do not require the person to be real.

   switch (local_selector) {
      case selector_all:
      case selector_everyone:
         return true;
      case selector_none:
         return false;
   }

   uint32_t pid1 = ss->people[place].id1;
   uint32_t pid2 = ss->people[place].id2;
   uint32_t pid3 = ss->people[place].id3;

   // Demand that the subject be real, or that
   // we can evaluate based on position alone.

   if (!(pid1 & BIT_PERSON)) {
      // We can still do it for some selectors.  However, this violates
      // the rules about using the *original* centers in calls like
      // "rims trade back".  But, if the person isn't real, this is the best
      // we can do.
      //
      // We do this based on the stuff in the attribute tables.  If the
      // setup is larger than 8, we can't -- the tables become very complicated
      // based on the actual occupancy, and we are looking at an unoccupied
      // space.

      if (attr::slimit(ss) < 8) {
         const id_bit_table *ptr = setup_attrs[ss->kind].id_bit_table_ptr;

         if (ptr) {
            switch (local_selector) {
            case selector_centers:
            case selector_ends:
            case selector_center2:
            case selector_outer6:
            case selector_verycenters:
            case selector_center6:
            case selector_outer2:
            case selector_veryends:
            case selector_ctrdmd:
            case selector_ctr_1x4:
            case selector_ctr_1x6:
            case selector_outer1x3s:
            case selector_center4:
            case selector_center_wave:
            case selector_center_line:
            case selector_center_col:
            case selector_center_wave_of_6:
            case selector_center_line_of_6:
            case selector_center_col_of_6:
            case selector_center_box:
            case selector_outerpairs:
               pid2 = ptr[place][0] & ID2_BITS_TO_CLEAR_FOR_PHANTOM_SELECTOR;
               goto do_switch;
            }
         }
      }

      fail("Can't decide who are selected.");
   }

 do_switch:

   switch (local_selector) {
   case selector_boys:
      if      ((pid2 & (ID2_PERM_BOY|ID2_PERM_GIRL)) == ID2_PERM_BOY) return true;
      else if ((pid2 & (ID2_PERM_BOY|ID2_PERM_GIRL)) == ID2_PERM_GIRL) return false;
      break;
   case selector_girls:
      if      ((pid2 & (ID2_PERM_BOY|ID2_PERM_GIRL)) == ID2_PERM_GIRL) return true;
      else if ((pid2 & (ID2_PERM_BOY|ID2_PERM_GIRL)) == ID2_PERM_BOY) return false;
      break;
   case selector_heads:
      if      ((pid2 & (ID2_PERM_HEAD|ID2_PERM_SIDE)) == ID2_PERM_HEAD) return true;
      else if ((pid2 & (ID2_PERM_HEAD|ID2_PERM_SIDE)) == ID2_PERM_SIDE) return false;
      break;
   case selector_sides:
      if      ((pid2 & (ID2_PERM_HEAD|ID2_PERM_SIDE)) == ID2_PERM_SIDE) return true;
      else if ((pid2 & (ID2_PERM_HEAD|ID2_PERM_SIDE)) == ID2_PERM_HEAD) return false;
      break;
   case selector_headcorners:
      if      ((pid2 & (ID2_PERM_HCOR|ID2_PERM_SCOR)) == ID2_PERM_HCOR) return true;
      else if ((pid2 & (ID2_PERM_HCOR|ID2_PERM_SCOR)) == ID2_PERM_SCOR) return false;
      break;
   case selector_sidecorners:
      if      ((pid2 & (ID2_PERM_HCOR|ID2_PERM_SCOR)) == ID2_PERM_SCOR) return true;
      else if ((pid2 & (ID2_PERM_HCOR|ID2_PERM_SCOR)) == ID2_PERM_HCOR) return false;
      break;
   case selector_headboys:
      if      ((pid2 & (ID2_PERM_ALL_ID|ID2_PERM_NHB)) == (ID2_PERM_HEAD|ID2_PERM_BOY))
         return true;
      else if ((pid2 & ID2_PERM_NHB) == ID2_PERM_NHB)
         return false;
      break;
   case selector_headgirls:
      if      ((pid2 & (ID2_PERM_ALL_ID|ID2_PERM_NHG)) == (ID2_PERM_HEAD|ID2_PERM_GIRL))
         return true;
      else if ((pid2 & ID2_PERM_NHG) == ID2_PERM_NHG)
         return false;
      break;
   case selector_sideboys:
      if      ((pid2 & (ID2_PERM_ALL_ID|ID2_PERM_NSB)) == (ID2_PERM_SIDE|ID2_PERM_BOY))
         return true;
      else if ((pid2 & ID2_PERM_NSB) == ID2_PERM_NSB)
         return false;
      break;
   case selector_sidegirls:
      if      ((pid2 & (ID2_PERM_ALL_ID|ID2_PERM_NSG)) == (ID2_PERM_SIDE|ID2_PERM_GIRL))
         return true;
      else if ((pid2 & ID2_PERM_NSG) == ID2_PERM_NSG)
         return false;
      break;
   case selector_centers:
   case selector_ends:
      p2 = pid2 & (ID2_CENTER|ID2_END);
      if      (p2 == ID2_CENTER) s = selector_centers;
      else if (p2 == ID2_END)    s = selector_ends;
      else break;
      goto eq_return;
   case selector_leaders:
      local_selector = selector_leads;
      // FALL THROUGH!!!
   case selector_leads:
   case selector_trailers:
      // FELL THROUGH!!!
      p2 = pid2 & (ID2_LEAD|ID2_TRAILER);
      if      (p2 == ID2_LEAD)    s = selector_leads;
      else if (p2 == ID2_TRAILER) s = selector_trailers;
      else break;
      goto eq_return;
   case selector_lead_ends:
   case selector_lead_ctrs:
   case selector_trail_ends:
   case selector_trail_ctrs:
      p2 = pid2 & (ID2_LEAD|ID2_TRAILER|ID2_CENTER|ID2_END);
      if      (p2 == (ID2_LEAD|ID2_END))       s = selector_lead_ends;
      else if (p2 == (ID2_LEAD|ID2_CENTER))    s = selector_lead_ctrs;
      else if (p2 == (ID2_TRAILER|ID2_END))    s = selector_trail_ends;
      else if (p2 == (ID2_TRAILER|ID2_CENTER)) s = selector_trail_ctrs;
      else break;
      goto eq_return;
   case selector_lead_beaus:
   case selector_lead_belles:
   case selector_trail_beaus:
   case selector_trail_belles:
      p2 = pid2 & (ID2_LEAD|ID2_TRAILER|ID2_BEAU|ID2_BELLE);
      if      (p2 == (ID2_LEAD|ID2_BEAU))      s = selector_lead_beaus;
      else if (p2 == (ID2_LEAD|ID2_BELLE))     s = selector_lead_belles;
      else if (p2 == (ID2_TRAILER|ID2_BEAU))   s = selector_trail_beaus;
      else if (p2 == (ID2_TRAILER|ID2_BELLE))  s = selector_trail_belles;
      else break;
      goto eq_return;
   case selector_end_boys:
   case selector_end_girls:
   case selector_center_boys:
   case selector_center_girls:
      p2bg = pid2 & (ID2_PERM_BOY|ID2_PERM_GIRL);
      p2 = pid2 & (ID2_CENTER|ID2_END);
      if (ss->kind == s4x5 &&
          (local_selector == selector_end_boys || local_selector == selector_end_girls)) {
         // We claim we can identify these people as "ends"
         if (((0x8C631 >> place) & 1) != 0) {
            if (p2bg == ID2_PERM_BOY) s = selector_end_boys;
            else if (p2bg == ID2_PERM_GIRL) s = selector_end_girls;
            else break;
            goto eq_return;
         }
         else
            return false;
      }
      else {
         if      (p2 == ID2_END && p2bg == ID2_PERM_BOY)     s = selector_end_boys;
         else if (p2 == ID2_END && p2bg == ID2_PERM_GIRL)    s = selector_end_girls;
         else if (p2 == ID2_CENTER && p2bg == ID2_PERM_BOY)  s = selector_center_boys;
         else if (p2 == ID2_CENTER && p2bg == ID2_PERM_GIRL) s = selector_center_girls;
         else break;
         goto eq_return;
      }
   case selector_beaus:
   case selector_belles:
      p2 = pid2 & (ID2_BEAU|ID2_BELLE);
      if      (p2 == ID2_BEAU)  s = selector_beaus;
      else if (p2 == ID2_BELLE) s = selector_belles;
      else break;
      goto eq_return;
   case selector_mysticbeaus:
   case selector_mysticbelles:
      p2 = pid2 & (ID2_BEAU|ID2_BELLE|ID2_CTR4|ID2_END);
      p1 = pid2 & (ID2_BEAU|ID2_BELLE|ID2_OUTRPAIRS);
      if      (p2 == (ID2_END|ID2_BEAU) ||
               p2 == (ID2_CTR4|ID2_BELLE)) s = selector_mysticbeaus;
      else if (p2 == (ID2_END|ID2_BELLE) ||
               p2 == (ID2_CTR4|ID2_BEAU)) s = selector_mysticbelles;
      else if (p1 == (ID2_OUTRPAIRS|ID2_BEAU)) s = selector_mysticbeaus;
      else if (p1 == (ID2_OUTRPAIRS|ID2_BELLE)) s = selector_mysticbelles;
      else break;
      goto eq_return;
   case selector_center2:
   case selector_outer6:
      if (ss->kind == s3x4 && local_selector == selector_center2) {
         return (pid2 & ID2_CTR2) != 0;  // This one is always valid, even if we don't know who the "outer 6" are.
      }
      else if (ss->kind == s4x5 && local_selector == selector_center2) {
         return (place == 7 || place == 17);
      }
      else if (ss->kind == s4x4 && local_selector == selector_center2) {
         // Demand that there not be too many in the center.
         if ((livemask & 0x01010101) == 0x00010001 || (livemask & 0x01010101) == 0x01000100)
         return ((place&3) == 3);
      }
      else if (ss->kind == s1x6) {
         p2 = pid2 & (ID2_CTR2|ID2_OUTRPAIRS);
         if      (p2 == ID2_CTR2)      s = selector_center2;
         else if (p2 == ID2_OUTRPAIRS) s = selector_outerpairs;
         else break;
      }
      else if (ss->kind == s2x7 || ss->kind == s_23232 || ss->kind == sdblspindle) {
         if (local_selector == selector_center2)
            return (pid2 & ID2_CTR2) != 0;
         else break;
      }
      else {
         p2 = pid2 & (ID2_CTR2|ID2_OUTR6);
         if      (p2 == ID2_CTR2)  s = selector_center2;
         else if (p2 == ID2_OUTR6) s = selector_outer6;
         else break;
      }
      goto eq_return;
   case selector_center6:
   case selector_outer2:
      p2 = pid2 & (ID2_CTR6|ID2_OUTR2);
      if      (p2 == ID2_CTR6)  s = selector_center6;
      else if (p2 == ID2_OUTR2) s = selector_outer2;
      else break;
      goto eq_return;
   case selector_veryends:    /* Gotta fix this stuff - use fall-through variable. */
      p2 = pid2 & (ID2_CTR6|ID2_OUTR2);
      if      (p2 == ID2_CTR6)  s = selector_center6;
      else if (p2 == ID2_OUTR2) s = selector_veryends;
      else if (ss->kind == s1x12 && local_selector == selector_veryends) return false;  // Close enough.
      else break;
      goto eq_return;
   case selector_ctrdmd:
   case selector_notctrdmd:
      p2 = pid2 & (ID2_CTRDMD|ID2_NCTRDMD);
      if      (p2 == ID2_CTRDMD) s = selector_ctrdmd;
      else if (p2 == ID2_NCTRDMD) s = selector_notctrdmd;
      else break;
      goto eq_return;
   case selector_center_col_of_6:
      // In these three setups, "center column of 6" has a different meaning than the 1x6
      // meaning used in the rest of this program.  It means the same as "center 6".
      // Of course, in a 3x4, the population will have to be correct.
      if (ss->kind == s3x4 || ss->kind == s_qtag || ss->kind == s_spindle) {
         p2 = pid2 & (ID2_CTR6|ID2_OUTR2);
         if      (p2 == ID2_CTR6)  return true;
         else if (p2 == ID2_OUTR2) return false;
         else break;
      }
      // FALL THROUGH!!!
   case selector_ctr_1x6:
   case selector_center_wave_of_6:
   case selector_center_line_of_6:
      // FELL THROUGH!!!
      if      ((pid2 & (ID2_CTR1X6|ID2_NCTR1X6)) == ID2_CTR1X6) return true;
      else if ((pid2 & (ID2_CTR1X6|ID2_NCTR1X6)) == ID2_NCTR1X6) return false;
      break;
   case selector_outer1x3s:
      if      ((pid2 & (ID2_OUTR1X3|ID2_NOUTR1X3)) == ID2_OUTR1X3) return true;
      else if ((pid2 & (ID2_OUTR1X3|ID2_NOUTR1X3)) == ID2_NOUTR1X3) return false;
      break;
   case selector_ctr_1x4:
   case selector_center_wave:
   case selector_center_line:
   case selector_center_col:
      if      ((pid2 & (ID2_CTR1X4|ID2_NCTR1X4)) == ID2_CTR1X4) return true;
      else if ((pid2 & (ID2_CTR1X4|ID2_NCTR1X4)) == ID2_NCTR1X4) return false;
      break;
   case selector_center4:
      if      ((pid2 & (ID2_CTR4|ID2_OUTRPAIRS)) == ID2_CTR4) return true;
      else if ((pid2 & (ID2_CTR4|ID2_OUTRPAIRS)) == ID2_OUTRPAIRS) return false;
      else if ((pid2 & (ID2_CTR4|ID2_END)) == ID2_CTR4) return true;
      else if ((pid2 & (ID2_CTR4|ID2_END)) == ID2_END) return false;
      else if ((pid2 & (ID2_CTR4|ID2_NCTR1X4)) == ID2_CTR4) return true;
      else if ((pid2 & (ID2_CTR4|ID2_NCTR1X4)) == ID2_NCTR1X4) return false;
      break;
   case selector_center_box:
      if      ((pid2 & (ID2_CTR4|ID2_OUTRPAIRS)) == ID2_CTR4) return true;
      else if ((pid2 & (ID2_CTR4|ID2_OUTRPAIRS)) == ID2_OUTRPAIRS) return false;
      else if ((pid2 & (ID2_CTR4|ID2_END)) == ID2_CTR4) return true;
      else if ((pid2 & (ID2_CTR4|ID2_END)) == ID2_END) return false;
      break;
   case selector_outerpairs:
      if (ss->kind == s1x6) {
         if      ((pid2 & (ID2_CTR2  |ID2_OUTRPAIRS)) == ID2_OUTRPAIRS) return true;
         else if ((pid2 & (ID2_CTR2  |ID2_OUTRPAIRS)) == ID2_CTR2) return false;
      }
      else {
         if      ((pid2 & (ID2_CTR4  |ID2_OUTRPAIRS)) == ID2_OUTRPAIRS) return true;
         else if ((pid2 & (ID2_CENTER|ID2_OUTRPAIRS)) == ID2_OUTRPAIRS) return true;
         else if ((pid2 & (ID2_CTR1X4|ID2_OUTRPAIRS)) == ID2_OUTRPAIRS) return true;
         else if ((pid2 & (ID2_CTR4  |ID2_OUTRPAIRS)) == ID2_CTR4) return false;
         else if ((pid2 & (ID2_CENTER|ID2_OUTRPAIRS)) == ID2_CENTER) return false;
         else if ((pid2 & (ID2_CTR1X4|ID2_OUTRPAIRS)) == ID2_CTR1X4) return false;
      }

      break;
   case selector_the2x3:
      switch (ss->kind) {
      case s_spindle:
         selected_person_mask = 0x77;
         break;
      case s_qtag:
         selected_person_mask = 0xBB;
         break;
      case s4x4:
         if (livemask == 0x0F03333FU)
            selected_person_mask = 0xE888;
         else if (livemask == 0x3F0F0333U)
            selected_person_mask = 0x888E;
         else if (livemask == 0x333F0F03U)
            selected_person_mask = 0x88E8;
         else if (livemask == 0x03333F0FU)
            selected_person_mask = 0x8E88;
         break;
      }

      break;
   case selector_thetriangle:
      switch (ss->kind) {
      case s_trngl4:
         selected_person_mask = 0xE;
         break;
      case slinebox:
         selected_person_mask = 0x98;
         break;
      }

      break;
   case selector_thediamond:
      p2 = pid2 & (ID2_CTRDMD|ID2_NCTRDMD);
      if      (p2 == ID2_CTRDMD) return true;
      else if (p2 == ID2_NCTRDMD) return false;

      switch (ss->kind) {
      case s1x3p1dmd:
      case s3p1x1dmd:
         selected_person_mask = 074;
         break;
      case slinefbox:
      case s4p2x1dmd:
         selected_person_mask = 0xD8;
         break;
      case splinepdmd:
      case splinedmd:
         selected_person_mask = 0xF0;
         break;
      case slinepdmd:
      case slinedmd:
      case sboxpdmd:
      case sboxdmd:
         selected_person_mask = 0x0F;
         break;
      case sdbltrngl4:
         selected_person_mask = 0x1E;
         break;
      }

      break;
   case selector_theline:
      switch (ss->kind) {
      case s4x4:
         if (livemask == 0x0F03333FU)
            selected_person_mask = 0x0A84;
         else if (livemask == 0x3F0F0333U)
            selected_person_mask = 0xA840;
         else if (livemask == 0x333F0F03U)
            selected_person_mask = 0x840A;
         else if (livemask == 0x03333F0FU)
            selected_person_mask = 0x40A8;
         break;
      case splinepdmd:
      case splinedmd:
      case slinebox:
         if ((directions & livemask & 0x5500) == 0)
            selected_person_mask = 0x0F;
         break;
      case slinepdmd:
      case slinedmd:
         if ((directions & livemask & 0x0055) == 0)
            selected_person_mask = 0xF0;
         break;
      }

      break;
   case selector_thecolumn:
      switch (ss->kind) {
      case s4x4:
         if (livemask == 0x0F03333FU)
            selected_person_mask = 0x0A84;
         else if (livemask == 0x3F0F0333U)
            selected_person_mask = 0xA840;
         else if (livemask == 0x333F0F03U)
            selected_person_mask = 0x840A;
         else if (livemask == 0x03333F0FU)
            selected_person_mask = 0x40A8;
         break;
      case splinepdmd:
      case splinedmd:
      case slinebox:
         if (((directions ^ 0x5500) & livemask & 0x5500) == 0)
            selected_person_mask = 0x0F;
         break;
      case slinepdmd:
      case slinedmd:
         if (((directions ^ 0x0055) & livemask & 0x0055) == 0)
            selected_person_mask = 0xF0;
         break;
      }

      break;
   case selector_headliners:
      p3 = pid3 & (ID3_FACEFRONT|ID3_FACEBACK|ID3_FACELEFT|ID3_FACERIGHT);
      if      (p3 == ID3_FACEFRONT || p3 == ID3_FACEBACK) return true;
      else if (p3 == ID3_FACELEFT || p3 == ID3_FACERIGHT) return false;
      break;
   case selector_sideliners:
      p3 = pid3 & (ID3_FACEFRONT|ID3_FACEBACK|ID3_FACELEFT|ID3_FACERIGHT);
      if      (p3 == ID3_FACELEFT || p3 == ID3_FACERIGHT) return true;
      else if (p3 == ID3_FACEFRONT || p3 == ID3_FACEBACK) return false;
      break;
   case selector_thosefacing:
      if      ((pid2 & (ID2_FACING|ID2_NOTFACING)) == ID2_FACING) return true;
      else if ((pid2 & (ID2_FACING|ID2_NOTFACING)) == ID2_NOTFACING) return false;
      break;
   case selector_facingfront:
      if      (pid3 & ID3_FACEFRONT) return true;
      else if (pid3 & (ID3_FACEBACK|ID3_FACELEFT|ID3_FACERIGHT)) return false;
      break;
   case selector_facingback:
      if      (pid3 & ID3_FACEBACK) return true;
      else if (pid3 & (ID3_FACEFRONT|ID3_FACELEFT|ID3_FACERIGHT)) return false;
      break;
   case selector_facingleft:
      if      (pid3 & ID3_FACELEFT) return true;
      else if (pid3 & (ID3_FACEFRONT|ID3_FACEBACK|ID3_FACERIGHT)) return false;
      break;
   case selector_facingright:
      if      (pid3 & ID3_FACERIGHT) return true;
      else if (pid3 & (ID3_FACEFRONT|ID3_FACEBACK|ID3_FACELEFT)) return false;
      break;
   case selector_lineleft:
      if      (pid3 & ID3_LEFTLINE) return true;
      else if (pid3 & (ID3_RIGHTLINE|ID3_RIGHTCOL|ID3_RIGHTBOX)) return false;
      break;
   case selector_lineright:
      if      (pid3 & ID3_RIGHTLINE) return true;
      else if (pid3 & (ID3_LEFTLINE|ID3_LEFTCOL|ID3_LEFTBOX)) return false;
      break;
   case selector_columnleft:
      if      (pid3 & ID3_LEFTCOL) return true;
      else if (pid3 & (ID3_RIGHTCOL|ID3_RIGHTLINE|ID3_RIGHTBOX)) return false;
      break;
   case selector_columnright:
      if      (pid3 & ID3_RIGHTCOL) return true;
      else if (pid3 & (ID3_LEFTCOL|ID3_LEFTLINE|ID3_LEFTBOX)) return false;
      break;
   case selector_boxleft:
      if      (pid3 & ID3_LEFTBOX) return true;
      else if (pid3 & (ID3_RIGHTBOX|ID3_RIGHTCOL|ID3_RIGHTLINE)) return false;
      break;
   case selector_boxright:
      if      (pid3 & ID3_RIGHTBOX) return true;
      else if (pid3 & (ID3_LEFTBOX|ID3_LEFTCOL|ID3_LEFTLINE)) return false;
      break;
   case selector_nearline:
      if      (pid3 & ID3_NEARLINE) return true;
      else if (pid3 & (ID3_FARLINE|ID3_FARCOL|ID3_FARBOX|ID3_FARFOUR)) return false;
      break;
   case selector_farline:
      if      (pid3 & ID3_FARLINE) return true;
      else if (pid3 & (ID3_NEARLINE|ID3_NEARCOL|ID3_NEARBOX|ID3_NEARFOUR)) return false;
      break;
   case selector_nearcolumn:
      if      (pid3 & ID3_NEARCOL) return true;
      else if (pid3 & (ID3_FARLINE|ID3_FARCOL|ID3_FARBOX|ID3_FARFOUR)) return false;
      break;
   case selector_farcolumn:
      if      (pid3 & ID3_FARCOL) return true;
      else if (pid3 & (ID3_NEARLINE|ID3_NEARCOL|ID3_NEARBOX|ID3_NEARFOUR)) return false;
      break;
   case selector_nearbox:
      if      (pid3 & ID3_NEARBOX) return true;
      else if (pid3 & (ID3_FARLINE|ID3_FARCOL|ID3_FARBOX|ID3_FARFOUR)) return false;
      break;
   case selector_farbox:
      if      (pid3 & ID3_FARBOX) return true;
      else if (pid3 & (ID3_NEARLINE|ID3_NEARCOL|ID3_NEARBOX|ID3_NEARFOUR)) return false;
      break;
   case selector_nearfour:
      if      (pid3 & ID3_NEARFOUR) return true;
      else if (pid3 & ID3_FARFOUR) return false;
      break;
   case selector_farfour:
      if      (pid3 & ID3_FARFOUR) return true;
      else if (pid3 & ID3_NEARFOUR) return false;
      break;
   case selector_neartwo:
      if (two_couple_calling) {
         if      (pid3 & ID3_NEARTWO) return true;
         else if (pid3 & ID3_FARTWO) return false;
      }
      else {
         if      (pid3 & ID3_NEARTWO) return true;
         else if (pid3 & ID3_FARSIX) return false;
      }
      break;
   case selector_fartwo:
      if (two_couple_calling) {
         if      (pid3 & ID3_FARTWO) return true;
         else if (pid3 & ID3_NEARTWO) return false;
      }
      else {
         if      (pid3 & ID3_FARTWO) return true;
         else if (pid3 & ID3_NEARSIX) return false;
      }
      break;
   case selector_nearsix:
      if      (pid3 & ID3_NEARSIX) return true;
      else if (pid3 & ID3_FARTWO) return false;
      break;
   case selector_farsix:
      if      (pid3 & ID3_FARSIX) return true;
      else if (pid3 & ID3_NEARTWO) return false;
      break;
   case selector_nearthree: case selector_neartriangle:
      if (two_couple_calling) {
         if (local_selector == selector_nearthree || ss->kind == sdmd) {
            if      (pid3 & ID3_NEARTHREE) return true;
            else if (pid3 & ID3_FARTHEST1) return false;
         }
      }
      else {
         if (local_selector == selector_nearthree || (ss->kind == s_ptpd && (ss->rotation & 1)) != 0) {
            if      (pid3 & ID3_NEARTHREE) return true;
            else if (pid3 & ID3_FARFIVE) return false;
         }
      }
      break;
   case selector_farthree: case selector_fartriangle:
      if (two_couple_calling) {
         if (local_selector == selector_farthree || ss->kind == sdmd) {
            if      (pid3 & ID3_FARTHREE) return true;
            else if (pid3 & ID3_NEAREST1) return false;
         }
      }
      else {
         if (local_selector == selector_farthree || (ss->kind == s_ptpd && (ss->rotation & 1)) != 0) {
            if      (pid3 & ID3_FARTHREE) return true;
            else if (pid3 & ID3_NEARFIVE) return false;
         }
      }
      break;
   case selector_nearfive:
      if      (pid3 & ID3_NEARFIVE) return true;
      else if (pid3 & ID3_FARTHREE) return false;
      break;
   case selector_farfive:
      if      (pid3 & ID3_FARFIVE) return true;
      else if (pid3 & ID3_NEARTHREE) return false;
      break;
   case selector_farthest1:
      if (two_couple_calling) {
         if      (pid3 & ID3_FARTHEST1) return true;
         else if (pid3 & ID3_NEARTHREE) return false;
      }
      else {
         if      (pid3 & ID3_FARTHEST1) return true;
         else if (pid3 & ID3_NOTFARTHEST1) return false;
      }
      break;
   case selector_nearseven:
      if      (pid3 & ID3_NOTFARTHEST1) return true;
      else if (pid3 & ID3_FARTHEST1) return false;
      break;
   case selector_nearest1:
      if (two_couple_calling) {
         if      (pid3 & ID3_NEAREST1) return true;
         else if (pid3 & ID3_FARTHREE) return false;
      }
      else {
         if      (pid3 & ID3_NEAREST1) return true;
         else if (pid3 & ID3_NOTNEAREST1) return false;
      }
      break;
   case selector_farseven:
      if      (pid3 & ID3_NOTNEAREST1) return true;
      else if (pid3 & ID3_NEAREST1) return false;
      break;

      // For the unsymmetrical selectors, we demand that the person not be virtual
      // (e.g. tandem person) or active phantom.  The former requirement could cause
      // us to lose a few extremely rare cases, but it isn't worth using up zillions of bits.

   case selector_boy1:
      if (!(pid1 & (XPID_MASK & ~PID_MASK))) return (pid1 & PID_MASK) == 0000;
      break;
   case selector_girl1:
      if (!(pid1 & (XPID_MASK & ~PID_MASK))) return (pid1 & PID_MASK) == 0100;
      break;
   case selector_cpl1:
      if (!(pid1 & (XPID_MASK & ~PID_MASK))) return (pid1 & PID_MASK & ~0100) == 0000;
      break;
   case selector_boy2:
      if (!(pid1 & (XPID_MASK & ~PID_MASK))) return (pid1 & PID_MASK) == 0200;
      break;
   case selector_girl2:
      if (!(pid1 & (XPID_MASK & ~PID_MASK))) return (pid1 & PID_MASK) == 0300;
      break;
   case selector_cpl2:
      if (!(pid1 & (XPID_MASK & ~PID_MASK))) return (pid1 & PID_MASK & ~0100) == 0200;
      break;
   case selector_boy3:
      if (!(pid1 & (XPID_MASK & ~PID_MASK))) return (pid1 & PID_MASK) == 0400;
      break;
   case selector_girl3:
      if (!(pid1 & (XPID_MASK & ~PID_MASK))) return (pid1 & PID_MASK) == 0500;
      break;
   case selector_cpl3:
      if (!(pid1 & (XPID_MASK & ~PID_MASK))) return (pid1 & PID_MASK & ~0100) == 0400;
      break;
   case selector_boy4:
      if (!(pid1 & (XPID_MASK & ~PID_MASK))) return (pid1 & PID_MASK) == 0600;
      break;
   case selector_girl4:
      if (!(pid1 & (XPID_MASK & ~PID_MASK))) return (pid1 & PID_MASK) == 0700;
      break;
   case selector_cpl4:
      if (!(pid1 & (XPID_MASK & ~PID_MASK))) return (pid1 & PID_MASK & ~0100) == 0600;
      break;
   case selector_cpls1_2:
      if (!(pid1 & (XPID_MASK & ~PID_MASK)))
         return (pid1 & PID_MASK & ~0300) == 0000;
      break;
   case selector_cpls2_3:
      if (!(pid1 & (XPID_MASK & ~PID_MASK)))
         return ((pid1+0200) & PID_MASK & ~0300) == 0400;
      break;
   case selector_cpls3_4:
      if (!(pid1 & (XPID_MASK & ~PID_MASK)))
         return (pid1 & PID_MASK & ~0300) == 0400;
      break;
   case selector_cpls4_1:
      if (!(pid1 & (XPID_MASK & ~PID_MASK)))
         return ((pid1+0200) & PID_MASK & ~0300) == 0000;
      break;

   case selector_firstone:
   case selector_lastone:
   case selector_firsttwo:
   case selector_lasttwo:
   case selector_firstthree:
   case selector_lastthree:
      if (ss->kind == s2x4) {
         if (directions == (livemask & 0x55FF))
            thing_to_test = place & 3;        // Right column.
         else if (directions == (livemask & 0xFF55))
            thing_to_test = 3-(place & 3);    // Left column.
         else
            break;

         goto first_last_test;
      }
      else if (ss->kind == s1x8) {
         int q = place ^ ((place & 3) >> 1);
         if (directions == (livemask & 0x55FF))
            thing_to_test = q & 3;        // Facing center.
         else if (directions == (livemask & 0xFF55))
            thing_to_test = 3-(q & 3);    // Facing away from center.
         else
            break;

         goto first_last_test;
      }
      break;

   case selector_leftmostone:
   case selector_rightmostone:
   case selector_leftmosttwo:
   case selector_rightmosttwo:
   case selector_leftmostthree:
   case selector_rightmostthree:
      if (ss->kind == s2x4) {
         if (directions == (livemask & 0x00AA))
            thing_to_test = place & 3;        // Lines out.
         else if (directions == (livemask & 0xAA00))
            thing_to_test = 3-(place & 3);    // Lines in.
         else
            break;

         goto first_last_test;
      }
      else if (ss->kind == s1x8) {
         int q = place ^ ((place & 3) >> 1);
         if (directions == (livemask & 0x00AA))
            thing_to_test = q & 3;        // Left as couples 2FL.
         else if (directions == (livemask & 0xAA00))
            thing_to_test = 3-(q & 3);    // Right as couples 2FL.
         else
            break;

         goto first_last_test;
      }
      break;

   case selector_firstfour:
   case selector_lastfour:
   case selector_leftmostfour:
   case selector_rightmostfour:
      fail("This selector picks people, in a 1x4, relative to their own frame of reference, " \
           "which you can't do.  Perhaps you mean something like \"line on the caller's left\".");

   case selector_inpoint_tgl:
   case selector_inpoint_intlk_tgl:
   case selector_intlk_inpoint_tgl:
   case selector_magic_inpoint_tgl:
   case selector_magic_intlk_inpoint_tgl:
   case selector_intlk_magic_inpoint_tgl:
      if ((ss->people[0].id1 & d_mask) == d_east) thing_to_test |= 1;
      if ((ss->people[4].id1 & d_mask) == d_west) thing_to_test |= 1;
      if ((ss->people[1].id1 & d_mask) == d_west) thing_to_test |= 2;
      if ((ss->people[5].id1 & d_mask) == d_east) thing_to_test |= 2;
      goto finish_inoutpoint;
   case selector_outpoint_tgl:
   case selector_outpoint_intlk_tgl:
   case selector_intlk_outpoint_tgl:
   case selector_magic_outpoint_tgl:
   case selector_magic_intlk_outpoint_tgl:
   case selector_intlk_magic_outpoint_tgl:
      if ((ss->people[0].id1 & d_mask) == d_west) thing_to_test |= 1;
      if ((ss->people[4].id1 & d_mask) == d_east) thing_to_test |= 1;
      if ((ss->people[1].id1 & d_mask) == d_east) thing_to_test |= 2;
      if ((ss->people[5].id1 & d_mask) == d_west) thing_to_test |= 2;
      goto finish_inoutpoint;
   case selector_beaupoint_tgl:
   case selector_beaupoint_intlk_tgl:
   case selector_intlk_beaupoint_tgl:
   case selector_magic_beaupoint_tgl:
   case selector_magic_intlk_beaupoint_tgl:
   case selector_intlk_magic_beaupoint_tgl:
      if ((ss->people[0].id1 & d_mask) == d_east) thing_to_test |= 1;
      if ((ss->people[4].id1 & d_mask) == d_west) thing_to_test |= 1;
      if ((ss->people[1].id1 & d_mask) == d_east) thing_to_test |= 2;
      if ((ss->people[5].id1 & d_mask) == d_west) thing_to_test |= 2;
      if ((ss->people[0].id1 & d_mask) == d_north) other_other_thing_to_test |= 1;
      if ((ss->people[4].id1 & d_mask) == d_south) other_other_thing_to_test |= 1;
      if ((ss->people[6].id1 & d_mask) == d_north) other_other_thing_to_test |= 2;
      if ((ss->people[2].id1 & d_mask) == d_south) other_other_thing_to_test |= 2;
      if ((ss->people[0].id1 & d_mask) == d_north) other_thing_to_test |= 1;
      if ((ss->people[2].id1 & d_mask) == d_south) other_thing_to_test |= 2;
      goto finish_inoutpoint;
   case selector_bellepoint_tgl:
   case selector_bellepoint_intlk_tgl:
   case selector_intlk_bellepoint_tgl:
   case selector_magic_bellepoint_tgl:
   case selector_magic_intlk_bellepoint_tgl:
   case selector_intlk_magic_bellepoint_tgl:
      if ((ss->people[0].id1 & d_mask) == d_west) thing_to_test |= 1;
      if ((ss->people[4].id1 & d_mask) == d_east) thing_to_test |= 1;
      if ((ss->people[1].id1 & d_mask) == d_west) thing_to_test |= 2;
      if ((ss->people[5].id1 & d_mask) == d_east) thing_to_test |= 2;
      if ((ss->people[0].id1 & d_mask) == d_south) other_other_thing_to_test |= 1;
      if ((ss->people[4].id1 & d_mask) == d_north) other_other_thing_to_test |= 1;
      if ((ss->people[6].id1 & d_mask) == d_south) other_other_thing_to_test |= 2;
      if ((ss->people[2].id1 & d_mask) == d_north) other_other_thing_to_test |= 2;
      if ((ss->people[0].id1 & d_mask) == d_south) other_thing_to_test |= 1;
      if ((ss->people[2].id1 & d_mask) == d_north) other_thing_to_test |= 2;
   finish_inoutpoint:
      if (ss->kind == s_qtag) {
         if (thing_to_test == 1)
            return (((0xDD >> place) & 1) != 0);
         else if (thing_to_test == 2)
            return (((0xEE >> place) & 1) != 0);
      }
      else if (ss->kind == s_ptpd) {
         if (other_other_thing_to_test == 1)
            return (((0xBB >> place) & 1) != 0);
         else if (other_other_thing_to_test == 2)
            return (((0xEE >> place) & 1) != 0);
      }
      else if (ss->kind == sdmd) {
         if (other_thing_to_test == 1)
            return (((0xB >> place) & 1) != 0);
         else if (other_thing_to_test == 2)
            return (((0xE >> place) & 1) != 0);
      }

      fail("Can't find designated point.");
   case selector_inside_intlk_tgl:
   case selector_intlk_inside_tgl:
   case selector_inside_tgl:
      p2 = pid2 & (ID2_CTR6|ID2_OUTR2);
      if (ss->kind == sbigdmd)
         return (((03636 >> place) & 1) != 0);
      else if (ss->kind == sd2x7)
         return (((0x1F3E >> place) & 1) != 0);
      else if (p2 == ID2_CTR6) return true;
      else if (p2 == ID2_OUTR2) return false;
      break;

   case selector_outside_intlk_tgl:
   case selector_intlk_outside_tgl:
      if (ss->kind == s_rigger)
         return (place != 3 && place != 7);
      // FALL THROUGH!!!
   case selector_outside_tgl:
      // FELL THROUGH!!!
      p2 = pid2 & (ID2_CTR2|ID2_OUTR6);
      if      (p2 == ID2_OUTR6) return true;
      else if (p2 == ID2_CTR2) return false;
      break;

   case selector_tand_base_tgl:
   case selector_tand_base_intlk_tgl:
   case selector_intlk_tand_base_tgl:
      tand_base = 1;
      // FALL THROUGH!!!
   case selector_wave_base_tgl:
   case selector_wave_base_intlk_tgl:
   case selector_intlk_wave_base_tgl:
      // FELL THROUGH!!!

      switch (ss->kind) {
      case s_bone:
         thing_to_test = ss->people[0].id1 | ss->people[1].id1 | ss->people[4].id1 | ss->people[5].id1;
         if ((thing_to_test & 011) != 011 && ((tand_base ^ thing_to_test) & 1))
            return (((0x77 >> place) & 1) != 0);
         break;
      case s_bone6:
         thing_to_test = ss->people[0].id1 | ss->people[1].id1 | ss->people[3].id1 | ss->people[4].id1;
         if ((thing_to_test & 011) != 011 && ((tand_base ^ thing_to_test) & 1))
            return true;
         break;
      case s_rigger:
         thing_to_test = ss->people[0].id1 | ss->people[1].id1 | ss->people[4].id1 | ss->people[5].id1;
         if ((thing_to_test & 011) != 011 && ((tand_base ^ thing_to_test) & 1))
            return (((0xBB >> place) & 1) != 0);
         break;
      case s_short6:
         thing_to_test = ss->people[0].id1 | ss->people[2].id1 | ss->people[3].id1 | ss->people[5].id1;
         if ((thing_to_test & 011) != 011 && !((tand_base ^ thing_to_test) & 1))
            return true;
         break;
      case s_galaxy:
         thing_to_test = ss->people[1].id1 | ss->people[3].id1 | ss->people[5].id1 | ss->people[7].id1;
         if ((thing_to_test & 011) != 011) {
            if ((tand_base ^ thing_to_test) & 1)
               return (((0xBB >> place) & 1) != 0);
            else
               return (((0xEE >> place) & 1) != 0);
         }
         break;
      case s_hrglass:
         thing_to_test = ss->people[0].id1 | ss->people[1].id1 | ss->people[4].id1 | ss->people[5].id1;
         if ((thing_to_test & 011) != 011 && !((tand_base ^ thing_to_test) & 1))
            return (((0xBB >> place) & 1) != 0);
         break;
      case sbigdmd:
         thing_to_test = ss->people[2].id1 | ss->people[3].id1 | ss->people[8].id1 | ss->people[9].id1;
         if ((thing_to_test & 011) != 011 && ((tand_base ^ thing_to_test) & 1)) {
            // Do top and bottom halves separately, of course.
            uint32_t testbits = 0;
            if (ss->people[1].id1 != 0 && ss->people[4].id1 == 0)
               testbits = 0x00E;
            else if (ss->people[4].id1 != 0 && ss->people[1].id1 == 0)
               testbits = 0x01C;
            else
               break;

            if (ss->people[10].id1 != 0 && ss->people[7].id1 == 0)
               testbits |= 0x700;
            else if (ss->people[7].id1 != 0 && ss->people[10].id1 == 0)
               testbits |= 0x380;
            else
               break;

            return (((testbits >> place) & 1) != 0);
         }
         break;
      case s_ntrglcw:
      case s_nptrglcw:
         thing_to_test = ss->people[2].id1 | ss->people[3].id1 | ss->people[6].id1 | ss->people[7].id1;
         if ((thing_to_test & 011) != 011 && !((tand_base ^ thing_to_test) & 1))
            return (((0xDD >> place) & 1) != 0);
         break;
      case s_ntrglccw:
      case s_nptrglccw:
         thing_to_test = ss->people[0].id1 | ss->people[1].id1 | ss->people[4].id1 | ss->people[5].id1;
         if ((thing_to_test & 011) != 011 && !((tand_base ^ thing_to_test) & 1))
            return (((0xBB >> place) & 1) != 0);
         break;
      case s_nxtrglcw:
         thing_to_test = ss->people[1].id1 | ss->people[2].id1 | ss->people[5].id1 | ss->people[6].id1;
         if ((thing_to_test & 011) != 011 && !((tand_base ^ thing_to_test) & 1))
            return (((0x77 >> place) & 1) != 0);
         break;
      case s_nxtrglccw:
         thing_to_test = ss->people[0].id1 | ss->people[1].id1 | ss->people[4].id1 | ss->people[5].id1;
         if ((thing_to_test & 011) != 011 && !((tand_base ^ thing_to_test) & 1))
            return (((0x77 >> place) & 1) != 0);
         break;
      case s_ntrgl6cw:
         thing_to_test = ss->people[1].id1 | ss->people[2].id1 | ss->people[4].id1 | ss->people[5].id1;
         if ((thing_to_test & 011) != 011 && !((tand_base ^ thing_to_test) & 1))
            return true;
         break;
      case s_ntrgl6ccw:
         thing_to_test = ss->people[0].id1 | ss->people[1].id1 | ss->people[3].id1 | ss->people[4].id1;
         if ((thing_to_test & 011) != 011 && !((tand_base ^ thing_to_test) & 1))
            return true;
         break;
      case s_323:
         // Apparently we can't do plain "wave-based" or "tandem-based" here.
         break;
      case s_c1phan:
         if ((livemask & ~directions & 0x55555555) == 0)
            tand_base ^= 1;   // There is no one facing NS
         else if ((livemask & directions & 0x55555555) != 0)
            break;           // There are people facing both ways; not legal.

         // Now tand_base is 1 to select the triangles whose bases are
         // vertically aligned, and 0 for the horizontally aligned bases.

         if (local_selector == selector_tand_base_tgl ||
             local_selector == selector_wave_base_tgl) {
            if (tand_base != 0)
               return (((0xDEDE >> place) & 1) != 0);
            else
               return (((0xEDED >> place) & 1) != 0);
         }
         else {
            if (tand_base != 0)
               return (((0x7B7B >> place) & 1) != 0);
            else
               return (((0xB7B7 >> place) & 1) != 0);
         }
      case sdeepbigqtg:
         thing_to_test = ss->people[4].id1 | ss->people[5].id1 | ss->people[6].id1 | ss->people[7].id1 |
            ss->people[12].id1 | ss->people[13].id1 | ss->people[14].id1 | ss->people[15].id1;
         if (!(thing_to_test & ((tand_base & 1) ? 010 : 1)))
            return (((0xFCFC >> place) & 1) != 0);
         break;
      }

      fail("Can't do this concept in this setup.");

   case selector_anyone_base_tgl:
   case selector_anyone_base_intlk_tgl:
   case selector_intlk_anyone_base_tgl:
      fail("Can't do this concept in this setup.");

   case selector_some:
      // We have to figure out how to group the people, based on unambiguous information from facing directions.
      thing_to_test = -1;

      switch (attr::slimit(ss)) {
         uint32_t A, B, C, D, E, F;
      case 5:
         A = (directions >> 4) & 00303;
         B = (directions >> 2) & 00303;
         C = (directions >> 0) & 00303;

         if (allow_some == 2) {
            switch (ss->kind) {
            case s_1x2dmd:
               if (A == B) thing_to_test = 0x3;
               break;
            case s_short6:
               if (A == C) thing_to_test = 0x5;
               break;
            case s1x6:
               if (A == B && B != C) thing_to_test = 0x3;
               else if (B == C && A != B) thing_to_test = 0x6;
               break;
            case s2x3:
               if (A == B && B != C) thing_to_test = 0x3;
               else if (B == C && A != B) thing_to_test = 0x6;
               break;
            }
         }
         else if (allow_some == 3 && (ss->kind == s1x6 || ss->kind == s2x3)) {
            if (livemask == 0xFCC) {
               directions >>= 6;
               place += 3;
            }
            else if (livemask != 0x33F)
               break;    // Will fail.

            // Have 3x1 triangle of some sort.
            if ((directions & 3) == ((directions>>2) & 3) &&
                (directions & 3) == ((directions>>4) & 3))
               // All 3 facing same way.
               return (place >= 3 && place < 6);
            else
               break;    // Will fail.
         }

         if (thing_to_test != -1) return ((thing_to_test >> (place % 3)) & 1) != 0;
         break;
      case 7:
         A = (directions >> 6) & 0x0303;
         B = (directions >> 4) & 0x0303;
         C = (directions >> 2) & 0x0303;
         D = (directions >> 0) & 0x0303;

         if (allow_some == 2) {
            switch (ss->kind) {
            case s1x3dmd:
               if (A == B && B != C) thing_to_test = 0x33;
               else if (B == C && A != B) thing_to_test = 0x66;
               break;
            case s1x8:
               if (A == B && B != D && C != D) thing_to_test = 0x33;
               else if (B == D && C != D && A != B) thing_to_test = 0xAA;
               else if (C == D && B != D && A != B) thing_to_test = 0xCC;
               break;
            case s2x4:
               if (A == B && B != C && C != D) thing_to_test = 0x33;
               else if (B == C && C != D && A != B) thing_to_test = 0x66;
               else if (C == D && B != C && A != B) thing_to_test = 0xCC;
               break;
            case s_ptpd:
               if (B == D) thing_to_test = 0xAA;
               break;
            case s_bone:
               if ((C ^ D) == 0x202 && (A ^ B) == 0x202) thing_to_test = 0x33;
               break;
            case s_qtag:
               if (C == D && (A ^ B) == 0x202) thing_to_test = 0xCC;
               break;
            case s_rigger:
               if ((C ^ D) == 0x202 && (A ^ B) == 0x202) thing_to_test = 0x33;
               break;
            }
         }
         else if (allow_some == 3) {
            switch (ss->kind) {
            case s_323:
            case s3x1dmd:
            case s1x3dmd:
               if (A == B && B == C) thing_to_test = 0x77;
               break;
            case s_qtag:
               if ((A ^ 0x0202) == B && B == D) thing_to_test = 0xBB;
               break;
            case s1x8:
               if (A == B && B == D && C != D) thing_to_test = 0xBB;
               else if (B == C && C == D && A != B) thing_to_test = 0xEE;
               else if (A == 0x200 && B == 0x000 && C == 0x002 && D == 0x000) thing_to_test = 0xBE;
               else if (A == 0x002 && B == 0x202 && C == 0x200 && D == 0x202) thing_to_test = 0xBE;
               else if (A == 0x002 && B == 0x000 && C == 0x200 && D == 0x000) thing_to_test = 0xEB;
               else if (A == 0x200 && B == 0x202 && C == 0x002 && D == 0x202) thing_to_test = 0xEB;
               break;
            case s2x4:
               if (A == B && B == C && C != D) thing_to_test = 0x77;
               else if (B == C && C == D && A != B) thing_to_test = 0xEE;
               else if (A == 0x200 && B == 0x000 && C == 0x000 && D == 0x002) thing_to_test = 0x7E;
               else if (A == 0x002 && B == 0x202 && C == 0x202 && D == 0x200) thing_to_test = 0x7E;
               else if (A == 0x002 && B == 0x000 && C == 0x000 && D == 0x200) thing_to_test = 0xE7;
               else if (A == 0x200 && B == 0x202 && C == 0x202 && D == 0x002) thing_to_test = 0xE7;
               break;
            }
         }

         if (thing_to_test != -1) return ((thing_to_test >> place) & 1) != 0;
         break;
      case 11:
         A = (directions >> 10) & 0x003003;
         B = (directions >> 8) & 0x003003;
         C = (directions >> 6) & 0x003003;
         D = (directions >> 4) & 0x003003;
         E = (directions >> 2) & 0x003003;
         F = (directions >> 0) & 0x003003;

         if (allow_some == 3) {
            switch (ss->kind) {
            case s3x4:
               if (livemask == 0xC3FC3F && (A ^ 0x002002) == D && D == E) thing_to_test = 0x19;
               else if (livemask == 0x3CF3CF && (B ^ 0x002002) == C && C == F) thing_to_test = 0x26;
               break;
            }
         }

         if (thing_to_test != -1) return ((thing_to_test >> (place % 6)) & 1) != 0;
         break;
      }

      break;

   default:
      fail("INTERNAL ERROR - selector failed to get initialized.");
   }

   if (selected_person_mask != ~0U) {
      if (selected_person_mask & (1 << place)) return true;
      else return false;
   }

   fail("Can't decide who are selected.");

 eq_return:
   return (local_selector == s);

 first_last_test:
   switch (local_selector) {
   case selector_firstone: case selector_rightmostone:
      return thing_to_test >= 3;
   case selector_firsttwo: case selector_rightmosttwo:
      return thing_to_test >= 2;
   case selector_firstthree: case selector_rightmostthree:
      return thing_to_test >= 1;
   case selector_lastone: case selector_leftmostone:
      return thing_to_test < 1;
   case selector_lasttwo: case selector_leftmosttwo:
      return thing_to_test < 2;
   case selector_lastthree: case selector_leftmostthree:
      return thing_to_test < 3;
   }

   fail("Can't decide who are selected.");   // Won't happen.
}


static const int32_t iden_tab[25]        = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                                            14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
static const int32_t dbl_tab01[5]        = {0, 1, 0, 1, 0};
static const int32_t dbl_tab21[5]        = {2, 1, 1, 0, 0};
static const int32_t dbl_tab01n[5]       = {0, 1, 0, 1, 1};
static const int32_t dbl_tab21n[5]       = {2, 1, 1, 0, 1};
static const int32_t x22tabtandem[4]     = {3, 0, 1, 0};
static const int32_t x22tabantitandem[4] = {3, 2, 1, 0};
static const int32_t x22tabfacing[4]     = {3, 2, 1, 0x1B};
static const int32_t x24tabtandem[4]     = {7, 0, 1, 0};
static const int32_t x24tabantitandem[4] = {7, 2, 1, 0};
static const int32_t x24tabfacing[4]     = {7, 2, 1, 0x1B1B};

static const int32_t boystuff_no_rh[3]   = {ID2_PERM_BOY,  ID2_PERM_GIRL, 0};
static const int32_t girlstuff_no_rh[3]  = {ID2_PERM_GIRL, ID2_PERM_BOY,  0};
static const int32_t boystuff_rh[3]      = {ID2_PERM_BOY,  ID2_PERM_GIRL, 1};
static const int32_t girlstuff_rh[3]     = {ID2_PERM_GIRL, ID2_PERM_BOY,  1};
static const int32_t samesex[2]          = {0, ID2_PERM_BOY|ID2_PERM_GIRL};
static const int32_t semi_squeeze_tab[8] = {0xD, 0xE, 0x9, 0x9, 0x2, 0xD, 0x2, 0xE};


// Here are the predicates.  They will get put into the array "pred_table".

static bool someone_selected(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   return selectp(real_people, real_index ^ (*extra_stuff));
}

static bool sum_mod_real(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   int otherindex = (*extra_stuff) - real_index;
   int size = attr::slimit(real_people)+1;
   if (otherindex >= size) otherindex -= size;
   else if (otherindex < 0) otherindex += size;
   return real_people->people[otherindex].id1 != 0;
}

static bool sum_mod_selected(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   int otherindex = (*extra_stuff) - real_index;
   int size = attr::slimit(real_people)+1;
   if (otherindex >= size) otherindex -= size;
   else if (otherindex < 0) otherindex += size;
   return selectp(real_people, otherindex);
}



static bool plus_mod_selected(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   int size = attr::slimit(real_people)+1;
   int otherindex = ((real_people->kind == s1x3) && (real_direction & 2)) ?
      real_index + size - (*extra_stuff) :
      real_index + (*extra_stuff);
   if (otherindex >= size) otherindex -= size;
   return selectp(real_people, otherindex);
}

static bool plus_mod_selected_real(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   int size = attr::slimit(real_people)+1;
   int otherindex = ((real_people->kind == s1x3) && (real_direction & 2)) ?
      real_index + size - (*extra_stuff) :
      real_index + (*extra_stuff);
   if (otherindex >= size) otherindex -= size;
   return real_people->people[otherindex].id1 && selectp(real_people, otherindex);
}

static bool plus_mod_real(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   int size = attr::slimit(real_people)+1;
   int otherindex = ((real_people->kind == s1x3) && (real_direction & 2)) ?
      real_index + size - (*extra_stuff) :
      real_index + (*extra_stuff);
   if (otherindex >= size) otherindex -= size;
   return real_people->people[otherindex].id1 != 0;
}



/* ARGSUSED */
static bool semi_squeezer_select(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   int other_index = ((northified_index ^ extra_stuff[northified_index&7]) + real_index - northified_index) & 0xF;

   return (real_people->people[other_index].id1 & BIT_PERSON) &&
          selectp(real_people, other_index);
}

/* ARGSUSED */
static bool unselect(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   return !selectp(real_people, real_index);
}

/* ARGSUSED */
static bool select_near_select(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   if (!selectp(real_people, real_index)) return false;
   if (current_options.who.who[0] == selector_all || current_options.who.who[0] == selector_everyone)
      return true;

   // We generally try to make "near" mean lateral; we usually get it right with no extra
   // fussing.  But in the case of a 2x2 with people facing sideways, it takes extra work.
   int other_index = (real_people->kind == s2x2 && (real_direction & 1)) ?
      real_index ^ 3 : real_index ^ 1;

   return (real_people->people[other_index].id1 & BIT_PERSON) &&
          selectp(real_people, other_index);
}

static int base_table[12] = {0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};

/* ARGSUSED */
static bool select_near_select_or_phantom(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   int lim, base_person;

   if (!selectp(real_people, real_index))
      return false;
   else if (current_options.who.who[0] == selector_all || current_options.who.who[0] == selector_everyone)
      return true;

   if (attr::slimit(real_people) == 11) {
      base_person = base_table[real_index];
      lim = 2;
   }
   if (attr::slimit(real_people) == 15) {
      base_person = real_index & 4;
      lim = 3;
   }
   else {
      return
         !(real_people->people[real_index ^ 1].id1 & BIT_PERSON) ||
         selectp(real_people, real_index ^ 1);
   }

   for ( ; lim >= 0 ; lim--) {
      if ((real_people->people[base_person+lim].id1 & BIT_PERSON) &&
          !selectp(real_people, base_person+lim))
         return false;
   }

   return true;
}

/* ARGSUSED */
static bool select_near_unselect(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   if (!selectp(real_people, real_index)) return false;
   if (current_options.who.who[0] == selector_all || current_options.who.who[0] == selector_everyone)
      return false;

   return !(real_people->people[real_index ^ 1].id1 & BIT_PERSON) ||
          !selectp(real_people, real_index ^ 1);
}

/* ARGSUSED */
static bool unselect_near_select(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   if (selectp(real_people, real_index)) return false;
   if (current_options.who.who[0] == selector_none) return false;

   return      (real_people->people[real_index ^ 1].id1 & BIT_PERSON) &&
               selectp(real_people, real_index ^ 1);
}

/* ARGSUSED */
static bool unselect_near_unselect(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   int lim, base_person;

   if (selectp(real_people, real_index)) return false;
   else if (current_options.who.who[0] == selector_none) return true;

   if (attr::slimit(real_people) == 11) {
      base_person = base_table[real_index];
      lim = 2;
   }
   if (attr::slimit(real_people) == 15) {
      base_person = real_index & 4;
      lim = 3;
   }
   else {
      return
         !(real_people->people[real_index ^ 1].id1 & BIT_PERSON) ||
         !selectp(real_people, real_index ^ 1);
   }

   for ( ; lim >= 0 ; lim--) {
      if ((real_people->people[base_person+lim].id1 & BIT_PERSON) &&
          selectp(real_people, base_person+lim))
         return false;
   }

   return true;
}

/* ARGSUSED */
static bool select_once_rem_from_select(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   if (!selectp(real_people, real_index)) return false;
   if (current_options.who.who[0] == selector_all || current_options.who.who[0] == selector_everyone)
      return true;

   if (real_people->kind == s2x4)
      return (real_people->people[real_index ^ 2].id1 & BIT_PERSON) &&
             selectp(real_people, real_index ^ 2);
   else
      return (real_people->people[real_index ^ 3].id1 & BIT_PERSON) &&
             selectp(real_people, real_index ^ 3);
}

/* ARGSUSED */
static bool select_once_rem_from_unselect(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   if (!selectp(real_people, real_index)) return false;
   if (current_options.who.who[0] == selector_all || current_options.who.who[0] == selector_everyone)
      return false;

   int other_index;

   if (real_people->kind == s2x6) {
      int q = (real_index >= 6) ? real_index-6 : real_index;
      other_index = real_index + 2;
      if (q >= 4) other_index = real_index - 2;
      else if (q >= 2) {
         // We demand BOTH outer people be (nonexistent or unselected).
         if ((real_people->people[real_index - 2].id1 & BIT_PERSON) &&
             selectp(real_people, real_index - 2))
            return false;
      }
   }
   else if (real_people->kind == s2x4)
      other_index = real_index ^ 2;
   else
      other_index = real_index ^ 3;

   return !(real_people->people[other_index].id1 & BIT_PERSON) ||
          !selectp(real_people, other_index);
}

/* ARGSUSED */
static bool unselect_once_rem_from_select(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   if (selectp(real_people, real_index)) return false;
   if (current_options.who.who[0] == selector_none) return false;

   if (real_people->kind == s2x4)
      return   (real_people->people[real_index ^ 2].id1 & BIT_PERSON) &&
               selectp(real_people, real_index ^ 2);
   else
      return   (real_people->people[real_index ^ 3].id1 & BIT_PERSON) &&
               selectp(real_people, real_index ^ 3);
}

/* ARGSUSED */
static bool select_and_roll_is_cw(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   return selectp(real_people, real_index) && (real_people->people[real_index].id1 & ROLL_DIRMASK) == ROLL_IS_R;
}

/* ARGSUSED */
static bool select_and_roll_is_ccw(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   return selectp(real_people, real_index) && (real_people->people[real_index].id1 & ROLL_DIRMASK) == ROLL_IS_L;
}

/* ARGSUSED */
static bool always(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   return true;
}

/* ARGSUSED */
static bool x22_cpltest(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   if (extra_stuff[4]) warn(warn_suspect_destroyline);
   int other_index;

   switch (real_people->cmd.cmd_assume.assumption) {
   case cr_wave_only: case cr_miniwaves: return extra_stuff[2] != 0;
   case cr_2fl_only: case cr_couples_only: case cr_li_lo: return extra_stuff[3] != 0;
   }

   other_index = real_index ^ (((real_direction << 1) & 2) ^ extra_stuff[1]);
   return ((real_people->people[real_index].id1 ^
            real_people->people[other_index].id1) & DIR_MASK) == (uint32_t) *extra_stuff;
}

/* ARGSUSED */
static bool whos_on_base(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   if ((real_people->result_flags.misc & RESULTFLAG__IMPRECISE_ROT) ||
       real_people->eighth_rotation != 0)
      fail("Rotation is imprecise or is 45 degrees.");

   if (real_people->kind == s_alamo)
      real_index = 12+(real_index<<1);

   uint32_t N = current_options.number_fields & NUMBER_FIELD_MASK;
   if (N < 1 || N > 4) fail("Number must be between 1 and 4.");
   return ((((real_index + *extra_stuff) >> 2) + N) & 3) == 0;
}

/* ARGSUSED */
static bool facing_test(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   // If the "trailer only" word is nonzero, figure out whether person is a trailer.
   // The word has 2-bit fields in little-endian order, indexed by the person's
   // position.  That 2-bit field is added to the direction, and the result must
   // have the "2" bit off.
   if (extra_stuff[3] &&
       (((extra_stuff[3] >> (real_index*2)) + real_direction) & 2))
      return false;

   int other_index = real_index ^ extra_stuff[(real_direction << 1) & 2];
   return ((real_people->people[real_index].id1 ^
            real_people->people[other_index].id1) & DIR_MASK) == (uint32_t) extra_stuff[1];
}

/* ARGSUSED */
static bool x14_side_of_line_facing(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int end_of_indicated_side = (real_index ^ (extra_stuff[0] << 1)) & 2;
   int indicated_position = end_of_indicated_side + (extra_stuff[0] >> 2);

   if (extra_stuff[0] & 8) {
      // This is "1x4_selectee_of_ ..." rather than "1x4_end_of_ ..." or "1x4_center_of_ ..."
      bool z0 = selectp(real_people, end_of_indicated_side);
      bool z1 = selectp(real_people, end_of_indicated_side+1);
      if (z0 && z1) return false;
      if (!z0 && !z1) return false;
      indicated_position = end_of_indicated_side + (z1 ? 1 : 0);
   }

   uint32_t person_of_interest = real_people->people[indicated_position].id1;
   if (!(person_of_interest & 8)) return false;

   return ((indicated_position ^ person_of_interest ^ extra_stuff[0]) & 2) == 0;
}

/* ARGSUSED */
static bool x12_side_of_line_facing(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   bool z0 = selectp(real_people, 0);
   bool z1 = selectp(real_people, 1);
   if (z0 && z1) return false;
   if (!z0 && !z1) return false;
   int selected_position = z1 ? 1 : 0;

   uint32_t selectee = real_people->people[selected_position].id1;
   if (!(selectee & 8)) return false;

   return (((selected_position << 1) ^ selectee ^ extra_stuff[0]) & 2) == 0;
}

/* ARGSUSED */
static bool kicker_coming(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   if (real_people->kind == s1x2 || real_people->kind == s_qtag) {
      if ((real_people->people[real_index^1].id1 & 8) != 0 &&
          selectp(real_people, real_index^1))
         return true;
   }
   else if (real_people->kind == s_spindle) {
      if ((real_index & 3) != 3 &&
          (real_people->people[6-real_index].id1 & 1) != 0 &&
          selectp(real_people, 6-real_index))
         return true;
   }
   else if (real_people->kind == sdmd) {
      if ((real_index & 1) != 0 &&
          (real_people->people[real_index^2].id1 & 1) != 0 &&
          selectp(real_people, real_index^2))
         return true;
   }
   else {
      if ((real_people->people[(real_index+1)&3].id1 & (1 << (3-((real_index&1)*3)))) != 0 &&
          selectp(real_people, (real_index+1)&3))
         return true;

      if ((real_people->people[(real_index+3)&3].id1 & (1 << ((real_index&1)*3))) != 0 &&
          selectp(real_people, (real_index+3)&3))
         return true;
   }

   return false;
}



/* ARGSUSED */
static bool cols_someone_in_front(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   if (real_people->kind == s2x3) {
      if (real_index == 1 || real_index == 4) {
         if (real_people->people[real_index+((northified_index==1) ? 1 : -1)].id1)
            return true;
         else return false;
      }
      else return true;
   }
   else {
      if (northified_index < 3 || northified_index > 4) {
         if (real_people->people[real_index+((northified_index<4) ? 1 : -1)].id1)
            return true;
         else return false;
      }
      else return false;
   }
}

// the "extra_stuff" argument is:
//   1 for "x14_once_rem_miniwave"
//   3 for "intlk_cast_normal_or_warn"
/* ARGSUSED */
static bool x14_once_rem_miniwave(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   switch (real_people->cmd.cmd_assume.assumption) {
   case cr_wave_only: case cr_li_lo: case cr_1fl_only: return false;
   case cr_2fl_only: case cr_magic_only: return true;
   }

   switch ((real_people->people[real_index].id1 ^
            real_people->people[real_index ^ 3].id1) & DIR_MASK) {
   case 0:
      return false;
   case 2:
      return true;
   default:
      if (extra_stuff[0] & 2) {
         // This is "intlk_cast_normal_or_warn".  Don't give the warning if person
         // would have known what to do anyway.
         if (!(real_index & 1))
            warn(warn__opt_for_normal_cast);
         return true;
      }
      else
         return false;
   }
}

/* ARGSUSED */
static bool x14_once_rem_couple(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   switch (real_people->cmd.cmd_assume.assumption) {
   case cr_wave_only: case cr_li_lo: case cr_1fl_only: return true;
   case cr_2fl_only: case cr_magic_only: return false;
   }

   return ((real_people->people[real_index].id1 ^
            real_people->people[real_index ^ 3].id1) & DIR_MASK) == 0;
}

/* ARGSUSED */
static bool lines_miniwave(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   switch (real_people->cmd.cmd_assume.assumption) {
   case cr_wave_only: case cr_magic_only:
      if (real_people->kind != sdmd) return true;
   case cr_2fl_only:
      if (real_people->kind != sdmd) return false;
   default:
      int this_person = real_people->people[real_index].id1;
      int other_person = real_people->people[real_index ^ ((real_people->kind == sdmd) ? 2 : 1)].id1;
      if (real_people->kind == s1x6 && real_index >= 2)
         other_person = real_people->people[7 - real_index].id1;
      return ((this_person ^ other_person) & DIR_MASK) == 2;
   }
}

/* ARGSUSED */
static bool lines_couple(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   switch (real_people->cmd.cmd_assume.assumption) {
   case cr_wave_only: case cr_magic_only:
      if (real_people->kind != sdmd) return false;
   case cr_2fl_only:
      if (real_people->kind != sdmd) return true;
   default:
      int this_person = real_people->people[real_index].id1;
      int other_person = real_people->people[real_index ^ ((real_people->kind == sdmd) ? 2 : 1)].id1;
      if (real_people->kind == s1x6 && real_index >= 2)
         other_person = real_people->people[7 - real_index].id1;
      return ((this_person ^ other_person) & DIR_MASK) == 0;
   }
}

static const int32_t tab_mwv_in_3n1[8]  = {2, 0, 0, 4, 3, 7, 2};
static const int32_t tab_cpl_in_3n1[8]  = {0, 2, 0, 4, 3, 7, 2};
static const int32_t tab_mwv_out_3n1[8] = {2, 0, 1, 5, 2, 6, 3};
static const int32_t tab_cpl_out_3n1[8] = {0, 2, 1, 5, 2, 6, 3};

/* ARGSUSED */
static bool check_3n1_setup(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   uint32_t A = extra_stuff[0];
   uint32_t B = extra_stuff[1];
   uint32_t C = extra_stuff[2];
   uint32_t D = extra_stuff[3];
   uint32_t E = extra_stuff[4];
   uint32_t F = extra_stuff[5];
   uint32_t G = extra_stuff[6];

   if (real_people->cmd.cmd_assume.assumption == cr_wave_only ||
       real_people->cmd.cmd_assume.assumption == cr_2fl_only ||
       ((real_people->people[real_index].id1 ^ real_people->people[real_index ^ 1].id1) & DIR_MASK) != A)
      return false;
   else if (real_people->kind == s2x4) {
      int k = real_index & 2;

      return
         ((real_people->people[C].id1 ^ real_people->people[E].id1) & DIR_MASK) == 0 &&
         ((real_people->people[D].id1 ^ real_people->people[F].id1) & DIR_MASK) == 0 &&
         ((real_people->people[C].id1 ^ real_people->people[D].id1) & DIR_MASK) == 2 &&
         ((real_people->people[k ^ 2].id1 ^ real_people->people[k ^ 3].id1) & DIR_MASK) == B &&
         ((real_people->people[k ^ 6].id1 ^ real_people->people[k ^ 7].id1) & DIR_MASK) == B &&
         ((real_people->people[k ^ 4].id1 ^ real_people->people[k ^ 5].id1) & DIR_MASK) == A;
   }
   else if (real_people->kind == s1x8) {
      int k = real_index & 2;
      int j = k + (k>>1);

      return
         ((real_people->people[C].id1 ^ real_people->people[G].id1) & DIR_MASK) == 0 &&
         ((real_people->people[D].id1 ^ real_people->people[F^1].id1) & DIR_MASK) == 0 &&
         ((real_people->people[C].id1 ^ real_people->people[D].id1) & DIR_MASK) == 2 &&
         ((real_people->people[k ^ 2].id1 ^ real_people->people[k ^ 3].id1) & DIR_MASK) == B &&
         ((real_people->people[j ^ 6].id1 ^ real_people->people[j ^ 7].id1) & DIR_MASK) == B &&
         ((real_people->people[j ^ 4].id1 ^ real_people->people[j ^ 5].id1) & DIR_MASK) == A;
   }
   else {
      return
         ((real_people->people[C].id1 ^ real_people->people[G].id1) & DIR_MASK) == 0 &&
         ((real_people->people[C].id1 ^ real_people->people[(real_index & 2) ^ E].id1) & DIR_MASK) == B;
   }
}

/* the "extra_stuff" argument is:
     0 for "miniwave_side_of_2n1_line"
     3 for "couple_side_of_2n1_line" */
/* ARGSUSED */
static bool some_side_of_2n1_line(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int k = extra_stuff[0];
   if (real_index == 0 || real_index == 3) k ^= 3;

   return
      ((real_people->people[0].id1 ^ real_people->people[2].id1) & DIR_MASK) == 2      &&
      ((real_people->people[0].id1 ^ real_people->people[5].id1) & DIR_MASK) == 0      &&
      ((real_people->people[2].id1 ^ real_people->people[3].id1) & DIR_MASK) == 0      &&
      ((real_people->people[0].id1 ^ real_people->people[k+1].id1) & DIR_MASK) == 0    &&
      ((real_people->people[2].id1 ^ real_people->people[4-k].id1) & DIR_MASK) == 0;
}

// the "extra_stuff" argument is:
//   0 for "cast_pushy"
//   1 for "cast_normal"
//   3 for "cast_normal_or_warn"
//   7 for "cast_normal_or_nowarn"
/* ARGSUSED */
static bool cast_normal_or_whatever(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   if (real_people->cmd.cmd_assume.assumption == cr_wave_only ||
         real_people->cmd.cmd_assume.assumption == cr_miniwaves ||
         real_people->cmd.cmd_assume.assump_cast)
      return (extra_stuff[0] & 1) != 0;
   else if (real_people->cmd.cmd_assume.assumption == cr_couples_only ||
         real_people->cmd.cmd_assume.assumption == cr_2fl_only)
      return (extra_stuff[0] & 1) == 0;
   else {
      uint32_t this_person = real_people->people[real_index].id1;
      uint32_t other_person = real_people->people[real_index ^ 1].id1;
      if (real_people->kind == s1x6 && real_index >= 2)
         other_person = real_people->people[7 - real_index].id1;

      switch (((this_person ^ other_person) & DIR_MASK) ^ ((extra_stuff[0] & 1) << 1)) {
      case 0:
         return true;
      case 2:
         return false;
      default:
         if (extra_stuff[0] & 4)
            return true;   // This is "cast_normal_or_nowarn".  Just do normal cast, don't complain.
         else if (extra_stuff[0] & 2) {
            // This is "cast_normal_or_warn".  Don't give the warning if person
            // would have known what to do anyway.
            if (real_people->kind == s1x2 ||
                (real_index != 1 && real_index != ((real_people->kind == s1x6) ? 4 : 3)))
               warn(warn__opt_for_normal_cast);
            return true;
         }
         else
            return false;
      }
   }
}

/* ARGSUSED */
static bool columns_tandem(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   uint32_t this_person;
   int other_index;

   if (real_people->kind == s_qtag) {
      if (real_index == 2 || real_index == 6) return false;  // Wings of qtag always fail.
   }
   else if (real_people->kind == s_spindle) {
      if (real_index == 3 || real_index == 7) return false; // Or points of spindle.
   }
   else {
      switch (real_people->cmd.cmd_assume.assumption) {
      case cr_wave_only: case cr_2fl_only: return (extra_stuff[0] ^ 1) != 0;
      case cr_magic_only: case cr_li_lo: return extra_stuff[0] != 0;
      }
   }

   this_person = real_people->people[real_index].id1;
   other_index = real_index ^ 1;

   if (real_people->kind == s1x6 && real_index >= 2)
      other_index = 7 - real_index;
   else if (real_people->kind == s1x3) {
      if (real_index == 2 || !(real_direction & 2))
         other_index = 3 - real_index;
   }
   else if (real_people->kind == s2x3) {
      if (real_index == 2)
         other_index = 1;
      else if (real_index == 3)
         other_index = 4;
      else if (!(real_direction & 2)) {
         if (real_index == 1)
            other_index = 2;
         else if (real_index == 4)
            other_index = 3;
      }
   }
   else if (real_people->kind == s_qtag) {
      other_index = real_index ^ 2;
      if ((real_index & 3) == 0 ||
          (((real_index & 3) == 3) && ((real_direction ^ (real_index >> 1)) & 2)))
         other_index = real_index ^ 7;
   }
   else if (real_people->kind == s_spindle) {
      switch (real_index) {
      case 0: other_index = 1; break;
      case 1: other_index = (real_direction & 2) ? 0: 2; break;
      case 2: other_index = 1; break;
      case 4: other_index = 5; break;
      case 5: other_index = (real_direction & 2) ? 6: 4; break;
      case 6: other_index = 5; break;
      }
   }

   return ((this_person ^ real_people->people[other_index].id1) & DIR_MASK) ==
      ((uint32_t) extra_stuff[0] << 1);
}

/* ARGSUSED */
static bool same_in_pair(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int this_person;
   int other_person;

   if (real_people->kind == s_bone) {
      // This is only valid for the ends; the documentation says so.
      other_person = real_people->people[real_index ^ 5].id1;
   }
   else {
      if (real_people->cmd.cmd_assume.assump_col == 1) {
         switch (real_people->cmd.cmd_assume.assumption) {
         case cr_2fl_only: case cr_li_lo: return true;
         case cr_wave_only: case cr_magic_only: return false;
         }
      }

      other_person = real_people->people[real_index ^ 7].id1;
   }

   this_person = real_people->people[real_index].id1;

   return ((this_person ^ other_person) & DIR_MASK) == 0;
}

/* ARGSUSED */
static bool opp_in_pair(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int this_person;
   int other_person;

   if (real_people->kind == s_bone) {
      // This is only valid for the ends; the documentation says so.
      other_person = real_people->people[real_index ^ 5].id1;
   }
   else {
      if (real_people->cmd.cmd_assume.assump_col == 1) {
         switch (real_people->cmd.cmd_assume.assumption) {
         case cr_wave_only: case cr_magic_only: return true;
         case cr_2fl_only: case cr_li_lo: return false;
         }
      }

      other_person = real_people->people[real_index ^ 7].id1;
   }

   this_person = real_people->people[real_index].id1;

   return ((this_person ^ other_person) & DIR_MASK) == 2;
}

/* ARGSUSED */
static bool opp_in_magic(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int this_person;
   int other_person;
   int other_index;

   if (real_people->cmd.cmd_assume.assump_col == 1) {
      switch (real_people->cmd.cmd_assume.assumption) {
         case cr_wave_only: case cr_li_lo: return true;
         case cr_2fl_only: case cr_magic_only: return false;
      }
   }

   this_person = real_people->people[real_index].id1;
   other_index = real_index ^ 6;
   if (real_people->kind == s2x3) {
      other_index = 4 >> (((real_index+5) >> 3) << 1);
      if (real_index == 1)
         other_index = 3 + (this_person & 2);
      else if (real_index == 4)
         other_index = 2 - (this_person & 2);
   }
   other_person = real_people->people[other_index].id1;
   return ((this_person ^ other_person) & DIR_MASK) == 2;
}

/* ARGSUSED */
static bool same_in_magic(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int this_person;
   int other_person;
   int other_index;

   if (real_people->cmd.cmd_assume.assump_col == 1) {
      switch (real_people->cmd.cmd_assume.assumption) {
         case cr_2fl_only: case cr_magic_only: return true;
         case cr_wave_only: case cr_li_lo: return false;
      }
   }

   this_person = real_people->people[real_index].id1;
   other_index = real_index ^ 6;
   if (real_people->kind == s2x3) {
      other_index = 4 >> (((real_index+5) >> 3) << 1);
      if (real_index == 1)
         other_index = 3 + (this_person & 2);
      else if (real_index == 4)
         other_index = 2 - (this_person & 2);
   }
   other_person = real_people->people[other_index].id1;
   return ((this_person ^ other_person) & DIR_MASK) == 0;
}

/* ARGSUSED */
static bool once_rem_test(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int this_person = real_people->people[real_index].id1;
   int other_person = real_people->people[real_index ^ 2].id1;
   return ((this_person ^ other_person) & DIR_MASK) == extra_stuff[0];
}

/* ARGSUSED */
static bool x12_beau_or_miniwave(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   if (real_people->cmd.cmd_assume.assumption == cr_wave_only ||
         real_people->cmd.cmd_assume.assumption == cr_miniwaves ||
         northified_index == 0 ||
         (real_people->kind == s2x2 && northified_index == 3))
      return true;
   else if (real_people->cmd.cmd_assume.assumption == cr_couples_only)
      return false;
   else {
      int other_person = real_people->people[real_index ^ 1 ^ ((real_direction&1) << 1)].id1;
      int direction_diff = other_person ^ real_direction;

      // This was "1x2_beau_miniwave_or_warn", and the other person is a phantom,
      // we give a warning and assume we had a miniwave.

      if (extra_stuff[0] == 1 && !other_person) {
         warn(warn__opt_for_normal_hinge);
         return true;
      }

      // But if this was "1x2_beau_miniwave_or_ok", and the other person is a phantom,
      // we let it pass, and assume a miniwave.
      if (extra_stuff[0] == 3 && !other_person) {
         return true;
      }

      if (extra_stuff[0] == 2 && !other_person) {
         return true;
      }

      if (!other_person || (direction_diff & 1))
         fail("Need a real, not T-boned, person to work with.");

      if ((direction_diff & 2) == 2)
         return true;

      if (extra_stuff[0] == 2)
         warn(warn__like_linear_action);

      return false;
   }
}

static const int32_t swingleft_1x3dmd[8] = {-1, 0, 1, -1, 5, 6, -1, 3};
static const int32_t swingleft_1x4[4] = {-1, 0, 3, 1};
static const int32_t swingleft_c1phan[16] = {-1, -1, 0, 1, -1, 7, 4, -1, 10, 11, -1, -1, 14, -1, -1, 13};
static const int32_t swingleft_4x4[16] = {14, 3, 7, 15, 5, 6, 8, 11, -1, -1, -1, 9, -1, 12, 13, 10};
static const int32_t swingleft_2x6[16] = {-1, 0, 1, 2, 3, 4, 7, 8, 9, 10, 11, -1};
static const int32_t swingleft_2x8[16] = {-1, 0, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, -1};
static const int32_t swingleft_3dmd[12] = {-1, -1, -1, 4, 5, 11, -1, -1, -1, -1, 9, 10};
static const int32_t swingleft_deep2x1dmd[10] = {-1, 0, -1, 1, 3, 6, 8, -1, 9, -1};
static const int32_t swingleft_wqtag[10] = {-1, -1, 3, 4, 9, -1, -1, -1, 7, 8};

static bool can_swing_left(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int t;
   int size = attr::slimit(real_people)+1;
   int halfsize = size >> 1;

   // For "can_swing_right" fudge the northified_index to be a southified_index.
   int direction_index = northified_index - (*extra_stuff ? halfsize : 0);
   if (direction_index < 0) direction_index += size;

   switch (real_people->kind) {
   case s1x4:
      t = swingleft_1x4[direction_index];
      break;
   case s1x3dmd:
      t = swingleft_1x3dmd[direction_index];
      break;
   case swqtag:
      t = swingleft_wqtag[direction_index];
      break;
   case s3dmd:
      t = swingleft_3dmd[direction_index];
      break;
   case sdeep2x1dmd:
      t = swingleft_deep2x1dmd[direction_index];
      break;
   case s_c1phan:
      t = swingleft_c1phan[direction_index];
      break;
   case s4x4:
      t = swingleft_4x4[direction_index];
      break;
   case s2x6:
      t = swingleft_2x6[direction_index];
      break;
   case s2x8:
      t = swingleft_2x8[direction_index];
      break;
   default:
      return false;
   }

   if (t < 0) return false;

   if (direction_index != real_index) {
      t -= halfsize;
   }

   if (real_direction & 1)
      t += direction_index - real_index - size;    // Be sure it's downward.

   // We could be way out of range, always downward.
   while (t < 0) t += size;

   return ((real_people->people[real_index].id1 ^ real_people->people[t].id1) & DIR_MASK) == 2;
}


// Test for wheel and deal to be done 2FL-style, or beau side of 1FL.  Returns
// false if belle side of 1FL.  Raises a warning if wheel and deal can't be done,
// and opts for L2FL.
/* ARGSUSED */
static bool x14_wheel_and_deal(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   // We assume people have already been checked for coupleness.

   if (northified_index <= 1)
      // We are in the beau-side couple -- it's OK.
      return true;
   else {
      // We are in the belle-side couple.  First, see if an assumption is guiding us.

      switch (real_people->cmd.cmd_assume.assumption) {
         case cr_2fl_only: return true;
         case cr_1fl_only: return false;
      }

      // Find the two people in the other couple.
      // Just "or" them to be sure we get someone.  They are already known
      // to be facing consistently if they are both there.
      int other_people = real_people->people[real_index ^ 2].id1 |
                         real_people->people[real_index ^ 3].id1;

      // At least one of those people must exist.
      if (!other_people) {
         if (extra_stuff[0] != 0) warn(warn__opt_for_2fl);
         return extra_stuff[0] != 0;
      }

      // See if they face the same way as myself.  Note that the "2" bit of
      // real_index is the complement of my own direction bit.
      if (((other_people ^ real_index) & 2))
         return false;   // This is a 1FL.

      return true;       // This is a 2FL.
   }
}

// Test for 3X3 wheel_and_deal to be done 2FL-style, or beau side of 1FL.
/* ARGSUSED */
static bool x16_wheel_and_deal(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   // We assume people have already been checked for coupleness.

   if (northified_index <= 2 || northified_index >= 9)
      // We are in the beau-side triad -- it's OK.
      return true;
   else {
      // We are in the belle-side triad -- find the three people in the other triad.
      // Just "or" them to be sure we get someone.  They are already known
      // to be facing consistently if they are all there.
      int other_side = ((real_index / 3) ^ 1) * 3;
      int other_people = real_people->people[other_side].id1 |
                           real_people->people[other_side+1].id1 |
                           real_people->people[other_side+2].id1;

      // At least one of those people must exist.
      if (!other_people)
         fail("Can't tell how to do this -- no live people.");

      // See if they face the same way as myself.
      if (((other_people ^ real_people->people[real_index].id1) & 2))
         return true;       // This is a 2FL.

      return false;         // This is a 1FL.
   }
}

// Test for 4X4 wheel_and_deal to be done 2FL-style, or beau side of 1FL.
/* ARGSUSED */
static bool x18_wheel_and_deal(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   // We assume people have already been checked for coupleness.

   if (northified_index <= 3 || northified_index >= 12)
      // We are in the beau-side quad -- it's OK.
      return true;
   else {
      // We are in the belle-side quad -- find the four people in the other quad.
      // Just "or" them to be sure we get someone.  They are already known
      // to be facing consistently if they are all there.
      int other_side = (real_index & 014) ^ 4;
      int other_people = real_people->people[other_side].id1 |
                           real_people->people[other_side+1].id1 |
                           real_people->people[other_side+2].id1 |
                           real_people->people[other_side+3].id1;

      // At least one of those people must exist.
      if (!other_people)
         fail("Can't tell how to do this -- no live people.");

      // See if they face the same way as myself.
      if (((other_people ^ real_people->people[real_index].id1) & 2))
         return true;       // This is a 2FL.

      return false;         // This is a 1FL.
   }
}

// First test for how to do cycle and wheel.  This always passes the extreme beau,
// and always fails the extreme belle (causing the belle to go on to the next test.)
// For centers, it demands an adjacent end (otherwise we wouldn't know whether
// we were cycling or wheeling) and then returns true if that end is an extreme beau.
/* ARGSUSED */
static bool cycle_and_wheel_1(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   if (northified_index == 0)
      return true;
   else if (northified_index == 2)
      return false;
   else {     /* We are a center.  Find our adjacent end. */
      int other_person;

      // We think it is rather silly to use an "assume" concept to specify
      // a symmetric kind of line, and then give a call that doesn't do
      // anything interesting with any of the lines that the assumption can
      // make, but we have to do this.

      switch (real_people->cmd.cmd_assume.assumption) {
         case cr_1fl_only:  case cr_2fl_only:   return northified_index == 1;
         case cr_wave_only: case cr_magic_only: return northified_index == 3;
      }

      other_person = real_people->people[real_index ^ 1].id1;

      if (!other_person)
         fail("Can't tell how to do this -- no live people.");

      // See if he is an extreme beau.
      if (((other_person ^ real_index) & 2))
         return false;

      return true;
   }
}

/* Second test for how to do cycle and wheel.  This finds the far end, and checks
   whether he faces the same way as myself. */
/* ARGSUSED */
static bool cycle_and_wheel_2(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   switch (real_people->cmd.cmd_assume.assumption) {
      case cr_1fl_only: case cr_2fl_only: return true;
      case cr_wave_only:  return northified_index != 2;
      case cr_magic_only: return northified_index == 2;
   }

   int other_person = real_people->people[(real_index ^ 2) & (~1)].id1;

   // Here we default to the "non-colliding" version of the call if the
   // opposite end of our line doesn't exist.
   if (!other_person) {
      warn(warn__opt_for_no_collision);
      return northified_index == 1;
   }

   // See if he faces the same way as myself.
   if (((other_person ^ real_people->people[real_index].id1) & 2))
      return false;

   return true;
}

static bool vert1(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   if (!(northified_index & 1))
      return true;
   else if (real_people->cmd.cmd_assume.assumption == cr_wave_only ||
         real_people->cmd.cmd_assume.assumption == cr_miniwaves)
      return false;
   else if (real_people->cmd.cmd_assume.assumption == cr_couples_only ||
            real_people->cmd.cmd_assume.assumption == cr_li_lo)
      return true;
   else {
      int this_person = real_people->people[real_index].id1;
      int other_person = real_people->people[real_index ^ (((real_direction << 1) & 2) | 1)].id1;

      return ((this_person ^ other_person) & DIR_MASK) == 0;
   }
}

static bool vert2(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   if (!(northified_index & 1))
      return false;
   else if (real_people->cmd.cmd_assume.assumption == cr_wave_only ||
         real_people->cmd.cmd_assume.assumption == cr_miniwaves)
      return true;
   else if (real_people->cmd.cmd_assume.assumption == cr_couples_only ||
            real_people->cmd.cmd_assume.assumption == cr_li_lo)
      return false;
   else {
      int this_person = real_people->people[real_index].id1;
      int other_person = real_people->people[real_index ^ (((real_direction << 1) & 2) | 1)].id1;

      return ((this_person ^ other_person) & DIR_MASK) == 2;
   }
}

/* ARGSUSED */
static bool inner_active_lines(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   if ((real_index+3) & 2)
      return northified_index >= 4;     // I am an end.
   else if (real_people->people[real_index ^ 1].id1)
      return                            // I am a center, with a live partner.
         (uint32_t) (012 - ((real_index & 4) >> 1)) ==
         (real_people->people[real_index ^ 1].id1 & 017);
   else if (real_people->cmd.cmd_assume.assumption == cr_wave_only)
      return northified_index < 4;
   else if (real_people->cmd.cmd_assume.assumption == cr_2fl_only)
      return northified_index >= 4;
   else
      return false;
}

/* ARGSUSED */
static bool outer_active_lines(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   if ((real_index+3) & 2)
      return northified_index < 4;      // I am an end.
   else if (real_people->people[real_index ^ 1].id1)
      return                            // I am a center, with a live partner.
         (uint32_t) (010 + ((real_index & 4) >> 1)) ==
         (real_people->people[real_index ^ 1].id1 & 017);
   else if (real_people->cmd.cmd_assume.assumption == cr_wave_only)
      return northified_index >= 4;
   else if (real_people->cmd.cmd_assume.assumption == cr_2fl_only)
      return northified_index < 4;
   else
      return false;
}


//                               line of 3     line of 4    line of 5    line of 6    line of 7    line of 8
//                                                                                 (no such thing)
static const int32_t jr1x4[24]  = {2, 0, 0, 2,  1, 0, 0, 2,  3, 0, 0, 2,  1, 0, 0, 2,  7, 7, 7, 7,  1, 0, 0, 2};
static const int32_t sl1x4[24]  = {0, 2, 0, 2,  1, 2, 0, 2,  0, 3, 0, 2,  1, 2, 0, 2,  7, 7, 7, 7,  1, 2, 0, 2};
static const int32_t jl1x4[24]  = {0, 2, 2, 0,  0, 2, 2, 0,  0, 3, 2, 0,  0, 2, 2, 0,  7, 7, 7, 7,  0, 2, 2, 0};
static const int32_t sr1x4[24]  = {2, 0, 2, 0,  0, 0, 2, 0,  3, 0, 2, 0,  0, 0, 2, 0,  7, 7, 7, 7,  0, 0, 2, 0};

static bool judge_check(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int sizem1 = attr::slimit(real_people);
   extra_stuff += (sizem1-2) << 2;
   uint32_t this_person = real_people->people[real_index].id1;

   if (sizem1 == 2 || sizem1 == 4) {
      // Lines of 3 or 5 are special.
      int f = this_person & 2;
      if (sizem1 == 4)
         f += f>>1;

      return
         (((real_people->people[extra_stuff[0] ^ f].id1 ^ this_person) & 013) == (const uint32_t) extra_stuff[2])
         &&
         (((real_people->people[extra_stuff[1] ^ f].id1 ^ this_person) & 013) != (const uint32_t) extra_stuff[3]);
   }

   uint32_t f = (this_person & 2) ^ extra_stuff[1];

   int reverse_count = (sizem1+1) >> 1;

   switch (sizem1) {
   case 3:
      // Line of 4.
      // Some assumptions make this easy.  Sort of.
      switch (real_people->cmd.cmd_assume.assumption) {
      case cr_wave_only: case cr_2fl_only: return false;      // This can't work.
      case cr_all_facing_same: case cr_1fl_only: case cr_li_lo: return extra_stuff[0] != 0;
      case cr_magic_only: return (real_index & 1) != extra_stuff[0];
      }
      break;
   case 5:
      // Line of 6.
      f += f>>1;

      switch (real_people->cmd.cmd_assume.assumption) {
      case cr_wave_only: case cr_3x3_2fl_only: return false;      // This can't work.
      case cr_all_facing_same: case cr_1fl_only: return extra_stuff[0] != 0;
      }

      break;
   case 7:
      // Line of 8.
      f <<= 1;

      switch (real_people->cmd.cmd_assume.assumption) {
      case cr_wave_only: case cr_4x4_2fl_only: return false;      // This can't work.
      case cr_all_facing_same: case cr_1fl_only: return extra_stuff[0] != 0;
      }

      break;
   }

   // If judge/socker exists in the correct place, and
   // we do not have another judge/socker in the wrong place, OK.
   if (((real_people->people[reverse_count-f].id1 ^ this_person) & 013) == (uint32_t) extra_stuff[2] &&
       ((real_people->people[f].id1 ^ this_person) & 013) != (uint32_t) extra_stuff[3])
      return true;

   // If correct place is empty, but the person on the other side
   // can't be a judge or socker, OK.
   if (real_people->people[reverse_count-f].id1 == 0 &&
       ((real_people->people[f].id1 ^ this_person) & 013) != (uint32_t) extra_stuff[3]) {
      warn(warn__phantom_judge);
      return true;
   }

   return false;
}


static int8_t inroll_directions[24] = {
   012, 012, 012, 012, 010, 010, 010, 010,
      3,  3,  3,  3,  7,  7,  7,  7,
      0,  0,  0,  0,  4,  4,  4,  4};

static int8_t magic_inroll_directions[24] = {
   012, 010, 010, 012, 010, 012, 012, 010,
      3,  7,  7,  3,  7,  3,  3,  7,
      0,  4,  4,  0,  4,  0,  0,  4};

static int8_t inroll_directions_2x3[18] = {
   012, 012, 012, 010, 010, 010,
      2,  2,  2,  5,  5,  5,
      0,  0,  0,  3,  3,  3};

static int8_t magic_inroll_directions_2x3[18] = {
   012, 010, 012, 010, 012, 010,
      2,  5,  2,  5,  2,  5,
      0,  3,  0,  3,  0,  3};

static int8_t inroll_directions_2x6[36] = {
   012, 012, 012, 012, 012, 012, 010, 010, 010, 010, 010, 010,
      5,  5,  5,  5,  5,  5, 11, 11, 11, 11, 11, 11,
      0,  0,  0,  0,  0,  0,  6,  6,  6,  6,  6,  6};

static int8_t inroll_directions_2x8[48] = {
   012, 012, 012, 012, 012, 012, 012, 012, 010, 010, 010, 010, 010, 010, 010, 010,
      7,  7,  7,  7,  7,  7,  7,  7, 15, 15, 15, 15, 15, 15, 15, 15,
      0,  0,  0,  0,  0,  0,  0,  0,  8,  8,  8,  8,  8,  8,  8,  8};

enum inroll_assume_test {
   ira__no_wave,
   ira__sixes,
   ira__eights,
   ira__gen
};


struct inroll_action {
   int8_t *directions;
   int32_t code;
   inroll_assume_test ira;
};


static const inroll_action inroller_cw            = {inroll_directions,           0, ira__gen};
static const inroll_action outroller_cw           = {inroll_directions,           1, ira__gen};
static const inroll_action magic_inroller_cw      = {magic_inroll_directions,     2, ira__gen};
static const inroll_action magic_outroller_cw     = {magic_inroll_directions,     3, ira__gen};
static const inroll_action inroller_cw_2x3        = {inroll_directions_2x3,       0, ira__no_wave};
static const inroll_action outroller_cw_2x3       = {inroll_directions_2x3,       1, ira__no_wave};
static const inroll_action magic_inroller_cw_2x3  = {magic_inroll_directions_2x3, 2, ira__no_wave};
static const inroll_action magic_outroller_cw_2x3 = {magic_inroll_directions_2x3, 3, ira__no_wave};
static const inroll_action inroller_2x6           = {inroll_directions_2x6,       0, ira__sixes};
static const inroll_action outroller_2x6          = {inroll_directions_2x6,       1, ira__sixes};
static const inroll_action inroller_2x8           = {inroll_directions_2x8,       0, ira__eights};
static const inroll_action outroller_2x8          = {inroll_directions_2x8,       1, ira__eights};



static bool in_out_roll_select(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   const inroll_action *thing = (const inroll_action *) extra_stuff;
   const int8_t *directions = thing->directions;
   int code = thing->code;

   switch (thing->ira) {
   case ira__no_wave:
      if (real_people->cmd.cmd_assume.assumption == cr_wave_only)
         fail("Not legal.");
      break;
   case ira__sixes:
      if (real_people->cmd.cmd_assume.assumption == cr_wave_only)
         return ((northified_index ^ (northified_index / 6) ^ code) & 1) == 0;
      break;
   case ira__eights:
      if (real_people->cmd.cmd_assume.assumption == cr_wave_only)
         return ((northified_index ^ (northified_index >> 3) ^ code) & 1) == 0;
      break;
   case ira__gen:
      if (real_people->cmd.cmd_assume.assumption == cr_wave_only)
         return ((northified_index ^ (northified_index >> 2) ^ code) & 1) == 0;
      if (real_people->cmd.cmd_assume.assumption == cr_2fl_only)
         return (((northified_index >> 1) ^ (northified_index >> 2) ^ code) & 1) == 0;
      break;
   }

   // "Yes_roll_direction" is the facing direction that constitutes what we are
   // looking for (inroller or outroller as the case may be).

   int yes_roll_direction = directions[real_index];
   if (code&1) yes_roll_direction = 022 - yes_roll_direction;
   int no_roll_direction = 022 - yes_roll_direction;

   int size = attr::slimit(real_people)+1;

   int cw_idx  = directions[real_index+size];
   int ccw_idx = directions[real_index+2*size];

   int cw_end =  real_people->people[cw_idx].id1 & 017;
   int ccw_end = real_people->people[ccw_idx].id1 & 017;

   if (
       // cw_end exists and is proper, and we do not have ccw_end proper also
       (cw_end == yes_roll_direction && ccw_end != yes_roll_direction) ||
       // or if ccw_end exists and is improper, and cw_end is a phantom
       (ccw_end == no_roll_direction && cw_end == 0))
      return true;
   else if (
            // ccw_end exists and is proper, and we do not have cw_end proper also
            (ccw_end == yes_roll_direction && cw_end != yes_roll_direction) ||
            // or if cw_end exists and is improper, and ccw_end is a phantom
            (cw_end == no_roll_direction && ccw_end == 0))
      return false;
   else {
      static const char * const errmsg[4] = {
         "Can't find end looking in.",
         "Can't find end looking out.",
         "Can't find magic end looking in.",
         "Can't find magic end looking out."};
      fail(errmsg[code&3]);
      return false;
   }
}


static bool outposter_is_cw(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   uint32_t outroll_direction;
   uint32_t cw_dir;

   if (real_people->kind == s2x3) {
      if (real_people->cmd.cmd_assume.assumption == cr_wave_only)
         return (northified_index & 1) == 0;
      else if (real_people->cmd.cmd_assume.assumption == cr_1fl_only)
         return northified_index < 3;

      outroll_direction = (real_index >= 3) ? 012 : 010;
      cw_dir = real_people->people[(real_index < 3) ? 2 : 5].id1 & 017;

      // Cw_end exists and is looking out.
      if (cw_dir == outroll_direction) return true;
      // Or cw_end is phantom but ccw_end is looking in, so it has to be the phantom.
      else if (cw_dir == 0 &&
               (real_people->people[(real_index < 3) ? 0 : 3].id1 & 017) == (022 - outroll_direction))
         return true;
      // We don't know.
      else return false;
   }
   else {
      if (real_people->cmd.cmd_assume.assumption == cr_wave_only)
         return ((northified_index ^ (northified_index >> 2)) & 1) != 0;
      else if (real_people->cmd.cmd_assume.assumption == cr_2fl_only)
         return ((northified_index ^ (northified_index >> 1)) & 2) != 0;
      else if (real_people->cmd.cmd_assume.assumption == cr_1fl_only)
         return (northified_index & 4) == 0;
      else if (real_people->cmd.cmd_assume.assumption == cr_li_lo &&
               real_people->cmd.cmd_assume.assump_both == 2)
         return true;

      outroll_direction = 010 + ((real_index & 4) >> 1);
      cw_dir = real_people->people[real_index | 3].id1 & 017;

      // Cw_end exists and is looking out.
      if (cw_dir == outroll_direction) return true;
      // Or cw_end is phantom but ccw_end is looking in, so it has to be the phantom.
      else if (cw_dir == 0 &&
               (real_people->people[real_index & 4].id1 & 017) == (022 - outroll_direction))
         return true;
      // We don't know.
      else return false;
   }
}

/* ARGSUSED */
static bool outposter_is_ccw(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   uint32_t outroll_direction;

   if (real_people->kind == s2x3) {
      if (real_people->cmd.cmd_assume.assumption == cr_wave_only)
         return (northified_index & 1) == 0;
      else if (real_people->cmd.cmd_assume.assumption == cr_1fl_only)
         return northified_index < 3;

      outroll_direction = (real_index >= 3) ? 012 : 010;
      uint32_t ccw_dir = real_people->people[(real_index < 3) ? 0 : 3].id1 & 017;

      // Ccw_end exists and is looking out.
      if (ccw_dir == outroll_direction) return true;
      // Or ccw_end is phantom but cw_end is looking in, so it has to be the phantom.
      else if (ccw_dir == 0 &&
               (real_people->people[(real_index < 3) ? 2 : 5].id1 & 017) == (022 - outroll_direction))
         return true;
      // We don't know.
      else return false;
   }
   else {
      if (real_people->cmd.cmd_assume.assumption == cr_wave_only)
         return ((northified_index ^ (northified_index >> 2)) & 1) == 0;
      else if (real_people->cmd.cmd_assume.assumption == cr_2fl_only)
         return ((northified_index ^ (northified_index >> 1)) & 2) == 0;
      else if (real_people->cmd.cmd_assume.assumption == cr_1fl_only)
         return false;
      else if (real_people->cmd.cmd_assume.assumption == cr_li_lo &&
               real_people->cmd.cmd_assume.assump_both == 2)
         return false;

      outroll_direction = 012 - ((real_index & 4) >> 1);

      // Demand that cw_end be looking in -- otherwise "outposter_is_cw"
      // would have accepted it if it were legal.
      if ((real_people->people[real_index | 3].id1 & 017) != outroll_direction) return false;
      else {
         // Now if ccw_end is looking out or is a phantom, it's OK.
         uint32_t ccw_dir = real_people->people[real_index & 4].id1 & 017;
         if (ccw_dir == 0 || ccw_dir == 022-outroll_direction) return true;
         return false;
      }
   }
}

/* ARGSUSED */
static bool raise_some_sglfile(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   warn(warn_some_singlefile);
   return true;
}

/* ARGSUSED */
static bool count_cw_people(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int i;
   int count = 0;

   int limit = (attr::slimit(real_people)+1) >> 1;
   if (real_index >= limit) limit <<= 1;

   for (i=real_index+1; i<limit ; i++) {
      if (real_people->people[i].id1) count++;
   }

   return (count == (*extra_stuff));
}

/* ARGSUSED */
static bool check_4x4_quad(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   return real_people->people[(real_index+extra_stuff[0]) & 0xF].id1 != 0;
}

/* ARGSUSED */
static bool check_facing_dmd_spot(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int my_handedness = real_people->people[real_index].id1 ^ real_index;
   int next_index = (real_index + 1 - (my_handedness & 2)) & 3;
   int next_handedness = real_people->people[next_index].id1 ^ next_index;
   // We demand that the next person be real.
   return ((my_handedness ^ next_handedness) & (BIT_PERSON|3)) == extra_stuff[0];
}

/* ARGSUSED */
static bool check_qtag_spot_facing_me(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   if (real_index & 1) {
      int my_handedness = real_people->people[real_index].id1 ^ real_index;
      int next_index = (real_index + 1 - (my_handedness & 2)) & 3;
      int next_handedness = real_people->people[next_index].id1 ^ next_index;
      // We demand that the next person be real.
      return (next_handedness & (BIT_PERSON|3)) == (extra_stuff[0] | BIT_PERSON);
   }
   else {
      int center_xor = real_people->people[1].id1 ^ real_people->people[3].id1;
      if ((center_xor & (BIT_PERSON|3)) != 2) return false;
      return (int) (real_people->people[1].id1 & 3) == extra_stuff[0];
   }
}


// -3 means error, -2 means return false, -1 does not occur, and >= 0 means test that person.

/* ARGSUSED */
static bool check_tbone(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   int32_t z = extra_stuff[(real_index<<2) + real_direction];

   if (z == -2)
      return false;
   else if (z >= 0) {
      uint32_t zz = real_people->people[z].id1;
      if (zz & BIT_PERSON)
         return ((zz ^ real_people->people[real_index].id1) & 1) != 0;
      else if (real_people->kind == s_short6) {
         switch (real_people->cmd.cmd_assume.assumption) {
         case cr_diamond_like:
            return true;
         case cr_qtag_like:
            return false;
         case cr_jleft:
         case cr_jright:
         case cr_ijleft:
         case cr_ijright:
            return real_people->cmd.cmd_assume.assump_col != 0;
         }
      }
      else if (real_people->kind == s_trngl) {
         switch (real_people->cmd.cmd_assume.assumption) {
         case cr_jleft: case cr_jright:
            // If "assume normal diamonds" or "assume facing diamonds" is present,
            // the spot is t-boned for person zero.
            if (real_people->cmd.cmd_assume.assump_col == 4)
               return real_index == 0;
            break;
         case cr_tall6:
            return real_index == 0;
         default:
            if (real_index==0) {
               // Try looking at the other base person!
               uint32_t zz = real_people->people[z^3].id1;
               if (zz & BIT_PERSON) {
                  warn(warn__opt_for_not_tboned_base);
                  return ((zz ^ real_people->people[real_index].id1) & 1) != 0;
               }
            }
         }
      }
   }
   fail("Can't determine where to go or which way to face.");
   return false;
}

static const int32_t trnglspot_tboned_tab[12] = {-3,  2, -3,  1,    -2, -2,  0, 0,    -2,  0,  0, -2};
static const int32_t six2spot_tboned_tab[24]  = {-2, -2, -2, -2,    -3,  2, -3, 0,    -2, -2, -2, -2,
                                                 -2, -2, -2, -2,    -3, 3, -3, 5,     -2, -2, -2, -2};
static const int32_t mag62spot_tboned_tab[24] = {-3, -2, -3, -2,    -3,  3, -3, 5,    -3, -2, -3, -2,
                                                 -3, -2, -3, -2,    -3, 2, -3, 0,     -3, -2, -3, -2};

/* ARGSUSED */
static bool nextinttrnglspot_is_tboned(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   static const int32_t bb[24] = {-2, -2, 4, 4,     -3, 3, -3, 5,     -2, 4, 4, -2,
                                  1, 1, -2, -2,     -3, 2, -3, 0,     1, -2, -2, 1};
   static const int32_t cc[24] = {2, 2, -2, -2,     5, -2, -2, 5,     0, -3, 4, -3,
                                  -2, -2, 5, 5,     -2, 2, 2, -2,     1, -3, 3, -3};

   return check_tbone(real_people, real_index, real_direction, northified_index,
         (real_people->kind == s_short6) ? bb : cc);
}

/* ARGSUSED */
static bool next_galaxyspot_is_tboned(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   static const int32_t aa[32] = {1, -3, 7, -3,     2, 2, 0, 0,     -3, 3, -3, 1,     2, 4, 4, 2,
                                  3, -3, 5, -3,     4, 4, 6, 6 ,    -3, 5, -3, 7,     0, 6, 6, 0};

   /* We always return true for centers.  That way
      the centers can reverse flip a galaxy even if the
      next point does not exist.  Maybe this isn't a good way
      to do it, and we need another predicate.  Sigh. */

   if (real_index & 1) return true;

   return check_tbone(real_people, real_index, real_direction, northified_index, aa);
}

/* ARGSUSED */
static bool column_double_down(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   return
      (northified_index < 3)              // Unless #1 in column, it's easy.
         ||
      (northified_index > 4)
         ||
      // If #1, my adjacent end must exist and face in.
      ((uint32_t) ((((real_index + 2) & 4) >> 1) + 1) == (real_people->people[real_index ^ 7].id1 & 017));
}



/* ARGSUSED */
static bool apex_test(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   uint32_t unmoving_end = ((real_people->people[0].id1+NROLL_BIT) & (NROLL_BIT*2)) ? 2 : 0;
   uint32_t status;

   if ((real_people->people[0].id1 &
        real_people->people[1].id1 &
        real_people->people[2].id1 & BIT_PERSON) == 0)
      fail("Can't do this with phantoms.");

   status = (real_people->people[1].id1 ^ real_people->people[unmoving_end].id1) & 2;

   if ((uint32_t) real_index == unmoving_end ||
       (real_index == 1 && (real_people->people[1].id1 & 2) != unmoving_end))
      status |= 1;

   return (*extra_stuff & ~status) == 0;
}



/* ARGSUSED */
static bool boygirlp(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   // If this is a slide thru from a miniwave that is not right-handed, raise a warning.
   if (extra_stuff[2] && northified_index != 0)
      warn(warn__tasteless_slide_thru);

   return (real_people->people[real_index].id2 & extra_stuff[0]) != 0;
}

/* ARGSUSED */
static bool roll_is_cw(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   return (real_people->people[real_index].id1 & ROLL_DIRMASK) == ROLL_IS_R;
}

/* ARGSUSED */
static bool roll_is_ccw(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   return (real_people->people[real_index].id1 & ROLL_DIRMASK) == ROLL_IS_L;
}

/* ARGSUSED */
static bool slide_or_roll_is_cw(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   // Slide info takes precedence.
   if (real_people->people[real_index].id1 & NSLIDE_MASK)
      return (real_people->people[real_index].id1 & NSLIDE_MASK) == SLIDE_IS_L;

   return (real_people->people[real_index].id1 & ROLL_DIRMASK) == ROLL_IS_R;
}

/* ARGSUSED */
static bool slide_or_roll_is_ccw(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   // Slide info takes precedence.
   if (real_people->people[real_index].id1 & NSLIDE_MASK)
      return (real_people->people[real_index].id1 & NSLIDE_MASK) == SLIDE_IS_R;

   return (real_people->people[real_index].id1 & ROLL_DIRMASK) == ROLL_IS_L;
}

/* ARGSUSED */
static bool x12_with_other_sex(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int this_person2 = real_people->people[real_index].id2;
   int other_person2 = real_people->people[real_index ^ 1].id2;
   return (this_person2 & extra_stuff[0]) && (other_person2 & extra_stuff[1]);
}

/* ARGSUSED */
static bool x22_facing_other_sex(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int this_person2 = real_people->people[real_index].id2;
   int other_person2 = real_people->people[real_index ^ (((real_direction<<1) & 2) ^ 3)].id2;
   return (this_person2 & extra_stuff[0]) && (other_person2 & extra_stuff[1]);
}

/* ARGSUSED */
static bool lateral_to_sex(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   int this_person2 = real_people->people[real_index].id2 & (ID2_PERM_BOY|ID2_PERM_GIRL);
   int t = (real_people->kind == s1x2) ? 0 : (real_direction<<1) & 2;
   int other_person2 = real_people->people[real_index ^ t ^ 1].id2 & (ID2_PERM_BOY|ID2_PERM_GIRL);
   return this_person2 && other_person2 && ((this_person2 ^ other_person2) == *extra_stuff);
}

/* ARGSUSED */
static bool directionp(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   direction_used = true;
   direction_kind d = (direction_kind) extra_stuff[0];

   if (current_options.where == direction_the_music) {
      uint32_t r3 = 0;
      switch (d) {
      case direction_no_direction: r3 = ID3_FACEFRONT; break;
      case direction_left: r3 = ID3_FACELEFT; break;
      case direction_right: r3 = ID3_FACERIGHT; break;
      case direction_back: r3 = ID3_FACEBACK; break;
      }

      return (real_people->people[real_index].id3 & r3) != 0;
   }

   return current_options.where == d;
}



/* ARGSUSED */
static bool dmd_ctrs_rh(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   if (real_people->cmd.cmd_assume.assump_col == 0) {
      switch (real_people->cmd.cmd_assume.assumption) {
      case cr_jleft:
         if (real_people->cmd.cmd_assume.assump_both == 2) return *extra_stuff == 0;
         else if (real_people->cmd.cmd_assume.assump_both == 1) return *extra_stuff != 0;
         break;
      case cr_jright:
         if (real_people->cmd.cmd_assume.assump_both == 2) return *extra_stuff != 0;
         else if (real_people->cmd.cmd_assume.assump_both == 1) return *extra_stuff == 0;
         break;
      }
   }

   assumption_thing tt;
   bool booljunk;

   tt.assumption = cr_dmd_ctrs_mwv;
   tt.assump_col = 0;
   tt.assump_both = 1;
   tt.assump_negate = 0;
   tt.assump_live = 0;
   if (verify_restriction(real_people, tt, false, &booljunk) == restriction_passes)
      return *extra_stuff == 0;
   tt.assump_both = 2;
   if (verify_restriction(real_people, tt, false, &booljunk) == restriction_passes)
      return *extra_stuff != 0;

   fail("Can't determine handedness.");
   return false;
}

/* ARGSUSED */
static bool trngl_pt_rh(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff) THROW_DECL
{
   switch (real_people->people[0].id1 & d_mask) {
   case d_west:
      return true;
   case d_east:
      return false;
   default:
      fail("Can't determine handedness of triangle point.");
      return false;
   }
}


struct simple_qtag_action {
   int32_t ctr_action;    // -1 for true, -2 for false, else required direction xor.
   int32_t end_action;
   int32_t bbbbb;
};

struct full_qtag_action {
   simple_qtag_action if_jleft;
   simple_qtag_action if_jright;
   simple_qtag_action if_ijleft;
   simple_qtag_action if_ijright;
   simple_qtag_action if_tag;
   simple_qtag_action if_line;
   simple_qtag_action none;
};

static const full_qtag_action q_tag_front_action = {
   {-1,   1,  0},       // if_jleft
   {-1,   0,  1},       // if_jright
   {-2,  -1, 99},       // if_ijleft
   {-2,  -1, 99},       // if_ijright
   {-1,   0, 98},       // if_tag
   {-2,  -1, 99},       // if_line
   { 2, 010,  2}};      // none

static const full_qtag_action q_tag_back_action = {
   {-1,   0,  1},       // if_jleft
   {-1,   1,  0},       // if_jright
   {-2,  -1, 99},       // if_ijleft
   {-2,  -1, 99},       // if_ijright
   {-1,   2, 98},       // if_tag
   {-2,  -1, 99},       // if_line
   { 2, 012,  2}};      // none

static const full_qtag_action q_line_front_action = {
   {-2,  -1, 99},       // if_jleft
   {-2,  -1, 99},       // if_jright
   {-1,   1,  0},       // if_ijleft
   {-1,   0,  1},       // if_ijright
   {-2,  -1, 99},       // if_tag
   {-1,   0, 97},       // if_line
   { 0, 010,  0}};      // none

static const full_qtag_action q_line_back_action = {
   {-2,  -1, 99},       // if_jleft
   {-2,  -1, 99},       // if_jright
   {-1,   0,  1},       // if_ijleft
   {-1,   1,  0},       // if_ijright
   {-2,  -1, 99},       // if_tag
   {-1,   2, 97},       // if_line
   { 0, 012,  0}};      // none

/* ARGSUSED */
static bool q_tag_check(setup *real_people, int real_index,
   int real_direction, int northified_index, const int32_t *extra_stuff)
{
   const full_qtag_action *bigactionp = (const full_qtag_action *) extra_stuff;
   const simple_qtag_action *actionp;
   int both = real_people->cmd.cmd_assume.assump_both;

   switch (real_people->cmd.cmd_assume.assumption) {
   case cr_jleft: actionp = (const simple_qtag_action *) &(bigactionp->if_jleft); break;
   case cr_jright: actionp = (const simple_qtag_action *) &(bigactionp->if_jright); break;
   case cr_ijleft: actionp = (const simple_qtag_action *) &(bigactionp->if_ijleft); break;
   case cr_ijright: actionp = (const simple_qtag_action *) &(bigactionp->if_ijright); break;
   case cr_real_1_4_tag: actionp = (const simple_qtag_action *) &(bigactionp->if_tag); both = 2; break;
   case cr_real_3_4_tag: actionp = (const simple_qtag_action *) &(bigactionp->if_tag); both = 1; break;
   case cr_real_1_4_line: actionp = (const simple_qtag_action *) &(bigactionp->if_line); both = 2; break;
   case cr_real_3_4_line: actionp = (const simple_qtag_action *) &(bigactionp->if_line); both = 1; break;
   default: actionp = (const simple_qtag_action *) &(bigactionp->none); break;
   }

   if (real_index & 2) {
      // I am in the center line.
      if (actionp->ctr_action == -1) return true;
      else if (actionp->ctr_action == -2) return false;
      // This line is executed if there is no assumption.  It attempts to determine whether the physical setup
      // is a wave or a 2FL by checking just the subject and his partner.  Of course, a more thorough check
      // would be a nice idea.
      else return ((real_people->people[real_index].id1 ^ real_people->people[real_index ^ 1].id1) & DIR_MASK) == (uint32_t) actionp->ctr_action;
   }
   else {
      // I am on the outside; find the end of the center line nearest me.

      if (actionp->end_action < 0) return false;
      else if (actionp->end_action <= 2) {
         if (real_people->cmd.cmd_assume.assump_col == 4)
            return ((((real_index+3) >> 1) ^ real_people->people[real_index].id1) & 2) == ((actionp->end_action == 0) ? 0U : 2U);

         if (actionp->bbbbb == 98) {
            uint32_t t;

            both = 0;

            if ((t = real_people->people[2].id1))
               both |= actionp->end_action ^ t;

            if ((t = real_people->people[3].id1))
               both |= actionp->end_action ^ t ^ 2;

            if ((t = real_people->people[6].id1))
               both |= actionp->end_action ^ t ^ 2;

            if ((t = real_people->people[7].id1))
               both |= actionp->end_action ^ t;

            if (!both) return false;
            both >>= 1;
         }
         else if (actionp->bbbbb == 97) {
            uint32_t t;

            both = 0;

            if ((t = real_people->people[2].id1))
               both |= actionp->end_action ^ t;

            if ((t = real_people->people[3].id1))
               both |= actionp->end_action ^ t;

            if ((t = real_people->people[6].id1))
               both |= actionp->end_action ^ t ^ 2;

            if ((t = real_people->people[7].id1))
               both |= actionp->end_action ^ t ^ 2;

            if (!both) return false;
            both >>= 1;
         }
         else {
            if (actionp->bbbbb == 0) both ^= 1;
         }

         return ((real_index ^ both) & 1) != 0;
      }
      else {
         int z;
         if (real_index & 1) z = real_index ^ 3; else z = real_index ^ 6;

         // Demand that the indicated line end or diamond point be facing toward
         // or way from this person, as required by the front/back nature of
         // the predicate.

         // Also, demand that the line in the center consist of miniwaves or couples,
         // as required by the wave/line nature of the predicate.  But if the setup
         // is an hourglass, we waive the second test.  Hence "q_tag_front" and
         // "q_line_front" are indistinguishable in an hourglass.

         return
            ((real_people->people[z].id1 & 017) == ((uint32_t) actionp->end_action ^ (real_index >> 1))) &&
            (real_people->kind == s_hrglass ||
             (((real_people->people[z].id1 ^ real_people->people[z ^ 1].id1) & DIR_MASK) == (uint32_t) actionp->bbbbb));
      }
   }
}


// BEWARE!!  This list must track the array "predtab" in mkcalls.cpp.
// BEWARE!!  Obey the correctness of SELECTOR_PREDS.

// The first several of these (the ones before "SELECTOR_PREDS") take a selector.
// Any call that uses one of these predicates will have its "need_a_selector"
// flag set during initialization.

predicate_descriptor pred_table[] = {
      {someone_selected,               &iden_tab[0]},            // "select"
      {unselect,                     (const int32_t *) 0},       // "unselect"
      {select_near_select,           (const int32_t *) 0},       // "select_near_select"
      {select_near_select_or_phantom,(const int32_t *) 0},       // "select_near_select_or_phantom"
      {select_near_unselect,         (const int32_t *) 0},       // "select_near_unselect"
      {unselect_near_select,         (const int32_t *) 0},       // "unselect_near_select"
      {unselect_near_unselect,       (const int32_t *) 0},       // "unselect_near_unselect"
      {select_once_rem_from_select,  (const int32_t *) 0},       // "select_once_rem_from_select"
      {someone_selected,               &iden_tab[2]},            // "conc_from_select"
      {someone_selected,               &iden_tab[3]},            // "other_spindle_cw_select"
      {someone_selected,               &iden_tab[4]},            // "grand_conc_from_select"
      {someone_selected,               &iden_tab[5]},            // "other_diamond_point_select"
      {someone_selected,               &iden_tab[6]},            // "other_spindle_ckpt_select"
      {someone_selected,               &iden_tab[7]},            // "pair_person_select"
      {sum_mod_selected,               &iden_tab[5]},            // "person_select_sum5"
      {sum_mod_selected,               &iden_tab[8]},            // "person_select_sum8"
      {sum_mod_selected,               &iden_tab[9]},            // "person_select_sum9"
      {sum_mod_selected,              &iden_tab[11]},            // "person_select_sum11"
      {sum_mod_selected,              &iden_tab[13]},            // "person_select_sum13"
      {plus_mod_selected,             &iden_tab[1]},             // "person_select_plus1"
      {plus_mod_selected,             &iden_tab[2]},             // "person_select_plus2"
      {plus_mod_selected,             &iden_tab[3]},             // "person_select_plus3"
      {plus_mod_selected,             &iden_tab[4]},             // "person_select_plus4"
      {plus_mod_selected,             &iden_tab[5]},             // "person_select_plus5"
      {plus_mod_selected,             &iden_tab[6]},             // "person_select_plus6"
      {plus_mod_selected,             &iden_tab[7]},             // "person_select_plus7"
      {plus_mod_selected,             &iden_tab[8]},             // "person_select_plus8"
      {plus_mod_selected,             &iden_tab[9]},             // "person_select_plus9"
      {plus_mod_selected,             &iden_tab[10]},            // "person_select_plus10"
      {plus_mod_selected,             &iden_tab[11]},            // "person_select_plus11"
      {plus_mod_selected,             &iden_tab[12]},            // "person_select_plus12"
      {plus_mod_selected_real,        &iden_tab[1]},             // "person_select_real_plus1"
      {plus_mod_selected_real,        &iden_tab[2]},             // "person_select_real_plus2"
      {plus_mod_selected_real,        &iden_tab[3]},             // "person_select_real_plus3"
      {plus_mod_selected_real,        &iden_tab[4]},             // "person_select_real_plus4"
      {plus_mod_selected_real,        &iden_tab[5]},             // "person_select_real_plus5"
      {plus_mod_selected_real,        &iden_tab[6]},             // "person_select_real_plus6"
      {plus_mod_selected_real,        &iden_tab[7]},             // "person_select_real_plus7"
      {plus_mod_selected_real,        &iden_tab[8]},             // "person_select_real_plus8"
      {plus_mod_selected_real,        &iden_tab[9]},             // "person_select_real_plus9"
      {plus_mod_selected_real,        &iden_tab[10]},            // "person_select_real_plus10"
      {plus_mod_selected_real,        &iden_tab[11]},            // "person_select_real_plus11"
      {semi_squeezer_select,          semi_squeeze_tab},         // "semi_squeezer_select"
      {select_once_rem_from_unselect,(const int32_t *) 0},       // "select_once_rem_from_unselect"
      {unselect_once_rem_from_select,(const int32_t *) 0},       // "unselect_once_rem_from_select"
      {select_and_roll_is_cw,        (const int32_t *) 0},       // "select_and_roll_is_cw"
      {select_and_roll_is_ccw,       (const int32_t *) 0},       // "select_and_roll_is_ccw"
      {x12_side_of_line_facing,       &iden_tab[0]},             // "1x2_selectee_is_linelike_facing_cw"
      {x12_side_of_line_facing,       &iden_tab[2]},             // "1x2_selectee_is_linelike_facing_ccw"
      {x14_side_of_line_facing,       &iden_tab[9]},             // "1x4_selectee_of_far_side_is_linelike_facing_cw"
      {x14_side_of_line_facing,       &iden_tab[11]},            // "1x4_selectee_of_far_side_is_linelike_facing_ccw"
      {kicker_coming,                (const int32_t *) 0},       // "kicker_coming"
// End of predicates that force use of selector.
#define SELECTOR_PREDS 52
      {always,                       (const int32_t *) 0},       // "always"
      {plus_mod_real,                 &iden_tab[1]},             // "person_real_plus1"
      {plus_mod_real,                 &iden_tab[2]},             // "person_real_plus2"
      {plus_mod_real,                 &iden_tab[3]},             // "person_real_plus3"
      {plus_mod_real,                 &iden_tab[4]},             // "person_real_plus4"
      {plus_mod_real,                 &iden_tab[5]},             // "person_real_plus5"
      {plus_mod_real,                 &iden_tab[6]},             // "person_real_plus6"
      {plus_mod_real,                 &iden_tab[7]},             // "person_real_plus7"
      {plus_mod_real,                 &iden_tab[8]},             // "person_real_plus8"
      {plus_mod_real,                 &iden_tab[9]},             // "person_real_plus9"
      {plus_mod_real,                 &iden_tab[10]},            // "person_real_plus10"
      {plus_mod_real,                 &iden_tab[11]},            // "person_real_plus11"
      {plus_mod_real,                 &iden_tab[12]},            // "person_real_plus12"
      {plus_mod_real,                 &iden_tab[13]},            // "person_real_plus13"
      {plus_mod_real,                 &iden_tab[14]},            // "person_real_plus14"
      {plus_mod_real,                 &iden_tab[15]},            // "person_real_plus15"
      {plus_mod_real,                 &iden_tab[16]},            // "person_real_plus16"
      {plus_mod_real,                 &iden_tab[17]},            // "person_real_plus17"
      {plus_mod_real,                 &iden_tab[18]},            // "person_real_plus18"
      {plus_mod_real,                 &iden_tab[19]},            // "person_real_plus19"
      {sum_mod_real,                  &iden_tab[5]},             // "person_real_sum5"
      {sum_mod_real,                  &iden_tab[8]},             // "person_real_sum8"
      {sum_mod_real,                  &iden_tab[9]},             // "person_real_sum9"
      {sum_mod_real,                  &iden_tab[11]},            // "person_real_sum11"
      {sum_mod_real,                  &iden_tab[13]},            // "person_real_sum13"
      {sum_mod_real,                  &iden_tab[24]},            // "person_real_sum24"
      {x22_cpltest,                    dbl_tab21},               // "2x2_miniwave"
      {x22_cpltest,                    dbl_tab01},               // "2x2_couple"
      {x22_cpltest,                    dbl_tab21n},              // "2x2_miniwave_nocycle_wheel"
      {x22_cpltest,                    dbl_tab01n},              // "2x2_couple_nocycle_wheel"
      {facing_test,                    x22tabtandem},            // "2x2_tandem_with_someone"
      {facing_test,                    x22tabantitandem},        // "2x2_antitandem"
      {facing_test,                    x22tabfacing},            // "2x2_facing_someone"
      {facing_test,                    x24tabtandem},            // "2x4_tandem_with_someone"
      {facing_test,                    x24tabantitandem},        // "2x4_antitandem"
      {facing_test,                    x24tabfacing},            // "2x4_facing_someone"
      {x14_side_of_line_facing,       &iden_tab[0]},             // "1x4_end_of_this_side_is_linelike_facing_cw"
      {x14_side_of_line_facing,       &iden_tab[1]},             // "1x4_end_of_far_side_is_linelike_facing_cw"
      {x14_side_of_line_facing,       &iden_tab[2]},             // "1x4_end_of_this_side_is_linelike_facing_ccw"
      {x14_side_of_line_facing,       &iden_tab[3]},             // "1x4_end_of_far_side_is_linelike_facing_ccw"
      {x14_side_of_line_facing,       &iden_tab[4]},             // "1x4_center_of_this_side_is_linelike_facing_cw"
      {x14_side_of_line_facing,       &iden_tab[5]},             // "1x4_center_of_far_side_is_linelike_facing_cw"
      {x14_side_of_line_facing,       &iden_tab[6]},             // "1x4_center_of_this_side_is_linelike_facing_ccw"
      {x14_side_of_line_facing,       &iden_tab[7]},             // "1x4_center_of_far_side_is_linelike_facing_ccw"
      {cols_someone_in_front,        (const int32_t *) 0},       // "columns_someone_in_front"
      {x14_once_rem_miniwave,         &iden_tab[1]},             // "x14_once_rem_miniwave"
      {x14_once_rem_couple,          (const int32_t *) 0},       // "x14_once_rem_couple"
      {lines_miniwave,               (const int32_t *) 0},       // "lines_miniwave"
      {lines_couple,                 (const int32_t *) 0},       // "lines_couple"
      {check_3n1_setup,              tab_mwv_in_3n1},            // "miniwave_side_of_in_3n1_line"
      {check_3n1_setup,              tab_cpl_in_3n1},            // "couple_side_of_in_3n1_line"
      {check_3n1_setup,             tab_mwv_out_3n1},            // "miniwave_side_of_out_3n1_line"
      {check_3n1_setup,             tab_cpl_out_3n1},            // "couple_side_of_out_3n1_line"
      {check_3n1_setup,              tab_mwv_in_3n1},            // "antitandem_side_of_in_3n1_col"
      {check_3n1_setup,              tab_cpl_in_3n1},            // "tandem_side_of_in_3n1_col"
      {check_3n1_setup,             tab_mwv_out_3n1},            // "antitandem_side_of_out_3n1_col"
      {check_3n1_setup,             tab_cpl_out_3n1},            // "tandem_side_of_out_3n1_col"
      {some_side_of_2n1_line,          &iden_tab[0]},            // "miniwave_side_of_2n1_line"
      {some_side_of_2n1_line,          &iden_tab[3]},            // "couple_side_of_2n1_line"
      {some_side_of_2n1_line,          &iden_tab[0]},            // "antitandem_side_of_2n1_col"
      {some_side_of_2n1_line,          &iden_tab[3]},            // "tandem_side_of_2n1_col"
      {cast_normal_or_whatever,        &iden_tab[1]},            // "cast_normal"
      {cast_normal_or_whatever,        &iden_tab[0]},            // "cast_pushy"
      {cast_normal_or_whatever,        &iden_tab[3]},            // "cast_normal_or_warn"
      {cast_normal_or_whatever,        &iden_tab[7]},            // "cast_normal_or_nowarn"
      {x14_once_rem_miniwave,          &iden_tab[3]},            // "intlk_cast_normal_or_warn"
      {opp_in_magic,                 (const int32_t *) 0},       // "lines_magic_miniwave"
      {same_in_magic,                (const int32_t *) 0},       // "lines_magic_couple"
      {once_rem_test,                  &iden_tab[2]},            // "lines_once_rem_miniwave"
      {once_rem_test,                  &iden_tab[0]},            // "lines_once_rem_couple"
      {same_in_pair,                 (const int32_t *) 0},       // "lines_tandem"
      {opp_in_pair,                  (const int32_t *) 0},       // "lines_antitandem"
      {columns_tandem,                 &iden_tab[0]},            // "columns_tandem"
      {columns_tandem,                 &iden_tab[1]},            // "columns_antitandem"
      {same_in_magic,                (const int32_t *) 0},       // "columns_magic_tandem"
      {opp_in_magic,                 (const int32_t *) 0},       // "columns_magic_antitandem"
      {once_rem_test,                  &iden_tab[0]},            // "columns_once_rem_tandem"
      {once_rem_test,                  &iden_tab[2]},            // "columns_once_rem_antitandem"
      {same_in_pair,                 (const int32_t *) 0},       // "columns_couple"
      {opp_in_pair,                  (const int32_t *) 0},       // "columns_miniwave"
      {x12_beau_or_miniwave,           &iden_tab[0]},            // "1x2_beau_or_miniwave"
      {x12_beau_or_miniwave,           &iden_tab[1]},            // "1x2_beau_miniwave_or_warn"
      {x12_beau_or_miniwave,           &iden_tab[2]},            // "1x2_beau_miniwave_for_breaker"
      {x12_beau_or_miniwave,           &iden_tab[3]},            // "1x2_beau_miniwave_or_ok"
      {can_swing_left,                 &iden_tab[0]},            // "can_swing_left"
      {can_swing_left,                 &iden_tab[1]},            // "can_swing_right"
      {x14_wheel_and_deal,             &iden_tab[1]},            // "1x4_wheel_and_deal"
      {x14_wheel_and_deal,             &iden_tab[0]},            // "1x4_wheel_and_deal_or_1fl"
      {x16_wheel_and_deal,           (const int32_t *) 0},       // "1x6_wheel_and_deal"
      {x18_wheel_and_deal,           (const int32_t *) 0},       // "1x8_wheel_and_deal"
      {cycle_and_wheel_1,            (const int32_t *) 0},       // "cycle_and_wheel_1"
      {cycle_and_wheel_2,            (const int32_t *) 0},       // "cycle_and_wheel_2"
      {vert1,                        (const int32_t *) 0},       // "vert1"
      {vert2,                        (const int32_t *) 0},       // "vert2"
      {inner_active_lines,           (const int32_t *) 0},       // "inner_active_lines"
      {outer_active_lines,           (const int32_t *) 0},       // "outer_active_lines"
      {judge_check,                           jr1x4},            // "judge_is_right"
      {judge_check,                           jl1x4},            // "judge_is_left"
      {judge_check,                           sr1x4},            // "socker_is_right"
      {judge_check,                           sl1x4},            // "socker_is_left"
      {in_out_roll_select, (const int32_t *) &inroller_cw},      // "inroller_is_cw"
      {in_out_roll_select, (const int32_t *) &magic_inroller_cw}, // "magic_inroller_is_cw"
      {in_out_roll_select, (const int32_t *) &outroller_cw},     // "outroller_is_cw"
      {in_out_roll_select, (const int32_t *) &magic_outroller_cw}, // "magic_outroller_is_cw"
      {in_out_roll_select, (const int32_t *) &inroller_cw_2x3},  // "inroller_is_cw_2x3"
      {in_out_roll_select, (const int32_t *) &magic_inroller_cw_2x3}, // "magic_inroller_is_cw_2x3"
      {in_out_roll_select, (const int32_t *) &outroller_cw_2x3}, // "outroller_is_cw_2x3"
      {in_out_roll_select, (const int32_t *) &magic_outroller_cw_2x3}, // "magic_outroller_is_cw_2x3"
      {in_out_roll_select, (const int32_t *) &inroller_2x6},     // "inroller_is_cw_2x6"
      {in_out_roll_select, (const int32_t *) &outroller_2x6},    // "outroller_is_cw_2x6"
      {in_out_roll_select, (const int32_t *) &inroller_2x8},     // "inroller_is_cw_2x8"
      {in_out_roll_select, (const int32_t *) &outroller_2x8},    // "outroller_is_cw_2x8"
      {outposter_is_cw,              (const int32_t *) 0},       // "outposter_is_cw"
      {outposter_is_ccw,             (const int32_t *) 0},       // "outposter_is_ccw"
      {raise_some_sglfile,           (const int32_t *) 0},       // "raise_some_sglfile"
      {count_cw_people,                &iden_tab[0]},            // "zero_cw_people"
      {count_cw_people,                &iden_tab[1]},            // "one_cw_person"
      {count_cw_people,                &iden_tab[2]},            // "two_cw_people"
      {count_cw_people,                &iden_tab[3]},            // "three_cw_people"
      {check_4x4_quad,                 &iden_tab[14]},           // "quad_person_cw"
      {check_4x4_quad,                 &iden_tab[11]},           // "quad_person_ccw"
      {check_facing_dmd_spot,          &iden_tab[2]},            // "next_dmd_spot_is_facing"
      {check_facing_dmd_spot,          &iden_tab[0]},            // "next_dmd_spot_is_normal"
      {check_qtag_spot_facing_me,      &iden_tab[1]},            // "next_qtag_spot_faces_me"
      {check_qtag_spot_facing_me,      &iden_tab[3]},            // "next_qtag_spot_faces_away"
      {check_tbone,            trnglspot_tboned_tab},            // "nexttrnglspot_is_tboned"
      {nextinttrnglspot_is_tboned,   (const int32_t *) 0},       // "nextinttrnglspot_is_tboned"
      {check_tbone,             six2spot_tboned_tab},            // "next62spot_is_tboned"
      {check_tbone,            mag62spot_tboned_tab},            // "next_magic62spot_is_tboned"
      {next_galaxyspot_is_tboned,    (const int32_t *) 0},       // "next_galaxyspot_is_tboned"
      {column_double_down,           (const int32_t *) 0},       // "column_double_down"
      {apex_test,                      &iden_tab[1]},            // "apex_test_1"
      {apex_test,                      &iden_tab[2]},            // "apex_test_2"
      {apex_test,                      &iden_tab[3]},            // "apex_test_3"
      {boygirlp,                     boystuff_no_rh},            // "boyp"
      {boygirlp,                    girlstuff_no_rh},            // "girlp"
      {boygirlp,                        boystuff_rh},            // "boyp_rh_slide_thru"
      {boygirlp,                       girlstuff_rh},            // "girlp_rh_slide_thru"
      {roll_is_cw,                   (const int32_t *) 0},       // "roll_is_cw"
      {roll_is_ccw,                  (const int32_t *) 0},       // "roll_is_ccw"
      {slide_or_roll_is_cw,          (const int32_t *) 0},       // "slide_or_roll_is_cw"
      {slide_or_roll_is_ccw,         (const int32_t *) 0},       // "slide_or_roll_is_ccw"
      {x12_with_other_sex,            boystuff_no_rh},           // "x12_boy_with_girl"
      {x12_with_other_sex,           girlstuff_no_rh},           // "x12_girl_with_boy"
      {x22_facing_other_sex,          boystuff_no_rh},           // "x22_boy_facing_girl"
      {x22_facing_other_sex,         girlstuff_no_rh},           // "x22_girl_facing_boy"
      {lateral_to_sex,                 &samesex[0]},             // "lateral_to_same_sex"
      {lateral_to_sex,                 &samesex[1]},             // "lateral_to_opposite_sex"
      {directionp,           &iden_tab[direction_left]},         // "leftp"
      {directionp,           &iden_tab[direction_right]},        // "rightp"
      {directionp,           &iden_tab[direction_in]},           // "inp"
      {directionp,           &iden_tab[direction_out]},          // "outp"
      {directionp,           &iden_tab[direction_back]},         // "backp"
      {directionp,           &iden_tab[direction_zigzag]},       // "zigzagp"
      {directionp,           &iden_tab[direction_zagzig]},       // "zagzigp"
      {directionp,           &iden_tab[direction_zigzig]},       // "zigzigp"
      {directionp,           &iden_tab[direction_zagzag]},       // "zagzagp"
      {directionp,           &iden_tab[direction_no_direction]}, // "no_dir_p"
      {dmd_ctrs_rh,                    &iden_tab[0]},            // "dmd_ctrs_rh"
      {dmd_ctrs_rh,                    &iden_tab[1]},            // "dmd_ctrs_lh"
      {trngl_pt_rh,                  (const int32_t *) 0},       // "trngl_pt_rh"
      {q_tag_check, (const int32_t *) &q_tag_front_action},      // "q_tag_front"
      {q_tag_check, (const int32_t *) &q_tag_back_action},       // "q_tag_back"
      {q_tag_check, (const int32_t *) &q_line_front_action},     // "q_line_front"
      {q_tag_check, (const int32_t *) &q_line_back_action},      // "q_line_back"
      {whos_on_base,                   &iden_tab[0]},            // "base_is_across"
      {whos_on_base,                   &iden_tab[4]},            // "base_is_right"
      {whos_on_base,                   &iden_tab[8]},            // "base_is_here"
      {whos_on_base,                   &iden_tab[12]}};          // "base_is_left"

int selector_preds = SELECTOR_PREDS;
