/*
 SDP parsing and management

 Copyright (c) 2014 Diedrick H, as part of his "SDP" Github repository at https://github.com/diederickh/SDP
 License -- none given. Internet archive page as of 10Jan21 https://web.archive.org/web/20200918222637/https://github.com/diederickh/SDP

 Copyright (c) 2021-2024 Signalogic, Dallas, Texas

 Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Revision History
  Modified Jan 2021 JHB, add a=rtpmap attribute support
  Modified Mar 2021 JHB, more codec types
  Modified Jan 2023 JHB, additional codec types
  Modified Jun 2024 JHB, add H.263 and H.265 codec types
*/

#include <sdp/utils.h>

namespace sdp {

  bool string_to_net_type(std::string& input, NetType& result) {

    result = SDP_NETTYPE_NONE;

    if (input.size() == 0) {
      return false;
    }

    if (input == "IN") {
      result = SDP_IN;
      return true;
    }

    return false;
  }

  /* convert a string to an AddrType */
  bool string_to_addr_type(std::string& input, AddrType& result) {

    result = SDP_ADDRTYPE_NONE;

    if (input.size() == 0) {
      return false;
    }

    if (input == "IP4") {
      result = SDP_IP4;
    }
    else if (input == "IP6") {
      result = SDP_IP6;
    }

    return result != SDP_ADDRTYPE_NONE;
  }

  /* convert a string to a CodecType */
  bool string_to_codec_type(std::string& input, CodecType& result) {

    result = SDP_CODECTYPE_NONE;

    if (input.size() == 0) {
      return false;
    }

    #if 0  // debug, JHB Jan 2023
    printf("\n *** codec str %s \n", input.c_str());
    #endif

    if (input == "PCMU") {
      result = SDP_G711U;
    }
    else if (input == "PCMA") {
      result = SDP_G711A;
    }
    else if (input == "G722") {
      result = SDP_G722;
    }
    else if (input == "G7221") {
      result = SDP_G7221;
    }
    else if (input == "G726-16") {
      result = SDP_G726_16;
    }
    else if (input == "G726-24") {
      result = SDP_G726_24;
    }
    else if (input == "G726-32") {
      result = SDP_G726_32;
    }
    else if (input == "G726-40") {
      result = SDP_G726_40;
    }
    else if (input == "G729") {
      result = SDP_G729;
    }
    else if (input == "AMR" || input == "AMR-NB") {
      result = SDP_AMRNB;
    }
    else if (input == "AMR-WB") {
      result = SDP_AMRWB;
    }
    else if (input == "EVS") {
      result = SDP_EVS;
    }
    else if (input == "CN") {
      result = SDP_CN;  // comfort noise
    }
    else if (input == "H263") {
      result = SDP_H263;
    }
    else if (input == "H264") {
      result = SDP_H264;
    }
    else if (input == "H265") {
      result = SDP_H265;
    }
    else if (input == "iLBC") {
      result = SDP_iLBC;
    }
    else if (input == "Speex") {
      result = SDP_Speex;
    }
    else if (input == "gsm") {
      result = SDP_gsm;
    }
    else if (input == "SILK") {
      result = SDP_SILK;
    }
    else if (input == "telephone-event") {
      result = SDP_TELEPHONE_EVENT;
    }
    else if (input == "tone") {
      result = SDP_TONE;
    }

    return result != SDP_CODECTYPE_NONE;
  }

  /* convert a string to an MediaType */
  bool string_to_media_type(std::string& input, MediaType& result) {

    result = SDP_MEDIATYPE_NONE;

    if (input.size() == 0) {
      return false;
    }

    if (input == "video") {
      result = SDP_VIDEO;
    }
    else if (input == "audio") {
      result = SDP_AUDIO;
    }
    else if (input == "text") {
      result = SDP_TEXT;
    }
    else if (input == "message") {
      result = SDP_MESSAGE;
    }
    else if (input == "application") {
      result = SDP_APPLICATION;
    }

    return result != SDP_MEDIATYPE_NONE;
  }

  /* convert a string to an MediaProto */
  bool string_to_media_proto(std::string& input, MediaProto& result) {

    result = SDP_MEDIAPROTO_NONE;

    if (input.size() == 0) {
      return false;
    }

    if (input == "udp") {
      result = SDP_UDP;
    }
    else if (input == "RTP/AVP") {
      result = SDP_RTP_AVP;
    }
    else if (input == "RTP/SAVP") {
      result = SDP_RTP_SAVP;
    }
    else if (input == "RTP/SAVPF") {
      result = SDP_RTP_SAVPF;
    }

    return result != SDP_MEDIAPROTO_NONE;
  }

  /* convert a string to a candidate type */
  bool string_to_cand_type(std::string& input, CandType& result) {

    result = SDP_CANDTYPE_NONE;

    if (input.size() == 0) {
      return false;
    }

    if (input == "host") {
      result = SDP_HOST;
    }
    else if (input == "host") {
      result = SDP_HOST;
    }
    else if(input == "srflx") {
      result = SDP_SRFLX;
    }
    else if(input == "prflx") {
      result = SDP_PRFLX;
    }
    else if(input == "relay") {
      result = SDP_RELAY;
    }

    return result != SDP_CANDTYPE_NONE;
  }

  std::string net_type_to_string(NetType type) {
    switch (type) {
      case SDP_IN: { return "IN"; } 
      default: { return "unknown"; }
    };
  }

  std::string addr_type_to_string(AddrType type) {
    switch (type) {
      case SDP_IP4: { return "IP4"; } 
      case SDP_IP6: { return "IP6"; } 
      default: { return "unknown"; }
    };
  }

  std::string codec_type_to_string(CodecType type) {
    switch (type) {
      case SDP_G711U: { return "PCMU"; }
      case SDP_G711A: { return "PCMA"; }
      case SDP_G722: { return "G722"; }
      case SDP_G7221: { return "G722"; }
      case SDP_G726_16: { return "G726-16"; }
      case SDP_G726_24: { return "G726-24"; }
      case SDP_G726_32: { return "G726-32"; }
      case SDP_G726_40: { return "G726-40"; }
      case SDP_G729: { return "G729"; }
      case SDP_AMRNB: { return "AMR"; }
      case SDP_AMRWB: { return "AMR-WB"; }
      case SDP_iLBC: { return "iLBC"; }
      case SDP_Speex: { return "Speex"; }
      case SDP_gsm: { return "gsm"; }
      case SDP_SILK: { return "SILK"; }
      case SDP_CN: { return "Comfort Noise"; }
      case SDP_H263: { return "H.263"; }
      case SDP_H264: { return "H.264"; }
      case SDP_H265: { return "H.265"; }
      case SDP_TELEPHONE_EVENT: { return "telephone-event"; }
      default: { return "unknown"; }
    };
  }

  std::string media_type_to_string(MediaType type) {
    switch (type) {
      case SDP_VIDEO:        { return "video";          } 
      case SDP_AUDIO:        { return "audio";          } 
      case SDP_TEXT:         { return "text";           } 
      case SDP_APPLICATION:  { return "application";    } 
      case SDP_MESSAGE:      { return "message";        } 
      default:               { return "unknown";        } 
    }
  }

  std::string media_proto_to_string(MediaProto proto) {
    switch (proto) {
      case SDP_UDP:        { return "udp";       } 
      case SDP_RTP_AVP:    { return "RTP/AVP";   } 
      case SDP_RTP_SAVP:   { return "RTP/SAVP";  } 
      case SDP_RTP_SAVPF:  { return "RTP/SAVPF"; } 
      default:             { return "unknown";   } 
    };
  }

  std::string cand_type_to_string(CandType type) {
    switch (type) {
      case SDP_HOST:      { return "host";      } 
      case SDP_SRFLX:     { return "srflx";     } 
      case SDP_PRFLX:     { return "prflx";     } 
      case SDP_RELAY:     { return "relay";     }
      default:            { return "unknown";   } 
    }
  }

} /* namespace sdp */
