/*
 $Header: /root/Signalogic/apps/mediaTest/mediaMin/user_io.h

 Copyright (C) Signalogic Inc. 2021-2025

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  Header file for user I/O related source for mediaMin reference application

 Documentation

  https://www.github.com/signalogic/SigSRF_SDK/tree/master/mediaTest_readme.md#user-content-mediamin

  Older documentation links:
  
    after Oct 2019: https://signalogic.com/documentation/SigSRF/SigSRF_Software_Documentation_R1-8.pdf)

    before Oct 2019: ftp://ftp.signalogic.com/documentation/SigSRF

 Revision History

   Created Apr 2021 JHB, split off from mediaMin.cpp
   Modified Jan 2023 JHB, add APP_PRINTF_PRINT_ONLY flag
   Modified Jun 2024 JHB, add PrintPacketBuffer()
   Modified Mar 2025 JHB, add cur_time param to app_printf() and UpdateCounters()
   Modified Apr 2025 JHB, add APP_PRINTF_SAME_LINE_PRESERVE flag
*/

#ifndef _USER_IO_H_
#define _USER_IO_H_

#define APP_PRINTF_PRINT_ONLY              0           /* doesn't do anything, but can make it clear no log entry will happen */
#define APP_PRINTF_SAME_LINE               1           /* app_printf() flags */
#define APP_PRINTF_NEW_LINE                2
#define APP_PRINTF_THREAD_INDEX_SUFFIX     4
#define APP_PRINTF_EVENT_LOG               8
#define APP_PRINTF_EVENT_LOG_NO_TIMESTAMP  0x10
#define APP_PRINTF_EVENT_LOG_STRIP_LFs     0x20        /* strip intermediate (screen formatting) LFs */
#define APP_PRINTF_SAME_LINE_PRESERVE      0x40        /* */

#include "shared_include/session.h"    /* session management structs and definitions */
#include "shared_include/config.h"     /* configuration structs and definitions */

/* functions in user_io.cpp */

void UpdateCounters(uint64_t cur_time, int thread_index);
bool ProcessKeys(HSESSION hSessions[], DEBUG_CONFIG* pDebugConfig, uint64_t cur_time, int thread_index);  /* process keyboard command input */
void app_printf(unsigned int uFlags, uint64_t cur_time, int thread_index, const char* fmt, ...);
void PrintPacketBuffer(uint8_t* buf, int len, const char*, const char*);
void PrintSIPInviteFragments(uint8_t* pkt_buf, PKTINFO* PktInfo, int pkt_len);

#endif
