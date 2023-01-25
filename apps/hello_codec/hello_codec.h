/* hello_codec.h

   Copyright (C) Signalogic 2022-2023

   Revision History
     Created Aug 2022
*/

#ifndef _HELLO_CODEC_H
#define _HELLO_CODEC_H

/* definitions */

#define AUDIO_SAMPLE_SIZE  2      /* in bytes. Currently all codecs take 16-bit samples. Some like AMR require 14-bit left-justified within 16 bits */

#define NUM_FRAMES         100    /* number of test data frames -- change as needed */
#define F_1KHZ             1000   /* 1 kHz used for test sine wave generation -- change as needed */
#define A_4096             4096   /* amplitude used for test waveform generation -- change as needed*/

/* hello_codec local functions. Notes:

  -these are reference/example functions, replace with application-specific code as needed
  -the most important helper function is set_codec_params(), which fills in CODEC_PARAMS encoder and decoder structs
*/

int read_codec_config_file(codec_test_params_t* codec_test_params, int* input_sampleRate, int* output_sampleRate);
bool set_codec_params(codec_test_params_t* codec_test_params, CODEC_PARAMS* CodecParams, float* codec_frame_duration, int* codec_sampleRate);
int set_frame_sizes(codec_test_params_t* codec_test_params, float codec_frame_duration, int input_sampleRate, int output_sampleRate, int* input_framesize, int* coded_framesize, int* output_framesize, int* inbuf_size, int*outbuf_size);

void generate_test_waveform(uint8_t* buffer, int numFrames, int input_framesize, int Fs, int Fc, int Amp);
int write_wav_file(uint8_t* buffer, int input_sampleRate, int numChan, int len);
int cmdline(int argc, char** argv);
void print_info();
void prolog();
void epilog();

/* vars */

/* command line and platform management items, see notes above */

static HPLATFORM       hPlatform = -1;             /* platform handle, see DSAssignPlatform() call */
codec_test_params_t    codec_test_params = { 0 };  /* codec parameters read from cmd line config file */

/* Codec related var declarations. Notes:

  -HCODEC defines a codec handle, returned by DSCodecCreate(), then used for voplib APIs DSCodecEncode(), DSCodecDecode(), DSGetCodecInfo(), etc
  -handle value returned by DSCodecCreate():  0 = not initialized, < 0 indicates an error, > 0 is valid codec handle
  -here we use arrays of codec handles to allow multichannel audio processing as an example. Note that multichannel audio (e.g. stereo, or N-channel wav file) is in addition to concurrent (multithread) codec streams; i.e. they are not the same thing
*/

HCODEC               encoder_handles[MAX_AUDIO_CHAN] = { 0 },  /* MAX_AUDIO_CHAN defined in voplib.h ... this is a completely arbitrary value, can be whatever the application needs */
                     decoder_handles[MAX_AUDIO_CHAN] = { 0 };

CODEC_PARAMS         CodecParams = { 0 };  /* CODEC_PARAMS and CODEC_OUTARGS defined in voplib.h */
CODEC_OUTARGS        encOutArgs = { 0 };   /* currently only used by AMR-WB+, see comments below */

int                  numChan = 1;  /* number of audio channels per codec stream (e.g. stereo, or N-channel wav file), one codec handle per channel. Note this is separate from concurrent codec streams (multithread) */

/* note - we keep some sampling rate conversion related definitions in place as a reminder for storage requirements, even though Fs conversion functionality is not supported in hello_codec as it is in mediaTest */

#define MAX_FS_CONVERT_MEDIATEST  160  /* mediaTest sampling rate conversion worst case:  44100 to/from 48000 kHz (was 24 for 8 to/from 192 kHz) */
#define MAX_CHAN_FS_CONVERT_TRADEOFF_SIZE  (MAX_FS_CONVERT_MEDIATEST*4)  /* to limit stack usage, we define a "tradeoff size" between number of audio channels and worst-case Fs conversion, for example 4 channels at 44.1 <--> 48 kHz, or 100 channels at 8 <--> 48 kHz, etc */

uint8_t in_buf[MAX_RAW_FRAME*MAX_CHAN_FS_CONVERT_TRADEOFF_SIZE*AUDIO_SAMPLE_SIZE];  /* raw audio input buffer, prior to encoding */
uint8_t out_buf[MAX_RAW_FRAME*MAX_CHAN_FS_CONVERT_TRADEOFF_SIZE*AUDIO_SAMPLE_SIZE] = { 0 };  /* decoded audio output buffer */
uint8_t coded_buf[MAX_CODED_FRAME*MAX_AUDIO_CHAN];  /* encoder output buffer */

struct timeval tv;
uint64_t t1, t2;

int frame_count = 0, input_sampleRate = 0, output_sampleRate = 0, codec_sampleRate = 0;

float codec_frame_duration = 0.0;  /* codec frame duration, in msec. Note this is a float, as some codecs have non-integral frame sizes */

int input_framesize = 0, coded_framesize = 0, output_framesize = 0, inbuf_size = 0, outbuf_size = 0, __attribute__((unused)) len; /* note: in bytes, not samples */

#endif
