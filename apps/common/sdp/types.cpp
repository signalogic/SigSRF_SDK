/*
  SDP parsing and management

  Copyright (c) 2014 Diedrick H, as part of his "SDP" Github repository at https://github.com/diederickh/SDP
  License -- none given. Internet archive page as of 10Jan21 https://web.archive.org/web/20200918222637/https://github.com/diederickh/SDP

  Copyright (c) 2021 Signalogic, Dallas, Texas

  Revision History
    Modified Jan 2021 JHB, re-arranged initializer lists to remove -wReorder warnings
    Modified Jan 2021 JHB, add a=rtpmap attribute support
*/

#include <sdp/types.h>

namespace sdp {

  /* ----------------------------------------------------------- */

  /* generic sdp line */
  Node::Node(Type t)
    :type(t)
  {
  }

  void Node::addNode(Node* n) {
    nodes.push_back(n);
  }

  bool Node::find(Type t, std::vector<Node*>& result) {
    for (size_t i = 0; i < nodes.size(); ++i) {
      if (nodes[i]->type == t) {
        result.push_back(nodes[i]);
      }
    }
    return result.size();
  }

  bool Node::find(MediaType t, Media** m) {
    *m = NULL;
    for (size_t i = 0; i < nodes.size(); ++i) {
      if (nodes[i]->type == SDP_MEDIA) {
        *m = static_cast<Media*>(nodes[i]);
        return true;
      }
    }
    return false;
  }

  bool Node::find(AttrType t, Attribute** a) {
    Attribute* attr;
    *a = NULL;
    for (size_t i = 0; i < nodes.size(); ++i) {
      if (nodes[i]->type == SDP_ATTRIBUTE) {
        attr = static_cast<Attribute*>(nodes[i]);
        if (attr->attr_type == t) {
          *a = static_cast<Attribute*>(nodes[i]);
          return true;
        }
      }
    }
    return false;
  }

  int Node::find(AttrType t, std::vector<Attribute*>& result) {  /* change return type to int so user easily knows how many rtpmaps were found. 0 = none, JHB Jan2021 */
    Attribute* attr;
    for (size_t i = 0; i < nodes.size(); ++i) {
      if (nodes[i]->type == SDP_ATTRIBUTE) {
        attr = static_cast<Attribute*>(nodes[i]);
        if (attr->attr_type == t) {
          result.push_back(attr);
        }
      }
    }
    return result.size();
  }

  /* initializations -- note that items in comments are expected sdp format text, JHB Jan2021 */

  /* v=0 */
  Version::Version():
    Node(SDP_VERSION),
    version(0)
  {
  }

  /* o=- 621762799816690644 7 IN IP4 127.0.0.1 */
  Origin::Origin():
    Node(SDP_ORIGIN),
    sess_version(1),
    net_type(SDP_IN),
    addr_type(SDP_IP4)
  {
  }

  /* s=- */
  SessionName::SessionName():
    Node(SDP_SESSION_NAME)
  {
  }

  /* i= */
  SessionInformation::SessionInformation():
    Node(SDP_SESSION_INFORMATION)
  {
  }

  /* u= */
  URI::URI(): 
    Node(SDP_URI)
  {
  }

  /* e= */
  EmailAddress::EmailAddress():
    Node(SDP_EMAIL_ADDRESS)
  {
  }

  /* p= */
  PhoneNumber::PhoneNumber():
    Node(SDP_PHONE_NUMBER)
  {
  }

  /* c= */
  ConnectionData::ConnectionData():
    Node(SDP_CONNECTION_DATA),
    net_type(SDP_IN),
    addr_type(SDP_IP4)
  {
  }

  /* t=0 0 */
  Timing::Timing():
    Node(SDP_TIMING),
    start_time(0),
    stop_time(0)
  {
  }

  /* m= */
  Media::Media():
    Node(SDP_MEDIA),
    media(SDP_MEDIATYPE_NONE),
    port(0),
    proto(SDP_MEDIAPROTO_NONE),
    fmt(0)
  {
  }

  /* a= */
  Attribute::Attribute():
    Node(SDP_ATTRIBUTE),
    attr_type(SDP_ATTRTYPE_NONE)
  {
  }

  /* a=rtcp: */
  AttributeRTCP::AttributeRTCP():
    Attribute()
  {
    attr_type = SDP_ATTR_RTCP;
  }

  /* a=rtpmap: */
  AttributeRTP::AttributeRTP():
    Attribute()
  {
    attr_type = SDP_ATTR_RTPMAP;
  }

  AttributeCandidate::AttributeCandidate():
    Attribute(),
    component_id(0),
    priority(0),
    port(0),
    rel_port(0)
  {
    attr_type = SDP_ATTR_CANDIDATE;
  }

};
