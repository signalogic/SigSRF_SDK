/*
  $Header: /root/Signalogic_YYYYvN/shared_include/ia.h
 
  Purpose:
 
    Image analytics struct and constant definitions

  Description
  
    Shared header file between CPU and coCPU

  Copyright (C) Signalogic Inc. 2015-2017
 
  Revision History:
 
    Created,  6May15, JHB, carved out from cimlib.h
    Modified, Jun 2017, JHB, moved definition for ImageStream() and IA_Stream_Demo() here from vdi.h
*/

#ifndef CIM_IA_H
#define CIM_IA_H


#include "video.h"
#include "streamlib.h"

#ifdef __cplusplus
  extern "C" {
#endif


typedef struct {

  unsigned int     numStreams;
  VIDEOPARAMS      Video;
  STREAMINGPARAMS  Streaming;
  
  unsigned int     uTILibsConfig;
  
} IAPARAMS;  /* image analytics params */

typedef IAPARAMS* PIAPARAMS;

#define IaNumStreams(a) a[0].numStreams

/* constants for uTILibsConfig */

#define IA_OPENCV_USE_TI_VLIB              1
#define IA_OPENCV_USE_FAST_FUNCS           2  /* includes optimized functions, fast YUV conversion */
#define IA_OPENCV_USE_PYRAMIDS_FOR_RESIZE  4
#define IA_OPENCV_USE_INTRINSICS           8
#define IA_OPENCV_USE_TI_IMGLIB            0x10

#define IA_PROCLEVEL_STATS                 0x00100000
#define IA_PROCLEVEL_SHAPE                 0x00200000
#define IA_PROCLEVEL_TEMPLATE              0x00400000

#define IA_VISIBLE_DEBUG_LEVEL1            0x01000000  /* enable visible info for debug purposes */
#define IA_VISIBLE_DEBUG_LEVEL2            0x02000000

#ifdef _TI66X

int ia_prolog(unsigned char*, unsigned char*);
int ia_epilog();

#endif

#ifndef _CIMF
//  int ImageStream();
  void* ImageStream(void*);  /* changed for Gcc compatibility in x86 + coCPU demos, JHB Jun2017 */
  int IA_Stream_Demo();
#endif

#ifdef __cplusplus
}
#endif


#endif /* CIM_IA_H */
