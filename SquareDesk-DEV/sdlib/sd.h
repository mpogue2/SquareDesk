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

#include <stdio.h>
#include <string.h>

// ****** We would like to bring this in only in sd.h, not here.  But it contains more "boilerplate",
// specifically, the definitions of uint32 etc.  And mkcalls needs that.  So we should really have
// another file for that, and bring it, but not database, in here.
#include "database.h"
#include "sdbase.h"

class saved_error_info {

public:

   void collect(error_flag_type flag);      // in sdtop.cpp.
   void throw_saved_error() THROW_DECL;     // in sdtop.cpp.

private:

   error_flag_type save_error_flag;
   char save_error_message1[MAX_ERR_LENGTH];
   char save_error_message2[MAX_ERR_LENGTH];
   uint32 save_collision_person1;
   uint32 save_collision_person2;
};


enum { MAX_PEOPLE = 24 };


enum {
   MIMIC_SETUP_LINES   = 0x00000001U,
   MIMIC_SETUP_COLUMNS = 0x00000002U,
   MIMIC_SETUP_WAVES   = 0x00000004U,
   MIMIC_SETUP_BOXES   = 0x00000008U
};




// Some versions of gcc don't like the class "concept", so we change the spelling.
class SDLIB_API conzept {

   // We want the concept list, as used by the main program, to be
   // constant.  But we can't literally make it constant, because
   // we need to translate the names during initialization, writing
   // over the "menu_name" field.  So the actual table that has the
   // static initializer in sdctable will *not* be constant.  But it
   // will be private.

 private:

   static concept_descriptor unsealed_concept_descriptor_table[];

 public:

   // This does the translation, writes over the "menu_name" fields,
   // and places a const pointer to the thing in "concept_descriptor_table".
   // This is in sdinit.
   static void translate_concept_names();
};

// We need a forward reference; it's defined later in this file.
struct predptr_pair;

struct callarray {
   callarray *next;
   uint32 callarray_flags;
   uint32 qualifierstuff;   /* See QUALBIT__??? definitions in database.h */
   call_restriction restriction;

   uint8 start_setup;       /* Must cast to begin_kind! */
   uint8 end_setup;         /* Must cast to setup_kind! */
   uint8 end_setup_in;      /* Only if end_setup = concentric */  /* Must cast to setup_kind! */
   uint8 end_setup_out;     /* Only if end_setup = concentric */  /* Must cast to setup_kind! */
   begin_kind get_start_setup() { return (begin_kind) start_setup; }
   setup_kind get_end_setup() { return (setup_kind) end_setup; }
   setup_kind get_end_setup_in() { return (setup_kind) end_setup_in; }
   setup_kind get_end_setup_out() { return (setup_kind) end_setup_out; }

   union {
      // Dynamically allocated to whatever size is required.
      uint16 array_no_pred_def[4]; // Only if pred = false.
      struct {                     // Only if pred = true.
         predptr_pair *predlist;
         // Dynamically allocated to whatever size is required.
         char errmsg[4];
      } prd;
   } stuff;
};

struct calldef_block {
   /* This has individual keys for groups of heritable modifiers.  Hence one
      can't say anything like "alternate_definition [3x3 4x4]".  But of course
      one can mix things from different groups. */
   uint32 modifier_seth;
   callarray *callarray_list;
   calldef_block *next;
   dance_level modifier_level;
};

struct by_def_item {
   short call_id;
   mods1_word modifiers1;

   /* The "modifiersh" field of an invocation of a subcall tells what
      incoming modifiers will be passed to the subcall.  What the bits mean
      depends on whether the call's top-level "callflagsh" bit is on.

      If the bit is on in "callflagsh", the corresponding bit in "modifiersh"
      says to pass the modifier, if present, on to the subcall.  If this is
      a group (e.g. INHERITFLAG_MXNMASK), and it is on in "callflagsh", the
      entire field in "modifiersh" must be on or off.  It will be on if the
      subcall involcation is marked "inherit_mxn" or wahatever.

      If the bit is off in "callflagsh", the corresponding bit in "modifiersh"
      says to force the modifier.  In this case groups are treated as individual
      modifiers.  An individual switch like "force_3x3" causes that key to be
      placed in the field of "modifiersh". */

   uint32 modifiersh;
};

struct matrix_def_block {
   matrix_def_block *next;
   uint32 alternate_def_flags;
   dance_level modifier_level;
   uint32 matrix_def_items[2];     // Dynamically allocated to either 2 or 16.
};

struct calldefn {
   uint32 callflags1;    // The CFLAG1_??? flags.
   uint32 callflagsh;    // The mask for the heritable flags.
   // Within the "callflagsh" field, the various grouped fields
   // (e.g. INHERITFLAG_MXNMASK) are uniform -- either all the bits are
   // on or they are all off.  A call can only inherit the entire group,
   // by saying "mxn_is_inherited".
   uint32 callflagsf;    // The ESCAPE_WORD__???  and CFLAGH__??? flags.
   mutable short int frequency;  // For call-use statistics.  Logically ought to be at the top level, but we hide it here.
   short int level;
   calldef_schema schema;
   calldefn *compound_part;
   union {
      struct {
         calldef_block *def_list;
      } arr;            // if schema = schema_by_array
      struct {
         uint32 matrix_flags;
         matrix_def_block *matrix_def_list;
      } matrix;         // if schema = schema_matrix, schema_partner_matrix, or schema_partner_partial_matrix
      struct {
         int howmanyparts;
         by_def_item *defarray;  // Dynamically allocated, there are "howmanyparts" of them.
      } seq;                     // if schema = schema_sequential or whatever
      struct {
         by_def_item innerdef;
         by_def_item outerdef;
      } conc;           // if schema = any of the concentric ones.
   } stuff;
};

struct call_with_name {
   calldefn the_defn;
   /* This is the "pretty" name -- "@" escapes have been changed to things like "<N>".
      If there are no escapes, this just points to the stuff below.
      If escapes are present, it points to allocated storage elsewhere.
      Either way, it persists throughout the program. */
   Cstring menu_name;
   /* Dynamically allocated to whatever size is required, will have trailing null.
      This is the name as it appeared in the database, with "@" escapes. */
   char name[4];
};


// These bits are used to allocate flag bits
// that appear in the "callflagsf" word of a top level calldefn block
// and the "cmd_final_flags.final" of a setup with its command block.
// We need to leave the top 16 bits free in order to accomodate the "CFLAG2" bits.

enum {
   // A 3-bit field.
   CFLAGH__TAG_CALL_RQ_MASK        = 0x00000007U,
   CFLAGH__TAG_CALL_RQ_BIT         = 0x00000001U,
   CFLAGH__REQUIRES_SELECTOR       = 0x00000008U,
   CFLAGH__REQUIRES_DIRECTION      = 0x00000010U,
   CFLAGH__CIRC_CALL_RQ_BIT        = 0x00000020U,
   CFLAGH__ODD_NUMBER_ONLY         = 0x00000040U,
   CFLAGH__HAS_AT_ZERO             = 0x00000080U,
   CFLAGH__HAS_AT_M                = 0x00000100U,
   CFLAGHSPARE_1                   = 0x00000200U,
   CFLAGHSPARE_2                   = 0x00000400U,
   CFLAGHSPARE_3                   = 0x00000800U,
   CFLAGHSPARE_4                   = 0x00001000U,
   CFLAGHSPARE_5                   = 0x00002000U,
   CFLAGHSPARE_6                   = 0x00004000U,
   CFLAGHSPARE_7                   = 0x00008000U
};

/* These flags, and "CFLAGH__???" flags, go along for the ride, in the callflagsf
   word of a callspec.  We use symbols that have been graciously
   provided for us from database.h to tell us what bits may be safely used next
   to the heritable flags.  Note that these bits overlap the FINAL__?? bits above.
   These are used in the callflagsf word of a callspec.  The FINAL__?? bits are
   used elsewhere.  They don't mix. */

enum {
   ESCAPE_WORD__LEFT            = CFLAGHSPARE_1,
   ESCAPE_WORD__CROSS           = CFLAGHSPARE_2,
   ESCAPE_WORD__MAGIC           = CFLAGHSPARE_3,
   ESCAPE_WORD__INTLK           = CFLAGHSPARE_4,
   ESCAPE_WORD__GRAND           = CFLAGHSPARE_5
};


/* These flags go along for the ride, in some parts of the code (BUT NOT
   THE CALLFLAGSF WORD OF A CALLSPEC!), in the same word as the heritable flags,
   but are not part of the inheritance mechanism.  We use symbols that have been
   graciously provided for us from above to tell us what bits may be safely
   used next to the heritable flags. */

enum finalflags {
   FINAL__SPLIT                      = CFLAGHSPARE_1,
   FINAL__SPLIT_SQUARE_APPROVED      = CFLAGHSPARE_2,
   FINAL__SPLIT_DIXIE_APPROVED       = CFLAGHSPARE_3,
   FINAL__MUST_BE_TAG                = CFLAGHSPARE_4,
   FINAL__TRIANGLE                   = CFLAGHSPARE_5,
   FINAL__LEADTRIANGLE               = CFLAGHSPARE_6,
   FINAL__UNDER_RANDOM_META          = CFLAGHSPARE_7  // This is a "yes" block under init/final/random/piecewise.
};


// Because this is used in static C-style initializers (see, for example,
// "test_setup_1x8" et. al. in sdinit.cpp and "configuration::startinfolist"
// in sdtables.cpp) we apparently can't make constructors for it, though
// we are allowed to make it a class.  And we aren't allowed to make the
// members private.  Note that the static initializers don't attempt
// to initialize this part.  So we just barely get away with this.
class final_and_herit_flags {
 public:
   heritflags herit;
   finalflags final;

   // Stuff for manipulating the "herit" bits.

   // This tests a single bit.  It returns an int rather than a bool
   // in case the client wants to combine the result with other
   // stuff for greater efficiency.
   inline uint32 test_heritbit(heritflags y) const { return herit & y; }
   // This tests against a word, presumably containing a mask of 2 or more bits.
   inline uint32 test_heritbits(uint32 y) const { return herit & y; }
   // This clears a single bit.
   inline void clear_heritbit(heritflags y) { herit = (heritflags) (herit & ~y); }
   // This clears a mask of bits.  Bits that are ON in the argument
   // get CLEARED in the field.
   inline void clear_heritbits(uint32 y) { herit = (heritflags) (herit & ~y); }
   // This sets a single bit.
   inline void set_heritbit(heritflags y) { herit = (heritflags) (herit | y); }
   // This sets a mask of bits.
   inline void set_heritbits(uint32 y) { herit = (heritflags) (herit | y); }
   // Clear all bits.
   inline void clear_all_heritbits() { herit = (heritflags) 0; }

   // Stuff for manipulating the "final" bits.  Exactly analogous
   // to the "herit" stuff.  See comments above.

   inline uint32 test_finalbit(finalflags y) { return final & y; }
   inline uint32 test_finalbits(uint32 y) { return final & y; }
   inline void clear_finalbit(finalflags y) { final = (finalflags) (final & ~y); }
   inline void clear_finalbits(uint32 y) { final = (finalflags) (final & ~y); }
   inline void set_finalbit(finalflags y) { final = (finalflags) (final | y); }
   inline void set_finalbits(uint32 y) { final = (finalflags) (final | y); }
   inline void clear_all_finalbits() { final = (finalflags) 0; }

   // Clear both herit and final bits.
   inline void clear_all_herit_and_final_bits()
      { herit = (heritflags) 0; final = (finalflags) 0; }

   // Test both fields, to see whether any bits, in either field, are on.
   inline bool test_for_any_herit_or_final_bit() { return (herit | final) != 0; }
};



class parse_block {

public:

   const concept_descriptor *concept; // the concept or end marker
   call_with_name *call;          // if this is end mark, gives the call; otherwise unused
   call_with_name *call_to_print; // the original call, for printing (sometimes the field
                                  // above gets changed temporarily)
   parse_block *next;             // next concept, or, if this is end mark,
                                  // points to substitution list
   parse_block *subsidiary_root;  // for concepts that take a second call,
                                  // this is its parse root

   final_and_herit_flags more_finalherit_flags;

   call_conc_option_state options;// number, selector, direction, etc.
   short int replacement_key;     // this is the "DFM1_CALL_MOD_MASK" stuff
                                  // (shifted down) for a modification block
   bool no_check_call_level;      // if true, don't check whether this call
                                  // is at the level
   bool say_and;                  // Used by "randomize" -- put "AND" in front of this

   // We allow static instantiation of these things with just
   // the "concept" field filled in.
   parse_block(const concept_descriptor & ccc) { initialize(&ccc); }
   // Which of course means we need to provide the default constructor too.
   parse_block() { initialize((concept_descriptor *) 0); }

   void initialize(const concept_descriptor *cc);     // In sdutil.cpp

   // In case someone runs a some kind of global memory leak detector, this releases all blocks.
   static void final_cleanup();

   static parse_block *parse_active_list;

   // Being "old school", and not fully trusting the de-fragmentation mechanism,
   // we try to do our own memory management.  So we keep an "inactive list".

   parse_block *gc_ptr;           // used for reclaiming dead blocks

   static parse_block *parse_inactive_list;
};


// This defines a person in a setup.
struct personrec {
   uint32 id1;       // Frequently used bits go here.
   uint32 id2;       // Bits used for evaluating predicates.
   uint32 id3;       // More, for unsymmetrical stuff, plus the "permanent ID" bits.
};

// Bits that go into the "id1" field.
//
// This field comprises the important part of the state of this person.
// It changes frequently as the person moves around.

enum {
   // Highest bit is unused.  (But there are more bits in id2 and id3.)

   OVERCAST_BIT     = 0x40000000U,  // person is in danger of turning overflow

   // These comprise a 2 bit field for slide info.
   NSLIDE_MASK      = 0x30000000U,  // mask of the field
   NSLIDE_BIT       = 0x10000000U,  // low bit of the field
   SLIDE_IS_R       = 0x10000000U,
   SLIDE_IS_L       = 0x20000000U,

   // These comprise a 3 bit field for roll info.
   // High bit says person moved.
   // Low 2 bits are: 1=R; 2=L; 3=M; 0=roll unknown/unsupported.
   NROLL_MASK       = 0x0E000000U,  // mask of the field
   PERSON_MOVED     = 0x08000000U,
   ROLL_DIRMASK     = 0x06000000U,  // the low 2 bits, that control roll direction
   NROLL_BIT        = 0x02000000U,  // low bit of the field
   ROLL_IS_R        = 0x02000000U,
   ROLL_IS_L        = 0x04000000U,
   ROLL_IS_M        = 0x06000000U,

   NSLIDE_ROLL_MASK = NSLIDE_MASK | NROLL_MASK,   // Commonly used together.

   // These comprise a 13 bit field for fractional stability info.  STABLE_ENAB
   // means some fractional stable (or fractional twosome) is in effect.  The
   // "R" field has the amount of fraction left to go.  It is initialized to the
   // number given (in eighths) in the fractional stable command, and counts
   // down to zero.  The "V" field is zero while the "R" field still has some
   // turning left.  Once the turning exceeds the specified amount, and the "R"
   // field is zero, the "V" field tells how this person is turned relative the
   // limit.  For example, V=6 says this person is now 6/8 clockwise (that is,
   // one quadrant counterclockwise) from where she was when the limit was
   // reached, and therefore needs to be turned 1 quadrant clockwise at the end
   // of the fractional stable call to compensate.
   //
   // All counting is done in eighths, not quarters.
   STABLE_ENAB      = 0x01000000U,  // fractional stability enabled
   STABLE_VLMASK    = 0x00F00000U,  // stability "V" field for left turns, 4 bits
   STABLE_VLBIT     = 0x00100000U,  // this is low bit
   STABLE_VRMASK    = 0x000F0000U,  // stability "V" field for right turns, 4 bits
   STABLE_VRBIT     = 0x00010000U,  // this is low bit
   STABLE_RMASK     = 0x0000F000U,  // stability "R" field, 4 bits
   STABLE_RBIT      = 0x00001000U,  // this is low bit
   STABLE_ALL_MASK  = STABLE_ENAB|STABLE_VLMASK|STABLE_VRMASK|STABLE_RMASK, // This is 0x01FFF000.

   BIT_PERSON       = 0x00000800U,  // live person (just so at least one bit is always set)
   BIT_ACT_PHAN     = 0x00000400U,  // active phantom (see below, under XPID_MASK)
   BIT_TANDVIRT     = 0x00000200U,  // virtual person (see below, under XPID_MASK)

   // Person ID.  These bit positions are extremely hard wired into, among other
   // things, the resolver and the printer.
   PID_MASK         = 0x000001C0U,  // these 3 bits identify actual person

   // Extended person ID.  These 5 bits identify who the person is for the purpose
   // of most manipulations.  0 to 7 are normal live people (the ones who squared up).
   // 8 to 15 are virtual people assembled for tandem or couples.  16 to 31 are
   // active (but nevertheless identifiable) phantoms.  This means that, when
   // BIT_ACT_PHAN is on, it usurps the meaning of BIT_TANDVIRT.
   XPID_MASK        = 0x000007C0U,

   // Remaining bits:
   // For various historical reasons, we count these in octal.
   //     0x00000030 (060) - these 2 bits must be clear for rotation
   //     0x00000008 (010) - part of rotation (facing north/south)
   //     0x00000004 (004) - bit must be clear for rotation
   //     0x00000002 (002) - part of rotation
   //     0x00000001 (001) - part of rotation (facing east/west)
   d_mask  = BIT_PERSON | 013U,
   d_north = BIT_PERSON | 010U,
   d_south = BIT_PERSON | 012U,
   d_east  = BIT_PERSON | 001U,
   d_west  = BIT_PERSON | 003U,

   // If a real person and a person under test are XOR'ed, the result AND'ed
   // with this constant, and the low bits of that result examined, it will tell
   // whether the person under test is real and facing in the same direction
   // (result = 0), the opposite direction (result == 2), or whatever.
   DIR_MASK = (BIT_PERSON | 3)
};

// Bits that go into the "id2" field.
//
// This field contains information to respond to queries like
// "Is this person a center?" that are used in doing various operations
// like "centers run".  Such operations are usually based on this field,
// rather than on the actual geometry.  This is what makes it possible
// to do things like "bounce the centers".  This field is updated, from
// the geometry, by "update_id_bits", at strategic times, such as at
// the beginning of a call.  It is not updated between the parts of
// "bounce".

enum {
   ID2_CTR2       = 0x80000000U,
   ID2_BELLE      = 0x40000000U,
   ID2_BEAU       = 0x20000000U,
   ID2_CTR6       = 0x10000000U,
   ID2_OUTR2      = 0x08000000U,
   ID2_OUTR6      = 0x04000000U,
   ID2_TRAILER    = 0x02000000U,
   ID2_LEAD       = 0x01000000U,
   ID2_CTRDMD     = 0x00800000U,
   ID2_NCTRDMD    = 0x00400000U,
   ID2_CTR1X4     = 0x00200000U,
   ID2_NCTR1X4    = 0x00100000U,
   ID2_CTR1X6     = 0x00080000U,
   ID2_NCTR1X6    = 0x00040000U,
   ID2_OUTR1X3    = 0x00020000U,
   ID2_NOUTR1X3   = 0x00010000U,
   ID2_FACING     = 0x00008000U,
   ID2_NOTFACING  = 0x00004000U,
   ID2_CENTER     = 0x00002000U,
   ID2_END        = 0x00001000U,
   ID2_CTR4       = 0x00000800U,
   ID2_OUTRPAIRS  = 0x00000400U,
   ID2_FACEFRONT  = 0x00000200U,
   ID2_FACEBACK   = 0x00000100U,
   ID2_FACELEFT   = 0x00000080U,
   ID2_FACERIGHT  = 0x00000040U,
   ID2_LEFTCOL    = 0x00000020U,
   ID2_LEFTLINE   = 0x00000010U,
   ID2_LEFTBOX    = 0x00000008U,
   ID2_RIGHTCOL   = 0x00000004U,
   ID2_RIGHTLINE  = 0x00000002U,
   ID2_RIGHTBOX   = 0x00000001U,

   // Various useful combinations.

   ID2_ABSOLUTE_PROXIMITY_BITS =
      ID2_LEFTCOL|ID2_LEFTLINE|ID2_LEFTBOX|
      ID2_RIGHTCOL|ID2_RIGHTLINE|ID2_RIGHTBOX,

   ID2_ABSOLUTE_FACING_BITS = ID2_FACEFRONT|ID2_FACEBACK|ID2_FACELEFT|ID2_FACERIGHT,

   // These bits are not a property just of the person and his position
   // in the formation -- they depend on other people's facing direction.
   ID2_BITS_NOT_INTRINSIC = ID2_FACING | ID2_NOTFACING,

   ID2_BITS_TO_CLEAR_FOR_PHANTOM_SELECTOR =
      ID2_CENTER|ID2_END|
      ID2_CTR2|ID2_CTR6|ID2_OUTR2|ID2_OUTR6|ID2_CTRDMD|ID2_NCTRDMD|
      ID2_CTR1X4|ID2_NCTR1X4|ID2_CTR1X6|ID2_NCTR1X6|
      ID2_OUTR1X3|ID2_NOUTR1X3|ID2_CTR4|ID2_OUTRPAIRS,

   ID2_BITS_TO_CLEAR_FOR_UPDATE =
      ID2_BITS_NOT_INTRINSIC |
      ID2_BITS_TO_CLEAR_FOR_PHANTOM_SELECTOR |
      ID2_LEAD|ID2_TRAILER|ID2_BEAU|ID2_BELLE
};


// Bits that go into the "id3" field.  It is sort of an extension of "id2".
//
// This field contains "permanent" information related to the identity
// of this person.  For real people, this never changes.  The reason
// this field is needed is that the bits get cobbled together (by ANDing)
// when combining people for tandem, etc.  For active phantoms, these
// bits are all zero.  This field also has unsymmetrical selector bits.

enum {
   ID3_NEARCOL      = 0x80000000U,
   ID3_NEARLINE     = 0x40000000U,
   ID3_NEARBOX      = 0x20000000U,
   ID3_NEARFOUR     = 0x10000000U,
   ID3_FARCOL       = 0x08000000U,
   ID3_FARLINE      = 0x04000000U,
   ID3_FARBOX       = 0x02000000U,
   ID3_FARFOUR      = 0x01000000U,
   ID3_NEARTHREE    = 0x00800000U,
   ID3_NEARFIVE     = 0x00400000U,
   ID3_FARTHREE     = 0x00200000U,
   ID3_FARFIVE      = 0x00100000U,
   ID3_FARTHEST1    = 0x00080000U,
   ID3_NOTFARTHEST1 = 0x00040000U,
   ID3_NEAREST1     = 0x00020000U,
   ID3_NOTNEAREST1  = 0x00010000U,
   ID3_NEARTWO      = 0x00008000U,
   ID3_FARTWO       = 0x00004000U,
   ID3_NEARSIX      = 0x00002000U,
   ID3_FARSIX       = 0x00001000U,

   // 2 available codes.

   ID3_PERM_NSG     = 0x00000200U,  // Not side girl
   ID3_PERM_NSB     = 0x00000100U,  // Not side boy
   ID3_PERM_NHG     = 0x00000080U,  // Not head girl
   ID3_PERM_NHB     = 0x00000040U,  // Not head boy
   ID3_PERM_HCOR    = 0x00000020U,  // Head corner
   ID3_PERM_SCOR    = 0x00000010U,  // Side corner
   ID3_PERM_HEAD    = 0x00000008U,  // Head
   ID3_PERM_SIDE    = 0x00000004U,  // Side
   ID3_PERM_BOY     = 0x00000002U,  // Boy
   ID3_PERM_GIRL    = 0x00000001U,  // Girl
   ID3_PERM_ALLBITS = 0x000003FFU,

   ID3_ABSOLUTE_PROXIMITY_BITS =
      ID3_NEARCOL|ID3_NEARLINE|ID3_NEARBOX|ID3_NEARFOUR|
      ID3_FARCOL|ID3_FARLINE|ID3_FARBOX|ID3_FARFOUR|
      ID3_NEARTHREE|ID3_NEARFIVE|ID3_FARTHREE|ID3_FARFIVE|
      ID3_FARTHEST1|ID3_NOTFARTHEST1|ID3_NEAREST1|ID3_NOTNEAREST1|
      ID3_NEARTWO|ID3_FARTWO|ID3_NEARSIX|ID3_FARSIX,

   // These are the bits that never change.
   ID3_PERM_ALL_ID = ID3_PERM_HEAD|ID3_PERM_SIDE|ID3_PERM_BOY|ID3_PERM_GIRL,

   // These are the standard definitions for the 8 people in the square.
   ID3_B1 = ID3_PERM_NSG|ID3_PERM_NSB|ID3_PERM_NHG|ID3_PERM_HCOR|ID3_PERM_HEAD|ID3_PERM_BOY,
   ID3_G1 = ID3_PERM_NSG|ID3_PERM_NSB|ID3_PERM_NHB|ID3_PERM_SCOR|ID3_PERM_HEAD|ID3_PERM_GIRL,
   ID3_B2 = ID3_PERM_NSG|ID3_PERM_NHG|ID3_PERM_NHB|ID3_PERM_SCOR|ID3_PERM_SIDE|ID3_PERM_BOY,
   ID3_G2 = ID3_PERM_NSB|ID3_PERM_NHG|ID3_PERM_NHB|ID3_PERM_HCOR|ID3_PERM_SIDE|ID3_PERM_GIRL,
   ID3_B3 = ID3_PERM_NSG|ID3_PERM_NSB|ID3_PERM_NHG|ID3_PERM_HCOR|ID3_PERM_HEAD|ID3_PERM_BOY,
   ID3_G3 = ID3_PERM_NSG|ID3_PERM_NSB|ID3_PERM_NHB|ID3_PERM_SCOR|ID3_PERM_HEAD|ID3_PERM_GIRL,
   ID3_B4 = ID3_PERM_NSG|ID3_PERM_NHG|ID3_PERM_NHB|ID3_PERM_SCOR|ID3_PERM_SIDE|ID3_PERM_BOY,
   ID3_G4 = ID3_PERM_NSB|ID3_PERM_NHG|ID3_PERM_NHB|ID3_PERM_HCOR|ID3_PERM_SIDE|ID3_PERM_GIRL
};

// The following items are not actually part of the setup description,
// but are placed here for the convenience of "move" and similar procedures.
// They contain information about the call to be executed in this setup.
// Once the call is complete, that is, when printing the setup or storing it
// in a history array, this stuff is meaningless.

struct assumption_thing {
   unsigned int assump_col:  16;  // Stuff to go with assumption -- col vs. line.
   unsigned int assump_both:  8;  // Stuff to go with assumption -- "handedness" enforcement
                                  // 0/1/2 = either/1st/2nd.
   unsigned int assump_cast:  6;  // Nonzero means there is an "assume normal casts"
                                  // assumption.
   unsigned int assump_live:  1;  // One means to accept only if everyone is live.
   unsigned int assump_negate:1;  // One means to invert the sense of everything.
   call_restriction assumption;   // Any "assume waves" type command.
};

struct small_setup {
   setup_kind skind;
   int srotation;
};


enum {
   PRIOR_ELONG_BASE_FOR_TANDEM = 0x00010000,
   PRIOR_ELONG_CONC_RULES_CHECK_HORRIBLE = 0x00020000
};

struct setup_command {
   parse_block *parseptr;
   call_with_name *callspec;
   final_and_herit_flags cmd_final_flags;
   fraction_command cmd_fraction;
   uint32_t cmd_misc_flags;
   uint32_t cmd_misc2_flags;
   uint32_t cmd_misc3_flags;
   uint32_t do_couples_her8itflags;
   assumption_thing cmd_assume;
   uint32_t prior_elongation_bits;
   uint32_t prior_expire_bits;
   parse_block *restrained_concept;
   parse_block **restrained_final;
   fraction_command restrained_fraction;
   uint32_t restrained_super8flags;
   uint32_t restrained_super9flags;
   bool restrained_do_as_couples;
   uint32_t restrained_miscflags;
   uint32_t restrained_misc2flags;
   uint32_t restrained_selector_decoder[2];
   uint32_t extraspecialsuperduper_misc2flags;
   parse_block *skippable_concept;
   uint32_t skippable_heritflags;
   uint32_t cmd_heritflags_to_save_from_mxn_expansion;
};


// We would like to have made this a class, with the "split_info" fields
// private, since those fields are somewhat dangerous.  But this is used
// in static aggregate initializers, so we can't.  Of course, what we
// would really like to have done, if the fields were private, is have
// the methods do tricky things by accessing the those fields as a
// single 32-bit integer, somewhat the way older (extremely dangerous)
// versions of the code used to.  But it would get us into endian-ness
// problems, and do-we-really-know-that-uint32-is-twice-as-big-as-uint16
// problems, that are simply not worth it.  Especially on 64-bit machines.

struct resultflag_rec {
   // BEWARE!!!  If add fields to this, be sure to initialize them in constructor, below.
   uint16 split_info[2];  // The split stuff.  X in slot 0, Y in slot 1.
   uint32 misc;           // Miscellaneous info, with names like RESULTFLAG__???.
   uint32 res_heritflags_to_save_from_mxn_expansion;   // Used if misc has RESULTFLAG__DID_MXN_EXPANSION bit on.
                           // Gets copied to command block for next cycle of sequential call.

   inline void clear_split_info()
   { split_info[0] = split_info[1] = 0; }

   inline void maximize_split_info()
   { split_info[0] = split_info[1] = ~0; }

   inline void copy_split_info(const resultflag_rec & rhs)
   { split_info[0] = rhs.split_info[0]; split_info[1] = rhs.split_info[1]; }

   inline void swap_split_info_fields()
   {
      uint16 temp = split_info[0];
      split_info[0] = split_info[1];
      split_info[1] = temp;
   }

   resultflag_rec() : misc(0), res_heritflags_to_save_from_mxn_expansion(0)
   { clear_split_info(); }
};


struct setup {
   setup_kind kind;
   uint16 rotation;
   uint16 eighth_rotation;
   setup_command cmd;
   personrec people[MAX_PEOPLE];

   // The following item is not actually part of the setup description, but contains
   // miscellaneous information left by "move" and similar procedures, for the
   // convenience of whatever called same.
   resultflag_rec result_flags;     // Miscellaneous info, with names like RESULTFLAG__???.

   // The following three items are only used if the setup kind is "s_normal_concentric".
   // Note in particular that "outer_elongation" is thus underutilized, and that a lot
   // of complexity goes into storing similar information (in two different places!)
   // in the "prior_elongation_bits" and "result_flags" words.
   small_setup inner;
   small_setup outer;
   int concsetup_outer_elongation;

   inline void clear_people();
   inline void clear_person(int resultplace);
   inline void suppress_roll(int place);
   inline void suppress_all_rolls(bool just_clear_the_moved_bit);
   inline void clear_all_overcasts();
   inline void swap_people(int oneplace, int otherplace);
   inline void rotate_person(int where, int rotamount);

   setup() {}
   setup(setup_kind k, int r,    // in sdtop.cpp.
         uint32 P0, uint32 I0,
         uint32 P1, uint32 I1,
         uint32 P2, uint32 I2,
         uint32 P3, uint32 I3,
         uint32 P4, uint32 I4,
         uint32 P5, uint32 I5,
         uint32 P6, uint32 I6,
         uint32 P7, uint32 I7,
         uint32 little_endian_wheretheygo = 0x76543210);
};


class conc_tables {

 private:

   // We make this a struct inside the class, rather than having its
   // fields just comprise the class itself (note that there are no
   // fields in this class, and it is never instantiated) so that
   // we can make "conc_init_table" be a statically initialized array.

   struct cm_thing {
      setup_kind bigsetup;
      calldef_schema lyzer;
      veryshort maps[24];
      setup_kind insetup;
      setup_kind outsetup;
      int inner_rot;    // 1 if inner setup is rotated CCW relative to big setup
      int outer_rot;    // 1 if outer setup is rotated CCW relative to big setup
      int mapelong;
      int center_arity;
      int elongrotallow;
      calldef_schema getout_schema;
      uint32 used_mask_analyze;
      uint32 used_mask_synthesize;
      cm_thing *next_analyze;
      cm_thing *next_synthesize;
   };

   // Must be a power of 2.
   enum { NUM_CONC_HASH_BUCKETS = 32 };

   // The big initialization table, in sdtables.
   static cm_thing conc_init_table[];

   // The hash tables, constructed during initialization.  In sdconc.
   static cm_thing *conc_hash_synthesize_table[NUM_CONC_HASH_BUCKETS];
   static cm_thing *conc_hash_analyze_table[NUM_CONC_HASH_BUCKETS];

 public:

   static bool analyze_this(             // In sdconc.
      setup *ss,
      setup *inners,
      setup *outers,
      int *center_arity_p,
      int & mapelong,
      int & inner_rot,
      int & outer_rot,
      calldef_schema analyzer);

   static bool synthesize_this(             // In sdconc.
      setup *inners,
      setup *outers,
      int center_arity,
      uint32 orig_elong_is_controversial,
      int relative_rotation,
      uint32 matrix_concept,
      int outer_elongation,
      calldef_schema synthesizer,
      calldef_schema orig_synthesizer,
      setup *result);

   static void initialize();             // In sdconc.
};


struct predicate_descriptor {
   // We wish we could put a "throw" clause on this function, but we can't.
   bool (*predfunc) (setup *, int, int, int, const int32 *);
   const int32 *extra_stuff;
};

struct predptr_pair {
   predicate_descriptor *pred;
   predptr_pair *next;
   uint16 array_pred_def[4];   // Dynamically allocated to whatever size is required.
};


enum {
   LOOKUP_NONE     = 0x1U,
   LOOKUP_DIST_CLW = 0x2U,
   LOOKUP_DISC     = 0x4U,
   LOOKUP_IGNORE   = 0x8U,
   LOOKUP_DIST_DMD = 0x10U,
   LOOKUP_Z        = 0x20U,
   LOOKUP_DIST_BOX = 0x40U,
   LOOKUP_DIAG_BOX = 0x80U,
   LOOKUP_STAG_BOX = 0x100U,
   LOOKUP_TRAPEZOID= 0x200U,
   LOOKUP_DIAG_CLW = 0x400U,
   LOOKUP_OFFS_CLW = 0x800U,
   LOOKUP_STAG_CLW = 0x1000U,
   LOOKUP_DBL_BENT = 0x2000U,
   LOOKUP_MINI_B   = 0x4000U,
   LOOKUP_MINI_O   = 0x8000U,

   LOOKUP_GEN_MASK = (LOOKUP_DIST_DMD|LOOKUP_Z|LOOKUP_DIST_BOX|LOOKUP_DIAG_BOX|
                      LOOKUP_STAG_BOX|LOOKUP_TRAPEZOID|LOOKUP_DIAG_CLW|
                      LOOKUP_OFFS_CLW|LOOKUP_STAG_CLW|LOOKUP_DBL_BENT|
                      LOOKUP_MINI_B|LOOKUP_MINI_O)
};


class select {

 public:

   enum fixerkey {
      fx0,        // The null table entry.
      fx_foo33a,
      fx_foocc,
      fx_foo33,
      fx_foocct,
      fx_foo33t,
      fx_fooccd,
      fx_foo33d,
      fx_foo55,
      fx_fooaa,
      fx_foo11,
      fx_foo22,
      fx_foo44,
      fx_foo88,
      fx_foo77,
      fx_fooEE,
      fx_fooEEprime,
      fx_1x4p2d,
      fx_1x4p2l,
      fx_4p2x1d,
      fx_phantg4a,
      fx_phantg4b,
      fx_phantg4c,
      fx_phantg4d,
      fx_1x6lowf,
      fx_1x6hif,
      fx_1x3p1lowf,
      fx_1x3p1lhif,
      fx_3p1x1lowf,
      fx_3p1x1lhif,
      fx_n1x43,
      fx_n1x4c,
      fx_n1x45,
      fx_n1x4a,
      fx_n1x3a,
      fx_n1x3b,
      fx_f1x6aa,
      fx_f1x8aa,
      fx_f1x855,
      fx_fo6zz,
      fx_foozz,
      fx_fo6zzd,
      fx_fo6zze,
      fx_foozzd,
      fx_f3x4east,
      fx_f3x4west,
      fx_f3x4down,
      fx_f3x4up,
      fx_f3x4left,
      fx_f3x4right,
      fx_f3x4lzz,
      fx_f3x4rzz,
      fx_f4x5down,
      fx_f4x5up,
      fx_f4x4nw,
      fx_f4x4ne,
      fx_f4x4wn,
      fx_f4x4en,
      fx_f4x4lzz,
      fx_f4x4rzz,
      fx_f4x4lzza,
      fx_f4x4rzza,
      fx_f2x4tt0,
      fx_f2x4tt1,
      fx_f2x8qq0,
      fx_f2x8qq1,
      fx_f2x8tt0,
      fx_f2x8tt1,
      fx_f2x8tt2,
      fx_f2x8tt3,
      fx_f4x4neq,
      fx_f4x4seq,
      fx_f4x4swq,
      fx_f4x4nwq,
      fx_f2x6qq0,
      fx_f2x6qq1,
      fx_f2x6tt0,
      fx_f2x6tt1,
      fx_f2x6tt2,
      fx_f2x6tt3,
      fx_f3x4outer,
      fx_f3dmouter,
      fx_f3ptpdin,
      fx_fpgdmdcw,
      fx_fpgdmdccw,
      fx_f2x6cw,
      fx_f2x6ccw,
      fx_fdhrgl,
      fx_specspindle,
      fx_specfix3x40,
      fx_specfix3x41,
      fx_f525nw,
      fx_f525ne,
      fx_fdpx44,
      fx_fdpxwve,
      fx_f1x12outer,
      fx_f3x4outrd,
      fx_f3dmoutrd,
      fx_fdhrgld,
      fx_f1x12outrd,
      fx_f1x12outre,
      fx_f1x3d6,
      fx_f323,
      fx_f3223,
      fx_f3x6a,
      fx_f343,
      fx_f323d,
      fx_f3x1zzd,
      fx_f1x3zzd,
      fx_f3x1yyd,
      fx_f2x1yyd,
      fx_fwstyyd,
      fx_f1x4xv,
      fx_f1x3yyd,
      fx_f1x6aad,
      fx_f1x4qv,
      fx_fo6qqd,
      fx_f21dabd,
      fx_f12dabd,
      fx_f1x6abd,
      fx_f2x1ded,
      fx_f1x6ed,
      fx_f1x8aad,
      fx_fxwv1d,
      fx_fxwv2d,
      fx_fxwv3d,
      fx_fqtgns,
      fx_ftharns,
      fx_ftharew,
      fx_falamons,
      fx_falamoew,
      fx_fqtgj1,
      fx_fqtgj2,
      fx_f2x3j1,
      fx_f2x3j2,
      fx_fqtgjj1,
      fx_fqtgjj2,
      fx_f2x3jj1,
      fx_f2x3jj2,
      fx_f2x3jj3,
      fx_fgalcv,
      fx_fgalch,
      fx_fdblbentcw,
      fx_fdblbentccw,
      fx_fcrookedcw,
      fx_fcrookedccw,
      fx_fspindlc,
      fx_fspindlf,
      fx_fspindlg,
      fx_fspindlfd,
      fx_fspindlgd,
      fx_ftgl6cwd,
      fx_ftgl6ccwd,
      fx_ftgl6cld,
      fx_ftgl6ccld,
      fx_ftgl6ccd,
      fx_ftgl6cccd,
      fx_f1x3aad,
      fx_f2x3c,
      fx_f2x3a41,
      fx_f2x3a14,
      fx_box3c,
      fx_box6c,
      fx_box9c,
      fx_boxcc,
      fx_box55,
      fx_boxaa,
      fx_f2x5c,
      fx_f2x5d,
      fx_f2x5e,
      fx_f2x5f,
      fx_sp12wing,
      fx_f4ptpd,
      fx_fd2x5d,
      fx_fd2x7d1,
      fx_fd2x7d2,
      fx_fd2x7d3,
      fx_fd2x7d4,
      fx_fd2x7q1,
      fx_fd2x7q2,
      fx_fd2x7q3,
      fx_fd2x7q4,
      fx_fd2x7za,
      fx_fd2x7zb,
      fx_bghrgla,
      fx_bghrglb,
      fx_shsqtga,
      fx_shsqtgb,
      fx_f4x6a,
      fx_f4x6b,
      fx_f4x6c,
      fx_f4x6d,
      fx_f4x6e,
      fx_f4x6f,
      fx_f1x2aad,
      fx_f2x166d,
      fx_f1x3bbd,
      fx_f1x3wwd,
      fx_fhrglassd,
      fx_fspindld,
      fx_fptpzzd,
      fx_f3ptpo6,
      fx_fspindlbd,
      fx_fspindlod,
      fx_d2x4b1,
      fx_d2x4b2,
      fx_d2x4w1,
      fx_d2x4w2,
      fx_d2x4q1,
      fx_d2x4q2,
      fx_d2x4d1,
      fx_d2x4d2,
      fx_d2x4c1,
      fx_d2x4c2,
      fx_d2x4z1,
      fx_d2x4z2,
      fx_d2x4y1,
      fx_d2x4y2,
      fx_d3x4ul1,
      fx_d3x4ul2,
      fx_d3x4ul3,
      fx_d3x4ul4,
      fx_d3x4ul5,
      fx_d3x4ul6,
      fx_d3x4ub1,
      fx_d3x4ub2,
      fx_d3x4ub3,
      fx_d3x4ub4,
      fx_d3x4ub5,
      fx_d3x4ub6,
      fx_d3x4vl1,
      fx_d3x4vl2,
      fx_d3x4vl3,
      fx_d3x4vl4,
      fx_d3x4vb1,
      fx_d3x4vb2,
      fx_d3x4vb3,
      fx_d3x4vb4,
      fx_d4x4b1,
      fx_d4x4b2,
      fx_d4x4b3,
      fx_d4x4b4,
      fx_d2x4x1,
      fx_d2x4x2,
      fx_dgalw1,
      fx_dgalw2,
      fx_dgalw3,
      fx_dgalw4,
      fx_dgald1,
      fx_dgald2,
      fx_dgald3,
      fx_dgald4,
      fx_ddmd1,
      fx_ddmd2,
      fx_distbone1,
      fx_distbone2,
      fx_distbone5,
      fx_distbone6,
      fx_distpgdmd1,
      fx_distpgdmd2,
      fx_distrig3,
      fx_distrig1,
      fx_distrig4,
      fx_distrig2,
      fx_distrig7,
      fx_distrig5,
      fx_distrig8,
      fx_distrig6,
      fx_distptp1,
      fx_distptp2,
      fx_distptp3,
      fx_distptp4,
      fx_distptp7,
      fx_distptp8,
      fx_disthrg1,
      fx_disthrg2,
      fx_d4x4l1,
      fx_d4x4l2,
      fx_d4x4l3,
      fx_d4x4l4,
      fx_d4x4d1,
      fx_d4x4d2,
      fx_d4x4d3,
      fx_d4x4d4,
      fx_dqtgdbb1,
      fx_dqtgdbb2,
      fx_hrgminb,
      fx_ptpminb,
      fx_rigmino,
      fx_galmino,
      fx_qtgminb,
      fx_fcpl12,
      fx_fcpl23,
      fx_fcpl34,
      fx_fcpl41,
      fx_foo55d,
      fx_fgalctb,
      fx_f3x1ctl,
      fx_f2x2pl,
      fx_f1x4pl,
      fx_fdmdpl,
      fx_f1x2pl,
      fx_f3x1d_2,
      fx_f1x8_88,
      fx_f1x8_22,
      fx_f1x8_11,
      fx_fdqtagzzz,
      fx_f1x8_77_3,
      fx_f1x8_77,
      fx_fdrhgl_bb,
      fx_f3x1d77,
      fx_f1x8_bb,
      fx_f1x8_dd,
      fx_f1x10abd,
      fx_foo99d,
      fx_foo66d,
      fx_f1x8ctr,
      fx_fqtgctr,
      fx_fxwve,
      fx_fboneendd,
      fx_fqtgend,
      fx_f1x6endd,
      fx_f1x2dd,
      fx_f3ptp,
      fx_f3dmdln,
      fx_fbn6ndd,
      fx_f2x3od,
      fx_fdmded,
      fx_f1x4ed,
      fx_fdrhgl1,
      fx_f1x8endd,
      fx_f1x8endo,
      fx_fbonectr,
      fx_fbonetgl,
      fx_frigtgl,
      fx_fboneendo,
      fx_frigendd,
      fx_frigctr,
      fx_f2x4ctr,
      fx_f2x4lr,
      fx_f2x4rl,
      fx_f4dmdiden,
      fx_1x5p1d,
      fx_1x5p1e,
      fx_1x5p1f,
      fx_1x5p1g,
      fx_1x5p1h,
      fx_1x5p1j,
      fx_1x5p1k,
      fx_linefbox0,
      fx_linefbox1,
      fx_linefbox2,
      fx_linefbox3,
      fx_linefbox4,
      fx_l2b1,
      fx_l2b2,
      fx_l2b3,
      fx_l2b4,
      fx_l2b5,
      fx_l2b6,
      fx_l2b7,
      fx_l2b8,
      fx_l6b1,
      fx_l6b2,
      fx_l6b3,
      fx_l6b4,
      fx_l6b5,
      fx_l6b6,
      fx_l6b7,
      fx_l6b8,
      fx_l6b9,
      fx_l6b10,
      fx_l6b11,
      fx_l6b12,
      fx_l6b13,
      fx_l6b14,
      fx_l6b15,
      fx_l6b16,
      fx_dbt1,
      fx_dbt2,
      fx_dbt3,
      fx_dbt4,
      fx_dbt1a,
      fx_dbt2a,
      fx_dbt1b,
      fx_dbt2b,
      fx_dbt1d,
      fx_dbt3t,
      fx_dbt6t,
      fx_dbt6u,
      fx_dbt7t,
      fx_dbt1l,
      fx_1x5p1y,
      fx_1x5p1z,
      fx_5p1x1d,
      fx_plndmda,
      fx_5p1x1z,
      fx_linpdma,
      fx_lndmda,
      fx_linpdmd,
      fx_lndmd9,
      fx_linpdm8,
      fx_lndmd8,
      fx_boxdma,
      fx_boxdmb,
      fx_boxdmc,
      fx_boxdmd,
      fx_boxpdma,
      fx_boxpdmb,
      fx_beehive1,
      fx_beehive2,
      fx_beehive3,
      fx_spnnrtgl,
      fx_spnfrtgl,
      fx_galnrtgl,
      fx_galfrtgl,
      fx_galnrvee,
      fx_galfrvee,
      fx_trngl8a,
      fx_trngl8b,
      fxdmdpdmda,
      fxdmdpdmdb,
      fxdmdpdmdc,
      fx323a,
      fx323b,
      fx323c,
      fx323d,
      fxntrgla,
      fxntrglb,
      // ***** New ones go here!
      fxboxdmda,
      fxboxdmdb,
      fxboxdmdc,
      fxboxpdmda,
      fxboxpdmdb,
      fxboxpdmdc,
      fxboxpdmdd,
      fxboxpdmde,
      fxlinboxa,
      fxlinboxb,
      fx23232a,
      fx23232b,
      fx23232c,
      fx23232d,
      fx23232e,
      fx23232u,
      fx23232v,
      fx23232w,
      fx23232x,
      fx_f1x8lowf,
      fx_f1x8hif,
      fx_f1x8low2,
      fx_f1x8hi2,
      fx_f1x8low6,
      fx_f1x8hi6,
      fx_f1x8lodbt4,
      fx_f1x8hidbt4,
      fx_fqtglowf,
      fx_fqtghif,
      fx_fdmdlowf,
      fx_fdmdhif,
      fx_fdmdlow3,
      fx_fdmdhi3,
      fx_f1x8low3,
      fx_f1x8hi3,
      fx_f2x4far,
      fx_f2x4near,
      fx_f2x4pos1,
      fx_f2x4pos2,
      fx_f2x4pos3,
      fx_f2x4pos4,
      fx_f2x4pos5,
      fx_f2x4pos6,
      fx_f2x4pos7,
      fx_f2x4pos8,
      fx_f2x4pos02,
      fx_f2x4pos13,
      fx_f2x4posa,
      fx_f2x4posb,
      fx_f2x4posc,
      fx_f2x4posd,
      fx_f2x4pose,
      fx_f2x4posf,
      fx_f2x4posp,
      fx_f2x4posq,
      fx_f2x4posr,
      fx_f2x4poss,
      fx_f2x4posu,
      fx_f2x4posv,
      fx_f2x4posw,
      fx_f2x4posx,
      fx_f2x4posy,
      fx_f2x4posz,
      fx_f2x4left,
      fx_f2x4right,
      fx_f2x4dleft,
      fx_f2x4dright,
      fx_f2zzrdsc,
      fx_f2zzrdsd,
      fx_f288rdsd,
      fx_f2yyrdsc,
      fx_f2x5rdsd,
      fx_f2qt1dsc,
      fx_f2qt1dsd,
      fx_f2qt2dsc,
      fx_f2qt2d99,
      fx_f2x6ndsc,
      fx_f2x6ndsd,
      fx_fdmdndx,
      fx_fdmdnd,
      fx_f3x4ndsd,
      fx_f2x8nd99,
      fx_f2x8nda5,
      fx_f4x6ndoo,
      fx_f4x6ndxx,
      fx_f1x8nd96,
      fx_f1x8nd69,
      fx_f1x8nd41,
      fx_f1x8nd82,
      fx_f1x8nd28,
      fx_f1x8nd14,
      fx_f1x10ndsc,
      fx_f1x10ndsd,
      fx_f1x10ndse,
      fx_z1x4u,
      fx_z2x3a,
      fx_z2x3b,
      fx_fgalpts,
      fx_f2x4endd,
      fx_fhrgl1,
      fx_fhrgl2,
      fx_fhrgle,
      fx_fptpdid,
      fx_phanigna,
      fx_phanignb,
      fx_phanignc,
      fx_phanignd,
      fx_phanigne,
      fx_phanignf,
      fx_phanigng,
      fx_phanignh,
      fx_f2x477,
      fx_f2x4ee,
      fx_f2x4bb,
      fx_f2x4dd,
      fx_fdhrgld1,
      fx_fdhrgld2,
      fx_f2x4endo,
      fx_f4x4endo,
      fx_bar55d,
      fx_fppaad,
      fx_fpp55d,
      fx_4x5eight0,
      fx_4x5eight1,
      fx_4x5eight2,
      fx_4x5eight3,
      fx_ENUM_EXTENT   // Not a fixer; indicates extent of the enum.
   };

   // We make this a struct inside the class, rather than having its
   // fields just comprise the class itself (note that there are no
   // fields in this class, and it is never instantiated) so that
   // we can make "fixer_init_table" be a statically initialized array.

   struct fixer {
      fixerkey mykey;
      setup_kind ink;
      setup_kind outk;
      uint32_t rot;
      short prior_elong;
      short numsetups;
      veryshort indices[24];
      fixerkey next1x2;
      fixerkey next1x2rot;
      fixerkey next1x4;
      fixerkey next1x4rot;
      fixerkey nextdmd;
      fixerkey nextdmdrot;
      fixerkey next2x2;
      fixerkey next2x2v;
   };

   // We make this a struct inside the class, rather than having its
   // fields just comprise the class itself (note that there are no
   // fields in this class, and it is never instantiated) so that
   // we can make "sel_init_table" be a statically initialized array.

   struct sel_item {
      uint32 key;
      setup_kind kk;
      uint32 thislivemask;
      fixerkey fixp;
      fixerkey fixp2;
      int use_fixp2;
      sel_item *next;
   };

 private:

   // Must be a power of 2.
   enum { NUM_SEL_HASH_BUCKETS = 32 };

   // The big initialization table, in sdtables.
   static sel_item sel_init_table[];

   // The hash table, constructed during initialization.  In sdconc.
   static sel_item *sel_hash_table[NUM_SEL_HASH_BUCKETS];

   // The big initialization table, in sdtables.
   static fixer fixer_init_table[];

 public:

   // In sdconc.
   static fixer *fixer_ptr_table[fx_ENUM_EXTENT];
   static void initialize();             // In sdconc.

   // In sdconc.
   static const fixer *hash_lookup(setup_kind kk, uint32 thislivemask,
                                   bool allow_phantoms,
                                   uint32 key, uint32 arg, const setup *ss);
};

class tglmap {

 public:

   enum tglmapkey {
      tgl0,        // The null table entry.
      tglmap1b,
      tglmap2b,
      tglmap1i,
      tglmap2i,
      tglmap1d,
      tglmap2d,
      tglmap1m,
      tglmap2m,
      tglmap1j,   // In/out point interlocked, in qtag.
      tglmap2j,   // In/out point interlocked, in qtag.
      tglmap3j,   // Inside interlocked, in qtag.
      tglmap1x,
      tglmap2x,
      tglmap1y,
      tglmap2y,
      tglmap1k,
      tglmap2k,
      tglmap2r,
      tglmap2p,
      tglmap1t,
      tglmap2t,
      tglmap3t,
      tglmapd71,
      tglmapd72,
      tg6mapcw,
      tg6mapccw,
      tglmaps6,
      tglmapb6,
      tglmap323_33,
      tglmap323_66,
      tgl_ENUM_EXTENT   // Not a key; indicates extent of the enum.
   };

   // We make this a struct inside the class, rather than having its
   // fields just comprise the class itself (note that there are no
   // fields in this class, and it is never instantiated) so that
   // we can make "init_table", and other items, be statically
   // initialized arrays.

   struct map {
      tglmapkey mykey;
      setup_kind kind;
      setup_kind kind1x3;
      tglmapkey otherkey;
      veryshort nointlkshapechange;
      veryshort randombits;
      veryshort mapqt1[8];   // In quarter-tag: first triangle (upright),
                             // then second triangle (inverted), then idle.
      veryshort mapcp1[8];   // In C1 phantom: first triangle (inverted),
                             // then second triangle (upright), then idle.
      veryshort mapbd1[8];   // In bigdmd.
      veryshort map241[8];   // In 2x4.
      veryshort map261[8];   // In 2x6.
   };

 private:

   // The big initialization table, in sdtables.
   static const map init_table[];

   // The pointer table, constructed during initialization.
   static const map *ptrtable[tgl_ENUM_EXTENT];

 public:

   static void initialize();             // In sdistort.

   static void do_glorious_triangles(    // In sdistort.
      setup *ss,
      const tglmapkey *map_ptr_table,
      int indicator,
      setup *result) THROW_DECL;

   // In sdtables.
   static const tglmapkey t6cwtglmap1[];
   static const tglmapkey t6ccwtglmap1[];
   static const tglmapkey s6tglmap1[];
   static const tglmapkey b6tglmap1[];
   static const tglmapkey c1tglmap1[];
   static const tglmapkey c1tglmap2[];
   static const tglmapkey dbqtglmap1[];
   static const tglmapkey dbqtglmap2[];
   static const tglmapkey qttglmap1[];
   static const tglmapkey qttglmap2[];
   static const tglmapkey bdtglmap1[];
   static const tglmapkey bdtglmap2[];
   static const tglmapkey rgtglmap1[];
   static const tglmapkey rgtglmap2[];
   static const tglmapkey rgtglmap3[];
   static const tglmapkey ritglmap1[];
   static const tglmapkey ritglmap2[];
   static const tglmapkey ritglmap3[];
   static const tglmapkey d7tglmap1[];
   static const tglmapkey d7tglmap2[];
   static const tglmapkey s323map33[];
   static const tglmapkey s323map66[];
};


typedef uint32 id_bit_table[4];

struct ctr_end_mask_rec {
   uint32 mask_normal;
   uint32 mask_6_2;
   uint32 mask_2_6;
   uint32 mask_ctr_dmd;
};


struct coordrec {
   int get_index_from_coords(int x, int y) const
   {
      int mag = xfactor >> 4;
      int place = diagram[32 - (1 << ((xfactor&0xF)-1)) - ((y >> mag) << (xfactor&0xF)) + (x >> mag)];
      if (place < 0 || (xca[place] != x) || (yca[place] != y))
         return -1;
      return place;
   }

   setup_kind result_kind;
   int xfactor;
   veryshort xca[24];
   veryshort yca[24];
   veryshort diagram[64];
};


// Beware!  There are >= tests lying around, so order is important.
// In particular, sdconc (search for "brute_force_merge") has such tests.
enum merge_action {
   merge_strict_matrix,
   merge_for_own,
   merge_c1_phantom,
   merge_c1_phantom_real_couples,
   merge_c1_phantom_real,
   merge_after_dyp,
   merge_without_gaps
};

class merge_table {

 public:

   // We make this a struct inside the class, rather than having its
   // fields just comprise the class itself (note that there are no
   // fields in this class, and it is never instantiated) so that
   // we can make "merge_init_table" be a statically initialized array.

   struct concmerge_thing {
      setup_kind k1;
      setup_kind k2;
      uint32 m1;
      uint32 m2;
      // See comments in sdtables.cpp at definition of merge_table::merge_init_table
      // for explanation of these next two fields.
      unsigned short rotmask;
      unsigned short swap_setups;
      calldef_schema conc_type;
      setup_kind innerk;
      setup_kind outerk;
      warning_index warning;
      int irot;
      int orot;
      veryshort innermap[16];
      veryshort outermap[24];  // Only one map (bigh+4x6) needs more than 16
      concmerge_thing *next;
   };

 private:

   // Must be a power of 2.
   enum { NUM_MERGE_HASH_BUCKETS = 64 };

   // The big initialization table, in sdtables.
   static concmerge_thing merge_init_table[];

   // The hash table, constructed during initialization.  In sdconc.
   static concmerge_thing *merge_hash_tables[NUM_MERGE_HASH_BUCKETS];

 public:

   static const concmerge_thing map_tgl4l;
   static const concmerge_thing map_tgl4b;
   static const concmerge_thing map_2234b;
   static const concmerge_thing map_24r24a;
   static const concmerge_thing map_24r24b;
   static const concmerge_thing map_24r24c;
   static const concmerge_thing map_24r24d;

   static void merge_setups(setup *ss,   // In sdconc.
                            merge_action action,
                            setup *result,
                            call_with_name *maybe_the_call = (call_with_name *) 0) THROW_DECL;

   static void initialize();             // In sdconc.
};

struct setup_attr {
   // This is the size of the setup MINUS ONE.
   int setup_limits;

   // These "coordrec" items have the fudged coordinates that are used for doing
   // press/truck calls.  For some setups, the coordinates of some people are
   // deliberately moved away from the obvious precise matrix spots so that
   // those people can't press or truck.  For example, the lateral spacing of
   // diamond points is not an integer.  If a diamond point does any truck or loop
   // call, he/she will not end up on the other diamond point spot (or any other
   // spot in the formation), so the call will not be legal.  This enforces our
   // view, not shared by all callers (Hi, Clark!) that the diamond points are NOT
   // as if the ends of lines of 3, and hence can NOT trade with each other by
   // doing a right loop 1.
   const coordrec *setup_coords;

   // The above table is not suitable for performing mirror inversion because,
   // for example, the points of diamonds do not reflect onto each other.  This
   // table has unfudged coordinates, in which all the symmetries are observed.
   // This is the table that is used for mirror reversal.  Most of the items in
   // it are the same as those in the table above.
   const coordrec *nice_setup_coords;

   // These determine how designators like "side boys" get turned into
   // "center 2", so that so-and-so moves can be done with the much
   // more powerful concentric mechanism.
   ctr_end_mask_rec setup_conc_masks;

   // These show the beginning setups that we look for in a by-array call
   // definition in order to do a call in this setup.
   begin_kind keytab[2];

   // In the bounding boxes, we do not fill in the "length" of a diamond, nor
   // the "height" of a qtag.  Everyone knows that the number must be 3, but it
   // is not really accepted that one can use that in instances where precision
   // is required.  That is, one should not make "matrix" calls depend on this
   // number.  Witness all the "diamond to quarter tag adjustment" stuff that
   // callers worry about, and the ongoing controversy about which way a quarter
   // tag setup is elongated, even though everyone knows that it is 4 wide and 3
   // deep, and that it is generally recognized, by the mathematically erudite,
   // that 4 is greater than 3.
   short int bounding_box[2];

   // This is true if the setup has 4-way symmetry.  Such setups will always be
   // canonicalized so that their rotation field will be zero.
   bool four_way_symmetry;

   // This is true if the setup has no (that is, 1-way) symmetry.  Such setups
   // will never be canonicalized -- their rotation field may be 0, 1, 2, or 3.
   bool no_symmetry;

   // This is the bit table for filling in the "ID2" bits.
   const id_bit_table *id_bit_table_ptr;

   // These are the tables that show how to print out the setup.
   Cstring print_strings[2];
};


extern const setup_attr setup_attrs[];    // In sdtables.


// These are the "schema_attr" bits.
enum {
   SCA_CENTRALCONC     = 0x00000001U,
   SCA_CROSS           = 0x00000002U,
   SCA_COPY_LYZER      = 0x00000004U,
   SCA_SNAGOK          = 0x00000008U,
   SCA_DETOUR          = 0x00000010U,
   SCA_SPLITOK         = 0x00000020U,
   SCA_INV_SUP_ELWARN  = 0x00000040U,
   SCA_CONC_REV_ORDER  = 0x00000080U,
   SCA_NO_OVERCAST     = 0x00000100U,
   SCA_REMOVE_VERIFY   = 0x00000200U
};

struct schema_attr {
   uint32 attrs;
   calldef_schema uncrossed;
};


extern const schema_attr schema_attrs[];    // In sdtables.

// This class just exists for the purpose of reading out the setup_limits.
// It had much more glorious plans at one time.

class attr {

 public:

   // Get the "limit" (that's the number of people minus one), given
   // only a setup kind.
   static inline int klimit(setup_kind k) { return setup_attrs[k].setup_limits; }

   // Get the limit for a given setup, reading out its setup kind.
   static inline int slimit(const setup *s) { return setup_attrs[s->kind].setup_limits; }
};


/* These enumerate the setups from which we can perform a "normalize" search. */
/* This list tracks the array "nice_setup_info". */

enum nice_start_kind {
   nice_start_4x4,
   nice_start_3x4,
   nice_start_2x8,
   nice_start_2x6,
   nice_start_1x10,
   nice_start_1x12,
   nice_start_1x14,
   nice_start_1x16,
   nice_start_3dmd,
   nice_start_4dmd,
   nice_start_4x6,
   NUM_NICE_START_KINDS   // End mark; not really in the enumeration.
};

struct nice_setup_thing {
   const useful_concept_enum *zzzfull_list;
   useful_concept_enum *zzzon_level_list;
   int full_list_size;
};

struct nice_setup_info_item {
   setup_kind kind;
   nice_setup_thing *thing;
   const useful_concept_enum *array_to_use_now;
   int number_available_now;
};



/* Flags that reside in the "result_flags" word of a setup AFTER a call is executed.

   The low two bits tell, for a 2x2 setup after
   having a call executed, how that 2x2 is elongated in the east-west or
   north-south direction.  Since 2x2's are always totally canonical, the
   interpretation of the elongation direction is always absolute.  A 1 in this
   field means the elongation is east-west.  A 2 means the elongation is
   north-south.  A zero means the elongation is unknown.  These bits will be set
   whenever we know which elongation is preferred.  This will be the case if the
   setup was an elongated 2x2 prior to the call (because they are the ends), or
   the setup was a 1x4 prior to the call.  So, for example, after any recycle
   from a wave, these bits will be set to indicate that the dancers want to
   elongate the 2x2 perpendicular to their original wave.  These bits will be
   looked at only if the call was directed to the ends (starting from a grand
   wave, for example.)  Furthermore, the bits will be overridden if the
   "concentric" or "checkpoint" concepts were used, or if the ends did this
   as part of a concentrically defined call for which some forcing modifier
   such as "force_columns" was used.  Note that these two bits are in the same
   format as the "cmd.prior_elongation" field of a setup before the call is
   executed.  In many case, that field can be simply copied into the "result_flags"
   field.

   RESULTFLAG__DID_LAST_PART means that, when a sequentially defined call was executed
   with the "cmd_frac_flags" word nonzero, so that just one part was done,
   that part was the last part.  Hence, if we are doing a call with some "piecewise"
   or "random" concept, we do the parts of the call one at a time, with appropriate
   concepts on each part, until it comes back with this flag set.  This is used with
   RESULTFLAG__PARTS_ARE_KNOWN.

   RESULTFLAG__PARTS_ARE_KNOWN means that the bit in RESULTFLAG__DID_LAST_PART is
   valid.  Sometimes we are unable to obtain information about parts, because
   no one is doing the call.  This can legally happen, for example, on "interlocked
   parallelogram leads shakedown while the trailers star thru" from waves.  In each
   virtual 2x2 setup one or the other of the calls is being done by no one.

   RESULTFLAG__EXPAND_TO_2X3 means that a call was executed that takes a four person
   starting setup (typically a line) and yields a 2x3 result setup.  Two of those
   result spots will be phantoms, of course.  When recombining a divided setup in
   which such a call was executed, if the recombination wuold yield a 2x6, and if
   the center 4 spots would be empty, this flag directs divided_setup_move to
   collapse the setup to a 2x4.  This is what makes calls like "pair the line"
   and "step and slide" behave correctly from parallel lines and from a grand line.

   RESULTFLAG__NEED_DIAMOND means that a call has been executed with the "magic" or
   "interlocked" modifier, and it really should be changed to the "magic diamond"
   concept.  Otherwise, we might end up saying "magic diamond single wheel" when
   we should have said "magic diamond, diamond single wheel".

   RESULTFLAG__ACTIVE_PHANTOMS_ON and _OFF tell whether the state of the "active
   phantoms" mode flag was used in this sequence.  If it was read and found to
   be on, RESULTFLAG__ACTIVE_PHANTOMS_ON is set.  If it was read and found to
   be off, RESULTFLAG__ACTIVE_PHANTOMS_OFF is set.  It is, of course, read any
   time an "assume <whatever> concept is invoked.  This information is used to
   tell what kind of annotation to place in the transcript file to tell what
   assumptions were made.  The reason for having both bits is that the state
   of the active phantoms mode can be toggled at arbitrary times, so a single
   card might have both usages.  In that unusual case, we want to so indicate
   on the card.

   RESULTFLAG__DID_SHORT6_2X3 means that we fudged a short6 to a 2x3
   (with permission), and, if we are putting back a concentric formation,
   we may need to act accordingly.

   The "split_info" field has info saying whether the call was split
   vertically (2) or horizontally (1) in "absolute space".  We can't say for
   sure whether the orientation is absolute, since a client may have stripped
   out rotation info.  What we mean is that it is relative to the incoming setup
   including the effect of its rotation field.  So, if the incoming setup was
   a 2x4 with rotation=0, that is, horizontally oriented in "absolute space",
   1 means it was done in 2x2's (split horizontally) and 2 means it was done
   in 1x4's (split vertically).  If the incoming setup was a 2x4 with
   rotation=1, that is, vertically oriented in "absolute space", 1 means it
   was done in 1x4's (split horizontally) and 2 means it was done in 2x2's
   (split vertically).  3 means that the call was a 1 or 2 person call, that
   could be split either way, so we have no information.
*/

enum {
   // The two low bits are used for result elongation, so bits start with 0x00000004.
   // This is a four bit field.
   RESULTFLAG__PART_COMPLETION_BITS = 0x0000003CU,
   RESULTFLAG__DID_LAST_PART        = 0x00000004U,
   RESULTFLAG__DID_NEXTTOLAST_PART  = 0x00000008U,
   RESULTFLAG__SECONDARY_DONE       = 0x00000010U,
   RESULTFLAG__PARTS_ARE_KNOWN      = 0x00000020U,

   RESULTFLAG__NEED_DIAMOND         = 0x00000040U,
   RESULTFLAG__DID_MXN_EXPANSION    = 0x00000080U,
   RESULTFLAG__COMPRESSED_FROM_2X3  = 0x00000100U,
   // 3 spare bits here
   RESULTFLAG__ACTIVE_PHANTOMS_ON   = 0x00001000U,
   RESULTFLAG__ACTIVE_PHANTOMS_OFF  = 0x00002000U,
   RESULTFLAG__EXPAND_TO_2X3        = 0x00004000U,

   // This is a 5 bit field.
   RESULTFLAG__EXPIRATION_BITS      = 0x000F8000U,
   RESULTFLAG__YOYO_ONLY_EXPIRED    = 0x00008000U,
   RESULTFLAG__GEN_STING_EXPIRED    = 0x00010000U,
   RESULTFLAG__TWISTED_EXPIRED      = 0x00020000U,
   RESULTFLAG__SPLIT_EXPIRED        = 0x00040000U,
   RESULTFLAG__EXPIRATION_ENAB      = 0x00080000U,

   RESULTFLAG__DID_TGL_EXPANSION    = 0x00100000U,
   RESULTFLAG__VERY_ENDS_ODD        = 0x00200000U,
   RESULTFLAG__VERY_CTRS_ODD        = 0x00400000U,

   // This is a 2 bit field.
   RESULTFLAG__DID_Z_COMPRESSMASK   = 0x01800000U,
   RESULTFLAG__DID_Z_COMPRESSBIT    = 0x00800000U,

   RESULTFLAG__NO_REEVALUATE        = 0x02000000U,
   RESULTFLAG__REALLY_NO_REEVALUATE = 0x04000000U,
   RESULTFLAG__IMPRECISE_ROT        = 0x08000000U,
   RESULTFLAG__DID_SHORT6_2X3       = 0x10000000U,
   RESULTFLAG__FORCE_SPOTS_ALWAYS   = 0x20000000U,
   RESULTFLAG__INVADED_SPACE        = 0x40000000U,
   RESULTFLAG__STOP_OVERCAST_CHECK  = 0x80000000U
   // No spares!  Actually, 3 spares above.
};



/* These flags go into the "concept_prop" field of a "concept_table_item".

   CONCPROP__SECOND_CALL means that the concept takes a second call, so a sublist must
      be created, with a pointer to same just after the concept pointer.

   CONCPROP__USE_SELECTOR means that the concept requires a selector, which must be
      inserted into the concept list just after the concept pointer.

   CONCPROP__NEEDK_4X4   mean that the concept requires the indicated setup, and, at
   CONCPROP__NEEDK_2X8   the top level, the existing setup should be expanded as needed.
      etc....

   CONCPROP__SET_PHANTOMS means that phantoms are in use under this concept, so that,
      when looking for tandems or couples, we shouldn't be disturbed if we
      pair someone with a phantom.  It is what makes "split phantom lines tandem"
      work, so that "split phantom lines phantom tandem" is unnecessary.

   CONCPROP__NO_STEP means that stepping to a wave or rearing back from one is not
      allowed under this concept.

   CONCPROP__GET_MASK means that tbonetest & livemask need to be computed before executing the concept.

   CONCPROP__STANDARD means that the concept can be "standard".

   CONCPROP__USE_NUMBER         Concept takes one number.
   CONCPROP__USE_TWO_NUMBERS    Concept takes two numbers.
   CONCPROP__USE_FOUR_NUMBERS   Etc.

   CONCPROP__SHOW_SPLIT means that the concept prepares the "split_axis" bits properly
      for transmission back to the client.  Normally this is off, and the split axis bits
      will be cleared after execution of the concept.

   CONCPROP__NEED_ARG2_MATRIX means that the "arg2" word of the concept_descriptor
      block contains additional bits of the "CONCPROP__NEEDK_3X8" kind to be sent to
      "do_matrix_expansion".  This is done so that concepts with different matrix
      expansion bits can share the same concept type.
*/



enum {
   CONCPROP__SECOND_CALL     = 0x00000001U,
   CONCPROP__USE_SELECTOR    = 0x00000002U,
   CONCPROP__SET_PHANTOMS    = 0x00000004U,
   CONCPROP__NO_STEP         = 0x00000008U,

   // This is a five bit field.  CONCPROP__NEED_LOBIT marks its low bit.
   // WARNING!!!  The values in this field are encoded into a bit field
   // for the setup expansion/normalization tables (see the definition
   // of the macro "NEEDMASK".)  It follows that there can't be more than 32 of them.
   CONCPROP__NEED_MASK       = 0x000001F0U,
   CONCPROP__NEED_LOBIT      = 0x00000010U,
   CONCPROP__NEEDK_4X4       = 0x00000010U,
   CONCPROP__NEEDK_2X8       = 0x00000020U,
   CONCPROP__NEEDK_2X6       = 0x00000030U,
   CONCPROP__NEEDK_4DMD      = 0x00000040U,
   CONCPROP__NEEDK_BLOB      = 0x00000050U,
   CONCPROP__NEEDK_4X6       = 0x00000060U,
   CONCPROP__NEEDK_3X8       = 0x00000070U,
   CONCPROP__NEEDK_3DMD      = 0x00000080U,
   CONCPROP__NEEDK_1X10      = 0x00000090U,
   CONCPROP__NEEDK_1X12      = 0x000000A0U,
   CONCPROP__NEEDK_3X4       = 0x000000B0U,
   CONCPROP__NEEDK_1X16      = 0x000000C0U,
   CONCPROP__NEEDK_QUAD_1X4  = 0x000000D0U,
   CONCPROP__NEEDK_TWINDMD   = 0x000000E0U,
   CONCPROP__NEEDK_TWINQTAG  = 0x000000F0U,
   CONCPROP__NEEDK_CTR_DMD   = 0x00000100U,
   CONCPROP__NEEDK_END_DMD   = 0x00000110U,
   CONCPROP__NEEDK_TRIPLE_1X4= 0x00000120U,
   CONCPROP__NEEDK_CTR_1X4   = 0x00000130U,
   CONCPROP__NEEDK_END_1X4   = 0x00000140U,
   CONCPROP__NEEDK_CTR_2X2   = 0x00000150U,
   CONCPROP__NEEDK_END_2X2   = 0x00000160U,
   CONCPROP__NEEDK_3X4_D3X4  = 0x00000170U,
   CONCPROP__NEEDK_3X6       = 0x00000180U,
   CONCPROP__NEEDK_4D_4PTPD  = 0x00000190U,
   CONCPROP__NEEDK_4X5       = 0x000001A0U,
   CONCPROP__NEEDK_2X10      = 0x000001B0U,
   CONCPROP__NEEDK_2X12      = 0x000001C0U,
   CONCPROP__NEEDK_DBLX      = 0x000001D0U,
   CONCPROP__NEEDK_DEEPXWV   = 0x000001E0U,
   CONCPROP__NEEDK_QUAD_1X3  = 0x000001F0U,

   CONCPROP__NEED_ARG2_MATRIX= 0x00000200U,
   CONCPROP__USE_DIRECTION   = 0x00000400U,
   /* spare:                   0x00000800U, */
   /* spare:                   0x00010000U, */
   /* spare:                   0x00020000U, */
   CONCPROP__USES_PARTS      = 0x00040000U,
   CONCPROP__IS_META         = 0x00080000U,
   CONCPROP__GET_MASK        = 0x00100000U,
   CONCPROP__STANDARD        = 0x00200000U,
   CONCPROP__USE_NUMBER      = 0x00400000U,
   CONCPROP__USE_TWO_NUMBERS = 0x00800000U,
   CONCPROP__USE_FOUR_NUMBERS= 0x01000000U,
   CONCPROP__MATRIX_OBLIVIOUS= 0x02000000U,
   CONCPROP__PERMIT_MATRIX   = 0x04000000U,
   CONCPROP__SHOW_SPLIT      = 0x08000000U,
   CONCPROP__PERMIT_MYSTIC   = 0x10000000U,
   CONCPROP__PERMIT_REVERSE  = 0x20000000U,
   CONCPROP__PERMIT_MODIFIERS= 0x40000000U
};

// Used by distorted_move.
enum distort_key {
   DISTORTKEY_DIST_CLW,
   DISTORTKEY_OFFSCLW_SINGULAR,
   DISTORTKEY_TIDALCLW,
   DISTORTKEY_DIST_QTAG,
   DISTORTKEY_CLW_OF_3,
   DISTORTKEY_STAGGER_CLW_OF_3,
   DISTORTKEY_OFFS_SPL_PHAN_BOX,
   DISTORTKEY_OFFS_QTAG,
   DISTORTKEY_OFFS_TRIPLECLW,
   DISTORTKEY_OFFS_TRIPLE_BOX
};


// A "configuration" is a state in the evolving sequence.
// There is a global array of these, making up the sequence itself.
// It is in the static array "history", running from 1 or 2 up to "config_history_ptr".

class configuration {
 public:
   parse_block *command_root;
   setup state;
   bool state_is_valid;
   bool draw_pic;
   int text_line;          // How many lines of text existed after this item was written,
                           // only meaningful if "written_history_items" is >= this index.

   static const resolve_tester *null_resolve_ptr;    /* in SDTOP */

 private:
   resolve_indicator resolve_flag;
   warning_info warnings;

 public:

   // These next two things used to be private.  Now used in sdtop near line 5780.

   // This is the index into the "startinfolist".  It is only nonzero for history[1].
   // It shows how the sequence starts.
   int startinfoindex;

   // This constant table tells how to set up the configuration at the start of a sequence.
   // values that might be in "startinfoindex", e.g. sides start or heads 1P2P.
   static startinfo startinfolist[start_select_as_they_are+1];    // In sdinit.cpp

   static void initialize_startinfolist_item(start_select_kind k,
                                             const char *name,
                                             bool into_the_middle,
                                             setup *the_setup_p)
      {
         configuration::startinfolist[k].name = name;
         configuration::startinfolist[k].into_the_middle = into_the_middle;
         configuration::startinfolist[k].the_setup_p = the_setup_p;
      }

   static void initialize();             // In sdinit.

   // The sequence being written is in a global array "history".  The item
   // indexed by "config_history_ptr" is the "current" configuration, and the next higher
   // one is the "next" configuration.  Each configuration has:
   //   "state" -- a formation.  (While doing a call, the "cmd" part of the
   //              state will have the call do be done *from* that formation,
   //              which will be initialized from the *next higher* "command_root".)
   //   "command_root" -- the parse tree for the call that got *to* that formation.
   //   "warnings" -- the warnings, if any, that were raised by that call.
   //   "resolve_flag" -- the resolve info, if any, for that formation.  That is,
   //              the resolve *before* the call was executed.
   //
   // While we are working a call, the current configuration (indexed by
   //   "config_history_ptr", and given by "current_config") has the formation before
   //   doing the call.  Its "cmd" part has the actual call.  The next
   //   configuration (given by "next_config") has the call in its "command_root",
   //   and receives the result formation, the warnings that the call may have
   //   raised, and the resolve for that result.

   static configuration *history;                     // in SDTOP
   static int whole_sequence_low_lim;                 // in SDTOP

   inline static configuration & current_config() { return history[config_history_ptr]; }
   inline static configuration & next_config() { return history[config_history_ptr+1]; }

   inline static int concepts_in_place()
      { return next_config().command_root != 0; }

   inline void init_centersp_specific() { startinfoindex = 0; }
   inline static void initialize_history(int c) {
      config_history_ptr = 1;
      history[1].startinfoindex = c;
      history[1].draw_pic = false;
      history[1].state_is_valid = false;
      whole_sequence_low_lim = (startinfolist[c].into_the_middle) ? 2 : 1;
   }
   inline bool nontrivial_startinfo_specific() { return startinfoindex != 0; }
   inline startinfo *get_startinfo_specific() { return &startinfolist[startinfoindex]; }
   inline void init_resolve() { resolve_flag.the_item = null_resolve_ptr; }
   void calculate_resolve();                          // in SDGETOUT
   inline static resolve_indicator & current_resolve() { return current_config().resolve_flag; }
   inline static resolve_indicator & next_resolve() { return next_config().resolve_flag; }
   static bool sequence_is_resolved();                // in SDGETOUT

   inline void restore_warnings_specific(const warning_info & rhs)
      { warnings = rhs; }
   inline void init_warnings_specific()
      { warnings = warning_info(); }
   inline void clear_one_warning_specific(warning_index i)
      { warnings.clearbit(i); }
   inline void set_one_warning_specific(warning_index i)
      { warnings.setbit(i); }
   inline bool test_one_warning_specific(warning_index i) const { return warnings.testbit(i); }

   inline bool warnings_are_different(const configuration & rhs) const
      { return warnings != rhs.warnings; }

   // This one has a wrapper, accessible in sdui.h, called config_save_warnings.
   inline static warning_info save_warnings()
      { return next_config().warnings; }
   // This one too.
   inline static void restore_warnings(const warning_info & rhs)
      { next_config().warnings = rhs; }
   inline static void init_warnings()
      { next_config().warnings = warning_info(); }
   inline static void set_one_warning(warning_index i)
      { next_config().set_one_warning_specific(i); }
   inline static void clear_one_warning(warning_index i)
      { next_config().clear_one_warning_specific(i); }
   inline static void set_multiple_warnings(const warning_info & rhs)
      { next_config().warnings.setmultiple(rhs); }
   inline static void clear_multiple_warnings(const warning_info & rhs)
      { next_config().warnings.clearmultiple(rhs); }
   inline static bool test_multiple_warnings(const warning_info & rhs)
      { return next_config().warnings.testmultiple(rhs); }
};


struct concept_table_item {
   uint32 concept_prop;      /* Takes bits of the form CONCPROP__??? */
   // We wish we could put a "throw" clause on this function, but we can't.
   void (*concept_action)(setup *, parse_block *, setup *);
};


/* It should be noted that the CMD_MISC__??? and RESULTFLAG__XXX bits have
   nothing to do with each other.  It is not intended that
   the flags resulting from one call be passed to the next.  In fact, that
   would be incorrect.  The CMD_MISC__??? bits should start at zero at the
   beginning of each call, and accumulate stuff as the call goes deeper into
   recursion.  The RESULTFLAG__??? bits should, in general, be the OR of the
   bits of the components of a compound call, though this may not be so for
   the elongation bits at the bottom of the word. */



/* Meanings of the items in a "setup_command" structure:

   parseptr
      The full parse tree for the concept(s)/call(s) we are trying to do.
      While we traverse the big concepts in this tree, the "callspec" and
      "cmd_final_flags" are typically zero.  They only come into use when
      we reach the end of a subtree of big concepts, at which point we read
      the remaining "small" (or "final") concepts and the call name out of
      the end of the concept parse tree.

   callspec
      The call, after we reach the end of the parse tree.

   cmd_final_flags
      The various "final concept" flags with names INHERITFLAG_??? and FINAL__???.
      The INHERITFLAG_??? bits contain the final concepts like "single" that
      can be inherited from one part of a call definition to another.  The
      FINAL__??? bits contain other miscellaneous final concepts.

   cmd_misc_flags
      Other miscellaneous info controlling the execution of the call,
      with names like CMD_MISC__???.

   cmd_misc2_flags
      Even more miscellaneous info controlling the execution of the call,
      with names like CMD_MISC2__???.

   do_couples_heritflags
      These are "heritable" flags that tell what single/3x3/etc modifiers
      are to be used in determining what to do when CMD_MISC__DO_AS_COUPLES
      is specified.

   cmd_frac_flags
      If nonzero, fractionalization info controlling the execution of the call.
      See the comments in front of "get_fraction_info" in sdmoves.cpp for details.

   cmd_assume
      Any "assume waves" type command.

   skippable_concept
      For "<so-and-so> work <concept>", this is the concept to be skipped by some people.

   skippable_heritflags
      As above, when it's a simple modifier.

   prior_elongation_bits
      This tells, for a 2x2 setup prior to having a call executed, how that
      2x2 is elongated (due to these people being the outsides) in the east-west
      or north-south direction.  Since 2x2's are always totally canonical, the
      interpretation of the elongation direction is always absolute.  A 1 means
      the elongation is east-west.  A 2 means the elongation is north-south.
      A zero means there was no elongation.

      The 0x40 bit of this is sometimes set, in sdconc.cpp, for a 1x2/1x4/1x6,
      to indicate that these people are disconnected.  It is looked at in sdbasic.cpp.
      Also, some stuff shifted left by 8 bits is sometimes put here by sdconc.cpp.
      Needed by t49\3045.  Something about triple percolate.  This is indicated by
      PRIOR_ELONG_BASE_FOR_PERCOLATE.    (Not actually implemented this way, pending investigation.)
      Also, tandem operations may put elongation into a 2x2 to aid in semi-bogus
      "crazy" stuff.  This is indicated by PRIOR_ELONG_BASE_FOR_TANDEM.
*/


/* Flags that reside in the "cmd_misc_flags" word of a setup BEFORE a call is executed.

   BEWARE!! These flags co-exist in the cmd_misc_flags word with copies of some
   of the call invocation flags.  The mask for those flags is
   DFM1_CONCENTRICITY_FLAG_MASK.  Those flags, and that mask, are defined
   in database.h .  We must define these flags starting where the concentricity
   flags end.

   CMD_MISC__EXPLICIT_MIRROR means that the setup has been mirrored by the "mirror"
   concept, separately from anything done by "left" or "reverse".  Such a mirroring
   does NOT cause the "take right hands" stuff to compensate by going to left hands,
   so it has the effect of making the people physically take left hands.

   CMD_MISC__MATRIX_CONCEPT means that the "matrix" concept has been given, and we have
   to be very careful about what we can do.  Specifically, this will make everything illegal
   except calls to suitable "split phantom" concepts.

   CMD_MISC__VERIFY_WAVES means that, before doing the call, we have to act as though the
   "assume waves" concept had been given.  That is, we have to verify that the setup is in fact
   waves, and we have to set the assumption stuff, so that a "with active phantoms" concept
   can be handled.  This flag is used only through "divided_setup_move", to propagate
   information from a concept like "split phantom waves" through the division, and then
   cause the assumption to be acted upon in each resulting setup.  It is removed immediately
   by "divided_setup_move" after use.

   CMD_MISC__EXPLICIT_MATRIX means that the caller said "4x4 matrix" or "2x6 matrix" or whatever,
   so we got to this matrix explicitly.  This enables natural splitting of the setup, e.g. form
   a parallelogram, "2x6 matrix 1x2 checkmate" is legal -- the 2x6 gets divided naturally
   into 2x3's.

   CMD_MISC__NO_EXPAND_MATRIX means that we are at a level of recursion that
   no longer permits us to do the implicit expansion of the matrix
   (e.g. add outboard phantoms to turn a 2x4 into a 2x6 if the concept
   "triple box" is used) that some concepts perform at the top level.

   CMD_MISC__DISTORTED means that we are at a level of recursion in which some
   distorted-setup concept has been used.  When this is set, "matrix" (a.k.a.
   "space invader") calls, such as press and truck, are not permitted.  Sorry, Clark.

   CMD_MISC__OFFSET_Z means that we are doing a 2x3 call (the only known case
   of this is "Z axle") that came from Z's picked out of a 4x4 setup.  The map
   that we are using has a "50% offset" map schema, so that, if the call goes
   into 1x4's (which Z axle does) it will put the results into a parallelogram,
   in accordance with customary usage.  The problem is that, if the call goes
   into a setup other than a 1x4 oriented in the appropriate way, using the
   50% offset schema will lead to the wrong result.  For example, if a call
   were defined that went from Z's into 2x2's, the program would attempt to
   bizarrely offset boxes.  (Yes, I know that Z axle is the only call in the
   database that can cause Z's to be picked out of a 4x4.  The program is just
   trying to be very careful.)  So, when this flag is on and a shape-changer
   occurs, the program insists on a parallelogram result.

   CMD_MISC__SAID_SPLIT means that we are at a level of recursion in which
   the "split" concept has been used.  When this is on, it is not permissible
   to do a 4-person call that has not 8-person definition, and hence would have
   been done the same way without the "split" concept.  This prevents superfluous
   things like "split pass thru".

   CMD_MISC__NO_CHK_ELONG means that the elongation of the incoming setup is for
   informational purposes only (to tell where people should finish) and should not
   be used for raising error messages.  It suppresses the error that would be
   raised if we have people in a facing 2x2 star thru when they are far from the
   ones they are facing.

   CMD_MISC__PHANTOMS means that we are at a level of recursion in which some phantom
   concept has been used, or when "on your own" or "so-and-so do your part" concepts
   are in use.  It indicates that we shouldn't be surprised if there are phantoms
   in the setup.  When this is set, the "tandem" or "as couples" concepts will
   forgive phantoms wherever they might appear, so that "split phantom lines tandem
   <call>" is legal.  Otherwise, we would have to say "split phantom lines phantom
   tandem <call>.  This flag also allows "basic_move" to be more creative about
   inferring the starting position for a call.  If I know that I am doing a
   "so-and-so do your part" operation, it may well be that, even though the setup
   is, say, point-to-point diamonds, the centers of the diamonds are none of my
   business (I am a point), and I want to think of the setup as a grand wave,
   so I can do my part of a grand mix.  The "CMD_MISC__PHANTOMS" flag makes it
   possible to do a grand mix from point-to-point diamonds in which the centers
   are absent.

   CMD_MISC__NO_STEP_TO_WAVE means that we are at a level of recursion that
   no longer permits us to do the implicit step to a wave or rear back from one
   that some calls permit at the top level.
*/


/* Same, for the "cmd_misc3_flags" word.
   CMD_MISC3__SAID_TRIANGLE means that we are at a level of recursion in which
   the word "triangle" has been uttered, so it is OK to do things like "triangle
   peel and trail" without giving the explicit "triangle" concept again.  This
   makes it possible to say things like "tandem-based triangles peel and trail".

   Similarly for CMD_MISC3__SAID_DIAMOND, etc.

   CMD_MISC3__DO_AS_COUPLES means that the "couples_unless_single" invocation
   flag is on, and the call may need to be done as couples or whatever.  The
   bits in "do_couples_heritflags" control this.  If INHERITFLAG_SINGLE is on,
   do not do it as couples.  If off, do it as couples.  If various other bits
   are on (e.g. INHERITFLAG_1X3), do the appropriate thing.
*/


// Beware!  These flags must be disjoint from DFM1_CONCENTRICITY_FLAG_MASK, in database.h .
// If that changes, these flags need to be chacked.
// Since DFM1_CONCENTRICITY_FLAG_MASK is FF, we start at 100 hex.

enum {
   CMD_MISC__EXPLICIT_MIRROR      = 0x00000100U,
   CMD_MISC__MATRIX_CONCEPT       = 0x00000200U,

   // This is a 4 bit field.
   CMD_MISC__VERIFY_MASK          = 0x00003C00U,
   // Here are the encodings that can go into same.
   CMD_MISC__VERIFY_WAVES         = 0x00000400U,
   CMD_MISC__VERIFY_2FL           = 0x00000800U,
   CMD_MISC__VERIFY_DMD_LIKE      = 0x00000C00U,
   CMD_MISC__VERIFY_QTAG_LIKE     = 0x00001000U,
   CMD_MISC__VERIFY_1_4_TAG       = 0x00001400U,
   CMD_MISC__VERIFY_3_4_TAG       = 0x00001800U,
   CMD_MISC__VERIFY_REAL_1_4_TAG  = 0x00001C00U,
   CMD_MISC__VERIFY_REAL_3_4_TAG  = 0x00002000U,
   CMD_MISC__VERIFY_REAL_1_4_LINE = 0x00002400U,
   CMD_MISC__VERIFY_REAL_3_4_LINE = 0x00002800U,
   CMD_MISC__VERIFY_LINES         = 0x00002C00U,
   CMD_MISC__VERIFY_COLS          = 0x00003000U,
   CMD_MISC__VERIFY_TALL6         = 0x00003400U,

   CMD_MISC__EXPLICIT_MATRIX      = 0x00004000U,

   CMD_MISC__NO_EXPAND_1          = 0x00008000U,  // Allow only one triple box expansion.
   CMD_MISC__NO_EXPAND_2          = 0x00010000U,  // Allow only one split phantom C/L/W expansion.
   CMD_MISC__NO_EXPAND_AT_ALL     = 0x00020000U,  // Positively no expansion.
   CMD_MISC__NO_EXPAND_MATRIX = CMD_MISC__NO_EXPAND_1 | CMD_MISC__NO_EXPAND_2 | CMD_MISC__NO_EXPAND_AT_ALL,

   CMD_MISC__DISTORTED            = 0x00040000U,
   CMD_MISC__OFFSET_Z             = 0x00080000U,
   CMD_MISC__SAID_SPLIT           = 0x00100000U,
   //   CMD_MISC__                = 0x00200000U,    // Unused
   //   CMD_MISC__                = 0x00400000U,    // Unused
   CMD_MISC__SAID_PG_OFFSET       = 0x00800000U,  // Explicitly said it, so space-invasion rules don't apply.
   CMD_MISC__NO_CHECK_MOD_LEVEL   = 0x01000000U,
   CMD_MISC__MUST_SPLIT_HORIZ     = 0x02000000U,
   CMD_MISC__MUST_SPLIT_VERT      = 0x04000000U,
   CMD_MISC__NO_CHK_ELONG         = 0x08000000U,
   CMD_MISC__PHANTOMS             = 0x10000000U,
   CMD_MISC__NO_STEP_TO_WAVE      = 0x20000000U,
   CMD_MISC__ALREADY_STEPPED      = 0x40000000U,
   CMD_MISC__DID_LEFT_MIRROR      = 0x80000000U,

   CMD_MISC__MUST_SPLIT_MASK      = (CMD_MISC__MUST_SPLIT_HORIZ|CMD_MISC__MUST_SPLIT_VERT)
};


// Flags that reside in the "cmd_misc2_flags" word of a setup BEFORE a call is executed.

// The following are used for the "<anyone> work <concept>" or "snag the <anyone>" mechanism:
//    CMD_MISC2__ANY_WORK or CMD_MISC2__ANY_SNAG is on if there is such an
//    operation in place.  We will make the centers or ends (depending on
//    CMD_MISC2__ANY_WORK_INVERT) use the next concept (or do 1/2 of the call)
//    while the others skip that concept.  The schema, which is one of
//    schema_concentric_2_6, schema_concentric_6_2, schema_concentric, or
//    schema_single_concentric, is in the low 16 bits.
//
//    CMD_MISC2__ANY_WORK_INVERT is only meaningful if the CMD_MISC2__ANY_WORK is on.
//    It says that the ends are doing the concept, instead of the centers.

enum {

   // The low 12 bits are used for encoding the schema if
   // CMD_MISC2__ANY_WORK or CMD_MISC2__ANY_SNAG is on.

   // This indicates that the setup is actually a 2x3, but the "Z" concept has been given,
   //    and the setup should probably be turned into a 2x2.  The only exception
   //    is if the call takes a 2x3 starting setup but not a 2x2 (that is, the call
   //    is "Z axle" or "counter rotate").  In that case, the call is done directly
   //    in the 2x3, and the "Z" distortion is presumed not to have been in place.
   CMD_MISC2__REQUEST_Z         = 0x00001000U,
   // spare:         = 0x00002000U,
   // spare:         = 0x00004000U,
   // spare:         = 0x00008000U,

   CMD_MISC2_RESTRAINED_SUPER   = 0x00010000U,

   // The following two are used for "mystic".

   // This tells "divided_setup_move" to perform selective mirroring
   // of the subsidiary setups because a concept like "mystic triple boxes" is in use.
   // It is removed immediately by "divided_setup_move" after use.
   CMD_MISC2__MYSTIFY_SPLIT     = 0x00020000U,

   // This is only meaningful when CMD_MISC2__MYSTIFY_SPLIT is on.
   // It says that the concept is actually "invert mystic triple boxes" or whatever.
   CMD_MISC2__MYSTIFY_INVERT    = 0x00040000U,

   CMD_MISC2__ANY_WORK          = 0x00080000U,
   CMD_MISC2__ANY_SNAG          = 0x00100000U,
   CMD_MISC2__ANY_WORK_INVERT   = 0x00200000U,

   // Here are the inversion bits for the basic operations.
   CMD_MISC2__INVERT_CENTRAL    = 0x00400000U,
   CMD_MISC2__INVERT_SNAG       = 0x00800000U,
   CMD_MISC2__INVERT_MYSTIC     = 0x01000000U,

   // The following are used for what we call the "center/end" mechanism.  This
   //    mechanism is used for the "invert" (centers and ends) concept, as well as
   //    "central", "snag", and "mystic" and inverts thereof.
   //
   // Here are the basic operations we can do.
   CMD_MISC2__DO_CENTRAL        = 0x02000000U,
   CMD_MISC2__CENTRAL_SNAG      = 0x04000000U,
   CMD_MISC2__CENTRAL_MYSTIC    = 0x08000000U,
   // This field embraces the above 3 bits.  When nonzero, says that one of the "central", "snag",
   //    or "mystic" concepts is in use.  They are all closely related.
   CMD_MISC2__CTR_END_KMASK     = 0x0E000000U,

   // This says the the operator said "invert".  It might later cause
   // a "central" to be turned into an "invert central".
   CMD_MISC2__SAID_INVERT       = 0x10000000U,

   // This mask embraces this whole mechanism, including the "invert" bit.
   CMD_MISC2__CTR_END_MASK      = 0x1FC00000U,

   CMD_MISC2__DO_NOT_EXECUTE    = 0x20000000U,
   CMD_MISC2__ANY_WORK_CALL_CROSSED=0x40000000U
};


// Flags that reside in the "cmd_misc3_flags" word of a setup BEFORE a call is executed.
enum {
   CMD_MISC3__PUT_FRAC_ON_FIRST    = 0x00000002U,
   CMD_MISC3__RESTRAIN_CRAZINESS   = 0x00000004U,
   CMD_MISC3__RESTRAIN_MODIFIERS   = 0x00000008U,
   CMD_MISC3__META_NOCMD           = 0x00000010U,
   CMD_MISC3__DO_AS_COUPLES        = 0x00000020U,
   CMD_MISC3__DOING_YOUR_PART      = 0x00000040U,    // Some kind of "DYP" has happened, setups may be bizarre.
   CMD_MISC3__NEED_DIAMOND         = 0x00000080U,
   CMD_MISC3__DOING_ENDS           = 0x00000100U,    // This call is directed only to the ends of the original setup.
                                                     // If the call turns out to be an 8-person call with distinct
                                                     // centers and ends parts, we may want to just apply the ends part.
                                                     // This is what makes "ends detour" work.
   CMD_MISC3__TWO_FACED_CONCEPT    = 0x00000200U,
   CMD_MISC3__NO_ANYTHINGERS_SUBST = 0x00000400U,    // Treat "<anything> motivate" as plain motivate.
   CMD_MISC3__PARENT_COUNT_IS_ONE  = 0x00000800U,
   CMD_MISC3__IMPOSE_Z_CONCEPT     = 0x00001000U,
   CMD_MISC3__DONE_WITH_REST_SUPER = 0x00002000U,
   CMD_MISC3__STOP_OVERCAST_CHECK  = 0x00004000U,    // Off at start of utterance, gets turned on after first part.
                                                     // This is how we enforce the "no overcast warnings for actions
                                                     // internal to a compound call" rule.
   CMD_MISC3__ROLL_TRANSP          = 0x00008000U,
   CMD_MISC3__ROLL_TRANSP_IF_Z     = 0x00010000U,
   CMD_MISC3__SUPERCALL            = 0x00020000U,
   CMD_MISC3__SAID_TRIANGLE        = 0x00040000U,
   CMD_MISC3__SAID_DIAMOND         = 0x00080000U,
   CMD_MISC3__SAID_GALAXY          = 0x00100000U,
   CMD_MISC3__SAID_Z               = 0x00200000U,
   CMD_MISC3__TRY_MIMIC_LINES      = 0x00400000U,
   CMD_MISC3__TRY_MIMIC_COLS       = 0x00800000U,

   // This is a 2 bit field.
   CMD_MISC3__DID_Z_COMPRESSMASK   = 0x03000000U,
   CMD_MISC3__DID_Z_COMPRESSBIT    = 0x01000000U,

   CMD_MISC3__ACTUAL_Z_CONCEPT     = 0x04000000U,

   // This refers to the special invocation of a "optional_special_number" call;
   // call is being given an optional numeric arg because of really hairy fraction.
   CMD_MISC3__SPECIAL_NUMBER_INVOKE= 0x08000000U
};

enum normalize_action {

   normalize_before_isolated_callMATRIXMATRIXMATRIX,

   simple_normalize,
   normalize_after_exchange_boxes,

   normalize_before_isolated_call,
   normalize_before_isolate_not_too_strict,
   plain_normalize,
   normalize_after_triple_squash,
   normalize_to_6,
   normalize_to_4,
   normalize_to_2,
   normalize_after_disconnected,
   normalize_before_merge,
   normalize_strict_matrix,
   normalize_compress_bigdmd,
   normalize_recenter,
   normalize_never
};


class expand {

 public:

   // We make this a struct inside the class, rather than having its
   // fields just comprise the class itself (note that there are no
   // fields in this class, and it is never instantiated) so that
   // we can make "init_table", and many other items, be statically
   // initialized arrays.

   struct thing {
      veryshort source_indices[24];
      setup_kind inner_kind;
      setup_kind outer_kind;
      int rot;
      uint32 lillivemask;    // Little-endian.
      uint32 biglivemask;    // Little-endian.
      bool must_be_exact_level;
      warning_index expwarning;
      warning_index norwarning;
      normalize_action action_level;
      uint32 expandconcpropmask;
      thing *next_expand;
      thing *next_compress;
   };

   static void initialize();

   static void compress_setup(const thing & thing, setup *stuff) THROW_DECL;   // In sdtop.

   static void expand_setup(const thing & thing, setup *stuff) THROW_DECL;     // In sdtop.

   static void fix_3x4_to_qtag(setup *stuff) THROW_DECL;                       // In sdtop.

   static bool compress_from_hash_table(setup *ss,
                                        normalize_action action,
                                        uint32 livemask,
                                        bool noqtagcompress) THROW_DECL;

   static bool expand_from_hash_table(setup *ss,
                                      uint32 needpropbits,
                                      uint32 livemask) THROW_DECL;

 private:

   // Must be a power of 2.
   enum { NUM_EXPAND_HASH_BUCKETS = 32 };

   static thing *expand_hash_table[NUM_EXPAND_HASH_BUCKETS];
   static thing *compress_hash_table[NUM_EXPAND_HASH_BUCKETS];

   static thing init_table[];
};

class full_expand {

 public:

   // We make this a struct inside the class, rather than having its
   // fields just comprise the class itself (note that there are no
   // fields in this class, and it is never instantiated) so that
   // we can make a number of items be statically initialized arrays.

   struct thing {
      warning_index warning;
      // Low 2 bits = elongation bits to forbid;
      // "4" bit = must set elongation.
      // Also, the "8" bit means to use "gather" and do this the other way.
      // Also, the "16" bit means allow only step to a box, not step to a full wave.
      // Also, the "32" bit means to give "some people touch" warning if this is
      //     a "step_to_wave_4_people" call.
      // Also, the "64" bit means this is an item relating to having just the centers
      // touch to a miniwave, and we may need to give the "warn__some_touch_evil"
      // warning.
      // Also, the "128" bit means this is an item that lets you do a swing thru
      // from the partially occupied 2x4 that would result from everyone doing a
      // 1/2 press back from a right-hand wave.  We don't allow the touch to a wave
      // if the user gave a phantom concept.
      int forbidden_elongation;
      expand::thing *expand_lists;
      setup_kind kind;
      uint32 live;
      uint32 dir;
      uint32 dirmask;
      thing *next;
   };

   static void initialize_touch_tables();
   static thing *search_table_1(setup_kind kind, uint32 livemask, uint32 directions);
   static thing *search_table_2(setup_kind kind, uint32 livemask, uint32 directions);
   static thing *search_table_3(setup_kind kind,
                                uint32 livemask, uint32 directions, uint32 touchflags);

 private:

   // Must be a power of 2.
   enum { NUM_TOUCH_HASH_BUCKETS = 32 };

   static thing *touch_hash_table1[NUM_TOUCH_HASH_BUCKETS];
   static thing *touch_hash_table2[NUM_TOUCH_HASH_BUCKETS];
   static thing *touch_hash_table3[NUM_TOUCH_HASH_BUCKETS];
};


enum phantest_kind {
   /* These control error messages that arise when we divide a setup
      into subsetups (e.g. phantom lines) and find that one of
      the setups is empty.  For example, if we are in normal lines
      and select "phantom lines", it will notice that one of the setups
      (the outer one) isn't occupied and will give the error message
      "Don't use phantom concept if you don't mean it." */

   phantest_ok,               // Anything goes
   phantest_impossible,       // Can't happen in symmetric stuff??
   phantest_both,             // Require both setups (partially) occupied
   phantest_only_one,         // Require only one setup occupied, don't care which
   phantest_only_first_one,   // Require only first setup occupied, second empty
   phantest_only_second_one,  // Require only second setup occupied, first empty
   phantest_only_one_pair,    // Require only first 2 (of 4) or only last 2
   phantest_first_or_both,    // Require first setup only, or a mixture
   phantest_ctr_phantom_line, // Special, created when outside phantom setup is empty.
   phantest_2x2_both,
   phantest_2x2_only_two,
   phantest_not_just_centers
};

enum disttest_kind {
   disttest_t, disttest_nil, disttest_only_two,
   disttest_any, disttest_offset, disttest_z};


enum restriction_test_result {
   restriction_passes,
   restriction_fails,
   restriction_bad_level,
   restriction_fails_on_2faced,
   restriction_no_item
};

struct concept_fixer_thing {
   uint32 newheritmods;
   uint32 newfinalmods;
   useful_concept_enum before;
   useful_concept_enum after;
};

enum selective_key {
   // Warning!!!!  Order is important!  See all the stupid ways these are used
   // in sdconc.cpp.
   selective_key_dyp,
   selective_key_dyp_for_mystic,
   selective_key_own,
   selective_key_plain_no_live_subsets,  // there is an
                                         // "indicator < selective_key_plain_no_live_subsets"
   selective_key_plain,       // there is an "indicator >= selective_key_plain"
   selective_key_disc_dist,   // there is an "indicator < selective_key_disc_dist"
                              // and an "indicator >= selective_key_disc_dist"
   selective_key_ignore,
   selective_key_work_concept,
   selective_key_lead_for_a,
   selective_key_promenade_and_lead_for_a,
   selective_key_work_no_concentric,
   selective_key_snag_anyone,
   selective_key_plain_from_id_bits,
   selective_key_mini_but_o
};

enum tandem_key {
   tandem_key_tand = 0,
   tandem_key_cpls = 1,
   tandem_key_siam = 2,
   tandem_key_overlap_siam = 3,
   tandem_key_tand3 = 4,
   tandem_key_cpls3 = 5,
   tandem_key_siam3 = 6,
   tandem_key_tand4 = 8,
   tandem_key_cpls4 = 9,
   tandem_key_siam4 = 10,
   tandem_key_box = 16,
   tandem_key_diamond = 17,
   tandem_key_skew = 18,
   tandem_key_outpoint_tgls = 20,
   tandem_key_inpoint_tgls = 21,
   tandem_key_inside_tgls = 22,
   tandem_key_outside_tgls = 23,
   tandem_key_wave_tgls = 26,
   tandem_key_tand_tgls = 27,
   tandem_key_anyone_tgls = 30,
   tandem_key_3x1tgls = 31,
   tandem_key_ys = 32,
   tandem_key_special_triangles = 33
};

enum part_key_kind {
   part_key_follow_by,
   part_key_precede_by,
   part_key_start_with,
   part_key_use_nth_part,
   part_key_use,
   part_key_half_and_half,
   part_key_frac_and_frac,
   part_key_use_last_part
};

// BEWARE!!  This list is keyed to the table "meta_key_props" in sdtables.cpp .
enum meta_key_kind {
   meta_key_random,
   meta_key_rev_random,   // Must follow meta_key_random.
   meta_key_piecewise,
   meta_key_finish,
   meta_key_revorder,
   meta_key_roundtrip,
   meta_key_like_a,
   meta_key_finally,
   meta_key_initially_and_finally,
   meta_key_nth_part_work,
   meta_key_first_frac_work,
   meta_key_skip_nth_part,
   meta_key_skip_last_part,
   meta_key_shift_n,
   meta_key_echo,
   meta_key_rev_echo,   // Must follow meta_key_echo.
   meta_key_randomize,
   meta_key_shift_half
};


// These are the "meta_key_props" bits.
enum mkprop {
   MKP_RESTRAIN_1    = 0x00000001U,
   MKP_RESTRAIN_2    = 0x00000002U,
   MKP_COMMA_NEXT    = 0x00000004U
};

extern const uint32 meta_key_props[];    // In sdtables.

enum revert_weirdness_type {
   weirdness_off,
   weirdness_flatten_from_3,
   weirdness_otherstuff
};


enum split_command_kind {
   split_command_none,
   split_command_1x4,
   split_command_1x8,
   split_command_2x3,
};




/* VARIABLES */


extern SDLIB_API char error_message1[MAX_ERR_LENGTH];               /* in SDTOP */
extern SDLIB_API char error_message2[MAX_ERR_LENGTH];               /* in SDTOP */
extern SDLIB_API bool enforce_overcast_warning;                     /* in SDTOP */
extern SDLIB_API uint32 collision_person1;                          /* in SDTOP */
extern SDLIB_API uint32 collision_person2;                          /* in SDTOP */
extern SDLIB_API int history_allocation;                            /* in SDTOP */
extern SDLIB_API int written_history_items;                         /* in SDTOP */

extern SDLIB_API call_with_name **base_calls;                       /* in SDTOP */
extern SDLIB_API dance_level level_threshholds_for_pick[];          /* in SDTOP */
extern SDLIB_API int hashed_randoms;                                /* in SDTOP */

extern SDLIB_API error_flag_type global_error_flag;                 /* in SDUTIL */
extern SDLIB_API bool global_cache_failed_flag;                     /* in SDUTIL */
extern SDLIB_API int global_cache_miss_reason[3];                   /* in SDUTIL */
extern SDLIB_API uims_reply_thing global_reply;                     /* in SDUTIL */
extern configuration *clipboard;                                    /* in SDUTIL */
extern int clipboard_size;                                          /* in SDUTIL */
extern bool retain_after_error;                                     /* in SDUTIL */
extern SDLIB_API const Cstring concept_key_table[];                 /* in SDUTIL */

extern SDLIB_API const concept_table_item concept_table[];          /* in SDCONCPT */
extern uint32 global_tbonetest;                                     /* in SDCONCPT */
extern uint32 global_livemask;                                      /* in SDCONCPT */
extern uint32 global_selectmask;                                    /* in SDCONCPT */
extern uint32 global_tboneselect;                                   /* in SDCONCPT */

extern parse_block *last_magic_diamond;                             /* in SDTOP */
extern warning_info no_search_warnings;                             /* in SDTOP */
extern warning_info conc_elong_warnings;                            /* in SDTOP */
extern warning_info dyp_each_warnings;                              /* in SDTOP */
extern warning_info useless_phan_clw_warnings;                      /* in SDTOP */
extern int concept_sublist_sizes[call_list_extent];                 /* in SDTOP */
extern short int *concept_sublists[call_list_extent];               /* in SDTOP */
extern int good_concept_sublist_sizes[call_list_extent];            /* in SDTOP */
extern short int *good_concept_sublists[call_list_extent];          /* in SDTOP */

extern predicate_descriptor pred_table[];                           /* in SDPREDS */
extern int selector_preds;                                          /* in SDPREDS */


extern const ctr_end_mask_rec dead_masks;                           /* in SDTABLES */
extern const ctr_end_mask_rec masks_for_3x4;                        /* in SDTABLES */
extern const ctr_end_mask_rec masks_for_3dmd_ctr2;                  /* in SDTABLES */
extern const ctr_end_mask_rec masks_for_3dmd_ctr4;                  /* in SDTABLES */
extern const ctr_end_mask_rec masks_for_bigh_ctr4;                  /* in SDTABLES */
extern const ctr_end_mask_rec masks_for_4x4;                        /* in SDTABLES */
extern int begin_sizes[];                                           /* in SDCOMMON */

extern const coordrec tgl3_0;                                       /* in SDTABLES */
extern const coordrec tgl3_1;                                       /* in SDTABLES */
extern const coordrec tgl4_0;                                       /* in SDTABLES */
extern const coordrec tgl4_1;                                       /* in SDTABLES */

extern id_bit_table id_bit_table_2x5_z[];                           /* in SDTABLES */
extern id_bit_table id_bit_table_2x5_ctr6[];                        /* in SDTABLES */
extern id_bit_table id_bit_table_d2x7a[];                           /* in SDTABLES */
extern id_bit_table id_bit_table_d2x7b[];                           /* in SDTABLES */
extern id_bit_table id_bit_table_d2x5_ctr6[];                       /* in SDTABLES */
extern id_bit_table id_bit_table_2x6_pg[];                          /* in SDTABLES */
extern id_bit_table id_bit_table_bigdmd_wings[];                    /* in SDTABLES */
extern id_bit_table id_bit_table_bigbone_wings[];                   /* in SDTABLES */
extern id_bit_table id_bit_table_bighrgl_wings[];                   /* in SDTABLES */
extern id_bit_table id_bit_table_bigdhrgl_wings[];                  /* in SDTABLES */
extern id_bit_table id_bit_table_bigh_dblbent[];                    /* in SDTABLES */
extern id_bit_table id_bit_table_3x4_h[];                           /* in SDTABLES */
extern id_bit_table id_bit_table_3x4_ctr6[];                        /* in SDTABLES */
extern id_bit_table id_bit_table_3x4_offset[];                      /* in SDTABLES */
extern id_bit_table id_bit_table_3x4_corners[];                     /* in SDTABLES */
extern id_bit_table id_bit_table_3x4_zs[];                          /* in SDTABLES */
extern id_bit_table id_bit_table_butterfly[];                       /* in SDTABLES */
extern id_bit_table id_bit_table_4x4_outer_pairs[];                 /* in SDTABLES */
extern id_bit_table id_bit_table_525_nw[];                          /* in SDTABLES */
extern id_bit_table id_bit_table_525_ne[];                          /* in SDTABLES */
extern id_bit_table id_bit_table_343_outr[];                        /* in SDTABLES */
extern id_bit_table id_bit_table_343_innr[];                        /* in SDTABLES */
extern id_bit_table id_bit_table_545_outr[];                        /* in SDTABLES */
extern id_bit_table id_bit_table_545_innr[];                        /* in SDTABLES */
extern id_bit_table id_bit_table_3dmd_in_out[];                     /* in SDTABLES */
extern id_bit_table id_bit_table_3dmd_ctr1x6[];                     /* in SDTABLES */
extern id_bit_table id_bit_table_3dmd_ctr1x4[];                     /* in SDTABLES */
extern id_bit_table id_bit_table_4dmd_cc_ee[];                      /* in SDTABLES */
extern id_bit_table id_bit_table_3ptpd[];                           /* in SDTABLES */
extern id_bit_table id_bit_table_wqtag_hollow[];                    /* in SDTABLES */
extern const id_bit_table id_bit_table_3x6_with_1x6[];              /* in SDTABLES */

extern const expand::thing s_2x2_2x4_ctrs;
extern const expand::thing s_2x2_2x4_ctrsb;
extern const expand::thing s_2x2_2x4_ends;
extern const expand::thing s_2x2_2x4_endsb;
extern const expand::thing s_1x4_1x8_ctrs;
extern const expand::thing s_1x4_1x8_ends;
extern const expand::thing s_qtg_2x4;
extern const expand::thing s_4x4_4x6a;
extern const expand::thing s_4x4_4x6b;
extern const expand::thing s_4x4_4dma;
extern const expand::thing s_4x4_4dmb;
extern const expand::thing s_23232_4x5a;
extern const expand::thing s_23232_4x5b;
extern const expand::thing s_c1phan_4x4a;
extern const expand::thing s_c1phan_4x4b;
extern const expand::thing s_1x4_dmd;
extern const expand::thing s_qtg_3x4;
extern const expand::thing s_short6_2x3;
extern const expand::thing s_bigrig_dblbone;
extern const expand::thing s_bigbone_dblrig;
extern const veryshort identity24[24];
extern full_expand::thing rear_1x2_pair;
extern full_expand::thing rear_2x2_pair;
extern full_expand::thing rear_bone_pair;
extern full_expand::thing step_8ch_pair;
extern full_expand::thing step_qtag_pair;
extern full_expand::thing step_2x2h_pair;
extern full_expand::thing step_2x2v_pair;
extern full_expand::thing step_spindle_pair;
extern full_expand::thing step_dmd_pair;
extern full_expand::thing step_tgl_pair;
extern full_expand::thing step_ptpd_pair;
extern full_expand::thing step_qtgctr_pair;

extern full_expand::thing touch_init_table1[];
extern full_expand::thing touch_init_table2[];
extern full_expand::thing touch_init_table3[];


inline uint32 get_meta_key_props(const concept_descriptor *this_concept)
{
   if (concept_table[this_concept->kind].concept_prop & CONCPROP__IS_META)
      return meta_key_props[this_concept->arg1];
   else
      return 0;
}


#define NEEDMASK(K) ((uint32) (1<<(((uint32) (K))/((uint32) CONCPROP__NEED_LOBIT))))

enum mpkind {
   MPKIND__NONE,
   MPKIND__SPLIT,
   MPKIND__SPLIT_OTHERWAY_TOO,
   MPKIND__SPLIT_WITH_45_ROTATION,
   MPKIND__SPLIT_WITH_M45_ROTATION,
   MPKIND__SPLIT_WITH_45_ROTATION_OTHERWAY_TOO,
   MPKIND__QTAG8,
   MPKIND__QTAG8_WITH_45_ROTATION,
   MPKIND__REMOVED,
   MPKIND__TWICE_REMOVED,
   MPKIND__THRICE_REMOVED,
   MPKIND__OVERLAP,
   MPKIND__OVERLAP14,
   MPKIND__OVERLAP34,
   MPKIND__SPEC_MATRIX_OVERLAP,
   MPKIND__UNDERLAP,
   MPKIND__INTLK,
   MPKIND__CONCPHAN,
   MPKIND__MAGIC,
   MPKIND__INTLKDMD,
   MPKIND__MAGICINTLKDMD,
   MPKIND__OFFS_L_ONEQ,
   MPKIND__OFFS_R_ONEQ,
   MPKIND__OFFS_L_THIRD,
   MPKIND__OFFS_R_THIRD,
   MPKIND__OFFS_L_HALF,
   MPKIND__OFFS_R_HALF,
   MPKIND__FUDGYOFFS_L_HALF,
   MPKIND__FUDGYOFFS_R_HALF,
   MPKIND__OVLOFS_L_HALF,
   MPKIND__OVLOFS_R_HALF,
   MPKIND__OFFS_L_HALF_STAGGER,
   MPKIND__OFFS_R_HALF_STAGGER,
   MPKIND__OFFS_L_THRQ,
   MPKIND__OFFS_R_THRQ,
   MPKIND__OFFS_L_FULL,
   MPKIND__OFFS_R_FULL,
   MPKIND__OFFS_BOTH_FULL,
   MPKIND__OFFS_BOTH_SINGLEV,
   MPKIND__OFFS_BOTH_SINGLEH,
   MPKIND__LILZCCW,
   MPKIND__LILZCW,
   MPKIND__LILAZCCW,
   MPKIND__LILAZCW,
   MPKIND__O_SPOTS,
   MPKIND__X_SPOTS,
   MPKIND__NS_CROSS_IN_4X4,
   MPKIND__EW_CROSS_IN_4X4,
   MPKIND__4_QUADRANTS,
   MPKIND__4_QUADRANTS_WITH_45_ROTATION,
   MPKIND__4_EDGES_FROM_4X4,
   MPKIND__4_EDGES,
   MPKIND__ALL_8,
   MPKIND__DMD_STUFF,
   MPKIND__NONISOTROPDMD,
   MPKIND__STAG,
   MPKIND__OX,
   MPKIND__DIAGQTAG,
   MPKIND__DIAGQTAG4X6,
   MPKIND__BENT0CW,
   MPKIND__BENT0CCW,
   MPKIND__BENT1CW,
   MPKIND__BENT1CCW,
   MPKIND__BENT2CW,
   MPKIND__BENT2CCW,
   MPKIND__BENT3CW,
   MPKIND__BENT3CCW,
   MPKIND__BENT4CW,
   MPKIND__BENT4CCW,
   MPKIND__BENT5CW,
   MPKIND__BENT5CCW,
   MPKIND__BENT6CW,
   MPKIND__BENT6CCW,
   MPKIND__BENT7CW,
   MPKIND__BENT7CCW,
   MPKIND__BENT8NE,
   MPKIND__BENT8SE,
   MPKIND__BENT8SW,
   MPKIND__BENT8NW,
   MPKIND__HET_SPLIT,
   MPKIND__HET_OFFS_L_HALF,
   MPKIND__HET_OFFS_R_HALF,
   MPKIND__HET_CONCPHAN,
   MPKIND__HET_ONCEREM,
   MPKIND__HET_CO_ONCEREM,   // exactly colocated
   MPKIND__HET_TWICEREM,
   MPKIND__TRIPLETRADEINWINGEDSTAR6,
   MPKIND__OFFSET_UPWARD_1,
   NUM_PLAINMAP_KINDS   // End mark; not really in the enumeration.
};


inline bool hetero_mapkind(mpkind k)
{
   return (k == MPKIND__HET_SPLIT || k == MPKIND__HET_CONCPHAN || k == MPKIND__HET_CO_ONCEREM ||
           k == MPKIND__HET_ONCEREM || k == MPKIND__HET_TWICEREM ||
           k == MPKIND__HET_OFFS_L_HALF || k == MPKIND__HET_OFFS_R_HALF);
}


// See sdtables.cpp (search for "map_thing") for extensive discussion
// of how these maps, and their code numbers, are handled.

class map {

 public:

   // We make this a struct inside the class, rather than having its
   // fields just comprise the class itself (note that there are no
   // fields in this class, and it is never instantiated) so that
   // we can make "map_init_table" be a statically initialized array.

   struct map_thing {
      veryshort maps[48];  // Darn it!  40 would be enough for all maps but one.
      setup_kind inner_kind;
      int arity;
      mpkind map_kind;
      int vert;
      warning_index warncode;
      setup_kind outer_kind;
      // Rotation, 2 bits per inner setup, in low 16 bits.
      // For the heterogeneous maps this has the secondary inner_kind in the high 8 bits.
      // The other bits have a few things:
      //   0x30000 -- add to final rotation
      //   0x40000 -- get out immediately; don't use getout map
      uint32 rot;
      uint32 per_person_rot;
      uint32 code;
      map_thing *next;
   };

   static void initialize();
   static const map_thing *get_map_from_code(uint32 map_encoding);

 private:

   // Must be a power of 2.
   enum { NUM_MAP_HASH_BUCKETS = 128 };

   static map_thing *map_hash_table2[NUM_MAP_HASH_BUCKETS];

   // The big initialization tables, in sdtables.
   static const map_thing spec_map_table[];
   static map_thing map_init_table[];
};

enum specmapkind {
   spcmap_stairst,
   spcmap_ladder,
   spcmap_but_o,
   spcmap_blocks,
   spcmap_2x4_int_pgram,
   spcmap_2x4_trapezoid,
   spcmap_trngl_box1,
   spcmap_trngl_box2,
   spcmap_inner_box,
   spcmap_lh_stag,
   spcmap_rh_stag,
   spcmap_lh_stag_1_4,
   spcmap_rh_stag_1_4,
   spcmap_lh_ox,
   spcmap_rh_ox,
   spcmap_lh_c1phana,
   spcmap_rh_c1phana,
   spcmap_lh_c1phanb,
   spcmap_rh_c1phanb,
   spcmap_d1x10,
   spcmap_tgl451,
   spcmap_tgl452,
   spcmap_dmd_1x1,
   spcmap_star_1x1,
   spcmap_qtag_f0,
   spcmap_qtag_f1,
   spcmap_qtag_f2,
   spcmap_diag2a,
   spcmap_diag2b,
   spcmap_diag23a,
   spcmap_diag23b,
   spcmap_diag23c,
   spcmap_diag23d,
   spcmap_f2x8_4x4,
   spcmap_f2x8_4x4h,
   spcmap_w4x4_4x4,
   spcmap_w4x4_4x4h,
   spcmap_f2x8_2x8,
   spcmap_f2x8_2x8h,
   spcmap_w4x4_2x8,
   spcmap_emergency1,
   spcmap_emergency2,
   spcmap_fix_triple_turnstyle,
   spcmap_2x2v,
   spcmap_ptp_magic,
   spcmap_ptp_intlk,
   spcmap_ptp_magic_intlk,
   spcmap_4x4_ns,
   spcmap_4x4_ew,
   spcmap_4x4_spec0,
   spcmap_4x4_spec1,
   spcmap_4x4_spec2,
   spcmap_4x4_spec3,
   spcmap_4x4_spec4,
   spcmap_4x4_spec5,
   spcmap_4x4v,
   spcmap_4x4_1x1,
   spcmap_trglbox3x4a,
   spcmap_trglbox3x4b,
   spcmap_trglbox3x4c,
   spcmap_trglbox3x4d,
   spcmap_trglbox4x4,
   spcmap_trglbox4x5a,
   spcmap_trglbox4x5b,
   spcmap_2x3_1234,
   spcmap_2x3_0145,
   spcmap_1x8_1x6,
   spcmap_rig_1x6,
   spcmap_qtag_2x3,
   spcmap_dbloff1,
   spcmap_dbloff2,
   spcmap_dhrgl1,
   spcmap_dhrgl2,
   spcmap_dbgbn1,
   spcmap_dbgbn2,
   spcmap_dqtag1,
   spcmap_dqtag2,
   spcmap_dqtag3,
   spcmap_dqtag4,
   spcmap_dqtag5,
   spcmap_dqtag6,
   spcmap_stw3a,
   spcmap_stw3b,
   spcmap_3ri,
   spcmap_3li,
   spcmap_3ro,
   spcmap_3lo,
   spcmap_328a,
   spcmap_328b,
   spcmap_328c,
   spcmap_328d,
   spcmap_328e,
   spcmap_328f,
   spcmap_31x3d,
   spcmap_33x1d,
   spcmap_34x6a,
   spcmap_34x6b,
   spcmap_3lqtg,
   spcmap_3spn,
   spcmap_31x6,
   spcmap_blob_1x4a,
   spcmap_blob_1x4b,
   spcmap_blob_1x4c,
   spcmap_blob_1x4d,
   spcmap_wblob_1x4a,
   spcmap_wblob_1x4b,
   spcmap_wblob_1x4c,
   spcmap_wblob_1x4d,
   spcmap_bigbone_cw,
   spcmap_bigbone_ccw,
   NUM_SPECMAP_KINDS   // End mark; not really in the enumeration.
};

// Code format for special maps is just the specmapkind number, which is clear in the left half.
// Code format for normal maps is the following, which has stuff in the left half because the
//    setupkind is nonzero:
//
//    Non-hetero normal maps (all those without "HET" in the name):
//
//     |    setupkind     |             0             |    mapkind   |    arity-1   | v |
//     ----------------------------------------------------------------------------------
//     |         9        |            10             |       8      |       3      | 1 |
//
//    Hetero normal maps:
//
//     |    setupkind     | delta R | secondary setup |    mapkind   |    arity-1   | v |
//     ----------------------------------------------------------------------------------
//     |         8        |    4    |       8         |       8      |       3      | 1 |

// Code in sdinit.cpp checks that mapkinds will fit into 8 bits and setups also into 8 bits.
// If setups require going to 9, we could probably arrange for the setups that can appear as
// secondary setups (they are always small setups) could be guaranteed to pack into 8 bits,
// with 9 bits for the primary setup field.

#define MAPCODE(setupkind,num,mapkind,vert) ( \
   (((int)(setupkind)) << 24) | (((int)(mapkind)) << 4) | (((num)-1) << 1) | (vert) )

// The heterogeneous mapcodes, maps are selected on all the usual things, plus the secondary setup kind.
// (The secondary setup kind is stored in the high 8 bits of the "rot" field.)
// The secondary setup is in the high 9 bits of the 19 bits available for the setup.  (10 bits are plenty.  I think.)
#define HETERO_MAPCODE(setupkind,num,mapkind,vert,secondary,seconddelr) ( \
   ((seconddelr) << 20) | (((int)(secondary)) << 12) | \
   (((int)(setupkind)) << 24) | (((int)(mapkind)) << 4) | (((num)-1) << 1) | (vert) )

struct clw3_thing {
   setup_kind k;
   uint32 mask;
   uint32 test;
   uint32 map_code;
   int rot;
   veryshort inactives[9];
};


/* in SDCTABLE */

extern nice_setup_info_item nice_setup_info[];

extern const concept_fixer_thing concept_fixer_table[];


extern const clw3_thing clw3_table[];                               /* in SDTABLES */


/* In SDPREDS */

extern bool selectp(const setup *ss, int place, int allow_some = 0) THROW_DECL;

/* In SDGETOUT */

extern int concepts_in_place();
extern int resolve_command_ok();
extern int nice_setup_command_ok();
void initialize_getout_tables();


/* In SDTOP */

extern void update_id_bits(setup *ss);
extern void clear_bits_for_update(setup *ss);
extern void clear_absolute_proximity_bits(setup *ss);
extern void clear_absolute_proximity_and_facing_bits(setup *ss);

// This gets a ===> BIG-ENDIAN <=== mask of people's facing directions.
// Each person occupies 2 bits in the resultant masks.  The "livemask"
// bits are both on if the person is live.
extern void big_endian_get_directions(
   const setup *ss,
   uint32 & directions,
   uint32 & livemask,
   uint32 * high_directions_p = 0,    // These are optional, for setups larger than 16 people.
   uint32 * high_livemask_p = 0);

extern void touch_or_rear_back(
   setup *scopy,
   bool did_mirror,
   int callflags1) THROW_DECL;

extern void do_matrix_expansion(
   setup *ss,
   uint32 concprops,
   bool recompute_id) THROW_DECL;

void initialize_sdlib();
void finalize_sdlib();

extern void crash_print(const char *filename, int linenum, int newtb, setup *ss) THROW_DECL;

struct skipped_concept_info {
   parse_block *m_old_retval;
   parse_block *m_skipped_concept;
   uint32 m_need_to_restrain;   // 1=(if not doing echo), 2=(yes, always)
   uint32 m_heritflag;
   parse_block *m_concept_with_root;
   parse_block *m_result_of_skip;
   parse_block **m_root_of_result_of_skip;
   uint32 m_nocmd_misc3_bits;

   skipped_concept_info() : m_nocmd_misc3_bits(0) {}
   skipped_concept_info(parse_block *incoming) THROW_DECL;    // In SDTOP
   parse_block *get_next() { return (m_heritflag != 0) ? m_concept_with_root : m_result_of_skip; }
};

extern bool check_for_concept_group(
   parse_block *parseptrcopy,
   skipped_concept_info & retstuff,
   bool want_result_root) THROW_DECL;


extern SDLIB_API const concept_descriptor *concept_descriptor_table;


extern restriction_test_result verify_restriction(
   setup *ss,
   assumption_thing tt,
   bool instantiate_phantoms,
   bool *failed_to_instantiate) THROW_DECL;

extern callarray *assoc(
   begin_kind key,
   setup *ss,
   callarray *spec,
   bool *specialpass = (bool *) 0) THROW_DECL;

uint32 uncompress_position_number(uint32 datum);

extern void clear_result_flags(setup *z, uint32 preserve_these = 0);

inline uint32 rotperson(uint32 n, int amount)
{ if (n == 0) return 0; else return (n + amount) & ~064; }

inline uint32 rotcw(uint32 n)
{ if (n == 0) return 0; else return (n + 011) & ~064; }

inline uint32 rotccw(uint32 n)
{ if (n == 0) return 0; else return (n + 033) & ~064; }


inline uint32 little_endian_live_mask(const setup *ss)
{
   int i;
   uint32 j, result;
   for (i=0, j=1, result = 0; i<=attr::slimit(ss); i++, j<<=1) {
      if (ss->people[i].id1) result |= j;
   }
   return result;
}


inline uint32 or_all_people(const setup *ss)
{
   uint32 result = 0;

   for (int i=0 ; i<=attr::slimit(ss) ; i++)
      result |= ss->people[i].id1;

   return result;
}

inline void setup::clear_people()
{
   memset(people, 0, sizeof(personrec)*MAX_PEOPLE);
}


inline void setup::clear_person(int place)
{
   people[place].id1 = 0;
   people[place].id2 = 0;
   people[place].id3 = 0;
}

inline void setup::suppress_roll(int place)
{
   if (people[place].id1)
      people[place].id1 = (people[place].id1 & (~(NROLL_MASK|OVERCAST_BIT))) | ROLL_IS_M;
}

inline void setup::suppress_all_rolls(bool just_clear_the_moved_bit)
{
   // If we can't determine the setup size, it will be -1,
   // and the loop below will take no action.
   for (int k=0; k<=attr::klimit(kind); k++) {
      if (just_clear_the_moved_bit) {
         people[k].id1 = people[k].id1 & (~(PERSON_MOVED|OVERCAST_BIT));
      }
      else {
         suppress_roll(k);
      }
   }
}

inline void setup::clear_all_overcasts()
{
   for (int k=0; k<=attr::klimit(kind); k++)
      people[k].id1 &= ~OVERCAST_BIT;
}

inline void setup::swap_people(int oneplace, int otherplace)
{
   personrec temp = people[otherplace];
   people[otherplace] = people[oneplace];
   people[oneplace] = temp;
}

inline void setup::rotate_person(int where, int rotamount)
{
   uint32 newperson = people[where].id1;
   if (newperson) people[where].id1 = (newperson + rotamount) & ~064;
}


extern uint32 copy_person(setup *resultpeople, int resultplace, const setup *sourcepeople, int sourceplace);

extern uint32 copy_rot(setup *resultpeople, int resultplace, const setup *sourcepeople, int sourceplace, int rotamount);

extern void install_person(setup *resultpeople, int resultplace, const setup *sourcepeople, int sourceplace);

extern void install_rot(setup *resultpeople, int resultplace, const setup *sourcepeople, int sourceplace, int rotamount) THROW_DECL;

extern void scatter(setup *resultpeople, const setup *sourcepeople,
                    const veryshort *resultplace, int countminus1, int rotamount) THROW_DECL;

extern void gather(setup *resultpeople, const setup *sourcepeople,
                   const veryshort *resultplace, int countminus1, int rotamount);

extern void install_scatter(setup *resultpeople, int num, const veryshort *placelist,
                            const setup *sourcepeople, int rot) THROW_DECL;

void warn_about_concept_level();

void turn_4x4_pinwheel_into_c1_phantom(setup *ss);

bool clean_up_unsymmetrical_setup(setup *ss);

setup_kind try_to_expand_dead_conc(const setup & result,
                                          const call_with_name *call,
                                          setup & lineout, setup & qtagout, setup & dmdout);

parse_block *process_final_concepts(
   parse_block *cptr,
   bool check_errors,
   final_and_herit_flags *final_concepts,
   bool forbid_unfinished_parse,
   bool only_one) THROW_DECL;

// Normally, this always returns false, and clients don't look at the result.
// But if "allow_hetero_and_notify" is on, this will return true if the given
// setups are different, rather than the normal action of throwing an error.
bool fix_n_results(
   int arity,
   int goal,
   setup z[],
   uint32 & rotstates,
   uint32 & pointclip,
   uint32 fudgystupidrot,
   bool allow_hetero_and_notify = false) THROW_DECL;

bool warnings_are_unacceptable(bool strict);

void normalize_setup(setup *ss, normalize_action action, bool noqtagcompress) THROW_DECL;

void check_concept_parse_tree(parse_block *conceptptr, bool strict) THROW_DECL;

bool check_for_centers_concept(uint32 & callflags1_to_examine,     // We rewrite this.
                               parse_block * & parse_scan,         // This too.
                               const setup_command *the_cmd) THROW_DECL;

bool do_subcall_query(
   int snumber,
   parse_block *parseptr,
   parse_block **newsearch,
   bool this_is_tagger,
   bool this_is_tagger_circcer,
   call_with_name *orig_call);

call_list_kind find_proper_call_list(setup *s);

enum collision_severity {
   collision_severity_no,
   collision_severity_controversial,
   collision_severity_ok
};

class fraction_info {
 public:
   // Normal simple constructor.
   fraction_info(int n) :
      m_reverse_order(false),
      m_instant_stop(99),  // If not 99, says to stop instantly after doing one part,
                         // and to report (in RESULTFLAG__PART_COMPLETION_BITS bit)
                         // whether that part was the last part.
      m_do_half_of_last_part((fracfrac) 0),
      m_do_last_half_of_first_part((fracfrac) 0),
      m_highlimit(n),
      m_start_point(0),
      m_end_point(n-1),
      m_fetch_index(0),
      m_client_index(0),
      m_fetch_total(n),
      m_client_total(n),
      m_subcall_incr(1)
      {}

   // Special constructor for grabbing the parts out of a call that we hope is next in the parse tree.
   // Offset is the number of parts by which we are to shorten the call from what its actual
   // definition says.  So, for example, for "finish", offset is 1.
   fraction_info(setup_command & cmd, int offset, bool allow_halves = false) :
      m_reverse_order(false),
      m_instant_stop(-99),  // Error indication; will change to +99 if OK.
      m_do_half_of_last_part((fracfrac) 0),
      m_do_last_half_of_first_part((fracfrac) 0),
      m_highlimit(0),
      m_start_point(0),
      m_end_point(0),
      m_fetch_index(0),
      m_client_index(0),
      m_fetch_total(0),
      m_client_total(0),
      m_subcall_incr(1)
      {
         fraction_command corefracs = cmd.cmd_fraction;
         corefracs.flags &= ~CMD_FRAC_THISISLAST;
         parse_block *pp = cmd.parseptr;

         if (corefracs.flags == 0 && pp) {
            while (pp->next &&
                   (concept_table[pp->concept->kind].concept_prop & (CONCPROP__USES_PARTS|CONCPROP__SECOND_CALL)) == 0)
               pp = pp->next;

            if (pp->concept->kind <= marker_end_of_list &&
                pp->call && pp->call->the_defn.schema == schema_sequential) {
               m_fetch_total = pp->call->the_defn.stuff.seq.howmanyparts-offset;
               m_highlimit = m_fetch_total;
               m_client_total = m_fetch_total;
               m_end_point = m_fetch_total-1;
               get_fraction_info(corefracs,
                                 (pp->call->the_defn.callflags1 & CFLAG1_VISIBLE_FRACTION_MASK) /
                                 CFLAG1_VISIBLE_FRACTION_BIT,
                                 weirdness_off);

               if (m_fetch_total <= 1)
                  m_instant_stop = -99;

               if (!allow_halves && (m_do_half_of_last_part != 0 || m_do_last_half_of_first_part != 0))
                  m_instant_stop = -99;
            }
         }
      }


   // This one is in sdmoves.cpp
   void get_fraction_info(fraction_command frac_stuff,
                          uint32 part_visibility_info,
                          revert_weirdness_type doing_weird_revert,
                          parse_block **restrained_concept_p = (parse_block **) 0) THROW_DECL;

   // This one is in sdmoves.cpp
   fracfrac get_fracs_for_this_part();

   // This one is in sdmoves.cpp
   bool query_instant_stop(uint32 & result_flag_wordmisc) const;

   void demand_this_part_exists()  const THROW_DECL
      {
         if (m_fetch_index >= m_fetch_total || m_fetch_index < 0)
            fail("The indicated part number doesn't exist.");
      }

   void fudge_client_total(int delta)
      {
         m_client_total += delta;
         m_highlimit = m_client_total;
         m_end_point = m_highlimit-1;
      }

   bool not_yet_in_active_section()
      {
         if (m_reverse_order) {
            if (m_client_index > m_start_point) return true;
         }
         else {
            if (m_client_index < m_start_point) return true;
         }
         return false;
      }

   bool ran_off_active_section()
      {
         if (m_reverse_order) {
            if (m_client_index < m_end_point) return true;
         }
         else {
            if (m_client_index > m_end_point) return true;
         }
         return false;
      }

   inline bool this_starts_at_beginning()
      { return
           m_start_point == 0 &&
           !m_do_last_half_of_first_part &&
           !m_reverse_order;
      }

 public:
   bool m_reverse_order;
   int m_instant_stop;
   bool m_first_call;
   fracfrac m_do_half_of_last_part;
   fracfrac m_do_last_half_of_first_part;
   int m_highlimit;
 private:
   int m_start_point;
   int m_end_point;
 public:
   int m_fetch_index;
   int m_client_index;
   int m_fetch_total;
   int m_client_total;
   int m_subcall_incr;
};


/* In SDBASIC */

class collision_collector {
public:

   // Simple constructor, takes argument saying whether collisions will be legal.
   collision_collector(collision_severity allow):
      m_allow_collisions(allow),
      m_collision_mask(0),
      m_callflags1(CFLAG1_TAKE_RIGHT_HANDS),  // Default is that collisions are legal.
      m_assume_ptr((assumption_thing *) 0),
      m_force_mirror_warn(false),
      m_doing_half_override(false),
      m_cmd_misc_flags(0),
      m_collision_appears_illegal(1),  // Halfway between "appears_illegal"
                                       // and not -- use table item.
      m_result_mask(0)
   {}

   // Glorious constructor, takes all sorts of stuff.
   collision_collector(bool mirror, setup_command *cmd, const calldefn *callspec):
      m_allow_collisions(collision_severity_ok),
      m_collision_mask(0),
      m_callflags1(callspec->callflags1),
      m_assume_ptr(&cmd->cmd_assume),
      m_force_mirror_warn(mirror),
      m_doing_half_override(false),
      m_cmd_misc_flags(cmd->cmd_misc_flags),
      m_collision_appears_illegal(0),  // May change to 2 as call progresses.
      m_result_mask(0)
      {
         // If doing half of a call, and doing it left,
         // and there is a "collision", make them come to left hands.
         if (mirror && cmd->cmd_final_flags.test_heritbit(INHERITFLAG_HALF)) {
            m_force_mirror_warn = false;
            m_doing_half_override = true;
         }
      }

   void note_prefilled_result(const setup *result)
      { m_result_mask = little_endian_live_mask(result); }

   void install_with_collision(
      setup *result, int resultplace,
      const setup *sourcepeople, int sourceplace,
      int rot) THROW_DECL;

   void fix_possible_collision(setup *result,
                               merge_action action = merge_strict_matrix,
                               uint32 callarray_flags = 0,
                               setup *ss = (setup *) 0) THROW_DECL;

private:
   const collision_severity m_allow_collisions;
   int m_collision_index;
   uint32 m_collision_mask;
   uint32 m_callflags1;
   assumption_thing *m_assume_ptr;
   bool m_force_mirror_warn;
   bool m_doing_half_override;
   uint32 m_cmd_misc_flags;
   // This is way too hairy.  In addition to its low 3 bits, the 0x100 bit is now in use,
   // meaning "if a collision occurs, give a warning that it is controversial".  It is
   // used when a collision occurs between the two groups doing an "own the anyone" operation.
   int m_collision_appears_illegal;
   uint32 m_result_mask;
};

extern void mirror_this(setup *s) THROW_DECL;

extern void do_stability(uint32 *personp,
                         int field,
                         int turning,
                         bool mirror) THROW_DECL;

extern bool check_restriction(
   setup *ss,
   assumption_thing restr,
   bool instantiate_phantoms,
   uint32 flags) THROW_DECL;

extern void basic_move(
   setup *ss,
   calldefn *the_calldefn,
   int tbonetest,
   bool fudged,
   bool mirror,
   setup *result) THROW_DECL;

/* In SDMOVES */

extern void initialize_matrix_position_tables();

extern void canonicalize_rotation(setup *result) THROW_DECL;

extern void reinstate_rotation(const setup *ss, setup *result) THROW_DECL;

extern void fix_roll_transparency_stupidly(const setup *ss, setup *result);

extern void remove_mxn_spreading(setup *ss) THROW_DECL;

extern void remove_fudgy_2x3_2x6(setup *ss) THROW_DECL;

extern void repair_fudgy_2x3_2x6(setup *ss) THROW_DECL;

extern bool do_1x3_type_expansion(setup *ss, uint32 heritflags_to_check, bool splitting) THROW_DECL;

extern bool divide_for_magic(
   setup *ss,
   uint32 heritflags_to_check,
   setup *result) THROW_DECL;

extern bool do_simple_split(
   setup *ss,
   split_command_kind split_command,
   setup *result) THROW_DECL;

extern uint32 do_call_in_series(
   setup *sss,
   bool dont_enforce_consistent_split,
   bool normalize,
   bool qtfudged) THROW_DECL;

extern void brute_force_merge(const setup *res1, const setup *res2,
                              merge_action action, setup *result) THROW_DECL;

extern void drag_someone_and_move(setup *ss, parse_block *parseptr, setup *result) THROW_DECL;

extern void anchor_someone_and_move(setup *ss, parse_block *parseptr, setup *result) THROW_DECL;

extern void process_number_insertion(uint32 mod_word);

extern bool get_real_subcall(
   parse_block *parseptr,
   const by_def_item *item,
   const setup_command *cmd_in,
   const calldefn *parent_call,
   bool forbid_flip,
   uint32 extra_heritmask_bits,
   setup_command *cmd_out) THROW_DECL;


extern int try_to_get_parts_from_parse_pointer(setup const *ss, parse_block const *pp) THROW_DECL;

bool fill_active_phantoms_and_move(setup *ss, setup *result, bool suppress_fudgy_2x3_2x6_fixup = false) THROW_DECL;

void move_perhaps_with_active_phantoms(setup *ss, setup *result, bool suppress_fudgy_2x3_2x6_fixup = false) THROW_DECL;

void impose_assumption_and_move(setup *ss, setup *result, bool suppress_fudgy_2x3_2x6_fixup = false) THROW_DECL;

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
   setup *result) THROW_DECL;

void move(
   setup *ss,
   bool qtfudged,
   setup *result,
   bool suppress_fudgy_2x3_2x6_fixup = false) THROW_DECL;

/* In SDISTORT */

extern void prepare_for_call_in_series(setup *result, setup *ss, bool dont_clear__no_reeval = false);

extern void minimize_splitting_info(setup *ss, const resultflag_rec & other_split_info);

extern void remove_z_distortion(setup *ss) THROW_DECL;

extern void remove_tgl_distortion(setup *ss) THROW_DECL;

struct whuzzisthingy {
   concept_kind k;
   int rot;
};

extern void divided_setup_move(
   setup *ss,
   uint32 map_encoding,
   phantest_kind phancontrol,
   bool recompute_id,
   setup *result,
   unsigned int noexpand_bits_to_set = CMD_MISC__NO_EXPAND_1 | CMD_MISC__NO_EXPAND_2,
   whuzzisthingy *thing = (whuzzisthingy *) 0) THROW_DECL;

extern void overlapped_setup_move(
   setup *ss,
   uint32 map_encoding,
   uint32 *masks,
   setup *result,
   unsigned int noexpand_bits_to_set = CMD_MISC__NO_EXPAND_1 | CMD_MISC__NO_EXPAND_2) THROW_DECL;

extern void do_phantom_2x4_concept(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL;

extern void do_phantom_stag_qtg_concept(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL;

extern void do_phantom_diag_qtg_concept(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL;

extern void distorted_2x2s_move(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL;

extern void distorted_move(
   setup *ss,
   parse_block *parseptr,
   disttest_kind disttest,
   uint32 keys,
   setup *result) THROW_DECL;

extern void triple_twin_move(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL;

extern void do_concept_rigger(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL;

extern void do_concept_wing(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL;

extern void initialize_commonspot_tables();

extern void common_spot_move(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL;

extern void do_glorious_triangles(
   setup *ss,
   const tglmap::tglmapkey *map_ptr_table,
   int indicator,
   setup *result) THROW_DECL;

extern void triangle_move(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL;

/* In SDCONCPT */

extern bool do_big_concept(
   setup *ss,
   parse_block *the_concept_parse_block,
   bool handle_concept_details,
   setup *result) THROW_DECL;

void nose_move(
   setup *ss,
   bool everyone,
   selector_kind who,
   direction_kind where,
   setup *result) THROW_DECL;

void stable_move(
   setup *ss,
   bool fractional,
   bool everyone,
   int howfar,
   selector_kind who,
   setup *result) THROW_DECL;

/* In SDTAND */

extern void initialize_tandem_tables();

extern void tandem_couples_move(
   setup *ss,
   selector_kind selector,
   int twosome,           // solid=0 / twosome=1 / solid-to-twosome=2 / twosome-to-solid=3
   int fraction_fields,   // number fields, if doing fractional twosome/solid
   int phantom,           // normal=0 phantom=1 general-gruesome=2 gruesome-with-wave-check=3
   tandem_key key,
   uint32 mxn_bits,
   bool phantom_pairing_ok,
   setup *result) THROW_DECL;

extern void mimic_move(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL;

extern bool process_brute_force_mxn(
   setup *ss,
   parse_block *parseptr,
   void (*backstop)(setup *, parse_block *, setup *),
   setup *result) THROW_DECL;

/* In SDCONC */

extern void concentric_move(
   setup *ss,
   setup_command *cmdin,
   setup_command *cmdout,
   calldef_schema analyzer,
   uint32 modifiersin1,
   uint32 modifiersout1,
   bool recompute_id,
   bool enable_3x1_warn,
   uint32 specialoffsetmapcode,
   setup *result) THROW_DECL;

extern resultflag_rec get_multiple_parallel_resultflags(setup outer_inners[], int number) THROW_DECL;

extern void initialize_fix_tables();

extern void normalize_concentric(
   const setup *ss,        // The "dyp_squash" schemata need to see this; it's allowed to be null.
   calldef_schema synthesizer,
   int center_arity,
   setup outer_inners[],   // Outers in position 0, inners follow.
   int outer_elongation,
   uint32 matrix_concept,
   setup *result) THROW_DECL;

extern void on_your_own_move(
   setup *ss,
   parse_block *parseptr,
   setup *result) THROW_DECL;

extern void punt_centers_use_concept(setup *ss, setup *result) THROW_DECL;

extern void selective_move(
   setup *ss,
   parse_block *parseptr,
   selective_key indicator,
   int others,  // -1 - only selectees do the call, others can still roll
                //  0 - only selectees do the call, others can't roll
                //  1 - both sets
   uint32 arg2,
   uint32 override_selector,
   selector_kind selector_to_use,
   bool concentric_rules,
   setup *result) THROW_DECL;

extern void inner_selective_move(
   setup *ss,
   setup_command *cmd1,
   setup_command *cmd2,
   selective_key indicator,
   int others,  // -1 - only selectees do the call, others can still roll
                //  0 - only selectees do the call, others can't roll
                //  1 - both sets
   uint32 arg2,
   bool demand_both_setups_live,
   uint32 override_selector,
   selector_kind selector_to_use,
   uint32 modsa1,
   uint32 modsb1,
   setup *result) THROW_DECL;

/* In SDUTIL */

extern void FuckingThingToTryToKeepTheFuckingStupidMicrosoftCompilerFromScrewingUp();

SDLIB_API extern parse_block *copy_parse_tree(parse_block *original_tree);
SDLIB_API extern void reset_parse_tree(parse_block *original_tree, parse_block *final_head);
SDLIB_API void randomize_couple_colors();
SDLIB_API void string_copy(char **dest, Cstring src);
SDLIB_API extern void initialize_parse();
SDLIB_API extern uint32 translate_selector_fields(parse_block *xx, uint32 mask);
SDLIB_API extern bool fix_up_call_for_fidelity_test(const setup *old, const setup *nuu, uint32 &global_status);

/* In SDPICK */

bool in_exhaustive_search();
void reset_internal_iterators();
selector_kind do_selector_iteration(bool allow_iteration);
direction_kind do_direction_iteration();
void do_number_iteration(int howmanynumbers,
                         uint32 odd_number_only,
                         bool allow_iteration,
                         uint32 *number_list);
bool do_tagger_iteration(uint32 tagclass,
                         uint32 *tagg,
                         uint32 numtaggers,
                         call_with_name **tagtable);
void do_circcer_iteration(uint32 *circcp);
const concept_descriptor *pick_concept(bool already_have_concept_in_place);
call_with_name *do_pick();
resolve_goodness_test get_resolve_goodness_info();
bool pick_allow_multiple_items();
void start_pick();
void end_pick();
bool forbid_call_with_mandatory_subcall();
bool allow_random_subcall_pick();
