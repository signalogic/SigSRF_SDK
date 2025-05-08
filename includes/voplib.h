/*
 $Header: /root/Signalogic/DirectCore/include/voplib.h

 Copyright (C) Signalogic Inc. 2010-2025

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  Voice and video over packet library -- APIs for creating and managing streaming and transcoding instances

 Projects

  SigSRF, DirectCore
 
 Revision History

  Created Mar 2017 Chris Johnson (some elements copied from legacy ds_vop.h)
  Modified May 2017 CJ, moved isEVSHeaderFullFormat from pktlib_priv.h
  Modified Aug 2017 CJ, moved EVS helper functions from mediaTest
  Modified Mar 2018 JHB, moved VOPLIB_VERSION global var inside extern "C" (https://stackoverflow.com/questions/38141390/extern-and-extern-c-for-variables)
  Modified Jun 2018 JHB, moved DSConvertFs to alglib
  Modified Oct 2018 JHB, removed #ifdef CODECXXX_INSTALLED references for codec header file includes. The voplib Makefile no longer sets these, and the install script installs all codec header files (codec lib files are delivery dependent). stublib is used by mediaTest and mediaMin makefiles to link only the installed codecs
  Modified Jan 2019 JHB, remove uFlags param from DSCodecDelete() (it was never used, the codec handle links to internal info that contains flags if any)
  Modified Jul 2019 JHB, modify input struct ptr param and add flags for DSCodecCreate(), which is now used in both packet flow and codec test mode. Flags include type of input struct data, and create encoder, decoder, or both
  Modified Jul 2019 JHB, DSGetCodecFs() removed, use DSGetCodecSampleRate(hCodec) instead
  Modified Feb 2020 JHB, add DS_GET_NUMFRAMES flag in DSCodecDecode()
  Modified Oct 2020 JHB, add DSCodecGetInfo() API to pull all available encoder/decoder info as needed. First input param is codec handle or codec type, depending on uFlags. Added codec_name, raw_frame_size, and coded_frame_size elements to CODEC_PARAMS struct support DSCodecGetInfo()
  Modified Jan 2022 JHB, add DS_CC_TRACK_MEM_USAGE flag
  Modified Feb 2022 JHB, change DSGetCodecTypeStr() to DSGetCodecName() and add uFlags to allow codec param to be interpreted as either HCODEC or codec type (as with DSGetCodecInfo)
  Modified Feb 2022 JHB, modify DSCodecEncode() and DSCodecDecode() to accept pointer to one or more codec handles and a num channels param. This enables multichannel encoding/decoding (e.g. stereo audio files) and simplified concurrent codec instance test and measurement (e.g. 30+ codec instances within one thread or CPU core). Multichannel input and output data must be interleaved; see API comments and examples in mediaTest_proc.c
  Modified Sep 2022 JHB, add "payload_shift" to CODEC_DEC_PARAMS struct to allow RTP payload shift after encoding or before decoding for debug/test purposes. Shift conditions can be controlled by filter flags (in bits 15-8); shift amount ranges from -8 to +7 (in bits 7-0); see comments in TERMINATION_INFO struct in shared_include/session.h
  Modified Oct 2022 JHB, change DSGetPayloadHeaderFormat() to DSGetPayloadHeaderInfo() to reflect updates for additional params and info retrieval
  Modified Oct 2022 JHB, add pBitrateIndex param to DSGetCompressedFramesize()
  Modified Oct 2022 JHB, consolidate DSGetCodecName(), DSGetPayloadHeaderInfo(), and several others into DSGetCodecInfo() API, following pktlib model
  Modified Mar 2023 JHB, add pInArgs param to DSCodecEncode(), add CODEC_INARGS struct definition, add bitRate and CMR params to CODEC_OUTARGS struct. These were previously in license-only versions now they are public. See comments
  Modified Jul 2023 JHB, add CODEC_NAME_MAXLEN definition (used when manipulating codec name strings)
  Modified Oct 2023 JHB, implement CODECS_ONLY build option
  Modified Nov 2023 JHB, move definition of MAX_CODEC_INSTANCES here so it's correct in codecs-only app builds
  Modified Nov 2023 JHB, add uFlags param and return value to DSCodecDelete()
  Modified Nov 2023 JHB, add CODEC_DEC_PARAMS pointer to CODEC_INARGS struct, add CODEC_INARGS pointer param to DSCodecDecode()
  Modified Nov 2023 JHB, add uFlags to CODEC_ENC_PARAMS and CODEC_DEC_PARAMS structs; uFlags can accept RTP_FORMAT_xxx and DEBUG_OUTPUT_xxx flags
  Modified Nov 2023 JHB, add CMR param to DSGetPayloadInfo()
  Modified Dec 2023 JHB, add additional DEBUG_OUTPUT_xxx flags (DEBUG_OUTPUT_VOPLIB_SHOW_DECODER_INPUT_INFO)
  Modified Dec 2023 JHB, clarify instructions for minimal header and codecs-only application builds
  Modified Dec 2023 JHB, restructure include file logic to avoid alias.h when CODECS_ONLY is defined. Implement HIGHCAP definition to allow increases to MAX_CODEC_INSTANCES
  Modified Dec 2023 JHB, specifying DEBUG_OUTPUT_ENCODER_SHOW_INIT_PARAMS in CODEC_ENC_PARAMS uFlags will show encoder init params both on entry (after call to DSCodecCreate) and after all param validation and check
  Modified Feb 2024 JHB, updates to audio classification definitions and enums, no change in struct definitions. Fix #if defines for shared_include/codec.h and shared_include/config.h
  Modified Mar 2024 JHB, add DS_CODEC_USE_EVENT_LOG and DEBUG_TEST_ABORT_EXIT_INTERCEPTION flags, consolidate separate DEBUG_OUTPUT_xxx encoder and decoder flags, reassign value of DEBUG_OUTPUT_ADD_TO_EVENT_LOG flag, deprecate DS_CV_GLOBALCONFIG and DS_CV_DEBUGCONFIG flags
  Modified Apr 2024 JHB, clarify documentation for CMR handling in CODEC_OUTARGS and CODEC_INARGS structs
  Modified May 2024 JHB, change comments that reference x86_mediaTest.c to mediaTest_proc.c
  Modified Jun 2024 JHB, rename DS_CODEC_DECODE_GET_NUMFRAMES to DS_CODEC_GET_NUMFRAMES
  Modified Jul 2024 JHB, define PAYLOAD_INFO struct, DSGetPayloadInfo() now uses a PAYLOAD_INFO* param to return items extracted from payloads
  Modified Jul 2024 JHB, deprecate DSGetCodecType(); DSGetCodecInfo() now returns a codec type if both DS_CODEC_INFO_HANDLE or DS_CODEC_INFO_TYPE flags are given
  Modified Sep 2024 JHB, add DS_CODEC_INFO_TYPE_FROM_NAME flag, apps can combine this with pInfo codec name input and obtain a codec type. Standardized codec names used in all SigSRF libs and applications are in get_codec_type_from_name() in shared_include/codec.h
  Modified Sep 2024 JHB, support multiple frames per payload in DSGetPayloadInfo() if applicable to codec type. In PAYLOAD_INFO struct make ToC an array and add FrameSize array. Multiple frames are used for variable ptime, multiple independent mono channels, and stereo channels
  Modified Sep 2024 JHB, add hf-only support for EVS
  Modified Oct 2024 JHB, implement DS_PAYLOAD_INFO_SID_ONLY and DS_PAYLOAD_INFO_IGNORE_DTMF flags for DSGetPayloadInfo(). If the only info about a payload needed is whether it's a SID, using the SID_ONLY flag gives a faster result for EVS and other complex codecs by short-circuiting a full frame parse
  Modified Oct 2024 JHB, add DEBUG_OUTPUT_VOPLIB_CMR_INFO flag
  Modified Nov 2024 JHB, implement DS_PAYLOAD_INFO_GENERIC flag for DSGetPayloadInfo(). This allows generic subset payload analysis and information retrieval without a codec type
  Modified Dec 2024 JHB, implment DS_CODEC_INFO_NAME_VERBOSE flag
  Modified Jan 2025 JHB, provide alternate codec_types definition for codec lib builds (NO_VOPLIB_HEADERS defined) and stand-alone builds (CODECS_ONLY defined)
  Modified Feb 2025 JHB, add SDP_INFO struct definition for passing SDP fmtp fields into DSGetPayloadInfo(); to DSGetPayloadInfo() add sdp_info, nId, and fp_out params for codec bitstream extraction, otherwise set to NULL and/or zero (unused)
  Modified Feb 2025 JHB, add DS_PAYLOAD_INFO_DEBUG_OUTPUT and DS_PAYLOAD_INFO_RESET_ID flags
  Modified Mar 2025 JHB, define DS_VOPLIB_SUPPRESS_WARNING_ERROR_MSG and DS_VOPLIB_SUPPRESS_INFO_MSG flags with values equivalent to their counterparts in pktlib.h
  Modified Mar 2025 JHB, reorganize PAYLOAD_INFO struct into common items and substructs for voice, audio, and video. Add FU_Header item to video substruct
  Modified Mar 2025 JHB, add pInfo to DSGetPayloadInfo() to allow copying to mem buffer extracted bitstream data. fp_out and pInfo can both be supplied at the same time
  Modified Apr 2025 JHB, add H.264 support to DSGetPayloadInfo() and extract_rtp_video()
  Modified May 2025 JHB, add codec_type to PAYLOAD_INFO struct
*/
 
#ifndef _VOPLIB_H_
#define _VOPLIB_H_

/* include header files for codec types, debug configuration, session struct and other definitions:

  -define NO_VOPLIB_HEADERS for minimal header files in codec lib builds
  -define CODECS_ONLY for stand-alone codec usage. voplib configuration support (DSConfigVoplib() and shared_include/config.h) are required for concurrent / multithread functionality and event/error logging
*/

#if !defined(NO_VOPLIB_HEADERS) && !defined(CODECS_ONLY)
  #include "alias.h"
  #include "shared_include/session_cmd.h"  /* bring in session.h, codec.h, and config.h from shared_include subfolder */
  #include "alglib.h"                      /* sampling rate conversion */
#else                                      /* if needed define a few items normally handled in alias.h */
  #include <stdint.h>
  #include <stdio.h>
  #include <stdbool.h>
  #define HCODEC int32_t
  #define codec_types int                  /* codec_types typedef normally defined in shared_include/codec.h, JHB Jan 2025 */
#endif

#if defined(CODECS_ONLY) && !defined(NO_VOPLIB_HEADERS)
  #include "shared_include/codec.h"        /* bring in codec_type enums from shared_include subfolder */
  #include "shared_include/config.h"       /* bring in config.h (DEBUG_CONFIG struct definition and enums) from shared_include subfolder */
#endif

#ifndef NCORECHAN
  #if defined(CODECS_ONLY) || defined(NO_VOPLIB_HEADERS)
    #ifdef HIGHCAP
      #define NCORECHAN 8192
    #else
      #define NCORECHAN 2048
    #endif
  #else
    #include "shared_include/transcoding.h"  /* NCORECHAN defined in transcoding.h, demo builds have a small number, JHB Aug 2019 */
  #endif
#endif
#define MAX_CODEC_INSTANCES (2*NCORECHAN)  /* maximum allowable codec instances */

/* algorithm related definitions */

/* Sampling frequency and frame size max values, max, min, and nominal ptimes */
#if 0
  #define MAX_PTIME        20       /* 20 ms */
#else
  #define MAX_PTIME        60       /* in msec */
  #define MIN_PTIME        20
  #define NOM_PTIME        20
#endif

#define NB_CODEC_FS        8000     /* narrow band audio Fs 8 kHz */
#define WB_CODEC_FS        16000    /* wideband audio Fs 16 kHz */
#define SWB_CODEC_FS       32000    /* super wideband audio Fs 32 kHz */
#define FB_CODEC_FS        48000    /* fullband audio Fs 48 kHz */

#define MAX_SAMPLES_FRAME  (FB_CODEC_FS/1000*NOM_PTIME)  /* max audio domain frame size 960 samples */
#define MAX_RAW_FRAME      (MAX_SAMPLES_FRAME*sizeof(int16_t))  /* maximum audio domain frame size, in bytes, defined here as 1920 bytes*/
#define MAX_CODED_FRAME    328      /* AMR-WB+: 80 byte max frame size + 2 byte header * 4 sub frames */
#define MAX_AUDIO_CHAN     100      /* max audio channels supported in the mediaTest refererence application. Note this channel count is completely separate from max channels in pktlib and the mediaMin reference app */
#define MAX_FSCONV_UP_DOWN_FACTOR  160  /* current maximum Fs conversion up/down factor allowed in mediaTest and mediaMin reference apps. 160 allows sampling rate conversion worst case of 44100 to/from 48000 kHz and also handles other extended ranges such as converting 8 to/from 192 kHz. DSConvertFs() in alglib also reference this definition */

/* Payload format definitions for EVS, AMR, and H.26x codec formats */

#define DS_PYLD_FMT_COMPACT             0
#define DS_PYLD_FMT_FULL                1
#define DS_PYLD_FMT_BANDWIDTHEFFICIENT  DS_PYLD_FMT_COMPACT
#define DS_PYLD_FMT_OCTETALIGN          DS_PYLD_FMT_FULL
#define DS_PYLD_FMT_HF_ONLY             2   /* EVS hf-only format formally supported and tested, JHB Sep 2024 */
#define DS_PYLD_FMT_H264                0x10
#define DS_PYLD_FMT_H265                0x11

/* typedefs for voplib handles */

#if defined __MSVC__ 
  typedef void* HCODEC;  /* channel handle */
#elif defined _LINUX_
  /* done in alias.h */
#elif defined _WIN32_WINNT
  /* done in alias.h */
#else
  typedef HGLOBAL HCODEC;  /* channel handle */
#endif
  
/* Operational modes and APIs

   - Packet-based transcoding
      - used by pktlib to create encoder and decoder instances based on session and termination info

   - Frame-based encoding/decoding
      - DSCodecCreate() - create an encoder or decoder instance with specified parameters, receiving a handle to the instance 
      - DSCodecEncode() - encode one or more frames using one or more encoder handles
      - DSCodecDecode() - decode one or more frames using one or more decoder handles
      - DSCodecDelete() - delete an encoder or decoder instance

      - DSCodecTranscode() - transcode between two codecs [CURRENTLY DEPRECATED; use mediaTest or mediaMin apps instead]
*/
  
#ifdef __cplusplus
extern "C" {
#endif

  #ifndef NO_VOPLIB_HEADERS

  extern char VOPLIB_VERSION[256];  /* voplib version string */

/* voplib config API. pGlobalConfig and pDebugConfig point to GLOBAL_CONFIG and DEBUG_CONFIG structs defined in config.h. See DS_CV_xx flags below */

  int DSConfigVoplib(GLOBAL_CONFIG* pGlobalConfig, DEBUG_CONFIG* pDebugConfig, unsigned int uFlags);  /* global config, debug config, or both can be configured depending on attributes specified in uFlags. NULL should be given for either pointer not used */
  #endif

/* codec instance definitions and APIs */

  typedef struct  {  /* CODEC_ENC_PARAMS */

/* generic items */

   int bitRate;               /* bitrate in bps */
   int samplingRate;          /* most codecs are based on a fixed sampling rate so this is used only for advanced codecs such as EVS and Opus */
   float frameSize;           /* amount of data (in msec) processed by the codec per frame, for example 20 msec for AMR or EVS, 22.5 msec for MELPe, etc */

   union {
      int dtx_enable;
      int vad;                /* G729 terminology for DTX */
   } dtx;
   union {
      int payload_format;     /* RTP payload format ... e.g. for AMR, octet align vs. bandwidth efficient, for EVS compact vs. full header */
      int oct_align;          /* same field, applicable to AMR codecs */
   } rtp_pyld_format;

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

   unsigned int uFlags;       /* see RTP_FORMAT_xxx and DEBUG_OUTPUT_xxx flag definitions */

   int reserved[19];

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

   unsigned int uFlags;       /* see RTP_FORMAT_xxx and DEBUG_OUTPUT_xxx flag definitions below */

   int reserved[19];

  } CODEC_DEC_PARAMS;

/* audio classification frame types returned in CODEC_OUTARGS frameType */

  enum audio_classification_frametype {
 
    FRAMETYPE_NONE = 0,
    FRAMETYPE_VOICED = 1,           /* speech, voiced */
    FRAMETYPE_UNVOICED = 2,         /* speech, unvoiced */
    FRAMETYPE_SID = 4,              /* SID (silence / comfort noise) frames for codecs that support DTX */
    FRAMETYPE_NODATA = 8,           /* untransmitted frame for codecs that support DTX */
    FRAMETYPE_NOISE = 0x10,         /* background noise for codecs that support audio classification */
    FRAMETYPE_AUDIO = 0x20,         /* sounds and other audio for codecs that support audio classification */
    FRAMETYPE_MUSIC = 0x40,         /* music for codecs that support audio classification */

/* flags that may be combined with above types */

    FRAMETYPE_TRANSITION = 0x1000,  /* transition between types */
    FRAMETYPE_MELP = 0x2000,        /* low bitrate voiced (mixed excited linear prediction) */
    FRAMETYPE_NELP = 0x4000,        /* low bitrate voiced (noise excited linear prediction) */

    FRAMETYPE_ITEM_MASK = 0xfff     /* mask to separate items from flags */
  };
 
  typedef struct {  /* optional output from DSCodecEncode() and DSCodecDecode() APIs, if pOutArgs is non-NULL */

   short int size;       /* generic size field, used differently by codecs */
   short int frameType;  /* audio content frame type classified by the encoder, if supported by the codec type. Possible types are enumerated in audio_classification_frametype */
   int extendedError;

   uint8_t CMR;          /* for DSCodecDecode(), CMR (Codec Mode Request) will reflect the value in the input bitstream, if supported by the codec type. Notes:

                            -for AMR codecs CMR examples include 0xf0 (no mode request), 0x20 (AMR-WB 12.65 bps), 0x70 (AMR-NB 1.20 kbps), etc. If the bitstream CMR is "no mode request" (the default value), CMR will be 0xf0
                            -for EVS codecs CMR will be non-zero only if present in the input bitstream. Examples include 0x80 (CMR = 0), 0xa4 (CMR = 0x24), 0x92 (CMR = 0x12), etc. CMR will be non-zero if the input bitstream is (a) in headerfull format and includes a CMR byte or (b) in AMR-WB IO mode compact format
                            -received CMR values are not shifted in any way. For octet align and headerfull formats, CMR contains the whole byte as received (including H bit or R bits as applicable). For bandwidth efficent and compact formats, CMR contains the partial 4 or 3 bits, in the exact position they were received, with other CMR bits zero
                         */

   int bitRate;          /* for DSCodecDecode(), bitrate detected by the decoder (in bps) from the input bitstream, if supported by the codec type */

  } CODEC_OUTARGS;

  typedef struct {  /* optional input to DSCodecEncode() and DSCodecDecode() APIs, if pInArgs is non-NULL */

   uint8_t CMR;          /* for DSCodecEncode(), this is the CMR (Codec Mode Request) in the encoder output bitstream frame. Notes:

                            -SigSRF encoders generate a CMR if mandated by the spec. For example, encoders generate a CMR for all AMR frames and for EVS AMR-WB IO mode SID frames (as required by spec section A.2.2.1.1). In these cases a CMR specified here will override the one generated
                            -for AMR codecs if "no mode request" should be inserted, then specify 0xf0. When pInArgs is NULL, the default CMR value is 0xf0
                            -for EVS codecs using headerfull format, if pInArgs is non-NULL then zero CMR values are ignored. Examples of valid CMR values include 0x80 (CMR = 0), 0xa4 (CMR = 0x24), 0x92 (CMR = 0x12), etc. Note the CMR value msb should be set in order to comply with spec section A.2.2.1.1 (the "H" bit). When pInArgs is NULL, or when compact format is in use, CMR is ignored
                            -for EVS codecs using AMR-WB IO mode in compact format valid values of CMR include 0 (6.6 kbps), 0xc0 (23.85 kbps), 0xe0 (no mode request), etc.
                            -CMR should not be shifted in any way. For octet align and headerfull formats, CMR should give the whole byte to insert in the output frame (including H bit or R bits as applicable). For bandwidth efficent and compact formats, CMR should give the partial 4 or 3 bits, in the exact position within a payload byte as shown in the codec spec, with the rest of CMR zero'ed

                            for DSCodecDecode(), this is the CMR in the decoder input bitstream frame. Notes:

                            -zero values are ignored
                            -normally SigSRF decoders expect CMR in frame input. If one is specified here, then it's inserted at the start of the frame and processed without any additional assumptions. This implies that if the frame already has a CMR, the calling application should remove it
                         */

   CODEC_ENC_PARAMS* pCodecEncParams;  /* to change bitRate or codec-specific parameters within the duration of an encoder instance, specify a CODEC_ENC_PARAMS struct. Note this only applies to newer, advanced codecs such as EVS and Opus, JHB Mar 2023 */

   CODEC_DEC_PARAMS* pCodecDecParams;  /* to change output sampling rate or codec-specific parameters within the duration of a decoder instance, specify a CODEC_DEC_PARAMS struct. Note this only applies to newer, advanced codecs such as EVS and Opus */

  } CODEC_INARGS;


/* CODEC_PARAMS struct used in DSCodecCreate() and DSGetCodecInfo() */

  #define CODEC_NAME_MAXLEN  50

  typedef struct {

     codec_types codec_type;              /* should specify a codec_type enum -- see "DS_CODEC_XXX" enums defined in shared_include/codec.h */
     char codec_name[CODEC_NAME_MAXLEN];  /* pointer to codec name string that will be filled in. Note this is the same string as returned by DSGetCodecInfo() with DS_CODEC_INFO_NAME flag */
     uint16_t raw_frame_size;             /* filled in by DSCodecCreate() and DSGetCodecInfo() */
     uint16_t coded_frame_size;           /*   "    "    " */
     int payload_shift;                   /* special case item, when non-zero indicates shift payload after encoding or before decoding, depending on which codec and the case. Initially needed to "unshift" EVS AMR-WB IO mode bit-shifted packets observed in-the-wild. Note shift can be +/-, JHB Sep 2022 */

     CODEC_ENC_PARAMS enc_params;         /* if encoder instance is being created, this must point to desired encoder params. See examples in mediaTest_proc.c or hello_codec.c */
     CODEC_DEC_PARAMS dec_params;         /* if decoder instance is being created, this must point to desired decoder params. See examples in mediaTest_proc.c or hello_codec.c */

  } CODEC_PARAMS;


  HCODEC DSCodecCreate(void* pCodecInfo, unsigned int uFlags);  /* for direct or "codec only" usage, pCodecInfo should point to a CODEC_PARAMS struct; for example usage see mediaTest_proc.c or hello_codec.c. For packet based applications (indirect codec usage), if the DS_CODEC_CREATE_USE_TERMINFO flag is given in uFlags, then pCodecInfo should point to a TERMINATION_INFO struct (defined in shared_include/session.h); for example usage see packet_flow_media_proc.c (packet/media thread processing) */

  int DSCodecDelete(HCODEC hCodec, unsigned int uFlags);

  int DSCodecEncode(HCODEC*          hCodec,        /* pointer to one or more codec handles, as specified by numChan */
                    unsigned int     uFlags,        /* flags, see DS_CODEC_ENCODE_xxx flags below */
                    uint8_t*         inData,        /* pointer to input media data */
                    uint8_t*         outData,       /* pointer to output coded bitstream data */
                    uint32_t         in_frameSize,  /* size of input media data, in bytes */
                    int              numChan,       /* number of channels to be encoded. Multichannel data must be interleaved */
                    CODEC_INARGS*    pInArgs,       /* optional parameters for encoding media data; see CODEC_INARGS struct notes above. If not used this param should be NULL */
                    CODEC_OUTARGS*   pOutArgs);     /* optional encoder output info; see CODEC_OUTARGS struct notes above. If not used this param should be NULL */

  int DSCodecDecode(HCODEC*          hCodec,        /* pointer to one or more codec handles, as specified by numChan */
                    unsigned int     uFlags,        /* flags, see DS_CODEC_DECODE_xxx flags below */
                    uint8_t*         inData,        /* pointer to input coded bitstream data */
                    uint8_t*         outData,       /* pointer to output media data */
                    uint32_t         in_frameSize,  /* size of coded bitstream data, in bytes */
                    int              numChan,       /* number of channels to be decoded. Multichannel data must be interleaved */
                    CODEC_INARGS*    pInArgs,       /* optional parameters for decoding RTP payloads; see CODEC_INARGS struct notes above. If not used this param should be NULL */
                    CODEC_OUTARGS*   pOutArgs);     /* optional decoder output info; see CODEC_OUTARGS struct notes above. If not used this param should be NULL */

  int DSCodecTranscode(HCODEC*       hCodecSrc,
                       HCODEC*       hCodecDst,
                       unsigned int  uFlags,
                       uint8_t*      inData,
                       uint32_t      in_frameSize,  /* in bytes */
                       uint8_t*      outData,
                       int           numChan);

/* APIs that take either an hCodec returned by DSCodecCreate() or a codec_type enum. General notes:

   -DSGetCodecInfo() and DSGetPayloadInfo() accept either codec_type enums or hCodec handles depending on uFlags

   -a codec instance is an hCodec handle from a previous call to DSCodecCreate(), codec_type is one of the DS_CODEC_XXX enums in shared_include/codec.h

   -return values are < 0 on error conditions
*/

/* DSGetCodecInfo() returns information for the specified codec and uFlags (see below for uFlags definitions). Notes:

   -codec_param can be either a codec handle (HCODEC) or a codec_type enum, depending on uFlags. If DS_CODEC_INFO_HANDLE is given in uFlags then codec_param is interpreted as an hCodec, returned by a previous call to DSCodecCreate(). If both DS_CODEC_INFO_HANDLE and DS_CODEC_INFO_TYPE flags are given, codec_param is interpreted as an hCodec and the return value is a codec_type (cast as an int). If neither are given, DS_CODEC_INFO_TYPE is assumed as the default. For examples of DS_CODEC_INFO_TYPE and DS_CODEC_INFO_HANDLE usage, see packet_flow_media_proc.c and mediaTest_proc.c

   -if uFlags specifies DS_CODEC_INFO_TYPE, codec_param should be a DS_CODEC_XXX enum defined in shared_include/codec.h, and uFlags can also contain DS_CODEC_INFO_NAME, DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE, or DS_CODEC_INFO_PARAMS. If uFlags specifies DS_CODEC_INFO_TYPE_FROM_NAME, pInfo should contain a standard codec name string. Standardized SigSRF codec names are given in shared_include/codec.h

   -nInput1 and nInput2 are required for uFlags DS_CODEC_INFO_BITRATE_TO_INDEX, DS_CODEC_INFO_INDEX_TO_BITRATE, and DS_CODEC_INFO_CODED_FRAMESIZE

   -nInput1 is required for uFlags DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE, DS_CODEC_INFO_CMR_BITRATE, and DS_CODEC_INFO_LIST_TO_CLASSIFICATION

   -retrieved info is copied to pInfo if uFlags contains DS_CODEC_INFO_NAME, DS_CODEC_INFO_PARAMS, or DS_CODEC_INFO_TYPE_FROM_NAME

   -return value is > 0 for success, 0 if no information is available for the given uFlags, and < 0 for error conditions
*/

  int DSGetCodecInfo(int codec_param, unsigned int uFlags, int nInput1, int nInput2, void* pInfo);

/* DSGetPayloadInfo() returns payload format and other info for codec RTP payloads. Notes:

     -codec_param can be either a codec_type enum or a codec handle (an HCODEC returned by a prior call to DSCodecCreate()), depending on uFlags. In most cases uFlags should specify DS_CODEC_INFO_TYPE to interpret codec_param as an DS_CODEC_XXX enum defined in shared_include/codec.h. If neither are given, DS_CODEC_INFO_TYPE is assumed as the default. For examples of DS_CODEC_INFO_TYPE and DS_CODEC_INFO_HANDLE usage, see packet_flow_media_proc.c and mediaTest_proc.c

     -rtp_payload should point to an RTP payload

     -payload_size should give the size (in bytes) of the RTP payload pointed to by rtp_payload

     -if payload_info is not NULL it should point to a PAYLOAD_INFO struct fully initialized to zero, and the following payload_info items will be set or cleared:

       -codec_type is (i) set to a codec type associated with the codec handle given by codec param if uFlags contains DS_CODEC_INFO_HANDLE, or (ii) copied from codec_param if uFlags contains DS_CODEC_INFO_TYPE
       -uFormat is set to a DS_PYLD_FMT_XXX payload format definition (see above) for applicable codecs (e.g. AMR, EVS, H.26x)
       -NumFrames is set to the number of frames in the payload
       -FrameSize[] is set to the size of each frame in the payload if applicable to the codec type, or set to zero if not 
       -BitRate[] is set to the codec bitrate corresponding to the frame size

       -CMR is set to the payload change mode request value if present and applicable to the codec type, or set to zero if not
       -ToC[] is set to the payload header "table of contents" value for each frame in the payload if applicable to the codec type, or set to zero if not 
       -fSID is set if the payload is a SID (silence identifier), or cleared if not
       -fDTMF is set if the payload is a DTMF event, or cleared if not
       -only applicable to EVS, fAMRWB_IO_MOde is set true for an AMR-WB IO mode payload, false for a primary mode payload, and false for all other codec types
       -for H.26x codecs NALU_Header and FU_Header are extracted from payload header values 

     -sdp_info if not NULL should point to an SDP_INFO struct associated with the RTP payload. For example a video stream may not contain in-band vps, sps, or pps NAL units in which case DSGetPayloadInfo() can construct that information from "fmtp" SDP info fields. If sdp_info is not NULL and the first RTP payload for a stream does not contain xps info sdp_info will be scanned for sprop-vps, sprop-sps, and sprop-pps fields, converted from base64 into binary sequences, and inserted into the elementary bitstream

     -nId is an optional unique thread or session identifer that should only used be for output bitstream extraction and/or file write. If not used a value of -1 should be given, otherwise saved state information may become confused between threads or sessions

     -fp_out if not NULL should point to an open output binary file to append bitstream data extracted from payload contents per the relevant codec RTP payload specification

     -pInfo if not NULL should point to a memory buffer to copy bitstream data extracted from payload contents per the relevant codec RTP payload specification

     -return value is (i) a DS_PYLD_FMT_XXX payload format definition (see above) for applicable codecs (e.g. AMR, EVS, H.26x), (ii) 0 for other codec types, (iii) number of bytes written to bitstream file if fp_out is not NULL, or (iv) < 0 for error conditions
*/

  #define MAX_PAYLOAD_FRAMES 12  /* maximum number of frames per payload currently supported. For example, for a 20 msec nominal ptime codec, a max ptime of 240 msec is supported, CKJ Sep 2017 */


  typedef struct {  /* RTP payload items extracted or derived from a combination of codec type, payload header, and payload size */

  /* common items derived from payload header and size */

     codec_types  codec_type;                     /* set to codec type determined inside DSGetPayloadInfo() depending on uFlags containing DS_CODEC_INFO_HANDLE or DS_CODEC_INFO_TYPE (in the latter case it's a copy of codec_param). codec_types enums are defined in shared_include/codec.h with size int */

     uint8_t      uFormat;                        /* set to a DS_PYLD_FMT_XXX payload format definition for applicable codecs (e.g. AMR, EVS, H.26x), 0 otherwise */

     int16_t      NumFrames;                      /* number of frames in payload. On error set to number of valid payload frames processed before the error occurred */
     int32_t      FrameSize[MAX_PAYLOAD_FRAMES];  /* frame size in bytes for all codec types except AMR. For AMR (NB, WB, WB+) codecs this is frame size in bits (i.e. number of data bits in the frame, as shown at https://www.ietf.org/proceedings/50/slides/avt-8/sld009.htm). -1 indicates an error condition in payload ToC */
     int32_t      BitRate[MAX_PAYLOAD_FRAMES];    /* bitrate */

     struct {

    /* voice payload types, header values, or operating modes */

       uint8_t    CMR;                            /* change mode request value, if found in payload and applicable to the codec type, zero otherwise. Not shifted or otherwise modified */
       uint8_t    ToC[MAX_PAYLOAD_FRAMES];        /* payload header ToC (table of contents) if applicable to the codec type, including EVS and AMR, which can have multiple frames per payload (e.g. variable ptime), or multiple channels per payload (e.g. stereo or independent mono channels), or a mix. Zero otherwise */
       bool       fSID;                           /* true for a SID payload, false otherwise */
       bool       fAMRWB_IO_Mode;                 /* true for an EVS AMR-WB IO compatibility mode payload, false otherwise */
       bool       fDTMF;                          /* true for a DTMF event payload, false otherwise */
       uint8_t    Reserved[16];

     } voice;

     struct {

       uint8_t    Reserved[16];

     } audio;

  /* video payload header values */

     struct {

       uint16_t   NALU_Header;                    /* H.26x NAL unit header. Possible values include NAL unit types defined in H.26x specs and fragment and aggregation unit types defined only for RTP transport (RFCs 6184 and 7798) */
       uint8_t    FU_Header;                      /* H.26x Fragmentation Packet header */

       int        Reserved[32];

     } video;

  /* misc */

     int          start_of_payload_data;          /* index into rtp_payload[] of start of payload data. If DSGetPayloadInfo() returns an error condition, this is the payload header byte on which the error occurred */

     int          amr_decoder_bit_pos;            /* reserved */

     int          Reserved[16];

  } PAYLOAD_INFO;

  typedef struct {  /* SDP info associated with RTP payload, if any */
  
     uint8_t      payload_type;
     char*        fmtp;

  } SDP_INFO;

  int DSGetPayloadInfo(int codec_param, unsigned int uFlags, uint8_t* rtp_payload, int payload_size, PAYLOAD_INFO* payload_info, SDP_INFO* sdp_info, int nId, FILE* fp_out, void* pInfo);

  #define DS_PAYLOAD_INFO_SID_ONLY                       1    /* if DS_PAYLOAD_INFO_SID_ONLY is given in uFlags DSGetPayloadInfo() will make a quick check for a SID payload. codec_param should be a valid DS_CODEC_xxx enum (defined in shared_include/codec.h), uFlags must include DS_CODEC_INFO_TYPE, no error checking is performed, and fSID in payload_info will be set or cleared. If the payload contains multiple frames only the first frame is considered. Return values are a DS_PYLD_FMT_XXX value for a SID payload and -1 for not a SID payload */

  #define DS_PAYLOAD_INFO_GENERIC                        2    /* if DS_PAYLOAD_INFO_GENERIC is given in uFlags DSGetPayloadInfo() will ignore codec_param and rtp_payload and set fDTMF (DTMF event) in payload_info if payload_size = 4 or set fSID (SID payload) in payload_info if payload_size <= 8. This is reliable for most codecs for single-frame payloads; however, for multiple frames (e.g. variable ptime, multiple channels, etc) -- or any situation where detailed payload information is needed -- this flag should not be used. fDTMF and fSID in payload_info are set or cleared. Return values are 0 for a SID payload and -1 for not a SID payload */

  #define DS_PAYLOAD_INFO_NO_CODEC  DS_PAYLOAD_INFO_GENERIC  /* alternative flag name */

  #define DS_PAYLOAD_INFO_IGNORE_DTMF                    4   /* DSGetPayloadInfo() default behavior is to recognize payload_size == 4 as a DTMF event (RFC 4733), set NumFrames to 1 and set fDTMF in payload_info, and immediately return 0 without reference to codec_param and rtp_payload. To override this behavior DS_PAYLOAD_INFO_IGNORE_DTMF can be given in uFlags */ 

  #define DS_PAYLOAD_INFO_DEBUG_OUTPUT                   8   /* ask for additional debug output in DSGetPayloadInfo() */ 

  #define DS_PAYLOAD_INFO_RESET_ID                    0x10   /* reset unique stream or thread identifier */ 

  #define DS_PAYLOAD_INFO_IGNORE_INBAND_XPS           0x20   /* default DSGetPayloadInfo() behavior for video RTP streams is to favor inband xps info within the stream, and if not found insert SDP xps info when sdp_info contains an fmtp string. The DS_PAYLOAD_INFO_IGNORE_INBAND_XPS flag can be applied to override this behavior and force sdp_info to be used regardless of inband xps info. For example VLC output streams may contain repeated SDP info fmtp xps fields, but no inband xps info, in which case the mediaMin reference application will supply SDP info when calling DSGetPayloadInfo() */

  
/* DSGetPayloadHeaderToC() returns a nominal AMR or EVS payload header ToC based on payload size. Notes:

   -for EVS, this API should be called *only* with non-collision ("protected") payload sizes
   -for AMR, DSGetPayloadHeaderToC() calls DSGetPayloadInfo() internally, as information in the payload is required to determine its format (bandwidth-efficient vs octet-aligned), and in turn its ToC
   -for EVS payloads, payload is only needed for"special case" payloads (see spec section A.2.1.3, Special case for 56 bit payload size (EVS Primary or EVS AMR-WB IO SID), otherwise it can be given as NULL
   -the returned ToC is normally one byte, including a 4-bit FT (frame type) field, so it's returned in the low byte of the return value, but could be larger for other codecs if supported
   -see the AMR and EVS specs for ToC bit fields definitions
*/

  int DSGetPayloadHeaderToC(codec_types codec_type, uint8_t* payload, int payload_size);

/* #define DSGETPAYLOADSIZE // define to allow use of deprecated DSGetPayloadSize() */
  #ifdef DSGETPAYLOADSIZE

/* DSGetPayloadSize() returns payload size, given a codec_type enum and bitrate code. Notes:

  -this API is deprecated and scheduled to be replaced by DSGetCodecInfo() with DS_CODEC_INFO_CODED_FRAMESIZE flag
  -it was only used by mediaTest when processing encoded input files (.cod files)
  -for EVS, the bitrate code is the lower 4 bits of the ToC byte
*/

  int DSGetPayloadSize(codec_types codec_type, unsigned int bitrate_code);
  #endif

#ifdef __cplusplus
}
#endif

/* DSConfigVoplib() uFlags */

#if 0  /* deprecated -- if GLOBAL_CONFIG* and DEBUG_CONFIG* params are non-NULL they are used */
#define DS_CV_GLOBALCONFIG                            0x01
#define DS_CV_DEBUGCONFIG                             0x02
#endif
#define DS_CV_INIT                                    0x04

/* DSCodecCreate() uFlags */

#define DS_CODEC_CREATE_ENCODER                       0x01  /* create an encoder instance - may be combined with DS_CODEC_CREATE_DECODER */
#define DS_CODEC_CREATE_DECODER                       0x02  /* create a decoder instance - may be combined with DS_CODCEC_CREATE_ENCODER */
#define DS_CODEC_CREATE_USE_TERMINFO                 0x100  /* pCodecInfo points to a TERMINATION_INFO struct */
#define DS_CODEC_CREATE_NO_MEM_BUFS                  0x200  /* create codec instance with no mem buffers - Reserved, for test purposes only; codec handle is not valid for use */

/* DSCodecDecode() uFlags */

#define DS_CODEC_GET_NUMFRAMES                       0x100  /* if specified in uFlags, DSCodecDecode() returns the number of frames in the payload. No decoding is performed */

/* DSGetCodecInfo() flags */

#define DS_CODEC_INFO_HANDLE                         0x100  /* codec_param is interpreted as an hCodec (i.e. handle created by prior call to DSCodecCreate() */ 
#define DS_CODEC_INFO_TYPE                           0x200  /* codec_param is interpreted as a codec_type enum (see codec_type enums defined in shared_include/codec.h). This is the default if neither DS_CODEC_INFO_HANDLE nor DS_CODEC_INFO_TYPE is given. If both DS_CODEC_INFO_HANDLE and DS_CODEC_INFO_TYPE are given a codec_type enum is returned (cast as an int) */ 

/* DSGetCodecInfo() item flags. If no item flag is given, DS_CODEC_INFO_HANDLE should be specified and pInfo is expected to point to a CODEC_PARAMS struct. Some item flags must be combined with the DS_CODEC_INFO_HANDLE flag (see per-flag comments) */

#define DS_CODEC_INFO_NAME                            0x01  /* returns a codec name as a string pointed to by pInfo. Typically string length is 5-10 char, always less than CODEC_NAME_MAXLEN. Return value is length of codec name string */
#define DS_CODEC_INFO_RAW_FRAMESIZE                   0x02  /* returns codec media frame size (i.e. prior to encode, after decode), in bytes. If DS_CODEC_INFO_HANDLE is not given, returns default media frame size for one ptime. For EVS, nInput1 should specify one of the four (4) EVS sampling rates (in Hz) */
#define DS_CODEC_INFO_MEDIA_FRAMESIZE  DS_CODEC_INFO_RAW_FRAMESIZE
#define DS_CODEC_INFO_CODED_FRAMESIZE                 0x03  /* returns codec coded frame size (i.e. after encode, prior to decode), in bytes. If uFlags specifies DS_CODEC_INFO_TYPE then nInput1 should give a bitrate and nInput2 should give a payload format (see DS_PYLD_FMT_XXX definitions above) */
#define DS_CODEC_INFO_BITRATE                         0x04  /* returns codec bitrate in bps. Requires DS_CODEC_INFO_HANDLE flag */
#define DS_CODEC_INFO_SAMPLERATE                      0x05  /* returns codec sampling rate in Hz. If DS_CODEC_INFO_HANDLE is not given, returns default sample rate for the specified codec. For EVS, nInput1 can specify one of the four (4) EVS sampling rates with values 0-3 */
#define DS_CODEC_INFO_PTIME                           0x06  /* returns ptime in msec. Default value is raw framesize / sampling rate at codec creation-time, then is dynamically adjusted as packet streams are processed. Requires DS_CODEC_INFO_HANDLE flag */
#define DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE           0x07  /* given an nInput1 sample rate in Hz, returns sample rate code specified in "xxx_codec_flags" enums in shared_include/codec.h, where xxx is the codec abbreviation (e.g. evs_codec_flags or melpe_codec_flags) */
#define DS_CODEC_INFO_BITRATE_TO_INDEX                0x08  /* converts a codec bitrate (nInput1) to an index 0-31 (currently only EVS and AMR codecs supported) */
#define DS_CODEC_INFO_INDEX_TO_BITRATE                0x09  /* inverse */
#define DS_CODEC_INFO_PAYLOAD_SHIFT                   0x0a  /* returns payload shift specified in CODEC_PARAMS or TERMINATION_INFO structs at codec creation time, if any. Requires DS_CODEC_INFO_HANDLE flag. Default value is zero */
#define DS_CODEC_INFO_LIST_TO_CLASSIFICATION          0x0c  /* converts a codec audio classification list item (nInput1) to 0-3 letter classification string (currently EVS and AMR codecs supported) */
#define DS_CODEC_INFO_TYPE_FROM_NAME                  0x0d  /* returns a codec_type enum (defined in shared_include/codec.h) if pInfo contains a standard codec name. codec_param is ignored. Standardized codec names used in all SigSRF libs and applications are in get_codec_type_from_name() in shared_include/codec.h. Returns -1 if pInfo is NULL or codec name is not found */
#define DS_CODEC_INFO_CMR_BITRATE                     0x0e  /* returns requested bitrate from CMR (Change Mode Request) value given in nInput1. Applicable only to codecs that use CMR */

#define DS_CODEC_INFO_BITRATE_CODE                   0x800  /* when combined with DS_CODEC_INFO_CODED_FRAMESIZE, indicates nInput1 should be treated as a "bitrate code" instead of a bitrate. A bitrate code is typically found in the RTP payload header, for example a 4 bit field specifying 16 possible bitrates. Currently only EVS and AMR codecs support this flag, according to Table A.4 and A.5 in section A.2.2.1.2, "ToC byte" of EVS spec TS 26.445. See mediaTest source code for usage examples */

#define DS_CODEC_INFO_SIZE_BITS                     0x1000  /* indicates DS_CODEC_INFO_CODED_FRAMESIZE return value should be in size of bits (instead of bytes) */

#define DS_CODEC_INFO_NAME_VERBOSE                  0x2000  /* when combined with the DS_CODEC_INFO_NAME flag, additional information is included in some codec names, for example "L16" becomes "L16 (linear 16-bit PCM)". When this flag is used the resulting codec name should not be used with the DS_CODEC_TYPE_FROM_NAME flag */

#define DS_CODEC_INFO_ITEM_MASK                       0xff  /* mask to help when isolating above DS_CODEC_INFO_xxx items */

/* general API flags */

#define DS_CODEC_TRACK_MEM_USAGE                     0x400  /* track instance memory usage, can be applied to DSCodecCreate() and DSCodecDelete() */
#define DS_CODEC_USE_EVENT_LOG                       0x800  /* Use the SigSRF diaglib event log for progress, debug, and error messages. By default codec event and error logging is handled according to uEventLogMode element of a DEBUG_CONFIG struct specified in DSConfigVoplib(). uEventLogMode should be set with EVENT_LOG_MODE enums in shared_include/config.h. This flag may be combined with uFlags in DSCodecCreate() and/or uFlags in CODEC_ENC_PARAMS and CODEC_DEC_PARAMS structs to override uEventLogMode */

#define DS_VOPLIB_SUPPRESS_WARNING_ERROR_MSG   0x20000000L  /* suppress voplib API error and warning messages, e.g. DSGetCodecInfo() codec type not found. Note these are same values used in pktlib.h */
#define DS_VOPLIB_SUPPRESS_INFO_MSG            0x40000000L  /* suppress voplib API info messages; e.g. DSGetPayloadInfo() fp_out NULL when extracting video payloads */

/* definitions for uFlags in CODEC_ENC_PARAMS and CODEC_DEC_PARAMS */

#define RTP_FORMAT_ENCODER_NO_AMRWBIO_PADDING_BYTES      1
#define RTP_FORMAT_ENCODER_NO_VBR_PADDING_BYTES          2
#define RTP_FORMAT_DECODER_IGNORE_AMRWBIO_PADDING_BYTES  4
#define RTP_FORMAT_DECODER_IGNORE_VBR_PADDING_BYTES      8
#define RTP_FORMAT_ENCODER_NOREQ_CMR                  0x10  /* insert a "NO_REQ" CMR at start of output. Intended for test/debug purposes, or if apps want the encoder to do this prior to transmission. To insert a specific CMR value a CODEC_INARGS struct pointer should be given in pInAargs, and the struct's CMR field should be set as needed */

#define DEBUG_OUTPUT_VOPLIB_ONTHEFLY_UPDATES       0x10000  /* show on-the-fly updates at voplib level */
#define DEBUG_OUTPUT_CODEC_LIB_ONTHEFLY_UPDATES    0x20000  /* show on-the-fly updates at encoder or decoder lib level */

#define DEBUG_OUTPUT_VOPLIB_PADDING_BYTE_APPEND    0x40000  /* show encoder padding bytes when appended */
#define DEBUG_OUTPUT_VOPLIB_SHOW_BITSTREAM_BYTES   0x80000  /* show input bitstream bytes on entry to DSCodecDecode(), or output bitstream bytes on exit from DSCodecEncode(). Displayed values are compressed bitstream bytes in hex format */
#define DEBUG_OUTPUT_VOPLIB_SHOW_INTERNAL_INFO    0x100000  /* show decoder or encoder internal info, once for each framesize. Internal info includes CMR, I/O mode, payload format, framesize, and first payload byte */
#define DEBUG_OUTPUT_SHOW_INIT_PARAMS             0x200000  /* show encoder and/or decoder init params when instance is created. Note this flag is only active during DSCodecCreate() */

#define DEBUG_TEST_ABORT_EXIT_INTERCEPTION        0x400000  /* test abort() and exit() function interception at encoder or decoder lib level. Interception prevents abort() or exit() from terminating the libary and calling application, and allows code flow to continue. The test will simulate one abort() and one exit() interception, and display an example event log error message. Note that this only applies to codecs with embedded abort() and exit() functions that cannot -- or are not allowed to -- be removed, for example due to (i) binary implementation, (ii) possible license violation if source code is modified */

#define DEBUG_OUTPUT_VOPLIB_CMR_INFO              0x800000  /* show decoder and encoder CMR related debug output */

#define DEBUG_OUTPUT_ADD_TO_EVENT_LOG            0x1000000  /* add debug output to event log */

#endif  /* _VOPLIB_H_ */
