/* $Header: /root/Signalogic/DirectCore/include/userInfo.h
 
 User Interface header file

 Project: DirectCore, SigSRF

 Copyright Signalogic Inc. 1994-2025

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 2     10/10/05 12:32p
 8 DSP support.

 1     8/24/05 7:13p
 Clean up code and rebuild.
 
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
   Modified Jan 2020 JHB, add #define for nRepeat, for mediaMin app cmd line usage
   Modified Jan 2021 JHB, add #define for szSDPFile for mediaMin app SDP file cmd line input
   Modified Dec 2021 JHB, make debugMode 64-bit int
   Modified Dec 2022 JHB, add #define for szStreamGroupWavOutputPath for mediaMin app stream group wav output path cmd line input
   Modified May 2023 JHB, change frameRate from int to float in UserInterface struct, change dstUdpPort[] from unsigned int to uint16_t
   Modified May 2023 JHB, enable mediamin app usage of -l for RFC7198 lookback depth. Note default value of 1 if no entry, handled in getUserInfo() in get_user_interface.cpp
   Modified Nov 2023 JHB, add nCut command line option
   Modified Dec 2023 JHB, add szStreamGroupPcapOutputPath #define to support --group_pcap cmd line option for mediaMin app
   Modified Feb 2024 JHB, add md5sum and show_audio_classification #defines to support --md5sum and --show_aud_clas cmd line options for mediaMin and mediaTest apps
   Modified Jul 2024 JHB, add CmdLineFlags_t struct with 1-bit flags to support continued command-line options expansion
   Modified Jul 2024 JHB, add nRandomBitErrorPercentage define to support mediaTest payload / packet impairment operations
   Modified Aug 2024 JHB, add sha1sum and sha512sum flags to CmdLineFlags_t
   Modified Feb 2025 JHB, remove references to MAX_CONCURRENT_STREAMS and MAXSTREAMS; instead all libs and apps are now using a single definition MAX_STREAMS, in shared_include/streamlib.h
   Modified Jul 2025 JHB, add stdout_ready_profile to CmdLineFlags_t
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

#if 0
#ifdef MAXSTREAMS
  #define MAX_CONCURRENT_STREAMS MAXSTREAMS
#else
  #define MAX_CONCURRENT_STREAMS 8
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CmdLineFlags_t {  /* 1-bit flags used inside UserInterface_t, JHB Jul 2024 */

  uint64_t  md5sum: 1;
  uint64_t  sha1sum: 1;
  uint64_t  sha512sum: 1;
  uint64_t  show_audio_classification : 1;
  uint64_t  group_output_no_copy : 1;
  uint64_t  stdout_ready_profile : 1;
  uint64_t  exclude_payload_type_from_key : 1;

  uint64_t  Reserved : 57;

} CmdLineFlags_t;

typedef struct UserInterface_t {

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
   char      inputFile[MAX_STREAMS][CMDOPT_MAX_INPUT_LEN];
   char      outputFile[MAX_STREAMS][CMDOPT_MAX_INPUT_LEN];
   char      configFile[MAX_STREAMS][CMDOPT_MAX_INPUT_LEN];
   char      logFile[MAX_STREAMS][CMDOPT_MAX_INPUT_LEN];
   int       algorithmIdNum;
   int       libFlags;
   uint64_t  taskAssignmentCoreLists;
   int64_t   debugMode;
   int       programMode;
   int       programSubMode;
   char      executeMode;

/* FFT algorithm parameters */

   int       fftOrder;
   int       inputType;

/* video parameters */

   int       xres[MAX_STREAMS];
   int       yres[MAX_STREAMS];
   int       streamingMode[MAX_STREAMS];
   float     frameRate[MAX_STREAMS];
   int       profile[MAX_STREAMS];
   int       bitrateConfig[MAX_STREAMS];
   int       qpValues[MAX_STREAMS];
   int       interFrameConfig[MAX_STREAMS];

/* streaming parameters */

   int       bitRate[MAX_STREAMS];
   int       dstIpAddr[MAX_STREAMS];
   uint16_t  dstUdpPort[MAX_STREAMS];
   unsigned long long dstMacAddr[MAX_STREAMS];  /* 6 bytes */
   int       srcIpAddr[MAX_STREAMS];
   uint16_t  srcUdpPort[MAX_STREAMS];
   unsigned long long srcMacAddr[MAX_STREAMS];  /* 6 bytes */
   
/* Scrypt algorithm parameters */

   char      szScryptFile[CMDOPT_MAX_INPUT_LEN];
   char      szRmtIpAddr[CMDOPT_MAX_INPUT_LEN];
 //char      decryptedFile[MAX_INPUT_LEN]; 
   char      scryptpasswd[CMDOPT_MAX_INPUT_LEN]; 	
   char      scryptsalt[CMDOPT_MAX_INPUT_LEN]; 
   bool      userMode;
   bool      encMode;
   bool      decMode;
   union {                                            /* to support command line options expansion, overlay CmdLineFlags struct on unused scrypt parameters. Each flag is 1 bit, JHB Jul 2024 */
     uint64_t  scryptParamN;
     CmdLineFlags_t CmdLineFlags;
   };
   uint32_t	 scryptParamr;
   uint32_t  scryptParamp; 
   uint32_t  scryptdklen;

/* re-use some items for different apps, try to avoid changing this struct when possible, JHB Sep 2018 */

   #define   uPerformanceMeasurement scryptParamp     /* non-Scrypt app usage of -p cmd line entry, JHB Sep 2018 */
#if 0
   #define   lReuseInputs scryptParamN                /* non-Scrypt app usage of -N cmd line entry, JHB Jan 2019 */
#else
   #define   nReuseInputs fftOrder                    /* mediaMin app usage of -n cmd line entry, JHB Apr 2019 */
#endif
   #define   nSegmentation inputType                  /* mediaTest app usage of -s cmd line entry, JHB Jul 2019 */
   #define   nInterval scryptParamr                   /* mediaTest app usage of -I cmd line entry, JHB Jul 2019 */
   #define   nAmplitude baseAddr                      /* mediaTest app usage of -A cmd line entry, JHB Jul 2019 */
   #define   nJitterBufferOptions scryptParamp        /* mediaMin app usage of -j cmd line entry, JHB Dec 2019 */ 
   #define   nRepeatTimes scryptdklen                 /* mediaMin app usasge of -R cmd line entry, JHB Jan 2020 */
   #define   szSDPFile szScryptFile                   /* mediaMin app usasge of -s cmd line entry for SDP file input. Note that mediaTest uses -s for audio file segmentation, JHB Jan 2021 */
   #define   szStreamGroupWavOutputPath scryptpasswd  /* mediaMin app usasge of -g cmd line entry for stream group wav output path, JHB Dec 2022 */
   #define   szStreamGroupPcapOutputPath scryptsalt   /* mediaMin app usasge of --group_pcaps cmd line entry for stream group pcap output path, JHB Dec 2023 */
   #define   nSamplingFrequency scryptParamp          /* mediaTest app Fs for gpx processing, JHB Mar 2022 */
   #define   nLookbackDepth libFlags                  /* mediamin app usage of -l for RFC7198 lookback depth. Note default value of 1 if no entry, handled in getUserInfo() in get_user_interface.cpp, JHB May 2023 */
   #define   nCut detailsLevel
   #define   nRandomBitErrorPercentage algorithmIdNum

} UserInterface;

extern int getUserInfo(int argc, char* argv[], UserInterface *userIfs, unsigned int uFlags, const char* ver_str);

#ifdef __cplusplus
}
#endif

#endif
