

/* Packet info API

    -DSGetPacketInfo() retrieves information about packets. Notes:

      -sessionHandle should contain a session handle if uFlags contains a DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flag, which require the packet be verified as matching with sessionHandle as a valid existing session. Otherwise sessionHandle should be set to -1, for any general packet. Some additional sessionHandle notes:

        -if both sessionHandle is -1 and uFlags contains a DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx flag, then all existing sessions will be searched

        -SigSRF documentation often refers to "user managed sessions", which implies that user applications will store and maintain session handles created by DSCreateSession()

      -pkt should point to a packet, and pktlen should contain the length of the packet, in bytes. If a packet length is unknown, pktlen can be given as -1. Packets may be provided from socket APIs, pcap files, or other sources. The DSOpenPcap() and DSReadPcapRecord() APIs can be used for pcap and pcapng files. The DSFormatPacket() API can be used to help construct a packet

      -uFlags should contain one DS_BUFFER_PKT_xxx_PACKET flag and one or more DS_PKT_INFO_xxx flags, defined below. If the DS_BUFFER_PKT_IP_PACKET flag is given the packet should start with an IP header; if the DS_BUFFER_PKT_UDP_PACKET or DS_BUFFER_PKT_RTP_PACKET flags are given the packet should start with a UDP or RTP header. DS_BUFFER_PKT_IP_PACKET is the default no flag is given. Use the DS_PKT_INFO_HOST_BYTE_ORDER flag if packet headers are in host byte order. Network byte order is the default if no flag is given (or the DS_PKT_INFO_NETWORK_BYTE_ORDER flag can be given). Byte order flags apply only to headers, not payload contents

      -if a DS_PKT_INFO_RTP_xxx flag is given, the corresponding RTP header item is returned

      -if uFlags values of DS_PKT_INFO_SESSION_xxx, DS_PKT_INFO_CODEC_xxx, or DS_PKT_INFO_CHNUM_xxx are given, packet headers (plus session handle if user managed sessions are active) are used to match an existing session, after which a codec handle or channel number is returned and associated struct data is copied to pInfo as a TERMINATION_INFO or SESSION_DATA struct if pInfo is not NULL. If non-session-related, general information should be retrieved from the packet, sessionHandle should be given as -1

      -pInfo, if not NULL, will contain either (i) a PKTINFO struct (see definition below) if uFlags includes the DS_PKT_INFO_PKTINFO flag or (ii) an RTPHeader struct (see definition above) if uFlags includes the DS_PKT_INFO_RTP_HEADER flag

      -chnum, if not NULL, will contain a matching channel number when the DS_PKT_INFO_CHNUM or DS_PKT_INFO_CHNUM_PARENT flags are given. If the packet matches a child channel number and the DS_PKT_INFO_CHNUM_PARENT flag is given, chnum will contain the child channel number and the parent channel number will be returned

      -return value is < 0 for an error condition, or as specified in the above notes. Note that some RTP items, such as SSRC, may have legitimate values < 0 when interpreted as a 32-bit int

      -both mediaMin.cpp and packet_flow_media_proc.c contain many examples of DSGetPacketInfo() usage
 */

  typedef int (DSGetPacketInfo_t)(HSESSION sessionHandle, unsigned int uFlags, uint8_t* pkt, int pktlen, void* pInfo, int* chnum);

  #ifndef GET_PKT_INFO_TYPEDEF_ONLY  /* app or lib can specify no prototype for DSGetPacketInfo() if it will be doing its own dlsym() lookup, JHB May 2024 */
  DSGetPacketInfo_t DSGetPacketInfo;
  #endif

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
