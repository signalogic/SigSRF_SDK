/*
  $Header: /root/Signalogic/shared_include/media.h
 
  Purpose:
 
    Media transcoding struct and constant definitions

  Description
  
    Shared header file between CPU and coCPU

  Copyright (C) Signalogic Inc. 2015-2017
 
  Revision History:
 
    Created,  Oct15, JHB, copied from vdi.h
  
*/

#ifndef CIM_MEDIA_H
#define CIM_MEDIA_H

#include "video.h"
#include "streamlib.h"

#ifdef __cplusplus
  extern "C" {
#endif

#ifndef CMDOPT_MAX_INPUT_LEN
  #define CMDOPT_MAX_INPUT_LEN  256
#endif

typedef struct {

   unsigned int     numStreams;

#ifdef _CIM_BUILD_
   VIDEOPARAMS      Video;                                  /* currently, only some parts of VIDEOPARAMS and STREAMINGPARAM are in use for media test/demo apps. JHB, Oct 2015 */
#else
   VIDEOPARAMS      Media;
#endif
   STREAMINGPARAMS  Streaming;

/* media specific params */

   char             configFilename[CMDOPT_MAX_INPUT_LEN];   /* -C on command line specifies config files */

   unsigned int     samplingRate;  /* in Hz */

   uint64_t         inputFilesize;  /* used in codec unit-test, JHB Feb2017 */
   uint64_t         outputFilesize;

} MEDIAPARAMS;  /* struct params for high capacity media transcoding */

typedef MEDIAPARAMS* PMEDIAPARAMS;

/* macros for MEDIAPARAMS struct */

#define MediaNumStreams(a) a[0].numStreams

#ifdef __cplusplus
}
#endif

#endif /* CIM_MEDIA_H */
