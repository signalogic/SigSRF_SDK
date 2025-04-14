/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/sdp_app.cpp

 Copyright (C) Signalogic Inc. 2021-2025

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  SDP related parsing and object management, SIP Invite and other SIP message processing
  
 Purpose
 
  support for mediaMin reference application

 Documentation

  https://www.github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediamin

  Older documentation links:
  
    after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

    before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

 Notes

    Uses sdp namespace and SDP class in apps/common/sdp

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
   Modified Apr 2023 JHB, change packet data offset from int to uint8_t*, simplifies code a bit and makes it easier to understand. Look for p_start
   Modified May 2023 JHB, implement a case-insensitive search option in find_keyword(). This handles observed SIP message headers with variations in case
   Modified Jun 2023 JHB, additional SIP Invite debug if needed, look for FIND_INVITE_DEBUG
   Modified Jun 2024 JHB, in SESSION_CONTROL_NO_PARSE section (latter part) of ProcessSessionControl(), add logic to omit Invite messages if the SESSION_CONTROL_SIP_INVITE_MESSAGES flag is not given in uFlags, and omit Bye messages if the SESSION_CONTROL_SIP_BYE_MESSAGES FLAG is not given. This gives mediaMin and user apps more control over what is parsed and displayed
   Modified Jun 2024 JHB, include protocol type (TCP or UDP) in SIP messages
   Modified Jun 2024 JHB, in ProcessSessionControl() return SESSION_CONTROL_FOUND_SIP_FRAGMENT when saving payload data for an incomplete / fragmented SDP info packet
   Modified Jun 2024 JHB, in SDPParseInfo() add media descriptions (e.g. m=video 45002 RTP/AVP xxx xxx) to thread_info[]
   Modified Jun 2024 JHB, include packet number, non-fragment dst port, and protocol in log/console messages
   Modified Jun 2024 JHB, use CRC32 to look for duplicate SDP info; if found, don't parse and don't report
   Modified Jul 2024 JHB, per change in pktlib.h, edit DSGetPacketInfo() calls to make uFlags last param
   Modified Nov 2024 JHB, include directcore.h (no longer implicitly included in other header files)
   Modified Dec 2024 JHB, include <algorithm> and use std namespace
   Modified Feb 2025 JHB, add fmtp parsing, save in thread_info[] fmtps attribute per-stream vectors
   Modified Feb 2025 JHB, for all items, not only origins, search database first to see if already there and avoid adding duplicates. This became necessary after video stream test cases where SDP info is repeated at regular intervals
   Modified Mar 2025 JHB, fix bug in media description search (look for SDP_MEDIA_ANY)
   Modified Mar 2025 JHB, improve display of SDP info originating from .sdp files (i) implement SDP_PARSE_ALLOW_ZERO_ORIGIN flag, (i) leave comments occurring later on the same line as SDP info text, (iii) remove whitespace lines (e.g. all spaces then a CRLF)
   Modified Apr 2025 JHB, improve SIP message detection and display, add SESSION_CONTROL_FOUND_SIP_TCP_OTHER and SESSION_CONTROL_FOUND_SIP_UDP_OTHER flags, add port exclude and text exclude to avoid MySQL messages with similar keywords as SIP or messages with conflicting keywords
   Modified Apr 2025 JHB, fix bug in find_keyword() case-insensitive search, return value is offset relative to buffer input param, not tmpstr
*/

#include <algorithm>  /* bring in std::min and std::max */
#include <fstream>

using namespace std;

#include "directcore.h"  /* DirectCore APIs */
#include "pktlib.h"      /* DSGetPacketInfo() */
#include "diaglib.h"     /* bring in Log_RT() definition */

#include <sdp/sdp.h>     /* SDP info API header file (../apps/common/sdp) */
#include <crc/crc.h>     /* CRC32 routine (in ../apps/common/crc */

#include "mediaTest.h"   /* bring in some constants needed by mediaMin.h */
#include "mediaMin.h"    /* bring in THREAD_INFO typedef for thread_info[].xx[] access, indexed by thread and input stream */
#include "sdp_app.h"

#ifdef FRAGMENT_DEBUG
#include "user_io.h"     /* PrintPacketBuffer() */
#endif

extern APP_THREAD_INFO thread_info[];  /* THREAD_INFO struct defined in mediaMin.h, indexed by thread_index */

/* SDPParseInfo() notes:

  -expects SDP info in sdpstr as plain text per RFC 8866, without any additional header or other packet content
  -adds SDP info to thread_info[] SDP data, for reference during dynamic session creation
  -SDP info can be from command line .sdp file or SIP invite packet text data. SDP info can contain multiple Media elements, multiple rtpmap attributes
  -SDP info can be added at any time, in any sequence (cmd line .sdp file, if one, is added first)
  -currently duplicate media elements and rtpmap attributes are not filtered out. When searching through an rtpmap vector, it's application dependent on whether first or latest matching rtpmap is used
  -when struct items are added in sdp/types.h (e.g. Media, AttributeRTP, AttributeFMTP) the new items should also be added to comparisons below. To-do: created an overloaded == comparison for the sdp class
*/

int SDPParseInfo(std::string sdpstr, unsigned int uFlags, int nStream, int thread_index) {

/* SDP related items */

   sdp::SDP sdp_session;
   sdp::Reader reader;
//   sdp::Writer writer;  /* not currently used */
   sdp::Media* media = NULL;
//   sdp::Media media = {};
   int nodes = 0;

// #define PRINT_SDP_DEBUG  /* turn on for SDP parsing debug */

   if (!sdpstr.length()) return -1;  /* empty string is an error */

/* parse input SDP info string into an sdp::SDP session */

   reader.parse(sdpstr, &sdp_session, 0);

/* retrieve Media elements ("m=audio, m=video") */

   #ifdef PRINT_SDP_DEBUG
   sdp_session.print(NULL);
   #endif

/* Sequence and logic, JHB Jan 2023:

   -first search for an origin field. If any found through them and create Origin objects. If not found then do the loop at least once in order to find all Media objects. To-Do: at some future point we may associate each Origin object with a group of Media objects
 
   -inside the loop

     -compare found originator description session IDs with thread_info[].origins[stream] originator descriptions. If already existing, don't add an Origin object, assuming the SIP sender has repeated a SIP Invite for whatever reason
     -if an Origin object does not already exist then add it
     -find all media descriptions and add Media objects to the stream's SDP database, including their RTP, RTCP, etc attributes. Currently this means that for multiple Origins, all Media objects get added after the first Origin (see To-Do note above)

   -second summarize results, including display/log messages (see nOriginsFound and nMediaObjectsFound)

   -return number of origins added

   Notes

   -media and origin nodes are parents and attribute nodes are children of media element nodes (see reader.cpp)
   -in find() functions in types.cpp, starting node is zero unless we pass in a start node. nodes.size() = 0 means no nodes were found/added (see types.cpp)
   -after finding a media node, we advance by one in the while loop to find attributes associated with that media node, and for searching for the next media node. For origin nodes search those already found/added looking for unique session Id
   -it's possible to loop/search through attributes objects also, but currently we're not doing that (unless the debug #define is turned on)
*/

   vector<sdp::Origin*> origins = {};  /* create origin vector, init to zero */
   int nOriginsFound = 0, nOriginsAdded = 0, nMediaObjectsFound = 0, nMediaObjectsAdded = 0, nRtpmapsFound = 0, nRtpmapsAdded = 0, nFmtpsFound = 0, nFmtpsAdded = 0;
   bool fMediaAlreadyExist = true, fRtpmapsAlreadyExist = true, fFmtpsAlreadyExist = true;

/* search for origin nodes, if any found then we find out how many, gather info on them (e.g. session ID), and iterate through them */

   int num_origins = sdp_session.find(sdp::SDP_ORIGIN, origins, NULL);  /* retrieve origin nodes if any. Currently we don't save origin node numbers (which are essentially line numbers) */
   bool fParseOrigins = !(uFlags & SDP_PARSE_IGNORE_ORIGINS) && (num_origins > 0);  /* don't look for origins if IGNORE flag is set */

// printf(" *** after session.find \n");

   char szSessionIDs[300] = "";

  //printf(" \n *** before origins loop, parse = %d, num origins = %d, o pos = %d \n", fParseOrigins, num_origins, (int)sdpstr.find("o="));

   for (int i=0; i<max(num_origins, 1); i++) {  /* we need to enter the loop at least once. fParseOrigins is checked inside the loop */

      #ifdef PRINT_SDP_DEBUG
      printf(" + num rtpmaps %d \n", num_origins);
      #endif

   /* loop through found origin nodes - should be only one per SDP info, but you never know */

      bool fNewOrigin = false;
      int j;

      if (fParseOrigins) {

         sdp::Origin* origin_found = (sdp::Origin*)origins[i];  /* make a scalar origin object to work with */

         #ifdef PRINT_SDP_DEBUG
         printf("%s orgin[%d], session id = %s, username = %s, session version = %llu, \n", !i ? "\n" : "", i, origin_found->sess_id.c_str(), origin_found->username.c_str(), (long long unsigned int)origin_found->sess_version);
         #endif

         if (((origin_found->sess_id == "0" && !(uFlags & SDP_PARSE_ALLOW_ZERO_ORIGIN)) || origin_found->sess_id == "")) Log_RT(4, "mediaMin INFO: SDP info with invalid Origin session ID %s not used \n", origin_found->sess_id.c_str());  /* consider SDP_PARSE_ALLOW_ZERO_ORIGIN flag, JHB Mar 2025 */
         else for (fNewOrigin = true, j=0; j<thread_info[thread_index].num_origins[nStream]; j++) {  /* search existing origins. Note we set fNewOrigin in case it's the first one */
 
            sdp::Origin* origin = (sdp::Origin*)thread_info[thread_index].origins[nStream][j];

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

               vector<sdp::Origin*> th_origins = thread_info[thread_index].origins[nStream];  /* append origin info */

//  printf(" before insert, session id = %s \n", origin_found->sess_id.c_str());

               th_origins.insert( th_origins.end(), origins.begin() + i, origins.begin() + i + 1 );  /* C++ still doesn't have append so we "insert at the end" ( https://stackoverflow.com/questions/2551775/appending-a-vector-to-a-vector) */
               thread_info[thread_index].origins[nStream] = th_origins;

               thread_info[thread_index].num_origins[nStream]++;  /* increment number of origins in stream's thread_info[] */

               nOriginsAdded++;
            }
         }
      }

      #if 0
      if (!fParseOrigins || nOriginsFound) while (sdp_session.find(sdp::SDP_AUDIO, &media, &nodes) || sdp_session.find(sdp::SDP_VIDEO, &media, &nodes)) {  /* find audio and video media elements. nodes is returned as the number of parent level nodes before and including the found media node, if one. See "parent and child node" notes in Reader::parse() in apps/common/sdp/reader.cpp. Node::find(MediaType t, Media** m, int* node) is in apps/common/sdp/types.cpp */
      #else  /* bug-fix: search for all media description type at one time, otherwise our order of search might be different from order in the SDP info. Test with 1920x1080_H.265.pcapng and VxxxxCALL_Exx_H265.pcapng, make sure all media descriptions, rtpmaps, and fmtps are reported, JHB Mar 2025 */
      if (!fParseOrigins || nOriginsFound) while (sdp_session.find(sdp::SDP_MEDIA_ANY, &media, &nodes)) {  /* find audio and video media elements. nodes is returned as the number of parent level nodes before and including the found media node, if one. See "parent and child node" notes in Reader::parse() in apps/common/sdp/reader.cpp. Node::find(MediaType t, Media** m, int* node) is in apps/common/sdp/types.cpp */
      #endif

         #ifdef PRINT_SDP_DEBUG
         printf(" + media found, current search node = %d \n", nodes);
         #endif

         nodes++;  /* advance media sibling node by one before searching for next media description. See "parent and child node" notes in Reader::parse() in apps/common/sdp/reader.cpp */

         nMediaObjectsFound++;

        {  /* currently we always add media descriptions and ignore the SD_PARSE_ADD flag. That might change so we still continue to use separate found and added counters */
 
         /* save found media descriptions in thread_info[] if not already existing. User apps can look at these to make exceptions for media ports outside standard port ranges (see mediaMin.cpp for an example), JHB Jun 2024 */

            bool fNewMedia = true;

            for (j=0; j<thread_info[thread_index].num_media_descriptions[nStream]; j++) {  /* search existing rtpmaps */

               sdp::Media* media_database = (sdp::Media*)thread_info[thread_index].media_descriptions[nStream][j];

               if (media->media_type == media_database->media_type &&
                   media->port == media_database->port &&
                   media->proto == media_database->proto &&
                   media->fmt == media_database->fmt) { fNewMedia = false; break; }
            }

            if (fNewMedia) {

               vector<sdp::Media*> th_media_descriptions = thread_info[thread_index].media_descriptions[nStream];  /* append media object */
               th_media_descriptions.insert( th_media_descriptions.end(), media );
               thread_info[thread_index].media_descriptions[nStream] = th_media_descriptions;

               thread_info[thread_index].num_media_descriptions[nStream]++;  /* increment number of media descriptions, JHB Jan 2023 */

               nMediaObjectsAdded++;
               fMediaAlreadyExist = false;
            }
         }

         vector<sdp::Attribute*> rtpmaps = {};  /* create an rtpmap vector, init to zero */

         if (int num_rtpmaps = media->find(sdp::SDP_ATTR_RTPMAP, rtpmaps, NULL)) {  /* search for rtpmaps within the found media node. int Node::find(AttrType t, std::vector<Attribute*>& result, int* node) is in apps/common/sdp/types.cpp */

            #ifdef PRINT_SDP_DEBUG
            printf(" + parsing num rtpmaps %d \n", num_rtpmaps);
            #endif

            nRtpmapsFound += num_rtpmaps;

            #ifdef PRINT_SDP_DEBUG
            for (int k=0; k<num_rtpmaps; k++) {  /* debug: loop through rtpmap attributes - this could be used to skip existing identical attributes, etc */

               sdp::AttributeRTP* rtpmap = (sdp::AttributeRTP*)rtpmaps[k];  /* map RTP attribute onto generic attribute in order to do something useful with it ... */

               printf("%s rtpmap[%d], pyld type = %d, codec type = %d, sample rate = %d, num chan = %d \n", !k ? "\n" : "", k, rtpmap->pyld_type, rtpmap->codec_type, rtpmap->clock_rate, rtpmap->num_chan);
            }
            #endif

            for (int k=0; k<num_rtpmaps; k++) {  /* loop through rtpmaps found */

            /* if identical rtpmap is already there don't add another one */

               bool fNewRtpmap = true;

               sdp::AttributeRTP* rtpmap_found = (sdp::AttributeRTP*)rtpmaps[k];  /* make a scalar for comparison purposes */

               for (j=0; j<thread_info[thread_index].num_rtpmaps[nStream]; j++) {  /* search existing rtpmaps */

                  sdp::AttributeRTP* rtpmap = (sdp::AttributeRTP*)thread_info[thread_index].rtpmaps[nStream][j];

               /* compare with existing rtpmap, if duplicate found break out of loop */

                  if (rtpmap->pyld_type == rtpmap_found->pyld_type &&
                      rtpmap->codec_type == rtpmap_found->codec_type &&
                      rtpmap->clock_rate == rtpmap_found->clock_rate &&
                      rtpmap->num_chan == rtpmap_found->num_chan) { fNewRtpmap = false; break; }  /* I tried using just rtpmap == rtpmap_found but doesn't work, I guess because "C++ only automatically defines the equality operator (==) for built-in types (e.g., int, float, char). For user-defined types, if you don't explicitly define ==, the compiler might provide a default implementation that performs a shallow comparison. This means it simply checks if the memory addresses of the two objects are the same" per Gemini */
               }
 
               if (fNewRtpmap && (uFlags & SDP_PARSE_ADD)) {  /* add new rtpmap if found */

               /* save found rtmpaps in thread_info[] for reference in create_dynamic_session() in mediaMin.cpp */

                  vector<sdp::Attribute*> th_rtpmaps = thread_info[thread_index].rtpmaps[nStream];  /* append rtpmap attribute */
                  th_rtpmaps.insert( th_rtpmaps.end(), rtpmap_found );
                  thread_info[thread_index].rtpmaps[nStream] = th_rtpmaps;

                  thread_info[thread_index].num_rtpmaps[nStream]++;

                  #if 0
                  thread_info[thread_index].num_rtpmaps[nStream] += num_rtpmaps;  /* increment number of rtpmaps. Note we're not inside a num_rtpmaps loop, so we're adding all rtpmaps from the current while-loop Media object vs adding one-by-one as with Origin objects. Each Media object may add more rtpmaps, JHB Jan 2023 */
                  #endif

                  nRtpmapsAdded++;
                  fRtpmapsAlreadyExist = false;
               }
            }

         /* if an rtpmap is found, also look for fmtp. Currently payload types don't have to match, we just parse them and add to SDP database, JHB Feb 2025 */

            vector<sdp::Attribute*> fmtps = {};  /* create an fmtp vector, init to zero */

            if (int num_fmtps = media->find(sdp::SDP_ATTR_FMTP, fmtps, NULL)) {

               #ifdef PRINT_SDP_DEBUG
               printf(" + parsing num fmtps %d \n", num_fmtps);
               #endif

               nFmtpsFound += num_fmtps;  /* search for fmtps, starting with first media node. int Node::find(AttrType t, std::vector<Attribute*>& result, int* node) is in apps/common/sdp/types.cpp) */
 
            /* always add fmtps to SDP database, they may or may not be needed. For example, for video we may need sprop-xps fields if the RTP stream doesn't include in-band xps info, JHB Feb 2025 */

               for (int k=0; k<num_fmtps; k++) {  /* loop through fmtps found */

            /* if identical fmtp is already there don't add another one */

                  bool fNewFmtp = true;

                  sdp::AttributeFMTP* fmtp_found = (sdp::AttributeFMTP*)fmtps[k];  /* make a scalar for comparison purposes */

                  for (j=0; j<thread_info[thread_index].num_fmtps[nStream]; j++) {  /* search existing fmtps */
 
                     sdp::AttributeFMTP* fmtp = (sdp::AttributeFMTP*)thread_info[thread_index].fmtps[nStream][j];

//    printf("\n *** fmtp found options = %s, comparing with existing %s \n", fmtp_found->options.c_str(), fmtp->options.c_str());

                  /* compare with existing fmtp, if duplicate found break out of loop */

                     if (fmtp->pyld_type == fmtp_found->pyld_type && fmtp->options == fmtp_found->options) { fNewFmtp = false; break; }
                  }

                  if (fNewFmtp) {  /* add new fmtp if found */

                     vector<sdp::Attribute*> th_fmtps = thread_info[thread_index].fmtps[nStream];  /* append fmtp attribute */
                     th_fmtps.insert( th_fmtps.end(), fmtp_found );
                     thread_info[thread_index].fmtps[nStream] = th_fmtps;

                     thread_info[thread_index].num_fmtps[nStream]++;

                     nFmtpsAdded++;
                     fFmtpsAlreadyExist = false;
                  }
               }
            }
         }
      }  /* end of while audio/video media description search loop */
   }

/* format and display and/or log SDP info summary message of found / added items */

   if (nMediaObjectsFound || nRtpmapsFound || nFmtpsFound || nOriginsFound) {

      bool fPrev1 = false, fPrev2 = false;

      char szLogSummary[500] = "mediaMin INFO: SDP info with";

      if (nMediaObjectsFound) {
         sprintf(&szLogSummary[strlen(szLogSummary)], " %d %smedia description%s", nMediaObjectsFound, !fMediaAlreadyExist ? "unique " : "", nMediaObjectsFound > 1 ? "s" : "");
         sprintf(&szLogSummary[strlen(szLogSummary)], "%s %s database", nMediaObjectsAdded ? "" : " found but", fMediaAlreadyExist ? "already in" : (!nMediaObjectsAdded ? "not added to" : "added to"));
         fPrev1 = true;
      }

      if (nOriginsFound) {
         sprintf(&szLogSummary[strlen(szLogSummary)], "%s %d unique Origin session ID%s%s", fPrev1 ? "," : "", nOriginsFound, nOriginsFound > 1 ? "s" : "", szSessionIDs);
         sprintf(&szLogSummary[strlen(szLogSummary)], "%s added to database", nOriginsAdded ? "" : " found but not");
         fPrev2 = true;
      }

      if (nRtpmapsFound) {
         sprintf(&szLogSummary[strlen(szLogSummary)], "%s %d RTP attribute%s", fPrev1 || fPrev2 ? "," : "", nRtpmapsFound, nRtpmapsFound > 1 ? "s" : "");
         sprintf(&szLogSummary[strlen(szLogSummary)], "%s %s database", nRtpmapsAdded ? "" : " found but", fRtpmapsAlreadyExist && (uFlags & SDP_PARSE_ADD) ? "already in" : (!nRtpmapsAdded ? "not added to" : "added to"));
      }

      if (nFmtpsFound) {
         sprintf(&szLogSummary[strlen(szLogSummary)], "%s %d FMTP attribute%s", fPrev1 || fPrev2 ? "," : "", nFmtpsFound, nFmtpsFound > 1 ? "s" : "");
         sprintf(&szLogSummary[strlen(szLogSummary)], "%s %s database", nFmtpsAdded ? "" : " found but", fFmtpsAlreadyExist ? "already in" : (!nFmtpsAdded ? "not added to" : "added to"));
      }

      sprintf(&szLogSummary[strlen(szLogSummary)], " for thread %d stream %d%s", thread_index, nStream, uFlags & SDP_PARSE_ADD ? "" : ". To add origins and rtpmaps, apply the ENABLE_STREAM_SDP_INFO flag in cmd line -dN options");

      Log_RT(4, "%s \n", szLogSummary);  /* display and print to event log if enabled */
   }

   return nOriginsAdded;
}

#if 0
unsigned int checksum32(unsigned int data[], int len) {

   unsigned int sum = 0;
   for (int i=0; i<len; i++) sum += data[i];
   return -sum;
}
#endif

/* format_sdp_str() removes any (i) extra trailing zeros and end-of-lines, (ii) blank lines, (iii) early terminating zeros */

void format_sdp_str(char* szSDP, int len) {

int i, j;

   while (szSDP[len-1] == 0 && len-1 > 0) len--;  /* discard any trailing zeros. SAP packets again, arggh */

   while (len-1 > 0 && (szSDP[len-1] == 0x0a || szSDP[len-1] == 0x0d)) len--;  /* trim any Linux or Windows trailing end-of-lines */

   for (i=0; i<len; i++) if (szSDP[i] == 0) szSDP[i] = (unsigned char)176;  /* avoid early string termination, JHB Jun 2024 */
 
   szSDP[len] = 0;  /* add terminating NULL before any string operations */
   strcat(szSDP, "\n");  /* add one and only one Linux type end-of-line */

find_blank_lines:

   len = strlen(szSDP);
   int k = 0;

   for (i=0; i<len; i++) {

      if (szSDP[i] == 0x0a || szSDP[i] == 0x0d) {

         bool fStrip = false;

         if (i < len-1 && (szSDP[i+1] == 0x0a || szSDP[i+1] == 0x0d)) fStrip = true;
         else if (k == 0) fStrip = true;

         if (fStrip) {

            if (k > 0) szSDP[i++] = 0x0a;  /* leave one Linux style end-of-line, strip out anything else */
            j = i;
            bool fEndOfLine = false;
            while ((szSDP[j] == 0x0a || szSDP[j] == 0x0d) && j < len) { j++; fEndOfLine = true; }
            if (fEndOfLine) {
               memmove(&szSDP[i], &szSDP[j], len - j);
               szSDP[len - (j-i)] = 0;
               goto find_blank_lines;
            }
         }
      }

      k++;
   }

   #if 0  /* handy debug to see szSDP in its final form before parsing, JHB Apr 2023 */
   printf(" **** inside format_sdp_str after string cleanup: \n%safter szSDP \n", szSDP);
   #endif
}

/* add SDP info from cmd line file. Notes:

   -we expect a similar format as SIP invite packets, so we use the SESSION_CONTROL_ADD_SIP_INVITE_SDP_INFO flag
   -read whole file into a std::string (sdpstr) and submit to SDPParseInfo()
   -To-do: cmd line SDP info applies to all inputs  -- we may need to modify this to allow per-stream .sdp files
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

   std::string temp_sdpstr( (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>() );  /* ignore the extra (), it's a case of Most Vexing Parse (not kidding: https://en.wikipedia.org/wiki/Most_vexing_parse) */
   sdpstr = temp_sdpstr;

   #ifdef PRINT_SDP_DEBUG
   printf("\n sdp file = \n%s \n", sdpstr.c_str());  /* print all lines of sdp file */
   #endif

   char* szSDP = (char*)malloc(sdpstr.size());
   strcpy(szSDP, sdpstr.c_str());

/* make a trimmed and formatted SDP info version for display and logging */

find_comment:

   int i, j, k = 0, len = strlen(szSDP);
   int nWhiteSpace = 0;

   for (i=0; i<len; i++) {

      if (szSDP[i] == ' ' || szSDP[i] == 0x0d) nWhiteSpace++;  /* possibly add tabs or other white space to this */

      if ((szSDP[i] == 0x0a) && k > 0 && k == nWhiteSpace) {  /* remove leading white space */

         memmove(&szSDP[i-nWhiteSpace], &szSDP[i], len-i);
         szSDP[len - nWhiteSpace] = 0;

         goto find_comment;  /* repeat until all comments are handled */
      }

      if (szSDP[i] == '#' && k == nWhiteSpace) {  /* strip out comments occurring by themselves (on seperate lines), leave comments occurring later on the same line as SDP info text. Note that RFC 8866 allows '#' in the s= and i= (session or stream description, additional info) fields, and we would not strip those because they would not be stand-alone comment lines, JHB Mar 2025 */

         for (j=i+1; j<len; j++) {

            bool fEndOfLine = false;
            while ((szSDP[j] == 0x0a || szSDP[j] == 0x0d) && j < len) { j++; fEndOfLine = true; }
            if (fEndOfLine) break;
         }

         memmove(&szSDP[i], &szSDP[j], len - j);
         szSDP[len - (j-i)] = 0;
         goto find_comment;  /* repeat until all comments are handled */
      }

      if (szSDP[i] == 0x0a) { k = 0; nWhiteSpace = 0; }
      else k++;
   }

   format_sdp_str(szSDP, strlen(szSDP));  /* remove blank lines and any extra trailing zeros or end-of-lines */

   Log_RT(4, "mediaMin INFO: opened SDP file %s and parsing contents as follows \n%s", szSDPFile, szSDP);

   if (strlen(szSDP)) for (int nStream=0; nStream<thread_info[thread_index].nInPcapFiles; nStream++) {

   /* parse SDP info according to SIP Invite format and add any valid results to all streams' SDP info database. Note that SDPParseInfo() handles all error and status/progress messages. To-do: find a way to handle per-stream .sdp files on the command line, JHB Jan 2023 */

      if ((ret_val = SDPParseInfo(sdpstr, SESSION_CONTROL_ADD_SIP_INVITE_SDP_INFO | SDP_PARSE_ADD | SDP_PARSE_ALLOW_ZERO_ORIGIN, nStream, thread_index)) < 0) break;  /* add SDP_PARSE_ALLOW_ZERO_ORIGIN flag, JHB Mar 2025 */
   }

   if (szSDP) free(szSDP);

   return ret_val;
}


/* list of SIP messages we look for. SIP_MESSAGES struct is defined in sdp_app.h */

static SIP_MESSAGES SIP_Messages[] = { {"100 Trying", "100 Trying", SESSION_CONTROL_FOUND_SIP_TRYING, "", 0},
                                       {"180 Ringing", "180 Ringing", SESSION_CONTROL_FOUND_SIP_RINGING, "", 0},
                                       {"183 Session", "183 Session Progress", SESSION_CONTROL_FOUND_SIP_PROGRESS, "", 0},
                                       {"PRACK sip", "Prov ACK", SESSION_CONTROL_FOUND_SIP_PROV_ACK, "", 0},
                                       {"ACK sip", "ACK", SESSION_CONTROL_FOUND_SIP_ACK, "", 0},
                                       {"200 OK", "200 Ok", SESSION_CONTROL_FOUND_SIP_OK, "", 0},
                                       {"BYE\r", "BYE", SESSION_CONTROL_FOUND_SIP_BYE, "", 0},  /* BYE followed by either carriage return or line feed, per RFC 2327, JHB Mar 2023 */
                                       {"BYE\n", "BYE", SESSION_CONTROL_FOUND_SIP_BYE, "", 0},
                                       {"Invite", "Invite", SESSION_CONTROL_FOUND_SIP_INVITE, "SUBSCRIBE", 3306},   /* generic "invite" but exclude messages with "subscribe" and MySQL messages (port 3306) with similar keywords */
                                       {"INVITE sip", "Invite", SESSION_CONTROL_FOUND_SIP_INVITE, "SUBSCRIBE", 0},  /*  ""   ""  */
                                       {"200 Playing Announcement", "200 Playing Announcement", SESSION_CONTROL_FOUND_SIP_PLAYING_ANNOUNCEMENT, "", 0},
                                       {"INFO", "INFO Request", SESSION_CONTROL_FOUND_SIP_INFO_REQUEST, "", 3306},  /* generic "info" but exclude MySQL messages (port 3306) with similar keywords */
                                       {"UDP SIP/2.0", "Options or other", SESSION_CONTROL_FOUND_SIP_UDP_OTHER, "", 0},  /* includes OPTIONS, CANCEL, REGISTER, etc. Lowest priority in search */
                                       {"TCP SIP/2.0", "Options or other", SESSION_CONTROL_FOUND_SIP_TCP_OTHER, "", 0}   /* same, TCP */
                                     };

uint8_t* find_keyword(uint8_t* buffer, uint16_t buflen, const char* szKeyword, bool fCaseInsensitive) {

   if (!strlen(szKeyword)) return NULL;

   if (fCaseInsensitive) {  /* if fCaseInsensitive specified we copy buffer to a temporary string, being careful not to overflow and replacing any zero chars, and do case insensitive search, JHB May 2023 */

      char tmpstr[4000]; int i = 0;

      for (int j=0; j<min((int)(sizeof(tmpstr)-1), (int)buflen); j++) tmpstr[i++] = buffer[j] != 0 ? buffer[j] : 127;  /* copy buffer to temporary string, replace zeros with something temporary */
      tmpstr[i] = 0;  /* add terminating zero */

      uint8_t* p_ret = (uint8_t*)strcasestr(tmpstr, szKeyword);  /* case-insensitive comparison */

      return p_ret ? p_ret - (uint8_t*)tmpstr + buffer : NULL;  /* if found return offset relative to buffer, JHB Apr 2025 */
   }

   return (uint8_t*)memmem(buffer, buflen, (const void*)szKeyword, strlen(szKeyword));  /* case-exact search, ignoring any NULL chars */
}

int ProcessSessionControl(uint8_t* pkt_buf, unsigned int uFlags, int nStream, int thread_index, char* szKeyword) {

PKTINFO PktInfo;
bool fFragmentData = false;

   DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTINFO | DS_PKT_INFO_PKTINFO_EXCLUDE_RTP, pkt_buf, -1, &PktInfo, NULL);  /* get packet info excluding RTP items, JHB Jun 2024 */

   int pyld_len = PktInfo.pyld_len;
   int pyld_ofs = PktInfo.pyld_ofs;

   //#define FIND_INVITE_DEBUG

   #ifdef FIND_INVITE_DEBUG
   static int count = 0;
   int state = 0, save_amount = 0;
   #endif

/* insert previous fragment data (if any) at start of payload */

   if (thread_info[thread_index].sip_info_save_len[nStream]) {

      #ifdef FRAGMENT_DEBUG
      PrintPacketBuffer(thread_info[thread_index].sip_info_save[nStream], thread_info[thread_index].sip_info_save_len[nStream], " *** inside fragment restore, start of saved data \n", " *** end of saved data \n");
      #endif

      memmove(&pkt_buf[pyld_ofs + thread_info[thread_index].sip_info_save_len[nStream]], &pkt_buf[pyld_ofs], pyld_len);
      memcpy(&pkt_buf[pyld_ofs], thread_info[thread_index].sip_info_save[nStream], thread_info[thread_index].sip_info_save_len[nStream]);
      pyld_len += thread_info[thread_index].sip_info_save_len[nStream];

      #ifdef FIND_INVITE_DEBUG
      save_amount = thread_info[thread_index].sip_info_save_len[nStream];
      #endif

      #ifdef FRAGMENT_DEBUG
      char tmpstr[400];
      sprintf(tmpstr, " *** inside fragment restore, start of aggregated data, saved amount = %d, pyld_ofs = %d, pyld_len = %d, flags = 0x%x \n", thread_info[thread_index].sip_info_save_len[nStream], pyld_ofs, pyld_len, PktInfo.flags);
      PrintPacketBuffer(&pkt_buf[pyld_ofs], pyld_len, tmpstr, " *** end of aggregated data \n");
      #endif

      thread_info[thread_index].sip_info_save_len[nStream] = 0;
      free(thread_info[thread_index].sip_info_save[nStream]);

      fFragmentData = true;
   }

   int session_pkt_type_found = 0, candidate_session_pkt_type_found = SESSION_CONTROL_FOUND_SIP_INVITE;
   int index = 0;
   bool fSIPInviteFoundMessageDisplayed = false;

/* handle SIP invite messages, if not found look for REQUEST, STATUS, BYE SIP packets, log/display status, JHB Jan 2023 */

type_check:

   char search_str[50] = "a=rtpmap";
   char search_str2[50] = "m=audio";
   uint8_t* p, *p_rtpmap;
   uint8_t* p_start;
   int len;

// if (index > pyld_len) fprintf(stderr, " ==== index %d > pyld_len %d \n", index, pyld_len);

   if (!(uFlags & SESSION_CONTROL_NO_PARSE) && pyld_len > index && ((p_rtpmap = find_keyword(&pkt_buf[pyld_ofs+index], pyld_len-index, search_str, false)) || (p_rtpmap = find_keyword(&pkt_buf[pyld_ofs+index], pyld_len-index, search_str2, false)))) {  /* first find rtpmap, then back up and look for length field or application keyword. Check for SESSION_CONTROL_NO_PARSE uFlag first, JHB Mar 2023 */

      strcpy(search_str, "Length:");
      p = find_keyword(&pkt_buf[pyld_ofs+index], (uint16_t)(p_rtpmap - &pkt_buf[index]), search_str, false);  /* fix bug: use saved location of "a=rtpmap" as upper limit for subsequent searches, JHB Jun 2024 */

      #ifdef FRAGMENT_DEBUG
      if (p) printf("\n *** inside ProcessSessionControl found Length, pyld_ofs = %d, pyld_len = %d \n", pyld_ofs, pyld_len);
      #endif

      if (!p) {
         strcpy(search_str, "l: ");
         p = find_keyword(&pkt_buf[pyld_ofs+index], (uint16_t)(p_rtpmap - &pkt_buf[index]), search_str, false);
      }

      if (!p) {  /* SAP/SDP protocol packets do not include a length field */

         strcpy(search_str, "application");
         p = find_keyword(&pkt_buf[pyld_ofs+index], (uint16_t)(p_rtpmap - &pkt_buf[index]), search_str, false);

         candidate_session_pkt_type_found = SESSION_CONTROL_FOUND_SAP_SDP;
      }

      if (p) {

         #if 0  /* debug - print out packet search area */
         PrintPacketBuffer(&pkt_buf[pyld_ofs+index], (int)(p - &pkt_buf[index]), " *** start of pkt \n" " *** end of pkt \n");  /* user_io.h */
         #endif

         #ifdef FIND_INVITE_DEBUG
         char tmpstr[4096];
         int j;
         memcpy(tmpstr, &pkt_buf[pyld_ofs+index], pyld_len-index);
         for (j=0; j<pyld_len-index; j++) if (tmpstr[j] < 32 && tmpstr[j] != 10 && tmpstr[j] != 13) tmpstr[j] = 176;  /* fill with printable char */
         tmpstr[pyld_len-index] = 0;
         state = 1;
         #endif

         uint8_t* p_ofs = p;
         uint8_t* p_siprec = NULL;

         if (candidate_session_pkt_type_found == SESSION_CONTROL_FOUND_SIP_INVITE) {

            p += strlen(search_str);
            int i = 0;
            while (p[i] >= 0x20) i++;
            uint8_t save = p[i];
            p[i] = 0;
            len = atoi((const char*)p);
            p[i] = save;

            if (len <= 1 || len > (int)MAX_TCP_PACKET_LEN) { session_pkt_type_found = -1; goto ret; };  /* invalid Length: value */

         /* additional search to handle INVITE formats where non-SDP info lines appear between Content-Length: and actual SDP info, JHB May 2021

            -Content-Length: value only applies to actual SDP info, but in some cases (e.g. siprec) may include additional, non-useful info
            -this makes us dependent on presence of v=0. RFC4566 does say v= is mandatory, and for many years version is zero
         */

            #ifdef FIND_INVITE_DEBUG
            state = 2;
            #endif

            uint8_t* p2;
            strcpy(search_str, "v=0");
            p2 = find_keyword(p, pyld_len - index - (int)(&p[i] - &pkt_buf[pyld_ofs+index]), search_str, false);
            if (!p2) { p2 = find_keyword(p, pyld_len - index - (int)(&p[i] - &pkt_buf[pyld_ofs+index]), "v=1", false); if (p2) strcpy(search_str, "v=1"); }  /* also try v=1 in case SIP guys ever bump version from 0.x to 1.x (unlikely but not impossible) */

            if (!p2) goto ret;  /* v=0 not found */

            #ifdef FIND_INVITE_DEBUG
            state = 3;
            #endif

         /* Session Recording Protocol (SIPREC, RFC 7866) is an open SIP based protocol for call recording, partly based on RFC 7245 (https://datatracker.ietf.org/doc/id/draft-portman-siprec-protocol-01.html) */

            p_siprec = find_keyword(p2, pyld_len - index - (int)(p2 - &pkt_buf[pyld_ofs+index]), "--OSS-unique-boundary-42", true);  /* look for siprec header, JHB Apr 2023 */

            if (p_siprec) {  /* siprec Invite has a different format, with "unique-boundary" marked header and footer, and XML section */

               #ifdef FIND_INVITE_DEBUG
               state = 4;
               #endif

               len = (int)(p_siprec - p2);  /* for siprec, use alternative len calculation -- avoid (i) siprec header intro and padding before v=0 (ii) XML section after end of sdp info (separated by another siprec header, if found), JHB Apr 2023 */
            }

            p_start = p2;  /* start of contents ("v=0") */
         }
         else {  /* SAP/SDP protocol packets are lightweight with no header info (e.g. length:, v=, etc) */

            #ifdef FIND_INVITE_DEBUG
            state = 5;
            #endif

            len = pyld_len - (int)(p - &pkt_buf[pyld_ofs+index]);
            p_start = p;  /* start of contents ("application" keyword) */
         }

         int rem = pyld_len - index - (int)(p_start - &pkt_buf[pyld_ofs+index]);

         #ifdef FIND_INVITE_DEBUG
         printf("\nSIP invite state = %d, len>rem %s, count = %d \n pyld_ofs = %d, pyld_len = %d, index = %d \n len = %d, rem = %d, p_start-&pktbuf[ofs] = %d, save_amount = %d \n", state, len > rem ? "yes, saving partial" : "no, goto more search", count++, pyld_ofs, pyld_len, index, len, rem, (int)(p_start - &pkt_buf[pyld_ofs+index]), save_amount);
         if (count == 3) printf(tmpstr);
         #endif

         if (len > rem) {  /* save partial SIP invite, starting with "Length:" */

            thread_info[thread_index].sip_info_save_len[nStream] = pyld_len - (p_ofs - &pkt_buf[pyld_ofs]);
            thread_info[thread_index].sip_info_save[nStream] = (uint8_t*)malloc(thread_info[thread_index].sip_info_save_len[nStream]);
            memcpy(thread_info[thread_index].sip_info_save[nStream], p_ofs, thread_info[thread_index].sip_info_save_len[nStream]);

            #ifdef FRAGMENT_DEBUG
            char tmpstr[400];
            sprintf(tmpstr, " *** inside fragment save, start of saved data, amount = %d, len = %d, rem = %d, pyld_len = %d, flags = 0x%x, start to \"v0\" = %d, ret val = %d \n", thread_info[thread_index].sip_info_save_len[nStream], len, rem, pyld_len, PktInfo.flags, (int)(p_ofs - &pkt_buf[pyld_ofs]), candidate_session_pkt_type_found);

            PrintPacketBuffer(thread_info[thread_index].sip_info_save[nStream], thread_info[thread_index].sip_info_save_len[nStream], tmpstr, " *** end of saved data \n");
            #endif

         /* tested with openli-voip-example2.pcap and this still happens for encapsulated packets, possibly not being reassembled correctly in PushPackets() in mediaMin.cpp, JHB Apr 2025 */

            session_pkt_type_found =  SESSION_CONTROL_FOUND_SIP_FRAGMENT;  /* return non-zero for fragments, let user app (or PushPackets() in mediaMin.cpp) know, JHB Jun 2024 */
         }
         else {  /* SIP invite or SAP/SDP protocol found, display and/or extract Origin and Media objects from SDP info, add to thread_info[].origins[stream] */

            char szSDP[MAX_TCP_PACKET_LEN];
            memcpy(szSDP, p_start, len);

            uint8_t* p2 = (uint8_t*)memmem(szSDP, len, "sdp", 3);
            if (p2 && *(p2+3) == 0) *(p2+3) = '\n';  /* some SAP packet generators seem to stick a zero after "application/sdp" and before "m=", which I don't see in any spec, but whatever. If so we replace with a new line, JHB Jan 2023 */

            format_sdp_str(szSDP, len);  /* format into null-terminated string. Remove extra blank lines and trailing end-of-lines, if any */

         /* check for duplicate SDP info in continuing fragments. Notes, JHB Jun 2024:

            -seems that SDP info may repeat in consecutive TCP ACK and PSH,ACK sequences. We attempt to weed those out, and possibly other cases we haven't encountered yet. Test with openli-voip-example.pcap
            -if found then we log an INFO message but no parsing and no display of SDP info
            -calculate CRC32 of SDP info and compare with previous SDP info CRC32
            -CRC should have better reliability than checksum for similarity of strings. Checksum is more for detecting differences (i.e. bit errors)
          */

            int crc32_val = crc32(-1, (const unsigned char*)szSDP, len);

            if (fFragmentData && crc32_val == thread_info[thread_index].sip_info_crc32[nStream]) {  /* ignore duplicates: no parsing, no announcement */

               Log_RT(4, "mediaMin INFO: duplicate %s found, pkt# %u, %s dst port = %u, pyld len = %d, flags = 0x%x, len = %d, rem = %d, index = %d \n", (candidate_session_pkt_type_found == SESSION_CONTROL_FOUND_SIP_INVITE) ? "SIP Invite" : "SAP/SDP protocol", thread_info[thread_index].packet_number[nStream], PktInfo.protocol == TCP ? "TCP" : "UDP", thread_info[thread_index].dst_port[nStream], pyld_len, PktInfo.flags, len, rem, index);

               goto update_index;
            }

            thread_info[thread_index].sip_info_crc32[nStream] = crc32_val;  /* update saved CRC32 */

            #ifdef FRAGMENT_DEBUG
            printf("\n *** inside SDP parse, len = %d, rem = %d, pyld_len = %d, flags = 0x%x, ret val = %d. szSDP = \n%s", len, rem, pyld_len, PktInfo.flags, candidate_session_pkt_type_found, szSDP);
            #endif

            session_pkt_type_found = candidate_session_pkt_type_found;

            if (!(uFlags & SESSION_CONTROL_DISABLE_MESSAGE_DISPLAY)) {  /* display/log mediaMin INFO message and SDP info contents */

               Log_RT(4, "mediaMin INFO: %s found, pkt# %u, %s dst port = %u, pyld len = %d, flags = 0x%x, len = %d, rem = %d, index = %d, SDP info content as follows \n%s", (session_pkt_type_found == SESSION_CONTROL_FOUND_SIP_INVITE) ? "SIP Invite" : "SAP/SDP protocol", thread_info[thread_index].packet_number[nStream], PktInfo.protocol == TCP ? "TCP" : "UDP", thread_info[thread_index].dst_port[nStream], pyld_len, PktInfo.flags, len, rem, index, szSDP);

               fSIPInviteFoundMessageDisplayed = true;
            }

            SDPParseInfo(szSDP, (uFlags & SESSION_CONTROL_ADD_ITEM_MASK) ? SDP_PARSE_ADD : SDP_PARSE_NOADD, nStream, thread_index);  /* SDPParseInfo() will show messages if SDP infos are invalid, repeats of existing session IDs, or added to the input stream's SDP database. Note that return value is number of Invite added, if that should be needed, JHB Jan 2023 */

update_index:

            index = (int)(p_start - &pkt_buf[pyld_ofs+index]) + len;

            goto type_check;  /* look for more SDP info contents in this packet */
         }
      }
   }
   else if (uFlags & SESSION_CONTROL_ALL_MESSAGES) {  /* look for other (i.e. non-Invite) SIP message types */

      strcpy(search_str, "");
      int i, num_session_types = sizeof(SIP_Messages)/sizeof(SIP_MESSAGES);

      for (i=0; i<num_session_types; i++) if (find_keyword(&pkt_buf[pyld_ofs], pyld_len, SIP_Messages[i].szTextStr, true) && !find_keyword(&pkt_buf[pyld_ofs], pyld_len, SIP_Messages[i].szTextExclude, true) && SIP_Messages[i].uPortExclude != thread_info[thread_index].dst_port[nStream] && SIP_Messages[i].uPortExclude != thread_info[thread_index].src_port[nStream]) {

      /* implement updated flags to control message parse and display logic with more precision, JHB Jun 2024 */

         if (SIP_Messages[i].val == SESSION_CONTROL_FOUND_SIP_BYE) {
            if (!(uFlags & SESSION_CONTROL_SIP_BYE_MESSAGES)) break;  /* return if Bye message found but not requested, JHB Jun 2024 */
         }
         else if (SIP_Messages[i].val == SESSION_CONTROL_FOUND_SIP_INVITE) {
            if (!(uFlags & SESSION_CONTROL_SIP_INVITE_MESSAGES)) break;  /* return if Invite message found but not requested (we could be here if the SESSION_CONTROL_NO_PARSE flag was given) */
            if (fSIPInviteFoundMessageDisplayed) break;  /* after finding and displaying SDP info, SIP Invite handling does a "goto type_check" and continues to search the payload. We want to avoid displaying "SIP Invite message found" more than once per packet, JHB Jun 2024 */ 
         }
         else if ((uFlags & SESSION_CONTROL_ALL_MESSAGES) != SESSION_CONTROL_ALL_MESSAGES) continue;  /* skip other messages if not requested, but keep searching */

         strcpy(search_str, SIP_Messages[i].szType);
         session_pkt_type_found = SIP_Messages[i].val;

         if (!(uFlags & SESSION_CONTROL_DISABLE_MESSAGE_DISPLAY)) {  /* display/log mediaMin INFO SIP message contents */

            Log_RT(4, "mediaMin INFO: SIP %s message found, pkt# %u, %s dst port = %u, pyld len = %d, index = %d \n", search_str, thread_info[thread_index].packet_number[nStream], PktInfo.protocol == TCP ? "TCP" : "UDP", thread_info[thread_index].dst_port[nStream], pyld_len, index);  /* include packet number, non-fragmented packet dst port, and protocol in message, JHB Jun 2024 */
         }

         break;
      }
   }

ret:
   if (szKeyword && strlen(search_str)) strcpy(szKeyword, search_str);

   return session_pkt_type_found;
}
