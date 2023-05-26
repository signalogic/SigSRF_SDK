/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/sdp_app.cpp

 Copyright (C) Signalogic Inc. 2021-2023

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

 Description

  SDP related parsing and object management, SIP Invite and other SIP message processing
  
 Purpose
 
  support for mediaMin reference application

 Documentation

  ftp://ftp.signalogic.com/documentation/SigSRF

  (as of Oct 2019, most recent doc is ftp://ftp.signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

 Revision History

   Created Apr 2021 JHB, split off from mediaMin.cpp
   Modified Jan 2023 JHB, change SDPAdd() to SDPParseInfo() and expand its functionality to handle Origin objects
   Modified Jan 2023 JHB, change FindSIPInvite() to ProcessSessionControl() add params and expand its functionality in 2 main areas:
                          -normalize SDP info handling independent of its source (SIP invite or SAP/SDP packets)
                          -add additional SIP message handling
   Modified Jan 2023 JHB, handle session IDs in Origin objects. We are seeing a lot of pcaps with repeated SIP Invites (for various reasons, like the receiver was slow so the sender repeats with PSH,ACK) so we need to add SDP info to an input stream's SDP database only when it has a unique session ID
   Modified Jan 2023 JHB, add rudimentary SIP message parsing and status to ProcessSessionControl()
   Modified Jan 2023 JHB, add support for SAP/SDP protocol payloads to ProcessSessionControl()
   Modified Mar 2023 JHB, implement SESSION_CONTROL_NO_PARSE uFlag, add more SIP message types, fix bug in searching for BYE message
   Modified Apr 2023 JHB, handle SIPREC format in SIP invite packets (partly based on RFC7245). Look for p_siprec
*/

#include <fstream>
using namespace std;

#include "pktlib.h"     /* DSGetPacketInfo() */
#include "voplib.h"
#include "diaglib.h"    /* bring in Log_RT() definition */

#include <sdp/sdp.h>    /* SDP info API header file */

#include "mediaTest.h"  /* bring in some constants needed by mediaMin.h */
#include "mediaMin.h"   /* bring in THREAD_INFO typedef for thread_info[].xx[] access, indexed by thread and input stream */
#include "sdp_app.h"

extern THREAD_INFO thread_info[];  /* THREAD_INFO struct defined in mediaMin.h. Indexed by thread_index */

/* SDPParseInfo() notes:

  -expects SDP info in sdpstr as plain text per RFC 8866, without any additional header or other packet content
  -adds SDP info to thread_info[] SDP data, for reference during dynamic session creation
  -SDP info can be from command line .sdp file or SIP invite packet text data. SDP info can contain multiple Media elements, multiple rtpmap attributes
  -SDP info can be added at any time, in any sequence (cmd line .sdp file, if one, is added first)
  -currently duplicate media elements and rtpmap attributes are not filtered out. When searching through an rtpmap vector, it's application dependent on whether first or latest matching rtpmap is used
*/

int SDPParseInfo(std::string sdpstr, unsigned int uFlags, int nInput, int thread_index) {

/* SDP related items */

   sdp::SDP sdp_session;
   sdp::Reader reader;
//   sdp::Writer writer;  /* not currently used */
   sdp::Media* media = NULL;
   int node = 0;

// #define PRINTSDPDEBUG  /* turn on for SDP parsing debug */

   if (!sdpstr.length()) return -1;  /* empty string is an error */

// printf(" *** before reader parse \n");

/* parse input SDP info string into an sdp::SDP session */

   reader.parse(sdpstr, &sdp_session, 0);

// printf(" *** after reader parse \n");

/* retrieve Media elements ("m=audio, m=video") */

   #ifdef PRINTSDPDEBUG
   sdp_session.print(NULL);
   #endif

/* Sequence and logic, JHB Jan 2023:

   -first search for an origin field. If any found through them and create Origin objects. If not found then do the loop at least once in order to find all Media objects. To-Do: at some future point we may associate each Origin object with a group of Media objects
 
   -inside the loop:

     -compare found originator description session IDs with thread_info[].origins[stream] originator descriptions. If already existing, don't add an Origin object, assuming the SIP sender has repeated a SIP Invite for whatever reason
     -if an Origin object does not already exist then add it
     -find all media descriptions and add Media objects to the stream's SDP database, including their RTP, RTCP, etc attributes. Currently this means that for multiple Origins, all Media objects get added after the first Origin (see To-Do note above)

   -second summarize results, including display/log messages (see nOriginsFound and nMediaObjectsFound)

   -return number of Origins added

   Notes:

   -Media and Origin nodes are parents and Attribute nodes are children of Media element nodes (see reader.cpp)
   -in find() functions in types.cpp, starting node is zero unless we pass in a start node. nodes.size() = 0 means no nodes were found/added (see types.cpp)
   -for Media nodes, advance by one in the while loop to find attributes associated with that media node. For Origin nodes search those already found/added looking for unique session Id
   -it's possible to loop/search through attributes objects also, but currently we're not doing that (unless the debug #define is turned on)
*/

   vector<sdp::Origin*> origins = {};  /* create Origin vector, init to zero */
   int nOriginsFound = 0, nOriginsAdded = 0, nMediaObjectsFound = 0, nMediaObjectsAdded = 0;

/* search for origin fields, if any found then we find out how many, gather info on them (e.g. session ID), and iterate through them */

// printf(" *** before session.find \n");

   int num_origins = sdp_session.find(sdp::SDP_ORIGIN, origins, NULL);  /* retrieve Origin nodes if any. Starting node is always zero */
   bool fParseOrigins = !(uFlags & SDP_PARSE_IGNORE_ORIGINS) && (num_origins > 0);  /* don't look for Origins if IGNORE flag is set */

// printf(" *** after session.find \n");

   char szSessionIDs[300] = "";

  //printf(" \n *** before origins loop, parse = %d, num origins = %d, o pos = %d \n", fParseOrigins, num_origins, (int)sdpstr.find("o="));

   for (int i=0; i<max(num_origins, 1); i++) {  /* we need to enter the loop at least once even. fParseOrigins is checked inside the loop */

      #ifdef PRINTSDPDEBUG
      printf(" + num rtpmaps %d \n", num_origins);
      #endif

   /* loop through found Origin nodes - should be only one per SDP info, but you never know */

      bool fNewOrigin = false;
      int j;

      if (fParseOrigins) {

         sdp::Origin* origin_found = (sdp::Origin*)origins[i];  /* make a scalar Origin object to work with */

         #ifdef PRINTSDPDEBUG
         printf("%s orgin[%d], session id = %s, username = %s, session version = %llu, \n", !i ? "\n" : "", i, origin_found->sess_id.c_str(), origin_found->username.c_str(), (long long unsigned int)origin_found->sess_version);
         #endif

         if (origin_found->sess_id == "0" || origin_found->sess_id == "") Log_RT(4, "mediaMin INFO: SDP info with invalid Origin session ID %s not used \n", origin_found->sess_id.c_str());
         else for (fNewOrigin = true, j=0; j<thread_info[thread_index].num_origins[nInput]; j++) {  /* search existing origins. Note we set fNewOrigin in case it's the first one */
 
            sdp::Origin* origin = (sdp::Origin*)thread_info[thread_index].origins[nInput][j];

            if (origin_found->sess_id == origin->sess_id) {  /* compare with existing origin */

               Log_RT(4, "mediaMin INFO: SDP info with already existing Origin session ID %s not used \n", origin_found->sess_id.c_str());
               fNewOrigin = false;  /* found a duplicate, break out of loop */
               break;
            }
         }

//  printf(" after search loop, fNewOrigin = %d \n", fNewOrigin);

         if (fNewOrigin) {

            nOriginsFound++;  /* increment number of unique origins found */
            sprintf(&szSessionIDs[strlen(szSessionIDs)], " %s", origins[i]->sess_id.c_str());

            if (uFlags & SDP_PARSE_ADD) {

            /* save found origin(s) in thread_info[]. Note we add i to ignore origins that were either invalid or already existing */

               vector<sdp::Origin*> th_origins = thread_info[thread_index].origins[nInput];  /* append additional origin info */

//  printf(" before insert, session id = %s \n", origin_found->sess_id.c_str());

               th_origins.insert( th_origins.end(), origins.begin() + i, origins.begin() + i + 1 );  /* C++ still doesn't have append so we "insert at the end" ( https://stackoverflow.com/questions/2551775/appending-a-vector-to-a-vector) */
               thread_info[thread_index].origins[nInput] = th_origins;

               thread_info[thread_index].num_origins[nInput]++;  /* increment number of origins in stream's thread_info[] */

               nOriginsAdded++;
            }
         }
      }

      if (!fParseOrigins || nOriginsFound) while (sdp_session.find(sdp::SDP_AUDIO, &media, &node)  /* find all audio Media elements. Note that node starts at zero (Node::find(MediaType t, Media** m, int* node) is in types.cpp) */
       #if 0  /* uncomment to parse video Media elements */
       || sdp_session.find(sdp::SDP_VIDEO, &media, &node)
       #endif
      ) {

         #ifdef PRINTSDPDEBUG
         printf(" + media found, media_node = %d \n", node);
         #endif

         node++;  /* increment node, in case there are more Media elements */

         vector<sdp::Attribute*> rtpmaps = {};  /* create an rtpmap vector, init to zero */

         if (int num_rtpmaps = media->find(sdp::SDP_ATTR_RTPMAP, rtpmaps, NULL)) {  /* (int Node::find(AttrType t, std::vector<Attribute*>& result, int* node) in types.cpp) */

//  printf(" adding %d num_rtpmaps \n", num_rtpmaps);

            nMediaObjectsFound++;  /* technically this should be rtpmaps found, but if we find a media object with no attributes then it's useless anyway */

            #ifdef PRINTSDPDEBUG
            printf(" + num rtpmaps %d \n", num_rtpmaps);
            #endif

            #ifdef PRINTSDPDEBUG
            for (int i=0; i<num_rtpmaps; i++) {  /* loop through rtpmap attributes - this could be used to skip existing identical attributes, etc */

               sdp::AttributeRTP* rtpmap = (sdp::AttributeRTP*)rtpmaps[i];  /* map RTP attribute onto generic attribute in order to do something useful with it ... */

               printf("%s rtpmap[%d], pyld type = %d, codec type = %d, sample rate = %d, num chan = %d \n", !i ? "\n" : "", i, rtpmap->pyld_type, rtpmap->codec_type, rtpmap->clock_rate, rtpmap->num_chan);
            }
            #endif
 
            if (uFlags & SDP_PARSE_ADD) {

            /* save found rtmpap(s) in thread_info[] for reference in create_dynamic_session() in mediaMin.cpp */

               vector<sdp::Attribute*> th_rtpmaps = thread_info[thread_index].rtpmaps[nInput];  /* append additional SDP info */
               th_rtpmaps.insert( th_rtpmaps.end(), rtpmaps.begin(), rtpmaps.end() );
               thread_info[thread_index].rtpmaps[nInput] = th_rtpmaps;

               thread_info[thread_index].num_rtpmaps[nInput] += num_rtpmaps;  /* increment number of rtpmaps. Note we're not inside a num_rtpmaps loop, so we're adding all rtpmaps from the current while-loop Media object vs adding one-by-one as with Origin objects, JHB Jan 2023 */

               nMediaObjectsAdded++;
            }
         }
      }
   }

/* format and display/log summary message */

   if (nOriginsFound || nMediaObjectsFound) {

      char szLogSummary[300] = "mediaMin INFO: SDP info with ";

      if (nOriginsFound) {
         sprintf(&szLogSummary[strlen(szLogSummary)], "unique Origin session ID%s%s", nOriginsFound > 1 ? "s" : "", szSessionIDs);
         if (!nMediaObjectsFound && !nMediaObjectsAdded) sprintf(&szLogSummary[strlen(szLogSummary)], "%s added to database", nOriginsAdded ? "" : " found but not");
      }

      if (nMediaObjectsFound) {
         sprintf(&szLogSummary[strlen(szLogSummary)], "%s%d Media description%s and RTP attributes", nOriginsFound ? " and " : "", nMediaObjectsFound, nMediaObjectsFound > 1 ? "s" : "");
         sprintf(&szLogSummary[strlen(szLogSummary)], "%s added to database", nMediaObjectsAdded ? "" : " found but not");
      }

      sprintf(&szLogSummary[strlen(szLogSummary)], " for thread %d stream %d%s", thread_index, nInput, (uFlags & SDP_PARSE_ADD) ? "" : ". To add, apply the ENABLE_STREAM_SDP_INFO flag in cmd line -dN options");

      Log_RT(4, "%s \n", szLogSummary);
   }

// printf("\n returning from SDPParseInfo() %d\n", nOriginsAdded);

   return nOriginsAdded;
}

/* format_sdp_str() removes any (i) extra trailing zeros and end-of-lines, (ii) blank lines */

void format_sdp_str(char* szSDP, int len) {

int i = len-1;

   while (szSDP[i] == 0 && i>0) i--;  /* discard any trailing zeros. SAP packets again, arggh */

   while (i > 0 && (szSDP[i] == 0x0a || szSDP[i] == 0x0d)) { szSDP[i] = 0; i--; }  /* trim any Linux or Windows trailing end-of-lines */
   szSDP[i+1] = 0;  /* tack on a zero to be sure before doing any string operations */
   strcat(szSDP, "\n");  /* add one and only one Linux type end-of-line */

#if 1
find_blank_lines:

   len = strlen(szSDP);
   int j, k = 0;

   for (i=0; i<len; i++) {

     if ((szSDP[i] == 0x0a || szSDP[i] == 0x0d) && (szSDP[i+1] == 0x0a || szSDP[i+1] == 0x0d)) {

        szSDP[i++] = 0x0a;  /* leave one Linux style end-of-line, strip out anything else */
        j = i;
        while ((szSDP[j] == 0x0a || szSDP[j] == 0x0d) && j < len) { j++; k = 1; }
        if (k) {
            memmove(&szSDP[i], &szSDP[j], len - j);
            szSDP[len - (j-i)] = 0;
            goto find_blank_lines;
         }
      }
   }
#endif

   #if 0  /* handy debug to see szSDP in its final form before parsing, JHB Apr 2023 */
   printf(" **** inside szSDP cleanup: \n%safter szSDP \n", szSDP);
   #endif
}

/* add SDP info from cmd line file. Notes:

   -we expect a similar format as SIP invite packets, so we use the SESSION_CONTROL_ADD_SIP_INVITE_SDP_INFO flag
   -read whole file into a std::string (sdpstr) and submit to SDPParseInfo()
   -To-Do: cmd line SDP info applies to all inputs  -- we may need to modify this to allow per-stream .sdp files
*/

int SDPSetup(const char* szSDPFile, int thread_index) {

std::string sdpstr;
int ret_val = 0;

   if (!szSDPFile || !strlen(szSDPFile)) {
      Log_RT(2, "mediaMin ERROR: SDPSetup() says SDP filename NULL or has zero length \n");
      return -1;
   }

/* read SDP file specified in -s cmd line option into a string (sdpstr) */

   std::ifstream ifs(szSDPFile, std::ios::in);  /* note - ifs is an RAII object so ifstream will be closed when function exits */

   if (!ifs.is_open()) {
      Log_RT(2, "mediaMin ERROR: SDPSetup() says cmd line -s arg, SDP file %s not found \n", szSDPFile);
      return -1;
   }

   std::string temp_sdpstr( (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>() );  /* ignore the extra (), it's a case of Most Vexing Parse (https://en.wikipedia.org/wiki/Most_vexing_parse) */
   sdpstr = temp_sdpstr;

   #ifdef PRINTSDPDEBUG
   printf("\n sdp file = \n%s \n", sdpstr.c_str());  /* print all lines of sdp file */
   #endif

   char* szSDP = (char*)malloc(sdpstr.size());
   strcpy(szSDP, sdpstr.c_str());

/* make a condensed SDP info version for display and logging */

find_comment:

   int i, j, k = 0, len = strlen(szSDP);

   for (i=0; i<len; i++) if (szSDP[i] == '#') {  /* strip out comments */

      for (j=i+1; j<len; j++) {

         while ((szSDP[j] == 0x0a || szSDP[j] == 0x0d) && j < len) { j++; k = 1; }
         if (k) break;
      }

      memmove(&szSDP[i], &szSDP[j], len - j);
      szSDP[len - (j-i)] = 0;
      goto find_comment;  /* repeat until all comments are removed */
   }

   format_sdp_str(szSDP, strlen(szSDP));  /* remove blank lines and any extra trailing zeros or end-of-lines */

   Log_RT(4, "mediaMin INFO: opened SDP file %s and parsing contents as follows \n%s", szSDPFile, szSDP);

   if (strlen(szSDP)) for (int nStream=0; nStream<thread_info[thread_index].nInPcapFiles; nStream++) {

   /* parse SDP info according to SIP Invite format and add any valid results to all streams' SDP info database. Note that SDPParseInfo() handles all error and status/progress messages. To-do: find a way to handle per-stream .sdp files on the command line, JHB Jan 2023 */

      if ((ret_val = SDPParseInfo(sdpstr, SESSION_CONTROL_ADD_SIP_INVITE_SDP_INFO | SDP_PARSE_ADD, nStream, thread_index)) < 0) break;
   }

   if (szSDP) free(szSDP);

   return ret_val;
}


/* list of SIP messages we look for. SIP_MESSAGES struct is defined in sdp_app.h */

static SIP_MESSAGES SIP_Messages[] = { {"100 Trying", "100 Trying", SESSION_CONTROL_FOUND_SIP_TRYING},
                                       {"180 Ringing", "180 Ringing", SESSION_CONTROL_FOUND_SIP_RINGING},
                                       {"183 Session", "183 Session Progress", SESSION_CONTROL_FOUND_SIP_PROGRESS},
                                       {"PRACK sip", "Prov ACK", SESSION_CONTROL_FOUND_SIP_PROV_ACK},
                                       {"ACK sip", "ACK", SESSION_CONTROL_FOUND_SIP_ACK},
                                       {"200 OK", "200 Ok", SESSION_CONTROL_FOUND_SIP_OK},
                                       {"BYE\r", "BYE", SESSION_CONTROL_FOUND_SIP_BYE},  /* BYE followed by either carriage return or line feed, per RFC 2327, JHB Mar 2023 */
                                       {"BYE\n", "BYE", SESSION_CONTROL_FOUND_SIP_BYE},
                                       {"Invite", "Invite", SESSION_CONTROL_FOUND_SIP_INVITE},
                                       {"INVITE sip", "Invite", SESSION_CONTROL_FOUND_SIP_INVITE},
                                       {"200 Playing Announcement", "200 Playing Announcement", SESSION_CONTROL_FOUND_SIP_PLAYING_ANNOUNCEMENT},
                                       {"INFO", "INFO Request", SESSION_CONTROL_FOUND_SIP_INFO_REQUEST}
                                     };

uint8_t* find_keyword(uint8_t* buffer, uint16_t buflen, const char* szKeyword, bool fCaseInsensitive) {

   if (fCaseInsensitive) {  /* if fCaseInsensitive specified we use strupr() and assume relatively small buffer and substring sizes */

      char tmpstr1[500];
      char tmpstr2[100];
      int str1len = min((int)(sizeof(tmpstr1)-1), buflen-1);

      memcpy(tmpstr1, buffer, str1len);
      tmpstr1[str1len] = 0;
      strupr(tmpstr1);
      strcpy(tmpstr2, szKeyword);
      strupr(tmpstr2);

      return (uint8_t*)strstr(tmpstr1, tmpstr2);
   }

   return (uint8_t*)memmem(buffer, buflen, (const void*)szKeyword, strlen(szKeyword));
}

int ProcessSessionControl(uint8_t* pkt_in_buf, unsigned int uFlags, int nInput, int thread_index, char* szKeyword) {

   int pyld_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDLEN, pkt_in_buf, -1, NULL, NULL);
   int pyld_ofs = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDOFS, pkt_in_buf, -1, NULL, NULL);

   if (thread_info[thread_index].sip_save_len[nInput]) {

      memmove(&pkt_in_buf[pyld_ofs + thread_info[thread_index].sip_save_len[nInput]], &pkt_in_buf[pyld_ofs], pyld_len);
      memcpy(&pkt_in_buf[pyld_ofs], thread_info[thread_index].sip_save[nInput], thread_info[thread_index].sip_save_len[nInput]);
      pyld_len += thread_info[thread_index].sip_save_len[nInput];
      thread_info[thread_index].sip_save_len[nInput] = 0;
      free(thread_info[thread_index].sip_save[nInput]);
   }

   int session_pkt_type_found = 0, candidate_session_pkt_type_found = SESSION_CONTROL_FOUND_SIP_INVITE;
   int index = 0;

/* to-do: handle SIP_INVITE_look for REQUEST, STATUS, BYE SIP packets, print something out, JHB Jan 2023 */

type_check:

   char search_str[50] = "a=rtpmap";
   uint8_t* p;
   #ifndef USE_OLD_CONTENTS_START  /* change start from int to uint8_t*, simplifies code a bit and makes it easier to understand, JHB Apr 2023 */
   uint8_t* p_start;
   #else
   int start;
   #endif
   int len;

// if (index > pyld_len) fprintf(stderr, " ==== index %d > pyld_len %d \n", index, pyld_len);

   if (!(uFlags & SESSION_CONTROL_NO_PARSE) && pyld_len > index && (p = find_keyword(&pkt_in_buf[pyld_ofs+index], pyld_len-index, search_str, false))) {  /* first find rtpmap, then back up and look for length field or application keyword. Check for SESSION_CONTROL_NO_PARSE uFlag first, JHB Mar 2023 */

      strcpy(search_str, "Length:");
      p = find_keyword(&pkt_in_buf[pyld_ofs+index], (uint16_t)(p - &pkt_in_buf[index]), search_str, false);

      if (!p) {
         strcpy(search_str, "l: ");
         p = find_keyword(&pkt_in_buf[pyld_ofs+index], (uint16_t)(p - &pkt_in_buf[index]), search_str, false);
      }

      if (!p) {  /* SAP/SDP protocol packets do not include a length field */

         strcpy(search_str, "application");
         p = find_keyword(&pkt_in_buf[pyld_ofs+index], (uint16_t)(p - &pkt_in_buf[index]), search_str, false);

         candidate_session_pkt_type_found = SESSION_CONTROL_FOUND_SAP_SDP;
      }

      if (p) {

         //#define FINDINVITEDEBUG
         #ifdef FINDINVITEDEBUG
         static int count = 0;
         char tmpstr[4096];
         int j;
         memcpy(tmpstr, &pkt_in_buf[pyld_ofs+index], pyld_len-index);
         for (j=0; j<pyld_len-index; j++) if (tmpstr[j] < 32 && tmpstr[j] != 10 && tmpstr[j] != 13) tmpstr[j] = 176;  /* fill with printable char */
         tmpstr[pyld_len-index] = 0;
         #endif

         uint8_t* p_ofs = p;
         uint8_t* p_siprec = NULL;

         if (candidate_session_pkt_type_found == SESSION_CONTROL_FOUND_SIP_INVITE) {

            p += strlen(search_str);
            int i = 0;
            while (p[i] >= 0x20) i++;
            p[i] = 0;
            len = atoi((const char*)p);

            if (len <= 1 || len > (int)MAX_TCP_PACKET_LEN) { session_pkt_type_found = -1; goto ret; };  /* invalid Length: value */

         /* additional search to handle INVITE formats where non-SDP info lines appear between Content-Length: and actual SDP info, JHB May2021

            -Content-Length: value only applies to actual SDP info, but in some cases (e.g. siprec) may include additional, non-useful info
            -this makes us dependent on presence of v=0. RFC4566 does say v= is mandatory, and for many years version is zero
         */

            uint8_t* p2;
            strcpy(search_str, "v=0");
            p2 = find_keyword(p, pyld_len - index - (int)(&p[i] - &pkt_in_buf[pyld_ofs+index]), search_str, false);
            if (!p2) { p2 = find_keyword(p, pyld_len - index - (int)(&p[i] - &pkt_in_buf[pyld_ofs+index]), "v=1", false); if (p2) strcpy(search_str, "v=1"); }  /* also try v=1 in case SIP guys ever bump version from 0.x to 1.x (unlikely but not impossible) */

            if (!p2) goto ret;  /* v=0 not found */

         /* Session Recording Protocol (SIPREC, RFC 7866) is an open SIP based protocol for call recording, partly based on RFC 7245 (https://datatracker.ietf.org/doc/id/draft-portman-siprec-protocol-01.html) */

            p_siprec = find_keyword(p2, pyld_len - index - (int)(p2 - &pkt_in_buf[pyld_ofs+index]), "--OSS-unique-boundary-42", false);  /* look for siprec header, JHB Apr 2023 */

            if (p_siprec) {  /* siprec Invite has a different format, with "unique-boundary" marked header and footer, and XML section */

               len = (int)(p_siprec - p2);  /* for siprec, use alternative len calculation -- avoid (i) siprec header intro and padding before v=0 (ii) XML section after end of sdp info (separated by another siprec header, if found), JHB Apr 2023 */
            }

            #ifndef USE_OLD_CONTENTS_START
            p_start = p2;  /* start of contents ("v=0") */
            #else
            start = (int)(p2 - p);  /* offset to start of contents ("v=0") */
            #endif
         }
         else {  /* SAP/SDP protocol packets are lightweight with no header info (e.g. length:, v=, etc) */

            len = pyld_len - (int)(p - &pkt_in_buf[pyld_ofs+index]);
            #ifndef USE_OLD_CONTENTS_START
            p_start = p;  /* start of contents ("application" keyword) */
            #else
            start = 0;  /* offset to start of contents ("application" keyword) */
            #endif
         }

         #ifndef USE_OLD_CONTENTS_START
         int rem = pyld_len - index - (int)(p_start - &pkt_in_buf[pyld_ofs+index]);
         #else
         int rem = pyld_len - index - (int)(&p[start] - &pkt_in_buf[pyld_ofs+index]);
         #endif

         #ifdef FINDINVITEDEBUG
         printf("\nSIP invite debug %d\n pyld_ofs = %d, pyld_len = %d, index = %d \n len = %d, rem = %d \n", count, pyld_ofs, pyld_len, index, len, rem);
         #endif

         if (len > rem) {  /* save partial SIP invite, starting with "Length:" */

            thread_info[thread_index].sip_save_len[nInput] = pyld_len - (p_ofs - &pkt_in_buf[pyld_ofs]);
            thread_info[thread_index].sip_save[nInput] = (uint8_t*)malloc(thread_info[thread_index].sip_save_len[nInput]);
            memcpy(thread_info[thread_index].sip_save[nInput], p_ofs, thread_info[thread_index].sip_save_len[nInput]);
         }
         else {  /* SIP invite or SAP/SDP protocol found, display and/or extract Origin and Media objects from SDP info, add to thread_info[].origins[stream] */

            char szSDP[MAX_TCP_PACKET_LEN];
            #ifndef USE_OLD_CONTENTS_START
            memcpy(szSDP, p_start, len);
            #else
            memcpy(szSDP, &p[start], len);
            #endif

            uint8_t* p2 = (uint8_t*)memmem(&szSDP[0], len, "sdp", 3);
            if (p2 && *(p2+3) == 0) *(p2+3) = '\n';  /* some SAP packet generators seem to stick a zero after "application/sdp" and before "m=", which I don't see in any spec, but whatever. If so we replace with a new line, JHB Jan 2023 */

            format_sdp_str(szSDP, len);  /* remove extra trailing end-of-lines, if any */

            session_pkt_type_found = candidate_session_pkt_type_found;

            if (uFlags & SESSION_CONTROL_SHOW_ALL_MESSAGES) {  /* display/log mediaMin INFO message and SDP info contents */

               uint16_t dest_port = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_DST_PORT, pkt_in_buf, -1, NULL, NULL);

               Log_RT(4, "mediaMin INFO: %s found, dst port = %u, pyld len = %d, len = %d, rem = %d, index = %d, SDP info contents as follows \n%s", (session_pkt_type_found == SESSION_CONTROL_FOUND_SIP_INVITE) ? "SIP Invite" : "SAP/SDP protocol", dest_port, pyld_len, len, rem, index, szSDP);
            }

// printf(" *** before SDPParseInfo \n");

            SDPParseInfo(szSDP, (uFlags & SESSION_CONTROL_ADD_ITEM_MASK) ? SDP_PARSE_ADD : 0, nInput, thread_index);  /* SDPParseInfo() will show messages if SDP infos are invalid, repeats of existing session IDs, or added to the input stream's SDP database. Note that return value is number of Invite added, if that should be needed, JHB Jan 2023 */

            #ifndef USE_OLD_CONTENTS_START
            index = (int)(p_start - &pkt_in_buf[pyld_ofs+index]) + len;
            #else
            index = &p[start] - &pkt_in_buf[pyld_ofs+index] + len;
            #endif

            goto type_check;  /* look for more SDP info contents in this packet */
         }
      }
   }
   else if (uFlags & SESSION_CONTROL_SHOW_ALL_MESSAGES) {  /* if requested, parse other (i.e. non-Invite) SIP message types */

      strcpy(search_str, "");
      int i, num_session_types = sizeof(SIP_Messages)/sizeof(SIP_MESSAGES);

      for (i=0; i<num_session_types; i++) if (find_keyword(&pkt_in_buf[pyld_ofs], pyld_len, SIP_Messages[i].szTextStr, true)) {

         strcpy(search_str, SIP_Messages[i].szType);
         session_pkt_type_found = SIP_Messages[i].val;

         uint16_t dest_port = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_DST_PORT, pkt_in_buf, -1, NULL, NULL);
         Log_RT(4, "mediaMin INFO: SIP %s message found, dst port = %u, pyld len = %d, index = %d \n", search_str, dest_port, pyld_len, index);
         break;
      }
   }

ret:
   if (szKeyword && strlen(search_str)) strcpy(szKeyword, search_str);
   return session_pkt_type_found;
}
