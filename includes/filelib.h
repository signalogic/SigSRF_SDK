/*
  $Header: /Signalogic/DirectCore/include/filelib.h

  Description:  File management library for various types of audio and waveform files, including header handling and manipulation for .wav, Hypersignal .tim, and Matlab (includes gateway function for MATLAB calls)

  Copyright (c) 1993-2023 Signalogic, Inc.  All Rights Reserved

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

  Revision History
   Created Nov 1993, Varsha Shamabhat
   Modified 1995-1998, Jeff Brower
   Modified 2002-2010
   Modified Jan 2017 JHB, added DS_CLOSE, DS_WRITE, DS_READ, made DS_CREATE non-zero
   Modified Jun 2017 JHB, DSWriteWvfrmHeader() returns number of bytes written (header length)
   Modified Apr 2018 JHB, added compression codes for AMRNB, AMRWB, MELPe, and EVS
   Modified Jan 2020 JHB, add MAXTHREADS definition and DSCreateFilelibThread()
   Modified Feb 2022 JHB, add DS_ prefix to DSSeekPos() flags, add HFILE alias to HFILEW for app use, add DS_SEEKPOS_RETURN_BYTES flag for DSSeekPos()
   Modified Dec 2022 JHB, add DSGetOSFileHandle() return FILE* handle maintained internally by filelib, bump MAXTHREADS to 64
   Modified Dec 2022 JHB, add DSDeleteFilelibThread()
   Modified Jan 2023 JHB, add DSGetFileHandle()
   Modified Jan 2023 JHB, add DS_CREATE_TMP and DS_CREATE_BAK options for use with DS_CREATE in DSOpenFile(). See comments
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
#define MAXTHREADS               64    /* max application threads, see change in filemgr.cpp, JHB Jan2020 */

#define DS_START_POS             22    /* seek constants, add DS_ prefix and use with DSSeekPos(), JHB Feb2022 */
#define DS_END_POS               23
#define DS_CURRENT_POS           24
#define DS_SEEKPOS_RETURN_BYTES  0x100  /* returns byte position instead of samples, for example header length instead of zero at start of a .wav file, JHB Feb 2022 */
#define DS_SEEKPOS_ITEM_MASK     0xff

/*
#define MAG                      0
#define PHZ                      1
*/

#define CHUNK_LEN                60000L
#define CHUNK                    32768L

/* flags used by mode param DSOpenFile(), and uFlags param in DSSaveDataFile() and DSLoadDataFile() in directcore.h */

#define DS_CREATE                1      /* create new file for read-write, if file already exists it's overwritten */
#define DS_OPEN                  2      /* open existing file for read-write */
#define DS_EXISTS                4      /* check if file exists without opening the file */
#define DS_WRITE                 0x10   /* write to file */
#define DS_READ                  0x20   /* read from file */

#define DS_CREATE_TMP            0x40   /* can be combined with DS_CREATE - if the specified file already exists, a tmp file is created and used for subsequent read/write operations. On DSCloseFile(), the existing file is deleted and the tmp file renamed to the specified filename. This option is intended for real-time output media streams (e.g. wav files), minimizing time lost to file open and initialization. The tradeoff is that twice the file size of disk space is used until the media stream is finished, at which time the tmp file is renamed and the existing file deleted */ 

#define DS_CREATE_BAK            0x80   /* same as DS_CREATE_TMP option, but on DSCloseFile() the existing file is renamed to .bak instead of being deleted. Twice the file size of disk space is always consumed */

/* flag used by uFlags param in DSSaveDataFile() and DSLoadDataFile() in directcore.h */

#define DS_CLOSE                 8      /* close the file */

/* waveform header types (note:  constants used are minimum length of header in bytes, but actual header length could be longer) */

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
#define HFILE HFILEW  /* removed from alias.h so it's Ok to use for Linux, but WinXX would be a conflict, JHB Feb2022 */

DECLSPEC HFILE LIBAPI DSOpenFile(LPCSTR, short int mode); /* open / create File -> returns handle to file (returns 0 on error) */

DECLSPEC short int LIBAPI DSCloseFile(HFILE);  /* close File -> returns 0 on errors */

DECLSPEC short int LIBAPI DSGetFileHandle(FILE*);  /* get filelib handle from OS file pointer, JHB Jan 2023 */

DECLSPEC short int LIBAPI DSCopyFile(LPCSTR, LPCSTR, short int, short int, float);  /* copy a file; also can convert data precisions and perform scaling if needed (sf != 1) */

DECLSPEC short int LIBAPI DSDeleteFile(LPCSTR);  /* delete a file (should not already be open) */

DECLSPEC short int LIBAPI DSReadWvfrmHeader(HFILE);  /* read waveform file header into current header image (in memory), leaving the file pointer at the end of the header (start of data).  Returns number of bytes read */

DECLSPEC short int LIBAPI DSWriteWvfrmHeader(HFILE);  /* write the current header image to a waveform file header, leaving the file pointer at the end of the header (start of data).  Returns number of bytes written */

DECLSPEC long_t LIBAPI DSGetWvfrmHeader(HFILE, short int);  /* get specified values from the current header image (in memory) */

DECLSPEC void LIBAPI DSSetWvfrmHeader(HFILE, short int, long_t);  /* set specified values into the current header image (in memory) */

DECLSPEC void LIBAPI DSInheritHeader(HFILE, HFILE, short int, short int);  /* inherit header values form one file to another */

DECLSPEC long LIBAPI DSReadWvfrmData(HFILE, void far*, long_t, short int);  /* read data from the waveform file into the specified buffer, using the specified precision */

DECLSPEC void LIBAPI DSWriteWvfrmData(HFILE, void far*, long_t, short int); /* write data from the specified buffer to the waveform file, using the specified precision */

DECLSPEC void LIBAPI DSUpdateHeader(HFILE, short int, long_t);  /* update specified values in header */

DECLSPEC short int LIBAPI DSInitWvfrmHeader(HFILE, short int);  /* initialize waveform file headers in specified format, using default values.  Should be called if a new file has been created and before header values are set */

DECLSPEC long LIBAPI DSSeekPos(HFILE, short int, long_t);  /* seek to a specified location in the waveform file */

DECLSPEC int LIBAPI DSCreateFilelibThread(void);  /* create a thread index for multithreaded / multiple app concurrent filelib usage */

DECLSPEC int LIBAPI DSDeleteFilelibThread(void);  /* delete a thread index for multithreaded / multiple app concurrent filelib usage */

DECLSPEC FILE* LIBAPI DSGetOSFileHandle(HFILE);  /* return FILE* handle maintained internally by filelib, JHB Dec 2022 */

#ifdef _MATLAB_INTERFACE_
DECLSPEC void LIBAPI DSCloseFigureFiles(HFILE);
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
