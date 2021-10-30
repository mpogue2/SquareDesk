/*
 * sdui-ttu.cpp - helper functions for sdui-tty interface to use the Unix
 * "curses" mechanism.
 * Time-stamp: <93/11/27 11:06:39 gildea>
 * Copyright 1993 Stephen Gildea
 * Copyright 2001 Chris "Fen" Tamanaha	<tamanaha@coyotesys.com>
 *
 * Permission to use, copy, modify, and distribute this software for
 * any purpose is hereby granted without fee, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * The authors make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 * By Stephen Gildea <gildea@lcs.mit.edu> January 1993
 * Certain editorial comments by William Ackerman <wba@apollo.com> July 1993
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <termios.h>   // We use this stuff if "-no_cursor" was specified.
#include <unistd.h>    //    This too.

#if !defined(NO_IOCTL) && (defined(__linux__) || defined(__CYGWIN__) || defined(__APPLE__))
#include <sys/ioctl.h>
#endif

#include <signal.h>
#include <string.h>
#include "sd.h"



static int current_tty_mode = 0;

struct colorspec {
   int curses_index;
   Cstring vt100_string;
};

static colorspec color_translations[8] = {
   {0, "00"},  // 0 - not used
   {0, "30"},  // 1 - substitute yellow
   {2, "31"},  // 2 - red
   {3, "32"},  // 3 - green
   {4, "33"},  // 4 - yellow
   {5, "34"},  // 5 - blue
   {6, "35"},  // 6 - magenta
   {7, "36"}}; // 7 - cyan

static void csetmode(int mode) // 1 means raw, no echo, one character at a time; 0 means normal.
{
    static cc_t orig_eof = '\004';
    struct termios term;

    if (mode == current_tty_mode) return;

    int fd = fileno(stdin);

    tcgetattr(fd, &term);
    if (mode == 1) {
       orig_eof = term.c_cc[VEOF]; // VMIN may clobber
       term.c_cc[VMIN] = 1;        // 1 char at a time
       term.c_cc[VTIME] = 0;       // no time limit on input
       term.c_lflag &= ~(ICANON|ECHO);
    }
    else {
       term.c_cc[VEOF] = orig_eof;
       term.c_lflag |= ICANON|ECHO;
    }

    tcsetattr(fd, TCSADRAIN, &term);
    current_tty_mode = mode;
}


static void term_handler(int n)
{
   if ( n == SIGQUIT ) {
      abort();
   }
   else {
      session_index = 0;  // Prevent attempts to update session file.
      general_final_exit(n);
   }
}

extern void ttu_set_window_title(const char *string) {}
void iofull::set_pick_string(const char *string) {}


void iofull::display_help()
{
   printf("-lines <n>                  assume this many lines on the screen\n");
   printf("-no_line_delete             do not use the \"line delete\" function for screen management\n");
   printf("-no_cursor                  do not use screen management functions at all\n");
   printf("-journal <filename>         echo input commands to journal file\n");
}

bool iofull::help_manual() { return false; }
bool iofull::help_faq() { return false; }



extern void ttu_initialize()
{
   // Set the default value if the user hasn't explicitly set something.
   if (sdtty_screen_height <= 0) sdtty_screen_height = 25;

#ifdef NO_IOCTL
   sdtty_no_console = true;
#endif
}

void ttu_terminate()
{
   csetmode(0);   // Restore normal input mode.
}

// If not using curses, query the terminal driver for the
//	window size.  The file descriptor for stdout is 1.
//	If the window size cannot be determined, return 24,
//	(or whatever the user specified) as a reasonable guess.
//
static int get_nocurses_term_lines()
{
   if (!sdtty_no_console) {
#ifndef NO_IOCTL
      // If NO_IOCTL is on, we can't compile these lines.
      // But we'll never get here in that case.
      struct winsize w;

      if (ioctl(1, TIOCGWINSZ, &w) >= 0)
         return w.ws_row;
      else
         return sdtty_screen_height-1;
#else
      return sdtty_screen_height-1;
#endif
   }
   else
      return sdtty_screen_height-1;
}

extern int get_lines_for_more()
{
   return get_nocurses_term_lines();
}




extern void clear_line()
{
   // We may be on a printing terminal, so we can't erase anything.  Just
   // print "XXX" at the end of the line that we want to erase, and start
   // a new line.  This will happen if the user types "C-U", or after any
   // "--More--" line.  We can't make the "--More--" line go away completely,
   // leaving a seamless transcript.  This is the best we can do.
   printf(" XXX\n");
}

extern void rubout()
{
   printf("\b \b");    // We hope that this works.
}

extern void erase_last_n(int n)
{
}

extern void put_line(const char the_line[])
{
   if (sdtty_no_console) {
      // No funny stuff at all.
      // By leaving "use_escapes_for_drawing_people" at zero, we know
      // that the line will be nothing but ASCII text.
      fputs(the_line, stdout);
   }
   else {
      // We need to watch for escape characters
      // denoting people to be printed in color.

      char c;
      while ((c = *the_line++)) {
         if (c == '\013') {
            int personidx = (*the_line++) & 7;
            int persondir = (*the_line++) & 0xF;

            int randomized_person_color = personidx;

            if (ui_options.color_scheme == color_by_couple_random && ui_options.hide_glyph_numbers) {
               randomized_person_color = (color_randomizer[personidx>>1] << 1) | (personidx & 1);
            }

            put_char(' ');

            if (ui_options.color_scheme != no_color) {
               fputs("\033[1;", stdout);
               fputs(color_translations[color_index_list[randomized_person_color]].vt100_string, stdout);

               if (ui_options.reverse_video)
                  fputs(";40m", stdout);
               else
                  fputs(";47m", stdout);
            }

            if (ui_options.hide_glyph_numbers)
               put_char(' ');
            else
               put_char(ui_options.pn1[personidx]);

            put_char(ui_options.pn2[personidx]);
            put_char(ui_options.direc[persondir]);

            if (ui_options.color_scheme != no_color) {
               if (ui_options.reverse_video)
                  (void) fputs("\033[0;37;40m", stdout);
               else
                  (void) fputs("\033[0;30;47m", stdout);
            }
         }
         else
            put_char(c);
      }
   }
}

extern void put_char(int c)
{
   putchar(c);
}


extern int get_char()
{
   csetmode(1);         // Raw, no echo, single-character mode.
   // Unfortunately, "read" doesn't work.  Previously written text doesn't appear
   // on the screen until "read" reads a character.

   int nc = getchar();

   if (nc == 0x1B) {
      int ec = getchar();

      if (ec == 0x1B) {
         // Need to type escape twice on Linux.  (It has to do with the behavior of the
         // VT100 thin-wire terminal device.  Aren't you glad you asked?)  Just return same.
      }
      else if (ec == 0x4F) {
         ec = getchar();
         if (ec == 0x51)
            nc = FCN_KEY_TAB_LOW+1;
         else if (ec == 0x52)
            nc = FCN_KEY_TAB_LOW+2;
         else if (ec == 0x53)
            nc = FCN_KEY_TAB_LOW+3;
         else if (ec == 0x48)       // home
            nc = EKEY+4;
         else if (ec == 0x46)       // end
            nc = EKEY+3;
      }
      else if (ec == 0x5B) {
         int ec1 = getchar();

         if (ec1 == 0x41)      // up arrow
            nc = EKEY+6;
         else if (ec1 == 0x42) // down arrow
            nc = EKEY+8;
         else if (ec1 == 0x43) // right arrow
            nc = EKEY+7;
         else if (ec1 == 0x44) // left arrow
            nc = EKEY+5;
         else {
            int ec2 = getchar();

            if (ec2 == '~') {
               if (ec1 == '5') // page up
                  nc = EKEY+1;
               else if (ec1 == '6') // page down
                  nc = EKEY+2;
            }
            else {
               int ec3 = getchar();

               if (ec3 == '~') {
                  if (ec1 == '2' && ec2 == '4')
                     nc = FCN_KEY_TAB_LOW+11;
                  else if (ec1 == '1' && ec2 == '5')
                     nc = FCN_KEY_TAB_LOW+4;
                  else if (ec1 == '1' && ec2 == '7')
                     nc = FCN_KEY_TAB_LOW+5;
                  else if (ec1 == '1' && ec2 == '8')
                     nc = FCN_KEY_TAB_LOW+6;
                  else if (ec1 == '1' && ec2 == '9')
                     nc = FCN_KEY_TAB_LOW+7;
                  else if (ec1 == '2' && ec2 == '0')
                     nc = FCN_KEY_TAB_LOW+8;
                  else if (ec1 == '2' && ec2 == '1')
                     nc = FCN_KEY_TAB_LOW+9;
               }
            }
         }
      }
      else if (ec >= 0x61 && ec <= 0x7A) {
         nc = ec+ALTLET-0x20;     // alt letter; comes across as lower case letter after the escape.
      }
      else if (ec >= 0x01 && ec <= 0x1A) {
         nc = CTLALTLET+0x40+ec;  // ctl-alt letter; comes across as control letter after the escape.
      }
   }

   // Turn control characters (other than ones
   // that are real genuine control characters)
   // into our special encoding.

   if (nc >= 'A'-0100 && nc <= 'Z'-0100) {
      if (nc != '\b' && nc != '\r' &&
          nc != '\n' && nc != '\t')
         nc += CTLLET+0100;
   }

   return nc;
}


extern void get_string(char *dest, int max)
{
   int size;

   csetmode(0);         // Regular full-line mode with system echo.
   if (!fgets(dest, max, stdin)) return;
   size = strlen(dest);

   while (size > 0 && (dest[size-1] == '\n' || dest[size-1] == '\r'))
      dest[--size] = '\000';

   fputs(dest, stdout);
   putchar('\n');
}


extern void ttu_bell()
{
   putchar('\007');
}

// signal handling

static void stop_handler(int n)
{
   csetmode(0);
   signal(SIGTSTP, SIG_DFL);
   raise(SIGTSTP);			// resend the caught SIGTSTP
}

// Need a forward declaration.
void initialize_signal_handlers();

static void cont_handler(int n)
{
   initialize_signal_handlers();	// restore signal handlers
   refresh_input();
}

void initialize_signal_handlers()
{
    signal(SIGINT,  term_handler);
    signal(SIGQUIT, term_handler);
    signal(SIGTERM, term_handler);
    signal(SIGCONT, cont_handler);
    signal(SIGTSTP, stop_handler);
}

void iofull::final_initialize()
{
   if (!sdtty_no_console)
      ui_options.use_escapes_for_drawing_people = 1;

   initialize_signal_handlers();
}
