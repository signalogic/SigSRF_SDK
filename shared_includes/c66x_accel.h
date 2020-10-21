/*

 c66x accelerator support
 
   -accelerate ffmpeg, OpenCV, etc with TI multicore CPUs
   -allow streaming on the accelerator card
   -support ffmpeg at HPC levels of performance inside VMs
   -_TI66X_ACCEL enables c66x accelerator support
     
 Copyright (c) 2014-2015  Signalogic, Inc.

*/

#ifndef _C66X_ACCEL_H
#define _C66X_ACCEL_H

#define MAXCPUSPERCARD                      8  /* given for C6678, these may change in C7000 processor family */
#define MAXCORESPERCPU                      8
#define MAXCORESPERCARD                     (MAXCPUSPERCARD*MAXCORESPERCPU)
#define MAXCARDS                            8

/* attributes used for accelerated ffmpeg and opencv support */

#define C66X_ACCEL_ENABLE_ENCODE_H264       1
#define C66X_ACCEL_ENABLE_DECODE_H264       2
#define C66X_ACCEL_ENABLE_ENCODE_VPX        4
#define C66X_ACCEL_ENABLE_DECODE_VPX        8
#define C66X_ACCEL_ENABLE_STREAMING_RTP     0x10
#define C66X_ACCEL_ENABLE_STREAMING_MPEGTS  0x20

#define C66X_ACCEL_HOSTFS_MODE              1
#define C66X_ACCEL_HOSTIO_MODE              2
#define C66X_ACCEL_VIRTFS_MODE              5
#define C66X_ACCEL_VIRTIO_MODE              6

#define C66X_ACCEL_GUI_FOREGROUND           0x100

#include "vdi.h"

#endif /* _C66X_ACCEL_H */
