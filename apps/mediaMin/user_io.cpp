/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/user_io.cpp

 Copyright (C) Signalogic Inc. 2021

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

 Description

  User I/O related source for mediaMin reference application

 Documentation

  ftp://ftp.signalogic.com/documentation/SigSRF

  (as of Oct 2019, most recent doc is ftp://ftp.signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

 Revision History

   Created Apr 2021 JHB, split off from mediaMin.cpp
*/

#include <stdio.h>
#include <stdarg.h>

using namespace std;

#include "diaglib.h"    /* bring in Log_RT() definition */
#include "pktlib.h"

#include "shared_include/session.h"    /* session management structs and definitions */
#include "shared_include/config.h"     /* configuration structs and definitions */

#include "mediaTest.h"  /* bring in constants needed by mediaMin.h */

#include "mediaMin.h"
#include "user_io.h"

extern unsigned int num_app_threads;   /* see comments in mediaMin.h */
extern THREAD_INFO thread_info[MAX_MEDIAMIN_THREADS];  /* THREAD_INFO struct defined in mediaMin.h, MAX_MEDIAMIN_THREADS defined in mediaTest.h */

extern bool fQuit;         /* set if 'q' (quit) key is pressed */
extern bool fPause;        /* "" 'p' (pause).  Pauses operation, another 'p' resumes.  Can be combined with 'd' (display) key to read out internal p/m thread debug, capacity, stats, and other info */ 
extern bool fStop;         /* "" 's' (stop).  Stop prior to next repeat (only applies if -RN is entered on cmd line.  Intended for clean stop to repeating test, avoiding partial output files, especially when ENABLE_RANDOM_WAIT is active */

extern int num_pktmed_threads;    /* number of packet/media threads running */
extern bool fRepeatIndefinitely;  /* true if -R0 is given on the cmd line */
extern int nRepeatsRemaining[MAX_MEDIAMIN_THREADS];


/* update screen counters */

void UpdateCounters(uint64_t cur_time, int thread_index) {

char tmpstr[MAX_APP_STR_LEN] = "";
static uint64_t last_time[MAX_PKTMEDIA_THREADS] = { 0 };

   if (last_time[thread_index] == 0) last_time[thread_index] = cur_time;
   if ((int64_t)cur_time - (int64_t)last_time[thread_index] <= 100*1000) return;  /* update counters no faster than 100 msec */

   last_time[thread_index] = cur_time;

   if (thread_info[thread_index].pkt_push_ctr != thread_info[thread_index].prev_pkt_push_ctr || thread_info[thread_index].pkt_pull_jb_ctr != thread_info[thread_index].prev_pkt_pull_jb_ctr || thread_info[thread_index].pkt_pull_xcode_ctr != thread_info[thread_index].prev_pkt_pull_xcode_ctr || thread_info[thread_index].pkt_pull_streamgroup_ctr != thread_info[thread_index].prev_pkt_pull_streamgroup_ctr) {

      if (thread_info[thread_index].pkt_pull_jb_ctr >= 100000L) sprintf(tmpstr, "\rPsh %d, pul %d", thread_info[thread_index].pkt_push_ctr, thread_info[thread_index].pkt_pull_jb_ctr);
      else sprintf(tmpstr, "\rPushed pkts %d, pulled pkts %d", thread_info[thread_index].pkt_push_ctr, thread_info[thread_index].pkt_pull_jb_ctr);
      if (thread_info[thread_index].pkt_pull_xcode_ctr || thread_info[thread_index].pkt_pull_streamgroup_ctr) sprintf(&tmpstr[strlen(tmpstr)], "j");
      if (thread_info[thread_index].pkt_pull_xcode_ctr) sprintf(&tmpstr[strlen(tmpstr)], " %dx", thread_info[thread_index].pkt_pull_xcode_ctr);
      if (thread_info[thread_index].pkt_pull_streamgroup_ctr) sprintf(&tmpstr[strlen(tmpstr)], " %ds", thread_info[thread_index].pkt_pull_streamgroup_ctr);

      thread_info[thread_index].prev_pkt_push_ctr = thread_info[thread_index].pkt_push_ctr;
      thread_info[thread_index].prev_pkt_pull_jb_ctr = thread_info[thread_index].pkt_pull_jb_ctr;
      thread_info[thread_index].prev_pkt_pull_xcode_ctr = thread_info[thread_index].pkt_pull_xcode_ctr;
      thread_info[thread_index].prev_pkt_pull_streamgroup_ctr = thread_info[thread_index].pkt_pull_streamgroup_ctr;
   }

   if (strlen(tmpstr)) app_printf(APP_PRINTF_SAMELINE | APP_PRINTF_THREAD_INDEX_SUFFIX, thread_index, tmpstr);  /* use fully buffered I/O; i.e. not stdout (line buffered) or stderr (per character) */
}


/* process interactive keyboard input */

bool ProcessKeys(HSESSION hSessions[], uint64_t cur_time, DEBUG_CONFIG* dbg_cfg, int thread_index) {

char key;
static int app_thread_index_debug = 0;
static int pm_thread_index_debug = 0;
int i;
char tmpstr[500] = "";
PACKETMEDIATHREADINFO PacketMediaThreadInfo;
static uint64_t last_time = 0;
static uint8_t save_uPrintfLevel = 0;

   if (isMasterThread) {  /* master application threads (thread_index = 0) thread handles interactive keyboard commands */

      if (last_time == 0) last_time = cur_time;
      if ((int64_t)cur_time - (int64_t)last_time < 100*1000 && !fPause) return false;  /* check keys every 100 msec. Make an exception for pause key, otherwise we never get out of pause */

      last_time = cur_time;

      key = (char)tolower(getkey());

      if (key == 'q' || run <= 0) {  /* quit key, Ctrl-C, or p/m thread error condition */

         strcpy(tmpstr, "#### ");
         if (key == 'q') sprintf(&tmpstr[strlen(tmpstr)], "q key entered");
         else if (run == 0) sprintf(&tmpstr[strlen(tmpstr)], "Ctrl-C entered");
         else if (run < 0) sprintf(&tmpstr[strlen(tmpstr)], "p/m thread error and abort condition"); 
         sprintf(&tmpstr[strlen(tmpstr)], ", exiting mediaMin");
         app_printf(APP_PRINTF_NEWLINE, thread_index, tmpstr);

         fQuit = true;
         return true;
      }

      if (key == 's') fStop = true;  /* graceful stop, not the same as quit. In a graceful stop each app thread stops after it reaches the end of its inputs, flushes sessions, etc, and does not repeat */

      if (key == 'p') fPause ^= 1;  /* pause */

      if (key == 'o') {  /* toggle p/m thread screen output off/on. Applies to all active p/m threads */

         if (dbg_cfg->uPrintfLevel != 0) {

            save_uPrintfLevel = dbg_cfg->uPrintfLevel;
            dbg_cfg->uPrintfLevel = 0;
         }
         else dbg_cfg->uPrintfLevel = save_uPrintfLevel;

         DSConfigPktlib(NULL, dbg_cfg, DS_CP_DEBUGCONFIG);
      }

      if (key >= '0' && key <= '9') {

         pm_thread_index_debug = key - '0';  /* select a packet/media thread for debug output (subsequent 'd' input) */
         if (pm_thread_index_debug >= num_pktmed_threads) pm_thread_index_debug = num_pktmed_threads-1;
      }

      bool fDisp = false;

      if (key == '-') {
         app_thread_index_debug--;
         if (app_thread_index_debug < 0) app_thread_index_debug = (int)(num_app_threads-1);
         fDisp = true;
      }

      if (key == '+') {
         app_thread_index_debug++;
         if (app_thread_index_debug == (int)num_app_threads) app_thread_index_debug = 0;
         fDisp = true;
      }

      if (key == 'd' || fDisp) {  /* display debug output */

         DSGetLogTimeStamp(tmpstr, sizeof(tmpstr), DS_LOG_LEVEL_WALLCLOCK_TIMESTAMP | DS_LOG_LEVEL_UPTIME_TIMESTAMP);

         char repeatstr[50];
         if (!fRepeatIndefinitely && nRepeatsRemaining[thread_index] >= 0) sprintf(repeatstr, ", repeats remaining = %d", nRepeatsRemaining[thread_index]);  /* if cmd line entry includes -RN with N >= 0, nRepeatsRemaining will be > 0 for repeat operation, JHB Jan2020 */
         else if (nRepeatsRemaining[thread_index] == -1) strcpy(repeatstr, ", no repeats");  /* nRepeat is -1 if cmd line has no -RN entry (no repeats). For cmd line entry -R0, fRepeatIndefinitely will be set */

         printf("%s#### (App Thread) %sDebug info for app thread %d, run = %d%s \n", uLineCursorPos ? "\n" : "", tmpstr, app_thread_index_debug, run, fRepeatIndefinitely ? ", repeating indefinitely" : repeatstr);

         strcpy(tmpstr, "");
         for (i=0; i<thread_info[app_thread_index_debug].nSessionsCreated; i++) sprintf(&tmpstr[strlen(tmpstr)], " %d", thread_info[app_thread_index_debug].flush_state[i]);
         printf("flush state =%s, flush_count = %d, nSessionsCreated = %d, push cnt = %d, jb pull cnt = %d, xcode pull cnt = %d \n", tmpstr, thread_info[app_thread_index_debug].flush_count, thread_info[app_thread_index_debug].nSessionsCreated, thread_info[app_thread_index_debug].pkt_push_ctr, thread_info[app_thread_index_debug].pkt_pull_jb_ctr, thread_info[app_thread_index_debug].pkt_pull_xcode_ctr);

         if (hSessions) {

            sprintf(tmpstr, "push queue check =");
            for (i=0; i<thread_info[app_thread_index_debug].nSessionsCreated; i++) {
               if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) sprintf(&tmpstr[strlen(tmpstr)], " %d", DSPushPackets(DS_PUSHPACKETS_GET_QUEUE_STATUS, NULL, NULL, &hSessions[i], 1));
            }

            sprintf(&tmpstr[strlen(tmpstr)], ", pull queue check =");
            for (i=0; i<thread_info[app_thread_index_debug].nSessionsCreated; i++) {
               if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) sprintf(&tmpstr[strlen(tmpstr)], " %d", DSPullPackets(DS_PULLPACKETS_GET_QUEUE_STATUS | DS_PULLPACKETS_TRANSCODED | DS_PULLPACKETS_JITTER_BUFFER, NULL, NULL, hSessions[i], NULL, 0, 0));
            }

            sprintf(&tmpstr[strlen(tmpstr)], ", pcap input check =");
            for (i=0; i<thread_info[app_thread_index_debug].nInPcapFiles; i++) {
               sprintf(&tmpstr[strlen(tmpstr)], " %d", thread_info[app_thread_index_debug].pcap_in[i] != NULL);
            }
 
            printf("%s \n", tmpstr);

#if 0  /* deprecated, don't use this method */
            run = 2;
#else  /* ask for run-time debug output from one or more packet / media threads */
            uint64_t uThreadList = 1UL << pm_thread_index_debug;  /* uThreadList is a bitwise list of threads to display.  In this example only one bit is set */
            DSDisplayThreadDebugInfo(uThreadList, DS_DISPLAY_THREAD_DEBUG_INFO_SCREEN_OUTPUT, "#### (PM Thread) ");  /* display run-time debug info for one or more packet/media threads.  Note that DS_DISPLAY_THREAD_DEBUG_INFO_EVENT_LOG_OUTPUT could also be used to print to the event log */
#endif
         }
      }

      if (key == 't') {  /* print packet/media thread info, some of which is redundant with the 'd' command above. This is mainly an example of using the DSGetThreadInfo() API. The PACKETMEDIATHREADINFO struct is defined in pktlib.h */

         DSGetThreadInfo(pm_thread_index_debug, 0, &PacketMediaThreadInfo);
         printf("\n##### debug info for packet/media thread %d \n", pm_thread_index_debug);
         printf("thread id = 0x%llx, uFlags = 0x%x, niceness = %d, max inactivity time (sec) = %d\n", (unsigned long long)PacketMediaThreadInfo.threadid, PacketMediaThreadInfo.uFlags, PacketMediaThreadInfo.niceness, (int)(PacketMediaThreadInfo.max_inactivity_time/1000000L));

         int num_counted = 0;
         uint64_t cpu_time_sum = 0;

         for (i=0; i<THREAD_STATS_TIME_MOVING_AVG; i++) {

            if (PacketMediaThreadInfo.CPU_time_avg[i] > 1000) {

               cpu_time_sum += PacketMediaThreadInfo.CPU_time_avg[i];
               num_counted++;
            }
         }

         printf("CPU time (msec): avg %2.2f, max %2.2f\n", 1.0*cpu_time_sum/max(num_counted, 1)/1000, 1.0*PacketMediaThreadInfo.CPU_time_max/1000);
      }

      if (key == 'z') {  /* do not use -- reserved for Linux / system stall simulation (p/m thread "zap" function, hehe) */
         if (run == 99) run = 1;
         else run = 99;
      }

      return false;
   }

   return fQuit;  /* non-master threads don't handle keyboard commands, they do whatever the master thread does */
}


/* local function to handle application screen output and cursor position update */

void app_printf(unsigned int uFlags, int thread_index, const char* fmt, ...) {

char outstr[MAX_APP_STR_LEN];
char* p;
va_list va;
int slen;

   p = &outstr[1];

   va_start(va, fmt);
   vsnprintf(p, sizeof(outstr)-1, fmt, va);
   va_end(va);

   if ((uFlags & APP_PRINTF_THREAD_INDEX_SUFFIX) && num_app_threads > 1) sprintf(&p[strlen(p)], " (%d)", thread_index);  /* add application thread index suffix if specified */

/* make a reasonable effort to coordinate screen output between application threads and p/m threads, JHB Apr2020:

   -p/m threads indicate when they are printing to screen by setting a bit in pm_thread_printf
   -atomic read/compare/write sets/clears isCursorMidLine to indicate cursor position is "start of line" or somewhere mid-line
   -race conditions in determining when the cursor is mid-line can still occur, but they are greatly reduced
*/

   while (pm_thread_printf);  /* wait for any p/m threads printing to finish. No locks are involved so this is quick */

   if ((slen = strlen(p)) && !(uFlags & APP_PRINTF_SAMELINE) && p[slen-1] != '\n') { strcat(p, " \n"); slen += 2; }

   if (slen) {

      if ((uFlags & APP_PRINTF_NEWLINE) && __sync_val_compare_and_swap(&isCursorMidLine, 1, 0)) *(--p) = '\n';  /* update isCursorMidLine if needed */
      else if (p[slen-1] != '\n') __sync_val_compare_and_swap(&isCursorMidLine, 0, 1);

      uLineCursorPos = p[slen-1] != '\n' ? slen : 0;  /* update line cursor position */

      printf("%s", p);  /* use buffered output */
      
      if ((uFlags & APP_PRINTF_EVENT_LOG) || (uFlags & APP_PRINTF_EVENT_LOG_NO_TIMESTAMP)) {

         if (uFlags & APP_PRINTF_EVENT_LOG_STRIP_LFs) {

            bool fEndLF = false;
            char* pLF = strrchr(p, '\n');
            if (pLF == p+strlen(p)-1) { fEndLF = true; *pLF = '~'; }  /* don't strip end LF */
            while ((pLF = strrchr(p, '\n'))) *pLF = '.';  /* replace any formatting LFs with . */
            if (fEndLF) *(p+strlen(p)-1) = '\n';  /* restore end LF */
         }

         Log_RT(4 | DS_LOG_LEVEL_FILE_ONLY | ((uFlags & APP_PRINTF_EVENT_LOG_NO_TIMESTAMP) ? DS_LOG_LEVEL_NO_TIMESTAMP : 0), p);  /* if specified also print to event log */
      }
   }
}
