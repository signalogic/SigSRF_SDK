/*
  keybd.c

  keyboard handling support for VOP Program emulation terminal Program
  Copyright (C) Signalogic, 2007-2010
  Ver 2.0

  Revision History
  ----------------
  Created   May 2007, NR
  Modified  May-Jun 2010, adjusted to updated DirectCore (merged with SigC641x), VR, JHB
  
*/

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

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

int getkey() {

int character;
struct termios orig_term_attr;
struct termios new_term_attr;

static int sem = 0;

   if (sem > 0) return 0;  /* crude semaphore for use with multithread operation, needs to be done right, JHB Jul 2018 */
   else sem++;
 
/* set the terminal to raw mode */

   tcgetattr(fileno(stdin), &orig_term_attr);
   memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
   new_term_attr.c_lflag &= ~(ECHO|ICANON);
   new_term_attr.c_cc[VTIME] = 0;
   new_term_attr.c_cc[VMIN] = 0;
   tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

/* read a character from the stdin stream without blocking */
/* returns EOF (-1) if no character is available */
   character = fgetc(stdin);

/* restore the original terminal attributes */
   tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);

   sem--;

   return character;
}
