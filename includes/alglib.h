/*
  $Header: /root/Signalogic/DirectCore/include/alglib.h
 
  Description: API for generic algorithmic functions, including signal processing, speech recognition, matrix operations, neural networks, etc.

  Projects: SigSRF, DirectCore
 
  Copyright (C) Signalogic Inc. 2001-2024

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

  Revision History

   fs_conv.h Created Apr 2001 Rastko Selmic
   Modified May 2001 Donald Eubanks
   Modified Aug 2013 Jeff Brower
   Modified Mar 2015 Jeff Brower

   alglib.h Created Jun 2018 Chris Johnson
   Modified Jun 2018 CKJ, add DSAudioMerge() API
   Modified Jun 2018 JHB, move DSConvertFs() here from voplib
   Modified Jul 2018 JHB, add DSAgc()
   Modified Jul 2018 JHB, renam DSAudioMerge() to DSMergeAudioStream() and add chnum param
   Modified Jan 2019 CKJ, add DSConvertDataFormat() + DS_CONVERTDATA_xxx flags
   Modified Jul 2019 JHB, add defines for mediaTest segmentation and silence/sounds strip command line entry (-sN)
   Modified Oct 2019 JHB, add autoscaling in DSMergeStreamAudioEX() to avoid clipping, and DS_AUDIO_MERGE_NORESCALE flag to disable if needed
   Modified Feb 2022 JHB, add user-defined filter params (pFilt and filt_len), add uFlags param and DS_FSCONV_xxx flags, change DSConvertFs() pData and pDelay params to void* to lay groundwork for floating-point filters
   Modified Aug 2023 JHB, add memadd() prototype
   Modified Feb 2024 JHB, add DS_FSCONV_SATURATE flag. If any samples in output of DSConvertFs() will wrap min/max amplitude value, saturate instead
   Modified Dec 2024 JHB, add 64-bit signed integer saturated addition, add DS_FSCONV_DEBUG_SHOW_SATURATION_OCCURRENCES flag
*/

#ifndef _ALGLIB_H_
#define _ALGLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* alglib version string global var */

extern const char ALGLIB_VERSION[];

/* Standard C Function: Greatest Common Divisor */
static inline int gcd(int a, int b) {

int c;

   while ( a != 0 ) { c = a; a = b%a; b = c; }

   return b;
}

#if (_GCC_VERSION >= 70000)  /* _GCC_VERSION macro defined in alias.h */

/* 64-bit signed integer saturated addition from Peter Cordes https://stackoverflow.com/questions/17580118/signed-saturated-add-of-64-bit-ints/56531252#56531252 to optimize saturated 64-bit add */

static inline int64_t signed_sat_add64_gnuc_v2(int64_t a, int64_t b) {

long long res;
bool overflow = __builtin_saddll_overflow(a, b, &res);  /* __builtin_saddll_overflow() available in gcc 7.0 or later */

   if (overflow) {
   // overflow is only possible in one direction depending on the sign bit
      return ((uint64_t)b >> 63) + INT64_MAX;
   // INT64_MIN = INT64_MAX + 1  wraparound, done with unsigned
   }

   return res;
}
#endif

/* Following APIs can be applied to data independently of source (source can be UDP/RTP packet, file, USB audio, etc) */

/*
   DSConvertFs() - converts sampling rate of an input buffer.  Notes:

    -up factor and down factor should be smallest possible integers. For example if up sampling from 16 kHz to 24 kHz, up factor should be 3 and down factor should be 2. The ratio of up factor to down factor is the "conversion ratio"

    -input data length should be specified in samples

    -returns an output data length in samples, calculated by multiplying input data length by the conversion ratio. If DS_FSCONV_NO_INTERPOLATE is specified then the return value is the input data length multiplied by up_factor; if DS_FSCONV_NO_DECIMATE is specified then the return value is the input data length divided by down_factor. -1 is returned for error conditions

   Note this API is different than DSConvertFsPacket() in pktlib, which performs Fs conversion based on TERMINATION_INFO struct (shared_include/session.h) info given in session definitions
*/

int DSConvertFs(void* pData,         /* pointer to input and output data (processing is done in-place). Assumed to be interleaved by num chanenls for multichannel data */
                int Fs,              /* sampling rate of data, in Hz */
                int up_factor,       /* up factor */    
                int down_factor,     /* down factor */
                void* pDelay,        /* pointer to delay values (per channel, not interleaved). Delay values must be preserved between calls to DSConvertFs() */
                int data_len,        /* data length, in samples */
                int numChan,         /* num channels represented in input/output data */
                void* pFilt,         /* pointer to user-defined filter coefficients. pFilt is NULL for typical telecom and audio Fs conversions*/
                int filt_len,        /* user-defined filter length, not used unless pFilt is non-NULL */
                unsigned int uFlags  /* flags */
               );

#define DS_FSCONV_FLOATING_POINT        0x100  /* if the DS_FSCONV_FLOATING_POINT flag is given then input/output data, delay values, and filter coefficient are single precision (32-bit) floating point, otherwise (no flag, which is the defaut) they are integer (16-bit) fixed point. Note - floating-point is in process of being added, JHB Feb2022 */
#define DS_FSCONV_NO_INTERPOLATE        0x200  /* don't do interpolation (ignore up factor) */
#define DS_FSCONV_NO_DECIMATE           0x400  /* dont do decimation (ignore down factor) */
#define DS_FSCONV_NO_FILTER             0x800  /* don't perform filtering */

#define DS_FSCONV_SATURATE            0x10000  /* saturate result (avoid wrapping in output integer data) */

#define DS_FSCONV_DEBUG_SHOW_SATURATION_OCCURRENCES  0x100000

int DSAgc(float x[],     /* input/output array, single-precision float in, short int out */
          float mem[],   /* per channel memory values of size 2, init to [0,0] */
          const short n  /* array size, in number of elements */
         );

/* Following APIs require a chnum parameter that specifies a stream group owner */
 
/*
  DSMergeStreamAudio() - merge stream audio.  Notes:

    -chnum is used when uFlags specifies an AGC or adaptive gain algorithm for controlling merging output levels (these algorithms typically require previous audio data, or "history", which is maintained by the stream group owner). Otherwise chnum can be NULL
    -x1 and x2 are input audio arrays
    -y is the output audio stream
    -uFlags specifies the merge mode
    -length is the length of input and output arrays (both inputs should be same length)
*/

int DSMergeStreamAudio(unsigned int chnum, int16_t *x1, float x1_scale, int16_t *x2, float x2_scale, int16_t *y, unsigned int uFlags, uint32_t length);

int DSMergeStreamAudioEx(unsigned int chnum, unsigned int num_vec, int16_t *x, float* scale, int16_t *y, unsigned int uFlags, int vec_len);

#define DS_AUDIO_MERGE_NONE                 0     /* no action */
#define DS_AUDIO_MERGE_ADD              0x100     /* default operation */
#define DS_AUDIO_MERGE_ADD_AGC          0x200     /* enable AGC */
#define DS_AUDIO_MERGE_ADD_SCALING      0x400
#define DS_AUDIO_MERGE_ADD_COMPRESSION  0x800

#define DS_AUDIO_MERGE_LOUDEST_TALKER  0x1000    /* add only loudest talkers (applies to 3 or more input streams).  This flag may be combined with DS_AUDIO_MERGE_ADD and DS_AUDIO_MERGE_ADD_WITH_AGC flags */

void* memadd(void* dst, const void* src, size_t len);

int isArrayZero(uint8_t*, int);
int isArrayLess(short int* array, int len, int thresh);
int ScaleArray(short int* array, unsigned int uFlags, int len, float scale_or_limit);

#define DS_SCALE_ARRAY_LIMIT                1
#define DS_SCALE_ARRAY_SCALE                2

/* convert from one data type to another */

int DSConvertDataFormat(void *in, void *out, uint32_t uFlags, int length);

/* flags for DSConvertDataFormat */
#define DS_CONVERTDATA_CHAR              0x01
#define DS_CONVERTDATA_SHORT             0x02
#define DS_CONVERTDATA_INT               0x03
#define DS_CONVERTDATA_FLOAT             0x04
#define DS_CONVERTDATA_DOUBLE            0x05

/* audio segmentation and strip flags (value of N in -sN mediaTest command line option) */

#define DS_SEGMENT_AUDIO                 0x01      /* segment audio input into intervals based on audio content.  Notes:

                                                    -if DS_SEGMENT_ADJUST is given, minimum target interval duration is given by command line interval entry -IN (N is in msec).  If no cmd line interval entry is given, default minimum interval is 250 msec
                                                    -if DS_SEGMENT_ADJUST is not given, segmentation is done strictly by interval. If no cmd line interval entry is given, default maximum interval is 2000 msec
                                                 */

#define DS_SEGMENT_ADJUST                0x02      /* adjust intervals to be on non-speech boundaries, based on DS_STRIP_xxx flags below (ignored if no DS_STRIP_xxx flags are given) */

#define DS_SEGMENT_TRIM                  0x04      /* trim silence and/or sounds from segment ends, but not within segments, based on DS_STRIP_xxx flags below (ignored if no DS_STRIP_xxx flags are given) */

#define DS_SEGMENT_TIMESTAMPS_TEXT       0x08      /* write interval timestamps to text file.  The text filename is the same as the input audio file (e.g. wav file) suffixed with _seg_ts.txt */
#define DS_SEGMENT_TIMESTAMPS_SCREEN     0x10      /* print to screen interval timestamps */

/* following constants specify generation of additional audio output files, which can be utilized as needed or overlaid on the input audio to analyze segmentation quality */

#define DS_SEGMENT_OUTPUT_CONCATENATE    0x20      /* create audio file with segments concatenated.  The output filename is the input name with suffix "concat" */
#define DS_SEGMENT_OUTPUT_STRIPPED       0x40      /* create audio file showing content that was stripped.  The output filename is the input name with suffix "stripped" */
#define DS_SEGMENT_ADD_MARKERS           0x80      /* may be used with OUTPUT_CONCATENATE and OUTPUT_STRIPPED flags to show markers at segment boundaries.  Markers are 2 samples, one max negative and one max positive */

/* audio strip flags, may be used with / without segmentation */

#define DS_STRIP_SILENCE               0x1000    /* strip silence and background noise from audio input */
#define DS_STRIP_SOUNDS                0x2000    /* strip sounds from audio input (i.e. non-voice sounds music, tones, etc) */

#define DS_SEGMENT_DEBUG_INFO        0x100000  /* display additional segmentation debug info */


/* APIs to be moved here from c66x and other lib source code dating prior to 2010:

  -FFT (various)
  -convolution
  -transfer function
*/

#ifdef __cplusplus
}
#endif

#endif  /* _ALGLIB_H_ */
