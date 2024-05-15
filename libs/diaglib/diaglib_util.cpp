/*
 $Header: /root/Signalogic/DirectCore/lib/diaglib/diaglib_util.cpp
 
 Description: SigSRF and EdgeStream event diagnostic related utility APIs
 
 Project: SigSRF, DirectCore
 
 Copyright Signalogic Inc. 2017-2024

 Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Revision History
  
  Modified Apr 2020 JHB, implement DSGetLogTimeStamp() API
  Modified Feb 2024 JHB, add optional user-defined time value param to DSGetLogTimestamp(), implement new DS_EVENT_LOG_USER_TIMEVAL and DS_EVENT_LOG_TIMEVAL_PRECISE flags (defined in shared_include/config.h)
  Modified Feb 2024 JHB, add DSGetMD5Sum() API, see usage comments below and in diaglib.h
  Modified Mar 2024 JHB, add return value to Log_RT(). Now it returns number of chars written, as with printf() and fprintf()
  Modified Mar 2024 JHB, fix bug in open_log_file() where it attempted to use the diaglib semaphore before it's initialized
  Modified Apr 2024 JHB, deprecate DS_LOG_LEVEL_UPTIME_TIMESTAMP flag (diaglib.h) and DS_EVENT_LOG_UPTIME_TIMESTAMPS flag (shared_include/config.h), the default (no flag) is now uptime timestamps. DS_LOG_LEVEL_NO_TIMESTAMP can be combined with log_level (i.e. Log_RT(log_level, ...) to specify no timestamp
  Modified Apr 2024 JHB, DSGetLogTimestamp() now returns length of timestamp string
  Modified May 2024 JHB, add DSGetBacktrace() API
  Created May 2024, DSGetLogTimeStamp(), DSGetMD5Sum(), and DSGetBacktrace() APIs moved here from lib_logging.cpp
*/

/* Linux and/or other OS includes */

#include <ctype.h>     /* isalnum() */
#include <string.h>
#include <stdlib.h>
#include <time.h>      /* struct tm, time(), localtime_r() */
#include <sys/time.h>  /* gettimeofday() */
#include <execinfo.h>  /* backtrace() */
#include <algorithm>   /* std::min and std::max */

// #define DEBUG_OUTPUT  /* enable if needed */
#ifdef DEBUG_OUTPUT
  #include <stdio.h>
#endif

using namespace std;

/* SigSRF includes */

#include "diaglib.h"

#ifndef MAX_INPUT_LEN
  #define MAX_INPUT_LEN 256  /* MAX_INPUT_LEN is defined in shared_include/userInfo.h as equivalent to CMDOPT_MAX_INPUT_LEN (defined in apps/common/cmdLineOpt.h). But we'd like to avoid those includes for diaglib, so we define here if needed */
#endif

extern uint64_t usec_base;  /* declared in lib_logging.cpp */
extern uint8_t usec_init_lock;

/* retrieve and format a timestamp, can be absolute (wall-clock) time, relative to start, or both. Log_RT() (lib_logging.cpp) depends on this */

int DSGetLogTimeStamp(char* timestamp, int max_str_len, uint64_t user_timeval, unsigned int uFlags) {

time_t ltime;
struct tm tm;
struct timeval tv;
uint64_t usec = 0;
bool fWallClockTimestamp = (uFlags & DS_EVENT_LOG_WALLCLOCK_TIMESTAMPS) != 0;
#if 0  /* the default (no flag) is now uptime timestamps. When calling Log_RT(), the DS_LOG_LEVEL_NO_TIMESTAMP can be combined with log_level (i.e. Log_RT(log_level, ...) to specify no timestamp, JHB Apr 2024 */
bool fUptimeTimestamp = (uFlags & DS_EVENT_LOG_UPTIME_TIMESTAMPS) != 0;
#else
bool fUptimeTimestamp = true;
#endif

/* set a memory barrier, prevent multiple uncoordinated threads from initializing usec_base more than once. Note the same lock protects usec_base initialization in Log_RT() (in lib_logging.cpp), JHB May 2024 */

   while (__sync_lock_test_and_set(&usec_init_lock, 1) != 0);  /* wait until the lock is zero then write 1 to it. While waiting keep writing a 1 */

   if (!usec_base) {  /* initialize usec_base if needed. Log_RT() makes the same check, JHB May 2024 */

      struct timeval tv;
      gettimeofday(&tv, NULL);
      usec_base = tv.tv_sec*1000000L + tv.tv_usec;
   }
 
   __sync_lock_release(&usec_init_lock);  /* clear the mem barrier (write 0 to the lock) */

/* Note that wall clock and uptime timestamps can be combined (as an example, mediaMin interactive keyboard debug output ('d' key) does this), JHB Apr 2020 */
 
   if (fWallClockTimestamp) { /* include wall clock timestamp if specified, JHB Feb 2020 */
   
      ltime = time(NULL);
      localtime_r(&ltime, &tm);
      strftime(timestamp, max_str_len, "%m/%d/%Y %H:%M:%S", &tm);

      if (uFlags & DS_EVENT_LOG_USER_TIMEVAL) usec = user_timeval;
      else {
         gettimeofday(&tv, NULL);
         usec = tv.tv_sec*1000000L + tv.tv_usec - usec_base;
      }

      if (!fUptimeTimestamp || (uFlags & DS_EVENT_LOG_TIMEVAL_PRECISE)) sprintf(&timestamp[strlen(timestamp)], ".%03d.%03d", (int)(usec/1000) % 1000, (int)(usec % 1000));  /* add msec and usec -- see uptime timestamp generation comments below */
   }
   
   if (fUptimeTimestamp) {  /* include uptime timestamp if specified, JHB Apr 2020 */

      if (!fWallClockTimestamp) {

         timestamp[0] = '\0';  /* if wallclock timestamps were not requested, ensure timestamp has zero length before concatenating */

         if (uFlags & DS_EVENT_LOG_USER_TIMEVAL) usec = user_timeval;
         else {
            gettimeofday(&tv, NULL);
            usec = tv.tv_sec*1000000L + tv.tv_usec - usec_base;
         }
      }

   /* generate uptime (relative) timestamp:

      -hours, minutes, sec, msec, and usec. A high granularity is required to help measure and debug audio quality, stream alignment, and other timing issues
      -note for number of hours we don't use modulus to stay within a range. When number of digits exceeds the %02 specifier, sprintf doesn't cut anything off (verified with 100+ hr stress tests, JHB Mar 2019)
   */

      if (fWallClockTimestamp) strcat(timestamp, " (");  /* note that parens () are applied only if both wallclock and uptime timestamps are requested */
      int hours = (int)(usec/3600000000L);
      if (!(uFlags & DS_EVENT_LOG_USER_TIMEVAL) || hours > 0) sprintf(&timestamp[strlen(timestamp)], "%02d:", hours);  /* for user-specified timeval, omit hours unless != zero, JHB Feb 2024 */
      sprintf(&timestamp[strlen(timestamp)], "%02d:%02d", (int)(usec/60000000L) % 60, (int)(usec/1000000L) % 60);

      if (!(uFlags & DS_EVENT_LOG_USER_TIMEVAL) || (uFlags & DS_EVENT_LOG_TIMEVAL_PRECISE)) sprintf(&timestamp[strlen(timestamp)], ".%03d.%03d", (int)(usec/1000) % 1000, (int)(usec % 1000));
      if (fWallClockTimestamp) strcat(timestamp, ")");  /* for user-specified timeval, omit sec and usec unless user gives TIMEVAL_PRECISE flag, JHB Feb 2024 */
   }

   return strlen(timestamp);
}

/* get md5sum of path+file in szFilename, max_str_len should specify maximum string length of md5str. Return value of 1 indicates success, negative values an error condition, JHB Feb 2024 */

int DSGetMD5Sum(const char* szFilename, char* md5str, int max_str_len) {

char cmdstr[2*MAX_INPUT_LEN] = "md5sum";  /* command string to format and give to popen() */
int ret_val = -1;

   sprintf(&cmdstr[strlen(cmdstr)], " %s", szFilename);

   if (md5str && max_str_len > 0) {

      FILE *fp = popen(cmdstr, "r");  /* use popen() to execute md5sum cmd line and fscanf() to retrieve cmd line output */

      if (fp) {

         char formatstr[50];
         sprintf(formatstr, "%%%ds", max_str_len);  /* create format string with limit on buffer read, for example "%200s" */

         ret_val = fscanf(fp, formatstr, md5str);  /* if it runs correctly, md5sum will output 2 values: (i) md5 hash result (ii) filename. We are scanning only for first one, so we expect ret_val of 1 on success */
         pclose(fp);
      }
   }
   
   return ret_val;
}


/* use glibc backtrace() to get nLevels of call stack, clean that up, remove repeats, and format in szBacktrace (return string) */

int DSGetBacktrace(int nLevels, char* szBacktrace, unsigned int uFlags) {

#define BT_BUF_SIZE 512
int nptrs, i, j, k = 0;
void* buffer[BT_BUF_SIZE];
char** strings;
char* p;
bool fTopLevelRepeat = false;

   if (!szBacktrace) return -1;

   nptrs = backtrace(buffer, BT_BUF_SIZE);
   strings = backtrace_symbols(buffer, nptrs);

   if (!strings || !nptrs) return -1;

   #ifdef DEBUG_OUTPUT
   printf("nLevels = %d, nptrs = %d \n", nLevels, nptrs);
   #endif

   for (i=min(nptrs-1, nLevels-1); i>=0; i--) {

      #ifdef DEBUG_OUTPUT
      printf("symbol %d = %s \n", i, strings[i]);
      #endif

      if ((p = strstr(strings[i], "() ["))) {  /* strip off repeats with no function name. This happens when executable was linked without -rdynamic flag, JHB May 2024 */

         char* p2 = strrchr(strings[i], '/');
         if (p2) p2++;
         else p2 = strings[i];

         bool fValidFuncName = true;

         for (j=0; j<p-p2+1; j++) {
            if (!isalnum(p2[j]) && p2[j] != '_') { fValidFuncName = false; break; }  /* look for valid function name */
         }

         if (fValidFuncName) {  /* keep first one, strip any additional */
            if (fTopLevelRepeat) continue;
            else fTopLevelRepeat = true;
         }
      }

      if (strstr(strings[i], "DSGetBacktrace")) continue;  /* this function name is meaningless - skip */

      if (!(uFlags & DS_GETBACKTRACE_INCLUDE_GLIBC_FUNCS)) {
         if (strstr(strings[i], "libc.so.") && !strstr(strings[i], "libc_start_main")) continue;  /* skip glibc, if found */
         if (strstr(strings[i], "libpthread")) continue;  /* skip libpthread, if found */
      }

      p = strrchr(strings[i], ')');  /* strip off [0xNNNN...] */
      if (p) *(++p) = 0;

      if (k == 0) {
         if (uFlags & DS_GETBACKTRACE_INSERT_MARKER) strcpy(szBacktrace, "backtrace: ");  /* insert marker at start if asked by caller */
         else szBacktrace[0] = 0;
      }

      #ifdef DEBUG_OUTPUT
      printf("adding symbol %s \n", strings[i]);
      #endif

      sprintf(&szBacktrace[strlen(szBacktrace)], "%s%s", k > 0 ? " " : "", strings[i]);
      k++;
   }

   free(strings);

   return nptrs;
}
