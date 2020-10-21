/*
 $Header: /Signalogic/DirectCore/include/hwmgr.h     10/10/05 12:32p
 
  Description: HwMgr specific definitions
 
 
  Project: DirectCore
 
  Copyright Signalogic Inc. 1994-2013
  
  2     10/10/05 12:32p
  8 DSP support.

  Revision History:
  
    Created:  1994
    Modified: 2002-2016
*/
 
#ifndef _HWMGR_H_
#define _HWMGR_H_

// temporary "strict" literals for Win16 BOOL and UINT

#if !defined(__WIN32__) && !defined(_LINUX_)
  #ifndef RC_INVOKED
    #define BOOL short int
    #define UINT unsigned short int
  #endif
#endif

// Win16 / Win32 import definition

#if !defined(LIBAPI)
  #if !defined(__WIN32__)
    #define LIBAPI WINAPI
  #else
    #ifdef _MSC_VER
      #define LIBAPI __stdcall
    #else
      #define LIBAPI __stdcall
    #endif
  #endif
#endif


// supporting definitions for hardware setup

typedef char DESCSTR[52];               // descriptors:  mfg, description, designator
typedef char VALSTR[12];                // values:  base addresses, clock rates, etc.

#define _HWMGR_MAXPATH 256

typedef struct HWLISTENTRY_s {          // list of valid DSP/Analog hardware entries discovered when reading board registration files; e.g. hwsetup.lst

   DESCSTR    szMfg;                    // manufacturer
   DESCSTR    szDescription;            // description:  model/name, DSP device, etc.
   DESCSTR    szCardDesignator;         // card designator
   short int  NumModuleSites;           // maximum number of module sites supported
   short int  MajorDriverID;            // major driver classification for board
   short int  MinorDriverID;            // minor driver classification
   char       szCodeFile[144];          // current DSP program file (filename of DSP executable)
   char       szCodeFileDef[144];       // default DSP program file
   VALSTR     szIOBaseAddr;             // current base I/O address (in hex)
   VALSTR     szIOBaseAddrDef;          // default base I/O address
   VALSTR     szMemBaseAddr;            // current base memory address (in hex)
   VALSTR     szMemBaseAddrDef;         // default base memory address
   short int  BusType;
   short int  BusTypeDef;
   VALSTR     szProcClock;              // processor clock (in MHz)
   VALSTR     szProcClockDef;
   VALSTR     szVoltageRanges;          // voltage ranges (should be stored as "input,output")
   VALSTR     szVoltageRangesDef;
   short int  AnalClockType;            // analog clock:  0 = ext, 1 = fixed int, 2 = prog int
   short int  AnalClockTypeDef;
   VALSTR     szMemArch;                // memory architecture
   VALSTR     szMemArchDef;
   short int  NumProcessors;            // number of processors
   short int  NumProcessorsDef;
   short int  maxNumCores;              // max number of cores allowed (note -- cores <> number of processors / CPUs)
   DWORD      ModuleConfig;             // module configuration: 0x7766554433221100
                                        // where 00 denotes module at site0, 11 denotes module at site1
                                        // e.g. for IIM44-AIX-A4D4, ModuleConfig = 0x0305
   DWORD      ModuleConfigDef;          // default module config (0x00)

   char       szReserved[4];            // reserved area

/* following parameters not read from file: */

   short int  CallClass;                // calls-allowed classification
   short int  Flags;                    // internal info used to mark pending updates, deletes, etc.

/* following list file parameters added to v4.0 DirectCore */

   char       szUserDefinedDriver[_HWMGR_MAXPATH]; // storage for name of user-defined driver

   DESCSTR    szLocalMACAddr;  /* MAC addrs added in process of merging old 5561 lib into current DirectCore sw, JHB May10 */
   DESCSTR    szRemoteMACAddr;

   WORD       wMaxBufferSize;           // max bus transfer size buffer, set using DSAssignBoard (added Jul12)

/* currently not used, handled by uOptions param in DSAssignCard.  Possiby use at some point if a GUI dialog for hardware setup should be used again.  JHB Feb 2016 */

   WORD       wResetAttributes;

} HWLISTENTRY;

typedef HWLISTENTRY FAR* PHWLISTENTRY;

typedef HWLISTENTRY HWList[1];        // list of valid DSP/Analog hardware entries discovered when reading library hwsetup.lst file


/* library (DLL or .so) prototypes */

#ifdef __cplusplus
extern "C" {
#endif

// initialize library (also done automatically by DSShowHardwareSelector and DSReadHWSetupFile)

HWND       LIBAPI DSInitHWMgr();

// hwmgr status

short int  LIBAPI DSGetHWMgrErrorStatus();

// hwmgr functions

short int  LIBAPI DSGetHWMgrNumEntries();

short int  LIBAPI DSGetHWMgrEntryIndex(LPCSTR);  // get index of entry matching specified board designator string

UINT       LIBAPI DSGetHWMgrEntry(short int, PHWLISTENTRY);  // read entry into HWLISTENTRY structure
UINT       LIBAPI DSSetHWMgrEntry(short int, PHWLISTENTRY);  // write entry from HWLISTENTRY structure
UINT       LIBAPI DSGetHWMgrEntryEx(short int, PHWLISTENTRY, int);  // Ex versions include sizeof(HwListEntry) parameter
UINT       LIBAPI DSSetHWMgrEntryEx(short int, PHWLISTENTRY, int);

UINT       LIBAPI DSGetHWMgrCurVal(LPSTR);       // get board designator string of current hardware value
UINT       LIBAPI DSSetHWMgrCurVal(LPCSTR);      // set current hardware value to board designator string

UINT       LIBAPI DSGetHWMgrVar(WORD, LPSTR);    // get board designator string from specified hardware variable
UINT       LIBAPI DSSetHWMgrVar(WORD, LPCSTR);   // set board designator string of specified hardware variable

HGLOBAL    LIBAPI DSReadHWSetupFile(LPCSTR);     // read hardware setup file
UINT       LIBAPI DSWriteHWSetupFile(HGLOBAL);   // write hardware setup file

long       LIBAPI DSShowHardwareSelectorDlg(HWND, LPSTR);  // show the hardware selector dialog

UINT       LIBAPI DSSetSetupFilename(LPCSTR);
UINT       LIBAPI DSGetSetupFilename(LPSTR);

BOOL       LIBAPI DSGetBoardDriver(LPCSTR, LPSTR, int);

#ifdef __cplusplus
}
#endif


// error codes

#define DSUNABLETOREADHWSETUPFILE   -100
#define DSCOULDNOTLOCKHWLIST        -101
#define DSBOARDDESIGNATORNOTLISTED  -102
#define DSCOULDNOTFINDHWSETUPFILE   -103
#define DSCOULDNOTINCREASEHWLISTMEM -104
#define DSHWSETUPFILENOTYETREAD     -105


// DSP/analog "variables" that are referred to by source code generation; this scheme allows
// multiple boards to be in use at the same time, because each variable can be assigned a
// different hardware value

#define MAXBOARDVARS                14   // maximum number of hardware "variables" that user can refer to (each variable contains one of the board types)

#define DS_GHV_RTE                  0    // real-time engine
#define DS_GHV_SA                   1    // spectrum analyzer
#define DS_GHV_ACC                  2    // DSP accelerator
#define DS_GHV_DTR                  3    // digital tape recorder
#define DS_GHV_ACQ                  4    // data acquisition, waveform file record
#define DS_GHV_CSG                  5    // continuous signal generation
#define DS_GHV_SR                   6    // stimulus & response
#define DS_GHV_PB                   7    // waveform file playback
#define DS_GHV_DO                   8    // digital oscilloscope
#define DS_GHV_UD1                  9    // user-defined 1-5
#define DS_GHV_UD2                  10
#define DS_GHV_UD3                  11
#define DS_GHV_UD4                  12
#define DS_GHV_UD5                  13

// hardware categories that determine which calls into HWLib are allowed for each entry

#define CALLCLASS_NODSP             1    // calls-allowed classfications
#define CALLCLASS_NOANALOG          2
#define CALLCLASS_NOPROCCALLS       4

// module selector constants

#define MODULE_SITE0                0
#define MODULE_SITE1                MODULE_SITE0+0x100

// OMNIBUS module subtypes

#define MODULE_NONE                 0
#define MODULE_A4D1                 1
#define MODULE_A16D2                2
#define MODULE_A4D4                 3
#define MODULE_AD40                 4
#define MODULE_AIX                  5
#define MODULE_AIX20                6
#define MODULE_DAC40                7
#define MODULE_DIG                  8
#define MODULE_SD4                  9
#define MODULE_SD16                 8
#define MODULE_A16D16               10
#define MAX_MODULE_SITES            2     // maximum number of modules sites
#define MAX_PROC_CLOCK              720   // maximum clock speed in MHz


#if !defined(__WIN32__) && !defined(_LINUX_)
  #undef BOOL
  #undef UINT
#endif

#endif

