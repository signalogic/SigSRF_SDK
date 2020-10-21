/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/mediaMin.c

 Copyright (C) Signalogic Inc. 2018-2020

 Description

   Application source code for packet + media processing, including:

     -telecom and analytics applications, e.g. SBC, lawful interception, ASR

     -standard operating mode

       -basic API interface to SigSRF pktlib, including packet push/pull queues, session create/delete and session get/set info
       -SigSRF pktlib packet/media thread usage, including multiple threads
       -static session creation based on session config files
       -dynamic session creation based on packet contents ("dynamic call mode"), supporting multistream pcaps and UDP flow ("dynamic call" mode)
       -support for pcaps with/without packet timestamps (wall clock packet arrival times)
       -SigSRF lib event logging, packet logging, packet time and loss stats

     -capacity measurement and test modes

       -multiple application threads, including all above SOP functionality per thread 
       -functional test
       -stress test

  Documentation

    ftp://ftp.signalogic.com/documentation/SigSRF

    (as of Oct 2019, most recent doc is ftp://ftp.signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

  Revision History

   Created Jul 2018 JHB, see also revision history in x86_mediaTest.c and packet_flow_media_proc.c
   Modified Jul 2018 CKJ, add pcap file I/O for testing
   Modified Jul 2018 JHB, fix problem with screen counters, pause before exiting to allow packet/media thread time to clean up and generate packet stats log
   Modified Jul 2018 JHB, enable user managed sessions and stream merging
   Modified Aug 2018 JHB, add reference to debugMode ("Mode") to allow new testing options (start packet/media thread before/after creating sessions, delete sessions while thread continues to run, etc)
   Modified Aug 2018 JHB, correctly push/pulls packets for multiple session test cases
   Modified Aug 2018 CKJ, add stress test with dynamic session create/delete, support multistream EVS and AMR pcaps
   Modified Sep 2018 JHB, change FlushSession() to reflect new usage of DS_SESSION_INFO_STATE in DSSetSessionInfo()
   Modified Sep 2018 JHB, add support for multiple stream groups, see StreamGroupOutputSetup() and GetNextGroupSessionIndex()
   Modified Sep 2018 CKJ, add dynamic call mode
   Modified Sep 2018 JHB, organize cmd line entry used for operational modes, stress tests, and options/flags.  Include RTP payload type in key used for dynamic session detection.  Additional checks for codec type estimation in dynamic session detection
   Modified Sep 2018 JHB, add multiple mediaMin thread support.  A typical command line is ./mediaTest -M0 -cx86 -itest_files/file.pcap -L -dN -r0 -Et -t6, where -Et invokes mediaMin, -tN specifies number of concurrent threads, and -dN specifies operating mode and media flags depending on input type, call type, etc (see flag definitions)
   Modified Oct 2019 JHB, add stop key, for clean exit in multithread tests
   Modified Oct 2018 JHB, add ANALYTICS_MODE flag, which enables pktlib FTRT mode and modifies operation of PushPackets() and PullPackets() to push/pull packets at ptime intervals (e.g. 20 msec for EVS and AMR NB/WB).  The objective is output packets, including signal processing outputs such as merged audio, at regular 20 msec intervals that can be verified with Wireshark wall clock stats
   Modified Oct 2018 JHB, make use of numPkts parameter added to DSPullPackets()
   Modified Oct 2018 JHB, fix bug in PullPackets() where packet output to jitter buffer and transcoded output pcap files was mixed together
   Modified Oct 2018 JHB, disable retry-wait in PushPackets() if push queue is full, replace with stderr notice if log level >= 8
   Modified Oct 2018 JHB, added variable rate push algorithm in PushPackets(), based on transcoded output queue levels.  Look for "average_push_rate"
   Modified Oct 2018 JHB, additional -dN cmd line entry flags
   Modified Nov 2018 JHB, implement simulated live UDP port packet input, using packet arrival times (timestamps) in pcap file (see -dN cmd line flag USE_PACKET_ARRIVAL_TIMES)
   Modified Nov 2018 JHB, add support for termN output_buffer_interval
   Modified Nov 2018 JHB, fix detection of AMR-NB SID packet in estimate_codec_type()
   Modified Nov 2018 JHB, add retry for stream group queue packet pull, if a packet is not available on strict 20 msec interval timing, then wait 1 msec and retry a few times.  Only active if USE_PACKET_ARRIVAL_TIMES mode is in effect.  See USE_GROUP_PULL_RETRY definition
   Modified Jan 2019 JHB, add -n cmd line entry for input reuse, to increase number of sessions for stress and capacity testing
   Modified Feb 2019 JHB, set EVS decode output Fs to 8 kHz instead of 16 kHz if stream group merging is active.  This reduces decode time and eliminates sampling rate conversion for narrowband merging, increasing session capacity
   Modified Sep 2019 JHB, modify codec detection algorithm in estimate_codec_type() to handle additional EVS bitrates, AMR octet-aligned cases, and improve best guess for cases when different codecs have identical RTP payload lengths
   Modified Oct 2019 JHB, add auto-quit (active when all inputs are files), wait for p/m threads to close before exit, and apply STREAM_GROUP_DEBUG_STATS flag to termination endpoints to enable stats for overrun compensation (for per channel overrun, silence frame detection and compression is enabled by default; see shared_include/streamlib.h)
   Modified Oct 2019 JHB, add multiple stream groups in dynamic call mode, treating each cmd line input spec ("-ixx") as a multistream call.  For example, for two pcaps on the cmd line each would (i) be considered a multistream call and (ii) have its own stream group
   Modified Oct 2019 JHB, add COMBINE_CALLS cmd line flag, which is similar to DYNAMIC_CALL, but combines all cmd line input specs into one call (and one stream group if applicable)
   Modified Nov 2019 JHB, added option to inject 1 sec timing markers in stream group audio output
   Modified Dec 2019 JHB, add -jN cmd line option for jitter buffer target and max delay values.  These can be set in session config files, but for dynamic calls there was not previously a way to control this
   Modified Jan 2020 JHB, add -RN cmd line option for repeat operation. N = number of times to repeat the cmd line, -R0 repeats indefinitely, no entry doesn't repeat.  See nRepeatsRemaining and fRepeatIndefinitely vars.  The ENABLE_REPEAT Mode flag is no longer used
   Modified Jan 2020 JHB, implement per-session flush, session delete in last flush state (for dynamic mode calls only). Upon deletion hSession[] handles are marked as deleted (set to -1), allowing their stats to stay available but preventing any further pktlib API usage
   Modified Jan 2020 JHB, PushPackets() now takes cur_time as a param instead of calling get_time().  This is a little faster but more importantly all input packet flows use the same reference when calculating packet timestamps vs. elapsed time. This fixes a slight variability seen in multiple input flow handling (for example with repeat enabled, stream group output FLCs might vary between repeats)
   Modified Jan 2020 JHB, add TERM_PKT_REPAIR_ENABLE and TERM_OVERRUN_SYNC_ENABLE flags to termN.uFlags during dynamic session creation
   Modified Feb 2020 JHB, make sure all sessions are fully deleted before exiting or repeating.  This is more efficient than sleeping some arbitary amount of time, and is also more reliable in the case of very long output wav files
   Modified Feb 2020 JHB, for real-time packet input (e.g. pcaps with packet timestamps, UDP input), move session flush to be immediately after input flow ends.  Session delete continues to take place after all queues are empty
   Modified Mar 2020 JHB, change DISABLE_SID_REPAIR flag to DISABLE_PACKET_REPAIR -- the flag, if included in -dN cmd line entry, now applies to both SID and media packet repair
   Modified Mar 2020 JHB, rework auto-adjust push rate algorithm and fix a problem it had when nReuseInputs is active. See comments in PushPackets() near ENABLE_AUTO_ADJUST_PUSH_RATE
   Modified Mar 2020 JHB, fix problem with "missed stream group interval" stats that were still accumulating after session flush
   Modified Mar 2020 JHB, add REPEAT_INPUTS flag to repeat input flows when applicable, for example wrapping pcap files back to start
   Modified Mar 2020 JHB, improve AppThreadSync() function to handle (i) waiting for master application thread and (ii) waiting for all threads
   Modified Mar 2020 JHB, rename file to mediaMin.c (from mediaTest_thread_app.c)
   Modified Mar 2020 JHB, update SetTimingInterval() to set termN.input_buffer_interval and termN.output_buffer_interval for both dynamic and static sessions
   Modified Mar 2020 JHB, modify PullPackets() to pull correct number of packets for telecom mode
   Modified Mar 2020 JHB, fix problem where SetIntervalTiming() was being called after session creation, instead of before. SetIntervalTiming() modifies session_data[] used during session creation
   Modified Apr 2020 JHB, rename ENABLE_FTRT_MODE flag to ANALYTICS_MODE, store stream group, event log, and packet log with _"am" suffix to make it easier in analyzing/comparing analytics vs. telecom mode output
   Modified Apr 2020 JHB, telecom mode updates:
                          -fix a few places where timing was incorrect; modified to look for combination of ((Mode & ANALYTICS_MODE) || term1.input_buffer_interval) to indicate "timed situations"
                          -set default jitter buffer max and target delay to 14 and 10
   Modified Apr 2020 JHB, clean up handling of DS_SESSION_INFO_DELETE_STATUS when exiting or repeating
   Modified Apr 2020 JHB, app_printf() enhancements
   Modified May 2020 JHB, add handling for TERM_IGNORE_ARRIVAL_TIMING and TERM_OOO_HOLDOFF_ENABLE flags
   Modified Jun 2020 JHB, fix bug where string size wasn't large enough to handle multiple session stats summary print out (just prior to program exit)
   Modified Jun 2020 JHB, move static session creation into StaticSessionCreate()
   Modified Jun 2020 JHB, add ENABLE_ALIGNMENT_MARKERS -dN cmd line option, to support visual inspection when deduplication algorithm is active (ENABLE_STREAM_GROUP_DEDUPLICATION flag)
   Modified Sep 2020 JHB, mods for compatibility with gcc 9.3.0: include math.h (for min/max functions), fix various security and "indentation" warnings
   Modified Oct 2020 JHB, tested with .pcapng input files after support added to pktlib for reading pcapng format
*/


/* Linux header files */

#include <stdio.h>
#include <pthread.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <signal.h>
#include <assert.h>
#include <stdarg.h>
#include <math.h>

/* mediaTest header file */

#include "mediaTest.h"  /* bring in vars declared in cmd_line_interface.c, including MediaParams[], PlatformParams, frameInterval[], and Mode */

/* lib header files */

#include "pktlib.h"     /* packet push/pull and session management APIs. Pktlib includes packet/media threads, packet handling and formatting, jitter buffers, session handling */
#include "voplib.h"     /* voplib provides an API interface to all codecs. Normally this is used by pktlib but can be accessed directly if needed */
#include "diaglib.h"    /* diagnostics including event and packet logging. Event logging includes default stats and optional stats depending on cmd line entry. Packet logging includes detailed packet stats */

#include "shared_include/session.h"    /* session management structs and definitions */
#include "shared_include/config.h"     /* configuration structs and definitions */
#include "shared_include/streamlib.h"  /* streamlib provides an API interface for stream group management. Normally this is used by pktlib but can be accessed directly if needed */

/* number of possible input streams, including streams that are re-used for multithread and high capacity testing */
#define MAX_INPUT_STREAMS MAX_SESSIONS  /* MAX_SESSIONS is defined in transcoding.h, which is included in mediaTest.h */

//#define LOG_OUTPUT  LOG_SCREEN_ONLY  /* screen output only */
//#define LOG_OUTPUT  LOG_FILE_ONLY  /* file output only */
#define LOG_OUTPUT  LOG_SCREEN_FILE  /* screen + file output (LOG_SCREEN_FILE defined in diaglib.h) */

#define ENABLE_MANAGED_SESSIONS
//  #define MERGE_BUFFER_SIZE  8000  /* default merge buffer size is 2000 samples = 1/4 sec at 8000 Hz sampling rate.  Uncommenting this define is an example of setting merge buffer size to another value, in this example 1 sec */
#define USE_GROUP_PULL_RETRY

#include "mediaMin.h"  /* bring in some struct typedefs and other definitions */
#include "cmdLineOpt.h"  /* cmd line handling */

/* following are standard operating modes, stress tests, and options that can be specified by -dN cmd line entry (N may be given in hex format, for example -d0xN).  Value of N is referred to in the source as "Mode" */

/* standard operating modes */

#define SESSION_CONFIG_FILE                   0  /* default mode (no -d entry), a session config file must be given on the cmd line, static sessions are created */
#define DYNAMIC_CALL                          1  /* treat each cmd line input spec ("-ixx") as a multistream call and dynamically create sessions as they appear.  If stream groups are enabled, each call has its own stream group.  If a session config file is given on the cmd line it's ignored */
#define COMBINE_CALLS                         2  /* similar to DYNAMIC_CALL, but combine all cmd line input specs into one call (and if stream groups are enabled, combine all group output into one group) */
#define ENABLE_STREAM_GROUP_DEDUPLICATION     4  /* applies a deduplication algorithm, which looks for similar content between stream group contributors and attempts to align similar streams. The objective is to reduce perceived reverb/echo due to duplicated streams. A typical scenario is a multipath (duplicated) endpoint with different latencies */
#define ENABLE_STREAM_GROUP_ASR               8  /* enable ASR processing on stream group output */

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
#define ENABLE_MERGE_DEBUG_STATS     0x10000000  /* enable packet/media thread internal audio merging debug stats */
#define ENABLE_MERGE_DEBUG_STATS_L2  0x20000000  /* reserved */
#define ENABLE_ALIGNMENT_MARKERS     0x40000000  /* when combined with the ENABLE_STREAM_GROUP_DEDUPLICATION flag, enables alignment markers to show the point at which streams were aligned (the deduplication algorithm uses cross correlation to align one or more streams) */
#define ENABLE_MEM_STATS             0x80000000  /* show mem usage stats in the event log */

//#define VALGRIND_DEBUG  /* enable when using Valgrind for debug */
#ifdef VALGRIND_DEBUG
#define VALGRIND_DELAY 100  /* usleep delay value in usec for allowing valgrind to run multithreaded apps on the same core */
#endif

static char prog_str[] = "mediaMin: packet media streaming for analytics and telecom applications on x86 and coCPU platforms, Rev 2.7, Copyright (C) Signalogic 2018-2020\n";

/* vars shared between app threads */

static HPLATFORM     hPlatform = -1;        /* initialized by DSAssignPlatform() API in DirectCore lib */
static int           debug_test_state;
static bool          fThreadSync1 = false;  /* flag used to coordinate app threads during first stage of initialization */
static bool          fThreadSync2 = false;  /* same, for second stage of initialization */
static bool          fQuit = false;         /* set if 'q' (quit) key is pressed */
static bool          fPause = false;        /* "" 'p' (pause).  Pauses operation, another 'p' resumes.  Can be combined with 'd' (display) key to read out internal p/m thread debug, capacity, stats, and other info */ 
static bool          fStop = false;         /* "" 's' (stop).  Stop prior to next repeat (only applies if -RN is entered on cmd line.  Intended for clean stop to repeating test, avoiding partial output files, especially when ENABLE_RANDOM_WAIT is active */
static unsigned int  num_app_threads = 1;   /* set to more than one if multiple mediaMin app threads are active. This is controlled by a mediaTest cmd line that includes "-Et -tn" options, where n is the number of app threads (see SigSRF documentation) */
static int           num_pktmed_threads = 0;/* number of packet/media threads running */
static int           log_level = 0;         /* set in LoggingSetup() */
static bool          fStressTest;           /* determined from cmd line options, number of app threads, and session re-use */
static bool          fCapacityTest;         /*    ""    ""   */
static char          szSessionName[MAX_INPUT_STREAMS][384] = {{ "" }};  /* set in LoggingSetup() which should always be called */
static bool          fInputsAllFinite = true;  /* set to false if inputs include UDP port or USB audio.  Default is true if all inputs are pcap or other file */
static bool          fAutoQuit = false;     /* fAutoQuit determines whether program stops automatically.  This is the default for cmd lines with (i) all inputs are files (i.e. no UDP or USB audio inputs) and (ii) no repeating stress or capacity tests */
static bool          fRepeatIndefinitely = false;  /* true if -R0 is given on the cmd line */
static bool          fNChannelWavOutput = false;

/* per application thread info */

static THREAD_INFO thread_info[MAX_MEDIAMIN_THREADS] = {{ 0 }};  /* MAX_MEDIAMIN_THREADS defined in mediaTest.h */
static int average_push_rate[MAX_MEDIAMIN_THREADS] = { 0 };
static int nRepeatsRemaining[MAX_MEDIAMIN_THREADS] = { 0 };

/* misc local definitions (most definitions are in mediaTest.h and mediaMin.h) */

#define SESSION_MARKED_AS_DELETED          0x80000000  /* flag used to mark hSessions[] entries as deleted during dynamic call operation */
#define TIMER_INTERVAL                     1           /* timer value in seconds for CREATE_DELETE_TEST_PCAP test mode */
#define WAIT_FOR_MASTER_THREAD             1           /* mode values used in AppThreadSync() local function */
#define WAIT_FOR_ALL_THREADS               2

#define APP_PRINTF_SAMELINE                1           /* app_printf() flags */
#define APP_PRINTF_NEWLINE                 2
#define APP_PRINTF_THREAD_INDEX_SUFFIX     4
#define APP_PRINTF_EVENT_LOG               8
#define APP_PRINTF_EVENT_LOG_NO_TIMESTAMP  0x10

/* local functions */

/* logging and configuration setup */

#if (LOG_OUTPUT != LOG_SCREEN_ONLY)
FILE* fp_sig_lib_log = NULL;
char sig_lib_log_filename[] = { "sig_lib_log.txt" };  /* default event log filename */
#endif

#define LOG_EVENT_SETUP        1
#define LOG_PACKETSTATS_SETUP  2

void LoggingSetup(DEBUG_CONFIG*, int setup_type);  /* logging setup */
void GlobalConfig(GLOBAL_CONFIG* gbl_cfg);  /* lib config */
void DebugSetup(DEBUG_CONFIG* dbg_cfg);  /* lib debug config */

/* I/O setup */

void InputSetup(int thread_index);
void JitterBufferOutputSetup(int thread_index);
void TranscodedOutputSetup(int thread_index);
void StreamGroupOutputSetup(HSESSION hSessions[], int nInput, int thread_index);

/* application thread helper functions */

void ThreadWait(int when, int thread_index);
void AppThreadSync(unsigned int, bool* fThreadSync, int thread_index);
void PmThreadSync(int thread_index);

/* local wrapper functions for DSPushPackets() and DSPullPackets(), including pcap read/write, queue full check, etc */
  
int PushPackets(uint8_t* pkt_in_buf, HSESSION hSessions[], SESSION_DATA session_data[], int nSessions, uint64_t cur_time, int thread_index);
int PullPackets(uint8_t* pkt_out_buf, HSESSION hSessions[], SESSION_DATA session_data[], unsigned int uFlags, unsigned int pkt_buf_len, int thread_index);
void FlushCheck(HSESSION hSessions[], uint64_t cur_time, uint64_t queue_check_time[], int thread_index);

/* helper functions for creating and managing packet/media threads */

int StartPacketMediaThreads(int num_pm_threads, int thread_index);

/* helper functions for creating and managing sessions */

int ReadSessionConfig(SESSION_DATA session_data[], int thread_index);
int StaticSessionCreate(HSESSION hSessions[], SESSION_DATA session_data[], int nSessionsConfigured, int thread_index);
void SetIntervalTiming(SESSION_DATA session_data[]);
void reset_dynamic_session_info(int);
unsigned int GetSessionFlags();

/* stress test helper functions */

int TestActions(HSESSION hSessions[], int thread_index);
void handler(int signo);
void TimerSetup();

/* counters, keyboard handling, and output */

void UpdateCounters(uint64_t, int);  /* update screen counters */
bool ProcessKeys(HSESSION hSessions[], uint64_t, DEBUG_CONFIG*, int);  /* process keyboard command input */
void app_printf(unsigned int uFlags, int thread_index, const char* fmt, ...);


/* mediaMin application entry point. Program and multithreading notes:

  -one mediaMin application thread is active if mediaMin is run from the cmd line. This includes standard operating mode at low capacity 
  -multiple mediaMin application threads may be active if invoked from the mediaTest cmd line, using the -Et and -tN arguments. This is the case for (i) high capacity operation and (ii) stress tests
  -in the case of multiple mediaMin threads, the var "thread_index" indicates the current thread
  -in all cases, thread_index = 0 is the master mediaMin app thread. The master thread handles initialization, housekeeping, and exit cleanup. In addition the master thread manages packet/media threads, starting one or more p/m threads depending on cmd line entry
  -application threads are separate from packet/media threads -- these should not be confused. Packet/media threads run in the pktlib shared library. Section 5, High Capacity Operation, in the SigSRF documentation includes htop screen caps showing both application and packet/media threads, and notes about CPU core usage, thread affinity, and other multithreading issues 
  -mediaMin accepts the same command line as mediaTest, except that mediaMin (i) recognizes -dN entry for operating mode options (ignored by mediaTest), and (ii) ignores -Ex and -tN entry, which is used only by mediaTest (ignored by mediaMin)
*/

#ifdef MEDIAMIN  /* MEDIAMIN is defined in the mediaMin Makefile. This is true when mediaMin is run from the command line, in which case only one mediaMin application thread is active */
int main(int argc, char **argv) {
#else  /* MEDIAMIN is not defined in the mediaTest Makefile. This is the case when mediaTest is run from the command line with the -Et and -tN options, in which case (i) N mediaMin application threads are active and (ii) the mediaMin_thread() function is included in the mediaTest build */
void* mediaMin_thread(void* thread_arg) {  /* see the "executionMode[0]" switch statement in x86_mediaTest.c.  In that switch statement, the 't' case arrives here */
#endif

HSESSION       hSessions[MAX_SESSIONS] = { 0 };
SESSION_DATA   session_data[MAX_SESSIONS] = {{ 0 }};

unsigned char  pkt_in_buf[32*MAX_RTP_PACKET_LEN] = { 0 }, pkt_out_buf[32*MAX_RTP_PACKET_LEN] = { 0 };

DEBUG_CONFIG dbg_cfg = { 0 };  /* struct used for lib debug configuration; see shared_include/debug.h */
GLOBAL_CONFIG gbl_cfg = { 0 };

int i, j, nSessionsConfigured = 0, nRemainingToDelete = 0, thread_index = 0;  /* mediaMin application thread index (normally zero, except for high capacity and stress test situations, see above comments) */

unsigned long long cur_time = 0, base_time = 0;
uint64_t interval_count = 0, queue_check_time[MAX_SESSIONS] = { 0 };
bool fExitErrorCond, fRepeatFromStart = false;  /* fRepeatFromStart is set true "start" or "session_create" labels are used. This happens if -RN cmd line entry is given (look for "nRepeat") or certain stress test types are specified */
#define MAX_APP_STR_LEN 2000
char tmpstr[MAX_APP_STR_LEN];

  	if (isMasterThread) {  /* print banner including program and lib version info, copyright */
      printf("%s", prog_str);
      printf("  SigSRF libraries in use: DirectCore v%s, pktlib v%s, streamlib v%s, voplib v%s, alglib v%s, diaglib v%s, cimlib v%s\n", HWLIB_VERSION, PKTLIB_VERSION, STREAMLIB_VERSION, VOPLIB_VERSION, ALGLIB_VERSION, DIAGLIB_VERSION, CIMLIB_VERSION);
   }

   #ifdef MEDIAMIN  /* running from cmd line, mediaMin is running as a process */

   if (!cmdLineInterface(argc, argv, CLI_MEDIA_APPS)) exit(EXIT_FAILURE);  /* parse command line and set MediaParams, PlatformParams, frameInterval, and pktStatsLogFile, use_log_file, and others.  See mediaTest.h and cmd_line_interface.c */
   thread_index = 0;
   printf("mediaMin start, cmd line execution\n");

   #else  /* running as either (i) a function call or (ii) one or more threads created by mediaTest.  In either case, command line has already been processed by mediaTest */

   thread_index = *((int*)thread_arg) & 0xff;
   num_app_threads = (*((int*)thread_arg) & 0xff00) >> 8;

   if (num_app_threads) {  /* mediaMin is running as one or more application level threads */

      printf("mediaMin start, thread execution, num threads = %d, thread_index = %d\n", num_app_threads, thread_index);
      free(thread_arg);
   }
   else {  /* mediaMin is running as function call */

      printf("mediaMin start, function call execution\n");
      num_app_threads = 1;
   }

   #endif  /* #ifdef MEDIAMIN */

   if (Mode == -1) Mode = 0;  /* default value if no cmd line entry given is -1 (Mode is defined in mediaTest.h, same as debugMode which is set on the command line by -d argument) */

   if (nRepeat == 0) fRepeatIndefinitely = true;  /* nRepeat is initialized in cmd_line_interface.c from -RN cmd line entry (if none nRepeat = -1). Note that some stress tests already have repeat built in, so -RN entry may be ignored or treated differently in those cases */
   nRepeatsRemaining[thread_index] = nRepeat;  /* each app thread keeps an independent repeat count, as they may repeat at different times (for example if ENABLE_RANDOM_WAIT is set) */

   if ((Mode & 0xff) < 0x20) {

      if (isMasterThread) {

         printf(" Standard Operating Mode\n");
         if ((Mode & 0xf) == SESSION_CONFIG_FILE) printf("  default mode, create sessions from session config file (specified with -C on cmd line)\n");
         else if ((Mode & 0xf) == DYNAMIC_CALL) printf("  treat pcap as multistream call, create sessions dynamically as they appear\n");
         if (Mode & USE_PACKET_ARRIVAL_TIMES) printf("  use packet arrival times to control push rate\n");
      }

      if (Mode & DYNAMIC_CALL) thread_info[thread_index].fDynamicCallMode = true;  /* set for all app threads */
   }

   if ((Mode & 0xff) > 0x10) {

      if (isMasterThread) {

         printf(" Test Mode\n");
         bool fTestModePrinted = false;
         if (Mode & CREATE_DELETE_TEST) { printf("  test mode, create, delete, and recreate sessions.  Automatically repeats\n"); fTestModePrinted = true; }
         if (Mode & CREATE_DELETE_TEST_PCAP) { printf("  test mode, dynamically create sessions from pcap with initial static session.  Automatically repeats\n"); fTestModePrinted = true; }
         char repeatstr[20];
         sprintf(repeatstr, "%d times", nRepeat);
         if (nRepeat >= 0) { printf("  repeat %s\n", nRepeat == 0 ? "indefinitely" : repeatstr); fTestModePrinted = true; }
         if (Mode & ENABLE_RANDOM_WAIT) { printf("  random wait at start and between repeats enabled\n"); fTestModePrinted = true; }
         if (Mode & START_THREADS_FIRST) { printf("  start packet / media threads first\n"); fTestModePrinted = true; }
         if (Mode & ENERGY_SAVER_TEST) { printf("  force an initial 30+ sec delay to test packet/media thread energy saver mode\n"); fTestModePrinted = true; }
         if (!fTestModePrinted) printf("  none\n");
      }
   }

   if (Mode & 0xffff00) {

      if (isMasterThread) {

         printf(" Options Enabled / Disabled\n");
         if (Mode & ENABLE_STREAM_GROUPS) printf("  stream group(s) enabled\n");
         if (Mode & ENABLE_WAV_OUTPUT) printf("  stream group wav file output enabled\n");
         if (Mode & ANALYTICS_MODE) printf("  Analytics mode with ptime push/pull rate enabled\n");
         if (Mode & ENABLE_MERGE_DEBUG_STATS) printf("  audio merge debug stats output enabled\n");
         if (Mode & ENABLE_AUTO_ADJUST_PUSH_RATE) printf("  auto-adjust dynamic packet push rate\n");
         if (Mode & DISABLE_DTX_HANDLING) printf("  DTX handling disabled\n");
         if (Mode & DISABLE_FLC) printf("  FLC (frame loss concealment) on stream group output disabled\n");
//         if (Mode & DISABLE_OUTPUT_BUF_INTERVAL) printf("  output buffer interval field in termN definitions is disabled\n");
         if (Mode & ENABLE_ONHOLD_FLUSH_DETECT) printf("  on-hold flush detection for audio merge contributors enabled (this is deprecated)\n");
         if (Mode & ENABLE_TIMING_MARKERS) printf("  debug: 1 sec timing markers will be injected in stream group output\n");
         if (Mode & ENABLE_PACKET_INPUT_ALARM) printf("  input packet alarm enabled, if DSPushPackets() is not called for the alarm time limit a warning will show in the event log\n");
         if (Mode & DISABLE_AUTOQUIT) printf("  auto-quit disabled\n");
      }
   }

   AppThreadSync(WAIT_FOR_MASTER_THREAD, &fThreadSync1, thread_index);  /* app threads wait here for master thread to do following first stage initialization */

   if (isMasterThread) {

      fStressTest = (Mode & CREATE_DELETE_TEST) || (Mode & CREATE_DELETE_TEST_PCAP);  /* set fStressTest if stress test options have been given */
      fCapacityTest = num_app_threads > 1 || nReuseInputs;  /* set fCapacityTest if load/capacity options have been given */

      fAutoQuit = !(Mode & DISABLE_AUTOQUIT) && !fStressTest && !fRepeatIndefinitely && fInputsAllFinite;

   /* set up timer for continuous pcap test mode (only needed for stress tests) */

      if (Mode & CREATE_DELETE_TEST_PCAP) TimerSetup();

   /* logging, library init, and config */

      LoggingSetup(&dbg_cfg, LOG_EVENT_SETUP);  /* set up library event logging, dbg_cfg struct is defined in shared_include/config.h */

      LoggingSetup(&dbg_cfg, LOG_PACKETSTATS_SETUP);  /* set up packet stats logging */

      GlobalConfig(&gbl_cfg);  /* configure libraries, gbl_cfg struct is defined in shared_include/config.h */

      DebugSetup(&dbg_cfg);  /* set up debug items (see cmd line debug flags above) */

   /* configure pktlib */

      DSConfigPktlib(&gbl_cfg, &dbg_cfg, DS_CP_INIT | DS_CP_DEBUGCONFIG | DS_CP_GLOBALCONFIG);

   /* make a few initial event log entries (the first entries are made by DSConfigPktlib) */

      Log_RT(4 | DS_LOG_LEVEL_FILE_ONLY, prog_str);  /* include program version in event log */
      Log_RT(4, "mediaMin INFO: event log setup complete, log file %s, log level = %d ", dbg_cfg.szEventLogFilePath, dbg_cfg.uLogLevel);

      #if 0  /* example of dynamic dbg_cfg modification (for example enabling and disabling event log), should it be needed. Note that local GLOBAL_CONFIG and DEBUG_CONFIG structs must be maintained in order to allow dynamic updating without loss of prior settings (pktlib doesn't maintain per-app thread copies) */
      dbg_cfg.uEventLogMode |= DS_EVENT_LOG_DISABLE;
      DSConfigPktlib(NULL, &dbg_cfg, DS_CP_DEBUGCONFIG);  /* disable */

      dbg_cfg.uEventLogMode &= ~DS_EVENT_LOG_DISABLE;
      DSConfigPktlib(NULL, &dbg_cfg, DS_CP_DEBUGCONFIG);  /* re-enable */
      #endif

   /* configure voplib and streamlib */
   
      DSConfigVoplib(NULL, &dbg_cfg, DS_CV_INIT | DS_CV_DEBUGCONFIG);

      DSConfigStreamlib(NULL, &dbg_cfg, DS_CS_INIT | DS_CV_DEBUGCONFIG);

   /* ask DirectCore lib for a platform handle */

      hPlatform = DSAssignPlatform(NULL, PlatformParams.szPlatformDesignator, 0, 0, 0);  /* create a platform handle, needed for managing concurrency, containers, etc.  If PlatformParams is not filled in (the mediaMin app does this in cmd_line_interface.c), the platform designator default is "x86" */

   /* start packet / media thread(s) */

      if (Mode & START_THREADS_FIRST) if (StartPacketMediaThreads(num_app_threads > 1 ? NUM_PKTMEDIA_THREADS : 1, thread_index) < 0) goto cleanup;

      fThreadSync1 = true;  /* release any app threads waiting in AppThreadSync() */
   }

/* first stage initialization complete */

start:  /* note - label used only if test mode repeats are enabled */

/* session configuration and packet I/O init */

   if (thread_info[thread_index].fDynamicCallMode) nSessionsConfigured = 0;
   else {
      nSessionsConfigured = ReadSessionConfig(session_data, thread_index);
      if (!nSessionsConfigured) goto cleanup;
   }

   InputSetup(thread_index);
   TranscodedOutputSetup(thread_index);

   if (thread_info[thread_index].init_err && !fThreadSync2) goto cleanup;  /* setup error occurred.  If this is first time through we clean up and exit, if not, then the user can see which threads / repeat runs have the error and quit as needed */

   #if 0
   printf("hPlatform = 0x%x, nSessionsConfigured = %d, nInPcapFiles = %d, nOutPcapFiles = %d\n", hPlatform, nSessionsConfigured, thread_info[thread_index].nInPcapFiles, thread_info[thread_index].nOutPcapFiles);
   #endif

/* second stage initialization complete */

   memset(hSessions, 0xff, sizeof(hSessions));  /* initialize all session handles to -1. Valid session handles are >= 0 */

session_create:  /* note - label used only if test mode repeats are enabled */

   if (!thread_info[thread_index].fDynamicCallMode) {  /* if cmd line not in dynamic call mode, create static sessions */

      if (StaticSessionCreate(hSessions, session_data, nSessionsConfigured, thread_index) < 0) goto cleanup;   /* error out if static sessions were configured but none created */
   }

/* all packet I/O and static session creation (if any) complete, sync app threads before continuing */

   AppThreadSync(WAIT_FOR_MASTER_THREAD, &fThreadSync2, thread_index);  /* app threads wait here for master app thread to do following second stage initialization, which includes packet/media thread configuration / start */

/* start packet / media thread(s), if not already started  */

   if (isMasterThread && !fThreadSync2) {

      if (!(Mode & START_THREADS_FIRST)) if (StartPacketMediaThreads(num_app_threads > 1 ? NUM_PKTMEDIA_THREADS : 1, thread_index) < 0) goto cleanup;

      fThreadSync2 = true;  /* release any app threads still waiting in AppThreadSync() */
   }

   if ((num_app_threads > 1 && (Mode & ENABLE_RANDOM_WAIT)) || (Mode & ENERGY_SAVER_TEST)) ThreadWait(0, thread_index);  /* staggered start for threads */

   if (!fRepeatFromStart) app_printf(APP_PRINTF_NEWLINE, thread_index, "Starting packet push/pull loop, press 'q' to exit");


/* all initialization complete, begin continuous packet push / pull loop */

   do {

      if (fPause) continue;  /* skip to end of loop if pause is in effect */

      cur_time = get_time(USE_CLOCK_GETTIME); if (!base_time) base_time = cur_time;

      if (Mode & USE_PACKET_ARRIVAL_TIMES) PushPackets(pkt_in_buf, hSessions, session_data, thread_info[thread_index].nSessionsCreated, cur_time, thread_index);  /* in this mode packets are pushed based on arrival time (for pcaps a push happens when elapsed time exceeds the packet's arrival timestamp) */

   /* otherwise we push packets according to a specified interval. Options include (i) pushing packets as fast as possible (-r0 cmd line entry), (ii) N msec intervals (cmd line entry -rN), and an average push rate based on output queue levels (the latter can be used with pcaps that don't have arrival timestamps) */

      if (cur_time - base_time < interval_count*frameInterval[0]*1000) continue; else interval_count++;  /* if the time interval has elapsed, push and pull packets and increment the interval. Comparison is in usec */

   /* read packets from input flows, push to packet/media threads */

      if (!(Mode & USE_PACKET_ARRIVAL_TIMES)) PushPackets(pkt_in_buf, hSessions, session_data, thread_info[thread_index].nSessionsCreated, cur_time, thread_index);

      //#define SINGLE_STEP
      #ifdef SINGLE_STEP
      if (isMasterThread) { printf("After push\n"); fPause = 1; continue; }
      #endif

   /* pull available packets from packet/media threads, write to output flows */

      PullPackets(pkt_out_buf, hSessions, session_data, DS_PULLPACKETS_JITTER_BUFFER, sizeof(pkt_out_buf), thread_index);
      PullPackets(pkt_out_buf, hSessions, session_data, DS_PULLPACKETS_TRANSCODED, sizeof(pkt_out_buf), thread_index);
      PullPackets(pkt_out_buf, hSessions, session_data, DS_PULLPACKETS_STREAM_GROUP, sizeof(pkt_out_buf), thread_index);

   /* check for end of input flows, end of output packet flows sent by packet/media threads, flush sessions if needed */

      FlushCheck(hSessions, cur_time, queue_check_time, thread_index);

   /* update screen counters */

      UpdateCounters(cur_time, thread_index);

   /* update test conditions as needed. Note that repeating tests exit the push/pull loop here, after each thread detects end of input and flushes sessions. Also auto-quit (single app thread, no repeat) exits here */

      if (!TestActions(hSessions, thread_index)) break;

   } while (!ProcessKeys(hSessions, cur_time, &dbg_cfg, thread_index));  /* process interactive keyboard commands */

 
/* session deletion */

   for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) nRemainingToDelete++;  /* see if any sessions remain to be deleted, depending on operational mode. For dynamic calls all sessions may already be deleted */

   if (nRemainingToDelete) {
   
      sprintf(tmpstr, "Deleting %d session%s [index] hSession/flush state", nRemainingToDelete, nRemainingToDelete > 1 ? "s" : "");
      for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) sprintf(&tmpstr[strlen(tmpstr)], "%s [%d] %d/%d", i > 0 ? "," : "", i, hSessions[i], thread_info[thread_index].flush_state[i]);

      app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_THREAD_INDEX_SUFFIX, thread_index, tmpstr);  /* show session delete info onscreen */
      Log_RT(4 | DS_LOG_LEVEL_FILE_ONLY, "mediaMin INFO: %s ", tmpstr);  /* include session delete info in event log */

      for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) {  /* delete sessions */
         DSDeleteSession(hSessions[i]);  /* pktlib API */
         thread_info[thread_index].nSessionsDeleted++;
      }
   }

   app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_THREAD_INDEX_SUFFIX, thread_index, "Total sessions created = %d, deleted = %d", thread_info[thread_index].total_sessions_created, thread_info[thread_index].nSessionsDeleted);


/* cleanup before exiting or repeating */

cleanup:

/* make sure all sessions are fully deleted before exiting or repeating.  Notes:

   -there could be some wait time if (i) wav file output has been specified for stream groups (especially N-channel wav file generation) or (ii) a lot of sessions are open
   -for dynamic call operation, if the cmd line had multiple calls, sessions for already completed calls should aleady be deleted, but the last call(s) might still be in the process of deletion 
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

         run = 0;            /* instruct packet/media thread(s) to exit.  Note that in case of error condition, none may have been started or still be running */

         if (!fExitErrorCond) {

            base_time = get_time(USE_CLOCK_GETTIME);
            unsigned long long check_time = 0;
            uint8_t uQuitMessage = 0, fQKey = false;

            while (!fPMMasterThreadExit) {  /* wait for packet/media threads to exit, and also for master p/m thread to collate, analyze, and write out packet log if packet history stats are enabled. Packet history logging + analysis can take a while if 100k+ packets were pushed/pulled.  Max packets for packet logging is somewhere around 300k (can be changed in packet_flow_media_proc.c, look for MAX_PKT_STATS) */

               if (fPMThreadsClosing && !uQuitMessage) {

                  sprintf(tmpstr, "Waiting for p/m threads to close%s", fNChannelWavOutput ? ", N-channel wav file processing," : "");

                  if (use_log_file) {  /* use_log_file is set if packet history logging is enabled; see LoggingSetup() below. Cmd line entry -L[filename] enables packet history logging */

                     int num_input_pkts = DSGetThreadInfo(thread_index, DS_THREAD_INFO_NUM_INPUT_PKT_STATS, NULL); 
                     int num_pulled_pkts = DSGetThreadInfo(thread_index, DS_THREAD_INFO_NUM_PULLED_PKT_STATS, NULL); 

                     sprintf(&tmpstr[strlen(tmpstr)], " and packet history logging and analyis of %d input packets and %d pulled packets", num_input_pkts, num_pulled_pkts);
                  }

                  printf("%s, press 'q' if needed ...\n", tmpstr);
                  uQuitMessage = 1;
               }

               cur_time = get_time(USE_CLOCK_GETTIME);

               if (!check_time) check_time = cur_time;

               if (uQuitMessage < 2 && cur_time - check_time > 3*1000000L) {  /* after 3 sec */

                  if (!fPMThreadsClosing) {
                     sprintf(tmpstr, "P/M threads still not closing after 3 sec, there may be a problem");  /* this should not happen, if so it may indicate a problem */
                     printf("%s, press 'q' if needed ... \n", tmpstr);
                  }

                  uQuitMessage = 2;
               }

               if (cur_time - check_time > 30*1000000L) {  /* print a dot every 30 sec */
                  printf(".");
                  check_time = cur_time;
               }

               usleep(250000L);  /* check keybd every 1/4 sec */

               char key = (char)tolower(getkey());
               if (key == 'q') { fQKey = true; break; }  /* quit key ? */
            }

            if (fQKey) printf("\n");
         }
      }
   }

/* close input file descriptors */

   for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) if (thread_info[thread_index].pcap_in[i]) { fclose(thread_info[thread_index].pcap_in[i]); thread_info[thread_index].pcap_in[i] = NULL; }

   for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) if (thread_info[thread_index].fp_pcap_jb[i]) { fclose(thread_info[thread_index].fp_pcap_jb[i]); thread_info[thread_index].fp_pcap_jb[i] = NULL; }

   if (!fExit && (Mode & CREATE_DELETE_TEST)) {

      printf("Recreate test enabled, rerunning test from session create, total sessions created = %d\n", thread_info[thread_index].total_sessions_created);

      for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) {
         thread_info[thread_index].flush_state[i] = 0;
         queue_check_time[i] = 0;
      }

      thread_info[thread_index].nSessionsCreated = 0;
      nRemainingToDelete = 0;

      for (i=0; i<MAX_STREAM_GROUPS; i++) {
         thread_info[thread_index].fFirstGroupPull[i] = false;
         for (j=0; j<MAX_INPUT_REUSE; j++) thread_info[thread_index].fGroupTermCreated[i][j] = false;
      }

      base_time = 0;
      interval_count = 0;

      InputSetup(thread_index);

      fRepeatFromStart = true;
      goto session_create;
   }

   for (i=0; i<thread_info[thread_index].nOutPcapFiles; i++) if (thread_info[thread_index].pcap_out[i]) { fclose(thread_info[thread_index].pcap_out[i]); thread_info[thread_index].pcap_out[i] = NULL; }

   for (i=0; i<MAX_STREAM_GROUPS; i++) {

      if (thread_info[thread_index].fp_pcap_group[i]) { fclose(thread_info[thread_index].fp_pcap_group[i]); thread_info[thread_index].fp_pcap_group[i] = NULL; }
      strcpy(thread_info[thread_index].szGroupName[i], "");
      thread_info[thread_index].fFirstGroupPull[i] = false;
      for (j=0; j<MAX_INPUT_REUSE; j++) thread_info[thread_index].fGroupTermCreated[i][j] = false;
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
      }

      #if 0  /* allow stream group interval/pull stats for repeating tests */
      thread_info[thread_index].group_interval_stats_index = 0;
      thread_info[thread_index].group_pull_stats_index = 0;
      memset(thread_info[thread_index].GroupIntervalStats, 0, sizeof(GROUPINTERVALSTATS)*MAX_GROUP_STATS);
      memset(thread_info[thread_index].GroupPullStats, 0, sizeof(GROUPPULLSTATS)*MAX_GROUP_STATS);
      #endif

      base_time = 0;
      interval_count = 0;

      reset_dynamic_session_info(thread_index);

      if (Mode & ENABLE_RANDOM_WAIT) ThreadWait(1, thread_index);  /* if random wait is enabled, then each app thread waits a random number of msec */

      thread_info[thread_index].nSessionsCreated = 0;
      nRemainingToDelete = 0;

   /* reset packet stats history before repeating, JHB Jan2020:

      -we could write out packet log to filename with some type of "repeatN" suffix so a log is saved for each repeat, instead of writing once at end of the test run
      -but writing out packet stats history and analyzing input vs. jitter buffer output takes time, and if we do it on every repeat cycle it will cause a delay in the mediaMin application thread
   */

      if (isMasterThread) DSWritePacketStatsHistoryLog(0, DS_WRITE_PKT_STATS_HISTORY_LOG_THREAD_INDEX | DS_WRITE_PKT_STATS_HISTORY_LOG_RESET_STATS, NULL);

      sprintf(tmpstr, "Cmd line completed, repeating");
      if (!fRepeatIndefinitely) sprintf(&tmpstr[strlen(tmpstr)], ", number of repeats remaining %d", nRepeatsRemaining[thread_index]+1);
      else sprintf(&tmpstr[strlen(tmpstr)], " ...");
      app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_THREAD_INDEX_SUFFIX, thread_index, tmpstr);

      fRepeatFromStart = true;
      goto start;
   }

/* clean up and exit */

   if (isMasterThread) {

      #if (LOG_OUTPUT != LOG_SCREEN_ONLY)
      if (fp_sig_lib_log) {
         fclose(fp_sig_lib_log);
         fp_sig_lib_log = NULL;
      }
      else if (dbg_cfg.uEventLogFile) {
         fclose(dbg_cfg.uEventLogFile);
      }
      #endif

      if (hPlatform != -1) DSFreePlatform((intptr_t)hPlatform);  /* free platform handle */
   }

   if (!fExitErrorCond && !fStressTest && !fCapacityTest && (thread_info[thread_index].fDynamicCallMode || (Mode & ENABLE_STREAM_GROUPS))) {

      sprintf(tmpstr, "===== mediaMin stats");
      if (num_app_threads > 1) sprintf(&tmpstr[strlen(tmpstr)], " (%d)", thread_index);
      strcat(tmpstr, "\n");

      if (thread_info[thread_index].fDynamicCallMode) for (i=0; i<thread_info[thread_index].dynamic_session_stats_index; i++) {
  
         char szSessInfo[100];
         sprintf(szSessInfo, "\t[%d] hSession %d, codec = %s, bitrate = %d, payload type = %d \n", i, thread_info[thread_index].DynamicSessionStats[i].hSession, thread_info[thread_index].DynamicSessionStats[i].codecstr, thread_info[thread_index].DynamicSessionStats[i].bitrate, thread_info[thread_index].DynamicSessionStats[i].payload_type);

         if (strlen(tmpstr) + strlen(szSessInfo) < sizeof(tmpstr)) sprintf(&tmpstr[strlen(tmpstr)], "%s", szSessInfo);  /* if enough sessions and/or repeats we need to split up printouts, JHB Jun2020 */
         else {
            app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_EVENT_LOG, thread_index, tmpstr);
            tmpstr[0] = (char)0;
         }
      }

      if (strlen(tmpstr)) app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_EVENT_LOG, thread_index, tmpstr);

      if (Mode & ENABLE_STREAM_GROUPS) app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_EVENT_LOG_NO_TIMESTAMP, thread_index, "\tMissed stream group intervals = %d", thread_info[thread_index].group_interval_stats_index);

      for (i=0; i<thread_info[thread_index].group_interval_stats_index; i++) {

         sprintf(tmpstr, "\t[%d] missed stream group interval = %d, hSession = %d", i, thread_info[thread_index].GroupIntervalStats[i].missed_interval, thread_info[thread_index].GroupIntervalStats[i].hSession);
         if (thread_info[thread_index].GroupIntervalStats[i].repeats) sprintf(&tmpstr[strlen(tmpstr)], " %dx", thread_info[thread_index].GroupIntervalStats[i].repeats+1);

         app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_EVENT_LOG_NO_TIMESTAMP, thread_index, tmpstr);
      }

      if (Mode & ENABLE_STREAM_GROUPS) app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_THREAD_INDEX_SUFFIX | APP_PRINTF_EVENT_LOG_NO_TIMESTAMP, thread_index, "\tMarginal stream group pulls = %d", thread_info[thread_index].group_pull_stats_index);

      for (i=0; i<thread_info[thread_index].group_pull_stats_index; i++) {
         sprintf(tmpstr, "\t[%d] marginal stream group pull at %d, retries = %d, hSession = %d", i, thread_info[thread_index].GroupPullStats[i].retry_interval, thread_info[thread_index].GroupPullStats[i].num_retries, thread_info[thread_index].GroupPullStats[i].hSession);
         app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_EVENT_LOG_NO_TIMESTAMP, thread_index, tmpstr);
      }
   }

   sprintf(tmpstr, "mediaThread app end");
   if (num_app_threads > 1) sprintf(&tmpstr[strlen(tmpstr)], " (%d)", thread_index);
   printf("%s\n", tmpstr);

   #ifdef MEDIAMIN
   return 0;
   #else
   return NULL;
   #endif
}


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

/* read session configuration file and create static sessions.  Note this depends on -dN cmd line entry, see Mode var comments above */

int ReadSessionConfig(SESSION_DATA session_data[], int thread_index) {

char default_session_config_file[] = "session_config/packet_test_config";
char* session_config_file;
FILE* session_cfg_fp = NULL;
int nSessionsConfigured = 0;
char tmpstr[1024];

   if (thread_info[thread_index].init_err) return 0;

   if (strlen(MediaParams[0].configFilename) == 0 || access(MediaParams[0].configFilename, F_OK) == -1) {

      if (strlen(MediaParams[0].configFilename) == 0) goto err;
      
      strcpy(tmpstr, "../");  /* try up one subfolder, in case the cmd line entry forgot the "../" prefix */
      strcat(tmpstr, MediaParams[0].configFilename);

      if (access(tmpstr, F_OK) == -1) {
err:
         printf("Specified config file: %s does not exist, using default file\n", MediaParams[0].configFilename);
         session_config_file = default_session_config_file;
      }
      else session_config_file = tmpstr;
   }
   else session_config_file = MediaParams[0].configFilename;

   printf("Opening session config file: %s\n", session_config_file);

/* open session config file */

   session_cfg_fp = fopen(session_config_file, "r");

   if (session_cfg_fp == NULL) {

      fprintf(stderr, "Error: SessionConfiguration() says failed to open static session config file %s, exiting mediaMin (%d)\n", session_config_file, thread_index);
      thread_info[thread_index].init_err = true;

      return 0;
   }

/* parse session config file */

   while (run > 0 && (parse_session_config(session_cfg_fp, &session_data[nSessionsConfigured]) != -1)) nSessionsConfigured++;

   printf("Info: SessionConfiguration() says %d session(s) found in config file\n", nSessionsConfigured);

   if (nSessionsConfigured > MAX_SESSIONS) {

      fprintf(stderr, "Warning: SessionConfiguration() says number of sessions exceeds pktlib max, reducing to %d\n", MAX_SESSIONS);
      nSessionsConfigured = MAX_SESSIONS;
   }

/* close session config file */

   fclose(session_cfg_fp);

   return nSessionsConfigured;
}

/* create static sessions */

int StaticSessionCreate(HSESSION hSessions[], SESSION_DATA session_data[], int nSessionsConfigured, int thread_index) {

int i, nSessionsCreated = 0;
HSESSION hSession;

   for (i=0; i<nSessionsConfigured; i++) {

      printf("++++++++Creating session %d\n", thread_info[thread_index].total_sessions_created);

      if (Mode & CREATE_DELETE_TEST) {  /* change group ID names */

         static int create_counter = 10000;
         char tmp_str[128];

         itoa(create_counter, tmp_str, 10);
         if (session_data[i].group_term.group_mode > 0) memmove(&session_data[i].group_term.group_id[strlen(session_data[i].group_term.group_id)-5], tmp_str, strlen(tmp_str));
         if (session_data[i].term1.group_mode > 0) memmove(&session_data[i].term1.group_id[strlen(session_data[i].term1.group_id)-5], tmp_str, strlen(tmp_str));
         if (session_data[i].term2.group_mode > 0) memmove(&session_data[i].term2.group_id[strlen(session_data[i].term2.group_id)-5], tmp_str, strlen(tmp_str));

         if (i == nSessionsConfigured-1) create_counter++;  /* bug fix, JHB Jan 2019 */
      }

      if (Mode & DISABLE_DTX_HANDLING) {  /* DTX handling enabled by default in session config parsing (in transcoder_control.c), disable here if specified in cmd line */
         session_data[i].term1.uFlags &= ~TERM_DTX_ENABLE;
         session_data[i].term2.uFlags &= ~TERM_DTX_ENABLE;
      }

      if (Mode & DISABLE_PACKET_REPAIR) {  /* packet repair flags enabled by default in session config parsing (in transcoder_control.c), disable them here if specified in cmd line */
         session_data[i].term1.uFlags &= ~(TERM_SID_REPAIR_ENABLE | TERM_PKT_REPAIR_ENABLE);
         session_data[i].term2.uFlags &= ~(TERM_SID_REPAIR_ENABLE | TERM_PKT_REPAIR_ENABLE);
      }

      if (thread_info[thread_index].nInPcapFiles > 1) session_data[i].term2.uFlags |= TERM_EXPECT_BIDIRECTIONAL_TRAFFIC;  /* if we have multiple cmd line inputs, and we are in static session mode, we can set this flag, which makes p/m thread receive queue handling more efficient for bidirectional traffic */

      int target_delay = 0, max_delay = 0;

      if (nJitterBufferParams >= 0) {  /* cmd line param -jN, if entered. nJitterBufferParams is -1 if no cmd line entry */
         target_delay = nJitterBufferParams & 0xff;
         max_delay = (nJitterBufferParams & 0xff00) >> 8;
      }
      else if ((Mode & ENABLE_STREAM_GROUPS) || session_data[i].group_term.group_mode > 0) {
         target_delay = 10;
         max_delay = 14;
      }

      if (target_delay) session_data[i].term1.jb_config.target_delay = target_delay;
      if (max_delay) session_data[i].term1.jb_config.max_delay = max_delay;

      if (!(Mode & ANALYTICS_MODE) || target_delay > 7) session_data[i].term1.uFlags |= TERM_OOO_HOLDOFF_ENABLE;  /* jitter buffer holdoffs enabled except in analytics compatibility mode */

      if ((Mode & ENABLE_STREAM_GROUPS) || session_data[i].group_term.group_mode > 0) {  /* adjust stream group_mode if needed, prior to creating session */

         Mode |= ENABLE_STREAM_GROUPS;  /* in case stream groups were not enabled on cmd line, but they are for at least one session in the static session config file */

         if (Mode & ENABLE_WAV_OUTPUT) {

            session_data[i].group_term.group_mode |= STREAM_GROUP_WAV_OUT_MERGED | STREAM_GROUP_WAV_OUT_STREAM_MONO;  /* specify mono and group output wav files. If merging is enabled, the group output wav file will contain all input streams merged (unified conversation) */

            if (!fStressTest && !fCapacityTest && nRepeatsRemaining[thread_index] == -1) {  /* specify N-channel wav output. Disable if load/capacity or stress test options are active. Don't enable if repeat is active, otherwise thread preemption warnings may show up in the event log (because N-channel processing takes a while), JHB Jun 2019 */

               session_data[i].group_term.group_mode |= STREAM_GROUP_WAV_OUT_STREAM_MULTICHANNEL;
               fNChannelWavOutput = true;
            }
         }

         session_data[i].term1.uFlags |= TERM_OVERRUN_SYNC_ENABLE;  /* overrun synchronization enabled by default in session config parsing (in transcoder_control.c), enabling again here is redundant and shown only for info purposes */
         session_data[i].term2.uFlags |= TERM_OVERRUN_SYNC_ENABLE;

         if ((Mode & USE_PACKET_ARRIVAL_TIMES) && (Mode & ENABLE_ONHOLD_FLUSH_DETECT)) {
            session_data[i].term1.group_mode |= STREAM_CONTRIBUTOR_ONHOLD_FLUSH_DETECTION_ENABLE;
            session_data[i].term2.group_mode |= STREAM_CONTRIBUTOR_ONHOLD_FLUSH_DETECTION_ENABLE;
         }

         if ((Mode & DISABLE_CONTRIB_PACKET_FLUSH) || (!(Mode & USE_PACKET_ARRIVAL_TIMES) && (Mode & ENABLE_AUTO_ADJUST_PUSH_RATE))) {
            session_data[i].term1.group_mode |= STREAM_CONTRIBUTOR_DISABLE_PACKET_FLUSH;  /* auto-adjust push rate (i.e. not based on timestamp timing) disqualifies use of packet flush, JHB Dec2019 */
            session_data[i].term2.group_mode |= STREAM_CONTRIBUTOR_DISABLE_PACKET_FLUSH;
         }

         if (Mode & ENABLE_MERGE_DEBUG_STATS) session_data[i].group_term.group_mode |= STREAM_GROUP_DEBUG_STATS;
         if (Mode & ENABLE_MERGE_DEBUG_STATS_L2) session_data[i].group_term.group_mode |= STREAM_GROUP_DEBUG_STATS_L2;
         if (Mode & DISABLE_FLC) session_data[i].group_term.group_mode |= STREAM_GROUP_FLC_DISABLE;

         if (!session_data[i].group_term.ptime) session_data[i].group_term.ptime = 20;
      }

      SetIntervalTiming(&session_data[i]);  /* set termN.input_buffer_interval and termN.output_buffer_interval -- for user apps note it's important this be done */

   /* call DSCreateSession() API (in pktlib .so) */

      if ((hSession = DSCreateSession(hPlatform, NULL, &session_data[i], GetSessionFlags())) >= 0) {

         hSessions[nSessionsCreated++] = hSession;  /* valid session handle returned from DSCreateSession(), add to hSessions[] */

         #ifdef MERGE_BUFFER_SIZE
         DSSetSessionInfo(hSession, DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_MERGE_BUFFER_SIZE, MERGE_BUFFER_SIZE, NULL);  /* if MERGE_BUFFER_SIZE defined above, set merge buffer size to value other than pktlib default */
         #endif

         thread_info[thread_index].nSessionsCreated++;  /* update per app thread vars */
         thread_info[thread_index].total_sessions_created++;

      /* for debug mode "create sessions from pcap", create 1 initial session, create all others dynamically, based on pcap contents */

         if (Mode & CREATE_DELETE_TEST_PCAP) break;
      }
      else app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_EVENT_LOG, thread_index, "mediaMin INFO: Failed to create static session %d, continuing test with already created sessions \n", i);
   }

   if (nSessionsCreated) {

      JitterBufferOutputSetup(thread_index);  /* set up jitter buffer output for all static sessions created */

      if (Mode & ENABLE_STREAM_GROUPS) {  /* stream group output depends on session creation results, so we do after all static sessions are created. In Dynamic Call mode, it's done when sessions are created after first appearing in the input stream */
   
         StreamGroupOutputSetup(hSessions, 0, thread_index);  /* if any sessions created have a group term, set up stream group output */
      }
   }
   else if (nSessionsConfigured) {

      thread_info[thread_index].init_err = true;
      return -1;  /* return error -- static sessions were configured but none created */
   }

   return nSessionsCreated;
}

/* following are dynamic session creation definitions and local functions */

#define MAX_KEYS 128

#define INCLUDE_PYLDTYPE_IN_KEY
#ifdef INCLUDE_PYLDTYPE_IN_KEY
#define KEY_LENGTH 37   /* each key is up to 37 bytes (ipv6 address size (2*16) + udp port size (2*2)) + RTP payload type (1) */
#else
#define KEY_LENGTH 36   /* each key is up to 36 bytes (ipv6 address size (2*16) + udp port size (2*2)) */
#endif

uint8_t keys[MAX_MEDIAMIN_THREADS][MAX_KEYS][KEY_LENGTH] = {{ 0 }};
uint32_t nKeys[MAX_MEDIAMIN_THREADS] = { 0 };

/* check_for_new_session() looks for new streams in the specified and returns 1 if found.  Notes:

  -finding a new stream means a new session should be created "on the fly" (i.e. dynamic session creation). A new stream is determined by (i) new IP addr:port header and/or (ii) new RTP payload type
  -this info is combined into a "key" that defines the session and is saved to compare with possible new sessions
  -SSRC is not included in the key, in order to maintain RFC8108 compliance (multiple RTP streams within the same session)
  -DTMF packets must match an existing session excluding payload type; i.e. they will not cause a new session to be created
*/

int check_for_new_session(uint8_t *pkt, int pkt_len, uint8_t pyld_type, int pyld_size, int thread_index) {

int version = pkt[0] >> 4;
uint8_t key[KEY_LENGTH], found_match = 0, fInitKeys = 0;
unsigned int len = 0, i;
struct udphdr *udphdr;

   if (version == 4)
   {  
      struct iphdr *iphdr = (struct iphdr *)pkt;
      udphdr = (struct udphdr *)&pkt[iphdr->ihl*4];

   /* copy IPv4 src/dst addr info to key */

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
      fprintf(stderr, "check_for_new_session() says invalid IP version field in packet: %d\n", version);
      return -1;
   }

/* copy UDP port info to key */

   memcpy(&key[len], &udphdr->source, sizeof(udphdr->source));
   len += sizeof(udphdr->source);
   memcpy(&key[len], &udphdr->dest, sizeof(udphdr->dest));
   len += sizeof(udphdr->dest);

   #ifdef INCLUDE_PYLDTYPE_IN_KEY

/* copy RTP payload type to key (but not DTMF packets, which must match an existing session, JHB May 2019) */

   if (pyld_size != 4) {

      memcpy(&key[len], &pyld_type, sizeof(uint8_t));
      len += sizeof(uint8_t);
   }
   #endif

/* see if we already know about this stream */
   
   for (i=0; i<nKeys[thread_index]; i++) {

      if (!memcmp(keys[thread_index][i], key, len)){
         found_match = 1;
         break;
      }
   }

   if (!found_match) {  /* new stream */
      memcpy(keys[thread_index][nKeys[thread_index]], key, len);
      if (nKeys[thread_index] == 0) fInitKeys = 1;
      nKeys[thread_index]++;
   }

   /*static int cnt = 0;printf("check_for_new_sesion: cnt = %d, found_match = %d, nKeys = %d, fInitKeys = %d, len = %d\n", cnt++, found_match, nKeys[thread_index], fInitKeys, len);
   printf("key value: "); for(i = 0; i < KEY_LENGTH; i++) printf("%02x ", key[thread_index][i]); printf("\n");*/

   if ((!fInitKeys || thread_info[thread_index].fDynamicCallMode) && !found_match) return 1;
   else return 0;
}

void reset_dynamic_session_info(int thread_index) {

   nKeys[thread_index] = 0;
   memset(keys[thread_index], 0, MAX_KEYS*KEY_LENGTH);
}

/* codec types currently supported in codec estimation algorithm (used by dynamic session creation).  Add codec types as needed */

enum {
   EVS,
   AMR_WB,
   AMR,
   G711U,
   G711A
};

/* estimate_codec_type() uses an ad-hoc algorithm to make a best guess at codec type and bitrate. Notes:

   -identifies G711u/A, AMR-WB, AMR-NB, and EVS codecs
   -looks at packet payload size, CMR field in payload header, and ToC byte if present
   -to keep the algorithm as unrestricted as possible, some EVS bitrates may be mis-identified, but not a problem as EVS decoder uses bitrate found in the RTP payload headers in the bitstream
   -can be updated as needed
*/

int estimate_codec_type(uint8_t* rtp_pkt, uint32_t payload_len, uint8_t payload_type, uint32_t* bitrate, uint32_t* ptime, uint8_t* cat) {

/* handle static / predefined payload types */

   if (payload_type == 0) return G711U;
   else if (payload_type == 8) return G711A;

/* dynamic payload types */

#if 1  /* added 0x21 check to pick up AMR-WB in 13041.0 pcap, JHB Feb 2019 */
   if (((rtp_pkt[0] == 0xf1 || rtp_pkt[0] == 0x21) && !(rtp_pkt[1] & 0x80)) || (rtp_pkt[0] == 0xf4 && (rtp_pkt[1] & /*0x80*/0xc0)))  /* look for AMR first, check CMR byte and first bit of ToC byte.  Changed 0x80 to 0xc0 mask to detect AMR-NB SID, JHB Nov 2018 */
#else
   if (rtp_pkt[0] & 0x80)  /* check if the MSB is 1 or 0, if it's 1, we know this is an AMR or AMR-WB packet */
#endif
   {
      *cat = 1;

      switch (payload_len) {

         case 6:  /* SID frames = AMR-WB */
         case 7:
            if ((rtp_pkt[1] & 0x80) == 0) {  /* added to detect AMR-NB SID, JHB Nov 2018.  Note, modified this slightly to deal with 13041.0 pcap which has AMR-WB 23850 SIDs.  AMR-NB SID pcaps include AMR_MusixFile, 3838.ws, 6537.0, JHB Feb 2019 */
               *bitrate = 12200;
               return AMR;
            }

         case 33:
            if (!*bitrate) *bitrate = 12650;  /* notes:  1) this is a kludge fall-through for some AMR-WB SID cases, for example AMR-WB 23850 SID vs. 12650 SID there will be a problem, 2) payload size 33 conflicts with EVS 13200 compact header format */
         case 37:
            if (!*bitrate) *bitrate = 14250;
         case 47:
            if (!*bitrate) *bitrate = 18250;
         case 51:
            if (!*bitrate) *bitrate = 19850;
         case 59:
            if (!*bitrate) *bitrate = 23050;
         case 61:
         case 62:
            return AMR_WB;  /* default 23850 */

         case 31:
         case 32:
            *bitrate = 12200;
            return AMR;
      }
   }

   if (rtp_pkt[0] == 0xf0 && !(rtp_pkt[1] & 0x80)) {  /* check for AMR octet-aligned */
   
      *cat |= 2;

      if (payload_len == 33) {
         *bitrate = 12200;
         return AMR;
      }
      else if (payload_len == 62) {  /* AMR 23850 octet-aligned */
         *bitrate = 23850;
         return AMR_WB;
      }
   }

   *cat |= 4;

/* mostly likely EVS, but could still be AMR bitrates > 12650 for longer payload lengths */
   switch (payload_len) {

      case 6:  /* EVS SID frames, compact format, header full, or header full with CMR byte */
      case 7:
      case 8: 
      case 33:  /* EVS 13200 bps, 33 for compact header format, 34 for full header format, 35 for full header with CMR byte */
      case 34:
      case 35:
         return EVS;
      case 41:  /* EVS 16400 bps, 41 for compact header format, 42 for full header format */
      case 42:
         *bitrate = 16400;
         return EVS;

      case 61:
         if ((rtp_pkt[0] & 0xf8) == 0xf0) return AMR_WB;  /*  AMR-WB 23850 bwe */
         else {
            *bitrate = 24400;
            return EVS;  /* EVS 24400 bps compact format */
         }
      case 62:
         if ((rtp_pkt[0] & 0xf8) == 0xf0) return AMR_WB;  /*  AMR-WB 23850 octet-aligned (see AMR_WB.pcap for example) */
         else {
            *bitrate = 24400;
            return EVS;   /* EVS 22440 bps header full */
         }
      case 63:  /* EVS 24400 bps, header full with CMR byte preface */
         *bitrate = 24400;
         return EVS;

      case 31:
      case 32:
         *bitrate = 12200;
         return AMR;
         
      case 186:
      case 187:
         *bitrate = 24400;
#if 0
         *ptime = 60;  /* EVS uses variable ptime, with ToC byte indicating number of frames in the RTP payload.  For EVS we use ptime of 20 msec in session creation, and let pktlib handle variable ptime on the fly, JHB Oct 2019 */
#endif
         return EVS;

      default:
         return -1;
   }
}

/* create a new session on-the-fly when dynamic call mode is in effect, or during stress tests that create sessions from pcaps.  Returns 1 for successful create, 0 if not a codec payload (for example, RTCP packets), and -1 for error condition */
 
int create_dynamic_session(uint8_t *pkt, int pkt_len, HSESSION hSessions[], SESSION_DATA session_data[], int thread_index, int nInput, int nReuse) {

   int i, ip_version, codec_index, ip_hlen = 0;
   HSESSION hSession;
   SESSION_DATA* session;
   char codecstr[10];
   uint32_t bitrate = 0, ptime = 20;
   int rtp_version, pyld_type, pkt_len_lib, rtp_pyld_ofs, rtp_pyld_len;
   uint32_t rtp_ssrc; 
   char group_id[MAX_SESSION_NAME_LEN] = "";
   uint8_t cat = 0;
   char errstr[100] = "";
   int target_delay = 0,  max_delay = 0;

/* perform thorough packet validation */

   ip_version = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_IP_VERSION, pkt, pkt_len, NULL, NULL);

   if (ip_version != 4 && ip_version != 6) {  /* must be IPv4 or IPv6 */
      sprintf(errstr, "invalid IP version = %d, pkt_len = %d", ip_version, pkt_len);
      goto bad_packet;
   }

   rtp_version = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_VERSION, pkt, pkt_len, NULL, NULL);

   if (rtp_version != 2) {  /* must be RTP v2 */
      sprintf(errstr, "invalid RTP version = %d, pkt_len = %d", rtp_version, pkt_len);
      goto bad_packet;
   }

   pyld_type = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDTYPE, pkt, pkt_len, NULL, NULL);

   if (pyld_type < 0) {
      sprintf(errstr, "invalid payload type = %d, pkt_len = %d", pyld_type, pkt_len);
      goto bad_packet;
   }

   if (pyld_type >= 72 && pyld_type <= 82) return 0;  /* ignore RTCP packets */

   pkt_len_lib = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTLEN, pkt, pkt_len, NULL, NULL);

   if (pkt_len_lib <= 0) {
      sprintf(errstr, "invalid pkt len = %d, pkt_len param = %d, payload type = %d", pkt_len_lib, pkt_len, pyld_type);
      goto bad_packet;
   }

   rtp_pyld_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDLEN, pkt, pkt_len, NULL, NULL);

   if (rtp_pyld_len <= 0) {
      sprintf(errstr, "invalid RTP payload len %d, pkt len = %d, pkt_len_lib = %d, payload type = %d", rtp_pyld_len, pkt_len, pkt_len_lib, pyld_type);
      goto bad_packet;
   }

   rtp_pyld_ofs = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDOFS, pkt, pkt_len, NULL, NULL);

   if (rtp_pyld_ofs <= 0) {

      sprintf(errstr, "invalid RTP payload offset %d, pkt len = %d, pkt_len_lib = %d, payload type = %d, rtp_pyld_len = %d", rtp_pyld_ofs, pkt_len, pkt_len_lib, pyld_type, rtp_pyld_len);

bad_packet:

      fprintf(stderr, "DSGetPacketInfo() returns error value for new session packet, no codec estimation performed, %s \n", errstr);
      return -1;
   }

/* check for stub packets or out-of-place DTMF packet */

   if (rtp_pyld_len < 6) {

      if (rtp_pyld_len != 4) {
         sprintf(errstr, "packet has RTP payload size %d less than minimum 4 for DTMF and 6 for media", rtp_pyld_len);
         goto err_msg;
      }
      else fprintf(stderr, "DTMF packet found at start of new stream, DTMF packets dropped until after stream's first media packet \n");

      return -1;
   }

   rtp_ssrc = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_SSRC, pkt, pkt_len, NULL, NULL);

/* estimate codec type */

   codec_index = estimate_codec_type(&pkt[rtp_pyld_ofs], rtp_pyld_len, pyld_type, &bitrate, &ptime, &cat);

   if (codec_index < 0) {

      strcpy(errstr, "Codec type estimate failed");

err_msg:
      fprintf(stderr, "%s, IP ver %d, payload type %d, pkt len %d, RTP pyld size %d, cat %d, pyld[0] %d, pyld[1] %d, pyld[2] %d \n", errstr, ip_version, pyld_type, pkt_len_lib, rtp_pyld_len, cat, pkt[rtp_pyld_ofs], pkt[rtp_pyld_ofs+1], pkt[rtp_pyld_ofs+2]);
      return -1;
   }

/* create session */

   session = &session_data[thread_info[thread_index].nSessionsCreated];

   memset(session, 0, sizeof(SESSION_DATA));  /* clear all SESSION_DATA items before filling in */

   if (ip_version == 4) {

      struct iphdr* iphdr = (struct iphdr*)pkt;

      ip_hlen = iphdr->ihl*4;

      session->term1.remote_ip.type = DS_IPV4;
      session->term1.local_ip.type = DS_IPV4;

      session->term1.remote_ip.u.ipv4 = iphdr->saddr;
      session->term1.local_ip.u.ipv4 = iphdr->daddr;
   }
   else if (ip_version == 6) {

      struct ipv6hdr* ipv6hdr = (struct ipv6hdr*)pkt;

      ip_hlen = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_IP_HDRLEN, pkt, pkt_len, NULL, NULL);  /* IPv6 header len normally fixed at 40, but we call pktlib API as customary practice. Note that DSGetPacketInfo() can be called for basic packet info items without a session handle (i.e. using -1 for session handle param) */

      session->term1.remote_ip.type = DS_IPV6;
      session->term1.local_ip.type = DS_IPV6;

      for (i = 0; i < DS_IPV6_ADDR_LEN; i++) {
         session->term1.remote_ip.u.ipv6[i] = ipv6hdr->saddr.s6_addr[i];
         session->term1.local_ip.u.ipv6[i] = ipv6hdr->daddr.s6_addr[i];
      }
   }

   struct udphdr* udphdr = (struct udphdr*)&pkt[ip_hlen];
   session->term1.remote_port = udphdr->source;
   session->term1.local_port = udphdr->dest;
   session->term1.attr.voice_attr.rtp_payload_type = pyld_type;
   session->term1.attr.voice_attr.ptime = ptime;
   session->term1.ptime = ptime;

   session->term1.max_loss_ptimes = 3;
   session->term1.max_pkt_repair_ptimes = 4;

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
   else if (Mode & ENABLE_STREAM_GROUPS) {
      target_delay = 10;
      max_delay = 14;
   }

   if (target_delay) session->term1.jb_config.target_delay = target_delay;
   if (max_delay) session->term1.jb_config.max_delay = max_delay;

/* set termination endpoint flags */

   if (!(Mode & DISABLE_DTX_HANDLING)) session->term1.uFlags |= TERM_DTX_ENABLE;
   if (!(Mode & DISABLE_PACKET_REPAIR)) session->term1.uFlags |= TERM_SID_REPAIR_ENABLE | TERM_PKT_REPAIR_ENABLE;  /* packet repair and overrun synchronization flags enabled by default */
   if (Mode & ENABLE_STREAM_GROUPS) session->term1.uFlags |= TERM_OVERRUN_SYNC_ENABLE;
   if (!(Mode & ANALYTICS_MODE) || target_delay > 7) session->term1.uFlags |= TERM_OOO_HOLDOFF_ENABLE;  /* jitter buffer holdoffs enabled except in analytics compatibility mode */

   if (Mode & ENABLE_STREAM_GROUPS) {

      char szSessionNameTemp[MAX_SESSION_NAME_LEN] = "";

      if (nInput > 0 && (Mode & COMBINE_CALLS)) nInput = 0;  /* combine calls:  use first cmd line input spec for group id and stream group pcap output filename.  Otherwise use cmd line input spec for group id and output name */

      if (strlen(szSessionName[nInput])) {

         strncpy(szSessionNameTemp, szSessionName[nInput], MAX_SESSION_NAME_LEN);
         szSessionNameTemp[min(MAX_SESSION_NAME_LEN-1, strlen(szSessionName[nInput]))] = 0;
      }

      if (strlen(thread_info[thread_index].szGroupName[nInput])) {  /* check if group name has already been assigned */

         strcpy(group_id, thread_info[thread_index].szGroupName[nInput]);  /* yes, use assigned group name */
      }
      else {  /* no, determine a unique group name (see notes below) */

         if (!fStressTest && !fCapacityTest && (Mode & DYNAMIC_CALL) && strlen(szSessionNameTemp)) {  /* set group id as session name, which will be used by packet/media threads for output wav files.  Don't do this if (i) static session config or (ii) load/capacity or stress test options are active, JHB Jun 2019 */

            strcpy(group_id, szSessionNameTemp);
         }
         else {

            strcpy(group_id, "stream_group");  /* Note the name "stream_group" matches names used in example static session config files.  We also use this as generic naming for stress and capacity tests */
         }

     /*  Important -- if more than one stream group is created the group name ("group ID") must be unique, so we use input index, thread index, and/or re-use count to form unique group IDs */

// if (!(Mode & COMBINE_CALLS) && nInput > 0) printf("\n ============= group_id = %s, prev group id = %s \n", group_id, thread_info[thread_index].szGroupName[nInput-1]);
 
         if (!(Mode & COMBINE_CALLS) && (Mode & DYNAMIC_CALL)) {  /* dynamic call default operation (when calls are not combined) is to generate unique stream group names using cmd line input specs (e.g. pcaps):

                                                                     -each input spec is treated as a call; i.e. a separate stream group (each of which may contain multiple streams)
                                                                     -if an input spec is a duplicate of another one, an "_iN" suffix is added
                                                                     -stream group naming is handled separately from duplicate IP header content, which is handled in PushPackets()
                                                                  */
 
            for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) {

               if (i != nInput && strlen(thread_info[thread_index].szGroupName[i]) && !strcmp(group_id, thread_info[thread_index].szGroupName[i])) sprintf(&group_id[strlen(group_id)], "_i%d", nInput);
            }
         }

         strcpy(thread_info[thread_index].szGroupName[nInput], group_id);  /* keep track of stream group names, before non-input spec suffixes are added */
      }

      if (!fStressTest && !fCapacityTest) strcpy(session->szSessionName, thread_info[thread_index].szGroupName[nInput]);  /* set session->szSessionName for wav outputs */

      if (nReuse) sprintf(&group_id[strlen(group_id)], "_n%d", nReuse);  /* add the re-use count, if applicable.  nReuse is typically non-zero only for capacity or stress tests */

  /* add the application thread index, if applicable:

     -the number of application threads (num_app_threads) is typically more than one only for capacity or stress tests
     -the number of application threads is independent of the number of packet/media threads
  */
      if (num_app_threads > 1) sprintf(&group_id[strlen(group_id)], "_t%d", thread_index);

      session->term1.group_mode = DS_AUDIO_MERGE_ADD;
      if (Mode & WHOLE_GROUP_THREAD_ALLOCATE) session->term1.group_mode |= STREAM_CONTRIBUTOR_WHOLE_GROUP_THREAD_ALLOCATE;
      if ((Mode & DISABLE_CONTRIB_PACKET_FLUSH) || (!(Mode & USE_PACKET_ARRIVAL_TIMES) && (Mode & ENABLE_AUTO_ADJUST_PUSH_RATE))) session->term1.group_mode |= STREAM_CONTRIBUTOR_DISABLE_PACKET_FLUSH;  /* auto-adjust push rate (i.e. not based on timestamp timing) disqualifies use of packet flush, JHB Dec2019 */
      if ((Mode & USE_PACKET_ARRIVAL_TIMES) && (Mode & ENABLE_ONHOLD_FLUSH_DETECT)) session->term1.group_mode |= STREAM_CONTRIBUTOR_ONHOLD_FLUSH_DETECTION_ENABLE;  /* on-hold flush detection usually makes sense for UDP input or pcaps with wall clock arrival times. One functional test with TC1200B_ingestion.1654.0.pcap and ENABLE_AUTO_ADJUST_PUSH_RATE will fail without this, JHB Jan2019 */
      strcpy(session->term1.group_id, group_id);
   }

   switch (codec_index) {

      case EVS:

//         printf("template term1 codec type = %d, flags = %d, sample_rate = %d, bitrate = %d\n", session->term1.codec_type, session->term1.attr.voice_attr.u.evs.codec_flags, session->term1.sample_rate, session->term1.bitrate);

         session->term1.codec_type = DS_VOICE_CODEC_TYPE_EVS;

//#define FORCE_EVS_16KHZ_OUTPUT  /* define this if EVS decoder output should be 16 kHz for stream groups, JHB Mar 2019 */
#ifndef FORCE_EVS_16KHZ_OUTPUT
         if (Mode & ENABLE_STREAM_GROUPS) {  /* changed this to reduce processing time for EVS decode and improve session capacity with stream group enabled.  Stream group output is G711, so this also avoids Fs conversion prior to G711 encode, JHB Feb 2019 */
            session->term1.attr.voice_attr.u.evs.codec_flags = DS_EVS_FS_8KHZ | (DS_EVS_BITRATE_13_2 << 2);  /* set to 8 kHz for improvement in session capacity with merging enabled; bitrate is ignored for decode as EVS decoder figures it out on the fly */
            session->term1.sample_rate = 8000;
            session->term1.input_sample_rate = 16000;
         }
         else
#endif
         {
            session->term1.attr.voice_attr.u.evs.codec_flags = DS_EVS_FS_16KHZ | (DS_EVS_BITRATE_13_2 << 2);  /* for EVS for without a stream group we set to 16 kHz and 13200 bps, no other flags set.  See "evs_codec_flags" in shared_include/session.h */
            session->term1.sample_rate = 16000;                                                               /* for static sessions, would be read from session config file */
            session->term1.input_sample_rate = 16000;
         }

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
         session->term1.bitrate = (bitrate == 0) ? 12200 : bitrate;
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

   session->term2.remote_ip.type = DS_IPV4;
   session->term2.remote_ip.u.ipv4 = htonl(0x0A000001 + thread_info[thread_index].nSessionsCreated);  /* arbitrary UDP port values for stream group output.  For static sessions these would be read from session config file */
   session->term2.local_ip.type = DS_IPV4;
   session->term2.local_ip.u.ipv4 = htonl(0x0A000101 + thread_info[thread_index].nSessionsCreated);

   session->term2.remote_port = udphdr->source + thread_info[thread_index].nSessionsCreated;
   session->term2.local_port = udphdr->dest + thread_info[thread_index].nSessionsCreated;

   session->term2.codec_type = DS_VOICE_CODEC_TYPE_G711_ULAW;
   session->term2.bitrate = 64000;
   session->term2.sample_rate = 8000;
   session->term2.attr.voice_attr.rtp_payload_type = 0; /* 0 for G711 ulaw */
   session->term2.attr.voice_attr.ptime = 20; /* assume 20ms ptime */
   session->term2.ptime = 20;

   session->term2.max_loss_ptimes = 3;
   session->term2.max_pkt_repair_ptimes = 4;

   if (target_delay) session->term2.jb_config.target_delay = target_delay;
   if (max_delay) session->term2.jb_config.max_delay = max_delay;

/* set termination endpoint flags */

   if (!(Mode & DISABLE_DTX_HANDLING)) session->term2.uFlags |= TERM_DTX_ENABLE;
   if (!(Mode & DISABLE_PACKET_REPAIR)) session->term2.uFlags |= TERM_SID_REPAIR_ENABLE | TERM_PKT_REPAIR_ENABLE;  /* packet repair and overrun synchronization flags enabled by default */
   if (Mode & ENABLE_STREAM_GROUPS) session->term2.uFlags |= TERM_OVERRUN_SYNC_ENABLE;
   if (!(Mode & ANALYTICS_MODE) || target_delay > 7) session->term2.uFlags |= TERM_OOO_HOLDOFF_ENABLE;  /* jitter buffer holdoffs enabled except in analytics compatibility mode */

/* group term setup */

   if ((Mode & ENABLE_STREAM_GROUPS) && thread_info[thread_index].fDynamicCallMode && !thread_info[thread_index].fGroupTermCreated[!(Mode & COMBINE_CALLS) ? nInput: 0][nReuse]) {

      session->group_term.remote_ip.type = DS_IPV4;
      session->group_term.remote_ip.u.ipv4 = htonl(0x0A010001);
      session->group_term.local_ip.type = DS_IPV4;
      session->group_term.local_ip.u.ipv4 = htonl(0x0A010101);

      session->group_term.remote_port = udphdr->source + thread_info[thread_index].nSessionsCreated;
      session->group_term.local_port = udphdr->dest + thread_info[thread_index].nSessionsCreated;
   
      session->group_term.codec_type = DS_VOICE_CODEC_TYPE_G711_ULAW;
      session->group_term.bitrate = 64000;
      session->group_term.sample_rate = 8000;
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
      }

      if (Mode & ENABLE_MERGE_DEBUG_STATS) session->group_term.group_mode |= STREAM_GROUP_DEBUG_STATS;
      if (Mode & ENABLE_MERGE_DEBUG_STATS_L2) session->group_term.group_mode |= STREAM_GROUP_DEBUG_STATS_L2;
      if (Mode & DISABLE_FLC) session->group_term.group_mode |= STREAM_GROUP_FLC_DISABLE;

      strcpy(session->group_term.group_id, group_id);
   }

   app_printf(APP_PRINTF_NEWLINE, thread_index, "^^^^^^^^^ Creating dynamic session %d, input #%d, estimated codec type = %s, bitrate = %d%s%s. Creation packet info: IP ver %d, ssrc = 0x%x, payload type %d, pkt len %d, RTP payload size %d, cat %d", thread_info[thread_index].nSessionsCreated+1, nInput+1, codecstr, session->term1.bitrate, strlen(group_id) ? ", group " : "", strlen(group_id) ? group_id : "", ip_version, rtp_ssrc, pyld_type, pkt_len_lib, rtp_pyld_len, cat);

   SetIntervalTiming(session);  /* set timing values, including termN.input_buffer_interval and termN.output_buffer_interval -- for user apps note it's very important this be done */

   if ((hSession = DSCreateSession(hPlatform, NULL, session, GetSessionFlags())) < 0) {

      app_printf(APP_PRINTF_NEWLINE, thread_index, "Failed to create dynamic session, app thread %d", thread_index); 
      return -2;  /* critical error */
   }

/* store session handle and update session counts */

   hSessions[thread_info[thread_index].nSessionsCreated] = hSession;

   thread_info[thread_index].nSessionsCreated++;
   thread_info[thread_index].nDynamicSessions++;
   thread_info[thread_index].total_sessions_created++;

/* update dynamic session stats */

   if (thread_info[thread_index].dynamic_session_stats_index < MAX_DYNAMIC_SESSION_STATS) {

      thread_info[thread_index].DynamicSessionStats[thread_info[thread_index].dynamic_session_stats_index].hSession = hSession;
      strcpy(thread_info[thread_index].DynamicSessionStats[thread_info[thread_index].dynamic_session_stats_index].codecstr, codecstr);
      thread_info[thread_index].DynamicSessionStats[thread_info[thread_index].dynamic_session_stats_index].bitrate = session->term1.bitrate;
      thread_info[thread_index].DynamicSessionStats[thread_info[thread_index].dynamic_session_stats_index].payload_type = pyld_type;
      thread_info[thread_index].dynamic_session_stats_index++;
   }

/* set up jitter buffer output for this session */
  
   JitterBufferOutputSetup(thread_index);

/* set up group output if this is a group owner session */

   if ((Mode & ENABLE_STREAM_GROUPS) && thread_info[thread_index].fDynamicCallMode && !thread_info[thread_index].fGroupTermCreated[!(Mode & COMBINE_CALLS) ? nInput: 0][nReuse]) {

      StreamGroupOutputSetup(hSessions, nInput, thread_index);

      thread_info[thread_index].fGroupTermCreated[!(Mode & COMBINE_CALLS) ? nInput: 0][nReuse] = true;
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

   -in dynamic session mode -- using packet timestamps
     -create new sessions as they appear in input packet flow (see comments)
     -packet/media threads handle duplicate packets
     -filter RTCP packets

   -in static sesssion mode -- not using packet timestamps, instead pushing at regular intervals or using auto-adjust push algorithm
     -we ask DSPushPackets() to strip duplicate packets and let us know so we can immediately push another packet; otherwise we are pushing actual packets 2 times -rN msec apart
     -filter RTCP packets
*/

int PushPackets(uint8_t* pkt_in_buf, HSESSION hSessions[], SESSION_DATA session_data[], int nSessions, uint64_t cur_time, int thread_index) {

int i, j, n, ret_val;
unsigned int pkt_len, uFlags = !(Mode & DYNAMIC_CALL) ? DS_PUSHPACKETS_IP_PACKET | DS_PUSHPACKETS_ENABLE_RFC7198_DEDUP : DS_PUSHPACKETS_IP_PACKET;
int chnum, pyld_type, push_cnt = 0;
int session_push_cnt[128] = { 0 };
static uint8_t queue_full_warning[MAX_SESSIONS] = { 0 };
uint64_t fp_sav_pos;
int pyld_size;
int auto_adj_push_count = 0;

static uint64_t last_cur_time = 0;  /* only used by master app thread */

pcaprec_hdr_t pcap_rec_hdr;
pcaprec_hdr_t* p_pcap_rec_hdr = NULL;

#if 0
  #define RATE_FORCE 1  /* force packet push rate to fixed interval (in msec).  This is reserved for stress test and debug purposes */
#endif
#ifdef RATE_FORCE
static int num_pcap_packets = 0;
#endif

   for (j=0; j<thread_info[thread_index].nInPcapFiles; j++) {

      if (thread_info[thread_index].pcap_in[j] != NULL) {

         if (Mode & ENABLE_AUTO_ADJUST_PUSH_RATE) {
            auto_adj_push_count = 0;
            if (!average_push_rate[thread_index]) goto push_ctrl;  /* dynamically adjust average push rate (APR) if auto-adjust push rate is enabled */
         }

read_packet:

         fp_sav_pos = ftell(thread_info[thread_index].pcap_in[j]);

         if (Mode & USE_PACKET_ARRIVAL_TIMES) p_pcap_rec_hdr = &pcap_rec_hdr;

         //#define STRESS_DEBUG
         #ifdef STRESS_DEBUG
         if (nRepeatsRemaining[thread_index] >= 0 && thread_info[thread_index].num_packets_in[j] > 1000) pkt_len = 0;
         else
         #endif

         pkt_len = DSReadPcapRecord(thread_info[thread_index].pcap_in[j], pkt_in_buf, 0, p_pcap_rec_hdr, thread_info[thread_index].link_layer_len[j]);

         if (pkt_len == 0) {  /* pcap file ends - close (or rewind if input repeat or certain types of stress tests are enabled) */

            if (!(Mode & CREATE_DELETE_TEST_PCAP) && !(Mode & REPEAT_INPUTS)) {  /* check for input repeat */

               if (thread_info[thread_index].pcap_in[j]) fclose(thread_info[thread_index].pcap_in[j]);  /* close the input file and set file handle to NULL so it's no longer operated on */
               thread_info[thread_index].pcap_in[j] = NULL;

               thread_info[thread_index].total_push_time[j] += cur_time - thread_info[thread_index].initial_push_time[j];
            }
            else {  /* in certain test modes or if input repeat is enabled, start the pcap over */

               bool fQueueEmpty = true;  /* wait for all stream group queues to be empty before rewinding the pcap, JHB Mar2020 */

               if ((Mode & ENABLE_STREAM_GROUPS) && (Mode & DYNAMIC_CALL)) {  /* for dynamic calls we wait for all sessions associated with the current input */

                  for (i=0; i<thread_info[thread_index].nSessions[j]; i++) if (!DSPullPackets(DS_PULLPACKETS_GET_QUEUE_STATUS | DS_PULLPACKETS_STREAM_GROUPS, NULL, NULL, hSessions[thread_info[thread_index].nSessionIndex[j][i]], NULL, 0, 0)) { fQueueEmpty = false; break; }
               }
               else if (!DSPullPackets(DS_PULLPACKETS_GET_QUEUE_STATUS | DS_PULLPACKETS_STREAM_GROUPS, NULL, NULL, -1, NULL, 0, 0)) fQueueEmpty = false;  /* for static sessions or if stream groups are not enabled, we wait for all sessions */

               if (!fQueueEmpty) continue;  /* not empty yet, move on to next input */

               thread_info[thread_index].total_push_time[j] += cur_time - thread_info[thread_index].initial_push_time[j];

            /* note that wrapping a pcap will typically cause warning messages about "large negative" timestamp and sequence number jumps, JHB Mar2020 */

               fseek(thread_info[thread_index].pcap_in[j], sizeof(pcap_hdr_t), SEEK_SET);  /* seek to pcap header offset (start of pcap) */
               fp_sav_pos = ftell(thread_info[thread_index].pcap_in[j]);

               app_printf(APP_PRINTF_NEWLINE, thread_index, "mediaMin INFO: pcap %s wraps", MediaParams[thread_info[thread_index].input_index[j]].Media.inputFilename);

               pkt_len = DSReadPcapRecord(thread_info[thread_index].pcap_in[j], pkt_in_buf, 0, p_pcap_rec_hdr, thread_info[thread_index].link_layer_len[j]);
            }

            thread_info[thread_index].initial_push_time[j] = 0;  /* reset initial push time */

            if (((Mode & USE_PACKET_ARRIVAL_TIMES) || frameInterval[0] > 1) && isMasterThread) {

               char tmpstr[200];
               sprintf(tmpstr, "===== mediaMin INFO: %stotal input pcap[%d] time = %4.2f (sec)", !(Mode & USE_PACKET_ARRIVAL_TIMES) ? "estimated " : "", j, 1.0*thread_info[thread_index].total_push_time[j]/1000000L);
               app_printf(APP_PRINTF_NEWLINE, thread_index, tmpstr);
               Log_RT(4 | DS_LOG_LEVEL_FILE_ONLY, tmpstr);
            }
         }

      /* if pkt_len is zero we've reached end of this input:

         -tell pktlib to not expect more packets for sessions associated with this input, and move on to next input
         -note this disables the push packets elapsed time alarm, if enabled (DS_ENABLE_PUSHPACKETS_ELAPSED_TIME_ALARM flag in DEBUG_CONFIG struct item uDebugMode; see shared_include/config.h)
      */
  
         if (pkt_len == 0) {

            int nSessionIndex;
            for (i=0; i<thread_info[thread_index].nSessions[j]; i++) if ((nSessionIndex = thread_info[thread_index].nSessionIndex[j][i]) >= 0) DSPushPackets(DS_PUSHPACKETS_PAUSE_INPUT, NULL, NULL, &hSessions[nSessionIndex], 1);
            continue;
         }

         thread_info[thread_index].num_packets_in[j]++;

         if (!thread_info[thread_index].initial_push_time[j]) {

            thread_info[thread_index].initial_push_time[j] = cur_time;
            #ifdef RATE_FORCE
            num_pcap_packets = 0;
            #endif
         }

         if (Mode & USE_PACKET_ARRIVAL_TIMES) {

            uint64_t pkt_timestamp, elapsed_time;  /* passed param cur_time, pkt_timestamp, and elapsed_time are in usec */
            uint32_t msec_curtime, msec_timestamp;

            pkt_timestamp = (uint64_t)p_pcap_rec_hdr->ts_sec*1000000L + p_pcap_rec_hdr->ts_usec;

            if (!thread_info[thread_index].pkt_base_timestamp[j]) thread_info[thread_index].pkt_base_timestamp[j] = pkt_timestamp;  /* save initial "base" timestamp, which can be zero or some other value like the "epoch" (Jan 1 1970) */

            pkt_timestamp -= thread_info[thread_index].pkt_base_timestamp[j];  /* subtract base timestamp */

            #ifndef RATE_FORCE
            msec_timestamp = (pkt_timestamp + 500)/1000;  /* calculate in msec, with rounding.  Note this method compensates for jitter in how long it takes the main loop to repeat and get back to this point to push another packet.  It provides repeatable results, JHB Jan2020 */
            #else
            msec_timestamp = (num_pcap_packets+1)*RATE_FORCE;  /* debug/stress testing:  push packet either at a specific interval or at random intervals */
            #endif

            elapsed_time = cur_time - thread_info[thread_index].initial_push_time[j];  /* subtract initial time */

            msec_curtime = (elapsed_time + 500)/1000;  /* calculate in msec, with rounding */

            if (msec_curtime < msec_timestamp) {  /* push packet when elapsed time >= packet timestamp */

               fseek(thread_info[thread_index].pcap_in[j], fp_sav_pos, SEEK_SET);  /* not time to push yet, restore file position */
               continue;  /* move on to next input */
            }

            #ifdef RATE_FORCE
            num_pcap_packets++;
            #endif

            #if 0  /* packet timestamp / push timing debug */
            static uint64_t last_push_time = 0;
            int max_push_interval = 0;

            if ((int)(msec_curtime - last_push_time) > max_push_interval) {

               max_push_interval = (int)(msec_curtime - last_push_time);
               printf("\n ! pushing packet in packet arrival time mode, pkt timestamp = %2.1f, push delta = %d, elapsed time = %llu packet timestamp = %llu\n", pkt_timestamp/1000.0, max_push_interval, (unsigned long long)msec_curtime, (unsigned long long)msec_timestamp);
            }

            last_push_time = msec_curtime;
            #endif
         }

         #define FILTER_RTCP_PACKETS_IF_rN_TIMING
         #ifdef FILTER_RTCP_PACKETS_IF_rN_TIMING  /* RTCP packets are already filtered by packet/media threads but if the push rate is 2 msec or slower then we filter them here to avoid FlushCheck() prematurely seeing empty queues and flushing the session.  Notes:

                                                     -session flush for USE_PACKET_ARRIVAL_TIMES mode is not dependent on empty queues, so it's execluded
                                                     -a burst of RTCP packets in a multisession pcap may mean an on-hold or call-waiting period; i.e. one or more (or even all) call legs are not sending RTP
                                                     -packet_flow_media_proc.c uses the DS_RECV_PKT_FILTER_RTCP flag in DSRecvPackets()
                                                   */

         pyld_type = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDTYPE, pkt_in_buf, pkt_len, NULL, NULL);

         if (pyld_type < 0) {
            Log_RT(3, "mediaMin WARNING: PushPackets() says DSGetPacketInfo(DS_PKT_INFO_RTP_PYLDTYPE) returns error value, not checking for new session tupple, not pushing packet, pkt len = %d \n", pkt_len);
            goto read_packet;
         }

         if ((pyld_type >= 72 && pyld_type <= 82) && frameInterval[0] > 1 && !(Mode & USE_PACKET_ARRIVAL_TIMES)) goto read_packet;
         #endif

         pyld_size = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDSIZE, pkt_in_buf, pkt_len, NULL, NULL);

      /* push packets using DSPushPackets() API in pktlib:

         -if dynamic call mode is enabled, look for new sessions -- we find IP headers that have not occurred before and hash them to create a unique key. New session handling performs auto-detection of codec type
         -look for DTMF, filter RTCP, etc
         -if session reuse is active, we modify headers to ensure they are unique (also this is done if we find duplicated inputs on the cmd line)
      */
         
         for (n=0; n<nReuseInputs+1; n++) {  /* default value of nReuseInputs is zero, unless entered on cmd line with -nN.  For nReuseInputs > 0 we reuse each input N times */

check_for_duplicated_headers:

            if (n > 0 || thread_info[thread_index].fDuplicatedHeaders[j]) {  /* modify packet header slightly for each reuse, so all packets in a reused stream look different than other streams.  Notes:

                                                                                1) increment the src UDP port and decrement the dst UDP port to reduce chances of inadvertently duplicating another session
                                                                                2) also increment SSRC to avoid packet/media thread "dormant SSRC" detection (SSRC is not part of key that mediaMin uses to keep track of unique sessions)
                                                                             */

               unsigned int src_udp_port, dst_udp_port, ip_hdr_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_IP_HDRLEN, pkt_in_buf, pkt_len, NULL, NULL);

               memcpy(&src_udp_port, &pkt_in_buf[ip_hdr_len], 2);
               memcpy(&dst_udp_port, &pkt_in_buf[ip_hdr_len+2], 2);
               src_udp_port++;
               dst_udp_port--;
               memcpy(&pkt_in_buf[ip_hdr_len], &src_udp_port, 2);
               memcpy(&pkt_in_buf[ip_hdr_len+2], &dst_udp_port, 2);

               unsigned int rtp_hdr_ofs = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_HDROFS, pkt_in_buf, pkt_len, NULL, NULL);
               unsigned int ssrc = ((unsigned int)pkt_in_buf[rtp_hdr_ofs+11] << 24 | (unsigned int)pkt_in_buf[rtp_hdr_ofs+10] << 16 | (unsigned int)pkt_in_buf[rtp_hdr_ofs+9] << 8 | pkt_in_buf[rtp_hdr_ofs+8]);
               ssrc++;
               pkt_in_buf[rtp_hdr_ofs+11] = ssrc >> 24; pkt_in_buf[rtp_hdr_ofs+10] = (ssrc >> 16) & 0xff; pkt_in_buf[rtp_hdr_ofs+9] = (ssrc >> 8) & 0xff; pkt_in_buf[rtp_hdr_ofs+8] = ssrc & 0xff;
            }

            bool fNewSession = false;

            if (((Mode & CREATE_DELETE_TEST_PCAP) && debug_test_state == CREATE) || thread_info[thread_index].fDynamicCallMode) {

               if (check_for_new_session(pkt_in_buf, pkt_len, pyld_type, pyld_size, thread_index) > 0) {

                  ret_val = create_dynamic_session(pkt_in_buf, pkt_len, hSessions, session_data, thread_index, j, n);

                  if (ret_val > 0) {

                     app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_THREAD_INDEX_SUFFIX, thread_index, "+++++++++Created dynamic session #%d, total sessions created %d", thread_info[thread_index].nSessionsCreated, thread_info[thread_index].total_sessions_created);

                     nSessions++;
                     fNewSession = true;
                  }
                  else {  /* error or problem of some type, remove the key created by check_for_new_session() */

                     nKeys[thread_index]--;
                     memset(keys[thread_index][nKeys[thread_index]], 0, KEY_LENGTH);

                     if (ret_val == -2) { thread_info[thread_index].init_err = true; return -1; }
                  }
               }
               else {  /* if we find duplicated inputs on the cmd line, we slightly modify IP headers of each successive one so they create new sessions / stream groups (this is a hack for mult-input cmd line stress testing), JHB Jan2020 */

                  if (!(Mode & COMBINE_CALLS) && !thread_info[thread_index].nSessions[j] && !thread_info[thread_index].fDuplicatedHeaders[j]) {

                     int l;
                     for (l=0; l<thread_info[thread_index].nInPcapFiles; l++) {

                        if (l != j && thread_info[thread_index].nSessions[l]) {

                           app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_THREAD_INDEX_SUFFIX, thread_index, "++++++++ Cmd line input #%d IP headers are duplicates of cmd line input #%d, modifying headers for input #%d", j+1, l+1, j+1);

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

               chnum = DSGetPacketInfo(hSessions[i], DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_CHNUM_PARENT | DS_PKT_INFO_SUPPRESS_ERROR_MSG, pkt_in_buf, pkt_len, NULL, NULL);  /* get the stream's parent chnum (ignore SSRC) */ 

               #define CHECK_RTP_PAYLOAD_TYPE
               #ifdef CHECK_RTP_PAYLOAD_TYPE  /* this is a special case useful for checking duplicated sessions that differ only in RTP payload type.  It doesn't handle the general case of exactly duplicated sessions */
               if (chnum >= 0 && pyld_size != 4) {  /* don't check DTMF packets */

                  int pyld_type_term = -1;
                  pyld_type = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDTYPE, pkt_in_buf, pkt_len, NULL, NULL);
                  #if 0
                  pyld_type_term = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_RTP_PAYLOAD_TYPE, DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_TERM, 0, NULL), NULL);
                  #else  /* this way is faster and also demonstrates convenient use of SESSION_DATA structs after session creation */
                  int term = DSGetSessionInfo(chnum, DS_SESSION_INFO_CHNUM | DS_SESSION_INFO_TERM, 0, NULL);
                  if (term == 1) pyld_type_term = session_data[i].term1.attr.voice_attr.rtp_payload_type;
                  else if (term == 2) pyld_type_term = session_data[i].term2.attr.voice_attr.rtp_payload_type;
                  #endif
                  if (pyld_type_term != pyld_type) chnum = -1;
               }
               #endif

               if (chnum >= 0) {  /* if packet matches a stream (i.e. a term defined for a session), push to correct session queue.  Note that SSRC is not included in the session match because DSGetPacketInfo() was called with DS_PKT_INFO_CHNUM_PARENT */

                  if (nFirstSession == -1) nFirstSession = hSessions[i];
                  else app_printf(APP_PRINTF_NEWLINE, thread_index, "######### Two pushes for same packet, nFirstSession = %d, hSession = %d, chnum = %d", nFirstSession, hSessions[i], chnum);  /* this should not happen, if it does call attention to it.  If it occurs, it means there are exactly duplicated sessions, including RTP payload type, and we need more information to differentiate */

                  int retry_count = 0;

                  #ifdef FIRST_TIME_TIMING  /* reserved for timing debug purposes */
                  static bool fSync = false;
                  if (!fSync && !fStressTest && !fCapacityTest) { PmThreadSync(thread_index); fSync = true; }  /* sync between app thread and master p/m thread. This removes any timing difference between starting time of application thread vs. p/m thread */

                  static bool fOnce = false;
                  if (!fOnce) { printf("\n === time to first push %llu \n", (unsigned long long)((first_push_time = get_time(USE_CLOCK_GETTIME)) - base_time)); fOnce = true; }
                  #endif

push:
                  ret_val = DSPushPackets(uFlags, pkt_in_buf, &pkt_len, &hSessions[i], 1);  /* push packet to packet/media thread queue */

                  if (!(Mode & DYNAMIC_CALL) && (ret_val & DS_PUSHPACKETS_ENABLE_RFC7198_DEDUP)) goto read_packet;  /* duplicate packet, read next packet */

                  if (!ret_val) {  /* push queue is full, try waiting and pushing the packet again */

                     uint32_t uSleepTime = max(1000, frameInterval[0]*1000);
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

                     if (fNewSession) thread_info[thread_index].nSessionIndex[j][thread_info[thread_index].nSessions[j]++] = i;  /* maintain an index into hSessions[] for each session in the input */

                     session_push_cnt[i]++;
                     thread_info[thread_index].pkt_push_ctr++;
                     push_cnt++;

                     if (queue_full_warning[hSessions[i]]) queue_full_warning[hSessions[i]] = 0;  /* reset queue full warning if needed */

                     break;  /* break out of nSessions loop, the packet should match no other sessions */
                  }
               }
            }  /* nSessions loop (i) */
         }  /* packet reuse loop (n) */
      }  /* input flow loop (j, if fp[j] != NULL */

   /* Dynamic push rate algorithm:

      -enabled if ENABLE_AUTO_ADJUST_PUSH_RATE flag is included in cmd line -dN entry (see all flag definitions near start of this file)
      -intended to be used in the absence of input packet flow timing, for example pcaps with no packet timestamps, UDP input flow from a source not using accurate wall clock timing, etc
      -the push rate is adjusted dynamically by monitoring transcoded output (G711) queue levels, which after transcoding are independent of input packet types (media vs SID, multiframe packets, variable ptime, etc). The objective is to adapt the push rate to timing derived from media content, in the absence of input packet flow timing
      -currently the average push rate (APR) is calculated per mediaMin thread, the idea being to treat all sessions equally. Adjustment is first initialized to push as many packets as there are sessions every -rN msec
      -when stream group processing is enabled, further alignment of individual streams is possible; the STREAM_GROUP_ENABLE_DEDUPLICATION flag is one possible option
      -DSPullPackets() and DSPushPackets() xxx_QUEUE_LEVEL flags return "distance" (in bytes) between input and output queue pointers
      -note that allowing the push rate to become too high will eventually overflow the push queue and a "queue full" status will be returned by DSPushPackets()
   */

      if ((Mode & ENABLE_AUTO_ADJUST_PUSH_RATE) && nSessions && thread_info[thread_index].pcap_in[j]) {  /* if input file no longer open, avoid jumping back to "read_packet:" and operating on closed file */

push_ctrl:  /* upon entry PushPackets() jumps here if current average push rate (APR) is zero */

         int nSessionsActive = 0, nSessionsPushed = 0;
         bool fReduce = false, fIncrease = false;

         for (i=0; i<nSessions; i++) {
            if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) nSessionsActive++;
            if (session_push_cnt[i]) nSessionsPushed++;
         }

         nSessionsPushed /= (1 + nReuseInputs);  /* normalize totals if session reuse is active */
         nSessionsActive /= (1 + nReuseInputs);

         if (++auto_adj_push_count < average_push_rate[thread_index] && nSessionsPushed < nSessionsActive) goto read_packet;  /* first push packets according to current APR value, then update APR */

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

            app_printf(APP_PRINTF_SAMELINE, thread_index, "apr %d ", average_push_rate[thread_index]);
            last_cur_time = cur_time;
         }
      }
   }

   return push_cnt;
}


/* pull packets from packet / media per-session queue.  Packets are pulled by category:  jitter buffer output, transcoded, and stream group */

int PullPackets(uint8_t* pkt_out_buf, HSESSION hSessions[], SESSION_DATA session_data[], unsigned int uFlags, unsigned int pkt_buf_len, int thread_index) {

int j, num_pkts = 0, i = 0, num_pkts_total = 0, numPkts;
FILE* fp = NULL;
unsigned int packet_out_len[1024];  /* These sizes should handle the maximum number of packets that fit into pkt_buf_len amount of space.  That will vary from app to app depending on codec types, max ptimes, etc.  MAXSPACEDEBUG can be used below to look at the worst case.  JHB Aug 2018 */
uint64_t packet_info[1024];
uint8_t* pkt_out_ptr;
char errstr[20];
int nRetry[MAX_SESSIONS] = { 0 };
int group_idx;
bool fRetry;

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
         group_idx = DSGetStreamGroupInfo(hSessions[i], DS_GETGROUPINFO_CHECK_GROUPTERM, NULL, NULL, NULL);  /* note if hSessions[i] is marked for deletion it will have 0x80000000 flag and DSGetStreamGroupInfo() will return -1. This is not a problem since we check for the deletion flag at pull: */
         fp = thread_info[thread_index].fp_pcap_group[group_idx];
         strcpy(errstr, "stream group");
      }
      else return 0;  /* no group owner sessions found */
   }
   else return -1;  /* invalid uFlags or mode */

   if ((Mode & ANALYTICS_MODE) || session_data[i].term1.input_buffer_interval) numPkts = 1;  /* pull one packet in timed situations */
   else numPkts = -1;  /* numPkts set to -1 tells DSPullPackets() to pull all available packets */

pull:

   if (!(hSessions[i] & SESSION_MARKED_AS_DELETED) && !(nRetry[i] & 0x100)) {  /* make sure we have a valid session handle (we may have previously deleted the session), also check to see if we've already had a successful pull */

      num_pkts = DSPullPackets(uFlags, pkt_out_buf, packet_out_len, hSessions[i], packet_info, pkt_buf_len, numPkts);  /* pull available output packets of type specified by uFlags from packet/media thread queue. If numPkts = -1, that indicates pull all that's available */

      if (num_pkts < 0) {
         app_printf(APP_PRINTF_NEWLINE, thread_index, "Error in DSPullPackets() for %s output, return code = %d", errstr, num_pkts);
         goto exit;
      }
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
   static int prev_num_xcoded[MAX_MEDIAMIN_THREADS][MAX_SESSIONS] = {{0}}, total_zero_xcoded[MAX_MEDIAMIN_THREADS][MAX_SESSIONS] = {{0}};

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
   else if (uFlags == DS_PULLPACKETS_TRANSCODED) thread_info[thread_index].pkt_pull_xcode_ctr += num_pkts;
   else if (uFlags == DS_PULLPACKETS_STREAM_GROUP) thread_info[thread_index].pkt_pull_streamgroup_ctr += num_pkts;

   if (fp) {

      if (uFlags == DS_PULLPACKETS_STREAM_GROUP) {

         if (!fStressTest && !fCapacityTest && (Mode & (USE_PACKET_ARRIVAL_TIMES | ANALYTICS_MODE))) {

            if (!num_pkts) {

               if (thread_info[thread_index].fFirstGroupPull[i] && !thread_info[thread_index].flush_state[i]) {

//  printf("\n === wtf are we in here \n");

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
                  -when this occurs it can be identified in output stream group pcaps as a slight variation in packet delta, for example 22 msec, followed by one of 18 msec (for example Wireshark stats under Telephony | RTP | Stream Analysis)
               */

                  nRetry[i]++;  /* mark session as needing a retry */
               }
#endif
            }
            else {

               thread_info[thread_index].fFirstGroupPull[i] = true;

               if (nRetry[i]) {

                  if (thread_info[thread_index].group_pull_stats_index < MAX_GROUP_STATS) {
                     thread_info[thread_index].GroupPullStats[thread_info[thread_index].group_pull_stats_index].retry_interval = thread_info[thread_index].pkt_pull_streamgroup_ctr - num_pkts;
                     thread_info[thread_index].GroupPullStats[thread_info[thread_index].group_pull_stats_index].num_retries = nRetry[i];
                     thread_info[thread_index].GroupPullStats[thread_info[thread_index].group_pull_stats_index].hSession = hSessions[i];
                     thread_info[thread_index].group_pull_stats_index++;
                  }
               }

               nRetry[i] |= 0x100;  /* mark session as a successful pull */
            }
         }
      }

      for (j=0, pkt_out_ptr=pkt_out_buf; j<num_pkts; j++) {

         if (DSWritePcapRecord(fp, pkt_out_ptr, NULL, NULL, NULL, NULL, packet_out_len[j]) < 0) { fprintf(stderr, "DSWritePcapRecord() failed for %s output\n", errstr); return -1; }
         else pkt_out_ptr += packet_out_len[j];

         num_pkts_total++;
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
            group_idx = DSGetStreamGroupInfo(hSessions[i], DS_GETGROUPINFO_CHECK_GROUPTERM, NULL, NULL, NULL);  /* note if hSessions[i] is marked for deletion it will have 0x80000000 flag and DSGetStreamGroupInfo() will return -1. This is not a problem since we check for the deletion flag at pull: */
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

      if (fRetry) {
         usleep(1000);  /* sleep 1 msec */
         i = 0;
         goto entry;  /* retry one or more sessions */
      }
   }

exit:
   return num_pkts_total;
}


/* set input and output buffer interval timing.  Currently we are using term1.xx values for overall timing */

void SetIntervalTiming(SESSION_DATA* session_data) {

/* set input buffer intervals */

   if (Mode & ANALYTICS_MODE) {  /* if -dN cmd line entry specifies analytics mode, we set termN buffer_interval values to zero regardless of what they already are, and regardless of -rN cmd line entry */

      session_data->term1.input_buffer_interval = 0;
      session_data->term2.input_buffer_interval = 0;
   }
   else if ((int)frameInterval[0] != -1) {  /* frameInterval[0] is value of N in-rN cmd line entry */

      if (frameInterval[0] < session_data->term1.ptime) session_data->term1.input_buffer_interval = 0;
      else session_data->term1.input_buffer_interval = frameInterval[0];

      if (frameInterval[0] < session_data->term2.ptime) session_data->term2.input_buffer_interval = 0;
      else session_data->term2.input_buffer_interval = frameInterval[0];
   }

   if (session_data->term1.input_buffer_interval == -1) session_data->term1.input_buffer_interval = session_data->term1.ptime;  /*  if buffer_interval values are not given in either programmatic session setup (dynamic calls) or session config file, then set to ptime */
   if (session_data->term2.input_buffer_interval == -1) session_data->term2.input_buffer_interval = session_data->term2.ptime;

   if (Mode & ENABLE_AUTO_ADJUST_PUSH_RATE) {  /* set in situations when packet arrival timing is not accurate, for example pcaps without packet arrival timestamps, analytics mode sending packets faster than real-time, etc */

      session_data->term1.uFlags |= TERM_IGNORE_ARRIVAL_TIMING;
      session_data->term2.uFlags |= TERM_IGNORE_ARRIVAL_TIMING;
   }

/* set output buffer intervals:

   -required for packet loss flush and pastdue flush to be active (see packet_flow_media_proc.c)
   -required for accurate stream group output timing (i.e. should be set if stream groups are active)
*/

   if (session_data->term1.output_buffer_interval == -1 || (Mode & DYNAMIC_CALL)) {

      if ((Mode & ANALYTICS_MODE) || session_data->term1.input_buffer_interval) session_data->term1.output_buffer_interval = session_data->term2.ptime;  /* output intervals use ptime from opposite terms */
      else session_data->term1.output_buffer_interval = 0;
   }

   if (session_data->term2.output_buffer_interval == -1 || (Mode & DYNAMIC_CALL)) {

      if ((Mode & ANALYTICS_MODE) || session_data->term2.input_buffer_interval)session_data->term2.output_buffer_interval = session_data->term1.ptime;
      else session_data->term2.output_buffer_interval = 0;
   }

   if (Mode & ENABLE_STREAM_GROUPS) {

      if ((Mode & ANALYTICS_MODE) ||
          (session_data->term1.input_buffer_interval && session_data->term1.group_mode) ||
          (session_data->term2.input_buffer_interval && session_data->term2.group_mode)) session_data->group_term.output_buffer_interval = session_data->group_term.ptime;

      if (session_data->group_term.output_buffer_interval < 0) session_data->group_term.output_buffer_interval = 0;  /* if not specified, set to zero */
   }

   if ((int)frameInterval[0] == -1) frameInterval[0] = session_data->term1.input_buffer_interval;
}


unsigned int GetSessionFlags() {

   unsigned int uFlags = DS_SESSION_MODE_IP_PACKET | DS_SESSION_DYN_CHAN_ENABLE | DS_SESSION_DISABLE_PRESERVE_SEQNUM;  /* default flags for DSCreateSession()*/

   #if 0
   uFlags |= DS_SESSION_STATE_ALLOW_DYNAMIC_ADJUST;  /* add dynamic jitter buffer delay adjust option, if needed */
   #endif

   #ifdef ENABLE_MANAGED_SESSIONS
   uFlags |= DS_SESSION_USER_MANAGED;
   #endif

   #ifdef ALLOW_BACKGROUND_PROCESS  /* deprecated, no longer used */
   if (use_bkgnd_process) {
      uFlags |= DS_SESSION_DP_LINUX_SOCKETS;
   }
   else
   #endif

   if (!fNetIOAllowed) uFlags |= DS_SESSION_DISABLE_NETIO;

   return uFlags;
}


void InputSetup(int thread_index) {

int i = 0, j = 0;
char tmpstr[1024];
unsigned int uFlags;

   if (thread_info[thread_index].init_err) return;

   if (Mode & ENABLE_AUTO_ADJUST_PUSH_RATE) average_push_rate[thread_index] = 2;  /* initialize auto-adjust push rate algorithm */

   thread_info[thread_index].nInPcapFiles = 0;

   uFlags = DS_READ | DS_OPEN_PCAP_READ_HEADER;
   if (fCapacityTest) uFlags |= DS_OPEN_PCAP_QUIET;

/* open input pcap files, advance file pointer to first packet.  Abort program on any input file failure */

   while (MediaParams[i].Media.inputFilename != NULL && strlen(MediaParams[i].Media.inputFilename)) {

      if (strstr(strupr(strcpy(tmpstr, MediaParams[i].Media.inputFilename)), ".PCAP")) {  /* looks for both .pcap and .pcapng, JHB Oct2020 */

         if ((thread_info[thread_index].link_layer_len[j] = DSOpenPcap(MediaParams[i].Media.inputFilename, &thread_info[thread_index].pcap_in[j], NULL, "", uFlags)) < 0) {

            strcpy(tmpstr, "../");  /* try up one subfolder */
            strcat(tmpstr, MediaParams[i].Media.inputFilename);

            if ((thread_info[thread_index].link_layer_len[j] = DSOpenPcap(tmpstr, &thread_info[thread_index].pcap_in[j], NULL, "", uFlags)) < 0) {

               fprintf(stderr, "Failed to open input pcap file: %s, index = %d, thread_index = %d, ret_val = %d\n", tmpstr, j, thread_index, thread_info[thread_index].link_layer_len[j]);
               thread_info[thread_index].pcap_in[j] = NULL;
               thread_info[thread_index].init_err = true;  /* error condition for at least one input file, error message is already printed and/or logged */
               break;
            }
         }

         thread_info[thread_index].num_packets_in[j] = 0;
         thread_info[thread_index].input_index[j] = i;  /* save an index that maps to cmd line input specs */
         thread_info[thread_index].nInPcapFiles = ++j;
      }
      else { fprintf(stderr, "Input file: %s is not a .pcap file\n", MediaParams[i].Media.inputFilename); break; }

      frameInterval[i] = MediaParams[i].Media.frameRate;  /* get cmd line rate entry, if any.  Default value if no entry is -1, which indicates to use session ptime */

      i++;  /* advance to next cmd line input spec */
   }

   if (i == 0) thread_info[thread_index].init_err = true;  /* error if no inputs */

   if (thread_info[thread_index].init_err) app_printf(APP_PRINTF_NEWLINE, thread_index, " *************** inside input setup, init err true, thread_index = %d", thread_index);
}


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

   while (MediaParams[i].Media.outputFilename != NULL && strlen(MediaParams[i].Media.outputFilename)) {

      if (strstr(strupr(strcpy(tmpstr, MediaParams[i].Media.outputFilename)), ".PCAP")) {

         char* p1 = strstr(tmpstr, ".PCAPNG");
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

         group_idx = DSGetStreamGroupInfo(hSessions[i], DS_GETGROUPINFO_CHECK_GROUPTERM, NULL, NULL, NULL);

         if (!thread_info[thread_index].fp_pcap_group[group_idx]) {

            sprintf(filestr, "%s%d", group_output_pcap_filename, group_idx);
            if (num_app_threads > 1) sprintf(&filestr[strlen(filestr)], "_%d", thread_index);
            if (Mode & ANALYTICS_MODE) strcat(filestr, "_am");
            strcat(filestr, ".pcap"); 

            ret_val = DSOpenPcap(filestr, &thread_info[thread_index].fp_pcap_group[group_idx], NULL, "", uFlags);

            if (ret_val < 0) {

               fprintf(stderr, "Failed to open stream group output pcap file: %s, ret_val = %d\n", filestr, ret_val);
               thread_info[thread_index].fp_pcap_group[group_idx] = NULL;
            }
         }

         if (Mode & ENABLE_STREAM_GROUP_ASR) {

            if (!thread_info[thread_index].fp_text_group[group_idx]) {

               sprintf(filestr, "%s%d", group_output_text_filename, group_idx);
               if (num_app_threads > 1) sprintf(&filestr[strlen(filestr)], "_%d", thread_index);
               if (Mode & ANALYTICS_MODE) strcat(filestr, "_am");
               strcat(filestr, ".txt"); 

               thread_info[thread_index].fp_text_group[group_idx] = fopen(filestr, "w");

               if (!thread_info[thread_index].fp_text_group[group_idx]) {

                  fprintf(stderr, "Failed to open stream group output text file: %s, errno = %d, errno description = %s\n", filestr, errno, strerror(errno));
               }
            }
         }
      }

      i++;
   }
}

void JitterBufferOutputSetup(int thread_index) {

int ret_val, i, nInput = 0;
char filestr[1024];
char jb_output_pcap_filename[1024] = "";
unsigned int uFlags;

   if (thread_info[thread_index].init_err) return;

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


/* update screen counters */

void UpdateCounters(uint64_t cur_time, int thread_index) {

char tmpstr[MAX_APP_STR_LEN] = "";
static uint64_t last_time[MAX_PKTMEDIA_THREADS] = { 0 };

   if (last_time[thread_index] == 0) last_time[thread_index] = cur_time;
   if ((int64_t)cur_time - (int64_t)last_time[thread_index] <= 100*1000) return;  /* update counters no faster than 100 msec */

   last_time[thread_index] = cur_time;

   if (thread_info[thread_index].pkt_push_ctr != thread_info[thread_index].prev_pkt_push_ctr || thread_info[thread_index].pkt_pull_jb_ctr != thread_info[thread_index].prev_pkt_pull_jb_ctr || thread_info[thread_index].pkt_pull_xcode_ctr != thread_info[thread_index].prev_pkt_pull_xcode_ctr || thread_info[thread_index].pkt_pull_streamgroup_ctr != thread_info[thread_index].prev_pkt_pull_streamgroup_ctr) {

      if (thread_info[thread_index].pkt_pull_jb_ctr >= 100000L) sprintf(tmpstr, "\rPsh %d, pul %d", thread_info[thread_index].pkt_push_ctr, thread_info[thread_index].pkt_pull_jb_ctr);
      else sprintf(tmpstr, "\rPushed pkts %d, pulled pkts %d", thread_info[thread_index].pkt_push_ctr, thread_info[thread_index].pkt_pull_jb_ctr);
      if (thread_info[thread_index].pkt_pull_xcode_ctr || thread_info[thread_index].pkt_pull_streamgroup_ctr) sprintf(&tmpstr[strlen(tmpstr)], "j");
      if (thread_info[thread_index].pkt_pull_xcode_ctr) sprintf(&tmpstr[strlen(tmpstr)], " %dx", thread_info[thread_index].pkt_pull_xcode_ctr);
      if (thread_info[thread_index].pkt_pull_streamgroup_ctr) sprintf(&tmpstr[strlen(tmpstr)], " %ds", thread_info[thread_index].pkt_pull_streamgroup_ctr);

      thread_info[thread_index].prev_pkt_push_ctr = thread_info[thread_index].pkt_push_ctr;
      thread_info[thread_index].prev_pkt_pull_jb_ctr = thread_info[thread_index].pkt_pull_jb_ctr;
      thread_info[thread_index].prev_pkt_pull_xcode_ctr = thread_info[thread_index].pkt_pull_xcode_ctr;
      thread_info[thread_index].prev_pkt_pull_streamgroup_ctr = thread_info[thread_index].pkt_pull_streamgroup_ctr;
   }

   if (strlen(tmpstr)) app_printf(APP_PRINTF_SAMELINE | APP_PRINTF_THREAD_INDEX_SUFFIX, thread_index, tmpstr);  /* use fully buffered I/O; i.e. not stdout (line buffered) or stderr (per character) */
}


/* start specified number of packet/media threads */

int StartPacketMediaThreads(int num_pm_threads, int thread_index) {  /* should only be called by master thread.  See "thread sync points" above (thread_syncN: labels) */

unsigned int uFlags;

   if (nReuseInputs) num_pm_threads = num_app_threads * nReuseInputs * 3 / 30;  /* note:  without DS_CONFIG_MEDIASERVICE_ROUND_ROBIN sessions are assigned to each p/m thread until it fills up. Some p/m threads may end up unused */

   num_pm_threads = max(min(num_pm_threads, 10), 1);  /* from 1 to 10 */

   if (Mode & ROUND_ROBIN_SESSION_ALLOCATION) num_pm_threads = max(num_pm_threads, 2);  /* start a minimum of two p/m threads if round-robin session-to-thread allocation has been specified, for example running multiple cmd line input packet flows, JHB Jan2020 */

   num_pktmed_threads = num_pm_threads;  /* num_pktmed_threads is a global */

   app_printf(APP_PRINTF_NEWLINE, 0, "Starting %d packet and media processing threads", num_pktmed_threads);

   uFlags = DS_CONFIG_MEDIASERVICE_START | DS_CONFIG_MEDIASERVICE_THREAD | DS_CONFIG_MEDIASERVICE_PIN_THREADS | DS_CONFIG_MEDIASERVICE_SET_NICENESS;
   if (Mode & ROUND_ROBIN_SESSION_ALLOCATION) uFlags |= DS_CONFIG_MEDIASERVICE_ROUND_ROBIN;

   uFlags |= DS_CONFIG_MEDIASERVICE_ENABLE_THREAD_PROFILING;  /* slight impact on performance, but useful.  Turn off for highest possible performance */

   if (DSConfigMediaService(NULL, num_pktmed_threads, uFlags, packet_flow_media_proc, NULL) < 0) {  /* start packet/media thread(s) */

      thread_info[MasterThread].init_err = true;
      return -1;
   }

   return 1;
}


/* process interactive keyboard input */

bool ProcessKeys(HSESSION hSessions[], uint64_t cur_time, DEBUG_CONFIG* dbg_cfg, int thread_index) {

char key;
static int app_thread_index_debug = 0;
static int pm_thread_index_debug = 0;
int i;
char tmpstr[500] = "";
PACKETMEDIATHREADINFO PacketMediaThreadInfo;
static uint64_t last_time = 0;
static uint8_t save_uPrintfLevel = 0;

   if (isMasterThread) {  /* master application threads (thread_index = 0) thread handles interactive keyboard commands */

      if (last_time == 0) last_time = cur_time;
      if ((int64_t)cur_time - (int64_t)last_time < 100*1000 && !fPause) return false;  /* check keys every 100 msec. Make an exception for pause key, otherwise we never get out of pause */

      last_time = cur_time;

      key = (char)tolower(getkey());

      if (key == 'q' || run <= 0) {  /* quit key, Ctrl-C, or p/m thread error condition */

         strcpy(tmpstr, "#### ");
         if (key == 'q') sprintf(&tmpstr[strlen(tmpstr)], "q key entered");
         else if (run == 0) sprintf(&tmpstr[strlen(tmpstr)], "Ctrl-C entered");
         else if (run < 0) sprintf(&tmpstr[strlen(tmpstr)], "p/m thread error and abort condition"); 
         sprintf(&tmpstr[strlen(tmpstr)], ", exiting mediaMin");
         app_printf(APP_PRINTF_NEWLINE, thread_index, tmpstr);

         fQuit = true;
         return true;
      }

      if (key == 's') fStop = true;  /* graceful stop, not the same as quit. In a graceful stop each app thread stops after it reaches the end of its inputs, flushes sessions, etc, and does not repeat */

      if (key == 'p') fPause ^= 1;  /* pause */

      if (key == 'o') {  /* toggle p/m thread screen output off/on. Applies to all active p/m threads */

         if (dbg_cfg->uPrintfLevel != 0) {

            save_uPrintfLevel = dbg_cfg->uPrintfLevel;
            dbg_cfg->uPrintfLevel = 0;
         }
         else dbg_cfg->uPrintfLevel = save_uPrintfLevel;

         DSConfigPktlib(NULL, dbg_cfg, DS_CP_DEBUGCONFIG);
      }

      if (key >= '0' && key <= '9') {

         pm_thread_index_debug = key - '0';  /* select a packet/media thread for debug output (subsequent 'd' input) */
         if (pm_thread_index_debug >= num_pktmed_threads) pm_thread_index_debug = num_pktmed_threads-1;
      }

      bool fDisp = false;

      if (key == '-') {
         app_thread_index_debug--;
         if (app_thread_index_debug < 0) app_thread_index_debug = (int)(num_app_threads-1);
         fDisp = true;
      }

      if (key == '+') {
         app_thread_index_debug++;
         if (app_thread_index_debug == (int)num_app_threads) app_thread_index_debug = 0;
         fDisp = true;
      }

      if (key == 'd' || fDisp) {  /* display debug output */

         DSGetLogTimeStamp(tmpstr, sizeof(tmpstr), DS_LOG_LEVEL_WALLCLOCK_TIMESTAMP | DS_LOG_LEVEL_UPTIME_TIMESTAMP);

         char repeatstr[50];
         if (!fRepeatIndefinitely && nRepeatsRemaining[thread_index] >= 0) sprintf(repeatstr, ", repeats remaining = %d", nRepeatsRemaining[thread_index]);  /* if cmd line entry includes -RN with N >= 0, nRepeatsRemaining will be > 0 for repeat operation, JHB Jan2020 */
         else if (nRepeatsRemaining[thread_index] == -1) strcpy(repeatstr, ", no repeats");  /* nRepeat is -1 if cmd line has no -RN entry (no repeats). For cmd line entry -R0, fRepeatIndefinitely will be set */

         printf("%s#### (App Thread) %sDebug info for app thread %d, run = %d%s \n", uLineCursorPos ? "\n" : "", tmpstr, app_thread_index_debug, run, fRepeatIndefinitely ? ", repeating indefinitely" : repeatstr);

         strcpy(tmpstr, "");
         for (i=0; i<thread_info[app_thread_index_debug].nSessionsCreated; i++) sprintf(&tmpstr[strlen(tmpstr)], " %d", thread_info[app_thread_index_debug].flush_state[i]);
         printf("flush state =%s, flush_count = %d, nSessionsCreated = %d, push cnt = %d, jb pull cnt = %d, xcode pull cnt = %d \n", tmpstr, thread_info[app_thread_index_debug].flush_count, thread_info[app_thread_index_debug].nSessionsCreated, thread_info[app_thread_index_debug].pkt_push_ctr, thread_info[app_thread_index_debug].pkt_pull_jb_ctr, thread_info[app_thread_index_debug].pkt_pull_xcode_ctr);

         if (hSessions) {

            sprintf(tmpstr, "push queue check =");
            for (i=0; i<thread_info[app_thread_index_debug].nSessionsCreated; i++) {
               if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) sprintf(&tmpstr[strlen(tmpstr)], " %d", DSPushPackets(DS_PUSHPACKETS_GET_QUEUE_STATUS, NULL, NULL, &hSessions[i], 1));
            }

            sprintf(&tmpstr[strlen(tmpstr)], ", pull queue check =");
            for (i=0; i<thread_info[app_thread_index_debug].nSessionsCreated; i++) {
               if (!(hSessions[i] & SESSION_MARKED_AS_DELETED)) sprintf(&tmpstr[strlen(tmpstr)], " %d", DSPullPackets(DS_PULLPACKETS_GET_QUEUE_STATUS | DS_PULLPACKETS_TRANSCODED | DS_PULLPACKETS_JITTER_BUFFER, NULL, NULL, hSessions[i], NULL, 0, 0));
            }

            sprintf(&tmpstr[strlen(tmpstr)], ", pcap input check =");
            for (i=0; i<thread_info[app_thread_index_debug].nInPcapFiles; i++) {
               sprintf(&tmpstr[strlen(tmpstr)], " %d", thread_info[app_thread_index_debug].pcap_in[i] != NULL);
            }
 
            printf("%s \n", tmpstr);

#if 0  /* deprecated, don't use this method */
            run = 2;
#else  /* ask for run-time debug output from one or more packet / media threads */
            uint64_t uThreadList = 1UL << pm_thread_index_debug;  /* uThreadList is a bitwise list of threads to display.  In this example only one bit is set */
            DSDisplayThreadDebugInfo(uThreadList, DS_DISPLAY_THREAD_DEBUG_INFO_SCREEN_OUTPUT, "#### (PM Thread) ");  /* display run-time debug info for one or more packet/media threads.  Note that DS_DISPLAY_THREAD_DEBUG_INFO_EVENT_LOG_OUTPUT could also be used to print to the event log */
#endif
         }
      }

      if (key == 't') {  /* print packet/media thread info, some of which is redundant with the 'd' command above. This is mainly an example of using the DSGetThreadInfo() API. The PACKETMEDIATHREADINFO struct is defined in pktlib.h */

         DSGetThreadInfo(pm_thread_index_debug, 0, &PacketMediaThreadInfo);
         printf("\n##### debug info for packet/media thread %d \n", pm_thread_index_debug);
         printf("thread id = 0x%llx, uFlags = 0x%x, niceness = %d, max inactivity time (sec) = %d\n", (unsigned long long)PacketMediaThreadInfo.threadid, PacketMediaThreadInfo.uFlags, PacketMediaThreadInfo.niceness, (int)(PacketMediaThreadInfo.max_inactivity_time/1000000L));

         int num_counted = 0;
         uint64_t cpu_time_sum = 0;

         for (i=0; i<THREAD_STATS_TIME_MOVING_AVG; i++) {

            if (PacketMediaThreadInfo.CPU_time_avg[i] > 1000) {

               cpu_time_sum += PacketMediaThreadInfo.CPU_time_avg[i];
               num_counted++;
            }
         }

         printf("CPU time (msec): avg %2.2f, max %2.2f\n", 1.0*cpu_time_sum/max(num_counted, 1)/1000, 1.0*PacketMediaThreadInfo.CPU_time_max/1000);
      }

      if (key == 'z') {  /* do not use -- reserved for Linux / system stall simulation (p/m thread "zap" function, hehe) */
         if (run == 99) run = 1;
         else run = 99;
      }

      return false;
   }

   return fQuit;  /* non-master threads don't handle keyboard commands, they do whatever the master thread does */
}


/* check for session inactivity, including empty push and pull queues and end of inputs.  Flush inactive sessions to force all remaining packets out of jitter buffer and algorthim queues */

void FlushCheck(HSESSION hSessions[], uint64_t cur_time, uint64_t queue_check_time[], int thread_index) {

#define FINAL_FLUSH_STATE 3

int i, j, nDelayTime, nInput;
int nFlushed = 0;
char flushstr[MAX_APP_STR_LEN] = "Flushing NNN sessions";  /* this text will be updated at actual print/log time */
int flushstr_initlen = strlen(flushstr);

   if (Mode & CREATE_DELETE_TEST_PCAP) return;  /* don't flush sessions in test modes where pcap is wrapping and playing continuously */

   for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) {

      if (thread_info[thread_index].flush_state[i] < 2) {  /* check input, push and xcode pull queues to see if they are finished / empty */

         bool queue_empty = true;

         if (thread_info[thread_index].pkt_push_ctr == 0) queue_empty = false;  /* at least one packet has to be pushed first */

         if (queue_empty && (Mode & (USE_PACKET_ARRIVAL_TIMES | ANALYTICS_MODE))) {  /* in these modes, we don't look at queue status until input flow ends, for example there may be valid call waiting or on-hold gaps */

            if (Mode & DYNAMIC_CALL) {

               if ((nInput = GetInputFromSessionIndex(i, thread_index)) < 0 || thread_info[thread_index].pcap_in[nInput]) queue_empty = false;  /* if input is finished then wait for queues to empty out */
            }
            else {  /* for static session creation we don't know which input files match which sessions, so we wait for all inputs on the cmd line to finish, JHB Mar2020 */

               for (j=0; j<thread_info[thread_index].nInPcapFiles; j++) if (thread_info[thread_index].pcap_in[j] != NULL) { queue_empty = false; break; }
            }
         }

         if (queue_empty) {  /* continue with queue status check */

            if (DSPushPackets(DS_PUSHPACKETS_GET_QUEUE_STATUS, NULL, NULL, &hSessions[i], 1) == 0) queue_empty = false;  /* not empty yet */
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

         if (!queue_empty || queue_check_time[i] == 0) queue_check_time[i] = cur_time;
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

         if ((Mode & (ANALYTICS_MODE | USE_PACKET_ARRIVAL_TIMES)) || !fAutoQuit) nDelayTime = 60;  /* default 60 msec delay */
         else nDelayTime = 3000;  /* longer if stopping without 'q' key */

         if (cur_time - queue_check_time[i] > 1000*(nDelayTime + 10*frameInterval[0])*num_app_threads) {  /* arbitrary delay to wait after flushing.  We increase this if multiple mediaMin threads are running as packet/media threads will take longer to flush */

            thread_info[thread_index].flush_state[i] = FINAL_FLUSH_STATE;  /* set session's flush state to final */

            if (!fStressTest && !fCapacityTest && (Mode & DYNAMIC_CALL) && !(Mode & COMBINE_CALLS)) {  /* in static call and test modes, sessions are deleted at the end of the call or test, either at the end or as app threads repeat */

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

                        app_printf(APP_PRINTF_NEWLINE, thread_index, "%s", deletestr);  /* show session delete info onscreen */
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
      memcpy((p = &flushstr[max(flushstr_initlen-strlen(prefixstr), 0)]), prefixstr, strlen(prefixstr));

      if (num_app_threads > 1) sprintf(&p[strlen(p)], " (%d)", thread_index);

      app_printf(APP_PRINTF_NEWLINE, thread_index, "%s", p);  /* show session flush info onscreen */
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

   if (Mode & ENABLE_MERGE_DEBUG_STATS) {
   
      dbg_cfg->uDebugMode |= DS_ENABLE_GROUP_MODE_STATS;  /* equivalent to creating a stream group with STREAM_GROUP_DEBUG_STATS in its group term group_mode flags, but this method applies to all stream groups and can be enabled/disabled at run-time by calling DSConfigPktlib() or DSConfigStreamlib() with DS_CP_DEBUGCONFIG or DS_CP_STREAMLIB */ 
      dbg_cfg->uDebugMode |= DS_ENABLE_EXTRA_PACKET_STATS;
   }

   if (Mode & ENABLE_PACKET_INPUT_ALARM) {  /* enable elapsed time "no input packets" alarm inside DSPushPackets() */

      dbg_cfg->uPushPacketsElapsedTimeAlarm = 15000;  /* this is the default value, if nothing is set (in msec) */
      dbg_cfg->uDebugMode |= DS_ENABLE_PUSHPACKETS_ELAPSED_TIME_ALARM;
   }
}


/* configure event log, packet log, and packet run-time stats */

void LoggingSetup(DEBUG_CONFIG* dbg_cfg, int setup_type) {

int i = 0;
char tmpstr[1024], szInputFileNoExt[1024] = "", szEventLogFile[1024] = "", szPacketLogFile[1024] = "";
char* p;

   if (!dbg_cfg) return;  /* a valid DEBUG_CONFIG struct is required (defined in shared_include/config.h) */

/* enable and configure event log */

   if (setup_type == LOG_EVENT_SETUP) {

      dbg_cfg->uDisableMismatchLog = 1;
      dbg_cfg->uDisableConvertFsLog = 1;

      if (!(Mode & CREATE_DELETE_TEST_PCAP)) dbg_cfg->uLogLevel = 8;  /* 8 is default setting, includes p/m thread, jitter buffers, and codecs. Set to level 9 to see all possible debug messages */
      else dbg_cfg->uLogLevel = 5;  /* log level 5 is used for create/delete test */

      log_level = dbg_cfg->uLogLevel;  /* set global var available to all local funcs and threads */

      dbg_cfg->uEventLogMode = LOG_OUTPUT;  /* Event log output. See LOG_OUTPUT definition at top, which has default definition LOG_SCREEN_FILE, specifying output to both event log file and to screen. LOG_SCREEN_FILE, LOG_FILE_ONLY, and LOG_SCREEN_ONLY are defined in diaglib.h */

#if 0  /* define to enable wall clock date/timestamps (i.e. system time). The default is relative, or "up time", timestamps */
      dbg_cfg->uEventLogMode |= DS_EVENT_LOG_WALLCLOCK_TIMESTAMPS;
#else
      dbg_cfg->uEventLogMode |= DS_EVENT_LOG_UPTIME_TIMESTAMPS;
#endif

      if (!fStressTest && !fCapacityTest) dbg_cfg->uEventLogMode |= LOG_SET_API_STATUS;  /* for functional tests, enable API status and error numbers */

      if (!fStressTest && !fCapacityTest) {  /* in standard opearting mode, associate event log filename with first input pcap file found */

         while (MediaParams[i].Media.inputFilename != NULL && strlen(MediaParams[i].Media.inputFilename)) {

            if (strstr(strupr(strcpy(tmpstr, MediaParams[i].Media.inputFilename)), ".PCAP")) {

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
         sprintf(&szEventLogFile[strlen(szEventLogFile)], "_event_log%s.txt", (Mode & ANALYTICS_MODE) ? "_am" : "");
      }
      else strcpy(szEventLogFile, sig_lib_log_filename);  /* use default if necessary */

#if 0  /* if the app should control event log file descriptor, use this way, which tells diaglib not to create the log file.  Note the app is responsible for appending and/or rewinding the file if needed, and closing it */
      fp_sig_lib_log = fopen(szEventLogFile, "w");
      dbg_cfg->uLogEventFile = fp_sig_lib_log;
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

      if (!strlen((const char*)pktStatsLogFile)) {  /* if a log filename not already given on cmd line, we use an input file to construct a log filename */

         i = 0;

         while (MediaParams[i].Media.inputFilename != NULL && strlen(MediaParams[i].Media.inputFilename)) {

            if (strstr(strupr(strcpy(tmpstr, MediaParams[i].Media.inputFilename)), ".PCAP")) {

               strcpy(szPacketLogFile, MediaParams[i].Media.inputFilename);
               p = strrchr(szPacketLogFile, '/');
               if (p) strcpy(szPacketLogFile, p+1);
               p = strrchr(szPacketLogFile, '.');
               if (p) *p = 0;
               sprintf(&szPacketLogFile[strlen(szPacketLogFile)], "_pkt_log%s.txt", (Mode & ANALYTICS_MODE) ? "_am" : "");
               break;  /* break on first input found */
            }

            i++;
         }

         if (strlen(szPacketLogFile)) strcpy((char*)pktStatsLogFile, szPacketLogFile);  /* pktStatsLogFile is a global var in pktlib.  If not an empty string, pktlib will use this to log all packets and do input vs. output analyis on pkt/media thread exit */
      }

   /* enable packet stats history logging if -L[filename] given on the cmd line.  Notes:

      -packet stats history allows detailed packet log file output after a call is completed.  Packet stats are collected at run-time and stored in mem with negligible impact on performance
      -detailed analysis takes time to process, depending on call length (number of packets) it can take from several sec to several minutes
      -see comments in config.h for packet_stats_logging enums and DEBUG_CONFIG struct
      -"use_log_file" is set in cmd_line_interface.c if cmd line -L entry is present
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

/* local function to handle application screen output and cursor position update */

void app_printf(unsigned int uFlags, int thread_index, const char* fmt, ...) {

char outstr[MAX_APP_STR_LEN];
char* p;
va_list va;
int slen;

   p = &outstr[1];

   va_start(va, fmt);
   vsnprintf(p, sizeof(outstr)-1, fmt, va);
   va_end(va);

   if ((uFlags & APP_PRINTF_THREAD_INDEX_SUFFIX) && num_app_threads > 1) sprintf(&p[strlen(p)], " (%d)", thread_index);  /* add application thread index suffix if specified */

/* make a reasonable effort to coordinate screen output between application threads and p/m threads, JHB Apr2020:

   -p/m threads indicate when they are printing to screen by setting a bit in pm_thread_printf
   -atomic read/compare/write sets/clears isCursorMidLine to indicate cursor position is "start of line" or somewhere mid-line
   -race conditions in determining when the cursor is mid-line can still occur, but they are greatly reduced
*/

   while (pm_thread_printf);  /* wait for any p/m threads printing to finish. No locks are involved so this is quick */

   if ((slen = strlen(p)) && !(uFlags & APP_PRINTF_SAMELINE) && p[slen-1] != '\n') { strcat(p, " \n"); slen += 2; }

   if (slen) {

      if ((uFlags & APP_PRINTF_NEWLINE) && __sync_val_compare_and_swap(&isCursorMidLine, 1, 0)) *(--p) = '\n';  /* update isCursorMidLine if needed */
      else if (p[slen-1] != '\n') __sync_val_compare_and_swap(&isCursorMidLine, 0, 1);

      uLineCursorPos = p[slen-1] != '\n' ? slen : 0;  /* update line cursor position */

      printf("%s", p);  /* use buffered output */
      
      if ((uFlags & APP_PRINTF_EVENT_LOG) || (uFlags & APP_PRINTF_EVENT_LOG_NO_TIMESTAMP)) Log_RT(4 | DS_LOG_LEVEL_FILE_ONLY | ((uFlags & APP_PRINTF_EVENT_LOG_NO_TIMESTAMP) ? DS_LOG_LEVEL_NO_TIMESTAMP : 0), p);  /* if specified also print to event log */
   }
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
         app_printf(APP_PRINTF_NEWLINE, thread_index, "Master thread waiting %lu sec to test energy saver mode", (long int)wait_time_usec/1000000L);
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

         if (when == 0) app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_THREAD_INDEX_SUFFIX, thread_index, "! mediaMin app thread %d staggered start waiting %d msec", thread_index, wait_msec);
         else app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_THREAD_INDEX_SUFFIX, thread_index, "! mediaMin app thread %d waiting %d msec before repeat", thread_index, wait_msec);

         for (j=0; j<wait_msec*1000; j+=500) {

            usleep(500);  /* sleep in 500 usec intervals and check for fQuit */
            if (fQuit) return;
         }

         app_printf(APP_PRINTF_NEWLINE | APP_PRINTF_THREAD_INDEX_SUFFIX, thread_index, "! mediaMin app thread %d waited %d msec", thread_index, wait_msec);
      }
   }
}

/* update stress test vars and states, if active. Also "auto quit" looks for all sessions flushed, indicating the app should exit */

int TestActions(HSESSION hSessions[], int thread_index) {

int i, ret_val = 1;

/* actions for stress tests, if active (see Mode var comments at top for possible tests that can be specified in the cmd line) */

   if ((Mode & CREATE_DELETE_TEST_PCAP) && debug_test_state == DELETE)  /* delete dynamic sessions in the "create from pcap" stress test mode.  Note that debug_test_state is updated by a timer in the "handler" signal handler function */
   {

      for (i = 0; i < thread_info[thread_index].nDynamicSessions; i++)  /* delete a dynamic session, but not the base session */
      {
         app_printf(APP_PRINTF_NEWLINE, thread_index, "+++++++++deleting session %d, nSessionsCreated = %d, nDynamicSessions = %d", hSessions[thread_info[thread_index].nSessionsCreated-1], thread_info[thread_index].nSessionsCreated, thread_info[thread_index].nDynamicSessions);

         thread_info[thread_index].nSessionsCreated--;
         DSDeleteSession(hSessions[thread_info[thread_index].nSessionsCreated]);
         thread_info[thread_index].nDynamicSessions--;
      }

      reset_dynamic_session_info(thread_index);  /* reset all dynamic session keys, cause all sessions to be "re-detected", including static sessions if any */

      debug_test_state = INIT;
   }

/* more actions, including repeat mode and auto-quit */

   bool fAllSessionsFlushed = (thread_info[thread_index].nSessionsCreated > 0);

   for (i=0; i<thread_info[thread_index].nSessionsCreated; i++) if (thread_info[thread_index].flush_state[i] != FINAL_FLUSH_STATE) { fAllSessionsFlushed = false; break; }  /* any session not yet flushed makes this false */

   if (fAllSessionsFlushed) {
 
      if ((Mode & CREATE_DELETE_TEST) || nRepeatsRemaining[thread_index]-1 >= 0 || fRepeatIndefinitely) {
   
         if (!isMasterThread) usleep(1000*50);
         ret_val = 0;  /* for session delete/recreate stress test or repeat mode, start test over after all sessions are flushed */
      }
      else if (fAutoQuit) {  /* set fStop (graceful per-thread stop, same as 's' key) */

         fStop = true;
         ret_val = 0;
      }
   }

   if (thread_info[thread_index].init_err) ret_val = 0;

   #ifdef VALGRIND_DEBUG
   usleep(VALGRIND_DELAY);
   #endif

   return ret_val;
}
