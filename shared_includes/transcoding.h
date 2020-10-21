/*
  transcoding.h

  Header file for:

    -mailbox interface between control plane cores and coCPU cores
    -buffer interface between data plane cores and coCPU cores

  Copyright (C) Signalogic and Mavenir Systems, 2013
  Copyright (C) Signalogic, 2014-2019

  Revision History

    Created Sep 2013 Liem
    Modified 2015-2016 Signalogic, added support for ADV 8901
    Modified May 2017 JHB, added MAX_SESSIONS_PER_CORE
    Modified Nov 2018 JHB, move definition of MAX_SESSIONS here, remove from lib_priv.h, any other location
    Modified Feb 2019 JHB, move primary definition of NCORECHAN here
    Modified Mar 2019 JHB, add reference to DEMOBUILD, which is defined in pktlib, voplib, and/or hwlib Makefiles for demo build version
    Modified Sep 2019 JHB, define NIPCHAN = NCORECHAN for x86 builds
*/

#ifndef TRANSCODING_H_
#define TRANSCODING_H_

#include <stdint.h>

/* Constant definitions */
#ifdef _ADV8901_
  #define NUM_OF_DSPS          20
#else
  #define NUM_OF_DSPS          4
#endif

#define NUM_CORES_PER_DSP      8
#define MAGIC_NUMBER           0xbabeface

#ifdef _X86
//  #if defined(PKTLIB) && defined(DEMOBUILD)
  #ifdef DEMOBUILD
    #define MAX_SESSIONS_PER_CORE  4    /* demo version */
  #else
    #define MAX_SESSIONS_PER_CORE  512  /* x86 build current limit */
  #endif
#else
    #define MAX_SESSIONS_PER_CORE  2048  /* c66x build limit */
#endif

#ifdef _X86
  #define MAX_SESSIONS MAX_SESSIONS_PER_CORE
  #define NCORECHAN (4*MAX_SESSIONS_PER_CORE)   /* allow for dynamic channels */
  #define NIPCHAN   NCORECHAN
#endif

#if 0  /* remove, _NO_HASH_ should be defined in Makefile, JHB Sep2016 */
  #if defined(_USE_NET_IO_) && !defined(_USE_PCIE_)
  #define _NO_HASH_
  #endif
#endif

/* Common between control plane cores and coCPU cores */
#define DSP_MAILBOX_MEM_SIZE            0x10000
#define TRANS_PER_MAILBOX_MEM_SIZE      (DSP_MAILBOX_MEM_SIZE/NUM_CORES_PER_DSP/2)
#define TRANS_MAILBOX_MAX_PAYLOAD_SIZE  202

/* Common between DPDK cores and coCPU cores */
/* coCPU configuration information - host should configure these parameters */
typedef struct dsp_config_info_s
{
	uint32_t dp_dsp_ctrl_reg;
	uint32_t dsp_dp_ctrl_reg;
	uint32_t dp_dsp_length;
	uint32_t dp_dsp_buffer_id;			/* 0 for A buffer, 1 for B buffer */
	uint32_t dsp_dp_length;
	uint32_t dp_dsp_buffer_a;
	uint32_t dp_dsp_buffer_b;
	uint32_t dsp_dp_buffer;
} dsp_config_info_t;

#ifdef _USE_PCIE_
/* Bit definitions for ctrl_reg */
#define CTRL_DP_DSP_NEED_DATA    0x00000001	/* coCPU core sets this bit to indicate it needs more data */
#define CTRL_DP_DSP_DATA_READY   0x00000002	/* host DP core sets this bit to indicate it has data ready */
#define CTRL_DSP_DP_CAN_XFER     0x00000010	/* host DP core sets this bit to indicate DSP can transfer data now */
#define CTRL_DSP_DP_XFER_DONE    0x00000020	/* coCPU core sets this bit to indicate data transfer is completed */

/* Buffer length for buffers */
#define DP_DSP_BUFFER_SIZE       0x100000
#define DSP_DP_BUFFER_SIZE       0x100000
#endif

/* File paths for c66x images */
#ifdef _ADV8901_
  #define DSP_DIRECTORY          "/usr/IMS/current/bin"
  #define DSP_IMAGE_NAME         DSP_DIRECTORY"/C66xx_RTAF_SYSBIOS_CCSv54.out"
  #define DSP_IMAGE_NAME_NEW     "/root/dsp_tester/C66xx_RTAF_SYSBIOS_CCSv54.out"
  #define DSP_INIT_OUT           DSP_DIRECTORY"/init.out"
  #define CMEM_DEV_KO            DSP_DIRECTORY"/cmem_dev.ko"
#else
  #define DSP_DIRECTORY          "/root/test_utility/dsp_images"
  #define DSP_IMAGE_NAME         DSP_DIRECTORY"/C66xx_RTAF_SYSBIOS_CCSv54.out"
  #define DSP_INIT_OUT           DSP_DIRECTORY"/init.out"
  #define CMEM_DEV_KO            DSP_DIRECTORY"/cmem_dev.ko"
#endif

#endif /* TRANSCODING_H_ */
