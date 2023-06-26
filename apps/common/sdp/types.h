/*
 SDP parsing and management

 Copyright (c) 2014 Diedrick H, as part of his "SDP" Github repository at https://github.com/diederickh/SDP
 License -- none given. Internet archive page as of 10Jan21 https://web.archive.org/web/20200918222637/https://github.com/diederickh/SDP

 Copyright (c) 2021-2023 Signalogic, Dallas, Texas

 Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Revision History
  Modified Jan 2021 JHB, add a=rtpmap attribute support, see struct AttributeRTP
  Modified Mar 2021 JHB, add num_chan to struct AttributeRTP
  Modified Mar 2021 JHB, add more codec types
  Modified Mar 2021 JHB, add SDP_MEDIA_ANY for use in media element find()
  Modified Jan 2023 JHB, add Node::find() function for Origin objects
  Modified Jan 2023 JHB, handle more codec types
*/

/*
  Types
  -----

  This file contains the structs/nodes that make up a SDP and is based on RFC4566.
  We follow the same naming as the elements as described in RFC4566.
  We implement the nodes in the same order as described in the "SDP specification" chapter in http://tools.ietf.org/html/rfc4566.html (same as https://www.rfc-editor.org/rfc/rfc8866.html)
*/

#ifndef SDP_TYPES_H
#define SDP_TYPES_H

#include <stdint.h>
#include <string>
#include <vector>

namespace sdp {

  enum Type {
    SDP_NONE,
    SDP_SESSION,                        /* a full SDP session */
    SDP_ORIGIN,
    SDP_VERSION,
    SDP_SESSION_NAME,
    SDP_SESSION_INFORMATION,
    SDP_URI,
    SDP_EMAIL_ADDRESS,
    SDP_PHONE_NUMBER,
    SDP_CONNECTION_DATA,
    SDP_TIMING,
    SDP_MEDIA,
    SDP_CANDIDATE,
    SDP_ATTRIBUTE,
    SDP_BANDWIDTH
  };

  enum NetType {
    SDP_NETTYPE_NONE,
    SDP_IN
  };

  enum AddrType {
    SDP_ADDRTYPE_NONE,
    SDP_IP4,
    SDP_IP6
  };

  enum MediaType {
    SDP_MEDIATYPE_NONE,
    SDP_MEDIA_ANY,
    SDP_VIDEO,
    SDP_AUDIO,
    SDP_TEXT,
    SDP_APPLICATION,
    SDP_MESSAGE
  };

  enum MediaProto {
    SDP_MEDIAPROTO_NONE,
    SDP_UDP,
    SDP_RTP_AVP,
    SDP_RTP_SAVP,
    SDP_RTP_SAVPF                    /* http://tools.ietf.org/html/rfc5124 */
  };

  enum CodecType {
    SDP_CODECTYPE_NONE,
    SDP_G711U,
    SDP_G711A,
    SDP_G722,
    SDP_G7221,
    SDP_G726_16,
    SDP_G726_24,
    SDP_G726_32,
    SDP_G726_40,
    SDP_G729,
    SDP_AMRNB,
    SDP_AMRWB,
    SDP_EVS,
    SDP_iLBC,
    SDP_Speex,
    SDP_gsm,
    SDP_SILK,
    SDP_CN,                          /* comfort noise */
    SDP_H264,
    SDP_TELEPHONE_EVENT,
    SDP_TONE                         /* not sure what this is, but seeing it in SIP Invite messages */
  };

  enum AttrType {
    SDP_ATTRTYPE_NONE,
    SDP_ATTR_RTCP,
    SDP_ATTR_KEYWDS,
    SDP_ATTR_TOOL,
    SDP_ATTR_PTIME,
    SDP_ATTR_MAXPTIME,
    SDP_ATTR_RTPMAP,   /* added JHB Feb 2021 */
    SDP_ATTR_RECVONLY,
    SDP_ATTR_SENDRECV,
    SDP_ATTR_SENDONLY,
    SDP_ATTR_INACTIVE,
    SDP_ATTR_ORIENT,
    SDP_ATTR_TYPE,
    SDP_ATTR_CHARSET,
    SDP_ATTR_SDPLANG,
    SDP_ATTR_LANG,
    SDP_ATTR_CANDIDATE,
    SDP_ATTR_ICE_UFRAG,
    SDP_ATTR_ICE_PWD,
    SDP_ATTR_FINGERPRINT,

    /* etc... etc.. */
    SDP_ATTR_UNKNOWN
  };

  enum CandType {
    SDP_CANDTYPE_NONE,
    SDP_HOST,
    SDP_SRFLX,
    SDP_PRFLX,
    SDP_RELAY
  };

  /* forward declared for Node::find() */
  struct Version;
  struct Origin;
  struct Media;
  struct SessionName;
  struct SessionInformation;
  struct Timing;
  struct ConnectionData;
  struct Attribute;
  struct AttributeRTCP;
  struct AttributeCandidate;
  struct AttributeRTP;
  struct Bandwidth;

  struct Node {
  public:
    Node(Type t);
    void addNode(Node* n);
    int print(int* node);
    int find(Type t, std::vector<Node*>& result, int* node);             /* try to find a child node for the given type */
    bool find(MediaType t, Media** result, int* node);                   /* sets result to the first found media element of the given media type */
    int find(AttrType t, std::vector<Attribute*>& result, int* node);    /* find all attributes for the given type */
    int find(Type t, std::vector<Origin*>& result, int* node);

  public:
    Type type;
    std::vector<Node*> nodes;
  };

  /* v= */
  struct Version : public Node {
    Version();
    int version;
  };

  /* o= */
  struct Origin : public Node {
    Origin();

    std::string username;                /* users login, or "-" if user IDs are not supported */
    std::string sess_id;                 /* numeric string that is used as unique identifier, e.g. timestamp, e.g. "621762799816690644" */
    uint64_t sess_version;               /* version number of this SDP, e.g. "1"  */
    NetType net_type;                    /* SDP_IN */
    AddrType addr_type;                  /* SDP_IP4, SDP_IP6 */
    std::string unicast_address;         /* address of the machine from which the session was created, e.g. 127.0.0.1 */
  };

  /* m= */
  struct Media : public Node {
    Media();

    MediaType media_type;
    uint16_t port;
    MediaProto proto;
    int fmt;
  };

  /* s= */
  struct SessionName : public Node {
    SessionName();
    std::string session_name;
  };

  /* i= */
  struct SessionInformation : public Node {
    SessionInformation();
    std::string session_description;
  };

  /* u= */
  struct URI : public Node {
    URI();
    std::string uri;
  };

  /* e= */
  struct EmailAddress : public Node { 
    EmailAddress();
    std::string email_address;
  };

  /* p= */
  struct PhoneNumber : public Node {
    PhoneNumber();
    std::string phone_number;
  };

  /* t= */
  struct Timing : public Node {
    Timing();
    uint64_t start_time;
    uint64_t stop_time;
  };

  /* c= */
  struct ConnectionData : public Node {
    ConnectionData();
    NetType net_type;
    AddrType addr_type;
    std::string connection_address; 
  };

  /* 
     Because the list of attribute types is huge, we create a generic Attribute struct which contains some members that are meant for common types.
     So in general not all members of this struct are always used. The reader will set the members base on the AttrType member.

     a=
  */
  struct Attribute : public Node {
    Attribute();
    AttrType attr_type;
    std::string name;
    std::string value;
  };

  /* e.g. a=rtcp:59976 IN IP4 192.168.0.194 */
  struct AttributeRTCP : public Attribute {
    AttributeRTCP();
    uint16_t port;
    NetType net_type;
    AddrType addr_type;
    std::string connection_address;
  };

  /* e.g. a=rtpmap:96 AMR-WB/16000, a=rtpmap:109 EVS/16000/1, etc. */
  struct AttributeRTP : public Attribute {
    AttributeRTP();
    uint16_t pyld_type;
    CodecType codec_type;
    uint32_t clock_rate;
    uint16_t num_chan;
  };

  /* e.g. a=candidate:4252876256 1 udp 2122260223 192.168.0.194 59976 typ host generation 0 */
  struct AttributeCandidate : public Attribute {
    AttributeCandidate();

    std::string foundation;
    uint64_t component_id;
    std::string transport;
    uint64_t priority;
    std::string connection_address;
    int port;
    CandType cand_type;
    std::string rel_addr;
    uint16_t rel_port;    
  };

  /* b= */
  struct Bandwidth : public Node {
    Bandwidth();
    std::string total_bandwidth_type;
    uint32_t bandwidth;
  };
};

#endif
