/*
  mediaMin.h

  header file for mediaMin reference application including (i) simplified SigSRF push/pull interface and (ii) test and measurement program
  
  Copyright (C) Signalogic, 2018-2024

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

  Documentation

   https://github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediamin

   Older documentation links:
  
    after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

    before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

  Revision History

   Created Nov 2018 JHB, moved struct typedefs and other items out of mediaMin.c
   Modified Oct 2019 JHB, index fGroupTermCreated by number of input specs to allow multiple multistream pcaps per cmd line, index fp_pcap_jb by number of sessions to allow jitter buffer output per session
   Modified Jan 2020 JHB, add nSessions, hSessions, and fDuplicatedHeaders for increased flexibility in handling multiple inputs / stream group cmd line inputs
   Modified Jan 2020 JHB, flush_state per-session (see comments about per-session flush in mediaThread_test_app.c), increase size of flushstr
   Modified Mar 2020 JHB, index fFirstGroupPull[], add first_packet_push_time[]
   Modified Apr 2020 JHB, add hSession to DYNAMICSESSIONSTATS struct, increase max number of dynamic session stats to allow for repeating tests with multiple cmd line inputs
   Modified Oct 2020 JHB, expand link_layer_len to int32_t to handle mods to DSOpenPcap() and DSReadPcap() (pktlib) made to support pcapng format. Note - still needs to be an int (not unint) because DSOpenPcap() returns values < 0 for error conditions
   Modified Jan 2021 JHB, for SDP info and SIP Invite message support, add rtpmaps records (a vector of rtpmaps) and a few other items (see comments)
   Modified Mar 2021 JHB, add hDerStreams[] to support DER encoded encapsulated streams
   Modified Mar 2021 JHB, add SIP invite items
   Modified Apr 2021 JHB, add include for derlib.h
   Modified Jan 2022 JHB, move ENABLE_xxx definitions to separate cmd_line_options_flags.h include file
   Modified Jan 2023 JHB, add Origin records (a vector of Origins) to SDP info in order to keep track of unique SDP session IDs
   Modified Jan 2023 JHB, add STREAM_TERMINATE_xxx flags
   Modified Jan 2023 JHB, add PORT_INFO_LIST struct definition
   Modified Apr 2023 JHB, add fReseek, PktInfo, and tcp_redundant_discard. For usage see comments in mediaMin.cpp
   Modified May 2023 JHB, add isFTRTMode and isAFAPMode macros, accel_time_ts[] to support "faster than real-time" and "as fast as possible" modes
   Modified Sep 2023 JHB, change PKTINFO_ITEMS reference to PKTINFO (due to change in pktlib.h)
   Modified Dec 2023 JHB, add szGroupPcap[MAX_STREAM_GROUPS] to support --group_pcap cmd line option
   Modified Dec 2023 JHB, include voplib.h
   Modified May 2024 JHB, add pcap_file_hdr[] to support .rtp format files (needed for use with modified DSReadPcapRecord() in pktlib)
   Modified Jun 2024 JHB, separate counters for UDP and RTP packet stats, fragmented packet counter, encapsulated packet counter
   Modified Jun 2024 JHB, add SDP info and SIP Invite message media descriptions, look for media_descriptions[]
   Modified Jun 2024 JHB, add per-stream source and destination ports, which are set for most recent (i) non-fragmented packet or (ii) first segment of a fragmented packet
   Modified Jun 2024 JHB, add isPortAllowed() return definitions, sip_info_checksum (use to help find SDP info duplicates)
*/

#ifndef _MEDIAMIN_H_
#define _MEDIAMIN_H_

#include "cmd_line_options_flags.h"  /* cmd line options and flags definitions. cmd_line_options_flags.h is on mediaTest subfolder, JHB Jan 2022 */
#include "voplib.h"                  /* MAX_SESSIONS definition */

#ifndef HAVE_SDP
  #include <sdp/sdp.h>  /* include sdp.h if needed */
#endif

#include "derlib.h"  /* bring in definition for HDERSTREAM (handle to a DER encapsulated stream) */

#define MAX_APP_STR_LEN               2000

#define SESSION_MARKED_AS_DELETED     0x80000000  /* private mediaMin flag used to mark hSessions[] entries as deleted during dynamic session operation */

#define MAX_INPUT_REUSE               16  /* in practice, cmd line entry up to -N9 has been tested (i.e. total reuse of 10x) */

/* dynamic stream terminations */

#define STREAM_TERMINATE_BYE_MESSAGE             1
#define STREAM_TERMINATE_PORT_CLOSES             2
#define STREAM_TERMINATE_INPUT_ENDS_NO_SESSIONS  0x10

/* definitions returned by isPortAllowed(). Look for nAllowedPortStatus in mediaMin.cpp */

#define PORT_ALLOW_UNKNOWN               0
#define PORT_ALLOW_KNOWN                 1
#define PORT_ALLOW_ON_MEDIA_ALLOW_LIST   2
#define PORT_ALLOW_SDP_MEDIA_DISCOVERED  3
#define PORT_ALLOW_SDP_INFO              4


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

typedef struct {

   uint16_t port;

} PORT_INFO_LIST;

/* APP_THREAD_INFO defines per-thread application vars and structs. If mediaMin is run from the cmd line then there is just one application thread, if mediaMin is run from mediaTest with -Et command line entry, then -tN entry determines how many application threads */

typedef struct {

  int          nSessionsCreated;
  int          nSessionsDeleted;
  int          nDynamicSessions;
  uint32_t     total_sessions_created;

  int16_t      nInPcapFiles;
  int16_t      nOutPcapFiles;

  int32_t      link_layer_len[MAX_INPUT_STREAMS];  /* note: the current definition of MAX_INPUT_STREAMS (512, in mediaTest.h) is overkill.  Something like 10 to 50 pcaps, with say up to 10 streams (sessions) each, is more realistic */
  FILE*        pcap_in[MAX_INPUT_STREAMS];
  uint16_t     input_index[MAX_INPUT_STREAMS];
  pcap_hdr_t*  pcap_file_hdr[MAX_INPUT_STREAMS];  /* used in DSOpenPcap() and DSOpenPcapRecord(), JHB May 2024 */

  FILE*        pcap_out[MAX_INPUT_STREAMS];

  int          nSessions[MAX_INPUT_STREAMS];
  int          nSessionIndex[MAX_INPUT_STREAMS][MAX_SESSIONS];
  bool         fDuplicatedHeaders[MAX_INPUT_STREAMS];
  FILE*        fp_pcap_jb[MAX_SESSIONS];
  bool         init_err;

/* packet stats */

  uint32_t     packet_number[MAX_INPUT_STREAMS];  /* for pcaps this stat matches "packet number" in Wireshark displays. It also serves as a counter for total number of packets per stream */
  uint32_t     num_tcp_packets[MAX_INPUT_STREAMS];
  uint32_t     num_udp_packets[MAX_INPUT_STREAMS];
  uint32_t     num_packets_encapsulated[MAX_INPUT_STREAMS];
  uint32_t     num_rtp_packets[MAX_INPUT_STREAMS];

  uint32_t     num_packets_fragmented[MAX_INPUT_STREAMS];
  uint32_t     num_packets_reassembled[MAX_INPUT_STREAMS];

  FILE*        fp_pcap_group[MAX_STREAM_GROUPS];  /* note:  this array is accessed by a session counter, and each app thread might handle up to 50 sessions, so this size (172, defined in shared_include/streamlib.h) is overkill.  But leave it for now */
  FILE*        fp_text_group[MAX_STREAM_GROUPS];
  char         szGroupPcap[MAX_STREAM_GROUPS][CMDOPT_MAX_INPUT_LEN];  /* added to support --group_pcap cmd line option, JHB Dec 2023 */
  char         szGroupName[MAX_STREAM_GROUPS][MAX_GROUPID_LEN];
  bool         fGroupTermCreated[MAX_STREAM_GROUPS][MAX_INPUT_REUSE];  /* used in dynamic session mode */

  bool                fFirstGroupPull[MAX_STREAM_GROUPS];
  GROUPPULLSTATS      GroupPullStats[MAX_GROUP_STATS];
  int16_t             group_pull_stats_index;
  GROUPINTERVALSTATS  GroupIntervalStats[MAX_GROUP_STATS];
  int16_t             group_interval_stats_index;

  DYNAMICSESSIONSTATS DynamicSessionStats[MAX_DYNAMIC_SESSION_STATS];
  int16_t             dynamic_session_stats_index;

  uint32_t     pkt_push_ctr, pkt_pull_jb_ctr, pkt_pull_xcode_ctr, pkt_pull_streamgroup_ctr, prev_pkt_push_ctr, prev_pkt_pull_jb_ctr, prev_pkt_pull_xcode_ctr, prev_pkt_pull_streamgroup_ctr;

  int8_t       flush_state[MAX_SESSIONS];
  uint32_t     flush_count;

  bool         fDynamicSessions;

  uint64_t     pkt_base_timestamp[MAX_INPUT_STREAMS];
  uint64_t     initial_push_time[MAX_INPUT_STREAMS];
  uint64_t     total_push_time[MAX_INPUT_STREAMS];

/* SDP info and SIP invite message items, added JHB Jan 2021 */

  uint16_t                num_rtpmaps[MAX_INPUT_STREAMS];
  vector<sdp::Attribute*> rtpmaps[MAX_INPUT_STREAMS];
  uint16_t                num_origins[MAX_INPUT_STREAMS];
  vector<sdp::Origin*>    origins[MAX_INPUT_STREAMS];
  uint16_t                num_media_descriptions[MAX_INPUT_STREAMS];  /* add media descriptions, JHB Jun 2024 */
  vector<sdp::Media*>     media_descriptions[MAX_INPUT_STREAMS];

  bool         fUnmatchedPyldTypeMsg[MAX_DYN_PYLD_TYPES][MAX_INPUT_STREAMS];
  bool         fDisallowedPyldTypeMsg[MAX_DYN_PYLD_TYPES][MAX_INPUT_STREAMS];

  uint8_t      dynamic_terminate_stream[MAX_INPUT_STREAMS];  /* non-zero values will terminate a stream, for example a SIP BYE messsage from sender or recipient with same IP addr as active media stream.  See STREAM_TERMINATE_xxx defines above */

/* SIP aggregated packet handling, supports SIP messages, SIP invite, SAP protocol, and other SDP info packets, added JHB Mar 2021 */
  
  uint8_t*     sip_info_save[MAX_INPUT_STREAMS];
  int          sip_info_save_len[MAX_INPUT_STREAMS];
  int32_t      sip_info_crc32[MAX_INPUT_STREAMS];

/* LI HI2/HI3 items */

  HDERSTREAM   hDerStreams[MAX_INPUT_STREAMS];  /* DER stream handles, added JHB Mar 2021 */
  FILE*        hFile_ASN_XML[MAX_INPUT_STREAMS];  /* DER stream XML output file handles, JHB Dec 2022 */

 /* items used in PushPackets() */

  bool         fReseek[MAX_INPUT_STREAMS];                 /* flag indicating whether current packet processing is a "reseek" (i) a packet arrival timestamp not yet elapsed (ii) a TCP packet being consumed in segments */
  PKTINFO      PktInfo[MAX_INPUT_STREAMS];                 /* saved copy of PktInfo, can be used to compare current and previous packets */ 
  unsigned int tcp_redundant_discards[MAX_INPUT_STREAMS];  /* count of discarded TCP redundant retransmissions */
  unsigned int udp_redundant_discards[MAX_INPUT_STREAMS];  /* count of discarded UDP redundant retransmissions, JHB Jun 2024 */

/* packet fragmentation items, JHB Jun 2024 */

  uint16_t     dst_port[MAX_INPUT_STREAMS];                /* ports are saved when MF flag is set and fragment offset is not */
  uint16_t     src_port[MAX_INPUT_STREAMS];
  
/* AFAP and FTRT mode support */

  struct timespec  accel_time_ts[MAX_STREAM_GROUPS];  /* added to support FTRT and AFAP modes, JHB May 2023 */

} APP_THREAD_INFO;

/* helper definitions */

#define isMasterThread        (thread_index == 0)  /* in multithread operation, only thread 0 (the "master thread") does certain init and cleanup things, and other threads sync with the master thread and cannot proceed until those things are done */
#define MasterThread          0
#define NUM_PKTMEDIA_THREADS  3  /* typically mediaMin starts one packet/media thread. Given enough cmd line input specs it may start up to NUM_PKTMEDIA_THREADS packet/media threads */

#define isAFAPMode            (RealTimeInterval[0] == 0)  /* macro for "as fast as possible" processing mode, evaluates as true for -r0 cmd line entry, JHB May 2023 */
#define isFTRTMode            (RealTimeInterval[0] > 0 && RealTimeInterval[0] < 1)  /* macro for "faster than real-time" processing mode, evaluates as true for -rN cmd line entry where 0 < N < 1, JHB May 2023 */

#endif  /* _MEDIAMIN_H_ */
