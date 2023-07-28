/*
 $Header: /root/Signalogic/DirectCore/include/voplib.h

 Copyright (C) Signalogic Inc. 2010-2023

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
  Modified Feb 2022 JHB, modify DSCodecEncode() and DSCodecDecode() to accept pointer to one or more codec handles and a num channels param. This enables multichannel encoding/decoding (e.g. stereo audio files) and simplified concurrent codec instance test and measurement (e.g. 30+ codec instances within one thread or CPU core). Multichannel input and output data must be interleaved; see API comments and examples in x86_mediaTest.c
  Modified Sep 2022 JHB, add "payload_shift" to CODEC_DEC_PARAMS struct to allow RTP payload shift after encoding or before decoding for debug/test purposes. Shift conditions can be controlled by filter flags (in bits 15-8); shift amount ranges from -8 to +7 (in bits 7-0); see comments in TERMINATION_INFO struct in shared_include/session.h
  Modified Oct 2022 JHB, change DSGetPayloadHeaderFormat() to DSGetPayloadHeaderInfo() to reflect updates for additional params and info retrieval
  Modified Oct 2022 JHB, add pBitrateIndex param to DSGetCompressedFramesize()
  Modified Oct 2022 JHB, consolidate DSGetCodecName() and several others into DSGetCodecInfo() API, following pktlib model
  Modified Mar 2023 JHB, add pInArgs param to DSCodecEncode(), add CODEC_INARGS struct definition, add bitRate and CMR params to CODEC_OUTARGS struct. These were previously in license-only versions now they are public. See comments
  Modified Jul 2023 JHB, add CODEC_NAME_MAXLEN definition (used when manipulating codec name strings)
*/
 
#ifndef _VOPLIB_H_
#define _VOPLIB_H_

#include "alias.h"
  
/* session struct and related definitions */
#include "session_cmd.h"

/* algorithm related definitions */

#include "alglib.h"

/* Sampling frequency and frame size max values, max, min, and nominal ptimes */
#if 0
  #define MAX_PTIME        20       /* 20 ms */
#else
  #define MAX_PTIME        60       /* in msec */
  #define MIN_PTIME        20       /* in msec */
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
#define OCTETALIGNED        HEADERFULL

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

  enum encoder_frametype {
 
    ENCODER_FRAMETYPE_SPEECH = 0,      /* speech frame */
    ENCODER_FRAMETYPE_SIDFRAME = 1,    /* SID frames for codecs that support DTX */
    ENCODER_FRAMETYPE_NODATA = 2,      /* untransmitted frame for codecs that support DTX */
    ENCODER_FRAMETYPE_NOISE = 3,       /* background noise for codecs that support audio classification */
    ENCODER_FRAMETYPE_SOUND = 4,       /* sound for codecs that support audio classification */
    ENCODER_FRAMETYPE_MUSIC = 5        /* music for codecs that support audio classification */

  };
 
  typedef struct {  /* optional additional output from DSCodecEncode() and DSCodecDecode() APIs */

   short int size;                     /* generic size field, used differently by codecs */
   short int frameType;                /* possible frame types after encoder classifies audio */
   int extendedError;

   uint8_t CMR;                        /* CMR (Codec Mode Request) received from remote endpoint, if supported by the codec type. Notes:

                                          -for AMR codecs CMR will contain the value in the payload header being decoded. Examples include 0xf0 (no mode request), 0x20 (AMR-WB 12.65 bps), 0x70 (AMR-NB 1.20 kbps), etc. If the remote endpoint sent "no mode request", CMR will be 0xf0
                                          -for EVS codecs CMR will be non-zero only if sent by the remote endpoint. Examples include 0x80 (CMR = 0), 0xa4 (CMR = 0x24), 0x92 (CMR = 0x12), etc. CMR will be non-zero if (a) the remote endpoint is sending in headerfull format and includes a CMR byte or (b) the remote endpoint is sending in AMR-WB IO mode compact format
                                          -received CMR values are not shifted in any way. For octet aligned and headerfull formats, CMR contains the whole byte as received (including H bit or R bits as applicable). For bandwidth efficent and compact formats, CMR contains the partial 4 or 3 bits, in the exact position they were received, with other CMR bits zero
                                       */

   int bitRate;                        /* bitrate detected by the decoder (in bps), if supported by the codec type */

  } CODEC_OUTARGS;

  typedef struct {  /* optional additional input to DSCodecEncode() API */

   uint8_t CMR;                        /* CMR (Codec Mode Request) to be sent to remote endpoint, if any. Notes:

                                          -for AMR codecs if pInArgs is non-NULL then CMR will be sent to the remote endpoint, in both octet-aligned and bandwdith efficient formats. If "no mode request" should be sent, then specify 0xf0. When pInArgs is NULL, 0xf0 is sent by voplib (i.e. no mode request)
                                          -for EVS codecs using headerfull format, if pInArgs is non-NULL then non-zero CMR values will be sent. Examples include 0x80 (CMR = 0), 0xa4 (CMR = 0x24), 0x92 (CMR = 0x12), etc. Note the CMR value msb should be set in order to comply with section A.2.2.1.1 in the EVS specification (the "H" bit). When pInArgs is NULL, or when compact format is in use, no CMR is sent
                                          -for EVS codecs using AMR-WB IO mode in compact format if pInArgs is non-NULL then CMR will be sent to the remote endpoint. Examples include 0 (6.6 kbps), 0xc0 (23.85 kbps), 0xe0 (no mode request), etc.
                                          -CMR should not be shifted in any way. For octet aligned and headerfull formats, CMR should give the whole byte to insert in outgoing payloads (including H bit or R bits as applicable). For bandwidth efficent and compact formats, CMR should give the partial 4 or 3 bits, in the exact position within a payload byte as shown in the codec spec, with the rest of CMR zero'ed
                                       */

   CODEC_ENC_PARAMS* pCodecEncParams;  /* to change bitRate or codec-specific parameters within the duration of a codec instance, specify a CODEC_ENC_PARAMS struct. Note this only applies to newer, advanced codecs such as EVS and OPUS, JHB Mar 2023 */

  } CODEC_INARGS;


/* CODEC_PARAMS struct used in DSCodecCreate() and DSGetCodecInfo() */

  #define CODEC_NAME_MAXLEN  50

  typedef struct {

     int codec_type;               /* specifies codec type -- see "voice_codec_type" enums in shared_include/session.h */
     char codec_name[CODEC_NAME_MAXLEN];  /* pointer to codec name string that will be filled in. Note this is the same string as returned by DSGetCodecInfo() with DS_CODEC_INFO_NAME flag */
     uint16_t raw_frame_size;      /* filled in by DSCodecCreate() and DSGetCodecInfo() */
     uint16_t coded_frame_size;    /*   "    "    " */
     int payload_shift;            /* special case item, when non-zero indicates shift payload after encoding or before decoding, depending on which codec and the case. Initially needed to "unshift" EVS AMR-WB IO mode bit-shifted packets observed in-the-wild. Note shift can be +/-, JHB Sep 2022 */

     CODEC_ENC_PARAMS enc_params;  /* if encoder instance is being created, this must point to desired encoder params. See examples in x86_mediaTest.c or hello_codec.c */
     CODEC_DEC_PARAMS dec_params;  /* if decoder instance is being created, this must point to desired decoder params. See examples in x86_mediaTest.c or hello_codec.c */

  } CODEC_PARAMS;


  HCODEC DSCodecCreate(void* pCodecInfo, unsigned int uFlags);  /* by default pCodecInfo should point to a CODEC_PARAMS struct; for example usage see x86_mediaTest.c or hello_codec.c. If the DS_CC_USE_TERMINFO flag is given in uFlags, then pCodecInfo should point to a TERMINATION_INFO struct (defined in shared_include/session.h); for example usage see packet_flow_media_proc.c (packet/media thread processing) */

  void DSCodecDelete(HCODEC hCodec);

  int DSCodecEncode(HCODEC*          hCodec,        /* pointer to one or more codec handles, as specified by numChan */
                    unsigned int     uFlags,        /* flags, see DS_CE_xxx flags below */
                    uint8_t*         inData,        /* pointer to input audio data */
                    uint8_t*         outData,       /* pointer to output coded bitstream data */
                    uint32_t         in_frameSize,  /* size of input audio data, in bytes */
                    int              numChan,       /* number of channels to be encoded. Multichannel data must be interleaved */
                    CODEC_INARGS*    pInArgs,       /* optional parameters for encoding the audio frame (only supported by newer codecs) */
                    CODEC_OUTARGS*   pOutArgs);     /* optional encoding parameters. If not used this param should be NULL */

  int DSCodecDecode(HCODEC*          hCodec,        /* pointer to one or more codec handles, as specified by numChan */
                    unsigned int     uFlags,        /* flags, see DS_CD_xxx flags below */
                    uint8_t*         inData,        /* pointer to input coded bitstream data */
                    uint8_t*         outData,       /* pointer to output audio data */
                    uint32_t         in_frameSize,  /* size of coded bitstream data, in bytes */
                    int              numChan,       /* number of channels to be decoded. Multichannel data must be interleaved */
                    CODEC_OUTARGS*   pOutArgs);     /* optional decoding parameters. If not used this param should be NULL */

  int DSCodecTranscode(HCODEC*       hCodecSrc,
                       HCODEC*       hCodecDst,
                       unsigned int  uFlags,
                       uint8_t*      inData,
                       uint32_t      in_frameSize,  /* in bytes */
                       uint8_t*      outData,
                       int           numChan);

#define DS_GET_NUMFRAMES  0x100  /* if specified in uFlags, DSCodecDecode() returns the number of frames in the payload.  No decoding is performed */

/* APIs that take an hCodec returned by DSCodecCreate(). Notes:

   -return values are -1 or otherwise < 0 on error conditions
   -DSGetCodecInfo() accept flags to specify a codec type or hCodec handle input; see additional comments below
*/

/* DSGetCodecType() returns codec type, see "voice_codec_type" enums in shared_include/session.h. hCodec must be a valid codec handle generated by DSCodecCreate() */

  int DSGetCodecType(HCODEC hCodec);

/* DSGetCodecInfo() returns information for the specified codec and uFlags (see below for uFlags definitions). Notes:

   -in most cases uFlags should specify DS_CODEC_INFO_HANDLE, indicating the "codec" param will be interpreted as an hCodec. If neither DS_CODEC_INFO_HANDLE or DS_CODEC_INFO_TYPE is given, the default is DS_CODEC_INFO_HANFDLE
   -if uFlags specifies DS_CODEC_INFO_TYPE, the following flags can be used: DS_CODEC_INFO_NAME, DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE
   -returned info is copied into pInfo for following flags:  DS_CODEC_INFO_NAME, DS_CODEC_INFO_PARAMS
   -nInput1 and nInput2 are required for flags: DS_CODEC_INFO_BITRATE_TO_INDEX, DS_CODEC_INFO_INDEX_TO_BITRATE, and DS_CODEC_INFO_CODED_FRAMESIZE (the latter when combined with DS_CODEC_INFO_HANDLE)
   -nInput1 is required for the DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE flag
*/
  
  int DSGetCodecInfo(int codec, unsigned int uFlags, int nInput1, int nInput2, void* pInfo);

/* more codec helper functions, these are generic. Notes:

  -generic means not requiring an existing codec instance handle, so these functions accept a codec type
  -an existing codec instance is one already created and/or running dynamically, in which case an hCodec must be provided (see above functions)
*/

/* DSGetPayloadInfo() returns header format and other info for codec RTP payloads. Notes, JHB Oct 2022:

     -codec type should be one of the types specified in shared_include/session.h
     -payload should point to a codec RTP payload
     -payload_len should give the size (in bytes) of the RTP payload pointed to by payload
     -return value: for EVS returns 0 for CH (compact header) format, 1 for FH (full header) format, for AMR returns 0 for bandwidth-efficient format, 1 for octet-aligned format, for other codecs returns 0, for error condition returns -1
     -only for EVS, fAMRWB_IOMode is set to 1 in AMR-WB IO mode, 0 for EVS mode
     -fSID is set to 1 if the packet is a SID
*/
  int DSGetPayloadInfo(unsigned int codec_type, uint8_t* payload, unsigned int payload_len, unsigned int* fAMRWB_IOMode, unsigned int* fSID);

/* DSGetPayloadHeaderToC() returns a nominal AMR or EVS payload header ToC based on payload size. The ToC is normally one byte, but could be larger for future codecs. See the AMR and EVS specs for bit fields defined in the ToC */

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

/* DSConfigVoplib() flags */

#define DS_CV_GLOBALCONFIG                   0x01
#define DS_CV_DEBUGCONFIG                    0x02
#define DS_CV_INIT                           0x04

/* DSCodecCreate() flags */

#define DS_CODEC_CREATE_ENCODER              0x01
#define DS_CODEC_CREATE_DECODER              0x02
#define DS_CODEC_CREATE_USE_TERMINFO         0x100
#define DS_CODEC_CREATE_TRACK_MEM_USAGE      0x200

/* DSGetCodecInfo() flags */

#define DS_CODEC_INFO_HANDLE                 0x100  /* specifies the "codec" param (first param) is interpreted as an hCodec (i.e. handle created by prior call to DSCodecCreate(). This is the default if neither DS_CODEC_INFO_HANDLE or DS_CODEC_INFO_TYPE is given */ 
#define DS_CODEC_INFO_TYPE                   0x200  /* specifies the "codec" param (first param) is interpreted as a codec_type */ 

/* DSGetCodecInfo() item flags. If no item flag is given, DS_CODEC_INFO_HANDLE should be specified and pInfo is expected to point to a CODEC_PARAMS struct. Some item flags must be combined with the DS_CODEC_INFO_HANDLE flag (see per-flag comments) */

#define DS_CODEC_INFO_NAME                   0x01   /* returns codec name in text string pointed to by pInfo. Typically string length is 5-10 char, always less than CODEC_NAME_MAXLEN char */
#define DS_CODEC_INFO_RAW_FRAMESIZE          0x02   /* returns codec media frame size (i.e. prior to encode, after decode), in bytes. If DS_CODEC_INFO_HANDLE is not given, returns default media frame size for one ptime. For EVS, nInput1 should specify one of the four (4) EVS sampling rates (in Hz) */
#define DS_CODEC_INFO_CODED_FRAMESIZE        0x03   /* returns codec compressed frame size (i.e. after encode, prior to decode), in bytes. If uFlags specifies DS_CODEC_INFO_TYPE then nInput1 should give a bitrate and nInput2 should give header format (0 or 1) */
#define DS_CODEC_INFO_BITRATE                0x04   /* returns codec bitrate in bps. Requires DS_CODEC_INFO_HANDLE flag */
#define DS_CODEC_INFO_SAMPLERATE             0x05   /* returns codec sampling rate in Hz. If DS_CODEC_INFO_HANDLE is not given, returns default sample rate for the specified codec. For EVS, nInput1 can specify one of the four (4) EVS sampling rates with values 0-3 */
#define DS_CODEC_INFO_PTIME                  0x06   /* returns ptime in msec. Default value is raw framesize / sampling rate at codec creation-time, then is dynamically adjusted as packet streams are processed. Requires DS_CODEC_INFO_HANDLE flag */
#define DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE  0x07   /* given an nInput1 sample rate in Hz, returns sample rate code specified in "xxx_codec_flags" enums in shared_include/session.h, where xxx is the codec abbreviation (e.g. evs_codec_flags or melpe_codec_flags) */
#define DS_CODEC_INFO_BITRATE_TO_INDEX       0x08   /* converts an EVS or AMR bitrate (nInput1) to an index 1-25 */
#define DS_CODEC_INFO_INDEX_TO_BITRATE       0x09   /* inverse */
#define DS_CODEC_INFO_PAYLOAD_SHIFT          0x0a   /* returns payload shift specified in CODEC_PARAMS or TERMINATION_INFO structs at codec creation time, if any. Requires DS_CODEC_INFO_HANDLE flag. Default value is zero */

#define DS_CODEC_INFO_BITRATE_CODE           0x400  /* when combined with DS_CODEC_INFO_CODED_FRAMESIZE, indicates nInput1 should be treated as a "bitrate code" instead of a bitrate. A bitrate code is typically found in the RTP payload header, for example a 4 bit field specifying 16 possible bitrates. Currently only EVS and AMR codecs support this flag, according to Table A.4 and A.5 in section A.2.2.1.2, "ToC byte" of EVS spec TS 26.445. See mediaTest source code for usage examples */

#define DS_CODEC_INFO_SIZE_BITS              0x800  /* indicates DS_CODEC_INFO_CODED_FRAMESIZE return value should be in size of bits (instead of bytes) */

#define DS_CODEC_INFO_ITEM_MASK              0xff

#endif  /* _VOPLIB_H_ */
