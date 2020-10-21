/*
  $Header: /Signalogic/DirectCore/include/filelib.h

  Description:  File management library for various types of audio and waveform files, including header handling and manipulation for .wav, Hypersignal .tim, and Matlab (includes gateway function for MATLAB calls)

  Copyright (c) 1993-2020 Signalogic, Inc.
  All Rights Reserved

  Revision History
  
   Created Nov 1993, Varsha Shamabhat
   Modified 1995-1998, Jeff Brower
   Modified 2002-2010
   Modified Jan 2017 JHB, added DS_CLOSE, DS_WRITE, DS_READ, made DS_CREATE non-zero
   Modified Jun 2017 JHB, DSWriteWvfrmHeader() returns number of bytes written (header length)
   Modified Apr 2018 JHB, added compression codes for AMRNB, AMRWB, MELPe, and EVS
   Modified Jan 2020 JHB, add MAXTHREADS definition and DSCreateFilelibThread()
*/

/* temporary "strict" literals for BOOL and UINT */

#ifndef _FILELIB_H_
#define _FILELIB_H_

#if !defined(__WIN32__) && !defined(__MSVC__) && !defined(_LINUX_) && !defined(_WIN32_WINNT)

  #define BOOL short int
  #define UINT unsigned short int
#endif


/* Win16 / Win32 / Linux import definition */

#if !defined(LIBAPI)
  #if !defined(__WIN32__) && !defined(__MSVC__)
    #define LIBAPI WINAPI
  #else
    #ifdef _MSC_VER
/*      #define LIBAPI __declspec(dllimport)  // this method fails with MSVC 4.2 IMPLIB */
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


/* general */

#define MAXFILES                 128   /* max number of files that can be open at one time */
#define MAXFILECHANNELS          64
#define MAXTHREADS               32    /* max application threads, see change in filemgr.cpp, JHB Jan2020 */

#define START_POS                22    /* seek constants */
#define END_POS                  23
#define CURRENT_POS              24

/*
#define MAG                      0
#define PHZ                      1
*/

#define CHUNK_LEN                60000L
#define CHUNK                    32768L

#define DS_CREATE                1      /* create new file */
#define DS_OPEN                  2      /* open existing file */
#define DS_EXISTS                4      /* see if file exists (does not leave file open) */
#define DS_CLOSE                 8      /* close the file */
#define DS_WRITE                 0x10   /* write to file */
#define DS_READ                  0x20   /* read from file */

/* header types (note:  constants used are minimum length of header in bytes, but actual header length could be longer) */

#define DS_RAWAUDIO              0
#define DS_TON                   2
#define DS_HYPSHORT              20
#define DS_HYPLONG               128
#define DS_WAV                   44
#define DS_MAT                   100  /* not determined yet */
#define DS_DPR                   101  /* not determined yet */


/* precision and data types */

/* example of usage:  ReadWvfrmData(hFile, ptr, num, FP | CPLX | SINGLE) which would read data into ptr as complex, 32-bit floating-point; any type conversions from the actual data format in hFile are done automatically; another example:  ReadWvfrmData(hFile, ptr, num, FIXED | SHORTINT) */

#define DS_DT_FIXED              256
#define DS_DT_FP                 512
#define DS_DT_CPLX               1024
#define DS_DT_UNSIGNED           2048

#define DS_DP_BYTE               8
#define DS_DP_SHORTINT           16
#define DS_DP_SHORT              DS_DP_SHORTINT
#define DS_DP_LONGINT            32
#define DS_DP_LONG               DS_DP_LONGINT
#define DS_DP_SINGLE             32
#define DS_DP_FLOAT              32
#define DS_DP_DOUBLE             64


/* header parameters and attributes */

#define DS_GWH_HEADERTYPE        1
#define DS_GWH_HEADERLEN         2   /* returns value in bytes */
#define DS_GWH_DATAPREC          3   /* a 16-bit value, as follows:  high byte has type (FP, FIXED, and/or CPLX attributes), low byte has N-bit precision (for either fp or fixed) */
#define DS_GWH_MAXAMP            4
#define DS_GWH_FRMLEN            5
#define DS_GWH_SAMPFREQ          6
#define DS_GWH_WVFRMLEN          7
#define DS_GWH_NUMCHAN           8
#define DS_GWH_FFTORD            9
#define DS_GWH_EXPONENT         10
#define DS_GWH_TYPEATTRIBUTE    11
#define DS_GWH_TIMESOURCE       12
#define DS_GWH_FFTSIZE          13
#define DS_GWH_FRMOVERLAP       14
#define DS_GWH_WINSCL           15
#define DS_GWH_WINTYPE          16
#define DS_GWH_NYQUISTPT        17
#define DS_GWH_CPLXFLG          18
#define DS_GWH_FILETYPE         19
#define DS_GWH_MANTISSA         21
#define DS_GWH_COMPRESSIONCODE  22

/* subattributes (related to an attribute or parameter above) */

#define DS_GWH_FT_TIME          0   /* FILETYPE values */
#define DS_GWH_FT_MAG           1
#define DS_GWH_FT_PHZ           2
#define DS_GWH_FT_CPLX          3

#define DS_GWH_TS_ARB           0   /* TIMESOURCE types (in TIMEATTRIBUTE value) */
#define DS_GWH_TS_LPC           1
#define DS_GWH_TS_IMPULSE       2
#define DS_GWH_TS_USERDEF       3

#define DS_GWH_WT_RECT          0   /* window types (in TIMEATTRIBUTE value) */
#define DS_GWH_WT_HAMM          1
#define DS_GWH_WT_HANN          2
#define DS_GWH_WT_BLACK         3
#define DS_GWH_WT_BART          4
#define DS_GWH_WT_GAUSS         5
#define DS_GWH_WT_USER          7

#define DS_GWH_CC_UNKNOWN       0
#define DS_GWH_CC_PCM           1
#define DS_GWH_CC_ALAW          6
#define DS_GWH_CC_ULAW          7
#define DS_GWH_CC_G723          20
#define DS_GWH_CC_MS_GSM610     32
#define DS_GWH_CC_GSM_AMR       49
#define DS_GWH_CC_MP3           80

#define DS_GWH_CC_G729          129  /* added JHB Apr 2018 */
#define DS_GWH_CC_GSM_AMRWB     130
#define DS_GWH_CC_MELPE         131
#define DS_GWH_CC_EVS           132

typedef short int HIO;           /* file or I/O stream handle */


#ifdef __cplusplus
extern "C" {
#endif

#if defined(__WIN32__) || defined(__MSVC__)

DECLSPEC HWND LIBAPI DSInitFileLib();  /* Entry point code for DLL */

#endif

#define long_t long   /* long_t will be 32 bits on 32-bit system, 64 bits on 64-bit system, JHB Jan2017 */

typedef short int HFILEW;

DECLSPEC short int LIBAPI DSOpenFile(LPCSTR, HFILEW); /* open / create File -> returns handle to file (returns 0 on error) */

DECLSPEC short int LIBAPI DSCloseFile(HFILEW);  /* close File -> returns 0 on errors */

DECLSPEC short int LIBAPI DSCopyFile(LPCSTR, LPCSTR, short int, short int, float);  /* copy a file; also can convert data precisions and perform scaling if needed (sf != 1) */

DECLSPEC short int LIBAPI DSDeleteFile(LPCSTR);  /* delete a file (should not already be open) */

DECLSPEC short int LIBAPI DSReadWvfrmHeader(HFILEW);  /* read waveform file header into current header image (in memory), leaving the file pointer at the end of the header (start of data).  Returns number of bytes read */

DECLSPEC short int LIBAPI DSWriteWvfrmHeader(HFILEW);  /* write the current header image to a waveform file header, leaving the file pointer at the end of the header (start of data).  Returns number of bytes written */

DECLSPEC long_t LIBAPI DSGetWvfrmHeader(HFILEW, short int);  /* get specified values from the current header image (in memory) */

DECLSPEC void LIBAPI DSSetWvfrmHeader(HFILEW, short int, long_t);  /* set specified values into the current header image (in memory) */

DECLSPEC void LIBAPI DSInheritHeader(HFILEW, HFILEW, short int, short int);  /* inherit header values form one file to another */

DECLSPEC long LIBAPI DSReadWvfrmData(HFILEW, void far*, long_t, short int);  /* read data from the waveform file into the specified buffer, using the specified precision */

DECLSPEC void LIBAPI DSWriteWvfrmData(HFILEW, void far*, long_t, short int); /* write data from the specified buffer to the waveform file, using the specified precision */

DECLSPEC void LIBAPI DSUpdateHeader(HFILEW, short int, long_t);  /* update specified values in header */

DECLSPEC short int LIBAPI DSInitWvfrmHeader(HFILEW, short int);  /* initialize waveform file headers in specified format, using default values.  Should be called if a new file has been created and before header values are set */

DECLSPEC long LIBAPI DSSeekPos(HFILEW, short int, long_t);  /* seek to a specified location in the waveform file */

DECLSPEC void LIBAPI DSCreateFilelibThread(void);  /* create a thread index for multithreaded / multiple application concurrent filelib usage */

#ifdef _MATLAB_INTERFACE_
DECLSPEC void LIBAPI DSCloseFigureFiles(HFILEW);
#endif

DECLSPEC short int LIBAPI DSDeleteFile(LPCSTR);  /* delete a file (should not already be open) */

#ifdef __cplusplus
}
#endif


#if !defined(__WIN32__) && !defined(__MSVC__) && !defined(_LINUX_) && !defined(_WIN32_WINNT)
  #undef UINT
  #undef BOOL
#endif

#endif /* _FILELIB_H_ */
