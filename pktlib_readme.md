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
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSReadPcap**](#user-content-dsreadpcap)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSWritePcap**](#user-content-dswritepcap)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSFindPcapPacket**](#user-content-dsfindpcappacket)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSFilterPacket**](#user-content-dsfilterpacket)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSClosePcap**](#user-content-dsclosepcap)<br/>
</sup></sub>

[**_Minimum Push/Pull API Interface_**](#user-content-minimumapiinterface)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSPushPackets**](#user-content-dspushpackets)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**DSPullPackets**](#user-content-dspullpackets)<br/>
</sup></sub>

&nbsp;&nbsp;&nbsp;[**Structs**](#user-content-structs)<br/>

<sub><sup>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**TCP Header Struct**](#user-content-tcpheaderstruct)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**UDP Header Struct**](#user-content-udpheaderstruct)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**RTP Header Struct**](#user-content-rtpheaderstruct)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**PKTINFO Struct**](#user-content-pktinfostruct)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**Pcap API Structs**](#user-content-pcapapistructs)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**Pcap File Header Struct**](#user-content-pcaphdrtstruct)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**Pcapng File Header Struct**](#user-content-pcapnghdrtstruct)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**Pcap Record Struct**](#user-content-pcaprechdrtstruct)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**Pcapng IDB Struct**](#user-content-pcapngidbtstruct)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**Pcapng EPB Struct**](#user-content-pcapngepbtstruct)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**VLAN Header Struct**](#user-content-vlanhdrstruct)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[**FORMAT_PKT Struct**](#user-content-formatpktstruct)<br/>
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

DSGetPacketInfo() retrieves specified packet information as individual items, a PKTINFO struct, or reassembled packet data. Both [mediaMin.cpp](https://www.github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaMin/mediaMin.cpp) and [packet_flow_media_proc.c (packet/media thread processing)](https://www.github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/packet_flow_media_proc.c) contain several examples of DSGetPacketInfo() usage.

```c++
int DSGetPacketInfo(HSESSION      sessionHandle,
                    unsigned int  uFlags,
                    uint8_t*      pkt_buf,
                    int           pkt_buf_len,
                    void*         pInfo,
                    int*          chnum);
```

  * sessionHandle should contain a session handle if uFlags contains a DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flag, which require the packet be verified as matching with sessionHandle as a valid existing session. Otherwise sessionHandle should be set to -1, for any general packet. See additional sessionHandle notes below
  * uFlags should contain one DS_BUFFER_PKT_xxx_PACKET flag and one or more DS_PKT_INFO_xxx flags, defined below. If DS_BUFFER_PKT_IP_PACKET is given the packet should start with an IP header; if DS_BUFFER_PKT_UDP_PACKET or DS_BUFFER_PKT_RTP_PACKET are given the packet should start with a UDP or RTP header. DS_BUFFER_PKT_IP_PACKET is the default if no flag is given. Use DS_PKT_INFO_HOST_BYTE_ORDER if packet headers are in host byte order. Network byte order is the default if no flag (or DS_PKT_INFO_NETWORK_BYTE_ORDER) is given. Byte order flags apply only to headers, not payload contents. See additional uFlags notes below
  * pkt_buf should point to a packet, and pkt_buf_len should contain the length of the packet, in bytes. If a packet length is unknown, pkt_buf_len can be given as -1. Packets may be provided from socket APIs, pcap files, or other sources. The DSOpenPcap() and DSReadPcap() APIs can be used for pcap and pcapng files. The DSFormatPacket() API can be used to help construct a packet
  * pkt_buf_len should contain the length of the packet, in bytes. If a packet length is unknown, pkt_buf_len can be given as -1

  * return value is (i) packet item(s) as specified, (ii) PKT_INFO_RETURN_xxx flags if uFlags includes DS_PKT_INFO_PKTINFO or DS_PKT_INFO_FRAGMENT_xxx, (iii) packet length for reassembled packets, or (iv) < 0 for an error condition (note that some RTP items, such as SSRC, may have legitimate values < 0 when interpreted as a 32-bit int)

Below is more detailed parameter information.

```c++
int DSGetPacketInfo(HSESSION      sessionHandle,  /* additional sessionHandle notes: (i) if both sessionHandle is -1 and uFlags contains DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, DS_PKT_INFO_CHNUM_xxx flags, then all existing sessions will be searched. (ii) SigSRF documentation refers to "user managed sessions", which implies that user applications will store and maintain session handles created by DSCreateSession() */
                    unsigned int  uFlags,         /* one or more of the flags given in Packet Info Flags below. If a DS_PKT_INFO_RTP_xxx flag is given, the corresponding RTP header item is returned. (ii) if DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flags are given, packet headers (plus session handle if user managed sessions are active) are used to match an existing session, after which a codec handle or channel number is returned and associated struct data is copied to pInfo as a TERMINATION_INFO or SESSION_DATA struct if pInfo is not NULL. If non-session-related, general information should be retrieved from the packet, sessionHandle should be given as -1 */
                    uint8_t*      pkt_buf,        /* pkt_buf should point to a byte (uint8_t) array of packet data. Packets may be provided from socket APIs, pcap files, or other sources. The pktlib DSOpenPcap() and DSReadPcapRecord() APIs can be used for pcap and pcapng files. The DSFormatPacket() API can be used to help construct a packet */
                    int           pkt_buf_len,    /* pkt_buf_len should contain the length of the packet, in bytes. If a packet length is unknown, pkt_buf_len can be given as -1 */
                    void*         pInfo,          /* pInfo, if not NULL, on return will contain:
                                                     -a PKTINFO struct (see definition below) if uFlags includes DS_PKT_INFO_PKTINFO
                                                     -an RTPHeader struct (see definition below) if uFlags includes DS_PKT_INFO_RTP_HEADER
                                                     -a fully re-assembled packet if uFlags includes DS_PKT_INFO_REASSEMBLY_GET_PACKET
                                                     -a TERMINATION_INFO or SESSION_DATA struct if uFlags includes a DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flag
                                                  */
                    int* chnum                    /* chnum, if not NULL, will contain a matching channel number when DS_PKT_INFO_CHNUM or DS_PKT_INFO_CHNUM_PARENT are given in uFlags. If the packet matches a child channel number and DS_PKT_INFO_CHNUM_PARENT is given, chnum will contain the child channel number and the parent channel number will be returned */
                   );
```

<a name="PacketInfoFlags"></a>
# Packet Info Flags

Below are flags that can be used in the uFlags param of DSGetPacketInfo()

```c++
/* DSGetPacketInfo() index item uFlags definitions */

#define DS_PKT_INFO_CODEC                     /* these flags specify struct pointer for pInfo arg, either a TERMINATION_INFO struct or a SESSION_DATA struct */
#define DS_PKT_INFO_CODEC_LINK
#define DS_PKT_INFO_SESSION
#define DS_PKT_INFO_CHNUM
#define DS_PKT_INFO_CHNUM_PARENT
#define DS_PKT_INFO_CODEC_TYPE
#define DS_PKT_INFO_CODEC_TYPE_LINK

#define DS_PKT_INFO_INDEX_MASK                /* mask value to isolate above DS_PKT_INFO_xxx index item flags */

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

#define DS_PKT_INFO_RTP_ITEM_MASK             /* mask value to isolate above DS_PKT_INFO_RTP_xxx item flags */

#define DS_PKT_INFO_RTP_HEADER                /* returns whole RTP header in void* pInfo arg */

/* DSGetPacketInfo() IP header item uFlags definitions */

#define DS_PKT_INFO_HDRLEN                    /* returns length of IP address headers (valid for IPv4 and IPv6) */
#define DS_PKT_INFO_PKTLEN                    /* returns total packet length, including IP, UDP, and RTP headers, and payload */
#define DS_PKT_INFO_SRC_PORT
#define DS_PKT_INFO_DST_PORT
#define DS_PKT_INFO_IP_VERSION
#define DS_PKT_INFO_PROTOCOL
#define DS_PKT_INFO_PYLDOFS                   /* returns offset to start of UDP or TCP payload data */
#define DS_PKT_INFO_PYLDLEN                   /* returns size of packet payload. For UDP packets this is the UDP header "Length" field excluding the UDP header size (to include the UDP header add DS_PKT_INFO_PKTINFO_PYLDLEN_INCLUDE_UDP_HDR). For TCP packets this is packet length excluding IP and TCP headers */
#define DS_PKT_INFO_SRC_ADDR                  /* requires pInfo to point to array of sufficient size, returns IP version */
#define DS_PKT_INFO_DST_ADDR

#define DS_PKT_INFO_ITEM_MASK                 /* mask value to isolate above DS_PKT_INFO_xxx item flags */

/* DSGetPacketInfo() PKTINFO struct related uFlags definitions */

#define DS_PKT_INFO_PKTINFO                   /* stores a PKTINFO struct in pInfo (if specified) with a return value of 1 on success, 2 if a fully re-assembled packet is available, and -1 on error condition. This API is intended to minimize packet processing overhead if several packet items are needed. See PKTINFO struct definition above, containing TCP, UDP, and RTP items */

#define DS_PKT_INFO_PKTINFO_EXCLUDE_RTP
#define DS_PKT_INFO_PKTINFO_PYLDLEN_INCLUDE_UDP_HDR

#define DS_PKT_INFO_FRAGMENT_SAVE             /* if packet IP header contains fragmentation info save fragment to pktlib internal fragment list using header's Identification field */
#define DS_PKT_INFO_FRAGMENT_REMOVE           /* if packet IP header contains fragmentation info remove fragment from pktlib internal list using header's Identification field */
#define DS_PKT_INFO_REASSEMBLY_GET_PACKET     /* retrieve fully reassembled packet in pInfo and return the reassembled packet's length. This flag should only be specified if a previous call to DSGetPacketInfo() with DS_PKT_INFO_FRAGMENT_SAVE has returned a DS_PKT_INFO_RETURN_XXX value indicating a fully re-assembled packet is available */

/* the following flags are returned by DSGetPacketInfo() when uFlags contains DS_PKT_INFO_PKTINFO or DS_PKT_INFO_FRAGMENT_xxx flags */

#define DS_PKT_INFO_RETURN_OK                            /* PktInfo struct filled successfully */
#define DS_PKT_INFO_RETURN_FRAGMENT                      /* packet is a fragment */
#define DS_PKT_INFO_RETURN_FRAGMENT_SAVED                /* fragment was saved to pktlib internal list */
#define DS_PKT_INFO_RETURN_FRAGMENT_REMOVED              /* fragment was removed from pktlib internal list */
#define DS_PKT_INFO_RETURN_REASSEMBLED_PACKET_AVAILABLE  /* a fully re-assembled packet is available using DS_PKT_INFO_GET_REASSEMBLED_PACKET in a subsequent DSGetPacketInfo() call */
```

<a name="PcapAPIInterface"></a>
# Pcap API Interface

The pktlib pcap API interface supports read/write to pcap, pcapng, and rtp files.

<a name="DSOpenPcap"></a>
## DSOpenPcap

DSOpenPcap() opens a pcap, pcapng, or rtp/rtpdump file and fills in a pcap_hdr_t struct (see [Structs](#user-content-structs) below), performing basic verification on magic number and supported link layer types.

```c++
int DSOpenPcap(const char*   pcap_file,
               FILE**        fp_pcap,
               pcap_hdr_t*   pcap_file_hdr,
               const char*   errstr,
               unsigned int  uFlags);
```

  * on success, reads the file header(s) and leaves file fp_pcap pointing at the first pcap record, and returns a filled pcap_hdr_t struct pointed to by pcap_file_hdr
  * uFlags options are given in DS_OPEN_PCAP_XXX definitions (see [Pcap API Flags](#user-content-pcapapiflags) below)
  * errstr is optional; if used it should point to an error information string to be included in warning or error messages. NULL indicates not used

  * return value is a 32-bit int formatted as:<br>
      (link_type << 20) | (file_type << 16) | link_layer_length<br>
    where link_type is one of the LINKTYPE_XXX definitions below, file_type is one of the PCAP_TYPE_XXX definitions below, and link_layer_length is the length (in bytes) of link related information preceding the pcap record (typically ranging from 0 to 14)
  
  * note the full return value should be saved and then given as the link_layer_info param in DSReadPcap() and DSFilterPacket()
  * a return value < 0 indicates an error

<a name="DSReadPcap"></a>
## DSReadPcap

DSReadPcap() reads one or more pcap records at the current file position of fp_pcap into pkt_buf, and fills in one or more pcaprec_hdr_t structs (see [Structs](#user-content-structs) below). DSReadPcap() skips over data link layer of each record, reads and interprets vlan header, and fills in structs with returns packet data, timestamp, length.

```c++  
int DSReadPcap(FILE*           fp_pcap,
               uint8_t*        pkt_buf,
               pcaprec_hdr_t*  pcap_pkt_hdr,
               int             link_layer_info,
               uint16_t*       hdr_type,
               pcap_hdr_t*     pcap_file_hdr,
               unsigned int    uFlags);
```

  * pkt_buf should point to a sufficiently large memory area to contain returned packet data
  * link_layer_info should be supplied from a prior DSOpenPcap() call. See DSOpenPcap() comments above
  * if an optional hdr_type pointer is supplied, one or more ETH_P_XXX flags will be returned (as defined in linux/if_ether.h). NULL indicates not used
  * if an optional pcap_file_hdr pointer is supplied, the file header will be copied to this pointer (see pcap_hdr_t struct definition). NULL indicates not used
  * uFlags are given in DS_READ_PCAP_XXX definitions (see [Pcap API Flags](#user-content-pcapapiflags) below)

  * return value is the length of the packet read (in bytes), zero if file end has been reached, or < 0 for an error condition

<a name="PcapAPIFlags"></a>
# Pcap API Flags

Following are definitions used by pktlib pcap APIs

```c++
#define PCAP_TYPE_LIBPCAP                     /* PCAP_TYPE_LIBPCAP and PCAP_TYPE_PCAPNG are returned by DSOpenPcap() in upper 16 bits of return value, depending on file type discovered */
#define PCAP_TYPE_PCAPNG
#define PCAP_TYPE_BER                         /* PCAP_TYPE_BER and PCAP_TYPE_HI3 are used by mediaMin for intermediate packet output */
#define PCAP_TYPE_HI3
#define PCAP_TYPE_RTP

#define PCAP_LINK_LAYER_LEN_MASK              /* return value of DSOpenPcap() contains link type in bits 27-20, file type in bits 19-16, and link layer length in lower 16 bits */
#define PCAP_LINK_LAYER_FILE_TYPE_MASK
#define PCAP_LINK_LAYER_LINK_TYPE_MASK

#ifndef LINKTYPE_ETHERNET                     /* define pcap file link types if needed. We don't require libpcap to be installed */

  #define LINKTYPE_ETHERNET                   /* standard Ethernet Link Layer */
  #define LINKTYPE_LINUX_SLL                  /* Linux "cooked" capture encapsulation */
  #define LINKTYPE_RAW_BSD                    /* Raw IP, OpenBSD compatibility value */
  #define LINKTYPE_RAW                        /* Raw IP */
  #define LINKTYPE_IPV4                       /* Raw IPv4 */
  #define LINKTYPE_IPV6                       /* Raw IPv6 */
#endif

#define DS_OPEN_PCAP_READ                     /* use filelib.h definitions */
#define DS_OPEN_PCAP_WRITE
#define DS_OPEN_PCAP_READ_HEADER
#define DS_OPEN_PCAP_WRITE_HEADER
#define DS_OPEN_PCAP_QUIET
#define DS_OPEN_PCAP_RESET                    /* seek to start of pcap; assumes a valid (already open) file handle given to DSOpenPcap(). Must be combined with DS_OPEN_PCAP_READ */

#define DS_READ_PCAP_COPY                     /* copy pcap record(s) only, don't advance file pointer */
```
<a name="MininumAPIInterface"></a>
# Minimum Push/Pull API Interface

The pktlib minimum API interface supports application level "push" and "pull" to/from packet queues, from which packet/media worker threads receive/send packets for RTP jitter buffer, packet repair, RTP decoding, media domain, and other processing.

<a name="Structs"></a>
# Structs

Following are structs used in pktlib APIs

<a name="TCPHeaderStruct"></a>
## TCP Header

TCP header struct

```c++
  typedef struct {

    uint16_t  SrcPort;         /* source port */
    uint16_t  DstPort;         /* destination port */
    uint32_t  seq_num;         /* sequence number */
    uint32_t  ack_num;         /* ack number */
    uint16_t  hdr_len_misc;    /* header size and flags */
    uint16_t  window;
    uint16_t  checksum;        /* checksum */
    uint16_t  urgent;

  } TCPHeader;
```

<a name="UDPHeaderStruct"></a>
## UDP Header

UDP header struct

```c++
  typedef struct {

    uint16_t  SrcPort;         /* source port */
    uint16_t  DstPort;         /* destination port */
    uint16_t  UDP_length;      /* length */
    uint16_t  UDP_checksum;    /* checksum */

  } UDPHeader;
```

<a name="RTPHeaderStruct"></a>
## RTP Header

RTP header struct

```c++
typedef struct {

 /* Implemented as bit fields:

    -this makes all RTP header fields defined in RFC 3550 directly accessible from C/C++ application code and avoids host vs. network byte ordering issues for first 2 bytes of the RTP header
    -bit fields are in lsb order due to gcc limitations, so ordering within each byte is reversed from msb-first layout defined in RFC 3550
 */

/* 1st byte of RTP header */
  uint8_t   CC        : 4;   /* CSRC count */
  uint8_t   ExtHeader : 1;   /* Extension header */
  uint8_t   Padding   : 1;   /* Padding */
  uint8_t   Version   : 2;   /* RTP version */
/* 2nd byte of RTP header */
  uint8_t   PyldType  : 7;   /* Payload type */
  uint8_t   Marker    : 1;   /* Marker bit */

  uint16_t  Sequence;        /* Sequence number */
  uint32_t  Timestamp;       /* Timestamp */
  uint32_t  SSRC;            /* SSRC */
  uint32_t  CSRC[1];         /* remainder of header, depending on CSRC count and extension header */

} RTPHeader;
```
<a name="PKTINFOStruct"></a>
## PKTINFO Struct

Following is the PKTINFO struct used in DSGetPacketInfo()

```c++
  typedef struct {

     uint8_t             version;
     uint8_t             protocol;
     uint8_t             flags;                   /* one or more DS_PKT_FRAGMENT_XXX flags (see below) */
     int                 pkt_len;
     int                 ip_hdr_len;
     unsigned short int  src_port;
     unsigned short int  dst_port;
     unsigned int        seqnum;                  /* TCP sequence number or UDP/RTP sequence number */
     unsigned int        ack_seqnum;              /* TCP acknowlegement sequence number */
     unsigned int        ip_hdr_checksum;         /* IP header checksum */
     unsigned int        seg_length;              /* TCP segment length */
     int                 pyld_ofs;                /* TCP or UDP payload offset from start of packet to payload data */
     int                 pyld_len;                /* TCP or UDP payload size, excluding UDP header. To include the UDP header add DS_PKT_INFO_PKTINFO_PYLDLEN_INCLUDE_UDP_HDR to uFlags */
     int                 pyld_len_all_fragments;  /* for a packet with MF flag set and no fragment offset, this is the total size of all fragments, not including UDP header */
     unsigned int        udp_checksum;            /* UDP checksum */

  /* RTP items filled for UDP packets. If not a valid RTP packet then RTP items may be undefined. DS_PKT_INFO_PKTINFO_EXCLUDE_RTP can be combined with DS_PKT_INFO_PKTINFO to specify that RTP items should not be processed */

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

  #define PKT_FRAGMENT_MF                         /* set in PKTINFO "flags" if packet MF flag (more fragments) is set */
  #define PKT_FRAGMENT_OFS                        /* set in PKTINFO "flags" if packet fragment offset is non-zero */
  #define PKT_FRAGMENT_ITEM_MASK                  /* mask for fragment related flags */
```

<a name="PcapAPIStructs"></a>
## Pcap API Structs

Following are DSOpenPcap() and DSReadPcap() structs

<a name="pcaphdrtStruct"></a>
```c++
typedef struct pcap_hdr_s {       /* header for standard libpcap format, also for .rtp (.rtpdump) format */

  union {

    struct {                      /* pcap and pcapng format, the default */

      uint32_t magic_number;      /* magic number */
      uint16_t version_major;     /* major version number */
      uint16_t version_minor;     /* minor version number */
      int32_t  thiszone;          /* GMT to local correction */
      uint32_t sigfigs;           /* accuracy of timestamps */
      uint32_t snaplen;           /* max length of captured packets, in octets */
      uint32_t link_type;         /* data link type */
    };

    struct {                      /* add rtp format as a union (https://formats.kaitai.io/rtpdump) */

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
```
<a name="pcapnghdrtStruct"></a>
```c++
typedef struct pcapng_hdr_s {     /* section header block (SHB) for pcapng format */

  uint32_t magic_number;          /* magic number */
  uint32_t block_length;
  uint32_t byte_order_magic;
  uint16_t version_major;         /* major version number */
  uint16_t version_minor;         /* minor version number */
  int64_t  section_length;        /* can be -1 */

} pcapng_hdr_t;
```
<a name="pcapngidbtStruct"></a>
```c++
typedef struct pcapng_idb_s {     /* interface description block (IDB) for pcapng format */

  uint32_t block_type;
  uint32_t block_length;
  uint16_t link_type;
  uint16_t reserved;
  uint32_t snaplen;

} pcapng_idb_t;
```
<a name="pcaprechdrtStruct"></a>
```c++
typedef struct pcaprec_hdr_s {    /* pcap packet (record) header */

  uint32_t ts_sec;                /* timestamp seconds */
  uint32_t ts_usec;               /* timestamp microseconds */
  uint32_t incl_len;              /* number of octets of packet saved in file */
  uint32_t orig_len;              /* actual length of packet */

} pcaprec_hdr_t;
```
<a name="pcapngepbtStruct"></a>
```c++
typedef struct pcapng_epb_s {     /* enhanced packet block (EPB) for pcapng format */

  uint32_t block_type;
  uint32_t block_length;
  uint32_t interface_id;
  uint32_t timestamp_hi;
  uint32_t timestamp_lo;
  uint32_t captured_pkt_len;
  uint32_t original_pkt_len;

} pcapng_epb_t;
```
<a name="vlanhdrStruct"></a>
```c++
typedef struct {                  /* pcap record vlan header */

  uint16_t id;
  uint16_t type;

} vlan_hdr_t;
```

FORMAT_PKT struct, used in DSFormatPacket() API

<a name="formatpktStruct"></a>
```c++
typedef struct {
typedef struct pcaprec_hdr_s {    /* pcap packet (record) header */

  uint8_t    BitFields;           /* Bit fields = Vers:4, Header length:4 */
  uint8_t    Type;                /* Type of Service:8 */
  uint16_t   TotalLength;         /* Total length */
  uint16_t   ID;                  /* Identification */
  uint16_t   FlagFrag;            /* Flag:3, Fragment Offset:13 */
  uint8_t    TimeLive;            /* Time to live, Hop Count for IPv6 */
  uint8_t    Protocol;            /* Protocol */
  uint16_t   HeaderChecksum;      /* Header Checksum */
  uint8_t    TrafficClass;        /* Traffic Class */
  uint32_t   FlowLabel;           /* Flow Label */
  uint16_t   PayloadLength;       /* Size of the payload in octets */
  uint8_t    NextHeader;          /* Next header type */
  uint8_t    SrcAddr[16];         /* IPv4 or IPv6 source addr */
  uint8_t    DstAddr[16];         /* IPv4 or IPv6 dest addr */
  uint32_t   IP_Version;          /* either IPv4 or IPv6 constants defined in pktlib.h or DS_IPV4 or DS_IPV6 enums defined in shared_include/session.h */

  UDPHeader  udpHeader;
  RTPHeader  rtpHeader;
  TCPHeader  tcpHeader;           /* used only if DSFormatPacket() uFlags includes DS_FMT_PKT_TCPIP */

  uint16_t   ptime;

} FORMAT_PKT;
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
