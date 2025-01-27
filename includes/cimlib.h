/*
  $Header: /root/Signalogic_YYYYvN/DirectCore/include/cimlib.h

  Purpose:

      CIM (Compute Intensive Multicore) Library API definition
      
        -high level CPU array management
        -x86 and CIM code generation support
        -support VoIP libraries (voplib, callmgr)

  Copyright (C) Signalogic Inc. 2008-2024

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

  Revision History

   Created Sep 2008, JHB
   Modified Mar 2009 - Apr 2009 XC, JHB
   Modified Jun 2009 JHB, prepare for shared lib format, eliminate extern vars used by host app
   Modified Nov 2009 JHB, add support for x86 server with no T1/E1 or T3 hardware installed
   Modified Jul 2010 JHB, create separate cimlib
   Modified Apr 2015 JHB, Rev 2 CIM code generation support, added cimRunHardware() API.  Modified Makefile to create two libs, cimlib and cimlib_emu. The EMU version can be called by API programs (cimlib is built with _CIM flag)
   Modified Jan 2021 JHB, add CIM_GCL_MEDIAMIN flag. See usage in getUserInfo() in getUserInterface.cpp
   Modified Jul 2023 JHB, add const char* version_info param to cimGetCmdLine()
   Modified Nov 2023 JHB, edit ver_str to version_info in cimGetCmdLine. No functional change
   Modified Feb 2024 JHB, add CIM_GCL_MEDIATEST flag. See usage in getUserInfo() in getUserInterface.cpp
   Modified Nov 2024 JHB, remove streamlib.h include
*/

#ifndef _CIMLIB_H_
#define _CIMLIB_H_


#ifdef __cplusplus
  extern "C" {
#else
  #ifndef bool
    #define bool unsigned char
  #endif
#endif

#include "alias.h"
#include "userInfo.h"

/* host and target shared header files */

#include "cim.h"
#include "c66x_accel.h"


/* cimlib version string global var */
extern const char CIMLIB_VERSION[256];

/* error types */

typedef enum {

   DS_STATUS_FAIL,      /* operation failed */
   DS_STATUS_SUCCESS,   /* success */
   DS_STATUS_ERROR      /* operation partially succeeded, but had some errors */

} DS_STATUS;


typedef enum {

/* generic errors */

   DS_ERROR_NONE,      /* no error */
   DS_INVALID_ARGS,    /* invalid/insufficient/missing arguments */
   DS_INTERNAL_ERROR,  /* internal error - unable to process request */

/* target CPU errors */

   DS_ERROR_TARGET_NOT_INITIALIZED,

/* Hardware related errors */

   DS_ERROR_TARGET_HARDWARE_ASSIGN,
   DS_ERROR_TARGET_HARDWARE_LIB,
   DS_ERROR_TARGET_HARDWARE_INIT,
   DS_ERROR_TARGET_HARDWARE_ERRCOND,

} DS_TARGET_ERROR;



/* CIM run-time definitions and APIs */

#define MAX_CIM_VARS 4096

typedef struct {

  const char* szVarName;  /* var name */
  unsigned int Type;      /* type:  input, output, or bidirectional */
  unsigned int HostAddr;  /* host mem address (e.g. user space address) */
  unsigned int CimAddr;   /* CIM mem address (e.g. after symbol table lookup */
  unsigned int NumBytes;  /* var length */
  
} CIMVARS;

typedef CIMVARS* PCIMVARS;


typedef struct {

  CIMVARS CimVars[MAX_CIM_VARS];  /* list of variables */
  unsigned int NumVars;           /* number of vars in the list */
  
} RTCODESECTIONS;

typedef RTCODESECTIONS* PRTCODESECTIONS;


typedef struct {

  short int Command_Status;
  short int TaskNum;

} CIMWORKQUEUE;

typedef CIMWORKQUEUE* PCIMWORKQUEUE;

#define DS_CIM_RUNTASK 1


typedef struct {

/* from command line */
 
  char          szCardDesignator[CMDOPT_MAX_INPUT_LEN];
  char          szTargetExecutableFile[CMDOPT_MAX_INPUT_LEN];
  unsigned int  nClockRate;
  QWORD         nCoreList;

/* derived from command line entries */

  char          szCardDescription[CMDOPT_MAX_INPUT_LEN];
  unsigned int  maxCoresPerCPU;
  unsigned int  maxCPUsPerCard;
  unsigned int  maxActiveCoresPerCard;
  
  unsigned int  numActiveCPUs;   /* total number of currently active CPUs (note: not max CPUs, but CPUs currently in use) */
  unsigned int  numActiveCores;  /* total number of currently active cores (note: not max cores, but cores currently in use) */

  bool          enableNetIO;     /* set if command line params indicate that network I/O is needed.  Various application-specific params are checked */
  
  WORD          wCardClass;      /* card classifier */
  unsigned int  uTestMode;       /* parameter controlling hardware power-on and reset test modes (POST) */
  bool          enableTalker;    /* Not used for x86 and c66x hardware.  Used for legacy c64x and c55x hardware */

  CIMINFO       cimInfo[MAXCPUSPERCARD];  /* consists of uint8_t taskAssignmentCoreLists[MAX_TASKASSIGNMENT_CORELISTS], see cim.h in shared_include folder */
  
} CARDPARAMS;  /* common target CPU and card params */

typedef CARDPARAMS* PCARDPARAMS;

#define PLATFORMPARAMS CARDPARAMS  /* platform reference for x86 systems */
#define PPLATFORMPARMS PCARDPARAMS
#define szPlatformDesignator szCardDesignator

/* function mode flag values, used by functions called within task pragmas */

#define  CIM_FUNCMODE_NOTUSED              0
#define  CIM_FUNCMODE_INIT                 1
#define  CIM_FUNCMODE_UPDATE               2
#define  CIM_FUNCMODE_CLEANUP              3


/* API call flags */

/* cimGetCmdLine flags */

#define CIM_GCL_VID                        1
#define CIM_GCL_STREAMING                  2
#define CIM_GCL_VDI                        4
#define CIM_GCL_IA                         8
#define CIM_GCL_MED                        0x10
#define CIM_GCL_FFT                        0x20
#define CIM_GCL_MEDIAMIN                   0x40
#define CIM_GCL_MEDIATEST                  0x80
#define CIM_GCL_CMDLINEMASK                0xff
#ifdef DS_STC_DEBUGPRINT
  #define CIM_GCL_DEBUGPRINT               DS_STC_DEBUGPRINT
#else
  #define CIM_GCL_DEBUGPRINT               0x10000000U
#endif

#define CIM_GCL_DISABLE_MANDATORIES        0x100
#define CIM_GCL_FILLUSERIFS                0x200
#define CIM_GCL_SUPPRESS_STREAM_MSGS       0X400


/* cimRunHardware flags (note - flags from 0 to 0xff shared with other APIs) */

#define CIM_RH_EMULATECIMBUILD             0x100  /* for API builds that want to test using CIM generated target CPU codes */
#define CIM_RH_EMULATECIMBUILD_NOERRPRINT  0x400  /* same, but no error print if symbols are not found */
#define CIM_RH_ENABLENETIO                 0x200
#define CIM_RH_DEBUGPRINT                  DS_STC_DEBUGPRINT


/* cimInitHardware flags */

#define CIM_IH_DEBUGPRINT                  DS_STC_DEBUGPRINT


/* cimCloseHardware flags */

#define CIM_CH_DEBUGPRINT                  DS_STC_DEBUGPRINT


/* cimBarrier flags */

#define CIM_B_WAIT                         1
#define CIM_B_CHECKSTATUS                  2
#define CIM_B_INIT                         0x100
#define CIM_B_DEBUGPRINT                   DS_STC_DEBUGPRINT


/* cimDebugPrint flags */

#define CIM_DP_FORMAT_SAMELINE             1
#define CIM_DP_FORMAT_HEX                  0x100
#define CIM_DP_FORMAT_UNSIGNED             0x200
#define CIM_DP_FORMAT_LONG                 0x400
#define CIM_DP_FORMAT_CPUNOTATION          0x1000
#define CIM_DP_FORMAT_SHOWSYMADDR          0x2000

 
/* cimControlCPUArray() constants */

#define CIM_RUN_CPU                        1
#define CIM_RESET_CPU                      2
#define CIM_PWRDN_CPU                      3
#define CIM_LIB_INIT                       4  /* causes MXP shell to run on SigC5561 card (ignored on SigC641x card) */
#define CIM_LIB_CLOSE                      5  /* returns to VxWorks shell on SigC5561 card (ignored on SigC641x card) */


/* Run-Time API definitions */

BOOL cimInitHostSections(PRTCODESECTIONS pRtCodeSections, unsigned int NumSecs);

BOOL cimInitCimSections(PRTCODESECTIONS pRtCodeSections, unsigned int NumSecs);

HCARD cimInitHardware(UINT, PCARDPARAMS);
#define cimInitPlatform cimInitHardware

UINT cimRunHardware(HCARD, UINT, PCARDPARAMS, void*);

UINT cimCloseHardware(HCARD, UINT, QWORD, void*);
#define cimClosePlatform cimCloseHardware

UINT cimGetCmdLine(int argc, char* argv[], UserInterface*, UINT, PCARDPARAMS, void*, const char* version_info);

UINT cimWriteSymbol(HCARD, UINT, const char*, void*, UINT, UINT, QWORD);

UINT cimReadSymbol(HCARD, UINT, const char*, void*, UINT, UINT, QWORD);

QWORD cimBarrier(HCARD, UINT, const char*, QWORD);

UINT cimDebugPrint(HCARD, UINT, const char*, const char*, UINT, UINT, QWORD);


/* initialization-time target CPU C Code mem symbols (vars, arrays, structures, etc) */

typedef struct {

/* basic target CPU performance and capacity parameters */

   DWORD dwNumAlgChan;
   DWORD dwClockRate;

/* unique CPU Id -- target CPU software can look at this and know "which chip am I" */

   DWORD dwCPUId;

/* run-time target CPU C Code mem flags */

   DWORD dwCtbusMute;
   DWORD dwMotBdInCircuit;
   DWORD dwAgcActive;
   DWORD dwVadActive;
   DWORD dwDtmfActive;
   DWORD dwfEchoReduceActive;
   DWORD dwSup;
   DWORD dwVADResult;
   DWORD dwPassThru;

/* CPU event items */

   DWORD dwEventBuf;
   DWORD dwEventPtr;
   DWORD dwHostEventPtr;

/* Diagnostic data CPU mem addresses */

   DWORD dwDiagnostic;
   DWORD dwDebugArray;
   DWORD dwsysMemory;

/* IP/UDP/RTP items in CPU mem */

   DWORD dwPktbuf;
   DWORD dwDummy32;
   DWORD dwRecvPktBuf16;
   DWORD dwHostPktPtr;
   DWORD dwSrcAddr;
   DWORD dwDstAddr;
   DWORD dwSrcPort;
   DWORD dwDstPort;
   DWORD dwIpSendPtr;
   DWORD dwHostIpSendPtr;
   DWORD dwIpSendBuf;

/* dynamic update of VAD parameters in CPU memory */

   DWORD dwVADparams;
   DWORD dwfnewVADparams;

/* time stamp and profiling items in CPU mem */

   DWORD dwH110TimeStamp;
   DWORD dwTimeStamp;
   DWORD dwTimeOut;
   DWORD dwisr_array;
   DWORD dwh110_16;
   DWORD dwMax_Time;

/* CTbus settings */

   DWORD dwCtbusFramesyncDelay;
  
} DS_TARGET_ADDR;


/* CPU Array Management APIs */

/* Initialize SigC641x or SigC5561 DSP array (PTMC module or PCI/PCIe card)
*/
HBOARD cimOpenCPUArray(const char* szBoard, int nModule, int numProcessors, const char* szCodeFile, DS_TARGET_ERROR* const pError);

/* Initialize one or more CPUs and associated resources

   Notes: 1) nCoreList is a list of CPU cores.  Each bit specifies a core, for example list
             of 1 is first core, list of 3 is first 2 cores, list of 0xff is 8 cores
*/
DS_STATUS cimInitCPUArray(HBOARD hBoard, int nCoreList, DS_TARGET_ADDR* target_addr, DS_TARGET_ERROR* const pError);

/* Control one or more CPUs -- run, reset, power-down, etc
*/
DS_STATUS cimControlCPUArray(HBOARD hBoard, int nCoreList, int nAction, DS_TARGET_ERROR* const pError);


#ifdef __cplusplus
}
#endif

#endif /* _CIMLIB_H_ */
