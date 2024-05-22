/*
  keybd.c

  keyboard handling support for SigSRF and EdgeStream reference applications

  Copyright (C) Signalogic, 2007-2024

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

  Revision History

   Created May 2007 NR
   Modified May-Jun 2010 VR, JHB, adjusted to updated DirectCore (merged with SigC641x)
   Modified Feb 2023 JHB, in getkey(), add simple multithread semaphore lock and fix problem in Docker containers where fgetc() input is ignored
   Modified Apr 2023 JHB, fix uninitialized variable in getkey(). See comment
   Modified May 2024 JHB, add extern "C" around getkey(), which fixes link errors with g++ compilation (same mod made in keybd.h). Possibly other functions will need this at some point
*/

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <semaphore.h>

#include "alias.h"  /* _GCC_VERSION macro */

int kbhit() {

struct timeval tv;
fd_set read_fd;

   tv.tv_sec = 0;
   tv.tv_usec = 10000;  /* some delay needed to correctly process backspace key -- amount of delay may have something to do with whether Putty or other remote access is used */
   FD_ZERO(&read_fd);
   FD_SET(STDIN_FILENO, &read_fd);
  
   if (select(STDIN_FILENO+1, &read_fd, NULL, NULL, &tv) == -1) return 0;

   return (FD_ISSET(STDIN_FILENO, &read_fd));
}


void disable_kbd_echo() {

struct termios ttystate;  

/* get the terminal state */

   tcgetattr(STDIN_FILENO, &ttystate);  

/* turn off echo */

   ttystate.c_lflag &= ~ECHO;  

/* set terminal state */

   tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);  
}


void enable_kbd_echo() {

struct termios ttystate;  

/* get the terminal state */

   tcgetattr(STDIN_FILENO, &ttystate);  

/* turn off echo */

   ttystate.c_lflag |= ECHO;  

/* set terminal state */

   tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);  
}


void enable_kbd_nonblock() {

struct termios ttystate;  

// get the terminal state  

   tcgetattr(STDIN_FILENO, &ttystate);  

// turn off canonical mode  

   ttystate.c_lflag &= ~ICANON;
//   ttystate.c_iflag &= ~IUTF8;

// minimum of number input read.  

   ttystate.c_cc[VMIN] = 1;  

   tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);  
}


void disable_kbd_nonblock() {

struct termios ttystate;  

// get the terminal state  

   tcgetattr(STDIN_FILENO, &ttystate);  

// turn on canonical mode  

   ttystate.c_lflag |= ICANON;  
   ttystate.c_iflag |= IUTF8;

// set the terminal attributes.  

   tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);  
} 


int getentry(const char* fstr, char* rstr) {

int fValidEntry;
char ch;

   #if (_GCC_VERSION >= 40500)  // Added by HP, SC (Oct 2014) to compare with GCC version to remove pragma error in lower versions
     #pragma GCC diagnostic push
     #pragma GCC diagnostic ignored "-Wunused-result"
   #endif

   scanf(fstr, rstr);

   #if (_GCC_VERSION >= 40500)
     #pragma GCC diagnostic pop
   #endif

   fValidEntry = 1;

   do {  /* flush anything left after scanf */

      ch = getchar();

      if (!strcmp(fstr, "%d")) {  /* for numeric entry check for valid entry */

         if (!isdigit(ch) && ch != '-' && ch != '\n') fValidEntry = 0;
      }

   } while (ch != '\n');

   return fValidEntry;
}

#ifdef __cplusplus
extern "C" {
#endif

int getkey() {

int ch = 0;  /* initialized to zero to fix read(fileno, 1) not handling some upper case chars (don't know why this didn't manifest earlier), JHB Apr 2023 */
struct termios orig_term_attr, new_term_attr;

#if 0
static bool fOnce = false;
if (!fOnce) { /*setbuf(stdin, NULL);*/ fprintf(stderr, " ***** STDIN_FILENO = %d, fileno(stdin) = %d \n", STDIN_FILENO, fileno(stdin)); fOnce = true; }
#endif

static int sem = 0;

   #if 0
   if (sem > 0) return 0;  /* crude semaphore for use with multithread operation, needs to be done right, JHB Jul 2018 */
   else sem++;
   #else  /* improve the semaphore, JHB Feb 2023 */
   while (__sync_lock_test_and_set(&sem, 1) != 0);  /* wait until the lock is zero then write 1 to it. While waiting keep writing a 1 */
   #endif

/* set terminal to raw mode, this uses non-blocking and doesn't wait for char input (i.e. doesn't wait for entry + Enter key) */

   tcgetattr(fileno(stdin), &orig_term_attr);
   memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
   new_term_attr.c_lflag &= ~(ECHO|ICANON);
   new_term_attr.c_cc[VTIME] = 0;
   new_term_attr.c_cc[VMIN] = 0;
   tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

   #ifdef USE_FGETC
   ch = fgetc(stdin);  /* read a character from the stdin stream without blocking, returns EOF (-1) if no character is available */
   #else  /* fix problem in Docker containers where fgetc() input is ignored */
   if (!read(fileno(stdin), (char*)&ch, 1)) ch = -1;  /* if no chars available, set to -1 to be compatible with fgetc() JHB Feb 2023 */
   #endif

   tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);  /* restore the original terminal attributes */

   #if 0
   sem--;
   #else
   __sync_lock_release(&sem);  /* clear the mem barrier (write 0 to the lock) */
   #endif

   return ch;
}

#ifdef __cplusplus
}
#endif