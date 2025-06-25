/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/port_io.cpp

 Copyright (C) Signalogic Inc. 2020-2025

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  port related source for mediaMin reference application

 Documentation

  https://www.github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediamin

  Older documentation links:
  
    after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

    before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

 Revision History before being moved to separate file
 
   Modified Jan 2023 JHB, filter non-dynamic UDP ports and media port allow list. See NON_DYNAMIC_UDP_PORT_RANGE, UDP_Port_Media_Allow_List[], and FILTER_UDP_PACKETS
   Modified May 2023 JHB, support port list on cmd line (-pN entry), search through uPortList[] in isPortAllowed()
   Modified Jun 2024 JHB, look for additional protocols in isPortAllowed() and isReservedUDP() (GPRS tunnelling, DNS, pichat)
   Modified Jun 2024 JHB, fix problems with SIP port range handling, update isPortAllowed(). Look for PORT_ALLOW_xxx definitions
   Modified Apr 2025 JHB, in isPortAllowed() improvements to console formatting of port-found messages

 Revision History

   Created Jun 2025 JHB, split off from mediaMin.cpp
*/

#include <algorithm>
using namespace std;

#include <stdio.h>
#include <stdarg.h>

#include "mediaTest.h"  /* uPortList[], CMDOPT_MAX_INPUT_LEN, MAX_APP_THREADS, etc */

#include "mediaMin.h"
#include "user_io.h"    /* app_printf() */

/*  as noted in Revision History, code was split from mediaMin.cpp; the following extern references are necessary to retain tight coupling with related source in mediaMin.cpp. There are no multithread or concurrency issues in these references */

extern APP_THREAD_INFO thread_info[];  /* APP_THREAD_INFO struct defined in mediaMin.h */
extern bool fFirstConsoleMediaOutput;

static uint16_t UDP_Port_Media_Allow_List[] = { 1234, 3078, 3079 };  /* add exceptions here for UDP ports that should be allowed for RTP media (and are not expressed by in-stream SDP info). Currently the list has some arbitrary ports found in a few legacy test pcaps used in mediaMin regression test. Port exceptions can also be added at run-time using -p cmd line entry. See usage in isPortAllowed() */

/* local functions */

bool sdp_info_check(uint8_t* pkt_buf, int pkt_len) {

   if (!pkt_buf || pkt_len <= 0) return false;

   char s[] = "Length: ";
   void* p = memmem(pkt_buf, pkt_len, s, strlen(s));
   if (!p) {
      char s[] = "a=rtpmap";
      p = memmem(pkt_buf, pkt_len, s, strlen(s));
   }

   return (p != NULL);
}

/* isPortAllowed() handles non-RTP UDP and TCP ports, notes JHB Jan 2023:

   -looks through list of allowed ports given on cmd line
   -looks through UDP_Port_Media_Allow_List[]
   -looks for media ports discovered via SIP/SDP/SAP protocols
   -looks for some common protocols that can be reported in console output (GTP, NetBIOS, MySQL, etc)
   -formats and displays console protocol messages. Note that for common protocols output is condensed to use the same line for successive messages
   -more protocols can be added in the UDP and TCP switch statements (see protocol definitions in pktlib.h)
*/

int isPortAllowed(uint16_t port, uint8_t port_type, uint8_t* pkt_buf, int pkt_len, uint8_t uProtocol, int nStream, uint64_t cur_time, int thread_index) {

char portstr[50] = "", countstr[50] = "";
int i = 0;

int nFound = 0, ret_val = PORT_ALLOW_UNKNOWN;
bool fSDPInfoFound = false;
static unsigned num_GPRS = 0;

static int nLastFound[MAX_APP_THREADS][MAX_STREAMS_THREAD] = {{ 0 }};
static unsigned int uCursorPos[MAX_APP_THREADS][MAX_STREAMS_THREAD] = {{ 0 }};

   if (uProtocol == UDP) {

   /* check cmd line -pN entries, if any, JHB May 2023 */

      while (uPortList[i] && i < MAX_STREAMS) if (port == uPortList[i++]) return PORT_ALLOW_ON_MEDIA_ALLOW_LIST;  /* uPortList[] is defined in cmd_line_interface.c, PORT_ALLOW_xxx flags are defined in mediaMin.h */

   /* check source code defined list of allowed ports */

      for (i=0; i<(int)sizeof(UDP_Port_Media_Allow_List)/(int)sizeof(UDP_Port_Media_Allow_List[0]); i++) if (port == UDP_Port_Media_Allow_List[i]) return PORT_ALLOW_ON_MEDIA_ALLOW_LIST;

   /* check SDP info database for discovered media ports, JHB Jun 2024 */

      for (i=0; i<thread_info[thread_index].num_media_descriptions[nStream]; i++) if (port == ((sdp::Media*)(thread_info[thread_index].media_descriptions[nStream][i]))->port) return PORT_ALLOW_SDP_MEDIA_DISCOVERED;  /* media_descriptions[] are parsed and processed in SDPParseInfo() in sdp_app.cpp */

   /* misc protocols we can report in console output. Unrecognized UDP ports are displayed as "ignoring UDP port ..." by PushPackets() */

      switch (port) {  /* see additional comments for port definitions in pktlib.h */

         case DNS_PORT:
            strcpy(portstr, "DNS");
            nFound = 1;
            break;

         case DHCPv6_PORT:
            strcpy(portstr, "DHCPv6");
            nFound = 2;
            break;

         case NetBIOS_PORT:
         case NetBIOS_PORT+1:
            strcpy(portstr, "NetBIOS");
            nFound = 3;
            break;

         case QUIC_PORT:  /* added JHB Feb 2025 */
            strcpy(portstr, "QUIC");
            nFound = 4;
            break;

         case GTP_PORT:  /* GPRS tunneling protocol */
            if (num_GPRS++ < 16 || (num_GPRS < 512 && num_GPRS % 32 == 0) || num_GPRS % 512 == 0) { sprintf(portstr, "GPRS Tunneling"); sprintf(countstr, " (%d)", num_GPRS); }
            nFound = 5;
            fSDPInfoFound = sdp_info_check(pkt_buf, pkt_len);
            break;

         case PICHAT_PORT:
            strcpy(portstr, "pichat");
            nFound = 6;
            break;

         default:
            break;
      }
   }
   else if (uProtocol == TCP) {  /* add TCP protocol reporting, JHB Apr 2025 */
   
      switch (port) {

         case MYSQL_PORT:
            strcpy(portstr, "MySQL");
            nFound = 100;
            break;

         default:
            break;
      }
   }

/* handle console messages for non-RTP, non-SIP/SDP/SAP protocols */

   if (nFound) {

      if ((!(Mode & DISABLE_PORT_IGNORE_MESSAGES) || !fFirstConsoleMediaOutput) && strlen(portstr)) {

         unsigned int uFlags = APP_PRINTF_PRINT_ONLY;

         if (!uCursorPos[thread_index][nStream] || uLineCursorPos != uCursorPos[thread_index][nStream] || nFound != nLastFound[thread_index][nStream]) {

            #if 0  /* debug helper */
            // if (thread_info[thread_index].packet_number[nStream] == xxxx)
            printf("\n *** nFound = %d, nLastFound[%d] = %d, uCursorPos[%d] = %d, uLineCursorPos = %u, isCursorMidLine = %u \n", nFound, nStream, nLastFound[thread_index][nStream], nStream, uCursorPos[thread_index][nStream], uLineCursorPos, isCursorMidLine);
            #endif

            app_printf(uFlags | APP_PRINTF_SAME_LINE, cur_time, thread_index, "%s%s packet found%s, %s %s port = %u, nStream = %d, pkt# %u", uLineCursorPos && nLastFound[thread_index][nStream] > 0 && nFound != nLastFound[thread_index][nStream] ? "\n" : "", portstr, countstr, uProtocol == UDP ? "UDP" : "TCP", port_type == 0 ? "dst" : "src", port, nStream, thread_info[thread_index].packet_number[nStream]);
         }
         else {  /* for consecutive port found messages append packet numbers to existing line display. This reduces and consolidates console output, which tends to make debugging user cases easier, JHB Apr 2025 */

            app_printf(uFlags | APP_PRINTF_SAME_LINE, cur_time, thread_index, ", %u", thread_info[thread_index].packet_number[nStream]);
         }

         uCursorPos[thread_index][nStream] = uLineCursorPos;
      }

      if (fSDPInfoFound) ret_val = PORT_ALLOW_SDP_INFO;
      else ret_val = PORT_ALLOW_KNOWN;
   }

   if (nFound && strlen(portstr)) nLastFound[thread_index][nStream] = nFound;  /* update nLastFound[][] if a port was found and if something was actually displayed (portstr not empty) */

   return ret_val;
}
