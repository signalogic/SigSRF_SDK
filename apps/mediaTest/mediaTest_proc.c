/*
 $Header: /root/Signalogic/apps/mediaTest/mediaTest_proc.c

 Copyright (C) Signalogic Inc. 2017-2024
 
 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description
 
  Source code for mediaTest platform (see mediaTest.c for coCPU functionality)

 Purposes
 
  1) Implementation, test, and measurement for codecs and transcoding including multiple RFC compliant packet flow, advanced jitter buffer, DTX handling, DTMF event handling, multichannel packets, ptime conversion, and more.  Measurements include:
   
    -x86 server performance
    -verify bitexactness for codecs, measure audio quality.  Interoperate at encoded bitstream level with 3GPP test vectors and reference codes
    -packet loss and other packet statistics
   
  2) Support RTP streaming for network sockets and pcap I/O

  3) Provide file I/O support for .wav, .tim, raw audio, encoded bitstream (e.g. .cod), and other file formats
   
  4) Support multithreading and multiple concurrent codec instances

  5) Demonstrate signal processing and deep learning insertion points
   
  6) Provide user application source code examples, including correct transcoding data flow and API usage for Pktlib, Voplib, Diaglib, and Aviolib

  7) Provide basis for limited, demo/eval version available on Github

 Documentation

  https://www.github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediatest

  Older documentation links:
  
    after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

    before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

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
   - Dependency on libpcap/libpcap-devel has been removed
   - ctrl-c signal is handled to exit x86 test modes cleanly
  Modified May CKJ 2017, added pcap extract mode - supports EVS only
  Modified Mar 2018 JHB, added USB audio input, tested with a Focusrite 2i2 unit.  Significant mods made to (i) mediaTest_proc.c and mediaTest.c, (ii) aviolib (audio video I/O lib) which uses Alsa "asound" lib, and (iii) mediaTest Makefile (modified to link aviolib)
  Modified Mar 2018 JHB, modified codec test mode to support (i) .wav file input (using DSLoadDataFile), (ii) USB audio input input, and (iii) pass-thru (codec type = NONE), in which case no encoding/decoding is performed (e.g. save USB audio to wav file, convert raw audio to wav file)
  Modified Mar-Apr 2018 JHB, CKJ, add support for MELPe codec
  Modified May 2018 CKJ, add support for G729 and G726 codecs
  Modified May-Jun 2018 CKJ - add support for AMR and AMR WB codecs
  Modified Jun 2018 JHB, integrate USB audio output
  Modified Jul 2018 JHB, packet_flow_media_proc(executeMode) function moved to separate file to support media thread and process execution. executeMode can be app, thread, or process. See also DSConfigMediaService() and its uControl and uFlags definitions in pktlib.h
  Modified Oct 2018 JHB, add sample rate conversion for USB audio output, independent of waveform or USB audio input prior to codec processing.  Calculate separate period and buffer size parameters not dependent on those used for USB audio input.  Note that many USB audio devices, such as the Focusrite 2i2 used in Sig lab testing, support a minimum sample rate of 44.1 kHz.  For codec support, it helps a lot if the device supports 48 kHz
  Modified Dec 2018 CKJ, add support for AMR-WB+ codec, including ".BIT" coded data file extension, variable encoder input framesize.  Pass &encOutArgs to DSCodecEncode() function
  Modified Dec 2018 JHB, adjust thread/core affinity for high capacity mediaTest -Et -tN mode testing (creates N mediaMin application threads)
  Modified Jul 2019 JHB, codecs now accessed via voplib API calls and CODEC_PARAMS struct.  XDAIS interface is now visibile from voplib and alglib but not applications
  Modified Aug-Sep 2019 JHB, add segmentation and cmd line options
  Modified Mar 2020 JHB, handle name change of mediaThread_test_app.c to mediaMin.c
  Modified Sep 2020 JHB, mods for compatibility with gcc 9.3.0: include minmax.h (for min/max functions), fix various security and "indentation" warnings
  Modified Jan 2021 JHB, fix warning for sampleRate_codec used uninitialized (no idea why this suddenly popped up)
  Modified Jan 2021 JHB, include minmax.h as min() and max() macros may no longer be defined for builds that include C++ code (to allow std:min and std:max)
  Modified Apr 2021 JHB, fix AMR problem with sequence of encoding audio to .amr or .awb, then decoding .amr or .awb to audio. Notes:
                         -see comments near AMR handling of bitrate_code, also see variable fAMROctetAligned
                         -applies to both AMR-NB and AMR-WB
                         -tested with bandwidth efficient and octet aligned coded file format
                         -bug did not affect back-to-back encode/decode (i.e. audio to audio)
  Modified Apr 2021 JHB, fix issues with G726 uncompressed vs. compressed mode, retest all bitrates
  Modified Jan 2022 JHB, use cmd line -dN flag to specify DS_CODEC_CREATE_TRACK_USAGE_FLAG in DSCodecCreate()
  Modified Jan 2022 JHB, fix warning in gcc/g++ 5.3.1 for ret_val in mediaTest_proc() ("may be used uninitialized"). Later tool versions are able to recognize there is no conditional logic path that leaves it uninitialized
  Modified Feb 2022 JHB, fix issues with sampling rate conversion applied to multichannel data (i) fs_convert_delay_buf not declared correctly and (ii) num_samples used in DSConvertFs() was incorrectly divided by numChan
  Modified Feb 2022 JHB, modify DSCodecEncode() and DSCodecDecode() to pass pointer to one or more codec handles and a num channels param. This enables multichannel encoding/decoding (e.g. stereo audio files) and simplifies concurrent codec instance test and measurement (e.g. 30+ codec instances within one thread or CPU core)
  Modified Feb 2022 JHB, make responsive to -RN command line entry (repeat), with same specs as mediaMin. For example, for an encoder-decoder data flow, -R2 repeats the data flow twice, wrapping the input waveform file after each repeat
  Modified Mar 2022 JHB, move strrstr() to dsstring.h (as static line)
  Modified Mar 2022 JHB, add first pass of gpx file processing
  Modified Jun 2022 JHB, add delta change in heading to aggressive filter coefficient calculation in GPX track filtering
  Modified Aug 2022 JHB, add _NO_PKTLIB_ and _NO_MEDIAMIN_ in a few places to allow no_mediamin and no_pktlib options in mediaTest build (look for run, frame_mode, etc vars)
  Modified Sep 2022 JHB, for pcap output, make source/dest IP addr and port and payload type values compatible with "pcap_file_test_config" config file, which is referred to in several mediaTest demo command lines
  Modified Sep 2022 JHB, add support for .cod to .pcap pass-thru case (i.e. convert an encoded bitstream file into a pcap)
  Modified Oct 2022 JHB, change DSGetPayloadHeaderFormat() to DSGetPayloadInfo() in pcap_extract. See comments in voplib.h
  Modified Oct 2022 JHB, change DSGetCompressedFramesize() to DSGetCodecInfo()
  Modified Dec 2022 JHB, change references to cmd_line_debug_flags.h to cmd_line_options_flags.h
  Modified Dec 2022 JHB, replace ancient event log config with new diaglib APIs DSInitLogging() and DSCloseLogging()
  Modified Mar 2023 JHB, add pInArgs param to DSCodecEncode()
  Modified Oct 2023 JHB, add pOutArgs param to DSCodecDecode() API as an example showing how to obtain decoder detected output info. A "codecs only" build option is now available for apps that removes session related info associated with pktlib. To use this apps should define CODECS_ONLY and link with voplib_codecs_only.so
  Modified Nov 2023 JHB, implement --cut N cmd line option. First pass of this cuts N leading media frames from data flow
  Modified Nov 2023 JHB, add pInArgs param to DSCodecDecode()
  Modified Dec 2023 JHB, allow zero framesizes to decoder input and let codec decide whether to process. This can be the case with DTX ("NO_DATA" frames)
  Modified Dec 2023 JHB, improve / clarify ENCODER_USE_INARGS examples
  Modified Dec 2023 JHB, support input_sample_rate field in codec_test_params_t struct (mediaTest.h) to handle raw audio files with no header to specify input Fs
  Modified Feb 2024 JHB, additional comments in EVS section of .cod file framesize read, after fixing problem in DS_CODEC_INFO_BITRATE_CODE section of voplib API DSGetCodecInfo() that was causing decode of EVS headerfull .cod files to fail
  Modified Feb 2024 JHB, implement codec multi-thread testing using -Ec and -tN command line options. This test mode creates multiple concurrent encode and/or decode threads. Also threads can be pinned to same physical CPU core by defining PIN_TO_SAME_CORE. Cmd line example: ./mediaTest -cx86 -itest_files/T_mode.wav -otest_files/T_mode_48kHz_5900.wav -Csession_config/evs_48kHz_input_16kHz_5900bps_full_band_config -Ec -t2
  Modified Feb 2024 JHB, add DS_DATAFILE_USE_SEMAPHORE flag to all DSLoadDataFile() and DSSaveDataFile() DirectCore API calls to support multi-thread testing. As a note, an alternative method -- but intended for very high capacity, high performance apps and not used here -- is to call DSCreateFilelibThread() (in filelib.h) for each thread after creation
 Modified Apr 2024 JHB, remove DS_CP_DEBUGCONFIG flag, which is now deprecated
 Modified May 2024 JHB, call DSGetBacktrace() before starting threads, show result as " ... start sequence = ..." in console output
 Modified May 2024 JHB, rename x86_mediaTest() function to mediaTest_proc()
 Modified Jun 2024 JHB, rename DSReadPcapRecord() to DSReadPcap() and DSWritePcapRecord() to DSWritePcap(), per change in pktlib.h
 Modified Jul 2024 JHB, per changes in pktlib.h due to documentation review, DS_OPEN_PCAP_READ_HEADER and DS_OPEN_PCAP_WRITE_HEADER flags are no longer required in DSOpenPcap() calls, move uFlags to second param in DSReadPcap(), add uFlags param and move pkt buffer len to fourth param in DSWritePcap()
 Modified Jul 2024 JHB, per changes in diaglib.h due to documentation review, move uFlags to second param in calls to DSGetBackTrace()
 Modified Jul 2024 JHB, update pcap_extract mode to support payload and/or packet operations (e.g. insert impairments, filter/remove packets by matching criteria (e.g. SSRC), etc). Initial support for --random_bit_error N command line argument, with --filter_packet_ssrc N,N,N ... planned, and others to follow
 Modified Jul 2024 JHB, modify calls to DSWritePcap() to add pcap_hdr_t* param, remove timestamp (struct timespec*) and TERMINATION_INFO* params (the packet record header param now supplies a timestamp, if needed, and IP type is read from packet data in pkt_buf). See pktlib.h comments
*/

/* Linux header files */

#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>
#include <math.h>

/* app support header files */

#include "mediaTest.h"
#include "minmax.h"
#include "cmd_line_options_flags.h"  /* bring in ENABLE_xxx definitions used when parsing -dN cmd line flag (look for "debugMode"), JHB Jan2022 */
#include "dsstring.h"
#include "gpx/gpxlib.h"

/* SigSRF lib header files (all libs are .so format) */

#include "voplib.h"
#include "pktlib.h"
#include "alglib.h"
#include "diaglib.h"

int detect_codec_type_and_bitrate(uint8_t* rtp_pkt, uint32_t rtp_pyld_len, unsigned int uFlags, uint8_t payload_type, int codec_type, uint32_t* bitrate, uint32_t* ptime, uint8_t* cat);

/* Used to hold input file names for codec test mode */
/* when the ft_info struct is populated, memory is allocated to hold the filename strings */
char* encoder_input_files[MAX_CODEC_INSTANCES] = { NULL };
char* decoder_input_files[MAX_CODEC_INSTANCES] = { NULL };

int encoded_frame_cnt[MAX_CODEC_INSTANCES] = { 0 };
int decoded_frame_cnt[MAX_CODEC_INSTANCES] = { 0 };

char thread_status[2*MAX_CODEC_INSTANCES] = { 0 };

int numChan = 1;  /* numChan is sitting out here because it's referred to in USB audio callback functions in aviolib. To-do: fix this by adding a caller data struct when registering callback functions (a pointer to which is then available pcm_callback in struct sent to callback functions) , JHB Feb2022 */

static int nProcessInit = 0, nProcessClose = 0;  /* multi-thread synchronization for any once-per-process initialization and cleanup, JHB Feb 2024 */

extern PLATFORMPARAMS PlatformParams;  /* command line params */

#define AUDIO_SAMPLE_SIZE  2       /* in bytes.  Currently all codecs take 16-bit samples.  Some like AMR require 14-bit left-justified within 16 bits */

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

unsigned int numChan_device = 2;  /* currently set for Focusrite 2i2. These will be replaced with reference to SESSION_CONTROL struct, which will contain per-device info */
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

         ret_val = DSSaveDataFile(DS_GM_HOST_MEM, p_fp_out_stripped, NULL, (uintptr_t)addr, len, DS_WRITE | DS_DATAFILE_USE_SEMAPHORE, pMediaInfoStripped);
      }
   }
   else {  /* write to audio segment file(s) */

      ret_val = DSSaveDataFile(DS_GM_HOST_MEM, p_fp_out_segment, NULL, (uintptr_t)addr, len, DS_WRITE | DS_DATAFILE_USE_SEMAPHORE, pMediaInfoSegment);
      if (len) fMarkerWritten = false;

      if (nSegmentation & DS_SEGMENT_OUTPUT_CONCATENATE) {

         ret_val = DSSaveDataFile(DS_GM_HOST_MEM, p_fp_out_concat, NULL, (uintptr_t)addr, len, DS_WRITE | DS_DATAFILE_USE_SEMAPHORE, pMediaInfoConcat);
      }

      if (nSegmentation & DS_SEGMENT_OUTPUT_STRIPPED) {

         ret_val = DSSaveDataFile(DS_GM_HOST_MEM, p_fp_out_stripped, NULL, (uintptr_t)zerobuf, len, DS_WRITE | DS_DATAFILE_USE_SEMAPHORE, pMediaInfoStripped);
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

      DSSaveDataFile(DS_GM_HOST_MEM, p_fp_out_segment, NULL, (uintptr_t)NULL, 0, DS_CLOSE | DS_DATAFILE_USE_SEMAPHORE, pMediaInfoSegment);  *p_fp_out_segment = NULL;  /* close current audio segment file */

      strcpy(tmpstr, pMediaInfoSegment->szFilename);
      char* p = strrchr(tmpstr, '.');  if (p) *p = 0;
      char* p2 = (char*)strrstr(tmpstr, "_seg");  if (p2) *p2 = 0;
  
      sprintf(pMediaInfoSegment->szFilename, "%s_seg%d%s%s", tmpstr, segment_count + 1, p ? "." : "", p ? p+1 : "");
      if (nSegmentation & DS_SEGMENT_TIMESTAMPS_TEXT) sprintf(segments_text_filename, "%s_seg_ts.txt", tmpstr);  /* form timestamp text filename at this point, if it will be needed */

      //printf(" $$$$$$$$ MediaInfoSegment.szFilename = %s \n", MediaInfoSegment.szFilename);

      ret_val = DSSaveDataFile(DS_GM_HOST_MEM, p_fp_out_segment, pMediaInfoSegment->szFilename, (uintptr_t)NULL, 0, DS_CREATE | DS_DATAFILE_USE_SEMAPHORE, pMediaInfoSegment);  /* open next audio segment file */
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

void* mediaTest_proc(void* thread_arg) {

int thread_index = *((int*)thread_arg) & 0xff;
int num_app_threads = (*((int*)thread_arg) & 0xff00) >> 8;
bool fProcessEntry = executeMode[0] == (char)-1;
char tmpstr[1024] = "";

   char threadstr[20]; if (num_app_threads) sprintf(threadstr, "thread = %d", thread_index);
   printf("x86 mediaTest() entry point (%s) \n", num_app_threads ? threadstr : "process");

   if (codec_test && (fProcessEntry || num_app_threads > 0)) {

      char* szBacktrace = &((char*)thread_arg)[4];
      if (strlen(szBacktrace) < 1000 && strstr(szBacktrace, "backtrace:")) sprintf(&tmpstr[strlen(tmpstr)], "%s", &szBacktrace[11]);

      DSGetBacktrace(4, 0, &tmpstr[strlen(tmpstr)]);  /* currently only 4 levels needed; might change, JHB May 2024 */

      char tstr[300];
      sprintf(tstr, "codec test start, debug flags = 0x%llx, start sequence = %s", (unsigned long long)debugMode, tmpstr);
      if (num_app_threads > 0) sprintf(&tstr[strlen(tstr)], ", thread = %d", thread_index);
      printf("%s \n", tstr);

      #define MAX_FS_CONVERT_MEDIATEST  160  /* mediaTest sampling rate conversion worst case:  44100 to/from 48000 kHz (was 24 for 8 to/from 192 kHz) */
      #define MAX_CHAN_FS_CONVERT_TRADEOFF_SIZE  (MAX_FS_CONVERT_MEDIATEST*4)  /* to limit stack usage, we define a "tradeoff size" between number of audio channels and worst-case Fs conversion, for example 4 channels at 44.1 <--> 48 kHz, or 100 channels at 8 <--> 48 kHz, etc */
      uint8_t in_buf[MAX_RAW_FRAME*MAX_CHAN_FS_CONVERT_TRADEOFF_SIZE*AUDIO_SAMPLE_SIZE];
      uint8_t out_buf[MAX_RAW_FRAME*MAX_CHAN_FS_CONVERT_TRADEOFF_SIZE*AUDIO_SAMPLE_SIZE] = { 0 };

      uint8_t coded_buf[MAX_CODED_FRAME*MAX_AUDIO_CHAN], coded_buf_sav[MAX_CODED_FRAME*MAX_AUDIO_CHAN];

      int ret_val = 0;
      int framesize = -1, i;

      FILE *fp_in = NULL, *fp_out = NULL;
      HFILE hFile_in = (intptr_t)NULL;  /* filelib file handle, JHB Feb 2022 */
      int frame_count = 0;
      bool fRepeatIndefinitely = (nRepeat == 0); /* nRepeat is initialized in cmd_line_interface.c from -RN cmd line entry (if no entry nRepeat = -1). See also mediaMin usage of nRepeat, JHB Feb2022 */
      char tmpstr2[1024] = "", szConfigInfo[256];
      codec_test_params_t  codec_test_params;
      char default_config_file[] = "session_config/codec_test_config";
      char *config_file;
      int len;
      unsigned int inbuf_size = 0;
      uint8_t* addr;
      char key;
      unsigned int sampleRate_input = 0, sampleRate_output = 0, sampleRate_codec = 8000, fs_divisor;
      bool fConfig_vs_InputChanConflict = false;
      int input_framesize = 0;  /* in bytes, determined by input sampling rate and codec or pass-thru framesize */
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

      short int fs_convert_delay_buf[MAX_AUDIO_CHAN][MAX_CHAN_FS_CONVERT_TRADEOFF_SIZE] = {{ 0 }};
      short int fs_convert_delay_buf_output[MAX_AUDIO_CHAN][MAX_CHAN_FS_CONVERT_TRADEOFF_SIZE] = {{ 0 }};
      unsigned int upFactor = 0, downFactor = 0, upFactor_output = 0, downFactor_output = 0;
      float codec_frame_duration = 0;  /* in msec */

      FILE *fp_cfg = NULL;

      MEDIAINFO MediaInfo = { 0 };

      struct timeval tv;
      uint64_t t1, t2;
      char szCodecName[50] = "";
      bool fFramePrint = false;

      bool fCreateCodec = true;  /* set to false for pass-thru case (no codecs specified) */
      HCODEC encoder_handle[MAX_AUDIO_CHAN] = { 0 }, decoder_handle[MAX_AUDIO_CHAN] = { 0 };  /* codec handles: 0 = not initialized, < 0 indicates an error, > 0 is valid codec handle. We are using arrays of handles here to allow multichannel audio processing */
      CODEC_PARAMS CodecParams = { 0 };  /* see voplib.h */
      CODEC_OUTARGS encOutArgs = { 0 };  /* encoder output args currently only needed by AMR-WB+. See comments below */

      #define MAX_SID_FRAMESIZE 10
      int nSIDStats[MAX_SID_FRAMESIZE] = { 0 };
      bool fPrintSIDStats = false;
      uint8_t uStripFrame = 0;
      MEDIAINFO MediaInfoSegment = { 0 };
      MEDIAINFO MediaInfoConcat = { 0 };
      MEDIAINFO MediaInfoStripped = { 0 };
      FILE* fp_out_segment = NULL, *fp_out_concat = NULL, *fp_out_stripped = NULL;
      char* p;

      #ifndef _NO_PKTLIB_  /* JHB Aug 2022 */

      HPLATFORM hPlatform = -1;  /* DirectCore platform handle, see DSAssignPlatform() call */

   /* items added to support pcap output, JHB Jan2021 */

      TERMINATION_INFO term_info;
      FORMAT_PKT format_pkt;
      unsigned int uFlags_format_pkt;
      uint16_t seq_num = 0;
      uint32_t timestamp = 0;
      uint32_t SSRC = 0x1235678;
      int nMarkerBit = 1;
      uint8_t pkt_buf[1024] = { 0 };
      int pkt_len;
      struct timespec ts_pcap;
      uint64_t nsec_pcap = 0;
      #endif

   /* Event log notes:

      1) SigSRF libraries use the Log_RT() API for event logging.  Applications also should use this API (located in diaglib)

      2) If an application does not define/enable event logging, ptklib uses the LOG_OUTPUT define (below) to enable and manage Log_RT() output (default
         definition is LOG_SCREEN_FILE). In this case, applications are responsible for opening and closing log files, and if so, then passing a log file
         handle within a DEBUG_CONFIG struct (uEventLogFile) and giving a pointer to the struct in DSConfigPktlib(). DEBUG_CONFIG is defined in shared_include/config.h
 
      3) If applications do not define/enable event logging, then pktlib does it

      4) The log level can be controlled (uLogLevel within a DEBUG_CONFIG struct).  Log levels are defined in config.h (more or less they follow the Linux standard,
         e.g. http://man7.org/linux/man-pages/man2/syslog.2.html)
   */

      //#define LOG_OUTPUT  LOG_SCREEN_ONLY  /* screen output only */
      //#define LOG_OUTPUT  LOG_FILE_ONLY  /* file output only */
      #define LOG_OUTPUT  LOG_SCREEN_FILE  /* screen + file output */

   /* start of code for codec test */

   /* Configure (i) event logging, (ii) voplib, and (iii) pktlib if it will be needed. Note that we check the global event log FILE* first, in case an application (e.g. mediaTest) has already opened it, JHB Dec 2022 */

      DEBUG_CONFIG dbg_cfg = { 0 };  /* added JHB Dec 2022 */
      bool fInitAdvancedLogging = false;

      dbg_cfg.uDisableMismatchLog = 1;
      dbg_cfg.uDisableConvertFsLog = 1;
      dbg_cfg.uLogLevel = 8;  /* 5 is default, set to 8 to see INFO messages, including jitter buffer */
      dbg_cfg.uEventLogMode = LOG_OUTPUT | DS_EVENT_LOG_UPTIME_TIMESTAMPS;  /* enable event log and timestamps. Display on console and write to event log file */

      #if (LOG_OUTPUT != LOG_SCREEN_ONLY)
      strcpy(dbg_cfg.szEventLogFilePath, sig_lib_event_log_filename);  /* set event log filename */
      #endif

      dbg_cfg.uPrintfLevel = 5;

      fInitAdvancedLogging = DSInitLogging(&dbg_cfg, 0);  /* initialize event logging. Note that DSInitLogging() should be called once per thread and not called twice without a matching DSCloseLogging() call, as it increments a semaphore count to track multithread usage. DSInitLogging() is in diaglib, JHB Dec 2022 */

      if (__sync_val_compare_and_swap(&nProcessInit, 0, 1) == 0) {   /* first thread to get here does anything required once per process, JHB Feb 2024 */
 
         hPlatform = DSAssignPlatform(NULL, PlatformParams.szCardDesignator, 0, 0, 0);  /* assign DirectCore platform handle for process concurrency and VM management */
          __sync_lock_test_and_set(&nProcessInit, max(num_app_threads, 1)+1);
      }
      else while (__sync_fetch_and_add(&nProcessInit, 0) != max(num_app_threads, 1)+1);  /* other threads wait */

      DSConfigVoplib(NULL, NULL, DS_CV_INIT);  /* initialize voplib */

      #ifndef _NO_PKTLIB_  /* JHB Aug 2022 */
      if (inFileType == PCAP || outFileType == PCAP) DSConfigPktlib(NULL, &dbg_cfg, DS_CP_INIT);  /* initialize pktlib if needed */
      #endif

   /* look at in and out file types (see cmd line parsing at start of main() in mediaTest.c */

      if (inFileType == ENCODED && outFileType == ENCODED) {
         fprintf (stderr, "ERROR: both input and output file types encoded is unsupported\n");
         goto codec_test_cleanup;
      }

      if (inFileType != USB_AUDIO) {

         if (inFileType != ENCODED) {

            DSLoadDataFile(DS_GM_HOST_MEM, &fp_in, MediaParams[0].Media.inputFilename, (uintptr_t)NULL, 0, DS_OPEN | DS_DATAFILE_USE_SEMAPHORE, &MediaInfo, &hFile_in);  /* for wav files, pMediaInfo will be initialized with wav file header info */
         }
         else {

            fp_in = fopen(MediaParams[0].Media.inputFilename, "rb");  /* can be .cod, .pcap, etc */
         }

         char filestr[20] = "audio";
         if (inFileType == ENCODED) strcpy(filestr, "encoded");
         else if (inFileType == PCAP) strcpy(filestr, "pcap");

         if (fp_in) printf("Opened %s input file %s\n", filestr, MediaParams[0].Media.inputFilename);
         else {
            printf("Unable to open %s input file %s\n", filestr, MediaParams[0].Media.inputFilename);
            goto codec_test_cleanup;
         }

      /* use results of DSLoadDataFile() if valid */

         if (MediaInfo.Fs > 0) sampleRate_input = MediaInfo.Fs;
         if (MediaInfo.NumChan > 0) numChan = MediaInfo.NumChan;
//     printf(" .wav file numchan = %d \n", numChan);
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

         codec_test_params.codec_type = DS_CODEC_TYPE_NONE;

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

         sprintf(szConfigInfo, "No config file specified, assuming default parameters: ");
      }
      else {

         parse_codec_config(fp_cfg, &codec_test_params);

         sampleRate_output = codec_test_params.sample_rate;

         if (sampleRate_input == 0) {  /* raw audio file with no header. If the "input_sample_rate" field is non-zero then we can use that, otherwise we assume input and output rates are equal, JHB Dec 2023 */

            if (codec_test_params.input_sample_rate > 0) sampleRate_input = codec_test_params.input_sample_rate;
            else sampleRate_input = sampleRate_output;
         }

         numChan = codec_test_params.num_chan;  /* default is 1 if num_chan is not specified in the codec config file */

         if (MediaInfo.NumChan > 0 && MediaInfo.NumChan != numChan) {  /* config file conflicts with input waveform header; we trust the latter, JHB Feb2022 */

            numChan = MediaInfo.NumChan;
            fConfig_vs_InputChanConflict = true;
         }

         sprintf(szConfigInfo, "Opened config file: ");
      }

   /* update MediaInfo struct if it still doesn't have valid numbers */

      if (!MediaInfo.Fs) MediaInfo.Fs = sampleRate_input;;
      if (!MediaInfo.NumChan) MediaInfo.NumChan = numChan;
      if (!MediaInfo.SampleWidth) MediaInfo.SampleWidth = DS_DP_SHORTINT;
      if (!MediaInfo.CompressionCode) MediaInfo.CompressionCode = DS_GWH_CC_PCM;

      if (DSGetCodecInfo(codec_test_params.codec_type, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_NAME, 0, 0, szCodecName) <= 0) {

         printf("\rError: non-supported or invalid codec type found in config file\n");
         goto codec_test_cleanup;
      }

   /* print some config file items */
 
      sprintf(&szConfigInfo[strlen(szConfigInfo)], "codec = %s, ", szCodecName);
      if (codec_test_params.codec_type != DS_CODEC_TYPE_NONE) sprintf(&szConfigInfo[strlen(szConfigInfo)], "bitrate = %d bps, ", codec_test_params.bitrate);
      if (inFileType != ENCODED) sprintf(&szConfigInfo[strlen(szConfigInfo)], "input sample rate = %d Hz, ", sampleRate_input);
      if (outFileType != ENCODED && outFileType != PCAP) sprintf(&szConfigInfo[strlen(szConfigInfo)], "output sample rate = %d Hz, ", sampleRate_output);
      else {
         char frmstr[30];
         sprintf(frmstr, "%d", codec_test_params.framesize);
         sprintf(&szConfigInfo[strlen(szConfigInfo)], "framesize (bytes) = %s, ", (int)codec_test_params.framesize == -1 ? "not specified" : frmstr);
      }
      sprintf(&szConfigInfo[strlen(szConfigInfo)], "num channels = %d", codec_test_params.num_chan);
      if (fConfig_vs_InputChanConflict) sprintf(&szConfigInfo[strlen(szConfigInfo)], "(note: input waveform header %d channels overrides config file value %d)", numChan, codec_test_params.num_chan);

      printf("%s \n", szConfigInfo);

      if (codec_test_params.codec_type != DS_CODEC_TYPE_NONE && (int)codec_test_params.bitrate <= 0) {

         printf("Error: config file specifies a codec but not a bitrate\n");
         goto codec_test_cleanup;
      }

      memset(&CodecParams, 0, sizeof(CodecParams));

   /* setup/init for specified codec.  Codecs use voplib APIs */

      switch (codec_test_params.codec_type) {

         case DS_VOICE_CODEC_TYPE_EVS:
         {
            CodecParams.enc_params.samplingRate = codec_test_params.sample_rate;       /* in Hz. Note that for fullband (FB, 48 kHz) sampling rate (Fs) with cutoff frequency (Fc) of 20 kHz, a minimum bitrate of 24.4 kbps is required. If you give 13.2 kbps bitrate, then the codec enforces an Fc of 14.4 kHz */
            CodecParams.enc_params.bitRate = codec_test_params.bitrate;                /* in bps. Note that bitrate determines whether Primary or AMR-WB IO mode payload format is used (see the EVS specification for valid rates for each mode) */
            CodecParams.enc_params.dtx.dtx_enable = codec_test_params.dtx_enable;      /* 0 = DTX disabled, 1 = enabled */
            CodecParams.enc_params.sid_update_interval = codec_test_params.dtx_value ? codec_test_params.dtx_value : (codec_test_params.dtx_enable ? 8 : 0);  /* if DTX is enabled then default SID update interval is 8.  A zero update interval enables "adaptive SID" */
            CodecParams.enc_params.rf_enable = codec_test_params.rf_enable;
            CodecParams.enc_params.fec_indicator = codec_test_params.fec_indicator;
            CodecParams.enc_params.fec_offset = codec_test_params.fec_offset;
            CodecParams.enc_params.bandwidth_limit = codec_test_params.fec_offset;
            #if 0
            CodecParams.enc_params.bandwidth_limit = DS_EVS_BWL_SWB;                   /* codec will set limit depending on sampling rate */
            #else  /* change this to make encoder setup more clear, JHB Feb2022 */
            if ((int)codec_test_params.bandwidth_limit == -1) CodecParams.enc_params.bandwidth_limit = DS_EVS_BWL_FB;  /* codec will set limit lower if required by specified sampling rate, JHB Feb 2022 */
            else CodecParams.enc_params.bandwidth_limit = codec_test_params.bandwidth_limit;
            #endif

            //#define SHOW_INIT_PARAMS  /* enable to see codec initialization params, both on entry after calling DSCodecCreate() and just before initializing the encoder (i.e. after all param validation and check), JHB Dec 2023 */
            #ifdef SHOW_INIT_PARAMS
            CodecParams.enc_params.uFlags |= DEBUG_OUTPUT_SHOW_INIT_PARAMS;
            #endif

            if ((int)codec_test_params.header_format == -1) CodecParams.enc_params.rtp_pyld_hdr_format.header_format = 1;  /* if no value given (default entry) then hard code to 1 to match 3GPP encoder reference executable, which only writes header full format */
            else CodecParams.enc_params.rtp_pyld_hdr_format.header_format = codec_test_params.header_format;  /* otherwise use value given in codec config file */

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
            CodecParams.enc_params.rtp_pyld_hdr_format.oct_align = codec_test_params.header_format;

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
            CodecParams.enc_params.samplingRate = codec_test_params.sample_rate;                                        /* in Hz */
            CodecParams.enc_params.bitRate = (int)codec_test_params.mode == -1 ? codec_test_params.bitrate_plus : 0.0;  /* in bps */
            CodecParams.enc_params.mode = codec_test_params.mode;
            CodecParams.enc_params.isf = codec_test_params.isf;
            CodecParams.enc_params.low_complexity = codec_test_params.low_complexity;
            CodecParams.enc_params.dtx.vad = codec_test_params.vad;
            #if 0
            CodecParams.enc_params.rtp_pyld_hdr_format.oct_align = codec_test_params.header_format;  /* no mention of CMR in RFC 4352, so we assume CMR is not used in AMR-WB+, JHB Mar 2023 */
            #endif
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

         /* codec_frame_duration notes:
         
            -for G726, increase if more than 10 msec is being encoded or decoded per frame
            -currently codec_test_params_t struct (mediaTest.h) doesn't have a ptime element to control framesize multiples. That's a to-do item
            -packet/media thread processing in pktlib does handle ptime
         */

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

            #if 0  /* now done in common section "if fCreateCodec" below, JHB Jul2019 */
            if (inFileType != ENCODED) printf("MELPe before DSCodecCreate (encoder), bitrate = %d, bitDensity = %d, Npp = %d\n", encoderParams.bitRate, encoderParams.bitDensity, encoderParams.Npp);
            if (outFileType != ENCODED) printf("MELPe before DSCodecCreate (decoder), bitrate = %d, bitDensity = %d, post filter = %d\n", decoderParams.bitRate, decoderParams.bitDensity, decoderParams.post);
            #endif

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
         unsigned int uFlags = (debugMode & ENABLE_MEM_STATS) ? DS_CODEC_TRACK_MEM_USAGE : 0;  /* debugMode set with -dN on cmd line. ENABLE_MEM_STATS is defined in cmd_line_options_flags.h, JHB Jan2022 */

         for (i=0; i<numChan; i++) {

            if ((inFileType != ENCODED) && (encoder_handle[i] = DSCodecCreate(&CodecParams, DS_CODEC_CREATE_ENCODER | uFlags)) < 0) {
               printf("codec test mode, failed to init encoder\n");
               goto codec_test_cleanup;
            }

            if ((outFileType != ENCODED && outFileType != PCAP) && (decoder_handle[i] = DSCodecCreate(&CodecParams, DS_CODEC_CREATE_DECODER | uFlags)) < 0) {
               printf("codec test mode, failed to init decoder\n");
               goto codec_test_cleanup;
            }
         }
      }

   /* Sampling rate conversion setup. Note there are two (2) possible Fs conversion stages (i) before encoding (because encoder rates are limited) and (ii) after decoding (because output rates are limited for some reason, such as USB audio) */

   /* set up and down factors for possible input sampling rate conversion (applied if sampleRate_input != sampleRate_output) */

      sampleRate_input = max(sampleRate_input, 1);  /* don't allow zeros / avoid floating-point exceptions, JHB Sep 2022 */
      sampleRate_output = max(sampleRate_output, 1);

      #if 1  /* use glibc greatest common demoninator function, JHB Feb2022 */
      fs_divisor = gcd(sampleRate_input, sampleRate_output);
      upFactor = sampleRate_output / fs_divisor;
      downFactor = sampleRate_input / fs_divisor;
      #else
      upFactor = sampleRate_output > sampleRate_input ? sampleRate_output / sampleRate_input : 1;
      downFactor = sampleRate_input > sampleRate_output ? sampleRate_input / sampleRate_output : 1;
      #endif

   /* set up and down factors for possible output sampling rate conversion (applied if sampleRate_output != sampleRate_USBAudio) */

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

            coded_framesize = DSGetCodecInfo(codec_test_params.codec_type, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_CODED_FRAMESIZE, codec_test_params.bitrate, 0, NULL);
            break;

         case DS_VOICE_CODEC_TYPE_G729AB:

            coded_framesize = DSGetCodecInfo(codec_test_params.codec_type, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_CODED_FRAMESIZE, codec_test_params.bitrate, 0, NULL);
            break;

         case DS_VOICE_CODEC_TYPE_EVS:
         case DS_VOICE_CODEC_TYPE_AMR_NB:
         case DS_VOICE_CODEC_TYPE_AMR_WB:
         case DS_VOICE_CODEC_TYPE_AMR_WB_PLUS:

            coded_framesize = DSGetCodecInfo(codec_test_params.codec_type, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_CODED_FRAMESIZE, codec_test_params.bitrate, codec_test_params.header_format, NULL);  /* header_format (3rd) param was previously hardcoded to HEADERFULL, now reflects what's in the config file. See voplib.h for "header format definitions", JHB Sep 2022 */
            break;

         case DS_VOICE_CODEC_TYPE_MELPE:

            if (!codec_test_params.bitDensity) codec_test_params.bitDensity = 54;  /* default bit density handling should be moved to transcoder_control.c */
            coded_framesize = DSGetCodecInfo(codec_test_params.codec_type, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_CODED_FRAMESIZE, codec_test_params.bitrate, codec_test_params.bitDensity, NULL);
            break;

         case DS_CODEC_TYPE_NONE:

#if defined(_ALSA_INSTALLED_) && defined(ENABLE_USBAUDIO)
            if (fUSBTestMode) input_framesize = period_size_USBAudio*AUDIO_SAMPLE_SIZE;  /* for USB test mode, use hardcoded params (see above) */
#endif
            if (inFileType == ENCODED) {

               if ((int)codec_test_params.framesize != -1) {
                  input_framesize = codec_test_params.framesize;
                  coded_framesize = codec_test_params.framesize;
               }
            }
            break;
      }

      if (codec_test_params.codec_type != DS_CODEC_TYPE_NONE && !coded_framesize) {

         printf("Error: DSGetCodecInfo() with DS_CODEC_INFO_CODED_FRAMESIZE flag returns zero \n");
         goto codec_test_cleanup;
      }

   /* set buffer size just prior to codec (or pass-thru) input.  Note that coded_buf is not used for pass-thru mode */

      inbuf_size = input_framesize*upFactor/downFactor;

   /* print some relevant params and stats */

      if (codec_test_params.codec_type != DS_CODEC_TYPE_NONE) {
         if (encoder_handle[0]) strcpy(tmpstr, "encoder");
         if (decoder_handle[0]) sprintf(tmpstr2, "coded framesize (bytes) = %d, ", coded_framesize);
      }
      else {
         strcpy(tmpstr, "pass-thru");
      }

      if (inFileType == ENCODED)
        printf("  %s framesize (bytes) = %d, %snum channel%s = %d \n", tmpstr, coded_framesize, tmpstr2, numChan > 1 ? "s" : "", numChan);
      else
        printf("  input framesize (samples) = %d, %s framesize (samples) = %d, %sinput Fs = %d Hz, output Fs = %d Hz, num channel%s = %d%s%s \n", input_framesize/AUDIO_SAMPLE_SIZE, tmpstr, inbuf_size/AUDIO_SAMPLE_SIZE, tmpstr2, sampleRate_input, sampleRate_output, numChan > 1 ? "s" : "", numChan, codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_EVS ? ", header format = " : "", codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_EVS ? (CodecParams.enc_params.rtp_pyld_hdr_format.header_format == 1 ? "full" : "compact") : "");

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

            if (codec_test_params.codec_type != DS_CODEC_TYPE_NONE) {  /* in this case Fs conversion may be needed twice, once prior to codec processing, and once after, JHB Oct 2018 */

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

// #define CODEC_FILE_DEBUG /* turn on for AMR encode/decode debug */

   /* adjust encoded input file offset, if needed depending on file type. Notes:

      -we assume EVS and AMR .cod files have MIME headers, with a magic number in first few bytes:
        #!AMR\n          AMR-NB, defined in  RFC4867
        #!AMR-WB\n       AMR-WB, defined in  RFC4867
        #!EVS_MC1.0\n    EVS, defined in ETSI 3GPP 26.445

      -MIME headers contain no frame size or sampling rate info
   */

      if (inFileType == ENCODED) {

         if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_NB) {
            fseek(fp_in, 6, SEEK_SET);  /* for input COD file, skip AMR MIME header (only used for file i/o operations with decoder) */
         }
         else if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_WB) {

            #ifdef CODEC_FILE_DEBUG
            printf(" AMR-WB seek header 9 \n");
            #endif

            fseek(fp_in, 9, SEEK_SET);  /* for input COD file, skip AMR MIME header (only used for file i/o operations with decoder) */
         }
         else if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_WB_PLUS) {  /* AMR-WB+ uses .bit file extension */

         }
         else if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_EVS) {
 
            fseek(fp_in, 16, SEEK_SET);  /* for input .cod files, skip EVS MIME header (only used for file i/o operations with decoder) */
         }
         else {  /* note that .cod files don't necessarily have a header; e.g. this is the case for G726 and G729AB, JHB Apr2021 */

         /* see if .cod file has a MIME header and if so seek to start of data accordingly, JHB Sep 2022 */

            char header_buf[100];
            if ((ret_val = fread(header_buf, sizeof(uint8_t), 16, fp_in)) != 16) goto codec_test_cleanup;  /* exit out if .cod value has less than 16 bytes ... not good */
            if (strstr(header_buf, "#!EVS_MC1.0\n")) {}  /* no further adjustment, already at start of data offset (don't know what the trailing 4 bytes are for) */
            else if (strstr(header_buf, "#!AMR\n")) fseek(fp_in, SEEK_CUR, -10);
            else if (strstr(header_buf, "#!AMR-WB\n")) fseek(fp_in, SEEK_CUR, -7);
            else fseek(fp_in, 0, SEEK_SET);  /* no supported MIME header, reset file pointer */
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

            if (IS_AUDIO_FILE_TYPE(outFileType2)) strcpy(tmpstr, MediaParams[1].Media.outputFilename);
            else strcpy(tmpstr, MediaParams[0].Media.outputFilename);
         }

         p = strrchr(tmpstr, '.');  if (p) *p = 0;
         sprintf(MediaInfoSegment.szFilename, "%s_seg0.wav", tmpstr);

         ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_segment, MediaInfoSegment.szFilename, (uintptr_t)NULL, 0, DS_CREATE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfoSegment);

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

            ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_concat, MediaInfoConcat.szFilename, (uintptr_t)NULL, 0, DS_CREATE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfoConcat);

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

            ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_stripped, MediaInfoStripped.szFilename, (uintptr_t)NULL, 0, DS_CREATE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfoStripped);

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

      if (outFileType == ENCODED || outFileType == PCAP) {

         if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_EVS) MediaInfo.CompressionCode = DS_GWH_CC_EVS;
         else if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_MELPE)  MediaInfo.CompressionCode = DS_GWH_CC_MELPE;
         else if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_NB) MediaInfo.CompressionCode = DS_GWH_CC_GSM_AMR;
         else if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_WB) MediaInfo.CompressionCode = DS_GWH_CC_GSM_AMRWB;
      }

   /* open output file. If output is .wav, DSSaveDataFile() uses MediaInfo[] elements to set the wav header */

      if (outFileType != USB_AUDIO) {

         if (IS_AUDIO_FILE_TYPE(outFileType2)) strcpy(MediaInfo.szFilename, MediaParams[1].Media.outputFilename);
         else strcpy(MediaInfo.szFilename, MediaParams[0].Media.outputFilename);

         char szOutFilename[DSMAXPATH];
         strcpy(szOutFilename, MediaInfo.szFilename);

         if (thread_index > 0) {
            char* p = strrchr(szOutFilename, '.');
            char ext[DSMAXPATH] = "";
            if (p) { *p++ = 0; strcpy(ext, p); }
            sprintf(&szOutFilename[strlen(szOutFilename)], "_%d.%s", thread_index, ext);

            strcpy(MediaInfo.szFilename, szOutFilename);  /* update MediaInfo filename, it may be used in DSLoadDataFile() or DSSaveDataFile() APIs */
         }

         #ifndef _NO_PKTLIB_  /* JHB Aug 2022 */

         if (outFileType == PCAP) {

            ret_val = DSOpenPcap(szOutFilename, DS_WRITE, &fp_out, NULL, "");

            if (ret_val < 0) {
               fprintf(stderr, "Failed to open output pcap file: %s, ret_val = %d\n", szOutFilename, ret_val);
               goto codec_test_cleanup;
            }

         /* one-time packet IP header and RTP header set-up. Notes

            -DS_FMT_PKT_STANDALONE flag allows DSFormatPacket() pktlib API to be used without creating a session and streams (channels) via DSCreateSession()
            -several hardcoded values for now, user-specified IP version, IP addr, UDP port, payload type, etc can be implemented later
            -DS_FMT_PKT_USER_HDRALL specifies DS_FMT_PKT_USER_SRC_IPADDR, DS_FMT_PKT_USER_DST_IPADDR, DS_FMT_PKT_USER_SRC_PORT, and DS_FMT_PKT_USER_DST_PORT
         */

            uFlags_format_pkt = DS_FMT_PKT_STANDALONE | DS_FMT_PKT_USER_HDRALL | DS_FMT_PKT_USER_SEQNUM | DS_FMT_PKT_USER_TIMESTAMP | DS_FMT_PKT_USER_PYLDTYPE | DS_FMT_PKT_USER_SSRC | DS_FMT_PKT_USER_MARKERBIT;

            #if 0
            term_info.local_ip.type = DS_IPV6;
            term_info.remote_ip.type = DS_IPV6;
            memcpy(&term_info.local_ip.u.ipv6, xxx, DS_IPV6_ADDR_LEN);  /* DS_IPV6_ADDR_LEN defined in shared_include/session.h */
            memcpy(&term_info.remote_ip.u.ipv6, xxx, DS_IPV6_ADDR_LEN);
            #else
            term_info.local_ip.type = DS_IPV4;  /* default: use source/dest IP addr and port and payload type values compatible with "pcap_file_test_config" config file, which is referred to in several mediaTest demo command lines. IPv6 and user-specified IP addr and UDP port can be added later */
            term_info.remote_ip.type = DS_IPV4;
            term_info.local_ip.u.ipv4 = htonl(0xC0A80003);  /* 192.168.0.3 */
            term_info.remote_ip.u.ipv4 = htonl(0xC0A80001);  /* 192.168.0.1 */
            #endif

            term_info.local_port = 0x0228;  /* 10242, network byte order */
            term_info.remote_port = 0x0A18;  /* 6154, network byte order */
            term_info.attr.voice_attr.rtp_payload_type = 127;

            memcpy(&format_pkt.SrcAddr, &term_info.local_ip.u, DS_IPV4_ADDR_LEN);  /* DS_IPVn_ADDR_LEN defined in shared_include/session.h */
            memcpy(&format_pkt.DstAddr, &term_info.remote_ip.u, DS_IPV4_ADDR_LEN);
            format_pkt.IP_Version = term_info.local_ip.type;
            format_pkt.udpHeader.SrcPort = term_info.local_port;
            format_pkt.udpHeader.DstPort = term_info.remote_port;
            #if 0
            format_pkt.rtpHeader.BitFields = term_info.attr.voice_attr.rtp_payload_type;  /* set payload type (BitFields is 16-bit so we use network byte order. An alternative is to apply DS_FMT_PKT_HOST_BYTE_ORDER flag) */
            #else
            format_pkt.rtpHeader.PyldType = term_info.attr.voice_attr.rtp_payload_type;  /* set payload type */
            #endif

            clock_gettime(CLOCK_REALTIME, &ts_pcap);
            nsec_pcap = ts_pcap.tv_sec*1000000L + ts_pcap.tv_nsec;
         }
         else
         #endif  /* ifndef _NO_PKTLIB_ */
         {  /* create output file for all file formats except for pcap */

            ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, szOutFilename, (uintptr_t)NULL, 0, DS_CREATE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfo);  /* DSSaveDataFile returns bytes written, with DS_CREATE flag it returns header length (if any, depending on file type) */
         }

         char filestr[20] = "audio";
         if (outFileType == ENCODED) strcpy(filestr, "encoded");
         else if (outFileType == PCAP) strcpy(filestr, "pcap");

         if (fp_out) printf("Opened output %s file %s\n", filestr, szOutFilename);
         else {
            printf("Failed to open output %s file %s, ret_val = %d\n", filestr, szOutFilename, ret_val);
            goto codec_test_cleanup;
         }
      }

   /* get ready to run the test */

      gettimeofday(&tv, NULL);
      t1 = (uint64_t)tv.tv_sec*1000000L + (uint64_t)tv.tv_usec;

      if (encoder_handle[0] && decoder_handle[0]) sprintf(tmpstr, "encoder-decoder");
      else if (encoder_handle[0]) sprintf(tmpstr, "encoder");
      else if (decoder_handle[0]) sprintf(tmpstr, "decoder");
      else sprintf(tmpstr, "pass-thru");
      printf("Running %s data flow ... \n", tmpstr);

      while (pm_run) {
loop:
         key = toupper(getkey());
         if (key == 'Q') {  /* on quit key break out of while(run) loop */
            pm_run = 0;
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
               ret_val = DSLoadDataFile(DS_GM_HOST_MEM, &fp_in, NULL, (uintptr_t)in_buf, input_framesize*numChan, DS_READ | DS_DATAFILE_USE_SEMAPHORE, NULL, &hFile_in);
 
               if (ret_val > 0) {
                  int i;
                  for (i = ret_val; i < input_framesize*numChan; i++) in_buf[i] = 0;  /* fill in last frame with zeros, if needed (if partial frame) */
               }
               else {
                  segmenter(SEGMENTER_CLEANUP, frame_count, codec_frame_duration, uStripFrame, 0, 0, &fp_out_segment, &MediaInfoSegment, &fp_out_concat, &MediaInfoConcat, &fp_out_stripped, &MediaInfoStripped);  /* clean up segmentation, if active. Note - segmenter() will return immediately if input segmentation is not active, JHB Apr 2021 */

                  if (fRepeatIndefinitely || --nRepeat >= 0) {  /* if repeat active, wrap waveform file, JHB Feb 2022 */

                     int64_t fpos = 0;

                     #if 0  /* either of these work to get start of waveform file data, DSSeekPos() keeps the filelib fp up to date also in case other filelib operations are needed */
                     if (hFile_in) fpos = DSGetWvfrmHeader(hFile_in, DS_GWH_HEADERLEN);  /* get header length, in bytes*/
                     #else
                     if (hFile_in) fpos = DSSeekPos(hFile_in, DS_START_POS | DS_SEEKPOS_RETURN_BYTES, 0);  /* seek to start of waveform file header, if applicable. Note that hFile_in is returned by DSLoadDataFile() with a DS_OPEN param, if the file is a supported waveform file type, JHB Feb2022 */
                     #endif

                     fseek(fp_in, fpos, SEEK_SET);  /* seek to fpos bytes after start of file */

                     goto loop;  /* continue until user hits 'q' */
                  }

                  break;  /* exit while loop */
               }
               #else
               if ((ret_val = DSLoadDataFile(DS_GM_HOST_MEM, &fp_in, NULL, (uintptr_t)in_buf, input_framesize*numChan, DS_READ | DS_DATAFILE_USE_SEMAPHORE, NULL, &hFile_in)) != input_framesize*numChan) break;
               #endif
            }

         /* we have valid input data with no errors; update frame count and process the frame */

            frame_count++;
            printf("\rProcessing frame %d...", frame_count);
            fflush(stdout);
            fFramePrint = true;

            if (nCut > 0) { nCut--; goto loop; }  /* cut N input frames if --cut N entered on cmd line, JHB Nov 2023 */

         /* perform sample rate conversion if needed.  Notes:

              -sampling rate of output data is input rate * upFactor / downFactor
              -data is processed in-place, so in_buf contains both input data and decimated or interpolated output data. For interpolation case, in_buf must point to a buffer large enough to handle the increased amount of output data
              -DSConvertFs() is in alglib
         */

            if (sampleRate_input != sampleRate_output) {

               int num_samples = input_framesize/AUDIO_SAMPLE_SIZE;

               for (i=0; i<numChan; i++) {

                  DSConvertFs(&((short int*)in_buf)[i],  /* pointer to data. Multichannel data must be interleaved */
                              sampleRate_input,          /* sampling rate of data, in Hz */
                              upFactor,                  /* up factor */    
                              downFactor,                /* down factor */
                              fs_convert_delay_buf[i],   /* per-channel pointer to delay values, buffer must be preserved between calls to DSConvertFs() */
                              num_samples,               /* data length per channel, in samples */
                              numChan,                   /* number of interleaved channels in the input data */
                              NULL,                      /* user-defined filter N/A */
                              0,                         /* length of user-defined filter N/A */
                              DS_FSCONV_SATURATE);       /* apply saturate flag to ensure no wrapping in audio output, Feb 2024 */
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

         /* call codec encoder if needed. encOutArgs contains the number of samples needed for the next frame in encOutArgs.size (currently applies only to AMR-WB+, CKJ Dec 2018) */

            if (encoder_handle[0]) {

               CODEC_INARGS* pInArgs = NULL;

               //#define ENCODER_USE_INARGS   /* define to test debug outputs and/or on-the-fly encoder param updates, if supported by the codec type */
               #ifdef ENCODER_USE_INARGS
               CODEC_INARGS CodecInArgs = { 0 };  /* create a CODEC_INARGS struct, init to zero. Especially CMR should be set to zero if not sending a CMR */

               #if 1
               CodecInArgs.pCodecEncParams = &CodecParams.enc_params;  /* default is CODEC_ENC_PARAMS struct used to create the encoder instance */
               #else
               CODEC_ENC_PARAMS CodecEncParams = { 0 };  /* alternatively, create a CODEC_ENC_PARAMS struct, init to zero. Params that are (i) valid and (ii) different than their current value will be updated (i.e. different from their original DSCodecCreate() value or from a prior on-the-fly update) */
               CodecInArgs.pCodecEncParams = &CodecEncParams;
               #endif

               #if 0  /* example of changing bitrate and bandwidth limit */
               if (frame_count == 50) {
                  CodecInArgs.pCodecEncParams->bitRate = 13200;
                  CodecInArgs.pCodecEncParams->bandwidth_limit = DS_EVS_BWL_NB;
               }

               if (frame_count == 100) {
                  CodecInArgs.pCodecEncParams->bandwidth_limit = DS_EVS_BWL_WB;
               }
               #endif

               #if 0  /* example of turning on/off RF aware mode */
               if (frame_count == 200) {
                  CodecInArgs.pCodecEncParams->bitRate = 13200;
                  CodecInArgs.pCodecEncParams->bandwidth_limit = DS_EVS_BWL_WB;
                  CodecInArgs.pCodecEncParams->rf_enable = 1;
                  CodecInArgs.pCodecEncParams->fec_offset = 5;
                  CodecInArgs.pCodecEncParams->fec_indicator = 1;
               }

               if (frame_count == 300) {
                  CodecInArgs.pCodecEncParams->rf_enable = 0;
                  CodecInArgs.pCodecEncParams->fec_offset = 0;
                  CodecInArgs.pCodecEncParams->fec_indicator = 0;
               }
               #endif

               #if 1  /* example of changing bitrate to VBR, then 7200, then back to previous value */
               static int bitrate_save = 0;
               if (frame_count == 200) {
                  bitrate_save = CodecInArgs.pCodecEncParams->bitRate;
                  CodecInArgs.pCodecEncParams->bitRate = 5900;
               }
               if (frame_count == 500) {
                  CodecInArgs.pCodecEncParams->bitRate = 7200;
               }
               if (frame_count == 1000) {
                  CodecInArgs.pCodecEncParams->bitRate = bitrate_save;
               }
               #endif

               #if 0
               if (frame_count == 1000) {
                  CodecInArgs.pCodecEncParams->bitRate = 13200;  /* simple example of changing bitrate */
               }
               #endif

               #if 0  /* option to show encoder output bytes for each frame */
               CodecInArgs.pCodecEncParams->uFlags |= DEBUG_OUTPUT_VOPLIB_SHOW_BITSTREAM_BYTES;  /* show encoder output bytes on exit */
               #endif

               #if 1  /* option to specify console and log output flags. Turn this on to verify updates occur as expected */
               CodecInArgs.pCodecEncParams->uFlags |= DEBUG_OUTPUT_CODEC_LIB_ONTHEFLY_UPDATES | DEBUG_OUTPUT_VOPLIB_ONTHEFLY_UPDATES;
               #endif

               #if 0  /* option to add debug output to event log */
               CodecInArgs.pCodecEncParams->uFlags |= DEBUG_OUTPUT_ADD_TO_EVENT_LOG;
               #endif

               pInArgs = &CodecInArgs;
               #endif  /* ENCODER_USE_INARGS */

            /* call encoder */

               coded_framesize = DSCodecEncode(encoder_handle, 0, in_buf, coded_buf, inbuf_size, numChan, pInArgs, &encOutArgs);  /* voplib codec encode API */

               #if 0  /* debug - view all coded framesizes */
               static bool fOnce[200] = { false };
               if (!fOnce[coded_framesize]) { fOnce[coded_framesize] = true; printf("\n *** DSCodecEncode() returns coded_framesize[%d] \n", coded_framesize); }
               #endif
               #ifdef CODEC_FILE_DEBUG
               printf(" encode: duration (ptime) = %d, uncompress = %d, coded frame size = %d \n", (int)codec_frame_duration, codec_test_params.uncompress, coded_framesize);
               #endif

               if (coded_framesize < 0) {
                  fprintf(stderr, "DSCodecEncode() returns error %d, exiting test \n", coded_framesize);
                  goto codec_test_cleanup;
               }

               int max_sid_framesize = MAX_SID_FRAMESIZE;

               if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_EVS && codec_test_params.bitrate == 5900) {  /* handle EVS VBR mode */
                  if (codec_test_params.header_format == 1) max_sid_framesize = 8;
                  else max_sid_framesize = 7;
               }

               if (coded_framesize < max_sid_framesize) {  /* coded_framesize < max_sid_framesize indicates a silence (SID) frame if encoder has DTX / VAD enabled */

                  nSIDStats[coded_framesize]++;

                  if (nSegmentation & DS_STRIP_SILENCE) {  /* DS_STRIP_xxx flags defined in alglib.h */

                     if (coded_framesize == 1) uStripFrame |= STRIP_FRAME_DTX_CONT;
                     else uStripFrame |= STRIP_FRAME_DTX;
                  }
               }
            }
         }
         else {  /* encoded input */

            int bitrate_code = 0, offset = 0;
            bool fAMROctetAligned = false;
            #ifndef DSGETPAYLOADSIZE  /* DSGetPayloadSize() deprecated, now we use DSGetCodecInfo(), see comments in voplib.h, JHB Oct 2022 */
            unsigned int uFlags = DS_CODEC_INFO_TYPE | DS_CODEC_INFO_CODED_FRAMESIZE;
            unsigned int header_format = 0;
            #endif

            if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_NB || codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_WB) {

               if ((ret_val = fread(coded_buf, sizeof(char), 2, fp_in)) != 2) {  /* read payload header bytes from .amr or .awb file */

                  #ifdef CODEC_FILE_DEBUG
                  printf(" break after ToC read, coded_buf[0] = 0x%x \n", coded_buf[0]);
                  #endif

                  break;
               }

               #if 0
               fseek(fp_in, -2L, SEEK_CUR);  /* restore file pointer */
               #else
               offset = 2;  /* offset is used in fread() */
               #endif

               fAMROctetAligned = coded_buf[0] == 0xf0 && (coded_buf[1] & 3) == 0;  /* added Apr2021, JHB */

               #ifndef DSGETPAYLOADSIZE
               uFlags |= DS_CODEC_INFO_BITRATE_CODE;
               #endif

               if (fAMROctetAligned) bitrate_code = (coded_buf[1] >> 3) & 0x0f;  /* bitrate code is a 4-bit field within 2nd byte for octet aligned format, and split across first two bytes for bandwidth efficient format. The field is labeled "FT" in RFC4867, JHB Apr 2021 */
               else bitrate_code = ((coded_buf[0] & 7) << 1) | (coded_buf[1] >> 7);

               #ifdef CODEC_FILE_DEBUG
               if (frame_count < 30) printf(" file pos = %d, in buf[0,1] = 0x%x, 0x%x, bitrate code = 0x%x \n", (int)ftell(fp_in), coded_buf[0], coded_buf[1], bitrate_code);
               #endif
            }

            if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_EVS) {

               if ((ret_val = fread(coded_buf, sizeof(char), 1, fp_in)) != 1) break;  /* read ToC byte from .cod file (see pcap extract mode notes below about .cod file format) */
               #ifndef DSGETPAYLOADSIZE
               uFlags |= DS_CODEC_INFO_BITRATE_CODE;
               #endif

               bitrate_code = coded_buf[0] & 0x3f;  /* bitrate code is a 5-bit field within ToC byte (changed from 0x0f to 0x3f to support EVS AMR-WB IO mode, JHB Oct 2022). Note that for EVS VBR .cod files, each frame may have different ToC (containing a different bitrate code) JHB Feb 2024 */

               offset = 1;  /* we assume header-full format in .cod files and we handle it using offset in fread() below, so we don't indicate header-full (nInput2 = 1) in call to DSGetCodecInfo(DS_CODEC_INFO_CODED_FRAMESIZE) below. That reduces the framesize result returned by DSGetCodecInfo() by one */
            }

            if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_G726) {

               bitrate_code = codec_test_params.bitrate;
            }

            if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_MELPE) {

               #ifndef DSGETPAYLOADSIZE
               uFlags |= DS_CODEC_INFO_SIZE_BITS;  /* MELPe framesize specified in bits */
               bitrate_code = codec_test_params.bitrate;
               header_format = codec_test_params.bitDensity;
               #else
               bitrate_code = (codec_test_params.bitrate << 16) | codec_test_params.bitDensity;
               #endif
            }

         /* determine coded data frame size */

            if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_WB_PLUS) {

               int i, break_on_error = 0;
               #ifndef DSGETPAYLOADSIZE
               uFlags |= DS_CODEC_INFO_BITRATE_CODE;
               #endif

               for (i = 0; i < 4; i++) /* read in 4 20ms frames to pass 1 80ms super frame to the decoder */
               {
                  if ((ret_val = _fread(coded_buf + offset, sizeof(char), 2, fp_in)) != 2) { break_on_error = 1; break; }  /* _fread is an fread() wrapper defined in mediaTest.h. See comments there about why in some cases it's needed */
                  offset += 2;
                  bitrate_code = coded_buf[0];
                  #ifndef DSGETPAYLOADSIZE
                  framesize = DSGetCodecInfo(codec_test_params.codec_type, uFlags, bitrate_code, header_format, NULL);
                  #else
                  framesize = DSGetPayloadSize(codec_test_params.codec_type, bitrate_code);
                  #endif
                  if (framesize < 0)
                  {
                     printf("ERROR: Invalid frame size: %d\n", framesize);
                     break;
                  }
                  if ((ret_val = _fread(coded_buf + offset, sizeof(char), framesize, fp_in)) != framesize) {break_on_error = 1; break;}
                  offset += framesize;
                  if (!((bitrate_code >= 10 && bitrate_code <= 13) || bitrate_code > 15)) break; /* if not extension mode, only read 1 20 ms frame */
               }
               if (break_on_error) break;  /* fread() error of some type, break out of while loop */

               coded_framesize = framesize;  /* used in DSCodecDecode() */
            }
            else if (codec_test_params.codec_type != DS_CODEC_TYPE_NONE) {  /* all codecs except for AMR-WB+ */

               #ifndef DSGETPAYLOADSIZE
               framesize = DSGetCodecInfo(codec_test_params.codec_type, uFlags, bitrate_code, header_format, NULL);  /* voplib API to get payload size */
               #else
               framesize = DSGetPayloadSize(codec_test_params.codec_type, bitrate_code);  /* voplib API to get payload size (deprecated if DSGETPAYLOADSIZE not defined, see comments in voplib.h) */
               #endif

               #if 0
               printf("after DSGetCodecInfo(), uFlags = 0x%x, bitrate code = 0x%x, header_format = %d, framesize = %d \n", uFlags, bitrate_code, header_format, framesize);
               #endif

               if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_NB || codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_AMR_WB) {

                  framesize++;  /* account for CMR information in both bandwidth efficient and octet aligned formats. This is somewhat similar to ToC byte used by EVS for coded files, JHB Apr2021 */
                  if (fAMROctetAligned) framesize++;

                  #ifdef CODEC_FILE_DEBUG
                  if (framesize < 0) printf(" encoded input frame size < 0, bitrate code = %d \n", bitrate_code);
                  #endif
               }
            }
            else {  /* codec type = none; i.e. pass-thru of some type like .cod to .pcap, JHB Sep 2022 */

               framesize = coded_framesize;
            }

            if (codec_test_params.uncompress && codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_G729AB) {

               if ((ret_val = fread(coded_buf, sizeof(short), 2, fp_in)) != 2) break;  /* read frame start and frame size bytes from .cod file */
               framesize = ((short int*)coded_buf)[1] * sizeof(short);
               offset = 4;
            }

            if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_G726) {

            /* G726 uncompressed vs. compressed mode notes, JHB Apr2021:
 
               -for uncompress mode we override framesize result returned by DSGetCodecInfo() (see bitrate_code above). Uncompressed mode is not used in RTP packet transport (doing so would make compression pointless)
               -uncompressed mode should be used when comparing with reference vectors/program
               -note that previously we used DSPayloadSize() instead of DSGetCodecInfo(), the former is now deprecated, JHB Oct 2022
            */
  
               if (codec_test_params.uncompress) framesize = codec_frame_duration*8*sizeof(short);
               else framesize *= codec_frame_duration/10;
            }

#ifdef _MELPE_INSTALLED_
            if (codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_MELPE) {

               if (codec_test_params.bitrate == 2400) framesize = melpe_decoder_56bd_pattern[melpe_decoder_pattern_index];  /* next amount of data expected by the decoder (in bytes) */
               else framesize = melpe_decoder_88bd_pattern[melpe_decoder_pattern_index];
               //offset = 0;
            }
#endif

         /* read framesize amount of encoded data (for all codecs except for AMR-WB+) */

     //if (frame_count > 11850) printf("before fread() bitrate_code = %d, framesize = %d \n", bitrate_code, framesize);
 
            if ((codec_test_params.codec_type != DS_VOICE_CODEC_TYPE_AMR_WB_PLUS) && (framesize < 0 || (ret_val = _fread(coded_buf + offset, sizeof(char), framesize, fp_in)) != framesize)) {  /* read from coded file and check for error conditions */

               #ifdef CODEC_FILE_DEBUG
               if (frame_count < 20) {

                  int k;
                  printf(" enc file pos = %d, in buf[] = ", (int)ftell(fp_in));
                  for (k=0; k<framesize; k++) printf("%x ", coded_buf[k]);
                  printf("\n");
               }
               printf(" break after data read, frame size = %d, ret_val = %d \n", framesize, ret_val);
               #endif

               break;  /* no print message here, input is consumed and test finishes. fp_in file read errors are reported below, look for "loop exit condition" */
            }
            
            #if 0  /* offset already set above */
            if (codec_test_params.uncompress && codec_test_params.codec_type == DS_VOICE_CODEC_TYPE_G729AB) {
               framesize += 4; /* add frame header to framesize */
            }
            #endif

            #ifdef CODEC_FILE_DEBUG
            printf(" decode: duration = %d, uncompress = %d, bitrate code = %d, framesize = %d, ret_val = %d \n", (int)codec_frame_duration, codec_test_params.uncompress, bitrate_code, framesize, ret_val);
            #endif

            coded_framesize = framesize + offset;  /* all framesize calculations after reading file payload ToCs/frame headers are finished, set coded bitstream framesize for use in DSCodecDecode() */
         }

         if (!fFramePrint) {

            frame_count++;
            printf("\rProcessing frame %d...", frame_count);
            fflush(stdout);
         }

         if (outFileType != ENCODED && outFileType != PCAP) {

         /* call codec decoder if needed */

            if (decoder_handle[0]) {

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

               if (coded_framesize >= 0 && !uStripFrame) {  /* allow zero framesizes and let codec decide whether to process. This can be the case with DTX ("NO_DATA" frames), JHB Nov 2023 */

                  if (nCut > 0) { nCut--; goto loop; }  /* handle --cut if entered on cmd line. This can probably be moved up, but safe enough here, JHB Nov 2023 */

                  CODEC_OUTARGS CodecOutArgs = { 0 };
                  CODEC_INARGS* pInArgs = NULL;

                  //#define DECODER_USE_INARGS   /* define to test DEBUG_OUTPUT_xxx flags and/or on-the-fly decoder param updates, if supported by the codec type */
                  #ifdef DECODER_USE_INARGS

                  CODEC_INARGS CodecInArgs = { 0 };  /* create a CODEC_INARGS struct, init to zero */
                  CODEC_DEC_PARAMS CodecDecParams = { 0 };  /* create a CODEC_DEC_PARAMS struct, init to zero */
                  CodecInArgs.pCodecDecParams = &CodecDecParams;
                  #if 0
                  CodecInArgs.pCodecDecParams->uFlags |= DEBUG_OUTPUT_VOPLIB_SHOW_BITSTREAM_BYTES;  /* show decoder bitstream bytes on entry */
                  #endif
                  pInArgs = &CodecInArgs;

                  #endif  /* DECODER_USE_INARGS */

     //if (frame_count > 11850) printf("before decode, bitrate_code = %d, coded_framesize = %d \n", bitrate_code, coded_framesize);
 
                  len = DSCodecDecode(decoder_handle, 0, coded_buf, out_buf, coded_framesize, numChan, pInArgs, &CodecOutArgs);  /* call voplib codec decode API */

                  #if 0  /* act on decoder outputs in CODEC_OUTARGS struct */
                  printf(" docoder detected bitrate = %d \n", CodecOutArgs.bitRate);
                  #endif

                  if (len < 0) {
                     fprintf(stderr, "DSCodecDecode() returns error %d, exiting test \n", len);
                     goto codec_test_cleanup;
                  }

                  len *= numChan;  /* adjust for multichannel data before writing out to wav file or USB audio, JHB Feb2022 */
               }
               else len = 0;
            }
            else {  /* pass-thru (codec_type == NONE) */

               len = inbuf_size*numChan;
               memcpy(out_buf, in_buf, len);
            }

            addr = out_buf;
         }
         else {  /* audio file format, USB output, or pcap */

            len = coded_framesize;
            addr = coded_buf;
         }

         if (outFileType != USB_AUDIO) {

            #ifndef _NO_PKTLIB_  /* JHB Aug 2022 */

            if (outFileType == PCAP) {

               format_pkt.rtpHeader.Sequence = seq_num++;
               format_pkt.rtpHeader.SSRC = SSRC;
               format_pkt.rtpHeader.Timestamp = timestamp;
               timestamp += 320;  /* hardcoded for now to wideband audio, need to set this based on codec type, JHB Jan2021 */

               if (nMarkerBit >= 0) {
                  #if 0
                  if (nMarkerBit) DSSetMarkerBit(&format_pkt, uFlags_format_pkt);
                  else DSClearMarkerBit(&format_pkt, uFlags_format_pkt);
                  #else
                  format_pkt.rtpHeader.Marker = nMarkerBit ? 1 : 0;
                  #endif
                  nMarkerBit--;
               }

            /* format packet including encoded data as payload, results are in pkt_buf */

               pkt_len = DSFormatPacket(-1, uFlags_format_pkt, addr, len, &format_pkt, pkt_buf);

               if (pkt_len <= 0) {
                  fprintf(stderr, "ERROR: DSFormatPacket() returns %d error code \n", pkt_len);
                  goto codec_test_cleanup;
               }

               #if 0
               ts_pcap.tv_sec = nsec_pcap / 1000000000L;
               ts_pcap.tv_nsec = nsec_pcap % 1000000000L;
               #else
               pcaprec_hdr_t pcaprec_hdr = { 0 };
               pcaprec_hdr.ts_sec = nsec_pcap / 1000000000L;
               pcaprec_hdr.ts_usec = (nsec_pcap % 1000000000L)/1000;
               #endif
               nsec_pcap += 20000000L;  /* increment by 20 msec */

               #if 0
               if ((ret_val = DSWritePcap(fp_out, 0, pkt_buf, pkt_len, NULL, NULL, &term_info, &ts_pcap)) < 0) {
               #else  /* remove struct timespec* param (timestamp in pcap_pkt_hdr is used instead), and remove TERMINATION_INFO* (IP type in pkt_buf is used instead), JHB Jul 2024 */
               if ((ret_val = DSWritePcap(fp_out, 0, pkt_buf, pkt_len, &pcaprec_hdr, NULL, NULL)) < 0) {
               #endif
                  fprintf(stderr, "ERROR: DSWritePcap() returns %d error code \n", ret_val);
                  goto codec_test_cleanup;
               }
            }
            else
            #endif  /* ifndef _NO_PKTLIB_ */
            {  /* write out data for all file formats except for pcap, using pointer to bytes (addr) and number of bytes (len) */

               #ifdef CODEC_FILE_DEBUG
               if (outFileType == ENCODED && frame_count < 20) {
  
                  printf(" out file pos = %d, len = %d", (int)ftell(fp_out), len);
                  if (len > 0) {
                     int k;
                     printf(", out buf[] = ");
                     for (k=0; k<len; k++) printf("%.2x ", coded_buf[k]);
                  }
                  printf("\n");
               }
               #endif
 #if 0
 static bool fOnce2 = false;
 if (!fOnce2 && (outFileType == ENCODED || outFileType == PCAP)) { printf(" before DSSaveDataFile, inbuf_size = %d, len = %d, output data =", inbuf_size, len); for (i=0; i<len; i++) printf(" 0x%x", addr[i]); printf("\n"); fOnce2 = true; }
 #endif
 
               ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, NULL, (uintptr_t)addr, len, DS_WRITE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfo);   /* DSSaveDataFile() returns bytes written */

               if (ret_val != len) {
                  printf("Error writing output wav file frame %d: tried to write %d bytes, wrote %d bytes\n", frame_count, len, ret_val);
                  goto codec_test_cleanup;
               }
            }
         }

         if (IS_AUDIO_FILE_TYPE(outFileType)) {  /* segmented audio currently requires an audio output cmd line spec, but maybe in the future that's not the case, for example to strip silence before encoding to pcap, JHB Jan2021 */

         /* write out audio file segments, if specified in cmd line. Use fixed or adjusted segment intervals, as specified by flags */

            if (nSegmentation & DS_SEGMENT_AUDIO) {
 
               if (segmenter(SEGMENTER_FRAME, frame_count, codec_frame_duration, uStripFrame, in_buf, inbuf_size*numChan, &fp_out_segment, &MediaInfoSegment, &fp_out_concat, &MediaInfoConcat, &fp_out_stripped, &MediaInfoStripped) < 0) goto codec_test_cleanup;
            }
         }

         if (outFileType & USB_AUDIO) {

            if (codec_test_params.codec_type != DS_CODEC_TYPE_NONE && upFactor_output != downFactor_output) {

               int num_samples = len/numChan/AUDIO_SAMPLE_SIZE;

               for (i=0; i<numChan; i++) {

                  DSConvertFs(&((short int*)addr)[i],          /* pointer to data */
                              sampleRate_output,               /* sampling rate of data, in Hz */
                              upFactor_output,                 /* up factor */    
                              downFactor_output,               /* down factor */
                              fs_convert_delay_buf_output[i],  /* pointer to delay values (this buffer has to be preserved between calls to DSConvertFs() so it must be per channel */
                              num_samples,                     /* data length, in samples */
                              numChan,                         /* number of interleaved channels in the input data */
                              NULL,                            /* user-defined filter N/A */
                              0,                               /* length of user-defined filter N/A */
                              DS_FSCONV_SATURATE);             /* apply saturate flag to ensure no wrapping in audio output, JHB Feb 2024 */
               }
            }
         }

      }  /* while loop */

      printf("\n");  /* leave existing status line, including any error messages (don't clear it) */

      if (!pm_run) printf("Exiting test\n");

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

      if (!USBAudioInput && pm_run && fp_in && !feof(fp_in))
      {
         printf("Error -- did not reach input file EOF, last fread() read %d bytes\n", ret_val);
      }

codec_test_cleanup:

   /* codec tear down / cleanup */

      unsigned int uFlags = (debugMode & ENABLE_MEM_STATS) ? DS_CODEC_TRACK_MEM_USAGE : 0;

      for (i=0; i<numChan; i++) {
         if (encoder_handle[i] > 0) DSCodecDelete(encoder_handle[i], uFlags);
         if (decoder_handle[i] > 0) DSCodecDelete(decoder_handle[i], uFlags);
      }

      if (fp_in) {
         if (inFileType != ENCODED) DSLoadDataFile(DS_GM_HOST_MEM, &fp_in, NULL, (uintptr_t)NULL, 0, DS_CLOSE | DS_DATAFILE_USE_SEMAPHORE, NULL, &hFile_in);
         else fclose(fp_in);
      }

      if (fp_out) {
         if (outFileType == PCAP) DSClosePcap(fp_out, 0);
         else DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, NULL, (uintptr_t)NULL, 0, DS_CLOSE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfo);
      }

      if (fp_out_segment) DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_segment, NULL, (uintptr_t)NULL, 0, DS_CLOSE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfoSegment);
      if (fp_out_concat) DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_concat, NULL, (uintptr_t)NULL, 0, DS_CLOSE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfoConcat);
      if (fp_out_stripped) DSSaveDataFile(DS_GM_HOST_MEM, &fp_out_stripped, NULL, (uintptr_t)NULL, 0, DS_CLOSE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfoStripped);

#if defined(_ALSA_INSTALLED_) && defined(ENABLE_USBAUDIO)
      if (usb_device_capture) DSCloseAvioDevice(usb_device_capture, pcm_callback_capture);
      if (usb_device_playback) DSCloseAvioDevice(usb_device_playback, pcm_callback_playback);
#endif

      if (fShow_md5sum) {  /* display md5 stats if --md5sum entered on cmd line (fShow_md5sum is in cmd_line_interface.c), JHB Feb 2024 */

         char md5str[2*CMDOPT_MAX_INPUT_LEN] = "";

         if (fp_out && DSGetMD5Sum(MediaInfo.szFilename, md5str, sizeof(md5str)-1) == 1 && strlen(md5str) > 0) printf("md5sum %s %s \n", md5str, MediaInfo.szFilename);  /* get MD5 sum from diaglib API */
      }

      __sync_fetch_and_add(&nProcessClose, 1);

      if (hPlatform != -1) {

         while (__sync_fetch_and_add(&nProcessClose, 0) != max(num_app_threads, 1));  /* wait for any other threads still working to get here */

         DSFreePlatform((intptr_t)hPlatform);  /* free DirectCore platform handle */
      }

      if (fInitAdvancedLogging) DSCloseLogging(0);

      sprintf(tmpstr, "x86 codec test end");
      if (num_app_threads > 0) sprintf(&tmpstr[strlen(tmpstr)], " thread = %d", thread_index);
      printf("%s \n", tmpstr);
   }
   #if 0  /* JHB Feb 2024 */
   else if (x86_pkt_test || frame_mode) {
   #else
   else if (!fProcessEntry) {
   #endif

      int num_threads = PlatformParams.cimInfo[0].taskAssignmentCoreLists[0] | (PlatformParams.cimInfo[0].taskAssignmentCoreLists[1] << 8) | (PlatformParams.cimInfo[0].taskAssignmentCoreLists[2] << 16) | (PlatformParams.cimInfo[0].taskAssignmentCoreLists[3] << 24);  /* this is the -tN cmd line value, if entered.  -1 means there was no entry, JHB Sep 2018 */

      if (num_threads == -1) {

         PlatformParams.cimInfo[0].taskAssignmentCoreLists[0] = 0;
         PlatformParams.cimInfo[0].taskAssignmentCoreLists[1] = 0;
         PlatformParams.cimInfo[0].taskAssignmentCoreLists[2] = 0;
         PlatformParams.cimInfo[0].taskAssignmentCoreLists[3] = 0;
      }

      #define THREAD_ARG_SIZE 512

      switch (executeMode[0]) {

         case 'a':  /* app execution mode - default setting in cmdLineInterface() (cmd_line_interface.c) */

            {
               printf("x86 app test start \n");

               uint32_t* arg = (uint32_t*)calloc(1, THREAD_ARG_SIZE);

               if (arg == NULL) 
               {
                  fprintf(stderr, "%s:%d: Could not allocate memory for mediaMin thread arg \n", __FILE__, __LINE__);
                  exit(-1);
               }

               ((char*)arg)[0] = (char)executeMode[0];

               packet_flow_media_proc((void*)arg);  /* packet data flow and media processing.  packet_flow_media_proc.c */

               free(arg);
            }

            break;

         case 'p':  /* process execution, not used yet */

            printf("x86 process test start \n");

            break;

         case 't':  /* thread execution, DSPush/PullPackets APIs used to interface with mediaMin and packet_flow_media_proc() running as threads */

#ifdef _NO_MEDIAMIN_
            printf("Attempting to call mediaMin_thread() but build had mediaMin disabled (i.e. make cmd line with no_mediamin=1) \n");
#else

            printf("x86 mediaMin multithread test start, num threads = %d \n", num_threads);

            if (num_threads <= 0) {

               int thread_index = 0;

               mediaMin_thread((void*)&thread_index);  /* mediaMin_thread() (in mediaMin.cpp) starts one or more packet/media threads and uses packet push/pull queues. We call it here as a function */
            }
            else {  /* in this case we start mediaMin_thread() as one or more application level threads */

               uint32_t* arg;
               int i, tc_ret, num_threads_started = 0;
               pthread_attr_t* ptr_attr = NULL;
               pthread_t mediaMinThreads[MAX_APP_THREADS];  /* MAX_APP_THREADS defined in mediaTest.h */

               for (i=0; i<num_threads; i++) {

                  arg = (uint32_t*)calloc(1, THREAD_ARG_SIZE);

                  if (arg == NULL) 
                  {
                     fprintf(stderr, "%s:%d: Could not allocate memory for mediaMin thread arg \n", __FILE__, __LINE__);
                     exit(-1);
                  }

                  *arg = (num_threads << 8) | i;  /* tell mediaMin app thread which one it is and how many total threads */

                  DSGetBacktrace(4, DS_GETBACKTRACE_INSERT_MARKER, &((char*)arg)[4]);  /* get backtrace info before thread starts, then thread also gets its own backtrace info, JHB May 2024 */

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

               free(arg);
            }

#endif  /* ifndef _NO_MEDIAMIN_ */

            break;

         case 'c':  /* command line execution. Default is codec multi-thread testing, example: ./mediaTest -cx86 -itest_files/T_mode.wav -otest_files/T_mode_48kHz_5900.wav -Csession_config/evs_48kHz_input_16kHz_5900bps_full_band_config -Ec -t2. At some later points we might add -Ecx, where x is some other type of command-line mode */

            printf("x86 multithread test start, num threads = %d \n", num_threads);

            if (num_threads > 0) {  /* run mediaTest_proc() as one or more application level threads */


               uint32_t* arg;
               int i, tc_ret, num_threads_started = 0;
               pthread_attr_t* ptr_attr = NULL;
               pthread_t mediaTestThreads[MAX_APP_THREADS];  /* MAX_APP_THREADS defined in mediaTest.h */

               for (i=0; i<num_threads; i++) {
      
                  arg = (uint32_t*)calloc(1, THREAD_ARG_SIZE);

                  if (arg == NULL) 
                  {
                     fprintf(stderr, "%s:%d: Could not allocate memory for mediaTest thread arg\n", __FILE__, __LINE__);
                     exit(-1);
                  }

                  *arg = (num_threads << 8) | i;  /* tell mediaTest app thread which one it is and how many total threads */

                  DSGetBacktrace(4, DS_GETBACKTRACE_INSERT_MARKER, &((char*)arg)[4]);  /* get backtrace info before thread starts, then thread also gets its own backtrace info, JHB May 2024 */

                  if ((tc_ret = pthread_create(&mediaTestThreads[i], ptr_attr, mediaTest_proc, arg))) fprintf(stderr, "%s:%d: pthread_create() failed for mediaTest thread, thread number = %d, ret val = %d\n", __FILE__, __LINE__, i, tc_ret);
                  else {

                     cpu_set_t cpuset;
                     CPU_ZERO(&cpuset);

                     // #define PIN_TO_SAME_CORE  /* define if needed for debug */
                     #ifndef PIN_TO_SAME_CORE
                     int j;
                     if (nReuseInputs) for (j=10; j<16; j++) {
                        CPU_SET(j, &cpuset);
                        CPU_SET(j+16, &cpuset);
                     }
                     else for (j=6; j<32; j++) if (j < 16 || j > 18) CPU_SET(j, &cpuset);  /* avoid first 6 physical cores (and their logical core companions) */
                     #else
                     CPU_SET(6, &cpuset);
                     #endif

                     tc_ret = pthread_setaffinity_np(mediaTestThreads[i], sizeof(cpu_set_t), &cpuset);

                     num_threads_started++;
                  }
               }

               if (num_threads_started) {  /* wait here for all threads to exit */

                  for (i=0; i<num_threads_started; i++) pthread_join(mediaTestThreads[i], NULL);
               }

               free(arg);
            }

            break;
      }
   }
   else if (x86_frame_test)
   {
      printf("x86 frame test start \n");

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

      if (strlen(MediaParams[0].configFilename) == 0 || (access(MediaParams[0].configFilename, F_OK ) == -1)) 
      {
         printf("Specified config file: %s does not exist, using default file.\n", MediaParams[0].configFilename);
         config_file = default_config_file;
      }
      else config_file = MediaParams[0].configFilename;

      printf("Opening session config file: %s\n", config_file);

      fp_cfg = fopen(config_file, "r");

      while (parse_codec_config_frame_mode(fp_cfg, &ft_info) != -1) {

         unsigned int uFlags = DS_CODEC_CREATE_ENCODER | DS_CODEC_CREATE_DECODER | DS_CODEC_CREATE_USE_TERMINFO;
         uFlags |= (debugMode & ENABLE_MEM_STATS) ? DS_CODEC_TRACK_MEM_USAGE : 0;

         if ((hCodec[nCodecs] = DSCodecCreate(&ft_info.term, uFlags)) < 0)
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
         return (void*)-1;
      }

      for (i=0; i<nCodecs; i++)
      {
         if (encoder_input_files[i] != NULL)
         {
            if ((ret_val = pthread_create(&process_threads[2*i], NULL, encode_thread_task, &hCodec[i]))) 
            {
               fprintf(stderr, "%s:%d: pthread_create() failed for codec number %d, returned %d\n", __FILE__, __LINE__, i, ret_val);
               return (void*)-1;
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
               return (void*)-1;
            }
            else
            {
               nThreads++;
               thread_status[2*i+1] = 1;
            }
         }
      }

      printf("Waiting for %d processing threads to complete...\n", nThreads);
      while (pm_run && !threads_finished)
      {
         threads_finished = 1;
         for (i=0; i<2*nCodecs; i++) 
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

      for (i=0; i<nCodecs; i++)
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

   /* cleanup */

      unsigned int uFlags = (debugMode & ENABLE_MEM_STATS) ? DS_CODEC_TRACK_MEM_USAGE : 0;

      for (i=0; i<nCodecs; i++) {

         DSCodecDelete(hCodec[i], uFlags);

         if (encoder_input_files[i] != NULL) free(encoder_input_files[i]);
         if (decoder_input_files[i] != NULL) free(decoder_input_files[i]);
      }

      fclose(fp_cfg);
  
      printf("x86 frame test end\n");
   }
   else if (pcap_extract) {

#ifdef _NO_PKTLIB_  /* JHB Aug 2022 */
      printf("Attempting to call pktlib() functions but build had pktlib disabled (i.e. make cmd line with no_pktlib=1) \n");
#else

   /* pcap extract mode (i) extracts RTP payloads from pcap files and writes to 3GPP decoder compatible .cod files or (ii) copies between pcap files, applying operations to RTP media contents. Notes:

      1) 3GPP decoders typically support MIME and G.192 file formats. MIME format normally expects consecutive RTP payloads in FH (Header-Full) format, each including a leading ToC byte

      2) Currently the pcap extract mode only supports MIME format. If pcap RTP payloads are in CH (Compact Header) format, they are converted to FH (header-full) format by adding a ToC payload header byte

      3) Major overhaul to support pacp-to-pcap operations, JHB Jul 2024
   */

      MEDIAINFO MediaInfo = {0};

      FILE *fp_in = NULL, *fp_out = NULL;

      int ret_val, frame_count = 0, rtcp_packet_count = 0;

      uint8_t pkt_buffer[MAX_RTP_PACKET_LEN];

      int packet_length, link_layer_info, rtp_pyld_len;
      uint8_t rtp_pyld_type;
      uint8_t *rtp_pyld_ptr;
      unsigned int uFlags;
      pcap_hdr_t pcap_file_hdr = { 0 };
      pcaprec_hdr_t pcap_pkt_hdr = { 0 };

   /* define STRIP_SID to remove SID frames from .cod output */
      // #define STRIP_SID
      #ifdef STRIP_SID
      int pyld_datatype, SID_drop_count = 0;
      #endif

   /* define LIST_TOCS to track unique ToC values found (displayed after pcap extraction finishes). ToC values are one or more "table of contents" bytes in the RTP payload header, as defined in the EVS spec */
      #define LIST_TOCS
      #ifdef LIST_TOCS
      #define MAX_TOCS 256
      int i, num_tocs = 0, sav_tocs[MAX_TOCS];
      #endif

      printf("pcap extract start \n");

      memset(sav_tocs, 0xff, sizeof(sav_tocs));  /* set all sav_toc[] to 0xff */

      #pragma GCC diagnostic push  /* suppress "address of var will never be NULL" warnings in gcc 12.2; safe-coding rules prevail, JHB May 2023 */
      #pragma GCC diagnostic ignored "-Waddress"
      if (MediaParams[0].Media.inputFilename != NULL) {
      #pragma GCC diagnostic pop

         #if 0
         strcpy(tmpstr, MediaParams[0].Media.inputFilename);
         strupr(tmpstr);

         if (strstr(tmpstr, ".PCAP"))
         #else
         if (inFileType == PCAP)
         #endif
         {
            if ((link_layer_info = DSOpenPcap(MediaParams[0].Media.inputFilename, DS_READ | DS_OPEN_PCAP_FILE_HDR_PCAP_FORMAT, &fp_in, &pcap_file_hdr, "")) < 0) goto pcap_extract_cleanup;  /* open pcap for reading, retrieve file header info in legacy libpcap format (regardless of whether file is pcap or pcapng) */
            #if 0
            printf("\n *** after pcap open, link type = %d, file_type = %d, link_layer len = %d, file hdr link type = %u \n", link_layer_info >> 20, (link_layer_info >> 16) & 0xf, link_layer_info & 0xffff, pcap_file_hdr.link_type);
            #endif
         }
         else {
            fprintf(stderr, "Input file %s is not a pcap file as required in pcap extract mode \n", MediaParams[0].Media.inputFilename);
            goto pcap_extract_cleanup;
         }
      }
      else {
         fprintf(stderr, "No input file given\n");
         goto pcap_extract_cleanup;
      }

      #pragma GCC diagnostic push  /* suppress "address of var will never be NULL" warnings in gcc 12.2; safe-coding rules prevail, JHB May 2023 */
      #pragma GCC diagnostic ignored "-Waddress"
      if (MediaParams[0].Media.outputFilename != NULL) {
      #pragma GCC diagnostic pop

         #if 0
         strcpy(tmpstr, MediaParams[0].Media.outputFilename);
         strupr(tmpstr);

         if (strstr(tmpstr, ".COD") || strstr(tmpstr, ".AMR") || strstr(tmpstr, ".AWB") || strstr(tmpstr, ".BIT"))
         #else
         if (outFileType == ENCODED)
         #endif
         {
            strcpy(MediaInfo.szFilename, MediaParams[0].Media.outputFilename);

            MediaInfo.CompressionCode = DS_GWH_CC_EVS;  /* default */
            int codec_type = 0;  /* not sure yet how to determine codec type -- payload contents can't be used without a session config file, so maybe something on the command line, or maybe make mediaMin codec detection heuristic into a callable function ... */

            if (strcasestr(MediaInfo.szFilename, ".awb") || codec_type == DS_VOICE_CODEC_TYPE_AMR_WB) MediaInfo.CompressionCode = DS_GWH_CC_GSM_AMRWB;
            else if (strcasestr(MediaInfo.szFilename, ".amr") || codec_type == DS_VOICE_CODEC_TYPE_AMR_NB) MediaInfo.CompressionCode = DS_GWH_CC_GSM_AMR;
            else if (codec_type == DS_VOICE_CODEC_TYPE_EVS) MediaInfo.CompressionCode = DS_GWH_CC_EVS;  /* EVS uses .cod extension */
            else if (codec_type == DS_VOICE_CODEC_TYPE_MELPE) MediaInfo.CompressionCode = DS_GWH_CC_MELPE;  /* no file extension defined for MELPe */

            ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, MediaParams[0].Media.outputFilename, (uintptr_t)NULL, 0, DS_CREATE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfo);  /* DSSaveDataFile returns bytes written, with DS_CREATE flag it returns header length (if any, depending on file type) */

            if (fp_out == NULL) {
               fprintf(stderr, "Failed to open coded bitstream output file: %s, ret_val = %d \n", MediaParams[0].Media.outputFilename, ret_val);
               goto pcap_extract_cleanup;
            }
            else printf("Opened coded output file: %s\n", MediaParams[0].Media.outputFilename);
         }
         else if (outFileType == PCAP) {  /* add pcap output, JHB Jul 2024 */

            if ((ret_val = DSOpenPcap(MediaParams[0].Media.outputFilename, DS_WRITE, &fp_out, &pcap_file_hdr, "")) < 0) {  /* open write pcap with file header from read pcap */

               fprintf(stderr, "Failed to open pcap output file: %s, ret_val = %d \n", MediaParams[0].Media.outputFilename, ret_val);  /* DSOpenPcap() will display an error message so displaying another one not necessary */
               goto pcap_extract_cleanup;
            }
         }
         else {
            fprintf(stderr, "ERROR: output file %s is not an encoded bitstream or pcap format file \n", MediaParams[0].Media.outputFilename);
            goto pcap_extract_cleanup;
         }
      }
      else {
         fprintf(stderr, "No output file given \n");
         goto pcap_extract_cleanup;
      }

      #ifdef USE_OLD_NETWORK_BYTE_ORDER_FLAG
      uFlags = DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_NETWORK_BYTE_ORDER;  /* used with DSGetPacketInfo() */
      #else
      uFlags = DS_BUFFER_PKT_IP_PACKET;  /* used with DSGetPacketInfo() */
      #endif

   /* open pcap file and read its header, initialize link layer offset */

      while (pm_run) {

         char key = toupper(getkey());
         if (key == 'Q') {  /* on quit key break out of while(run) loop */
            pm_run = 0;
            break;
         }

      /* read next input pcap or pcapng packet */

         uint16_t eth_hdr_type;

         if (!(packet_length = DSReadPcap(fp_in, 0, pkt_buffer, &pcap_pkt_hdr, link_layer_info, &eth_hdr_type, NULL))) break;  /* read pcap, save packet header in pcap_pkt_hdr in case needed for DSPcapWrite(). On end of file, break out of main while loop, JHB Jul 2024 */

         frame_count++;
         if (outFileType == ENCODED) printf("\rExtracting pcap payload %d", frame_count);
         else if (outFileType == PCAP) printf("\rOperating on pcap payload %d", frame_count);

         #ifdef STRIP_SID  /* typically not defined unless for debug purposes */
         pyld_datatype = DSGetPacketInfo(-1, uFlags | DS_PKT_INFO_RTP_PYLDDATATYPE, pkt_buffer, packet_length, NULL, NULL);

         if (pyld_datatype == DS_PKT_PYLDDATATYPE_SID) {

            SID_drop_count++;
            continue;  /* skip SID (DTX) frames which the 3GPP decoder does not handle when the .cod file contains MIME format data */
         }
         #endif

         rtp_pyld_ptr = pkt_buffer + DSGetPacketInfo(-1, uFlags | DS_PKT_INFO_RTP_PYLDOFS, pkt_buffer, -1, NULL, NULL);
         rtp_pyld_len = DSGetPacketInfo(-1, uFlags | DS_PKT_INFO_RTP_PYLDLEN, pkt_buffer, -1, NULL, NULL);
         rtp_pyld_type = DSGetPacketInfo(-1, uFlags | DS_PKT_INFO_RTP_PYLDTYPE, pkt_buffer, -1, NULL, NULL);  /* payload type used to filter RTCP packets, JHB Jul 2024 */

         uint32_t bitrate = 0;
         uint8_t category = 0;
         int codec_type = detect_codec_type_and_bitrate(rtp_pyld_ptr, rtp_pyld_len, 0, rtp_pyld_type, 0, &bitrate, NULL, &category);
         #if 0  /* debug with evs_interop.sh */
         if (codec_type != DS_VOICE_CODEC_TYPE_EVS && codec_type != -1) printf("\n *** found non-EVS codec type = %d \n", codec_type);
         #endif
         if (codec_type == -1 && rtp_pyld_len == 2) codec_type = DS_VOICE_CODEC_TYPE_EVS;  /* not sure if DSGetPayloadXXX APIs can handle AMR NO_DATA payloads, they can for EVS */

         //#define CODEC_TYPE_DEBUG
         #ifdef CODEC_TYPE_DEBUG
         static int count = 0;
         #endif

      /* packet type filtering. Notes:

         -ignore RTCP packets for bitstream coded output
         -many other types of non-RTP packet filtering should be added. PushPackets() in mediaMin.cpp does this
      */

         if (rtp_pyld_type >= RTCP_PYLD_TYPE_MIN && rtp_pyld_type <= RTCP_PYLD_TYPE_MAX) {

            rtcp_packet_count++;

            if (outFileType == PCAP) goto pcap_out;     /* for pcap output, write RTCP packets but apply no operations */
            else if (outFileType == ENCODED) continue;  /* for bitstream coded output, strip / ignore RTCP packets */
         }

      /* determine header format */

         #if 0
         bool fAMRWB_IO_Mode, fSID;  /* currently not used, but available if needed */
         uint8_t CMR, ToC;           /* payload header CMR and ToC (table of contents) */
         #else
         PAYLOAD_INFO PayloadInfo;
         #endif
         bool fTocInPyld;

         fTocInPyld = true;  /* default until found otherwise */
         memset(&PayloadInfo, 0, sizeof(PayloadInfo));

         if (!DSGetPayloadInfo(codec_type, DS_CODEC_INFO_TYPE, rtp_pyld_ptr, rtp_pyld_len, &PayloadInfo)) {  /* returns zero if compact header or bandwidth efficient format; fills in AMR-WB IO mode, SID, CMR, and ToC */

            if (codec_type == DS_VOICE_CODEC_TYPE_EVS) {

               PayloadInfo.ToC = DSGetPayloadHeaderToC(codec_type, rtp_pyld_len);  /* generate a ToC byte based on payload size (convert to FH format) */
               fTocInPyld = false;

               if (outFileType == ENCODED) {

                  ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, NULL, (uintptr_t)&PayloadInfo.ToC, 1, DS_WRITE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfo);  /* write ToC byte (DSSaveDataFile returns bytes written) */

                  if (ret_val != 1) {
                     printf("Error writing ToC byte for frame %d, return val = %d \n", frame_count, ret_val);
                     goto pcap_extract_cleanup;
                  }
               }
            }
         }
         else {  /* full header format */

            #if 0
            if (rtp_pyld_ptr[0] & 0x80)  /* check for CMR byte */
            #else  /* make use of CMR returned from DSGetPayloadInfo(), JHB Jul 2024 */
            if (outFileType == ENCODED && PayloadInfo.CMR)  /* if CMR byte present, don't include in MIME output coded bitstream frame write to file. ToC is included */
            #endif
            {
               rtp_pyld_ptr++;
               rtp_pyld_len--;
            }
         }

         #ifdef LIST_TOCS
         {
            bool found = false;
            for (i=0; i<num_tocs; i++) {

               if (PayloadInfo.ToC == sav_tocs[i]) {
                  found = true;
                  break;
               }
            }
            if (!found) { sav_tocs[min(num_tocs, MAX_TOCS-1)] = PayloadInfo.ToC; num_tocs++; }  /* if not found then add this ToC value to saved list */
         }
         #endif

      /* check for payload/packet impairment and filtering operations, JHB Jul 2024 */

      // #define ERROR_DISTRIBUTION_DEBUG  /* error distribution debug */
      
         if (nRandomBitErrorPercentage > 0) {  /* if random bit error percentage specified on command line, insert random bit errors in the payload */

            int pyld_hdr_ofs = (PayloadInfo.CMR ? 1 : 0) + (fTocInPyld ? 1 : 0);  /* not quite right for AMR bandwidth efficient, but won't matter */
            float num_bit_errors = 1.0*(rtp_pyld_len-pyld_hdr_ofs)*8*nRandomBitErrorPercentage/100;

            #ifdef CODEC_TYPE_DEBUG
            char tocstr[20];
            sprintf(tocstr, "0x%x", ToC);
            if (codec_type != DS_VOICE_CODEC_TYPE_EVS) { count++; printf("\n *** codec type = %d, bitrate = %d, count = %d, rtp_pyld_len = %d, CMR = %d, rtp pyld[0] = 0x%x, cat = %d, ToC = %s, num_bit_errors = %2.4f \n", codec_type, bitrate, count, rtp_pyld_len, PayloadInfo.CMR, rtp_pyld_ptr[0], category, fTocInPyld ? tocstr : "none", num_bit_errors); }
            #endif

            #ifdef ERROR_DISTRIBUTION_DEBUG
            int min_bit_pos = 32767;
            int max_bit_pos = 0;
            #endif

            for (i=0; i<(int)num_bit_errors; i++) {

               int bit_pos = 1.0*rand()/RAND_MAX*(rtp_pyld_len-pyld_hdr_ofs)*8;

               rtp_pyld_ptr[max(1, pyld_hdr_ofs) + bit_pos/8] ^= 1 << (bit_pos & 7);  /* flip bit specified by bit_pos. For full header, start at location immediately after ToC, for compact header don't mess with first byte otherwise we might accidentally create a bogus CMR byte, which would cause errors in codec decoder */

               #ifdef ERROR_DISTRIBUTION_DEBUG
               max_bit_pos = max(bit_pos, max_bit_pos);
               min_bit_pos = min(bit_pos, min_bit_pos);
               #endif
            }

            #ifdef ERROR_DISTRIBUTION_DEBUG
            static int count = 0;
            int count_prev = count;
            if (rtp_pyld_len == 63 || rtp_pyld_len == 61) {
               if (count < 50) count++;
            }
            else count++;
            if (count_prev != count && count < 500) printf("\n *** min_bit_pos = %d, max_bit_pos = %d, pyld len = %d \n", min_bit_pos, max_bit_pos, rtp_pyld_len-pyld_hdr_ofs);
            #endif
         }

         if (outFileType == ENCODED) {

            ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, NULL, (uintptr_t)rtp_pyld_ptr, rtp_pyld_len, DS_WRITE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfo);

            if (ret_val != rtp_pyld_len) {
               printf("Error writing encoded bitstream output frame %d, wrote %d bytes\n", frame_count, ret_val);
               goto pcap_extract_cleanup;
            }
         }
         else if (outFileType == PCAP) {  /* add pcap write, JHB Jul 2024 */
pcap_out:
            ret_val = DSWritePcap(fp_out, 0, pkt_buffer, packet_length, &pcap_pkt_hdr, NULL, &pcap_file_hdr);
            if (ret_val < 0) { fprintf(stderr, "pcap extract mode DSWritePcap() failed, ret_val = %d \n", ret_val); goto pcap_extract_cleanup; }
         }
      }

      if (pm_run && fp_in && !feof(fp_in)) fprintf(stderr, "Error while reading input pcap file \n");

pcap_extract_cleanup:  /* added single exit point for success + most errors, JHB Jun 2017 */

      #ifdef STRIP_SID
      frame_count = max(0, frame_count - SID_drop_count);
      #endif
      if (outFileType == ENCODED) printf("\nExtracted %d pcap payloads", frame_count);
      else if (outFileType == PCAP)  printf("\nOperated on %d pcap payloads", frame_count);
      #ifdef STRIP_SID
      if (SID_drop_count > 0) printf(", not including %d SID frames (which are not supported in .cod file MIME format)", SID_drop_count);
      #endif
      printf(" \n");

      #ifdef LIST_TOCS
      printf("Unique ToC values found: ");
      for (i=0; i<num_tocs; i++) printf("0x%x ", sav_tocs[i]);
      printf("\n");
      #endif
      printf("RTCP packets found: %d \n", rtcp_packet_count);

      if (fp_in) fclose(fp_in);

      if (fp_out) {
         if (outFileType == ENCODED) DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, NULL, (uintptr_t)NULL, 0, DS_CLOSE | DS_DATAFILE_USE_SEMAPHORE, &MediaInfo);
         else if (outFileType == PCAP) DSClosePcap(fp_out, 0);
      }

      printf("pcap extract end \n");

#endif  /* ifndef _NO_PKTLIB_ */
   }
   else if (gpx_process) {

      printf("gpx test start \n");
   
      FILE *fp_in = NULL, *fp_out = NULL;
      char key;
      int i, j;

  #define N_LOOKBACK 16

      GPX_POINT gpx_points_in_buffer[N_LOOKBACK + NUM_GPX_POINTS_PER_FRAME] = { 0 };
      GPX_POINT gpx_points_out_buffer[N_LOOKBACK + NUM_GPX_POINTS_PER_FRAME] = { 0 };

      GPX_POINT* gpx_points_in = &gpx_points_in_buffer[N_LOOKBACK];
      GPX_POINT* gpx_points_out = &gpx_points_out_buffer[N_LOOKBACK];

      #define N_RUN 4  /* must always be power of 2 */
	   float run_sum_save_d[N_RUN] = { 0 };  /* distance running sum */
	   float run_sum_save_h[N_RUN] = { 0 };  /* heading (bearing) running sum */
      float run_sum_d = 0, run_sum_h = 0;
      int run_sum_index = 0;
      int alt_filt_count = 0;

      int aggressive_count = 0, relax_count = 0, loop_fix_count = 0, alt_dev_count = 0, drop_out_count = 0;  /* stat counters */

      const char header1[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<gpx creator=\"EdgeStreamGPX\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\" version=\"1.1\" xmlns=\"http://www.topografix.com/GPX/1/1\">\n<metadata>\n";
      const char header2[] = "</metadata>\n<trk>\n  <name>test output</name>\n  <trkseg>\n";
      const char trailer1[] = "  </trkseg>\n</trk>\n</gpx>\n";

      int ret_val, frame_count = 0;

      bool fFirstPoint = false;

   /* open input gpx file */

      fp_in = fopen(MediaParams[0].Media.inputFilename, "r");  /* cmd line -i should specify gpx text file */

      if (!fp_in) {

         printf("Unable to find input gpx file %s \n", MediaParams[0].Media.inputFilename);
         goto gpx_process_cleanup;
      }

   /* open output gpx file and write header info */

      fp_out = fopen(MediaParams[0].Media.outputFilename, "w");  /* cmd line -o should specify gpx text file */

      if (!fp_out) {

         printf("Unable to create output gpx file %s \n", MediaParams[0].Media.outputFilename);
         goto gpx_process_cleanup;
      }

      fwrite(header1, strlen(header1), 1, fp_out);

      strcpy(tmpstr, "  <time>");
      time_t t;
      time(&t);
      struct tm* timeinfo;
      timeinfo = gmtime(&t);
      sprintf(&tmpstr[strlen(tmpstr)], "%04d-%02d-%02dT%02d:%02d:%02d", 1900+timeinfo->tm_year, timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
      strcat(tmpstr, "Z");
      strcat(tmpstr, "</time>\n");
      fwrite(tmpstr, strlen(tmpstr), 1, fp_out);

      fwrite(header2, strlen(header2), 1, fp_out);

   /* any other initialization */
   
      nSamplingFrequency = max(nSamplingFrequency, GPS_FS_DEFAULT);  /* if -Fn given on command line then set to value of n. If no -Fn given then default is value defined in gpxlib.h (currently set to 1 Hz) */

      printf("Running gpx data flow ... \n");

      while (pm_run) {

         key = toupper(getkey());
         if (key == 'Q') {  /* break out of while loop */
            pm_run = 0;
            break;
         }

         frame_count++;
         printf("\rReading gpx frame %d", frame_count);

      /* read gpx data frame */

         if ((ret_val = gpx::read_gpx_frame(fp_in, gpx_points_in, NUM_GPX_POINTS_PER_FRAME)) < 0) pm_run = 0;  /* read_gpx_frame() returns number of gpx points read. Break out of while loop on error condition */

      /* apply signal processing flow:

         -fix "loops", where one or more points shows a physically impossible (or at least highly unlikely) heading change
         -correct points with extreme altitude deviations (i.e. phyically impossible deviations within 1-2 sec)
         -detect and account for GPS dropout
         -2-D lowpass filtering
         -[to-do] nearest road recognition
         -[to-do] point-by-point perpendicular snap-to-road
      */

         for (i=0; i<ret_val; i++) {

            if (!fFirstPoint) {  /* initialize lookback buffers after reading first frame */

               for (j=1; j<=N_LOOKBACK; j++) {
                  gpx_points_in[-j] = gpx_points_in[i];
                  gpx_points_out[-j] = gpx_points_in[i];
               }

               fFirstPoint = true;
            }

         /* calculate distance and heading, update running sums */

  #if 0
  if (gpx_points_in[i].lat == 37.354092f) printf("*** found i = %d specific point lat = %f, time change = %d, elev change = %d \n", i, gpx_points_in[i].lat, (int)(gpx_points_in[i].time - gpx_points_in[i-1].time), (int)(gpx_points_in[i].elev - gpx_points_in[i-1].elev));
  #endif
            float d = gpx::gpx_distance(gpx_points_in[i-1].lat, gpx_points_in[i-1].lon, gpx_points_in[i].lat, gpx_points_in[i].lon);
            float h = gpx::gpx_bearing(gpx_points_in[i-1].lat, gpx_points_in[i-1].lon, gpx_points_in[i].lat, gpx_points_in[i].lon);

            run_sum_d += d - run_sum_save_d[(run_sum_index - N_RUN) & (N_RUN-1)];  /* add newest running sum value, subtract oldest */
            run_sum_h += h - run_sum_save_h[(run_sum_index - N_RUN) & (N_RUN-1)];

         /* fix loops which are physically impossible:

            -if heading reverses compared to prior trajectory (running sum), then interpolate a new point
            -re-update running sums
            -note - I assume such loops result from GPS receiver and/or OS service fuck-ups; e.g. sat signal was lost, then regained with a new sat, one or both points had errors, and the new point is impossible (or at least extremely unlikely) - but no assessment was made
         */ 

            float dh = abs(h - run_sum_h/N_RUN);  /* heading delta change (note - gpx_bearing() returns h in radians, -pi/2 to pi/2) */

            if (dh > 2*M_PI/5 && i < ret_val-1) {  /* does heading change more than 144 degrees compared to previous N_RUN points ? (i.e. 4/5 of pi/2, which is close to a 180 reversal) */

               gpx_points_in[i].lon = (gpx_points_in[i+1].lon + gpx_points_in[i-1].lon)/2;  /* replace point with before and after interpolation */
               gpx_points_in[i].lat = (gpx_points_in[i+1].lat + gpx_points_in[i-1].lat)/2;
               gpx_points_in[i].elev = (gpx_points_in[i+1].elev + gpx_points_in[i-1].elev)/2;

            /* recalculate distance and heading, re-update running sums */

               run_sum_d -= d;
               run_sum_h -= h;

               d = gpx::gpx_distance(gpx_points_in[i-1].lat, gpx_points_in[i-1].lon, gpx_points_in[i].lat, gpx_points_in[i].lon);
               h = gpx::gpx_bearing(gpx_points_in[i-1].lat, gpx_points_in[i-1].lon, gpx_points_in[i].lat, gpx_points_in[i].lon);

               run_sum_d += d;
               run_sum_h += h;

               loop_fix_count++;
            }

            float dt = gpx_points_in[i].time - gpx_points_in[i-1].time;  /* delta time */

         /* look for extreme altitude deviations:

            -change of altitude more than 4 m in under 2 sec ? not likely; even a 20% downhill grade at 20 m/sec change would not exceed that
            -we further require minimum heading change (22.5 deg) and distance excursion (which seems to indicate a sat switch or error)
            -note - after further review of a lot of recordings, altitude data is completely unreliable, so the distance requirement is set higher (20 m)
         */

            if (dt < 2*nSamplingFrequency && dh > M_PI/16 && d > 20 && abs(gpx_points_in[i].elev - gpx_points_in[i-1].elev) > 4) {

               alt_filt_count = 10;
            /* to-do: add "marker", add 100 m vertical distance to point so it can easily be seen, but after all processing has occurred */

   //printf("***** alt dev = %d, dist = %f, lat = %2.5f \n", abs(gpx_points_in[i].elev - gpx_points_in[i-1].elev), d, gpx_points_in[i].lat);

               alt_dev_count++;
            }
            else if (alt_filt_count > 0) alt_filt_count--;

         /* update running sum buffers with new values */

            run_sum_save_d[run_sum_index & (N_RUN-1)] = d;
            run_sum_save_h[run_sum_index & (N_RUN-1)] = h;
            run_sum_index++;

         /* dynamically adjust filter coefficients and apply filter:

            -higher speed with unlikely changes in heading --> more aggressive filtering (compensate for overshoot) (add change in heading, JHB Jun2022)
            -extreme altitude deviations --> more aggressive filtering
         */ 

            float a = alt_filt_count > 0 ? 0.1 : (d > 10 && dh > M_PI/16 ? 0.3 : 0.5);
            float b = 1-a;  /* unity gain filter */

         /* look for GPS dropout:
         
            -after dropout, have no choice but to "fully trust" next available point
            -override any previous settings
         */

            if (dt > 4*nSamplingFrequency) {

               alt_filt_count = 0;
               a = 1;
               b = 0;
               drop_out_count++;
            }

            if (a < 0.5) aggressive_count++;
            else relax_count++;

         /* set output values */

            gpx_points_out[i].lat = a*gpx_points_in[i].lat + b*gpx_points_out[i-1].lat;
            gpx_points_out[i].lon = a*gpx_points_in[i].lon + b*gpx_points_out[i-1].lon;
            gpx_points_out[i].elev = gpx_points_in[i].elev;

            gpx_points_out[i].time = gpx_points_in[i].time;
            gpx_points_out[i].time_zone = gpx_points_in[i].time_zone;

         }  /* end of frame loop */

      /* write gpx output frame */

         gpx::write_gpx_frame(fp_out, gpx_points_out, ret_val);

      /* update lookback buffers */

         memcpy(gpx_points_in_buffer, &gpx_points_in[NUM_GPX_POINTS_PER_FRAME - N_LOOKBACK], N_LOOKBACK*sizeof(GPX_POINT));
         memcpy(gpx_points_out_buffer, &gpx_points_out[NUM_GPX_POINTS_PER_FRAME - N_LOOKBACK], N_LOOKBACK*sizeof(GPX_POINT));

         if (ret_val < NUM_GPX_POINTS_PER_FRAME) pm_run = 0;  /* break out of while loop on end of data */
      }

      printf("\n");

      printf("stats: aggressive filter count = %d, relaxed filter count = %d, loop fix count = %d, alt deviation count = %d, drop out count = %d \n", aggressive_count, relax_count, loop_fix_count, alt_dev_count, drop_out_count);

   /* write output file trailer info */

      fwrite(trailer1, strlen(trailer1), 1, fp_out);

gpx_process_cleanup:

      if (fp_in) fclose(fp_in);
      if (fp_out) fclose(fp_out);
   }

   strcpy(tmpstr, "x86 mediaTest end");
   if (num_app_threads > 1) sprintf(&tmpstr[strlen(tmpstr)], " thread = %d", thread_index);
   else strcat(tmpstr, " process");
   printf("%s \n", tmpstr);

   return (void*)1;
}
