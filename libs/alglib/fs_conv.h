/*
  fs_conv.h

  Copyright (C) Signalogic, 2001-2022

  Revision History

    Created Apr 2001 Rastko Selmic
    Modified May 2001 Donald Eubanks
    Modified Aug 2013 Jeff Brower
    Modified Mar 2015 Jeff Brower
    Modified Feb 2022 JHB, #ifdef out last remnants. See alglib.h and filt_coeffs.h
*/

#if !defined(_MINCONFIG_) && defined(USE_OLD_FSCONV)

#include "std_rtaf.h"

void fs_conv(Int16*,   /* pointer to data */
             Int16,    /* sampling rate of data, in Hz */
             Int16,    /* up factor */    
             Int16,    /* down factor */
             Int16*,   /* pointer to delay values */
             UInt16);  /* data length */

/*
Sampling Rate (Fs) Definitions
  Narrowband (NB)      8 kHz
  Medium Band (MB)     12 kHz
  Wideband (WB)        16 kHz
  Super Wideband (SWB) 24 kHz
  Fullband (FB)        48 kHz
*/

extern const short int fir_filt_4khz_nb2wb[];  /* NB to WB, 4 kHz FIR lowpass filter (16 kHz Fs) */
extern const UInt16 FIR_FILT_NB2WB_SIZE;

extern const short int fir_filt_4khz_wb2nb[];  /* WB to NB, 4 kHz FIR lowpass filter (16 kHz Fs) */
extern const UInt16 FIR_FILT_WB2NB_SIZE;

extern const short int fir_filt_4khz_nb2fb[];  /* NB to FB, 4 kHz Fc FIR lowpass filter (48 kHz Fs) */
extern const UInt16 FIR_FILT_NB2FB_SIZE;

extern const short int fir_filt_4khz_fb2nb[];  /* FB to NB, 4 kHz Fc FIR lowpass filter (48 kHz Fs) */
extern const UInt16 FIR_FILT_FB2NB_SIZE;

extern const short int fir_filt_6khz_mb2fb[];  /* MB to FB, 6 kHz Fc FIR lowpass filter (48 kHz Fs) */
extern const UInt16 FIR_FILT_MB2FB_SIZE;

extern const short int fir_filt_6khz_fb2mb[];  /* FB to MB, 6 kHz Fc FIR lowpass filter (48 kHz Fs) */
extern const UInt16 FIR_FILT_FB2MB_SIZE;

extern const short int fir_filt_8khz_wb2fb[];  /* WB to FB, 8 kHz Fc FIR lowpass filter (48 kHz Fs) */
extern const UInt16 FIR_FILT_WB2FB_SIZE;

extern const short int fir_filt_8khz_fb2wb[];  /* FB to WB, 8 kHz Fc FIR lowpass filter (48 kHz Fs) */
extern const UInt16 FIR_FILT_FB2WB_SIZE;

#endif

