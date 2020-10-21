/*
  $Header: /root/Signalogic/DirectCore/include/voplib.h
 
  Description: voice and video over packet library -- APIs for creating and managing streaming and transcoding instances
 
  Projects: SigSRF, SigMRF, DirectCore
 
  Copyright (C) Signalogic Inc. 2010-2020

  Revision History:
  
   Created Mar 2017 Chris Johnson (some elements copied from legacy ds_vop.h)
   Modified May 2017 CJ, moved isEVSHeaderFullFormat from pktlib_priv.h
   Modified Aug 2017 CJ, moved EVS helper functions from mediaTest
   Modified Mar 2018 JHB, moved VOPLIB_VERSION global var inside extern "C" (https://stackoverflow.com/questions/38141390/extern-and-extern-c-for-variables)
   Modified Jun 2018 JHB, moved DSConvertFs to alglib
   Modified Oct 2018 JHB, removed #ifdef CODECXXX_INSTALLED references for codec header file includes.  The voplib Makefile no longer sets these, and the install script installs all codec header files (codec lib files are delivery dependent). stublib is used by mediaTest and mediaMin makefiles to link only the installed codecs
   Modified Jan 2019 JHB, remove uFlags param from DSCodecDelete() (it was never used, the codec handle links to internal info that contains flags if any)
   Modified Jul 2019 JHB, modify input struct ptr param and add flags for DSCodecCreate(), which is now used in both packet flow and codec test mode.  Flags include type of input struct data, and create encoder, decoder, or both
   Modified Feb 2020 JHB, add DS_GET_NUMFRAMES flag in DSCodecDecode()
   Modified Oct 2020 JHB, add DSCodecGetInfo() API to pull all available encoder/decoder info as needed. First input param is codec handle or codec_type, depending on uFlags. Added codec_name, raw_frame_size, and coded_frame_size elements to CODEC_PARAMS struct support DSCodecGetInfo()
*/
 
#ifndef _VOPLIB_H_
#define _VOPLIB_H_

#include "alias.h"
  
/* session struct and related definitions */
#include "session_cmd.h"

/* algorithm related definitions */

#include "alglib.h"

/* Sampling frequency and frame size max values, min ptime */
#if 1
  #define MAX_PTIME        20       /* 20 ms */
#else
  #define MAX_PTIME        60       /* in msec */
  #define MIN_PTIME        20       /* in msec */
#endif

#define MAX_FS             48       /* 48 kHz */
#define MAX_SAMPLES_FRAME  (MAX_FS*MAX_PTIME)
#define MAX_RAW_FRAME      (MAX_SAMPLES_FRAME*sizeof(short int))
#define MAX_CODED_FRAME    328      /* AMR-WB+: 80 byte max frame size + 2 byte header * 4 sub frames */

/* constants used for AMR and EVS codec formats */

#define HEADERCOMPACT  0  
#define HEADERFULL     1

/* typedefs for various voplib handles */

#if defined __MSVC__ 
  typedef void* HCODEC;  /* channel handle */
#elif defined _LINUX_
  /* done in alias.h */
#elif defined _WIN32_WINNT
  /* done in alias.h */
#else
  typedef HGLOBAL HCODEC;  /* channel handle */
#endif
  
/* Operation modes and corresponding APIs:
   - Packet-based transcoding
      - Use pktlib APIs where media type is set to speech in session data
   - Frame-based encoding/decoding/transcoding
      - DSCodecCreate() - setup an encoder and decoder for a codec with associated termination info 
      - DSCodecEncode()/DSCodecDecode() - encode/decode a frame with the given codec
      - DSCodecTranscode() - transcode between the two given codecs
      - DSCodecDelete() - teardown the encoder and decoder associated with the handle
*/
  
#ifdef __cplusplus
extern "C" {
#endif

/* voplib version string */
  extern char VOPLIB_VERSION[256];

/* Voplib config API.  pGlobalConfig and pDebugConfig point to GLOBAL_CONFIG and DEBUG_CONFIG structs defined in config.h.  See DS_CV_xx flags below */

  int DSConfigVoplib(GLOBAL_CONFIG* pGlobalConfig, DEBUG_CONFIG* pDebugConfig, unsigned int uFlags);  /* global config, debug config, or both can be configured depending on attributes specified in uFlags.  NULL should be given for either pointer not used */

/* codec instance definitions and APIs */

  typedef struct  {  /* CODEC_ENC_PARAMS */

/* generic items */

   int bitRate;
   int samplingRate;          /* most codecs are based on a fixed sampling rate so this is used only for advanced codecs such as EVS and Opus */
   float frameSize;           /* amount of data (in msec) processed by the codec per frame, for example 20 msec for AMR or EVS, 22.5 msec for MELPe, etc */

   union {
      int dtx_enable;
      int vad;                /* G729 terminology for DTX */
   } dtx;
   union {
      int header_format;      /* RTP payload header format ... e.g. for AMR, octet align vs. bandwidth efficient, for EVS compact vs. full header */
      int oct_align;          /* AMR terminology */
   } rtp_pyld_hdr_format;

/* G729, G726 items */

   int uncompress;

/* AMR-WB+ items */

   int mode;
   float isf;                 /* internal sampling frequency */
   int low_complexity;
   int nChannels;
   int mono;

/* EVS, Opus, other advanced codec items */

   int sid_update_interval;   /* interval between SID frames when DTX is enabled */
   int rf_enable;             /* channel-aware mode (for EVS only supported at 13.2 kbps) */
   int fec_indicator;         /* for EVS, LO = 0, HI = 1 */
   int fec_offset;            /* for EVS, 2, 3, 5, or 7 in number of frames */
   int bandwidth_limit;       /* for EVS, typically set to SWB or FB */

/* LBR codec items (e.g. MELPe) */

   int bitDensity;            /* channel bit density: 6, 54, 56 */
   int Npp;                   /* noise preprocessor control flag */

   int reserved[20];

  } CODEC_ENC_PARAMS;

  typedef struct {  /* CODEC_DEC_PARAMS */

/* generic items */

   int bitRate;               /* bitrate may not be used for codecs that can derive it from payload contents */
   int samplingRate;          /* not used for most codecs */
   float frameSize;           /* amount of data (in msec) processed by the codec per frame, for example 20 msec for AMR or EVS, 22.5 msec for MELPe, etc */

/* G729, G726 items */

   int uncompress;

/* AMR-WB+ items */

   int limiter;               /* avoids output clipping (recommended) */
   int mono;

/* LBR codec items (e.g. MELPe) */

   int bitDensity;            /* channel bit density: 6, 54, 56 */
   int post;                  /* post filter flag */
   int noReseed;              /* disable random number generator seeding (used for jitter) */

   int reserved[20];

  } CODEC_DEC_PARAMS;

  typedef struct {  /* additional output from DSCodecEncode() and DSCodecDecode() APIs */

   short int size;            /* generic size field, used differently by codecs */
   short int frameType;       /* encoder frame types */
   int extendedError;

  } CODEC_OUTARGS;

  typedef struct {

     int codec_type;
     char codec_name[50];
     uint16_t raw_frame_size;
     uint16_t coded_frame_size;
     CODEC_ENC_PARAMS enc_params;
     CODEC_DEC_PARAMS dec_params;

  } CODEC_PARAMS;


  HCODEC DSCodecCreate(void* pCodecInfo, unsigned int uFlags);  /* if DS_CC_USE_TERMINFO flag is given, pCodecInfo is interpreted as TERMINATION_INFO* (shared_include/session.h), otherwise as CODEC_PARAMS* (above) */

  void DSCodecDelete(HCODEC hCodec);

  int DSCodecEncode(HCODEC           hCodec,
                    unsigned int     uFlags, 
                    uint8_t*         inData, 
                    uint8_t*         outData,
                    uint32_t         in_frameSize,
                    CODEC_OUTARGS*   pOutArgs);

  int DSCodecDecode(HCODEC           hCodec,
                    unsigned int     uFlags, 
                    uint8_t*         inData,
                    uint8_t*         outData,
                    uint32_t         in_frameSize,
                    CODEC_OUTARGS*   pOutArgs);

  int DSCodecTranscode(HCODEC        hCodecSrc,
                       HCODEC        hCodecDst,
                       unsigned int  uFlags, 
                       uint8_t*      inData,
                       uint32_t      in_frameSize, 
                       uint8_t*      outData);

#define DS_GET_NUMFRAMES  0x100  /* if specified in uFlags, DSCodecDecode() returns the number of frames in the payload.  No decoding is performed */

/* APIs that need an hCodec (note -- DSCodecInfo() has a flag for an option here hCodec is interpreted as a codec type */
  
  int DSGetCodecSampleRate(HCODEC hCodec);
  int DSGetCodecBitRate(HCODEC hCodec);
  int DSGetCodecRawFrameSize(HCODEC hCodec);
  int DSGetCodecCodedFrameSize(HCODEC hCodec);
  int DSGetCodecType(HCODEC hCodec);
  int DSGetCodecInfo(HCODEC hCodec, unsigned int uFlags, void* pInfo);

/* Codec helper functions */

/* get sample rate code, given a codec type and actual sampling rate in Hz */

  char DSGetSampleRateValue(unsigned int codec_type, int sample_rate);


/* get payload size, given a codec type and bitrate code (note -- for EVS, the bitrate code is the lower 4 bits of the ToC byte */

  int DSGetPayloadSize(unsigned int codec_type, unsigned int bitrate_code);


/* get compressed data frame size given a codec type, bitrate, and header format */

  unsigned int DSGetCompressedFramesize(unsigned int codec_type, unsigned int bitRate, unsigned int headerFormat);


/* get header format and number of frames of an AMR or EVS payload.  Returns 0 for CH (compact header) format or 1 for FH (full header) format, number of ptimes found, and compact format frame size */

#if 0
  int8_t DSGetPayloadHeaderFormat(unsigned int codec_type, unsigned int payload_size, uint8_t* num_ptimes, uint32_t* compact_frame_size);
#else
  uint8_t DSGetPayloadHeaderFormat(unsigned int codec_type, unsigned int payload_size);
#endif

/* return an AMR or EVS payload header ToC based on payload size.  The ToC is normally a byte, but could be larger for future codecs.  See the AMR and EVS specs for bit fields defined in the ToC */

  int DSGetPayloadHeaderToC(unsigned int codec_type, unsigned int pyld_len);
 
#if 0
/* return sampling rate for given codec and flags */
  
  int DSGetCodecFs(unsigned int codec_type, unsigned int uFlags);
#endif

#ifdef __cplusplus
}
#endif

/* DSConfigVoplib() flags */

#define DS_CV_GLOBALCONFIG      0x01
#define DS_CV_DEBUGCONFIG       0x02
#define DS_CV_INIT              0x04

/* DSCodecCreate() flags */

#define DS_CC_CREATE_ENCODER    0x01
#define DS_CC_CREATE_DECODER    0x02
#define DS_CC_USE_TERMINFO      0x100

/* DSGetCodecInfo() flags */

#define DS_CODEC_INFO_HANDLE    0x100
#define DS_CODEC_INFO_TYPE      0x200

#endif   /* _VOPLIB_H_ */
