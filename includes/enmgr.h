/*
  $Header: /Signalogic/DirectCore/include/enmgr.h     10/10/05 12:32p
 
  Description: header file for Engine Manager
  
  Project: DirectCore
 
  Copyright Signalogic Inc. 1994-2017
  
  2     10/10/05 12:32p
  8 DSP support

  Revision History:
  
   Created  1994
   Modified 2002-2010
   Modified Aug 2017 JHB, replaced C++ style comments with C style for compatibility with ISO C90 sources, removed references to SWM_SCOPE
*/

#ifndef _ENMGR_H_
#define _ENMGR_H_


#include "alias.h"  /* Win16 / Win32 / Linux normalization */

#ifdef _USE_GUI_
/* Standard Xlib includes */
   #include <X11/Xlib.h>
   #include <X11/X.h>
   #include <X11/Xcms.h>
   #include <X11/Xutil.h>
   #include <X11/Xresource.h>
   #include <X11/Xatom.h>
   #include <X11/cursorfont.h>
   #include <X11/keysymdef.h>
   #include <X11/keysym.h>
   #include <X11/X10.h>
#endif

#ifndef _CIM
   #include <unistd.h>
#endif
   #include <pthread.h>
   #include <ctype.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <sys/poll.h>
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <errno.h>
   #include <fcntl.h>


/* library / DLL call prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/* initialize library (also done automatically by DSEngineOpen and DSAddEngMgrStatusLine) */

#ifndef _USE_GUI_

HWND       LIBAPI DSInitEngMgr               (void);

/* enmgr DLL status */

short int  LIBAPI DSGetEngMgrErrorStatus     (void);

/* engine manager error/status window */

void       LIBAPI DSShowEngMgrStatusWindow   (void);
HWND       LIBAPI DSAddEngMgrStatusLine      (LPCSTR);
void       LIBAPI DSHideEngMgrStatusWindow   (void);

#endif

/* engine functions */

HENGINE    LIBAPI DSEngineOpen               (LPCSTR, LPCSTR, UINT);
UINT       LIBAPI DSEngineClose              (HENGINE);
#define DSDriverOpen DSEngineOpen
#define DSDriverClose DSEngineClose
/*
#define DSLibInit DSEngineOpen
#define DSLibClose DSEngineClose
*/

#ifndef _LINUX_

HENGINE    LIBAPI DSExecEngine               (LPCSTR, UINT);
UINT       LIBAPI DSGetEngineUsage           (void);
UINT       LIBAPI DSExecEngineFunction       (HENGINE, LPCSTR, UINT);

#endif

short int  LIBAPI DSGetEngineErrorStatus     (HENGINE);

#ifndef _LINUX_
UINT       LIBAPI DSGetEnginePath            (HENGINE, LPSTR);
UINT       LIBAPI DSGetEngineWaveformPath    (HENGINE, LPSTR);
UINT       LIBAPI DSGetEngineState           (HENGINE);

UINT       LIBAPI DSSendEngineCommand        (HENGINE, short int, UINT);
UINT       LIBAPI DSRegisterEngineMsgWnd     (HENGINE, UINT, HWND);
UINT       LIBAPI DSUnregisterEngineMsgWnd   (HENGINE, UINT);
UINT       LIBAPI DSSetEngineWaveformPath    (HENGINE, LPCSTR);
UINT       LIBAPI DSSetEngineTemplatePath    (HENGINE, LPCSTR);

#endif

UINT       LIBAPI DSAppErrMsg(HWND, LPCSTR, LPCSTR, DWORD);
void       LIBAPI DSSleep(DWORD);

#ifdef _USE_GUI_

/* Show the engine manager window on the screen/intitialize engine manager */
void DSInitEngMgr(void);

/* Function to add a line to the status window */
BOOL DSAddEngMgrStatusLine(const char str[128]);

/* Show/hide the status window */
void DSShowEngMgrStatusWindow(void);
void DSHideEngMgrStatusWindow(void);

/* Set flags for the status window */
void DSSetEngMgrStatusFlags(DWORD dwFlags);

#endif

/* Shut down engine manager */
void DSShutdownEngMgr(void);

/* Function to poll for message events from the LKM */
void *LKM_Message_Thread(void *unused);

void *LKM_Message_Handling_Thread(void *unused);

/* Function to clear status window and status buffer */
BOOL ClearStatusBuffer(void);

/* Function to strip out anything not text, numbers, or punctuation */
void ClearStringNonPrint(char *str);

BOOL AddLKMPollItem(HBOARD hBoardx, int Bufnumber, int dspaddr, void *cbfunc);

/* Get the PID of the status window */
pid_t GetStatusWindowPid(void);


#ifdef __cplusplus
}
#endif

/* DSP Engine messages; window must be registered with DSRegisterEngineMsgWnd to receive */

#define WM_DSPENGINE_CMDDATA         WM_USER      /* engine data/command queue needs attention */

#define WM_DSPENGINE_BUFRDY          WM_USER+1    /* buffer ready */
#define WM_DSPENGINE_FLGRDY          WM_USER+2    /* flag ready */

/* program status messages */

#define WM_DSPENGINE_PROGSTATUS      WM_USER+3    /* engine program status (function done, program done, etc) */

/* engine status messages */

#define WM_DSPENGINE_ENGINESTATE     WM_USER+5    /* engine state has changed */
#define WM_DSPENGINE_FUNCTIONERROR   WM_USER+6    /* engine encountered function input or setup error */
#define WM_DSPENGINE_CODEGENERROR    WM_USER+7    /* engine encountered code generation error */
#define WM_DSPENGINE_COMPILEERROR    WM_USER+8    /* engine encountered compile error */
#define WM_DSPENGINE_RUNTIMEERROR    WM_USER+9    /* engine encountered run-time error (waveform file error, hardware problem, etc) */

/* engine statistics/parameter messages */

#define WM_DSPENGINE_BUFSIZE         WM_USER+10   /* current buffer size */
#define WM_DSPENGINE_SAMPFREQ        WM_USER+11   /* actual sampling frequency */

#define WM_DSPENGINE_BUFNUM          WM_USER+12   /* current buffer number */

#define WM_DSPENGINE_LEVEL           WM_USER+13   /* current amplitude level */

/* DSP code/data monitor message */

#define WM_UPDATEMONITOR             WM_USER+14   /* msg sent to DSP code/data monitor windows (wParam has tick rate in msed, lParam has current time count) */


#define WM_DS_USER                   WM_USER+15   /* next available user-defined message for apps using enmgr.h */


/* wParam values in WM_DSPENGINE_PROGSTATUS messages */

#define DS_PS_FUNCDONE               0            /* function is done */
#define DS_PS_PROGDONE               1            /* program is done */


/* DSP engine states sent in wParam of WM_DSPENGINE_ENGINESTATE message */

#define DS_ES_RUNNING                32
#define DS_ES_IDLE                   33
#define DS_ES_STOPPENDING            1
#define DS_ES_STOPPED                -1


/* error codes */
#include "dllerr.h"                               /* include general DSPower DLL error messages */
   
#define DSCOULDNOTOPENENGINE         2            /* engine-related error messages */
#define DSINVALIDENGINEPATH          1
#define DSENGINETIMEOUT              -12
#define DSCOULDNOTOPENPIPE           -13
#define DSCOULDNOTSTARTENGINE        -14
#define DSNOENGINESOPEN              -15
#define DSNOENGINESFOUNDINLIST       -16
#define DSENGINEHANDLEWASNULL        -17
#define DSENGINEMEMORYDISCARDED      -18

/* possible default engine types for use in DSEngineOpen call */

#define DS_EO_HSM                    (LPCSTR)1L   /* Hypersignal-Macro 4.x (real-time) */
#define DS_EO_HSA                    (LPCSTR)2L   /* Hypersignal-Acoustic 4.x */
#define DS_EO_HSMNRT                 (LPCSTR)3L   /* Hypersignal-Macro NRT 4.x (non-real-time) */
#define DS_EO_MAT                    (LPCSTR)4L   /* MATLAB 4.x */

#define DS_EO_NOTVISIBLE             0
#define DS_EO_VISIBLE                1
#define DS_EO_SYNC                   2            /* EngineOpen call waits for engine to complete; close is called automatically */

/* ExecEngineFunction constants */

#define DS_EEF_ASYNC                 0            /* calling app continues while engine function executes */
#define DS_EEF_SYNC                  1            /* app waits for engine function to complete */


/* ExecEngine constants */

#define DS_EE_NOTVISIBLE             0
#define DS_EE_VISIBLE                1
#define DS_EE_ASYNC                  2            /* app continues to run while engine runs */
#define DS_EE_SYNC                   4            /* app waits for engine to complete and close */


/* SendEngineCommand constants */

#define DS_SEC_ABORT                 -1
#define DS_SEC_IDLE                  -2
#define DS_SEC_PAUSE                 -3
#define DS_SEC_RESUME                -4


/* RegisterEngineMsgWnd constants */

#define DS_REMW_ENGINESTATUSMSG      0x0001       /* engine status messages */
#define DS_REMW_PROGSTATUSMSG        0x0002       /* program status messages */
#define DS_REMW_DSPDATARDYMSG        0x0004       /* DSP buffer ready and flag ready callbacks */
#define DS_REMW_STATISTICSMSG        0x0008       /* DSP/analog hardware statistics/parameters messages */
#define DS_REMW_BUFNUMMSG            0x0010       /* current buffer count messages */
#define DS_REMW_LEVELMSG             0x0020       /* current data level messages */
#define DS_REMW_ALLMSG               0x00ff       /* set all messages to one control */


/* monitor-related items */

#define DS_IDMONITORTIMER            1000         /* ID of master timer */
#define DS_MONITORTIMERTICK          100          /* current master timer tick, in msec */
#define DS_MONITORMAXDUTYCYCLE       200.0        /* max amount of time "update-in-progress" LED can be on */
#define szMonitorTitle               "DSPower-Monitor"
#define szMonitorClass               "DBDMonitor"


/* structure pointed to be lParam in WM_DSPENGINE_BUFRDY and WM_DSPENGINE_FLGRDY messages */

typedef struct {

   short int  nNum;                /* buffer or flag number */
   DWORD      dwFlagValue;         /* flag value */ 
   DWORD      dwFlagAddr;          /* flag address */
   HGLOBAL    hBoard;              /* board handle */

} DSPDATARDYINFO, FAR* PDSPDATARDYINFO;


/* structure pointed to be lParam in WM_DSPENGINE_COMPILEERR and WM_DSPENGINE_RUNTIMEERR messages */

typedef struct {

   short int  nErrorNum;           /* buffer or flag number */
   char       szErrorStatus[256];  /* error/status string or error log filename */

} ERRORMSGINFO, FAR* PERRORMSGINFO;



/* Flag values for DSSetEngMgrStatusFlags */

#define ESW_SHOW 0x01
#define ESW_HIDE 0x02
#define ESW_DEBUG 0x04
#define ESW_DEBUGOFF 0x08


typedef void cbFunc(void);

/* data structure that contains all the info that the LKM_Message_Thread needs to poll a board for a variable */

typedef struct sLKM_PollIte3m   {

   BOOL bPoll;
   int nbufNum;
   int flagAddr;
   HBOARD hBoard;
   cbFunc *cbf;

} LKMPollItem;

/* data structure that contains all the info that the Message Handling Thread needs to tell EngMgr to execute  a callback function */
typedef struct sENMGR_RunItem   {

   BOOL bRun;
   HBOARD hBoard;
   cbFunc *cbf;

} ENMGR_RunItem;

#ifdef _USE_GUI_

typedef struct sWindowParams {

   Display  *display;         /* XWindows display */
   Window   rootWindow;       /* Our window */
   GC       gc;               /* Graphics context */
   XEvent   e;                /* Event handling information */
   int      whiteColor,       /* White Colour */
            blackColor,       /* Black Colour */
            screenTopX,       /* Top Left X Screen Coord */
            screenTopY,       /* Top Left Y Screen Coord */
            screenWidth,      /* Width of Screen */
            screenHeight,     /* Height of Screen */
            borderWidth,      /* Width of Window Border */
            rootWindowID;     /* ID of Root Window (Parent of Window) */
} WindowParams;

#endif

#endif  /* _ENMGR_H_ */
