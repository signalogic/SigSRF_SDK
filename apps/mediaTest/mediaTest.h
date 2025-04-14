/*
  mediaTest.h

  header file for mediaMin application and mediaTest test and measurement program

  Copyright (C) Signalogic, 2015-2025

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

  Revision History

   Created 2015 CJ
   Modifed Mar 2017 JHB, edits for codec testing
   Modified Jul 2017 CJ, moved pcap file related structs to pktlib
   Modified Aug 2017 CJ, moved codec related payload size, bitrate, and sampling rate functions to voplib
   Modified Aug 2017 JHB, pulled in items remaining in x86_mediaTest.h (which is no longer used)
   Modified Mar 2018 JHB, added extern unsigned int USBAudioInput for USB audio input handling
   Modified Mar 2018 JHB, added codec_type and num_chan elements to codec_test_parms_t struct
   Modified Apr 2018 CKJ, added MELPe codec items to codec_test_parms_t struct
   Modified May 2018 CKJ, added vad codec test item to codec_test_params_t struct
   Modified Aug 2018 JHB, add extern references for ExecuteMode[] and debugMode options, which are used in cmd line handling (see cmd_line_interface.c)
   Modified Jan 2019 JHB, add extern reference for nReuseInputs (to support 500+ session stress testing)
   Modified Jul 2019 JHB, add extern references for nSegmentation, nInterval, and nAmplitude.  These support segmentation, silence detection, strip, and chunk rewrite functionality in mediaTest (e.g. for Kaldi group users)
   Modified Sep 2019 JHB, add uFlags param to cmdLineInterface() definition
   Modified Nov 2019 JHB, add TEXT and CSV output file types (former to support Kaldi ASR)
   Modified Dec 2019 JHB, add extern reference for nJitterBufferParams to support jitter buffer target and max delay cmd line entry (-jN)
   Modified Jan 2020 JHB, add extern reference for nRepeats to support number of repeat times cmd line entry (-RN)
   Modified Mar 2020 JHB, handle name change of mediaThread_test_app.c to mediaMin.c
   Modified Apr 2020 JHB, add references for uLineCursorPos, isCursorMidLine, and pm_thread_printf
   Modified May 2020 JHB, add pm_sync[] reference. Currently this is only used for timing debug, see comments in mediaMin.c
   Modified Oct 2020 JHB, PCAP definition in I/O file type enum can be used with both pcap and pcapng formats
   Modified Jan 2021 JHB, change AUDIO_FILE_TYPES macro to IS_AUDIO_FILE_TYPE, add extern reference for char szSDPFile[CMDOPT_MAX_INPUT_LEN]
   Modified Apr 2021 JHB, add "header_format" to codec_test_params_t struct
   Modified Mar 2022 JHB, add GPX file type and reference to gpx_process (cmd line handling)
   Modified Sep 2022 JHB, add "framesize" option in codec_test_params_t, this allows a framesize to be specified in codec config files for for pass-thru cases such as .cod to .pcap, where no encoding/decoding is specified
   Modified Sep 2022 JHB, add "payload_shift" option in codec_test_params_t, this is a special case to "unshift" EVS AMR-WB IO mode bit-shifted packets observed in-the-wild. Note shift can be +/-
   Modified Dec 2022 JHB, add extern references for char szSDPFile[CMDOPT_MAX_INPUT_LEN] and sig_lib_event_log_filename
   Modified Jan 2023 JHB, add extern reference to fCtrl_C_pressed (see ctrl-C event handler in mediaTest/see cmd_line_interface.c)
   Modified Jan 2023 JHB, add szAppFullCmdLine and GetCommandLine() references
   Modified Mar 2023 JHB, add frac() helper, does modf() but needs only one argument
   Modified May 2023 JHB, add timeScale and convert RealTimeInterval[] to float to support FTRT and AFAP modes, add uPortList[], add uLoopbackDepth
   Modified Jun 2023 JHB, move NOMINAL_REALTIME_INTERVAL definition here from mediaMin.h
   Modified Jul 2023 JHB, add const char* version_info param to cmdLineInterface()
   Modified Aug 2023 JHB, add uTimestampMatchMode. Flags are defined in shared_include/streamlib.h
   Modified Sep 2023 JHB, and fCapacityTest. Moved here from mediaMin.cpp, referenced both there and in packet_flow_media_proc.c
   Modified Nov 2023 JHB, add nCut, set from cmd line --cut option
   Modified Nov 2023 JHB, remove definition of MAX_CODEC_INSTANCES (now in voplib.h)
   Modified Dec 2023 JHB, add char szStreamGroupPcapOutputPath[CMDOPT_MAX_INPUT_LEN]
   Modified Dec 2023 JHB, remove session_cmd.h and transcoding.h includes
   Modified Dec 2023 JHB, add bandwidth_limit and input_sample_rate fields to codec_test_params_t struct
   Modified Feb 2024 JHB, modify definition of mediaTest_proc() to support multi-thread testing
   Modified Feb 2024 JHB, add fShow_md5sum and fShow_audio_classification to support --md5sum and --show_aud_clas command line options
   Modified Apr 2024 JHB, rename executionMode[] to executeMode[] for consistency across all source codes
   Modified May 2024 JHB, change x86_mediaTest.c references to mediaTest_proc.c
   Modified Jul 2024 JHB, add fGroupOutputNoCopy to support --group_pcap_nocopy cmd line option
   Modified Jul 2024 JHB, add nRandomBitErrorPercentage to support --random_bit_error cmd line option
   Modified Aug 2024 JHB, add fShow_sha1sum and fShow_sha512sum to support --sha1sum and --sha512sum command line options
   Modified Sep 2024 JHB, add io_data_type name to IO type enums
   Modified Nov 2024 JHB, remove directcore.h include
   Modified Dec 2024 JHB, rename "header_format" to "payload_format" in codec_test_params_t struct
   Modified Feb 2025 JHB, change references to MAX_INPUT_STREAMS to MAX_STREAMS, which is defined in shared_include/streamlib.h. MAX_STREAMS specifies maximum streams available for reference applications and multithread / high capacity testing
   Modified Apr 2025 JHB, add isLinePreserve extern
*/

#ifndef _MEDIA_TEST_H_
#define _MEDIA_TEST_H_

#include <asm/byteorder.h>    /* get platform byte ordering */
#include <netinet/in.h>
#include <math.h>

/* SigSRF header files */

#include "filelib.h"  /* waveform file lib API */
#include "cimlib.h"   /* CIM lib API, after definition of MAX_STREAMS */

/* SigSRF header files shared by host and coCPUs, see Signalogic/shared_include folder */

#include "shared_include/streamlib.h"  /* MAX_STREAMS definition */
#include "shared_include/media.h"      /* media definitions shared with target CPUs */
#include "shared_include/debug.h"

/* app helper includes */

#include "keybd.h"            /* interactive key command support */

/* program modes */

#define NETWORK_PACKET_TEST        0
#define COCPU_SIM_TEST             1
#define COCPU_NETWORK_TEST         2
#define CODEC_TEST                 3

#define X86_PACKET_TEST            0
#define X86_CODEC_TEST             3
#define X86_FRAME_TEST             4

#define LOG_FILE_DIAGNOSTICS       10

#define NOMINAL_REALTIME_INTERVAL  20  /* default used if no real-time interval (-rN) given on mediaMin command line */

/* max app values */

#define MAX_APP_THREADS            64
#define MAX_CMDLINE_STR_LEN        1024

#define RTP_DETECT_EXCLUDE_VIDEO   1    /* flags for detect_codec_type_and_bitrate() in mediaMin.cpp */

#ifdef __cplusplus
  extern "C" {
#endif

/* params for codec test modes */
typedef struct
{
   unsigned int bitrate;
   unsigned int sample_rate;
   unsigned int input_sample_rate;  /* input sampling rate, intended for use with raw audio files with no header */
   unsigned int encoder_enable;
   unsigned int decoder_enable;
   unsigned int dtx_enable;
   unsigned int dtx_value;
   unsigned int rf_enable;        /* RF params (channel aware) */
   unsigned int fec_indicator;
   unsigned int fec_offset;
   unsigned int bandwidth_limit;  /* bandwidth limit, JHB Dec 2023 */
/* added Mar 2018, JHB */
   unsigned int codec_type;
   unsigned int num_chan;
   unsigned int sample_width;
/* added Apr 2018, CKJ */
   unsigned int bitDensity;  /* MELPe and AMR-WB+ */
   unsigned int Npp;
   unsigned int post;
   unsigned int vad;
   unsigned int uncompress;
   unsigned int mono;
   unsigned int limiter;
   unsigned int low_complexity;
   float isf;
   unsigned int mode;
   float bitrate_plus;
   unsigned int payload_format;  /* added Apr 2021 JHB, renamed Dec 2024 */
   unsigned int framesize;  /* added Sep 2022 JHB */
   int payload_shift;  /* special case to "unshift" EVS AMR-WB IO mode bit-shifted packets observed in-the-wild. Note shift can be +/-, Sep 2022 JHB */

} codec_test_params_t;

/* c66x-x86 shared mem dual buffer struct */
typedef struct fp_buffers_s
{
   // Memory pointer to host map memory
   unsigned int  *dp_dsp_ctrl_reg;
   unsigned int  *dsp_dp_ctrl_reg;
   unsigned int  *dp_dsp_length;
   unsigned int  *dp_dsp_buffer_id;
   unsigned int  *dsp_dp_length;
   unsigned char *dp_dsp_buffer_a;
   unsigned char *dp_dsp_buffer_b;
   unsigned char *dsp_dp_buffer;

   // Local data
   unsigned int curr_buffer_index;      // 0 for A buffer, 1 for B buffer
   unsigned int curr_buffer_length;
   
} fp_buffers_t;

/* struct to hold test config info for x86 frame-based test mode */
typedef struct {

   TERMINATION_INFO term;
   char *encoder_file;
   char *decoder_file;

} FRAME_TEST_INFO;

struct ipv6header {

   #if defined(__LITTLE_ENDIAN_BITFIELD)
   __u8    priority:4,
           version:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
   __u8    version:4,
           priority:4;
#else
#error	"Please fix <asm/byteorder.h>"
#endif
   __u8    flow_lbl[3];

   __be16  payload_len;
   __u8    nexthdr;
   __u8    hop_limit;

   struct in6_addr saddr;
   struct in6_addr daddr;
};

extern                   MEDIAPARAMS MediaParams[];
extern                   PLATFORMPARAMS PlatformParams;
extern float             RealTimeInterval[];

extern volatile char     pktStatsLogFile[CMDOPT_MAX_INPUT_LEN];
extern volatile bool     demo_build;
extern volatile bool     frame_mode, use_bkgnd_process, use_log_file;
extern volatile int8_t   pm_run;  /* "q" (quit) key and ctrl-C handler variable.  In thread mode, this is direct access to thread state variable if needed:  1 = run (default, set when pktlib is loaded), 0 = exit, 2 = debug info */
extern volatile char     fPMMasterThreadExit;
extern volatile char     fPMThreadsClosing;
extern volatile uint8_t  uQueueRead;

extern volatile uint8_t  uLineCursorPos;  /* may be referenced by apps if they want to know / set the current screen line cursor position. These 4 vars are declared in diaglib.so */
extern volatile uint8_t  isCursorMidLine;
extern volatile uint32_t pm_thread_printf;
extern volatile uint8_t  isLinePreserve;

extern volatile bool     fNetIOAllowed;
extern volatile bool     fUSBIOAllowed;
extern volatile int      debug_thread;   /* which packet/media thread is currently selected for run-time debug output */

extern char              network_packet_test, cocpu_sim_test, cocpu_network_test, codec_test, gpx_process;
extern unsigned int      CPU_mode, programMode;
extern unsigned int      inFileType, outFileType, outFileType2, USBAudioInput, USBAudioOutput;
extern char              executeMode[2];
extern int64_t           debugMode;
#define Mode debugMode
extern int               performanceMeasurementMode;
extern int               nReuseInputs;
extern int               nSegmentation;
extern int               nSegmentInterval;
extern int               nAmplitude;
extern int               nJitterBufferParams;
extern int               nRepeats;
extern char              szSDPFile[CMDOPT_MAX_INPUT_LEN];
extern int               nSamplingFrequency;
extern char              szStreamGroupWavOutputPath[CMDOPT_MAX_INPUT_LEN];
extern char              szStreamGroupPcapOutputPath[CMDOPT_MAX_INPUT_LEN];
extern uint16_t          uPortList[MAX_STREAMS];
extern uint8_t           uLookbackDepth;

extern QWORD             nCoreList;                         /* bitwise core list given in command line */
extern HCARD             hCard;
extern DWORD             numCores;
extern unsigned int      nCoresPerCPU;

extern fp_buffers_t      fp_buffers[MAXCPUS][MAXCORESPERCPU];

extern unsigned char     dummy_packet[];
extern unsigned int      sizeof_dummy_packet;
extern uint8_t           pm_sync[];

#ifdef FIRST_TIME_TIMING  /* reserved for timing debug purposes */
extern unsigned long long base_time, first_push_time, first_buffer_time, first_pull_time, first_contribute_time;
#endif

extern char              sig_lib_event_log_filename[];  /* moved here from packet_flow_media_proc.c, JHB Dec 2022 */
extern bool              fCtrl_C_pressed;
extern char              full_cmd_line[];  /* app command line, filled in by cmdLineInterface(), which calls GetCommandLine(), JHB Jan 2023 */
extern double            timeScale;  /* support FTRT and AFAP modes, see comments in packet_flow_media_proc.c */
extern unsigned int      uTimestampMatchMode;  /* timestamp-match wav output mode, flags are defined in shared_include/streamlib.h, JHB Aug 2023 */
extern bool              fCapacityTest;  /* set by apps if they are doing capacity test (moved here from mediaMin.cpp), JHB Sep 2023 */
extern int               nCut;  /* command line --cut input */
extern bool              fShow_md5sum;  /* command line --md5sum */
extern bool              fShow_audio_classification;  /* command line --show_aud_clas */
extern bool              fGroupOutputNoCopy;  /* command line --group_pcap_nocopy */
extern int               nRandomBitErrorPercentage;  /* command line --random_bit_error */
extern bool              fShow_sha1sum;  /* command line --sha1sum */
extern bool              fShow_sha512sum;  /* command line --sha512sum */

#define szAppFullCmdLine (((const char*)full_cmd_line))  /* szAppFullCmdLine is what apps should use. full_cmd_line should not be modified so this is a half-attempt to remind user apps that it should be treated as const char* */

int sigMRF_init(void);
void sigMRF_cleanup(void);

int needQuit(pthread_mutex_t *mtx);

#ifndef NO_MAILBOX
int query_mb(unsigned int node);
int read_mb(unsigned int node, unsigned char *buf, unsigned int *size, unsigned int *trans_id);
int write_mb(unsigned int node, unsigned char *buf, unsigned int size, unsigned int trans_id);
#endif

void *control_thread_task(void*);

void *encode_thread_task(void*);
void *decode_thread_task(void*);

int transcode_init(void);
int create_sessions(PMEDIAPARAMS);
int delete_sessions(void);

int parse_session_config(FILE *fp, SESSION_DATA *params);
void parse_codec_config(FILE*, codec_test_params_t*);
int parse_codec_config_frame_mode(FILE *fp, FRAME_TEST_INFO *info);
int init_codec_test(PMEDIAPARAMS, codec_test_params_t*);

void fill_pcie_buffer(uint8_t *buffer, int length, uint32_t chip_id, uint32_t core_id);
void check_for_host_to_c66x_xfer();
void check_for_c66x_to_host_xfer(int (*process_buffer)(unsigned char *, int));

extern volatile int send_sock_fd;
void send_packet(uint8_t *packet, uint32_t length);

/* mediaMin and mediaTest helper functions */

extern void* mediaTest_proc(void*);
extern void* mediaMin_thread(void*);
extern int cmdLineInterface(int argc, char **argv, unsigned int uFlags, const char* version_info, const char* banner_info);
extern int GetOutputFilename(char* out_filename, int output_type_file, const char* output_type_content);
int GetCommandLine(char* cmdlinestr, int str_size);

/* mediaTest_proc.cpp items */

extern char x86_frame_test, x86_pkt_test, pcap_extract;

/* _fread() is a wrapper to avoid the following linker warning, should it be needed:

    /usr/include/x86_64-linux-gnu/bits/stdio2.h:285:71: warning: call to ¡®__fread_chk_warn¡¯ declared with attribute warning: fread called with bigger size * nmemb than length of destination buffer [enabled by default] return __fread_chk_warn (__ptr, __bos0 (__ptr), __size, __n, __stream);

  this warning can occur when the -flto linker option is enabled and the compiler is building -O2 or -O3 code. Not all freads() encounter this
*/

static size_t __attribute__((optimize("O1"))) __attribute__((unused)) _fread(void *ptr, size_t size, size_t count, FILE *stream) {

   return fread(ptr, size, count, stream);
}

/* I/O file type enums */

  enum io_data_type {

     RAW_AUDIO = 1,  /* currently includes .out, .inp, .raw, .sam, and .pcm.  Subject to change */
     WAV_AUDIO,
     TIM_AUDIO,
     AU_AUDIO,
     ENCODED,
     PCAP,           /* supports both pcap and pcapng formats, JHB Oct 2020 */
     TEXT,
     CSV,
     BER,            /* added as experimental, JHB Dec 2021 */
     GPX,
     USB_AUDIO = 0x100,  /* for output only, USB audio can be combined with audio file types */
     INVALID
  };

  #define OUTFILETYPEMASK 0xff
  #define IS_AUDIO_FILE_TYPE(a) (((a) & OUTFILETYPEMASK) >= RAW_AUDIO && ((a) & OUTFILETYPEMASK) < ENCODED)

  static inline int array_sum(int array[], int length) {
     int i = 0;
     int sum = 0;
     for (; i < length; i++) sum += array[i];
     return sum;
  }

  static inline double frac(double x) {  /* added JHB Mar 2023 */
     double not_used;
     return modf(x, &not_used);
  }
#ifdef __cplusplus
}
#endif

#endif  /* _MEDIA_TEST_H_ */
