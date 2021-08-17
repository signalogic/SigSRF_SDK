/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/user_io.h

 Copyright (C) Signalogic Inc. 2021

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

 Description

  Header file for user I/O related source for mediaMin reference application

 Documentation

  ftp://ftp.signalogic.com/documentation/SigSRF

  (as of Oct 2019, most recent doc is ftp://ftp.signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

 Revision History

   Created Apr 2021 JHB, split off from mediaMin.cpp
*/

#ifndef _USER_IO_H_
#define _USER_IO_H_

#define APP_PRINTF_SAMELINE                1           /* app_printf() flags */
#define APP_PRINTF_NEWLINE                 2
#define APP_PRINTF_THREAD_INDEX_SUFFIX     4
#define APP_PRINTF_EVENT_LOG               8
#define APP_PRINTF_EVENT_LOG_NO_TIMESTAMP  0x10
#define APP_PRINTF_EVENT_LOG_STRIP_LFs     0x20        /* strip intermediate (screen formatting) LFs */

#include "shared_include/session.h"    /* session management structs and definitions */
#include "shared_include/config.h"     /* configuration structs and definitions */

/* functions in user_io.cpp */

void UpdateCounters(uint64_t cur_time, int thread_index);
bool ProcessKeys(HSESSION hSessions[], uint64_t, DEBUG_CONFIG*, int);  /* process keyboard command input */
void app_printf(unsigned int uFlags, int thread_index, const char* fmt, ...);

#endif

