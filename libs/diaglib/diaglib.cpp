/*
 $Header: /root/Signalogic/DirectCore/lib/diaglib/diaglib.cpp

 Copyright Signalogic Inc. 2017-2024

 Github SigSRF License, Version 1.1 (https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md). Absolutely prohibited for AI language or programming model training use

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use
 
 Description

  packet diagnostic library with APIs for:

  -packet tracing and history logging
  -packet statistics, including ooo (out-of-order), DTX, packet loss and gaps, timestamp integrity, etc.
  -packet analysis, including input vs. jitter buffer output analysis

 Project

 DirectCore, SigSRF

 Documentation

  https://github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediamin

  Older documentation links:
  
   after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

   before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

 Revision History
  
  Created Aug 2017 Jeff Brower, (based on original c66x diagnostics APIs, created Oct-Dec 2016)
  Modified Aug 2017 JHB, display SID, SID Reuse, and SID NoData packet types, account for SID reuse packets when comparing input / buffered packets with output / pulled packets
  Modified Sep 2017 JHB, improve SID reuse handling for multiple SSRC streams
  Modified Sep 2017 JHB, implement DS_PKTSTATS_LOG_COLLATE_STREAMS flag in DSFindSSRCGroups() to add stream collation option
  Modified Sep 2017 JHB, modified DSPktStatsLogSeqnums() to handle duplicated packets, in particular DTMF event packets which may give several consecutive packets with identical seq numbers and timestamps
  Modified Sep 2017 JHB, added DS_PKTSTATS_LOG_MARK_DTMF_DUPLICATE flag
  Modified Jul 2018 JHB, modify input vs. output SSRC group comparison / analysis to account for possible different ordering of input vs. output SSRC groups (added a "mapping" between in/out SSRCs)
  Modified Jul 2018 JHB, add optional label param to DSPktStatsLogSeqnums()
  Modified Jul 2018 JHB, clearly label three main packet groups in log file, input, jitter buffer output, and analysis
  Modified Sep 2018 JHB, modified DSFindSSRCGroups() and DSPktStatsWriteLogFile() to use calloc instead of about 4.3 MB of stack usage (see comments in the function declarations)
  Modified Jan 2019 JHB, changed io_map_ssrcs and used_map_ssrcs usage and mallocs to int from short int.  gdb was indicating an intermittent crash when freeing these two
  Modified Feb 2019 JHB, fix crash case where "i_out" was -1 (due to an error situation, but still cannot be allowed)
  Modified Feb 2019 JHB, remove hSession param from DSPktStatsAddEntries() API.  All DSGetPacketInfo() calls inside the API do not need a session handle (do not need session/channel hashing and lookup)
  Modified Feb 2019 JHB, fix sorting bug that caused "orphan of one" in SSRC group sorting and collation.  See comments below
  Modified Mar 2019 JHB, remove references to DEMOBUILD and libbuild.h
  Modified Sep-Oct 2019 JHB, handle RTP sequence number wraps.  Look for seq_wrap[], in_seq_wrap[], and out_seq_wrap[]
  Modified Oct 2019 JHB, modify first stage "SSRC discovery" in DSFindSSRCGroups() to be more efficient and minimize initial SSRC groups found
  Modified Oct 2019 JHB, add ssrcs, pkt index, and rtp seq num arrays to DSPktStatsLogSeqnums(), and return number of ssrcs found.  This cuts in half number of calls to DSFindSSRCGroups() in DSPktStatsWriteLogFile()
  Modified Dec 2019 JHB, STREAM_STATS struct added in diaglib.h, modify DSPktStatsLogSeqnums() to return stats info in a STREAM_STATS struct ptr
  Modified Dec 2019 JHB, upgrade "analysis and stats" printout section:
                         -implement new DS_PKTSTATS_ORGANIZE_BY_SSRC, DS_PKTSTATS_ORGANIZE_BY_CHNUM, and DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP flags in diaglib.h
                         -add optional hSession and idx info to PKT_STATS struct which are referenced in analysis and stats depending on new flags (and if set to valid values)
                         -include for each SSRC in printout % packet loss, % ooo, max ooo, total input and output packets, number of missing seq numbers, and max consec missing seq numbers
                         -see analysis_and_stats() + comments
  Modified Jan 2020 JHB, fix sequence number wrap bug in DSPktStatsLogSeqnums(), display "Stream n" as channel number in analysis_and_stats() if DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP flag set
  Modified Jan 2020 JHB, make DSFindSSRCGroups() faster (see if !fCollate), fix problem in finding end seq number
  Modified Jan 2020 JHB, fix bugs in organizing-by-stream group, see "GroupMap" struct method
  Modified Mar 2020 JHB, in analysis_and_stats() (inp vs output packet analysis), add brief info printout for timestamp mismatches as they occur, similar to packet drops.  This saves time when debugging timestamp alignment issues in pktlib
  Modified Apr 2020 JHB, fix bug in timestamp mismatch output where output sequence number didn't include wrap count
  Modified May 2020 JHB, implement STREAM_STATS struct changes to rename numRepair to numSIDRepair and add numMediaRepair
  Modified Mar 2021 JHB, add STANDALONE #define option to build without SigSRF header files
  Modified Jun 2023 JHB, minor changes to packet description format (i) "pkt len =" to "rtp pyld len =" (ii) print "media" instead of nothing for media packets
  Modified Nov 2023 JHB, in analysis_and_stats() (analysis of input vs output packets) implement input vs. output sequence number range check. This solves some cases of transmissions that incorrectly increment sequence numbers after a SID (i.e. increment sequence number without sending a packet), in which case pktlib sees missing packets as loss and fills them in with SID reuse. See comments for more detail
  Modified Nov 2023 JHB, in analysis_and_stats() correct what appears to be a mistake in overwriting search_offset
  Modified Feb 2024 JHB, Makefile now defines NO_PKTLIB, NO_HWLIB, and STANDALONE if standalone=1 given on command line. Delete DIAGLIB_STANDALONE references
  Modified Feb 2024 JHB, increase version minor number due to changes in lib_logging.c
  Modified Mar 2024 JHB, move version string to lib_logging.c
  Modified Apr 2024 JHB, implement DS_PKT_PYLD_CONTENT_MEDIA_REUSE, rename sid_reuse_offset to search_offset
  Modified Apr 2024 JHB, add "long SID timestamp mismatch" case handling in analysis_and_stats()
  Modified May 2024 JHB, convert to cpp
  Modified May 2024 JHB, remove references to NO_PKTLIB, NO_HWLIB, and STANDALONE. DSInitLogging() in lib_logging.cpp now uses dlsym() run-time checks for pktlib and hwlib APIs to eliminate need for a separate stand-alone version of diaglib. Makefile cmd line no longer recognizes standalone=1
  Modified May 2024 JHB, update DSPktStatsAddEntries() documentation, param naming, and error handling. Make pkt_length[] param an int to allow -1 values (i.e. packet length unknown)
  Modified Jul 2024 JHB, to support SSRCs shared across streams, implement DS_PKTSTATS_ORGANIZE_COMBINE_SSRC_CHNUM flag, create in_chnum[] and out_chnum[], and add to chnum[] param to DSFindSSRCGroups() and DSPktStatsLogSeqnums()
  Modified Jul 2024 JHB, per changes in diaglib.h due to documentation review, uFlags moved to be second param in all relevant APIs. Also in non-published API analysis_and_stats() 
*/

/* Linux includes */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>  /* gettimeofday() */
#include <algorithm>   /* std::min and std::max */

using namespace std;

/* SigSRF includes */

#include "diaglib.h"

#define GET_PKT_INFO_TYPEDEF_ONLY  /* specify DSGetPacketInfo() typedef only (no prototype) in pktlib.h */
#include "pktlib.h"  /* pktlib header file, only constants and definitions used here */

#define GET_TIME_TYPEDEF_ONLY  /* specify get_time() typedef only (no prototype) in hwlib.h */
#include "hwlib.h"  /* DirectCore header file, only constants and definitions used here */

extern DEBUG_CONFIG lib_dbg_cfg;  /* in lib_logging.cpp */

extern sem_t diaglib_sem;  /* in lib_logging.cpp */
extern int diaglib_sem_init;

/* function pointers set in DSInitLogging() in lib_logging.cpp with return value of dlsym(), which looks for run-time presence of SigSRF APIs. Note hidden attribute to make sure diaglib-local functions are not confused at link-time with their SigSRF library function counterparts if they both exist, JHB May 2024 */

extern __attribute__((visibility("hidden"))) DSGetPacketInfo_t* DSGetPacketInfo;  /* DSGetPacketInfo_t typedef in pktlib.h */
extern __attribute__((visibility("hidden"))) get_time_t* get_time;  /* get_time_t typedef in hwlib.h */

/* private includes */

#include "diaglib_priv.h"

extern LOGGING_THREAD_INFO Logging_Thread_Info[];

#define DS_PKT_PYLD_CONTENT_DTMF_END  1  /* DTMF Event End, determined in DSPktStatsAddEntries and then passed thru to other functions, JHB Jun 2019 */

int DSPktStatsAddEntries(PKT_STATS* pkt_stats, unsigned int uFlags, int num_pkts, uint8_t* pkt_buffer, int pkt_length[], unsigned int payload_content[]) {

int j, len;
unsigned int offset = 0;

   if (!DSGetPacketInfo) return -2;  /* return error condition if DSGetPacketInfo() not in run-time build; see DSInitLogging() in lib_logging.cpp, JHB May 2024 */

   if (!pkt_stats || !pkt_buffer) return -1;

   for (j=0; j<num_pkts; j++) {

      if (!pkt_length || pkt_length[j] <= 0) len = -1;
      else len = pkt_length[j];

      pkt_stats->rtp_seqnum = DSGetPacketInfo(-1, DS_PKT_INFO_RTP_SEQNUM | uFlags, pkt_buffer + offset, len, NULL, NULL);
      pkt_stats->rtp_timestamp = DSGetPacketInfo(-1, DS_PKT_INFO_RTP_TIMESTAMP | uFlags, pkt_buffer + offset, len, NULL, NULL);
      pkt_stats->rtp_ssrc = DSGetPacketInfo(-1, DS_PKT_INFO_RTP_SSRC | uFlags, pkt_buffer + offset, len, NULL, NULL);
      pkt_stats->rtp_pyldlen = DSGetPacketInfo(-1, DS_PKT_INFO_RTP_PYLDLEN | uFlags, pkt_buffer + offset, len, NULL, NULL);

      if (payload_content) {

         pkt_stats->content_flags = payload_content[j];

         if ((payload_content[j] & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_DTMF) {

            unsigned int rtp_pyldoffset = DSGetPacketInfo(-1, DS_PKT_INFO_RTP_PYLDOFS | uFlags, pkt_buffer + offset, len, NULL, NULL);
            if (pkt_buffer[offset + rtp_pyldoffset + 1] & 0x80) pkt_stats->content_flags |= DS_PKT_PYLD_CONTENT_DTMF_END;
         }
      }

      if (len <= 0) len = DSGetPacketInfo(-1, DS_PKT_INFO_PKTLEN | uFlags, pkt_buffer + offset, -1, NULL, NULL);

      offset += max(0, len);

      pkt_stats += sizeof(PKT_STATS);
   }

   return j;  /* return number of entries added */
}

/* #define SIMULATE_SLOW_TIME 1  // turn this on to simulate time-consuming packet logging, for example if app debug is needed when aborting during packet logging, JHB Jan 2023 */

/* group data by unique SSRCs */

int DSFindSSRCGroups(PKT_STATS* pkts, unsigned int uFlags, int num_pkts, uint32_t ssrcs[], uint16_t chnum[], int first_pkt_idx[], int last_pkt_idx[], uint32_t first_rtp_seqnum[], uint32_t last_rtp_seqnum[]) {

int        i, j, k;
uint32_t   first_seqnum, last_seqnum;
bool       fCollated = false;
int        sorted_point;
PKT_STATS  temp_pkts;
int        seq_wrap[MAX_SSRC_TRANSITIONS] = { 0 };  /* MAX_SSRC_TRANSITIONS defined in shared_include/session.h, currently 128 */
int        ssrc_idx, num_ssrcs;
bool       fDebug = lib_dbg_cfg.uLogLevel > 8;  /* lib_dbg_cfg is in lib_logging.cpp */

#define SEARCH_WINDOW        30
#define MAX_MISSING_SEQ_GAP  20000  /* max missing seq number gap we can tolerate */

   int nThreadIndex = GetThreadIndex(true);

group_ssrcs:

/* SSRC discovery stage */

   num_ssrcs = 0;

   for (j=0; j<num_pkts; j++) {

      #ifdef SIMULATE_SLOW_TIME
      usleep(SIMULATE_SLOW_TIME);
      #endif
      if (Logging_Thread_Info[nThreadIndex].uFlags & DS_PKTLOG_ABORT) goto exit;  /* see if abort flag set, JHB Jan 2023 */

   /* first check if we've already seen this SSRC and chnum (channel number) combination */

      bool fExistingSSRC = false;

      if (num_ssrcs > 0) for (i=0; i<num_ssrcs; i++) {

         if (pkts[j].rtp_ssrc == ssrcs[i] && (!(uFlags & DS_PKTSTATS_ORGANIZE_COMBINE_SSRC_CHNUM) || pkts[j].chnum == chnum[i])) {  /* add chnum comparison, JHB Jul 2024 */

         /* this can't actually happen unless there is corruption in the ssrcs[] array */
            if (fDebug && fExistingSSRC && ssrc_idx != i) Log_RT(8, "INFO: DSFindSSRCGroups (diaglib packet logging) says SSRC 0x%x chan %d appears more than once, ssrc_idx = %d, i = %d, num_ssrcs = %d \n", pkts[j].rtp_ssrc, pkts[j].chnum, ssrc_idx, i, num_ssrcs);

            ssrc_idx = i;
            fExistingSSRC = true;
            if (!fDebug) break;
         }
      }

   /* create a new SSRC data set if needed */

      if (!fExistingSSRC) {

         ssrc_idx = num_ssrcs;

         ssrcs[ssrc_idx] = pkts[j].rtp_ssrc;
         chnum[ssrc_idx] = pkts[j].chnum;  /* note we save chnum regardless of flags, so they can be printed in log summaries, JHB Jul 2024 */
         first_pkt_idx[ssrc_idx] = j;
         last_pkt_idx[ssrc_idx] = j;
         seq_wrap[ssrc_idx] = 0;

         if (!fCollated) {  /* only search for start/end sequence numbers on first pass, JHB Jan2020 */

         /* choose first sequence number carefully, otherwise all further comparisons can be off by one or two.  So we look SEARCH_WINDOW packets ahead in case there is any ooo happening, JHB Jul2017 */

            first_seqnum = pkts[j].rtp_seqnum;
            last_seqnum = pkts[j].rtp_seqnum;  /* for last seq num this is an initial value only, will get updated by further packets with this SSRC */

            bool fWrap = false;
            for (k=1; k<SEARCH_WINDOW; k++) {  /* search a few packets ahead, in case of ooo right at the start */

               int nWrap = 0;

               if (j+k < num_pkts) {

                  if (pkts[j+k].rtp_ssrc == ssrcs[ssrc_idx] && (!(uFlags & DS_PKTSTATS_ORGANIZE_COMBINE_SSRC_CHNUM) || pkts[j+k].chnum == chnum[ssrc_idx])) {  /* add chnum comparison, JHB Jul 2024 */

#define NEW_SEQ_CALC
                     if (!nWrap && !seq_wrap[ssrc_idx]) first_seqnum = min(first_seqnum, (unsigned int)pkts[j+k].rtp_seqnum);  /* if wrap has occurred at any point for this SSRC, no longer look for first seq number */
#ifndef NEW_SEQ_CALC
                     last_seqnum = max(last_seqnum, pkts[j+k].rtp_seqnum + 65536L*nWrap);
#endif
                     if (!nWrap && pkts[j+k].rtp_seqnum == 65535L) { nWrap = 1; fWrap = true; }  /* check for seq number wrap.  Note we are sliding over a short window, so this is a local wrap check, done only once during each slide */
                  }
                  else break;  /* any change in SSRC breaks the look-ahead search */
               }
            }

            first_rtp_seqnum[ssrc_idx] = first_seqnum;
            last_rtp_seqnum[ssrc_idx] = last_seqnum;

            if (fWrap) seq_wrap[ssrc_idx]++;  /* check for seq number wrap */
         }

         num_ssrcs++;

         if (num_ssrcs >= MAX_SSRCS) {
            Log_RT(4, "INFO: DSFindSSRCGroups (diaglib packet logging) says number of SSRCs found exceeds %d \n", MAX_SSRCS);
            num_ssrcs = MAX_SSRCS-1;
         }
      }
      else {  /* update the current data set if existing SSRC */

         last_pkt_idx[ssrc_idx] = j;

         if (!fCollated) {  /* only search for start/end sequence numbers on first pass, JHB Jan2020 */

#ifndef NEW_SEQ_CALC
            if (pkts[j].rtp_seqnum + 65536L*seq_wrap[ssrc_idx] > last_rtp_seqnum[ssrc_idx]) {

               last_rtp_seqnum[ssrc_idx] = pkts[j].rtp_seqnum + 65536L*seq_wrap[ssrc_idx];
               if ((pkts[j].rtp_seqnum & 0xffff) == 65535L) seq_wrap[ssrc_idx]++;  /* check for seq number wrap */
            }
#else
            last_seqnum = pkts[j].rtp_seqnum + 65536L*seq_wrap[ssrc_idx];

         /* wraps may occur "early" due to ooo, so we don't update the end seqnum if there is too big a jump from the previous one, JHB Jan2020

            -21995.0 is a test case for this, SSRC 0x83f34914 has ooo near and around seq number 65535
            -"too big" would be 60000+ i.e. a wrong wrap
            -the abs() is operating on unsigned ints, hopefully that's ok
          */
  
            if (abs((int32_t)(last_seqnum - last_rtp_seqnum[ssrc_idx])) < MAX_MISSING_SEQ_GAP) last_rtp_seqnum[ssrc_idx] = max(last_seqnum, last_rtp_seqnum[ssrc_idx]);

            if (pkts[j].rtp_seqnum == 65535L) seq_wrap[ssrc_idx]++;  /* check for seq number wrap */
#endif
       
            #if 0  /* debug output for RTP seq num wrap */
            if (last_rtp_seqnum[ssrc_idx-1] == 65535L) printf(" $$$ rtp seq num = 65535, ssrc = 0x%x, pkt index = %d, seq wrap = %d \n", ssrcs[ssrc_idx-1], j, seq_wrap[ssrc_idx-1]);
            #endif
         }
      }
   }

   #if 0  /* debug output */
   printf("inside ssrc find groups, num ssrcs found = %d, num_pkts = %d \n", num_ssrcs, num_pkts);
   for (k=0; k<num_ssrcs; k++) printf("num_ssrcs[%d] = 0x%x \n", k, num_ssrcs[k]);
   #endif

/* if collate streams flag is active we collate streams (i.e. group SSRCs together) */

   if ((uFlags & DS_PKTSTATS_LOG_COLLATE_STREAMS) && !fCollated) {  /* use discovered first-pass SSRC groups to perform stream collation */

   /* with number of unique SSRCs known, collate streams.  NB -- took a while to get exactly right combination of j, i, and sorted_point.  Adjusting any of these by +/- 1 will break things, for example it might cause resorting of already sorted entries, which can make it hard to see what happened.  So debug carefully and use the #if 0 debug helpers if needed ... JHB Sep 2017 */

      sorted_point = 0;

      for (k=0; k<num_ssrcs-1; k++) {  /* collate N-1 unique SSRCs, last one ends up collated by default (if there is only one, no collation is needed) */

//  if (fOnce[k] < 10) { printf("before sort, sorted_point = %d, num_pkts = %d, ssrc = 0x%x\n", sorted_point, num_pkts, unique_ssrcs[k]); fOnce[k]++; }

find_transition:

         i = 0;

         for (j=sorted_point+1; j<num_pkts; j++) {

            #ifdef SIMULATE_SLOW_TIME
            usleep(SIMULATE_SLOW_TIME);
            #endif
            if (Logging_Thread_Info[nThreadIndex].uFlags & DS_PKTLOG_ABORT) goto exit;  /* see if abort flag set, JHB Jan 2023 */

            if (pkts[j].rtp_ssrc != ssrcs[k] || ((uFlags & DS_PKTSTATS_ORGANIZE_COMBINE_SSRC_CHNUM) && pkts[j].chnum != chnum[k])) {  /* add chnum comparison, JHB Jul 2024 */

               if (!i) {

                  i = j;  /* find first non-matching SSRC or chnum */
                  sorted_point = i-1;  /* adjust sorted point.  Note -- added this to fix the "orphan SSRC" number of SSRC groups problem, see comments below near in_ssrc_start and out_ssrc_start.  This also makes the sort faster, avoids unnecessary moving of already sorted enrties, JHB Feb 2019 */

//  printf("non-matching SSRC = 0x%x, current ssrc = 0x%x, i = %d\n", pkts[j].rtp_ssrc, unique_ssrcs[k], i);
               }
            }
            else if (i > sorted_point) {  /* found a match, move it up to just after its last match, and move anything in between down */

//  printf("moving pkts[%d] %u up to pkts[%d] %u\n", j, pkts[j].rtp_ssrc, i, pkts[i].rtp_ssrc);

               sorted_point = i;  /* save point of progress to prevent repeat sorting */

               memcpy(&temp_pkts, &pkts[j], sizeof(PKT_STATS));

               int l;
               for (l=j; l>i; l--) memcpy(&pkts[l], &pkts[l-1], sizeof(PKT_STATS));

               memcpy(&pkts[i], &temp_pkts, sizeof(PKT_STATS));

               goto find_transition;  /* restart the search, continue looking for non-matching SSRCs */
            }
         }
      }

      fCollated = true;
      goto group_ssrcs;  /* re-do packet indexing after collation */
   }

exit:

   return num_ssrcs;  /* return number of SSRC groups found */
}


/* print_packet_type() - helper function avoids repeating packet content labels, JHB Apr 2024 */

void print_packet_type(FILE* fp_log, unsigned int content_flags, int rtp_pyldlen, int chnum) {

   if ((content_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID) fprintf(fp_log, " (SID)");
   else if ((content_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_REUSE) fprintf(fp_log, " (SID CNG-R)");
   else if ((content_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_MEDIA_REUSE) fprintf(fp_log, " (Media-R)");
   else if ((content_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_DTMF) {
      if (content_flags & DS_PKT_PYLD_CONTENT_DTMF_END)  fprintf(fp_log, " (DTMF Event End)");
      else fprintf(fp_log, " (DTMF Event)");
   }
   else if (rtp_pyldlen > 0 && rtp_pyldlen <= 7) fprintf(fp_log, " (DTX)");

   if (chnum >= 0) fprintf(fp_log, " chnum = %d", chnum);

   fprintf(fp_log, "\n");
}


int DSPktStatsLogSeqnums(FILE* fp_log, unsigned int uFlags, PKT_STATS* pkts, int num_pkts, const char* label, uint32_t ssrcs[], uint16_t chnum[], int first_pkt_idx[], int last_pkt_idx[], uint32_t first_rtp_seqnum[], uint32_t last_rtp_seqnum[], STREAM_STATS StreamStats[]) {

int           i, j, k, nSpaces;
bool          fFound_sn, fDup_sn, fOoo_sn;
unsigned int  rtp_seqnum, dup_rtp_seqnum, ooo_rtp_seqnum, numDTX, numSIDNoData;
char          seqstr[100], tmpstr[200];
int           num_ssrcs;
int           seq_wrap[MAX_SSRC_TRANSITIONS] = { 0 };
uint32_t      max_consec_missing[MAX_SSRC_TRANSITIONS] = { 0 };
char          szLastSeq[100];

   int nThreadIndex = GetThreadIndex(true);

/* first group data by unique SSRCs */

   num_ssrcs = DSFindSSRCGroups(pkts, uFlags, num_pkts, ssrcs, chnum, first_pkt_idx, last_pkt_idx, first_rtp_seqnum, last_rtp_seqnum);

   #ifdef SIMULATE_SLOW_TIME
   usleep(SIMULATE_SLOW_TIME);
   #endif
   if (Logging_Thread_Info[nThreadIndex].uFlags & DS_PKTLOG_ABORT) goto exit;  /* see if abort flag set, JHB Jan 2023 */

   #if 0  /* debug -- see if sort looks ok */
   fprintf(fp_log, "%s sorted by SSRC (no analysis), numpkts = %d\n", label, num_pkts);
   for (j=0; j<num_pkts; j++) {

      fprintf(fp_log, "seq = %u, ssrc = 0x%x", pkts[j].rtp_seqnum, pkts[j].rtp_ssrc);

      print_packet_type(fp_log, pkts[j].content_flags, -1, -1);
   }
   fprintf(fp_log, "\n");
   #endif

   for (i=0; i<num_ssrcs; i++) memset(StreamStats[i].chnum, 0xff, sizeof(StreamStats[0].chnum));

/* for each SSRC group, fill in StreamStats[], and write stats to log file if fp_log not NULL */

   for (i=0; i<num_ssrcs; i++) {

      #if 0  /* debug output */
      printf("num_ssrcs = %d, i = %d, first j = %d, last j = %d, first seq num = %u, last seq num = %u\n", num_ssrcs, i, first_pkt_idx[i], last_pkt_idx[i], first_rtp_seqnum[i], last_rtp_seqnum[i]);
      #endif
 
      strcpy(tmpstr, "");
      for (k=i-1; k >= 0; k--) {
         if (ssrcs[i] == ssrcs[k] && (!(uFlags & DS_PKTSTATS_ORGANIZE_COMBINE_SSRC_CHNUM) || chnum[i] == chnum[k])) {  /* add chnum comparison, JHB Jul 2024 */
            strcpy(tmpstr, " (cont)");  /* annotate if this SSRC and chnum combination have appeared before */
            break;
         }
      }

      if (fp_log) {
         if (label) fprintf(fp_log, "%s ", label);
         sprintf(szLastSeq, "%u", last_rtp_seqnum[i]);
         if (uFlags & DS_PKTSTATS_LOG_SHOW_WRAPPED_SEQNUMS) sprintf(&szLastSeq[strlen(szLastSeq)], " (%u)", last_rtp_seqnum[i] & 0xffff);
         fprintf(fp_log, "Packet info for SSRC = 0x%x chnum = %d%s, first seq num = %u, last seq num = %s ...\n\n", ssrcs[i], chnum[i], tmpstr, first_rtp_seqnum[i], szLastSeq);
      }

      j = first_pkt_idx[i];

      numDTX = 0, numSIDNoData = 0;

      rtp_seqnum = first_rtp_seqnum[i];

      while (rtp_seqnum <= last_rtp_seqnum[i] && j <= last_pkt_idx[i]) {

         #ifdef SIMULATE_SLOW_TIME
         usleep(SIMULATE_SLOW_TIME);
         #endif
         if (Logging_Thread_Info[nThreadIndex].uFlags & DS_PKTLOG_ABORT) goto exit;  /* see if abort flag set, JHB Jan 2023 */

         if (StreamStats[i].chnum[max(StreamStats[i].num_chnum - 1, 0)] != pkts[j].chnum) {  /* handle "dormant SSRCs" that are taken over by another channel, JHB Jan 2020 */

            if (StreamStats[i].num_chnum < MAX_CHAN_PER_SSRC) {  /* need to review this now that we're handling SSRCs shared across streams, JHB Jul 2024 */
               StreamStats[i].chnum[StreamStats[i].num_chnum] = pkts[j].chnum;
               StreamStats[i].num_chnum++;;
            }
         }

         StreamStats[i].idx = pkts[j].idx;

         fFound_sn = false;
         fDup_sn = false;
         fOoo_sn = false;
         ooo_rtp_seqnum = 0;
         dup_rtp_seqnum = 0;

      /* first check for duplicated seq numbers.  We use a very narrow definition:  2 consecutive identical seq numbers.  If a seq number randomly repeats somewhere, we don't currently look for that */

         if (j > 0 && pkts[j].rtp_seqnum == pkts[j-1].rtp_seqnum) {  /* is it duplicated ? */

//  printf("dup, pkts[j].rtp_seqnum = %u, next seq_num = %d, pyldlen = %d\n", pkts[j].rtp_seqnum, rtp_seqnum, pkts[j].rtp_pyldlen);

            fDup_sn = true;  /* duplicated seq number found */

            if ((pkts[j].content_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_DTMF && !(uFlags & DS_PKTSTATS_LOG_MARK_DTMF_DUPLICATE)) fFound_sn = true;  /* if it's a DTMF event packet we don't label it duplicated (DTMF events can have several duplicated packets) */
         }
         else if (pkts[j].rtp_seqnum + seq_wrap[i]*65536L != rtp_seqnum) {  /* recorded seq number matches next (expected) seq number ? */

            #define OOO_SEARCH_WINDOW 30  /* possibly this should be something users can set ?  JHB Dec2019 */

            for (k=max(j-(OOO_SEARCH_WINDOW-1), first_pkt_idx[i]); k<min(j+OOO_SEARCH_WINDOW, last_pkt_idx[i]+1); k++) {  /* search +/- OOO_SEARCH_WINDOW number of packets to find ooo packets.  Allow for 2x consecutive duplicates, this is a window of +/- 1/2x ptime */

               if (pkts[k].rtp_seqnum + seq_wrap[i]*65536L == rtp_seqnum) {

                  StreamStats[i].ooo_max = max(StreamStats[i].ooo_max, (unsigned int)abs(k-j));  /* record max ooo */
                  fOoo_sn = true;  /* found ooo seq num */
                  break;
               }
            }
         }
         else fFound_sn = true;

         if (fFound_sn) strcpy(seqstr, "");
         strcpy(tmpstr, "");

         if (fOoo_sn) {

            ooo_rtp_seqnum = pkts[j].rtp_seqnum + seq_wrap[i]*65536L;
            sprintf(seqstr, "ooo %u", (uFlags & DS_PKTSTATS_LOG_SHOW_WRAPPED_SEQNUMS) ? rtp_seqnum & 0xffff : rtp_seqnum);
            StreamStats[i].ooo_seqnum++;
            max_consec_missing[i] = 0;
         }
         else if (fDup_sn) {

            if (!fFound_sn) {
               strcpy(seqstr, "dup");
               StreamStats[i].dup_seqnum++;
            }

            dup_rtp_seqnum = pkts[j].rtp_seqnum + seq_wrap[i]*65536L;
            max_consec_missing[i] = 0;
         }
         else if (!fFound_sn) {

            strcpy(seqstr, "nop");
            StreamStats[i].missing_seqnum++;
            max_consec_missing[i]++;
            StreamStats[i].max_consec_missing_seqnum = max(StreamStats[i].max_consec_missing_seqnum, max_consec_missing[i]);
         }
         else max_consec_missing[i] = 0;

         nSpaces = max(1, 12-(int)strlen(seqstr));
         for (k=0; k<nSpaces; k++) strcat(seqstr, " ");

         if (ooo_rtp_seqnum) sprintf(&tmpstr[strlen(tmpstr)], "Seq num %u %s", (uFlags & DS_PKTSTATS_LOG_SHOW_WRAPPED_SEQNUMS) ? ooo_rtp_seqnum & 0xffff : ooo_rtp_seqnum, seqstr);
         else if (fDup_sn) sprintf(&tmpstr[strlen(tmpstr)], "Seq num %u %s", (uFlags & DS_PKTSTATS_LOG_SHOW_WRAPPED_SEQNUMS) ? dup_rtp_seqnum & 0xffff : dup_rtp_seqnum, seqstr);
         else sprintf(&tmpstr[strlen(tmpstr)], "Seq num %u %s", (uFlags & DS_PKTSTATS_LOG_SHOW_WRAPPED_SEQNUMS) ? rtp_seqnum & 0xffff : rtp_seqnum, seqstr);

         if (fFound_sn || fDup_sn || fOoo_sn) {

            sprintf(&tmpstr[strlen(tmpstr)], " timestamp = %u, rtp pyld len = %u", pkts[j].rtp_timestamp, pkts[j].rtp_pyldlen);  /* changed from "pkt len =", JHB Jun 2023 */

            if ((pkts[j].content_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID) {
               StreamStats[i].numSID++;
               sprintf(&tmpstr[strlen(tmpstr)], " SID");
            }
            else if ((pkts[j].content_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_REUSE) {
               StreamStats[i].numSIDReuse++;
               sprintf(&tmpstr[strlen(tmpstr)], " SID CNG-R");
            }
            else if ((pkts[j].content_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_MEDIA_REUSE) {
               StreamStats[i].numMediaReuse++;
               sprintf(&tmpstr[strlen(tmpstr)], " media-R");
            }
            else if ((pkts[j].content_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_NODATA) {
               numSIDNoData++;
               sprintf(&tmpstr[strlen(tmpstr)], " SID NoData");
            }
            else if ((pkts[j].content_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_DTMF) {
               StreamStats[i].numDTMFEvent++;
               if (pkts[j].content_flags & DS_PKT_PYLD_CONTENT_DTMF_END) sprintf(&tmpstr[strlen(tmpstr)], " DTMF Event End");
               sprintf(&tmpstr[strlen(tmpstr)], " DTMF Event");
            }
            else if (pkts[j].rtp_pyldlen > 0 && pkts[j].rtp_pyldlen <= 7) {
               numDTX++;
               sprintf(&tmpstr[strlen(tmpstr)], " DTX");
            }
            else sprintf(&tmpstr[strlen(tmpstr)], " media");  /* added JHB Jun 2023 */

            if (pkts[j].content_flags & DS_PKT_PYLD_CONTENT_REPAIR) {

               if ((pkts[j].content_flags & ~DS_PKT_PYLD_CONTENT_REPAIR) == DS_PKT_PYLD_CONTENT_MEDIA) StreamStats[i].numMediaRepair++;
               else StreamStats[i].numSIDRepair++;

               sprintf(&tmpstr[strlen(tmpstr)], ", repaired");
            }

            j++;
         }

         if (fp_log) fprintf(fp_log, "%s\n", tmpstr);

         if (!fDup_sn) {
            rtp_seqnum++;  /* advance to next expected seq number */
            if ((rtp_seqnum & 0xffff) == 0) seq_wrap[i]++;  /* check for wrap after incrementing, JHB Jan2020 */
         }
      }

      if (fp_log) {

         fprintf(fp_log, "\n%s SSRC 0x%x chnum %d out-of-order seq numbers = %u, duplicate seq numbers = %u, missing seq numbers = %u, max consec missing seq numbers = %u", label, ssrcs[i], chnum[i], StreamStats[i].ooo_seqnum, StreamStats[i].dup_seqnum, StreamStats[i].missing_seqnum, StreamStats[i].max_consec_missing_seqnum);
         if (StreamStats[i].numSID) fprintf(fp_log, ", SID packets = %u", StreamStats[i].numSID);
         if (StreamStats[i].numSIDReuse) fprintf(fp_log, ", SID CNG-R packets = %u", StreamStats[i].numSIDReuse);
         if (StreamStats[i].numSIDRepair) fprintf(fp_log, ", repaired SID packets = %u", StreamStats[i].numSIDRepair);
         if (StreamStats[i].numMediaRepair) fprintf(fp_log, ", repaired media packets = %u", StreamStats[i].numMediaRepair);
         if (StreamStats[i].numMediaReuse) fprintf(fp_log, ", media-R packets = %u", StreamStats[i].numMediaReuse);
         if (numSIDNoData) fprintf(fp_log, ", SID CNG-N packets = %u", numSIDNoData);
         if (!StreamStats[i].numSID && !StreamStats[i].numSIDReuse && !numSIDNoData) fprintf(fp_log, ", DTX packets = %u", numDTX);
         if (StreamStats[i].numDTMFEvent) fprintf(fp_log, ", DTMF Event packets = %u", StreamStats[i].numDTMFEvent);
         fprintf(fp_log, "\n");

         if (i+1 < num_ssrcs) fprintf(fp_log, "\n");
      }
   }

exit:

   return num_ssrcs;
}


static int analysis_and_stats(FILE* fp_log, unsigned int uFlags, int num_ssrcs, uint32_t in_ssrcs[], uint16_t in_chnum[], PKT_STATS input_pkts[], int in_first_pkt_idx[], int in_last_pkt_idx[], uint32_t in_first_rtp_seqnum[], uint32_t in_last_rtp_seqnum[], STREAM_STATS InputStreamStats[], uint32_t out_ssrcs[], uint16_t out_chnum[], PKT_STATS output_pkts[], int out_first_pkt_idx[], int out_last_pkt_idx[], uint32_t out_first_rtp_seqnum[], uint32_t out_last_rtp_seqnum[], STREAM_STATS OutputStreamStats[], int in_ssrc_start, int out_ssrc_start, int io_map_ssrcs[]) {

int           i = 0, j, k, i_out, pkt_cnt;
unsigned int  rtp_seqnum, rtp_seqnum_chk, mismatch_count, search_offset = 0;
int           drop_consec_cnt, drop_cnt, dup_cnt, timestamp_mismatches, last_timestamp_mismatches, long_SID_adjust_attempts;
int           in_seq_wrap[MAX_SSRC_TRANSITIONS] = { 0 };
int           out_seq_wrap[MAX_SSRC_TRANSITIONS] = { 0 };
uint32_t      total_search_offset[MAX_SSRCS] = { 0 };
bool          fNext;

char          ssrc_indent[20] = "";
char          info_indent[20] = "  ";
char          szLastSeq[100], szStreamStr[200], szGroupStr[200] = "";
char          tmpstr[200];
int           num_in_pkts, num_out_pkts;

typedef struct {
   int output_index;
   uint32_t input_rtp_seqnum;
} found_history_t;

#define MAX_GROUPS 256  /* max number of stream groups is 256 and max streams per group is 8, defined in session.h and streamlib.h, but diaglib is supposed to be a generic tool, with no dependencies on pktlib or streamlib, JHB Dec2019 */

typedef struct {
  int num_streams;
  int streams[MAX_SSRCS];
} GROUPMAP;

GROUPMAP* GroupMap = NULL;
int nGroupIndex, stream_count, nNumGroups = 0;

   (void)in_chnum;  /* currently not used */
  
   int nThreadIndex = GetThreadIndex(true);

   if (num_ssrcs <= 0 || !fp_log) {
   
      Log_RT(3, "WARNING: analysis_and_stats() in DSPktStatsWriteLogFile() says num_ssrcs %d <= 0 or invalid packet log file handle \n", num_ssrcs);
      return -1;
   }

/* if organize-by-stream group flag is set, create a map of ssrcs to stream groups ("GroupMap") */

   if (uFlags & DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP) {

      GroupMap = (GROUPMAP*)calloc(MAX_GROUPS, sizeof(GROUPMAP));  /* as of Jan2020 there is still some problem with stack space in diaglib.  Declaring GroupMap on the stack causes a seg fault upon entry to analysis_and_stats() (even if first line is a printf, it won't print, and gdb shows nothing beyond the function header), so we're using calloc, JHB Jan2020 */

      for (i=0; i<num_ssrcs; i++) {

         if (io_map_ssrcs[i] == -1) continue;

         for (j=0; j<MAX_GROUPS; j++) {

            if (InputStreamStats[i].idx == j) {  /* does input SSRC's idx match ? (idx is group number stored by packet/media thread when it logged the packet) */

//  printf("\n==== idx[%d] %d == nGroupIndex %d \n", i, InputStreamStats[i].idx, j);

               GroupMap[j].streams[GroupMap[j].num_streams++] = i;  /* save stream and increment number of streams belonging to this group.  A group map entry is active if its num_streams is non-zero */

               break;  /* break out of group loop, group numbers are unique so InputStreamStats[].idx is not going to match another group number */
            }
         }
      }

      for (j=0, nNumGroups=0; j<MAX_GROUPS; j++) if (GroupMap[j].num_streams) {  /* determine total number of groups mapped */

         sprintf(&szGroupStr[strlen(szGroupStr)], "%s %d", nNumGroups > 0 ? "," : "", j);
         nNumGroups++;
      }

      nGroupIndex = 0;
      stream_count = 0;

      fprintf(fp_log, "\nStream groups found = %d, group indexes =%s\n", nNumGroups, szGroupStr);

      if (nNumGroups) {
         strcpy(ssrc_indent, "  ");  /* increase indent of items under stream group headings */
         strcpy(info_indent, "    ");
         strcpy(szGroupStr, "");
      }
   }
   else if (uFlags & DS_PKTSTATS_ORGANIZE_BY_CHNUM) {}  /* to-do: implement something similar for channel numbers; i.e. a "channel map" */


/* iterate through input SSRCs, search each input seq number for a match within corresponding output SSRCs, JHB Sep2017:

   -perform comparison and analysis between input and output sequence numbers
   -for example if output sequence number is not found it's a dropped packet, if found more than once it's a duplicated packet, etc
   -loop flow and termination depend on DS_PKTSTATS_ORGANIZE_BY_xx flags, see label "next_i"
*/

   do {

      if (nNumGroups) {  /* if organize-by-stream group flag is set and we found groups, get the ssrc index ("i") from the group map.  Otherwise, we simply start from 0 and increment i until num_ssrcs (which was the original coding in 2017), JHB Dec2019 */

         if (GroupMap[nGroupIndex].num_streams && stream_count < GroupMap[nGroupIndex].num_streams) {  /* process all SSRCs belonging to same stream group */

            if (stream_count == 0) {  /* print a stream group heading */
               sprintf(szGroupStr, "Stream group %d, ", nGroupIndex);  /* save string for use by Log_RT() at end of i-loop below */
               fprintf(fp_log, "\n%s%d stream%s\n", szGroupStr, GroupMap[nGroupIndex].num_streams, GroupMap[nGroupIndex].num_streams > 0 ? "s" : "");
            }

            i = GroupMap[nGroupIndex].streams[stream_count++];  /* get SSRC from map */
         }
         else {
            stream_count = 0;
            nGroupIndex++;  /* advance to next possible stream group */
            goto next_i;
         }
      }

   /* we now have an input index ("i") into ssrc data ... */

      if (io_map_ssrcs[i] == -1) goto next_i;  /* make sure i_out is never -1, which would be an error case but could happen, JHB Feb 2019 */

      i_out = io_map_ssrcs[i];  /*  ... and a corresponding output index ("i_out") into ssrc data */

//   printf("ssrc = 0x%x, in_first_pkt_idx = %d, in_last_pkt_idx = %d, out_first_pkt_idx = %d, out_last_pkt_idx = %d,\n", in_ssrcs[i], in_first_pkt_idx[i], in_last_pkt_idx[i], out_first_pkt_idx[i], out_last_pkt_idx[i]);

      num_in_pkts = in_last_pkt_idx[i+in_ssrc_start] - in_first_pkt_idx[i+in_ssrc_start] + 1;
      num_out_pkts = out_last_pkt_idx[i_out+out_ssrc_start] - out_first_pkt_idx[i_out+out_ssrc_start] + 1;

      sprintf(szStreamStr, " %d", i);  /* print a stream heading */

      if (uFlags & DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP) {  /* heading has additional info when organizing by stream group */

         sprintf(&szStreamStr[strlen(szStreamStr)], ", channel");
         sprintf(&szStreamStr[strlen(szStreamStr)], InputStreamStats[i+in_ssrc_start].num_chnum > 1 ? "s" : "");

         for (j=0; j<InputStreamStats[i+in_ssrc_start].num_chnum; j++) {
            if (j > 0) sprintf(&szStreamStr[strlen(szStreamStr)], ",");
            sprintf(&szStreamStr[strlen(szStreamStr)], " %d", InputStreamStats[i+in_ssrc_start].chnum[j]);
         }
      }

      sprintf(tmpstr, "Stream%s, SSRC = 0x%x, %d input pkts, %d output pkts", szStreamStr, in_ssrcs[i+in_ssrc_start], num_in_pkts, num_out_pkts);
      strcpy(szStreamStr, tmpstr);  /* save for use in Log_RT() at end of i-loop below */
      fprintf(fp_log,"\n%s%s\n\n", ssrc_indent, szStreamStr);

      sprintf(szLastSeq, "%u", in_last_rtp_seqnum[i+in_ssrc_start]);
      if ((uFlags & DS_PKTSTATS_LOG_SHOW_WRAPPED_SEQNUMS) && in_last_rtp_seqnum[i+in_ssrc_start] > 65535) sprintf(&szLastSeq[strlen(szLastSeq)], " (%u)", in_last_rtp_seqnum[i+in_ssrc_start] & 0xffff);
      fprintf(fp_log, "%sInput packets = %d, ooo packets = %d, SID packets = %d, seq numbers = %u..%s, missing seq numbers = %d, max consec missing seq numbers = %d\n", info_indent, num_in_pkts, InputStreamStats[i+in_ssrc_start].ooo_seqnum, InputStreamStats[i+in_ssrc_start].numSID, in_first_rtp_seqnum[i+in_ssrc_start], szLastSeq, InputStreamStats[i+in_ssrc_start].missing_seqnum, InputStreamStats[i+in_ssrc_start].max_consec_missing_seqnum);
      fprintf(fp_log, "%sInput packet loss = %2.3f%%\n", info_indent, 100.0*InputStreamStats[i+in_ssrc_start].missing_seqnum/num_in_pkts);
      fprintf(fp_log, "%sInput ooo = %2.3f%%, max ooo = %d\n", info_indent, 100.0*InputStreamStats[i+in_ssrc_start].ooo_seqnum/2/num_in_pkts, InputStreamStats[i+in_ssrc_start].ooo_max);
      fprintf(fp_log, "\n");
      sprintf(szLastSeq, "%u", out_last_rtp_seqnum[i_out+out_ssrc_start]);
      if ((uFlags & DS_PKTSTATS_LOG_SHOW_WRAPPED_SEQNUMS) && out_last_rtp_seqnum[i_out+out_ssrc_start] > 65535) sprintf(&szLastSeq[strlen(szLastSeq)], " (%u)", out_last_rtp_seqnum[i_out+out_ssrc_start] & 0xffff);
      fprintf(fp_log, "%sOutput packets = %d, ooo packets = %d, seq numbers = %u..%s, missing seq numbers = %d, max consec missing seq numbers = %d, SID packets = %d, SID-R packets = %d, media-R packets = %d, repaired SID packets = %d, repaired media packets = %d\n", info_indent, num_out_pkts, OutputStreamStats[i_out+out_ssrc_start].ooo_seqnum, out_first_rtp_seqnum[i_out+out_ssrc_start], szLastSeq, OutputStreamStats[i_out+out_ssrc_start].missing_seqnum, OutputStreamStats[i_out+out_ssrc_start].max_consec_missing_seqnum, OutputStreamStats[i_out+out_ssrc_start].numSID, OutputStreamStats[i_out+out_ssrc_start].numSIDReuse, OutputStreamStats[i_out+out_ssrc_start].numMediaReuse, OutputStreamStats[i_out+out_ssrc_start].numSIDRepair, OutputStreamStats[i_out+out_ssrc_start].numMediaRepair);
      fprintf(fp_log, "%sOutput packet loss = %2.3f%%\n", info_indent, 100.0*OutputStreamStats[i_out+out_ssrc_start].missing_seqnum/num_out_pkts);
      fprintf(fp_log, "%sOutput ooo = %2.3f%%, max ooo = %d\n", info_indent, 100.0*OutputStreamStats[i_out+out_ssrc_start].ooo_seqnum/2/num_out_pkts, OutputStreamStats[i_out+out_ssrc_start].ooo_max);

#if 0  /* debug helper, shows whether matching between input/output ssrc groups is correct */
   printf(" ====== loop %d, i_out = %d, input stream SSRC = 0x%x, output stream SSRC = 0x%x\n", i, i_out, in_ssrcs[i+in_ssrc_start], out_ssrcs[i_out+out_ssrc_start]);
#endif


   /* analyze input packets vs jitter buffer output packets, JHB Aug 2017. Updated notes, JHB Nov 2023:

      -for every input packet, all output packets are searched (we look for any input packets ooo, dropped, or duplicated). This is inefficient given that both sides of the jitter buffer have already been sorted into ascending lists, but the analysis code does not make any assumptions
      -to find an input packet in the output list, both seq numbers and timestamps need to match. Input sequence numbers are compared against output packets adjusted for DTX expansion, then checked for matching timestamps. Note that timestamps should match without adjustment of any kind
      -DTX expansion (aka SID reuse packets) are not matched, instead they increment a search offset (search_offset)
   */

      {
      drop_cnt = 0;
      drop_consec_cnt = 0;
      dup_cnt = 0;
      timestamp_mismatches = 0;
      last_timestamp_mismatches = 0;
      long_SID_adjust_attempts = 0;

      found_history_t found_history[4] = {{ 0 }};
      found_history_t timestamp_mismatch_history[16] __attribute__ ((unused)) = {{ 0 }};
      int found_index = 0, mismatch_index = 0, diff;
      int total_match_found = 0;

      rtp_seqnum = input_pkts[in_first_pkt_idx[i+in_ssrc_start]].rtp_seqnum;

      #if 0
      int timestamp_adjust = 0, last_timestamp_adjust = 0;
      #endif

   /* determine range of input and output sequence numbers */

      unsigned int in_seqnum_range = in_last_rtp_seqnum[i+in_ssrc_start] - in_first_rtp_seqnum[i+in_ssrc_start] + 1;
      unsigned int out_seqnum_range = out_last_rtp_seqnum[i_out+out_ssrc_start] - out_first_rtp_seqnum[i_out+out_ssrc_start] + 1;
      bool fEnableReuse = out_seqnum_range > in_seqnum_range;  /* enable reuse calculation based on sequence number range check: if output contains no additional (i.e. SID reuse) sequence numbers then disable the search offset, otherwise SID reuse packets inserted by pktlib as repairs for what it sees as packet loss will be wrongly interpreted here. Incorrectly incrementing sequence numbers is actually a transmission error, but we can handle it in this way. Note to Signalogic testers: this can be tested with test_files/reference_code_output_xxx pcaps in Nov 2023 time-frame, JHB Nov 2023 */

      #if 0
      printf("\n *** in_seqnum_range = %d, out_seqnum_range = %d \n", in_seqnum_range, out_seqnum_range);
      #endif

      for (j=in_first_pkt_idx[i+in_ssrc_start]; j<=in_last_pkt_idx[i+in_ssrc_start]; j++) {

         #ifdef SIMULATE_SLOW_TIME
         usleep(SIMULATE_SLOW_TIME);
         #endif
         if (Logging_Thread_Info[nThreadIndex].uFlags & DS_PKTLOG_ABORT) goto exit;  /* see if abort flag set, JHB Jan 2023 */

         rtp_seqnum_chk = input_pkts[j].rtp_seqnum + in_seq_wrap[i]*65536L;  /* input seq number */

         if (abs((int32_t)(rtp_seqnum_chk - rtp_seqnum)) < SEARCH_WINDOW) rtp_seqnum = rtp_seqnum_chk;  /* watch for case where input seq number wrapped early due to ooo, JHB Jan 2020 */
         else rtp_seqnum = input_pkts[j].rtp_seqnum + max(in_seq_wrap[i]-1, 0)*65536L;

         mismatch_count = 0;

         out_seq_wrap[i_out] = 0;  /* inner loop cycles through output packets so we need to reset this */
         pkt_cnt = 0;
         search_offset = total_search_offset[i_out];
         #if 0  /* why was this here ? looks like a mistake, JHB Nov 2023 */
         search_offset = 0;
         #endif

         for (k=out_first_pkt_idx[i_out+out_ssrc_start]; k<=out_last_pkt_idx[i_out+out_ssrc_start]; k++) {

            bool fTryRepairAsReuse = false;

check_for_reuse:

            if (fEnableReuse &&  /* increment search_offset if SID / media reuse is enabled, see sequence number range check above, JHB Nov 2023 */
                (output_pkts[k].content_flags == DS_PKT_PYLD_CONTENT_SID_REUSE || output_pkts[k].content_flags == DS_PKT_PYLD_CONTENT_MEDIA_REUSE)) {  /* note that because repaired packets fill in for missing seq nums, they do not contribute to the search offset so we don't & with item mask to remove repair flags, JHB Feb 2020 */

               search_offset++;
            }
            else {

               if (rtp_seqnum == output_pkts[k].rtp_seqnum + out_seq_wrap[i_out]*65536L - search_offset) {

                  pkt_cnt++;  /* sequence number match found */

                  if ((diff = input_pkts[j].rtp_timestamp  /* now check for timestamp match */
                     #if 0
                     + timestamp_adjust
                     #endif
                     - output_pkts[k].rtp_timestamp) != 0) {

                     #if 0  /* have not been able to get this to work. Evidently once timestamps no longer match, the amount of mismatch varies constantly. That makes it hard to print a couple of lines of output and then "get back on track".  Ends up being 100s of lines of meaningless output, JHB Feb 2020 */

                     timestamp_adjust = output_pkts[k].rtp_timestamp - input_pkts[j].rtp_timestamp;  /* update adjustment once difference stabilizes */
                     printf("ssrc 0x%x chnum %d inp seq number %u matches out seq num %u, but inp timestamp %u + adjust > out timestamp %u by = %d, adjust = %d \n", in_ssrcs[i+in_ssrc_start], in_chnum[i+in_ssrc_start], rtp_seqnum, output_pkts[k].rtp_seqnum, input_pkts[j].rtp_timestamp, output_pkts[k].rtp_timestamp, diff, timestamp_adjust);
                     #endif

                     #if 0  /* timestamp mismatch debug helper. Note - not a good idea to enable if you have 100s of mismatches */
                     printf(" ****** ssrc 0x%x inp seq number %u matches out seq num %u, but inp timestamp %u <> out timestamp %u by = %d \n", in_ssrcs[i+in_ssrc_start], rtp_seqnum, output_pkts[k].rtp_seqnum, input_pkts[j].rtp_timestamp, output_pkts[k].rtp_timestamp, diff);
                     #endif

                  /* handle case of "long SID" timestamp mismatch, where the log generator (e.g. pktlib) has repaired a long SID gap using a repeating SID length shorter than the gap. Notes JHB Apr 2024:

                     -we change the repair to a reuse, then recalculate search_offset
                     -we change output_pkts[] flag value to ensure search_offset is calculated the same for subsequent passes through the inner loop. This assumes the log has already been written to file, so we're not altering actual log output. To-do: we may need to restore the original flag; for example if analysis_and_stats() gets called again it may or may not be a problem
                     -try only once - no further effort if a timestamp mismatch still exists
                     -pktlib uses a max SID length of 8
                     -Signalogic testers: use tmpwpP7am.pcap in analytics mode, which without this will show timestamp mismatches in ssrc 0x73fc8880 starting at 3956254610
                  */

                     if (!fTryRepairAsReuse && output_pkts[k].content_flags == (DS_PKT_PYLD_CONTENT_SID | DS_PKT_PYLD_CONTENT_REPAIR)) {

                        fTryRepairAsReuse = true;  /* re-try only once per inner loop */
                        pkt_cnt--;  /* undo match found */

                        output_pkts[k].content_flags = DS_PKT_PYLD_CONTENT_MEDIA_REUSE;  /* change the info flag */
                        long_SID_adjust_attempts++;  /* increment stat for this */

                        #if 0
                        printf("\n *** before retry, mismatch_count = %d , input timestamp = %u, output timestamp = %u, input seqnum = %u, search offset = %u \n", mismatch_count, input_pkts[j].rtp_timestamp, output_pkts[k].rtp_timestamp, rtp_seqnum, search_offset);
                        #endif
                        goto check_for_reuse;  /* recalculate the search offset */
                     }

                     timestamp_mismatch_history[mismatch_index].output_index = k;
                     timestamp_mismatch_history[mismatch_index].input_rtp_seqnum = rtp_seqnum;
                     mismatch_index = (mismatch_index+1) & (16-1);

                     mismatch_count++;

//                     set prior_timestamp to input_pkts[j].rtp_timestamp
//                     next iteration compare output_pkts[timestamp_mismatch_history[index].output_index].rtp_timestamp with prior_timestamp
                  }

                  found_history[found_index].output_index = k;
                  found_history[found_index].input_rtp_seqnum = rtp_seqnum;
                  found_index = (found_index+1) & (4-1);
                  total_match_found++;

                  #if 0  /* no because otherwise we may miss duplicates */
                  break;  /* break out of inner loop: input seq num found, timestamp match found, no further searching required */
                  #endif
               }
            }

            if (output_pkts[k].rtp_seqnum == 65535L) out_seq_wrap[i_out]++;
         }

#if 0  /* debug */
         static bool fOnce = false;

         if (!pkt_cnt && !fOnce) {

            search_offset = total_search_offset[i_out];

            for (k=out_first_pkt_idx[i_out+out_ssrc_start]; k<=out_last_pkt_idx[i_out+out_ssrc_start]; k++) {

               if ((output_pkts[k].content_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_REUSE) search_offset++;
               else fprintf(fp_log, "no match, rtp_seqnum = %d, output_pkts[%d].rtp_seqnum = %d, search_offset = %d\n", rtp_seqnum, k, output_pkts[k].rtp_seqnum + out_seq_wrap[i_out]*65536L, search_offset);
            }

            fOnce = true;
         }
#endif

         if (!pkt_cnt) {  /* count packets not found as dropped by the jitter buffer */

            int sp, splen;
            #define COLUMN2 32  /* assumes max 10 digit number for %u uint32_t */

            if (!drop_consec_cnt) {

               if (total_match_found >= 2) {

                  int history_index = (found_index-2) & 3;
                  int out_index = found_history[history_index].output_index;

                  sprintf(tmpstr, "%sInput seq num %u corresponds to output seq num %u", info_indent, found_history[history_index].input_rtp_seqnum, (unsigned int)(output_pkts[out_index].rtp_seqnum + out_seq_wrap[i_out]*65536L));  /* in_seq_wrap[] is cumulative so it's not correct here, JHB Jan 2020 */
                  splen = max(COLUMN2 - (int)strlen(tmpstr), 1);
                  for (sp = 0; sp < splen; sp++) sprintf(&tmpstr[strlen(tmpstr)], " ");
                  fprintf(fp_log, "%stimestamp = %u, rtp len = %u\n", tmpstr, output_pkts[out_index].rtp_timestamp, output_pkts[out_index].rtp_pyldlen);
               }

               if (total_match_found >= 1) {

                  int history_index = (found_index-1) & 3;
                  int out_index = found_history[history_index].output_index;

                  sprintf(tmpstr, "%sInput seq num %u corresponds to output seq num %u", info_indent, found_history[history_index].input_rtp_seqnum, (unsigned int)(output_pkts[out_index].rtp_seqnum + out_seq_wrap[i_out]*65536L));
                  splen = max(COLUMN2 - (int)strlen(tmpstr), 1);
                  for (sp = 0; sp < splen; sp++) sprintf(&tmpstr[strlen(tmpstr)], " ");  
                  fprintf(fp_log, "%stimestamp = %u, rtp len = %u\n", tmpstr, output_pkts[out_index].rtp_timestamp, output_pkts[out_index].rtp_pyldlen);
               }
            }

            drop_cnt++;

            sprintf(tmpstr, "%sDrop %d: input seq num %u not found", info_indent, drop_cnt, rtp_seqnum);
            splen = max(COLUMN2 - (int)strlen(tmpstr), 1);
            for (sp = 0; sp < splen; sp++) sprintf(&tmpstr[strlen(tmpstr)], " ");  
            fprintf(fp_log, "%stimestamp = %u, rtp len = %u", tmpstr, input_pkts[j].rtp_timestamp, input_pkts[j].rtp_pyldlen);

            print_packet_type(fp_log, input_pkts[j].content_flags, input_pkts[j].rtp_pyldlen, -1);

            drop_consec_cnt++;
         }
         else if (pkt_cnt > 1) {

            if ((input_pkts[j].content_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) != DS_PKT_PYLD_CONTENT_DTMF || (uFlags & DS_PKTSTATS_LOG_MARK_DTMF_DUPLICATE)) {

               dup_cnt++;

               strcpy(tmpstr, "");
               for (k=0; k<pkt_cnt; k++) sprintf(&tmpstr[strlen(tmpstr)], " %u", (unsigned int)(output_pkts[found_history[(found_index-k) & 3].output_index].rtp_seqnum + out_seq_wrap[i_out]*65536L));
               fprintf(fp_log, "%sDuplicate %d: input seq num %u corresponds to output seq nums%s, input rtp len = %u", info_indent, dup_cnt, rtp_seqnum, tmpstr, input_pkts[j].rtp_pyldlen);

               print_packet_type(fp_log, input_pkts[j].content_flags, input_pkts[j].rtp_pyldlen, -1);
            }

            drop_consec_cnt = 0;
         }
         else drop_consec_cnt = 0;

         if (mismatch_count) {

            timestamp_mismatches++;

            if (timestamp_mismatches < 4) {

            /* print initial mismatch history ... it's difficult to be comprehensive once timestamps encounter an initial mismatch; see comments above near timestamp_adjust */

               for (k=0; k<timestamp_mismatches-last_timestamp_mismatches; k++) {
                  int index = (mismatch_index - (k+1)) & (16-1);
                  fprintf(fp_log, "%sTimestamp mismatch %d: inp seq number %u corresponds to out seq num %u, but inp timestamp %u != out timestamp %u \n", info_indent, timestamp_mismatches, timestamp_mismatch_history[index].input_rtp_seqnum, (unsigned int)(output_pkts[timestamp_mismatch_history[index].output_index].rtp_seqnum + out_seq_wrap[i_out]*65536L), input_pkts[j].rtp_timestamp, output_pkts[timestamp_mismatch_history[index].output_index].rtp_timestamp);
               }
            }

            last_timestamp_mismatches = timestamp_mismatches;
         }

         if ((rtp_seqnum & 0xffff) == 65535L) in_seq_wrap[i]++;

      }  /* end of j loop */
      }

      total_search_offset[i_out] = search_offset;

      for (k=i_out+1; k<num_ssrcs; k++) {  /* update total search offset for any subsequent output SSRC stream that has same SSRC number as the SSRC stream just processed; i.e. if they are a resumption of the current stream, JHB Sep2017 */

         if (out_ssrcs[k+out_ssrc_start] == out_ssrcs[i_out+out_ssrc_start] && (!(uFlags & DS_PKTSTATS_ORGANIZE_COMBINE_SSRC_CHNUM) || out_chnum[k+out_ssrc_start] == out_chnum[i_out+out_ssrc_start])) {

            total_search_offset[k] = total_search_offset[i_out];
//            printf("i_out = %u, updating total_search_offset[%d] = %u\n", i_out, k, total_search_offset[k]);
         }
      }

      fprintf(fp_log, "\n");

      if (uFlags & DS_PKTSTATS_LOG_EVENT_LOG_SUMMARY) {
         if (strlen(szGroupStr) && szGroupStr[0] == 'S') szGroupStr[0] = 's';
         if (strlen(szStreamStr) && szStreamStr[0] == 'S') szStreamStr[0] = 's';
         Log_RT(4, "INFO: DSPktStatsWriteLogFile() packet history analysis summary for %s%s\n", szGroupStr, szStreamStr);
      }

      sprintf(tmpstr, "%sPackets dropped by jitter buffer = %u\n", info_indent, drop_cnt);
      if (uFlags & DS_PKTSTATS_LOG_EVENT_LOG_SUMMARY) Log_RT(4, "  %s", tmpstr);
      fprintf(fp_log, "%s", tmpstr);

      sprintf(tmpstr, "%sPackets duplicated by jitter buffer = %u\n", info_indent, dup_cnt);
      if (uFlags & DS_PKTSTATS_LOG_EVENT_LOG_SUMMARY) Log_RT(4, "  %s", tmpstr);
      fprintf(fp_log, "%s", tmpstr);

      char tmpstr2[50];
      sprintf(tmpstr2, ", long SID adjust attempts = %u", long_SID_adjust_attempts);
      sprintf(tmpstr, "%sTimestamp mismatches = %u%s\n", info_indent, timestamp_mismatches, long_SID_adjust_attempts ? tmpstr2 : "");
      if (uFlags & DS_PKTSTATS_LOG_EVENT_LOG_SUMMARY) Log_RT(4, "  %s", tmpstr);
      fprintf(fp_log, "%s", tmpstr);

next_i:

      if (nNumGroups) {  /* if organize-by-stream group flag is set and we found groups, we stay in the loop until all possible stream group indexes have been looked at, JHB Dec2019 */

         fNext = (nGroupIndex < MAX_GROUPS);
      }
      else fNext = ++i < num_ssrcs;  /* if no flags set, then increment ssrc index and compare with num_ssrcs, JHB Dec2019 */

   } while (fNext);  /* end of i (ssrc) loop */

exit:

   if (GroupMap) free(GroupMap);

   return 1;
}


int DSPktStatsWriteLogFile(const char* szLogfile, unsigned int uFlags, PKT_STATS* input_pkts, PKT_STATS* output_pkts, PKT_COUNTERS* pkt_counters) {

FILE*         fp_log = NULL;
int           input_idx = 0, output_idx = 0;
int           ret_code = 0;
int           i, j, k, num_ssrcs;

int           in_ssrc_groups = 0;
int           out_ssrc_groups = 0;

#if 0  /* move these off the stack as they seemed to be causing sporadic seg-faults, can't even get to first line in the function.  MAX_SSRCS is 65536 (defined in diaglib.h) so this is around 2 MB of total space, maybe not a good idea. JHB Sep 2018 */
int           in_first_pkt_idx[MAX_SSRCS] = { 0 };
int           in_last_pkt_idx[MAX_SSRCS] = { 0 };
uint32_t      in_first_rtp_seqnum[MAX_SSRCS] = { 0 };
uint32_t      in_last_rtp_seqnum[MAX_SSRCS] = { 0 };
uint32_t      in_ssrcs[MAX_SSRCS] = { 0 };
uint16_t      in_chnum[MAX_SSRCS] = { 0 };

int           out_first_pkt_idx[MAX_SSRCS] = { 0 };
int           out_last_pkt_idx[MAX_SSRCS] = { 0 };
uint32_t      out_first_rtp_seqnum[MAX_SSRCS] = { 0 };
uint32_t      out_last_rtp_seqnum[MAX_SSRCS] = { 0 };
uint32_t      out_ssrcs[MAX_SSRCS] = { 0 };
uint16_t      out_chnum[MAX_SSRCS] = { 0 };

int           io_map_ssrcs[MAX_SSRCS] = { 0 };
int           used_map_ssrcs[MAX_SSRCS] = { 0 };

#else

int*          in_first_pkt_idx = NULL;  /* packet index math in diaglib.cpp depends on indexes being int.  Don't change these to uint32_t, JHB Oct 2019 */ 
int*          in_last_pkt_idx = NULL;
uint32_t*     in_first_rtp_seqnum = NULL;
uint32_t*     in_last_rtp_seqnum = NULL;
uint32_t*     in_ssrcs = NULL;
uint16_t*     in_chnum = NULL;

int*          out_first_pkt_idx = NULL;
int*          out_last_pkt_idx = NULL;
uint32_t*     out_first_rtp_seqnum = NULL;
uint32_t*     out_last_rtp_seqnum = NULL;
uint32_t*     out_ssrcs = NULL;
uint16_t*     out_chnum = NULL;

int*          io_map_ssrcs = NULL;
int*          used_map_ssrcs = NULL;

STREAM_STATS*  InputStreamStats = NULL;
STREAM_STATS*  OutputStreamStats = NULL;

#endif

uint64_t       t1, t2;
int            nThreadIndex;

   if (uFlags & DS_PKTSTATS_LOG_APPEND) fp_log = fopen(szLogfile, "ab");
   else fp_log = fopen(szLogfile, "wb");

   if (!fp_log) {
      ret_code = 0;
      goto cleanup;
   }

   nThreadIndex = GetThreadIndex(true);

   in_first_pkt_idx = (int*)calloc(MAX_SSRCS, sizeof(int));
   in_last_pkt_idx = (int*)calloc(MAX_SSRCS, sizeof(int));
   in_first_rtp_seqnum = (uint32_t*)calloc(MAX_SSRCS, sizeof(uint32_t));
   in_last_rtp_seqnum = (uint32_t*)calloc(MAX_SSRCS, sizeof(uint32_t));
   in_ssrcs = (uint32_t*)calloc(MAX_SSRCS, sizeof(uint32_t));
   in_chnum = (uint16_t*)calloc(MAX_SSRCS, sizeof(uint16_t));

   out_first_pkt_idx = (int*)calloc(MAX_SSRCS, sizeof(int));
   out_last_pkt_idx = (int*)calloc(MAX_SSRCS, sizeof(int));
   out_first_rtp_seqnum = (uint32_t*)calloc(MAX_SSRCS, sizeof(uint32_t));
   out_last_rtp_seqnum = (uint32_t*)calloc(MAX_SSRCS, sizeof(uint32_t));
   out_ssrcs = (uint32_t*)calloc(MAX_SSRCS, sizeof(uint32_t));
   out_chnum = (uint16_t*)calloc(MAX_SSRCS, sizeof(uint16_t));

   io_map_ssrcs = (int*)calloc(MAX_SSRCS, sizeof(int));
   used_map_ssrcs = (int*)calloc(MAX_SSRCS, sizeof(int));

   InputStreamStats = (STREAM_STATS*)calloc(MAX_SSRCS, sizeof(STREAM_STATS));
   OutputStreamStats = (STREAM_STATS*)calloc(MAX_SSRCS, sizeof(STREAM_STATS));

   if (get_time) t1 = get_time(USE_CLOCK_GETTIME);
   else {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      t1 = tv.tv_sec * 1000000L + tv.tv_usec;
   }

   fprintf(fp_log, "** Packet Ingress Stats **\n\n");

   if (!pkt_counters) fprintf(fp_log, "DSPktStatsWriteLogFile:  PKT_COUNTERS* arg is NULL\n"); 
   else {

      fprintf(fp_log, "Total packets read from pcap = %d\n", pkt_counters->pkt_read_cnt);
      fprintf(fp_log, "Total packets input from network socket = %d\n", pkt_counters->pkt_input_cnt);

      if (uFlags & DS_PKTSTATS_LOG_PACKETMODE) {

         fprintf(fp_log, "Total packets submitted to jitter buffer = %d\n", pkt_counters->pkt_submit_to_jb_cnt);
         fprintf(fp_log, "Total packets successfully added to jitter buffer = %d\n", pkt_counters->pkt_add_to_jb_cnt);
      }

      if (uFlags & DS_PKTSTATS_LOG_FRAMEMODE) fprintf(fp_log, "Total packet payloads extracted and successfully decoded = %d\n", pkt_counters->num_input_pkts);  /* frame mode */

      fprintf(fp_log, "\n");

      input_idx = pkt_counters->num_input_pkts;
   }

   if (input_idx) {

      if (uFlags & DS_PKTSTATS_LOG_LIST_ALL_INPUT_PKTS) {  /* list all input packets for reference / debug */

         for (j=0; j<input_idx; j++) {

            fprintf(fp_log, "seq = %u, ssrc = 0x%x", input_pkts[j].rtp_seqnum, input_pkts[j].rtp_ssrc);

            print_packet_type(fp_log, input_pkts[j].content_flags, -1, input_pkts[j].chnum);
         }
         fprintf(fp_log, "\n");
      }

      if (uFlags & DS_PKTSTATS_LOG_RFC7198_DEBUG) {  /* RFC 7198 debug (this is handled by pktlib, if duplicated packets are being incorrectly added to the jitter buffer turn this on to look for them) */

         int delay_intervals_sum = 0, last_j = 0;

         k = 0;
         for (j=0; j<input_idx; j++) {

            if (j+1 < input_idx && input_pkts[j].rtp_seqnum == input_pkts[j+1].rtp_seqnum) {
 
               delay_intervals_sum += j - last_j;
               last_j = j;
            }
         }

         if (input_idx - delay_intervals_sum < input_idx/20) {  /* yes, delay intervals are regular to within 5% of total packets, remove duplicated seq numbers */

            j = 0;
            for (k=0; k<input_idx; k++) {

               if (input_pkts[k].rtp_seqnum != input_pkts[k+1].rtp_seqnum) input_pkts[j++] = input_pkts[k];  /* keep only non-duplicated */
            }

            input_idx = j;
         }
      }

   /* log ingress packet info -- group by SSRC values, including ooo and missing seq numbers */

      in_ssrc_groups = DSPktStatsLogSeqnums(fp_log, uFlags, input_pkts, input_idx, "Ingress", in_ssrcs, in_chnum, in_first_pkt_idx, in_last_pkt_idx, in_first_rtp_seqnum, in_last_rtp_seqnum, InputStreamStats);

      #ifdef SIMULATE_SLOW_TIME
      usleep(SIMULATE_SLOW_TIME);
      #endif

      if (Logging_Thread_Info[nThreadIndex].uFlags & DS_PKTLOG_ABORT) goto cleanup;  /* see if abort flag set, JHB Jan 2023 */
   }

/* log jitter buffer stats */

   if (!fp_log) {
   
      fp_log = fopen(szLogfile, "wb");

      if (!fp_log) {
         ret_code = -1;
         goto cleanup;
      }
   }
   else fprintf(fp_log, "\n\n");

   fprintf(fp_log, "** Jitter Buffer Stats **\n\n");

   if (!pkt_counters) fprintf(fp_log, "DSPktStatsWriteLogFile:  PKT_COUNTERS* arg is NULL\n");
   else {

      fprintf(fp_log, "Total packets pulled from buffer = %d\n", pkt_counters->num_pulled_pkts);

      output_idx = pkt_counters->num_pulled_pkts;
   }

   if (input_idx || output_idx) {

      fprintf(fp_log, "\n");

      if (uFlags & DS_PKTSTATS_LOG_LIST_ALL_PULLED_PKTS) {  /* list all pulled packets for reference / debug */

         for (j=0; j<output_idx; j++) {

            fprintf(fp_log, "seq = %u, ssrc = 0x%x", output_pkts[j].rtp_seqnum, output_pkts[j].rtp_ssrc);

            print_packet_type(fp_log, output_pkts[j].content_flags, -1, output_pkts[j].chnum);
         }
         fprintf(fp_log, "\n");
      }

   /* log jitter buffer output packet info -- grouped by SSRC values, including ooo and missing seq numbers */

      out_ssrc_groups = DSPktStatsLogSeqnums(fp_log, uFlags, output_pkts, output_idx, "Jitter Buffer", out_ssrcs, out_chnum, out_first_pkt_idx, out_last_pkt_idx, out_first_rtp_seqnum, out_last_rtp_seqnum, OutputStreamStats);

      #ifdef SIMULATE_SLOW_TIME
      usleep(SIMULATE_SLOW_TIME);
      #endif

      if (Logging_Thread_Info[nThreadIndex].uFlags & DS_PKTLOG_ABORT) goto cleanup;  /* see if abort flag set, JHB Jan 2023 */

      if (get_time) t2 = get_time(USE_CLOCK_GETTIME);
      else {
         struct timeval tv;
         gettimeofday(&tv, NULL);
         t2 = tv.tv_sec * 1000000L + tv.tv_usec;
      }

      char tstr[] = "msec";
      float ltime = (t2-t1)/1000.0;
      if (ltime > 100) { ltime = (t2-t1)/1000000.0; strcpy(tstr, "sec"); }
      char instr[] = "streams", outstr[] = "streams";
      if (in_ssrc_groups == 1) instr[strlen(instr)-1] = 0;
      if (out_ssrc_groups == 1) outstr[strlen(outstr)-1] = 0;
      t1 = t2;

      Log_RT(4, "INFO: DSPktStatsWriteLogFile() says %d input SSRC %s with %d total packets and %d output SSRC %s with %d total packets logged in %2.1f %s, now analyzing...\n", in_ssrc_groups, instr, input_idx, out_ssrc_groups, outstr, output_idx, ltime, tstr);

// printf("after log seqnums\n");

      fprintf(fp_log, "\n** Packet Stats and Analysis **\n");

   /* compare output/pulled packets with input/buffered packets */

      num_ssrcs = in_ssrc_groups;

      int in_ssrc_start = 0, out_ssrc_start = 0;

      if (in_ssrc_groups < out_ssrc_groups) {

         char whichstr[10];

         if (out_last_pkt_idx[0] - out_first_pkt_idx[0] > 0) {  /* NB - the "sort bug" is fixed, but leaving this code here just in case, JHB Feb 2019.  Original comment:  seems to be a sorting bug with collate streams enabled, where the first SSRC group can end up with only entry, and all of its other entries are in another group.  This hack looks for a "one entry orphan group" and if found, ignores it during analysis, JHB Aug 2018 */
            strcpy(whichstr, "last");
         }
         else {
            strcpy(whichstr, "first");
            out_ssrc_start = 1;
         }

         fprintf(fp_log, "\nNumber of input SSRC(s) %d less than number of output SSRC(s) %d, not comparing with %s %d output SSRC(s)\n", in_ssrc_groups, out_ssrc_groups, whichstr, out_ssrc_groups-in_ssrc_groups);
         num_ssrcs = in_ssrc_groups;
      }
      else if (out_ssrc_groups < in_ssrc_groups) {

         char whichstr[10];

         if (in_last_pkt_idx[0] - in_first_pkt_idx[0] > 0) {  /* NB - the "sorting bug" is fixed, but leaving this code here just in case, JHB Feb 2019.  Orginal comment:  seems to be a sorting bug with collate streams enabled, where the first SSRC group can end up with only entry, and all of its other entries are in another group.  This hack looks for a "one entry orphan group" and if found, ignores it during analysis, JHB Aug 2018 */
            strcpy(whichstr, "last");
         }
         else {
            strcpy(whichstr, "first");
            in_ssrc_start = 1;
         }

         fprintf(fp_log, "\nNumber of output SSRC(s) %d less than number of input SSRC(s) %d, not comparing with %s %d input SSRC(s)\n", out_ssrc_groups, in_ssrc_groups, whichstr, in_ssrc_groups-out_ssrc_groups);
         num_ssrcs = out_ssrc_groups;
      }   

   /* before comparing / analyzing input vs. output SSRC groups, we match up the groups, in case their order is different.  JHB Jul 2018 */

//      for (i=0; i<num_ssrcs; i++) printf("in_ssrcs[%u] = 0x%x\n", i, in_ssrcs[i]);
//      for (i=0; i<num_ssrcs; i++) printf("out_ssrcs[%u] = 0x%x\n", i, out_ssrcs[i]);

      memset(io_map_ssrcs, -1, MAX_SSRCS*sizeof(int));  /* initialize all mapping values to -1 */
      memset(used_map_ssrcs, -1, MAX_SSRCS*sizeof(int));

      for (i=0; i<num_ssrcs; i++) {

         for (j=0; j<num_ssrcs; j++) {

            if ((io_map_ssrcs[i] == -1) && (used_map_ssrcs[j] == -1) && (in_ssrcs[i+in_ssrc_start] == out_ssrcs[j+out_ssrc_start]) && (!(uFlags & DS_PKTSTATS_ORGANIZE_COMBINE_SSRC_CHNUM) || (in_chnum[i+in_ssrc_start] == out_chnum[j+out_ssrc_start]))) {

               io_map_ssrcs[i] = j;
               used_map_ssrcs[j] = i;  /* we set both sides of the mapping, as it has to be 1:1 relationship, no entry on one side or the other mapped twice */
//             printf("in_ssrcs[%d] = 0x%x, out_ssrcs[%d] = 0x%x, io_map_ssrcs[%d] = %u\n", i, in_ssrcs[i], j, out_ssrcs[j], i, io_map_ssrcs[i]);
               break;
            }
         }
      }

      for (i=0; i<num_ssrcs; i++) if (io_map_ssrcs[i] == -1) fprintf(fp_log, "\nCorresponding output SSRC group not found for input SSRC 0x%x chnum %d, group %u\n", in_ssrcs[i+in_ssrc_start], in_chnum[i+in_ssrc_start], i);


   /* call analysis_and_stats() as needed, depending on uFlags */

      unsigned int uFlags_as;
      int ret_val = 1;

      if (uFlags & DS_PKTSTATS_ORGANIZE_BY_SSRC) {

         uFlags_as = uFlags & ~DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP & ~DS_PKTSTATS_ORGANIZE_BY_CHNUM;
         ret_val = analysis_and_stats(fp_log, uFlags_as, num_ssrcs, in_ssrcs, in_chnum, input_pkts, in_first_pkt_idx, in_last_pkt_idx, in_first_rtp_seqnum, in_last_rtp_seqnum, InputStreamStats, out_ssrcs, out_chnum, output_pkts, out_first_pkt_idx, out_last_pkt_idx, out_first_rtp_seqnum, out_last_rtp_seqnum, OutputStreamStats, in_ssrc_start, out_ssrc_start, io_map_ssrcs);

         #ifdef SIMULATE_SLOW_TIME
         usleep(SIMULATE_SLOW_TIME);
         #endif

         if (Logging_Thread_Info[nThreadIndex].uFlags & DS_PKTLOG_ABORT) goto cleanup;  /* see if abort flag set, JHB Jan 2023 */
      }

      if (ret_val > 0 && (uFlags & DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP)) {

         uFlags_as = uFlags & ~DS_PKTSTATS_ORGANIZE_BY_SSRC & ~DS_PKTSTATS_ORGANIZE_BY_CHNUM;
         analysis_and_stats(fp_log, uFlags_as, num_ssrcs, in_ssrcs, in_chnum, input_pkts, in_first_pkt_idx, in_last_pkt_idx, in_first_rtp_seqnum, in_last_rtp_seqnum, InputStreamStats, out_ssrcs, out_chnum, output_pkts, out_first_pkt_idx, out_last_pkt_idx, out_first_rtp_seqnum, out_last_rtp_seqnum, OutputStreamStats, in_ssrc_start, out_ssrc_start, io_map_ssrcs);

         #ifdef SIMULATE_SLOW_TIME
         usleep(SIMULATE_SLOW_TIME);
         #endif

         if (Logging_Thread_Info[nThreadIndex].uFlags & DS_PKTLOG_ABORT) goto cleanup;  /* see if abort flag set, JHB Jan 2023 */
      }

      if (ret_val > 0 && (uFlags & DS_PKTSTATS_ORGANIZE_BY_CHNUM)) {

         uFlags_as = uFlags & ~DS_PKTSTATS_ORGANIZE_BY_SSRC & ~DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP;
         analysis_and_stats(fp_log, uFlags_as, num_ssrcs, in_ssrcs, in_chnum, input_pkts, in_first_pkt_idx, in_last_pkt_idx, in_first_rtp_seqnum, in_last_rtp_seqnum, InputStreamStats, out_ssrcs, out_chnum, output_pkts, out_first_pkt_idx, out_last_pkt_idx, out_first_rtp_seqnum, out_last_rtp_seqnum, OutputStreamStats, in_ssrc_start, out_ssrc_start, io_map_ssrcs);

         #ifdef SIMULATE_SLOW_TIME
         usleep(SIMULATE_SLOW_TIME);
         #endif

         if (Logging_Thread_Info[nThreadIndex].uFlags & DS_PKTLOG_ABORT) goto cleanup;  /* see if abort flag set, JHB Jan 2023 */
      }
   }

   if (!fp_log) fp_log = fopen(szLogfile, "wb");
   else fprintf(fp_log, "\n");

   fprintf(fp_log, "** Packet Egress Stats **\n\n");

   fprintf(fp_log, "Total packets written to pcap = %d\n", pkt_counters->pkt_write_cnt);
   fprintf(fp_log, "Total packets output to network socket = %d\n", pkt_counters->pkt_output_cnt);
   fprintf(fp_log, "Total packets decoded and written to wav file = %d\n", pkt_counters->frame_write_cnt);

   if (get_time) t2 = get_time(USE_CLOCK_GETTIME);
   else {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      t2 = tv.tv_sec * 1000000L + tv.tv_usec;
   }

   {
      char tstr[] = "msec";
      float ltime = (t2-t1)/1000.0;
      if (ltime > 100) { ltime = (t2-t1)/1000000.0; strcpy(tstr, "sec"); }

      Log_RT(4, "INFO: DSPktStatsWriteLogFile() says packet log analysis completed in %2.1f %s, packet log file = %s\n", ltime, tstr, szLogfile);
   }

   ret_code = 1;

cleanup:

   if (fp_log) fclose(fp_log);

   if (in_first_pkt_idx) free(in_first_pkt_idx);
   if (in_last_pkt_idx) free(in_last_pkt_idx);
   if (in_first_rtp_seqnum) free(in_first_rtp_seqnum);
   if (in_last_rtp_seqnum) free(in_last_rtp_seqnum);
   if (in_ssrcs) free(in_ssrcs);
   if (in_chnum) free(in_chnum);

   if (out_first_pkt_idx) free(out_first_pkt_idx);
   if (out_last_pkt_idx) free(out_last_pkt_idx);
   if (out_first_rtp_seqnum) free(out_first_rtp_seqnum);
   if (out_last_rtp_seqnum) free(out_last_rtp_seqnum);
   if (out_ssrcs) free(out_ssrcs);
   if (out_chnum) free(out_chnum);

   if (io_map_ssrcs) free(io_map_ssrcs);
   if (used_map_ssrcs) free(used_map_ssrcs);

   if (InputStreamStats) free(InputStreamStats);
   if (OutputStreamStats) free(OutputStreamStats);

   return ret_code;
}
