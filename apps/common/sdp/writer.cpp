/*
 SDP parsing and management

 Copyright (c) 2014 Diedrick H, as part of his "SDP" Github repository at https://github.com/diederickh/SDP
 License -- none given. Internet archive page as of 10Jan21 https://web.archive.org/web/20200918222637/https://github.com/diederickh/SDP

 Copyright (c) 2021 Signalogic, Dallas, Texas
 
 Revision History
 
  Modified Mar 2021 JHB, add bandwidth field support
*/

#include <sdp/writer.h>

namespace sdp {

  std::string Writer::toString(SDP* sdp) {

    std::string result;

    for (size_t i = 0; i < sdp->nodes.size(); ++i) {    
      result += toString(sdp->nodes[i]);
    }

    return result;
  }

  std::string Writer::toString(Node* node) {

    std::string result;

    switch (node->type) {
      case SDP_VERSION:      { result += toString(static_cast<Version*>(node));        break; }
      case SDP_ORIGIN:       { result += toString(static_cast<Origin*>(node));         break; } 
      case SDP_SESSION_NAME: { result += toString(static_cast<SessionName*>(node));    break; } 
      case SDP_TIMING:       { result += toString(static_cast<Timing*>(node));         break; } 
      case SDP_MEDIA:        { result += toString(static_cast<Media*>(node));          break; }
      case SDP_ATTRIBUTE:    { result += toString(static_cast<Attribute*>(node));      break; }
      case SDP_BANDWIDTH:    { result += toString(static_cast<Bandwidth*>(node));      break; }
      default: {
        printf("Error: cannot convert node type to string: %d \n", node->type);
        break;
      }
    }

    return result;
  }

  /* v=0 */
  std::string Writer::toString(Version* v) {
    std::stringstream ss;
    ss << "v=" << v->version << "\r\n";
    return ss.str();
  }

  /* o=- 621762799816690644 7 IN IP4 127.0.0.1 */
  std::string Writer::toString(Origin* o) {
    std::stringstream ss;

    ss << "o=" 
       << o->username << " " 
       << o->sess_id << " " 
       << o->sess_version << " "
       << sdp::net_type_to_string(o->net_type) << " "
       << sdp::addr_type_to_string(o->addr_type) << " " 
       << o->unicast_address.c_str() 
       << "\r\n";

    return ss.str();
  }

  /* s= */
  std::string Writer::toString(SessionName* s) {
    std::stringstream ss;
    ss << "s=" << s->session_name << "\r\n";
    return ss.str();
  }

  /* t=  */
  std::string Writer::toString(Timing* t) {
    std::stringstream ss;

    ss << "t="
       << t->start_time << " "
       << t->stop_time
       << "\r\n";

    return ss.str();
  }

  /* m= */
  std::string Writer::toString(Media* m) {

    std::stringstream ss;

    ss << "m="
       << sdp::media_type_to_string(m->media_type) << " "
       << m->port << " "
       << sdp::media_proto_to_string(m->proto) << " "
       << m->fmt
       << "\r\n";

    /* @todo -> add RTPMAP attribute ids. */

    /* add the attributes of a media element. */
    for (size_t i = 0; i < m->nodes.size(); ++i) {
      ss << toString(m->nodes[i]); 
    }

    return ss.str();
  }

  /* a= */
  std::string Writer::toString(Attribute* a) {
    std::stringstream ss;
    //printf(">> ATTR: %s\n", a->value.c_str());
    switch (a->attr_type) {

      /* generic name-value attributes */
      case SDP_ATTR_ICE_UFRAG:
      case SDP_ATTR_ICE_PWD:
      case SDP_ATTR_UNKNOWN: {
        ss << "a=" << a->name ;

        if (a->value.size()) {
         ss << ":" << a->value << "\r\n";
        }

        return ss.str();
      }

      case SDP_ATTR_CANDIDATE: {
        return toString(static_cast<AttributeCandidate*>(a));
      }

      /* unknown/unhandled */
      default: {
        printf("Error: cannot convert attribute type to a string: %d\n", a->attr_type);
        return "";
      }
    }
  }

  /* c= */
  std::string Writer::toString(AttributeCandidate* c) {
    std::stringstream ss;
    
    ss << "a=candidate:" 
       << c->foundation << " "
       << c->component_id << " "
       << c->transport << " "
       << c->priority << " " 
       << c->connection_address << " "
       << c->port << " " 
       << "typ "
       << sdp::cand_type_to_string(c->cand_type)
       << "\r\n";

    return ss.str();
  }

  /* b=  */
  std::string Writer::toString(Bandwidth* b) {
    std::stringstream ss;

    ss << "b="
       << b->total_bandwidth_type << ":" << b->bandwidth << "\r\n";

    return ss.str();
  }

} /* namespace sdp */
