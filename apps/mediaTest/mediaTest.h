/*
  mediaTest.h

  header file for mediaMin application and mediaTest test and measurement program

  Copyright (C) Signalogic, 2015-2020

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
   Modified Aug 2018 JHB, add extern references for ExecutionMode[] and debugMode options, which are used in cmd line handling (see cmd_line_interface.c)
   Modified Jan 2019 JHB, add extern reference for nReuseInputs (to support 500+ session stress testing)
   Modified Jul 2019 JHB, add extern references for nSegmentation, nInterval, and nAmplitude.  These support segmentation, silence detection, strip, and chunk rewrite functionality in mediaTest (e.g. for Kaldi group users)
   Modified Sep 2019 JHB, add uFlags param to cmdLineInterface() definition
   Modified Nov 2019 JHB, add TEXT and CSV output file types (former to support Kaldi ASR)
   Modified Dec 2019 JHB, add extern reference for nJitterBufferParams to support jitter buffer target and max delay cmd line entry (-jN)
   Modified Jan 2020 JHB, add extern reference for nRepeat to supper number of repeat times cmd line entry (-RN)
   Modified Mar 2020 JHB, handle name change of mediaThread_test_app.c to mediaMin.c
   Modified Apr 2020 JHB, add references for uLineCursorPos, isCursorMidLine, and pm_thread_printf
   Modified May 2020 JHB, add pm_sync[] reference. Currently this is only used for timing debug, see comments in mediaMin.c
   Modified Oct 2020 JHB, PCAP definition in I/O file type enum can be used with both pcap and pcapng formats
*/

#ifndef _MEDIA_TEST_H_
#define _MEDIA_TEST_H_

#include <asm/byteorder.h>    /* get platform byte ordering */
#include <netinet/in.h>

#include "hwlib.h"            /* DirectCore API */
#include "cimlib.h"           /* CIM lib API */
#include "filelib.h"          /* waveform file lib API */
#include "keybd.h"            /* interactive key command support */

/* header files shared by host and coCPUs, see Signalogic/shared_include folder */

#include "media.h"            /* media definitions shared with target CPUs */
#include "session_cmd.h"      /* mailbox command interface */
#include "transcoding.h"      /* media transcoding header */
#include "debug.h"

/* program modes */

#define NETWORK_PACKET_TEST   0
#define COCPU_SIM_TEST        1
#define COCPU_NETWORK_TEST    2
#define CODEC_TEST            3

#define X86_PACKET_TEST       0
#define X86_CODEC_TEST        3
#define X86_FRAME_TEST        4

#define LOG_FILE_DIAGNOSTICS  10

/* defined max values in app */

#define MAX_CODEC_INSTANCES   MAX_SESSIONS_PER_CORE*2

/* number of possible input streams, including streams that are re-used for multithread and high capacity testing */
#define MAX_INPUT_STREAMS MAX_SESSIONS

#define MAX_MEDIAMIN_THREADS  64

/* params for codec test modes */
typedef struct
{
   unsigned int bitrate;
   unsigned int sample_rate;
   unsigned int encoder_enable;
   unsigned int decoder_enable;
   unsigned int dtx_enable;
   unsigned int dtx_value;
   unsigned int rf_enable;
   unsigned int fec_indicator;
   unsigned int fec_offset;
/* added Mar 2018, JHB */
   unsigned int codec_type;
   unsigned int num_chan;
   unsigned int sample_width;
/* added Apr 2018, CKJ */
   unsigned int bitDensity;
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
extern unsigned int      frameInterval[];

extern volatile char     pktStatsLogFile[CMDOPT_MAX_INPUT_LEN];
extern volatile bool     demo_build;
extern volatile bool     frame_mode, use_bkgnd_process, use_log_file;
extern volatile int8_t   run;  /* "q" (quit) key and ctrl-C handler variable.  In thread mode, this is direct access to thread state variable if needed:  1 = run (default, set when pktlib is loaded), 0 = exit, 2 = debug info */
extern volatile char     fPMMasterThreadExit;
extern volatile char     fPMThreadsClosing;
extern volatile uint8_t  uQueueRead;

extern volatile uint8_t  uLineCursorPos;  /* may be referenced by apps if they want to know / set the current screen line cursor position. These 3 vars are declared in diaglib.so */
extern volatile uint8_t  isCursorMidLine;
extern volatile uint32_t pm_thread_printf;

extern volatile bool     fNetIOAllowed;
extern volatile bool     fUSBIOAllowed;
extern volatile int      debug_thread;   /* which packet/media thread is currently selected for run-time debug output */

extern char              network_packet_test, cocpu_sim_test, cocpu_network_test, codec_test;
extern unsigned int      CPU_mode, programMode;
extern unsigned int      inFileType, outFileType, outFileType2, USBAudioInput, USBAudioOutput;
extern char              executionMode[2];
extern int               debugMode;
#define Mode debugMode
extern int               performanceMeasurementMode;
extern int               nReuseInputs;
extern int               nSegmentation;
extern int               nSegmentInterval;
extern int               nAmplitude;
extern int               nJitterBufferParams;
extern int               nRepeat;

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
int parse_codec_params(FILE *fp, FRAME_TEST_INFO *info);
void parse_codec_test_params(FILE*, codec_test_params_t*);
int init_codec_test(PMEDIAPARAMS, codec_test_params_t*);

void fill_pcie_buffer(uint8_t *buffer, int length, uint32_t chip_id, uint32_t core_id);
void check_for_host_to_c66x_xfer();
void check_for_c66x_to_host_xfer(int (*process_buffer)(unsigned char *, unsigned int));

extern volatile int send_sock_fd;
void send_packet(uint8_t *packet, uint32_t length);

/* x86 mediaTest items */

extern void x86_mediaTest(void);
//extern void* packet_flow_media_proc(void*);
extern void* mediaMin_thread(void*);
extern int cmdLineInterface(int argc, char **argv, unsigned int uFlags);
extern int GetOutputFilename(char* out_filename, int output_type_file, const char* output_type_content);

extern char x86_frame_test, x86_pkt_test, pcap_extract;

#ifdef __cplusplus
extern "C" {
#endif

  /* I/O file type enum */
  enum
  {
     RAW_AUDIO = 1,  /* currently includes .out, .inp, .raw, .sam, and .pcm.  Subject to change */
     WAV_AUDIO,
     TIM_AUDIO,
     AU_AUDIO,
     ENCODED,
     PCAP,           /* supports both pcap and pcapng formats, JHB Oct2020 */
     TEXT,
     CSV,
#if 0
     USB_AUDIO,
#else
     USB_AUDIO = 0x100,  /* for output only, USB audio can be combined with audio file types */
#endif
     INVALID
  };

  #define OUTFILETYPEMASK 0xff
  #define AUDIO_FILE_TYPES(a) (((a) & OUTFILETYPEMASK) >= RAW_AUDIO && ((a) & OUTFILETYPEMASK) < ENCODED)

  static inline int array_sum(int array[], int length)
  {
     int i = 0;
     int sum = 0;
     for (; i < length; i++) sum += array[i];
     return sum;
  }

#ifdef __cplusplus
}
#endif

#endif  /* _MEDIA_TEST_H_ */
