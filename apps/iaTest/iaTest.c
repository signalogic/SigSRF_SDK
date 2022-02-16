/*
   $Header: /root/Signalogic/DirectCore/apps/SigC641x_C667x/iaTest/iaTest.c

   Purpose:

     Test, demo, and benchmark program for Image Analytics
 
   Description:

     Image analytics demo, test, and performance measurement program, using x86 and/or coCPU hardware

   Copyright (C) Signalogic Inc. 2014-2017

   Revision History
     Created Nov 2014 AKM
     Modified 2015, JHB, support additional modes
     Modified 2015, JHB, make consistent with vid_streaming_host_rt.c OpenMP test program
     Modified May-Jun 2017, AM & JHB, add x86 mode for purposes of (i) SigSRF image analytics demo and (ii) benchmark and comparison between x86 and coCPU
*/

/*
  General form of command line:

  ./iaTest -mcore_list -fclock_rate -eexe_file -cplatform_type -s0 -iinput_file.yuv -xxres -yyres -ooutput_file.yuv -llib_flags

  Where:

   - core_list is number of cores to use
   - clock_rate is target CPU clock rate
   - platform_type is type of platform or coCPU card (plus optional suffix for number of cores to allocate)
   - s0 specifies one-shot mode (file to file).  Other modes include -s2 (continuous streaming)
   - input_file.yuv and output_file.yuv are input and output files in YUV 4:2:0 format (note:  when displaying with VLC Media Player, use J420 format instead of default I420)
   - xres and yres are x and y resolution
   - libs flag (-l cmd line option) controls operating mode values.  The example values below select the following constants (defined in ia.h):  IA_OPENCV_USE_FAST_FUNCS | IA_PROCLEVEL_STATS | IA_VISIBLE_DEBUG_LEVEL1 | IA_OPENCV_USE_TI_VLIB

  Specific x86 command line examples:

    x86 command line exmples:
    
      ./iaTest -m1 -cx86 -s0 -i/install_path/Signalogic/video_files/hallway_352x288p_30fps_420fmt.yuv -x352 -y288 -ohall_test.yuv -l0x01000003

    coCPU command line examples:

      ./iaTest -m1 -f1600 -eia.out -cSIGC66XX-8 -s0 -i/install_path/Signalogic/video_files/hallway_352x288p_30fps_420fmt.yuv -x352 -y288 -ohall_test.yuv -l0x01000003

      ./iaTest -m1 -f1600 -eia.out -cSIGC66XX-8 -s0 -i/install_path/Signalogic/video_files/CCTV_640x360p_30fps_420fmt.yuv -x640 -y360 -occtv_test.yuv -l0x01100003

*/

#include <stdio.h>
#include <sys/socket.h>
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

/* Signalogic header files */

#include "hwlib.h"          /* DirectCore API header header file */
#include "cimlib.h"

/* following header files required depending on application type */

#include "test_programs.h"
#include "keybd.h"

/* following shared host/target CPU header files required depending on app type */

#include "streamlib.h"
#include "video.h"
#include "ia.h"


/* Global variables and default values */

QWORD           nCoreList = 0;   /* bitwise core list, usually given in command line */

unsigned char   inputbuf[MAXSTREAMS][MAXVIDDESCRIPTORSIZE];
unsigned char   outputbuf[MAXSTREAMS][MAXVIDDESCRIPTORSIZE];
IAPARAMS        IAParams[MAXSTREAMS] = { 0 };  /* image analytics, video, and streaming params filled in by command line, see cimlib.h */
unsigned int    numStreams = 0; 
unsigned int    hostFramesWritten = 0;
unsigned int    numFileBytes, numBytesPerFrame;
unsigned int    start_usec = 0, total_usec;

volatile unsigned int  numFramesEncoded = 0, testrun = 0, cvwidth = 0, cvheight = 0, cvdepth = 0, ddrWriteIdx = 0, ddrReadIdx = 0, host_bufptr = 0;
volatile uintptr_t     ddrInputBase = 0, ddrOutputBase = 0;

/* these needed if streamlib is used */

DWORD dwAddr_bufnum;
DWORD dwAddr_ddrOutputNumBytes;  /* wrap-point debug addrs */
//DWORD ddrOutputBase = 0;  /* typically evaluates to ERAM_OUTPUT_DATA in c66x code (stream_h264.c) */
//DWORD ddrInputBase = 0;   /* typically evaluates to ERAM_PRELOADED_DATA in c66x code (stream_h264.c) */
DWORD dwAddr_inbufptr = 0, dwAddr_outbufptr = 0;  /* current host pointers inside target CPU video data circular buffers */

bool            fCoresLoaded = false;
unsigned int    CPU_mode = 0;  /* CPU_mode set depending on command line platform / card designator.  See possible constants defined in hwlib.h */
unsigned char*  pBuffer8_load = NULL, * pBuffer8_save;
uintptr_t       loadDataAddr = (uintptr_t)NULL, saveDataAddr = (uintptr_t)NULL;

/* local functions */

void RunOnce(int, time_t[]);
void UpdateStats(HDATAPLANE, int[], int);
unsigned int ExitLoop(HDATAPLANE, int[], int);
void SaveOutputFile(HDATAPLANE, unsigned int, int, int, void*);
bool InitStreams(HDATAPLANE, unsigned int, FILE* [], FILE* [], char* [], time_t[], void*);
void SavecoCPULog(HDATAPLANE);


int main(int argc, char *argv[]) {

/* items shared between API and CIM apps */

HDATAPLANE      dpHandle = (HDATAPLANE)NULL;  /* handle to data plane config and/or coCPU card */
PLATFORMPARAMS  PlatformParams;
int             i, exitCode, nBytesReadInput[MAXSTREAMS], nBytesReadOutput[MAXSTREAMS];
FILE*           fp_in[MAXSTREAMS] = { NULL }; 
FILE*           fp_out[MAXSTREAMS] = { NULL }; 
char*           memBuffer[MAXSTREAMS] = { NULL };
time_t          timerInterval[MAXSTREAMS] = { 1000 };  /* default timer setting:  1 msec rate in oneshot mode.  For continuous mode we set based on frame rate (below) */


/* display program banner */
  
   printf("DirectCore + OpenCV image analytics test program for x86 and/or coCPU platforms (bare metal or VMs), Rev 2.1, Copyright (C) Signalogic 2015-2017\n");

/* process command line for basic target CPU items:  card type, clock rate, executable file */

   if (!cimGetCmdLine(argc, argv, NULL, CIM_GCL_DEBUGPRINT, &PlatformParams, NULL)) exit(EXIT_FAILURE);

/* Check card designator and enable CPU mode */

   if (strstr(PlatformParams.szCardDesignator, "X86")) CPU_mode = CPUMODE_X86;
   else if (strstr(PlatformParams.szCardDesignator, "C66X")) CPU_mode = CPUMODE_C66X;
   else {
      printf ("Invalid CPU or coCPU setting in command line -c argument, %s\n", PlatformParams.szCardDesignator);
      exit(EXIT_FAILURE);
   }

   if (CPU_mode & CPUMODE_COCPU)  /* display coCPU card info */
     printf("coCPU card info: %s-%2.1fGHz, target executable file %s\n", PlatformParams.szCardDescription, PlatformParams.nClockRate/1e9, PlatformParams.szTargetExecutableFile);

/* assign platform handle, init cores, reset cores */

   if (!(dpHandle = cimInitPlatform(CIM_IH_DEBUGPRINT, &PlatformParams))) {  /* use CIM_IH_DEBUGPRINT flag so cimInitHardware will print error messages, if any */

      printf("cimInitHardware failed\n");
      exit(EXIT_FAILURE);
   }
   
   nCoreList = PlatformParams.nCoreList;
   
/* process command line again, get video and image analytics items */
  
   if (!cimGetCmdLine(argc, argv, NULL, CIM_GCL_IA | CIM_GCL_DEBUGPRINT, &PlatformParams, &IAParams)) goto cleanup;

   numStreams = IaNumStreams(IAParams);
 
   numBytesPerFrame = (IAParams[0].Video.width * IAParams[0].Video.height * YUV12bits_per_pixel / CHAR_BIT);

/* initialize streams depending on mode specified in cmd line */

   if (!InitStreams(dpHandle, CIM_GCL_IA, fp_in, fp_out, memBuffer, timerInterval, &IAParams)) goto cleanup;


   if (CPU_mode & CPUMODE_COCPU) {  /* coCPU card initialization, if applicable */

   /* load executable file(s) to target coCPU(s) */

      printf("Loading executable file %s to target CPU corelist 0x%lx\n", PlatformParams.szTargetExecutableFile, nCoreList);

      if (!(fCoresLoaded = DSLoadFileCores(dpHandle, PlatformParams.szTargetExecutableFile, nCoreList))) {

         printf("DSLoadFileCores failed\n");
         goto cleanup;
      }

   /* run target CPU hardware.  Give application type flag and also pointer to application property struct */

      if (!cimRunHardware(dpHandle, CIM_GCL_IA | CIM_RH_DEBUGPRINT | (PlatformParams.enableNetIO ? CIM_RH_ENABLENETIO : 0), &PlatformParams, (PIAPARAMS)&IAParams)) {
   
         printf("cimRunHardware failed\n");  /* use CIM_RH_DEBUGPRINT flag so cimRunHardware will print any error messages */
         goto cleanup;
      }
   }

/* start loop for oneshot or continuous image analysis */

   do {

      if (IsTimerEventReady()) { 

         for (i=0; i<numStreams; i++) {  /* multiple streams supported */
  
            if (StreamingMode(IAParams) == STREAM_MODE_CONTINUOUS) {  /* read stream data from input endpoint, write to target core(s) */

               nBytesReadInput[i] = streamRead((HANDLE*)&fp_in[i], i, memBuffer[i], STREAM_MODE_CONTINUOUS | STREAM_ENDPOINT_FILE, numBytesPerFrame, 0);
 
               if (nBytesReadInput[i] != 0)
                 streamWrite((HANDLE*)((uintptr_t)&dpHandle), i, memBuffer[i], STREAM_MODE_CONTINUOUS | STREAM_ENDPOINT_TARGETCPUMEM | IAParams[0].Streaming.bufferingMode, numBytesPerFrame, 0);

               if (fp_out[i] != NULL) {  /* read stream data from target core(s), write to output endpoint */ 

                  nBytesReadOutput[i] = streamRead((HANDLE*)((uintptr_t)&dpHandle), i, memBuffer[i], STREAM_MODE_CONTINUOUS | STREAM_ENDPOINT_TARGETCPUMEM | IAParams[i].Streaming.bufferingMode, 0, 0);
                  if (nBytesReadOutput[i]) streamWrite((HANDLE*)&fp_out[i], i, memBuffer[i], STREAM_MODE_CONTINUOUS | STREAM_ENDPOINT_FILE, nBytesReadOutput[i], 0);
               }
            }
         }

         UpdateStats(dpHandle, nBytesReadInput, numStreams);  /* print host and target frame counters, other stats */
      }

      RunOnce(numStreams, timerInterval);  /* one-time items, including timer start */
   
   } while (!(exitCode = ExitLoop(dpHandle, nBytesReadInput, numStreams)));


   printf("Total elapsed processing time = %5.3f sec\n", 1.0*total_usec/1000000.0);

   
/* save output .h264 or .yuv file if (i) in oneshot mode or (ii) 'S' (save) key command was given */

   for (i=0; i<numStreams; i++) {
   
      if (StreamingMode(IAParams) == STREAM_MODE_CONTINUOUS) {

         if (fp_in[i]) fclose(fp_in[i]);
         if (fp_out[i]) fclose(fp_out[i]);
      }
      else SaveOutputFile(dpHandle, CIM_GCL_IA, i, exitCode, &IAParams);
   }

cleanup:

   if (CPU_mode & CPUMODE_COCPU)  /* save coCPU log, if applicable */
     if (fCoresLoaded) SavecoCPULog(dpHandle);
   
   printf("Program and platform cleanup, dpHandle = %d\n", dpHandle);

   for (i=0; i<numStreams; i++) if (memBuffer[i]) free(memBuffer[i]);

   if (pBuffer8_load) free(pBuffer8_load);

   if (pBuffer8_save) free(pBuffer8_save);

/* platform cleanup */

   if (dpHandle) cimClosePlatform(dpHandle, CIM_CH_DEBUGPRINT | CIM_GCL_IA, nCoreList, (PVDIPARAMS)&IAParams);  /* note:  we're passing CIM_GCL_IA flag and IAParams pointer, but cimCloseHardware currently does not use these.  It might in the future.  JHB, Oct 2015 */
}


/* local functions */

void RunOnce(int numStreams, time_t timerInterval[]) {

static bool fRunOnce[MAXSTREAMS] = { false };
int i;

   for (i=0; i<numStreams; i++) {
   
      if (!fRunOnce[i]) {

         if (i == 0) {

            setTimerInterval((time_t)0, timerInterval[i]);  /* start timer */

            printf("Frame stats[%d]:", i);  /* print a stats label */
         }

         fRunOnce[i] = true;
      }
   }
}


void UpdateStats(HDATAPLANE dpHandle, int nBytesReadInput[], int numStreams) {

unsigned int    i, nCore, numCPUs;
unsigned int    extern_heap_ptr = 0;
bool            fFrameIncrement;
struct timeval  tv;


   if (StreamingMode(IAParams) == STREAM_MODE_CONTINUOUS) {
   
      fFrameIncrement = false;

      for (i=0; i<numStreams; i++) {
   
         if (nBytesReadInput[i] != 0) fFrameIncrement = true;
      }
   }
   else fFrameIncrement = true;

   if (fFrameIncrement) hostFramesWritten++;
   
   if (CPU_mode & CPUMODE_CPU) numCPUs = DSGetCardInfo(dpHandle, DS_GCI_NUMCPUSPERCARD);
   else numCPUs = DSGetPlatformInfo(dpHandle, DS_GCI_NUMPLATFORMCPUS);

   for (i=0; i<numStreams; i++) {
   
      nCore = numCPUs*i;

      if (CPU_mode & CPUMODE_COCPU) {  /* coCPU card symbol lookup and stats update, if applicable */
   
         cimReadSymbol(dpHandle, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, "testrun", (void*)&testrun, DS_RM_SIZE32, 1, (QWORD)1 << nCore);
         cimReadSymbol(dpHandle, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, "cvwidth", (void*)&cvwidth, DS_RM_SIZE32, 1, (QWORD)1 << nCore);
         cimReadSymbol(dpHandle, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, "cvheight", (void*)&cvheight, DS_RM_SIZE32, 1, (QWORD)1 << nCore);
         cimReadSymbol(dpHandle, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, "cvdepth", (void*)&cvdepth, DS_RM_SIZE32, 1, (QWORD)1 << nCore);
         cimReadSymbol(dpHandle, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, "extern_heap_ptr", (void*)&extern_heap_ptr, DS_RM_SIZE32, 1, (QWORD)1 << nCore);
         cimReadSymbol(dpHandle, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, "host_bufptr", (void*)&host_bufptr, DS_RM_SIZE32, 1, (QWORD)1 << nCore);
         cimReadSymbol(dpHandle, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, "ddrInputBase", (void*)&ddrInputBase, DS_RM_SIZE32, 1, (QWORD)1 << nCore);
         cimReadSymbol(dpHandle, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, "ddrOutputBase", (void*)&ddrOutputBase, DS_RM_SIZE32, 1, (QWORD)1 << nCore);
         cimReadSymbol(dpHandle, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, "ddrReadIdx", (void*)&ddrReadIdx, DS_RM_SIZE32, 1, (QWORD)1 << nCore);
         cimReadSymbol(dpHandle, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, "ddrWriteIdx", (void*)&ddrWriteIdx, DS_RM_SIZE32, 1, (QWORD)1 << nCore);

         cimReadSymbol(dpHandle, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, "numFramesEncoded", (void*)&numFramesEncoded, DS_RM_SIZE32, 1, (QWORD)1 << nCore);  /* numFramesEncoded gives coCPU core progress */
      }

      if (i == 0) printf("\r\t\t");

      printf("%d %d  ", StreamingMode(IAParams) == STREAM_MODE_ONESHOT ? IAParams[i].Video.framesToEncode : hostFramesWritten, numFramesEncoded);

      if (start_usec == 0) {

         gettimeofday(&tv, NULL);
         start_usec = 1000000L*tv.tv_sec + tv.tv_usec;
      }

      if (i == 0) {

         if (StreamingMode(IAParams) == STREAM_MODE_CONTINUOUS) printf("host_bufptr=0x%x ", host_bufptr);

         printf("testrun=%d ddrRIdx=0x%x ddrWIdx=0x%x h=0x%x cvw=%d cvh=%d, cvd=%d        ", testrun, ddrReadIdx, ddrWriteIdx, extern_heap_ptr, cvwidth, cvheight, cvdepth);

//      if (i == 0) printf("testrun=%d ddrInputBase=0x%x ddrOutputBase=0x%x ddrReadIdx=0x%x ddrWriteIdx=0x%x cvw=%d cvh=%d, cvd=%d h=0x%x", testrun, ddrInputBase, ddrOutputBase, ddrReadIdx, ddrWriteIdx, cvwidth, cvheight, cvdepth, extern_heap_ptr);
      }
   }
}

unsigned int ExitLoop(HDATAPLANE dpHandle, int nBytesReadInput[], int numStreams) {  /* exit loop if 'Q' or 'S' keys pressed, or if we're in oneshot mode and we finish processing number of frames in the video file */

unsigned int retval = 0;
bool fDone = false;
char ch;
int i;
struct timeval tv;

   for (i=0; i<numStreams; i++) {
   
      if (StreamingMode(IAParams) == STREAM_MODE_ONESHOT) {
   
         if (numFramesEncoded >= IAParams[i].Video.framesToEncode) fDone = true;  /* all streams have to be done */
         else fDone = false;
      }
      else if (StreamingMode(IAParams) == STREAM_MODE_CONTINUOUS) {

         if (nBytesReadInput[i] == 0) fDone = true;
         else fDone = false;
      }
   }

   if (fDone) {

      gettimeofday(&tv, NULL);
      total_usec = 1000000L*tv.tv_sec + tv.tv_usec - start_usec;

      UpdateStats(dpHandle, nBytesReadInput, numStreams);

      fflush(stdout);  /*  remote terminals might not have printed all stats and frame counts yet */
      retval = EXIT_DONE;
   }
   else {

      ch = toupper(getkey());
   
      if (ch == 'Q') retval = EXIT_QUIT;
      else if (ch == 'S') retval = EXIT_SAVE;
   }

   if (retval) {
   
      printf("\n");
   }

   return retval;
}

void SaveOutputFile(HDATAPLANE dpHandle, unsigned int uFlags, int nStream, int exitCode, void* pAppParams) {

unsigned int  coCPUAddr;
unsigned int  mode = structype(uFlags, pAppParams, [0].Streaming.mode);

   if (((mode == STREAM_MODE_ONESHOT && exitCode == EXIT_DONE) || exitCode == EXIT_SAVE) && strlen(structype(uFlags, pAppParams, [0].Video.outputFilename)) > 0) {

      if (CPU_mode & CPUMODE_COCPU) cimReadSymbol(dpHandle, DS_RM_LINEAR_DATA, "ddrWriteIdx", (void*)&ddrWriteIdx, DS_RM_SIZE32, 1, nCoreList);  /* to-do:  this needs to read ddrWriteIdx from correct stream (mapped to correct CPU), JHB Oct2015 */

      if (ddrWriteIdx > 0) {

         if (strstr(structype(uFlags, pAppParams, [nStream].Video.outputFilename), ".yuv")) {

            if (mode == STREAM_MODE_ONESHOT && ddrWriteIdx > numFileBytes) ddrWriteIdx = numFileBytes;

            coCPUAddr = ERAM_OUTPUT_DATA_IMAGE;
         }
         else coCPUAddr = ERAM_OUTPUT_DATA_STREAMING;

      /* read .yuv data and save to file  */

         if (DSSaveDataFile(CPU_mode & CPUMODE_CPU ? DS_GM_HOST_MEM : dpHandle, NULL, structype(uFlags, pAppParams, [nStream].Video.outputFilename), CPU_mode & CPUMODE_CPU ? saveDataAddr : coCPUAddr, ddrWriteIdx, (uint32_t)NULL, NULL)  > 0)
           printf("Saving %d bytes of output video data to file %s...\n", ddrWriteIdx, structype(uFlags, pAppParams, [nStream].Video.outputFilename));
         else
           printf("Error opening or writing output video data file %s...\n", structype(uFlags, pAppParams, [nStream].Video.outputFilename));
      }
      else printf("ddrWriteIdx zero; no data processed for output video data file %s\n", structype(uFlags, pAppParams, [nStream].Video.outputFilename));
   }
}


bool InitStreams(HDATAPLANE dpHandle, unsigned int uFlags, FILE* fp_in[], FILE* fp_out[], char* memBuffer[], time_t timerInterval[], void* pAppParams) {

unsigned int  mode = structype(uFlags, pAppParams, [0].Streaming.mode);
unsigned int  numStreams = structype(uFlags, pAppParams, [0].numStreams);
int           numBytes, i, rc;
pthread_t     thread;

   if (CPU_mode & CPUMODE_CPU) {

      pBuffer8_load = (unsigned char*)malloc(100000000L);  /* 100 MB mem area for input .yuv data.  To-do, replace later with inputFilesize element in IaParams struct */
      pBuffer8_save = (unsigned char*)malloc(100000000L);  /* 100 MB mem area for output .yuv data.  To-do, replace later with inputFilesize element in IaParams struct */

      if (pBuffer8_load) loadDataAddr = (uintptr_t)pBuffer8_load;
      if (pBuffer8_save) saveDataAddr = (uintptr_t)pBuffer8_save;

  printf("loadDataAddr = 0x%llx\n", (long long unsigned)loadDataAddr);

      ddrInputBase = loadDataAddr;
      ddrOutputBase = saveDataAddr;
   }
   else {

      loadDataAddr = COCPU_BUFFER_BASE_ADDR;
   }

   if (mode == STREAM_MODE_ONESHOT) {

      printf("Loading input video data from file %s... \n", structype(uFlags, pAppParams, [0].Video.inputFilename));

      if ((numBytes = DSLoadDataFile(CPU_mode & CPUMODE_CPU ? DS_GM_HOST_MEM : dpHandle, NULL, structype(uFlags, pAppParams, [0].Video.inputFilename), loadDataAddr, 0, (uint32_t)NULL, NULL, NULL)) <= 0) {

         printf("Input video file %s not found\n", structype(uFlags, pAppParams, [0].Video.inputFilename));
         return 0;
      }
      else {

         numFileBytes = numBytes;
         printf("Loaded %d bytes of input video data file %s\n", numFileBytes, structype(uFlags, pAppParams, [0].Video.inputFilename));
      }

      structype_rhs(uFlags, pAppParams, [0].Video.framesToEncode, = numFileBytes/numBytesPerFrame);

      printf("Number of frames to process %d\n", structype(uFlags, pAppParams, [0].Video.framesToEncode));
   }
   else if (mode == STREAM_MODE_CONTINUOUS) {

      for (i=0; i<numStreams; i++) {

         if (strlen(structype(uFlags, pAppParams, [i].Video.inputFilename))) {

            printf("Opening input video data file[%d] %s... \n", i, structype(uFlags, pAppParams, [i].Video.inputFilename));

//    printf("ptr in func = %u\n", fp_in);
//    if (fp_in == NULL) return 0;

            fp_in[i] = fopen(structype(uFlags, pAppParams, [i].Video.inputFilename), "rb");

            if (fp_in[i] == NULL) {

               printf("Could not find / open input video file %s\n", structype(uFlags, pAppParams, [i].Video.inputFilename));
               return 0;
            }

            structype_rhs(uFlags, pAppParams, [i].Video.framesToEncode, = 0);  /* zero indicates continuous (indefinite) operation */

            memBuffer[i] = (char*)malloc(MAX_MEM_BUFFER_SIZE*sizeof(char));

            timerInterval[i] = 1000000L/(structype(uFlags, pAppParams, [i].Video.frameRate));  /* set timer to frame rate in continuous mode */
         }

         if (strlen(structype(uFlags, pAppParams, [i].Video.outputFilename))) {

            printf("Creating output video data file[%d] %s... \n", i, structype(uFlags, pAppParams, [i].Video.outputFilename));

            fp_out[i] = fopen(structype(uFlags, pAppParams, [i].Video.outputFilename), "wb");
  
            if (fp_out[i] == NULL) {

               printf("Could not create output video file %s\n", structype(uFlags, pAppParams, [i].Video.outputFilename));
               return 0;
            }

            if (memBuffer[i] == NULL) *memBuffer = (char*)malloc(MAX_MEM_BUFFER_SIZE*sizeof(char));
         }
      }

      fp_in++;
      fp_out++;
      memBuffer++;
      timerInterval++;

   }  /* for i numStreams loop */

/* for x86 start per-instance threads, according to core list (-m cmd line arg) */

   if (CPU_mode & CPUMODE_X86) {

      rc = pthread_create(&thread, NULL, ImageStream, NULL);
   }

   return 1;
}


void SavecoCPULog(HDATAPLANE dpHandle) {

char szLogStartSymbol[100] = "xdc_runtime_SysMin_Module_State_0_outbuf__A";
char szLogFilename[100] = "coCPU_log.txt";
char szStream[10];
DWORD c66xLog_addr;
QWORD nCoreList_save;

   c66xLog_addr = DSGetSymbolAddr(dpHandle, NULL, szLogStartSymbol);

   if (c66xLog_addr) {

      nCoreList_save = DSGetCoreList(dpHandle);  /* save current core list */
   
      printf("Saving coCPU log, target mem address: 0x%x\n", c66xLog_addr);
      
      DSSaveDataFile(dpHandle, NULL, szLogFilename, c66xLog_addr, 1048576, (uint32_t)NULL, NULL);

      DSSetCoreList(dpHandle, nCoreList_save);  /* restore core list */
   }
   else printf("coCPU log not saved, unable to find symbol %s\n", szLogStartSymbol);

#define CJ_TEMP_DEBUG  /* uncomment to provide additional debug, including DMA activity in c66x code */
#ifdef CJ_TEMP_DEBUG
   DWORD prog_var, numCores_addr, mainprog_addr, dwAddr_testrun, setlocidprog_addr, lastfxnprog_addr, prologprog_addr, ipcBarOpen_prog_addr, ipcBarCreate_prog_addr, rman_debug_addr, Bar_open_prog_addr, errorCode_addr, h264_encode_prog_addr, net_init_status_addr, net_init_progress_addr, cimInfo_addr, init_cpsw_status_addr, encode_int_variable_addr, scaledwfn_prog_variable_addr, dmawait_prog_variable_addr, putsliceP_prog_variable_addr, ipr_bit_mask_scratch_addr, ipr_bit_mask_handle_addr, ipr_register_address_scratch_addr, dma_channel_number_scratch_addr, dma_channel_number_handle_addr, qdma_channel_number_handle_addr, h264_ecpychannel_variable_addr;
   unsigned int nCore, nCPU, nCoresPerCPU;
   int i;
   QWORD nCoreList_temp, nCoreList_temp2;
   
   if (dpHandle && fCoresLoaded) {  /* don't do if dpHandle invalid (driver not loaded, cmd line incorrect card designator, etc) */

      numCores_addr = DSGetSymbolAddr(dpHandle, NULL, "numCores");

      mainprog_addr = DSGetSymbolAddr(dpHandle, NULL, "mainProg");

      printf("\nmain_prog addr = 0x%0x\n", mainprog_addr);

      dwAddr_testrun = DSGetSymbolAddr(dpHandle, NULL, "testrun");	
      setlocidprog_addr = DSGetSymbolAddr(dpHandle, NULL, "setLocalId_prog");
      lastfxnprog_addr = DSGetSymbolAddr(dpHandle, NULL, "lastFxnsTestVal");
      prologprog_addr = DSGetSymbolAddr(dpHandle, NULL, "vid_encode_prolog_prog");
      ipcBarCreate_prog_addr = DSGetSymbolAddr(dpHandle, NULL, "ipcBarCreate_prog");
      ipcBarOpen_prog_addr = DSGetSymbolAddr(dpHandle, NULL, "ipcBarOpen_prog");
      Bar_open_prog_addr = DSGetSymbolAddr(dpHandle, NULL, "Bar_open_prog");
      h264_encode_prog_addr  = DSGetSymbolAddr(dpHandle, NULL, "h264_encode_prog");
      net_init_status_addr = DSGetSymbolAddr(dpHandle, NULL, "net_init_status");
      net_init_progress_addr = DSGetSymbolAddr(dpHandle, NULL, "net_init_progress");
      init_cpsw_status_addr = DSGetSymbolAddr(dpHandle, NULL, "init_cpsw_status");
      errorCode_addr = DSGetSymbolAddr(dpHandle, NULL, "errorCode");
      cimInfo_addr = DSGetSymbolAddr(dpHandle, NULL, "cimInfo");
      encode_int_variable_addr = DSGetSymbolAddr(dpHandle, NULL, "encode_int_variable"); 
      scaledwfn_prog_variable_addr = DSGetSymbolAddr(dpHandle, NULL, "scaledwfn_prog_variable");
      dmawait_prog_variable_addr = DSGetSymbolAddr(dpHandle, NULL, "dmawait_prog_variable");  
      putsliceP_prog_variable_addr = DSGetSymbolAddr(dpHandle, NULL, "putsliceP_prog_variable");
      ipr_bit_mask_scratch_addr = DSGetSymbolAddr(dpHandle, NULL, "ipr_bit_mask_scratch");
      ipr_bit_mask_handle_addr = DSGetSymbolAddr(dpHandle, NULL, "ipr_bit_mask_handle");
      ipr_register_address_scratch_addr = DSGetSymbolAddr(dpHandle, NULL, "ipr_register_address_scratch"); 
      dma_channel_number_scratch_addr = DSGetSymbolAddr(dpHandle, NULL, "dma_channel_number_scratch");
      dma_channel_number_handle_addr = DSGetSymbolAddr(dpHandle, NULL, "dma_channel_number_handle");
      qdma_channel_number_handle_addr = DSGetSymbolAddr(dpHandle, NULL, "qdma_channel_number_handle");
      h264_ecpychannel_variable_addr = DSGetSymbolAddr(dpHandle, NULL, "h264_ecpychannel_variable");

      rman_debug_addr = DSGetSymbolAddr(dpHandle, NULL, "rman_debug_variable");

      nCoresPerCPU = DSGetCardInfo(dpHandle, DS_GCI_NUMCORESPERCPU);

      i = 0;

      nCoreList_temp = nCoreList;

      do {  /* modified in case corelist is not contiguous, for example core 0 on different CPUs. JHB, Feb 2015 */

         if (nCoreList_temp & 1) {  /* skip any zeros in core list (zero = core not used) */

            nCPU = i/nCoresPerCPU;
            nCore = i%nCoresPerCPU;
            nCoreList_temp2 = 1 << (nCPU*nCoresPerCPU + nCore);

            printf("CPU[%d] core %d, Ex() core list = 0x%lx: ", nCPU, nCore, nCoreList_temp2);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, numCores_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("numCores = %d, ", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, mainprog_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("mainProg = 0x%x, ", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, setlocidprog_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("setLocalId_prog = %d, ", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, lastfxnprog_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("lastFxnsTestVal = %d, ", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, prologprog_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("vid_encode_prolog_prog = %d, ", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, h264_encode_prog_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("h264_encode_prog = %d, ", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, ipcBarCreate_prog_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("\tipcBarCreate_prog = 0x%x, ", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, ipcBarOpen_prog_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("ipcBarOpen_prog = 0x%x, ", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, Bar_open_prog_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("Bar_open_prog = 0x%x, ", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, net_init_status_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("net_init_status = %d\n", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, net_init_progress_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("net_init_progress = %d, ", prog_var);
            
            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, init_cpsw_status_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("init_cpsw_status = %d\n", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, errorCode_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("errorCode = %d\n", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, dwAddr_testrun | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("\ttestrun = %d\n", prog_var);

            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, cimInfo_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("\tcimInfo addr = 0x%x, cimInfo value = 0x%x\n", cimInfo_addr, prog_var);
            
            DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, encode_int_variable_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("encode_int_variable = %d\n", prog_var);
			
			DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, scaledwfn_prog_variable_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("scaledwfn_prog_variable = %d\n", prog_var); 
			
			DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, dmawait_prog_variable_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("dmawait_prog_variable = %d\n", prog_var); 
			
			DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, ipr_bit_mask_scratch_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("ipr_bit_mask_scratch = %d\n", prog_var); 
			DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, ipr_bit_mask_handle_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("ipr_bit_mask_handle = %d\n", prog_var); 
			DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, ipr_register_address_scratch_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("ipr_register_address_scratch = %d\n", prog_var); 
			DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, dma_channel_number_scratch_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("dma_channel_number_scratch = %d\n", prog_var); 
			DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, dma_channel_number_handle_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("dma_channel_number_handle = %d\n", prog_var); 
			DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, qdma_channel_number_handle_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("qdma_channel_number_handle = %d\n", prog_var); 
			DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, h264_ecpychannel_variable_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("h264_ecpychannel_variable = %d\n", prog_var); 
		    DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA, putsliceP_prog_variable_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf("putsliceP_prog_variable_addr = %d\n", prog_var);
			
			 DSReadMemEx(dpHandle, DS_GM_LINEAR_DATA,  rman_debug_addr | 0x10000000 | (0x01000000 * nCore), DS_GM_SIZE32, &prog_var, 1, nCoreList_temp2);
            printf(" rman_debug_variable = %d\n", prog_var); 
         }

         i++;
      }
      while (nCoreList_temp >>= 1);

      printf("\n");
   }
#endif
/* end of temp debug - CJ */
}
