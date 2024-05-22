/* keybd.h

  kbhit, getkey functions

  Copyright (C) Signalogic 2006-2024

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

  Revision History
   Created Nov 2006
   Modified Sep 2020 JHB, change getkey() return to char to fix warning in gcc 9.3.0
   Modified May 2024 JHB, add extern "C"
*/

#ifndef KEYBD_H
#define KEYBD_H

#define ESC 27
#define CR 13
#define LF 10
#define BS 8

#ifdef __cplusplus
extern "C" {
#endif

int kbhit();
void enable_kbd_nonblock();
void disable_kbd_nonblock();
void enable_kbd_echo();
void disable_kbd_echo();
int getentry(const char*, char*);
int getkey();

#ifdef __cplusplus
}
#endif

#endif
