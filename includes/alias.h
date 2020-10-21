/*
  $Header: /root/Signalogic/DirectCore/include/alias.h 2     10/10/05 12:32p

  Description: alias definitions and typedefs to allow unified support for Linux, Win32, and Win64

  Project: DirectCore lib and driver

  Copyright Signalogic Inc. 1994-2020

  Revision History

   2     10/10/05 12:32p
   8 DSP support
  
   1     8/24/05 7:13p
   Clean up code and rebuild
 
   Revised JHB Nov-Dec 2008, clean-up during SigC641x work
   Revised JHB Jul 2010, clean up during SigC5561 work (rename msgbuf typedef to sig_msgbuf)
   Revised JHB Jul 2014, moved _GCC_VERSION macro here, defined INT, typedef socket, etc.
   Revised JHB Jun 2017, added bool type if __cplusplus is not defined
   Revised JHB Aug 2017, protected some codes with #if __STDC_VERSION__ >= 199901L to allow compatibility with C90 sources (for example 3GPP codec sources)
   Revised JHB Oct 2018, changed definition of "bool" for non C++ codes to uint8_t.  See comments
*/

 
#ifndef _ALIAS_H_
#define _ALIAS_H_

#define BASE0 0
#define BASE1 1

#ifdef _WIN32_WINNT
   extern "C" {
      #include <ntddk.h>
      #include <stdio.h>
      #include <stdlib.h>
   }
   #include "Unicode.h"

   void ntknlDebugPrint(char * str);
#endif

#ifdef __NTMODE__
   #include "rwhead.h"
#endif

#ifndef ROUND
   #define ROUND(x) floor(x + 0.5f)
#endif

#if defined (_LINUX_) && !defined (__KERNEL__)
   #include "minmax.h"
#endif

#if defined (__LIBRARYMODE__)
   #define MAXPATH 260
   #define MAXFILE 260

   #if defined (__MSVC__) || defined (_WIN32_WINNT)
      #define huge
      #define far
   #endif
#endif

#if defined (_LINUX_) || defined (_WIN32_WINNT)
   #define lstrcpy strcpy
   #define lstrcmp strcmp
   #define lstrcat strcat
   #define lstrlen strlen
#endif

/* #include <linux/version.h> */

#if defined (_LINUX_)

   #define _GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)  /* Modified:  Added GCC version Macro to compare it with version >=4.5 to avoid pragma error in GCC versions < 4.5. HP, SC, Oct 2014 */

   #define MAX_INPUT_LEN 	256
   
   #ifndef __KERNEL__
   
      #include <stdint.h>
		#include <string.h>
		#include <stdio.h>
      #include <inttypes.h>
      
/* changed from sys/io.h to /usr/include/sys/io.h YT 16JUL08 // inb, outb, etc. */
	
      #if defined(__ppc__) || defined(powerpc) || defined(ppc) || defined(_ARCH_PPC)
/*    #if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,11)) */
  		   #include </usr/src/linux/include/asm-powerpc/io.h>
	   #else
  		   #include <sys/io.h>
	   #endif

/* End of Mod YT 16JUL08 */

      typedef int SOCKET; /* adding SOCKET typedef AKM 04/24/2015 */

   #else
      #include <linux/kernel.h>   /* printk() */
      #include <linux/random.h>   /* get_random_bytes() */
      #include <linux/delay.h>    /* mdelay() */
      #include <linux/sched.h>
   #endif
   
   #include <linux/types.h> 	/* pid_t */

   #undef NULL
   #define NULL 0

	/* #define BOOL unsigned short int */
	#define BOOL unsigned int
	#ifndef FALSE
		#define FALSE 0
	#endif
	
	#ifndef TRUE
		#define TRUE 1
	#endif

   #define Boolean BOOL

   #ifndef __cplusplus  /* added JHB Jun2017 */
#if 0
      #define bool   BOOL
#else
      #define bool   uint8_t  /* made one byte to be consistent across libs, apps, C and C++, JHB Oct 2018 */
#endif
      #define false  0
      #define true   1
	#endif

	#ifndef EOS
		#define EOS '\0'
	#endif

/* **************************** Turn debug msg on or off ********************* */

   #undef PDEBUG
	#ifdef DEBUGON
		#ifdef __KERNEL__
			#define PDEBUG(fmt, args...) printk(KERN_DEBUG "driver: " fmt , ##args)
		#else
			#define PDEBUG(fmt, args...) if (globalDebugLevel >= 2) printf(fmt , ##args)
		#endif
	#else
      #if __STDC_VERSION__ >= 199901L  /* check if C90 is being used (for example 3GPP sources) */
		  #define PDEBUG(fmt, args...)
      #endif
	#endif

	#undef PDEBUGG
   #if __STDC_VERSION__ >= 199901L  /* check if C90 is being used (for example 3GPP sources) */
   #define PDEBUGG(fmt, args...)
   #endif
/* *************************************************************************** */

   #define INT          int32_t
#if 0
   #define UINT         unsigned int
   #define UINT16       unsigned short int
   #define DWORD        unsigned int
   #define QWORD        unsigned long long
   #define HBOARD       long int
   #define HCARD        long int
   #define HPLATFORM    long int
   #define HCODEC       long int
   #define HSESSION     long int
#else 
   #ifndef UINT
     #define UINT       uint32_t
   #endif
   #ifndef UINT16
     #define UINT16     uint16_t
   #endif
   #ifndef DWORD
     #define DWORD      uint32_t
   #endif
   #define QWORD        uint64_t
   #define HBOARD       int32_t
   #define HCARD        int32_t
   #define HPLATFORM    int32_t
   #define HCODEC       int32_t
   #define HSESSION     int32_t
   #define HDATAPLANE   int32_t
#endif

   #define HANDLE       void*
   #define HGLOBAL      HANDLE
   #define HINSTANCE    HANDLE
   #define HTASK        HANDLE
   #define HENGINE      HANDLE
   #define PENGINE      ENGINE*

   #define MAXQWORDBITS (sizeof(QWORD)*8)
   
   #define MAX_NUM_CHAN	8		/* Max of channel numbers */
   #define HWND         void *
   #define RECT         void *
   #define HHOOK        int
   #define LPVOID       void *

   /* GlobalAlloc constants */
   #define GMEM_ZEROINIT           0x0040
   #define GMEM_MOVEABLE           0x0002
   #define GMEM_DISCARDABLE        0x0100

   #define GlobalUnlock(hMem) hMem = hMem
   #define GlobalLock(hMem) hMem
   #define GlobalFree(hMem) MemFree(hMem);
   #define GlobalAlloc(uFlags, dwBytes) MemGet(uFlags, dwBytes);

   #define farcalloc calloc
   #define farfree free

   #ifdef __KERNEL__    /* LKM Has No Need For "Pass Through" Function */
   
     	#define outp(port, value) outb(value, port)
     	#define inp inb

   	#ifdef _SIGFMOD_MAIN_
     		#define  SIGFM_SCOPE
   	#else
     		#define  SIGFM_SCOPE  extern
   	#endif

     	typedef int StatusLineFunc(const char sz[128]);

     	#ifndef _GLOBALS_  /* Note: globals are actually defined in alias.cpp */
        	#define _GLOBALS_
        	/* PID of process we're calling for engine manager status messages */
        	extern pid_t EngMgrPid;

        	/* Engine manager message queue */
        	#define ENGMGR_QUEUE_SIZE 10

        	/* Actual engine manager queue */
        	extern char sEngMgrMessages[ENGMGR_QUEUE_SIZE][128];

        	/* Are we printing debug messages to the syslog? (defaults to false) */
        	extern BOOL bPrintDebug;

     	#endif /* ! defined _GLOBALS_ */

     	/* Add a message to the queue go out */
     	void DSAddEngMgrStatusLineB(char *Message);

     	/* Add the given message to the top of the engine manager queue */
     	void AddMessageToEngMgrQueue(char *Message);

     	/* See if the message queue is empty */
     	BOOL IsEngMgrQueueEmpty();

     	/* Get the topmost message on the queue */
     	char *GetTopEngMgrMessage();

     	/* Remove the topmost message on the queue */
     	void DeleteTopEngMgrMessage();

  	#else                /* SO Requires This to Access Hardware */
   
     	#define outp SigLKM_Write
     	#define inp SigLKM_Read
      
  	#endif

  	#define huge
  	#define FAR
  	#define far

  	#define LIBAPI
  	#define WINAPI
  	#define FARPROC void *  /* This may or may not be correct... -CJS */
  	#define DECLSPEC

  #ifndef WORD
   #define WORD unsigned short int
  #endif
   
   typedef long int LONG;

   #ifndef TCHAR
    	#define TCHAR char
   #endif

   #define LPCSTR const TCHAR*
   #define LPSTR TCHAR*

   #define BYTE unsigned char

   #define HFILE  FILE *
   #define HFILE_ERROR  (HFILE)-1

   #define _llseek fseek
   #define _lread(file, ptrdata, sizeinbytes) fread(ptrdata, sizeinbytes, 1, file)
   #define _lopen(filename, readwrite) fopen(filename, readwrite)
   #define _lclose(file) fclose(file)

   #define _itoa ltoa
   #define _ltoa ltoa
   #define itoa ltoa
   #define _fstrchr strchr
   #define _fstrrchr strrchr
   #define _fmemset memset
   #define _fstrlen strlen
   #define _fmemcpy memcpy
   #define _fstrcpy strcpy
   #define _fstrcat strcat
   #define _fstrupr strupr
   #define _fstrstr strstr
   #define _fstricmp stricmp

   /* Pulled from windef.h */
   #define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
   #define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
   #define LOWORD(l)           ((WORD)(l))
   #define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
   #define LOBYTE(w)           ((BYTE)(w))
   #define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))

   #define IDOK                1
   #define IDCANCEL            2
   #define IDABORT             3
   #define IDRETRY             4
   #define IDIGNORE            5
   #define IDYES               6
   #define IDNO                7
   #define IDCLOSE             8
   #define IDHELP              9
   #define IDTRYAGAIN         10
   #define IDCONTINUE         11
   
   #ifdef __cplusplus
   extern "C" {
   #endif

   /* Prototypes for otherwise non-existent functions */
   char *ltoa(long int value, char *string, long int radix);
   char *strupr(char *string);
   char *strlwr(char *string);
   int lstrcmpi(char *string1, char *string2);

   #ifdef __cplusplus
   }
   #endif

   int MemFree(void *memblock);  /* Version of ANSI C free that returns TRUE */
   void *MemGet(UINT flags, DWORD NumBytes);  /* Used for GlobalAlloc alias */

   #ifdef __KERNEL__
     	#define Win32Sleep(val) mdelay(val / 10)
   #else
     	#define Win32Sleep(val) usleep(val * 1000)
   #endif

/* File pointer/function redefinitions in kernel mode (redefining as memory pointers/accessors) */
	#ifdef __cplusplus
   		extern "C"  {
	#endif
   	void SigLKM_Write(int, int);
   	int SigLKM_Read(int);
	#ifdef __cplusplus
   		}
	#endif

   #ifdef __KERNEL__
     	#define FILE BYTE
     	#define EOF 1
     	#define fopen mopen
     	#define fclose mclose
     	#define fread mread
     	#define fwrite mwrite
     	#define fseek mseek

     	#define SEEK_SET 0
     	#define SEEK_CUR 1
     	#define SEEK_END 2
   #endif

   /* Message buffer for EngMgr message queue messages */
   typedef struct message_buf {
   
    	long    mtype;
      char    mtext[128];
      
   } sig_msgbuf;

/* C library functions we need in kernel mode (and don't exist otherwise) */

   void SetMemPtr(BYTE *pMem, long Size);

   #ifdef __KERNEL__  /* only used in old style driver */
   	int atoi(const char *string);
     	FILE *mopen(const char *filename, const char *mode);
     	int mclose(FILE *stream);
     	size_t mread(void *buffer, size_t size, size_t count, FILE *stream);
     	size_t mwrite(const void *buffer, size_t size, size_t count, FILE *stream);
     	int mseek(FILE *stream, long offset, int origin);
     	int rand();
     	double pow(double, double);  /* raises parameter 1 to the power of parameter 2 */
   #endif

#endif /* _LINUX_ */

#if !defined(__WIN32__) && !defined (_WIN32_WINNT) && !defined (_LINUX_)
   #define _itoa itoa
   #define _ltoa ltoa
#endif


#if defined (__LIBRARYMODE__)

  #if defined(DEBUGON) 
    #if defined(__KERNEL__)
      #define debugPrint printk 
    #else
      #define debugPrint(a, b...) printf(a , ##b)
    #endif
  #else
    #define debugPrint(a, b...)	
  #endif

	
  #if defined (__DEBUG__)
    #define DSAddEngMgrStatusLine AddStatusLine
  #elif defined _WIN32_WINNT
    #define DSAddEngMgrStatusLine DebugPrint
  #elif defined __MSVC__
    int DSAddEngMgrStatusLine(char* sMsg);
  #elif !defined __KERNEL__  
 /* #define DSAddEngMgrStatusLine printf */
    #define DebugPrint printf
    extern int GetFileDescriptor(HBOARD);
  #elif defined(DEBUGON) 

    #ifdef __KERNEL__
      #define DebugPrint printk
      #define DSAddEngMgrStatusLine printk
      extern int GetFileDescriptor(HBOARD);
			
    #endif
/*  #elif !defined(DEBUGON)
    #define DSAddEngMgrStatusLine AddStatusLine 
    #define DebugPrint AddStatusLine
    extern int GetFileDescriptor(HBOARD);
*/
  #endif

#else

  #define debugPrint	printf 

#endif

#if defined (__LIBRARYMODE__)

   #ifndef _LINUX_
      char* GetFilePath(char* szPath);
   #endif

	#if defined (__MSVC__) || defined (_WIN32_WINNT)
      #define Win32Sleep Sleep
      #define _fmemset  memset
      #define _fstrlen  strlen
      #define _fmemcpy  memcpy
      #define _fstrrchr strrchr
      #define _fstrchr  strchr
   #endif
#else
   #ifndef _LINUX_      /* Add Here By Uncaught Problems When Compiling In Linux -AG */
	   void Win32Sleep(unsigned long dwMsec);
   #endif
#endif

#ifndef __NTMODE__
  #if defined(__MSVC__) 
    #define outp _outp
    #define inp _inp
    #define inpw  _inpw
    #define outpw _outpw
  #endif
#endif

#if defined (_WIN32_WINNT)	

   #undef NULL
   #define NULL 0

   #define BOOL int
   #define FALSE 0
   #define TRUE 1

   #define Boolean BOOL

   #define UINT unsigned long int
   #define WORD unsigned short int
   #define DWORD unsigned long 
   #define HENGINE   HANDLE
	#define HBOARD    HANDLE
	#define HCARD     HANDLE
   #define PENGINE   ENGINE *
   #define HFILE     HANDLE
   #define HGLOBAL   HANDLE

   typedef long int LONG;

   #define LPCSTR const TCHAR*
   #define LPSTR TCHAR*

   #define BYTE unsigned char

   #define HFILE_ERROR  (HFILE)-1

   #define _llseek fseek
   #define _lread(file, ptrdata, sizeinbytes) fread(ptrdata, sizeinbytes, 1, file)
   #define _lopen(filename, readwrite) fopen(filename, readwrite)
   #define _lclose(file) fclose(file)
   #define GlobalUnlock(hMem) hMem = hMem
   #define GlobalLock(hMem) hMem
   #define GlobalFree(hMem) MemFree(hMem);
   #define GlobalAlloc(uFlags, dwBytes) MemGet(uFlags, dwBytes);

   #define _fstrchr strchr
   #define _fstrrchr strrchr
   #define _fmemset memset
   #define _fstrlen strlen
   #define _fmemcpy memcpy

   /* Pulled from windef.h */
   #define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
   #define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
   #define LOWORD(l)           ((WORD)(l))
   #define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
   #define LOBYTE(w)           ((BYTE)(w))
   #define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))

   /* Prototypes for otherwise non-existent functions */
   int lstrcmpi(char *string1, char *string2);
   
   int MemFree(void *memblock);  /* Version of ANSI C free that returns TRUE */
   void *MemGet(UINT flags, DWORD NumBytes);  /* Used for GlobalAlloc alias */


   /* File pointer/function redefinitions in kernel mode (redefining as memory pointers/accessors) */
   #define outp(port, value) WRITE_PORT_UCHAR((PUCHAR) port, (UCHAR) value)
   #define inp(port) READ_PORT_UCHAR((PUCHAR) port) 
   
#endif

#ifdef __NTMODE__

   #define outp outB
   #define inp inB
   #define inpw inport
   #define outpw outport

   int outB (unsigned short PortNumber, int DataValue );
   int inB (unsigned short PortNumber);

#endif

#endif /* _ALIAS_H_ */
