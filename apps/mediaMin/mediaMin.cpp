/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/mediaMin.cpp

 Copyright (C) Signalogic Inc. 2018-2024

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  Application source code for packet + media processing, including:

   -telecom and analytics applications, e.g. SBC, lawful interception, ASR and transcription, call recording

   -standard operating mode

     -basic API interface to SigSRF pktlib, including packet push/pull queues, session create/delete and session get/set info
     -SigSRF pktlib packet/media thread usage, including multiple threads
     -static session creation based on session config files
     -dynamic session creation based on packet contents, supporting multistream pcaps and UDP flow ("dynamic sessions" mode)
     -support for pcaps with/without packet timestamps (wall clock packet arrival times)
     -SigSRF lib event logging, packet logging, packet time and loss stats

   -capacity measurement and test modes

     -multiple application threads, including all above SOP functionality per thread 
     -functional test
     -stress test

 Documentation

  https://www.github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediamin

  Older documentation links:
  
    after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

    before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

 Source Code Notes
 
  -mediaMin.cpp is primarily C code for high performance, but newer additions are C++ code
  -several additional cpp files are included in the build

 Revision History

   Created Jul 2018 JHB, see also revision history in mediaTest_proc.c and packet_flow_media_proc.c
   Modified Jul 2018 CKJ, add pcap file I/O for testing
   Modified Jul 2018 JHB, fix problem with screen counters, pause before exiting to allow packet/media thread time to clean up and generate packet stats log
   Modified Jul 2018 JHB, enable user managed sessions and stream merging
   Modified Aug 2018 JHB, add reference to debugMode ("Mode") to allow new testing options (start packet/media thread before/after creating sessions, delete sessions while thread continues to run, etc)
   Modified Aug 2018 JHB, correctly push/pulls packets for multiple session test cases
   Modified Aug 2018 CKJ, add stress test with dynamic session create/delete, support multistream EVS and AMR pcaps
   Modified Sep 2018 JHB, change FlushSession() to reflect new usage of DS_SESSION_INFO_STATE in DSSetSessionInfo()
   Modified Sep 2018 JHB, add support for multiple stream groups, see StreamGroupOutputSetup() and GetNextGroupSessionIndex()
   Modified Sep 2018 CKJ, add dynamic sessions mode
   Modified Sep 2018 JHB, organize cmd line entry used for operational modes, stress tests, and options/flags.  Include RTP payload type in key used for dynamic session detection.  Additional checks for codec type estimation in dynamic session detection
   Modified Sep 2018 JHB, add multiple mediaMin thread support.  A typical command line is ./mediaTest -M0 -cx86 -itest_files/file.pcap -L -dN -r0 -Et -t6, where -Et invokes mediaMin, -tN specifies number of concurrent threads, and -dN specifies operating mode and media flags depending on input type, sessions mode, etc (see flag definitions)
   Modified Oct 2019 JHB, add stop key, for clean exit in multithread tests
   Modified Oct 2018 JHB, add ANALYTICS_MODE flag, which enables pktlib analytics mode and modifies operation of PushPackets() and PullPackets() to push/pull packets at ptime intervals (e.g. 20 msec for EVS and AMR NB/WB).  The objective is output packets, including signal processing outputs such as merged audio, at regular 20 msec intervals that can be verified with Wireshark wall clock stats
   Modified Oct 2018 JHB, make use of numPkts parameter added to DSPullPackets()
   Modified Oct 2018 JHB, fix bug in PullPackets() where packet output to jitter buffer and transcoded output pcap files was mixed together
   Modified Oct 2018 JHB, disable retry-wait in PushPackets() if push queue is full, replace with stderr notice if log level >= 8
   Modified Oct 2018 JHB, added variable rate push algorithm in PushPackets(), based on transcoded output queue levels.  Look for "average_push_rate"
   Modified Oct 2018 JHB, additional -dN cmd line entry flags
   Modified Nov 2018 JHB, implement simulated live UDP port packet input, using packet arrival times (timestamps) in pcap file (see -dN cmd line flag USE_PACKET_ARRIVAL_TIMES)
   Modified Nov 2018 JHB, add support for termN output_buffer_interval
   Modified Nov 2018 JHB, fix detection of AMR-NB SID packet in detect_codec_type_and_bitrate()
   Modified Nov 2018 JHB, add retry for stream group queue packet pull, if a packet is not available on strict 20 msec interval timing, then wait 1 msec and retry a few times.  Only active if USE_PACKET_ARRIVAL_TIMES mode is in effect.  See USE_GROUP_PULL_RETRY definition
   Modified Jan 2019 JHB, add -n cmd line entry for input reuse, to increase number of sessions for stress and capacity testing
   Modified Feb 2019 JHB, set EVS decode output Fs to 8 kHz instead of 16 kHz if stream group merging is active.  This reduces decode time and eliminates sampling rate conversion for narrowband merging, increasing session capacity
   Modified Sep 2019 JHB, modify codec detection algorithm in detect_codec_type_and_bitrate() to handle additional EVS bitrates, AMR octet-aligned cases, and improve best guess for cases when different codecs have identical RTP payload lengths
   Modified Oct 2019 JHB, add auto-quit (active when all inputs are files), wait for p/m threads to close before exit, and apply STREAM_GROUP_DEBUG_STATS flag to termination endpoints to enable stats for overrun compensation (for per channel overrun, silence frame detection and compression is enabled by default; see shared_include/streamlib.h)
   Modified Oct 2019 JHB, add multiple stream groups in dynamic sessions mode, treating each cmd line input spec ("-ixx") as a multistream group.  For example, for two pcaps on the cmd line each would (i) be considered a multistream group and (ii) have its own stream group
   Modified Oct 2019 JHB, add COMBINE_INPUT_SPECS cmd line flag, which is similar to DYNAMIC_SESSIONS, but combines all cmd line input specs into one multistream group (and one stream group if applicable)
   Modified Nov 2019 JHB, added option to inject 1 sec timing markers in stream group audio output
   Modified Dec 2019 JHB, add -jN cmd line option for jitter buffer target and max delay values.  These can be set in session config files, but for dynamic sessions there was not previously a way to control this
   Modified Jan 2020 JHB, add -RN cmd line option for repeat operation. N = number of times to repeat the cmd line, -R0 repeats indefinitely, no entry doesn't repeat.  See nRepeatsRemaining and fRepeatIndefinitely vars.  The ENABLE_REPEAT Mode flag is no longer used
   Modified Jan 2020 JHB, implement per-session flush, session delete in last flush state (for dynamic sessions mode only). Upon deletion hSession[] handles are marked as deleted (set to -1), allowing their stats to stay available but preventing any further pktlib API usage
   Modified Jan 2020 JHB, PushPackets() now takes cur_time as a param instead of calling get_time().  This is a little faster but more importantly all input packet flows use the same reference when calculating packet timestamps vs. elapsed time. This fixes a slight variability seen in multiple input flow handling (for example with repeat enabled, stream group output FLCs might vary between repeats)
   Modified Jan 2020 JHB, add TERM_PKT_REPAIR_ENABLE and TERM_OVERRUN_SYNC_ENABLE flags to termN.uFlags during dynamic session creation
   Modified Feb 2020 JHB, make sure all sessions are fully deleted before exit or repeat. This is more efficient than sleeping some arbitary amount of time, and is also more reliable in the case of very long output wav files
   Modified Feb 2020 JHB, for real-time packet input (e.g. pcaps with packet timestamps, UDP input), move session flush to be immediately after input flow ends.  Session delete continues to take place after all queues are empty
   Modified Mar 2020 JHB, change DISABLE_SID_REPAIR flag to DISABLE_PACKET_REPAIR -- the flag, if included in -dN cmd line entry, now applies to both SID and media packet repair
   Modified Mar 2020 JHB, rework auto-adjust push rate algorithm and fix a problem it had when nReuseInputs is active. See comments in PushPackets() near AUTO_ADJUST_PUSH_RATE
   Modified Mar 2020 JHB, fix problem with "missed stream group interval" stats that were still accumulating after session flush
   Modified Mar 2020 JHB, add REPEAT_INPUTS flag to repeat input flows when applicable, for example wrapping pcap files back to start
   Modified Mar 2020 JHB, improve AppThreadSync() function to handle (i) waiting for master application thread and (ii) waiting for all threads
   Modified Mar 2020 JHB, rename file to mediaMin.c (from mediaTest_thread_app.c)
   Modified Mar 2020 JHB, update SetTimingInterval() to set termN.input_buffer_interval and termN.output_buffer_interval for both dynamic and static sessions
   Modified Mar 2020 JHB, modify PullPackets() to pull correct number of packets for telecom mode
   Modified Mar 2020 JHB, fix problem where SetSessionTiming() was being called after session creation, instead of before. SetSessionTiming() modifies session_data[] used during session creation
   Modified Apr 2020 JHB, rename ENABLE_FTRT_MODE flag to ANALYTICS_MODE, store stream group, event log, and packet log with _"am" suffix to make it easier in analyzing/comparing analytics vs. telecom mode output
   Modified Apr 2020 JHB, telecom mode updates:
                          -fix a few places where timing was incorrect; modified to look for combination of ((Mode & ANALYTICS_MODE) || term1.input_buffer_interval) to indicate "timed situations"
                          -set default jitter buffer max and target delay to 14 and 10
   Modified Apr 2020 JHB, clean up handling of DS_SESSION_INFO_DELETE_STATUS when exit or repeat
   Modified Apr 2020 JHB, app_printf() enhancements (in user_io.cpp)
   Modified May 2020 JHB, add handling for TERM_IGNORE_ARRIVAL_PACKET_TIMING and TERM_OOO_HOLDOFF_ENABLE flags
   Modified Jun 2020 JHB, fix bug where string size wasn't large enough to handle multiple session stats summary print out (just prior to program exit)
   Modified Jun 2020 JHB, move static session creation into CreateStaticSessions()
   Modified Jun 2020 JHB, add ENABLE_ALIGNMENT_MARKERS -dN cmd line option, to support visual inspection when deduplication algorithm is active (ENABLE_STREAM_GROUP_DEDUPLICATION flag)
   Modified Sep 2020 JHB, mods for compatibility with gcc 9.3.0, fix various security and "indentation" warnings
   Modified Oct 2020 JHB, tested with .pcapng input files after support added to pktlib for reading pcapng file format
   Modified Jan 2021 JHB, add cmd line SDP file parsing and management. See SDPParseInfo() in sdp_app.cpp and apps/common/sdp folder for source code
   Modified Jan 2021 JHB, continued ASR integration and testing
   Modified Jan 2021 JHB, convert to .cpp, include <algorithm> and use namespace std
   Modified Feb/Mar 2021 JHB, add DER encoded encapsulated stream handling, with full abstraction of DER encoded and aggregated packets
                              -first tested on HI3 content in OpenLI captures
                              -for API functionality and documentation see include/derlib.h, for implementation see lib/derlib
                              -for app usage see source within ENABLE_DER_STREAM_DECODE below
   Modified Mar 2021 JHB, modify SDP info to add from TCP/IP SIP invite packets, in addition to cmd line .sdp file. Multiple SDP info can be added at any time, in any sequence. See comments for SDPParseInfo() in sdp_app.cpp
   Modified Apr 2021 JHB, move related functions to separate files:
                          -SDPSetup(), SDPParseInfo(), and ProcessSessionControl() to sdp_app.cpp
                          -ReadSessionConfig(), CreateStaticSessions(), SetTimingInterval(), and GetSessionFlags() to session_app.cpp
                          -UpdateCounters(), ProcessKeys(), and app_printf() to user_io.cpp
   Modified May 2021 JHB, simplify DER encapsulated stream handling (around DSFindDerStream() and DSDecodeDerStream() ). Added comments
   Modified Dec 2021 JHB, add experimental handling of .ber files on command line. Note the file input type definition PCAP_TYPE_BER added in pktlib.h
   Modified Dec 2021 JHB, re-organize RTP packet validation in CreateDynamicSession() - blunt checks first with no warnings, increasing to precise checks that warn
   Modified Dec 2021 JHB, add handling for non-IP packets (e.g. ARP, 802.2 LLC frames) in PushPackets()
   Modified Dec 2021 JHB, add handling for IP non-RTP packets (e.g. ICMP, DHCP) in CreateDynamicSession(). Note a change was made in pktlib to handle ICMPv6
   Modified Dec 2021 JHB, implement debug flags ENABLE_DER_DECODING_STATS and ENABLE_INTERMEDIATE_PCAP
   Modified Jan 2022 JHB, testing with separate cmd_line_options_flags.h header file, included in mediaMin.h
   Modified Sep 2022 JHB, add support for dormant_SSRC_wait_time in TERMINATION_INFO structs, DISABLE_DORMANT_SESSION_DETECTION flag defined in cmd_line_options_flags.h
   Modified Sep 2022 JHB, add ReadCodecConfig() to handle special case codec debug/test (for both static and dynamic sessions). See comments
   Modified Dec 2022 JHB, add DISABLE_JITTER_BUFFER_OUTPUT_PCAPS flag and wav output seek time alarm, see comments near ENABLE_WAV_OUT_SEEK_TIME_ALARM 
   Modified Dec 2022 JHB, replace ancient event log config with new diaglib APIs DSInitLogging() and DSCloseLogging(). Also see example of using DSUpdateLogConfig()
   Modified Dec 2022 JHB, call DSConfigMediaService() with DS_CONFIG_MEDIASERVICE_EXIT flag at in cleanup/exit
   Modified Jan 2023 JHB, improve codec auto-detection for ocet-aligned AMR-NB and AMR-WB
   Modified Jan 2023 JHB, filter non-dynamic UDP ports and media port allow list. See NON_DYNAMIC_UDP_PORT_RANGE, UDP_Port_Media_Allow_List[], and FILTER_UDP_PACKETS
   Modified Jan 2023 JHB, fix issue where 'q' key during post-processing (e.g. packet log collate and analysis) caused a hang. See call to DSConfigLogging()
   Modified Jan 2023 JHB, in PushPackets() use DS_PKT_INFO_PKTINFO flag in DSGetPacketInfo() to minimize packet handling overhead
   Modified Jan 2023 JHB, add support for SAP/SDP protocol
   Modified Jan 2023 JHB, add rudimentary support for SIP message session control. Look for SESSION_CONTROL_FOUND_SIP_xxx flags. The SIP BYE message will terminate a stream (this can be disabled by the DISABLE_TERMINATE_STREAM_ON_BYE flag in -dN cmd line options)
   Modified Mar 2023 JHB, handle SIP messages when source port is 5060 (show but not parse)
   Modified Apr 2023 JHB, in TestActions() auto-quit handling, check for all streams terminated
   Modified Apr 2023 JHB, implement filtering of redundant TCP retransmissions
   Modified Apr 2023 JHB, add num TCP and UDP packets to mediaMin stats summary
   Modified May 2023 JHB, add support for AFAP and FTRT modes ("as fast as possible" and "faster than real-time"). Look for comments near isAFAPMode and isFTRTMode (defined in mediaMin.h)
   Modified May 2023 JHB, RealTimeInterval[] now handled as float, as part of AFAP and FTRT mode support. extern reference to timeScale for accelerated time in FTRT mode
   Modified May 2023 JHB, support port list on cmd line (-pN entry), search through uPortList[] in isPortAllowed()
   Modified May 2023 JHB, modify CreateDynamicSession() to support RFC7198 lookback depth on cmd line (-lN entry, uLookbackDepth value set by cmdLineInterface() in cmd_line_interface.c). Default of no entry is 1 packet lookback, zero disables (-l0 entry), max is 8 packet lookback
   Modified Jun 2023 JHB, codec auto-detection updates: add AMR-WB 14250, 18250, and 19850 bps, in cat 4 detection fix AMR-NB SIDs wrongly detected as EVS
   Modified Jul 2023 JHB, handle cmd line option to reflect input pauses in wav outputs (INCLUDE_PAUSES_IN_WAV_OUTPUT flag)
   Modified Jul 2023 JHB, give program version info to cmdLineInterface()
   Modified Aug 2023 JHB, implement timestamp-matched wav output (specified on cmd line with ENABLE_WAV_OUTPUT_TIMESTAMP_MATCH flag). Look for TIMESTAMP_MATCH_MODE_XXX flag usage (defined in shared_include/streamlib.h)
   Modified Aug 2023 JHB, implement SHOW_PACKET_ARRIVAL_STATS flag. This includes packet arrival info in mediaMin stats display
   Modified Oct 2023 JHB, in detect_codec_type_and_bitrate() resolve framesize collisions between EVS primary mode compact header format and EVS AMR-WB IO mode header-full format
   Modified Nov 2023 JHB, update auto-detect for EVS VBR mode 5900 bps (which includes 7200, 8000, 8850, 9600 bps), and additional AMR-WB IO mode bitrates. These are needed to handle output of pcaps generated by mediaTest for codec testing purposes
   Modified Nov 2023 JHB, add const char* banner_info param to cmdLineInterface, implement version_info and banner_info to be uniform with mediaTest. Implement --cut N cmd line option. First pass of this cuts N leading packets from data flow
   Modified Nov 2023 JHB, implement PCAP_TYPE_RTP to handle .rtpdump (and .rtp) format inputs. Heavy lifting is done in pktlib DSOpenPcap() and DSReadPcap(). See the modified pcap_hdr_t struct in pktlib.h
   Modified Nov 2023 JHB, fix 16 kHz output option for stream group output wav files (look for FORCE_EVS_16KHZ_OUTPUT). Note this does not affect ASR
   Modified Dec 2023 JHB, implement --group_pcap cmd line option for user-specified stream group pcap output path
   Modified Feb 2024 JHB, use DS_VOICE_CODEC_TYPE_L16 (linear PCM 16-bit) for term 2 codec type in timestamp matching mode. This makes transcoded output sent from packet/media threads more useful for Wireshark and other debug
   Modified Feb 2024 JHB, fix bug in calculation of stream group sampling rate when FORCE_EVS_16KHZ_OUTPUT is not defined
   Modified Feb 2024 JHB, modify media and processing time output to use hours:min:sec format, show both processing and media time in FTRT mode
   Modified Mar 2024 JHB, use fabsf() instead of abs() for float-point average jitter calculation. During tools/system regression test only gcc 5.x has a complaint about this, but doesn't matter for output results, so we can leave it this way
   Modified Apr 2024 JHB, remove DS_CP_DEBUGCONFIG flag, which is now deprecated
   Modified May 2024 JHB, add RTP auto-detection for stereo EVS 5900 (handle Fraunhofer example 5900 bps .rtp files which include some stereo)
   Modified May 2024 JHB, update comments that reference x86_mediaTest to mediaTest_proc
   Modified May 2024 JHB, for .rtp format file support, save .rtp file headers in thread_info pcap_file_hdr[], then later give to DSReadPcap() calls
   Modified May 2024 JHB, in detect_codec_type_and_bitrate() auto-detect AMR 5900 and AMR-WB 6600
   Modified Jun 2024 JHB, look for additional protocols in isPortAllowed() and isReservedUDP() (GPRS tunnelling, DNS, pichat)
   Modified Jun 2024 JHB, implement DISABLE_PORT_IGNORE_MESSAGES and DISABLE_SIP_INFO_REQUEST_OK_MESSAGES flags, fFirstConsoleMediaOutput. Modify calls to ProcessSessionControl() (in sdp_app.cpp) to use these flags, plus new flags in sdp_app.h to more precisely control SDP/SAP message parsing and display
   Modified Jun 2024 JHB, rename DSReadPcapRecord() to DSReadPcap() and DSWritePcapRecord() to DSWritePcap(), per change in pktlib.h
   Modified Jun 2024 JHB, add duplicated UDP packet detection. See isDuplicatePacket()
   Modified Jun 2024 JHB, add run-time stats for UDP duplicated, fragmented, and encapsulated packets. Add packet_number running stat that exactly matches packet number column in Wireshark, and start including this in info and debug messages
   Modified Jun 2024 JHB, handle SDP info media descriptions, including port discovery and exceptions
   Modified Jun 2024 JHB, improved codec auto-detection for AMR low-end bitrates (4750, 5150, 5900 bps)
   Modified Jun 2024 JHB, add H.265 codec detection and dynamic session creation
   Modified Jun 2024 JHB, implement packet fragmentation and reassembly as pktlib DSGetPacketInfo() now API supports this. Look for DS_INFO_PKT_FRAGMENT_xxx, DS_INFO_PKT_REASSEMBLY_xxx, and DS_INFO_PKT_RETURN_xxx uFlags
   Modified Jun 2024 JHB, fix problems with SIP port range handling, update isPortAllowed(). Look for PORT_ALLOW_xxx definitions
   Modified Jul 2024 JHB, set DS_PKTSTATS_ORGANIZE_COMBINE_SSRC_CHNUM packet logging flag in DSWritePacketStatsHistoryLog() uFlags if DISABLE_DORMANT_SESSION_DETECTION cmd line option is set (see diaglib.h for flag info)
*/

/* Linux header files */

#include <stdio.h>
#include <pthread.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <signal.h>
#include <assert.h>

#include <algorithm>  /* bring in std::min and std::max */
#include <fstream>

using namespace std;

/* mediaTest header file */

#include "mediaTest.h"  /* bring in vars declared in cmd_line_interface.c, including MediaParams[], PlatformParams, RealTimeInterval[], and debugMode (defined as Mode in mediaMin.h) */

/* lib header files */

#include "pktlib.h"     /* packet push/pull and session management APIs. Pktlib includes packet/media threads, packet handling and formatting, jitter buffers, session handling */
#include "voplib.h"     /* voplib provides an API interface to all codecs. Normally this is used by pktlib but can be accessed directly if needed */
#include "diaglib.h"    /* diagnostics including event and packet logging. Event logging includes default stats and optional stats depending on cmd line entry. Packet logging includes detailed packet stats */
#include "derlib.h"

#include "shared_include/session.h"    /* session management structs and definitions */
#include "shared_include/config.h"     /* configuration structs and definitions */
#include "shared_include/streamlib.h"  /* streamlib provides an API interface for stream group management. Normally this is used by pktlib but can be accessed directly if needed */

#include <sdp/sdp.h>  /* SDP API header file */

/* app level header files */

#include "mediaMin.h"     /* struct typedefs and other definitions (also includes cmd_line_options_flags.h) */
#include "cmdLineOpt.h"   /* cmd line handling */
#include "sdp_app.h"      /* app level SDP management */
#include "session_app.h"  /* app level session management */
#include "user_io.h"      /* user I/O (keybd, counters and other output) */

/* number of possible input streams, including streams that are re-used for multithread and high capacity testing */
//#define MAX_INPUT_STREAMS MAX_SESSIONS  /* MAX_SESSIONS is defined in transcoding.h, which is included in mediaTest.h */

//#define LOG_OUTPUT  LOG_SCREEN_ONLY  /* screen output only */
//#define LOG_OUTPUT  LOG_FILE_ONLY    /* file output only */
#define LOG_OUTPUT  LOG_SCREEN_FILE    /* screen + file output (LOG_SCREEN_FILE defined in diaglib.h) */

#define USE_GROUP_PULL_RETRY

#define NON_DYNAMIC_UDP_PORT_RANGE  4096  /* non-dynamic UDP port range. Change this if less or more UDP ports should be ignored. See FILTER_UDP_PACKETS below, JHB Jan 2023 */

PORT_INFO_LIST UDP_Port_Media_Allow_List[] = { 1234, 3078, 3079 };  /* add exceptions here for UDP ports that should be allowed for RTP media (and are not expressed in input stream SDP info). Currently the list has some arbitrary ports found in a few legacy test pcaps used in mediaMin regression test. Ports can also be added at run-time using -p cmd line entry */

static char prog_str[] = "mediaMin";
#ifdef _MEDIAMIN_
static char banner_str[] = "packet media streaming for analytics and telecom applications on x86 and coCPU platforms";
#endif
static char version_str[] = "v3.6.1";
static char copyright_str[] = "Copyright (C) Signalogic 2018-2024";

//#define VALGRIND_DEBUG  /* enable when using Valgrind for debug */
#ifdef VALGRIND_DEBUG
#define VALGRIND_DELAY 100  /* usleep delay value in usec for allowing valgrind to run multithreaded apps on the same core */
#endif

/* vars shared between app threads */

HPLATFORM     hPlatform = -1;           /* initialized by DSAssignPlatform() API in DirectCore lib */
static int    debug_test_state;
static bool   fThreadSync1 = false;     /* flag used to coordinate app threads during first stage of initialization */
static bool   fThreadSync2 = false;     /* same, for second stage of initialization */
bool          fQuit = false;            /* set if 'q' (quit) key is pressed */
bool          fPause = false;           /* "" 'p' (pause).  Pauses operation, another 'p' resumes.  Can be combined with 'd' (display) key to read out internal p/m thread debug, capacity, stats, and other info */ 
bool          fStop = false;            /* "" 's' (stop).  Stop prior to next repeat (only applies if -RN is entered on cmd line.  Intended for clean stop to repeating test, avoiding partial output files, especially when ENABLE_RANDOM_WAIT is active */
unsigned int  num_app_threads = 1;      /* set to more than one if multiple mediaMin app threads are active. This is controlled by a mediaTest cmd line that includes "-Et -tn" options, where n is the number of app threads (see SigSRF documentation) */
int           num_pktmed_threads = 0;   /* number of packet/media threads running */
static int    log_level = 0;            /* set in LoggingSetup() */
bool          fStressTest;              /* determined from cmd line options, number of app threads, and session re-use */
static char   szSessionName[MAX_INPUT_STREAMS][384] = {{ "" }};  /* set in LoggingSetup() which should always be called */
static bool   fInputsAllFinite = true;  /* set to false if inputs include UDP port or USB audio.  Default is true if all inputs are pcap or other file */
static bool   fAutoQuit = false;        /* fAutoQuit determines whether program stops automatically. This is the default for cmd lines with (i) all inputs are files (i.e. no UDP or USB audio inputs) and (ii) no repeating stress or capacity tests */
bool          fRepeatIndefinitely = false;  /* true if -R0 is given on the cmd line */
bool          fNChannelWavOutput = false;  /* true if stream group N-channel wav output enabled; N-channel means a wav file with 2 or more channels (i.e. as many as needed for the stream group */
bool          fUntimedMode = false;     /* true if neither ANALYTICS_MODE nor USE_PACKET_ARRIVAL_TIMES (telecom mode) flags are set in -dN options. This is the case with some old test scripts with -r0 push-pull rate (as fast as possible). Without analytics or telecom mode specified we call it "untimed mode" and leave it available for legacy testing, but it's not recommended for normal use */
const char tabstr[] = "    ";           /* used for stats formatting */

codec_test_params_t codec_config_params = { 0 };  /* added to support non-standard codec configurations. Notes: use -C cmd line option to specify a codec config file name, (ii) works for both dynamic and static sessions, (iii) not sure yet if this should be made per-thread or per-session; currently it's a test/debug option that applies to all mediaMin threads, JHB Sep 2022 */

/* arrival timing stats. To-do: add these to THREAD_INFO struct definition in mediaMin.h */

static float arrival_avg_delta[10] = { 0 }, arrival_avg_jitter[10] = { 0 }, arrival_max_delta[10] = { 0 }, arrival_max_jitter[10] = { 0 }, last_msec_timestamp[10] = { 0 };
static uint32_t num_arrival_stats_pkts[10] = { 0 };
static int last_rtp_pyld_len[10] = { 0 };

bool fFirstConsoleMediaOutput = false;  /* set on first media-related console output; used to help manage console output about non-RTP related protocols and messages, JHB Jun 2024 */


/* per application thread info */

APP_THREAD_INFO thread_info[MAX_APP_THREADS] = {{ 0 }};  /* THREAD_INFO struct defined in mediaMin.h, MAX_APP_THREADS defined in mediaTest.h */
static int average_push_rate[MAX_APP_THREADS] = { 0 };
int nRepeatsRemaining[MAX_APP_THREADS] = { 0 };

/* misc local definitions (most definitions are in mediaTest.h and mediaMin.h) */

#define TIMER_INTERVAL                     1   /* timer value in seconds for CREATE_DELETE_TEST_PCAP test mode */
#define WAIT_FOR_MASTER_THREAD             1   /* mode values used in AppThreadSync() local function */
#define WAIT_FOR_ALL_THREADS               2

/* below are local functions inside mediaMin.cpp. Others are in app level source files (e.g. session_app.cpp, sdp_app.cpp, etc). SigSRF lib APIs have "DS" prefix, local functions do not */

/* logging and configuration setup */

#if 0  /* these are now in cmd_line_interface.c, JHB Dec 2022 */
#if (LOG_OUTPUT != LOG_SCREEN_ONLY)
FILE* fp_sig_lib_event_log = NULL;
char sig_lib_event_log_filename[] = { "sig_lib_event_log.txt" };  /* default event log filename */
#endif
#endif

#define LOG_EVENT_SETUP        1
#define LOG_PACKETSTATS_SETUP  2

void LoggingSetup(DEBUG_CONFIG*, int setup_type);  /* logging setup */
void GlobalConfig(GLOBAL_CONFIG* gbl_cfg);         /* lib config */
void DebugSetup(DEBUG_CONFIG* dbg_cfg);            /* lib debug config */

/* I/O setup */

void InputSetup(int thread_index);
void JitterBufferOutputSetup(int thread_index);
void TranscodedOutputSetup(int thread_index);
void StreamGroupOutputSetup(HSESSION hSessions[], int nInput, int thread_index);
void MiscOutputSetup(int thread_index);

/* application thread helper functions */

void ThreadWait(int when, int thread_index);
void AppThreadSync(unsigned int, bool* fThreadSync, int thread_index);
void PmThreadSync(int thread_index);

/* wrapper functions for pktlib DSPushPackets() and DSPullPackets(), including pcap read/write, queue full check, etc */
  
int PushPackets(uint8_t* pkt_in_buf, HSESSION hSessions[], SESSION_DATA session_data[], int nSessions, uint64_t cur_time, int thread_index);
int PullPackets(uint8_t* pkt_out_buf, HSESSION hSessions[], SESSION_DATA session_data[], unsigned int uFlags, unsigned int pkt_buf_len, int thread_index);
void FlushCheck(HSESSION hSessions[], uint64_t cur_time, uint64_t (*queue_check_time)[MAX_SESSIONS], int thread_index);
bool isDuplicatePacket(PKTINFO* PktInfo, PKTINFO* PktInfo2, int nInput, int thread_index);

/* create and manage packet/media threads */

int StartPacketMediaThreads(int num_pm_threads, int thread_index);

/* helper functions for creating and managing sessions. See also functions in session_app.cpp */

void ResetDynamicSession_info(int);
int CreateDynamicSession(uint8_t *pkt, int pkt_len, HSESSION hSessions[], SESSION_DATA session_data[], int thread_index, int nInput, int nReuse);

/* stress test helper functions */

int TestActions(HSESSION hSessions[], int thread_index);
void handler(int signo);
void TimerSetup();

/* misc helpers */

int packet_actions(uint8_t* pyld_data, uint8_t* pkt_buf, uint8_t protocol, int* pkt_len, unsigned int uFlags);
int isPortAllowed(uint16_t port, uint8_t* pkt_buf, int pkt_len, int nInput, int thread_index);
bool isReservedUDP(uint16_t port);
void cmdLine(int argc, char** argv, char* tmpstr);
void GetMD5Sum(char* tmpstr, int thread_index);
int isPacketFragment(uint8_t* pkt_buf, PKTINFO* PktInfo, int pkt_len, int nInput, int _thread_index);

/* mediaMin application entry point. Program and multithreading notes:

   -one mediaMin application thread is active if mediaMin is run from the cmd line. This includes standard operating mode for reference apps (SBC, lawful interception, call recording, ASR, RTP malware detection, etc)

   -multiple mediaMin application threads may be active if invoked from the mediaTest cmd line, using the -Et and -tN arguments. This is the case for high capacity operation and stress tests

   -in either case, the first mediaMin application thread is the master app thread:
     -the master thread handles initialization, housekeeping, and exit cleanup
     -in addition the master thread manages one or more packet/media threads, starting p/m threads depending on need (determined from cmd line entry)
     -in the case of multiple mediaMin threads, the var "thread_index" indicates the current app thread (thread_index = 0 for app thread 0, 1 for app thread 1, etc)

   -application threads are separate from packet/media threads; these should not be confused. Packet/media threads run in the pktlib shared library. The High Capacity Operation section SigSRF documentation includes htop screen caps showing both application and packet/media threads, and notes about CPU core usage, thread affinity, and other multithreading issues 

   -mediaMin and mediaTest have the same command line format. Key differences include (i) mediaMin recognizes many more -dN options for operating modes and flags, (ii) handling of I/O (different sets of input file types), and (iii) mediaMin ignores -Ex and -tN entry (used only by mediaTest)
*/

#ifdef _MEDIAMIN_  /* _MEDIAMIN_ is defined in the mediaMin Makefile. This is true when mediaMin is run from the command line, in which case only one mediaMin application thread is active */
int main(int argc, char **argv) {
#else  /* _MEDIAMIN_ is not defined when mediaTest is run from the command line with the -Et and -tN options, in which case (i) mediaTest is the process, (ii) the entry point here is mediaMin_thread() instead of main(), and (iii) mediaTest starts one or more mediaMin application threads with mediaMin_thread() as the callable function entry point */
void* mediaMin_thread(void* thread_arg) {  /* see the "executionMode[0]" switch statement in mediaTest_proc.c.  In that switch statement, the 't' case arrives here. See also console message " ... start sequence = ..." when mediaMin runs, which shows the call stack for packet/media worker threads */
#endif

HSESSION       hSessions[MAX_SESSIONS] = { 0 };
SESSION_DATA   session_data[MAX_SESSIONS] = {{ 0 }};

unsigned char  pkt_in_buf[32*MAX_RTP_PACKET_LEN] = { 0 }, pkt_out_buf[32*MAX_RTP_PACKET_LEN] = { 0 };

DEBUG_CONFIG dbg_cfg = { 0 };  /* structs used for lib debug configuration; see shared_include/config.h */
GLOBAL_CONFIG gbl_cfg = { 0 };

int i, j, nStaticSessionsConfigured = 0, nRemainingToDelete = 0, thread_index = 0;  /* mediaMin application thread index (normally zero, except for high capacity and stress test situations, see definitions and comments in cmd_line_options_flags.h) */

unsigned long long cur_time = 0, base_time = 0;
uint64_t interval_count = 0, queue_check_time[MAX_SESSIONS] = { 0 };
bool fExitErrorCond, fRepeatFromStart = false;  /* fRepeatFromStart is set true "start" or "session_create" labels are used. This happens if -RN cmd line entry is given (look for "nRepeat") or certain stress test types are specified */
char tmpstr[MAX_APP_STR_LEN];  /* large temporary string used for various purposes */

   #ifdef _MEDIAMIN_  /* main() runs as a process from a mediaMin command line */

   cmdLine(argc, argv, tmpstr);  /* handle command line, version info, etc */

   printf("mediaMin start, cmd line execution\n");

   #else  /* mediaMin_thread() runs as either (i) a function call or (ii) one or more threads created by mediaTest; both are initiated by a mediaTest command line */

   thread_index = *((int*)thread_arg) & 0xff;
   num_app_threads = (*((int*)thread_arg) & 0xff00) >> 8;

   if (num_app_threads) {  /* mediaMin is running as one or more application threads */

      printf("mediaMin start, thread execution, num threads = %d, thread_index = %d\n", num_app_threads, thread_index);
      free(thread_arg);
   }
   else {  /* mediaMin is running as function call */

      printf("mediaMin start, function call execution\n");
      num_app_threads = 1;
   }

   #endif  /* #ifdef _MEDIAMIN_ */

   if (Mode == -1) Mode = 0;  /* default value if no cmd line entry given is -1 (Mode is defined from "debugMode" in mediaTest.h. debugMode is set in cmd_line_interface.c from the command line by -d argument) */

   if (nRepeat == 0) fRepeatIndefinitely = true;  /* nRepeat is initialized in cmd_line_interface.c from -RN cmd line entry (if no entry nRepeat = -1). Note that some stress tests already have repeat built in, so -RN entry may be ignored or treated differently in those cases */
   nRepeatsRemaining[thread_index] = nRepeat;  /* each app thread keeps an independent repeat count, as they may repeat at different times (for example if ENABLE_RANDOM_WAIT is set) */

   if (isMasterThread) {  /* isMasterThread defined in mediaMin.h */

      printf(" Standard Operating Mode\n");
      if (Mode & DYNAMIC_SESSIONS)         printf("  dynamic sessions created as they appear in stream input\n");
      else                                 printf("  static sessions created from session config file (specified with -C on cmd line)\n");
      if (Mode & COMBINE_INPUT_SPECS)      printf("  combine all input specs into one stream (and stream group if enabled)\n");
      else                                 printf("  each input may contain one or more streams (each input is a \"stream group\")\n");
      if (Mode & ENABLE_DER_STREAM_DECODE) printf("  DER encapsulated stream detection and decoding enabled\n");
      if (Mode & ENABLE_STREAM_GROUP_ASR)  printf("  ASR enabled for stream group output\n");

      printf(" Test Modes\n");
      bool fTestModePrinted = false;
      if (Mode & CREATE_DELETE_TEST) { printf("  test mode, create, delete, and recreate sessions. Automatically repeats\n"); fTestModePrinted = true; }
      if (Mode & CREATE_DELETE_TEST_PCAP) { printf("  test mode, dynamically create sessions from pcap with initial static session. Automatically repeats\n"); fTestModePrinted = true; }
      char repeatstr[20];
      sprintf(repeatstr, "%d times", nRepeat);
      if (nRepeat >= 0) { printf("  repeat %s\n", nRepeat == 0 ? "indefinitely" : repeatstr); fTestModePrinted = true; }
      if (Mode & ENABLE_RANDOM_WAIT) { printf("  random wait at start and between repeats enabled\n"); fTestModePrinted = true; }
      if (Mode & START_THREADS_FIRST) { printf("  start packet / media threads first\n"); fTestModePrinted = true; }
      if (Mode & ENERGY_SAVER_TEST) { printf("  initial 30+ sec delay enabled to test packet/media thread energy saver mode\n"); fTestModePrinted = true; }
      if (!fTestModePrinted) printf("  none\n");

      printf(" Options\n");
      if (Mode & ENABLE_STREAM_GROUPS) printf("  stream group(s) enabled\n");
      if (Mode & ENABLE_WAV_OUTPUT) printf("  wav file output for stream group(s) enabled\n");

      if (Mode & ANALYTICS_MODE) printf("  Analytics mode with -rN nominal rate enabled\n");
      else if (Mode & USE_PACKET_ARRIVAL_TIMES) printf("  Telecom mode with -rN nominal rate enabled\n");
      else { printf("  Untimed mode with -rN nominal rate enabled\n"); fUntimedMode = true; } /* added this for legacy test command lines with no -dN options and -r0 push-pull rate (0 = as fast as possible). Not recommended for normal operation, JHB Jan 2023 */

      if (Mode & AUTO_ADJUST_PUSH_RATE)  printf("  auto-adjust packet push rate\n");  /* note - both auto-adjust and use-packet-arrival-times flags set is undefined behavior */
      else if (Mode & USE_PACKET_ARRIVAL_TIMES) printf("  packet arrival times control packet push rate\n");
      else                                      printf("  packet push rate method not defined\n");

      if (Mode & DISABLE_DTX_HANDLING) printf("  DTX handling disabled\n");
      if (Mode & DISABLE_FLC) printf("  FLC (frame loss concealment) on stream group output disabled\n");
      if (Mode & ENABLE_FLC_HOLDOFFS) printf("  FLC Holdoffs for stream group output enabled\n");
//         if (Mode & DISABLE_OUTPUT_BUF_INTERVAL) printf("  output buffer interval field in termN definitions is disabled\n");
      if (Mode & ENABLE_ONHOLD_FLUSH_DETECT) printf("  on-hold flush detection for audio merge contributors enabled (this is deprecated)\n");
      if (Mode & ENABLE_TIMING_MARKERS) printf("  timing markers injected every 1 sec into stream group audio output\n");
      if (Mode & ENABLE_PACKET_INPUT_ALARM) printf("  alarm for input packet enabled, if DSPushPackets() is not called for the alarm time limit a wàrning will show in the event log\n");
      if (Mode & ENABLE_WAV_OUT_SEEK_TIME_ALARM) printf("  alarm for wav output file seek time enabled, streamlib will show wàrnings if wav output file writes exceed time threshold\n");

      if (Mode & DISABLE_AUTOQUIT) printf("  auto-quit disabled\n");
      if (Mode & DISABLE_DORMANT_SESSION_DETECTION) printf("  dormant session detection disabled\n");
      if (Mode & DISABLE_JITTER_BUFFER_OUTPUT_PCAPS) printf("  jitter buffer output pcaps disabled\n");
      if (Mode & ENABLE_STREAM_SDP_INFO) printf("  SDP in-stream info enabled\n");
      if (Mode & DISABLE_TERMINATE_STREAM_ON_BYE) printf("  SIP BYE message stream termination disabled\n");

      if (Mode & ENABLE_DEBUG_STATS) printf("  debug info and stats enabled\n");
      if (Mode & ENABLE_DER_DECODING_STATS) printf("  DER decoding stats enabled\n");
      if (Mode & ENABLE_INTERMEDIATE_PCAP) printf("  HI2 / HI3 / BER intermediate pcap output enabled\n");
      if (Mode & ENABLE_ASN_OUTPUT) printf("  ASN intermediate output enabled\n");
      if (Mode & DISABLE_PORT_IGNORE_MESSAGES) printf("  after first media disable some non-RTP port messages\n");
      if (Mode & DISABLE_SIP_INFO_REQUEST_OK_MESSAGES) printf("  after first media disable SIP info request and Ok messages\n");
      if (Mode & INCLUDE_PAUSES_IN_WAV_OUTPUT) printf("  pauses in stream input are reflected in wav output as \"silence zeros\" (e.g. call-on-hold)\n");

      if (Mode & ENABLE_WAV_OUTPUT_TIMESTAMP_MATCH) {

         uTimestampMatchMode = TIMESTAMP_MATCH_MODE_ENABLE;  /* set timestamp matching mode flags */

         if (!(Mode & ENABLE_STREAM_GROUPS)) uTimestampMatchMode |= TIMESTAMP_MATCH_DISABLE_FLUSH | TIMESTAMP_MATCH_DISABLE_RESYNCS;
         if (Mode & INCLUDE_PAUSES_IN_WAV_OUTPUT) uTimestampMatchMode |= TIMESTAMP_MATCH_INCLUDE_INPUT_PAUSES;
         if (Mode & ENABLE_TIMESTAMP_MATCH_LIVE_MERGE) uTimestampMatchMode |= TIMESTAMP_MATCH_LIVE_MERGE_OUTPUT;
         printf("  timestamp-matched wav output enabled\n");
      }

      if (Mode & SHOW_PACKET_ARRIVAL_STATS) printf("  show packet arrival stats\n");
   }

   if (Mode & DYNAMIC_SESSIONS) thread_info[thread_index].fDynamicSessions = true;  /* currently set for all app threads. To-Do: allow mix of static and dynamic sessions between threads, JHB Jan 2023 */

   AppThreadSync(WAIT_FOR_MASTER_THREAD, &fThreadSync1, thread_index);  /* app threads wait here for master thread to do following first stage initialization */

   if (isMasterThread) {

      fStressTest = (Mode & CREATE_DELETE_TEST) || (Mode & CREATE_DELETE_TEST_PCAP);  /* set fStressTest if stress test options have been given */
      fCapacityTest = num_app_threads > 1 || nReuseInputs;  /* set fCapacityTest if load/capacity options have been given */

      fAutoQuit = !(Mode & DISABLE_AUTOQUIT) && !fStressTest && !fRepeatIndefinitely && fInputsAllFinite;

   /* set up timer for session create/delete/repeat test mode (only needed for stress tests) */

      if (Mode & CREATE_DELETE_TEST_PCAP) TimerSetup();

   /* logging, library init, and config */

      LoggingSetup(&dbg_cfg, LOG_EVENT_SETUP);  /* set up library event logging, dbg_cfg struct is defined in shared_include/config.h */

      LoggingSetup(&dbg_cfg, LOG_PACKETSTATS_SETUP);  /* set up packet stats logging */

      GlobalConfig(&gbl_cfg);  /* configure libraries, gbl_cfg struct is defined in shared_include/config.h */

      DebugSetup(&dbg_cfg);  /* set up debug items (see cmd line debug flag definitions and comments in cmd_line_options_flags.h) */

      DSInitLogging(&dbg_cfg, 0);  /* initialize event logging. Calls to Log_RT() are now active. Note that DSInitLogging() should not be called twice unless it has a matching DSCloseLogging() call, as it increments a semaphore count to track multithread usage. DSInitLogging() is in diaglib, JHB Dec 2022 */

   /* make an initial event log entry */

      Log_RT(4 | DS_LOG_LEVEL_FILE_ONLY, "%s, %s, %s \n", prog_str, version_str, copyright_str);  /* include (i) program info in event log (we already printed it above onscreen) and (ii) full command line */
      {
         char tmpstr[MAX_CMDLINE_STR_LEN];
         if (strlen(szAppFullCmdLine)) strcpy(tmpstr, szAppFullCmdLine);  /* full command line, saved by cmdLineInterface() (which calls GetCommandLine()) */
         else sprintf(tmpstr, "0x%llx", (unsigned long long int)Mode);
         Log_RT(4, "mediaMin INFO: event log setup complete, log file %s, log level %d, %s %s ", dbg_cfg.szEventLogFilePath, dbg_cfg.uLogLevel, strlen(szAppFullCmdLine) ? "cmd line" : "-dN cmd line options", tmpstr);
      }

   /* get a platform handle from DirectCore lib. Notes:

      -DirectCore detects and manages VM flags (e.g. rtdscp support), number of cores, interprocess concurrency, containers. Maintains shared mem files (/dev/shm/hwlib*) that are shared between processes
      -if PlatformParams is not filled in (mediaMin and mediaTest apps do this in cmd_line_interface.c), the platform designator default is "x86"
      -only one app thread should call DSAssignPlatform(). mediaMin designates the first app thread as "master" for housekeeping purposes. The isMasterThread macro returns this thread
 */

      hPlatform = DSAssignPlatform(NULL, PlatformParams.szPlatformDesignator, 0, 0, 0);

   /* init and configure pktlib */

      DSConfigPktlib(&gbl_cfg, &dbg_cfg, DS_CP_INIT);

      #if 0  /* example of dynamic event log modification (for example enabling and disabling event log, changing timestamp formats, etc), should it be needed. Notes:

                -the local DEBUG_CONFIG struct must be maintained in order to allow dynamic updating without loss of prior settings as diaglib does not maintain per-app copies
                -same is true for calling DSConfigPktlib() dynamically - pktlib does not maintain per-app copies of GLOBAL_CONFIG and DEBUG_CONFIG structs
                -DSInitLogging() should be called only once, as it increments a semaphore count to track multi-app usage
             */

      dbg_cfg.uEventLogMode |= DS_EVENT_LOG_DISABLE;
      DSUpdateLogConfig(&dbg_cfg, 0);  /* disable */

      dbg_cfg.uEventLogMode &= ~DS_EVENT_LOG_DISABLE;
      DSUpdateLogConfig(&dbg_cfg, 0);  /* re-enable */
      #endif

   /* init configure voplib and streamlib */

      DSConfigVoplib(NULL, &dbg_cfg, DS_CV_INIT);

      DSConfigStreamlib(NULL, &dbg_cfg, DS_CS_INIT);

   /* init and configure derlib if DER stream decoding specified in the cmd line */

      if (Mode & ENABLE_DER_STREAM_DECODE) DSConfigDerlib(NULL, NULL, DS_CD_INIT);

   /* start packet / media thread(s) */

      if (Mode & START_THREADS_FIRST) if (StartPacketMediaThreads(num_app_threads > 1 ? NUM_PKTMEDIA_THREADS : 1, thread_index) < 0) goto cleanup;

      fThreadSync1 = true;  /* release any app threads waiting in AppThreadSync() */

   }  /* end of master thread section */

/* first stage initialization complete */

start:  /* note - label used only if test mode repeats are enabled */

/* session configuration and packet I/O init */

   if (thread_info[thread_index].fDynamicSessions) nStaticSessionsConfigured = 0;
   else {
      nStaticSessionsConfigured = ReadSessionConfig(session_data, thread_index);  /* ReadSessionConfig() is in session_app.cpp */
      if (!nStaticSessionsConfigured) goto cleanup;
   }

/* codec config info regardless of sessions mode.  Notes:

   -codec config files are normally used by mediaTest
   -this is intended only for items not session-specific or needed as an override, for example enabling/disabling a codec option for testing purposes
*/

   if (!nStaticSessionsConfigured) ReadCodecConfig(&codec_config_params, thread_index);
   #if 0
   printf(" codec_config_params.payload_shift = %d \n", codec_config_params.payload_shift);
   #endif

/* set up inputs and transcoded outputs */
   
   InputSetup(thread_index);

   TranscodedOutputSetup(thread_index);

   MiscOutputSetup(thread_index);

/* set up SDP info from command line .sdp entry, if any */

   if (strlen(szSDPFile)) SDPSetup(szSDPFile, thread_index);  /* note - SDPSetup() is in sdp_app.cpp */

/* check for any setup errors */

   if (thread_info[thread_index].init_err && !fThreadSync2) goto cleanup;  /* setup error occurred. If this is first time through we clean up and exit, if not, then the user can see which threads / repeat runs have the error and quit as needed */

   #if 0
   printf("hPlatform = 0x%x, nStaticSessionsConfigured = %d, nInPcapFiles = %d, nOutPcapFiles = %d\n", hPlatform, nStaticSessionsConfigured, thread_info[thread_index].nInPcapFiles, thread_info[thread_index].nOutPcapFiles);
   #endif

/* second stage initialization complete */

   memset(hSessions, 0xff, sizeof(hSessions));  /* initialize all session handles to -1. Valid session handles are >= 0 */

session_create:  /* note - label used only if test mode repeats are enabled */

   if (!thread_info[thread_index].fDynamicSessions) {  /* if cmd line not in dynamic sessions mode, create static sessions */

      if (CreateStaticSessions(hSessions, session_data, nStaticSessionsConfigured, thread_index) < 0) goto cleanup;   /* error out if static sessions were configured but none created. Note - CreateStaticSessions() is in session_app.cpp */

      if (!fFirstConsoleMediaOutput) fFirstConsoleMediaOutput = true;
   }

/* all packet I/O and static session creation (if any) complete, sync app threads before continuing */

   AppThreadSync(WAIT_FOR_MASTER_THREAD, &fThreadSync2, thread_index);  /* app threads wait here for master app thread to do following second stage initialization, which includes packet/media thread configuration / start */

/* start packet / media thread(s), if not already started  */

   if (isMasterThread && !fThreadSync2) {  /* isMasterThread defined in mediaMin.h */

      if (!(Mode & START_THREADS_FIRST)) if (StartPacketMediaThreads(num_app_threads > 1 ? NUM_PKTMEDIA_THREADS : 1, thread_index) < 0) goto cleanup;

      fThreadSync2 = true;  /* release any app threads still waiting in AppThreadSync() */
   }

   if ((num_app_threads > 1 && (Mode & ENABLE_RANDOM_WAIT)) || (Mode & ENERGY_SAVER_TEST)) ThreadWait(0, thread_index);  /* staggered start for threads */

   if (!fRepeatFromStart) app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "Starting packet push-pull loop, press 'q' to exit, 'd' for real-time debug output, and other keys as described in online documentation");

/* all initialization complete, begin continuous packet push-pull loop */

   do {

      if (fPause) continue;  /* if keyboard interactive pause is in effect */

      cur_time = get_time(USE_CLOCK_GETTIME); if (!base_time) base_time = cur_time;

      if (Mode & USE_PACKET_ARRIVAL_TIMES) PushPackets(pkt_in_buf, hSessions, session_data, thread_info[thread_index].nSessionsCreated, cur_time, thread_index);  /* in this mode packets are pushed when elapsed time equals or exceeds their arrival timestamp */

   /* if packets are not pushed according to arrival timestamp, then we push packets according to a specified interval. Options include (i) pushing packets as fast as possible (-r0 cmd line entry), (ii) N msec intervals (cmd line entry -rN), and an average push rate based on output queue levels (the latter can be used with pcaps that don't have arrival timestamps) */

      if (cur_time - base_time < interval_count*RealTimeInterval[0]*1000) continue; else interval_count++;  /* if real-time interval has elapsed push and pull packets, increment count. Comparison is in usec. Note that real-time interval may be less than ptime in FTRT modes ("faster than real-time") */

   /* read from packet input flows, push to packet/media threads */

      if (!(Mode & USE_PACKET_ARRIVAL_TIMES)) PushPackets(pkt_in_buf, hSessions, session_data, thread_info[thread_index].nSessionsCreated, cur_time, thread_index);

      //#define SINGLE_STEP
      #ifdef SINGLE_STEP
      if (isMasterThread) { printf("After push\n"); fPause = 1; continue; }
      #endif

   /* pull available packets from packet/media threads, write to output packet flows */

      PullPackets(pkt_out_buf, hSessions, session_data, DS_PULLPACKETS_JITTER_BUFFER, sizeof(pkt_out_buf), thread_index);
      PullPackets(pkt_out_buf, hSessions, session_data, DS_PULLPACKETS_TRANSCODED, sizeof(pkt_out_buf), thread_index);
      PullPackets(pkt_out_buf, hSessions, session_data, DS_PULLPACKETS_STREAM_GROUP, sizeof(pkt_out_buf), thread_index);

   /* check for end of packet input flows, check for end of packet output flows sent by packet/media threads, flush sessions if needed */

      FlushCheck(hSessions, cur_time, &queue_check_time, thread_index);

   /* update screen counters */

      UpdateCounters(cur_time, thread_index);  /* in user_io.cpp */

   /* update test conditions as needed. Note that repeating tests exit the push/pull loop here, after each thread detects end of input and flushes sessions. Also auto-quit exits here (if repeat not enabled) */

      if (!TestActions(hSessions, thread_index)) break;

   } while (!ProcessKeys(hSessions, cur_time, &dbg_cfg, thread_index));  /* process interactive keyboard commands, see user_io.cpp */

 
/* session deletion */

   for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) nRemainingToDelete++;  /* see if any sessions remain to be deleted, depending on operational mode. In dynamic sessions mode all sessions may already be deleted */

   if (nRemainingToDelete) {
   
      sprintf(tmpstr, "Deleting %d session%s [index] hSession/flush state", nRemainingToDelete, nRemainingToDelete > 1 ? "s" : "");
      for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) sprintf(&tmpstr[strlen(tmpstr)], "%s [%d] %d/%d", i > 0 ? "," : "", i, hSessions[i], thread_info[thread_index].flush_state[i]);

      app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_PRINT_ONLY, thread_index, tmpstr);  /* show session delete info onscreen */
      Log_RT(4 | DS_LOG_LEVEL_FILE_ONLY, "mediaMin INFO: %s ", tmpstr);  /* include session delete info in event log */

      for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) {  /* delete sessions */
         DSDeleteSession(hSessions[i]);  /* pktlib API */
         thread_info[thread_index].nSessionsDeleted++;
      }
   }

   app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_PRINT_ONLY, thread_index, "Total sessions created = %d, deleted = %d", thread_info[thread_index].total_sessions_created, thread_info[thread_index].nSessionsDeleted);


/* cleanup before exit or repeat */

cleanup:

/* make sure all sessions are fully deleted before exit or repeat.  Notes:

   -there could be some wait time if (i) wav file output has been specified for stream groups (especially N-channel wav file generation) or (ii) a lot of sessions are open
   -for dynamic sessions operation, if the cmd line had multiple groups, sessions for already completed groups should aleady be deleted, but some groups might still be in the process of deletion 
   -we suppress error messages as session handles are likely to already be invalid
*/
   bool fAllSessionsDeleted;

   do {

      fAllSessionsDeleted = true;

      for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) {

         if (DSGetSessionInfo(hSessions[i] & ~SESSION_MARKED_AS_DELETED, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_DELETE_STATUS | DS_SESSION_INFO_SUPPRESS_ERROR_MSG, 0, NULL) > 0) { fAllSessionsDeleted = false; break; }
      }

   } while (!fAllSessionsDeleted);


/* we either 1) exit on quit, stop, or error condition, or 2) repeat depending on test condition */

   fExitErrorCond = thread_info[thread_index].init_err && (num_app_threads == 1 || thread_index > 0 || !fThreadSync2);

   bool fExit = fQuit || fStop || fExitErrorCond;

   if (fExit) {

      AppThreadSync(WAIT_FOR_ALL_THREADS, NULL, thread_index);  /* wait here for all app threads to arrive */

      if (isMasterThread) {  /* only master thread does exit cleanup */

         pm_run = 0;  /* instruct packet/media thread(s) to exit. Note that in case of error condition, none may have been started or still be running */

         if (!fExitErrorCond) {

            base_time = get_time(USE_CLOCK_GETTIME);
            unsigned long long check_time = 0;
            uint8_t uQuitMessage = 0, fQKey = false;

         /* wait for packet/media worker threads to exit, notes:

            -this includes stream group post-processing (e.g. N-channel wav files) if stream groups are enabled, and collating, analyzing, and writing out packet log if packet history stats are enabled
            -for stream group processing, applies to any worker thread handling sessions with stream groups enabled
            -for packet history stats and analysis, applies to only the master p/m worker thread
            -packet history logging + analysis can take a while if 100k+ packets were pushed/pulled. Max packets for packet logging is somewhere around 300k (can be changed in packet_flow_media_proc.c, look for MAX_PKT_STATS)
         */

  #if 0
  if (fShow_md5sum) {
     char md5str[200] = "";
     GetMD5Sum(md5str, thread_index);
     printf(" \n*** MD5 sum = %s \n", md5str);
  }
  #endif
            while (!fPMMasterThreadExit) {

               if (fPMThreadsClosing && !uQuitMessage) {

                  sprintf(tmpstr, "Waiting for packet/media threads to close%s", fNChannelWavOutput ? ", N-channel wav file processing," : "");

                  if (use_log_file) {  /* use_log_file is set if packet history logging is enabled; see LoggingSetup() below. Cmd line entry -L[filename] enables packet history logging */

                     int num_input_pkts = DSGetThreadInfo(thread_index, DS_THREAD_INFO_NUM_INPUT_PKT_STATS, NULL); 
                     int num_pulled_pkts = DSGetThreadInfo(thread_index, DS_THREAD_INFO_NUM_PULLED_PKT_STATS, NULL); 

                     sprintf(&tmpstr[strlen(tmpstr)], " and packet history logging and analysis of %d input packets and %d output packets", num_input_pkts, num_pulled_pkts);
                  }

                  printf("%s, press 'q' if needed ...\n", tmpstr);
                  uQuitMessage = 1;
               }

               cur_time = get_time(USE_CLOCK_GETTIME);
               if (!check_time) check_time = cur_time;

               if ((uQuitMessage < 2 || fQKey) && cur_time - check_time > 3*1000000L) {  /* after 3 sec */

                  if (!fPMThreadsClosing) {
                     sprintf(tmpstr, "Packet/media threads still not closing after 3 sec, there may be a problem");  /* this should not happen, if so it may indicate a problem */
                     printf("%s, press 'q' if needed ... \n", tmpstr);
                  }
                  else if (fQKey) {
                     sprintf(tmpstr, "Packet/media threads not fully exited after 3 sec, quitting anyway");  /* this should not happen either; at this point we break out of the loop */
                     break;
                  }

                  uQuitMessage = 2;
               }

               if (cur_time - check_time > 5*1000000L) {  /* print a dot every 5 sec -- let user know we're alive if packet logging is taking time due to very long streams */
                  printf(".");
                  check_time = cur_time;
               }

               usleep(250000L);  /* check keyboard input every 1/4 sec */

               if ((char)tolower(getkey()) == 'q') {  /* check for quit key */

                  if (use_log_file) {
                     fprintf(stderr, "Quit key pressed, aborting packet log analysis ... \n");
                     DSConfigLogging(DS_CONFIG_LOGGING_SET_FLAG, NULL, DS_PKTLOG_ABORT | DS_CONFIG_LOGGING_ALL_THREADS);  /* tell all packet/media threads to abort packet logging. Note the "ALL_THREADS" flag will terminate packet logging for any other apps running also (to-do: a flag or a pointer to a thread list that specifies to diaglib to match threads with those created by pktlib), JHB Jan 2023 */
                  }
                  else fprintf(stderr, "Quit key pressed ... \n");

                  fQKey = true;
               }
               else if (fCtrl_C_pressed) {
                  fprintf(stderr, "Ctrl-C key pressed ... \n");  /* Ctrl-C handler is in cmd_line_interface.c. It aborts logging so we don't do it here */
                  fQKey = true;
               }
            }
         }
      }
   }

/* close input file descriptors and encapsulated stream decoding, if any*/

   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) {
   
      if (thread_info[thread_index].pcap_in[i]) {

         if (thread_info[thread_index].pcap_file_hdr[i]) free(thread_info[thread_index].pcap_file_hdr[i]);  /* free file header space, JHB May 2024 */

         DSClosePcap(thread_info[thread_index].pcap_in[i]);
         thread_info[thread_index].pcap_in[i] = NULL;
      }

      #if 0
      if ((Mode & ENABLE_DER_STREAM_DECODE) && thread_info[thread_index].hDerStreams[i]) DSDeleteDerStream(thread_info[thread_index].hDerStreams[i]);
      #else
      if (thread_info[thread_index].hDerStreams[i]) DSDeleteDerStream(thread_info[thread_index].hDerStreams[i]);
      #endif
      if (thread_info[thread_index].hFile_ASN_XML[i]) fclose(thread_info[thread_index].hFile_ASN_XML[i]);
   }

/* close jitter buffer output file descriptors (there may not be any if DISABLE_JITTER_BUFFER_OUTPUT_PCAPS is set on cmd line) */

   for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) {

      if (thread_info[thread_index].fp_pcap_jb[i]) { DSClosePcap(thread_info[thread_index].fp_pcap_jb[i]); thread_info[thread_index].fp_pcap_jb[i] = NULL; }

   /* reset arrival timing stats */

      #if 0  /* for -Rn entry (repeat cmd line) currently stats continue to be calculated without reset between runs. This is true of all mediaMin overall stats, JHB Sep 2023 */
      arrival_avg_delta[i] = 0; arrival_avg_jitter[i] = 0; arrival_max_delta[i] = 0; arrival_max_jitter[i] = 0; last_msec_timestamp[i] = 0; num_arrival_stats_pkts[i] = 0; last_rtp_pyld_len[i] = 0;
      #endif
   }

/* close timestamp-matched wav outputs, if active. This includes post-processing merge, if live merge is not specified (see flags for uTimestampMatchMode in shared_include/streamlib.h), JHB Aug 2023 */

   if (uTimestampMatchMode & TIMESTAMP_MATCH_MODE_ENABLE) DSCloseStreamGroupsTSM(&hSessions[0], thread_info[thread_index].nSessionsCreated, uTimestampMatchMode);  /* pass first session handle, number of sessions, and timestamp matching mode. Note that in repeat mode (-Rn entry) session handles are always unique and do not repeat */

   if (!fExit && (Mode & CREATE_DELETE_TEST)) {

      printf("Recreate test enabled, re-running test from session create, total sessions created = %d\n", thread_info[thread_index].total_sessions_created);

      for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) {
         thread_info[thread_index].flush_state[i] = 0;
         queue_check_time[i] = 0;
      }

      thread_info[thread_index].nSessionsCreated = 0;
      nRemainingToDelete = 0;

      for (i=0; i<MAX_STREAM_GROUPS; i++) {

         thread_info[thread_index].fFirstGroupPull[i] = false;
         for (j=0; j<MAX_INPUT_REUSE; j++) thread_info[thread_index].fGroupTermCreated[i][j] = false;
         memset(&thread_info[thread_index].accel_time_ts[i], 0, sizeof(struct timespec));
      }

      base_time = 0;
      interval_count = 0;

      InputSetup(thread_index);

      fRepeatFromStart = true;
      goto session_create;
   }

/* close transcoded output file descriptors (there may not be any if no -o arguments on command line) */

   for (i=0; i<thread_info[thread_index].nOutPcapFiles; i++) if (thread_info[thread_index].pcap_out[i]) { DSClosePcap(thread_info[thread_index].pcap_out[i]); thread_info[thread_index].pcap_out[i] = NULL; }

/* close stream group output file descriptors and other items */

   for (i=0; i<MAX_STREAM_GROUPS; i++) {

      if (thread_info[thread_index].fp_pcap_group[i]) {

         DSClosePcap(thread_info[thread_index].fp_pcap_group[i]);

      /* copy pcap from group output path to local subfolder. This is just for documentation compatibility purposes, doesn't really have to be done */

         int group_path_len = strlen(szStreamGroupPcapOutputPath);
         char* p = strstr(thread_info[thread_index].szGroupPcap[i], szStreamGroupPcapOutputPath);

         if (p == thread_info[thread_index].szGroupPcap[i] && group_path_len > 0 && strcmp(thread_info[thread_index].szGroupPcap[i], szStreamGroupPcapOutputPath) != 0) {  /* verify that path specified by cmd line --group_pcap, if any, is at start of output pcap file. Also make sure they are not exactly the same */

            char cmdstr[1024];
            sprintf(cmdstr, "cp -f %s %s", thread_info[thread_index].szGroupPcap[i], &thread_info[thread_index].szGroupPcap[i][group_path_len]);
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wunused-result"
            system(cmdstr);
            #pragma GCC diagnostic pop
         }

         thread_info[thread_index].fp_pcap_group[i] = NULL;
      }

      strcpy(thread_info[thread_index].szGroupName[i], "");
      thread_info[thread_index].fFirstGroupPull[i] = false;
      for (j=0; j<MAX_INPUT_REUSE; j++) thread_info[thread_index].fGroupTermCreated[i][j] = false;
      memset(&thread_info[thread_index].accel_time_ts[i], 0, sizeof(struct timespec));
   }

/* check for repeat */

   nRepeatsRemaining[thread_index]--;

   if (!fExit && (fRepeatIndefinitely || nRepeatsRemaining[thread_index] >= 0)) {

      for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) {
         thread_info[thread_index].flush_state[i] = 0;
         queue_check_time[i] = 0;
      }

      for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) {
         thread_info[thread_index].nSessions[i] = 0;
         thread_info[thread_index].fDuplicatedHeaders[i] = false;
         thread_info[thread_index].initial_push_time[i] = 0;
         thread_info[thread_index].total_push_time[i] = 0;
         thread_info[thread_index].dynamic_terminate_stream[i] = 0;
      }

      #if 0  /* allow stream group interval/pull stats for repeating tests */
      thread_info[thread_index].group_interval_stats_index = 0;
      thread_info[thread_index].group_pull_stats_index = 0;
      memset(thread_info[thread_index].GroupIntervalStats, 0, sizeof(GROUPINTERVALSTATS)*MAX_GROUP_STATS);
      memset(thread_info[thread_index].GroupPullStats, 0, sizeof(GROUPPULLSTATS)*MAX_GROUP_STATS);
      #endif

      base_time = 0;
      interval_count = 0;

      ResetDynamicSession_info(thread_index);

      if (Mode & ENABLE_RANDOM_WAIT) ThreadWait(1, thread_index);  /* if random wait is enabled, then each app thread waits a random number of msec */

      thread_info[thread_index].nSessionsCreated = 0;
      nRemainingToDelete = 0;

   /* reset packet stats history before repeating, JHB Jan2020:

      -we could write out packet log to filename with some type of "repeatN" suffix so a log is saved for each repeat, instead of writing once at end of the test run
      -but writing out packet stats history and analyzing input vs. jitter buffer output takes time, and if we do it on every repeat cycle it will cause a delay in the mediaMin application thread
   */

      if (isMasterThread) DSWritePacketStatsHistoryLog(0, NULL, DS_WRITE_PKT_STATS_HISTORY_LOG_THREAD_INDEX | DS_WRITE_PKT_STATS_HISTORY_LOG_RESET_STATS | ((Mode & DISABLE_DORMANT_SESSION_DETECTION) ? DS_PKTSTATS_ORGANIZE_COMBINE_SSRC_CHNUM : 0));

      sprintf(tmpstr, "Cmd line completed, repeating");
      if (!fRepeatIndefinitely) sprintf(&tmpstr[strlen(tmpstr)], ", number of repeats remaining %d", nRepeatsRemaining[thread_index]+1);
      else sprintf(&tmpstr[strlen(tmpstr)], " ...");
      app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_PRINT_ONLY, thread_index, tmpstr);

      fRepeatFromStart = true;
      goto start;
   }

   unsigned int nOrphansRemoved, nMaxListFragments;

   nOrphansRemoved = DSPktRemoveFragment(NULL, &nMaxListFragments, 0);

/* print final mediaMin stats - session summary, stream group output timing */

   if (!fExitErrorCond && !fStressTest && !fCapacityTest && (thread_info[thread_index].fDynamicSessions || (Mode & ENABLE_STREAM_GROUPS))) {

   /* note stats are concatenated into one string and one call to app_printf() prior to display and logging; this avoids fragmentation and mixing with other messages when multiple app threads and packet/media threads are running */

      sprintf(tmpstr, "=== mediaMin stats");
      if (num_app_threads > 1) sprintf(&tmpstr[strlen(tmpstr)], " (%d)", thread_index);

      sprintf(&tmpstr[strlen(tmpstr)], "\n%spackets [input]", tabstr);

      sprintf(&tmpstr[strlen(tmpstr)], "\n%s%stotal in stream ", tabstr, tabstr);
      for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], "%s[%d]%u", i != 0 ? " " : "", i, thread_info[thread_index].packet_number[i]);

      sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sFragments = ", tabstr, tabstr);
      for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], "%s[%d]%u", i != 0 ? " " : "", i, thread_info[thread_index].num_packets_fragmented[i]);

      sprintf(&tmpstr[strlen(tmpstr)], ", reassembled = ");
      for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], "%s[%d]%u", i != 0 ? " " : "", i, thread_info[thread_index].num_packets_reassembled[i]);

      sprintf(&tmpstr[strlen(tmpstr)], ", orphans = ");
      sprintf(&tmpstr[strlen(tmpstr)], "%u", nOrphansRemoved);

      sprintf(&tmpstr[strlen(tmpstr)], ", max on list = ");
      sprintf(&tmpstr[strlen(tmpstr)], "%u", nMaxListFragments);

      sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sTCP = ", tabstr, tabstr);
      for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], "%s[%d]%u", i != 0 ? " " : "", i, thread_info[thread_index].num_tcp_packets[i]);

      sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sUDP = ", tabstr, tabstr);
      for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], "%s[%d]%u", i != 0 ? " " : "", i, thread_info[thread_index].num_udp_packets[i]);

      sprintf(&tmpstr[strlen(tmpstr)], ", encapsulated = ");
      for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], "%s[%d]%u", i != 0 ? " " : "", i, thread_info[thread_index].num_packets_encapsulated[i]);

      sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sRTP = ", tabstr, tabstr);
      for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], "%s[%d]%u", i != 0 ? " " : "", i, thread_info[thread_index].num_rtp_packets[i]);

      sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sRedundant discards TCP = ", tabstr, tabstr);
      for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], "%s[%d]%u", i != 0 ? " " : "", i, thread_info[thread_index].tcp_redundant_discards[i]);  /* display/log number of redundant TCP retransmissions discarded, if any */

      sprintf(&tmpstr[strlen(tmpstr)], ", UDP = ");
      for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) sprintf(&tmpstr[strlen(tmpstr)], "%s[%d]%u", i != 0 ? " " : "", i, thread_info[thread_index].udp_redundant_discards[i]);  /* display/log number of redundant UDP retransmissions discarded, if any */

      strcat(tmpstr, "\n");

   /* if cmd line has --md5sum, display md5sum info, JHB Sep 2023 */

      if (fShow_md5sum) GetMD5Sum(tmpstr, thread_index);

   /* if specified, display packet arrival stats, JHB Aug 2023 */

      if ((Mode & USE_PACKET_ARRIVAL_TIMES) && (Mode & SHOW_PACKET_ARRIVAL_STATS)) {

         sprintf(&tmpstr[strlen(tmpstr)], "%sarrival timing [session]", tabstr);

         sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sdelta avg/max (msec) = ", tabstr, tabstr);
         for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) sprintf(&tmpstr[strlen(tmpstr)], "%s[%d]%4.2f/%4.2f", i != 0 ? " " : "", i, arrival_avg_delta[i], arrival_max_delta[i]);

         sprintf(&tmpstr[strlen(tmpstr)], "\n%s%sjitter avg/max (msec) = ", tabstr, tabstr);
         for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) sprintf(&tmpstr[strlen(tmpstr)], "%s[%d]%4.2f/%4.2f", i != 0 ? " " : "", i, arrival_avg_jitter[i], arrival_max_jitter[i]);

         strcat(tmpstr, "\n");
      }

      bool fLogTimeStampPrinted = false;  /* make sure only one event log timestamp is printed in the case of multiple strings */

      if (thread_info[thread_index].fDynamicSessions) {

         sprintf(&tmpstr[strlen(tmpstr)], "%srun-time [session]\n", tabstr);

         for (i=0; i<thread_info[thread_index].dynamic_session_stats_index; i++) {

            char szSessInfo[200];
            sprintf(szSessInfo, "%s%s[%d] hSession %d, codec = %s, bitrate = %d, payload type = %d, ssrc = 0x%x \n", tabstr, tabstr, i, thread_info[thread_index].DynamicSessionStats[i].hSession, thread_info[thread_index].DynamicSessionStats[i].codecstr, thread_info[thread_index].DynamicSessionStats[i].bitrate, thread_info[thread_index].DynamicSessionStats[i].payload_type, thread_info[thread_index].DynamicSessionStats[i].ssrc);

            if (strlen(tmpstr) + strlen(szSessInfo) < sizeof(tmpstr)) sprintf(&tmpstr[strlen(tmpstr)], "%s", szSessInfo);  /* if enough sessions and/or repeats we need to split up printouts, JHB Jun2020 */
            else {
               app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_SAME_LINE | APP_PRINTF_EVENT_LOG | (fLogTimeStampPrinted ? APP_PRINTF_EVENT_LOG_NO_TIMESTAMP : 0), thread_index, tmpstr);
               app_printf(APP_PRINTF_EVENT_LOG | APP_PRINTF_EVENT_LOG_NO_TIMESTAMP, thread_index, szSessInfo);  /* added a second app_printf() and fLogTimeStampPrinted to fix a bug in reporting multi-hundred count session summary stats that appeared during 2+ hr stress tests. Combining all summary stats per thread into one string would be optimal for multi-thread app operation, but they could potentially be really huge strings needing std::string (or malloc + realloc, whichever is faster). Also would need to deal with Log_RT() (called inside app_printf) which truncates around 4k. Not something to worry about for the time being, JHB Jan 2023 */
               fLogTimeStampPrinted = true;
               tmpstr[0] = (char)0;  /* start the string over */
            }
         }
      }

      if (strlen(tmpstr)) app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_EVENT_LOG | (fLogTimeStampPrinted ? APP_PRINTF_EVENT_LOG_NO_TIMESTAMP : 0), thread_index, tmpstr);  /* display stats summary */
  
   /* display stream group output stats */

      if ((Mode & ENABLE_STREAM_GROUPS) && !isAFAPMode && !isFTRTMode) {  /* in "as fast as possible" and "faster than real-time" modes (-r0 cmd line entry) we are currently not showing stream group output stats. This may change after extensive AFAP and FTRT mode testing, JHB May 2023 */
  
         app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_EVENT_LOG_NO_TIMESTAMP, thread_index, "\tMissed stream group intervals = %d", thread_info[thread_index].group_interval_stats_index);

         for (i=0; i<thread_info[thread_index].group_interval_stats_index; i++) {

            sprintf(tmpstr, "\t[%d] missed stream group interval = %d, hSession = %d", i, thread_info[thread_index].GroupIntervalStats[i].missed_interval, thread_info[thread_index].GroupIntervalStats[i].hSession);
            if (thread_info[thread_index].GroupIntervalStats[i].repeats) sprintf(&tmpstr[strlen(tmpstr)], " %dx", thread_info[thread_index].GroupIntervalStats[i].repeats+1);

            app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_EVENT_LOG_NO_TIMESTAMP, thread_index, tmpstr);
         }

         app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_EVENT_LOG_NO_TIMESTAMP, thread_index, "\tMarginal stream group pulls = %d", thread_info[thread_index].group_pull_stats_index);

         for (i=0; i<thread_info[thread_index].group_pull_stats_index; i++) {
            sprintf(tmpstr, "\t[%d] marginal stream group pull at %d, retries = %d, hSession = %d", i, thread_info[thread_index].GroupPullStats[i].retry_interval, thread_info[thread_index].GroupPullStats[i].num_retries, thread_info[thread_index].GroupPullStats[i].hSession);
            app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_EVENT_LOG_NO_TIMESTAMP, thread_index, tmpstr);
         }
      }
   }

/* clean up and exit */

   if (isMasterThread) {

      DSConfigMediaService(NULL, 0, DS_CONFIG_MEDIASERVICE_EXIT | DS_CONFIG_MEDIASERVICE_THREAD, NULL, NULL);  /* close packet/media thread(s), JHB Dec 2022 */

      if (hPlatform != -1) DSFreePlatform((intptr_t)hPlatform);  /* free DirectCore platform handle. See DSAssignPlatform() comments above */

      DSCloseLogging(0);  /* close event logging. See diaglib.h */
   }

   sprintf(tmpstr, "mediaMin app end");
   if (num_app_threads > 1) sprintf(&tmpstr[strlen(tmpstr)], " (%d)", thread_index);
   printf("%s\n", tmpstr);

   #ifdef _MEDIAMIN_  /* _MEDIAMIN_ defined in mediaMin Makefile */
   return 0;
   #else
   return NULL;
   #endif

}  /* end of main() */


/* local functions */

unsigned int count_threads(unsigned int* pThreadList) {
unsigned int i, count = 0;

   for (i=0; i<num_app_threads; i++) if (__sync_fetch_and_add(pThreadList, 0) & (1 << i)) count++;
   return count;
}

/* AppThreadSync() implements thread "sync points", where application threads wait for the master thread or for each other */

void AppThreadSync(unsigned int mode, bool* fThreadSync, int thread_index) {

static unsigned int uThreadList = 0;  /* bitwise "arrival list" of threads used in WAIT_FOR_ALL_THREADS mode */

   #define  WAIT_1MSEC 1000  /* 1000 usec */

   if (mode & WAIT_FOR_MASTER_THREAD) {  /* non-master threads wait for master thread (i.e. wait for master thread to perform initialization, housekeeping, one-time only task, etc) */

      while (!isMasterThread && fThreadSync && !*fThreadSync) usleep(WAIT_1MSEC);  /* master thread sets global flag pointed to by *fThreadSync after finishing it's task */
   }

   if (mode & WAIT_FOR_ALL_THREADS) {  /* wait until all threads have arrived */

      __sync_or_and_fetch(&uThreadList, 1 << thread_index);  /* set bit in list indicating thread has arrived */

      if (isMasterThread) {

         while (count_threads(&uThreadList) < num_app_threads) usleep(WAIT_1MSEC);  /* wait until everyone is here */

         __sync_lock_test_and_set(&uThreadList, 0);  /* master thread clears the arrival list, leaving it ready for reuse */
         #if 1  /* added release, was not an issue as last AppThreadSync() call was only one with WAIT_FOR_ALL_THREADS, JHB Jan 2023 */
         __sync_lock_release(&uThreadList);
         #endif
      }
      else while (__sync_fetch_and_add(&uThreadList, 0)) usleep(WAIT_1MSEC);  /* non-master app threads wait for the list to be reset */
   }
}

/* PmThreadSync() waits for master p/m thread to cross a specific point. This can be used to initially sync execution start between app thread and master p/m thread, which may help when debugging timing wobbles make results less repeatable */

void PmThreadSync(int thread_index) {

uint8_t before_sync, after_sync;

   do {

      before_sync = __sync_fetch_and_add(&pm_sync[0], 0);
      after_sync = __sync_fetch_and_add(&pm_sync[0], 0);

   } while (before_sync == after_sync);
}


/* following are dynamic session creation definitions and local functions */

#define MAX_KEYS 512  /* increased from 128, JHB Jun 2024 */

/* keys are unique per session; not hashes. No run-time locks are needed */ 

#define INCLUDE_PYLDTYPE_IN_KEY
#ifdef INCLUDE_PYLDTYPE_IN_KEY
#define KEY_LENGTH 37   /* each key is up to 37 bytes (ipv6 address size (2*16) + udp port size (2*2)) + RTP payload type (1) */
#else
#define KEY_LENGTH 36   /* each key is up to 36 bytes (ipv6 address size (2*16) + udp port size (2*2)) */
#endif

uint8_t keys[MAX_APP_THREADS][MAX_KEYS][KEY_LENGTH] = {{ 0 }};
uint32_t nKeys[MAX_APP_THREADS] = { 0 };

/* FindSession() looks for new streams in the specified packet and returns 1 if found.  Notes:

  -finding a new stream means a new session should be created "on the fly" (i.e. dynamic session creation). A new stream is determined by (i) new IP addr:port header and/or (ii) new RTP payload type
  -this info is combined into a "key" that defines the session and is saved to compare with existing sessions
  -SSRC is not included in the key, in order to maintain RFC8108 compliance (multiple RTP streams within the same session)
  -DTMF packets must match an existing session excluding payload type; i.e. they will not cause a new session to be created
  -each application thread has its own set of keys to avoid semaphores / locks. It doesn't help with duplicated streams in multiple pcaps (however, note that that PushPackets() looks for this and deals with it by slightly altering duplicated inputs to make them unique)
*/

int FindSession(uint8_t *pkt, int pkt_len, uint8_t rtp_pyld_type, int pyld_size, int thread_index) {

int version = pkt[0] >> 4;
uint8_t key[KEY_LENGTH] = { 0 };
bool fFoundMatch = false;
#if 0
uint8_t fInitKeys = 0;
#endif
unsigned int len = 0;
struct udphdr *udphdr;

   if (version == 4)
   {  
      struct iphdr *iphdr = (struct iphdr *)pkt;
      udphdr = (struct udphdr *)&pkt[iphdr->ihl*4];

   /* copy IPv4 src/dst addr info to key. Note we don't care about byte order here for internal key purposes */

      memcpy(key, &iphdr->saddr, sizeof(iphdr->saddr));
      len += sizeof(iphdr->saddr);
      memcpy(&key[len], &iphdr->daddr, sizeof(iphdr->daddr));
      len += sizeof(iphdr->daddr);
   }
   else if (version == 6)
   {
      struct ipv6hdr *ipv6hdr = (struct ipv6hdr *)pkt;
      udphdr = (struct udphdr *)&pkt[sizeof(struct ipv6hdr)];

   /* copy IPv6 src/dst addr info to key */

      memcpy(key, &ipv6hdr->saddr.s6_addr, sizeof(ipv6hdr->saddr));
      len += sizeof(ipv6hdr->saddr);
      memcpy(&key[len], &ipv6hdr->daddr.s6_addr, sizeof(ipv6hdr->daddr));
      len += sizeof(ipv6hdr->daddr);
   }
   else
   {
      fprintf(stderr, "FindSession() says invalid IP version field in packet: %d\n", version);
      return -1;  /* error condition */
   }

/* copy UDP port info to key */

   memcpy(&key[len], &udphdr->source, sizeof(udphdr->source));
   len += sizeof(udphdr->source);
   memcpy(&key[len], &udphdr->dest, sizeof(udphdr->dest));
   len += sizeof(udphdr->dest);

   #ifdef INCLUDE_PYLDTYPE_IN_KEY

/* copy RTP payload type to key (but not DTMF packets, which must match an existing session, JHB May 2019) */

   if (pyld_size != 4) key[len++] = rtp_pyld_type;
   #endif

/* see if we already know about this stream */
   
   for (unsigned int i=0; i<nKeys[thread_index]; i++) {

      if (!memcmp(keys[thread_index][i], key, len)) { fFoundMatch = true; break; }
   }

   if (!fFoundMatch) {  /* new stream */

      memcpy(keys[thread_index][nKeys[thread_index]], key, len);
      #if 0
      if (nKeys[thread_index] == 0) fInitKeys = 1;
      #endif
      if (nKeys[thread_index] >= MAX_KEYS) return -1;  /* error condition */
      else nKeys[thread_index]++;
   }

   /*static int cnt = 0;printf("check_for_new_sesion: cnt = %d, found_match = %d, nKeys = %d, fInitKeys = %d, len = %d\n", cnt++, found_match, nKeys[thread_index], fInitKeys, len);
   printf("key value: "); for(i = 0; i < KEY_LENGTH; i++) printf("%02x ", key[thread_index][i]); printf("\n");*/

   #if 0
   if ((!fInitKeys || thread_info[thread_index].fDynamicSessions) && !found_match) return 1;  /* new session */
   else return 0;  /* existing session */
   #else
   return !fFoundMatch ? nKeys[thread_index] : 0;
   #endif
}

void ResetDynamicSession_info(int thread_index) {

   nKeys[thread_index] = 0;
   memset(keys[thread_index], 0, MAX_KEYS*KEY_LENGTH);
}

/* codec types currently supported in codec estimation algorithm (used by dynamic session creation). Add codec types as needed. Note these are local definitions, we later translate these to SigSRF definitions in shared_include/codec.h */

enum {
  G711U = 1,
  G711A,
  G726,
  G722,
  G729AB,
  AMR,
  AMR_WB,
  EVS,
  H263,
  H264,
  H265
};

/* detect_codec_type_and_bitrate() uses a heuristic algorithm to estimate codec type and bitrate. Notes:

   -static payload types G711u/A, G726, and G729 are handled per RFC (https://en.wikipedia.org/wiki/RTP_payload_formats)
   -dynamic payload types for AMR-WB, AMR-NB, and EVS codecs are handled by the algorithm. Auto-detection for these codecs is periodically updated as new pcaps are tested. As of early 2023 the test set is around 120 pcaps
   -factors including RTP payload size, compact vs header full (or bandwidth efficient vs octet aligned), and CMR field and ToC byte (if present) are considered
   -to keep the algorithm as flexible as possible, some EVS bitrates may be mis-identified, but that's not a problem as the EVS decoder will determine the correct bitrate on-the-fly

   -input params are packet pointer, RTP payload size, RTP payload type, and initial codec_type. The latter is given as follows:
      -codec_type zero specifies full auto-detection
      -codec_type already set to one of above enums (for example after SDPInput() processing) specifies partial detection (currently limited to bitrate)

   -output params are codec type (if full auto-detection specified), bitrate, ptime, and cat (category) which is a debug helper showing the decision path taken by the algorithm

   -as of Oct 2023 tested on over 150 pcaps with a wide range of codec types and bitrates, no known failures. If you have a pcap not detected correctly please anonymize (you can use TraceWrangler) and submit on the Github page or privately by e-mail
*/

int detect_codec_type_and_bitrate(uint8_t* rtp_pkt, uint32_t rtp_pyld_len, uint8_t payload_type, int codec_type, uint32_t* bitrate, uint32_t* ptime, uint8_t* cat) {

/* handle static / predefined payload types */

   if (payload_type == 0) {
      *bitrate = 64000;
      return G711U;
   }
   else if (payload_type == 8) {
      *bitrate = 64000;
      return G711A;
   }
   else if (payload_type == 2) {

      switch (rtp_pyld_len) {
         case 100:
            *bitrate = 40000;
            break;
         case 80:
            *bitrate = 32000;
            break;
         case 48:
            *bitrate = 24000;
            break;
         case 40:
            *bitrate = 16000;
            break;
      }
      return G726;
   }
   else if (payload_type == 18) {
      *bitrate = 8000;
      return G729AB;
   }

   #define ENABLE_H26X
   #ifdef ENABLE_H26X

/* scan for H.26x video, JHB Jun 2024 */

   if (rtp_pyld_len > 10) {  /* omit small payloads (which for audio typically contain SID frames */

      if ((((rtp_pkt[0] << 8) | rtp_pkt[1]) & 0x81f8) == 0 && (rtp_pkt[1] & 7) != 0) {  /* parse NAL unit header, see if F bit and LayerId are zero and TID is not */

         char code_seq_3byte[3] = { '\0', '\0', '\1' };
         char code_seq_4byte[4] = { '\0', '\0', '\0', '\1' };

         if (!memmem(&rtp_pkt[2], rtp_pyld_len-2, code_seq_3byte, sizeof(code_seq_3byte)) && !memmem(&rtp_pkt[2], rtp_pyld_len-2, code_seq_4byte, sizeof(code_seq_4byte))) {  /* look for false positives: if forbidden bitstream code sequences found it's not H.26x */

            *bitrate = 320000;  /* temp */
            return H265;
         }
      }
   }
   #endif

/* audio, dynamic payload types */

/* added 0x21 check to pick up AMR-WB in 13041.0 pcap, JHB Feb 2019 */
/* added 0x24 check to pick up AMR-WB for AMR-WB 12.65 7-byte SID, JHB Dec 2020 */

   if ((codec_type == AMR || codec_type == AMR_WB) ||  /* add non-auto-detect: AMR-xx codec type already set (e.g. SDP file input), JHB Jan 2021 */
       (!codec_type && (((rtp_pkt[0] == 0xf1 || rtp_pkt[0] == 0x21) && !(rtp_pkt[1] & 0x80)) || ((rtp_pkt[0] == 0xf4 || rtp_pkt[0] == 0x24) && (rtp_pkt[1] & /*0x80*/0xc0))))  /* auto-detect: look for AMR first, check CMR byte and first bit of ToC byte. Changed 0x80 to 0xc0 mask to detect AMR-NB SID, JHB Nov 2018 */
      )
   {
      *cat = 1;  /* category 1 */

// AMR-WB 12.65 kbps SID: 24e4e77e584c80

      switch (rtp_pyld_len) {

         case 6:  /* SID frames = AMR-WB */
         case 7:
            if (codec_type == AMR || (!codec_type && (rtp_pkt[1] & 0x80) == 0)) {  /* added to detect AMR-NB SID, JHB Nov 2018.  Note, modified this slightly to deal with 13041.0 pcap which has AMR-WB 23850 SIDs.  AMR-NB SID pcaps include AMR_MusixFile, 3838.ws, 6537.0, JHB Feb 2019 */
               *bitrate = 12200;
               return AMR;
            }

         case 33:
            if ((!codec_type || codec_type == AMR_WB) && !*bitrate) *bitrate = 12650;  /* notes:  1) this is a kludge fall-through for some AMR-WB SID cases, for example AMR-WB 23850 SID vs 12650 SID there will be a problem, 2) payload size 33 conflicts with (i) EVS 13200 compact header format and (ii) AMR-NB 12200 octet-aligned */
         case 37:
            if ((!codec_type || codec_type == AMR_WB) && !*bitrate) *bitrate = 14250;
         case 47:
            if ((!codec_type || codec_type == AMR_WB) && !*bitrate) *bitrate = 18250;
         case 51:
            if ((!codec_type || codec_type == AMR_WB) && !*bitrate) *bitrate = 19850;
         case 59:
            if ((!codec_type || codec_type == AMR_WB) && !*bitrate) *bitrate = 23050;
         case 61:
         case 62:
            if (!codec_type || codec_type == AMR_WB) return AMR_WB;  /* default 23850 */

         case 31:
         case 32:
            if (!codec_type || codec_type == AMR) {
               *bitrate = 12200;
               return AMR;
            }
      }
   }

   if (
       (codec_type == AMR || codec_type == AMR_WB) ||
       (!codec_type && (rtp_pkt[0] == 0xf0 && !(rtp_pkt[1] & 0x80)))  /* check for AMR-xx octet-aligned with CMR byte 15 (no specific mode requested) */
       ) {
   
      *cat |= 2;  /* category 1 */

      if (rtp_pyld_len == 33) {
         *bitrate = 12200;
         return AMR;
      }
      else if (rtp_pyld_len == 62) {  /* AMR 23850 octet-aligned */
         *bitrate = 23850;
         return AMR_WB;
      }
   }

   *cat |= 4;  /* category 4 */

/* mostly likely EVS, but could still be some AMR NB/WB bitrates depending on payload length. Added checks for codec_type already set, to support SDP file input, JHB Jan 2021 */

   bool fSID_EVS = false, fSID_AMR_NB = false, fSID_AMR_WB = false, fAMRWB_IOMode = false;

   switch (rtp_pyld_len) {

      case 6:  /* EVS or AMR SID frames, compact format (or bandwidth efficient), header full (or octet aligned), or header full with CMR byte */
      case 7:

      /* check for valid EVS and AMR SIDs by calling DSGetPayloadInfo() API in voplib, which does an exact and thorough check, including EVS AMR-WB IO mode. If not found, then we fall through to non-SID detection case statements, JHB Jun 2023 */

         DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_EVS, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, NULL, &fSID_EVS, NULL);
         if (fSID_EVS || codec_type == EVS) {
            *bitrate = 13200;  /* we don't know actual bitrate yet, so we guess 13.2 kbps which is typical. Not a problem as EVS decoder will figure it out */
            return EVS;
         }

         DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_AMR_NB, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, NULL, &fSID_AMR_NB, NULL);
         if (fSID_AMR_NB || codec_type == AMR) {
            *bitrate = 12200;  /* we don't know actual bitrate yet, so we guess 12.2 kbps which is typical. Not a problem as AMR decoder will figure it out */
            return AMR;
         }

         DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_AMR_WB, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, NULL, &fSID_AMR_WB, NULL);
         DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_EVS, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, &fAMRWB_IOMode, &fSID_EVS, NULL);

         #if 0
         printf("before EVS check, fSID AMRWB = %d, fAMRWB_IOMode = %d, fSID EVS = %d, rtp_pkt[0] = 0x%x \n", fSID_AMR_WB, fAMRWB_IOMode, fSID_EVS, rtp_pkt[0]);
         #endif

         if ((!fAMRWB_IOMode && !fSID_EVS && ((rtp_pkt[0] & 0xe0) == 0x60)) || codec_type == EVS) {
            *bitrate = 5900;  /* EVS primary 2800 bps, no ToC (compact header format), no CMR. Note (i) 2800 bps only occurs during 5900 bps VBR mode (average variable bit rate), (ii) attempting to create an encoder instance with 2800 bps bitrate will cause errors, JHB Nov 2023 */
            return EVS; 
         }

         if (fSID_AMR_WB || codec_type == AMR_WB) {
            *bitrate = 23850;  /* we don't know actual bitrate yet, so we guess 23.85 kbps which is typical. Not a problem as AMR-WB decoder will figure it out */
            return AMR_WB;
         }

         goto ret;  /* don't fall through to 8 */

      case 8:
         if (!codec_type || codec_type == EVS) {
            DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_EVS, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, &fAMRWB_IOMode, &fSID_EVS, NULL);
            if (!fAMRWB_IOMode && fSID_EVS) {
               *bitrate = 13200;  /* EVS CMR + ToC + SID */
               return EVS; 
            }

            if (!fAMRWB_IOMode && !fSID_EVS && rtp_pkt[0] == 0) {
               *bitrate = 5900;  /* EVS primary 2800 bps, ToC (header-full format), no CMR. See notes in case 7 about 5900 bps */
               return EVS; 
            }
         }

         goto ret;  /* don't fall through to 9 */

      case 9:
         if (!codec_type || codec_type == EVS) {
            DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_EVS, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, &fAMRWB_IOMode, &fSID_EVS, NULL);
            if (!fAMRWB_IOMode && !fSID_EVS && (rtp_pkt[0] & 0x80) && rtp_pkt[1] == 0) {
               *bitrate = 5900;  /* EVS primary 2800 bps, ToC (header-full format), with CMR. See notes in case 7 about 5900 bps */
               return EVS; 
            }
         }

         goto ret;  /* don't fall through to 14+ */

      case 14:

         if (
             ((rtp_pkt[0] & 0x0f) == 0 && (rtp_pkt[1] & 0x80) == 0 && !(rtp_pkt[rtp_pyld_len-1] & 0x7f)) ||  /* bandwidth efficient (95 + 10 bits, 1-bit partial trailing byte). Test with 81786.4289256.478164.pcap */
             ((rtp_pkt[0] & 0x0f) == 0 && (rtp_pkt[1] & 0xfb) == 0 && !(rtp_pkt[rtp_pyld_len-1] & 0x01))     /* octet-aligned */
            ) {

            *bitrate = 4750;  /* AMR 4750 bps, JHB Jun 2024 */
            return AMR;
         }

         goto ret;

      case 15:  /* the EVS part of cases 15 and 16 is here because Fraunhofer .rtp files have some EVS stereo 5900 bps examples (mediaTest/pcaps/evs_5900_2_hf0.rtpdump, mediaTest/pcaps/evs_5900_2_hf1.rtpdump), JHB Aug 2023. Notes, May 2024: (i) how to handle stereo EVS in the general case is still being considered, (ii) for the time being even if a mistake is made, as long as auto-detect comes up with EVS (e.g. from a SID packet) then media output should still mostly be corrrect (at least for channel 0), (iii) writing separate channel pcap files would need to be implemented and wav output would need to be modified */
      case 16:

         DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_EVS, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, &fAMRWB_IOMode, &fSID_EVS, NULL);

         if ((!fAMRWB_IOMode && rtp_pkt[0] == 0x40) || codec_type == EVS) {
            *bitrate = 5900;  /* stereo EVS primary 2800 bps, ToC F bit is set (full header format), no CMR, JHB Aug 2023 */
            return EVS; 
         }

         if (rtp_pyld_len == 15 &&
             (
              ((rtp_pkt[0] & 0x0f) == 0 && (rtp_pkt[1] & 0x80) == 1 && !(rtp_pkt[rtp_pyld_len-1] & 0x7f)) ||  /* bandwidth efficient (103 + 10 bits, 1-bit partial trailing byte) */
              ((rtp_pkt[0] & 0x0f) == 0 && (rtp_pkt[1] & 0xfb) == 8 && !(rtp_pkt[rtp_pyld_len-1] & 0x01))     /* octet-aligned */
             )
            )
            {

            *bitrate = 5150;  /* AMR 5150 bps, JHB Jun 2024 */
            return AMR;
         }

         if (rtp_pyld_len == 16 &&
             (
              rtp_pkt[0] & 0x0f) == 1 && (rtp_pkt[1] & 0x80) == 0  /* bandwidth efficient (118 + 10 bits, no partial-bit trailing byte). Test with 85236.4284266.158664.pcap */
             )
            {

            *bitrate = 5900;  /* AMR 5900 bps, JHB May 2024 */
            return AMR;
         }

         goto ret;  /* don't fall through to 17+ */

      case 17:  /* EVS AMR-WB IO 6600 bps compact header, full header, collision case (+1 padding byte), JHB Oct 2023 */
      case 18:  /* either 17 or 18 EVS primary 7200 kbps, added JHB Dec 2020 */

         if ((rtp_pyld_len == 18 && (rtp_pkt[0] & 0xf0) == 0x20) || codec_type == AMR_WB) {
            *bitrate = 6600;  /* AMR-WB (either bandwidth efficient or octet-aligned), JHB May 2024 */
            return AMR_WB;
         }

         if (rtp_pyld_len == 17 &&
             (
              (rtp_pkt[0] & 0x0f) == 0 && (rtp_pkt[1] & 0xfb) == 0x10 && !(rtp_pkt[rtp_pyld_len-1] & 0x03)  /* octet-aligned, 6-bit partial trailing byte */
             )
            )
            {

            *bitrate = 5900;  /* AMR 5900 bps, JHB Jun 2024 */
            return AMR;
         }

      case 19:
         if (!codec_type || codec_type == EVS) {
            DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_EVS, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, &fAMRWB_IOMode, NULL, NULL);
            if (fAMRWB_IOMode) {
               *bitrate = 6600;  /* with or without TOC and padding byte, assumes no CMR */
               return EVS;
            }
            if (rtp_pyld_len == 18 || rtp_pkt[0] == 1) {
               *bitrate = 7200;  /* EVS primary 7200 bps with or without ToC, no CMR */
               return EVS; 
            }
         }
      case 20:  /* EVS primary 8 kbps, JHB Oct 2023 */
      case 21:
         if (!codec_type || codec_type == EVS) {
            *bitrate = 8000;  /* with or without TOC, assumes no CMR */
            return EVS;
         }
      case 23:
      case 24:
      case 25:
         if (!codec_type || codec_type == EVS) {
            DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_EVS, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, &fAMRWB_IOMode, NULL, NULL);
            if (fAMRWB_IOMode) {
               *bitrate = 8850;  /* with or without TOC and padding byte, assumes no CMR */
               return EVS;
            }
            if (rtp_pyld_len == 24 || rtp_pkt[0] == 3) {
               *bitrate = 9600;  /* EVS primary 9600 bps with or without ToC, assumes no CMR */
               return EVS; 
            }
         }

      case 31:
      case 32:
         if (rtp_pyld_len == 32 && ((rtp_pkt[0] & 0x8f) == 0x80) && (!codec_type || codec_type == EVS)) {  /* if AMR-NB 12200 is mis-identified as EVS 12650 this line is the reason why */
            DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_EVS, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, &fAMRWB_IOMode, NULL, NULL);
            if (fAMRWB_IOMode) {
               *bitrate = 12650;  /* without CMR, TOC and padding byte */
               return EVS;
            }
         }
         if (!codec_type || codec_type == AMR) {
            *bitrate = 12200;
            return AMR;
         }

      case 33:  /* EVS 13200 bps, 33 for compact header format, 34 for full header format, 35 for full header with CMR byte */
         if (codec_type == AMR || (!codec_type && !(rtp_pkt[0] & 0x80) && !(rtp_pkt[0] & 0x0f) && rtp_pyld_len > 6)) {  /* look for AMR 12200 octet-aligned with CMR byte, last 4 bits of first byte must be zero. Note for octet-aligned AMR SID RTP payload size must be > 6, JHB Jan 2023 */
            *bitrate = 12200;
            return AMR;
         }
      case 34:
         if (!codec_type || codec_type == EVS) {
            DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_EVS, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, &fAMRWB_IOMode, NULL, NULL);  /* resolve framesize collision between EVS primary mode compact header format 13.2 kbps and EVS AMR-WB IO mode header-full format 12.65 kbps, JHB Oct 2023 */
            if (fAMRWB_IOMode) *bitrate = 12650;  /* AMR-WB IO mode, header-full, one padding byte appended */
            else *bitrate = 13200;  /* primary mode, header-full */
            return EVS; 
         }
      case 35:
         if (!codec_type || codec_type == EVS) return EVS;  /* add non-auto-detect: EVS codec type already set (e.g. SDP file input), JHB Jan 2021 */
      case 37:
         if ((!codec_type || codec_type == AMR_WB) && rtp_pyld_len == 37) {  /* added JHB Jun 2023 */
            *bitrate = 14250;
            return AMR_WB;
         }
      case 41:  /* EVS 16400 bps, 41 for compact header format, 42 for full header format */
      case 42:
         if (!codec_type || codec_type == EVS) {
            DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_EVS, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, &fAMRWB_IOMode, NULL, NULL);  /* resolve framesize collision between EVS primary mode compact header format 16.4 kbps and EVS AMR-WB IO mode header-full format 15.85 kbps, JHB Oct 2023 */
            if (fAMRWB_IOMode) *bitrate = 15850;  /* AMR-WB IO mode, header-full, one padding byte appended */
            else *bitrate = 16400;  /* primary mode, header-full */
            return EVS;
         }
      case 61:
         if (codec_type == AMR_WB || (!codec_type && (rtp_pkt[0] & 0xf8) == 0xf0)) {
            return AMR_WB;  /*  AMR-WB 23850 bandwidth-efficient format */
         }
         else if (!codec_type || codec_type == EVS) {
            *bitrate = 24400;
            return EVS;  /* EVS 24400 bps compact format */
         }
      case 62:
         if (codec_type == AMR_WB || (!codec_type && !(rtp_pkt[0] & 0x0f) && rtp_pyld_len > 6)) {
            *bitrate = 23850;
            return AMR_WB;  /* AMR-WB 23850 octet-aligned format with CMR byte (see test_files/AMR_WB.pcap for example) */
         }
         else if (!codec_type || codec_type == EVS) {
            DSGetPayloadInfo(DS_VOICE_CODEC_TYPE_EVS, DS_CODEC_INFO_TYPE, rtp_pkt, rtp_pyld_len, &fAMRWB_IOMode, NULL, NULL);  /* resolve framesize collision between EVS primary mode compact header format 24.4 kbps and EVS AMR-WB IO mode header-full format 23.85 kbps, JHB Oct 2023 */
            if (fAMRWB_IOMode) *bitrate = 23850;  /* AMR-WB IO mode, header-full, one padding byte appended */
            else *bitrate = 24400;  /* primary mode, header-full */
            return EVS;
         }
      case 63:  /* EVS 24400 bps, header full with CMR byte preface */
         if (!codec_type || codec_type == EVS) {
            *bitrate = 24400;
            return EVS;
         }
      #if 0
      case 31:
      case 32:
         if (!codec_type || codec_type == AMR) {
            *bitrate = 12200;
            return AMR;
         }
      #endif
      case 186:
      case 187:
         if (!codec_type || codec_type == EVS) {
            *bitrate = 24400;
#if 0
            *ptime = 60;  /* EVS uses variable ptime, with ToC byte indicating number of frames in the RTP payload.  For EVS we use ptime of 20 msec in session creation, and let pktlib handle variable ptime on the fly, JHB Oct 2019 */
#endif
            return EVS;
         }

      #if 0
      default:
         if (!codec_type) return -1;
      #endif
   }

ret:

   return codec_type ? codec_type : -1;  /* return codec_type or error condition if still no codec type determined */
}

/* create a new session on-the-fly when dynamic sessions mode is in effect, or during stress tests that create sessions from pcaps. Returns 1 for successful create, 0 if not a codec payload (for example, RTCP packets), and -1 for error condition */
 
int CreateDynamicSession(uint8_t* pkt, int pkt_len, HSESSION hSessions[], SESSION_DATA session_data[], int thread_index, int nInput, int nReuse) {

int i, ip_version, codec_type = 0, ip_hdr_len;
HSESSION hSession;
SESSION_DATA* session;
char codecstr[20];
uint32_t bitrate = 0, ptime = 20;
int rtp_version, rtp_pyld_type, pkt_len_lib, rtp_hdr_len, rtp_pyld_ofs, rtp_pyld_len, rtp_padding_size;
uint32_t rtp_ssrc;
short unsigned int rtp_seqnum;
char group_id[MAX_SESSION_NAME_LEN] = "";
uint8_t cat = 0;  /* init auto-detection category type */
char errstr[300] = "", tmpstr[300], tmpstr2[300];
int target_delay = 0,  max_delay = 0;
bool fShowErrDebugInfo = false;
static bool fPrevErr = false;

/* add SDP related items, JHB Jan 2021 */
bool fSDPPyldTypeFound = false;
uint32_t clock_rate = 0;

/* perform thorough packet validation before creating a session. Notes:

   -we could use DSGetPacketInfo() with the DS_PKT_INFO_PKTINFO flag, which would retrieve all of the items below in a PktInfo struct (see pktlib.h)
   -but we want fine-grained error messages so we use individual DS_PKT_INFO_xxx flags
   -PushPackets() uses the DS_PKT_INFO_PKTINFO flag for higher performance (minimize packet handling overhead)
*/

   bool fShowWarnings = (Mode & ENABLE_DEBUG_STATS) != 0;

   ip_version = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_IP_VERSION | (fShowWarnings ? 0 : DS_PKTLIB_SUPPRESS_ERROR_MSG), pkt, pkt_len, NULL, NULL);

   if (ip_version != 4 && ip_version != 6) {  /* must be IPv4 or IPv6 */
      sprintf(errstr, "invalid IP version = %d, pkt_len = %d", ip_version, pkt_len);
      goto non_rtp_packet;
   }

   ip_hdr_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_HDRLEN | (fShowWarnings ? 0 : DS_PKTLIB_SUPPRESS_ERROR_MSG), pkt, pkt_len, NULL, NULL);

   if (ip_hdr_len < 0) {
      sprintf(errstr, "invalid IP header len = %d, pkt_len = %d", ip_hdr_len, pkt_len);
      goto non_rtp_packet;
   }

   rtp_version = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_VERSION | (fShowWarnings ? 0 : DS_PKTLIB_SUPPRESS_ERROR_MSG | DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG), pkt, pkt_len, NULL, NULL);

   if (rtp_version != 2) {  /* must be RTP v2 */
      sprintf(errstr, "invalid RTP version = %d, pkt_len = %d", rtp_version, pkt_len);
      goto non_rtp_packet;
   }

   rtp_hdr_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_HDRLEN | (fShowWarnings ? 0 : DS_PKTLIB_SUPPRESS_ERROR_MSG | DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG), pkt, pkt_len, NULL, NULL);

   if (rtp_hdr_len <= 0) {
      sprintf(errstr, "invalid RTP header len %d, IP hdr len = %d, pkt len = %d", ip_hdr_len, rtp_hdr_len, pkt_len);
      goto non_rtp_packet;
   }

   rtp_pyld_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDLEN | (fShowWarnings ? 0 : DS_PKTLIB_SUPPRESS_ERROR_MSG | DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG), pkt, pkt_len, NULL, NULL);

   if (rtp_pyld_len <= 0) {
      #if 0
      sprintf(errstr, "invalid RTP payload len %d, pkt len = %d, pkt_len_lib = %d, rtp hdr len = %d", rtp_pyld_len, pkt_len, pkt_len_lib, rtp_hdr_len);
      #else
      sprintf(errstr, "invalid RTP payload len %d, IP hdr len = %d, pkt len = %d", ip_hdr_len, rtp_pyld_len, pkt_len);
      #endif
      goto non_rtp_packet;
   }

   rtp_pyld_type = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDTYPE | (fShowWarnings ? 0 : DS_PKTLIB_SUPPRESS_ERROR_MSG | DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG), pkt, pkt_len, NULL, NULL);

   if (rtp_pyld_type < 0) {
      sprintf(errstr, "invalid RTP payload type = %d, pkt len = %d", rtp_pyld_type, pkt_len);
      fShowWarnings = true;
      goto non_rtp_packet;
   }

   rtp_padding_size = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PADDING_SIZE | (fShowWarnings ? 0 : DS_PKTLIB_SUPPRESS_ERROR_MSG | DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG), pkt, pkt_len, NULL, NULL);

/* UDP + RTP sanity check */

   int tlen;
   if ((tlen = ip_hdr_len + UDP_HEADER_LEN + rtp_hdr_len + rtp_pyld_len + rtp_padding_size - pkt_len) != 0) {

      if (tlen > 0 || (tlen < 0 && !(Mode & ALLOW_OUTOFSPEC_RTP_PADDING))) {  /* as we've seen some pcaps with a few extra bytes at end of each RTP payload, but without RTP padding specified in the RTP header, the cmd line -dN ALLOW_OUTOFSPEC_RTP_PADDING flag can be used avoid the error message, JHB Jan 2023 */

         sprintf(errstr, "malformed UDP/RTP packet, IP hdr len = %d, rtp pyld len = %d, rtp padding size = %d, pkt len = %d, rtp pyld type = %d. Possibly try ALLOW_OUTOFSPEC_RTP_PADDING flag in cmd line -dN argument", ip_hdr_len, rtp_pyld_len, rtp_padding_size, pkt_len, rtp_pyld_type);
         fShowWarnings = true;
         goto non_rtp_packet;
      }
   }

   #if 0
   else {
      static int count = 0;
      if (count < 20) { printf(" ***** pkt ok, pkt_len = %d, rtp_pyld_len = %d \n", pkt_len, rtp_pyld_len); count ++; }
   }
   #endif

/* ignore RTCP packets */
  
   if (rtp_pyld_type >= RTCP_PYLD_TYPE_MIN && rtp_pyld_type <= RTCP_PYLD_TYPE_MAX) return 0;


/* SDP info check: num_rtpmaps will be > 0 if (i) valid cmd line SDP file was given, (ii) added from SIP invite / SDP info packets */

   if (thread_info[thread_index].num_rtpmaps[nInput]) {

   /* search through rtpmap payload types previously parsed and recorded by SDPParseInfo() (in sdp_app.cpp) */

      for (i=0; i<thread_info[thread_index].num_rtpmaps[nInput]; i++) {

         sdp::AttributeRTP* rtpmap = (sdp::AttributeRTP*)thread_info[thread_index].rtpmaps[nInput][i];

         if (rtp_pyld_type == rtpmap->pyld_type) {  /* if found set codec type and sample rate */

            int found_codec_type = 0;

            switch (rtpmap->codec_type) {  /* set codec_type to one of enums listed above */

               case sdp::SDP_G711U:
                  found_codec_type = G711U;
                  break;
               case sdp::SDP_G711A:
                  found_codec_type = G711A;
                  break;
               case sdp::SDP_G722:
                  found_codec_type = G722;
                  break;
               case sdp::SDP_G729:
                  found_codec_type = G729AB;
                  break;
               case sdp::SDP_AMRNB:
                  found_codec_type = AMR;
                  break;
               case sdp::SDP_AMRWB:
                  found_codec_type = AMR_WB;
                  break;
               case sdp::SDP_EVS:
                  found_codec_type = EVS;
                  break;
               case sdp::SDP_H265:
                  found_codec_type = H265;
                  break;

               case sdp::SDP_H263:
               case sdp::SDP_H264:
               case sdp::SDP_CN:  /* codec types in sdp/types.h that SDK version of mediaMin is currently not handling, JHB May 2021 */

               default:
                  int rtp_pyld_type_index = min(max(rtp_pyld_type-96, 0), MAX_DYN_PYLD_TYPES-1);
                  if (!thread_info[thread_index].fUnmatchedPyldTypeMsg[rtp_pyld_type_index][nInput]) {
                     Log_RT(3, "mediaMin WARNING: CreateDynamicSession() says SDP codec type %d for input %d unmatched to supported codecs \n", nInput, rtpmap->codec_type);
                     thread_info[thread_index].fUnmatchedPyldTypeMsg[rtp_pyld_type_index][nInput] = true;
                  }
                  break;
            }

            if (found_codec_type) {

               codec_type = found_codec_type;
               clock_rate = rtpmap->clock_rate;
               fSDPPyldTypeFound = true;  /* set function-wide flag indicating at least one valid rtpmap found */

               #if 0  /* note - currently search continues, so most recent matching rtpmap will be used. To use first matching rtpmap, break out of the search */
               break;  /* break out of the search */
               #endif
            }
         }
      }

      if (!fSDPPyldTypeFound) {

         int rtp_pyld_type_index = min(max(rtp_pyld_type-96, 0), MAX_DYN_PYLD_TYPES-1);

         if (!thread_info[thread_index].fDisallowedPyldTypeMsg[rtp_pyld_type_index][nInput]) {
            char fileinfo[256] = "";
            if (strlen(szSDPFile)) sprintf(fileinfo, "file %s or ", szSDPFile);
            Log_RT(3, "mediaMin WARNING: CreateDynamicSession() says RTP packet with payload type %d found but not defined in SDP %spacket info for input %d, ignoring all RTP packets with this payload type, pkt len = %d, rtp pyld len = %d \n", rtp_pyld_type, fileinfo, nInput, pkt_len, rtp_pyld_len);
            thread_info[thread_index].fDisallowedPyldTypeMsg[rtp_pyld_type_index][nInput] = true;
         }

         return 0;  /* ignore payload types effectively disallowed by SDP file */
      }
   }

/* more packet validation ... */

   pkt_len_lib = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTLEN | (fShowWarnings ? 0 : DS_PKTLIB_SUPPRESS_ERROR_MSG), pkt, pkt_len, NULL, NULL);

   if (pkt_len_lib <= 0) {
      sprintf(errstr, "invalid pkt len = %d, pkt_len param = %d, payload type = %d", pkt_len_lib, pkt_len, rtp_pyld_type);
      fShowWarnings = true;
      goto non_rtp_packet;
   }

   rtp_pyld_ofs = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDOFS | (fShowWarnings ? 0 : DS_PKTLIB_SUPPRESS_ERROR_MSG | DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG), pkt, pkt_len, NULL, NULL);

   if (rtp_pyld_ofs <= 0) {

      sprintf(errstr, "invalid RTP payload offset %d, pkt len = %d, pkt_len_lib = %d, payload type = %d, rtp_pyld_len = %d", rtp_pyld_ofs, pkt_len, pkt_len_lib, rtp_pyld_type, rtp_pyld_len);
      fShowWarnings = true;

non_rtp_packet:

      if (fShowWarnings) fprintf(stderr, "DSGetPacketInfo() returns error value for new session packet, no codec estimation performed, %s \n", errstr);
      return -1;
   }

   rtp_ssrc = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_SSRC | DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG, pkt, pkt_len, NULL, NULL);
   rtp_seqnum = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_SEQNUM | DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG, pkt, pkt_len, NULL, NULL);

/* check for stub packets or out-of-place DTMF packet */

   if (rtp_pyld_len < 6) {

      if (rtp_pyld_len == 2 && (pkt[rtp_pyld_ofs] >= 7 && pkt[rtp_pyld_ofs] <= 39) && pkt[rtp_pyld_ofs+1] == 0xc0) {  /* ignore AMR-WB 2-byte "NO_DATA" packets */
         sprintf(errstr, "packet appears to be AMR-WB NO_DATA with payload len = %d bytes", rtp_pyld_len);
         fShowErrDebugInfo = true;
         goto err_msg;
      }

      if (rtp_pyld_len != 4) {
         sprintf(errstr, "packet has RTP payload size %d less than minimum 4 for DTMF and 5 for media", rtp_pyld_len);
         fShowErrDebugInfo = true;
         goto err_msg;
      }
      else sprintf(errstr, "DTMF packet found at start of new stream, DTMF packets dropped until after stream's first media packet");

err_msg:
      if (fShowErrDebugInfo) {

         char logstr[512];

         sprintf(logstr, "%s, IP ver %d, payload type %d, ssrc = 0x%x, seq num = %d, pkt len %d, RTP pyld offset = %d, RTP pyld size %d, cat %d, pyld[0..2] 0x%x 0x%x 0x%x \n", errstr, ip_version, rtp_pyld_type, rtp_ssrc, rtp_seqnum, pkt_len_lib, rtp_pyld_ofs, rtp_pyld_len, cat, pkt[rtp_pyld_ofs], pkt[rtp_pyld_ofs+1], pkt[rtp_pyld_ofs+2]);

         if (fPrevErr) app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, logstr);
         else Log_RT(4, "mediaMin INFO: %s", logstr);  /* add first occurrence to event log, JHB Sep 2022 */

         fPrevErr = true;
      }
      else fprintf(stderr, "%s \n", errstr);

      return -1;
   }

   fPrevErr = false;

/* detect_codec_type_and_bitrate() notes:

   -see above for input/output param specs
   -if codec_type is zero, full auto-detection is specified
   -if codec_type already set to one of above enums, for example from SDP file input processing above, auto-detection is limited to bitrate (future: SDP file can also force bitrate settings)
*/

   codec_type = detect_codec_type_and_bitrate(&pkt[rtp_pyld_ofs], rtp_pyld_len, rtp_pyld_type, codec_type, &bitrate, &ptime, &cat);

   if (codec_type < 0) {
      strcpy(errstr, "Codec type and/or bitrate detection failed");
      fShowErrDebugInfo = true;
      goto err_msg;
   }

/* create session */

   session = &session_data[thread_info[thread_index].nSessionsCreated];

   memset(session, 0, sizeof(SESSION_DATA));  /* clear all SESSION_DATA items before filling in */

/* set termination endpoints to IPv4 or IPv6 */

   if (ip_version == 4) {

      struct iphdr* iphdr = (struct iphdr*)pkt;

      ip_hdr_len = iphdr->ihl*4;

      session->term1.remote_ip.type = DS_IPV4;
      session->term1.local_ip.type = DS_IPV4;

      session->term1.remote_ip.u.ipv4 = iphdr->saddr;
      session->term1.local_ip.u.ipv4 = iphdr->daddr;
   }
   else if (ip_version == 6) {

      struct ipv6hdr* ipv6hdr = (struct ipv6hdr*)pkt;

      ip_hdr_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_HDRLEN, pkt, pkt_len, NULL, NULL);  /* IPv6 header len normally fixed at 40, but we call pktlib API as customary practice. Note that DSGetPacketInfo() can be called for basic packet info items without a session handle (i.e. using -1 for session handle param) */

      session->term1.remote_ip.type = DS_IPV6;
      session->term1.local_ip.type = DS_IPV6;

      for (i = 0; i < DS_IPV6_ADDR_LEN; i++) {
         session->term1.remote_ip.u.ipv6[i] = ipv6hdr->saddr.s6_addr[i];
         session->term1.local_ip.u.ipv6[i] = ipv6hdr->daddr.s6_addr[i];
      }
   }

   struct udphdr* udphdr = (struct udphdr*)&pkt[ip_hdr_len];
   session->term1.remote_port = udphdr->source;
   session->term1.local_port = udphdr->dest;
   session->term1.attr.voice_attr.rtp_payload_type = rtp_pyld_type;
   session->term1.attr.voice_attr.ptime = ptime;
   session->term1.ptime = ptime;

   session->term1.max_loss_ptimes = 3;

   //#define INCREASE_GAP_REPAIR_SIZE  /* enable to allow larger packet gaps to be repaired. This is likely to cause audio artifacts, for example "cyborg voice", JHB Nov 2023 */
   #ifdef INCREASE_GAP_REPAIR_SIZE
   session->term1.max_pkt_repair_ptimes = 10;
   #else   
   session->term1.max_pkt_repair_ptimes = 4;
   #endif

   #if 1
   /* dormant_SSRC_wait_time controls detection and flush time when a stream "takes over" another stream's SSRC, JHB Sep 2022. Notes:
   
       -set to non-zero (in msec) to override pktlib default of 100 msec
       -set to 1 if immediate flush is needed (which helps avoid packets being dropped from the jitter buffer because they arrived too late)
       -longer wait times can be needed if streams are legitimately alternating use of the same SSRC. In that case it's also possible to completely disable dormant session detection and flush using the cmd line -dn options DISABLE_DORMANT_SESSION_DETECTION flag
   */
   if (Mode & SLOW_DORMANT_SESSION_DETECTION) session->term1.dormant_SSRC_wait_time = 1000;  /* slow dormant session detection time is 1 sec, JHB Jun 2023 */
   #endif
   if (codec_config_params.payload_shift) session->term1.payload_shift = codec_config_params.payload_shift;

/* jitter buffer target and max delay notes, JHB May2020:

   -defaults for stream group processing, in both analytics and telecom modes, are 10 and 14. Stream groups require high accuracy of stream alignment
   -otherwise defaults are 5 and 12 (set in pktlib if not set here)
   -use either 5/12 or 7/12 for "analytics compatibility mode" (this will obtain results prior to June 2020)
   -delay values are specified in "ptime periods" and represent an amount of time. For example a stream that starts with 1 SID packet and 2 media packets will reach the target delay at the same time as a stream that starts with 10 media packets
   -cmd line entry sets the nJitterBufferParams var and takes precedence if specified
*/
  
   if (nJitterBufferParams >= 0) {  /* cmd line param -jN, if entered. nJitterBufferParams is -1 if no cmd line entry */

      target_delay = nJitterBufferParams & 0xff;
      max_delay = (nJitterBufferParams & 0xff00) >> 8;
   }
   else if ((Mode & ENABLE_STREAM_GROUPS) || (Mode & ENABLE_WAV_OUTPUT_TIMESTAMP_MATCH)) {
      target_delay = 10;
      max_delay = 14;
   }
   else {  /* otherwise pktlib sets target_delay to 5 and max_delay to 12 (i.e. if pktlib sees zero values at session-creation time) */
   }

   if (target_delay) session->term1.jb_config.target_delay = target_delay;
   if (max_delay) session->term1.jb_config.max_delay = max_delay;

/* set termination endpoint flags */

   if (!(Mode & DISABLE_DTX_HANDLING)) session->term1.uFlags |= TERM_DTX_ENABLE;
   if (!(Mode & DISABLE_PACKET_REPAIR)) session->term1.uFlags |= TERM_SID_REPAIR_ENABLE | TERM_PKT_REPAIR_ENABLE;  /* packet repair and overrun synchronization flags enabled by default */
   if (Mode & ENABLE_STREAM_GROUPS) session->term1.uFlags |= TERM_OVERRUN_SYNC_ENABLE;
   if ((!(Mode & ANALYTICS_MODE) || fUntimedMode) || target_delay > 7) session->term1.uFlags |= TERM_OOO_HOLDOFF_ENABLE;  /* jitter buffer holdoffs enabled except in analytics compatibility mode */
   if (Mode & DISABLE_DORMANT_SESSION_DETECTION) session->term1.uFlags |= TERM_DISABLE_DORMANT_SESSION_DETECTION;
   session->term1.uFlags |= TERM_DYNAMIC_SESSION;  /* set for informational purposes. Applications should apply this flag for dynamically created sessions in order to see correct stats reported by packet/media threads, although functionality is not affected if the flag is omitted. See also comments in shared_include/session.h */
   session->term1.RFC7198_lookback = uLookbackDepth;  /* number of packets to lookback for RFC7198 de-duplication in DSRecvPackets(), default is 1 if no entry on cmd line (see getUserInfo() in get_user_interface.cpp). Zero entry (-l0) disables. Max allowed is 8, JHB May 2023 */

   if (Mode & ENABLE_STREAM_GROUPS) {

      char szSessionNameTemp[MAX_SESSION_NAME_LEN] = "";

      if (nInput > 0 && (Mode & COMBINE_INPUT_SPECS)) nInput = 0;  /* combine input specs: use first cmd line input spec for group id and stream group pcap output filename. Otherwise use each cmd line input spec for group id and output name */

      if (strlen(szSessionName[nInput])) {

         strncpy(szSessionNameTemp, szSessionName[nInput], MAX_SESSION_NAME_LEN);
         szSessionNameTemp[min(MAX_SESSION_NAME_LEN-1, (int)strlen(szSessionName[nInput]))] = 0;
      }

      if (strlen(thread_info[thread_index].szGroupName[nInput])) {  /* check if group name has already been assigned */

         strcpy(group_id, thread_info[thread_index].szGroupName[nInput]);  /* yes, use assigned group name */
      }
      else {  /* no, determine a unique group name (see notes below) */

         if (!fStressTest && !fCapacityTest && (Mode & DYNAMIC_SESSIONS) && strlen(szSessionNameTemp)) {  /* set group id as session name, which will be used by packet/media threads for output wav files.  Don't do this if (i) static session config or (ii) load/capacity or stress test options are active, JHB Jun 2019 */

            strcpy(group_id, szSessionNameTemp);
         }
         else {

            strcpy(group_id, "stream_group");  /* Note the name "stream_group" matches names used in example static session config files.  We also use this as generic naming for stress and capacity tests */
         }

     /*  Important -- if more than one stream group is created the group name ("group ID") must be unique, so we use input index, thread index, and/or re-use count to form unique group IDs */

// if (!(Mode & COMBINE_INPUT_SPECS) && nInput > 0) printf("\n ============= group_id = %s, prev group id = %s \n", group_id, thread_info[thread_index].szGroupName[nInput-1]);
 
         if (!(Mode & COMBINE_INPUT_SPECS) && (Mode & DYNAMIC_SESSIONS)) {  /* in dynamic sessions mode default operation (and when input specs are not combined) is to generate a unique stream group name for each cmd line input spec:

                                                                              -each input spec is treated as a multistream group; i.e. a separate stream group (each of which may contain multiple streams)
                                                                              -if an input spec is a duplicate of another one, an "_iN" suffix is added
                                                                              -stream group naming is handled separately from duplicate IP header content, which is handled in PushPackets()
                                                                  */
 
            for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) {

               if (i != nInput && strlen(thread_info[thread_index].szGroupName[i]) && !strcmp(group_id, thread_info[thread_index].szGroupName[i])) sprintf(&group_id[strlen(group_id)], "_i%d", nInput);
            }
         }

         strcpy(thread_info[thread_index].szGroupName[nInput], group_id);  /* keep track of stream group names, before non-input spec suffixes are added */
      }

      if (!fStressTest && !fCapacityTest) {

         sprintf(session->szSessionName, "%s%s", szStreamGroupWavOutputPath, thread_info[thread_index].szGroupName[nInput]);  /* set session->szSessionName for wav outputs. Note we currently don't allow wav outputs during capacity test */
      }

      if (nReuse) sprintf(&group_id[strlen(group_id)], "_n%d", nReuse);  /* add the re-use count, if applicable.  nReuse is typically non-zero only for capacity or stress tests */

  /* add the application thread index, if applicable:

     -the number of application threads (num_app_threads) is typically more than one only for capacity or stress tests
     -the number of application threads is independent of the number of packet/media threads
  */
      if (num_app_threads > 1) sprintf(&group_id[strlen(group_id)], "_t%d", thread_index);

      session->term1.group_mode = DS_AUDIO_MERGE_ADD;
      if (Mode & WHOLE_GROUP_THREAD_ALLOCATE) session->term1.group_mode |= STREAM_CONTRIBUTOR_WHOLE_GROUP_THREAD_ALLOCATE;
      if ((Mode & DISABLE_CONTRIB_PACKET_FLUSH) || (!(Mode & USE_PACKET_ARRIVAL_TIMES) && (Mode & AUTO_ADJUST_PUSH_RATE))) session->term1.group_mode |= STREAM_CONTRIBUTOR_DISABLE_PACKET_FLUSH;  /* auto-adjust push rate (i.e. not based on timestamp timing) disqualifies use of packet flush, JHB Dec2019 */
      if ((Mode & USE_PACKET_ARRIVAL_TIMES) && (Mode & ENABLE_ONHOLD_FLUSH_DETECT)) session->term1.group_mode |= STREAM_CONTRIBUTOR_ONHOLD_FLUSH_DETECTION_ENABLE;  /* on-hold flush detection usually makes sense for UDP input or pcaps with wall clock arrival times. One functional test with xxx_ingestion.1654.0.pcap and AUTO_ADJUST_PUSH_RATE will fail without this, JHB Jan2019 */
      strcpy(session->term1.group_id, group_id);
   }

   switch (codec_type) {

      case H265:

         session->term1.codec_type = DS_VIDEO_CODEC_TYPE_H265;
         session->term1.sample_rate = 60;
         session->term1.bitrate = (bitrate == 0) ? 320000 : bitrate;
         strcpy(codecstr, "H.265");
 
         break;

      case EVS:

//         printf("template term1 codec type = %d, flags = %d, sample_rate = %d, bitrate = %d\n", session->term1.codec_type, session->term1.attr.voice_attr.u.evs.codec_flags, session->term1.sample_rate, session->term1.bitrate);

         session->term1.codec_type = DS_VOICE_CODEC_TYPE_EVS;

//#define FORCE_EVS_16KHZ_OUTPUT  /* define this if EVS decoder output should be 16 kHz for stream groups, JHB Mar 2019. Note this definition also affects stream group output sample rate (below), JHB Nov 2023 */
#ifndef FORCE_EVS_16KHZ_OUTPUT
         if (!(Mode & ENABLE_STREAM_GROUP_ASR) && (Mode & ENABLE_STREAM_GROUPS)) {  /* changed this to reduce processing time for EVS decode and improve session capacity with stream group enabled.  Stream group output is G711, so this also avoids Fs conversion prior to G711 encode, JHB Feb 2019 */
            session->term1.attr.voice_attr.u.evs.codec_flags = DS_EVS_FS_8KHZ | (DS_EVS_BITRATE_13_2 << 2);  /* set decode (output) Fs to 8 kHz for improvement in session capacity with merging enabled; bitrate is ignored for decode as EVS decoder figures it out on the fly */
            session->term1.sample_rate = 8000;
         }
         else
#endif
         {
         /* set decode (output) Fs. For static sessions termN.sample_rate may be read from session config file */

            session->term1.attr.voice_attr.u.evs.codec_flags = DS_EVS_FS_16KHZ | (DS_EVS_BITRATE_13_2 << 2);  /* for EVS with ASR specified or without a stream group we set to 16 kHz and 13200 bps, no other flags set.  See "evs_codec_flags" in shared_include/codec.h */
            session->term1.sample_rate = 16000;
         }

      /* for EVS also set incoming sample rate (i.e. rate used by remote end encoder), as it can be independent from decode output Fs. For static sessions termN.input_sample_rate may be read from session config file. Use SDP specified sample rate if found, JHB Jan 2021 */
  
         session->term1.input_sample_rate = (fSDPPyldTypeFound && clock_rate != 0) ? clock_rate : 16000;

      /* set bitrate */

         session->term1.bitrate = (bitrate == 0) ? 13200 : bitrate;  /* for static sessions this is read from session config file, for dynamic sessions this is an estimate produced by the auto-detect algorithm. However in both cases this is eventually not used, as EVS derives actual bitrate from incoming bitstream */
         strcpy(codecstr, "EVS");
         break;

      case AMR_WB:

         session->term1.codec_type = DS_VOICE_CODEC_TYPE_AMR_WB;
         session->term1.sample_rate = 16000;
         session->term1.bitrate = (bitrate == 0) ? 23850 : bitrate;  /* same comment as above for EVS */
         strcpy(codecstr, "AMR-WB");
         break;

      case AMR:

         session->term1.codec_type = DS_VOICE_CODEC_TYPE_AMR_NB;
         session->term1.sample_rate = 8000;
         session->term1.bitrate = (bitrate == 0) ? 12200 : bitrate;  /* same comment as above for AMR_WB */
         strcpy(codecstr, "AMR-NB");
         break;

      case G711U:

         session->term1.codec_type = DS_VOICE_CODEC_TYPE_G711_ULAW;
         session->term1.sample_rate = 8000;
         session->term1.bitrate = 64000;
         strcpy(codecstr, "G711u");
         break;

      case G711A:

         session->term1.codec_type = DS_VOICE_CODEC_TYPE_G711_ALAW;
         session->term1.sample_rate = 8000;
         session->term1.bitrate = 64000;
         strcpy(codecstr, "G711a");
         break;

      default:
         strcpy(codecstr, "none");
         break;
   }

/* dynamic sessions are unidirectional (i.e. for bidirectional streams discovered in input packet flow 2 sessions are created), so here we set arbitrary IP addr and UDP port values for term2. Note - for static sessions term2 values are read from session config file or set via user app, creating bidirectional sessions */

   session->term2.remote_ip.type = DS_IPV4;
   session->term2.remote_ip.u.ipv4 = htonl(0x0A000001 + thread_info[thread_index].nSessionsCreated);
   session->term2.local_ip.type = DS_IPV4;
   session->term2.local_ip.u.ipv4 = htonl(0x0A000101 + thread_info[thread_index].nSessionsCreated);

   session->term2.remote_port = udphdr->source + thread_info[thread_index].nSessionsCreated;
   session->term2.local_port = udphdr->dest + thread_info[thread_index].nSessionsCreated;

   if (uTimestampMatchMode & TIMESTAMP_MATCH_MODE_ENABLE) {  /* for term2 transcoded audio we use codec type L16 (16-bit linear PCM) for timestamp-matched wav output. packet/media threads treat L16 as raw audio. Note this is not a CLEARMODE codec, JHB Aug 2023 */
      session->term2.codec_type = DS_VOICE_CODEC_TYPE_L16;
      session->term2.bitrate = 128000;  /* 8 kHz Fs output. For call recording applications this might need to be 16 kHz Fs */
   }
   else {  /* otherwise default is G711 uLaw */
      session->term2.codec_type = DS_VOICE_CODEC_TYPE_G711_ULAW;
      session->term2.bitrate = 64000;
   }

   session->term2.attr.voice_attr.rtp_payload_type = 0; /* 0 for G711 ulaw */
   session->term2.sample_rate = 8000;
   session->term2.attr.voice_attr.ptime = 20; /* assume 20ms ptime */
   session->term2.ptime = 20;

   session->term2.max_loss_ptimes = 3;
   session->term2.max_pkt_repair_ptimes = 4;
   if (codec_config_params.payload_shift) session->term2.payload_shift = codec_config_params.payload_shift;

   if (target_delay) session->term2.jb_config.target_delay = target_delay;
   if (max_delay) session->term2.jb_config.max_delay = max_delay;

/* set termination endpoint flags */

   if (!(Mode & DISABLE_DTX_HANDLING)) session->term2.uFlags |= TERM_DTX_ENABLE;
   if (!(Mode & DISABLE_PACKET_REPAIR)) session->term2.uFlags |= TERM_SID_REPAIR_ENABLE | TERM_PKT_REPAIR_ENABLE;  /* packet repair and overrun synchronization flags enabled by default */
   if (Mode & ENABLE_STREAM_GROUPS) session->term2.uFlags |= TERM_OVERRUN_SYNC_ENABLE;
   if ((!(Mode & ANALYTICS_MODE) || fUntimedMode) || target_delay > 7) session->term2.uFlags |= TERM_OOO_HOLDOFF_ENABLE;  /* jitter buffer holdoffs enabled except in analytics compatibility mode */
   if (Mode & DISABLE_DORMANT_SESSION_DETECTION) session->term2.uFlags |= TERM_DISABLE_DORMANT_SESSION_DETECTION;
   session->term2.uFlags |= TERM_DYNAMIC_SESSION;  /* set for informational purposes. Applications should apply this flag for dynamically created sessions in order to see correct stats reported by packet/media threads, although functionality is not affected if the flag is omitted. See also comments in shared_include/session.h */
   session->term2.RFC7198_lookback = uLookbackDepth;  /* number of packets to lookback for RFC7198 de-duplication in DSRecvPackets(), default is 1 if no entry on cmd line (see getUserInfo() in get_user_interface.cpp). Zero entry (-l0) disables. Max allowed is 8, JHB May 2023 */

/* group term setup */

   if ((Mode & ENABLE_STREAM_GROUPS) && thread_info[thread_index].fDynamicSessions && !thread_info[thread_index].fGroupTermCreated[!(Mode & COMBINE_INPUT_SPECS) ? nInput: 0][nReuse]) {

      session->group_term.remote_ip.type = DS_IPV4;
      session->group_term.remote_ip.u.ipv4 = htonl(0x0A010001);
      session->group_term.local_ip.type = DS_IPV4;
      session->group_term.local_ip.u.ipv4 = htonl(0x0A010101);

      session->group_term.remote_port = udphdr->source + thread_info[thread_index].nSessionsCreated;
      session->group_term.local_port = udphdr->dest + thread_info[thread_index].nSessionsCreated;

   /* specify stream group output codec and bitrate. Default is G711 but can be changed */

      session->group_term.codec_type = DS_VOICE_CODEC_TYPE_G711_ULAW;
      session->group_term.bitrate = 64000;

   /* specify stream group output sampling rate. Notes:

        -a stream group's output sampling rate is independent of its output codec type. If Fs rates are different, then down-sampling rate conversion is performed prior to codec encode (look for DSConvertFs() in DSProcessAudio() in audio_domain_pocessing.c)
        -ASR requires wideband audio for accuracy reasons. Other types of processing of stream group output may also need wideband or higher bandwidth
   */
      #ifndef FORCE_EVS_16KHZ_OUTPUT  /* optionally defined above */
      if (!(Mode & ENABLE_STREAM_GROUP_ASR)) session->group_term.sample_rate = 8000;  /* fix syntax bug, JHB Feb 2024 */
      else
      #endif
      {
         session->group_term.sample_rate = 16000;
      }

      session->group_term.attr.voice_attr.rtp_payload_type = 0; /* 0 for G711 ulaw */
      session->group_term.attr.voice_attr.ptime = 20; /* assume 20 msec ptime */
      session->group_term.ptime = 20;

      session->group_term.group_mode = STREAM_GROUP_ENABLE_MERGING;  /* STREAM_GROUP_xxx flags are in shared_include/streamlib.h */
      if (Mode & ENABLE_STREAM_GROUP_ASR) session->group_term.group_mode |= STREAM_GROUP_ENABLE_ASR;
      if (Mode & ENABLE_STREAM_GROUP_DEDUPLICATION) session->group_term.group_mode |= STREAM_GROUP_ENABLE_DEDUPLICATION;

      if (Mode & ENABLE_WAV_OUTPUT) {

         session->group_term.group_mode |= STREAM_GROUP_WAV_OUT_MERGED | STREAM_GROUP_WAV_OUT_STREAM_MONO;  /* specify mono and group output wav files.  If merging is enabled, the group output wav file will contain all input streams merged (unified conversation) */

         if (!fStressTest && !fCapacityTest && nRepeatsRemaining[thread_index] == -1) {  /* specify N-channel wav output. Disable if load/capacity or stress test options are active.  Don't enable if repeat is active, otherwise thread preemption warnings will show up in the event log (because N-channel processing takes a while), JHB Jun 2019 */

            session->group_term.group_mode |= STREAM_GROUP_WAV_OUT_STREAM_MULTICHANNEL;
            fNChannelWavOutput = true;
         }

         if (Mode & INCLUDE_PAUSES_IN_WAV_OUTPUT) session->group_term.group_mode |= STREAM_GROUP_WAV_OUT_INCLUDE_PAUSES_AS_SILENCE;  /* handle cmd line option to reflect input pauses in wav outputs, JHB Jul 2023 */
      }

      if (Mode & DISABLE_FLC) session->group_term.group_mode |= STREAM_GROUP_FLC_DISABLE;
      if (Mode & ENABLE_FLC_HOLDOFFS) session->group_term.group_mode |= STREAM_GROUP_FLC_HOLDOFFS_ENABLE;
      if (Mode & ENABLE_DEBUG_STATS) session->group_term.group_mode |= STREAM_GROUP_DEBUG_STATS;
      if (Mode & ENABLE_DEBUG_STATS_L2) session->group_term.group_mode |= STREAM_GROUP_DEBUG_STATS_L2;

      strcpy(session->group_term.group_id, group_id);
   }

/* display dynamic session creation info on stdout and print to event log */

   sprintf(tmpstr,
           "Creating dynamic session %d, input #%d, %s codec type %s, auto-detected bitrate %d"
           "%s%s",  /* stream group, if applicable */
           thread_info[thread_index].nSessionsCreated+1, nInput+1, fSDPPyldTypeFound ? "SDP specified" : "auto-detected", codecstr, session->term1.bitrate,
           strlen(group_id) ? ", stream group " : "", strlen(group_id) ? group_id : "");
   sprintf(tmpstr2,
           "Creation packet info: pkt number = %d, IPv%d, ssrc = 0x%x, seq num = %d, payload type %d, pkt len %d, RTP payload size %d, cat %d, rtp_pkt[0..2] 0x%x 0x%x 0x%x, src port = %u, dst_port = %u",
           thread_info[thread_index].packet_number[nInput], ip_version, rtp_ssrc, rtp_seqnum, rtp_pyld_type, pkt_len_lib, rtp_pyld_len, cat, pkt[rtp_pyld_ofs], pkt[rtp_pyld_ofs+1], pkt[rtp_pyld_ofs+2], thread_info[thread_index].src_port[nInput], thread_info[thread_index].dst_port[nInput]);

   app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_PRINT_ONLY, thread_index, "^^^^^^^ %s\n\t%s", tmpstr, tmpstr2);
   if (num_app_threads > 1) sprintf(&tmpstr2[strlen(tmpstr2)], " (%d)", thread_index);
   Log_RT(4 | DS_LOG_LEVEL_FILE_ONLY, "mediaMin INFO: %s. %s", tmpstr, tmpstr2);

/* set timing values, including termN.input_buffer_interval and termN.output_buffer_interval -- for user apps note it's very important this be done before creating the session */

   SetSessionTiming(session);  /* note - in session_app.cpp */

/* create the session */

   if ((hSession = DSCreateSession(hPlatform, NULL, session, GetSessionFlags())) < 0) {  /* note - GetSessionFlags() is in session_app.cpp */

      app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "Failed to create dynamic session, app thread %d", thread_index); 
      return -2;  /* critical error */
   }

/* store session handle and update session counts */

   hSessions[thread_info[thread_index].nSessionsCreated] = hSession;

   thread_info[thread_index].nSessionsCreated++;  /* nSessionsCreated and nDynamicSessions increment when sessions open, they may also decrement in some test modes. See also .nSessionsDeleted which increments when dynamic sessions close */
   thread_info[thread_index].nDynamicSessions++;
   thread_info[thread_index].total_sessions_created++;  /* total_sessions_created never decrements */

/* update dynamic session stats */

   if (thread_info[thread_index].dynamic_session_stats_index < MAX_DYNAMIC_SESSION_STATS) {

      thread_info[thread_index].DynamicSessionStats[thread_info[thread_index].dynamic_session_stats_index].hSession = hSession;
      strcpy(thread_info[thread_index].DynamicSessionStats[thread_info[thread_index].dynamic_session_stats_index].codecstr, codecstr);
      thread_info[thread_index].DynamicSessionStats[thread_info[thread_index].dynamic_session_stats_index].bitrate = session->term1.bitrate;
      thread_info[thread_index].DynamicSessionStats[thread_info[thread_index].dynamic_session_stats_index].payload_type = rtp_pyld_type;
      thread_info[thread_index].DynamicSessionStats[thread_info[thread_index].dynamic_session_stats_index].ssrc = rtp_ssrc;
      thread_info[thread_index].dynamic_session_stats_index++;
   }

/* set up jitter buffer output for this session */
  
   JitterBufferOutputSetup(thread_index);

/* set up group output if this is a group owner session */

   if ((Mode & ENABLE_STREAM_GROUPS) && thread_info[thread_index].fDynamicSessions && !thread_info[thread_index].fGroupTermCreated[!(Mode & COMBINE_INPUT_SPECS) ? nInput: 0][nReuse]) {

      StreamGroupOutputSetup(hSessions, nInput, thread_index);

      thread_info[thread_index].fGroupTermCreated[!(Mode & COMBINE_INPUT_SPECS) ? nInput: 0][nReuse] = true;
   }

   return 1;  /* return success */
}


/* helper functions for managing our hSessions[] array of session handles:

  -GetNextGroupSessionIndex() looks through active session handles and finds the next one that's a stream group owner
  -GetInputFromSessionIndex() finds the cmd line input spec associated with a session index (i.e. an index into hSessions[])
  -FlushSession() flushes a session
  -DeleteSession() deletes a session
*/

int GetNextGroupSessionIndex(HSESSION hSessions[], int nSessionIndex, int thread_index) {

int ret_val = -1;

   while (nSessionIndex < thread_info[thread_index].nSessionsCreated) {
  
      if (!(hSessions[nSessionIndex] & SESSION_MARKED_AS_DELETED) && DSGetSessionInfo(hSessions[nSessionIndex], DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_OWNER, 0, NULL) == hSessions[nSessionIndex]) {

         ret_val = nSessionIndex;  /* note that we return an index into hSessions[], not a session handle */
         break;
      }
      else nSessionIndex++;
   }

   return ret_val;
}

int GetInputFromSessionIndex(int nSessionIndex, int thread_index) {

int j, k;

   for (j=0; j<thread_info[thread_index].nInPcapFiles; j++) {
      for (k=0; k<thread_info[thread_index].nSessions[j]; k++) if (nSessionIndex == thread_info[thread_index].nSessionIndex[j][k]) return j;
   }

   return -1;
}

void FlushSession(HSESSION hSessions[], int nSessionIndex) {

   DSSetSessionInfo(hSessions[nSessionIndex], DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_STATE, DS_SESSION_STATE_FLUSH_PACKETS, NULL);
}

void DeleteSession(HSESSION hSessions[], int nSessionIndex, int thread_index) {

   DSDeleteSession(hSessions[nSessionIndex]);
   thread_info[thread_index].nSessionsDeleted++;
   hSessions[nSessionIndex] |= SESSION_MARKED_AS_DELETED;  /* mark the session as deleted in our session handles array -- we keep its stats available, but no longer call pktlib APIs using its session handle, JHB Jan2020 */
}


/* push incoming packets to packet/media per-session queue:

   -in dynamic session mode -- using packet arrival timestamps
     -create new sessions as they appear in input packet flow (see comments)
     -packet/media threads handle duplicate packets

   -in static sesssion mode -- using packet arrival timestamps
     -packet/media threads handle duplicate packets

   -in either dynamic or static session modes, if packet arrival timestamps are disabled then
     -if packet arrival timestamps are disabled, then packets are pushed (i) at regular intervals (using -rN command line entry) or (ii) based auto-adjust push algorithm
     -we ask DSPushPackets() to strip duplicate packets and let us know so we can immediately push another packet

   -in all cases:
    -filter non-dynamic UDP ports, act on some UDP control plane messages (SAP/SDP protocol, some SIP messages)
    -filter RTCP packets
*/

int PushPackets(uint8_t* pkt_buf, HSESSION hSessions[], SESSION_DATA session_data[], int nSessions, uint64_t cur_time, int thread_index) {

int i, j, n, pkt_len, pkt_info_ret_val = 0;
unsigned int uFlags_push = !(Mode & DYNAMIC_SESSIONS) ? DS_PUSHPACKETS_IP_PACKET | DS_PUSHPACKETS_ENABLE_RFC7198_DEDUP : DS_PUSHPACKETS_IP_PACKET;
int chnum, rtp_pyld_type, rtp_pyld_len, push_cnt = 0;
int session_push_cnt[128] = { 0 };
static uint8_t queue_full_warning[MAX_SESSIONS] = { 0 };
uint64_t fp_sav_pos, wait_time;
int auto_adj_push_count = 0;
uint16_t hdr_type;
float msec_timestamp_fp = 0;

static uint64_t last_cur_time = 0, last_wait_check_time = 0, wait_pause = 0;  /* only used by master app thread */
static int waiting_inputs = 0;

pcaprec_hdr_t pcap_rec_hdr;
pcaprec_hdr_t* p_pcap_rec_hdr = NULL;

PKTINFO PktInfo;  /* struct in pktlib.h */

#if 0
  #define RATE_FORCE 1  /* force packet push rate to fixed interval (in msec).  This is reserved for stress test and debug purposes */
#endif
#ifdef RATE_FORCE
static int num_pcap_packets = 0;
#endif

   for (j=0; j<thread_info[thread_index].nInPcapFiles; j++) {

      if (thread_info[thread_index].pcap_in[j] != NULL) {

         if (Mode & AUTO_ADJUST_PUSH_RATE) {
            auto_adj_push_count = 0;
            if (!average_push_rate[thread_index]) goto push_ctrl;  /* dynamically adjust average push rate (APR) if auto-adjust push rate is enabled */
         }

next_packet:  /* next packet input */

         fp_sav_pos = ftell(thread_info[thread_index].pcap_in[j]);

         if (Mode & USE_PACKET_ARRIVAL_TIMES) p_pcap_rec_hdr = &pcap_rec_hdr;

         //#define STRESS_DEBUG
         #ifdef STRESS_DEBUG
         if (nRepeatsRemaining[thread_index] >= 0 && thread_info[thread_index].num_rtp_packets[j] > 1000) pkt_len = 0;
         else
         #endif

         uint16_t input_type = (thread_info[thread_index].link_layer_len[j] & PCAP_LINK_LAYER_FILE_TYPE_MASK) >> 16;

         if (input_type == PCAP_TYPE_LIBPCAP || input_type == PCAP_TYPE_PCAPNG || input_type == PCAP_TYPE_RTP) {  /* handle pcap formats here, BER and other file formats below */

         /* read next pcap record */

            pkt_len = DSReadPcap(thread_info[thread_index].pcap_in[j], pkt_buf, 0, p_pcap_rec_hdr, thread_info[thread_index].link_layer_len[j], &hdr_type, thread_info[thread_index].pcap_file_hdr[j]);

         /* process non-zero length packets */
  
            if (pkt_len > 0) {

            /* flags for DSGetPacketInfo() (see pktlib.h for detailed info) */

               unsigned int uFlags = DS_BUFFER_PKT_IP_PACKET |          /* basic IP packet */
                                     DS_PKT_INFO_PKTINFO |              /* fill PKTINFO struct */
                                     DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG;  /* if packet is malformed (invalid IP version, incorrect header, mismatching length, etc) return value is < 0 and a warning message will be printed by DSGetPacketInfo(). We use the DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG flag to suppress RTP related warning messages as the packet type is unknown at this point */

            /* check if pcap record is being re-read for some reason; examples include switching between input streams (ports or files) and arrival timestamps still ahead of the wall clock */

               if (!thread_info[thread_index].fReseek[j]) {

               /* increment packet number, notes:

                  -for pcaps this number can be printed to console or file at any time and compared with "packet number" in Wireshark display
                  -the final number displayed in mediaMin "total input packets" summary stat may be shorter than Wireshark if a SIP Bye message is found, or other reason the stream terminates before all packets are read
               */

                  thread_info[thread_index].packet_number[j]++;

               /* non-IP packet handling. Standard Ethernet Protocols are ETH_P_IP and ETH_P_IPV6 (defined in if_ether.h Linux header file) but a few others are common enough we look for them and ignore if found, JHB Dec 2021 */

                  if (hdr_type == ETH_P_ARP) {  /* ignore ARP packets (ETH_P_ARP is 0x0806 in if_ether.h) */
                     #ifdef PKT_DISCARD_DEBUG
                     printf(" ************* ignoring ARP pkt \n");
                     #endif
                     goto next_packet;
                  }

                  if (hdr_type >= 82 && hdr_type <= 1536) { /* ignore 802.2 LLC frames (https://networkengineering.stackexchange.com/questions/50586/eth-ii-vs-802-2-llc-snap). Note - added the lower range check of 82 after some .pcapng test files with Ethernet prototype value of zero were misinterpreted as 802.2, JHB Sep 2022 */
                     #ifdef PKT_DISCARD_DEBUG
                     printf(" ************* ignoring LLC frame, hdr_type = %d \n", hdr_type);
                     #endif
                     goto next_packet;
                  }

                  uFlags |= DS_PKT_INFO_FRAGMENT_SAVE;  /* instruct pktlib to save and manage packet fragments */
               }

            /* fill in PktInfo struct with IP items. UDP and RTP header items are also filled, depending on IP protocol(s) found in the packet. See the PKTINFO struct in pktlib.h */
 
               pkt_info_ret_val = DSGetPacketInfo(-1, uFlags, pkt_buf, -1, &PktInfo, NULL);

               #if 0  // debug
               if (pkt_info_ret_val & DS_PKT_INFO_RETURN_FRAGMENT_SAVED) printf("\n *** mediaMin packet fragment added, protocol = %d, pkt number = %d, pkt info ret val = 0x%x \n", PktInfo.protocol, thread_info[thread_index].packet_number[j], pkt_info_ret_val);
               #endif

               if (thread_info[thread_index].fReseek[j]) {  /* avoid repeating in reseek case */

                  thread_info[thread_index].fReseek[j] = false;
                  goto protocol_based_processing;
               }

               if (pkt_info_ret_val < 0) {

                  #ifdef PKT_DISCARD_DEBUG
                  printf("************* invalid IP version or malformed packet, pkt type = %d, pkt len = %d \n", hdr_type, pkt_len);
                  #endif
                  goto next_packet;
               }

               if (PktInfo.protocol != UDP && PktInfo.protocol != TCP) { /* ignore ICMP, ESP encrypted payloads, VRRP, and various other protocols. Contact Signalogic if you need help implementing a new protocol or have protocol-related suggestions for the developers. Examples might include new encapsulation protocols, new media codecs, etc. */

                  #ifdef PKT_DISCARD_DEBUG
                  printf(" ************* ignoring non UDP or TCP pkt with protocol = %d \n", PktInfo.protocol);
                  #endif

               /* remove any saved fragments for protocols that we're not going to process; e.g. ESP (protocol == ENCAPSULATING_SECURITY_PAYLOAD in pktlib.h) */

                  if (pkt_info_ret_val & DS_PKT_INFO_RETURN_FRAGMENT_SAVED) pkt_info_ret_val = DSGetPacketInfo(-1, (uFlags & ~DS_PKT_INFO_FRAGMENT_SAVE) | DS_PKT_INFO_FRAGMENT_REMOVE, pkt_buf, -1, NULL, NULL);

                  goto next_packet;
               }

            /* check for duplicated packets */

               if (isDuplicatePacket(&PktInfo, &thread_info[thread_index].PktInfo[j], j, thread_index)) {

               /* remove any duplicate saved packet fragments */

                  if (pkt_info_ret_val & DS_PKT_INFO_RETURN_FRAGMENT_SAVED) pkt_info_ret_val = DSGetPacketInfo(-1, (uFlags & ~DS_PKT_INFO_FRAGMENT_SAVE) | DS_PKT_INFO_FRAGMENT_REMOVE, pkt_buf, -1, NULL, NULL);

                  goto next_packet;  /* discard duplicated / redundant packets */
               }

               if (pkt_info_ret_val & DS_PKT_INFO_RETURN_FRAGMENT_SAVED) thread_info[thread_index].num_packets_fragmented[j]++;

               thread_info[thread_index].PktInfo[j] = PktInfo;  /* save packet info for duplicate packet comparison */

               if (PktInfo.protocol == TCP) thread_info[thread_index].num_tcp_packets[j]++;  /* increment per stream TCP packet count */
               else thread_info[thread_index].num_udp_packets[j]++;  /* increment per stream UDP packet count */

               if (nCut > 0) { nCut--; goto next_packet; }  /* handle --cut if entered on cmd line, JHB Nov 2023 */

               if (!(PktInfo.flags & DS_PKT_FRAGMENT_OFS)) {  /* save port info for non-fragmented packets and first fragment of fragmented packets */

                  thread_info[thread_index].dst_port[j] = PktInfo.dst_port;  /* save UDP or TCP ports, they are not present in intermediate fragments */
                  thread_info[thread_index].src_port[j] = PktInfo.src_port;
               }

            /* check for packet fragments */

               if ((pkt_info_ret_val & DS_PKT_INFO_RETURN_FRAGMENT) && !(pkt_info_ret_val & DS_PKT_INFO_RETURN_REASSEMBLED_PACKET_AVAILABLE)) goto next_packet;  /* fragments are managed in pktlib, so we wait until all fragments have arrived, then reassemble */

               #if 0  /* debug */
               PrintSIPInviteFragments(pkt_buf, &PktInfo, pkt_len);
               #endif

#if 0  /* example showing how to find/filter packet(s) for debug / stream manipulation. This example isn't doing anything to separate out SIP or other non-RTP packets, so calls DSGetPacketInfo() for RTP items may cause "DEBUG" or warning messages */
   static int nCount = 0;
   if (nCount < 2) {
      if (PktInfo.src_port != 5060 && PktInfo.dst_port != 5060) {

         if (PktInfo.rtp_pyld_type == 105) { nCount++; if (nCount == 2) printf("\n\n two 105 packets omitted \n"); goto next_packet; }  /* don't push first two packets found */
      }
   }
#endif

#if 0  /* another debug example showing how to find/filter packet(s) for stream manipulation  */
    __uint128_t src_ip_addr = 0;
   DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_SRC_ADDR, pkt_buf, -1, &src_ip_addr, NULL);
   if (/*src_ip_addr == 0xc0a88615 || src_ip_addr == 0x0a0e0742 || */ rtp_ssrc == 0xBBF51B9C && (/*rtp_pyld_len == 63 || rtp_pyld_len == 35 || */ PktInfo.rtp_pyld_len == 8)) {  /* 0xc0a88615 -> 192.168.134.21 */

      printf(" filtering packet with src ip addr 0x%llx != 192.168.134.21 or rtp ssrc 0x%x \n", (long long unsigned int)src_ip_addr, PktInfo.rtp_ssrc);
      goto next_packet;
   }
#endif

#if 0  /* filter RTP packets example, using SSRC(s) only */

   if (PktInfo.protocol == UDP && (PktInfo.dst_port < SIP_PORT_RANGE_LOWER || PktInfo.src_port > SIP_PORT_RANGE_UPPER) && pkt_len < 200) {  /* must be UDP, rough checks to avoid non-RTP packets */

      uint32_t filt_rtp_ssrc[] = { 0xe38e9ffc, 0xc87c0d25 };  /* SSRCs to allow -- examples only */
      bool fFilter = false;

      for (unsigned int i=0; i<sizeof(filt_rtp_ssrc)/sizeof(uint32_t); i++) if (PktInfo.rtp_ssrc != filt_rtp_ssrc[i]) { fFilter = true; break; }  /* search list of ssrcs to allow, anything that doesn't match break and filter it out */

      if (fFilter) {  /* filter ssrc */

         char tmpstr[200] = "\n *** filtering out packets with rtp ssrc ";
         static bool fPrintOnce = false;

         if (!fPrintOnce) { sprintf(&tmpstr[strlen(tmpstr)], "0x%x != ", PktInfo.rtp_ssrc); for (unsigned int i=0; i<sizeof(filt_rtp_ssrc)/sizeof(uint32_t); i++) sprintf(&tmpstr[strlen(tmpstr)], "%s0x%x", !i ? "" : ", ", filt_rtp_ssrc[i]); printf("%s\n", tmpstr); fPrintOnce = true; }

         goto next_packet;  /* filter this ssrc -- don't process its packet */
      }
   }
#endif

#if 0  /* packet search example, after finding a specific packet force a session change by altering a session Id value (in this case payload type) */

   if ((PktInfo.dst_port > SIP_PORT_RANGE_LOWER || PktInfo.src_port > SIP_PORT_RANGE_UPPER) && PktInfo.protocol == UDP) {  /* avoid SIP packets */
   
      uint32_t filt_rtp_ssrc[] = { 0x2f42ae };

      static bool fSessionChangeFound = false;

      for (unsigned int i=0; i<sizeof(filt_rtp_ssrc)/sizeof(uint32_t); i++) if (PktInfo.rtp_ssrc == filt_rtp_ssrc[i]) {  /* search list of ssrcs */

         if (!fSessionChangeFound && PktInfo.rtp_seqnum == 50980) {

            fSessionChangeFound = true;
            #if 1
            printf("\n *** for stream 0x%x seq num %d changing payload type value %d \n", PktInfo.rtp_ssrc, rtp_seqnum, PktInfo.rtp_pyld_type);
            #else
            printf("\n *** for stream 0x%x seq num %d changing source port value %d \n", PktInfo.rtp_ssrc, rtp_seqnum, PktInfo.src_port);
            #endif
         }

         if (fSessionChangeFound) pkt_buf[PktInfo.ip_hdr_len+UDP_HEADER_LEN+1] = ++PktInfo.rtp_pyld_type;  /* change payload type for all remaining packets in the target stream */
      }
   }
#endif
            }
         }
         else if (input_type == PCAP_TYPE_BER) {  /* experimental BER input handling, JHB Dec 2021 */

            //#define BER_DEBUG
            #ifdef BER_DEBUG
            printf("***** inside BER detect \n");
            #endif

            uint8_t ber_data[1024];

            pkt_len = fread(ber_data, sizeof(char), sizeof(ber_data), thread_info[thread_index].pcap_in[j]);

            if (pkt_len > 0 && (Mode & ENABLE_INTERMEDIATE_PCAP)) {  /* format BER data into TCP/IP packet and generate pcap output if specified in cmd line flags, JHB Dec 2021*/

               packet_actions(ber_data, pkt_buf, (PktInfo.protocol = TCP), &pkt_len, PCAP_TYPE_BER);
               pkt_info_ret_val = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTINFO, pkt_buf, -1, &PktInfo, NULL);  /* update PktInfo */
            }

         }  /* end of input_type checks */

         if (pkt_len == 0 || thread_info[thread_index].dynamic_terminate_stream[j]) {  /* packet stream ends (i) end of pcap file (ii) dynamic termination (e.g. SIP BYE message) (iii) UDP port closes. In the case of a pcap file we close it (or rewind if input repeat or certain types of stress tests are enabled) */

            bool fRepeat = false;

            if (!(Mode & CREATE_DELETE_TEST_PCAP) && !(Mode & REPEAT_INPUTS)) {  /* check for input repeat */

               if (thread_info[thread_index].pcap_in[j]) DSClosePcap(thread_info[thread_index].pcap_in[j]);  /* close the input file and set file handle to NULL so it's no longer operated on */
               thread_info[thread_index].pcap_in[j] = NULL;

               if (thread_info[thread_index].initial_push_time[j]) thread_info[thread_index].total_push_time[j] += cur_time - thread_info[thread_index].initial_push_time[j];  /* set total push time (if at least one push has happened, JHB Dec 2021) */
            }
            else {  /* in certain test modes or if input repeat is enabled, start the pcap over */

               bool fQueueEmpty = true;  /* wait for all stream group queues to be empty before rewinding the pcap, JHB Mar2020 */

               if ((Mode & ENABLE_STREAM_GROUPS) && (Mode & DYNAMIC_SESSIONS)) {  /* for dynamic sessions we wait for all sessions associated with the current input */

                  for (i=0; i<thread_info[thread_index].nSessions[j]; i++) if (!DSPullPackets(DS_PULLPACKETS_GET_QUEUE_STATUS | DS_PULLPACKETS_STREAM_GROUPS, NULL, NULL, hSessions[thread_info[thread_index].nSessionIndex[j][i]], NULL, 0, 0)) { fQueueEmpty = false; break; }
               }
               else if (!DSPullPackets(DS_PULLPACKETS_GET_QUEUE_STATUS | DS_PULLPACKETS_STREAM_GROUPS, NULL, NULL, -1, NULL, 0, 0)) fQueueEmpty = false;  /* for static sessions or if stream groups are not enabled, we wait for all sessions */

               if (!fQueueEmpty) continue;  /* not empty yet, move on to next input */

               if (thread_info[thread_index].initial_push_time[j]) thread_info[thread_index].total_push_time[j] += cur_time - thread_info[thread_index].initial_push_time[j];  /* set total push time (if at least one push has happened, JHB Dec 2021) */

            /* note that wrapping a pcap will typically cause warning messages about "large negative" timestamp and sequence number jumps, JHB Mar2020 */

               DSOpenPcap(NULL, &thread_info[thread_index].pcap_in[j], NULL, "", DS_READ | DS_OPEN_PCAP_READ_HEADER | DS_OPEN_PCAP_RESET);  /* seek to start of first pcap record */

               app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "mediaMin INFO: pcap %s wraps", MediaParams[thread_info[thread_index].input_index[j]].Media.inputFilename);

               if (thread_info[thread_index].num_rtp_packets[j]) fRepeat = true;  /* set repeat flag (note the check whether stream has produced at least one valid packet, we don't wanna end up in infinite pkt_len == 0 loop), JHB Dec 2021 */
            }

            thread_info[thread_index].initial_push_time[j] = 0;  /* reset initial push time */

            #if 0  /* with extended AFAP and FTRT mode support in place, we now display and log media time or processing time in all cases, JHB May 2023 */
            if (((Mode & USE_PACKET_ARRIVAL_TIMES) || RealTimeInterval[0] > 1) && isMasterThread) {
            #else
            if (isMasterThread) {
            #endif

               char tmpstr[200], proctimestr[50], mediatimestr[50], descripstr[50] = "media";
               uint64_t total_push_time = thread_info[thread_index].total_push_time[j];

               if (isAFAPMode || isFTRTMode) strcpy(descripstr, "processing");
               DSGetLogTimeStamp(proctimestr, sizeof(proctimestr), total_push_time, DS_EVENT_LOG_USER_TIMEVAL);  /* use DSGetLogTimeStamp() user-specified time value feature to format as hours:min:sec (hours not shown if not > 0), JHB Feb 2024 */
               sprintf(tmpstr, "=== mediaMin INFO: %sinput pcap[%d] %s time %s", !(Mode & USE_PACKET_ARRIVAL_TIMES) ? "estimated " : "", j, descripstr, proctimestr);

               if (isFTRTMode) {  /* show both processing and media time in FTRT mode, JHB Feb 2024 */
                  DSGetLogTimeStamp(mediatimestr, sizeof(mediatimestr), total_push_time*timeScale, DS_EVENT_LOG_USER_TIMEVAL);
                  sprintf(&tmpstr[strlen(tmpstr)], ", media time %s", mediatimestr);
               }

               app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, tmpstr);
               Log_RT(4 | DS_LOG_LEVEL_FILE_ONLY, tmpstr);
            }

            if (fRepeat) goto next_packet;

         }  /* end of stream termination checks */

      /* if pkt_len is zero we've reached end of this input:

         -tell pktlib to not expect more packets for sessions associated with this input, and move on to next input
         -note this disables the push packets elapsed time alarm, if enabled (DS_ENABLE_PUSHPACKETS_ELAPSED_TIME_ALARM flag in DEBUG_CONFIG struct item uDebugMode; see shared_include/config.h)
      */
  
         if (pkt_len == 0 || thread_info[thread_index].dynamic_terminate_stream[j]) {

            int nSessionIndex;
            for (i=0; i<thread_info[thread_index].nSessions[j]; i++) if ((nSessionIndex = thread_info[thread_index].nSessionIndex[j][i]) >= 0) DSPushPackets(DS_PUSHPACKETS_PAUSE_INPUT, NULL, NULL, &hSessions[nSessionIndex], 1);
            if (!thread_info[thread_index].total_sessions_created) thread_info[thread_index].dynamic_terminate_stream[j] |= STREAM_TERMINATE_INPUT_ENDS_NO_SESSIONS; /* stream terminates with no sessions created */
            continue;
         }

      /* start of protocol specific processing */

protocol_based_processing:

      /* DER encoded encapsulated stream processing, JHB Mar 2021 */

         if (Mode & ENABLE_DER_STREAM_DECODE) {  /* look for DER encoded streams if specified in cmd line */

            string szInterceptPointId(256, (char)0), szId(256, (char)0);  /* create null C++ strings we can give to either C++ or C functions, JHB May 2021 */
            uint16_t der_dst_port_list[MAX_DER_DSTPORTS] = { 0 };

            HDERSTREAM hDerStream = thread_info[thread_index].hDerStreams[j];

            if (hDerStream) DSGetDerStreamInfo(hDerStream, DS_DER_INFO_DSTPORT_LIST, der_dst_port_list);  /* DER stream already exists, initialize dest port list */

         /* DSFindDerStream() finds info at HI2 level, including 1) new DER streams, 2) new destination ports for an existing DER stream */

            #ifdef BER_DEBUG
            if (input_type == PCAP_TYPE_BER) printf("******** attempting to find intercept ID \n");
            #endif

            if ((Mode & ENABLE_ASN_OUTPUT) && thread_info[thread_index].hFile_ASN_XML[j] == NULL) thread_info[thread_index].hFile_ASN_XML[j] = fopen("HI_ASN_output.xml", "w");  /* open as write-only text */

            unsigned int uFlags = DS_DER_FIND_INTERCEPTPOINTID | DS_DER_FIND_DSTPORT | DS_DER_FIND_PORT_MUST_BE_EVEN;
            if (Mode & ENABLE_ASN_OUTPUT_DEBUG_INFO) uFlags |= DS_DECODE_DER_PRINT_ASN_DEBUG_INFO;

            if (DSFindDerStream(pkt_buf, uFlags, &szInterceptPointId[0], der_dst_port_list, thread_info[thread_index].hFile_ASN_XML[j]) > 0) {

               #ifdef BER_DEBUG
               if (input_type == PCAP_TYPE_BER) printf("******** found intercept ID \n");
               #endif

               if (!hDerStream) {  /* create DER stream if needed */

                  hDerStream = DSCreateDerStream(&szInterceptPointId[0], der_dst_port_list[0], 0);
                  if (hDerStream > 0) thread_info[thread_index].hDerStreams[j] = hDerStream; else hDerStream = (intptr_t)NULL;  /* to-do: add error handling */
               }
               else if (DSGetDerStreamInfo(hDerStream, DS_DER_INFO_INTERCEPTPOINTID, &szId[0]) > 0 &&  /* DER stream already exists, add port if interception point Ids match */
                        szInterceptPointId == szId) DSSetDerStreamInfo(hDerStream, DS_DER_INFO_DSTPORT_LIST, der_dst_port_list);
            }

            if (hDerStream) {  /* process DER encoded streams at HI3 level */

               uint8_t pkt_out_buf[MAX_RTP_PACKET_LEN] = { 0 };
               HI3_DER_DECODE der_decode = { 0 };
               int cc_pktlen;
               bool fFoundEncapsulatedCCPkt = false;

            /* DSDecodeDerStream() parses/decodes a DER stream looking for CC packets, and if found returns as fully formed IPv4/6 UDP packets */

               unsigned int uFlags = DS_DER_SEQNUM | DS_DER_TIMESTAMP | DS_DER_TIMESTAMPQUALIFIER | DS_DER_CC_PACKET;  /* tell DSDecodeDerStream() what to look for */

               if (Mode & ENABLE_DER_DECODING_STATS) uFlags |= DS_DECODE_DER_PRINT_DEBUG_INFO;

               if ((cc_pktlen = DSDecodeDerStream(hDerStream, pkt_buf, pkt_out_buf, uFlags, &der_decode, thread_info[thread_index].hFile_ASN_XML[j])) > 0) {  /* 0 means nothing found, < 0 is an error condition, > 0 is length of found packet */

                  pkt_len = cc_pktlen;  /* valid CC packet found, set new pkt_len and pkt_buf values for subsequent IP/UDP packet processing */
                  memcpy(pkt_buf, pkt_out_buf, pkt_len);
                  fFoundEncapsulatedCCPkt = true;

                  if ((Mode & USE_PACKET_ARRIVAL_TIMES) && (der_decode.uList & DS_DER_TIMESTAMP)) {  /* if valid timestamp found in decoded DER stream, use for packet arrival time */
                     p_pcap_rec_hdr->ts_sec = der_decode.timeStamp_sec.value;
                     p_pcap_rec_hdr->ts_usec = der_decode.timeStamp_usec.value;
                  }

                  if (der_decode.asn_index != 0) {

                    fseek(thread_info[thread_index].pcap_in[j], fp_sav_pos, SEEK_SET);  /* retain TCP stream packet until its encapsulated stream is consumed */
                    thread_info[thread_index].fReseek[j] = true;
                  }
 
//  uint64_t pkt_timestamp = (uint64_t)p_pcap_rec_hdr->ts_sec*1000000L + p_pcap_rec_hdr->ts_usec;
//  if (pkt_index) printf("updating pkt index = %d, prev index = %d, timestamp = %llu \n", pkt_index, thread_info[thread_index].EncapsulatedStreamIndex[j], (unsigned long long)pkt_timestamp);
               }

               if (der_decode.uList && !fFoundEncapsulatedCCPkt) continue;  /* found one or more DER items but not a CC packet, move to next input (socket or pcap) */

               if (fFoundEncapsulatedCCPkt) {  /* CC UDP packet found, set protocol to UDP/RTP, update PktInfo */

                  pkt_info_ret_val = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTINFO, pkt_buf, -1, &PktInfo, NULL);  /* note we allow error messages here because we expect UDP/RTP and would like to know if that's not the case ... might need to change this, JHB Jan 2023*/

                  if (PktInfo.protocol != UDP) Log_RT(3, "mediaMin WARNING: DER decoded packet not UDP format \n");
                  else thread_info[thread_index].num_packets_encapsulated[j]++;
               }

               if ((Mode & ENABLE_INTERMEDIATE_PCAP) && fFoundEncapsulatedCCPkt) packet_actions(NULL, pkt_buf, PktInfo.protocol, &pkt_len, PCAP_TYPE_HI3);  /* write out decoded DER stream to intermediate pcap; note this currently only includes decoded UDP packets, JHB Dec 2021 */
            }
         }  /* end of DER encapsulation processing (includes HI2/HI3) */


      /* TCP handling. mediaMin SDK version currently ignores TCP/IP packets other than encapsulated DER streams and SIP invites */

         if (PktInfo.protocol == TCP) {

         /* look for SIP invite packets */

            if (ProcessSessionControl(pkt_buf, SESSION_CONTROL_ADD_SIP_INVITE_SDP_INFO | ((!(Mode & DISABLE_SIP_INFO_REQUEST_OK_MESSAGES) || !fFirstConsoleMediaOutput) ? SESSION_CONTROL_ALL_MESSAGES : SESSION_CONTROL_SIP_INVITE_MESSAGES | SESSION_CONTROL_SIP_BYE_MESSAGES), j, thread_index, NULL) == SESSION_CONTROL_FOUND_SIP_INVITE) {  /* ProcessSessionControl() in sdp_app.cpp will show messages if Invites are invalid, repeats of existing session IDs, or valid and added to thread_info[] SDP database. Return value is type of SIP message found, if any, JHB Jan 2023 */

               uint16_t der_dst_port_list[MAX_DER_DSTPORTS] = { 0 };
               HDERSTREAM hDerStream;
               int i = 0;

               if ((hDerStream = thread_info[thread_index].hDerStreams[j])) {

                  DSGetDerStreamInfo(hDerStream, DS_DER_INFO_DSTPORT_LIST, der_dst_port_list);
                  for (i=0; i<MAX_DER_DSTPORTS; i++) if (thread_info[thread_index].dst_port[j] > 0 && thread_info[thread_index].dst_port[j] == der_dst_port_list[i]) break;
               }

               if ((Mode & ENABLE_DER_DECODING_STATS) && (!hDerStream || i < MAX_DER_DSTPORTS)) {

                  char szDerStream[20] = "";
                  if (hDerStream) sprintf(szDerStream, "DER stream %d ", hDerStream);
                  app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, " ==== %sTCP packet, not processed, pyld len = %d, dst port = %u \n", szDerStream, PktInfo.pyld_len, thread_info[thread_index].dst_port[j]);
               }
            }

            goto next_packet;  /* continue reading from this input, JHB Dec 2020 */
         }

         if (!thread_info[thread_index].initial_push_time[j]) {

            thread_info[thread_index].initial_push_time[j] = cur_time;
            #ifdef RATE_FORCE
            num_pcap_packets = 0;
            #endif
         }

         if (Mode & USE_PACKET_ARRIVAL_TIMES) {

            uint64_t pkt_timestamp, elapsed_time;  /* passed param cur_time, pkt_timestamp, and elapsed_time are in usec */
            uint32_t msec_timestamp, msec_elapsedtime;

            pkt_timestamp = (uint64_t)p_pcap_rec_hdr->ts_sec*1000000L + p_pcap_rec_hdr->ts_usec;

            if (!thread_info[thread_index].pkt_base_timestamp[j]) thread_info[thread_index].pkt_base_timestamp[j] = pkt_timestamp;  /* save initial "base" timestamp, which can be zero or some other value like the "epoch" (Jan 1 1970) */

            pkt_timestamp -= thread_info[thread_index].pkt_base_timestamp[j];  /* subtract base timestamp */

            #ifndef RATE_FORCE
            msec_timestamp = (pkt_timestamp + 500)/1000;  /* calculate in msec, with rounding. This compensates for jitter in how long it takes the main loop to repeat and get back to this point to push another packet and helps provide repeatable results, JHB Jan2020 */
            msec_timestamp_fp = 1.0*pkt_timestamp/1000;
            #else
            msec_timestamp = (num_pcap_packets+1)*RATE_FORCE;  /* debug/stress testing:  push packet either at a specific interval or at random intervals */
            #endif

            elapsed_time = timeScale * (cur_time - thread_info[thread_index].initial_push_time[j]);  /* subtract initial time to get elapsed_time, timeScale > 1 accelerates time in FTRT mode */

            msec_elapsedtime = (elapsed_time + 500)/1000;  /* calculate in msec, with rounding */

            if (msec_elapsedtime < msec_timestamp) {  /* push packet when elapsed time >= packet timestamp */

               bool fReseek = true;

               if (isMasterThread) {

                  if (((wait_time = msec_timestamp - msec_elapsedtime) > 2000 || last_wait_check_time) && msec_elapsedtime - last_wait_check_time > 1000) {

                     if (PktInfo.protocol == UDP && (PktInfo.rtp_pyld_type >= RTCP_PYLD_TYPE_MIN && PktInfo.rtp_pyld_type <= RTCP_PYLD_TYPE_MAX)) fReseek = false;  /* don't allow RTCP packets to restart a pause, verify with 6537.0 test-case. To-do: there are pauses in other payload type ranges also, we can add later, JHB Jun 2023 */
                     else {

                        if (!wait_pause) wait_pause = wait_time;

                        if (++waiting_inputs >= thread_info[thread_index].nInPcapFiles) {  /* print waiting status only if all inputs are waiting */
                           if (wait_time/1000 > 0) app_printf(APP_PRINTF_SAME_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "%sWaiting %llu of %llu sec pause in packet arrival times ...", (isCursorMidLine && !last_wait_check_time) ? "\n" : "\r", (long long unsigned int)wait_time/1000, (long long unsigned int)wait_pause/1000);
                           last_wait_check_time = msec_elapsedtime;  waiting_inputs = 0;
                        }
                     }
                  }
               }

               if (fReseek) {

                  fseek(thread_info[thread_index].pcap_in[j], fp_sav_pos, SEEK_SET);  /* packet arrival time not yet reached, restore file position */
                  thread_info[thread_index].fReseek[j] = true;
               }

               continue;  /* move on to next input */
            }

         /* reassemble fragmented packet if needed */

            if (pkt_info_ret_val & DS_PKT_INFO_RETURN_REASSEMBLED_PACKET_AVAILABLE) {  /* if a fully reassembled packet is available then read whole packet into pkt_buf and update pkt_len and PktInfo */

               pkt_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTINFO | DS_PKT_INFO_REASSEMBLY_GET_PACKET | DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG, pkt_buf, -1, &PktInfo, NULL);  /* notes - (i) pkt_buf will be overwritten with reassembled packet data, (ii) PktInfo fields pkt len, UDP len, and RTP payload len will be updated */

               if (pkt_len > 0) thread_info[thread_index].num_packets_reassembled[j]++;

               #if 0
               printf("\n *** mediaMin reassembled packet number = %d length = %d, src port = %d, dst port = %d \n", thread_info[thread_index].packet_number[j], pkt_len, thread_info[thread_index].src_port[j], thread_info[thread_index].dst_port[j]);
               #endif
            }

         /* proceed with packet processing */

            #ifdef SHOW_PUSH_STATS
            static int fcount = 0;
            if (fcount < 10) { fcount++; printf("\n *** timeScale = %4.2f, elapsed time = %llu, pkt_timestamp = %llu, msec_elapsedtime = %lu, msec_timestamp = %lu \n", timeScale, (long long unsigned int)elapsed_time, (long long unsigned int)pkt_timestamp, (long unsigned int)msec_elapsedtime, (long unsigned int)msec_timestamp); }
            #endif

            if (last_wait_check_time) { last_wait_check_time = 0; wait_pause = 0; waiting_inputs = 0; }  /* any input not waiting resets wait status */

            #ifdef RATE_FORCE
            num_pcap_packets++;
            #endif

            #if 0  /* packet timestamp / push timing debug */
            static uint64_t last_push_time = 0;
            int max_push_interval = 0;

            if ((int)(msec_elapsedtime - last_push_time) > max_push_interval) {

               max_push_interval = (int)(msec_elapsedtime - last_push_time);
               printf("\n ! pushing packet in packet arrival time mode, pkt timestamp = %2.1f, push delta = %d, elapsed time = %llu packet timestamp = %llu\n", pkt_timestamp/1000.0, max_push_interval, (unsigned long long)msec_elapsedtime, (unsigned long long)msec_timestamp);
            }

            last_push_time = msec_elapsedtime;
            #endif
         }

         #define FILTER_UDP_PACKETS
         #ifdef FILTER_UDP_PACKETS  /* filter UDP packets */

         #define MAX_PKT_IGNORE_COUNT 16
         char szKeyword[50] = "";
         int nMsgTypeFound = -1;
         static int pkt_ignore_count = 0;
         static unsigned int last_ignore_str_len = 0;
         static int pkt_ignore_nums[MAX_PKT_IGNORE_COUNT], pkt_ignore_lens[MAX_PKT_IGNORE_COUNT], pkt_ignore_flags[MAX_PKT_IGNORE_COUNT];
         bool fSIP = false;
         int nShowPorts = 0, nPortAllowedStatus;
         uint16_t dst_port = thread_info[thread_index].dst_port[j];  /* get saved UDP dst and src ports */
         uint16_t src_port = thread_info[thread_index].src_port[j];

         #if 0  /* example of printing a range of packet buffers to compare with Wireshark display */
         if (thread_info[thread_index].packet_number[j] >= 648 && thread_info[thread_index].packet_number[j] <= 655) {
            char tmpstr[300];
            sprintf(tmpstr, "\n *** before UDP port checks, pkt number = %d, pkt len = %d, pyld ofs = %d, frag flags = 0x%x, dst port = %u, buf start[ \n", thread_info[thread_index].packet_number[j], pkt_len, PktInfo.pyld_ofs, PktInfo.flags, dst_port);
            PrintPacketBuffer(&pkt_buf[PktInfo.pyld_ofs], PktInfo.pyld_len, tmpstr, " *** buf end] \n");
         }
         #endif

      /* SIP message, SIP Invite, SAP and other SDP Info port handling */

         nPortAllowedStatus = isPortAllowed(dst_port, pkt_buf, pkt_len, j, thread_index);  /* isPortAllowed() will come back with any media or SIP port exceptions to the standard port range. PORT_ALLOW_xxx definitions are in mediaMin.h */

      /* if dst port is on media port allow list or discovered in SDP media info then we look no further and submit the packet for RTP processing, JHB Jan 2023 */

         if (nPortAllowedStatus == PORT_ALLOW_ON_MEDIA_ALLOW_LIST || nPortAllowedStatus == PORT_ALLOW_SDP_MEDIA_DISCOVERED) goto rtp_packet_processing;

         if (dst_port < NON_DYNAMIC_UDP_PORT_RANGE || isReservedUDP(dst_port)) {  /* ignore non-dynamic and reserved UDP ports */

            nShowPorts = 3;

            #ifdef FORCE_IGNORE
            nPortAllowedStatus = 0;
            #endif

            if (nPortAllowedStatus == PORT_ALLOW_UNKNOWN) goto ignore_udp_packet;  /* unknown - ignore it */
            else if (nPortAllowedStatus == PORT_ALLOW_KNOWN) goto next_packet;     /* known type but not media or SDP info - display/log it (isPortAllowed() does that), then move on to next packet */
            else if (nPortAllowedStatus == PORT_ALLOW_SDP_INFO) goto sip_check;    /* known and is carrying SDP info (SIP Invite, SAP protocol, etc) */
            else {}                                                                /* fall through and process it */
         }
         else if ((dst_port >= SIP_PORT_RANGE_LOWER && dst_port <= SIP_PORT_RANGE_UPPER) || dst_port == SAP_PORT || nPortAllowedStatus == PORT_ALLOW_SDP_INFO) {  /* see if dst port is (i) within range of commonly used SIP ports, (ii) known to carry SDP info (such as GTP), JHB Jun 2024 */

sip_check:
            fSIP = true;
            nShowPorts = 2;

         /* ProcessSessionControl() in sdp_app.cpp, notes:

             -accepts flags specifying types of SDP info to find, including SIP Invite messages and SAP/SDP protocol payloads. In the usage below we set uFlags SESSION_CONTROL_ADD_SIP_INVITE_SDP_INFO and SESSION_CONTROL_ADD_SAP_SDP_INFO
             -accepts flags specifying other types of SIP messages to find and report
             -handles message content split across multiple packets
             -displays/logs session ID related messages including (i) invalid ID (ii) repeat of existing ID (iii) valid + new ID added to thread_info[].xx[stream] SDP database
             -returns type of message found (see sdp_app.h)
             -see sdp_app.cpp for detailed comments and info
         */

            if ((nMsgTypeFound = ProcessSessionControl(pkt_buf, ((!(Mode & DISABLE_SIP_INFO_REQUEST_OK_MESSAGES) || !fFirstConsoleMediaOutput) ? SESSION_CONTROL_ALL_MESSAGES : SESSION_CONTROL_SIP_INVITE_MESSAGES | SESSION_CONTROL_SIP_BYE_MESSAGES) | ((Mode & ENABLE_STREAM_SDP_INFO) ? SESSION_CONTROL_ADD_SIP_INVITE_SDP_INFO | SESSION_CONTROL_ADD_SAP_SDP_INFO : 0), j, thread_index, szKeyword)) > 0) {

               switch (nMsgTypeFound) {

                  case SESSION_CONTROL_FOUND_SIP_INVITE:  /* do something additional with SIP invites if needed */
                  break;

                  case SESSION_CONTROL_FOUND_SAP_SDP:
                  break;

                  case SESSION_CONTROL_FOUND_SIP_OK:
                  break;

                  case SESSION_CONTROL_FOUND_SIP_BYE:

                     if (!(Mode & DISABLE_TERMINATE_STREAM_ON_BYE)) {  /* default is to act on SIP BYE messages embedded in the stream, unless command line flag is set to disable */

                        thread_info[thread_index].dynamic_terminate_stream[j] = STREAM_TERMINATE_BYE_MESSAGE;
                        Log_RT(4, "mediaMin INFO: terminating stream %d due to BYE message \n", j);
                     }
                  break;
               }

               pkt_ignore_count = 0;  /* reset packet ignore state machine */
               last_ignore_str_len = 0;
            }
            else {

ignore_udp_packet:

               if (!(Mode & DISABLE_PORT_IGNORE_MESSAGES) || !fFirstConsoleMediaOutput) {

                  pkt_ignore_nums[pkt_ignore_count] = thread_info[thread_index].packet_number[j];  /* implement a limited packet history so we can see fragmented packet sequences for ignored / unrecognized UDP ports, JHB Jun 2024 */
                  pkt_ignore_lens[pkt_ignore_count] = pkt_len;
                  pkt_ignore_flags[pkt_ignore_count] = PktInfo.flags;
                  pkt_ignore_count++;

                  char tmpstr[300], pkt_ignore_str[20], frag_flags_str[300], pkt_num_str[300], pkt_len_str[300], port_str[50] = "", search_str[50] = "";

                  sprintf(pkt_ignore_str, " (%d)", pkt_ignore_count);

                  sprintf(pkt_num_str, "pkt number%s", pkt_ignore_count > 1 ? "s" : "");
                  sprintf(pkt_len_str, "pkt len%s", pkt_ignore_count > 1 ? "s" : "");
                  sprintf(frag_flags_str, "frag flags");

                  for (int i=0; i<min(pkt_ignore_count, MAX_PKT_IGNORE_COUNT); i++) {
                     sprintf(&pkt_num_str[strlen(pkt_num_str)], " %d", pkt_ignore_nums[i]);
                     sprintf(&pkt_len_str[strlen(pkt_len_str)], " %d", pkt_ignore_lens[i]);
                     sprintf(&frag_flags_str[strlen(frag_flags_str)], " 0x%x", pkt_ignore_flags[i]);
                  }

                  if (nShowPorts) sprintf(port_str, ", ");
                  if (nShowPorts == 3) sprintf(&port_str[strlen(port_str)], "dst port = %d, src port = %d", dst_port, src_port);
                  else if (nShowPorts == 2) sprintf(&port_str[strlen(port_str)], "dst port = %d", dst_port);
                  else if (nShowPorts == 1) sprintf(&port_str[strlen(port_str)], "src port = %d", src_port);

                  if (fSIP && strlen(szKeyword)) sprintf(search_str, ", last keyword search = \"%s\"", szKeyword);

                  sprintf(tmpstr, "%signoring %s%s packet%s%s, %s, %s, %s%s%s", (isCursorMidLine && !pkt_ignore_count) ? "\n" : "\r", (PktInfo.protocol == TCP) ? "TCP" : "UDP", fSIP ? " SIP" : "", pkt_ignore_count > 1 ? "s" : "", pkt_ignore_str, pkt_num_str, pkt_len_str, frag_flags_str, port_str, search_str);  /* note we use \r to avoid new lines and using up screen to report ignored UDP ports, also this makes sure the message and port number is visible, JHB Feb 2023. Show either destination port or fragmentation flags; port is not reliable in fragmented packets, JHB Jun 2024 */

                  char tmpstr2[400];
                  sprintf(tmpstr2, !fSIP && (strlen(tmpstr) > last_ignore_str_len || (frac(log10(pkt_ignore_count)) == 0.0)) ? ". To allow port use -pN option or add to UDP_Port_Media_Allow_List[]" : (fSIP ? "    " : ""));

                  last_ignore_str_len = strlen(tmpstr);

                  app_printf(APP_PRINTF_SAME_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "%s%s", tmpstr, tmpstr2);
               }
            }

            goto next_packet;  /* move on to next packet */
         }
         else if (  /* if source port in UDP SIP range we look for and display SIP messages but don't act on them, JHB Mar 2023 */
                  (src_port >= SIP_PORT_RANGE_LOWER && src_port <= SIP_PORT_RANGE_UPPER) || src_port == SAP_PORT ||
                  (nPortAllowedStatus = isPortAllowed(src_port, pkt_buf, pkt_len, j, thread_index)) == PORT_ALLOW_SDP_INFO
                 ) {

            fSIP = true;
            nShowPorts = 1;

            if ((nMsgTypeFound = ProcessSessionControl(pkt_buf, SESSION_CONTROL_NO_PARSE | ((!(Mode & DISABLE_SIP_INFO_REQUEST_OK_MESSAGES) || !fFirstConsoleMediaOutput) ? SESSION_CONTROL_ALL_MESSAGES : 0), j, thread_index, szKeyword)) > 0) {

               pkt_ignore_count = 0;
               last_ignore_str_len = 0;
               goto next_packet;
            }
            else goto ignore_udp_packet;
         }

/* from this point we assume we're working with RTP media packets, although there may still be exceptions */

rtp_packet_processing:

         pkt_ignore_count = 0;
         last_ignore_str_len = 0;

         bool fShowWarnings = (Mode & ENABLE_DEBUG_STATS) != 0;

         thread_info[thread_index].num_rtp_packets[j]++;  /* increment per stream RTP packet count */

         rtp_pyld_type = PktInfo.rtp_pyld_type;

         if (rtp_pyld_type < 0 && fShowWarnings) {
            Log_RT(4, "mediaMin INFO: PushPackets() says unknown UDP packet; DSGetPacketInfo() did not find valid RTP payload type, dst port = %u, pkt len = %d \n", thread_info[thread_index].dst_port[j], pkt_len);
            goto next_packet;
         }
         #endif  /* FILTER_UDP_PACKETS */

         #define FILTER_RTCP_PACKETS_IF_rN_TIMING
         #ifdef FILTER_RTCP_PACKETS_IF_rN_TIMING

      /* ignore RTCP packets in some cases. Notes, Feb 2019 JHB:

         -RTCP packets are already filtered by packet/media threads ** but if the push rate is 2 msec or faster then we filter here to avoid FlushCheck() prematurely seeing empty queues and flushing the session (is this still needed ? JHB Jun 2023)
         -session flush for USE_PACKET_ARRIVAL_TIMES mode is not dependent on empty queues, so it's excluded
         -** packet_flow_media_proc() applies the DS_RECV_PKT_FILTER_RTCP flag in DSRecvPackets()
         -as a side note, repetitive RTCP packets have been observed in multisession flows with long on-hold or call-waiting periods (one or more streams are not sending RTP)
      */
         if ((rtp_pyld_type >= RTCP_PYLD_TYPE_MIN && rtp_pyld_type <= RTCP_PYLD_TYPE_MAX) && RealTimeInterval[0] > 1 && !(Mode & USE_PACKET_ARRIVAL_TIMES)) goto next_packet;  /* move on to next packet */
         #endif

         rtp_pyld_len = PktInfo.rtp_pyld_len;

         if (rtp_pyld_len <= 0 && fShowWarnings) {
            Log_RT(4, "mediaMin INFO: PushPackets() says unknown UDP packet; DSGetPacketInfo() says invalid RTP payload size %d, dst port = %u, pkt len = %d \n", rtp_pyld_len, thread_info[thread_index].dst_port[j], pkt_len);
            goto next_packet;
         }

      /* push packets using DSPushPackets() API in pktlib:

         -if dynamic sessions are enabled, look for new sessions -- we find IP headers that have not occurred before and hash them to create a unique key. New session handling performs auto-detection of codec type
         -look for DTMF, filter RTCP, etc
         -if session reuse is active, we modify headers to ensure they are unique (also this is done if we find duplicated inputs on the cmd line)
         -note we go through the n<nReuseInput loop once in normal operation (i.e. nReuseInputs == 0). Basically remaining PushPackets() is within the reuse loop, except for push rate auto-adjust. Yes it's a tad confusing
      */
         
         for (n=0; n<nReuseInputs+1; n++) {  /* default value of nReuseInputs is zero, unless entered on cmd line with -nN.  For nReuseInputs > 0 we reuse each input N times */

check_for_duplicated_headers:

            if (n > 0 || thread_info[thread_index].fDuplicatedHeaders[j]) {  /* modify packet header slightly for each reuse, so all packets in a reused stream look different than other streams.  Notes:

                                                                                1) increment the src UDP port and decrement the dst UDP port to reduce chances of inadvertently duplicating another session
                                                                                2) also increment SSRC to avoid packet/media thread "dormant SSRC" detection (SSRC is not part of key that mediaMin uses to keep track of unique sessions)
                                                                             */

               unsigned int src_udp_port, dst_udp_port, ip_hdr_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_HDRLEN, pkt_buf, pkt_len, NULL, NULL);

               memcpy(&src_udp_port, &pkt_buf[ip_hdr_len], 2);
               memcpy(&dst_udp_port, &pkt_buf[ip_hdr_len+2], 2);
               src_udp_port++;
               dst_udp_port--;
               memcpy(&pkt_buf[ip_hdr_len], &src_udp_port, 2);
               memcpy(&pkt_buf[ip_hdr_len+2], &dst_udp_port, 2);

               unsigned int rtp_hdr_ofs = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_HDROFS, pkt_buf, pkt_len, NULL, NULL);
               unsigned int ssrc = ((unsigned int)pkt_buf[rtp_hdr_ofs+11] << 24 | (unsigned int)pkt_buf[rtp_hdr_ofs+10] << 16 | (unsigned int)pkt_buf[rtp_hdr_ofs+9] << 8 | pkt_buf[rtp_hdr_ofs+8]);
               ssrc++;
               pkt_buf[rtp_hdr_ofs+11] = ssrc >> 24; pkt_buf[rtp_hdr_ofs+10] = (ssrc >> 16) & 0xff; pkt_buf[rtp_hdr_ofs+9] = (ssrc >> 8) & 0xff; pkt_buf[rtp_hdr_ofs+8] = ssrc & 0xff;
            }

            bool fNewSession = false, fInitialStaticSession = ((Mode & CREATE_DELETE_TEST_PCAP) && debug_test_state == CREATE);

            if (thread_info[thread_index].fDynamicSessions || fInitialStaticSession) {

               int nSessionsFound = FindSession(pkt_buf, pkt_len, rtp_pyld_type, rtp_pyld_len, thread_index);  /* FindSession() returns > 0 if it finds a new session */

               if (nSessionsFound > 0 && !(fInitialStaticSession && nSessionsFound == 1)) {  /* for a test mode with an initial static session we ignore the first new session found */

                  int ret_val = CreateDynamicSession(pkt_buf, pkt_len, hSessions, session_data, thread_index, j, n);

                  if (ret_val > 0) {

                     app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_PRINT_ONLY, thread_index, "+++++++++Created dynamic session #%d, total sessions created %d", thread_info[thread_index].nSessionsCreated, thread_info[thread_index].total_sessions_created);

                     nSessions++;
                     fNewSession = true;
                     if (!fFirstConsoleMediaOutput) fFirstConsoleMediaOutput = true;
                  }
                  else {  /* error or problem of some type in CreateDynamicSession(), remove the key created by FindSession() */

                     nKeys[thread_index]--;
                     memset(keys[thread_index][nKeys[thread_index]], 0, KEY_LENGTH);

                     if (ret_val == -2) { thread_info[thread_index].init_err = true; return -1; }
                  }
               }
               else {  /* already existing session */

                  if (!(Mode & COMBINE_INPUT_SPECS) && !thread_info[thread_index].nSessions[j] && !thread_info[thread_index].fDuplicatedHeaders[j]) { /* if we find duplicated inputs on the cmd line, we slightly modify IP headers of each successive one so they create new sessions / stream groups (this is a hack for mult-input cmd line stress testing), JHB Jan2020 */

                     int l;
                     for (l=0; l<thread_info[thread_index].nInPcapFiles; l++) {

                        if (l != j && thread_info[thread_index].nSessions[l]) {

                           app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_PRINT_ONLY, thread_index, "++++++++ Cmd line input #%d IP headers are duplicates of cmd line input #%d, modifying headers for input #%d", j+1, l+1, j+1);

                           thread_info[thread_index].fDuplicatedHeaders[j] = true;
                           goto check_for_duplicated_headers;
                        }
                     }
                  }
               }
            }

            int nFirstSession = -1;

            for (i=0; i<nSessions; i++) {

               if (hSessions[i] & SESSION_MARKED_AS_DELETED) continue;  /* hSessions[] entry may be marked as already deleted */

               chnum = DSGetPacketInfo(hSessions[i], DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_CHNUM_PARENT | DS_PKTLIB_SUPPRESS_ERROR_MSG, pkt_buf, pkt_len, NULL, NULL);  /* get the stream's parent chnum (ignore SSRC) */ 

               #define CHECK_RTP_PAYLOAD_TYPE
               #ifdef CHECK_RTP_PAYLOAD_TYPE  /* this is a special case useful for checking duplicated sessions that differ only in RTP payload type.  It doesn't handle the general case of exactly duplicated sessions */
               if (chnum >= 0 && rtp_pyld_len != 4) {  /* don't check DTMF packets */

                  int rtp_pyld_type_term = -1;
                  rtp_pyld_type = PktInfo.rtp_pyld_type;
                  #if 0
                  rtp_pyld_type_term = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_RTP_PAYLOAD_TYPE, DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_TERM, 0, NULL), NULL);
                  #else  /* this way is faster and also demonstrates convenient use of SESSION_DATA structs after session creation */
                  int term = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_TERM, 0, NULL);
                  if (term == 1) rtp_pyld_type_term = session_data[i].term1.attr.voice_attr.rtp_payload_type;
                  else if (term == 2) rtp_pyld_type_term = session_data[i].term2.attr.voice_attr.rtp_payload_type;
                  #endif

                  if (rtp_pyld_type_term != rtp_pyld_type) chnum = -1;
               }
               #endif

               if (chnum >= 0) {  /* if packet matches a stream (i.e. a term defined for a session), push to correct session queue.  Note that SSRC is not included in the session match because DSGetPacketInfo() was called with DS_PKT_INFO_CHNUM_PARENT */

                  #if 0
                  if (rtp_pyld_len < 4 && fShowWarnings) {

                     int chnum2 = DSGetPacketInfo(hSessions[i], DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_CHNUM | DS_PKTLIB_SUPPRESS_ERROR_MSG, pkt_buf, pkt_len, NULL, NULL);  /* get the stream's chnum (either parent or child) */ 
                     Log_RT(4, "mediaMin INFO: PushPackets() says dropping RTP packet that matches session %d channel %d with payload size %d < 4, SSRC = 0x%x, seq num = %d \n", hSessions[i], chnum2, rtp_pyld_len, PktInfo.rtp_ssrc, PktInfo.rtp_seqnum);
                     continue;  /* don't push this packet */
                  }
                  #endif

                  if (nFirstSession == -1) nFirstSession = hSessions[i];
                  else app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "######### Two pushes for same packet, nFirstSession = %d, hSession = %d, chnum = %d", nFirstSession, hSessions[i], chnum);  /* this should not happen, if it does call attention to it.  If it occurs, it means there are exactly duplicated sessions, including RTP payload type, and we need more information to differentiate */

                  int retry_count = 0;

                  #ifdef FIRST_TIME_TIMING  /* reserved for timing debug purposes */
                  static bool fSync = false;
                  if (!fSync && !fStressTest && !fCapacityTest) { PmThreadSync(thread_index); fSync = true; }  /* sync between app thread and master p/m thread. This removes any timing difference between starting time of application thread vs. p/m thread */

                  static bool fOnce = false;
                  if (!fOnce) { printf("\n === time to first push %llu \n", (unsigned long long)((first_push_time = get_time(USE_CLOCK_GETTIME)) - base_time)); fOnce = true; }
                  #endif

                  if (fNewSession) thread_info[thread_index].nSessionIndex[j][thread_info[thread_index].nSessions[j]++] = i;  /* increment total number of sessions, maintain an index into hSessions[] for each session in the input */

push:

                  #if 1  /* temporary during test and development: not pushing video packets */
                  if (session_data[i].term1.codec_type > DS_VOICE_NCODECS && session_data[i].term1.codec_type < DS_CODEC_TYPE_INVALID) {
                     // printf("\n *** not pushing H.26x packet, len = %d \n", pkt_len);
                     continue;
                  }
                  #endif

                  int ret_val = DSPushPackets(uFlags_push, pkt_buf, &pkt_len, &hSessions[i], 1);  /* push packet to packet/media thread queue */

                  if (!(Mode & DYNAMIC_SESSIONS) && (ret_val & DS_PUSHPACKETS_ENABLE_RFC7198_DEDUP)) goto next_packet;  /* duplicate packet, move on to next packet */

                  if (!ret_val) {  /* push queue is full, try waiting and pushing the packet again */

                     uint32_t uSleepTime = max(1000, (int)(RealTimeInterval[0]*1000));
                     usleep(uSleepTime);

                     if (retry_count++ < 3) goto push;  /* max retries */
                     else {

                     /* not good if this is happening so we print to event log. But we don't want to overrun the log and/or screen with warnings, so we have a warning counter */

                        if (!queue_full_warning[hSessions[i]]) Log_RT(3, "mediaMin WARNING: says DSPushPackets() timeout, unable to push packet for %d msec \n", (retry_count-1)*uSleepTime/1000);
                        queue_full_warning[hSessions[i]]++;  /* will wrap every 255 */
                     }

                     fseek(thread_info[thread_index].pcap_in[j], fp_sav_pos, SEEK_SET);  /* unable to push this packet, restore pcap file pointer and return 0 (no packets pushed) */
                     return 0;
                  }
                  else if (ret_val == -1) {  /* error condition */

                     fprintf(stderr, "Error condition returned by DSPushPackets, hSession = %d, pkt_len = %d\n", hSessions[i], pkt_len);
                     return -1;
                  }
                  else {  /* packet was successfully pushed */

                     #if 0  /* move to before "push:" - session indexing does not depend on successful packet pushing. Once a session is created we need track it, JHB Jun 2024 */
                     if (fNewSession) thread_info[thread_index].nSessionIndex[j][thread_info[thread_index].nSessions[j]++] = i;  /* maintain an index into hSessions[] for each session in the input */
                     #endif

                     session_push_cnt[i]++;
                     thread_info[thread_index].pkt_push_ctr++;
                     push_cnt++;

                     if (queue_full_warning[hSessions[i]]) queue_full_warning[hSessions[i]] = 0;  /* reset queue full warning if needed */

                  /* if specified, calculate packet arrival timing stats, JHB Aug 2023:
                   
                     -only calculate for successive media packets and SID packets that immediately follow a media packet, and omit consecutive SID, DTMF packets
                     -the objective is to measure delta and jitter with respect to ptime, and get a handle on whether packet rate is consistently fast or slow. Wireshark averages in SID packets which obscures accurate reading on things like media playout servers
                     -currently we have a 1/2 sec limit on gaps due to packet loss or input pauses, anything larger than that is not counted
                     -to-do: add to APP_THREAD_INFO struct, capacity tests
                  */

                     if ((Mode & USE_PACKET_ARRIVAL_TIMES) && (Mode & SHOW_PACKET_ARRIVAL_STATS)) {

                        float delta_timestamp = 0, ptime = DSGetSessionInfo(hSessions[i], DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_PTIME, 1, NULL);  /* get session ptime in msec */
                        #define MAX_STATS_GAP 25*ptime

                        if (last_rtp_pyld_len[i] > 8 && (delta_timestamp = msec_timestamp_fp - last_msec_timestamp[i]) < MAX_STATS_GAP) {  /* omit DTMFs and SIDs, gaps larger than 500 msec (for 20 sec nominal ptime) */

                           arrival_avg_delta[i] = (arrival_avg_delta[i]*num_arrival_stats_pkts[i] + delta_timestamp)/(num_arrival_stats_pkts[i]+1);
                           arrival_avg_jitter[i] = (arrival_avg_jitter[i]*num_arrival_stats_pkts[i] + fabsf(delta_timestamp - ptime))/(num_arrival_stats_pkts[i]+1);

                           num_arrival_stats_pkts[i]++;

                           #if 0
                           static int count[10] = { 0 };
                           if (count[i]++ < 50) printf("\n[%d]%d msec_timestamp_fp = %4.2f, delta_timestamp = %4.2f, jitter = %4.2f\n", i, num_arrival_stats_pkts[i], msec_timestamp_fp, delta_timestamp, fabsf(delta_timestamp - arrival_avg_delta[i]));
                           #endif

                           arrival_max_delta[i] = max(arrival_max_delta[i], delta_timestamp);
                           arrival_max_jitter[i] = max(arrival_max_jitter[i], fabsf(delta_timestamp - ptime));
                        }
                     }

                     last_rtp_pyld_len[i] = rtp_pyld_len;
                     last_msec_timestamp[i] = msec_timestamp_fp;

                     break;  /* break out of nSessions loop, the packet should match no other sessions */
                  }
               }
            }  /* nSessions loop (i) */
         }  /* packet reuse loop (n) */
      }  /* input flow loop (end of file or UDP port; e.g. if fp[j] != NULL) */

   /* Auto-adjust push rate algorithm:

      -enabled if AUTO_ADJUST_PUSH_RATE flag is included in cmd line -dN entry (see all flag definitions near start of this file)
      -intended to be used in the absence of input packet flow timing, for example pcaps with no packet timestamps, UDP input flow from a source not using accurate wall clock timing, etc
      -the push rate is adjusted dynamically by monitoring transcoded output (G711) queue levels, which after transcoding are independent of input packet types (media vs SID, multiframe packets, variable ptime, etc). The objective is to adapt the push rate to timing derived from media content, in the absence of input packet flow timing
      -currently the average push rate (APR) is calculated per mediaMin thread, the idea being to treat all sessions equally. Adjustment is first initialized to push as many packets as there are sessions every -rN msec
      -when stream group processing is enabled, further alignment of individual streams is possible; the STREAM_GROUP_ENABLE_DEDUPLICATION flag is one possible option
      -DSPullPackets() and DSPushPackets() xxx_QUEUE_LEVEL flags return "distance" (in bytes) between input and output queue pointers
      -note that allowing the push rate to become too high will eventually overflow the push queue and a "queue full" status will be returned by DSPushPackets()
   */

      if ((Mode & AUTO_ADJUST_PUSH_RATE) && thread_info[thread_index].pcap_in[j] && nSessions) {  /* make input stream is active and there are one or more existing sessions */

push_ctrl:  /* upon entry PushPackets() jumps here if current average push rate (APR) is zero */

         int nSessionsActive = 0, nSessionsPushed = 0;
         bool fReduce = false, fIncrease = false;

         for (i=0; i<nSessions; i++) {
            if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) nSessionsActive++;
            if (session_push_cnt[i]) nSessionsPushed++;
         }

         nSessionsPushed /= (1 + nReuseInputs);  /* normalize totals if session reuse is active */
         nSessionsActive /= (1 + nReuseInputs);

         if (++auto_adj_push_count < average_push_rate[thread_index] && nSessionsPushed < nSessionsActive) goto next_packet;  /* push the packet ! */

      /* first push packets according to current APR value, then update APR */
  
         int g711_pktlen = 200;  /* algorithm parameter estimates */
         int numpkts = 20;

         for (i=0; i<nSessions; i++) if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) {

            int queue_level = DSPullPackets(DS_PULLPACKETS_TRANSCODED | DS_PULLPACKETS_GET_QUEUE_LEVEL, NULL, NULL, hSessions[i], NULL, 0, 0);

            if (queue_level < numpkts*g711_pktlen) fIncrease = true;
            if (queue_level > 6*numpkts*g711_pktlen) fReduce = true;
         }

      /* update APR (note this is occurring either (i) every -rN msec per cmd line entry or (ii) or every ptime msec if ANALYTICS_MODE flag is set in -dN cmd line entry) */
  
         if (fReduce) {

            if (average_push_rate[thread_index]) average_push_rate[thread_index] = 0;
         }
         else average_push_rate[thread_index] = nSessionsActive;

         if (fIncrease) average_push_rate[thread_index]++;

         if (isMasterThread && cur_time - last_cur_time > 100*1000) {  /* update APR screen output no faster than 100 msec */

            app_printf(APP_PRINTF_SAME_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "apr %d ", average_push_rate[thread_index]);
            last_cur_time = cur_time;
         }
      }

   }  /* end of input stream loop */

   return push_cnt;
}


/* pull packets from packet / media per-session queue.  Packets are pulled by category:  jitter buffer output, transcoded, and stream group */

int PullPackets(uint8_t* pkt_out_buf, HSESSION hSessions[], SESSION_DATA session_data[], unsigned int uFlags, unsigned int pkt_buf_len, int thread_index) {

int j, num_pkts = 0, i = 0, num_pkts_total = 0, numPkts;
FILE* fp = NULL;
int packet_out_len[1024];  /* These sizes should handle the maximum number of packets that fit into pkt_buf_len amount of space.  That will vary from app to app depending on codec types, max ptimes, etc.  MAXSPACEDEBUG can be used below to look at the worst case.  JHB Aug 2018 */
uint64_t packet_info[1024];
uint8_t* pkt_out_ptr;
char errstr[20];
int nRetry[MAX_SESSIONS] = { 0 };
int group_idx = -1;
bool fRetry = false;

   if (!thread_info[thread_index].nSessionsCreated) return 0;  /* nothing to do if no sessions created yet */

entry:

   if (uFlags == DS_PULLPACKETS_JITTER_BUFFER) {

      fp = thread_info[thread_index].fp_pcap_jb[i];
      strcpy(errstr, "jitter buffer");
   }
   else if (uFlags == DS_PULLPACKETS_TRANSCODED) {

      fp = thread_info[thread_index].pcap_out[i];
      strcpy(errstr, "transcoded");
   }
   else if ((Mode & ENABLE_STREAM_GROUPS) && uFlags == DS_PULLPACKETS_STREAM_GROUP) {

      i = GetNextGroupSessionIndex(hSessions, i, thread_index);  /* for stream group packets, the session handle given to DSPullPackets() has to be a group session owner. To look for group session owners, we call GetNextGroupSessionIndex().  Note that it returns an index into hSessions[] not a handle */

      if (i >= 0) {

         group_idx = DSGetStreamGroupInfo(hSessions[i], DS_STREAMGROUP_INFO_CHECK_GROUPTERM, NULL, NULL, NULL);  /* note if hSessions[i] is marked for deletion it will have 0x80000000 flag and DSGetStreamGroupInfo() will return -1. This is not a problem since we check for the deletion flag at pull: */
         if (group_idx < 0) return 0;

         fp = thread_info[thread_index].fp_pcap_group[group_idx];
         strcpy(errstr, "stream group");
      }
      else return 0;  /* no group owner sessions found */
   }
   else return -1;  /* invalid uFlags or mode */

   if ((Mode & ANALYTICS_MODE) || session_data[i].term1.input_buffer_interval > 0) numPkts = 1;  /* pull one packet in timed situations */
   else numPkts = -1;  /* numPkts set to -1 tells DSPullPackets() to pull all available packets. Note this applies in untimed mode with -r0 timing (look for fUntimedMode), JHB Jan 2023 */

pull:

   if (!(hSessions[i] & SESSION_MARKED_AS_DELETED) && !(nRetry[i] & 0x100)) {  /* make sure we have a valid session handle (we may have previously deleted the session), also check to see if we've already had a successful pull */

      num_pkts = DSPullPackets(uFlags, pkt_out_buf, packet_out_len, hSessions[i], packet_info, pkt_buf_len, numPkts);  /* pull available output packets of type specified by uFlags from packet/media thread queue. If numPkts = -1, that indicates pull all that's available */

      if (num_pkts < 0) {
         app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "Error in DSPullPackets() for %s output, return code = %d", errstr, num_pkts);
         goto exit;
      }

      num_pkts_total += num_pkts;  /* change how we calculate return value, no longer depending on file output cmd line specs, JHB Aug 2023 */
   }
   else goto next_session;

   #ifdef MAXSPACEDEBUG
   static int max_num_pkts = 0;
   if (num_pkts > max_num_pkts) {
      max_num_pkts = num_pkts;
      printf("max num pkts = %d\n", max_num_pkts);
   }
   #endif

   //#define SHOW_XCODED_STATS
   #ifdef SHOW_XCODED_STATS
   static int prev_num_xcoded[MAX_APP_THREADS][MAX_SESSIONS] = {{0}}, total_zero_xcoded[MAX_APP_THREADS][MAX_SESSIONS] = {{ 0 }};

   if (uFlags == DS_PULLPACKETS_TRANSCODED) {

      if (!num_pkts && !thread_info[thread_index].flush_state[i]) {

         if (prev_num_xcoded[thread_index][i]) {

            total_zero_xcoded[thread_index][i]++;

            char tmpstr[100], xcodedstats_str[100];
            strcpy(tmpstr, "xc zero output session = ");

            sprintf(xcodedstats_str, "%s%d %d", j == 0 ? "" : ", ", hSessions[i], total_zero_xcoded[thread_index][i]);
            strcat(tmpstr, xcodedstats_str);

            strcat(tmpstr, " ");
            fprintf(stderr, tmpstr);
         }
      }
      else prev_num_xcoded[thread_index][i] = num_pkts;
   }
   #endif

   if (uFlags == DS_PULLPACKETS_JITTER_BUFFER) {
   
      thread_info[thread_index].pkt_pull_jb_ctr += num_pkts;
//    if (hSessions[i] == 0) printf(" == push - pull count %d\n", thread_info[thread_index].pkt_push_ctr - thread_info[thread_index].pkt_pull_jb_ctr);
   }
   else if (uFlags == DS_PULLPACKETS_TRANSCODED) {
   
      thread_info[thread_index].pkt_pull_xcode_ctr += num_pkts;

//      if (Mode & ENABLE_WAV_OUTPUT_TIMESTAMP_MATCH) DSProcessStreamGroupContributorsTSM(hSessions[i], pkt_out_buf, packet_out_len, num_pkts, MediaParams[0].Media.inputFilename, (Mode & INCLUDE_PAUSES_IN_WAV_OUTPUT) != 0);
   }
   else if (uFlags == DS_PULLPACKETS_STREAM_GROUP) thread_info[thread_index].pkt_pull_streamgroup_ctr += num_pkts;

   if (fp) {

      if (uFlags == DS_PULLPACKETS_STREAM_GROUP) {

         if (!fStressTest && !fCapacityTest && (Mode & (USE_PACKET_ARRIVAL_TIMES | ANALYTICS_MODE))) {  /* note we did not add fUntimedMode check here as stream group timing would be unpredictable and there is likely no point in attempting retries, JHB Jan 2023 */

            if (!num_pkts) {

               if (thread_info[thread_index].fFirstGroupPull[group_idx] && !thread_info[thread_index].flush_state[i]) {

                  if (!nRetry[i] && thread_info[thread_index].group_interval_stats_index < MAX_GROUP_STATS) {  /* record the retry in stream group stats */

                     if (thread_info[thread_index].group_interval_stats_index > 0 && thread_info[thread_index].GroupIntervalStats[thread_info[thread_index].group_interval_stats_index-1].missed_interval == thread_info[thread_index].pkt_pull_streamgroup_ctr) {
                        thread_info[thread_index].GroupIntervalStats[thread_info[thread_index].group_interval_stats_index-1].repeats++;
                     }
                     else {
                        thread_info[thread_index].GroupIntervalStats[thread_info[thread_index].group_interval_stats_index].missed_interval = thread_info[thread_index].pkt_pull_streamgroup_ctr;
                        thread_info[thread_index].GroupIntervalStats[thread_info[thread_index].group_interval_stats_index].hSession = hSessions[i];
                        thread_info[thread_index].group_interval_stats_index++;
                     }
                  }

#ifdef USE_GROUP_PULL_RETRY  
               
               /* For this combination of modes, consistent ptime output intervals is crucial, if we miss we wait some time and try again, up to some limit.  Notes:

                  -the current sleep and max wait times are 1 msec and 8 msec
                  -this handles cases where app or p/m threads are temporarily a bit slow, maybe due to file I/O or other system timing delays
                  -this happens rarely if stream group output has FLC enabled, in which case p/m threads are making every effort to generate on-time output
                  -when this occurs it can be identified in output stream group pcaps as a slight variation in packet delta, for example 22 msec, followed by one of 18 msec (analyze using Wireshark stats under Telephony | RTP | Stream Analysis)
               */

                  nRetry[i]++;  /* mark session as needing a retry */
               }
#endif
            }
            else {

               if (!thread_info[thread_index].fFirstGroupPull[group_idx]) thread_info[thread_index].fFirstGroupPull[group_idx] = true;

               if (nRetry[i]) {

                  if (thread_info[thread_index].group_pull_stats_index < MAX_GROUP_STATS) {
                     thread_info[thread_index].GroupPullStats[thread_info[thread_index].group_pull_stats_index].retry_interval = thread_info[thread_index].pkt_pull_streamgroup_ctr - num_pkts;
                     thread_info[thread_index].GroupPullStats[thread_info[thread_index].group_pull_stats_index].num_retries = nRetry[i];
                     thread_info[thread_index].GroupPullStats[thread_info[thread_index].group_pull_stats_index].hSession = hSessions[i];
                     thread_info[thread_index].group_pull_stats_index++;
                  }
               }

               nRetry[i] |= 0x100;  /* mark session with successful pull flag */
            }
         }
      }

      for (j=0, pkt_out_ptr=pkt_out_buf; j<num_pkts; j++) {

         struct timespec* p_ts = NULL;  /* default (NULL) tells DSWritePcap() to determine packet timestamps using wall clock */

         if (group_idx >= 0) {  /* group_idx valid only when set in (uFlags == DS_PULLPACKETS_STREAM_GROUP) above */

            if (isAFAPMode) {  /* for stream group output in AFAP mode we advance packet timestamps at regular ptime intervals, JHB May 2023 */

               if (!thread_info[thread_index].accel_time_ts[group_idx].tv_sec) clock_gettime(CLOCK_REALTIME, &thread_info[thread_index].accel_time_ts[group_idx]);  /* if sec is uninitialized we start with current time */
               else {

                  int ptime = DSGetSessionInfo(hSessions[i], DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_GROUP_PTIME, 0, NULL);

                  uint64_t t = 1000000ULL*(uint64_t)thread_info[thread_index].accel_time_ts[group_idx].tv_sec + (uint64_t)thread_info[thread_index].accel_time_ts[group_idx].tv_nsec/1000 + ptime*1000L;  /* calculate in usec */

                  thread_info[thread_index].accel_time_ts[group_idx].tv_sec = t/1000000ULL;  /* update stream group's packet timestamp */
                  thread_info[thread_index].accel_time_ts[group_idx].tv_nsec = (t - 1000000ULL*thread_info[thread_index].accel_time_ts[group_idx].tv_sec)*1000;
               } 

               p_ts = &thread_info[thread_index].accel_time_ts[group_idx];  /* override DSWritePcap() default with specified packet timestamp */
            }
            else if (isFTRTMode) {  /* for stream group output in FTRT mode we advance packet timestamps at accelerated time (based on timeScale value), JHB May 2023 */

               struct timespec ts;
               clock_gettime(CLOCK_REALTIME, &ts);

               uint64_t cur_time = ts.tv_sec * 1000000ULL + ts.tv_nsec/1000;  /* calculate in usec */

               if (!thread_info[thread_index].accel_time_ts[group_idx].tv_sec) { thread_info[thread_index].accel_time_ts[group_idx].tv_sec = cur_time/1000000ULL; thread_info[thread_index].accel_time_ts[group_idx].tv_nsec = (cur_time - 1000000ULL*thread_info[thread_index].accel_time_ts[group_idx].tv_sec)*1000; }  /* save one-time calculation of base time */

               uint64_t base_time = thread_info[thread_index].accel_time_ts[group_idx].tv_sec * 1000000ULL + thread_info[thread_index].accel_time_ts[group_idx].tv_nsec/1000; 

               uint64_t t = base_time + (cur_time - base_time) * timeScale;

               ts.tv_sec = t/1000000ULL;  /* update stream group's packet timestamp */
               ts.tv_nsec = (t - 1000000ULL*ts.tv_sec)*1000;

               p_ts = &ts;  /* override DSWritePcap() default with specified packet timestamp */
            }
         }

         if (DSWritePcap(fp, pkt_out_ptr, NULL, NULL, NULL, p_ts, packet_out_len[j]) < 0) { fprintf(stderr, "DSWritePcap() failed for %s output\n", errstr); return -1; }
         else pkt_out_ptr += packet_out_len[j];
      }
   }

next_session:

   if ((uFlags == DS_PULLPACKETS_TRANSCODED || uFlags == DS_PULLPACKETS_JITTER_BUFFER) && ++i < thread_info[thread_index].nSessionsCreated) {

      if (uFlags == DS_PULLPACKETS_JITTER_BUFFER) fp = thread_info[thread_index].fp_pcap_jb[i];
      else fp = thread_info[thread_index].pcap_out[i];
      goto pull;
   }

   if (uFlags == DS_PULLPACKETS_STREAM_GROUP) {

      if (++i < thread_info[thread_index].nSessionsCreated) {

         i = GetNextGroupSessionIndex(hSessions, i, thread_index);

         if (i >= 0) {
            group_idx = DSGetStreamGroupInfo(hSessions[i], DS_STREAMGROUP_INFO_CHECK_GROUPTERM, NULL, NULL, NULL);  /* note if hSessions[i] is marked for deletion it will have 0x80000000 flag and DSGetStreamGroupInfo() will return -1. This is not a problem since we check for the deletion flag at pull: */
            fp = thread_info[thread_index].fp_pcap_group[group_idx];
            goto pull;
         }
      }

   /* check for stream groups that may need a retry. Notes JHB Mar2020:

      -for a retry we sleep 1 msec, then call DSPullPackets() again. This includes all stream group owner sessions that didn't yet produce a packet (if any)
      -max number of retries is 8
      -currently retries apply only to stream group output when packet arrival times (packet timestamps) and ptime output timing are enabled. In this case regular output timing is required and we want to avoid any variation
   */

      for (i=0, fRetry=false; i<thread_info[thread_index].nSessionsCreated && !fRetry; i++) if (nRetry[i] > 0 && nRetry [i] < 8) fRetry = true;
   
      #if 0  /* debug */
      static int nOnce = 0;
      if (fRetry && nOnce < 100) { printf("\n ==== retry = true, nRetry[%d] = %d \n", i, nRetry[i]); nOnce++; }
      #endif

      if (!isAFAPMode && !isFTRTMode && fRetry) {
         usleep(1000);  /* sleep 1 msec */
         i = 0;
         goto entry;  /* retry one or more sessions */
      }
   }

exit:
   return num_pkts_total;
}


void InputSetup(int thread_index) {

int i = 0, j = 0;
char tmpstr[1024];
unsigned int uFlags;

   if (thread_info[thread_index].init_err) return;

   if (Mode & AUTO_ADJUST_PUSH_RATE) average_push_rate[thread_index] = 2;  /* initialize auto-adjust push rate algorithm */

   thread_info[thread_index].nInPcapFiles = 0;

   uFlags = DS_READ | DS_OPEN_PCAP_READ_HEADER;
   if (fCapacityTest) uFlags |= DS_OPEN_PCAP_QUIET;

/* open input pcap files, advance file pointer to first packet.  Abort program on any input file failure */

   #pragma GCC diagnostic push  /* suppress "address of var will never be NULL" warnings in gcc 12.2; safe-coding rules prevail, JHB May 2023 */
   #pragma GCC diagnostic ignored "-Waddress"
   while (MediaParams[i].Media.inputFilename != NULL && strlen(MediaParams[i].Media.inputFilename)) {
   #pragma GCC diagnostic pop

      if (strcasestr(MediaParams[i].Media.inputFilename, ".pcap") || strcasestr(MediaParams[i].Media.inputFilename, ".rtp")) {  /* look for .pcap and .pcapng, JHB Oct 2020. Look for .rtp and .rtpdump, JHB Nov 2023 */

         thread_info[thread_index].pcap_file_hdr[j] = (pcap_hdr_t*)calloc(1, sizeof_field(pcap_hdr_t, rtp));  /* allocate space for file header (sizeof_field macro is in pktlib.h), JHB May 2024 */

         if ((thread_info[thread_index].link_layer_len[j] = DSOpenPcap(MediaParams[i].Media.inputFilename, &thread_info[thread_index].pcap_in[j], thread_info[thread_index].pcap_file_hdr[j], "", uFlags)) < 0) {

            strcpy(tmpstr, "../");  /* try up one subfolder */
            strcat(tmpstr, MediaParams[i].Media.inputFilename);

            if ((thread_info[thread_index].link_layer_len[j] = DSOpenPcap(tmpstr, &thread_info[thread_index].pcap_in[j], thread_info[thread_index].pcap_file_hdr[j], "", uFlags)) < 0) {

               fprintf(stderr, "Failed to open input file %s, input = %d, thread_index = %d, DSOpenPcap ret val = %d \n", tmpstr, j, thread_index, thread_info[thread_index].link_layer_len[j]);
               thread_info[thread_index].pcap_in[j] = NULL;
               thread_info[thread_index].init_err = true;  /* error condition for at least one input file, error message is already printed and/or logged */
               break;
            }
         }

         goto valid_input_spec;
      }
      else if (strcasestr(MediaParams[i].Media.inputFilename, ".ber")) {

         if (!(thread_info[thread_index].pcap_in[j] = fopen(MediaParams[i].Media.inputFilename, "rb+"))) {

            strcpy(tmpstr, "../");  /* try up one subfolder */
            strcat(tmpstr, MediaParams[i].Media.inputFilename);

            if (!(thread_info[thread_index].pcap_in[j] = fopen(tmpstr, "rb+"))) {

               fprintf(stderr, "Failed to open input ber file %s, index = %d, thread_index = %d \n", tmpstr, j, thread_index);
               thread_info[thread_index].pcap_in[j] = NULL;
               thread_info[thread_index].init_err = true;  /* error condition for at least one input file, error message is already printed and/or logged */
               break;
            }
         }

         thread_info[thread_index].link_layer_len[j] = PCAP_TYPE_BER << 16;

valid_input_spec:

         thread_info[thread_index].packet_number[j] = 0;
         thread_info[thread_index].num_tcp_packets[j] = 0;
         thread_info[thread_index].num_udp_packets[j] = 0;
         thread_info[thread_index].num_rtp_packets[j] = 0;
         thread_info[thread_index].num_packets_encapsulated[j] = 0;
         thread_info[thread_index].num_packets_fragmented[j] = 0;
         thread_info[thread_index].num_packets_reassembled[j] = 0;
         thread_info[thread_index].input_index[j] = i;  /* save an index that maps to cmd line input specs */
         thread_info[thread_index].nInPcapFiles = ++j;
      }
      else { fprintf(stderr, "Input file %s does not have .pcap, .pcapng, .rtp, .rtpdump, or .ber file extension \n", MediaParams[i].Media.inputFilename); break; }

      i++;  /* advance to next cmd line input spec */
   }

   if (isFTRTMode) timeScale = NOMINAL_REALTIME_INTERVAL/RealTimeInterval[0];  /* if "faster than real-time" mode set timeScale to accelerate time. RealTimeInterval[] is initialized in cmdLineInterface() in cmd_line_interface.c, Jun 2023 */
   else timeScale = 1;  /* otherwise normal time. Note in AFAP mode (-r0 cmd entry) RealTimeInterval[0] is zero and timeScale stays 1 */

   if (i == 0) thread_info[thread_index].init_err = true;  /* error if no inputs */

   if (thread_info[thread_index].init_err) app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, " *************** inside input setup, init err true, thread_index = %d", thread_index);
}

/* set up transcoded outputs from -oXXX outupt specs entered on command line (if any) */

void TranscodedOutputSetup(int thread_index) {

int i = 0, ret_val;
char tmpstr[1024];
char filestr[1024];
unsigned int uFlags;

   if (thread_info[thread_index].init_err) return;

   thread_info[thread_index].nOutPcapFiles = 0;

   uFlags = DS_WRITE | DS_OPEN_PCAP_WRITE_HEADER;
   if (fCapacityTest) uFlags |= DS_OPEN_PCAP_QUIET;

/* open output pcap files, stop on first failure (but still allow program to run) */

   #pragma GCC diagnostic push  /* suppress "address of var will never be NULL" warnings in gcc 12.2; safe-coding rules prevail, JHB May 2023 */
   #pragma GCC diagnostic ignored "-Waddress"
   while (MediaParams[i].Media.outputFilename != NULL && strlen(MediaParams[i].Media.outputFilename)) {
   #pragma GCC diagnostic pop

      if (strcasestr(MediaParams[i].Media.outputFilename, ".pcap")) {

         char* p1 = strcasestr(MediaParams[i].Media.outputFilename, ".pcapng");
         if (p1) fprintf(stderr, "Note - output file %s will be written in pcap format, not pcapng \n", MediaParams[i].Media.outputFilename);  /* print note, as currently pktlib supports pcapng read but not write, JHB Oct2020 */

         strcpy(tmpstr, MediaParams[i].Media.outputFilename);
         char* p2 = strrchr(tmpstr, '.');
         if (p2) *p2 = 0;

         if (num_app_threads > 1) {
            if (p1) sprintf(filestr, "%s%d.pcapng", tmpstr, thread_index);
            else sprintf(filestr, "%s%d.pcap", tmpstr, thread_index);
         }
         else strcpy(filestr, MediaParams[i].Media.outputFilename);

         if (!thread_info[thread_index].pcap_out[thread_info[thread_index].nOutPcapFiles] && (ret_val = DSOpenPcap(filestr, &thread_info[thread_index].pcap_out[thread_info[thread_index].nOutPcapFiles], NULL, "", uFlags)) < 0) {

            fprintf(stderr, "Failed to open transcoded output pcap file: %s, index = %d, thread_index = %d, ret_val = %d \n", filestr, thread_info[thread_index].nOutPcapFiles, thread_index, ret_val);
            thread_info[thread_index].pcap_out[thread_info[thread_index].nOutPcapFiles] = NULL;
            break;
         }

         thread_info[thread_index].nOutPcapFiles++;
      }

      i++;
   }
}

void MiscOutputSetup(int thread_index) {

/* if entered, apply -gStreamGroupOutputPath cmd line option to stream group and/or timestamp-matching mode output wav files. Notes:

   -wav files are written by packet/media threads, if these are being written to an HDD (rotating media) any reduction in seek times can significantly improve overall thread performance. With Linux ext4 filesystems, long seek times can result from an HDD operating at near full capacity over time, in which case it may be slightly fragmenting during write operations (i.e. files with a few sectors seperated by a long physical distance on the disk platter)

   -if mediaMin display and/or event log outputs shows pre-emption messages such as:

      WARNING: p/m thread 0 has not run for 45.39 msec, may have been preempted, num sessions = 3,
      creation history = 0 0 0 0, deletion history = 0 0 0 0, last decode time = 0.00,
      last encode time = 0.02, ms time = 0.00 msec, last ms time = 0.00, last buffer time = 0.00,
      last chan time = 0.00, last pull time = 0.00, last stream group time = 45.38

    this can indicate seek times for stream group output wav files are negatively impacting performance. The key text is "last stream group time"

   -for example, if a ramdisk exists, then the mediaMin command line might contain:

      -g/mnt/ramdisk

    or

      -g/tmp/ramdisk

    or as appropriate depending on the system. Look in /etc/fstab to see if a ramdisk is active and if so its path

   -default is no entry, in which case wav files are generated on the mediaMin app subfolder
   -does not apply to N-channel wav files, which are post-processed after a stream group closes (all streams in the group are finished)
*/

   if (strlen(szStreamGroupWavOutputPath) > 0 && szStreamGroupWavOutputPath[strlen(szStreamGroupWavOutputPath)-1] != '/') strcat(szStreamGroupWavOutputPath, "/");  /* add / suffix, if not already there */

   if (strlen(szStreamGroupPcapOutputPath) > 0 && szStreamGroupPcapOutputPath[strlen(szStreamGroupPcapOutputPath)-1] != '/') strcat(szStreamGroupPcapOutputPath, "/");  /* add / suffix, if not already there */
}


/* Set up audio stream group output pcap files:

   -we search through created sessions for group owner sessions and for each one found we create output an filename with "N" suffix (stream group number)
   -if no group owner sessions are found there will be no stream group pulled packets or output pcaps files
*/

void StreamGroupOutputSetup(HSESSION hSessions[], int nInput, int thread_index) {

int ret_val, i = 0, i2, group_idx;
char filestr[1024];
char group_output_pcap_filename[1024] = "";
char group_output_text_filename[1024] = "";
unsigned int uFlags;

   if (thread_info[thread_index].init_err) return;

   if (strlen(szSessionName[nInput])) {
      strcpy(group_output_pcap_filename, szSessionName[nInput]);
      sprintf(&group_output_pcap_filename[strlen(group_output_pcap_filename)], "_group");
   }
   else {
      GetOutputFilename(group_output_pcap_filename, PCAP, "_group");  /* function in cmd_line_interface.c */
      char* p = strrchr(group_output_pcap_filename, '.');
      if (p) *p = 0;
   }

   if (Mode & ENABLE_STREAM_GROUP_ASR) {

      if (GetOutputFilename(group_output_text_filename, TEXT, "_group") >= 0) {
         char* p = strrchr(group_output_text_filename, '.');
         if (p) *p = 0;
      }
      else if (strlen(szSessionName[nInput])) {
         strcpy(group_output_text_filename, szSessionName[nInput]);
         sprintf(&group_output_text_filename[strlen(group_output_text_filename)], "_group");
      }
      else strcpy(group_output_text_filename, group_output_pcap_filename);
   }

   uFlags = DS_WRITE | DS_OPEN_PCAP_WRITE_HEADER;
   if (fCapacityTest) uFlags |= DS_OPEN_PCAP_QUIET;

   while (i < thread_info[thread_index].nSessionsCreated) {

      i2 = GetNextGroupSessionIndex(hSessions, i, thread_index);

      if (i2 >= 0) {

         i = i2;

         group_idx = DSGetStreamGroupInfo(hSessions[i], DS_STREAMGROUP_INFO_CHECK_GROUPTERM, NULL, NULL, NULL);

         if (!thread_info[thread_index].fp_pcap_group[group_idx]) {

            sprintf(filestr, "%s%s%d", szStreamGroupPcapOutputPath, group_output_pcap_filename, group_idx);  /* prefix with --group_pcap output path, if any, JHB Dec 2023 */
            if (num_app_threads > 1) sprintf(&filestr[strlen(filestr)], "_%d", thread_index);
            if (Mode & ANALYTICS_MODE) strcat(filestr, "_am");
            else if (fUntimedMode) strcat(filestr, "_um");
            strcat(filestr, ".pcap"); 

            strcpy(thread_info[thread_index].szGroupPcap[group_idx], filestr);  /* save copy of group output pcap path, JHB Dec 2023 */

            ret_val = DSOpenPcap(filestr, &thread_info[thread_index].fp_pcap_group[group_idx], NULL, "", uFlags);

            if (ret_val < 0) {

               fprintf(stderr, "Failed to open stream group output pcap file: %s, ret_val = %d\n", filestr, ret_val);
               thread_info[thread_index].fp_pcap_group[group_idx] = NULL;
            }
         }

         #if 0  /* not used, ASR output text is handled in pktlib and streamlib, JHB Jan 2021 */
         if (Mode & ENABLE_STREAM_GROUP_ASR) {

            if (!thread_info[thread_index].fp_text_group[group_idx]) {

               sprintf(filestr, "%s%d", group_output_text_filename, group_idx);
               if (num_app_threads > 1) sprintf(&filestr[strlen(filestr)], "_%d", thread_index);
               if (Mode & ANALYTICS_MODE) strcat(filestr, "_am");
               else if (fUntimedMode) strcat(filestr, "_um");
               strcat(filestr, ".txt"); 

               thread_info[thread_index].fp_text_group[group_idx] = fopen(filestr, "w");

               if (!thread_info[thread_index].fp_text_group[group_idx]) {

                  fprintf(stderr, "Failed to open stream group output text file: %s, errno = %d, errno description = %s\n", filestr, errno, strerror(errno));
               }
            }
         }
         #endif
      }

      i++;
   }
}

/* set up jitter buffer pcap outputs. This is done by default and does not require -oXXX command line arguments, but can be disabled by the DISABLE_JITTER_BUFFER_OUTPUT_PCAPS flag in -dN cmd line options */

void JitterBufferOutputSetup(int thread_index) {

int ret_val, i, nInput = 0;
char filestr[1024];
char jb_output_pcap_filename[1024] = "";
unsigned int uFlags;

   if (thread_info[thread_index].init_err || (Mode & DISABLE_JITTER_BUFFER_OUTPUT_PCAPS)) return;

   if (strlen(szSessionName[nInput])) {
      strcpy(jb_output_pcap_filename, szSessionName[nInput]);
      sprintf(&jb_output_pcap_filename[strlen(jb_output_pcap_filename)], "_jb");
   }
   else {
      GetOutputFilename(jb_output_pcap_filename, PCAP, "_jb");
      char* p = strrchr(jb_output_pcap_filename, '.');
      if (p) *p = 0;
   }

   uFlags = DS_WRITE | DS_OPEN_PCAP_WRITE_HEADER;
   if (fCapacityTest) uFlags |= DS_OPEN_PCAP_QUIET;

   for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) {

      if (!thread_info[thread_index].fp_pcap_jb[i]) {

         sprintf(filestr, "%s%d", jb_output_pcap_filename, i);
         if (num_app_threads > 1) sprintf(&filestr[strlen(filestr)], "_%d", thread_index);
         sprintf(&filestr[strlen(filestr)], ".pcap"); 

         ret_val = DSOpenPcap(filestr, &thread_info[thread_index].fp_pcap_jb[i], NULL, "", uFlags);

         if (ret_val < 0 || thread_info[thread_index].fp_pcap_jb[i] == NULL) { fprintf(stderr, "Failed to open jitter buffer output pcap file: %s ret_val = %d\n", filestr, ret_val); }
      }
   }
}


/* start specified number of packet/media threads */

int StartPacketMediaThreads(int num_pm_threads, int thread_index) {  /* should only be called by master thread.  See "thread sync points" above (thread_syncN: labels) */

unsigned int uFlags;

   if (nReuseInputs) num_pm_threads = num_app_threads * nReuseInputs * 3 / 30;  /* note:  without DS_CONFIG_MEDIASERVICE_ROUND_ROBIN sessions are assigned to each p/m thread until it fills up. Some p/m threads may end up unused */

   num_pm_threads = max(min(num_pm_threads, 10), 1);  /* from 1 to 10 */

   if (Mode & ROUND_ROBIN_SESSION_ALLOCATION) num_pm_threads = max(num_pm_threads, 2);  /* start a minimum of two p/m threads if round-robin session-to-thread allocation has been specified, for example running multiple cmd line input packet flows, JHB Jan2020 */

   num_pktmed_threads = num_pm_threads;  /* num_pktmed_threads is a global */

   app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, 0, "Starting %d packet and media processing threads", num_pktmed_threads);

   uFlags = DS_CONFIG_MEDIASERVICE_START | DS_CONFIG_MEDIASERVICE_THREAD | DS_CONFIG_MEDIASERVICE_PIN_THREADS | DS_CONFIG_MEDIASERVICE_SET_NICENESS;
   if (Mode & ROUND_ROBIN_SESSION_ALLOCATION) uFlags |= DS_CONFIG_MEDIASERVICE_ROUND_ROBIN;

   uFlags |= DS_CONFIG_MEDIASERVICE_ENABLE_THREAD_PROFILING;  /* slight impact on performance, but useful.  Turn off for highest possible performance */

   if (DSConfigMediaService(NULL, num_pktmed_threads, uFlags, packet_flow_media_proc, NULL) < 0) {  /* start packet/media thread(s) */

      thread_info[MasterThread].init_err = true;
      return -1;
   }

   return 1;
}


/* check for session inactivity, including empty push and pull queues and end of inputs.  Flush inactive sessions to force all remaining packets out of jitter buffer and algorthim queues */

void FlushCheck(HSESSION hSessions[], uint64_t cur_time, uint64_t (*pQueueCheckTime)[MAX_SESSIONS], int thread_index) {

#define FINAL_FLUSH_STATE 3

int i, j, nDelayTime, nInput;
int nFlushed = 0;
char flushstr[MAX_APP_STR_LEN] = "Flushing NNN sessions";  /* this text will be updated at actual print/log time */
int flushstr_initlen = strlen(flushstr);
uint64_t* queue_check_time = (uint64_t*)pQueueCheckTime;

   if (Mode & CREATE_DELETE_TEST_PCAP) return;  /* don't flush sessions in session create/delete test mode where pcap is repeating */

   for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) {

      if (thread_info[thread_index].flush_state[i] < 2) {  /* check input, push and xcode pull queues to see if they are finished / empty */

         bool queue_empty = true;

         if (thread_info[thread_index].pkt_push_ctr == 0) queue_empty = false;  /* at least one packet has to be pushed first */

         if (queue_empty && (Mode & (USE_PACKET_ARRIVAL_TIMES | ANALYTICS_MODE))) {  /* in these modes, we don't look at queue status until input flow ends, for example there may be valid call waiting or on-hold pauses */

            if (Mode & DYNAMIC_SESSIONS) {

               if ((nInput = GetInputFromSessionIndex(i, thread_index)) < 0 || thread_info[thread_index].pcap_in[nInput]) queue_empty = false;  /* if input is finished then wait for queues to empty out */
            }
            else {  /* for static session creation we don't know which input files match which sessions, so we wait for all inputs on the cmd line to finish, JHB Mar2020 */

               for (j=0; j<thread_info[thread_index].nInPcapFiles; j++) if (thread_info[thread_index].pcap_in[j]) { queue_empty = false; break; }
            }
         }

         if (queue_empty) {  /* continue with queue status check */

            if (DSPushPackets(DS_PUSHPACKETS_GET_QUEUE_STATUS, NULL, NULL, &hSessions[i], 1) == 0) queue_empty = false;  /* input queue not empty yet */
            else {

               if ((Mode & (USE_PACKET_ARRIVAL_TIMES | ANALYTICS_MODE)) && !thread_info[thread_index].flush_state[i]) {  /* for real-time packet flow when input has ended (e.g. pcap input with packet arrival times), flush on end of input and push queue empty, JHB Mar2020 */

                  FlushSession(hSessions, i);
                  sprintf(&flushstr[strlen(flushstr)], "%s %d", nFlushed > 0 ? "," : "", hSessions[i]);
                  nFlushed++;

                  thread_info[thread_index].flush_state[i]++;  /* only need to flush once */
               }

               unsigned int queue_flags = DS_PULLPACKETS_TRANSCODED | DS_PULLPACKETS_JITTER_BUFFER | DS_PULLPACKETS_STREAM_GROUPS;  /* if stream groups are not enabled, the stream group queue status will show as empty, so this is ok */ 

               #if 0
               if ((Mode & ENABLE_STREAM_GROUPS) && !(Mode & (USE_PACKET_ARRIVAL_TIMES | ANALYTICS_MODE))) queue_flags |= DS_PULLPACKETS_STREAM_GROUPS;  /* in these modes, we look only at jitter buffer and xcoded output, and flush when those run dry.  Stream group output after that happens is unpredictable, JHB Jan2020 */
               #endif

               if (DSPullPackets(DS_PULLPACKETS_GET_QUEUE_STATUS | queue_flags, NULL, NULL, hSessions[i], NULL, 0, 0) == 0) queue_empty = false;  /* not empty yet */
            }
         }

         unsigned int flush_wait = 50000;  /* arbitrary delay to wait before checking whether all queues are empty, or for packet arrival time / analytics modes, finalizing flush state */

         if (!queue_empty || queue_check_time[i] == 0) queue_check_time[i] = cur_time;  /* don't flush yet if queue not empty, reset queue check time */
         else if (cur_time - queue_check_time[i] > flush_wait) {

            if (!thread_info[thread_index].flush_state[i]) {

               FlushSession(hSessions, i);  /* flush session if not already flushed */
               sprintf(&flushstr[strlen(flushstr)], "%s %d", nFlushed > 0 ? "," : "", hSessions[i]);
               nFlushed++;
            }

            thread_info[thread_index].flush_state[i] = FINAL_FLUSH_STATE-1;
            thread_info[thread_index].flush_count++;  /* increrment thread stats flush count */
         }
      }
      else if (thread_info[thread_index].flush_state[i] == FINAL_FLUSH_STATE-1) {  /* check if flush state should be advanced */

         #if 0
         if ((Mode & (ANALYTICS_MODE | USE_PACKET_ARRIVAL_TIMES)) || !fAutoQuit) nDelayTime = 60;  /* default 60 msec delay */
         else nDelayTime = 3000;  /* longer if stopping without 'q' key */
         #else  /* removed 3 sec delay, seems to no longer be needed for legacy test pcaps with -r0 push rate. I guess over time the methods of detecting end-of-packet flow (queue checks etc) improved a lot, JHB Jan 2023 */
         nDelayTime = fAutoQuit ? 60 : 3000;
         #endif

         if (cur_time - queue_check_time[i] > 1000*(nDelayTime + 10*RealTimeInterval[0])*num_app_threads) {  /* arbitrary delay to wait after flushing.  We increase this if multiple mediaMin threads are running as packet/media threads will take longer to flush */

            thread_info[thread_index].flush_state[i] = FINAL_FLUSH_STATE;  /* set session's flush state to final */

            if (!fStressTest && !fCapacityTest && (Mode & DYNAMIC_SESSIONS) && !(Mode & COMBINE_INPUT_SPECS)) {  /* in static session and test modes, sessions are deleted at the end of the all inputs or end of test, either at the end or as app threads repeat */

               #define DELETE_SESSIONS_PER_INPUT_GROUP  /* if defined wait for all sessions associated with an input packet flow to reach final flush state, then delete together */ 
               #ifdef DELETE_SESSIONS_PER_INPUT_GROUP

               if ((nInput = GetInputFromSessionIndex(i, thread_index)) >= 0) {

               /* search sessions associated with an input packet flow to see if they've all been flushed, JHB Jan2020 */

                  bool fAllGroupSessionsFlushed = true;

                  for (j=0; j<thread_info[thread_index].nSessions[nInput]; j++) if (thread_info[thread_index].flush_state[thread_info[thread_index].nSessionIndex[nInput][j]] != FINAL_FLUSH_STATE) { fAllGroupSessionsFlushed = false; break; }

                  if (fAllGroupSessionsFlushed) {  /* delete sessions associated with an input packet flow */

                     char deletestr[1000] = "";

                     for (j=0; j<thread_info[thread_index].nSessions[nInput]; j++) {
                        if (j == 0) sprintf(deletestr, "Deleting %d session%s", thread_info[thread_index].nSessions[nInput], thread_info[thread_index].nSessions[nInput] > 1 ? "s" : "");
                        sprintf(&deletestr[strlen(deletestr)], "%s %d", j > 0 ? "," : "", hSessions[thread_info[thread_index].nSessionIndex[nInput][j]]);
                     }

                     if (strlen(deletestr)) {

                        if (num_app_threads > 1) sprintf(&deletestr[strlen(deletestr)], " (%d)", thread_index);

                        app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "%s", deletestr);  /* show session delete info onscreen */
                        Log_RT(4 | DS_LOG_LEVEL_FILE_ONLY, "mediaMin INFO: %s ", deletestr);  /* include session delete info in event log */
                     }

                     for (j=0; j<thread_info[thread_index].nSessions[nInput]; j++) DeleteSession(hSessions, thread_info[thread_index].nSessionIndex[nInput][j], thread_index);
                  }
               }

               #else  /* delete each sesssion independently as it reaches final flush state */

               DSDeleteSession(hSessions, i, thread_index);

               #endif
            }
         }
      }
   }

   if (nFlushed) {

      char *p, prefixstr[40];

      sprintf(prefixstr, "Flushing %d session%s", nFlushed, nFlushed > 1 ? "s" :"");
      memcpy((p = &flushstr[max((int)(flushstr_initlen-strlen(prefixstr)), 0)]), prefixstr, strlen(prefixstr));

      if (num_app_threads > 1) sprintf(&p[strlen(p)], " (%d)", thread_index);

      app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "%s", p);  /* show session flush info onscreen */
      Log_RT(4 | DS_LOG_LEVEL_FILE_ONLY, "mediaMin INFO: %s ", p);  /* include session flush info in event log */
   }
}


void GlobalConfig(GLOBAL_CONFIG* gbl_cfg) {

//#define SET_MAX_SESSIONS
//#define SET_ENERGY_SAVER_TIMING

/* see GLOBAL_CONFIG struct comments in config.h */

#ifdef SET_MAX_SESSIONS
   gbl_cfg->uMaxSessionsPerThread = 25;
   gbl_cfg->uMaxGroupsPerThread = 8;
#endif

#ifdef SET_ENERGY_SAVER_TIMING
   gbl_cfg->uThreadEnergySaverInactivityTime = 45000;  /* in msec */
   gbl_cfg->uThreadEnergySaverSleepTime = 500; /* in usec */
//   gbl_cfg->uThreadEnergySaverWaitForAppQueuesEmptyTime = 10000;  /* in msec */
#endif

/* when ASR is enabled, we currently have the packet/media thread pre-emption warning turned off. Notes, JHB Jan 2021

  -this is until we implement an inferlib thread (including Kaldi libs) running on the 2nd core of a physical core pair, JHB Jan 2021
  -setting uThreadPreemptionElapsedTimeAlarm to zero causes the default 40 msec limit, so have to use -1
*/

   if (Mode & ENABLE_STREAM_GROUP_ASR) gbl_cfg->uThreadPreemptionElapsedTimeAlarm = (uint32_t)-1;
}


/* configure pktlib and streamlib debug options. Several are enabled by default, others depend on -dN cmd line entry */

void DebugSetup(DEBUG_CONFIG* dbg_cfg) {

   if (!dbg_cfg) return;  /* valid DEBUG_CONFIG struct is required (defined in shared_include/config.h) */

   dbg_cfg->uEnableDataObjectStats = 1;  /* very slight impact on performance when creating sessions, but good info for capacity and stress tests */

   if (Mode & ENABLE_MEM_STATS) dbg_cfg->uDebugMode |= DS_SHOW_MALLOC_STATS;

   if (Mode & ENABLE_TIMING_MARKERS) dbg_cfg->uDebugMode |= DS_INJECT_GROUP_TIMING_MARKERS;  /* enabled one-sec timing markers in stream group audio output */

   if (Mode & ENABLE_ALIGNMENT_MARKERS) dbg_cfg->uDebugMode |= DS_INJECT_GROUP_ALIGNMENT_MARKERS;  /* alignment markers when deduplication algorithm is active */

   #if 0
   dbg_cfg->uDebugMode |= DS_INJECT_GROUP_OUTPUT_MARKERS;  /* optional stream group output buffer boundary markers (currently no mediaMin cmd line -dN flag for this) */
   #endif

   if (Mode & ENABLE_DEBUG_STATS) {
   
      dbg_cfg->uDebugMode |= DS_ENABLE_GROUP_MODE_STATS;  /* equivalent to creating a stream group with STREAM_GROUP_DEBUG_STATS in its group term group_mode flags, but this method applies to all stream groups and can be enabled/disabled at run-time by calling DSConfigPktlib() or DSConfigStreamlib() */ 
      dbg_cfg->uDebugMode |= DS_ENABLE_EXTRA_PACKET_STATS;
   }

   if (Mode & ENABLE_PACKET_INPUT_ALARM) {  /* enable elapsed time "no input packets" alarm inside DSPushPackets() */

      dbg_cfg->uPushPacketsElapsedTimeAlarm = 15000;  /* this is the default value, if nothing is set (in msec) */
      dbg_cfg->uDebugMode |= DS_ENABLE_PUSHPACKETS_ELAPSED_TIME_ALARM;
   }

/* Wav output seek time alarm:

   -this can be useful if packet/media thread (pktlib) pre-emption warnings are displayed on the console or in the event log showing "last stream group time" with a high value
   -if so, enabling this alarm helps look into what inside streamlib is causing the pre-emption
   -if wav output is slow for any reason, writing wav files to ramdisk may work around the problem (use mediaMin -g cmd line option)
*/
   if (Mode & ENABLE_WAV_OUT_SEEK_TIME_ALARM) dbg_cfg->uStreamGroupOutputWavFileSeekTimeAlarmThreshold = 10;  /* default setting, in msec. Zero disables */
}


/* configure event log, packet log, and packet run-time stats */

void LoggingSetup(DEBUG_CONFIG* dbg_cfg, int setup_type) {

int i = 0;
char szInputFileNoExt[1024] = "", szEventLogFile[1024] = "", szPacketLogFile[1024] = "";
char* p;

   if (!dbg_cfg) return;  /* a valid DEBUG_CONFIG struct is required (defined in shared_include/config.h) */

/* enable and configure event log */

   if (setup_type == LOG_EVENT_SETUP) {

      dbg_cfg->uDisableMismatchLog = 1;
      dbg_cfg->uDisableConvertFsLog = 1;

      if (!(Mode & CREATE_DELETE_TEST_PCAP)) dbg_cfg->uLogLevel = 8;  /* 8 is default setting, includes p/m thread, jitter buffers, and codecs. Set to level 9 to see all possible debug messages */
      else dbg_cfg->uLogLevel = 5;  /* log level 5 is used for create/delete test */

      log_level = dbg_cfg->uLogLevel;  /* set global var available to all local funcs and threads */

      dbg_cfg->uEventLogMode = LOG_OUTPUT | DS_EVENT_LOG_UPTIME_TIMESTAMPS;  /* enable event log output and timestamps. See LOG_OUTPUT definition at top, which has default definition LOG_SCREEN_FILE, specifying output to both event log file and to screen. LOG_SCREEN_FILE, LOG_FILE_ONLY, and LOG_SCREEN_ONLY are defined in diaglib.h */

#if 0  /* define to enable wall clock date/timestamps (i.e. system time). The default is relative, or "up time", timestamps; the DS_LOG_LEVEL_NO_TIMESTAMP can be combined with log_level (i.e. Log_RT(log_level, ...) to specify no timestamp, JHB Apr 2024 */
      dbg_cfg->uEventLogMode |= DS_EVENT_LOG_WALLCLOCK_TIMESTAMPS;
#endif

      if (!fStressTest && !fCapacityTest) dbg_cfg->uEventLogMode |= LOG_SET_API_STATUS;  /* for functional tests, enable API status and error numbers */

      if (!fStressTest && !fCapacityTest) {  /* in standard opearting mode, associate event log filename with first input pcap file found */

         #pragma GCC diagnostic push  /* suppress "address of var will never be NULL" warnings in gcc 12.2; safe-coding rules prevail, JHB May 2023 */
         #pragma GCC diagnostic ignored "-Waddress"
         while (MediaParams[i].Media.inputFilename != NULL && strlen(MediaParams[i].Media.inputFilename)) {
         #pragma GCC diagnostic pop

            if (strcasestr(MediaParams[i].Media.inputFilename, ".pcap") || strcasestr(MediaParams[i].Media.inputFilename, ".rtp")) {

               strcpy(szInputFileNoExt, MediaParams[i].Media.inputFilename);
               p = strrchr(szInputFileNoExt, '/');
               if (p) strcpy(szInputFileNoExt, p+1);
               p = strrchr(szInputFileNoExt, '.');
               if (p) *p = 0;
               strcpy(szSessionName[i], szInputFileNoExt);  /* save the processed filename as the session name, used also for output wav files, JHB Jun 2019 */ 
            }

            i++;
         }
      }

      #if (LOG_OUTPUT != LOG_SCREEN_ONLY)
      if (strlen(szInputFileNoExt)) {
         strcpy(szEventLogFile, szInputFileNoExt);
         sprintf(&szEventLogFile[strlen(szEventLogFile)], "_event_log%s.txt", (Mode & ANALYTICS_MODE) ? "_am" : (fUntimedMode ? "_um" : ""));
      }
      else strcpy(szEventLogFile, sig_lib_event_log_filename);  /* use default if necessary */

#if 0  /* if the app should control event log file descriptor, use this way, which tells diaglib not to create the log file.  Note the app is responsible for appending and/or rewinding the file if needed, and closing it */
      fp_sig_lib_event_log = fopen(szEventLogFile, "w");
      dbg_cfg->uLogEventFile = fp_sig_lib_event_log;
#else
      strcpy(dbg_cfg->szEventLogFilePath, szEventLogFile);  /* diaglib will create the log file, or if append mode is specified then open it for appending (see uEventLogMode enums in config.h) */
//      dbg_cfg->uEventLogMode |= DS_EVENT_LOG_APPEND;  /* example showing append mode */
      if (!fStressTest && !fCapacityTest) dbg_cfg->uEventLog_fflush_size = 1024;  /* set flush size for standard operating mode operation */
#endif
      #endif

      dbg_cfg->uPrintfLevel = 5;
   }

/* setup and enable packet stats history logging and run-time packet stats */

   if (setup_type == LOG_PACKETSTATS_SETUP) {

   /* determine packet log filename */

      if (!strlen((const char*)pktStatsLogFile)) {  /* if a log filename not already given on cmd line with -L entry (and [nopktlog] option not given), we use an input file to construct a log filename */

         i = 0;

         #pragma GCC diagnostic push  /* suppress "address of var will never be NULL" warnings in gcc 12.2; safe-coding rules prevail, JHB May 2023 */
         #pragma GCC diagnostic ignored "-Waddress"
         while (MediaParams[i].Media.inputFilename != NULL && strlen(MediaParams[i].Media.inputFilename)) {
         #pragma GCC diagnostic pop

            if (strcasestr(MediaParams[i].Media.inputFilename, ".pcap") || strcasestr(MediaParams[i].Media.inputFilename, ".rtp")) {

               strcpy(szPacketLogFile, MediaParams[i].Media.inputFilename);
               p = strrchr(szPacketLogFile, '/');
               if (p) strcpy(szPacketLogFile, p+1);
               p = strrchr(szPacketLogFile, '.');
               if (p) *p = 0;
               sprintf(&szPacketLogFile[strlen(szPacketLogFile)], "_pkt_log%s.txt", (Mode & ANALYTICS_MODE) ? "_am" : (fUntimedMode ? "_um" : ""));
               break;  /* break on first input found */
            }

            i++;
         }

         if (strlen(szPacketLogFile)) strcpy((char*)pktStatsLogFile, szPacketLogFile);  /* pktStatsLogFile is a global var in pktlib.  If not an empty string, pktlib will use this to log all packets and do input vs. output analyis on pkt/media thread exit */
      }

   /* enable packet stats history logging if -L[filename] given on the cmd line.  Notes:

      -packet stats history allows detailed packet log file output after all inputs complete.  Packet stats are collected at run-time and stored in mem with negligible impact on performance
      -detailed analysis takes time to process, depending on stream(s) length (number of packets) it can take from several sec to several minutes
      -see comments in config.h for packet_stats_logging enums and DEBUG_CONFIG struct
      -"use_log_file" is set in cmd_line_interface.c if any cmd line -L entry is present
   */
 
      if (use_log_file) dbg_cfg->uPktStatsLogging = DS_ENABLE_PACKET_STATS_HISTORY_LOGGING;  /* optional DS_LOG_BAD_PACKETS can be added here if packets rejected by the jitter buffer should be logged */

   /* enable run-time packet time, loss, and ooo, SID repair, media repair, underrun and overrun, and other stats.  Notes:

      -run-time packet stats have negligible impact on run-time performance, and can be written to the event log at any time, on per-session or per-stream group basis
      -however, they are not as accurate as packet history stats
      -pktlib default behavior is to write run-time packet stats to the event log just prior to session deletion
   */

      if (!fStressTest && !fCapacityTest) dbg_cfg->uPktStatsLogging |= DS_ENABLE_PACKET_TIME_STATS | DS_ENABLE_PACKET_LOSS_STATS;
   }
}


/* signal handler function */

void handler(int signo)
{
   assert(signo == SIGALRM);
//#define PRINTSTATES
#ifdef PRINTSTATES
   static int cnt = 0;
   printf("######TIMER HANDLER FUNCTION:::::::: %d, initial state = %d, ", cnt++, debug_test_state);
#endif
   switch(debug_test_state){
      case INIT:
         debug_test_state = CREATE;
         break;
      case CREATE:
         debug_test_state = DELETE;
         break;
      case DELETE:
         debug_test_state = CREATE;
         break;
   }
#ifdef PRINTSTATES
   printf("new state = %d  \n", debug_test_state);
#endif
}


void TimerSetup() {

struct itimerval tval;

   timerclear(& tval.it_interval);
   timerclear(& tval.it_value);

   tval.it_value.tv_sec = TIMER_INTERVAL;
   tval.it_interval.tv_sec = TIMER_INTERVAL;

   (void)signal(SIGALRM, handler);
   (void)setitimer(ITIMER_REAL, &tval, NULL);
}


void ThreadWait(int when, int thread_index) {

int i, j, wait_msec, wait_time;
static bool fFirstWait = false;

   if (isMasterThread) {

      if ((Mode & ENERGY_SAVER_TEST) && !fFirstWait) {

         uint32_t wait_time_usec = (pktlib_gbl_cfg.uThreadEnergySaverInactivityTime + 1000)*1000;  /* uThreadEnergySaverInactivityTime is in msec */
         app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "Master thread waiting %lu sec to test energy saver mode", (long int)wait_time_usec/1000000L);
         usleep(wait_time_usec);  /* wait energy saver state inactivity time + 1 sec */
         fFirstWait = true;
      }

      return;  /* the master application thread never sleeps for long periods otherwise we have problems responding to keybd commands */
   }

   if (when == 0) wait_time = 20000;
   else wait_time = 2000;

   for (i=0; i<(int)num_app_threads; i++) {

      if (i == thread_index) {

         wait_msec = rand() % wait_time;  /* delay thread from zero to 2 - 20 sec */

         if (when) wait_msec = max(wait_msec, 150);

         if (when == 0) app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_PRINT_ONLY, thread_index, "! mediaMin app thread %d staggered start waiting %d msec", thread_index, wait_msec);
         else app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_PRINT_ONLY, thread_index, "! mediaMin app thread %d waiting %d msec before repeat", thread_index, wait_msec);

         for (j=0; j<wait_msec*1000; j+=500) {

            usleep(500);  /* sleep in 500 usec intervals and check for fQuit */
            if (fQuit) return;
         }

         app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_PRINT_ONLY, thread_index, "! mediaMin app thread %d waited %d msec", thread_index, wait_msec);
      }
   }
}

/* update stress test vars and states, if active. Also "auto quit" looks for all sessions flushed, indicating the app should exit */

int TestActions(HSESSION hSessions[], int thread_index) {

int i, ret_val = 1;

/* actions for stress tests, if active (see Mode var comments in cmd_line_options_flags.h for possible tests that can be specified in the cmd line) */

   if ((Mode & CREATE_DELETE_TEST_PCAP) && debug_test_state == DELETE)  /* delete dynamic sessions in the "create from pcap" stress test mode.  Note that debug_test_state is updated by a timer in the "handler" signal handler function */
   {

      for (i = 0; i < thread_info[thread_index].nDynamicSessions; i++)  /* delete a dynamic session, but not the base session */
      {
         app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "+++++++++deleting session %d, nSessionsCreated = %d, nDynamicSessions = %d", hSessions[thread_info[thread_index].nSessionsCreated-1], thread_info[thread_index].nSessionsCreated, thread_info[thread_index].nDynamicSessions);

         thread_info[thread_index].nSessionsCreated--;
         DSDeleteSession(hSessions[thread_info[thread_index].nSessionsCreated]);
         thread_info[thread_index].nDynamicSessions--;
      }

      ResetDynamicSession_info(thread_index);  /* reset all dynamic session keys, cause all sessions to be "re-detected", including static sessions if any */

      debug_test_state = INIT;
   }

/* more actions, including repeat mode and auto-quit */

   bool fAllStreamsTerminated = false;

   bool fAllSessionsFlushed = (thread_info[thread_index].nSessionsCreated > 0);

   if (!thread_info[thread_index].total_sessions_created) {  /* if no sessions were created then session flush doesn't apply for auto-quit purposes, so we check stream terminations, for example BYE messages, JHB Apr 2023 */

      fAllStreamsTerminated = true;
      for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) if (!thread_info[thread_index].dynamic_terminate_stream[i]) { fAllStreamsTerminated = false; break; }  /* any stream not terminated makes this false and we go by session flush */
      
   }

   for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) if (thread_info[thread_index].flush_state[i] != FINAL_FLUSH_STATE) { fAllSessionsFlushed = false; break; }  /* any session not yet flushed makes this false */

   if (fAllStreamsTerminated || fAllSessionsFlushed) {
 
      if ((Mode & CREATE_DELETE_TEST) || nRepeatsRemaining[thread_index]-1 >= 0 || fRepeatIndefinitely) {
   
         if (!isMasterThread) usleep(1000*50);
         ret_val = 0;  /* for session delete/recreate stress test or repeat mode, start test over after all sessions are flushed */
      }
      else if (fAutoQuit) {

         fStop = true;  /* set fStop (graceful per-thread stop, same as 's' key). Note this is ok for any thread to set, as all threads wait and synchronize before proceeding with exit processing (look for if (fExit) { ... AppThreadSync() ...), JHB Apr 2023 */
         ret_val = 0;  /* cause thread to exit continuous push/pull loop */
      }
   }

   if (thread_info[thread_index].init_err) ret_val = 0;

   #ifdef VALGRIND_DEBUG
   usleep(VALGRIND_DELAY);
   #endif

   return ret_val;
}

/* format and/or write packets, JHB Dec 2021:

  -misc packet handling: format data into IP packet, write packets to intermediate output files, more if needed
  -to-do: error conditions, way to specify output file on cmd line, close files on prog exit (i.e. not every time something is written)
*/

int packet_actions(uint8_t* pyld_data, uint8_t* pkt_in_buf, uint8_t protocol, int* pkt_len, unsigned int uFlags) {

uint8_t pcap_type;

   if (pyld_data) {

      FORMAT_PKT formatPkt = { 0 };
      unsigned int uFlags_format_pkt = DS_FMT_PKT_STANDALONE | DS_FMT_PKT_USER_HDRALL;
      if (protocol == TCP) uFlags_format_pkt |= DS_FMT_PKT_TCPIP;  /* add flag if TCP/IP format specified by caller */

      formatPkt.IP_Version = DS_IPV4;
      uint32_t* p = (uint32_t*)&formatPkt.SrcAddr;
      *p = htonl(0x0A000101);
      p = (uint32_t*)&formatPkt.DstAddr;
      *p = htonl(0x0A000001);
      formatPkt.tcpHeader.SrcPort = 0xa0a0;
      formatPkt.tcpHeader.DstPort = 0xb0b0;

      if (pkt_len) *pkt_len = DSFormatPacket(-1, uFlags_format_pkt, pyld_data, *pkt_len, &formatPkt, pkt_in_buf);  /* format packet, adjust pkt_len */
   }

   if ((pcap_type = (uFlags & 0x0f)) && pkt_len && *pkt_len > 0) {  /* output to pcap if specified by uFlags */

      static bool fOnce = false;
      static FILE* fp_pcap_out = NULL;
      char temp_filename[256] = "";

      if (pcap_type == PCAP_TYPE_BER) strcpy(temp_filename, "ber_output.pcap");
      else if (pcap_type == PCAP_TYPE_HI3) strcpy(temp_filename, "hi3_output.pcap");

      if (!fOnce) {

         DSOpenPcap(temp_filename, &fp_pcap_out, NULL, "", DS_WRITE | DS_OPEN_PCAP_WRITE_HEADER);
         fclose(fp_pcap_out);
         fOnce = true;
      }

      if (fp_pcap_out) {

         fp_pcap_out = fopen(temp_filename, "rb+");
         fseek(fp_pcap_out, 0, SEEK_END);
         DSWritePcap(fp_pcap_out, pkt_in_buf, NULL, NULL, NULL, NULL, *pkt_len);
         fclose(fp_pcap_out);
      }
   }

   return 1;
}

bool SDPInfoCheck(uint8_t* pkt_buf, int pkt_len) {

   if (!pkt_buf || pkt_len <= 0) return false;

   char s[] = "Length: ";
   void* p = memmem(pkt_buf, pkt_len, s, strlen(s));
   if (!p) {
      char s[] = "a=rtpmap";
      p = memmem(pkt_buf, pkt_len, s, strlen(s));
   }

   return (p != NULL);
}

/* look through lists of allowed UDP ports, JHB Jan 2023 */

int isPortAllowed(uint16_t port, uint8_t* pkt_buf, int pkt_len, int nInput, int thread_index) {

char portstr[50] = "", countstr[50] = "";
unsigned int i = 0;

bool fFound = false, fSDPInfoFound = false;
static unsigned num_GPRS = 0;

/* first check cmd line -pN entries, if any, JHB May 2023 */

   while (uPortList[i] && i < MAX_CONCURRENT_STREAMS) if (port == uPortList[i++]) return PORT_ALLOW_ON_MEDIA_ALLOW_LIST;  /* PORT_ALLOW_xxx flags are defined in mediaMin.h */

/* second check allowed list of ports */

   for (i=0; i<sizeof(UDP_Port_Media_Allow_List)/sizeof(PORT_INFO_LIST); i++) if (port == UDP_Port_Media_Allow_List[i].port) return PORT_ALLOW_ON_MEDIA_ALLOW_LIST;

/* check SDP info database for allowed media ports, JHB Jun 2024 */

   for (i=0; i<thread_info[thread_index].num_media_descriptions[0]; i++) if (port == ((sdp::Media*)(thread_info[thread_index].media_descriptions[0][i]))->port) return PORT_ALLOW_SDP_MEDIA_DISCOVERED;  /* media_descriptions[] are parsed and processed in SDPParseInfo() in sdp_app.cpp */

/* misc protocols we can report instead of displaying "ignoring UDP port ..." message */

   switch (port) {

      case 53:
         strcpy(portstr, "DNS");
         fFound = true;
         break;

      case 547:
         strcpy(portstr, "DHCPv6");
         fFound = true;
         break;

      case 137:
      case 138:
         strcpy(portstr, "NetBIOS");
         fFound = true;
         break;

      case GTP_PORT:  /* GPRS tunneling protocol */
         if (num_GPRS++ < 16 || (num_GPRS < 512 && num_GPRS % 32 == 0) || num_GPRS % 512 == 0) { sprintf(portstr, "GPRS Tunneling"); sprintf(countstr, " (%d)", num_GPRS); }
         fFound = true;
         fSDPInfoFound = SDPInfoCheck(pkt_buf, pkt_len);
         break;

      case 9009:
         strcpy(portstr, "pichat");  /* https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml?search=90&page=3 */
         fFound = true;
         break;
   }

   if (fFound) {

      if (!(Mode & DISABLE_PORT_IGNORE_MESSAGES) && strlen(portstr)) app_printf(APP_PRINTF_NEW_LINE | APP_PRINTF_PRINT_ONLY, thread_index, "%s packet found%s, pkt number = %u, dst port = %u \n", portstr, countstr, thread_info[thread_index].packet_number[nInput], port);

      if (fSDPInfoFound) return PORT_ALLOW_SDP_INFO;

      return PORT_ALLOW_KNOWN;
   }

   return PORT_ALLOW_UNKNOWN;
}

bool isReservedUDP(uint16_t port) {

   switch (port) {

      case 137:
      case 138:   /* NetBIOS */
         return true;

      case 9009:  /* pichat */
         return true;

      case 547:   /* DHCPv6 */
         return true;

      case GTP_PORT:
         return true;
   }
   
   return false;
}

#ifdef _MEDIAMIN_

/* process command line input, show version and copyright info */

void cmdLine(int argc, char** argv, char* tmpstr) {

char version_info[500], lib_info[500], banner_info[500];

   sprintf(version_info, "%s %s \n%s \n", prog_str, version_str, copyright_str);

   sprintf(lib_info, "  SigSRF libraries in use: DirectCore v%s, pktlib v%s, streamlib v%s, voplib v%s, derlib v%s, alglib v%s, diaglib v%s, cimlib v%s", HWLIB_VERSION, PKTLIB_VERSION, STREAMLIB_VERSION, VOPLIB_VERSION, DERLIB_VERSION, ALGLIB_VERSION, DIAGLIB_VERSION, CIMLIB_VERSION);

   sprintf(banner_info, "%s: %s %s \n%s \n%s \n", prog_str, banner_str, version_str, copyright_str, lib_info);

   if (!cmdLineInterface(argc, argv, CLI_MEDIA_APPS | CLI_MEDIA_APPS_MEDIAMIN, version_info, banner_info)) exit(EXIT_FAILURE);  /* parse command line and set MediaParams, PlatformParams, RealTimeInterval, and pktStatsLogFile, use_log_file, and others. Print banner info. Set version_info and banner_info to NULL if not used. See mediaTest.h and cmd_line_interface.c */
}
#endif

void GetMD5Sum(char* tmpstr, int thread_index) {

char szMergeFilename[2*CMDOPT_MAX_INPUT_LEN] = "", md5str[2*CMDOPT_MAX_INPUT_LEN];

/* currently we are using the first stream group. We need a way to specify for which (or all) stream group output pcap(s) md5 stats are shown */

   if (Mode & ENABLE_WAV_OUTPUT_TIMESTAMP_MATCH) DSGetStreamGroupInfo(0, DS_STREAMGROUP_INFO_HANDLE_IDX | DS_STREAMGROUP_INFO_MERGE_TSM_FILENAME, NULL, NULL, szMergeFilename);
   else if (Mode & ENABLE_STREAM_GROUPS) {

      if (Mode & ENABLE_WAV_OUTPUT) DSGetStreamGroupInfo(0, DS_STREAMGROUP_INFO_HANDLE_IDX | DS_STREAMGROUP_INFO_MERGE_FILENAME, NULL, NULL, szMergeFilename);  /* stream group output wav filename to-do: specify group number */
      else strcpy(szMergeFilename, thread_info[thread_index].szGroupPcap[0]);  /* stream group output pcap filename to-do: specify group number */ 
   }

   if (strlen(szMergeFilename) > 0 && DSGetMD5Sum(szMergeFilename, md5str, sizeof(md5str)-1) == 1) {  /* get MD5 sum of output wav or pcap filename from diaglib API */

      sprintf(&tmpstr[strlen(tmpstr)], "%smd5sum\n", tabstr);
 
      if (Mode & ENABLE_WAV_OUTPUT_TIMESTAMP_MATCH) sprintf(&tmpstr[strlen(tmpstr)], "%s%stimestamp-match mode", tabstr, tabstr);
      else sprintf(&tmpstr[strlen(tmpstr)], "%s%s%s mode", tabstr, tabstr, isFTRTMode ? "FTRT" : (isAFAPMode ? "AFAP" : "real-time"));
      sprintf(&tmpstr[strlen(tmpstr)], " %s %s", md5str, szMergeFilename);
      strcat(tmpstr, "\n");
   }
}

bool isDuplicatePacket(PKTINFO* PktInfo, PKTINFO* PktInfo2, int nInput, int thread_index) {

   if (PktInfo->protocol != PktInfo2->protocol) return false;
   
   /* remove redundant TCP retransmissions. Notes, JHB Apr 2023:

      -streams may contain redundant TCP retransmission of some or all packets. Normally this may be due to transmission errors, but appears there are other cases also, such as for FEC purposes like F5 does, or some HI2/HI3 streams where every packet is duplicated
      -we detect and strip these out. Sequence numbers, length, and ports must be an exact copy
      -currently this is a rudimentary implementation, not likely to work with multiple/mixed TCP sessions
      -to-do: implement TCP session management, separate but similar to existing UDP sessions handled by pktlib
   */

   if (PktInfo->protocol == TCP) {

      if (PktInfo->seqnum == PktInfo2->seqnum && PktInfo->ack_seqnum == PktInfo2->ack_seqnum && PktInfo->pkt_len == PktInfo2->pkt_len && PktInfo->dst_port == PktInfo2->dst_port && PktInfo->src_port == PktInfo2->src_port) {

         thread_info[thread_index].tcp_redundant_discards[nInput]++;  /* increment number of redundant TCP transmissions discarded for input stream */

         #if 0  /* debug */
         printf(" *** discard TCP retransmit, pkt # = %u, seqnum = %u, tcp discard count = %d \n", thread_info[thread_index].num_tcp_packets[nInput]+1, PktInfo[j].seqnum, thread_info[thread_index].tcp_redundant_discards[nInput]);
         #endif

         return true;
      }
   }
   else if (PktInfo->protocol == UDP) {

   /* UDP/RTP packets are not typically duplicated with exception of RFC 7198, which applies to RTP media and is handled in pktlib. However, in general (not RTP) fragmented UDP packets (for example long SIP messages and SDP info descriptions) and certain ports may be duplicated because senders are worried about dropping the packet, making reassembly impossible or losing key network control info (e.g. DHCP). PushPackets() calls isDuplicatePacket() to look for such UDP packets and if found strips them out. Notes, JHB Jun 2024:

      -UDP checksum is ignored -- unreliable due to Wireshark warning about "UDP checksum offload". There is a lot of online discussion about this

      -currently certain packets sent to certain ports are looked at, including GTP, DHCP, and NetBIOS. This likely needs refinement for RTP over GTP, in which case we need to let same-SSRC detection and RFC 7198 make duplication decisions

      -UDP duplicates appearing 2 or more packets later are not currently detected. This may be the case if you see a console message like:

        ignoring UDP SIP fragment packet (2), pkt len = 653, frag flags = 0x2, last keyword search = "application"

      -pktlib's RFC 7198 implementation will "look back" up to 8 packets. mediaMin allows control over this with the -lN command line option where N is the number of packets to look back
   */

   #if 0
   if (thread_info[thread_index].packet_number[nInput] >= 627 && thread_info[thread_index].packet_number[nInput] <= 655) {

      char tmpstr[400];
      sprintf(tmpstr, "\n *** inside isDuplicatePacket pkt number %d, len = %d, len prev = %d, flags = 0x%x flags prev = 0x%x, offset = %d offset prev = %d, ip hdr checksum = 0x%x, ip hdr checksum prev = 0x%x", thread_info[thread_index].packet_number[nInput], PktInfo->pkt_len, PktInfo2->pkt_len, PktInfo->flags, PktInfo2->flags, PktInfo->fragment_offset, PktInfo2->fragment_offset, PktInfo->ip_hdr_checksum, PktInfo2->ip_hdr_checksum);
      if (PktInfo->fragment_offset == 0) sprintf(&tmpstr[strlen(tmpstr)], " udp checksum = 0x%x, udp checksum prev = 0x%x", PktInfo->udp_checksum, PktInfo2->udp_checksum);
      printf("%s \n", tmpstr);
   }
   #endif

      bool fFragmentCompare = (PktInfo->flags & DS_PKT_FRAGMENT_ITEM_MASK) && (PktInfo->flags & DS_PKT_FRAGMENT_ITEM_MASK) == (PktInfo2->flags & DS_PKT_FRAGMENT_ITEM_MASK);  /* both current and previous packet contain identical fragment info ? */
      bool fPortCompare = isReservedUDP(PktInfo->dst_port) && PktInfo->dst_port == PktInfo2->dst_port;  /* both current and previous packet are sent to specific dst ports ? */

      if (
          (fFragmentCompare || fPortCompare) &&
          (
           #if 0  /* UDP checksum ignored, as noted above. Maybe there is some way to know when / when not */
           (PktInfo->udp_checksum == PktInfo2->udp_checksum) &&
           #endif
           (PktInfo->fragment_offset != 0 || PktInfo2->fragment_offset != 0 || PktInfo->pyld_len == PktInfo2->pyld_len) &&  /* compare UDP payload lengths if both fragment offsets are zero */
           PktInfo->pkt_len == PktInfo2->pkt_len &&  /* compare packet lengths */
           PktInfo->ip_hdr_checksum == PktInfo2->ip_hdr_checksum  /* compare IP header checksums, which implicitly compares IP header lengths */
          )
         ) {

         thread_info[thread_index].udp_redundant_discards[nInput]++;  /* increment number of redundant UDP transmissions discarded for input stream */

         #if 0  /* debug */
         printf(" *** discard UDP retransmit, pkt # = %u, checkum = 0x%x, pkt len = %d, udp discard count = %d \n", thread_info[thread_index].num_udp_packets[nInput]+1, PktInfo.udp_checksum, PktInfo.pkt_len,  thread_info[thread_index].udp_redundant_discards[nInput]);
         #endif

         return true;
      }
   }

   return false;
}
