/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/session_app.cpp

 Copyright (C) Signalogic Inc. 2021-2025

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  Session management related source for mediaMin reference application

 Documentation

  https://github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediamin

  Older documentation links:
  
    after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

    before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

 Revision History

   Created Apr 2021 JHB, split off from mediaMin.cpp
   Modified Sep 2022 JHB, add ReadCodecConfig(), split out check_config_file(), used by both ReadSessionConfig() and ReadCodecConfig()
   Modified Dec 2022 JHB, change references to DYNAMIC_CALL to DYNAMIC_SESSIONS
   Modified Jan 2023 JHB, added reference to extern fUntimedMode, in case it should be needed in SetSessionTiming(). See comments in mediaMin.cpp
   Modified May 2023 JHB, set group_term.input_buffer_interval in SetSessionTiming(). This is needed in streamlib for "fast as possible" and "faster than real-time" modes (when 0 <= N < 1 is given for -rN entry on the mediaMin cmd line)
   Modified Jul 2024 JHB, per change in pktlib.h, move uFlags to second param in DSCreateSession()
   Modified Sep 2024 JHB, change TERM_NO_PACKET_TIMING to TERM_NO_PACKET_ARRIVAL_TIMESTAMPS, per flag rename in shared_include/session.h
   Modified Sep 2024 JHB, adjust placement of OutputSetup(), StreamGroupOutputSetup(), and JitterBufferOutputSetup(), which have been restructured and now operate per-session
   Modified Nov 2024 JHB, include directcore.h (no longer implicitly included in other header files)
   Modified Nov 2024 JHB, in CreateStaticSessions() add stream stats entries for created sessions (was already supported for dynamic sessions in mediaMin.cpp)
   Modified Nov 2024 JHB, modify FindStream() to use correct IPv6 header length
   Modified Dec 2024 JHB, set some term2 items that were left out from previous updates. This fixes issues with static session regression test, for example this cmd line:

                             mediaMin -cx86 -i../pcaps/AMRWB.pcap -i../pcaps/pcmutest.pcap -L -d0x08000c10 -r20 -g /tmp/shared -C../session_config/merge_testing_config_amrwb

                           should now produce 3 jitter buffer underrun resyncs (on ch 1) and no stream group underruns, and this cmd line:

                             mediaMin -cx86 -i../pcaps/AMRWB.pcap -i../pcaps/pcmutest.pcap -L -d0x08040c10 -r0.9 -g /tmp/shared -C../session_config/merge_testing_config_amrwb --md5sum

                           should produce an md5 sum ending in d689c8 for -rN entries from 0.8 to 20

   Modified Dec 2024 JHB, include <algorithm> and use std namespace
   Modified Feb 2025 JHB, change references to MAX_INPUT_STREAMS to MAX_STREAMS
   Modified Mar 2025 JHB, add cur_time param to CreateStaticSessions(), which gives it to app_printf()
   Modified Apr 2025 JHB, in CreateStaticSessions() update stream stats initialization per changes in mediaMin.h and mediaMin.cpp
   Modified Jun 2025 JHB, in CreateStaticSessions[] update thread vars nSessionsCreated and total_sessions_created before calling JitterBufferOutputSetup() and OutputSetup(), which rely on GetSessionIndex() which references those thread vars. Test with legacy session config files merge_1613-1_1613-0_config_evs and multi_session_evs_merge_testing_config
   Modified Jun 2025 JHB, rename check_config_file() to CheckConfigFile(), add to session_app.h, and change return value to be length of config file path found, if any
   Modified Jun 2025 JHB, fix bug in stream group wav output naming (szSessionName[] was not included)
   Modified Jul 2025 JHB, move FindStream() here from mediaMin.cpp
*/

#include <algorithm>
using namespace std;

#include <stdio.h>

/* SigSRF includes */

#include "pktlib.h"      /* DSGetPacketInfo() */

/* app support header files */

#include "mediaTest.h"   /* bring in constants needed by mediaMin.h */
#include "mediaMin.h"    /* bring in THREAD_INFO struct typedef */
#include "user_io.h"     /* bring in app_printf() */
#include "session_app.h"

extern HPLATFORM hPlatform;            /* initialized by DSAssignPlatform() API in DirectCore lib */
extern APP_THREAD_INFO thread_info[];  /* THREAD_INFO struct defined in mediaMin.h */
extern bool fCreateDeleteTest;         /* determined from cmd line options (note - renamed from fStressTest, JHB Jun 2025) */
extern bool fCapacityTest;             /* set when number of app threads and/or input reuse is enabled */
extern bool fNChannelWavOutput;
extern bool fUntimedMode;              /* set if neither ANALYTICS_MODE nor USE_PACKET_ARRIVAL_TIMES (telecom mode) flags are set in -dN options. This is true of some old test scripts with -r0 push-pull rate (as fast as possible), which is why we call it "untimed" */
extern int nRepeatsRemaining[];
extern char szSessionName[][384];      /* initialized in LoggingSetup() in mediaMin.cpp */

/* functions currently in mediaMin.cpp */

void JitterBufferOutputSetup(HSESSION hSessions[], HSESSION hSession, int thread_index);  /* set up jitter buffer output */
int OutputSetup(HSESSION hSessions[], HSESSION hSession, int thread_index);  /* set up application packet/media packet output */
void StreamGroupOutputSetup(HSESSION hSession, int nStream, int thread_index);  /* set up stream group output */


/* FindStream() examines an IP/UDP packet for a new stream. Callers can use results to determine if a new session should be created "on the fly" (i.e. dynamic session creation). Notes:

  -a new stream is determined by (i) new IP addr:port header and/or (ii) new RTP payload type. This info is combined into a "key" that defines the stream and is saved to compare with existing streams
  -SSRC is not normally included in the key, in order to maintain RFC8108 compliance (multiple RTP streams within the same initial stream). However, if the ENABLE_SSRC_STREAM_JOINING flag in the -dN command line argument is active then only SSRCs are used to match streams from different endpoints; in this case 0 is returned to indicate an existing stream and FIND_STREAM_SSRC_MATCH is set in pStreamFlags. This is a limited mode for specific applications
  -DTMF packets must match an existing stream excluding payload type; i.e. they will not cause a new key to be created
  -each application thread has its own set of keys to avoid semaphores / locks. Duplicated streams across multiple inputs / threads are currently not detected, although PushPackets() does look for pcaps repeated on the cmd line (and deals with it by slightly altering duplicated inputs to make them unique, look for fDuplicatedHeaders). This could be addressed by including additional info in the key (e.g. stream, thread)

  -return value is zero for an existing stream, total streams found so far for a new stream, or -1 for an error condition
*/

#define MAX_KEYS 512  /* increased from 128, JHB Jun 2024 */

/* keys are unique per stream (they are not hashes). No run-time locks are needed */ 

#define KEY_LENGTH 37  /* each key is up to 37 bytes (ipv6 address size (2*16) + udp port size (2*2)) + RTP payload type (1) */

static uint8_t keys[MAX_APP_THREADS][MAX_KEYS][KEY_LENGTH] = {{{ 0 }}};
static uint32_t ssrcs[MAX_APP_THREADS][MAX_KEYS] = {{ 0 }};
static int nKeys[MAX_APP_THREADS] = { 0 };

int FindStream(uint8_t* pkt, int ip_hdr_len, uint8_t rtp_pyld_type, uint32_t ssrc, bool fDTMF, unsigned int* pStreamFlags, int thread_index) {

int version, len;
uint8_t key[KEY_LENGTH] = { 0 };
unsigned int uStreamFlags = 0;
bool fPayloadTypeInKey = false, fExistingStream = false;

   if (!pkt) { Log_RT(2, "mediaMin ERROR: FindStream() says pkt is NULL \n"); return -1; }

   if (isRTCPPacket(rtp_pyld_type)) { if (pStreamFlags) *pStreamFlags = FIND_STREAM_RTCP_PACKET; return -1; }  /* RTCP packets not accepted for creating / comparing streams */

/* form key from IP addresses and ports */

   version = pkt[0] >> 4;
   memcpy(key, &pkt[version == IPv4 ? IPV4_ADDR_OFS : IPV6_ADDR_OFS], (len = 2*(version == IPv4 ? IPV4_ADDR_LEN : IPV6_ADDR_LEN)));  /* copy src and dst IP addresses to key */
   memcpy(&key[len], &pkt[ip_hdr_len], 2*sizeof(unsigned short int));  /* copy src and dst UDP ports to key */
   len += 2*sizeof(unsigned short int);

/* copy RTP payload type to key (but not for DTMF packets, which must match an existing stream, JHB May 2019) */

   if (!fDTMF && !fExclude_payload_type_from_key) { fPayloadTypeInKey = true; key[len++] = rtp_pyld_type; }  /* also check if --exclude_payload_type_from key is on the command line, JHB Jul 2025 */

/* see if we already know about this stream */

   for (int i=0; i<nKeys[thread_index]; i++) if (!memcmp(&keys[thread_index][i], key, len)) { fExistingStream = true; break; }  /* matches existing stream */

/* possible new stream */

   if (!fExistingStream && (Mode & ENABLE_SSRC_STREAM_JOINING)) for (int i=0; i<nKeys[thread_index]; i++) if (ssrc == ssrcs[thread_index][i]) {  /* if SSRC stream joining is enabled then look for existing stream with same SSRC, JHB Jul 2025 */

   /* report in stream flags that stream matches existing stream with same SSRC */

      uStreamFlags |= FIND_STREAM_SSRC_MATCH;

      if (fPayloadTypeInKey && rtp_pyld_type != keys[thread_index][i][len-1]) uStreamFlags |= FIND_STREAM_PAYLOAD_TYPE_UNMATCHED;  /* advise caller if payload type does not match (different payload types imply different codecs, something to consider) */

      #if 0
      static bool fOnce[10] = { false };
      if (!fOnce[i]) { fOnce[i] = true; printf("\n *** pkt#%u joining stream SSRC = 0x%x, i = %d, nKeys = %d \n", thread_info[thread_index].packet_number[0], ssrc, i, nKeys[thread_index]); }
      #endif

      fExistingStream = true;  /* existing stream */
      break;
   }
   
   if (!fExistingStream) {  /* if no match create a new stream key */

      if (nKeys[thread_index] >= MAX_KEYS) {  /* return error condition if not enough space for a new key */
         Log_RT(2, "mediaMin ERROR: FindStream() exceeds %d allowable stream keys \n", MAX_KEYS);
         return -1;
      }

      memcpy(&keys[thread_index][nKeys[thread_index]], key, len);  /* store key */
      ssrcs[thread_index][nKeys[thread_index]] = ssrc;  /* store SSRC */

      #if 0
      printf("\n *** pkt#%u storing ssrc = 0x%x, nKeys = %d, len = %d \n", thread_info[thread_index].packet_number[0], ssrcs[thread_index][nKeys[thread_index]], nKeys[thread_index], len);
      #endif

      nKeys[thread_index]++;
   }

   #if 0
   static int cnt = 0;
   printf("check_for_new_sesion: cnt = %d, ret_val = %d, nKeys = %d, len = %d\n", cnt++, ret_val, nKeys[thread_index], len);
   printf("key value: ");
   for (i = 0; i < KEY_LENGTH; i++) printf("%02x ", key[thread_index][i]);
   printf("\n");
   #endif

   if (pStreamFlags) *pStreamFlags = uStreamFlags;

   return fExistingStream ? 0 : nKeys[thread_index];
}

void RemoveLastStreamKey(int thread_index) {

   if (nKeys[thread_index] > 0) {

      nKeys[thread_index]--;
      memset(&keys[thread_index][nKeys[thread_index]], 0, KEY_LENGTH);
      ssrcs[thread_index][nKeys[thread_index]] = 0;
   }
}

void ResetStreamKeys(int thread_index) {

   nKeys[thread_index] = 0;
   memset(&keys[thread_index], 0, MAX_KEYS*KEY_LENGTH*sizeof(uint8_t));
   memset(&ssrcs[thread_index], 0, MAX_KEYS*sizeof(ssrcs[0][0]));
}

/* SetSessionTiming() notes:

   -set input and output buffer interval timing. Currently we are using term1.xx values for overall timing
   -called by CreateDynamicSession() in mediaMin.cpp
   -also called by CreateStaticSessions() below
*/

void SetSessionTiming(SESSION_DATA* session_data) {

/* set input buffer intervals */

   #if 0
   if (Mode & ANALYTICS_MODE) {  /* if -dN cmd line entry specifies analytics mode, we set termN buffer_interval values to zero regardless of what they already are, and regardless of -rN cmd line entry */

      session_data->term1.input_buffer_interval = 0;
      session_data->term2.input_buffer_interval = 0;
   }
   else if ((int)RealTimeInterval[0] != -1) {  /* RealTimeInterval[0] is value of N in -rN cmd line entry */
      if ((int)RealTimeInterval[0] < session_data->term1.ptime) session_data->term1.input_buffer_interval = 0;
      else session_data->term1.input_buffer_interval = RealTimeInterval[0];

      if ((int)RealTimeInterval[0] < session_data->term2.ptime) session_data->term2.input_buffer_interval = 0;
      else session_data->term2.input_buffer_interval = RealTimeInterval[0];
   }
   #else
   if (Mode & ANALYTICS_MODE) {  /* if -dN cmd line entry specifies analytics mode set TERM_ANALYTICS_MODE_TIMING flag. Note this decouples analytics mode from input_buffer_interval, which is needed for FTRT and AFAP modes, JHB May 2023 */

      session_data->term1.uFlags |= TERM_ANALYTICS_MODE_PACKET_TIMING;
      session_data->term2.uFlags |= TERM_ANALYTICS_MODE_PACKET_TIMING;
   }

   if ((int)RealTimeInterval[0] != -1) {  /* RealTimeInterval[0] is value of N in -rN cmd line entry */
      session_data->term1.input_buffer_interval = RealTimeInterval[0];
      session_data->term2.input_buffer_interval = RealTimeInterval[0];
   }
   #endif

   if (session_data->term1.input_buffer_interval == -1) session_data->term1.input_buffer_interval = session_data->term1.ptime;  /*  if buffer_interval values are not given in either programmatic session setup (dynamic sessions) or session config file, then set to ptime */
   if (session_data->term2.input_buffer_interval == -1) session_data->term2.input_buffer_interval = session_data->term2.ptime;

   if (Mode & AUTO_ADJUST_PUSH_TIMING) {  /* set in situations when packet arrival timing is not accurate, for example pcaps without packet arrival timestamps, analytics mode sending packets faster than real-time, etc */

      session_data->term1.uFlags |= TERM_IGNORE_ARRIVAL_PACKET_TIMING;
      session_data->term2.uFlags |= TERM_IGNORE_ARRIVAL_PACKET_TIMING;
   }
   else if (!(Mode & USE_PACKET_ARRIVAL_TIMES)) {

      session_data->term1.uFlags |= TERM_NO_PACKET_ARRIVAL_TIMESTAMPS;
      session_data->term2.uFlags |= TERM_NO_PACKET_ARRIVAL_TIMESTAMPS;
   }

/* set output buffer intervals:

   -required for packet loss flush and pastdue flush to be active (see packet_flow_media_proc.c)
   -required for accurate stream group output timing (i.e. should be set if stream groups are active)
*/

   if (session_data->term1.output_buffer_interval == -1 || (Mode & DYNAMIC_SESSIONS)) {

      if ((Mode & ANALYTICS_MODE) || session_data->term1.input_buffer_interval > 0) session_data->term1.output_buffer_interval = session_data->term2.ptime;  /* output intervals use ptime from opposite terms */
      else session_data->term1.output_buffer_interval = 0;
   }

   if (session_data->term2.output_buffer_interval == -1 || (Mode & DYNAMIC_SESSIONS)) {

      if ((Mode & ANALYTICS_MODE) || session_data->term2.input_buffer_interval) session_data->term2.output_buffer_interval = session_data->term1.ptime;
      else session_data->term2.output_buffer_interval = 0;
   }

   if (Mode & ENABLE_STREAM_GROUPS) {

      if ((Mode & ANALYTICS_MODE) ||
          (session_data->term1.input_buffer_interval > 0 && session_data->term1.group_mode) ||
          (session_data->term2.input_buffer_interval > 0 && session_data->term2.group_mode)) session_data->group_term.output_buffer_interval = session_data->group_term.ptime;

      if (session_data->group_term.output_buffer_interval < 0) session_data->group_term.output_buffer_interval = 0;  /* if not specified, set to zero */

   /* also set group term input_buffer_interval. This is needed in streamlib to handle "fast as possible mode" timing (-r0 on command line), JHB May 2023 */

      if (session_data->term1.input_buffer_interval > 0 && session_data->term1.group_mode) session_data->group_term.input_buffer_interval = session_data->term1.input_buffer_interval;
      else if (session_data->term2.input_buffer_interval > 0 && session_data->term2.group_mode) session_data->group_term.input_buffer_interval = session_data->term2.input_buffer_interval;

      if (session_data->group_term.input_buffer_interval < 0) session_data->group_term.input_buffer_interval = 0;  /* if not specified, set to zero */
   }

   if ((int)RealTimeInterval[0] == -1) RealTimeInterval[0] = session_data->term1.input_buffer_interval;
}


/* GetSessionFlags() notes:

   -determine uFlags for subsequent call to DSCreateSession() in pktlib
   -called by StaticSessionCreate() below and create_dynamic_session() in mediaMin.cpp
   -DS_SESSION_MODE_IP_PACKET is in shared_include/session_cmd.h, other flags in pktlib.h
*/

#define ENABLE_MANAGED_SESSIONS  /* managed sessions are defined by default */

unsigned int GetSessionFlags() {

   unsigned int uFlags = DS_SESSION_MODE_IP_PACKET | DS_SESSION_DYN_CHAN_ENABLE | DS_SESSION_DISABLE_PRESERVE_SEQNUM;  /* default flags for DSCreateSession()*/

   #if 0
   uFlags |= DS_SESSION_STATE_ALLOW_DYNAMIC_ADJUST;  /* add dynamic jitter buffer delay adjust option, if needed */
   #endif

   #ifdef ENABLE_MANAGED_SESSIONS
   uFlags |= DS_SESSION_USER_MANAGED;
   #endif

   #ifdef ALLOW_BACKGROUND_PROCESS  /* deprecated, no longer used */
   if (use_bkgnd_process) {
      uFlags |= DS_SESSION_DP_LINUX_SOCKETS;
   }
   else
   #endif

   if (!fNetIOAllowed) uFlags |= DS_SESSION_DISABLE_NETIO;

   return uFlags;
}


/* helper function used by ReadCodecConfig() and ReadSessionConfig(). Also may be called near start of mediaMin.cpp, depending on -dN cmd line entry */

int CheckConfigFile(char* config_file, int thread_index) {

char tmpstr[1024] = "";
int ret_val = -1;  /* default return value is error condition */

   (void)thread_index;  /* not currently used */

   if (strlen(MediaParams[0].configFilename) == 0) { ret_val = 0; goto exit; }  /* return zero for config file path length. Indicates no -Cxxx entry on cmd line, but not an error condition */

   if (access(MediaParams[0].configFilename, F_OK) == -1) {

      strcpy(tmpstr, "../");  /* try up one folder, in case cmd user's line entry forgot the "../" prefix */
      strcat(tmpstr, MediaParams[0].configFilename);

      if (access(tmpstr, F_OK) == -1) goto exit;  /* can't find config file path either on caller's current folder or up one folder */

      if (config_file) strcpy(config_file, tmpstr);  /* file found */
      ret_val = strlen(tmpstr);
   }
   else {

      if (config_file) strcpy(config_file, MediaParams[0].configFilename);  /* file found */
      ret_val = strlen(MediaParams[0].configFilename);
   }

exit:

   return ret_val;  /* return value is now length of config file path, if found, JHB Jun 2025 */
}

/* codec config files are valid for both dynamic and static session creation */

int ReadCodecConfig(codec_test_params_t* codec_test_params, int thread_index) {

char codec_config_file[1024] = "";
FILE* codec_cfg_fp = NULL;

   if (thread_info[thread_index].init_err) return 0;

   if (CheckConfigFile(codec_config_file, thread_index) <= 0) return 0;  /* no error message if not found ... optional on mediaMin cmd line, JHB Sep 2022. Check for <= 0 as CheckConfigFile() return value is now length of config file path, if found, JHB Jun 2025 */

   printf("Opening codec config file: %s\n", codec_config_file);

/* open codec config file */

   codec_cfg_fp = fopen(codec_config_file, "r");

   if (codec_cfg_fp == NULL) {

      fprintf(stderr, "mediaMin Error: ReadCodecConfig() says failed to open codec config file %s (%d)\n", MediaParams[0].configFilename, thread_index);
      thread_info[thread_index].init_err = true;

      return -1;
   }

/* parse codec config file */

   parse_codec_config(codec_cfg_fp, codec_test_params);  /* in transcoder_control.c */

/* close codec config file */

   fclose(codec_cfg_fp);

   return 1;
}

/* read session configuration file info needed to create static sessions. Note that static vs. dynamic session creation depends on -dN cmd line entry, see Mode var comments in mediaMin.h */

int ReadSessionConfig(SESSION_DATA session_data[], int thread_index) {

char default_session_config_file[] = "session_config/packet_test_config";
char session_config_file[1024] = "";
FILE* session_cfg_fp = NULL;
int nSessionsConfigured = 0;

   if (thread_info[thread_index].init_err) return 0;

   if (CheckConfigFile(session_config_file, thread_index) <= 0) {  /* check for <= 0 as CheckConfigFile() return value is now length of config file path, if found, JHB Jun 2025 */

      printf("mediaMin Info: cannot find specified session config file: %s, using default file (%d)\n", MediaParams[0].configFilename, thread_index);

      strcpy(session_config_file, default_session_config_file);
   }

   printf("Opening session config file: %s\n", session_config_file);

/* open session config file */

   session_cfg_fp = fopen(session_config_file, "r");

   if (session_cfg_fp == NULL) {

      fprintf(stderr, "mediaMin Error: ReadSessionConfig() says failed to open static session config file %s, exiting mediaMin (%d)\n", session_config_file, thread_index);
      thread_info[thread_index].init_err = true;

      return 0;
   }

/* parse session config file. Note there can be multiple sessions and we monitor run var in case the user exits using 'q' key */

   while (pm_run > 0 && (parse_session_config(session_cfg_fp, &session_data[nSessionsConfigured]) != -1)) nSessionsConfigured++;  /* in transcoder_control.c */

   printf("mediaMin Info: ReadSessionConfig() says %d session(s) found in config file\n", nSessionsConfigured);

   if (nSessionsConfigured > MAX_SESSIONS) {

      fprintf(stderr, "mediaMin Warning: ReadSessionConfig() says number of sessions exceeds pktlib max, reducing to %d\n", MAX_SESSIONS);
      nSessionsConfigured = MAX_SESSIONS;
   }

/* close session config file */

   fclose(session_cfg_fp);

   return nSessionsConfigured;
}

/* create static sessions */

int CreateStaticSessions(HSESSION hSessions[], SESSION_DATA session_data[], int nSessionsConfigured, uint64_t cur_time, int thread_index) {

int i, nSessionsCreated = 0;
HSESSION hSession;

   for (i=0; i<nSessionsConfigured; i++) {

      printf("++++++++Creating session %d\n", thread_info[thread_index].total_sessions_created);

      if (Mode & CREATE_DELETE_TEST) {  /* change group ID names */

         static int create_counter = 10000;
         char tmp_str[128];

         itoa(create_counter, tmp_str, 10);
         if (session_data[i].group_term.group_mode > 0) memmove(&session_data[i].group_term.group_id[strlen(session_data[i].group_term.group_id)-5], tmp_str, strlen(tmp_str));
         if (session_data[i].term1.group_mode > 0) memmove(&session_data[i].term1.group_id[strlen(session_data[i].term1.group_id)-5], tmp_str, strlen(tmp_str));
         if (session_data[i].term2.group_mode > 0) memmove(&session_data[i].term2.group_id[strlen(session_data[i].term2.group_id)-5], tmp_str, strlen(tmp_str));

         if (i == nSessionsConfigured-1) create_counter++;  /* bug fix, JHB Jan 2019 */
      }

      if (Mode & DISABLE_DTX_HANDLING) {  /* DTX handling enabled by default in session config parsing (in transcoder_control.c), disable here if specified in cmd line */
         session_data[i].term1.uFlags &= ~TERM_DTX_ENABLE;
         session_data[i].term2.uFlags &= ~TERM_DTX_ENABLE;
      }

      if (Mode & DISABLE_PACKET_REPAIR) {  /* packet repair flags enabled by default in session config parsing (in transcoder_control.c), disable them here if specified in cmd line */
         session_data[i].term1.uFlags &= ~(TERM_SID_REPAIR_ENABLE | TERM_PKT_REPAIR_ENABLE);
         session_data[i].term2.uFlags &= ~(TERM_SID_REPAIR_ENABLE | TERM_PKT_REPAIR_ENABLE);
      }

      if (thread_info[thread_index].nInPcapFiles > 1) session_data[i].term2.uFlags |= TERM_EXPECT_BIDIRECTIONAL_TRAFFIC;  /* if we have multiple cmd line inputs, and we are in static session mode, we can set this flag, which makes p/m thread receive queue handling more efficient for bidirectional traffic */

      int target_delay = 0, max_delay = 0;

      if (nJitterBufferParams >= 0) {  /* cmd line param -jN, if entered. nJitterBufferParams is -1 if no cmd line entry */
         target_delay = nJitterBufferParams & 0xff;
         max_delay = (nJitterBufferParams & 0xff00) >> 8;
      }
      else if ((Mode & ENABLE_STREAM_GROUPS) || session_data[i].group_term.group_mode > 0) {
         target_delay = 10;
         max_delay = 14;
      }

      if (target_delay) {
         session_data[i].term1.jb_config.target_delay = target_delay;
         session_data[i].term2.jb_config.target_delay = target_delay;  /* also set term2, JHB Dec 2024 */
      }
      if (max_delay) {
         session_data[i].term1.jb_config.max_delay = max_delay;
         session_data[i].term2.jb_config.max_delay = max_delay;  /* also set term2, JHB Dec 2024 */
      }

      if (!(Mode & ANALYTICS_MODE) || fUntimedMode || target_delay > 7) {

         session_data[i].term1.uFlags |= TERM_OOO_HOLDOFF_ENABLE;  /* jitter buffer holdoffs enabled except in analytics compatibility mode */
         session_data[i].term2.uFlags |= TERM_OOO_HOLDOFF_ENABLE;  /* also set term2, JHB Dec 2024 */
      }

      session_data[i].term1.RFC7198_lookback = uLookbackDepth;
      session_data[i].term2.RFC7198_lookback = uLookbackDepth;

      if ((Mode & ENABLE_STREAM_GROUPS) || session_data[i].group_term.group_mode > 0) {  /* adjust stream group_mode if needed, prior to creating session */

         Mode |= ENABLE_STREAM_GROUPS;  /* in case stream groups were not enabled on cmd line, but they are for at least one session in the static session config file */

         if (Mode & ENABLE_WAV_OUTPUT) {

            session_data[i].group_term.group_mode |= STREAM_GROUP_WAV_OUT_MERGED | STREAM_GROUP_WAV_OUT_STREAM_MONO;  /* specify mono and group output wav files. If merging is enabled, the group output wav file will contain all input streams merged (unified conversation) */

            if (!fCreateDeleteTest && !fCapacityTest && nRepeatsRemaining[thread_index] == -1) {  /* specify N-channel wav output. Disable if load/capacity or stress test options are active. Don't enable if repeat is active, otherwise thread preemption warnings may show up in the event log (because N-channel processing takes a while), JHB Jun 2019 */

               session_data[i].group_term.group_mode |= STREAM_GROUP_WAV_OUT_STREAM_MULTICHANNEL;

            /* set session_data[].szSessionName for wav outputs, JHB Jun 2025 */

               int nStream = 0;
               strcpy(thread_info[thread_index].szGroupName[nStream], szSessionName[nStream]);
               sprintf(session_data[i].szSessionName, "%s%s", szStreamGroupWavOutputPath, thread_info[thread_index].szGroupName[nStream]);

               fNChannelWavOutput = true;
            }
         }

         session_data[i].term1.uFlags |= TERM_OVERRUN_SYNC_ENABLE;  /* overrun synchronization enabled by default in session config parsing (in transcoder_control.c), enabling again here is redundant and shown only for info purposes */
         session_data[i].term2.uFlags |= TERM_OVERRUN_SYNC_ENABLE;

         if ((Mode & USE_PACKET_ARRIVAL_TIMES) && (Mode & ENABLE_ONHOLD_FLUSH_DETECT)) {
            session_data[i].term1.group_mode |= STREAM_CONTRIBUTOR_ONHOLD_FLUSH_DETECTION_ENABLE;
            session_data[i].term2.group_mode |= STREAM_CONTRIBUTOR_ONHOLD_FLUSH_DETECTION_ENABLE;
         }

         if ((Mode & DISABLE_CONTRIB_PACKET_FLUSH) || (!(Mode & USE_PACKET_ARRIVAL_TIMES) && (Mode & AUTO_ADJUST_PUSH_TIMING))) {
            session_data[i].term1.group_mode |= STREAM_CONTRIBUTOR_DISABLE_PACKET_FLUSH;  /* auto-adjust push rate (i.e. not based on timestamp timing) disqualifies use of packet flush, JHB Dec2019 */
            session_data[i].term2.group_mode |= STREAM_CONTRIBUTOR_DISABLE_PACKET_FLUSH;
         }

         if (Mode & ENABLE_DEBUG_STATS) session_data[i].group_term.group_mode |= STREAM_GROUP_DEBUG_STATS;
         if (Mode & ENABLE_DEBUG_STATS_L2) session_data[i].group_term.group_mode |= STREAM_GROUP_DEBUG_STATS_L2;
         if (Mode & DISABLE_FLC) session_data[i].group_term.group_mode |= STREAM_GROUP_FLC_DISABLE;

         if (!session_data[i].group_term.ptime) session_data[i].group_term.ptime = 20;
      }

      SetSessionTiming(&session_data[i]);  /* set termN.input_buffer_interval and termN.output_buffer_interval -- for user apps note it's important this be done */

   /* call DSCreateSession() API (defined in pktlib.h) */

      if ((hSession = DSCreateSession(hPlatform, GetSessionFlags(), NULL, &session_data[i])) >= 0) {

         hSessions[thread_info[thread_index].nSessionsCreated] = hSession;  /* valid session was handle returned from DSCreateSession(); add to hSessions[]. Note we append to any dynamic or static sessions already created */

         thread_info[thread_index].nSessionsCreated++;  /* update per app thread vars. Note this needs to be done before calling JitterBufferOutputSetup() or OutputSetup(), which rely on GetSessionIndex(), JHB Jun 2025 */
         thread_info[thread_index].total_sessions_created++;
         nSessionsCreated++;  /* increment local count */

        // #define STREAM_GROUP_BUFFER_TIME  1000  /* default stream group buffer time is 260 msec (2080 samples at 8 kHz sampling rate, 4160 samples at 16 kHz, etc). Uncommenting this define will set the buffer time, in this example to 1 sec */

         #ifdef STREAM_GROUP_BUFFER_TIME
         DSSetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_BUFFER_TIME, STREAM_GROUP_BUFFER_TIME, NULL);  /* if STREAM_GROUP_BUFFER_TIME defined above, set group buffer time to value other than 260 msec default */
         #endif

         JitterBufferOutputSetup(hSessions, hSession, thread_index);  /* set up jitter buffer output for this session */

         if (!OutputSetup(hSessions, hSession, thread_index)) {  /* set up next matching cmd line output for this session, if any (e.g. transcoded audio, video bitstream), JHB Sep 2024 */

         /* if no matching cmd line output spec found, disable output queue packets for this session */

            if (!(Mode & AUTO_ADJUST_PUSH_TIMING)) {  /* auto-adjust push rate requires transcoded packets for its queue balancing algorithm regardless of cmd line, so don't disable in that case */
 
               DSSetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_TERM_FLAGS, 1, (void*)TERM_DISABLE_OUTPUT_QUEUE_PACKETS);  /* note - positive value will OR with current flags, neg value will AND */
               DSSetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_TERM_FLAGS, 2, (void*)TERM_DISABLE_OUTPUT_QUEUE_PACKETS);
            }
         }

         if (Mode & ENABLE_STREAM_GROUPS) {

            int nStream = 0;  /* default is first cmd line input */

            StreamGroupOutputSetup(hSession, nStream, thread_index);  /* set up stream group output if session is a group owner */

           // thread_info[thread_index].fGroupOwnerCreated[!(Mode & COMBINE_INPUT_SPECS) ? nStream : 0][nReuse] = true;  /* done in CreateDynamicSession(), not sure yet if this is needed for static sessions */
         }

      /* update stream stats, JHB Nov 2024 */

         for (int j=0; j<MAX_TERMS; j++) {

            if (thread_info[thread_index].num_stream_stats < MAX_STREAMS) {

               TERMINATION_INFO termInfo = !j ? session_data[i].term1 : session_data[i].term2;

               if (termInfo.remote_ip.type && termInfo.local_ip.type) {  /* make sure term been initialized */

                  thread_info[thread_index].StreamStats[thread_info[thread_index].num_stream_stats].uFlags |= STREAM_STAT_STATIC_SESSION;
 
                  thread_info[thread_index].StreamStats[thread_info[thread_index].num_stream_stats].hSession = hSession;
                  thread_info[thread_index].StreamStats[thread_info[thread_index].num_stream_stats].term = j;
                  thread_info[thread_index].StreamStats[thread_info[thread_index].num_stream_stats].chnum = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CHNUM, j+1, NULL);  /* channel number known after DSCreateSession() called */

                  char codec_name[100] = "";
                  if (DSGetCodecInfo(termInfo.codec_type, DS_CODEC_INFO_NAME, 0, 0, codec_name) >= 0) strcpy(thread_info[thread_index].StreamStats[thread_info[thread_index].num_stream_stats].codec_name, codec_name);

                  thread_info[thread_index].StreamStats[thread_info[thread_index].num_stream_stats].bitrate = termInfo.bitrate;
                  thread_info[thread_index].StreamStats[thread_info[thread_index].num_stream_stats].payload_type = termInfo.voice.rtp_payload_type;

                  thread_info[thread_index].num_stream_stats++;
               }
            }
         }

      /* for debug mode "create sessions from pcap", create 1 initial session, create all others dynamically, based on pcap contents */

         if (Mode & CREATE_DELETE_TEST_PCAP) break;
      }
      else app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_EVENT_LOG, cur_time, thread_index, "mediaMin INFO: Failed to create static session %d%s \n", i, i > 0 ? ", continuing test with already created sessions" : "");

   }  /* nSessionsConfigured loop */

   if (nSessionsConfigured && !nSessionsCreated) {

      thread_info[thread_index].init_err = true;
      return -1;  /* return error -- static sessions were configured but none created */
   }

   return nSessionsCreated;
}
