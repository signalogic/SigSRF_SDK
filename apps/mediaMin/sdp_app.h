/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/sdp_app.h

 Copyright (C) Signalogic Inc. 2021

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

 Description

  Header file for SDP related source for mediaMin reference application

 Documentation

  ftp://ftp.signalogic.com/documentation/SigSRF

  (as of Oct 2019, most recent doc is ftp://ftp.signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

 Revision History

   Created Apr 2021 JHB, split off from mediaMin.cpp
*/

#ifndef _SDP_APP_H_
#define _SDP_APP_H_

/* uFlags options for SDPAdd() */

#define SDP_ADD_FILE    1
#define SDP_ADD_STRING  2

/* functions in sdp_app.cpp */

bool SDPAdd(const char* szInvite, unsigned int uFlags, int nInput, int thread_index);
void SDPSetup(const char* szSDPFile, int thread_index);
bool FindSIPInvite(uint8_t* pkt_in_buf, int nInput, int thread_index);

#endif
