/* $Header: /root/Signalogic/DirectCore/include/dsstring.h

 Description: some additional string functions which were lacking or convenient in C++ examples including:
              
              -finding a substring inside another string starting at an arbitrary position
              -deleting a portion of a string
              -inserting a substring into another starting at an arbitrary position

              note - for Linux development, none of the "lstrXXX" functions are used, only the static inlines

 Project: DirectCore

 Copyright Signalogic Inc. 1994-2024

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 $Log: /Deliverable/NMS/C641x/Linux/Distro/include/dsstring.h $

 2     10/10/05 12:32p
 8 DSP support

 1     8/24/05 7:13p
 Clean up code and rebuild.

 Revision History

   Modified Aug 2017 JHB, add strcpyrws() function
   Modified Aug 2019 JHB, add generic reverse strstr()
   Modified Mar 2022 JHB, move str_remove_whitespace() here from transcoder_control.c, move strrstr() here from mediaTest_proc.c
   Modified Apr 2022 JHB, add error checking to strrstr(), str_remove_whitespace(), and str_remove_linebreaks()
   Modified Jul 2023 JHB, add strncpy_s(), safe version of strncpy(), in case gcc version in use doesn't support it
   Modified May 2024 JHB, change comment that references x86_mediaTest.c to mediaTest_proc.c
   Modified Aug 2024 JHB, improvement to strncpy_s()
   Modified Dec 2024 JHB, use extern C if __cplusplus defined
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
#if !defined(__cplusplus) || defined(USE_MIN_MAX)
  #include "minmax.h"
#endif

#ifndef TCHAR
   #define TCHAR char
#endif

#if !defined(_LINUX_)
  typedef char FILESTR[MAXFILE];
#endif

typedef char LINESTR[768];       /* allow long line lengths in template files, with assumption that individual fields won't be more than 256 */
typedef char PATHSTR[MAXPATH];   /* DOS paths, including input parameters */

#ifdef __cplusplus
extern "C" {
#endif

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

int i, j = 0;

   if (!s1 || !s2) return;

   for (i=0; i<(int)strlen(s2); i++) if (s2[i] != ' ') s1[j++] = s2[i];

   s1[j] = 0;
}

/* generic whitespace removal func, JHB Jun2016 */

static inline void str_remove_whitespace(char* str) {

unsigned int i;
char* p = str;

   if (!p) return;

   for (i=0; i<strlen(str); i++) if (str[i] != ' ') *p++ = str[i];  /* in-place whitespace removal */

   *p = 0;  /* terminating zero */
}

static inline void str_remove_linebreaks(char* str) {

unsigned int i;
char* p = str;

   if (!p) return;

   for (i=0; i<strlen(str); i++) if (str[i] != 0x0A && str[i] != 0x0D) *p++ = str[i];  /* in-place linebreak removal */

   *p = 0;  /* terminating zero */
}

/* generic reverse strstr, JHB Aug 2019 */

static inline const char* strrstr(const char* haystack, const char* needle) {

if (!haystack || !needle) return NULL;

int i, needle_len = strlen(needle);
char* p = (char*)haystack + strlen(haystack) - needle_len - 1;  /* don't compare terminating zeros */

   while (p >= haystack) {

      for (i=0; i<needle_len; i++) if (p[i] != needle[i]) break;

      if (i == needle_len) return p;  /* found it */
      else p--;
   }

   return NULL;
}

#ifndef __STDC_LIB_EXT1__  /* per https://stackoverflow.com/questions/40045973/strcpy-s-not-working-with-gcc, __STDC_LIB_EXT1__ must be defined otherwise strxxx_s() functions are not available with gcc version in use, JHB Jul 2023 */

/* safe version of strncpy(), in case gcc version doesn't support strncpy_s(). Always null-terminates, JHB Jul 2023 */

static inline char* strncpy_s(char* dst, size_t max_dst_len, const char* src, size_t count) {  /* note: typically count >= max_dst_len, although count can be less for whatever reason */

   if (!dst || !src || (int)max_dst_len <= 0 || (int)count <= 0) return NULL;

   #if 0
   unsigned int len = min(strlen(src), max_dst_len-1);
   if (count >= len) {
      memcpy(dst, src, len);  /* copy the string */
      dst[len] = 0;  /* add terminating NULL */
   }
   else memcpy(dst, src, count);  /* if count is the min, then no null termination, per gcc strncpy_s operation */
   #else  /* simpler way, JHB Aug 2024 */
   unsigned int len = min(min(strlen(src), max_dst_len-1), count);
   memcpy(dst, src, len);  /* copy string, either (i) its length, (ii) max allowable space, or (iii) as specified by count */
   dst[len] = 0;  /* add terminating NULL if not already there */
   #endif

   return dst;
}
#endif

#ifdef __cplusplus
}
#endif

#endif  /* _DSSTRING_H_ */
