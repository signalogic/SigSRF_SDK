/*
  cmd_line_debug_flags.h

  header file for mediaMin and mediaTest reference applications defining -dN cmd line flags
  
  Copyright (C) Signalogic, 2018-2022

  Revision History

   Created Jan 2022 JHB, debug flag definitions moved here from mediaMin.h to clarify which flags apply to mediaMin only and which to both mediaMin and mediaTest
*/

#ifndef _CMDLINEDEBUGFLAGS_H_
#define _CMDLINEDEBUGFLAGS_H_

/* following are standard operating modes, stress tests, and options that can be specified by -dN cmd line entry. Notes:

  -N may be given in hex format, for example -d0xN
  -value of N is referred to in mediaMin.cpp source as "Mode" and x86_mediaTest.c source as "debugMode"
  -in the comments for each flag, m indicates mediaMin only, mm indicates both mediaMin and mediaTest
*/

/* standard operating modes */

#define SESSION_CONFIG_FILE                   0  /* m | default mode (no -d entry), a session config file must be given on the cmd line, static sessions are created */
#define DYNAMIC_CALL                          1  /* m | treat each cmd line input spec ("-ixx") as a multistream call and dynamically create sessions as they appear.  If stream groups are enabled, each call has its own stream group.  If a session config file is given on the cmd line it's ignored */
#define COMBINE_CALLS                         2  /* m | similar to DYNAMIC_CALL, but combine all cmd line input specs into one call (and if stream groups are enabled, combine all group output into one group) */
#define ENABLE_STREAM_GROUP_DEDUPLICATION     4  /* m | applies a deduplication algorithm, which looks for similar content between stream group contributors and attempts to align similar streams. The objective is to reduce perceived reverb/echo due to duplicated streams. A typical scenario is a multipath (duplicated) endpoint with different latencies */
#define ENABLE_STREAM_GROUP_ASR               8  /* m | enable ASR processing on stream group output */
#define ENABLE_DER_STREAM_DECODE         0x1000

#define USE_PACKET_ARRIVAL_TIMES           0x10  /* m | use arrival times (packet timestamps) in pcap records to control push rate (can be combined with other mode flags) */

/* stress tests / functional tests (see also fStressTest and fCapacityTest flags below) */

#define CREATE_DELETE_TEST                 0x20  /* m | basic create / delete session stress test (automatically repeats) */
#define CREATE_DELETE_TEST_PCAP            0x40  /* m | create / delete session stress test using sessions found in pcap (automatically repeats) */
#define START_THREADS_FIRST                0x80  /* m | static sessions are created before starting packet/media thread(s) by default; set this to start threads first.  Dynamic sessions are always created after starting packet/media thread(s).  See StartPacketMediaThreads() */
#define ENERGY_SAVER_TEST                 0x100  /* m | enables an initial delay before pushing packets to test packet/media thread "energy saver" mode */
#define REPEAT_INPUTS                     0x200  /* mm | repeat inputs, for example rewind pcap files when they finish. Note this mode requires 'q' key manual entry to exit */
#define ENABLE_RANDOM_WAIT              0x20000  /* m | enable random wait when a mediaMin application thread is repeating. This flag is normally used in stress tests */

/* operating mode options */

#define ENABLE_STREAM_GROUPS              0x400  /* m | enable stream groups, currently only valid with dynamic call modes. If set the first session created from each multistream pcap will contain a stream group. Currently the default stream group processing is merging and time alignment of all audio */
#define ENABLE_WAV_OUTPUT                 0x800  /* m | enable wav file output for stream group processing, such as audio stream merging */
#define ROUND_ROBIN_SESSION_ALLOCATION   0x4000  /* m | allocate sessions to packet/media threads in round-robin manner. The idea is to keep p/m thread load balanced. This flag should be specified for high capacity situations */
#define WHOLE_GROUP_THREAD_ALLOCATE      0x8000  /* m | do not split stream group sessions across packet/media threads. This avoids use of locks (semaphores) inside streamlib and gives higher performance */
#define ANALYTICS_MODE                  0x40000  /* m | enable combination of pktlib FTRT mode and ptime interval packet push/pull. This is used in analytics mode when input packets do not have wall clock timing, or they do but timing is unreliable (lawful interception is one example) */
#define ENABLE_AUTO_ADJUST_PUSH_RATE    0x80000  /* m | enable automatic control of push rate.  Currently supported only when ANALYTICS_MODE and DYNAMIC_CALL are also enabled */

/* disables, enables */

#define DISABLE_DTX_HANDLING           0x100000  /* m | DTX handling is enabled by default */
#define DISABLE_FLC                    0x200000  /* m | stream group output FLC is enabled by default */
#define ENABLE_ONHOLD_FLUSH_DETECT     0x400000  /* m | on-hold flush detection is disabled by default.  This is deprecated and has been replaced by a concept called "pastdue flush", which streamlib algorithms use to flush jitter buffers in the presence of high rates of packet loss.  If on-hold flush is enabled, packets are flushed from their jitter buffers if a stream is inactive for 0.2 sec (which is a very arbitrary amount; the pastdue approach reacts faster and more precisely) */
#define DISABLE_PACKET_REPAIR          0x800000  /* m | packet repair enabled by default. Missing SID and media packets (detected by sequence number and timestamp discontinuities after packet re-ordering) are repaired */
#define DISABLE_CONTRIB_PACKET_FLUSH  0x1000000  /* m | group contributor streams are flushed from their jitter buffer when their contribution rate becomes slow, decreasing the need for FLC on stream group combined output. Enabled by default */
#define DISABLE_AUTOQUIT              0x2000000  /* m | disable automatic quit for cmd lines with (i) all inputs are files (i.e. no UDP or USB audio inputs) and (ii) no repeating stress or capacity tests. Automatic quit is enabled by default */

/* alarms, debug, mem, intermediate pcap output, and extra stats */

#define ENABLE_PACKET_INPUT_ALARM       0x10000  /* m | enable packet input alarm -- if no packets are received by pktlib; i.e. no packets are pushed via DSPushPackets() API by an application (in this case by mediaMin) for some elapsed time, then pktlib will print a warning message in the event log */
#define ENABLE_TIMING_MARKERS        0x08000000  /* m | inject 1 sec wall clock timing markers in stream group output.  This can be helpful in debugging timing issues, for example the app (in this case mediaMin) is not pulling packets fast enough, or not maintaining a consistent pull interval. Additional audio marker options can be specified with uDebugMode (see examples below in DebugSetup() and LoggingSetup(), see also DEBUG_CONFIG struct definitions in shared_include/config.h) */
#define ENABLE_DEBUG_STATS           0x10000000  /* mm | enable debug info and stats for (i) additional mediaMin warnings, (ii) internal packet/media thread, (iii) audio merging, and (iv) DER stream decoding */
#define ENABLE_DEBUG_STATS_L2        0x20000000  /* reserved */
#define ENABLE_ALIGNMENT_MARKERS     0x40000000  /* m | when combined with the ENABLE_STREAM_GROUP_DEDUPLICATION flag, enables alignment markers to show the point at which streams were aligned (the deduplication algorithm uses cross correlation to align one or more streams) */
#define ENABLE_MEM_STATS             0x80000000  /* mm | show mem usage stats in the event log */
#define ENABLE_DER_DECODING_STATS 0x100000000LL  /* m | show stats and info messages for DER encapsulated streams */
#define ENABLE_INTERMEDIATE_PCAP  0x200000000LL  /* m | for HI2/HI3 and .ber input, enable intermediate pcap output after decoding */

#endif  /* _CMDLINEDEBUGFLAGS_ */
