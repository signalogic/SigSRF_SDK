/*
  codec.h

  Voice / video codec support for c66x, x86, or combined coCPU platforms

  Copyright (C) Signalogic, Inc, 2008-2012

    Created for c64x voice platforms

  Copyright (C) Mavenir Systems and Signalogic, Inc, 2013-2015
  
    Support for c66x coCPU card PCIe and ATCA blade SRIO interfaces

  Copyright (C) Signalogic, Inc, 2016-2024

    Add APIs to (i) encapsulate c66x PCIe and SRIO interfaces, and (ii) support x86-only or combined x86 and c66x server usage.  APIs are consistent between all use cases

  License
  
    Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

  Revision History

    Created Oct 2023 JHB, from session.h, codec related items separated to more cleanly support codec-only applications
    Modified Dec 2023 JHB, comments only
    Modified Feb 2024 JHB, add DS_VOICE_CODEC_TYPE_L16 definition (linear PCM 16-bit)
    Modified May 2024 JHB, change #ifdef _X86 to #if defined(_X86) || defined(_ARM)
*/

#ifndef _CODEC_H_
#define _CODEC_H_

enum media_type {

  DS_MEDIA_TYPE_VOICE,
  DS_MEDIA_TYPE_VIDEO
};

enum voice_codec_type {

  DS_VOICE_CODEC_TYPE_NONE,          /* pass-thru */
  DS_VOICE_CODEC_TYPE_G711_ULAW,     /* 1 */
  DS_VOICE_CODEC_TYPE_G711_ALAW,     /* 2 */
  DS_VOICE_CODEC_TYPE_G711_WB_ULAW,  /* 3, G711.1 */ 
  DS_VOICE_CODEC_TYPE_G711_WB_ALAW,  /* 4, G711.1 */
  DS_VOICE_CODEC_TYPE_G726,          /* 5 */
  DS_VOICE_CODEC_TYPE_G729AB,        /* 6 */
  DS_VOICE_CODEC_TYPE_G723,          /* 7 */
  DS_VOICE_CODEC_TYPE_AMR_NB,        /* 8 */
  DS_VOICE_CODEC_TYPE_AMR_WB,        /* 9 */
  DS_VOICE_CODEC_TYPE_EVRC,          /* 10 */
  DS_VOICE_CODEC_TYPE_ILBC,          /* 11 */
  DS_VOICE_CODEC_TYPE_ISAC,          /* 12 */
  DS_VOICE_CODEC_TYPE_OPUS,          /* 13 */
  DS_VOICE_CODEC_TYPE_EVRCB,         /* 14 */
  DS_VOICE_CODEC_TYPE_GSMFR,         /* 15 */
  DS_VOICE_CODEC_TYPE_GSMHR,         /* 16 */
  DS_VOICE_CODEC_TYPE_GSMEFR,        /* 17 */
  DS_VOICE_CODEC_TYPE_G722,          /* 18 */
  DS_VOICE_CODEC_TYPE_EVRC_NW,       /* 19 */
  DS_VOICE_CODEC_TYPE_CLEARMODE,     /* 20 */
  DS_VOICE_CODEC_TYPE_EVS,           /* 21 */
  DS_VOICE_CODEC_TYPE_MELPE,         /* 22 */
  DS_VOICE_CODEC_TYPE_AMR_WB_PLUS,   /* 23 */
  DS_VOICE_CODEC_TYPE_L16,           /* 24 */
  DS_VOICE_CODEC_TYPE_INVALID,       /* 25 */
  DS_VOICE_NCODECS = DS_VOICE_CODEC_TYPE_INVALID
};

enum video_codec_type {

  DS_VIDEO_CODEC_TYPE_MPEG2,
  DS_VIDEO_CODEC_TYPE_H264,
  DS_VIDEO_CODEC_TYPE_VP8,
  DS_VIDEO_CODEC_TYPE_INVALID,
  DS_VIDEO_NCODECS = DS_VIDEO_CODEC_TYPE_INVALID
};

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

  DS_EC_NONE,        /* NONE */
  DS_EC_TI_LEC,         /* Telogy line ec */
  DS_EC_TI_LEC_ACOUSTIC /* Telinnovations line/acoustic ec */
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
    DS_EVRC_FRAME_SIZE      = 0x00000001,   /* 0 = 8 khz, 1 = 16 kHz */
    DS_EVRC_FIXED_RATE      = 0x00000002,   /* 0 = half rate, 1 = full rate */
    DS_EVRC_PACKET_FORMAT   = 0x0000000C,   /* 0 = Interleave/bundled, 1 = header free, 2 = compact bundled */
    DS_EVRC_BITRATE         = 0x00000070,   /* values from 0 - 7 */
    DS_EVRC_MODE            = 0x00000700,   /* values from 0 - 7 */
    DS_EVRC_MAX_INTERLEAVE  = 0x00007000,   /* values from 0 - 7 */
    DS_EVRC_DTMF            = 0x00010000,   /* values from 0 - 1 */
    DS_EVRC_TTY_MODE        = 0x00060000,   /* values from 0 - 3 or 0 - 1 */
    DS_EVRC_NOISE_SUPP      = 0x00080000,   /* values from 0 - 1 */
    DS_EVRC_POST_FILTER     = 0x00100000    /* values from 0 - 1 */
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
    DS_OPUS_MAX_AVG_BITRATE = 0x00FFFFFF,   /* 6000 - 510000 */
    DS_OPUS_STEREO          = 0x01000000,   /* 0 - mono, 1 stereo */
    DS_OPUS_SPROP_STEREO    = 0x02000000,   /* 0 - mono, 1 stereo */
    DS_OPUS_CBR             = 0x04000000,   /* 0 = variable, 1 = constant bitrate */
    DS_OPUS_FEC             = 0x08000000    /* 0 = FEC disabled, 1 = FEC enabled */
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

  DS_EVS_CMR_ZERO,            /* 0 = "0"  all CMR values enabled */
  DS_EVS_CMR_ONE,             /* 1 = "1"  CMR must be present in each packet */
  DS_EVS_CMR_MINUS_ONE        /* 2 = "-1" EVS Primary Mode, CMR byte in RTP header is disabled */
};

enum evs_ch_aw_recv {

  DS_EVS_CH_AW_RECV_MINUS_ONE = 8, /* 8 = "-1" partial redundancy disabled in receive direction */
  DS_EVS_CH_AW_RECV_ZERO      = 0, /* 0 = "0"  partial redundancy not used at session startup */
  DS_EVS_CH_AW_RECV_TWO       = 2, /* 2,3,5,7  partial redundancy used at session startup with the value provided as the offset */
  DS_EVS_CH_AW_RECV_THREE     = 3,
  DS_EVS_CH_AW_RECV_FIVE      = 5,
  DS_EVS_CH_AW_RECV_SEVEN     = 7
};

enum evs_bit_rate {

  DS_EVS_BITRATE_5_9_SC_VBR,  /* 0 */
  DS_EVS_BITRATE_7_2,         /* 1 */
  DS_EVS_BITRATE_8_0,         /* 2 */
  DS_EVS_BITRATE_9_6,         /* 3 */
  DS_EVS_BITRATE_13_2,        /* 4 */
  DS_EVS_BITRATE_13_2_CA,     /* 5 */
  DS_EVS_BITRATE_16_4,        /* 6 */
  DS_EVS_BITRATE_24_4,        /* 7 */
  DS_EVS_BITRATE_32,          /* 8 */
  DS_EVS_BITRATE_48,          /* 9 */
  DS_EVS_BITRATE_64,          /* 10 */
  DS_EVS_BITRATE_96,          /* 11 */
  DS_EVS_BITRATE_128          /* 12 */
};

enum evs_sample_rate {  /* sampling rate enums match EVS lib constants, don't change */

  DS_EVS_FS_8KHZ,             /* 0 */
  DS_EVS_FS_16KHZ,            /* 1 */
  DS_EVS_FS_32KHZ,            /* 2 */
  DS_EVS_FS_48KHZ             /* 3 */
};

enum evs_bandwidth_limit {    /* bandwidth_limit enums match EVS lib constants, don't change */

  DS_EVS_BWL_NB,              /* 4 kHz max bandwidth */
  DS_EVS_BWL_WB,              /* 8 kHz max bandwidth */
  DS_EVS_BWL_SWB,             /* 14 kHz max bandwidth */
  DS_EVS_BWL_FB               /* 20 kHz max bandwidth */
};

/*
  -EVS codec flags used with evs.codec_flags element of voice_attributes struct, which is inside TERMINATION_INFO struct (session.h), which is used by pktlib. These are not used when apps interact directly with voplib and CODEC_ENC_PARAMS and CODEC_DEC_PARAMS structs
  -silencesupp is set using VOICE_ATTR_FLAG_VAD
*/
#define DS_RF_FEC_OFFSET_SHIFT 13
#define DS_DTX_VALUE_SHIFT 18
enum evs_codec_flags
{
    DS_EVS_SAMPLE_RATE     = 0x00000003,  /* 0 = 8 kHz (NB), 1 = 16 kHz (WB), 2 = 32 kHz (SWB), 3= 48 kHz (FB) */
    DS_EVS_BITRATE         = 0x0000003C,  /* values from 0 - 12, see evs_bit_rate enum */
    DS_EVS_PACKET_FORMAT   = 0x00000040,  /* 0 = Compact, 1 = Header-Full, see evs_packet_format enum */
    DS_EVS_RTCP_APP_ENABLE = 0x00000180,  /* 0 = Disable, 1 = DSP, 2 = HOST, see evs_rtcp_app_enable enum */
    DS_EVS_MAX_REDUNDANCY  = 0x00001E00,  /* values from 0 - 11 (Max # Redundant Frames per RTP packet) */
    DS_EVS_CMR             = 0x00006000,  /* 0 = "0"  all CMR values enabled */
                                          /* 1 = "1"  CMR must be present in each packet */
                                          /* 2 = "-1" EVS Primary Mode, CMR byte in RTP header is disabled */
                                          /* see evs_cmr enum */
    DS_EVS_CH_SEND         = 0x00018000,  /* Number of audio channels supported in send direction {1..2}, not present = 1, default = 1 (mono) */
    DS_EVS_CH_RECV         = 0x00060000,  /* Number of audio channels supported in recv direction {1..2}, not present = 1, default = 1 (mono) */
    DS_EVS_CH_AW_RECV      = 0x00780000,  /* Specifies how channel aware mode is configured or used in the receive direction {-1, 0, 2, 3, 5, 7} */
                                          /* 8 = "-1" partial redundancy disabled in receive direction */
                                          /* 0        partial redundancy not used at session startup */
                                          /* 2,3,5,7  partial redundancy used at session startup with the value provided as the offset */
                                          /* see evs_ch_aw_recv enum */

    DS_RF_FEC_INDICATOR    = 0x00001000,  /* RF FEC Indicator {0, 1} */
    DS_RF_FEC_OFFSET       = 0x0001E000,  /* RF FEC Offset {-1, 0, 2, 3, 5, 7} */
    DS_RF_FEC_OFFSET_SIGN  = 0x00010000,  /* RF FEC Offset sign bit */
    DS_DTX_ENABLE          = 0x00020000,  /* DTX Enable {0, 1} */
    DS_DTX_VALUE           = 0x01FC0000   /* DTX value {0 = adaptive, 3...100}*/
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
    DS_EVS_SAMPLE_RATE     = 0x00000003,   /* 0 = 8 kHz (NB), 1 = 16 kHz (WB), 2 = 32 kHz (SWB), 3= 48 kHz (FB) */
    DS_EVS_BITRATE         = 0x0000003C,   /* values from 0 - 12 */
    DS_EVS_PACKET_FORMAT   = 0x00000040,   /* 0 = Compact, 1 = Header-Full */
    DS_EVS_RTCP_APP_ENABLE = 0x00000080,   /* 0 = Disable, 1 = Enable */
    DS_EVS_MAX_REDUNDANCY  = 0x00000F00,   /* values from 0 - 11 (Max # Redundant Frames per RTP packet) */
    DS_RF_FEC_INDICATOR    = 0x00001000,   /* RF FEC Indicator {0, 1} */
    DS_RF_FEC_OFFSET       = 0x0001E000,   /* RF FEC Offset {-1, 0, 2, 3, 5, 7} */
    DS_RF_FEC_OFFSET_SIGN  = 0x00010000,   /* RF FEC Offset sign bit */
    DS_DTX_ENABLE          = 0x00020000,   /* DTX Enable {0, 1} */
    DS_DTX_VALUE           = 0x01FC0000    /* DTX value {0 = adaptive, 3...100}*/
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
