/*
 $Header: /root/Signalogic/apps/mediaTest/cmd_line_interface.c

 Copyright (C) Signalogic Inc. 2018-2023
 
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
   Modified Jan 2020 JHB, add nRepeat to supper number of repeat times cmd line entry (-RN)
   Modified Jan 2021 JHB, change references to AUDIO_FILE_TYPES to IS_AUDIO_FILE_TYPE, initialize outFileType2, USBAudioInput, and USBAudioOutput, allow pcap for output file type, add char szSDPFile[CMDOPT_MAX_INPUT_LEN]
   Modified Dec 2021 JHB, make debugMode 64-bit int
   Modified Mar 2022 JHB, add GPX file input handling, -Fn flag for mediaTest gpx processing
   Modified Aug 2022 JHB, adjust declaration of a few global vars to allow no_mediamin and no_pktlib options in mediaTest build (look for run, frame_mode, etc vars)
   Modified Dec 2022 JHB, add char szStreamGroupOutputPath[CMDOPT_MAX_INPUT_LEN]
   Modified Dec 2022 JHB, move sig_lib_event_log_filename here from packet_flow_media_proc.c
   Modified Jan 2023 JHB, in ctrl-c event handler call DSConfigLogging() with DS_PKTLOG_ABORT flag and set fCtrl_C_pressed. Don't set run = 0 for mediaMin
   Modified Jan 2023 JHB, add szAppFullCmdLine var and GetCommandLine()
   Modified May 2023 JHB, suppress "address of var will never be NULL" warnings in gcc 12.2; safe-coding rules prevail
   Modified May 2023 JHB, add timeScale and convert RealTimeInterval[] to float to support FTRT and AFAP modes, add uPortList[], add uLoopbackDepth
   Modified Jun 2023 JHB, initialize timeScale to zero to allow packet_float_media_proc() to know if an app has already set timeScale
*/


/* system header files */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

/* Signalogic header files */

#include "test_programs.h"   /* misc program support (command line entry, etc) */
#include "minmax.h"

/* mediaTest definitions (also includes DirectCore library header and host/coCPU shared header files) */

#include "mediaTest.h"

#if defined(_ALSA_INSTALLED_)
  #include "aviolib.h"
#endif

#include "diaglib.h"  /* event log and packet log APIs. Currently we only use DSConfigLogging() here, JHB Jan 2023 */

/* global vars filled by cimGetCmdLine() */

PLATFORMPARAMS PlatformParams = {{ 0 }};
MEDIAPARAMS MediaParams[MAXSTREAMS];
float RealTimeInterval[MAX_INPUT_STREAMS] = { 0 };

/* global vars referenced in mediaMin.c, x86_mediaTest.c, and packet_flow_media_proc.c */

volatile int8_t  pm_run = 1;  /* may be cleared by application signal handler to stop packet / media processing loops */
unsigned int     inFileType, outFileType, outFileType2 = 0, USBAudioInput = 0, USBAudioOutput = 0;
char             executionMode[2] = { 'a', 'c' }; /* default is app execution mode, cmd line */
int64_t          debugMode = 0;
int              performanceMeasurementMode = 0;
int              nReuseInputs = 0;
int              nSegmentation = 0;
int              nSegmentInterval = 0;
int              nAmplitude = 0;
int              nJitterBufferParams = 0;
int              nRepeat = 0;
char             szSDPFile[CMDOPT_MAX_INPUT_LEN] = "";
int              nSamplingFrequency;  /* rate of gps point recording in gpx file processing, Mar 2022 JHB */
char             szStreamGroupOutputPath[CMDOPT_MAX_INPUT_LEN] = "";
#ifndef __LIBRARYMODE__  /* this group declared here in non-library build, otherwise declared in packet_flow_media_proc.c, Aug 2022 JHB */
volatile char    pktStatsLogFile[CMDOPT_MAX_INPUT_LEN] = "";
volatile int     send_sock_fd = -1, send_sock_fd_ipv6 = -1;
volatile bool    frame_mode = false, use_bkgnd_process = false, use_log_file = false;
#endif
char             sig_lib_event_log_filename[] = { "sig_lib_event_log.txt" };  /* moved here from packet_flow_media_proc.c, JHB Dec 2022 */
bool             fCtrl_C_pressed = false;
char             full_cmd_line[MAX_CMDLINE_STR_LEN] = "";  /* app full command line, filled in by cmdLineInterface(), which calls GetCommandLine(). Note that mediaTest.h publishes this as this as const char* szAppFullCmdLine, JHB Jan 2023 */
double           timeScale = 0;  /* support FTRT and AFAP modes, see comments in packet_flow_media_proc.c */
uint16_t         uPortList[MAX_CONCURRENT_STREAMS] = { 0 };
uint8_t          uLookbackDepth = 1;

/* global vars set in packet_flow_media_proc, but only visible within an app build (not exported from a lib build) */

#ifndef _MEDIAMIN_
extern int     nInFiles, nOutFiles, numStreams, num_wav_outputs, num_pcap_outputs;
extern int     out_type[MAX_SESSIONS];
#endif

/* ctrl-c signal handler */

void intHandler(int sig) {

   #ifndef _MEDIAMIN_  /* mediaMin has orderly cleanup so don't do any shortcuts, JHB Jan 2023 */
   pm_run = 0;  /* global var in packet/media thread processing (packet_flow_media_proc.c) */
   #endif
   
   fCtrl_C_pressed = true;

   DSConfigLogging(DS_CONFIG_LOGGING_SET_FLAG, DS_PKTLOG_ABORT | DS_CONFIG_LOGGING_ALL_THREADS, NULL);  /* tell possibly time-consuming packet logging functions to abort. Currently we combine with "ALL_THREADS" flag which will terminate packet logging for any apps running. See additional comments in mediaMin.cpp JHB Jan 2023 */
}

/* global vars used by other files in the build */

/* operating mode flags (xxx_test flags):  Notes:

  1) These flags are set either (i) per mode value (-M cmd line option, see comments above), or (ii) if no mode option is given then determined by cmd line input/output options (-i and -o).  For example, codec_test is set if both input and output files are specified on the cmd line
  
  2) Only one flag is set at one time
*/

char           network_packet_test = 0, cocpu_sim_test = 0, cocpu_network_test = 0, codec_test = 0, x86_frame_test = 0, x86_pkt_test = 0, pcap_extract = 0, gpx_process = 0;
unsigned int   CPU_mode = 0, programMode = 0;


/* local functions */

int get_file_type(const char*, unsigned int);


int cmdLineInterface(int argc, char **argv, unsigned int uFlags) {

UserInterface userIfs = {0, 0, 0, 0, "", "", false};

bool fAudioInputFile, fAudioOutputFile, fPcapInputFile, fCodedInputFile, fCodedOutputFile, fPcapOutputFile, fGpxInputFile;
bool __attribute__ ((unused)) fTextOutputFile, fCSVOutputFile;  /* added Nov 2019 JHB */
unsigned int cim_uFlags;
int i;

   GetCommandLine((char*)szAppFullCmdLine, MAX_CMDLINE_STR_LEN);  /* save full command in szAppFullCmdLine global var, for use by apps as needed, JHB Jan 2023 */

/* Process command line, results are in PlatformParams, MediaParms, and userIfs structs */

   cim_uFlags = CIM_GCL_DISABLE_MANDATORIES | CIM_GCL_SUPPRESS_STREAM_MSGS | CIM_GCL_FILLUSERIFS;
   if (uFlags & CLI_MEDIA_APPS) cim_uFlags |= CIM_GCL_MED;
   if (uFlags & CLI_MEDIA_APPS_MEDIAMIN) cim_uFlags |= CIM_GCL_MEDIAMIN;
   
   cimGetCmdLine(argc, argv, &userIfs, cim_uFlags, &PlatformParams, &MediaParams);  /* run first with mandatories disabled, print-outs disabled */

   if (userIfs.programMode != LOG_FILE_DIAGNOSTICS) {  /* most cmd line arguments are ignored for log file diagnostics */

      cim_uFlags = CIM_GCL_SUPPRESS_STREAM_MSGS | CIM_GCL_FILLUSERIFS | CIM_GCL_DEBUGPRINT;
      if (uFlags & CLI_MEDIA_APPS) cim_uFlags |= CIM_GCL_MED;
      if (uFlags & CLI_MEDIA_APPS_MEDIAMIN) cim_uFlags |= CIM_GCL_MEDIAMIN;

      if (!cimGetCmdLine(argc, argv, &userIfs, cim_uFlags, &PlatformParams, &MediaParams)) return 0;  /* run again with everything enabled, report cmd line errors */
   }

   programMode = userIfs.programMode;
   
   if (programMode == LOG_FILE_DIAGNOSTICS) return 1;


/* Check card designator and enable CPU and coCPU mode */

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

   codec_test = ((fAudioInputFile || USBAudioInput || fCodedInputFile) && (fAudioOutputFile || USBAudioOutput || fCodedOutputFile || fPcapOutputFile));  /* codec mode = both I/O audio or compressed bitstream of some type (also includes output pcap) */
   pcap_extract = (fPcapInputFile && fCodedOutputFile);
   gpx_process = (fGpxInputFile != 0);

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

         if (x86_frame_test && (fPcapInputFile || fPcapOutputFile)) frame_mode = true;  /* added frame mode with pcap input (payload-only processing, no jitter buffer).  This causes a variation of x86_pkt_test code to run, Jun2017 */
      }
   }

   use_bkgnd_process = userIfs.programSubMode == 2;  /* programSubMode is 2 if 'b' suffix was entered after -M value; see apps/common/getUserInterface.cpp */

   if (strlen(userIfs.logFile[0]) && !strstr(userIfs.logFile[0], "-nopktlog") && !strstr(userIfs.logFile[0], "-nopacketlog")) {

      use_log_file = true;
      if (!strstr(userIfs.logFile[0], "[default]")) strcpy((char*)pktStatsLogFile, userIfs.logFile[0]);  /* if default entry (only -L entered with no path+filename) then just set the var, don't copy the string, JHB Sep2017 */
   }

   for (int i=0; i<min(MAXSTREAMS, MAX_INPUT_STREAMS); i++) RealTimeInterval[i] = MediaParams[i].Media.frameRate;  /* store -rN frame rate cmd line entries in RealTimeInterval[], JHB May 2023 */

   executionMode[0] = userIfs.executeMode;  /* execution mode used by mediaTest to specify app, thread, and cmd line execution modes */

   debugMode = userIfs.debugMode;  /* debug mode used by mediaMin to control various test/debug scenarios */

   performanceMeasurementMode = (int)userIfs.uPerformanceMeasurement;
   nReuseInputs = userIfs.nReuseInputs;
   nSegmentation = userIfs.nSegmentation;
   nSegmentInterval = userIfs.nInterval;
   nAmplitude = userIfs.nAmplitude;
   nJitterBufferParams = userIfs.nJitterBufferOptions;  /* lsbyte is target delay, next byte is max delay, JHB Dec2019 */
   nRepeat = (int)userIfs.nRepeatTimes;  /* -1 = no entry (no repeat), 0 = repeat forever, > 1 is repeat number of times, JHB Jan2020 */
   if (strlen(userIfs.szSDPFile)) strcpy(szSDPFile, userIfs.szSDPFile);
   nSamplingFrequency = userIfs.nSamplingFrequency;  /* sampling frequency for gpx processing */
   if (strlen(userIfs.szStreamGroupOutputPath)) strcpy(szStreamGroupOutputPath, userIfs.szStreamGroupOutputPath);

   for (i=0; i<MAX_CONCURRENT_STREAMS; i++) uPortList[i] = userIfs.dstUdpPort[i];  /* fill in port list, JHB May 2023 */

   uLookbackDepth = userIfs.nLookbackDepth;

/* register signal handler to catch Ctrl-C signal and cleanly exit mediaTest, mediaMin, and other test programs */

#if 1  /* disable this if needed for use with gdb (to prevent mediaTest or mediaMin from handling ctrl-c), JHB Jan 2019 */
   struct sigaction act = {0};
   act.sa_handler = intHandler;
   sigaction(SIGINT, &act, NULL);
#endif

   return 1;
}


int get_file_type(const char* filestr, unsigned int io) {

int fileType = 0;
char tmpstr[1024];

   strcpy(tmpstr, filestr);
   strupr(tmpstr);

#if defined(_ALSA_INSTALLED_)
   if (!strcmp(tmpstr, "USB0")) {

      fileType = USB_AUDIO;

      if (!io) USBAudioInput = AUDIO_INPUT_USB0;  /* initial USB audio supported, add other devices later.  Sampling rate, bitwidth, number of channels, etc are in session config files.  JHB Mar2018 */
      else USBAudioOutput = AUDIO_OUTPUT_USB0;
   }
   else if (!strcmp(tmpstr, "USB1")) {

      fileType = USB_AUDIO;

      if (!io) USBAudioInput = AUDIO_INPUT_USB1;
      else USBAudioOutput = AUDIO_OUTPUT_USB1;
   }
   else
#endif

   if (strstr(tmpstr, ".PCAP")) fileType = PCAP;
   else if (strstr(tmpstr, ".INP") || strstr(tmpstr, ".OUT") || strstr(tmpstr, ".PCM") || strstr(tmpstr, ".RAW") || strstr(tmpstr, ".SAM")) fileType = RAW_AUDIO;
   else if (strstr(tmpstr, ".TIM")) fileType = TIM_AUDIO;
   else if (strstr(tmpstr, ".AU")) fileType = AU_AUDIO;
   else if (strstr(tmpstr, ".WAV")) fileType = WAV_AUDIO;
   else if (strstr(tmpstr, ".COD") || strstr(tmpstr, ".AMR") || strstr(tmpstr, ".AWB") || strstr(tmpstr, ".BIT")) fileType = ENCODED;
   else if (strstr(tmpstr, ".TXT")) fileType = TEXT;
   else if (strstr(tmpstr, ".CSV")) fileType = CSV;
   else if (strstr(tmpstr, ".BER")) fileType = BER;
   else if (strstr(tmpstr, ".GPX")) fileType = GPX;

   return fileType;
}


/* generate an output filename (i.e. one not entered on cmd line) as a combination of output names, or if not outputs then a combination of input names.  Needed for stream merging, possibly other output content types, CKJ Jul 2018 */

int GetOutputFilename(char* output_filename, int output_type_file, const char* output_type_content) {

char tmpstr[1024];
char *tmpptr1, *tmpptr2;
int ret_val = -1;
int i;

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
         for (i = 0; out_type[i] != PCAP; i++);
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
         for (i = 0; out_type[i] != PCAP; i++);
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

int ret = 0;

   FILE* fpCmdLine = fopen("/proc/self/cmdline", "rb");

   if (fpCmdLine) {

      ret = _fread(cmdlinestr, sizeof(char), str_size, fpCmdLine);  /* unknown as to why size must be 1 and count must be size, I got that from https://stackoverflow.com/questions/1406635/parsing-proc-pid-cmdline-to-get-function-parameters which mentions cmd line params are separated by NULLs. But still doesn't make sense since file is opened in binary mode. Maybe because OS forces the file to open in text mode. As for _fread() wrapper see comment in mediaTest.h */

      fclose(fpCmdLine);

      for (int i=0; i<ret; i++) if (cmdlinestr[i] == 0) cmdlinestr[i] = ' ';  /* replace NULLs with spaces */
   }

   return ret;
}
