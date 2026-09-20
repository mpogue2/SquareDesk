// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

#ifndef SDCHARS_H
#define SDCHARS_H

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


// Codes for special accelerator keystrokes and such.

// Function keys can be plain, shifted, control, alt, or control-alt.

enum {
   FKEY = 128,
   SFKEY = 144,
   CFKEY = 160,
   AFKEY = 176,
   CAFKEY = 192,

   // "Enhanced" keys can be plain, shifted, control, alt, or control-alt.
   // e1 = page up
   // e2 = page down
   // e3 = end
   // e4 = home
   // e5 = left arrow
   // e6 = up arrow
   // e7 = right arrow
   // e8 = down arrow
   // e13 = insert
   // e14 = delete

   EKEY = 208,
   SEKEY = 224,
   CEKEY = 240,
   AEKEY = 256,
   CAEKEY = 272,

   // Digits can be control, alt, or control-alt.

   CTLDIG = 288,
   ALTDIG = 298,
   CTLALTDIG = 308,

   // Numeric keypad can be control, alt, or control-alt.

   CTLNKP = 318,
   ALTNKP = 328,
   CTLALTNKP = 338,

   // Letters can be control, alt, or control-alt.

   CTLLET = (348-'A'),
   ALTLET = (374-'A'),
   CTLALTLET = (400-'A'),

   CLOSEPROGRAMKEY = 426,

   FCN_KEY_TAB_LOW = (FKEY+1),
   FCN_KEY_TAB_LAST = CLOSEPROGRAMKEY
};


#endif   /* SDCHARS_H */
