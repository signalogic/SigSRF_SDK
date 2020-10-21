/*
 $Header: /root/Signalogic/apps/mediaTest/x86_mediaTest.c

 Copyright (C) Signalogic Inc. 2017-2020
 
 Description
 
   Source code for mediaTest x86 platform (see mediaTest.c for coCPU functionality)

 Purposes
 
   1) Implementation, test, and measurement for codecs and transcoding including multiple RFC compliant packet flow, advanced jitter buffer, DTX handling, DTMF event handling, multichannel packets, ptime conversion, and more.  Measurements include:
   
     -x86 server performance

     -verify bitexactness for codecs, measure audio quality.  Interoperate at encoded bitstream level with 3GPP test vectors and reference codes

     -packet loss and other packet statistics
   
   2) Support RTP streaming for network sockets and pcap I/O

   3) Provide file I/O support for .wav, .tim, raw audio, encoded bitstream (e.g. .cod), and other file formats
   
   4) Support multithreading, background process, and multiple concurrent codec instances

   5) Demonstrate signal processing and deep learning insertion points
   
   6) Provide user application source code examples, including correct transcoding data flow and API usage for Pktlib, Voplib, Diaglib, and Aviolib

   7) Provide basis for limited, demo/eval version available on Github

 Revision History

   Created Jan 2017 CKJ
   Modified Mar 2017 JHB - edits for codec testing
   Modified May 2017 CKJ
      - All config file parsing code is now in transcoder_control.c
      - frame test mode 
         - will now display the total encoded and decode frame counts while running
         - encode thread will also now adjust the filename extension based on the codec type (default is .coded)
      - packet test mode
         - updated mediaTest source to check for the magic number in any input pcap files, display a message if it's an unexpected value and skip over that input file
      - The dependency on libpcap/libpcap-devel has been removed
      - ctrl-c signal is handled to exit x86 test modes cleanly
   Modified May CKJ 2017, added pcap extract mode - supports EVS only
   Modified Mar 2018 JHB, added USB audio input, tested with a Focusrite 2i2 unit.  Significant mods made to (i) x86_mediaTest.c and mediaTest.c, (ii) aviolib (audio video I/O lib) which uses Alsa "asound" lib, and (iii) mediaTest Makefile (modified to link aviolib)
   Modified Mar 2018 JHB, modified codec test mode to support (i) .wav file input (using DSLoadDataFile), (ii) USB audio input input, and (iii) pass-thru (codec type = NONE), in which case no encoding/decoding is performed (e.g. save USB audio to wav file, convert raw audio to wav file)
   Modified Mar-Apr 2018 JHB, CKJ, add support for MELPe codec
   Modified May 2018 CKJ, add support for G729 and G726 codecs
   Modified May-Jun 2018 CKJ - add support for AMR and AMR WB codecs
   Modified Jun 2018 JHB, integrate USB audio output
   Modified Jul 2018 JHB, packet_flow_media_proc(executionMode) function moved to separate file to support media thread and process execution.  executionMode can be app, thread, or process.  See also DSConfigMediaService() and its uControl and uFlags definitions in pktlib.h
   Modified Oct 2018 JHB, add sample rate conversion for USB audio output, independent of waveform or USB audio input prior to codec processing.  Calculate separate period and buffer size parameters not dependent on those used for USB audio input.  Note that many USB audio devices, such as the Focusrite 2i2 used in Sig lab testing, support a minimum sample rate of 44.1 kHz.  For codec support, it helps a lot of the device supports 48 kHz
   Modified Dec 2018 CKJ, add support for AMR-WB+ codec, including ".BIT" coded data file extension, variable encoder input framesize.  Pass &encOutArgs to DSCodecEncode() function
   Modified Dec 2018 JHB, adjust thread/core affinity for high capacity mediaTest -Et -tN mode testing (creates N mediaMin application threads)
   Modified Jul 2019 JHB, codecs now accessed via voplib API calls and CODEC_PARAMS struct.  XDAIS interface is now visibile from voplib and alglib but not applications
   Modified Mar 2020 JHB, handle name change of mediaThread_test_app.c to mediaMin.c
   Modified Sep 2020 JHB, mods for compatibility with gcc 9.3.0: include math.h (for min/max functions), fix various security and "indentation" warnings
*/

#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>
#include <math.h>

#include "mediaTest.h"

/* lib header files (all libs are .so format) */

#include "voplib.h"
#include "pktlib.h"
#include "alglib.h"
#include "diaglib.h"


/* Used to hold input file names for codec test mode */
/* when the ft_info struct is populated, memory is allocated to hold the filename strings */
char* encoder_input_files[MAX_CODEC_INSTANCES] = { NULL };
char* decoder_input_files[MAX_CODEC_INSTANCES] = { NULL };

int encoded_frame_cnt[MAX_CODEC_INSTANCES] = { 0 };
int decoded_frame_cnt[MAX_CODEC_INSTANCES] = { 0 };

char thread_status[2*MAX_CODEC_INSTANCES] = { 0 };

static HPLATFORM hPlatform = -1;  /* platform handle, see DSAssignPlatform() call */

extern PLATFORMPARAMS PlatformParams;  /* command line params */

/* local functions */

int get_codec_name(int codec_type, char* szCodecName);
const char* strrstr(const char* haystack, const char* needle);


#define AUDIO_SAMPLE_SIZE  2       /* in bytes.  Currently all codecs take 16-bit samples.  Some like AMR require 14-bit left-justified within 16 bits */

//unsigned int numChan;
int numChan = 1;

/* USB audio support enabled by default.  If command line input is given as -iusb0, -iusb1, etc then USB audio input is active; sampling rate, bitwidth, num channels, etc should be specified in session config file.  Comment this define to disable USB audio related code */

#define ENABLE_USBAUDIO

#if defined(_ALSA_INSTALLED_) && defined(ENABLE_USBAUDIO)  /* _ALSA_INSTALLED_ is defined in the mediaTest Makefile, which checks for ALSA /proc/asound folder */

#include "aviolib.h"

#define USB_AUDIO_BUFFER_TIMEOUT      500     /* in msec */
#define DEFAULT_USBAUDIO_PERIOD_SIZE  256     /* in samples */
#define MAX_USBAUDIO_FRAMESIZE        30      /* in msec */
#define MAX_USBAUDIO_SAMPLE_RATE      192000  /* in Hz */
#define MAX_USBAUDIO_CHANNELS         8
#define MAX_USBAUDIO_BUFLEN16         (MAX_USBAUDIO_SAMPLE_RATE*MAX_USBAUDIO_FRAMESIZE/1000*MAX_USBAUDIO_CHANNELS*AUDIO_SAMPLE_SIZE)
#define MAX_USBAUDIO_BUFLEN32         (MAX_USBAUDIO_BUFLEN16*2)

bool usb_audio_callback = 0;

short int buf16_in[MAX_USBAUDIO_BUFLEN16] = { 0 };
int buf32_in[MAX_USBAUDIO_BUFLEN32] = { 0 };
short int buf16_out[MAX_USBAUDIO_BUFLEN16] = { 0 };
int buf32_out[MAX_USBAUDIO_BUFLEN32] = { 0 };

unsigned int numChan_device = 2;  /* currently set for Focusrite 2i2.  These will be replaced with reference to SESSION_CONTROL struct, which will contain per-device info */
unsigned int bytesPerSample_device = 4;

snd_pcm_uframes_t period_size_USBAudio = DEFAULT_USBAUDIO_PERIOD_SIZE;
snd_pcm_uframes_t buffer_size_USBAudio = period_size_USBAudio * numChan_device * bytesPerSample_device * 2;  /* multiply by num channels * bytes per sample * 2 (allow double buffering) */
snd_pcm_uframes_t period_size_USBAudio_output = DEFAULT_USBAUDIO_PERIOD_SIZE;
snd_pcm_uframes_t buffer_size_USBAudio_output = period_size_USBAudio_output * numChan_device * bytesPerSample_device * 2;
snd_async_handler_t *pcm_callback_capture, *pcm_callback_playback;
snd_pcm_hw_params_t* hw_params;


void USBAudioCallbackCapture(snd_async_handler_t* pcm_callback) {	

unsigned int uFlags = DS_AVIO_BUFFER_USE_UPPER_16BITS;

   if (numChan == 1) uFlags |= DS_AVIO_BUFFER_LEFT_CHANNEL;

   DSReadAvioBuffer(NULL, pcm_callback, period_size_USBAudio, buf32_in, buf16_in, 0, uFlags);

   usb_audio_callback = 1;
}

void USBAudioCallbackPlayback(snd_async_handler_t* pcm_callback) {	

unsigned int uFlags = DS_AVIO_BUFFER_USE_UPPER_16BITS;

   if (numChan == 1) uFlags |= DS_AVIO_BUFFER_LEFT_CHANNEL;

#if 0
 static int count = 0;
 if (count < 100 && numChan == 2) {

  printf("buf16_out even = %d, buf16_out odd = %d\n", buf16_out[count], buf16_out[count+1]);
  count += 2;
 }
#endif

   DSWriteAvioBuffer(NULL, pcm_callback, period_size_USBAudio_output, buf32_out, buf16_out, 0, uFlags);

   usb_audio_callback = 1;
}

#endif  /* _ALSA_INSTALLED_ && ENABLE_USBAUDIO */

/* fread() wrapper to avoid this:

    /usr/include/x86_64-linux-gnu/bits/stdio2.h:285:71: warning: call to ¡®__fread_chk_warn¡¯ declared with attribute warning: fread called with bigger size * nmemb than length of destination buffer [enabled by default] return __fread_chk_warn (__ptr, __bos0 (__ptr), __size, __n, __stream);

  when -flto linker option is enabled with binutils 2.22.  This requires binutils 2.26 or 2.28 to fix
*/

static size_t __attribute__((optimize("O1"))) _fread(void *ptr, size_t size, size_t count, FILE *stream) {

   return fread(ptr, size, count, stream);
}

#define STRIP_FRAME_SILENCE    1
#define STRIP_FRAME_DTX        2
#define STRIP_FRAME_DTX_CONT   4
#define STRIP_FRAME_SOUNDS     8

#define SEGMENTER_INIT         1
#define SEGMENTER_FRAME        2
#define SEGMENTER_CLEANUP      4
#define SEGMENTER_PRINT_STATS  8

/* segment handler:  write out audio segments using fixed or adjusted intervals based on cmd line flags */

int segmenter(unsigned int uFlags, int frame_count, float codec_frame_duration, uint8_t uStripFrame, uint8_t* addr, int len, FILE** p_fp_out_segment, MEDIAINFO* pMediaInfoSegment, FILE** p_fp_out_concat, MEDIAINFO* pMediaInfoConcat, FILE** p_fp_out_stripped, MEDIAINFO* pMediaInfoStripped) {

char tmpstr[1024], segments_text_filename[1024];
FILE* fp_segments_text = NULL;
int ret_val, i;
float interval_limit = 1.0, interval_duration;
bool fCloseSegment = false;
static int frame_strip_count = 0, segment_frame_count = 0, segment_count = 0;
//static uint8_t uContAudioFrames = 0;
static short int marker_values[2] = { -32767, 32767 };
static short int segment_marker_save_samples_concat[2];
static short int segment_marker_save_samples_stripped[2];
static bool fMarkerWritten = false;
static int64_t fpos_marker_save_concat, fpos_marker_save_stripped;
#define MAX_PREV_FRAMES  64
static uint8_t uPrevStripFrame[MAX_PREV_FRAMES] = { 0 };
uint8_t zerobuf[MAX_RAW_FRAME] = { 0 };

   if (uFlags & SEGMENTER_PRINT_STATS) {

      printf("Segment stats: num segments = %d, num partial segments = %d \n", segment_count, segment_frame_count != frame_count ? 1 : 0);
      return 1;
   }

   if ((((nSegmentation & DS_SEGMENT_OUTPUT_CONCATENATE) && (!p_fp_out_concat || !*p_fp_out_concat)) || ((nSegmentation & DS_SEGMENT_OUTPUT_STRIPPED) && (!p_fp_out_stripped || !*p_fp_out_stripped))) || (!p_fp_out_segment || !*p_fp_out_segment)) return 0;  /* make sure audio output and current audio segment file handles are valid */

   if (uFlags & SEGMENTER_CLEANUP) {

      if ((nSegmentation & DS_SEGMENT_ADD_MARKERS) && fMarkerWritten) {

         if (nSegmentation & DS_SEGMENT_OUTPUT_CONCATENATE) {

            fclose(*p_fp_out_concat);
            *p_fp_out_concat = fopen(pMediaInfoConcat->szFilename, "rb+");
            fseek(*p_fp_out_concat, fpos_marker_save_concat, SEEK_SET);

            #ifdef MARKER_DEBUG
            int64_t fpos = ftell(*p_fp_out_concat);
            size_t ret_val = fwrite(segment_marker_save_samples_concat, AUDIO_SAMPLE_SIZE, 2, *p_fp_out_concat);
            int64_t fpos2 = ftell(*p_fp_out_concat);
            printf(" $$$$$$$$$ inside marker restore, fpos = %ld, fpos2 = %ld, ret_val = %lu, val1 = %d, val2 = %d \n", fpos, fpos2, ret_val, segment_marker_save_samples_concat[0], segment_marker_save_samples_concat[1]);
            #else
            fwrite(segment_marker_save_samples_concat, AUDIO_SAMPLE_SIZE, 2, *p_fp_out_concat);
            #endif
         }

         if (nSegmentation & DS_SEGMENT_OUTPUT_STRIPPED) {

            fclose(*p_fp_out_stripped);
            *p_fp_out_stripped = fopen(pMediaInfoStripped->szFilename, "rb+");
            fseek(*p_fp_out_stripped, fpos_marker_save_stripped, SEEK_SET);

            #ifdef MARKER_DEBUG
            int64_t fpos = ftell(*p_fp_out_stripped);
            size_t ret_val = fwrite(segment_marker_save_samples_stripped, AUDIO_SAMPLE_SIZE, 2, *p_fp_out_stripped);
            int64_t fpos2 = ftell(*p_fp_out_stripped);
            printf(" $$$$$$$$$ inside marker restore, fpos = %ld, fpos2 = %ld, ret_val = %lu, val1 = %d, val2 = %d \n", fpos, fpos2, ret_val, segment_marker_save_samples_stripped[0], segment_marker_save_samples_stripped[1]);
            #else
            fwrite(segment_marker_save_samples_stripped, AUDIO_SAMPLE_SIZE, 2, *p_fp_out_stripped);
            #endif
         }

         fMarkerWritten = false;
      }

      return 1;
   }

   if (uStripFrame) {
   
      frame_strip_count++;

      if (nSegmentation & DS_SEGMENT_OUTPUT_STRIPPED) {

         ret_val = DSSaveDataFile(DS_GM_HOST_MEM, p_fp_out_stripped, NULL, (uintptr_t)addr, len, DS_WRITE, pMediaInfoStripped);
      }
   }
   else {  /* write to audio segment file(s) */

      ret_val = DSSaveDataFile(DS_GM_HOST_MEM, p_fp_out_segment, NULL, (uintptr_t)addr, len, DS_WRITE, pMediaInfoSegment);
      if (len) fMarkerWritten = false;

      if (nSegmentation & DS_SEGMENT_OUTPUT_CONCATENATE) {

         ret_val = DSSaveDataFile(DS_GM_HOST_MEM, p_fp_out_concat, NULL, (uintptr_t)addr, len, DS_WRITE, pMediaInfoConcat);
      }

      if (nSegmentation & DS_SEGMENT_OUTPUT_STRIPPED) {

         ret_val = DSSaveDataFile(DS_GM_HOST_MEM, p_fp_out_stripped, NULL, (uintptr_t)zerobuf, len, DS_WRITE, pMediaInfoStripped);
      }
   }

   interval_duration = (frame_count - frame_strip_count - segment_frame_count) * codec_frame_duration;

   if ((nSegmentation & DS_SEGMENT_DEBUG_INFO) && uStripFrame && !(uStripFrame & STRIP_FRAME_SILENCE)) {
      printf(" Strip silence: found DTX not already silence, type = %d, duration = %4.1f \n", uStripFrame, interval_duration);
      for (i=0; i<MAX_PREV_FRAMES; i++) printf("%s%d%s", i == 0 ? "\t\t\t Prev frames " : "", uPrevStripFrame[i], i == MAX_PREV_FRAMES-1 ? "\n" : ",");
   }

  /* save frame strip history */

   for (i=MAX_PREV_FRAMES-1; i>0; i--) uPrevStripFrame[i] = uPrevStripFrame[i-1];
   uPrevStripFrame[i] = uStripFrame;

/* if specified in cmd line flags, look for adjustable segmentation point based on silence and/or sound */

   if ((nSegmentation & DS_SEGMENT_ADJUST) && (nSegmentation & DS_STRIP_SILENCE)) {

//      interval_limit = 1.1;  /* if adjustable segmentation point not found within -30% <= interval <= +10%, close the segment */
//      if (uStripFrame && interval_duration >= interval_limit * nSegmentInterval) fCloseSegment = true;

      if (uStripFrame /*&& uContAudioFrames > 3*/ && interval_duration >= (nSegmentInterval > 0 ? nSegmentInterval : 250)) fCloseSegment = true;  /* cmd line -IN entry (interval) treated as minimum */
//      if (!uStripFrame) uContAudioFrames++;
//      else uContAudioFrames = 0;
   }
   else fCloseSegment = interval_duration >= interval_limit * (nSegmentInterval > 0 ? nSegmentInterval : 2000);  /* cmd line -IN entry (interval) treated as maximum */

/* depending on fixed or adjustable interval duration, close audio segment file and open next one */

   //printf(" $$$$$$$$ frame_count = %d, strip count = %d, count = %f, last count = %d, interval = %f \n", frame_count, frame_strip_count, *segment_frame_count, (frame_count - frame_strip_count - *segment_frame_count) * codec_frame_duration, interval_limit * nSegmentInterval);

   if (fCloseSegment) {

      if (nSegmentation & DS_SEGMENT_TIMESTAMPS_SCREEN) printf("Segment %d interval duration = %4.1f (msec), type = %d \n", segment_count, interval_duration, uStripFrame);

      DSSaveDataFile(DS_GM_HOST_MEM, p_fp_out_segment, NULL, (uintptr_t)NULL, 0, DS_CLOSE, pMediaInfoSegment);  *p_fp_out_segment = NULL;  /* close current audio segment file */

      strcpy(tmpstr, pMediaInfoSegment->szFilename);
      char* p = strrchr(tmpstr, '.');  if (p) *p = 0;
      char* p2 = (char*)strrstr(tmpstr, "_seg");  if (p2) *p2 = 0;
  
      sprintf(pMediaInfoSegment->szFilename, "%s_seg%d%s%s", tmpstr, segment_count + 1, p ? "." : "", p ? p+1 : "");
      if (nSegmentation & DS_SEGMENT_TIMESTAMPS_TEXT) sprintf(segments_text_filename, "%s_seg_ts.txt", tmpstr);  /* form timestamp text filename at this point, if it will be needed */

      //printf(" $$$$$$$$ MediaInfoSegment.szFilename = %s \n", MediaInfoSegment.szFilename);

      ret_val = DSSaveDataFile(DS_GM_HOST_MEM, p_fp_out_segment, pMediaInfoSegment->szFilename, (uintptr_t)NULL, 0, DS_CREATE, pMediaInfoSegment);  /* open next audio segment file */
      if (ret_val <= 0) *p_fp_out_segment = NULL;

   /* add markers to concatenated segment and/or stripped content audio file(s), if specified in cmd line flags */

      if (nSegmentation & DS_SEGMENT_ADD_MARKERS) {

         if (nSegmentation & DS_SEGMENT_OUTPUT_CONCATENATE) {

            fclose(*p_fp_out_concat);
            *p_fp_out_concat = fopen(pMediaInfoConcat->szFilename, "rb+");  /* re-open in read-update mode, avoid read/write buffering and sync problems found in write-update mode (see also comments about "change in I/O direction" in fopen() doc) */
            fseek(*p_fp_out_concat, 0L, SEEK_END);

            #ifdef MARKER_DEBUG
            int64_t fpos = ftell(*p_fp_out_concat);
            int ret_val_seek1 = fseek(*p_fp_out_concat, -2L*AUDIO_SAMPLE_SIZE, SEEK_CUR);
            int64_t fpos2 = ftell(*p_fp_out_concat);
            #else
            fseek(*p_fp_out_concat, -2L*AUDIO_SAMPLE_SIZE, SEEK_CUR);
            #endif

            fpos_marker_save_concat = ftell(*p_fp_out_concat);
            size_t __attribute__((unused)) ret_val = fread(segment_marker_save_samples_concat, AUDIO_SAMPLE_SIZE, 2, *p_fp_out_concat);  /* save waveform values prior to marking, may be needed during segmentation cleanup */

            #ifdef MARKER_DEBUG
            int ret_val_seek2 = fseek(*p_fp_out_concat, -2L*AUDIO_SAMPLE_SIZE, SEEK_CUR);
            int64_t fpos3 = ftell(*p_fp_out_concat);
            size_t ret_val2 = fwrite(marker_values, AUDIO_SAMPLE_SIZE, 2, *p_fp_out_concat);
            int64_t fpos4 = ftell(*p_fp_out_concat);
            printf(" $$$$$$$$$ writing marker, fpos = %ld, fpos2 = %ld, fpos3 = %ld,  fpos4 = %ld, ret_val = %lu, ret_val2 = %lu, rvs1 = %d, rvs2 = %d, val1 = %d, val2 = %d \n", fpos, fpos2, fpos3, fpos4, ret_val, ret_val2, ret_val_seek1, ret_val_seek2, segment_marker_save_samples_concat[0], segment_marker_save_samples_concat[1]);
            #else
            fseek(*p_fp_out_concat, -2L*AUDIO_SAMPLE_SIZE, SEEK_CUR);
            fwrite(marker_values, AUDIO_SAMPLE_SIZE, 2, *p_fp_out_concat);
            #endif
         }

         if (nSegmentation & DS_SEGMENT_OUTPUT_STRIPPED) {

            fclose(*p_fp_out_stripped);
            *p_fp_out_stripped = fopen(pMediaInfoStripped->szFilename, "rb+");  /* re-open in read-update mode, avoid read/write buffering and sync problems found in write-update mode (see also comments about "change in I/O direction" in fopen() doc) */
            fseek(*p_fp_out_stripped, 0L, SEEK_END);

            #ifdef MARKER_DEBUG
            int64_t fpos = ftell(*p_fp_out_stripped);
            int ret_val_seek1 = fseek(*p_fp_out_stripped, -2L*AUDIO_SAMPLE_SIZE, SEEK_CUR);
            int64_t fpos2 = ftell(*p_fp_out_stripped);
            #else
            fseek(*p_fp_out_stripped, -2L*AUDIO_SAMPLE_SIZE, SEEK_CUR);
            #endif

            fpos_marker_save_stripped = ftell(*p_fp_out_stripped);
            size_t __attribute__((unused)) ret_val = fread(segment_marker_save_samples_stripped, AUDIO_SAMPLE_SIZE, 2, *p_fp_out_stripped);  /* save waveform values prior to marking, may be needed during segmentation cleanup */

            #ifdef MARKER_DEBUG
            int ret_val_seek2 = fseek(*p_fp_out_stripped, -2L*AUDIO_SAMPLE_SIZE, SEEK_CUR);
            int64_t fpos3 = ftell(*p_fp_out_stripped);
            size_t ret_val2 = fwrite(marker_values, AUDIO_SAMPLE_SIZE, 2, *p_fp_out_stripped);
            int64_t fpos4 = ftell(*p_fp_out_stripped);
            printf(" $$$$$$$$$ writing marker, fpos = %ld, fpos2 = %ld, fpos3 = %ld,  fpos4 = %ld, ret_val = %lu, ret_val2 = %lu, rvs1 = %d, rvs2 = %d, val1 = %d, val2 = %d \n", fpos, fpos2, fpos3, fpos4, ret_val, ret_val2, ret_val_seek1, ret_val_seek2, segment_marker_save_samples_stripped[0], segment_marker_save_samples_stripped[1]);
            #else
            fseek(*p_fp_out_stripped, -2L*AUDIO_SAMPLE_SIZE, SEEK_CUR);
            fwrite(marker_values, AUDIO_SAMPLE_SIZE, 2, *p_fp_out_stripped);
            #endif
         }

         fMarkerWritten = true;
      }

      if (nSegmentation & DS_SEGMENT_TIMESTAMPS_TEXT) {

         char fmode[10];
         if (access(segments_text_filename, F_OK) != -1 ) strcpy(fmode, "w");  /* create */
         else strcpy(fmode, "a");  /* append */

         fp_segments_text = fopen(segments_text_filename, fmode);

         if (fp_segments_text) {

            sprintf(tmpstr, "%4.1f, %d \n", interval_duration, uStripFrame);
            fwrite(tmpstr, 1, strlen(tmpstr), fp_segments_text);  /* write timestamp data */
            fclose(fp_segments_text);
         }
      }

      segment_count++;  /* update segment count */
      segment_frame_count = frame_count - frame_strip_count;  /* update segment frame count */
   }

   return 1;
}

/* main function entry */

void x86_mediaTest(void) {

   printf("x86 mediaTest start\n");
   
   if (codec_test)
   {
      uint8_t in_buf[MAX_RAW_FRAME*24];  /* 24 is sampling rate conversion worst case:  192 kHz down to 8 kHz */
      uint8_t coded_buf[MAX_CODED_FRAME], coded_buf_sav[MAX_CODED_FRAME];
      uint8_t out_buf[MAX_RAW_FRAME*24] = { 0 };
      int ret_val;
      int frame_size = -1, i;

      FILE *fp_in = NULL, *fp_out = NULL;
      int frame_count = 0;
      char tmpstr[1024], tmpstr2[1024];
      codec_test_params_t  codec_test_params;
      char default_config_file[] = "session_config/codec_test_config";
      char *config_file;
      int len;
      unsigned int inbuf_size;
      uintptr_t addr;
      char key;
      unsigned int sampleRate_input = 0, sampleRate_output, sampleRate_codec;
      int input_framesize;  /* in bytes, determined by input sampling rate and codec or pass-thru framesize */
      int coded_framesize = 0;
      unsigned int __attribute__((unused)) output_framesize;  /* currently not used unless _ALSA_INSTALLED_ is defined, but likely to be used in the future */

#ifdef _MELPE_INSTALLED_

/* some MELPe specific items needed to handle "packed bit density" mode, where bytes are split across frame boundaries (MELPe supports a "sync bit" for bit and byte serial channels, similar to the sync word used in RF communications) */

      unsigned int melpe_decoder_pattern_index = 0;
      unsigned int melpe_decoder_56bd_pattern[4] = { 7, 7, 7, 6 };
      unsigned int melpe_decoder_88bd_pattern[8] = { 11, 10, 10, 10, 10, 10, 10, 10 };
#endif

#if defined(_ALSA_INSTALLED_) && defined(ENABLE_USBAUDIO)
      snd_pcm_t *usb_device_capture = NULL, *usb_device_playback = NULL;
      uint64_t t1_usb = 0, t2_usb;
      unsigned int sampleRate_USBAudio = 48000;  /* USB audio input has been tested with the Focusrite 2i2 unit, which supports sampling rates 44.1, 48, 88.2, 96, 176.4, and 192 kHz.  Default rate is 48 kHz to allow accurate and computationally
                                                    minimal conversion between codec sampling rates, such as 8 kHz (G711, G729, MELPe, etc), 16 kHz (AMR-WB, EVS, etc), or 32 kHz (super wideband). 44.1 kHz is used if no config file given */
      char hwDevice[100] = "";
      char szPortName[100] = "";
      bool fUSBTestMode = false;
      bool fFirstUSBAudioBuffer = false;
#endif

      short int fs_convert_delay_buf[MAX_SAMPLES_FRAME*24][8] = {{ 0 }};  /* 24 is sampling rate conversion worst case:  192 kHz down to 8 kHz */
      short int fs_convert_delay_buf_output[MAX_SAMPLES_FRAME*24][8] = {{ 0 }};
      unsigned int upFactor, downFactor, upFactor_output, downFactor_output;
      float codec_frame_duration = 0;  /* in msec */

      FILE *fp_cfg = NULL;

      MEDIAINFO MediaInfo = { 0 };

      struct timeval tv;
      uint64_t t1, t2;
      char szCodecName[20] = "";
      char szNumChan[20] = "";
      bool fFramePrint = false;

      bool fCreateCodec = true;  /* set to false for pass-thru case (no codecs specified) */
      HCODEC encoder_handle = 0, decoder_handle = 0;  /* 0 = not initialized, < 0 indicates an error, > 0 is valid codec handle */
      CODEC_PARAMS CodecParams = { 0 };  /* see voplib.h */
      CODEC_OUTARGS encOutArgs = { 0 };  /* currently only used by AMR-WB+, see comments below */

      #define MAX_SID_FRAMESIZE 10
      int nSIDStats[MAX_SID_FRAMESIZE] = { 0 };
      bool fPrintSIDStats = false;
      uint8_t uStripFrame = 0;
      MEDIAINFO MediaInfoSegment = { 0 };
      MEDIAINFO MediaInfoConcat = { 0 };
      MEDIAINFO MediaInfoStripped = { 0 };
      FILE* fp_out_segment = NULL, *fp_out_concat = NULL, *fp_out_stripped = NULL;
      char* p;


   /* start of code for codec test mode */

      printf("x86 codec test start\n");

      hPlatform = DSAssignPlatform(NULL, PlatformParams.szCardDesignator, 0, 0, 0);  /* assign platform handle, needed for concurrency and VM management */

      DSConfigVoplib(NULL, NULL, DS_CV_INIT);  /* initialize voplib */

   /* look at in and out file types (see cmd line parsing at start of main() in mediaTest.c */

      if (inFileType == ENCODED && outFileType == ENCODED) {
         fprintf (stderr, "ERROR: both input and output file types encoded is unsupported\n");
         goto codec_test_cleanup;
      }

      if (inFileType != USB_AUDIO) {

         if (inFileType != ENCODED) {

            DSLoadDataFile(DS_GM_HOST_MEM, &fp_in, MediaParams[0].Media.inputFilename, (uintptr_t)NULL, 0, DS_OPEN, &MediaInfo);  /* for wav files, pMediaInfo will be initialized with wav file header info */
         }
         else {

            fp_in = fopen(MediaParams[0].Media.inputFilename, "rb");
         }

         if (fp_in) printf("Opened audio input file %s\n", MediaParams[0].Media.inputFilename);
         else {
            printf("Unable to open audio input file %s\n", MediaParams[0].Media.inputFilename);
            goto codec_test_cleanup;
         }

      /* use results of DSLoadDataFile() if valid */

         if (MediaInfo.Fs > 0) sampleRate_input = MediaInfo.Fs;
         if (MediaInfo.NumChan > 0) numChan = MediaInfo.NumChan;
      }

#if defined(_ALSA_INSTALLED_) && defined(ENABLE_USBAUDIO)

      if (inFileType == USB_AUDIO || (outFileType & USB_AUDIO)) {

         if ((USBAudioInput & AUDIO_INPUT_USB0) || (USBAudioOutput & AUDIO_OUTPUT_USB0)) {
            strcpy(hwDevice, "hw:0,0");
            strcpy(szPortName, "usb0");
         }
         else if ((USBAudioInput & AUDIO_INPUT_USB1) || (USBAudioOutput & AUDIO_OUTPUT_USB1)) {
            strcpy(hwDevice, "hw:1,0");
            strcpy(szPortName, "usb1");
         }

         if (inFileType == USB_AUDIO) {

            usb_device_capture = DSOpenAvioDevice(hw_params, DS_SND_PCM_STREAM_CAPTURE, buffer_size_USBAudio, period_size_USBAudio, NULL, NULL, hwDevice, sampleRate_USBAudio);  /* valid port check, we will re-open later.  In this case, open with NULL for callback handler, no handler will be initialized */

            if (usb_device_capture) {

               fprintf(stderr, "Opened %s audio capture port\n", szPortName);
               usleep(100*1000);
               DSCloseAvioDevice(usb_device_capture, NULL);  /* we will re-open the USB device when we know required sampling rate, number of channels, etc.  But we need to check it early, and let the user know if the device is turned off, not attached, or other problem */
               usb_device_capture = NULL;
            }
            else {

               fprintf(stderr, "Unable to open %s audio capture port\n", szPortName);
               goto codec_test_cleanup;
            }

            sampleRate_input = sampleRate_USBAudio;
         }

         if (outFileType & USB_AUDIO) {

            usb_device_playback = DSOpenAvioDevice(hw_params, DS_SND_PCM_STREAM_PLAYBACK, buffer_size_USBAudio_output, period_size_USBAudio_output, NULL, NULL, hwDevice, sampleRate_USBAudio);

            if (usb_device_playback) {

               fprintf(stderr, "Opened %s audio playback port\n", szPortName);
               usleep(100*1000);
               DSCloseAvioDevice(usb_device_playback, NULL);  /* we will re-open the USB device when we know required sampling rate, number of channels, etc.  But we need to check it early, and let the user know if the device is turned off, not attached, or other problem */

#define ALSA_HANG_DEBUG

               #ifdef ALSA_HANG_DEBUG
               fprintf(stderr, "After DSCloseAvioDevice()\n");
               #endif

               usb_device_playback = NULL;
            }
            else {

               printf("Unable to open %s audio playback port\n", szPortName);
               goto codec_test_cleanup;
            }
         }
      }
#endif

   /* Config file handling:  (i) give an error if config file doesn't exist, (ii) use default file only if no config file given and input waveform file appears to be a 3GPP test vector, (iii) otherwise go with input waveform header and/or test mode.  JHB Mar 2018 */

      if (strlen(MediaParams[0].configFilename) == 0) {

         if (strstr(MediaParams[0].Media.inputFilename, "stv")) config_file = default_config_file;  /* use default config file only if input waveform seems to be a 3GPP test vector */
         else config_file = NULL;
      }
      else if (access(MediaParams[0].configFilename, F_OK ) == -1) {

         printf("Codec config file %s not found\n", MediaParams[0].configFilename);
         goto codec_test_cleanup;
      }
      else config_file = MediaParams[0].configFilename;

      if (config_file) {

         printf("Opening codec config file: %s\n", config_file);

         fp_cfg = fopen(config_file, "r");
      }
      else fp_cfg = NULL;

      if (!fp_cfg) {

         codec_test_params.codec_type = DS_VOICE_CODEC_TYPE_NONE;

         if (!USBAudioInput) {

            if (sampleRate_input == 0) sampleRate_input = 8000;  /* if input was raw audio file with no header, and no codec specified, then we need to set an arbitrary sampling rate value */
            numChan = max(MediaInfo.NumChan, 1);
         }
         else {  /* for USB audio input testing with no config file, we set hardcoded params.  Currently we use params supported by Focusrite 2i2 */

#if defined(_ALSA_INSTALLED_) && defined(ENABLE_USBAUDIO)
            fUSBTestMode = true;
#endif

            sampleRate_input = 44100;
            numChan = 2;
         }

         sampleRate_output = sampleRate_input;

         printf("No config file specified, assuming default parameters: ");
      }
      else {

         parse_codec_test_params(fp_cfg, &codec_test_params);

         sampleRate_output = codec_test_params.sample_rate;
         if (sampleRate_input == 0) sampleRate_input = sampleRate_output;  /* raw audio file with no header */

         numChan = codec_test_params.num_chan;  /* default is 1 if num_chan is not specified in the codec config file */
//printf("numChan = %d\n", numChan);
         printf("Opened config file: ");
      }

   /* update MediaInfo struct if it still doesn't have valid numbers */

      if (!MediaInfo.Fs) MediaInfo.Fs = sampleRate_input;;
      if (!MediaInfo.NumChan) MediaInfo.NumChan = numChan;
      if (!MediaInfo.SampleWidth) MediaInfo.SampleWidth = DS_DP_SHORTINT;
      if (!MediaInfo.CompressionCode) MediaInfo.CompressionCode = DS_GWH_CC_PCM;


      if (!get_codec_name(codec_test_params.codec_type, szCodecName)) {

         printf("Error: non-supported or invalid codec type found in config file\n");
         goto codec_test_cleanup;
      }

      printf("codec = %s, ", szCodecName);
      if (codec_test_params.codec_type != DS_VOICE_CODEC_TYPE_NONE) printf("%d bitrate, ", codec_test_params.bitrate);
      printf("sample rate = %d Hz\n", sampleRate_output);

      if (codec_test_params.codec_type != DS_VOICE_CODEC_TYPE_NONE && (int)codec_test_params.bitrate <= 0) {

         printf("Error: config file specifies a codec but not a bitrate\n");
         goto codec_test_cleanup;
      }

      memset(&CodecParams, 0, sizeof(CodecParams));

   /* setup/init for specified codec.  Codecs use voplib APIs */

      switch (codec_test_params.codec_type) {

         case DS_VOICE_CODEC_TYPE_EVS:
         {
            CodecParams.enc_params.samplingRate = codec_test_params.sample_rate;       /* in Hz */
            CodecParams.enc_params.bitRate = codec_test_params.bitrate;                /* in bps */
            CodecParams.enc_params.dtx.dtx_enable = codec_test_params.dtx_enable;      /* 0 = DTX disabled, 1 = enabled */
            CodecParams.enc_params.sid_update_interval = codec_test_params.dtx_value ? codec_test_params.dtx_value : (codec_test_params.dtx_enable ? 8 : 0);  /* if DTX is enabled then default SID update interval is 8.  A zero update interval enables "adaptive SID" */
            CodecParams.enc_params.rf_enable = codec_test_params.rf_enable;
            CodecParams.enc_params.fec_indicator = codec_test_params.fec_indicator;
            CodecParams.enc_params.fec_offset = codec_test_params.fec_offset;
            CodecParams.enc_params.bandwidth_limit = DS_EVS_BWL_SWB;                   /* codec will set limit depending on sampling rate */
            CodecParams.enc_params.rtp_pyld_hdr_format.header_format = 1;              /* hard coded to 1 to match 3GPP encoder reference executable, which only writes header full format */

         /* EVS codec DTX notes:

            1) DTX should be specified in codec configuration file.  If not given, default is disabled
            2) EVS codec is used for silence stripping and audio segmentation.  In that case we enable DTX and set the update interval to 0.  An update interval of 0 specifies "adaptive SID"
         */

            if (nSegmentation & DS_STRIP_SILENCE) {

               CodecParams.enc_params.dtx.dtx_enable = 1;
               CodecParams.enc_params.sid_update_interval = 0;  /* notes: (i) zero is "adaptive SID", (ii) default for normal telecom operation is 8 */
               printf("  Strip silence: EVS encoder DTX = %d, sid update interval = %d, nSegmentation = 0x%x \n",  CodecParams.enc_params.dtx.dtx_enable, CodecParams.enc_params.sid_update_interval, nSegmentation);
            }

            CodecParams.dec_params.samplingRate = codec_test_params.sample_rate;
            CodecParams.dec_params.bitRate = codec_test_params.bitrate;             /* we set this to avoid param validation error in DSCodecCreate().  At run-time EVS codec determines bitrate from compressed bitstream info */

            codec_frame_duration = 20;  /* in msec */
            sampleRate_codec = codec_test_params.sample_rate;

            break;
         }

         case DS_VOICE_CODEC_TYPE_G711_ULAW:
         case DS_VOICE_CODEC_TYPE_G711_ALAW:
            codec_frame_duration = 20;  /* in msec */
            break;

#ifdef _AMR_INSTALLED_
         case DS_VOICE_CODEC_TYPE_AMR_NB:
         {
            CodecParams.enc_params.samplingRate = 8000;                  /* in Hz */
            CodecParams.enc_params.bitRate = codec_test_params.bitrate;  /* in bps */
            CodecParams.enc_params.dtx.vad = codec_test_params.vad;

            CodecParams.dec_params.samplingRate = 8000;
            CodecParams.dec_params.bitRate = codec_test_params.bitrate;  /* we set this to avoid param validation error in DSCodecCreate().  At run-time AMR-NB codec determines bitrate from compressed bitstream info */

            codec_frame_duration = 20;  /* in msec */
            sampleRate_codec = 8000;

            break;
         }
#endif

#ifdef _AMRWB_INSTALLED_
         case DS_VOICE_CODEC_TYPE_AMR_WB:
         {
            CodecParams.enc_params.samplingRate = 16000;                 /* in Hz */
            CodecParams.enc_params.bitRate = codec_test_params.bitrate;  /* in bps */
            CodecParams.enc_params.dtx.vad = codec_test_params.vad;
//            CodecParams.enc_params.rtp_pyld_hdr_format.oct_align = codec_test_params.header_format;

            CodecParams.dec_params.samplingRate = 16000;
            CodecParams.dec_params.bitRate = codec_test_params.bitrate;  /* we set this to avoid param validation error in DSCodecCreate().  At run-time AMR-WB codec determines bitrate from compressed bitstream info */

            codec_frame_duration = 20;  /* in msec */
            sampleRate_codec = 16000;

            break;
         }
#endif

#ifdef _AMRWBPLUS_INSTALLED_
         case DS_VOICE_CODEC_TYPE_AMR_WB_PLUS:
         {
            CodecParams.enc_params.samplingRate = codec_test_params.sample_rate;                                   /* in Hz */
            CodecParams.enc_params.bitRate = (int)codec_test_params.mode == -1 ? codec_test_params.bitrate_plus : 0.0;  /* in bps */
            CodecParams.enc_params.mode = codec_test_params.mode;
            CodecParams.enc_params.isf = codec_test_params.isf;
            CodecParams.enc_params.low_complexity = codec_test_params.low_complexity;
            CodecParams.enc_params.dtx.vad = codec_test_params.vad;
            CodecParams.enc_params.nChannels = codec_test_params.num_chan;
            CodecParams.enc_params.mono = codec_test_params.mono;

            CodecParams.dec_params.samplingRate = codec_test_params.sample_rate;
            CodecParams.dec_params.bitRate = CodecParams.enc_params.bitRate;  /* we set this to avoid param validation error in DSCodecCreate().  At run-time AMR-WB+ codec determines bitrate from compressed bitstream info */
            CodecParams.dec_params.limiter = codec_test_params.limiter;
            CodecParams.dec_params.mono = codec_test_params.mono;

            //if (codec_test_params.mono == 0) numChan = 2;

            codec_frame_duration = 80; /* 80 msec super frame */
            sampleRate_codec = codec_test_params.sample_rate;

            break;
         }
#endif

#ifdef _G726_INSTALLED_
         case DS_VOICE_CODEC_TYPE_G726:
         {
            CodecParams.enc_params.samplingRate = 8000;                  /* in Hz */
            CodecParams.enc_params.bitRate = codec_test_params.bitrate;  /* in bps */
            CodecParams.enc_params.uncompress = codec_test_params.uncompress;

            CodecParams.dec_params.samplingRate = 8000;
            CodecParams.dec_params.bitRate = codec_test_params.bitrate;
            CodecParams.dec_params.uncompress = codec_test_params.uncompress;

            codec_frame_duration = 10;  /* in msec */
            sampleRate_codec = 8000;

            break;
         }
#endif

#ifdef _G729AB_INSTALLED_
         case DS_VOICE_CODEC_TYPE_G729AB:
         {
            CodecParams.enc_params.samplingRate = 8000;  /* in Hz */
            CodecParams.enc_params.bitRate = 8000;       /* in bps  */
            CodecParams.enc_params.dtx.vad = codec_test_params.vad;
            CodecParams.enc_params.uncompress = codec_test_params.uncompress;

            CodecParams.dec_params.samplingRate = 8000;
            CodecParams.dec_params.bitRate = 8000;
            CodecParams.dec_params.uncompress = codec_test_params.uncompress;

            codec_frame_duration = 10;  /* in msec */
            sampleRate_codec = 8000;

            break;
         }
#endif

#ifdef _MELPE_INSTALLED_
         case DS_VOICE_CODEC_TYPE_MELPE:
         {
            printf("  MELPe bit packing density = %d, NPP = %d, Post Filter = %d\n", codec_test_params.bitDensity, codec_test_params.Npp, codec_test_params.post);  /* print additional codec-specific info */

            CodecParams.enc_params.samplingRate = 8000;                  /* in Hz */
            CodecParams.enc_params.bitRate = codec_test_params.bitrate;  /* in bps */
            CodecParams.enc_params.bitDensity = codec_test_params.bitDensity;
            CodecParams.enc_params.Npp = codec_test_params.Npp;

            CodecParams.dec_params.samplingRate = 8000;
            CodecParams.dec_params.bitRate = codec_test_params.bitrate;
            CodecParams.dec_params.bitDensity = codec_test_params.bitDensity;
            CodecParams.dec_params.post = codec_test_params.post;

            switch (codec_test_params.bitrate) {
               case 600:
                  codec_frame_duration = 90;  /* in msec */
                  break;
               case 1200:
                  codec_frame_duration = 67.5;  /* in msec */
                  break;
               case 2400:
                  codec_frame_duration = 22.5;  /* in msec */
                  break;
            }

            sampleRate_codec = 8000;

            // if (inFileType != ENCODED) printf("MELPe before DSCodecCreate (encoder), bitrate = %d, bitDensity = %d, Npp = %d\n", encoderParams.bitRate, encoderParams.bitDensity, encoderParams.Npp);
            // if (outFileType != ENCODED) printf("MELPe before DSCodecCreate (decoder), bitrate = %d, bitDensity = %d, post filter = %d\n", decoderParams.bitRate, decoderParams.bitDensity, decoderParams.post);

            break;
         }
#endif

         default:
            codec_frame_duration = 20;
            fCreateCodec = false;
            break;
      }

      if (fCreateCodec) {

         CodecParams.enc_params.frameSize = CodecParams.dec_params.frameSize = codec_frame_duration;  /* in msec */
         CodecParams.codec_type = codec_test_params.codec_type;

         if ((inFileType != ENCODED) && (encoder_handle = DSCodecCreate(&CodecParams, DS_CC_CREATE_ENCODER)) < 0) {
            printf("codec test mode, failed to init encoder\n");
            goto codec_test_cleanup;
         }

         if ((outFileType != ENCODED) && (decoder_handle = DSCodecCreate(&CodecParams, DS_CC_CREATE_DECODER)) < 0) {
            printf("codec test mode, failed to init decoder\n");
            goto codec_test_cleanup;
         }
      }

   /* set up and down factors for possible sampling rate conversion (applied if sampleRate_input != sampleRate_output) */

      upFactor = sampleRate_output > sampleRate_input ? sampleRate_output / sampleRate_input : 1;
      downFactor = sampleRate_input > sampleRate_output ? sampleRate_input / sampleRate_output : 1;

#if defined(_ALSA_INSTALLED_) && defined(ENABLE_USBAUDIO)
      upFactor_output = sampleRate_USBAudio > sampleRate_output ? sampleRate_USBAudio / sampleRate_output : 1;
#else
      upFactor_output= 1;
#endif
      downFactor_output = 1;

   /* set buffers and frame sizes */

      input_framesize = codec_frame_duration*(1.0*sampleRate_input/1000)*AUDIO_SAMPLE_SIZE;  /* codec_frame_duration is floating-point value in msec */
      output_framesize = codec_frame_duration*(1.0*sampleRate_codec/1000)*AUDIO_SAMPLE_SIZE;

   /* set codec specific things */

      switch (codec_test_params.codec_type) {

         case DS_VOICE_CODEC_TYPE_G726:

            coded_framesize = DSGetCompressedFramesize(codec_test_params.codec_type, codec_test_params.bitrate, 0);
            break;

         case DS_VOICE_CODEC_TYPE_G729AB:

            coded_framesize = DSGetCompressedFramesize(codec_test_params.codec_type, 0, 0);
            break;

         case DS_VOICE_CODEC_TYPE_EVS:
         case DS_VOICE_CODEC_TYPE_AMR_NB:
         case DS_VOICE_CODEC_TYPE_AMR_WB:
         case DS_VOICE_CODEC_TYPE_AMR_WB_PLUS:

            coded_framesize = DSGetCompressedFramesize(codec_test_params.codec_type, codec_test_params.bitrate, HEADERFULL);
//            printf("input_framesize = %d, coded_framesize = %d\n", input_framesize, coded_framesize);
            break;

         case DS_VOICE_CODEC_TYPE_MELPE:

            if (!codec_test_params.bitDensity) codec_test_params.bitDensity = 54;  /* default bit density handling should be moved to transcoder_control.c */
            coded_framesize = DSGetCompressedFramesize(codec_test_params.codec_type, codec_test_params.bitrate, codec_test_params.bitDensity);
            break;

         case DS_VOICE_CODEC_TYPE_NONE:

#if defined(_ALSA_INSTALLED_) && defined(ENABLE_USBAUDIO)
            if (fUSBTestMode) input_framesize = period_size_USBAudio*AUDIO_SAMPLE_SIZE;  /* for USB test mode, use hardcoded params (see above) */
#endif
            break;
      }

      if (codec_test_params.codec_type != DS_VOICE_CODEC_TYPE_NONE && !coded_framesize) {

         printf("Error: DSGetCompressedFramesize() returns zero\n");
         goto codec_test_cleanup;
      }

   /* set buffer size just prior to codec (or pass-thru) input.  Note that coded_buf is not used for pass-thru mode */

      inbuf_size = input_framesize*upFactor/downFactor;

   /* print some relevant params and stats */

      sprintf(szNumChan, "%d channel", numChan);
      if (numChan > 1) strcat(szNumChan, "s");
      strcpy(tmpstr2, "");
      if (codec_test_params.codec_type != DS_VOICE_CODEC_TYPE_NONE) {
         if (encoder_handle) strcpy(tmpstr, "encoder");
         if (decoder_handle) sprintf(tmpstr2, "decoder framesize (bytes) = %d, ", coded_framesize);
      }
      else strcpy(tmpstr, "pass-thru");

      printf("  input framesize (samples) = %d, %s framesize (samples) = %d, %sinput Fs = %d (Hz), output Fs = %d (Hz), %s\n", input_framesize/AUDIO_SAMPLE_SIZE, tmpstr, inbuf_size/AUDIO_SAMPLE_SIZE, tmpstr2, sampleRate_input, sampleRate_output, szNumChan);

#if defined(_ALSA_INSTALLED_) && defined(ENABLE_USBAUDIO)

   /* for USB audio, now that we know required sampling rate(s), number of channels, bitwidth, etc. we re-open USB devices for input, output, or both with these specs */

      if (inFileType == USB_AUDIO || (outFileType & USB_AUDIO)) {

         if (inFileType == USB_AUDIO) {

            period_size_USBAudio = input_framesize/AUDIO_SAMPLE_SIZE;  /* period size based on input sample rate, 20 msec frame */
         }
         else {

            period_size_USBAudio = input_framesize*upFactor/downFactor/AUDIO_SAMPLE_SIZE;  /* period size based on output sample rate, 20 msec frame */
         }

         buffer_size_USBAudio = period_size_USBAudio * bytesPerSample_device * 2;  /* period_size_USBAudio is in samples, so multiply by number of channels, bytes per sample, and by 2 for double buffering */  /* note -- removed numChan_device, as period size is in frame buffers, which already accounts for number of device channels.  JHB Jun2018 */

         if (outFileType & USB_AUDIO) {

            if (codec_test_params.codec_type != DS_VOICE_CODEC_TYPE_NONE) {  /* in this case Fs conversion may be needed twice, once prior to codec processing, and once after, JHB Oct 2018 */

               period_size_USBAudio_output = output_framesize*upFactor_output/downFactor_output/AUDIO_SAMPLE_SIZE;
               buffer_size_USBAudio_output = period_size_USBAudio_output * bytesPerSample_device * 2;
            }
            else {

               period_size_USBAudio_output = period_size_USBAudio;
               buffer_size_USBAudio_output = buffer_size_USBAudio;
            }
         }

         printf("  USB audio input framesize = %lu, input buffer size = %lu, output framesize = %lu, output buffer size = %lu, output Fs = %d\n", period_size_USBAudio, buffer_size_USBAudio, period_size_USBAudio_output, buffer_size_USBAudio_output, sampleRate_USBAudio);

         if (inFileType == USB_AUDIO) {

            usb_device_capture = DSOpenAvioDevice(hw_params, DS_SND_PCM_STREAM_CAPTURE, buffer_size_USBAudio, period_size_USBAudio, &pcm_callback_capture, USBAudioCallbackCapture, hwDevice, sampleRate_input);

            if (!usb_device_capture) {

               printf("Unable to re-open %s audio capture port\n", szPortName);
               goto codec_test_cleanup;
            }
         }

         if (outFileType & USB_AUDIO) {

            usb_device_playback = DSOpenAvioDevice(hw_params, DS_SND_PCM_STREAM_PLAYBACK, buffer_size_USBAudio_output, period_size_USBAudio_output, &pcm_callback_playback, USBAudioCallbackPlayback, hwDevice, sampleRate_USBAudio);

            if (!usb_device_playback) {

               printf("Unable to re-open %s audio playback port\n", szPortName);
               goto codec_test_cleanup;
            }

            memset(buf32_out, 0, sizeof(buf32_out));  /* clear all of the ALSA device output buffer, in case we're doing single channel output */
         }
      }
#endif

   /* adjust encoded input file offset, if needed */

      if (inFileType == ENCODED) {

         if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_NB)
            fseek(fp_in, 6, SEEK_SET);  /* for input COD file, skip AMR MIME header (only used for file i/o operations with decoder) */

         if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_WB)
            fseek(fp_in, 9, SEEK_SET);  /* for input COD file, skip AMR MIME header (only used for file i/o operations with decoder) */

         if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_EVS) {
 
            fseek(fp_in, 16, SEEK_SET);  /* for input COD file, skip EVS MIME header (only used for file i/o operations with decoder) */
         }
      }

      if (inFileType != ENCODED && (nSegmentation & DS_SEGMENT_AUDIO)) {

      /* initialize segment audio output file:

           -for waveform file input, use same header info, add suffix filename
           -for USB audio input use output filename as a base to form segment filename
           -sample rate must be updated if sampling rate conversion occurs before encoding and/or segment detection
      */

         if (inFileType != USB_AUDIO) {

            memcpy(&MediaInfoSegment, &MediaInfo, sizeof(MEDIAINFO));  /* MediaInfo struct still contains values from opening input waveform file, above */

            strcpy(tmpstr, MediaInfoSegment.szFilename);
         }
         else {

            MediaInfoSegment.Fs = sampleRate_input;
            MediaInfoSegment.NumChan = numChan; 
            MediaInfoSegment.SampleWidth = AUDIO_SAMPLE_SIZE*CHAR_BIT;
            MediaInfoSegment.CompressionCode = DS_GWH_CC_PCM;

            if (AUDIO_FILE_TYPES(outFileType2)) strcpy(tmpstr, MediaParams[1].Media.outputFilename);
            else strcpy(tmpstr, MediaParams[0].Media.outputFilename);
         }

         p = strrchr(tmpstr, '.');  if (p) *p = 0;
         sprintf(MediaInfoSegment.szFilename, "%s_seg0.wav", tmpstr);

         ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_segment, MediaInfoSegment.szFilename, (uintptr_t)NULL, 0, DS_CREATE, &MediaInfoSegment);

         if (fp_out_segment) printf("Opened output audio segment file %s\n", MediaInfoSegment.szFilename);
         else {
            printf("Failed to open output audio segment file %s, ret_val = %d\n", MediaInfoSegment.szFilename, ret_val);
            goto codec_test_cleanup;
         }

         if (nSegmentation & DS_SEGMENT_OUTPUT_CONCATENATE) {  /* output concatenated audio segment file */

            memcpy(&MediaInfoConcat, &MediaInfo, sizeof(MEDIAINFO));  /* MediaInfo struct still contains values from opening input waveform file, above */
            strcpy(tmpstr, MediaInfo.szFilename);
            p = strrchr(tmpstr, '.');  if (p) *p = 0;
            sprintf(MediaInfoConcat.szFilename, "%s_concat.wav", tmpstr);

            ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_concat, MediaInfoConcat.szFilename, (uintptr_t)NULL, 0, DS_CREATE, &MediaInfoConcat);

            if (fp_out_concat) printf("Opened output concatenated audio segment file %s\n", MediaInfoConcat.szFilename);
            else {
               printf("Failed to open output concatenated audio segment file %s, ret_val = %d\n", MediaInfoConcat.szFilename, ret_val);
               goto codec_test_cleanup;
            }
         }

         if (nSegmentation & DS_SEGMENT_OUTPUT_STRIPPED) {  /* output stripped audio content file */

            memcpy(&MediaInfoStripped, &MediaInfo, sizeof(MEDIAINFO));  /* MediaInfo struct still contains values from opening input waveform file, above */
            strcpy(tmpstr, MediaInfo.szFilename);
            p = strrchr(tmpstr, '.');  if (p) *p = 0;
            sprintf(MediaInfoStripped.szFilename, "%s_stripped.wav", tmpstr);

            ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_stripped, MediaInfoStripped.szFilename, (uintptr_t)NULL, 0, DS_CREATE, &MediaInfoStripped);

            if (fp_out_stripped) printf("Opened output stripped audio content file %s\n", MediaInfoStripped.szFilename);
            else {
               printf("Failed to open output stripped audio content file %s, ret_val = %d\n", MediaInfoStripped.szFilename, ret_val);
               goto codec_test_cleanup;
            }
         }
      }


   /* set output params as specified by (i) input file, (ii) codec config file, or (iii) USB audio test modes */

      MediaInfo.Fs = (float)sampleRate_output;
      MediaInfo.NumChan = numChan;
      MediaInfo.SampleWidth = AUDIO_SAMPLE_SIZE*CHAR_BIT;
      MediaInfo.CompressionCode = DS_GWH_CC_PCM;  /* default is 16-bit PCM.  G711 uLaw and ALaw are also options */

      if (outFileType == ENCODED) {

         if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_EVS) MediaInfo.CompressionCode = DS_GWH_CC_EVS;
         else if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_MELPE)  MediaInfo.CompressionCode = DS_GWH_CC_MELPE;
         else if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_NB) MediaInfo.CompressionCode = DS_GWH_CC_GSM_AMR;
         else if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_WB) MediaInfo.CompressionCode = DS_GWH_CC_GSM_AMRWB;
      }


   /* open output file.  If output is .wav, DSSaveDataFile() uses MediaInfo[] elements to set the wav header */

      if (outFileType != USB_AUDIO) {

         if (AUDIO_FILE_TYPES(outFileType2)) strcpy(MediaInfo.szFilename, MediaParams[1].Media.outputFilename);
         else strcpy(MediaInfo.szFilename, MediaParams[0].Media.outputFilename);

         ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, MediaInfo.szFilename, (uintptr_t)NULL, 0, DS_CREATE, &MediaInfo);  /* DSSaveDataFile returns bytes written, with DS_CREATE flag it returns header length (if any, depending on file type) */

         if (fp_out) printf("Opened output audio file %s\n", MediaInfo.szFilename);
         else {
            printf("Failed to open output audio file %s, ret_val = %d\n", MediaInfo.szFilename, ret_val);
            goto codec_test_cleanup;
         }
      }

   /* get ready to run the test */

      gettimeofday(&tv, NULL);
      t1 = (uint64_t)tv.tv_sec*1000000L + (uint64_t)tv.tv_usec;

      if (encoder_handle && decoder_handle) printf("Running encoder-decoder test\n");
      else if (encoder_handle) printf("Running encoder test\n");
      else if (decoder_handle) printf("Running decoder test\n");
      else printf("Running pass-thru test\n");

      while (run) {

         key = toupper(getkey());
         if (key == 'Q')
         {
            run = 0;
            break;
         }

         uStripFrame = 0;

         if (inFileType != ENCODED) {

#if defined(_ALSA_INSTALLED_) && defined(ENABLE_USBAUDIO)

            if (inFileType == USB_AUDIO || (outFileType & USB_AUDIO)) {

            /* wait for ALSA callback function.  Polling for this may need to be in a separate / background process, JHB Mar 2018 */

PollBuffer:
               usb_audio_callback = 0;

               while (!usb_audio_callback) {

                  gettimeofday(&tv, NULL);  /* time out if no new audio buffer after timeout value (in msec) */
                  t2_usb = (uint64_t)tv.tv_sec*1000000L + (uint64_t)tv.tv_usec;

                  if (t1_usb == 0) t1_usb = t2_usb;
                  else if ((t2_usb-t1_usb) > 1000*USB_AUDIO_BUFFER_TIMEOUT) break;
               }

               if (!usb_audio_callback) {

                  printf("ALSA audio buffer time-out after %d msec\n", USB_AUDIO_BUFFER_TIMEOUT);
                  goto codec_test_cleanup;
               }

               t1_usb = t2_usb;

               if (!fFirstUSBAudioBuffer) {  /* we discard first buffer in case there are any stale samples left over in ALSA lower layers.  With the 2i2 unit, sometimes first 30 or so samples are either artifacts or left over from previous run. JHB Apr 2018 */

                  fFirstUSBAudioBuffer = true;
                  goto PollBuffer;
               }

               if (inFileType == USB_AUDIO) memcpy(in_buf, buf16_in, period_size_USBAudio*AUDIO_SAMPLE_SIZE*numChan);
               if (outFileType & USB_AUDIO) memcpy(buf16_out, out_buf, period_size_USBAudio_output*AUDIO_SAMPLE_SIZE*numChan);  /* note there will be a one loop iteration delay as out_buf is calculated below */
            }
#endif

            if (inFileType != USB_AUDIO) {

               if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_WB_PLUS)  /* AMR-WB+ encoder inputs a slightly variable amount of data for each frame (this averages out over a few frames to an 80 msec superframe, with 4x 20 msec subframes), CKJ Dec 2018 */
               {
                  if (frame_count == 0) {  /* for first AMR-WB+ frame, calculate input_framesize based on codec framesize + sample rate.  After that, use value returned in encOutArgs.size */

                     if (codec_test_params.mode > 15) input_framesize = 2*codec_test_params.sample_rate*0.08*AUDIO_SAMPLE_SIZE;
                     else input_framesize = codec_test_params.sample_rate*0.08*AUDIO_SAMPLE_SIZE;
                  }
                  else input_framesize = encOutArgs.size*AUDIO_SAMPLE_SIZE; /* use encOutArgs.size from the previous encode call to know ho much data is needed from the file for next encode call */
                  //printf("input_Framesize = %d, numChan = %d\n", input_framesize/AUDIO_SAMPLE_SIZE, numChan);
               }

               #define FILL_LAST_FRAME  /* if last frame is partial, zerofill, CKJ Dec 2018 */
               #ifdef FILL_LAST_FRAME
               ret_val = DSLoadDataFile(DS_GM_HOST_MEM, &fp_in, NULL, (uintptr_t)in_buf, input_framesize*numChan, DS_READ, NULL);
               if (ret_val > 0)
               {
                  int i;
                  for (i = ret_val; i < input_framesize*numChan; i++) in_buf[i] = 0;
               }
               else {
                  segmenter(SEGMENTER_CLEANUP, frame_count, codec_frame_duration, uStripFrame, 0, 0, &fp_out_segment, &MediaInfoSegment, &fp_out_concat, &MediaInfoConcat, &fp_out_stripped, &MediaInfoStripped);  /* clean up segmentation, if active */
                  break;  /* exit while loop */
               }
               #else
               if ((ret_val = DSLoadDataFile(DS_GM_HOST_MEM, &fp_in, NULL, (uintptr_t)in_buf, input_framesize*numChan, DS_READ, NULL)) != input_framesize*numChan) break;
               #endif
            }

         /* we have valid input data with no errors; update frame count and process the frame */

            frame_count++;
            printf("\rProcessing frame %d...", frame_count);
            fflush(stdout);
            fFramePrint = true;

         /* perform sample rate conversion if needed (DSConvertFs() is in alglib).  Notes:

              1) Sampling rate of output data is input rate * upFactor / downFactor

              2) Data is processed in-place, so in_buf contains both input data and decimated or interpolated output data.  For interpolation case, in_buf must point to a buffer large enough to handle the increased amount of output data
         */
  
            if (sampleRate_input != sampleRate_output) {

               int num_samples = input_framesize/numChan/AUDIO_SAMPLE_SIZE;

               for (i=0; i<numChan; i++) {

                  DSConvertFs(&((short int*)in_buf)[i],  /* pointer to data */
                              sampleRate_input,          /* sampling rate of data, in Hz */
                              upFactor,                  /* up factor */    
                              downFactor,                /* down factor */
                              fs_convert_delay_buf[i],   /* pointer to delay values (this buffer has to be preserved between calls to DSConvertFs() so it must be per channel */
                              num_samples,               /* data length, in samples */
                              numChan);                  /* number of interleaved channels in the input data */
               }
            }

            #if 1
            if ((nSegmentation & DS_STRIP_SILENCE) && isArrayLess((short int*)in_buf, input_framesize*numChan/AUDIO_SAMPLE_SIZE, nAmplitude ? nAmplitude : 64)) {  /* default amplitude threshold if none given on cmd line is 64 (units are in A/D sample values) */
            #else
            if ((nSegmentation & DS_STRIP_SILENCE) && isArrayZero(in_buf, input_framesize*numChan)) {
            #endif

               nSIDStats[0]++;
               uStripFrame = STRIP_FRAME_SILENCE;
            }

         /* call codec encoder if needed.  encOutArgs contains the number of samples needed for the next frame in encOutArgs.size (currently applies only to AMR-WB+, CKJ Dec 2018) */

            if (encoder_handle) {

               coded_framesize = DSCodecEncode(encoder_handle, 0, in_buf, coded_buf, inbuf_size, &encOutArgs);  /* voplib API */

               if (coded_framesize < 0) {
                  fprintf(stderr, "DSCodecEncode() returns error %d, exiting test \n", coded_framesize);
                  goto codec_test_cleanup;
               }

               if (coded_framesize < MAX_SID_FRAMESIZE) {  /* coded_framesize < MAX_SID_FRAMESIZE only happens if encoder has DTX / VAD enabled */

                  nSIDStats[coded_framesize]++;

                  if (nSegmentation & DS_STRIP_SILENCE) {  /* DS_STRIP_xxx flags defined in alglib.h */

                     if (coded_framesize == 1) uStripFrame |= STRIP_FRAME_DTX_CONT;
                     else uStripFrame |= STRIP_FRAME_DTX;
                  }
               }
            }

//            if (uStripFrame) frame_strip_count++;
         }
         else {  /* encoded input */

            int bitRate_code = 0, offset = 0;

            if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_NB || codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_WB) {

               if ((ret_val = fread(coded_buf, sizeof(char), 1, fp_in)) != 1) break;  /* read ToC byte from .cod file (see pcap extract mode below for notes about .cod file format) */
               bitRate_code = (coded_buf[0] >> 3) & 0xf;  /* bitrate code is a bitfield within ToC byte */
               offset = 1;
            }

            if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_EVS) {

               if ((ret_val = fread(coded_buf, sizeof(char), 1, fp_in)) != 1) break;  /* read ToC byte from .cod file (see pcap extract mode notes below about .cod file format) */
               bitRate_code = coded_buf[0] & 0xf;  /* bitrate code is a bitfield within ToC byte */
               offset = 1;
            }

            if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_MELPE) {

               bitRate_code = (codec_test_params.bitrate << 16) | codec_test_params.bitDensity;
               offset = 0;
            }

            if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_WB_PLUS) {
               int i, break_out = 0;
               offset = 0;
               for (i = 0; i < 4; i++) /* read in 4 20ms frames to pass 1 80ms super frame to the decoder */
               {
                  if ((ret_val = _fread(coded_buf + offset, sizeof(char), 2, fp_in)) != 2) {break_out = 1; break;}
                  offset += 2;
                  bitRate_code = coded_buf[0];
                  frame_size = DSGetPayloadSize(codec_test_params.codec_type, bitRate_code);
                  if (frame_size < 0)
                  {
                     printf("ERROR: Invalid frame size: %d\n", frame_size);
                     break;
                  }
                  if ((ret_val = _fread(coded_buf + offset, sizeof(char), frame_size, fp_in)) != frame_size) {break_out = 1; break;}
                  offset += frame_size;
                  if (!((bitRate_code >= 10 && bitRate_code <= 13) || bitRate_code > 15)) break; /* if not extension mode, only read 1 20 ms frame */
               }
               if (break_out) break;
            }

            if (!(codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_WB_PLUS)) frame_size = DSGetPayloadSize(codec_test_params.codec_type, bitRate_code); /* get payload size */

            if (codec_test_params.uncompress && codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_G729AB) {

               if ((ret_val = fread(coded_buf, sizeof(short), 2, fp_in)) != 2) break;  /* read frame start and frame size bytes from .cod file */
               frame_size = ((short int*)coded_buf)[1] * sizeof(short);
               offset = 4;
            }

            if (codec_test_params.uncompress && codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_G726) {

               frame_size = codec_frame_duration*8*sizeof(short);/* use uncompressed frame size in codec test mode for bit comaprison with reference vectors/program */
               coded_framesize = frame_size;
               offset = 0;
            }

#ifdef _MELPE_INSTALLED_
            if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_MELPE) {

               if (codec_test_params.bitrate == 2400) frame_size = melpe_decoder_56bd_pattern[melpe_decoder_pattern_index];  /* next amount of data expected by the decoder (in bytes) */
               else frame_size = melpe_decoder_88bd_pattern[melpe_decoder_pattern_index];
               coded_framesize = frame_size;
               offset = 0;
            }
#endif

            if ((codec_test_params.codec_type != DS_VOICE_CODEC_TYPE_AMR_WB_PLUS) && (frame_size < 0 || (ret_val = _fread(coded_buf + offset, sizeof(char), frame_size, fp_in)) != frame_size)) break;  /* no print message here, input is consumed and test finishes */
            
            if (codec_test_params.uncompress && codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_G729AB) {
               frame_size += 4; /* add frame header to frame_size */
               coded_framesize = frame_size;
            }
         }

         if (!fFramePrint) {

            frame_count++;
            printf("\rProcessing frame %d...", frame_count);
            fflush(stdout);
         }

         if (outFileType != ENCODED)
         {

         /* call codec decoder if needed */

            if (decoder_handle) {

#ifdef _MELPE_INSTALLED_
               if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_MELPE && ((codec_test_params.bitDensity == 56) || (codec_test_params.bitDensity == 88))) {  /* special case for MELPe full path with packed bit densities.  MELPe supports packed bit densities that require fractional bytes split across frames */

                  static unsigned int sav_bytes_in = 0, sav_bytes_out = 0;
                  unsigned int num_bytes;

               /* for packed bit densities, the MELPe decoder requires a specific, repeating pattern of bytes to sustain an average bits per frame (54 bits for 2400 bps, 81 bits for 1200 bps) , so we store encoder output and feed to decoder only when we have enough data, JHB May2018 */

//                  if (frame_count <= 10) printf("frame %d coded_framesize = %d\n", frame_count, coded_framesize);

                  memcpy(coded_buf_sav + sav_bytes_in, coded_buf, coded_framesize);
                  sav_bytes_in += coded_framesize;

                  if (codec_test_params.bitrate == 2400) num_bytes = melpe_decoder_56bd_pattern[melpe_decoder_pattern_index];  /* next amount of data expected by the decoder (in bytes) */
                  else num_bytes = melpe_decoder_88bd_pattern[melpe_decoder_pattern_index];
 
                  if ((sav_bytes_in - sav_bytes_out) < num_bytes) {  /* we don't have enough data to decode, wait until next encoder output */

                     // printf("MELPe not enough data, frame = %d, num_bytes = %d\n", frame_count, num_bytes);
                     continue;
                  }
                  else {  /* we have enough data, copy from the save buffer and decode */

//                     printf("MELPe enough data, frame = %d, in - out = %d\n", frame_count, sav_bytes_in - sav_bytes_out);

                     if (codec_test_params.bitrate == 2400) melpe_decoder_pattern_index = (melpe_decoder_pattern_index + 1) & 3;
                     else melpe_decoder_pattern_index = (melpe_decoder_pattern_index + 1) & 7;

                     memcpy(coded_buf, coded_buf_sav + sav_bytes_out, num_bytes);
                     coded_framesize = num_bytes;
                     sav_bytes_out += num_bytes;

                     if (sav_bytes_in > MAX_CODED_FRAME/2) {  /* reset indexes to avoid overflowing the save buffer */

                        memcpy(coded_buf_sav, coded_buf_sav + sav_bytes_out, sav_bytes_in - sav_bytes_out);
                        sav_bytes_in -= sav_bytes_out;
                        sav_bytes_out = 0;
//                        printf("reset indexes, frame = %d\n", frame_count);
                     }
                  }
               }
#endif

               if (coded_framesize > 0 && !uStripFrame) {

                  len = DSCodecDecode(decoder_handle, 0, coded_buf, out_buf, coded_framesize, NULL);  /* voplib API */

                  if (len < 0) {
                     fprintf(stderr, "DSCodecDecode() returns error %d, exiting test \n", len);
                     goto codec_test_cleanup;
                  }
               }
               else len = 0;
            }
            else {  /* pass-thru (codec_type == NONE) */

               len = inbuf_size*numChan;
               memcpy(out_buf, in_buf, len);
            }

            addr = (uintptr_t)out_buf;
         }
         else {

            len = coded_framesize;
            addr = (uintptr_t)coded_buf;
         }

         if (outFileType != USB_AUDIO) {

            ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, NULL, addr, len, DS_WRITE, &MediaInfo);   /* DSSaveDataFile() returns bytes written */

            if (ret_val != len) 
            {
               printf("Error writing output wav file frame %d: tried to write %d bytes, wrote %d bytes\n", frame_count, len, ret_val);
               goto codec_test_cleanup;
            }

         /* write out audio file segments, if specified in cmd line.  Use fixed or adjusted segment intervals, as specified by flags */

            if (nSegmentation & DS_SEGMENT_AUDIO) {
 
               if (segmenter(SEGMENTER_FRAME, frame_count, codec_frame_duration, uStripFrame, in_buf, inbuf_size*numChan, &fp_out_segment, &MediaInfoSegment, &fp_out_concat, &MediaInfoConcat, &fp_out_stripped, &MediaInfoStripped) < 0) goto codec_test_cleanup;
            }
         }

         if (outFileType & USB_AUDIO) {

            if (codec_test_params.codec_type != DS_VOICE_CODEC_TYPE_NONE && upFactor_output != downFactor_output) {

               int num_samples = len/numChan/AUDIO_SAMPLE_SIZE;

               for (i=0; i<numChan; i++) {

                  DSConvertFs(&((short int*)addr)[i],          /* pointer to data */
                              sampleRate_output,               /* sampling rate of data, in Hz */
                              upFactor_output,                 /* up factor */    
                              downFactor_output,               /* down factor */
                              fs_convert_delay_buf_output[i],  /* pointer to delay values (this buffer has to be preserved between calls to DSConvertFs() so it must be per channel */
                              num_samples,                     /* data length, in samples */
                              numChan);                        /* number of interleaved channels in the input data */
               }
            }
         }

      }  /* while loop */

      printf("\n");  /* leave existing status line, including any error messages (don't clear it) */

      if (!run) printf("Exiting test\n");

      gettimeofday(&tv, NULL);
      t2 = (uint64_t)tv.tv_sec*1000000L + (uint64_t)tv.tv_usec;

      printf("Run-time: %3.6fs\n", 1.0*(t2-t1)/1e6);

   /* print SID stats if encoder (i) is active and (ii) supports DTX */

      if (CodecParams.enc_params.dtx.dtx_enable) { 

         for (i=MAX_SID_FRAMESIZE-1; i >= 0; i--) {

            if (nSIDStats[i]) {

               if (!fPrintSIDStats) { printf("DTX stats: "); fPrintSIDStats = true; }
               else printf(", ");
               printf("frmsiz %d = %d", i, nSIDStats[i]);
            }
         }

         if (fPrintSIDStats) printf("\n");
      }

   /* print segmentation stats */

      if (nSegmentation & DS_SEGMENT_AUDIO) { 

         segmenter(SEGMENTER_PRINT_STATS, frame_count, codec_frame_duration, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL);
      }

   /* check if loop exit condition was an error */

      if (!USBAudioInput && run && !feof(fp_in))
      {
         printf("Error -- did not reach input file EOF, last fread() read %d bytes\n", ret_val);
      }

codec_test_cleanup:

      /* codec tear down / cleanup */

      if (encoder_handle) DSCodecDelete(encoder_handle);
      if (decoder_handle) DSCodecDelete(decoder_handle);

      if (fp_in) {
         if (inFileType != ENCODED) DSLoadDataFile(DS_GM_HOST_MEM, &fp_in, NULL, (uintptr_t)NULL, 0, DS_CLOSE, NULL);
         else fclose(fp_in);
      }

      if (fp_out) DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, NULL, (uintptr_t)NULL, 0, DS_CLOSE, &MediaInfo);
      if (fp_out_segment) DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_segment, NULL, (uintptr_t)NULL, 0, DS_CLOSE, &MediaInfoSegment);
      if (fp_out_concat) DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_concat, NULL, (uintptr_t)NULL, 0, DS_CLOSE, &MediaInfoConcat);
      if (fp_out_stripped) DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_stripped, NULL, (uintptr_t)NULL, 0, DS_CLOSE, &MediaInfoStripped);

#if defined(_ALSA_INSTALLED_) && defined(ENABLE_USBAUDIO)
      if (usb_device_capture) DSCloseAvioDevice(usb_device_capture, pcm_callback_capture);
      if (usb_device_playback) DSCloseAvioDevice(usb_device_playback, pcm_callback_playback);
#endif

      if (hPlatform != -1) DSFreePlatform((intptr_t)hPlatform);  /* free platform handle */

      printf("x86 codec test end\n");
   }
   else if (x86_pkt_test || frame_mode)
   {
      int num_threads = PlatformParams.cimInfo[0].taskAssignmentCoreLists[0] | (PlatformParams.cimInfo[0].taskAssignmentCoreLists[1] << 8) | (PlatformParams.cimInfo[0].taskAssignmentCoreLists[2] << 16) | (PlatformParams.cimInfo[0].taskAssignmentCoreLists[3] << 24);  /* this is the -tN cmd line value, if entered.  -1 means there was no entry, JHB Sep 2018 */

      if (num_threads == -1) {

         PlatformParams.cimInfo[0].taskAssignmentCoreLists[0] = 0;
         PlatformParams.cimInfo[0].taskAssignmentCoreLists[1] = 0;
         PlatformParams.cimInfo[0].taskAssignmentCoreLists[2] = 0;
         PlatformParams.cimInfo[0].taskAssignmentCoreLists[3] = 0;
      }

      switch (executionMode[0]) {

         case 'a':  /* app execution */

            uint32_t arg;
            *((char*)&arg) = (char)executionMode[0];

            packet_flow_media_proc((void*)&arg);  /* packet data flow and media processing.  packet_flow_media_proc.c */

            break;

         case 'p':  /* process execution, not used yet */
            break;

         case 't':  /* thread execution, DSPush/PullPackets APIs used to interface with packet_flow_media_proc() running as a thread */

            if (num_threads <= 0) {

               int thread_index = 0;

               mediaMin_thread((void*)&thread_index);  /* mediaMin_thread() (in mediaMin.c) is an application that starts one or more packet/media threads and uses packet push/pull queues. We call it here as a function */
            }
            else {  /* in this case we start mediaMin_thread() as one or more application level threads */

               uint32_t* arg;
               int i, tc_ret, num_threads_started = 0;
               pthread_attr_t* ptr_attr = NULL;
               pthread_t mediaMinThreads[MAX_MEDIAMIN_THREADS];  /* MAX_MEDIAMIN_THREADS is defined in mediaTest.h */

               for (i=0; i<num_threads; i++) {
      
                  arg = (uint32_t*)malloc(sizeof(uint32_t));

                  if (arg == NULL) 
                  {
                     fprintf(stderr, "%s:%d: Could not allocate memory for mediaMin thread arg\n", __FILE__, __LINE__);
                     exit(-1);
                  }

                  *arg = (num_threads << 8) | i;  /* tell the mediaMin app thread which one it is and how many total threads */

                  if ((tc_ret = pthread_create(&mediaMinThreads[i], ptr_attr, mediaMin_thread, arg))) fprintf(stderr, "%s:%d: pthread_create() failed for mediaMin thread, thread number = %d, ret val = %d\n", __FILE__, __LINE__, i, tc_ret);
                  else {

                     cpu_set_t cpuset;
                     int j;
                     CPU_ZERO(&cpuset);

                     if (nReuseInputs) for (j=10; j<16; j++) {
                        CPU_SET(j, &cpuset);
                        CPU_SET(j+16, &cpuset);
                     }
                     else for (j=6; j<32; j++) if (j < 16 || j > 18) CPU_SET(j, &cpuset);  /* avoid first 6 physical cores (and their logical core companions) */

                     tc_ret = pthread_setaffinity_np(mediaMinThreads[i], sizeof(cpu_set_t), &cpuset);

                     num_threads_started++;
                  }
               }

               if (num_threads_started) {  /* wait here for all threads to exit */

                  for (i=0; i<num_threads_started; i++) pthread_join(mediaMinThreads[i], NULL);
               }
            }

            break;
      }
   }
   else if (x86_frame_test)
   {
      HCODEC hCodec[MAX_CODEC_INSTANCES];
      FRAME_TEST_INFO ft_info = {{0}};
      int nCodecs = 0;

      char default_config_file[] = "session_config/frame_test_config";
      char *config_file;
      FILE *fp_cfg = NULL;

      /* need a separate thread for encoder and decoder */
      pthread_t process_threads[2*MAX_CODEC_INSTANCES];
      int i, ret_val, nThreads = 0;
      char threads_finished = 0;
      int total_encoded_frame_cnt, total_decoded_frame_cnt;

      printf("x86 frame test start\n");

      if (strlen(MediaParams[0].configFilename) == 0 || (access(MediaParams[0].configFilename, F_OK ) == -1)) 
      {
         printf("Specified config file: %s does not exist, using default file.\n", MediaParams[0].configFilename);
         config_file = default_config_file;
      }
      else config_file = MediaParams[0].configFilename;

      printf("Opening session config file: %s\n", config_file);

      fp_cfg = fopen(config_file, "r");

      while (parse_codec_params(fp_cfg, &ft_info) != -1) 
      {
         if ((hCodec[nCodecs] = DSCodecCreate(&ft_info.term, DS_CC_CREATE_ENCODER | DS_CC_CREATE_DECODER | DS_CC_USE_TERMINFO)) < 0)
         {
            fprintf(stderr, "%s:%d: Failed to create codec\n", __FILE__, __LINE__);
            continue;
         }

         encoder_input_files[nCodecs] = ft_info.encoder_file;
         decoder_input_files[nCodecs] = ft_info.decoder_file;

         memset(&ft_info, 0, sizeof(ft_info));
         nCodecs++;
      }

      if (nCodecs <= 0)
      {
         fprintf(stderr, "Failed to create any coders, exiting test\n");
         return;
      }

      for (i = 0; i < nCodecs; i++)
      {
         if (encoder_input_files[i] != NULL)
         {
            if ((ret_val = pthread_create(&process_threads[2*i], NULL, encode_thread_task, &hCodec[i]))) 
            {
               fprintf(stderr, "%s:%d: pthread_create() failed for codec number %d, returned %d\n", __FILE__, __LINE__, i, ret_val);
               return;
            }
            else
            {
               nThreads++;
               thread_status[2*i] = 1;
            }
         }

         if (decoder_input_files[i] != NULL)
         {
            if ((ret_val = pthread_create(&process_threads[2*i+1], NULL, decode_thread_task, &hCodec[i]))) 
            {
               fprintf(stderr, "%s:%d: pthread_create() failed for codec number %d, returned %d\n", __FILE__, __LINE__, i, ret_val);
               return;
            }
            else
            {
               nThreads++;
               thread_status[2*i+1] = 1;
            }
         }
      }

      printf("Waiting for %d processing threads to complete...\n", nThreads);
      while (run && !threads_finished)
      {
         threads_finished = 1;
         for (i = 0; i < 2*nCodecs; i++) 
         {
            if(thread_status[i] == 1)
            {
               threads_finished = 0;
               break;
            }
         }

         total_encoded_frame_cnt = array_sum(encoded_frame_cnt, nCodecs);
         total_decoded_frame_cnt = array_sum(decoded_frame_cnt, nCodecs);
         
         printf("\rEncoded %d frames, Decoded %d frames", total_encoded_frame_cnt, total_decoded_frame_cnt);
      }

      total_encoded_frame_cnt = array_sum(encoded_frame_cnt, nCodecs);
      total_decoded_frame_cnt = array_sum(decoded_frame_cnt, nCodecs);

      printf("\rEncoded %d frames, Decoded %d frames\n", total_encoded_frame_cnt, total_decoded_frame_cnt);

      for (i = 0; i < nCodecs; i++)
      {
         if (thread_status[2*i])
         {
            if ((ret_val = pthread_join(process_threads[2*i], NULL))) 
            {
               fprintf(stderr, "%s:%d: pthread_join() failed for codec number %d, returned %d\n", __FILE__, __LINE__, i, ret_val);
            }
         }
         
         if (thread_status[2*i+1])
         {
            if ((ret_val = pthread_join(process_threads[2*i+1], NULL))) 
            {
               fprintf(stderr, "%s:%d: pthread_join() failed for codec number %d, returned %d\n", __FILE__, __LINE__, i, ret_val);
            }
         }
      }

      /* Cleanup */
      for (i = 0; i < nCodecs; i++)
      {
         DSCodecDelete(hCodec[i]);

         if (encoder_input_files[i] != NULL) free(encoder_input_files[i]);
         if (decoder_input_files[i] != NULL) free(decoder_input_files[i]);
      }

      fclose(fp_cfg);
  
      printf("x86 frame test end\n");
   }
   else if (pcap_extract)
   {

   /* The pcap extract mode extracts RTP payloads from pcap files and writes to 3GPP decoder compatible .cod files.  Notes:
   
      1) The 3GPP decoder supports MIME and G.192 file formats.  In the case of MIME it expects consecutive RTP payloads in FH (Full Header) format, each including a leading ToC byte

      2) Currently the pcap extract mode only supports MIME format

      3) If pcap RTP payloads are in CH (Compact Header) format, they are converted to FH format (ToC byte added)
   */

      MEDIAINFO MediaInfo = {0};
      
      FILE *fp_in = NULL, *fp_out = NULL;
      char tmpstr[1024];

      int ret_val, frame_count = 0;

      uint8_t pkt_buffer[MAX_RTP_PACKET_LEN];
      char toc;

      int packet_length, link_layer_length, pyld_len;
      uint8_t *pyld_ptr;
      unsigned int uFlags;

   /* define STRIP_SID to remove SID frames from .cod output */
      // #define STRIP_SID
      #ifdef STRIP_SID
      int pyld_datatype, SID_drop_count = 0;
      #endif

   /* define LIST_TOCS to list unique ToC values found (displayed after the extract finishes).  ToC values are "table of contents" bytes in the payload header */
      #define LIST_TOCS
      #ifdef LIST_TOCS
      int i, num_tocs = 0, sav_tocs[256] = { 0 };
      #endif

      printf("pcap extract start\n");

      if (MediaParams[0].Media.inputFilename != NULL)
      {
         strcpy(tmpstr, MediaParams[0].Media.inputFilename);
         strupr(tmpstr);

         if (strstr(tmpstr, ".PCAP"))
         {
            if ((link_layer_length = DSOpenPcap(MediaParams[0].Media.inputFilename, &fp_in, NULL, "", DS_READ | DS_OPEN_PCAP_READ_HEADER)) < 0) goto pcap_extract_cleanup;
         }
         else
         {
            fprintf(stderr, "Input file %s is not a pcap file\n", MediaParams[0].Media.inputFilename);
            goto pcap_extract_cleanup;
         }
      }
      else
      {
         fprintf(stderr, "No input file given\n");
         goto pcap_extract_cleanup;
      }

      if (MediaParams[0].Media.outputFilename != NULL)
      {
         strcpy(tmpstr, MediaParams[0].Media.outputFilename);
         strupr(tmpstr);

         if (strstr(tmpstr, ".COD") || strstr(tmpstr, ".AMR") || strstr(tmpstr, ".AWB") || strstr(tmpstr, ".BIT"))
         {
            strcpy(MediaInfo.szFilename, MediaParams[0].Media.outputFilename);

            MediaInfo.CompressionCode = DS_GWH_CC_EVS;  /* default */
            int codec_type = 0;  /* not sure yet how to determine codec type -- payload contents can't be used without a session config file, so maybe something on the command line */

            if (strstr(tmpstr, ".AWB") || codec_type == DS_VOICE_CODEC_TYPE_AMR_WB) MediaInfo.CompressionCode = DS_GWH_CC_GSM_AMRWB;
            else if (strstr(tmpstr, ".AMR") || codec_type == DS_VOICE_CODEC_TYPE_AMR_NB) MediaInfo.CompressionCode = DS_GWH_CC_GSM_AMR;
            else if (codec_type == DS_VOICE_CODEC_TYPE_EVS) MediaInfo.CompressionCode = DS_GWH_CC_EVS;  /* EVS uses .cod extension */
            else if (codec_type == DS_VOICE_CODEC_TYPE_MELPE)  MediaInfo.CompressionCode = DS_GWH_CC_MELPE;  /* no file extension defined for MELPe */

            ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, MediaParams[0].Media.outputFilename, (uintptr_t)NULL, 0, DS_CREATE, &MediaInfo);  /* DSSaveDataFile returns bytes written, with DS_CREATE flag it returns header length (if any, depending on file type) */

            if (fp_out == NULL) 
            {
               fprintf(stderr, "Failed to open coded output file: %s, ret_val = %d\n", MediaParams[0].Media.outputFilename, ret_val);
               goto pcap_extract_cleanup;
            }
            else
               printf("Opened coded output file: %s\n", MediaParams[0].Media.outputFilename);
         }
         else
         {
            fprintf(stderr, "Output file %s is not a cod file\n", MediaParams[0].Media.outputFilename);
            goto pcap_extract_cleanup;
         }
      }
      else
      {
         fprintf(stderr, "No output file given\n");
         goto pcap_extract_cleanup;
      }

      uFlags = DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_NETWORK_BYTE_ORDER;  /* used with DSGetPacketInfo() */

   /* open pcap file and read its header, initialize link layer offset */

      while (run) {

      /* read next pcap packet */

         if (!(packet_length = DSReadPcapRecord(fp_in, pkt_buffer, 0, NULL, link_layer_length))) break;

         frame_count++;
         printf("\rExtracting frame %d", frame_count);

         #ifdef STRIP_SID
         pyld_datatype = DSGetPacketInfo(-1, uFlags | DS_PKT_INFO_RTP_PYLDDATATYPE, pkt_buffer, packet_length, NULL, NULL);

         if (pyld_datatype == DS_PKT_PYLDDATATYPE_SID) {

            SID_drop_count++;
            continue;  /* skip SID (DTX) frames which the 3GPP decoder does not handle when the .cod file contains MIME format data */
         }
         #endif

         pyld_ptr = pkt_buffer + DSGetPacketInfo(-1, uFlags | DS_PKT_INFO_RTP_PYLDOFS, pkt_buffer, packet_length, NULL, NULL);
         pyld_len = DSGetPacketInfo(-1, uFlags | DS_PKT_INFO_RTP_PYLDLEN, pkt_buffer, packet_length, NULL, NULL);

      /* determine header format */

         if (!DSGetPayloadHeaderFormat(DS_VOICE_CODEC_TYPE_EVS, pyld_len))  /* compact header format */
         {
            toc = DSGetPayloadHeaderToC(DS_VOICE_CODEC_TYPE_EVS, pyld_len);  /* add ToC byte based on payload size (convert to FH format) */

            ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, NULL, (uintptr_t)&toc, 1, DS_WRITE, &MediaInfo);  /* write ToC byte (DSSaveDataFile returns bytes written) */

            if (ret_val != 1) 
            {
               printf("Error writing ToC byte for frame %d, wrote %d bytes\n", frame_count, ret_val);
               goto pcap_extract_cleanup;
            }
         }
         else {  /* full header format */

            toc =  pyld_ptr[0];  /* save toc value */

            if (pyld_ptr[0] & 0x80)  /* check for CMR byte */
            {
               pyld_ptr++;
               pyld_len--;
            }
         }

         #ifdef LIST_TOCS
         bool found = false;
         for (i=0; i<max(1, num_tocs); i++) {

            if (toc == sav_tocs[i]) {
               found = true;
               break;
            }
         }
         if (!found) sav_tocs[num_tocs++] = toc;  /* if not found then add this toc value to saved list */
         #endif

         ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, NULL, (uintptr_t)pyld_ptr, pyld_len, DS_WRITE, &MediaInfo);

         if (ret_val != pyld_len) 
         {
            printf("Error writing frame %d, wrote %d bytes\n", frame_count, ret_val);
            goto pcap_extract_cleanup;
         }
      }

      #ifdef STRIP_SID
      frame_count = max(0, frame_count - SID_drop_count);
      #endif
      printf("\nExtracted %d frames", frame_count);
      #ifdef STRIP_SID
      if (SID_drop_count > 0) printf(", not including %d SID frames (which are not supported in .cod file MIME format)", SID_drop_count);
      #endif
      printf("\n");

      #ifdef LIST_TOCS
      printf("Unique ToC values found: ");
      for (i=0; i<num_tocs; i++) printf("%d ", sav_tocs[i]);
      printf("\n");
      #endif

      if (!feof(fp_in))
      {
         fprintf(stderr, "Error while reading input pcap file\n");
      }

pcap_extract_cleanup:  /* added single exit point for success + most errors, JHB Jun2017 */

      if (fp_in) fclose(fp_in);
      if (fp_out) DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, NULL, (uintptr_t)NULL, 0, DS_CLOSE, &MediaInfo);

      printf("pcap extract end\n");
   }

   printf("x86 mediaTest end\n");
}


/* assign codec name string, based on codec type (see list of constants in Signalogic/shared_include/session.h */

int get_codec_name(int codec_type, char* szCodecName) {

   switch (codec_type) {

      case DS_VOICE_CODEC_TYPE_G711_ULAW:
         strcpy(szCodecName, "G711u");
         break;

      case DS_VOICE_CODEC_TYPE_G711_ALAW:
         strcpy(szCodecName, "G711a");
         break;

      case DS_VOICE_CODEC_TYPE_EVS:
         strcpy(szCodecName, "EVS");
         break;

      case DS_VOICE_CODEC_TYPE_AMR_NB:
         strcpy(szCodecName, "AMR-NB");
         break;

      case DS_VOICE_CODEC_TYPE_AMR_WB:
         strcpy(szCodecName, "AMR-WB");
         break;
         
      case DS_VOICE_CODEC_TYPE_AMR_WB_PLUS:
         strcpy(szCodecName, "AMR-WB+");
         break;

      case DS_VOICE_CODEC_TYPE_G726:
         strcpy(szCodecName, "G726");
         break;

      case DS_VOICE_CODEC_TYPE_G729AB:
         strcpy(szCodecName, "G729AB");
         break;

      case DS_VOICE_CODEC_TYPE_MELPE:
         strcpy(szCodecName, "MELPe");
         break;

      case DS_VOICE_CODEC_TYPE_NONE:
         strcpy(szCodecName, "None (pass-thru)");
         break;

      default:
         strcpy(szCodecName, "");
         return (int)false;
   }

   return (int)true;
}

const char* strrstr(const char* haystack, const char* needle) {

   unsigned needle_length = strlen(needle);
   char* p = (char*)haystack + strlen(haystack) - needle_length - 1;  /* don't compare terminating zeros */
   size_t i;

   while (p >= haystack) {

      for (i = 0; i < needle_length; ++i) if (p[i] != needle[i]) {
         p--;
         continue;
      }

      return p;
   }

   return NULL;
}
