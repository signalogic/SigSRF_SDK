# SigSRF Pktlib Documentation

<a name="TOC"></a>
## Table of Contents

[**_Overview_**](#user-content-overview)<br/>

[**_API Interface_**](#user-content-apiinterface)<br/>

[**_Packet API Interface_**](#user-content-packetapiinterface)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSGetPacketInfo**](#user-content-dsgetpacketinfo)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSBufferPackets**](#user-content-dsbufferpackets)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSGetOrderedPackets**](#user-content-dsgetorderedpackets)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSFormatPacket**](#user-content-dsformatpacket)<br/>
</sup></sub>

[**_Pcap API Interface_**](#user-content-pcapapiinterface)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSOpenPcap**](#user-content-dsopenpcap)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSReadPcap**](#user-content-dsreapcap)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSWritePcap**](#user-content-dsbufferpackets)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSFindPcapPacket**](#user-content-dsbufferpackets)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSFilterPacket**](#user-content-dsbufferpackets)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSClosePcap**](#user-content-dsgetorderedpackets)<br/>
</sup></sub>

[**_Minimum Push/Pull API Interface_**](#user-content-minimumapiinterface)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSPushPackets**](#user-content-dspushpackets)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSPullPackets**](#user-content-dspullpackets)<br/>
</sup></sub>

&nbsp;&nbsp;&nbsp;[**Structs**](#user-content-structs)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**TCPHeader**](#user-content-tcpheader)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**UDPHeader**](#user-content-udpheader)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**RTPHeader**](#user-content-rtpheader)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**PKTINFO**](#user-content-pktinfo)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**FORMAT_PKT**](#user-content-formatpkt)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**pcap_hdr_t**](#user-content-pcaphdrt)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**pcapng_hdr_t**](#user-content-pcapnghdrt)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**pcapng_idb_t**](#user-content-pcapngidbt)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**pcapng_epb_t**](#user-content-pcapngepbt)<br/>
</sup></sub>

&nbsp;&nbsp;&nbsp;[**General Pktlib API Flags**](#user-content-GeneralPktlibAPIFlags)<br/>

<a name="Overview"></a>
# Overview

This page documents the SigSRF pktlib, or packet library. Pktlib contains a number of generic, useful APIs for handling IP packets and pcap, pcapng, and rtp files, with emphasis on RTP media. The deployment / production grade mediaMin and mediaTest apps rely on pktlib for high-performance, multi-threaded, stable operation.

Not all pktlib APIs are included here yet, so this is a work in progress. But the number of developers using pktlib has increased greatly since 2021, so this page is needed and should be helpful.

<a name="APIInterface"></a>
# API Interface

The following APIs and structs are defined in [pktlib.h](https://www.github.com/signalogic/SigSRF_SDK/blob/master/includes/pktlib.h).

The pktlib API is large, so here it's divided into the following groups:

  * [Packet API Interface](#user-content-packetapiinterface)
  * [Pcap API Interface](#user-content-pcapapiinterface)
  * [Minimum Push/Pull Interface](#user-content-minimumapiinterface)

<a name="PacketAPIInterface"></a>
# Packet API Interface

<a name="DSGetPacketInfo"></a>
## DSGetPacketInfo

  * retrieves packet information
  * pkt_buf should point to a packet, and pktlen should contain the length of the packet, in bytes. If a packet length is unknown, pktlen can be given as -1. Packets may be provided from socket APIs, pcap files, or other sources. The DSOpenPcap() and DSReadPcap() APIs can be used for pcap and pcapng files. The DSFormatPacket() API can be used to help construct a packet
  * sessionHandle should contain a session handle if uFlags contains a DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flag, which require the packet be verified as matching with sessionHandle as a valid existing session. Otherwise sessionHandle should be set to -1, for any general packet. See additional sessionHandle notes below
  * uFlags should contain one DS_BUFFER_PKT_xxx_PACKET flag and one or more DS_PKT_INFO_xxx flags, defined below. If the DS_BUFFER_PKT_IP_PACKET flag is given the packet should start with an IP header; if the DS_BUFFER_PKT_UDP_PACKET or DS_BUFFER_PKT_RTP_PACKET flags are given the packet should start with a UDP or RTP header. DS_BUFFER_PKT_IP_PACKET is the default if no flag is given. Use the DS_PKT_INFO_HOST_BYTE_ORDER flag if packet headers are in host byte order. Network byte order is the default if no flag is given (or the DS_PKT_INFO_NETWORK_BYTE_ORDER flag is given). Byte order flags apply only to headers, not payload contents. See additional uFlags notes below
  * return value is the packet item(s) as specified, PKT_INFO_RET_XXX flags if uFlags includes DS_PKT_INFO_PKTINFO or DS_PKT_INFO_FRAGMENT_xxx flags, packet length for reassembled packets, or < 0 for an error condition (note that some RTP items, such as SSRC, may have legitimate values < 0 when interpreted as a 32-bit int)
  * both [mediaMin.cpp](https://www.github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaMin/mediaMin.cpp) and [packet_flow_media_proc.c (packet/media thread processing)](https://www.github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/packet_flow_media_proc.c) contain many examples of DSGetPacketInfo() usage

```c++
int DSGetPacketInfo(HSESSION sessionHandle,  /* additional sessionHandle notes: (i) if both sessionHandle is -1 and uFlags contains a DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flag, then all existing sessions will be searched. (ii) SigSRF documentation often refers to "user managed sessions", which implies that user applications will store and maintain session handles created by DSCreateSession() */
                    unsigned int uFlags,     /* additional uFlags notes: (i) if a DS_PKT_INFO_RTP_xxx flag is given, the corresponding RTP header item is returned. (ii) if uFlags values of DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx are given, packet headers (plus session handle if user managed sessions are active) are used to match an existing session, after which a codec handle or channel number is returned and associated struct data is copied to pInfo as a TERMINATION_INFO or SESSION_DATA struct if pInfo is not NULL. If non-session-related, general information should be retrieved from the packet, sessionHandle should be given as -1 */
                    uint8_t* pkt_buf,        /* pkt_buf should point to a byte (uint8_t) array of packet data. Packets may be provided from socket APIs, pcap files, or other sources. The DSOpenPcap() and DSReadPcapRecord() APIs can be used for pcap and pcapng files. The DSFormatPacket() API can be used to help construct a packet */
                    int pkt_buf_len,         /* pkt_buf_len should contain the length of the packet, in bytes. If a packet length is unknown, pktlen can be given as -1 */
                    void* pInfo,             /* pInfo, if not NULL, will contain
                                                -a PKTINFO struct (see definition below) if uFlags includes the DS_PKT_INFO_PKTINFO flag
                                                -an RTPHeader struct (see definition above) if uFlags includes the DS_PKT_INFO_RTP_HEADER flag
                                                -a fully re-assembled packet if uFlags includes the DS_PKT_INFO_REASSEMBLY_GET_PACKET flag
                                                -a TERMINATINO_INFO or SESSION_DATA struct if uFlags includes DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flags
                                             */
                    int* chnum               /* chnum, if not NULL, will contain a matching channel number when the DS_PKT_INFO_CHNUM or DS_PKT_INFO_CHNUM_PARENT flags are given. If the packet matches a child channel number and the DS_PKT_INFO_CHNUM_PARENT flag is given, chnum will contain the child channel number and the parent channel number will be returned */
                   );
```

<a name="PacketInfoFlags"></a>
# Packet Info Flags

Below are flags that can be used in the uFlags param of DSGetPacketInfo():

```c++
/* DSGetPacketInfo() index item uFlags definitions */

#define DS_PKT_INFO_CODEC                     /* these flags specify struct pointer for pInfo arg, either a TERMINATION_INFO struct or a SESSION_DATA struct */
#define DS_PKT_INFO_CODEC_LINK
#define DS_PKT_INFO_SESSION
#define DS_PKT_INFO_CHNUM
#define DS_PKT_INFO_CHNUM_PARENT
#define DS_PKT_INFO_CODEC_TYPE
#define DS_PKT_INFO_CODEC_TYPE_LINK

#define DS_PKT_INFO_INDEX_MASK                /* mask value to isolate above DS_PKT_INFO_XXX index item flags */

/* DSGetPacketInfo() RTP item uFlags definitions */

#define DS_PKT_INFO_RTP_VERSION
#define DS_PKT_INFO_RTP_PYLDTYPE
#define DS_PKT_INFO_RTP_MARKERBIT
#define DS_PKT_INFO_RTP_HDROFS                /* retrieves offset to start of RTP header (assumes a UDP packet) */
#define DS_PKT_INFO_RTP_PADDING_SIZE          /* retrieves RTP payload padding size */
#define DS_PKT_INFO_RTP_SEQNUM
#define DS_PKT_INFO_RTP_TIMESTAMP
#define DS_PKT_INFO_RTP_SSRC
#define DS_PKT_INFO_RTP_PYLDOFS               /* retrieves offset to start of RTP payload */
#define DS_PKT_INFO_RTP_PYLDLEN
#define DS_PKT_INFO_RTP_PYLD_CONTENT          /* retrieves content type, not payload data. Use either DS_PKT_INFO_PYLDOFS or DS_PKT_INFO_RTP_PYLDOFS to get offset to start of packet data */
#define DS_PKT_INFO_RTP_HDRLEN                /* retrieves RTP header length, including extensions if any */

#define DS_PKT_INFO_RTP_ITEM_MASK             /* mask value to isolate above DS_PKT_INFO_RTP_XXX item flags */

#define DS_PKT_INFO_RTP_HEADER                /* returns whole RTP header in void* pInfo arg */

/* DSGetPacketInfo() IP header item uFlags definitions */

#define DS_PKT_INFO_HDRLEN                    /* returns length of IP address headers (valid for IPv4 and IPv6) */
#define DS_PKT_INFO_PKTLEN                    /* returns total packet length, including IP, UDP, and RTP headers, and payload */
#define DS_PKT_INFO_SRC_PORT
#define DS_PKT_INFO_DST_PORT
#define DS_PKT_INFO_IP_VERSION
#define DS_PKT_INFO_PROTOCOL
#define DS_PKT_INFO_PYLDOFS                   /* returns offset to start of UDP or TCP payload data */
#define DS_PKT_INFO_PYLDLEN                   /* returns size of packet payload. For UDP packets this is the UDP header "Length" field excluding the UDP header size (to include the UDP header add the DS_PKT_INFO_PKTINFO_PYLDLEN_INCLUDE_UDP_HDR flag). For TCP packets this is packet length excluding IP and TCP headers */
#define DS_PKT_INFO_SRC_ADDR                  /* requires pInfo to point to array of sufficient size, returns IP version */
#define DS_PKT_INFO_DST_ADDR

#define DS_PKT_INFO_ITEM_MASK                 /* mask value to isolate above DS_PKT_INFO_XXX item flags */

/* DSGetPacketInfo() PKTINFO struct related uFlags definitions */

#define DS_PKT_INFO_PKTINFO                   /* stores a PKTINFO struct in pInfo (if specified) with a return value of 1 on success, 2 if a fully re-assembled packet is available, and -1 on error condition. This API is intended to minimize packet processing overhead if several packet items are needed. See PKTINFO struct definition above, containing TCP, UDP, and RTP items */

#define DS_PKT_INFO_PKTINFO_EXCLUDE_RTP
#define DS_PKT_INFO_PKTINFO_PYLDLEN_INCLUDE_UDP_HDR

#define DS_PKT_INFO_FRAGMENT_SAVE             /* if packet IP header contains fragmentation info save fragment to pktlib internal fragment list using header's Identification field */
#define DS_PKT_INFO_FRAGMENT_REMOVE           /* if packet IP header contains fragmentation info remove fragment from pktlib internal list using header's Identification field */
#define DS_PKT_INFO_REASSEMBLY_GET_PACKET     /* retrieve fully reassembled packet in pInfo and return the reassembled packet's length. This flag should only be specified if a previous call to DSGetPacketInfo() with the DS_PKT_INFO_FRAGMENT_SAVE flag has returned a flag value indicating a fully re-assembled packet is available */

/* DSGetPacketInfo() return flags when uFlags contains DS_PKT_INFO_PKTINFO or DS_PKT_INFO_FRAGMENT_xxx flags */

#define DS_PKT_INFO_RETURN_OK                           /* PktInfo struct filled successfully */
#define DS_PKT_INFO_RETURN_FRAGMENT                      /* packet is a fragment */
#define DS_PKT_INFO_RETURN_FRAGMENT_SAVED                /* fragment was saved to pktlib internal list */
#define DS_PKT_INFO_RETURN_FRAGMENT_REMOVED              /* fragment was removed from pktlib internal list */
#define DS_PKT_INFO_RETURN_REASSEMBLED_PACKET_AVAILABLE  /* a fully re-assembled packet is available using the DS_PKT_INFO_GET_REASSEMBLED_PACKET flag in a subsequent DSGetPacketInfo() call */

```

<a name="PcapAPIInterface"></a>
# Pcap API Interface

The pktlib pcap API interface supports read/write to pcap, pcapng, and rtp files.

<a name="MininumAPIInterface"></a>
# Minimum Push/Pull API Interface

The pktlib minimum API interface supports application level "push" and "pull" to/from packet queues, from which packet/media worker threads receive/send packets for RTP jitter buffer, packet repair, RTP decoding, media domain, and other processing.

<a name="Structs"></a>
# Structs

```c++
  typedef struct {

     uint8_t             version;
     uint8_t             protocol;
     uint8_t             flags;
     int                 pkt_len;
     int                 ip_hdr_len;
     unsigned short int  src_port;
     unsigned short int  dst_port;
     unsigned int        seqnum;                  /* TCP sequence number or UDP/RTP sequence number */
     unsigned int        ack_seqnum;              /* TCP acknowlegement sequence number */
     unsigned int        ip_hdr_checksum;         /* IP header checksum */
     unsigned int        seg_length;              /* TCP segment length */
     int                 pyld_ofs;                /* TCP or UDP payload offset from start of packet to payload data */
     int                 pyld_len;                /* TCP or UDP payload size, excluding UDP header. To include the UDP header add the DS_PKT_INFO_PKTINFO_PYLDLEN_INCLUDE_UDP_HDR flag to uFlags */
     int                 pyld_len_all_fragments;  /* for a packet with MF flag set and no fragment offset, this is the total size of all fragments, not including UDP header */
     unsigned int        udp_checksum;            /* UDP checksum */

  /* RTP items filled for UDP packets. If not a valid RTP packet then RTP items may be undefined. The DS_PKT_INFO_PKTINFO_EXCLUDE_RTP flag can be combined with the DS_PKT_INFO_PKTINFO flag to specify that RTP items should not be processed */

     int                 rtp_hdr_ofs;             /* offset from start of packet to RTP header */
     int                 rtp_hdr_len;
     int                 rtp_pyld_ofs;            /* offset from start of packet to RTP payload data */
     int                 rtp_pyld_len;
     uint8_t             rtp_pyld_type;
     int                 rtp_padding_len;
     uint32_t            rtp_timestamp;
     uint32_t            rtp_ssrc;
     uint16_t            rtp_seqnum;

  } PKTINFO;

/* PKTINFO flags definitions */

  #define PKT_FRAGMENT_MF         1               /* set in PKTINFO "flags" if packet MF flag (more fragments) is set */
  #define PKT_FRAGMENT_OFS        2               /* set in PKTINFO "flags" if packet fragment offset is non-zero */
  #define PKT_FRAGMENT_ITEM_MASK  7               /* mask for fragment related flags */
```

<a name="General Pktlib API Flags"></a>
# General Pktlib API Flags

Below are general pktlib API flags, for use with the uFlags param in DSGetPacketInfo(), DSFormatPacket(), DSBufferPackets(), and DSGetOrderedPackets(). API specific flags are included in their API descriptions.

```c++
#define DS_BUFFER_PKT_IP_PACKET                   /* indicates pkt_buf points to full IP header followed by TCP or UDP packet data */
#define DS_BUFFER_PKT_UDP_PACKET                  /* indicates pkt_buf points to a UDP header followed by a UDP payload (for example, a UDP defined protocol such as RTP, GTP, etc) */
#define DS_BUFFER_PKT_RTP_PACKET                  /* incdicates pkt_buf points to an RTP header followed by an RTP payload */

#define DS_PKTLIB_NETWORK_BYTE_ORDER              /* indicates packet header data is in network byte order. The byte order flags apply only to headers, not payload contents. This flag is zero as the default (no flag) is network byte order, and is defined here only for documentation purposes */
#define DS_PKTLIB_HOST_BYTE_ORDER                 /* indicates packet header data is in host byte order. The byte order flags apply only to headers, not payload contents. Default (no flag) is network byte order */
#define DS_PKTLIB_SUPPRESS_ERROR_MSG              /* suppress general packet format error messages */
#define DS_PKTLIB_SUPPRESS_RTP_ERROR_MSG          /* suppress RTP related error messages */
```
