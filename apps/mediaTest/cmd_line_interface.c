/*
 $Header: /root/Signalogic/apps/mediaTest/cmd_line_interface.c

 Copyright (C) Signalogic Inc. 2018-2025

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use
 
 Description
 
   command line interface and processing for packet / media related SigSRF programs

 Purpose
 
   support cmd line entry for mediaTest and mediaMin apps

 Revision History
 
   Created Aug 2018 JHB, orginated as command line related sections of mediaTest.c
   Modified Mar 2018 JHB, (from mediaTest.c) add USB audio cmd line support
   Modified Apr 2018 JHB, (from mediaTest.c) added get_file_type(), consolidated cmd line processing for all file I/O types (raw audio, wav, tim, encoded, pcap, USB audio. Encoded includes .cod, .amr, .awb. See mediaTest.h enums)
   Modified Aug 2018 JHB, moved ctrl-c signal handler here, moved GetOutputFilename() here, moved vars used by packet_flow_media_proc() to be extern here and global in packet_flow_media_proc.c, which is now included in both mediaTest and pktlib builds
   Modified Aug 2018 JHB, set debugMode cmd line option for use by mediaMin app
   Modified Dec 2018 JHB, remove extern var declarations that were duplicates of some in mediaTest.h
   Modified Dec 2018 CKJ, add encoded file extension ".BIT" to support AMR-WB+
   Modified Jan 2019 JHB, add nReuseInputs to support 500+ session stress testing
   Modified Jul 2019 JHB, add nSegmentation, nInterval, and nAmplitude to support wav file segmentation, silence detection, strip, and chunk rewrite functionality in mediaTest (e.g. for Kaldi group users)
   Modified Sep 2019 JHB, in cmdLineInterface() check for CLI_MEDIA_APPS flag and pass to cimGetCmdLine() (cimlib.so), which will in turn give to getUserInfo() (getUserInterface.cpp)
   Modified Nov 2019 JHB, add CSV and TEXT output file types (latter to support Kaldi ASR)
   Modified Dec 2019 JHB, add nJitterBufferParams to support jitter buffer target and max delay cmd line entry (-jN)
   Modified Jan 2020 JHB, add nRepeats to support number of repeat times cmd line entry (-RN)
   Modified Jan 2021 JHB, change references to AUDIO_FILE_TYPES to IS_AUDIO_FILE_TYPE, initialize outFileType2, USBAudioInput, and USBAudioOutput, allow pcap for output file type, add char szSDPFile[CMDOPT_MAX_INPUT_LEN]
   Modified Dec 2021 JHB, make debugMode 64-bit int
   Modified Mar 2022 JHB, add GPX file input handling, -Fn flag for mediaTest gpx processing
   Modified Aug 2022 JHB, adjust declaration of a few global vars to allow no_mediamin and no_pktlib options in mediaTest build (look for run, frame_mode, etc vars)
   Modified Dec 2022 JHB, add char szStreamGroupWavOutputPath[CMDOPT_MAX_INPUT_LEN]
   Modified Dec 2022 JHB, move sig_lib_event_log_filename here from packet_flow_media_proc.c
   Modified Jan 2023 JHB, in ctrl-c event handler call DSConfigLogging() with DS_CONFIG_LOGGING_PKTLOG_ABORT flag and set fCtrl_C_pressed. Don't set run = 0 for mediaMin
   Modified Jan 2023 JHB, add szAppFullCmdLine var and GetCommandLine()
   Modified May 2023 JHB, suppress "address of var will never be NULL" warnings in gcc 12.2; safe-coding rules prevail
   Modified May 2023 JHB, add timeScale and convert RealTimeInterval[] to float to support FTRT and AFAP modes, add uPortList[], add uLoopbackDepth
   Modified Jun 2023 JHB, initialize timeScale to zero to allow packet_float_media_proc() to know if it's already been set by an app
   Modified Jul 2023 JHB, add const char* version_info param to cmdLineInterface()
   Modified Aug 2023 JHB, add uTimestampMatchMode. Flags are defined in shared_include/streamlib.h
   Modified Sep 2023 JHB, and fCapacityTest. Moved here from mediaMin.cpp, referenced both there and in packet_flow_media_proc.c
   Modified Nov 2023 JHB, add banner_info param to cmdLineInterface(), add nCut (set from cmd line --cut option). Use strcasestr() and strcasecmp() in get_file_type()
   Modified Nov 2023 JHB, add support for .rtp and .rtpdump
   Modified Dec 2023 JHB, add char szStreamGroupPcapOutputPath[CMDOPT_MAX_INPUT_LEN]
   Modified Feb 2024 JHB, omit reference to cimGetCmdLine() when NO_CIMLIB is defined
   Modified Feb 2024 JHB, change default init values of executeMode[], fix compiler security warning when printing banner_info in cmdLineInterface()
   Modified Feb 2024 JHB, add fShow_md5sum and fShow_audio_classification to support --md5sum and --show_aud_clas command line options
   Modified Feb 2024 JHB, give CIM_GCL_MEDIATEST flag to cimGetCmdLine() if needed
   Modified Apr 2024 JHB, set executeMode[0] to 'a' for mediaTest packet and frame test modes, nail down initialization of RealTimeInterval[]
   Modified May 2024 JHB, add more checks when setting executeMode[0]
   Modified May 2024 JHB, update comments that reference x86_mediaTest to mediaTest_proc
   Modified Jul 2024 JHB, adjust param order in DSConfigLogging() after change in diaglib.h
   Modified Jul 2024 JHB, trim leading and trailing spaces of -g and --group_pcap cmd line argument strings, in case apps are run from shell scripts
   Modified Jul 2024 JHB, add  fGroupOutputNoCopy to support --group_pcap_nocopy cmd line option, integrate CmdLineFlags_t changes in userInfo.h
   Modified Jul 2024 JHB, set mediaTest pcap_extract when both input and output files are pcaps, to support pcap modification operations (insert bit errors, filter, etc)
   Modified Jul 2024 JHB, add nRandomBitErrorPercentage to support --random_bit_error cmd line option
   Modified Aug 2024 JHB, add fShow_sha1sum and fShow_sha512sum to support --sha1sum and --sha512sum command line options
   Modified Sep 2024 JHB, update get_file_type() to handle .h264, .h265, and .hevc file extensions
   Modified Nov 2024 JHB, include directcore.h (no longer implicitly included in other header files)
   Modified Dec 2024 JHB, include <algorithm> and use std namespace if __cplusplus defined
   Modified Jan 2025 JHB, avoid unused var warning in get_file_type()
   Modified Feb 2025 JHB, change references to MAX_INPUT_STREAMS and MAX_CONCURRENT_STREAMS to MAX_STREAMS, defined in shared_include/streamlib.h. All libs and reference apps are now using the same definition
   Modified Mar 2025 JHB, remove debug print flag from cimGetCmdLine() uFlags for mediaMin and mediaTest apps
*/

#ifdef __cplusplus
  #include <algorithm>
  using namespace std;
#endif

/* system header files */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

/* SigSRF includes */

#include "directcore.h"  /* DirectCore APIs */
#include "diaglib.h"  /* event log and packet log APIs. Currently we only use DSConfigLogging() here, JHB Jan 2023 */

#if defined(_ALSA_INSTALLED_)
  #include "aviolib.h"
#endif

/* app support header files */

#include "test_programs.h"   /* misc program support (command line entry, etc) */
#include "minmax.h"          /* define min-max if __cplusplus not defined */
#include "dsstring.h"        /* lstrtrim() */

#include "mediaTest.h"  /* externs for following global vars, plus mediaTest definitions */

/* global vars filled by cimGetCmdLine() */

PLATFORMPARAMS PlatformParams = {{ 0 }};
MEDIAPARAMS MediaParams[MAX_STREAMS];
float RealTimeInterval[MAX_STREAMS] = { 0 };

/* global vars referenced in mediaMin.cpp, mediaTest_proc.c, and packet_flow_media_proc.c */

volatile int8_t  pm_run = 1;  /* may be cleared by application signal handler to stop packet / media processing loops */
unsigned int     inFileType, outFileType, outFileType2 = 0, USBAudioInput = 0, USBAudioOutput = 0;
char             executeMode[2] = { (char)-1, (char)-1 }; /* initialize to none. Look for switch (executeMode[0]) in mediaTest_proc.c for run-time options */
int64_t          debugMode = 0;
int              performanceMeasurementMode = 0;
int              nReuseInputs = 0;
int              nSegmentation = 0;
int              nSegmentInterval = 0;
int              nAmplitude = 0;
int              nJitterBufferParams = 0;
int              nRepeats = 0;
char             szSDPFile[CMDOPT_MAX_INPUT_LEN] = "";
int              nSamplingFrequency;  /* rate of gps point recording in gpx file processing, Mar 2022 JHB */
char             szStreamGroupWavOutputPath[CMDOPT_MAX_INPUT_LEN] = "";
#ifndef __LIBRARYMODE__  /* this group declared here in non-library build, otherwise declared in packet_flow_media_proc.c, Aug 2022 JHB */
volatile char    pktStatsLogFile[CMDOPT_MAX_INPUT_LEN] = "";
volatile int     send_sock_fd = -1, send_sock_fd_ipv6 = -1;
volatile bool    frame_mode = false, use_bkgnd_process = false, use_log_file = false;
#endif
char             sig_lib_event_log_filename[] = { "sig_lib_event_log.txt" };  /* moved here from packet_flow_media_proc.c, JHB Dec 2022 */
bool             fCtrl_C_pressed = false;
char             full_cmd_line[MAX_CMDLINE_STR_LEN] = "";  /* app full command line, filled in by cmdLineInterface(), which calls GetCommandLine(). Note that mediaTest.h publishes this as this as const char* szAppFullCmdLine, JHB Jan 2023 */
double           timeScale = 0;  /* support FTRT and AFAP modes, see comments in packet_flow_media_proc.c */
uint16_t         uPortList[MAX_STREAMS] = { 0 };
uint8_t          uLookbackDepth = 1;
unsigned int     uTimestampMatchMode = 0;  /* timestamp-match wav output mode. Flags are defined in shared_include/streamlib.h, JHB Aug 2023 */
bool             fCapacityTest = false;  /* set by apps if they are doing capacity test (moved here from mediaMin.cpp), JHB Sep 2023 */
int              nCut = 0;  /* command line --cut input */
char             szStreamGroupPcapOutputPath[CMDOPT_MAX_INPUT_LEN] = "";
bool             fShow_md5sum = false;
bool             fShow_audio_classification = false;
bool             fGroupOutputNoCopy = false;
int              nRandomBitErrorPercentage = 0;
bool             fShow_sha1sum = false;
bool             fShow_sha512sum = false;

/* global vars set in packet_flow_media_proc, but only visible within an app build (not exported from a lib build) */

#ifndef _MEDIAMIN_
extern int     nInFiles, nOutFiles, numStreams, num_wav_outputs, num_pcap_outputs;
extern int     out_type[];
#endif

/* ctrl-c signal handler */

void intHandler(int sig) {

   (void)sig;  /* not currently used */

   #ifndef _MEDIAMIN_  /* mediaMin has orderly cleanup so don't do any shortcuts, JHB Jan 2023 */
   pm_run = 0;  /* global var in packet/media thread processing (packet_flow_media_proc.c) */
   #endif
   
   fCtrl_C_pressed = true;

   DSConfigLogging(DS_CONFIG_LOGGING_ACTION_SET_FLAG, DS_CONFIG_LOGGING_PKTLOG_ABORT | DS_CONFIG_LOGGING_ALL_THREADS, NULL);  /* tell possibly time-consuming packet logging functions to abort. Currently we combine with "ALL_THREADS" flag which will terminate packet logging for any apps running. See additional comments in mediaMin.cpp JHB Jan 2023 */
}

/* global vars used by other files in the build */

/* operating mode flags (xxx_test flags):  Notes:

  1) These flags are set either (i) per mode value (-M cmd line option, see comments above), or (ii) if no mode option is given then determined by cmd line input/output options (-i and -o).  For example, codec_test is set if both input and output files are specified on the cmd line
  
  2) Only one flag is set at one time
*/

char  network_packet_test = 0, cocpu_sim_test = 0, cocpu_network_test = 0, codec_test = 0, x86_frame_test = 0, x86_pkt_test = 0, pcap_extract = 0, gpx_process = 0;
unsigned int  CPU_mode = 0, programMode = 0;


/* local functions */

int get_file_type(const char*, unsigned int);


int cmdLineInterface(int argc, char **argv, unsigned int uFlags, const char* version_info, const char* banner_info) {

UserInterface userIfs = { 0, 0, 0, 0, "", "", false };

bool fAudioInputFile, fAudioOutputFile, fPcapInputFile, fCodedInputFile, fCodedOutputFile, fPcapOutputFile, fGpxInputFile;
bool __attribute__ ((unused)) fTextOutputFile, fCSVOutputFile;  /* added Nov 2019 JHB */
unsigned int cim_uFlags;
int i;

   GetCommandLine((char*)szAppFullCmdLine, MAX_CMDLINE_STR_LEN);  /* save full command in szAppFullCmdLine global var, for use by apps as needed, JHB Jan 2023 */

/* process command line, results are in PlatformParams, MediaParms, and userIfs structs */

   cim_uFlags = CIM_GCL_DISABLE_MANDATORIES | CIM_GCL_SUPPRESS_STREAM_MSGS | CIM_GCL_FILLUSERIFS;
   if (uFlags & CLI_MEDIA_APPS) cim_uFlags |= CIM_GCL_MED;
   if (uFlags & CLI_MEDIA_APPS_MEDIAMIN) cim_uFlags |= CIM_GCL_MEDIAMIN;
   if (uFlags & CLI_MEDIA_APPS_MEDIATEST) cim_uFlags |= CIM_GCL_MEDIATEST;
 
   #ifndef NO_CIMLIB
   cimGetCmdLine(argc, argv, &userIfs, cim_uFlags, &PlatformParams, &MediaParams, version_info);  /* run first with mandatories disabled, print-outs disabled */
   #else
   unsigned int get_user_info_flags = (cim_uFlags & CIM_GCL_DISABLE_MANDATORIES) ? CLI_DISABLE_MANDATORIES : 0;
   if (uFlags & CIM_GCL_MED) get_user_info_flags |= CLI_MEDIA_APPS;
   if (uFlags & CIM_GCL_MEDIAMIN) get_user_info_flags |= CLI_MEDIA_APPS_MEDIAMIN;
   if (uFlags & CIM_GCL_MEDIATEST) get_user_info_flags |= CLI_MEDIA_APPS_MEDIATEST;

   if (getUserInfo(argc, argv, &userIfs, get_user_info_flags, version_info) == EXIT_FAILURE) return EXIT_FAILURE;
   #endif

   if (banner_info && strlen(banner_info) > 0 && strlen(banner_info) < 1024) printf("%s", banner_info);  /* show banner info before calling cimGetCmdLine() again with error/diagnostic info enabled, JHB Nov 2023 */

   if (userIfs.programMode != LOG_FILE_DIAGNOSTICS) {  /* most cmd line arguments are ignored for log file diagnostics */

      cim_uFlags = CIM_GCL_SUPPRESS_STREAM_MSGS | CIM_GCL_FILLUSERIFS | CIM_GCL_DEBUGPRINT;
      if (uFlags & CLI_MEDIA_APPS) cim_uFlags |= CIM_GCL_MED;
      if (uFlags & CLI_MEDIA_APPS_MEDIAMIN) cim_uFlags = (cim_uFlags | CIM_GCL_MEDIAMIN) & ~CIM_GCL_DEBUGPRINT;  /* remove debug print flag for mediaMin and mediaTest apps, no longer useful JHB Mar 2025 */
      if (uFlags & CLI_MEDIA_APPS_MEDIATEST) cim_uFlags = (cim_uFlags | CIM_GCL_MEDIATEST) & ~CIM_GCL_DEBUGPRINT;

      #ifndef NO_CIMLIB
      if (!cimGetCmdLine(argc, argv, &userIfs, cim_uFlags, &PlatformParams, &MediaParams, version_info)) return 0;  /* run again with everything enabled, report cmd line errors */
      #else
      unsigned int get_user_info_flags = (cim_uFlags & CIM_GCL_DISABLE_MANDATORIES) ? CLI_DISABLE_MANDATORIES : 0;
      if (uFlags & CIM_GCL_MED) get_user_info_flags |= CLI_MEDIA_APPS;
      if (uFlags & CIM_GCL_MEDIAMIN) get_user_info_flags |= CLI_MEDIA_APPS_MEDIAMIN;
      if (getUserInfo(argc, argv, &userIfs, get_user_info_flags, version_info) == EXIT_FAILURE) return EXIT_FAILURE;
      #endif
   }

   programMode = userIfs.programMode;
   
   if (programMode == LOG_FILE_DIAGNOSTICS) return 1;


/* check card designator and enable CPU and coCPU mode */

   if (strstr(PlatformParams.szCardDesignator, "X86")) CPU_mode = CPUMODE_X86;

   if (strstr(PlatformParams.szCardDesignator, "SIGX86")) CPU_mode = CPUMODE_X86_TEST;  /* added x86 test mode that uses x86 / c66x shared codec test code, JHB Feb2017 */

   if (!CPU_mode && strstr(PlatformParams.szCardDesignator, "C66X")) CPU_mode = CPUMODE_C66X;

   if (!CPU_mode) {
      printf ("Invalid CPU or coCPU setting in command line -c argument, %s\n", PlatformParams.szCardDesignator);
      return 0;
   }

/* Determine program mode */

   inFileType = get_file_type(MediaParams[0].Media.inputFilename, 0);

   fCodedInputFile = inFileType == ENCODED;
   fPcapInputFile = inFileType == PCAP;
   fAudioInputFile = IS_AUDIO_FILE_TYPE(inFileType);
   fGpxInputFile = inFileType == GPX;
   
   outFileType = get_file_type(MediaParams[0].Media.outputFilename, 1);

   fCodedOutputFile = outFileType == ENCODED;
   fPcapOutputFile = outFileType == PCAP;
   fAudioOutputFile = IS_AUDIO_FILE_TYPE(outFileType);

   fTextOutputFile = outFileType == TEXT;
   fCSVOutputFile = outFileType == CSV;

   if (fAudioOutputFile) {
   
      outFileType2 = get_file_type(MediaParams[1].Media.outputFilename, 1);
      if (outFileType2 == USB_AUDIO) outFileType |= USB_AUDIO;
   }
   else if (outFileType == USB_AUDIO) {
   
      outFileType2 = get_file_type(MediaParams[1].Media.outputFilename, 1);

      if (IS_AUDIO_FILE_TYPE(outFileType2)) {
         fAudioOutputFile = true;
         outFileType |= outFileType2;
      }
   }

   executeMode[0] = userIfs.executeMode;  /* execution mode used by mediaTest to specify app, thread, and cmd line execution modes */

   codec_test = ((fAudioInputFile || USBAudioInput || fCodedInputFile) && (fAudioOutputFile || USBAudioOutput || fCodedOutputFile || fPcapOutputFile));  /* codec mode = both I/O audio or compressed bitstream of some type (also includes output pcap) */
   pcap_extract = (fPcapInputFile && fCodedOutputFile) || (fPcapInputFile && fPcapOutputFile);
   gpx_process = (fGpxInputFile != 0);

   if (!codec_test && !pcap_extract && fPcapInputFile && (fAudioOutputFile || USBAudioOutput)) {

      if (executeMode[0] != 't' && executeMode[0] != 'p' && executeMode[0] != 'c') {  /* safeguard -- don't overwrite executeMode[0] if it already has a different legit value, JHB May 2024 */

         executeMode[0] = 'a';  /* mediaTest packet and frame modes that assign threads to packet_flow_media_proc(), JHB Apr 2024 */
         codec_test = true;
      }
   }

// printf("codec_test = %d, pcap_extract = %d, pcap input file = %d\n", codec_test, pcap_extract, fPcapInputFile);

   if (!codec_test && !pcap_extract) {  /* if not codec test, then check cmd line mode flag (-Mn entry) */

      if (CPU_mode & CPUMODE_C66X)
      {
         network_packet_test = (programMode == NETWORK_PACKET_TEST);
         cocpu_sim_test = (programMode == COCPU_SIM_TEST);
         cocpu_network_test = (programMode == COCPU_NETWORK_TEST);
      }
      else
      {
         x86_frame_test = (programMode == X86_FRAME_TEST);
         x86_pkt_test = (programMode == X86_PACKET_TEST);

         if (x86_frame_test && (fPcapInputFile || fPcapOutputFile)) frame_mode = true;  /* added frame mode with pcap input (payload-only processing, no jitter buffer). This causes a variation of x86_pkt_test code to run, Jun2017 */
      }
   }

   use_bkgnd_process = userIfs.programSubMode == 2;  /* programSubMode is 2 if 'b' suffix was entered after -M value; see apps/common/getUserInterface.cpp */

   if (strlen(userIfs.logFile[0]) && !strstr(userIfs.logFile[0], "-nopktlog") && !strstr(userIfs.logFile[0], "-nopacketlog")) {

      use_log_file = true;
      if (!strstr(userIfs.logFile[0], "[default]")) strcpy((char*)pktStatsLogFile, userIfs.logFile[0]);  /* if default entry (only -L entered with no path+filename) then just set the var, don't copy the string, JHB Sep2017 */
   }

   if (userIfs.CmdLineFlags.md5sum) fShow_md5sum = true;  /* check for default value, which indicates --md5sum was entered on the cmd line, JHB Feb 2024 */
   if (userIfs.CmdLineFlags.sha1sum) fShow_sha1sum = true;  /* add sha sums, JHB Aug 2024 */
   if (userIfs.CmdLineFlags.sha512sum) fShow_sha512sum = true;

   if (userIfs.CmdLineFlags.show_audio_classification) fShow_audio_classification = true;

   for (i=0; i<MAX_STREAMS; i++) {
   
      RealTimeInterval[i] = MediaParams[i].Media.frameRate;  /* store -rN frame rate cmd line entries in RealTimeInterval[], JHB May 2023 */
      if (isnan(RealTimeInterval[i]) || RealTimeInterval[i] < 0) RealTimeInterval[i] = userIfs.frameRate[i];  /* make sure it's not NaN and not negative. Default value should be -1, but we can be ultra careful that we don't get a messed up timing value, JHB Apr 2024 */
      if (isnan(RealTimeInterval[i]) || RealTimeInterval[i] < 0) RealTimeInterval[i] = NOMINAL_REALTIME_INTERVAL;
   }

   debugMode = userIfs.debugMode;  /* debug mode used by mediaMin to control various test/debug scenarios */

   performanceMeasurementMode = (int)userIfs.uPerformanceMeasurement;
   nReuseInputs = userIfs.nReuseInputs;
   nSegmentation = userIfs.nSegmentation;
   nSegmentInterval = userIfs.nInterval;
   nAmplitude = userIfs.nAmplitude;
   nJitterBufferParams = userIfs.nJitterBufferOptions;  /* lsbyte is target delay, next byte is max delay, JHB Dec2019 */
   nRepeats = (int)userIfs.nRepeatTimes;  /* -1 = no entry (no repeat), 0 = repeat forever, > 1 is repeat number of times, JHB Jan2020 */
   if (strlen(userIfs.szSDPFile)) strcpy(szSDPFile, userIfs.szSDPFile);
   nSamplingFrequency = userIfs.nSamplingFrequency;  /* sampling frequency for gpx processing */

   if (strlen(userIfs.szStreamGroupWavOutputPath)) {
      strcpy(szStreamGroupWavOutputPath, userIfs.szStreamGroupWavOutputPath);
      lstrtrim(szStreamGroupWavOutputPath);  /* trim leading and trailing spaces, if any ... can happen when apps are run inside shell scripts, JHB Jul 2024 */
   }
   if (strlen(userIfs.szStreamGroupPcapOutputPath)) {
      strcpy(szStreamGroupPcapOutputPath, userIfs.szStreamGroupPcapOutputPath);
      lstrtrim(szStreamGroupPcapOutputPath);  /* trim leading and trailing spaces, if any ... can happen when apps are run inside shell scripts, JHB Jul 2024 */
   }

   nRandomBitErrorPercentage = userIfs.nRandomBitErrorPercentage;

   fGroupOutputNoCopy = userIfs.CmdLineFlags.group_output_no_copy;

   for (i=0; i<MAX_STREAMS; i++) uPortList[i] = userIfs.dstUdpPort[i];  /* fill in port list, JHB May 2023 */

   uLookbackDepth = userIfs.nLookbackDepth;

   nCut = userIfs.nCut;

/* register signal handler to catch Ctrl-C signal and cleanly exit mediaTest, mediaMin, and other test programs */

#if 1  /* disable this if needed for use with gdb (to prevent mediaTest or mediaMin from handling ctrl-c), JHB Jan 2019 */
   struct sigaction act = {{ 0 }};
   act.sa_handler = intHandler;
   sigaction(SIGINT, &act, NULL);
#endif

   return 1;
}


char* find_extension(const char* filestr, const char* extstr) {  /* find extension in a path, and verify it's at the end of the path, JHB Apr 2024 */

char* p;

   if ((p = (char*)strcasestr(filestr, extstr)) && strlen(p) == strlen(extstr)) return p;
   else return NULL;
}

int get_file_type(const char* filestr, unsigned int io) {

int fileType = 0;

   (void)io;  /* avoid unused var warning, JHB Jan 2025 */

#if defined(_ALSA_INSTALLED_)
   if (!strcasecmp(filestr, "usb0")) {

      fileType = USB_AUDIO;

      if (!io) USBAudioInput = AUDIO_INPUT_USB0;  /* initial USB audio supported, add other devices later.  Sampling rate, bitwidth, number of channels, etc are in session config files.  JHB Mar 2018 */
      else USBAudioOutput = AUDIO_OUTPUT_USB0;
   }
   else if (!strcasecmp(filestr, "usb1")) {

      fileType = USB_AUDIO;

      if (!io) USBAudioInput = AUDIO_INPUT_USB1;
      else USBAudioOutput = AUDIO_OUTPUT_USB1;
   }
   else
#endif

   if (find_extension(filestr, ".pcap") || find_extension(filestr, ".pcapng") || find_extension(filestr, ".rtp") || find_extension(filestr, ".rtpdump")) fileType = PCAP;  /* add .rtp and .rtpdump, JHB Nov 2023 */
   else if (find_extension(filestr, ".inp") || find_extension(filestr, ".out") || find_extension(filestr, ".pcm") || find_extension(filestr, ".raw") || find_extension(filestr, ".sam")) fileType = RAW_AUDIO;
   else if (find_extension(filestr, ".tim")) fileType = TIM_AUDIO;
   else if (find_extension(filestr, ".au")) fileType = AU_AUDIO;
   else if (find_extension(filestr, ".wav")) fileType = WAV_AUDIO;
   else if (find_extension(filestr, ".cod") || find_extension(filestr, ".amr") || find_extension(filestr, ".awb") || find_extension(filestr, ".bit") || find_extension(filestr, ".h264") || find_extension(filestr, ".h265") || find_extension(filestr, ".hevc")) fileType = ENCODED;
   else if (find_extension(filestr, ".txt")) fileType = TEXT;
   else if (find_extension(filestr, ".csv")) fileType = CSV;
   else if (find_extension(filestr, ".ber")) fileType = BER;
   else if (find_extension(filestr, ".gpx")) fileType = GPX;

   return fileType;
}


/* generate an output filename (i.e. one not entered on cmd line) as a combination of output names, or if not outputs then a combination of input names.  Needed for stream merging, possibly other output content types, CKJ Jul 2018 */

int GetOutputFilename(char* output_filename, int output_type_file, const char* output_type_content) {

char tmpstr[1024];
char *tmpptr1, *tmpptr2;
int i, ret_val = -1;

#ifndef _MEDIAMIN_

   if (output_type_file == WAV_AUDIO)
   {
      if (num_wav_outputs > 0)
      {
         for (i = 0; out_type[i] != WAV_AUDIO; i++);
         strcpy(tmpstr, MediaParams[i].Media.outputFilename);
         tmpptr1 = strrchr(tmpstr, '/');
         if (tmpptr1 != NULL) tmpptr1++;
         else tmpptr1 = tmpstr;
         tmpptr2 = strrchr(tmpstr, '.');
         *tmpptr2 = '\0';
         strcat(output_filename, tmpptr1);

         if (num_wav_outputs > 1)
         {
            i++;
            for (; out_type[i] != WAV_AUDIO; i++);
            strcpy(tmpstr, MediaParams[i].Media.outputFilename);
            tmpptr1 = strrchr(tmpstr, '/');
            if (tmpptr1 != NULL) tmpptr1++;
            else tmpptr1 = tmpstr;
            tmpptr2 = strrchr(tmpstr, '.');
            *tmpptr2 = '\0';
            strcat(output_filename, tmpptr1);
         }

         ret_val = i;
         goto content_label;
      }
      else if (num_pcap_outputs > 0)
      {
         for (i=0; out_type[i] != PCAP; i++);
         strcpy(tmpstr, MediaParams[i].Media.outputFilename);
         tmpptr1 = strrchr(tmpstr, '/');
         if (tmpptr1 != NULL) tmpptr1++;
         else tmpptr1 = tmpstr;
         tmpptr2 = strrchr(tmpstr, '.');
         *tmpptr2 = '\0';
         strcat(output_filename, tmpptr1);

         if (num_pcap_outputs > 1)
         {
            i++;
            for (; out_type[i] != PCAP; i++);
            strcpy(tmpstr, MediaParams[i].Media.outputFilename);
            tmpptr1 = strrchr(tmpstr, '/');
            if (tmpptr1 != NULL) tmpptr1++;
            else tmpptr1 = tmpstr;
            tmpptr2 = strrchr(tmpstr, '.');
            *tmpptr2 = '\0';
            strcat(output_filename, tmpptr1);
         }

         ret_val = i;
         goto content_label;
      }
   }
   else if (output_type_file == PCAP)
   {
      if (num_pcap_outputs > 0)
      {
         for (i=0; out_type[i] != PCAP; i++);
         strcpy(tmpstr, MediaParams[i].Media.outputFilename);
         tmpptr1 = strrchr(tmpstr, '/');
         if (tmpptr1 != NULL) tmpptr1++;
         else tmpptr1 = tmpstr;
         tmpptr2 = strrchr(tmpstr, '.');
         *tmpptr2 = '\0';
         strcat(output_filename, tmpptr1);

         if (num_pcap_outputs > 1)
         {
            i++;
            for (; out_type[i] != PCAP; i++);
            strcpy(tmpstr, MediaParams[i].Media.outputFilename);
            tmpptr1 = strrchr(tmpstr, '/');
            if (tmpptr1 != NULL) tmpptr1++;
            else tmpptr1 = tmpstr;
            tmpptr2 = strrchr(tmpstr, '.');
            *tmpptr2 = '\0';
            strcat(output_filename, tmpptr1);
         }

         ret_val = i;
         goto content_label;
      }
      else if (num_wav_outputs > 0)
      {
         for (i=0; out_type[i] != WAV_AUDIO; i++);
         strcpy(tmpstr, MediaParams[i].Media.outputFilename);
         tmpptr1 = strrchr(tmpstr, '/');
         if (tmpptr1 != NULL) tmpptr1++;
         else tmpptr1 = tmpstr;
         tmpptr2 = strrchr(tmpstr, '.');
         *tmpptr2 = '\0';
         strcat(output_filename, tmpptr1);

         if (num_wav_outputs > 1)
         {
            i++;
            for (; out_type[i] != WAV_AUDIO; i++);
            strcpy(tmpstr, MediaParams[i].Media.outputFilename);
            tmpptr1 = strrchr(tmpstr, '/');
            if (tmpptr1 != NULL) tmpptr1++;
            else tmpptr1 = tmpstr;
            tmpptr2 = strrchr(tmpstr, '.');
            *tmpptr2 = '\0';
            strcat(output_filename, tmpptr1);
         }

         ret_val = i;
         goto content_label;
      }
   }


   if (num_wav_outputs <= 0 && num_pcap_outputs <= 0)
#endif
   {
      i = 0;

      #pragma GCC diagnostic push  /* suppress "address of var will never be NULL" warnings in gcc 12.2; safe-coding rules prevail, JHB May 2023 */
      #pragma GCC diagnostic ignored "-Waddress"
      if (MediaParams[i].Media.inputFilename != NULL && strlen(MediaParams[i].Media.inputFilename)) {
      #pragma GCC diagnostic pop

         strcpy(tmpstr, MediaParams[i].Media.inputFilename);
         tmpptr1 = strrchr(tmpstr, '/');
         if (tmpptr1 != NULL) tmpptr1++;
         else tmpptr1 = tmpstr;
         tmpptr2 = strrchr(tmpstr, '.');
         *tmpptr2 = '\0';
         strcat(output_filename, tmpptr1);
      }

      #pragma GCC diagnostic push  /* suppress "address of var will never be NULL" warnings in gcc 12.2; safe-coding rules prevail, JHB May 2023 */
      #pragma GCC diagnostic ignored "-Waddress"
      if (MediaParams[i+1].Media.inputFilename != NULL && strlen(MediaParams[i+1].Media.inputFilename)) {
      #pragma GCC diagnostic pop

         i++;
         strcpy(tmpstr, MediaParams[i].Media.inputFilename);
         tmpptr1 = strrchr(tmpstr, '/');
         if (tmpptr1 != NULL) tmpptr1++;
         else tmpptr1 = tmpstr;
         tmpptr2 = strrchr(tmpstr, '.');
         *tmpptr2 = '\0';
         strcat(output_filename, tmpptr1);
      }

      ret_val = i;
      goto content_label;
   }

content_label:

   if (ret_val >= 0) {

      if (output_type_content && strlen(output_type_content) > 0) strcat(output_filename, output_type_content);
      else strcat(output_filename, "_");

      if (output_type_file == WAV_AUDIO) strcat(output_filename, ".wav");
      else if (output_type_file == PCAP) strcat(output_filename, ".pcap");
   }

   return ret_val;
}

/* read command line from /proc/self/cmdline, JHB Jan 2023 */

int GetCommandLine(char* cmdlinestr, int str_size) {

int i, ret = 0;

   FILE* fpCmdLine = fopen("/proc/self/cmdline", "rb");

   if (fpCmdLine) {

      ret = _fread(cmdlinestr, sizeof(char), str_size, fpCmdLine);  /* unknown as to why size must be 1 and count must be size, I got that from https://stackoverflow.com/questions/1406635/parsing-proc-pid-cmdline-to-get-function-parameters which mentions cmd line params are separated by NULLs. But still doesn't make sense since file is opened in binary mode. Maybe because OS forces the file to open in text mode. As for _fread() wrapper see comment in mediaTest.h */

      fclose(fpCmdLine);

      for (i=0; i<ret; i++) if (cmdlinestr[i] == 0) cmdlinestr[i] = ' ';  /* replace NULLs with spaces */
   }

   return ret;
}
