/*
 $Header: /root/Signalogic/apps/mediaTest/packet_flow_media_proc.c

 Copyright (C) Signalogic Inc. 2017-2020

 Description

   Source code for "packet / media thread" packet and media processing

 Purposes

   1) Provide multithread capability in SigSRF software, enabling fully concurrent multiple packet streams, codecs, and jitter buffers

   2) Per thread, implement SigSRF library APIs to provide packet flow/media processing functionality, test, and measurement, and transcoding, including:

      -multiple RFC compliant packet flow, advanced jitter buffer, DTX handling, DTMF event handling, multichannel packets, ptime conversion, and more
      -measurements, including:
        -x86 server performance
        -verify bitexactness for codecs, measure audio quality.  Interoperate at encoded bitstream level with 3GPP test vectors and reference codes
        -packet loss and other packet statistics

   3) Implement push/pull packet queues to allow applications, including mediaMin, to use only a small, minimal subset of the SigSRF API

      -support stress and capacity testing by applications by providing a cmd line option to open multiple mediaMin app threads
      -support UDP network socket packet data flow both directly and via push/pull queues

   4) Implement insertion points for application and user-defined signal processing and deep learning

   5) Act as key source code component of the SigSRF SDK, providing API source code examples, including correct transcoding data flow and API usage for SigSRF libs including pktlib, voplib, streamlib, diaglib, and aviolib

   6) Provide basis for limited, demo/eval version available on Github

 Revision History

   Created Jul 2018 JHB, separated packet mode processing section from x86_mediaTest.c.  See revision history in x86_mediaTest.c

   Modified Jun CKJ 2017, added support for simultaneously using input pcap files of different link layer types 
                          added support for ipv6 in pcap extract mode, added check to exit cleanly if internet layer is not ipv4 or ipv6
   Modified Jun 2017 JHB, added pcap to wav support, single exit point for success + most errors in pcap extract mode
   Modified Jun 2017 JHB, added pcap and jitter buffer stats logging
   Modified Jun 2017 JHB, added frame mode option for pcap input (see "packet mode" and "frame mode" comments)
   Modified Jul 2017 JHB, modified packet info log to group data by SSRC.  Currently a max of 8 unique SSRCs are supported in one session
   Modified Jul 2017 JHB, added multithread operation test code (see ENABLE_MULTITHREAD_OPERATION)
   Modified Jul 2017 JHB, added DSOpenPcap(), DSReadPcapRecord(), and DSWritePcapRecord() APIs to consolidate and reduce pcap file handling code
   Modified Jul 2017 CKJ, moved pcap file related structs and functions to pktlib
   Modified Jul 2017 JHB, added example showing use of DS_GETORD_PKT_RETURN_ALL_DELIVERABLE flag for multiple SSRC streams within the same session.  The flag is applied briefly to force remaining packets for the current stream out of the buffer when a new stream starts
   Modified Aug 2017 JHB, fixed problem in .cod to .wav conversion (codec test mode) where varying frame sizes were not handled correctly
   Modified Aug 2017 CKJ, added mode to test / demonstrate background process (see ALLOW_BACKGROUND_PROCESS and use_bkgnd_process)
   Modified Aug 2017 JHB, added DTX handling example, using the termN.flags field in session config files to enable/disable.  Default is enabled (flags has TERM_DTX_ENABLED set).  If not enabled a one-time message is printed when mediaTest runs.  If enabled the packet stats log file will show both SID and "SID Reuse" packets in the output stream
   Modified Sep 2017 CKJ, added DTMF handling example per RFC4733, including DTMF event processing and output packet forwarding.  In the packet stats log, look for packets labeled "DTMF Event"
   Modified Oct 2017 CKJ, added variable ptime handling and use of DSStoreStreamData() and DSGetStreamData() APIs.  Decoupled stream data processing from ordered buffering, highlight user-defined media data processing insertion point
   Modified Jun 2018 CKJ, add audio stream merging; see stream group API usage (such as DSAttachStreamToGroup) and code at "media processing insertion point"
   Modified Jul 2018 CKJ, change LIB_LOG_TO_FILE define to general case LOG_OUTPUT and use the Log_RT() file + screen output constants defined in diaglib.h
   Modified Jul 2018 CKJ, modify get_wav_index() to select correct channel from current session (when multiple sessions are active)
   Modified Jul 2018 JHB, create GetMergeInfo() and DisableStreamMerging() local functions and otherwise simplify and reduce stream merging source code example
   Modified Jul 2018 JHB, add references to executionMode to handle differences in command line vs. thread execution.  Add calls to DSRecvPackets() and DSSendPackets() for thread execution
   Modified Jul 2018 JHB, implement changes that make packet processing less dependent on input stream index, which allows packets to arrive in any order from any stream.  These include:
                            -modify CheckForSSRCChange() to use packet's channel number instead of stream index to keep track of SSRC changes (chan number is determined by packet hashing, parent channel is used for dynamic channels)
                            -a few places where channel number can be used for indexing instead of stream number
   Modified Aug 2018 JHB, modifications to allow this file to be included in pklib build, in order to support the DSPushPackets() and DSPullPackets() minimum API interface with packet_flow_media_proc() running as a thread.  These include
                            -eliminate warnings in the pktlib build which uses -Wall
                            -use of __LIBRARYMODE__ for a few sections of code that not be included in .so build
   Modified Aug 2018 JHB, fix merged output pcap timestamp problem (see merge_timestamp)
   Modified Aug 2018 JHB, in pktlib thread build (__LIBRARYMODE__ defined), eliminate I/O related code and most global vars
   Modified Aug 2018 JHB, add initial support for on-the-fly session create/delete, including InitSession() and InitStream() functions
   Modified Aug 2018 JHB, add sub loop for cases where DSRecvPackets() returns more than one packet
   Modified Sep 2018 JHB, implement "master buffer" scheme for stream audio merging.  This allows stream contributions at unequal and varying rates.  The default "allowable gap" for lagging streams is 1/4 sec, but can be set up to 8 sec via DS_SESSION_INFO_MERGE_BUFFER_SIZE
   Modified Sep 2018 JHB, further refinement to stream audio merging (i) ensure exact time-alignment and start offset of all streams, and (ii) fill in missing contributions with audio zeros under certain conditions
   Modified Sep 2018 JHB, debug and test multiple merge groups.  Add merge group initialization to InitSession()
   Modified Oct 2018 JHB, implement multiple packet/media threads.  Modifications / optimizations include thread_index initialization, ManageSessions(), get_session_handle(), ThreadDebugOutput(), sig_printf(), isMasterThread references, etc
   Modified Oct 2018 JHB, additional error checking for cases when asynchronous pkt/media threads may be concurrently processing same stream groups
   Modified Oct 2018 JHB, optimize usage of DSGetStreamData(). This also fixed a crash case with multiple pkt/media threads "seeing" each other's active channels.  See USE_CHAN_NUMS define and associated comments
   Modified Nov 2018 JHB, move merging related codes to streamlib.so (streamlib.c).  streamlib API calls here include DSInitMergeGroup() and DSMergeStreamGroupContributors()
   Modified Nov 2018 JHB, add CheckForDormantSSRC() to detect and flush channels with dormant SSRCs that are "taken over" by another channel, for example due to call-waiting or on-hold situations
   Modified Dec 2018 JHB, add packet/media thread energy saver mode (see fThreadInputActive and fThreadOutputActive flags and comments below about CPU usage)
   Modified Dec 2018 JHB, improve ThreadDebugOutput() to show group info for any thread sessions attached to a group.  Notes:
                          -this is needed to see exact effects of the STREAM_GROUP_WHOLE_THREAD_ALLOCATE and DS_CONFIG_MEDIASERVICE_ROUND_ROBIN flags (streamlib.h and pktlib.h)
                          -correct whole group allocation requires STREAM_GROUP_WHOLE_THREAD_ALLOCATE to be added to the term1 group mode field at session creation time, and DS_CONFIG_MEDIASERVICE_ROUND_ROBIN flag to be set when creating a packet/media thread. Without the round robin flag there can still be some groups split across threads
                          -see the nomenclature comments in ThreadDebugOutput() to interpret the screen output
   Modified Dec 2018 JHB, more info printed in ThreadDebugOutput(), including some system wide stats
   Modified Feb 2019 JHB, add DS_GETORD_PKT_ENABLE_SID_REPAIR usage.  Applying this flag enables "CNG fill-in" when damage to long silence durations is detected (audio that contains long SID periods can be significantly shortened/mis-aligned by packet loss.  Test with 13572.0 pcap)
   Modified Feb 2019 JHB, add RecordPacketTimeStats() to keep track of packet timestamp and RTP timestamp duration (both before and after jitter buffer)
   Modified Feb 2019 JHB, add CheckForPacketLossFlush() for packet loss mitigation (monitors jitter buffer levels for parent and child channels).  See max_loss_ptimes in TERMINATION_INFO struct (shared_include/session.h)
   Modified Mar 2019 JHB, add packet time and loss stats.  See PACKET_TIME_STATS define, see also fPacketTimeStatsEnabled and fPacketLossStatsEnabled in PACKETMEDIATHREADINFO struct and DS_CONFIG_MEDIASERVICE_EN/DISABLE_THREAD_PACKET_TIME/LOSS_STATS flags (pktlib.h)
   Modified Mar 2019 JHB, improve profiling, give each category its own independent running sum index and increase running sum length to 16 (THREAD_STATS_TIME_MOVING_AVG defined in pktlib.h)
   Modified May 2019 JHB, when acting on payload content type returned by DSGetOrderedPackets(), check for both DS_PKT_PYLD_CONTENT_DTMF_SESSION (indicating a match to a session-defined DTMF payload type), and for generic DS_PKT_PYLD_CONTENT_DTMF content type
   Modified Aug 2019 JHB, add extern C to definition of functions used externally (sig_printf, ResetPktStats), limit number of channels that can be tracked in profiling (see MAX_CHAN_TRACKED below), add an error string param to ThreadAbort()
   Modified Sep 2019 JHB, fix incorrect nDormantChanFlush[] indexing and one case of nOnHoldChanFlush[] incorrect indexing:  session + term based, not channel
   Modified Sep 2019 JHB, remove sig_sprintf() function, no longer used
   Modified Sep 2019 JHB, change include folder for udp.h and ip.h from "linux" to "netinet" to fix -Wodr (one definition rule) warning with gcc 5.4.  Remove if_ether.h and arpa/inet.h includes (already in pktlib.h)
   Modified Oct 2019 JHB, implement pastdue in CheckForPacketLossFlush()
   Modified Oct 2019 JHB, act on STREAM_GROUP_OVERFLOW_STOP_GROUP_ON_DETECTION flag (one of overrun compensation related flags added to shared_include/streamlib.h). More detailed info in warning message if DSStoreStreamGroupContributorData() returns -1
   Modified Dec 2019 JHB, make pkt_counters[] global (needed in packet.c), include chnum and idx information in PKT_STATS collection (see DSGetStreamGroupInfo() with DS_GETGROUPINFO_HANDLE_CHNUM flag)
   Modified Dec 2019 JHB, implement DSWritePacketStatsHistoryLog() and DSLogPacketTimeLossStats() APIs.  The latter collates, formats, and prints run-time packet stats to the event log
   Modified Jan 2020 JHB, use DSGetStreamGroupInfo() when collecting packet history stats (PKT_STATS), ensure run-time packet stats are not logged twice for same session prior to deletion
   Modified Jan 2020 JHB, define SECONDARY_THREADS_DEPRECATED, which deprecates old "secondary thread" code.  Since early 2018, packet_flow_media_proc() is thread-safe and allows multiple packet/media "worker" threads.  Multiple app threads are implemented using mediaTest -Et ant -tN cmd line options, which invoke one or more mediaMin application threads
   Modified Jan 2020 JHB, call DSCreateFilelibThread() in p/m thread initialization.  See comments in filelib.h and filemgr.cpp
   Modified Jan 2020 JHB, implement p/m thread preemption alarm
   Modified Jan-Feb 2020 JHB, disallow past due flush if the channel's jb output is currently in SID state.  This avoids generation of possibly extra SID reuse packets and leaves the SID reuse responsibility completely with pktlib packet repair
   Modified Feb 2020 JHB, optimize jitter output packet processing loop and stream data loop (after variable ptime):  obtain decoder codec handle, termInfo, termInfo_link, etc only if chnum changes between loop iterations (see prev_chnum)
   Modified Feb 2020 JHB, improve p/m thread preemption alarm logic, add profile times and other stats in preemption event log messages
   Modified Mar 2020 JHB, fix dormant session countdown which was hardwired to a constant, but should be equal to jitter buffer target delay
   Modified Mar 2020 JHB, fix consecutive same-channel packet (channel burst) stat, which was not being recorded correctly
   Modified Mar 2020 JHB, move session flush check before session delete check in ManageSessions()
   Modified Mar 2020 JHB, implement TERM_EXPECT_BIDIRECTIONAL_TRAFFIC flag in receive queue handling, which reduces calls to DSRecvPackets() with DS_RECV_PKT_QUEUE_COPY (look ahead) flag set. This increases performance for unidirectional traffic (analytics mode)
   Modified Mar 2020 JHB, add case to pastdue processing for sessions not a stream group member
   Modified Apr 2020 JHB, move session_info_thread[] to pktlib, instead of being on the stack for each p/m thread. Remove session_info_thread[] param from several functions
   Modified Apr 2020 JHB, in telecom mode set last_pull_time[] only if DSGetOrderedPackets() returns non-zero, instead of each time it's called as in analytics mode
   Modified Apr 2020 JHB, modify pastdue processing to use instantaneous jitter buffer number of packets (see DS_JITTER_BUFFER_INFO_NUM_PKTS usage in pktlib.h, rtp_jubf.c)
   Modified Apr 2020 JHB, fix pkt_ptr increment bug in packet processing loop following DSGetOrderedPackets() (loop decodes and stores stream data). See comment near end of loop
   Modified Apr 2020 JHB, new support for telecom mode and hybrid analytics/telecom mode:
                          -DSGetOrderedPackets() post processing to maintain jitter buffer mem usage limits (see comments)
                          -rewrite CheckForDormantSSRC() to handle all modes and termN endpoints. This includes the addition of last_buffer_time[]
                          -new jitter buffer run-time stats added, including max packets, resyncs, and overrun buffer percent full
   Modified Apr 2020 JHB, fix problem with "average stats calc" run-time stat not being displayed correctly
   Modified May 2020 JHB, implement analytics mode improvements in CheckForPacketLossFlush(). Continue to support "analytics compatibility mode" by implementing a separate code path for it
   Modified May 2020 JHB, in ManageSessions() fix integer-out-of-range problem with state_clear_flags that was causing DSSetSessionInfo() with DS_SESSION_INFO_STATE flag to be called every time, even if session state flags had not changed (they change rarely)
   Modified May 2020 JHB, add pm_sync[] flag toggle in pm thread loop
   Modified May 2020 JHB, after call to DSGetOrderedPackets(), implement telecom mode packet loss flush, based on cumulative time vs. cumulative timestamp
   Modified May 2020 JHB, add fFirstGroupContribution[] to fix ptime msec wobble in first contribution to a stream group. See comments near DSMergeStreamGroupContributors()
   Modified Oct 2020 JHB, include codec type and bitrate in run-time stats session description info, other minor changes to run-time stats output. Codec info uses DSGetCodecInfo() API added to voplib
   Modified Oct 2020 JHB, clarify stream change/resume log info message, include new channel in the message
*/

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <netinet/ip.h>
#ifndef __LIBRARYMODE__
#include <netinet/udp.h>
#endif
#include <semaphore.h>
#include <limits.h>
#include <sched.h>
#include <sys/syscall.h>  /* SYS_gettid() */
#include "math.h"
#include "errno.h"  /* errno and strerror */

#include "mediaTest.h"  /* application level defines and vars, including items used by cmd_line_interface.c */

/* DirectCore and SigSRF lib header files (all libs are .so format) */

#include "shared_include/session.h"
#include "hwlib.h"
#include "voplib.h"
#include "pktlib.h"
#include "alglib.h"
#include "diaglib.h"
#include "shared_include/streamlib.h"

//#define DEBUG_CONSISTENCY_MARKERS
#ifdef DEBUG_CONSISTENCY_MARKERS
  #ifdef MEDIATEST
    #define _USE_CM_
    #define _MAVMOD_UAG_
    #define _SIGMOD_RTAFv5_
  #endif
  #include "../../DirectCore/lib/pktlib/call.h"
#endif

/* Network packet I/O notes:

  1) When USE_PKTLIB_NETIO (below) is defined, network I/O APIs in Pktlib are used, including DSRecvPackets() and DSSendPackets()

  2) When not defined, user application code is responsible for network I/O. In the #else and #ifndef USE_PKTLIB_NETIO sections of code below there are some examples of using socket I/O
     combined with other Pktlib APIs

  3) Typically the app has to run with raw socket access permissions; i.e. either run as root or a method involving setuid() and CAP_NET_RAW, setcap from the cmd line, or similar to set
     raw socket permissions (as shown here https://stackoverflow.com/questions/9772068/raw-socket-access-as-normal-user-on-linux-2-4)

  4) NONBLOCKING define below applies to both cases (either USE_PKTLIB_NETIO defined or not defined)
*/

#define USE_PKTLIB_NETIO


/* define used to control whether or not the socket used in packet test blocks
      if socket is blocking, packet processing depends on continually receiving packets 
      using nonblocking sockets is recommended. seriously, just don't use blocking sockets in this test app
*/

#define NONBLOCKING 1


/* Max channels supported for network packet test */
#define MT_MAX_CHAN 2
#define JB_DEPTH 7

/* number of possible input streams, including streams that are re-used for multithread and high capacity testing */
#define MAX_INPUT_STREAMS MAX_SESSIONS  /* max sessions defined in mediaTest.h */


/* Ptime notes:

  1) Per session ptime_config[] values are set to term1.ptime values in the specified session config file(s).  If not given in the session config file, the default value is 20 ms

  2) SESSION_DATA struct definitions support ptime multiples, specified as values that are integer multiples of the "natural frame size" of the codec
*/

#ifndef __LIBRARYMODE__
unsigned int ptime_config[MAX_SESSIONS] = { 20 };  /* in msec */
#endif

/* Frame interval notes:

  1) frameInterval[] values are the rates packets are buffered per input stream (in msec)

  2) If no command line entry is given for an input stream's buffer rate, the input's session definition ptime value is used.  If command line -rN entry is given then N is the interval
     value in msec, and overrides session definition ptime.  In the case of cmd line entry, values less than the ptime value in the input's session definition will cause the
     DS_GETORD_PKT_FTRT flag to be used (FTRT = faster-than-real-time operation).  A value of zero is the fastest possible buffering rate

  3) Print out for buffer rate is shown as "tps", or transactions per sec, to avoid confusion with bps (bits per sec)
*/

#ifndef __LIBRARYMODE__
extern unsigned int frameInterval[];
#endif

/* User Managed Sessions notes:

  1) mediaTest uses the ENABLE_MANAGED_SESSIONS define (below) to enable/disable user managed sessions.  The default is disabled

  2) Purpose is to allow sessions to be created with duplicated src/dst IP addr:port values.  This mode of operation requires careful "user management" of session handles,
     i.e. awareness at all times of of which sessions are associated with which incoming packets

  3) In user managed session mode, the user app is responsible for packet input, either socket I/O, pcap files, or other means, keeping track of which packets are associated
     with which sessions, and supplying a valid session handle to APIs such as DSBufferPackets() and DSGetOrderedPackets().  The DSRecvPackets() and DSSendPackets() APIs cannot be used

  4) If ENABLE_MANAGED_SESSIONS is not defined, pktlib expects unique (non duplicated) session definitions, and internally handles Rx packet-to-session matching

  5) For more info, see "session handle notes" comments below
*/

#define ENABLE_MANAGED_SESSIONS

/* Decouple packet and stream processing:

  1) Default is defined and recommended.  See notes below

  2) If not defined, packet and stream processing are combined in the same loop.  This may be slightly more efficient
     but loses the advantages of decoupled buffering and stream processing.  See notes below
*/

#define DECOUPLE_STREAM_PROCESSING


#ifndef __LIBRARYMODE__
  /* overwrite input file data ip/port info with session data info */
  #define OVERWRITE_INPUT_DATA


  /* re-use input file(s) for all sessions, should only be defined if OVERWRITE_INPUT_DATA or ENABLE_MANAGED_SESSIONS is also defined */
  #if defined(ENABLE_MANAGED_SESSIONS) || defined(OVERWRITE_INPUT_DATA)
  #define REUSE_INPUT_FILES
//  #define NEW_REUSE_CODE
  #define PERFORMANCE_MEASUREMENT
  #endif
#endif


/* Logging notes:

  1) SigSRF libraries use the Log_RT() API for logging.  Applications can use this API also (located in diaglib)

  2) mediaTest uses the LOG_OUTPUT define (below) to enable and manage Log_RT() output.  Default is LOG_SCREEN_ONLY

  2) The application is responsible for opening and closing a og file, and if so, then passing a log file handle within a DEBUG_CONFIG struct
     (uEventLogFile) and giving a pointer to the struct in DSConfigPktlib().  DEBUG_CONFIG is defined in config.h

  3) The log level can be controlled (uLogLevel within a DEBUG_CONFIG struct).  Log levels are defined in config.h (more or less they follow the Linux standard,
     e.g. http://man7.org/linux/man-pages/man2/syslog.2.html)
*/

//#define LOG_OUTPUT  LOG_SCREEN_ONLY  /* screen output only */
//#define LOG_OUTPUT  LOG_FILE_ONLY  /* file output only */
#define LOG_OUTPUT  LOG_SCREEN_FILE  /* screen + file output */


/* SigSRF background process notes:

   1) mediaTest uses the ALLOW_BACKGROUND_PROCESS define (below) and use_bkgnd_process var to control the background process.  Defaults are (i) allow use of the process, and (ii) cmd line
      entry enables/disables the process

   2) To enable the background process, a "b" suffix has to be entered after command line mode entry, for example -M0b.  The background process should be enabled only for commands that
      perform packet transcoding (i.e. not including codec test, pcap extract, frame test, etc)

   3) When enabled, the background process directly handles network packet I/O, packet processing, and transcoding.  The background process calls Pktlib and Voplib APIs

   4) When disabled, the user app (e.g. mediaTest) is responsible for calling all Pktlib and Voplib APIs required to handle and process packets

   5) The background process operates in real-time and does not implement several flags, including DS_GETORD_PKT_FTRT, DS_GETORD_PKT_RETURN_ALL_DELIVERABLE, and DS_GETORD_PKT_FLUSH.  The
      background process enables DS_SESSION_DYN_CHAN_ENABLE by default

   6) The "raw socket access permissions" comment in the Network Packet I/O Notes above also applies to the background process
*/

#define ALLOW_BACKGROUND_PROCESS


/* save intermediate raw data in packet test mode to file "dbg_data.raw" */

#define SAVE_INTERIM_OUTPUT 0

//#define VALGRIND_DEBUG
#ifdef VALGRIND_DEBUG
#define VALGRIND_DELAY 1000  /* usleep delay value in usec to allow valgrind to run multithreaded apps on the same core */
#endif

#define USE_THREAD_SEM

/* global vars */

#ifndef _LIBRARYMODE__
extern PLATFORMPARAMS PlatformParams;  /* command line params */
extern MEDIAPARAMS MediaParams[MAXSTREAMS];
extern unsigned int inFileType, outFileType, outFileType2, USBAudioInput, USBAudioOutput;
#endif

volatile bool    fNetIOAllowed = false;  /* set true if UDP socket input should be handled.  Will be set false if program / process permissions do not allow network sockets and/or USB ports to be opened. Default is disabled */
volatile bool    fUSBIOAllowed = false;
volatile int8_t  run = 1;                /* may be cleared by application signal handler to stop packet / media processing loop */
volatile char    fPMMasterThreadExit = 0;
volatile char    fPMThreadsClosing = 0;
volatile uint8_t uQueueRead = 0;
volatile char    pktStatsLogFile[CMDOPT_MAX_INPUT_LEN] = "";
volatile int     send_sock_fd = -1, send_sock_fd_ipv6 = -1;
volatile bool    frame_mode = false, use_bkgnd_process = false, use_log_file = false;  /* this group moved here from cmd_line_interface.c */
volatile bool    demo_build = false;
volatile int     debug_thread = 0;
volatile int     nManageSessionRetriesAllowed = 1;

#ifndef __LIBRARYMODE__
int num_pcap_inputs = 0, num_wav_inputs = 0, num_wav_outputs = 0, num_pcap_outputs = 0;
int in_type[MAX_SESSIONS] = { 0 };
int out_type[MAX_SESSIONS] = { 0 };
int nInFiles = 0, nOutFiles = 0;
#endif

extern DEBUG_CONFIG lib_dbg_cfg;  /* in diaglib, which is set by calls to DSConfigPktlib() with DS_CP_DEBUGCONFIG flag */
extern GLOBAL_CONFIG pktlib_gbl_cfg;  /* in pktlib, set by calls to DSConfigPktlib() with DS_CP_GLOBALCONFIG flag */

extern sem_t pktlib_sem;  /* pktlib.so */
extern sem_t pcap_write_sem;  /* declared in streamlib, semaphore used to ensure no more than 1 thread writes to a particular output file at a time */

extern uint32_t num_jb_zero_pulls[NCORECHAN];  /* values set inside get_chan_packets() in pktlib */

extern uint8_t uShowGroupContributorAmounts[MAX_SESSIONS];  /* streamlib */

int num_missed_interval_index[MAX_STREAM_GROUPS] = { 0 };  /* referenced in streamlib.so */
int num_flc_applied[MAX_STREAM_GROUPS] = { 0 };
uint32_t uFramesDropped[NCORECHAN] = { 0 };
int nMaxStreamDataAvailable[NCORECHAN] = { 0 };  /* per contributor max data available, used as a run-time stat in DSLogPacketTimeLossStats(). Updated by DSStoreStreamData() in streamlib */

#ifndef __LIBRARYMODE__  /* global vars in cmd line build, but not in thread build (pktlib) */

static HPLATFORM      hPlatform = -1;

static FILE*          fp_in[MAX_INPUT_STREAMS] = { NULL }, *fp_out[MAX_INPUT_STREAMS] = { NULL };
static unsigned int   link_layer_length[MAX_INPUT_STREAMS];

#endif

static uint32_t       nSessions_gbl = 0, nThreads_gbl = 0, num_pktmedia_threads = 0;


#ifdef OVERWRITE_INPUT_DATA
static bool fReuseInputs = false;
static int ReuseInputs(uint8_t*, unsigned int, uint32_t, SESSION_DATA*);
#endif


/* Packet stats log file notes:

  1) If ENABLE_PKT_STATS (below) is defined, and an -L or -Lfilename cmd line entry is given, a packets stats history log file is printed after the test run completes

  2) Packet history stats include:

    -sequence numbers, including missing numbers (gaps)
    -timestamps
    -packet length and special payload types (e.g. DTX)
    -out-of-order (ooo) and missing packets
    -transitions between SSRC streams
    -analysis/comparison of SSRCs and packets between jitter buffer input and output.  Packets are grouped by SSRC for analysis

    a typical log file is divided into three (3) sections, input packets, packets pulled from jitter buffer (if used), and analysis (comparison) between input and pulled packets

  3) Currently packet stats history logging is enabled for the master p/m thread only

  4) Packet stats and logging requires the diaglib module, and diaglib.h header file

  5) DSPktStatsAddEntries() records one or more packet entries in PKT_STATS structs, and DSPktStatsWriteLogFile() writes a specified number of entries to a packet stats history log file (the latter
     API is used in the "cleanup" code below)
*/
  
#define ENABLE_PKT_STATS

#ifdef ENABLE_PKT_STATS

  // #define USE_CHANNEL_PKT_STATS

/* New approach to maintaining and logging packet stats history and analysis, JHB Dec2019:

  -per-channel tracking instead of pool of stats containing all SSRCs across all threads
  -DSWritePacketLogStats() API allows packet stats history and analysis to be logged by (i) session (all channels for the session) (ii) stream group (all channels for the group) and (iii) all active channels, which supports the legacy functionality
  -channels use malloc/realloc to increase their stats mem by PKT_STATS_CHUNK number of bytes as needed.  PKT_STATS_CHUNK should be a multiple of the PKT_STATS struct defined in diaglib.h (currently 20 bytes)
*/

  #ifdef USE_CHANNEL_PKT_STATS

  PKT_STATS_HISTORY input_pkts[NCORECHAN+1] = {{ 0 }};
  PKT_STATS_HISTORY pulled_pkts[NCORECHAN+1] = {{ 0 }};

  #else

  #define MAX_PKT_STATS  1200000L  /* increased from 300K, PKT_STATS struct in diaglib.h compacted, JHB Dec2019 */
  static PKT_STATS input_pkts[MAX_PKT_STATS+100], pulled_pkts[MAX_PKT_STATS+100];

  #endif

  #define INPUT_PKTS input_pkts
  #define PULLED_PKTS pulled_pkts

#else
  #define INPUT_PKTS NULL
  #define PULLED_PKTS NULL
#endif  

#define PKT_COUNTERS_GLOBAL
#ifdef PKT_COUNTERS_GLOBAL  /* made global var, indexed by thread, to allow use as (i) param in DSPktStatsWriteLogFile() and (ii) return value in DSGetThreadInfo(), JHB Dec2019 */
   PKT_COUNTERS pkt_counters[MAX_PKTMEDIA_THREADS] = {{ 0 }};
#endif

/* multiple pkt/media thread management */

#define isMasterThread (thread_index == 0)

/* thread level items */

extern PACKETMEDIATHREADINFO packet_media_thread_info[MAX_PKTMEDIA_THREADS];  /* array of thread handles in pktlib.so, zero indicates no thread.  MAX_PKTMEDIA_THREADS is defined in pktlib.h */
extern int nPktMediaThreads;  /* current number of allocated packet/media threads */

extern uint64_t session_last_push_time[];  /* only used in debug mode with DS_ENABLE_PUSHPACKETS_ELAPSED_TIME_ALARM flag set */
extern uint8_t session_input_flags[];

extern SESSION_INFO_THREAD session_info_thread[MAX_SESSIONS];  /* in pktlib, referenced also by streamlib. SESSION_INFO_THREAD struct is defined in shared_include/session.h */

uint8_t pm_sync[MAX_PKTMEDIA_THREADS] = { 0 };

#ifdef FIRST_TIME_TIMING  /* reserved for timing debug purposes */
unsigned long long base_time = 0, first_push_time = 0, first_buffer_time = 0, first_pull_time = 0, first_contribute_time = 0;
#endif

#ifdef DEBUG_CONSISTENCY_MARKERS
extern CHANINFO_CORE ChanInfo_Core[];
#endif

/* local functions */

int get_wav_index(HSESSION[], int, int, int);
int get_pcap_index(int);

static inline int get_session_handle(HSESSION[], int, int);
static inline int get_channels(HSESSION, int[], int[], int);
static inline unsigned int uFlags_session(HSESSION);

static inline int CheckForSSRCChange(HSESSION, int[], uint8_t*, unsigned int*, int, unsigned int, unsigned int, unsigned int[], int);
static inline int CheckForDormantSSRC(HSESSION, int num_chan, int chan_nums[], int, int, HSESSION[], uint64_t cur_time, int thread_index);
static inline int CheckForOnHoldFlush(HSESSION hSession, int num_chan, int chan_nums[]);
static inline int CheckForPacketLossFlush(HSESSION hSession, int num_chan, int chan_nums[], uint64_t cur_time, int thread_index);

int InitStream(HSESSION[], int, int, bool*);
int InitSession(HSESSION, int);
int CleanSession(HSESSION, int);
#ifdef USE_CHANNEL_PKT_STATS
int ManageSessions(HSESSION[], PKT_COUNTERS[], PKT_STATS_HISTORY[], PKT_STATS_HISTORY[], bool*, int);
int WritePktLog(HSESSION, PKT_COUNTERS[], PKT_STATS_HISTORY[], PKT_STATS_HISTORY[], int);
#else
int ManageSessions(HSESSION[], PKT_COUNTERS[], PKT_STATS[], PKT_STATS[], bool*, int);
int WritePktLog(HSESSION, PKT_COUNTERS[], PKT_STATS[], PKT_STATS[], int);
#endif
void ThreadDebugOutput(HSESSION[], int, int, int, unsigned int);
void DisplayChanInfo(HSESSION, int, int[], int);

#define PACKET_TIME_STATS
#ifdef PACKET_TIME_STATS

#define PACKET_TIME_STATS_INPUT  0
#define PACKET_TIME_STATS_PULL   1

void RecordPacketTimeStats(int, uint8_t*, int, uint32_t, int);
void add_stats_str(char* stats_str, unsigned int max_len, const char* fmt, ...);
#endif

/* sig_printf() defines */

#define PRN_LEVEL_INFO     4
#define PRN_LEVEL_STATS    3
#define PRN_LEVEL_WARNING  2
#define PRN_LEVEL_ERROR    1
#define PRN_LEVEL_NONE     0  /* if DEBUG_CONFIG struct (shared_include/config.h) uPrintfLevel is set to zero, screen output is effectively turned off */
#define PRN_LEVEL_MASK     0xf

#define PRN_SAME_LINE      0x10

#ifdef __cplusplus
extern "C" {
#endif
void ResetPktStats(HSESSION);
void sig_printf(char*, int, int);
#ifdef __cplusplus
}
#endif

void manage_pkt_stats_mem(PKT_STATS_HISTORY[], int, int);

void ThreadAbort(int, char*);


#define SECONDARY_THREADS_DEPRECATED  /* mediaTest no longer supports "secondary threads". Since early 2018, packet_flow_media_proc() supports multiple packet/media threads, and multiple app threads are implemented using mediaTest -Et ant -tN cmd line options, which invoke one or more mediaMin application threads, JHB Jan2020 */

#ifndef SECONDARY_THREADS_DEPRECATED

/* Multithread / muliticore operation notes:

  1) If the -tN cmd line option specifies N > 1, then N-1 threads are started (in addition to main thread) that open / process N-1 additional sessions and/or pcaps

  2) Currently the "core list" cmd line option (-mN, where N is a bitwise core list) is not active for x86 operation.  This may be used at some future point for core pinning (aka processor affinity).
     The core list option is used for high capacity coCPU operation (it's a required cmd line entry in that case)
  
  3) ENABLE_MULTITHREAD_OPERATION may be undefined to de-activte multithread code
*/
  
#define ENABLE_MULTITHREAD_OPERATION

#if defined(ENABLE_MULTITHREAD_OPERATION) && !defined(__LIBRARYMODE__)

static SESSION_DATA session_data_g[MAX_SESSIONS] = {{ 0 }};  /* used by secondaryThreads() */

void* secondaryThreads(void* arg);

static int num_pkts_read_multithread = 0;
static int num_pkts_buffered_multithread = 0;
static int pkt_xcode_cnt_multithread = 0;
static int pkt_write_cnt_multithread = 0;
static int pkt_output_cnt_multithread = 0;
#endif

#endif  /* !SECONDARY_THREADS_DEPRECATED */


/* enable stream audio data merging */

#define ENABLE_STREAM_GROUPS

#ifdef ENABLE_STREAM_GROUPS

#define DS_GROUP_CHANNEL -1

/* local functions for stream merging */
void DisableStreamMerging(int chnum_parent);

#endif

#if 0
static int progress_var[MAX_SESSIONS] = { 0 };
#endif

static uint8_t nDormantChanFlush[MAX_SESSIONS][MAX_TERMS] = {{ 0 }};
static uint8_t nOnHoldChanFlush[MAX_SESSIONS][MAX_TERMS] = {{ 0 }};
extern short int nOnHoldChan[MAX_SESSIONS][MAX_TERMS];  /* declared in streamlib.so, referenced by DSMergeStreamGroupContributors() */

static int8_t input_buffer_interval[MAX_SESSIONS][MAX_TERMS] = {{ 0 }};
static int8_t output_buffer_interval[MAX_SESSIONS][MAX_TERMS] = {{ 0 }};
static int8_t ptime[MAX_SESSIONS][MAX_TERMS] = {{ 0 }};

static uint8_t uDisplayDTMFEventMsg[MAX_SESSIONS][MAX_TERMS] = {{ 0 }};
static uint8_t uDTMFState[MAX_SESSIONS][MAX_TERMS] = {{ 0 }};

static int8_t nMaxLossPtimes[MAX_SESSIONS][MAX_TERMS] = {{ 0 }};

static uint64_t last_packet_time[MAX_SESSIONS] = { 0 };
static uint64_t no_pkt_elapsed_time[MAX_SESSIONS] = { 0 };

#define DELTA_SUM_LENGTH 32
static int64_t pkt_delta_runsum[MAX_SESSIONS][DELTA_SUM_LENGTH] = {{ 0 }};
static int64_t pkt_delta_sum[MAX_SESSIONS] = { 0 };
static int pkt_sum_index[MAX_SESSIONS] = { 0 };
static uint32_t pkt_count[MAX_SESSIONS] = { 0 };
static HSESSION hSession0 = -1, hSession1 = -1, hSession2 = -1;

#ifdef PACKET_TIME_STATS

static uint64_t packet_in_time[NCORECHAN] = { 0 };
static uint64_t last_packet_in_time[NCORECHAN] = { 0 };
static uint32_t packet_rtp_time[NCORECHAN] = { 0 };
static uint32_t last_rtp_timestamp[NCORECHAN] = { 0 };
static uint64_t packet_max_delta[NCORECHAN] = { 0 };
static uint32_t max_delta_packet[NCORECHAN] = { 0 };

static uint64_t packet_media_delta[NCORECHAN] = { 0 };
static uint64_t packet_sid_delta[NCORECHAN] = { 0 };
static uint64_t packet_max_media_delta[NCORECHAN] = { 0 };
static uint32_t max_media_delta_packet[NCORECHAN] = { 0 };
static uint64_t packet_max_sid_delta[NCORECHAN] = { 0 };
static uint32_t max_sid_delta_packet[NCORECHAN] = { 0 };
static uint32_t media_stats_pkt_count[NCORECHAN] = { 0 };
static uint32_t sid_stats_pkt_count[NCORECHAN] = { 0 };
static uint16_t prev_pyld_content[NCORECHAN] = { 0 };

static uint64_t packet_in_time_pull[NCORECHAN] = { 0 };
static uint64_t last_packet_in_time_pull[NCORECHAN] = { 0 };
static uint32_t packet_rtp_time_pull[NCORECHAN] = { 0 };
static uint32_t last_rtp_timestamp_pull[NCORECHAN] = { 0 };

uint64_t last_buffer_time[NCORECHAN] = { 0 };  /* per stream last jitter buffer add time (in msec) */
static uint64_t last_pull_time[NCORECHAN] = { 0 };  /* per stream last jitter buffer pull time (in msec), updated after calls to DSGetOrderedPackets(). Referenced in get_chan_packets() in pktlib.c */

static uint32_t packet_in_bursts[NCORECHAN] = { 0 };
extern uint32_t packet_out_bursts[];  /* stat maintained in pktlib (pktlib.c) */

static int32_t pkt_count_group[MAX_STREAM_GROUPS] = { 0 };
static bool fDisplayActiveChannels[MAX_SESSIONS] = { false };

#endif

static uint32_t pkt_loss_flush[NCORECHAN] = { 0 };
static uint32_t pkt_pastdue_flush[NCORECHAN] = { 0 };
static uint32_t pkt_level_flush[NCORECHAN] = { 0 };

static bool fFirstXcodeOutputPkt[NCORECHAN] = { false };
static uint8_t session_run_time_stats[NCORECHAN] = { 0 };
static bool fFirstGroupContribution[MAX_SESSIONS] = { false };


void* packet_flow_media_proc(void* pExecutionMode) {

/* Execution mode notes:

     -execution modes include app (pExecutionMode[0] = 'a'), thread (pExecutionMode[0] = 't'), and process (pExecutionMode[0] = 'p').  Optional command line processing can be specified by adding 'c' in ExecutionMode[1]
     -thread execution:  running here as a thread started by user app.  Set fMediaThread flag, free arg passed by pthread_create()
*/

   bool fMediaThread = ((char*)pExecutionMode)[0] == 't';
   if (fMediaThread) free(pExecutionMode);

   bool packet_mode = !frame_mode;

   int thread_index = 0;
   bool fThreadInputActive = false;
   bool fThreadOutputActive = false;

   int32_t i, j, ret_val, num_chan, n, session_status, num_pkts;
   int32_t nSessionsCreated = 0, nSessionsInit = 0, nStreamsInit = 0, numStreams, numSessions;

   int hSession = -1, hSession_flags = -1;

#ifndef __LIBRARYMODE__
   char key;
   uint32_t inFilesIndex = 0;
   bool fInputWrap = false;
   char default_config_file[] = "session_config/packet_test_config";
   char *config_file;
   FILE *fp_cfg = NULL;
   unsigned int uFlags_session_create;
   int ret_val_wav;
   MEDIAINFO MediaInfo[MAX_INPUT_STREAMS] = {{{ 0 }}};
   int pcap_index, wav_index;
   MEDIAINFO MediaInfo_merge = {{ 0 }};
#endif

   HSESSION hSessions_t[MAX_SESSIONS] = { -1 };
   SESSION_DATA session_data_t[MAX_SESSIONS] = {{ 0 }};

   unsigned int packet_length, pyld_len, data_length;
   unsigned int packet_len[256], payload_info[256];
   uint8_t *pkt_ptr, *pyld_ptr;

   int recv_len = 0, send_len = 0;
   unsigned char pkt_in_buf[32*MAX_RTP_PACKET_LEN] = {0}, pkt_out_buf[MAX_RTP_PACKET_LEN] = {0}, media_data_buffer[4*MAX_RTP_PACKET_LEN] = {0}, encoded_data_buffer[2*MAX_RTP_PACKET_LEN] = {0};
   uint8_t recv_jb_buffer[MAX_RTP_PACKET_LEN*MT_MAX_CHAN*JB_DEPTH] = {0};
   
   TERMINATION_INFO termInfo, termInfo_link;
   HCODEC hCodec;
   int chnum_parent, chnums[MAX_TERMS*4], chnum;
   int recv_sock_fd = -1;

   int pkt_pulled_cnt = 0, pkt_xcode_cnt = 0, pkt_passthru_cnt = 0, pkt_group_cnt = 0;
   int last_pkt_input_cnt = -1, last_pkt_read_cnt = -1, last_pkt_add_to_jb_cnt = -1, last_pkt_pulled_cnt = -1, last_pkt_xcode_cnt = 0, last_pkt_group_cnt = 0;
   int pkt_decode_cnt = 0;
   int last_pkt_decode_cnt = 0;

   uint32_t threadid = 0;
   #if defined(ENABLE_MULTITHREAD_OPERATION) && !defined(__LIBRARYMODE__)
   pthread_t threads[MAX_SESSIONS] = { 0 };
   int last_multithread_buffered_cnt = 0, last_multithread_pkt_write_cnt = 0;
   #endif

   char tmpstr[1024];

   unsigned long long interval_time, start_time = 0, cur_time = 0, last_packet_time_thread = 0, prev_display_time = 0/*-1000000L*/, prev_thread_CPU_time = 0, interval_count = 0, start_profile_time = 0, end_profile_time, input_time = 0, buffer_time = 0, chan_time = 0, pull_time = 0, decode_time = 0, encode_time = 0, group_time = 0;
   int num_thread_buffer_packets =  0, num_thread_decode_packets = 0, num_thread_encode_packets = 0, num_thread_group_contributions = 0;

   #if SAVE_INTERIM_OUTPUT
   FILE *fp_dbg = fopen("dbg_data.raw", "wb");
   #endif
   unsigned int uFlags_add, uFlags_info, uFlags_get, uFlags_format;
   int rtp_ofs;

   int stream_indexes[MAX_INPUT_STREAMS] = { 0 };

   int chan_nums[MAX_TERMS+32] = { 0 };  /* allow for dynamic channels */

   int32_t pkts_read[MAX_INPUT_STREAMS] = { 0 };

   int media_data_len, out_media_data_len;
   int in_media_sample_rate __attribute__ ((unused));
   int out_media_sample_rate __attribute__ ((unused));
   unsigned int sample_rate[MAX_INPUT_STREAMS] __attribute__ ((unused)) = { 0 };
   FormatPkt formatPkt = { 0 };

#ifndef PKT_COUNTERS_GLOBAL  /* moved to global var, indexed by thread, to allow use as param in DSPktStatsWriteLogFile(), JHB Dec2019 */
   PKT_COUNTERS pkt_counters = { 0 };
#endif

   #if (LOG_OUTPUT != LOG_SCREEN_ONLY)
   FILE *fp_sig_lib_log = NULL;
   char sig_lib_log_filename[] = { "sig_lib_log.txt" };
   #endif

   #ifdef ENABLE_STREAM_GROUPS

/* stream audio data merging declarations */

   #ifndef __LIBRARYMODE__
   char merge_wav_filename[1024] = "", merge_pcap_filename[1024] = "";
   FILE *fp_out_wav_merge = NULL, *fp_out_pcap_merge = NULL;
   #endif

   #endif

   unsigned int pkt_len[512];
   int numPkts = 0, numPkts_matched;
   int pyld_type;
   bool fFTRTInUse = false;

   char szMissingContributors[200] = "";

   DEBUG_CONFIG dbg_cfg = { 0 };

   bool fPreemptOmit;

   int nNumCleanupLoops = 0;

//#define DEBUGINPUTPACKETS
#ifdef DEBUGINPUTPACKETS
   int pkt_chnum_ctr = 0;
   int pkt_chnum[8192] = { 0 };
   int pkt_chnum_len[8192] = { 0 };
   int pkt_chnum_i[8192] = { 0 };
   int pkt_chnum_hSession[8192] = { 0 };
   int pkt_chnum_numStreams[8192] = { 0 };
   unsigned long long int pkt_chnum_buftime[8192] = { 0 };
#endif


/* packet mode code starts */

   #ifndef __LIBRARYMODE__
   sem_init(&pcap_write_sem, 0, 1);
   #endif

   sprintf(tmpstr, "x86 pkt/media start");
   #ifdef __LIBRARYMODE__
   sprintf(&tmpstr[strlen(tmpstr)], ", pktlib");
   #endif

   if (fMediaThread) sprintf(&tmpstr[strlen(tmpstr)], ", thread execution, thread id = 0x%llx", (unsigned long long)pthread_self());
   else sprintf(&tmpstr[strlen(tmpstr)], ", mediaTest cmd line");
   if (frame_mode) sprintf(&tmpstr[strlen(tmpstr)], " (frame mode)");
   strcat(tmpstr, "\n");
   sig_printf(tmpstr, PRN_LEVEL_INFO, -1);

   if (!fMediaThread) {  /* in thread mode platform and/or data plane config and logging are handled by user app */

      #ifndef __LIBRARYMODE__
      hPlatform = DSAssignPlatform(NULL, PlatformParams.szCardDesignator, 0, 0, 0);  /* assign platform handle, needed for concurrency and VM management */
      #endif

   /* Configure packet lib debug logging */

      dbg_cfg.uDisableMismatchLog = 1;
      dbg_cfg.uDisableConvertFsLog = 1;
      dbg_cfg.uLogLevel = 8;  /* 5 is default, set to 8 to see INFO messages, including jitter buffer */
      dbg_cfg.uEventLogMode = LOG_OUTPUT;

      #if (LOG_OUTPUT != LOG_SCREEN_ONLY)
      fp_sig_lib_log = fopen(sig_lib_log_filename, "w");
      dbg_cfg.uEventLogFile = fp_sig_lib_log;
      #endif

      dbg_cfg.uPrintfLevel = 5;

      DSConfigPktlib(NULL, &dbg_cfg, DS_CP_INIT | DS_CP_DEBUGCONFIG);
      DSConfigVoplib(NULL, NULL, DS_CV_INIT);
   }

   #if defined(ENABLE_MULTITHREAD_OPERATION) && !defined(__LIBRARYMODE__)
   if (!fMediaThread) nThreads_gbl = PlatformParams.cimInfo[0].taskAssignmentCoreLists[0];  /* this is the -tN cmd line value, if entered */
   #endif

   if (isMasterThread && nThreads_gbl <= 0) nThreads_gbl = 1;

   if (isMasterThread && demo_build && nThreads_gbl > 2)
   {
      fprintf(stderr, "Number of threads exceeds demo limit, reducing number of threads to 2\n");
      nThreads_gbl = 2;
   }


/* init thread level items */

   if (fMediaThread) {

      static bool fVersionLog = false;
      if (!fVersionLog) {
         fVersionLog = true;
         Log_RT(4, "INFO: first packet/media thread running, lib versions DirectCore v%s, pktlib v%s, streamlib v%s, voplib v%s, alglib v%s, diaglib v%s \n", HWLIB_VERSION, PKTLIB_VERSION, STREAMLIB_VERSION, VOPLIB_VERSION, ALGLIB_VERSION, DIAGLIB_VERSION);
      }
 
      #if 0
      i = 0;
      pthread_t threadid = pthread_self();
      Log_RT(8, "DEBUG2: before thread init, packet_media_thread_info[%d].threadid = 0x%llx, numSessions = %d, sizeof(packet_media_thread_info[0]) = %lu, sizeof(packet_media_thread_info.threadid) = %lu, sizeof(packet_media_thread_info.fMediaThread) = %lu, threadid = 0x%llx\n", i, (unsigned long long)packet_media_thread_info[i].threadid, packet_media_thread_info[i].numSessions, sizeof(packet_media_thread_info[0]), sizeof(packet_media_thread_info[0].threadid), sizeof(packet_media_thread_info[0].fMediaThread), (unsigned long long)threadid);
      #endif

      bool fFound = false;
      int num_pktmedia_threads_local;
 
      while (!fFound) {  /* find our thread index.  note we need to wait until DSConfigMediaService() has initialized mediaThreads[] */

         for (i=0; i<MAX_PKTMEDIA_THREADS; i++) {

            #if 0
            static int fOnce[MAX_PKTMEDIA_THREADS] = { 0 };
            if (fOnce[i] < 100) {
               Log_RT(8, "DEBUG2: packet_media_thread_info[%d].threadid = 0x%llx, numSessions = %d, threadid = 0x%llx\n", i, (unsigned long long)packet_media_thread_info[i].threadid, packet_media_thread_info[i].numSessions, (unsigned long long)threadid);
               fOnce[i]++;
            }
            #endif

            if (pthread_equal(packet_media_thread_info[i].threadid, pthread_self())) {

#ifdef USE_THREAD_SEM
               if (!packet_media_thread_info[i].thread_sem_init) {

                  sem_init(&packet_media_thread_info[i].thread_sem, 0, 1);
                  packet_media_thread_info[i].thread_sem_init = true;
               }
#endif

               if (packet_media_thread_info[i].niceness) {  /* change thread niceness here, if specified in DSConfigMediaService() call, JHB Dec2018 */

                  pid_t tid = syscall(SYS_gettid);
                  int tc_ret = setpriority(PRIO_PROCESS, tid, packet_media_thread_info[i].niceness);

                  char infostr[40] = "";
                  if (tc_ret < 0) sprintf(infostr, ", errno = %s", strerror(errno));

                  Log_RT(tc_ret < 0 ? 3: 4, "%s: DSConfigMediaService() says setpriority() %sset Niceness to %d for pkt/media thread %d%s \n", tc_ret < 0 ? "WARNING" : "INFO", tc_ret < 0 ? "failed to " : "", packet_media_thread_info[i].niceness, i, infostr);
               }

               thread_index = i;

               sem_wait(&pktlib_sem);  /* protect any mod to num_pktmedia_threads. ManageSessions() runs immediately upon creation of first p/m thread, JHB Jan2020 */
               num_pktmedia_threads++;
               num_pktmedia_threads_local =  num_pktmedia_threads;

               DSCreateFilelibThread();  /* establish a thread index entry in filelib, this allows us to avoid semaphore usage when calling DSLoadDataFile() and DSSaveDataFile() (hwlib) which in turn call filelib APIs DSOpenFile, DSCloseFile, DSWriteWvfrmHeader(), etc, JHB Jan2020 */

               sem_post(&pktlib_sem);

               fFound = true;

               break;
            }
         }

         if (!fFound) usleep(1000);  /* wait 1 msec */
         else Log_RT(4, "INFO: initializing packet/media thread %d, uFlags = 0x%x, threadid = 0x%llx, total num pkt/med threads = %d\n", thread_index, packet_media_thread_info[thread_index].uFlags, (unsigned long long)packet_media_thread_info[thread_index].threadid, num_pktmedia_threads_local);
      }
   }
   else packet_media_thread_info[thread_index].threadid = pthread_self();

   packet_media_thread_info[thread_index].fMediaThread = fMediaThread;
   packet_media_thread_info[thread_index].packet_mode = packet_mode;


/* initialize session handles to -1 before session creation */

   memset(hSessions_t, 0xff, sizeof(hSessions_t));

   
/* when running as a thread discover any pre-existing sessions created by the user app before DSConfigMediaService() was called (i.e. API in pktlib that starts the thread) */

   if (fMediaThread) {

      if (!isMasterThread) goto run_loop;  /* only master thread does pre-existing session init */

      i = 0;
      do {

         hSession = DSGetSessionInfo(i, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_SESSION | DS_SESSION_INFO_SUPPRESS_ERROR_MSG, 0, &session_data_t[i]);

         if (hSession >= 0) {

            hSessions_t[i] = hSession;
            i++;
         }

         if (demo_build && i > 0) {

            fprintf(stderr, "Demo build is limited to 1 session per thread, ignoring subsequent sessions\n");
            break;
         }

      } while (hSession >= 0 && i < MAX_SESSIONS);

      nSessions_gbl = i;

      sprintf(tmpstr, "thread mode, nSessions_gbl = %d\n", nSessions_gbl);
      sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);

      goto init_sessions;  /* in thread execution mode, user app packet I/O is through DSPushPackets() and DSPullPackets().  In command line execution mode, open / create pcap files */
   }


#ifndef __LIBRARYMODE__

   if (strlen(MediaParams[0].configFilename) == 0 || access(MediaParams[0].configFilename, F_OK) == -1) 
   {
      printf("Specified config file: %s does not exist, using default file.\n", MediaParams[0].configFilename);
      config_file = default_config_file;
   }
   else config_file = MediaParams[0].configFilename;

   sprintf(tmpstr, "Opening session config file: %s\n", config_file);
   sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);

/* open config file */
   fp_cfg = fopen(config_file, "r");
   if (fp_cfg == NULL)
   {
      fprintf(stderr, "Failed to open config file %s, exiting codec mode\n", config_file);
      goto cleanup;
   }

/* parse config file */
   while (run > 0 && (parse_session_config(fp_cfg, &session_data_t[nSessions_gbl]) != -1)) nSessions_gbl++;

   printf("%d session(s) found in config file\n", nSessions_gbl);

   if(demo_build && nSessions_gbl > 2)
   {
      printf("Demo build limited to 2 sessions max, using first 2 sessions found\n");
      nSessions_gbl = 2;
   }

   if (nSessions_gbl > MAX_SESSIONS)
   {
      printf("Number of sessions exceeds mediaTest's maximum reducing to %d\n", MAX_SESSIONS);
      nSessions_gbl = MAX_SESSIONS;
   }

   #ifdef ENABLE_MULTITHREAD_OPERATION
   if (nThreads_gbl > nSessions_gbl) {
      printf("More threads specified than sessions available, reducing number of threads to %d\n", nSessions_gbl);
      nThreads_gbl = nSessions_gbl;
   }
   #endif

/* Close config file */
   fclose(fp_cfg);

/* open any input pcap files given, advance file pointer to first packet.  Abort program on any input file failure */
   while (MediaParams[inFilesIndex].Media.inputFilename != NULL && strlen(MediaParams[inFilesIndex].Media.inputFilename)) {

      if (strstr(strupr(strcpy(tmpstr, MediaParams[inFilesIndex].Media.inputFilename)), ".PCAP"))
      {
         if ((link_layer_length[nInFiles] = DSOpenPcap(MediaParams[inFilesIndex].Media.inputFilename, &fp_in[nInFiles], NULL, "", DS_READ | DS_OPEN_PCAP_READ_HEADER)) < 0) break;
      }
      else
      {
         fprintf(stderr, "Input file: %s is not a .pcap file\n", MediaParams[inFilesIndex].Media.inputFilename);
         break;
      }

      frameInterval[nInFiles] = MediaParams[inFilesIndex].Media.frameRate;  /* get cmd line rate entry, if any */

      in_type[num_pcap_inputs] = PCAP;
      num_pcap_inputs++;
      inFilesIndex++;
      nInFiles++;

//      #if defined(REUSE_INPUT_FILES) && !defined(NEW_REUSE_CODE)
      #ifdef REUSE_INPUT_FILES
      if (nThreads_gbl > 1 && (MediaParams[inFilesIndex].Media.inputFilename == NULL || !strlen(MediaParams[inFilesIndex].Media.inputFilename)))  /* only active if -tN cmd line entry is present and N > 1 (more than one thread).  This avoids issues with pcaps containing multiple streams used with multi-session config files, in which case sessions > inputs.  Added Sep2018, JHB */
      {
         numStreams = inFilesIndex;  /* save number of input streams specified on cmd line */

         if (nInFiles >= (int)nSessions_gbl) break;
         #ifdef OVERWRITE_INPUT_DATA
         else fReuseInputs = true;
         #endif

         inFilesIndex = 0;
         fInputWrap = true;
      }
      else if (fInputWrap && nInFiles >= (int)nSessions_gbl) break;
      #else
      numStreams = nInFiles;
      #endif
   }

#if 0
   #ifdef NEW_REUSE_CODE
   if (nSessions_gbl > nInFiles) fReuseInputs = true;
   #endif
#endif

   for (i=0; i<nInFiles; i++) if (fp_in[i] == NULL) goto cleanup;  /* error condition for at least one input file, message already printed */

/* open output pcap and/or wav files, stop on first failure (but still allow program to run) */

   while (MediaParams[nOutFiles].Media.outputFilename != NULL && strlen(MediaParams[nOutFiles].Media.outputFilename)) {

      if (strstr(strupr(strcpy(tmpstr, MediaParams[nOutFiles].Media.outputFilename)), ".PCAP") && packet_media_thread_info[thread_index].packet_mode) {

         if (DSOpenPcap(MediaParams[nOutFiles].Media.outputFilename, &fp_out[nOutFiles], NULL, "", DS_WRITE | DS_OPEN_PCAP_WRITE_HEADER) < 0) break;
         out_type[nOutFiles] = PCAP;
         num_pcap_outputs++;
      }
      else if (strstr(tmpstr, ".WAV")) {  /* added pcap to wav mode, JHB Jun2017 */

         strcpy(MediaInfo[nOutFiles].szFilename, MediaParams[nOutFiles].Media.outputFilename);

         MediaInfo[nOutFiles].Fs = 16000;  /* set default sampling rate (in Hz), update wav header after decoding, when sampling rate becomes known */
         MediaInfo[nOutFiles].SampleWidth = 16;  /* 16-bit audio */
         MediaInfo[nOutFiles].NumChan = 1;  /* mono */
         MediaInfo[nOutFiles].CompressionCode = DS_GWH_CC_PCM;  /* default is 16-bit PCM.  G711 uLaw and ALaw are also options */

         ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out[nOutFiles], MediaInfo[nOutFiles].szFilename, (uintptr_t)NULL, 0, DS_CREATE, &MediaInfo[nOutFiles]);  /* create .wav file and initialize wav header */

         if (fp_out[nOutFiles] == NULL) {

            fprintf(stderr, "Failed to open output wav file: %s, ret_val = %d\n", MediaInfo[nOutFiles].szFilename, ret_val);
            break;
         }
         else {

            out_type[nOutFiles] = WAV_AUDIO;
            printf("Opened audio output file: %s\n", MediaInfo[nOutFiles].szFilename);
            num_wav_outputs++;
         }
      }

      nOutFiles++;
   }


   #ifdef ALLOW_BACKGROUND_PROCESS
   if (!use_bkgnd_process)
   #endif
   #if 1  /* make sure if external app disables fNetIOAllowed, no socket I/O occurs, JHB Oct2018 */
   if (fNetIOAllowed && isMasterThread)
   #endif
   {
      /* See if a network socket can be opened */
      recv_sock_fd = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
   
      if (recv_sock_fd == -1)
      {

         printf("socket() errno = %d, errno description = %s\n", errno, strerror(errno));

         if (errno == EACCES || errno == EPERM) {

            printf("User permissions do not allow network I/O sockets to be opened; program will still run with file I/O\n");

            fNetIOAllowed = false;  /* permissions or other system problems prevent opening a network socket, so we disable network I/O from this point forward */
         }
         else {

            printf("ERROR: failed to open network socket for receiving\n");
            goto cleanup;
         }
      }
      else {
      
         fNetIOAllowed = true;  /* network I/O allowed */
         Log_RT(8, "DEBUG2: fNetIOAllowed enabled\n");
      }

      #ifndef USE_PKTLIB_NETIO
         /* pktlib network I/O not in use, make local socket non-blocking */
         #if NONBLOCKING
         if (recv_sock_fd != -1) fcntl(recv_sock_fd, F_SETFL, O_NONBLOCK);
         #endif
      #else
         /* pktlib network I/O in use, local network socket won't be used so close it */
         if (recv_sock_fd != -1) {
            close(recv_sock_fd);
            recv_sock_fd = -1;
         }
      #endif
   }

   #ifdef ENABLE_MULTITHREAD_OPERATION  /* deprecated, see comments above near SECONDARY_THREADS_DEPRECATED */

/* if multithread test enabled using -tN cmd line option (N > 1), then start N-1 threads that open / process N-1 additional sessions and pcap files.  The primary thread is
   processed here in main(), and secondary threads are processed in the secondaryThreads() local function.  The main purpose of secondary threads is to test and measure
   concurrency (high capacity) and thread-safe operation; currently they handle only pcap I/O, with no network I/O and no wav file output */

   if (nThreads_gbl > 1) {

      int tc_ret;

      memcpy(session_data_g, session_data_t, sizeof(session_data_g));  /* initialize session_data_g[] for use in secondaryThreads().  Note that secondaryThreads() has a local hSessions_t[], JHB Oct2018 */

      for (i = 1; i < (int)nThreads_gbl; i++)
      {
         uint32_t *arg = (uint32_t *)malloc(sizeof(uint32_t));
         if (arg == NULL) 
         {
            fprintf(stderr, "%s:%d: Couldn't allocate memory for thread %d arg\n", __FILE__, __LINE__, i);
            break;
         }

         *arg = i;
         printf("Multithread test enabled, creating thread %d\n", i);

         if ((tc_ret = pthread_create(&threads[i], NULL, secondaryThreads, arg)))
         {
            fprintf(stderr, "%s:%d: pthread_create() failed for thread number %d, returned %d\n", __FILE__, __LINE__, i, tc_ret);
            goto cleanup;
         }
      }
   }

   #endif

#endif  /* __LIBRARYMODE__ */

init_sessions:

/* create sessions */

   for (i = threadid; i < (int)nSessions_gbl; i += nThreads_gbl)
   {
      if(demo_build && nSessionsCreated > 0)
      {
         fprintf(stderr, "Demo build is limited to 1 session per thread, ignoring subsequent sessions\n");
         break;
      }

      if (fMediaThread) goto set_session_flags;  /* in thread execution mode, user app interface is through DSPushPackets() and DSPullPackets(), including session handles.  The user app creates all sessions */

#ifndef __LIBRARYMODE__

      printf("Creating session %d\n", i);

   /* DSCreateSession() notes:

      1) A SESSION_DATA struct is required, this can be filled in by user app code, or from external source such as session config files (used here with the mediaTest program).  A
         SESSION_DATA struct contains term1 and term2 TERMINATION_INFO substructs

      2) The DS_SESSION_MODE_IP_PACKET flag indicates that packets will be received and transmitted for the session using IP/UDP/RTP format, either IPv4 or IPv6.  For IPv6, header extensions
         are supported

      3) The DS_SESSION_DYN_CHAN_ENABLE flag enables "dynamic channel creation", which occurs if an SSRC transition is detected in incoming packet flow.  In that case, if a new SSRC appears,
         then a new channel (in addition to the termN channels initially defined in TERMINATION_INFO structs) is created on the fly, along with a new decoder instance.  Currently there is not
         a ratified RFC for "multiple RTP streams within one session", but there is a long history of proposals and recommended guidelines put forward by Magnus Westerlund @ Ericsson and Qin
         Wu @ Huawei (search for RFC 8108)

      4) The DS_SESSION_USER_MANAGED flag allows sessions to be created with partial or all duplication of termN IP addr:port values with other sessions.  In this case, the user app is expected
         to pay careful attention to which packets are associated with which sessions (see additional comments below).  Without this flag, Pktlib handles all packet-to-session matching
         internally using hashing methods
   */

      uFlags_session_create = DS_SESSION_MODE_IP_PACKET | DS_SESSION_DYN_CHAN_ENABLE | DS_SESSION_DISABLE_PRESERVE_SEQNUM;

      #ifdef ENABLE_MANAGED_SESSIONS
      uFlags_session_create |= DS_SESSION_USER_MANAGED;
      #endif

      #ifdef ALLOW_BACKGROUND_PROCESS
      if (use_bkgnd_process) {
         uFlags_session_create |= DS_SESSION_DP_LINUX_SOCKETS;
      }
      else
      #endif

      if (!fNetIOAllowed) uFlags_session_create |= DS_SESSION_DISABLE_NETIO;

      hSessions_t[i] = DSCreateSession(hPlatform, NULL, &session_data_t[i], uFlags_session_create);

      if (hSessions_t[i] == -1) 
      {
         printf("Failed to create session, continuing test with already created sessions\n");
         break;
      }


      #ifdef ENABLE_STREAM_GROUPS

   /* Determine if stream audio data merging is active */

      if (fp_out_wav_merge == NULL && (session_data_t[i].term1.group_mode > 0 || session_data_t[i].term2.group_mode > 0))
      {
#if 0
         MediaInfo_merge.Fs = DSGetCodecFs(session_data_t[i].group_term.codec_type, session_data_t[i].group_term.sample_rate);
#else
         HCODEC hCodecGroup = DSGetSessionInfo(hSessions_t[i], DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CODEC, 0, NULL);
         MediaInfo_merge.Fs = DSGetCodecSampleRate(hCodecGroup);
#endif
         MediaInfo_merge.SampleWidth = 16;  /* 16-bit audio */
         MediaInfo_merge.NumChan = 1;  /* mono */
         MediaInfo_merge.CompressionCode = DS_GWH_CC_PCM;  /* default is 16-bit PCM.  G711 uLaw and ALaw are also options */

         GetOutputFilename(merge_wav_filename, WAV_AUDIO, "_merge");  /* get merged stream output filename */

         strcpy(MediaInfo_merge.szFilename, merge_wav_filename);

         ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_wav_merge, merge_wav_filename, (uintptr_t)NULL, 0, DS_CREATE, &MediaInfo_merge);

         if (fp_out_wav_merge == NULL) {

            fprintf(stderr, "Failed to open output merge wav file: %s, ret_val = %d\n", merge_wav_filename, ret_val);
            break;
         }
         else
            printf("Opened audio output merged file: %s\n", merge_wav_filename);
      }

      if (fp_out_pcap_merge == NULL && (session_data_t[i].term1.group_mode > 0 || session_data_t[i].term2.group_mode > 0))
      {
         GetOutputFilename(merge_pcap_filename, PCAP, "_merge");
         
         ret_val = DSOpenPcap(merge_pcap_filename, &fp_out_pcap_merge, NULL, "", DS_WRITE | DS_OPEN_PCAP_WRITE_HEADER);
         
         if (fp_out_pcap_merge == NULL) {
            fprintf(stderr, "Failed to open output merge pcap file: %s ret_val = %d\n", merge_pcap_filename, ret_val);
            break;
         }
      }

      #endif  /* stream merging */

#endif /* __LIBRARY_MODE__ */

set_session_flags:

      nSessionsCreated++;
   }

   if (!fMediaThread && nSessionsCreated <= 0) {

      fprintf(stderr, "Failed to create any sessions, exiting packet/media processing\n");
      goto cleanup;
   }

   for (i = threadid; i < nSessionsCreated; i += nThreads_gbl) {

      if (InitSession(hSessions_t[i], thread_index)) nSessionsInit++;
   }

   if (!fMediaThread) {
      sprintf(tmpstr, "Created %d sessions(s), initialized %d session(s)\n", nSessionsCreated, nSessionsInit);
      sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);
   }

   #ifndef __LIBRARYMODE__
   numStreams = fMediaThread ? nSessionsCreated : max(nInFiles, nSessionsCreated);
   #else
   numStreams = nSessionsCreated;
   #endif

   for (i = threadid; i < numStreams; i += nThreads_gbl) {

      if (InitStream(hSessions_t, i, thread_index, &fFTRTInUse)) nStreamsInit++;
   }

   sprintf(tmpstr, "Initialized %d static sessions(s) and %d stream(s)\n", nSessionsInit, nStreamsInit);
   sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);

   #ifndef __LIBRARYMODE__
   if (!fMediaThread) {
      if (!nOutFiles || !num_wav_outputs) printf("Created %d transcoding session(s)\n", min(nSessionsCreated, nSessionsInit));
      else if (!num_pcap_outputs) printf("Created %d decoding session(s)\n", min(nSessionsCreated, nSessionsInit));
      else printf("Created %d transcoding / decoding session(s)\n", min(nSessionsCreated, nSessionsInit));
   }
   #endif


run_loop:

   if (!fMediaThread) printf("Starting processing loop, press 'q' to exit\n");

   if (!start_time) start_time = get_time(USE_CLOCK_GETTIME);
   
/* packet/media thread loop */

   do {

      if (run == 99) continue;  /* reserved for system stall stress testing (p/m threads are stalled / preempted for whatever reason), JHB Apr2020 */

      if (fFTRTInUse || (cur_time - prev_display_time > 20000)) {  /* print counters and check keyboard input every 20 msec */

         if ((int)pkt_counters[thread_index].pkt_input_cnt != last_pkt_input_cnt || (int)pkt_counters[thread_index].pkt_read_cnt != last_pkt_read_cnt || (int)pkt_counters[thread_index].pkt_add_to_jb_cnt != last_pkt_add_to_jb_cnt || pkt_xcode_cnt != last_pkt_xcode_cnt || pkt_pulled_cnt != last_pkt_pulled_cnt || pkt_group_cnt != last_pkt_group_cnt
             #if defined(ENABLE_MULTITHREAD_OPERATION) && !defined(__LIBRARYMODE__)
             || last_multithread_buffered_cnt != num_pkts_buffered_multithread || last_multithread_pkt_write_cnt != pkt_write_cnt_multithread
             #endif
             || pkt_decode_cnt != last_pkt_decode_cnt
            )
         {
            bool fNotZero = false;
            char tabstr[20] = "";

            if (fMediaThread) strcpy(tabstr, "\t\t\t\t\t\t");

            strcpy(tmpstr, "\r");

            if (fMediaThread) {
               int recv = pkt_counters[thread_index].pkt_input_cnt + pkt_counters[thread_index].pkt_read_cnt;
               if (recv >= 1000000L) sprintf(&tmpstr[strlen(tmpstr)], "%sPkts %drcv", tabstr, recv);
               else sprintf(&tmpstr[strlen(tmpstr)], "%sPkts recv %d", tabstr, recv);
            }
            else sprintf(&tmpstr[strlen(tmpstr)], "%sPackets input + read %d", tabstr, pkt_counters[thread_index].pkt_input_cnt + pkt_counters[thread_index].pkt_read_cnt);

            if (!packet_media_thread_info[thread_index].fNoJitterBuffersUsed && (packet_media_thread_info[thread_index].packet_mode || pkt_counters[thread_index].pkt_add_to_jb_cnt)) {

               char bufstr[10];
               if (fMediaThread) strcpy(bufstr, "buf");
               else strcpy(bufstr, "buffered");
               if (fMediaThread && pkt_counters[thread_index].pkt_add_to_jb_cnt >= 1000000L) sprintf(&tmpstr[strlen(tmpstr)], " %d%s", pkt_counters[thread_index].pkt_add_to_jb_cnt, bufstr);
               else sprintf(&tmpstr[strlen(tmpstr)], " %s %d", bufstr, pkt_counters[thread_index].pkt_add_to_jb_cnt);

               if (pkt_counters[thread_index].pkt_add_to_jb_cnt) fNotZero = true;
            }

            if (pkt_pulled_cnt) {

               char jbstr[10];
               if (fMediaThread) strcpy(jbstr, "jb");
               else strcpy(jbstr, "pulled");
               if (fMediaThread && pkt_pulled_cnt >= 1000000L)  sprintf(&tmpstr[strlen(tmpstr)], " %d%s", pkt_pulled_cnt, jbstr);
               else sprintf(&tmpstr[strlen(tmpstr)], " %s %d", jbstr, pkt_pulled_cnt);

               if (pkt_pulled_cnt) fNotZero = true;
            }

            if (pkt_xcode_cnt) {

               char xcodestr[10];
               if (fMediaThread) strcpy(xcodestr, "xc");
               else strcpy(xcodestr, "xcoded");
               if (fMediaThread && pkt_xcode_cnt >= 1000000L)  sprintf(&tmpstr[strlen(tmpstr)], " %d%s", pkt_xcode_cnt, xcodestr);
               else sprintf(&tmpstr[strlen(tmpstr)], " %s %d", xcodestr, pkt_xcode_cnt);

               fNotZero = true;
            }
            else if (pkt_decode_cnt) {

               sprintf(&tmpstr[strlen(tmpstr)], " decoded %d", pkt_decode_cnt);

               fNotZero = true;
            }

            if (pkt_group_cnt) {

               char streamgroupstr[10];
               if (fMediaThread) strcpy(streamgroupstr, "sg");
               else strcpy(streamgroupstr, "group");
               if (fMediaThread && pkt_group_cnt >= 1000000L)  sprintf(&tmpstr[strlen(tmpstr)], " %d%s", pkt_group_cnt, streamgroupstr);
               else sprintf(&tmpstr[strlen(tmpstr)], " %s %d", streamgroupstr, pkt_group_cnt);
            }


            #if defined(ENABLE_MULTITHREAD_OPERATION) && !defined(__LIBRARYMODE__)
            if (num_pkts_buffered_multithread) sprintf(&tmpstr[strlen(tmpstr)], " Other threads buffered %d", num_pkts_buffered_multithread);
            if (pkt_write_cnt_multithread) sprintf(&tmpstr[strlen(tmpstr)], " Other threads transcoded %d", pkt_write_cnt_multithread);
            #endif

            if (pkt_xcode_cnt || pkt_passthru_cnt) {

               if (fMediaThread) {
                  int sent = pkt_counters[thread_index].pkt_output_cnt + pkt_counters[thread_index].pkt_write_cnt;
                  if (sent >= 1000000L) sprintf(&tmpstr[strlen(tmpstr)], " %dsnt", sent);
                  else sprintf(&tmpstr[strlen(tmpstr)], " sent %d", sent);
               }
               else sprintf(&tmpstr[strlen(tmpstr)], " output + written %d", pkt_counters[thread_index].pkt_output_cnt + pkt_counters[thread_index].pkt_write_cnt);
            }

            if (!fNotZero) strcat(tmpstr, "%s  ");  /* add some spaces for readability in case there is an error message printed by one of the libs, which is likely until at least one packet has been buffered or pulled */

            #ifdef ENABLE_STREAM_GROUPS
            if (strlen(szMissingContributors)) {
               strcat(tmpstr, szMissingContributors);
               strcpy(szMissingContributors, "");
            }
            #endif

            sprintf(&tmpstr[strlen(tmpstr)], " mnp %d %d %d", hSession0 >= 0 ? (int)(no_pkt_elapsed_time[hSession0]/1000) : -1, hSession1 >= 0 ? (int)(no_pkt_elapsed_time[hSession1]/1000) : -1, hSession2 >= 0 ? (int)(no_pkt_elapsed_time[hSession2]/1000) : -1);

#if 0
            int sum0 = 0, sum1 = 0, sum2 = 0;

            for (j=0; j<NUM_PKT_STATS; j++) {
               if (hSession0 >= 0) sum0 += avg_pkt_elapsed_time[hSession0][j]; else sum0 = -1000;
               if (hSession1 >= 0) sum1 += avg_pkt_elapsed_time[hSession1][j]; else sum1 = -1000;
               if (hSession2 >= 0) sum2 += avg_pkt_elapsed_time[hSession2][j]; else sum2 = -1000;
            }

            sprintf(&tmpstr[strlen(tmpstr)], " ap %d %d %d", (int)(1.0*sum0/NUM_PKT_STATS/1000), (int)(1.0*sum1/NUM_PKT_STATS/1000), (int)(1.0*sum2/NUM_PKT_STATS/1000));
#endif

            uint32_t pkt_cnt0 = min(pkt_count[hSession0], DELTA_SUM_LENGTH), pkt_cnt1 = min(pkt_count[hSession1], DELTA_SUM_LENGTH), pkt_cnt2 = min(pkt_count[hSession2], DELTA_SUM_LENGTH);
            sprintf(&tmpstr[strlen(tmpstr)], " pd %1.2f %1.2f %1.2f", hSession0 >= 0 && pkt_cnt0 ? 1.0*pkt_delta_sum[hSession0]/pkt_cnt0/1000 : -1, hSession1 >= 0 && pkt_cnt1 ? 1.0*pkt_delta_sum[hSession1]/pkt_cnt1/1000 : -1, hSession2 >= 0 && pkt_cnt2 ? 1.0*pkt_delta_sum[hSession2]/pkt_cnt2/1000 : -1);
            sig_printf(tmpstr, PRN_LEVEL_STATS | PRN_SAME_LINE, thread_index);

            last_pkt_input_cnt = pkt_counters[thread_index].pkt_input_cnt;
            last_pkt_read_cnt = pkt_counters[thread_index].pkt_read_cnt;
            last_pkt_add_to_jb_cnt = pkt_counters[thread_index].pkt_add_to_jb_cnt;
            last_pkt_pulled_cnt = pkt_pulled_cnt;
            last_pkt_xcode_cnt = pkt_xcode_cnt;
             #if defined(ENABLE_MULTITHREAD_OPERATION) && !defined(__LIBRARYMODE__)
            last_multithread_buffered_cnt = num_pkts_buffered_multithread;
            last_multithread_pkt_write_cnt = pkt_write_cnt_multithread;
            #endif
            last_pkt_decode_cnt = pkt_decode_cnt;
            last_pkt_group_cnt = pkt_group_cnt;
         }

#ifndef __LIBRARYMODE__

         if (!fMediaThread) {

            key = toupper(getkey());
            if (key == 'Q')
            {
               printf("\nQ key entered, exiting packet/media processing\n");  /* leave existing status line, including any error messages (don't clear it) */
               run = 0;
               break;
            }
         }
#endif

         prev_display_time = cur_time;
      }

   /* dynamically locate currently active sessions, and store a copy of handles in hSessions_t.  Notes:

      -first call ManageSessions(), which (i) returns number of active sessions, (ii) initializes thread-level per-session items if a session is new (recently created), and (iii) deletes thread-owned sessions marked with delete-pending

      -in thread execution, sessions created externally during the input/buffer and packet/media loops (below) will not be processed until those loops finish and control returns here
   */

      if (packet_media_thread_info[thread_index].fProfilingEnabled) start_profile_time = get_time(USE_CLOCK_GETTIME);

      bool fDebugPass = false;
      bool fAllSessionsDataAvailable = true;

      numSessions = ManageSessions(hSessions_t, pkt_counters, INPUT_PKTS, PULLED_PKTS, &fAllSessionsDataAvailable, thread_index);  /* last two args are #defines depending on ENABLE_PKT_STATS (defined above) */

      if (numSessions >= 1) __sync_xor_and_fetch(&pm_sync[thread_index], 1);  /* toggle pm thread sync flag for any app that needs it */

      if (thread_index == debug_thread && (run == 2 || run == 3)) {  /* check for call to ThreadDebugOutput() request by DSDisplayThreadDebugInfo() API.  Accessing debug info at this point, just after ManageSessions() guarantees accurate info without contention */

         ThreadDebugOutput(hSessions_t, numSessions, 0, debug_thread, run == 2 ? DS_DISPLAY_THREAD_DEBUG_INFO_SCREEN_OUTPUT : DS_DISPLAY_THREAD_DEBUG_INFO_EVENT_LOG_OUTPUT);
         fDebugPass = true;
      }

      if (packet_media_thread_info[thread_index].fProfilingEnabled) end_profile_time = get_time(USE_CLOCK_GETTIME);  /* record profile time for ManageSessions() */
      else end_profile_time = 0;  /* avoid compiler warning */

   /* set cur_time, used for packet input interval timing */

      cur_time = get_time(USE_CLOCK_GETTIME);

   /* measure and record thread CPU usage */

      fPreemptOmit = false;
      uint64_t elapsed_thread_time = 0;
      float last_decode_time, last_encode_time;

      if (prev_thread_CPU_time) {

         elapsed_thread_time = cur_time - prev_thread_CPU_time;

         if (!packet_media_thread_info[thread_index].nChannelWavProc && packet_media_thread_info[thread_index].fFTRTPtime) {  /* N channel wav file processing is post-processing, not real-time, so we don't include this in preemption checks */

            if (elapsed_thread_time > (uint64_t)pktlib_gbl_cfg.uThreadPreemptionElapsedTimeAlarm*1000) {  /* anything more than warning limit (default is 40 msec) we consider that Linux may have pre-empted the thread.  Not good, but we need to know. JHB Jan2020 */

               last_decode_time = packet_media_thread_info[thread_index].decode_time[(packet_media_thread_info[thread_index].decode_time_index-1) & (THREAD_STATS_TIME_MOVING_AVG-1)];
               last_encode_time = packet_media_thread_info[thread_index].encode_time[(packet_media_thread_info[thread_index].encode_time_index-1) & (THREAD_STATS_TIME_MOVING_AVG-1)];

            /* set preemption alarm if (i) total elapsed time - (encode + decode) is more than 1/4 the alarm time or (ii) total elapsed time is more than 3/2 the alarm time.  Notes, JHB Feb2020:

               -the main reason for this is that high capacity encode + decode total time can exceed ptime depending on sampling rate and number of streams with "complex" audio content, for example EVS wideband music. That's not a problem as long as it happens infrequently, due to jitter buffer and stream group FLC
               -this is imperfect for sure -- we could easily be preempted during encode or decode. Probably we need some sort of worst-case estimate so we know what to expect. But that's a little difficult, due to content as mentioned
               -when set, fPreemptOmit omits most profiling, packet delta stats, anything else that expects normal timing
               -note that encode and decode times are always calculated, regardless of whether packet_media_thread_info[].fProfilingEnabled is set
            */

               if ((int64_t)elapsed_thread_time - (last_encode_time + last_decode_time) > 0.25*pktlib_gbl_cfg.uThreadPreemptionElapsedTimeAlarm*1000 ||
                   elapsed_thread_time > (uint64_t)pktlib_gbl_cfg.uThreadPreemptionElapsedTimeAlarm*1500) fPreemptOmit = true;
            }

            packet_media_thread_info[thread_index].max_elapsed_time_thread_preempt = max(elapsed_thread_time, packet_media_thread_info[thread_index].max_elapsed_time_thread_preempt);  /* keep track of worst-case preemption time */
            packet_media_thread_info[thread_index].current_elapsed_time_thread_preempt = elapsed_thread_time;

#if 1
            if (elapsed_thread_time > 20000) __sync_or_and_fetch(&lib_dbg_cfg.uEventLogMode, DS_EVENT_LOG_WARN_ERROR_ONLY);  /* temporarily set event log to warnings and errors only */
            else {

               bool fAllThreadsFast = true;
               for (i=0; i<nPktMediaThreads; i++) if (packet_media_thread_info[i].current_elapsed_time_thread_preempt > 20000) { fAllThreadsFast = false; break; }
               if (fAllThreadsFast) __sync_and_and_fetch(&lib_dbg_cfg.uEventLogMode, ~DS_EVENT_LOG_WARN_ERROR_ONLY);
            }
#endif
         }
         else packet_media_thread_info[thread_index].nChannelWavProc = 0;

         if (!fPreemptOmit && fThreadInputActive && fAllSessionsDataAvailable && !fDebugPass) {

            packet_media_thread_info[thread_index].CPU_time_avg[packet_media_thread_info[thread_index].thread_stats_time_moving_avg_index] = elapsed_thread_time;
            packet_media_thread_info[thread_index].CPU_time_max = max(packet_media_thread_info[thread_index].CPU_time_max, elapsed_thread_time);
         }
      }

   /* was preemption alarm set ? */

      if (fPreemptOmit) {

         sprintf(tmpstr, "WARNING: p/m thread %d has not run for %4.2f msec, may have been preempted, num sessions = %d", thread_index, 1.0*elapsed_thread_time/1000, numSessions);

         sprintf(&tmpstr[strlen(tmpstr)], ", creation history =");
         for (i=0; i<MS_HISTORY_LEN; i++) sprintf(&tmpstr[strlen(tmpstr)], " %d", packet_media_thread_info[thread_index].manage_sessions_creation_history[(i-1) & (MS_HISTORY_LEN-1)]);
         sprintf(&tmpstr[strlen(tmpstr)], ", deletion history =");
         for (i=0; i<MS_HISTORY_LEN; i++) sprintf(&tmpstr[strlen(tmpstr)], " %d", packet_media_thread_info[thread_index].manage_sessions_deletion_history[(i-1) & (MS_HISTORY_LEN-1)]);

      /* encode and decode profiling times are always enabled, as they are the major drivers of thread capacity, and used for some capacity management related decisions, JHB Feb2020 */

         sprintf(&tmpstr[strlen(tmpstr)], ", last decode time = %4.2f", last_decode_time/1000);
         sprintf(&tmpstr[strlen(tmpstr)], ", last encode time = %4.2f", last_encode_time/1000);

         if (packet_media_thread_info[thread_index].fProfilingEnabled) {

            sprintf(&tmpstr[strlen(tmpstr)], ", ms time = %4.2f msec", 1.0*(end_profile_time - start_profile_time)/1000);
            sprintf(&tmpstr[strlen(tmpstr)], ", last ms time = %4.2f", 1.0*packet_media_thread_info[thread_index].manage_time[(packet_media_thread_info[thread_index].manage_time_index-1) & (THREAD_STATS_TIME_MOVING_AVG-1)]/1000);

            sprintf(&tmpstr[strlen(tmpstr)], ", last buffer time = %4.2f", 1.0*packet_media_thread_info[thread_index].buffer_time[(packet_media_thread_info[thread_index].buffer_time_index-1) & (THREAD_STATS_TIME_MOVING_AVG-1)]/1000);
            sprintf(&tmpstr[strlen(tmpstr)], ", last chan time = %4.2f", 1.0*packet_media_thread_info[thread_index].chan_time[(packet_media_thread_info[thread_index].chan_time_index-1) & (THREAD_STATS_TIME_MOVING_AVG-1)]/1000);
            sprintf(&tmpstr[strlen(tmpstr)], ", last pull time = %4.2f", 1.0*packet_media_thread_info[thread_index].pull_time[(packet_media_thread_info[thread_index].pull_time_index-1) & (THREAD_STATS_TIME_MOVING_AVG-1)]/1000);
            sprintf(&tmpstr[strlen(tmpstr)], ", last stream group time = %4.2f", 1.0*packet_media_thread_info[thread_index].group_time[(packet_media_thread_info[thread_index].group_time_index-1) & (THREAD_STATS_TIME_MOVING_AVG-1)]/1000);
         }

         Log_RT(3, "%s \n", tmpstr);
      }

      if (!fPreemptOmit && packet_media_thread_info[thread_index].fProfilingEnabled) {

         packet_media_thread_info[thread_index].manage_time[packet_media_thread_info[thread_index].manage_time_index] = end_profile_time - start_profile_time;
         packet_media_thread_info[thread_index].manage_time_max = max(packet_media_thread_info[thread_index].manage_time_max, end_profile_time - start_profile_time);
         packet_media_thread_info[thread_index].manage_time_index = (packet_media_thread_info[thread_index].manage_time_index + 1) & (THREAD_STATS_TIME_MOVING_AVG-1);
      }

      prev_thread_CPU_time = cur_time;
      num_thread_buffer_packets = 0;
      num_thread_decode_packets = 0;
      num_thread_encode_packets = 0;
      num_thread_group_contributions = 0;

      fThreadInputActive = false;

   /* Interval timing notes:

      1) Default timing is the natural codec frame duration (i.e. one minimum ptime), for example 20 msec for EVS and AMR codecs.  Jitter buffers use RTP timestamps to determine packet arrival time and buffer delay

      2) If frameInterval[] is zero, then FTRT mode is in effect; i.e. "faster than real-time" (analytics mode). Jitter buffers return the next available packet each time it's called.  This allows "packet burst" or reading from UDP or pcap as fast as possible or other scenarios where packets are given to jitter buffers at arbitrary rates
   */

      if (fMediaThread) interval_time = 0;  /* in thread execution the sender is responsible for packet interval timing, either by sending packets remotely over the network, or locally as an app calling DSPushPackets */
      #ifndef __LIBRARYMODE__
      else if (packet_media_thread_info[thread_index].packet_mode) interval_time = frameInterval[0];
      #endif
      else interval_time = 0;  /* frame mode:  no waiting if packets are not added/pulled to/from jitter buffer */

      #ifndef __LIBRARYMODE__

   /* for cmd line execution, look through all input streams (e.g. pcap files) and organize into queues that match sessions.  For each stream we look ahead number of possible channels, as defined by look_ahead above */

      if (!fMediaThread) {

         static int push_cnt[MAX_INPUT_STREAMS] = { 0 };

         if (cur_time > start_time + interval_time*1000*interval_count) {  /* packet input interval has elapsed ?  (all comparisons are in usec) */

            bool fDataAvailable = false;

            for (j=0; j<nInFiles; j++) if (in_type[j] == PCAP) {

//read:
               pkt_len[0] = DSReadPcapRecord(fp_in[j], pkt_in_buf, 0, NULL, link_layer_length[j]);

#if 0  /* no longer needed, done by DSRecvPackets() when S_RECV_PKT_ENABLE_RFC7198_DEDUP flag is applied */
               if (pkt_len[0] > 0) {

                  pyld_type = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDTYPE, pkt_in_buf, pkt_len[0], NULL, NULL);

                  if (pyld_type >= 72 && pyld_type <= 82) goto read;
               }
#endif
               for (i = threadid; i < (int)nSessions_gbl; i += nThreads_gbl) {

                  hSession = get_session_handle(hSessions_t, i, thread_index);

                  if (pkt_len[0] > 0) {

get_pkt_info:
                     chnum_parent = DSGetPacketInfo(hSession, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_CHNUM_PARENT | DS_PKT_INFO_SUPPRESS_ERROR_MSG, pkt_in_buf, pkt_len[0], NULL, NULL);

                     if (chnum_parent >= 0) {

                        DSPushPackets(DS_PUSHPACKETS_FULL_PACKET, pkt_in_buf, pkt_len, &hSession, 1);  /* if packet matches a stream, i.e. a channel defined in any session termN, then push to correct session queue */

                        push_cnt[j]++;
                     }
//                     #ifdef NEW_REUSE_CODE
//                     else if (fReuseInputs)
                     #ifdef PERFORMANCE_MEASUREMENT
                     else if (performanceMeasurementMode == 1)
                     {
                        ReuseInputs(pkt_in_buf, pkt_len[0], i, &session_data_t[i]);
                        goto get_pkt_info;
                     }
                     #endif
                  }

               /* for cmd line execution, data available is false if (i) no further input of any type is available, including pcap files or UDP sockets, and (ii) once that happens, if all pushed packets have been consumed.  Note 
                  note that for thread exeuction, it's up to the user app, which has to "flush" each session to indicate no further data available, depending on whatever application dependent criteria it needs to use */
  
                  fDataAvailable = false;
                  for (int k=0; k<nInFiles; k++) if (in_type[k] == PCAP && !feof(fp_in[k])) fDataAvailable = true;
                  if (!fDataAvailable && !DSPushPackets(DS_PUSHPACKETS_GET_QUEUE_STATUS, NULL, NULL, &hSession, 1)) fDataAvailable = true;

                  session_info_thread[hSession].fDataAvailable = fDataAvailable;
               }
            }

            //#define DEBUGDATAAVAILABLE
            #ifdef DEBUGDATAAVAILABLE

            static bool prev_fDataAvailable = true;

            if (!fDataAvailable && prev_fDataAvailable) {

               printf("+++++++++++++++ data available goes false, chnum_map[hSession, i] =");
               for (j=0; j<MAX_TERMS; j++) printf(" %d", session_info_thread[hSession].chnum_map[j]);
               printf(" push count =");
               for (j=0; j<nInFiles; j++) printf(" %d", push_cnt[j]);
               printf("\n");
            }

            prev_fDataAvailable = fDataAvailable;
            #endif
         }
      }
      #endif

      if (packet_media_thread_info[thread_index].fProfilingEnabled) {
         input_time = 0;
         buffer_time = 0;
      }

   /* receive and buffer incoming packets:

      -receive using DSRecvPackets(), which contains packets either (i) pushed by an external app in thread execution, or (ii) pushed by code immediately above that sorts through streams specified on the cmd line (i.e. each -i cmd line option specifies a pcap file or network socket)
      -add to jitter buffer (unless disabled in session config)
   */

      int look_ahead = MAX_TERMS; /* look ahead N packets, where N is the max number of channels per session */

      for (i = threadid; i < (fMediaThread ? numSessions : (int)nSessions_gbl); i += nThreads_gbl) {

         static bool fOnce[MAX_PKTMEDIA_THREADS] = { false };
         if (!fOnce[thread_index]) {
            Log_RT(7, "INFO: First thread session input check, p/m thread = %d, fMediaThread = %d, i = %d, numSessions = %d\n", thread_index, fMediaThread, i, numSessions);
            fOnce[thread_index] = true;
         }

         hSession = get_session_handle(hSessions_t, i, thread_index);

         if (hSession == -1) continue;  /* session handle not found */

         if (packet_media_thread_info[thread_index].fProfilingEnabled) start_profile_time = get_time(USE_CLOCK_GETTIME);

         if (lib_dbg_cfg.uDebugMode & DS_ENABLE_PUSHPACKETS_ELAPSED_TIME_ALARM) {

            if (!(session_input_flags[hSession] & 1) && session_last_push_time[hSession] && ((int64_t)cur_time - (int64_t)__sync_fetch_and_add(&session_last_push_time[hSession], 0))/1000 >= lib_dbg_cfg.uPushPacketsElapsedTimeAlarm) {  /* note -- need to use int64_t calculation here as DSPushPackets() is an asynchronous (user) thread and might update session_last_push_time[] after we update cur_time, JHB Jan2020 */

//  printf(" ================= cur_time = %llu, session_last_push_time[hSession] = %llu \n", cur_time, session_last_push_time[hSession]); 

               Log_RT(3, "WARNING: p/m thread %d says DSPushPackets() has pushed no packets for session %d for %d msec \n", thread_index, hSession, lib_dbg_cfg.uPushPacketsElapsedTimeAlarm);
               __sync_lock_test_and_set(&session_last_push_time[hSession], cur_time);  /* update the push time so we won't log another warning message for another uPushPacketsElapsedTimeAlarm number of msec */
            }
         }

#if 0
         if ((progress_var[hSession] & 1) == 0) {
            printf("&&&&&& inside input/buffer loop\n");
            progress_var[hSession] |= 1;
         }
#endif

         if (session_info_thread[hSession].fDataAvailable) {  /* reset channel markers of this sessions's channels (not including dynamic channels) */

            for (j=0; j<MAX_TERMS; j++) session_info_thread[hSession].chnum_map[j] = -1;
         }

         if (cur_time > start_time + interval_time*1000*interval_count) {  /* packet buffering interval has elapsed ?  (all comparisons are in usec) */

            pkts_read[i] = 0;  /* init number of packets read to zero for this session */
            bool fNoLookAhead = false;

            #if 0
            if (thread_index == 1) {
               static bool fOnce = false;
               if (!fOnce) { printf("========== thread 1 hSession = %d, hSessions_t[0] = %d, hSessions_t[1] = %d, hSessions_t[2] = %d\n", hSession, hSessions_t[0], hSessions_t[1], hSessions_t[2]); fOnce = true; }
            }
            #endif

            numPkts = 0;

         /* if bidirectional traffic flag is set, then "look ahead" into the receive packet queue and see if we can pull packets for both termination endpoints:
         
            -at stream initialization, we wait up to 200 msec for bidirectional streams to align. This handles one stream starting with a SID and the other not (assuming SID delta of 160 msec)
            -note if the bidirectional flag is not set then avoiding look-ahead increases receive queue performance, JHB Apr2020
          */

            if (DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_TERM_FLAGS, 2, NULL) & TERM_EXPECT_BIDIRECTIONAL_TRAFFIC) {

               numPkts = DSRecvPackets(hSession, DS_RECV_PKT_QUEUE | DS_RECV_PKT_QUEUE_COPY | DS_RECV_PKT_FILTER_RTCP | DS_RECV_PKT_ENABLE_RFC7198_DEDUP, pkt_in_buf, sizeof(pkt_in_buf), pkt_len, look_ahead);  /* look ahead N packets */

               if (!session_info_thread[hSession].look_ahead_time && numPkts < look_ahead && cur_time - session_info_thread[hSession].init_time < 200000L) goto next_session;  /* wait to obtain look-ahead number of packets at start of stream flow for this session. Stop waiting if it doesn't happen for 100 msec after the session is initialized */
               else session_info_thread[hSession].look_ahead_time = cur_time - session_info_thread[hSession].init_time;
            }

            if (numPkts == 0) {

            /* if unidirectional traffic, or look-ahead says no packets in the queue, then we read without look-ahead */

               #define PKTS_TO_READ 1  /* up to 4 tested in telecom mode, works fine. But so far have not found a reason/advantage to use it */

               numPkts = DSRecvPackets(hSession, DS_RECV_PKT_QUEUE | DS_RECV_PKT_FILTER_RTCP | DS_RECV_PKT_ENABLE_RFC7198_DEDUP, pkt_in_buf, sizeof(pkt_in_buf), pkt_len, PKTS_TO_READ);
               fNoLookAhead = true;
            }

            if (numPkts && !fThreadInputActive) fThreadInputActive = true;  /* any packet for any session sets input active flag */

            if (!fPreemptOmit && session_info_thread[hSession].fDataAvailable) { 

               uint64_t elapsed_time = cur_time - last_packet_time[hSession];

               if (numPkts == 0 && last_packet_time[hSession]) {

                  if (elapsed_time > no_pkt_elapsed_time[hSession]) no_pkt_elapsed_time[hSession] = elapsed_time;  /* check for new session mnp */
               }
               else {

                  if (last_packet_time[hSession]) {  /* numPkts is non-zero */

                     int index = pkt_sum_index[hSession];
                     int oldest_value = pkt_delta_runsum[hSession][index];
                     pkt_delta_runsum[hSession][index] = elapsed_time;

                     pkt_delta_sum[hSession] += elapsed_time - oldest_value;

                     pkt_sum_index[hSession] = (index + 1) & (DELTA_SUM_LENGTH-1);
                     pkt_count[hSession]++;
                  }

                  last_packet_time[hSession] = cur_time;
               }

               if (i == 0) { hSession0 = hSession; hSession1 = -1; hSession2 = -1; }
               if (i == 1) hSession1 = hSession;
               if (i == 2) hSession2 = hSession;
            }

         /* numStreams is determined dynamically per session as the number of channels matching an input packet from any source.  Notes:

            1) Currently the number of channels per session is 1 or 2, depending on termN definitions (TERMINATION_INFO) defined for each session.  Note that MAX_TERMS is currently defined as 2, but may increase in the future

            2) Input always flows through per-session receive queues.  The following input sources push packets to the receive queue:

              -pcap files and UDP ports in cmd line execution
              -external applications in thread execution (arbitrary, application defined input sources)

            3) Each pkt/media thread has its own receive queue.  For coCPU hardware, a thread typically equals a CPU core

            4) For an example of an application using the per session receive queues, see the mediaMin app, which reads pcap files and pushes packets to the queues, which are then processed here by one or more packet_flow_media_proc() threads
         */

            int chnums_lookahead[MAX_TERMS*4];
            memset(chnums_lookahead, 0xff, sizeof(chnums_lookahead));  /* set all chnum to -1 */

            hSession_flags = (uFlags_session(hSession) & DS_SESSION_USER_MANAGED) ? hSession : -1;

            pkt_ptr = pkt_in_buf;
            numStreams = 0;
            numPkts_matched = 0;

//  static int count = 0;
//  if (numPkts > 1 && count++ < 50) printf("\n === session %d buffering %d pkts \n", hSession, numPkts);

            for (j=0; j<numPkts; j++) {  /* find channels matching this session */

               chnums_lookahead[j] = DSGetPacketInfo(hSession_flags, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_CHNUM_PARENT | DS_PKT_INFO_SUPPRESS_ERROR_MSG, pkt_ptr, pkt_len[j], NULL, &chnums[j]);
               pkt_ptr += pkt_len[j];

               if (chnums_lookahead[j] >= 0) {  /* channel matches ? */

                  if (!numStreams) numStreams++;
                  else {

                     bool fChanAlreadyMatched = false;
                     int k = 1;

                     do {

                        if (chnums_lookahead[j] == chnums_lookahead[j-k]) {

                           fChanAlreadyMatched = true;
                           packet_in_bursts[chnums_lookahead[j]]++;  /* update same-channel burst stat */
                        }

                     } while (j - ++k >= 0);

                     if (!fChanAlreadyMatched) numStreams++;  /* increment numStreams if channels are unique (i.e. different channels of same session -- for example term1 and term2 active for bidirectional traffic) */
                  }

                  numPkts_matched++;
               }
            }

  #if 1
  static bool fOnce2[MAX_PKTMEDIA_THREADS][8] = {{ false }};
  if (i < 8 && !fOnce2[thread_index][i]) {
     sprintf(tmpstr, "look ahead pkts = %d, time = %llu, chnums[0] = %d, chnums[1] = %d, hSession = %d, uFlags = 0x%x\n", numPkts, session_info_thread[hSession].look_ahead_time, chnums_lookahead[0], chnums_lookahead[1], hSession, uFlags_session(hSession));
     sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);
     fOnce2[thread_index][i] = true;
  }
  #endif

         /* if numPkts reflects only look-ahead (copy) packets at this point, then receive numStreams packets from queue specified by hSession, if available:

            -if fNoLookAhead is true then numPkts represents packets actually pulled at this point, and we don't call DSRecvPackets() again
            -even if numStreams is zero, we still try to pull one even if there is no match, in case the queue contains wrong packets for this session (i.e. user error), we never want the queue to overflow
         */

            if (!fNoLookAhead && numPkts > 0) numPkts = DSRecvPackets(hSession, DS_RECV_PKT_QUEUE | DS_RECV_PKT_FILTER_RTCP | DS_RECV_PKT_ENABLE_RFC7198_DEDUP, pkt_in_buf, sizeof(pkt_in_buf), pkt_len, max(numStreams, 1));

            if (isMasterThread) uQueueRead ^= 1;

            #if 1
            static int fOnce[8] = { 0 };
            if (isMasterThread && i < 8 && fOnce[i] < 1) {
               #ifndef __LIBRARYMODE__
               if (!fMediaThread) printf("numStreams = %d, numSessions = %d, nSessions_gbl = %d, numPkts = %d, nInFiles = %d\n", numStreams, numSessions, nSessions_gbl, numPkts, nInFiles);
               else
               #endif
               sprintf(tmpstr, "numStreams = %d, numSessions = %d, numPkts = %d\n", numStreams, numSessions, numPkts);
               sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);
               fOnce[i]++;
            }
            #endif

         /* loop through packet list and buffer channels */

            pkt_ptr = pkt_in_buf;

#ifdef DEBUGINPUTPACKETS  /* chnum-to-session mapping trace that can be very helpful in debugging multistream issues */
            bool fdbg;
            if (numPkts == 0) {
               chnum_parent = -1;
               goto debug;
            }
#endif

         /* input time profiling, if enabled */

            if (!fPreemptOmit && packet_media_thread_info[thread_index].fProfilingEnabled) {

               end_profile_time = get_time(USE_CLOCK_GETTIME);
               input_time += end_profile_time - start_profile_time;
               start_profile_time = end_profile_time;
            }

            for (j=0; j<numPkts; j++) {

            /* get parent chnum (channel number) that matches the packet.  Notes, JHB Feb2019:

               -termN channels are parent channels.  The DS_PKT_INFO_CHNUM_PARENT flag tells DSGetPacketInfo() to ignore SSRC when hashing the packet
               -chnum is returned as the full packet match (i.e. including SSRC).  If chnum != chnum_parent, then chnum is a child channel.  A parent channel can have multiple child channels, depending on the number of stream (SSRC) changes (refer to RFC8108)
               -note that both DSBufferPackets() and DSGetOrderedPackets() expect parent chnums, and will automatically handle child chnums belonging to the parent
               -chnums_lookahead[] and chnums[] usage saves time -- for channels we've already matched there is no need to call DSGetPacketInfo() again, JHB Jan2020
             */

               if (j < numPkts_matched) {
                  chnum_parent = chnums_lookahead[j];  /* parent (and child if applicable) already filled in, JHB Jan2020 */
                  chnum = chnums[j];
               }
               else chnum_parent = DSGetPacketInfo(hSession_flags, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_CHNUM_PARENT | DS_PKT_INFO_SUPPRESS_ERROR_MSG, pkt_ptr, pkt_len[j], NULL, &chnum);

               if (lib_dbg_cfg.uEnablePktTracing & DS_PACKET_TRACE_RECEIVE) DSLogPktTrace(hSession_flags, pkt_ptr, pkt_len[j], thread_index, (lib_dbg_cfg.uEnablePktTracing & ~DS_PACKET_TRACE_MASK) | DS_PACKET_TRACE_RECEIVE);

               pyld_type = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDTYPE, pkt_ptr, pkt_len[j], NULL, NULL);

#ifdef DEBUGINPUTPACKETS  /* chnum-to-session mapping trace that can be very helpful in debugging multistream issues in FTRT mode */
debug:
               fdbg = false;

               if (numPkts > 0 && (pyld_type >= 72 && pyld_type <= 82)) {

                  pkt_chnum[pkt_chnum_ctr] = -2;
                  fdbg = true;
               }
               else if (numPkts > 0 && chnum_parent >= 0) {

                  pkt_chnum[pkt_chnum_ctr] = chnum_parent;
                  fdbg = true;
               }
               else pkt_chnum[pkt_chnum_ctr] = -3;

               if (fdbg) {

                  pkt_chnum_len[pkt_chnum_ctr] = pkt_len[j];
                  pkt_chnum_i[pkt_chnum_ctr] = i;
                  pkt_chnum_hSession[pkt_chnum_ctr] = hSession;
                  pkt_chnum_numStreams[pkt_chnum_ctr] = numStreams;
                  pkt_chnum_buftime[pkt_chnum_ctr] = cur_time;

                  pkt_chnum_ctr++;
               }

               if (numPkts == 0) goto next_session;
#endif
               pkt_counters[thread_index].pkt_read_cnt++;  /* increment read count */

               if (pyld_type >= 72 && pyld_type <= 82)  goto next_session;  /* filter out RTCP packets.  Add other filters here */

               #ifdef OVERWRITE_INPUT_DATA
               if (fReuseInputs && !ReuseInputs(pkt_ptr, pkt_len[j], hSession, &session_data_t[i])) continue;
               #endif

               #ifdef ALLOW_BACKGROUND_PROCESS  /* use APIs to manage packets */
               if (!use_bkgnd_process)
               #endif
               {

               /* set channel mapping for packet's parent channel.  Notes:

                  -channel numbers are unique, also session handle + term is unique.  In either case we are effectively "mapping" the stream (packet header contents) to channel space.  We use the latter as all loops are session based
                  -see get_channels() for chnum_map[] usage
                  -chnum_map[] values are reset at start of each session loop (see above)
                  -chnum_map_history[] keeps track of all terms used by a session over the course of its lifespan
               */

                  int term;

                  if (chnum_parent >= 0) {

                     term = DSGetSessionInfo(chnum_parent, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_TERM, 0, NULL);  /* find which endpoint TERMINATION_INFO of this session matches the channel number */
                     if (term < 1) continue;  /* channel may have been deleted */
                     else term -= 1;

                     if (session_info_thread[hSession].chnum_map[term] == -1) {

                        if (session_info_thread[hSession].chnum_map_history[term] == -1) {  /* has the channel been marked at least once for this session ? */

                           session_info_thread[hSession].num_streams_active++;  /* keep track of per-session active streams */

                           packet_media_thread_info[thread_index].num_streams_active++;  /* keep track of active streams across all sessions */

                           session_info_thread[hSession].chnum_map_history[term] = 0; /* it has been marked once */
                        }
                     }

                     session_info_thread[hSession].chnum_map[term] = chnum_parent;  /* mark this channel as in use */

                     #if 0  /* on-hold flush state machine clears itself.  If we clear it here because child stream packets are arriving, we can stop parent stream flush before it's complete, JHB Jan2200 */ 
                     nOnHoldChanFlush[hSession][term] = 0;  /* reset session's on-hold flush state */
                     nOnHoldChan[hSession][term] = 0;
                     #endif
                  }
                  else continue;  /* chnum_parent <= -1 is an error condition */

                /* In packet mode, DSBufferPackets() adds packets to a jitter buffer.  Packet mode notes:

                    1) DSBufferPackets() expects packet headers in network byte order by default.  The DS_PKT_INFO_HOST_BYTE_ORDER flag can be used if headers are in host byte order

                    2) DTX packets are processed, out-of-order packets are handled, and packets duplicated per RFC7198 are stripped out

                    3) For pcap input, DS_BUFFER_PKT_DISABLE_OVERRUN can be set to disable jitter buffer overrun detection, which occurs when the RTP timestamp window is larger than the max delay. 
                       This effectively removes jitter buffer max depth. This flag is typically not used for live network input

                    4) The DS_BUFFER_PKT_DISABLE_PROBATION flag is also intended only for pcap input. This avoids some aspects of jitter buffer "priming" and minmizes loss of initial packets in the pcap.

                  In frame mode, DSBufferPackets() is not called, and payloads are extracted from packets in whatever order and format in which they are read from pcap or received from network socket.  Frame mode notes:

                    1) DTX, out-of-order packets, RFC7198, and SSRC changes are not handled.  If packet input is not a sequential, non-duplicated, single RTP stream representation of original encoded audio frame data
                       prior to network transmission, then resulting decoded audio may contain artifacts or even be completely unintelligible

                    2) Log output, if enabled, does not include jitter buffer stats
                */

                  if (packet_media_thread_info[thread_index].packet_mode) {  /* packet mode */

                  /* Session handle notes for DSBufferPackets():

                       1) -1 tells DSBufferPackets() to match incoming packets with previously defined sessions using a hash method (based on src/dst IP addr:port values), then add the packets to the jitter
                          buffer for that session

                       2) A specific session handle adds packets only for that session

                       3) If the session was created with the DS_SESSION_USER_MANAGED flag, then the session handle _must_ specify the correct session, otherwise the packet won't be added to the buffer.  Situations
                          where DS_SESSION_USER_MANAGED can be useful include creation of sessions with partial or all duplication of termN IP addr:port values with other sessions

                       4) Multithread operation has no impact on any of these methods 
                  */

                     if (session_info_thread[hSession].fUseJitterBuffer) {

                        uFlags_add = DS_BUFFER_PKT_IP_PACKET | DS_BUFFER_PKT_DISABLE_PROBATION;

                        #if 0  /* deduplication is handled by DSPushPackets() or DSRecvPackets(). The DS_BUFFER_PKT_ENABLE_RFC7198_DEDUP flag is deprecated but it would still work if for some reason the other APIs are not being used */
                        uFlags_add |= DS_BUFFER_PKT_ENABLE_RFC7198_DEDUP;
                        #endif

                     /* for DTX handling with possible pcap input, we remove jitter buffer depth limit. For real-time traffic, this is not needed.  For pcap input packets can be input faster than real-time
                        and the buffer may contain a large range of timestamps at any given time, and has to handle a wider range of possible packet problems (gaps, timestamp jumps, large re-orderings, etc)
                        that would not ordinarily occur in normal network socket input.  If for any reason buffer depth must be strictly limited, this flag should not be set */

//                        if (session_info_thread[hSession].fDTXEnabled) {  /* this is failing with DTX disabled in FTRT mode.  Need to ask Chris to look into it, JHB Oct2018 */

                           uFlags_add |= DS_BUFFER_PKT_ALLOW_DYNAMIC_DEPTH;
//                        }

                        int session_state = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_STATE, 0, NULL);
                        /*if (session_state & DS_SESSION_STATE_ALLOW_TIMSTAMP_JUMP)*/ uFlags_add |= DS_BUFFER_PKT_ALLOW_TIMESTAMP_JUMP;  /* use this flag to ignore large positive timestamp jumps, for example long pauses in streams due to call waiting or on-hold, manual pcap manipulation, etc */
                        if (session_state & DS_SESSION_STATE_ALLOW_DYNAMIC_ADJUST) uFlags_add |= DS_BUFFER_PKT_ENABLE_DYNAMIC_ADJUST;  /* use this flag to enable dynamic adjust of target delay, based on incoming packets */

                        packet_len[0] = pkt_len[j];  /* fill in first packet_len[] item with total bytes to process, after call packet_len[] will contain lengths of all packets found to be correctly formatted, meeting all matching criteria, and added to the buffer */

                        #ifdef FIRST_TIME_TIMING
                        static bool fOnce = false;
                        if (!fOnce) { printf("\n === time from first push to first buffer %llu \n", (unsigned long long)((first_buffer_time = get_time(USE_CLOCK_GETTIME)) - first_push_time)); fOnce = true; }
                        #endif

                        ret_val = DSBufferPackets(hSession_flags, uFlags_add, pkt_ptr, packet_len, &payload_info[j], &chnum);  /* buffer one or more packets for this session, applying flags as required. chnum[] is returned as the packet match, either parent or child, as applicable, JHB Jan2020 */

                        if (ret_val > 0) {

                           pkts_read[i] += ret_val;
                           pkt_counters[thread_index].pkt_submit_to_jb_cnt += ret_val;
                           pkt_counters[thread_index].pkt_add_to_jb_cnt += ret_val;

                           #ifdef PACKET_TIME_STATS  /* we wait until this point to record packet time stats because we need the chnum for either parent or child.  For a child we don't know it until created by DSBufferPackets() */
                           if (lib_dbg_cfg.uPktStatsLogging & DS_ENABLE_PACKET_TIME_STATS && !fPreemptOmit) RecordPacketTimeStats(chnum, pkt_ptr, packet_len[0], pkt_count[hSession], PACKET_TIME_STATS_INPUT);
                           #endif

                        /* mark session's ssrc state as live upon buffering a packet, record channel's most recent buffer time */

                           if (!nDormantChanFlush[hSession][term]) session_info_thread[hSession].ssrc_state[term] = SSRC_LIVE;

                           last_buffer_time[chnum] = cur_time;
                        }
                        else {

                           pkt_counters[thread_index].pkt_submit_to_jb_cnt++;  /* packet not added or error condition -- we tried to submit at least one packet, so increment this counter */

                           session_status = DSGetSessionStatus(hSession);

                        /* Act on error and/or warning conditions from DSBufferPackets().  Notes:

                           1) DSBufferPackets returns -1 for error conditions and the number of successfully packets added for warning conditions, which requires checking the API's most recent status to know if
                              anything went wrong.  Warning conditions are defined as one or more packets to be added, but not all, have some issue that prevents them from being added.  An example might be a bit
                              error that causes header validation to fail

                           2) Which warning conditions to act on is application-dependent. In this case we treat RTP validation as an error situation, since if one packet from a pcap RTP stream is wrong, then
                              likely all packets for that stream have the same problem (other RTP streams in the pcap may be fine, which this example handles).  For live input that might not be the case, so
                              this is just an example
                         */

                           if (ret_val < 0 || (ret_val == 0 && session_status == DS_BUFFER_PKT_ERROR_RTP_VALIDATION)) {

                              int API_codes = DSGetAPIStatus((uintptr_t)NULL);

                              sprintf(tmpstr, "Error condition %d for input stream %d, failed to add packet %d to jitter buffer, API identifiers and codes = 0x%x, numPkts = %d\n", session_status, i, pkt_counters[thread_index].pkt_submit_to_jb_cnt, API_codes, numPkts);
                              sig_printf(tmpstr, PRN_LEVEL_ERROR, thread_index);

                              session_info_thread[hSession].chnum_map[term] = -1;
                           }

                           if (ret_val == 0 && session_status == DS_BUFFER_PKT_SEQ_DUPLICATE) continue;  /* duplicate packet found, don't update interval timing */
                        }
                     }
                     else {

                        uFlags_add = 0;  /* avoid compiler warning */
                        pkts_read[i] += 1;
                        packet_len[j] = pkt_len[j];
                        payload_info[j] = DSGetPacketInfo(hSession_flags, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLD_CONTENT, pkt_ptr, pkt_len[j], NULL, NULL);
                        ret_val = 1;  /* ok to check for SSRC change and log packet */
                     }

                     uFlags_info = DS_BUFFER_PKT_IP_PACKET;

                     if (session_info_thread[hSession].fUseJitterBuffer && (uFlags_add & DS_PKT_INFO_HOST_BYTE_ORDER)) uFlags_info |= DS_PKT_INFO_HOST_BYTE_ORDER;  /* if user app packet headers are in host byte order, then adjust uFlags for subsequent DSGetPacketInfo() calls.  mediaTest doesn't do this, so this line is here just as an example */
                  }
                  else {  /* frame mode */

                     pkts_read[i] += 1;
                     packet_len[j] = pkt_len[j];
                     payload_info[j] = 0;
                     ret_val = 1;  /* ok to check for SSRC change and log packet */

                     uFlags_info = DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_NETWORK_BYTE_ORDER;  /* RTP header items still in network byte order */
                  }

                  unsigned int pkt_ctrs[3]; pkt_ctrs[0] = pkt_counters[thread_index].pkt_input_cnt; pkt_ctrs[1] = pkt_counters[thread_index].pkt_read_cnt; pkt_ctrs[2] = pkt_counters[thread_index].pkt_add_to_jb_cnt;
                  int nSSRC_change;

                  if (ret_val > 0 && (nSSRC_change = CheckForSSRCChange(hSession, &chnum_parent, pkt_ptr, &pkt_len[j], 1, uFlags_info, uFlags_session(hSession), pkt_ctrs, thread_index)) > 0) {  /* look for an SSRC change in the input stream, set fSSRC_Change[] if found */

                     session_info_thread[hSession].fSSRC_change_active[term] = true;

                     ResetPktStats(hSession);  /* reset session packet stats */

                     if (nSSRC_change == 1) {  /* note - nSSRC_change is 2 for a resuming stream */

                        session_info_thread[hSession].num_streams_active++;  /* increment number of active streams */
                        packet_media_thread_info[thread_index].num_streams_active++;

                     /* the current thinking is no action is needed here, as both parent and child will be acted on by continued calls to DSGetOrderedPackets(), and the parent jitter buffer will end up fully flushed, JHB Jan2020:

                        -in fact this is what we see -- there are no parent packets "left over" when we look at cumulative input vs. jitter buffer output packet times and rtp times
                        -this is a different situation than a stream that simply ends or goes on-hold, which is handled by pastdue processing
                        -side notes:
                          -DSGetOrderedPackets() when given a parent chnum acts on both parent and children, if any.  The DS_GETORD_PKT_CHNUM_PARENT_ONLY flag is implemented, but not tested yet
                          -if given a child chnum, DSGetOrderedPackets() acts only on the child
                     */

                        #if 0
                        int chnum_parent = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_CHNUM_PARENT, 0, NULL);
                        nOnHoldChan[hSession][term] = chnum_parent+1;  /* force a full jitter buffer flush on the parent channel, otherwise min delay packets will be left over */
                        #endif
                     }

                     #ifdef ENABLE_STREAM_GROUPS
                     HSESSION hSessionOwner = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_OWNER, 0, NULL);

                     if (hSessionOwner >= 0) {

                        uShowGroupContributorAmounts[hSessionOwner] = 1;  /* tell streamlib to report contributor info after each stream change (only displays if debug stats are enabled) */
                        #if 0
                        DSResetContributorBuffer(hSessionOwner, chnum);
                        #endif
                     }
                     #endif
                  }

                  #ifdef ENABLE_PKT_STATS
                  uint8_t uPktStatsLogging;

                  #ifdef USE_CHANNEL_PKT_STATS

                  if ((uPktStatsLogging = DSIsPktStatsHistoryLoggingEnabled(thread_index))) {

                     if (ret_val > 0 || (uPktStatsLogging & DS_LOG_BAD_PACKETS)) { 

                     /* fill in session and stream group info.  Note that group index (idx) may be -1 if session does not belong to a stream group, JHB Dec2019 */

                        input_pkts[chnum].pkt_stats->chnum = chnum;
                        input_pkts[chnum].pkt_stats->idx = chnum_group_map[chnum]-1;

                     /* add packet stats entry */

                        int num_stats = DSPktStatsAddEntries(input_pkts[chnum].pkt_stats, ret_val >= 0 ? ret_val : 1, pkt_ptr, &pkt_len[j], &payload_info[j], uFlags_info);  /* log input packets; for multiple input streams the DS_PKTSTATS_LOG_COLLATE_STREAMS flag is used (see below) */
                        pkt_counters[thread_index].num_input_pkts += num_stats;

                        manage_pkt_stats_mem(input_pkts, chnum, num_stats);
                  #else

                  if (isMasterThread && (uPktStatsLogging = DSIsPktStatsHistoryLoggingEnabled(thread_index))) {

                     if (ret_val > 0 || (uPktStatsLogging & DS_LOG_BAD_PACKETS)) { 

                     /* fill in session and stream group info.  Note that group index (idx) may be -1 if session does not belong to a stream group, JHB Dec2019 */

                        input_pkts[pkt_counters[thread_index].num_input_pkts].chnum = chnum;
                        input_pkts[pkt_counters[thread_index].num_input_pkts].idx = DSGetStreamGroupInfo(chnum, DS_GETGROUPINFO_HANDLE_CHNUM, NULL, NULL, NULL);

                     /* add packet stats entry */

                        pkt_counters[thread_index].num_input_pkts += DSPktStatsAddEntries(&input_pkts[pkt_counters[thread_index].num_input_pkts], ret_val >= 0 ? ret_val : 1, pkt_ptr, &pkt_len[j], &payload_info[j], uFlags_info);  /* log input packets; for multiple input streams the DS_PKTSTATS_LOG_COLLATE_STREAMS flag is used (see below) */

                        if (pkt_counters[thread_index].num_input_pkts >= MAX_PKT_STATS) {
                           Log_RT(4, "INFO: input packet stats array exceeds %d packets, packet log will likely show missing SSRCs and/or packets \n", MAX_PKT_STATS);
                           pkt_counters[thread_index].num_input_pkts = 0;  /* wrap */
                        }
                  #endif
                     }
                  }
                  #endif
               }

               #ifdef ALLOW_BKGND_PROCESS
               else {  /* send packets to background process */

               }
               #endif

               pkt_ptr += pkt_len[j];

               num_thread_buffer_packets++;

            }  /* end of j..numPkts-1 loop */

         /* buffer time profiling, if enabled */

            if (!fPreemptOmit && packet_media_thread_info[thread_index].fProfilingEnabled) {

               end_profile_time = get_time(USE_CLOCK_GETTIME);
               buffer_time += end_profile_time - start_profile_time;
            }

next_session:
            i = i;
         }
      }  /* end of input/buffering loop */

      if (!fPreemptOmit && packet_media_thread_info[thread_index].fProfilingEnabled) {

         if (input_time > 0) {
            packet_media_thread_info[thread_index].input_time[packet_media_thread_info[thread_index].input_time_index] = input_time;
            packet_media_thread_info[thread_index].input_time_max = max(packet_media_thread_info[thread_index].input_time_max, input_time);
            packet_media_thread_info[thread_index].input_time_index = (packet_media_thread_info[thread_index].input_time_index + 1) & (THREAD_STATS_TIME_MOVING_AVG-1);
         }

         if (buffer_time > 0) {
            packet_media_thread_info[thread_index].buffer_time[packet_media_thread_info[thread_index].buffer_time_index] = buffer_time;
            packet_media_thread_info[thread_index].buffer_time_max = max(packet_media_thread_info[thread_index].buffer_time_max, buffer_time);
            packet_media_thread_info[thread_index].buffer_time_index = (packet_media_thread_info[thread_index].buffer_time_index + 1) & (THREAD_STATS_TIME_MOVING_AVG-1);
         }
      }

   /* Energy saver mode handling */

      if (!fThreadInputActive && !fThreadOutputActive && last_packet_time_thread) {

         uint64_t no_pkt_elapsed_time_thread = cur_time - last_packet_time_thread;

         packet_media_thread_info[thread_index].max_inactivity_time = max(no_pkt_elapsed_time_thread, packet_media_thread_info[thread_index].max_inactivity_time);

      /* check if thread has been inactive, any sent packets have been pulled from app queues */

         if (pktlib_gbl_cfg.uThreadEnergySaverInactivityTime > 0) {  /* a zero value disables energy saver state */

            if (no_pkt_elapsed_time_thread > pktlib_gbl_cfg.uThreadEnergySaverInactivityTime*1000) {  /* comparison is in usec */

               bool app_queues_empty_check = true;

               if (pktlib_gbl_cfg.uThreadEnergySaverWaitForAppQueuesEmptyTime &&
                   no_pkt_elapsed_time_thread - pktlib_gbl_cfg.uThreadEnergySaverInactivityTime*1000 > pktlib_gbl_cfg.uThreadEnergySaverWaitForAppQueuesEmptyTime*1000) {

                  for (i = threadid; i < (fMediaThread ? numSessions : (int)nSessions_gbl); i += nThreads_gbl) {

                     hSession = get_session_handle(hSessions_t, i, thread_index);

                     if (DSPullPackets(DS_PULLPACKETS_GET_QUEUE_STATUS | DS_PULLPACKETS_TRANSCODED | DS_PULLPACKETS_JITTER_BUFFER, NULL, NULL, hSession, NULL, 0, 0) == 0) {
                        app_queues_empty_check = false;
                        break;
                     }
                  }
               }

               if (app_queues_empty_check) {  /* all energy saver state conditions met ? */

                  if (packet_media_thread_info[thread_index].nEnergySaverState == THREAD_RUN_STATE) {  /* enter energy saver state if not already there */

                     packet_media_thread_info[thread_index].nEnergySaverState = THREAD_ENERGY_SAVER_STATE;
                     packet_media_thread_info[thread_index].energy_saver_state_count++;  /* keep track of number of times thread enters energy saver state */
                     int count = packet_media_thread_info[thread_index].energy_saver_state_count;
                     Log_RT(4, "INFO: Packet/media thread %d entering energy saver state after inactivity time %d sec (has entered %d time%s, max recorded inactivity time = %d sec)\n", thread_index, (int)(no_pkt_elapsed_time_thread/1000000L), count, count > 1 ? "s" : "", (int)( packet_media_thread_info[thread_index].max_inactivity_time/1000000L));
                  }

                  usleep(pktlib_gbl_cfg.uThreadEnergySaverSleepTime);  /* sleep as specified.  Note that CPU usage percentage is essentially a ratio of what an inactive p/m thread does to check for input (i.e. "floor" CPU usage) and the sleep time. The floor time measures around 500 usec, so a sleep time of 500 usec (approx 50 sessions active) gives about 50% CPU usage, JHB Jan2019 */
               }
            }
         }
         else packet_media_thread_info[thread_index].nEnergySaverState = THREAD_RUN_STATE;  /* energy saver state is disabled, so regardless of what's happening with input/output activity, we stay in full run state */
      }
      else {

         last_packet_time_thread = cur_time;
         packet_media_thread_info[thread_index].nEnergySaverState = THREAD_RUN_STATE;
      }

      if (cur_time > start_time + interval_time*1000*interval_count) interval_count++;

   /* incoming packets -- first look for available network socket packets, if UDP socket input is enabled */

      if (fNetIOAllowed && isMasterThread) {
         #ifdef USE_PKTLIB_NETIO
         recv_len =  DSRecvPackets(0, DS_RECV_PKT_FILTER_RTCP, pkt_in_buf, MAX_RTP_PACKET_LEN, NULL, 1);  /* use Pktlib API */
         #else
         recv_len = recv(recv_sock_fd, pkt_in_buf, MAX_RTP_PACKET_LEN, 0);  /* user defined socket */
         #endif
      }
      else recv_len = 0;

      #if NONBLOCKING
      if (recv_len > 0)
      {
      #endif
         struct iphdr* ip_hdr = (struct iphdr *)pkt_in_buf;

         /* filter out packets with source port not in the range used for this test; base port is set to src port specific on command line, if no port is specific default = 10240, 4 ports per session */
         /* don't filter packets in mediaTest for now, rely on pktlib matching any udp packets with a previously created channel, mismatch error message is disabled through previous DSConfigPktlib() call */
         /*udp_hdr = (struct udphdr *)(pkt_in_buf + ip_hdr->ihl * 4);
         if (((ntohs(udp_hdr->source) - MediaParams[0].Streaming.udpPort_src) < 0) || (ntohs(udp_hdr->source) - MediaParams[0].Streaming.udpPort_src >= (nSessions * 4))) continue; */ 

         packet_length = ntohs(ip_hdr->tot_len);

         if (packet_length > 0) {

            pkt_counters[thread_index].pkt_input_cnt++;

            packet_len[0] = packet_length;  /* fill in first packet_len[] item with total bytes to process, after call packet_len[] will contain lengths of all packets found to be correctly formatted, meeting all matching criteria, and added to the buffer */

            ret_val = DSBufferPackets(-1, DS_BUFFER_PKT_IP_PACKET, pkt_in_buf, packet_len, payload_info, NULL);

            if (ret_val > 0) {
               pkt_counters[thread_index].pkt_submit_to_jb_cnt += ret_val;
               pkt_counters[thread_index].pkt_add_to_jb_cnt += ret_val;
            }
            else pkt_counters[thread_index].pkt_submit_to_jb_cnt++;  /* error condition -- we tried to submit at least one packet, so increment this counter */
         }
         else ret_val = -1;

         if (ret_val < 0) Log_RT(3, "WARNING: failed to add network socket packet to jitter buffer\n");

         #ifdef ENABLE_PKT_STATS
         uint8_t uPktStatsLogging;

         #ifdef USE_CHANNEL_PKT_STATS
         if ((uPktStatsLogging = DSIsPktStatsHistoryLoggingEnabled(thread_index))) {
         
            if (ret_val > 0 || (uPktStatsLogging & DS_LOG_BAD_PACKETS)) { 

            /* fill in session and stream group info.  Note that group index (idx) may be -1 if session does not belong to a stream group, JHB Dec2019 */

               input_pkts[NCORECHAN].pkt_stats->chnum = -1;
               input_pkts[NCORECHAN].pkt_stats->idx = -1;

            /* add packet stats entry */

               int num_stats = DSPktStatsAddEntries(input_pkts[NCORECHAN].pkt_stats, ret_val >= 0 ? ret_val : 1, pkt_in_buf, packet_len, payload_info, DS_BUFFER_PKT_IP_PACKET);

               pkt_counters[thread_index].num_input_pkts += num_stats;
               manage_pkt_stats_mem(input_pkts, NCORECHAN, num_stats);

         #else

         if (isMasterThread && (uPktStatsLogging = DSIsPktStatsHistoryLoggingEnabled(thread_index))) {

            if (ret_val > 0 || (uPktStatsLogging & DS_LOG_BAD_PACKETS)) { 

            /* fill in session and stream group info.  Note that group index (idx) may be -1 if session does not belong to a stream group, JHB Dec2019 */

               input_pkts[pkt_counters[thread_index].num_input_pkts].chnum = -1;
               input_pkts[pkt_counters[thread_index].num_input_pkts].idx = -1;

            /* add packet stats entry */

               pkt_counters[thread_index].num_input_pkts += DSPktStatsAddEntries(&input_pkts[pkt_counters[thread_index].num_input_pkts], ret_val >= 0 ? ret_val : 1, pkt_in_buf, packet_len, payload_info, DS_BUFFER_PKT_IP_PACKET);
               if (pkt_counters[thread_index].num_input_pkts >= MAX_PKT_STATS) pkt_counters[thread_index].num_input_pkts = 0;
         #endif
            }
         }
         #endif

#if 0  /* fDataAvailable is now session based, so not sure what to do about this one.  JHB Aug2018 */
         fDataAvailable = true;
#endif
      #if NONBLOCKING
      }
      #endif

#ifdef ALLOW_BACKGROUND_PROCESS

      if (!use_bkgnd_process)  /* use APIs to manage packets */
#endif
      {

         fThreadOutputActive = false;

         if (packet_media_thread_info[thread_index].fProfilingEnabled) {
            chan_time = 0;
            pull_time = 0;
            group_time = 0;
         }

         encode_time = 0;
         decode_time = 0;

      /* In packet mode (either network socket or pcap input), DSGetOrderedPackets() pulls ordered packets from jitter buffers.  Notes:

         -multiple packets might be available in the same time window (i.e. in code below ret_val might be > 1)
         -the DS_GETORD_PKT_FLUSH flag is set when all input (queues, pcap files, UDP ports) has been exhausted, for example a pcap file runs out or when closing a live traffic session. This flag pushes out any remaining jitter buffer packets
      */

         for (i = threadid; i < (fMediaThread ? numSessions : (int)nSessions_gbl); i += nThreads_gbl) {  /* note - threadid is zero and nThreads_gbl is 1 in thread mode */

            hSession = get_session_handle(hSessions_t, i, thread_index);

            if (hSession == -1) continue;

            if (packet_media_thread_info[thread_index].fProfilingEnabled) start_profile_time = get_time(USE_CLOCK_GETTIME);

            num_chan = get_channels(hSession, stream_indexes, chan_nums, thread_index);

            num_chan = CheckForDormantSSRC(hSession, num_chan, chan_nums, numSessions, threadid, hSessions_t, cur_time, thread_index);

            num_chan = CheckForOnHoldFlush(hSession, num_chan, chan_nums);

            num_chan = CheckForPacketLossFlush(hSession, num_chan, chan_nums, cur_time, thread_index);  /* in analytics mode (clockless, or FTRT mode), if last time DSGetOrderedPackets() was called exceeds N ptimes, then call again */

            if (!num_chan) continue;  /* no jitter buffer output, decode/transcode, media processing if (i) this session currently has no input streams that map to channels, or (ii) active channel has no packets available in jitter buffer */

            hSession_flags = (uFlags_session(hSession) & DS_SESSION_USER_MANAGED) ? hSession : -1;

            #if 0
            static bool fOnce[MAX_SESSIONS] = { false };
            if (!fOnce[hSession]) {

               sprintf(tmpstr, "++++++hSession[%d]= %d, num_chan = %d", i, hSession, num_chan);
               for (j=0; j<num_chan; j++) sprintf(&tmpstr[strlen(tmpstr)], " chan_nums[%d] = %d", j, chan_nums[j]);

               strcat(tmpstr, "\n");
               sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);
               fOnce[hSession] = true;
            }
            #endif

            if (!fPreemptOmit && packet_media_thread_info[thread_index].fProfilingEnabled) {

               end_profile_time = get_time(USE_CLOCK_GETTIME);
               chan_time += end_profile_time - start_profile_time;
               start_profile_time = end_profile_time;
            }

         /* Pull packets from the buffer for decode, output to application queues, pcap / wav file write, and logging.  Notes:

            1) This example uses the DS_GETORD_PKT_CHNUM flag for each active channel.  This approach of pulling packets from the buffer separately per channel allows flags such as DS_GETORD_PKT_FTRT,
               DS_GETORD_PKT_ENABLE_DTX, DS_GETORD_PKT_ENABLE_DTMF, and DS_GETORD_PKT_RETURN_ALL_DELIVERABLE to be applied on per channel basis

            2) This is only one example; packets can be pulled for (i) all currently existing sessions, (ii) one session (matching packet contents and/or session handle if user-managed sessions are
               active), or (iii) single channel only. When to use which cases is application-dependent
         */

            n = 0;

            do {

               if (chan_nums[n] >= 0) {

               /* note:  n does not correspond to term-1, for example session term1 may be inactive and term2 active, in which case n = 0 and term-1 = 1.  This is why we call DSGetSessionInfo() with DS_SESSION_INFO_TERM, JHB Sep2019 */

                  int term = DSGetSessionInfo(chan_nums[n], DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_TERM, 0, NULL); 
                  if (term < 1) continue;  /* channel may have been deleted */
                  else term -= 1;

                  #if 0  /* move this to buffer add side so that session's ssrc state is marked as live when a packet is buffered w/o error. This is (i) independent of jitter buffer target delay (for example a short session that never primes, or takes ) and (ii) compatible with both analytics and telecom modes, JHB Apr2020 */
                  if (!nDormantChanFlush[hSession][term]) session_info_thread[hSession].ssrc_state[term] = SSRC_LIVE;
                  #endif

                  #ifdef WAV_OUT_DEBUG_PRINT
                  static bool fChanPrint[10] = false;
                  if (!fChanPrint[n]) { printf("  proc chan = %d\n", n); fChanPrint[n] = true; }
                  #endif

                  #ifndef __LIBRARYMODE__
                  pcap_index = get_pcap_index(i);  /* map session handle to output pcap file indexes */
                  wav_index = get_wav_index(hSessions_t, i, n, thread_index);  /* map session handle and channel number to output wav file indexes */
                  #endif

                  if (packet_media_thread_info[thread_index].packet_mode) {  /* packet mode */

                     if (session_info_thread[hSession].fUseJitterBuffer) {

                     /* set flags for DSGetOrderedPackets() */

                        bool fFlushChan = !session_info_thread[hSession].fDataAvailable || nDormantChanFlush[hSession][term] || nOnHoldChanFlush[hSession][term];
                        bool fParentOnly = nDormantChanFlush[hSession][term] || nOnHoldChanFlush[hSession][term];

                        #if 0  /* not needed - flushing a parent channel when it still has packets in its jitter buffer after creating a child channel */
                        if (DSGetJitterBufferInfo(chan_nums[n], DS_JITTER_BUFFER_INFO_NUM_PKTS) && DSGetSessionInfo(chan_nums[n], DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_DYNAMIC_CHANNELS, 0, NULL)) fFlushChan = true;
                        #endif
   
                        uFlags_get = DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_NETWORK_BYTE_ORDER | (fFlushChan ? DS_GETORD_PKT_FLUSH : 0) | (fParentOnly ? DS_GETORD_PKT_CHNUM_PARENT_ONLY : 0);

                     /* DS_GETORD_PKT_FTRT flag notes:

                       1) FTRT means "faster than real-time", aka "clockless" or data-driven mode for applications with encapsulated packet formats, such as data analytics and LI

                       2) Primary use cases include (i) clockless applications, where RTP streams are encapsulated or otherwise transported independently of actual RTP timing, and (ii) when the interval between packets
                          (calls to DSBufferPackets()) are less than the ptime of the incoming packet stream.  Application examples of the first case include data analytics and lawful interception.  In the latter case 
                          packets are being added to a jitter buffer faster than they normally would be in real-time. The add interval can be anything from "as fast as possible" to some percentage less than the expected ptime (e.g. every 19.5 msec for 20 msec ptime)

                       3) When the DS_GETORD_PKT_FTRT flag is set, on the first call to DSGetOrderedPackets() for each channel, the jitter buffer's internal timestamp offset value is initialized to a time value
                          equivalent to -N (negative N) packets, where N is calculated using jitter buffer delay settings 
                     */

                        if (input_buffer_interval[hSession][term] < ptime[hSession][term]) uFlags_get |= DS_GETORD_PKT_FTRT;

                        #define FTRTDEBUG
                        #ifdef FTRTDEBUG
                        static bool fOnce[MAX_SESSIONS][MAX_TERMS] = {{ false }};
                        if (!fOnce[hSession][term]) {
                           sprintf(tmpstr, "chan_nums[%d] = %d, num_chan = %d, hSession = %d, term = %d, input_buffer_interval = %d, ptime = %d, timing = %s%s\n", n, chan_nums[n], num_chan, hSession, term, input_buffer_interval[hSession][term], ptime[hSession][term], (uFlags_get & DS_GETORD_PKT_FTRT) ? "analytics" : "telecom", DSGetJitterBufferInfo(chan_nums[n], DS_JITTER_BUFFER_INFO_TARGET_DELAY) <= 7 ? ", compatibilty mode" : "");
                           sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);
                           fOnce[hSession][term] = true;
                        }
                        #endif

                     /* DTX handling and DS_GETORD_PKT_RETURN_ALL_DELIVERABLE flag notes:

                        1) DTX handling is enabled by default. It can be controlled by the TERM_DTX_ENABLE flag in TERMINATION_INFO struct uFlags element (see shared_include/session.h). Typically the flag is set either by application code or session config file

                        2) When DTX handling is not enabled, the DS_GETORD_PKT_RETURN_ALL_DELIVERABLE flag is used force all deliverable packets from the previous stream if a new SSRC stream has started.  For DTX handling enabled case, DTX handling inside
                           DSGetOrderedPackets() already does this

                        3) The DS_GETORD_PKT_RETURN_ALL_DELIVERABLE flag forces any packets currently in a jitter buffer to be delivered, given at least one packet is currently in the deliverable time window.  This is different than flushing 
                           (DS_DSGETORD_PKT_FLUSH) which forces out all packets regardless
                     */

                        unsigned int uFlags_term = DSGetSessionInfo(chan_nums[n], DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_TERM_FLAGS, 0, NULL);

                        if (session_info_thread[hSession].fSSRC_change_active[term] && !(uFlags_term & TERM_DTX_ENABLE)) uFlags_get |= DS_GETORD_PKT_RETURN_ALL_DELIVERABLE;

                     /* Enable DTMF event handling in DSGetOrderedPackets().  See the check below for payload_info[] = DS_PKT_PYLD_CONTENT_DTMF */

                        uFlags_get |= DS_GETORD_PKT_ENABLE_DTMF;

                     /* enable ooo holdoff, see comments in pktlib.h. The mediaMin application sets the TERM_OOO_HOLDOFF_ENABLE flag by default when creating sessions */

                        if (uFlags_term & TERM_OOO_HOLDOFF_ENABLE) uFlags_get |= DS_GETORD_PKT_ENABLE_OOO_HOLDOFF;

                     /* DSGetOrderedPackets() notes:

                        1) If the DS_GETORD_PKT_SESSION flag is not given, a handle argument of -1 tells DSGetOrderedPackets() to retrieve all packets deliverable in the current time window for all currently active sessions

                        2) If DS_GETORD_PKT_SESSION flag is given, the handle argument retrieves packets only for that session.  If the session was created with the DS_SESSION_USER_MANAGED flag, then the session
                           handle may specify sessions with partial or all duplication of termN IP addr:port values with other sessions

                        3) If the DS_GETORD_PKT_CHNUM flag is given, the handle argument specifies a channel and retrives packets only for that channel (a session may have more than one channel)

                        4) If a session's TERMINATION_INFO endpoint channels have created dynamic channels due to RTP stream (SSRC) changes, then whatever flags are used in the DSGetOrderedPackets() call are also applied
                           to the dynamic channels (per RFC8108). Note that dynamic channels are also referred to as child channels
                     */

                        pkt_ptr = recv_jb_buffer;
                        payload_info[0] = 0;

                        if (!fDisplayActiveChannels[hSession]) {
                           DisplayChanInfo(hSession, num_chan, chan_nums, thread_index);  /* one-time display/debug output per session */
                           fDisplayActiveChannels[hSession] = true;
                        }

                        uint32_t ptr_ofs = 0;
                        int ch[64] = { 0 };
                        int pull_pkts, offset = 0, num_ch = 1, nRePull = 0;
                        unsigned int uInfo = 0;
                        num_pkts = 0;
                        bool fRePull;

                        #define USE_CHAN_NUMS
                        #define USE_CHNUM
pull:
                        #ifdef USE_CHAN_NUMS

                        num_pkts += (pull_pkts = DSGetOrderedPackets(chan_nums[n], uFlags_get | DS_GETORD_PKT_CHNUM, cur_time, pkt_ptr + ptr_ofs, &packet_len[num_pkts], &payload_info[num_pkts], &uInfo));  /* other options for the handle param include (i) -1 value to retrieve all packets for all sessions and (ii) a specific hSession combined with DS_GETORD_PKT_SESSION */

                        #else

                        num_pkts += (pull_pkts = DSGetOrderedPackets(hSession, uFlags_get | DS_GETORD_PKT_SESSION, cur_time, pkt_ptr + ptr_ofs, &packet_len[num_pkts], &payload_info[num_pkts], &uInfo));

                        #endif

                     /* jitter buffer pulls notes, JHB Apr2020:

                        -jitter buffer depth can continue to grow in overrun situations; i.e. the application is pushing packets at a sustained rate exceeding packet ptime (e.g. average packet deltas less than 20 msec)
                        -we need to control jitter buffer memory usage, so we establish nominal packet depth limits, one for analytics mode, another for telecom mode
                        -since analytics mode is data driven, timing is controlled by buffer add time (which causes a buffer pull attempt) so the analytics mode limit is based on a number of packets; i.e. the currently configured max jitter buffer delay. The telecom mode limit is set to an arbitarily large time value
                        -once we hit these limits we start "pulling from future time" to maintain the limit and let streamlib contributor management and other mechanisms handle overrun
                        -to do this, we set the "timestamp advance" flag (DS_GETORD_PKT_ADVANCE_TIMESTAMP), and pull one or more additional times, which causes DSGetOrderedPackets() to look ptime into the future of the channel's jitter buffer
                        -each channel can overrun (hit the limit) independently. streamlib overrun management handles contributor synchronization in overrun situations
                        -jitter buffer packet depth limit is arbitrary. For telecom mode currently the default is 50 ptimes -- let's say there are 512 sessions, each with 3 G711 channels (no SIDs), that would be about 20 MB of mem, so not a problem. Other codecs would be much less 
                        -at session creation time the packet depth limit can be specified with the max_depth_ptimes element of the JITTER_BUFFER_CONFIG struct, which is part of the TERMINATION_INFO struct (both defined in shared_include/session.h)
                        -at run-time the packet depth limit can be specified using DSSetJitterBufferInfo() or directly in TERMINATION_INFOs' JITTER_BUFFER_CONFIG structs 
                     */

                        #ifdef FIRST_TIME_TIMING
                        static bool fOnce2 = false;
                        if (!fOnce2 && pull_pkts == 1) { printf("\n === time from first buffer to first pull %llu \n", (unsigned long long)((first_pull_time = get_time(USE_CLOCK_GETTIME)) - first_buffer_time)); fOnce2 = true; }
                        #endif

#if 1
                        if (!fFlushChan && (uInfo & DS_GETORD_PKT_INFO_PULLATTEMPT)) do {

                           fRePull = false;
                           bool fFlush = false, fLevel = false;
                           int numpkts, chan;

                           ch[0] = chan_nums[n];
                           num_ch += DSGetSessionInfo(chan_nums[n], DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_DYNAMIC_CHANNELS, 0, (void*)&ch[num_ch]);  /* retrieve child channels, if any. All channels are in ch[], starting with parent. (Note - need typecast here, otherwise compiler messes up.  Not sure why) JHB Feb2019 */

                           for (j=0; j<num_ch; j++) {  /* search channel and child channels, if any */

                           /* for analytics mode the depth limit is max delay number of packets, for telecom mode the limit is an amount of time delay. In telecom mode comparing with time delay (not packets) is required to correctly handle large input gaps (test with 6537.0). See also above comments */

                              numpkts = DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_NUM_PKTS);
                              int nTargetDelay = DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_TARGET_DELAY);
                              int nMaxDelay = DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_MAX_DELAY);

                              if (
                                  ((uFlags_get & DS_GETORD_PKT_FTRT) && (fLevel = numpkts > nTargetDelay) && nTargetDelay > 7) ||
                                  ((uFlags_get & DS_GETORD_PKT_FTRT) && (fLevel = numpkts > nMaxDelay) && nTargetDelay <= 7) ||  /* level flush added for analytics compatibility mode. Verify with test cases 5280.0.ws, 5281.0.ws, 13041.0, JHB May2020 */
                                  (!(uFlags_get & DS_GETORD_PKT_FTRT) && ((fFlush = DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_CUMULATIVE_TIMESTAMP) < DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_CUMULATIVE_PULLTIME) && (cur_time - last_pull_time[chan_nums[n]] + 500)/1000 > (uint32_t)ptime[hSession][term] && numpkts > nTargetDelay) ||
                                                                          (fLevel = DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_DELAY) > DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_MAX_DEPTH_PTIMES))))
                                 ) {

//  printf("\n === ch %d delay = %d, num pkts = %d, max timestamp gap = %d \n", ch[j], DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_DELAY), DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_NUM_PKTS), DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_MAX_TIMESTAMP_GAP));

                                 if ((nRePull == 0 && fFlush) || (nRePull < 50 && !fFlush)) {  /* impose some limit on number of repulls to avoid excessive time spent */

                                    chan = ch[j];
                                    fRePull = true;
                                    break;
                                 }
                              }
                              #if 0
                              else {
                                 static int count = 0;
                                 int delta = (cur_time - last_pull_time[chan_nums[n]] + 500)/1000;
                                 if (cum_timestamp[ch[j]] < cum_timeDelta[ch[j]] && delta >= 20 && count++ < 100) printf("\n === ch %d pull time delta = %d, numpkts = %d \n", chan_nums[n], delta, numpkts);
                              }
                              #endif
                           }

                           if (fRePull) {

                              if (fFlush) {

                                 #if 0
                                 static uint64_t basetime = 0;
                                 if (!basetime) basetime = cur_time;
                                 printf("\n === ch %d flush time = %d (usec) %d (msec), cur time - last pull time = %d \n", chan, cur_time - basetime, (cur_time - basetime)/1000, (cur_time - last_pull_time[chan_nums[n]])/1000);
                                 #endif
  
                                 uFlags_get |= DS_GETORD_PKT_FLUSH;
                                 uFlags_get &= ~DS_GETORD_PKT_ADVANCE_TIMESTAMP;  /* advance jitter buffer pull timestamp */
                              }
                              else {

                                 #if 0
                                 printf("\n === level repull ch %d, num pkts = %d, delay ptimes = %d, fFlush = %d, nRePull = %d \n", chan_nums[n], numpkts, DSGetJitterBufferInfo(chan, DS_JITTER_BUFFER_INFO_DELAY), fFlush, nRePull);
                                 #endif

                                 uFlags_get |= DS_GETORD_PKT_ADVANCE_TIMESTAMP;  /* advance both clock timestamp and jitter buffer pull timestamp. See comments in pktlib.h */
                                 uFlags_get &= ~DS_GETORD_PKT_FLUSH;
                              }

                              for (j=0; j<pull_pkts; j++) ptr_ofs += packet_len[offset+j];
                              offset = num_pkts;

                              if (fFlush) pkt_loss_flush[chan]++;  /* increment packet loss flush stat */
                              if (fLevel) pkt_level_flush[chan]++;  /* increment packet level flush stat */

                              nRePull++;
                              goto pull;
                           }

                        } while (fRePull);
#endif

                        if (num_pkts < 0) {
                           sprintf(tmpstr, "Error retrieving packet(s) from jitter buffer for session %d\n", i);
                           sig_printf(tmpstr, PRN_LEVEL_ERROR, thread_index);
                           continue;
                        }

                     /* record last pull time for this channel */

                        if (uFlags_get & DS_GETORD_PKT_FTRT) last_pull_time[chan_nums[n]] = cur_time;
                        else if (num_pkts) last_pull_time[chan_nums[n]] = cur_time;  /* in telecom mode we go by whether a packet was returned, JHB Apr2020 */

                        if (session_info_thread[hSession].fDataAvailable && fFlushChan) {

                           int numpkts = DSGetJitterBufferInfo(chan_nums[n], DS_JITTER_BUFFER_INFO_NUM_PKTS);
                           int min_delay = DSGetJitterBufferInfo(chan_nums[n], DS_JITTER_BUFFER_INFO_MIN_DELAY);

                           if (numpkts <= min_delay) DSSetJitterBufferInfo(chan_nums[n], DS_JITTER_BUFFER_INFO_UNDERRUN_RESYNC_WARNING, min_delay);  /* set warning omit-count to min delay, JHB Feb2019 */
                        }
#if 0
                        if (num_pkts == 0 && session_info_thread[hSession].fDataAvailable) {

                           if ((uFlags_get & DS_GETORD_PKT_FTRT) || payload_info[0] == DS_PKT_PYLD_CONTENT_PROBATION) {  /* look for special case where packets exist but are available yet because nominal jitter buffer delay has not been reached (jitter buffer not yet "primed") */

                              payload_info[0] = DS_PKT_PYLD_CONTENT_PROBATION;
                              num_pkts = 1;
                           }
                        }
#endif
                     }
                     else {  /* jitter buffer not used, set num_pkts and pkt_ptr; packet_len[] and payload_info[] remain set from input stream loop above */

                        num_pkts = pkts_read[stream_indexes[n]];
                        pkt_ptr = pkt_in_buf;
                     }
                  }
                  else {  /* frame mode, set num_pkts and pkt_ptr; packet_len[] and payload_info[] are not used in frame mode */

                     uFlags_get = DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_NETWORK_BYTE_ORDER;

                     num_pkts = pkts_read[stream_indexes[n]];
                     pkt_ptr = pkt_in_buf;
                  }

               /* pull time profiling, if enabled */

                  if (!fPreemptOmit && packet_media_thread_info[thread_index].fProfilingEnabled) {

                     end_profile_time = get_time(USE_CLOCK_GETTIME);
                     pull_time += end_profile_time - start_profile_time;
                     start_profile_time = end_profile_time;
                  }

               /* for subsequent DSGetPacketInfo() calls make sure uFlags_info is set correctly, depending on whatever is being used with DSGetOrderedPackets() */

                  uFlags_info = DS_BUFFER_PKT_IP_PACKET;

                  if (uFlags_get & DS_PKT_INFO_NETWORK_BYTE_ORDER) uFlags_info |= DS_PKT_INFO_NETWORK_BYTE_ORDER;
                  else uFlags_info |= DS_PKT_INFO_HOST_BYTE_ORDER;

                  #define NO_PACKET         0  /* some local definitions helpful in illustrating possible packet processing cases */
                  #define MEDIA_PACKET      1
                  #define DTMF_PACKET       2
                  #define PROBATION_PACKET  3

               /* note:  from this point forward, if jitter buffer is not used then pkt_ptr, packet_len, payload_info, and num_pkts all refer to pcap or input stream values */

                  media_data_len = 0;  /* length of decoded media frame, in bytes */
                  hCodec = (intptr_t)NULL;
                  int prev_chnum = -1;

               /* packet payload processing loop. A jitter buffer may have returned multiple packets representing multiple channels */
  
                  for (j=0; j<num_pkts; j++) {

                     int packet_type = MEDIA_PACKET;  /* default */

                  /* find the channel number matching the buffer output packet (could be parent or child), and error check */

                     if ((chnum = DSGetPacketInfo(hSession_flags, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_CHNUM, pkt_ptr, packet_len[j], NULL, NULL)) < 0) {

                        Log_RT(2, "ERROR: p/m thread %d says chum not found; failed to match packet header to a channel, chan_nums[%d] = %d, packet_len[%d] = %d \n", thread_index, n, chan_nums[n], j, packet_len[j]);  /* Note -- if user managed sessions are active, then hSession cannot be -1 */
                        break;
                     }

                     if (payload_info[j] != DS_PKT_PYLD_CONTENT_PROBATION) {

                        rtp_ofs = DSGetPacketInfo(-1, DS_PKT_INFO_RTP_PYLDOFS | uFlags_info, pkt_ptr, packet_len[j], NULL, NULL);

                        if (rtp_ofs < 0) {
                           Log_RT(2, "ERROR: p/m thread %d says invalid packet pointer or length given to DSGetPacketInfo(), packet len = %d, num pkts = %d\n", thread_index, packet_len[j], num_pkts);
                           break;
                        }

                        if (packet_media_thread_info[thread_index].packet_mode && session_info_thread[hSession].fUseJitterBuffer) {

                           if (lib_dbg_cfg.uEnablePktTracing & DS_PACKET_TRACE_JITTER_BUFFER) DSLogPktTrace(hSession_flags, pkt_ptr, packet_len[j], thread_index, (lib_dbg_cfg.uEnablePktTracing & ~DS_PACKET_TRACE_MASK) | DS_PACKET_TRACE_JITTER_BUFFER);

                           #ifdef PACKET_TIME_STATS
                           if (lib_dbg_cfg.uPktStatsLogging & DS_ENABLE_PACKET_TIME_STATS && !fPreemptOmit) RecordPacketTimeStats(chnum, pkt_ptr, packet_len[j], 0, PACKET_TIME_STATS_PULL);
                           #endif

                           pkt_pulled_cnt++;

                           #ifdef ENABLE_PKT_STATS

                           #ifdef USE_CHANNEL_PKT_STATS
                           if (DSIsPktStatsHistoryLoggingEnabled(thread_index)) {

                           /* fill in session and stream group info.  Note that group index (idx) may be -1 if session does not belong to a stream group, JHB Dec2019 */

                              pulled_pkts[chnum].pkt_stats->chnum = chnum;
                              pulled_pkts[chnum].pkt_stats->idx = chnum_group_map[chnum]-1;

                           /* add packet stats entry */

                              int num_stats = DSPktStatsAddEntries(pulled_pkts[chnum].pkt_stats, 1, pkt_ptr, &packet_len[j], &payload_info[j], uFlags_info);  /* we log all buffer output packets; for multiple output streams the DS_PKTSTATS_LOG_COLLATE_STREAMS flag is used (see below) */

                              pkt_counters[thread_index].num_pulled_pkts += num_stats;

                              manage_pkt_stats_mem(pulled_pkts, chnum, num_stats);

                           #else

                           if (isMasterThread && DSIsPktStatsHistoryLoggingEnabled(thread_index)) {

                           /* fill in session and stream group info.  Note that group index (idx) may be -1 if session does not belong to a stream group, JHB Dec2019 */

                              pulled_pkts[pkt_counters[thread_index].num_pulled_pkts].chnum = chnum;
                              pulled_pkts[pkt_counters[thread_index].num_pulled_pkts].idx = DSGetStreamGroupInfo(chnum, DS_GETGROUPINFO_HANDLE_CHNUM, NULL, NULL, NULL);

                           /* add packet stats entry */

                              pkt_counters[thread_index].num_pulled_pkts += DSPktStatsAddEntries(&pulled_pkts[pkt_counters[thread_index].num_pulled_pkts], 1, pkt_ptr, &packet_len[j], &payload_info[j], uFlags_info);  /* we log all buffer output packets; for multiple output streams the DS_PKTSTATS_LOG_COLLATE_STREAMS flag is used (see below) */

                              if (pkt_counters[thread_index].num_pulled_pkts >= MAX_PKT_STATS) {
                                 Log_RT(4, "INFO: pulled packet stats array exceeds %d packets, packet log will likely show missing SSRCs and/or packets \n", MAX_PKT_STATS);
                                 pkt_counters[thread_index].num_pulled_pkts = 0;
                              }
                           #endif
                           }
                           #endif

                           if (fMediaThread) {

//  if (chnum == 6) printf(" === chnum 6 before send jb pkt, num_pkts = %d, pktlen = %d, \n", num_pkts, packet_len[0]);

                              DSSendPackets((HSESSION*)&hSession, DS_SEND_PKT_QUEUE | DS_PULLPACKETS_JITTER_BUFFER | DS_SEND_PKT_SUPPRESS_QUEUE_FULL_MSG, pkt_ptr, &packet_len[j], 1);  /* send jitter buffer output packet */
                           }
                        }

                        pyld_ptr = pkt_ptr + rtp_ofs;
                        pyld_len = DSGetPacketInfo(-1, DS_PKT_INFO_RTP_PYLDLEN | uFlags_info, pkt_ptr, packet_len[j], NULL, NULL);
                     }
                     else {

                        packet_type = PROBATION_PACKET;

                        pyld_ptr = pkt_ptr;  /* only here to avoid compiler warnings about possible uninit vaar */
                        pyld_len = 0;
                     }

                  /* Get term info for outgoing endpoint and error check.  We do this here because in a few cases (like DTMF) we might need to know how the packet will be treated if it should be forwarded or transcoded for output */

                     if (chnum != prev_chnum && DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM, 2, &termInfo_link) < 0) {
                        Log_RT(2, "ERROR: p/m thread %d says failed to get chnum %d terminfo for term2 \n", thread_index, chnum);
                        break;
                     }

                  /* Check if packet is DTMF event */

                     if (payload_info[j] == DS_PKT_PYLD_CONTENT_DTMF || payload_info[j] == DS_PKT_PYLD_CONTENT_DTMF_SESSION) {  /* check for DTMF packets, both generic and matching session-defined DTMF payload types, JHB May2019 */ 

                        int dtmf_display_msg_limit;

                        if (termInfo_link.attr.voice_attr.dtmf_mode & DS_DTMF_RTP) {

                           packet_type = DTMF_PACKET;  /* DTMF event packet, pass through to opposite direction term (if incoming termN.dtmf_type field session config is set to send RTP events) */
                           dtmf_display_msg_limit = 1;
                        }
                        else {

                           packet_type = NO_PACKET;  /* no packet to process, print some DTMF event info */
                           dtmf_display_msg_limit = 24;
                        }

                        struct dtmf_event dtmf_info = { 0 };

                        DSGetDTMFInfo(-1, (uintptr_t)NULL, pyld_ptr, pyld_len, &dtmf_info);

                        if (isMasterThread && uDisplayDTMFEventMsg[hSession][term] < dtmf_display_msg_limit) {  /* arbitrary limit on how many DTMF events to display during normal operation; to see all events look at the packet stats log generated when the program exits */

                           uDisplayDTMFEventMsg[hSession][term]++;

                           if (uDTMFState[hSession][term] == 0) {
                              strcpy(tmpstr, "\n");
                              uDTMFState[hSession][term] = 1;
                           }
                           else strcpy(tmpstr, "");

                           sprintf(&tmpstr[strlen(tmpstr)], "DTMF Event packet %u received @ pkt %d", uDisplayDTMFEventMsg[hSession][term], pkt_pulled_cnt);

                           if (packet_type == DTMF_PACKET) strcat(tmpstr, ", will be forwarded to output");
                           if (uDisplayDTMFEventMsg[hSession][term] == dtmf_display_msg_limit) strcat(tmpstr, " (check packet log for all further events)");

                           sprintf(&tmpstr[strlen(tmpstr)], ", Event = %d, Duration = %d, Volume = %d", dtmf_info.event, dtmf_info.duration, dtmf_info.volume);

                           if (pyld_ptr[1] & 0x80) {

                              strcat(tmpstr, ", End of Event");

                              DSSetJitterBufferInfo(chnum, DS_JITTER_BUFFER_INFO_UNDERRUN_RESYNC_WARNING, 1);  /* avoid underrun resync warning when media packets resume, JHB Jun2019 */
                              uDTMFState[hSession][term] = 0;
                           }

                           strcat(tmpstr, "\n");
                           sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);
                        }
                     }

                  /* process media packets */

                     if (packet_type == MEDIA_PACKET || packet_type == PROBATION_PACKET) {

                        if (packet_type == PROBATION_PACKET) {

                           if ((chnum != prev_chnum || hCodec == (intptr_t)NULL) && (hCodec = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_CODEC, 1, &termInfo)) < 0) {  /* 1 retrieves codec handle based only on chnum (i.e. can be parent or child) */

                              fprintf(stderr, "Probation packet, failed to get terminfo for chnum %d \n", chnum);
                              break;
                           }

                           media_data_len = DSGetCodecRawFrameSize(hCodec);
                           memset(media_data_buffer, 0, media_data_len);  /* zeros */
                        }
                        else {

                        /* Decode one or more frames inside an RTP payload extracted from either (i) packet pulled from jitter buffer (packet mode) or (ii) input packet (frame mode) */

                           if ((chnum != prev_chnum || hCodec == (intptr_t)NULL) && (hCodec = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_CODEC, 1, &termInfo)) < 0) {  /* find the channel's codec instance (could be parent or child). Notes: 1) if channel has changed from previous loop iteration or not yet initialized (e.g. first packet is a DTMF event, followed by a media packet) then we need to look up hCodec, 2) decoder and encoder instances are always created for a session, even if they are not used (for example, for passthru or unidirectional operation, both decoder and encoder instances exist) */

                              Log_RT(2, "ERROR: pkt/media thread %d says DSGetPacketInfo() failed to get decode codec info, hSession = %d, chnum = %d\n", thread_index, hSession, chnum);
                              break;
                           }

                        /* Decoding notes:

                           -DSGetCodecRawFramesize() could also be used to get the raw audio frame length, as long as packets contain only one frame per payload. If they contain multiple frames, the return value from DSCodecDecode() should be used
                           -both input payload length and output media buffer length are in bytes
                           -for pass-thru case, DSCodecDecode() simply copies data from in to out buffer
                        */

   #if 0 /* use this debug with test case 5280.ws.0 to document issue where first ~2.5 frames of decoded EVS stream are zeros, JHB Apr2020

            -session 0/ch 0 is AMR-WB, then the sender switches audio "in mid-stream" to an EVS session (we see it as session 1/ch 2)
            -the problem appears as a 60 msec section of zeros in stream group merged output after end of session 0. It does look like a gap; i.e. audio would "fit together" if zeros were not there
            -input pcap shows 20 msec delta between session 0 and session 1, so that's not the issue
            -if we overwrite a later frame with the the first EVS decode output frame (as in the debug code below) it does indeed decode as zeros
            -is this an EVS encoder issue ?  All handsets or just some ?
         */
   static int pyld_len_save, nOnce = 0;
   static uint8_t pyld_save[200];

   if (termInfo.codec_type == DS_VOICE_CODEC_TYPE_EVS) {

      if (nOnce == 0) {
         memcpy(pyld_save, pyld_ptr, pyld_len);
         pyld_len_save = pyld_len;
      }

      if (nOnce == 5) {  /* overwrite frame 5 with frame 0 */
         memcpy(pyld_ptr, pyld_save, pyld_len_save);
         pyld_len = pyld_len_save;
         printf("\n === EVS frame overwrite \n");
      }

      nOnce++;
   }
   #endif

                           media_data_len = DSCodecDecode(hCodec, 0, pyld_ptr, media_data_buffer, pyld_len, NULL);

                           if (media_data_len < 0) {

                              Log_RT(2, "ERROR: pkt/media thread %d says DSCodecDecode() returned error condition, hSession = %d, chnum = %d, pyld_len = %d\n", thread_index, hSession, chnum, pyld_len);
                              break;  /* error condition */
                           }
                        }

                        #if 0 /* moved up to just inside channel loop, in order to avoid compiler "could be uninitialized" warning, JHB Aug2018 */
                        pcap_index = get_pcap_index(i);  /* map session handle to output pcap file indexes */
                        wav_index = get_wav_index(i, n);  /* map session handle and channel number to output wav file indexes */
                        #endif

                        if (termInfo.codec_type != DS_VOICE_CODEC_TYPE_NONE) {  /* check for term1 (decoder) pass-thru (codec type constants are in session.h) */

                           in_media_sample_rate = DSGetCodecSampleRate(hCodec);
                           pkt_decode_cnt++;
                        }
                        else {  /* codec type = none is pass-thru case */

                           in_media_sample_rate = termInfo.sample_rate;   /* for pass-thru case, use termN sample rate value, don't increment decode counter */
                        }

                        #if SAVE_INTERIM_OUTPUT
                        fwrite(media_data_buffer, media_data_len, 1, fp_dbg);
                        #endif

                        #ifndef __LIBRARYMODE__

                     /* Write decoded audio data to wav file, if specified in the cmd line */

                        if (wav_index >= 0) {  /* write to output wav file if applicable */

                           #ifdef WAV_OUT_DEBUG_PRINT
                           static bool fOnce[MAX_TERMS+1] = {false};
                           if (!fOnce[n]) {
                              printf("  wav_index = %d, chan = %d\n", wav_index, n);
                              fOnce[n] = true;
                           }
                           #endif

                           sample_rate[wav_index] = in_media_sample_rate;  /* save sample rate for wav header update when DS_CLOSE is issued */

                           ret_val_wav = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out[wav_index], NULL, (uintptr_t)media_data_buffer, media_data_len, DS_WRITE, &MediaInfo[wav_index]);  /* DSSaveDataFile returns bytes written */

                           if (ret_val_wav <= 0) fprintf(stderr, "Error writing to .wav file, ret_val_wav = %d\n", ret_val_wav);
                           else pkt_counters[thread_index].frame_write_cnt++;

                           if (pcap_index < 0) continue;  /* encoding and/or packet output not required for .wav only output */
                        }
                        #endif
                     }

                     #ifdef DECOUPLE_STREAM_PROCESSING

 #if 0
 unsigned long long t1;
 static unsigned long long t1_save = 0;
 static int total_store = 0;
    t1 = get_time(USE_CLOCK_GETTIME);
    total_store += media_data_len/16.0;
    printf("\n$$$ clock time %2.1f, store time %d, j = %d \n", t1_save ? (t1-t1_save)/1000.0 : 0.0, total_store, j);
    if (!t1_save) t1_save = t1;
 #endif

                     #ifdef USE_CHNUM
                     chnum = chan_nums[n];  /* for time being we store audio stream data using only parent chnum.  DSGetStreamData() needs to be modified to search its chnum param for child channels, JHB Dec2019 */
                     #endif

                     if (packet_type == MEDIA_PACKET || packet_type == PROBATION_PACKET) {

   #if 0  /* debug: use sin data as output. SID payloads show with lower amplitude */
   short int* y = (short int*)media_data_buffer;
   int len = media_data_len/2;
   int amp = 2000;
   if (pyld_len < 10) amp = 1000;
   int l;
   for (l=0; l<len; l++) y[l] = amp*sin(2*M_PI*l/len);
   #endif

                        DSStoreStreamData(chnum, (uintptr_t)NULL, media_data_buffer, media_data_len);  /* store decoded media data */
                     }
                     else if (packet_type == DTMF_PACKET) DSStoreStreamData(chnum, (uintptr_t)NULL, pyld_ptr, pyld_len);  /* store DTMF event */

                     num_thread_decode_packets++;

                     prev_chnum = chnum;

                     pkt_ptr += packet_len[j];  /* advance DSGetOrderedPackets() output pointer to next packet. Placing this here is the bug fix referred to in revision history, JHB Apr2020 */

                  /* error out if we can't fit the next packet, JHB Apr2020 */

                     if (j+1 < num_pkts && pkt_ptr + packet_len[j+1] > recv_jb_buffer + sizeof(recv_jb_buffer)) {
                        Log_RT(2, "ERROR: p/m thread %d says exceeding length of buffer returned by jitter buffer, %d packets remaining in buffer, dropping packet %d \n", thread_index, num_pkts-j-1, j+1);
                        break;
                     }

                  }  /* end of packet + payload processing loop.  Note -- if DECOUPLE_STREAM_PROCESSING is not defined then the end of the loop shifts down */

               /* decode time profiling, if enabled */

                  if (!fPreemptOmit
                  #if 0  /* encode and decode profiling times are always enabled, as they are the major drivers of thread capacity, and used for some capacity management related decisions, JHB Feb2020 */
                  && packet_media_thread_info[thread_index].fProfilingEnabled
                  #endif
                  ) {

                     end_profile_time = get_time(USE_CLOCK_GETTIME);
                     decode_time += end_profile_time - start_profile_time;
                     start_profile_time = end_profile_time;
                  }

                  int num_data, packet_type;
                  unsigned int data_len[256], data_chan[256], data_info[256];
                  uint8_t stream_data[5*10240];
                  uint8_t* stream_ptr = stream_data;
                  HCODEC hCodec_link = (intptr_t)NULL;
                  bool fStreamGroupMember = false;  /* if a channel is a stream group contributor */
                  HSESSION hSessionOwner;
                  unsigned int contributor_flags = 0;

                  prev_chnum = -1;
                  chnum_parent = -1;

               /* pull audio data from stream buffers.  DSGetStreamData() accounts for variable ptime, multichannel data, etc */

                  #ifdef USE_CHAN_NUMS  /* the more efficient way is to search for specific channels, and avoid searching the entire channel space.  Also it's required for multiple pkt/media thread operation, JHB Oct2018 */
                  num_data = DSGetStreamData(chan_nums[n], 0, stream_ptr, sizeof(stream_data), data_len, data_info, data_chan);
                  #else
                  num_data = DSGetStreamData(-1, 0, stream_ptr, sizeof(stream_data), data_len, data_info, data_chan);
                  #endif

 #if 0
 unsigned long long t1;
 static unsigned long long t1_save = 0;
 static int total_get = 0;
 int l;
 if (session_info_thread[hSession].fDataAvailable) {

   t1 = get_time(USE_CLOCK_GETTIME);
  // if (t1_prev) printf(" $$$ get %2.1f: chan_nums[n] = %d, num_data = %d, data_len[0] = %d, data_chan[0] = %d \n", (t1-t1_prev)/1000.0, chan_nums[n], num_data, data_len[0], data_chan[0]);
   for (l=0; l<num_data; l++) total_get += data_len[l]/16;
   if (t1_save) printf(" $$$ %2.1f, num_data = %d, get time %d, get len = %d \n", (t1-t1_save)/1000.0, num_data, total_get, total_get*16);
   else t1_save = t1;
 }
 #endif

                  #ifdef USE_CHAN_NUMS
                  if (num_data < 0) {
                     Log_RT(1, "CRITICAL: packet/media thread %d says DSGetStreamData() error for chan = %d\n", thread_index, chan_nums[n]);
                     break;
                  }
                  #endif

               /* Media stream data processing loop.  Notes:

                  1) If DECOUPLE_STREAM_PROCESSING is defined (default setting), then stream processing is decoupled from packet + payload processing above (i.e. two separate loops).  The define can
                     be commented out and stream processing is then combined with packet + payload processing -- but that is not recommended.  Decoupling has key advantages, including (i) ability to
                     handle unequal ptimes, or "transrating", between endpoints, and (ii) serving as an interface to algorithm and signal processing for a particular media type, such as speech
                     recognition, image analytics, etc.  Note that transrating also can be considered as signal processing, using simple delay lines

                  2) The packet + payload processing loop above uses DSStoreStreamData() and the stream processing loop uses DSGetStreamData()

                  3) In this example, stream processing includes sample rate conversion (if needed) and encoding.  Algorithm and domain processing can be inserted before or after sample rate conversion,
                     as needed.  The actual the "media processing insertion point" is where DSConvertFsPacket() is called (sampling rate conversion).  Media processing functions can be inserted either
                     before or after sampling rate conversion, as needed.

                  4) In this example, the loop also includes packet formatting and output
               */

                  for (j=0; j<num_data; j++) {  /* start of payload processing loop */

                     chnum = data_chan[j];
                     data_length = data_len[j];

                     if (j > 0) stream_ptr += data_len[j-1];

                     if (data_info[j] == DS_PKT_PYLD_CONTENT_MEDIA) packet_type = MEDIA_PACKET;
                     else if (data_info[j] == DS_PKT_PYLD_CONTENT_DTMF) packet_type = DTMF_PACKET;
                     else {
                        Log_RT(1, "CRITICAL: packet/media thread %d says invalid data type from DSGetStreamData(): 0x%x\n", thread_index, data_info[j]);
                        break;
                     }

                     #else  /* #ifdef DECOUPLE_STREAM_PROCESSING */

                  /* Note -- for combined packet/payload + stream processing, chnum, pyld_ptr, and pyld_len values set earlier may be used */

                     if (packet_type == MEDIA_PACKET) {
                        stream_ptr = media_data_buffer;
                        data_length = out_media_data_len;
                     }
                     else if (packet_type == DTMF_PACKET) {
                        stream_ptr = pyld_ptr;
                        data_length = pyld_len;
                     }
                     else continue;  /* packet_type == NO_PACKET */

                     #endif  /* #ifdef DECOUPLE_STREAM_PROCESSING */

                     if (chnum != prev_chnum && (hCodec_link = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_CODEC, 2, &termInfo_link)) < 0)  /* 2 indicates outgoing (encode) codec handle and type (type stored in termInfo_link).  Either or both may be used depending on output packet type */
                     {
                        Log_RT(1, "CRITICAL: packet/media thread %d says failed to get hCodec and terminfo for term 2, chnum = %d \n", thread_index, chnum);
                        break;
                     }

                     if (packet_type == MEDIA_PACKET) {

                     /* Media processing insertion point starts here.  Stream data has been decoded and ptime equalized.  Media processing can be inserted before or after sampling rate conversion, or both.  See stream processing comments, above */

                        #ifdef ENABLE_STREAM_GROUPS
 
                     /* if this channel is specified in session definition as a stream group contributor, store media data for stream group processing.  Note there must be a stream group owner session with its group term defined */

         #if 0
         static bool fOnce[MAX_SESSIONS][MAX_TERMS] = {{ false }};
         if (!fOnce[hSession][term]) {
            hSessionOwner = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_OWNER, 0, NULL);
            DSGetSessionInfo(hSessionOwner, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_ID, 0, tmpstr);
            contributor_flags = DSGetSessionInfo(chnum_parent, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_GROUP_MODE, 1, NULL);  /* note term_id param value just needs to be non-zero to get channel's contributor flags (0 indicates group_term group_mode flags for the session that owns the channel) */
            unsigned int group_flags = DSGetSessionInfo(hSessionOwner, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_MODE, 0, NULL)
            printf("+++++++before DSStoreStreamGroupContributorData hSession = %d, chnum_parent = %d, owner session = %d, term = %d, contributor flags = %d, group flags = %d, group_id = %s\n", hSession, chnum_parent, hSessionOwner, term+1, contributor_flags, group_flags, tmpstr);
            fOnce[hSession][term] = true;
         }
         #endif

                        if (chnum != prev_chnum) {

                           chnum_parent = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_CHNUM_PARENT, 0, NULL);

                        /* before storing channel data as a stream group contributor (e.g. for merging, ASR, or other algorithm), check for two things (i) is there an existing group term owner session, and (ii) is this chnum a contributor */

                           fStreamGroupMember = (chnum_parent >= 0 && (hSessionOwner = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_OWNER, 0, NULL)) >= 0 && (contributor_flags = DSGetSessionInfo(chnum_parent, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_GROUP_MODE, 1, NULL)) > 0);  /* note term_id param value just needs to be non-zero to get channel's contributor flags (0 indicates group_term group_mode flags for the session that owns the channel) */
                        }

                        if (fStreamGroupMember) {

#if 0
   static int count = 0;
   static uint64_t lasttime = 0;
   uint64_t ctime = get_time(USE_CLOCK_GETTIME);

extern int32_t merge_save_buffer_read[NCORECHAN], merge_save_buffer_write[NCORECHAN];

   if (fCh6Active && chnum_parent == 4 && count < 50) {

     static int last_wr = 0, last_rd = 0;

     if (!lasttime) lasttime = ctime;
     if (!last_wr) last_wr =  merge_save_buffer_write[4];
     if (!last_rd) last_rd =  merge_save_buffer_read[4];

     printf(" === chnum %d storing stream group data time = %d, len = %d, delta wr = %d, delta rd = %d, wrptr = %d, rdptr = %d \n", chnum_parent, (int)((ctime-lasttime)/1000), data_length, merge_save_buffer_write[4] - last_wr, merge_save_buffer_read[4] - last_rd, merge_save_buffer_write[4], merge_save_buffer_read[4]);
     lasttime = ctime;
     last_wr =  merge_save_buffer_write[4];
     last_rd =  merge_save_buffer_read[4];

//     short int* y = (short int*)stream_ptr;
//     int len = data_length/2;
//     int amp = 2000;
//     int l;
//     for (l=0; l<len; l++) y[l] = amp*sin(2*M_PI*l/len);
     count++;
   }
#endif
                           #ifdef FIRST_TIME_TIMING
                           static bool fOnce = false;
                           if (!fOnce) { Log_RT(4, "\n === time from first pull to first group contribute %llu \n", (unsigned long long)((first_contribute_time = get_time(USE_CLOCK_GETTIME)) - first_pull_time)); fOnce = true; }
                           #endif

                           if ((ret_val = DSStoreStreamGroupContributorData(chnum_parent, stream_ptr, data_length, 0)) < 0) {

                              int thread_index_owner = DSGetSessionInfo(hSessionOwner, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_THREAD, 0, NULL);
                              int pull_queue_level = DSPullPackets(DS_PULLPACKETS_GET_QUEUE_LEVEL | DS_PULLPACKETS_TRANSCODED, NULL, NULL, hSession, NULL, 0, 0);
                              int push_queue_level = DSPushPackets(DS_PUSHPACKETS_GET_QUEUE_LEVEL, NULL, NULL, &hSession, 1);
                              bool fStopContributor = false;

                              sprintf(tmpstr, "WARNING: packet/media thread %d says merge buffer overflow", thread_index);

                              if (contributor_flags & STREAM_CONTRIBUTOR_STOP_ON_OVERFLOW_DETECTION) {
                                 sprintf(&tmpstr[strlen(tmpstr)], ", output continues w/o merging");
                                 fStopContributor = true;
                              }
                              sprintf(&tmpstr[strlen(tmpstr)], ", chnum = %d, chnum parent = %d, data_length = %d, hSession = %d, owner session = %d, group owner thread = %d, pull ql = %d, push ql = %d", chnum, chnum_parent, data_length, hSession, hSessionOwner, thread_index_owner, pull_queue_level, push_queue_level);

                              Log_RT(3, "%s \n", tmpstr);

                              if (fStopContributor) DisableStreamMerging(chnum_parent);  /* turn off stream merging for this contributor */
                           }

                           if (ret_val > 0) fFirstGroupContribution[hSessionOwner] = true;
                        }
                        #endif

                     /* Perform sampling rate conversion (if needed for encoding to output endpoint).  Data is not touched if input and output sampling rates are the same for this channel (as defined in its session config info) */

                        out_media_data_len = DSConvertFsPacket(chnum, (int16_t*)stream_ptr, data_length);  /* Notes -- 1) data length arg and output buffer length are given in bytes, 2) stream_ptr[] is unchanged and out_media_data_len = data_length if incoming and outgoing sampling rates are the same */

                     /* Encode one or more raw audio frames */

                        pyld_len = DSCodecEncode(hCodec_link, 0, stream_ptr, encoded_data_buffer, out_media_data_len, NULL);  /* Notes -- 1) Input media framesize can be either equal to a single framesize (depending on sampling
                                                                                                                                             rate in use) or an integer multiple.  In the latter case, an RTP payload will be created
                                                                                                                                             created containing multiple frames

                                                                                                                                          2) Both input media buffer length and output payload length are in bytes

                                                                                                                                          3) For pass-thru case, copies data from in to out buffer
                                                                                                                              */

                        if ((int)pyld_len < 0) break;  /* error condition */

                        if (termInfo_link.codec_type != DS_VOICE_CODEC_TYPE_NONE) {  /* check for term2 (encoder) pass-thru (codec type constants are in session.h) */

                           out_media_sample_rate = DSGetCodecSampleRate(hCodec_link);
                           pkt_xcode_cnt++;
                        }
                        else {  /* codec type = none is pass-thru case */

                           out_media_sample_rate = termInfo_link.sample_rate;
                           pkt_passthru_cnt++;
                        }
                     }
                     else {  /* these are only here to avoid compiler warnings.  They are set and used inside separated "if (packet_type == MEDIA_PACKET) .. " at the same level, but compiler doesn't seem to recognize that, JHB Aug2018 */

                        out_media_sample_rate = 0;
                        out_media_data_len = 0;
                        pyld_len = 0;
                     }

                     uFlags_format = (uintptr_t)NULL;

                     if (termInfo_link.codec_type == DS_VOICE_CODEC_TYPE_G711_ULAW || termInfo_link.codec_type == DS_VOICE_CODEC_TYPE_G711_ALAW) {  /* for G711U/A RTP output, control marker bits in output stream */

                        uFlags_format |= DS_FMT_PKT_USER_MARKERBIT;  /* tell DSFormatPackets() to read the marker bit from formatPkt and use it when formatting */

                        if (!fFirstXcodeOutputPkt[chnum]) {
                           DSSetMarkerBit(&formatPkt, uFlags_format);  /* if output stream is G711, set marker bit for first packet, per guidelines in RFC 3550/3551 */
                           fFirstXcodeOutputPkt[chnum] = true;
                        }
                        else DSClearMarkerBit(&formatPkt, uFlags_format);  /* otherwise make sure the marker bit is cleared */
                     }

                     if (out_media_data_len && !session_info_thread[hSession].fUseJitterBuffer) {

                        formatPkt.rtpHeader.Sequence++;  
                        formatPkt.rtpHeader.Timestamp += out_media_data_len/2;  /* advance timestamp by number of samples in the payload */
                        uFlags_format |= DS_FMT_PKT_USER_SEQNUM;
                        uFlags_format |= DS_FMT_PKT_USER_TIMESTAMP;
                     }

                  /* Format media and/or DTMF packets for output.  Notes:

                     1) DSFormatPacket() accepts either chnum to specify session config termN info, or a FormatPkt struct pointer and uFlags values specifying which struct values to use, or a combination of both

                     2) DTMF event packets are handled differently
                  */

                     if (packet_type == MEDIA_PACKET) {

                        packet_length = DSFormatPacket(chnum, uFlags_format, encoded_data_buffer, pyld_len, uFlags_format ? &formatPkt : NULL, pkt_out_buf);
                     }
                     else if (packet_type == DTMF_PACKET) {

                        packet_length = DSFormatPacket(chnum, uFlags_format | DS_FMT_PKT_RTP_EVENT, stream_ptr, data_length, &formatPkt, pkt_out_buf);
                     }

                     if (packet_length <= 0) {

                        Log_RT(3, "WARNING: packet/media thread %d says DSFormatPacket() returns % packet length, hSession = %d \n", thread_index, packet_length, hSession);
                        break;
                     }

   #ifndef __LIBRARYMODE__
   #if 1  /* this call to get_pcap_index() is duplicated from one above, should not be here.  Leave for now in case any bug is introduced */
     int temp = get_pcap_index(i);
     if (temp != pcap_index) printf("different pcap index temp = %d, pcap_index = %d\n", temp, pcap_index);
   #else
                     pcap_index = get_pcap_index(i);  /* map codec handle to output pcap file index */
   #endif
   #endif

                  /* write to file if this channel has an associated file handle, otherwise send packet over network */

                     if (fMediaThread) {

                        #if 0
                        extern bool fDebugTelecomSIDHandling;
                        if (fDebugTelecomSIDHandling) printf("\n === sending packet TimeStamp = %u \n", (unsigned int)(get_time(USE_CLOCK_GETTIME)/1000));
                        #endif

                        DSSendPackets((HSESSION*)&hSession, DS_SEND_PKT_QUEUE | DS_PULLPACKETS_TRANSCODED | DS_SEND_PKT_SUPPRESS_QUEUE_FULL_MSG, pkt_out_buf, &packet_length, 1);  /* send transcoded packet */
                        pkt_counters[thread_index].pkt_write_cnt++;
                        fThreadOutputActive = true;  /* any output packet for any session sets output active flag */
                     }
                     #ifndef __LIBRARYMODE__
                     else if (pcap_index >= 0 && fp_out[pcap_index] != NULL) {

                        sem_wait(&pcap_write_sem);

                        if (DSWritePcapRecord(fp_out[pcap_index], pkt_out_buf, NULL, NULL, &termInfo_link, NULL, packet_length) < 0) {
                           sem_post(&pcap_write_sem);
                           fprintf(stderr, "Main thread test, problem with DSWritePcapRecord()\n");
                           break;
                        }

                        sem_post(&pcap_write_sem);

                        pkt_counters[thread_index].pkt_write_cnt++;
                        fThreadOutputActive = true;  /* any output packet for any session sets output active flag */
                     }
                     #endif
                     else {

                        #if defined(ENABLE_TERM_MODE_FIELD) && defined(ENABLE_TERM_MODE_DONT_CARE)

                     /* don't send packet if term2 IP:port values for this packet have been defined as don't care */

                        int hSession_index = DSGetPacketInfo(hSession, DS_PKT_INFO_SESSION, pkt_out_buf, packet_length, &termInfo_link, NULL);  /* get session index and term2 info associated with the packet */

                        if (!(termInfo_link.mode & TERMINATION_MODE_IP_PORT_DONTCARE))
                        #endif

                        {
                           if (fNetIOAllowed) {

                              #ifdef USE_PKTLIB_NETIO
                              send_len = DSSendPackets((HSESSION*)&hSession, 0, pkt_out_buf, &packet_length, 1);  /* send packet to network socket */

                              if (send_len != (int)packet_length) printf("Error sending packet, send length = %d, packet length = %d\n", send_len, packet_length);

                              #elif !defined(__LIBRARYMODE__)
                              send_packet(pkt_out_buf, packet_length);  /* send packet using network socket (code is in mediaTest.c, first call causes send_sock_fd to be initialized) */
                              #endif

                              pkt_counters[thread_index].pkt_output_cnt++;
                           }
                        }
                     }

                  /* encode time profiling, if enabled */

                     if (!fPreemptOmit
                     #if 0  /* encode and decode profiling times are always enabled, as they are the major drivers of thread capacity, and used for some capacity management related decisions, JHB Feb2020 */
                     && packet_media_thread_info[thread_index].fProfilingEnabled
                     #endif
                     ) {

                        end_profile_time = get_time(USE_CLOCK_GETTIME);
                        encode_time += end_profile_time - start_profile_time;
                        start_profile_time = end_profile_time;
                     }

                     num_thread_encode_packets++;

                     prev_chnum = chnum;
                  }

                  if (lib_dbg_cfg.uEnablePktTracing & DS_PACKET_TRACE_TRANSMIT) DSLogPktTrace(hSession_flags, pkt_out_buf, packet_length, thread_index, (lib_dbg_cfg.uEnablePktTracing & ~DS_PACKET_TRACE_MASK) | DS_PACKET_TRACE_TRANSMIT);
               }

               #ifdef ENABLE_STREAM_GROUPS

               else {  /* group channel -- note that current hSession is a group owner session; see get_channels() */

               /* if no contributions yet, don't allow timing internal to DSMergeContributors() to initialize, otherwise a stream group's first contribution can have a ptime msec wobble, JHB May2020:

                  -without this the error case is the group has no contributions yet, and the interval timing in DSMergeContributor() has just transitioned to a new ptime interval, then the first contribution is unnecessarily delayed to the next ptime interval. With this no intervval transitions happen until there is at least one group contribution
                  -note that debug codes gated with FIRST_TIME_TIMING can be enabled to measure initial timing for first push (in mediaMin.c), buffer, pull, group contribution, and merge group contributors (below)
                  -use test cases 13572.0 and 2922.0, which are extremely sensitive to what happens in the first 1 sec, and verify repeatability in run-time stats, especially PLCs, resynsc, holdoffs, and max packets. Monitor timestamps on "first appears @ pkt N" log messages generated by streamlib
               */      
  
                  if (!fFirstGroupContribution[hSession]) continue;

               /* stream group profiling, if enabled */

                  if (packet_media_thread_info[thread_index].fProfilingEnabled) start_profile_time = get_time(USE_CLOCK_GETTIME);

                  #ifdef __LIBRARYMODE__  /* set some params to NULL if this is pktlib build */
                  FILE* fp_out_pcap_merge = NULL, *fp_out_wav_merge = NULL;
                  MEDIAINFO* pMediaInfo_merge = NULL;
                  #else
                  MEDIAINFO* pMediaInfo_merge = &MediaInfo_merge;  /* otherwise make those params active for mediaTest cmd line build */
                  #endif

                  int contrib_ch;

               /* Merge stream group contributors and output stream group queue packets.  Notes:

                  -DSMergeStreamGroupContributors() is in streamlib.so. See shared_include/streamlib.h for more info.  hSession must be a group owner session
                  -merging includes gap compensation algorithm and FLC + digital signal processing to compensate for irregular packet rates, stream mis-alignment, and stream overrun/underrun
                  -optional wav file output for individual contributor and merge audio can be enabled using session creation constants defined in streamlib. This is considered a debug option, but can be used as needed
                  -merge delay buffer size can be controlled using DSSetSessionInfo() with the DS_SESSION_INFO_MERGE_BUFFER_SIZE flag. Default buffer size is 2080 (in samples, which is 0.26 sec delay at 8000 kHz output rate)
               */

                  #ifdef FIRST_TIME_TIMING
                  static bool fOnce = false;
                  if (!fOnce && first_contribute_time) { Log_RT(4, "\n === time from first pull to first group process %llu \n", (unsigned long long)(get_time(USE_CLOCK_GETTIME) - first_contribute_time)); fOnce = true; }
                  #endif

                  ret_val = DSMergeStreamGroupContributors(hSession, fp_out_pcap_merge, fp_out_wav_merge,  pMediaInfo_merge, szMissingContributors, &pkt_group_cnt, &num_thread_group_contributions, cur_time, (void*)&pkt_counters, thread_index, &contrib_ch);

                  if (ret_val < 0) {

                     char group_name[MAX_GROUPID_LEN] = "";
                     int idx = DSGetStreamGroupInfo(hSession, (intptr_t)NULL, NULL, NULL, group_name);

                     sprintf(tmpstr, "WARNING: DSMergeStreamGroupContributors() returns error condition ");

                     if (ret_val == -2) {
                        HSESSION hSessionContrib = DSGetSessionInfo(contrib_ch, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_SESSION | DS_SESSION_INFO_SUPPRESS_ERROR_MSG, 0, NULL);
                        if (hSessionContrib < 0) sprintf(&tmpstr[strlen(tmpstr)], "for non-existing or previously deleted ch %d", contrib_ch);
                        else sprintf(&tmpstr[strlen(tmpstr)], "for contributor session %d ch %d", hSessionContrib, contrib_ch);
                     }
                     else sprintf(&tmpstr[strlen(tmpstr)], "for owner session %d", hSession);

                     Log_RT(3, "%s, idx = %d, group_name = %s, thread = %d, ret_val = %d \n", tmpstr, idx, group_name, thread_index, ret_val);
                  }

                  if (ret_val == 2) fThreadOutputActive = true;  /* any output packet for any session sets output active flag */

                  if (!fPreemptOmit && packet_media_thread_info[thread_index].fProfilingEnabled) {

                     end_profile_time = get_time(USE_CLOCK_GETTIME);
                     group_time += end_profile_time - start_profile_time;
                  }
               }

               #endif  /* ENABLE_STREAM_GROUPS */

            } while (++n < num_chan);  /* channel loop */

         }  /* session loop (index i) */
      }

      #ifdef ALLOW_BACKGROUND_PROCESS
      else {

      }
      #endif

      #ifdef VALGRIND_DEBUG
      usleep(VALGRIND_DELAY);
      #endif

      if (!fPreemptOmit) {

         if (decode_time > 0) {
            packet_media_thread_info[thread_index].decode_time[packet_media_thread_info[thread_index].decode_time_index] = decode_time;
            packet_media_thread_info[thread_index].decode_time_max = max(packet_media_thread_info[thread_index].decode_time_max, decode_time);
            packet_media_thread_info[thread_index].decode_time_index = (packet_media_thread_info[thread_index].decode_time_index + 1) & (THREAD_STATS_TIME_MOVING_AVG-1);
         }

         if (encode_time > 0) {
            packet_media_thread_info[thread_index].encode_time[packet_media_thread_info[thread_index].encode_time_index] = encode_time;
            packet_media_thread_info[thread_index].encode_time_max = max(packet_media_thread_info[thread_index].encode_time_max, encode_time);
            packet_media_thread_info[thread_index].encode_time_index = (packet_media_thread_info[thread_index].encode_time_index + 1) & (THREAD_STATS_TIME_MOVING_AVG-1);
         }

         if (packet_media_thread_info[thread_index].fProfilingEnabled) {

            if (chan_time > 0) {
               packet_media_thread_info[thread_index].chan_time[packet_media_thread_info[thread_index].chan_time_index] = chan_time;
               packet_media_thread_info[thread_index].chan_time_max = max(packet_media_thread_info[thread_index].chan_time_max, chan_time);
               packet_media_thread_info[thread_index].chan_time_index = (packet_media_thread_info[thread_index].chan_time_index + 1) & (THREAD_STATS_TIME_MOVING_AVG-1);
            }

            if (pull_time > 0) {
               packet_media_thread_info[thread_index].pull_time[packet_media_thread_info[thread_index].pull_time_index] = pull_time;
               packet_media_thread_info[thread_index].pull_time_max = max(packet_media_thread_info[thread_index].pull_time_max, pull_time);
               packet_media_thread_info[thread_index].pull_time_index = (packet_media_thread_info[thread_index].pull_time_index + 1) & (THREAD_STATS_TIME_MOVING_AVG-1);
            }

            if (group_time > 0) {
               packet_media_thread_info[thread_index].group_time[packet_media_thread_info[thread_index].group_time_index] = group_time;
               packet_media_thread_info[thread_index].group_time_max = max(packet_media_thread_info[thread_index].group_time_max, group_time);
               packet_media_thread_info[thread_index].group_time_index = (packet_media_thread_info[thread_index].group_time_index + 1) & (THREAD_STATS_TIME_MOVING_AVG-1);
            }
         }
      }

      if (!fPreemptOmit && fThreadInputActive) {

         int stats_index = packet_media_thread_info[thread_index].thread_stats_time_moving_avg_index;

         packet_media_thread_info[thread_index].num_buffer_packets[stats_index] = num_thread_buffer_packets;
         packet_media_thread_info[thread_index].num_decode_packets[stats_index] = num_thread_encode_packets;
         packet_media_thread_info[thread_index].num_encode_packets[stats_index] = num_thread_decode_packets;
         packet_media_thread_info[thread_index].num_group_contributions[stats_index] = num_thread_group_contributions;

         if (fAllSessionsDataAvailable && !fDebugPass) packet_media_thread_info[thread_index].thread_stats_time_moving_avg_index = (stats_index + 1) & (THREAD_STATS_TIME_MOVING_AVG-1);
      }

      if (!run && nNumCleanupLoops < 3) {  /* make sure ManageSessions() deletes any sessions marked pending for deletion, and otherwise cleans up, then allow exit, JHB Dec2019 */
         nNumCleanupLoops++;
         goto run_loop;
      }

   } while (run > 0);  /* pkt/media thread loop */


/* thread exit:  run = 0 */

   if (!fMediaThread) {

   /* Delete Sessions */

      for (i = threadid; i < (int)nSessions_gbl; i += nThreads_gbl)
      {

         if ((int)hSessions_t[i] >= 0) {

            hSession = hSessions_t[i];

            sprintf(tmpstr, "Deleting session %d\n", hSession);
            sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);

            DSDeleteSession(hSession);
         }
      }
   }

   #if defined(ENABLE_MULTITHREAD_OPERATION) && !defined(__LIBRARYMODE__)
/* wait for other threads to exit */
   for (i = 1; i < (int)nThreads_gbl; i++) pthread_join(threads[i], NULL);
   #endif

   if (fMediaThread) sprintf(tmpstr, "Num pkts recv = %d", pkt_counters[thread_index].pkt_input_cnt + pkt_counters[thread_index].pkt_read_cnt);
   else sprintf(tmpstr, "Number of packets input + read = %d", pkt_counters[thread_index].pkt_input_cnt + pkt_counters[thread_index].pkt_read_cnt);

   if (!packet_media_thread_info[thread_index].fNoJitterBuffersUsed) {
      if (fMediaThread) sprintf(&tmpstr[strlen(tmpstr)], ", buffer = %d", pkt_counters[thread_index].pkt_add_to_jb_cnt);
      else sprintf(&tmpstr[strlen(tmpstr)], ", buffered = %d", pkt_counters[thread_index].pkt_add_to_jb_cnt);
   }
   if (pkt_pulled_cnt) {
      if (fMediaThread) sprintf(&tmpstr[strlen(tmpstr)], ", jb = %d", pkt_pulled_cnt);
      else sprintf(&tmpstr[strlen(tmpstr)], ", pulled = %d", pkt_pulled_cnt);
   }
   if (pkt_xcode_cnt) {
      if (fMediaThread) sprintf(&tmpstr[strlen(tmpstr)], ", xcode = %d", pkt_xcode_cnt);
      else sprintf(&tmpstr[strlen(tmpstr)], ", transcoded = %d", pkt_xcode_cnt);
   }
   else if (pkt_decode_cnt) sprintf(&tmpstr[strlen(tmpstr)], ", decoded = %d", pkt_decode_cnt);

   if (pkt_group_cnt) sprintf(&tmpstr[strlen(tmpstr)], ", group = %d", pkt_group_cnt);

   if (pkt_xcode_cnt || pkt_passthru_cnt) {
      if (fMediaThread) sprintf(&tmpstr[strlen(tmpstr)], ", sent = %d", pkt_counters[thread_index].pkt_output_cnt + pkt_counters[thread_index].pkt_write_cnt);
      else sprintf(&tmpstr[strlen(tmpstr)], ", output + written = %d", pkt_counters[thread_index].pkt_output_cnt + pkt_counters[thread_index].pkt_write_cnt);
   }

   strcat(tmpstr, "\n");
   sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);

#ifdef DEBUGINPUTPACKETS
   for (i=0; i<pkt_chnum_ctr; i++) {
   
      printf("pkt_chnum[%d] = ", i);
      if (pkt_chnum[i] >= 0) printf("%d", pkt_chnum[i]);
      else if (pkt_chnum[i] == -2) printf("rtcp");
      else if (pkt_chnum[i] == -3) printf("notused");
      else printf("uninit");

      if (pkt_chnum[i] >= 0 || pkt_chnum[i] == -2) printf(", len = %d", pkt_chnum_len[i]);
      printf(", i = %d, hSession = %d, numStreams = %d, buffering time = %llu\n", pkt_chnum_i[i], pkt_chnum_hSession[i], pkt_chnum_numStreams[i], pkt_chnum_buftime[i]);
   }
#endif

cleanup:

   #if defined(ENABLE_MULTITHREAD_OPERATION) && !defined(__LIBRARYMODE__)
   if (nThreads_gbl > 1) {

      printf("Multithread test, number of packets input + read = %d, buffered = %d", num_pkts_read_multithread, num_pkts_buffered_multithread);
      if (pkt_xcode_cnt_multithread) printf(", transcoded = %d", pkt_xcode_cnt_multithread);
      printf(", output + written = %d\n", pkt_write_cnt_multithread + pkt_output_cnt_multithread);
   }
   #endif

/* close file descriptors */

   #ifndef __LIBRARYMODE__
   for (i = 0; i < nInFiles; i++) if (fp_in[i] != NULL) fclose(fp_in[i]);

   for (i = 0; i < nOutFiles; i++) if (fp_out[i] != NULL) {

      if (out_type[i] == WAV_AUDIO) {

         MediaInfo[i].Fs = sample_rate[i];

         ret_val_wav = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out[i], NULL, (uintptr_t)NULL, 0, DS_CLOSE, &MediaInfo[i]);  /* close wav file, update Fs and length in Wav header */
      }
      else fclose(fp_out[i]);
   }
   #endif

   #if defined(ENABLE_STREAM_GROUPS) && !defined(__LIBRARYMODE__)
   if (fp_out_wav_merge != NULL) 
      DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_wav_merge, NULL, (uintptr_t)NULL, 0, DS_CLOSE, &MediaInfo_merge);  /* close wav file, update Fs and length in Wav header */

   if (fp_out_pcap_merge != NULL) 
      fclose(fp_out_pcap_merge);
   #endif

/* close network sockets */

   if (isMasterThread && fNetIOAllowed) {

      if (recv_sock_fd != -1) close(recv_sock_fd);
      if (send_sock_fd != -1) close(send_sock_fd);
   }

   if (!fMediaThread) {

      #if (LOG_OUTPUT != LOG_SCREEN_ONLY)
      if (fp_sig_lib_log) fclose(fp_sig_lib_log);
      #endif
   }

   #if SAVE_INTERIM_OUTPUT
   fclose(fp_dbg);
   #endif

   if (isMasterThread) fPMThreadsClosing = 1;

static bool fSyncExit[MAX_PKTMEDIA_THREADS] = { false };

sync_exit:

   if (!fSyncExit[thread_index]) {
   
      if (isMasterThread) {
         #ifdef ENABLE_PKT_STATS  /* log stats for input and jitter buffer packets */
         WritePktLog(-1, pkt_counters, input_pkts, pulled_pkts, thread_index);  /* note there is a published API DSWritePacketStatsHistoryLog() that also does this */
         #endif
      }

      fSyncExit[thread_index] = true;
      goto sync_exit;
   }
   else for (i=0; i < (int)num_pktmedia_threads; i++) if (!fSyncExit[i]) {

      usleep(1000);
      goto sync_exit;
   }

/* free platform handle */

   #ifndef __LIBRARYMODE__
   if (!fMediaThread && hPlatform != -1) DSFreePlatform((intptr_t)hPlatform);
   #endif

   sprintf(tmpstr, "x86 pkt/media%s end\n", num_pktmedia_threads ? " thread" : "");
   sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);

   if (isMasterThread) fPMMasterThreadExit = 1;

   return NULL;
}


#ifndef __LIBRARYMODE__

/* helper functions that map codec handles to output file indexes.  Currently these apply to output files only, as there can be more outputs than active transcoding / decoding sessions, JHB Jul2017 */

/* get_wav_index() returns an output wav file index based on the current session and current channel for that session.  Output wav files contain a single stream; for instance the first
   output wav file specified on the command line would contain one stream from the first session and the second wav file would contain the next stream */

int get_wav_index(HSESSION hSessions[], int nSess, int nChan, int thread_index) {

int i, n = 0;
int wav_index;
int num_prev_chan = 0;

   if (!nOutFiles) return -1;

   for (i=0; i<nSess; i++) num_prev_chan += get_channels(hSessions[i], NULL, NULL, thread_index);
   
   for (wav_index = 0; wav_index < nOutFiles; wav_index++) {

      if (out_type[wav_index] == WAV_AUDIO) {
      
         if (n == (nChan + num_prev_chan)) return wav_index;  /* return first wav file found for first channel, second wav file found for 2nd channel, etc */
         else n++;
      }
   }

   return -1;
}

/* get_pcap_index() returns an output pcap file index based on the current session.  Output pcaps contain streams from the same session; for instance the first output pcap specified on
   the command line would contain two streams from the first session and the second output pcap would contain two streams from the next session */

int get_pcap_index(int nSess) {
   
int n = 0;
int pcap_index, nPcaps = 0;

   if (!nOutFiles) return -1;
   
   for (pcap_index = 0; pcap_index < nOutFiles; pcap_index++) if (out_type[pcap_index] == PCAP) nPcaps++;

   for (pcap_index = 0; pcap_index < nOutFiles; pcap_index++) {

      if (out_type[pcap_index] == PCAP) {
      
         if (n == nSess%nPcaps) return pcap_index;
         else n++;
      }
   }

   return -1;
}
#endif


static inline bool isSessionAssignedToThread(HSESSION hSession, int thread_index) {

   #if 0  /* multiple packet/media debug */
   static bool fOnce[MAX_SESSIONS] = { false };

   int ret_val = packet_media_thread_info[thread_index].threadid == DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_THREAD_ID | DS_SESSION_INFO_SUPPRESS_ERROR_MSG, 0, NULL);

   if (!fOnce[hSession] && (bool)ret_val) { printf("==========isSessionAssigned, hSession = %d, thread_index = %d, ret_val = %d\n", hSession, thread_index, ret_val); fOnce[hSession] = true; }
   
   return (bool)ret_val;
   #else

   return packet_media_thread_info[thread_index].threadid == (pthread_t)DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_THREAD_ID | DS_SESSION_INFO_SUPPRESS_ERROR_MSG, 0, NULL);

   #endif
}

/* map loop index to session handle */

static inline int get_session_handle(HSESSION hSessions[], int n, int thread_index) {

int i = 0, k = 0;

   do {

      while (hSessions[i] == -1) {  /* skip over non-active / deleted sessions */

         if (i < MAX_SESSIONS-1) i++;
         else {

            if (packet_media_thread_info[thread_index].fMediaThread) return -1;
            else goto stream_check;
         }
      }

      while (k < n && hSessions[i] >= 0) {  /* find nth valid session handle */

         k++;

         if (i < MAX_SESSIONS-1) i++;
         else {

            if (packet_media_thread_info[thread_index].fMediaThread) return -1;
            else goto stream_check;
         }
      }

   } while (hSessions[i] == -1 || k < n);

stream_check:

#ifndef __LIBRARYMODE__
   if (!packet_media_thread_info[thread_index].fMediaThread) {

      i = n;
      while (hSessions[i] == -1 && i >= 0) i--;  /* in cmd line execution, if we still don't have a valid session handle, try assuming there are more input streams than sessions */
   }
#endif

/* last check:  make sure the session is assigned to this packet/media thread */

#if 0  /* not needed because modified ManageSessions() now creates an hSessions_t[] reflection filtered by thread, and also left-shifted, JHB Oct2018 */
   if (isSessionAssignedToThread(hSessions[i], thread_index)) return hSessions[i];
   else return -1;
#else
   return hSessions[i];
#endif
}


static inline int get_channels(HSESSION hSession, int stream_indexes[], int chan_nums[], int thread_index) {

int chnum0 = -1, chnum1 = -1, num_chan = 0;
char errstr[50];

/* chan_num[] notes:

  1) chan_num[] and num_chan values are used to determine number of, and which, active (buffered) channels to use when calling DSGetOrderedPackets().  This is efficient because it gives us an option to save processing time when we already know a channel does not have data buffered

  2) For FTRT timing:
  
    -chnum_map[] values are saved in the input/buffering loop that calls DSBufferPackets().  The values are dynamic, reset each time input streams/queues are processed
  
    -when data runs out in cmd line execution, or the user app flushes a session in thread execution, fDataAvailable goes to false and in that case we use either the last known chnum_map[] values, or the channel number corresponding to the session's term definitions

  3) For real-time (ptime interval) timing:
   
    -chnum_map[] values are not used.  DSGetOrderedPackets() is called independently (and frequently) of the input/buffering loop, and packet timestamps dictate how many packets are returned per session

    -chan_nums[] and num_chan depend solely on the channel number assigned to the sessions's term definition

  4) FTRT timing is enabled when (i) buffer_interval set to zero for thread execution, or (ii) -r0 entry is given for cmd line execution
  
  5) Channels assigned to session term definitions ("termN" are unique values over all sessions
*/

   int input_buffer_interval1 = input_buffer_interval[hSession][0];

   int term1_chnum = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CHNUM, 1, NULL);  /* get channel number for term1  */

   if (term1_chnum < 0) {
      sprintf(errstr, "get_channels(), term1_chnum"); 
      ThreadAbort(thread_index, errstr);
   }

/* REQUIRE_INPUT allows out-of-real-time pkt/media thread processing, even a complete pause in input, without affecting stream group output audio.  Also it prevents the group channel from "spinning" with nothing effective
   to do.  Problem with it though is that if the group owner stream itself has no contributions then the group channel will not run and other contributors will back up in their contributor queues, JHB Oct2018 */

//#define REQUIRE_INPUT

   if (input_buffer_interval1 == 0) {

   /* note that chnum_map[] can be -1, in a case where external data supply continues, but one or more streams has no content for some time.  In that case, no value of chan_nums[] is assigned and num_chan is not incremented */
   
      if (session_info_thread[hSession].fDataAvailable || session_info_thread[hSession].chnum_map[0] != -1) chnum0 = session_info_thread[hSession].chnum_map[0];
#ifndef REQUIRE_INPUT
      else chnum0 = term1_chnum;
#else
      else if (!session_info_thread[hSession].fDataAvailable) chnum0 = term1_chnum;
#endif
   }
   else chnum0 = term1_chnum;
   
   int input_buffer_interval2 = input_buffer_interval[hSession][1];

   int term2_chnum = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CHNUM, 2, NULL);  /* get channel number for term2 */

   if (term2_chnum < 0) {
      sprintf(errstr, "get_channels(), term2_chnum"); 
      ThreadAbort(thread_index, errstr);
   }
   
   if (input_buffer_interval2 == 0) {

      if (session_info_thread[hSession].fDataAvailable || session_info_thread[hSession].chnum_map[1] != -1) chnum1 = session_info_thread[hSession].chnum_map[1];
#ifndef REQUIRE_INPUT
      else chnum1 = term2_chnum;
#else
      else if (!session_info_thread[hSession].fDataAvailable) chnum1 = term2_chnum;
#endif
   }
   else chnum1 = term2_chnum;

   if (chnum0 >= 0) {
      if (stream_indexes) stream_indexes[num_chan] = chnum0;
      if (chan_nums) chan_nums[num_chan] = chnum0;
      num_chan++;
   }

   if (chnum1 >= 0) {
      if (stream_indexes) stream_indexes[num_chan] = chnum1;
      if (chan_nums) chan_nums[num_chan] = chnum1;
      num_chan++;
   }

#ifdef ENABLE_STREAM_GROUPS
   if (DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_OWNER, 0, NULL) == hSession) {  /* is this session a group term owner ? */

#ifdef REQUIRE_INPUT
      if (num_chan || input_buffer_interval1 != 0 || input_buffer_interval2 !=0)
#endif
      {
         if (chan_nums) chan_nums[num_chan] = DS_GROUP_CHANNEL;  /* if yes, set a chan_nums[] value and increment num_chan */
         num_chan++;
      }
   }
#endif

   return num_chan;
}


/* check for an SSRC transition within a session's packet stream. This is part of pktlib support for RFC8108 */

int CheckForSSRCChange(HSESSION hSession, int chnum[], uint8_t* pkt_in_buf, unsigned int packet_len[], int num_pkts, unsigned int uFlags_info, unsigned int uFlags_session, unsigned int pkt_counters[], int thread_index) {

int j, k, rtp_SSRC, term;
unsigned int offset = 0;
bool nSSRC_change = 0;  /* return value */
int pkt_input_cnt, pkt_read_cnt, pkt_add_to_jb_cnt;
char szSSRCStatus[200];

   for (j=0; j<num_pkts; j++) {

#if 0
      chnum = DSGetPacketInfo((uFlags_session & DS_SESSION_USER_MANAGED) ? hSession : -1, uFlags_info | DS_PKT_INFO_CHNUM_PARENT | DS_PKT_INFO_SUPPRESS_ERROR_MSG, pkt_in_buf + offset, packet_len[j], NULL, NULL);
      if (chnum == -1) return -1;  /* error condition */
#endif

      term = DSGetSessionInfo(chnum[j], DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_TERM, 0, NULL);
      if (term < 1) continue;
      else term -= 1;  /* decrement term so it can be used as array index */

   /* look for an SSRC change, if so set a flag that can be acted on when pulling packets, for example to "get all deliverable" packets for the previous SSRC stream before the new stream starts */

      rtp_SSRC = DSGetPacketInfo(-1, uFlags_info | DS_PKT_INFO_RTP_SSRC, pkt_in_buf + offset, packet_len[j], NULL, NULL);

      offset += packet_len[j];

      int ssrc_change_index = max(session_info_thread[hSession].num_SSRC_changes[term]-1, 0);

      if (rtp_SSRC != session_info_thread[hSession].last_rtp_SSRC[term][ssrc_change_index]) {  /* SSRC change found */

         if (session_info_thread[hSession].last_rtp_SSRC[term][ssrc_change_index]) {

            char reportstr[100];
            bool fPrevSSRC = false;

            for (k=0; k<session_info_thread[hSession].num_SSRC_changes[term]-1; k++) {

               if (rtp_SSRC == session_info_thread[hSession].last_rtp_SSRC[term][k]) {

                  fPrevSSRC = true;
                  break;
               }
            }

            nSSRC_change = 1;  /* return "new SSRC starting" */

            if (frame_mode || !(uFlags_session & DS_SESSION_DYN_CHAN_ENABLE)) strcpy(reportstr, "reading");  /* in packet mode, check if dynamic channel creation is enabled */
            else if (!fPrevSSRC) strcpy(reportstr, "starting");
            else {
               strcpy(reportstr, "resuming");
               nSSRC_change = 2;  /* return "previous SSRC resuming" */
            }

         /* report the SSRC change:

            (i) frame mode (or packet mode without dynamic channel creation enabled):  report only that it occurred
            (ii) packet mode:  report either creating a new stream or resuming a previous one
         */

            pkt_input_cnt = pkt_counters[0];
            pkt_read_cnt = pkt_counters[1];
            pkt_add_to_jb_cnt = pkt_counters[2];

            if (packet_media_thread_info[thread_index].fMediaThread) sprintf(szSSRCStatus, "stream change #%d", session_info_thread[hSession].num_SSRC_changes[term]);  /* not sure why the terminology is any different, JHB Nov2019 */
            else sprintf(szSSRCStatus, "stream transition #%d detected", session_info_thread[hSession].num_SSRC_changes[term]);

            int new_ch = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CUR_ACTIVE_CHANNEL, 0, NULL);

            sprintf(&szSSRCStatus[strlen(szSSRCStatus)], " for hSession %d ch %d SSRC 0x%x, %s RTP stream ch %d SSRC 0x%x @ pkt %d", hSession, chnum[j], session_info_thread[hSession].last_rtp_SSRC[term][ssrc_change_index], reportstr, new_ch, rtp_SSRC, pkt_add_to_jb_cnt ? pkt_add_to_jb_cnt : pkt_read_cnt + pkt_input_cnt);
            Log_RT(4, "INFO: %s \n", szSSRCStatus);

// static bool fOnce[8] = { false };
// if (!fOnce[session_info_thread[hSession].num_SSRC_changes[term]]) { printf("########## ssrc change inside term = %d, str = %s\n", term, szSSRCStatus); fOnce[session_info_thread[hSession].num_SSRC_changes[term]] = true; }
         }

         if (session_info_thread[hSession].num_SSRC_changes[term] == MAX_SSRC_TRANSITIONS-1) session_info_thread[hSession].num_SSRC_changes[term] = 0;  /* start over if we are array limit. MAX_SSRC_TRANSITIONS is defined in shared_include/session.h */

         session_info_thread[hSession].last_rtp_SSRC[term][session_info_thread[hSession].num_SSRC_changes[term]] = rtp_SSRC;  /* store current SSRC. Note this handles all situations: (i) initialization (first SSRC), (ii) SSRC change, and (iii) wrap-around to start */
         session_info_thread[hSession].num_SSRC_changes[term] = min(session_info_thread[hSession].num_SSRC_changes[term]+1, MAX_SSRC_TRANSITIONS-1);  /* increment but don't exceed array limits */
      }
   }

   return nSSRC_change;
}


static inline int CheckForDormantSSRC(HSESSION hSession, int num_chan, int chan_nums[], int numSessions, int threadid, HSESSION hSessions_t[], uint64_t cur_time, int thread_index) {

int i, i2, j, k;
HSESSION hSession2;
bool fChanFound = false;

   for (i=0; i<MAX_TERMS; i++) {

      int ssrc_change_index = max(session_info_thread[hSession].num_SSRC_changes[i]-1, 0);
      unsigned int stream_ssrc = session_info_thread[hSession].last_rtp_SSRC[i][ssrc_change_index];  /* num_SSRC_changes[] and last_rtp_SSRC[] contain per-channel SSRC history */

  /* skip stream if it's not yet active or if dormant session detection is disabled by termination endpoint contributor flags */
  
      if (!stream_ssrc || (unsigned int)DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_MODE, i+1, NULL) & STREAM_CONTRIBUTOR_DORMANT_SSRC_DETECTION_DISABLE) continue;  /* ssrc is zero until stream buffers first packet */

      for (j=threadid; j<(packet_media_thread_info[thread_index].fMediaThread ? numSessions : (int)nSessions_gbl); j += nThreads_gbl) {

         hSession2 = get_session_handle(hSessions_t, j, thread_index);

         if (hSession2 >= 0 && hSession2 != hSession) for (i2=0; i2<MAX_TERMS; i2++) {

            int ssrc_change_index2 = max(session_info_thread[hSession2].num_SSRC_changes[i2]-1, 0);
            unsigned int stream_ssrc2 = session_info_thread[hSession2].last_rtp_SSRC[i2][ssrc_change_index2];

            if (stream_ssrc2 == stream_ssrc && session_info_thread[hSession].ssrc_state[i] == SSRC_LIVE && session_info_thread[hSession2].ssrc_state[i2] == SSRC_LIVE) {  /* two streams with same SSRC, and both are live ? */

               HSESSION hOwnerSession = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_OWNER, 0, NULL);
               HSESSION hOwnerSession2 = DSGetSessionInfo(hSession2, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_OWNER, 0, NULL);

               if (hOwnerSession >= 0 && hOwnerSession == hOwnerSession2) {  /* both sessions must be part of same stream group, otherwise we can't pass stress tests where each thread is using same pcap info */

                  int chnum = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CHNUM, i+1, NULL);
                  int chnum2 = DSGetSessionInfo(hSession2, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CHNUM, i2+1, NULL);

               /* is hSession's channel dormant ?  current rule is a dormant channel is the oldest. Hopefully they don't start bouncing back and forth ... */

                  if ((cur_time - last_buffer_time[chnum]) > (cur_time - last_buffer_time[chnum2])) {

                     if (!nDormantChanFlush[hSession][i]) {

                        Log_RT(4, "======== INFO: detected session %d channel %d now using dormant session %d channel %d SSRC value 0x%x, flushing dormant channel %d \n", hSession2, chnum2, hSession, chnum, stream_ssrc, chnum);
                        nDormantChanFlush[hSession][i] = DSGetJitterBufferInfo(chnum, DS_JITTER_BUFFER_INFO_TARGET_DELAY);  /* if there was a way to force all remaining packets out at once, that would avoid the count-down */
                     }
                     else nDormantChanFlush[hSession][i]--;

                  /* modify num_chan for hSession and force a call to DSGetOrderedPackets() to retrieve any of the dormant channel's packets still in its jitter buffer and set a state var indicating the channel is now considered dormant (which will be reset if the channel starts receiving again) */

                     if (nDormantChanFlush[hSession][i]) {

                        for (k=0; k<num_chan; k++) if (chan_nums[k] == chnum) { fChanFound = true; break; }

                        if (!fChanFound) {

                           int n = num_chan;  /* add to end of channel list */
                           if (n > 0 && chan_nums[n-1] == DS_GROUP_CHANNEL) { chan_nums[n] = DS_GROUP_CHANNEL; n--; }  /* if group channel is at end of list, move it up one */
                           chan_nums[n] = chnum;  /* insert parent channel and increment channel count */
                           num_chan++;
                        }
                     }
                     else session_info_thread[hSession].ssrc_state[i] = SSRC_DORMANT;  /* flushing completed, set this channel's SSRC state to "dormant" */
                  }
               }
            }
         }
      }
   }

   return num_chan;
}


static inline int CheckForOnHoldFlush(HSESSION hSession, int num_chan, int chan_nums[]) {

int i, j, n, num_ch, numpkts;
int ch[32];

   for (i=0; i<MAX_TERMS; i++) {

      if (nOnHoldChan[hSession][i]) {

         bool fChanFound = false;
         for (n=0; n<num_chan; n++) if (chan_nums[n] == nOnHoldChan[hSession][i]-1) { fChanFound = true; break; }

         if (!fChanFound) {

            ch[0] = nOnHoldChan[hSession][i]-1;  num_ch = 1;  /* save parent channel */

            num_ch += DSGetSessionInfo(ch[0], DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_DYNAMIC_CHANNELS, 0, (void*)&ch[num_ch]);  /* include in the search dynamic channels belonging to this term, if any.  (Note - need typecast here, otherwise it's messed up.  Not sure why) JHB Feb2019 */

            for (j=0, numpkts=0; j<num_ch; j++) numpkts = max(numpkts, DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_NUM_PKTS));  /* check if channel's jitter buffer has packets */

            if (numpkts) {

               n = num_chan;  /* add to end channel list */
               if (n > 0 && chan_nums[n-1] == DS_GROUP_CHANNEL) { chan_nums[n] = DS_GROUP_CHANNEL; n--; }  /* if group channel is at end of list, move it up one */
               chan_nums[n] = ch[0];  /* insert parent channel */
               num_chan++;

               if (!nOnHoldChanFlush[hSession][i]) {

                  nOnHoldChanFlush[hSession][i] = numpkts;  /* start the flush countdown */

               /* hSession is a group owner by definition (nOnHoldChan[][] is set in DSMergeStreamGroupContributors() in streamlib), so we can get the group mode and check for flags, JHB Nov2019 */

                  bool fDebugStats = (DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_MODE, 0, NULL) & STREAM_GROUP_DEBUG_STATS) || (lib_dbg_cfg.uDebugMode & DS_ENABLE_GROUP_MODE_STATS);
                  if (fDebugStats) Log_RT(4, "INFO: on-hold flush for hSession %d ch %d, num avail packets = %d \n", hSession, ch[0], numpkts);
               }
            }
         }
      }
   }

   for (i=0; i<MAX_TERMS; i++) {

      if (nOnHoldChanFlush[hSession][i]) {

         nOnHoldChanFlush[hSession][i]--;
         if (!nOnHoldChanFlush[hSession][i]) nOnHoldChan[hSession][i] = 0;  /* flushing complete */
      }
#if 0  /* debug */
      if (nOnHoldChanFlush[hSession][i]) Log_RT(4, "on-hold flushing for hSession %d ch %d = %d, num_chan = %d, chan_nums[0] = %d\n", hSession, i, nOnHoldChanFlush[hSession][i], num_chan, chan_nums[0]);
#endif
   }

   return num_chan;
}


static inline int CheckForPacketLossFlush(HSESSION hSession, int num_chan, int chan_nums[], uint64_t cur_time, int thread_index) {

int i, j, n, num_ch, min_packets, target_packets, num_packets, chan;
int ch[64] = { 0 };
bool fFlush;
char errstr[50];
bool fChanFound, fAnalyticsMode, fAnalyticsCompatibilityMode;

   for (i=0; i<MAX_TERMS; i++) {

      if (nMaxLossPtimes[hSession][i] < 0) continue;  /* setting max_loss_ptimes in TERMINATION_INFO struct to -1 disables packet loss mitigation (shared_include/session.h) */

      ch[0] = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CHNUM, i+1, NULL);  num_ch = 1;  /* get the parent channel */

      if (ch[0] < 0) {
         sprintf(errstr, "CheckForPacketLossFlush(), i = %d", i); 
         ThreadAbort(thread_index, errstr);
      }

      for (n=0, fChanFound=false; n<num_chan; n++) if (chan_nums[n] == ch[0]) { fChanFound = true; break; }  /* if parent channel already in the pull list, then nothing to do. When given a chnum, DSGetOrderedPackets() also searches child (dynamic) channels for the chnum */

      if (!fChanFound) {  /* in telecom mode this is never true, channels are always on the pull list. In analytics mode we can skip further processing if the parent channel is already on the pull list */

         fAnalyticsMode = input_buffer_interval[hSession][i] < ptime[hSession][i] && output_buffer_interval[hSession][i];  /* determine analytics (clockless / FTRT) mode or telecom mode */
         fAnalyticsCompatibilityMode = fAnalyticsMode && DSGetJitterBufferInfo(ch[0], DS_JITTER_BUFFER_INFO_TARGET_DELAY) <= 7;

      /* for last_pull_time[] we need only check the parent channel, as DSGetOrderedPackets() expects parent as input and automatically searches any children */

         if (fAnalyticsMode && last_pull_time[ch[0]] && cur_time - last_pull_time[ch[0]] > (uint64_t)(nMaxLossPtimes[hSession][i]*ptime[hSession][i]*1000)) {

         /* when looking at jitter buffer levels, we need to check both parent and its child (dynamic) channels, if any */

            num_ch += DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_DYNAMIC_CHANNELS, i+1, (void*)&ch[num_ch]);  /* include in the search dynamic channels belonging to this term, if any.  (Note - need typecast here, otherwise compiler messes up.  Not sure why) JHB Feb2019 */

            for (j=0, fFlush=false; j<num_ch; j++) {
 
               if ((num_packets = DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_NUM_PKTS)) == 0) continue;  /* nothing to flush, channel has stopped (or is first receiving packets) */

               target_packets = DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_TARGET_DELAY);
               min_packets = DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_MIN_DELAY);

               if (fAnalyticsCompatibilityMode) {

#define USE_PASTDUE
#ifdef USE_PASTDUE  /* pastdue flush, JHB Sep-Nov2019 */

               /* pastdue flush notes:

                  -if target_packets is set to min_packets, pastdue flush occurs
                  -SID state check was added in Jan2020, this avoids generating additional SID reuse packets which do not align with input flow timestamps (see comments in pktlib SID / media packet repair)
               */

                  unsigned int contributor_flags;
                  HCODEC hCodec;
   
                  #if 0 /* use to debug pastdue flush conditions */
                  hCodec = DSGetSessionInfo(ch[j], DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_CODEC, 1, NULL);
                  int ptime_bytes = DSGetCodecRawFrameSize(hCodec);
                  int timestamp_delta = DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_TIMESTAMP_DELTA);
                  if (num_packets <= target_packets && DSGetStreamGroupContributorPastDue(ch[0]) >= ptime_bytes) {
                     printf("$$$ pastdue jb flush candidate, j = %d, timestamp delta = %d, num_chan = %d, parent ch = %d, ch = %d, num pkts = %d, pastdue = %d, avail data = %d, sid state = %d, ptime_bytes = %d, ptime = %d, ptime other term = %d, nMaxLossPtimes[hSession][i] = %d \n", j, timestamp_delta, num_chan, ch[0], chan, num_packets, DSGetStreamGroupContributorPastDue(ch[0]), DSGetStreamGroupData(ch[0], NULL, ptime_bytes, DS_GROUPDATA_PEEK), DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_SID_STATE), ptime_bytes, ptime[hSession][i], ptime[hSession][i ^ 1], nMaxLossPtimes[hSession][i]);
                  }
                  #endif

                  bool fAllowPastdueFlush = num_packets <= target_packets && !DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_SID_STATE);  /* past due flush disallowed if the channel's jb output is currently in SID state, JHB Jan-Feb2020 */

                  if (fAllowPastdueFlush) {

                     if ((contributor_flags = DSGetSessionInfo(ch[0], DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_GROUP_MODE, 1, NULL)) > 0) {  /* stream group output active */

                        if (!(contributor_flags & STREAM_CONTRIBUTOR_DISABLE_PACKET_FLUSH) &&
                             (hCodec = DSGetSessionInfo(ch[j], DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_CODEC, 1, NULL)) > 0 &&  /* termId param non-zero retrieves codec handle based only on chnum (i.e. can be either parent or child) */
                             (DSGetStreamGroupContributorPastDue(ch[0]) >= DSGetCodecRawFrameSize(hCodec) || !fAnalyticsMode)  /* DSGetCodecRawFrameSize() returns ptime (in bytes). Note that DSGetStreamGroupContributorPastDue() can return -1 for an error condition */
                        ) {

                           target_packets = min_packets;
                        }
                     }
                     else {  /* stream group output not active */

                        target_packets = min_packets;
                     }
                  }
#endif  /* USE_PASTDUE */

                  if (num_packets > target_packets) { fFlush = true; chan = ch[j]; break; }  /* analytics compatibility mode: check if channel's jitter buffer has more packets than target delay */
               }
               else if (DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_CUMULATIVE_TIMESTAMP) < DSGetJitterBufferInfo(ch[j], DS_JITTER_BUFFER_INFO_CUMULATIVE_PULLTIME)) { fFlush = true; chan = ch[j]; break; }  /* analytics mode: check cumulative timestamp vs. cumulative pull time, JHB May2020 */
            }

            if (fFlush) {  /* for analytics mode, add/insert the parent channel to the channel list, for telecom mode set nOnHoldChanFlush[] which will cause DS_GETORD_PKT_FLUSH flag to be set in next call to DSGetOrderedPackets() */

               if (fAnalyticsMode) {

                  n = num_chan;  /* add to end of channel list */
                  if (n > 0 && chan_nums[n-1] == DS_GROUP_CHANNEL) { chan_nums[n] = DS_GROUP_CHANNEL; n--; }  /* if group channel is at end of list, move it up one */
                  chan_nums[n] = ch[0];  /* insert parent channel and increment channel count */
                  num_chan++;
               }
               else nOnHoldChanFlush[hSession][i] = 1;  /* currently not used */

               if (target_packets <= min_packets || !fAnalyticsCompatibilityMode) {

                  DSSetJitterBufferInfo(chan, DS_JITTER_BUFFER_INFO_UNDERRUN_RESYNC_WARNING, min_packets);

                  #if 0  /* use to print out pastdue flush occurrences */
                  if (session_info_thread[hSession].fDataAvailable) printf("$$$ pastdue jb flush, num_chan = %d, parent ch = %d, ch = %d, num pkts = %d, pastdue = %d, avail data = %d, ptime_bytes = %d, ptime = %d, ptime other term = %d, nMaxLossPtimes[hSession][i] = %d \n", num_chan, ch[0], chan, num_packets, DSGetStreamGroupContributorPastDue(ch[0]), DSGetStreamGroupData(ch[0], NULL, ptime_bytes, DS_GROUPDATA_PEEK), ptime_bytes, ptime[hSession][i], ptime[hSession][i ^ 1], nMaxLossPtimes[hSession][i]);
                  #endif
               }

               if (lib_dbg_cfg.uPktStatsLogging & DS_ENABLE_PACKET_LOSS_STATS) {

                  if (target_packets > min_packets) pkt_loss_flush[ch[j]]++;  /* increment packet loss flush stat */
                  else pkt_pastdue_flush[ch[j]]++; /* increment pastdue flush stat */
               }
            }
         }
      }
   }

   return num_chan;
}


int InitStream(HSESSION hSessions[], int i, int thread_index, bool* fFTRTInUse) {

HSESSION hSession;
TERMINATION_INFO term1, term2;
char tmpstr[1024];

   sprintf(tmpstr, "Initializing stream %d", i);

   hSession = get_session_handle(hSessions, i, thread_index);

   DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_TERM, 1, &term1);
   DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_TERM, 2, &term2);

   #ifndef __LIBRARYMODE__
   int index = min(i, num_pcap_inputs-1);
   if ((int)frameInterval[index] == -1) frameInterval[index] = ptime_config[hSession];  /* if no cmd line entry, use ptime from session definition */
   else {
      term1.input_buffer_interval = frameInterval[index];  /*  if -rN cmd line entry given, then override termN.buffer_interval settings */
      term2.input_buffer_interval = frameInterval[index];
   }
   #endif
//  printf("term1.input_buffer_interval = %d, term1.ptime = %d\n", term1.input_buffer_interval, term1.ptime);

   if (term1.input_buffer_interval == -1) term1.input_buffer_interval = term1.ptime;  /* if buffer interval was not given, set to ptime */
   if (term2.input_buffer_interval == -1) term2.input_buffer_interval = term2.ptime;

   if (packet_media_thread_info[thread_index].packet_mode) {

      sprintf(&tmpstr[strlen(tmpstr)], ", buffer add rate for input stream[%d] = ", i);
      if (term1.input_buffer_interval > 0) sprintf(&tmpstr[strlen(tmpstr)], "%d tps", 1000/term1.input_buffer_interval);
      else sprintf(&tmpstr[strlen(tmpstr)], "as fast as possible");

      if (term1.input_buffer_interval < term1.ptime || term2.input_buffer_interval < term2.ptime) {
         sprintf(&tmpstr[strlen(tmpstr)], ", DS_GETORD_PKT_FTRT flag is enabled");
         *fFTRTInUse = true;
      }
   }

   strcat(tmpstr, " \n");  /*end of notes for this stream */
   sig_printf(tmpstr, PRN_LEVEL_INFO, thread_index);

   #ifdef SESSIONINFOMEMCPYDEBUG
   printf("before setsessioninfo, term2.input_buffer_interval = %d\n", term2.input_buffer_interval);
   #endif
 
   DSSetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_TERM, 1, &term1);
   DSSetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_TERM, 2, &term2);

/* input_buffer_interval[] values are first set in InitSession() (which is called before InitStream), the termN values they reflect could potentially be altered here, for example in non-library mode (mediaTest cmd line), so we keep them updated, JHB May2019 */

   input_buffer_interval[hSession][0] = term1.input_buffer_interval;
   input_buffer_interval[hSession][1] = term2.input_buffer_interval;

   #ifdef SESSIONINFOMEMCPYDEBUG
   DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_TERM, 2, &term2);
   printf("after setsessioninfo, term2.input_buffer_interval = %d\n", term2.input_buffer_interval);

   int input_buffer_interval_temp = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_INPUT_BUFFER_INTERVAL, 1, NULL);
   printf("after getsessioninfo buffer, input_buffer_interval for term1 = %d\n", input_buffer_interval_temp);

   input_buffer_interval_temp = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_INPUT_BUFFER_INTERVAL, 2, NULL);
   printf("after getsessioninfo buffer, input_buffer_interval for term 2 = %d\n", input_buffer_interval_temp);
   #endif

   return 1;
}


void ResetPktStats(HSESSION hSession) {

   no_pkt_elapsed_time[hSession] = 0;
   memset(&pkt_delta_runsum[hSession], 0, sizeof(pkt_delta_runsum)/MAX_SESSIONS);
   pkt_delta_sum[hSession] = 0;
   pkt_sum_index[hSession] = 0;
#if 0
   pkt_count[hSession] = 0;
#endif

   last_packet_time[hSession] = 0;
#if 0
   for (int j=0; j<NUM_PKT_STATS; j++) avg_pkt_elapsed_time[hSession][j] = 0;
   pkt_stats_index[hSession] = 0;
#endif
}


/* initialize a session with thread-level and/or app-level items */

int InitSession(HSESSION hSession, int thread_index) {

TERMINATION_INFO term1, term2;
char tmpstr[1024];
int session_state, j;

   DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_TERM, 1, &term1);
   DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_TERM, 2, &term2);

   session_state = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_STATE, 0, NULL);

   if (!(session_state & DS_SESSION_STATE_INIT_STATUS)) {  /* avoid re-initializing an existing session */

      sprintf(tmpstr, "INFO: Initializing session %d", hSession);

      if (num_pktmedia_threads > 1) sprintf(&tmpstr[strlen(tmpstr)], " (%d)", thread_index);

      if ((int)term1.buffer_depth < 0) {  /* session config buffer depth values can be DISABLE (or -1), DEFAULT, or a depth value.  If no entry is given, the default is enabled */

         session_info_thread[hSession].fUseJitterBuffer = false;  /* this session is not using jitter buffer */
         strcat(tmpstr, ", Jitter buffer disabled");  /* display a note that DTX handling has been disabled for this session */
      }
      else session_info_thread[hSession].fUseJitterBuffer = true;

      if (!(term1.uFlags & TERM_DTX_ENABLE)) {  /* set either by application code or session config file.  In session config files DTX handling values can be ENABLE, DISABLE (or -1), DEFAULT, or a flag value.  If no entry is given, the default is enabled */
         strcat(tmpstr, ", DTX disabled");  /* display a note that jitter buffer has been disabled for this session */
      }

      if (!(term1.uFlags & TERM_SID_REPAIR_ENABLE)) {  /* set either by application code or session config file.  In session config files SID repair values can be ENABLE, DISABLE (or -1), DEFAULT, or a flag value.  If no entry is given, the default is enabled */
         strcat(tmpstr, ", SID repair disabled");  /* display a note that jitter buffer has been disabled for this session */
      }

      Log_RT(4, "%s \n", tmpstr);  /* end of notes for this session */

      #ifndef __LIBRARYMODE__
      ptime_config[hSession] = term1.ptime;  /* get ptime value from session config file (in msec) */
      if ((int)ptime_config[hSession] < 5) ptime_config[hSession] = 5;  /* min supported ptime */
      #endif

   /* check for sessions created before any media threads were started, if found then initialize their threadid */

      if (!DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_THREAD_ID, 0, NULL)) DSSetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_THREAD_ID, pthread_self(), NULL);

   /* initialize various thread level per-session items */

      session_info_thread[hSession].init_time = get_time(USE_CLOCK_GETTIME);
      session_info_thread[hSession].look_ahead_time = 0;

      session_info_thread[hSession].fDataAvailable = true;

      memset(&session_info_thread[hSession].chnum_map, 0xff, sizeof(session_info_thread[0].chnum_map));

      memset(&session_info_thread[hSession].chnum_map_history, 0xff, sizeof(session_info_thread[0].chnum_map_history));

      session_info_thread[hSession].num_streams_active = 0;

      memset(&session_info_thread[hSession].last_rtp_SSRC, 0, sizeof(session_info_thread[0].last_rtp_SSRC));
      memset(&session_info_thread[hSession].num_SSRC_changes, 0, sizeof(session_info_thread[0].num_SSRC_changes));
      memset(&session_info_thread[hSession].fSSRC_change_active, 0, sizeof(session_info_thread[0].fSSRC_change_active));
      memset(&session_info_thread[hSession].ssrc_state, 0, sizeof(session_info_thread[0].ssrc_state));

      fDisplayActiveChannels[hSession] = false;

   /* init stream group items, if this session is a stream group owner */

      int idx;

      if (DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_OWNER | DS_SESSION_INFO_USE_PKTLIB_SEM, 0, NULL) == hSession) {  /* does this session own a group term ? */

         if ((idx = DSInitStreamGroup(hSession)) < 0) Log_RT(4, "WARNING: InitSession() says stream group owner session %d failed to return valid stream group idx\n", hSession);  /* yes, initialize its stream group */

         pkt_count_group[idx] = 0;

         fFirstGroupContribution[hSession] = false;
      }
#if 0
      else idx = DSGetStreamGroupInfo(hSession, DS_SESSION_INFO_USE_PKTLIB_SEM | DS_GETGROUPINFO_CHECK_ALLTERMS, NULL, NULL, NULL);
#endif

   /* following items could be indexed by stream group, and it is somewhat wasteful of mem to index by session.  However (i) it's safe to do so, because there is a 1:1 relationship between group owner sessions and stream group idx, and (ii) these items either are, or likely will be, needed in other places and session_info_thread[] is useful for that, JHB Sep2018 */

      session_info_thread[hSession].merge_audio_chunk_size = 2080; /* in samples.  Default buffer size is 0.26 sec, maximum can be up to MAX_MERGE_BUFFER_SIZE sec (defined in pktlib.h, figures assume 16-bit samples at 8 kHz output) */
//      session_info_thread[hSession].merge_audio_chunk_size = 4160; /* in samples.  Default buffer size is 0.52 sec, maximum can be up to MAX_MERGE_BUFFER_SIZE sec (defined in pktlib.h, figures assume 16-bit samples at 8 kHz output) */

      session_info_thread[hSession].fAllContributorsPresent = false;

      for (j=0; j<MAX_GROUP_CONTRIBUTORS; j++) {  /* this should be in DSInitMergeGroup() in streamlib, but session_info_thread[] is not passed to DSInitMergeGroup() */

         session_info_thread[hSession].uMissingContributions[j] = 0;
         session_info_thread[hSession].nPrevMissingContributor[j] = 0;
      }

      for (j=0; j<MAX_TERMS; j++) {

         input_buffer_interval[hSession][j] = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_INPUT_BUFFER_INTERVAL | DS_SESSION_INFO_SUPPRESS_ERROR_MSG, j+1, NULL);

         if (input_buffer_interval[hSession][j] < 0) {

            if (packet_media_thread_info[thread_index].fMediaThread) Log_RT(4, "WARNING: InitSession() says input_buffer_interval is not initialized for session %d\n", hSession);
            input_buffer_interval[hSession][j] = 0;
         }

         ptime[hSession][j] = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_PTIME, j+1, NULL);
         if (ptime[hSession][j] < 0) { Log_RT(4, "WARNING: InitSession() says ptime is not initialized for session %d\n", hSession); ptime[hSession][j] = 20; }

         output_buffer_interval[hSession][j] = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_OUTPUT_BUFFER_INTERVAL | DS_SESSION_INFO_SUPPRESS_ERROR_MSG, j+1, NULL);

         if (output_buffer_interval[hSession][j] < 0) {

            if (packet_media_thread_info[thread_index].fMediaThread) Log_RT(4, "WARNING: InitSession() says output_buffer_interval is not initialized for session %d\n", hSession);
            output_buffer_interval[hSession][j] = 0;
         }

         packet_media_thread_info[thread_index].fFTRTPtime = output_buffer_interval[hSession][j] > 0;  /* any session with FTRT + ptime set makes this true. If it stays false thread pre-emption alarms are disabled due to expected super fast push rate, JHB Jan2020 */ 

         nDormantChanFlush[hSession][j] = 0;
         nOnHoldChanFlush[hSession][j] = 0;  /* reset session's on-hold flush state */
         nOnHoldChan[hSession][j] = 0;

         nMaxLossPtimes[hSession][j] = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_MAX_LOSS_PTIMES, j+1, NULL);
         if (nMaxLossPtimes[hSession][j] < 0) { Log_RT(4, "WARNING: InitSession() says max_loss_ptimes is not initialized for session %d\n", hSession); nMaxLossPtimes[hSession][j] = 0; }

         uDisplayDTMFEventMsg[hSession][j] = 0;
         uDTMFState[hSession][j] = 0;
      }

   /* session timing stats items */

      ResetPktStats(hSession);

#if 0
      progress_var[hSession] = 0;
#endif

   /* initialize DSPushPackets() and DSRecvPackets() for this session */

      DSPushPackets(DS_PUSHPACKETS_INIT, NULL, 0, &hSession, 1);

      DSRecvPackets(hSession, DS_RECV_PKT_INIT, NULL, 0, NULL, 0);

   /* mark session state as initialized */
 
      DSSetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_STATE, DS_SESSION_STATE_INIT_STATUS, NULL);
   }

   return 1;
}


/* CleanSession() handles cleanup for items that may apply to both parent and child channels. Child channels are created dynamically so we don't know what index they will end up using when sessions are created.  We know only when sessions are deleted, JHB Nov2019 */

int CleanSession(HSESSION hSession, int thread_index) {

int ch[64];

/* first two indexes are parent channels */

   ch[0] = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CHNUM, 1, NULL);
   ch[1] = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CHNUM, 2, NULL);

   if (ch[0] < 0 || ch[1] < 0) {

      char errstr[50];
      sprintf(errstr, "CleanSession(), ch[0] = %d, ch[1] = %d", ch[0], ch[1]); 
      ThreadAbort(thread_index, errstr);
      return -1;
   }
   else {

      int j, num_ch = 2;  /* subsequent indexes start at 2 for child channels, if any */

      pkt_count[hSession] = 0;

   /* build channel list, including child channels belonging to this termN, if any. JHB Nov2019 */

      num_ch += DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_DYNAMIC_CHANNELS, 1, (void*)&ch[num_ch]);  /* note - need typecast here, otherwise compiler messes up.  Not sure why) JHB Feb2019 */
      num_ch += DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_DYNAMIC_CHANNELS, 2, (void*)&ch[num_ch]);

      for (j=0; j<num_ch; j++) {

         #ifdef PACKET_TIME_STATS

         packet_in_time[ch[j]] = 0;
         last_packet_in_time[ch[j]] = 0;
         packet_in_time_pull[ch[j]] = 0;
         last_packet_in_time_pull[ch[j]] = 0;

         packet_max_delta[ch[j]] = 0;
         max_delta_packet[ch[j]] = 0;

         packet_media_delta[ch[j]] = 0;
         packet_sid_delta[ch[j]] = 0;
         packet_max_media_delta[ch[j]] = 0;
         max_media_delta_packet[ch[j]] = 0;
         packet_max_sid_delta[ch[j]] = 0;
         max_sid_delta_packet[ch[j]] = 0;

         media_stats_pkt_count[ch[j]] = 0;
         sid_stats_pkt_count[ch[j]] = 0;

         prev_pyld_content[ch[j]] = 0;

         packet_rtp_time[ch[j]] = 0;
         last_rtp_timestamp[ch[j]] = 0;
         packet_rtp_time_pull[ch[j]] = 0;
         last_rtp_timestamp_pull[ch[j]] = 0;

         num_jb_zero_pulls[ch[j]] = 0;
         packet_in_bursts[ch[j]] = 0;
         packet_out_bursts[ch[j]] = 0;

         #endif

         pkt_loss_flush[ch[j]] = 0;
         pkt_pastdue_flush[ch[j]] = 0;
         pkt_level_flush[ch[j]] = 0;

         last_buffer_time[ch[j]] = 0;
         last_pull_time[ch[j]] = 0;

         fFirstXcodeOutputPkt[ch[j]] = false;

         session_run_time_stats[ch[j]] = 0;

         nMaxStreamDataAvailable[ch[j]] = 0;
      }
   }

   return 1;
}


/* ManageSessions() enumerates through all session handles and manages sessions assigned to this thread:

  -saves an accurate copy of currently active sessions in hSessions[] (hSessions[] is per thread, located on each thread's stack as hSessions_t[])
  -if a session is new (recently created), calls InitSession() to initialize with thread level items
  -if a session is marked as delete pending, calls CleanSession() to reset thread level items and DSDeleteSession() to delete
  -see detailed comments below
*/

#ifdef USE_CHANNEL_PKT_STATS
int ManageSessions(HSESSION* hSessions, PKT_COUNTERS pkt_counters[], PKT_STATS_HISTOR input_pkts[], PKT_STATS_HISTORY pulled_pkts[], bool* fAllSessionsDataAvailable, int thread_index) {
#else
int ManageSessions(HSESSION* hSessions, PKT_COUNTERS pkt_counters[], PKT_STATS input_pkts[], PKT_STATS pulled_pkts[], bool* fAllSessionsDataAvailable, int thread_index) {
#endif

int i, numSessions = 0, numSessionsFound;
HSESSION hSession;
bool fNoJitterBuffersUsed = true;
char tmpstr[1024];
int nRetry = 0, numInit = 0, numDeleted = 0;

get_num_sessions:

   if (num_pktmedia_threads <= 1) numSessions = DSGetSessionInfo(0, DS_SESSION_INFO_NUM_SESSIONS, 0, NULL);  /* note -- DS_SESSION_INFO_NUM_SESSIONS does not take DS_SESSION_INFO_HANDLE or DS_SESSION_INFO_CHNUM */
   else numSessions = DSGetSessionInfo(0, DS_SESSION_INFO_NUM_SESSIONS, packet_media_thread_info[thread_index].threadid, NULL);  /* if more than one thread active, we specify the thread and get its number of sessions */

   if (numSessions < 0) return 0;

   #if 1
   static bool fOnce[MAX_PKTMEDIA_THREADS] = { false };
   if (lib_dbg_cfg.uLogLevel > 8 && !fOnce[thread_index] && numSessions > 0) {
      Log_RT(8, "DEBUG2: ManageSessions, numSessions = %d, thread = %d, thread id = 0x%llx\n", numSessions, thread_index, (unsigned long long)pthread_self());
      fOnce[thread_index] = true;
   }
   #endif

   numSessionsFound = 0;
   bool fEarlyExit = false;
   #define MAX_SESSION_TRANSACTIONS_PER_PASS 3

   memset(hSessions, -1, sizeof(HSESSION)*MAX_SESSIONS);  /* in case numSessions is zero, and the loop doesn't run.  JHB Jan2019 */

   if (numSessions) for (i=0; i<MAX_SESSIONS; i++) {  /* search for active session handles.  Note that pktlib does not re-use deleted session indexes until it wraps around in sessions[] management struct */ 

      #if 0  /* debug */
      extern SESSION_CONTROL sessions[MAX_SESSIONS];
      if (nRetry && i == 0) printf("\n ==== sessions[i].threadid = %llu, sessions[i].thread_index = %d, sessions[i].in_use = %d \n", sessions[i].threadid, sessions[i].thread_index, sessions[i].in_use);
      #endif

      hSession = DSGetSessionInfo(i, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_SESSION | DS_SESSION_INFO_SUPPRESS_ERROR_MSG, 0, NULL);

      if (hSession >= 0 && isSessionAssignedToThread(hSession, thread_index)) {  /* limit hSessions_t[] "reflection" by thread and current value of numSessions (an app may be concurrently creating more sessions, if so we'll see them on the next pass) */

         hSessions[numSessionsFound] = hSession;

         int state = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_STATE, 0, NULL);
         int state_clear_flags = (int)0xffffffffL;

      /* session initialization:

         -this is p/m thread level initialization, which is separate from pktlib and voplib session initialization (session instance data, jitter buffer, media codecs, etc)
         -limit of MAX_SESSION_TRANSACTIONS_PER_PASS per ManageSessions() call
      */

         if (state == DS_SESSION_STATE_NEW) {

            if (numInit < MAX_SESSION_TRANSACTIONS_PER_PASS) {  /* impose a limit on how many sessions we initialize in one thread loop.  Initialization takes time, including InitSession() and more possible init inside streamlib. This prevents thread preemption alarms when running high cap / stress tests */

               InitSession(hSession, thread_index);
               numInit++;
            }
            else { fEarlyExit = true; packet_media_thread_info[thread_index].manage_sessions_create_early_exit++; break; }
         }

      /* session flush:

         -apps should flush a session when they are providing no further packet input for that session
         -flush empties all jitter buffers. The "residual packets" run-time stat should be zero 
         -stream group output FLC is disabled
         -here in ManageSessions() flush is handled prior to delete in case both are pending simultaneously
      */
 
         if (state & DS_SESSION_STATE_FLUSH_PACKETS) {

            session_info_thread[hSession].fDataAvailable = false;

            state_clear_flags &= ~DS_SESSION_STATE_FLUSH_PACKETS;

            *fAllSessionsDataAvailable = false;

            if (lib_dbg_cfg.uDebugMode & DS_ENABLE_MANAGE_SESSION_STATS) Log_RT(4, "INFO: ManageSessions() says flushing session %d \n", hSession);
         }

      /* session delete:

         -sessions are deleted here if they are pending deletion, which happens when an application calls DSDeleteSession(). Code and documentation sometimes refers to sessions as having been "marked for deletion"
         -here we also call DSDeleteSession(), which verifies the calling thread is a packet/media thread. If not, or a session had already been deleted, DSDeleteSession() writes "double delete" or "invalid handle" warning messages to the event log 
         -limit of MAX_SESSION_TRANSACTIONS_PER_PASS per ManageSessions() call
         -deletes are not performed if the thread has already been instructed to exit
      */

         int delete_status = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_DELETE_STATUS, 0, NULL);

         if (delete_status & DS_SESSION_DELETE_PENDING) {

            if (!run || (numDeleted < MAX_SESSION_TRANSACTIONS_PER_PASS)) {

            /* DSPostProcessStreamGroup() handles anything that is not real-time and might cause a gap in stream group output, such as N-channel wav file handling. We call it after session flush but before session delete, JHB Jan2020 */

               DSPostProcessStreamGroup(hSession, thread_index);

               if ((lib_dbg_cfg.uPktStatsLogging & DS_ENABLE_PACKET_TIME_STATS) || (lib_dbg_cfg.uPktStatsLogging & DS_ENABLE_PACKET_LOSS_STATS)) DSLogPacketTimeLossStats(hSession, DS_LOG_PKT_STATS_ORGANIZE_BY_STREAM_GROUP | DS_LOG_PKT_STATS_SUPPRESS_ERROR_MSG);

               CleanSession(hSession, thread_index);  /* clear p/m thread level items */

               DSDeleteSession(hSession);  /* pktlib */

               numDeleted++;

               goto get_num_sessions;  /* restart the search, don't make any assumptions on what pktlib is doing */
            }
            else { fEarlyExit = true; packet_media_thread_info[thread_index].manage_sessions_delete_early_exit++; break; }
         }

         if (state & DS_SESSION_STATE_WRITE_PKT_LOG) {  /* write packet log */

            if (WritePktLog(hSession, pkt_counters, input_pkts, pulled_pkts, thread_index)) {

               state_clear_flags &= ~DS_SESSION_STATE_WRITE_PKT_LOG;
            }
         }

         if (state & DS_SESSION_STATE_RESET_PKT_LOG) {  /* reset packet stats */

            memset(&pkt_counters[thread_index], 0, sizeof(PKT_COUNTERS));
            state_clear_flags &= ~DS_SESSION_STATE_RESET_PKT_LOG;
         }

         if (state_clear_flags != (int)0xffffffffL) DSSetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_STATE, state_clear_flags, NULL);  /* state_clear_flags needs to be an int so it will be extended to int64_t to match DSSetSessionInfo() prototype */

      /* see if user app has modified merge buffer size (and other algorithm items that might be added later on) */

         uint32_t merge_buffer_size = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_MERGE_BUFFER_SIZE, 0, NULL);
         if (merge_buffer_size > 0) session_info_thread[hSession].merge_audio_chunk_size = merge_buffer_size;

      /* update across-session flags */

         if (session_info_thread[hSession].fUseJitterBuffer) fNoJitterBuffersUsed = false;  /* any session using a jitter buffer makes this false */

         numSessionsFound++;

         if (numSessionsFound >= numSessions) break;  /* no need to continue looking at session handles if we've reached the session count, JHB Feb2019 */
      }
   }

   if (!fEarlyExit && numSessionsFound != numSessions) {

      if (abs(numSessions - numSessionsFound) > 0) {  /* pktlib sem locks are not used in DSGetSessionInfo() so it's possible we're off by one.  Allow one retry.  We do not want to be using semaphores when searching 512+ sessions */

         packet_media_thread_info[thread_index].manage_sessions_count_mismatch++;

         if (nRetry < nManageSessionRetriesAllowed) {  /* nManageSessionRetriesAllowed is a global with default value of 1, so it can be changed if necessary, JHB Jan2020 */
            nRetry++;
            goto get_num_sessions;
         }
      }

      sprintf(tmpstr, "INFO: ManageSessions() number of found sessions %d temporarily not matching pktlib count %d, thread index = %d, numInit = %d, numDeleted = %d, num pkt media threads = %d", numSessionsFound, numSessions, thread_index, numInit, numDeleted, num_pktmedia_threads);

      if (num_pktmedia_threads > 1) sprintf(&tmpstr[strlen(tmpstr)], " (%d)", thread_index);

      Log_RT(6, "%s \n", tmpstr);
   }

   packet_media_thread_info[thread_index].fNoJitterBuffersUsed = fNoJitterBuffersUsed;

/* update session management history */

   int history_index = packet_media_thread_info[thread_index].manage_sessions_history_index;
   packet_media_thread_info[thread_index].manage_sessions_creation_history[history_index] = numInit;
   packet_media_thread_info[thread_index].manage_sessions_deletion_history[history_index] = numDeleted;
   packet_media_thread_info[thread_index].manage_sessions_history_index = (history_index+1) & (MS_HISTORY_LEN-1);

#if 0
   return numSessions;
#else  /* changed to be compatible with "early exit" handling (see fEarlyExit above), JHB Jan2020 */
   return numSessionsFound;
#endif
}


/* return uFlags for a session */

static inline unsigned int uFlags_session(HSESSION hSession) {

   return (unsigned int)DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_UFLAGS, 0, NULL);
}


/* write out packet stats logs for currently active sessions.  Note the hSession param is for the time being only used to determine whether "collate streams" is set in log analysis */

#ifdef USE_CHANNEL_PKT_STATS
int WritePktLog(HSESSION hSession, PKT_COUNTERS pkt_counters[], PKT_STATS_HISTORY input_pkts[], PKT_STATS_HISTORY pulled_pkts[], int thread_index) {
#else
int WritePktLog(HSESSION hSession, PKT_COUNTERS pkt_counters[], PKT_STATS input_pkts[], PKT_STATS pulled_pkts[], int thread_index) {
#endif

char szLogFile[1024], tmpstr[1024], reportstr[50];
unsigned int uFlags_log;
int numStreams;
char* p = NULL;
int i;

   if (pkt_counters == NULL || input_pkts == NULL || pulled_pkts == NULL) return -1;

   if (DSIsPktStatsHistoryLoggingEnabled(thread_index) && (pkt_counters[thread_index].num_input_pkts || pkt_counters[thread_index].num_pulled_pkts)) {

      if (strlen((const char*)pktStatsLogFile)) strcpy(szLogFile, (const char*)pktStatsLogFile);  /* if no log file given on the cmd line, we use one of the output names and replace extension with .txt */
      else {

#ifndef __LIBRARYMODE__

      /* assign log filename to inherit the first output pcap filename found (but with .txt extension).  If no output pcaps found, then use the first .wav filename found */

         int j;
         char* p2;
 
         for (j=0; j<nOutFiles; j++) {

            strcpy(tmpstr, MediaParams[j].Media.outputFilename);
            strupr(tmpstr);

            if (strstr(tmpstr, ".PCAP")) {

               strcpy(szLogFile, MediaParams[j].Media.outputFilename);
               for (p = strstr(szLogFile, "."); ;) {  /* look for last '.' in that cmd line path */
                  if (p) {
                     p2 = strstr(p+1, ".");
                     if (p2) p = p2;
                     else break;
                  }
                  else break;
               }
               if (p) *p = 0;
               strcat(szLogFile, ".txt");
               break;
            }
         }

         if (!p) for (j=0; j<nOutFiles; j++) {

            strcpy(tmpstr, MediaParams[j].Media.outputFilename);
            strupr(tmpstr);

            if (strstr(tmpstr, ".WAV")) {

               strcpy(szLogFile, MediaParams[j].Media.outputFilename);
               for (p = strstr(szLogFile, "."); ;) {
                  if (p) {
                     p2 = strstr(p+1, ".");
                     if (p2) p = p2;
                     else break;
                  }
                  else break;
               }
               if (p) *p = 0;
               strcat(szLogFile, ".txt");
               break;
            }
         }
#endif

         if (!p) strcpy(szLogFile, "pcap_jb_log.txt");
      }
 
      uFlags_log = packet_media_thread_info[thread_index].packet_mode ? DS_PKTSTATS_LOG_PACKETMODE : DS_PKTSTATS_LOG_FRAMEMODE;  /* set uFlags value(s), listed in diaglib.h */

      if (hSession == -1 || (hSession >= 0 && DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_OWNER, 0, NULL) >= hSession)) {
         numStreams = packet_media_thread_info[thread_index].num_streams_active;
         strcpy(reportstr, "all sessions");
      }
      else {
         numStreams = session_info_thread[hSession].num_streams_active;
         sprintf(reportstr, "session %d", hSession);
      }

      if (numStreams > 1) uFlags_log |= DS_PKTSTATS_LOG_COLLATE_STREAMS;  /* Collate streams in the log printout for multiple input streams.  This is not recommended for testing SSRC changes within one input stream; in that case, collating
                                                                             streams could mask interleaving or other issues occurring around the SSRC transition points.  When using log printout to test/verify SSRC changes (dynamic channel
                                                                             creation), suggest using one input stream and not enabling the DS_PKTSTATS_LOG_COLLATE_STREAMS flag */

   /* set organize-by-group flag if any streams were stream group members */

      for (i=0; i<(int)pkt_counters[thread_index].num_input_pkts; i++) if (input_pkts[i].idx >= 0) { uFlags_log |= DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP; break; }

      if (!(uFlags_log & DS_PKTSTATS_ORGANIZE_BY_STREAMGROUP) && !(uFlags_log & DS_PKTSTATS_ORGANIZE_BY_CHNUM)) uFlags_log |= DS_PKTSTATS_ORGANIZE_BY_SSRC;  /* if no stream groups found, set organize-by-SSRC flag by default */

   /* enable event log summary for each stream */

      uFlags_log |= DS_PKTSTATS_LOG_EVENT_LOG_SUMMARY;

      #if 0  /* turn on these flags for debug purposes */
      uFlags_log |= DS_PKTSTATS_LOG_LIST_ALL_INPUT_PKTS;
      uFlags_log |= DS_PKTSTATS_LOG_LIST_ALL_PULLED_PKTS;
      #endif

   /* Logging notes:

      1) See main log file notes section above, where ENABLE_PKT_STATS is defined

      2) DSPktStatsAddEntries() records one or more packet entries in PKT_STATS structs, and DSPktStatsWriteLogFile() (below) writes packet stats to a log file.  For applicable flags, see diaglib.h

      3) The DS_PKTSTATS_LOG_COLLATE_STREAMS flag is active for multiple input streams.  The flag is not recommended for streams with SSRC transitions, as the stream collation algorithm in Diaglib currently
         cannot differentiate dynamically created streams (see "Collate streams" notes above)
   */

      if (isMasterThread) {

         sprintf(tmpstr, "INFO: master p/m thread says writing input and jitter buffer output packet stats to packet log file %s, streams found for %s = %d", szLogFile, reportstr, numStreams);
         if (uFlags_log & DS_PKTSTATS_LOG_COLLATE_STREAMS) strcat(tmpstr, " (collate streams enabled)");
         sprintf(&tmpstr[strlen(tmpstr)], ", total input pkts = %d, total jb pkts = %d", pkt_counters[thread_index].num_input_pkts, pkt_counters[thread_index].num_pulled_pkts);
         Log_RT(4, "%s... \n", tmpstr);

         DSPktStatsWriteLogFile(szLogFile, uFlags_log, input_pkts, pulled_pkts, &pkt_counters[thread_index]);
      }

      return 1;
   }

   return 0;
}


/* published API that does more or less what WritePktLog() does, with some extras, such as packet stats history reset, JHB Dec2019 */

int DSWritePacketStatsHistoryLog(HSESSION hSession, unsigned int uFlags, const char* szLogFilename) {

int thread_index;
char* szLocalLogFilename = NULL;

   if (uFlags & DS_WRITE_PKT_STATS_HISTORY_LOG_THREAD_INDEX) thread_index = (int)hSession;
   else thread_index = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_THREAD | DS_SESSION_INFO_SUPPRESS_ERROR_MSG, 0, NULL);

   if (thread_index < 0 || thread_index >= nPktMediaThreads) {
   
      Log_RT(3, "WARNING: DSWritePacketLogStats() says invalid %s %d \n", uFlags & DS_WRITE_PKT_STATS_HISTORY_LOG_THREAD_INDEX ? "thread index" : "hSession", thread_index);
      return -1;  /* thread index limited to currently active packet/media threads */
   }

   if (!szLogFilename) {
   
      if (uFlags & DS_WRITE_PKT_STATS_HISTORY_LOG_RESET_STATS) {  /* combination of NULL log filename and reset stats flag just does a reset */

         memset(&pkt_counters[thread_index], 0, sizeof(PKT_COUNTERS));
         return 1;
      }
   }
   else if (strlen(szLogFilename)) szLocalLogFilename = (char*)szLogFilename;

   if (!szLocalLogFilename) {  /* if szLogFilename is NULL or empty then check if global var pktStatsLogFile can be used */
   
      if (strlen((char*)pktStatsLogFile)) {

         szLocalLogFilename = (char*)pktStatsLogFile;
         Log_RT(4, "INFO: DSWritePacketLogStats() szLogFilename param NULL or empty string, using pktStatsLogFile var = %s \n", pktStatsLogFile);
      }
      else {
         Log_RT(3, "WARNING: DSWritePacketLogStats() szLogFilename param NULL or empty string \n");
         return -1;
      }
   }

   if (!(uFlags & DS_PKTSTATS_LOG_PACKETMODE) && !(uFlags & DS_PKTSTATS_LOG_FRAMEMODE)) uFlags |= DS_PKTSTATS_LOG_PACKETMODE;  /* default if neither is set */

/* call DSPktStatsWriteLogFile() in diaglib */

   int ret_val = DSPktStatsWriteLogFile(szLocalLogFilename, uFlags, input_pkts, pulled_pkts, &pkt_counters[thread_index]);  /* input_pkts, pulled_pkts, and pkt_counters are static vars, see top */

/* reset stats after logging is complete, if requested */

   if (uFlags & DS_WRITE_PKT_STATS_HISTORY_LOG_RESET_STATS) memset(&pkt_counters[thread_index], 0, sizeof(PKT_COUNTERS));
   
   return ret_val;
}

#if defined(ENABLE_MULTITHREAD_OPERATION) && !defined(__LIBRARYMODE__)  /* deprecated, no longer used.  See comments near SECONDARY_THREADS_DEPRECATED above */

/* multithread / concurrent channel example source */

void* secondaryThreads(void* arg) {  /* secondary threads run here */

uint8_t           pkt_buffer[MAX_RTP_PACKET_LEN] = { 0 }, recv_jb_buffer[MAX_RTP_PACKET_LEN*MT_MAX_CHAN*JB_DEPTH] = { 0 }, media_data_buffer[4*MAX_RTP_PACKET_LEN] = { 0 }, encoded_data_buffer[2*MAX_RTP_PACKET_LEN] = { 0 };
unsigned int      packet_len[64];

TERMINATION_INFO  termInfo, termInfo_link;
uint64_t          cur_time;
uint64_t          last_time[MAX_INPUT_STREAMS] = { 0 };
unsigned int      packet_length, pyld_len;
int               chnum, pkts_buffered, ret_val, i, j;
uint8_t           *pkt_ptr, *pyld_ptr;
HCODEC            hCodec, hCodec_link;
uint32_t          threadid, nSessions = 0, uFlags_session, hSession;
int               media_data_len, out_media_data_len;

HSESSION          hSessions_t[MAX_SESSIONS] = { 0 };


/* Create per-thread sessions.  Notes:

   1) Each thread may create more than one session, for example if session config file contains more sessions than the number of threads specified with -tN cmd line option

   2) IP addr and/or port numbers must be different between sessions, otherwise DSCreateSession will return with a "duplicated hash" error message

   3) If more threads are specified on the cmd line than sessions then a warning message will be displayed and remaining threads will not be not created.  This avoids
      duplicated session info, as noted in 2)
*/

   threadid = *(uint32_t*)arg;
   free(arg);

   /* create sessions */
   for (i = threadid; i < (int)nSessions_gbl; i += nThreads_gbl)
   {
      if(demo_build && nSessions > 0) {

         fprintf(stderr, "Demo build is limited to 1 session per thread, ignoring subsequent sessions\n");
         break;
      }

      printf("Multithread test thread id = %d, creating session %d\n", threadid, i);
      
      uFlags_session = DS_SESSION_MODE_IP_PACKET | DS_SESSION_DYN_CHAN_ENABLE;

      #ifdef ENABLE_MANAGED_SESSIONS
      uFlags_session |= DS_SESSION_USER_MANAGED;
      #endif

      if (!fNetIOAllowed) uFlags_session |= DS_SESSION_DISABLE_NETIO;
 
      hSessions_t[i] = DSCreateSession(hPlatform, NULL, &session_data_g[i], uFlags_session);  /* create session handle from global session data */

      if (hSessions_t[i] == -1) {

         printf("Failed to create multithread session thread id = %d, continuing test with already created sessions\n", threadid);
         break;
      }

      ptime_config[hSessions_t[i]] = session_data_g[i].term1.ptime;  /* get ptime value from session config file (in msec) */
      if ((int)ptime_config[hSessions_t[i]] <=0) ptime_config[hSessions_t[i]] = 5;  /* min supported ptime */

      nSessions++;
   }

   if (nSessions <= 0)
   {
      fprintf(stderr, "Failed to create any sessions in thread %d, exiting secondary thread\n", threadid);
      goto multithread_cleanup;
   }

/* calculate frame interval for each input stream (in msec) */

   for (i = threadid; i < nInFiles; i += nThreads_gbl) {

//      hSession = get_session_handle(hSessions_t, i, 0);
      hSession = hSessions_t[i];

      if ((int)frameInterval[i] == -1) frameInterval[i] = ptime_config[hSession];  /* if no cmd line entry, use ptime from session definition */
   }

/* Enter main processing loop, similar to primary thread above */

   while (run > 0) 
   {

   /* check if ptime has elapsed since last packet read */

      cur_time = get_time(USE_CLOCK_GETTIME);
      
      static bool fDataAvailable = false;

   /* incoming packets -- get next packet from all input pcap files specified in cmd line for this thread (-i cmd line option) */

      for (i = threadid; i < nInFiles; i += nThreads_gbl) {

         if (cur_time - last_time[i] >= frameInterval[i]*1000) {  /* has interval elapsed ?  (comparison is in usec) */

            if (!(packet_length = DSReadPcapRecord(fp_in[i], pkt_buffer, 0, NULL, link_layer_length[i]))) continue;
            else __sync_add_and_fetch(&num_pkts_read_multithread, 1);

            #ifdef ENABLE_MANAGED_SESSIONS
            hSession = hSessions_t[i];
            #else
            hSession = -1;
            #endif

            #ifdef OVERWRITE_INPUT_DATA
            if (fReuseInputs && !ReuseInputs(pkt_buffer, packet_length, hSession, &session_data_g[i])) continue;
            #endif

            packet_len[0] = packet_length;  /* fill in first packet_len[] item with total bytes to process, after call packet_len[] will contain lengths of all packets found to be correctly formatted, meeting all matching criteria, and added to the buffer */

            pkts_buffered = DSBufferPackets(hSession, DS_BUFFER_PKT_IP_PACKET | DS_BUFFER_PKT_DISABLE_PROBATION, pkt_buffer, packet_len, NULL, NULL);

            if (pkts_buffered < 0)
              printf("Multithread test thread id = %d, packet not added to jitter buffer, num pkts = %d\n", threadid, num_pkts_buffered_multithread); 
            else if (pkts_buffered > 0)
              __sync_add_and_fetch(&num_pkts_buffered_multithread, pkts_buffered);

            last_time[i] = cur_time;
         }
      }

      fDataAvailable = false;
      for (i=threadid; i<nInFiles; i+=nThreads_gbl) if (fp_in[i] && !feof(fp_in[i])) fDataAvailable = true;  /* check if all file data consumed */


      unsigned int uFlags_get = DS_BUFFER_PKT_IP_PACKET | (!fDataAvailable ? DS_GETORD_PKT_FLUSH : 0) | DS_PKT_INFO_NETWORK_BYTE_ORDER;

      for (i = threadid; i < (int)nSessions_gbl; i += nThreads_gbl)
      {
         if (hSessions_t[i] == -1) continue;
         
         pkt_ptr = recv_jb_buffer;
         uFlags_get |= DS_GETORD_PKT_SESSION;

         ret_val = DSGetOrderedPackets(hSessions_t[i], uFlags_get, pkt_ptr, packet_len, NULL);

         if (ret_val < 0) 
         {
            printf("Multithread test thread id = %d, error retrieving packet(s) from jitter buffer for session %d\n", threadid, i);
            continue;
         }

         #ifdef ENABLE_MANAGED_SESSIONS
         hSession = hSessions_t[i];
         #else
         hSession = -1;
         #endif

         for (j=0; j<ret_val; j++) {

            if (j > 0) pkt_ptr += packet_len[j-1];
            packet_length = packet_len[j];

            uint32_t uFlags_info = DS_BUFFER_PKT_IP_PACKET;

            if (uFlags_get & DS_PKT_INFO_HOST_BYTE_ORDER) uFlags_info = (uFlags_info & ~DS_PKT_INFO_NETWORK_BYTE_ORDER) | DS_PKT_INFO_HOST_BYTE_ORDER;
            else uFlags_info = (uFlags_info & ~DS_PKT_INFO_HOST_BYTE_ORDER) | DS_PKT_INFO_NETWORK_BYTE_ORDER;

            pyld_ptr = pkt_ptr + DSGetPacketInfo(hSession, uFlags_info | DS_PKT_INFO_RTP_PYLDOFS, pkt_ptr, packet_length, NULL, NULL);
            pyld_len = DSGetPacketInfo(hSession, uFlags_info | DS_PKT_INFO_RTP_PYLDLEN, pkt_ptr, packet_length, NULL, NULL);

            if ((hCodec = DSGetPacketInfo(hSession, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_CODEC, pkt_ptr, packet_length, &termInfo, NULL)) < 0)
            {
               fprintf(stderr, "Multithread test thread id = %d, failed to get decode codec info\n", threadid);
               break;
            }

            media_data_len = DSCodecDecode(hCodec, 0, pyld_ptr, media_data_buffer, pyld_len, NULL);

            if ((chnum = DSGetPacketInfo(hSession, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_CHNUM, pkt_ptr, packet_length, &termInfo, NULL)) < 0)
            {
               fprintf(stderr, "Multithread test thread id = %d, failed to get chnum\n", threadid);
               break;
            }

            out_media_data_len = DSConvertFsPacket(chnum, (int16_t*)media_data_buffer, media_data_len);

            if ((hCodec_link = DSGetPacketInfo(hSession, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_CODEC_LINK, pkt_ptr, packet_length, &termInfo_link, NULL)) < 0)
            {
               fprintf(stderr, "Multithread test thread id = %d, failed to get encode codec info\n", threadid);
               break;
            }

         /* encode raw audio frame given by decoder */

            pyld_len = DSCodecEncode(hCodec_link, 0, media_data_buffer, encoded_data_buffer, out_media_data_len, NULL);

         /* write output packet */

            packet_length = DSFormatPacket(chnum, 0, encoded_data_buffer, pyld_len, NULL, pkt_buffer);

            int pcap_index = get_pcap_index(i);  /* map codec handle to output pcap file index */

            /* write to file if this channel has an associated file handle, otherwise send packet over network */
            if (pcap_index >= 0 && fp_out[pcap_index] != NULL)
            {
               sem_wait(&pcap_write_sem);
               if (DSWritePcapRecord(fp_out[pcap_index], pkt_buffer, NULL, NULL, &termInfo_link, NULL, packet_length) < 0) {
                  sem_post(&pcap_write_sem);
                  fprintf(stderr, "Multithread test thread id = %d, problem with DSWritePcapRecord()\n", threadid);
                  continue;
               }
               sem_post(&pcap_write_sem);
               __sync_add_and_fetch(&pkt_write_cnt_multithread, 1);
            }
            else
            {

               #if defined(ENABLE_TERM_MODE_FIELD) && defined(ENABLE_TERM_MODE_DONT_CARE)
            /* don't send packet if term2 IP:port values for this packet have been defined as don't care */

               int hSession_index = DSGetPacketInfo(hSession, DS_PKT_INFO_SESSION, pkt_buffer, packet_length, &termInfo_link, NULL);  /* get session index and term2 info associated with the packet */

               if (!(termInfo_link.mode & TERMINATION_MODE_IP_PORT_DONTCARE))
               #endif
               {
                  send_packet(pkt_buffer, packet_length);  /* send packet using network socket */
               }
            }
         }

         __sync_add_and_fetch(&pkt_xcode_cnt_multithread, ret_val);
      }
   }

   /* Delete Sessions */
   for (i = threadid; i < (int)nSessions_gbl; i += nThreads_gbl)
   {
      if ((int)hSessions_t[i] >= 0) {

         printf("thread id %d, deleting session %d\n", threadid, i);

         DSDeleteSession(hSessions_t[i]);
      }
   }

multithread_cleanup:

   return (void*)0;
}

#endif  /* ENABLE_MULTITHREAD_OPERATION && !__LIBRARYMODE__ */


#ifdef OVERWRITE_INPUT_DATA
int ReuseInputs(uint8_t* pkt_buffer, unsigned int packet_length, uint32_t hSession, SESSION_DATA* session_data) {

struct iphdr *ip_hdr;
struct ipv6hdr *ipv6_hdr;
struct udphdr *udp_hdr;

   uint8_t version = (pkt_buffer[0] & 0xf0) >> 4;

   if (version == 4)
   {
      ip_hdr = (struct iphdr *)pkt_buffer;
      ip_hdr->saddr = session_data->term1.remote_ip.u.ipv4;
      ip_hdr->daddr = session_data->term1.local_ip.u.ipv4;
   }
   else if (version == 6)
   {
      ipv6_hdr = (struct ipv6hdr *)pkt_buffer;
      memcpy(ipv6_hdr->saddr.s6_addr, session_data->term1.remote_ip.u.ipv6, DS_IPV6_ADDR_LEN);
      memcpy(ipv6_hdr->daddr.s6_addr, session_data->term1.local_ip.u.ipv6, DS_IPV6_ADDR_LEN);
   }
   else
   {
      fprintf(stderr, "ReuseInputs(): invalid ip version for input packet: %d, dropping packet\n", version);
      return 0;
   }

   udp_hdr = (struct udphdr *)&pkt_buffer[DSGetPacketInfo(hSession, DS_PKT_INFO_IP_HDRLEN | DS_BUFFER_PKT_IP_PACKET, pkt_buffer, packet_length, NULL, NULL)];
   udp_hdr->source = session_data->term1.remote_port;
   udp_hdr->dest = session_data->term1.local_port;

   return 1;
}
#endif


#ifdef ENABLE_STREAM_GROUPS

/* turn off stream merging */
  
void DisableStreamMerging(int chnum_parent) {

TERMINATION_INFO termInfo, termInfo_link;

   if (DSGetSessionInfo(chnum_parent, DS_SESSION_INFO_CHNUM, 1, &termInfo) >= 0) {

#if 0
      termInfo.group_status = 0;  /* don't modify status, which is referenced in DSDeleteSession() to remove streams from groups, JHB Oct2018 */
#endif
      termInfo.group_mode = 0;
      DSSetSessionInfo(chnum_parent, DS_SESSION_INFO_CHNUM, 1, &termInfo);
   }

   if (DSGetSessionInfo(chnum_parent, DS_SESSION_INFO_CHNUM, 2, &termInfo_link) >= 0) {

#if 0
      termInfo_link.group_status = 0;
#endif
      termInfo_link.group_mode = 0;
      DSSetSessionInfo(chnum_parent, DS_SESSION_INFO_CHNUM, 2, &termInfo_link);
   }
}              

#endif  /* ENABLE_STREAM_GROUPS */

void DisplayChanInfo(HSESSION hSession, int num_chan, int chan_nums[], int thread_index) {

int j;
int ch[MAX_TERMS] = { 0 }, k = 0, group_chan = -1;
char tmpstr[256];

   for (j=0; j<num_chan; j++) {
      if (chan_nums[j] >= 0) ch[k++] = chan_nums[j];
      else group_chan = j;
   }

   if (k > MAX_TERMS) Log_RT(2, "CRITICAL: p/m thread %d says num parent chans %d exceeds session limit  %d \n", thread_index, k, MAX_TERMS);  /* sanity check, JHB Apr2020 */
 
   if (group_chan > 1) sprintf(tmpstr, "channels %d and %d active for session %d", ch[0], ch[1], hSession);
   else if (group_chan != 0) sprintf(tmpstr, "channel %d active for session %d", ch[0], hSession);

   if (group_chan >= 0) sprintf(&tmpstr[strlen(tmpstr)], ", group chan = %d", group_chan);

   strcat(tmpstr, ", calling DSGetOrderedPackets with SESSION_CHNUM flag\n");
   sig_printf(tmpstr, PRN_LEVEL_INFO, 0);
}


#define MAX_PKT_STATS_STRLEN 4000

void add_stats_str(char* stats_str, unsigned int max_len, const char* fmt, ...) {

   va_list va;
   char tmpstr[MAX_PKT_STATS_STRLEN];

   va_start(va, fmt);

   vsnprintf(tmpstr, sizeof(tmpstr), fmt, va);
   
   va_end(va);

   if (strlen(stats_str) + strlen(tmpstr) < max_len - 1) strcat(stats_str, tmpstr);
}

#ifdef PACKET_TIME_STATS

void RecordPacketTimeStats(int chnum, uint8_t* pkt, int pkt_len, uint32_t pkt_count_session, int flow_point) {

uint64_t packet_time;
uint32_t rtp_timestamp, Fs, pkt_count;
int idx;

   if (flow_point == PACKET_TIME_STATS_INPUT) {

      rtp_timestamp = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_TIMESTAMP, pkt, pkt_len, NULL, NULL);

      if (last_rtp_timestamp[chnum]) {  /* note that input RTP timestamps may be ooo, so we use int calculations here and allow negative values. They will still add up correctly, JHB Jan2020 */

         Fs = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_INPUT_SAMPLE_RATE, 0, NULL);
         if (!Fs) Fs = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_SAMPLE_RATE, 0, NULL);  /* use decode sample rate if no input sample rate was assigned */
         if (Fs) packet_rtp_time[chnum] += 1000*((int64_t)rtp_timestamp - (int64_t)last_rtp_timestamp[chnum])/Fs;  /* save in msec (and avoid divide-by-zero, JHB May2019) */
      }
 
      last_rtp_timestamp[chnum] = rtp_timestamp;

      packet_time = get_time(USE_CLOCK_GETTIME);

      if ((idx = DSGetStreamGroupInfo(chnum, DS_GETGROUPINFO_HANDLE_CHNUM, NULL, NULL, NULL)) >= 0) pkt_count = ++pkt_count_group[idx];
      else pkt_count = pkt_count_session;

      if (last_packet_in_time[chnum]) {

         uint64_t elapsed_time = packet_time - last_packet_in_time[chnum];

         packet_in_time[chnum] += elapsed_time;  /* save in usec */

         if (elapsed_time > packet_max_delta[chnum]) {
            packet_max_delta[chnum] = elapsed_time;
            max_delta_packet[chnum] = pkt_count;
         }

         int pyld_content = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLD_CONTENT, pkt, pkt_len, NULL, NULL);

         if (pyld_content == DS_PKT_PYLD_CONTENT_MEDIA && prev_pyld_content[chnum] == DS_PKT_PYLD_CONTENT_MEDIA) {

            packet_media_delta[chnum] += elapsed_time;
            media_stats_pkt_count[chnum]++;

            if (elapsed_time > packet_max_media_delta[chnum]) {
               packet_max_media_delta[chnum] = elapsed_time;
               max_media_delta_packet[chnum] = pkt_count;
            }
         }
         else if (pyld_content == DS_PKT_PYLD_CONTENT_SID && prev_pyld_content[chnum] == DS_PKT_PYLD_CONTENT_SID) {

            packet_sid_delta[chnum] += elapsed_time;
            sid_stats_pkt_count[chnum]++;

            if (elapsed_time > packet_max_sid_delta[chnum]) {
               packet_max_sid_delta[chnum] = elapsed_time;
               max_sid_delta_packet[chnum] = pkt_count;
            }
         }

         prev_pyld_content[chnum] = pyld_content;
      }

      last_packet_in_time[chnum] = packet_time;
   }
   else if (flow_point == PACKET_TIME_STATS_PULL) {

      rtp_timestamp = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_TIMESTAMP, pkt, pkt_len, NULL, NULL);

      if (last_rtp_timestamp_pull[chnum]) {

         Fs = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_INPUT_SAMPLE_RATE, 0, NULL);
         if (!Fs) Fs = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_SAMPLE_RATE, 0, NULL);  /* use decode sample rate if no input sample rate was assigned */
         if (Fs) packet_rtp_time_pull[chnum] += 1000*((int64_t)rtp_timestamp - (int64_t)last_rtp_timestamp_pull[chnum])/Fs;  /* save in msec (and avoid divide-by-zero, JHB May2019) */
      }

      last_rtp_timestamp_pull[chnum] = rtp_timestamp;

      packet_time = get_time(USE_CLOCK_GETTIME);

      if (last_packet_in_time_pull[chnum]) packet_in_time_pull[chnum] += packet_time - last_packet_in_time_pull[chnum];  /* save in usec */

      last_packet_in_time_pull[chnum] = packet_time;
   }
}

#endif  /* PACKET_TIME_STATS */


extern uint32_t event_log_critical_errors;  /* in diagalib.so */
extern uint32_t event_log_errors;
extern uint32_t event_log_warnings;

/* DSLogPacketTimeLossStats() is called by ManageSessions before sessions are deleted, depending on uPktStatsLogging enables (DEBUG_CONFIG struct in config.h).  Can also be called by applications as needed */

int DSLogPacketTimeLossStats(HSESSION hSession, unsigned int uFlags) {

#define MAX_CHAN_TRACKED 64  /* max channels per session counted for stats.  This should be an out-of-reach number in real world processing, it would be 60+ child channels in one session */
int thread_index;
int ch[MAX_CHAN_TRACKED], num_ch, num_dyn_ch, i, j;
char szGroupId[MAX_GROUPID_LEN];
int  idx = -1, nc, nContributors, ch_list[MAX_GROUP_CONTRIBUTORS], num_sessions = 0, num_ch_stats = 0;
bool fNext, fOrganizeByStreamGroup = false, fShowOwnerOnce;
HSESSION hSessionGroupOwner = -1, hSession_prev = -1;

#define MAX_STATS_STRLEN 80
char iptstr[MAX_STATS_STRLEN] = "", jbptstr[MAX_STATS_STRLEN] = "", jbrpstr[MAX_STATS_STRLEN] = "", jbzpstr[MAX_STATS_STRLEN] = "", mxooostr[MAX_STATS_STRLEN] = "", sidrstr[MAX_STATS_STRLEN] = "", tsastr[MAX_STATS_STRLEN] = "", plflstr[MAX_STATS_STRLEN] = "",
     pdflstr[MAX_STATS_STRLEN] = "", ssrcstr[MAX_STATS_STRLEN] = "", missstr[MAX_STATS_STRLEN] = "", consstr[MAX_STATS_STRLEN] = "", pktlstr[MAX_STATS_STRLEN] = "", calcstr[MAX_STATS_STRLEN] = "", sessstr[MAX_STATS_STRLEN] = "", npktstr[MAX_STATS_STRLEN] = "",
     undrstr[MAX_STATS_STRLEN] = "", ovrnstr[MAX_STATS_STRLEN] = "", medstr[MAX_STATS_STRLEN] = "", sidstr[MAX_STATS_STRLEN] = "", medxstr[MAX_STATS_STRLEN] = "", sidxstr[MAX_STATS_STRLEN] = "", maxdstr[MAX_STATS_STRLEN] = "", brststr[MAX_STATS_STRLEN] = "",
     sidistr[MAX_STATS_STRLEN] = "", tsamstr[MAX_STATS_STRLEN] = "", purgstr[MAX_STATS_STRLEN] = "", dupstr[MAX_STATS_STRLEN] = "", jbundrstr[MAX_STATS_STRLEN] = "", jboverstr[MAX_STATS_STRLEN] = "", iooostr[MAX_STATS_STRLEN] = "", jboooostr[MAX_STATS_STRLEN] = "",
     jbmxooostr[MAX_STATS_STRLEN] = "", jbdropstr[MAX_STATS_STRLEN] = "", jbdupstr[MAX_STATS_STRLEN] = "", jbtgapstr[MAX_STATS_STRLEN] = "", mxovrnstr[MAX_STATS_STRLEN] = "", mxnpktstr[MAX_STATS_STRLEN] = "", noutpkts[MAX_STATS_STRLEN] = "",
     jbhldadj[MAX_STATS_STRLEN] = "", jbhlddel[MAX_STATS_STRLEN] = "", pobrststr[MAX_STATS_STRLEN] = "", lvflstr[MAX_STATS_STRLEN] = "";

   if (uFlags & DS_LOG_PKT_STATS_ORGANIZE_BY_STREAM_GROUP) {

      hSessionGroupOwner = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_OWNER, 0, NULL);

      if (hSessionGroupOwner == -2) {
         Log_RT(3, "WARNING: DSLogPacketTimeLossStats() says invalid hSession %d \n", hSession);
         return -1;
      }

      if (hSessionGroupOwner == -1) {

         if (!(uFlags & DS_LOG_PKT_STATS_SUPPRESS_ERROR_MSG)) Log_RT(3, "WARNING: DSLogPacketTimeLossStats() says hSession %d not a stream group member \n", hSession);
         goto organize_by_ssrc;  /* hSession is not a group member */
      }

      idx = DSGetStreamGroupInfo(hSessionGroupOwner, DS_GETGROUPINFO_CHECK_ALLTERMS, &nContributors, ch_list, szGroupId);

      if (idx < 0) {
         Log_RT(3, "WARNING: DSLogPacketTimeLossStats() says invalid stream group index %d \n", idx);
         return -1;
      }

      if (!nContributors) {
         Log_RT(3, "WARNING: DSLogPacketTimeLossStats() says stream group %d has no active contributors \n", idx);
         return 0;
      }

      thread_index = DSGetSessionInfo(hSessionGroupOwner, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_THREAD, 0, NULL);

      fOrganizeByStreamGroup = true;
      fShowOwnerOnce = false;
      nc = 0;
   }
   else {

organize_by_ssrc:
   
      thread_index = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_THREAD, 0, NULL);
   }

   if (thread_index < 0) {
      Log_RT(3, "WARNING: DSLogPacketTimeLossStats() says invalid p/m thread index found for hSession %d \n", fOrganizeByStreamGroup ? hSessionGroupOwner : hSession);
      return -1;
   }

/* if session is marked for deletion we assume we're here from ManageSessions() */

   bool fDeletePending = (DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_DELETE_STATUS, 0, NULL) & DS_SESSION_DELETE_PENDING) != 0;  /* note: accessing session delete status is reserved ... don't use it unless you know what you're doing */

   do {

      num_ch = 0;

      if (fOrganizeByStreamGroup) hSession = DSGetSessionInfo(ch_list[nc], DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_SESSION, 0, NULL);

      for (j=0; j<MAX_TERMS; j++) {

         int ret_val = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CHNUM, j+1, NULL);

         if (ret_val < 0) {
            Log_RT(2, "ERROR: DSLogPacketTimeLossStats() reports DSGetSessionInfo() error code %d, possibly invalid session handle %d \n", ret_val, hSession);
            return ret_val;
         }

         ch[num_ch] = ret_val;
         if (num_ch < MAX_CHAN_TRACKED/2) num_ch++;
      }

      for (j=0; j<MAX_TERMS; j++) {
         num_dyn_ch = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_DYNAMIC_CHANNELS, j+1, (void*)&ch[num_ch]);  /* need typecast here, otherwise compiler messes up.  Not sure why, JHB Feb2019 */
         if (num_ch + num_dyn_ch <= MAX_CHAN_TRACKED) num_ch += num_dyn_ch;
      }

      for (i=0; i<num_ch; i++) {

         int c = ch[i];

         if (fDeletePending) {
            if (session_run_time_stats[c] & 1) continue;  /* run-time stats for this channel have already been logged prior to deletion, don't log them twice, JHB Jan2020 */
            else session_run_time_stats[c] |= 1;
         }

         if (DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_INPUT_PKT_COUNT | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING)) {  /* show stats for any stream with at least one packet. Note this will exclude streams that didn't start yet, for example mediaMin was terminated early */

            num_ch_stats++;

            if (lib_dbg_cfg.uPktStatsLogging & DS_ENABLE_PACKET_TIME_STATS) {

               add_stats_str(iptstr, MAX_STATS_STRLEN, " %d/%2.2f/%2.2f", c, 1.0*packet_in_time[c]/1000000L, 1.0*packet_rtp_time[c]/1000);
               add_stats_str(jbptstr, MAX_STATS_STRLEN, " %d/%2.2f/%2.2f", c, 1.0*packet_in_time_pull[c]/1000000L, 1.0*packet_rtp_time_pull[c]/1000);
               add_stats_str(medstr, MAX_STATS_STRLEN, " %d/%2.2f", c, 1.0*packet_media_delta[c]/media_stats_pkt_count[c]/1000);
               add_stats_str(sidstr, MAX_STATS_STRLEN, " %d/%2.2f", c, 1.0*packet_sid_delta[c]/sid_stats_pkt_count[c]/1000);
               add_stats_str(medxstr, MAX_STATS_STRLEN, " %d/%2.2f/%d", c, 1.0*packet_max_media_delta[c]/1000, max_media_delta_packet[c]);
               add_stats_str(sidxstr, MAX_STATS_STRLEN, " %d/%2.2f/%d", c, 1.0*packet_max_sid_delta[c]/1000, max_sid_delta_packet[c]);
               add_stats_str(maxdstr, MAX_STATS_STRLEN, " %d/%2.2f/%d", c, 1.0*packet_max_delta[c]/1000, max_delta_packet[c]);
            }

            if (hSession != hSession_prev) {

               TERMINATION_INFO termInfo;
               char codec_name[50] = "";

               DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_TERM, 1, &termInfo);

               #if 0  /* example of how to use voplib DSGetCodecInfo() with a codec handle, JHB Oct2020 */
               CODEC_PARAMS codec_params;
               HCODEC hCodec = DSGetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_CODEC, 1, NULL);  /*  1 = decoder, 2 = encoder ... see pktlib.h comments */
               DSGetCodecInfo(hCodec, DS_CODEC_INFO_HANDLE, &codec_params);  /* voplib API */
               strcpy(codec_name, codec_params.codec_name);
               // printf(" bit rate = %d, sampling rate = %d \n", codec_params.dec_params.bitRate, codec_params.dec_params.samplingRate);
               #else  /* or if codec type is already known ... */
               DSGetCodecInfo(termInfo.codec_type, DS_CODEC_INFO_TYPE, codec_name);
               #endif

               add_stats_str(sessstr, MAX_STATS_STRLEN, " %d%s/%d/%s/%d", hSession, hSession == hSessionGroupOwner && !fShowOwnerOnce ? "(grp owner)" : "", c, codec_name, termInfo.bitrate);
            }
            else add_stats_str(sessstr, MAX_STATS_STRLEN, ",%d", c);  /* add channel to previous session string, showing it as a dynamic (child) channel */

            if (fOrganizeByStreamGroup) {

               if (DSGetSessionInfo(c, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_CHNUM_PARENT, 0, NULL) == c) {  /* add overrun stats only for parent channels (as explained in streamlib.h, child channels contribute to their parent group member stream), JHB Apr2020 */

                  add_stats_str(ovrnstr, MAX_STATS_STRLEN, " %d/%u", c, uFramesDropped[c]);

                  HCODEC hCodec;
                  int framesize;

                  if ((hCodec = DSGetSessionInfo(c, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_CODEC, 1, NULL)) > 0 && (framesize = DSGetCodecRawFrameSize(hCodec)) > 0) add_stats_str(mxovrnstr, MAX_STATS_STRLEN, " %d/%2.2f", c, 100.0*nMaxStreamDataAvailable[c]/framesize/DSGetStreamGroupContributorMaxFrameCapacity(c));
               }

               if (!fShowOwnerOnce) add_stats_str(undrstr, MAX_STATS_STRLEN, " %d/%d/%d", idx, num_missed_interval_index[idx], num_flc_applied[idx]);

               fShowOwnerOnce = true;
            }

         /* use post delete flag here, as app may have already marked sessions for deletion */
  
            add_stats_str(ssrcstr, MAX_STATS_STRLEN, " %d/0x%x", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_SSRC | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(npktstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_INPUT_PKT_COUNT | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(brststr, MAX_STATS_STRLEN, " %d/%d", c, packet_in_bursts[c]);
            add_stats_str(pktlstr, MAX_STATS_STRLEN, " %d/%2.3f", c, 100.0*DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_MISSING_SEQ_NUM | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING)/max(DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_INPUT_PKT_COUNT | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING), 1));

            add_stats_str(plflstr, MAX_STATS_STRLEN, " %d/%d", c, pkt_loss_flush[c]);
            add_stats_str(pdflstr, MAX_STATS_STRLEN, " %d/%d", c, pkt_pastdue_flush[c]);
            add_stats_str(lvflstr, MAX_STATS_STRLEN, " %d/%d", c, pkt_level_flush[c]);

            add_stats_str(dupstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_NUM_7198_DUPLICATE_PKTS | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));

            if (lib_dbg_cfg.uPktStatsLogging & DS_ENABLE_PACKET_LOSS_STATS) {  /* run-time packet loss stats can be disabled in pktlib in case they cause any issue, JHB Mar2020 */

               add_stats_str(missstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_MISSING_SEQ_NUM | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
               add_stats_str(consstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_MAX_CONSEC_MISSING_SEQ_NUM | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
               add_stats_str(iooostr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_NUM_INPUT_OOO | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
               add_stats_str(mxooostr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_MAX_INPUT_OOO | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
               add_stats_str(calcstr, MAX_STATS_STRLEN, " %d/%2.2f", c, 1.0*DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_STATS_CALC_PER_PKT | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING)/max(DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_INPUT_PKT_COUNT | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING), 1));
            }

            add_stats_str(jbrpstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_NUM_PKTS | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));  /* residual num pkts */
            add_stats_str(jbzpstr, MAX_STATS_STRLEN, " %d/%d", c, num_jb_zero_pulls[c]);

         /* repair stats */

            add_stats_str(sidrstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_SID_REPAIR | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(tsastr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_SID_TIMESTAMP_ALIGN | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(tsamstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_MEDIA_TIMESTAMP_ALIGN | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(sidistr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_SID_REPAIR_INSTANCE | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));

         /* jitter buffer stats */

            add_stats_str(noutpkts, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_OUTPUT_PKT_COUNT | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(mxnpktstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_MAX_NUM_PKTS | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(jbdropstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_NUM_OUTPUT_DROP_PKTS | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(jbdupstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_NUM_OUTPUT_DUPLICATE_PKTS | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(jboooostr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_NUM_OUTPUT_OOO | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(jbmxooostr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_MAX_OUTPUT_OOO | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(purgstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_NUM_PURGES | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(jbundrstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_UNDERRUN_RESYNC_COUNT | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(jboverstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_OVERRUN_RESYNC_COUNT | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(jbtgapstr, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_TIMESTAMP_GAP_RESYNC_COUNT | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(jbhldadj, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_NUM_HOLDOFF_ADJUSTS | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(jbhlddel, MAX_STATS_STRLEN, " %d/%d", c, DSGetJitterBufferInfo(c, DS_JITTER_BUFFER_INFO_NUM_HOLDOFF_DELIVERIES | DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING));
            add_stats_str(pobrststr, MAX_STATS_STRLEN, " %d/%d", c, packet_out_bursts[c]);
         }

         if (hSession != hSession_prev) num_sessions++;
         hSession_prev = hSession;
      }

      if (fOrganizeByStreamGroup) fNext = (++nc < nContributors);
      else fNext = false;

   } while (fNext);


   if (num_ch_stats) {

      char pkt_stats_str[MAX_PKT_STATS_STRLEN] = "Stream Info + Stats, ";

   /* Main stats heading is either group info (if fOrganizeByStreamGroup is set), or session info:

      -if fOrganizeByStreamGroup is set, then session info is a sub-indent
      -first 2 code lines below form the main heading, don't insert something between them
      -to-do: if fOrganizeByStreamGroup not set, then we should add each session's thread index
      -to-do: if fOrganizeByStreamGroup is set, then we should add each session's thread index if the group's STREAM_CONTRIBUTOR_WHOLE_GROUP_THREAD_ALLOCATE flag not set. Otherwise adding thread_index to stream group info (which we do now) is ok. DSGetSessionInfo() with DS_SESSION_INFO_THREAD_ID can be used
   */

      if (fOrganizeByStreamGroup) add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "stream group \"%s\", grp %d, p/m thread %d, num packets %d \n", szGroupId, idx, thread_index, pkt_count_group[idx]);
      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "%session%s (hSession/ch/codec/bitrate[,ch...])%s\n", fOrganizeByStreamGroup ? "  S" : "s", num_sessions > 1 ? "s" : "", sessstr);

      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "  SSRC%s (ch/ssrc)%s\n", num_ch_stats > 1 ? "s" : "", ssrcstr);

      if (fOrganizeByStreamGroup) {
         add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "  Overrun (ch/frames dropped)%s, (ch/max %%)%s\n", ovrnstr, mxovrnstr);
         add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "  Underrun (grp/missed intervals/FLCs)%s\n", undrstr);
         add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "  Pkt flush (ch/num) loss%s, pastdue%s, level%s\n", plflstr, pdflstr, lvflstr);
      }

      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "  Packet Stats\n");
      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Input (ch/pkts)%s, RFC7198 duplicates%s, bursts%s\n", npktstr, dupstr, brststr);

      if (lib_dbg_cfg.uPktStatsLogging & DS_ENABLE_PACKET_LOSS_STATS) {
         add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Loss (ch/%%)%s, missing seq (ch/num)%s, max consec missing seq%s\n", pktlstr, missstr, consstr);
         add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Ooo (ch/pkts)%s, max%s\n", iooostr, mxooostr);
         add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Avg stats calcs (ch/num)%s\n", calcstr);
      }

      if (lib_dbg_cfg.uPktStatsLogging & DS_ENABLE_PACKET_TIME_STATS) {

         add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Delta avg (ch/msec) media%s, SID%s\n", medstr, sidstr);
         add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Delta max (ch/msec/pkt) media%s, SID%s, overall%s\n", medxstr, sidxstr, maxdstr);
         add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Cumulative input times         (sec) (ch/inp/rtp)%s\n", iptstr);
         add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Cumulative jitter buffer times (sec) (ch/out/rtp)%s\n", jbptstr);
      }

      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "  Packet Repair\n");
      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    SID repair (ch/num) instance%s, total%s\n", sidistr, sidrstr);
      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Timestamp repair (ch/num) SID%s, media%s\n", tsastr, tsamstr);

      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "  Jitter Buffer\n");
      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Output (ch/pkts)%s, max%s, residual%s, bursts%s\n", noutpkts, mxnpktstr, jbrpstr, pobrststr);
      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Ooo (ch/pkts)%s, max%s, drops%s, duplicates%s\n", jboooostr, jbmxooostr, jbdropstr, jbdupstr);
      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Resyncs (ch/num) underrun%s, overrun%s, timestamp gap%s, purges (ch/num)%s\n", jbundrstr, jboverstr, jbtgapstr, purgstr);
      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "    Holdoffs (ch/num) adj%s, dlvr%s, zero pulls (ch/num)%s\n", jbhldadj, jbhlddel, jbzpstr);

   /* include event log stats, to make it easier to see if anything happened to worry about. Note we use a few alternate characters to avoid this line turning up in manual or automated log searches for "warning", "error", etc, JHB May2020 */

      add_stats_str(pkt_stats_str, MAX_PKT_STATS_STRLEN, "  Event log w¨¡rnings, ¨¥rrors, cr¨ªtical %u, %u, %u\n", __sync_fetch_and_add(&event_log_warnings, 0), __sync_fetch_and_add(&event_log_errors, 0), __sync_fetch_and_add(&event_log_critical_errors, 0));

      Log_RT(6 | DS_LOG_LEVEL_NO_API_CHECK, pkt_stats_str);  /* write packet stats string to event log file.  Note that we make a single Log_RT() call with a multi-line string to avoid interleaving with Log_RT() output from other threads */
   }

   return num_ch_stats;
}


/* helper function to control screen print out */

void sig_printf(char* prnstr, int level, int thread_index) {  /* note: do not use with const char* strings unless they have an extra 10 chars or so of room */

int slen;
static bool fOnce = false;
bool fNewLine = false, fNextLine = false;

   if (!fOnce && isMasterThread) {  /* only possible reason this could be useful is if for some reason the app didn't call DSConfigPktlib(), which would be bad app behavior */
 
      printf("lib_dbg_cfg.uPrintfLevel = %d\n", lib_dbg_cfg.uPrintfLevel);
      printf("lib_dbg_cfg.uLogLevel = %d\n", lib_dbg_cfg.uLogLevel);
      printf("lib_dbg_cfg.uPrintfControl = %d\n", lib_dbg_cfg.uPrintfControl);
      printf("lib_dbg_cfg.uEventLogMode = 0x%x\n", lib_dbg_cfg.uEventLogMode);
      printf("lib_dbg_cfg.uEventLogFile = 0x%p\n", lib_dbg_cfg.uEventLogFile ? lib_dbg_cfg.uEventLogFile : NULL);
      printf("lib_dbg_cfg.uPktStatsLogging = 0x%x\n", lib_dbg_cfg.uPktStatsLogging);
      fOnce = true;
   }

   if ((level & PRN_LEVEL_MASK) > lib_dbg_cfg.uPrintfLevel) return;  /* default level should be set to 5.  See above at start of packet_flow_media_proc() and also mediaMin.c. 'o' interactive key in mediaMin sets uPrintfLevel to "none", effectively turning off p/m thread screen output */

   slen = strlen(prnstr);

#if 0  /* I think this is actually incorrect, since .num_streams_active can be more than 1 within one p/m thread (for example due to child streams), plus it never decreases or resets ... JHB Mar2020 */
   int i, num_threads_active = 0;
   for (i=0; i < (int)__sync_fetch_and_add(&num_pktmedia_threads, 0); i++) if (packet_media_thread_info[i].num_streams_active) num_threads_active++;

   if (thread_index >= 0 && num_threads_active > 1) {  /* add thread identifier if multiple pkt/media threads are running */
#else
   if (thread_index >= 0 && __sync_fetch_and_add(&num_pktmedia_threads, 0) > 1) {
#endif
      if (slen && prnstr[slen-1] == '\n') {
         prnstr[--slen] = (char)0;  /* remove \n if at end of string */
         fNewLine = true;
      }

      sprintf(&prnstr[slen], " (%d)", thread_index);  /* tack on thread index */
      if (fNewLine) strcat(prnstr, " \n");  /* restore \n if needed */
      slen = strlen(prnstr);
   }

   __sync_or_and_fetch(&pm_thread_printf, 1 << thread_index);  /* set pm_thread_printf bit */

   if (!(level & PRN_SAME_LINE) && __sync_val_compare_and_swap(&isCursorMidLine, 1, 0)) fNextLine = true;  /* fNextLine reflects leading \n decision */
   else if (prnstr[slen-1] != '\n') __sync_val_compare_and_swap(&isCursorMidLine, 0, 1);

   if (lib_dbg_cfg.uPrintfControl == 0) printf("%s%s", fNextLine ? "\n" : "", prnstr);  /* 0 = fully buffered (default), 1 = stdout (line buffered), 2 = stderr (no buffering; e.g. per character), 3 = none */
   else if (lib_dbg_cfg.uPrintfControl == 1) fprintf(stdout, "%s%s", fNextLine ? "\n" : "", prnstr);
   else if (lib_dbg_cfg.uPrintfControl == 2) fprintf(stderr, "%s%s", fNextLine ? "\n" : "", prnstr);

   uLineCursorPos = (slen && prnstr[slen-1] != '\n') ? slen : 0;

   __sync_and_and_fetch(&pm_thread_printf, ~(1 << thread_index));  /* clear  pm_thread_printf bit */
}

bool DSIsPktStatsHistoryLoggingEnabled(int thread_index) {

   if (packet_media_thread_info[thread_index].fMediaThread) return ((lib_dbg_cfg.uPktStatsLogging & DS_ENABLE_PACKET_STATS_HISTORY_LOGGING) != 0);
   else return (bool)use_log_file;
}

#ifdef USE_CHANNEL_PKT_STATS
void manage_pkt_stats_mem(PKT_STATS_HISTORY pkt_stats[], int chnum, int num_pkts) {

   pkt_stats[chnum].num_pkts += num_pkts;
   pkt_stats[chnum].mem_usage += sizeof(PKT_STATS);
   if ((pkt_stats[chnum].mem_usage % PKT_STATS_CHUNK_SIZE) == 0) pkt_stats[chnum].pkt_stats = realloc(pkt_stats[chnum].pkt_stats, pkt_stats[chnum].mem_usage + PKT_STATS_CHUNK_SIZE);
}
#endif

/* Debug Manager.  Notes:

  -currently implemented as a function that can be invoked at run-time by apps controlling the 'run' global var
  -called by p/m thread loop just after ManageSessions and just before input stage
  -DSDisplayThreadDebugInfo() in pktlib implements the run var method
*/

extern char dtdi_userstr[MAX_PKTMEDIA_THREADS][MAX_DTDI_STR_LEN];  /* pktlib.c */
extern volatile unsigned int channel_max_bucket_depth, lookup_hash_max_loops, min_free_channel_handles;  /* net_pkt.c */
extern int max_groups, max_sessions, min_free_session_handles;  /* pktlib.c */

void ThreadDebugOutput(HSESSION hSessions_t[], int numSessions, int level, int thread_index, unsigned int uFlags) {

int i, j, cpu;
uint64_t cpu_time_sum = 0, manage_time_sum = 0, input_time_sum = 0, buffer_time_sum = 0, chan_time_sum = 0, pull_time_sum = 0, decode_time_sum = 0, encode_time_sum = 0, group_time_sum = 0;
uint64_t buf_pkt_sum = 0, enc_pkt_sum = 0, dec_pkt_sum = 0, group_contrib_sum = 0, num_counted = 0, num_buf_counted = 0, num_enc_counted = 0, num_dec_counted = 0, num_group_counted = 0;
int __attribute__ ((unused)) session_id, in_use;
int group_idx[MAX_STREAM_GROUPS] = { 0 };
int group_member_count[MAX_STREAM_GROUPS] = { 0 };
int group_member_sessions[MAX_STREAM_GROUPS][MAX_GROUP_CONTRIBUTORS] = {{ 0 }};
char szGroupId[MAX_STREAM_GROUPS][MAX_GROUPID_LEN] = { "" };
char group_name[MAX_GROUPID_LEN] = "";
char tmpstr[8000] = "";

   for (i=0; i<THREAD_STATS_TIME_MOVING_AVG; i++) {

      if (packet_media_thread_info[thread_index].CPU_time_avg[i] > 0) {

         cpu_time_sum += packet_media_thread_info[thread_index].CPU_time_avg[i];
         num_counted++;
      }

      if (packet_media_thread_info[thread_index].fProfilingEnabled) {

         manage_time_sum += packet_media_thread_info[thread_index].manage_time[i];
         input_time_sum += packet_media_thread_info[thread_index].input_time[i];
         buffer_time_sum += packet_media_thread_info[thread_index].buffer_time[i];
         chan_time_sum += packet_media_thread_info[thread_index].chan_time[i];
         pull_time_sum += packet_media_thread_info[thread_index].pull_time[i];
         decode_time_sum += packet_media_thread_info[thread_index].decode_time[i];
         encode_time_sum += packet_media_thread_info[thread_index].encode_time[i];
         group_time_sum += packet_media_thread_info[thread_index].group_time[i];
      }

      if (packet_media_thread_info[thread_index].num_buffer_packets[i]) { buf_pkt_sum += packet_media_thread_info[thread_index].num_buffer_packets[i]; num_buf_counted++; }
      if (packet_media_thread_info[thread_index].num_encode_packets[i]) { enc_pkt_sum += packet_media_thread_info[thread_index].num_encode_packets[i]; num_enc_counted++; }
      if (packet_media_thread_info[thread_index].num_decode_packets[i]) { dec_pkt_sum += packet_media_thread_info[thread_index].num_decode_packets[i]; num_dec_counted++; }
      if (packet_media_thread_info[thread_index].num_group_contributions[i]) { group_contrib_sum += packet_media_thread_info[thread_index].num_group_contributions[i]; num_group_counted++; }
   }

   #ifndef _9APR20_
   strcpy(tmpstr, dtdi_userstr[thread_index]);
   #endif

   cpu = sched_getcpu();

   sprintf(&tmpstr[strlen(tmpstr)], "Debug info for p/m thread %d, CPU %d, usage (msec) avg = %2.2f, max = %2.2f, flags = 0x%x, state = %s, es count = %d, max inactivity time (sec) = %d, ms mismatch = %d, ms create early exit = %d,  ms delete early exit = %d, max preemption time (msec) = %4.2f\n", thread_index, cpu, 1.0*cpu_time_sum/max(num_counted, 1)/1000, 1.0*packet_media_thread_info[thread_index].CPU_time_max/1000, packet_media_thread_info[thread_index].uFlags, packet_media_thread_info[thread_index].nEnergySaverState ? "energy save" : "run", packet_media_thread_info[thread_index].energy_saver_state_count, (int)(packet_media_thread_info[thread_index].max_inactivity_time/1000000L), packet_media_thread_info[thread_index].manage_sessions_count_mismatch, packet_media_thread_info[thread_index].manage_sessions_create_early_exit, packet_media_thread_info[thread_index].manage_sessions_delete_early_exit, 1.0*packet_media_thread_info[thread_index].max_elapsed_time_thread_preempt/1000);

   if (packet_media_thread_info[thread_index].fProfilingEnabled) {

      sprintf(&tmpstr[strlen(tmpstr)], "ravg/max: manage %2.2f/%2.2f, input %2.2f/%2.2f, bufr %2.2f/%2.2f, chan %2.2f/%2.2f, pull %2.2f/%2.2f, dec %2.2f/%2.2f, fs+enc %2.2f/%2.2f, sg %2.2f/%2.2f\n", 1.0*manage_time_sum/THREAD_STATS_TIME_MOVING_AVG/1000, 1.0*packet_media_thread_info[thread_index].manage_time_max/1000, 1.0*input_time_sum/THREAD_STATS_TIME_MOVING_AVG/1000, 1.0*packet_media_thread_info[thread_index].input_time_max/1000, 1.0*buffer_time_sum/THREAD_STATS_TIME_MOVING_AVG/1000, 1.0*packet_media_thread_info[thread_index].buffer_time_max/1000, 1.0*chan_time_sum/THREAD_STATS_TIME_MOVING_AVG/1000, 1.0*packet_media_thread_info[thread_index].chan_time_max/1000, 1.0*pull_time_sum/THREAD_STATS_TIME_MOVING_AVG/1000, 1.0*packet_media_thread_info[thread_index].pull_time_max/1000, 1.0*decode_time_sum/THREAD_STATS_TIME_MOVING_AVG/1000, 1.0*packet_media_thread_info[thread_index].decode_time_max/1000, 1.0*encode_time_sum/THREAD_STATS_TIME_MOVING_AVG/1000, 1.0*packet_media_thread_info[thread_index].encode_time_max/1000, 1.0*group_time_sum/THREAD_STATS_TIME_MOVING_AVG/1000, 1.0*packet_media_thread_info[thread_index].group_time_max/1000);
   }

   sprintf(&tmpstr[strlen(tmpstr)], "buffer pkts = %2.2f, decode pkts = %2.2f, encode pkts = %2.2f, stream group contributions = %2.2f \n", 1.0*buf_pkt_sum/max(num_buf_counted, 1), 1.0*enc_pkt_sum/max(num_enc_counted, 1), 1.0*dec_pkt_sum/max(num_dec_counted, 1), 1.0*group_contrib_sum/max(num_group_counted, 1));

   char sessstr[20];
   if (numSessions >= 0) sprintf(sessstr, "numSessions = %d, ", numSessions);
   else strcpy(sessstr, "");  /* this case if called from ThreadAbort() */
   
   sprintf(&tmpstr[strlen(tmpstr)], "%sthread numSessions = %d, thread numGroups = %d\n", sessstr, packet_media_thread_info[thread_index].numSessions, packet_media_thread_info[thread_index].numGroups);

/* gather group info about sessions in this thread */

   if (hSessions_t) {  /* NULL if called from ThreadAbort() */

      if (numSessions > 0) sprintf(&tmpstr[strlen(tmpstr)], "hSessions_t[0..%d] =", numSessions-1);
      else sprintf(&tmpstr[strlen(tmpstr)], "hSessions_t[] =");

      int group_info_count = 0, idx;

      for (i=0; i<MAX_SESSIONS; i++) {

         sprintf(&tmpstr[strlen(tmpstr)], " %d", hSessions_t[i]);

         if (hSessions_t[i] >= 0) {

            if ((idx = DSGetStreamGroupInfo(hSessions_t[i], DS_GETGROUPINFO_CHECK_ALLTERMS, NULL, NULL, NULL)) >= 0) {

               group_idx[idx] = hSessions_t[i] + 1;  /* fill in group map location if session is a member of any group */
               group_info_count++;
            }

            if ((idx = DSGetStreamGroupInfo(hSessions_t[i], DS_GETGROUPINFO_CHECK_TERM1, NULL, NULL, NULL)) >= 0) group_member_sessions[idx][group_member_count[idx]++] = hSessions_t[i] + 1;  /* either term1 or term2 count for member sessions */
            else if ((idx = DSGetStreamGroupInfo(hSessions_t[i], DS_GETGROUPINFO_CHECK_TERM2, NULL, NULL, NULL)) >= 0) group_member_sessions[idx][group_member_count[idx]++] = hSessions_t[i] + 1;

            if (idx >= 0) {

               if ((idx = DSGetStreamGroupInfo(hSessions_t[i], DS_GETGROUPINFO_CHECK_GROUPTERM, NULL, NULL, group_name)) >= 0) {

                  group_member_sessions[idx][group_member_count[idx]-1] |= 0x10000;  /* flag indicating the session is also a group owner */
                  strcpy(szGroupId[idx], group_name);
               }
            }
         }
      }

      sprintf(&tmpstr[strlen(tmpstr)], "\n");

      if (numSessions > 0) {
   
         sprintf(&tmpstr[strlen(tmpstr)], "avg pkt[0..%d] =", numSessions-1);

         for (i=0; i<numSessions; i++) {

            uint32_t pkt_cnt =  min(pkt_count[hSessions_t[i]], DELTA_SUM_LENGTH);

            if (hSessions_t[i] >= 0) sprintf(&tmpstr[strlen(tmpstr)], " %1.2f/%d", pkt_cnt ? 1.0*pkt_delta_sum[hSessions_t[i]]/pkt_cnt/1000 : -1, DSPushPackets(DS_PUSHPACKETS_GET_QUEUE_LEVEL, NULL, NULL, &hSessions_t[i], 1));
         }

         sprintf(&tmpstr[strlen(tmpstr)], "\n");
      }


   /* Display info about any of this thread's sessions attached to a group.  Notes:

      1) The group_idx[] array is effectively a group map (although we don't display the non-used locations). The array index is the group number (idx) so groups are displayed in ascending order
      2) Display format is "gGG, oSS mSS mSS mSS ..." where "g" indicates group number (idx), "o" indicates owner session, and "m" member session.  "om" indicates both
   */

      bool fFirstGroupFound = false;
      int numSplitGroups = 0;

      sprintf(&tmpstr[strlen(tmpstr)], "group info[0..%d] =", group_info_count-1);

      for (i=0; i<MAX_STREAM_GROUPS; i++) {

         if (group_idx[i]) {

            if (fFirstGroupFound) strcat(tmpstr, " |");
            else fFirstGroupFound = true;

            sprintf(&tmpstr[strlen(tmpstr)], " g%d \"%s\",", i, szGroupId[i]);  /* show group idx and name */

            bool fOwnerMemberFound = false, fMemberFound = false;

            for (j=0; j<MAX_GROUP_CONTRIBUTORS; j++) {

               HSESSION hSession = group_member_sessions[i][j];

               if (hSession) {  /* show session if attached to a group */

                  if (hSession & 0x10000) {
                     sprintf(&tmpstr[strlen(tmpstr)], " om%d", (hSession & ~0x10000)-1);  /* owner + member session */
                     fOwnerMemberFound = true;
                  }
                  else {
                     sprintf(&tmpstr[strlen(tmpstr)], " m%d", hSession-1);  /* member only */
                     fMemberFound = true;
                  }
               }
            }

            if (!fOwnerMemberFound && !fMemberFound) sprintf(&tmpstr[strlen(tmpstr)], " o%d", group_idx[i]-1);  /* session is group owner only without any members */
            if ((int)fOwnerMemberFound ^ (int)fMemberFound) numSplitGroups++;
         }
      }

      sprintf(&tmpstr[strlen(tmpstr)], "\n");
      sprintf(&tmpstr[strlen(tmpstr)], "num split groups = %d\n", numSplitGroups);
   }

   int nThreads = 0;
   for (i=0; i<MAX_PKTMEDIA_THREADS; i++) if (packet_media_thread_info[i].threadid) nThreads++;

   char tmpstr2[20] = "N/A", tmpstr3[20] = "N/A";

   if (lib_dbg_cfg.uEnableDataObjectStats) {
      sprintf(tmpstr2, "%u", min_free_session_handles);
      sprintf(tmpstr3, "%d", min_free_channel_handles);
   }

   sprintf(&tmpstr[strlen(tmpstr)], "system wide info: num p/m threads %d, max sessions %d, max groups %d, min free session/channel handles %s/%s, max bucket depth %u, max hash lookup %u \n", nThreads, max_sessions, max_groups, tmpstr2, tmpstr3, channel_max_bucket_depth, lookup_hash_max_loops);
   sprintf(&tmpstr[strlen(tmpstr)], "event log info: warnings = %u, errors = %u, critical errors = %u \n", __sync_fetch_and_add(&event_log_warnings, 0), __sync_fetch_and_add(&event_log_errors, 0), __sync_fetch_and_add(&event_log_critical_errors, 0));

   if (level > 0) {

      sprintf(&tmpstr[strlen(tmpstr)], "get_sh(i) 0..%d =", MAX_SESSIONS-1);
      for (i=0; i<MAX_SESSIONS; i++) sprintf(&tmpstr[strlen(tmpstr)], " %d", get_session_handle(hSessions_t, i, thread_index));
      sprintf(&tmpstr[strlen(tmpstr)], "\n");

      sprintf(&tmpstr[strlen(tmpstr)], "ch[0..%d] =", NCORECHAN-1);
      for (i=0; i<NCORECHAN; i++) if (DSGetDebugInfo(0, i, &session_id, &in_use) == 1) sprintf(&tmpstr[strlen(tmpstr)], " %d=%d,%d", i, session_id, in_use);
      sprintf(&tmpstr[strlen(tmpstr)], "\n");
   }

   if (uFlags & DS_DISPLAY_THREAD_DEBUG_INFO_SCREEN_OUTPUT) printf("%s", tmpstr);  /* use buffered I/O, JHB Jan2020 */
   if (uFlags & DS_DISPLAY_THREAD_DEBUG_INFO_EVENT_LOG_OUTPUT) Log_RT(4, tmpstr);

   if (run != 1) run = 1;  /* restore run var */
}


void ThreadAbort(int thread_index, char* errstr) {

int i;

   Log_RT(2, "CRITICAL, %s, unrecoverable error in packet/media thread %d, aborting\n", errstr, thread_index);

   if (run > 0) {
   
      for (i=0; i<nPktMediaThreads; i++) ThreadDebugOutput(NULL, -1, 0, i, DS_DISPLAY_THREAD_DEBUG_INFO_EVENT_LOG_OUTPUT);  /* show debug output for existing threads, JHB Sep2019 */
      run = -1;
   }
}
