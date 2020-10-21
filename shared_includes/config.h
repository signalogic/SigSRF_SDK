/* config.h

  Description:
   
    Configuration for x86, coCPU, or combined platforms

    Currently used by pktlib, streamlib, and voplib (may be expanded in the future)

  Copyright (C) Signalogic and Mavenir Systems 2013-2015

    Support for c66x card PCIe and SRIO interfaces

  Copyright (C) Signalogic, Inc, 2016-2020

    Add APIs to support x86-only or combined x86 and coCPU platforms.  APIs are now consistent between all use cases (for example, no difference betweeen coCPU platforms that use PCIe or SRIO interfaces)

  Revision History

    Created Sep 2013, JHB, configuration support for SigC66xx PCIe cards and SigC641x PMC/PTMC modules

    Modified Jan 2015
     -change elements of size less than 32-bits to use bitfields
     -add support for BIG_ENDIAN to support ATCA platforms (SRIO host interface)

    Modified Dec 2016 JHB
     -add DS_JITTER_BUFFER_RTPGEN_COMPENSATE enum (see comments below)
     
    Modified Aug 2017 CKJ
      -added uLogFile (log file handle) to DEBUG_CONFIG struct
      
    Modified Sep-Oct 2018 JHB
      -add items to DEBUG_CONFIG struct
      -add uMaxSessionsPerThread to GLOBAL_CONFIG struct

    Modified Dec 2018 JHB
      -add uMaxGroupsPerThread, uThreadEnergySaverStateInactivityTime, and uThreadEnergySaverStateSleepTime to GLOBAL_CONFIG struct

    Modified Jan 2019 JHB
      -add uEnablePktTracing to DEBUG_CONFIG struct
      
    Modified Apr 2019 JHB
      -add uEnableDataObjectStats to DEBUG_CONFIG struct
      
    Modified Nov 2019 JHB
      -additional DEBUG_MODE enums for uDebugMode in DEBUG_CONFIG struct

    Modified Dec 2019 JHB
      -added event log path and ufflush_size in DEBUG_CONFIG struct, EVENT_LOG_MODE enums
  
    Modified Feb 2020 JHB
      -add uEventLogMode flags DS_EVENT_LOG_UPTIME_TIMESTAMPS, DS_EVENT_LOG_WALL_CLOCK_TIMESTAMPS, DS_EVENT_LOG_WARN_ERROR_ONLY
      -make uEventLogMode uint32_t

   Modified Mar 2020 JHB
      -add DS_EVENT_LOG_ADD_NEWLINE, DS_EVENT_LOG_IGNORE_LINE_CURSOR_POS flags to DEBUG_CONFIG struct

   Modified Jun 2020 JHB
      -add DS_INJECT_GROUP_ALIGNMENT_MARKERS flag for uDebugMode in DEBUG_CONFIG struct
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

typedef struct {

#ifdef __BIG_ENDIAN__
   uint32_t uWatchdogTimerMode : 16;
   uint32_t uMaxCoreChan : 16;
#else
   uint32_t uMaxCoreChan : 16;        /* max per-core channels, default 2048 */
   uint32_t uWatchdogTimerMode : 16;  /* watchdog timer mode:  0 = disabled, 1 = enabled, 3 = enabled with auto core reset, default 3 */
#endif

   uint32_t cpu_usage_low_watermark;
   uint32_t cpu_usage_high_watermark;

#ifdef __BIG_ENDIAN__
   uint32_t reserved : 24;
   uint32_t uPreserve_SSRC : 8;
#else
   uint32_t uPreserve_SSRC : 8;       /* 0 = preserve SSRC, 1 = assign new SSRC, default 0 */
   uint32_t reserved : 24;
#endif

#ifdef __BIG_ENDIAN__
   uint32_t num_ports : 16;
   uint32_t port_start : 16;
#else
   uint32_t port_start : 16;          /* starting udp port to listen on; field only valid for virtual ip */
   uint32_t num_ports : 16;           /* number of ports per core to listen on; field only valid for virtual ip */
#endif

#ifdef USE_ATCA_GLOBALCONFIG_MODS
   uint32_t uInActiveTimeOut;
   uint32_t uIdleTimeOut;
   uint32_t uNtpTsMsw;
   uint32_t uNtpTsLsw;
#endif

#ifdef _X86
   uint32_t uMaxSessionsPerThread;  /* allowed max value of sessions assigned to a packet/media thread.  Can be exceeded in some circumstances, see session-to-thread allocation logic in DSCreateSession (in pktlib.c) */
   uint32_t uMaxGroupsPerThread;    /*     ""     ""        stream groups    ""   */

   uint32_t uThreadEnergySaverInactivityTime;  /* inactivity time (i.e. no input packets) after which a packet/media thread will enter "energy saver" state in order to reduce CPU usage (in msec). A zero value disables energy saver state.  A typical value might be 20000 (20 sec) */
   uint32_t uThreadEnergySaverSleepTime;  /* amount of time a thread in energy saver state sleeps before checking for input again (in usec) */
   uint32_t uThreadEnergySaverWaitForAppQueuesEmptyTime;  /* additional amount of time to wait for application queues to empty out (in msec).  Default is zero (disabled) */

   uint32_t uThreadPreemptionElapsedTimeAlarm;  /* amount of elapsed time before p/m thread preemption warning will appear in the event log.  If left at zero, DSConfigPktlib() will set to default of 40 msec */

   uint32_t uReserved1;
   uint32_t uReserved2;
   uint32_t uReserved3;
   uint32_t uReserved4;
   uint32_t uReserved5;
   uint32_t uReserved6;
   uint32_t uReserved7;
   uint32_t uReserved8;
   uint32_t uReserved9;
   
#endif

} GLOBAL_CONFIG;

enum DEBUG_MODE {  /* enums for uDebugMode in DEBUG_CONFIG struct below */

/* pktlib debug options */

   DS_JB_DISABLE = 0x1,                        /* Disable jitter buffer */
   DS_VAU_DISABLE = 0x2,                       /* Disable voice activity detection */
   DS_ECU_DISABLE = 0x4,                       /* Disable echo cancellation */
   DS_TDU_DISABLE = 0x8,                       /* Disable tone detection */
   DS_TGE_DISABLE = 0x10,                      /* Disable tone generation */
   DS_DP_DSP_XFER_MEMCPY = 0x20,               /* Force DP->DSP transfer to use memcpy instead of DMA */
   DS_DSP_DP_XFER_MEMCPY = 0x40,               /* Force DSP->DP transfer to use memcpy instead of DMA */
   DS_LOG_HOST_MEM_XFER_TIMES = 0x80,          /* Log host memory transfer times */
   DS_DISABLE_CACHE = 0x100,                   /* Disable Cache, currently must be hard coded or set after code is loaded but before it runs */
   DS_LOG_JITTER_BUFFER = 0x200,               /* Log jitter buffer info */
   DS_JITTER_BUFFER_RTPGEN_COMPENSATE = 0x400, /* Use relaxed jitter buffer timestamp verification code (a way to hide messages in the log that can frequently occur when using rtpGen or other arbitrary pcap manipulation tool), JHB Dec2016 */

/* streamlib debug options */

   DS_INJECT_XCODER_OUTPUT_SINEWAV = 0x1,
   DS_INJECT_FLC_OUTPUT_LEVEL = 0x2,           /* inject level marker instead of FLC output, to show where FLC is occurring */
   DS_INJECT_GROUP_OUTPUT_MARKERS = 0x4,       /* inject markers at output buffer boundaries in stream group output*/
   DS_INJECT_GROUP_TIMING_MARKERS = 0x8,       /* inject 1 sec timing markers in stream group output */
   DS_INJECT_GROUP_ALIGNMENT_MARKERS = 0x10,   /* inject stream alignnment point marker in stream group output */

/* general debug options (they apply to all libs) */

   DS_SHOW_MALLOC_STATS = 0x1000,              /* show malloc stats before/after codec creation in voplib (currently only place malloc is used in SigSRF software, JHB Oct2019 */
   DS_ENABLE_GROUP_MODE_STATS = 0x2000,        /* equivalent to GROUP_MODE_DEBUG_STATS flag in streamlib.h, applies to all stream groups whether or not they were created with GROUP_MODE_DEBUG_STATS, JHB Dec2019 */
   DS_ENABLE_PUSHPACKETS_ELAPSED_TIME_ALARM = 0x4000,  /* set elapsed time alarm inside DSPushPackets() */
   DS_ENABLE_MANAGE_SESSION_STATS = 0x8000,
   DS_ENABLE_EXTRA_PACKET_STATS = 0x10000      /* enable logging of additional packet stats */
};

enum PACKET_STATS_LOGGING {  /* enums for uPktStatsLogging in DEBUG_CONFIG struct below */

  DS_ENABLE_PACKET_STATS_HISTORY_LOGGING = 1,  /* enable packet stats history logging for jitter buffer input and output.  Enabling packet stats history logging allows end-of-call packet log file output, including detailed input vs. output analysis, to be performed by DSWritePacketStatsHistoryLog() (pktlib) or DSPktStatsWriteLogFile() (diaglib) */
  DS_LOG_BAD_PACKETS = 2,                      /* include in packet stats history packets rejected by DSBufferPackets() (add to jitter buffer) because they are malformed, have an out-of-range timestamp or seq number jump, etc.  Rejected packets will show in the input side of the packet log file output, but not output side, causing dropped packet entries in input vs. output analysis */
  DS_ENABLE_PACKET_TIME_STATS = 4,             /* enable run-time packet time stats, these can be displayed in the event log at any time using DSLogPacketTimeLossStats() (pktlib) */
  DS_ENABLE_PACKET_LOSS_STATS = 8              /* enable run-time packet loss stats,  "" */
};

enum PACKING_FORMAT {

   DS_IF1_NOCRC = 0,                           /* Interface format 1 without CRC */
   DS_IF1_CRC = 1,                             /* Interface format 1 with CRC */
   DS_IF2 = 2,                                 /* Interface format 2 */
   DS_MMS_IO = 3,                              /* MMS IO format */
   DS_NO_OVERRIDE = 4                          /* Do not override the packing format; use default */
};

enum EVENT_LOG_MODE {  /* uEventLogMode enums, these are in addition to LOG_xx constants defined in diaglib.h, JHB Dec 2019 */

  DS_EVENT_LOG_DISABLE = 4,                    /* disables all file and screen output.  Calls to Log_RT() do nothing and immediately return */
  DS_EVENT_LOG_APPEND = 8,                     /* open event log in append mode (i.e. append if it already exists) */
  DS_EVENT_LOG_UPTIME_TIMESTAMPS = 0x20,       /* event log uses uptime (relative) time stamps */
  DS_EVENT_LOG_WALLCLOCK_TIMESTAMPS = 0x40,    /* event log uses wall clock (system) date/time stamps */
  DS_EVENT_LOG_WARN_ERROR_ONLY = 0x80,         /* set event log to level 3 output and below.  Intended for temporary purposes, for example file or screen I/O is taking a lot of system time */
};

#define DS_LOG_LEVEL_MASK                 0x1f       /* up to 15 log levels supported */
#define DS_LOG_LEVEL_NO_API_CHECK         0x1000
#define DS_LOG_LEVEL_NO_TIMESTAMP         0x2000
#define DS_LOG_LEVEL_FILE_ONLY            0x4000
#define DS_LOG_LEVEL_APPEND_STRING        0x8000     /* append Log_RT() output, including timestamps if configured, to its string param contents up to first specifier. Note this should be used carefully as it assumes a valid string has been passed to Log_RT() */
#define DS_LOG_LEVEL_DONT_ADD_NEWLINE     0x10000    /* don't add newline to end of Log_RT() strings if one not already there */
#define DS_LOG_LEVEL_IGNORE_LINE_CURSOR_POS 0x20000  /* ignore line cursor position for screen output. No effect on event log file output */

/* flag options for uEnablePktTracing in DEBUG_CONFIG struct (below) */

#define DS_PACKET_TRACE_PUSH              1
#define DS_PACKET_TRACE_RECEIVE           2
#define DS_PACKET_TRACE_JITTER_BUFFER     4
#define DS_PACKET_TRACE_TRANSMIT          8
#define DS_PACKET_TRACE_PULL              0x10
#define DS_PACKET_TRACE_MASK              0xff

#define DS_PACKET_TRACE_LOG_SRC_IP_ADDR   0x100  /* flags for additional info to log during packet tracing.  Default info is the packet's channel number (chnum) and session handle */
#define DS_PACKET_TRACE_LOG_DST_IP_ADDR   0x200
#define DS_PACKET_TRACE_LOG_SRC_UDP_PORT  0x400
#define DS_PACKET_TRACE_LOG_DST_UDP_PORT  0x800

typedef struct {

#ifdef __BIG_ENDIAN__
   uint32_t uLogLevel;
   uint32_t uLoopbackLevel : 16;
   uint32_t reserved1 : 16;
   uint32_t uDebugMode;
   uint32_t reserved2 : 8;
   uint32_t uLowLevelMemTest : 8;
   uint32_t uAMRPackingFormat : 16;
   uint32_t reserved3;
#else
   uint32_t uLogLevel;                        /* Log Level values are defined as follows.  Note that log level usage more or less follow the Linux standard (http://man7.org/linux/man-pages/man2/syslog.2.html)

                                                   0 = Disabled
                                                   1 = System is unusable (critical failure / imminent crash)
                                                   2 = Action must be taken immediately (e.g. peripheral failure, memory error, stack overflow, etc)
                                                   3 = Critical conditions  (e.g. unexpected bad data, buffer overrun, unexpected bad value)
                                                   4 = Warning conditions
                                                   5 = Normal but significant condition (e.g. heavy CPU load, overly high buffer usage, higher than normal error rate, etc)
                                                   6 = Information messages
                                                   7 = Debug level messages (e.g. for temporary or debug-mode messages)
                                                   8 = show all messages
                                               */
   uint32_t reserved1 : 16;
   uint32_t uLoopbackLevel : 16;               /* loopback level:  0 = none, 1 = buffer, 2 = packet, 3 = payload, 4 = transcode, 5 = transcode + procesing algorithms, default 0, 5 == 0, 9 == 1.5  */
   uint32_t uDebugMode;                        /* options for enabling/disabling various code; use with debug_mode enum */
   uint32_t uAMRPackingFormat : 16;            /* packing format override for use with AMR codecs; use packing_format enum */
   uint32_t uLowLevelMemTest : 8;              /* low level external memory test; 0 = disable, 1 = enable, default 0 */
   uint32_t reserved2 : 8;
   uint32_t reserved3;
#endif

#ifdef _SIGRT
   uint8_t uDisableMismatchLog;
   uint8_t uDisableConvertFsLog;
   uint32_t uEventLogMode;          /* EVENT_LOG_MODE enums above, including log to screen, file, or both */
   FILE* uEventLogFile;
#ifdef _X86
   #define MAX_EVENT_LOG_PATHNAME_LEN 256
   char szEventLogFilePath[MAX_EVENT_LOG_PATHNAME_LEN];  /* event log path name, if not empty diaglib creates an event log using uLogMode definitions, JHB Dec 2019 */
   uint32_t uEventLog_fflush_size;  /* if set non-zero, specifies number of bytes of event log file growth before flushing.  Some Linux and/or devices may have very large buffer sizes so flushing may help keep log files updated more often */ 
   uint64_t uEventLog_max_size;     /* if set non-zero, limits event log max size (in bytes) */
   uint8_t uPrintfControl;          /* control how packet/media thread screen output is handled -- 0 = non-buffered I/O, 1 = stdout (line buffered I/O), 2 = stderr (per character I/O).  Added JHB Aug 2018 */
   uint8_t uPrintfLevel;            /* sets level for  packet/media thread sig_print() API; levels include PRN_LEVEL_INFO, PRN_LEVEL_STATS, PRN_LEVEL_WARNING, PRN_LEVEL_ERROR, and PRN_LEVEL_NONE defined in packet_flow_media_proc.c. mediaMin 'o' interactive kbd cmd uses this to toggle p/m thread screen output */
   uint8_t uPktStatsLogging;        /* enable packet logging, see packet_stats_logging enums above */
   uint8_t uEnablePktTracing;       /* packet tracing with timestamps.  0 = disabled. To enable, see DS_PACKET_TRACE_XX flags above.  Should *only* be enabled for debug purposes, as it severely impacts performance */
   uint8_t uEnableDataObjectStats;  /* session, channel, codec instance stats, including min amount of free handles.  Has small but significant impact on session and dynamic channel creation performance, should only be enabled for measurement/debug purposes */
   uint32_t uPushPacketsElapsedTimeAlarm;  /* if DSPushPackets() is not called for this amount of time, a warning will show in the event log (in msec).  The DS_ENABLE_PUSHPACKETS_ELAPSED_TIME_ALARM flag (uDebugMode) must be set */ 

   uint32_t uReserved1;
   uint32_t uReserved2;
   uint32_t uReserved3;
   uint32_t uReserved4;
   uint32_t uReserved5;
   uint32_t uReserved6;
   uint32_t uReserved7;
   uint32_t uReserved8;
   uint32_t uReserved9;
#endif
#endif

} DEBUG_CONFIG;

#endif /* _CONFIG_H_ */
