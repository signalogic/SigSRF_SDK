/*
  $Header: /root/Signalogic/DirectCore/lib/diaglib/diaglib_priv.h
 
  Description: private functions called by diaglib.c and lib_logging.c
 
  Projects: SigSRF, SigMRF, DirectCore
 
  Copyright Signalogic Inc. 2017-2023

  Revision History:
  
   Created Sep 2017 CJ
   Modified Oct 2018 JHB, moved DNUM definition to shared_include/std_rtaf.h
   Modified Jan 2023 JHB, remove reference to set_api_status()
   Modified Jan 2023 JHB, add items to support DSConfigPktLogging() API
*/
 
#ifndef _DIAGLIB_PRIV_H_
#define _DIAGLIB_PRIV_H_

#if 1  /* moved here from pktlib_priv.h, JHB Sep2017 */
//#define DNUM                  0  /* moved to std_rtaf.h */
#define Dsp_Log(a, ...)       Log_RT(a, __VA_ARGS__)
#endif

#if 0  /* removed, JHB Jan 2023 */
/* update error status */
void set_api_status(int status_code, unsigned int uFlags);
#endif

#define MAXTHREADS 64

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

   pthread_t     ThreadId;
   unsigned int  status_code;
   int           uFlags;  /* can be set by DSConfigPacketLogging() */

} LOGINFO;

int GetThreadIndex(bool);

#ifdef __cplusplus
}
#endif

#endif  /* _DIAGLIB_PRIV_H_ */
