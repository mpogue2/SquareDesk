// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

#ifndef DATABASE_H
#define DATABASE_H

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2016  William B. Ackerman.
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

// These are written as the first two halfwords of the binary database file.
// The format version is not related to the version of the program or database.
// It is used only to make sure that the "mkcalls" program that compiled
// the database and the "sd" program that reads it are in sync with each
// other, as indicated by the version of this file.  Whenever we change
// anything in this file that could cause an incompatibility, we bump the
// database format version.

#define DATABASE_MAGIC_NUM 21316
#define DATABASE_FORMAT_VERSION 331


// We used to do some stuff to cater to compiler vendors (e.g. Sun
// Microsystems) that couldn't be bothered to do the "const" attribute
// correctly.
//
// So we used to have a line that said "#define Const const".  But not
// any longer.  If your compiler doesn't handle "const" correctly (or
// any other aspect of ANSI C++, for that matter), that's too bad.
//
// We no longer take pity on broken compilers.


// We would like "veryshort" to be a signed char, but not all
// compilers are fully ANSI compliant.  The IBM AIX compiler, for
// example, considers char to be unsigned.  The switch
// "NO_SIGNED_CHAR" alerts us to that fact.  The configure script has
// checked this for us.
//
// Baloney!  We no longer use a configure script.  We are aware of the
// fact that there is a whole "industry" (config/autoconf/etc.)
// dedicated to the job of figuring out what hideous brokenness any
// given Unix implementation is inflicting on us today, but we have no
// patience for that kind of garbage, and we are not going to support
// that "industry".  If your compiler or OS can't handle this, tough.
// (But we still leave the ifdef in place; it does no harm.)
//
// We no longer take pity on broken compilers or operating systems.

#ifdef NO_SIGNED_CHAR
#error "We need to have the char datatype be signed"
#endif

// We would like to think that we will always be able to count on compilers to do the
// right thing with "int" and "long int" and so on.  What we would really like is
// for compilers to be counted on to make "int" at least 32 bits, because we need
// 32 bits in many places.  However, some compilers don't, so we have to use
// "long int" or "unsigned long int".  We think that all compilers we deal with
// will do the right thing with that, but, just in case, we use a typedef.
//
// The type "uint32" must be an unsigned integer of at least 32 bits.
// The type "uint16" must be an unsigned integer of at least 16 bits.
//
// Note also:  There are many places in the program (not just in database.h and sd.h)
// where the suffix "UL" is put on constants that are intended to be of type "uint32".
// If "uint32" is changed to anything other than "unsigned long int", it may be
// necessary to change all of those.   Done.
//
// The above dates from a time when the major challenge was dealing with compilers that
// considered "int" to mean 16 bits.  Those days are long gone.  (Or, to be precise,
// I have no interest in supporting such compilers.)  Contemporary usage (that is, 32 bit
// and 64 bit gcc) seems to be that "int" means 32 bits and "long int" means the longest
// native type.  In principle, the code should be impervious to using "long int" on a
// 64 bit sytem (the comment does say "at least 32 bits", doesn't it?), but in practice
// there have been problems.

// Update, 2013:  We now just use the standard C99 things.

// Further update, 2013:  It seems that not all versions of stdint.h are correct.  The version
// that we use for production builds of Sd and Sdtty works correctly.  It is MinGW version 5.14,
// using gcc 3.4.5, and the stdint.h file that comes with it has
//    "ISO C9x  7.18  Integer types <stdint.h>"
//    "Based on ISO/IEC SC22/WG14 9899 Committee draft (SC22 N2794)"
// and shows a date of 2000-12-02.
//
//
// But it seems that this is not really the official standard that all modern compilers
// abide by.  Compliant compilers are *not required* to have the INT16_C and UINT16_C macros
// produce a result that is of a 16 bit type.  But some other compilers demand a 16 bit
// type in struct initializers of 16 bit type.
//
// But since the usual behavior of the INT16_C and UINT16_C macros is so close to what we want
// that we have decided, rather than using other names (which would just make the code more
// confusing) to tweak the definitions of those macros.  That makes it possible for someone
// reading the code not to have to worry about these subtle problems.
//
// There is a common way of telling the stdint.h include file not to define those macros:
// they are only defined if __STDC_CONSTANT_MACROS is set.  But, once again, compliant compilers
// are *not required* to obey this, and some include files in fact do not.
//
// Therefore, we forcibly undefine these macros after including stdint.h, and give our own
// definitions.

// For those systems that don't have a stdint.h file at all (Visual C++ version 6), we
// recognize the symbol USE_OWN_INT_MACROS.  When defined, we substitute our own definitions
// of everything, hoping that our use of "short", "long", and the constant suffix "L" will do
// the right thing.  For Visual C++ version 6, it does.

// Modern versions of the Micro$oft compiler probably provide stdint.h, but we're too cheap
// to buy modern versions, especially since both VC++ version 5 and version 6 generate
// incorrect code, in a phase-of-the-moon-dependent way, for "snag bits and pieces",
// and there is no evidence that Micro$oft ever fixed this.  So we stick with version 6
// and use it only for its nice debugger.  Production code is compiled with MinGW/gcc.

#if defined(_MSC_VER)
#define USE_OWN_INT_MACROS
#endif

// Put other definitions based on known properties of compilers here ....


#if defined(USE_OWN_INT_MACROS)
typedef int int32_t;
typedef unsigned int uint32_t;
typedef short int int16_t;
typedef unsigned short int uint16_t;
typedef char int8_t;
typedef unsigned char uint8_t;
#else
#include <stdint.h>
#endif

// We may or may not have definitions of the macros here.
// Remove them, and use our own definitions.
#undef INT8_C
#undef UINT8_C
#undef INT16_C
#undef UINT16_C
#undef INT32_C
#undef UINT32_C
#define INT8_C(val) ((int8_t) + (val))
#define UINT8_C(val) ((uint8_t) + (val##U))
#define INT16_C(val) ((int16_t) + (val))
#define UINT16_C(val) ((uint16_t) + (val##U))
#define INT32_C(val) ((uint32_t) + (val##L))
#define UINT32_C(val) ((uint32_t) + (val##UL))



// The goal is to get rid of the following typedefs, and use the standard
// type symbols everywhere. That is, things like "uint32" are "legacy code".
// They will be replaced in due time.

typedef int32_t int32;
typedef uint32_t uint32;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int8_t veryshort;
typedef int8_t int8;
typedef uint8_t uint8;
// Except this one.
typedef const char *Cstring;


// BEWARE!!  These must track the items in "tagtabinit" in mkcalls.cpp .
enum base_call_index {
   base_call_unused,
   base_call_null,
   base_call_null_second,
   base_call_basetag0,
   base_call_basetag0_noflip,
   base_call_flip,
   base_call_armturn_n4,
   base_call_ends_shadow,
   base_call_chreact_1,
   base_call_makepass_1,
   base_call_nuclear_1,
   base_call_scootback,
   base_call_scoottowave,
   base_call_backemup,
   base_call_circulate,
   base_call_trade,
   base_call_any_hand_remake,
   base_call_passthru,
   base_call_check_cross_counter,
   base_call_lockit,
   base_call_disband1,
   base_call_slither,
   base_call_maybegrandslither,
   base_call_dixiehalftag,
   base_call_plan_ctrtoend,
   base_base_prepare_to_drop,
   base_base_hinge,
   base_base_hinge_for_nicely,
   base_base_hinge_with_warn,
   base_base_hinge_for_breaker,
   base_base_hinge_and_then_trade,
   base_base_hinge_and_then_trade_for_breaker,
   base_call_two_o_circs,
   base_call_cloverleaf,
   base_call_clover,
   // The next "NUM_TAGGER_CLASSES" (that is, 4) must be a consecutive group.
   base_call_tagger0,
   base_call_tagger1_noref,
   base_call_tagger2_noref,
   base_call_tagger3_noref,
   base_call_circcer,
   base_call_turnstar_n,
   base_call_revert_if_needed,
   base_call_extend_n,
   num_base_call_indices    // Not an actual enumeration item.
};



// BEWARE!!  This list must track the tables "flagtabh", "defmodtabh",
// "forcetabh", and "altdeftabh" in mkcalls.cpp .  The "K" items also track
// the tables "yoyotabforce", "mxntabforce", "nxntabforce", "yoyotabplain", "mxntabplain", "nxntabplain",
// "reverttabplain", and "reverttabforce" in mkcalls.cpp .
//
// These are the infamous "heritable flags".  They are used in generally
// corresponding ways in the "callflagsh" word of a top level callspec_block,
// the "modifiersh" word of a "by_def_item", and the "modifier_seth" word of a
// "calldef_block", and the "cmd_final_flags.herit" of a setup with its command block.

enum heritflags {
   INHERITFLAG_DIAMOND    = 0x00000001U,
   INHERITFLAG_REVERSE    = 0x00000002U,
   INHERITFLAG_LEFT       = 0x00000004U,
   INHERITFLAG_FUNNY      = 0x00000008U,
   INHERITFLAG_INTLK      = 0x00000010U,
   INHERITFLAG_MAGIC      = 0x00000020U,
   INHERITFLAG_GRAND      = 0x00000040U,
   INHERITFLAG_12_MATRIX  = 0x00000080U,
   INHERITFLAG_16_MATRIX  = 0x00000100U,
   INHERITFLAG_CROSS      = 0x00000200U,
   INHERITFLAG_SINGLE     = 0x00000400U,
   INHERITFLAG_SINGLEFILE = 0x00000800U,
   INHERITFLAG_HALF       = 0x00001000U,
   INHERITFLAG_REWIND     = 0x00002000U,
   INHERITFLAG_STRAIGHT   = 0x00004000U,
   INHERITFLAG_TWISTED    = 0x00008000U,
   INHERITFLAG_LASTHALF   = 0x00010000U,
   INHERITFLAG_FRACTAL    = 0x00020000U,
   INHERITFLAG_FAST       = 0x00040000U,

   // This is a 2 bit field.  These track "yoyotabforce" and "yoyotabplain" in mkcalls.cpp.
   INHERITFLAG_YOYOETCMASK      = 0x00180000U,
   // This is its low bit.
   INHERITFLAG_YOYOETCBIT       = 0x00080000U,
   // These 3 things are the choices available inside.
   // Warning!  We sort of cheat on inheritance.  The low bit ("yoyo") means inherit yoyo,
   // and the high bit ("generous") means inherit both generous and stingy.
   // The local function "fix_gensting_weirdness", in sdmoves.cpp, deals with this.
   INHERITFLAG_YOYOETCK_YOYO    = 0x00080000U,
   INHERITFLAG_YOYOETCK_GENEROUS= 0x00100000U,
   INHERITFLAG_YOYOETCK_STINGY  = 0x00180000U,

   // This is a 4 bit field.  These track "mxntabforce" and "mxntabplain" in mkcalls.cpp.
   INHERITFLAG_MXNMASK    = 0x01E00000U,
   // This is its low bit.
   INHERITFLAG_MXNBIT     = 0x00200000U,
   // These 10 things are some of the 15 choices available inside.
   INHERITFLAGMXNK_1X2    = 0x00200000U,
   INHERITFLAGMXNK_2X1    = 0x00400000U,
   INHERITFLAGMXNK_1X3    = 0x00600000U,
   INHERITFLAGMXNK_3X1    = 0x00800000U,
   INHERITFLAGMXNK_0X3    = 0x00A00000U,
   INHERITFLAGMXNK_3X0    = 0x00C00000U,
   INHERITFLAGMXNK_0X4    = 0x00E00000U,
   INHERITFLAGMXNK_4X0    = 0x01000000U,
   INHERITFLAGMXNK_6X2    = 0x01200000U,   // For 6x2 acey deucey.
   INHERITFLAGMXNK_3X2    = 0x01400000U,   // For 3x2 acey deucey.

   // This is a 3 bit field.  These track "nxntabforce" and  "nxntabplain" in mkcalls.cpp.
   INHERITFLAG_NXNMASK    = 0x0E000000U,
   // This is its low bit.
   INHERITFLAG_NXNBIT     = 0x02000000U,
   // These 7 things are the choices available inside.
   INHERITFLAGNXNK_2X2    = 0x02000000U,
   INHERITFLAGNXNK_3X3    = 0x04000000U,
   INHERITFLAGNXNK_4X4    = 0x06000000U,
   INHERITFLAGNXNK_5X5    = 0x08000000U,
   INHERITFLAGNXNK_6X6    = 0x0A000000U,
   INHERITFLAGNXNK_7X7    = 0x0C000000U,
   INHERITFLAGNXNK_8X8    = 0x0E000000U,

   // This is a 3 bit field.  These track "reverttabforce" and "reverttabplain" in mkcalls.cpp .
   INHERITFLAG_REVERTMASK = 0x70000000U,
   // This is its low bit.
   INHERITFLAG_REVERTBIT  = 0x10000000U,
   // These 7 things are the choices available inside.
   INHERITFLAGRVRTK_REVERT= 0x10000000U,
   INHERITFLAGRVRTK_REFLECT=0x20000000U,
   INHERITFLAGRVRTK_RVF   = 0x30000000U,
   INHERITFLAGRVRTK_RFV   = 0x40000000U,
   INHERITFLAGRVRTK_RVFV  = 0x50000000U,
   INHERITFLAGRVRTK_RFVF  = 0x60000000U,
   INHERITFLAGRVRTK_RFF   = 0x70000000U

   // one bit remains.
};


// CFLAG1_FUDGE_TO_Q_TAG means three things, the main one being that the
// setup is to fudged from a suitably populated 3x4 into a 1/4 tag.
// The intention is that one can do "plenty" after doing a "1/2 press ahead"
// (as opposed to the more natural "extend") from waves.  It also means
// that we can fudge the other way if the schema is schema_in_out_triple
// or schema_in_out_triple_squash.  See the call "quick step part 2".
// It also means that a short6 is to be fudged to a 2x3.  See the call
// "quick step part 1".
//
// CFLAG1_NUMBER_MASK and CFLAG1_NUMBER_BIT, when nonzero, say that the call
// takes that number of numeric arguments.  So it must have that number of "@9"
// (or equivalent) things in its name.  The number can go up to 4.  But there
// is a special case for an encoding of 7.  That says that the call is declared
// "optional_special_number".  If an otherwise intractable fraction comes in
// (e.g. 3/4 swing the fractions from a left wave), a subcall that would otherwise
// not take a numeric argument, but has indicated, through this mechanism, that
// it is willing to accept an optional argument, will be given an argument.

// BEWARE!!  This list must track the table "flagtab1" in mkcalls.cpp .
// These flags go into the "callflags1" word of a callspec_block,
// and the "topcallflags1" word of the parse_state.

enum {
   CFLAG1_VISIBLE_FRACTION_MASK     = 0x00000007U, // 3 bit field
   CFLAG1_VISIBLE_FRACTION_BIT      = 0x00000001U, // its low bit
   CFLAG1_12_16_MATRIX_MEANS_SPLIT  = 0x00000008U,
   CFLAG1_PRESERVE_Z_STUFF          = 0x00000010U,
   CFLAG1_SPLIT_LIKE_DIXIE_STYLE    = 0x00000020U,
   CFLAG1_PARALLEL_CONC_END         = 0x00000040U,
   CFLAG1_TAKE_RIGHT_HANDS          = 0x00000080U,
   CFLAG1_TAKE_RIGHT_HANDS_AS_COUPLES= 0x00000100U,
   CFLAG1_YOYO_FRACTAL_NUM          = 0x00000200U,
   CFLAG1_FUDGE_TO_Q_TAG            = 0x00000400U,
   CFLAG1_STEP_REAR_MASK            = 0x00003800U, // 3 bit field
   CFLAG1_STEP_TO_WAVE              = 0x00000800U, // the encodings inside
   CFLAG1_REAR_BACK_FROM_R_WAVE     = 0x00001000U,
   CFLAG1_STEP_TO_NONPHAN_BOX       = 0x00001800U,
   CFLAG1_REAR_BACK_FROM_QTAG       = 0x00002000U,
   CFLAG1_REAR_BACK_FROM_EITHER     = 0x00002800U,
   CFLAG1_STEP_TO_QTAG              = 0x00003000U,
   CFLAG1_DISTRIBUTE_REPETITIONS    = 0x00004000U,
   CFLAG1_NUMBER_MASK               = 0x00038000U, // 3 bit field
   CFLAG1_NUMBER_BIT                = 0x00008000U, // its low bit
   CFLAG1_LEFT_MEANS_TOUCH_OR_CHECK = 0x00040000U,
   CFLAG1_SEQUENCE_STARTER          = 0x00080000U,
   CFLAG1_SEQUENCE_STARTER_PROM     = 0x00100000U,
   CFLAG1_DONT_USE_IN_RESOLVE       = 0x00200000U,
   CFLAG1_DONT_USE_IN_NICE_RESOLVE  = 0x00400000U,
   CFLAG1_SPLIT_LARGE_SETUPS        = 0x00800000U,
   CFLAG1_SPLIT_IF_Z                = 0x01000000U,
   CFLAG1_BASE_TAG_CALL_MASK        = 0x0E000000U, // 3 bit field
   CFLAG1_BASE_TAG_CALL_BIT         = 0x02000000U, // its low bit
   CFLAG1_BASE_CIRC_CALL            = 0x10000000U,
   CFLAG1_ENDS_TAKE_RIGHT_HANDS     = 0x20000000U,
   CFLAG1_FUNNY_MEANS_THOSE_FACING  = 0x40000000U,
   CFLAG1_SPLIT_LIKE_SQUARE_THRU    = 0x80000000U
};

// These are the logical continuation of the "CFLAG1" bits, that have to overflow
// into the "flagsf" word.  They must lie in the top 16 bits.
enum {
   CFLAG2_NO_SEQ_IF_NO_FRAC         = 0x00010000U,
   CFLAG2_CAN_BE_ONE_SIDE_LATERAL   = 0x00020000U,
   CFLAG2_NO_ELONGATION_ALLOWED     = 0x00040000U,
   CFLAG2_IMPRECISE_ROTATION        = 0x00080000U,
   CFLAG2_CAN_BE_FAN                = 0x00100000U,
   CFLAG2_EQUALIZE                  = 0x00200000U,
   CFLAG2_ONE_PERSON_CALL           = 0x00400000U,
   CFLAG2_YIELD_IF_AMBIGUOUS        = 0x00800000U,
   CFLAG2_DO_EXCHANGE_COMPRESS      = 0x01000000U,
   CFLAG2_IF_MOVE_CANT_ROLL         = 0x02000000U,
   CFLAG2_FRACTIONAL_NUMBERS        = 0x04000000U,
   CFLAG2_NO_RAISE_OVERCAST         = 0x08000000U,
   CFLAG2_OVERCAST_TRANSPARENT      = 0x10000000U,
   CFLAG2_IS_STAR_CALL              = 0x20000000U,
   CFLAG2_ACCEPT_IN_ALL_MENUS       = 0x40000000U
   // 1 spare.
};

// Beware!!  This list must track the table "matrixcallflagtab" in mkcalls.cpp .
enum {
   MTX_USE_SELECTOR           = 0x01,
   MTX_STOP_AND_WARN_ON_TBONE = 0x02,
   MTX_TBONE_IS_OK            = 0x04,
   MTX_IGNORE_NONSELECTEES    = 0x08,
   MTX_MUST_FACE_SAME_WAY     = 0x10,
   MTX_FIND_JAYWALKERS        = 0x20,
   MTX_BOTH_SELECTED_OK       = 0x40,
   MTX_FIND_SQUEEZERS         = 0x80,
   MTX_FIND_SPREADERS         = 0x100,
   MTX_USE_VEER_DATA          = 0x200,
   MTX_USE_NUMBER             = 0x400,
   MTX_MIRROR_IF_RIGHT_OF_CTR = 0x800
};


// These definitions help to encode things in the "qualifierstuff" field of a call
// definition.  That field is an unsigned 32 bit integer, of which we may only use 17
// bits (it is tightly packed in the compiled database file.)  The low 8 bits are a cast
// of the qualifier kind itself (hence we allow 255 nontrivial qualifiers).  The high 9
// bits are as follows.

enum {
   // The next two must be consecutive for encoding in the "assump_both" field.
   QUALBIT__LEFT           = 0x00010000,
   QUALBIT__RIGHT          = 0x00008000,
   QUALBIT__LIVE           = 0x00004000,
   QUALBIT__TBONE          = 0x00002000,  // TBONE and NTBONE together mean "explicit assumption"
   QUALBIT__NTBONE         = 0x00001000,
   // A 4 bit field.  If nonzero, there is a number requirement,
   // and the field is that number plus 1.
   QUALBIT__NUM_MASK       = 0x00000F00,
   QUALBIT__NUM_BIT        = 0x00000100,
   QUALBIT__QUAL_CODE      = 0x000000FF
};


// BEWARE!!  This list must track the table "leveltab" in mkcalls.cpp .
// BEWARE!!  This list must track the table "getout_strings" in sdtables.cpp .
// BEWARE!!  This list must track the table "old_filename_strings" in sdtables.cpp .
// BEWARE!!  This list must track the table "filename_strings" in sdtables.cpp .
// BEWARE!!  This list must track the table "level_threshholds_for_pick" in sdtop.cpp .

enum dance_level {
   l_mainstream,
   l_plus,
   l_a1,
   l_a2,
   l_c1,
   l_c2,
   l_c3a,
   l_c3,
   l_c3x,
   l_c4a,
   l_c4,
   l_c4x,
   l_dontshow,
   l_nonexistent_concept,   // We can't have more than 16 levels.

   // Tags for some of the above.

   dixie_grand_level = l_plus,
   extend_34_level = l_plus,
   zig_zag_level = l_a2,
   face_the_music_level = l_c3a,
   beau_belle_level = l_a2,
   cross_by_level = l_c1,
   intlk_triangle_level = l_c1,
   magic_triangle_level = l_c2,
   triangle_in_box_level = l_c2,
   stagger_not_in_2x4_level = l_c3a,
   general_magic_level = l_c4a,
   phantom_tandem_level = l_c4a,
   quadruple_CLW_level = l_c4a,
   Z_CLW_level = l_c4a
};

/* These are the states that people can be in, and the "ending setups" that can appear
   in the call data base. */
/* BEWARE!!  This list must track the array "estab" in mkcalls.cpp . */
/* BEWARE!!  This list must track the array "setup_attrs" in sdtables.cpp . */
/* BEWARE!!  The procedure "merge_setups" canonicalizes pairs of setups by their
   order in this list, and will break if it is re-ordered randomly.  See the comments
   there before changing the order of existing setups. In general, keep small setups
   first, particularly 4-person setups before 8-person setups. */

enum setup_kind {
   nothing,
   s1x1,
   s1x2,
   s1x3,
   s2x2,
   s1x4,
   sdmd,
   s_star,
   s_trngl,
   s_trngl4,
   sbeehive,
   svee,
   s_bone6,
   s_short6,
   s1x5,
   s1x6,
   s2x3,
   s_1x2dmd,
   s_2x1dmd,
   s_wingedstar6,
   s1x3p1dmd,
   s3p1x1dmd,
   s_qtag,
   s_bone,
   s1x8,
   slittlestars,
   s_2stars,
   s1x3dmd,
   s3x1dmd,
   s_spindle,
   s_hrglass,
   s_dhrglass,
   s_hyperglass,
   s_crosswave,
   s2x4,
   s2x5,
   sd2x5,
   s_ntrgl6cw,
   s_ntrgl6ccw,
   s_nftrgl6cw,
   s_nftrgl6ccw,
   s_ntrglcw,
   s_ntrglccw,
   s_nptrglcw,
   s_nptrglccw,
   s_nxtrglcw,
   s_nxtrglccw,
   spgdmdcw,
   spgdmdccw,
   s1x4dmd,
   swqtag,
   sdeep2x1dmd,
   swhrglass,
   s_rigger,
   s3x3,
   s3x4,
   s2x6,
   s2x7,
   sd2x7,
   s2x9,
   sd3x4,
   sd4x5,
   s_spindle12,
   s1p5x8,   // internal use only
   s1p5x4,
   sfudgy2x6l,
   sfudgy2x6r,
   sfudgy2x3l,
   sfudgy2x3r,
   s2x8,
   s4x4,
   s1x10,
   s1x12,
   s1x14,
   s1x16,
   s_c1phan,
   s_bigblob,
   s_ptpd,
   s3dmd,
   s4dmd,
   s3ptpd,
   s4ptpd,
   s_trngl8,
   s1x4p2dmd,
   s4p2x1dmd,
   splinepdmd,
   splinedmd,
   slinepdmd,
   slinedmd,
   slinebox,
   sline2box,
   sline6box,
   sdbltrngl4,
   sboxdmd,
   sboxpdmd,
   sdmdpdmd,
   s_hsqtag,
   s_dmdlndmd,
   s_hqtag,
   s_wingedstar,
   s_wingedstar12,
   s_wingedstar16,
   s_barredstar,
   s_barredstar12,
   s_barredstar16,
   s_galaxy,
   sbigh,
   sbigx,
   s3x6,
   s3x8,
   s4x5,
   s4x6,
   s2x10,
   s2x12,
   sdeepqtg,
   sdeepbigqtg,
   swiderigger,
   srigger12,
   sdeepxwv,
   s3oqtg,
   s_thar,
   s_alamo,
   sx4dmd,    // These are too big to actually represent --
   sx4dmdbone,// we don't let them out of their cage.
   s_hyperbone, // Ditto.
   s_hyper3x4,  // Ditto.
   s_tinyhyperbone, // Ditto.
   s8x8,      // Ditto.
   sxequlize, // Ditto.
   sx1x6,     // Ditto.
   sx1x8,     // This one is now real!
   sx1x16,    // Ditto.
   shypergal, // Ditto.
   shyper4x8a,// Ditto.
   shyper4x8b,// Ditto.
   shyper3x8, // Ditto.
   shyper2x16,// Ditto.
   sfat2x8,   // Ditto.  These are big setups that are the size of 4x8's,
   swide4x4,  // but only have 16 people.  The reason is to prevent loss of phantoms.
   s_323,
   s_343,
   s_3223,
   s_525,
   s_545,
   sh545,
   s_23232,
   s_3mdmd,
   s_3mptpd,
   s_4mdmd,
   s_4mptpd,
   sbigbigh,
   sbigbigx,
   sbigrig,
   sbighrgl,
   sbigdhrgl,
   sbigbone,
   sdblbone6,
   sbigdmd,
   sbigptpd,
   s5x1dmd,
   s1x5dmd,
   sbig3dmd,
   sbig4dmd,
   sdblxwave,
   sdblspindle,
   sdblbone,
   sdblrig,
   s_dead_concentric,
   s_normal_concentric,
   NUM_SETUP_KINDS   // End mark; not really in the enumeration.
};

// These are the "beginning setups" that can appear in the call data base.
// BEWARE!!  This list must track the array "sstab" in mkcalls.cpp.
// BEWARE!!  This list must track the array "begin_sizes" in common.cpp.

enum begin_kind {
   b_nothing,
   b_1x1,
   b_1x2,
   b_2x1,
   b_1x3,
   b_3x1,
   b_2x2,
   b_dmd,
   b_pmd,
   b_star,
   b_trngl,
   b_ptrngl,
   b_trngl4,
   b_ptrngl4,
   b_beehive,
   b_pbeehive,
   b_vee,
   b_pvee,
   b_trngl8,
   b_ptrngl8,
   b_bone6,
   b_pbone6,
   b_short6,
   b_pshort6,
   b_1x2dmd,
   b_p1x2dmd,
   b_2x1dmd,
   b_p2x1dmd,
   b_wingedstar6,
   b_pwingedstar6,
   b_qtag,
   b_pqtag,
   b_bone,
   b_pbone,
   b_rigger,
   b_prigger,
   b_3x3,
   b_2stars,
   b_p2stars,
   b_spindle,
   b_pspindle,
   b_hrglass,
   b_phrglass,
   b_dhrglass,
   b_pdhrglass,
   b_crosswave,
   b_pcrosswave,
   b_1x4,
   b_4x1,
   b_1x8,
   b_8x1,
   b_2x4,
   b_4x2,
   b_2x3,
   b_3x2,
   b_2x5,
   b_5x2,
   b_d2x5,
   b_d5x2,
   b_wqtag,
   b_pwqtag,
   b_deep2x1dmd,
   b_pdeep2x1dmd,
   b_whrglass,
   b_pwhrglass,
   b_1x5,
   b_5x1,
   b_1x6,
   b_6x1,
   b_3x4,
   b_4x3,
   b_2x6,
   b_6x2,
   b_2x7,
   b_7x2,
   b_d2x7,
   b_d7x2,
   b_2x9,
   b_9x2,
   b_d3x4,
   b_d4x3,
   b_d4x5,
   b_d5x4,
   b_spindle12,
   b_pspindle12,
   b_2x8,
   b_8x2,
   b_4x4,
   b_1x10,
   b_10x1,
   b_1x12,
   b_12x1,
   b_1x14,
   b_14x1,
   b_1x16,
   b_16x1,
   b_c1phan,
   b_galaxy,
   b_3x6,
   b_6x3,
   b_3x8,
   b_8x3,
   b_4x5,
   b_5x4,
   b_4x6,
   b_6x4,
   b_2x10,
   b_10x2,
   b_2x12,
   b_12x2,
   b_deepqtg,
   b_pdeepqtg,
   b_deepbigqtg,
   b_pdeepbigqtg,
   b_widerigger,
   b_pwiderigger,
   b_rigger12,
   b_prigger12,
   b_deepxwv,
   b_pdeepxwv,
   b_3oqtg,
   b_p3oqtg,
   b_thar,
   b_alamo,
   b_ptpd,
   b_pptpd,
   b_1x3dmd,
   b_p1x3dmd,
   b_3x1dmd,
   b_p3x1dmd,
   b_3dmd,
   b_p3dmd,
   b_4dmd,
   b_p4dmd,
   b_3ptpd,
   b_p3ptpd,
   b_4ptpd,
   b_p4ptpd,
   b_hqtag,
   b_phqtag,
   b_hsqtag,
   b_phsqtag,
   b_wingedstar,
   b_pwingedstar,
   b_323,
   b_p323,
   b_343,
   b_p343,
   b_3223,
   b_p3223,
   b_525,
   b_p525,
   b_545,
   b_p545,
   bh545,
   bhp545,
   b_23232,
   b_p23232,
   b_3mdmd,
   b_p3mdmd,
   b_3mptpd,
   b_p3mptpd,
   b_4mdmd,
   b_p4mdmd,
   b_4mptpd,
   b_p4mptpd,
   b_1x4dmd,
   b_p1x4dmd,
   b_bigh,
   b_pbigh,
   b_bigx,
   b_pbigx,
   b_bigbigh,
   b_pbigbigh,
   b_bigbigx,
   b_pbigbigx,
   b_bigrig,
   b_pbigrig,
   b_bighrgl,
   b_pbighrgl,
   b_bigdhrgl,
   b_pbigdhrgl,
   b_bigbone,
   b_pbigbone,
   b_dblbone6,
   b_pdblbone6,
   b_bigdmd,
   b_pbigdmd,
   b_bigptpd,
   b_pbigptpd,
   b_5x1dmd,
   b_p5x1dmd,
   b_1x5dmd,
   b_p1x5dmd,
   b_big3dmd,
   b_pbig3dmd,
   b_big4dmd,
   b_pbig4dmd,
   b_dblxwave,
   b_pdblxwave,
   b_dblspindle,
   b_pdblspindle,
   b_dblbone,
   b_pdblbone,
   b_dblrig,
   b_pdblrig
};

// These bits are used in the "callarray_flags" field of a "callarray".
// There is room for 21 of them.

enum {
   // This one must be 1!!!!
   CAF__ROT                     = 0x1,
   CAF__FACING_FUNNY            = 0x2,
   // Next one says this is concentrically defined --- the "end_setup" slot
   // has the centers' end setup, and there is an extra slot with the ends' end setup.
   CAF__CONCEND                 = 0x4,
   // Next one meaningful only if previous one is set.
   CAF__ROT_OUT                 = 0x8,
   // This is a 3 bit field.
   CAF__RESTR_MASK             = 0x70,
   // These next 6 are the nonzero values it can have.
   CAF__RESTR_UNUSUAL          = 0x10,
   CAF__RESTR_FORBID           = 0x20,
   CAF__RESTR_RESOLVE_OK       = 0x30,
   CAF__RESTR_CONTROVERSIAL    = 0x40,
   CAF__RESTR_BOGUS            = 0x50,
   CAF__RESTR_ASSUME_DPT       = 0x60,
   CAF__RESTR_EACH_1X3         = 0x70,
   CAF__PREDS                  = 0x80,
   CAF__NO_CUTTING_THROUGH    = 0x100,
   CAF__NO_FACING_ENDS        = 0x200,
   CAF__LATERAL_TO_SELECTEES  = 0x400,
   CAF__OTHER_ELONGATE        = 0x800,
   CAF__SPLIT_TO_BOX         = 0x1000,
   CAF__REALLY_WANT_DIAMOND  = 0x2000,
   CAF__NO_COMPRESS          = 0x4000,
   CAF__PLUSEIGHTH_ROTATION  = 0x8000,
};

// BEWARE!!  This list must track the array "qualtab" in mkcalls.cpp
// There is room for 255 of them, because they have to fit into a 8 bit field
// in the "qualifierstuff" field of a callarray.  This is checked at startup.
enum call_restriction {
   cr_none,                // Qualifier only.
   cr_alwaysfail,          // Restriction only.
   cr_give_fudgy_warn,
   cr_wave_only,
   cr_wave_unless_say_2faced,
   cr_all_facing_same,
   cr_1fl_only,
   cr_2fl_only,
   cr_2fl_per_1x4,
   cr_ctr_2fl_only,
   cr_3x3_2fl_only,
   cr_4x4_2fl_only,
   cr_leads_only,          // Restriction only.
   cr_trailers_only,       // Restriction only.
   cr_couples_only,
   cr_3x3couples_only,
   cr_4x4couples_only,
   cr_ckpt_miniwaves,      // Restriction only.
   cr_ctr_miniwaves,       // Restriction only.
   cr_ctr_couples,         // Restriction only.
   cr_awkward_centers,     // Restriction only.
   cr_dmd_same_pt,         // Qualifier only.
   cr_dmd_facing,
   cr_diamond_like,
   cr_qtag_like,
   cr_qtag_like_anisotropic,
   cr_pu_qtag_like,
   cr_conc_iosame,
   cr_conc_iodiff,
   cr_reg_tbone,
   cr_gen_qbox,            // Qualifier only.
   cr_nice_diamonds,
   cr_nice_wv_triangles,
   cr_nice_tnd_triangles,
   cr_magic_only,
   cr_li_lo,               // Qualifier only.
   cr_ctrs_in_out,         // Qualifier only.
   cr_indep_in_out,        // Qualifier only.
   cr_miniwaves,
   cr_not_miniwaves,       // Qualifier only.
   cr_tgl_tandbase,        // Qualifier only.
   cr_true_Z_cw,           // Qualifier only.
   cr_true_Z_ccw,          // Qualifier only.
   cr_true_PG_cw,          // Qualifier only.
   cr_true_PG_ccw,         // Qualifier only.
   cr_lateral_cols_empty,  // Qualifier only.
   cr_ctrwv_end2fl,        // Qualifier only.
   cr_ctr2fl_endwv,        // Qualifier only.
   cr_split_dixie,         // Qualifier only.
   cr_not_split_dixie,     // Qualifier only.
   cr_dmd_ctrs_mwv,        // Qualifier only.
   cr_dmd_ctrs_mwv_no_mirror,  // Qualifier only.
   cr_dmd_ctrs_mwv_change_to_34_tag,// Qualifier only.
   cr_spd_base_mwv,        // Qualifier only.
   cr_qtag_mwv,            // Qualifier only.
   cr_qtag_mag_mwv,        // Qualifier only.
   cr_dmd_ctrs_1f,         // Qualifier only.
   cr_dmd_pts_mwv,         // Qualifier only.
   cr_dmd_pts_1f,          // Qualifier only.
   cr_dmd_intlk,
   cr_dmd_not_intlk,
   cr_tall6,               // Actually not checked as qualifier or restriction.
   cr_ctr_pts_rh,          // Qualifier only.
   cr_ctr_pts_lh,          // Qualifier only.
   cr_extend_inroutl,      // Qualifier only.
   cr_extend_inloutr,      // Qualifier only.
   cr_said_dmd,            // Qualifier only.
   cr_said_tgl,            // Qualifier only.
   cr_didnt_say_tgl,       // Qualifier only.
   cr_said_gal,            // Qualifier only.
   cr_occupied_as_stars,   // Qualifier only.
   cr_occupied_as_clumps,  // Qualifier only.
   cr_occupied_as_blocks,  // Qualifier only.
   cr_occupied_as_h,       // Qualifier only.
   cr_occupied_as_qtag,    // Qualifier only.
   cr_occupied_as_3x1tgl,  // Qualifier only.
   cr_line_ends_looking_out, // Qualifier only.
   cr_col_ends_lookin_in,  // Qualifier only.
   cr_ripple_one_end,      // Qualifier only.
   cr_ripple_both_ends,    // Qualifier only.
   cr_ripple_both_ends_1x4_only, // Qualifier only.
   cr_ripple_both_centers, // Qualifier only.
   cr_ripple_any_centers,  // Qualifier only.
   cr_people_1_and_5_real, // Qualifier only.
   cr_ctrs_sel,
   cr_ends_sel,
   cr_all_sel,
   cr_not_all_sel,
   cr_some_sel,
   cr_none_sel,
   cr_nor_unwrap_sel,
   cr_ptp_unwrap_sel,
   cr_explodable,          // Restriction only.
   cr_rev_explodable,      // Restriction only.
   cr_peelable_box,        // Restriction only.
   cr_ends_are_peelable,   // Restriction only.
   cr_siamese_in_quad,
   cr_not_tboned_in_quad,
   cr_inroller_is_cw,
   cr_inroller_is_ccw,
   cr_outroller_is_cw,
   cr_outroller_is_ccw,
   cr_judge_is_cw,
   cr_judge_is_ccw,
   cr_socker_is_cw,
   cr_socker_is_ccw,
   cr_levelplus,
   cr_levela1,
   cr_levela2,
   cr_levelc1,
   cr_levelc2,
   cr_levelc3,
   cr_levelc4,
   cr_not_tboned,          // Restriction only.
   cr_opposite_sex,
   cr_quarterbox_or_col,   // Restriction only.
   cr_quarterbox_or_magic_col, // Restriction only.
   cr_all_ns,              // Restriction only.
   cr_all_ew,              // Restriction only.
   cr_real_1_4_tag,        // Restriction only.
   cr_real_3_4_tag,        // Restriction only.
   cr_real_1_4_line,       // Restriction only.
   cr_real_3_4_line,       // Restriction only.
   cr_jleft,               // Restriction only.
   cr_jright,              // Restriction only.
   cr_ijleft,              // Restriction only.
   cr_ijright,             // Restriction only.
   NUM_QUALIFIERS          // Not really in the enumeration.
};

// Schemata with "maybe" in their names will be translated to other
// schemata based on the modifiers with which they are used.  This is
// done by "get_real_callspec_and_schema", which happens early in
// "move_with_real_call".  For example, the "single" modifier on a
// call will turn "schema_maybe_single_concentric" into
// "schema_single_concentric".

// After the above, schemata with "or" in their names will be
// translated to other schemata based on the setup.  For example,
// "schema_concentric_2_4_or_normal" will be turned into
// "schema_concentric_2_4" (2 centers and 4 outsides) if the setup has
// 6 spots, and into "schema_single_concentric" if the setup has 4
// spots.

// BEWARE!!  This list must track the array "schematab" in mkcalls.cpp .
// BEWARE!!  This list must track the array "schema_attrs" in sdtables.cpp .
// Also, "schema_sequential" must be the start of all the sequential ones.
enum calldef_schema {
   schema_concentric,
   schema_cross_concentric,
   schema_3x3k_concentric,
   schema_3x3k_cross_concentric,
   schema_4x4k_concentric,
   schema_4x4k_cross_concentric,
   schema_single_concentric,
   schema_single_cross_concentric,
   schema_grand_single_concentric,
   schema_grand_single_cross_concentric,
   schema_single_concentric_together,
   schema_single_cross_concentric_together,
   schema_maybe_matrix_single_concentric_together,
   schema_maybe_single_concentric,
   schema_maybe_single_cross_concentric,
   schema_maybe_grand_single_concentric,
   schema_maybe_grand_single_cross_concentric,
   schema_maybe_special_single_concentric,
   schema_maybe_special_trade_by,
   schema_special_trade_by,
   schema_grand_single_or_matrix_concentric,
   schema_3x3_concentric,
   schema_4x4_lines_concentric,
   schema_4x4_cols_concentric,
   schema_maybe_nxn_lines_concentric,
   schema_maybe_nxn_cols_concentric,
   schema_maybe_nxn_1331_lines_concentric,
   schema_maybe_nxn_1331_cols_concentric,
   schema_1331_concentric,
   schema_1313_concentric,       // Not for public use!
   schema_1221_concentric,
   schema_concentric_diamond_line,
   schema_concentric_lines_z,
   schema_concentric_diamonds,
   schema_cross_concentric_diamonds,
   schema_concentric_zs,
   schema_cross_concentric_zs,
   schema_concentric_or_diamond_line,
   schema_concentric_or_6_2_line,
   schema_concentric_6_2,
   schema_cross_concentric_6_2,
   schema_concentric_6_2_line,
   schema_concentric_2_6,
   schema_cross_concentric_2_6,
   schema_concentric_2_4,
   schema_cross_concentric_2_4,
   schema_concentric_2_4_or_normal,
   schema_concentric_4_2,
   schema_concentric_4_2_prefer_1x4,
   schema_cross_concentric_4_2,
   schema_concentric_4_2_or_normal,
   schema_concentric_or_2_6,
   schema_concentric_or_6_2,
   schema_concentric_8_4,        // Not for public use!
   schema_concentric_big2_6,     // Not for public use!
   schema_concentric_2_6_or_2_4,
   schema_cross_concentric_2_6_or_2_4,
   schema_concentric_innermost,
   schema_concentric_touch_by_1_of_3,
   schema_cross_concentric_innermost,
   schema_cross_concentric_touch_by_1_of_3,
   schema_concentric_touch_by_2_of_3,
   schema_single_concentric_together_if_odd,
   schema_single_cross_concentric_together_if_odd,
   schema_concentric_6p,
   schema_concentric_6p_or_normal,
   schema_concentric_6p_or_normal_or_2x6_2x3,
   schema_concentric_6p_or_sgltogether,
   schema_cross_concentric_6p_or_normal,
   schema_concentric_others,
   schema_concentric_6_2_tgl,
   schema_concentric_to_outer_diamond,
   schema_concentric_no31dwarn,
   schema_concentric_specialpromenade,
   schema_cross_concentric_specialpromenade,
   schema_conc_12,
   schema_conc_16,
   schema_conc_star,
   schema_conc_star12,
   schema_conc_star16,
   schema_conc_bar,
   schema_conc_bar12,
   schema_conc_bar16,
   schema_conc_o,
   schema_intermediate_diamond,
   schema_outside_diamond,
   schema_maybe_matrix_conc,
   schema_maybe_matrix_conc_star,
   schema_maybe_matrix_conc_bar,
   schema_checkpoint,
   schema_checkpoint_spots,
   schema_checkpoint_mystic_ok,
   schema_cross_checkpoint,
   schema_rev_checkpoint,
   schema_rev_checkpoint_concept,
   schema_ckpt_star,
   schema_maybe_in_out_triple_dyp_squash,
   schema_in_out_triple_dyp_squash,
   schema_maybe_in_out_triple_squash,
   schema_in_out_triple_squash,
   schema_sgl_in_out_triple_squash,
   schema_3x3_in_out_triple_squash,
   schema_4x4_in_out_triple_squash,
   schema_in_out_triple,
   schema_sgl_in_out_triple,
   schema_3x3_in_out_triple,
   schema_4x4_in_out_triple,
   schema_in_out_quad,
   schema_in_out_12mquad,
   schema_in_out_triple_zcom,
   schema_in_out_center_triple_z,
   schema_select_leads,
   schema_select_headliners,
   schema_select_sideliners,
   schema_select_original_rims,
   schema_select_original_hubs,
   schema_select_those_facing_both_live,
   schema_select_ctr2,
   schema_select_ctr4,
   schema_select_ctr6,
   schema_select_who_can,
   schema_select_who_did,
   schema_select_who_didnt,
   schema_select_who_did_and_didnt,
   schema_lateral_6,             // Not for public use!
   schema_vertical_6,            // Not for public use!
   schema_intlk_lateral_6,       // Not for public use!
   schema_intlk_vertical_6,      // Not for public use!
   schema_by_array,
   schema_nothing,
   schema_nothing_noroll,
   schema_nothing_other_elong,
   schema_matrix,
   schema_partner_matrix,
   schema_partner_partial_matrix,
   schema_roll,
   schema_recenter,
   schema_sequential,            // All after this point are sequential.
   schema_split_sequential,
   schema_sequential_with_fraction,
   schema_sequential_with_split_1x8_id,
   schema_sequential_alternate,
   schema_sequential_remainder,
   schema_alias                  // Not a schema once the program is running.
};


/* BEWARE!!  Some of these flags co-exist with other flags defined elsewhere.
   The early ones are "concentricity" flags.  They must co-exist with the
   setupflags defined in sd.h because they share the same word.  For that reason,
   the latter flags are defined at the high end of the word, and the concentricity
   flags shown here are at the low end.
   The last bunch of flags are pushed up against the high end of the word, so that
   they can exactly match some other flags.

   The constant HERITABLE_FLAG_MASK embraces them.     **** NOT SO!!!!!  NO SUCH THING!!!!

   The flags that must stay in step are in the "FINAL__XXX" group
   in sd.h, the "cflag__xxx" group in database.h, and the "dfm_xxx" group in
   database.h . There is compile-time code in sdinit.cpp to check that these
   constants are all in step.

   dfm_conc_demand_lines             --  concdefine outers: must be ends of lines at start
   dfm_conc_demand_columns           --  concdefine outers: must be ends of columns at start
   dfm_conc_force_lines              --  concdefine outers: force them to line spots when done
   dfm_conc_force_columns            --  concdefine outers: force them to column spots when done
   dfm_conc_force_otherway           --  concdefine outers: force them to other spots when done
   dfm_conc_force_spots              --  concdefine outers: force them to same spots when done
   dfm1_conc_concentric_rules        --  concdefine outers: apply actual concentric ("lines-to-lines/columns-to-columns") rule
   dfm1_suppress_elongation_warnings --  concdefine outers: suppress warn_lineconc_perp etc.
           NOTE: the above 8 flags are specified only in the second spec, even if the concept is
              cross-concentric, in which case the "demand" flags might be considered to belong
              with the first spec.
   DFM1_CALL_MOD_MASK                 --  concdefine/seqdefine: can substitute something under certain circumstances
                  specific encodings within this mask are:
                     1   "or_anycall"
                     2   "mandatory_anycall"
                     3   "allow_plain_mod"
                     4   "allow_forced_mod"
                     5   "or_secondary_call"
                     6   "mandatory_secondary_call"
   dfm1_repeat_n                      --  seqdefine: take a numeric argument and replicate this part N times
   dfm1_repeat_n_alternate            --  seqdefine: take a numeric argument and replicate this part and the next one N times alternately
   dfm1_endscando                     --  concdefine outers: can tell ends only to do this
   dfm1_repeat_nm1                    --  seqdefine: take a numeric argument and replicate this part N-1 times
   dfm1_roll_transparent              --  seqdefine: any person who is marked roll-neutral after this call has his previous roll status restored
   dfm1_cpls_unless_single            --  seqdefine: the do this part as couples, unless this call is being done "single"
                                                               and "single_is_inherited" was set

   DFM1_NUM_INSERT_MASK              --  if nonzero, shift that 3-bit number into the number fields
   DFM1_FRACTAL_INSERT               --  if on, any number insertion is to have 1<->3 switched.
   DFM1_NO_CHECK_MOD_LEVEL           --  don't check the level of a modifier like "interlocked" -- if the subcall
                                             says it is only legal at C3, accept it anyway.

   INHERITFLAG_DIAMOND               --  concdefine/seqdefine: if original call said "diamond" apply it to this part
   INHERITFLAG_LEFT                  --  concdefine/seqdefine: if original call said "left" apply it to this part
   INHERITFLAG_FUNNY                 --  concdefine/seqdefine: if original call said "funny" apply it to this part
   INHERITFLAG_INTLK                 --  concdefine/seqdefine: if original call said "interlocked" apply it to this part
   INHERITFLAG_MAGIC                 --  concdefine/seqdefine: if original call said "magic" apply it to this part
   INHERITFLAG_GRAND                 --  concdefine/seqdefine: if original call said "grand" apply it to this part
   INHERITFLAG_12_MATRIX             --  concdefine/seqdefine: if original call said "12matrix" apply it to this part
   INHERITFLAG_16_MATRIX             --  concdefine/seqdefine: if original call said "16matrix" apply it to this part
   INHERITFLAG_CROSS                 --  concdefine/seqdefine: if original call said "cross" apply it to this part
   INHERITFLAG_SINGLE                --  concdefine/seqdefine: if original call said "single" apply it to this part
*/


// These are the call modifiers bits.  They go in the "modifiers1" word of a by_def_item.
// BEWARE!!  The "CONC" stuff, and all the later stuff, must track the table "defmodtab1" in mkcalls.cpp .
// BEWARE!!  The "SEQ" stuff must track the table "seqmodtab1" in mkcalls.cpp .
// BEWARE!!  The union of all of these flags, which is encoded in DFM1_CONCENTRICITY_FLAG_MASK,
// must coexist with the CMD_MISC__ flags defined in sd.h .  Note that the bit definitions
// of those flags start where these end.  Keep it that way.  If any flags are added here,
// they must be taken away from the CMD_MISC__ flags.

enum mods1_word {
   // These are the "conc" flags.  They overlay the "seq" flags.

   DFM1_CONC_DEMAND_LINES            = 0x00000001,
   DFM1_CONC_DEMAND_COLUMNS          = 0x00000002,
   DFM1_CONC_FORCE_LINES             = 0x00000004,
   DFM1_CONC_FORCE_COLUMNS           = 0x00000008,
   DFM1_CONC_FORCE_OTHERWAY          = 0x00000010,
   DFM1_CONC_FORCE_SPOTS             = 0x00000020,
   DFM1_CONC_CONCENTRIC_RULES        = 0x00000040,
   DFM1_SUPPRESS_ELONGATION_WARNINGS = 0x00000080,
   // Beware!!  The above "conc" flags must all lie within DFM1_CONCENTRICITY_FLAG_MASK.

   // These are the "seq" flags.  They overlay the "conc" flags.

   // Under normal conditions, we do *not* re-evaluate between parts.  This
   // flag overrides that and makes us re-evaluate.
   DFM1_SEQ_RE_EVALUATE              = 0x00000001U,
   DFM1_SEQ_DO_HALF_MORE             = 0x00000002U,
   // But, if we break up a call with something like "random", the convention
   // is to re-evaluate at the break point.  This flag, used for calls like
   // "patch the <anyone>" or "rims trade back", says that we *never* re-evaluate,
   // even if the call is broken up.
   DFM1_SEQ_NO_RE_EVALUATE           = 0x00000004U,
   DFM1_SEQ_REENABLE_ELONG_CHK       = 0x00000008U,
   DFM1_SEQ_REPEAT_N                 = 0x00000010U,
   DFM1_SEQ_REPEAT_NM1               = 0x00000020U,
   DFM1_SEQ_NORMALIZE                = 0x00000040U,
   // Beware!!  The above "seq" flags must all lie within DFM1_CONCENTRICITY_FLAG_MASK.

   // End of the separate conc/seq flags.  This constant embraces them.
   // Beware!!  The above "conc" and "seq" flags must lie within this.
   // Beware also!!  The CMD_MISC__ flags must be disjoint from this.
   // If this mask is made bigger, be sure the CMD_MISC__ flags (in sd.h)
   // are moved out of the way.  If it is made smaller (to accommodate CMD_MISC__)
   // be sure the conc/seq flags stay inside it.
   DFM1_CONCENTRICITY_FLAG_MASK      = 0x000000FF,

   // BEWARE!!  The following ones must track the table "defmodtab1" in mkcalls.cpp
   // Start of miscellaneous flags.

   // This is a 3 bit field -- CALL_MOD_BIT tells where its low bit lies.
   DFM1_CALL_MOD_MASK                = 0x00000700U,
   DFM1_CALL_MOD_BIT                 = 0x00000100U,
   // Here are the codes that can be inside.
   DFM1_CALL_MOD_ANYCALL             = 0x00000100U,
   DFM1_CALL_MOD_MAND_ANYCALL        = 0x00000200U,
   DFM1_CALL_MOD_ALLOW_PLAIN_MOD     = 0x00000300U,
   DFM1_CALL_MOD_ALLOW_FORCED_MOD    = 0x00000400U,
   DFM1_CALL_MOD_OR_SECONDARY        = 0x00000500U,
   DFM1_CALL_MOD_MAND_SECONDARY      = 0x00000600U,

   DFM1_ONLY_FORCE_ELONG_IF_EMPTY    = 0x00000800U,
   DFM1_ROLL_TRANSPARENT_IF_Z        = 0x00001000U,
   DFM1_ENDSCANDO                    = 0x00002000U,
   DFM1_FINISH_THIS                  = 0x00004000U,
   DFM1_ROLL_TRANSPARENT             = 0x00008000U,
   DFM1_PERMIT_TOUCH_OR_REAR_BACK    = 0x00010000U,
   DFM1_CPLS_UNLESS_SINGLE           = 0x00020000U,
   // This is a 2 bit field -- NUM_SHIFT_BIT tells where its low bit lies.
   DFM1_NUM_SHIFT_MASK               = 0x000C0000U,
   DFM1_NUM_SHIFT_BIT                = 0x00040000U,
   // This is a 3 bit field -- NUM_INSERT_BIT tells where its low bit lies.
   DFM1_NUM_INSERT_MASK              = 0x00700000U,
   DFM1_NUM_INSERT_BIT               = 0x00100000U,
   DFM1_NO_CHECK_MOD_LEVEL           = 0x00800000U,
   DFM1_FRACTAL_INSERT               = 0x01000000U,
   DFM1_SUPPRESS_ROLL                = 0x02000000U
};

enum  {
   // These are the individual codes.  They must fit in 3 bits.
   STB_NONE,      // Unknown if REVERSE off, "Z" if REVERSE on.
   STB_A,         // "A" - person turns anticlockwise from 1 to 4 quadrants
   STB_AC,        // "AC" - person turns anticlockwise once,
                  //     then clockwise 1 to 4 quadrants
   STB_AAC,       // "AAC" - person turns anticlockwise twice,
                  //     then clockwise 1 to 4 quadrants
   STB_AAAC,      // "AAAC" - person turns anticlockwise 3 times,
                  //     then clockwise 1 to 4 quadrants
   STB_AAAAC,     // "AAAAC" - person turns anticlockwise 4 times,
                  //     then clockwise 1 to 4 quadrants
   STB_AA,        // "AA" - person turns anticlockwise from 5 to 8 quadrants

   // This is the entire field in which it fits.
   STB_MASK = 0xF,

   // This bit reverses everything (or changes "none" to "Z".)
   STB_REVERSE = 8
};

// These define the format of the short int (16 bits, presumably) items emitted
// for each person in a by-array call definition.  These will get read into the
// "arr" array of a predptr_pair or the "stuff.def" array of a callarray.
//
// The format of this item is:
//     stability info   slide and roll info         where to go     direction to face
//                   (2 for slide, 3 for roll)     (compressed!)
//         4 bits            5 bits                   5 bits            2 bits
//
// The compressed location is arranged so that it will never be zero.  A word of
// zero in the database has a different meaning -- this person has no legal action.

// These help with the identification of the above fields.
enum {
   DBSTAB_BIT      = 0x1000,
   DBSLIDE_BIT     = 0x0400,
   DBSLIDEROLL_BIT = 0x0080
};

// Some external stuff in common.cpp:

extern bool do_heritflag_merge(uint32 *dest, uint32 source);
extern int begin_sizes[];

#endif   /* DATABASE_H */
