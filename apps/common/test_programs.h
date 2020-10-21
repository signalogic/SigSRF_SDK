/*
  $Header: /root/Signalogic_YYYYvN/DirectCore/apps/common/test_programs.h
 
  Purpose / Description
 
    Application misc items, including timer callback function

  Copyright (C) Signalogic Inc. 2005-2015
 
  Revision History:
 
    Created, 2005, JHB
    Revised, May2015, JHB.  Added API interface in addition to global var
  
*/

#ifndef _TEST_PROGRAMS_H_
#define _TEST_PROGRAMS_H_

#ifndef _CIM
  #include "alias.h"
  #include "cmdLineOpt.h"
#endif

#include "userInfo.h"

/* mem address value specific to C66x cards */

#define C6678_LAST_L2MEM_ADDR  0x0087fffc

#define EXIT_DONE 1
#define EXIT_QUIT 2
#define EXIT_SAVE 3

/* macros for handling typecasting for different application property structs */

#if defined(_IA_) || defined(_CIM)

#include "cimlib.h"
#include "vdi.h"
#include "ia.h"

#define structype(a, b, c) ((a & CIM_GCL_VDI) ? ((PVDIPARAMS)b)c : ((PIAPARAMS)b)c)
#define structype_rhs(a, b, c, d) ((a & CIM_GCL_VDI) ? ((PVDIPARAMS)b)c d : ((PIAPARAMS)b)c d)

#endif
  
#ifdef __cplusplus
extern "C" {
#endif

  extern uint32_t globalDebugLevel;  /* can be used to set debug level inside DirectCore libs, JHB Feb 2016 */

  #ifndef _CIM
    extern void targetSetParams(UserInterface *userIfs);
  #endif

  extern volatile bool fTimerCallbackOccurred;

  void TargetTimerTestHdl(int signum);

  void setTimerInterval(time_t _sec, time_t _usec);

  bool IsTimerEventReady();

  void ResetTimerEvent();

  bool IsTimerEnabled();

#ifdef __cplusplus
}
#endif

#endif  /* _TEST_PROGRAMS_H_ */
