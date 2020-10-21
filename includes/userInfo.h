/* $Header: /root/Signalogic/DirectCore/include/userInfo.h
 *	
 *	User Interface header file
 *
 * Project: DirectCore, SigSRF
 *
 * Copyright Signalogic Inc. 1994-2020
 * 
 * 2     10/10/05 12:32p
 * 8 DSP support.
 * 
 * 1     8/24/05 7:13p
 * Clean up code and rebuild.
 
   Modified Nov 2009 JHB, added enable TDM option
   Modified Apr 2009 JHB, added enable H.110 option
   Modified Jun 2012 JHB, added test mode and routing config
   Modified Jul 2012 JHB, added level of display details
   Modified May 2014 AM,  added command lineparameters for crypto and FFT demo programs
   Modified Nov 2014 JHB, fixed some naming to support multiple targets, unify/consolidate command line params for all test programs
   Modified Dec 2014 JHB, add support for IP addr, UDP port, and MAC addr entry (e.g. -Daa.bb.cc.dd:port:aa-bb-cc-dd-ee-ff-gg)
   Modified Aug 2017 JHB, added programModeSubMode to hold cmd line sub-mode value (e.g. -M0b where b is the sub-mode).  Added streamlib.h include to equate MAX_CONCURRENT_STREAMS with MAXSTREAMS
   Modified Feb 2019 JHB, add #define for nReuseInputs, overloaded on fftOrder.  This is to support input reuse in mediaMin, for high capacity / load testing
   Modified Jul 2019 JHB, add #defines for nSegmentation, overloaded on inputType, and for nAmplitude, overloaded on baseAddr.  These are to support silence detection, strip, and chunk rewrite functionality in mediaTest (e.g. for Kaldi group users)
   Modified Dec 2019 JHB, add #define for nJitterBufferParams, for mediaMin app usage
   Modified Jan 2020 JHB, add #define for nRepeat, for mediaMin app usage
*/

#ifndef _USERINFO_H_
#define _USERINFO_H_

#include "alias.h"
#include <stdint.h>

#ifndef CMDOPT_MAX_INPUT_LEN
  #define CMDOPT_MAX_INPUT_LEN MAX_INPUT_LEN
#endif

#if __GNUC__ >= 5
  #if __has_include("streamlib.h")
    #include "streamlib.h"
  #endif
#else
  #include "streamlib.h"
#endif

#ifdef MAXSTREAMS
  #define MAX_CONCURRENT_STREAMS MAXSTREAMS
#else
  #define MAX_CONCURRENT_STREAMS 8
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

/* shared/common test program parameters */

   int       numCoresPerCPU;
   uint64_t  coreBitMask;
   int       processorClockrate;
   int       baseAddr;
   char      targetFileName[CMDOPT_MAX_INPUT_LEN];
   char      cardDesignator[CMDOPT_MAX_INPUT_LEN];
   bool      enableTalker;
   bool      enableTDM;
   bool      enableH110;
   int       verbose;
   char      testMode[CMDOPT_MAX_INPUT_LEN];
   int       routingConfig;
   int       detailsLevel;
   char      inputFile[MAX_CONCURRENT_STREAMS][CMDOPT_MAX_INPUT_LEN];
   char      outputFile[MAX_CONCURRENT_STREAMS][CMDOPT_MAX_INPUT_LEN];
   char      configFile[MAX_CONCURRENT_STREAMS][CMDOPT_MAX_INPUT_LEN];
   char      logFile[MAX_CONCURRENT_STREAMS][CMDOPT_MAX_INPUT_LEN];
   int       algorithmIdNum;
   int       libFlags;
   uint64_t  taskAssignmentCoreLists;
   int       debugMode;
   int       programMode;
   int       programSubMode;
   char      executeMode;

/* FFT algorithm parameters */

   int       fftOrder;
   int       inputType;

/* video parameters */

   int       xres[MAX_CONCURRENT_STREAMS];
   int       yres[MAX_CONCURRENT_STREAMS];
   int       streamingMode[MAX_CONCURRENT_STREAMS];
   int       frameRate[MAX_CONCURRENT_STREAMS];
   int       profile[MAX_CONCURRENT_STREAMS];
   int       bitrateConfig[MAX_CONCURRENT_STREAMS];
   int       qpValues[MAX_CONCURRENT_STREAMS];
   int       interFrameConfig[MAX_CONCURRENT_STREAMS];

/* streaming parameters */

   int       bitRate[MAX_CONCURRENT_STREAMS];
   int       dstIpAddr[MAX_CONCURRENT_STREAMS];
   int       dstUdpPort[MAX_CONCURRENT_STREAMS];
   unsigned long long dstMacAddr[MAX_CONCURRENT_STREAMS];  /* 6 bytes */
   int       srcIpAddr[MAX_CONCURRENT_STREAMS];
   int       srcUdpPort[MAX_CONCURRENT_STREAMS];
   unsigned long long srcMacAddr[MAX_CONCURRENT_STREAMS];  /* 6 bytes */
   
/* Scrypt algorithm parameters */

   char      szScriptFile[CMDOPT_MAX_INPUT_LEN];
   char      szRmtIpAddr[CMDOPT_MAX_INPUT_LEN];
   //char decryptedFile[MAX_INPUT_LEN]; 
   char      scryptpasswd[CMDOPT_MAX_INPUT_LEN]; 	
   char      scryptsalt[CMDOPT_MAX_INPUT_LEN]; 
   bool      userMode;
   bool      encMode;
   bool      decMode;
   uint64_t  scryptParamN; 
   uint32_t	 scryptParamr;
   uint32_t  scryptParamp; 
   uint32_t  scryptdklen;

/* re-use some items for different apps, try to avoid changing this struct when possible, JHB Sep2018 */

   #define   uPerformanceMeasurement scryptParamp  /* non-Scrypt app usage of -p cmd line entry, JHB Sep2018 */
#if 0
   #define   lReuseInputs scryptParamN             /* non-Scrypt app usage of -N cmd line entry, JHB Jan2019 */
#else
   #define   nReuseInputs fftOrder                 /* mediaMin app usage of -n cmd line entry, JHB Apr2019 */
#endif
   #define   nSegmentation inputType               /* mediaTest app usage of -s cmd line entry, JHB Jul2019 */
   #define   nInterval scryptParamr                /* mediaTest app usage of -I cmd line entry, JHB Jul2019 */
   #define   nAmplitude baseAddr                   /* mediaTest app usage of -A cmd line entry, JHB Jul2019 */
   #define   nJitterBufferOptions scryptParamp     /* mediaMin app usage of -j cmd line entry, JHB Dec2019 */ 
   #define   nRepeatTimes scryptdklen              /* mediaMin app usasge of -R cmd line enry, JHB Jan2020 */

} UserInterface;


extern int getUserInfo(int argc, char* argv[], UserInterface *userIfs, unsigned int uFlags);

#ifdef __cplusplus
}
#endif

#endif
