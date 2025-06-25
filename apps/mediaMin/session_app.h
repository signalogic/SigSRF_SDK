/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/session_app.h

 Copyright (C) Signalogic Inc. 2021-2025

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  Header file for session management related source for mediaMin reference application

 Documentation

  https://github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediamin

  Older documentation links:
  
    after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

    before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

 Revision History

   Created Apr 2021 JHB, split off from mediaMin.cpp
   Modified May 2023 JHB, changed SetIntervalTiming() to SetSessionTiming() and StaticSessionsCreate() to CreateStaticSessions()
   Modified Mar 2025 JHB, add cur_time param to CreateStaticSessions()
   Modified Jun 2025 JHB, add CheckConfigFile()
*/
#ifndef _SESSION_APP_H_
#define _SESSION_APP_H_

#include "shared_include/session.h"    /* session management structs and definitions */

/* functions in session_app.cpp */

int ReadSessionConfig(SESSION_DATA session_data[], int thread_index);
int ReadCodecConfig(codec_test_params_t* codec_test_params, int thread_index);
int CreateStaticSessions(HSESSION hSessions[], SESSION_DATA session_data[], int nSessionsConfigured, uint64_t cur_time, int thread_index);
void SetSessionTiming(SESSION_DATA session_data[]);
unsigned int GetSessionFlags();
int CheckConfigFile(char* config_file, int thread_index);

#endif  /*  _SESSION_APP_H_ */
