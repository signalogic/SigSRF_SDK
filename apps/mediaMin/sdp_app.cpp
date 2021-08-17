/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/sdp_app.cpp

 Copyright (C) Signalogic Inc. 2021

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

 Description

  SDP related source for mediaMin reference application

 Documentation

  ftp://ftp.signalogic.com/documentation/SigSRF

  (as of Oct 2019, most recent doc is ftp://ftp.signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

 Revision History

   Created Apr 2021 JHB, split off from mediaMin.cpp
*/

#include <fstream>
using namespace std;

#include "mediaTest.h"  /* bring in constants needed by mediaMin.h */

#include "pktlib.h"     /* DSGetPacketInfo() */
#include "voplib.h"
#include "diaglib.h"    /* bring in Log_RT() definition */

#include <sdp/sdp.h>    /* SDP API header file */

#include "mediaMin.h"   /* bring in THREAD_INFO typedef */
#include "sdp_app.h"

extern THREAD_INFO thread_info[MAX_MEDIAMIN_THREADS];  /* THREAD_INFO struct defined in mediaMin.h, MAX_MEDIAMIN_THREADS defined in mediaTest.h */

/* SDPAdd

  -add SDP info to thread data, for reference during dynamic session creation
  -SDP info can be from command line .sdp file or SIP invite packet text data. SDP info can contain multiple Media elements, multiple rtpmap attributes
  -SDP info can be added at any time, in any sequence (cmd line .sdp file, if one, is added first)
  -currently duplicate media elements and rtpmap attributes are not filtered out. It's application dependent on whether first or latest matching rtpmap is used
*/

bool SDPAdd(const char* szInvite, unsigned int uFlags, int nInput, int thread_index) {

std::string sdpstr;

   if (uFlags & SDP_ADD_STRING) sdpstr = szInvite;
   else {

   /* invite string NULL, read command line SDP file */

      std::ifstream ifs(szInvite, std::ios::in);  /* note - ifs is an RAII object so ifstream will be closed when function exits */

      if (!ifs.is_open()) {
         Log_RT(2, "mediaMin ERROR: SDPAdd() file %s not found \n", szInvite);
         return false;
      }

      std::string temp_sdpstr( (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>() );  /* ignore the extra (), it's a case of Most Vexing Parse (https://en.wikipedia.org/wiki/Most_vexing_parse) */
      sdpstr = temp_sdpstr;
   }

   #ifdef PRINTSDPDEBUG
   printf("\n sdp file = \n%s \n", sdpstr.c_str());  /* print all lines of sdp file */
   #endif

/* create SDP related items */

   sdp::SDP sdp_session;
   sdp::Reader reader;
//   sdp::Writer writer;
   sdp::Media* media = NULL;
   int node = 0;

// #define PRINTSDPDEBUG  /* turn on for SDP parsing debug */

/* parse input SDP file into an sdp::SDP session */

  reader.parse(sdpstr, &sdp_session);

/* retrieve Media elements ("m=audio, m=video") */

   #ifdef PRINTSDPDEBUG
   sdp_session.print(NULL);
   #endif

   while (sdp_session.find(sdp::SDP_AUDIO, &media, &node)  /* find next audio Media element */
          #if 0  /* uncomment to parse video Media elements */
          || sdp_session.find(sdp::SDP_VIDEO, &media, &node)
          #endif
         ) {

      #ifdef PRINTSDPDEBUG
      printf(" + media found, media_node = %d \n", node);
      #endif

      node++;  /* increment node index, in case there are more Media elements */

      vector<sdp::Attribute*> rtpmaps = {};  /* create an rtpmap vector, init to zero */

      if (int num_rtpmaps = media->find(sdp::SDP_ATTR_RTPMAP, rtpmaps, NULL)) {  /* retrieve rtpmap attributes ("a=rtpmap:") for this Media element. Attribute nodes are children of Media element nodes, so starting node is always zero */

         #ifdef PRINTSDPDEBUG
         printf(" + num rtpmaps %d \n", num_rtpmaps);
         #endif

         for (int i=0; i<num_rtpmaps; i++) {  /* loop through rtpmap attributes */

            sdp::AttributeRTP* rtpmap __attribute__ ((unused)) = (sdp::AttributeRTP*)rtpmaps[i];  /* map RTP attribute onto generic attribute in order to do something useful with it ... */

            #ifdef PRINTSDPDEBUG
            printf("%s rtpmap[%d], pyld type = %d, codec type = %d, sample rate = %d, num chan = %d \n", !i ? "\n" : "", i, rtpmap->pyld_type, rtpmap->codec_type, rtpmap->clock_rate, rtpmap->num_chan);
            #endif
         }
 
      /* save found rtmpaps in thread_info[] for reference in create_dynamic_session() in mediaMin.cpp */

         if (!thread_info[thread_index].num_rtpmaps) thread_info[thread_index].rtpmaps[nInput] = rtpmaps;  /* first SDP info */
         else {
            vector<sdp::Attribute*> th_rtpmaps = thread_info[thread_index].rtpmaps[nInput];  /* append additional SDP info */
            th_rtpmaps.insert( th_rtpmaps.end(), rtpmaps.begin(), rtpmaps.end() );
            thread_info[thread_index].rtpmaps[nInput] = th_rtpmaps;
         }

         thread_info[thread_index].num_rtpmaps[nInput] += num_rtpmaps;  /* increment number of rtpmaps */
      }
   }

   return true;
}


/* add SDP info from cmd line file, if any. Notes:

   -cmd line SDP info applies to all inputs
   -SDP info extracted from SIP invite packets applies only to input receiving the invite packets
   -SDPAdd() adds SDP info text, extracted from either a file or SIP invite packets
*/

void SDPSetup(const char* szSDPFile, int thread_index) {

int i;

   if (szSDPFile && strlen(szSDPFile)) {

      for (i=0; i<thread_info[thread_index].nInPcapFiles; i++) {
         if (SDPAdd(szSDPFile, SDP_ADD_FILE, i, thread_index)) {
            if (i == 0) Log_RT(4, "mediaMin INFO: cmd line SDP file %s processed \n", szSDPFile);
         }
         else {
            Log_RT(2, "mediaMin ERROR: cmd line -s arg, SDP file %s not found \n", szSDPFile);
            break;
         }
      }
   }
}

bool FindSIPInvite(uint8_t* pkt_in_buf, int nInput, int thread_index) {

   int pyld_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDLEN, pkt_in_buf, -1, NULL, NULL);
   int pyld_ofs = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDOFS, pkt_in_buf, -1, NULL, NULL);

   if (thread_info[thread_index].sip_save_len[nInput]) {

      memmove(&pkt_in_buf[pyld_ofs + thread_info[thread_index].sip_save_len[nInput]], &pkt_in_buf[pyld_ofs], pyld_len);
      memcpy(&pkt_in_buf[pyld_ofs], thread_info[thread_index].sip_save[nInput], thread_info[thread_index].sip_save_len[nInput]);
      pyld_len += thread_info[thread_index].sip_save_len[nInput];
      thread_info[thread_index].sip_save_len[nInput] = 0;
      free(thread_info[thread_index].sip_save[nInput]);
   }

   bool fSIPInvite = false;
   int index = 0;

invite_check:

   char search_str[50] = "a=rtpmap";
   uint8_t* p;

// if (index > pyld_len) fprintf(stderr, " ==== index %d > pyld_len %d \n", index, pyld_len);

   if (pyld_len > index && (p = (uint8_t*)memmem(&pkt_in_buf[pyld_ofs+index], pyld_len-index, search_str, strlen(search_str)))) {  /* first find rtpmap, then back up and look for Length: */

      strcpy(search_str, "Length:");

      if ((p = (uint8_t*)memmem(&pkt_in_buf[pyld_ofs+index], (uint16_t)(p - &pkt_in_buf[index]), search_str, strlen(search_str)))) {

         //#define FINDINVITEDEBUG
         #ifdef FINDINVITEDEBUG
         char tmpstr[4096];
         int j;
         static int count = 0;
         memcpy(tmpstr, &pkt_in_buf[pyld_ofs+index], pyld_len-index);
         for (j=0; j<pyld_len-index; j++) if (tmpstr[j] < 32 && tmpstr[j] != 10 && tmpstr[j] != 13) tmpstr[j] = 176;
         tmpstr[pyld_len-index] = 0;
         #endif

         uint8_t* p_ofs = p;

         p += strlen(search_str);
         int i = 0;
         while (p[i] >= 0x20) i++;
         p[i] = 0;
         int len = atoi((const char*)p);
         if (len <= 0 || len > (int)MAX_RTP_PACKET_LEN) goto ret;  /* invalid Length: value */

         #if 0
         while (p[i] < 0x20) i++;  /* skip blank lines, if any */
         int start = i;
         #else

      /* additional search to handle INVITE formats where non-SDP info lines appear between Content-Length: and actual SDP info, JHB May2021

         -Content-Length: value only applies to actual SDP info, so not sure why some senders do this
         -this makes us dependent on presence of v=0. RFC4566 does say v= is mandatory, and for many years version is zero
      */

         uint8_t* p2;
         strcpy(search_str, "v=0");
         p2 = (uint8_t*)memmem(p, pyld_len - index - (int)(&p[i] - &pkt_in_buf[pyld_ofs+index]), search_str, strlen(search_str));
         if (!p2) goto ret;  /* v=0 not found */
         int start = (int)(p2 - p);
         #endif

         #ifdef FINDINVITEDEBUG
         printf("\nSIP invite debug %d\n index = %d \n start = %d \n %s\n", ++count, index, start, tmpstr);
         #endif

         fSIPInvite = true;

         int rem = pyld_len - index - (int)(&p[start] - &pkt_in_buf[pyld_ofs+index]);

         if (len > rem) {  /* save partial SIP invite, starting with "Length:" */

            thread_info[thread_index].sip_save_len[nInput] = pyld_len - (p_ofs - &pkt_in_buf[pyld_ofs]);
            thread_info[thread_index].sip_save[nInput] = (uint8_t*)malloc(thread_info[thread_index].sip_save_len[nInput]);
            memcpy(thread_info[thread_index].sip_save[nInput], p_ofs, thread_info[thread_index].sip_save_len[nInput]);
         }
         else {  /* complete SIP invite found, extract SDP info and add to thread data */

            char szInvite[MAX_RTP_PACKET_LEN];
            memcpy(szInvite, &p[start], len);
            szInvite[len] = 0;

            #if 1
            uint16_t dest_port = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_DST_PORT, pkt_in_buf, -1, NULL, NULL);
            Log_RT(4, "mediaMin INFO: SIP invite found, dst port = %u, pyld len = %d, len = %d, rem = %d, start = %d, index = %d \n%s", dest_port, pyld_len, len, rem, start, index, szInvite);
            #endif

            SDPAdd(szInvite, SDP_ADD_STRING, nInput, thread_index);  /* add SDP info to thread data */

            index = &p[start] - &pkt_in_buf[pyld_ofs+index] + len;
            goto invite_check;  /* look for more SIP invites in this packet */
         }
      }
   }

ret:

   return fSIPInvite;
}
