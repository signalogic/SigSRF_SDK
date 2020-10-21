/*
  $Header: /root/Signalogic_YYYYvN/shared_include/vdi.h
 
  Purpose:
 
    VDI struct and constant definitions

  Description
  
    Shared header file between CPU and coCPU

  Copyright (C) Signalogic Inc. 2015-2017
 
  Revision History:
 
    Created,  6May15, JHB, carved out from cimlib.h
  
*/

#ifndef CIM_VDI_H
#define CIM_VDI_H

#include "video.h"
#include "ia.h"
#include "streamlib.h"

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct {

  unsigned int     numStreams;
  VIDEOPARAMS      Video;
  STREAMINGPARAMS  Streaming;
  
} VDIPARAMS;  /* VDI (Virtual Desktop Infrastructure) params */

typedef VDIPARAMS* PVDIPARAMS;

/* macros for VDIPARAMS struct */

#define VdiNumStreams(a) a[0].numStreams

#ifdef _TI66X

  int H264Encode(unsigned int uMode, unsigned char* inputBuf, unsigned char* outputBuf, VDIPARAMS VDIParams[]);
  int TrackObject(unsigned int uMode, unsigned char* inputBuf, unsigned char* outputBuf, IAPARAMS IAParams[]);

#endif

#ifndef _CIMF
  int VideoStream();  /* cStandard build test functions for continuous streaming and image analytics */
#endif

#ifdef __cplusplus
}
#endif

#endif /* CIM_VDI_H */
