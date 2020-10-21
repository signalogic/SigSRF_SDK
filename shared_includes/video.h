/*
  $Header: /root/Signalogic/shared_include/video.h
 
  Purpose:
 
    Video struct and constant definitions

  Description
  
    Shared header file between host and target

  Copyright (C) Signalogic Inc. 2015
 
  Revision History:
 
    Created,  6May15, JHB, carved out from cimlib.h
    MOdified, 23Jul15, JHB, additional video configuration options to support ffmpeg
  
*/

#ifndef CIM_VIDEO_H
#define CIM_VIDEO_H

#ifdef __cplusplus
  extern "C" {
#endif

#ifndef CMDOPT_MAX_INPUT_LEN
  #define CMDOPT_MAX_INPUT_LEN  256
#endif

#define H264_HIGH_PROFILE       0  /* default */
#define H264_MAIN_PROFILE       1
#define H264_BASELINE_PROFILE   2

#define BITRATE_CBR             0  /* default */
#define BITRATE_VBR             1

#define SCANTYPE_PROGRESSIVE    0  /* default */
#define SCANTYPE_INTERLACED     1

typedef struct {

  char          inputFilename[CMDOPT_MAX_INPUT_LEN];   /* -i command line parameter specifies input files in raw format (.yuv).  To-do:  support RGB file formats */
  char          outputFilename[CMDOPT_MAX_INPUT_LEN];  /* -o command line parameter specifies output files in .yuv, .h264, and other formats */

  unsigned int  profile;
  unsigned int  width;
  unsigned int  height;
  unsigned int  frameRate;
  unsigned int  framesToEncode;
  unsigned int  bitrateConfig;     /* bitrate configuration, for example constant vs. variable bitrate */
  unsigned int  qpValues;          /* qp values */
  unsigned int  interFrameConfig;  /* key (IDR) frame rate configuration, other frame refresh options */

  unsigned int  ddrInputBase;      /* ddrInputBase and ddrOutputBase are read from C66x software in cimRunHardware().  Defaults are set in cimGetGmdLine() */
  unsigned int  ddrOutputBase;

} VIDEOPARAMS;  /* video params */

typedef VIDEOPARAMS* PVIDEOPARAMS;

#ifdef _TI66X

int vid_encode_prolog(unsigned char*, unsigned char*);
int vid_encode_epilog();

#endif

#ifdef __cplusplus
}
#endif

#endif /* CIM_VIDEO_H */
