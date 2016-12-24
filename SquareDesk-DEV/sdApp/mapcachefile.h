#ifndef MAPPED_CACHE_FILE_H
#define MAPPED_CACHE_FILE_H

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2004  William B. Ackerman.
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
//    This is for version 36.


// This implements the handling of a pair of files, typically a
// "source" text file containing, say, a dictionary, and a "cache"
// file containing the result of pre-processing that dictionary.  The
// intention is that a program normally wants to use the precomputed
// information in the cache file, so that it can run quickly in the
// normal case.  But, whenever the source file changes, it recomputes
// the cached information, stores it in the cache file for future
// runs, and proceeds.  This simplifies the management of such pairs
// of files, and obviates the user inconvenience of having two
// programs, one for preparing the precomputed file and another for
// using it.

// The cache file might be organized so that only sparse references to
// it are required.  These procedures use memory mapping, if supported
// by the operating system, so that the entire cache file does not
// necessarily need to be read.

// These procedures use a client-supplied version number, the source
// file length, the source file modification time, and a measurement
// of the system's endian-ness to determine whether the source file is
// stale.  Those items are stored in the first few words of the cache
// file, and are invisible to you, the client.  The version number is
// normally a constant in your program.  Whenever you change your
// program so that the format of the information in the cache file
// might be incompatible, increase the version number.  This will
// cause all extant cache files to be considered stale.  A user may
// also, of course, delete cache files at any time.

// The constructor takes the number of source files (typically 1), the
// array of source file names, the array of POSIX file descriptors to
// hold the opened source files, the extension to be used for the
// cache file, and your version number.  A typical call might look
// like:
//
//     char *sourcenames[1] = {"dict87.txt"};
//     FILE *sourcefiles[1];
//     MAPPED_CACHE_FILE the_cache(1, sourcenames, sourcefiles, "frequencycount", 13);
//
// This would use the single file "dict87.txt" as the source file, and
// use the cache file "dict87.frequencycount".
//
// To use multiple source files, do this:
//
//     char *sourcenames[3] = {"alpha.txt", "beta.stats", "gamma.data"};
//     FILE *sourcefiles[3];
//     MAPPED_CACHE_FILE the_cache(3, sourcenames, sourcefiles, "xyz", 13);

// This would use the given source files, and use the cache file
// "alpha+beta+gamma.xyz".  We strip off all the source file suffixes,
// append the names by means of plus signs, and append the given suffix,
// to make the cache file name.  You would be well advised not to give
// your source files names that make this process ambiguous.

// In this example, the version number is 13.  You should change that
// whenever you modify the program in a way that changes the format
// of the cache file.  Doing so will cause all extant cache files to
// become obsolete.

// The constructor can take an optional 6th argument, which is an
// array of booleans, the same number as the number of source files.
// Each boolean in the array tells whether the corresponding source
// file is to be opened in binary mode.  If false, that file is opened
// in text mode.  If the argument array is not given, all files are
// opened in text mode.

// The constructor opens the source files as a plain POSIX files, in
// read-only mode.  They are opened in binary mode if the final
// optional argument array says to do so; otherwise they are in text
// mode.  It fills the array that was provided as the 3rd argument.
// The cache file will also be open and mapped if possible, perhaps in
// a read-only memory segment.  The source files will not have had
// anything read from them, so they will still be at the beginning of
// the file.

// The address at which the cache file is mapped can be obtained by
// calling the method "map_address()".  It will be zero if mapping
// was unsuccessful for any reason.

// After constructing a cache object, examine the map address first,
// by making a call to "map_address()".  If it is nonzero, we found
// and opened the cache file and all the source files, and found that
// the cache file matched your given version number and the size and
// creation times of the source files.  The contents of the cache file
// (other than our header) are mapped at the address that we returned.
// It will be word-aligned.  (We consider a "word" to be 32 bits.)
// The source files are also open.

// At this point you may elect to look at the mapped memory and do
// further validation of its contents, perhaps reading the source
// files.  Use the "srcfiles" array for this.  If you find things to
// your liking, you may use the mapped data.  You should close the
// source files at some point.

// If the address returned by "map_address()" is zero, or if you don't
// like what you see in the mapped memory, you need to recompute and
// rewrite the cache file.  First, check the "srcfiles" array.  If any
// item is zero, we couldn't even open that source file.  You should
// presumably give some error message to that effect.  Also, you
// should look to see whether any other source file descriptors are
// nonzero.  If so, you should close them.  Since the source files
// couldn't be opened, you presumably can't proceed.

// If the address was zero and all the source files were successfully
// opened, as indicated by the "srcfiles" array elements all being
// nonzero, use those files to compute the new cached data.  Figure
// out how big that data will be, in bytes, and pass it as the
// argument to "map_for_writing".  A subsequent call to
// "map_address()" will be the area into which you should write the
// indicated number of bytes of data.  It will be word-aligned, and,
// of course, writeable.  It may be different from the former mapping
// address.  That former address will no longer be valid.  You must
// use the new address from a new call to "map_address()" made after
// calling "map_for_writing".

// After you have finished with the source files, you should close
// them.  At this point you may use the new mapped memory area into
// which you have written the cached data, exactly as you would have
// used it if you had not rewritten the cache file.  Don't modify it
// further, as the cache file may not yet have been written.

// The function "get_miss_reason" will tell why a construction failed.
// It is intended for debugging purposes.  In general, you don't need
// to use it--rewriting the cache when "map_address()" returns zero,
// as described above, is all you need.

// The object destructor will remove the mapping, and write and close
// the cache file.  The created cache file will be the size indicated
// in the call to "map_for_writing", plus 20 bytes or so for our
// header.  The destructor does *not* close the source files.  You
// should have closed them when you were finished reading them, or
// when you realized that you wouldn't need them.

// On some operating systems, the cache file might not actually be
// written until the destructor is called.

// The information that we use for validating the cache file is the
// version number (provided by you, the client) along with the
// "st_size" and "st_mtime" attributes of the source file.  Whether
// those attributes are portable from one operating system to another
// is questionable.  There may also be endian-ness or word size
// problems that we do not catch, although we try to check that.  We
// do not recommend moving cache files from one system to another.
// When in doubt, delete the cache files.  In fact, we recommend making
// your program have an option to ignore any existing cache file and
// consider it to be stale.  To do that, simply act as though the value
// returned by "map_address()" after object construction had been zero.

// The Windows and Linux versions of this use the file mapping system
// operations, and should be very efficient.

#include <stdio.h>

struct MAPPED_CACHE_INNARDS;

class MAPPED_CACHE_FILE {
   int *client_address;
   MAPPED_CACHE_INNARDS *innards;
 public:

   enum miss_reason {
      NO_MISS,
      MISS_CANT_OPEN_CACHE,
      MISS_WRONG_SOURCE_FILE_SIZE,
      MISS_WRONG_SOURCE_FILE_TIME,
      MISS_WRONG_CLIENT_VERSION,
      MISS_WRONG_ENDIAN,
      MISS_CANT_OPEN_SOURCE,
      MISS_CANT_GET_SOURCE_STATUS
   };

   MAPPED_CACHE_FILE(int numsourcefiles,
                     const char * const * srcnames,
                     FILE **srcfiles,
                     const char *mapext,
                     int clientversion,
                     const bool *srcbinary = 0);
   ~MAPPED_CACHE_FILE();
   inline int *map_address() { return client_address; }
   void map_for_writing(int clientmapfilesizeinbytes);
   miss_reason get_miss_reason();
};

#endif
