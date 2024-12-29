# SigSRF Codecs Documentation

<a name="TOC"></a>
## Table of Contents

[**_Overview_**](#user-content-overview)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;[**Codec Software Architecture Diagram**](#user-content-codecsoftwarearchitecturediagram)<br/>
</sup></sub>

[**_I/O Support_**](#user-content-iosupport)<br/>

[**_API Interface_**](#user-content-apiinterface)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;[**DSCodecCreate**](#user-content-dscodeccreate)<br/>
&nbsp;&nbsp;&nbsp;[**DSCodecDecode**](#user-content-dscodecdecode)<br/>
&nbsp;&nbsp;&nbsp;[**DSCodecEncode**](#user-content-dscodecencode)<br/>
&nbsp;&nbsp;&nbsp;[**DSGetCodecInfo**](#user-content-dsgetcodecinfo)<br/>
&nbsp;&nbsp;&nbsp;[**DSGetPayloadInfo**](#user-content-dsgetpayloadinfo)<br/>
&nbsp;&nbsp;&nbsp;[**DSGetPayloadHeaderToC**](#user-content-dsgetpayloadheadertoc)<br/>
&nbsp;&nbsp;&nbsp;[**DSCodecDelete**](#user-content-dscodecdelete)<br/>
</sup></sub>

&nbsp;&nbsp;&nbsp;[**Structs**](#user-content-structs)<br/>
<sub><sup>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**CODEC_PARAMS**](#user-content-codecparams)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**CODEC_DEC_PARAMS**](#user-content-codecdecparams)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**CODEC_ENC_PARAMS**](#user-content-codecencparams)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**CODEC_OUTARGS**](#user-content-codecoutargs)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**CODEC_INARGS**](#user-content-codecinargs)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**PAYLOAD_INFO**](#user-content-payloadinfo)<br/>
</sup></sub>

[**_hello_codec Example App_**](#user-content-hellocodecexampleapp)<br/>

[**_Test, Measurement, and Interoperation_**](#user-content-testmeasurementandinteroperation)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;[**_Codec Regression Test Scripts_**](#user-content-codecregressiontestscripts)<br/>
&nbsp;&nbsp;&nbsp;[**_EVS Regression Test Script_**](#user-content-evsregressiontestscript)<br/>
&nbsp;&nbsp;&nbsp;[**_AMR NB / WB Test Script_**](#user-content-amrregressiontestscript)<br/>
</sup></sub>

<a name="Overview"></a>
# Overview

Codecs are an essential component in streaming media use cases, necessary to decode RTP media in telecom, analytics, robotics, and many other applications. In addition, codecs are often necessary to encode media, for example transcoding in SBC, language translation, home and mobile assistants, and other applications. This page documents SigSRF codecs available in the mediaMin, mediaTest, and hello_codec reference applications, including EVS, AMR, AMR-WB, G711, and more. As part of the SigSRF SDK, codecs may also be incorporated into user applications for test and evaluation purposes. Documentation is organized as:

  * test, measurement, and interoperation [:link:](#user-content-testmeasurementandinteroperation)
  * I/O support, including audio waveform files, pcap, and RTP [:link:](#user-content-iosupport)
  * API interface [:link:](#user-content-apiinterface)
  * "hello codec" example user application [:link:](#user-content-hellocodecexampleapp)

SigSRF codecs are available in demo format as [Docker containers](https://www.github.com/signalogic/SigSRF_SDK/tree/master?tab=readme-ov-file#user-content-dockercontainers) and [installable Rar packages](https://www.github.com/signalogic/SigSRF_SDK/tree/master?tab=readme-ov-file#user-content-rarpackages), and in production / deployment format as a codec license, or included in mediaMin and mediaTest licenses. Demo licenses are limited in total time and frame duration; after some point they continue to operate normally but produce "noisy media".

Below is a software architecture diagram showing the relationship between user applications, voplib, and codec libraries. 

<a name="CodecSoftwareArchitectureDiagram"></a>
## Codec Software Architecture Diagram

<p>&nbsp;</p>

![SigSRF codecs software architecture diagram](https://github.com/signalogic/SigSRF_SDK/blob/master/images/SigSRF_codecs_software_architecture_diagram.png?raw=true "SigSRF codecs software architecture diagram")

<p>&nbsp;</p>

[mediaTest](https://www.github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#user-content-mediatest) is a tool for codec measurement and interoperation testing (performance, audio quality measurement, reference vector comparison, debug, etc). [mediaMin](https://www.github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#user-content-mediamin) is a production / deployment grade application (telecom, analytics, call recording, lawful interception, robotics, etc). mediaMin can use RTP auto-detection, SDP packet info, or both to determine codec type and [create dynamic sessions](https://www.github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#user-content-dynamicsessioncreation). [hello_codec](#user-content-hellocodecexampleapp) is a simple codec example application. 


Notes about [voplib](https://www.github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#user-content-voplib) (voice/video-over-packet library):

* voplib provides a unified, documented interface to all codecs, and manages all memory allocation per the [XDAIS standard](https://en.wikipedia.org/wiki/XDAIS_algorithms). voplib abstracts codec architecture variation, for example codecs may have different numbers of shared object (.so) files, depending on how their standards body source code is organized, some support on-the-fly commands, the way errors are handled varies, etc. Also, voplib supports high capacity, "stand alone", diagnostic, and other specialized builds for application specific purposes

* voplib supports two types of struct interfaces in [DSCodecCreate()](#user-content-dscodeccreate), CODEC_PARAMS and TERMINATION_INFO. See [API Interface](#user-content-apiinterface) below for more information

Some additional notes about the above diagram:

* the dashed line indicates high-level pktlib APIs such as DSPushPackets() and DSPullPackets() available to user applications. For example pktlib API usage see the [mediaMin Minimum API Interface](https://www.github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#minimumapiinterface) and look for PushPackets() and PullPackets() in [mediaMin.cpp source code](https://www.github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaMin/mediaMin.cpp)

* for data flow diagrams, see [telecom and analytics mode data flow diagrams](https://www.github.com/signalogic/SigSRF_SDK#user-content-telecommodedataflowdiagram) on the main SigSRF SDK page

<a name="IOSupport"></a>
# I/O Support

The following RTP input types are supported:

| Input | mediaMin | mediaTest | Demo Versions | Comments |
|-----|-----|-----|-----|-----|
| UDP | Y | N | N |
| pcap | Y | Y | Y |
| pcapng | Y | Y | Y |
| rtp | Y | Y | Y | same as .rtpdump |

The following media format file types are supported:

| Input | mediaMin | mediaTest | Demo Versions | Comments |
|-----|-----|-----|-----|-----|
| wav | N | Y | Y |
| au | N | Y | Y |
| tim | N | Y | Y | Hypersignal waveform file format |
| raw | N | Y | Y | no header, a codec configuration file can be used instead |

The following coded file types are supported:

| Input | mediaMin | mediaTest | Demo Versions | Comments |
|-----|-----|-----|-----|-----|
| cod | N | Y | Y | raw coded bitstream, no header |
| amr | N | Y | Y | AMR-NB bitstream file |
| awb | N | Y | Y | AMR-WB bitstream file |
| mime | N | Y | Y | 3GPP bitstream file |

<a name="APIInterface"></a>
# API Interface

The following APIs and structs are defined in [voplib.h](https://www.github.com/signalogic/SigSRF_SDK/blob/master/includes/voplib.h).

<a name="DSCodecCreate"></a>
## DSCodecCreate

  * creates an encoder or decoder instance with parameters specified in a struct of type specified by uFlags and pointed to by pCodecInfo
  * if pCodecInfo points to a CODEC_PARAMS struct, then either a CODEC_ENC_PARAMS or CODEC_DEC_PARAMS struct (or both) should be included in CODEC_PARAMS
  * returns an HCODEC (codec instance handle) > 0, 0 if no handle was created, and < 0 for an error condition

```c++
    HCODEC DSCodecCreate(void* pCodecInfo,            /* pointer to CODEC_PARAMS struct or TERMINATION_INFO struct - see comments below */
                         unsigned int uFlags          /* one or more DS_CODEC_CREATE_xxx flags, or general flags shown below */
                        );
```
     uFlags definitions
```c++
    #define DS_CODEC_CREATE_ENCODER                   /* create an encoder instance - may be combined with DS_CODEC_CREATE_DECODER */
    #define DS_CODEC_CREATE_DECODER                   /* create a decoder instance - may be combined with DS_CODCEC_CREATE_ENCODER */
    #define DS_CODEC_CREATE_USE_TERMINFO              /* pCodecInfo points to a TERMINATION_INFO struct. The default (no flag) is a CODEC_PARAMS struct */
```

For direct or "codec only" usage, pCodecInfo should point to a CODEC_PARAMS struct; for example usage see [mediaTest_proc.c](https://www.github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/mediaTest_proc.c) or [hello_codec.c](https://www.github.com/signalogic/SigSRF_SDK/blob/master/apps/hello_codec/hello_codec.c). For packet based applications (indirect codec usage), if the DS_CODEC_CREATE_USE_TERMINFO flag is given in uFlags, then pCodecInfo should point to a TERMINATION_INFO struct (defined in [shared_include/session.h](https://www.github.com/signalogic/SigSRF_SDK/blob/master/shared_includes/session.h)); for example usage see [packet_flow_media_proc.c (packet/media thread processing)](https://www.github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/packet_flow_media_proc.c)
  
<a name="DSCodecDecode"></a>
## DSCodecDecode

  * decodes one or more frames using one or more decoder instance handles
  * returns length of decoded media frame(s) (in bytes)

```c++
    int DSCodecDecode(HCODEC*          hCodec,        /* pointer to one or more codec instance handles, as specified by numChan */
                      unsigned int     uFlags,        /* see DS_CODEC_DECODE_xxx flags below */
                      uint8_t*         inData,        /* pointer to input coded bitstream data. Input may include a payload header and CMR value, if supported by the codec type and header/payload format */
                      uint8_t*         outData,       /* pointer to output audio frame data */
                      uint32_t         in_frameSize,  /* size of coded bitstream data, in bytes */
                      int              numChan,       /* number of channels to be decoded. Multichannel data must be interleaved */
                      CODEC_INARGS*    pInArgs,       /* optional parameters for decoding RTP payloads; see CODEC_INARGS struct notes below. If not used this param should be NULL */
                      CODEC_OUTARGS*   pOutArgs       /* optional decoder output info; see CODEC_OUTARGS struct notes below. If not used this param should be NULL */
                     );
```

   uFlags definitions

```c++
    #define DS_CODEC_DECODE_GET_NUMFRAMES             /* return the number of frames in the payload. No decoding is performed */
```
<a name="DSCodecEncode"></a>
## DSCodecEncode

  * encodes one or more frames using one or more encoder instance handles
  * returns length of encoded bitstream frame(s) (in bytes)

```c++
    int DSCodecEncode(HCODEC*          hCodec,        /* pointer to one or more codec instance handles, as specified by numChan */
                      unsigned int     uFlags,        /* see DS_CODEC_ENCODE_xxx flags below */
                      uint8_t*         inData,        /* pointer to input audio frame data */
                      uint8_t*         outData,       /* pointer to output coded bitstream data. Output may include a payload header and CMR value, if supported by the codec type and header/payload format */
                      uint32_t         in_frameSize,  /* size of input audio frame data, in bytes */
                      int              numChan,       /* number of channels to be encoded. Multichannel data must be interleaved */
                      CODEC_INARGS*    pInArgs,       /* optional parameters for encoding audio data; see CODEC_INARGS struct notes below. If not used this param should be NULL */
                      CODEC_OUTARGS*   pOutArgs       /* optional encoder output info; see CODEC_OUTARGS struct notes below. If not used this param should be NULL */
                     );
```
  uFlags definitions 

    None

<a name="DSGetCodecInfo"></a>
## DSGetCodecInfo

  * returns and/or retrieves into a pInfo pointer information for the specified codec
  * return value is > 0 for success, 0 if no information is available for the given uFlags, and < 0 for error conditions
  
```c++
    int DSGetCodecInfo(int codec_param,               /* codec_param can be either a codec instance handle (HCODEC) or a codec type (a DS_CODEC_xxx enum defined in shared_include/codec.h), depending on uFlags.  If DS_CODEC_INFO_HANDLE is given in uFlags then codec_param is interpreted as an hCodec, returned by a previous call to DSCodecCreate(). If both DS_CODEC_INFO_HANDLE and DS_CODEC_INFO_TYPE flags are given, codec_param is interpreted as an hCodec and the return value is a codec type. If neither are given, DS_CODEC_INFO_TYPE is assumed as the default and codec_param is interpreted as a codec type. For examples of DS_CODEC_INFO_TYPE and DS_CODEC_INFO_HANDLE usage, see packet_flow_media_proc.c and mediaTest_proc.c */
                       unsigned int uFlags,           /* if uFlags specifies DS_CODEC_INFO_TYPE, codec_param should be a DS_CODEC_XXX enum defined in shared_include/codec.h, and uFlags can also contain DS_CODEC_INFO_NAME, DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE, or DS_CODEC_INFO_PARAMS. If uFlags specifies DS_CODEC_INFO_TYPE_FROM_NAME, pInfo should contain a standard codec name string. Standardized SigSRF codec names are given in shared_include/codec.h
 */
                       int nInput1,                   /* nInput1 is required for uFlags DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE, DS_CODEC_INFO_CMR_BITRATE, or DS_CODEC_INFO_LIST_TO_CLASSIFICATION. Both nInput1 and nInput2 are required for uFlags DS_CODEC_INFO_BITRATE_TO_INDEX, DS_CODEC_INFO_INDEX_TO_BITRATE, or DS_CODEC_INFO_CODED_FRAMESIZE */
                       int nInput2,
                       void* pInfo                    /* retrieved info is copied into pInfo for uFlags DS_CODEC_INFO_NAME, DS_CODEC_INFO_PARAMS, or DS_CODEC_INFO_TYPE_FROM_NAME */
                      );
```
   uFlags definitions

```c++
    #define DS_CODEC_INFO_HANDLE                      /* codec_param is interpreted as an hCodec; i.e. codec instance handle created by prior call to DSCodecCreate() */ 
    #define DS_CODEC_INFO_TYPE                        /* codec_param is interpreted as a codec_type. This is the default if neither DS_CODEC_INFO_HANDLE nor DS_CODEC_INFO_TYPE is given. If both DS_CODEC_INFO_HANDLE and DS_CODEC_INFO_TYPE are given a codec type is returned */ 
```
  Item Flags
  
  * if no item flag is given, DS_CODEC_INFO_HANDLE should be specified and pInfo should point to a CODEC_PARAMS struct
  * some item flags require the DS_CODEC_INFO_HANDLE flag (see per-flag comments)

```c++
    #define DS_CODEC_INFO_NAME                        /* returns codec name in text string pointed to by pInfo. A typical string length is 5-10 char, and always less than CODEC_NAME_MAXLEN */
    #define DS_CODEC_INFO_RAW_FRAMESIZE               /* returns codec media frame size (i.e. prior to encode, after decode), in bytes. If DS_CODEC_INFO_HANDLE is not given, returns default media frame size for one ptime. For EVS, nInput1 should specify one of the four (4) EVS sampling rates (in Hz) */
    #define DS_CODEC_INFO_CODED_FRAMESIZE             /* returns codec compressed frame size (i.e. after encode, prior to decode), in bytes. If uFlags specifies DS_CODEC_INFO_TYPE then nInput1 should give a bitrate and nInput2 should give payload format (see DS_PYLD_FMT_XXX definitions below) */
    #define DS_CODEC_INFO_BITRATE                     /* returns codec bitrate in bps. Requires DS_CODEC_INFO_HANDLE flag */
    #define DS_CODEC_INFO_SAMPLERATE                  /* returns codec sampling rate in Hz. If DS_CODEC_INFO_HANDLE is not given, returns default sample rate for the specified codec. For EVS, nInput1 can specify one of the four (4) EVS sampling rates with values 0-3 */
    #define DS_CODEC_INFO_PTIME                       /* returns ptime in msec. Default value is raw framesize / sampling rate at codec creation-time, then is dynamically adjusted as packet streams are processed. Requires DS_CODEC_INFO_HANDLE flag */
    #define DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE       /* given an nInput1 sample rate in Hz, returns sample rate code specified in "xxx_codec_flags" enums in shared_include/codec.h, where xxx is the codec abbreviation (e.g. evs_codec_flags or melpe_codec_flags) */
    #define DS_CODEC_INFO_BITRATE_TO_INDEX            /* converts a codec bitrate (nInput1) to an index 0-31 (currently only EVS and AMR codecs supported) */
    #define DS_CODEC_INFO_INDEX_TO_BITRATE            /* inverse */
    #define DS_CODEC_INFO_PAYLOAD_SHIFT               /* returns payload shift specified in CODEC_PARAMS or TERMINATION_INFO structs at codec creation time, if any. Requires DS_CODEC_INFO_HANDLE flag. Default value is zero */
    #define DS_CODEC_INFO_LIST_TO_CLASSIFICATION      /* converts a codec audio classification list item (nInput1) to 0-3 letter classification string (currently EVS and AMR codecs supported) */
    #define DS_CODEC_INFO_TYPE_FROM_NAME              /* returns a codec_type if pInfo contains a standard codec name. codec_param is ignored. Standardized codec names used in all SigSRF libs and applications are in get_codec_type_from_name() in shared_include/codec.h. Returns -1 if pInfo is NULL or codec name is not found */
    #define DS_CODEC_INFO_CMR_BITRATE                 /* returns requested bitrate from CMR (Change Mode Request) value given in nInput1. Applicable only to codecs that use CMR */

    #define DS_CODEC_INFO_BITRATE_CODE                /* when combined with DS_CODEC_INFO_CODED_FRAMESIZE, indicates nInput1 should be treated as a "bitrate code" instead of a bitrate. A bitrate code is typically found in the RTP payload header, for example a 4 bit field specifying 16 possible bitrates. Currently only EVS and AMR codecs support this flag, according to Table A.4 and A.5 in section A.2.2.1.2, "ToC byte" of EVS spec TS 26.445. See mediaTest source code for usage examples */

    #define DS_CODEC_INFO_SIZE_BITS                   /* indicates DS_CODEC_INFO_CODED_FRAMESIZE return value should be in size of bits (instead of bytes) */

    #define DS_CODEC_INFO_SUPPRESS_WARNING_MSG        /* suppress DSGetCodecInfo() warning messages */

    #define DS_CODEC_INFO_ITEM_MASK                   /* the item mask can be used to AND uFlags and extract an item value */
```
<a name="DSGetPayloadInfo"></a>
## DSGetPayloadInfo

  * returns information about an RTP payload
  * return value is (i) a DS_PYLD_FMT_XXX payload format definition for applicable codecs (e.g. AMR, EVS), (ii) 0 for other codec types, or (iii) < 0 for error conditions. See [Payload Format Definitions](#user-content-payloadformatdefinitions) below

DSGetPayloadInfo() is a crucial SigSRF API, used by voplib internally in DSCodecDecode() and also by reference apps mediaTest and mediaMin. A full RTP payload parsing and inspection mode as well as generic and "lightweight" modes are supported

```c++
    int DSGetPayloadInfo(int codec_param,             /* codec_param can be either a codec instance handle (HCODEC) or a codec type (a DS_CODEC_xxx enum defined in shared_include/codec.h), depending on uFlags. If DS_CODEC_INFO_HANDLE is given in uFlags then codec_param is interpreted as an hCodec, returned by a previous call to DSCodecCreate(). If both DS_CODEC_INFO_HANDLE and DS_CODEC_INFO_TYPE flags are given, codec_param is interpreted as an hCodec and the return value is a codec type. If neither are given, DS_CODEC_INFO_TYPE is assumed as the default and codec_param is interpreted as a codec type. For examples of DS_CODEC_INFO_TYPE and DS_CODEC_INFO_HANDLE usage, see packet_flow_media_proc.c and mediaTest_proc.c */
                         unsigned int uFlags,         /* if uFlags specifies DS_CODEC_INFO_TYPE, codec_param should be a DS_CODEC_XXX enum defined in shared_include/codec.h */
                         uint8_t* payload,            /* payload should point to an RTP payload in an IPv4 or IPv6 UDP packet */
                         int payload_size,            /* size of the RTP payload, in bytes */
                         PAYLOAD_INFO* payload_info   /* payload_info should point to a PAYLOAD_INFO struct or be NULL if not used. If payload_info is non-NULL then the following payload_info items are set or cleared:

                                                         -CMR is set to the payload change mode request value if present and applicable to the codec type, or set to zero if not
                                                         -ToC[] is set to the payload header "table of contents" value for each frame in the payload if applicable to the codec type, or set to zero if not 
                                                         -FrameSize[] is set to the size of each frame in the payload if applicable to the codec type, or set to zero if not 
                                                         -BitRate[] is set to the codec bitrate corresponding to the frame size
                                                         -uFormat is a copy of the return value, excluding error conditions
                                                         -fSID is set if the payload is a SID (silence identifier), or cleared if not
                                                         -fDTMF is set if the payload is a DTMF event, or cleared if not
                                                         -only applicable to EVS, fAMRWB_IO_MOde is set true for an AMR-WB IO mode payload, false for a primary mode payload, and false for all other codec types */
                        );
```

uFlags definitions

```c++
    #define DS_PAYLOAD_INFO_SID_ONLY                  /* if DS_PAYLOAD_INFO_SID_ONLY is given in uFlags DSGetPayloadInfo() will make a quick check for a SID payload. codec_param should be a valid DS_CODEC_xxx enum (defined in shared_include/codec.h), uFlags should include DS_CODEC_INFO_TYPE, no error checking is performed, and fSID in payload_info will be set or cleared. If the payload contains multiple frames only the first frame is considered. Return values are a DS_PYLD_FMT_XXX value for a SID payload and -1 for not a SID payload */

    #define DS_PAYLOAD_INFO_NO_CODEC                  /* if DS_PAYLOAD_INFO_NO_CODEC is given in uFlags DSGetPayloadInfo() will ignore codec_param and payload and set fDTMF (DTMF event) in payload_info if payload_size = 4 or set fSID (SID payload) in payload_info if payload_size <= 8. This is reliable for most codecs for single-frame payloads; however, for multiple frames (e.g. variable ptime, multiple channels, etc) this flag should not be used. In that case -- or any other situation where detailed payload information is needed (e.g. payload format or operating mode) -- valid codec_param and payload params should be given without this flag. fDTMF and fSID in payload_info are set or cleared. Return values are 0 for a SID payload and -1 for not a SID payload */

    #define DS_PAYLOAD_INFO_IGNORE_DTMF               /* DSGetPayloadInfo() default behavior is to recognize payload_size == 4 as a DTMF event (RFC 4733), set NumFrames to 1 and set fDTMF in payload_info, and immediately return 0 without reference to codec_param and payload. To override this behavior DS_PAYLOAD_INFO_IGNORE_DTMF can be given in uFlags */ 

    #define DS_PAYLOAD_INFO_SUPPRESS_WARNING_MSG      /* suppress DSGetPayloadInfo() warning messages. Examples of this usage are in mediaMin.cpp */
```

<a name="DSGetPayloadHeaderToC"></a>
## DSGetPayloadHeaderToC

  * returns a nominal AMR or EVS payload header ToC based on payload size. For EVS, this API should be called *only* with non-collision ("protected") payload sizes. The returned ToC is normally one byte, including a 4-bit FT (frame type) field, so it's returned in the low byte of the return value, but could be larger for other codecs if supported. See the AMR and EVS specs for ToC bit fields definitions
  * return value is < 0 for error conditions

```c++
    int DSGetPayloadHeaderToC(int codec_type,         /* a DS_CODEC_xxx enum defined in shared_include/codec.h */
                              uint8_t* payload,       /* payload should point to an RTP payload in an IPv4 or IPv6 UDP packet. For EVS payloads payload is only needed for "special case" payloads (see spec section A.2.1.3, Special case for 56 bit payload size (EVS Primary or EVS AMR-WB IO SID), otherwise it can be given as NULL. For AMR payloads, DSGetPayloadHeaderToC() calls DSGetPayloadInfo() internally as information in the payload is required to determine its format (bandwidth-efficient vs octet-aligned), and in turn its ToC */
                              int payload_size,       /* size of the RTP payload, in bytes */
                             );
```

<a name="DSCodecDelete"></a>
## DSCodecDelete

  * deletes an encoder or decoder instance
  * returns > 0 on success, or < 0 on error condition

```c++
    int DSCodecDelete(HCODEC hCodec,                  /* handle of encoder or decoder instance to be deleted */
                      unsigned int uFlags             /* flags - currently only the DS_CODEC_TRACK_MEM_USAGE flag is recognized */
                     );
```
General flags, applicable in multiple APIs

```c++
    #define DS_CODEC_TRACK_MEM_USAGE                  /* track instance memory usage */
    #define DS_CODEC_USE_EVENT_LOG                    /* Use the SigSRF diaglib event log for progress, debug, and error messages. By default codec event and error logging is handled according to uEventLogMode element of a DEBUG_CONFIG struct specified in DSConfigVoplib(). uEventLogMode should be set with EVENT_LOG_MODE enums in shared_include/config.h. This flag may be combined with uFlags in DSCodecCreate() and/or uFlags in CODEC_ENC_PARAMS and CODEC_DEC_PARAMS structs to override uEventLogMode */
```
<a name="PayloadFormatDefinitions"></a>
## Payload Format Definitions

Payload format definitions (currently applicable only to EVS and AMR codec formats). These definitions can be used in the CODEC_ENC_PARAMS struct format or oct_align fields, and DSGetPayloadInfo() returns a payload format definition.

```c++
    #define DS_PYLD_FMT_COMPACT                       /* compact format (coded media frame has no payload header) */
    #define DS_PYLD_HDR_FULL                          /* header-full format (coded media frames may or may not have a payload header, differentiated by collision avoidance padding */
    #define DS_PYLD_FMT_HF_ONLY                       /* header-full only (coded media frames always have a payload header, with no collision avoidance padding */
    #define DS_PYLD_FMT_BANDWIDTHEFFICIENT            /* bandwidth-efficient format */
    #define DS_PYLD_FMT_OCTETALIGN                    /* octet-aligned format */
```

<a name="Structs"></a>
## Structs

<a name="CodecParams"></a>
### CODEC_PARAMS

Struct used in DSCodecCreate() and DSGetCodecInfo()

```c++
  typedef struct {

     int codec_type;                      /* specifies codec type -- see "voice_codec_type" and "video_codec_type" enums in shared_include/codec.h */
     char codec_name[CODEC_NAME_MAXLEN];  /* pointer to codec name string that will be filled in. Note this is the same string as returned by DSGetCodecInfo() with DS_CODEC_INFO_NAME flag */
     uint16_t raw_frame_size;             /* filled in by DSCodecCreate() and DSGetCodecInfo() */
     uint16_t coded_frame_size;           /*   "    "    " */
     int payload_shift;                   /* special case item, when non-zero indicates shift payload after encoding or before decoding, depending on which codec and the case. Initially needed to "unshift" EVS AMR-WB IO mode bit-shifted packets observed in-the-wild. Note shift can be +/- */

     CODEC_ENC_PARAMS enc_params;         /* if encoder instance is being created, this must point to desired encoder params. See examples in mediaTest_proc.c or hello_codec.c */
     CODEC_DEC_PARAMS dec_params;         /* if decoder instance is being created, this must point to desired decoder params. See examples in mediaTest_proc.c or hello_codec.c */

  } CODEC_PARAMS;
```

<a name="CodecDecParams"></a>
### CODEC_DEC_PARAMS

Decode parameters struct, a substruct within CODEC_PARAMS and CODEC_INARGS

```c++
  typedef struct {

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
```

<a name="CodecEncParams"></a>
### CODEC_ENC_PARAMS

Encode parameters struct, a substruct within CODEC_PARAMS and CODEC_INARGS

```c++
  typedef struct  {

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
      int oct_align;          /* AMR terminology */
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
```

Audio classification frame types returned in CODEC_OUTARGS frameType

```c++
  enum audio_classification_frametype {

/* classification type items */

    FRAMETYPE_VOICED,              /* speech, voiced */
    FRAMETYPE_UNVOICED,            /* speech, unvoiced */
    FRAMETYPE_SID,                 /* SID (silence / comfort noise) frames for codecs that support DTX */
    FRAMETYPE_NODATA,              /* untransmitted frame for codecs that support DTX */
    FRAMETYPE_NOISE,               /* background noise for codecs that support audio classification */
    FRAMETYPE_AUDIO,               /* sounds and other audio for codecs that support audio classification */
    FRAMETYPE_MUSIC,               /* music for codecs that support audio classification */

/* flags that may be combined with above type items */

    FRAMETYPE_TRANSITION = 0x100,  /* transition between types */
    FRAMETYPE_MELP = 0x200,        /* low bitrate voiced (mixed excited linear prediction) */
    FRAMETYPE_NELP = 0x400,        /* low bitrate voiced (noise excited linear prediction) */

    FRAMETYPE_ITEM_MASK = 0xff     /* mask to separate type items from flags */
  };
 ```

<a name="CodecOutArgs"></a>
### CODEC_OUTARGS

Optional struct for output from DSCodecEncode() and DSCodecDecode()

  * pOutArgs in DSCodecEncode() or DSCodecDecode() should point to a CODEC_OUTARGS struct, otherwise be given as NULL

```c++
  typedef struct {

   short int size;       /* generic size field, used differently by codecs */
   short int frameType;  /* audio content frame type classified by the encoder, if supported by the codec type. Possible types are enumerated in audio_classification_frametype */
   int extendedError;

   uint8_t CMR;          /* for DSCodecDecode(), CMR (Codec Mode Request) will reflect the value in the input bitstream, if supported by the codec type. Notes:

                            -for AMR codecs CMR examples include 0xf0 (no mode request), 0x20 (AMR-WB 12.65 bps), 0x70 (AMR-NB 1.20 kbps), etc. If the bitstream CMR is "no mode request" (the default value), CMR will be 0xf0
                            -for EVS codecs CMR will be non-zero only if present in the input bitstream. Examples include 0x80 (CMR = 0), 0xa4 (CMR = 0x24), 0x92 (CMR = 0x12), etc. CMR will be non-zero if the input bitstream is (a) in headerfull format and includes a CMR byte or (b) in AMR-WB IO mode compact format
                            -received CMR values are not shifted in any way. For octet aligned and headerfull formats, CMR contains the whole byte as received (including H bit or R bits as applicable). For bandwidth efficent and compact formats, CMR contains the partial 4 or 3 bits, in the exact position they were received, with other CMR bits zero
                         */

   int bitRate;          /* for DSCodecDecode(), bitrate detected by the decoder (in bps) from the input bitstream, if supported by the codec type */

  } CODEC_OUTARGS;
```

<a name="CodecOutArgs"></a>
### CODEC_INARGS

Optional struct for input to DSCodecEncode() and DSCodecDecode()

  * pInArgs in DSCodecEncode() or DSCodecDecode() should point to a CODEC_INARGS struct, otherwise be given as NULL

```c++
  typedef struct {

   uint8_t CMR;          /* for DSCodecEncode(), this is the CMR (Codec Mode Request) in the encoder output bitstream frame. Notes:

                            -SigSRF encoders generate a CMR if mandated by the spec. For example, encoders generate a CMR for all AMR frames and for EVS AMR-WB IO mode SID frames (as required by spec section A.2.2.1.1). In these cases a CMR specified here will override the one generated
                            -for AMR codecs if "no mode request" should be inserted, then specify 0xf0. When pInArgs is NULL, the default CMR value is 0xf0
                            -for EVS codecs using headerfull format, if pInArgs is non-NULL then zero CMR values are ignored. Examples of valid CMR values include 0x80 (CMR = 0), 0xa4 (CMR = 0x24), 0x92 (CMR = 0x12), etc. Note the CMR value msb should be set in order to comply with spec section A.2.2.1.1 (the "H" bit). When pInArgs is NULL, or when compact format is in use, CMR is ignored
                            -for EVS codecs using AMR-WB IO mode in compact format valid values of CMR include 0 (6.6 kbps), 0xc0 (23.85 kbps), 0xe0 (no mode request), etc.
                            -CMR should not be shifted in any way. For octet aligned and headerfull formats, CMR should give the whole byte to insert in the output frame (including H bit or R bits as applicable). For bandwidth efficent and compact formats, CMR should give the partial 4 or 3 bits, in the exact position within a payload byte as shown in the codec spec, with the rest of CMR zero'ed

                            for DSCodecDecode(), this is the CMR in the decoder input bitstream frame. Notes:

                            -zero values are ignored
                            -normally SigSRF decoders expect CMR in frame input. If one is specified here, then it's inserted at the start of the frame and processed without any additional assumptions. This implies that if the frame already has a CMR, the calling application should remove it
                         */

   CODEC_ENC_PARAMS* pCodecEncParams;  /* to change bitRate or codec-specific parameters within the duration of an encoder instance, specify a CODEC_ENC_PARAMS struct. Note this only applies to newer, advanced codecs such as EVS and Opus */

   CODEC_DEC_PARAMS* pCodecDecParams;  /* to change output sampling rate or codec-specific parameters within the duration of a decoder instance, specify a CODEC_DEC_PARAMS struct. Note this only applies to newer, advanced codecs such as EVS and Opus */

  } CODEC_INARGS;
```
Definitions for uFlags in CODEC_ENC_PARAMS and CODEC_DEC_PARAMS

```c++
  #define RTP_FORMAT_ENCODER_NO_AMRWBIO_PADDING_BYTES      
  #define RTP_FORMAT_ENCODER_NO_VBR_PADDING_BYTES          
  #define RTP_FORMAT_DECODER_IGNORE_AMRWBIO_PADDING_BYTES  
  #define RTP_FORMAT_DECODER_IGNORE_VBR_PADDING_BYTES      
  #define RTP_FORMAT_ENCODER_FORCE_CMR                                /* force CMR to be inserted at start of output (value of 0xff "NO_REQ" is used). Intended for test/debug purposes */

  #define DEBUG_OUTPUT_VOPLIB_ONTHEFLY_UPDATES                        /* show on-the-fly updates at voplib level */
  #define DEBUG_OUTPUT_CODEC_LIB_ONTHEFLY_UPDATES                     /* show on-the-fly updates at encoder or decoder lib level */

  #define DEBUG_OUTPUT_VOPLIB_PADDING_BYTE_APPEND                     /* show encoder padding bytes when appended */
  #define DEBUG_OUTPUT_VOPLIB_SHOW_BITSTREAM_BYTES                    /* show input bitstream bytes on entry to DSCodecDecode(), or output bitstream bytes on exit from DSCodecEncode(). Displayed values are compressed bitstream bytes in hex format */
  #define DEBUG_OUTPUT_VOPLIB_SHOW_INTERNAL_INFO                      /* show decoder or encoder internal info, once for each framesize. Internal info includes CMR, I/O mode, header and/or payload format, framesize, and first payload byte */
  #define DEBUG_OUTPUT_SHOW_INIT_PARAMS                               /* show encoder and/or decoder init params when instance is created. Note this flag is only active during DSCodecCreate() */

  #define DEBUG_TEST_ABORT_EXIT_INTERCEPTION                          /* test abort() and exit() function interception at encoder or decoder lib level. Interception prevents abort() or exit() from terminating the libary and calling application, and allows code flow to continue. The test will simulate one abort() and one exit() interception, and display an example event log error message. Note that this only applies to codecs with embedded abort() and exit() functions that cannot -- or are not allowed to -- be removed, for example due to (i) binary implementation, (ii) possible license violation if source code is modified */

  #define DEBUG_OUTPUT_ADD_TO_EVENT_LOG                               /* add debug output to event log */
```
<a name="PayloadInfo"></a>
### PAYLOAD_INFO

A PAYLOAD_INFO struct is returned by DSGetPayloadInfo().

```c++
  typedef struct {  /* codec RTP payload items extracted or derived from a combination of codec type, payload header, and payload size */

  /* payload header items */

     uint8_t   CMR;                            /* change mode request value, if found in payload and applicable to codec type, zero otherwise. Not shifted or otherwise modified */
     int16_t   NumFrames;                      /* number of frames in payload. On error set to number of valid payload frames processed before the error occurred */
     uint8_t   ToC[MAX_PAYLOAD_FRAMES];        /* payload header ToC (table of contents) if applicable to codec type, including EVS and AMR, which can have multiple frames per payload (e.g. variable ptime), or multiple channels per payload (e.g. stereo or independent mono channels), or a mix. Zero otherwise */
     int16_t   FrameSize[MAX_PAYLOAD_FRAMES];  /* frame size in bytes for all codec types except AMR. For AMR (NB, WB, WB+) codecs this is frame size in bits (i.e. number of data bits in the frame, as shown at https://www.ietf.org/proceedings/50/slides/avt-8/sld009.htm). -1 indicates an error condition in payload ToC */
     int32_t   BitRate[MAX_PAYLOAD_FRAMES];    /* bitrate */
     uint16_t  NALU_Hdr;                       /* H.26x NALU header */

  /* payload types or operating modes */

     uint8_t   uFormat;                        /* set to a DS_PYLD_FMT_XXX payload format definition for applicable codecs (e.g. AMR, EVS), 0 otherwise */
     bool      fSID;                           /* true for a SID payload, false otherwise */
     bool      fAMRWB_IO_Mode;                 /* true for an EVS AMR-WB IO compatibility mode packet, false otherwise */
     bool      fDTMF;                          /* true for a DTMF event payload, false otherwise */

     int       start_of_payload_data;          /* index into payload[] of start of payload data. If DSGetPayloadInfo() returns an error condition, this is the payload header byte on which the error occurred */

     int       amr_decoder_bit_pos;            /* reserved */

     int       Reserved[16];

  } PAYLOAD_INFO;

```

<a name="hellocodecExampleApp"></a>
# hello_codec Example App
 
This section gives a step-by-step description of hello_codec source code use of the voplib codec API interface.  Click here for [information on hello_codec build and command line usage](https://www.github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#user-content-hellocodec).


<a name="TestMeasurementandInteroperation"></a>
# Test, Measurement, and Interoperation

The following tools and reference apps are used for codec regression test, debugging customer issues, and verifying interoperation with network and telecom endpoint codecs:

    mediaTest
      encode only
      decode only
      back-to-back encode/decode
      pcap and wav output
      
    mediaMin
      pcap and UDP RTP stream handling, buffering, and decoding
      further audio domain processing if enabled (stream merging, speech recognition, etc)

Codec Regression Test Scripts below gives instructions and notes for running regression test scripts. Here are some notes on more advanced test procedures:

1) In Signalogic labs we compare output md5 sums with reference test outputs. As these figures vary from system to system (and from compiler to compiler), please contact Signalogic to discuss if you're interested in such test procedures

2) Some of the script command lines below include the --md5sum option, in most cases the final md5 sum shown for mediaTest or mediaMin output should be repeatable. For any "time stamp match" mode command lines, they should absolutely be repeatable

<a name="CodecRegressionTestScripts"></a>
## Codec Regression Test Scripts

Below are regression test scripts used in Signalogic labs for testing EVS and AMR codecs.

Following is the basic test procedure:

1) Run the script (see "usage" notes in the script comments). Even with accelerated (bulk) pcap handling enabled, this can take several minutes
2) Copy all console output to an editor; for example in Putty this is under main menu "Copy All to Clipboard". Note that your console window should be set to hold at least 12,000 lines of scroll back (in Putty this is under main menu "Change Settings", "Window", "Lines of scrollback")
3) Search console output for the following keywords:

      error<br/>
      warning<br/>
      critical<br/>
      fail<br/>
      invalid<br/>
      exceed<br/>
      bad<br/>
      seg<br/>

There should be no occurrences.

<a name="EVSRegressionTestScript"></a>
### EVS Codec Regression Test Script

```bash
#!/bin/bash

# evs_interop_test.sh
#
# Copyright (c) Signalogic, Dallas, 2023-2024
#
# Objectives
#
#   -test encoding and decoding of combinations of EVS primary and AMR-WB IO mode bitrates, VBR mode, header-full and compact header, DTX and RF enable/disable
#   -decode EVS RTP streams in generated pcaps
#   -interoperate with several "in the wild" pcaps containing EVS RTP streams (some may include a mix of other codecs also)
#
# Notes
#
#   -mediaTest is used to generate wav and pcap files
#   -mediaMin is used to verify pcap files
#   -mediaMin cmd lines use -r0.9 to accelerate processing time. For detailed information on acceleration and bulk pcap handling, see
#      https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#bulk-pcap-handling
#
# Usage
#
#   cd /signalogic_software_installpath/sigsrf_sdk_demo/apps/mediaTest
#   source ./evs_interop_test.sh

# mediaTest outputs:
#   default mediaTest outputs are stored on mediaTest/test_files subfolder (installed with the SigSRF SDK from Rar packages or pre-installed in Docker containers)
#   for your system, replace as needed
MEDIATEST_OUTPUTS="test_files"

# mediaMin outputs:
#   default mediaMin outputs are stored to a RAM disk ("/tmp/shared" in the following examples)
#   for your system, replace as needed
#   to store output files on mediaMin subfolder, assign to empty string
MEDIAMIN_WAV_OUTPUTS="-g /tmp/shared"
MEDIAMIN_PCAP_OUTPUTS="--group_pcap_nocopy /tmp/shared"

# AMR-WB IO mode tests
#
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_15850_full_header.wav -Csession_config/evs_16kHz_15850bps_amrwb_io_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_15850_full_header_dtx_disabled.wav -Csession_config/evs_16kHz_15850bps_amrwb_io_full_header_dtx_disabled_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_15850_full_header_dtx_disabled.pcap -Csession_config/evs_16kHz_15850bps_amrwb_io_full_header_dtx_disabled_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_15850_full_header.pcap -Csession_config/evs_16kHz_15850bps_amrwb_io_full_header_config
cd mediaMin
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_15850_full_header.pcap -L -d0x08000c11 -r0.9 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
cd mediaMin
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_15850_full_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
cd ..
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_12650_full_header.wav -Csession_config/evs_16kHz_12650bps_amrwb_io_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_12650_full_header.pcap -Csession_config/evs_16kHz_12650bps_amrwb_io_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_12650_compact_header.wav -Csession_config/evs_16kHz_12650bps_amrwb_io_compact_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_12650_compact_header.pcap -Csession_config/evs_16kHz_12650bps_amrwb_io_compact_header_config
cd mediaMin
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_12650_full_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_12650_compact_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
cd ..
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_23850_full_header.wav -Csession_config/evs_16kHz_23850bps_amrwb_io_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_23850_full_header.pcap -Csession_config/evs_16kHz_23850bps_amrwb_io_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_23850_compact_header.wav -Csession_config/evs_16kHz_23850bps_amrwb_io_compact_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_23850_compact_header.pcap -Csession_config/evs_16kHz_23850bps_amrwb_io_compact_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_23050_full_header.pcap -Csession_config/evs_16kHz_23050bps_amrwb_io_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_23050_compact_header.pcap -Csession_config/evs_16kHz_23050bps_amrwb_io_compact_header_config
cd mediaMin
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_23850_full_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_23850_compact_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_23050_full_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_23050_compact_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
cd ..
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_8850_full_header.wav -Csession_config/evs_16kHz_8850bps_amrwb_io_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_8850_full_header.pcap -Csession_config/evs_16kHz_8850bps_amrwb_io_full_header_config
cd mediaMin
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_8850_full_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
cd ..
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_6600_full_header.wav -Csession_config/evs_16kHz_6600bps_amrwb_io_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_6600_full_header.pcap -Csession_config/evs_16kHz_6600bps_amrwb_io_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_6600_compact_header.wav -Csession_config/evs_16kHz_6600bps_amrwb_io_compact_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_6600_compact_header.pcap -Csession_config/evs_16kHz_6600bps_amrwb_io_compact_header_config
cd mediaMin
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_6600_full_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_6600_compact_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# mixed modes and mixed rates, including AMR-WB IO mode with bit shifted payloads
#
mediaMin -cx86 -i../pcaps/evs_mixed_mode_mixed_rate.pcap -L -d0x0c0c0c01 -r0.9 -C../session_config/EVS_AMR-WB_IO_mode_payload_shift "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
cd ..

# VBR 5900 bps mode tests
#
mediaTest -cx86 -itest_files/stv16c.wav -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_5900_full_header.wav -Csession_config/evs_16kHz_5900bps_full_header_config  
mediaTest -cx86 -itest_files/stv16c.wav -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_5900_compact_header.wav -Csession_config/evs_16kHz_5900bps_compact_header_config  
mediaTest -cx86 -itest_files/stv16c.wav -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_5900_full_header_dtx_disabled.wav -Csession_config/evs_16kHz_5900bps_full_header_dtx_disabled_config
mediaTest -cx86 -itest_files/stv16c.wav -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_5900_compact_header_dtx_disabled.wav -Csession_config/evs_16kHz_5900bps_compact_header_dtx_disabled_config
mediaTest -cx86 -itest_files/stv16c.wav -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_5900_full_header.pcap -Csession_config/evs_16kHz_5900bps_full_header_config
mediaTest -cx86 -itest_files/stv16c.wav -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_5900_compact_header.pcap -Csession_config/evs_16kHz_5900bps_compact_header_config
mediaTest -cx86 -itest_files/stv16c.wav -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_5900_full_header_dtx_disabled.pcap -Csession_config/evs_16kHz_5900bps_full_header_dtx_disabled_config
mediaTest -cx86 -itest_files/stv16c.wav -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_5900_compact_header_dtx_disabled.pcap -Csession_config/evs_16kHz_5900bps_compact_header_dtx_disabled_config
mediaTest -cx86 -itest_files/AAdefaultBusinessHoursGreeting.pcm -o"${MEDIATEST_OUTPUTS}"/AAdefaultBusinessHoursGreeting_16kHz_5900_full_header.wav -Csession_config/evs_16kHz_5900bps_full_header_config
mediaTest -cx86 -itest_files/AAdefaultBusinessHoursGreeting.pcm -o"${MEDIATEST_OUTPUTS}"/AAdefaultBusinessHoursGreeting_16kHz_5900_compact_header.wav -Csession_config/evs_16kHz_5900bps_compact_header_config
mediaTest -cx86 -itest_files/AAdefaultBusinessHoursGreeting.pcm -o"${MEDIATEST_OUTPUTS}"/AAdefaultBusinessHoursGreeting_8kHz_5900_full_header.wav -Csession_config/evs_8kHz_input_8kHz_5900bps_full_header_config
mediaTest -cx86 -itest_files/AAdefaultBusinessHoursGreeting.pcm -o"${MEDIATEST_OUTPUTS}"/AAdefaultBusinessHoursGreeting_8kHz_5900_compact_header.wav -Csession_config/evs_8kHz_input_8kHz_5900bps_compact_header_config

mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stvc16_48kHz_5900.wav -Csession_config/evs_48kHz_input_16kHz_5900bps_full_band_config
mediaTest -cx86 -itest_files/T_mode.wav -o"${MEDIATEST_OUTPUTS}"/T_mode_48kHz_5900.wav -Csession_config/evs_48kHz_input_16kHz_5900bps_full_band_config

cd mediaMin
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_5900_full_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_5900_compact_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# 24400 and 13200, mix of CMR/no CMR, mix of ptimes per payload, variable ptime, RF enable
#
mediaMin -c x86 -i ../test_files/evs_float_b24_4m_wb_cbr_hfOnly0_cmr0_ptime20.pcap -L -d 0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -c x86 -i ../test_files/evs_float_b24_4m_dtx_swb_cbr_hfOnly1_cmr1_ptime20.pcap -L -d 0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -c x86 -i ../test_files/evs_float_b24_4m_dtx_swb_cbr_hfOnly0_cmr1_ptime60.pcap -L -d 0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -c x86 -i ../test_files/evs_float_b13_2m_dtx_swb_cbr_rf3hi_hfOnly0_cmr0_ptime20.pcap -L -d 0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -c x86 -i ../test_files/evs_float_b13_2m_dtx_swb_cbr_rf3hi_hfOnly0_cmr1_ptime20.pcap -L -d 0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# VBR 5900 mono channel, hf0 has collision avoidance padding, hf1 is hf-only format
#
mediaMin -cx86 -i../pcaps/evs_5900_1_hf0.rtpdump -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../pcaps/evs_5900_1_hf1.rtpdump -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
#
# VBR 5900 2x mono channels, hf0 has collision avoidance padding, hf1 is hf-only format
#
mediaMin -cx86 -i../pcaps/evs_5900_2_hf0.rtpdump -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../pcaps/evs_5900_2_hf1.rtpdump -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
cd ..

# 16400 bps tests
#
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_16400_full_header.pcap -Csession_config/evs_16kHz_16400bps_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_16400_compact_header.pcap -Csession_config/evs_16kHz_16400bps_compact_header_config
cd mediaMin
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_16400_full_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_16400_compact_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
cd ..

# 24400 bps tests
#
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_24400_full_header.pcap -Csession_config/evs_16kHz_24400bps_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_24400_compact_header.pcap -Csession_config/evs_16kHz_24400bps_compact_header_config
cd mediaMin
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_24400_full_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_24400_compact_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
cd ..

# 13200 bps tests, including RF enable mode tests
#
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_13200_full_header.wav -Csession_config/evs_16kHz_13200bps_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_13200_compact_header.wav -Csession_config/evs_16kHz_13200bps_compact_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_13200_full_header.pcap -Csession_config/evs_16kHz_13200bps_full_header_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_13200_compact_header.pcap -Csession_config/evs_16kHz_13200bps_compact_header_config
cd mediaMin
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_13200_full_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_13200_compact_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_13200_full_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_evs_16kHz_13200_compact_header.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# 17 min audio pcap with 2x EVS RTP streams out of alignment (also includes one AMR-WB stream)
#
mediaMin -cx86 -i../pcaps/evs_long_rate_alignment.pcap -L -d0x08000c11 -r0.9 "${MEDIAMIN_WAV_OUTPUTS}" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# return to mediaTest folder starting point
cd ..
```
<a name="AMRRegressionTestScript"></a>
### AMR NB / WB Codec Regression Test Script

```bash
#!/bin/bash

# amr_interop_test.sh
#
# Copyright (c) Signalogic, Dallas, 2023-2024
#
# Objectives
#
#   -encode and decode all possible AMR NB and WB bitrates and octet-aligned and bandwidth-efficient formats
#   -decode AMR NB and WB RTP streams in generated pcaps
#   -interoperate with several "in the wild" pcaps containing AMR NB and WB RTP streams (some may include a mix of other codecs also)
#
# Notes
#
#   -mediaTest is used to generate wav and pcap files
#   -mediaMin is used to verify pcap files
#   -mediaMin cmd lines use -r0.N to accelerate processing time. For detailed information on acceleration and bulk pcap handling, see
#      https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#bulk-pcap-handling
#
# Usage
#
#   cd /signalogic_software_installpath/sigsrf_sdk_demo/apps/mediaTest
#   source ./amr_interop_test.sh

# mediaTest outputs:
#   default mediaTest outputs are stored on mediaTest/test_files subfolder (installed with the SigSRF SDK from Rar packages or pre-installed in Docker containers)
#   for your system, replace as needed
MEDIATEST_OUTPUTS="test_files"

# mediaMin outputs:
#   default mediaMin outputs are stored to a RAM disk ("/tmp/shared" in the following examples)
#   for your system, replace as needed
#   to store output files on mediaMin subfolder, assign to empty string
MEDIAMIN_WAV_OUTPUTS="-g /tmp/shared"
MEDIAMIN_PCAP_OUTPUTS="--group_pcap_nocopy /tmp/shared"

# generate wav and/or pcap files with all possible AMR NB and WB bitrates

mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_23850_16kHz_mime.pcap -Csession_config/amrwb_codec_test_config

mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_23850_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_23850bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_23850_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_23850bps_octet_align_config

mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_23050_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_23050bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_23050_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_23050bps_octet_align_config

mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_19850_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_19850bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_19850_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_19850bps_octet_align_config

mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_18250_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_18250bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_18250_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_18250bps_octet_align_config

mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_15850_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_15850bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_15850_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_15850bps_octet_align_config

mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_14250_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_14250bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_14250_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_14250bps_octet_align_config

mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_12650_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_12650bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_12650_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_12650bps_octet_align_config

mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_8850_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_8850bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_8850_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_8850bps_octet_align_config

mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_6600_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_6600bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv16c.INP -o"${MEDIATEST_OUTPUTS}"/stv16c_amr_6600_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_6600bps_octet_align_config

mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_12200_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_12200bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_12200_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_12200bps_octet_align_config

mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_10200_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_10200bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_10200_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_10200bps_octet_align_config

mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_7950_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_7950bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_7950_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_7950bps_octet_align_config

mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_7400_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_7400bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_7400_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_7400bps_octet_align_config

mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_6700_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_6700bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_6700_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_6700bps_octet_align_config

mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_5900_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_5900bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_5900_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_5900bps_octet_align_config

mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_5150_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_5150bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_5150_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_5150bps_octet_align_config

mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_4750_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_4750bps_bandwidth_efficient_config
mediaTest -cx86 -itest_files/stv8c.INP -o"${MEDIATEST_OUTPUTS}"/stv8c_amr_4750_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_4750bps_octet_align_config

# run mediaMin on generated pcap files

cd mediaMin

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_23850_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_23850_bw_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_23850_oa_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_23050_bw_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_23050_oa_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_19850_bw_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_19850_oa_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_18250_bw_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_18250_oa_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_15850_bw_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_15850_oa_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_14250_bw_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_14250_oa_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_12650_bw_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_12650_oa_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_8850_bw_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_8850_oa_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_6600_bw_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv16c_amr_6600_oa_16kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_12200_bw_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_12200_oa_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_10200_bw_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_10200_oa_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_7950_bw_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_7950_oa_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_7400_bw_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_7400_oa_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_6700_bw_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_6700_oa_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_5900_bw_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_5900_oa_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_5150_bw_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_5150_oa_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_4750_bw_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"
mediaMin -cx86 -i../"${MEDIATEST_OUTPUTS}"/stv8c_amr_4750_oa_8kHz_mime.pcap -L -d0x08000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# run mediaMin on various test cases

# AMR-NB 12200 bandwidth-efficient md5 sum ending in 0012d2
mediaMin -c x86 -i ../pcaps/announcementplayout_metronometones1sec_2xAMR.pcapng -L -d 0x580000008040811 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }" --md5sum

# AMR-WB 12200 octet-algined md5sum ending in c0dd1f
mediaMin -cx86 -i../pcaps/AMR_MusixFile.pcap -L -d0x08040c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }" --md5sum

mediaMin -cx86 -C../session_config/merge_testing_config_amrwb -i../pcaps/AMRWB.pcap -i../pcaps/pcmutest.pcap -L -d0x08040800 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# AMR 23850 octet-aligned md5 sum ending in d689c8
mediaMin -cx86 -i../pcaps/AMRWB.pcap -i../pcaps/pcmutest.pcap -L -d0x08040c10 -r0.9 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }" -C../session_config/merge_testing_config_amrwb --md5sum

# AMR-WB 23850 octet-aligned md5 sum ending in cd0e3d
mediaMin -cx86 -i../pcaps/AMRWB.pcap -L -d0x08040c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }" --md5sum

mediaMin -cx86 -C ../session_config/amrwb_packet_test_config_AMRWB-23.85kbps-20ms_bw -i ../pcaps/AMRWB-23.85kbps-20ms_bw.pcap -r0 -L -d0x08040800 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

mediaMin -cx86 -i ../pcaps/AMRWB-23.85kbps-20ms_bw.pcap -r0 -L -d0x20000008040801 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# AMR-WB 23850 bandwidth-efficient md5 sum ending in d876b3 (not stable)
mediaMin -cx86 -i ../pcaps/AMRWB-23.85kbps-20ms_bw.pcap -r0.9 -L -d0x20000008040c01 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }" --md5sum

mediaMin -cx86 -C../session_config/merge_testing_config_amr -i../pcaps/AMR_MusixFile.pcap -i../pcaps/PCMU.pcap -L -r0.5 -d0x08000c11 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# AMR-WB 23850 bandwidth-efficient
mediaMin -cx86 -C ../session_config/amrwb_packet_test_config_AMRWB_SID -i../pcaps/AMRWB_SID.pcap -r0.5 -L -d0x08040800 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# AMR-WB 12650 md5 sum ending in cb27d5
mediaMin -cx86 -i../pcaps/mediaplayout_music_1malespeaker_5xAMRWB_notimestamps.pcapng -L -d0x080c0c01 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# AMR-NB 12200 bandwidth-efficient md5 sum ending in 629aff
mediaMin -cx86 -i../test_files/crash1.pcap -L -d0x580000008040811 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }" -l2 --md5sum

# AMR-NB 12200 bandwidth-efficient md5 sum ending in b6f504
mediaMin -cx86 -i../test_files/tmpwpP7am.pcap -L -d0x580000008040811 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }" --md5sum

# AMR-WB 23850 octet-aligned only 1 packet
mediaMin -cx86 -i../test_files/amr-bw-efficient.pcap -L -d0x20018000c11 -r20 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# AMR-WB 23850 octet-aligned md5 sum ending in ebc64b
mediaMin -c x86 -i ../test_files/codecs3-amr-wb.pcap -L -d 0x20018040c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }" --md5sum

mediaMin -cx86 -i../test_files/codecs-amr-12.pcap -L -d0x20018000c11 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }"

# AMR-NB 4750 bandwidth-efficient md5 sum ending in a023fb
mediaMin -cx86 -i../test_files/81786.4289256.478164.pcap -L -d0x580000008040811 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }" --md5sum

# AMR-NB 5900 bandwidth-efficient md5 sum ending in 3389db
mediaMin -cx86 -i../test_files/85236.4284266.158664.pcap -L -d0x580000008040811 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }" --md5sum

# AMR-WB 6600 bandwidth-efficient md5 sum ending in 1d2bd2
mediaMin -cx86 -i../test_files/65446.4425483.49980.pcap -L -d0x580000008040811 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }" --md5sum

# AMR-WB 6600 bandwidth-efficient md5 sum ending in ca6e94
mediaMin -cx86 -i../test_files/6936.3576684.1144122.pcap -L -d0x580000008040811 -r0.5 "$MEDIAMIN_WAV_OUTPUTS" "${MEDIAMIN_PCAP_OUTPUTS% *}" "${MEDIAMIN_PCAP_OUTPUTS#* }" --md5sum

cd ..
```
