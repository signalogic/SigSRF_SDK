/*
  codec.h

  Voice / video codec support for c66x, x86, Arm, or combined coCPU platforms

  Copyright (C) Signalogic, Inc, 2008-2012

    Created for c64x voice platforms

  Copyright (C) Mavenir Systems and Signalogic, Inc, 2013-2015
  
    Support for c66x coCPU card PCIe and ATCA blade SRIO interfaces

  Copyright (C) Signalogic, Inc, 2016-2025

    Add APIs to (i) encapsulate c66x PCIe and SRIO interfaces, and (ii) support x86-only or combined x86 and c66x server usage.  APIs are consistent between all use cases

  License
  
    Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

  Revision History

    Created Oct 2023 JHB, from session.h, codec related items separated to more cleanly support codec-only applications
    Modified Dec 2023 JHB, comments only
    Modified Feb 2024 JHB, add DS_CODEC_VOICE_L16 definition (linear PCM 16-bit)
    Modified May 2024 JHB, change #ifdef _X86 to #if defined(_X86) || defined(_ARM)
    Modified Jun 2024 JHB, make video codecs enums sequential with voice codecs enums, define generic DS_NUM_VOICE_CODECS and DS_NUM_VIDEO_CODECS
    Modified Jul 2024 JHB, define isVoiceCodec() and isVideoCodec() macros
    Modified Sep 2024 JHB, add audio_codecs enums, define isAudioCodec() macro, add get_codec_type_from_name()
    Modified Nov 2024 JHB, rename codec enums to start with DS_CODEC_XXX for documentation purposes
    Modified Dec 2024 JHB, for documentation purposes, combine voice_codec_types, audio_codec_types, and video_codec_types into one codec_types enum definition; define a codec_types typedef
    Modified Dec 2024 JHB, move get_codec_name() here from voplib.c, implement fVerbose option, add additional codec macros, e.g. isAMRCodec()
    Modified Jan 2025 JHB, add compile-time assert if size of codec_types enum is not equal to int
    Modified May 2025 JHB, thanks to a user question about C++11, specify type (and size) for media_types and codec_types enums. This is important for structs that contain an enum typedef, like PAYLOAD_INFO in voplib.h Enum sizes should be int for C++03 and earlier but enum scoping was not supported then
*/

#ifndef _CODEC_H_
#define _CODEC_H_

#if __cplusplus >= 201103
enum media_types: int {
#else
enum media_types {
#endif

  DS_MEDIA_TYPE_VOICE,
  DS_MEDIA_TYPE_AUDIO,
  DS_MEDIA_TYPE_VIDEO
};

#if __cplusplus >= 201103
enum codec_types: int {
#else
enum codec_types {
#endif

  DS_CODEC_NONE,                                      /* 0, pass-thru */

  DS_VOICE_CODECS_MIN = DS_CODEC_NONE,                /* inclusive */

  DS_CODEC_VOICE_G711_ULAW,                           /* 1 */
  DS_CODEC_VOICE_G711_ALAW,                           /* 2 */
  DS_CODEC_VOICE_G711_WB_ULAW,                        /* 3, G711.1 */ 
  DS_CODEC_VOICE_G711_WB_ALAW,                        /* 4, G711.1 */
  DS_CODEC_VOICE_G726,                                /* 5 */
  DS_CODEC_VOICE_G729AB,                              /* 6 */
  DS_CODEC_VOICE_G723,                                /* 7 */
  DS_CODEC_VOICE_AMR_NB,                              /* 8 */
  DS_CODEC_VOICE_AMR_WB,                              /* 9 */
  DS_CODEC_VOICE_EVRC,                                /* 10 */
  DS_CODEC_VOICE_ILBC,                                /* 11 */
  DS_CODEC_VOICE_ISAC,                                /* 12 */
  DS_CODEC_VOICE_OPUS,                                /* 13 */
  DS_CODEC_VOICE_EVRCB,                               /* 14 */
  DS_CODEC_VOICE_GSMFR,                               /* 15 */
  DS_CODEC_VOICE_GSMHR,                               /* 16 */
  DS_CODEC_VOICE_GSMEFR,                              /* 17 */
  DS_CODEC_VOICE_G722,                                /* 18 */
  DS_CODEC_VOICE_EVRC_NW,                             /* 19 */
  DS_CODEC_VOICE_CLEARMODE,                           /* 20 */
  DS_CODEC_VOICE_EVS,                                 /* 21 */
  DS_CODEC_VOICE_MELPE,                               /* 22 */
  DS_CODEC_VOICE_AMR_WB_PLUS,                         /* 23 */

  DS_CODEC_VOICE_RESERVED1,
  DS_CODEC_VOICE_RESERVED2,
  DS_CODEC_VOICE_RESERVED3,
  DS_CODEC_VOICE_RESERVED4,
  DS_CODEC_VOICE_RESERVED5,
  DS_CODEC_VOICE_RESERVED6,
  DS_CODEC_VOICE_RESERVED7,
  DS_CODEC_VOICE_RESERVED8,
  
  DS_VOICE_CODECS_UPPER_BOUND,                        /* exclusive */

  DS_NUM_VOICE_CODECS = (DS_VOICE_CODECS_UPPER_BOUND - DS_VOICE_CODECS_MIN),

  DS_AUDIO_CODECS_MIN = DS_VOICE_CODECS_UPPER_BOUND,  /* inclusive */

  DS_CODEC_AUDIO_L16 = DS_AUDIO_CODECS_MIN,           /* linear 16-bit PCM */
  DS_CODEC_AUDIO_MP3,

  DS_CODEC_AUDIO_RESERVED1,
  DS_CODEC_AUDIO_RESERVED2,
  DS_CODEC_AUDIO_RESERVED3,
  DS_CODEC_AUDIO_RESERVED4,

  DS_AUDIO_CODECS_UPPER_BOUND,                        /* exclusive */

  DS_NUM_AUDIO_CODECS = (DS_AUDIO_CODECS_UPPER_BOUND - DS_AUDIO_CODECS_MIN),

  DS_VIDEO_CODECS_MIN = DS_AUDIO_CODECS_UPPER_BOUND,  /* inclusive */

  DS_CODEC_VIDEO_MPEG2 = DS_VIDEO_CODECS_MIN,
  DS_CODEC_VIDEO_H263,
  DS_CODEC_VIDEO_H264,
  DS_CODEC_VIDEO_H265,
  DS_CODEC_VIDEO_VP8,
  DS_CODEC_VIDEO_VP9,

  DS_CODEC_VIDEO_RESERVED1,
  DS_CODEC_VIDEO_RESERVED2,
  DS_CODEC_VIDEO_RESERVED3,
  DS_CODEC_VIDEO_RESERVED4,
  DS_CODEC_VIDEO_RESERVED5,
  DS_CODEC_VIDEO_RESERVED6,
  DS_CODEC_VIDEO_RESERVED7,
  DS_CODEC_VIDEO_RESERVED8,

  DS_VIDEO_CODECS_UPPER_BOUND,                        /* exclusive */

  DS_NUM_VIDEO_CODECS = (DS_VIDEO_CODECS_UPPER_BOUND - DS_VIDEO_CODECS_MIN)
};

typedef enum codec_types codec_types;

STATIC_ASSERT(sizeof(codec_types) == 4)  /* this is here to cause a compile-time error if for any reason codec_types enums are not equivalent to int (i.e. size 4 bytes). STATIC_ASSERT is defined in alias.h, JHB Jan 2025 */

#define DS_TOTAL_NUM_CODECS (DS_NUM_VOICE_CODECS + DS_NUM_AUDIO_CODECS + DS_NUM_VIDEO_CODECS)

#define isVoiceCodec(codec_type)  ((int8_t)(codec_type) >= DS_VOICE_CODECS_MIN && (int8_t)(codec_type) < DS_VOICE_CODECS_UPPER_BOUND)  /* we limit to int8_t because codec_type field is int8_t in TERMINATION_INFO struct in session.h */
#define isAudioCodec(codec_type)  ((int8_t)(codec_type) >= DS_AUDIO_CODECS_MIN && (int8_t)(codec_type) < DS_AUDIO_CODECS_UPPER_BOUND)
#define isVideoCodec(codec_type)  ((int8_t)(codec_type) >= DS_VIDEO_CODECS_MIN && (int8_t)(codec_type) < DS_VIDEO_CODECS_UPPER_BOUND)

#define isAMRCodec(codec_type)    ((codec_type) == DS_CODEC_VOICE_AMR_NB || (codec_type) == DS_CODEC_VOICE_AMR_WB || (codec_type) == DS_CODEC_VOICE_AMR_WB_PLUS)
#define isEVSCodec(codec_type)    ((codec_type) == DS_CODEC_VOICE_EVS)

#define CODEC_NAME_MAXLEN  50

#ifdef _GNU_SOURCE  /* _GNU_SOURCE must already be defined; typically done at application level or in Makefile build options */

#ifdef __STDC_LIB_EXT1__  /* per https://stackoverflow.com/questions/40045973/strcpy-s-not-working-with-gcc, __STDC_LIB_EXT1__ must be defined otherwise strxxx_s() functions are not available with gcc version in use, JHB Jul 2023 */
  #define __STDC_WANT_LIB_EXT1__ 1
#else
  #include "dsstring.h"  /* sig version of strncpy_s() */
#endif

#include <string.h>  /* strcasestr(), strncpy_s() if __STDC_LIB_EXT1__ defined */

/* for any SigSRF lib or application that needs codec names, or a new one should be added, these are the standardized name definitions. The DSGetCodecInfo() API in voplib calls get_codec_name() when given the DS_CODEC_INFO_NAME flag and calls get_codec_type_from_name() when given the DS_CODEC_INFO_TYPE_FROM_NAME flag. Codec name string length should be less than CODEC_NAME_MAXLEN */

/* return codec name given a codec_type */

static inline int get_codec_name(codec_types codec_type, bool fVerbose, char* codecstr) {

   if ((int8_t)codec_type < 0) return -1;

   switch (codec_type) {

      case DS_CODEC_VOICE_AMR_NB:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "AMR-NB", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_VOICE_AMR_WB:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "AMR-WB", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_VOICE_AMR_WB_PLUS:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "AMR-WB+", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_VOICE_EVS:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "EVS", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_VOICE_G729AB:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "G729AB", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_VOICE_G726:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "G726", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_VOICE_MELPE:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "MELPe", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_VOICE_G711_ULAW:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "G711u", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_VOICE_G711_ALAW:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "G711a", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_AUDIO_L16:
         if (fVerbose) strncpy_s(codecstr, CODEC_NAME_MAXLEN, "L16 (linear 16-bit PCM)", CODEC_NAME_MAXLEN);
         else strncpy_s(codecstr, CODEC_NAME_MAXLEN, "L16", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_VIDEO_H263:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "H.263", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_VIDEO_H264:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "H.264", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_VIDEO_H265:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "H.265", CODEC_NAME_MAXLEN);
         break;

      case DS_CODEC_NONE:
         if (fVerbose) strncpy_s(codecstr, CODEC_NAME_MAXLEN, "None (pass-thru)", CODEC_NAME_MAXLEN);
         else strncpy_s(codecstr, CODEC_NAME_MAXLEN, "None", CODEC_NAME_MAXLEN);
         break;

      default:
         strncpy_s(codecstr, CODEC_NAME_MAXLEN, "undefined", CODEC_NAME_MAXLEN);  /* codec type not recognized */
         return 0;
   }

   return strlen(codecstr);
}

/* return a codec type given a codec name */

static inline int get_codec_type_from_name(const char* codecstr) {

   if (strcasestr(codecstr, "NONE"))  /* anything starting with None ... */
      return DS_CODEC_NONE;
   else if (strcasestr(codecstr, "G711_ULAW") || strcasestr(codecstr, "G711u"))
      return DS_CODEC_VOICE_G711_ULAW;
   else if (strcasestr(codecstr, "G711_ALAW") || strcasestr(codecstr, "G711a"))
      return DS_CODEC_VOICE_G711_ALAW;
   else if (strcasestr(codecstr, "G711_ULAW"))
      return DS_CODEC_VOICE_G711_ULAW;
   else if (strcasestr(codecstr, "G711_WB_ULAW") || strcasestr(codecstr, "G711-WBu"))
      return DS_CODEC_VOICE_G711_WB_ULAW;
   else if (strcasestr(codecstr, "G711_WB_ALAW") || strcasestr(codecstr, "G711-WBa"))
      return DS_CODEC_VOICE_G711_WB_ALAW;
   else if (strcasestr(codecstr, "G726"))
      return DS_CODEC_VOICE_G726;
   else if (strcasestr(codecstr, "G729AB"))
      return DS_CODEC_VOICE_G729AB;
   else if (strcasestr(codecstr, "G723"))
      return DS_CODEC_VOICE_G723;
   else if (strcasestr(codecstr, "G722"))
      return DS_CODEC_VOICE_G722;
   else if (strcasestr(codecstr, "AMR_NB") || strcasestr(codecstr, "AMR-NB"))
      return DS_CODEC_VOICE_AMR_NB;
   else if (strcasestr(codecstr, "AMR_WB_PLUS") || strcasestr(codecstr, "AMR-WB+"))  /* needs to be before AMR-WB */
      return DS_CODEC_VOICE_AMR_WB_PLUS;
   else if (strcasestr(codecstr, "AMR_WB") || strcasestr(codecstr, "AMR-WB"))
      return DS_CODEC_VOICE_AMR_WB;
   else if (strcasestr(codecstr, "EVRCA"))
      return DS_CODEC_VOICE_EVRC;
   else if (strcasestr(codecstr, "ILBC"))
      return DS_CODEC_VOICE_ILBC;
   else if (strcasestr(codecstr, "ISAC"))
      return DS_CODEC_VOICE_ISAC;
   else if (strcasestr(codecstr, "OPUS"))
      return DS_CODEC_VOICE_OPUS;
   else if (strcasestr(codecstr, "EVRCB"))
      return DS_CODEC_VOICE_EVRCB;
   else if (strcasestr(codecstr, "GSMFR") || strcasestr(codecstr, "GSM-FR"))
      return DS_CODEC_VOICE_GSMFR;
   else if (strcasestr(codecstr, "GSMHR") || strcasestr(codecstr, "GSM-HR"))
      return DS_CODEC_VOICE_GSMFR;
   else if (strcasestr(codecstr, "GSMEFR") || strcasestr(codecstr, "GSM-EFR"))
      return DS_CODEC_VOICE_GSMEFR;
   else if (strcasestr(codecstr, "EVRCNW"))
      return DS_CODEC_VOICE_EVRC_NW;
   else if (strcasestr(codecstr, "CLEARMODE"))
      return DS_CODEC_VOICE_CLEARMODE;
   else if (strcasestr(codecstr, "EVS"))
      return DS_CODEC_VOICE_EVS;
   else if (strcasestr(codecstr, "MELPe"))  /* added Apr 2018, CKJ */
      return DS_CODEC_VOICE_MELPE;
   else if (strcasestr(codecstr, "L16"))  /* linear 16-bit PCM */
      return DS_CODEC_AUDIO_L16;
   else if (strcasestr(codecstr, "MP3"))
      return DS_CODEC_AUDIO_MP3;
   else if (strcasestr(codecstr, "MPEG2"))
      return DS_CODEC_VIDEO_MPEG2;
   else if (strcasestr(codecstr, "H.263"))
      return DS_CODEC_VIDEO_H263;
   else if (strcasestr(codecstr, "H.264"))
      return DS_CODEC_VIDEO_H264;
   else if (strcasestr(codecstr, "H.265"))
      return DS_CODEC_VIDEO_H265;
   else if (strcasestr(codecstr, "VP8"))
      return DS_CODEC_VIDEO_VP8;
   else if (strcasestr(codecstr, "VP9"))
      return DS_CODEC_VIDEO_VP9;

   else return -1;  /* unrecognized codec name */
}

#endif  /* _GNU_SOURCE */

/* default value 0, no dtmf detection or transcoding needed. */
enum dtmf_processing {

   DS_DTMF_NONE = 0x00,       /* equivalent to pass-through */
   DS_DTMF_RTP = 0x01,        /* detect DTMF event and forward in outgoing packets as DTMF event */
   DS_DTMF_TONE = 0x02,       /* detect DTMF event from audio tones, and forward in outgoing packets as audio tones, unless combined with DS_DTMF_RTP, in which case forward as DTMF events */
   DS_DTMF_STRIP = 0x04,      /* no outgoing RTP event packet or audio tone (can be combined with other flags) */
   DS_DTMF_SIP_INFO = 0x08    /* Reserved */
};

/* default value 0, no ec */
enum ec_type {

  DS_EC_NONE,                 /* NONE */
  DS_EC_TI_LEC,               /* Telogy line ec */
  DS_EC_TI_LEC_ACOUSTIC       /* Telinnovations line/acoustic ec */
};

/* VAD: 0 = none, 1= enabled */
#define VOICE_ATTR_FLAG_VAD 0x01

/* 
 * comfort noise:  0 = none, 1 = enabled.  
 * When enabled, assumes use of VAD (discontinuous transmission) 
*/
#define VOICE_ATTR_FLAG_CNG 0x02

/* AMR codec flags */
enum amr_codec_flags
{
    DS_AMR_CHANNELS             = 0x00000007,   /* Possible values = 1 - 6 */
    DS_AMR_OCTET_ALIGN          = 0x00000008,
    DS_AMR_CRC                  = 0x00000010,
    DS_AMR_ROBUST_SORTING       = 0x00000020,
    DS_AMR_INTERLEAVING         = 0x00000040,
    DS_AMR_MODE_CHANGE_PERIOD   = 0x00000080,   /* 0 = 1, 1 = 2 */
    DS_AMR_MODE_CHANGE_CAP      = 0x00000100,   /* 0 - 1, 1 = 2 */
    DS_AMR_MODE_CHANGE_NEIGH    = 0x00000200
};

/*
  EVRC codec flags
  silencesupp is set using VOICE_ATTR_FLAG_VAD
*/
enum evrc_codec_flags
{
    DS_EVRC_FRAME_SIZE      = 0x00000001,       /* 0 = 8 khz, 1 = 16 kHz */
    DS_EVRC_FIXED_RATE      = 0x00000002,       /* 0 = half rate, 1 = full rate */
    DS_EVRC_PACKET_FORMAT   = 0x0000000C,       /* 0 = Interleave/bundled, 1 = header free, 2 = compact bundled */
    DS_EVRC_BITRATE         = 0x00000070,       /* values from 0 - 7 */
    DS_EVRC_MODE            = 0x00000700,       /* values from 0 - 7 */
    DS_EVRC_MAX_INTERLEAVE  = 0x00007000,       /* values from 0 - 7 */
    DS_EVRC_DTMF            = 0x00010000,       /* values from 0 - 1 */
    DS_EVRC_TTY_MODE        = 0x00060000,       /* values from 0 - 3 or 0 - 1 */
    DS_EVRC_NOISE_SUPP      = 0x00080000,       /* values from 0 - 1 */
    DS_EVRC_POST_FILTER     = 0x00100000        /* values from 0 - 1 */
};

#define DS_EVRC_PACKET_FORMAT_SHIFT     2
#define DS_EVRC_BITRATE_SHIFT           4
#define DS_EVRC_MODE_SHIFT              8
#define DS_EVRC_MAX_INTERLEAVE_SHIFT    12
#define DS_EVRC_TTY_MODE_SHIFT          16

enum evrc_packet_format
{
    DS_EVRC_INTERLEAVE_BUNDLED,
    DS_EVRC_HEADER_FREE,
    DS_EVRC_COMPACT_BUNDLED
};

/*
  OPUS codec flags
  DTX is set using VOICE_ATTR_FLAG_VAD
  if maxaveragebitrate = 0, this value is not specified
*/
enum opus_codec_flags
{
    DS_OPUS_MAX_AVG_BITRATE = 0x00FFFFFF,       /* 6000 - 510000 */
    DS_OPUS_STEREO          = 0x01000000,       /* 0 - mono, 1 stereo */
    DS_OPUS_SPROP_STEREO    = 0x02000000,       /* 0 - mono, 1 stereo */
    DS_OPUS_CBR             = 0x04000000,       /* 0 = variable, 1 = constant bitrate */
    DS_OPUS_FEC             = 0x08000000        /* 0 = FEC disabled, 1 = FEC enabled */
};

#if defined(USE_ATCA_EVS_MODS) || defined(_X86) || defined(_ARM)

#define DS_EVS_BITRATE_SHIFT             2
#define DS_EVS_PACKET_FORMAT_SHIFT       6
#define DS_EVS_RTCP_APP_ENABLE_SHIFT     7
#define DS_EVS_MAX_REDUNDANCY_SHIFT      8
#define DS_EVS_CMR_SHIFT                 13
#define DS_EVS_CH_SEND_SHIFT             15
#define DS_EVS_CH_RECV_SHIFT             17
#define DS_EVS_CH_AW_RECV_SHIFT          19

enum evs_packet_format {

  DS_EVS_COMPACT,
  DS_EVS_HEADER_FULL
};

enum evs_rtcp_app_enable {

  DS_EVS_RTCP_APP_DISABLE,
  DS_EVS_RTCP_APP_DSP,
  DS_EVS_RTCP_APP_HOST
};

enum evs_cmr {

  DS_EVS_CMR_ZERO,                              /* 0 = "0"  all CMR values enabled */
  DS_EVS_CMR_ONE,                               /* 1 = "1"  CMR must be present in each packet */
  DS_EVS_CMR_MINUS_ONE                          /* 2 = "-1" EVS Primary Mode, CMR byte in RTP header is disabled */
};

enum evs_ch_aw_recv {

  DS_EVS_CH_AW_RECV_MINUS_ONE = 8,              /* 8 = "-1" partial redundancy disabled in receive direction */
  DS_EVS_CH_AW_RECV_ZERO      = 0,              /* 0 = "0"  partial redundancy not used at session startup */
  DS_EVS_CH_AW_RECV_TWO       = 2,              /* 2,3,5,7  partial redundancy used at session startup with the value provided as the offset */
  DS_EVS_CH_AW_RECV_THREE     = 3,
  DS_EVS_CH_AW_RECV_FIVE      = 5,
  DS_EVS_CH_AW_RECV_SEVEN     = 7
};

enum evs_bit_rate {

  DS_EVS_BITRATE_5_9_SC_VBR,                    /* 0 */
  DS_EVS_BITRATE_7_2,                           /* 1 */
  DS_EVS_BITRATE_8_0,                           /* 2 */
  DS_EVS_BITRATE_9_6,                           /* 3 */
  DS_EVS_BITRATE_13_2,                          /* 4 */
  DS_EVS_BITRATE_13_2_CA,                       /* 5 */
  DS_EVS_BITRATE_16_4,                          /* 6 */
  DS_EVS_BITRATE_24_4,                          /* 7 */
  DS_EVS_BITRATE_32,                            /* 8 */
  DS_EVS_BITRATE_48,                            /* 9 */
  DS_EVS_BITRATE_64,                            /* 10 */
  DS_EVS_BITRATE_96,                            /* 11 */
  DS_EVS_BITRATE_128                            /* 12 */
};

enum evs_sample_rate {  /* sampling rate enums match EVS lib constants, please don't change */

  DS_EVS_FS_8KHZ,                               /* 0 */
  DS_EVS_FS_16KHZ,                              /* 1 */
  DS_EVS_FS_32KHZ,                              /* 2 */
  DS_EVS_FS_48KHZ                               /* 3 */
};

enum evs_bandwidth_limit {    /* bandwidth_limit enums match EVS lib constants, please don't change */

  DS_EVS_BWL_NB,                                /* 4 kHz max bandwidth */
  DS_EVS_BWL_WB,                                /* 8 kHz max bandwidth */
  DS_EVS_BWL_SWB,                               /* 14 kHz max bandwidth */
  DS_EVS_BWL_FB                                 /* 20 kHz max bandwidth */
};

/*
  -EVS codec flags used with evs.codec_flags element of voice_attributes struct, which is inside TERMINATION_INFO struct (session.h), which is used by pktlib. These are not used when apps interact directly with voplib and CODEC_ENC_PARAMS and CODEC_DEC_PARAMS structs
  -silencesupp is set using VOICE_ATTR_FLAG_VAD
*/
#define DS_RF_FEC_OFFSET_SHIFT 13
#define DS_DTX_VALUE_SHIFT 18
enum evs_codec_flags
{
    DS_EVS_SAMPLE_RATE     = 0x00000003,        /* 0 = 8 kHz (NB), 1 = 16 kHz (WB), 2 = 32 kHz (SWB), 3= 48 kHz (FB) */
    DS_EVS_BITRATE         = 0x0000003C,        /* values from 0 - 12, see evs_bit_rate enum */
    DS_EVS_PACKET_FORMAT   = 0x00000040,        /* 0 = Compact, 1 = Header-Full, see evs_packet_format enum */
    DS_EVS_RTCP_APP_ENABLE = 0x00000180,        /* 0 = Disable, 1 = DSP, 2 = HOST, see evs_rtcp_app_enable enum */
    DS_EVS_MAX_REDUNDANCY  = 0x00001E00,        /* values from 0 - 11 (Max # Redundant Frames per RTP packet) */
    DS_EVS_CMR             = 0x00006000,        /* 0 = "0"  all CMR values enabled */
                                                /* 1 = "1"  CMR must be present in each packet */
                                                /* 2 = "-1" EVS Primary Mode, CMR byte in RTP header is disabled */
                                                /* see evs_cmr enum */
    DS_EVS_CH_SEND         = 0x00018000,        /* Number of audio channels supported in send direction {1..2}, not present = 1, default = 1 (mono) */
    DS_EVS_CH_RECV         = 0x00060000,        /* Number of audio channels supported in recv direction {1..2}, not present = 1, default = 1 (mono) */
    DS_EVS_CH_AW_RECV      = 0x00780000,        /* Specifies how channel aware mode is configured or used in the receive direction {-1, 0, 2, 3, 5, 7} */
                                                /* 8 = "-1" partial redundancy disabled in receive direction */
                                                /* 0        partial redundancy not used at session startup */
                                                /* 2,3,5,7  partial redundancy used at session startup with the value provided as the offset */
                                                /* see evs_ch_aw_recv enum */

    DS_RF_FEC_INDICATOR    = 0x00001000,        /* RF FEC Indicator {0, 1} */
    DS_RF_FEC_OFFSET       = 0x0001E000,        /* RF FEC Offset {-1, 0, 2, 3, 5, 7} */
    DS_RF_FEC_OFFSET_SIGN  = 0x00010000,        /* RF FEC Offset sign bit */
    DS_DTX_ENABLE          = 0x00020000,        /* DTX Enable {0, 1} */
    DS_DTX_VALUE           = 0x01FC0000         /* DTX value {0 = adaptive, 3...100}*/
};

#else

/*
  EVS codec flags
  silencesupp is set using VOICE_ATTR_FLAG_VAD
*/
#define DS_RF_FEC_OFFSET_SHIFT 13
#define DS_DTX_VALUE_SHIFT 18
enum evs_codec_flags
{
    DS_EVS_SAMPLE_RATE     = 0x00000003,        /* 0 = 8 kHz (NB), 1 = 16 kHz (WB), 2 = 32 kHz (SWB), 3= 48 kHz (FB) */
    DS_EVS_BITRATE         = 0x0000003C,        /* values from 0 - 12 */
    DS_EVS_PACKET_FORMAT   = 0x00000040,        /* 0 = Compact, 1 = Header-Full */
    DS_EVS_RTCP_APP_ENABLE = 0x00000080,        /* 0 = Disable, 1 = Enable */
    DS_EVS_MAX_REDUNDANCY  = 0x00000F00,        /* values from 0 - 11 (Max # Redundant Frames per RTP packet) */
    DS_RF_FEC_INDICATOR    = 0x00001000,        /* RF FEC Indicator {0, 1} */
    DS_RF_FEC_OFFSET       = 0x0001E000,        /* RF FEC Offset {-1, 0, 2, 3, 5, 7} */
    DS_RF_FEC_OFFSET_SIGN  = 0x00010000,        /* RF FEC Offset sign bit */
    DS_DTX_ENABLE          = 0x00020000,        /* DTX Enable {0, 1} */
    DS_DTX_VALUE           = 0x01FC0000         /* DTX value {0 = adaptive, 3...100}*/
};

#endif  /* ATCA blade system EVS mods */

enum melpe_codec_flags
{
   DS_MELPE_BITDENSITY  =  0x0000007F,
   DS_MELPE_NPP         =  0x00000080,
   DS_MELPE_POST        =  0x00000100
};

/* payload shift filter flags, JHB Sep 2022 */

#define CODEC_PAYLOAD_SHIFT_AMRWBIOMODE    0x100
#define CODEC_PAYLOAD_SHIFT_COMPACTHEADER  0x200
#define CODEC_PAYLOAD_SHIFT_FULLHEADER     0x400
#define CODEC_PAYLOAD_SHIFT_FILTERMASK     0xff00

/* support legacy names, Oct 2023 */

#define TERM_PAYLOAD_SHIFT_AMRWBIOMODE     CODEC_PAYLOAD_SHIFT_AMRWBIOMODE
#define TERM_PAYLOAD_SHIFT_COMPACTHEADER   CODEC_PAYLOAD_SHIFT_COMPACTHEADER
#define TERM_PAYLOAD_SHIFT_FULLHEADER      CODEC_PAYLOAD_SHIFT_FULLHEADER
#define TERM_PAYLOAD_SHIFT_FILTERMASK      CODEC_PAYLOAD_SHIFT_FILTERMASK

#endif  /* _CODEC_H_ */
