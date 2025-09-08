/*
 $Header: /root/Signalogic/DirectCore/include/pktlib.h

 Copyright (C) Signalogic Inc. 2010-2025

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  Packet flow and streaming management library -- APIs for creating and managing network traffic sessions and for sending/receiving packets to/from Pktlib processing buffers

 Projects

  SigSRF, DirectCore

 Revision History

  Created Mar 2017 Chris Johnson, (some elements copied from legacy ds_vop.h)
  Modified Jun 2017 JHB, added RTP packet info items for use with DSGetPacketInfo()
  Modified Jun 2017 JHB, added DS_BUFFER_PKT_DISABLE_PROBATION, DS_BUFFER_PKT_FLUSH, and DS_BUFFER_PKT_RATECONTROL flags
  Modified Jul 2017 CKJ, added pcap file related structs and functions
  Modified Jul 2017 CKJ, added support for cases where channel numbers in Pktlib do not match up with codec handles in Voplib
  Modified Jul 2017 CKJ, added DS_SESSION_USER_MANAGED flag which includes session id in channel hash key
  Modified Jul 2017 CKJ, added support for user app supplied log file handle and log write 
  Modified Aug 2017 JHB, added FORMAT_PKT struct definition and optional header pointer arg for DSFormatPacket() (replaces pyldType, which is now either looked up in session info using chnum arg, or given in the optional RTP header pointer). Added DS_FMT_PKT_USER_xxx attributes to support use of the FORMAT_PKT* arg
  Modified Aug 2017 JHB, modified DSConvertFsPacket() to take an input data length arg and return amount of valid output data. If the input data length arg is given as -1, the API calculates input data length internally based on channel info specified by chnum
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
  Modified Oct 2018 JHB, change DSGetSessionInfo() return value from int to int64_t, change term_id param in DSGetSessionInfo() and DSSetSessionInfo() from int to int64_t. This supports new session info, including 64-bit thread ID values
  Modified Oct 2018 JHB, add numPkts param to DSPullPackets(), to allow a specific number of packet to be pulled. A -1 value indicates to pull all packets (which was the default operation prior to this mod)
  Modified Nov 2018 JHB, move all stream group and stream merging definitions and APIs to streamlib.h (streamlib.so must be included in mediaTest and mediaMin builds)
  Modified Dec 2018 JHB, add DS_CONFIG_MEDIASERVICE_SET_NICENESS flag to allow control over packet/media thread priority and niceness
  Modified Dec 2018 JHB, add uFlags element to PACKETMEDIATHREADINFO struct
  Modified Jan 2019 JHB, add DSGetThreadInfo()
  Modified Feb 2019 JHB, add profiling time items to PACKETMEDIATHREADINFO struct definition. Items are labeled xxx_max", for example manage_sessions_time_max, input_time_max, decode_time_max, encode_time_max, etc. Packet/media thread profiling can be turned on/off by calling DSConfigMediaService() with the thread index and DS_CONFIG_MEDIASERVICE_ENABLE_THREAD_PROFILING and DS_CONFIG_MEDIASERVICE_DISABLE_THREAD_PROFILING flags
  Modified Feb 2019 JHB, add DS_GETORD_PKT_ENABLE_SID_REPAIR uFlag for DSGetOrderedPackets(). Add DSGetJitterBufferInfo() and DSSetJitterBufferInfo() APIs. Add DS_SESSION_INFO_TERM_FLAGS, DS_SESSION_INFO_MAX_LOSS_PTIMES, and DS_SESSION_INFO_DYNAMIC_CHANNELS uFlags for DSGetSessionInfo()
  Modified Mar 2019 JHB, add DSGetJitterBufferInfo() and DSSetJitterBufferInfo() APIs and associated definitions
  Modified May 2019 JHB, add DS_PKT_PYLD_CONTENT_DTMF_SESSION payload content type, which is returned by DSGetOrderedPackets() when returning DTMF packets matching a session-defined DTMF payload type. Otherwise the generic DS_PKT_PYLD_CONTENT_DTMF content type is returned
  Modified Aug 2019 JHB, removed #ifdef USE_PKTLIB_INLINES around extern C definition of DSGetSessionInfo()
  Modified Aug 2019 JHB, DSPullPackets() hSession param changed from unsigned int to HSESSION
  Modified Sep 2019 JHB, change include folder for if_ether.h from "linux" to "netinet" to fix -Wodr (one definition rule) warning with gcc 5.4
  Modified Oct 2019 JHB, add DS_JITTER_BUFFER_INFO_SID_FILL definition
  Modified Nov 2019 JHB, add DS_JITTER_BUFFER_INFO_NUM_PKT_LOSS_FLUSH and DS_JITTER_BUFFER_INFO_NUM_PASTDUE_FLUSH flags to DSGetJitterBufferInfo()
  Modified Dec 2019 JHB, add DSWritePacketLogStats() API, can be called either with session handle or packet/media thread index
  Modified Dec 2019 JHB, DS_JITTER_BUFFER_INFO_xx flags to support run-time packet stats calculation added to pktlib
  Modified Jan 2020 JHB, add optional chnum param to DSBufferPackets() and DSGetPacketInfo() to return channel numbers of matched packets. This includes parent or child as applicable, and can save time by avoiding subsequent calls to specifically ask for parent or child chnum
  Modified Jan 2020 JHB, implement elapsed time alarm debug option inside DSPushPackets(). See uPushPacketsElapsedTime and DS_ENABLE_PUSHPACKETS_ELAPSED_TIME_ALARM in shared_include/config.h
  Modified Mar 2020 JHB, deprecate DS_GETORD_PKT_ENABLE_DTX and DS_GETORD_PKT_ENABLE_SID_REPAIR flags. Instead of these flags, DTX handling and SID repair should be controlled by TERM_DTX_ENABLE and TERM_SID_REPAIR_ENABLE flags in TERMINATION_INFO struct uFlags element (shared_include/session.h). The deprecated flags can still be applied to force DTX handling / SID repair on a case-by-case basis, but it's not recommended
  Modified Mar 2020 JHB, add DS_JITTER_BUFFER_INFO_TERM_FLAGS and DS_JITTER_BUFFER_INFO_NUM_PURGES flags in DSGetJitterBufferInfo()
  Modified Mar 2020 JHB, implement DS_PUSHPACKETS_ENABLE_RFC7198_DEDUP flag for DSPushPackets(), which is useful for cases without packet timestamps (e.g. static session config or regular interval push rate)
  Modified Apr 2020 JHB, add DS_JITTER_BUFFER_INFO_NUM_xxx_DUPLICATE_PKTS, DS_JITTER_BUFFER_INFO_xxx_RESYNC_COUNT, and DS_JITTER_BUFFER_INFO_NUM_OUTPUT_xxx flags
  Modified Apr 2020 JHB, add DS_GETORD_ADV_TIMESTAMP flag
  Modified May 2020 JHB, add uTimestamp and uInfo params to DSGetOrderedPackets(). See comments
  Modified May 2020 JHB, add DS_JITTER_BUFFER_INFO_NUM_OUTPUT_DROP_PKTS, DS_JITTER_BUFFER_INFO_HOLDOFF_COUNT, and DS_JITTER_BUFFER_INFO_NUM_HOLDOFF_xxx items to DSGetJitterBufferInfo()
  Modified May 2020 JHB, add DS_JITTER_BUFFER_INFO_CUMULATIVE_TIMESTAMP and DS_JITTER_BUFFER_INFO_CUMULATIVE_PULLTIME flags to DSGetJitterBufferInfo()
  Modified May 2020 JHB, move isPmThread() here as static inline from pktlib.c. Define isPmThread as isPmThreadInline
  Modified Oct 2020 JHB, add limited pcapng format capability to DSOpenPcap() and DSReadPcap(). This was mainly done to support TraceWrangler output (pcap anonymizer tool). Support for pcapng format write is not currently planned
  Modified Jan 2021 JHB, implement bit fields in RTPHeader struct for first 2 bytes (see comments), remove DSSet/ClearMarkerBit(), change definition of DS_FMT_PKT_STANDALONE to allow use of DSFormatPacket() with no reference to session / streams created via DSCreateSession()
  Modified Feb 2021 JHB, added DS_PKT_INFO_PYLDLEN option to DSGetPacketInfo()
  Modified Feb 2021 JHB, changed len[] param in DSPushPackets(), DSPullPackets(), DSRecvPackets(), DSSendPackets(), DSGetOrderedPackets(), and DSBufferPackets() from unsigned int* to int*. Any packet length with -1 value should be interpreted as an error condition independent of other packets in the array
  Modified Mar 2021 JHB, change DSLogPacketTimeLossStats() to DSLogRunTimeStats(), add DS_LOG_RUNTIME_STATS_XX flags
  Modified Dec 2021 JHB, add DS_FMT_PKT_TCPIP flag to allow DSFormatPacket() to format/create TCP/IP packets
  Modified Dec 2021 JHB, modify DSOpenPcap to return packet type (based on EtherType). This allows ARP, LLC frames, etc to be differentiated
  Modified Dec 2021 JHB, add DS_OPEN_PCAP_RESET flag to instruct DSOpenPcap() to reset an existing (already open) pcap to start of first packet record
  Modified Sep 2022 JHB, add DS_PKT_INFO_SRC_ADDR and DS_PKT_INFO_DST_ADDR flags, deprecate DS_PKTLIB_NETWORK_BYTE_ORDER flag, see comments
  Modified Oct 2022 JHB, add DS_JITTER_BUFFER_INFO_INPUT_SID_COUNT and DS_JITTER_BUFFER_INFO_PKT_BITRATE_LIST flags to DSGetJitterBufferInfo()
  Modified Dec 2022 JHB, add DS_JITTER_BUFFER_INFO_CURRENT_ALLOCS and DS_JITTER_BUFFER_INFO_MAX_ALLOCS, for tracking jitter buffer mem usage
  Modified Jan 2023 JHB, increase MAX_PKTMEDIA_THREADS to 64, implement DS_CONFIG_MEDIASERVICE_EXIT in DSConfigMediaService()
  Modified Jan 2023 JHB, change DS_PKT_INFO_SUPPRESS_ERROR_MSG to generic DS_PKTLIB_SUPPRESS_ERROR_MSG which is used by DSGetPacketInfo(), DSFormatPacket(), DSBufferPackets(), and DSGetOrderedPackets(). Add DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG flag for additional error/warning message control. For usage examples see mediaMin.cpp
  Modified Jan 2023 JHB, add DSGetPacketInfo() DS_PKT_INFO_RTP_PADDING_SIZE flag
  Modified Apr 2023 JHB, add MAX_TCP_PACKET_LEN definition. In PKTINFO struct, add TCP sequence number and acknowledgement sequence number
  Modified May 2023 JHB, add cur_time param to DSBufferPackets() and DSRecvPackets(), in support of FTRT and AFAP modes
  Modified May 2023 JHB, add DS_SESSION_INFO_RFC7198_LOOKBACK flag to allow retrieval of RFC7198 lookback depth
  Modified Jul 2023 JHB, add DS_JITTER_BUFFER_INFO_NUM_DTMF_PKTS
  Modified Sep 2023 JHB, add DSFilterPacket() and DSFindPcapPacket() APIs, DS_FILTER_PKT_XXX flags, and DS_FIND_PCAP_PACKET_XXX flags
  Modified Nov 2023 JHB, modify pcap_hdr_t struct, add PCAP_TYPE_RTP flag, and update DSOpenPcap() and DSReadPcapRecord() to handle .rtp (.rtpdump) format
  Modified Nov 2023 JHB, add DS_FMT_PKT_USER_UDP_PAYLOAD flag so DSFormatPacket() can handle formatting an input payload that needs no UDP payload modification (e.g. it already includes a full/correct RTP header)
  Modified Nov 2023 JHB, update comments and remove references to "background process"
  Modified Feb 2024 JHB, add DS_SESSION_INFO_LAST_ACTIVE_CHANNEL option for DSGetSessionInfo()
  Modified Feb 2024 JHB, add start and end optional params to DSFindPcapPacket(), add optional amount read param to DSFilterPacket(), add DS_FIND_PCAP_PACKET_USE_SEEK_OFFSET flag (applicable to both DSFindPcapPacket() and DSFilterPacket()) to specify seek based search vs record based and greatly improve performance of these APIs
  Modified Apr 2024 JHB, in DSGetSessionInfo() add DS_SESSION_INFO_SAMPLE_RATE_MULT case, fix issue for DS_SESSION_INFO_OUTPUT_BUFFER_INTERVAL
  Modified Apr 2024 JHB, add DS_PKT_PYLD_CONTENT_MEDIA_REUSE packet payload content definition
  Modified May 2024 JHB, add optional pcap_hdr_t* param to DSReadPcapRecord() API to reference the file header from a prior DSOpenPcap() call. For .rtp format files, DSReadPcapRecord() will use the optional pointer for src/dst IP addr and port values to extent possible when reading RTP records
  Modified May 2024 JHB, define DSGetPacketInfo_t typedef to support alternative DSGetPacketInfo() function signatures in other apps and libs. Honor GET_PKT_INFO_TYPEDEF_ONLY if set by app or lib source to define only typedef for DSGetPacketInfo() and no prototype
  Modified Jun 2024 JHB, add fields to PKTINFO struct: flags, fragment_offset, ip_hdr_checksum, pkt_len_all_fragments, rtp_ssrc, and rtp_seqnum. Define DS_PKT_FRAGMENT_xxx flags, set in PktInfo flags by DSGetPacketInfo() if the packet is a fragment
  Modified Jun 2024 JHB, implement DS_PKT_INFO_PKTINFO_EXCLUDE_RTP flag to allow DSGetPacketInfo() with DS_PKT_INFO_PKTINFO flag to exclude RTP items. Saves time and possibly avoids errors if items needed are non-RTP 
  Modified Jun 2024 JHB, rename DSReadPcapRecord() to DSReadPcap() and DSWritePcapRecord() to DSWritePcap()
  Modified Jun 2024 JHB, define range for SIP ports of 5060 thru 5090 (before it was only 5060 and 5061)
  Modified Jun 2024 JHB, implement packet fragmentation and reassembly. See DS_PKT_INFO_FRAGMENT_xxx, DS_PKT_INFO_REASSEMBLY_xxx, and DS_PKT_INFO_RETURN_xxx DSGetPacketInfo() uFlags definitions; usage example is in mediaMin.cpp
  Modified Jul 2024 JHB, changes required per documentation review: DS_OPEN_PCAP_READ_HEADER and DS_OPEN_PCAP_WRITE_HEADER flags are no longer required in DSOpenPcap() calls, move uFlags to second param in DSReadPcap(), DSGetTermChan(), and DSConfigMediaService(), move pkt_buf_len to fourth param and add uFlags param in DSWritePcap()
  Modified Jul 2024 JHB, modify DSWritePcap() to remove (i) struct timespec* param and instead use the packet record header param timestamp and uFlags definition DS_WRITE_PCAP_SET_TIMESTAMP_WALLCLOCK, and (ii) TERMINATION_INFO* param and instead use IP type in pkt_buf
  Modified Aug 2024 JHB, add DSIsPacketDuplicate() and DS_PKT_DUPLICATE_XXX uFlags, add DSIsReservedUDP(). Both moved here after being initially developed and tested in mediaMin.cpp. Source is in pktlib_RFC791_fragmentation.cpp
  Modified Sep 2024 JHB, rename DS_PULLPACKETS_TRANSCODED to DS_PULLPACKETS_OUTPUT, to reflect expanded type of output packets generated by packet/media threads
  Modified Sep 2024 JHB, implement DS_SESSION_INFO_TERM_FLAGS in DSSetSessionInfo()
  Modified Sep 2024 JHB, rename DS_SESSION_INFO_SAMPLE_RATE_MULT to DS_SESSION_INFO_RTP_CLOCKRATE
  Modified Nov 2024 JHB, add #ifndef MIN_HDR for includes that need minimum definitions, add rtp_version to PKTINFO struct
  Modified Dec 2024 JHB, add DS_PKT_INFO_EXTENDED_SESSION_SEARCH and DS_BUFFER_PKT_EXTENDED_SESSION_SEARCH flags (they are defined to same value). See comments
  Modified Dec 2024 JHB, add DSGetPacketInfo() flags DS_PKT_INFO_USE_IP_HDR_LEN and DS_PKT_INFO_COPY_IP_HDR_LEN_IN_PINFO, these can be used to minimize number of DSGetPacketInfo() calls and overhead per call; see comments
  Modified Jan 2025 JHB, add isRTCPPacket() macro
  Modified Feb 2025 JHB, mods needed to support improved pcapng format handling in pktlib
                         -pktlib improvements include full block type handling for simple packet blocks, IDB blocks, other known blocks (e.g. NRB, interface statistics, etc), and custom (unknown) blocks
                         -add p_block_type param to DSReadPcap()
                         -re-factor pcapng structs, break out pcapng_block_header struct separately. Add struct for simple packet blocks
  Modified Feb 2025 JHB, add DSReadPcap() flags DS_READ_PCAP_DISABLE_NULL_LOOPBACK_PROTOCOL and DS_READ_PCAP_DISABLE_TSO_LENGTH_FIX, see comments
  Modified Mar 2025 JHB, to standardize with other SigSRF libs, adjust values of DS_PKTLIB_SUPPRESS_WARNING_ERROR_MSG, DS_PKTLIB_SUPPRESS_INFO_MSG, and DS_PKTLIB_SUPPRESS_RTP_WARNING_ERROR_MSG flags
  Modified Apr 2025 JHB, add NOMINAL_MTU definition
  Modified Apr 2025 JHB, add DS_PKT_INFO_PINFO_CONTAINS_ETH_PROTOCOL flag to support rudimentary non-IP packet handling in DSGetPacketInfo(). An ethernet protocol can be given in pInfo and this flag applied. For example usage see GetInputData() in mediaMin.cpp
  Modified Apr 2025 JHB, add rtcp_pyld_type field to PKTINFO struct, add isRTCPCustomPacket() macro. See comments
  Modified May 2025 JHB, add DS_PKT_PYLD_CONTENT_IGNORE_PTIME flag
  Modified Jun 2025 JHB, add DS_READ_PCAP_REPORT_TSO_LENGTH_FIX flag
  Modified Jun 2025 JHB, add uPktNumber and szUserMsgString params to DSReadPcap(). See comments
  Modified Aug 2025 JHB, add uTimestamp_mode_record_search to PACKETMEDIAINFO struct. See usage in packet_flow_media_proc()
  Modified Aug 2025 JHB, add uPktNumber param in DSGetPacketInfo()
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
#include "diaglib.h"  /* PKT_STATS struct */

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

#ifndef ETH_P_UBDEBUG  /* evidently not always included in if_ether.h */
  #define ETH_P_UBDEBUG 0x900
#endif

/* define some useful constants either not defined in Linux header files ip.h or netinet/in.h (alternatively they could be defined using sizeof() on struct fields from those header files) */

#define IPV4_ADDR_OFS                   12  /* IP packet byte offsets to start of IP addrs */
#define IPV6_ADDR_OFS                   8

#define IPV4_ADDR_LEN                   4   /* IP address lengths (in bytes) */
#define IPv4_ADDR_LEN                   IPV4_ADDR_LEN
#define IPV6_ADDR_LEN                   16
#define IPv6_ADDR_LEN                   IPV6_ADDR_LEN

/* basic IP, UDP, and RTP header lengths (in bytes) */

#define IPV4_HEADER_LEN                 (IPV4_ADDR_OFS + 2*IPV4_ADDR_LEN)
#define IPv4_HEADER_LEN                 IPV4_HEADER_LEN
#define IPV6_HEADER_LEN                 (IPV6_ADDR_OFS + 2*IPV6_ADDR_LEN)  /* minimum, excluding extensions */
#define IPv6_HEADER_LEN                 IPV6_HEADER_LEN
#define UDP_HEADER_LEN                  8
#define RTP_HEADER_LEN                  12  /* minimum, excluding extensions */

#define MIN_IP_UDP_RTP_HEADER_LEN       (IPV4_HEADER_LEN + UDP_HEADER_LEN + RTP_HEADER_LEN)
#define MAX_IP_UDP_RTP_HEADER_LEN       (IPV6_HEADER_LEN + UDP_HEADER_LEN + RTP_HEADER_LEN)  /* approximate, with header extensions could be a variable amount longer */

/* max RTP packet length, mediaTest has test cases consisting of ptimes up to 240 ms, G711 will require a 1994 byte packet for IPv6. This definition also used in mediaMin */

#define MAX_RTP_PACKET_LEN              (MAX_RAW_FRAME + MAX_IP_UDP_RTP_HEADER_LEN)
#define MAX_TCP_PACKET_LEN              65535

#define NOMINAL_MTU                     1500  /* define an acceptable / reasonable MTU size value */
#define MAX_RTP_PYLD_MTU                (NOMINAL_MTU - MIN_IP_UDP_RTP_HEADER_LEN)  /* a more or less safe guess assuming an MTU size of 1500 */

/* IP protocols */

#ifndef IPv4
  #define IPv4                          4
#endif
#ifndef IPv6
  #define IPv6                          6
#endif
#define UDP_PROTOCOL                    17
#ifndef UDP
  #define UDP UDP_PROTOCOL                    /* define short-hand version if system allows it, JHB Jun 2024 */
#endif
#define TCP_PROTOCOL                    6
#ifndef TCP
  #define TCP TCP_PROTOCOL
#endif
#define ICMP_PROTOCOL                   1
#ifndef ICMP
  #define ICMP ICMP_PROTOCOL
#endif
/* some IPv6 header extension protocols (https://en.wikipedia.org/wiki/List_of_IP_protocol_numbers) */
#define HOPOPT                          0
#define IPv6_Route                      43
#define IPv6_Frag                       44
#define ENCAPSULATING_SECURITY_PAYLOAD  50
#define AUTHENTICATION_HEADER           51
#define ICMPv6                          58
#define IPv6_NoNxt                      59
#define IPv6_Opts                       60
#define VRRP                            112

/* SIP / SDP info ports */

#define SIP_PORT                        5060  /* default SIP message port */
#define SIP_PORT_ENCRYPTED              5061  /* same, encrypted */
#define SIP_PORT_RANGE_LOWER            SIP_PORT
#define SIP_PORT_RANGE_UPPER            5090  /* typical range of SIP ports (https://portforward.com/sip) */
#define SAP_PORT                        9875  /* default UDP port for processing Session Announcement Protocol (SAP) SDP info */

/* misc UDP port numbers */

#define DNS_PORT                        53
#define NetBIOS_PORT                    137   /* also uses 138 */
#define QUIC_PORT                       443
#define DHCPv6_PORT                     547
#define GTP_PORT                        2152  /* GPRS Tunneling Protocol port */
#define PICHAT_PORT                     9009  /* https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml?search=90&page=3 */

/* misc TCP port numbers */

#define MYSQL_PORT                      3306

/* RTCP payload types */

#define RTCP_PYLD_TYPE_MIN              72
#define RTCP_PYLD_TYPE_MAX              82
#define isRTCPPacket(payload_type)      ((payload_type) >= RTCP_PYLD_TYPE_MIN && (payload_type) <= RTCP_PYLD_TYPE_MAX)  /* give this macro 7-bit payload values */

#define RTCP_CUSTOM_PYLD_TYPE_MIN         243
#define RTCP_CUSTOM_PYLD_TYPE_MAX         252
#define isRTCPCustomPacket(payload_type)  ((payload_type) >= RTCP_CUSTOM_PYLD_TYPE_MIN && (payload_type) <= RTCP_CUSTOM_PYLD_TYPE_MAX)  /* give this macro 8-bit payload values */

/* fixed RTP payload types */

#define PCMU_PYLD_TYPE                  0
#define PCMA_PYLD_TYPE                  8
#define L16_PYLD_TYPE                   11

#ifndef MIN_HDR  /* omit remaining definitions if MIN_HDR defined */

/* session and configuration struct and related definitions */

#include "shared_include/session_cmd.h"  /* brings in shared_include/session.h */
#include "shared_include/config.h"

#ifdef __cplusplus
extern "C" {
#endif

  #ifndef _COCPU  /* _COCPU not defined for host or native CPU (x86, PowerPC, ARM, etc) */

/* RTP header struct */

  typedef struct {

    #if 0
    uint16_t  BitFields;       /* Bit fields = Vers:2, pad:1 xind:1 Cc:4 marker:1 pyldtype:7 */
    #else

/* Implemented as bit fields JHB Jan 2021, Notes:

   -this makes all RTP header fields defined in RFC 3550 directly accessible from C/C++ application code and avoids host vs. network byte ordering issues for first 2 bytes of the RTP header
   -bit fields are in lsb order due to gcc limitations, so ordering within each byte is reversed from msb-first layout defined in RFC 3550
*/
 
/* 1st byte of RTP header */
    uint8_t   CC        : 4;    /* CSRC count */
    uint8_t   ExtHeader : 1;    /* Extension header */
    uint8_t   Padding   : 1;    /* Padding */
    uint8_t   Version   : 2;    /* RTP version */
/* 2nd byte of RTP header */
    uint8_t   PyldType  : 7;    /* Payload type */
    uint8_t   Marker    : 1;    /* Marker bit */
    #endif
    uint16_t  Sequence;         /* Sequence number */
    uint32_t  Timestamp;        /* Timestamp */
    uint32_t  SSRC;             /* SSRC */
    uint32_t  CSRC[1];          /* remainder of header */

  } RTPHeader;

/* UDP header struct */

  typedef struct {

    uint16_t  SrcPort;          /* Source Port */
    uint16_t  DstPort;          /* Destination Port */
    uint16_t  UDP_length;       /* Length */
    uint16_t  UDP_checksum;     /* Checksum */

  } UDPHeader;

/* TCP header struct */

  typedef struct {

    uint16_t  SrcPort;          /* Source Port */
    uint16_t  DstPort;          /* Destination Port */
    uint32_t  seq_num;          /* sequence number */
    uint32_t  ack_num;          /* ack number */
    uint16_t  hdr_len_misc;     /* header size and flags */
    uint16_t  window;
    uint16_t  checksum;         /* Checksum */
    uint16_t  urgent;

  } TCPHeader;

/* FORMAT_PKT struct, used in DSFormatPacket() API */

  typedef struct {

    uint8_t    BitFields;       /* Bit fields = Vers:4, Header length:4 */
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
    uint32_t   IP_Version;      /* accepts either IPv4 or IPv6 constants defined below or IPV4 or IPV6 enums defined in shared_include/session.h (both sets of definitions have identical values) */

    UDPHeader  udpHeader;
    RTPHeader  rtpHeader;
    TCPHeader  tcpHeader;       /* used only if DSFormatPacket() uFlags includes DS_FMT_PKT_TCPIP */

    uint16_t   ptime;

  } FORMAT_PKT;

/* struct used for packet fragmentation management linked lists */

  typedef struct PKT_FRAGMENT {

    uint8_t   flags;            /* fragment flags */
    uint16_t  offset;           /* fragment offset */
    uint32_t  identifier;       /* identification field */

  /* 3-way tuple defines the stream connection */

    uint8_t   protocol;
    unsigned __int128 ip_src_addr;  /* 12 bytes left unused for IPv4 */
    unsigned __int128 ip_dst_addr;

  /* saved fragment data */

    uint16_t  ip_hdr_len;       /* IP header length and saved header data (copied from first fragment) */
    uint8_t*  ip_hdr_buf;

    uint16_t  len;              /* fragment length and saved packet data (no IP headers) */
    uint8_t*  pkt_buf;

    struct PKT_FRAGMENT* next;  /* pointer to next fragment */

  } PKT_FRAGMENT;

  /* thread level items */

  #define THREAD_STATS_TIME_MOVING_AVG    16

  #define THREAD_RUN_STATE                0
  #define THREAD_ENERGY_SAVER_STATE       1
  
  typedef struct {  /* per packet/media thread info */

    bool       fMediaThread;
    bool       packet_mode;
    bool       fNoJitterBuffersUsed;
    bool       fProfilingEnabled;
    bool       fPreEmptionMonitorEnabled;

    int        nRealTime;  /* time allowed for pkt/media thread to run and still be in real-time (specified in msec) */
    int        nRealTimeMargin;  /* real-time overhead margin, specified as a percentage of nRealTime (for example 20%) */

    pthread_t  threadid;
    uint32_t   uFlags;
    sem_t      thread_sem;
    bool       thread_sem_init;
    pid_t      niceness;

    int        numSessions;  /* current number of assigned sessions */
    int        numGroups;    /* current number of assigned stream groups */
    int        numSessionsMax;

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
    uint64_t   manage_sessions_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   input_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   buffer_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   chan_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   pull_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   decode_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   encode_time[THREAD_STATS_TIME_MOVING_AVG];
    uint64_t   stream_group_time[THREAD_STATS_TIME_MOVING_AVG];

    uint64_t   CPU_time_max;
    uint64_t   manage_sessions_time_max;
    uint64_t   input_time_max;
    uint64_t   buffer_time_max;
    uint64_t   chan_time_max;
    uint64_t   pull_time_max;
    uint64_t   decode_time_max;
    uint64_t   encode_time_max;
    uint64_t   stream_group_time_max;

    int        num_buffer_packets[THREAD_STATS_TIME_MOVING_AVG];
    int        num_decode_packets[THREAD_STATS_TIME_MOVING_AVG];
    int        num_encode_packets[THREAD_STATS_TIME_MOVING_AVG];
    int        num_stream_group_contributions[THREAD_STATS_TIME_MOVING_AVG];

    uint8_t    thread_stats_time_moving_avg_index;
    uint8_t    manage_sessions_time_index;
    uint8_t    input_time_index;
    uint8_t    buffer_time_index;
    uint8_t    chan_time_index;
    uint8_t    pull_time_index;
    uint8_t    decode_time_index;
    uint8_t    encode_time_index;
    uint8_t    stream_group_time_index;
    uint8_t    uTimestamp_mode_record_search;

  } PACKETMEDIATHREADINFO;

  #define MAX_PKTMEDIA_THREADS         64
  #define NOMINAL_SESSIONS_PER_THREAD  51
  #define NOMINAL_GROUPS_PER_THREAD    17

/* packet stats history items, see comments near USE_CHANNEL_PKT_STATS in packet_flow_media_proc.c */

  #define PKT_STATS_CHUNK_SIZE  10000  /* in bytes */

  typedef struct {

    PKT_STATS*  pkt_stats;  /* pointer to channel's PKT_STATS[] array (defined in diaglib.h) */
    int32_t     mem_usage;  /* current amount of mem usage, in bytes */
    int32_t     num_pkts;   /* channel's current number of pkt stats */

  } PKT_STATS_HISTORY;

  #endif  /* ifndef _COCPU */

/* pktlib version string global var */

  extern const char PKTLIB_VERSION[];

/* DSConfigPktlib() handles basic lib configuration. pGlobalConfig and pDebugConfig point to GLOBAL_CONFIG and DEBUG_CONFIG structs defined in config.h. See DS_CP_xx flags below */

  int DSConfigPktlib(GLOBAL_CONFIG* pGlobalConfig, DEBUG_CONFIG* pDebugConfig, unsigned int uFlags);  /* global config, debug config, or both can be configured depending on attributes specified in uFlags. NULL should be given for either pointer not used */

/* Session APIs:

    -DSCreateSession() - create a session to send and/or receive packets on one of following network interfaces:

       Network Interface                   uFlags

       none [default]                      DS_SESSION_DP_NONE
       server motherboard                  DS_SESSION_DP_LINUX_SOCKETS
       network I/O add-in card             DS_SESSION_DP_DPDK_QUEUE
       coCPU card including network I/O    DS_SESSION_DP_COCPU_QUEUE

      The return value is a session handle (sessionHandle) for use with other APIs. Inputs include:

        network interface -- see above

        uFlags -- see "uFlags" notes given here and in constant definition comments below

        pSessionData -- in the mediaTest source code examples, session data is read from session configuration files. User apps can re-use that method and source code, or implement alternative methods to initialize SESSION_DATA structs. pSessionData cannot be NULL

      Network interface notes:
      
        -when no network interface is specified (default):

          -user applications are responsible for packet I/O and calling DSBufferPackets(), DSGetOrderedPackets(), and other APIs
          -several APIs match sessions to packets using IP and UDP header hashing. If user-managed sessions are active, then the session handle is also included in the hash

        -when a network interface is specified:

          -reserved pm thread [1] uses the DSRecvPackets() and DSSendPackets() APIs for packet I/O, and calls other APIs internally
          -networkIfName examples include:

            -eth0, em1 ...  # motherboard NIC
            -p2p1, p6p1 ... # add-in card or coCPU card

     One each of following operating modes, data flow paths, and timing values should be specified:

       Operating Mode                      uFlags                          Notes

       frames only, no packet headers      DS_SESSION_MODE_FRAME           Currently not supported
       packet flow with IP/UDP/RTP header  DS_SESSION_MODE_IP_PACKET       in shared_include/session_cmd.h
       packet flow with UDP/RTP header     DS_SESSION_MODE_UDP_PACKET

       Data Flow Path                      uFlags                          Notes

       user app APIs                       none [default]                  User application is responsible for packet I/O
       Linux sockets, reserved p/m thread  DS_SESSION_DP_LINUX_SOCKETS     Reserved pm thread [1] handles packet I/O by calling DSRecvPackets(), DSSendPackets(), and other APIs
       DPDK queue                          DS_SESSION_DP_DPDK_QUEUE
       coCPU queue                         DS_SESSION_DP_COCPU_QUEUE

      Timing                               uFlags

       -0 -- none, DSSessionTranscode() or other APIs are called by application code based on a user-defined timing. This should be used with frame mode [default]
       -N -- internal SigSRF timing is used and packet and codec related APIs are explicitly called every N msec

     The following attributes may also be included in uFlags:

       Modes or Attributes                 uFlags                          Notes

       default                             none                            SigSRF internal jitter buffer is enabled
       no jitter buffer                    DS_SESSION_NO_JITTERBUFFER      This applies to reserved pm thread [1] usage only, otherwise user application code decides whether to call jitter buffer APIs
       use codec jitter buffer             DS_SESSION_CODEC_JITTERBUFFER   Only valid for codecs that include a jitter buffer in their formal spec
       enable dynamic channels             DS_SESSION_DYN_CHAN_ENABLE      Allow channels to be created based on SSRC transitions during packet flow (this implements RFC 8108 support for multiple RTP streams in a session)
       enable user managed sessions        DS_SESSION_USER_MANAGED         User is responsible for managing sessions and must supply the session handle to any APIs that normally would match packets to sessions internally. One use case for this is where session termination definitions are all or partially duplicated
       disable network I/O initialization  DS_SESSION_DISABLE_NETIO        Subsequent DSRecv/SendPackets() calls will return errors if this flag is used when creating a session
       preserve incoming sequence numbers  DS_SESSION_PRESERVE_SEQNUM      Preserve RTP sequence numbers from incomming stream when formatting packets for output

     Notes:

       -if no network interface is given, DSBufferPackets, DSGetOrderedPackets, and DSFormatPacket APIs may be used but not DSSendPackets and DSRecvPackets

       -in all cases, DSAssignPlatform() must be called first to obtain a platform handle (dpHandle). This applies to user apps, reserved pm thread [1], and for coCPU and DPDK cores. DSAssignPlatform() is a DirectCore API that handles resource management and monitoring, including mem allocation, core usage, number of instances and users, etc

    -DSSessionTranscode() -- transcodes one or more packets based on codecs, ptime, and other pSessionData params specified in DSCreateSession(). Notes:

      ** THIS API IS CURRENTLY DEPRECATED **
      -calls DSCodecEncode() and/or DSCodecDecode()
      -calls DSBufferPackets(), DSGetOrderedPackets() and DSFormatPacket() if a packet mode was given in DSCreateSession()
      -calls DSSendPackets() and DSRecvPackets() if a network interface was given in DSCreateSession()

    -DSDeleteSession() - delete an existing session


   [1] reserved packet/media thread, see comments in packet_flow_media_proc.c
*/

  HSESSION DSCreateSession(HPLATFORM dpHandle, unsigned int uFlags, char* networkIfName, SESSION_DATA* pSessionData);  /* note -- SESSION_DATA struct defined in shared_include/session.h */

  int DSTranscodeSession(HSESSION sessionHandle, unsigned int uFlags, uint8_t* pkt_buf, unsigned int pkt_buf_len);

  int DSDeleteSession(HSESSION sessionHandle);

/* Packet flow and processing APIs:

    -DSRecvPackets() - receive one or more network packets. Notes:

      -specifying -1 for sessionHandle receives all available packets for all existing sessions created by DSCreateSession()

      -uFlags is one or more DS_RECV_PKT_XXX flags

        -the DS_RECV_PKT_QUEUE flag indicates that packets will be received from queues used by DSPushPackets()

        -the DS_RECV_PKT_SOCKET_HANDLE flag indicates that sessionHandle specifies a user-defined socket handle, otherwise sessionHandle should specify a session handle
         generated by DSCreateSession(), with the network I/O interface, IP addr, and port values as defined in the session

        -default behavior is to return immediately whether or not one or more packets are available. To block use the DS_RECV_PKT_BLOCK flag

        -if DS_RECV_PKT_ADDTOJITTERBUFFER is given then received packets are also added to the SigSRF internal jitter buffer

      -pkt_max_buf_len is the maximum size of the buffer pointed to by pkt_buf

      -TODO: add interrupt based operation

      -TODO: add - setup a callback function to be automatically called whenever a packet is available

    -DSSendPackets() - send one or more packets to network sockets or queues used for media service thread or process. Notes:

      -sessionHandle is a pointer to an array of session handles, of length numPkts

      -pkt_buf[] is an array of buffers containing one or more packets. Packets are stored consecutively in IP/UDP/RTP format, with no marker, tag or other intermediate values

      -pkt_buf_len[] is an array of packet sizes, of length numPkts
 
      -numPkts is the number of packets to send

      -the DS_SEND_PKT_SOCKET_HANDLE flag indicates that sessionHandle specifies a user-defined socket handle, otherwise sessionHandle
       should specify a session handle generated by DSCreateSession(), with the network I/O interface, IP addr, and port values as defined in the session

      -the DS_SEND_PKT_QUEUE flag indicates that packets will be sent to the queue used by DSPullPackets()

      -if DS_SEND_PKT_FMT is given then DSFormatPacket() is called

    -DSFormatPacket() - given an RTP payload and an RTP header specifying at least payload type and marker bit, format a network packet for sending. Notes:

      -the API looks up IP addr and port info using the chnum arg, and generates IP/UDP headers

      -the API increments timestamps and sequence numbers and generates an RTP header

      -chnum can be determined by calling DSGetPacketInfo() with the DS_PKT_INFO_CHNUM flag (see reference source code). If chnum is -1, then the FORMAT_PKT* arg
       (see next note) must specify *all* information about the packet's headers

      -an optional (i.e. non-NULL) packet header struct pointer (FORMAT_PKT*) can be given to specify IP, UDP, and/or RTP header items (see also the UDPHeader and
       RTPHeader structs). Also some items that are "header related" are included, such as ptime. Packet header items are specified with a combination of one or
       more DS_FMT_PKT_USER_xxx attributes given in uFlags (defined below). For a NULL pointer or any items not specified in uFlags, DSFormatPacket() will use
       internal Pktlib values
*/

  int DSRecvPackets(HSESSION hSession, unsigned int uFlags, uint8_t pkt_buf[], int pkt_buf_len[], unsigned int pkt_max_buf_len, int numPkts, uint64_t cur_time);

  int DSSendPackets(HSESSION hSession[], unsigned int uFlags, uint8_t pkt_buf[], int pkt_buf_len[], int numPkts);

  int DSFormatPacket(int chnum, unsigned int uFlags, uint8_t* pyld, unsigned int pyldSize, FORMAT_PKT* formatHdr, uint8_t* pkt_buf);

/* Jitter buffer APIs:

   Common notes:

    -if the session handle is -1, then for DSBufferPackets, packets to be added are matched to an existing session via hashing, and for DSGetOrderedPackets all
     available packets for all existing sessions are pulled. If a specific session handle is given, then no hashing is performed and packets added/pulled apply
     only to that session. If a user-managed session is active, the session handle cannot be -1 and the correct session handle must be given (hashing is performed
     and includes the session handle)

    -if the pkt_info[] arg is given it will be assigned one or more DS_PKT_PYLD_CONTENT_xxx flags describing packets added/pulled to/from the buffer

    -DS_PKTLIB_HOST_BYTE_ORDER indicates that input packet headers (for DSBufferPackets) or output packet headers (for DSGetOrderedPackets) are in host byte order.
     Network byte order is the default if no flag is given (DS_PKTLIB_NETWORK_BYTE_ORDER is given). Byte order flags apply only to headers, not payload contents

    -DSBufferPackets() - add one or more packets to the SigSRF jitter buffer. Notes:

      -returns the number of packets added. A zero value can be returned for several reasons, including no packet match (hashing with existing sessions) and the
       packet timestamp is out of the current time window. Returns -1 for an error condition

      -multiple packets can be added. On input pkt_buf_len[0] contains the overall number of bytes to process (i.e. length of all pkt_buf[] data), on output
       pkt_buf_len[] contains lengths of all packets found to be correctly formatted, meeting matching criteria, and added to the buffer

      -DS_BUFFER_PKT_RETURN_ALL_DELIVERABLE will force currently available packets to be delivered regardless of timestamp or sequence number.
       Typically this is done during an open session, for example when a new RTP stream (new SSRC) starts. This flag should be treated as an "override" or
       "brute force pull" during which some aspects of correct jitter buffer operation may not apply 

      -the API should not be used -- or used very carefully -- if DSRecvPackets() is called with DS_RECV_PKT_ADDTOJITTERBUFFER

    -DSGetOrderedPackets() -- pull one or more packets from the SigSRF jitter buffer that are deliverable in the current time window. Notes:

      -returns the number of packets pulled. A zero value can be returned for several reasons, including no packets available in the current time window. If more
       than one packet is pulled, pkt_buf[] will contain each packet consecutively and the pkt_buf_len[] array will contain the length of each packet. If there is an
       error condition then -1 is returned

      -if DS_GETORD_PKT_SESSION is given, sessionHandle must specify a currently active session. If DS_GETORD_PKT_CHNUM is given, sessionHandle must specify a currently
       valid channel number (including dynamic channels). Otherwise sessionHandle is ignored and all packets (for all currently active sessions) that are deliverable
       in the current time window are returned

      -use DS_GETORD_PKT_ANALYTICS if packets are being added to the buffer without accurate arrival timestamps. Applying this flag updates jitter buffer time windows
       every time DSGetOrderedPackets() is called. Without the flag, the ptime value specified in session configuration determines the window update interval

      -DS_GETORD_PKT_FLUSH forces any packets still in the buffer to be output. Typically this is done prior to closing a session

      -use DS_GET_ORDERED_PKT_ENABLE_DTX when DTX handling should be enabled. Note that in this case there may be more packets output from the buffer
       than input; typically these additional packets contain SID Reuse or SID NoData payloads due to expansion of DTX periods. When DTX handling is not enabled,
       jitter buffer operation in the presence of DTX packets may vary, for example large time gaps between DTX packets and subsequent packets may cause
       unpredictable results. Additional notes (i) DTX = discontinuous transmission, (ii) SID frames are used by decoders for CNG (comfort noise generation) audio

      -use DS_GET_ORDERED_PKT_ENABLE_DTMF when DTMF event handling should be enabled. When enabled, the termN.dtmf_type fields in the session configuration
       determine how incoming DTMF event packets are handled, and whether they are translated through to outgoing packets

      -if pkt_info is NULL when DTX and/or DTMF handling is enabled an error condition (-1) is returned

      -uTimestamp should be provided in usec (it's divided internally by 1000 to get msec). If uTimestamp is zero the API will generate its own timestamp, but this may cause timing variations between successive calls depending on intervening processing and its duration
*/

  int DSBufferPackets(HSESSION sessionHandle, unsigned int uFlags, uint8_t pkt_buf[], int pkt_buf_len[], unsigned int pkt_info[], int chnum[], uint64_t cur_time);

  int DSGetOrderedPackets(HSESSION sessionHandle, unsigned int uFlags, uint64_t uTimestamp, uint8_t pkt_buf[], int pkt_buf_len[], unsigned int pkt_info[], void* pInfo);

  int64_t DSGetJitterBufferInfo(int chnum, unsigned int uFlags);
  int DSSetJitterBufferInfo(int chnum, unsigned int uFlags, int value);

/* Session info APIs

    -DSGetSessionInfo() - retrieves information about a session, including (i) terminations defined in session configurations, (ii) channels, including dynamic channels, in use by the session, and (iii) other session info

      -if uFlags specifies DS_SESSION_INFO_HANDLE, a valid session handle must be given. If uFlags specifies DS_SESSION_INFO_CHNUM, a valid channel number must be given. Supplying a channel number is useful when dyanmic channels are active, in which case new channels can be created after session creation -- at any time during packet flow -- depending on packet contents (see discussion of RFC 8108). Unlike some other APIs, -1 is not allowed as a session handle argument, as no internal session matching based on packet headers is performed

      -use DS_SESSION_INFO_xxx definitions for uFlags to specify which info to return

      -if DS_SESSION_INFO_SESSION is included in uFlags, pInfo should point to a SESSION_DATA struct; otherwise pInfo should point to a TERMINATION_INFO struct. If pInfo is given as NULL, no struct data is copied

      -Term id values are typically 1 or 2 and refer to term1 and term2 session config file definitions (in the future arbitrary N values may be supported). term_id values can be omitted (given as zero) when DS_SESSION_INFO_CHNUM is applied if other uFlags attributes imply a term_id value, for example DS_SESSION_INFO_CODEC_LINK implies term_id = 2

*/

  int64_t DSGetSessionInfo(HSESSION sessionHandle, unsigned int uFlags, int64_t term_id, void* pInfo);

  int DSSetSessionInfo(HSESSION sessionHandle, unsigned int uFlags, int64_t term_id, void* pInfo);

/* Packet info API

    -DSGetPacketInfo() retrieves packet information. Notes:

      -sessionHandle should contain a session handle if uFlags contains a DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flag, which require the packet be verified as matching with sessionHandle as a valid existing session. Otherwise sessionHandle should be set to -1, for any general packet. Some additional sessionHandle notes:

        -if both sessionHandle is -1 and uFlags contains a DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flag, then all existing sessions will be searched

        -SigSRF documentation often refers to "user managed sessions", which implies that user applications will store and maintain session handles created by DSCreateSession()

      -uFlags should contain one DS_BUFFER_PKT_xxx_PACKET flag and one or more DS_PKT_INFO_xxx flags, defined below. If DS_BUFFER_PKT_IP_PACKET is given the packet should start with an IP header; if the DS_BUFFER_PKT_UDP_PACKET or DS_BUFFER_PKT_RTP_PACKET are given the packet should start with a UDP or RTP header (DS_BUFFER_PKT_IP_PACKET is the default if no flag is given). Use the DS_PKT_INFO_HOST_BYTE_ORDER flag if packet headers are in host byte order (network byte order is the default if no flag is given, or DS_PKTLIB_NETWORK_BYTE_ORDER can be given for readability). Byte order flags apply only to headers, not payload contents

      -pkt_buf should point to a packet, and len should contain the length of the packet (size in bytes). If a packet length is unknown, len can be given as -1. Packets may be provided from socket APIs, pcap files, or other sources. The DSOpenPcap() and DSReadPcap() APIs can be used for pcap and pcapng files. The DSFormatPacket() API can be used to help construct a packet

      -if a DS_PKT_INFO_RTP_xxx flag is given in uFlags, the corresponding RTP header item is returned

      -if a DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flag is given, packet headers (plus session handle if user managed sessions are active) are used to match an existing session, after which a codec handle or channel number is returned and associated struct data is copied to pInfo as a TERMINATION_INFO or SESSION_DATA struct if pInfo is not NULL. If non-session-related, general information should be retrieved from the packet, sessionHandle should be given as -1

      -len if given as a positive value specifies packet length (size in bytes) or if combined with the DS_PKT_INFO_USE_IP_HDR_LEN flag specifies IP header length of the packet (i.e. already known from a previous DSGetPacketInfo() call). Typically len is given as -1 to allow DSGetPacketInfo() to calculate the packet's length from its header contents

      -pInfo, if not NULL, will contain:

       -a PKTINFO struct (see definition below) if uFlags includes DS_PKT_INFO_PKTINFO
       -a TERMINATION_INFO or SESSION_DATA struct if uFlags includes a DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flag
       -an RTPHeader struct (see definition above) if uFlags includes DS_PKT_INFO_RTP_HEADER
       -a fully re-assembled packet if uFlags includes DS_PKT_INFO_REASSEMBLY_GET_PACKET

      -chnum, if not NULL, will contain a matching channel number when DS_PKT_INFO_CHNUM or DS_PKT_INFO_CHNUM_PARENT are given. If the packet matches a child channel number and DS_PKT_INFO_CHNUM_PARENT is given, chnum will contain the child channel number and the parent channel number will be returned

      -uPktNumber, if not zero, should give an input or other reference packet number that will be included in warning and error messages, if any

      -return value is < 0 for an error condition, or as specified in the above notes. Note that some RTP items, such as SSRC, may have legitimate values < 0 when interpreted as a 32-bit int

      -both mediaMin.cpp and packet_flow_media_proc.c contain many examples of DSGetPacketInfo() usage
 */

  typedef int (DSGetPacketInfo_t)(HSESSION sessionHandle, unsigned int uFlags, uint8_t* pkt_buf, int len, void* pInfo, int* chnum, unsigned int uPktNumber);

  #ifndef GET_PKT_INFO_TYPEDEF_ONLY  /* app or lib can specify no prototype for DSGetPacketInfo() if it will be doing its own dlsym() lookup, JHB May 2024 */
  DSGetPacketInfo_t DSGetPacketInfo;
  #endif

/* PKTINFO struct filled by DSGetPacketInfo() when uFlags includes DS_PKT_INFO_PKTINFO. A PKTINFO struct param is also used by DSFindPcapPacket() */

  typedef struct {

     uint8_t             version;
     uint8_t             protocol;
     uint8_t             flags;                   /* one or more DS_PKT_FRAGMENT_XXX flags */
     uint16_t            fragment_offset;
     int                 pkt_len;
     int                 ip_hdr_len;              /* IP header size (in bytes), including IPv6 extension headers if any */
     unsigned short int  src_port;
     unsigned short int  dst_port;
     unsigned int        seqnum;                  /* TCP sequence number or UDP/RTP sequence number */
     unsigned int        ack_seqnum;              /* TCP acknowlegement sequence number */
     unsigned int        ip_hdr_checksum;         /* IP header checksum */
     unsigned int        seg_length;              /* TCP segment length */
     int                 pyld_ofs;                /* TCP or UDP offset from start of packet to payload data. For RTP packets, this will be the same value as rtp_pyld_ofs */
     int                 pyld_len;                /* TCP or UDP payload size, excluding UDP header. To include the UDP header add DS_PKT_INFO_PKTINFO_PYLDLEN_INCLUDE_UDP_HDR to uFlags */
     int                 pyld_len_all_fragments;  /* for a UDP packet with MF flag set and no fragment offset, this is the total payload size of all fragments, excluding UDP header */
     unsigned int        udp_checksum;            /* UDP checksum */

  /* RTP items filled for UDP packets. If not a valid RTP packet then RTP items may be undefined. DS_PKT_INFO_PKTINFO_EXCLUDE_RTP can be combined with DS_PKT_INFO_PKTINFO to specify that RTP items should not be processed */

     int                 rtp_hdr_ofs;             /* offset from start of packet to RTP header */
     int                 rtp_hdr_len;
     int                 rtp_pyld_ofs;            /* offset from start of packet to RTP payload data */
     int                 rtp_pyld_len;
     uint8_t             rtp_version;
     uint8_t             rtcp_pyld_type;          /* 8-bit payload type. Can be used with isRTCPCustomPacket() macro */
     uint8_t             rtp_pyld_type;           /* 7-bit payload type */
     int                 rtp_padding_len;
     uint32_t            rtp_timestamp;
     uint32_t            rtp_ssrc;
     uint16_t            rtp_seqnum;

  } PKTINFO;

/* PKTINFO flags definitions */

  #define DS_PKT_FRAGMENT_MF         1            /* set in PKTINFO "flags" if packet MF flag (more fragments) is set */
  #define DS_PKT_FRAGMENT_OFS        2            /* set in PKTINFO "flags" if packet fragment offset is non-zero */
  #define DS_PKT_FRAGMENT_ITEM_MASK  7            /* mask for fragment related flags */

int DSIsPacketDuplicate(unsigned int uFlags, PKTINFO* PktInfo1, PKTINFO* PktInfo2, void* pInfo);

#define DS_PKT_DUPLICATE_PRINT_PKTNUMBER                 0x100  /* debug info printed; pInfo is interpreted as a packet number */
#define DS_PKT_DUPLICATE_INCLUDE_UDP_CHECKSUM            0x200  /* include UDP checksum in duplicate comparison; default is UDP checksum ignored (see comments in pktlib_RFC791_fragmentation.cpp) */

int DSIsReservedUDP(uint16_t port);

int DSPktRemoveFragment(uint8_t* pkt_buf, uint8_t* pFragHdrIPv6, unsigned int uFlags, unsigned int* max_list_fragments);  /* Reserved API: currently undocumented */

/* media processing related APIs:
   
    -DSConvertFsPacket() - converts sampling rate from one codec to another, taking into account RTP packet info. Notes:

      -chnum can be determined by calling DSGetPacketInfo() with DS_PKT_INFO_CHNUM (see reference source code)

      -sampling_rate elements in TERMINFO struct voice attributes are used to determine up or down sampling amount and calculate an *integer ratio* used to multiply the input sampling rate. For example if up  sampling from 16 kHz to 24 kHz, the function will use a multiplier of 3 and a divisor of 2

      -input buffer length (data_len) is in bytes. If data_len is -1 then sampling_rate and ptime elements within the TERMINFO voice attributes used to create the channel (chnum) are referenced to determine input buffer length

      -returns output buffer length in bytes, calculated by multiplying data_len by the conversion ratio

      -pData points to input data and the operation is done in-place. When up sampling, the buffer must be able to hold the additional output samples. For example, if up sampling from 8 to 16 kHz, the buffer must be 2x the size of the input data

   Note this API is different than DSConvertFs() in alglib, which performs Fs conversion without knowing about ptime, codec type, etc
*/

  int DSConvertFsPacket(unsigned int chnum, int16_t* pData, int data_len);


/* DSGetDTMFInfo() -- parse DTMF event packet. Notes:

  -payload should point to a packet payload, and pyldlen should specify the payload length. Currently sessionHandle and uFlags are not used, although that may change in the future

  -pkt_info[] values returned by DSGetOrderedPackets() may be checked for DS_PKT_PYLD_CONTENT_DTMF, and if found then DSGetDTMFInfo() may be called (see mediaTest source code examples)

  -on return the dtmf_event struct contains info about the DTMF event. The dtmf_event struct is defined in shared_include/alarms.h. -1 is returned for an error condition

  -some notes to keep in mind about DTMF event packets:

     -a variable number of packets may be received for a DTMF event. Each packet contains event ID, volume, and duration info. Only the last packet of the event should be used for final duration info. Per RFC 4733, the last packet of the event may be duplicated

     -interval between DTMF packets may not match the ptime being used for media packets, for example voice packets may be arriving at 60 msec intervals and DTMF packets at 20 msec intervals

     -for all packets within one DTMF event, sequence numbers continue to increment, but timestamps are the same
*/

  int DSGetDTMFInfo(HSESSION sessionHandle, unsigned int uFlags, unsigned char* payload, unsigned int pyldlen, struct dtmf_event* info);

/* Get last error condition for a given session. See error codes defined below, for example DS_BUFFER_PKT_ERROR_xxx */

  int DSGetSessionStatus(HSESSION sessionHandle);


/* pcap and pcapng file usage functions and associated structs

  DSOpenPcap() - opens pcap file and reads in pcap file header 

    -does basic verification on magic number and supported link layer types
    -sets pcap file header struct if one is passed in the params
    -returns size of data link layer

  DSReadPcap() - reads in next pcap record

    -skips over data link layer
    -reads and interprets vlan header
    -returns packet data, timestamp, length 
*/

/* pcap, pcapng, and .rtp file format support */

  typedef struct pcap_hdr_s {  /* header for standard libpcap format, also for .rtp (.rtpdump) format */

    union {

      struct {  /* pcap format, the default */

        uint32_t magic_number;   /* magic number */
        uint16_t version_major;  /* major version number */
        uint16_t version_minor;  /* minor version number */
        int32_t  thiszone;       /* GMT to local correction */
        uint32_t sigfigs;        /* accuracy of timestamps */
        uint32_t snaplen;        /* max length of captured packets, in octets */
        uint32_t link_type;      /* data link type */
      };

      struct {  /* include rtp format as a union (https://formats.kaitai.io/rtpdump), JHB Nov 2023 */

        char     shebang[12];
        char     space[1];
        char     dst_ip_addr[128];  /* run-time strings have terminator values 47 and 10, we declare more than needed */
        char     dst_port[128];
        uint32_t start_sec;
        uint32_t start_usec;
        uint32_t src_ip_addr;
        uint16_t src_port;
        uint16_t padding;
      } rtp;
    };

  } pcap_hdr_t;

  #define SIZEOF_PCAP_HDR_T (offsetof(pcap_hdr_t, link_type) + sizeof_field(pcap_hdr_t, link_type))  /* define size of pcap part of pcap_hdr_t. This definition would also work without the union, JHB Nov 2023. sizeof_field() is defined in <stddef.h> or include/alias.h, JHB Sep 2024 */

  typedef struct pcaprec_hdr_s {  /* pcap packet (record) header */

    uint32_t ts_sec;         /* timestamp seconds */
    uint32_t ts_usec;        /* timestamp microseconds */
    uint32_t incl_len;       /* number of octets of packet record */
    uint32_t orig_len;       /* actual length of packet */

  } pcaprec_hdr_t;

  typedef struct pcapng_block_header_s {  /* basic header present in all pcapng block types */

    union {
      uint32_t block_type;
      uint32_t magic_number;  /* for section header blocks (SHBs) the block type is 0x0a0d0d0a, aka pcapng file magic number */
    };

    uint32_t block_length;

  } pcapng_block_header_t;

  typedef struct pcapng_hdr_s {  /* pcapng format section header block (SHB). pcapng files can have multiple SHBs, so the first SHB is effectively the pcapng file header */

    struct pcapng_block_header_s block_header;

    uint32_t byte_order_magic;
    uint16_t version_major;   /* major version number */
    uint16_t version_minor;   /* minor version number */
    int64_t  section_length;  /* can be -1 */

  } pcapng_hdr_t;

  typedef struct pcapng_idb_s {  /* pcapng format interface description block (IDB) */

    struct pcapng_block_header_s block_header;

    uint16_t link_type;
    uint16_t reserved;
    uint32_t snaplen;

  } pcapng_idb_t;

  typedef struct pcapng_spb_s {  /* pcapng format simple packet block */
  
    struct pcapng_block_header_s block_header;

    uint32_t original_pkt_len;

  } pcapng_spb_t;

  typedef struct pcapng_epb_s {  /* pcapng format enhanced packet block (EPB) */

    struct pcapng_block_header_s block_header;

    uint32_t interface_id;
    uint32_t timestamp_hi;
    uint32_t timestamp_lo;
    uint32_t captured_pkt_len;
    uint32_t original_pkt_len;

  } pcapng_epb_t;

  typedef struct {  /* pcap record vlan header */

    uint16_t id;
    uint16_t type;

  } vlan_hdr_t;

/* definitions for block_type in above structs */

  #define PCAP_PB_TYPE     0x7ff0  /* standard packet blocks for pcap and .rtpxxx files - these are not in any spec, we need values that should not conflict with the pcapng spec */
  #define RTP_PB_TYPE      0x7ff1
  #define PCAPNG_EPB_TYPE  6       /* pcapng enhanced block type, note this is the default block type containing IP/UDP/RTP data */
  #define PCAPNG_SPB_TYPE  3       /* pcapng simple block type, IETF spec (https://www.ietf.org/archive/id/draft-tuexen-opsawg-pcapng-03.html) section 4.4 explains "This block is preferred to the standard Enhanced Packet Block when performance or space occupation are critical factors, such as in sustained traffic capture applications" */
  #define PCAPNG_IDB_TYPE  1       /* pcapng interface description block */
  #define PCAPNG_NRB_TYPE  4       /* pcapng name resolution block */

/* definitions used by pcap APIs. Notes:

  -PCAP_TYPE_LIBPCAP and PCAP_TYPE_PCAPNG are returned by DSOpenPcap() in upper 16 bits of return value, depending on file type discovered
  -PCAP_TYPE_BER and PCAP_TYPE_HI3 are used by mediaMin for intermediate packet output
*/

  #define PCAP_TYPE_LIBPCAP                                  0
  #define PCAP_TYPE_PCAPNG                                   1
  #define PCAP_TYPE_BER                                      2
  #define PCAP_TYPE_HI3                                      3
  #define PCAP_TYPE_RTP                                      4

  #define PCAP_LINK_LAYER_LEN_MASK                      0xffff  /* return value of DSOpenPcap() contains link type in bits 27-20, file type in bits 19-16, and link layer length in lower 16 bits */
  #define PCAP_LINK_LAYER_FILE_TYPE_MASK              0x0f0000
  #define PCAP_LINK_LAYER_LINK_TYPE_MASK            0x0ff00000

  #ifndef LINKTYPE_ETHERNET  /* define pcap file link types if needed. We don't require libpcap to be installed */

    #define LINKTYPE_ETHERNET                                1  /* standard Ethernet Link Layer */
    #define LINKTYPE_LINUX_SLL                             113  /* Linux "cooked" capture encapsulation */
    #define LINKTYPE_RAW_BSD                                12  /* Raw IP, OpenBSD compatibility value */
    #define LINKTYPE_RAW                                   101  /* Raw IP */
    #define LINKTYPE_IPV4                                  228  /* Raw IPv4 */
    #define LINKTYPE_IPV6                                  229  /* Raw IPv6 */
  #endif

  #define LINKTYPE_LINUX_SLL_LINK_LEN                       16

/* DSOpenPcap() opens a pcap, pcapng, or rtp/rtpdump file and fills in a pcap_hdr_t struct (above). Notes:

   -reads the file header(s) and leaves fp_pcap pointing at the first pcap record, and returns a filled pcap_hdr_t struct pointed to by pcap_file_hdr
   -pErrstr is optional; if used it should point to an error information string to be included in warning or error messages. NULL indicates not used
   -uFlags are given in DS_OPEN_PCAP_XXX definitions below

   -return value is a 32-bit int formatted as:

      (link_type << 20) | (file_type << 16) | link_layer_length

    where link_type is one of the LINKTYPE_XXX definitions above, file_type is one of the PCAP_TYPE_XXX definitions above, and link_layer_length is the length (in bytes) of link related information preceding the pcap record (typically ranging from 0 to 14). The full return value should be saved and then given as link_layer_info in DSReadPcap() and DSFilterPacket()

   -a return value < 0 indicates an error
*/

  int DSOpenPcap(const char* pcap_file, unsigned int uFlags, FILE** fp_pcap, pcap_hdr_t* pcap_file_hdr, const char* pErrstr);

/* DSOpenPcap() definitions */

  #define DS_OPEN_PCAP_READ                            DS_READ  /* DS_READ and DS_WRITE are defined in filelib.h */
  #define DS_OPEN_PCAP_WRITE                          DS_WRITE
  #define DS_OPEN_PCAP_DONT_READ_HEADER                 0x0100  /* don't read file header */
  #define DS_OPEN_PCAP_DONT_WRITE_HEADER                0x0200  /* don't write file header */
  #define DS_OPEN_PCAP_QUIET                            0x0400  /* suppress status and progress messages */
  #define DS_OPEN_PCAP_RESET                            0x1000  /* seek to start of pcap; assumes a valid (already open) file handle given to DSOpenPcap(). Must be combined with DS_OPEN_PCAP_READ, JHB Dec 2021 */
  #define DS_OPEN_PCAP_FILE_HDR_PCAP_FORMAT             0x2000  /* info returned in pcap_file_hdr will be in pcap (libpcap) file format, even if the file being opened is in pcapng format */

/* DSReadPcap() reads one or more pcap records at the current file position of fp_pcap into pkt_buf, and fills in one or more pcaprec_hdr_t structs (see above definition). Notes:

   -fp_pcap should point to a pcap, pcapng, or rtpXXX file previously opened by DSOpenPcap()
   -uFlags are given in DS_READ_PCAP_XXX definitions below
   -pkt_buf should point to a sufficiently large memory area to contain returned packet data
   -link_layer_info should be supplied from a prior DSOpenPcap() call. See comments above
   -if an optional p_eth_protocol pointer is supplied, one or more ETH_P_XXX ethernet protocol flags will be returned (as defined in linux/if_ether.h). NULL indicates not used
   -if an optional p_block_type pointer is supplied, one or more PCAP_XXX_TYPE or PCAPNG_XXX_TYPE flags will be returned (as defined above). When reading pcap or .rtpxxx files, PCAP_PB_TYPE or RTP_PB_TYPE is returned. NULL indicates not used
   -if an optional pcap_file_hdr pointer is supplied, the file header will be copied to this pointer (see pcap_hdr_t struct definition)
   -if a non-zero uPktNumber is supplied, warning, error, and/or information messages displayed will include this number at the end of the message. For messages concerning Interface Description, Interface Statistics, Journal, Decryption, or other block types the text "last transmitted data " is prefixed and uPktNumber-1 is displayed, as these block types do not contain actual transmitted packet data. Applications are expected to keep track of packet numbers, for example to match accurately with Wireshark, even if they perform non-sequential file access
   -if an optional szUserMsgString pointer is supplied, warning, error, and/or information messages displayed will include this string at the end of the message

   -return value is the length of the packet, zero if file end has been reached, or < 0 for an error condition
*/

  int DSReadPcap(FILE* fp_pcap, unsigned int uFlags, uint8_t* pkt_buf, pcaprec_hdr_t* pcap_pkt_hdr, int link_layer_info, uint16_t* p_eth_protocol, uint16_t* p_block_type, pcap_hdr_t* pcap_file_hdr, unsigned int uPktNumber, const char* szUserMsgString);

  #define DS_READ_PCAP_COPY                             0x0100  /* copy pcap record(s) only, don't advance file pointer */

  #define DS_READ_PCAP_DISABLE_NULL_LOOPBACK_PROTOCOL   0x0200  /* by default DSReadPcap() looks for packets with "Null/Loopback" link layers produced by Wireshark capture. To disable this behavior the DS_READ_PCAP_DISABLE_NULL_LOOPBACK_PROTOCOL flag can be applied; note however that disabling may cause "malformed packet" warnings */

  #define DS_READ_PCAP_DISABLE_TSO_LENGTH_FIX           0x0400  /* by default DSReadPcap() fixes TCP Segment Offload (TSO) packets with "zero length", and sets packet length inside returned packet data to what's in the pcap/pcapng record (typically labeled as "captured" length). Currently this is done only for block types PCAP_PB_TYPE, PCAPNG_EPB_TYPE, and PCAPNG_SPB_TYPE, and for IPv4 TCP packets. Note that Wireshark will label these as "length reported as 0, presumed to be because of TCP segmentation offload (TSO)". To disable this behavior the DS_READ_PCAP_DISABLE_TSO_LENGTH_FIX flag can be applied, although this may cause "malformed packet" warnings */

  #define DS_READ_PCAP_REPORT_TSO_LENGTH_FIX            0x0800  /* by default DSReadPcap() does not report TSO packet zero length fixes. The DS_READ_PCAP_REPORT_TSO_LENGTH_FIX flag can be applied if these should be reported with information messages */
  
  #define DS_READ_PCAP_SUPPRESS_WARNING_ERROR_MSG       DS_PKTLIB_SUPPRESS_WARNING_ERROR_MSG
  #define DS_READ_PCAP_SUPPRESS_INFO_MSG                DS_PKTLIB_SUPPRESS_INFO_MSG

  int DSWritePcap(FILE* fp_pcap, unsigned int uFlags, uint8_t* pkt_buf, int pkt_buf_len, pcaprec_hdr_t* pcap_pkt_hdr, struct ethhdr* p_eth_hdr, pcap_hdr_t* pcap_file_hdr);

  #define DS_WRITE_PCAP_SET_TIMESTAMP_WALLCLOCK         0x0100  /* use wall clock to set packet record header timestamp (this is the arrival timestamp in Wireshark) */

  int DSClosePcap(FILE* fp_pcap, unsigned uFlags);

  #define DS_CLOSE_PCAP_QUIET                           DS_OPEN_PCAP_QUIET  /* suppress status and progress messages */

/* DSFilterPacket() returns the next packet from a pcap matching given filter specs */

  int DSFilterPacket(FILE* fp_pcap, unsigned int uFlags, int link_layer_info, pcaprec_hdr_t* p_pcap_rec_hdr, uint8_t* pkt_buf, int pkt_buf_len, PKTINFO* PktInfo, uint64_t* pNumRead);  /* if fp_pcap is NULL then pktbuf must contain a valid packet and pkt_buf_len must be correct. Otherwise fp_pcap must point to a valid, already-opened FILE* handle */

  #define DS_FILTER_PKT_ARP                            0x10000  /* DS_FILTER_PKT_xxx flags may be combined with some DS_FIND_PCAP_PACKET_xxx flags */
  #define DS_FILTER_PKT_802                            0x20000
  #define DS_FILTER_PKT_TCP                            0x40000
  #define DS_FILTER_PKT_UDP                            0x80000
  #define DS_FILTER_PKT_RTCP                          0x100000
  #define DS_FILTER_PKT_UDP_SIP                       0x200000

/* DSFindPcapPacket() finds specific packets in a pcap given packet matching specs */

  uint64_t DSFindPcapPacket(const char* szInputPcap, unsigned int uFlags, PKTINFO* PktInfo, uint64_t offset_start, uint64_t offset_end, uint64_t* pFoundOffset, int* error_cond);

/* DSFindPcapPacket() RTP values to match */

  #define DS_FIND_PCAP_PACKET_RTP_SSRC                       1
  #define DS_FIND_PCAP_PACKET_RTP_PYLDTYPE                   2
  #define DS_FIND_PCAP_PACKET_RTP_TIMESTAMP                  4

/* DSFindPcapPacket() general packet values to match */
 
  #define DS_FIND_PCAP_PACKET_SRC_PORT                   0x100
  #define DS_FIND_PCAP_PACKET_DST_PORT                   0x200
  #define DS_FIND_PCAP_PACKET_SEQNUM                     0x400  /* TCP sequence number or UDP/RTP sequence number */

  #define DS_FIND_PCAP_PACKET_FIRST_MATCHING            0x1000
  #define DS_FIND_PCAP_PACKET_LAST_MATCHING             0x2000
  #define DS_FIND_PCAP_PACKET_USE_SEEK_OFFSET           0x4000  /* use byte offset instead of record offset. Seek offset gives faster performance but record offset can be useful when the number of records searched prior to a match is needed. Record offset is the default. This flag may be combined with DS_FILTER_PKT_xxx flags and affects the return value of pNumRead in DSFilterPacket() */

/* DSConfigMediaService() -- start the SigSRF media service as a process or some number of packet/media threads. Notes:

    -threads[] is an array of thread handles specifying packet/media threads to be acted on (currently handles are indexes, for example, 0 .. 3 specifies packet/media threads 0 through 3). When the DS_CONFIG_MEDIA_SERVICE_START flag is specified, threads[] can be
     given as NULL in which case thread handles will not be returned to the user application. Other DS_CONFIG_MEDIA_SERVICE_XXX flags may require threads[] to be non-NULL. Note that thread handles should not be confused with thread id values, which are determined
     by the OS (typically Linux). Packet/media thread id values can be determined by calling DSGetThreadInfo()

    -num_threads is the number of thread handles contained in threads

    -uFlags starts, suspends, or exits the thread or process, and also specifies whether the media service should run as a thread or process when DS_CONFIG_MEDIA_SERVICE_START is given (see definitions below)

    *func is a pointer to a thread function. Ignored if DS_CONFIG_MEDIA_SERVICE_THREAD is not given

    -szCmdLine points to a string containing an optional command line. If NULL, or uFlags specifies a thread, then cmdLine is ignored

    -the return value is -1 for error conditions, otherwise the the return value is the number of threads acted on
*/

  int DSConfigMediaService(int threads[], unsigned int uFlags, int num_threads, void* (*func)(void*), char* szCmdLine);

  int64_t DSGetThreadInfo(int64_t thread_identifier, unsigned int uFlags, PACKETMEDIATHREADINFO* pInfo);  /* returns information about the packet/media thread specified by handle (by default a thread index, but also can be a pthread_t thread id, see comments) */

#if 0  /* currently not used */
  int DSSetThreadInfo(pthread_t thread, unsigned int uFlags, int64_t param, void* pInfo);
#endif


/* entry function for thread based packet flow and media processing */

  void* packet_flow_media_proc(void* pExecutionMode);


/* DSGetTermChan() gets the channel for hSession's termN endpoint, with optional channel validation checks. chnum can be NULL if the channel number is not needed and only purpose is validation. See DS_CHECK_CHAN_xxx flags below */

  int DSGetTermChan(HSESSION hSession, unsigned int uFlags, int* chnum, int nTerm);


/* DSPushPackets() and DSPullPackets() -- send and receive packets from the media service. Notes:

  -pkt_buf points to one or more pushed or pulled packets stored consecutively in memory

  -for DSPushPackets() numPkts is the number of packets to push. The return value is (i) number of packets pushed, (ii) zero which means queue is full and the push should be re-tried, or (iii) -1 which indicates an error condition

  -DSPullPackets() returns the number of packets pulled. -1 indicates an error condition

  -len is an array of length numPkts containing the length of each packet

  -uFlags can be used to filter packets pulled (see DS_PULLPACKETS_xxx constant definitions)

  -pktInfo is an array of length numPkts where each entry contains info about a packet, as follows:

     -bits 0..15:   type of pulled packet (jitter buffer, transcoded, or stream group)
     -bits 16..31:  codec type; i.e. type of compression used in the RTP payload
     -bits 32..63:  session handle; i.e. handle of the session to which the packet belongs

  -hSession:
  
     -for DSPushPackets(), an array of session handles with a one-to-one relationship with packets pointed to by pkt_buf
     -for DSPullPackets(), a session handle that can be used to filter pullled packets by session. If -1 is given, then all available packets for all active sessions are pulled, and per-packet session handles are stored in pktInfo[]

  -pkt_max_buf_len is the maximum amount of buffer space pointed to by pkt_buf that DSPullPackets can write
*/

  int DSPushPackets(unsigned int uFlags, uint8_t pkt_buf[], int pkt_buf_len[], HSESSION* hSession, unsigned int numPkts);
  int DSPullPackets(unsigned int uFlags, uint8_t pkt_buf[], int pkt_buf_len[], HSESSION hSession, uint64_t* pktInfo, unsigned int pkt_max_buf_len, int numPkts);

  int DSGetDebugInfo(unsigned int uFlags, int, int*, int*);  /* for internal use only */

  int DSDisplayThreadDebugInfo(uint64_t uThreadList, unsigned int uFlags, const char* userstr);

  void DSLogPktTrace(HSESSION, uint8_t* pkt_buf, int pkt_buf_len, int thread_index, unsigned int uFlags);

/* write full/detailed packet stats history to packet log text file, using either session handle or thread index. For packet stats history to be available, DS_ENABLE_PACKET_STATS_HISTORY_LOGGING must be set in the DEBUG_CONFIG struct uPktStatsLogging item (see shared_include/config.h). See also DS_PKT_STATS_HISTORY_LOG_xx flags below and DS_PKTSTATS_xx flags in diaglib.h */

  int DSWritePacketStatsHistoryLog(HSESSION hSession, unsigned int uFlags, const char* szLogFilename);

  bool DSIsPktStatsHistoryLoggingEnabled(int thread_index);

/* write run-time packet time and loss stats to event log using session handle. DSConfigPktlib() can be used to set DS_ENABLE_PACKET_TIME_STATS and/or DS_ENABLE_PACKET_LOSS_STATS flags in the DEBUG_CONFIG struct uPktStatsLogging item (see shared_include/config.h) */

  int DSLogRunTimeStats(HSESSION hSession, unsigned int uFlags);

#ifdef __cplusplus
}
#endif

/* uFlags for DSconfigPktlib() API above */

#if 0  /* deprecated, DSConfigPktlib() looks only at pGlobalConfig and pDebugConfig params, Apr 2024 */
#define DS_CP_GLOBALCONFIG                                0x01
#define DS_CP_DEBUGCONFIG                                 0x02
#endif
#define DS_CP_INIT                                        0x04

/* DSCreateSession() uFlags definitions */

#define DS_SESSION_USER_MANAGED                          0x100  /* session id will be used in hash key, requires app to know which session incomming packets belong to */
#define DS_SESSION_DYN_CHAN_ENABLE                       0x200  /* channels will be dyanimcally created for a given session when a new SSRC value is seen on a given channel (implementation follows RFC 8108) */
#define DS_SESSION_DISABLE_NETIO                         0x400  /* disable network I/O initialization, subsequent DSRecv/SendPackets() calls will return errors if this flag is used when creating a session */
#define DS_SESSION_DISABLE_PRESERVE_SEQNUM               0x800  /* don't preserve RTP sequence number from incomming stream */

#define DS_SESSION_NO_JITTERBUFFER                      0x1000

/* DSRecvPackets() uFlags definitions */

#define DS_RECV_PKT_ADDTOJITTERBUFFER                      0x1
#define DS_RECV_PKT_SOCKET_HANDLE                          0x2
#define DS_RECV_PKT_BLOCK                                  0x4
#define DS_RECV_PKT_QUEUE                                  0x8
#define DS_RECV_PKT_INIT                                  0x10

#define DS_RECV_PKT_FILTER_RTCP                          0x100  /* filter RTCP packets */
#define DS_RECV_PKT_QUEUE_COPY                           0x200  /* pull packets from the receive queue, but copy, or "look ahead" only, don't advance the receive queue ptr */
#define DS_RECV_PKT_ENABLE_RFC7198_DEDUP                 0x400  /* apply RFC7198 packet temporal de-duplication to packets when returning them to the calling app or thread. Default in the SigSRF packet/media thread is enabled */

/* DSSendPackets() uFlags definitions */

#define DS_SEND_PKT_FMT                                    0x1
#define DS_SEND_PKT_SOCKET_HANDLE                          0x2
#define DS_SEND_PKT_QUEUE                                  0x4
#define DS_SEND_PKT_SUPPRESS_QUEUE_FULL_MSG                DS_PKTLIB_SUPPRESS_WARNING_ERROR_MSG


/* DSBufferPackets() and DSGetOrderedPackets() uFlags definitions */

#define DS_BUFFER_PKT_HDR_ONLY                             0x1
#define DS_BUFFER_PKT_FULL_PACKET                          0x2

#define DS_BUFFER_PKT_IP_PACKET                           0x10  /* indicates pkt_buf points to full IP header followed by TCP or UDP packet data */
#define DS_BUFFER_PKT_UDP_PACKET                          0x20  /* indicates pkt_buf points to a UDP header followed by a UDP payload (for example, a UDP defined protocol such as RTP, GTP, etc) */
#define DS_BUFFER_PKT_RTP_PACKET                          0x40  /* incdicates pkt_buf points to an RTP header followed by an RTP payload */

#define DS_BUFFER_PKT_HDR_MASK                      0xf00000ffL

#define DS_BUFFER_PKT_ALLOW_DYNAMIC_DEPTH               0x1000
#define DS_BUFFER_PKT_DISABLE_PROBATION                 0x2000
#define DS_BUFFER_PKT_ALLOW_TIMESTAMP_JUMP              0x4000  /* prevent DSBufferPackets() from purging due to large timestamp jumps, and DSGetOrderedPackets() from returning non-deliverable due to same */ 
#define DS_BUFFER_PKT_ENABLE_RFC7198_DEDUP              0x8000  /* legacy method of handling RFC7198 temporal de-duplication, should not be used unless needed in a specific case. New method is to apply the DS_RECV_PKT_ENABLE_RFC7198_DEDUP flag to packets being received from a per session queue */
#define DS_BUFFER_PKT_ENABLE_DYNAMIC_ADJUST            0x10000  /* enable dynamic jitter buffer; i.e. target delay is adjusted dynamically based on measured incoming packet delays */
#define DS_BUFFER_PKT_EXTENDED_SESSION_SEARCH  DS_PKT_INFO_EXTENDED_SESSION_SEARCH  /* enable extended session search to allow packet buffering for mix of user-managed sessions and process-managed sessions */
#define DS_BUFFER_PKT_EXCLUDE_PAYLOAD_TYPE             0x20000

#define DS_GETORD_PKT_SESSION                            0x100
#define DS_GETORD_PKT_CHNUM                              0x200
#define DS_GETORD_PKT_CHNUM_PARENT_ONLY                  0x400
#define DS_GETORD_PKT_ANALYTICS                        0x10000  /* analytics mode, advance RTP retrieval timestamp every time DSGetOrderedPackets is called */
#define DS_GETORD_PKT_FLUSH                            0x20000
#define DS_GETORD_PKT_RETURN_ALL_DELIVERABLE           0x40000  /* tell DSGetOrderedPackets() to return any deliverable packets regardless of time window or sequence number */
#define DS_GETORD_PKT_ENABLE_DTX                       0x80000  /* enable DTX handling -- note this flag is deprecated, DTX handling should be controlled by the TERM_DTX_ENABLE flag in TERMINATION_INFO struct uFlags element (shared_include/session.h). DS_GETORD_PKT_ENABLE_DTX can still be applied to force DTX handling on a case-by-case basis, but that is not recommended */
#define DS_GETORD_PKT_ENABLE_DTMF                     0x100000  /* enable DTMF handling */
#define DS_GETORD_PKT_TIMESTAMP_GAP_RESYNC            0x200000  /* jitter buffer resync on timestamp gaps -- this flag causes DSGetOrderedPackets() to perform a jitter buffer resync for large timestamp gaps. For example if there is a large timestamp gap, the jitter buffer is immediately resync'd and packets are delivered as if there were no gap. This flag is ignored if the DS_GETORD_PKT_RETURN_ALL_DELIVERABLE flag is also specified */
#define DS_GETORD_PKT_ENABLE_SINGLE_PKT_LKAHD         0x400000  /* [Deprecated -- see DS_GETORD_PKT_ENABLE_OOO_HOLDOFF replacement] enable single packet look-ahead -- this flag is reserved for situations where, for some reason, the jitter buffer has only one packet available and is unable to perform comparisons for re-ordering. Under normal oepration, this does not occur, but the flag is available if such a case should ever arise */
#define DS_GETORD_PKT_ENABLE_SID_REPAIR               0x800000  /* enable SID repair if multiple lost SID packets are detected -- note this flag is deprecated, SID repair should be controlled by the TERM_SID_REPAIR_ENABLE flag in TERMINATION_INFO struct uFlags element (shared_include/session.h). DS_GETORD_PKT_ENABLE_SID_REPAIR can still be applied to force SID repair on a case-by-case basis, but that is not recommended */
#define DS_GETORD_PKT_ADVANCE_TIMESTAMP              0x1000000  /* instructs DSGetOrderedPackets() to advance the specified channels' timestamps by ptime amount, effectively "pulling packets from future time". This is used to help control jitter buffer memory usage in overrun situations. See comments near DSGetOrderedPackets() in packet_flow_media_proc.c */
#define DS_GETORD_PKT_ENABLE_OOO_HOLDOFF             0x2000000  /* enable dynamic holdoff to allow for outlier cases of ooo. Under certain conditions, including low jitter buffer level, a packet "in the future" will be temporarily held for delievery for a short time to see if one or missing packets arrive late. This flag replaces theDS_GETORD_PKT_ENABLE_SINGLE_PKT_LKAHD flag, which is deprecated */

/* flags return by *uInfo param (if uInfo is non NULL) */
 
#define DS_GETORD_PKT_INFO_PULLATTEMPT                     0x1  /* a valid pull attempt was made; i.e. there were no errors, timestamp delta was >= ptime, etc */

/* DSGetJitterBufferInfo() and DSSetJitterBufferInfo() uFlags definitions */

#define DS_JITTER_BUFFER_INFO_TARGET_DELAY                 0x2
#define DS_JITTER_BUFFER_INFO_MIN_DELAY                    0x3
#define DS_JITTER_BUFFER_INFO_MAX_DELAY                    0x4
#define DS_JITTER_BUFFER_INFO_MAX_DEPTH_PTIMES             0x5
#define DS_JITTER_BUFFER_INFO_UNDERRUN_RESYNC_WARNING      0x6
#define DS_JITTER_BUFFER_INFO_SID_REPAIR                   0x7
#define DS_JITTER_BUFFER_INFO_SID_TIMESTAMP_ALIGN          0x8
#if 0  /* these stats moved internal to pktlib, along with level flush stat, JHB Jun 2020 */
#define DS_JITTER_BUFFER_INFO_NUM_PKT_LOSS_FLUSH           0x9
#define DS_JITTER_BUFFER_INFO_NUM_PASTDUE_FLUSH            0xa
#endif
#define DS_JITTER_BUFFER_INFO_SSRC                         0xb
#define DS_JITTER_BUFFER_INFO_MISSING_SEQ_NUM              0xc
#define DS_JITTER_BUFFER_INFO_NUM_INPUT_OOO                0xd
#define DS_JITTER_BUFFER_INFO_MAX_INPUT_OOO                0xe
#define DS_JITTER_BUFFER_INFO_INPUT_PKT_COUNT              0xf
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
#define DS_JITTER_BUFFER_INFO_INPUT_SID_COUNT             0x2f  /* JHB Oct 2022 */
#define DS_JITTER_BUFFER_INFO_PKT_BITRATE_LIST            0x30
#define DS_JITTER_BUFFER_INFO_CURRENT_ALLOCS              0x31  /* JHB Dec 2022 */
#define DS_JITTER_BUFFER_INFO_MAX_ALLOCS                  0x32
#define DS_JITTER_BUFFER_INFO_NUM_DTMF_PKTS               0x33  /* DTMF RTP event packet count, JHB Jul 2023 */
#define DS_JITTER_BUFFER_INFO_PKT_CLASSIFICATION_LIST     0x34
#define DS_JITTER_BUFFER_INFO_NUM_TIMESTAMP_SETBACKS      0x35

#define DS_JITTER_BUFFER_INFO_ITEM_MASK                   0xff

#define DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING      0x1000  /* reserved */

/* DSGetPacketInfo() uFlags definitions */

#define DS_PKT_INFO_CODEC                                    1  /* these flags are "session and stream items"; i.e. they apply to packets that match previously created sessions. pInfo is an optional arg that may specify a pointer to a TERMINATION_INFO struct or a SESSION_DATA struct */
#define DS_PKT_INFO_CODEC_LINK                               2
#define DS_PKT_INFO_SESSION                                  3
#define DS_PKT_INFO_CHNUM                                    4
#define DS_PKT_INFO_CHNUM_PARENT                             5
#define DS_PKT_INFO_CODEC_TYPE                               6
#define DS_PKT_INFO_CODEC_TYPE_LINK                          7
#define DS_PKT_INFO_PYLD_CONTENT                             8  /* returns a DS_PKT_PYLD_CONTENT_XXX type. If pInfo is not NULL, on return it will point to a PAYLOAD_INFO struct (filled by calling DSGetPayloadInfo() in voplib) */

#define DS_PKT_INFO_SESSION_ITEM_MASK                     0x0f  /* mask value to isolate above DS_PKT_INFO_xxx session and stream item flags */

/* flags from this point work on general IP packets and do not require previously created sessions */

#define DS_PKT_INFO_RTP_VERSION                         0x0100  /* added RTP pkt info items, JHB Sep 2017 */
#define DS_PKT_INFO_RTP_PYLDTYPE                        0x0200
#define DS_PKT_INFO_RTP_MARKERBIT                       0x0300
#define DS_PKT_INFO_RTP_HDROFS                          0x0400  /* retrieves offset to start of RTP header (assumes a UDP packet) */
#define DS_PKT_INFO_RTP_PADDING_SIZE                    0x0500  /* retrieves RTP payload padding size */
#define DS_PKT_INFO_RTP_SEQNUM                          0x0800
#define DS_PKT_INFO_RTP_TIMESTAMP                       0x0900
#define DS_PKT_INFO_RTP_SSRC                            0x0a00
#define DS_PKT_INFO_RTP_PYLDOFS                         0x0b00  /* retrieves offset to start of RTP payload */
#define DS_PKT_INFO_RTP_PYLDLEN                         0x0c00
#define DS_PKT_INFO_RTP_HDRLEN                          0x0d00  /* retrieves RTP header length, including extensions if any */

#define DS_PKT_INFO_RTP_HEADER                          0xff00  /* returns whole RTP header in void* pInfo arg */

#define DS_PKT_INFO_RTP_ITEM_MASK                       0x0f00  /* mask value to isolate above DS_PKT_INFO_RTP_xxx item flags */

#define DS_PKT_INFO_HDRLEN                              0x1000  /* returns length of IP address headers (valid for IPv4 and IPv6, including IPv6 extension headers) */
#define DS_PKT_INFO_PKTLEN                              0x2000  /* returns total packet length, including IP, UDP, and RTP headers, and payload */
#define DS_PKT_INFO_SRC_PORT                            0x3000  /* UDP source port */
#define DS_PKT_INFO_DST_PORT                            0x4000  /* UDP destination port */
#define DS_PKT_INFO_IP_VERSION                          0x5000
#define DS_PKT_INFO_PROTOCOL                            0x6000  /* returns packet's protocol type; pInfo, if not NULL, contains a string with the protocol name. For IPv6 the returned name is the first protocol in the extension chain, JHB Apr 2025 */
#define DS_PKT_INFO_PYLDOFS                             0x7000  /* returns offset to start of UDP or TCP payload data */
#define DS_PKT_INFO_PYLDLEN                             0x8000  /* returns size of packet payload. For UDP packets this is the UDP header "Length" field excluding the UDP header size (to include the UDP header add the DS_PKT_INFO_PKTINFO_PYLDLEN_INCLUDE_UDP_HDR flag). For TCP packets this is packet length excluding IP and TCP headers */
#define DS_PKT_INFO_SRC_ADDR                            0x9000  /* requires pInfo to point to array of sufficient size, returns IP version */
#define DS_PKT_INFO_DST_ADDR                            0xa000
#define DS_PKT_INFO_EXT_HDRLEN                          0xb000  /* length of extension headers, applicable only to IPv6 packets */

#define DS_PKT_INFO_PKTINFO                             0xf000  /* stores a PKTINFO struct in pInfo (if specified) with a return value of 1 on success, 2 if a fully re-assembled packet is available, and -1 on error condition. This API is intended to minimize packet processing overhead if several packet items are needed. See PKTINFO struct definition above, containing TCP, UDP, and RTP items */

#define DS_PKT_INFO_ITEM_MASK                           0xff00  /* mask value to isolate above DS_PKT_INFO_xxx item flags */

#define DS_PKT_INFO_PKTINFO_EXCLUDE_RTP                0x10000
#define DS_PKT_INFO_PKTINFO_PYLDLEN_INCLUDE_UDP_HDR    0x20000

#define DS_PKT_INFO_FRAGMENT_SAVE                      0x40000  /* save packet fragment to pktlib internal fragment list using header's Identification field (should be applied only if packet IP header contains fragmentation info) */
#define DS_PKT_INFO_FRAGMENT_REMOVE                    0x80000  /* remove packet fragment from pktlib internal list using header's Identification field (should be applied only if packet IP header contains fragmentation info) */
#define DS_PKT_INFO_REASSEMBLY_GET_PACKET             0x100000  /* retrieve fully reassembled packet in pInfo and return the reassembled packet's length. This flag should only be specified if a previous call to DSGetPacketInfo() with the DS_PKT_INFO_FRAGMENT_SAVE flag has returned a flag value indicating a fully re-assembled packet is available */

#define DS_PKT_INFO_EXTENDED_SESSION_SEARCH           0x400000  /* enable extended session search to allow matching of packets to mix of user-managed sessions and process-managed sessions */
#define DS_PKT_INFO_COPY_IP_HDR_LEN_IN_PINFO          0x800000  /* return packet's IP header length in pInfo if pInfo is not NULL. Other flags that use pInfo take precedence if combined in uFlags */
#define DS_PKT_INFO_PINFO_CONTAINS_WARN_ERR_STRING   0x1000000  /* pInfo contains a string that should be included with warning or error messages. For example a unique packet or thread identifier might be given to help identify which DSGetPacketInfo() call is producing a message */
#define DS_PKT_INFO_PINFO_CONTAINS_ETH_PROTOCOL      0x2000000  /* pInfo contains ethernet protocol (e.g. ARP, LLC frame, etc). By default DSGetPacketInfo() assumes ETH_P_IP or ETH_P_IPV6 as defined in the Linux if_ether.h header file and looks at the IP packet header to determine IPv4 or IPv6. To handle link layer packets DSGetPacketInfo() needs to know the ethernet protocol, which can be supplied in pInfo and the DS_PKT_INFO_PINFO_CONTAINS_ETH_PROTOCOL flag applied */

/* the following flags are returned by DSGetPacketInfo() when uFlags contains DS_PKT_INFO_PKTINFO or DS_PKT_INFO_FRAGMENT_xxx flags */

#define DS_PKT_INFO_RETURN_OK                                1  /* PktInfo struct filled successfully */
#define DS_PKT_INFO_RETURN_FRAGMENT                          2  /* packet is a fragment */
#define DS_PKT_INFO_RETURN_FRAGMENT_SAVED                    4  /* fragment was saved to pktlib internal list */
#define DS_PKT_INFO_RETURN_FRAGMENT_REMOVED                  8  /* fragment was removed from pktlib internal list */
#define DS_PKT_INFO_RETURN_REASSEMBLED_PACKET_AVAILABLE   0x10  /* a fully re-assembled packet is available using the DS_PKT_INFO_GET_REASSEMBLED_PACKET flag in a subsequent DSGetPacketInfo() call */

/* the following flags may be returned by DSGetPacketInfo() when uFlags contains the DS_PKT_INFO_PINFO_CONTAINS_ETH_PROTOCOL flag */

#define DS_PKT_INFO_RETURN_UNRECOGNIZED_ETH_PROTOCOL        -2

/* pktlib general API flags, for use with uFlags argument in DSGetPacketInfo(), DSFormatPacket(), DSBufferPackets(), and DSGetOrderedPackets() */

#define DS_PKTLIB_NETWORK_BYTE_ORDER                0x00000000L /* indicates packet header data is in network byte order. The byte order flags apply only to headers, not payload contents. This flag is zero as the default (no flag) is network byte order, and is defined here only for documentation purposes */
#define DS_PKTLIB_HOST_BYTE_ORDER                   0x10000000L /* indicates packet header data is in host byte order. The byte order flags apply only to headers, not payload contents. Default (no flag) is network byte order */
#define DS_PKTLIB_SUPPRESS_WARNING_ERROR_MSG        0x20000000L /* suppress general packet format error messages; e.g. malformed packet, invalid IP version, invalid IP header, etc */
#define DS_PKTLIB_SUPPRESS_INFO_MSG                 0x40000000L /* suppress info messages, many are RTP related */
#define DS_PKTLIB_SUPPRESS_RTP_WARNING_ERROR_MSG    0x80000000L /* suppress RTP related error and warning messages, e.g. incorrect RTP version, RTP header extension mismatch, RTP padding mismatch, etc. Note RTP related warnings and errors are treated separately from general pktlib API warning and error messages */

/* other pktlib flags (not for use with uFlags argument) */

#define DS_PKT_INFO_USE_IP_HDR_LEN                  0x80000000L /* when combined with the len param, indicates len should be interpreted as IP header length. This flag should only be used with DS_PKT_INFO_xxx session and stream items. See PushPackets() in mediaMin.cpp for a usage example. NOTE - this flag is not a uFlag and should not be combined with any other DS_PKT_INFO_XXX uflags */

/* DSGetSessionInfo() and DSSetSessionInfo() uFlags definitions */

#define DS_SESSION_INFO_HANDLE                           0x100  /* specifes the sessionHandle argument is treated as a session handle (default) */
#define DS_SESSION_INFO_CHNUM                            0x200  /* specifies the sessionHandle argument should be treated as a channel number. If combined with DS_SESSION_INFO_HANDLE, DSGeSessionInfo() returns a channel number, depending on the term_id argument */

#define DS_SESSION_INFO_CODEC                              0x1  /* retrieves codec handles: term_id param 0 indicates group codec, 1 = chnum codec (decoder, or term1 if handle is an hSession), 2 = chnum link codec (encoder, or term2 if handle is an hSession) */
#define DS_SESSION_INFO_SAMPLE_RATE                        0x3
#define DS_SESSION_INFO_CODEC_TYPE                         0x4
#define DS_SESSION_INFO_SESSION                            0x5  /* for DS_SESSION_INFO_SESSION and DS_SESSION_INFO_TERM_ID, pInfo should point to a SESSION_DATA struct. For all other flags, pInfo should point to a TERMINATION_INFO struct */
#define DS_SESSION_INFO_TERM                               0x6  /* get term # and info using session handle or channel number */
#if 0  /* currently not used */
#define DS_SESSION_INFO_CHNUM_QUERY                        0x7  /* similar to DS_SESSION_INFO_TERM, except all current sessions are searched for a match, and if there is any inconsistency or duplication, an error is returned */
#endif
#define DS_SESSION_INFO_GROUP_STATUS                       0x8
#define DS_SESSION_INFO_GROUP_MODE                         0x9
#define DS_SESSION_INFO_UFLAGS                             0xa  /* returns uFlags applied when session was created if term_id = 0, or termN.uFlags if term_id = 1 or 2 */
#define DS_SESSION_INFO_STATE                              0xb  /* get or set current session state (see STATE_xxx flags below). When setting a session state, only one or more flags should be combined and used in DSSetSessionInfo(), not the session state itself. Positive value of flags is a set, negative value is a clear */
#define DS_SESSION_INFO_NUM_SESSIONS                       0xc  /* get total number of currently active sessions */
#define DS_SESSION_INFO_INPUT_BUFFER_INTERVAL              0xd  /* get buffer add interval of the session */
#define DS_SESSION_INFO_PTIME                              0xe  /* get ptime of the session. Note that each term (channel) also has its own ptime */
#define DS_SESSION_INFO_GROUP_OWNER                        0xf  /* get stream group owner session. This is the session that initially defined the stream group ID */
#define DS_SESSION_INFO_GROUP_SAMPLE_RATE                 0x11  /* get stream group term sample rate */
#define DS_SESSION_INFO_THREAD_ID                         0x12  /* get id of thread to which session is assigned. Only applicable if packet_flow_media_proc() is running as one or more threads */
#define DS_SESSION_INFO_CHNUM_PARENT                      0x13  /* get chnum of dynamic channel's parent. If param is already a parent returns itself */
#if 0  /* currently not used */
#define DS_SESSION_INFO_GROUP_TIMESTAMP                   0x14  /* most recent time group packetization was done */
#endif
#define DS_SESSION_INFO_GROUP_ID                          0x15
#define DS_SESSION_INFO_GROUP_BUFFER_TIME                 0x16  /* get / set stream group buffer time in msec (affects both merge buffer and sample domain processing buffer sizes. Default is 260 msec, see comments in streamlib.h) */
#define DS_SESSION_INFO_DELETE_STATUS                     0x17
#define DS_SESSION_INFO_THREAD                            0x18  /* get index of thread to which session is assigned. Packet/media thread indexes range from 0..MAX_PKTMEDIA_THREADS-1. Index 0 always exists, even if all sessions are static (no dynamic sessions) */
#define DS_SESSION_INFO_GROUP_PTIME                       0x19
#define DS_SESSION_INFO_OUTPUT_BUFFER_INTERVAL            0x1a  /* get buffer output interval of the session */
#define DS_SESSION_INFO_RTP_PAYLOAD_TYPE                  0x1b
#define DS_SESSION_INFO_INPUT_SAMPLE_RATE                 0x1c  /* only applicable to codecs for which input and decode sample rates can be different (so far only applies to EVS and Opus) */
#define DS_SESSION_INFO_TERM_FLAGS                        0x1d  /* returns "uFlags" item from TERMINATION_INFO struct for a given channel number or session and term_id. If term_id is zero then stream group term flags are returned (if applicable). Note that DS_SESSION_INFO_UFLAGS does same thing for term_id = 1 or 2 but for term_id 0 returns flags used in original DSSessionCreate() call */
#define DS_SESSION_INFO_MAX_LOSS_PTIMES                   0x1e  /* returns "max_loss_ptimes" item from TERMINATION_INFO struct for a given channel number or session + term_id */
#define DS_SESSION_INFO_DYNAMIC_CHANNELS                  0x1f  /* retrieve list of dynamic channels (child channels) for a parent, which can be specified either as a parent channel, or a parent hSession + termN_id */
#define DS_SESSION_INFO_NAME                              0x20  /* retrieve optional session name string, if any has been set */
#define DS_SESSION_INFO_CUR_ACTIVE_CHANNEL                0x21  /* returns currently active channel */
#define DS_SESSION_INFO_RFC7198_LOOKBACK                  0x22
#define DS_SESSION_INFO_LAST_ACTIVE_CHANNEL               0x23
#define DS_SESSION_INFO_RTP_CLOCKRATE                     0x24
#define DS_SESSION_INFO_GROUP_IDX                         0x25  /* get stream group index */

#define DS_SESSION_INFO_USE_PKTLIB_SEM             0x20000000L  /* use the pktlib semaphore */
#define DS_SESSION_INFO_SUPPRESS_ERROR_MSG         DS_PKTLIB_SUPPRESS_WARNING_ERROR_MSG  /* suppress warning or error messages generated by the API */

#define DS_SESSION_INFO_ITEM_MASK                         0xff

/* flags used with state values returned and set by DSsetSessionInfo() and DSGetSessionInfo() (when used with DS_SESSION_INFO_HANDLE | DS_SESSION_INFO_STATE ) */

#define DS_SESSION_STATE_NEW                                 0  /* session states */
#define DS_SESSION_STATE_INIT_STATUS                         1

/* actions */

#define DS_SESSION_STATE_FLUSH_PACKETS                   0x100  /* flush a session, for example prior to deleting a session, flush all remaining packets from the jitter buffer */
#define DS_SESSION_STATE_WRITE_PKT_LOG                   0x200  /* writes packet log for a session. If the session is a stream group owner, includes all group member sessions in the log */
#define DS_SESSION_STATE_RESET_PKT_LOG                   0x400  /* reset internal packet stats counters */

/* other (reserved) */

#define DS_SESSION_DELETE_PENDING                            1

/* jitter buffer options handled via DSSetSessionInfo() */
/* note - the DS_BUFFER_PKT_ALLOW_TIMESTAMP_JUMP and DS_BUFFER_PKT_ENABLE_DYNAMIC_ADJUST flags should be used instead, JHB Jan 2023 */

#define DS_SESSION_STATE_ALLOW_TIMSTAMP_JUMP           0x10000  /* instruct the jitter buffer to ignore large jumps in timestamps and sequence numbers, for example due to manual pcap manipulation or multistream packets arriving in alternating chunks between streams */
#define DS_SESSION_STATE_ALLOW_DYNAMIC_ADJUST          0x20000  /* instruct the jitter buffer to adjust target delay dynamically, based on measured incoming packet delays */


/* DS_PKT_PYLD_CONTENT_XXX types and flags are returned in pkt_info[] args in DSBufferPackets() and DSGetOrderedPackets(). Also DSGetPacketInfo() with uFlags containing DS_PKT_INFO_PYLD_CONTENT will return DS_PKT_PYLD_CONTENT_DTMF, DS_PKT_PYLD_CONTENT_SID, or DS_PKT_PYLD_CONTENT_MEDIA */

#define DS_PKT_PYLD_CONTENT_UNKNOWN                     0x2000  /* unknown */
#define DS_PKT_PYLD_CONTENT_MEDIA                       0x2100  /* compressed voice or video bitstream data, use session->termN.codec_type to know which codec */
#define DS_PKT_PYLD_CONTENT_SID                         0x2200  /* SID frame */
#define DS_PKT_PYLD_CONTENT_SID_REUSE                   0x2300  /* SID reuse frame (generated by DTX handling and/or SID packet repair) */
#define DS_PKT_PYLD_CONTENT_SID_NODATA                  0x2400  /* SID no data frame */
#define DS_PKT_PYLD_CONTENT_DTX                         0x2500  /* DTX frame, normally same as a SID but not in all cases */
#define DS_PKT_PYLD_CONTENT_RTCP                        0x2600  /* RTCP payload */
#define DS_PKT_PYLD_CONTENT_DTMF                        0x2700  /* DTMF Event Packet RFC 4733, generic definition */
#define DS_PKT_PYLD_CONTENT_PROBATION                   0x2800
#define DS_PKT_PYLD_CONTENT_DTMF_SESSION                0x2900  /* DTMF matching a session-defined DTMF payload type -- note, only returned by DSGetOrderedPackets(), not DSBufferPackets() or DSGetPacketInfo() */
#define DS_PKT_PYLD_CONTENT_MEDIA_REUSE                 0x2a00  /* media packet gap and timestamp jump adjustment */

/* can be combined with other DS_PKT_PYLD_CONTENT_XXX flags */

#define DS_PKT_PYLD_CONTENT_REPAIR                     0x10000  /* indicates packet was repaired, for example a jitter buffer output packet that resulted from media PLC or SID repair */
#define DS_PKT_PYLD_CONTENT_MULTICHAN                  0x20000
#define DS_PKT_PYLD_CONTENT_MULTIFRAME                 0x40000
#define DS_PKT_PYLD_CONTENT_IGNORE_PTIME               0x80000  /* currently used by packet/media thread workers calling DSGetStreamData() */

#define DS_PKT_PYLD_CONTENT_ITEM_MASK                   0xff00

/* DSFormatPacket() definitions */

#define DS_FMT_PKT_SEND                                 0x0010  /* send the packet after formatting */

#define DS_FMT_PKT_STANDALONE                           0x0020  /* format packet separately from sessions created by pktlib DSCreateSession(). The chnum param of the DSFormatPacket() API is ignored, and otherwise no association is made with existing pktlib sessions */

#define DS_FMT_PKT_TCPIP                                0x0040  /* format packet as TCP/IP */

#define DS_FMT_PKT_USER_PYLDTYPE                        0x0100  /* DS_FMT_PKT_USER_ITEM flags indicate that ITEM is being supplied in the FORMAT_PKT struct pointer param of the DSFormatPacket() API */ 
#define DS_FMT_PKT_USER_MARKERBIT                       0x0200
#define DS_FMT_PKT_USER_SEQNUM                          0x0400
#define DS_FMT_PKT_USER_TIMESTAMP                       0x0800
#define DS_FMT_PKT_USER_SSRC                            0x1000
#define DS_FMT_PKT_USER_PTIME                           0x2000
#define DS_FMT_PKT_USER_SRC_IPADDR                      0x4000
#define DS_FMT_PKT_USER_DST_IPADDR                      0x8000
#define DS_FMT_PKT_USER_SRC_PORT                       0x10000  /* either UDP or TCP source port */
#define DS_FMT_PKT_USER_DST_PORT                       0x20000  /* either UDP or TCP dest port */

#define DS_FMT_PKT_DISABLE_IPV4_CHECKSUM               0x40000  /* disable IPV4 checksum calculation when formatting the packet */
#define DS_FMT_PKT_RTP_EVENT                           0x80000
#define DS_FMT_PKT_NO_INC_CHNUM_TIMESTAMP             0x100000  /* do not increment chnum internal record timestamp (this flag is reserved, do not use) */

#define DS_FMT_PKT_USER_UDP_PAYLOAD                   0x200000  /* user supplies complete UDP payload, e.g. RTP header and payload */

#define DS_FMT_PKT_USER_HDRALL                        DS_FMT_PKT_USER_SRC_IPADDR | DS_FMT_PKT_USER_DST_IPADDR | DS_FMT_PKT_USER_SRC_PORT | DS_FMT_PKT_USER_DST_PORT

/* DSConfigMediaService() uFlags definitions

  -action flags cannot be combined. Pktlib internally uses DS_MEDIASERVICE_ACTION_MASK to perform a single action
  -task flags cannot be combined. Pktlib internally uses DS_MEDIASERVICE_TASK_MASK to act on a single task object (thread, process, or app)
  -session assignment flags (linear, round-robin) can be combined with DS_MEDIASERVICE_START
  -the cmd line flag can be combined with DS_MEDIASERVICE_START (in which case szCmdLine should not be NULL)
*/

#define DS_MEDIASERVICE_START                                1  /* start media service threads or process */
#define DS_MEDIASERVICE_SUSPEND                              2  /* suspend media service threads or process */
#define DS_MEDIASERVICE_RESUME                               3  /* resume media service threads or process */
#define DS_MEDIASERVICE_EXIT                                 4  /* exit media service threads or process */
#define DS_MEDIASERVICE_THREAD                           0x100  /* start media service as one or more threads */
#define DS_MEDIASERVICE_PROCESS                          0x200  /* start media service as a process */
#define DS_MEDIASERVICE_APP                              0x300  /* start media service as part of the application */
#define DS_MEDIASERVICE_LINEAR                         0x10000  /* assign sessions to available threads in linear arrangement (fully utilize one thread before allocating sessions to another thread). This is the default */
#define DS_MEDIASERVICE_ROUND_ROBIN                    0x20000  /* assign sessions to available threads in round-robin arrangement (assign sessions to threads equally) */
#define DS_MEDIASERVICE_CMDLINE                        0x40000  /* use the szCmdLine param to specify cmd line arguments to create sessions, read/write pcap files, specify buffer add interval, etc. Can be combined with thread, process, or app flags */
#define DS_MEDIASERVICE_PIN_THREADS                    0x80000
#define DS_MEDIASERVICE_SET_NICENESS                  0x100000

#define DS_MEDIASERVICE_ENABLE_THREAD_PROFILING      0x1000000
#define DS_MEDIASERVICE_DISABLE_THREAD_PROFILING     0x1000001

#define DS_MEDIASERVICE_ACTION_MASK                        0xf
#define DS_MEDIASERVICE_TASK_MASK                        0xf00

#define DS_MEDIASERVICE_GET_THREAD_INFO             0x10000000L

/* available flags for DSGetThreadInfo() */

#define DS_THREAD_INFO_NUM_INPUT_PKT_STATS                   1
#define DS_THREAD_INFO_NUM_PULLED_PKT_STATS                  2

#define DS_THREAD_INFO_ITEM_MASK                          0xff

#define DS_THREAD_INFO_PTHREAD_ID                       0x1000  /* specifies the handle param of DSGetThreadInfo() is a pthread_t thread id. By default the handle param is a thread index (from 0 to N-1, where N is current number of active packet/media threads) */


/* DSPullPackets() definitions, also used by DSSendPackets() */

#define DS_PULLPACKETS_JITTER_BUFFER                    0x1000  /* send or pull jitter buffer output packets. Jitter buffer output packets are re-ordered and DTX expanded, as needed */
#define DS_PULLPACKETS_OUTPUT                           0x2000  /* send or pull output packets. Audio transcoded packets and video bitstream packets are available for each channel after decoding */
#define DS_PULLPACKETS_STREAM_GROUP                     0x4000  /* send or pull stream group packets. For example, merged packets are available after decoding, audio merging, and encoding */
#define DS_PULLPACKETS_STREAM_GROUPS                   DS_PULLPACKETS_STREAM_GROUP
#define DS_PULLPACKETS_GET_QUEUE_STATUS                0x10000
#define DS_PULLPACKETS_GET_QUEUE_LEVEL                 0x20000

#if DECLARE_LEGACY_DEFINES
/* legacy define for apps prior to Mar 2019 */
  #define DS_PULL_PACKETS_MERGED DS_PULLPACKETS_STREAM_GROUP
#endif

/* DSPushPackets() definitions (note all are either shared with DSRecvPackets() */

#define DS_PUSHPACKETS_GET_QUEUE_STATUS                0x10000
#define DS_PUSHPACKETS_GET_QUEUE_LEVEL                 0x20000
#define DS_PUSHPACKETS_PAUSE_INPUT                     0x40000
#define DS_PUSHPACKETS_RESTART_INPUT                   0x80000
#define DS_PUSHPACKETS_FULL_PACKET                     DS_BUFFER_PKT_FULL_PACKET
#define DS_PUSHPACKETS_IP_PACKET                       DS_PUSHPACKETS_FULL_PACKET

#define DS_PUSHPACKETS_ENABLE_RFC7198_DEDUP            DS_RECV_PKT_ENABLE_RFC7198_DEDUP  /* discards duplicate packets and sets a "discarded" bit in return code. Added to support non dynamic call situations such as static session config and regular push intervals, JHB Mar2020 */
#define DS_PUSHPACKETS_INIT                            DS_RECV_PKT_INIT

/* DSGetTermChan() definitions */

#define DS_CHECK_CHAN_DELETE_PENDING                         1
#define DS_CHECK_CHAN_EXIST                                  2

/* DSWritePacketStatsHistoryLog() flags (note -- can be combined with DS_PKTSTATS_xx flags in diaglib.h) */

#define DS_PKT_STATS_HISTORY_LOG_THREAD_INDEX       0x10000000  /* treat hSession param as a thread index (0 .. N-1 where N is number of currently active packet/media threads) */
#define DS_PKT_STATS_HISTORY_LOG_RESET_STATS        0x20000000  /* reset packet stats and counters */

/* DSLogRunTimeStats() flags */

#define DS_LOG_RUNTIME_STATS_CONSOLE                         1  /* output run-time stats to console */
#define DS_LOG_RUNTIME_STATS_EVENTLOG                        2  /* output run-time stats to event log file. Note these two flags may be combined */
#define DS_LOG_RUNTIME_STATS_ORGANIZE_BY_STREAM_GROUP     0x10
#define DS_LOG_RUNTIME_STATS_SUPPRESS_ERROR_MSG           DS_PKTLIB_SUPPRESS_WARNING_ERROR_MSG

/* DSDisplayThreadDebugInfo() flags */

#define DS_DISPLAY_THREAD_DEBUG_INFO_SCREEN_OUTPUT           1
#define DS_DISPLAY_THREAD_DEBUG_INFO_EVENT_LOG_OUTPUT        2

#define MAX_DTDI_STR_LEN                                   100

/* error or warning conditions returned by DSGetSessionStatus() */

#define DS_BUFFER_PKT_ERROR_NONE                             0
#define DS_BUFFER_PKT_ERROR_DYNCHAN_MISMATCH                -1
#define DS_BUFFER_PKT_ERROR_DYNCHAN_CREATE                  -2
#define DS_BUFFER_PKT_ERROR_RTP_VALIDATION                  -3
#define DS_BUFFER_PKT_ERROR_SAMPLE_RATE                     -4
#define DS_BUFFER_PKT_ERROR_ADD_FAILED                      -5
#define DS_BUFFER_PKT_SEQ_DUPLICATE                         -6

/* reserved section of pktlib.h */

#if defined(__LIBRARYMODE__) || defined(MEDIATEST_DEV)  /* for dynamic library internal use only. User apps should not define these !! */
  #define USE_PKTLIB_INLINES
  #define DSGetSessionInfo DSGetSessionInfoInline
  #define DSGetJitterBufferInfo DSGetJitterBufferInfoInline
  #define isPmThread isPmThreadInline  /* changed from IsPmThread to isPmThread, JHB Jan 2023 */
#endif

/* DSGetSessionInfoInt2Float() and DSGetSessionInfoInt2Double() used when DSGetSessionInfo() returns a float or double contained inside int64_t */

static inline float DSGetSessionInfoInt2Float(int64_t ival) {

float ret_val;

   memcpy(&ret_val, &ival, sizeof(float));
   return ret_val;
}
   
static inline double DSGetSessionInfoInt2Double(int64_t ival) {

double ret_val;

   memcpy(&ret_val, &ival, sizeof(double));
   return ret_val;
}

typedef bool isPmThread_t(HSESSION hSession, int* pThreadIndex);
 
#ifdef USE_PKTLIB_INLINES

#include <semaphore.h>
#include "shared_include/transcoding.h"
#include "common/lib_priv.h"

#ifndef _USE_CM_  /* set defines needed for call.h struct alignment */
  #define _USE_CM_
#endif
#ifndef _SIGMOD_RTAFv5_
   #define _SIGMOD_RTAFv5_
#endif
#ifndef _MAVMOD_UAG_
  #define _MAVMOD_UAG_
#endif
#include "pktlib/call.h"
#include "diaglib.h"
#include "streamlib.h"
#include "pktlib/rtp_defs.h"
#include "pktlib/rtp.h"

#ifdef __cplusplus
extern "C" {
#endif

int get_group_idx(HSESSION, int, bool, const char*);  /* in streamlib.so */

/* pktlib.so externs */

extern CHANINFO_CORE ChanInfo_Core[];
extern SESSION_CONTROL sessions[];
extern int nPktMediaThreads;
extern PACKETMEDIATHREADINFO packet_media_thread_info[];
extern SESSION_INFO_THREAD session_info_thread[];  /* added Jan 2021, JHB */

/* function to determine if current thread is an application thread (i.e. pktlib API is being called from a user app, not from a packet/media thread). Returns true for app threads */
  
inline bool isPmThreadInline(HSESSION hSession, int* pThreadIndex) {

  #include "isPmThread.c"  /* include isPmThread() source */
}

static inline uint32_t get_session_thread_index(HSESSION hSession) {  /* called by set_session_last_push_time() in packet_flow_media_proc.c (DSPushPackets() calls  set_session_last_push_time()), JHB Jun 2023. Moved here from pktlib.c, JHB Jul 2025 */

  return sessions[hSession].thread_index;
}

#ifdef __cplusplus
}
#endif

/* inline version of DSGetSessionInfo(), compiled if USE_PKTLIB_INLINES is defined */

static inline int64_t DSGetSessionInfoInline(HSESSION sessionHandle, unsigned int uFlags, int64_t term_id, void* pInfo) {

/* more pktlib.so externs */
extern sem_t pktlib_sem;
extern int session_count;

/* streamlib.so externs */
extern STREAM_GROUP stream_groups[];

int n = -1;
int64_t ret_val = -1;
bool no_term_id_arg = false, fMinusOneOk = false;
int session_id = -1;
int i;
int in_use, delete_status;
char handle_str[20];

   if (sessionHandle < 0 || ((uFlags & DS_SESSION_INFO_HANDLE) && sessionHandle >= MAX_SESSIONS) || sessionHandle >= NCORECHAN) {

#if 0
error:
#endif
      if (!(uFlags & DS_SESSION_INFO_SUPPRESS_ERROR_MSG)) Log_RT(2, "ERROR: DSGetSessionInfo() says invalid %s %d, term_id = %d, uFlags = 0x%x. %s:%d \n", uFlags & DS_SESSION_INFO_CHNUM ? "chnum" : "session handle", sessionHandle, term_id, uFlags, __FILE__, __LINE__);
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

         if (!isPmThread(sessionHandle, NULL)) {  /* note -- error message not printed for p/m threads, which handle special cases during the time sessions are marked for deletion and they are actually deleted */

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

      #if 0  /* instead of error condition, now we make session handle the default if neither handle nor channel number is specified, JHB Aug 2023 */
      uFlags |= DS_SESSION_INFO_HANDLE;
      if (sessionHandle >= MAX_SESSIONS) goto error;  /* repeat error check we didn't do initially without DS_SESSION_INFO_HANDLE flag */
      #else
      Log_RT(2, "ERROR: DSGetSessionInfo() says DS_SESSION_INFO_HANDLE or DS_SESSION_INFO_CHNUM must be given, session handle or chnum = %d, term_id = %d, uFlags = 0x%x, %s:%d \n", sessionHandle, term_id, uFlags, __FILE__, __LINE__);
      return -2;
      #endif
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
      case DS_SESSION_INFO_CHNUM:  /* not actually used, just here as a marker. case 0 is used */

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
               else {  /* not found, could be child channel, added JHB Jan 2023 */

                  int nParent = ChanInfo_Core[n].parent_chnum;

                  if ((int)sessions[session_id].term1 == nParent) ret_val = 1;  /* repeat the term check */
                  else if ((int)sessions[session_id].term2 == nParent) ret_val = 2;
               }

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

      case DS_SESSION_INFO_GROUP_IDX:
      case DS_SESSION_INFO_GROUP_OWNER:

         if (uFlags & DS_SESSION_INFO_HANDLE) {

            bool fUseSem = (uFlags & DS_SESSION_INFO_USE_PKTLIB_SEM) != 0;
            int idx = get_group_idx(sessionHandle, 0, fUseSem, NULL);

            if (idx >= 0) {
               if ((uFlags & DS_SESSION_INFO_ITEM_MASK) == DS_SESSION_INFO_GROUP_IDX) ret_val = idx;
               else ret_val = stream_groups[idx].owner_session - 1;
               if (fUseSem) sem_post(&pktlib_sem);
            }

            if (idx == -1) {  /* if this session handle is not the group owner, then see if we can find the owner's session handle using term1 group id */

               idx = get_group_idx(sessionHandle, 1, fUseSem, NULL);

               if (idx >= 0) {
                  if ((uFlags & DS_SESSION_INFO_ITEM_MASK) == DS_SESSION_INFO_GROUP_IDX) ret_val = idx;
                  else ret_val = stream_groups[idx].owner_session - 1;
                  if (fUseSem) sem_post(&pktlib_sem);
               }
            }

            if (idx == -1) {  /* if still not found, try term2 group id */

               idx = get_group_idx(sessionHandle, 2, fUseSem, NULL);

               if (idx >= 0) {
                  if ((uFlags & DS_SESSION_INFO_ITEM_MASK) == DS_SESSION_INFO_GROUP_IDX) ret_val = idx;
                  else ret_val = stream_groups[idx].owner_session - 1;
                  if (fUseSem) sem_post(&pktlib_sem);
               }
            }

            return ret_val;  /* we don't use the standard return code here, which will produce error messages. app code needs to handle the -1 possibility */
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
         else {

            session_id = sessionHandle;

            if (term_id == 1) {
               ret_val = sessions[session_id].session_data.term1.uFlags;
               break;
            }
            else if (term_id == 2) {
               ret_val = sessions[session_id].session_data.term2.uFlags;
               break;
            }
         }

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

            if (term_id == 0) { ret_val = 0; memcpy(&ret_val, &sessions[sessionHandle].session_data.group_term.input_buffer_interval, sizeof(float)); }
            else if (term_id == 1) { ret_val = 0; memcpy(&ret_val, &sessions[sessionHandle].session_data.term1.input_buffer_interval, sizeof(float)); }
            else if (term_id == 2) { ret_val = 0; memcpy(&ret_val, &sessions[sessionHandle].session_data.term2.input_buffer_interval, sizeof(float)); }
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (no_term_id_arg) term_id = 1;
            if (term_id == 1 && ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) { ret_val = 0; memcpy(&ret_val, &ChanInfo_Core[n].term->input_buffer_interval, sizeof(float)); }
            else if (term_id == 2 && ChanInfo_Core[n].link && ChanInfo_Core[n].chan_exists) { ret_val = 0; memcpy(&ret_val, &ChanInfo_Core[n].link->term->input_buffer_interval, sizeof(float)); }
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

         if (ret_val == -1) fMinusOneOk = true;  /* no error if -1, JHB Apr 24 */

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

      case DS_SESSION_INFO_RFC7198_LOOKBACK:

         if (uFlags & DS_SESSION_INFO_HANDLE) {

            if (term_id == 0) ret_val = sessions[sessionHandle].session_data.group_term.RFC7198_lookback;
            else if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.RFC7198_lookback;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.RFC7198_lookback;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (no_term_id_arg) term_id = 1;
            if (term_id == 1 && ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].term->RFC7198_lookback;
            else if (term_id == 2 && ChanInfo_Core[n].link && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].link->term->RFC7198_lookback;
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
            if (term_id == 1) ret_val = sessions[sessionHandle].session_data.term1.voice.rtp_payload_type;
            else if (term_id == 2) ret_val = sessions[sessionHandle].session_data.term2.voice.rtp_payload_type;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (no_term_id_arg) term_id = 1;
            if (term_id == 1 && ChanInfo_Core[n].term && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].term->voice.rtp_payload_type;
            else if (term_id == 2 && ChanInfo_Core[n].link && ChanInfo_Core[n].chan_exists) ret_val = ChanInfo_Core[n].link->term->voice.rtp_payload_type;
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

      case DS_SESSION_INFO_LAST_ACTIVE_CHANNEL:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            ret_val = sessions[sessionHandle].last_active_chan;
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            ret_val = sessions[ChanInfo_Core[n].session_id].last_active_chan;
         }

         break;

      case DS_SESSION_INFO_RTP_CLOCKRATE:

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            ret_val = ChanInfo_Core[sessions[sessionHandle].last_active_chan].rtp_clockrate_decoder;  /* to-do: last_active_chan is a hack, but for now DS_SESSION_INFO_HANDLE is not used. There should probably be a pointer to associated ChanInfo_core[] in TERMINATION_INFO structs, JHB Apr 2024 */
         }
         else if (uFlags & DS_SESSION_INFO_CHNUM) {

            if (n == -1) goto check_n;

            if (term_id == 2) ret_val = ChanInfo_Core[n].link->rtp_clockrate_encoder;  /* look at term_id to decide decoder vs encoder, JHB Sep 2024 */
            else ret_val = ChanInfo_Core[n].rtp_clockrate_decoder;
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

      case DS_SESSION_INFO_GROUP_BUFFER_TIME:  /* note -- value of n is a don't care */

         if (uFlags & DS_SESSION_INFO_HANDLE) {
            ret_val = sessions[sessionHandle].group_buffer_time;
         }
         else { n = -2; goto check_n; }

         break;

      default:
         break;
   }

   if (ret_val != -1 && !fMinusOneOk) {

      if (pInfo != NULL) {

         if ((uFlags & DS_SESSION_INFO_ITEM_MASK) == DS_SESSION_INFO_THREAD) {

            memcpy(pInfo, &packet_media_thread_info[ret_val], sizeof(PACKETMEDIATHREADINFO));  /* copy thread info for this session */
         }
#if 0  /* compiler seems to be buggy when setting these directly, possibly on the link->term item. We fought a nasty bug fight to figure this out and ended up with explicit pointer math */
         else if (session_id >= 0) *(SESSION_DATA*)pInfo = sessions[session_id].session_data;
         else if (term_id == 1) *(TERMINATION_INFO*)pInfo = *ChanInfo_Core[n].term;
         else if (term_id == 2) *(TERMINATION_INFO*)pInfo = *ChanInfo_Core[n].link->term;
#else
         else if (session_id >= 0) {

            if (term_id == 1) memcpy(pInfo, (uint8_t*)&sessions[session_id].session_data.term1, sizeof(TERMINATION_INFO));
            else if (term_id == 2) memcpy(pInfo, (uint8_t*)&sessions[session_id].session_data.term1 + sizeof(TERMINATION_INFO), sizeof(TERMINATION_INFO));  /* for this hack, see comments in DSSetSessionInfo() in pktlib.c, JHB Aug 2018 */
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

      if ((uFlags & DS_SESSION_INFO_CHNUM) && !(uFlags & DS_SESSION_INFO_HANDLE)) strcpy(handle_str, "chnum");
      else strcpy(handle_str, "sessionHandle");

      if (n == -1) {
         if (!(uFlags & DS_SESSION_INFO_SUPPRESS_ERROR_MSG)) Log_RT(2, "ERROR: DSGetSessionInfo() says invalid uFlags -- DS_SESSION_INFO_HANDLE or DS_SESSION_INFO_CHNUM not specified or invalid: flags = 0x%x, %s = %d, n = %d, term_id = %d, ret_val = %d, %s:%d \n", uFlags, handle_str, sessionHandle, n, term_id, ret_val, __FILE__, __LINE__);
         return -2;
      }

      if (n == -2) {
         if (!(uFlags & DS_SESSION_INFO_SUPPRESS_ERROR_MSG)) Log_RT(2, "ERROR: DSGetSessionInfo() says DS_SESSION_INFO_CHNUM specified but only DS_SESSION_INFO_HANDLE is allowed: flags = 0x%x, %s = %d, n = %d, term_id = %d, ret_val = %d, %s:%d \n", uFlags, handle_str, sessionHandle, n, term_id, ret_val, __FILE__, __LINE__);
         return -2;
      }

      if (!(uFlags & DS_SESSION_INFO_SUPPRESS_ERROR_MSG)) Log_RT(2, "ERROR: DSGetSessionInfo() says invalid term info selected: flags = 0x%x, %s = %d, n = %d, term_id = %d, ret_val = %d, %s:%d \n", uFlags, handle_str, sessionHandle, n, term_id, ret_val, __FILE__, __LINE__);
      return -2;
   }
}

static inline int64_t DSGetJitterBufferInfoInline(int chnum, unsigned int uFlags) {

extern RTPCONNECT RTPConnect_Chan[];
extern uint64_t current_allocs, max_allocs;

bool fAllowPostDelete = (uFlags & DS_JITTER_BUFFER_INFO_ALLOW_DELETE_PENDING) != 0;

bool fChanActive = chnum >= 0 && chnum < NCORECHAN && ChanInfo_Core[chnum].chan_exists && (fAllowPostDelete || !ChanInfo_Core[chnum].delete_pending);

   if (!fChanActive) {

      unsigned int uItem = uFlags & DS_JITTER_BUFFER_INFO_ITEM_MASK;

      if (uItem != DS_JITTER_BUFFER_INFO_CURRENT_ALLOCS && uItem != DS_JITTER_BUFFER_INFO_MAX_ALLOCS) return -1;  /* error except for a few items not channel specific, in which case we can ignore chnum (caller should give it as zero), JHB Jan 2023 */
   }

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

      case DS_JITTER_BUFFER_INFO_INPUT_SID_COUNT:  /* JHB Oct 2022 */

         return JitterBuffer->total_input_sid_count;

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

      case DS_JITTER_BUFFER_INFO_PKT_BITRATE_LIST:

         return ChanInfo_Core[chnum].pkt_bitrate_list;

      case DS_JITTER_BUFFER_INFO_CURRENT_ALLOCS:

         return (int64_t)current_allocs;

      case DS_JITTER_BUFFER_INFO_MAX_ALLOCS:

         return (int64_t)max_allocs;

      case DS_JITTER_BUFFER_INFO_NUM_DTMF_PKTS:

         return JitterBuffer->total_input_dtmf_count;

      case DS_JITTER_BUFFER_INFO_PKT_CLASSIFICATION_LIST:

         return ChanInfo_Core[chnum].pkt_classification_list;

      case DS_JITTER_BUFFER_INFO_NUM_TIMESTAMP_SETBACKS:

         return JitterBuffer->timestamp_setback_count;
   }

   return -1;
}

#endif  /* USE_PKTLIB_INLINES */

#else
  #undef _PKTLIB_H_  /* allow additional includes if MIN_HDR defined */
#endif

#endif  /* _PKTLIB_H_ */
