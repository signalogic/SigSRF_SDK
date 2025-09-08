/*
 $Header: /root/Signalogic/DirectCore/apps/SigC641x_C667x/streamTest/streamlib.h

 Copyright (C) Signalogic Inc. 2014-2025

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Purpose

  API header file for SigSRF streamlib shared library used in media domain processing for c66x, x86, Arm or combined coCPU platforms

 Revision History:

  Created Nov 2014 HP
  Modified Jan 2015 JHB, HANDLE passed by pointer to streamRead()
  Modified Apr 2015 JHB, additional constants and formatting changes required to support use in CIM programs
  Modified Apr 2015 AKM, converted to shared header file, host and target CPUs
  Modified Apr 2015 JHB, add STREAM_CODEC_xx and STREAM_FORMAT_xx definitions
  Modified Jul 2015 CKJ, add STREAM_ENDPOINT_ALG_xx constants to specify endpoint algorithms (e.g. an endpoint does video streaming or image analytics)
  Modified Jul 2015 JHB, add STREAM_ENDPOINT_BUFFERMEM and NICMASK items, added aligned(n) attribute for x86 struct items < 4 bytes
  Modified Nov 2018 JHB, moved stream group and stream merging definitions and APIs here (from pktlib.c and packet_flow_media_proc.c in pktlib.so build). Create new DSProcessStreamGroupContributors() and DSInitMergeGroup() APIs
  Modified Nov 2018 JHB, add DSResetMergeBuffers() API. This is used to maintain merging stream alignment when one stream has SSRC transitions and another one does not, for example B places A on hold, B then plays music-on-hold from a media server (new SSRC), and when B resumes (starts sending RTP again), it's out of sync
  Modified Nov 2018 JHB, change "merge" references to "group" or "stream" where it doesn't involve actual merging.  In preparation for other stream group operations, including speech recognition.  See also comments in session.h
  Modified Dec 2018 JHB, move MAX_GROUPID_LEN define here (was internal).  Add WHOLE_GROUP_THREAD_ALLOCATE flag (see comments below)
  Modified Jan 2019 JHB, increase MAX_STREAM_GROUPS from 128 to 256.  This allows higher group capacity with sessions that are 2-3 calls each
  Modified May 2019 JHB, changed stream_id parameters to int for DSAttachStreamToGroup and DSDeleteStreamFromGroup(), changed chnum to int for DSStoreStreamGroupContributorData() and DSGetStreamGroupContributorData()
  Modified May 2019 JHB, add GROUP_MODE_WAV_OUT_STREAM_MONO and GROUP_MODE_WAV_OUT_STREAM_MULTICHANNEL flags, to allow N-channel wav file format for group contributors in addition to per-stream mono wav file for each contributor
  Modified Sep 2019 JHB, add contrib_ch param to DSStreamGroupContributors(), used for error condition reporting
  Modified Oct 2019 JHB, add GROUP_MODE_OVERRUN_xx flags
  Modified Dec 2019 JHB, add DSProcessAudio() and DS_PROCESS_AUDIO_xx flags
  Modified Jan 2020 JHB, add DSGetChanGroup()
  Modified Jan 2020 JHB, rename GROUP_MODE_OVERRUN_STOP_CONTRIBUTOR_ON_DETECTION to GROUP_MODE_OVERFLOW_STOP_CONTRIBUTOR_ON_DETECTION
                         -overrun refers to a manageable condition in contributor buffers that prevents overflow from occurring, controlled by GROUP_MODE_OVERRUN_xx flags
                         -overflow refers to buffer wrap with undefined amount of lost data, which can happen when overrun management is disabled
                         -in event logs "overflow" indicates a problem situation, and "overrun" normal operation
  Modified Feb 2020 JHB, add DS_GETGROUPINFO_HANDLE_IDX flag which allows an idx to be passed to DSGetStreamGroupInfo() as the hSession param
  Modified Mar 2020 JHB, add STREAM_GROUP_ENABLE_DEUPLICATION flag
  Modified Apr 2020 JHB, removed session_info_thread[] param from DSStreamGroupContributors(). See comment in packet_flow_media_proc.c
  Modified May 2020 JHB, API rework:
                         -rename several APIs to add "Contributor", make clear distinction between APIs that operate on stream groups and ones that operate on individual contributors
                         -remove DSGetChanGroup(), add equivalent functionality in DSGetStreamGroupInfo() with DS_GETGROUPINFO_HANDLE_CHNUM flag
  Modified Jun 2020 JHB, new APIs added:
                         -DSDeduplicateStreams(). Source is in audio_domain_processing.c; see also comments below
                         -audio data related APIs including DSGetStreamGroupContributorDataAvailable(), DSGetStreamGroupContributorFramesAvailable(), DSGetStreamGroupContributorFramesize(), and DSGetStreamGroupContributorDataPtr(). See comments below
  Modified Jul 2020 JHB, change all PLC references to FLC (frame loss concealment, where frame refers to non-packetized raw media data)
  Modified Jan 2021 JHB, reduced MAX_GROUP_BUFFER_SIZE from 64000 to 32000 bytes, see comments
  Modified Jan 2021 JHB, add MAX_GROUP_BUFFER_TIME_8KHZ definition, to make stream group buffer processing independent of output sampling rate. Units in msec
  Modified Mar 2021 JHB, change DSMergeStreamGroupContributors() API to DSProcessStreamGroupContributors()
  Modified Jan 2023 JHB, add STREAM_GROUP_FLC_HOLDOFFS_ENABLE flag
  Modified May 2023 JHB, add input_buffer_interval param to DSProcessAudio(). Search for "FTRT" and "AFAP" and see comments in audio_domain_processing.c and mediaMin.cpp
  Modified Jul 2023 JHB, add STREAM_GROUP_WAV_OUT_INCLUDE_PAUSES_AS_SILENCE flag (applicable to group_term.group_mode; see TERMINATION_INFO struct in shared_include/session.h)
  Modified Aug 2023 JHB, add DSProcessStreamGroupContributorsTSM() and DSCloseStreamGroupsTSM() prototypes, TIMESTAMP_MATCH_XXX flags
  Modified Feb 2024 JHB, add DS_STREAMGROUP_INFO_MERGE_FILENAME and DS_STREAMGROUP_INFO_MERGE_TSM_FILENAME flag options for DSGetStreamGroupInfo()
  Modified Mar 2024 JHB, deprecate DS_CS_GLOBALCONFIG and DS_CS_DEBUGCONFIG flags; see comments
  Modified Jul 2024 JHB, add TIMESTAMP_MATCH_WAV_OUTPUT flag. See usage example in mediaMin.cpp
  Modified Sep 2024 JHB, add DSGetStreamGroupInfo() DS_STREAMGROUP_INFO_OWNER_SESSION flag
  Modified Dec 2024 JHB, comments only
  Modified Feb 2025 JHB, rename MAXSTREAMS to MAX_COCPU_STREAMS, move definition of MAX_STREAMS here from mediaTest.h
  Modified Mar 2025 JHB, add TIMESTAMP_MATCH_ENABLE_STREAM_SYNC and TIMESTAMP_MATCH_ENABLE_DEBUG_OUTPUT flags
  Modified Jun 2025 JHB, add DS_STREAMGROUP_INFO_MERGE_TSM_PACKET_COUNT flag
  Modified Aug 2025 JHB, add thread_index parameter to DSProcessStreamGroupContributorsTSM()
*/

#ifndef _STREAMLIB_H_
#define _STREAMLIB_H_

#include <stdlib.h> 
#include <string.h>

#ifndef _COCPU  /* host or native CPU of any type (i.e. not a coCPU) */

  #include <stdio.h>

  #if defined(_TI66X_ACCEL)  /* directcore.h not available (e.g. ffmpeg accelerator) */

    #define DWORD uint32_t
    #define UINT uint32_t
    #define WORD uint16_t
    #define HANDLE void*

  #else  /* x86 apps and libraries (DirectCore, Pktlib, CIM) */
  
    #include <unistd.h>  /* required by usleep() */
    #include <sys/types.h>  
    #include <sys/stat.h>
    #include <fcntl.h>

    #define MIN_HDR
      #include "directcore.h"  /* bring in MEDIAINFO struct */
    #undef MIN_HDR

    #include "config.h"

  #endif
#endif

/* constant defines */

#define MAX_STREAMS                   512              /* max concurrent streams currently supported */

#define MAX_COCPU_STREAMS             8                /* max concurrent coCPU streams currently supported -- note this constant will be used by both host CPUs and coCPUs; shared mem arrays using this constant must agree, for example in CIM code generation */

#define MIN_FPS                       4                /* for now we use 4 fps as minimum frame rate allowed */
#define DEFAULT_30_FPS                30

#define MAXVIDBUFSIZE                 262144U

#define MAXVIDDESCRIPTORSIZE          64

/* Stream endpoints.  Host endpoints can be file, pipe, screen buffer, or network interface.  Accelerator endpoints can be card memory or card network interface */

#define STREAM_ENDPOINT_NONE          0
#define STREAM_ENDPOINT_FILE          1                /* File endpoint, for example input file containing YUV video data, or output file containing H264 encoded .h264 data */
#define STREAM_ENDPOINT_PIPE          2
#define STREAM_ENDPOINT_SCREENBUFMEM  3
#define STREAM_ENDPOINT_TARGETCPUMEM  4                /* Target CPU memory endpoint, for example input buffer containing YUV video data, or output buffer containing H264 encoded .h264 data */
#define STREAM_ENDPOINT_HOSTMEM       5
#define STREAM_ENDPOINT_BUFFERMEM     6                /* Internal buffer memory endpoint, for example algorithms on core task groups exchanging data using double-buffering */
#define STREAM_ENDPOINT_NETWORK       8                /* Network endpoint, for example IP/UDP/RTP input stream.  Can be combined with other endpoints, for example output both to file and network stream */
#define STREAM_ENDPOINT_NICMASK       0x0007
#define STREAM_ENDPOINT_MASK          0x000f

/* Stream modes */

#define STREAM_MODE_ONESHOT           0x0010           /* Target CPU processing is done in one-shot; e.g. video file download */
#define STREAM_MODE_CONTINUOUS        0x0020           /* Target CPU processing is done on continuous basis, using circular queue or dual/quad buffering */
#define STREAM_MODE_MASK              0x00f0

/* Buffering modes */

#define BUFFERING_MODE_NONE           0
#define BUFMODE_CIRCULAR_QUEUE        0x0100
#define BUFMODE_NBUFFERS              0x0200           /* target CPU processing is done on continuous basis, using dual/quad buffering */
#define BUFFER_MODE_MASK              0x0f00

/* Stream codecs and formats */

#define STREAM_CODEC_H264             0x10000
#define STREAM_CODEC_MPEG2            0x20000
#define STREAM_CODEC_VP8              0x30000

#define STREAM_FORMAT_YUV             0x100000
#define STREAM_FORMAT_RGB             0x200000
#define STREAM_FORMAT_RTP             0x300000
#define STREAM_FORMAT_UDP             0x400000

/* Stream endpoint algorithms */

#define STREAM_ENDPOINT_ALG_NONE      0
#define STREAM_ENDPOINT_ALG_IA        0x01000000       /* endpoint uses image analytics algorithm */
#define STREAM_ENDPOINT_ALG_VID       0x02000000       /* endpoint uses video encoding and/or streaming */
#define STREAM_ENDPOINT_ALG_USETASKLIST  0x06000000
#define STREAM_ENDPOINT_ALG_MASK      0xff000000       /* mask used to get endpoint algorithm type */

/* misc */

#define STREAM_RESEEK_TO_START        0x1000000


/* Raw video data formats */

#define YUV12bits_per_pixel           12
#define YUV16bits_per_pixel           16
#define RGB24bits_per_pixel           24

/* YUV preloaded data base addresses in C66x mem */

#define TARGET_CPU_BUFFER_SIZE        (0x0C000000)  /* = 155520000L */
#define COCPU_BUFFER_SIZE             TARGET_CPU_BUFFER_SIZE

#if (defined(_USE_IA_) && !defined(_USE_VID_)) || defined(_IA_)

  #define TARGET_CPU_BUFFER_BASE_ADDR (0xC0000000) 
  #define COCPU_BUFFER_BASE_ADDR      TARGET_CPU_BUFFER_BASE_ADDR
  #define ERAM_PRELOADED_DATA         (0xC0000000)     /* ERAM = target External RAM */
  #define ERAM_OUTPUT_DATA_STREAMING  (0xE0000000)
  #define ERAM_OUTPUT_DATA_IMAGE      (0xE0000000)

#else

  #define TARGET_CPU_BUFFER_BASE_ADDR (0xC0000000) 
  #define COCPU_BUFFER_BASE_ADDR      TARGET_CPU_BUFFER_BASE_ADDR
  #define ERAM_PRELOADED_DATA         (0xC0000000)
  #define ERAM_OUTPUT_DATA_STREAMING  (0xFE000000)
  #define ERAM_OUTPUT_DATA_IMAGE      (0xF0000000)
  
#endif

#define MAX_MEM_BUFFER_SIZE           4194304

#define NUM_C66X_STREAM_BUFFERS       4

#ifdef __cplusplus
  extern "C" {
#else
  #ifndef bool
    #define bool unsigned char
  #endif
  #ifndef true
    #ifndef TRUE
      #define TRUE 1
    #endif
    #define true TRUE
  #endif
  #ifndef false
    #ifndef FALSE
      #define FALSE 0
    #endif
    #define false FALSE
  #endif
#endif

typedef struct {

  unsigned int        mode;
  unsigned int        bufferingMode;
  bool                fUseSequentialBuffering;
  bool                fPauseMode __attribute__((aligned (4)));  /* align atribute needed for x86 structs for items < 4 bytes to match c66x struct layout */

  unsigned int        bitRate __attribute__((aligned (4)));
  unsigned int        fEnableDebugInfo;

  unsigned int        ipAddr_dst;
  unsigned int        ipAddr_src;
  unsigned int        udpPort_dst;
  unsigned int        udpPort_src;
  unsigned long long  macAddr_dst;
  unsigned long long  macAddr_src;

  unsigned int        inputEndpoint;  /* possible endpoint flags defined above.  Flags can be combined */
  unsigned int        outputEndpoint;
  
  unsigned int        ddrInputBase;  /* ddrInputBase and ddrOutputBase are read from C66x software in cimRunHardware().  Defaults are set in cimGetGmdLine() */
  unsigned int        ddrOutputBase;
  unsigned int        dwAddr_host_inbufptr;
  unsigned int        dwAddr_host_outbufptr;
  
} STREAMINGPARAMS;  /* streaming params */


typedef STREAMINGPARAMS* PSTREAMINGPARAMS;

#define StreamingMode(a) a[0].Streaming.mode

/* CPU-only definitions and streaming APIs */

#ifndef _COCPU  /* host or native CPU of any type (not coCPU) */

  int streamWrite(HANDLE*, int, char*, UINT, DWORD, WORD);

  int streamRead(HANDLE*, int, char*, UINT, DWORD, WORD);


#ifdef _IA_  /* simplified version of multi-plane image data required for coCPU image analytics.  We use _IA_ here this might conflict with XDAIS codecs, JHB Jun 2017 */

typedef union {

   struct {
      unsigned int width;
      unsigned int height;
   } tileMem;

   unsigned int bytes;

} XDM2_BufSize;

typedef struct XDM2_SingleBufDesc {

   int8_t* buf;

   XDM2_BufSize bufSize;   /* Buffer size(for tile memory/row memory */

} XDM2_SingleBufDesc;

typedef struct IVIDEO2_BufDesc {

   int numPlanes;
   XDM2_SingleBufDesc planeDesc[3];

} IVIDEO2_BufDesc;

#endif  /* _IA_ */

#endif

/* shared CPU and coCPU streaming APIs */

int ReadStream(unsigned int uMode, unsigned char* inputBuf, unsigned int frameCount, unsigned int uFlags);

int WriteStream(unsigned int uMode, unsigned char* inputBuf, unsigned int numBytes, unsigned int uFlags);


/* APIs not currently suported by coCPUs */

#ifndef _COCPU

/* streamlib version string global var */

  extern const char STREAMLIB_VERSION[];

/* DSConfigStreamlib() handles basic lib configuration.  pGlobalConfig and pDebugConfig point to GLOBAL_CONFIG and DEBUG_CONFIG structs defined in config.h.  See DS_CP_xx flags below */

  int DSConfigStreamlib(GLOBAL_CONFIG* pGlobalConfig, DEBUG_CONFIG* pDebugConfig, unsigned int uFlags);  /* global config, debug config, or both can be configured depending on attributes specified in uFlags.  NULL should be given for either pointer not used */

/* DSConfigStreamlib() uFlags constants */

  #if 0  /* deprecated -- if GLOBAL_CONFIG* and DEBUG_CONFIG* params are non-NULL they are used */
  #define DS_CS_GLOBALCONFIG  0x01
  #define DS_CS_DEBUGCONFIG   0x02
  #endif
  #define DS_CS_INIT          0x04

/* Stream data storage and retrieval functions.  Notes:

   -Background info:
     
     -stream data is defined as data extracted (and decrypted and/or decoded if required) from packet payloads, for example media data, DTMF events, and other data types

     -one use of these functions is to handle endpoints with different ptimes (also known as "transrating").  Another use is as an interface to domain processing, for
      example speech recognition, image analytics, etc

   -DSStoreStreamData() stores events and 1 ptime worth of media data.  Typically it's called after a media packet has been decoded, although payloads may contain other
    types of data.  Possible return values:
      
     Number of data elements stored (typically this is one)
     Zero if no data elements were stored
     -1 for an error condition

   -DSGetStreamData() returns events and 1 ptime worth of media data for the outgoing endpoint (defined in session configuration files).  Possible return values:

     Number of available data elements, including both events (which are immediately available) and media frames that meet the ptime requirement
     Zero if no data elements are available
     -1 for an error condition
*/

  int DSStoreStreamData(int chnum, unsigned int uFlags, uint8_t *data, uint32_t data_len);
  int DSGetStreamData(int chnum, unsigned uFlags, uint8_t *data, uint32_t data_buf_len, uint32_t data_len[], uint32_t data_info[], uint32_t data_chan[]);


/* Stream Group APIs.  Stream group notes:

  1) Stream groups are used for signal processing, for example merging audio data (a subset of conferencing), and speech recognition.  To do useful work, each group must have one or more "contributors".  Each contributor is a stream (endpoint) defined by a termN in session creation (see session.h and its usage in mediaMin source)

  2) A session termN that first references a Group ID becomes the owner of the group.  Upon first reference, a Group ID is defined (note - pktlib stores endpoints in TERMINATIION_INFO structs that define all aspects of a stream)

  3) Subsequent contributors use DSAttachStreamToGroup() to become a member of an already-defined Group ID

  4) Streams can be attached and removed from stream groups as needed.  DSRemoveStreamFromGroup() removes a stream from a group

  5) DSInitStreamGroupGroup() initializes a stream group

  6) Child streams created dynamically contribute to their parent's group member stream. As per RFC8108, only one stream (parent or one of its children) within a session is active at any one time. This is all handled automatically; no attach or remove should be used

  7) For stream merging, merged output inherits properties of the group owner; i.e. it has the same sampling rate and encoding format as the transcoded output of the owner stream.
     For example, if there are two endpoints in a session, one PCMU and another AMR-WB, and the group owner is the PCMU endpoint, then merged output will be 16 kHz sampling rate encoded as AMR-WB.
     If the group owner is the AMR-WB endpoint, then merged output will be 8 kHz sampling rate encoded as PCMU.  In this example, reversing the sequence of endpoint definition would reverse owership.

     For more information on merging algorithms, see DSMergeStreamAudio() and DSMergeStreamAudioEx() APIs in alglib
*/

  #define MAX_STREAM_GROUPS           256     /* max number of stream groups supported */
  #define MAX_MERGE_BUFFER_SIZE       32000L  /* internal max merge buffer size, in samples (dual buffers per stream group, each 2 sec at 8 kHz Fs) */
  #define MAX_GROUP_BUFFER_SIZE       32000L  /* stream group output max buffer size, in bytes (one buffer per stream group, 2 sec at 8 kHz Fs) */
  
  #define DEFAULT_GROUP_BUFFER_FS     8000
  #define MAX_GROUP_BUFFER_TIME_8KHZ  (1.0*MAX_GROUP_BUFFER_SIZE/2/DEFAULT_GROUP_BUFFER_FS)  /* max stream group buffer time. Notes:
                                                                                                 -2 sec at 8 kHz Fs, 1 sec at 16 kHz, etc. Use app value of stream group Fs instead of GROUP_DEFAULT_FS
                                                                                                 -app stream group Fs can be controlled by group_term.sample_rate (see shared_include/session.h)
                                                                                                 -apps can set/read stream group buffer time dynamically by applying DSSet/GetSessionInfo(DS_SESSION_INFO_GROUP_BUFFER_TIME) in pktlib.h to the stream group owner session, but this is not recommended
                                                                                             */
  #define MAX_GROUPID_LEN             128

/* stream group flags:

   -apply only to group_term.group_mode (see TERMINATION_INFO struct in shared_include/session.h)
   -should not be combined with group contributor flags (although currently they may not overlap in value, at some point they might)
*/

  #define STREAM_GROUP_ENABLE_MERGING                                        1  /* merge all group contributors to generate "unified conversation" output and maintain stream alignment.  Contributors can opt in or out using the DS_MERGE_AUDIO_xx flags (alglib.h) in their termN.group_mode flags */
  #define STREAM_GROUP_ENABLE_CONFERENCING                                   2
  #define STREAM_GROUP_ENABLE_DEDUPLICATION                                  4  /* applies a deduplication algorithm, which looks for similar content between stream contributors and attempts to align highly similar streams. The objective is to reduce perceived reverb/echo due to duplicated streams */
  #define STREAM_GROUP_ENABLE_ASR                                            8  /* apply ASR to stream group output */

  /* stream group wav output.  Stream group output wav files are named xxx_groupN.wav, multichannel contributor wav files are named xxx_streamN.wav, and mono contributor wav files are named xxx_streamN_M.wav, where xxx is first-found -o cmd line entry and N and M are the group and stream numbers, respectively */

  #define STREAM_GROUP_WAV_OUT_MERGED                              0x10000000L  /* generate mono wav file output for stream group's merged output */
  #define STREAM_GROUP_WAV_OUT_STREAM_MONO                          0x2000000L  /* generate mono wav file for each stream group contributor */
  #define STREAM_GROUP_WAV_OUT_STREAM_MULTICHANNEL                  0x4000000L  /* generate multichannel wav file where each channel is a stream group contributor */
  #define STREAM_GROUP_WAV_OUT_INCLUDE_PAUSES_AS_SILENCE            0x8000000L  /* include input pauses in wav output (e.g. call on hold). Pauses are written as zero values */

  /* flags to disable default operation */

  #define STREAM_GROUP_FLC_DISABLE                                   0x100000L  /* disable FLC applied to output merged audio that compensates (avoids gaps in output audio) due to ingress packet loss or slow arrival rate */
  #define STREAM_GROUP_RTP_TIMESTAMP_ONHOLD_ADVANCE_DISABLE          0x400000L  /* disable RTP timestamp advance.  Merged output RTP timestamp advance occurs when all contributors are on hold or call-waiting and then resume after some time */

  #define STREAM_GROUP_FLC_HOLDOFFS_ENABLE                           0x200000L  /* enable FLC holdoffs, JHB Jan 2023 */

  /* debug / stats output flags */

  #define STREAM_GROUP_DEBUG_STATS                                 0x20000000L  /* print/log basic group stats */
  #define STREAM_GROUP_DEBUG_STATS_L2                              0x40000000L  /* print/log detailed group stats */

/* stream (contributor) flags:

   -apply only to termN.group_mode (see TERMINATION_INFO struct in shared_include/session.h)
   -can be combined with DS_AUDIO_MERGE_xx flags in alglib.h
   -should not be combined with stream group flags, although they may not overlap in value at some point they might
*/

  #define STREAM_CONTRIBUTOR_WHOLE_GROUP_THREAD_ALLOCATE            0x1000000L  /* specify the contributor's session should be allocated to the same packet/media thread to which the group owner session is allocated.  If all group contributors specify this, then the group will not be split across threads; i.e. "whole group allocate". Whole group allocation has higher performance because it avoids multithread semaphore locks inside streamlib */

  #define STREAM_CONTRIBUTOR_STOP_ON_OVERFLOW_DETECTION              0x100000L  /* stop contributor input if the contributor encounters buffer overflow */

  /* contributor enables /disables other than default operation */

  #define STREAM_CONTRIBUTOR_DISABLE_PACKET_FLUSH                     0x80000L  /* disable contributor pastdue flush */
  #define STREAM_CONTRIBUTOR_DORMANT_SSRC_DETECTION_DISABLE          0x200000L  /* disable dormant SSRC detection.  A dormant channel may have its SSRC "taken over" by another channel within the same merge group, and if so the pkt/media thread will detect this and flush the dormant channel's jitter buffer packets.  Note that this flag must be applied to each individual channel (term definition) in the stream group that should not be detected (i.e. not the group term) */
  #define STREAM_CONTRIBUTOR_ONHOLD_FLUSH_DETECTION_ENABLE           0x800000L  /* enable on-hold flush detection.  The default is not enabled, having been replaced by a method called "pastdue contributors" that reacts faster with more precision.  If used, on-hold flush occurs approx 0.75 of the merge buffer delay time after a stream goes inactive, to force any leftover audio still in the jitter buffer to be contributed to merge output */

  /* contributor buffer overrun flags:
  
    -each contributor buffer, or "audio channel buffer", contains per channel audio data input to stream group processing
    -overrun flags control response to contributor buffer overflow conditions.  Overflow eventually occurs when a contributor's incoming audio stream is in a sustained overrun state; for example incoming packet deltas are less than the expected ptime, so decoded data is arriving faster than the wall clock based stream group output rate
    -default operation is to drop a silence frame when imminent overflow is detected
    -this effectively reduces sampling rate of output audio very slightly, which compensates for input overrun
 */

  #define STREAM_CONTRIBUTOR_OVERRUN_DETECTION_DISABLE                0x10000L  /* disable contributor overrun detection */
  #define STREAM_CONTRIBUTOR_OVERRUN_DROP_NEXT_FRAME                  0x20000L  /* drop next frame when imminent overrun is detected  */
  #define STREAM_CONTRIBUTOR_OVERRUN_DISABLE_FRAME_DROP               0x40000L  /* disable contributor frame drop */

  #define STREAM_CONTRIBUTOR_OVERRUN_DROP_SILENCE_FRAME(a)  (!((a) & (STREAM_CONTRIBUTOR_OVERRUN_DISABLE_FRAME_DROP | STREAM_CONTRIBUTOR_OVERRUN_DROP_NEXT_FRAME)))  /* use this macro with contributor flags to check for default operation (drop silence frames) */

  #if 1 /* DECLARE_LEGACY_DEFINES */
  /* legacy defines for apps prior to Mar 2019 */
  #define GROUP_MODE_DISABLE_FLC                                             STREAM_GROUP_FLC_DISABLE
  #define GROUP_MODE_DISABLE_RTP_TIMESTAMP_ONHOLD_ADVANCE                    STREAM_GROUP_RTP_TIMESTAMP_ONHOLD_ADVANCE_DISABLE
  #define GROUP_MODE_DISABLE_DORMANT_SSRC_DETECTION                          STREAM_CONTRIBUTOR_DORMANT_SSRC_DETECTION_DISABLE
  #define GROUP_MODE_ENABLE_ONHOLD_FLUSH_DETECTION                           STREAM_CONTRIBUTOR_ONHOLD_FLUSH_DETECTION_ENABLE
  #endif

/* Stream attach/remove APIs:

  -stream_id must be -1 for a group owner, or a valid chnum (0 to NCORECHAN-1) for a contributor (in definitions below, stream_id and chnum are equivalent)
  -hSession is needed by DSAttachStreamToGroup() to assign the group's owner session (i.e. when stream_id = -1).  Otherwise, it's needed only for warning/error logging, and not used by these APIs in stream management
*/

  int DSAttachStreamToGroup(int stream_id, HSESSION hSession, char* group_name);    /* creates a group with one stream when group_name is first referenced, otherwise attaches a stream to an existing group */
  int DSRemoveStreamFromGroup(int stream_id, HSESSION hSession, char* group_name);  /* removes a stream from a group; if the last stream then the group is deleted */

  int DSListStreamGroups();
  int DSGetStreamGroupStats(char* group_name);

  int DSGetStreamGroupContributorData(int chnum, uint8_t* buf, int length, unsigned int uFlags);
  int DSStoreStreamGroupContributorData(int chnum, uint8_t* buf, int length, unsigned int uFlags);

/* flags for DSGetStreamGroupContributorData() and DSStoreStreamGroupContributorData() */

  #define DS_GROUPDATA_PEEK                                                  1  /* peek: can be used with DSGetGroupData() to see if a channel has a specific amount of data available. No actual data is returned (buf can be NULL) and internal buffer pointers are not modified */
  #define DS_GROUPDATA_TOTAL_AVAILABLE                                       2  /* DSGetGroupData() will return a channel's total amount of available data. length param is ignored, and no actual data is returned (buf can be NULL) and internal buffer pointers are not modified. This can be used to monitor a channel's overrun condition */
  #define DS_GROUPDATA_SIM_TEST                                              4  /* reserved */
  #define DS_GROUPDATA_NORMALIZE_INSERTION_POINT                             8  /* reserved */

  int DSGetStreamGroupInfo(int handle, unsigned int uFlags, int* pNumContributors, int contributor_list[], char* szInfo);  /* returns a group index, szInfo if specified points to string into which group_name or filename should be retrieved */

  #define DS_STREAMGROUP_INFO_CHECK_GROUPTERM                                0  /* DSGetStreamGroupInfo() flags assume handle is an hSession, specifying hSession's group term, term1 or term2 to determine group info. Default is to use only the group term */
  #define DS_STREAMGROUP_INFO_CHECK_TERM1                                    1
  #define DS_STREAMGROUP_INFO_CHECK_TERM2                                    2
  #define DS_STREAMGROUP_INFO_CHECK_ALLTERMS                                 3  /* try all terms, starting with group term, Retrieve group name in szInfo if specified */
  #define DS_STREAMGROUP_INFO_MERGE_FILENAME                                 4  /* retrieve stream group filename in szInfo. For a usage example see mediaMin.cpp */
  #define DS_STREAMGROUP_INFO_MERGE_TSM_FILENAME                             5  /* retrieve timestamp match mode stream group filename in szInfo */
  #define DS_STREAMGROUP_INFO_MERGE_TSM_PACKET_COUNT                         6  /* return timestamp match mode packet count */
  #define DS_STREAMGROUP_INFO_OWNER_SESSION                                  7  /* retrieve group owner session when combined with DS_STREAMGROUP_INFO_HANDLE_IDX and handle is a valid idx */

  #define DS_STREAMGROUP_INFO_ITEM_MASK                                   0xff

  #define DS_STREAMGROUP_INFO_HANDLE_IDX                                 0x100  /* handle will be interpreted as an idx. If neither DS_STREAMGROUP_INFO_HANDLE_IDX or DS_STREAMGROUP_INFO_HANDLE_CHNUM is given handle is interpreted as an hSession */
  #define DS_STREAMGROUP_INFO_HANDLE_CHNUM                               0x200  /* handle will be interpreted as a chnum */

  int DSGetStreamGroupContributorPastDue(int chnum);                     /* these 2 APIs only supported for analytics compatibility mode */
  int DSSetStreamGroupContributorPastDue(int chnum, int pastdue);

  int DSGetStreamGroupContributorDataAvailable(int chnum);               /* get a contributor's total buffer data available */
  int DSGetStreamGroupContributorFramesAvailable(int chnum);             /* get a contributor's number of available audio frames -- similar to DSGetStreamGroupContributorDataAvailable() above, but return value is in frames */
  int DSGetStreamGroupContributorFramesize(int chnum);                   /* get a contributor's audio framesize (note it varies by codec) */
  short int* DSGetStreamGroupContributorDataPtr(int chnum, int offset);  /* retrieve pointer to a contributor's audio data buffer. 2nd param is offset from start of the data buffer */
  int DSGetStreamGroupContributorMaxFrameCapacity(int chnum);            /* get a contributor's total frame capacity. Currently only used for run-time stats purposes in packet_flow_media_proc.c, when it calculates overrun capacity usage percentage */

  int DSGetStreamGroupPacketInfo(int nGroupIndex, unsigned short int* seq_num, int* timestamp, int timestamp_inc, int* SSRC);
  short int* DSGetStreamGroupContributorDelayBuffer(int chnum);
  int DSInitStreamGroup(HSESSION hSessionOwner);

/* interval stats are maintained by the FLC (frame loss concealment) algorithm in stream merging, which is used to compensate for irregular / slow ingress packet rates, in order to maintain constant regular packet intervals in output audio */

  #define MAX_INTERVAL_STATS  512

  typedef struct {

    int num_intervals;
    float missed_intervals;
    int flc_frames;
    int avail_merge_data;
  
  } INTERVAL_STATS;

  int DSResetContributorBuffer(HSESSION hSessionOwner, int chnum);

/* DSProcessStreamGroupContributors() handles individual stream overrun/underrun, and merges stream contributors into stream group output */

  int DSProcessStreamGroupContributors(HSESSION hSession, FILE* fp_out_pcap_merge, FILE* fp_out_wav_merge,  MEDIAINFO* MediaInfo_merge, char* szMissingContributors, int* pkt_group_cnt, int* num_thread_merge_contributions, unsigned long long cur_time, void* p_pkt_counters, int thread_index, int* contrib_ch);

/* DSProcessStreamGroupContributorsTSM() handles timestamp-matched wav output, for all contributor and merged wav files */

  int DSProcessStreamGroupContributorsTSM(HSESSION hSession, uint8_t* pktbuf, int pktlen[], int numpkts, const char* szInputStream, const char* szOutputPath, unsigned int uTimestampMatchMode, int thread_index);

/* flags for DSProcessStreamGroupContributorsTSM(). In packet_flow_media_proc.c and mediaMin.cpp, look for uTimestampMatchMode to see flag usage, JHB Aug 2023 */

  #define TIMESTAMP_MATCH_MODE_ENABLE                                        1  /* enable timestamp-match output mode */
  #define TIMESTAMP_MATCH_WAV_OUTPUT                                         2  /* enable wav output in timestamp-match mode */
  #define TIMESTAMP_MATCH_DISABLE_FLUSH                                      4  /* disable all jitter buffer packet flush (loss, level, etc) */
  #define TIMESTAMP_MATCH_DISABLE_RESYNCS                                    8  /* disable jitter buffer resync */
  #define TIMESTAMP_MATCH_INCLUDE_INPUT_PAUSES                            0x10  /* include input stream pauses in timestamp-match wav output */
  #define TIMESTAMP_MATCH_LIVE_MERGE_OUTPUT                               0x20  /* enable live timestamp-match wav merge output */

  #define TIMESTAMP_MATCH_ENABLE_STREAM_SYNC                              0x40  /* enable stream synchronization. This only turns on under several strict conditions, the beta version still has issues with repeatability, JHB Mar 2025 */
  #define TIMESTAMP_MATCH_ENABLE_DEBUG_OUTPUT                             0x80  /* enable timestamp match mode debug output */

/* DSProcessAudio() performs audio domain processing, with options for sampling rate conversion, ASR, user-defined signal processing, and packet output

   -input can be either (i) stream group 16-bit linear audio output or (ii) arbitrary session term2 audio output
   -source code is in audio_domain_processing.c, on mediaTest subfolder
   -streamlib .so includes this function, also the mediaMin application includes this function, allowing users to modify source as needed
   -see additional comments in audio_domain_processing.c
*/

  int DSProcessAudio(HSESSION hSession, uint8_t* group_audio_buffer, int* num_frames, int frame_size, unsigned int uFlags, int idx, int nMarkerBit, unsigned long long merge_cur_time, int16_t* delay_buffer, int sample_rate, int* pkt_group_cnt, int thread_index, FILE* fp_out_pcap_merge, float input_buffer_interval);

/* uFlags for DSProcessAudio() */

  #define DS_PROCESS_AUDIO_STREAM_GROUP_OUTPUT                        1  /* input audio frames (group_audio_buffer) are from the stream group indexed by idx */ 
  #define DS_PROCESS_AUDIO_CONVERT_FS                             0x100  /* convert sampling rate, upf and dnf specify up and down conversion multipliers */ 
  #define DS_PROCESS_AUDIO_APPLY_ASR                              0x200  /* ASR should be applied to processe audio */
  #define DS_PROCESS_AUDIO_ENCODE                               0x10000  /* encode audio */
  #define DS_PROCESS_AUDIO_PACKET_OUTPUT                        0x20000  /* processed audio output should be encoded, formatted into RTP packets, and sent to applications */

/* DSDeduplicateStreams() applies a deduplication algorithm, which looks for similar content between stream group contributors and attempts to align similar streams. The objective is to reduce perceived reverb/echo due to duplicated streams. A typical scenario is a multipath (duplicated) endpoint with different latencies */

  int DSDeduplicateStreams(int idx, int nContributors, int contrib_ch[], unsigned int uFlags);

  int DSPostProcessStreamGroup(HSESSION hSession, int thread_index);

/* close timestamp-matched wav output files */

  int DSCloseStreamGroupsTSM(HSESSION hSessions[], int num_sessions, unsigned int uTimestampMatchMode);  /* pointer to array of session handles, number of session handles, timestamp matching mode. Return value < 0 indicates error condition */

#endif  /* ifndef _COCPU */

#ifdef __cplusplus
}
#endif

#endif  /* _STREAMLIB_H_ */
