/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/stats.cpp

 Copyright (C) Signalogic Inc. 2020-2025

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  stats related source for mediaMin reference application

 Documentation

  https://www.github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediamin

  Older documentation links:
  
    after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

    before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

 Revision History before being moved to separate file
 
   Modified Apr 2023 JHB, add num TCP and UDP packets to mediaMin stats summary
   Modified Aug 2023 JHB, implement SHOW_PACKET_ARRIVAL_STATS flag. This includes packet arrival info in mediaMin stats display
   Modified Jun 2024 JHB, add summary stats for UDP duplicated, fragmented, and encapsulated packets
   Modified Aug 2024 JHB, convert GetMD5Sum() to generic MediaOutputFileOps() function to support --md5sum, --sha1sum, and --sha512sum command line options
   Modified Nov 2024 JHB, update session summary stats and enable for static sessions (see comments in CreateStaticSessions() in session_app.cpp)
   Modified Apr 2025 JHB, add support for md5 and sha stats on video stream output files; this allows bit-exactness testing
   Modified May 2025 JHB, add output stats display and logging for all categories: stream groups (both media domain and timestamp matching), transcoding, and bitstream. Allow bit-exact ops for all combinations of hash options on the cmd line
   Modified Jun 2025 JHB, update summary stats to show bit-exact checks (md5 sum, sha1 sum, etc) for all transcode and bitstream outputs, reorganize MediaOutputFileOps()

 Revision History

   Created Jun 2025 JHB, split off from mediaMin.cpp
*/

#include <algorithm>
using namespace std;

#include <stdio.h>
#include <stdarg.h>

#include "pktlib.h"
#include "streamlib.h"

#include "mediaTest.h"  /* fShow_md5sum, fShow_sha1sum, fShow_sha512sum */
#include "mediaMin.h"
#include "user_io.h"    /* app_printf() */

/*  as noted in Revision History, code was split from mediaMin.cpp; the following extern references are necessary to retain tight coupling with related source in mediaMin.cpp. There are no multithread or concurrency issues in these references */

extern unsigned int num_app_threads;   /* see comments in mediaMin.h */
extern APP_THREAD_INFO thread_info[];  /* APP_THREAD_INFO struct defined in mediaMin.h */
extern const char tabstr[];
extern int nRepeats, nRepeatsCompleted[];
extern bool fRepeatIndefinitely;

/* MediaOutputFileOps() flag definitions */

#define MOFO_STREAMGROUP_BITEXACT            1
#define MOFO_TIMESTAMPMATCH_BITEXACT         2
#define MOFO_TRANSCODE_BITEXACT              3
#define MOFO_BITSTREAM_BITEXACT              4
#define MOFO_STR_APPEND                  0x100
#define MOFO_ITEM_MASK                   0x0ff

/* MediaOutputFileOps() executes operations on media output files. DSDisplayLogSummaryStats() calls this with hash commands (e.g. md5sum, sha1sum, etc), we might expand this in the future */

int MediaOutputFileOps(const std::string szCmd, char* szResult, unsigned int uFlags, int nOutput, int thread_index) {

std::string labelstr = "";
char szMediaFilename[2*CMDOPT_MAX_INPUT_LEN] = "", hashstr[2*CMDOPT_MAX_INPUT_LEN];
int ret_val = 0;

/* operate on output waveform depending on uFlags and output number (nOutput) */

   switch (uFlags & MOFO_ITEM_MASK) {
   
      case MOFO_STREAMGROUP_BITEXACT:

         if (Mode & ENABLE_WAV_OUTPUT) DSGetStreamGroupInfo(nOutput, DS_STREAMGROUP_INFO_HANDLE_IDX | DS_STREAMGROUP_INFO_MERGE_FILENAME, NULL, NULL, szMediaFilename);
         else strcpy(szMediaFilename, thread_info[thread_index].szGroupPcap[nOutput]);  /* stream group output pcap filename */ 

         if (strlen(szMediaFilename)) labelstr = (isFTRTMode() ? "FTRT" : (isAFAPMode() ? "AFAP" : "real-time")) + (std::string)" mode";
         break;

      case MOFO_TIMESTAMPMATCH_BITEXACT:

         DSGetStreamGroupInfo(nOutput, DS_STREAMGROUP_INFO_HANDLE_IDX | DS_STREAMGROUP_INFO_MERGE_TSM_FILENAME, NULL, NULL, szMediaFilename);

         if (strlen(szMediaFilename)) labelstr = "timestamp-match mode";
         break;

      case MOFO_TRANSCODE_BITEXACT:

         strcpy(szMediaFilename, thread_info[thread_index].szTranscodeOutput[nOutput]);  /* transcode output file */
         if (strlen(szMediaFilename)) labelstr = "transcode";
         break;

      case MOFO_BITSTREAM_BITEXACT:

         strcpy(szMediaFilename, thread_info[thread_index].szVideoStreamOutput[nOutput]);  /* video stream output file */
         if (strlen(szMediaFilename)) labelstr = "video output stream";
         break;

      default:
         break;
   }

   if (labelstr != "") {

   /* execute console command on output file. DSConsoleCommand() is defined in diaglib.h  */

      if (strlen(szMediaFilename) > 0 && DSConsoleCommand(szCmd.c_str(), szMediaFilename, hashstr, 1, sizeof(hashstr)) == 1) {  /* ask for first result string from cmd output */

      /* format result string in szResult and return string length */

         ret_val = sprintf(&szResult[(uFlags & MOFO_STR_APPEND) ? strlen(szResult) : 0], "%s %s %s %s", szCmd.c_str(), labelstr.c_str(), hashstr, szMediaFilename);  /* append or copy to user string command, label, result, filename */
      }
   }

   return ret_val;
}

/* display / log summary stats */

void DisplayLogSummaryStats(char* tmpstr, uint64_t cur_time, int thread_index) {

int i;
unsigned int nOrphansRemoved, nMaxListFragments;

   nOrphansRemoved = DSPktRemoveFragment(NULL, 0, &nMaxListFragments);

   bool fLogTimeStampPrinted = false;  /* make sure only one event log timestamp is printed in the case of multiple stats strings (which should only happen with 100s of inputs during stress tests) */
  
/* note stats are concatenated into one string declared in mediaMin and then give to one call to app_printf() for display and logging; when multiple app threads and packet/media threads are running this avoids text fragments and mixing with other messages. The string can be large, see MAX_APP_STR_LEN in mediaMin.h (currently set to 12000) */

   sprintf(tmpstr, "=== mediaMin stats");
   if (num_app_threads > 1) sprintf(&tmpstr[strlen(tmpstr)], " (%d)", thread_index);  /* show application thread if more than one. Show only once, on stats heading */

/* display input stats */

   sprintf(&tmpstr[strlen(tmpstr)], "\n%spackets [input]", tabstr);

   sprintf(&tmpstr[strlen(tmpstr)], "\n%s%stotal%s", tabstr, tabstr, thread_info[thread_index].nInPcapFiles > 1 ? "s" : "");
   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].packet_number[i]);

   sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sFragments =", tabstr, tabstr);
   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].num_packets_fragmented[i]);

   sprintf(&tmpstr[strlen(tmpstr)], ", reassembled =");
   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].num_packets_reassembled[i]);

   sprintf(&tmpstr[strlen(tmpstr)], ", orphans = ");
   sprintf(&tmpstr[strlen(tmpstr)], "%u", nOrphansRemoved);

   sprintf(&tmpstr[strlen(tmpstr)], ", max on list = ");
   sprintf(&tmpstr[strlen(tmpstr)], "%u", nMaxListFragments);

   sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sOversize non-fragmented =", tabstr, tabstr);
   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].num_oversize_nonfragmented_packets[i]);

   sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sTCP =", tabstr, tabstr);
   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].num_tcp_packets[i]);

   sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sUDP =", tabstr, tabstr);
   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].num_udp_packets[i]);

   sprintf(&tmpstr[strlen(tmpstr)], ", encapsulated =");
   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].num_packets_encapsulated[i]);

   sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sRTP =", tabstr, tabstr);
   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].num_rtp_packets[i]);

   sprintf(&tmpstr[strlen(tmpstr)], ", RTCP =");
   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].num_rtcp_packets[i]);

   for (int fFirst=0,i=0; i<thread_info[thread_index].nInPcapFiles; i++) if (thread_info[thread_index].num_rtcp_custom_packets[i]) {
      if (!fFirst) sprintf(&tmpstr[strlen(tmpstr)], ", Custom RTCP =");
      sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].num_rtcp_custom_packets[i]);
      fFirst = 1;
   }

   sprintf(&tmpstr[strlen(tmpstr)], ", Unhandled =");
   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].num_unhandled_rtp_packets[i]);

   sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sRedundant discards TCP =", tabstr, tabstr);
   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].tcp_redundant_discards[i]);  /* display/log number of redundant TCP retransmissions discarded, if any */

   sprintf(&tmpstr[strlen(tmpstr)], ", UDP =");
   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%u", i, thread_info[thread_index].udp_redundant_discards[i]);  /* display/log number of redundant UDP retransmissions discarded, if any */

   strcat(tmpstr, "\n");

/* if specified, display packet arrival stats, JHB Aug 2023 */

   if (Mode & SHOW_PACKET_ARRIVAL_STATS) {

      sprintf(&tmpstr[strlen(tmpstr)], "%sarrival timing [stream]", tabstr);

      sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sdelta avg/max (msec) =", tabstr, tabstr);
      for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%4.2f/%4.2f", i, thread_info[thread_index].arrival_avg_delta[i]/thread_info[thread_index].num_arrival_stats_pkts[i], thread_info[thread_index].arrival_max_delta[i]);

      sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sdelta avg clock (msec) =", tabstr, tabstr);
      for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%4.2f", i,  timeScale*thread_info[thread_index].arrival_avg_delta_clock[i]/thread_info[thread_index].num_arrival_stats_pkts[i]);

      #ifdef RTP_TIMESTAMP_STATS  /* can be enabled in mediaMin.h for RTP timestamp stats and debug. Not normally used as timestamps are unlikely to be in correct order until processing by pktlib jitter buffer, JHB Mar 2025 */

      sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sdelta avg rtp_timestamp (msec) =", tabstr, tabstr);
      for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%4.2f", i,  thread_info[thread_index].rtp_timestamp_avg_delta[i]/thread_info[thread_index].num_arrival_stats_pkts[i]);
      #endif

      sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sjitter avg/max (msec) =", tabstr, tabstr);
      for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%4.2f/%4.2f", i, thread_info[thread_index].arrival_avg_jitter[i]/thread_info[thread_index].num_arrival_stats_pkts[i], thread_info[thread_index].arrival_max_jitter[i]);

      strcat(tmpstr, "\n");
   }

   sprintf(&tmpstr[strlen(tmpstr)], "%ssession [stream]\n", tabstr);

   for (i=0; i<thread_info[thread_index].num_stream_stats; i++) {

      char szSessInfo[200], szTimestamp[200];

      if (thread_info[thread_index].StreamStats[i].uFlags & STREAM_STAT_FIRST_PKT) DSGetLogTimestamp(szTimestamp, DS_EVENT_LOG_USER_TIMEVAL | DS_EVENT_LOG_UPTIME_TIMESTAMPS | DS_EVENT_LOG_TIMEVAL_PRECISION_USEC, sizeof(szTimestamp), timeScale*thread_info[thread_index].StreamStats[i].first_pkt_usec);  /* convert usec to timestamp string */
      else strcpy(szTimestamp, "n/a");

      sprintf(szSessInfo, "%s%s[%d] hSession %d %s, term %d, ch %d, codec %s, bitrate %d, payload type %d, ssrc 0x%x, first packet %s \n", tabstr, tabstr, i, thread_info[thread_index].StreamStats[i].hSession, thread_info[thread_index].StreamStats[i].uFlags & STREAM_STAT_DYNAMIC_SESSION ? "dynamic" : "static", thread_info[thread_index].StreamStats[i].term, thread_info[thread_index].StreamStats[i].chnum, thread_info[thread_index].StreamStats[i].codec_name, thread_info[thread_index].StreamStats[i].bitrate, thread_info[thread_index].StreamStats[i].payload_type, thread_info[thread_index].StreamStats[i].first_pkt_ssrc, szTimestamp);

      if (strlen(tmpstr) + strlen(szSessInfo) < sizeof(tmpstr)) sprintf(&tmpstr[strlen(tmpstr)], "%s", szSessInfo);  /* if enough sessions and/or repeats we need to split up printouts, JHB Jun 2020 */
      else {
         app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_SAME_LINE | APP_PRINTF_EVENT_LOG | (fLogTimeStampPrinted ? APP_PRINTF_EVENT_LOG_NO_TIMESTAMP : 0), cur_time, thread_index, tmpstr);
         app_printf(APP_PRINTF_EVENT_LOG | APP_PRINTF_EVENT_LOG_NO_TIMESTAMP, cur_time, thread_index, szSessInfo);  /* added a second app_printf() and fLogTimeStampPrinted to fix a bug in reporting multi-hundred count session summary stats that appeared during 2+ hr stress tests. Combining all summary stats per thread into one string would be optimal for multi-thread app operation, but they could potentially be really huge strings needing std::string (or malloc + realloc, whichever is faster). Also would need to deal with Log_RT() (called inside app_printf) which truncates around 4k. Not something to worry about for the time being, JHB Jan 2023 */
         fLogTimeStampPrinted = true;
         tmpstr[0] = (char)0;  /* start the string over */
      }
   }

   /* display / log output stats */

   {
      sprintf(&tmpstr[strlen(tmpstr)], "%spackets [output] \n", tabstr);

   /* determine if cmd line bit-exact checks are active */

      #define NUM_CMDS 3
      std::string szCmd[NUM_CMDS] = { "" };
      if (fShow_md5sum) szCmd[0] = "md5sum";
      if (fShow_sha1sum) szCmd[1] = "sha1sum";
      if (fShow_sha512sum) szCmd[2] = "sha512sum";

      sprintf(&tmpstr[strlen(tmpstr)], "%s%sStream group =", tabstr, tabstr);

      int num_streamgroup_outputs = 0;
      for (i=0; i<thread_info[thread_index].nStreamGroups; i++) { sprintf(&tmpstr[strlen(tmpstr)], " [%d]%d", i, thread_info[thread_index].pkt_stream_group_pcap_out_ctr[i]); num_streamgroup_outputs++; }
      if (!num_streamgroup_outputs) sprintf(&tmpstr[strlen(tmpstr)], " n/a");
      strcat(tmpstr, " \n");

   /* display / log stream group overall stats */

      if (num_streamgroup_outputs && !isAFAPMode() && !isFTRTMode()) {  /* in "as fast as possible" (-r0 cmd line entry) and "faster than real-time" modes (-r0.N cmd line entry where 0 < 0.N < 1) we are currently not showing stream group output stats. This may change after extensive AFAP and FTRT mode testing, JHB May 2023 */

         sprintf(&tmpstr[strlen(tmpstr)], "%s%s%sMissed stream group intervals = %d \n", tabstr, tabstr, tabstr, thread_info[thread_index].group_interval_stats_index);

         for (i=0; i<thread_info[thread_index].group_interval_stats_index; i++) {

            sprintf(&tmpstr[strlen(tmpstr)], "%s%s%s%s[%d] missed stream group interval = %d, hSession = %d", tabstr, tabstr, tabstr, tabstr, i, thread_info[thread_index].GroupIntervalStats[i].missed_interval, thread_info[thread_index].GroupIntervalStats[i].hSession);
            if (thread_info[thread_index].GroupIntervalStats[i].repeats) sprintf(&tmpstr[strlen(tmpstr)], " %dx", thread_info[thread_index].GroupIntervalStats[i].repeats+1);

            strcat(tmpstr, " \n");
         }

         sprintf(&tmpstr[strlen(tmpstr)], "%s%s%sMarginal stream group pulls = %d \n", tabstr, tabstr, tabstr, thread_info[thread_index].group_pull_stats_index);

         for (i=0; i<thread_info[thread_index].group_pull_stats_index; i++) sprintf(&tmpstr[strlen(tmpstr)], "%s%s%s%s[%d] marginal stream group pull at %d, retries = %d, hSession = %d \n", tabstr, tabstr, tabstr, tabstr, i, thread_info[thread_index].GroupPullStats[i].retry_interval, thread_info[thread_index].GroupPullStats[i].num_retries, thread_info[thread_index].GroupPullStats[i].hSession);
      }

   /* display media output file md5 sum if --md5sum cmd line option is present, JHB Sep 2023. Expand to include --sha1sum and --sha512sum cmd line options, Aug 2024 */

      if (num_streamgroup_outputs) for (i=0; i<NUM_CMDS; i++) if (szCmd[i] != "") for (int j=0; j<num_streamgroup_outputs; j++) {

         sprintf(&tmpstr[strlen(tmpstr)], "%s%s%s[%d] ", tabstr, tabstr, tabstr, j);
         MediaOutputFileOps(szCmd[i], tmpstr, MOFO_STREAMGROUP_BITEXACT | MOFO_STR_APPEND, j, thread_index);  /* get bit-exact result for stream group output wav files. MOFO_STR_APPEND specifies appending command output to szResult (tmpstr) */
         strcat (tmpstr, " \n");
      }

      sprintf(&tmpstr[strlen(tmpstr)], "%s%sTimestamp match =", tabstr, tabstr);

      if (Mode & ENABLE_TIMESTAMP_MATCH_MODE) sprintf(&tmpstr[strlen(tmpstr)], " [%d]%d", 0, DSGetStreamGroupInfo(0, DS_STREAMGROUP_INFO_HANDLE_IDX | DS_STREAMGROUP_INFO_MERGE_TSM_PACKET_COUNT, NULL, NULL, NULL));
      else sprintf(&tmpstr[strlen(tmpstr)], " n/a");
      strcat(tmpstr, " \n");

      if (Mode & ENABLE_TIMESTAMP_MATCH_MODE) for (i=0; i<NUM_CMDS; i++) if (szCmd[i] != "") {

         sprintf(&tmpstr[strlen(tmpstr)], "%s%s%s[%d] ", tabstr, tabstr, tabstr, 0);
         MediaOutputFileOps(szCmd[i], tmpstr, MOFO_TIMESTAMPMATCH_BITEXACT | MOFO_STR_APPEND, 0, thread_index);  /* get bit-exact result for timestamp match output wav files. MOFO_STR_APPEND specifies appending command output to szResult (tmpstr) */
         strcat (tmpstr, " \n");
      }

      sprintf(&tmpstr[strlen(tmpstr)], "%s%sTranscode =", tabstr, tabstr);

      int num_transcode_outputs = 0;
      for (i=0; i<thread_info[thread_index].nOutFiles; i++) if (thread_info[thread_index].nOutputType[i] == PCAP) { sprintf(&tmpstr[strlen(tmpstr)], " [%d]%d", i, thread_info[thread_index].pkt_transcode_pcap_out_ctr[i]), num_transcode_outputs++; };
      if (!num_transcode_outputs) sprintf(&tmpstr[strlen(tmpstr)], " n/a");
      strcat(tmpstr, " \n");

      if (num_transcode_outputs) for (i=0; i<NUM_CMDS; i++) if (szCmd[i] != "") for (int j=0; i<thread_info[thread_index].nOutFiles; j++) if (thread_info[thread_index].nOutputType[j] == PCAP) {

         sprintf(&tmpstr[strlen(tmpstr)], "%s%s%s[%d] ", tabstr, tabstr, tabstr, j);
         MediaOutputFileOps(szCmd[i], tmpstr, MOFO_TRANSCODE_BITEXACT | MOFO_STR_APPEND, j, thread_index);  /* get bit-exact result for stream group output wav file. MOFO_STR_APPEND specifies appending command output to szResult (tmpstr) */
         strcat(tmpstr, " \n");
      }

      sprintf(&tmpstr[strlen(tmpstr)], "%s%sBitstream =", tabstr, tabstr);

      int num_bitstream_outputs = 0;
      for (i=0; i<thread_info[thread_index].nOutFiles; i++) if (thread_info[thread_index].nOutputType[i] == ENCODED) { sprintf(&tmpstr[strlen(tmpstr)], " [%d]%d", i, thread_info[thread_index].pkt_bitstream_out_ctr[i]), num_bitstream_outputs++; }
      if (!num_bitstream_outputs) sprintf(&tmpstr[strlen(tmpstr)], " n/a");
      strcat(tmpstr, " \n");

      if (num_bitstream_outputs) for (i=0; i<NUM_CMDS; i++) if (szCmd[i] != "") for (int j=0; j<thread_info[thread_index].nOutFiles; j++) if (thread_info[thread_index].nOutputType[j] == ENCODED) {

         sprintf(&tmpstr[strlen(tmpstr)], "%s%s%s[%d] ", tabstr, tabstr, tabstr, j);
         MediaOutputFileOps(szCmd[i], tmpstr, MOFO_BITSTREAM_BITEXACT | MOFO_STR_APPEND, j, thread_index);  /* get bit-exact result for stream group output wav file. MOFO_STR_APPEND specifies appending command output to szResult (tmpstr) */
         strcat(tmpstr, " \n");
      }
   }

/* show summary of repeat info if applicable, JHB Sep 2024 */

   if (nRepeats > 0 || fRepeatIndefinitely) {

      char szNumCmdLineRepeats[20] = "";
      if (nRepeats > 0) sprintf(szNumCmdLineRepeats, "/%d", nRepeats);
      sprintf(&tmpstr[strlen(tmpstr)], "%s%d%s repeat%s completed, cumulative wàrnings = %u, èrrors = %u, crìtical èrrors = %u", tabstr, nRepeatsCompleted[thread_index], szNumCmdLineRepeats, nRepeats > 0 ? "s" : (nRepeatsCompleted[thread_index] != 1 ? "s" : ""), __sync_fetch_and_add(&event_log_warnings, 0), __sync_fetch_and_add(&event_log_errors, 0), __sync_fetch_and_add(&event_log_critical_errors, 0));  /* note warning and error have slight "homoglyph" spelling differences to allow automated searches of event logs and console output for "warning" and "error" */
   }

   if (strlen(tmpstr)) app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_EVENT_LOG | (fLogTimeStampPrinted ? APP_PRINTF_EVENT_LOG_NO_TIMESTAMP : 0), cur_time, thread_index, tmpstr);  /* display stats summary */
}
