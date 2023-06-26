/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/sdp_app.h

 Copyright (C) Signalogic Inc. 2021-2023

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  Header file for SDP related parsing and object management, SIP Invite and other SIP message processing

 Purpose

  support for mediaMin reference application

 Documentation

  ftp://ftp.signalogic.com/documentation/SigSRF

  (as of Oct 2019, most recent doc is ftp://ftp.signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

 Revision History

   Created Apr 2021 JHB, split off from mediaMin.cpp
   Modified Jan 2023 JHB, change SDPAdd() to SDPParseInfo() and expand its functionality to handle Origin objects
   Modified Jan 2023 JHB, change FindSIPInvite() to ProcessSessionControl() add uFlags and szKeyword params
   Modified Mar 2023 JHB, add SESSION_CONTROL_NO_PARSE uFlag
*/

#ifndef _SDP_APP_H_
#define _SDP_APP_H_

/* uFlags options for SDPParseInfo() */

#define SDP_PARSE_NOADD                           0      /* use this or no flag to parse only, without adding to thread_info[].xxx[stream] SDP database */
#define SDP_PARSE_ADD                             1 
#define SDP_PARSE_IGNORE_ORIGINS                  2

/* uFlags options for ProcessSessionControl() */

#define SESSION_CONTROL_SHOW_SIP_INVITE_MESSAGES  1
#define SESSION_CONTROL_SHOW_ALL_MESSAGES         0x0f

#define SESSION_CONTROL_ADD_SIP_INVITE_SDP_INFO   0x100
#define SESSION_CONTROL_ADD_SAP_SDP_INFO          0x200

#define SESSION_CONTROL_ADD_ITEM_MASK             0xf00

#define SESSION_CONTROL_NO_PARSE                  0x1000  /* allow user apps to show messages found but not parse, JHB Mar 2023 */

#define SESSION_CONTROL_FOUND_SIP_INVITE                1
#define SESSION_CONTROL_FOUND_SIP_TRYING                2
#define SESSION_CONTROL_FOUND_SIP_RINGING               3
#define SESSION_CONTROL_FOUND_SIP_PROGRESS              4
#define SESSION_CONTROL_FOUND_SIP_ACK                   5
#define SESSION_CONTROL_FOUND_SIP_PROV_ACK              6  /* PRACK (provisional ACK) */
#define SESSION_CONTROL_FOUND_SIP_OK                    7
#define SESSION_CONTROL_FOUND_SIP_BYE                   8
#define SESSION_CONTROL_FOUND_SIP_INFO_REQUEST          9
#define SESSION_CONTROL_FOUND_SIP_PLAYING_ANNOUNCEMENT  10
#define SESSION_CONTROL_FOUND_SAP_SDP                   100

#ifdef __cplusplus
  extern "C" {
#endif

/* SIP message struct used in ProcessSessionControl() in sdp_app.cpp */

typedef struct {

  char szTextStr[40];  /* string pattern to search for in packet payload */
  char szType[30];     /* SIP message description that will be displayed/logged by ProcessSessionControl() */
  int  val;            /* SESSION_CONTROL_FOUND_xxx values (defined above) */

} SIP_MESSAGES;

/* functions in sdp_app.cpp */

int SDPSetup(const char* szSDPFile, int thread_index);
int SDPParseInfo(std::string szSDP, unsigned int uFlags, int nInput, int thread_index);
int ProcessSessionControl(uint8_t* pkt_in_buf, unsigned int uFlags, int nInput, int thread_index, char* szKeyword);

#ifdef __cplusplus
  }
#endif

#endif  /* _SDP_APP_H_ */
