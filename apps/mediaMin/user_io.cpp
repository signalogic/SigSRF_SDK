/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/user_io.cpp

 Copyright (C) Signalogic Inc. 2021-2025

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  User I/O related source for mediaMin reference application

 Documentation

  https://www.github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediamin

  Older documentation links:
  
    after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

    before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

 Revision History

   Created Apr 2021 JHB, split off from mediaMin.cpp
   Modified Jan 2023 JHB, add debugMode to thread info printout ('d' key interactive command)
   Modified Jan 2023 JHB, add handling of "fCtrl_C_pressed" var in ProcessKeys() (see comments in mediaTest/see cmd_line_interface.c)
   Modified Jan 2023 JHB, add command line to interactive 'd' key command (real-time debug output)
   Modified Feb 2023 JHB, one-time set stdout to non-buffered in ProcessKeys(). See comments
   Modified Feb 2024 JHB, update usage of DSGetLogTimestamp() per changes in diaglib.h
   Modified Apr 2024 JHB, remove DS_CP_DEBUGCONFIG and DS_LOG_LEVEL_UPTIME_TIMESTAMP flags, which are now deprecated (for the latter uptime timestamps are the default). See comments in pktlib.h and diaglib.h
   Modified Jun 2024 JHB, include '\r' in updating isCursorMidLine and uLineCursorPos
   Modified Jul 2024 JHB, update reference to isMasterThread()
   Modified Aug 2024 JHB, slight mod to quit key console display
   Modified Sep 2024 JHB, change DS_PULLPACKETS_TRANSCODED to DS_PULLPACKETS_OUTPUT per flag rename in pktlib.h
   Modified Oct 2024 JHB, fix bug in param passing order in DSGetLogTimestamp()
   Modified Nov 2024 JHB, include directcore.h (no longer implicitly included in other header files)
   Modified Dec 2024 JHB, include <algorithm> and use std namespace
   Modified Jan 2025 JHB, some comments added after AI tools source code review
   Modified Mar 2025 JHB, in app_printf() update thread_info[].most_recent_console_output, add cur_time param in app_printf() and UpdateCounters()
   Modified Apr 2025 JHB, in app_printf() implement APP_PRINTF_SAME_LINE_PRESERVE, fix bug with slen not being incremented when \n or \r inserted at output string reserved zeroth location
*/

#include <algorithm>
using namespace std;

#include <stdio.h>
#include <stdarg.h>

#include "directcore.h"  /* DirectCore APIs */
#include "diaglib.h"    /* bring in Log_RT() definition */
#include "pktlib.h"

#include "shared_include/session.h"    /* session management structs and definitions */
#include "shared_include/config.h"     /* configuration structs and definitions */

#include "mediaTest.h"  /* bring in constants needed by mediaMin.h */
#include "mediaMin.h"
#include "user_io.h"

/*  as noted in Revision History, code was split from mediaMin.cpp; the following extern references are necessary to retain tight coupling with related source in mediaMin.cpp. There are no multithread or concurrency issues in these references */

extern unsigned int num_app_threads;   /* see comments in mediaMin.h */
extern APP_THREAD_INFO thread_info[];  /* APP_THREAD_INFO struct defined in mediaMin.h */

extern bool fQuit;         /* set if 'q' (quit) key is pressed */
extern bool fPause;        /* "" 'p' (pause).  Pauses operation, another 'p' resumes.  Can be combined with 'd' (display) key to read out internal p/m thread debug, capacity, stats, and other info */ 
extern bool fStop;         /* "" 's' (stop).  Stop prior to next repeat (only applies if -RN is entered on cmd line.  Intended for clean stop to repeating test, avoiding partial output files, especially when ENABLE_RANDOM_WAIT is active */

extern int num_pktmed_threads;    /* number of packet/media threads running */
extern bool fRepeatIndefinitely;  /* true if -R0 is given on the cmd line */
extern int nRepeatsRemaining[];


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
      strcat(tmpstr, " ");  /* real-time stats readability, JHB Mar 2025 */

      thread_info[thread_index].prev_pkt_push_ctr = thread_info[thread_index].pkt_push_ctr;
      thread_info[thread_index].prev_pkt_pull_jb_ctr = thread_info[thread_index].pkt_pull_jb_ctr;
      thread_info[thread_index].prev_pkt_pull_xcode_ctr = thread_info[thread_index].pkt_pull_xcode_ctr;
      thread_info[thread_index].prev_pkt_pull_streamgroup_ctr = thread_info[thread_index].pkt_pull_streamgroup_ctr;
   }

   if (strlen(tmpstr)) app_printf(APP_PRINTF_SAME_LINE | APP_PRINTF_THREAD_INDEX_SUFFIX, cur_time, thread_index, tmpstr);  /* use fully buffered I/O; i.e. not stdout (line buffered) or stderr (per character) */
}


/* process interactive keyboard input */

bool ProcessKeys(HSESSION hSessions[], DEBUG_CONFIG* dbg_cfg, uint64_t cur_time, int thread_index) {

char key;
static int app_thread_index_debug = 0;
static int pm_thread_index_debug = 0;
int i;
char tmpstr[500] = "";
PACKETMEDIATHREADINFO PacketMediaThreadInfo;
static uint64_t last_time = 0;
static uint8_t save_uPrintfLevel = 0;
static bool fSetStdoutNonBuffered = false;

   if (isMasterThread(thread_index)) {  /* master application thread (thread_index = 0) handles interactive keyboard commands */

   /* One-time set stdout to non-buffered, JHB Feb 2023:
   
       -for some reason if getkey() in keybd.c uses read(fileno(stdin)...) instead of fgetc() then subsequent stdout like printf("\rxxx") won't appear until at least one "\n" is output. Fortunately if we set stdout to non-buffered here one-time before calling getkey() that's a fix

       -more discussion on this here: https://stackoverflow.com/questions/68943683/strange-behavior-in-terminal-raw-mode

       -the reason why fgetc() is no longer used is due to it not working inside Docker containers; see comments in getkey() in keybd.c

       -maybe fget() affects one or more flags not included in termios ?
    */

      if (!fSetStdoutNonBuffered) { setvbuf(stdout, NULL, _IONBF, 0); fSetStdoutNonBuffered = true; }

      if (last_time == 0) last_time = cur_time;
      if ((int64_t)cur_time - (int64_t)last_time < 100*1000 && !fPause) return false;  /* check keys every 100 msec. Make an exception for pause key, otherwise we never get out of pause */

      last_time = cur_time;

      key = (char)tolower(getkey());

      if (key == 'q' || pm_run <= 0 || fCtrl_C_pressed) {  /* quit key, Ctrl-C, or p/m thread error condition */

         strcpy(tmpstr, "#### ");
         #if 0
         if (key == 'q') sprintf(&tmpstr[strlen(tmpstr)], "q key entered");
         #else  /* q shows immediately on the console if pressed, maybe not repeating it and placing "####" after looks more readable and still noticeable, JHB Aug 2024 */
         if (key == 'q') {
            if (isCursorMidLine) strcpy(tmpstr, "q");
            else strcpy(tmpstr, "");
            sprintf(&tmpstr[strlen(tmpstr)], " key entered ####");
         }
         #endif
         else if (pm_run == 0) sprintf(&tmpstr[strlen(tmpstr)], "p/m run abort (run = 0)"); 
         else if (pm_run < 0) sprintf(&tmpstr[strlen(tmpstr)], "p/m thread error and abort condition"); 
         else if (fCtrl_C_pressed) sprintf(&tmpstr[strlen(tmpstr)], "Ctrl-C entered");

         sprintf(&tmpstr[strlen(tmpstr)], ", exiting mediaMin");
         app_printf(APP_PRINTF_NEW_LINE, cur_time, thread_index, tmpstr);

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

         DSConfigPktlib(NULL, dbg_cfg, 0);
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

         DSGetLogTimestamp(tmpstr, DS_LOG_LEVEL_WALLCLOCK_TIMESTAMP, sizeof(tmpstr), 0);  /* set additional user timeval param to zero (not used), JHB Feb 2024. Remove DS_LOG_LEVEL_UPTIME_TIMESTAMP flag, which is now deprecated (uptime timestamps are the default), JHB Apr 2024. Move DS_LOG_LEVEL_WALLCLOCK_TIMESTAMP flag to 2nd param, per change in diaglib.h, JHB Oct 2024 */
         strcat(tmpstr, " ");  /* DSGetLogTimestamp() no appends trailing space, JHB Feb 2024 */

         char repeatstr[50];
         if (!fRepeatIndefinitely && nRepeatsRemaining[thread_index] >= 0) sprintf(repeatstr, ", repeats remaining = %d", nRepeatsRemaining[thread_index]);  /* if cmd line entry includes -RN with N >= 0, nRepeatsRemaining will be > 0 for repeat operation, JHB Jan2020 */
         else if (nRepeatsRemaining[thread_index] == -1) strcpy(repeatstr, ", no repeats");  /* nRepeat is -1 if cmd line has no -RN entry (no repeats). For cmd line entry -R0, fRepeatIndefinitely will be set */

         printf("%s#### (App Thread) %sDebug info for app thread %d, run = %d%s, command line %s \n", uLineCursorPos ? "\n" : "", tmpstr, app_thread_index_debug, pm_run, fRepeatIndefinitely ? ", repeating indefinitely" : repeatstr, szAppFullCmdLine);

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
               if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) sprintf(&tmpstr[strlen(tmpstr)], " %d", DSPullPackets(DS_PULLPACKETS_GET_QUEUE_STATUS | DS_PULLPACKETS_OUTPUT | DS_PULLPACKETS_JITTER_BUFFER, NULL, NULL, hSessions[i], NULL, 0, 0));
            }

            sprintf(&tmpstr[strlen(tmpstr)], ", pcap input check =");
            for (i=0; i<thread_info[app_thread_index_debug].nInPcapFiles; i++) {
               sprintf(&tmpstr[strlen(tmpstr)], " %d", thread_info[app_thread_index_debug].pcap_in[i] != NULL);
            }
 
            printf("%s \n", tmpstr);

#if 0  /* deprecated, don't use this method */
            pm_run = 2;
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
         if (pm_run == 99) pm_run = 1;
         else pm_run = 99;
      }

      return false;
   }

   return fQuit;  /* non-master threads don't handle keyboard commands, they do whatever the master thread does */
}


/* handy function to handle application screen output, event logging, and cursor position update. For uFlags, see flags in user_io.h */

void app_printf(unsigned int uFlags, uint64_t cur_time, int thread_index, const char* fmt, ...) {

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

   while (pm_thread_printf);  /* wait for any p/m threads printing to finish. No locks involved so this is quick */

   if ((slen = strlen(p)) && !(uFlags & APP_PRINTF_SAME_LINE) && p[slen-1] != '\n') { strcat(p, " \n"); slen += 2; }  /* if not same line, suffix with newline */

   if (slen) {

      if ((uFlags & APP_PRINTF_NEW_LINE) && __sync_val_compare_and_swap(&isCursorMidLine, 1, 0)) { *(--p) = '\n'; slen++; }  /* prefix with newline, update isCursorMidLine if needed */
      else if (p[slen-1] != '\n' && p[slen-1] != '\r') {

         __sync_val_compare_and_swap(&isCursorMidLine, 0, 1);

         if (uFlags & APP_PRINTF_SAME_LINE_PRESERVE) isLinePreserve = true;
         else if (isLinePreserve) {
            if (p[0] != '\n') { *(--p) = '\n'; slen++; }
            isLinePreserve = false;
         }
      }

      uLineCursorPos = (p[slen-1] != '\n' && p[slen-1] != '\r') ? slen : 0;  /* update line cursor position */

      printf("%s", p);  /* use buffered output */

      thread_info[thread_index].most_recent_console_output = cur_time;  /* update time of most recent console output, JHB Mar 2025 */

      if ((uFlags & APP_PRINTF_EVENT_LOG) || (uFlags & APP_PRINTF_EVENT_LOG_NO_TIMESTAMP)) {

         if (uFlags & APP_PRINTF_EVENT_LOG_STRIP_LFs) {

            bool fEndLF = false;
            char* pLF = strrchr(p, '\n');
            if (pLF == p+strlen(p)-1) { fEndLF = true; *pLF = '~'; }  /* don't strip end LF */
            while ((pLF = strrchr(p, '\n'))) *pLF = '.';  /* replace any formatting LFs with . */
            if (fEndLF) *(p+strlen(p)-1) = '\n';  /* restore end LF */
         }

         Log_RT(4 | DS_LOG_LEVEL_OUTPUT_FILE | ((uFlags & APP_PRINTF_EVENT_LOG_NO_TIMESTAMP) ? DS_LOG_LEVEL_NO_TIMESTAMP : 0), p);  /* if specified also print to event log */
      }
   }
}

void PrintPacketBuffer(uint8_t* buf, int len, const char* szStartMarker, const char* szEndMarker) {

bool fNewLine = false;

   if (szStartMarker && strlen(szStartMarker)) printf("%s%s", isCursorMidLine ? "\n" : "", szStartMarker);

   for (int i=0; i<len; i++) {

      fNewLine = false;

      switch (buf[i]) {
         case 0 ... 9:
            printf("%c", 178);
            break;
         case 0xa:
            printf("%c", buf[i]);
            fNewLine = true;
            break;
         case 0xb ... 0x0c:
            printf("%c", 178);
            break;
         case 0xd:
            printf("%c", buf[i]);
            fNewLine = true;
            break;
         case 0xe ... 31:
            printf("%c", 178);
            break;
         case 127 ... 255:
            printf("%c", 178);
            break;
         default:
            printf("%c", buf[i]);
      }
   }

   if (szEndMarker && strlen(szEndMarker)) {

      printf("%s%s", !fNewLine ? "\n" : "", szEndMarker);
      if (szEndMarker[strlen(szEndMarker)-1] == '\n') __sync_val_compare_and_swap(&isCursorMidLine, 1, 0);
   }
}

void PrintSIPInviteFragments(uint8_t* pkt_buf, PKTINFO* PktInfo, int pkt_len) {

static int state = 0, count = 0;

   switch (state) {

      case 0:
      {
         char s[] = "Length:";
         int i = 0, len;
         uint8_t* p = (uint8_t*)memmem(pkt_buf, pkt_len, s, strlen(s));

         if (p) {
            p += strlen(s);
            while (p[i] >= 0x20) i++;
            uint8_t save = p[i];
            p[i] = 0;
            len = atoi((const char*)p);
            p[i] = save;

            if (len > 1) {
               printf("\n *** found Length = %d, pkt len = %d, flags = 0x%x, ip hdr checksum = 0x%x, udp checksum = 0x%x, src port = %u, dst port = %u \n", len, pkt_len, PktInfo->flags, PktInfo->ip_hdr_checksum, PktInfo->udp_checksum, PktInfo->src_port, PktInfo->dst_port);
               PrintPacketBuffer(&pkt_buf[PktInfo->ip_hdr_len], pkt_len-PktInfo->ip_hdr_len, "*** buf start \n", "*** buf end \n");
               state = 1;
            }
         }
         break;
      }

      case 1:
      {
         if (count < 4) {

            printf("\n *** count = %d, pkt_len = %d, flags = 0x%x, ip hdr checksum = 0x%x, udp checksum = 0x%x, src port = %u, dst port = %u \n", count++, pkt_len, PktInfo->flags, PktInfo->ip_hdr_checksum, PktInfo->udp_checksum, PktInfo->src_port, PktInfo->dst_port);
            PrintPacketBuffer(&pkt_buf[PktInfo->ip_hdr_len], pkt_len-PktInfo->ip_hdr_len, "*** buf start \n", "*** buf end \n");
         }
         else {
            count = 0;
            state = 0;
         }
         break;
      }
   }
}
