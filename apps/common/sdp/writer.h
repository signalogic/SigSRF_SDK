/*
 SDP parsing and management

 Copyright (c) 2014 Diedrick H, as part of his "SDP" Github repository at https://github.com/diederickh/SDP
 License -- none given. Internet archive page as of 10Jan21 https://web.archive.org/web/20200918222637/https://github.com/diederickh/SDP

 Copyright (c) 2021 Signalogic, Dallas, Texas
 
 Revision History
 
  Modified Mar 2021 JHB, add bandwidth field support
*/

#ifndef SDP_WRITER_H
#define SDP_WRITER_H

#include <string>
#include <vector>
#include <sstream>
#include <sdp/sdp.h>
#include <sdp/types.h>
#include <sdp/utils.h>

namespace sdp {

  class Writer {
  public:
    std::string toString(SDP* sdp);
    std::string toString(Node* sdp);
    std::string toString(Version* v);
    std::string toString(Origin* o);
    std::string toString(SessionName* s);
    std::string toString(Timing* t);
    std::string toString(Media* m);
    std::string toString(Attribute* a);
    std::string toString(AttributeCandidate* c);
    std::string toString(Bandwidth* b);
  };  

} /* namesapce sdp */
#endif
