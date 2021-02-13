/*
  $Header: /root/Signalogic/DirectCore/lib/diaglib/diaglib_priv.h
 
  Description: Diagnostic library, private functions callable by other libraries
 
  Projects: SigSRF, SigMRF, DirectCore
 
  Copyright Signalogic Inc. 2017-2018

  Revision History:
  
   Created Sep 2017 CJ
   Modified Oct 2018 JHB, moved DNUM definition to shared_include/std_rtaf.h
*/
 
#ifndef _DIAGLIB_PRIV_H_
#define _DIAGLIB_PRIV_H_

#if 1  /* moved here from pktlib_priv.h, JHB Sep2017 */
//#define DNUM                  0  /* moved to std_rtaf.h */
#define Dsp_Log(a, ...)       Log_RT(a, __VA_ARGS__)
#endif

/* update error status */
void set_api_status(int status_code, unsigned int uFlags);

#endif

