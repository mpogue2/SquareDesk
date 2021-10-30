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
   general_initialize
   generate_random_number
   hash_nonrandom_number
   get_mem
   get_date
   open_file
   write_file
   close_file
   read_from_abridge_file
   write_to_abridge_file
   close_abridge_file

and the following external variables:

   session_index
   rewrite_with_new_style_filename
   random_number
   database_filename
   new_outfile_string
   abridge_filename
*/

// We used to have the following obsolete comment relating to the problems
// with stdlib.h.  See also the editorializing in mkcalls.cpp.

// Of course, we no longer take pity on troglodyte development environments.

// /* You should compile this file (and might as well compile all the others
//    too) with some indicator symbol defined that tells what language system
//    semantics are to be provided.  This program requires at least POSIX.
//    A better random number generator mechanism (48 bits) is provided under
//    System 5 (_SYS5_SOURCE), OSF (_AES_SOURCE), XOPEN (_XOPEN_SOURCE), or
//    some proprietary systems, so you should define such a symbol if your
//    system provides those semantics.  If not, you should just turn on
//    _POSIX_SOURCE.  Normally, this is done on the compiler invocation line
//    (in the Makefile, or whatever) with some incantation like "-D_AES_SOURCE".
//
//    We recommend _AES_SOURCE over _XOPEN_SOURCE, since the latter doesn't
//    seem to recognize that 20th century programmers use include files with
//    prototypes for library functions in them.  Under OSF/AES, the file
//    stdlib.h has the necessary stuff.  Under XOPEN, you are expected to
//    copy the prototypes by hand from the printed manual into your program.
//    For those who absolutely must use XOPEN, we have done the necessary
//    copying below.
//
//    On HP-UX, we recommend turning on _AES_SOURCE for the reason given
//    above.  You could also turn on _HPUX_SOURCE to get the routines,
//    if you have no aversion to proprietary system semantics.
//
//    On SUN-OS, giving no symbol will get SUN-OS semantics, which gets the
//    48-bit routines, if you have no aversion to proprietary system semantics.
//    This is what the "defined(sun)" is for.
// */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "sd.h"
#include "paths.h"


/* These variables are external. */


/* Used for controlling the session file.  When index is nonzero,
   the session file is in use, and the final state should be written back
   to it at that line. */

int session_index = 0;        // If this is nonzero, we have opened a session.
bool rewrite_with_new_style_filename = false;   // User gave "change to new file format".

/* e1 = page up
   e2 = page down
   e3 = end
   e4 = home
   e5 = left arrow
   e6 = up arrow
   e7 = right arrow
   e8 = down arrow
   e13 = insert
   e14 = delete */


int random_number;
const char *database_filename = DATABASE_FILENAME;
const char *new_outfile_string = (char *) 0;
char abridge_filename[MAX_TEXT_LINE_LENGTH];

static bool file_error;
static char full_outfile_name[MAX_FILENAME_LENGTH+1];
static FILE *fildes;
static char fail_errstring[MAX_ERR_LENGTH];
static char fail_message[MAX_ERR_LENGTH];


extern void general_initialize()
{
   // If doing a resolve test, use a deterministic seed.
   // But make it depend on something the operator said.
   // That way, one can run two usefully independent tests
   // on a multiprocessor, by giving them slightly
   // different timeouts.
   unsigned int seed = (ui_options.resolve_test_random_seed != 0) ?
      ui_options.resolve_test_random_seed : (uint32_t) time((time_t *)0);
   srand(seed);
   random_count = 0;
}


extern int generate_random_number(int modulus)
{
   random_number = (int) rand();
   random_recent_history[(random_count++) & 127] = random_number;
   return random_number % modulus;
}


extern void hash_nonrandom_number(int number)
{
   hashed_randoms = hashed_randoms*1049633+number;
}


//   // This is obsolete; we now use the built-in memory management operations
//   // of the C++ language, of course.  It is left here as a memorial to the types
//   // of things people had to do when dealing with buggy compilers or libraries.
//
//   extern void *get_mem(uint32_t siz)
//   {
//      // Using "calloc" instead of "malloc" clears the memory.
//      // We claim this isn't necessary; our code, being correctly written,
//      // is insensitive to the initial contents of allocated memory.
//      // But someone is a wuss, and thinks we need this.
//      void *buf = calloc(siz, 1);
//
//      if (!buf && siz != 0) {
//         char msg [50];
//         sprintf(msg, "Can't allocate %d bytes of memory.", (int) siz);
//         gg->fatal_error_exit(2, msg);
//      }
//
//      return buf;
//   }
//
//
//   // An older version of this actually called "malloc" or "realloc"
//   // depending on whether the incoming pointer was nil.  There was a
//   // comment pointing out that this was because "SunOS 4 realloc doesn't
//   // handle NULL".  Isn't that funny?
//
//   // Of course, we no longer take pity on broken compilers or operating systems.
//
//   extern void *get_more_mem(void *oldp, uint32_t siz)
//   {
//      void *buf;
//
//      buf = realloc(oldp, siz);
//      if (!buf && siz != 0) {
//         char msg [50];
//         sprintf(msg, "Can't allocate %d bytes of memory.", (int) siz);
//         gg->fatal_error_exit(2, msg);
//      }
//      return buf;
//   }



void ui_utils::get_date(char dest[])
{
   time_t clocktime;
   char *junk;
   char *dstptr;

   time(&clocktime);
   junk = ctime(&clocktime);
   dstptr = dest;
   string_copy(&dstptr, junk);
   if (dstptr[-1] == '\n') dstptr[-1] = '\0';         /* Stupid UNIX! */
}

// There used to be some code here to work around non-compliance of the Sun compiler/runtime.
// Of course, we no longer take pity on broken compilers or operating systems.
//
// #if defined(sun)
//    extern int sys_nerr;
//    extern char *sys_errlist[];
//
//    if (errno < sys_nerr) return sys_errlist[errno];
//    else return "?unknown error?";

char *get_errstring()
{
   return strerror(errno);
}


void ui_utils::open_file()
{
   int this_file_position;
   int i;

   strncpy(full_outfile_name, outfile_prefix, MAX_FILENAME_LENGTH);
   strncat(full_outfile_name, outfile_string, MAX_FILENAME_LENGTH);

   file_error = false;

   // We need to find out whether there are garbage characters (e.g. ^Z)
   // near the end of the existing file, and remove same.  Such things
   // have been known to be placed in files by some programs running on PC's.
   //
   // Furthermore, some PC print software stops printing when it encounters
   // one, so we have to get rid of it.

   // On a number of operating systems, things are fairly simple.  We open the file in
   // "append" mode, thankful that we have escaped one of the most monumentally stupid
   // pieces of OS design ever to plague the universe, and only need to
   // deal with Un*x, which is merely one of the most monumentally stupid
   // pieces of OS design ever to plague this galaxy.  Or maybe we're running
   // on Windows or NT, operating systems that have sometimes been observed to do
   // the right thing.

   // Actually, we no longer have any idea whether Cygnus/Cygwin fits into this
   // category or the next.  The Cygwin compiler, for all its being touted as
   // a working implementation of the GNU tools on Windows, crashes ignominiously:
   //
   // C:\wba\sd>bash
   // bash-2.05a$ make
   // gcc -O2 -Wall -Wno-switch -Wno-uninitialized -Wno-char-subscripts -c sdmain.cpp
   // make: *** [sdmain.o] Aborted (core dumped)
   //
   // The Cygnus compiler generates a program, that then silently disappears while
   // reading in the database.
   //
   // I simply have no patience with broken software.

   // The following code used to be under a complicated "#if" to specialize things for the compiler
   // and run-time system.  This is now used for all supported operating systems.  The other code
   // is obsolete.

   struct stat statbuf;

   if (stat(full_outfile_name, &statbuf))
      this_file_position = 0;   // File doesn't exist.
   else
      this_file_position = statbuf.st_size;

   // This is still a little tricky.  We want a file mode that:
   // (1) creates a new file if none exists,
   // (2) allows us to read, so we can look around for control-Z,
   // (3) allows us to write at an arbitrary position, that is,
   //     not at the end of the file, so we can seek to just before
   //     the existing control-Z and then start writing at that point.
   // There is no mode that does all of these things.
   //
   // So we first open it in "create/append" mode.  This would
   // allow reading or writing, but would always write at the end
   // of the file -- seeks do not affect the write position.
   // But it will create the file if it doesn't exist.

   if (!(fildes = fopen(full_outfile_name, "a"))) {
      strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
      strncpy(fail_message, "open", MAX_ERR_LENGTH);
      file_error = true;
      return;
   }

   if ((last_file_position != -1) && (last_file_position != this_file_position)) {
      writestuff("Warning -- file has been modified since last sequence.");
      newline();
      newline();
   }

   // Now that we know that the file exists, open it in read-write mode.
   // This will let us seek to the spot where we wish to write.

   // On Windows NT and above, opening in "r+" mode will also erase the control-Z for
   // us.  (Well, more precisely, they don't use control-Z's any more.)  They finally
   // did something right!  (Well, they compensated for something extremely stupid.)
   // But we go through the control-Z search anyway, just in case.  Observation made on
   // Windows 2000, don't know about other versions.

   fclose(fildes);

   if (!(fildes = fopen(full_outfile_name, "r+"))) {
      strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
      strncpy(fail_message, "open", MAX_ERR_LENGTH);
      file_error = true;
      return;
   }

   if (this_file_position != 0) {
      // The file exists, and we have opened it in "r+" mode.  Look at its end.

      if (fseek(fildes, -4, SEEK_END)) {
         // It isn't 4 characters long -- forget it.  But first, position at the end.
         if (fseek(fildes, 0, SEEK_END)) {
            fclose(fildes);     // What happened?????
            strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
            strncpy(fail_message, "seek", MAX_ERR_LENGTH);
            file_error = true;
            return;
         }
         goto really_just_append;
      }

      // We are now 4 before the end.  Look at those last 4 characters.

      for (i=0 ; i<4 ; i++) {
         if (fgetc(fildes) == 0x1A) {
            writestuff("Warning -- file contains spurious control character -- removing same.");
            newline();
            newline();
            last_file_position = -1;   // Suppress the other error.
            break;
         }
      }

      // Now seek to the end, or to the point just before the offending character.

      if (fseek(fildes, i-4, SEEK_END)) {
         fclose(fildes);     // What happened?????
         strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
         strncpy(fail_message, "seek", MAX_ERR_LENGTH);
         file_error = true;
         return;
      }

      goto really_just_append;
   }

   // Here is the obsolete code for systems that have long since been deprecated.

   // The real code continues after this long flaming comment.  This dates from early
   // versions of the program, which were compiled with DJGPP and ran on Windows 3.1.
   // As you will see, getting file manipulation to work was rather frustrating back
   // then.

   // This comment written in 2002.
   // We are in big trouble.  This is legacy code.  It appears to have
   // been required when running on DOS.  Or maybe Windows 3.1 or something.
   // But no modern systems.  Well, actually, DJGPP requires this,
   // because the previous code, opening the files in text mode,
   // doesn't do the right thing with DJGPP.  I could change the previous
   // code to do things in binary mode (the way they did prior to August
   // 2002) and then it would work with DJGPP, but I won't.  These are
   // text files.  If DJGPP doesn't understand that, that's too bad.
   //
   // It was a lot of fun figuring out how to do this,
   // as you can tell from the comments below.
   //
   // Following comments written in early 1990's, most likely.
   //
   // /* Wait!  The OS is so convinced that it knows better than we what
   //    should be in a file, that, in addition to silently putting in this
   //    ^Z character and making the print software silently ignore everything
   //    in the file that occurs after it, IT MAKES IT INVISIBLE TO US!!!!!
   //    WE CAN'T EVEN SEE THE %$%^#%^@&*$%^#!@ CONTROL Z!!!!!!
   //    That is, the system won't let us see it if we open the file in
   //    the usual "text" mode.  It knows we couldn't possibly be interested
   //    in a character whose meaning is so trivial that it does nothing
   //    more than make it impossible to print a file.
   //
   //    So we thank our lucky stars that the system is watching out for our
   //    interests in this way and making life simple and convenient for us,
   //    and we open the file in "binary" mode.
   //
   //    The difference between "text" and "binary" mode for an open file is
   //    sometimes obscure.  It's good to know that we have discovered its
   //    significance on this system.
   //
   //    So we open in "rb+" mode, and look around for ^Z characters. */
   //
   // /* But first, it is an observed fact that, if we open a file in binary
   //    mode, and the file gets created because of that, some garbage header
   //    bytes get written to it.  So, we don't open it in binary mode until
   //    we have determined that it already exists.  I'm not making this up,
   //    you know.  It really behaves that badly. */
   //
   // if (!(fildes = fopen(outfile_string, "r+"))) {
   //
   //    /* Failed.  Maybe the file doesn't exist, in which case we open it
   //       in append mode (which will create it if necessary) and don't worry
   //       about the garbage characters. */
   //
   //    if (!(fildes = fopen(outfile_string, "a"))) {
   //       strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
   //       strncpy(fail_message, "open", MAX_ERR_LENGTH);
   //       file_error = true;
   //       return;
   //    }
   //
   //    this_file_position = ftell(fildes);
   //
   //    if ((last_file_position != -1) && (last_file_position != this_file_position)) {
   //       writestuff("Warning -- file has been modified since last sequence.");
   //       newline();
   //       newline();
   //    }
   //
   //    /* We are positioned at the end, because that's what "a" mode does. */
   //    goto really_just_append;
   // }
   //
   // /* Now that we know that the file exists, open it in binary mode. */
   //
   // fclose(fildes);
   //
   // if (!(fildes = fopen(outfile_string, "rb+"))) {
   //    strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
   //    strncpy(fail_message, "open", MAX_ERR_LENGTH);
   //    file_error = true;
   //    return;
   // }
   //
   // /* The file exists, and we have opened it in "rb+" mode.  Look at its end. */
   //
   // if (fseek(fildes, -4, SEEK_END)) {
   //    /* It isn't 4 characters long -- forget it.  But first, position at the end. */
   //    if (fseek(fildes, 0, SEEK_END)) {
   //       fclose(fildes);     /* What happened????? */
   //       strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
   //       strncpy(fail_message, "seek", MAX_ERR_LENGTH);
   //       file_error = true;
   //       return;
   //    }
   //    goto just_append;
   // }
   //
   // /* We are now 4 before the end.  Look at those last 4 characters. */
   //
   // for (i=0 ; i<4 ; i++) {
   //    if (fgetc(fildes) == 0x1A) {
   //       writestuff("Warning -- file contains spurious control character -- removing same.");
   //       newline();
   //       newline();
   //       last_file_position = -1;   /* Suppress the other error. */
   //       break;
   //    }
   // }
   //
   // /* Now seek to the end, or to the point just before the offending character. */
   //
   // if (fseek(fildes, i-4, SEEK_END)) {
   //    fclose(fildes);     /* What happened????? */
   //    strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
   //    strncpy(fail_message, "seek", MAX_ERR_LENGTH);
   //    file_error = true;
   //    return;
   // }
   //
   // just_append:
   //
   // this_file_position = ftell(fildes);
   //
   // if ((last_file_position != -1) && (last_file_position != this_file_position)) {
   //    writestuff("Warning -- file has been modified since last sequence.");
   //    newline();
   //    newline();
   // }
   //
   // /* But wait!!!!  There's more!!!!  On a PC, we can't write to the stream
   //    if it was opened in "binary" mode!  Don't ask me why, the standards
   //    documents clearly say that it is legal.  It is simply an observed
   //    fact that it doesn't work.
   //
   //    So we remember our seek position, close the file, reopen it in "text"
   //    mode (that is, "r+"), and seek back to that spot. */
   //
   // fclose(fildes);
   // if (!(fildes = fopen(outfile_string, "r+"))) {
   //    strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
   //    strncpy(fail_message, "open", MAX_ERR_LENGTH);
   //    file_error = true;
   //    return;
   // }
   //
   // if (fseek(fildes, i-4, SEEK_END)) {
   //    fclose(fildes);     /* What happened????? */
   //    strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
   //    strncpy(fail_message, "seek", MAX_ERR_LENGTH);
   //    file_error = true;
   //    return;
   // }
   //
   // /* One remaining question.  Will the system allow us to seek in a
   //    text file to a point that we determined while it was opened in
   //    binary?  Will it figure out some way to prevent us from backing up
   //    over that ^Z?  Will it write a ^Z after the seek point?  Will it
   //    find some creative way to screw us?
   //
   //    No.  It actually seems to work.  Aren't computers wonderful? */

   really_just_append:

   if (this_file_position == 0) {
      writestuff("File does not exist, creating it.");
      newline();
      newline();
   }
   else {
      if (last_file_position == -1) {
         writestuff("Appending to existing file.");
         newline();
         newline();
      }

      if ((fwrite("\f", 1, 1, fildes) != 1) || ferror(fildes)) {
         strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
         strncpy(fail_message, "write formfeed", MAX_ERR_LENGTH);
         file_error = true;
         return;
      }
   }
}


// More flaming, relating this time to Cygwin.  There was an #if selecting this code.
// We no longer take pity on broken compilers or operating systems.

// #if defined(__CYGWIN__)
//    // Cygwin seems to want us to write 2-character ("\r\n") line breaks.
//    // They are being such good doobies about the fact that they are running
//    // on Windows instead of Unix, and that Windows uses the "\r\n" convention
//    // for its native file format, that they seem to have lost sight of the
//    // fact that, when a POSIX file is being written in text mode, one
//    // *ALWAYS* uses the 1-character ("\n") convention in the fwrite calls.
//    // The library will take care of converting to native format.
//    // Actually, as noted above, Cygwin is broken, and I have no idea
//    // how to write line breaks.  But I'm leaving this code in.
//    //
//    char nl[] = "\r\n";
// #define NLSIZE 2
// etc.....

void ui_utils::write_file(const char line[])
{
   if (file_error) return;    // Don't keep trying after a failure.

   uint32_t size = strlen(line);

   if (size != 0) {
      if ((fwrite(line, 1, size, fildes) != size) || ferror(fildes)) {
         strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
         strncpy(fail_message, "write", MAX_ERR_LENGTH);
         file_error = true;      // Indicate that the sequence will not get written.
         return;
      }
   }

   if ((fwrite("\n", 1, 1, fildes) != 1) || ferror(fildes)) {
      strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
      strncpy(fail_message, "write", MAX_ERR_LENGTH);
      file_error = true;      // Indicate that the sequence will not get written.
   }
}


void ui_utils::close_file()
{
   struct stat statbuf;
   char foo[MAX_ERR_LENGTH*10];

   if (file_error) goto fail;

   if (fclose(fildes)) goto error;

   if (stat(full_outfile_name, &statbuf))
      goto error;

   last_file_position = statbuf.st_size;

   return;

 error:

   strncpy(fail_errstring, get_errstring(), MAX_ERR_LENGTH);
   strncpy(fail_message, "close", MAX_ERR_LENGTH);

 fail:

   strncpy(foo, "WARNING!!!  Sequence has not been written!  File ", MAX_ERR_LENGTH);
   strncat(foo, fail_message, MAX_ERR_LENGTH);
   strncat(foo, " failure on \"", MAX_ERR_LENGTH);
   strncat(foo, full_outfile_name, MAX_ERR_LENGTH);
   strncat(foo, "\": ", MAX_ERR_LENGTH);
   strncat(foo, fail_errstring, MAX_ERR_LENGTH);
   strncat(foo, " -- try \"change output file\" or \"change output prefix\" operation.", MAX_ERR_LENGTH);
   specialfail(foo);
}
