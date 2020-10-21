/*
  debug_rt.h

  Header file that contains debug structs and function prototypes

  Copyright © Signalogic, 2017

  Revision History:

    Created Jan 2017, CJ (pulled from debug.h.  debug_rt.h can be used with x86 only, coCPU, or mixed projects)
*/

#ifndef _DEBUG_RT_H_
#define _DEBUG_RT_H_

/* Debug/logging function prototypes */

#ifdef _TI66X
  #define LOG_LEVEL_MASK        0x0f
  #define LOG_ONE_TIME          0x10
  #define LOG_ONE_TIME_NO_HASH  0x20
  #define LOG_RUNNING           0x40
  #define MAX_ONE_TIME_LOGS     512
#endif

#if defined(_EVS_) && !defined(ENABLE_LOGGING)
  #ifdef _TI66X
    #undef Dsp_Log
  #endif
#endif

#ifdef _TI66X
  unsigned int Dsp_Log(uint16_t, char *, ...);
  #define Log_RT Dsp_Log
#endif

#if defined(_EVS_) && !defined(ENABLE_LOGGING)  /* if building EVS codes and ENABLE_LOGGING not defined, then cause log statements to be stripped out of code */
  #ifdef _TI66X
    #define Dsp_Log if (0) Dsp_Log
    #undef Log_RT
    #define Log_RT if (0) Dsp_Log
  #endif
#endif

#endif /* end of header file */

