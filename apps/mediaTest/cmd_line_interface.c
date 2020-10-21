/*
 $Header: /root/Signalogic/apps/mediaTest/cmd_line_interface.c

 Copyright (C) Signalogic Inc. 2018-2020
 
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
*/


/* system header files */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

/* Signalogic header files */

#include "test_programs.h"   /* misc program support (command line entry, etc) */

/* mediaTest definitions (also includes DirectCore library header and host/coCPU shared header files) */

#include "mediaTest.h"

#if defined(_ALSA_INSTALLED_)
  #include "aviolib.h"
#endif

/* global vars filled by cimGetCmdLine() */

PLATFORMPARAMS PlatformParams = {{ 0 }};
MEDIAPARAMS MediaParams[MAXSTREAMS];
unsigned int frameInterval[MAX_INPUT_STREAMS] = { 20 };

/* global vars set here */

unsigned int   inFileType, outFileType, outFileType2, USBAudioInput, USBAudioOutput;
char           executionMode[2] = { 'a', 'c' }; /* default is app execution mode, cmd line */
int            debugMode = 0;
int            performanceMeasurementMode = 0;
int            nReuseInputs = 0;
int            nSegmentation = 0;
int            nSegmentInterval = 0;
int            nAmplitude = 0;
int            nJitterBufferParams = 0;
int            nRepeat = 0;

/* global vars set in packet_flow_media_proc, but only visible within an app build (not exported from a lib build) */

#ifndef MEDIAMIN
extern int     nInFiles, nOutFiles, numStreams, num_wav_outputs, num_pcap_outputs;
extern int     out_type[MAX_SESSIONS];
#endif

/* ctrl-c control variable and signal handler */
void intHandler(int sig) {
   run = 0;  /* global var in packet/media thread processing (packet_flow_media_proc.c) */
}

/* global vars used by other files in the build */

/* operating mode flags (xxx_test flags):  Notes:

  1) These flags are set either (i) per mode value (-M cmd line option, see comments above), or (ii) if no mode option is given then determined by cmd line input/output options (-i and -o).  For example, codec_test is set if both input and output files are specified on the cmd line
  
  2) Only one flag is set at one time
*/

char           network_packet_test = 0, cocpu_sim_test = 0, cocpu_network_test = 0, codec_test = 0, x86_frame_test = 0, x86_pkt_test = 0, pcap_extract = 0;
unsigned int   CPU_mode = 0, programMode = 0;


/* local functions */

int get_file_type(const char*, unsigned int);


int cmdLineInterface(int argc, char **argv, unsigned int uFlags) {

UserInterface userIfs = {0, 0, 0, 0, "", "", false};

bool fAudioInputFile, fAudioOutputFile, fPcapInputFile, fCodedInputFile, fCodedOutputFile, fPcapOutputFile;
bool __attribute__ ((unused)) fTextOutputFile, fCSVOutputFile;  /* added Nov 2019 JHB */

unsigned int cim_uFlags;

/* Process command line, results are in PlatformParams, MediaParms, and userIfs structs */

   cim_uFlags = CIM_GCL_DISABLE_MANDATORIES | CIM_GCL_SUPPRESS_STREAM_MSGS | CIM_GCL_FILLUSERIFS;
   if (uFlags & CLI_MEDIA_APPS) cim_uFlags |= CIM_GCL_MED;
   
   cimGetCmdLine(argc, argv, &userIfs, cim_uFlags, &PlatformParams, &MediaParams);  /* run first with mandatories disabled, print-outs disabled */

   if (userIfs.programMode != LOG_FILE_DIAGNOSTICS) {  /* most cmd line arguments are ignored for log file diagnostics */

      cim_uFlags = CIM_GCL_SUPPRESS_STREAM_MSGS | CIM_GCL_FILLUSERIFS | CIM_GCL_DEBUGPRINT;
      if (uFlags & CLI_MEDIA_APPS) cim_uFlags |= CIM_GCL_MED;

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
   fAudioInputFile = AUDIO_FILE_TYPES(inFileType);
   
   outFileType = get_file_type(MediaParams[0].Media.outputFilename, 1);

   fCodedOutputFile = outFileType == ENCODED;
   fPcapOutputFile = outFileType == PCAP;
   fAudioOutputFile = AUDIO_FILE_TYPES(outFileType);

   fTextOutputFile = outFileType == TEXT;
   fCSVOutputFile = outFileType == CSV;

   if (fAudioOutputFile) {
   
      outFileType2 = get_file_type(MediaParams[1].Media.outputFilename, 1);
      if (outFileType2 == USB_AUDIO) outFileType |= USB_AUDIO;
   }
   else if (outFileType == USB_AUDIO) {
   
      outFileType2 = get_file_type(MediaParams[1].Media.outputFilename, 1);

      if (AUDIO_FILE_TYPES(outFileType2)) {
         fAudioOutputFile = true;
         outFileType |= outFileType2;
      }
   }

   codec_test = ((fAudioInputFile || USBAudioInput || fCodedInputFile) && (fAudioOutputFile || USBAudioOutput || fCodedOutputFile));  /* codec mode = both I/O audio or compressed bitstream of some type (not pcap) */
   pcap_extract = (fPcapInputFile && fCodedOutputFile);

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

   if (strlen(userIfs.logFile[0])) {

      use_log_file = true;
      if (!strstr(userIfs.logFile[0], "[default]")) strcpy((char*)pktStatsLogFile, userIfs.logFile[0]);  /* if default entry (only -L entered with no path+filename) then just set the var, don't copy the string, JHB Sep2017 */
   }

   executionMode[0] = userIfs.executeMode;  /* execution mode used by mediaTest to specify app, thread, and cmd line execution modes */

   debugMode = userIfs.debugMode;  /* debug mode used by mediaMin to control various test/debug scenarios */

   performanceMeasurementMode = (int)userIfs.uPerformanceMeasurement;
   nReuseInputs = userIfs.nReuseInputs;
   nSegmentation = userIfs.nSegmentation;
   nSegmentInterval = userIfs.nInterval;
   nAmplitude = userIfs.nAmplitude;
   nJitterBufferParams = userIfs.nJitterBufferOptions;  /* lsbyte is target delay, next byte is max delay, JHB Dec2019 */
   nRepeat = (int)userIfs.nRepeatTimes;  /* -1 = no entry (no repeat), 0 = repeat forever, > 1 is repeat number of times, JHB Jan2020 */

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

   return fileType;
}


/* generate an output filename (i.e. one not entered on cmd line) as a combination of output names, or if not outputs then a combination of input names.  Needed for stream merging, possibly other output content types, CKJ Jul 2018 */

int GetOutputFilename(char* output_filename, int output_type_file, const char* output_type_content) {

char tmpstr[1024];
char *tmpptr1, *tmpptr2;
int ret_val = -1;
int i;

#ifndef MEDIAMIN

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
      if (MediaParams[i].Media.inputFilename != NULL && strlen(MediaParams[i].Media.inputFilename)) {

         strcpy(tmpstr, MediaParams[i].Media.inputFilename);
         tmpptr1 = strrchr(tmpstr, '/');
         if (tmpptr1 != NULL) tmpptr1++;
         else tmpptr1 = tmpstr;
         tmpptr2 = strrchr(tmpstr, '.');
         *tmpptr2 = '\0';
         strcat(output_filename, tmpptr1);
      }

      if (MediaParams[i+1].Media.inputFilename != NULL && strlen(MediaParams[i+1].Media.inputFilename)) {

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
