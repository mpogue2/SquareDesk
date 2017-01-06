// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

#ifndef SORT_H
#define SORT_H

//    Copyright (C) 1993-2009  William B. Ackerman.

//    This is free software; you can redistribute it and/or modify it
//    under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This is distributed in the hope that it will be useful, but WITHOUT
//    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
//    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
//    License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this file; if not, write to the Free Software Foundation, Inc.,
//    59 Temple Place, Suite 330, Boston, MA 02111-1307 USA


// This is a template for a general sort procedure, using the
// "heapsort" algorithm.  It is typically used thusly:
//
//  Assume MYCLASS is the data type of the items in the array
//  to be sorted.  Make up names "MYCOMPARE" and "MYSORT".
//
//  After including this file, give the following two declarations:
//
//  ==========================================================
//  class MYCOMPARE {
//   public:
//     static bool inorder(MYCLASS a, MYCLASS b)
//        {
//           return  ... some boolean function of a and b ... ;
//        }
//  };
//
//  typedef SORT<MYCLASS, MYCOMPARE> MYSORT;
//  ==========================================================
//
//  Assignment statements to MYCLASS must be possible, since the
//  sort procedure will perform assignments as it moves the array
//  elements around.
//
//  The "inorder" function can take its arguments by reference
//  if you prefer:
//     static bool inorder(MYCLASS & a, MYCLASS & b)
//         ......
//
//
//  Now your code can create arrays of MYCLASS:
//
//     MYCLASS my_array[1000];
//
//  And sort them:
//
//     MYSORT::heapsort(my_array, 1000);
//
//   The array will be sorted such that if inorder(x, y) is true,
//   an item with value x will be earlier in the array than an item
//   with value y.
//
//   MYCLASS does not need to be an actual class.  To sort an array
//   of unsigned integers according to their low 16 bits, do this:
//
//  class RIGHTHALFCOMPARE {
//   public:
//     static bool inorder(unsigned int a, unsigned int b)
//        { return (a & 0xFFFF) < (b & 0xFFFF); }
//  };
//
//  typedef SORT<unsigned int, RIGHTHALFCOMPARE> RIGHTHALFSORT;
//
//  unsigned int my_array[1000];
//
//  RIGHTHALFSORT::heapsort(my_array, 1000);

// ********** End of client documentation. **********


// The notation and general algorithm is from "The Design and Analysis of
// Computer Algorithms", by Aho, Hopcroft and Ullman, Addison Wesley, 1974.


// Definition: a region of the array "a" is said to have the "heap" property
// (or "be a heap") if:
//    Whenever a[i] and a[2i+1] are both in the region, a[i] >= a[2i+1], and
//    Whenever a[i] and a[2i+2] are both in the region, a[i] >= a[2i+2].
//
// Two special properties are noteworthy:
//    1: If hi <= 2*lo, the region from lo to hi inclusive is automatically a heap,
//       since there are no such pairs lying in the region.
//    2: If the region that is a heap is [0..hi], a[0] is greater than or equal to
//       every other element of the region.  Why?  Because a[0] >= a[1..2],
//       and a[1] >= a[3..4] while a[2] >= a[5..6], so a[1] and a[2]
//       are collectively >= a[3..6], which are >= a[7..14], etc.
//
//       Heapify causes the region between lo-1 and hi-1, inclusive,
//       to be a heap, given that the region from lo to hi-1 was
//       already a heap.  It works as follows: when we declare "lo-1"
//       to be part of the region, the only way it can fail to be a
//       heap is if a[lo-1] is too small -- it might be less than
//       a[2*lo-1] or a[2*lo].  If this is the case, we swap it with
//       the larger of a[2*lo-1] and a[2*lo-1].  Now whatever we
//       swapped it with got smaller, so it might fail to meet the
//       heap property with respect to the elements farther down the
//       line, so we repeat the process until we are off the end of
//       the region.

template < typename C, typename CMP > class SORT {
 private:
   static void heapify(C *the_array, int lo, int hi)
      {
         int j = lo-1;

         for (;;) {
            int k = j*2+1;

            if (k+1 > hi) return;
            if (k+1 < hi) {
               if (CMP::inorder(the_array[k], the_array[k+1])) k++;
            }
            if (CMP::inorder(the_array[k], the_array[j])) return;
            C temp = the_array[j];
            the_array[j] = the_array[k];
            the_array[k] = temp;
            j = k;
         }
      }


 public:
   static void heapsort(C *the_array, int n)
      {
         int i;

         // First, turn the whole array into a heap, building downward from the top,
         // since adding one more item at the bottom is what heapify is good at.
         // We don't start calling heapify until the low limit is n/2, since heapify
         // wouldn't have anything to do until then.

         for (i=n/2; i>0; i--) heapify(the_array, i, n);

         // Now we use the property that a[0] has the largest element.
         // We pull that out and move it to the end.  We declare that item
         // to no longer be part of the region we are interested in.
         // Since we have changed a[0], we call heapify to repair the damage,
         // on the smaller region.  We repeat this, pulling out the largest
         // element of the remaining heap (which is always element 0), and
         // letting the heap shrink down to nothing.

         for (i=n; i>1; i--) {
            C temp = the_array[0];
            the_array[0] = the_array[i-1];
            the_array[i-1] = temp;
            heapify(the_array, 1, i-1);
         }
      }
};

#endif
