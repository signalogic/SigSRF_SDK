/*
  $Header: /root/Signalogic_YYYYvN/shared_include/cim.h
 
  Purpose:
 
    CIM code generation and target CPU core structs and constant definitions

  Description
  
    Shared header file between host and target

  Copyright (C) Signalogic Inc. 2011-2015
 
  Revision History:
 
    Created,  6Jul15, JHB, moved to shared header file status, was target side only
  
*/

#ifndef CIM_CORE_H
#define CIM_CORE_H

#ifdef _TI66X_ACCEL
  #include "c66x_accel.h"
#endif

#ifdef _TI66X
// #include <ti/csl/csl.h>
 #include <ti/csl/csl_cache.h>
#endif

//#include <limits.h>

#define MAX_TASKASSIGNMENT_CORELISTS  16

/* define cache_round macro for CPUs other than c66x */

#ifndef CACHE_ROUND_TO_LINESIZE
  #define CACHE_ROUND_TO_LINESIZE(a, b, c) b  /* for non-c66x CPUs, no rounding currently being done */
#endif

#ifdef _TI66X

/* CIM process complete flag */

extern volatile unsigned int cim_barrier_func;

void cim_cleanup();

void cim_start();

/* debug vars */

extern volatile int numcounter;

#endif

typedef struct {

  uint8_t taskAssignmentCoreLists[MAX_TASKASSIGNMENT_CORELISTS];  /* define core core task assignments per CPU (note uint8_t assumes up to 8 cores per CPU for now) */

} CIMINFO;


/* Resource Manager info */

#define CORE_USAGE_MAX_CORES       8
#define CORE_USAGE_STATS_ADDR      0x0c3fff00  /* core usage reserved location in MCSM.  Note this area is reserved as *outside* the C code environment */
#define CORE_USAGE_STATS_NUMBYTES  256

typedef struct {  /* core usage stats struct */

   unsigned short int nAvgUsage;
   unsigned short int nPeakUsage;
   unsigned char Reserved[28];
   
} core_usage_stats_t;

typedef struct {

  core_usage_stats_t chip[CORE_USAGE_MAX_CORES];

} core_usage_stats_chip_t;


/* indexes for task assignment core lists.  These are similar to "teams" in OpenMP */

#define HOST_INPUT_CORES           0
#define HOST_OUTPUT_CORES          1
#define PROCESSING_GRP0_CORES      2
#define PROCESSING_GRPN1_CORES     (MAX_TASKASSIGNMENT_CORELISTS-1)


#endif /* CIM_CORE_H */
