/*
  $Header: /root/Signalogic_YYYYvN/shared_include/fft.h
 
  Purpose:
 
    FFT struct and constant definitions

  Description
  
    Shared header file between host and target

  Copyright (C) Signalogic Inc. 2015
 
  Revision History:
 
    Created,  6May15, JHB, carved out from cimlib.h
  
*/

#ifndef CIM_FFT_H
#define CIM_FFT_H


#ifdef __cplusplus
  extern "C" {
#endif


typedef struct {

  unsigned int        MOrder;
  unsigned int        NOrder;
  
  unsigned int        AlgorithmFlag;
  
} FFTPARAMS;  /* FFT params */

typedef FFTPARAMS* PFFTPARAMS;


#ifdef __cplusplus
}
#endif


#endif /* CIM_FFT_H */
