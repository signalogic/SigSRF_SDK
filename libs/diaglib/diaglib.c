/*
 $Header: /root/Signalogic/DirectCore/lib/diaglib/diaglib.c

 Copyright Signalogic Inc. 2017-2021

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md
 
 Description

  packet diagnostic library with APIs for:

  -packet tracing and history logging
  -packet statistics, including ooo (out-of-order), DTX, packet loss and gaps, timestamp integrity, etc.
  -packet analysis, including input vs. jitter buffer output analysis

 Project

  DirectCore, SigSRF

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
  Modified Mar 2021 JHB, add DIAGLIB_STANDALONE #define option to build without DirectCore header file
*/

/* Linux includes */

#include <stdio.h>
#include <stdlib.h>

/* Sig includes */

#include "diaglib.h"
#include "pktlib.h"

#ifndef DIAGLIB_STANDALONE
  #include "hwlib.h"  /* DirectCore header file, used here for get_time(), define STANDALONE to use getimeofday() and omit hwlib.h */
#endif

extern DEBUG_CONFIG lib_dbg_cfg;  /* in lib_logging.c */


#define DS_PKT_PYLD_CONTENT_DTMF_END  1         /* DTMF Event End, determined in DSPktStatsAddEntries and then passed thru to other functions, JHB Jun 2019 */

/* diaglib version string */
const char DIAGLIB_VERSION[256] = "1.5.0";

int DSPktStatsAddEntries(PKT_STATS* pkt_stats, int num_pkts, uint8_t* pkt_buffer, unsigned int packet_length[], unsigned int packet_info[], unsigned int uFlags) {

int j;
unsigned int offset = 0;

   for (j=0; j<num_pkts; j++) {

      pkt_stats->rtp_seqnum = DSGetPacketInfo(-1, DS_PKT_INFO_RTP_SEQNUM | uFlags, pkt_buffer + offset, packet_length[j], NULL, NULL);
      pkt_stats->rtp_timestamp = DSGetPacketInfo(-1, DS_PKT_INFO_RTP_TIMESTAMP | uFlags, pkt_buffer + offset, packet_length[j], NULL, NULL);
      pkt_stats->rtp_SSRC = DSGetPacketInfo(-1, DS_PKT_INFO_RTP_SSRC | uFlags, pkt_buffer + offset, packet_length[j], NULL, NULL);
      pkt_stats->rtp_pyldlen = DSGetPacketInfo(-1, DS_PKT_INFO_RTP_PYLDLEN | uFlags, pkt_buffer + offset, packet_length[j], NULL, NULL);

      if (packet_info) {

         pkt_stats->info_flags = packet_info[j];

         if ((packet_info[j] & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_DTMF) {

            unsigned int rtp_pyldoffset = DSGetPacketInfo(-1, DS_PKT_INFO_RTP_PYLDOFS | uFlags, pkt_buffer + offset, packet_length[j], NULL, NULL);
            if (pkt_buffer[offset + rtp_pyldoffset + 1] & 0x80) pkt_stats->info_flags |= DS_PKT_PYLD_CONTENT_DTMF_END;
         }
      }

      offset += packet_length[j];

      pkt_stats += sizeof(PKT_STATS);
   }

   return j;  /* return number of entries added */
}

/* group data by unique SSRCs */

int DSFindSSRCGroups(PKT_STATS* pkts, int num_pkts, uint32_t ssrcs[], int first_pkt_idx[], int last_pkt_idx[], uint32_t first_rtp_seqnum[], uint32_t last_rtp_seqnum[], unsigned int uFlags) {

int        i, j, k;
uint32_t   first_seqnum, last_seqnum;
bool       fCollated = false;
int        sorted_point;
PKT_STATS  temp_pkts;
int        seq_wrap[MAX_SSRC_TRANSITIONS] = { 0 };  /* MAX_SSRC_TRANSITIONS defined in shared_include/session.h, currently 128 */
int        ssrc_idx, num_ssrcs;
bool       fDebug = lib_dbg_cfg.uLogLevel > 8;  /* lib_dbg_cfg is in lib_logging.c */

#define SEARCH_WINDOW        30
#define MAX_MISSING_SEQ_GAP  20000  /* max missing seq number gap we can tolerate */

group_ssrcs:

/* SSRC discovery stage */

   num_ssrcs = 0;

   for (j=0; j<num_pkts; j++) {

   /* first check if we've already seen this SSRC */

      bool fExistingSSRC = false;

      if (num_ssrcs > 0) for (i=0; i<num_ssrcs; i++) {

         if (pkts[j].rtp_SSRC == ssrcs[i]) {

         /* this can't actually happen unless there is corruption in the ssrcs[] array */
            if (fDebug && fExistingSSRC && ssrc_idx != i) Log_RT(8, "INFO: DSFindSSRCGroups (diaglib packet logging) says SSRC 0x%x appears more than once, ssrc_idx = %d, i = %d, num_ssrcs = %d \n", pkts[j].rtp_SSRC, ssrc_idx, i, num_ssrcs);

            ssrc_idx = i;
            fExistingSSRC = true;
            if (!fDebug) break;
         }
      }

   /* create a new SSRC data set if needed */

      if (!fExistingSSRC) {

         ssrc_idx = num_ssrcs;

         ssrcs[ssrc_idx] = pkts[j].rtp_SSRC;
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

                  if (pkts[j+k].rtp_SSRC == ssrcs[ssrc_idx]) {

#define NEW_SEQ_CALC
                     if (!nWrap && !seq_wrap[ssrc_idx]) first_seqnum = min(first_seqnum, pkts[j+k].rtp_seqnum);  /* if wrap has occurred at any point for this SSRC, no longer look for first seq number */
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
  
            if (abs(last_seqnum - last_rtp_seqnum[ssrc_idx]) < MAX_MISSING_SEQ_GAP) last_rtp_seqnum[ssrc_idx] = max(last_seqnum, last_rtp_seqnum[ssrc_idx]);

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

            if (pkts[j].rtp_SSRC != ssrcs[k]) {

               if (!i) {

                  i = j;  /* find first non-matching SSRC */
                  sorted_point = i-1;  /* adjust sorted point.  Note -- added this to fix the "orphan SSRC" number of SSRC groups problem, see comments below near in_ssrc_start and out_ssrc_start.  This also makes the sort faster, avoids unnecessary moving of already sorted enrties, JHB Feb 2019 */

//  printf("non-matching SSRC = 0x%x, current ssrc = 0x%x, i = %d\n", pkts[j].rtp_SSRC, unique_ssrcs[k], i);
               }
            }
            else if (i > sorted_point) {  /* found a match, move it up to just after its last match, and move anything in between down */

//  printf("moving pkts[%d] %u up to pkts[%d] %u\n", j, pkts[j].rtp_SSRC, i, pkts[i].rtp_SSRC);

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

   return num_ssrcs;  /* return number of SSRC groups found */
}


int DSPktStatsLogSeqnums(FILE* fp_log, unsigned int uFlags, PKT_STATS* pkts, int num_pkts, char* label, uint32_t ssrcs[], int first_pkt_idx[], int last_pkt_idx[], uint32_t first_rtp_seqnum[], uint32_t last_rtp_seqnum[], STREAM_STATS StreamStats[]) {

int           i, j, k, nSpaces;
bool          fFound_sn, fDup_sn, fOoo_sn;
unsigned int  rtp_seqnum, dup_rtp_seqnum, ooo_rtp_seqnum, numDTX, numSIDNoData;
char          seqstr[100], tmpstr[200];
int           num_ssrcs;
int           seq_wrap[MAX_SSRC_TRANSITIONS] = { 0 };
uint32_t      max_consec_missing[MAX_SSRC_TRANSITIONS] = { 0 };
char          szLastSeq[100];

/* first group data by unique SSRCs */

   num_ssrcs = DSFindSSRCGroups(pkts, num_pkts, ssrcs, first_pkt_idx, last_pkt_idx, first_rtp_seqnum, last_rtp_seqnum, uFlags);

   #if 0  /* debug -- see if sort looks ok */
   fprintf(fp_log, "%s sorted by SSRC (no analysis), numpkts = %d\n", label, num_pkts);
   for (j=0; j<num_pkts; j++) {

      fprintf(fp_log, "seq = %u, ssrc = 0x%x", pkts[j].rtp_seqnum, pkts[j].rtp_SSRC);
      if ((pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID) fprintf(fp_log, " (SID)");
      else if ((pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_REUSE) fprintf(fp_log, " (SID CNG-R)");
      else if ((pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_DTMF) {
         if (pkts[j].info_flags & DS_PKT_PYLD_CONTENT_DTMF_END)  fprintf(fp_log, " (DTMF Event End)");
         else fprintf(fp_log, " (DTMF Event)");
      }
      fprintf(fp_log, "\n");
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
         if (ssrcs[i] == ssrcs[k]) {
            strcpy(tmpstr, " (cont)");  /* annotate if this SSRC has appeared before */
            break;
         }
      }

      if (fp_log) {
         if (label) fprintf(fp_log, "%s ", label);
         sprintf(szLastSeq, "%u", last_rtp_seqnum[i]);
         if (uFlags & DS_PKTSTATS_LOG_SHOW_WRAPPED_SEQNUMS) sprintf(&szLastSeq[strlen(szLastSeq)], " (%u)", last_rtp_seqnum[i] & 0xffff);
         fprintf(fp_log, "Packet info for SSRC = 0x%x%s, first seq num = %u, last seq num = %s ...\n\n", ssrcs[i], tmpstr, first_rtp_seqnum[i], szLastSeq);
      }

      j = first_pkt_idx[i];

      numDTX = 0, numSIDNoData = 0;

      rtp_seqnum = first_rtp_seqnum[i];

      while (rtp_seqnum <= last_rtp_seqnum[i] && j <= last_pkt_idx[i]) {

         if (StreamStats[i].chnum[max(StreamStats[i].num_chnum - 1, 0)] != pkts[j].chnum) {  /* handle"dormant SSRCs" that are taken over by another channel, JHB Jan2020 */

            if (StreamStats[i].num_chnum < MAX_CHAN_PER_SSRC) {
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

            if ((pkts[j].info_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_DTMF && !(uFlags & DS_PKTSTATS_LOG_MARK_DTMF_DUPLICATE)) fFound_sn = true;  /* if it's a DTX event packet we don't label it duplicated (DTMF events can have several duplicated packets) */
         }
         else if (pkts[j].rtp_seqnum + seq_wrap[i]*65536L != rtp_seqnum) {  /* recorded seq number matches next (expected) seq number ? */

            #define OOO_SEARCH_WINDOW 30  /* possibly this should be something users can set ?  JHB Dec2019 */

            for (k=max(j-(OOO_SEARCH_WINDOW-1), first_pkt_idx[i]); k<min(j+OOO_SEARCH_WINDOW, last_pkt_idx[i]+1); k++) {  /* search +/- OOO_SEARCH_WINDOW number of packets to find ooo packets.  Allow for 2x consecutive duplicates, this is a window of +/- 1/2x ptime */

               if (pkts[k].rtp_seqnum + seq_wrap[i]*65536L == rtp_seqnum) {

                  StreamStats[i].ooo_max = max(StreamStats[i].ooo_max, abs(k-j));  /* record max ooo */
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

            sprintf(&tmpstr[strlen(tmpstr)], " timestamp = %u, pkt len = %u", pkts[j].rtp_timestamp, pkts[j].rtp_pyldlen);

            if ((pkts[j].info_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID) {
               StreamStats[i].numSID++;
               sprintf(&tmpstr[strlen(tmpstr)], " SID");
            }
            else if ((pkts[j].info_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_REUSE) {
               StreamStats[i].numSIDReuse++;
               sprintf(&tmpstr[strlen(tmpstr)], " SID CNG-R");
            }
            else if ((pkts[j].info_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_NODATA) {
               numSIDNoData++;
               sprintf(&tmpstr[strlen(tmpstr)], " SID NoData");
            }
            else if ((pkts[j].info_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_DTMF) {
               StreamStats[i].numDTMFEvent++;
               if (pkts[j].info_flags & DS_PKT_PYLD_CONTENT_DTMF_END) sprintf(&tmpstr[strlen(tmpstr)], " DTMF Event End");
               sprintf(&tmpstr[strlen(tmpstr)], " DTMF Event");
            }
            else if (pkts[j].rtp_pyldlen > 0 && pkts[j].rtp_pyldlen <= 7) {
               numDTX++;
               sprintf(&tmpstr[strlen(tmpstr)], " DTX");
            }

            if (pkts[j].info_flags & DS_PKT_PYLD_CONTENT_REPAIR) {

               if ((pkts[j].info_flags & ~DS_PKT_PYLD_CONTENT_REPAIR) == DS_PKT_PYLD_CONTENT_MEDIA) StreamStats[i].numMediaRepair++;
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

         fprintf(fp_log, "\n%s SSRC 0x%x out-of-order seq numbers = %u, duplicate seq numbers = %u, missing seq numbers = %u, max consec missing seq numbers = %u", label, ssrcs[i], StreamStats[i].ooo_seqnum, StreamStats[i].dup_seqnum, StreamStats[i].missing_seqnum, StreamStats[i].max_consec_missing_seqnum);
         if (StreamStats[i].numSID) fprintf(fp_log, ", SID packets = %u", StreamStats[i].numSID);
         if (StreamStats[i].numSIDReuse) fprintf(fp_log, ", SID CNG-R packets = %u", StreamStats[i].numSIDReuse);
         if (StreamStats[i].numSIDRepair) fprintf(fp_log, ", repaired SID packets = %u", StreamStats[i].numSIDRepair);
         if (StreamStats[i].numMediaRepair) fprintf(fp_log, ", repaired media packets = %u", StreamStats[i].numMediaRepair);
         if (numSIDNoData) fprintf(fp_log, ", SID CNG-N packets = %u", numSIDNoData);
         if (!StreamStats[i].numSID && !StreamStats[i].numSIDReuse && !numSIDNoData) fprintf(fp_log, ", DTX packets = %u", numDTX);
         if (StreamStats[i].numDTMFEvent) fprintf(fp_log, ", DTMF Event packets = %u", StreamStats[i].numDTMFEvent);
         fprintf(fp_log, "\n");

         if (i+1 < num_ssrcs) fprintf(fp_log, "\n");
      }
   }

   return num_ssrcs;
}


static int analysis_and_stats(FILE* fp_log, int num_ssrcs, uint32_t in_ssrcs[], PKT_STATS input_pkts[], int in_first_pkt_idx[], int in_last_pkt_idx[], uint32_t in_first_rtp_seqnum[], uint32_t in_last_rtp_seqnum[], STREAM_STATS InputStreamStats[], uint32_t out_ssrcs[], PKT_STATS output_pkts[], int out_first_pkt_idx[], int out_last_pkt_idx[], uint32_t out_first_rtp_seqnum[], uint32_t out_last_rtp_seqnum[], STREAM_STATS OutputStreamStats[], int in_ssrc_start, int out_ssrc_start, int io_map_ssrcs[], unsigned int uFlags) {

int           i = 0, j, k, i_out, pkt_cnt;
unsigned int  rtp_seqnum, rtp_seqnum_chk, mismatch_cnt, sid_reuse_offset = 0;
int           drop_consec_cnt, drop_cnt, dup_cnt, timestamp_mismatches, last_timestamp_mismatches;
int           in_seq_wrap[MAX_SSRC_TRANSITIONS] = { 0 };
int           out_seq_wrap[MAX_SSRC_TRANSITIONS] = { 0 };
uint32_t      total_sid_reuse_offset[MAX_SSRCS] = { 0 };
bool          fNext;

char          ssrc_indent[20] = "";
char          info_indent[20] = "  ";
char          szLastSeq[100], szStreamStr[200], szGroupStr[200] = "";
char          tmpstr[200];

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


   if (num_ssrcs <= 0 || !fp_log) {
   
      Log_RT(3, "WARNING: analysis_and_stats() in DSPktStatsWriteLogFile() says num_ssrcs %d <= 0 or invalid packet log file handle \n", num_ssrcs);
      return -1;
   }

/* if organize-by-stream group flag is set, create a map of ssrcs to stream groups ("GroupMap") */

   if (uFlags & DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP) {

      GroupMap = calloc(MAX_GROUPS, sizeof(GROUPMAP));  /* as of Jan2020 there is still some problem with stack space in diaglib.  Declaring GroupMap on the stack causes a seg fault upon entry to analysis_and_stats() (even if first line is a printf, it won't print, and gdb shows nothing beyond the function header), so we're using calloc, JHB Jan2020 */

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

      int num_in_pkts = in_last_pkt_idx[i+in_ssrc_start] - in_first_pkt_idx[i+in_ssrc_start] + 1;
      int num_out_pkts = out_last_pkt_idx[i_out+out_ssrc_start] - out_first_pkt_idx[i_out+out_ssrc_start] + 1;

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
      fprintf(fp_log, "%sOutput packets = %d, ooo packets = %d, seq numbers = %u..%s, missing seq numbers = %d, max consec missing seq numbers = %d, SID packets = %d, SID R packets = %d, repaired SID packets = %d, repaired media packets = %d\n", info_indent, num_out_pkts, OutputStreamStats[i_out+out_ssrc_start].ooo_seqnum,  out_first_rtp_seqnum[i_out+out_ssrc_start], szLastSeq, OutputStreamStats[i_out+out_ssrc_start].missing_seqnum, OutputStreamStats[i_out+out_ssrc_start].max_consec_missing_seqnum, OutputStreamStats[i_out+out_ssrc_start].numSID, OutputStreamStats[i_out+out_ssrc_start].numSIDReuse, OutputStreamStats[i_out+out_ssrc_start].numSIDRepair, OutputStreamStats[i_out+out_ssrc_start].numMediaRepair);
      fprintf(fp_log, "%sOutput packet loss = %2.3f%%\n", info_indent, 100.0*OutputStreamStats[i_out+out_ssrc_start].missing_seqnum/num_out_pkts);
      fprintf(fp_log, "%sOutput ooo = %2.3f%%, max ooo = %d\n", info_indent, 100.0*OutputStreamStats[i_out+out_ssrc_start].ooo_seqnum/2/num_out_pkts, OutputStreamStats[i_out+out_ssrc_start].ooo_max);

#if 0  /* debug helper, shows whether matching between input/output ssrc groups is correct */
   printf(" ====== loop %d, i_out = %d, input stream SSRC = 0x%x, output stream SSRC = 0x%x\n", i, i_out, in_ssrcs[i+in_ssrc_start], out_ssrcs[i_out+out_ssrc_start]);
#endif

      drop_cnt = 0;
      drop_consec_cnt = 0;
      dup_cnt = 0;
      timestamp_mismatches = 0;
      last_timestamp_mismatches = 0;

      found_history_t found_history[4] = {{ 0 }};
      found_history_t timestamp_mismatch_history[16] __attribute__ ((unused)) = {{ 0 }};
      int found_index = 0, mismatch_index = 0, diff;
      int total_match_found = 0;

      rtp_seqnum = input_pkts[in_first_pkt_idx[i+in_ssrc_start]].rtp_seqnum;

      #if 0
      int timestamp_adjust = 0, last_timestamp_adjust = 0;
      #endif

      for (j=in_first_pkt_idx[i+in_ssrc_start]; j<=in_last_pkt_idx[i+in_ssrc_start]; j++) {

      /* look for seq numbers in input / buffered packets that don't appear in output / pulled packets, JHB Aug2017:

         -take into account any SID reuse packets (which artificially increase output packet seq numbers)
         -both seq numbers and timestamps need to match
      */

         rtp_seqnum_chk = input_pkts[j].rtp_seqnum + in_seq_wrap[i]*65536L;  /* input seq number */

         if (abs(rtp_seqnum_chk - rtp_seqnum) < SEARCH_WINDOW) rtp_seqnum = rtp_seqnum_chk;  /* watch for case where input seq number wrapped early due to ooo, JHB Jan2020 */
         else rtp_seqnum = input_pkts[j].rtp_seqnum + max(in_seq_wrap[i]-1, 0)*65536L;

         pkt_cnt = 0;
         mismatch_cnt = 0;
         sid_reuse_offset = total_sid_reuse_offset[i_out];
         out_seq_wrap[i_out] = 0;  /* inner loop cycles through output packets so we need to reset this */
         sid_reuse_offset = 0;

         for (k=out_first_pkt_idx[i_out+out_ssrc_start]; k<=out_last_pkt_idx[i_out+out_ssrc_start]; k++) {

            if (output_pkts[k].info_flags == DS_PKT_PYLD_CONTENT_SID_REUSE) sid_reuse_offset++;  /* note that because repaired packets fill in for missing seq nums, they do not extend the search offset, JHB Feb2020 */
            else {

               if (rtp_seqnum == output_pkts[k].rtp_seqnum + out_seq_wrap[i_out]*65536L - sid_reuse_offset) {

                  pkt_cnt++;  /* match found */

                  if ((diff = input_pkts[j].rtp_timestamp
                     #if 0
                     + timestamp_adjust
                     #endif
                     - output_pkts[k].rtp_timestamp) != 0) {

                     #if 0  /* have not been able to get this to work.  Evidently once timestamps no longer match, the amount of mismatch varies constantly. That makes it hard to print a couple of lines of output and then
                               "get back on track".  Ends up being 100s of lines of meaningless output, JHB Feb2020 */

                     timestamp_adjust = output_pkts[k].rtp_timestamp - input_pkts[j].rtp_timestamp;  /* update adjustment once difference stabilizes */
                     printf("ssrc 0x%x inp seq number %u matches out seq num %u, but inp timestamp %u + adjust > out timestamp %u by = %d, adjust = %d \n", in_ssrcs[i+in_ssrc_start], rtp_seqnum, output_pkts[k].rtp_seqnum, input_pkts[j].rtp_timestamp, output_pkts[k].rtp_timestamp, diff, timestamp_adjust);
                     #endif

                     timestamp_mismatch_history[mismatch_index].output_index = k;
                     timestamp_mismatch_history[mismatch_index].input_rtp_seqnum = rtp_seqnum;
                     mismatch_index = (mismatch_index+1) & (16-1);

                     mismatch_cnt++;

//                     set prior_timestamp to input_pkts[j].rtp_timestamp
//                     next iteration compare output_pkts[timestamp_mismatch_history[index].output_index].rtp_timestamp with prior_timestamp
                  }

                  found_history[found_index].output_index = k;
                  found_history[found_index].input_rtp_seqnum = rtp_seqnum;
                  found_index = (found_index+1) & (4-1);
                  total_match_found++;
               }
            }

            if (output_pkts[k].rtp_seqnum == 65535L) out_seq_wrap[i_out]++;
         }

#if 0  /* debug */
         static bool fOnce = false;

         if (!pkt_cnt && !fOnce) {

            sid_reuse_offset = total_sid_reuse_offset[i_out];

            for (k=out_first_pkt_idx[i_out+out_ssrc_start]; k<=out_last_pkt_idx[i_out+out_ssrc_start]; k++) {

               if ((output_pkts[k].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_REUSE) sid_reuse_offset++;
               else fprintf(fp_log, "no match, rtp_seqnum = %d, output_pkts[%d].rtp_seqnum = %d, sid_reuse_offset = %d\n", rtp_seqnum, k, output_pkts[k].rtp_seqnum + out_seq_wrap[i_out]*65536L, sid_reuse_offset);
            }

            fOnce = true;
         }
#endif

         if (!pkt_cnt) {

            int sp, splen;
            #define COLUMN2 32  /* assumes max 10 digit number for %u uint32_t */

            if (!drop_consec_cnt) {

               if (total_match_found >= 2) {

                  int history_index = (found_index-2) & 3;
                  int out_index = found_history[history_index].output_index;

                  sprintf(tmpstr, "%sInput seq num %u corresponds to output seq num %u", info_indent, found_history[history_index].input_rtp_seqnum, (unsigned int)(output_pkts[out_index].rtp_seqnum + out_seq_wrap[i_out]*65536L));  /* in_seq_wrap[] is cumulative so it's not correct here, JHB Jan2020 */
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

            if ((input_pkts[j].info_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID) fprintf(fp_log, " (SID)");
            else if ((input_pkts[j].info_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_REUSE) fprintf(fp_log, " (SID CNG-R)");
            else if ((input_pkts[j].info_flags & DS_PKT_INFO_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_NODATA) fprintf(fp_log, " (SID NoData)");
            else if ((input_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_DTMF) {
               if (input_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_DTMF_END)  fprintf(fp_log, " (DTMF Event End)");
               else fprintf(fp_log, " (DTMF Event)");
            }
            else if (input_pkts[j].rtp_pyldlen > 0 && input_pkts[j].rtp_pyldlen <= 7) fprintf(fp_log, " (DTX)");
            fprintf(fp_log, "\n");

            drop_consec_cnt++;
         }
         else if (pkt_cnt > 1) {

            if ((input_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) != DS_PKT_PYLD_CONTENT_DTMF || (uFlags & DS_PKTSTATS_LOG_MARK_DTMF_DUPLICATE)) {

               dup_cnt++;

               strcpy(tmpstr, "");
               for (k=0; k<pkt_cnt; k++) sprintf(&tmpstr[strlen(tmpstr)], " %u", (unsigned int)(output_pkts[found_history[(found_index-k) & 3].output_index].rtp_seqnum + out_seq_wrap[i_out]*65536L));
               fprintf(fp_log, "%sDuplicate %d: input seq num %u corresponds to output seq nums%s, input rtp len = %u", info_indent, dup_cnt, rtp_seqnum, tmpstr, input_pkts[j].rtp_pyldlen);

               if ((input_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID) fprintf(fp_log, " (SID)");
               else if ((input_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_REUSE) fprintf(fp_log, " (SID CNG-R)");
               else if ((input_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_NODATA) fprintf(fp_log, " (SID NoData)");
               else if ((input_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_DTMF) {
                  if (input_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_DTMF_END)  fprintf(fp_log, " (DTMF Event End)");
                  else fprintf(fp_log, " (DTMF Event)");
               }
               else if (input_pkts[j].rtp_pyldlen <= 7) fprintf(fp_log, " (DTX)");
               fprintf(fp_log, "\n");
            }

            drop_consec_cnt = 0;
         }
         else drop_consec_cnt = 0;

         if (mismatch_cnt) {

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

      total_sid_reuse_offset[i_out] = sid_reuse_offset;

      for (k=i_out+1; k<num_ssrcs; k++) {  /* update total sid reuse offset for any subsequent output SSRC stream that has same SSRC number as the SSRC stream just processed; i.e. if they are a resumption of the current stream, JHB Sep2017 */

         if (out_ssrcs[k+out_ssrc_start] == out_ssrcs[i_out+out_ssrc_start]) {

            total_sid_reuse_offset[k] = total_sid_reuse_offset[i_out];
//            printf("i_out = %u, updating total_sid_reuse_offset[%d] = %u\n", i_out, k, total_sid_reuse_offset[k]);
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

      sprintf(tmpstr, "%sTimestamp mismatches = %u\n", info_indent, timestamp_mismatches);
      if (uFlags & DS_PKTSTATS_LOG_EVENT_LOG_SUMMARY) Log_RT(4, "  %s", tmpstr);
      fprintf(fp_log, "%s", tmpstr);

next_i:

      if (nNumGroups) {  /* if organize-by-stream group flag is set and we found groups, we stay in the loop until all possible stream group indexes have been looked at, JHB Dec2019 */

         fNext = (nGroupIndex < MAX_GROUPS);
      }
      else fNext = ++i < num_ssrcs;  /* if no flags set, then increment ssrc index and compare with num_ssrcs, JHB Dec2019 */

   } while (fNext);

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

#if 0  /* move these off the stack as they seemed to be causing sporadic seg-faults, can;t even get to first line in the funciton.  MAX_SSRCS is 65536 (defined in diaglib.h) so this is around 2 MB of total space, maybe not a good idea. JHB Sep 2018 */
int           in_first_pkt_idx[MAX_SSRCS] = { 0 };
int           in_last_pkt_idx[MAX_SSRCS] = { 0 };
uint32_t      in_first_rtp_seqnum[MAX_SSRCS] = { 0 };
uint32_t      in_last_rtp_seqnum[MAX_SSRCS] = { 0 };
uint32_t      in_ssrcs[MAX_SSRCS] = { 0 };

int           out_first_pkt_idx[MAX_SSRCS] = { 0 };
int           out_last_pkt_idx[MAX_SSRCS] = { 0 };
uint32_t      out_first_rtp_seqnum[MAX_SSRCS] = { 0 };
uint32_t      out_last_rtp_seqnum[MAX_SSRCS] = { 0 };
uint32_t      out_ssrcs[MAX_SSRCS] = { 0 };

int           io_map_ssrcs[MAX_SSRCS] = { 0 };
int           used_map_ssrcs[MAX_SSRCS] = { 0 };

#else

int*          in_first_pkt_idx = NULL;  /* packet index math in diaglib.c depends on indexes being int.  Don't change these to uint32_t, JHB Oct 2019 */ 
int*          in_last_pkt_idx = NULL;
uint32_t*     in_first_rtp_seqnum = NULL;
uint32_t*     in_last_rtp_seqnum = NULL;
uint32_t*     in_ssrcs = NULL;

int*          out_first_pkt_idx = NULL;
int*          out_last_pkt_idx = NULL;
uint32_t*     out_first_rtp_seqnum = NULL;
uint32_t*     out_last_rtp_seqnum = NULL;
uint32_t*     out_ssrcs = NULL;

int*          io_map_ssrcs = NULL;
int*          used_map_ssrcs = NULL;

STREAM_STATS*  InputStreamStats = NULL;
STREAM_STATS*  OutputStreamStats = NULL;

#endif

unsigned long long t1, t2;


   if (uFlags & DS_PKTSTATS_LOG_APPEND) fp_log = fopen(szLogfile, "ab");
   else fp_log = fopen(szLogfile, "wb");

   if (!fp_log) {
      ret_code = 0;
      goto cleanup;
   }

   in_first_pkt_idx = calloc(MAX_SSRCS, sizeof(int));
   in_last_pkt_idx = calloc(MAX_SSRCS, sizeof(int));
   in_first_rtp_seqnum = calloc(MAX_SSRCS, sizeof(uint32_t));
   in_last_rtp_seqnum = calloc(MAX_SSRCS, sizeof(uint32_t));
   in_ssrcs = calloc(MAX_SSRCS, sizeof(uint32_t));

   out_first_pkt_idx = calloc(MAX_SSRCS, sizeof(int));
   out_last_pkt_idx = calloc(MAX_SSRCS, sizeof(int));
   out_first_rtp_seqnum = calloc(MAX_SSRCS, sizeof(uint32_t));
   out_last_rtp_seqnum = calloc(MAX_SSRCS, sizeof(uint32_t));
   out_ssrcs = calloc(MAX_SSRCS, sizeof(uint32_t));

   io_map_ssrcs = calloc(MAX_SSRCS, sizeof(int));
   used_map_ssrcs = calloc(MAX_SSRCS, sizeof(int));

   InputStreamStats = calloc(MAX_SSRCS, sizeof(STREAM_STATS));
   OutputStreamStats = calloc(MAX_SSRCS, sizeof(STREAM_STATS));

   #ifndef DIAGLIB_STANDALONE
   t1 = get_time(USE_CLOCK_GETTIME);
   #else
   struct timeval tv;
   gettimeofday(&tv, NULL);
   t1 = tv.tv_sec * 1000000L + tv.tv_usec;
   #endif

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

            fprintf(fp_log, "seq = %u, ssrc = 0x%x", input_pkts[j].rtp_seqnum, input_pkts[j].rtp_SSRC);
            if ((input_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID) fprintf(fp_log, " (SID)");
            else if ((input_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_REUSE) fprintf(fp_log, " (SID CNG-R)");
            else if ((input_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_DTMF) {
               if (input_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_DTMF_END)  fprintf(fp_log, " (DTMF Event End)");
               else fprintf(fp_log, " (DTMF Event)");
            }
            fprintf(fp_log, " chnum = %d", input_pkts[j].chnum);
            fprintf(fp_log, "\n");
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

   /* log ingress packet info -- grouped by SSRC values, including ooo and missing seq numbers */

      in_ssrc_groups = DSPktStatsLogSeqnums(fp_log, uFlags, input_pkts, input_idx, "Ingress", in_ssrcs, in_first_pkt_idx, in_last_pkt_idx, in_first_rtp_seqnum, in_last_rtp_seqnum, InputStreamStats);
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

            fprintf(fp_log, "seq = %u, ssrc = 0x%x", output_pkts[j].rtp_seqnum, output_pkts[j].rtp_SSRC);
            if ((output_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID) fprintf(fp_log, " (SID)");
            else if ((output_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_SID_REUSE) fprintf(fp_log, " (SID CNG-R)");
            else if ((output_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_ITEM_MASK) == DS_PKT_PYLD_CONTENT_DTMF) {
               if (output_pkts[j].info_flags & DS_PKT_PYLD_CONTENT_DTMF_END)  fprintf(fp_log, " (DTMF Event End)");
               else fprintf(fp_log, " (DTMF Event)");
            }
            fprintf(fp_log, "\n");
         }
         fprintf(fp_log, "\n");
      }

   /* log jitter buffer output packet info -- grouped by SSRC values, including ooo and missing seq numbers */

      out_ssrc_groups = DSPktStatsLogSeqnums(fp_log, uFlags, output_pkts, output_idx, "Jitter Buffer", out_ssrcs, out_first_pkt_idx, out_last_pkt_idx, out_first_rtp_seqnum, out_last_rtp_seqnum, OutputStreamStats);

      #ifndef DIAGLIB_STANDALONE
      t2 = get_time(USE_CLOCK_GETTIME);
      #else
      struct timeval tv;
      gettimeofday(&tv, NULL);
      t2 = tv.tv_sec * 1000000L + tv.tv_usec;
      #endif

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

   /* before comparing / analyzing input vs. output SSRC groups, we match up the groups, in case their order is different.  JHB Jul2018 */

//      for (i=0; i<num_ssrcs; i++) printf("in_ssrcs[%u] = 0x%x\n", i, in_ssrcs[i]);
//      for (i=0; i<num_ssrcs; i++) printf("out_ssrcs[%u] = 0x%x\n", i, out_ssrcs[i]);

      memset(io_map_ssrcs, -1, MAX_SSRCS*sizeof(int));  /* initialize all mapping values to -1 */
      memset(used_map_ssrcs, -1, MAX_SSRCS*sizeof(int));

      for (i=0; i<num_ssrcs; i++) {

         for (j=0; j<num_ssrcs; j++) {

            if ((io_map_ssrcs[i] == -1) && (used_map_ssrcs[j] == -1) && (in_ssrcs[i+in_ssrc_start] == out_ssrcs[j+out_ssrc_start])) {

               io_map_ssrcs[i] = j;
               used_map_ssrcs[j] = i;  /* we set both sides of the mapping, as it has to be 1:1 relationship, no entry on one side or the other mapped twice */
//             printf("in_ssrcs[%d] = 0x%x, out_ssrcs[%d] = 0x%x, io_map_ssrcs[%d] = %u\n", i, in_ssrcs[i], j, out_ssrcs[j], i, io_map_ssrcs[i]);
               break;
            }
         }
      }

      for (i=0; i<num_ssrcs; i++) if (io_map_ssrcs[i] == -1) fprintf(fp_log, "\nCorresponding output SSRC group not found for input SSRC 0x%x, group %u\n", in_ssrcs[i+in_ssrc_start], i);


   /* call analysis_and_stats() as needed, depending on uFlags */

      unsigned int uFlags_as;
      int ret_val = 1;

      if (uFlags & DS_PKTSTATS_ORGANIZE_BY_SSRC) {

         uFlags_as = uFlags & ~DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP & ~DS_PKTSTATS_ORGANIZE_BY_CHNUM;
         ret_val = analysis_and_stats(fp_log, num_ssrcs, in_ssrcs, input_pkts, in_first_pkt_idx, in_last_pkt_idx, in_first_rtp_seqnum, in_last_rtp_seqnum, InputStreamStats, out_ssrcs, output_pkts, out_first_pkt_idx, out_last_pkt_idx, out_first_rtp_seqnum, out_last_rtp_seqnum, OutputStreamStats, in_ssrc_start, out_ssrc_start, io_map_ssrcs, uFlags_as);
      }

      if (ret_val > 0 && (uFlags & DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP)) {

         uFlags_as = uFlags & ~DS_PKTSTATS_ORGANIZE_BY_SSRC & ~DS_PKTSTATS_ORGANIZE_BY_CHNUM;
         analysis_and_stats(fp_log, num_ssrcs, in_ssrcs, input_pkts, in_first_pkt_idx, in_last_pkt_idx, in_first_rtp_seqnum, in_last_rtp_seqnum, InputStreamStats, out_ssrcs, output_pkts, out_first_pkt_idx, out_last_pkt_idx, out_first_rtp_seqnum, out_last_rtp_seqnum, OutputStreamStats, in_ssrc_start, out_ssrc_start, io_map_ssrcs, uFlags_as);
      }

      if (ret_val > 0 && (uFlags & DS_PKTSTATS_ORGANIZE_BY_CHNUM)) {

         uFlags_as = uFlags & ~DS_PKTSTATS_ORGANIZE_BY_SSRC & ~DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP;
         analysis_and_stats(fp_log, num_ssrcs, in_ssrcs, input_pkts, in_first_pkt_idx, in_last_pkt_idx, in_first_rtp_seqnum, in_last_rtp_seqnum, InputStreamStats, out_ssrcs, output_pkts, out_first_pkt_idx, out_last_pkt_idx, out_first_rtp_seqnum, out_last_rtp_seqnum, OutputStreamStats, in_ssrc_start, out_ssrc_start, io_map_ssrcs, uFlags_as);
      }
   }

   if (!fp_log) fp_log = fopen(szLogfile, "wb");
   else fprintf(fp_log, "\n");

   fprintf(fp_log, "** Packet Egress Stats **\n\n");

   fprintf(fp_log, "Total packets written to pcap = %d\n", pkt_counters->pkt_write_cnt);
   fprintf(fp_log, "Total packets output to network socket = %d\n", pkt_counters->pkt_output_cnt);
   fprintf(fp_log, "Total packets decoded and written to wav file = %d\n", pkt_counters->frame_write_cnt);

   #ifndef DIAGLIB_STANDALONE
   t2 = get_time(USE_CLOCK_GETTIME);
   #else
   struct timeval tv;
   gettimeofday(&tv, NULL);
   t2 = tv.tv_sec * 1000000L + tv.tv_usec;
   #endif

   char tstr[] = "msec";
   float ltime = (t2-t1)/1000.0;
   if (ltime > 100) { ltime = (t2-t1)/1000000.0; strcpy(tstr, "sec"); }

   Log_RT(4, "INFO: DSPktStatsWriteLogFile() says packet log analysis completed in %2.1f %s, packet log file = %s\n", ltime, tstr, szLogfile);

   ret_code = 1;

cleanup:
   
   if (fp_log) fclose(fp_log);

   if (in_first_pkt_idx) free(in_first_pkt_idx);
   if (in_last_pkt_idx) free(in_last_pkt_idx);
   if (in_first_rtp_seqnum) free(in_first_rtp_seqnum);
   if (in_last_rtp_seqnum) free(in_last_rtp_seqnum);
   if (in_ssrcs) free(in_ssrcs);

   if (out_first_pkt_idx) free(out_first_pkt_idx);
   if (out_last_pkt_idx) free(out_last_pkt_idx);
   if (out_first_rtp_seqnum) free(out_first_rtp_seqnum);
   if (out_last_rtp_seqnum) free(out_last_rtp_seqnum);
   if (out_ssrcs) free(out_ssrcs);

   if (io_map_ssrcs) free(io_map_ssrcs);
   if (used_map_ssrcs) free(used_map_ssrcs);

   if (InputStreamStats) free(InputStreamStats);
   if (OutputStreamStats) free(OutputStreamStats);

   return ret_code;
}
