/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/session_app.h

 Copyright (C) Signalogic Inc. 2021

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

 Description

  Header file for session management related source for mediaMin reference application

 Documentation

  ftp://ftp.signalogic.com/documentation/SigSRF

  (as of Oct 2019, most recent doc is ftp://ftp.signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

 Revision History

   Created Apr 2021 JHB, split off from mediaMin.cpp
*/
#ifndef _SESSION_APP_H_
#define _SESSION_APP_H_

#include "shared_include/session.h"    /* session management structs and definitions */

/* functions in session_app.cpp */

int ReadSessionConfig(SESSION_DATA session_data[], int thread_index);
int StaticSessionCreate(HSESSION hSessions[], SESSION_DATA session_data[], int nSessionsConfigured, int thread_index);
void SetIntervalTiming(SESSION_DATA session_data[]);
unsigned int GetSessionFlags();

#endif