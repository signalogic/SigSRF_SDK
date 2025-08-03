/*
 $Header: /root/Signalogic/DirectCore/lib/diaglib/event_logging.cpp
 
 Description: SigSRF and EdgeStream event logging APIs
 
 Project: SigSRF, DirectCore
 
 Copyright Signalogic Inc. 2017-2025

 Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Revision History
  
  Created Aug 2017 Chris Johnson
  Modified Sep 2017 JHB, moved Log_RT() here from pktlib_logging.c.  Moved declaration of pktlib_dbg_cfg here from pktlib.c (and renamed to lib_dbg_cfg).  Added DSGetAPIStatus() error/warning and function codes
  Modified Sep 2017 JHB, added more DS_API_CODE_xxx definitions
  Modified Jul 2018 CKJ, look at LOG_SCREEN, LOG_FILE, and LOG_SCREEN_FILE constants in Log_RT()
  Modified Oct 2018 JHB, add timestamps to Log_RT() output (see USE_TIMESTAMP define below).  Possibly this should be an option in the DEBUG_CONFIG struct (shared_include/config.h)
  Modified Jan 2019 JHB, remove 0..23 wrap from hours field of logging timestamp
  Modified Feb 2019 JHB, add code to look at LOG_SET_API_STATUS flag
  Modified Nov 2019 JHB, add va_end() to match va_start() in Log_RT().  Could not having then been cause of very slow mem leak ?
  Modified Dec 2019 JHB, add log file creation if lib_dbg_cfg.uEventLogFile is NULL, recreation if the file is deleted (possibly by an external process), uEventLog_fflush_size and uEventLog_max_size support
  Modified Jan 2020 JHB, use libg_dbg_cfg.uPrintfControl to determine screen output of Log_RT(), see comments near USE_NONBUFFERED_OUTPUT
  Modified Feb 2020 JHB, implement wall clock timestamp option
  Modified Mar 2020 JHB, implement DS_LOG_LEVEL_OUTPUT_FILE flag, handle leading newline (\n) in app supplied strings
  Modified Mar 2020 JHB, implement uLineCursorPos and isCursorMidLine in screen output handling; isCursorPosMidLine determines leading \n decisions. uLineCursorPos records cursor position within a line
  Modified Jan 2021 JHB, include string.h with _GNU_SOURCE defined, change loglevel param in Log_RT() from uint16_t to uint32_t, implement DS_LOG_LEVEL_SUBSITUTE_WEC flag (config.h). See comments
  Modified Mar 2021 JHB, minor adjustments to removal of unncessary Makefile defines, add STANDALONE #define option to build without isPmThread()
  Modified Dec 2022 JHB, add DSInitLogging(), DSUpdateLogConfig(), and DSCloseLogging() APIs to (i) simply interface from apps, and (ii) clarify multiprocess use of diaglib
  Modified Jan 2023 JHB, put in place new method of handling per-thread API status. See references to Logging_Thread_Info[] and comments
  Modified Jan 2023 JHB, in Log_RT(), replace strstr() with strcasestr(), avoid additional upper case copy to a temporary string
  Modified Jan 2023 JHB, make GetThreadIndex() not static, callable from diaglib.cpp
  Modified Jan 2023 JHB, add DSConfigLogging() to allow apps to abort DSPktStatsWriteLogFile() and other potentially time-consuming APIs if needed
  Modified Feb 2024 JHB, Makefile now defines NO_PKTLIB, NO_HWLIB, and STANDALONE if standalone=1 given on command line
  Modified Feb 2024 JHB, increase MAX_STR_SIZE and adjust max string size in vsnprintf() due to user-reported crash running pcap where one session had 20+ RFC8108 dynamic channel changes (evidently cell tower handoffs) and the run-time stats summary became very large
  Modified May 2024 JHB, convert to cpp, move DSGetLogTimestamp(), DSGetMD5Sum(), and DSGetBacktrace() APIs to diaglib_util.cpp
  Modified May 2024 JHB, remove references to NO_PKTLIB, NO_HWLIB, and STANDALONE. DSInitLogging() now uses dlsym() run-time checks for pktlib and hwlib APIs to eliminate need for a separate stand-alone version of diaglib. Makefile no longer recognizes standalone=1
  Modified Jun 2024 JHB, change last param in in DSConfigLogging() from void* to DEBUG_CONFIG*
  Modified Jun 2024 JHB, some var and struct renaming and comment updates to improve readability
  Modified Aug 2024 JHB, bump version number due to mods in diaglib.h and diaglib.cpp
  Modified Sep 2024 JHB, bump version number due to mods in diaglib.cpp
  Modified Sep 2024 JHB, implement DS_INIT_LOGGING_RESET_WARNINGS_ERRORS flag in DSInitLogging()
  Modified Nov 2024 JHB, implement harmonized LOG_XX definitions in diaglib.h and DS_LOG_LEVEL_OUTPUT_XXX flags in shared_include/config.h. See fOutputConsole and fOutputFile
  Modified Feb 2025 JHB, move isFileDeleted() to diaglib.h as static inline
  Modified Apr 2025 JHB, add isLinePreserve, change uLineCursorPos from uint8_t to unsigned int (to handle very long console display lines)
  Modified Apr 2025 JHB, in Log_RT() fixes and simplification to updating line cursor position, mid-line check, and isLinePreserve
  Modified Jul 2025 JHB, in Log_RT() add color printout for console warnings (yellow) and errors/critical errors (red)
  Modified Jul 2025 JHB, in Log_RT() call console_out(), which calls isStdoutReady() before printf() to avoid blocking if stdout has loss of connectivity
*/

/* Linux and/or other OS includes */

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE  /* strcasestr() */
#endif

#include <string.h>
#include <stdarg.h>  /* va_start(), va_end() */
#include <pthread.h>
#include <sys/time.h>  /* gettimeofday() */
#include <semaphore.h>
#include <stdio.h>
#include <stdbool.h>
#include <dlfcn.h>  /* dlsym(), RTLD_DEFAULT definition */
#include <algorithm>  /* bring in std::min and std::max */

using namespace std;

/* SigSRF includes */

#include "diaglib.h"

#define GET_PKT_INFO_TYPEDEF_ONLY  /* specify DSGetPacketInfo() typedef only (no prototype) in pktlib.h */
#include "pktlib.h"  /* pktlib header file, only constants and definitions used here */
#define GET_TIME_TYPEDEF_ONLY  /* specify get_time() typedef only (no prototype) in directcore.h */
#include "directcore.h"  /* DirectCore header file, only constants and definitions used here */

#include "shared_include/config.h"

/* private includes */

#include "diaglib_priv.h"

/* diaglib version string */
const char DIAGLIB_VERSION[256] = "1.9.9";

/* semaphores for thread safe logging init and close. Logging itself is lockless */

sem_t diaglib_sem;
int diaglib_sem_init = 0;

/* global vars */

static uint64_t last_size = 0;
static int app_log_file_count = 0;

DEBUG_CONFIG lib_dbg_cfg = { 5 };  /* moved here from pktlib.c, JHB Sep 2017.  Init to log level 5 for apps that don't make a DSConfigPktlib() call, JHB Jul 2019 */

/* referenced by p/m threads and apps to write/read current screen line cursor position. Read/written by Log_RT() (below), sig_printf() (called by p/m worker threads in packet_flow_media_proc.c), and app_printf() in user_io.cpp (mediaMin). Extern references are in mediaTest.h */
  
volatile unsigned int uLineCursorPos = 0;  /* size int to handle long lines */
volatile uint8_t isCursorMidLine = 0;      /* quick check for cursor position, set/cleared using __sync_val_compare_and_swap() */
volatile uint32_t pm_thread_printf = 0;
volatile uint8_t isLinePreserve = 0;

/* keep track of event log errors and warnings for program lifespan. The main purpose of this is to quickly show a general error summary in the run-time stats without having to search the event log. It's not that useful because it doesn't break down by session or stream (channel), JHB May 2020 */
  
uint32_t event_log_critical_errors = 0;
uint32_t event_log_errors = 0;
uint32_t event_log_warnings = 0;

/* private APIs and definitions */

/* function pointers set in DSInitLogging() with return value of dlsym(), which looks for run-time presence of SigSRF APIs. Note hidden attribute to make sure diaglib-local functions are not confused at link-time with their SigSRF library function counterparts if they both exist, JHB May 2024 */

__attribute__((visibility("hidden"))) DSGetPacketInfo_t* DSGetPacketInfo = NULL;  /* DSGetPacketInfo_t typedef in pktlib.h */
__attribute__((visibility("hidden"))) isPmThread_t* isPmThread = NULL;  /* isPmThread_t typedef in pktlib.h */
__attribute__((visibility("hidden"))) get_time_t* get_time = NULL;  /* get_time_t typedef in directcore.h */

/* create, get, and delete per-thread indexes for use in Logging_Thread_Info[nThread].xxx access, JHB Jan 2023:

  -pre-thread indexes are created by DSInitLogging() which calls private CreateThreadIndex(), and deleted by DSCloseLogging() which calls private DeleteThreadIndex(). Both use diaglib_sem to control multithread access
  -pktlib packet/media threads and mediaMin and mediaTest app threads call DSInitLogging() and DSCloseLogging()
  -the "zeroth" index is reserved for any applications or threads not calling DSInitLogging(); i.e. if GetThreadIndex() does not find a thread index, these share a thread index
*/

LOGGING_THREAD_INFO Logging_Thread_Info[MAXTHREADS] = {{ 0 }};  /* LOGGING_THREAD_INFO struct defined in diaglib_priv.h. Also referenced in diaglib.cpp */

static int CreateThreadIndex(void) {  /* create a thread index from its Id */

int i;

   for (i=1; i<MAXTHREADS; i++) if (Logging_Thread_Info[i].ThreadId == pthread_self()) return i;  /* first check to see if current thread already has a slot */

   for (i=1; i<MAXTHREADS; i++) if (Logging_Thread_Info[i].ThreadId == 0) {  /* get a new slot */

      Logging_Thread_Info[i].ThreadId = pthread_self();
//      printf("thread[%d] id = %llu \n", i, (long long unsigned int)Threads[i]);
      break;
   }

   if (i < MAXTHREADS) return i;
   else return -1;
}

__attribute__((visibility("hidden"))) int GetThreadIndex(bool fUseSem) {  /* Get a thread index from its Id. This is a diaglib-private API, also called by functions in diaglib.cpp */

int i, ret_val = 0;
bool fUseSemLocal = false;

   if (fUseSem && diaglib_sem_init) { fUseSemLocal = true; sem_wait(&diaglib_sem); }

   for (i=1; i<MAXTHREADS; i++) if (Logging_Thread_Info[i].ThreadId == pthread_self()) { ret_val = i; break; }

   if (fUseSemLocal) sem_post(&diaglib_sem);

   return ret_val;  /* return zeroth slot -- any/all apps or threads not calling DSInitLogging() */
}

static int DeleteThreadIndex(void) {  /* clear a thread index */

int nIndex = GetThreadIndex(false);  /* note - DeleteThreadIndex() called only from DSCloseLogging() which obtains the diaglib_sem semaphore */

   if (nIndex >= 0) { Logging_Thread_Info[nIndex].ThreadId = 0; return nIndex; }
   else return -1;
}


#if 0  /* use of problematic pthread_key_create() and pthread_setspecific() ripped out, new method in place; see Logging_Thread_Info[] and comments above, JHB Jan 2023 */
static void make_key()
{
   pthread_key_create(&status_key, NULL);
}
#endif

static int set_api_status(int status_code, unsigned int uFlags) {  /* set Logging_Thread_Info[].status_code, later retrieved by DSGetAPIStatus() */
#if 0
int *ptr;

   (void)uFlags;  /* avoid compiler warning (Makefile has -Wextra flag) */
   
   pthread_once(&key_once, make_key);
   
   /* if the value stored is NULL, this is the first time current thread is setting this value - allocate memory */
   if ((ptr = pthread_getspecific(status_key)) == NULL) ptr = (int *)malloc(sizeof(int));
   
   *ptr = status_code;
   pthread_setspecific(status_key, ptr);
   
   /* valgrind reports still reachable memory for the above malloc 
      I think pthread_setspecific() frees the pointer when it's set to a new value so another call with a NULL pointer would be needed to correctly cleanup all mallocs here -cj */
#else

int nIndex;

   (void)uFlags;  /* avoid compiler warning (Makefile has -Wextra flag) */

   nIndex = GetThreadIndex(false);
   if (nIndex >= 0) Logging_Thread_Info[nIndex].status_code = status_code;

   return nIndex;

#endif
}

static inline void strlcpy(char* dst, const char* src, int maxlen) {  /* implementation of strlcpy() since it's a BSD Linux function and evidently not available in gcc/g++, JHB Jan 2020 */

   int cpylen =  min((int)strlen(src), maxlen-1);
   memcpy(dst, src, cpylen);
   dst[cpylen] = (char)0;
}

static int open_log_file(bool fAllowAppend, bool fUseSem) {

bool fUseSemLocal = false;

   if (!lib_dbg_cfg.uEventLogFile && strlen(lib_dbg_cfg.szEventLogFilePath)) {  /* create event log file (or open it for appending), if not already done yet, JHB Dec 2019 */

      if (fUseSem && diaglib_sem_init) { fUseSemLocal = true; sem_wait(&diaglib_sem); }

      bool fAppend = (lib_dbg_cfg.uEventLogMode & DS_EVENT_LOG_APPEND) && fAllowAppend;

      lib_dbg_cfg.uEventLogFile = fopen(lib_dbg_cfg.szEventLogFilePath, fAppend ? "a" : "w");

      if (!lib_dbg_cfg.uEventLogFile) {  /* error condition */

         if (fUseSemLocal) sem_post(&diaglib_sem);

         fprintf(stderr, "ERROR: Log_RT() says unable to %s event log file %s, errno = %d \n", fAppend ? "open for appending" : "create", lib_dbg_cfg.szEventLogFilePath, errno);
         return -1;  /* < 0 indicates an error condition */
      }
      else {  /* successful create or append */

         #if 0
         struct stat stats;
         int fd = fileno(lib_dbg_cfg.uEventLogFile);
         static char buffer[BUFSIZ];

         if (fstat(fd, &stats) != -1) {
            int ret = setvbuf(lib_dbg_cfg.uEventLogFile, buffer, _IOFBF, BUFSIZ/*stats.st_blksize*/);
            printf("\n *** diaglib setvbuf ret = %d, BUFSIZ is %d, stats optimal block size is %ld, __fbufsize = %lu \n", ret, BUFSIZ, stats.st_blksize, __fbufsize(lib_dbg_cfg.uEventLogFile));
         }
         #endif

         app_log_file_count++;  /* increase app count */
      }

#if 0
      else {

         int fd = fileno(lib_dbg_cfg.uEventLogFile);  /* convert file pointer to file descriptor */
         int flags = fcntl(fd, F_GETFL);
         fcntl(fd, F_SETFL, flags | O_NONBLOCK);
      }
#endif
//      else fwrite("\xEF\xBB\xBF", 3, 1, lib_dbg_cfg.uEventLogFile);  /* set BOM to UTF-8 */

      if (fUseSemLocal) sem_post(&diaglib_sem);

      return 1;  /* 1 indicates event log file successfully opened */
   }

   return 0;  /* 0 indicates event log file aleady open */
}

static int update_log_config(DEBUG_CONFIG* dbg_cfg, unsigned uFlags, bool fUseSem) {  /* called by DSConfigLogging() below */

bool fUseSemLocal = false;

   (void)uFlags;

   if (!dbg_cfg) return -1;

   if (fUseSem && diaglib_sem_init) { fUseSemLocal = true; sem_wait(&diaglib_sem); }

   if (!lib_dbg_cfg.uEventLogFile) lib_dbg_cfg.uEventLogFile = dbg_cfg->uEventLogFile;  /* don't initialize event log file handle if for any reason it's already set */
   if (dbg_cfg->szEventLogFilePath) strcpy(lib_dbg_cfg.szEventLogFilePath, dbg_cfg->szEventLogFilePath);

   lib_dbg_cfg.uEventLogMode = dbg_cfg->uEventLogMode;
   lib_dbg_cfg.uLogLevel = dbg_cfg->uLogLevel;
   lib_dbg_cfg.uEventLog_fflush_size = dbg_cfg->uEventLog_fflush_size;
   lib_dbg_cfg.uEventLog_max_size = dbg_cfg->uEventLog_max_size;
   lib_dbg_cfg.uPrintfControl = dbg_cfg->uPrintfControl;
   lib_dbg_cfg.uDisableMismatchLog = dbg_cfg->uDisableMismatchLog;
   lib_dbg_cfg.uDisableConvertFsLog = dbg_cfg->uDisableConvertFsLog;
   lib_dbg_cfg.uPrintfLevel = dbg_cfg->uPrintfLevel;

   if (fUseSemLocal) sem_post(&diaglib_sem);

   return 1;
}

/* public APIs */

int DSGetAPIStatus(unsigned int uFlags) {  /* per-thread API status */

   (void)uFlags;  /* avoid compiler warning (Makefile has -Wextra flag) */
   
#if 0  /* use of problematic pthread_key_create() and pthread_setspecific() ripped out, new method in place; see Logging_Thread_Info[] and comments above, JHB Jan 2023 */
int *ptr = pthread_getspecific(status_key);

/* if no value has been set, ptr will be NULL and we return 0 indicating no status */

   if (ptr != NULL) return *ptr;
   else return 0;
#else

int nIndex = GetThreadIndex(false);

   if (nIndex >= 0) return Logging_Thread_Info[nIndex].status_code;  /* return current API status code */
   else return -1;

#endif
}

FILE* DSGetEventLogFileHandle(unsigned int uFlags) {

   (void)uFlags;

   return lib_dbg_cfg.uEventLogFile;
}

/* initialize event logging */

int DSInitLogging(DEBUG_CONFIG* dbg_cfg, unsigned int uFlags) {

int ret_val;
static uint8_t lock = 0;

/* set a memory barrier, prevent multiple uncoordinated threads from initializing the semaphore more than once, JHB Jan 2023 */

   while (__sync_lock_test_and_set(&lock, 1) != 0);  /* wait until the lock is zero then write 1 to it. While waiting keep writing a 1 */

   if (!diaglib_sem_init) {

      if (sem_init(&diaglib_sem, 0, 1)) {

         fprintf(stderr, "CRITICAL: failed to initialize diaglib semaphore: %s %s:%d \n", strerror(errno), __FILE__, __LINE__);
         __sync_lock_release(&lock);
         return -1;
      }

   /* one time symbol lookup for pktlib and hwlib functions, JHB May 2024 */

      #pragma GCC diagnostic push
      #if (_GCC_VERSION >= 50000)
      #pragma GCC diagnostic ignored "-Wpedantic"  /* correct syntax is "Wpedantic", but evidently gcc 4.9 and lower only recognize "pedantic", JHB Dec 2023 */
      #else
      #pragma GCC diagnostic ignored "-pedantic"
      #endif
      void* temp = dlsym(RTLD_DEFAULT, "DSGetPacketInfo");  /* use dlsym() to see if DSGetPacketInfo() function exists in the build, JHB May 2024 */
      memcpy(&DSGetPacketInfo, &temp, sizeof(void*));

      temp = dlsym(RTLD_DEFAULT, "isPmThread");  /* look for isPmThread() in the build, JHB May 2024 */
      memcpy(&isPmThread, &temp, sizeof(void*));

      temp = dlsym(RTLD_DEFAULT, "get_time");  /* look for get_time() in the build, JHB May 2024 */
      memcpy(&get_time, &temp, sizeof(void*));
      #pragma GCC diagnostic pop

      #if 0
      printf("DSGetPacketInfo func ptr = %p, isPmThread func ptr = %p, get_time func ptr = %p \n", DSGetPacketInfo, isPmThread, get_time);
      #endif

      diaglib_sem_init = 1;
   }

   __sync_lock_release(&lock);  /* clear the mem barrier (write 0 to the lock) */

/* Init global lib_dbg_cfg. Notes:

   -purpose is early set up of logging relevant section of lib_dbg_cfg, allowing logging to be up and running before anything else, including library init's

   -multithreaded operation:
     -we're using a semaphore inside init and close (DSCloseLogging)
     -each process will use a different semaphore, and presumably a different event log filename
     -for multithreaded apps (like mediaMin), it's expected the app has a master thread handling logging (i.e. only one of its threads). If not then app_log_file_count increments for each thread when calling DSInitLogging() and decrements when calling DSCloseLogging()

   -DSConfigPktlib() and DSconfigVoplib() may also init lib_dbg_cfg, but that should be an overwrite with same data if DSInitLogging() has already been called with the same dbg_cfg (or vice versa). Note that in both cases the file handle is not overwritten if already opened
*/

   sem_wait(&diaglib_sem);

   CreateThreadIndex();  /* get a slot for current thread (if it doesn't already exist) */

   if (dbg_cfg) update_log_config(dbg_cfg, uFlags, false);  /* note that DSInitLogging() can be called with a NULL dbg_cfg, for example another thread has already set the defaults. So far update_log_config() doesn't use uFlags, but possibly DS_CONFIG_LOGGING_xx flags or other flags could be used here in the future */

   bool fLogFileRelatedFlags = (uFlags != 0) ? true : false;

   if (uFlags & DS_INIT_LOGGING_RESET_WARNINGS_ERRORS) {  /* reset warning and error counters if specified, JHB Sep 2024 */

      event_log_warnings = 0;
      event_log_errors = 0;
      event_log_critical_errors = 0;
      fLogFileRelatedFlags = false;
   }

   if (uFlags & DS_INIT_LOGGING_ENABLE_STDOUT_READY_PROFILING) fEnableStdoutReadyProfiling = true;

   if (!dbg_cfg && !fLogFileRelatedFlags) ret_val = diaglib_sem_init == 2 ? 1 : 0;  /* handle initialization status request (dbg_cfg NULL and uFlags zero):  if any thread has made it past this point then at least one full initialization has happened */
   else ret_val = open_log_file(true, false);

   diaglib_sem_init = 2;  /* set to fully initialized */

   sem_post(&diaglib_sem);

   return ret_val;
}

/* DSConfigLogging - set/get Logging_Thread_Info[] items. We use the thread based indexes set by DSInitLogging() */

unsigned int DSConfigLogging(unsigned int action, unsigned int uFlags, DEBUG_CONFIG* pDebugConfig) {

unsigned int ret_val = (unsigned int)-1;
int i, nIndex, start, end;
bool fUseSem = false;

   if (uFlags & DS_CONFIG_LOGGING_ALL_THREADS) { start = 0; end = MAXTHREADS-1; }
   else {

      nIndex = GetThreadIndex(true);  /* get Logging_Thread_Info[] index for the current thread. Ask GetThreadIndex() to obtain the diaglib_sem semaphore */
      if (nIndex < 0) return (unsigned int)-1;
 
      start = nIndex; end = nIndex;
   }

   if (diaglib_sem_init) { fUseSem = true; sem_wait(&diaglib_sem); }

   for (i=start; i<=end; i++) {

      switch (action & DS_CONFIG_LOGGING_ACTION_MASK) {

         case DS_CONFIG_LOGGING_ACTION_SET_FLAG:
            ret_val = Logging_Thread_Info[i].uFlags;
            Logging_Thread_Info[i].uFlags |= uFlags;
            break;

         case DS_CONFIG_LOGGING_ACTION_CLEAR_FLAG:
            ret_val = Logging_Thread_Info[i].uFlags;
            Logging_Thread_Info[i].uFlags &= ~uFlags;
            break;

         case DS_CONFIG_LOGGING_ACTION_SET_UFLAGS:
            ret_val = Logging_Thread_Info[i].uFlags;
            Logging_Thread_Info[i].uFlags = uFlags;
            break;

         case DS_CONFIG_LOGGING_ACTION_GET_UFLAGS:
            ret_val = Logging_Thread_Info[i].uFlags;
            break;

         case DS_CONFIG_LOGGING_ACTION_SET_DEBUG_CONFIG:
            ret_val = update_log_config(pDebugConfig, uFlags, false);  /* update event log configuration dynamically */
            break;
      }
   }

   if (fUseSem) sem_post(&diaglib_sem);

   return ret_val;
}

int DSCloseLogging(unsigned int uFlags) {

   (void)uFlags;

   sem_wait(&diaglib_sem);

   if (lib_dbg_cfg.uEventLogFile && --app_log_file_count == 0) {

      fclose(lib_dbg_cfg.uEventLogFile);
      lib_dbg_cfg.uEventLogFile = NULL;
   }

   DeleteThreadIndex();  /* clear slot belonging to current thread */

   sem_post(&diaglib_sem);

   return app_log_file_count;
}

#define ERROR_PARSE_LOGMSG
#define MAX_STR_SIZE 8000  /* increased from 4000, JHB Feb 2024 */
#define MAX_ERRSTR_SIZE 200

uint64_t usec_base = 0;
uint8_t usec_init_lock = 0;

int Log_RT(uint32_t loglevel, const char* fmt, ...) {

va_list va;
char log_string[MAX_STR_SIZE];
int slen = 0, fmt_start = 0;

   if (lib_dbg_cfg.uEventLogMode & DS_EVENT_LOG_DISABLE) return 0;  /* event log is (temporarily) disabled */

   if ((lib_dbg_cfg.uEventLogMode & DS_EVENT_LOG_WARN_ERROR_ONLY) && (loglevel & DS_LOG_LEVEL_MASK) > 3) return 0;  /* event log warn and error output only (temporarily) */

/* set a memory barrier, prevent multiple uncoordinated threads from initializing usec_base more than once. Note this same lock also protects usec_base initialization in DSGetLogTimestamp() (in diaglib_util.cpp), JHB May 2024 */

   while (__sync_lock_test_and_set(&usec_init_lock, 1) != 0);  /* wait until the lock is zero then write 1 to it. While waiting keep writing a 1 */

   if (!usec_base) {  /* initialize usec_base if needed. Log_RT() makes the same check, JHB May 2024 */

      struct timeval tv;
      gettimeofday(&tv, NULL);
      usec_base = tv.tv_sec*1000000L + tv.tv_usec;
   }

   __sync_lock_release(&usec_init_lock);  /* clear the mem barrier (write 0 to the lock) */

   log_string[0] = (char)0;  /* ensure strlen(log_string) is zero */

   if (loglevel & DS_LOG_LEVEL_APPEND_STRING) {

      char* p_fmt_start = strstr((char*)fmt, "%s");

      if (p_fmt_start) {
         fmt_start = p_fmt_start - fmt;
         strncpy(log_string, fmt, fmt_start);
         log_string[fmt_start] = '\0';
      }
   }
  
   if ((loglevel & DS_LOG_LEVEL_MASK) < lib_dbg_cfg.uLogLevel) {

   /* get timestamp */

      if (!(loglevel & DS_LOG_LEVEL_NO_TIMESTAMP)) {

         if (DSGetLogTimestamp(&log_string[fmt_start], (unsigned int)lib_dbg_cfg.uEventLogMode, sizeof(log_string), 0)) strcat(log_string, " ");  /* DSGetLogTimestamp() returns length of timestamp, JHB Apr 2024 */
      }

      int ts_str_len = strlen(log_string);  /* zero or length of timestamp */

   /* add and format Log_RT() string */

      va_start(va, fmt);

      #if 0
      vsnprintf(&log_string[ts_str_len], sizeof(log_string) - ts_str_len, &fmt[fmt_start], va);
      #else
      vsnprintf(&log_string[ts_str_len], sizeof(log_string) - ts_str_len - 2, &fmt[fmt_start], va);  /* adjust string length limit, JHB Feb 2024 */
      #endif

      va_end(va);  /* added JHB Nov 2019 */

   /* leading newline handling: if first char in user string is a newline, move the newline to be in front of the timestamp, JHB Mar 2020 */

      if (log_string[ts_str_len] == '\n') {
         int i;
         for (i=ts_str_len; i>0; i--) log_string[i] = log_string[i-1];  /* this is a rare thing, so ok to do a slow move here */
         log_string[0] = '\n';
      }

   /* add newline if one not already there, JHB Sep2017. Made optional with DS_EVENT_LOG_DONT_ADD_NEWLINE flag, JHB Apr 2020 */

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

         if ((loglevel & DS_LOG_LEVEL_MASK) < 4) {  /* check only 0-3 for error/warning, JHB Jan 2020 */

            int status_code = 0;

            if (strcasestr(log_string, "ERROR") || strcasestr(log_string, "CRITICAL")) status_code |= DS_API_STATUS_CODE_ERROR;
            if (strcasestr(log_string, "WARNING")) status_code |= DS_API_STATUS_CODE_WARNING;

         /* published APIs */

            if (strcasestr(log_string, "DSCREATESESSION")) status_code |= DS_API_CODE_CREATESESSION;
            else if (strcasestr(log_string, "DSDELETESESSION")) status_code |= DS_API_CODE_DELETESESSION;
            else if (strcasestr(log_string, "DSBUFFERPACKETS")) status_code |= DS_API_CODE_BUFFERPKTS;
            else if (strcasestr(log_string, "DSGETORDEREDPACKETS")) status_code |= DS_API_CODE_GETORDEREDPKTS;
            else if (strcasestr(log_string, "DSGETPACKETINFO")) status_code |= DS_API_CODE_GETPACKETINFO;
            else if (strcasestr(log_string, "DSGETSESSININFO")) status_code |= DS_API_CODE_GETSESSIONINFO;
            else if (strcasestr(log_string, "DSGETDTMFINFO")) status_code |= DS_API_CODE_GETDTMFINFO;
            else if (strcasestr(log_string, "DSFORMATPACKET")) status_code |= DS_API_CODE_FORMATPACKET;
            else if (strcasestr(log_string, "DSSTORESTREAMDATA")) status_code |= DS_API_CODE_STORESTREAMDATA;
            else if (strcasestr(log_string, "DSGETSTREAMDATA")) status_code |= DS_API_CODE_GETSTREAMDATA;

         /* internal APIs, note these may be combined with published APIs */

            if (strcasestr(log_string, "VALIDATE_RTP")) status_code |= DS_API_CODE_VALIDATERTP;
            else if (strcasestr(log_string, "GET_CHAN_PACKETS")) status_code |= DS_API_CODE_GETCHANPACKETS;
            else if (strcasestr(log_string, "CREATE_DYNAMIC_CHAN")) status_code |= DS_API_CODE_CREATEDYNAMICCHAN;

            if (status_code) set_api_status(status_code, 0);  /* set API status for current thread */
         } 
      }
#endif  /* ERROR_PARSE_LOGMSG */

   /* implement updated flags in shared_include/config.h and diaglib.h to control output to console and/or event log file, JHB Nov 2024 */

      bool fOutputConsole = lib_dbg_cfg.uEventLogMode & LOG_CONSOLE;  /* LOG_CONSOLE_FILE flag in diaglib.h will set both */
      bool fOutputFile = lib_dbg_cfg.uEventLogMode & LOG_FILE;

   /* check for overrides in loglevel */

      if ((loglevel & DS_LOG_LEVEL_OUTPUT_FILE) && (loglevel & DS_LOG_LEVEL_OUTPUT_CONSOLE)) { fOutputFile = true; fOutputConsole = true; }  /* DS_LOG_LEVEL_OUTPUT_FILE_CONSOLE flag in config.h will set both */
      else if (loglevel & DS_LOG_LEVEL_OUTPUT_FILE) { fOutputFile = true; fOutputConsole = false; }
      else if (loglevel & DS_LOG_LEVEL_OUTPUT_CONSOLE) { fOutputFile = false; fOutputConsole = true; }
 
      if (fOutputFile) {

         bool fAllowAppend = true;
         bool fUseSem = true;

create_log_file_if_needed:

         open_log_file(fAllowAppend, fUseSem);

         if (lib_dbg_cfg.uEventLogFile) {

         /* check for WEC substitution, notes:  JHB Jan 2021

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
               slen = strlen(log_string_copy);

               do {  /* loop to handle all occurrences, always check to make sure we're not expanding the string past mem limits */

                  p = strcasestr(log_string_copy, "warning");
                  if (!p) p = strcasestr(log_string_copy, "error");
                  if (!p) p = strcasestr(log_string_copy, "critical");

                  if (p && slen+1 < MAX_STR_SIZE) { memmove(p+2, p+1, strlen(p+1)+1); *(p+1) = '|'; slen++; }

               } while (p);

               p = log_string_copy;
            }

            int ret_val = fwrite(p, strlen(p), 1, lib_dbg_cfg.uEventLogFile);

            if (ret_val != 1) {  /* we call fwrite() with number of "elements" = 1, not bytes */

               fprintf(stderr, "\nERROR: Log_RT() says not able to write to event log file %s, errno = %d \n", lib_dbg_cfg.szEventLogFilePath, errno);
            }
            else if (isFileDeleted(lib_dbg_cfg.uEventLogFile)) {  /* fwrite() won't show an error if file has been deleted, so we always call isFileDeleted() which calls fstat(), JHB Jan 2023 */

               fprintf(stderr, "\nERROR: Log_RT() says event log file %s may have been deleted, errno = %d, attempting to recreate file ... \n", lib_dbg_cfg.szEventLogFilePath, errno);
               lib_dbg_cfg.uEventLogFile = NULL;
               fAllowAppend = false;
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
 
      if (fOutputConsole) {

#ifdef USE_NONBUFFERED_OUTPUT
         fprintf(stderr, "%s", log_string);
         if ((slen=strlen(log_string)) && log_string[slen-1] != '\n') fprintf(stderr, "\n");  /* add newline char if one not already there, JHB Sep2017 */
#else
      /* As with sig_print() in packet_flow_media_proc.c, we now use lib_dbg_cfg.uPrintfControl to determine screen output type, JHB Jan 2020:

         -before stderr was always used, which uses non-buffered output (_IONBF)
         -non-buffered output is evidently more likely to block due to any type of I/O issue, for example intermittent internet connection in remote terminal usage
      */

         if ((slen = strlen(log_string))) {

            bool fNextLine = false;
            int thread_index = -1;

         /* make a reasonable effort to coordinate screen output between application threads and p/m threads, JHB Apr 2020:

            -let application threads know when a p/m thread is printing to screen
            -atomic read/compare/write sets/clears isCursorMidLine to indicate cursor position is "start of line" or somewhere mid-line. Note this is a boolean because x86 __sync_bool_compare_and_swap only offers eq comparison and not ne
            -race conditions in determining when the cursor is mid-line can still occur, but they are greatly reduced
          */

            if (isPmThread && isPmThread(-1, &thread_index)) __sync_or_and_fetch(&pm_thread_printf, 1 << thread_index);  /* if this is a packet/media thread, set corresponding bit in pm_thread_printf. isPmThread() is defined in pktlib.h, note that DSInitLogging() checks to see if pktlib.so is in the build, and if not sets isPMThread to NULL */

            #if 0
            if (!(loglevel & DS_LOG_LEVEL_IGNORE_LINE_CURSOR_POS) && __sync_val_compare_and_swap(&isCursorMidLine, 1, 0)) fNextLine = true;  /* fNextLine reflects leading \n decision. If semaphore is 1 (cursor currently at midline) then set to 0 (cursor is at start-of-line) and set fNextLine. If already 1 ignore. Note there is a tad bit of race condition here, but cursor control is not a critical thing; if we get it right 99% of the time that's fine */
            else if (slen && (log_string[slen-1] != '\n' && log_string[slen-1] != '\r')) __sync_val_compare_and_swap(&isCursorMidLine, 0, 1);  /* if line has no end-of-line, and cursor is at start-of-line, then set to 1 (midline). Add checks for slen  > 0 and \r, JHB Apr 2025 */
            #else

            if (!(loglevel & DS_LOG_LEVEL_IGNORE_LINE_CURSOR_POS) && isCursorMidLine) fNextLine = true;
            #endif

         /* update line cursor position, mid-line quick check, and isLinePreserve */

            uLineCursorPos = (slen && (log_string[slen-1] == '\n' || log_string[slen-1] == '\r')) ? 0 : (log_string[0] == '\r' ? 0 : uLineCursorPos) + slen;  /* modify to include checks for slen > 0 and \r, fix calculation as pos + len instead of just len, JHB Apr 2025 */
            __sync_val_compare_and_swap(&isCursorMidLine, uLineCursorPos == 0, uLineCursorPos != 0);  /* if uLineCursorPos > 0 set to 1, otherwise set to 0 */
            isLinePreserve = false;

            console_out(lib_dbg_cfg.uPrintfControl, loglevel, fNextLine, log_string);  /* write to console: type of output, log level, optional leading next line (console_out() is in diaglib_util.cpp), JHB Jul 2025 */
 
            if (thread_index >= 0) __sync_and_and_fetch(&pm_thread_printf, ~(1 << thread_index));  /* clear corresponding p/m thread bit */
         }
#endif
      }
   }

   return slen;
}
