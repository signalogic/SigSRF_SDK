# SigSRF Codecs Documentation

<a name="TOC"></a>
## Table of Contents

[**_Overview_**](#user-content-overview)<br/>

[**_Test, Measurement, and Interoperation_**](#user-content-testmeasurementandinteroperation)<br/>

[**_I/O Support_**](#user-content-iosupport)<br/>

[**_API Interface_**](#user-content-apiinterface)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;[**DSCodecCreate**](#user-content-dscodeccreate)<br/>
&nbsp;&nbsp;&nbsp;[**DSCodecDecode**](#user-content-dscodecdecode)<br/>
&nbsp;&nbsp;&nbsp;[**DSCodecEncode**](#user-content-dscodecencode)<br/>
&nbsp;&nbsp;&nbsp;[**DSGetCodecInfo**](#user-content-dsgetcodecinfo)<br/>
&nbsp;&nbsp;&nbsp;[**DSCodecDelete**](#user-content-dscodecdelete)<br/>
</sup></sub>

&nbsp;&nbsp;&nbsp;[**Structs**](#user-content-structs)<br/>
<sub><sup>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**CODEC_PARAMS**](#user-content-codecparams)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**CODEC_DEC_PARAMS**](#user-content-codecdecparams)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**CODEC_ENC_PARAMS**](#user-content-codecencparams)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**CODEC_OUTARGS**](#user-content-codecoutargs)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**CODEC_INARGS**](#user-content-codecinargs)<br/>
</sup></sub>

[**_hello_codec Example App_**](#user-content-hellocodecexampleapp)<br/>

<a name="Overview"></a>
# Overview

Codecs are an essential component in streaming media use cases, necessary to decode RTP media in telecom, analytics, robotics, and many other applications. In addition, codecs are often necessary to encode media, for example transcoding in SBC, language translation, home and mobile assistants, and other applications. This page documents SigSRF codecs available in the mediaMin, mediaTest, and hello_codec reference applications, including EVS, AMR, AMR-WB, G711, and more. As part of the SigSRF SDK, codecs may also be incorporated into user applications for test and evaluation purposes. Documentation is organized as:

  * test, measurement, and interoperation [:link:](#user-content-testmeasurementandinteroperation)
  * I/O support, including audio waveform files, pcap, and RTP [:link:](#user-content-iosupport)
  * API interface [:link:](#user-content-apiinterface)
  * "hello codec" example user application [:link:](#user-content-hellocodecexampleapp)

SigSRF codecs are available in demo format in Docker containers and installable Rar packages, and in production / deployment format as a codec license, or included in mediaMin and mediaTest licenses. Demo licenses are limited in total time and frame duration; after some point they continue to operate but produce "noisy media".

Below is a software architecture diagram showing the relationship between user applications, voplib, and codec libraries. 

<p>&nbsp;</p>

![SigSRF codecs software architecture diagram](https://github.com/signalogic/SigSRF_SDK/blob/master/images/SigSRF_codecs_software_architecture_diagram.png?raw=true "SigSRF codecs software architecture diagram")

<p>&nbsp;</p>

[mediaTest](https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#user-content-mediatest) is a tool for codec measurement and interoperation testing (performance, audio quality measurement, reference vector comparison, debug, etc). [mediaMin](https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#user-content-mediamin) is a production / deployment grade application (telecom, analytics, call recording, lawful interception, robotics, etc). mediaMin can use RTP auto-detection, SDP packet info, or both to determine codec type and <a href="mediaTest_readme.md#user-content-dynamicsessioncreation">create dynamic sessions</a>. [hello_codec](#user-content-hellocodecexampleapp) is a simple codec example application. 


Some additional notes about the above diagram:

* voplib provides the documented, unified interface to all codecs. Codecs have different numbers of shared object (.so) files, ranging from 1 to 3, depending on how their standards body source code is organized

* voplib supports two types of struct interfaces in [DSCodecCreate()](#user-content-dscodeccreate), CODEC_PARAMS and TERMINATION_INFO. See [API Interface](#user-content-apiinterface) below for more information

* the dashed line indicates high-level pktlib APIs such as DSPushPackets() and DSPullPackets() available to user applications. For example pktlib API usage see the <a href="../mediaTest_readme.md#minimum-api-interface">mediaMin Minimum API Interface</a> and look for PushPackets() and PullPackets() in [mediaMin.cpp source code](https://github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaMin/mediaMin.cpp)

* for data flow diagrams, see [telecom and analytics mode data flow diagrams](https://github.com/signalogic/SigSRF_SDK#user-content-telecommodedataflowdiagram) on the main SigSRF SDK page

<a name="TestMeasurementandInteroperation"></a>
# Test, Measurement, and Interoperation

  background info

    mediaTest - encode only, decode only, back-to-back encode/decode
    mediaMin -- RTP decode

  Pcap and RTP support

     -mediaTest

     -mediaMin
 
  EVS

    encode + decode

      mediaTest -cx86 -i test_files/stv16c.wav -o test_files/stv16c_13200bps.wav -C session_config/evs_16kHz_13200bps_full_header_config

      mediaTest -cx86 -i test_files/T_mode.wav -o test_files/T_mode_16kHz_24400.wav -C session_config/evs_16kHz_24400bps_config

    same, but add multithread test (-E execution mode option, c = command line, tN = number of threads).  For thread indexes 1 and higher output filenames have _n suffix (n = 0 .. N-1)
  
      mediaTest -cx86 -i test_files/stv16c.wav -o test_files/stv16c_13200bps.wav -C session_config/evs_16kHz_13200bps_full_header_config -Ec -t2

      mediaTest -cx86 -i test_files/T_mode.wav -o test_files/T_mode_16kHz_24400.wav -C session_config/evs_16kHz_24400bps_config -Ec -t2

    encode only

      mediaTest -cx86 -i test_files/stv16c.wav -o test_files/stv16c_13200bps.cod -C session_config/evs_16kHz_13200bps_full_header_config

      mediaTest -cx86 -i test_files/T_mode.wav -o test_files/T_mode_16kHz_24400.cod -C session_config/evs_16kHz_24400bps_config

    decode only

      mediaTest -cx86 -i test_files/stv16c_13200bps.cod -o test_files/stv16c_13200bps.wav -C session_config/evs_16kHz_13200bps_full_header_config

      mediaTest -cx86 -i test_files/T_mode_16kHz_24400.cod -o test_files/T_mode_16kHz_24400.wav -C session_config/evs_16kHz_24400bps_config

    Primary Mode

      RF channel aware

    AMR-WB IO Compatibility Mode

      generate an example compliant with 3GPP spec
      
        mediaTest -cx86 -i test_files/stv16c.INP -o test_files/stv16c_16kHz_23850_full_header.wav -C session_config/evs_16kHz_23850bps_amrwb_io_full_header_config --md5sum

      should display

        DTX stats: frmsiz 7 = 54, frmsiz 1 = 232
  
      decode 3GPP spec compliant example
      
      decode example requiring 2-bit payload left-shift
      

    Low Bitrate 5900 bps Mode

      mediaTest -cx86 -i test_files/T_mode.wav -o test_files/T_mode_48kHz_5900.wav -C session_config/evs_48kHz_input_16kHz_5900bps_full_band_config

      mediaTest -cx86 -i test_files/T_mode.wav -o test_files/T_mode_48kHz_5900.cod -C session_config/evs_48kHz_input_16kHz_5900bps_full_band_config

      mediaTest -cx86 -i test_files/T_mode_48kHz_5900.cod -o test_files/T_mode_48kHz_5900.wav -C session_config/evs_48kHz_input_16kHz_5900bps_full_band_config

    Stereo Frames

  AMR
 
    AMR-WB

    AMR-NB 

<a name="IOSupport"></a>
# I/O Support

<a name="APIInterface"></a>
# API Interface

The following APIs and structs are defined in [voplib.h](https://github.com/signalogic/SigSRF_SDK/blob/master/includes/voplib.h).

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
     Flags
```c++
    #define DS_CODEC_CREATE_ENCODER                   /* create an encoder instance - may be combined with DS_CODEC_CREATE_DECODER */
    #define DS_CODEC_CREATE_DECODER                   /* create a decoder instance - may be combined with DS_CODCEC_CREATE_ENCODER */
    #define DS_CODEC_CREATE_USE_TERMINFO              /* pCodecInfo points to a TERMINATION_INFO struct. The default (no flag) is a CODEC_PARAMS struct */
```

For direct or "codec only" usage, pCodecInfo should point to a CODEC_PARAMS struct; for example usage see [mediaTest_proc.c](https://github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/mediaTest_proc.c) or [hello_codec.c](https://github.com/signalogic/SigSRF_SDK/blob/master/apps/hello_codec/hello_codec.c). For packet based applications (indirect codec usage), if the DS_CODEC_CREATE_USE_TERMINFO flag is given in uFlags, then pCodecInfo should point to a TERMINATION_INFO struct (defined in [shared_include/session.h](https://github.com/signalogic/SigSRF_SDK/blob/master/shared_includes/session.h)); for example usage see [packet_flow_media_proc.c (packet/media thread processing)](https://github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/packet_flow_media_proc.c)
  
<a name="DSCodecDecode"></a>
## DSCodecDecode

  * decodes one or more frames using one or more decoder handles
  * returns length of decoded media frame(s) (in bytes)

```c++
    int DSCodecDecode(HCODEC*          hCodec,        /* pointer to one or more codec handles, as specified by numChan */
                      unsigned int     uFlags,        /* see DS_CODEC_DECODE_xxx flags below */
                      uint8_t*         inData,        /* pointer to input coded bitstream data. Input may include a payload header and CMR value, if supported by the codec type and header/payload format */
                      uint8_t*         outData,       /* pointer to output audio frame data */
                      uint32_t         in_frameSize,  /* size of coded bitstream data, in bytes */
                      int              numChan,       /* number of channels to be decoded. Multichannel data must be interleaved */
                      CODEC_INARGS*    pInArgs,       /* optional parameters for decoding RTP payloads; see CODEC_INARGS struct notes below. If not used this param should be NULL */
                      CODEC_OUTARGS*   pOutArgs       /* optional decoder output info; see CODEC_OUTARGS struct notes below. If not used this param should be NULL */
                     );
```

   Flags

```c++
    #define DS_CODEC_DECODE_GET_NUMFRAMES             /* return the number of frames in the payload. No decoding is performed */
```
<a name="DSCodecEncode"></a>
## DSCodecEncode

  * encodes one or more frames using one or more encoder handles
  * returns length of encoded bitstream frame(s) (in bytes)

```c++
    int DSCodecEncode(HCODEC*          hCodec,        /* pointer to one or more codec handles, as specified by numChan */
                      unsigned int     uFlags,        /* see DS_CODEC_ENCODE_xxx flags below */
                      uint8_t*         inData,        /* pointer to input audio frame data */
                      uint8_t*         outData,       /* pointer to output coded bitstream data. Output may include a payload header and CMR value, if supported by the codec type and header/payload format */
                      uint32_t         in_frameSize,  /* size of input audio frame data, in bytes */
                      int              numChan,       /* number of channels to be encoded. Multichannel data must be interleaved */
                      CODEC_INARGS*    pInArgs,       /* optional parameters for encoding audio data; see CODEC_INARGS struct notes below. If not used this param should be NULL */
                      CODEC_OUTARGS*   pOutArgs       /* optional encoder output info; see CODEC_OUTARGS struct notes below. If not used this param should be NULL */
                     );
```
  Flags

    None

<a name="DSGetCodecInfo"></a>
## DSGetCodecInfo

  * returns information for the specified codec
  
```c++
    int DSGetCodecInfo(int codec,                     /* codec can be either a codec handle (HCODEC) or a codec type (int), depending on uFlags. In most cases uFlags should specify DS_CODEC_INFO_HANDLE to interpret codec as an hCodec, returned by a previous call to DSCodecCreate(). If neither DS_CODEC_INFO_HANDLE or DS_CODEC_INFO_TYPE is given, the default is DS_CODEC_INFO_HANDLE */
                       unsigned int uFlags,           /* if uFlags specifies DS_CODEC_INFO_TYPE, codec should be one of the types specified in shared_include/codec.h, and uFlags can also contain DS_CODEC_INFO_NAME, DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE, or DS_CODEC_INFO_PARAMS */
                       int nInput1,                   /* nInput1 is required for the DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE flag. nInput1 and nInput2 are required for uFlags DS_CODEC_INFO_BITRATE_TO_INDEX, DS_CODEC_INFO_INDEX_TO_BITRATE, and DS_CODEC_INFO_CODED_FRAMESIZE */
                       int nInput2,
                       void* pInfo                    /* returned info is copied into pInfo for uFlags DS_CODEC_INFO_NAME and DS_CODEC_INFO_PARAMS */
                      );
```
   Flags

```c++
    #define DS_CODEC_INFO_HANDLE                      /* specifies the "codec" param (first param) is interpreted as an hCodec (i.e. handle created by prior call to DSCodecCreate(). This is the default if neither DS_CODEC_INFO_HANDLE or DS_CODEC_INFO_TYPE is given */ 
    #define DS_CODEC_INFO_TYPE                        /* specifies the "codec" param (first param) is interpreted as a codec_type */ 
```
  Item Flags
  
  * if no item flag is given, DS_CODEC_INFO_HANDLE should be specified and pInfo should point to a CODEC_PARAMS struct
  * some item flags require the DS_CODEC_INFO_HANDLE flag (see per-flag comments)

```c++
    #define DS_CODEC_INFO_NAME                        /* returns codec name in text string pointed to by pInfo. A typical string length is 5-10 char, and always less than CODEC_NAME_MAXLEN char */
    #define DS_CODEC_INFO_RAW_FRAMESIZE               /* returns codec media frame size (i.e. prior to encode, after decode), in bytes. If DS_CODEC_INFO_HANDLE is not given, returns default media frame size for one ptime. For EVS, nInput1 should specify one of the four (4) EVS sampling rates (in Hz) */
    #define DS_CODEC_INFO_CODED_FRAMESIZE             /* returns codec compressed frame size (i.e. after encode, prior to decode), in bytes. If uFlags specifies DS_CODEC_INFO_TYPE then nInput1 should give a bitrate and nInput2 should give header format (0 or 1) */
    #define DS_CODEC_INFO_BITRATE                     /* returns codec bitrate in bps. Requires DS_CODEC_INFO_HANDLE flag */
    #define DS_CODEC_INFO_SAMPLERATE                  /* returns codec sampling rate in Hz. If DS_CODEC_INFO_HANDLE is not given, returns default sample rate for the specified codec. For EVS, nInput1 can specify one of the four (4) EVS sampling rates with values 0-3 */
    #define DS_CODEC_INFO_PTIME                       /* returns ptime in msec. Default value is raw framesize / sampling rate at codec creation-time, then is dynamically adjusted as packet streams are processed. Requires DS_CODEC_INFO_HANDLE flag */
    #define DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE       /* given an nInput1 sample rate in Hz, returns sample rate code specified in "xxx_codec_flags" enums in shared_include/codec.h, where xxx is the codec abbreviation (e.g. evs_codec_flags or melpe_codec_flags) */
    #define DS_CODEC_INFO_BITRATE_TO_INDEX            /* converts a codec bitrate (nInput1) to an index 0-31 (currently only EVS and AMR codecs supported) */
    #define DS_CODEC_INFO_INDEX_TO_BITRATE            /* inverse */
    #define DS_CODEC_INFO_PAYLOAD_SHIFT               /* returns payload shift specified in CODEC_PARAMS or TERMINATION_INFO structs at codec creation time, if any. Requires DS_CODEC_INFO_HANDLE flag. Default value is zero */
    #define DS_CODEC_INFO_CLASSIFICATION_TO_INDEX     /* converts a codec audio classification (nInput1) to an index 0-31 (currently only EVS and AMR codecs supported) */
    #define DS_CODEC_INFO_INDEX_TO_CLASSIFICATION     /* inverse */

    #define DS_CODEC_INFO_BITRATE_CODE                /* when combined with DS_CODEC_INFO_CODED_FRAMESIZE, indicates nInput1 should be treated as a "bitrate code" instead of a bitrate. A bitrate code is typically found in the RTP payload header, for example a 4 bit field specifying 16 possible bitrates. Currently only EVS and AMR codecs support this flag, according to Table A.4 and A.5 in section A.2.2.1.2, "ToC byte" of EVS spec TS 26.445. See mediaTest source code for usage examples */

    #define DS_CODEC_INFO_SIZE_BITS                   /* indicates DS_CODEC_INFO_CODED_FRAMESIZE return value should be in size of bits (instead of bytes) */

    #define DS_CODEC_INFO_SUPPRESS_WARNING_MSG        /* suppress any warning messages */

    #define DS_CODEC_INFO_ITEM_MASK                   /* the item mask can be used to AND uFlags and extract an item value */
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
<a name="Structs"></a>
## Structs

<a name="CodecParams"></a>
### CODEC_PARAMS

Struct used in DSCodecCreate() and DSGetCodecInfo()

```c++
  typedef struct {

     int codec_type;                      /* specifies codec type -- see "voice_codec_type" enums in shared_include/codec.h */
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

<a name="hellocodecExampleApp"></a>
# hello_codec Example App
 
