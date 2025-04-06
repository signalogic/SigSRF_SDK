/*
 SDP info parsing and management

 Copyright (c) 2014 Diedrick H, as part of his "SDP" Github repository at https://github.com/diederickh/SDP
 License -- none given. Internet archive page as of 10Jan21 https://web.archive.org/web/20200918222637/https://github.com/diederickh/SDP

 Copyright (c) 2021-2025 Signalogic, Dallas, Texas

 Revision History

  Modified Jan 2021 JHB, re-arranged initializer lists to remove -wReorder warnings
  Modified Jan 2021 JHB, add a=rtpmap attribute support
  Modified Mar 2021 JHB, add b=bandwidth field support
  Modified Mar 2021 JHB, add *node param for some find() calls, which specifies start node and returns found node (NULL = start at 0). This allows finding/handling Media objects in sequence
  Modified Mar 2021 JHB, fix bug in Media object find() where media type parameter was ignored
  Modified Jan 2023 JHB, add Node::find() function for Origin objects. See notes below on find() functions
  Modified Feb 2025 JHB, add fmtp parsing
*/

#include <sdp/types.h>

namespace sdp {

/* generic sdp line */

  Node::Node(Type t)
    :type(t) { }

  void Node::addNode(Node* n) {
    nodes.push_back(n);
  }

  int Node::print(int* node) {
    for (size_t i = node ? *node : 0; i < nodes.size(); ++i) fprintf(stderr, " nnn node[%d] type = %d \n", (int)i, nodes[i]->type);
    return nodes.size();
  }

/* Notes on Node::find() functions, Jan 2023:

   -I haven't figured out yet how to consolidate into a unified, flexible find(). C++ makes object typecasting difficult, although in theory it should be do-able
   -so far the generic find() with a Nodes vector has no usage (first one below)
   -a separate find() for vectors of Media, Attribute, and Origin objects are called by SDPAdd() in sdp_app.cpp
*/

  int Node::find(Type t, std::vector<Node*>& result, int* node) {

    for (size_t i = node ? *node : 0; i < nodes.size(); ++i) {
      if (nodes[i]->type == t) result.push_back(nodes[i]);
    }

    return result.size();
  }

/* find one or more Media objects */

  bool Node::find(MediaType t, Media** m, int* node) {

    if (!m) { fprintf(stderr, "sdp: Media** is NULL, node = %d \n", node ? *node : 0); return false; }  /* add error check, JHB Feb 2025 */

    *m = NULL;  /* initialize to no media element found */

 #if 0
 printf(" ++ inside media node:find, *node = %d, size = %d \n", node ? *node : 0, (int)nodes.size());
 #endif

    for (size_t i = node ? *node : 0; i < nodes.size(); ++i) {

  //printf(" node type[%lu] = %d \n", i, nodes[i]->type);

      if (nodes[i]->type == SDP_MEDIA) {
        if (t == SDP_MEDIA_ANY || (static_cast<Media*>(nodes[i]))->media_type == t) {  /* the point is to find a specific media type, but t was never used so add == t check. Probably it was a Diedrick bug, JHB Mar2021 */
          *m = static_cast<Media*>(nodes[i]);
          if (node) *node = i;

  //printf(" media find before return \n");

          return true;
        }
      }
    }

    return false;
  }

/* find one or more Attributes objects */

  int Node::find(AttrType t, std::vector<Attribute*>& result, int* node) {  /* change return type to int so user easily knows how many rtpmaps were found. 0 = none, JHB Jan2021 */

    Attribute* attr;

 #if 0
 printf(" ++ inside %s node:find, *node = %d, size = %d \n", t == SDP_ATTR_RTPMAP ? "rtpmaps" : "fmtps", node ? *node : 0, (int)nodes.size());
 #endif
 
    for (size_t i = node ? *node : 0; i < nodes.size(); ++i) {

  //printf(" node type[%lu] = %d \n", i, nodes[i]->type);

      if (nodes[i]->type == SDP_ATTRIBUTE) {
        attr = static_cast<Attribute*>(nodes[i]);
        if (attr->attr_type == t) {
          result.push_back(attr);
        }
        if (node) *node = i;
      }
    }

    return result.size();
  }

/* find one or more Origin objects */

  int Node::find(Type t, std::vector<Origin*>& result, int* node) {  /* added JHB Jan 2023 */

 #if 0
 fprintf(stderr, " ++ inside node:find origin, *node = %d, size = %d, type = %d \n", node ? *node : 0, (int)nodes.size(), (int)t);
 #endif

   Origin* orig;

    for (size_t i = node ? *node : 0; i < nodes.size(); ++i) {

  //printf(" node type[%lu] = %d \n", i, nodes[i]->type);

      if (nodes[i]->type == t) {
        orig = static_cast<Origin*>(nodes[i]);
        result.push_back(orig);
      }
    }

    return result.size();
  }


/* initializations -- note that items in comments are examples of expected sdp format text, JHB Jan2021 */

  /* v=0 */
  Version::Version():
    Node(SDP_VERSION),
    version(0) { }

  /* o=- 621762799816690644 7 IN IP4 127.0.0.1 */
  Origin::Origin():
    Node(SDP_ORIGIN),

  /* sess_version and username are currently commented to avoid initialization order compiler warnings. I can't figure it out yet, but shouldn't matter because SDPAdd() doesn't create an Origin object and Reader::parse() creates a local vector of Origin objects used to initialize thread_info[], JHB Jan 2023 */

//    sess_id(""),
//    username(""),
    sess_version(1),
    net_type(SDP_IN),
    addr_type(SDP_IP4),
    unicast_address("")
  { }

  /* m= */
  Media::Media():
    Node(SDP_MEDIA),
    media_type(SDP_MEDIATYPE_NONE),
    port(0),
    proto(SDP_MEDIAPROTO_NONE),
    fmt(0)
  { }

  /* s=- */
  SessionName::SessionName():
    Node(SDP_SESSION_NAME)
  { }

  /* i= */
  SessionInformation::SessionInformation():
    Node(SDP_SESSION_INFORMATION)
  { }

  /* u= */
  URI::URI(): 
    Node(SDP_URI)
  { }

  /* e= */
  EmailAddress::EmailAddress():
    Node(SDP_EMAIL_ADDRESS)
  { }

  /* p= */
  PhoneNumber::PhoneNumber():
    Node(SDP_PHONE_NUMBER)
  { }

  /* c= */
  ConnectionData::ConnectionData():
    Node(SDP_CONNECTION_DATA),
    net_type(SDP_IN),
    addr_type(SDP_IP4)
  { }

  /* t=0 0 */
  Timing::Timing():
    Node(SDP_TIMING),
    start_time(0),
    stop_time(0)
  { }

  /* a= */
  Attribute::Attribute():
    Node(SDP_ATTRIBUTE),
    attr_type(SDP_ATTRTYPE_NONE)
  { }

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

  /* a=fmtp: */
  AttributeFMTP::AttributeFMTP():
    Attribute()
  {
    attr_type = SDP_ATTR_FMTP;
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

  /* b=type:N */
  Bandwidth::Bandwidth():
    Node(SDP_BANDWIDTH),
    total_bandwidth_type("CT"),  /* default */
    bandwidth(0)
  { }
};
