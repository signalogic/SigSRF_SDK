/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/session_app.cpp

 Copyright (C) Signalogic Inc. 2021-2023

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  Session management related source for mediaMin reference application

 Documentation

  ftp://ftp.signalogic.com/documentation/SigSRF

  (as of Oct 2019, most recent doc is ftp://ftp.signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

 Revision History

   Created Apr 2021 JHB, split off from mediaMin.cpp
   Modified Sep 2022 JHB, add ReadCodecConfig(), split out check_config_file(), used by both ReadSessionConfig() and ReadCodecConfig()
   Modified Dec 2022 JHB, change references to DYNAMIC_CALL to DYNAMIC_SESSIONS
   Modified Jan 2023 JHB, added reference to extern fUntimedMode, in case it should be needed in SetSessionTiming(). See comments in mediaMin.cpp
   Modified May 2023 JHB, set group_term.input_buffer_interval in SetSessionTiming(). This is needed in streamlib for "fast as possible" and "faster than real-time" modes (when 0 <= N < 1 is given for -rN entry on the mediaMin cmd line)
*/

#include <stdio.h>

using namespace std;

#include "mediaTest.h"  /* bring in constants needed by mediaMin.h */

#include "pktlib.h"     /* DSGetPacketInfo() */

#include "mediaMin.h"   /* bring in THREAD_INFO struct typedef */
#include "user_io.h"    /* bring in app_printf() */

extern HPLATFORM hPlatform;            /* initialized by DSAssignPlatform() API in DirectCore lib */
extern APP_THREAD_INFO thread_info[];  /* THREAD_INFO struct defined in mediaMin.h */
extern bool fStressTest;               /* determined from cmd line options, number of app threads, and session re-use */
extern bool fCapacityTest;             /*    ""    ""   */
extern bool fNChannelWavOutput;
extern bool fUntimedMode;              /* set if neither ANALYTICS_MODE nor USE_PACKET_ARRIVAL_TIMES (telecom mode) flags are set in -dN options. This is true of some old test scripts with -r0 push-pull rate (as fast as possible), which is why we call it "untimed" */
extern int nRepeatsRemaining[];

void JitterBufferOutputSetup(int thread_index);  /* set up jitter buffer output for sessions created */
void StreamGroupOutputSetup(HSESSION hSessions[], int nInput, int thread_index);  /* set up stream group output output for sessions created */

#define ENABLE_MANAGED_SESSIONS  /* managed sessions are defined by default. See GetSessionFlags() below */

// #define STREAM_GROUP_BUFFER_TIME  1000  /* default stream group buffer time is 260 msec (2080 samples at 8 kHz sampling rate, 4160 samples at 16 kHz, etc). Uncommenting this define will set the buffer time, in this example to 1 sec */


/* SetSessionTiming() notes:

   -set input and output buffer interval timing. Currently we are using term1.xx values for overall timing
   -called by CreateDynamicSession() in mediaMin.cpp
   -also called by CreateStaticSessions() below
*/

void SetSessionTiming(SESSION_DATA* session_data) {

/* set input buffer intervals */

/* RealTimeInterval[] default value for no -rN cmd line entry is -1, which indicates to use session ptime, JHB May 2023 */
  
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

   if (Mode & AUTO_ADJUST_PUSH_RATE) {  /* set in situations when packet arrival timing is not accurate, for example pcaps without packet arrival timestamps, analytics mode sending packets faster than real-time, etc */

      session_data->term1.uFlags |= TERM_IGNORE_ARRIVAL_PACKET_TIMING;
      session_data->term2.uFlags |= TERM_IGNORE_ARRIVAL_PACKET_TIMING;
   }
   else if (!(Mode & USE_PACKET_ARRIVAL_TIMES)) {

      session_data->term1.uFlags |= TERM_NO_PACKET_TIMING;
      session_data->term2.uFlags |= TERM_NO_PACKET_TIMING;
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


/* helper function used by ReadCodecConfig() and ReadSessionConfig() */

int check_config_file(char* config_file, int thread_index) {

char tmpstr[1024] = "";

   if (strlen(MediaParams[0].configFilename) == 0 || access(MediaParams[0].configFilename, F_OK) == -1) {

      if (strlen(MediaParams[0].configFilename) == 0) return -1;

      strcpy(tmpstr, "../");  /* try up one subfolder, in case cmd user's line entry forgot the "../" prefix */
      strcat(tmpstr, MediaParams[0].configFilename);

      if (access(tmpstr, F_OK) == -1) return -1;
      else strcpy(config_file, tmpstr);
   }
   else strcpy(config_file, MediaParams[0].configFilename);

   if (!strlen(config_file)) return -1;

   return 1;
}

/* codec config files are valid for both dynamic and static session creation */

int ReadCodecConfig(codec_test_params_t* codec_test_params, int thread_index) {

char codec_config_file[1024] = "";
FILE* codec_cfg_fp = NULL;

   if (thread_info[thread_index].init_err) return 0;

   if (check_config_file(codec_config_file, thread_index) < 0) return 0;  /* no error message if not found ... optional on mediaMin cmd line, JHB Sep 2022 */

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

   if (check_config_file(session_config_file, thread_index) < 0) {

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

int CreateStaticSessions(HSESSION hSessions[], SESSION_DATA session_data[], int nSessionsConfigured, int thread_index) {

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

      if (target_delay) session_data[i].term1.jb_config.target_delay = target_delay;
      if (max_delay) session_data[i].term1.jb_config.max_delay = max_delay;

      if (!(Mode & ANALYTICS_MODE) || target_delay > 7) session_data[i].term1.uFlags |= TERM_OOO_HOLDOFF_ENABLE;  /* jitter buffer holdoffs enabled except in analytics compatibility mode */

      if ((Mode & ENABLE_STREAM_GROUPS) || session_data[i].group_term.group_mode > 0) {  /* adjust stream group_mode if needed, prior to creating session */

         Mode |= ENABLE_STREAM_GROUPS;  /* in case stream groups were not enabled on cmd line, but they are for at least one session in the static session config file */

         if (Mode & ENABLE_WAV_OUTPUT) {

            session_data[i].group_term.group_mode |= STREAM_GROUP_WAV_OUT_MERGED | STREAM_GROUP_WAV_OUT_STREAM_MONO;  /* specify mono and group output wav files. If merging is enabled, the group output wav file will contain all input streams merged (unified conversation) */

            if (!fStressTest && !fCapacityTest && nRepeatsRemaining[thread_index] == -1) {  /* specify N-channel wav output. Disable if load/capacity or stress test options are active. Don't enable if repeat is active, otherwise thread preemption warnings may show up in the event log (because N-channel processing takes a while), JHB Jun 2019 */

               session_data[i].group_term.group_mode |= STREAM_GROUP_WAV_OUT_STREAM_MULTICHANNEL;
               fNChannelWavOutput = true;
            }
         }

         session_data[i].term1.uFlags |= TERM_OVERRUN_SYNC_ENABLE;  /* overrun synchronization enabled by default in session config parsing (in transcoder_control.c), enabling again here is redundant and shown only for info purposes */
         session_data[i].term2.uFlags |= TERM_OVERRUN_SYNC_ENABLE;

         if ((Mode & USE_PACKET_ARRIVAL_TIMES) && (Mode & ENABLE_ONHOLD_FLUSH_DETECT)) {
            session_data[i].term1.group_mode |= STREAM_CONTRIBUTOR_ONHOLD_FLUSH_DETECTION_ENABLE;
            session_data[i].term2.group_mode |= STREAM_CONTRIBUTOR_ONHOLD_FLUSH_DETECTION_ENABLE;
         }

         if ((Mode & DISABLE_CONTRIB_PACKET_FLUSH) || (!(Mode & USE_PACKET_ARRIVAL_TIMES) && (Mode & AUTO_ADJUST_PUSH_RATE))) {
            session_data[i].term1.group_mode |= STREAM_CONTRIBUTOR_DISABLE_PACKET_FLUSH;  /* auto-adjust push rate (i.e. not based on timestamp timing) disqualifies use of packet flush, JHB Dec2019 */
            session_data[i].term2.group_mode |= STREAM_CONTRIBUTOR_DISABLE_PACKET_FLUSH;
         }

         if (Mode & ENABLE_DEBUG_STATS) session_data[i].group_term.group_mode |= STREAM_GROUP_DEBUG_STATS;
         if (Mode & ENABLE_DEBUG_STATS_L2) session_data[i].group_term.group_mode |= STREAM_GROUP_DEBUG_STATS_L2;
         if (Mode & DISABLE_FLC) session_data[i].group_term.group_mode |= STREAM_GROUP_FLC_DISABLE;

         if (!session_data[i].group_term.ptime) session_data[i].group_term.ptime = 20;
      }

      SetSessionTiming(&session_data[i]);  /* set termN.input_buffer_interval and termN.output_buffer_interval -- for user apps note it's important this be done */

   /* call DSCreateSession() API (in pktlib .so) */

      if ((hSession = DSCreateSession(hPlatform, NULL, &session_data[i], GetSessionFlags())) >= 0) {

         hSessions[nSessionsCreated++] = hSession;  /* valid session was handle returned from DSCreateSession(), add to hSessions[] */

         #ifdef STREAM_GROUP_BUFFER_TIME
         DSSetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_BUFFER_TIME, STREAM_GROUP_BUFFER_TIME, NULL);  /* if STREAM_GROUP_BUFFER_TIME defined above, set group buffer time to value other than 260 msec default */
         #endif

         thread_info[thread_index].nSessionsCreated++;  /* update per app thread vars */
         thread_info[thread_index].total_sessions_created++;

      /* for debug mode "create sessions from pcap", create 1 initial session, create all others dynamically, based on pcap contents */

         if (Mode & CREATE_DELETE_TEST_PCAP) break;
      }
      else app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_EVENT_LOG, thread_index, "mediaMin INFO: Failed to create static session %d, continuing test with already created sessions \n", i);
   }

   if (nSessionsCreated) {

      JitterBufferOutputSetup(thread_index);  /* set up jitter buffer output for all static sessions created */

      if (Mode & ENABLE_STREAM_GROUPS) {  /* stream group output depends on session creation results, so we do after all static sessions are created. In dynamic sessions mode, it's done when sessions are created after first appearing in the input stream */
   
         StreamGroupOutputSetup(hSessions, 0, thread_index);  /* if any sessions created have a group term, set up stream group output */
      }
   }
   else if (nSessionsConfigured) {

      thread_info[thread_index].init_err = true;
      return -1;  /* return error -- static sessions were configured but none created */
   }

   return nSessionsCreated;
}
