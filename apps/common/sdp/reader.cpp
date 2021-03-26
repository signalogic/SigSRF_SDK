/*
  SDP parsing and management

  Copyright (c) 2014 Diedrick H, as part of his "SDP" Github repository at https://github.com/diederickh/SDP
  License -- none given. Internet archive page as of 10Jan21 https://web.archive.org/web/20200918222637/https://github.com/diederickh/SDP

  Copyright (c) 2021 Signalogic, Dallas, Texas

  Revision History
    Modified Jan 2021 JHB, add a=rtpmap attribute support
    Modified Feb 2021 JHB, add support for # comment delineator in SDP file lines. See parseLine() below and example.sdp for examples and more notes
    Modified Mar 2021 JHB, fix problem with possible trailing '/' after rtpmap clock rate, add reading of optional number of channels
*/

#include <sdp/reader.h>
#include <sdp/utils.h>
#include <cstring>

namespace sdp { 

   Token::Token() {
   }

   Token::Token(std::string value)
      :value(value)
   {
   }

   size_t Token::size() {
      return value.size();
   }

   bool Token::isNumeric() {
      return sdp::is_numeric(value);
   }

   int Token::toInt() {
      return sdp::convert<int>(value);
   }

   uint64_t Token::toU64() {
      return sdp::convert<uint64_t>(value);
   }

   std::string Token::toString() {
      return value;
   }

   NetType Token::toNetType() {
      NetType result = SDP_NETTYPE_NONE;
      sdp::string_to_net_type(value, result);
      return result;
   }

   AddrType Token::toAddrType() {
      AddrType result;
      sdp::string_to_addr_type(value, result);
      return result;
   }

   CodecType Token::toCodecType() {
      CodecType result;
      sdp::string_to_codec_type(value, result);
      return result;
   }

   MediaType Token::toMediaType() {
      MediaType result;
      sdp::string_to_media_type(value, result);
      return result;
   }

   MediaProto Token::toMediaProto() {
      MediaProto result;
      sdp::string_to_media_proto(value, result);
      return result;
   }

   CandType Token::toCandType() {
      CandType result;
      sdp::string_to_cand_type(value, result);
      return result;
   }

   Line::Line() {
   }

   Line::Line(std::string src)
     :value(src)
     ,index(0)
   {
   }

   void Line::skip(char until) {
      for(;index < value.size(); ++index) {
         if (value[index] == until) {
            index++;
            break;
         }
      }
   }

   void Line::ltrim() {
      for(size_t i = index; i < value.size(); ++i) {
         if (value[i] == ' ') {
            index++;
         }
         else {
            break;
         }
      }
   }

   std::string Line::readString(char until) {
      Token t = getToken(until);
      if (t.size() == 0) {
         throw ParseException("Invalid Token. Token is empty.");
         return "";
      }
      return t.toString();
   }

   int Line::readInt(char until, bool fReportError) {  /* add error reporting option, JHB Mar2021 */

      Token t = getToken(until);

      if (t.size() == 0 && fReportError) {
         throw ParseException("Int token is empty");
         return 0;
      }

      if (t.size() && !t.isNumeric()) {
         throw ParseException("Int token is not numeric");
         return 0;
      }

      if (t.size()) return t.toInt();
      else return 0;  /* no int found, JHB Mar2021 */
   }

   uint64_t Line::readU64(char until) {

      Token t = getToken(until);

      if (t.size() == 0) {
         throw ParseException("Token is empty");
         return 0;
      }

      if (!t.isNumeric()) {
         throw ParseException("U64 token is not numeric");
         return 0;
      }

      return t.toU64();
   }

  /* SDP_IP4 or SDP_IP6 */
   AddrType Line::readAddrType(char until) {

      Token t = getToken(until);
      if (t.size() == 0) {
         throw ParseException("IP address token is empty");
         return SDP_ADDRTYPE_NONE;
      }
    
      AddrType result = t.toAddrType();
      if (result == SDP_ADDRTYPE_NONE) {
         throw ParseException("Invalid IP address type");
      }

      return result;
   }

   NetType Line::readNetType(char until) {

      Token t = getToken(until);
      if (t.size() == 0) {
         throw ParseException("Net type token is empty");
         return SDP_NETTYPE_NONE;
      }

      NetType result = t.toNetType();
      if (result == SDP_NETTYPE_NONE) {
         throw ParseException("Invalid net type");
      }

      return result;
   }

   CodecType Line::readCodecType(char until) {

      Token t = getToken(until);
      if (t.size() == 0) {
         throw ParseException("Codec token is empty");
         return SDP_CODECTYPE_NONE;
      }

      CodecType result = t.toCodecType();
      if (result == SDP_CODECTYPE_NONE) {
         throw ParseException("Invalid codec type");
      }

      return result;
   }

   MediaProto Line::readMediaProto(char until) {

      Token t = getToken(until);
      if (t.size() == 0) {
         throw ParseException("Media proto token is empty");
         return SDP_MEDIAPROTO_NONE;
      }

      MediaProto result = t.toMediaProto();
      if (result == SDP_MEDIAPROTO_NONE) {
         throw ParseException("Invalid media proto");
      }

      return result;
   }

   MediaType Line::readMediaType(char until) {

      Token t = getToken(until);
      if (t.size() == 0) {
         throw ParseException("Media type token is empty");
         return SDP_MEDIATYPE_NONE;
      }

      MediaType result = t.toMediaType();
      if (result == SDP_MEDIATYPE_NONE) {
         throw ParseException("Invalid media type");
      }

      return result;
   }

   CandType Line::readCandType(char until) {

      Token t = getToken(until);
      if (t.size() == 0) {
         throw ParseException("Candidate type token is empty");
         return SDP_CANDTYPE_NONE;
      }

      CandType result = t.toCandType();
      if (result == SDP_CANDTYPE_NONE) {
         throw ParseException("Invalid candidate type: " +t.value);
      }

      return result;
   }

   Token Line::getToken(char until) {

      std::string result;

      for (size_t i = index; i < value.size(); ++i) {
         index++;
         if (value[i] == until) {
            break;
         }
         result.push_back(value[i]);
      }

      return Token(result);
   }

   bool Line::readType(char type) {
      if (value[0] == type) {
         skip('=');
         return true;
      }
      return false;
   }

   char Line::operator[](unsigned int dx) {
      return value.at(dx);
   }

   int Reader::parse(std::string source, SDP* result) {

      if (!source.size()) {
         return -1;  
      }

      std::vector<std::string> lines;    
      if (tokenize(source, '\n', lines) < 0) {
         return -2;
      }

      Node* parent = result; 

      for (size_t i = 0; i < lines.size(); ++i) {

         Line line(lines[i]);
         Node* node = parseLine(line);

         if (!node) {
            //printf("Error: cannot parse line: %ld\n", i);
            continue;  /* this just skips empty lines, which are not a problem. Diedrick shoulda said so when he commented out the printf(), JHB Feb2021 */
         }

         if (node->type == SDP_MEDIA) {
            result->addNode(node);
            parent = node;
         }
         else {
            parent->addNode(node);
         }
      }

      #ifdef DEBUGPRINT
      printf("%zu lines\n", lines.size());
      #endif

      return 0;
   }

   Node* Reader::parseLine(Line& l) {

   /* ignore comment section of line, JHB Feb2021 */

      char* p = strchr((char*)l.value.c_str(), '#');  /* look for comment symbol */
      if (p) *p = 0;

// printf(" parsing line %s \n", tmpstr);

      if (strlen((char*)l.value.c_str())) switch (l[0]) {  /* don't process empty line, for example if whole line was a comment */

         case 'v': { return parseVersion(l);             }
         case 'o': { return parseOrigin(l);              }
         case 's': { return parseSessionName(l);         }
         case 'i': { return parseSessionInformation(l);  }
         case 'u': { return parseURI(l);                 }
         case 'e': { return parseEmailAddress(l);        }
         case 'p': { return parsePhoneNumber(l);         }
         case 'c': { return parseConnectionData(l);      }
         case 't': { return parseTiming(l);              }
         case 'm': { return parseMedia(l);               }
         case 'a': { return parseAttribute(l);           }

         /* unhandled line */
         default: {
            fprintf(stderr, "sdp: ERROR: unhandled line: %s\n", l.value.c_str());
            return NULL;
         }
      }

      return NULL;
   }

   /* v= */
   Version* Reader::parseVersion(Line& line) {

      if (!line.readType('v')) {   
         return NULL;  
      }

      Version* node = new Version();

      try {
         node->version = line.readInt();
      }
      catch(std::exception& e) {
         printf("Error: %s\n", e.what());
         delete node;
         node = NULL;
      }

      return node;
   };

   /* o= */
   Origin* Reader::parseOrigin(Line& line) {

      if (!line.readType('o')) {
         return NULL;
      }

      Origin* node = new Origin();
    
      try {
         node->username         = line.readString();     /* e.g. "roxlu", "-" */
         node->sess_id          = line.readString();     /* e.g. "621762799816690644" */
         node->sess_version     = line.readU64();        /* e.g. 1 */
         node->net_type         = line.readNetType();    /* e.g. SDP_IN */
         node->addr_type        = line.readAddrType();   /* e.g. SDP_IP4 */
         node->unicast_address  = line.readString();     /* e.g. "127.0.0.1" */ 
      }
      catch(std::exception& e) {
         printf("Error: %s\n", e.what());
         delete node;
         node = NULL;
      }

      return node;
   }

   /* s= */
   SessionName* Reader::parseSessionName(Line& line) {

      if (!line.readType('s')) {
         return NULL;
      }

      SessionName* node = new SessionName();
    
      try {
         node->session_name = line.readString();
      }
      catch(std::exception& e) {
         printf("Error: %s\n", e.what());
         delete node;
         node = NULL;
      }

      return node;
   }

   /* i= */
   SessionInformation* Reader::parseSessionInformation(Line& line) {

      if (!line.readType('i')) {
         return NULL;
      }

      SessionInformation* node = new SessionInformation();

      try {
         node->session_description = line.readString();
      }
      catch(std::exception& e) {
         printf("Error: %s\n", e.what());
         delete node;
         node = NULL;
      }

      return node;
   }

   /* u= */
   URI* Reader::parseURI(Line& line) {

      if (!line.readType('u')) {
         return NULL;
      }

      URI* node = new URI();

      try {
         node->uri = line.readString();
      }
      catch(std::exception& e) {
         printf("Error: %s\n", e.what());
         delete node;
         node = NULL;
      }

      return node;
   }

   /* e= */
   EmailAddress* Reader::parseEmailAddress(Line& line) {

      if (!line.readType('e')) {
         return NULL;
      }

      EmailAddress* node = new EmailAddress();

      try {
         node->email_address = line.readString();
      }
      catch(std::exception& e) {
         printf("Error: %s\n", e.what());
         delete node;
         node = NULL;
      }

      return node;
   }

   /* p= */
   PhoneNumber* Reader::parsePhoneNumber(Line& line) {

      if (!line.readType('e')) {
         return NULL;
      }

      PhoneNumber* node = new PhoneNumber();

      try {
         node->phone_number = line.readString();
      }
      catch(std::exception& e) {
         printf("Error: %s\n", e.what());
         delete node;
         node = NULL;
      }

      return node;
   }

   ConnectionData* Reader::parseConnectionData(Line& line) {

      if (!line.readType('t')) {
         return NULL;
      }

      ConnectionData* node = new ConnectionData();

      try {
         node->net_type            = line.readNetType();
         node->addr_type           = line.readAddrType();
         node->connection_address  = line.readString();
      }
      catch(std::exception& e) {
         printf("Error: %s\n", e.what());
         delete node;
         node = NULL;
      }

      return node;
   }

   /* t= */
   Timing* Reader::parseTiming(Line& line) {

      if (!line.readType('t')) {
         return NULL;
      }

      Timing* node = new Timing();

      try {
         node->start_time = line.readU64();
         node->stop_time  = line.readU64();
      }
      catch(std::exception& e) {
         printf("Error: %s\n", e.what());
         delete node;
         node = NULL;
      }

      return node;
   }

   /* m= */
   Media* Reader::parseMedia(Line& line) {

      if (!line.readType('m')) {
         return NULL;
      }

      Media* node = new Media();

      try {
         node->media = line.readMediaType();
         node->port = line.readInt();
         node->proto = line.readMediaProto();
         node->fmt = line.readInt();
      }
      catch(std::exception& e) {
         printf("Error: %s\n", e.what());
         delete node;
         node = NULL;
      }

      return node;
   }

   /* a= */
   Attribute* Reader::parseAttribute(Line& line) {

      if (!line.readType('a')) {
         return NULL;
      }

      Attribute* node = NULL;
      std::string name;

      try {

         name = line.readString(':');
         line.ltrim();

         if (name == "rtcp") {
            AttributeRTCP* attr = new AttributeRTCP();
            node = (Attribute*) attr;
            attr->port = line.readInt();
            attr->net_type = line.readNetType();
            attr->addr_type = line.readAddrType();
            attr->connection_address = line.readString();
         }
         else if (name == "candidate") {
            AttributeCandidate* attr = new AttributeCandidate();
            node = (Attribute*) attr;
            attr->foundation = line.readString();
            attr->component_id = line.readInt();
            attr->transport = line.readString();
            attr->priority = line.readU64();
            attr->connection_address = line.readString();
            attr->port = line.readInt();
            line.skip(' '); /* "typ" */
            attr->cand_type = line.readCandType();
            /* @todo read the other elements! */
         }
         else if (name == "ice-ufrag") {
            node = new Attribute();
            node->name = name;
            node->value = line.readString();
            node->attr_type = SDP_ATTR_ICE_UFRAG;
         }
         else if (name == "ice-pwd") {
            node = new Attribute();
            node->name = name;
            node->value = line.readString();
            node->attr_type = SDP_ATTR_ICE_PWD;
         }

      /* add rtpmap - most important attribute, can't believe Diedrick left it out, JHB Jan2021 */

         else if (name == "rtpmap") {
            AttributeRTP* attr = new AttributeRTP();
            node = (Attribute*) attr;
            attr->pyld_type = line.readInt();
            attr->codec_type = line.readCodecType();
            attr->clock_rate = line.readInt('/');  /* note the possibility of a trailing /, which if present would be followed by a number-of-channels value, JHB Mar2021 */
            attr->num_chan = std::max(line.readInt('/', false), 1);  /* number of channels may or may not be there, JHB Mar2021 */
            node->attr_type = SDP_ATTR_RTPMAP;
         }

         else {
            node = new Attribute();
            node->name = name;
            node->attr_type = SDP_ATTR_UNKNOWN;

            if (line.value.size() > line.index) {
               node->value = line.value.substr(line.index); 
            }
         }
      }
      catch(std::exception& e) {
         printf("Error: %s\n", e.what());
         if (node) {
            delete node;
            node = NULL;
         }
      }

      return node;
   }

} /* namespace sdp */
