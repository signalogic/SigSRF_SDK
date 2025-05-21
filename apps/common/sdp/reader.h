/*
 SDP parsing and management

 Copyright (c) 2014 Diedrick H, as part of his "SDP" Github repository at https://github.com/diederickh/SDP
 License -- none given. Internet archive page as of 10Jan21 https://web.archive.org/web/20200918222637/https://github.com/diederickh/SDP

 Copyright (c) 2021-2025 Signalogic, Dallas, Texas

 Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Revision History
  Modified Jan 2021 JHB, add Line::readCodecType() for a=rtpmap attribute support
  Modified Mar 2021 JHB, add error reporting option to Line.readInt()
  Modified Mar 2021 JHB, add bandwidth field support (b= )
  Modified Apr 2023 JHB, add fReportError param to readNetType(), readAddrType(), and readString(). See comments in reader.cpp
  Modified May 2025 JHB, add fAllowNonNumeric param to Line:readInt()
*/

#ifndef SDP_READER_H
#define SDP_READER_H

#include <string>
#include <sstream>
#include <exception>
#include <sdp/sdp.h>
#include <sdp/types.h>

#define SDP_READER_PARSE_FORMAT_ONLY 1

namespace sdp { 

  /* parse exception */
  struct ParseException : public std::exception {
    std::string s;
    ParseException(std::string s):s(s) {}
    ~ParseException() throw() {}
    const char* what() const throw() { return s.c_str(); };
  };

  /* one element of a Line */
  class Token {
  public:
    Token();
    Token(std::string value);
    bool isNumeric();
    size_t size();

    /* conversion functions */
    int         toInt();
    uint64_t    toU64();
    std::string toString();
    AddrType    toAddrType();
    NetType     toNetType();
    MediaType   toMediaType();
    MediaProto  toMediaProto();
    CandType    toCandType();
    CodecType   toCodecType();

  public:
    std::string value;
  };

  /* a sdp line, e.g "a=rtcp:59976 IN IP4 192.168.0.194" */
  class Line {
  public:
    Line();
    Line(std::string src);

    /* generic parse functions */
    void skip(char until);                                  /* skip some characters until given character is found */
    void ltrim();                                           /* trim whitespace from left from current index */
    Token getToken(char until = ' ');                       /* read part of a sdp line until the given character */
    char operator[](unsigned int);

    /* read the next token as a specific type */
    bool        readType(char type);                        /* read until the type element (e.g. o=, v=, a=) and return true when the line is the given type */
    std::string readString(char until = ' ', bool fReportError = true);               /* read a string from the next token */
    int         readInt(char until = ' ', bool fAllowNonNumeric = false, bool fReportError = true);  /* read an integer value from the next token */
    uint64_t    readU64(char until = ' ');                  /* read an integer (u64) */
    AddrType    readAddrType(char until = ' ', bool fReportError = true);  /* read an AddrType */
    NetType     readNetType(char until = ' ', bool fReportError = true);  /* read a NetType */
    MediaType   readMediaType(char until = ' ');            /* read a MediaType */
    MediaProto  readMediaProto(char until = ' ');           /* read a MediaProto */ 
    CandType    readCandType(char until = ' ');             /* read a CandType */ 
    CodecType   readCodecType(char until = '/');            /* read a CodecType */

  public:
    std::string value;
    size_t index;                                           /* used to keep track until which character has been read */
  };

  /* parses an SDP */
  class Reader {
  public:
    int parse(std::string source, SDP* result, unsigned int uFlags);

  private:
    Node*               parseLine(Line& line);    
    Version*            parseVersion(Line& line);                /* v= */
    Origin*             parseOrigin(Line& line);                 /* o= */
    SessionName*        parseSessionName(Line& line);            /* s= */
    SessionInformation* parseSessionInformation(Line& line);     /* i= */
    URI*                parseURI(Line& line);                    /* u= */
    EmailAddress*       parseEmailAddress(Line& line);           /* e= */
    PhoneNumber*        parsePhoneNumber(Line& line);            /* p= */
    ConnectionData*     parseConnectionData(Line& line);         /* c= */
    Timing*             parseTiming(Line& line);                 /* t= */
    Media*              parseMedia(Line& line);                  /* m= */
    Attribute*          parseAttribute(Line& line);              /* a= */
    Bandwidth*          parseBandwidth(Line& line);              /* b= */
  };

}

#endif
