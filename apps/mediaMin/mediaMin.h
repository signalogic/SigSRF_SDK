/*
  mediaMin.h

  header file for mediaMin reference application including (i) simplified SigSRF push/pull interface and (ii) test and measurement program
  
  Copyright (C) Signalogic, 2018-2025

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

  Documentation

   https://github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediamin

   Older documentation links:
  
    after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

    before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

  Revision History

   Created Nov 2018 JHB, moved struct typedefs and other items out of mediaMin.c
   Modified Oct 2019 JHB, index fGroupOwnerCreated by number of input specs to allow multiple multistream pcaps per cmd line, index fp_pcap_jb by number of sessions to allow jitter buffer output per session
   Modified Jan 2020 JHB, add nSessions, hSessions, and fDuplicatedHeaders for increased flexibility in handling multiple inputs / stream group cmd line inputs to APP_THREAD_INFO struct
   Modified Jan 2020 JHB, flush_state per-session (see comments about per-session flush in mediaThread_test_app.c), increase size of flushstr
   Modified Mar 2020 JHB, index fFirstGroupPull[], add first_pkt_time[]
   Modified Apr 2020 JHB, add hSession to STREAM_STATS struct, increase max number of dynamic session stats to allow for repeating tests with multiple cmd line inputs
   Modified Oct 2020 JHB, expand link_layer_len to int32_t to handle mods to DSOpenPcap() and DSReadPcap() (pktlib) made to support pcapng format. Note - still needs to be an int (not unint) because DSOpenPcap() returns values < 0 for error conditions
   Modified Jan 2021 JHB, for SDP info and SIP Invite message support, add rtpmaps records (a vector of rtpmaps) and a few other items (see comments) to APP_THREAD_INFO struct
   Modified Mar 2021 JHB, add hDerStreams[] to APP_THREAD_INFO struct to support DER encoded encapsulated streams
   Modified Mar 2021 JHB, add SIP invite items to APP_THREAD_INFO struct
   Modified Apr 2021 JHB, add include for derlib.h
   Modified Jan 2022 JHB, move ENABLE_xxx definitions to separate cmd_line_options_flags.h include file
   Modified Jan 2023 JHB, add Origin records (a vector of Origins) to SDP info in order to keep track of unique SDP session IDs
   Modified Jan 2023 JHB, add STREAM_TERMINATES_xxx flags
   Modified Jan 2023 JHB, add PORT_INFO_LIST struct definition
   Modified Apr 2023 JHB, add fReseek, PktInfo, and tcp_redundant_discard to APP_THREAD_INFO struct. For usage see comments in mediaMin.cpp
   Modified May 2023 JHB, add isFTRTMode and isAFAPMode macros, accel_time_ts[] to APP_THREAD_INFO struct to support "faster than real-time" and "as fast as possible" modes
   Modified Sep 2023 JHB, change PKTINFO_ITEMS reference to PKTINFO (due to change in pktlib.h)
   Modified Dec 2023 JHB, add szGroupPcap[MAX_STREAM_GROUPS] to APP_THREAD_INFO struct to support --group_pcap cmd line option
   Modified Dec 2023 JHB, include voplib.h
   Modified May 2024 JHB, add pcap_file_hdr[] to APP_THREAD_INFO struct to support .rtp format files (needed for use with modified DSReadPcapRecord() in pktlib)
   Modified Jun 2024 JHB, separate counters for UDP and RTP packet stats, fragmented packet counter, encapsulated packet counter
   Modified Jun 2024 JHB, add SDP info and SIP Invite message media descriptions to APP_THREAD_INFO struct (look for media_descriptions[])
   Modified Jun 2024 JHB, add per-stream source and destination ports to APP_THREAD_INFO struct, which are set for most recent (i) non-fragmented packet or (ii) first segment of a fragmented packet
   Modified Jun 2024 JHB, add isPortAllowed() return definitions, sip_info_checksum (use to help find SDP info duplicates)
   Modified Jul 2024 JHB, update isMasterThread() macro
   Modified Aug 2024 JHB, add uOneTimeConsoleQuitMessage item to APP_THREAD_INFO struct, add STR_APPEND definition
   Modified Sep 2024 JHB, rename nPcapOutFiles to nOutFiles to include pcap, bitstream, and other format files
   Modified Sep 2024 JHB, add uInputType[], uOutputType[], and nSessionOutputIndex[] to APP_THREAD_INFO struct
   Modified Oct 2024 JHB, define INPUT_DATA_CACHE struct, add array of pointers to it in APP_THREAD_INFO struct. Rename fReseek to uCacheFlags and define CACHE_XXX flags
   Modified Nov 2024 JHB, rename DYNAMICSESSIONSTATS to STREAM_STATS and add stats support for static sessions. See StreamStats[] and StreamStatsState[] below; also see session_app.cpp for related changes
   Modified Jan 2025 JHB, add num_rtcp_packets[] and num_unhandled_rtp_packets[] to APP_THREAD_INFO struct
   Modified Feb 2025 JHB, rename pcap_out[] to out_file[], rename link_layer_len[] to link_layer_info[]
   Modified Feb 2025 JHB, add fmtp to SDP info database structs. A key use case is handling sprop-vps, -sps, and -pps fields for video streams
   Modified Mar 2025 JHB, add most_recent_console_output to thread_info[]; see comments in mediaMin.cpp
*/

#ifndef _MEDIAMIN_H_
#define _MEDIAMIN_H_

#include "cmd_line_options_flags.h"  /* cmd line options and flags definitions. cmd_line_options_flags.h is on mediaTest subfolder, JHB Jan 2022 */
#include "voplib.h"                  /* voice and video over packet library */

#ifndef HAVE_SDP
  #include <sdp/sdp.h>  /* include sdp.h if needed */
#endif

#include "derlib.h"  /* bring in definition for HDERSTREAM (handle to a DER encapsulated stream) */

/* stream and session notes

  1) No session related arrays are indexed directly by session handles, which can be quite large values. Instead mediaMin maintains an hSessions[] array that maps session indexes to handles. This both establishes realistic for per-thread performance limits and reduces memory requirements

  2) Each thread maintains stream / session mapping arrays

    map_session_index_to_stream[], which is many:1, i.e. each session maps to one stream. Given a session index, this array returns its stream
    map_stream_to_session_indexes[][], which is 1:many, i.e. one stream may have many sessions. Given a stream, this array returns a list of session indexes

  3) Input stream indexes are 0 based and generally correspond to command line input spec indexes (e.g. pcaps and UDP port ranges) but not always. cmd_line_input_index[] will safely return a cmd line input index from a stream index

  4) Output stream indexes are based on session functionality, for example output streams are created due to (i) stream groups (unified audio conversations) are formed, (ii) call/stream recordings, (iii) output specs on the cmd line

  5) MAX_SESSIONS_THREAD and MAX_STREAMS_THREAD are defined here as per-thread limits. These are not be confused with MAX_SESSIONS and MAX_STREAMS which are defined in shared_include/transcoding.h and shared_include/streamlib.h and used by packet/media thread workers (pktlib)
*/

#define MAX_STREAMS_THREAD                64  /* maximum number of streams per thread */
#define MAX_SESSIONS_THREAD               64  /* maximum number of sessions per thread */

#define MAX_APP_STR_LEN                   2000
#define STR_APPEND                        1

#define SESSION_MARKED_AS_DELETED         0x80000000L  /* private mediaMin flag used to mark hSessions[] entries as deleted during dynamic session operation */

#define MAX_INPUT_REUSE                   16  /* in practice, cmd line entry up to -N9 has been tested (i.e. total reuse of 10x) */

/* dynamic stream terminations */

#define STREAM_TERMINATES_ON_BYE_MESSAGE  1
#define STREAM_TERMINATES_ON_PORT_CLOSE   2
#define STREAM_TERMINATES_NO_SESSIONS     0x10

/* definitions returned by isPortAllowed(). Look for nAllowedPortStatus in mediaMin.cpp */

#define PORT_ALLOW_UNKNOWN                0
#define PORT_ALLOW_KNOWN                  1
#define PORT_ALLOW_ON_MEDIA_ALLOW_LIST    2
#define PORT_ALLOW_SDP_MEDIA_DISCOVERED   3
#define PORT_ALLOW_SDP_INFO               4


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
   
} GROUP_PULL_STATS;

typedef struct {

   uint32_t missed_interval;
   uint16_t repeats;
   HSESSION hSession;
   
} GROUP_INTERVAL_STATS;

#define MAX_GROUP_STATS  512

typedef struct {

   HSESSION hSession;
   uint8_t  term;                           /* session termination: 0 or 1 */
   int32_t  chnum;                          /* channel */
   uint8_t  uFlags;                         /* type of session (dynamic vs static), other flags as needed */
   char     codec_name[CODEC_NAME_MAXLEN];  /* CODEC_NAME_MAXLEN defined in voplib.h */
   uint16_t bitrate;
   uint8_t  payload_type;
   uint64_t first_pkt_usec;                 /* arrival time of first packet, in usec */
   uint32_t first_pkt_ssrc;                 /* first packet RTP SSRC */
   
} STREAM_STATS;

/* STREAM_STATS uFlags */

#define STATIC_SESSION       0  /* default, set in CreateStaticSessions() in session_app.cpp */
#define DYNAMIC_SESSION      1  /* dynamic session, set in CreateDynamicSession() in mediaMin.cpp */

#define MAX_DYN_PYLD_TYPES  32  /* max number of disallowed payload type messsages (fDisallowedPyldTypeMsg) */

typedef struct {

   uint16_t port;

} PORT_INFO_LIST;

typedef struct {  /* input data cache read items, JHB Oct 2024 */

  uint16_t       hdr_type;
  pcaprec_hdr_t  pcap_rec_hdr;
  int            pkt_len;
  uint8_t        pkt_buf[MAX_TCP_PACKET_LEN];

} INPUT_DATA_CACHE;

/* definitions for uCacheFlags field in APP_THREAD_INFO struct */

#define CACHE_INVALID       0  /* indicate to GetInputData() that input cache contains stale or outdated data */
#define CACHE_READ          1  /* indicate to GetInputData() that current packet data is still being processed and should be read from input cache, examples include (i) packet arrival timestamp not yet elapsed and (ii) a TCP packet being consumed in segments */
#define CACHE_READ_PKTBUF   2  /* same as CACHE_READ but indicates pktbuf is no longer valid due to in-place processing and should also be read from cache */

#define CACHE_NEW_DATA   0x10  /* set by GetInputData(), indicates input cache has been updated with new data */

#define CACHE_ITEM_MASK  0x0f  /* mask to isolate flags that instruct GetInputData() */


/* APP_THREAD_INFO defines per-thread application vars and structs. If mediaMin is run from the cmd line then there is just one application thread, if mediaMin is run from mediaTest with -Et command line entry, then -tN entry determines how many application threads */

typedef struct {

  int                   nSessionsCreated;
  int                   nSessionsDeleted;
  int                   nDynamicSessions;
  uint32_t              total_sessions_created;

  int16_t               nInPcapFiles;
  int16_t               nOutFiles;  /* output pcap or bitstream files */

  int32_t               link_layer_info[MAX_STREAMS_THREAD];
  FILE*                 pcap_in[MAX_STREAMS_THREAD];
  uint16_t              cmd_line_input_index[MAX_STREAMS_THREAD];
  pcap_hdr_t*           pcap_file_hdr[MAX_STREAMS_THREAD];  /* used in DSOpenPcap() and DSOpenPcapRecord(), JHB May 2024 */
  uint8_t               uInputType[MAX_STREAMS_THREAD];
  INPUT_DATA_CACHE*     input_data_cache[MAX_STREAMS_THREAD];  /* per-stream input data read cache, JHB Oct 2024 */

  FILE*                 out_file[MAX_STREAMS_THREAD];
  uint8_t               uOutputType[MAX_STREAMS_THREAD];

  int                   nSessions[MAX_STREAMS_THREAD];  /* thread's current number of sessions */

  int                   map_session_index_to_stream[MAX_SESSIONS_THREAD];  /* many:1 mapping, for a given session index, retrieve its stream. See "stream and session notes" */
  int                   map_stream_to_session_indexes[MAX_STREAMS_THREAD][MAX_SESSIONS_THREAD];  /* 1:many mapping: for a given stream, retrieve a list of session indexes */

  int                   nSessionOutputStream[MAX_SESSIONS_THREAD];
  bool                  fDuplicatedHeaders[MAX_STREAMS_THREAD];
  FILE*                 fp_pcap_jb[MAX_SESSIONS_THREAD];
  bool                  init_err;

/* packet stats */

  uint32_t              packet_number[MAX_STREAMS_THREAD];  /* for pcaps this stat matches "packet number" in Wireshark displays. It also serves as a counter for total number of packets per stream */
  uint32_t              num_tcp_packets[MAX_STREAMS_THREAD];
  uint32_t              num_udp_packets[MAX_STREAMS_THREAD];
  uint32_t              num_packets_encapsulated[MAX_STREAMS_THREAD];
  uint32_t              num_rtp_packets[MAX_STREAMS_THREAD];
  uint32_t              num_rtcp_packets[MAX_STREAMS_THREAD];
  uint32_t              num_unhandled_rtp_packets[MAX_STREAMS_THREAD];

  uint32_t              num_packets_fragmented[MAX_STREAMS_THREAD];
  uint32_t              num_packets_reassembled[MAX_STREAMS_THREAD];

  FILE*                 fp_pcap_group[MAX_STREAM_GROUPS];  /* note - this array is accessed by a session counter, and each app thread might handle up to 50 sessions, so this size (172, defined in shared_include/streamlib.h) is overkill.  But leave it for now */
  FILE*                 fp_text_group[MAX_STREAM_GROUPS];
  char                  szGroupPcap[MAX_STREAM_GROUPS][CMDOPT_MAX_INPUT_LEN];  /* added to support --group_pcap cmd line option, JHB Dec 2023 */
  char                  szGroupName[MAX_STREAM_GROUPS][MAX_GROUPID_LEN];
  bool                  fGroupOwnerCreated[MAX_STREAM_GROUPS][MAX_INPUT_REUSE];  /* used in dynamic session mode */

  bool                  fFirstGroupPull[MAX_STREAM_GROUPS];
  GROUP_PULL_STATS      GroupPullStats[MAX_GROUP_STATS];
  int16_t               group_pull_stats_index;
  GROUP_INTERVAL_STATS  GroupIntervalStats[MAX_GROUP_STATS];
  int16_t               group_interval_stats_index;

/* stream stats */

  uint32_t              uStreamStatsState[NCORECHAN];
  #define               STREAM_STATE_FIRST_PKT  0x10000000
  #define               STREAM_STATE_FLAG_MASK  0xf0000000
  STREAM_STATS          StreamStats[MAX_STREAMS_THREAD];
  int16_t               stream_stats_index;

  uint32_t              pkt_push_ctr, pkt_pull_jb_ctr, pkt_pull_xcode_ctr, pkt_pull_streamgroup_ctr, prev_pkt_push_ctr, prev_pkt_pull_jb_ctr, prev_pkt_pull_xcode_ctr, prev_pkt_pull_streamgroup_ctr;  /* referenced in UpdateCounters() console update in user_io.cpp */

  int8_t                flush_state[MAX_SESSIONS_THREAD];
  uint32_t              flush_count;

  bool                  fDynamicSessions;

  uint64_t              pkt_base_timestamp[MAX_STREAMS_THREAD];  /* arrival timestamp of first packet in a stream, in msec */
  uint64_t              first_pkt_time[MAX_STREAMS_THREAD];      /* time of first packet processed to be in a stream, in msec */
  uint64_t              total_pkt_time[MAX_STREAMS_THREAD];      /* total time of packet processing in a stream */

/* SDP info and SIP invite message items, added JHB Jan 2021 */

  uint16_t                 num_rtpmaps[MAX_STREAMS_THREAD];
  vector<sdp::Attribute*>  rtpmaps[MAX_STREAMS_THREAD];
  uint16_t                 num_origins[MAX_STREAMS_THREAD];
  vector<sdp::Origin*>     origins[MAX_STREAMS_THREAD];
  uint16_t                 num_media_descriptions[MAX_STREAMS_THREAD];  /* add media descriptions, JHB Jun 2024 */
  vector<sdp::Media*>      media_descriptions[MAX_STREAMS_THREAD];
  uint16_t                 num_fmtps[MAX_STREAMS_THREAD];
  vector<sdp::Attribute*>  fmtps[MAX_STREAMS_THREAD];

/* misc stream strangenesses we can keep track of */

  uint32_t              uNoDataFrame[MAX_STREAMS_THREAD];
  bool                  fUnmatchedPyldTypeMsg[MAX_DYN_PYLD_TYPES][MAX_STREAMS_THREAD];
  bool                  fDisallowedPyldTypeMsg[MAX_DYN_PYLD_TYPES][MAX_STREAMS_THREAD];

  uint8_t               dynamic_terminate_stream[MAX_STREAMS_THREAD];  /* non-zero values will terminate a stream, for example a SIP BYE messsage from sender or recipient with same IP addr as active media stream.  See STREAM_TERMINATES_xxx defines above */

/* SIP aggregated packet handling, supports SIP messages, SIP invite, SAP protocol, and other SDP info packets, added JHB Mar 2021 */
  
  uint8_t*              sip_info_save[MAX_STREAMS_THREAD];
  int                   sip_info_save_len[MAX_STREAMS_THREAD];
  int32_t               sip_info_crc32[MAX_STREAMS_THREAD];

/* LI HI2/HI3 items */

  HDERSTREAM            hDerStreams[MAX_STREAMS_THREAD];  /* DER stream handles, added JHB Mar 2021 */
  FILE*                 hFile_ASN_XML[MAX_STREAMS_THREAD];  /* DER stream XML output file handles, JHB Dec 2022 */

 /* items used in GetInputData() and PushPackets() */

  uint8_t               uCacheFlags[MAX_STREAMS_THREAD];             /* input cache flags controlling operation of GetInputData() */
  PKTINFO               PktInfo[MAX_STREAMS_THREAD];                 /* saved copy of PktInfo, can be used to compare current and previous packets */ 
  unsigned int          tcp_redundant_discards[MAX_STREAMS_THREAD];  /* count of discarded TCP redundant retransmissions */
  unsigned int          udp_redundant_discards[MAX_STREAMS_THREAD];  /* count of discarded UDP redundant retransmissions, JHB Jun 2024 */

/* packet fragmentation items, JHB Jun 2024 */

  uint16_t              dst_port[MAX_STREAMS_THREAD];                /* ports are saved when MF flag is set and fragment offset is not */
  uint16_t              src_port[MAX_STREAMS_THREAD];
  
/* AFAP and FTRT mode support */

  struct timespec       accel_time_ts[MAX_STREAM_GROUPS];  /* stream group accerated timestamps, added to support FTRT and AFAP modes, JHB May 2023 */

/* console output (auto quit, etc) */

  uint64_t              uOneTimeConsoleQuitMessage;
  uint64_t              most_recent_console_output;  /* most recent console output, in usec. See cur_time in mediaMin.cpp */ 

/* per-thread arrival timing stats */

  float                 arrival_avg_delta[MAX_SESSIONS_THREAD];
  float                 arrival_avg_delta_clock[MAX_SESSIONS_THREAD];
  float                 arrival_avg_jitter[MAX_SESSIONS_THREAD];
  float                 arrival_max_delta[MAX_SESSIONS_THREAD];
  float                 arrival_max_jitter[MAX_SESSIONS_THREAD];

  float                 last_msec_timestamp[MAX_SESSIONS_THREAD];
  int                   last_rtp_pyld_len[MAX_SESSIONS_THREAD];

  //# define RTP_TIMESTAMP_STATS  /* define for RTP timestamp stats and debug, and then do make clean; make all. Not normally used as timestamps are unlikely to be in correct order until processing by pktlib jitter buffer, JHB Mar 2025 */
  #ifdef RTP_TIMESTAMP_STATS
  float                 rtp_timestamp_avg_delta[MAX_SESSIONS_THREAD];
  uint32_t              last_rtp_timestamp[MAX_SESSIONS_THREAD];
  #endif

  uint32_t              num_arrival_stats_pkts[MAX_SESSIONS_THREAD];

} APP_THREAD_INFO;

/* helper definitions */

#define isMasterThread(thread_index)  (thread_index == 0)  /* in multithread operation, only thread 0 (the "master thread") does certain init and cleanup things, and other threads sync with the master thread and cannot proceed until those things are done */
#define MasterThread                  0
#define NUM_PKTMEDIA_THREADS          3  /* typically mediaMin starts one packet/media thread. Given enough cmd line input specs it may start up to NUM_PKTMEDIA_THREADS packet/media threads */

#define isAFAPMode                    (RealTimeInterval[0] == 0)  /* macro for "as fast as possible" processing mode, evaluates as true for -r0 cmd line entry, JHB May 2023 */
#define isFTRTMode                    (RealTimeInterval[0] > 0 && RealTimeInterval[0] < 1)  /* macro for "faster than real-time" processing mode, evaluates as true for -rN cmd line entry where 0 < N < 1, JHB May 2023 */

#endif  /* _MEDIAMIN_H_ */
