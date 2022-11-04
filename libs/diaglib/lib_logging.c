/*
  $Header: /root/Signalogic/DirectCore/lib/pktlib/pktlib_logging.c
 
  Description: SigSRF and EdgeStream event logging APIs
 
  Project: SigSRF, DirectCore
 
  Copyright Signalogic Inc. 2017-2022

  Revision History
  
   Created Aug 2017 Chris Johnson
   Modified Sep 2017 JHB, moved Log_RT() here from pktlib_logging.c.  Moved declaration of pktlib_dbg_cfg here from pktlib.c (and renamed to lib_dbg_cfg).  Added DSGetAPIStatus() error/warning and function codes
   Modified Sep 2017 JHB, added more DS_API_CODE_xxx definitions
   Modified Jul 2018 CKJ, look at LOG_SCREEN_ONLY, LOG_FILE_ONLY, AND LOG_SCREEN_FILE constants in Log_RT()
   Modified Oct 2018 JHB, add timestamps to Log_RT() output (see USE_TIMESTAMP define below).  Possibly this should be an option in the DEBUG_CONFIG struct (shared_include/config.h)
   Modified Jan 2019 JHB, remove 0..23 wrap from hours field of logging timestamp
   Modified Feb 2019 JHB, add code to look at LOG_SET_API_STATUS flag
   Modified Nov 2019 JHB, add va_end() to match va_start() in Log_RT().  Could not having then been cause of very slow mem leak ?
   Modified Dec 2019 JHB, add log file creation if lib_dbg_cfg.uEventLogFile is NULL, recreation if the file is deleted (possibly by an external process), uEventLog_fflush_size and uEventLog_max_size support
   Modified Jan 2020 JHB, use libg_dbg_cfg.uPrintfControl to determine screen output of Log_RT(), see comments near USE_NONBUFFERED_OUTPUT
   Modified Feb 2020 JHB, implement wall clock timestamp option
   Modified Mar 2020 JHB, implement DS_LOG_LEVEL_FILE_ONLY flag, handle leading newline (\n) in app supplied strings
   Modified Mar 2020 JHB, implement uLineCursorPos and isCursorMidLine in screen output handling; isCursorPosMidLine determines leading \n decisions. uLineCursorPos records line cursor position
   Modified Apr 2020 JHB, implement DSGetLogTimeStamp() API
   Modified Jan 2021 JHB, include string.h with _GNU_SOURCE defined, change loglevel param in Log_RT() from uint16_t to uint32_t, implement DS_LOG_LEVEL_SUBSITUTE_WEC flag (config.h). See comments
   Modified Mar 2021 JHB, minor adjustments to removal of unncessary Makefile defines, add DIAGLIB_STANDALONE #define option to build without IsPmThread()
*/

/* Linux and/or other OS includes */

#define _GNU_SOURCE
#include <string.h>
#include <stdarg.h>  /* va_start(), va_end() */
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/* Sig public includes */

#ifndef DIAGLIB_STANDLONE
  #define __LIBRARYMODE__
  #include "pktlib.h"
  #undef __LIBRARYMODE__
#endif

#include "diaglib.h"
#include "shared_include/config.h"

/* Sig private includes */

#include "diaglib_priv.h"

/* global vars */

static pthread_key_t status_key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static uint64_t last_size = 0;

DEBUG_CONFIG lib_dbg_cfg = { 5 };  /* moved here from pktlib.c, JHB Sep017.  Init to log level 5 for apps that don't make a DSConfigPktlib() call, JHB Jul2019 */

volatile uint8_t uLineCursorPos = 0;  /* referenced by p/m threads and apps if they want to know / set the current screen line cursor position */
volatile uint8_t isCursorMidLine = 0;
volatile uint32_t pm_thread_printf = 0;

uint32_t event_log_critical_errors = 0;  /* keep track of event log errors and warnings for program lifespan. The main purpose of this is to quickly show a general error summary in the run-time stats without having to search the event log. It's not that useful because it doesn't break down by session or stream (channel), JHB May2020 */
uint32_t event_log_errors = 0;
uint32_t event_log_warnings = 0;

/* private APIs */

static void make_key()
{
   pthread_key_create(&status_key, NULL);
}

void set_api_status(int status_code, unsigned int uFlags) {

int *ptr;

   (void)uFlags;  /* avoid compiler warning (Makefile has -Wextra flag) */
   
   pthread_once(&key_once, make_key);
   
   /* if the value stored is NULL, this is the first time this thread is setting this value - allocate memory */
   if ((ptr = pthread_getspecific(status_key)) == NULL)
      ptr = (int *)malloc(sizeof(int));
   
   *ptr = status_code;
   pthread_setspecific(status_key, ptr);
   
   /* valgrind reports still reachable memory for the above malloc 
      I think pthread_setspecific frees the pointer when it's set to a new value so another call with a NULL pointer would be needed to correctly cleanup all mallocs here -cj */
}


/* public APIs */

int DSGetAPIStatus(unsigned int uFlags) {

int *ptr = pthread_getspecific(status_key);

   (void)uFlags;  /* avoid compiler warning (Makefile has -Wextra flag) */
   
/* if no value has been set, ptr will be NULL and we return 0 indicating no status */

   if (ptr != NULL) return *ptr;
   else return 0;
}

bool isFileDeleted(FILE* fp) {  /* check if file has been deleted, possibly be an external process.  Note we cannot use fwrite() or other error codes, we need to look at file descriptor level, JHB Dec2019 */

bool fRet = false;
struct stat fd_stat;
int fd;

   fd = fileno(fp);  /* convert file pointer to file descriptor */

   if (fcntl(fd, F_GETFL) && !fstat(fd, &fd_stat)) {

      if (fd_stat.st_nlink == 0) fRet = true;  /* file has been deleted if no "hard links" exist, JHB Dec2019 */
   }

   return fRet;
}

static inline void strlcpy(char* dst, const char* src, int maxlen) {  /* implementation of strlcpy() since it's a BSD Linux function and evidently not available in gcc/g++, JHB Jan2020 */

   int cpylen =  min((int)strlen(src), maxlen-1);
   memcpy(dst, src, cpylen);
   dst[cpylen] = (char)0;
}


#define ERROR_PARSE_LOGMSG
#define MAX_STR_SIZE 4000
#define MAX_ERRSTR_SIZE 200

static uint64_t usec_init = 0;

void Log_RT(uint32_t loglevel, const char* fmt, ...) {

#ifdef ERROR_PARSE_LOGMSG
static int status_code = 0;  /* to-do: not thread safe, need some way to keep a status_code per thread, JHB Sep2017 */
#endif

va_list va;
char tmpstr[MAX_ERRSTR_SIZE], log_string[MAX_STR_SIZE];
int slen, fmt_start = 0;

   if (lib_dbg_cfg.uEventLogMode & DS_EVENT_LOG_DISABLE) return;  /* event log is (temporarily) disabled */

   if ((lib_dbg_cfg.uEventLogMode & DS_EVENT_LOG_WARN_ERROR_ONLY) && (loglevel & DS_LOG_LEVEL_MASK) > 3) return;  /* event log warn and error output only (temporarily) */

   if (!usec_init) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      usec_init = tv.tv_sec*1000000L + tv.tv_usec;
   }

   log_string[0] = (char)0;

   if (loglevel & DS_LOG_LEVEL_APPEND_STRING) {

      char* p_fmt_start = strstr(fmt, "%s");

      if (p_fmt_start) {
         fmt_start = p_fmt_start - fmt;
         strncpy(log_string, fmt, fmt_start);
         log_string[fmt_start] = '\0';
      }
   }
  
   if ((loglevel & DS_LOG_LEVEL_MASK) < lib_dbg_cfg.uLogLevel) {

   /* get timestamp */

      if (!(loglevel & DS_LOG_LEVEL_NO_TIMESTAMP)) DSGetLogTimeStamp(&log_string[fmt_start], sizeof(log_string), (unsigned int)lib_dbg_cfg.uEventLogMode);

      int log_str_len = strlen(log_string);

   /* add and format Log_RT() string */

      va_start(va, fmt);

      vsnprintf(&log_string[log_str_len], sizeof(log_string) - log_str_len, &fmt[fmt_start], va);

      va_end(va);  /* added JHB Nov 2019 */

   /* leading newline handling: if first char in user string is a newline, move the newline to be in front of the timestamp, JHB Mar2020 */

      if (log_string[log_str_len] == '\n') {
         int i;
         for (i=log_str_len; i>0; i--) log_string[i] = log_string[i-1];  /* this is a rare thing, so ok to do a slow move here */
         log_string[0] = '\n';
      }

   /* add newline if one not already there, JHB Sep2017. Made optional with DS_EVENT_LOG_DONT_ADD_NEWLINE flag, JHB Apr2020 */

      if (!(loglevel & DS_LOG_LEVEL_DONT_ADD_NEWLINE) && log_string[strlen(log_string)-1] != '\n') strcat(log_string, "\n");

   /* record lifespan stats */

      if ((loglevel & DS_LOG_LEVEL_MASK) < 2) __sync_add_and_fetch(&event_log_critical_errors, 1);
      else if ((loglevel & DS_LOG_LEVEL_MASK) == 2) __sync_add_and_fetch(&event_log_errors, 1);
      else if ((loglevel & DS_LOG_LEVEL_MASK) == 3) __sync_add_and_fetch(&event_log_warnings, 1);

   /* error / warning parsing if (i) enabled and (ii) level is below INFO */

#ifdef ERROR_PARSE_LOGMSG

      if (!(loglevel & DS_LOG_LEVEL_NO_API_CHECK) && lib_dbg_cfg.uEventLogMode & LOG_SET_API_STATUS) {

//  printf(" &&&&&&&&&&&&&&&&&&&& setting API code, log_string = %s\n", log_string);

      /* to-do:

         1) Add more published API codes.  Some APIs may not have function names in their log messaages yet (I have been updating many of them over last few weeks)
         2) Add more internal API codes
         3) Maybe some internal APIs should be "2nd and 3rd level" flags that can be combined, if calls can go 3 levels deep
      */

         if ((loglevel & DS_LOG_LEVEL_MASK) < 4) {  /* check only 0-3 for error/warning, JHB Jan2020 */

            strlcpy(tmpstr, log_string, MAX_ERRSTR_SIZE);  /* would use strncpy, except for its padding, JHB Jan2020 */
            strupr(tmpstr);

            int status_code_temp = 0;

            if (strstr(tmpstr, "ERROR") || strstr(tmpstr, "CRITICAL")) status_code_temp |= DS_API_STATUS_CODE_ERROR;
            if (strstr(tmpstr, "WARNING")) status_code_temp |= DS_API_STATUS_CODE_WARNING;

         /* published APIs */

            if (strstr(tmpstr, "DSCREATESESSION")) status_code_temp |= DS_API_CODE_CREATESESSION;
            else if (strstr(tmpstr, "DSDELETESESSION")) status_code_temp |= DS_API_CODE_DELETESESSION;
            else if (strstr(tmpstr, "DSBUFFERPACKETS")) status_code_temp |= DS_API_CODE_BUFFERPKTS;
            else if (strstr(tmpstr, "DSGETORDEREDPACKETS")) status_code_temp |= DS_API_CODE_GETORDEREDPKTS;
            else if (strstr(tmpstr, "DSGETPACKETINFO")) status_code_temp |= DS_API_CODE_GETPACKETINFO;
            else if (strstr(tmpstr, "DSGETSESSININFO")) status_code_temp |= DS_API_CODE_GETSESSIONINFO;
            else if (strstr(tmpstr, "DSGETDTMFINFO")) status_code_temp |= DS_API_CODE_GETDTMFINFO;
            else if (strstr(tmpstr, "DSFORMATPACKET")) status_code_temp |= DS_API_CODE_FORMATPACKET;
            else if (strstr(tmpstr, "DSSTORESTREAMDATA")) status_code_temp |= DS_API_CODE_STORESTREAMDATA;
            else if (strstr(tmpstr, "DSGETSTREAMDATA")) status_code_temp |= DS_API_CODE_GETSTREAMDATA;

         /* internal APIs, note these may be combined with published APIs */

            if (strstr(tmpstr, "VALIDATE_RTP")) status_code_temp |= DS_API_CODE_VALIDATERTP;
            else if (strstr(tmpstr, "GET_CHAN_PACKETS")) status_code_temp |= DS_API_CODE_GETCHANPACKETS;
            else if (strstr(tmpstr, "CREATE_DYNAMIC_CHAN")) status_code_temp |= DS_API_CODE_CREATEDYNAMICCHAN;

            if (status_code_temp) {
               status_code = status_code_temp;
               set_api_status(status_code, (intptr_t)NULL);
            }
         } 
      }
#endif  /* ERROR_PARSE_LOGMSG */

   /* output to log file and/or screen */

      if (((lib_dbg_cfg.uEventLogMode & LOG_MODE_MASK) != LOG_SCREEN_ONLY) && !(loglevel & DS_LOG_LEVEL_DISPLAY_ONLY)) {

         bool fOpenWriteOnly = false;

create_log_file_if_needed:

         if (!lib_dbg_cfg.uEventLogFile && strlen(lib_dbg_cfg.szEventLogFilePath)) {  /* create event log file (or open it for appending), if needed, JHB Dec2019 */

            bool fAppend = (lib_dbg_cfg.uEventLogMode & DS_EVENT_LOG_APPEND) && !fOpenWriteOnly;

            lib_dbg_cfg.uEventLogFile = fopen(lib_dbg_cfg.szEventLogFilePath, fAppend ? "a" : "w");

            if (!lib_dbg_cfg.uEventLogFile) fprintf(stderr, "ERROR: Log_RT() says unable to %s event log file %s, errno = %d \n", fAppend ? "open for appending" : "create", lib_dbg_cfg.szEventLogFilePath, errno);
#if 0
            else {

               int fd = fileno(lib_dbg_cfg.uEventLogFile);  /* convert file pointer to file descriptor */
               int flags = fcntl(fd, F_GETFL);
               fcntl(fd, F_SETFL, flags | O_NONBLOCK);
            }
#endif
//            else fwrite("\xEF\xBB\xBF", 3, 1, lib_dbg_cfg.uEventLogFile);  /* set BOM to UTF-8 */
         }

         if (lib_dbg_cfg.uEventLogFile) {

         /* check for WEC substitution, notes:  JHB Jan2021

            -applied to log file text only (not screen)
            -objective is to prevent false-positive keyword searches for warning, error, or critical when needed. Such searches may be manual or automated by scripts to check for stress test errors, in logs generated over hours or days
            -currently this is used in packet_flow_media_proc.c to prevent false-positive searches due to logging run-time stats
            -to alter the keyword, insert | after first character. Don't mess with substitutions based on ACP (ANSI code page), Unicode, etc -- these methods are unpredictable on various systems with various editors
         */

            char* p = log_string;

            if (loglevel & DS_LOG_LEVEL_SUBSITUTE_WEC) {

            /* make a string copy so screen output is not affected */

               char log_string_copy[MAX_STR_SIZE];
               strcpy(log_string_copy, log_string);
               int len = strlen(log_string_copy);

               do {  /* loop to handle all occurrences, always check to make sure we're not expanding the string past mem limits */

                  p = strcasestr(log_string_copy, "warning");
                  if (!p) p = strcasestr(log_string_copy, "error");
                  if (!p) p = strcasestr(log_string_copy, "critical");

                  if (p && len+1 < MAX_STR_SIZE) { memmove(p+2, p+1, strlen(p+1)+1); *(p+1) = '|'; len++; }

               } while (p);

               p = log_string_copy;
            }

            int ret_val = fwrite(p, strlen(p), 1, lib_dbg_cfg.uEventLogFile);

            if (ret_val != 1) {  /* we call fwrite() with number of "elements" = 1, not bytes */

               fprintf(stderr, "\nERROR: Log_RT() says not able to write to event log file %s, errno = %d \n", lib_dbg_cfg.szEventLogFilePath, errno);
            }
            else if (isFileDeleted(lib_dbg_cfg.uEventLogFile)) {

               fprintf(stderr, "\nERROR: Log_RT() says event log file %s may have been deleted, errno = %d, attempting to recreate file ... \n", lib_dbg_cfg.szEventLogFilePath, errno);
               lib_dbg_cfg.uEventLogFile = NULL;
               fOpenWriteOnly = true;
               goto create_log_file_if_needed;
            }
            else {  /* log file operating normally, check for flush_size and/or max_size in effect */

               if (lib_dbg_cfg.uEventLog_fflush_size) {

                  uint64_t fsize = ftell(lib_dbg_cfg.uEventLogFile);

                  if (fsize > lib_dbg_cfg.uEventLog_fflush_size) {
                     last_size = fsize;
                     fflush(lib_dbg_cfg.uEventLogFile);
                  }
               }

               if (lib_dbg_cfg.uEventLog_max_size) {

                  uint64_t fsize = ftell(lib_dbg_cfg.uEventLogFile);

                  if (fsize > lib_dbg_cfg.uEventLog_max_size) rewind(lib_dbg_cfg.uEventLogFile);
               }
            }
         }
      }
 
      if ((lib_dbg_cfg.uEventLogMode & LOG_MODE_MASK) != LOG_FILE_ONLY && !(loglevel & DS_LOG_LEVEL_FILE_ONLY)) {

#ifdef USE_NONBUFFERED_OUTPUT
         fprintf(stderr, "%s", log_string);
         if ((slen=strlen(log_string)) && log_string[slen-1] != '\n') fprintf(stderr, "\n");  /* add newline char if one not already there, JHB Sep2017 */
#else
      /* As with sig_print() in packet_flow_media_proc.c, we now use lib_dbg_cfg.uPrintfControl to determine screen output type, JHB Jan2020:

         -before stderr was always used, which uses non-buffered output (_IONBF)
         -non-buffered output is evidently more likely to block due to any type of I/O issue, for example intermittent internet connection in remote terminal usage
      */

         if ((slen = strlen(log_string))) {

            bool fNextLine = false;
            int thread_index = -1;

         /* make a reasonable effort to coordinate screen output between application threads and p/m threads, JHB Apr2020:

            -let application threads know when a p/m thread is printing to screen
            -atomic read/compare/write sets/clears isCursorMidLine to indicate cursor position is "start of line" or somewhere mid-line. Note this is a boolean because x86 __sync_bool_compare_and_swap only offers eq comparison and not ne
            -race conditions in determining when the cursor is mid-line can still occur, but they are greatly reduced
          */

            #ifndef DIAGLIB_STANDALONE
            if (IsPmThread(-1, &thread_index)) __sync_or_and_fetch(&pm_thread_printf, 1 << thread_index);  /* if this is a p/m thread, set corresponding bit in pm_thread_printf. IsPmThread() is in pktlib.h; if pktlib is not used then this call can be stubbed out in a placeholder .so to always return 0 */
            #endif

            if (!(loglevel & DS_LOG_LEVEL_IGNORE_LINE_CURSOR_POS) && __sync_val_compare_and_swap(&isCursorMidLine, 1, 0)) fNextLine = true;  /* fNextLine reflects leading \n decision */
            else if (log_string[slen-1] != '\n') __sync_val_compare_and_swap(&isCursorMidLine, 0, 1);

         /* screen output method:  0 = printf() (buffered, default), 1 = fprintf(stdout), 2 = fprintf(stderr), 3 = none */

            if (lib_dbg_cfg.uPrintfControl == 0) printf("%s%s", fNextLine ? "\n" : "", log_string);
            else if (lib_dbg_cfg.uPrintfControl == 1) fprintf(stdout, "%s%s", fNextLine ? "\n" : "", log_string);
            else if (lib_dbg_cfg.uPrintfControl == 2) fprintf(stderr,  "%s%s", fNextLine ? "\n" : "", log_string);

            uLineCursorPos = log_string[slen-1] != '\n' ? slen : 0;  /* update line cursor position */

            if (thread_index >= 0) __sync_and_and_fetch(&pm_thread_printf, ~(1 << thread_index));  /* clear corresponding p/m thread bit */
         }
#endif
      }
   }
}

void DSGetLogTimeStamp(char* timestamp, int max_str_len, unsigned int uFlags) {

time_t ltime;
struct tm tm;
struct timeval tv;
uint64_t usec = 0;
bool fWallClockTimestamp = (uFlags & DS_LOG_LEVEL_WALLCLOCK_TIMESTAMP) != 0;
bool fUptimeTimestamp = (uFlags & DS_LOG_LEVEL_UPTIME_TIMESTAMP) != 0;

   if (fWallClockTimestamp) { /* include wall clock timestamp if specified, JHB Feb2020 */
   
      ltime = time(NULL);
      localtime_r(&ltime, &tm);
      strftime(timestamp, max_str_len, "%m/%d/%Y %H:%M:%S", &tm);

      gettimeofday(&tv, NULL);
      usec = tv.tv_sec*1000000L + tv.tv_usec - usec_init;

      if (!fUptimeTimestamp) sprintf(&timestamp[strlen(timestamp)], ".%03d.%03d", (int)(usec/1000) % 1000, (int)(usec % 1000));  /* add msec and usec -- see uptime timestamp generation comments below */
      strcat(timestamp, " ");
   }
   
   if (fUptimeTimestamp) {  /* include uptime timestamp if specified. Note that wall clock and uptime timestamps can be combined (mediaMin interactive keyboard debug output does this), JHB Apr2020 */
   
      if (!fWallClockTimestamp) {  /* already done if wall clock timestamps were requested */
         timestamp[0] = '\0';
         gettimeofday(&tv, NULL);
         usec = tv.tv_sec*1000000L + tv.tv_usec - usec_init;
      }

   /* generate uptime (relative) timestamp:

      -hours, minutes, sec, msec, and usec. A high granularity is required to help measure and debug audio quality, stream alignment, and other timing issues
      -note for number of hours we don't use modulus to stay within a range.  When number of digits exceeds the %02 specifier, sprintf doesn't cut anything off (verified with 100+ hr stress tests, JHB Mar2019)
   */

      sprintf(&timestamp[strlen(timestamp)], "%s%02d:%02d:%02d.%03d.%03d%s ", fWallClockTimestamp ? "(" : "", (int)(usec/3600000000L), (int)(usec/60000000L) % 60, (int)(usec/1000000L) % 60, (int)(usec/1000) % 1000, (int)(usec % 1000), fWallClockTimestamp ? ")" : "");
   }
}
