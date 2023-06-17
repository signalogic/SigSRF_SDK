/*
  cmd_line_options_flags.h

  header file for mediaMin and mediaTest reference applications, definitions for -dN cmd line options and flags
  
  Copyright (C) Signalogic, 2018-2023

  License

   Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

  Revision History

   Created Jan 2022 JHB, option and flag definitions moved here from mediaMin.h to allow Mode flags to apply to both mediaMin and mediaTest. Note use below of m| and mm| to specify whether a flag applies to mediaMin only or both
   Modified Dec 2021 JHB, add new command line debug flags ENABLE_DER_DECODING_STATS and ENABLE_INTERMEDIATE_PCAP
   Modified Sep 2022 JHB, add DISABLE_DORMANT_SESSION_DETECTION flag. See commments below 
   Modified Dec 2022 JHB, add ENABLE_ASN_OUTPUT flag to enable ASN.1 output to XML file for HI2/HI3 input
   Modified Dec 2022 JHB, change filename from cmd_line_debug_flags.h to cmd_line_options_flags.h
   Modified Dec 2022 JHB, add DISABLE_JITTER_BUFFER_OUTPUT_PCAPS and ENABLE_WAV_OUT_SEEK_TIME_ALARM flags, re-order some debug and alarm flags
   Modified Jan 2023 JHB, add SIP handling options including ENABLE_STREAM_SDP_INFO, add ALLOW_OUTOFSPEC_RTP_PADDING flag
   Modified Jun 2023 JHB, add SLOW_DORMANT_SESSION_DETECTION flag
*/

#ifndef _CMDLINEOPTIONSFLAGS_H_
#define _CMDLINEOPTIONSFLAGS_H_

/* following are standard operating modes, stress tests, and options that can be specified by -dN cmd line entry. Notes:

  -N may be given in hex format, for example -d0xN
  -value of N is referred to in mediaMin.cpp source as "Mode" and x86_mediaTest.c source as "debugMode" (they are the same)
  -in the comments for each flag, m| indicates mediaMin only, mm| indicates both mediaMin and mediaTest
*/

/* standard operating modes */

#define SESSION_CONFIG_FILE                  0  /* m| default mode (no -d entry), a session config file must be given on the cmd line, static sessions are created */
#define DYNAMIC_SESSIONS                     1  /* m| treat each cmd line input spec ("-ixx") as multistream and dynamically create sessions as they appear. If stream groups are enabled, each call has its own stream group.  If a session config file is given on the cmd line it's ignored */
#define COMBINE_INPUT_SPECS                  2  /* m| similar to DYNAMIC_SESSIONS, but combine all cmd line input specs into one multistream (and if stream groups are enabled, combine all group output into one group) */
#define ENABLE_STREAM_GROUP_DEDUPLICATION    4  /* m| applies a deduplication algorithm, which looks for similar content between stream group contributors and attempts to align similar streams. The objective is to reduce perceived reverb/echo due to duplicated streams. A typical scenario is a multipath (duplicated) endpoint with different latencies */
#define ENABLE_STREAM_GROUP_ASR              8  /* m| enable ASR processing on stream group output */
#define ENABLE_DER_STREAM_DECODE             0x1000

#define USE_PACKET_ARRIVAL_TIMES             0x10  /* m| use arrival times (packet timestamps) in pcap records to control push rate. Should not be specified concurrently with AUTO_ADJUST_PUSH_RATE */

/* stress tests / functional tests (see also fStressTest and fCapacityTest flags below) */

#define CREATE_DELETE_TEST                   0x20  /* m| basic create / delete session stress test (automatically repeats) */
#define CREATE_DELETE_TEST_PCAP              0x40  /* m| create / delete session stress test using sessions found in pcap (automatically repeats) */
#define START_THREADS_FIRST                  0x80  /* m| static sessions are created before starting packet/media thread(s) by default; set this to start threads first.  Dynamic sessions are always created after starting packet/media thread(s).  See StartPacketMediaThreads() */
#define ENERGY_SAVER_TEST                    0x100  /* m| enables an initial delay before pushing packets to test packet/media thread "energy saver" mode */
#define REPEAT_INPUTS                        0x200  /* mm| repeat inputs, for example rewind pcap files when they finish. Note this mode requires 'q' key manual entry to exit */
#define ENABLE_RANDOM_WAIT                   0x20000  /* m| enable random wait when a mediaMin application thread is repeating. This flag is normally used in stress tests */

/* operating mode options */

#define ENABLE_STREAM_GROUPS                 0x400  /* m| enable stream groups, currently only valid with dynamic session modes. If set the first session created from each multistream pcap will contain a stream group. Currently the default stream group processing is merging and time alignment of all audio */
#define ENABLE_WAV_OUTPUT                    0x800  /* m| enable wav file output for stream group processing, such as audio stream merging */
#define ROUND_ROBIN_SESSION_ALLOCATION       0x4000  /* m| allocate sessions to packet/media threads in round-robin manner. The idea is to keep p/m thread load balanced. This flag should be specified for high capacity situations */
#define WHOLE_GROUP_THREAD_ALLOCATE          0x8000  /* m| do not split stream group sessions across packet/media threads. This avoids use of locks (semaphores) inside streamlib and gives higher performance */
#define ANALYTICS_MODE                       0x40000  /* m| enable combination of pktlib FTRT mode and ptime interval packet push/pull. This is used in analytics mode when input packets do not have wall clock timing, or they do but timing is unreliable (lawful interception is one example) */
#define AUTO_ADJUST_PUSH_RATE                0x80000  /* m| enable automatically adjusting packet push rate. Currently supported only when ANALYTICS_MODE is enabled. Should not be specified concurrently with USE_PACKET_ARRIVAL_TIMES */

/* disables, enables */

#define DISABLE_DTX_HANDLING                 0x100000  /* m| DTX handling is enabled by default */
#define DISABLE_FLC                          0x200000  /* m| stream group output FLC is enabled by default */
#define ENABLE_ONHOLD_FLUSH_DETECT           0x400000  /* m| on-hold flush detection is disabled by default.  This is deprecated and has been replaced by a concept known as "pastdue flush", which streamlib algorithms use to flush jitter buffers in the presence of high rates of packet loss.  If on-hold flush is enabled, packets are flushed from their jitter buffers if a stream is inactive for 0.2 sec (which is a very arbitrary amount; the pastdue approach reacts faster and more precisely) */
#define DISABLE_PACKET_REPAIR                0x800000  /* m| packet repair enabled by default. Missing SID and media packets (detected by sequence number and timestamp discontinuities after packet re-ordering) are repaired */
#define DISABLE_CONTRIB_PACKET_FLUSH         0x1000000  /* m| group contributor streams are flushed from their jitter buffer when their contribution rate becomes slow, decreasing the need for FLC on stream group combined output. Enabled by default */
#define ENABLE_FLC_HOLDOFFS                  0x2000000  /* m| enable FLC holdoffs to attempt to optimize audio quality in some cases (see documentation). Ignored if DISABLE_FLC flag is set. Disabled by default */
#define DISABLE_DORMANT_SESSION_DETECTION    0x4000000  /*m| disable dormant session detection and flush. Dormant sessions are defined as a session with a channel SSRC that was in used, has not been in use for some time, and then that SSRC is "taken over" by another session / channel. In that case the dormant session is flushed and any remaining media is cleared from session state information and jitter buffers. In cases where multiple sessions "overload" an SSRC value (i.e. duplicated SSRCs by actually different sessions) it's advisable to disable dormant session detection to avoid unwanted flushing. Note this flag applies to all sessions handled by mediaMin; to disable on per-session basis see TERM_DISABLE_DORMANT_SESSION_DETECTION flag in shared_include/session.h */
#define DISABLE_JITTER_BUFFER_OUTPUT_PCAPS   0x8000000  /* m| disable intermediate jitter buffer output pcap files. By default mediaMin pulls jitter buffer output packets from packet/media threads using DSPullPackets() with the DS_PULLPACKETS_JITTER_BUFFER flag, and writes to xx_jbN.pcap files. Filename formation (and the disable if active) is inside JitterBufferOutputSetup() in mediaMin.cpp */ 

/* debug info: extra stats, mem stats, audio output alignment markers, intermediate pcap output */

#define ENABLE_DEBUG_STATS                   0x10000000  /* mm| enable debug info and stats for (i) additional mediaMin warnings, (ii) internal packet/media thread, (iii) audio merging, and (iv) DER stream decoding */
#define ENABLE_DEBUG_STATS_L2                0x20000000  /* reserved */
#define ENABLE_ALIGNMENT_MARKERS             0x40000000  /* m| when combined with the ENABLE_STREAM_GROUP_DEDUPLICATION flag, enables alignment markers to show the point at which streams were aligned (the deduplication algorithm uses cross correlation to align one or more streams) */
#define ENABLE_TIMING_MARKERS                0x80000000  /* m| inject 1 sec wall clock timing markers in stream group output.  This can be helpful in debugging timing issues, for example the app (in this case mediaMin) is not pulling packets fast enough, or not maintaining a consistent pull interval. Additional audio marker options can be specified with uDebugMode (see examples below in DebugSetup() and LoggingSetup(), see also DEBUG_CONFIG struct definitions in shared_include/config.h) */
#define ENABLE_MEM_STATS                     0x100000000LL  /* mm| show mem usage stats in the event log */
#define ENABLE_DER_DECODING_STATS            0x200000000LL  /* m| show stats and info messages for DER encapsulated streams */
#define ENABLE_INTERMEDIATE_PCAP             0x400000000LL  /* m| for HI2/HI3 and .ber input, enable intermediate pcap output after decoding */
#define ENABLE_ASN_OUTPUT                    0x800000000LL  /* m| for HI2/HI3 input, enable ASN.1 output to XML file */
#define ENABLE_ASN_OUTPUT_DEBUG_INFO         0x1000000000LL  /* m| enable intermediate ASN decoding info, warning, and error messages */

/* alarms */

#define ENABLE_PACKET_INPUT_ALARM            0x10000000000LL  /* m| enable per-stream packet input alarm -- if no packets are received by pktlib for a stream; i.e. no packets are pushed via DSPushPackets() API by an application (in this case by mediaMin) for some elapsed time, then pktlib will display/log a warning message for the stream. The alarm is intended as a debug tool or an indicator, as a stream may stop for a range of normal reasons, or there may actually be a problem */
#define ENABLE_WAV_OUT_SEEK_TIME_ALARM       0x20000000000LL  /* m| enable wav output seek time alarm. This can be useful if packet/media thread (pktlib) pre-emption warnings are displayed on the console or in the event log showing "last stream group time" with a high value. If so, enabling this alarm helps look into what inside streamlib is causing the pre-emption */

/* SIP handling */

#define ENABLE_STREAM_SDP_INFO               0x1000000000000LL  /* m| filter input streams for SIP Invite messages and SAP/SDP protocol packets and if found add SDP info contents to the input stream's SDP database. This has the same effect as .sdp files on command line; the two methods can be used together */
#define DISABLE_TERMINATE_STREAM_ON_BYE      0x2000000000000LL  /* m| disable stream termination on BYE messages. Default is enabled */

/* misc */

#define DISABLE_AUTOQUIT                     0x10000000000000LL  /* m| disable automatic quit for cmd lines with (i) all inputs are files (i.e. no UDP or USB audio inputs) and (ii) no repeating stress or capacity tests. Automatic quit is enabled by default */
#define ALLOW_OUTOFSPEC_RTP_PADDING          0x20000000000000LL  /* mm| allow out-of-spec RTP padding. Suppresses error messages for RTP packets with unused trailing payload bytes not declared with the padding bit in the RTP packet header. See comments in CreateDynamicSession() in mediaMin.cpp */

#define SLOW_DORMANT_SESSION_DETECTION       0x40000000000000LL  /* mm| extend dormant session detection time. See usage in CreateDynamicSession() in mediaMin.cpp */ 

#endif  /* _CMDLINEOPTIONSFLAGS_ */
