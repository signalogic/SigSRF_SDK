/*
  mediaMin.h

  header file for mediaMin (i) simplified SigSRF push/pull interface and (ii) test and measurement program
  
  Copyright (C) Signalogic, 2018-2020

  Revision History

   Created Nov 2018 JHB, moved struct typedefs and other items out of mediaMin.c
   Modified Oct 2019 JHB, index fGroupTermCreated by number of calls to allow multiple multistream pcaps per cmd line, index fp_pcap_jb by number of sessions to allow jitter buffer output per session
   Modified Jan 2020 JHB, add nSessions, hSessions, and fDuplicatedHeaders for increased flexibility in handling multiple call / stream group cmd line inputs
   Modified Jan 2020 JHB, flush_state per-session (see comments about per-session flush in mediaThread_test_app.c), increase size of flushstr
   Modified Mar 2020 JHB, index fFirstGroupPull[], add first_packet_push_time[]
   Modified Apr 2020 JHB, add hSession to DYNAMICSESSIONSTATS struct, increase max number of dynamic session stats to allow for repeating tests with multiple cmd line inputs
   Modified Oct 2020 JHB, expand link_layer_len to int32_t to handle mods to DSOpenPcap() and DSReadPcap() (pktlib) made to support pcapng format. Note - still needs to be an int (not unint) because DSOpenPcap() returns values < 0 for error conditions
*/

#ifndef _MEDIAMIN_H_
#define _MEDIAMIN_H_

#define MAX_INPUT_REUSE  16  /* in practice, cmd line entry up to -N9 has been tested (reuse 10x) */

extern GLOBAL_CONFIG pktlib_gbl_cfg;

enum {
   INIT,
   CREATE,
   DELETE
};

typedef struct {

   uint32_t retry_interval;
   uint16_t num_retries;
   HSESSION hSession;
   
} GROUPPULLSTATS;

typedef struct {

   uint32_t missed_interval;
   uint16_t repeats;
   HSESSION hSession;
   
} GROUPINTERVALSTATS;

#define MAX_GROUP_STATS  512

typedef struct {

   HSESSION hSession;
   char codecstr[20];
   uint16_t bitrate;
   uint8_t payload_type;
   
} DYNAMICSESSIONSTATS;

#define MAX_DYNAMIC_SESSION_STATS  512

/* thread_info contains per-thread local vars.  If mediaMin is run from the cmd line then there is just one application thread, if mediaMin is run from mediaTest with -Et command line entry, then -tN entry determines how many application threads */

typedef struct {

  int      nSessionsCreated;
  int      nSessionsDeleted;
  int      nDynamicSessions;
  uint32_t total_sessions_created;

  int16_t  nInPcapFiles;
  int16_t  nOutPcapFiles;

  int32_t  link_layer_len[MAX_INPUT_STREAMS];  /* note: the current definition of MAX_INPUT_STREAMS (512, in mediaTest.h) is overkill.  Something like 10 to 50 pcaps, with say up to 10 streams (sessions) each, is more realistic */
  FILE*    pcap_in[MAX_INPUT_STREAMS];
  uint16_t input_index[MAX_INPUT_STREAMS];

  FILE*    pcap_out[MAX_INPUT_STREAMS];

  int      nSessions[MAX_INPUT_STREAMS];
  int      nSessionIndex[MAX_INPUT_STREAMS][MAX_SESSIONS];
  bool     fDuplicatedHeaders[MAX_INPUT_STREAMS];
  FILE*    fp_pcap_jb[MAX_SESSIONS];
  bool     init_err;

  uint32_t num_packets_in[MAX_INPUT_STREAMS];

  FILE*    fp_pcap_group[MAX_STREAM_GROUPS];  /* note:  this array is accessed by a session counter, and each app thread might handle up to 50 sessions, so this size (172, defined in shared_include/streamlib.h) is overkill.  But leave it for now */
  FILE*    fp_text_group[MAX_STREAM_GROUPS];
  char     szGroupName[MAX_STREAM_GROUPS][MAX_GROUPID_LEN];
  bool     fGroupTermCreated[MAX_STREAM_GROUPS][MAX_INPUT_REUSE];  /* used in dynamic call mode */

  bool                fFirstGroupPull[MAX_STREAM_GROUPS];
  GROUPPULLSTATS      GroupPullStats[MAX_GROUP_STATS];
  int16_t             group_pull_stats_index;
  GROUPINTERVALSTATS  GroupIntervalStats[MAX_GROUP_STATS];
  int16_t             group_interval_stats_index;

  DYNAMICSESSIONSTATS DynamicSessionStats[MAX_DYNAMIC_SESSION_STATS];
  int16_t             dynamic_session_stats_index;

  uint32_t pkt_push_ctr, pkt_pull_jb_ctr, pkt_pull_xcode_ctr, pkt_pull_streamgroup_ctr, prev_pkt_push_ctr, prev_pkt_pull_jb_ctr, prev_pkt_pull_xcode_ctr, prev_pkt_pull_streamgroup_ctr;

  int8_t   flush_state[MAX_SESSIONS];
  uint32_t flush_count;

  bool     fDynamicCallMode;

  uint64_t pkt_base_timestamp[MAX_INPUT_STREAMS];
  uint64_t initial_push_time[MAX_INPUT_STREAMS];
  uint64_t total_push_time[MAX_INPUT_STREAMS];

} THREAD_INFO;


#define isMasterThread        (thread_index == 0)  /* in multithread operation, only thread 0 (the "master thread") does certain init and cleanup things, and other threads sync with the master thread and cannot proceed until those things are done */
#define MasterThread          0
#define NUM_PKTMEDIA_THREADS  3  /* default number of packet/media threads started by mediaMin */

#endif  /* _MEDIAMIN_H_ */
