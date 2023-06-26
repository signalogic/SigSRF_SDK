/*
 $Header: /root/Signalogic_2012v4/DirectCore/apps/common/getUserInterface.cpp

 Purpose: Defines help menu and collects command line options

 Copyright (C) Signalogic Inc. 1992-2023

 Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Revision History

   Created, 10/11/05 2:40p Nithin Rajagopal
   Modified 7 Aug 2013 JG, added implementation for accepting the order 'n' for the FFT
   Modified 6 Aug 2014 JG, modifying the description of command line arguments to bring up to date
   Modified Nov 2014 JHB, updating to match changes in userInfo.h (cardDesignator, processorClockrate, and numCores), updated for v5 release and new documentation
   Modified Nov 2014 JHB, fix some naming to support multiple targets, unify/consolidate command line params for all test programs
   Modified Dec 2014 JHB, added support for IP addr, UDP port, and MAC addr entry (e.g. -Daa.bb.cc.dd:port:aa-bb-cc-dd-ee-ff-gg)
   Modified Dec 2015 JHB, added support for multiple instances of some cmd line entries (initially to support concurrent video streams)
   Modified Jul 2015 JHB, added -l and -t cmd lne options (lib configuration and core task assignments) to support multifunctional .out file, e.g. image analytics
   Modified Sep 2017 JHB, added -L entry for log file
   Modified Mar 2018 JHB, for userIfs summary display, print "Clock = N MHz" only if clock rate is non-zero, otherwise print "Clock = default"
   Modified Jul 2018 JHB, add -Ex entry for execution mode (-Ec cmd line, -Et thread, -Ep process)
   Modified Jul 2018 JHB, apply separate MANDATORY and MANDATORY_COCPU attributes
   Modified Feb 2019 JHB, add -nN for -nReuseInputs, overloaded on nFFTOrder.  This is for input re-use by mediaMin and mediaTest applications
   Modified Jul 2019 JHB, add -sN for nSegmentation, overloaded on inputType, and -AN for amplitude, overloaded on baseAddr, in userInfo.h.  This is to support segmentation, including silence detection, strip, and chunk rewrite functionality in mediaTest (added for Kaldi group users).  Should be no impact on -sN for streaming mode, used in iaTest
   Modified Sep 2019 JHB, change include order (move up iostream.h and iomanip.h) to fix -Wodr (one definition rule) warning with gcc 5.4
   Modified Sep 2019 JHB, check uFlags for CLI_MEDIA_APPS flag to handle input re-use, audio segmentation, and other cmd line options specific to mediaMin and mediaTest apps.  NB - because getUserInfo() is used by both apps and shared libs (e.g. cimlib.so) need to avoid build-time hard-codes.  Anything app-specific should be controlled by uFlags to allow run-time decisions
   Modified Dec 2019 JHB, add -jN cmd line entry for jitter buffer params (for mediaMin app usage), sets nJitterBufferOptions in userIfs (UserInterface struct in userInfo.h)
   Modified Jan 2020 JHB, add -RN cmd line entry for repeat number of times
   Modified Jan 2021 JHB, add -s option specific to mediaMin to allow SDP file input on cmd line
   Modified Dec 2021 JHB, make -d option (mode/debug flags) 64-bit integer
   Modified Feb 2022 JHB, in getUserInfo() display summary, show x86 clock rate instead of "Default", if coCPU executable file not used show "N/A" instead of nothing
   Modified Mar 2022 JHB, add -Fn flag for mediaTest gpx processing
   Modified Apr 2022 JHB, further clarify getUserInfo() display, Core List --> coCPU Core List and Clock --> coCPU Clock if applicable
   Modified Aug 2022 JHB, more readability mods to getUserInfo() display, including a lock to keep program start info coherent within multiple threads
   Modified Dec 2022 JHB, add -g option specific to mediaMin to allow stream group wav output path cmd line input
   Modified May 2023 JHB, support FLOAT option type in getUserInfo(), change 'r' (frame rate) option to float, add 'p' option to allow portList[] entry, for media applications allow -l entry to specify nLookbackDepth by overlaying library flags (libFlags)
*/

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <semaphore.h>

#include "test_programs.h"
#include "cmdLineOpt.h"
#include "alias.h"

static sem_t cout_sem;
static bool fSem_Init = false;

using namespace std;

CmdLineOpt::Record options[] = {

   {'c', CmdLineOpt::STRING, MANDATORY,
          (char *)"Platform / card designator (e.g. -cx86 or -cSIGC66XX)" },
   {'f', CmdLineOpt::INTEGER, MANDATORY_COCPU,
          (char *)"CPU clock frequency in MHz (e.g. -f1000)", {{(void*)1000}} },
   {'m', CmdLineOpt::INT64, MANDATORY_COCPU,
          (char *)"Core select bit mask. (e.g. -m1, means core0, -m2 means core1, -m3 means core0 and core1.  For some programs only one core can be selected at a time)" },
   {'e', CmdLineOpt::STRING, MANDATORY_COCPU,
          (char *)"coCPU executable file name (e.g. -efilename.out). File must be in ELF or COFF format" },
	{'i', CmdLineOpt::STRING, NOTMANDATORY,
          (char *)"Input file including path if applicable (e.g. -ifilename.pcap, -ifilename.wav, -ifilename.yuv, etc)" },
	{'o', CmdLineOpt::STRING, NOTMANDATORY,
          (char *)"Output file including path if applicable (e.g. -ofilename.pcap, -ofilename.wav, -ofilename.yuv, etc)" },
	{'C', CmdLineOpt::STRING, NOTMANDATORY,
          (char *)"Configuration file (e.g. -Csession_config/filename for mediaTest program)" },
	{'L', CmdLineOpt::STRING, NOTMANDATORY,
          (char *)"Log filename including path if applicable.  Entering only -L uses a default log filename", {{(void*)"[default]"}} },
   {'T', CmdLineOpt::BOOLEAN, NOTMANDATORY,
          (char *)"Run with talker enabled" },
   {'l', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Lookback depth for de-duplication, or library flags, used to control which libraries are configured in target CPU code" },
   {'t', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Task assignment core lists (e.g. -tN:N:N to define core lists for input, output, and logging for CPU0)" },
   {'A', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Set IO base address in Hex (used only for PC104 boards), (e.g. -A320)" },
   {'v', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Run in verbose mode, enter as -vN where N sets debug info level (0 = default, or none)" },
   {'h', CmdLineOpt::BOOLEAN, NOTMANDATORY,
          (char *)"Display this help list" },

	/* Algorithm flag */ 

	{'a', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Algorithm flag (-a0 for parallel FFT, -a1 for serial FFT, -a10 for Cryptographic Algorithm)", {{(void*)-1}} },

   /* Mode -- general program operating mode flag */

	{'M', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Operating mode for most programs (enter as -MN, where N is mode value)", {{(void*)-1}} },

   /* Execute Mode -- execute as cmd line, process, or thread (if not entered, cmd line is default) */

	{'E', CmdLineOpt::CHAR, NOTMANDATORY,
          (char *)"Execute mode (-Ea, -Ep, -Et for cmd line (default), process, or thread", {{(void*)'a'}} },

   /* FFT test program flags */
  
   {'n', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"FFT order (e.g. -n8 for order 8, default = 6), or input re-use option for mediaMin (default = 0)",  /* modified Feb 2019, JHB */
          {{(void*)6}} },

   {'I', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Interval for audio segmentation, or input FFT data waveform (-I0 for ramp, -I1 for impulse, default is ramp)" },

   /* video and audio streaming flags */
  
   {'x', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"x resolution (e.g. -x1920 for 1920 video width)", {{(void*)0}} },
   {'y', CmdLineOpt::INTEGER, NOTMANDATORY, 
          (char *)"y resolution (e.g. -x1080 for 1080 video height)", {{(void*)0}} },
	{'s', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Segmentation for mediaTest audio, streaming mode for streamTest (e.g. -s0 for oneshot, -s1 for continuous)", {{(void*)0}} },
	{'s', CmdLineOpt::STRING, NOTMANDATORY,
          (char *)"sdp file input for mediaMin" },
	{'r', CmdLineOpt::FLOAT, NOTMANDATORY,
          (char *)"Frame rate in frames per sec (default is 30 fps), or buffer add interval in msec (default is 20 msec)", {{(void*)-1}} },  /* changed to -1.  cimlib will set the default to 30 for streamTest and iaTest, JHB Sep2017 */
   {'D', CmdLineOpt::IPADDR, NOTMANDATORY,
          (char *)"Destination IP addr and port, in format aa.bb.cc.dd[:port][:mm-mm-mm-mm-mm-mm]", {{(void*)0}}, {0}, {0} },
   {'S', CmdLineOpt::IPADDR, NOTMANDATORY,
          (char *)"Source IP addr and port, in format aa.bb.cc.dd[:port][:mm-mm-mm-mm-mm-mm]", {{(void*)0}}, {0}, {0} },
   {'B', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Bit rate, in bps (default is 800 kbps)", {{(void*)800000}} },
   {'V', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Video configuration (e.g. -VN:N:N (e.g. -VN1:N2:N3 to set video profile to N1, bitrate config to N2, and interframe config to N3)", {{(void*)0}}, {0}, {0} },
	{'j', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Jitter buffer parameters, lower 8 bits is target delay, next 8 bits is max delay (in number of packets)", {{(void*)-1}} },
	{'R', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Repeat number of times", {{(void*)-1}} },
	{'g', CmdLineOpt::STRING, NOTMANDATORY,
          (char *)"stream group output path" },  /* added JHB Dec 2022 */
	{'p', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"UDP or TCP port" },  /* added JHB May 2023 */

   /* gpx processing flags */

   {'F', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Sampling frequency in Hz (e.g. -F8000)", {{(void*)-1}} },  /* initialize to -1 so programs can tell if -Fn has been given on cmd line */

	/* Scrypt test program flags */ 

	{'P', CmdLineOpt::STRING, NOTMANDATORY,
          (char *)"Scrypt algorithm Passphrase" },
	{'S', CmdLineOpt::STRING, NOTMANDATORY,
          (char *)"Scrypt algorithm Salt" },
	{'U', CmdLineOpt::BOOLEAN, NOTMANDATORY,
          (char *)"Scrypt test program runs in user mode" },
	{'E', CmdLineOpt::BOOLEAN, NOTMANDATORY,
          (char *)"Scrypt algorithm mode encode" },
	{'D', CmdLineOpt::BOOLEAN, NOTMANDATORY,
          (char *)"Scrypt algorithm mode decode" },
	{'s', CmdLineOpt::BOOLEAN, NOTMANDATORY,
          (char *)"Scrypt"},
	{'N', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Scrypt algorithm N parameter" },
	{'r', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Scrypt algorithm r parameter" },
	{'p', CmdLineOpt::INTEGER, NOTMANDATORY,
          (char *)"Scrypt algorithm p parameter", {{(void*)0}} },
	{'d', CmdLineOpt::INT64, NOTMANDATORY,
          (char *)"Debug mode for most programs (enter as -dN, where N is mode value). dkLen parameter for Scrypt Algorithm test program", {{(void*)-1}} },
};

/* global storage of cmd line options */

CmdLineOpt cmdOpts( options, sizeof(options)/sizeof(options[0]) );

int getUserInfo(int argc, char* argv[], UserInterface* userIfs, unsigned int uFlags) {

int instances, i;
char labelstr[CMDOPT_MAX_INPUT_LEN];
char tmpstr[CMDOPT_MAX_INPUT_LEN];
char clkstr[100];

/* int rc = EXIT_FAILURE; - unused variable CJ 4/25/17 */

   if (userIfs != NULL) {

      if (!cmdOpts.scanOptions(argc, argv, uFlags)) {
         cout << "Please use the above options" << endl;
         exit(1);
      }
      else {

         if (cmdOpts.nInstances('h')) {
            cmdOpts.printOptions();
            return EXIT_SUCCESS;
         }   

         userIfs->numCoresPerCPU = 0;  /* not currently assigned command-line syntax yet */

      /* common test program flags */

         if (uFlags & CLI_MEDIA_APPS) {
            if (cmdOpts.nInstances('A')) userIfs->nAmplitude = cmdOpts.getInt('A', 0, 0);
         }
         else {
            if (cmdOpts.nInstances('A')) userIfs->baseAddr = cmdOpts.getInt('A', 0, 0);
         }   

         if (cmdOpts.nInstances('f')) userIfs->processorClockrate = cmdOpts.getInt('f', 0, 0);

         if (cmdOpts.nInstances('F')) userIfs->nSamplingFrequency = cmdOpts.getInt('F', 0, 0);  /* mediaTest app gps sampling frequency, JHB Mar2022 */

         if (cmdOpts.nInstances('m')) userIfs->coreBitMask = cmdOpts.getInt64('m', 0);

         if (cmdOpts.nInstances('e')) strncpy(userIfs->targetFileName, cmdOpts.getStr('e', 0), CMDOPT_MAX_INPUT_LEN);

         if (cmdOpts.nInstances('c')) strncpy(userIfs->cardDesignator, cmdOpts.getStr('c', 0), CMDOPT_MAX_INPUT_LEN);

         if ((instances = cmdOpts.nInstances('i')))
           for (i=0; i<instances; i++) {
              if (cmdOpts.getStr('i', i) != NULL) strncpy(userIfs->inputFile[i], cmdOpts.getStr('i', i), CMDOPT_MAX_INPUT_LEN);  
           }
         
         if ((instances = cmdOpts.nInstances('o')))
           for (i=0; i<instances; i++) {
              if (cmdOpts.getStr('o', i) != NULL) strncpy(userIfs->outputFile[i], cmdOpts.getStr('o', i), CMDOPT_MAX_INPUT_LEN);
           }

         if ((instances = cmdOpts.nInstances('C')))
           for (i=0; i<instances; i++) {
              if (cmdOpts.getStr('C', i) != NULL) strncpy(userIfs->configFile[i], cmdOpts.getStr('C', i), CMDOPT_MAX_INPUT_LEN);
           }

         if ((instances = cmdOpts.nInstances('L')))
           for (i=0; i<instances; i++) {
              if (cmdOpts.getStr('L', i) != NULL) strncpy(userIfs->logFile[i], cmdOpts.getStr('L', i), CMDOPT_MAX_INPUT_LEN);
           }

         userIfs->algorithmIdNum = cmdOpts.getInt('a', 0, 0);  /* always call in order to get default value */

         if (cmdOpts.nInstances('l')) userIfs->libFlags = cmdOpts.getInt('l', 0, 0);  /* get library flags, default value is zero */
         else if (CLI_MEDIA_APPS) userIfs->libFlags = 1;  /* for media apps, nLookbackDepth overlays libFlags, and we want a default of 1 for RFC7198 de-duplication, JHB May 2023 */

         if (cmdOpts.nInstances('t')) {

            userIfs->taskAssignmentCoreLists = (uint64_t)cmdOpts.getInt('t', 0, 0);  /* get core task assignments default values are core zero for all tasks */
            userIfs->taskAssignmentCoreLists |= (uint64_t)cmdOpts.getInt('t', 0, 1) << 8;
            userIfs->taskAssignmentCoreLists |= (uint64_t)cmdOpts.getInt('t', 0, 2) << 16;
         }
         else userIfs->taskAssignmentCoreLists = (uint64_t)-1;  /* indicate no cmd line entry, JHB Sep 2018 */

         if (cmdOpts.nInstances('T')) userIfs->enableTalker = true;

         if (cmdOpts.nInstances('v')) userIfs->verbose = cmdOpts.getInt('v', 0, 0);  /* get verbose/debug level, default value is zero */

      /* FFT test program flags */ 

         if (uFlags & CLI_MEDIA_APPS) {
            if (cmdOpts.nInstances('n')) userIfs->nReuseInputs = cmdOpts.getInt('n', 0, 0);
            else userIfs->nReuseInputs = 0;  /* override default value 6, used by FFT order input */
         }
         else {
            userIfs->fftOrder = cmdOpts.getInt('n', 0, 0);  /* always call to get default value */
         }

         if (uFlags & CLI_MEDIA_APPS) {
            userIfs->nInterval = cmdOpts.getInt('I', 0, 0);  /* added for mediaTest segmentation, including silence detection, strip, chunk rewrite, etc.  JHB Jul 2019 */
         }
         else {
            if (cmdOpts.nInstances('I')) userIfs->inputType = cmdOpts.getInt('I', 0, 0);
         }

      /* video streaming test program flags */ 

         if ((instances = cmdOpts.nInstances('x')))
           for (i=0; i<instances; i++) userIfs->xres[i] = cmdOpts.getInt('x', i, 0);

         if ((instances = cmdOpts.nInstances('y')))
           for (i=0; i<instances; i++) userIfs->yres[i] = cmdOpts.getInt('y', i, 0);

         if (uFlags & CLI_MEDIA_APPS) {
            if ((uFlags & CLI_MEDIA_APPS_MEDIAMIN) && cmdOpts.getStr('s', 0) != NULL) strncpy(userIfs->szSDPFile, cmdOpts.getStr('s', 0), CMDOPT_MAX_INPUT_LEN);  /* added for mediaMin SDP file input, JHB Jan 2021 */
            else userIfs->nSegmentation = cmdOpts.getInt('s', 0, 0);  /* added for mediaTest segmentation, silence detection, strip, chunk rewrite, etc, JHB Jul 2019 */
         }
         else {
            if ((instances = cmdOpts.nInstances('s')))
              for (i=0; i<instances; i++) userIfs->streamingMode[i] = cmdOpts.getInt('s', i, 0);
         }

         if ((uFlags & CLI_MEDIA_APPS_MEDIAMIN) && cmdOpts.getStr('g', 0) != NULL) strncpy(userIfs->szStreamGroupOutputPath, cmdOpts.getStr('g', 0), CMDOPT_MAX_INPUT_LEN);  /* added for mediaMin stream group output path (including ramdisk), JHB Dec 2022 */

         if ((instances = cmdOpts.nInstances('r')))
           for (i=0; i<instances; i++) userIfs->frameRate[i] = cmdOpts.getFloat('r', i, 0);
         for (i=instances; i<MAX_INSTANCES; i++) userIfs->frameRate[i] = cmdOpts.getFloat('r', 0, 0);  /* get default values for remaining streams */

         if ((instances = cmdOpts.nInstances('D'))) 
           for (i=0; i<instances; i++) {
              userIfs->dstIpAddr[i] = cmdOpts.getIpAddr('D', i);
              userIfs->dstUdpPort[i] = cmdOpts.getUdpPort('D', i);
              userIfs->dstMacAddr[i] = cmdOpts.getMacAddr('D', i);
           }   

         if ((instances = cmdOpts.nInstances('S'))) 
           for (i=0; i<instances; i++) {
              userIfs->srcIpAddr[i] = cmdOpts.getIpAddr('S', i);
              userIfs->srcUdpPort[i] = cmdOpts.getUdpPort('S', i);
              userIfs->srcMacAddr[i] = cmdOpts.getMacAddr('S', i);
           }   

         if ((instances = cmdOpts.nInstances('B')))
           for (i=0; i<instances; i++) userIfs->bitRate[i] = cmdOpts.getInt('B', i, 0);
         else
           userIfs->bitRate[0] = cmdOpts.getInt('B', 0, 0);  /* get default value */

         if ((instances = cmdOpts.nInstances('V'))) {
            for (i=0; i<instances; i++) {
               userIfs->profile[i] = cmdOpts.getInt('V', i, 0);
               userIfs->bitrateConfig[i] = cmdOpts.getInt('V', i, 1);
               userIfs->qpValues[i] = cmdOpts.getInt('V', i, 2);
               userIfs->interFrameConfig[i] = cmdOpts.getInt('V', i, 3);
            }
         }
         else {  /* currently, default values are zero */

            userIfs->profile[0] = 0;
            userIfs->bitrateConfig[0] = 0;
            userIfs->interFrameConfig[0] = 0;
         }

         userIfs->nJitterBufferOptions = cmdOpts.getInt('j', 0, 0);

         if (uFlags & CLI_MEDIA_APPS) if ((instances = cmdOpts.nInstances('p'))) 
           for (i=0; i<instances; i++) userIfs->dstUdpPort[i] = (uint16_t)cmdOpts.getInt('p', i, 0);

      /* Scrypt test program flags */ 

         if (cmdOpts.nInstances('P'))
         {
            if (cmdOpts.getStr('P', 0) != NULL) strncpy(userIfs->scryptpasswd, cmdOpts.getStr('P', 0), CMDOPT_MAX_INPUT_LEN);
         }

         if (cmdOpts.nInstances('S'))
         {   
            if (cmdOpts.getStr('S', 0) != NULL) strncpy(userIfs->scryptsalt, cmdOpts.getStr('S', 0), CMDOPT_MAX_INPUT_LEN);
         }	

         if (cmdOpts.nInstances('U')) userIfs->userMode = true;

         if (cmdOpts.nInstances('E')) userIfs->encMode = true;

         if (cmdOpts.nInstances('D')) userIfs->decMode = true;

         if (cmdOpts.nInstances('N')) userIfs->scryptParamN = cmdOpts.getInt('N', 0, 0);

         if (cmdOpts.nInstances('r')) userIfs->scryptParamr = cmdOpts.getInt('r', 0, 0);

         if (!(uFlags & CLI_MEDIA_APPS))  /* don't overwrite portList */
           if (cmdOpts.nInstances('p')) userIfs->scryptParamp = cmdOpts.getInt('p', 0, 0);

         userIfs->debugMode = cmdOpts.getInt64('d', 0);  /* always call to get default value (-1) if no entry */

         if (uFlags & CLI_MEDIA_APPS) userIfs->nRepeatTimes = cmdOpts.getInt('R', 0, 0);  /* always call to get default value (-1) if no entry */
         else if (cmdOpts.nInstances('d')) userIfs->scryptdklen = cmdOpts.getInt('d', 0, 0);

         userIfs->programMode = cmdOpts.getInt('M', 0, 0);  /* always call to get default value (-1) if no entry */

         userIfs->executeMode = cmdOpts.getChar('E', 0);  /* always call to get default value (-1) if no entry */

         if (userIfs->programMode >= 0) {

            userIfs->programSubMode = userIfs->programMode >> 24;
            userIfs->programMode &= 0x00ffffffL;
         }
         else userIfs->programSubMode = -1;
      }

      if (!(uFlags & CLI_DISABLE_MANDATORIES)) {  /* CLI_DISABLE_MANDATORIES added 11May15, JHB */

         strcpy(tmpstr, userIfs->cardDesignator);
         strupr(tmpstr);

         bool fcoCPU = false;

         if (strstr(tmpstr, "X86")) strcpy(labelstr, "Platform Designator = ");
         else { strcpy(labelstr, "Card Designator = "); fcoCPU = true; }

         if (userIfs->processorClockrate) sprintf(clkstr, "%d MHz", userIfs->processorClockrate);
         else {  /* if no clock rate specified, get system clock rate, JHB Feb2022 */

            FILE* fp = fopen("/proc/cpuinfo", "r");

            bool fCpuMHzFound = false;

            if (fp) {

               size_t n = 0;
               char* line = NULL;

               while (getline(&line, &n, fp) > 0) {

                  if (strstr(line, "cpu MHz")) {

                     char* p = strrchr(line, ' ');  /* find first space from end */
                     if (p) {
                        strcpy(clkstr, p+1);
                        if ((p = strchr(clkstr, (char)0xd))) *p = 0;  /* remove any CR/LFs */
                        if ((p = strchr(clkstr, (char)0xa))) *p = 0;
                        strcat(clkstr, " MHz");
                        fCpuMHzFound = true;
                        break;
                     }
                  }
               }

               free(line);
               fclose(fp);
            }

            if (!fCpuMHzFound) strcpy(clkstr, "Default");
         }

#if 0  /* JHB Aug 2022 */
         char coCPU_executable[1000] = "N/A";
         if (strlen(userIfs->targetFileName)) strcpy(coCPU_executable, );
#endif

         if (!fSem_Init) { sem_init(&cout_sem, 0, 1); fSem_Init = true; }  /* add a lock to keep program start info coherent within multiple threads, JHB Aug2022 */
         sem_wait(&cout_sem);

         cout << "userSpecified = {" << labelstr << userIfs->cardDesignator <<  ", "
              << (fcoCPU ? "coCPU " : "") << "Clock = " << clkstr << ", "
              << "coCPU Core List = ";
         if (fcoCPU) cout << "0x" << hex << setfill('0') << setw(8) << userIfs->coreBitMask << ", ";  /* avoid issues with conditional controlling multiple << , JHB Aug2022 */
         else cout << "N/A, ";
         cout << "coCPU Executable = " << (fcoCPU ? userIfs->targetFileName : "N/A") << ", "              
              << "Algorithm Flag = " << dec << userIfs->algorithmIdNum << "}" << endl;  /* use stream modifiers for hex and decimal output, JHB Feb2015 */

         sem_post(&cout_sem);
      }

      return EXIT_SUCCESS;    
   }

   exit(1);
}   
