/*
 $Header: /root/Signalogic/DirectCore/include/voplib.h

 Copyright (C) Signalogic Inc. 2010-2024

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
  Modified Oct 2018 JHB, removed #ifdef CODECXXX_INSTALLED references for codec header file includes.  The voplib Makefile no longer sets these, and the install script installs all codec header files (codec lib files are delivery dependent). stublib is used by mediaTest and mediaMin makefiles to link only the installed codecs
  Modified Jan 2019 JHB, remove uFlags param from DSCodecDelete() (it was never used, the codec handle links to internal info that contains flags if any)
  Modified Jul 2019 JHB, modify input struct ptr param and add flags for DSCodecCreate(), which is now used in both packet flow and codec test mode.  Flags include type of input struct data, and create encoder, decoder, or both
  Modified Jul 2019 JHB, DSGetCodecFs() removed, use DSGetCodecSampleRate(hCodec) instead
  Modified Feb 2020 JHB, add DS_GET_NUMFRAMES flag in DSCodecDecode()
  Modified Oct 2020 JHB, add DSCodecGetInfo() API to pull all available encoder/decoder info as needed. First input param is codec handle or codec_type, depending on uFlags. Added codec_name, raw_frame_size, and coded_frame_size elements to CODEC_PARAMS struct support DSCodecGetInfo()
  Modified Jan 2022 JHB, add DS_CC_TRACK_MEM_USAGE flag
  Modified Feb 2022 JHB, change DSGetCodecTypeStr() to DSGetCodecName() and add uFlags to allow codec param to be interpreted as either HCODEC or codec_type (as with DSGetCodecInfo)
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
#endif

#if defined(CODECS_ONLY) && !defined(NO_VOPLIB_HEADERS)
  #include "shared_include/codec.h"        /* bring in codec types from shared_include subfolder */
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

#define MAX_FS             48       /* 48 kHz */
#define MAX_SAMPLES_FRAME  (MAX_FS*NOM_PTIME)  /* nominal value 960 samples */
#define MAX_RAW_FRAME      (MAX_SAMPLES_FRAME*sizeof(int16_t))  /* maximum raw audio frame size, in bytes. Nominal value 1920 bytes*/
#define MAX_CODED_FRAME    328      /* AMR-WB+: 80 byte max frame size + 2 byte header * 4 sub frames */
#define MAX_AUDIO_CHAN     100      /* max audio channels supported in the mediaTest refererence application. Note this channel count is completely separate from max channels in pktlib and the mediaMin reference app */
#define MAX_FSCONV_UP_DOWN_FACTOR  160  /* current maximum Fs conversion up/down factor allowed in mediaTest and mediaMin reference apps. This is also referred to in alglib */

/* Payload header format definitions for EVS and AMR codec formats */

#define HEADERCOMPACT       0
#define HEADERFULL          1
#define BANDWIDTHEFFICIENT  HEADERCOMPACT
#define OCTETALIGN          HEADERFULL

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

/* voplib config API.  pGlobalConfig and pDebugConfig point to GLOBAL_CONFIG and DEBUG_CONFIG structs defined in config.h.  See DS_CV_xx flags below */

  int DSConfigVoplib(GLOBAL_CONFIG* pGlobalConfig, DEBUG_CONFIG* pDebugConfig, unsigned int uFlags);  /* global config, debug config, or both can be configured depending on attributes specified in uFlags.  NULL should be given for either pointer not used */
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
 
    FRAMETYPE_VOICED,              /* speech, voiced */
    FRAMETYPE_UNVOICED,            /* speech, unvoiced */
    FRAMETYPE_SID,                 /* SID (silence / comfort noise) frames for codecs that support DTX */
    FRAMETYPE_NODATA,              /* untransmitted frame for codecs that support DTX */
    FRAMETYPE_NOISE,               /* background noise for codecs that support audio classification */
    FRAMETYPE_AUDIO,               /* sounds and other audio for codecs that support audio classification */
    FRAMETYPE_MUSIC,               /* music for codecs that support audio classification */

/* flags that may be combined with above types */

    FRAMETYPE_TRANSITION = 0x100,  /* transition between types */
    FRAMETYPE_MELP = 0x200,        /* low bitrate voiced (mixed excited linear prediction) */
    FRAMETYPE_NELP = 0x400,        /* low bitrate voiced (noise excited linear prediction) */

    FRAMETYPE_ITEM_MASK = 0xff     /* mask to separate items from flags */
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

     int codec_type;                      /* specifies codec type -- see "voice_codec_type" enums in shared_include/codec.h */
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
                    uint8_t*         inData,        /* pointer to input audio data */
                    uint8_t*         outData,       /* pointer to output coded bitstream data */
                    uint32_t         in_frameSize,  /* size of input audio data, in bytes */
                    int              numChan,       /* number of channels to be encoded. Multichannel data must be interleaved */
                    CODEC_INARGS*    pInArgs,       /* optional parameters for encoding audio data; see CODEC_INARGS struct notes above. If not used this param should be NULL */
                    CODEC_OUTARGS*   pOutArgs);     /* optional encoder output info; see CODEC_OUTARGS struct notes above. If not used this param should be NULL */

  int DSCodecDecode(HCODEC*          hCodec,        /* pointer to one or more codec handles, as specified by numChan */
                    unsigned int     uFlags,        /* flags, see DS_CODEC_DECODE_xxx flags below */
                    uint8_t*         inData,        /* pointer to input coded bitstream data */
                    uint8_t*         outData,       /* pointer to output audio data */
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

/* APIs that take an hCodec returned by DSCodecCreate(). Notes:

   -return values are -1 or otherwise < 0 on error conditions
   -DSGetCodecInfo() accept flags to specify a codec type or hCodec handle input; see additional comments below
*/

/* DSGetCodecType() returns codec type, see "voice_codec_type" enums in shared_include/codec.h. hCodec must be a valid codec handle generated by DSCodecCreate() */

  int DSGetCodecType(HCODEC hCodec);

/* DSGetCodecInfo() returns information for the specified codec and uFlags (see below for uFlags definitions). Notes:

   -codec can be either a codec handle (HCODEC) or a codec type (int), depending on uFlags. In most cases uFlags should specify DS_CODEC_INFO_HANDLE to interpret codec as an hCodec, returned by a previous call to DSCodecCreate(). If neither DS_CODEC_INFO_HANDLE or DS_CODEC_INFO_TYPE is given, the default is DS_CODEC_INFO_HANDLE
   -if uFlags specifies DS_CODEC_INFO_TYPE, codec should be one of the types specified in shared_include/codec.h, and uFlags can also contain DS_CODEC_INFO_NAME, DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE, or DS_CODEC_INFO_PARAMS
   -returned info is copied into pInfo for uFlags DS_CODEC_INFO_NAME and DS_CODEC_INFO_PARAMS
   -nInput1 and nInput2 are required for uFlags DS_CODEC_INFO_BITRATE_TO_INDEX, DS_CODEC_INFO_INDEX_TO_BITRATE, and DS_CODEC_INFO_CODED_FRAMESIZE
   -nInput1 is required for the DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE flag
*/
  
  int DSGetCodecInfo(int codec, unsigned int uFlags, int nInput1, int nInput2, void* pInfo);

/* more codec helper functions, these are generic. Notes:

  -generic means not requiring an existing codec instance handle, so these functions accept a codec type
  -an existing codec instance is one already created and/or running dynamically, in which case an hCodec must be provided (see above functions)
*/

/* DSGetPayloadInfo() returns header format and other info for codec RTP payloads. Notes, JHB Oct 2022:

     -codec can be either a codec type (int) or a codec handle (an HCODEC returned by a pevious call to DSCodecCreate()), depending on uFlags. In most cases uFlags should specify DS_CODEC_INFO_TYPE to interpret codec as one of the types specified in shared_include/codec.h. If neither DS_CODEC_INFO_HANDLE or DS_CODEC_INFO_TYPE is given, the default is DS_CODEC_INFO_TYPE
     -payload should point to a codec RTP payload
     -payload_len should give the size (in bytes) of the RTP payload pointed to by payload
     -if payload_info is non-NULL then:
       -payload_info->CMR is set to the payload CMR if present, or zero if not
       -payload_info->Header_Format is a copy of the return value, excluding error conditions
       -payload_info->fSID is set true if the packet is a SID, or false if not
       -only for EVS, payload_info->fAMRWB_IO_MOde is set true for an AMR-WB IO mode payload, or false for a primary mode payload (and set false for all other codec types)

     -return value is (i) 0 for EVS CH (compact header) format or AMR bandwidth-efficient format, (ii) 1 for EVS FH (full header) format or AMR octet-align format, (iii) 0 for other codecs, and (iv) -1 for error conditions
*/

  typedef struct {  /* items extracted or derived from a combination of codec type, payload header, and payload size */

  /* payload header items */

     uint8_t   CMR;             /* change mode request value, if applicable to codec type. Zero otherwise */
     uint16_t  ToC;             /* payload header ToC (table of contents) if applicable to codec type. Zero otherwise */
     uint16_t  NALU_Hdr;        /* H.26x NALU header */

  /* payload types or operating modes */

     uint8_t   Header_Format;   /* 0 for compact header / bandwidth efficient, 1 for header full / octet aligned, 0 otherwise */
     bool      fSID;            /* true for a SID packet, false otherwise */
     bool      fAMRWB_IO_Mode;  /* true for EVS AMR-WB IO compatibility mode, false otherwise */

  } PAYLOAD_INFO;

  int DSGetPayloadInfo(int codec, unsigned int uFlags, uint8_t* payload, unsigned int payload_len, PAYLOAD_INFO* payload_info);

/* DSGetPayloadHeaderToC() returns a nominal AMR or EVS payload header ToC based on payload size. For EVS, this API should be called *only* with compact header mode and non-collision payload sizes. The ToC is normally one byte, so it's returned in the low byte of the return value, but could be larger for future codecs. See the AMR and EVS specs for ToC bit fields definitions */

  int DSGetPayloadHeaderToC(unsigned int codec_type, unsigned int pyld_len);

/* #define DSGETPAYLOADSIZE // define to allow use of deprecated DSGetPayloadSize() */
#ifdef DSGETPAYLOADSIZE

/* DSGetPayloadSize() returns payload size, given a codec type and bitrate code. Notes:

  -this API is deprecated and scheduled to be replaced by DSGetCodecInfo() with DS_CODEC_INFO_CODED_FRAMESIZE flag
  -it was only used by mediaTest when processing encoded input files (.cod files)
  -for EVS, the bitrate code is the lower 4 bits of the ToC byte
*/

  int DSGetPayloadSize(unsigned int codec_type, unsigned int bitrate_code);
#endif


#ifdef __cplusplus
}
#endif

/* DSConfigVoplib() uFlags */

#if 0  /* deprecated -- if GLOBAL_CONFIG* and DEBUG_CONFIG* params are non-NULL they are used */
#define DS_CV_GLOBALCONFIG                     0x01
#define DS_CV_DEBUGCONFIG                      0x02
#endif
#define DS_CV_INIT                             0x04

/* DSCodecCreate() uFlags */

#define DS_CODEC_CREATE_ENCODER                0x01   /* create an encoder instance - may be combined with DS_CODEC_CREATE_DECODER */
#define DS_CODEC_CREATE_DECODER                0x02   /* create a decoder instance - may be combined with DS_CODCEC_CREATE_ENCODER */
#define DS_CODEC_CREATE_USE_TERMINFO           0x100  /* pCodecInfo points to a TERMINATION_INFO struct */
#define DS_CODEC_CREATE_NO_MEM_BUFS            0x200  /* create codec instance with no mem buffers - Reserved, for test purposes only; codec handle is not valid for use */

/* DSCodecDecode() uFlags */

#define DS_CODEC_GET_NUMFRAMES                 0x100  /* if specified in uFlags, DSCodecDecode() returns the number of frames in the payload. No decoding is performed */

/* DSGetCodecInfo() flags */

#define DS_CODEC_INFO_HANDLE                   0x100  /* specifies the "codec" param (first param) is interpreted as an hCodec (i.e. handle created by prior call to DSCodecCreate(). This is the default if neither DS_CODEC_INFO_HANDLE or DS_CODEC_INFO_TYPE is given */ 
#define DS_CODEC_INFO_TYPE                     0x200  /* specifies the "codec" param (first param) is interpreted as a codec_type. If both DS_CODEC_INFO_HANDLE and DS_CODEC_INFO_TYPE are given the return value is codec type */ 

/* DSGetCodecInfo() item flags. If no item flag is given, DS_CODEC_INFO_HANDLE should be specified and pInfo is expected to point to a CODEC_PARAMS struct. Some item flags must be combined with the DS_CODEC_INFO_HANDLE flag (see per-flag comments) */

#define DS_CODEC_INFO_NAME                     0x01   /* returns codec name in text string pointed to by pInfo. Typically string length is 5-10 char, always less than CODEC_NAME_MAXLEN char */
#define DS_CODEC_INFO_RAW_FRAMESIZE            0x02   /* returns codec media frame size (i.e. prior to encode, after decode), in bytes. If DS_CODEC_INFO_HANDLE is not given, returns default media frame size for one ptime. For EVS, nInput1 should specify one of the four (4) EVS sampling rates (in Hz) */
#define DS_CODEC_INFO_CODED_FRAMESIZE          0x03   /* returns codec compressed frame size (i.e. after encode, prior to decode), in bytes. If uFlags specifies DS_CODEC_INFO_TYPE then nInput1 should give a bitrate and nInput2 should give header format (0 or 1) */
#define DS_CODEC_INFO_BITRATE                  0x04   /* returns codec bitrate in bps. Requires DS_CODEC_INFO_HANDLE flag */
#define DS_CODEC_INFO_SAMPLERATE               0x05   /* returns codec sampling rate in Hz. If DS_CODEC_INFO_HANDLE is not given, returns default sample rate for the specified codec. For EVS, nInput1 can specify one of the four (4) EVS sampling rates with values 0-3 */
#define DS_CODEC_INFO_PTIME                    0x06   /* returns ptime in msec. Default value is raw framesize / sampling rate at codec creation-time, then is dynamically adjusted as packet streams are processed. Requires DS_CODEC_INFO_HANDLE flag */
#define DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE    0x07   /* given an nInput1 sample rate in Hz, returns sample rate code specified in "xxx_codec_flags" enums in shared_include/codec.h, where xxx is the codec abbreviation (e.g. evs_codec_flags or melpe_codec_flags) */
#define DS_CODEC_INFO_BITRATE_TO_INDEX         0x08   /* converts a codec bitrate (nInput1) to an index 0-31 (currently only EVS and AMR codecs supported) */
#define DS_CODEC_INFO_INDEX_TO_BITRATE         0x09   /* inverse */
#define DS_CODEC_INFO_PAYLOAD_SHIFT            0x0a   /* returns payload shift specified in CODEC_PARAMS or TERMINATION_INFO structs at codec creation time, if any. Requires DS_CODEC_INFO_HANDLE flag. Default value is zero */
#define DS_CODEC_INFO_CLASSIFICATION_TO_INDEX  0x0b   /* converts a codec audio classification (nInput1) to an index 0-31 (currently only EVS and AMR codecs supported) */
#define DS_CODEC_INFO_INDEX_TO_CLASSIFICATION  0x0c   /* inverse */

#define DS_CODEC_INFO_BITRATE_CODE             0x400  /* when combined with DS_CODEC_INFO_CODED_FRAMESIZE, indicates nInput1 should be treated as a "bitrate code" instead of a bitrate. A bitrate code is typically found in the RTP payload header, for example a 4 bit field specifying 16 possible bitrates. Currently only EVS and AMR codecs support this flag, according to Table A.4 and A.5 in section A.2.2.1.2, "ToC byte" of EVS spec TS 26.445. See mediaTest source code for usage examples */

#define DS_CODEC_INFO_SIZE_BITS                0x800  /* indicates DS_CODEC_INFO_CODED_FRAMESIZE return value should be in size of bits (instead of bytes) */

#define DS_CODEC_INFO_SUPPRESS_WARNING_MSG     0x1000

#define DS_CODEC_INFO_ITEM_MASK                0xff

/* general API flags */

#define DS_CODEC_TRACK_MEM_USAGE                         0x400      /* track instance memory usage, can be applied to DSCodecCreate() and DSCodecDelete() */
#define DS_CODEC_USE_EVENT_LOG                           0x800      /* Use the SigSRF diaglib event log for progress, debug, and error messages. By default codec event and error logging is handled according to uEventLogMode element of a DEBUG_CONFIG struct specified in DSConfigVoplib(). uEventLogMode should be set with EVENT_LOG_MODE enums in shared_include/config.h. This flag may be combined with uFlags in DSCodecCreate() and/or uFlags in CODEC_ENC_PARAMS and CODEC_DEC_PARAMS structs to override uEventLogMode */

/* definitions for uFlags in CODEC_ENC_PARAMS and CODEC_DEC_PARAMS */

#define RTP_FORMAT_ENCODER_NO_AMRWBIO_PADDING_BYTES      1
#define RTP_FORMAT_ENCODER_NO_VBR_PADDING_BYTES          2
#define RTP_FORMAT_DECODER_IGNORE_AMRWBIO_PADDING_BYTES  4
#define RTP_FORMAT_DECODER_IGNORE_VBR_PADDING_BYTES      8
#define RTP_FORMAT_ENCODER_FORCE_CMR                     0x10       /* force CMR to be inserted at start of output (value of 0xff "NO_REQ" is used). Intended for test/debug purposes */

#define DEBUG_OUTPUT_VOPLIB_ONTHEFLY_UPDATES             0x10000    /* show on-the-fly updates at voplib level */
#define DEBUG_OUTPUT_CODEC_LIB_ONTHEFLY_UPDATES          0x20000    /* show on-the-fly updates at encoder or decoder lib level */

#define DEBUG_OUTPUT_VOPLIB_PADDING_BYTE_APPEND          0x40000    /* show encoder padding bytes when appended */
#define DEBUG_OUTPUT_VOPLIB_SHOW_BITSTREAM_BYTES         0x80000    /* show input bitstream bytes on entry to DSCodecDecode(), or output bitstream bytes on exit from DSCodecEncode(). Displayed values are compressed bitstream bytes in hex format */
#define DEBUG_OUTPUT_VOPLIB_SHOW_INTERNAL_INFO           0x100000   /* show decoder or encoder internal info, once for each framesize. Internal info includes CMR, I/O mode, header and/or payload format, framesize, and first payload byte */
#define DEBUG_OUTPUT_SHOW_INIT_PARAMS                    0x200000   /* show encoder and/or decoder init params when instance is created. Note this flag is only active during DSCodecCreate() */

#define DEBUG_TEST_ABORT_EXIT_INTERCEPTION               0x400000   /* test abort() and exit() function interception at encoder or decoder lib level. Interception prevents abort() or exit() from terminating the libary and calling application, and allows code flow to continue. The test will simulate one abort() and one exit() interception, and display an example event log error message. Note that this only applies to codecs with embedded abort() and exit() functions that cannot -- or are not allowed to -- be removed, for example due to (i) binary implementation, (ii) possible license violation if source code is modified */

#define DEBUG_OUTPUT_ADD_TO_EVENT_LOG                    0x1000000  /* add debug output to event log */

#endif  /* _VOPLIB_H_ */
