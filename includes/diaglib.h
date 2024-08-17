/*
 $Header: /root/Signalogic/DirectCore/include/diaglib.h
 
 Copyright Signalogic Inc. 2016-2024

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  Event logging and packet logging library, including packet stats history and logging APIs, status and error code APIs, memory diagnostics, and more
 
 Projects

  SigSRF, DirectCore
 
 Documentation

 https://github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediamin

 Older documentation links:
  
  after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

  before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

 Revision History
  
  Created Aug 2017 Jeff Brower, based on original c66x diagnostics APIs, created Oct-Dec 2016
  Modified Sep 2017 JHB, added DSFindSSRCGroups() API
  Modified Sep 2017 CKJ, added DSGetAPIStatus() API
  Modified Sep 2017 JHB, added DS_PKTSTATS_LOG_MARK_DTMF_DUPLICATE flag
  Modified Mar 2018 JHB, moved DIAGLIB_VERSION global var inside extern "C" (https://stackoverflow.com/questions/38141390/extern-and-extern-c-for-variables)
  Modified Jul 2018 CKJ, add constants used by Log_RT() (and LOG_OUTPUT define in example apps)
  Modified Feb 2019 JHB, add LOG_SET_API_CODES flag, removed HSESSION param from DSPktStatsAddEntries()
  Modified Aug 2019 JHB, remove include for session.h
  Modified Oct 2019 JHB, add DS_PKTSTATS_LOG_EXTENDED_RTP_SEQNUMS flag
  Modified Dec 2019 JHB, add STREAM_STATS struct, modify DSPktStatsLogSeqnums() to return stats info in a STREAM_STATS struct ptr
  Modified Dec 2019 JHB, add hSession and idx to PKT_STATS struct, see comments
  Modified Jan 2020 JHB, implement DS_PKTSTATS_LOG_SHOW_WRAPPED_SEQNUMS flag
  Modified Feb 2020 JHB, add DS_PKTSTATS_LOG_EVENT_LOG_SUMMARY flag
  Modified Apr 2020 JHB, add DSGetLogTimeStamp() API. Initially this is used in mediaMin interactive keyboard debug info printouts, to make it easy to see uptime of ongoing tests that are not printing onscreen log output
  Modified May 2020 JHB, in STREAM_STATS struct, rename numRepair to numSIDRepair and add numMediaRepair
  Modified Jan 2021 JHB, change loglevel param in Log_RT() from uint16_t to uint32_t
  Modified Dec 2022 JHB, add DSInitLogging() and DSCloseLogging() APIs to simplify application and multithread interface to diaglib
  Modified Jan 2023 JHB, per-thread support for DSGetAPIStatus()
  Modified Jan 2023 JHB, implement DSConfigLogging() to allow apps to (i) configure packet logging and (ii) set/retrieve mid-operation data and flags. One example is ability to abort DSPktStatsWriteLogFile() and other potentially time-consuming APIs
  Modified Feb 2024 JHB, add optional user-specified time value to DSGetLogTimeStamp(), which will use this value instead of wallclock time or uptime if DS_LOG_LEVEL_USER_TIMEVAL flag is given
  Modified Feb 2024 JHB, add DSGetMD5Sum() API
  Modified Mar 2024 JHB, change default LOG_SCREEN_FILE definition to zero, add number of chars written return value to Log_RT(). Part of continuing effort to make event logging and Log_RT() easier to use for a wide range of applications
  Modified Mar 2024 JHB, remove alias.h include
  Modified Apr 2024 JHB, deprecate DS_LOG_LEVEL_UPTIME_TIMESTAMP flag and DS_EVENT_LOG_UPTIME_TIMESTAMPS (shared_include/config.h) flags, the default (no flag) is now uptime timestamps
  Modified Apr 2024 JHB, DSGetLogTimestamp() now returns length of timestamp string
  Modified Apr 2024 JHB, add numMediaReuse to STREAM_STATS struct
  Modified May 2024 JHB, add DSGetBacktrace() API
  Modified May 2024 JHB, update documentation for DSPktStatsAddEntries()
  Modified Jul 2024 JHB, add DS_PKTSTATS_ORGANIZE_COMBINE_SSRC_CHNUM flag to support SSRCs shared across streams. mediaMin sets this flag when dormant session detection is disabled on its command line
  Modified Jul 2024 JHB, due to documentation review, uFlags moved to second param in all relevant APIs
  Modified Jul 2024 JHB, rename SSRC fields in PKT_STATS struct to ssrc. Maintain consistency with other header file structs
  Modified Aug 2024 JHB, rename DS_PKTSTATS_ORGANIZE_COMBINE_SSRC_CHNUM flag to DS_PKTSTATS_MATCH_CHNUM
  Modified Aug 2024 JHB, rename DSGetMD5Sum() to DSConsoleCommand() and make into generic console command execution API. See comments
*/

#ifndef _DIAGLIB_H_
#define _DIAGLIB_H_

#include <stdio.h>  /* FILE* definition */

#include "shared_include/config.h"  /* config.h provides DEBUG_CONFIG struct definition used in DSInitLogging() and DSConfigLogging(); only includes Linux headers */

#ifdef __cplusplus
extern "C" {
#endif

/* diaglib version string (declared in diaglib.cpp) */

extern const char DIAGLIB_VERSION[256];

/* event logging APIs */

int Log_RT(uint32_t loglevel, const char *fmt, ...);  /* loglevel can be combined with DS_LOG_LEVEL_xxx flags, and also DS_EVENT_LOG__XXX_TIMESTAMPS flags, defined in shared_include/config.h */

#if 0 /* flag deprecated, the default (no flag) is now uptime timestamps. DS_LOG_LEVEL_NO_TIMESTAMP can be combined with log_level (i.e. Log_RT(log_level, ...) to specify no timestamp, JHB Apr 2024 */
#define DS_LOG_LEVEL_UPTIME_TIMESTAMP        DS_EVENT_LOG_UPTIME_TIMESTAMPS
#else
#define DS_LOG_LEVEL_UPTIME_TIMESTAMP        0  /* still available for readability purposes, but doesn't do anything */
#endif
#define DS_LOG_LEVEL_WALLCLOCK_TIMESTAMP     DS_EVENT_LOG_WALLCLOCK_TIMESTAMPS
#define DS_LOG_LEVEL_USER_TIMEVAL            DS_EVENT_LOG_USER_TIMEVAL
#define DS_LOG_LEVEL_TIMEVAL_PRECISE         DS_EVENT_LOG_TIMEVAL_PRECISE

int DSInitLogging(DEBUG_CONFIG* dbg_cfg, unsigned int uFlags);  /* initialize event logging. Note that DSInitLogging() should not be called twice without a matching DSCloseLogging() call, as it increments a semaphore count to track multithread usage */

unsigned int DSConfigLogging(unsigned int action, unsigned int uFlags, DEBUG_CONFIG* pDebugConfig);  /* configure event logging */

/* actions */

#define DS_CONFIG_LOGGING_ACTION_SET_FLAG               1  /* set one or more flags */
#define DS_CONFIG_LOGGING_ACTION_CLEAR_FLAG             2  /* clear one or more flags */
#define DS_CONFIG_LOGGING_ACTION_SET_UFLAGS             3  /* set all flags */
#define DS_CONFIG_LOGGING_ACTION_GET_UFLAGS             4  /* get all flags */ 
#define DS_CONFIG_LOGGING_ACTION_SET_DEBUG_CONFIG       5  /* update lib_dbg_cfg (event logging) */

#define DS_CONFIG_LOGGING_ACTION_MASK                0xff

/* uFlags */

#define DS_CONFIG_LOGGING_ALL_THREADS               0x100  /* apply set/clear action to all currently active threads */

#define DS_CONFIG_LOGGING_PKTLOG_ABORT             0x1000  /* set this flag if for any reason it's necessary to abort DSPktStatsWriteLogFile() or other packet logging APIs with potentially long processing times. To be effective, DSConfigLogging() should be called from a thread separate from one calling packet logging APIs, JHB Jan 2023 */

/* DSGetAPIStatus retrieves API status and/or error and warning conditions.  Notes:

  1) If used, it should be called immediately after the API is called, for example if the API returns -1
  
  2) In some cases an API may return zero for a warning or "not complete an error" situation.  One example is DSBufferPackets(), which may return zero for benign reasons (e.g. a random packet that doesn't match
     any defined sessions) or due to problems in adding one packet while other packets were added successfully.  DSGetAPIStatus() can also be used in these cases.  In addition see DSGetSessionStatus() in pktlib.c

  3) API identifiers defined below may be combined.  For example a code might be returned that identifies both DSBufferPackets() and an internal API such as validate_rtp()
*/

int DSCloseLogging(unsigned int uFlags);  /* close event logging (decrement usage count and close event log file if zero) */

FILE* DSGetEventLogFileHandle(unsigned int uFlags);

int DSGetAPIStatus(unsigned int uFlags);

/* error / warning codes and API identifiers returned by DSGetAPIStatus() */

#define DS_API_STATUS_CODE_ERROR                     0x01
#define DS_API_STATUS_CODE_WARNING                   0x02

/* API identifiers for published APIs */

#define DS_API_CODE_CREATESESSION                   0x100
#define DS_API_CODE_DELETESESSION                   0x200
#define DS_API_CODE_BUFFERPKTS                      0x300
#define DS_API_CODE_GETORDEREDPKTS                  0x400
#define DS_API_CODE_GETPACKETINFO                   0x500
#define DS_API_CODE_GETSESSIONINFO                  0x600
#define DS_API_CODE_GETDTMFINFO                     0x700
#define DS_API_CODE_FORMATPACKET                    0x800
#define DS_API_CODE_STORESTREAMDATA                 0x900
#define DS_API_CODE_GETSTREAMDATA                   0xa00

/* identifiers for internal APIs -- these may be combined with identifers returned by published APIs */

#define DS_API_CODE_VALIDATERTP                   0x10000
#define DS_API_CODE_GETCHANPACKETS                0x20000
#define DS_API_CODE_CREATEDYNAMICCHAN             0x40000


/* packet logging API and definitions */

#define MAX_SSRCS  65536L  /* maximum number of SSRCs (unique packet flows) that can be handled by DSPktStatXxx APIs */

/* configuration flags used by Log_RT(). These are set in uEventLogMode element of a DEBUG_CONFIG struct (shared_include/config.h), which is an input param to DSConfigPktlib() and other DSConfigXX APIs. Additional uEventLogMode flags are defined as EVENT_LOG_MODE enums in shared_include/config.h */

#define LOG_SCREEN_FILE       0  /* default is both event log (file) and screen. For example, if a DEBUG_CONFIG struct is created, initialized to zero, passed to a DSConfigXX API, then LOG_SCREEN_FILE is in effect */
#define LOG_SCREEN_ONLY       1
#define LOG_FILE_ONLY         2

#define LOG_SET_API_STATUS    0x10   /* can be combined (OR'd) with uEventLogMode settings */
#define LOG_MODE_MASK         0x0f

/* packet log entry struct */

typedef struct __attribute__((packed)) {

   uint16_t rtp_seqnum;
   uint32_t rtp_timestamp;
   uint32_t rtp_ssrc;
   uint16_t rtp_pyldlen;
   uint32_t content_flags;  /* one of the DS_PKT_PYLD_CONTENT_xxx (payload content) flags */
   int16_t  chnum;          /* optional channel (session) info, set by pktlib p/m threads. Set to -1 if not used */
   int16_t  idx;            /* optional stream group info,  "  "  */

} PKT_STATS;

#define MAX_CHAN_PER_SSRC 8

/* streams stat info struct output by DSPktStatsLogSeqnums() */

typedef struct {

   uint32_t ooo_seqnum;
   uint32_t dup_seqnum;
   uint32_t missing_seqnum;
   uint32_t max_consec_missing_seqnum;
   uint32_t ooo_max;
   uint32_t numSID;
   uint32_t numSIDReuse;
   uint32_t numSIDRepair;
   uint32_t numMediaRepair;
   uint32_t numMediaReuse;
   uint32_t numDTMFEvent;
   int16_t  chnum[MAX_CHAN_PER_SSRC];
   int16_t  num_chnum;
   int16_t  idx;

} STREAM_STATS;


/* DSPktStatsAddEntries() adds one or more packet stats entries. Notes:

  -pkt_stats should point to a PKT_STATS struct
  -pkt_buffer should contain num_pkts packets, each with a pkt_length[]. One or more pkt_length[] can be -1 if not known, or if all pkt_length[] are unknown pkt_length can be given as NULL
  -uFlags should contain one of the DS_BUFFER_PKT_xxx flags and optionally one or more of the "general API flags" defined in pktlib.h
  -payload_content[] may optionally specify one of the DS_PKT_PYLD_CONTENT_xxx flags defined in pktlib.h for one or more packets
  -return value is the number of packets added to pkt_stats, or -1 for an error condition

  -see mediaTest/packet_flow_media_proc.c for tested examples of API usage and PKT_STATS declaration

  -if DSGetPacketInfo() does not exist in the build (i.e. the build does not link the pktlib library) then DSPktStatsAddEntries() will return -2
*/

int DSPktStatsAddEntries(PKT_STATS* pkt_stats, unsigned int uFlags, int num_pkts, uint8_t* pkt_buffer, int pkt_length[], unsigned int payload_content[]);


/* DSFindSSRCGroups() find SSRC groups and returns start/end packet indexes and sequence numbers for each group */

int DSFindSSRCGroups(PKT_STATS*, unsigned int uFlags, int num_pkts, uint32_t ssrcs[], uint16_t chnum[], int first_pkt_idx[], int last_pkt_idx[], uint32_t first_rtp_seqnum[], uint32_t last_rtp_seqnum[]);

/* DSPktStatsLogSeqnums() gathers stats for, and logs to file (if fp_log is non-NULL), a collection of packet stats entries.  DSPktStatsLogSeqnums() does the following:

  1) Organizes packets by stream (SSRC), and if DS_PKTSTATS_MATCH_CHNUM is given, then also by channel number

  2) Checks for packets lost, ooo, duplicated, and dropped (by jitter buffer)

  3) If logging to file is requested, lists packets by sequence number with columns including problems (out-of-order, missing) and attributes including timestamp, payload length, and payload contents (SID, media, etc)
  
  4) Returns number of streams (SSRCs) found. The following arrays are returned (each indexed from 0 .. num_ssrcs-1):

    -ssrcs[], containing SSRC values
    -first/last pkt_idx[], containing index into pkts[] for first and last packet for each ssrc
    -first/last rtp_seqnum[], containing first and last sequence number for each ssrc

  5) Returns per-stream stats, see STREAM_STATS struct above. Each stream has a unique SSRC and/or channel number
*/

int DSPktStatsLogSeqnums(FILE* fp_log, unsigned int uFlags, PKT_STATS* pkts, int num_pkts, char* label, uint32_t ssrcs[], uint16_t chnum[], int first_pkt_idx[], int last_pkt_idx[], uint32_t first_rtp_seqnum[], uint32_t last_rtp_seqnum[], STREAM_STATS StreamStats[]);


/* DSPktStatsWriteLogFile() writes packet stats (previously added to PKT_STATS structs by DSPktStatsAddEntries) to a log file. Notes:

  1) Writes packet log in 3 sections

    a) input packet stats, if num_input_pkts in the PKT_COUNTERS struct is non-zero. Calls DSPktStatsLogSeqnums() to perform input packet stats logging
    b) jitter buffer output packet stats, if num_pulled_pkts is non-zero. Calls DSPktStatsLogSeqnums() to perform output packet stats logging
    c) input vs output analysis, if pkt_counters->num_input_pkts and pkt_counters->num_output_pkts are both > 0

    The input and output sections show nop (no packet, or lost on transmission) and ooo (out-of-order; ideally the output section will show no ooo). The analysis section:

      -shows dropped by jitter buffer or duplicated
      -traces packets from input to output by matching both sequence numbers and timestamps. Any mismatched timestamps are highlighted

  2) If DS_PKTSTATS_LOG_COLLATE_STREAMS is given, input and output packets are grouped together by stream (SSRC), and if DS_PKTSTATS_MATCH_CHNUM is given, then also by channel number. Normally collating streams is recommended; see notes below on when and when not to collate

  3) Printed output may include additional grouping by stream group or by channel number (i.e. DS_PKTSTATS_ORGANIZE_BY_xxx flags), but this is for formatting purposes and does not affect internal sorting and analysis

  4) Each printed packet group includes a stats prologue and summary

  5) Use the DS_PKTSTATS_LOG_APPEND flag to add entries to an existing log file
*/

#ifdef __cplusplus  /* if gcc is compiling this, we can't use struct PKT_COUNTERS* forward declaration with (also there is not a compiler cmd line flag to suppress the "declared inside parameter list" warning https://stackoverflow.com/questions/47782307/makefile-whats-the-name-of-this-warning) */

int DSPktStatsWriteLogFile(const char* szLogFilename, unsigned int uFlags, PKT_STATS* pInputPkts, PKT_STATS* pOutputPkts, struct PKT_COUNTERS* pPktCounters);
#endif

/* packet counters struct input to DSPktStatsWriteLogFile() describing packet stats pointed to by pInputPkts and pOutputPkts */

typedef struct PKT_COUNTERS {

   uint32_t num_input_pkts;
   uint32_t num_pulled_pkts;
   uint32_t pkt_input_cnt;
   uint32_t pkt_read_cnt;
   uint32_t pkt_submit_to_jb_cnt;
   uint32_t pkt_add_to_jb_cnt;
   uint32_t pkt_write_cnt;
   uint32_t pkt_output_cnt;
   uint32_t frame_write_cnt;

} PKT_COUNTERS;

#ifndef __cplusplus  /* for gcc, define the function after PKT_COUNTERS struct definition */
int DSPktStatsWriteLogFile(const char* szLogFilename, unsigned int uFlags, PKT_STATS* pInputPkts, PKT_STATS* pOutputPkts, struct PKT_COUNTERS* pPktCounters);
#endif

/* flags for DSPktStatsWriteLogFile() -- note these flags can be combined with DS_WRITE_PKT_STATS_HISTORY_xx flags for DSWritePacketStatsHistoryLog() API in pktlib.h */

#define DS_PKTSTATS_LOG_PACKETMODE                   0x01
#define DS_PKTSTATS_LOG_FRAMEMODE                    0x02  /* Use this flag if input entries were added in frame mode; i.e. no buffering is used, no output network or pcap output is used */
#define DS_PKTSTATS_LOG_APPEND                       0x04
#define DS_PKTSTATS_LOG_COLLATE_STREAMS              0x08  /* Applies to DSPktStatsWriteLogFile(), DSPktStatsLogSeqnums(), and DSFindSSRCGroups().  Notes:

                                                              -collates streams so that entries are grouped by SSRC number. Entry sorting is done in place; i.e. contents of the PKT_STATS* pointer arg are modified
                                                              -this flag will work with dynamically created streams (RFC8108), but if there are stream interleaving or other issues that need to be viewed or debugged, then collation should possibly not be applied
                                                           */

#define DS_PKTSTATS_LOG_MARK_DTMF_DUPLICATE          0x10  /* DTMF packets are not normally included in duplicated packet counts as RFC4733 allows for sequence numbers and timestamps to be duplicated. To mark these as duplicates use this flag */
#define DS_PKTSTATS_LOG_SHOW_WRAPPED_SEQNUMS         0x20  /* show RTP sequence numbers with wrapping (i.e. show "extended sequence numbers"). Typically this makes it harder to detect missing and ooo packets. The default is to show sequence numbers without wrapping, for example the sequence 65534, 65535, 0, 1 becomes 65534, 65535, 65536, 65537.  For spreadsheet analysis and other packet math, this can be helpful */
#define DS_PKTSTATS_LOG_EVENT_LOG_SUMMARY            0x40  /* print to event log a brief summary for each stream analyzed */

#define DS_PKTSTATS_LOG_LIST_ALL_INPUT_PKTS         0x100  /* initially print all input packets with no grouping, ooo detection, or other labeling. This will greatly increase the size of the packet log file, and should only be used for debug situations */
#define DS_PKTSTATS_LOG_LIST_ALL_PULLED_PKTS        0x200  /* print all buffer output packets,  "  "  */
#define DS_PKTSTATS_LOG_RFC7198_DEBUG              0x1000

/* DSPktStatsWriteLogFile() pkt stats organization flags:  can be combined, organize by SSRC is default if no flag specified */

#define DS_PKTSTATS_ORGANIZE_BY_SSRC             0x100000  /* organize analysis and stats by SSRC */
#define DS_PKTSTATS_ORGANIZE_BY_CHNUM            0x200000  /* ... by channel number */
#define DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP      0x400000  /* ... by stream group */

/* the DS_PKTSTATS_MATCH_CHNUM flag specifies that during packet sort and analysis, both stream (SSRC) and channel number will be used to match packets. Notes:

  -this flag helps generate clean packet logs when the same stream (i.e. same SSRC) is captured at multiple points but with different timestamp and sequence number content. For example a lawful intelligence application might capture the same stream at an endpoint and also at an intermediate transit point
  -mediaMin sets this flag when its cmd line -dN options include DISABLE_DORMANT_SESSION_DETECTION
  -the default (no flag) is to use only SSRC to group packets
  -can be combined with DS_PKTSTATS_ORGANIZE_xxx flags
*/
  
#define DS_PKTSTATS_MATCH_CHNUM                0x40000000  /* note - value should not overlap DS_PKT_STATS_HISTORY_LOG_xxx flags in pktlib.h */


/* diaglib utility APIs */

int DSGetLogTimeStamp(char* timestamp, unsigned int uFlags, int max_str_len, uint64_t user_timeval);

int DSGetAPIStatus(unsigned int uFlags);  /* get per-thread API status */

/* DSConsoleCommand() calls glibc popen() to execute console command and return result:

   -szCmd should point to command
   -szArgs should point to command arguments, for example a pathname and options
   -szResult should point to buffer that will contain command result
   -num_results specifies the number of command results to return. For example if the command prints 2 results and num_results is 1, only the first result is returned. Results are separated by a space in szResult
   -max_result_len should specify maximum buffer length for szResult. max_result_len is divided equally by num_results

   -returns number of command results on success, 0 if a parameter is not an error but causes nothing to happen (e.g. num_results or max_str_len is zero, command returns no results), and negative values on error conditions
*/

int DSConsoleCommand(const char* szCmd, const char* szArgs, char* szResult, int num_results, int max_result_len);

/* DSGetBacktrace() calls glibc backtrace() to retrieve and format current call stack:

  -nLevels specifies number of call stack levels
  -uFlags options are given below
  -szBacktrace should point to buffer containing call stack info. backtrace() output is formatted and cleaned into one long string; repeats if any are removed
  
  -returns negative values on error conditions
*/
  
int DSGetBacktrace(int nLevels, unsigned int uFlags, char* szBacktrace);

#define DS_GETBACKTRACE_INSERT_MARKER                   1  /* insert "backtrace: " marker at start of return string */
#define DS_GETBACKTRACE_INCLUDE_GLIBC_FUNCS             2  /* include "self" glibc functions (e.g. lib.so.N, libpthread.so, etc). Default is these are omitted */

#ifdef __cplusplus
}
#endif

#endif   /* _DIAGLIB_H_ */
