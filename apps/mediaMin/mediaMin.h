/*
  mediaMin.h

  header file for mediaMin reference application including (i) simplified SigSRF push/pull interface and (ii) test and measurement program
  
  Copyright (C) Signalogic, 2018-2021

  Revision History

   Created Nov 2018 JHB, moved struct typedefs and other items out of mediaMin.c
   Modified Oct 2019 JHB, index fGroupTermCreated by number of calls to allow multiple multistream pcaps per cmd line, index fp_pcap_jb by number of sessions to allow jitter buffer output per session
   Modified Jan 2020 JHB, add nSessions, hSessions, and fDuplicatedHeaders for increased flexibility in handling multiple call / stream group cmd line inputs
   Modified Jan 2020 JHB, flush_state per-session (see comments about per-session flush in mediaThread_test_app.c), increase size of flushstr
   Modified Mar 2020 JHB, index fFirstGroupPull[], add first_packet_push_time[]
   Modified Apr 2020 JHB, add hSession to DYNAMICSESSIONSTATS struct, increase max number of dynamic session stats to allow for repeating tests with multiple cmd line inputs
   Modified Oct 2020 JHB, expand link_layer_len to int32_t to handle mods to DSOpenPcap() and DSReadPcap() (pktlib) made to support pcapng format. Note - still needs to be an int (not unint) because DSOpenPcap() returns values < 0 for error conditions
   Modified Jan 2021 JHB, for SDP input file support, add rtpmaps and a few other items (see comments)
   Modified Mar 2021 JHB, add hDerStreams[] to support DER encoded encapsulated streams
   Modified Mar 2021 JHB, add SIP invite items
   Modified Apr 2021 JHB, add include for derlib.h, move Mode constants here
*/

#ifndef _MEDIAMIN_H_
#define _MEDIAMIN_H_

#ifndef HAVE_SDP
  #include <sdp/sdp.h>  /* include sdp.h if needed */
#endif

#include "derlib.h"  /* bring in definition for HDERSTREAM (handle to a DER encapsulated stream) */

/* following are standard operating modes, stress tests, and options that can be specified by -dN cmd line entry (N may be given in hex format, for example -d0xN). Value of N is referred to in the source as "Mode" */

/* standard operating modes */

#define SESSION_CONFIG_FILE                   0  /* default mode (no -d entry), a session config file must be given on the cmd line, static sessions are created */
#define DYNAMIC_CALL                          1  /* treat each cmd line input spec ("-ixx") as a multistream call and dynamically create sessions as they appear.  If stream groups are enabled, each call has its own stream group.  If a session config file is given on the cmd line it's ignored */
#define COMBINE_CALLS                         2  /* similar to DYNAMIC_CALL, but combine all cmd line input specs into one call (and if stream groups are enabled, combine all group output into one group) */
#define ENABLE_STREAM_GROUP_DEDUPLICATION     4  /* applies a deduplication algorithm, which looks for similar content between stream group contributors and attempts to align similar streams. The objective is to reduce perceived reverb/echo due to duplicated streams. A typical scenario is a multipath (duplicated) endpoint with different latencies */
#define ENABLE_STREAM_GROUP_ASR               8  /* enable ASR processing on stream group output */
#define ENABLE_DER_STREAM_DECODE         0x1000

#define USE_PACKET_ARRIVAL_TIMES           0x10  /* use arrival times (packet timestamps) in pcap records to control push rate (can be combined with other mode flags) */

/* stress tests / functional tests (see also fStressTest and fCapacityTest flags below) */

#define CREATE_DELETE_TEST                 0x20  /* basic create / delete session stress test (automatically repeats) */
#define CREATE_DELETE_TEST_PCAP            0x40  /* create / delete session stress test using sessions found in pcap (automatically repeats) */
#define START_THREADS_FIRST                0x80  /* static sessions are created before starting packet/media thread(s) by default; set this to start threads first.  Dynamic sessions are always created after starting packet/media thread(s).  See StartPacketMediaThreads() */
#define ENERGY_SAVER_TEST                 0x100  /* enables an initial delay before pushing packets to test packet/media thread "energy saver" mode */
#define REPEAT_INPUTS                     0x200  /* repeat inputs, for example rewind pcap files when they finish. Note this mode requires 'q' key manual entry to exit */
#define ENABLE_RANDOM_WAIT              0x20000  /* enable random wait when a mediaMin application thread is repeating. This flag is normally used in stress tests */

/* operating mode options */

#define ENABLE_STREAM_GROUPS              0x400  /* enable stream groups, currently only valid with dynamic call modes. If set the first session created from each multistream pcap will contain a stream group. Currently the default stream group processing is merging and time alignment of all audio */
#define ENABLE_WAV_OUTPUT                 0x800  /* enable wav file output for stream group processing, such as audio stream merging */
#define ROUND_ROBIN_SESSION_ALLOCATION   0x4000  /* allocate sessions to packet/media threads in round-robin manner. The idea is to keep p/m thread load balanced. This flag should be specified for high capacity situations */
#define WHOLE_GROUP_THREAD_ALLOCATE      0x8000  /* do not split stream group sessions across packet/media threads. This avoids use of locks (semaphores) inside streamlib and gives higher performance */
#define ANALYTICS_MODE                  0x40000  /* enable combination of pktlib FTRT mode and ptime interval packet push/pull. This is used in analytics mode when input packets do not have wall clock timing, or they do but timing is unreliable (lawful interception is one example) */
#define ENABLE_AUTO_ADJUST_PUSH_RATE    0x80000  /* enable automatic control of push rate.  Currently supported only when ANALYTICS_MODE and DYNAMIC_CALL are also enabled */

/* disables, enables */

#define DISABLE_DTX_HANDLING           0x100000  /* DTX handling is enabled by default */
#define DISABLE_FLC                    0x200000  /* stream group output FLC is enabled by default */
#define ENABLE_ONHOLD_FLUSH_DETECT     0x400000  /* on-hold flush detection is disabled by default.  This is deprecated and has been replaced by a concept called "pastdue flush", which streamlib algorithms use to flush jitter buffers in the presence of high rates of packet loss.  If on-hold flush is enabled, packets are flushed from their jitter buffers if a stream is inactive for 0.2 sec (which is a very arbitrary amount; the pastdue approach reacts faster and more precisely) */
#define DISABLE_PACKET_REPAIR          0x800000  /* packet repair enabled by default. Missing SID and media packets (detected by sequence number and timestamp discontinuities after packet re-ordering) are repaired */
#define DISABLE_CONTRIB_PACKET_FLUSH  0x1000000  /* group contributor streams are flushed from their jitter buffer when their contribution rate becomes slow, decreasing the need for FLC on stream group combined output. Enabled by default */
#define DISABLE_AUTOQUIT              0x2000000  /* disable automatic quit for cmd lines with (i) all inputs are files (i.e. no UDP or USB audio inputs) and (ii) no repeating stress or capacity tests. Automatic quit is enabled by default */

/* alarms, debug, mem, and other extra stats */

#define ENABLE_PACKET_INPUT_ALARM       0x10000  /* enable packet input alarm -- if no packets are received by pktlib; i.e. no packets are pushed via DSPushPackets() API by an application (in this case by mediaMin) for some elapsed time, then pktlib will print a warning message in the event log */
#define ENABLE_TIMING_MARKERS        0x08000000  /* inject 1 sec wall clock timing markers in stream group output.  This can be helpful in debugging timing issues, for example the app (in this case mediaMin) is not pulling packets fast enough, or not maintaining a consistent pull interval. Additional audio marker options can be specified with uDebugMode (see examples below in DebugSetup() and LoggingSetup(), see also DEBUG_CONFIG struct definitions in shared_include/config.h) */
#define ENABLE_DEBUG_STATS           0x10000000  /* enable debug info and stats for (i) internal packet/media thread, (ii) audio merging, and (iii) DER stream decoding */
#define ENABLE_DEBUG_STATS_L2        0x20000000  /* reserved */
#define ENABLE_ALIGNMENT_MARKERS     0x40000000  /* when combined with the ENABLE_STREAM_GROUP_DEDUPLICATION flag, enables alignment markers to show the point at which streams were aligned (the deduplication algorithm uses cross correlation to align one or more streams) */
#define ENABLE_MEM_STATS             0x80000000  /* show mem usage stats in the event log */

#define MAX_APP_STR_LEN              2000

#define SESSION_MARKED_AS_DELETED    0x80000000  /* flag used to mark hSessions[] entries as deleted during dynamic call operation */

#define MAX_INPUT_REUSE              16  /* in practice, cmd line entry up to -N9 has been tested (i.e. total reuse of 10x) */

extern GLOBAL_CONFIG pktlib_gbl_cfg;

/* struct definitions used inside of THREAD_INFO definition */

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
   uint32_t ssrc;
   
} DYNAMICSESSIONSTATS;

#define MAX_DYNAMIC_SESSION_STATS  512
#define MAX_DYN_PYLD_TYPES         32

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

  uint16_t num_rtpmaps[MAX_INPUT_STREAMS];  /* SDP items, added JHB Jan2021 */
  vector<sdp::Attribute*> rtpmaps[MAX_INPUT_STREAMS];
  bool fUnmatchedPyldTypeMsg[MAX_DYN_PYLD_TYPES][MAX_INPUT_STREAMS];
  bool fDisallowedPyldTypeMsg[MAX_DYN_PYLD_TYPES][MAX_INPUT_STREAMS];

  HDERSTREAM hDerStreams[MAX_INPUT_STREAMS];  /* DER stream handles, added JHB Mar2021 */

  uint8_t* sip_save[MAX_INPUT_STREAMS];  /* SIP aggregated packet handling, initially to support SIP invite packets, added JHB Mar2021 */
  int      sip_save_len[MAX_INPUT_STREAMS];

} THREAD_INFO;


#define isMasterThread        (thread_index == 0)  /* in multithread operation, only thread 0 (the "master thread") does certain init and cleanup things, and other threads sync with the master thread and cannot proceed until those things are done */
#define MasterThread          0
#define NUM_PKTMEDIA_THREADS  3  /* default number of packet/media threads started by mediaMin */

#endif  /* _MEDIAMIN_H_ */
