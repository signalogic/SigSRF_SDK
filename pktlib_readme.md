# SigSRF Pktlib Documentation

<a name="TOC"></a>
## Table of Contents

[**_Overview_**](#user-content-overview)<br/>

[**_API Interface_**](#user-content-apiinterface)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;[**DSGetPacketInfo**](#user-content-dsgetpacketinfo)<br/>
&nbsp;&nbsp;&nbsp;[**DSBufferPackets**](#user-content-dsbufferpackets)<br/>
&nbsp;&nbsp;&nbsp;[**DSGetOrderedPackets**](#user-content-dsgetorderedpackets)<br/>
&nbsp;&nbsp;&nbsp;[**DSFormatPacket**](#user-content-dsformatpacket)<br/>
</sup></sub>

[**_Pcap API Interface_**](#user-content-pcapapiinterface)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;[**DSOpenPcap**](#user-content-dsopenpcap)<br/>
&nbsp;&nbsp;&nbsp;[**DSReadPcap**](#user-content-dsreapcap)<br/>
&nbsp;&nbsp;&nbsp;[**DSWritePcap**](#user-content-dsbufferpackets)<br/>
&nbsp;&nbsp;&nbsp;[**DSFindPcapPacket**](#user-content-dsbufferpackets)<br/>
&nbsp;&nbsp;&nbsp;[**DSFilterPacket**](#user-content-dsbufferpackets)<br/>
&nbsp;&nbsp;&nbsp;[**DSClosePcap**](#user-content-dsgetorderedpackets)<br/>
</sup></sub>

[**_Minimum API Interface_**](#user-content-minimumapiinterface)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;[**DSPushPackets**](#user-content-dspushpackets)<br/>
&nbsp;&nbsp;&nbsp;[**DSPullPackets**](#user-content-dspullpackets)<br/>
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

<a name="Overview"></a>
# Overview

This page documents the SigSRF pktlib, or packet library. Pktlib contains a number of generic, useful APIs for handling IP packets and pcap, pcapng, and rtp files, with emphasis on RTP media. The deployment / production grade mediaMin and mediaTest apps rely on pktlib for high-performance, multi-threaded, stable operation.

Not all pktlib APIs are included here yet, so this is a work in progress. But the number of developers using pktlib has increased greatly since 2021, so this page is needed and should be helpful.

<a name="APIInterface"></a>
# API Interface

The following APIs and structs are defined in [pktlib.h](https://www.github.com/signalogic/SigSRF_SDK/blob/master/includes/pktlib.h).

<a name="DSGetPacketInfo"></a>
## DSGetPacketInfo

  * retrieves packet information
  * sessionHandle should contain a session handle if uFlags contains a DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flag, which require the packet be verified as matching with sessionHandle as a valid existing session. Otherwise sessionHandle should be set to -1, for any general packet. See additional sessionHandle notes below
  * uFlags should contain one DS_BUFFER_PKT_xxx_PACKET flag and one or more DS_PKT_INFO_xxx flags, defined below. If the DS_BUFFER_PKT_IP_PACKET flag is given the packet should start with an IP header; if the DS_BUFFER_PKT_UDP_PACKET or DS_BUFFER_PKT_RTP_PACKET flags are given the packet should start with a UDP or RTP header. DS_BUFFER_PKT_IP_PACKET is the default if no flag is given. Use the DS_PKT_INFO_HOST_BYTE_ORDER flag if packet headers are in host byte order. Network byte order is the default if no flag is given (or the DS_PKT_INFO_NETWORK_BYTE_ORDER flag can be given). Byte order flags apply only to headers, not payload contents. See additional uFlags notes below
  * returns packet items as specified, or < 0 for an error condition (note that some RTP items, such as SSRC, may have legitimate values < 0 when interpreted as a 32-bit int)
  * both [mediaMin.cpp](https://www.github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaMin/mediaMin.cpp) and [packet_flow_media_proc.c (packet/media thread processing)](https://www.github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/packet_flow_media_proc.c) contain many examples of DSGetPacketInfo() usage

```c++
int DSGetPacketInfo(HSESSION sessionHandle,  /* additional sessionHandle notes: (i) if both sessionHandle is -1 and uFlags contains a DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flag, then all existing sessions will be searched. (ii) SigSRF documentation often refers to "user managed sessions", which implies that user applications will store and maintain session handles created by DSCreateSession() */
                    unsigned int uFlags,     /* additional uFlags notes: (i) if a DS_PKT_INFO_RTP_xxx flag is given, the corresponding RTP header item is returned. (ii) if uFlags values of DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx are given, packet headers (plus session handle if user managed sessions are active) are used to match an existing session, after which a codec handle or channel number is returned and associated struct data is copied to pInfo as a TERMINATION_INFO or SESSION_DATA struct if pInfo is not NULL. If non-session-related, general information should be retrieved from the packet, sessionHandle should be given as -1 */
                    uint8_t* pkt,            /* pkt should point to a byte (uint8_t) array of packet data. Packets may be provided from socket APIs, pcap files, or other sources. The DSOpenPcap() and DSReadPcapRecord() APIs can be used for pcap and pcapng files. The DSFormatPacket() API can be used to help construct a packet */
                    int pktlen,              /* pktlen should contain the length of the packet, in bytes. If a packet length is unknown, pktlen can be given as -1 */
                    void* pInfo,             /* pInfo, if not NULL, will contain either (i) a PKTINFO struct (see definition below) if uFlags includes the DS_PKT_INFO_PKTINFO flag or (ii) an RTPHeader struct (see definition above) if uFlags includes the DS_PKT_INFO_RTP_HEADER flag */
                    int* chnum               /* chnum, if not NULL, will contain a matching channel number when the DS_PKT_INFO_CHNUM or DS_PKT_INFO_CHNUM_PARENT flags are given. If the packet matches a child channel number and the DS_PKT_INFO_CHNUM_PARENT flag is given, chnum will contain the child channel number and the parent channel number will be returned */
                   );
```

/* PKTINFO struct filled by DSGetPacketInfo() when uFlags includes the DS_PKT_INFO_PKTINFO flag. A PKTINFO struct param is also used by DSFindPcapPacket() */

  typedef struct {

     uint8_t             version;
     uint8_t             protocol;
     uint8_t             flags;
     int                 pkt_len;
     int                 ip_hdr_len;
     unsigned short int  src_port;
     unsigned short int  dst_port;
     unsigned int        seqnum;          /* TCP sequence number or UDP/RTP sequence number */
     unsigned int        ack_seqnum;      /* TCP acknowlegement sequence number */
     int                 pyld_ofs;        /* TCP or UDP payload offset */
     int                 pyld_len;        /* TCP or UDP payload length */
     int                 rtp_hdr_ofs;     /* RTP items filled for UDP packets. If not a valid RTP packet then RTP items may be undefined */
     int                 rtp_hdr_len;
     int                 rtp_pyld_ofs;
     int                 rtp_pyld_len;
     uint8_t             rtp_pyld_type;
     int                 rtp_padding_len;
     uint32_t            rtp_timestamp;
     uint32_t            rtp_ssrc;

  } PKTINFO;

/* PKTINFO flags definitions */

  #define PKT_FRAGMENT  1  /* set in PKTINFO "flags" if either of packet's MF flag (more fragments) or fragment offset are non-zero */
