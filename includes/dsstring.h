/* $Header: /root/Signalogic/DirectCore/include/dsstring.h

 Description: some additional string functions which were lacking     
              or convenient in C++ examples include finding a         
              substring inside another string starting at an          
              arbitrary position, deleting a portion of a string,     
              and inserting a substring into another starting at an   
              arbitrary position                                      

 Project: DirectCore

 Copyright Signalogic Inc. 1994-2017

 $Log: /Deliverable/NMS/C641x/Linux/Distro/include/dsstring.h $

 2     10/10/05 12:32p
 8 DSP support

 1     8/24/05 7:13p
 Clean up code and rebuild.

 Revision History

   Modified Aug 2017 JHB, added strcpyrws() function
*/

#ifndef _DSSTRING_H_
#define _DSSTRING_H_

#if !defined(__LIBRARYMODE__)

  #ifdef __WIN32__

    #include <stdio.h>
    #include <dir.h>

  #elif !defined(_LINUX_)

    #include <stdio_x.h>
    #include <dir_x.h>

  #endif
#endif

#include "alias.h"

#ifndef TCHAR
   #define TCHAR char
#endif

#if !defined(_LINUX_)
  typedef char FILESTR[MAXFILE];
#endif

typedef char LINESTR[768];       /* allow long line lengths in template files, with assumption that individual fields won't be more than 256 */
typedef char PATHSTR[MAXPATH];   /* DOS paths, including input parameters */

short lstrpos(LPCSTR, LPCSTR, short);  /* locate position of one string inside another */

short lstrposi(LPCSTR, LPCSTR, short);  /* same as above, but case-insensitive */

#if ! defined (__KERNEL__) && ! defined (_LINUX_)  && !defined (_WIN32_WINNT)  /* C code can't use short & */
short lstrposvs(LPCSTR, LPCSTR, short, short &);  /* same as above, but with valid separator check and operation type */
#endif

void lstrdel(LPSTR, short, short);  /* delete specified number of characters from a string */

void lstrins(LPCSTR, LPSTR, short);  /* insert one string into another starting at pos */

void lstrovw(LPCSTR, LPSTR, short);  /* overwrite one string into another starting at pos */

void lstrupr(LPSTR);  /* convert string to uppercase */

void lstrlwr(LPSTR);  /* convert string to lowercase */

void lstrtrim(LPSTR);  /* trim leading and trailing spaces */

void lstrtrm(LPSTR);  /* trim leading spaces */

void lstrtrm2(LPSTR);  /* trim trailing spaces */

void lstrPathFixup(LPSTR);  /* ensure that string has valid path format */

BOOL lstrContainsPathInfo(LPCSTR);

BOOL lstrContainsNonPrintableChar(LPCSTR);

LPSTR lstrcpydz(LPSTR, LPCSTR);

short lstrlendz(LPCSTR);

static inline void strcpyrws(char* s1, const char* s2) {  /* copy and remove white space (there isn't a standard C function for this (https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way), JHB Aug2017 */

unsigned int i, j = 0;

   if (!s1 || !s2) return;

   for (i=0; i<strlen(s2); i++) if (s2[i] != ' ') s1[j++] = s2[i];

   s1[j] = 0;
}

#endif 
