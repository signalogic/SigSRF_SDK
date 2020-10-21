/*
  $Header: /root/Signalogic/DirectCore/include/pktlib.h
 
  Description: Packet flow and streaming management library -- API for creating and managing network
               traffic sessions and for sending/receiving packets to/from Pktlib processing buffers
 
  Projects: SigSRF, DirectCore
 
  Copyright (C) Signalogic Inc. 2010-2020

  Revision History:
  
   Created Mar 2017 Chris Johnson, (some elements copied from legacy ds_vop.h)
   Modified Jun 2017 JHB, added RTP packet info items for use with DSGetPacketInfo()
   Modified Jun 2017 JHB, added DS_BUFFER_PKT_DISABLE_PROBATION, DS_BUFFER_PKT_FLUSH, and DS_BUFFER_PKT_RATECONTROL flags
   Modified Jul 2017 CJ, added pcap file related structs and functions
   Modified Jul 2017 CJ, added support for cases where channel numbers in Pktlib do not match up with codec handles in Voplib
   Modified Jul 2017 CJ, added DS_SESSION_USER_MANAGED flag which includes session id in channel hash key
   Modified Jul 2017 CJ, added support for user app supplied log file handle and log write 
   Modified Aug 2017 JHB, added FormatPkt struct definition and optional header pointer arg for DSFormatPacket() (replaces pyldType, which is now either looked up in session info using chnum arg,
                          or given in the optional RTP header pointer).  Added DS_FMT_PKT_USER_xxx attributes to support use of the FormatPkt* arg
   Modified Aug 2017 JHB, modified DSConvertFsPacket() to take an input data length arg and return amount of valid output data.  If the input data length arg is given as -1, the API calculates input data length internally based on channel info specified by chnum
   Modified Aug 2017 CKJ, added DS_GET_ORDERED_PKT_ENABLE_DTX flag and DTX handling in DSGetOrderedPackets()
   Modified Aug 2017 JHB, added packet_info[] arg to DSGetOrderedPackets() and DSBufferPackets(), added DS_PKT_INFO_SID and related DS_PKT_INFO_xxx flags
   Modified Sep 2017 JHB, added DSGetSessionStatus() API and status / error codes
   Modified Sep 2017 CKJ, added DS_PKT_PYLD_CONTENT_DTMF and DSGetDTMFInfo() API (references dtmf_event struct, defined in alarms.h)
   Modified Sep 2017 JHB, added DS_GETORD_PKT_ENABLE_SINGLE_PKT_LKAHD and DS_GETORD_PKT_ENABLE_DTMF flags
   Modified Sep 2017 CKJ, added DSStoreStreamData() and DSGetStreamData() to support variable ptime (unequal endpoint ptimes) and algorithm / signal processing insertion point
   Modified Mar 2018 JHB, moved PKTLIB_VERSION global var inside extern "C" (https://stackoverflow.com/questions/38141390/extern-and-extern-c-for-variables)
   Modified May-Jun 2018 CKJ, add stream group APIs, first use is for stream merging
   Modified Jul 2018 CKJ, add APIs and flag definitions to support running a media service as a thread or process, including DSConfigMediaService(), DSPushPackets(), and DSPullPackets()
   Modified Jul 2018 JHB, remove uFlags param from DSDeleteSession()
   Modified Jul 2018 JHB, add DSGetPacketInfo() uFlag DS_PKT_INFO_CHNUM_PARENT to support SSRC change detection when incoming packets are interleaved between sessions or otherwise in a random sequence, which can happen when receiving packets via DSRecvPackets()
   Modified Aug 2018 JHB, add entry function for thread based packet flow and media processing, void* packet_flow_media_proc(void* pExecutionMode)
   Modified Aug 2018 JHB, add DSGetPacketInfo() uFlags DS_PKT_INFO_CODEC_TYPE and DS_PKT_INFO_CODEC_TYPE_LINK, add DSGetSessionInfo() uFlag DS_SESSION_INFO_CODEC_TYPE
   Modified Aug 2018 JHB, add packet buffer length to DSPullPackets()
   Modified Aug 2018 CJK, new DSGetSessionInfo() flags to support enhanced stream groups (new method that uses a session definition third term, or "group term"
   Modified Aug 2018 JHB, new definition of DSConfigMediaService() and additional uFlags definitions
   Modified Aug 2018 CKJ, implement more flags in DSFormatPacket()
   Modified Aug 2018 JHB, additional DS_SESSION_INFO_xxx flags, including thread Id and channel parent
   Modified Sep 2018 JHB, increase max number of stream group contributors to 8
   Modified Oct 2018 JHB, change DSGetSessionInfo() return value from int to int64_t, change term_id param in DSGetSessionInfo() and DSSetSessionInfo() from int to int64_t.  This supports new session info, including 64-bit thread ID values
   Modified Oct 2018 JHB, add numPkts param to DSPullPackets(), to allow a specific number of packet to be pulled.  A -1 value indicates to pull all packets (which was the default operation prior to this mod)
   Modified Nov 2018 JHB, move all stream group and stream merging definitions and APIs to streamlib.h (streamlib.so must be included in mediaTest and mediaMin builds)
   Modified Dec 2018 JHB, add DS_CONFIG_MEDIASERVICE_SET_NICENESS flag to allow control over packet/media thread priority and niceness
   Modified Dec 2018 JHB, add uFlags element to PACKETMEDIATHREADINFO struct
   Modified Jan 2019 JHB, add DSGetThreadInfo()
   Modified Feb 2019 JHB, add profiling time items to PACKETMEDIATHREADINFO struct definition.  Items are labeled xxx_max", for example manage_time_max, input_time_max, decode_time_max, encode_time_max, etc.  Packet/media thread profiling can be turned on/off  
                          by calling DSConfigMediaService() with the thread index and DS_CONFIG_MEDIASERVICE_ENABLE_THREAD_PROFILING and DS_CONFIG_MEDIASERVICE_DISABLE_THREAD_PROFILING flags
   Modified Feb 2019 JHB, add DS_GETORD_PKT_ENABLE_SID_REPAIR uFlag for DSGetOrderedPackets().  Add DSGetJitterBufferInfo() and DSSetJitterBufferInfo() APIs.  Add DS_SESSION_INFO_TERM_FLAGS, DS_SESSION_INFO_MAX_LOSS_PTIMES, and DS_SESSION_INFO_DYNAMIC_CHANNELS uFlags for DSGetSessionInfo()
   Modified Mar 2019 JHB, add DSGetJitterBufferInfo() and DSSetJitterBufferInfo() APIs and associated definitions
   Modified May 2019 JHB, add DS_PKT_PYLD_CONTENT_DTMF_SESSION payload content type, which is returned by DSGetOrderedPackets() when returning DTMF packets matching a session-defined DTMF payload type.  Otherwise the generic DS_PKT_PYLD_CONTENT_DTMF content type is returned
   Modified Aug 2019 JHB, removed #ifdef USE_PKTLIB_INLINES around extern C definition of DSGetSessionInfo()
   Modified Aug 2019 JHB, DSPullPackets() hSession param changed from unsigned int to HSESSION
   Modified Sep 2019 JHB, change include folder for if_ether.h from "linux" to "netinet" to fix -Wodr (one definition rule) warning with gcc 5.4
   Modified Oct 2019 JHB, add DS_JITTER_BUFFER_INFO_SID_FILL definition
   Modified Nov 2019 JHB, add DS_JITTER_BUFFER_INFO_NUM_PKT_LOSS_FLUSH and DS_JITTER_BUFFER_INFO_NUM_PASTDUE_FLUSH flags to DSGetJitterBufferInfo()
   Modified Dec 2019 JHB, add DSWritePacketLogStats() API, can be called either with session handle or packet/media thread index
   Modified Dec 2019 JHB, DS_JITTER_BUFFER_INFO_xx flags to support run-time packet stats calculation added to pktlib
   Modified Jan 2020 JHB, add optional chnum param to DSBufferPackets() and DSGetPacketInfo() to return channel numbers of matched packets.  This includes parent or child as applicable, and can save time by avoiding subsequent calls to specifically ask for parent or child chnum
   Modified Jan 2020 JHB, implement elapsed time alarm debug option inside DSPushPackets(). See uPushPacketsElapsedTime and DS_ENABLE_PUSHPACKETS_ELAPSED_TIME_ALARM in shared_include/config.h
   Modified Mar 2020 JHB, deprecate DS_GETORD_PKT_ENABLE_DTX and DS_GETORD_PKT_ENABLE_SID_REPAIR flags. Instead of these flags, DTX handling and SID repair should be controlled by TERM_DTX_ENABLE and TERM_SID_REPAIR_ENABLE flags in TERMINATION_INFO struct uFlags element (shared_include/session.h). The deprecated flags can still be applied to force DTX handling / SID repair on a case-by-case basis, but it's not recommended
   Modified Mar 2020 JHB, add DS_JITTER_BUFFER_INFO_TERM_FLAGS and DS_JITTER_BUFFER_INFO_NUM_PURGES flags in DSGetJitterBufferInfo()
   Modified Mar 2020 JHB, implement DS_PUSHPACKETS_ENABLE_RFC7198_DEDUP flag for DSPushPackets(), which is useful for cases without packet timestamps (e.g. static session config or regular interval push rate)
   Modified Apr 2020 JHB, add DS_JITTER_BUFFER_INFO_NUM_xxx_DUPLICATE_PKTS, DS_JITTER_BUFFER_INFO_xxx_RESYNC_COUNT, and DS_JITTER_BUFFER_INFO_NUM_OUTPUT_xxx flags
   Modified Apr 2020 JHB, add DS_GETORD_ADV_TIMESTAMP flag
   Modified May 2020 JHB, add uTimestamp and uInfo params to DSGetOrderedPackets(). See comments
   Modified May 2020 JHB, add DS_JITTER_BUFFER_INFO_NUM_OUTPUT_DROP_PKTS, DS_JITTER_BUFFER_INFO_HOLDOFF_COUNT, and DS_JITTER_BUFFER_INFO_NUM_HOLDOFF_xxx items to DSGetJitterBufferInfo()
   Modified May 2020 JHB, add DS_JITTER_BUFFER_INFO_CUMULATIVE_TIMESTAMP and DS_JITTER_BUFFER_INFO_CUMULATIVE_PULLTIME flags to DSGetJitterBufferInfo()
   Modified May 2020 JHB, move IsPmThread() here as static inline from pktlib.c. Define IsPmThread as IsPmThreadInline
   Modified Oct 2020 JHB, add limited pcapng format capability to DSOpenPcap() and DSReadPcap(). This was mainly done to support TraceWrangler output (pcap anonymizer tool). Support for pcapng format write is not currently planned
 */

#ifndef _PKTLIB_H_
#define _PKTLIB_H_

#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <asm/byteorder.h>
#include <semaphore.h>
#include <sys/resource.h>

#include "alias.h"
#include "filelib.h"
#include "diaglib.h"

/* session and configuration struct and related definitions */

#include "shared_include/session_cmd.h"
#include "shared_include/config.h"

/* typedefs for various pktlib and voplib handles */

#if defined __MSVC__ 
 typedef void* HSESSION;  /* session handle */
#elif defined _LINUX_
 /* done in alias.h */
#elif defined _WIN32_WINNT
 /* done in alias.h */
#else
 typedef HGLOBAL HSESSION;  /* session handle */
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /* ipv6 header, copied from linux/ipv6.h to avoid redefined conflicts when including both linux/ipv6.h and arpa/inet.h */
  struct ipv6hdr {

  #if defined(__LITTLE_ENDIAN_BITFIELD)
    __u8    priority:4,
            version:4;
  #elif defined(__BIG_ENDIAN_BITFIELD)
    __u8    version:4,
            priority:4;
  #else
  #error   "Please fix <asm/byteorder.h>"
  #endif

    __u8    flow_lbl[3];

    __be16  payload_len;
    __u8    nexthdr;
    __u8    hop_limit;

    struct  in6_addr saddr;
    struct  in6_addr daddr;
  };

  #ifndef _COCPU  /* _COCPU not defined for host or native CPU (x86, PowerPC, ARM, etc) */

  /* RTP header struct */

  typedef struct {

    uint16_t  BitFields;       /* Bit fields = Vers:2, pad:1 xind:1 Cc:4 marker:1 pyldtype:7 */
    uint16_t  Sequence;        /* Sequence number */
    uint32_t  Timestamp;       /* Timestamp */
    uint32_t  SSRC;            /* SSRC */
    uint32_t  CSRC[1];         /* Remainder of header */

  } RTPHeader;

  /* UDP header struct */

  typedef struct {

    uint16_t  SrcPort;         /* Source Port */
    uint16_t  DestPort;        /* Destination Port */
    uint16_t  UDP_length;      /* Length */
    uint16_t  UDP_checksum;    /* Checksum */

  } UDPHeader;

  /* FormatPkt struct, used in DSFormatPacket() API */

  typedef struct {

    uint8_t    BitFlds;         /* Bit fields = Vers:4, Header length:4 */
    uint8_t    Type;            /* Type of Service:8 */
    uint16_t   TotalLength;     /* Total length */
    uint16_t   ID;              /* Identification */
    uint16_t   FlagFrag;        /* Flag:3, Fragment Offset:13 */
    uint8_t    TimeLive;        /* Time to live, Hop Count for IPv6 */
    uint8_t    Protocol;        /* Protocol */
    uint16_t   HeaderChecksum;  /* Header Checksum */
    uint8_t    TrafficClass;    /* Traffic Class */
    uint32_t   FlowLabel;       /* Flow Label */
    uint16_t   PayloadLength;   /* Size of the payload in octets */
    uint8_t    NextHeader;      /* Next header type */
    uint8_t    SrcAddr[16];     /* IPv4 or IPv6 source addr */
    uint8_t    DstAddr[16];     /* IPv4 or IPv6 dest addr */
    uint32_t   IP_Version;      /* DS_IPV4 or DS_IPV6 - defined in session.h */
   
    UDPHeader  udpHeader;
    RTPHeader  rtpHeader;

    uint16_t   ptime;

  } FormatPkt;

  /* basic IP, UDP, and RTP header lengths */

  #define IPV4_HEADER_LEN  20
  #define IPV6_HEADER_LEN  40
  #define UDP_HEADER_LEN    8
  #define RTP_HEADER_LEN   12

  #define MAX_IP_UDP_RTP_HEADER_LEN  (IPV6_HEADER_LEN + UDP_HEADER_LEN + RTP_HEADER_LEN)

  /* max RTP packet length, mediaTest has test cases consisting of ptimes up to 240 ms, G711 will require a 1994 byte packet for IPv6.  This definition also used in mediaMin */

  #define MAX_RTP_PACKET_LEN         (MAX_RAW_FRAME + MAX_IP_UDP_RTP_HEADER_LEN)

  /* thread level items */

  #define THREAD_STATS_TIME_MOVING_AVG  16

  #define THREAD_RUN_STATE              0
  #define THREAD_ENERGY_SAVER_STATE     1
  
  typedef struct {  /* per packet/media thread info */

    bool       fMediaThread;
    bool       packet_mode;
    bool       fNoJitterBuffersUsed;
    bool       fProfilingEnabled;
    bool       fFTRTPtime;

    int        nRealTime;  /* time allowed for pkt/media thread to run and still be in real-time (specified in msec) */
    int        nRealTimeMargin;  /* real-time overhead margin, specified as a percentage of nRealTime (for example 20%) */

    pthread_t  threadid;
    uint32_t   uFlags;
    sem_t      thread_sem;
    bool       thread_sem_init;
    pid_t      niceness;

    int        numSessions;  /* current number of assigned sessions */
    int        numGroups;    /* current number of assigned stream groups */

    int        nEnergySaverState;  /* current energy saver state */
    int        energy_saver_state_count;  /* number of times energy state has been entered */
    uint64_t   max_inactivity_time;
   
    uint64_t   max_elapsed_time_thread_preempt;
    uint64_t   current_elapsed_time_thread_preempt;
    int        manage_sessions_count_mismatch;
    int        manage_sessions_create_early_exit;
    int        manage_sessions_delete_early_exit;
    #define MS_HISTORY_LEN 4
    int        manage_sessions_creation_history[MS_HISTORY_LEN];
    int        manage_sessions_deletion_history[MS_HISTORY_LEN];
    int        manage_sessions_history_index;
    int        nChannelWavProc;
    int        num_streams_active;

    uint64_t   CPU_time_avg[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   manage_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   input_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   buffer_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   chan_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   pull_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   decode_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   encode_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   group_time[THREAD_STATS_TIME_MOVING_AVG];

    uint64_t   CPU_time_max;
    uint64_t   manage_time_max;
    uint64_t   input_time_max;
    uint64_t   buffer_time_max;
    uint64_t   chan_time_max;
    uint64_t   pull_time_max;
    uint64_t   decode_time_max;
    uint64_t   encode_time_max;
    uint64_t   group_time_max;

    int        num_buffer_packets[THREAD_STATS_TIME_MOVING_AVG];
    int        num_decode_packets[THREAD_STATS_TIME_MOVING_AVG];
    int        num_encode_packets[THREAD_STATS_TIME_MOVING_AVG];
    int        num_group_contributions[THREAD_STATS_TIME_MOVING_AVG];

    uint8_t    thread_stats_time_moving_avg_index;
    uint8_t    manage_time_index;
    uint8_t    input_time_index;
    uint8_t    buffer_time_index;
    uint8_t    chan_time_index;
    uint8_t    pull_time_index;
    uint8_t    decode_time_index;
    uint8_t    encode_time_index;
    uint8_t    group_time_index;

  } PACKETMEDIATHREADINFO;

  #define MAX_PKTMEDIA_THREADS         16
  #define NOMINAL_SESSIONS_PER_THREAD  51
  #define NOMINAL_GROUPS_PER_THREAD    17

/* packet stats history items, see comments near USE_CHANNEL_PKT_STATS in packet_flow_media_proc.c */

  #define PKT_STATS_CHUNK_SIZE  10000  /* in bytes */

  typedef struct {

    PKT_STATS*  pkt_stats;  /* pointer to channel's PKT_STATS[] array */
    int32_t     mem_usage;  /* current amount of mem usage, in bytes */
    int32_t     num_pkts;   /* channel's current number of pkt stats */

  } PKT_STATS_HISTORY;

  #endif  /* ifndef _COCPU */

/* pktlib version string global var */

  extern const char PKTLIB_VERSION[];

/* DSConfigPktlib() handles basic lib configuration.  pGlobalConfig and pDebugConfig point to GLOBAL_CONFIG and DEBUG_CONFIG structs defined in config.h.  See DS_CP_xx flags below */

  int DSConfigPktlib(GLOBAL_CONFIG* pGlobalConfig, DEBUG_CONFIG* pDebugConfig, unsigned int uFlags);  /* global config, debug config, or both can be configured depending on attributes specified in uFlags.  NULL should be given for either pointer not used */

/* Session APIs:

    -DSCreateSession() - create a session to send and/or receive packets on one of following network interfaces:

       Network Interface                   uFlags

       none [default]                      DS_SESSION_DP_NONE
       server motherboard                  DS_SESSION_DP_LINUX_SOCKETS
       network I/O add-in card             DS_SESSION_DP_DPDK_QUEUE
       coCPU card including network I/O    DS_SESSION_DP_COCPU_QUEUE

      The return value is a session handle (sessionHandle) for use with other APIs.  Inputs include:

        network interface -- see above

        uFlags -- see "uFlags" notes given here and in constant definition comments below

        session data -- in the mediaTest source code examples, session data is read from session configuration files.  User apps can re-use that method and source code, or implement alternative methods
                        to initialize SESSION_DATA structs

      Network interface notes:
      
        -when no network interface is specified (default):

          -user applications are responsible for packet I/O and calling DSBufferPackets(), DSGetOrderedPackets(), and other APIs
          -several APIs match sessions to packets using IP and UDP header hashing.  If user-managed sessions are active, then the session handle is also included in the hash

        -when a network interface is specified:

          -the background process uses the DSReceivePackets() and DSSendPackets() APIs for packet I/O, and calls other APIs internally
          -networkIfName examples include:

            -eth0, em1 ...    # motherboard NIC
            -p2p1, p6p1 ...   # add-in card or coCPU card

     One each of following operating modes, data flow paths, and timing values should be specified:

       Operating Mode                      uFlags                          Notes

       frames only, no packet headers      DS_SESSION_MODE_FRAME           Currently not supported
       packet flow with IP/UDP/RTP header  DS_SESSION_MODE_IP_PACKET
       packet flow with UDP/RTP header     DS_SESSION_MODE_UDP_PACKET

       Data Flow Path                      uFlags                          Notes

       user app APIs                       none [default]                  User application is responsible for packet I/O
       Linux sockets, background process   DS_SESSION_DP_LINUX_SOCKETS     Background process handles packet I/O by calling DSReceivePackets(), DSSendPackets(), and other APIs
       DPDK queue                          DS_SESSION_DP_DPDK_QUEUE
       coCPU queue                         DS_SESSION_DP_COCPU_QUEUE

      Timing                               uFlags

       -0 -- none, DSSessionTranscode() or other APIs are called by application code based on a user-defined timing.  This should be used with frame mode [default]
       -N -- internal SigSRF timing is used and packet and codec related APIs are explicitly called every N msec

     The following attributes may also be included in uFlags:

       Modes or Attributes                 uFlags                          Notes

       default                             none                            SigSRF internal jitter buffer is enabled
       no jitter buffer                    DS_SESSION_NO_JITTERBUFFER      This applies to background process usage only, otherwise user application code decides whether to call jitter buffer APIs
       use codec jitter buffer             DS_SESSION_CODEC_JITTERBUFFER   Only valid for codecs that include a jitter buffer in their formal spec
       enable dynamic channels             DS_SESSION_DYN_CHAN_ENABLE      Allow channels to be created based on SSRC transitions during packet flow (this implements RFC 8108 support for multiple RTP streams in a session)
       enable user managed sessions        DS_SESSION_USER_MANAGED         User is responsible for managing sessions and must supply the session handle to any APIs that normally would match packets to sessions internally.  One use case for this is where session termination definitions are all or partially duplicated
       disable network I/O initialization  DS_SESSION_DISABLE_NETIO        Subsequent DSRecv/SendPackets() calls will return errors if this flag is used when creating a session
       preserve incoming sequence numbers  DS_SESSION_PRESERVE_SEQNUM      Preserve RTP sequence numbers from incomming stream when formatting packets for output

     Notes:

       -if no network interface is given, DSBufferPackets, DSGetOrderedPackets, and DSFormatPacket APIs may be used but not DSSendPackets and DSRecvPackets

       -in all cases, DSAssignDataPlane() must be called first to obtain a data plane handle (dpHandle).  This applies in user API mode, background process mode, and for coCPU and DPDK cores.  DSAssignDataPlane()
        is a DirectCore API that handles resource management and molnitoring, including mem allocation, core usage, number of instances, etc

    -DSSessionTranscode() -- transcodes one or more packets based on codecs, ptime, and other pSessionData params specified in DSCreateSession().  Notes:

      -calls DSCodecEncode() and/or DSCodecDecode()
      -calls DSBufferPackets(), DSGetOrderedPackets() and DSFormatPacket() if a packet mode was given in DSCreateSession()
      -calls DSSendPackets() and DSRecvPackets() if a network interface was given in DSCreateSession()

    -DSDeleteSession() - delete an existing session.  Notes:

      -the last session deleted will stop and remove the SigSRF background process (if it was running)
*/

  HSESSION DSCreateSession(HDATAPLANE dpHandle, char* networkIfName, SESSION_DATA* pSessionData, unsigned int uFlags);  /* note -- SESSION_DATA struct defined in shared_include/session.h */

  int DSTranscodeSession(HSESSION sessionHandle, unsigned int uFlags, uint8_t* pkts, unsigned int pkt_buf_len);

  int DSDeleteSession(HSESSION sessionHandle);

/* Packet flow and processing APIs:

    -DSRecvPackets() - receive one or more network packets.  Notes:

      -specifying -1 for sessionHandle receives all available packets for all existing sessions created by DSCreateSession()

      -the DS_RECV_PKT_QUEUE flag indicates that packets will be received from queues used by DSPushPackets()

      -the DS_RECV_PKT_SOCKET_HANDLE flag indicates that sessionHandle specifies a user-defined socket handle, otherwise
       sessionHandle should specify a session handle generated by DSCreateSession(), with the network I/O interface, IP addr,
       and port values as defined in the session

      -default behavior is to return immediately whether or not one or more packets are available.  To block use the DS_RECV_PKT_BLOCK flag

      -if DS_RECV_PKT_ADDTOJITTERBUFFER is given then received packets are also added to the SigSRF internal jitter buffer

      -pkt_buf_len is the maximum size of the buffer pointed to by pkts

      -TODO: add interrupt based operation

      -TODO: add - setup a callback function to be automatically called whenever a packet is available

    -DSSendPackets() - send one or more packets to network sockets or queues used for media service thread or process.  Notes:

      -sessionHandle is a pointer to an array of session handles, of length numPkts

      -pkts points to a buffer containing one or more packets.  Packets are stored consecutively in IP/UDP/RTP format, with no marker, tag or other intermediate values

      -numPkts is the number of packets to send

      -the DS_SEND_PKT_SOCKET_HANDLE flag indicates that sessionHandle specifies a user-defined socket handle, otherwise sessionHandle
       should specify a session handle generated by DSCreateSession(), with the network I/O interface, IP addr, and port values as defined in the session

      -the DS_SEND_PKT_QUEUE flag indicates that packets will be sent to the queue used by DSPullPackets()

      -pkt_len is an array of packet sizes, of length numPkts
 
      -if DS_SEND_PKT_FMT is given then DSFormatPacket() is called

    -DSFormatPacket() - given an RTP payload and an RTP header specifying at least payload type and marker bit, format a network packet for sending.  Notes:

      -the API looks up IP addr and port info using the chnum arg, and generates IP/UDP headers

      -the API increments timestamps and sequence numbers and generates an RTP header

      -chnum can be determined by calling DSGetPacketInfo() with the DS_PKT_INFO_CHNUM flag (see reference source code).  If chnum is -1, then the FormatPkt* arg
       (see next note) must specify *all* information about the packet's headers

      -an optional (i.e. non-NULL) packet header struct pointer (FormatPkt*) can be given to specify IP, UDP, and/or RTP header items (see also the UDPHeader and
       RTPHeader structs).  Also some items that are "header related" are included, such as ptime.  Packet header items are specified with a combination of one or
       more DS_FMT_PKT_USER_xxx attributes given in uFlags (defined below). For a NULL pointer or any items not specified in uFlags, DSFormatPacket() will use
       internal Pktlib values
*/

  int DSRecvPackets(HSESSION sessionHandle, unsigned int uFlags, uint8_t* pkts, unsigned int pkt_buf_len, unsigned int* pkt_len, int numPkts);

  int DSSendPackets(HSESSION* sessionHandle, unsigned int uFlags, uint8_t* pkts, unsigned int* pkt_len, int numPkts);

  int DSFormatPacket(unsigned int chnum, unsigned int uFlags, uint8_t* pyld, unsigned int pyldSize, FormatPkt* formatHdr, uint8_t* pkt);

/* Jitter buffer APIs:

   Common notes:
   
    -if the session handle is -1, then for DSBufferPackets, packets to be added are matched to an existing session via hashing, and for DSGetOrderedPackets all
     available packets for all existing sessions are pulled.  If a specific session handle is given, then no hashing is performed and packets added/pulled apply
     only to that session.  If a user-managed session is active, the session handle cannot be -1 and the correct session handle must be given (hashing is performed
     and includes the session handle)

    -if the pkt_info[] arg is given it will be filled in with information about packets added/pulled to/from the buffer.  See the DS_PKT_PYLD_CONTENT_xxx flags

    -use the DS_PKT_INFO_HOST_BYTE_ORDER flag if input packet headers are in host byte order (for DSBufferPackets) or output packet headerS should be in host byte
     order (for DSGetOrderedPackets).  Network byte order is the default if no flag is given (or the DS_PKT_INFO_NETWORK_BYTE_ORDER flag can be given).
     The byte order flags apply only to headers, not payload contents

    -DSBufferPackets() - add one or more packets to the SigSRF jitter buffer.  Notes:

      -returns the number of packets added.  A zero value can be returned for several reasons, including no packet match (hashing with existing sessions) and the
       packet timestamp is out of the current time window.  Returns -1 for an error condition

      -multiple packets can be added.  On input pkt_buf_len[0] contains the overall number of bytes to process (i.e. max length of pkts[] data), on output
       pkt_buf_len[] contains lengths of all packets found to be correctly formatted, meeting matching criteria, and added to the buffer

      -the DS_GETORD_PKT_FLUSH can be used to force any packets still in the buffer to be output.  Typically this is done prior to closing a session

      -the DS_BUFFER_PKT_RETURN_ALL_DELIVERABLE flag can be to force currently available packets to be delivered regardless of timestamp or sequence number.
       Typically this is done during an open session, for example when a new RTP stream (new SSRC) starts.  This flag should be treated as an "override" or
       "brute force pull" during which some aspects of correct jitter buffer operation may not apply 

      -the API should not be used -- or used very carefully -- if DSRecvPackets() is called with DS_RECV_PKT_ADDTOJITTERBUFFER

    -DSGetOrderedPackets() -- pull one or more packets from the SigSRF jitter buffer that are deliverable in the current time window.  Notes:

      -returns the number of packets pulled.  A zero value can be returned for several reasons, including no packets available in the current time window.  If more
       than one packet is pulled, pkts[] will contain each packet consecutively and the pkt_buf_len[] array will contain the length of each packet.  If there is an
       error condition then -1 is returned

      -if the DS_GETORD_PKT_SESSION flag is given, sessionHandle must specify a currently active session. If the DS_GETORD_PKT_CHNUM flag is given, sessionHandle
       must specify a currently valid channel number (including dynamic channels).  Otherwise sessionHandle is ignored and all packets (for all currently active
       sessions) that are deliverable in the current time window are returned

      -use the DS_GETORD_PKT_FTRT flag if packets were added to the buffer "faster than real-time" (i.e. burst mode, such as adding all packets in a pcap as
       fast as possible).  Applying this flag "updates the time window" every time DSGetOrderedPackets() is called.  Without the flag, the ptime value specified
      in session configuration determines the window update interval

      -use the DS_GET_ORDERED_PKT_ENABLE_DTX flag when DTX handling should be enabled.  Note that in this case there may be more packets output from the buffer
       than input; typically these additional packets contain SID Reuse or SID NoData payloads due to expansion of DTX periods.  When DTX handling is not enabled,
       jitter buffer operation in the presence of DTX packets may vary, for example large time gaps between DTX packets and subsequent packets may cause
       unpredictable results.  Additional notes (i) DTX = discontinuous transmission, (ii) SID frames are used by decoders for CNG (comfort noise generation) audio

      -use the DS_GET_ORDERED_PKT_ENABLE_DTMF flag when DTMF event handling should be enabled.  When enabled, the termN.dtmf_type fields in the session configuration
       determine how incoming DTMF event packets are handled, and whether they are translated through to outgoing packets

      -payload_info must point to a valid unsigned int array when DTX and/or DTMF handling is enabled, otherwise an error condition (-1) is returned

      -uTimestamp should be provided in usec (it's divided internally by 1000 to get msec). If uTimestamp is zero the API will generate its own timestamp, but this may cause timing variations between successive calls depending on intervening processing and its duration
*/

  int DSBufferPackets(HSESSION sessionHandle, unsigned int uFlags, uint8_t* pkts, unsigned int pkt_buf_len[], unsigned int payload_info[], int chnum[]);

  int DSGetOrderedPackets(HSESSION sessionHandle, unsigned int uFlags, uint64_t uTimestamp, uint8_t* pkts, unsigned int pkt_buf_len[], unsigned int payload_info[], unsigned int* uInfo);

  int DSGetJitterBufferInfo(int chnum, unsigned int uFlags);
  int DSSetJitterBufferInfo(int chnum, unsigned int uFlags, int value);


/* Packet and session info APIs 

    -DSGetPacketInfo() - retrieves information about packets.  Notes:

      -if uFlag values of DS_PKT_INFO_SESSION, DS_PKT_INFO_CODEC, DS_PKT_INFO_CODEC_LINK, or DS_PKT_INFO_CHNUM are given, packet headers (plus session handle if user
       managed sessions are active) are used to match an existing session, after which a codec handle or channel number is returned and associated struct data is copied to
       pInfo as a TERMINATION_INFO or SESSION_DATA struct.  If user managed sessions are not active, the sessionHandle arg is ignored

      -if DS_PKT_INFO_RTP_xxx uFlags are given, the corresponding RTP header value is returned from the pkt* arg, using an offset indicated by a DS_BUFFER_PKT_xxx_PACKET
       flag (the default is DS_BUFFER_PKT_IP_PACKET).  The sessionHandle arg is ignored and pInfo is not used

      -use the DS_PKT_INFO_HOST_BYTE_ORDER flag if packet headers are in host byte order.  Network byte order is the default if no flag is given (or the
       DS_PKT_INFO_NETWORK_BYTE_ORDER flag can be given).  The byte order flags apply only to headers, not payload contents

      -if a DS_PKT_INFO_RTP_HEADER uFlag value is given, the input packet RTP header is copied to pInfo as an RTPHeader struct (see above definition) containing separate
       elements for RTP header items 

      -the *pkt argument should point to a packet, and the pktlen argument should contain the length of the packet, in bytes.  If the DS_BUFFER_PKT_IP_PACKET flag is given
       the packet should include IP, UDP, and RTP headers; if the DS_BUFFER_PKT_RTP_PACKET FLAG is given the packet should include only an RTP header
 
    -DSGetSessionInfo() - retrieves information about a session, including (i) terminations defined in session configurations, (ii) channels, including dynamic channels, in use
                          by the session, and (iii) other session info

      -if uFlags specifies DS_SESSION_INFO_HANDLE, a valid session handle must be given.  If uFlags specifies DS_SESSION_INFO_CHNUM, a valid channel number must be given.  Supplying a channel
       number is useful when dyanmic channels are active, in which case new channels can be created after session creation -- at any time during packet flow -- depending on packet contents
       (see discussion of RFC 8108).  Unlike some other APIs, -1 is not allowed as a session handle argument, as no internal session matching based on packet headers is performed

      -use DS_SESSION_INFO_xxx definitions for uFlags to specify which info to return

      -if DS_SESSION_INFO_SESSION is included in uFlags, pInfo should point to a SESSION_DATA struct; otherwise pInfo should point to a TERMINATION_INFO struct.  If pInfo is given as
       NULL, no struct data is copied

      -Term id values are typically 1 or 2 and refer to term1 and term2 session config file definitions (in the future arbitrary N values may be supported).  term_id values can be omitted
       (given as zero) when DS_SESSION_INFO_CHNUM is applied if other uFlags attributes imply a term_id value, for example DS_SESSION_INFO_CODEC_LINK implies term_id = 2
*/

  int DSGetPacketInfo(HSESSION sessionHandle, unsigned int uFlags, uint8_t* pkt, int pktlen, void* pInfo, int*);

  int64_t DSGetSessionInfo(HSESSION sessionHandle, unsigned int uFlags, int64_t term_id, void* pInfo);

  int DSSetSessionInfo(HSESSION sessionHandle, unsigned int uFlags, int64_t term_id, void* pInfo);

/* Additional Voice Processing APIs
   
    -DSConvertFsPacket() - converts sampling rate from one codec to another, taking into account RTP packet info.  Notes:

      -chnum can be determined by calling DSGetPacketInfo() with the DS_PKT_INFO_CHNUM flag (see reference source code)

      -sampling_rate elements in TERMINFO voice attributes are used to determine up or down sampling amount and calculate an *integer ratio* used to multiply the input
       sampling rate.  For example if up  sampling from 16 kHz to 24 kHz, the function will use a multiplier of 3 and a divisor of 2

      -input buffer length (data_len) is in bytes.  If data_len is -1 then sampling_rate and ptime elements within the TERMINFO voice attributes used to create the
       channel (chnum) are referenced to determine input buffer length

      -returns output buffer length in bytes, calculated by multiplying data_len by the conversion ratio

      -pData points to input data and the operation is done in-place.  When up sampling, the buffer must be able to hold the additional output samples.  For example, if
       up sampling from 8 to 16 kHz, the buffer must be 2x the size of the input data

   Note this API is different than DSConvertFs() in Voplib, which performs Fs conversion without knowing about ptime, codec type, etc.
*/

  int DSConvertFsPacket(unsigned int chnum, int16_t* pData, int data_len);


/* DSGetDTMFInfo() -- parse DTMF event packet.  Notes:

  -payload should point to a packet payload, and pyldlen should specify the payload length.  Currently sessionHandle and uFlags are not used, although that may change in the future

  -pkt_info[] values returned by DSGetOrderedPackets() should be checked for a DS_PKT_PYLD_CONTENT_DTMF type, and if found then DSGetDTMFInfo() is called (see mediaTest
   source code examples)

  -on return the dtmf_event struct contains info about the DTMF event.  The dtmf_event struct is defined in shared_include/alarms.h.  -1 is returned for an error condition

  -some notes to keep in mind about DTMF event packets:

     -a variable number of packets may be received for a DTMF event.  Each packet contains event ID, volume, and duration info.  Only the
      last packet of the event should be used for final duration info.  Per RFC 4733, the last packet of the event may be duplicated

     -interval between DTMF packets may not match the ptime being used for media packets, for example voice packets may be arriving at 60
      msec intervals and DTMF packets at 20 msec intervals

     -for all packets within one DTMF event, sequence numbers continue to increment, but timestamps are the same
*/

  int DSGetDTMFInfo(HSESSION sessionHandle, unsigned int uFlags, unsigned char* payload, unsigned int pyldlen, struct dtmf_event* info);
   

/* Get last error condition for a given session.  See error codes defined below, for example DS_BUFFER_PKT_ERROR_xxx */

  int DSGetSessionStatus(HSESSION sessionHandle);


/* pcap and pcapng file usage functions and associated structs

  DSOpenPcapReadHeader() - opens pcap file and reads in pcap file header 

    -does basic verification on magic number and supported link layer types
    -sets pcap file header struct if one is passed in the params
    -returns size of data link layer

  DSReadPcapRecord() - reads in next pcap record

    -skips over data link layer
    -reads and interprets vlan header
    -returns packet data, timestamp, length 
*/

/* pcap and pcapng file format support */

  typedef struct pcap_hdr_s {  /* header for standard libpcap format */

    uint32_t magic_number;   /* magic number */
    uint16_t version_major;  /* major version number */
    uint16_t version_minor;  /* minor version number */
    int32_t  thiszone;       /* GMT to local correction */
    uint32_t sigfigs;        /* accuracy of timestamps */
    uint32_t snaplen;        /* max length of captured packets, in octets */
    uint32_t link_type;      /* data link type */

  } pcap_hdr_t;

  typedef struct pcapng_hdr_s {  /* section header block (SHB) for pcapng format */

    uint32_t magic_number;   /* magic number */
    uint32_t block_length;
    uint32_t byte_order_magic;
    uint16_t version_major;  /* major version number */
    uint16_t version_minor;  /* minor version number */
    int64_t  section_length; /* can be -1 */

  } pcapng_hdr_t;

  typedef struct pcapng_idb_s {  /* interface description block (IDB) for pcapng format */

    uint32_t block_type;
    uint32_t block_length;
    uint16_t link_type;
    uint16_t reserved;
    uint32_t snaplen;

  } pcapng_idb_t;

  /* pcap packet (record) header */

  typedef struct pcaprec_hdr_s {

    uint32_t ts_sec;         /* timestamp seconds */
    uint32_t ts_usec;        /* timestamp microseconds */
    uint32_t incl_len;       /* number of octets of packet saved in file */
    uint32_t orig_len;       /* actual length of packet */

  } pcaprec_hdr_t;

  typedef struct pcapng_epb_s {  /* enhanced packet block (EPB) for pcapng format */

    uint32_t block_type;
    uint32_t block_length;
    uint32_t interface_id;
    uint32_t timestamp_hi;
    uint32_t timestamp_lo;
    uint32_t captured_pkt_len;
    uint32_t original_pkt_len;

  } pcapng_epb_t;

/* pcap record vlan header */

  typedef struct {

    uint16_t vlan_id;
    uint16_t type;

  } vlan_hdr_t;

  #define PCAP_TYPE_LIBPCAP         0
  #define PCAP_TYPE_PCAPNG          1
  #define PCAP_LINK_LAYER_LEN_MASK  0xffff

  int DSOpenPcap(const char* pcap_file, FILE** fp_pcap, pcap_hdr_t* pcap_file_hdr, const char* errstr, unsigned int uFlags);
  int DSReadPcapRecord(FILE* fp_in, uint8_t* pkt_buffer, unsigned int uFlags, pcaprec_hdr_t* pcap_pkt_hdr, int link_layer_length);
  int DSWritePcapRecord(FILE* fp_out, uint8_t* pkt_buffer, pcaprec_hdr_t* pcap_pkt_hdr, struct ether_header* eth_hdr, TERMINATION_INFO* termInfo,  struct timespec* ts, int packet_length);

   void DSSetMarkerBit(FormatPkt* formatPkt, unsigned int uFlags);
   void DSClearMarkerBit(FormatPkt* formatPkt, unsigned int uFlags);

/* DSConfigMediaService() -- start the SigSRF media service as a process or some number of packet/media threads.  Notes:

    -threads[] is an array of thread handles specifying packet/media threads to be acted on (currently handles are indexes, for example, 0 .. 3 specifies packet/media threads 0 through 3).  When the DS_CONFIG_MEDIA_SERVICE_START flag is specified, threads[] can be
     given as NULL in which case thread handles will not be returned to the user application.  Other DS_CONFIG_MEDIA_SERVICE_XXX flags may require threads[] to be non-NULL.  Note that thread handles should not be confused with thread id values, which are determined
     by the OS (typically Linux).  Packet/media thread id values can be determined by calling DSGetThreadInfo()

    -num_threads is the number of thread handles contained in threads

    -uFlags starts, suspends, or exits the thread or process, and also specifies whether the media service should run as a thread or process when the DS_CONFIG_MEDIA_SERVICE_START flag is given (see definitions below)

    *func is a pointer to a thread function.  Ignored if the DS_CONFIG_MEDIA_SERVICE_THREAD flag is not given

    -szCmdLine points to a string containing an optional command line.  If NULL, or uFlags specifies a thread, then cmdLine is ignored

    -the return value is -1 for error conditions, otherwise the the return value is the number of threads acted on
*/

  int DSConfigMediaService(int threads[], int num_threads, unsigned int uFlags, void* (*func)(void*), char* szCmdLine);

  int64_t DSGetThreadInfo(int64_t thread_identifier, unsigned int uFlags, PACKETMEDIATHREADINFO* pInfo);  /* returns information about the packet/media thread specified by handle (by default a thread index, but also can be a pthread_t thread id, see comments) */

#if 0  /* currently not used */
  int DSSetThreadInfo(pthread_t thread, unsigned int uFlags, int64_t param, void* pInfo);
#endif


/* entry function for thread based packet flow and media processing */

  void* packet_flow_media_proc(void* pExecutionMode);


/* DSGetTermChan() gets the channel for hSession's termN endpoint, with optional channel validation checks.  chnum can be NULL if the channel number is not needed and only purpose is validation.  See DS_CHECK_CHAN_xxx flags below */

  int DSGetTermChan(HSESSION hSession, int* chnum, int nTerm, unsigned int uFlags);


/* DSPushPackets() and DSPullPackets() -- send and receive packets from the media service.  Notes:

  -pktsPtr points to one or more pushed or pulled packets stored consecutively in memory

  -for DSPushPackets() numPkts is the number of packets to push. The return value is (i) number of packets pushed, (ii) zero which means queue is full and the push should be re-tried, or (iii) -1 which indicates an error condition

  -DSPullPackets() returns the number of packets pulled. -1 indicates an error condition

  -len is an array of length numPkts containing the length of each packet

  -uFlags can be used to filter packets pulled (see DS_PULLPACKETS_xxx constant definitions)

  -pktInfo is an array of length numPkts where each entry contains info about a packet, as follows:

     -bits 0..15:   type of pulled packet (jitter buffer, transcoded, or stream group)
     -bits 16..31:  codec type; i.e. type of compression used in the RTP payload
     -bits 32..63:  session handle; i.e. handle of the session to which the packet belongs

  -hSession:
  
     -for DSPushPackets(), an array of session handles with a one-to-one relationship with packets pointed to by pktsPtr
     -for DSPullPackets(), a session handle that can be used to filter pullled packets by session.  If -1 is given, then all available packets for all active sessions are pulled, and per-packet session handles are stored in pktInfo[]

  -pkt_buf_len is the maximum amount of buffer space pointed to by pktsPtr that DSPullPackets can write
*/

  int DSPushPackets(unsigned int uFlags, unsigned char* pktsPtr, unsigned int* len, HSESSION* hSession, unsigned int numPkts);
  int DSPullPackets(unsigned int uFlags, unsigned char* pktsPtr, unsigned int* len, HSESSION hSession, uint64_t* pktInfo, unsigned int pkt_buf_len, int numPkts);

  int DSGetDebugInfo(unsigned int uFlags, int, int*, int*);  /* for internal use only */

  int DSDisplayThreadDebugInfo(uint64_t uThreadList, unsigned int uFlags, const char* userstr);

  void DSLogPktTrace(HSESSION, uint8_t* pkt_ptr, int pkt_len, int thread_index, unsigned int uFlags);

/* write full/detailed packet stats history to packet log text file, using either session handle or thread index.  The DS_ENABLE_PACKET_STATS_HISTORY_LOGGING flag must be set in the DEBUG_CONFIG struct uPktStatsLogging item (see shared_include/config.h).  See also DS_WRITE_PKT_STATS_HISTORY_LOG_xx flags below and DS_PKTSTATS_xx flags in diaglib.h */

  int DSWritePacketStatsHistoryLog(HSESSION hSession, unsigned int uFlags, const char* szLogFilename);

  bool DSIsPktStatsHistoryLoggingEnabled(int thread_index);

/* write run-time packet time and loss stats to event log using session handle.  DSConfigPktlib() can be used to set DS_ENABLE_PACKET_TIME_STATS and/or DS_ENABLE_PACKET_LOSS_STATS flags in the DEBUG_CONFIG struct uPktStatsLogging item (see shared_include/config.h) */

  int DSLogPacketTimeLossStats(HSESSION hSession, unsigned int uFlags);

#ifdef __cplusplus
}
#endif

/* Pktlib config API constants; see DSconfigPktlib() API above */

#define DS_CP_GLOBALCONFIG                    0x01
#define DS_CP_DEBUGCONFIG                     0x02
#define DS_CP_INIT                            0x04

/* DSCreateSession() uFlags definitions */

#define DS_SESSION_USER_MANAGED               0x100          /* Session id will be used in hash key, requires user to know which session incomming packets belong to */
#define DS_SESSION_DYN_CHAN_ENABLE            0x200          /* channels will be dyanimcally created for a given session when a new SSRC value is seen on a given channel (implementation follows RFC 8108) */
#define DS_SESSION_DISABLE_NETIO              0x400          /* Disable network I/O initialization, subsequent DSRecv/SendPackets() calls will return errors if this flag is used when creating a session */
#define DS_SESSION_DISABLE_PRESERVE_SEQNUM    0x800          /* Don't preserve RTP sequence number from incomming stream */

#define DS_SESSION_NO_JITTERBUFFER            0x1000

/* DSRecvPackets() uFlags definitions */

#define DS_RECV_PKT_ADDTOJITTERBUFFER         0x1
#define DS_RECV_PKT_SOCKET_HANDLE             0x2
#define DS_RECV_PKT_BLOCK                     0x4
#define DS_RECV_PKT_QUEUE                     0x8
#define DS_RECV_PKT_INIT                      0x10

#define DS_RECV_PKT_FILTER_RTCP               0x100         /* filter RTCP packets */
#define DS_RECV_PKT_QUEUE_COPY                0x200         /* pull packets from the receive queue, but copy, or "look ahead" only, don't advance the receive queue ptr */
#define DS_RECV_PKT_ENABLE_RFC7198_DEDUP      0x400         /* apply RFC7198 packet temporal de-duplication to packets when returning them to the calling app or thread.  Default in the SigSRF packet/media thread is enabled */

/* DSSendPackets() uFlags definitions */

#define DS_SEND_PKT_FMT                       0x1
#define DS_SEND_PKT_SOCKET_HANDLE             0x2
#define DS_SEND_PKT_QUEUE                     0x4
#define DS_SEND_PKT_SUPPRESS_QUEUE_FULL_MSG   0x40000000L


/* DSBufferPackets() and DSGetOrderedPackets() uFlags definitions */

#define DS_BUFFER_PKT_HDR_ONLY                0x1
#define DS_BUFFER_PKT_FULL_PACKET             0x2

#define DS_BUFFER_PKT_IP_PACKET               0x10
#define DS_BUFFER_PKT_UDP_PACKET              0x20
#define DS_BUFFER_PKT_RTP_PACKET              0x40 

#define DS_BUFFER_PKT_HDR_MASK                0xf00000ffL

#define DS_BUFFER_PKT_ALLOW_DYNAMIC_DEPTH     0x1000
#define DS_BUFFER_PKT_DISABLE_PROBATION       0x2000
#define DS_BUFFER_PKT_ALLOW_TIMESTAMP_JUMP    0x4000         /* prevent DSBufferPackets() from purging due to large timestamp jumps, and DSGetOrderedPackets() from returning non-deliverable due to same */ 
#define DS_BUFFER_PKT_ENABLE_RFC7198_DEDUP    0x8000         /* legacy method of handling RFC7198 temporal de-duplication, should not be used unless needed in a specific case.  New method is to apply the DS_RECV_PKT_ENABLE_RFC7198_DEDUP flag to packets being received from a per session queue */
#define DS_BUFFER_PKT_ENABLE_DYNAMIC_ADJUST   0x10000        /* enable dynamic jitter buffer; i.e. target delay is adjusted dynamically based on measured incoming packet delays */

#define DS_GETORD_PKT_SESSION                 0x100
#define DS_GETORD_PKT_CHNUM                   0x200
#define DS_GETORD_PKT_CHNUM_PARENT_ONLY       0x400
#define DS_GETORD_PKT_FTRT                    0x10000        /* faster than real time, advance RTP retrieval timestamp every time DSGetOrderedPackets is called */
#define DS_GETORD_PKT_FLUSH                   0x20000
#define DS_GETORD_PKT_RETURN_ALL_DELIVERABLE  0x40000        /* tell DSGetOrderedPackets() to return any deliverable packets regardless of time window or sequence number */
#define DS_GETORD_PKT_ENABLE_DTX              0x80000        /* enable DTX handling -- note this flag is deprecated, DTX handling should be controlled by the TERM_DTX_ENABLE flag in TERMINATION_INFO struct uFlags element (shared_include/session.h). DS_GETORD_PKT_ENABLE_DTX can still be applied to force DTX handling on a case-by-case basis, but that is not recommended */
#define DS_GETORD_PKT_ENABLE_DTMF             0x100000       /* enable DTMF handling */
#define DS_GETORD_PKT_TIMESTAMP_GAP_RESYNC    0x200000       /* jitter buffer resync on timestamp gaps -- this flag causes DSGetOrderedPackets() to perform a jitter buffer resync for large timestamp gaps. For example if there is a large timestamp gap, the jitter buffer is immediately resync'd and packets are delivered as if there were no gap. This flag is ignored if the DS_GETORD_PKT_RETURN_ALL_DELIVERABLE flag is also specified */
#define DS_GETORD_PKT_ENABLE_SINGLE_PKT_LKAHD 0x400000       /* [Deprecated -- see DS_GETORD_PKT_ENABLE_OOO_HOLDOFF replacement] enable single packet look-ahead -- this flag is reserved for situations where, for some reason, the jitter buffer has only one packet available and is unable to perform comparisons for re-ordering.  Under normal oepration, this does not occur, but the flag is available if such a case should ever arise */
#define DS_GETORD_PKT_ENABLE_SID_REPAIR       0x800000       /* enable SID repair if multiple lost SID packets are detected -- note this flag is deprecated, SID repair should be controlled by the TERM_SID_REPAIR_ENABLE flag in TERMINATION_INFO struct uFlags element (shared_include/session.h). DS_GETORD_PKT_ENABLE_SID_REPAIR can still be applied to force SID repair on a case-by-case basis, but that is not recommended */
#define DS_GETORD_PKT_ADVANCE_TIMESTAMP       0x1000000      /* instructs DSGetOrderedPackets() to advance the specified channels' timestamps by ptime amount, effectively "pulling packets from future time". This is used to help control jitter buffer memory usage in overrun situations. See comments near DSGetOrderedPackets() in packet_flow_media_proc.c */
#define DS_GETORD_PKT_ENABLE_OOO_HOLDOFF      0x2000000      /* enable dynamic holdoff to allow for outlier cases of ooo. Under certain conditions, including low jitter buffer level, a packet "in the future" will be temporarily held for delievery for a short time to see if one or missing packets arrive late. This flag replaces theDS_GETORD_PKT_ENABLE_SINGLE_PKT_LKAHD flag, which is deprecated */

/* flags return by *uInfo param (if uInfo is non NULL) */
 
#define DS_GETORD_PKT_INFO_PULLATTEMPT        0x1            /* a valid pull attempt was made; i.e. there were no errors, timestamp delta was >= ptime, etc */

/* DSGetJitterBufferInfo() and DSSetJitterBufferInfo() uFlags definitions */

#define DS_JITTER_BUFFER_INFO_TARGET_DELAY                0x2
#define DS_JITTER_BUFFER_INFO_MIN_DELAY                   0x3
#define DS_JITTER_BUFFER_INFO_MAX_DELAY                   0x4
#define DS_JITTER_BUFFER_INFO_MAX_DEPTH_PTIMES            0x5
#define DS_JITTER_BUFFER_INFO_UNDERRUN_RESYNC_WARNING     0x6
#define DS_JITTER_BUFFER_INFO_SID_REPAIR                  0x7
#define DS_JITTER_BUFFER_INFO_SID_TIMESTAMP_ALIGN         0x8
#if 0  /* these stats moved internal to pktlib, along with level flush stat, JHB Jun2020 */
#define DS_JITTER_BUFFER_INFO_NUM_PKT_LOSS_FLUSH          0x9
#define DS_JITTER_BUFFER_INFO_NUM_PASTDUE_FLUSH           0xa
#endif
#define DS_JITTER_BUFFER_INFO_SSRC                        0xb
#define DS_JITTER_BUFFER_INFO_MISSING_SEQ_NUM             0xc
#define DS_JITTER_BUFFER_INFO_NUM_INPUT_OOO               0xd
#define DS_JITTER_BUFFER_INFO_MAX_INPUT_OOO               0xe
#define DS_JITTER_BUFFER_INFO_INPUT_PKT_COUNT             0xf
#define DS_JITTER_BUFFER_INFO_OUTPUT_PKT_COUNT            0x10
#define DS_JITTER_BUFFER_INFO_MAX_CONSEC_MISSING_SEQ_NUM  0x11
#define DS_JITTER_BUFFER_INFO_STATS_CALC_PER_PKT          0x12
#define DS_JITTER_BUFFER_INFO_MEDIA_TIMESTAMP_ALIGN       0x13
#define DS_JITTER_BUFFER_INFO_SID_REPAIR_INSTANCE         0x14
#define DS_JITTER_BUFFER_INFO_SID_STATE                   0x15
#define DS_JITTER_BUFFER_INFO_TIMESTAMP_DELTA             0x16
#define DS_JITTER_BUFFER_INFO_NUM_7198_DUPLICATE_PKTS     0x17
#define DS_JITTER_BUFFER_INFO_NUM_PURGES                  0x18
#define DS_JITTER_BUFFER_INFO_NUM_PKTS                    0x19
#define DS_JITTER_BUFFER_INFO_UNDERRUN_RESYNC_COUNT       0x1a
#define DS_JITTER_BUFFER_INFO_OVERRUN_RESYNC_COUNT        0x1b
#define DS_JITTER_BUFFER_INFO_TIMESTAMP_GAP_RESYNC_COUNT  0x1c
#define DS_JITTER_BUFFER_INFO_NUM_OUTPUT_OOO              0x1d
#define DS_JITTER_BUFFER_INFO_MAX_OUTPUT_OOO              0x1e
#define DS_JITTER_BUFFER_INFO_NUM_OUTPUT_DUPLICATE_PKTS   0x1f
#define DS_JITTER_BUFFER_INFO_MAX_NUM_PKTS                0x20
#define DS_JITTER_BUFFER_INFO_MIN_SEQ_NUM                 0x21
#define DS_JITTER_BUFFER_INFO_MAX_SEQ_NUM                 0x22
#define DS_JITTER_BUFFER_INFO_MIN_TIMESTAMP               0x23
#define DS_JITTER_BUFFER_INFO_MAX_TIMESTAMP               0x24
#define DS_JITTER_BUFFER_INFO_TIMESTAMP_SYNC              0x25
#define DS_JITTER_BUFFER_INFO_DELAY                       0x26
#define DS_JITTER_BUFFER_INFO_MAX_TIMESTAMP_GAP           0x27
#define DS_JITTER_BUFFER_INFO_TIMESTAMP_SYNC_OVERRIDE     0x28
#define DS_JITTER_BUFFER_INFO_NUM_OUTPUT_DROP_PKTS        0x29
#define DS_JITTER_BUFFER_INFO_HOLDOFF_COUNT               0x2a
#define DS_JITTER_BUFFER_INFO_NUM_HOLDOFF_ADJUSTS         0x2b
#define DS_JITTER_BUFFER_INFO_NUM_HOLDOFF_DELIVERIES      0x2c
#define DS_JITTER_BUFFER_INFO_CUMULATIVE_TIMESTAMP        0x2d
#define DS_JITTER_BUFFER_INFO_CUMULATIVE_PULLTIME         0x2e

#define DS_JITTER_BUFFER_INFO_ITEM_MASK                   0xff

#define DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING        0x1000  /* reserved */

/* DSGetPacketInfo() uFlags definitions */

#define DS_PKT_INFO_CODEC                     0x1            /* these flags specify struct pointer for pInfo arg, either a TERMINATION_INFO struct or a SESSION_DATA struct */
#define DS_PKT_INFO_CODEC_LINK                0x2
#define DS_PKT_INFO_SESSION                   0x3
#define DS_PKT_INFO_CHNUM                     0x4
#define DS_PKT_INFO_CHNUM_PARENT              0x5
#define DS_PKT_INFO_CODEC_TYPE                0x6
#define DS_PKT_INFO_CODEC_TYPE_LINK           0x7

#define DS_PKT_INFO_INDEX_MASK                0x0f

#define DS_PKT_INFO_RTP_VERSION               0x0100         /* added RTP pkt info items, JHB Jun2017 */
#define DS_PKT_INFO_RTP_PYLDTYPE              0x0200
#define DS_PKT_INFO_RTP_MARKERBIT             0x0300
#define DS_PKT_INFO_RTP_HDROFS                0x0400
#define DS_PKT_INFO_RTP_SEQNUM                0x0800
#define DS_PKT_INFO_RTP_TIMESTAMP             0x0900
#define DS_PKT_INFO_RTP_SSRC                  0x0a00
#define DS_PKT_INFO_RTP_PYLDOFS               0x0b00
#define DS_PKT_INFO_RTP_PYLDLEN               0x0c00
#define DS_PKT_INFO_RTP_PYLDSIZE              DS_PKT_INFO_RTP_PYLDLEN
#define DS_PKT_INFO_RTP_PYLD_CONTENT          0x0d00

#define DS_PKT_INFO_RTP_HEADER                0xff00         /* returns whole RTP header in void* pInfo arg */

#define DS_PKT_INFO_IP_HDRLEN                 0x1000         /* returns length of IP address headers (valid for IPv4 and IPv6) */
#define DS_PKT_INFO_PKTLEN                    0x2000         /* returns total packet length, including IP, UDP, and RTP headers, and payload */
#define DS_PKT_INFO_SRC_UDP_PORT              0x3000
#define DS_PKT_INFO_DST_UDP_PORT              0x4000
#define DS_PKT_INFO_IP_VERSION                0x5000

#define DS_PKT_INFO_ITEM_MASK                 0xff00

#define DS_PKT_INFO_HOST_BYTE_ORDER           0x10000000L    /* use with DSGetPacketInfo(), DSFormatPacket(), DSBufferPackets(), and DSGetOrderedPackets() if packet headers are in host byte order.  The byte order flags apply only to headers, not payload contents */
#define DS_PKT_INFO_NETWORK_BYTE_ORDER        0x20000000L    /* if no flag is specified, network byte order is the default */
#define DS_PKT_INFO_SUPPRESS_ERROR_MSG        0x40000000L    /* suppress any error messages generated by the API */


/* DSGetSessionInfo() and DSSetSessionInfo() uFlags definitions */

#define DS_SESSION_INFO_HANDLE                0x100          /* specifes the sessionHandle argument is treated as a session handle (default) */
#define DS_SESSION_INFO_CHNUM                 0x200          /* specifies the sessionHandle argument should be treated as a channel number.  If combined with DS_SESSION_INFO_HANDLE, DSGeSessionInfo() returns a channel number, depending on the term_id argument */

#define DS_SESSION_INFO_CODEC                 0x1            /* retrieves codec handles: term_id param 0 indicates group codec, 1 = chnum codec (decoder, or term1 if handle is an hSession), 2 = chnum link codec (encoder, or term2 if handle is an hSession) */
#define DS_SESSION_INFO_SAMPLE_RATE           0x3
#define DS_SESSION_INFO_CODEC_TYPE            0x4
#define DS_SESSION_INFO_SESSION               0x5            /* for DS_SESSION_INFO_SESSION and DS_SESSION_INFO_TERM_ID, pInfo should point to a SESSION_DATA struct.  For all other flags, pInfo should point to a TERMINATION_INFO struct */
#define DS_SESSION_INFO_TERM                  0x6            /* get term # and info using session handle or channel number */
#if 0  /* currently not used */
#define DS_SESSION_INFO_CHNUM_QUERY           0x7            /* similar to DS_SESSION_INFO_TERM, except all current sessions are searched for a match, and if there is any inconsistency or duplication, an error is returned */
#endif
#define DS_SESSION_INFO_GROUP_STATUS          0x8
#define DS_SESSION_INFO_GROUP_MODE            0x9
#define DS_SESSION_INFO_UFLAGS                0xa            /* return uFlags applied when session was created */
#define DS_SESSION_INFO_STATE                 0xb            /* get or set current session state (see STATE_xxx flags below).  When setting a session state, only one or more flags should be combined and used in DSSetSessionInfo(), not the session state itself.  Positive value of flags is a set, negative value is a clear */
#define DS_SESSION_INFO_NUM_SESSIONS          0xc            /* get total number of currently active sessions */
#define DS_SESSION_INFO_INPUT_BUFFER_INTERVAL 0xd            /* get buffer add interval of the session */
#define DS_SESSION_INFO_PTIME                 0xe            /* get ptime of the session.  Note that each term (channel) also has its own ptime */
#define DS_SESSION_INFO_GROUP_OWNER           0xf            /* get group owner session.  This is the session that initially defined the stream group ID */
#define DS_SESSION_INFO_GROUP_SAMPLE_RATE     0x11           /* get group term sample rate */
#define DS_SESSION_INFO_THREAD_ID             0x12           /* get id of thread to which session is assigned.  Only applicable if packet_flow_media_proc() is running as one or more threads */
#define DS_SESSION_INFO_CHNUM_PARENT          0x13           /* get chnum of dynamic channel's parent. If param is already a parent returns itself */
#if 0  /* currently not used */
#define DS_SESSION_INFO_GROUP_TIMESTAMP       0x14           /* most recent time group packetization was done */
#endif
#define DS_SESSION_INFO_GROUP_ID              0x15
#define DS_SESSION_INFO_MERGE_BUFFER_SIZE     0x16
#define DS_SESSION_INFO_DELETE_STATUS         0x17
#define DS_SESSION_INFO_THREAD                0x18           /* get index of thread to which session is assigned.  Packet/media thread indexes range from 0..MAX_PKTMEDIA_THREADS-1.  Index 0 always exists, even if all sessions are static (no dynamic sessions) */
#define DS_SESSION_INFO_GROUP_PTIME           0x19
#define DS_SESSION_INFO_OUTPUT_BUFFER_INTERVAL 0x1a          /* get buffer output interval of the session */
#define DS_SESSION_INFO_RTP_PAYLOAD_TYPE      0x1b
#define DS_SESSION_INFO_INPUT_SAMPLE_RATE     0x1c           /* only applicable to codecs for which input and decode sample rates can be different (so far this is only EVS and Opus) */
#define DS_SESSION_INFO_TERM_FLAGS            0x1d           /* get "flags" item from TERMINATION_INFO struct for a given channel number or session + term_id */
#define DS_SESSION_INFO_MAX_LOSS_PTIMES       0x1e           /* get "max_loss_ptimes" item from TERMINATION_INFO struct for a given channel number or session + term_id */
#define DS_SESSION_INFO_DYNAMIC_CHANNELS      0x1f           /* retrieve list of dynamic channels (child channels) for a parent, which can be specified either as a parent channel, or a parent hSession + termN_id */
#define DS_SESSION_INFO_NAME                  0x20           /* retrieve optional session name string, if any has been set */
#define DS_SESSION_INFO_CUR_ACTIVE_CHANNEL    0x21           /* get currently active channel */

#define DS_SESSION_INFO_USE_PKTLIB_SEM        0x20000000L    /* use the pktlib semaphore */
#define DS_SESSION_INFO_SUPPRESS_ERROR_MSG    0x40000000L    /* suppress any error messages generated by the API */

#define DS_SESSION_INFO_ITEM_MASK             0xff

/* flags used with state values returned and set by DSsetSessionInfo() and DSGetSessionInfo() (when used with DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_STATE ) */

#define DS_SESSION_STATE_NEW                  0              /* session states */
#define DS_SESSION_STATE_INIT_STATUS          1

/* actions */

#define DS_SESSION_STATE_FLUSH_PACKETS        0x100          /* flush a session, for example prior to deleting a session, flush all remaining packets from the jitter buffer */
#define DS_SESSION_STATE_WRITE_PKT_LOG        0x200          /* writes packet log for a session.  If the session is a stream group owner, includes all group member sessions in the log */
#define DS_SESSION_STATE_RESET_PKT_LOG        0x400          /* reset internal packet stats counters */

/* other (reserved) */

#define DS_SESSION_DELETE_PENDING             1

/* jitter buffer options handled via DSSetSessionInfo() */

#define DS_SESSION_STATE_ALLOW_TIMSTAMP_JUMP  0x10000        /* instruct the jitter buffer to ignore large jumps in timestamps and sequence numbers, for example due to manual pcap manipulation or multistream packets arriving in alternating chunks between streams */
#define DS_SESSION_STATE_ALLOW_DYNAMIC_ADJUST 0x20000        /* instruct the jitter buffer to adjust target delay dynamically, based on measured incoming packet delays */


/* values returned in pkt_info[] args in DSBufferPackets() and DSGetOrderedPackets(), also returned by call to DSGetPacketInfo() with DS_PKT_INFO_RTP_PYLD_CONTENT */

#define DS_PKT_PYLD_CONTENT_UNKNOWN           0x2000         /* unknown */
#define DS_PKT_PYLD_CONTENT_MEDIA             0x2100         /* compressed voice or video bitstream data, use session->termN.codec_type to know which codec */
#define DS_PKT_PYLD_CONTENT_SID               0x2200         /* SID frame */
#define DS_PKT_PYLD_CONTENT_SID_REUSE         0x2300         /* SID reuse frame (generated by DTX handling) */
#define DS_PKT_PYLD_CONTENT_SID_NODATA        0x2400         /* SID reuse frame (generated by DTX handling) */
#define DS_PKT_PYLD_CONTENT_DTX               0x2500         /* DTX frame, normally same as a SID but not in all cases */
#define DS_PKT_PYLD_CONTENT_RTCP              0x2600         /* RTCP payload */
#define DS_PKT_PYLD_CONTENT_DTMF              0x2700         /* DTMF Event Packet RFC 4733, generic definition */
#define DS_PKT_PYLD_CONTENT_PROBATION         0x2800
#define DS_PKT_PYLD_CONTENT_DTMF_SESSION      0x2900         /* DTMF matching a session-defined DTMF payload type -- note, only returned by DSGetOrderedPackets(), not DSBufferPackets() or DSGetPacketInfo() */

/* can be combined with other DS_PKT_PYLD_CONTENT_xx flags */

#define DS_PKT_PYLD_CONTENT_REPAIR            0x10000        /* indicates packet was repaired, for example a jitter buffer output packet that resulted from media PLC or SID repair */
#define DS_PKT_PYLD_CONTENT_MULTICHAN         0x20000
#define DS_PKT_PYLD_CONTENT_MULTIFRAME        0x40000

#define DS_PKT_PYLD_CONTENT_ITEM_MASK         0xff00

/* DSOpenPcap() definitions */

#define DS_OPEN_PCAP_READ                     DS_READ        /* use filelib definitions */
#define DS_OPEN_PCAP_WRITE                    DS_WRITE
#define DS_OPEN_PCAP_READ_HEADER              0x0100
#define DS_OPEN_PCAP_WRITE_HEADER             0x0200
#define DS_OPEN_PCAP_QUIET                    0x0400
#define DS_READ_PCAP_RECORD_COPY              0x0800         /* copy pcap record only, don't advance file pointer.  Use with DSReadPcapRecord() */

/* DSFormatPacket() definitions */

#define DS_FMT_PKT_SEND                       0x0010

#define DS_FMT_PKT_USER_PYLDTYPE              0x0100
#define DS_FMT_PKT_USER_MARKERBIT             0x0200
#define DS_FMT_PKT_USER_SEQNUM                0x0400
#define DS_FMT_PKT_USER_TIMESTAMP             0x0800
#define DS_FMT_PKT_USER_SSRC                  0x1000
#define DS_FMT_PKT_USER_PTIME                 0x2000
#define DS_FMT_PKT_USER_IPADDR_SRC            0x4000
#define DS_FMT_PKT_USER_IPADDR_DST            0x8000
#define DS_FMT_PKT_USER_UDPPORT_SRC           0x10000
#define DS_FMT_PKT_USER_UDPPORT_DST           0x20000
#define DS_FMT_PKT_DISABLE_IPV4_CHECKSUM      0x40000
#define DS_FMT_PKT_RTP_EVENT                  0x80000
#define DS_FMT_PKT_STANDALONE                 0x100000

#define DS_FMT_PKT_USER_HDRALL                DS_FMT_PKT_USER_IPADDR_SRC | DS_FMT_PKT_USER_IPADDR_DST | DS_FMT_PKT_USER_UDPPORT_SRC | DS_FMT_PKT_USER_UDPPORT_DST

/* DSConfigMediaService() uFlags definitions

  -action flags cannot be combined.  Pktlib internally uses DS_CONFIG_MEDIASERVICE_ACTION_MASK to perform a single action
  -task flags cannot be combined.  Pktlib internally uses DS_CONFIG_MEDIASERVICE_TASK_MASK to act on a single task object (thread, process, or app)
  -session assignment flags (linear, round-robin) can be combined with DS_CONFIG_MEDIASERVICE_START
  -the cmd line flag can be combined with DS_CONFIG_MEDIASERVICE_START (in which case szCmdLine should not be NULL)
*/

#define DS_CONFIG_MEDIASERVICE_START          1              /* start media service threads or process */
#define DS_CONFIG_MEDIASERVICE_SUSPEND        2              /* suspend media service threads or process */
#define DS_CONFIG_MEDIASERVICE_RESUME         3              /* resume media service threads or process */
#define DS_CONFIG_MEDIASERVICE_EXIT           4              /* exit media service threads or process */
#define DS_CONFIG_MEDIASERVICE_THREAD         0x100          /* start media service as one or more threads */
#define DS_CONFIG_MEDIASERVICE_PROCESS        0x200          /* start media service as a process */
#define DS_CONFIG_MEDIASERVICE_APP            0x300          /* start media service as part of the application */
#define DS_CONFIG_MEDIASERVICE_LINEAR         0x10000        /* assign sessions to available threads in linear arrangement (fully utilize one thread before allocating sessions to another thread).  This is the default */
#define DS_CONFIG_MEDIASERVICE_ROUND_ROBIN    0x20000        /* assign sessions to available threads in round-robin arrangement (assign sessions to threads equally) */
#define DS_CONFIG_MEDIASERVICE_CMDLINE        0x40000        /* use the szCmdLine param to specify cmd line arguments to create sessions, read/write pcap files, specify buffer add interval, etc.  Can be combined with thread, process, or app flags */
#define DS_CONFIG_MEDIASERVICE_PIN_THREADS    0x80000
#define DS_CONFIG_MEDIASERVICE_SET_NICENESS   0x100000

#define DS_CONFIG_MEDIASERVICE_ENABLE_THREAD_PROFILING           0x1000000
#define DS_CONFIG_MEDIASERVICE_DISABLE_THREAD_PROFILING          0x1000001

#define DS_CONFIG_MEDIASERVICE_ACTION_MASK    0xf
#define DS_CONFIG_MEDIASERVICE_TASK_MASK      0xf00

#define DS_CONFIG_MEDIASERVICE_GET_THREAD_INFO  0x10000000L

/* available flags for DSGetThreadInfo() */

#define DS_THREAD_INFO_NUM_INPUT_PKT_STATS    1
#define DS_THREAD_INFO_NUM_PULLED_PKT_STATS   2

#define DS_THREAD_INFO_ITEM_MASK              0xff

#define DS_THREAD_INFO_PTHREAD_ID             0x1000         /* specifies the handle param of DSGetThreadInfo() is a pthread_t thread id.  By default the handle param is a thread index (from 0 to N-1, where N is current number of active packet/media threads) */


/* DSPullPackets() definitions, also used by DSSendPackets() */

#define DS_PULLPACKETS_JITTER_BUFFER          0x1000         /* send or pull jitter buffer output packets.  Jitter buffer output packets are re-ordered and DTX expanded, as needed */
#define DS_PULLPACKETS_TRANSCODED             0x2000         /* send or pull transcoded packets.  Transcoded packets are available for each channel of a session after decoding and encoding */
#define DS_PULLPACKETS_STREAM_GROUP           0x4000         /* send or pull stream group packets.  For example, merged packets are available after decoding, audio merging, and encoding */
#define DS_PULLPACKETS_STREAM_GROUPS          DS_PULLPACKETS_STREAM_GROUP
#define DS_PULLPACKETS_GET_QUEUE_STATUS       0x10000
#define DS_PULLPACKETS_GET_QUEUE_LEVEL        0x20000

#if DECLARE_LEGACY_DEFINES
/* legacy define for apps prior to Mar 2019 */
  #define DS_PULL_PACKETS_MERGED DS_PULLPACKETS_STREAM_GROUP
#endif

/* DSPushPackets() definitions (note all are either shared with DSRecvPackets() */

#define DS_PUSHPACKETS_GET_QUEUE_STATUS       0x10000
#define DS_PUSHPACKETS_GET_QUEUE_LEVEL        0x20000
#define DS_PUSHPACKETS_PAUSE_INPUT            0x40000
#define DS_PUSHPACKETS_RESTART_INPUT          0x80000
#define DS_PUSHPACKETS_FULL_PACKET            DS_BUFFER_PKT_FULL_PACKET
#define DS_PUSHPACKETS_IP_PACKET              DS_PUSHPACKETS_FULL_PACKET

#define DS_PUSHPACKETS_ENABLE_RFC7198_DEDUP   DS_RECV_PKT_ENABLE_RFC7198_DEDUP  /* discards duplicate packets and sets a "discarded" bit in return code. Added to support non dynamic call situations such as static session config and regular push intervals, JHB Mar2020 */
#define DS_PUSHPACKETS_INIT                   DS_RECV_PKT_INIT

/* DSGetTermChan() definitions */

#define DS_CHECK_CHAN_DELETE_PENDING          1
#define DS_CHECK_CHAN_EXIST                   2

/* DSWritePacketStatsHistoryLog() flags (note -- can be combined with DS_PKTSTATS_xx flags in diaglib.h) */

#define DS_WRITE_PKT_STATS_HISTORY_LOG_THREAD_INDEX    0x10000000  /* treat hSession param as a thread index (0 .. N-1 where N is number of currently active packet/media threads) */
#define DS_WRITE_PKT_STATS_HISTORY_LOG_RESET_STATS     0x20000000  /* reset packet stats and counters */

/* DSLogPacketTimeLossStats() flags */

#define DS_LOG_PKT_STATS_ORGANIZE_BY_STREAM_GROUP      1
#define DS_LOG_PKT_STATS_SUPPRESS_ERROR_MSG            0x40000000L

/* DSDisplayThreadDebugInfo() flags */

#define DS_DISPLAY_THREAD_DEBUG_INFO_SCREEN_OUTPUT     1
#define DS_DISPLAY_THREAD_DEBUG_INFO_EVENT_LOG_OUTPUT  2

#define MAX_DTDI_STR_LEN                               100

/* error or warning conditions returned by DSGetSessionStatus() */

#define DS_BUFFER_PKT_ERROR_NONE              0
#define DS_BUFFER_PKT_ERROR_DYNCHAN_MISMATCH  -1
#define DS_BUFFER_PKT_ERROR_DYNCHAN_CREATE    -2
#define DS_BUFFER_PKT_ERROR_RTP_VALIDATION    -3
#define DS_BUFFER_PKT_ERROR_SAMPLE_RATE       -4
#define DS_BUFFER_PKT_ERROR_ADD_FAILED        -5
#define DS_BUFFER_PKT_SEQ_DUPLICATE           -6

#if defined(__LIBRARYMODE__) || defined(MEDIATEST_DEV)  /* for Signalogic internal use only.  User apps should not define these !! */
  #define USE_PKTLIB_INLINES
  #define DSGetSessionInfo DSGetSessionInfoInline
  #define DSGetJitterBufferInfo DSGetJitterBufferInfoInline
  #define IsPmThread IsPmThreadInline
#endif

#ifdef USE_PKTLIB_INLINES

#include <semaphore.h>
#include "transcoding.h"
#include "lib_priv.h"

#ifndef _USE_CM_  /* set defines needed for call.h struct alignment */
  #define _USE_CM_
#endif
#ifndef _SIGMOD_RTAFv5_
   #define _SIGMOD_RTAFv5_
#endif
#ifndef _MAVMOD_UAG_
  #define _MAVMOD_UAG_
#endif
#include "call.h"
#include "diaglib.h"
#include "streamlib.h"
#include "rtp_defs.h"
#include "rtp.h"

#ifdef __cplusplus
extern "C" {
#endif

  int get_group_idx(HSESSION, int, bool, const char*);  /* in streamlib.so */

#ifdef __cplusplus
}
#endif

/* inline version of DSGetSessionInfo(), compiled if USE_PKTLIB_INLINES is defined */

/* pktlib.so externs */

extern CHANINFO_CORE ChanInfo_Core[];
extern SESSION_CONTROL sessions[];
extern int nPktMediaThreads;
extern PACKETMEDIATHREADINFO packet_media_thread_info[];

/* function to determine if current thread is an application thread (i.e. pktlib API is being called from a user app, not from a packet/media thread).  Returns true for app threads */
  
static inline bool IsPmThreadInline(HSESSION hSession, int* pThreadIndex) {

int i;
bool fTest, fIsPmThread = false;

   if (hSession >= 0 && !sessions[hSession].threadid) return false;

   for (i=0; i<nPktMediaThreads; i++) {
   
      if (hSession >= 0) fTest = pthread_equal(sessions[hSession].threadid, packet_media_thread_info[i].threadid);  /* compare sessions's p/m thread Id with active p/m thread thread Ids */
      else fTest = pthread_equal(pthread_self(), packet_media_thread_info[i].threadid);  /* compare current thread with p/m thread Id */

      if (fTest) {
   
         fIsPmThread = true;  /* session belongs to a pkt/media thread */
         if (pThreadIndex) *pThreadIndex = i;  /* return thread index if if pointer non-NULL */
         break;
      }
   }

   return fIsPmThread;
}

static inline int64_t DSGetSessionInfoInline(HSESSION sessionHandle, unsigned int uFlags, int64_t term_id, void* pInfo) {

/* more pktlib.so externs */
extern sem_t pktlib_sem;
extern int session_count;

/* streamlib.so externs */
extern STREAM_GROUP stream_groups[];

int n = -1;
int64_t ret_val = -1;
bool no_term_id_arg = false;
int session_id = -1;
int i;
int in_use, delete_status;

   if (sessionHandle < 0 || ((uFlags & DS_SESSION_INFO_HANDLE) && sessionHandle >= MAX_SESSIONS) || sessionHandle >= NCORECHAN) {

      Log_RT(2, "ERROR: DSGetSessionInfo() says invalid session handle or chnum = %d, term_id = %d, uFlags = 0x%x. %s:%d \n", sessionHandle, term_id, uFlags, __FILE__, __LINE__);
      return -2;  /* -2 return code indicates a problem with session handle, chnum, or other param. -1 return code indicates something not found or not available, but otherwise params are ok */
   }

   if (uFlags & DS_SESSION_INFO_HANDLE) {

#ifdef USE_SEMAPHORES_IN_SESSION_INFO
      sem_wait(&pktlib_sem);
#endif
      in_use = sessions[sessionHandle].in_use;
      delete_status = sessions[sessionHandle].delete_status;

#ifdef USE_SEMAPHORES_IN_SESSION_INFO
      sem_post(&pktlib_sem);
#endif

      if (delete_status & DS_SESSION_DELETE_PENDING) {  /* if delete_status is non-zero we know in_use is either 1 (in creation or in deletion) or 2 (fully active) */

         if (!IsPmThread(sessionHandle, NULL)) {  /* note -- error message not printed for p/m threads, which handle special cases during the time sessions are marked for deletion and they are actually deleted */

            if (!(uFlags & DS_SESSION_INFO_SUPPRESS_ERROR_MSG)) Log_RT(2, "ERROR: DSGetSessionInfo() says session %d marked for deletion, term_id = %d, uFlags = 0x%x, %s:%d \n", sessionHandle, term_id, uFlags, __FILE__, __LINE__);

            if ((uFlags & DS_SESSION_INFO_ITEM_MASK) != DS_SESSION_INFO_DELETE_STATUS) return -2;  /* only thing apps are allowed to do after marking a session for deletion */
         }
      }
      else if (in_use != 2) {  /* 2 = fully active */

         if (!(uFlags & DS_SESSION_INFO_SUPPRESS_ERROR_MSG)) Log_RT(2, "ERROR: DSGetSessionInfo() says session %d not active, term_id = %d, uFlags = 0x%x, %s:%d \n", sessionHandle, term_id, uFlags, __FILE__, __LINE__);
         return -2;
      }

      if ((uFlags & DS_SESSION_INFO_ITEM_MASK) == DS_SESSION_INFO_DELETE_STATUS) return sessions[sessionHandle].delete_status;  /* special case -- not handled in switch statement */

      if (term_id == 1)
         n = sessions[sessionHandle].term1;
      else if (term_id == 2)
         n = sessions[sessionHandle].term2;
   }
   else if (uFlags & DS_SESSION_INFO_CHNUM) {

      n = sessionHandle;

      if (term_id < 1 || term_id > 2) no_term_id_arg = true;
   }
   else if (!((uFlags & DS_SESSION_INFO_ITEM_MASK) == DS_SESSION_INFO_NUM_SESSIONS)) {  /* only case where a session handle or chnum is not required */

      Log_RT(2, "ERROR: DSGetSessionInfo() says DS_SESSION_INFO_HANDLE or DS_SESSION_INFO_CHNUM must be given, session handle or chnum = %d, term_id = %d, uFlags = 0x%x, %s:%d \n", sessionHandle, term_id, uFlags, __FILE__, __LINE__);
      return -2;
   } 

   switch (uFlags & DS_SESSION_INFO_ITEM_MASK) {

      case DS_SESSION_INFO_CODEC:

         if (n == -1 && term_id != 0) goto check_n;

         if ((uFlags & DS_SESSION_INFO_HANDLE) && term_id == 0) {  /* group term codec handle */

            ret_val = sessions[sessionHandle].hCodec_group;
         }
         else if (ChanInfo_Core[n].chan_exists) {  /* includes both DS_SESSION_INFO_HANDLE and DS_SESSION_INFO_CHNUM */

            if (term_id == 1) ret_val = ChanInfo_Core[n].hCodec;  /* chnum or term1 codec handle */
            else if (term_id == 2) ret_val = ChanInfo_Core[n].link->hCodec;  /* term2 codec handle */
            else if (term_id == 0) ret_val = sessions[ChanInfo_Core[n].session_id].hCodec_group; /* group term codec handle */
         }

         break;

      case 0:  /* this happens when (i) DS_SESSION_INFO_CHNUM is given by itself, or (ii) DS_SESSION_INFO_HANDLE and DS_SESSION_INFO_CHNUM are combined */
      case DS_SESSION_INFO_CHNUM:  /* not actually used, just here as a marker.  case 0 is used */

         if (n == -1) goto check_n;
         if (ChanInfo_Core[n].chan_exists) ret_val = n;
         if (no_term_id_arg) term_id = 1;
         break;

      case DS_SESSION_INFO_DYNAMIC_CHANNELS:

         if (n == -1) goto check_n;  /* for hSession param term_id has to be 1 or 2 */

         if (uFlags & DS_SESSION_INFO_HANDLE) session_id = sessionHandle;
         else if (uFlags & DS_SESSION_INFO_CHNUM) session_id = ChanInfo_Core[n].session_id;

         if (ChanInfo_Core[n].chan_exists) {

            ret_val = 0;
            int* ch = (int*)pInfo;

            for (i=0; i<sessions[session_id].nDynChans; i++) {

               int chnum_child = sessions[session_id].dyn_chans[i];

               if (ChanInfo_Core[chnum_child].chan_exists && ChanInfo_Core[chnum_child].parent_chnum == n) {  /* if parent = either (i) chnum input param, or (ii) term1/2 of hSession input param, then return dynamic chan info in pInfo, with return value number of dynamic channels found */

                  if (ch) ch[ret_val] = chnum_child;  /* user could just want the number of child channels and pass a NULL param for child chan data */
                  ret_val++;
               }
            }
         }

         if (ret_val >= 0) return ret_val;  /* return directly to avoid standard pInfo handling below */
      
         break;

      case DS_SESSION_INFO_SAMPLE_RATE:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.sample_rate;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.sample_rate;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {
            if (n == -1) goto check_n;
            if (no_term_id_arg) term_id = 1;
            if (term_id == 1 && ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].term->sample_rate;
            else if (term_id == 2 && ChanInfo_Core[n].link && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].link->term->sample_rate;
         }
         break;

      case DS_SESSION_INFO_INPUT_SAMPLE_RATE:  /* only applicable to codecs that allow independent input and decode sample rates (so far this is only EVS and Opus) */

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.input_sample_rate;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.input_sample_rate;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {
            if (n == -1) goto check_n;
            if (no_term_id_arg) term_id = 1;
            if (term_id == 1 && ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].term->input_sample_rate;
            else if (term_id == 2 && ChanInfo_Core[n].link && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].link->term->input_sample_rate;
         }
         break;

      case DS_SESSION_INFO_GROUP_SAMPLE_RATE:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            ret_val = sessions[sessionHandle].session_data.group_term.sample_rate;
         }
         else { n = -2; goto check_n; }

         break;

      case DS_SESSION_INFO_CODEC_TYPE:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.codec_type;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.codec_type;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {
            if (n == -1) goto check_n;
            if (no_term_id_arg) term_id = 1;
            if (term_id == 1 && ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].term->codec_type;
            else if (term_id == 2 && ChanInfo_Core[n].link && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].link->term->codec_type;
         }
         break;
 
      case DS_SESSION_INFO_SESSION:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            session_id = sessionHandle;
            ret_val = sessionHandle;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (ChanInfo_Core[n].chan_exists) {
               session_id = ChanInfo_Core[n].session_id;
               ret_val = ChanInfo_Core[n].session_id;
            }
         }

         break;

      case DS_SESSION_INFO_TERM:

         if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (ChanInfo_Core[n].chan_exists) {

               session_id = ChanInfo_Core[n].session_id;  /* note this handles dynamic channels also, as the session_id is same for both parent and child channels, JHB Feb 2019 */

               if ((int)sessions[session_id].term1 == n) ret_val = 1;
               else if ((int)sessions[session_id].term2 == n) ret_val = 2;

               if (term_id == 0) term_id = ret_val;  /* allow case where user wants to get the term that matches chnum, and also fill in pInfo with that term data */
            }
         }
         else if (uFlags & DS_SESSION_INFO_HANDLE) {

            session_id = sessionHandle;
            ret_val = term_id;
         }
         else if (!no_term_id_arg) ret_val = term_id;

         break;

#if 0
      case DS_SESSION_INFO_CHNUM_QUERY:

         int nFoundCount;

         if (uFlags & DS_SESSION_INFO_CHNUM) {

            nFoundCount = 0;

            for (i=0; i<MAX_SESSIONS; i++) {
 
               if (sessions[i].in_use == 2) {

                  if (sessions[i].term1 == n) {
                     ret_val = 1;
                     nFoundCount++;
                  }
                  else if (sessions[i].term2 == n) {
                     ret_val = 2;
                     nFoundCount++;
                  }
               }
            }

            if (nFoundCount <= 1) break;  /* no error:  not found or one unique channel found */
            else {  /* error:  more than one channel matches, inconsistent channel vs. session database */
               ret_val = -1;
               Log_RT(2, "ERROR: DSGetSessionInfo() says more than one channel matches chnum = %d, uflags = 0x%x, %s:%d \n", n, uFlags, __FILE__, __LINE__);
            }
         }

         break;
#endif

      case DS_SESSION_INFO_GROUP_STATUS:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.group_status;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.group_status;
            else if (term_id == 0) ret_val = sessions[sessionHandle].session_data.group_term.group_status;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (ChanInfo_Core[n].chan_exists) {

               if (term_id == 1) ret_val = ChanInfo_Core[n].term->group_status;
               else if (term_id == 2) ret_val = ChanInfo_Core[n].link->term->group_status;
               else if (term_id == 0) ret_val = sessions[ChanInfo_Core[n].session_id].session_data.group_term.group_status;
            }
         }

         break;

      case DS_SESSION_INFO_GROUP_MODE:

         if (uFlags & DS_SESSION_INFO_HANDLE) {

            if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.group_mode;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.group_mode;
            else if (term_id == 0) ret_val = sessions[sessionHandle].session_data.group_term.group_mode;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (ChanInfo_Core[n].chan_exists) {

               if (term_id == 1) ret_val = ChanInfo_Core[n].term->group_mode;
               else if (term_id == 2) ret_val = ChanInfo_Core[n].link->term->group_mode;
               else if (term_id == 0) ret_val = sessions[ChanInfo_Core[n].session_id].session_data.group_term.group_mode;
            }
         }

         break;

      case DS_SESSION_INFO_GROUP_ID:

         if (uFlags & DS_SESSION_INFO_HANDLE) {

            if (pInfo) {

               if (term_id == 1) strcpy((char*)pInfo, sessions[sessionHandle].session_data.term1.group_id);
               else if (term_id == 2) strcpy((char*)pInfo, sessions[sessionHandle].session_data.term2.group_id);
               else if (term_id == 0) strcpy((char*)pInfo, sessions[sessionHandle].session_data.group_term.group_id);
               ret_val = 1;
            }
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (pInfo) {

               if (n == -1) goto check_n;

               if (ChanInfo_Core[n].chan_exists) {

                  if (term_id == 1) { strcpy((char*)pInfo, ChanInfo_Core[n].term->group_id); ret_val = 1; }
                  else if (term_id == 2) { strcpy((char*)pInfo, ChanInfo_Core[n].link->term->group_id); ret_val = 1; }
                  else if (term_id == 0) { strcpy((char*)pInfo, sessions[ChanInfo_Core[n].session_id].session_data.group_term.group_id); ret_val = 1; }
               }
            }
         }

         if (ret_val >= 0) return ret_val;  /* return directly to avoid standard pInfo handling below */

         break;

      case DS_SESSION_INFO_GROUP_OWNER:

         if (uFlags & DS_SESSION_INFO_HANDLE) {

            bool fUseSem = (uFlags & DS_SESSION_INFO_USE_PKTLIB_SEM) != 0;
            int idx = get_group_idx(sessionHandle, 0, fUseSem, NULL);

            if (idx >= 0) {
               ret_val = stream_groups[idx].owner_session - 1;
               if (fUseSem) sem_post(&pktlib_sem);
            }

            if (idx == -1) {  /* if this session handle is not the group owner, then see if we can find the owner's session handle using term1 group id */

               idx = get_group_idx(sessionHandle, 1, fUseSem, NULL);

               if (idx >= 0) {
                  ret_val = stream_groups[idx].owner_session - 1;
                  if (fUseSem) sem_post(&pktlib_sem);
               }
            }

            if (idx == -1) {  /* if still not found, try term2 group id */

               idx = get_group_idx(sessionHandle, 2, fUseSem, NULL);

               if (idx >= 0) {
                  ret_val = stream_groups[idx].owner_session - 1;
                  if (fUseSem) sem_post(&pktlib_sem);
               }
            }

            return ret_val;  /* we don't use the standard return code here, which will produce error messages.  app code needs to handle the -1 possibility */
         }
         else {

            n = -2;
            goto check_n;
         }
   
         break;

#if 0  /* currently not used */
      case DS_SESSION_INFO_GROUP_TIMESTAMP:

         ret_val = sessions[sessionHandle].session_data.group_term.buffer_depth;
         break;
#endif

      case DS_SESSION_INFO_UFLAGS:

         if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;
            if (ChanInfo_Core[n].chan_exists) session_id = ChanInfo_Core[n].session_id;
         }
         else session_id = sessionHandle;

         ret_val = sessions[session_id].uFlags;
         break;

      case DS_SESSION_INFO_STATE:  /* note -- value of n is a don't care */

         if (uFlags & DS_SESSION_INFO_HANDLE) {

            ret_val = __sync_fetch_and_add(&sessions[sessionHandle].state, 0);
         }
         else { n = -2; goto check_n; }

         break;

      case DS_SESSION_INFO_NUM_SESSIONS:  /* note -- value of n is a don't care */

         if (!term_id) ret_val = session_count;  /* term_id is a thread id */
         else {

            for (i=0; i<nPktMediaThreads; i++) {

               if (packet_media_thread_info[i].threadid == (pthread_t)term_id) {

                  ret_val = packet_media_thread_info[i].numSessions;
                  break;
               }
            }
         }

         break;

      case DS_SESSION_INFO_THREAD_ID:  /* note -- value of n is a don't care */

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            ret_val = sessions[sessionHandle].threadid;
         }
         else { n = -2; goto check_n; }

         break;

      case DS_SESSION_INFO_THREAD:  /* note -- value of n is a don't care */

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            ret_val = sessions[sessionHandle].thread_index;
         }
         else { n = -2; goto check_n; }

         break;

      case DS_SESSION_INFO_CHNUM_PARENT:

         if (uFlags & DS_SESSION_INFO_HANDLE) {

            if (n == -1) goto check_n;
            int nParent = -1;

            for (i=0; i<sessions[sessionHandle].nDynChans; i++) {

               if (sessions[sessionHandle].dyn_chans[i] == n) nParent = ChanInfo_Core[sessions[sessionHandle].dyn_chans[i]].parent_chnum;  /* return parent found */
            }

            if (nParent == -1) ret_val = n; /* return chnum param if chnum has no children */
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;
            if (ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].parent_chnum;
         }
         break;

      case DS_SESSION_INFO_INPUT_BUFFER_INTERVAL:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            if (term_id == 0) ret_val = sessions[sessionHandle].session_data.group_term.input_buffer_interval;
            else if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.input_buffer_interval;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.input_buffer_interval;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (no_term_id_arg) term_id = 1;
            if (term_id == 1 && ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].term->input_buffer_interval;
            else if (term_id == 2 && ChanInfo_Core[n].link && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].link->term->input_buffer_interval;
         }

         break;

      case DS_SESSION_INFO_OUTPUT_BUFFER_INTERVAL:

         if (uFlags & DS_SESSION_INFO_HANDLE) {

            if (term_id == 0) ret_val = sessions[sessionHandle].session_data.group_term.output_buffer_interval;
            else if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.output_buffer_interval;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.output_buffer_interval;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (no_term_id_arg) term_id = 1;
            if (term_id == 1 && ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].term->output_buffer_interval;
            else if (term_id == 2 && ChanInfo_Core[n].link && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].link->term->output_buffer_interval;
         }

         break;

      case DS_SESSION_INFO_TERM_FLAGS:

         if (uFlags & DS_SESSION_INFO_HANDLE) {

            if (term_id == 0) ret_val = sessions[sessionHandle].session_data.group_term.uFlags;
            else if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.uFlags;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.uFlags;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (no_term_id_arg) term_id = 1;
            if (term_id == 1 && ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].term->uFlags;
            else if (term_id == 2 && ChanInfo_Core[n].link && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].link->term->uFlags;
         }

         break;

      case DS_SESSION_INFO_MAX_LOSS_PTIMES:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.max_loss_ptimes;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.max_loss_ptimes;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (no_term_id_arg) term_id = 1;
            if (term_id == 1 && ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].term->max_loss_ptimes;
            else if (term_id == 2 && ChanInfo_Core[n].link && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].link->term->max_loss_ptimes;
         }

         break;

      case DS_SESSION_INFO_RTP_PAYLOAD_TYPE:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.attr.voice_attr.rtp_payload_type;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.attr.voice_attr.rtp_payload_type;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (no_term_id_arg) term_id = 1;
            if (term_id == 1 && ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].term->attr.voice_attr.rtp_payload_type;
            else if (term_id == 2 && ChanInfo_Core[n].link && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].link->term->attr.voice_attr.rtp_payload_type;
         }

         break;

      case DS_SESSION_INFO_PTIME:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.ptime;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.ptime;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (no_term_id_arg) term_id = 1;
            if (term_id == 1 && ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].term->ptime;
            else if (term_id == 2 && ChanInfo_Core[n].link && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].link->term->ptime;
         }

         break;

      case DS_SESSION_INFO_CUR_ACTIVE_CHANNEL:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            ret_val = sessions[sessionHandle].cur_active_chan;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            ret_val = sessions[ChanInfo_Core[n].session_id].cur_active_chan;
         }

         break;

      case DS_SESSION_INFO_NAME:

         if (uFlags & DS_SESSION_INFO_HANDLE) {

            ret_val = strlen(sessions[sessionHandle].session_data.szSessionName);

            if (ret_val >= 0 && pInfo) {

               strcpy((char*)pInfo, sessions[sessionHandle].session_data.szSessionName);
               return ret_val;  /* return directly to avoid standard pInfo handling below */
            }
         }
         else { n = -2; goto check_n; }

         break;

      case DS_SESSION_INFO_GROUP_PTIME:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            ret_val = sessions[sessionHandle].session_data.group_term.ptime;
         }
         else { n = -2; goto check_n; }

         break;

      case DS_SESSION_INFO_MERGE_BUFFER_SIZE:  /* note -- value of n is a don't care */

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            ret_val = sessions[sessionHandle].merge_buffer_size;
         }
         else { n = -2; goto check_n; }

         break;

      default:
         break;
   }

   if (ret_val != -1) {

      if (pInfo != NULL) {

         if ((uFlags & DS_SESSION_INFO_ITEM_MASK) == DS_SESSION_INFO_THREAD) {

            memcpy(pInfo, &packet_media_thread_info[ret_val], sizeof(PACKETMEDIATHREADINFO));  /* copy thread info for this session */
         }
#if 0  /* compiler seems to be buggy when setting these directly, possibly on the link->term item.  We fought a nasty bug fight to figure this out and ended up with explicit pointer math */
         else if (session_id >= 0) *(SESSION_DATA*)pInfo = sessions[session_id].session_data;
         else if (term_id == 1) *(TERMINATION_INFO*)pInfo = *ChanInfo_Core[n].term;
         else if (term_id == 2) *(TERMINATION_INFO*)pInfo = *ChanInfo_Core[n].link->term;
#else
         else if (session_id >= 0) {

            if (term_id == 1) memcpy(pInfo, (uint8_t*)&sessions[session_id].session_data.term1, sizeof(TERMINATION_INFO));
            else if (term_id == 2) memcpy(pInfo, (uint8_t*)&sessions[session_id].session_data.term1 + sizeof(TERMINATION_INFO), sizeof(TERMINATION_INFO));  /* for this hack, see comments below in DSSetSessionInfo(), JHB Aug 2018 */
            else memcpy(pInfo, (uint8_t*)&sessions[session_id].session_data, sizeof(SESSION_DATA));
         }
         else if (term_id == 1) {
            if (n == -1) goto check_n;
             if (ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) memcpy(pInfo, ChanInfo_Core[n].term, sizeof(TERMINATION_INFO));
         }
         else if (term_id == 2) {
            if (n == -1) goto check_n;
            if (ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) memcpy(pInfo, ChanInfo_Core[n].link->term, sizeof(TERMINATION_INFO));
         }
         else if (term_id == 0) {  /* copy group term info when applicable, JHB Feb2020 */

            if (uFlags & DS_SESSION_INFO_HANDLE) session_id = sessionHandle;
            else {
               if (n == -1) goto check_n;
               if (ChanInfo_Core[n].chan_exists) session_id = ChanInfo_Core[n].session_id;
            }

            if (session_id >= 0) memcpy(pInfo, (uint8_t*)&sessions[session_id].session_data.group_term, sizeof(TERMINATION_INFO));
         }
#endif
      }

      return ret_val;
   }
   else {

check_n:

      if (n == -1) {
         if (!(uFlags & DS_SESSION_INFO_SUPPRESS_ERROR_MSG)) Log_RT(2, "ERROR: DSGetSessionInfo() says invalid uFlags -- DS_SESSION_INFO_HANDLE or DS_SESSION_INFO_CHNUM not specified or invalid: flags = 0x%x, sessionHandle = %d, n = %d, term_id = %d, ret_val = %d, %s:%d \n", uFlags, sessionHandle, n, term_id, ret_val, __FILE__, __LINE__);
         return -2;
      }

      if (n == -2) {
         if (!(uFlags & DS_SESSION_INFO_SUPPRESS_ERROR_MSG)) Log_RT(2, "ERROR: DSGetSessionInfo() says DS_SESSION_INFO_CHNUM specified but only DS_SESSION_INFO_HANDLE is allowed: flags = 0x%x, sessionHandle = %d, n = %d, term_id = %d, ret_val = %d, %s:%d \n", uFlags, sessionHandle, n, term_id, ret_val, __FILE__, __LINE__);
         return -2;
      }

      if (!(uFlags & DS_SESSION_INFO_SUPPRESS_ERROR_MSG)) Log_RT(2, "ERROR: DSGetSessionInfo() says invalid term info selected: flags = 0x%x, sessionHandle = %d, n = %d, term_id = %d, ret_val = %d, %s:%d \n", uFlags, sessionHandle, n, term_id, ret_val, __FILE__, __LINE__);
      return -2;
   }
}

static inline int DSGetJitterBufferInfoInline(int chnum, unsigned int uFlags) {

extern RTPCONNECT RTPConnect_Chan[];

bool fAllowPostDelete = (uFlags & DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING) != 0;

bool fChanActive = chnum >= 0 && chnum < NCORECHAN && ChanInfo_Core[chnum].chan_exists && (fAllowPostDelete || !ChanInfo_Core[chnum].delete_pending);

   if (!fChanActive) return -1;

   JITTERBUFFER* JitterBuffer = (JITTERBUFFER*)&RTPConnect_Chan[chnum];

   switch (uFlags & DS_JITTER_BUFFER_INFO_ITEM_MASK) {

      case DS_JITTER_BUFFER_INFO_SSRC:

         return JitterBuffer->SSRC;

   /* configuration info */

      case DS_JITTER_BUFFER_INFO_TARGET_DELAY:

         return ChanInfo_Core[chnum].term->jb_config.target_delay;

      case DS_JITTER_BUFFER_INFO_MIN_DELAY:

         return ChanInfo_Core[chnum].term->jb_config.min_delay;

      case DS_JITTER_BUFFER_INFO_MAX_DELAY:

         return ChanInfo_Core[chnum].term->jb_config.max_delay;

      case DS_JITTER_BUFFER_INFO_MAX_DEPTH_PTIMES:

         return ChanInfo_Core[chnum].term->jb_config.max_depth_ptimes;

   /* packet repair */

      case DS_JITTER_BUFFER_INFO_SID_REPAIR_INSTANCE:

         return JitterBuffer->sid_repair_instance;

      case DS_JITTER_BUFFER_INFO_SID_REPAIR:

         return JitterBuffer->sid_repair_total;

      case DS_JITTER_BUFFER_INFO_SID_TIMESTAMP_ALIGN:

         return JitterBuffer->sid_timestamp_align_total;

      case DS_JITTER_BUFFER_INFO_MEDIA_TIMESTAMP_ALIGN:

         return JitterBuffer->media_timestamp_align_total;

      #if 0
      case DS_JITTER_BUFFER_INFO_NUM_PKT_LOSS_FLUSH:

         return JitterBuffer->pkt_loss_flush_total;

      case DS_JITTER_BUFFER_INFO_NUM_PASTDUE_FLUSH:

         return JitterBuffer->pastdue_flush_total;
      #endif

   /* run-time stats */

      case DS_JITTER_BUFFER_INFO_NUM_INPUT_OOO:

         return JitterBuffer->num_ooo;

      case DS_JITTER_BUFFER_INFO_MAX_INPUT_OOO:

         return JitterBuffer->max_ooo;

      case DS_JITTER_BUFFER_INFO_MISSING_SEQ_NUM:

         return JitterBuffer->total_missing_seq_num;

      case DS_JITTER_BUFFER_INFO_MAX_CONSEC_MISSING_SEQ_NUM:

         return JitterBuffer->max_consec_missing_seq_num;

      case DS_JITTER_BUFFER_INFO_STATS_CALC_PER_PKT:

         return JitterBuffer->num_stats_calcs;

   /* status / count / state info */

      case DS_JITTER_BUFFER_INFO_INPUT_PKT_COUNT:

         return JitterBuffer->total_input_pkt_count;

      case DS_JITTER_BUFFER_INFO_OUTPUT_PKT_COUNT:

         return JitterBuffer->total_output_pkt_count;

      case DS_JITTER_BUFFER_INFO_SID_STATE:

         return ChanInfo_Core[chnum].SID_state;

      case DS_JITTER_BUFFER_INFO_TIMESTAMP_DELTA:

         return JitterBuffer->buffer_timestamp_delta;

      case DS_JITTER_BUFFER_INFO_NUM_7198_DUPLICATE_PKTS:

         return ChanInfo_Core[chnum].num_7198_duplicate_pkts;

      case DS_JITTER_BUFFER_INFO_NUM_PURGES:

         return JitterBuffer->total_num_purges;

      case DS_JITTER_BUFFER_INFO_NUM_PKTS:

         return RTPGetJitterBufferInfo(chnum, DS_JITTER_BUFFER_INFO_NUM_PKTS, NULL);

      case DS_JITTER_BUFFER_INFO_UNDERRUN_RESYNC_COUNT:

         return JitterBuffer->underrun_resync_count;

      case DS_JITTER_BUFFER_INFO_OVERRUN_RESYNC_COUNT:

         return JitterBuffer->overrun_resync_count;

      case DS_JITTER_BUFFER_INFO_TIMESTAMP_GAP_RESYNC_COUNT:

         return JitterBuffer-> timestamp_gap_resync_count;

      case DS_JITTER_BUFFER_INFO_NUM_OUTPUT_DUPLICATE_PKTS:

         return ChanInfo_Core[chnum].num_jb_duplicate_pkts;

      case DS_JITTER_BUFFER_INFO_NUM_OUTPUT_OOO:

         return ChanInfo_Core[chnum].num_jb_ooo_pkts;

      case DS_JITTER_BUFFER_INFO_MAX_OUTPUT_OOO:

         return ChanInfo_Core[chnum].max_jb_ooo;

      case DS_JITTER_BUFFER_INFO_MAX_NUM_PKTS:

         return JitterBuffer->numpkts_max;

      case DS_JITTER_BUFFER_INFO_MIN_SEQ_NUM:

         return RTPGetJitterBufferInfo(chnum, DS_JITTER_BUFFER_INFO_MIN_SEQ_NUM, NULL);

      case DS_JITTER_BUFFER_INFO_MAX_SEQ_NUM:

         return RTPGetJitterBufferInfo(chnum, DS_JITTER_BUFFER_INFO_MAX_SEQ_NUM, NULL);

      case DS_JITTER_BUFFER_INFO_MIN_TIMESTAMP:

         return RTPGetJitterBufferInfo(chnum, DS_JITTER_BUFFER_INFO_MIN_TIMESTAMP, NULL);

      case DS_JITTER_BUFFER_INFO_MAX_TIMESTAMP:
 
         return RTPGetJitterBufferInfo(chnum, DS_JITTER_BUFFER_INFO_MAX_TIMESTAMP, NULL);

      case DS_JITTER_BUFFER_INFO_TIMESTAMP_SYNC:

         return JitterBuffer->TimeStampSync;  /* if this value is zero, jitter buffer is either initialized or has been reset and is waiting to fill to target delay level (waiting to be "primed") */

      case DS_JITTER_BUFFER_INFO_DELAY:

         return RTPGetJitterBufferInfo(chnum, DS_JITTER_BUFFER_INFO_DELAY, NULL);

      case DS_JITTER_BUFFER_INFO_MAX_TIMESTAMP_GAP:

         return JitterBuffer->max_timestamp_gap;

      case DS_JITTER_BUFFER_INFO_TIMESTAMP_SYNC_OVERRIDE:

         return JitterBuffer->TimeStampSync_Override;

      case DS_JITTER_BUFFER_INFO_NUM_OUTPUT_DROP_PKTS:

         return JitterBuffer->num_output_drops;

      case DS_JITTER_BUFFER_INFO_HOLDOFF_COUNT:

         return JitterBuffer->Holdoff_count;

      case DS_JITTER_BUFFER_INFO_NUM_HOLDOFF_ADJUSTS:

         return JitterBuffer->Holdoff_timestampsync_adjustments;

      case DS_JITTER_BUFFER_INFO_NUM_HOLDOFF_DELIVERIES:

         return JitterBuffer->Holdoff_late_deliveries;

      case DS_JITTER_BUFFER_INFO_CUMULATIVE_TIMESTAMP:

         return ChanInfo_Core[chnum].cumulative_timestamp;

      case DS_JITTER_BUFFER_INFO_CUMULATIVE_PULLTIME:

         return ChanInfo_Core[chnum].cumulative_timeDelta;
   }

   return -1;
}

#endif  /* USE_PKTLIB_INLINES */

#endif  /* _PKTLIB_H_ */
