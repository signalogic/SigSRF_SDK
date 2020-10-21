/*
   $Header: /Signalogic/DirectCore/include/tdmlib.h

   Description:

   TDM / CTBus API header file

     -API access to TDM stream control and mapping registers in SigC641x module TDM/host FPGA
     -API access to OSS DS3 module TEMUX and T8110
     -depends on presence of DirectCore software (hwlib library required)

 Copyright (C) Signalogic Inc. 2008-2013

 Revision History:

   Created:   Oct 2008, JHB 
   Modified:  Dec 2008, YT, JHB.  Added support for TEMUX and T8110 access on OSS DS3 module.
   Modified:  Mar 2009, JHB.      Added support for T8110 chip on HW400c/2 carrier board.
   Modified:  Jun-Jul 2009, JHB.  Added additional register constants and bit fields for DS3 module devices (TECT3, T8110)
 
*/


/* include driver header file in order to define TDM / CTBus interface registers (these registers located in FPGA logic on the PTMC module) */

#ifndef _TDMLIB_H_
#define _TDMLIB_H_

#if defined (_LINUX_)  || defined (_WIN32_WINNT)
   #include "alias.h"
#endif

// temporary "strict" literals for BOOL and UINT

#if !defined(__WIN32__) && !defined(__MSVC__) && !defined(_LINUX_) && !defined(_WIN32_WINNT)

  #define UINT unsigned short int
  #define BOOL short int

#endif


// Win16 / Win32 / Linux import definition

#if !defined(LIBAPI)
  #if !defined(__WIN32__) && !defined(__MSVC__)
    #define LIBAPI WINAPI
  #else
    #ifdef _MSC_VER
      //#define LIBAPI __declspec(dllimport)  // this method fails with MSVC 4.2 IMPLIB
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
            #define DECLSPEC __declspec(dllexport)  // this method fails with MSVC 4.2 IMPLIB

         #else
            #define DECLSPEC __declspec(dllimport)  // this method fails with MSVC 4.2 IMPLIB
         #endif
      #endif
   #endif
#endif

#if defined (__MSVC__)
   #include "hwdriver32.h"
#endif


/* TDM / CTBus stream registers */

#define DS_NUM_STREAM_REGS  16

#define DS_SCS_STREAM_A     0   /* stream registers in SigC641x FPGA logic */
#define DS_SCS_STREAM_B     1
#define DS_SCS_STREAM_C     2
#define DS_SCS_STREAM_D     3
#define DS_SCS_STREAM_E     4
#define DS_SCS_STREAM_F     5
#define DS_SCS_STREAM_G     6
#define DS_SCS_STREAM_H     7
#define DS_SCS_STREAM_I     8
#define DS_SCS_STREAM_J     9
#define DS_SCS_STREAM_K     11
#define DS_SCS_STREAM_L     12

#define DS_SCS_TX           0
#define DS_SCS_RX           1

#define DS_SCS_OEN          0x10000000
#define DS_SCS_CTIEN        0x40000000

#define DS_SCS_CTD_IN       0   /* bits 6-0 defining CTbus input stream (input from CTbus to FPGA logic) */
#define DS_SCS_CTD_OUT      0   /* bits 6-0 defining CTbus output stream (output from FPGA logic to CTbus) */
#define DS_SCS_CTD_INOUT    7   /* bits 11-7 defining additional CTbus I/O stream when forking or merging with CTD_IN and CTD_OUT */

#define DS_SCS_DSP_IN       22  /* bits 26-22 defining DSP input stream (output from FPGA logic to DSP) */
#define DS_SCS_DSP_OUT      22  /* bits 26-22 defining DSP output stream (output from DSP to FPGA logic) */


/*

TDM Control/Status Register
---------------------------

  TDM_CTRL_STAT     31-28

                  RESERVED


                    27                26                25             24

              CT_NETREF2_FAIL    CT_NETREF1_FAIL    CT_F_B_FAIL    CT_F_A_FAIL


                  23-20     19-12      11-8       7-4

                RESERVED   TEST_PAT   DSP_NUM   RESERVED


                     3                2              1            0

              TEST_DATA_AUTO_EN   INTCLK_AUTO_EN  TEST_DATA_EN  INTCLK_EN

        
  INTCLK_EN          1 = Enable internal clock
  TEST_DATA_EN       1 = Enable test pattern data
  INTCLK_AUTO_EN     1 = Internal clock enabled if CTbus clock not detected
  TEST_DATA_AUTO_EN  1 = Test pattern data enabled if CTbus clock not detected

  DSP_NUM            n = active DSP number
  TEST_PAT        0xdd = 8-bit data to use for DSP test pattern (DSP specified by DSP_NUM)

  CT_F_A_FAIL        1 = Period measurement failure on CT_F_A signal
  CT_F_B_FAIL        1 =   "        "          "       CT_F_B signal
  CT_NETREF1_FAIL    1 =   "        "          "       CT_NETREF1 signal
  CT_NETREF2_FAIL    1 =   "        "          "       CT_NETREF2 signal



CTbus Clock Source Register
---------------------------

  TDM_CT_CLKSRC     31-10                                                9-5             4-0
  
                  RESERVED                                           CT_B_CLKSRC     CT_A_CLKSRC

  CT_n_CLKSRC       0 = CT bus clock driven by offboard source (typically CTbus in
                        PTMC 2, 3, or 5 configurations)
                    1 = not used
                    2 = CT bus clock driven by onboard clock + logic
                    3 = CT bus clock driven WinPath TDM
                    4 = CT bus clock driven by DSP 0
                    :
                   15 = CT bus clock driven by DSP 11
                16-31 = reserved


DSP Clock Source Registers
--------------------------

  TDM_DSP_CLKSRC0   31-16        17-15          14-12          11-9            8-6            5-3            2-0

                  RESERVED  DSP_5_CLK_SRC  DSP_4_CLK_SRC  DSP_3_CLK_SRC  DSP_2_CLK_SRC  DSP_1_CLK_SRC  DSP_0_CLK_SRC

   
  TDM_DSP_CLKSRC1   31-16         17-15          14-12          11-9            8-6            5-3            2-0

                  RESERVED  DSP_11_CLK_SRC  DSP_10_CLK_SRC  DSP_9_CLK_SRC  DSP_8_CLK_SRC  DSP_7_CLK_SRC  DSP_6_CLK_SRC


  DSP_n_CLK_SRCm    0 = DSP McBSP clock driven by CT_C8_A
                    1 = DSP McBSP clock driven by CT_C8_B
                    2 = DSP McBSP clock driven by onboard clock + logic
                    3 = DSP McBSP clock driven WinPath TDM
                    4 = DSP McBSP clock driven by internal DSP serial port timer
                  5-7 = reserved


Internal Clock and Framesync Control Register
---------------------------------------------

  TDM_CLK_FS_CTRL    31-30      29-15    14-0

                    RESERVED   FS_DIV   CLK_DIV

  CLK_DIV    0-32767 = if internal clock is enabled, clock rate = 8.192 MHz / (CLK_DIV + 1)
  FS_DIV     0-32767 = if internal clock is enabled, framesync rate = (clock rate)/(FS_DIV + 1)


Stream Control Registers
------------------------

Notes:

1) Each stream has Rx and Tx components, which map to Tx and Rx lines for each DSP, respectively.

2) Multiple sources routed to the same destination are AND'ed together.

3) For more information about 32 time-slot groups, see CTbus-to-DSP mapping example below.


  TDM_STREAM_RX[n]  31        30-29     28     27      26-22       21-18       17-14        13-7        6-0

                  RESERVED   TS_BITW   OEN   SPL_EN   DSP_OUT   WP_TDM_OUT   WP_TDM_IN   CT_D_INOUT   CT_D_OUT

  CT_D_OUT      0-127 = lower 5 bits specifies CTbus output line, upper 2 bits group of 32 time-slots -- this is output from FPGA to CTbus
  CT_D_INOUT    0-127 = lower 5 bits specifies CTbus input/output line, upper 2 bits group of 32 time-slots
  WP_TDM_IN      0-15 = specifies WinPath TDM input line
  WP_TDM_OUT     0-15 = specifies WinPath TDM output line
  DSP_OUT        0-31 = specifies DSP output line (DSP Tx) -- this is output from DSP to FPGA logic
  SPL_EN            1 = split enable.  If set, then DSP_OUT field specifies DSP that transmits a
                        stream comprised of CTbus streams specified by CT_D_OUT and CT_D_INOUT
                        split apart.  In this case, CT_D_INOUT is treated as an output, and the
                        CTbus stream clockrates are 1/2 that of the DSP stream
  OEN               1 = output enable.  If set then output is enabled on CT stream specified by CT_D_OUT
  TS_BITW         0-3 = specifies time-slot bit-width, 0 = 8, 1 = 12, 2 = 16, 3 = reserved
  
   
  TDM_STREAM_TX[n]  31        30-29     28     27     26-22      21-18       17-14       13-7          6-0

                  RESERVED   TS_BITW   OEN   MRG_EN   DSP_IN   WP_TDM_OUT   WP_TDM_IN   CT_D_INOUT   CT_D_IN

  CT_D_IN       0-127 = lower 5 bits specifies CT bus input line, upper 2 bits group of 32 time-slots -- this is input from CTbus to FPGA logic
  CT_D_INOUT    0-127 = lower 5 bits specifies CT bus input/output line, upper 2 bits group of 32 time-slots
  WP_TDM_IN      0-15 = specifies WinPath TDM input line
  WP_TDM_OUT     0-15 = specifies WinPath TDM output line
  DSP_IN         0-31 = specifies DSP input line (DSP Rx) -- this is output from FPGA logic to DSP
  MRG_EN            1 = merge enabled.  If set, then DSP_IN field specifies DSP that receives a
                        stream comprised of CTbus streams specified by CT_D_IN and CT_D_INOUT
                        merged together.  In this case, CT_D_INOUT is treated as an input, and
                        the DSP stream clockrate is 2x that of the CTbus streams.
  OEN               1 = output enable.  If set then output is enabled on DSP stream specified by n
  TS_BITW         0-3 = specifies time-slot bit-width, 0 = 8, 1 = 12, 2 = 16, 3 = reserved

*/


#ifdef _DS3_SUPPORT_

/* DS3 module support */

#define MAXCONNECTIONS 4096  /* max connections allowed by T8110 device on DS3 module */

/* The following constants are used in CONNECTION structure in DSMakeDs3Connection(),
   DSMakeH110Connection(), DSGetDs3Connection(), DSDeleteH110Connection(), etc. APIs */
   
#define DS_DEVICE_MUSYCC    1
#define DS_DEVICE_TEMUX     2
#define DS_DEVICE_TECT3     2
#define DS_DEVICE_CTBUS     3
#define DS_DEVICE_DSP       4
#define DS_DEVICE_H110      5

#define DS_BUS_T8110_LOCAL  0
#define DS_BUS_T8110_H110   1


/* constants that can be used with uMode param in DSInitDs3Module() API */

#define DS3_CONFIG_LIU                   1
#define DS3_CONFIG_T8110                 2
#define DS3_CONFIG_TECT3                 4

#define DS3_T8110_CLKOUT_CT8             0x10
#define DS3_T8110_CLKOUT_NETREF          0x20

#define DS3_LIU_NO_LOOPBACK              0
#define DS3_LIU_ANALOG_LOCAL_LOOPBACK    0x100
#define DS3_LIU_DIGITAL_LOCAL_LOOPBACK   0x200
#define DS3_LIU_DIGITAL_REMOTE_LOOPBACK  0x300

#define DS3_LIU_TRANSMIT_ALL_1S          0x400
#define DS3_LIU_MUTE_ON_LOS              0x800

#define DS3_TECT3_FRAMING_NONE           0
#define DS3_TECT3_FRAMING_M13            0x10000
#define DS3_TECT3_FRAMING_CBITPARITY     0x20000


/* The following constants specify which device to access in DSReadDs3Device() and 
   DSWriteDs3Device() direct-access functions */
   
#define DS3_MUSYCC                     1       /* devices on the DS3 module */
#define DS3_TEMUX                      2
#define DS3_TECT3                      2
#define DS3_T8110                      3
#define DS3_CPLD                       5

#define DS3_REG8                       0       /* attributes:  read/write one byte, read/write 4 bytes (or with device type) */
#define DS3_REG32                      32

/* LIU register constants */

#define LIU_TX_CTRL                    0x000c
#define LIU_RX_CTRL                    0x0010

/* For TECT3 framer register constants, see OSS file temux.h.  Additional constants defined below */

#define TECT3_TPSC1_PCMDC_SIGC         0x1000
#define TECT3_TPSC2_PCMDC_SIGC         0x1080
#define TECT3_TPSC3_PCMDC_SIGC         0x1100
#define TECT3_TPSC4_PCMDC_SIGC         0x1180
#define TECT3_TPSC5_PCMDC_SIGC         0x1200
#define TECT3_TPSC6_PCMDC_SIGC         0x1280
#define TECT3_TPSC7_PCMDC_SIGC         0x1300
#define TECT3_TPSC8_PCMDC_SIGC         0x1380
#define TECT3_TPSC9_PCMDC_SIGC         0x1400
#define TECT3_TPSC10_PCMDC_SIGC        0x1480
#define TECT3_TPSC11_PCMDC_SIGC        0x1500
#define TECT3_TPSC12_PCMDC_SIGC        0x1580
#define TECT3_TPSC13_PCMDC_SIGC        0x1600
#define TECT3_TPSC14_PCMDC_SIGC        0x1680
#define TECT3_TPSC15_PCMDC_SIGC        0x1700
#define TECT3_TPSC16_PCMDC_SIGC        0x1780
#define TECT3_TPSC17_PCMDC_SIGC        0x1800
#define TECT3_TPSC18_PCMDC_SIGC        0x1880
#define TECT3_TPSC19_PCMDC_SIGC        0x1900
#define TECT3_TPSC20_PCMDC_SIGC        0x1980
#define TECT3_TPSC21_PCMDC_SIGC        0x1a00
#define TECT3_TPSC22_PCMDC_SIGC        0x1a80
#define TECT3_TPSC23_PCMDC_SIGC        0x1b00
#define TECT3_TPSC24_PCMDC_SIGC        0x1b80
#define TECT3_TPSC25_PCMDC_SIGC        0x1c00
#define TECT3_TPSC26_PCMDC_SIGC        0x1c80
#define TECT3_TPSC27_PCMDC_SIGC        0x1d00
#define TECT3_TPSC28_PCMDC_SIGC        0x1d80

/* TECT3 framer register bit field control and status.  No bits are defined in temux.h */

#define TECT3_TPSC_UAS_BUSY            0x80         /* polling bit in TPSC UAS */
#define TECT3_TPSC_UAS_RWB             0x80         /* read/write bit -- set to 1 for read, 0 for write */

#define TECT3_DS3_TRAN_CFG_CBE         0x01
#define TECT3_DS3_TRAN_CFG_TSIG        0x02
#define TECT3_DS3_FRMR_CFG_CBE         0x01
#define TECT3_DS3_FRMR_CFG_AISC        0x02
#define TECT3_DS3_MX23_CFG_CBE         0x02


/* T8110 register constants */
   
#define T8110_MASTER_ENABLE            0x102
#define T8110_DATAMEMORY_MODE          0x104
#define T8110_CLOCKREG_ACCESS_SELECT   0x106
#define T8110_DEVICE_ID_REGISTER       0x12a
#define T8110_MAIN_INPUT_SELECTOR      0x200
#define T8110_APLL1_INPUT_SELECTOR     0x202
#define T8110_RESOURCE_DIVIDER         0x204
#define T8110_LREF_INPUT_SELECT        0x208
#define T8110_MASTER_OUTPUT_ENABLE     0x220
#define T8110_LSC01_SELECT             0x228
#define T8110_LSC23_SELECT             0x22a
#define T8110_HBUS_RATE_GROUP_BA       0x300
#define T8110_HBUS_RATE_GROUP_DC       0x301
#define T8110_HBUS_RATE_GROUP_FE       0x302
#define T8110_HBUS_RATE_GROUP_HG       0x303
#define T8110_LBUS_RATE_GROUP_BA       0x320
#define T8110_LBUS_RATE_GROUP_DC       0x321
#define T8110_LBUS_RATE_GROUP_FE       0x322
#define T8110_LBUS_RATE_GROUP_HG       0x323

#define T8110_CONNECTION_MEMORY_BASE   0x40000  /* T8110 connection memory base address */

/* InitCtBus() constants */

#define CTBUS_RESET           1
#define CTBUS_LOGIC_LOOPBACK  2
#define CTBUS_CONNECT_DSP     4

/* constants that can be used with uMode param in DSInitH110() API */

/* H.110 clock modes */

#define H110_CLOCK_SLAVE                 0
#define H110_CLOCK_MASTER_A              1
#define H110_CLOCK_MASTER_B              2  /* H110 CT_8_B line */
#define H110_CLOCK_LOCALOSC              3  /* onboard 8 kHz osc */
#define H110_CLOCK_STANDALONE            H110_CLOCK_LOCALOSC

/* H.110 clock attributes:  sources, fallbacks */

#define H110_CLOCK_SOURCE_CTC8A          0x8   /* H110 CTC8A line (clock A 8.192 MHz) */
#define H110_CLOCK_SOURCE_CTC8B          0x10  /* H110 CTC8B line (clock B 8.192 MHz) */
#define H110_CLOCK_SOURCE_NETWORK        0x20  /* WAN, i.e. clock recovered from T3 line, passed through from local side */
#define H110_CLOCK_SOURCE_NETREF1        0x40
#define H110_CLOCK_SOURCE_NETREF2        0x80

#define H110_CLOCK_FALLBACK1_NONE        0
#define H110_CLOCK_FALLBACK2_NONE        0
#define H110_CLOCK_FALLBACK1_NETWORK     0x100
#define H110_CLOCK_FALLBACK1_NETREF1     0x200
#define H110_CLOCK_FALLBACK1_NETREF2     0x400
#define H110_CLOCK_FALLBACK1_CTC8A       0x800
#define H110_CLOCK_FALLBACK1_CTC8B       0x1000
#define H110_CLOCK_FALLBACK1_LOCALOSC    0x2000
#define H110_CLOCK_FALLBACK2_NETWORK     0x4000
#define H110_CLOCK_FALLBACK2_NETREF1     0x8000
#define H110_CLOCK_FALLBACK2_NETREF2     0x10000
#define H110_CLOCK_FALLBACK2_CTC8A       0x20000
#define H110_CLOCK_FALLBACK2_CTC8B       0x40000
#define H110_CLOCK_FALLBACK2_LOCALOSC    0x80000

#define H110_CLOCK_RATE_8192             0
#define H110_CLOCK_RATE_2048             0x100000
#define H110_CLOCK_RATE_1544             0x200000


/* typedefs used in following APIs:

  DSMakeDs3Connection()
  DSGetDs3Connection()
  DSDeleteDs3Connection()

  DSMakeH110Connection()
  DSGetH110Connection()
  DSDeleteH110Connection()
  
*/

typedef UINT DS3DEVICE;
typedef UINT HCONNECT; /* connection handle */

typedef struct {

  UINT uDevice;
  UINT uStream;
  UINT uChannel;

} ENDPOINT;

typedef struct {

  ENDPOINT In;
  ENDPOINT Out;
  UINT Id;
  
} CONNECTION;


/* for H.110 usage, some aliases that can be used in ENDPOINT typedef above */

#define uBus uDevice
#define uTimeSlot uChannel


/* DLL / .so API prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize DS3 module.  Return value is 1 if success, 0 if failure.  Note that DSAssignBoard() must have been called previously (see hwlib.h) */

DECLSPEC UINT LIBAPI DSInitDs3Module(HBOARD hBoard);

/* Delete all connections */

void DSResetDs3Connections(HBOARD hBoard);

/* Make a connection, returns handle to the connection */

DECLSPEC HCONNECT LIBAPI DSMakeDs3Connection(HBOARD hBoard, CONNECTION* pConnection);

/* Delete a connection.  Return value is 1 if success, 0 if failure. */

DECLSPEC UINT LIBAPI DSDeleteDs3Connection(HBOARD hBoard, HCONNECT);

/* Get connection information (structure pointed to by pConnection will be filled in upon return.  Return value is 1 if success, 0 if failure */

DECLSPEC UINT LIBAPI DSGetDs3Connection(HBOARD hBoard, HCONNECT hConnect, CONNECTION* pConnection);

/* Get number of current connections */

DECLSPEC UINT LIBAPI DSGetNumDs3Connections(HBOARD hBoard);

/* Read DS3 module register.  Note that module has several devices (see DS_DDR_xxx constants above). */

DECLSPEC UINT LIBAPI DSReadDs3Device(HBOARD hBoard, DS3DEVICE uDevice, UINT uReg);

/* Write DS3 module register.  Note that module has several devices (see DS_DDR_xxx constants above). */

DECLSPEC UINT LIBAPI DSWriteDs3Device(HBOARD hBoard, DS3DEVICE uDevice, UINT uReg, UINT uData);

#ifdef __cplusplus
}
#endif

#endif  /* _DS3_SUPPORT_ */


#ifdef _H110_SUPPORT_  /* HW400c/2 carrier board T8110 chip support */

/* H.110 related devices on HW400c/2 board */

#define H110_T8110   1
#define H110_CPLD    2

#define CPLD_BASE    0xe1000000  /* CPLD base address on HW400c/2 carrier board */

#define CPLD_CSR     4   /* clock select register */
#define CPLD_HRR     17  /* hardware revision register inside CPLD */

#define T8110_BASE   0xe2000000  /* T8110 device base address on HW400c/2 carrier board */


typedef UINT H110DEVICE;

#ifndef _DS3_SUPPORT_

  typedef UINT HCONNECT; /* connection handle */

  typedef struct {

    UINT uDevice;
    UINT uStream;
    UINT uChannel;

  } ENDPOINT;

  typedef struct {

    ENDPOINT In;
    ENDPOINT Out;
    UINT Id;
  
  } CONNECTION;

#endif



/* DLL / .so API prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize H.110 circuitry.  Return value is 1 if success, 0 if failure.  Note that DSAssignBoard() must have been called previously (see hwlib.h) */

DECLSPEC UINT LIBAPI DSInitH110(HBOARD hBoard);

/* Delete all H.110 connections */

void DSResetH110Connections(HBOARD hBoard);

/* Make a connection, returns handle to the connection */

DECLSPEC HCONNECT LIBAPI DSMakeH110Connection(HBOARD hBoard, CONNECTION* pConnection);

/* Delete a connection.  Return value is 1 if success, 0 if failure. */

DECLSPEC UINT LIBAPI DSDeleteH110Connection(HBOARD hBoard, HCONNECT);

/* Get connection information (structure pointed to by pConnection will be filled in upon return.  Return value is 1 if success, 0 if failure */

DECLSPEC UINT LIBAPI DSGetH110Connection(HBOARD hBoard, HCONNECT hConnect, CONNECTION* pConnection);

/* Get number of current connections */

DECLSPEC UINT LIBAPI DSGetH110NumConnections(HBOARD hBoard);


/* Read H.110 related register */

DECLSPEC UINT LIBAPI DSReadH110Device(HBOARD hBoard, H110DEVICE uDevice, UINT uReg);

/* Write H.110 related register */

DECLSPEC UINT LIBAPI DSWriteH110Device(HBOARD hBoard, H110DEVICE uDevice, UINT uReg, UINT uData);

#ifdef __cplusplus
}
#endif

#endif  /* _H110_SUPPORT_ */


/* DLL / .so API prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DECLSPEC UINT LIBAPI DSSetCtBusCSR(HBOARD hBoard, UINT uVal);
DECLSPEC UINT LIBAPI DSGetCtBusCSR(HBOARD hBoard);

DECLSPEC UINT LIBAPI DSSetCtbusClkSrc(HBOARD hBoard, UINT uVal);
DECLSPEC UINT LIBAPI DSSetDspClkSrc(HBOARD hBoard, UINT uVal);

DECLSPEC UINT LIBAPI DSSetCtBusClkFsCtrl(HBOARD hBoard, UINT uVal);

DECLSPEC UINT LIBAPI DSSetCtBusStream(HBOARD hBoard, UINT uStream, UINT uRxTx, UINT uVal);
DECLSPEC UINT LIBAPI DSGetCtBusStream(HBOARD hBoard, UINT uStream, UINT uRxTx);

DECLSPEC UINT LIBAPI DSSetTdmlibDebugMode(UINT uDebugMode);

#ifdef __cplusplus
}
#endif


#if !defined(__WIN32__) && !defined(__MSVC__) && !defined(_LINUX_) && !defined(_WIN32_WINNT)
  #undef UINT
  #undef BOOL
#endif

#endif  /* _TDMLIB_H_ */
