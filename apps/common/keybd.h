/* keybd.h
   kbhit, getch functions

   Copyright (C) Signalogic 2006-2020 unless otherwise specified   

   Revision History
     Created Nov 2006
     Modified Sep 2020, JHB change getkey() return to char to fix warning in gcc 9.3.0
*/

#ifndef KEYBD_H
#define KEYBD_H

#define ESC 27
#define CR 13
#define LF 10
#define BS 8

int kbhit();
void enable_kbd_nonblock();
void disable_kbd_nonblock();
void enable_kbd_echo();
void disable_kbd_echo();
int getentry(const char*, char*);
int getkey();

#endif
