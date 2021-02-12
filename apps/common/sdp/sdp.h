/*
  SDP parsing and management

  Copyright (c) 2014 Diedrick H, as part of his "SDP" Github repository at https://github.com/diederickh/SDP
  License -- none given. Internet archive page as of 10Jan21 https://web.archive.org/web/20200918222637/https://github.com/diederickh/SDP
*/

#ifndef SDP_H
#define SDP_H

#include <string>
#include <vector>
#include <sdp/types.h>

#define HAVE_SDP

namespace sdp { 

  class SDP : public Node { 

     public:
     SDP();
  };

} /* namespace sdp */

#include <sdp/reader.h>
#include <sdp/writer.h>

#endif
