/*
  $Header: /root/Signalogic/DirectCore/lib/alglib/alglib.c
 
  Description: API for generic algorithmic functions e.g. signal processing, matrix operations, neural networks, etc.
 
  Projects: SigSRF, DirectCore
 
  Copyright Signalogic Inc. 2018-2024

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

  Revision History:
  
   Created Jun 2018 Chris Johnson
   Modified Aug 2018 JHB, add DSMergeStreamAudioEx() which takes allows a specified number of vectors to be merged
   Modified Sep 2018 JHB, add isArrayZero() function
   Modified Jan 2019 CKJ, add DSConvertDataFormat() function
   Modified Jul 2019 JHB, add isArrayLess() to support mediaTest segmentation and silence stripping options
   Modified Oct 2019 JHB, add hybrid clip + high end compression algorithm in DSMergeStreamAudioEx() to avoid consecutive blocks of clipped output samples when scaling is not in effect.  Add DS_AUDIO_MERGE_ADD_COMPRESSION flag for experimental compression algorithm (see comments)
   Modified Feb 2022 JHB, update version string. Major changes made in fs_conv.c, see its comments
   Modified Aug 2023 JHB, add memadd()
   Modified May 2024 JHB, change #ifdef _X86 to #if defined(_X86) || defined(_ARM)
   Modified Dec 2024 JHB, comments only
*/

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>

#include "alias.h"
#include "alglib.h"
#include "session.h"

/* alglib version string */

const char ALGLIB_VERSION[256] = "1.2.6";

/* DSMergeStreamAudio()

   merge audio data from two streams x1 and x2
   apply scaling on the x1 and x2 values when merging based on the x1_scale and x2_scale values. If either value is 0, it will default to 0.5
   length is the number of samples to be merged
   return value is number of samples merged
*/

int DSMergeStreamAudio(unsigned int chnum, int16_t *x1, float x1_scale, int16_t *x2, float x2_scale, int16_t *y, uint32_t uFlags, uint32_t length) {

   int i, tmp;

   if (!uFlags) return 0;  /* no action on this stream, JHB Feb2020 */
   
   /* set to default scaling values if scaling values are unset */
   if (x1_scale == 0) x1_scale = 0.5;
   if (x2_scale == 0) x2_scale = 0.5;
   
   for (i = 0; i < length; i++)
   {
      tmp = x1_scale*x1[i] + x2_scale*x2[i];
      if (tmp > SHRT_MAX) y[i] = SHRT_MAX;
      else if (tmp < SHRT_MIN) y[i] = SHRT_MIN;
      else y[i] = tmp;
   }
   
   return i;
}

/* add two 16-bit vector, saturate to 16-bit */

void* memadd(void* dst, const void* src, size_t len) {

int16_t* y = (int16_t*)dst;
int16_t* x = (int16_t*)src;
int i;

   for (i =0; i<(int)len/2; i++) {

      int32_t sum = (int32_t)x[i] + (int32_t)y[i];

      if (sum > SHRT_MAX) y[i] = SHRT_MAX;
      else if (sum < SHRT_MIN) y[i] = SHRT_MIN;
      else y[i] = sum;
   }

   return dst;
}


/* short int clipping, used internally */

static inline short int clip2short(float x) {

   if (x > SHRT_MAX) return SHRT_MAX;
   else if (x < SHRT_MIN) return SHRT_MIN;
   else return (short int)x;
}

/* num_vec is number of x and scale arrays.  Each x (input) array and optional scale array is of length vec_len; i.e. non-interleaved.  y (output) is a single array of length vec_len */

int DSMergeStreamAudioEx(unsigned int chnum, unsigned int num_vec, int16_t *x, float* scale, int16_t *y, unsigned int uFlags, int vec_len) {

int i, j, k, l;
float xf, sum, prod;
bool scale_calloc = false, no_scale = false;
float inter_prod[MAX_GROUP_CONTRIBUTORS] = { 0.0 };
float sf, sf_inc;

/* check input params */

   if (!uFlags) return 0;  /* no action on this stream, JHB Feb2020 */

   if (x == NULL || y == NULL || num_vec == 0 || vec_len < 0) return -1;

   if (scale == NULL && !(uFlags & DS_AUDIO_MERGE_ADD_COMPRESSION)) {  /* no scale factor(s) given by user.  Note that user might give scale factors as negative or even zero.  We trust whatever input is given when scale != NULL, JHB Oct 2019 */
   
      if (uFlags & DS_AUDIO_MERGE_ADD_SCALING) {

         if (num_vec == 1) no_scale = true;
         else {

            scale = (float*)calloc(num_vec, sizeof(float));
            scale_calloc = true;

            for (j=0; j<num_vec; j++) scale[j] = 1.0/sqrt(num_vec);
         }
      }
      else no_scale = true;
   }

/* add arrays element-by-element, apply clipping, scaling, compression as specified */

   if (uFlags & DS_AUDIO_MERGE_ADD_COMPRESSION) {  /* algorithm derived from http://www.vttoth.com/CMS/index.php/technical-notes/68 and https://stackoverflow.com/questions/12089662/mixing-16-bit-linear-pcm-streams-and-avoiding-clipping-overflow.
                                                      This does seem to work as advertised, but is not fully correct yet -- still seems to clip slightly. As with uLaw compression, all amplitudes are reduced, but less so as amplitude decreases, so in that
                                                      sense there is always some amount of distortion, JHB Oct 2019 */
   
      for (i=0; i<vec_len; i++) {
   
         sum = 0;
         if (num_vec > 1) prod = 1.0;
         else prod = 0;

         for (j=0; j<num_vec; j++) {
  
            xf = x[vec_len*j+i] + 1.0*(SHRT_MAX+1);

            sum += xf;  /* sum all vectors as unsigned */

            for (k=0, l=0; k<num_vec; k++) if (k < j && l < num_vec) {
               inter_prod[l++] = (x[vec_len*k+i] + 1.0*(SHRT_MAX+1))*xf;  /* inter-term products */
            }

            if (num_vec > 1) prod *= xf;  /* product of all terms */
         }

         for (j=0; j<num_vec; j++) sum -= inter_prod[j];

         sum += prod;  /* subtract product of all terms and return to signed data */

         sum -= 1.0*num_vec*(SHRT_MAX+1);

         y[i] = (short int)sum;
      }
   }
   else if (no_scale) {  /* no user-defined scaling */

      sf = 1;
      sf_inc = 0;

      if (num_vec == 1) {

         memcpy(y, x, vec_len*sizeof(short int));  /* for (i=0; i<vec_len; i++) y[i] = x[i]; */
      }
      else for (i=0; i<vec_len; i++) {

         for (j=0, k=i, sum=0; j<num_vec; j++, k+=j*vec_len) sum += /* sf* */x[k];

      /* Default operation is clipping + high end compression hybrid algorithm.  The main objective is to avoid blocks of consecutive clipped output samples.  Assumptions are (i) clipping is not a major problem in an application that merges uncorrelated signals, 
         and (ii) isolated (single) clips cannot be perceived from surrounding audio (each one just looks like a max output).  Notes, JHB Oct 2019:

         -if we clip, we activate a scale factor (sf).  An immediate effect on the next output isn't going to matter because we just clipped, and adjacent audio is likely to be high amplitude for some number of samples
         -sf decays to 1 over remainder of the frame.  Another clip within the frame starts the process over again.  We want to taper back to sf = 1 before the next frame starts, otherwise we would need inter-frame memory to filter / smooth frame edges
         -max sf is 1/sqrt(num_vec).  For 2 vectors, that's amplitude headroom of 1.41; anything more than that we can't handle (at least not yet without putting in some user options)
       */
  
         if (sum > SHRT_MAX) {
            y[i] = SHRT_MAX;
            goto scale_attack;
         }
         else if (sum < SHRT_MIN) {
            y[i] = SHRT_MIN;
scale_attack:
            sf = 1/sqrt(num_vec);
            sf_inc = (1 - sf)/(vec_len - i);  /* calculate increment to taper scale factor to 1 by end of the frame.  Note that dividing by vec_len-i-1 might be exactly correct, but we don't do that to avoid having to check for divide-by-zero */
         }
         else {
            y[i] = (short int)sum;
            sf += sf_inc;  /* sf = 1 and sf_inc = 0 unless clipping occurs */
         }
      }
   }
   else {  /* user-defined scaling (note that in this case num_vec > 1) */

      for (i=0; i<vec_len; i++) {

         for (j=0, k=i, sum=0; j<num_vec; j++, k+=j*vec_len) sum += scale[j]*x[k];
         y[i] = clip2short(sum);
      }
   }

   if (scale_calloc) free(scale);

   return vec_len;
}


/* check if an array is all zero.  Any array of any dimension can be given, as long as len specifies total length in bytes */

int isArrayZero(uint8_t* array, int len) {

int non_zero = 0;

#if defined(_X86) || defined(_ARM)

   __asm__ (
      "cld\n"
      "xorb %%al, %%al\n"
      "repz scasb\n"
      : "=c" (non_zero)
      : "c" (len), "D" (array)
      : "eax", "cc"
   );

#else

int k;

   for (k=0; k<len; k++) {

      if (array[k]) { non_zero = 1; break; }
   }

#endif

   return !non_zero;
}

int isArrayLess(short int* array, int len, int thresh) {

int not_less = 0, k;

   for (k=0; k<len; k++) {

      if (abs(array[k]) >= thresh) { not_less = 1; break; }
   }

   return !not_less;
}

/* convert from one data type to another 

     -lower 16-bits of uFlags specifies input data type
     -upper 16-bits of uFlags specifies output data type
*/

int DSConvertDataFormat(void *in, void *out, uint32_t uFlags, int length) {

   if (in == NULL) 
   {
      printf("ERROR in DSConvertDataFormat(): NULL input pointer\n");
      return 0;
   }
   if (out == NULL) 
   {
      printf("ERROR in DSConvertDataFormat(): NULL output pointer\n");
      return 0;
   }
   
   switch(uFlags) {

      case DS_CONVERTDATA_SHORT | (DS_CONVERTDATA_FLOAT << 16):
      {
         int i;
         for (i = 0; i < length; i++)
            ((float *)out)[i] = ((short *)in)[i];
         break;
      }
      case DS_CONVERTDATA_FLOAT | (DS_CONVERTDATA_SHORT << 16):
      {
         int i;
         for (i = 0; i < length; i++)
            ((short *)out)[i] = ((float *)in)[i];
         break;
      }
      default:
         printf("ERROR: unsupported data types in DSConvertDataFormat(): 0x%08x", uFlags);
   }
   
   return length;
}

