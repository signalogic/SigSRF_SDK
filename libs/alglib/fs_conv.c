/*
 fs_conv.c

 Description: sampling rate conversion for common telecom / audio sampling sampling rates, using up/down conversion by integer ratios, for example 2, 3, 4, 6, 2/3, 4/3, etc
  
 Copyright (C) Signalogic, 2001-2022

 Revision History

   Created  April 2001 Rastko Selmic
   Modified May 2001 Donald Eubanks
   Modified Aug 2013 Jeff Brower
   Modified 2014 CKJ use optimized TI FIR filter functions if _USE_DSPLIB_ is defined
   Modified Mar 2015 JHB, fs_conv() updated from Convolve() function in C6xxx_Lib.c
   Modified Aug 2016 JHB, SC, to ensure no action if both up_factor and down_factor == 1
   Modified Apr 2017 CKJ, add x86 compatibility
   Modified Aug 2017 JHB, modified DSConvertFs() to return amount of valid output data after Fs conversion
   Modified Mar 2018 JHB, replaced _X86_ with _X86.  Change was made to maintain consistency with shared_include / coCPU usage.  _X86, _ARM, _TI66X, etc are "chip family" defines, specific chips are _X5_E930_, _C66XX_, etc)
   Modified Mar 2018 JHB, increased size of temp_store[].  See comments
   Modified Jun 2018 JHB, removed reference to voplib.h (this affected how temp_store[] is declared)
   Modified Mar 2019 JHB, fix problem with _X86 build (#else section of #ifdef _USE_DSPLIB_) where if both up_factor and down_factor = 1, it still did convolution
   Modified Jul 2019 JHB, add num_chan param, allow pData to point to start of a channel within interleaved (multichannel) data.  Note that pData must point to the channel's first value
   Modified Feb 2022 JHB, allow non-symmetric filters with odd or even length, make multiply/add loop more efficient (see USE_OLD_INDEXING), base filter choice on ratios of up_factor and down_factor as opposed to specific values. For example if user gives up 4 down 2 (instead of 2:1 for whatever reason) we handle it
   Modified Feb 2022 JHB, implement additional filters: 3/2 and 2/3 up and down, 44.1 to 48 and 48 to 44.1 kHz
   Modified Feb 2022 JHB, for large up/down factors (e.g. 44.1 <--> 48) (i) fix problem with int16 data_len overflow (change all input params to int) and (ii) increase size of temp_store (see notes)
   Modified Feb 2022 JHB, split coefficients out to filt_coeffs.h, add user-defined filter params (pFilt_user and filt_len_user), lay groundwork for floating-point filters
*/

#include <stdint.h>
#include <stdio.h>

#include "std_rtaf.h"
#include "fs_conv.h"
#include "alglib.h"
#include "voplib.h"
#include "diaglib.h"

#ifndef _X86
#include "debug.h"
#endif

#ifdef _C66XX_

  #define _USE_DSPLIB_

  //#include <ti/dsplib/src/DSP_fir_sym/c66/DSP_fir_sym.h>
  #include <ti/dsplib/src/DSP_fir_r8/c66/DSP_fir_r8.h>
  //#include <ti/dsplib/src/DSP_fir_r8_hM16_rM8A8X8/c66/DSP_fir_r8_hM16_rM8A8X8.h>

  extern short int fs_temp_buffer[];

#endif

#ifndef _MINCONFIG_
  #include "filt_coeffs.h" /* coeff header file created Feb2022 */ 
#endif

/* Filter coefficients are defined in filt_coeffs.h, and filter specs are given in those header files. Some basic info on filter specs:

Sampling Rate (Fs) Definitions

  Narrowband (NB)      8 kHz
  Medium Band (MB)     12 kHz
  Wideband (WB)        16 kHz
  Audio lo-fi          22.05 kHz
  Halfband             24 kHz
  Super Wideband (SWB) 32 kHz
  Audio hi-fi          44.1 kHz
  Fullband (FB)        48 kHz

Filter coefficients defined in filt_coeffs.h are:

  -FIR, even order, symmetric (but with all coefficients defined, there is no indexing assumption of symmetry or one-sided definition)
  -unity gain
  -quantized to 16-bit 

In general, filters can be added in the coefficient header files with any length, gain, and even/odd symmetry or non-symmetric. Floating-point coefficient support is being added, DS_FSCONV_FLOATING_POINT uFlags option can be used to specify floating-point filters

Below is an outline of algorithm used for sampling rate conversion, including interpolation and/or decimation, and convolution of two 16-bit waveforms:

             filt_len
              -----/
               \    
        y[n] =   >    x[n-k] * h[k]  
               /  
              -----\
               k=0

  x[n] -  Waveform data

       -filt_len zero values are assumed prior to the start of x[n] data
       -filt_len number of recent input values are saved for use in subsequent call

  h[n] - filter coefficients

  filt_len: length of filter waveform

  Note the operation is performed in-place; pData contains input data on entry and output data on exit.

Function Params

  pData        pointer to start of 16-bit x[n] waveform data on entry, pointer to start of 16-bit y[n] waveform data on exit
  fs           sampling rate, in Hz
  up_factor    up factor -- amount to multiply Fs
  down_factor  down factor -- amount to divide Fs
  pDelay       pointer to delay values (must be a per channel, saved area)
  data_len     length of x[n]

Return Value (only if _X86 defined)

  Amount of valid data in pData; for example, for a down conversion by 2, then this would be data_len/2

Example usage:
  
  fs = 8000;
  up_factor = 6; // 8 to 48 kHz, multiply by 6, divide by 1
  dn_factor = 1;

  fs = 8000;
  up_factor = 3; // 8 to 12 kHz, multiply by 3, divide by 2
  dn_factor = 2;

  fs = 48000;
  up_factor = 1; // 48 to 16 kHz, divide by 3
  dn_factor = 3;
  
  DSConvertFs(&inBuf[i*RTP_NSAMPLES], fs, up_factor, dn_factor, FirDelayBuf, RTP_NSAMPLES, 1)
*/  

#ifdef _X86
int DSConvertFs(
#else
void fs_conv(
#endif
              void*        pData_user,    /* pointer to input and output data. Assumed to be interleaved by num_chan for multichannel data */
              int          fs,            /* sampling rate of data, in Hz (currently not used) */
              int          up_factor,     /* up factor */
              int          down_factor,   /* down factor */
              void*        pDelay_user,   /* pointer to delay values (per channel) */
              int          data_len,      /* data length, in samples */
              int          num_chan,      /* number of channels contained in pData */
              void*        pFilt_user,    /* user-defined filter coefficients (typically NULL for Fs conversion) */ 
              int          filt_len_user, /* length of user-defined filter (only used if pFilt_user not NULL) */
              unsigned int uFlags
            ) {

const int16_t* pFilt = NULL;
const float* pFilt_f = NULL;

//#define USE_OLD_INDEXING

#ifndef _USE_DSPLIB_
  int sav_i, index;
  #ifdef USE_OLD_INDEXING
  int flen2, j2;
  #endif
  long long sum;
  #ifdef _X86
  int16_t temp_store[/*24576*/MAX_FSCONV_UP_DOWN_FACTOR*MAX_SAMPLES_FRAME];  /* temporary output storage allows in-place processing.  Increased from 1024 to allow 24x sampling rate down-conversion (see additional comments in voplib.h), JHB Mar2018. Increased again to allow 44.1 to 48 kHz conversion with input data_len of MAX_SAMPLES_FRAME (960) * 160. Note this size may be sensitive to caller amount of stack used; for example mediaTest uses a huge amount of stack and a size value here of 1MByte seems to cause a seg fault, JHB Feb2022 */
  #else
  int16_t temp_store[MAX_SAMPLES_FRAME];  /* temporary output storage allows in-place processing */
  #endif
#endif

int i, j, filt_len;
int16_t* pData = NULL, *pDelay = NULL;
float* pData_f = NULL, *pDelay_f = NULL;

   if (uFlags & DS_FSCONV_FLOATING_POINT) {
      pData_f = (float*)pData_user;
      pDelay_f = (float*)pDelay_user;
   }
   else {  /* default */
      pData = (int16_t*)pData_user;
      pDelay = (int16_t*)pDelay_user;
   }

   bool fNoRatio = (up_factor == 0 || down_factor == 0);

   if (pFilt_user != NULL) {
      if (uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = (const float*)pFilt_user;
      else pFilt = (int16_t*)pFilt_user;
      filt_len = filt_len_user;
   }
   else if (!fNoRatio && up_factor/down_factor == 2) {
      if (uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = fir_filt_up2_float;
      else pFilt = fir_filt_up2;
      filt_len = FIR_FILT_UP2_SIZE;
   }
   else if (!fNoRatio && up_factor/down_factor == 3) {
      if (uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = fir_filt_up3_float;
      else pFilt = fir_filt_up3;
      filt_len = FIR_FILT_UP3_SIZE;
   }
   else if (!fNoRatio && up_factor/down_factor == 4) {
      if (uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = fir_filt_up4_float;
      else pFilt = fir_filt_up4;
      filt_len = FIR_FILT_UP4_SIZE;
   }
   else if (up_factor/down_factor == 6) {
      if (!fNoRatio && uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = fir_filt_up6_float;
      else pFilt = fir_filt_up6;
      filt_len = FIR_FILT_UP6_SIZE;
   }
   else if (!fNoRatio && 2*up_factor/3 == down_factor) {
      if (uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = fir_filt_up1p5_float;
      else pFilt = fir_filt_up1p5;
      filt_len = FIR_FILT_UP1P5_SIZE;
   }
   else if (!fNoRatio && down_factor/up_factor == 2) {
      if (uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = fir_filt_down2_float;
      else pFilt = fir_filt_down2;
      filt_len = FIR_FILT_DOWN2_SIZE;
   }
   else if (!fNoRatio && down_factor/up_factor == 3) {
      if (uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = fir_filt_down3_float;
      else pFilt = fir_filt_down3;
      filt_len = FIR_FILT_DOWN3_SIZE;
   }
   else if (!fNoRatio && down_factor/up_factor == 4) {
      if (uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = fir_filt_down4_float;
      else pFilt = fir_filt_down4;
      filt_len = FIR_FILT_DOWN4_SIZE;
   }
   else if (!fNoRatio && down_factor/up_factor == 6) {
      if (uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = fir_filt_down6_float;
      else pFilt = fir_filt_down6;
      filt_len = FIR_FILT_DOWN6_SIZE;
   }
   else if (!fNoRatio && 2*down_factor/3 == up_factor) {
      if (uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = fir_filt_down1p5_float;
      else pFilt = fir_filt_down1p5;
      filt_len = FIR_FILT_DOWN1P5_SIZE;
   }
   else if (!fNoRatio && up_factor*44100/48000 == down_factor) {  /* 44.1 to 48, 22.05 to 24, 11.025 to 12, etc */
      if (uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = fir_filt_up160_down147_float;
      else pFilt = fir_filt_up160_down147;
      filt_len = FIR_FILT_UP160_DOWN147_SIZE;
   }
   else if (!fNoRatio && down_factor*44100/48000 == up_factor) {  /* 48 to 44.1, 24 to 22.05, etc */
      if (uFlags & DS_FSCONV_FLOATING_POINT) pFilt_f = fir_filt_up147_down160_float;
      else pFilt = fir_filt_up147_down160;
      filt_len = FIR_FILT_UP147_DOWN160_SIZE;
   }
#if !defined(_USE_DSPLIB_) && defined(USE_OLD_INDEXING)
   flen2 = filt_len/2;
#endif

   #if 0  /* debug */
   static int fOnce = 0;
   if (!fOnce) { printf(" inside alglib DSConvertFs(), upf = %d, dnf = %d, num_chan = %d, filt_len = %d \n", up_factor, down_factor, num_chan, filt_len); fOnce = 1; }
   #endif

   if (!(uFlags & DS_FSCONV_FLOATING_POINT)) {

   /* make sure pointers are valid before continuing */

      if (pData == NULL) {
         Log_RT(2, "ERROR: DSConvertFs() says no fixed-point I/O data specified \n");
         return -1;
      }

      if (pFilt == NULL && (up_factor > 1 || down_factor > 1) && !(uFlags & DS_FSCONV_NO_FILTER)) {  /* case where ratio doesn't match an availabile filter */
         Log_RT(2, "ERROR: DSConvertFs() says no fixed-point filter specified, up_factor = %d, down_factor = %d \n", up_factor, down_factor);
         return -1;
      }

   /* interpolation */

      if (up_factor > 1 && !(uFlags & DS_FSCONV_NO_INTERPOLATE)) {

         for (i=data_len-1; i>=0; i--)
            for (j=0; j<up_factor; j++)
              pData[num_chan*(up_factor*i+j)] = pData[num_chan*i];

         data_len *= up_factor;
      }

   /* FIR filtering -- use either TI DSPLIB method or straight C code.  Use same FIR filters in either case */

   #ifdef _USE_DSPLIB_  /* use optimized TI lib functions */

      #if 0
      if (up_factor > 1 || down_factor > 1) { /*JB Mod RTPWB Fix Aug2016*/
      #else
      if (pFilt && !(uFlags & DS_FSCONV_NO_FILTER)) {
      #endif

         if (pDelay != NULL) memcpy(fs_temp_buffer, pDelay, (filt_len-1)*sizeof(short int));  /* added pDelay != NULL JHB Aug2016*/

         memcpy(&fs_temp_buffer[(filt_len-1)], pData, data_len*sizeof(short int));  /* copy saved delay elements to start of data */
         //DSP_fir_sym(fs_temp_buffer, pFilt, pData, flen2, data_len, 15);
         DSP_fir_r8(fs_temp_buffer, pFilt, pData, filt_len, data_len);                   // most efficient for 64 filter coefficients or less
         //DSP_fir_r8_hM16_rM8A8X8(fs_temp_buffer, pFilt, pData, filt_len, data_len);     // most efficient for more than 64 filter coefficients

         if (pDelay != NULL) memcpy(pDelay, &fs_temp_buffer[data_len], (filt_len-1)*sizeof(short int));  /* added pDelay != NULL JHB Aug2016*/

         //Dsp_Log(0, "DEBUG: filt_len = %d, flen2 = %d, data_len = %d, up_factor = %d, down_factor = %d\n", filt_len, flen2, data_len, up_factor, down_factor);
      } /* if (up_factor > 1 || down_factor > 1) *//*JB Mod RTPWB Aug 2016*/

   #else

   /* convolution (apply filter) */

      #if 0
      if (up_factor > 1 || down_factor > 1) {  /* don't modify input data if no up/down conversion specified, JHB Mar 2019 */
      #else
      if (pFilt && !(uFlags & DS_FSCONV_NO_FILTER)) {  /* note that pFilt won't be set if both up_factor and down_factor are 1 and a user-defined filter is not given, JHB Feb2022 */
      #endif
   
         for (i=0; i<data_len; i++) { /* x[i] */

            sum = 0;
   
            for (j=0; j<filt_len; j++) {  /* h[j] */

               index = i-j;  /* reverse index for convolution (use i+j for correlation) */

               #ifdef USE_OLD_INDEXING
               if (j > flen2) j2 = filt_len-1-j;  /* assume symmetric, even-numbered filters always */
               else j2 = j;

               if (index >= 0) sum += pData[index*num_chan] * pFilt[j2];  /* multiply and add */
               else sum += pDelay[filt_len + index] * pFilt[j2];

               #else  /* allow even/odd length, non-symmetric filters, JHB Feb2022 */

               if (index >= 0) sum += pData[index*num_chan] * pFilt[j];  /* multiply and add from input data */
               else sum += pDelay[filt_len + index] * pFilt[j];  /* multiply and add from delay line data (which is per channel, managed by caller) */
               #endif
            }

            #if 0
            sum *= 0.8;  /* although filters are designed for unity gain, if coefficients are quantized fixed-point then slight numerical error could cause infrequent overflow if (i) input has sustained values near +/- max fixed-point and (ii) filter length is long, JHB Feb2022 */

            //static int fOnce2 = 0;
            //if (!fOnce2 && (sum > (32768*32768-1) || sum < -32768*32768)) { printf(" overflow, sum = %lld \n", sum); fOnce2 = 1; }
            #endif

            temp_store[i] = sum >> 15;  /* store 16-bit output in temporary array */
         }

         sav_i = data_len - filt_len;

         for (i=0; i<data_len; i++) {

            if (i >= sav_i) pDelay[i - sav_i] = pData[i*num_chan];  /* update delay line (save most recent filt_len samples for next call) */

            pData[i*num_chan] = temp_store[i];  /* update output */
         }
      }

   #endif  /* _USE_DSPLIB_ */

   /* decimation */

      if (down_factor > 1 && !(uFlags & DS_FSCONV_NO_DECIMATE)) {

         data_len /= down_factor;

         for (i=0; i<data_len; i++) pData[i*num_chan] = pData[down_factor*i*num_chan];
      }
   }
   else {  /* floating-point implementation */

      (void)pDelay_f;

   /* make sure pointers are valid before continuing */

      if (pData_f == NULL) {
         Log_RT(2, "ERROR: DSConvertFs() says no floating-point I/O data specified \n");
         return -1;
      }

      if (pFilt_f == NULL && (up_factor > 1 || down_factor > 1) && !(uFlags & DS_FSCONV_NO_FILTER)) {  /* case where ratio doesn't match an availabile filter */
         Log_RT(2, "ERROR: DSConvertFs() says no floating-point filter specified, up_factor = %d, down_factor = %d \n", up_factor, down_factor);
         return -1;
      }
   }

#ifdef _X86
   return data_len;
#endif
}