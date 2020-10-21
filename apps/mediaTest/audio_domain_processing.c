/*
 $Header: /root/Signalogic/apps/mediaTest/audio_domain_processing.c

 Copyright (C) Signalogic Inc. 2018-2020

 Description

   Source code for audio domain processing, with options for sampling rate conversion, ASR, packet output. Currently contains:
     DSProcessAudio()
     DSDeduplicateStreams()

 Purpose

   Make available insertion points for signal processing used inside SigSRF, including audio processing for merging, stream deduplication, and ASR
   By adding these functions to application build source, if names are identical they will take precedence over functions inside streamlib.so

 Revision History

   Created Nov 2019 JHB, separated out from streamlib.c
   Modified Feb 2020 JHB, move last_merge_output_time[] update from tail end of data flow in streamlib.c to here, after merge_gap_advance[] is handled. This is needed because DSProcessAudio() can be called from different places in streamlib
   Modified Jun 2020 JHB, add DSDeduplicateStreams(). See comments
*/


/* Linux header files */

#include <semaphore.h>

/* lib header files (all libs are .so format) */

#include "session.h"
#include "voplib.h"
#include "pktlib.h"
#include "diaglib.h"
#include "streamlib.h"

extern DEBUG_CONFIG lib_dbg_cfg;  /* in diaglib, set by calls to DSConfigPktlib() and DSConfigStreamlib() */

/* extern items in streamlib */

extern PACKETMEDIATHREADINFO packet_media_thread_info[];  /* array of thread handles in pktlib.so, zero indicates no thread.  MAX_PKTMEDIA_THREADS is defined in pktlib.h */
extern unsigned long long last_merge_output_time[];       /* set by DSMergeGroupContributors() (in streamlib) */
extern uint8_t merge_gap_advance[];                       /*    ""   */
extern int groupTimestampOffset[];                        /*    ""   */
extern sem_t pcap_write_sem;                              /* semaphore used to ensure no more than 1 thread writes to a particular output file at a time */

/* streamlib items needed by DSDeduplicateStreams() */

extern uint32_t align_interval_count[MAX_STREAM_GROUPS][MAX_GROUP_CONTRIBUTORS];


/* DSProcessGroupAudio, depending on uFlags options defined in streamlib.h:

  1) apply sampling rate conversion, ASR, or other signal processing to output audio frames. This may involve non-audio output, for example .txt or .csv file output for ASR or diarization
  2) packetize according to either (i) stream group owner session's termination (group_term) info or (ii) session's term2 (i or ii depends on idx input param).  Default encoding for stream groups is G711 but can be specified during session creation
  3) input can be either (i) stream group 16-bit linear audio output or (ii) arbitrary session term2 audio output
  4) send output packets to applications

  Input params

    -group_audio_buffer contains num_frames frames of audio data (audio data in 16-bit linear format)

    -frame_size is size of each frame (in bytes)

    -if uFlags includes DS_PROCESS_AUDIO_STREAM_GROUP_INPUT:
      -hSession is the stream group owner session
      -idx is the stream group index
      -either hSession or idx < 0 is an error condition

    -if uFlags do not include DS_PROCESS_AUDIO_STREAM_GROUP_INPUT:
      -hSession >= 0 specifies the stream associated with the session's term2 endpoint
      -hSession < 0 specifies arbitrary audio input data, with no associated session or stream 

    -if uFlags include DS_PROCESS_AUDIO_OUTPUT_PACKET:
      -if a valid hSession is given, the codec associated with hSession's term2 endpoint is used to encode audio before packetization
      -if nMarkerBit is 1, then the first output packet will have its RTP header "marker bit" set

  Output params

    -on error, num_frames is set to the number of frames processed at the time of error
    -in the case of arbitrary audio input data (no associated stream group or session, e.g. hSession not valid), group_audio_buffer contains processed audio data

  Return value

    -return value is > 0 on success, 0 if nothing was done (no frames or benign error condition), or < 0 for one of several error codes

  Calling references

    -called by DSMergeGroupContributors() in streamlib, which is called by packet/media thread function packet_flow_media_proc() in packet_flow_media_proc.c

  Build notes

    -this function is included in streamlib.so
    -the mediaMin (or other) application Makefile can be modified to include this function, allowing modified source to be incorporated into SigSRF operation as needed. At link-time the application version of the function will take precedence over the streamlib .so version
*/

int DSProcessAudio(HSESSION hSession, uint8_t* group_audio_buffer, int* num_frames, int frame_size, unsigned int uFlags, int idx, int nMarkerBit, unsigned long long merge_cur_time, int upf, int dnf, int* pkt_group_cnt, int thread_index, FILE* fp_out_pcap_merge) {

TERMINATION_INFO output_term;
uint8_t group_audio_encoded_frame[MAX_RAW_FRAME] = { 0 };  /* MAX_RAW_FRAME defined in voplib.h */
uint8_t group_audio_packet[MAX_RAW_FRAME + MAX_IP_UDP_RTP_HEADER_LEN] = { 0 };  /* MAX_IP_UDP_RTP_HEADER_LEN defined in pktlib.h */
HCODEC hCodec = (intptr_t)NULL;
int ptime, j, chnum, ret_val = 2;

FormatPkt mergeFormatPkt = { 0 };
int merge_uFlags_format, SSRC, timestamp, pyld_len = 0, packet_length = 0;
unsigned short int seq_num;

  (void)upf;  /* avoid compiler warnings until these vars are used */
  (void)dnf;

   if (!*num_frames) return 0;  /* return zero if no frames */

   if ((uFlags & DS_PROCESS_AUDIO_STREAM_GROUP_INPUT) && (idx < 0 || hSession < 0)) {
      Log_RT(2, "ERROR: DSProcessAudio() says uFlags 0x%x specifies stream group audio input, but idx % or hSession % is < 0 \n", uFlags, idx, hSession);
      return -1;
   }

   if (hSession >= 0) {  /* use hSession if valid */
 
   /* call DSGetTermChan() for term1 with channel validation flag.  The concern is to not push anything into application queues if the session's channels are pending deletion or no longer exist */

      if ((ret_val = DSGetTermChan(hSession, &chnum, 1, DS_CHECK_CHAN_DELETE_PENDING | DS_CHECK_CHAN_EXIST)) <= 0) return ret_val;  /* ret_val is < 0 for an error condition, and == 0 for a "not an error but don't send any data to the app" condition */

      if (uFlags & DS_PROCESS_AUDIO_STREAM_GROUP_INPUT) {  /* get input audio from stream group owner session's group_term */

#if 0
         hCodec = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_CODEC, 0, &output_term);
#else
         hCodec = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CODEC, 0, &output_term);  /* 0 indicates group term codec handle */
#endif
         if (hCodec < 0) return -6;
         ptime = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_PTIME, 0, NULL);
      }
      else {  /* get input audio from session's term2 stream */

         hCodec = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CODEC, 2, &output_term);
         if (hCodec < 0) return -6;
         ptime = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_PTIME, 2, NULL);
      }

      if (uFlags & DS_PROCESS_AUDIO_PACKET_OUTPUT) {

      /* one-time output packet format setup */

         merge_uFlags_format = DS_FMT_PKT_USER_SEQNUM | DS_FMT_PKT_USER_TIMESTAMP | DS_FMT_PKT_STANDALONE | DS_FMT_PKT_USER_HDRALL | DS_FMT_PKT_USER_PYLDTYPE | DS_FMT_PKT_USER_SSRC | DS_FMT_PKT_USER_MARKERBIT;
         memcpy(mergeFormatPkt.SrcAddr, &output_term.local_ip.u, DS_IPV6_ADDR_LEN);  /* DS_IPV6_ADDR_LEN defined in shared_include/session.h */
         memcpy(mergeFormatPkt.DstAddr, &output_term.remote_ip.u, DS_IPV6_ADDR_LEN);
         mergeFormatPkt.IP_Version = output_term.local_ip.type;
         mergeFormatPkt.udpHeader.SrcPort = output_term.local_port;
         mergeFormatPkt.udpHeader.DestPort = output_term.remote_port;
         mergeFormatPkt.rtpHeader.BitFields = output_term.attr.voice_attr.rtp_payload_type;  /* set payload type */

      /* check if there is a call on-hold or call waiting timestamp gap that needs to be accumulated, notes JHB Nov2019:

         -gap timestamp advance can be disabled (see STREAM_GROUP_RTP_TIMESTAMP_ONHOLD_ADVANCE_DISABLE flag in streamlib.h)
         -no impact on packet stats history logging
         -have seen a case (21161.0-ws) where gaps were accumulating after session was flushed (this is fixed in Mar2020 release, when session flush was moved up, just after end of input flow.  See comments in mediaMin source)
      */
  
         if ((uFlags & DS_PROCESS_AUDIO_STREAM_GROUP_INPUT) && merge_gap_advance[idx]) {

            unsigned int group_flags = (unsigned int)DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_MODE, 0, NULL);  /* use term id 0 to get the group_term mode value */
            bool fRTPTimeStampAdvance_enabled = !(group_flags & STREAM_GROUP_RTP_TIMESTAMP_ONHOLD_ADVANCE_DISABLE);

            if (fRTPTimeStampAdvance_enabled) {  /* advance RTP timestamp unless disabled by group_mode flag in owner session's group_term (group_term.group_mode) */

               int timestamp_advance = frame_size*((merge_cur_time - last_merge_output_time[idx] + 500)/(ptime*1000))/2;
               groupTimestampOffset[idx] += timestamp_advance;

               char szGroupName[MAX_GROUPID_LEN] = "";
               DSGetStreamGroupInfo(idx, DS_GETGROUPINFO_HANDLE_IDX, NULL, NULL, szGroupName);

               Log_RT(4, "INFO: after gap of %2.2f sec, RTP timestamp advanced by %d for stream group %s (idx %d) \n", 1.0*(merge_cur_time - last_merge_output_time[idx] + 500)/1000000L, timestamp_advance, szGroupName, idx);
            }

            merge_gap_advance[idx] = 0;  /* clear timestamp advance */
         }

         last_merge_output_time[idx] = merge_cur_time;
      }
   }

/* loop through audio frames */

   for (j=0; j<*num_frames; j++) {

   /* apply group audio output signal processing here, prior to codec encoding and packet output:
   
      -Kaldi ASR
      -user-defined
   */

      if (uFlags & DS_PROCESS_AUDIO_APPLY_ASR) {

      }

      if ((uFlags & DS_PROCESS_AUDIO_ENCODE) && hCodec) {

      /* encode audio */

         pyld_len = DSCodecEncode(hCodec, 0, (uint8_t*)((intptr_t)group_audio_buffer + j*frame_size), group_audio_encoded_frame, frame_size, NULL);

         if (pyld_len < 0) {  /* if merge codec doesn't exist or has already been deleted */
            Log_RT(3, "WARNING: DSProcessGroupAudio() says DSCodecEncode() returns %d error code, hSession = %d, idx = %d \n", pyld_len, hSession, idx);
            ret_val = -1;
            *num_frames = j;
            break;  /* break out of audio frame loop */
         }
      }

      if (uFlags & DS_PROCESS_AUDIO_PACKET_OUTPUT) {

      /* format packet */

         if (uFlags & DS_PROCESS_AUDIO_STREAM_GROUP_INPUT) DSGetStreamGroupPacketInfo(idx, &seq_num, &timestamp, frame_size/2, &SSRC);  /* DSGetStreamGroupPacketInfo() increments timestamp by frame_size/2 and seq_num by 1.  Returns < 0 on error */
         else {};  /* to-do: needs a non stream group alternative */

         mergeFormatPkt.rtpHeader.Sequence = seq_num;
         mergeFormatPkt.rtpHeader.SSRC = SSRC;
         mergeFormatPkt.rtpHeader.Timestamp = timestamp;
         if (uFlags & DS_PROCESS_AUDIO_STREAM_GROUP_INPUT) mergeFormatPkt.rtpHeader.Timestamp += groupTimestampOffset[idx];

         if (nMarkerBit >= 0) {
            if (nMarkerBit) DSSetMarkerBit(&mergeFormatPkt, merge_uFlags_format);
            else DSClearMarkerBit(&mergeFormatPkt, merge_uFlags_format);
            nMarkerBit--;
         }

         packet_length = DSFormatPacket(chnum, merge_uFlags_format, group_audio_encoded_frame, pyld_len, &mergeFormatPkt, group_audio_packet);

         if (packet_length <= 0) {
            Log_RT(3, "WARNING: DSProcessGroupAudio() says DSFormatPacket() returns %d error code, hSession = %d, idx = %d \n", (int)packet_length, hSession, idx);
            ret_val = -1;
            *num_frames = j;
            break;  /* break out of audio frame loop */
         }

      /* send packet */

         if (!packet_media_thread_info[thread_index].fMediaThread) {  /* non-library mode (mediaTest executable) */

            if (fp_out_pcap_merge) {

               sem_wait(&pcap_write_sem);

               if (DSWritePcapRecord(fp_out_pcap_merge, group_audio_packet, NULL, NULL, &output_term, NULL, packet_length) < 0) {
                  sem_post(&pcap_write_sem);
                  Log_RT(2, "ERROR: DSProcessGroupAudio() says DSWritePcapRecord() failed, hSession = %d, idx = %d, chnum = %d, j = %d, num_frames = %d, packet_length = %d \n", hSession, idx, chnum, j, *num_frames, packet_length);
                  ret_val = -1;
                  *num_frames = j;
                  break;  /* break out of audio frame loop */
               }

               sem_post(&pcap_write_sem);
            }
         }
         else {

            //#define USE_SEND_RETRY
            #ifdef USE_SEND_RETRY
            int retry_count = 0;
send_group_packet:
            #endif

//  static bool fOnce = false;
//  if (!fOnce) { printf("sending merge packets, packet_length = %d, hSession = %d\n", packet_length, hSession); fOnce = true; }

            #if 0  /* debug */
            int pkt_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTLEN | DS_PKT_INFO_SUPPRESS_ERROR_MSG, group_audio_packet, 0, NULL);
            if (pkt_len <= 0) Log_RT(4, " ================ pkt_len <= 0 inside SendMerge, j = %d, hSession = %d, idx = %d, chnum = %d, num_frames = %d, packet_length = %d, ret_val = %d \n", j, hSession, idx, chnum, num_frames, packet_length, ret_val);
            #endif

            int ret_val_send = DSSendPackets(&hSession, DS_SEND_PKT_QUEUE | DS_PULLPACKETS_STREAM_GROUP, group_audio_packet, (unsigned int*)&packet_length, 1);  /* send merged packet */

            if (ret_val_send < 0) {
               Log_RT(2, "ERROR: DSProcessGroupAudio() says DSSendPackets() failed, hSession = %d, idx = %d, chnum = %d, j = %d, num_frames = %d, packet_length = %d \n", hSession, idx, chnum, j, *num_frames, packet_length);
               *num_frames = j;
               ret_val = ret_val_send;
               break;
            }

            #ifdef USE_SEND_RETRY
            if (ret_val_send == 0) {

               if (retry_count < 20) {
                  retry_count++;
                  usleep(500);
                  goto send_group_packet;
               }
            }
            #endif
         }

         if (pkt_group_cnt) (*pkt_group_cnt)++;
      }
      else if ((uFlags & DS_PROCESS_AUDIO_ENCODE) && hCodec) {  /* stream audio input with no associated stream group.  Copy encoded audio over input (i.e. in-place processing) */

         memcpy((uint8_t*)((intptr_t)group_audio_buffer + j*frame_size), group_audio_encoded_frame, pyld_len);
      }
   }

   return ret_val;
}


/* DSDeduplicateStreams, JHB Jun2020:

   -applies deduplication algorithm, which looks for similar content between stream group contributors and attempts to align similar streams. The objective is to reduce perceived reverb/echo due to duplicated streams. A typical scenario is a multipath (duplicated) endpoint with different latencies
   -search contributor audio data for criteria to find similar streams, based on alignment algorithm that uses "local minimums" found by simpple amplitude threshold checks combined with cross-correlation
   -upon finding other streams that match the "reference" (most delayed) stream, (i) allow stream group processing to start, and (ii) delay earlier streams to align with the reference stream

  Input params

    -idx - stream group index
    -nContributors - number of contributors
    -contrib_ch[] - array of channel numbers, one for each contributor stream
    -uFlags - currently not used

  Output params

    -none

  Return value

    -zero indicates stream alignment not found
    -one indicates aligment found
    -negative values indicate an error code

  Calling references

    -called by DSMergeGroupContributors() in streamlib, which is called by packet/media thread function packet_flow_media_proc() in packet_flow_media_proc.c

  Build notes

    -this function is included in streamlib.so
    -the mediaMin (or other) application Makefile can be modified to include this function, allowing modified source to be incorporated into SigSRF operation needed. At link-time the application version of the function will take precedence over the streamlib .so version if link order is specified correctly
*/

int DSDeduplicateStreams(int idx, int nContributors, int contrib_ch[], unsigned int uFlags) {

int j, k, n, num_met, search_start = 0, ret_val = 0;  /* default return value indicates deduplication alignment not yet found */

   (void)uFlags;  /* param currently not used */

   if (idx < 0) {
      Log_RT(3, "WARNING: DSDeduplicateStreams() says stream group index < 0");
      return -1;  /* return error condition */
   }

   if (!contrib_ch) {
      Log_RT(3, "WARNING: DSDeduplicateStreams() says stream array is pointer NULL");
      return -1;
   }

   if (nContributors < 2) return 0;  /* need minimum of two streams to deduplicate. Not an error ... probably waiting for 2nd stream to appear */

/* alignment algorithm notes notes, JHB Jul2020:

   -the basic concept is to find "local minimums" in each stream using a low amplitude threshold search, place those at the center of a window, and cross correlate with local minimum windows in other streams
   -alignment is found when the normalized cross correlation sum exceeds a threshold. When a local minimum is rejected, we move the starting search point forward by 1/2 the cross correlation window size (i.e. 50% overlap)
   -normalization includes both (i) window length (as it may vary depending on amount of available of data) and (ii) number of streams
   -note - for some initial customer PoCs, cross correlation wasn't used and MIN_AMP_THRESH was set at 6600. Undefine USEXCORR to disable cross correlation
*/

   #define MIN_AMP_THRESH         1000    /* minimum amplitude threshold */
   #define XCORR_NORM_SUM_THRESH  200000  /* normalized cross correlation sum threshold */
   #define XCORR_WINDOW_SIZE      200     /* cross correlation window size, in samples */

   #define USEXCORR  /* undefine for debug/development purposes if neeeded. See comments above */

   //#define ALIGNDEBUG
   #ifdef ALIGNDEBUG
   int thresh_repeat = 0;
   #endif

thresh_search:

   num_met = 0;

/* local minimum search */

   for (j=0; j<nContributors; j++) {

      if (!align_interval_count[idx][j]) {  /* if criteria not yet met for this stream, search for it */

         short int* x = DSGetStreamGroupContributorDataPtr(contrib_ch[j], search_start);  /* retrieve pointer to start of contributor audio data */
         int N = (DSGetStreamGroupContributorDataAvailable(contrib_ch[j]) - search_start)/2;  /* return value is in bytes, so divide by 2 to get number of 16-bit audio samples */

      /* compare with local minimum amplitude threshold */

         for (n=0; n<N; n++) if (x[n] > MIN_AMP_THRESH) {

            align_interval_count[idx][j] = max(search_start/2 + n, 1);  /* save offset in samples, make sure it's not zero. Note this is converted later to an interval count */

            num_met++;
            break;
         }
      }
      else num_met++;  /* alignment for this stream already found (align_interval_count is static var) */
   }

/* all streams have a local minimum candidate for cross correlation ? */

   if (num_met == nContributors) {

      int ref_start = 0, ref_contrib = 0;
      int offset[MAX_GROUP_CONTRIBUTORS];

      for (j=0; j<nContributors; j++) {

         offset[j] = 2*align_interval_count[idx][j];  /* save offsets, in bytes */

         if (offset[j] > ref_start) {  /* find alignment point of "reference" stream (in bytes); i.e. one with most delayed offset */

            ref_start = offset[j];
            ref_contrib = j;
         }
      }

      #ifdef USEXCORR 

   /* apply cross correlation */

      int64_t sum = 0;
      int num = 0, nx = 0, ny = 0;

      for (j=0; j<nContributors; j++) {

         if (j != ref_contrib) {

            nx = max(offset[ref_contrib] - XCORR_WINDOW_SIZE, 0);  /* subtract half-window (in bytes) */
            ny = max(offset[j] - XCORR_WINDOW_SIZE, 0);

            short int* x = DSGetStreamGroupContributorDataPtr(contrib_ch[ref_contrib], nx);  /* note - if we exceed amount of available data then we hit zeros in the stream contributor buffers and sum will be zero, but we still increment num. That should not be a problem; if we miss an alignment, we should catch it on incoming frames, JHB Jul2020 */
            short int* y = DSGetStreamGroupContributorDataPtr(contrib_ch[j], ny);

            for (n=0; n<XCORR_WINDOW_SIZE; n++) {

               sum += (int)y[n]*(int)x[n];
               num++;
            }
         }
      }

      sum /= num*(nContributors-1);  /* normalize */

      #ifdef ALIGNDEBUG
      printf("\n === stream %d start %d, ref start %d, xcorr normalized sum = %lld, num = %d \n", j, ny, nx, (long long)sum, num);
      #endif

   /* if normalized crosscorrelation sum is less than threshold, then:

      -reset align_interval_count[]
      -move the amplitude threshold search starting point forward by half of the xcorr window size (50% overlap)
      -repeat the search
   */

      if (sum < XCORR_NORM_SUM_THRESH) {

         for (k=0; k<nContributors; k++) align_interval_count[idx][k] = 0;

         #if 0  /* this way of moving the search point forward doesn't work. The idea was to reduce number of searches by skipping areas that didn't meet the min threshold, JHB Jul2020 */
         int min_start = ref_start;

         for (k=0; k<nContributors; k++) min_start = min(min_start, offset[k]);
         search_start = min_start;

         #else  /* this way moves the search point forward by half the xcorr window size, so it's methodical but unnecessarily processes silence or low energy data */

         search_start += XCORR_WINDOW_SIZE;

         #endif

         for (k=0; k<nContributors; k++) if (search_start >= DSGetStreamGroupContributorDataAvailable(contrib_ch[k])) return 0;  /* return if for any stream we've reached the end of its available data */

         #ifdef ALIGNDEBUG
         printf("=== repeat %d threshold search \n", ++thresh_repeat);
         #endif

         goto thresh_search;
      }
      #endif  /* USEXCORR */

   /* alignment found */

      for (j=0; j<nContributors; j++) {

         int framesize = DSGetStreamGroupContributorFramesize(contrib_ch[j]);  /* get contributor's audio framesize (in bytes) */

      /* set align_interval_count[] values such that streams are delayed (shifted right) to match the reference stream */

         align_interval_count[idx][j] = (ref_start - offset[j] + framesize/2)/framesize;  /* calculate shift amount as number of framesize intervals (in bytes). The shift amount for the reference stream will be zero */
      }

   /* if debug option enabled, insert alignment marker in each stream. In Wireshark or other waveform display, all markers should appear "on top of each other" if alignment has been done correctly, JHB Jul2020 */

      if (lib_dbg_cfg.uDebugMode & DS_INJECT_GROUP_ALIGNMENT_MARKERS) for (j=0; j<nContributors; j++) {

         short int* x = DSGetStreamGroupContributorDataPtr(contrib_ch[j], 0);  /* retrieve pointer to start of contributor audio data */
         n = offset[j]/2;  /* in samples */

         for (k=0; k<6; k++) x[n+k] = 25000;  /* need a few of these due to some type of filtering done by Wireshark waveform display (appears to be a running average a few samples wide) */
      }

   /* generate deduplication event log INFO message */

      char tmpstr[500] = "";
      for (j=0; j<nContributors; j++) sprintf(&tmpstr[strlen(tmpstr)], "  stream %d, alignment offset (bytes) = %u, interval count = %u \n", contrib_ch[j], offset[j], align_interval_count[idx][j]);
      Log_RT(4, "INFO: group %d all streams meet deduplication alignment criteria, reference start = %d\n%s", idx, ref_start, tmpstr);

      ret_val = 1;  /* return value for deduplication alignment found */
   }

   return ret_val;
}
