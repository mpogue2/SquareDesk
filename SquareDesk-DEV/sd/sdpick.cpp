// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2013  William B. Ackerman.
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
//    This is for version 37.

/* This defines the following functions:
   in_exhaustive_search
   reset_internal_iterators
   do_selector_iteration
   do_direction_iteration
   do_number_iteration
   do_tagger_iteration
   do_circcer_iteration
   pick_concept
   do_pick
   get_resolve_goodness_info
   pick_allow_multiple_items
   start_pick
   end_pick
   forbid_call_with_mandatory_subcall
   allow_random_subcall_pick

and the following external variables:
   search_goal
*/

#include "sd.h"
#include "sdui.h"


// BEWARE!!  This list must track the array "pick_type_table".
enum pick_type {
   pick_starting_first_scan,
   pick_plain_nice_only,       // Just calls, very picky about quality.
   pick_concept_nice_only,     // Concept/call, very picky about quality.
   pick_plain_accept_all,      // Just calls, but accept anything.
   pick_concept_accept_all,    // Concept/call, accept anything.
   pick_in_random_search,
   pick_not_in_any_pick_at_all
};


struct pick_type_descriptor {
   bool exhaustive_search;
   bool accept_nice_only;
   bool with_concept;
   const char *display_string;
};


// BEWARE!!  This list is keyed to the definition of "pick_type".
pick_type_descriptor pick_type_table[] = {
   { false, false, false, "pick start first scan"},  // pick_starting_first_scan
   { true,  true,  false, "pick plain nice scan"},   // pick_plain_nice_only
   { true,  true,  true,  "pick concept nice scan"}, // pick_concept_nice_only
   { true,  false, false, "pick plain any scan"},    // pick_plain_accept_all
   { true,  false, true,  "pick concept any scan"},  // pick_concept_accept_all
   { false, false, false, "pick random search"},     // pick_in_random_search
   { false, false, false, 0}};                       // pick_not_in_any_pick_at_all

command_kind search_goal;

/* The "pick_concept_***" passes are actually several scans over several
   concepts.  This counter counts them. */
static int concept_scan_index;
static int concept_scan_limit;
static short int *concept_scan_table;

static uint32 selector_iterator = 0;
static uint32 direction_iterator = 0;
static uint32 number_iterator = 0;
static uint32 tagger_iterator = 0;
static uint32 circcer_iterator = 0;
static int resolve_scan_start_point;
static int resolve_scan_current_point;
// This is only meaningful if interactivity = interactivity_picking.
static pick_type current_pick_type = pick_not_in_any_pick_at_all;


static void display_pick()
{
   gg77->iob88.set_pick_string(pick_type_table[current_pick_type].display_string);
}



bool in_exhaustive_search()
{
   return pick_type_table[current_pick_type].exhaustive_search;
}


void reset_internal_iterators()
{
   selector_iterator = 0;
   direction_iterator = 0;
   number_iterator = 0;
   tagger_iterator = 0;
   circcer_iterator = 0;
}

selector_kind do_selector_iteration(bool allow_iteration)
{
   static selector_kind selector_iterator_table[] = {
      selector_boys,
      selector_girls,
      selector_centers,
      selector_ends,
      selector_leads,
      selector_trailers,
      selector_beaus,
      selector_belles,
      selector_uninitialized};

   if (interactivity == interactivity_database_init ||
       interactivity == interactivity_verify) {
      if (verify_options.who == selector_uninitialized) {
         verify_used_selector = true;
         return selector_for_initialize;
      }
      else
         return verify_options.who;
   }

   int j;

   if (pick_type_table[current_pick_type].exhaustive_search && allow_iteration) {
      j = (int) selector_iterator_table[selector_iterator];

      selector_iterator++;

      /* See if we have exhausted all possible selectors.
         We only look for "boys", "girls", "centers", and "ends" in the first scan. */
      if (selector_iterator_table[selector_iterator] ==
          ((current_pick_type == pick_plain_nice_only) ?
           selector_leads :
           ((calling_level < beau_belle_level) ?
            selector_beaus :
            selector_uninitialized)))
         selector_iterator = 0;
   }
   else if (pick_type_table[current_pick_type].exhaustive_search) {
      /* In a concept during exhaustive search.  That means we are doing
         the "<anyone> are tandem" stuff. */
      j = (int) selector_centers;
   }
   else {
      // We don't generate unsymmetrical selectors when searching.  It generates
      // too many "couple #3 u-turn-back" calls.
      j = generate_random_number(noresolve_SELECTOR_START-1)+1;
   }

   hash_nonrandom_number(j-1);
   return (selector_kind) j;
}


direction_kind do_direction_iteration()
{
   if (interactivity == interactivity_database_init ||
       interactivity == interactivity_verify) {
      if (verify_options.where == direction_uninitialized) {
         verify_used_direction = true;
         return direction_for_initialize;
      }
      else
         return verify_options.where;
   }

   int j;

   if (pick_type_table[current_pick_type].exhaustive_search) {
      static direction_kind direction_iterator_table[] = {
         direction_left,
         direction_right,
         direction_in,
         direction_out,
         direction_uninitialized};

      j = (int) direction_iterator_table[direction_iterator];

      if (selector_iterator == 0) {
         direction_iterator++;

         /* See if we have exhausted all possible directions.
            We only look for "left" and "right" in the first scan. */
         if (direction_iterator_table[direction_iterator] ==
             ((current_pick_type == pick_plain_nice_only) ?
              direction_in :
              direction_uninitialized))
            direction_iterator = 0;
      }
   }
   else {
      j = generate_random_number(last_direction_kind)+1;
   }

   hash_nonrandom_number(j-1);
   return (direction_kind) j;
}


void do_number_iteration(int howmanynumbers,
                         uint32 odd_number_only,
                         bool allow_iteration,
                         uint32 *number_list)
{
   int i;

   *number_list = 0;

   if (interactivity == interactivity_database_init ||
       interactivity == interactivity_verify) {
      for (i=0 ; i<howmanynumbers ; i++) {
         uint32 this_num;

         if (verify_options.howmanynumbers == 0) {
            // The second number in the series is always 1.
            // This makes "N-N-N-N change the web" and "N-N-N-N
            // relay the top" work.
            this_num = (i==1) ? 1 : number_for_initialize;
            verify_used_number = true;
         }
         else {
            this_num = verify_options.number_fields & NUMBER_FIELD_MASK;
            verify_options.number_fields >>= BITS_PER_NUMBER_FIELD;
            verify_options.howmanynumbers--;
         }

         *number_list |= (this_num << (i*BITS_PER_NUMBER_FIELD));

         if (odd_number_only && !(this_num & 1))
            *number_list = ~0U;     // Set it illegal.
      }

      return;
   }

   for (i=0 ; i<howmanynumbers ; i++) {
      uint32 this_num;

      if (allow_iteration &&
          pick_type_table[current_pick_type].exhaustive_search) {
         this_num = ((number_iterator >> (i*2)) & 3) + 1;
      }
      else if (odd_number_only)
         this_num = (generate_random_number(2)<<1)+1;
      else
         this_num = generate_random_number(4)+1;

      hash_nonrandom_number(this_num-1);

      *number_list |= (this_num << (i*BITS_PER_NUMBER_FIELD));
   }

   if (pick_type_table[current_pick_type].exhaustive_search &&
       (selector_iterator | direction_iterator) == 0) {
      number_iterator++;

      // If the call requires it, iterate over odd numbers only.
      if (odd_number_only) {
         while (number_iterator & 0x55555555)
            number_iterator += number_iterator & ~(number_iterator-1);
      }

      // If the call takes 3 or more numbers, we don't iterate in the
      // "nice" scans.  The reason is that we don't want to wast a lot
      // of time enumerating lots of combinations of i-j-k-l quarter
      // the deucey.  We will get to them on the random scan in any
      // case.

      if (howmanynumbers >= 3) {
         number_iterator = 0;
      }

      // See if we have exhausted all possible numbers.
      if (number_iterator >> (howmanynumbers*2)) number_iterator = 0;
   }
}


bool do_tagger_iteration(uint32 tagclass,
                         uint32 *tagg,
                         uint32 numtaggers,
                         call_with_name **tagtable)
{
   uint32 tag;

   if (pick_type_table[current_pick_type].exhaustive_search) {
      tag = tagger_iterator;

         /* But we don't allow any call that takes a another tag call (e.g. "revert"),
            or any other iteratable modifier.  It's just too complicated to iterate
            over the space of all calls when things like this happen.
            We also reject any that are marked not to be used in resolve. */

      while (tag < numtaggers &&
             ((tagtable[tag]->the_defn.callflags1 &
               (CFLAG1_DONT_USE_IN_RESOLVE|
                CFLAG1_NUMBER_MASK)) ||
              (tagtable[tag]->the_defn.callflagsf &
               (CFLAGH__TAG_CALL_RQ_MASK|
                CFLAGH__CIRC_CALL_RQ_BIT|
                CFLAGH__REQUIRES_SELECTOR|
                CFLAGH__REQUIRES_DIRECTION))))
         tag++;

      if (tag == numtaggers && tagger_iterator == 0)
         return true;  /* There simply are no acceptable taggers. */

      if ((selector_iterator | direction_iterator | number_iterator) == 0) {
         tagger_iterator = tag+1;

         while (tagger_iterator < numtaggers &&
                ((tagtable[tagger_iterator]->the_defn.callflags1 &
                  (CFLAG1_DONT_USE_IN_RESOLVE|
                   CFLAG1_NUMBER_MASK)) ||
                 (tagtable[tagger_iterator]->the_defn.callflagsf &
                  (CFLAGH__TAG_CALL_RQ_MASK|
                   CFLAGH__CIRC_CALL_RQ_BIT|
                   CFLAGH__REQUIRES_SELECTOR|
                   CFLAGH__REQUIRES_DIRECTION))))
            tagger_iterator++;

         /* See if we have exhausted all possible taggers. */
         if (tagger_iterator == numtaggers)
            tagger_iterator = 0;
      }
   }
   else {
      tag = generate_random_number(numtaggers);
   }

   hash_nonrandom_number(tag);

   /* We don't generate "dont_use_in_resolve" taggers in any random search. */
   if (tagtable[tag]->the_defn.callflags1 & CFLAG1_DONT_USE_IN_RESOLVE)
      fail_no_retry("This shouldn't get printed.");

   *tagg = (tagclass << 5) | (tag+1);
   return false;
}


void do_circcer_iteration(uint32 *circcp)
{
   if (pick_type_table[current_pick_type].exhaustive_search) {
      *circcp = circcer_iterator+1;

      if ((selector_iterator | direction_iterator | number_iterator | tagger_iterator) == 0) {
         circcer_iterator++;

         /* See if we have exhausted all possible circcers. */
         if (circcer_iterator == number_of_circcers)
            circcer_iterator = 0;
      }
   }
   else
      *circcp = generate_random_number(number_of_circcers)+1;

   hash_nonrandom_number(*circcp - 1);
}



// When we get here, resolve_scan_current_point has the *last* call that we
// did.  If the iterators are nonzero, we will just repeat that call.
// Otherwise, we will advance it to the next and use that call.

const concept_descriptor *pick_concept(bool already_have_concept_in_place)
{
   bool do_concept = false;

   if (interactivity != interactivity_picking)
      return (concept_descriptor *) 0;

   if (current_pick_type == pick_starting_first_scan) {

      // Generate the random starting point for the scan, so it won't be identical
      // each time we resolve from this position.  This is the only time that we use
      // the random number generator during the initial scans.  Note that this random
      // number will NOT be hashed.  But don't use a random number if in diagnostic mode.

      // Note also that this starting point for the call scan lies within
      // the available calls for the setup we are in.  Some scans will be
      // across the available calls for "any" setup, because we will be
      // doing concepts.  But "any" setup always allows more calls, so we are safe.

      resolve_scan_start_point =
         (ui_options.diagnostic_mode) ?
         0 :
         generate_random_number(number_of_calls[parse_state.call_list_to_use]);
      resolve_scan_current_point = resolve_scan_start_point-1;
      current_pick_type = pick_plain_nice_only;
      display_pick();
      reset_internal_iterators();
   }
   else if (pick_type_table[current_pick_type].exhaustive_search) {
      if (already_have_concept_in_place)
         return (concept_descriptor *) 0;

      if ((selector_iterator | direction_iterator | number_iterator |
           tagger_iterator | circcer_iterator) == 0) {
         if (resolve_scan_current_point == resolve_scan_start_point) {

            // Done with this scan.  Advance to the next thing to do.

            if (pick_type_table[current_pick_type].with_concept) {
               // Currently doing a scan with concepts.  Go to the next
               // concept.  If run out, go to next scan.
               concept_scan_index++;
               if (concept_scan_index < concept_scan_limit)
                  goto foobar;
            }

            // Now we are really doing the next major scan type.

            // We want to advance to the next choice in an enumeration type.
            // The C++ language quite rightly doesn't allow this, since it
            // takes types seriously and doesn't consider an enumeration
            // to be nothing but a funny form for an integer.  Shame on us.
            // So we use a type cast.  Shame on us.

            current_pick_type = (pick_type) (current_pick_type+1);
            display_pick();

            concept_scan_index = 0;  // If this is a concept scan, we will need these.
            concept_scan_limit = good_concept_sublist_sizes[parse_state.call_list_to_use];
            concept_scan_table = good_concept_sublists[parse_state.call_list_to_use];

            // Now, if we are in a scan that involves concepts, check whether
            // there are any concepts to use.  (There might not be --
            // we severely restrict the number of available concepts in these scans.)

            if (search_goal != command_resolve ||

                // Well, we *always* skip this stuff now.
                // It's just bad.

                true ||

                concept_scan_limit == 0) {
               // Can't go into concept scans, so skip over any of same.
               while (pick_type_table[current_pick_type].with_concept)
                  current_pick_type = (pick_type) (current_pick_type+1);
               display_pick();
            }


         foobar: ;
         }

         resolve_scan_current_point--;   /* Might go to -1.  Do_pick will fix same. */
      }
   }
   else if (current_pick_type == pick_in_random_search) {
      switch (search_goal) {
      case command_concept_call:
         if (!already_have_concept_in_place)
            { do_concept = true; break; }
         /* FALL THROUGH!!!!! */
      case command_resolve:
      case command_random_call:
      case command_standardize:
         do_concept = generate_random_number(8) <
            ((search_goal == command_standardize) ?
             STANDARDIZE_CONCEPT_PROBABILITY : CONCEPT_PROBABILITY);

         /* Since we are not going to use the random number in a one-to-one way,
            we run the risk of not having hashed_randoms uniquely represent
            what is happening.  To remedy the problem, we hash just the yes-no
            result of our decision. */

         hash_nonrandom_number((int) do_concept);
         break;
      }
   }

   if (do_concept) {
      int j = concept_sublist_sizes[parse_state.call_list_to_use];

      if (j != 0) {    /* If no concepts are available (perhaps some clown has
                          selected "pick concept call" at mainstream) we don't
                          insert a concept. */
         j = generate_random_number(j);

         global_reply.minorpart = concept_sublists[parse_state.call_list_to_use][j];
         return &concept_descriptor_table[global_reply.minorpart];
      }
   }
   else if (pick_type_table[current_pick_type].with_concept &&
            !already_have_concept_in_place) {
      global_reply.minorpart = concept_scan_table[concept_scan_index];
      return &concept_descriptor_table[global_reply.minorpart];
   }

   return (concept_descriptor *) 0;
}


call_with_name *do_pick()
{
   int i;
   uint32 rejectflag;
   call_with_name *result;

   if (pick_type_table[current_pick_type].exhaustive_search) {
      if (resolve_scan_current_point < 0)
         resolve_scan_current_point = number_of_calls[parse_state.call_list_to_use]-1;
      i = resolve_scan_current_point;
   }
   else        /* In random search. */
      i = generate_random_number(number_of_calls[parse_state.call_list_to_use]);

   /* Fix up the "hashed randoms" stuff as though we had generated this number
         through the random number generator. */

   hash_nonrandom_number(i);
   result = main_call_lists[parse_state.call_list_to_use][i];

   // Why don't we just call the random number generator again if the call is inappropriate?
   // Wouldn't that be much faster?  There are two reasons:  First, we would need to take
   // special precautions against an infinite loop.  Second, and more importantly, if we
   // just called the random number generator again, it would screw up the hash numbers,
   // which would make the uniquefication fail, so we could see the same thing twice.

   if ((search_goal == command_level_call || search_goal == command_8person_level_call) &&
       ((dance_level) result->the_defn.level) < level_threshholds_for_pick[calling_level])
      fail_no_retry("Level reject.");

   rejectflag = pick_type_table[current_pick_type].exhaustive_search ?
      (CFLAG1_DONT_USE_IN_RESOLVE|CFLAG1_DONT_USE_IN_NICE_RESOLVE) :
      CFLAG1_DONT_USE_IN_RESOLVE;

   if (result->the_defn.callflags1 & rejectflag) fail_no_retry("This shouldn't get printed.");
   return result;
}


resolve_goodness_test get_resolve_goodness_info()
{
   if (interactivity == interactivity_picking) {
      if (pick_type_table[current_pick_type].accept_nice_only)
         /* "Nice" exhaustive scans:  Only accept things with "how_bad" = 0,
            that is, RLG, LA, and prom.  And only if the promenade is short
            (if at C2 or above).  And only if one call long. */
         return resolve_goodness_only_nice;
      else if (current_pick_type != pick_in_random_search)
         /* Other exhaustive scans: accept any one-call resolve. */
         return resolve_goodness_always;
   }

   /* Anything else: Accept resolves randomly.  Do this during the random search. */

   return resolve_goodness_maybe;
}


bool pick_allow_multiple_items()
{
   return !pick_type_table[current_pick_type].exhaustive_search;
}


void start_pick()
{
   /* Following a suggestion of Eric Brosius, we initially scan the entire database once,
      looking for one-call resolves, before we start the complex search.  This way, we
      will never show a multiple-call resolve if a single-call one exists.  Of course,
      it's not really that simple -- if a call takes a number, direction, or person,
      we will only use one canned value for it, so we could miss a single call resolve
      on this first pass if that call involves an interesting number, etc. */

   interactivity = interactivity_picking;

   if (search_goal == command_resolve ||
       search_goal == command_reconcile ||
       search_goal == command_normalize ||
       search_goal == command_standardize ||
       search_goal >= command_create_any_lines)
      current_pick_type = pick_starting_first_scan;
   else
      current_pick_type = pick_in_random_search;

   display_pick();
}


void end_pick()
{
   current_pick_type = pick_not_in_any_pick_at_all;
   display_pick();
}


// When doing a pick, this predicate says that any call that takes a
// mandatory subcall is simply rejected.
bool forbid_call_with_mandatory_subcall()
{
   return pick_type_table[current_pick_type].exhaustive_search;
}

// When we are doing the special scans in the resolver, we don't geneerate
// random subcalls -- we just leave the default call in place.
// Only when in the random search do we generate random subcalls.
bool allow_random_subcall_pick()
{
   return current_pick_type == pick_in_random_search;
}
