/*
 filt_coeffs.h

 Description: coefficients for filters used in telecom / audio sampling rate conversion
  
 Copyright (C) Signalogic, 2001-2022

 Revision History
  Created  Feb 2022 JHB, split out from fs_conv.c
*/

/*
 
Sampling Rate (Fs) Definitions

  Narrowband (NB)      8 kHz
  Medium Band (MB)     12 kHz
  Wideband (WB)        16 kHz
  Audio lo-fi          22.05 kHz
  Halfband             24 kHz
  Super Wideband (SWB) 32 kHz
  Audio hi-fi          44.1 kHz
  Fullband (FB)        48 kHz

Filters defined here are:

  -FIR lowpass, even order, symmetric (but with all coefficients defined, as DSConvertFs() has no indexing assumption of one-sided definition)
  -unity gain
  -for fixed-point, quantized to 16-bit
  
In general, filters can be added here with any shape length, gain, and even/odd symmetry or non-symmetric. DSConvertFs() accepts uFlags DS_FSCONV_NO_INTERPOLATE and DS_FSCONV_NO_DECIMATE if only filtering should be performed (and also an optional pointer to user-defined filter coefficients)

Filter specs that produced the coefficients:

  Narrowband to Wideband, Wideband to Super Wideband (e.g. 8 -> 16, 12 -> 24, 24 -> 48)
    -32 tap, 4 kHz Fc, 16 kHz Fs, 500 Hz transition band
    -stopband attenuation 40 dB
    -passband ripple 1 dB

  Wideband to Narrowband, Super Wideband to Wideband
    -64 tap, 4 kHz Fc, 16 kHz Fs, 350 Hz transition band
    -stopband attenuation 40 dB
    -passband ripple 1 dB

  Narrowband to Fullband (8 -> 48)
    -32 tap, 4 kHz Fc, 48 kHz Fs, 850 Hz transition band
    -stopband attenuation 30 dB
    -passband ripple 2 dB

  Fullband to Narrowband
    -64 tap, 4 kHz Fc, 48 kHz Fs, 550 Hz transition band
    -stopband attenuation 30 dB
    -passband ripple 2 dB

  Mediumband to Fullband, Narrowband to Super Wideband (e.g. 12 -> 48, 8 -> 32)
    -32 tap, 6 kHz Fc, 48 kHz Fs, 850 Hz transition band
    -stopband attenuation 30 dB
    -passband ripple 2 dB

  Fullband to Mediumband, Super Wideband to Mediumband
    -64 tap, 6 kHz Fc, 48 kHz Fs, 550 Hz transition band
    -stopband attenuation 30 dB
    -passband ripple 2 dB

  Wideband to Fullband (16 -> 48, 8 -> 24)
    -32 tap, 8 kHz Fc, 48 kHz Fs, 850 Hz transition band
    -stopband attenuation 30 dB
    -passband ripple 2 dB

  Fullband to Wideband
    -64 tap, 8 kHz Fc, 48 kHz Fs, 550 Hz transition band
    -stopband attenuation 30 dB
    -passband ripple 2 dB

  Narrowband to Mediumband, Super Wideband to Fullband (e.g. 8 -> 12, 24 -> 32, 32 -> 48)
    -22 tap, 4 kHz Fc, 12 kHz Fs, 500 Hz transition band
    -stopband attenuation 35 dB
    -passband ripple 1 dB
 
   Mediumband to Narrowband, Fullband to Super Wideband (e.g. 12 -> 8, 32 -> 24, 48 -> 32)
    -28 tap, 4 kHz Fc, 12 kHz Fs, 350 Hz transition band
    -stopband attenuation 30.5 dB
    -passband ripple 1 dB

   44.1 kHz to Fullband
    -88 tap, 22.05 kHz Fc, 48 kHz Fs, 850 Hz transition band
    -stopband attenuation 30 dB
    -passband ripple 2 dB

   Fullband to 44.1 kHz
    -134 tap, 22.05 kHz Fc, 48 kHz Fs, 550 Hz transition band
    -stopband attenuation 30 dB
    -passband ripple 2 dB
*/

#ifndef _FILT_COEFFS_H_
#define _FILT_COEFFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _C66XX_
#pragma DATA_ALIGN(fir_filt_up2, 8)
#endif
const short int fir_filt_up2[32] = {
-84,
-702,
-504,
368,
358,
-592,
-291,
902,
144,
-1334,
185,
2004,
-949,
-3435,
3696,
14765,
14765,
3696,
-3435,
-949,
2004,
185,
-1334,
144,
902,
-291,
-592,
358,
368,
-504,
-702,
-84
};

const float fir_filt_up2_float[32] = { 0 };

const UInt16 FIR_FILT_UP2_SIZE = sizeof(fir_filt_up2)/sizeof(Int16);

#ifdef _C66XX_
#pragma DATA_ALIGN(fir_filt_down2, 8)
#endif
const short int fir_filt_down2[64] = {
-153,
-594,
-350,
124,
222,
-158,
-186,
203,
174,
-258,
-162,
322,
145,
-396,
-118,
482,
77,
-583,
-16,
705,
-74,
-858,
207,
1061,
-414,
-1360,
770,
1881,
-1518,
-3162,
4204,
14360,
14360,
4204,
-3162,
-1518,
1881,
770,
-1360,
-414,
1061,
207,
-858,
-74,
705,
-16,
-583,
77,
482,
-118,
-396,
145,
322,
-162,
-258,
174,
203,
-186,
-158,
222,
124,
-350,
-594,
-153
};

const float fir_filt_down2_float[64] = { 0 };

const UInt16 FIR_FILT_DOWN2_SIZE = sizeof(fir_filt_down2)/sizeof(Int16);

#ifdef _C66XX_
#pragma DATA_ALIGN(fir_filt_up6, 8)
#endif
const short int fir_filt_up6[32] = {
1428,
784,
736,
478,
28,
-534,
-1078,
-1448,
-1499,
-1137,
-346,
794,
2119,
3408,
4429,
4994,
4994,
4429,
3408,
2119,
794,
-346,
-1137,
-1499,
-1448,
-1078,
-534,
28,
478,
736,
784,
1428
};

const float fir_filt_up6_float[32] = { 0 };

const UInt16 FIR_FILT_UP6_SIZE = sizeof(fir_filt_up6)/sizeof(Int16);

#ifdef _C66XX_
#pragma DATA_ALIGN(fir_filt_down6, 8)
#endif
const short int fir_filt_down6[64] = {
-728,
133,
258,
428,
594,
704,
716,
613,
404,
131,
-142,
-348,
-430,
-360,
-152,
138,
425,
618,
639,
455,
89,
-378,
-818,
-1087,
-1056,
-646,
147,
1242,
2485,
3677,
4614,
5129,
5129,
4614,
3677,
2485,
1242,
147,
-646,
-1056,
-1087,
-818,
-378,
89,
455,
639,
618,
425,
138,
-152,
-360,
-430,
-348,
-142,
131,
404,
613,
716,
704,
594,
428,
258,
133,
-728
};

const float fir_filt_down6_float[64] = { 0 };

const UInt16 FIR_FILT_DOWN6_SIZE = sizeof(fir_filt_down6)/sizeof(Int16);

#ifdef _C66XX_
#pragma DATA_ALIGN(fir_filt_up4, 8)
#endif
const short int fir_filt_up4[32] = {
-282,
-1917,
-1278,
-1120,
-354,
507,
1027,
856,
8,
-1078,
-1693,
-1206,
579,
3254,
5923,
7584,
7584,
5923,
3254,
579,
-1206,
-1693,
-1078,
8,
856,
1027,
507,
-354,
-1120,
-1278,
-1917,
-282
};

const float fir_filt_up4_float[32] = { 0 };

const UInt16 FIR_FILT_UP4_SIZE = sizeof(fir_filt_up4)/sizeof(Int16);

#ifdef _C66XX_
#pragma DATA_ALIGN(fir_filt_down4, 8)
#endif
const short int fir_filt_down4[64] = {
-84,
-1180,
-776,
-715,
-386,
29,
339,
397,
190,
-146,
-397,
-395,
-124,
257,
506,
440,
66,
-406,
-673,
-521,
19,
653,
966,
669,
-190,
-1180,
-1652,
-1056,
743,
3331,
5868,
7433,
7433,
5868,
3331,
743,
-1056,
-1652,
-1180,
-190,
669,
966,
653,
19,
-521,
-673,
-406,
66,
440,
506,
257,
-124,
-395,
-397,
-146,
190,
397,
339,
29,
-386,
-715,
-776,
-1180,
-84
};

const float fir_filt_down4_float[64] = { 0 };

const UInt16 FIR_FILT_DOWN4_SIZE = sizeof(fir_filt_down4)/sizeof(Int16);

#ifdef _C66XX_
#pragma DATA_ALIGN(fir_filt_up3, 8)
#endif
const short int fir_filt_up3[32] = {
-399,
1551,
1800,
1474,
349,
-777,
-926,
54,
1165,
1103,
-407,
-2028,
-1720,
1455,
6220,
9755,
9755,
6220,
1455,
-1720,
-2028,
-407,
1103,
1165,
54,
-926,
-777,
349,
1474,
1800,
1551,
-399
};

const float fir_filt_up3_float[32] = { 0 };

const UInt16 FIR_FILT_UP3_SIZE = sizeof(fir_filt_up3)/sizeof(Int16);

#ifdef _C66XX_
#pragma DATA_ALIGN(fir_filt_down3, 8)
#endif
const short int fir_filt_down3[64] = {
890,
130,
-391,
-910,
-1023,
-638,
-82,
182,
-18,
-392,
-484,
-138,
322,
411,
7,
-483,
-510,
25,
604,
578,
-126,
-828,
-714,
272,
1200,
952,
-580,
-2052,
-1580,
1608,
6228,
9611,
9611,
6228,
1608,
-1580,
-2052,
-580,
952,
1200,
272,
-714,
-828,
-126,
578,
604,
25,
-510,
-483,
7,
411,
322,
-138,
-484,
-392,
-18,
182,
-82,
-638,
-1023,
-910,
-391,
130,
890
};

const float fir_filt_down3_float[64] = { 0 };

const UInt16 FIR_FILT_DOWN3_SIZE = sizeof(fir_filt_down3)/sizeof(Int16);

#ifdef _C66XX_
#pragma DATA_ALIGN(fir_filt_up1p5, 8)
#endif
const short int fir_filt_up1p5[44] = {  /* up 3, down 2 */
-85,
157,
-105,
-72,
252,
-259,
24,
317,
-478,
249,
288,
-738,
656,
67,
-997,
1353,
-558,
-1206,
2836,
-2659,
-1323,
18333,
18333,
-1323,
-2659,
2836,
-1206,
-558,
1353,
-997,
67,
656,
-738,
288,
249,
-478,
317,
24,
-259,
252,
-72,
-105,
157,
-85
};

const float fir_filt_up1p5_float[44] = { 0 };

const UInt16 FIR_FILT_UP1P5_SIZE = sizeof(fir_filt_up1p5)/sizeof(Int16);

#ifdef _C66XX_
#pragma DATA_ALIGN(fir_filt_down1p5, 8)
#endif
const short int fir_filt_down1p5[54] = {  /* up 2, down 3 */
144,
-120,
-28,
188,
-208,
33,
216,
-317,
140,
211,
-442,
306,
151,
-572,
549,
5,
-697,
904,
-291,
-803,
1481,
-920,
-881,
2795,
-2936,
-923,
17982,
17982,
-923,
-2936,
2795,
-881,
-920,
1481,
-803,
-291,
904,
-697,
5,
549,
-572,
151,
306,
-442,
211,
140,
-317,
216,
33,
-208,
188,
-28,
-120,
144
};

const float fir_filt_down1p5_float[54] = { 0 };

const UInt16 FIR_FILT_DOWN1P5_SIZE = sizeof(fir_filt_down1p5)/sizeof(Int16);

#ifdef _C66XX_
#pragma DATA_ALIGN(fir_filt_up160_down147, 8)
#endif
const short int fir_filt_up160_down147[88] = {  /* convert audio 44.1 kHz to telecom 48 kHz */
68,
-58,
44,
-26,
4,
22,
-50,
81,
-111,
141,
-167,
190,
-206,
216,
-216,
206,
-186,
154,
-111,
57,
7,
-79,
158,
-240,
323,
-403,
477,
-541,
590,
-620,
627,
-606,
552,
-462,
328,
-145,
-97,
413,
-828,
1389,
-2203,
3545,
-6440,
20135,
20135,
-6440,
3545,
-2203,
1389,
-828,
413,
-97,
-145,
328,
-462,
552,
-606,
627,
-620,
590,
-541,
477,
-403,
323,
-240,
158,
-79,
7,
57,
-111,
154,
-186,
206,
-216,
216,
-206,
190,
-167,
141,
-111,
81,
-50,
22,
4,
-26,
44,
-58,
68
};

const float fir_filt_up160_down147_float[88] = { 0 };

const UInt16 FIR_FILT_UP160_DOWN147_SIZE = sizeof(fir_filt_up160_down147)/sizeof(Int16);

#ifdef _C66XX_
#pragma DATA_ALIGN(fir_filt_up147_down160, 8)
#endif
const short int fir_filt_up147_down160[134] = {  /* convert telecom 48 kHz to audio 44.1 kHz */
-26,
14,
0,
-15,
31,
-47,
62,
-75,
86,
-92,
95,
-93,
86,
-73,
56,
-35,
10,
18,
-48,
77,
-105,
129,
-149,
163,
-170,
169,
-159,
139,
-111,
75,
-32,
-16,
68,
-120,
172,
-219,
259,
-289,
307,
-312,
300,
-272,
228,
-166,
90,
-1,
-99,
205,
-314,
421,
-520,
605,
-672,
714,
-725,
699,
-631,
514,
-339,
98,
224,
-653,
1235,
-2075,
3448,
-6377,
20098,
20098,
-6377,
3448,
-2075,
1235,
-653,
224,
98,
-339,
514,
-631,
699,
-725,
714,
-672,
605,
-520,
421,
-314,
205,
-99,
-1,
90,
-166,
228,
-272,
300,
-312,
307,
-289,
259,
-219,
172,
-120,
68,
-16,
-32,
75,
-111,
139,
-159,
169,
-170,
163,
-149,
129,
-105,
77,
-48,
18,
10,
-35,
56,
-73,
86,
-93,
95,
-92,
86,
-75,
62,
-47,
31,
-15,
0,
14,
-26
};

const float fir_filt_up147_down160_float[134] = { 0 };

const UInt16 FIR_FILT_UP147_DOWN160_SIZE = sizeof(fir_filt_up147_down160)/sizeof(Int16);

#ifdef __cplusplus
}
#endif

#endif /* _FILT_COEFFS_H_ */
