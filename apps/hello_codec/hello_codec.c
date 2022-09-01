/*
 $Header: /root/Signalogic/apps/mediaTest/hello_codec/hello_codec.c

 Copyright (C) Signalogic Inc. 2022
 
 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

 Description
 
  Minimum codec usage example

 Purposes
 
  1) Demonstrate minimum usage for SigSRF codecs
  2) Provide a simplified reference / starting point for customer integration
  3) Highlight where advanced functionality may be added (or has been implemented in mediaTest), for example sampling rate conversion, user-defined media processing, etc

 Notes
 
   -this example does not include audio file or USB I/O, intermediate coded output file I/O, sampling rate conversion, multichannel audio, etc. For a complete implementation, see x86_mediaTest() in x86_mediaTest.c
   -notwithstanding the above note, a simple "write result to wav file" is included at the tail end of the test, to allow convenient verification of codec output
   -HPLATFORM is a handle used for platform, VM, and concurrency management provided by DirectCore. A platform handle is not needed for licensed codec-only applications
   -PLATFORMPARAMS, MEDIAPARAMS, and codec_test_params_t are struct definitions used for command line and codec config file handling. They are used here for convenience and not needed in application-specific programs
   
 Revision History

  Created Aug 2022 JHB
*/

/* Linux header files */

#include <stdio.h>
#include <math.h>

/* app support header files */

#include "test_programs.h"         /* general application and test programs support (command line entry, etc) */
#include "mediaTest.h"             /* media specific app and test program support */
#include "cmd_line_debug_flags.h"  /* bring in ENABLE_xxx definitions to support -dN cmd line flag (look for "debugMode"), JHB Jan2022 */

/* SigSRF lib header files (all libs are .so format) */

#include "hwlib.h"  /* platform, VM, and concurrency management provided by DirectCore (not needed for licensed codec-only applications) */
#include "voplib.h"

/* cross-platform definitions */

#include "shared_include/session.h"      /* codec definitions */
#include "shared_include/transcoding.h"  /* transcoding related */

/* hello codec definitions */

#define AUDIO_SAMPLE_SIZE  2      /* in bytes. Currently all codecs take 16-bit samples. Some like AMR require 14-bit left-justified within 16 bits */

#define NUM_FRAMES         100    /* number of test data frames -- change as needed */
#define F_1KHZ             1000   /* 1 kHz used for test sine wave generation -- change as needed */
#define A_4096             4096   /* amplitude used for test waveform generation -- change as needed*/

/* local functions, defined after main() */

int read_codec_config_file(codec_test_params_t* codec_test_params, int* input_sampleRate, int* output_sampleRate);
bool set_codec_params(codec_test_params_t* codec_test_params, CODEC_PARAMS* CodecParams, float* codec_frame_duration, int* codec_sampleRate);
int set_frame_sizes(codec_test_params_t* codec_test_params, float codec_frame_duration, int input_sampleRate, int output_sampleRate, int* input_framesize, int* coded_framesize, int* output_framesize, int* inbuf_size, int*outbuf_size);

void generate_test_waveform(uint8_t* buffer, int numFrames, int input_framesize, int Fs, int Fc, int Amp);
int write_wav_file(uint8_t* buffer, int input_sampleRate, int numChan, int len);
int cmdline(int argc, char** argv);


/* command line and platform management items, see notes above */

static HPLATFORM       hPlatform = -1;             /* platform handle, see DSAssignPlatform() call */
codec_test_params_t    codec_test_params = { 0 };  /* codec parameters read from cmd line config file */

/* HCODEC defines a codec handle. Notes:

  -handle value returned by DSCodecCreate():  0 = not initialized, < 0 indicates an error, > 0 is valid codec handle
  -here we use arrays of handles to allow multichannel audio processing as an example. Note that multichannel audio (e.g. stereo, or N-channel wav file) is in addition to concurrent (multithread) codec streams; i.e. they are not the same thing
*/

HCODEC               encoder_handles[MAX_AUDIO_CHAN] = { 0 },
                     decoder_handles[MAX_AUDIO_CHAN] = { 0 };

CODEC_PARAMS         CodecParams = { 0 };  /* CODEC_PARAMS and CODEC_OUTARGS defined in voplib.h */
CODEC_OUTARGS        encOutArgs = { 0 };   /* currently only used by AMR-WB+, see comments below */

int                  numChan = 1;  /* number of audio channels per codec stream (e.g. stereo, or N-channel wav file). Note this is separate from concurrent codec streams (multithread), although it still requires multiple codec instances (one per channel) */

int main(int argc, char** argv) {

int i;
char tmpstr[500], tmpstr2[500], szNumChan[30];
struct timeval tv;
uint64_t t1, t2;
int frame_count = 0;

int input_sampleRate = 0, output_sampleRate = 0, codec_sampleRate = 0;

float codec_frame_duration = 0.0;  /* codec frame duration, in msec. Note this is a float, as some codecs have non-integral frame sizes */
int input_framesize = 0, coded_framesize = 0, output_framesize = 0, inbuf_size = 0, outbuf_size = 0, __attribute__((unused)) len; /* note: in bytes, not samples */

/* note - we keep some sampling rate conversion related definitions in place as a reminder for storage requirements, even though Fs conversion functionality is not supported in hello_codec as it is in mediaTest */

#define MAX_FS_CONVERT_MEDIATEST  160  /* mediaTest sampling rate conversion worst case:  44100 to/from 48000 kHz (was 24 for 8 to/from 192 kHz) */
#define MAX_CHAN_FS_CONVERT_TRADEOFF_SIZE  (MAX_FS_CONVERT_MEDIATEST*4)  /* to limit stack usage, we define a "tradeoff size" between number of audio channels and worst-case Fs conversion, for example 4 channels at 44.1 <--> 48 kHz, or 100 channels at 8 <--> 48 kHz, etc */

uint8_t in_buf[MAX_RAW_FRAME*MAX_CHAN_FS_CONVERT_TRADEOFF_SIZE*AUDIO_SAMPLE_SIZE];
uint8_t out_buf[MAX_RAW_FRAME*MAX_CHAN_FS_CONVERT_TRADEOFF_SIZE*AUDIO_SAMPLE_SIZE] = { 0 };
uint8_t coded_buf[MAX_CODED_FRAME*MAX_AUDIO_CHAN];

/* handle command line params. Params and format are same as mediaTest, but currently hello_codec is only paying attention to codec config file and debug mode (-dN entry) */

   if (cmdline(argc, argv) < 0) exit(EXIT_FAILURE);

   printf("hello codec start, debug flags = 0x%llx \n", (unsigned long long)debugMode);

   hPlatform = DSAssignPlatform(NULL, PlatformParams.szCardDesignator, 0, 0, 0);  /* assign platform handle used for concurrency, VM management, and demo management. A platform handle is not required for licensed codec-only applications */

   DSConfigVoplib(NULL, NULL, DS_CV_INIT);  /* initialize voplib and codec libs */

/* read codec config file */

   if (read_codec_config_file(&codec_test_params, &input_sampleRate, &output_sampleRate) < 0) goto cleanup;

/* fill CodecParams struct. Notes:

     -in this case we fill CodecParams from codec config file specified on the commmand line
     -applications can set CodecParams in any way as needed. See codec-specific sections of the switch statement inside set_codec_params()
     -note that CodecParams has encoder and decoder sub-structs
     -see additional notes inside set_codec_params()
*/
   if (set_codec_params(&codec_test_params, &CodecParams, &codec_frame_duration, &codec_sampleRate)) {

      CodecParams.enc_params.frameSize = CodecParams.dec_params.frameSize = codec_frame_duration;  /* in msec */
      CodecParams.codec_type = codec_test_params.codec_type;
      unsigned int uFlags = (debugMode & ENABLE_MEM_STATS) ? DS_CODEC_CREATE_TRACK_MEM_USAGE : 0;  /* debugMode set with -dN on cmd line. ENABLE_MEM_STATS is defined in mediaMin.h, JHB Jan2022 */

   /* create required number of encoder and decoder instances. Notes:

      -any number of codec instances can be created dynamically by any number of threads, at any time
      -to specify multichannel audio data (e.g. stereo, N-channel wav), set numChan > 1. Multichannel concurrent instances are separate from / additional to multithread conccurrent instances
   */

      for (i=0; i<numChan; i++) {

         if ((encoder_handles[i] = DSCodecCreate(&CodecParams, DS_CODEC_CREATE_ENCODER | uFlags)) < 0) { printf("failed to init encoder\n"); goto cleanup; }

         if ((decoder_handles[i] = DSCodecCreate(&CodecParams, DS_CODEC_CREATE_DECODER | uFlags)) < 0) { printf("failed to init decoder\n"); goto cleanup; }
      }
   }
   
/* set input/output and intermediate (coded) frame sizes */

   if (set_frame_sizes(&codec_test_params, codec_frame_duration, input_sampleRate, output_sampleRate, &input_framesize, &coded_framesize, &output_framesize, &inbuf_size, &outbuf_size) < 0) goto cleanup;

/* print some relevant params and stats -- sanity checks ! */

   sprintf(szNumChan, "%d channel", numChan);
   if (numChan > 1) strcat(szNumChan, "s");

   if (codec_test_params.codec_type != DS_VOICE_CODEC_TYPE_NONE) {

      strcpy(tmpstr, "encoder"); sprintf(tmpstr2, "decoder framesize (bytes) = %d, ", coded_framesize);
   }
   else { strcpy(tmpstr, "pass-thru"); strcpy(tmpstr2, ""); }

   printf("  input framesize (samples) = %d, %s framesize (samples) = %d, %sinput Fs = %d Hz, codec Fs = %d, output Fs = %d Hz, %s\n", input_framesize/AUDIO_SAMPLE_SIZE, tmpstr, inbuf_size/AUDIO_SAMPLE_SIZE, tmpstr2, input_sampleRate, codec_sampleRate, output_sampleRate, szNumChan);

/* generate simple test data ... no file or USB audio I/O supported as in mediaTest */

   generate_test_waveform(in_buf, NUM_FRAMES, input_framesize, input_sampleRate, F_1KHZ, A_4096);

 /* prepare to run the test */

   gettimeofday(&tv, NULL);
   t1 = (uint64_t)tv.tv_sec*1000000L + (uint64_t)tv.tv_usec;

   if (encoder_handles[0] && decoder_handles[0]) printf("Running encoder-decoder data flow ...\n");
   else if (encoder_handles[0]) printf("Running encoder ...\n");
   else if (decoder_handles[0]) printf("Running decoder ...\n");
   else printf("Running pass-thru ...\n");

 /* run the test: encode-decode loop for NUM_FRAMES number of frames */

   while (run) {

      if (toupper(getkey()) == 'Q') { run = 0; break; }  /* user can press 'q' key to break out of while(run) loop */

      for (i=0; i<numChan; i++) {  /* to specify multichannel audio data (e.g. stereo, N-channel wav), set numChan > 1 */
      
         coded_framesize = DSCodecEncode(encoder_handles, 0, &in_buf[frame_count*inbuf_size], coded_buf, inbuf_size, numChan, &encOutArgs);  /* call codec encode API (in voplib)*/

         if (coded_framesize > 0) len = DSCodecDecode(decoder_handles, 0, coded_buf, &out_buf[frame_count*outbuf_size], coded_framesize, numChan, NULL);  /* call codec decode API (in voplib) */
      }

      if (++frame_count >= NUM_FRAMES) break;  /* finished processing data ? */
      else printf("\rProcessing frame %d...", frame_count);
   }

   printf("\n");  /* leave existing status line, including any error messages (don't clear it) */

   if (!run) printf("Exiting test\n");  /* run == 0 indicates an early exit; e.g. user pressed quit key or an error occurred */

   gettimeofday(&tv, NULL);
   t2 = (uint64_t)tv.tv_sec*1000000L + (uint64_t)tv.tv_usec;

   printf("Run-time: %3.6fs\n", 1.0*(t2-t1)/1e6);

/* as a convenient, easy way to verify encode/decode, write output data to wav file */

   write_wav_file(out_buf, input_sampleRate, numChan, frame_count*outbuf_size);

cleanup:

/* codec tear down and cleanup */

   for (i=0; i<numChan; i++) {
      if (encoder_handles[i]) DSCodecDelete(encoder_handles[i]);
      if (decoder_handles[i]) DSCodecDelete(decoder_handles[i]);
   }

   if (hPlatform != -1) DSFreePlatform((intptr_t)hPlatform);  /* free platform handle */

   printf("hello codec end\n");

   return 0;
}

/* supporting functions */

/* set CODEC_PARAMS struct. Notes:

  -CODEC_PARAMS has encoder and decoder substructs. hello_codec sets both as an example; applications can set either or both, as needed
  -constant definitions are in shared_include/session.h
  -codec_frame_duration and codec_sampleRate are used in test processing by hello_codec, applications can either use those or roll their own
  -_XXX_INSTALLED_ defines are set in the Makefile, depending on which codec libs are discovered as installed
*/

bool set_codec_params(codec_test_params_t* codec_test_params, CODEC_PARAMS* CodecParams, float* codec_frame_duration, int* codec_sampleRate) {

bool fCreateCodec = true;

   /* setup/init for specified codec.  Codecs use voplib APIs */

      switch (codec_test_params->codec_type) {

         case DS_VOICE_CODEC_TYPE_EVS:
         {
            CodecParams->enc_params.samplingRate = codec_test_params->sample_rate;       /* in Hz. Note that for fullband (FB, 48 kHz) sampling rate (Fs) with cutoff frequency (Fc) of 20 kHz, a minimum bitrate of 24.4 kbps is required. If you give 13.2 kbps bitrate, then the codec enforces an Fc of 14.4 kHz */
            CodecParams->enc_params.bitRate = codec_test_params->bitrate;                /* in bps */
            CodecParams->enc_params.dtx.dtx_enable = codec_test_params->dtx_enable;      /* 0 = DTX disabled, 1 = enabled */
            CodecParams->enc_params.sid_update_interval = codec_test_params->dtx_value ? codec_test_params->dtx_value : (codec_test_params->dtx_enable ? 8 : 0);  /* if DTX is enabled then default SID update interval is 8.  A zero update interval enables "adaptive SID" */
            CodecParams->enc_params.rf_enable = codec_test_params->rf_enable;
            CodecParams->enc_params.fec_indicator = codec_test_params->fec_indicator;
            CodecParams->enc_params.fec_offset = codec_test_params->fec_offset;
            #if 0
            CodecParams->enc_params.bandwidth_limit = DS_EVS_BWL_SWB;                   /* codec will set limit depending on sampling rate */
            #else  /* change this to make encoder setup more clear, JHB Feb2022 */
            CodecParams->enc_params.bandwidth_limit = DS_EVS_BWL_FB;                    /* codec will set limit lower if required by specified sampling rate, JHB Feb2022 */
            #endif
            CodecParams->enc_params.rtp_pyld_hdr_format.header_format = 1;              /* hard coded to 1 to match 3GPP encoder reference executable, which only writes header full format */

         /* EVS codec DTX notes:

            1) DTX should be specified in codec configuration file.  If not given, default is disabled
            2) EVS codec is used for silence stripping and audio segmentation.  In that case we enable DTX and set the update interval to 0.  An update interval of 0 specifies "adaptive SID"
         */

            CodecParams->dec_params.samplingRate = codec_test_params->sample_rate;
            CodecParams->dec_params.bitRate = codec_test_params->bitrate;             /* we set this to avoid param validation error in DSCodecCreate().  At run-time EVS codec determines bitrate from compressed bitstream info */

            *codec_frame_duration = 20;  /* in msec */
            *codec_sampleRate = codec_test_params->sample_rate;

            break;
         }

         case DS_VOICE_CODEC_TYPE_G711_ULAW:
         case DS_VOICE_CODEC_TYPE_G711_ALAW:
            *codec_frame_duration = 20;  /* in msec */
            break;

#ifdef _AMR_INSTALLED_
         case DS_VOICE_CODEC_TYPE_AMR_NB:
         {
            CodecParams->enc_params.samplingRate = 8000;                   /* in Hz */
            CodecParams->enc_params.bitRate = codec_test_params->bitrate;  /* in bps */
            CodecParams->enc_params.dtx.vad = codec_test_params->vad;

            CodecParams->dec_params.samplingRate = 8000;
            CodecParams->dec_params.bitRate = codec_test_params->bitrate;  /* we set this to avoid param validation error in DSCodecCreate().  At run-time AMR-NB codec determines bitrate from compressed bitstream info */

            *codec_frame_duration = 20;  /* in msec */
            *codec_sampleRate = 8000;

            break;
         }
#endif

#ifdef _AMRWB_INSTALLED_
         case DS_VOICE_CODEC_TYPE_AMR_WB:
         {
            CodecParams->enc_params.samplingRate = 16000;                  /* in Hz */
            CodecParams->enc_params.bitRate = codec_test_params->bitrate;  /* in bps */
            CodecParams->enc_params.dtx.vad = codec_test_params->vad;
            CodecParams->enc_params.rtp_pyld_hdr_format.oct_align = codec_test_params->header_format;

            CodecParams->dec_params.samplingRate = 16000;
            CodecParams->dec_params.bitRate = codec_test_params->bitrate;  /* we set this to avoid param validation error in DSCodecCreate().  At run-time AMR-WB codec determines bitrate from compressed bitstream info */

            *codec_frame_duration = 20;  /* in msec */
            *codec_sampleRate = 16000;

            break;
         }
#endif

#ifdef _AMRWBPLUS_INSTALLED_
         case DS_VOICE_CODEC_TYPE_AMR_WB_PLUS:
         {
            CodecParams->enc_params.samplingRate = codec_test_params->sample_rate;                                         /* in Hz */
            CodecParams->enc_params.bitRate = (int)codec_test_params->mode == -1 ? codec_test_params->bitrate_plus : 0.0;  /* in bps */
            CodecParams->enc_params.mode = codec_test_params->mode;
            CodecParams->enc_params.isf = codec_test_params->isf;
            CodecParams->enc_params.low_complexity = codec_test_params->low_complexity;
            CodecParams->enc_params.dtx.vad = codec_test_params->vad;
            CodecParams->enc_params.nChannels = codec_test_params->num_chan;
            CodecParams->enc_params.mono = codec_test_params->mono;

            CodecParams->dec_params.samplingRate = codec_test_params->sample_rate;
            CodecParams->dec_params.bitRate = CodecParams->enc_params.bitRate;  /* we set this to avoid param validation error in DSCodecCreate().  At run-time AMR-WB+ codec determines bitrate from compressed bitstream info */
            CodecParams->dec_params.limiter = codec_test_params->limiter;
            CodecParams->dec_params.mono = codec_test_params->mono;

            //if (codec_test_params->mono == 0) numChan = 2;

            *codec_frame_duration = 80; /* 80 msec super frame */
            *codec_sampleRate = codec_test_params->sample_rate;

            break;
         }
#endif

#ifdef _G726_INSTALLED_
         case DS_VOICE_CODEC_TYPE_G726:
         {
            CodecParams->enc_params.samplingRate = 8000;                   /* in Hz */
            CodecParams->enc_params.bitRate = codec_test_params->bitrate;  /* in bps */
            CodecParams->enc_params.uncompress = codec_test_params->uncompress;

            CodecParams->dec_params.samplingRate = 8000;
            CodecParams->dec_params.bitRate = codec_test_params->bitrate;
            CodecParams->dec_params.uncompress = codec_test_params->uncompress;

         /* codec_frame_duration notes:
         
            -for G726, increase if more than 10 msec is being encoded or decoded per frame
            -currently codec_test_params_t struct (mediaTest.h) doesn't have a ptime element to control framesize multiples. That's a to-do item
            -packet/media thread processing in pktlib does handle ptime
         */

            *codec_frame_duration = 10;  /* in msec */
            *codec_sampleRate = 8000;

            break;
         }
#endif

#ifdef _G729AB_INSTALLED_
         case DS_VOICE_CODEC_TYPE_G729AB:
         {
            CodecParams->enc_params.samplingRate = 8000;  /* in Hz */
            CodecParams->enc_params.bitRate = 8000;       /* in bps  */
            CodecParams->enc_params.dtx.vad = codec_test_params->vad;
            CodecParams->enc_params.uncompress = codec_test_params->uncompress;

            CodecParams->dec_params.samplingRate = 8000;
            CodecParams->dec_params.bitRate = 8000;
            CodecParams->dec_params.uncompress = codec_test_params->uncompress;

            *codec_frame_duration = 10;  /* in msec */
            *codec_sampleRate = 8000;

            break;
         }
#endif

#ifdef _MELPE_INSTALLED_
         case DS_VOICE_CODEC_TYPE_MELPE:
         {
            printf("  MELPe bit packing density = %d, NPP = %d, Post Filter = %d\n", codec_test_params->bitDensity, codec_test_params->Npp, codec_test_params->post);  /* print additional codec-specific info */

            CodecParams->enc_params.samplingRate = 8000;                   /* in Hz */
            CodecParams->enc_params.bitRate = codec_test_params->bitrate;  /* in bps */
            CodecParams->enc_params.bitDensity = codec_test_params->bitDensity;
            CodecParams->enc_params.Npp = codec_test_params->Npp;

            CodecParams->dec_params.samplingRate = 8000;
            CodecParams->dec_params.bitRate = codec_test_params->bitrate;
            CodecParams->dec_params.bitDensity = codec_test_params->bitDensity;
            CodecParams->dec_params.post = codec_test_params->post;

            switch (codec_test_params->bitrate) {
               case 600:
                  *codec_frame_duration = 90;  /* in msec */
                  break;
               case 1200:
                  *codec_frame_duration = 67.5;  /* in msec */
                  break;
               case 2400:
                  *codec_frame_duration = 22.5;  /* in msec */
                  break;
            }

            *codec_sampleRate = 8000;

            break;
         }
#endif

         default:
            *codec_frame_duration = 20;
            fCreateCodec = false;
            break;
      }
      
   return fCreateCodec;
}

/* set frame and buffer sizes */

int set_frame_sizes(codec_test_params_t* codec_test_params, float codec_frame_duration, int input_sampleRate, int output_sampleRate, int* input_framesize, int* coded_framesize, int* output_framesize, int* inbuf_size, int* outbuf_size) {

int input_upFactor = 1, input_downFactor = 1; /* sampling rate conversion is disabled in hello_codec, for a full implementation see x86_mediaTest() in x86_mediaTest.c */ 
int output_upFactor = 1, output_downFactor = 1;

/* set buffer and frame sizes. Notes:

  -in hello_codec, frame sizes are determined only by codec sampling rate. In mediaTest, which supports sampling rate conversion, input and output frame sizes are determined by audio file / source type. Examples include wav files, USB audio buffers, etc
  -codec_frame_duration is floating-point value in msec
*/

   *input_framesize = codec_frame_duration*(1.0*input_sampleRate/1000)*AUDIO_SAMPLE_SIZE;
   *output_framesize = codec_frame_duration*(1.0*output_sampleRate/1000)*AUDIO_SAMPLE_SIZE;

/* set codec specific intermediate coded output size */

   switch (codec_test_params->codec_type) {

      case DS_VOICE_CODEC_TYPE_G726:

         *coded_framesize = DSGetCompressedFramesize(codec_test_params->codec_type, codec_test_params->bitrate, 0);
         break;

      case DS_VOICE_CODEC_TYPE_G729AB:

         *coded_framesize = DSGetCompressedFramesize(codec_test_params->codec_type, 0, 0);
         break;

      case DS_VOICE_CODEC_TYPE_EVS:
      case DS_VOICE_CODEC_TYPE_AMR_NB:
      case DS_VOICE_CODEC_TYPE_AMR_WB:
      case DS_VOICE_CODEC_TYPE_AMR_WB_PLUS:

         *coded_framesize = DSGetCompressedFramesize(codec_test_params->codec_type, codec_test_params->bitrate, HEADERFULL);
//         printf("input_framesize = %d, coded_framesize = %d\n", input_framesize, *coded_framesize);
         break;

      case DS_VOICE_CODEC_TYPE_MELPE:

         if (!codec_test_params->bitDensity) codec_test_params->bitDensity = 54;  /* default bit density handling should be moved to transcoder_control.c */
         *coded_framesize = DSGetCompressedFramesize(codec_test_params->codec_type, codec_test_params->bitrate, codec_test_params->bitDensity);
         break;

      case DS_VOICE_CODEC_TYPE_NONE:
         break;
   }

   if (codec_test_params->codec_type != DS_VOICE_CODEC_TYPE_NONE && !*coded_framesize) {

      printf("Error: DSGetCompressedFramesize() returns zero\n");
      return -1;
   }

/* set buffer size just prior to codec (or pass-thru) input.  Note that coded_buf is not used for pass-thru mode */

   *inbuf_size = *input_framesize*input_upFactor/input_downFactor;
   *outbuf_size = *output_framesize*output_upFactor/output_downFactor;

   return 1;
}

/* generate test waveform data (16-bit samples) */

void generate_test_waveform(uint8_t* in_buf, int numFrames, int input_framesize, int Fs, int Fc, int Amp) {

#define pi M_PI  /* M_PI is in math.h */

int i, j;
short int* p = (short int*)in_buf;

   for (i=0; i<numFrames; i++) {
      for (j=0; j<input_framesize; j++) *p++ = Amp*sin(2*pi*(i*input_framesize+j)/(input_framesize*Fc/2/Fs));
   }
}

/* write output waveform to .wav file */

int write_wav_file(uint8_t* buffer, int input_sampleRate, int numChan, int len) {

/* Notes:

  -MEDIAINFO and DSSaveDataFile() definitions are in hwlib.h
  -for file types that implement headers, MediaInfo should be filled in with header info
  -by not giving any uFlags (DS_CREATE, DS_OPEN, DS_CLOSE, etc), we invoke the "unified" open, write, close form of calling DSSaveDataFile() (like "batch mode" re. MATLAB and Nvidia, haha). To see frame-based usage of DSSaveDataFile() and DSLoadDataFile(), see x86_mediaTest() in x86_mediaTest.c
*/

MEDIAINFO MediaInfo = { 0 };
unsigned int uFlags = 0;
int ret_val;

   strcpy(MediaInfo.szFilename, "codec_output_test.wav");
   MediaInfo.Fs = input_sampleRate;
   MediaInfo.NumChan = numChan;
   MediaInfo.SampleWidth = DS_DP_SHORTINT;
   MediaInfo.CompressionCode = DS_GWH_CC_PCM;

   ret_val = DSSaveDataFile(DS_GM_HOST_MEM, NULL, MediaInfo.szFilename, (uintptr_t)buffer, len, (uintptr_t)uFlags, &MediaInfo); /* DSSaveDataFile() returns bytes written, with DS_CREATE flag it returns header length (if any, depending on file type) */

   #ifdef WAV_DEBUG
   printf("filename = %s, len = %d, ret val = %d \n", MediaInfo.szFilename, len, ret_val);
   #endif

   return ret_val;
}

/* read and parse codec config file. Notes:

  -config file handling is in transcoder_control.c (mediaTest folder one level up)
  -example codec config files for variety of codecs are in mediaTest/session_config folder
*/

int read_codec_config_file(codec_test_params_t* codec_test_params, int* input_sampleRate, int* output_sampleRate) {

char default_config_file[] = "../session_config/codec_test_config";
char *config_file;
FILE *fp_cfg = NULL;
char szCodecName[50] = "";

   /* Config file handling:  (i) give an error if config file doesn't exist, (ii) use default file only if no config file given and input waveform file appears to be a 3GPP test vector, (iii) otherwise go with input waveform header and/or test mode.  JHB Mar 2018 */

   if (strlen(MediaParams[0].configFilename) == 0) {

      if (strstr(MediaParams[0].Media.inputFilename, "stv")) config_file = default_config_file;  /* use default config file only if input waveform seems to be a 3GPP test vector */
      else config_file = NULL;
   }
   else if (access(MediaParams[0].configFilename, F_OK ) == -1) {

      printf("Codec config file %s not found\n", MediaParams[0].configFilename);
      return -1;
   }
   else config_file = MediaParams[0].configFilename;

   if (config_file) {

      printf("Opening codec config file: %s\n", config_file);

      fp_cfg = fopen(config_file, "r");
   }
   else fp_cfg = NULL;

   if (!fp_cfg) {

      codec_test_params->codec_type = DS_VOICE_CODEC_TYPE_NONE;

      if (*input_sampleRate == 0) *input_sampleRate = 8000;  /* if no codec specified set an arbitrary sampling rate value */
      *output_sampleRate = *input_sampleRate;

      printf("No config file specified, assuming default parameters: ");
   }
   else {

   /* hello_codec doesn't implement sample rate conversion, so we use only the rate given in the codec config file. For a full implementation of Fs conversion, see mediaTest, which determines input/output rates from audio I/O waveform file headers, USB audio buffers, etc */

      parse_codec_test_params(fp_cfg, codec_test_params);

      *output_sampleRate = codec_test_params->sample_rate;
      if (*input_sampleRate == 0) *input_sampleRate = *output_sampleRate;

      numChan = codec_test_params->num_chan;  /* default is 1 if num_chan is not specified in the codec config file */

      printf("Opened config file: ");
   }

   if (DSGetCodecName(codec_test_params->codec_type, szCodecName, DS_CODEC_INFO_TYPE) <= 0) {

      printf("\rError: non-supported or invalid codec type found in config file\n");
      return -1;
   }

   printf("codec = %s, ", szCodecName);
   if (codec_test_params->codec_type != DS_VOICE_CODEC_TYPE_NONE) printf("%d bitrate, ", codec_test_params->bitrate);
   printf("sample rate = %d Hz, ", *output_sampleRate);
   printf("num channels = %d\n", codec_test_params->num_chan);

   if (codec_test_params->codec_type != DS_VOICE_CODEC_TYPE_NONE && (int)codec_test_params->bitrate <= 0) {

      printf("Error: config file specifies a codec but not a bitrate\n");
      return -1;
  }
  
  return 1;
}

/* handle command line */

int cmdline(int argc, char** argv) {

char libstr[500];

  	printf("SigSRF hello codec example program for x86 platforms, Rev 1.0, Copyright (C) Signalogic 2022\n");

   sprintf(libstr, "  Libraries in use: DirectCore v%s", HWLIB_VERSION);
   sprintf(&libstr[strlen(libstr)], ", voplib v%s, diaglib v%s, cimlib v%s", VOPLIB_VERSION, DIAGLIB_VERSION, CIMLIB_VERSION);
   printf("%s\n", libstr);

   if (!cmdLineInterface(argc, argv, CLI_MEDIA_APPS)) exit(EXIT_FAILURE);

   #if 0  /* we don't check for I/O specs in hello_codec. mediaTest has a full I/O implementation */

/* verify test mode settings - any errors then exit the application (no cleanup is necessary at this point) */

   if (codec_test) {

      printf("Invalid cmd line options, make sure: \n");
      printf("  one or more -i and -o (input and output) options are correctly specified \n");
      printf("  one -C (config file) is correctly specified \n");
      printf("  one -c option (platform type) is given and either none or only one -M (operating mode) option is given \n");
      printf("To see the full list of cmd line options, run ./hello_codec -h \n");
      return -1;
   }
   #endif

   return 1;
}

