/*
  session.h

  Voice / video session support for c66x, x86, or combined coCPU platforms
    1) Per-channel definitions.  Each channel (stream), has local and remote endpoints (or ingress/Rx and egress/Tx endpoints)
    2) Session, termination, and voice / video attributes struct definitions

  Copyright (C) Signalogic, Inc, 2008-2012

    Created for c64x voice platforms

  Copyright (C) Mavenir Systems and Signalogic, Inc, 2013-2015
  
    Support for c66x coCPU card PCIe and ATCA blade SRIO interfaces

  Copyright (C) Signalogic, Inc, 2016-2023

    Add APIs to (i) encapsulate c66x PCIe and SRIO interfaces, and (ii) support x86-only or combined x86 and c66x server usage.  APIs are consistent between all use cases

  License
  
    Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

  Revision History

    Nov 2008, Created, JHB, session (voice/video) support for SigC66xx PCIe cards and SigC641x PMC/PTMC modules

    Mar 2009, Modified, JHB
      -add call statistics

    Apr 2009, Modified, XC
      -modified call statistics to support PTT features

    May 2009, Modified, XC
      - modified CHANINFO_HOST, moved per-channel Media related attributes from being separately defined
      - added support for internal DSP channels, not tied to specific tdm or IP channel

    Feb 2010, Modified, JHB
       -add encryption support

    Jun-Sep 2013, Modified, CJ, JHB
      -add c66x support
      -add FS conversion support
      -add dynamic jitter buffer support
      -add DPDK host interface support

    Sep 2013
      -call status info separated into call.h, JHB
      -verify stdint.h for TI C6x compilers, JHB

    Jun 2014
      -add supports for EVRC

    Aug 2014
      -add supports for OPUS

    Jan 2015
      -change elements of size less than 32-bits to use bitfields
      -add support for BIG_ENDIAN

    Sep 2015
      -add support for EVS

    Dec 2016 - Jan 2017, JHB
      -add APIs for configuration, session create/delete

    Apr 2017, JHB
      -rename HOST_TERM_INFO to TERMINATION_INFO and HOST_SESSION_DATA to SESSION_DATA.  Under coCPU architecture there is no differentiation between host and embedded target.  Host now refers only to VM host / guest

    Jul 2017, JHB
      -add "mode" field to TERMINATION_INFO struct.  The first mode flag handled is TERMINATION_MODE_IP_PORT_DONTCARE, which allows session config termN remote/local IP addr:port values to be treated as "don't care" 

    Mar 2018, JHB
      -add codec definition for MELPe

    Apr 2018, CJ
      -add MELPe params

    Jun 2018 CJ
      -add merge and deduplication fields to TERMINATION_INFO

    Nov 2018 JHB
      -add thread level session items, including SESSION_INFO_THREAD, to support (i) multithreading and (ii) streamlib.so (stream groups)
      -add output_bufer_interval to TERMINATION_INFO struct
      -change "merge" references to "group" where it doesn't necessarily involve merging, including "group_term", "group_mode", and "group_status" in TERMINATION_INFO and SESSION_DATA structs, and DS_GROUP_OWNER, DS_GROUP_CONTRIBUTOR enums.  In preparation for other stream group operations, including speech recognition.  See also comments in streamlib.h

    Feb 2019 JHB
     -enable enums and constants inside "USE_ATCA_EVS_MODS" for x86 platforms, add "evs_sample_rate" enums
     -added "input-sample-rate" to TERMINATION_INFO struct, to enable differentiation of input and output side of codecs that allow independent input and decode sample rates (so far this is only EVS and Opus)

    Jun 2019 JHB
     -rename .dtmf to .dtmf_mode in voice_attributes() struct

    Jan 2020 JHB
      -add max_pkt_repair_ptimes and TERM_PKT_REPAIR_ENABLE and TERM_OVERRUN_SYNC_ENABLE flags -- correct media packet loss when possible

    Mar 2020 JHB
      -remove fDTXEnabled and fSIDRepairEnabled items from SESSION_INFO_THREAD struct, remove "deduplicate" items
      -define TERM_EXPECT_BIDIRECTIONAL_TRAFFIC flag, which applications can set for telecom mode applications. If not set, packet/media thread receive queue handling performance is increased for unidirectional traffic (analytics mode)

    Apr 2020 JHB
      -add max_depth_ptimes to JITTER_BUFFER_CONFIG struct

    May 2020 JHB
      -define TERM_IGNORE_ARRIVAL_PACKET_TIMING flag for situations when packet arrival timing is not accurate, for example pcaps without packet arrival timestamps, analytics mode sending packets faster than real-time, etc

    Jan 2021 JHB
      -re-define merge_audio_chunk_size to stream_group_buffer_time in SESSION_THREAD_INFO struct. See comments

    Sep 2022 JHB
      -add TERM_DISABLE_DORMANT_SESSION_DETECTION flag and dormant_SSRC_wait_time item to TERMINATION_INFO struct. See comments
      -add "payload_shift" shift amount and filter flags in TERMINATION_INFO struct. CODEC_PAYLOAD_SHIFT_XXX flag definitions are in codec.h. See comments
      
    Jan 2023 JHB
      -add TERM_DYNAMIC_SESSION flag definition (applies to uFlags element in TERMINATION_INFO struct)
      
    May 2023 JHB
      -make input_buffer_interval in TERMINATION_INFO struct float
      -add TERM_ANALYTICS_MODE_PACKET_TIMING flag definition (applies to uFlags element in TERMINATION_INFO struct)
      -add TERM_NO_PACKET_TIMING flag definition (applies to uFlags element in TERMINATION_INFO struct)
      -add RFC7198_lookback uint8_t in TERMINATION_INFO struct

    Aug 2023 JHB
      -define TERMINFO "nickname" for TERMINATION_INFO
*/

#ifndef _SESSION_H_
#define _SESSION_H_

#include <stdint.h>

#define DYNAMIC_JITTER_ENABLE

#ifdef NPLUS1_BUILD
   #define  MAX_REDUNDANCY 13
#elif ONEPLUS1_BUILD
   #define  MAX_REDUNDANCY 1
#else
  #define  MAX_REDUNDANCY 0
#endif

#define USE_BIT8FIELDS

#define ENABLE_TERM_MODE_FIELD  /* termination mode field added in TERMINATION_INFO struct, JHB Jul2017 */
/*#define ENABLE_TERM_MODE_DONT_CARE */

                                /*  Notes:

                                    1) Currently the mode field is processed but no mode values are being used.  Functionality originally provided by the "dont care" mode value has been replaced by a more flexible
                                       "user managed" session operating mode.  See comments on this in pktlib.c and x86_mediaTest.c

                                    2) For reference, the "don't care" mode allows termN session config remote/local IP addr:port values to be designated as "don't care", for example term2 IP addr:port is a don't
                                       care because the user app is listening only, with no receive and transmit on term2 side
                                */

#include "codec.h"  /* pull in codec related items, JHB Oct 2023 */

struct voice_attributes {

#ifdef __BIG_ENDIAN__
  uint32_t flag : 8;               /* see VOICE_ATTR_FLAG_VAD and VOICE_ATTR_FLAG_CNG */
  uint32_t noise_reduction : 8;    /* noise reduction:  0 = none, 1-5 = type */
  uint32_t ec : 8;                 /* see ec_type enum */
  uint32_t ec_tail_len : 8;        /* tail length:  0 = not used, otherwise specified in msec */
#else
  uint32_t ec_tail_len : 8;        /* tail length:  0 = not used, otherwise specified in msec */
  uint32_t ec : 8;                 /* see ec_type enum */
  uint32_t noise_reduction : 8;    /* noise reduction:  0 = none, 1-5 = type */
  uint32_t flag : 8;               /* see VOICE_ATTR_FLAG_VAD and VOICE_ATTR_FLAG_CNG */
#endif

#ifdef __BIG_ENDIAN__
  uint32_t dtmf_payload_type : 8;  /* DTMF payload type to use in remote (egress) RTP header */
  uint32_t dtmf_mode : 8;          /* Refer to dtmf_processing enum */
  uint32_t rtp_payload_type : 8;   /* RTP payload type to use in remote (egress) RTP header */
  uint32_t ptime : 8;              /* in msec */
#else
  uint32_t ptime : 8;              /* in msec */
  uint32_t rtp_payload_type : 8;   /* RTP payload type to use in remote (egress) RTP header */
  uint32_t dtmf_mode : 8;          /* Refer to dtmf_processing enum */
  uint32_t dtmf_payload_type : 8;  /* DTMF payload type to use in remote (egress) RTP header */
#endif
  union
  {
      struct
      {
          uint32_t codec_flags;     /* See enum amr_codec_flags */
      } amr;
      struct
      {
          uint32_t codec_flags;     /* See enum evrc_codec_flags */
#ifdef __BIG_ENDIAN__
          uint32_t reserved : 8;
          uint32_t hangover : 8;
          uint32_t dtxmin : 8;
          uint32_t dtxmax : 8;
#else
          uint32_t dtxmax : 8;
          uint32_t dtxmin : 8;
          uint32_t hangover : 8;
          uint32_t reserved : 8;
#endif
      } evrc;
      struct
      {
          uint32_t codec_flags;             /* See enum opus_codec_flags */
#ifdef __BIG_ENDIAN__
          uint32_t sprop_max_capture_rate : 16;  /* 8000 - 48000 */
          uint32_t max_playback_rate : 16;       /* 8000 - 48000 */
#else
          uint32_t max_playback_rate : 16;       /* 8000 - 48000 */
          uint32_t sprop_max_capture_rate : 16;  /* 8000 - 48000 */
#endif
      } opus;

#ifdef USE_ATCA_EVS_MODS
      struct
      {
          uint32_t codec_flags;             /* See evs_codec_flags */
#ifdef __BIG_ENDIAN__
          uint32_t fixed_sid_update_interval : 16;     /* # frame periods (20 ms/frame) b/w fixed mode SID updates : {3 .. 100, default = 8} */
          uint32_t adaptive_sid_update_interval : 16;  /* # frame periods (20 ms/frame) b/w adaptive mode SID updates : {8 .. 50, default = 25} */
#else
          uint32_t adaptive_sid_update_interval : 16;  /* # frame periods (20 ms/frame) b/w adaptive mode SID updates : {8 .. 50, default = 25} */
          uint32_t fixed_sid_update_interval : 16;     /* # frame periods (20 ms/frame) b/w fixed mode SID updates : {3 .. 100, default = 8} */
#endif
      } evs;
#else
      struct
           {
               uint32_t codec_flags;             /* See evs_codec_flags */
           } evs;

#endif  /* ATCA blade system EVS mods */
      struct
      {
         uint32_t codec_flags;               /* See melpe_codec_flags */
      } melpe;
  } u;
};

#if DECLARE_LEGACY_DEFINES
/* legacy define for .dtmf to .dtmf_mode change in voice_attributes() struct above */
  #define dtmf dtmf_mode
#endif

struct video_attributes {

#ifdef __BIG_ENDIAN__
  uint32_t yres : 16;  /* y resolution in pixels */
  uint32_t xres : 16;  /* x resolution in pixels */
  uint32_t reserved : 16;
  uint32_t fps : 16;   /* frames per sec */
#else
  uint32_t xres : 16;  /* x resolution in pixels */
  uint32_t yres : 16;  /* y resolution in pixels */
  uint32_t fps : 16;   /* frames per sec */
  uint32_t reserved : 16;
#endif
};

#define DS_IPV4_ADDR_LEN 4
#define DS_IPV6_ADDR_LEN 16

enum ip_type {

  DS_IPV4,
  DS_IPV6
};

#define DS_MERGE_TYPE_FIELD 0xF

enum merge_type {
   DS_STREAM_GROUP_OWNER = 1,
   DS_STREAM_GROUP_CONTRIBUTOR = 2
};

struct ip_addr {

  enum ip_type type;

  union
  {
     uint32_t ipv4;
     uint8_t ipv6[DS_IPV6_ADDR_LEN];
  } u;
};

#ifdef DYNAMIC_JITTER_ENABLE
typedef struct {

#ifdef __BIG_ENDIAN__
/* Delay values are multiples of ptime, eg: 0, 1, 2, 3, etc */
   uint32_t reserved : 8;
   uint32_t min_delay : 8;            /* Minimum allowed delay */
   uint32_t max_delay : 8;            /* Maximum allowed delay, ultimately limited based on Rx buffer size (total size of all packets) or heap memory (jitter buffer entries) */
   uint32_t target_delay : 8;         /* Starting delay value, C66x sends alarm when target delay is consistently not being reached */
   uint32_t decay_coefficient : 16;   /* 'B' coefficient for weighting the current delay value */
   uint32_t attack_coefficient : 16;  /* 'A' coefficient for weighting the new delay value */
   uint32_t max_depth_ptimes;
#else
   uint32_t target_delay : 8;         /* Starting delay value, C66x sends alarm when target delay is consistently not being reached */
   uint32_t max_delay : 8;            /* Maximum allowed delay, ultimately limited based on Rx buffer size (total size of all packets) or heap memory (jitter buffer entries) */
   uint32_t min_delay : 8;            /* Minimum allowed delay */
   uint32_t reserved : 8;
   uint32_t attack_coefficient : 16;  /* 'A' coefficient for weighting the new delay value */
   uint32_t decay_coefficient : 16;   /* 'B' coefficient for weighting the current delay value */
   uint32_t max_depth_ptimes;
#endif
} JITTER_BUFFER_CONFIG;
#endif

typedef struct {

  uint32_t term_id;

/* common items for audio and video */

#ifdef USE_BIT8FIELDS

  #ifdef __BIG_ENDIAN__
    uint32_t vqe_processing_interval : 16; /* ms, reserved, currently not used, maybe configurable per realm */
    /* uint16_t payload_framesize;  // bits */
    uint32_t codec_type : 8;  /* if media_type is VOICE, use voice_codec_type enum, else use video_codec_type enum */
    uint32_t media_type : 8;  /* see enum media_type definition */
  #else
    uint32_t media_type : 8;  /* see enum media_type definition */
    uint32_t codec_type : 8;  /* if media_type is VOICE, use voice_codec_type enum, else use video_codec_type enum */
    /* uint16_t payload_framesize;  // bits */
    uint32_t vqe_processing_interval : 16; /* ms, reserved, currently not used, maybe configurable per realm */
  #endif

#else

   uint32_t vqe_processing_interval; /* ms, reserved, currently not used, maybe configurable per realm */
   uint32_t media_type;  /* see enum media_type definition */
   uint32_t codec_type;  /* if media_type is VOICE, use voice_codec_type enum, else use video_codec_type enum */

#endif

  uint32_t bitrate;     /* bps */

  /* Example1: G711    8kHz sampling rate bitrate =  8000 sample/s * 8 bits/sample =  64000 bps */
  /* Example2: G711.1 16kHz sampling rate bitrate = 16000 sample/s * 8 bits/sample = 128000 bps */
  /* Example3: AMR-NB codec rate 4.75 kbps = 4750 bps */

  struct ip_addr remote_ip;
  struct ip_addr local_ip;
#ifdef __BIG_ENDIAN__
  uint32_t local_port : 16;
  uint32_t remote_port : 16;
#else
  uint32_t remote_port : 16;
  uint32_t local_port : 16;
#endif

#ifdef DYNAMIC_JITTER_ENABLE
  JITTER_BUFFER_CONFIG jb_config;
#endif

#ifdef USE_ATCA_HOST_TERM_MODS
  uint32_t  pkt_seq_num; /*packet sequence number to be used as base sequence number by dsp for transcoded packets*/
#endif

  union {
     struct voice_attributes voice_attr;
     struct video_attributes video_attr;
  } attr;

#ifdef ENABLE_TERM_MODE_FIELD

  #define TERMINATION_MODE_DEFAULT           0  /* termN mode uFlags values can be OR'd together in application code and session config files */
  #define TERMINATION_MODE_IP_PORT_DONTCARE  1

  uint32_t mode;
#endif

#ifdef _X86

/* "uFlags" definitions */

  #define TERM_DTX_ENABLE                         1      /* enable DTX handling for termN */
  #define TERM_SID_REPAIR_ENABLE                  2      /* enable SID repair for termN: correct SID packet loss when possible */
  #define TERM_PKT_REPAIR_ENABLE                  4      /* enable packet repair for termN:  correct media packet loss when possible */
  #define TERM_OVERRUN_SYNC_ENABLE                8      /* enable overrun synchronization in streamlib */
  #define TERM_EXPECT_BIDIRECTIONAL_TRAFFIC       0x10   /* applications should set this flag for telecom mode applications. If not set, packet/media thread receive queue handling performance is increased for unidirectional traffic (analytics mode) */
  #define TERM_IGNORE_ARRIVAL_PACKET_TIMING       0x20   /* set this if packet arrival timing is not accurate, for example pcaps without packet arrival timestamps, analytics mode sending packets faster than real-time, etc */
  #define TERM_OOO_HOLDOFF_ENABLE                 0x40   /* see DS_GETORD_PKT_ENABLE_OOO_HOLDOFF comments in pktlib.h */
  #define TERM_DISABLE_DORMANT_SESSION_DETECTION  0x80   /* see comments in mediaTest/cmd_line_debug_flags.h */
  #define TERM_DYNAMIC_SESSION                    0x100  /* flag set by mediaMin in term1 and term2 structs when creating dynamic sessions. This is an "informational only" flag, useful only for status and information during a session lifespan, JHB Jan 2023 */
  #define TERM_ANALYTICS_MODE_PACKET_TIMING       0x200  /* flag set by applications in term1 and term2 structs when creating sessions. mediaMin does this in SetSessionTiming() in session_app.cpp, JHB May 2023 */
  #define TERM_NO_PACKET_TIMING                   0x400  /* flag set by applications in term1 and term2 structs when creating sessions. mediaMin does this in SetSessionTiming() in session_app.cpp, JHB May 2023 */
  uint32_t sample_rate;
  uint32_t input_sample_rate;
  uint32_t buffer_depth;
  uint32_t uFlags;
  uint16_t ptime;                      /* in msec */
  float    input_buffer_interval;      /* in msec */
  int16_t  output_buffer_interval;     /* in msec */
  uint32_t delay;                      /* in msec */
  uint16_t max_loss_ptimes;            /* number of consecutive packet loss ptimes before PLM activates (Packet Loss Monitoring).  Default is 3 */
  uint16_t max_pkt_repair_ptimes;      /* max number of consecutive lost packets that pktlib will attempt to repair.  Default is 3 */
  uint8_t  RFC7198_lookback;           /* amount of packets to look back for RFC7198 de-duplication in DSRecvPackets(). Zero disables, JHB May 2023 */

  #define MAX_GROUPID_LEN 128
  char group_id[MAX_GROUPID_LEN];
  uint32_t group_mode;
  uint32_t group_status;
#endif

  uint32_t dormant_SSRC_wait_time;     /* time period before to wait before a session channel's SSRC can be considered dormant (in msec). See CheckForDormantSSRC() in packet_flow_media_proc.c for comments */
  int32_t payload_shift;               /* non-zero indicates amount of RTP payload shift after encoding or before decoding. Shift conditions can be controlled by filter flags (in bits 15-8). Shift amount ranges from -8 to +7 (in bits 7-0) */
  uint8_t Reserved3;                   /* note - changed Reserved3 to 8-bit from 32-bit to compensate for changing input_buffer_interval to float from uint16_t and adding RFC7198_lookback, May 2023 */
  uint32_t Reserved4;
  uint32_t Reserved5;

} TERMINATION_INFO;

#define TERMINFO TERMINATION_INFO  /* nickname :-) */

typedef struct {

   uint32_t session_id;
#ifdef _X86
   uint32_t thread_id;  /* thread or core id */
#endif
   
   /* ha_index = 0 (ACTIVE or 1+1 case),  = X = (1..N) (Standby for Active X) */
   uint32_t ha_index;

   TERMINATION_INFO term1;
   TERMINATION_INFO term2;
#ifdef _X86
   TERMINATION_INFO group_term;
   #define MAX_SESSION_NAME_LEN 128
   char szSessionName[MAX_SESSION_NAME_LEN];  /* session name used for some output pcap/wav file naming purposes (optional, does not need to be set), JHB Jun 2019 */
#endif

} SESSION_DATA;

typedef struct {
   uint32_t term_id;
   uint32_t ssrc;
   uint32_t seq_num;
   uint32_t timestamp;

} HOST_REP_TERM_INFO;

typedef struct {
   uint32_t session_id;

   HOST_REP_TERM_INFO term1;
   HOST_REP_TERM_INFO term2;

} HOST_REP_SESSION_DATA;

#ifdef _X86  /* added JHB Nov 2018 */

/* thread level items indexed by session */

#define MAX_TERMS               2    /* current max terms allowed per session, not including group (algorithm) term */
#define MAX_GROUP_CONTRIBUTORS  8    /* max members per group (for example, merge contributors) */
#define MAX_SSRC_TRANSITIONS    128  /* max SSRC transitions allowed for analyzing and logging RFC8108 */

#define SSRC_LIVE               1
#define SSRC_DORMANT            2

#ifndef __cplusplus
   #ifndef bool
     #define bool unsigned char
     #define bool_def
   #endif
#endif
   
typedef struct {

  unsigned long long init_time;
  unsigned long long look_ahead_time;

  bool fUseJitterBuffer;  /* per session var set true if session definition enables jitter buffer (default is enabled) */
  bool fDataAvailable;    /* whether data is available for the session; normally true, but for obvious end-of-data situations, such as end of a pcap file, external thread flushes the session, etc it goes to false */

/* FTRT mode stream-to-channel mapping items */

  int chnum_map[MAX_TERMS];
  int chnum_map_history[MAX_TERMS];
  int num_streams_active;

/* SSRC tracking and transition detection items */

  int last_rtp_SSRC[MAX_TERMS][MAX_SSRC_TRANSITIONS];
  uint8_t num_SSRC_changes[MAX_TERMS];
  bool fSSRC_change_active[MAX_TERMS];
  uint8_t ssrc_state[MAX_TERMS];

/* stream group related items */

  int stream_group_buffer_time;  /* stream group buffer time, in msec. Changing this affects stream merging and sample domain processing. An integral number of stream group frames makes up a buffer, JHB Jan2021 */
  bool fAllContributorsPresent;
  unsigned int uMissingContributions[MAX_GROUP_CONTRIBUTORS];
  int nPrevMissingContributor[MAX_GROUP_CONTRIBUTORS];

} SESSION_INFO_THREAD;

#ifdef bool_def
  #undef bool
#endif

#endif

#endif /* _SESSION_H_ */
