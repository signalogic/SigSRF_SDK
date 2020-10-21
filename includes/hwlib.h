/*
  $Header: /root/Signalogic/DirectCore/include/hwlib.h   10/13/05 9:29p
 
  Description: API include file for DirectCore software
 
  Project: DirectCore
 
  Copyright (C) Signalogic Inc. 1994-2020
  
  3     10/13/05 9:29p
  Added SDRAM sanity check.
  
  2     10/10/05 12:32p
  8 DSP support.
  
  1     8/24/05 7:13p
  Clean up code and rebuild.

  Revision History
  
   Created 1994 JHB
   Modified 2002-2013 JHB
   Modified Jan-May 2013 CJ, JHB, multicore CPU support, additional APIs
   Modified May 2014 JHB, adding DS_GCI_DRIVERHANDLE
   Modified Nov 2014 JHB, continued updates to support multiple target types
   Modified Apr 2015 JHB, added DSInitRTAF(), DSSyncTargetCPU(), and DSWriteAppProperties() APIs, additional error/status codes
   Modified Aug 2015 JHB, increased MAXCPUS to 128
   Modified Sep 2015 CJ, added support for allocating contiguous host memory and mapping it to C66x addresses
   Modified Mar 2017 JHB, added DSAssignDataPlane and DSFreeDataPlane APIs (based on DSAssignCard and DSFreeCard)
   Modified Jul-Aug 2019 JHB, add static inline get_time() and TSC monitoring.  get_time() default operation is clock_gettime(CLOCK_MONOTONIC)
   Modified Sep 2019 JHB, use __builtin_ia32_rdtscp() and __builtin_ia32_rdtsc() to avoid including x86intrin.h, which seems to have gcc version (and maybe Linux version) compatibility issues
   Modified Jan 2020 JHB, add DS_DATAFILE_USESEM flag for use with DSSaveDataFile() and DSLoadDataFile()
   Modified Sep 2020 JHB, fix #pragma GCC diagnostic ignored "-pedantic", should be "-Wpedantic". This was causing a warning in gcc 9.3.0
*/
 
#ifndef _HWLIB_H_
#define _HWLIB_H_

/* needed by get_time() */
#include <sys/time.h>
#include <time.h>
#ifdef USE_X86INTRIN  /* in get_time() below, we use __builtin_ia32_xxx() functions directly to avoid including 86intrin.h, which seems to get tangled up in build issues with different versions of gcc and/or Linux (e.g. gcc 4.8.2 on Red Hat) */
  #include <x86intrin.h>
#endif

#include "diaglib.h" /* event log support */
#include "enmgr.h"
#include "boards.h"
#include "sig_mc_hw.h"  /* c66x driver header file */

extern const char HWLIB_VERSION[256];

/* temporary in-file "strict" literals for Win16 BOOL and UINT */
#if !defined(__WIN32__) && !defined(__MSVC__) && !defined(_LINUX_) && !defined(_WIN32_WINNT)

  #define UINT unsigned short int
  #define BOOL short int

#endif

/* Win16 / Win32 import definition */


#if !defined(LIBAPI)
   #if !defined(__WIN32__) && !defined(__MSVC__)
      #define LIBAPI WINAPI
   #else
      #ifdef _MSC_VER
         /* #define LIBAPI __declspec(dllimport)  // this method fails with MSVC 4.2 IMPLIB */
         #define LIBAPI __stdcall
      #else
         #define LIBAPI __stdcall
      #endif
   #endif
#endif

#if !defined(DECLSPEC)
   #if !defined(__WIN32__) && !defined(__MSVC__)
      #define DECLSPEC
   #else
      #if !defined (_LINUX_)
         #if !defined (__LIBRARYMODE__)
            #define DECLSPEC
         #elif defined (_DSPOWER_)
            #define DECLSPEC __declspec(dllexport)  /* this method fails with MSVC 4.2 IMPLIB */

         #else
            #define DECLSPEC __declspec(dllimport)  /* this method fails with MSVC 4.2 IMPLIB */
         #endif
      #endif
   #endif
#endif

#if defined (__MSVC__)
   #include "hwdriver32.h"
#endif

#if defined (_LINUX_)  || defined (_WIN32_WINNT)
   #include "alias.h"
#endif

/* supporting type definitions */

#if defined __MSVC__ 
   typedef void* HBOARD;         /* board handle */
   typedef void* HCARD;          /* card handle ("board" is deprecated, JHB, Jan13) */
   typedef void* HDATAPLANE;     /* data plane handle */
#elif defined _LINUX_
  /* done in alias.h */
#elif defined _WIN32_WINNT
  /* done in alias.h */
#else
   typedef HGLOBAL HBOARD;       /* board handle */
   typedef HGLOBAL HCARD;        /* card handle ("board" is deprecated, JHB, Jan13) */
   typedef HGLOBAL HDATAPLANE;   /* data plane handle */
#endif

#ifndef Boolean
   typedef short int Boolean;   /* boolean for both Win16 and Win32 */
#endif


/* overall constants */

#define MAXCPUS           128
#define MAXCORESPERCPU    8           /* to-do:  increase to 32 */
#define MAXCHAN           8           /* current maximum number of channels */
#define DSMAXPATH         260
#define DSMAXFILE         16
#define DSMAXBOARDNAME    128
#define DS_EVT_MAXEVENTS  256         /* maximum number of events in callback queue */


/* MEDIAINFO structure used in high-level APIs DSLoadDataFile, DSSaveDataFile, DSAcquireWvfrmFile, and DSGenerateWvfrmFile */

typedef struct {

/* Notes:

  1) entire structure should be initialized to zero first, so that default values can take
     effect for parameters not used

  2) Current value in DSSetWaveformPath is used for waveform filename, unless explicit
     path information is found in the filename

  3) TrigDelay parameter not yet operational.

*/

 /* basic info */

   char      szFilename[DSMAXPATH]; /* [path\]filename of waveform being acquired/generated */
   short int NumChan;
   short int ChanList[MAXCHAN];     /* channel list, maps physical channels to waveform traces */
   WORD      AcqFlags;              /* not currently used; reserved as zero */
   float     Fs;                    /* sampling rate (in Hz) */
   DWORD     NumSamples;            /* number of samples to acquire/generate */
   short int SampleWidth;           /* sample width in bits.  For example a .wav file might have samples expressed in 8, 16, or 24 bits */
   DWORD     DAStartSample;         /* starting sample for D/A output operations */
   char      szGainList[44];        /* string specifying gain list for input channels; can contain ',' and '-' other characters to specify lists and ranges */
   long      Offset;                /* offset added to data (in A/D counts, can be +/-) */
   Boolean   DALooping;             /* enables/disables continuous D/A looping; 0 or 1, valid only during generation (output) */
   Boolean   Monitor;               /* enables/disables D/A loopback when recording; 0 or 1, valid only during acquisition (input) */
   Boolean   FileSplit;             /* 1 enables A/D file-splitting for multichannel, 0 disables */
   Boolean   StoreOnAbort;          /* 1 == store current wavefrom data on abort */
   DWORD     Frmsiz;                /* framesize (stored in waveform header; used for post-processing only) */

/* real-time digital filter info */

   char      szFilter1[DSMAXFILE];  /* filename (no path) of real-time digital filter; can contain '/' character */
   char      szFilter2[DSMAXFILE];  /* to allow dual-filter entry (filter files must be located on same path as waveform) */

/* trigger info */

   WORD      TrigMode;              /* trigger mode (0 = internal, 1 = external) */
   WORD      TrigFlags;             /* not currently used; reserved as zero */
   long      TrigLevel;             /* trigger level (in A/D counts, can be +/-) */
   float     TrigDelay;             /* delay from trigger relative to data (in sec, can be +/-) */
   short int NumTrigChan;
   short int TrigChanList[MAXCHAN]; /* trigger channel list */

/* stimulus & response info */

   char      szStimName[DSMAXFILE]; /* filename (no path) of stimulus output; used only for DSStimRespMeasure function (must be located on same path as waveform) */
   WORD      StimMode;              /* controls stimulus usage in Stimulus/Response function */
   WORD      StimFlags;             /* not currently used; reserved as zero */
   long      StimDelay;             /* delay from stimulus onset to start of record (+/- value) */

/* compression info */

   short int CompressionCode;       /* added to support wav file output for G711, G726, other basic compressed formats supported by .wav file headers.  See DS_GWH_CC_xxx values in fileib.h, JHB Jul2017 */
   
} MEDIAINFO;

typedef MEDIAINFO FAR* PMEDIAINFO;

/* AcqFlags values in MEDIAINFO struct */

#define DS_ACQFLG_NOFILEPREALLOCATE  1
#define DS_ACQFLG_SYSINTSENABLED     2


/* Define EventInfo structure (used in callback functions; see DSRegisterCallbackFunc API) */

typedef struct {

#if defined(__ppc__) || defined(powerpc) || defined(ppc) || defined(_ARCH_PPC)
   UINT16 Chan;
   BYTE eventData;
   BYTE eventType;
#else
   BYTE eventType;
   BYTE eventData;
   UINT16 Chan;
#endif
   UINT TimeStamp;
   
} EVENTINFO;

typedef EVENTINFO FAR* LPEVENTINFO;

/* define user callback function type */

typedef void(*CALLBACK)(HCARD hCard, LPEVENTINFO lpEventInfo, UINT cbSize);


/* define RTAFInitProperties struct for DSInitRTAF() API */

typedef struct {

  UINT  uTestMode;     /* list of flags used in main_rtaf() to initialize target card I/O peripherals */
  UINT  uClockRate;    /* in Hz */
  WORD  wCardClass;    /* lower byte is card type, upper byte is sub-type */
  
} RTAFINITPROPERTIES;

typedef RTAFINITPROPERTIES FAR* PRTAFINITPROPERTIES;


/* define DSWriteAppProperties() API */

typedef struct {

  char     szSymbolName[256];
  UINT     uPropertyFlags;
  UINT     uPropertySizeBytes;
  uint64_t uPropertyVal;
  
} APPPROPERTIES;

typedef APPPROPERTIES ARRAY_APPPROPERTIES[1];

typedef ARRAY_APPPROPERTIES* PAPPPROPERTIES;  /* pointer to array of APPPROPERTIES structs */


/* DLL call prototypes */
#ifndef _NO_H_PROTOTYPE

#ifdef __cplusplus
extern "C" {
#endif

/* library error handling and status */

int LIBAPI DSGetHWLibErrorStatus(UINT);
#define DSGetAPIErrorStatus() DSGetHWLibErrorStatus(0)

/* board initialization and control functions (all processors simultaneous) */

DECLSPEC HCARD     LIBAPI DSAssignCard               (HENGINE, LPCSTR, UINT, WORD, WORD, BOOL, int);
DECLSPEC HPLATFORM LIBAPI DSAssignPlatform           (HENGINE, LPCSTR, UINT, UINT, int);
DECLSPEC HCARD     LIBAPI DSAttachCard               (HENGINE, HCARD);
DECLSPEC UINT      LIBAPI DSFreeCard                 (HCARD);
DECLSPEC UINT      LIBAPI DSFreePlatform             (HPLATFORM);
DECLSPEC UINT      LIBAPI DSGetCardInfo              (HCARD, UINT);
DECLSPEC UINT      LIBAPI DSGetCardStatus            (HCARD);
DECLSPEC int       LIBAPI DSGetInstance              (HCARD);

DECLSPEC UINT      LIBAPI DSInitCard                 (HCARD);  /* not used by Linux code */
DECLSPEC UINT      LIBAPI DSLoadFileCard             (HCARD,  LPCSTR);
DECLSPEC UINT      LIBAPI DSResetcard                (HCARD);
DECLSPEC UINT      LIBAPI DSRunCard                  (HCARD);
DECLSPEC UINT      LIBAPI DSDisableCard              (HCARD);
DECLSPEC UINT      LIBAPI DSHoldCard                 (HCARD);

#define DSAssignBoard DSAssignCard
#define DSAssignDataPlane DSAssignCard  /* currently DSAssignDataPlane has the same format and args as DSAssignCard.  This may change in the future.  JHB Apr 2017 */
#define DSInitBoard DSInitCard
#define DSRunBoard DSRunCard
#define DSResetBoard DSResetCard
#define DSLoadFileBoard DSLoadFileCard
#define DSFreeBoard DSFreeCard
#define DSFreeDataPlane DSFreeCard
#define DSDisableBoard DSDisableCard
#define DSGetBoardInfo DSGetCardInfo
#define DSGetPlatformInfo DSGetCardInfo

/* individual processor initialization and control functions */

DECLSPEC UINT      LIBAPI DSInitProcessor            (HCARD, QWORD); /* used by Linux code */
DECLSPEC UINT      LIBAPI DSLoadFileProcessor        (HCARD, LPCSTR, QWORD);
DECLSPEC UINT      LIBAPI DSResetProcessor           (HCARD, QWORD);
DECLSPEC UINT      LIBAPI DSRunProcessor             (HCARD, QWORD);
DECLSPEC UINT      LIBAPI DSHoldProcessor            (HCARD, QWORD);
DECLSPEC UINT      LIBAPI DSSetProcessorList         (HCARD, QWORD);
DECLSPEC QWORD     LIBAPI DSGetProcessorList         (HCARD);
DECLSPEC WORD      LIBAPI DSGetProcessorStatus       (HCARD);
DECLSPEC BOOL      LIBAPI DSSdramSanityCheck         (HCARD);

DECLSPEC UINT      LIBAPI DSResetDevices             (HCARD, QWORD);
DECLSPEC UINT      LIBAPI DSRunDevices               (HCARD, QWORD);

#define DSInitCore DSInitProcessor
#define DSInitCores DSInitProcessor
#define DSLoadFileCore DSLoadFileProcessor
#define DSLoadFileCores DSLoadFileProcessor
#define DSResetCore DSResetProcessor
#define DSResetCores DSResetProcessor
#define DSRunCore DSRunProcessor
#define DSRunCores DSRunProcessor
#define DSSetCoreList DSSetProcessorList
#define DSGetCoreList DSGetProcessorList
#define DSGetCoreStatus DSGetProcessorStatus

/* memory and variable transfers, IEEE conversion */
DECLSPEC INT       DSLoadDataFile                    (HCARD, FILE**, const char*, uintptr_t, UINT, UINT, PMEDIAINFO);  /* card handle (can be NULL or DS_GM_xx values), pointer to a file pointer, filename (cannot be NULL for DS_OPEN flag), buffer (or card address), length, flags, pointer to CONVERSIONINFO struct (NULL if not used) */ 
DECLSPEC INT       DSSaveDataFile                    (HCARD, FILE**, const char*, uintptr_t, UINT, UINT, PMEDIAINFO);  /* card handle (can be NULL or DS_GM_xx values), pointer to a file pointer, filename (cannot be NULL for DS_CREATE or DS_OPEN flags), buffer (or card address), length, flags, pointer to CONVERSIONINFO struct (NULL if not used) */ 

DECLSPEC UINT      LIBAPI DSPutMem                   (HCARD, UINT, DWORD, UINT, void far*, DWORD);
DECLSPEC UINT      LIBAPI DSGetMem                   (HCARD, UINT, DWORD, UINT, void far*, DWORD);
#define DSSetMem   DSPutMem
#define DSWriteMem DSPutMem
#define DSReadMem  DSGetMem

DECLSPEC UINT      LIBAPI DSPutMemEx                 (HCARD, UINT, DWORD, UINT, void far*, DWORD, QWORD);
DECLSPEC UINT      LIBAPI DSGetMemEx                 (HCARD, UINT, DWORD, UINT, void far*, DWORD, QWORD);
#define DSSetMemEx   DSPutMemEx
#define DSWriteMemEx DSPutMemEx
#define DSReadMemEx  DSGetMemEx

DECLSPEC UINT      LIBAPI DSPutHVarMem               (HCARD, DWORD, DWORD);
DECLSPEC DWORD     LIBAPI DSGetHVarMem               (HCARD, DWORD);
#define DSGetDSPProperty DSGetHVarMem
#define DSSetDSPProperty DSPutHVarMem
#define DSGetProperty DSGetHVarMem
#define DSSetProperty DSPutHVarMem

DECLSPEC UINT      LIBAPI DSPutHVarMemEx             (HCARD, DWORD, DWORD, QWORD);
DECLSPEC DWORD     LIBAPI DSGetHVarMemEx             (HCARD, DWORD, QWORD);
#define DSGetDSPPropertyEx DSGetHVarMemEx
#define DSSetDSPPropertyEx DSPutHVarMemEx
#define DSGetPropertyEx DSGetHVarMemEx
#define DSSetPropertyEx DSPutHVarMemEx

DECLSPEC UINT      LIBAPI DSIEEEToDSP                (HCARD, UINT, void far*, void far*, DWORD);
DECLSPEC UINT      LIBAPI DSDSPToIEEE                (HCARD, UINT, void far*, void far*, DWORD);
#define DSIEEEToTarget DSIEEEToDSP
#define DSTargetToIEEE DSDSPToIEEE

/* wait-for-buffer and wait-for-flag */

DECLSPEC UINT      LIBAPI DSWaitForBuffer            (HCARD, short int, DWORD, UINT);
DECLSPEC UINT      LIBAPI DSWaitForFlag              (HCARD, short int, DWORD, UINT);
DECLSPEC UINT      LIBAPI DSCancelWaitBuffer         (HCARD, short int);
DECLSPEC UINT      LIBAPI DSCancelWaitFlag           (HCARD, short int);

DECLSPEC DWORD     LIBAPI DSGetBufferInfo            (short int, UINT);
DECLSPEC DWORD     LIBAPI DSGetFlagInfo              (short int, UINT);


/* interrogate board system parameters */

DECLSPEC WORD      LIBAPI DSGetBoardClass            (HCARD);
DECLSPEC DWORD     LIBAPI DSCalcSampFreq             (HCARD, float, short int, short int far*, float far*);
DECLSPEC DWORD     LIBAPI DSGetMemSize               (HCARD, DWORD);
DECLSPEC UINT      LIBAPI DSGetMemArch               (HCARD);
DECLSPEC WORD      LIBAPI DSGetWordLength            (HCARD);

#define DSGetCardClass DSGetBoardClass

/* get/set board base addresses, bus type, registers, etc */

DECLSPEC WORD      LIBAPI DSGetBoardBaseAddr         (HCARD, UINT);
DECLSPEC UINT      LIBAPI DSSetBoardBaseAddr         (HCARD, UINT, WORD);

#define DSGetCardBaseAddr DSGetBoardBaseAddr
#define DSSetCardBaseAddr DSSetBoardBaseAddr

DECLSPEC UINT      LIBAPI DSGetBoardBusType          (HCARD);
DECLSPEC UINT      LIBAPI DSSetBoardBusType          (HCARD, UINT);

#define DSGetCardBusType DSGetBoardBusType
#define DSGStCardBusType DSSetBoardBusType

DECLSPEC DWORD     LIBAPI DSReadBoardReg             (HCARD, WORD);
DECLSPEC UINT      LIBAPI DSWriteBoardReg            (HCARD, WORD, DWORD);

#define DSReadCardReg DSReadBoardReg
#define DSWriteCardReg DSWriteBoardReg

DECLSPEC DWORD     LIBAPI DSReadBoardRegEx           (HCARD, WORD, QWORD);
DECLSPEC UINT      LIBAPI DSWriteBoardRegEx          (HCARD, WORD, DWORD, QWORD);

#define DSReadCardRegEx DSReadBoardRegEx
#define DSWriteCardRegEx DSWriteBoardRegEx

DECLSPEC UINT      LIBAPI DSRestoreBoardDefaults     (HCARD);


/* waveform acquisition functions */

DECLSPEC UINT      LIBAPI DSAcquireWvfrmFile         (HCARD, PMEDIAINFO, UINT);
DECLSPEC UINT      LIBAPI DSGenerateWvfrmFile        (HCARD, PMEDIAINFO, UINT);


/* target CPU boot, init, run, sync, and property functions */

DECLSPEC UINT      LIBAPI DSInitRTAF                 (HCARD, PRTAFINITPROPERTIES, UINT, QWORD);         /* RTAF properties initialization */
DECLSPEC UINT      LIBAPI DSSyncTargetCPU            (HCARD, UINT, QWORD);                              /* target CPU sync */
DECLSPEC UINT      LIBAPI DSWriteAppProperties       (HCARD, UINT, UINT, PAPPPROPERTIES, UINT, QWORD);  /* write list of properties to target CPU */


/* other COFF file and debugging operations */

DECLSPEC DWORD     LIBAPI DSGetSymbolAddress         (HCARD, LPCSTR, LPCSTR);
#define DSGetSymbolAddr DSGetSymbolAddress

DECLSPEC UINT      LIBAPI DSLoadObjectFile           (HCARD, LPCSTR, QWORD);

DECLSPEC HCARD     LIBAPI DSFindBoard                (LPCSTR);
#define DSFindCard DSFindBoard

/* callback related functions */

DECLSPEC UINT      LIBAPI DSRegisterCallbackFunc     (HCARD hCard, CALLBACK lpCallbackFunc, UINT uMode, UINT uModeInfo);

DECLSPEC UINT      LIBAPI DSCallbackFunc             (UINT cbHandle, UINT uCmd);

DECLSPEC UINT      LIBAPI DSUnregisterCallbackFunc   (HCARD hCard);


/* misc */

extern BOOL 	    globalVerbose;  /* deprecated, don't use.  JHB JUL2010 */

/* TSC monitoring, Aug 2019 */

#ifdef _X86

  #define USE_CLOCK_GETTIME  1
  #define USE_GETTIMEOFDAY   2
  #define MONITOR_TSC_INTEGRITY

  #pragma GCC diagnostic push  /* some codecs are built with -pedantic, we suppress mixed declarations/code warning for those, JHB Aug 2019 */
  
  #if GCC_VERSION >= 40800  /* check for gcc 4.8.0 or higher */
    #pragma GCC diagnostic ignored "-Wpedantic"
  #else
    #pragma GCC diagnostic ignored "-pedantic"
  #endif

  static inline uint64_t get_time(unsigned int uFlags) {

     uint64_t ret_val;

     #ifdef MONITOR_TSC_INTEGRITY
     extern bool fRDTSCPSupported;  /* global var in hwlib */

     static int64_t prev_rdtsc = 0;
     static unsigned int prev_core_id = 0;
     int64_t rdtsc1, rdtsc2, rdtsc3 = 0, slip1 = 0, slip2 = 0;

     if (fRDTSCPSupported) {
        unsigned int dummy;
        #ifdef USE_X86INTRIN
        rdtsc1 = __rdtscp(&dummy);
        #else
        rdtsc1 = __builtin_ia32_rdtscp(&dummy);
        #endif
     }
     else {
        #ifdef USE_X86INTRIN
        rdtsc1 = __rdtsc();
        #else
        rdtsc1 = __builtin_ia32_rdtsc();
        #endif
      }
     #endif

     if (uFlags == USE_CLOCK_GETTIME) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ret_val = ts.tv_sec * 1000000L + ts.tv_nsec/1000;
     }
     else {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        ret_val = tv.tv_sec * 1000000L + tv.tv_usec;
     }

     #ifdef MONITOR_TSC_INTEGRITY
     unsigned int core_id;

     if (fRDTSCPSupported) {
        #ifdef USE_X86INTRIN
        rdtsc2 = __rdtscp(&core_id);
        #else
        rdtsc2 = __builtin_ia32_rdtscp(&core_id);
        #endif
        if (rdtsc2 <= rdtsc1) slip1 = rdtsc2 - rdtsc1;
        core_id &= 0xff;  /* core id in lower 8 bits of TSC AUX register */
        if (rdtsc2 <= (rdtsc3 = __sync_fetch_and_add(&prev_rdtsc, 0)) && core_id == prev_core_id) slip2 = rdtsc2 - rdtsc3;  /* only compare same-core TSC reads, in case CPU does not support invariant TSC synchronized between cores (Sig lab servers with Xeon 2660 do not) */
     }
     else {
        #ifdef USE_X86INTRIN
        rdtsc2 = __rdtsc();
        #else
        rdtsc2 = __builtin_ia32_rdtsc();
        #endif
        if (rdtsc2 <= rdtsc1) slip1 = rdtsc2 - rdtsc1;
     }

     if (slip1 || slip2) Log_RT(3, "WARNING: get_time() reports TSC integrity / adjustment issue, time slip = %ld, context switch slip = %ld (cycles), r2 = %ld, r3 = %ld, core_id = %u, prev_core_id = %u \n", -slip1, -slip2, rdtsc2, rdtsc3, core_id, prev_core_id);

     if (fRDTSCPSupported) {
        __sync_lock_test_and_set(&prev_rdtsc, &rdtsc2);
        __sync_lock_test_and_set(&prev_core_id, &core_id);
     }
     #endif

     return ret_val;
  }

  #pragma GCC diagnostic pop

#endif  /* _X86 */

#ifdef __cplusplus
}
#endif

#endif /* _NO_H_PROTOTYPE */

/* Error / Status Codes returned by DSGetAPIErrorStatus() */

#define DSHARDWARENOTRESPONDING        11
#define DSDSPFILENOTFOUND              12
#define DSMAGICNUMBERNOTFOUND          13
#define DSNOTENOUGHHARDWAREMEM         14
#define DSHOSTINTERFACEERROR           15
#define DSHARDWAREMEMCONFIGPROBLEM     16
#define DSHARDWAREXTMEMERROR           17
#define DSDRIVERAPIERROR               18
#define DSLIBAPIERROR                  19

#define DSINVALIDBOARDHANDLE           -50
#define DSINVALIDCARDHANDLE            DSINVALIDBOARDHANDLE
#define DSCOULDNOTOPENTEMPLATEFILE     -51
#define DSCOULDNOTCREATESOURCEFILE     -52
#define DSCREATINGENGINEPROGRAMERROR   -53
#define DSUNABLETOALLOCATEMEMORY       -54
#define DSUNABLETOREADTEMPLATEFILE     -55
#define DSERRORINDATAPARAM             -56
#define DSBOARDDOESNOTSUPPORTCALL      -57
#define DSCARDDOESNOTSUPPORTCALL       DSBOARDDOESNOTSUPPORTCALL
#define DSINVALIDWORDLENGTH            -58
#define DSINVALIDMEMORYTYPE            -59
#define DSUNKNOWNCALLCLASSIFICATION    -60
#define DSINVALIDBOARDDESIGNATOR       -61
#define DSINVALIDCARDDESIGNATOR        DSINVALIDBOARDDESIGNATOR
#define DSBOARDNOTINHWSETUPFILE        -62
#define DSCARDNOTINHWSETUPFILE         DSBOARDNOTINHWSETUPFILE
#define DSALLBOARDHANDLESALLOCATED     -63
#define DSALLCARDHANDLESALLOCATED      DSALLBOARDHANDLESALLOCATED
#define DSINVALIDPROCESSORNUMBER       -64
#define DSINVALIDOBJFILEFORMAT         -65
#define DSOBJFILEUNABLETOOPEN          -66
#define DSOBJFILEHASNOSYMBOLS          -67
#define DSOBJFILESYMBOLNOTFOUND        -68
#define DSINVALIDPROCESSORTYPE         -69
#define DSINVALIDBUILDIMAGE            -70
#define DSNETWORKPROCESSORCOMMERROR    -71
#define DSOBJFILESYMBOLZEROLEN         -72
#define DSOBJFILESYMBOLEXCEEDSMAXLEN   -73
#define DSTIMEOUT                      -74
#define DSCOULDNOTACCESSMEM            -75
#define DSSHMOBJECTERROR               -76
#define DSMUTEXERROR                   -77


/* AssignCard / AssignPlatform constants */

#define DS_AB_PCXT                     0     /* bus types */
#define DS_AB_PCAT                     0
#define DS_AB_ENABLE80X86INST          1
#define DS_AB_USB                      2
#define DS_AB_PTMC                     3
#define DS_AC_PCXT                     DS_AB_PCXT
#define DS_AC_PCAT                     DS_AB_PCAT
#define DS_AC_ENABLE80X86INST          DS_AB_ENABLE80X86INST
#define DS_AC_USB                      DS_AB_USB
#define DS_AC_PTMC                     DS_AB_PTMC
#define DS_AC_PCIEX1                   4
#define DS_AC_PCIEX4                   5
#define DS_AC_PCIEX8                   6
#define DS_AC_PCIEX16                  7

/* uOptions constants */

#define DS_AB_RESET_CPUS_SIMULTANEOUS  1     /* reset attributes */
#define DS_AC_RESET_CPUS_SIMULTANEOUS  DS_AB_RESET_CPUS_SIMULTANEOUS
#define DS_AC_USEHARDRESET             2

#define DS_AC_ENABLETALKER             0x10  /* legacy c64x, c54x, and c55x hardware */
#define DS_AC_QUERYINSTANCES           0x20

/* these constants overload wMemBaseAddr param in DSAssignCard() */

#define DS_AB_MEMMODE_DIRECT           0x0000
#define DS_AB_MEMMODE_MASTER_NOWAIT    0X0010
#define DS_AB_MEMMODE_MASTER_WAIT      0X0020
#define DS_AB_MEMMODE_SLAVE_WAIT       0X0030

#define DS_AB_MEMMODE_MASK             0x00f0

#define DS_AC_MEMMODE_DIRECT           DS_AB_MEMMODE_DIRECT
#define DS_AC_MEMMODE_MASTER_NOWAIT    DS_AB_MEMMODE_MASTER_NOWAIT
#define DS_AC_MEMMODE_MASTER_WAIT      DS_AB_MEMMODE_MASTER_WAIT
#define DS_AC_MEMMODE_SLAVE_WAIT       DS_AB_MEMMODE_SLAVE_WAIT
#define DS_AC_MEMMODE_MASK             DS_AB_MEMMODE_MASK

#define DS_AC_CORELIST64               0
#define DS_AC_CORELISTEXTENDED         0x0100

/* CPU and coCPU modes */

#define CPUMODE_X86                    1
#define CPUMODE_X86_TEST               2
#define CPUMODE_CPU                    0xff                           /* native types are defined in 0x1 to 0xff range */
#define CPUMODE_C66X                   0x100
#define CPUMODE_COCPU                  0xff00                         /* coCPU types are defined in 0x0100 to 0xff00 range */
#define CPUMODE_X86_COCPU              (CPUMODE_X86 | CPUMODE_COCPU)


/* memory types returned by GetMemArch */

#define DS_GMA_LINEAR                  1
#define DS_GMA_HARVARD                 2
#define DS_GMA_VECTOR                  3


/* memory types for DSGetMem and DSPutMem */

#define DS_GM_VECTOR_DATA_X            1   /* vector "X" data memory */
#define DS_GM_VECTOR_DATA_Y            2   /* vector "Y" data memory */
#define DS_GM_LINEAR_DATA_RT           3   /* linear data memory, real-time (useful for some TMS320 series cards only) */
#define DS_GM_LINEAR_DATA              4   /* linear data memory */
#define DS_GM_LINEAR_PROG              5   /* linear program memory */
#define DS_GM_LINEAR_PROGRAM           DS_GM_LINEAR_PROG
#define DS_GM_VECTOR_DATA_L            6   /* vector long (combined X and Y) data memory */
#define DS_GM_NETWORKPROCESSOR_DPMEM   7   /* read/write data from/to network processor dual-port memory instead of DSP memory (if the card has this, SigC5561 is one example) */

#define DS_RM_MCSM                     8   /* multicore shared memory */
#define DS_RM_L2                       9   /* core-specific L2 memory */
#define DS_RM_EXTSDRAM                 10  /* device external SDRAM memory (e.g. DDR3 SDRAM) */
#define DS_RM_DDR3                     DS_RM_EXTSDRAM
#define DS_RM_MM_REGISTER              11  /* memory-mapped register */

#define DS_RM_VECTOR_DATA_X            DS_GM_VECTOR_DATA_X
#define DS_RM_VECTOR_DATA_Y            DS_GM_VECTOR_DATA_Y
#define DS_RM_LINEAR_DATA_RT           DS_GM_LINEAR_DATA_RT
#define DS_RM_LINEAR_DATA              DS_GM_LINEAR_DATA
#define DS_RM_LINEAR_PROG              DS_GM_LINEAR_PROG
#define DS_RM_LINEAR_PROGRAM           DS_GM_LINEAR_PROGRAM
#define DS_RM_VECTOR_DATA_L            DS_GM_VECTOR_DATA_L
#define DS_RM_NETWORKPROCESSOR_DPMEM   DS_GM_NETWORKPROCESSOR_DPMEM

/* memory type attribute flags usable with card handles. Notes:

  -currently supported only by DSSaveDataFile() and DSLoadDataFile() APIs
  -example: DSSaveDataFile(hCard | DS_GM_HOST_MEM, filename, bufferAddr, numBytes, pMediaInfo);  // hCard can be NULL, if so other DS_GM_xx types may be combined

  JHB Jan2017
*/

#define DS_GM_HOST_MEM                 0x10000000L
#define DS_GM_COCPU_MEM                0x20000000L

#define DS_DATAFILE_USESEM              0x1000000L

/* "direct access" attributes for that work with memory type. Notes:
   
   1) Used for driver streaming; minimize driver interaction and error-checking
   2) Only use with DSGetMem and DSPutMem function calls
   3) Only certain cards supported; contact Signalogic if unsure
*/

#define DS_GM_DIRECTACCESS             0x1000
#define DS_GM_READMULTIPLE             0x2000
#define DS_RM_MULTIPLECORE             DS_GM_READMULTIPLE  /* DS_RM_MULTIPLECORE can now be used with both read and write, JHB MAY 2014 */

#define DS_RM_MASTERMODE               0x4000  /* for example, DSWwriteMem(hCard, DS_RM_MCSM | DS_RM_MASTERMODE, DS_GM_SIZE32, ... ) */

#define DS_RM_DIRECTACCESS             DS_GM_DIRECTACCESS

/* bit-width constants for DSReadMem and DSWriteMem */

#define DS_GM_SIZE8                    1     /* 8-bits */
#define DS_GM_SIZE16                   2     /* 16-bits */
#define DS_GM_SIZE24                   3     /* 24-bits */
#define DS_GM_SIZE32                   4     /* 32-bits */
#define DS_GM_SIZE64                   8     /* 32-bits */
#define DS_GM_SIZE1                    0x10  /* 1-bit - currently only supported for DS_GM_READMULTIPLE calls to C667X */

#define DS_RM_SIZE8                    DS_GM_SIZE8
#define DS_RM_SIZE16                   DS_GM_SIZE16
#define DS_RM_SIZE24                   DS_GM_SIZE24
#define DS_RM_SIZE32                   DS_GM_SIZE32
#define DS_RM_SIZE64                   DS_GM_SIZE64
#define DS_RM_SIZE1                    DS_GM_SIZE1

/* constants for DSWaitForBuffer and DSWaitForFlag */

#define DS_WFB_POLLED                  0   /* polled notification */
#define DS_WFB_INTERRUPT               1   /* interrupt-driven notification */
#define DS_WFB_SYNC                    16  /* synchronous operation bit:  combine with type to wait for buffer to complete */


/* constants for DSGetCardBaseAddr and DSSetCardBaseAddr */

#define DS_GBBA_IOADDR                 0   /* I/O base address */
#define DS_GBBA_MEMADDR                1   /* memory base address */

#define DS_GCBA_IOADDR                 DS_GBBA_IOADDR
#define DS_GCBA_MEMADDR                DS_GBBA_MEMADDR

/* constants for DSGetCardInfo */

#define DS_GBI_ENGINEHANDLE            0
#define DS_GBI_DRIVERID                1
#define DS_GBI_TYPE                    DS_GBI_DRIVERID  /* card type can be determined from driver ID (unique for every card) */
#define DS_GBI_CALLSALLOWED            2
#define DS_GBI_HWMGRENTRYINDEX         3
#define DS_GBI_DSPWORDLENGTH           4  /* result returned in bits */
#define DS_GBI_GETBOARDFIRST           5
#define DS_GBI_GETBOARDNEXT            6
#define DS_GBI_MODULEID                7
#define DS_GBI_MFGDRIVERHANDLE         8
#define DS_GBI_MFGBOARDHANDLE          9

#define DS_GCI_ENGINEHANDLE            DS_GBI_ENGINEHANDLE
#define DS_GCI_DRIVERID                DS_GBI_DRIVERID
#define DS_GCI_CARDTYPE                DS_GBI_TYPE
#define DS_GCI_CALLSALLOWED            DS_GBI_CALLSALLOWED
#define DS_GCI_HWMGRENTRYINDEX         DS_GBI_HWMGRENTRYINDEX
#define DS_GCI_CPUWORDLENGTH           DS_GBI_DSPWORDLENGTH
#define DS_GCI_GETBOARDFIRST           DS_GBI_GETBOARDFIRST
#define DS_GCI_GETBOARDNEXT            DS_GBI_GETBOARDNEXT
#define DS_GCI_MODULEID                DS_GBI_MODULEID
#define DS_GCI_MFGDRIVERHANDLE         DS_GBI_MFGDRIVERHANDLE
#define DS_GCI_MFGCARDHANDLE           DS_GBI_MFGBOARDHANDLE

#define DS_GCI_DRIVERHANDLE            10  /* get driver handle */
#define DS_GCI_ENTRYPOINT              11  /* get target executable code entry point */
#define DS_GCI_NUMCORESPERCPU          12
#define DS_GCI_NUMCPUSPERCARD          13
#define DS_GCI_NUMPLATFORMCPUS         DS_GCI_NUMCPUSPERCARD  /* hwlib looks at driver ID and for x86 returns overall number of platform CPUs */

/* DSGetCardStatus return values */

#define DS_GCS_OPEN                    1
#define DS_GCS_ACTIVE                  2

/* flags for DSGetHwlibAPIErrorStatus */

#define DS_GHAES_SECONDARY             1

/* constants for DSTargetToIEEE and DSIEEEToTarget */

#define DS_DTI_IEEESIZE32              4
#define DS_DTI_IEEESIZE64              8

#define DS_TTI_IEEESIZE32              DS_DTI_IEEESIZE32
#define DS_TTI_IEEESIZE64              DS_DTI_IEEESIZE64

/* constants for DSAcquireWvfrmFile and DSGenerateWvfrmFile */

#define DS_AWF_ASYNC                   0   /* asynchronous operation (default) */
#define DS_AWF_SYNC                    1   /* synchronous operation (wait for file to complete) */


#define DSPutVarMem DSPutHVarMem           /* 'H' versions are obsolete */
#define DSGetVarMem DSGetHVarMem
#define DSPutVarMemEx DSPutHVarMemEx
#define DSGetVarMemEx DSGetHVarMemEx


/* constants for callback related functions */

#define DS_EVT_MAXEVENTS               256



/* DSRegisterCallbackFunc constants */

#define DS_RCBF_LTIMER                 1  /* uMode */
#define DS_RCBF_SIGIO                  2

/* DSCallbackFunc constants */

#define DS_CBF_DISABLE                 1  /* uCmd */
#define DS_CBF_RESTART                 2
#define DS_CBF_DELETE                  3
   

/* DSInitRTAF flag constants */

#define DS_IR_DEBUGPRINT               DS_STC_DEBUGPRINT


/* DSSyncTargetCPU flag constants */

#define DS_STC_BOOT                    1     /* boot target CPUs:  (typically boot from I2C flash) */
#define DS_STC_INIT                    2     /* init target CPUs:  run autoinit, handle .cinit and .pinit sections, all initialization prior to main() */
#define DS_STC_SYNC                    4     /* sync target CPUs:  sync all specified cores to known location inside main(), wait for host release before proceeding */
#define DS_STC_RUN                     8     /* run target CPUs:  run application code on all specified cores */

#define DS_STC_DEBUGPRINT              0X10000000U
#define DS_STC_COREDEBUGPRINT          0X20000000U


/* DSWriteAppProperties flag constants */

#define DS_WAP_VALISAPTR               1
#define DS_WAP_USESIZE8                2
#define DS_WAP_NOERRORPRINT            4
#define DS_WAP_DEBUGPRINT              DS_STC_DEBUGPRINT


/* identifiers which can be used in DSPutVarMem and DSGetVarMem calls; see Signalogic Software Reference Guide, CIM-RTAF Source Code Variables and Flags */

#define DSPROP_CARDCLASS       0x42F        /* card classification */
#define DSPROP_OPMODE          0x430        /* operating mode */
#define DSPROP_BUSYFLG         0x431        /* DSP busy flag */
#define DSPROP_INITWAIT        DSPROP_BUSYFLG
#define DSPROP_OVERFLOW        0x432        /* overflow flag */
#define DSPROP_MINVAL          0x433        /* minimum value in acquisition or processing */
#define DSPROP_FFTORD          0x434        /* FFT order */
#define DSPROP_FRMSIZ          0x435        /* Framesize */
#define DSPROP_FFTLEN          0x436        /* FFT length */
#define DSPROP_MAXVAL          0x437        /* maximum value in acquisition or processing buffer */
#define DSPROP_RIFLG           0x438        /* real/imaginary Flag */
#define DSPROP_COUPLIST        0x439        /* analog input coupling list */
#define DSPROP_GAINLIST        0x43A        /* analog input gain list */
#define DSPROP_FSMODE          0x43B        /* sampling rate clock generator mode value */
#define DSPROP_TRIGLEVEL       0x43C        /* analog input trigger level */
#define DSPROP_BUFLEN          0x43D        /* acquisition or processing buffer length */
#define DSPROP_HOSTBUFNUM      0x43E        /* current host buffer flag */
#define DSPROP_BUFNUM          0x43F        /* current DSP buffer flag */
#define DSPROP_SCALEIN         0x440        /* analog input digital scale factor */
#define DSPROP_OFFSETIN        0x441        /* analog input digital offset */
#define DSPROP_WINSCL          0x442        /* frequency domain window scaling factor */
#define DSPROP_PHZREQ          0x443        /* phase data required */
#define DSPROP_DUPFLG          0x444        /* duplicate trace flag */
#define DSPROP_FILTADDR1       0x445        /* filter 1 coefficient address */
#define DSPROP_CHANLIST        0x446        /* analog input channel list */
#define DSPROP_TRIGCHANLIST    0x447        /* analog input trigger channel list */
#define DSPROP_SCALEOUT        0x448        /* analog output digital scaling factor */
#define DSPROP_OFFSETOUT       0x449        /* analog output digital offset */
#define DSPROP_FILTADDR2       0x44A        /* filter 2 Coefficient Address */
#define DSPROP_MAXVALREAL      0x44B        /* real component of complex data maximum value */
#define DSPROP_MAXVALIMAG      0x44C        /* imag component of complex data maximum value */
#define DSPROP_FILTLEN1        0x44D        /* filter 1 length */
#define DSPROP_FILTLEN2        0x44E        /* filter 2 length */
#define DSPROP_LOGFLG1         0x44F        /* log magnitude flag, trace 1 */
#define DSPROP_LOGFLG2         0x450        /* log magnitude flag, trace 2 */
#define DSPROP_FILTTYPE1       0x451        /* filter type, trace 1 */
#define DSPROP_FSVALUE         0x452        /* sampling rate value (in Hz) */
#define DSPROP_PWRFLG1         0x453        /* power spectra flag, trace 1 */
#define DSPROP_PWRFLG2         0x454        /* power spectra flag, trace 2 */
#define DSPROP_PWRCOEFFA       0x455        /* power spectra exponential filter coefficient a */
#define DSPROP_PWRCOEFFB       0x456        /* power spectra exponential filter coefficient b */
#define DSPROP_XFERFLG1        0x457        /* transfer function flag, trace 1 */
#define DSPROP_XFERFLG2        0x458        /* transfer function flag, trace 2 */
#define DSPROP_MAXVAL1         0x459        /* maximum amplitude, trace 1 */
#define DSPROP_MAXVAL2         0x45A        /* maximum amplitude, trace 2 */
#define DSPROP_NUMCHAN         0x45B        /* number of analog input or output channels */
#define DSPROP_CARDSUBCLASS    0x45C        /* card subclassification */
#define DSPROP_TIMDATAADDR     0x45D        /* time domain data base address */
#define DSPROP_RIDATAADDR      0x45E        /* complex data base address */
#define DSPROP_MAGDATAADDR     0x45F        /* magnitude/phase data base address */
#define DSPROP_WINDATAADDR     0x460        /* window data base address */
#define DSPROP_MONITORFLG      0x461        /* monitor mode flag */
#define DSPROP_IOMODULE        0x462        /* I/O module indicator */
#define DSPROP_SERIALCTRL      0x463        /* serial port control for special cases */
#define DSPROP_LOGCOEFFA       0x464        /* log magnitude scaling coefficient a */
#define DSPROP_LOGCOEFFB       0x465        /* log magnitude scaling coefficient b */
#define DSPROP_ADCOUNT         0x466        /* acquisition delay counter */
#define DSPROP_DACOUNT         0x467        /* stimulus delay counter */
#define DSPROP_STMDATAADDR     0x468        /* stimulus data base Address */
#define DSPROP_STMBUFLEN       0x469        /* stimulus data buffer length */
#define DSPROP_FILTTYPE2       0x46A        /* filter type, trace 2 */
#define DSPROP_FILTQUANT       0x46B        /* filter quantization */
#define DSPROP_FILTUPDFLG      0x46C        /* filter coefficient update flag */
#define DSPROP_CODECCTRL1      0x46D        /* CODEC control word 1  (RTAF versions for different processor families vary on properties 0x46D and 0x46E) */
#define DSPROP_CODECCTRL2      0x46E        /* CODEC control word 2 */
#define DSPROP_ISRADD          0x46D        /* user-defined ISR address */
#define DSPROP_SYNCMODE        0x46E        /* synchronization mode flag [0 == host polls DSP, 1 == DSP notifies HOST with HINT and syncs with HOST, 2 == DSP notifies HOST with HINT and runs free (no sync)] */

/* overloaded properties */

#define DSPROP_CPUCORELIST     DSPROP_PWRFLG1    /* list of cores on a CPU that will be active and should be initialized */
#define DSPROP_TESTMODE        DSPROP_PWRFLG2    /* test mode, used by RTAF to initialize peripherals */
#define DSPROP_HOSTCSYNC       DSPROP_PWRCOEFFA  /* synchronization handshake used by host and target during boot-up and C code initialization */
#define DSPROP_CPUID           DSPROP_PWRCOEFFB  /* ID assigned to a CPU; e.g. CPU0, CPU1, etc */
#define DSPROP_CPUCLOCKRATE    DSPROP_XFERFLG1   /* CPU clock rate, in Hz */
#define DSPROP_COREID          DSPROP_XFERFLG2   /* ID assigned to a core.  Usually the same as DNUM */

#define DSPROP_USERVAR         0x46F        /* user-defined variable start */


#define DSP_BOARDCLASS         DSPROP_CARDCLASS
#define DSP_CARDCLASS          DSPROP_CARDCLASS
#define DSP_OPMODE             DSPROP_OPMODE
#define DSP_BUSYFLG            DSPROP_BUSYFLG
#define DSP_INITWAIT           DSPROP_INITWAIT
#define DSP_OVERFLOW           DSPROP_OVERFLOW
#define DSP_MINVAL             DSPROP_MINVAL
#define DSP_FFTORD             DSPROP_FFTORD
#define DSP_FRMSIZ             DSPROP_FRMSIZ
#define DSP_FFTLEN             DSPROP_FFTLEN
#define DSP_MAXVAL             DSPROP_MAXVAL
#define DSP_RIFLG              DSPROP_RIFLG
#define DSP_COUPLIST           DSPROP_COUPLIST
#define DSP_GAINLIST           DSPROP_GAINLIST
#define DSP_FSMODE             DSPROP_FSMODE
#define DSP_TRIGLEVEL          DSPROP_TRIGLEVEL
#define DSP_BUFLEN             DSPROP_BUFLEN
#define DSP_HOSTBUFNUM         DSPROP_HOSTBUFNUM
#define DSP_BUFNUM             DSPROP_BUFNUM
#define DSP_SCALEIN            DSPROP_SCALEIN
#define DSP_OFFSETIN           DSPROP_OFFSETIN
#define DSP_WINSCL             DSPROP_WINSCL
#define DSP_PHZREQ             DSPROP_PHZREQ
#define DSP_DUPFLG             DSPROP_DUPFLG
#define DSP_FILTADDR1          DSPROP_FILTADDR1
#define DSP_CHANLIST           DSPROP_CHANLIST
#define DSP_TRIGCHANLIST       DSPROP_TRIGCHANLIST
#define DSP_SCALEOUT           DSPROP_SCALEOUT
#define DSP_OFFSETOUT          DSPROP_OFFSETOUT
#define DSP_FILTADDR2          DSPROP_FILTADDR2
#define DSP_MAXVALREAL         DSPROP_MAXVALREAL
#define DSP_MAXVALIMAG         DSPROP_MAXVALIMAG
#define DSP_FILTLEN1           DSPROP_FILTLEN1
#define DSP_FILTLEN2           DSPROP_FILTLEN2
#define DSP_LOGFLG1            DSPROP_LOGFLG1
#define DSP_LOGFLG2            DSPROP_LOGFLG2
#define DSP_FILTTYPE1          DSPROP_FILTTYPE1
#define DSP_FSVALUE            DSPROP_FSVALUE
#define DSP_PWRFLG1            DSPROP_PWRFLG1
#define DSP_PWRFLG2            DSPROP_PWRFLG2
#define DSP_PWRCOEFFA          DSPROP_PWRCOEFFA
#define DSP_PWRCOEFFB          DSPROP_PWRCOEFFB
#define DSP_XFERFLG1           DSPROP_XFERFLG1
#define DSP_XFERFLG2           DSPROP_XFERFKG2
#define DSP_MAXVAL1            DSPROP_MAXVAL1
#define DSP_MAXVAL2            DSPROP_MAXVAL2
#define DSP_NUMCHAN            DSPROP_NUMCHAN
#define DSP_BOARDSUBCLASS      DSPROP_CARDSUBCLASS
#define DSP_CARDSUBCLASS       DSPROP_CARDSUBCLASS
#define DSP_TIMDATAADDR        DSPROP_TIMDATAADDR
#define DSP_RIDATAADDR         DSPROP_RIDATAADDR
#define DSP_MAGDATAADDR        DSPROP_MAGDATAADDR
#define DSP_WINDATAADDR        DSPROP_WINDATAADDR
#define DSP_MONITORFLG         DSPROP_MONITORFLG
#define DSP_IOMODULE           DSPROP_IOMODULE
#define DSP_SERIALCTRL         DSPROP_SERIALCTRL
#define DSP_LOGCOEFFA          DSPROP_LOGCOEFFA
#define DSP_LOGCOEFFB          DSPROP_LOGCOEFFB
#define DSP_ADCOUNT            DSPROP_ADCOUNT
#define DSP_DACOUNT            DSPROP_DACOUNT
#define DSP_STMDATAADDR        DSPROP_STMDATAADDR
#define DSP_STMBUFLEN          DSPROP_STMBUFLEN
#define DSP_FILTTYPE2          DSPROP_FILTTYPE2
#define DSP_FILTQUANT          DSPROP_FILTQUANT
#define DSP_FILTUPDFLG         DSPROP_FILTUPDFLG
#define DSP_CODECCTRL1         DSPROP_CODECCTRL1
#define DSP_CODECCTRL2         DSPROP_CODECCTRL2
#define DSP_ISRADD             DSPROP_ISRADD
#define DSP_SYNCMODE           DSPROP_SYNCMODE

#define DSP_USERVAR            0x46F        /* user-defined variable start */


#define _QUOTE(x) # x
#define QUOTE(x) _QUOTE(x)
#define __FILE__LINE__ __FILE__ "(" QUOTE(__LINE__) ") : "
#define Todo( x )  message( __FILE__LINE__"TODO: " #x "\n" )


#if !defined(__WIN32__) && !defined(__MSVC__) && !defined(_LINUX_) && !defined(_WIN32_WINNT)
  #undef UINT
  #undef BOOL
#endif

/* APIs for allocating contiguous host memory and mapping said memory to C66x addresses */

/**
 *  @brief Function DSAllocHostContigMem() Allocate contiguous host
           memory; Any other contiguous memory allocation scheme can be used by
           applications.
 *  @param[in]     hCard              Card handle
 *  @param[in]     num_of_buffers     Number of buffers
 *  @param[in]     size_of_buffer     Size of buffer
 *  @param[in]     host_buf_type      Host Buffer type static or dynamic
 *  @param[out]    buf_desc	        Array of allocated buffers
 *  @retval        0: for success, -1 for failure 
 *  @pre  
 *  @post 
 **/
DWORD DSAllocHostContigMem(HCARD hCard, uint32_t num_of_buffers, uint32_t size_of_buffer, uint16_t host_buf_type, host_buf_desc_t buf_desc[]);

/**
 *  @brief Function DSFreeHostContigMem() Free contiguous dma host
 *         memory; 
 *  @param[in]     hCard              Card handle
 *  @param[in]     num_of_buffers     Number of buffers
 *  @param[in]     size_of_buffer     Size of buffer
 *  @param[in]     host_buf_type      Host Buffer type static or dynamic
 *  @param[out]    buf_desc			  Array of allocated buffers
 *  @retval        0: for success, -1 for failure 
 *  @pre  
 *  @post 
 **/
DWORD DSFreeHostContigMem(HCARD hCard, uint32_t num_of_buffers, uint16_t host_buf_type,  host_buf_desc_t buf_desc[]);

/**
 *  @brief Function DSAllocC66xAddr() Allocate chip outbound mem range
 *         The allocated areas can be used to map Host buffer, so that chip can 
 *         access the host  buffers directly
 *         If reserved memory is needed, alloc can be called at the beginning of 
 *         application execution and freed only at the end of application
 *  @param[in]    hCard           Card handle
 *  @param[in]    mem_size        size of chip outbound memory to allocate
 *  @param[out]   chip_start_addr Allocated chip start address
 *  @retval       0: success, -1 : failure
 *  @pre  
 *  @post 
 **/
DWORD DSAllocC66xAddr(HCARD hCard, uint32_t mem_size, uint32_t *chip_start_addr);

/**
 *  @brief Function DSFreeC66xAddr() Free chip outbound mem range
 * ( If reserved memory is needed, It is recommended to call alloc at the
 * beginning of application execution and this will avoid defragmentation 
 * due to repeated aloc and free)
 *  @param[in]    hCard           Card handle
 *  @param[in]    mem_size        size of chip outbound memory to allocate
 *  @param[out]   chip_start_addr Allocated chip start address
 *  @retval       0: success, -1 : failure
 *  @pre  
 *  @post 
 **/
DWORD DSFreeC66xAddr(HCARD hCard, uint32_t mem_size, uint32_t chip_start_addr);

/**
 *  @brief Function DSMapHostMemToC66xAddr() Map host buffers to the 
 *         allocated chip outbound memory range 
 *  @param[in]    hCard           Card handle
 *  @param[in]    num_of_bufs     Number of host buffers
 *  @param[in]    buf_desc        list of host buffer descriptors 
 *  @param[out]   chip_start_addr chip start address to map 
 *  @retval       0: success, -1 : failure
 *  @pre  
 *  @post 
 **/
DWORD DSMapHostMemToC66xAddr(HCARD hCard, uint32_t num_of_bufs, host_buf_desc_t buf_desc[], uint32_t chip_start_addr);

#endif
