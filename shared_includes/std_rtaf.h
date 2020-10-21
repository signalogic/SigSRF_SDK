/*
  std_rtaf.h

  standard definitions for RTAF-based applications, supports C5xxx/C6xxx and x86.  Supports voice, video, CIM
      
  Copyright (C) Signalogic, 2002-2018

  Revision History

    Created 2002 JHB, provide definitions for TI devices compatible with standard C and Linux
    Modified Jan 2011 VR, added _FAR prefixes to several variable declarations, was required for CIM build with CGTv7.0.3
    Modified Sep 2012 JHB, added C66x support
    Modified Oct 2012 CJ, more C66x support
    Modified Jun 2013 JHB, added comments about chip families
    Modified Mar 2017 CJ, added support for linux on x86
    Modified Mar 2018 JHB, replaced _X86_ with _X86.  Change was made to maintain consistency with shared_include / coCPU usage.  _X86, _ARM, _TI66X, etc are "chip family" defines, specific chips are _X5_E930_, _C66XX_, etc)
    Modified Oct 2018 JHB, moved DNUM definition here from diaglib_priv.h
*/

#ifndef RTC_DSP_INC
  #define RTC_DSP_INC

#ifdef _X86

/* add x86 chip type definitions here, as needed */

  #define DNUM 0  /* moved here from diaglib_priv.h.  DNUM is a TI compiler recognized intrinsic specifying "which core am I" */

#else

/* Notes about chip type definitions below:

  1) There are three (3) main chip families defined:
  
     -C5000 (includes C54xx and C55xx, with slight differences)
     -C6000 (includes C62x, C64x, C64x+, C67x)
     -C6600
  
  2) C66x is substantially different from C64x and C64x+.  Floating-point, memory-mapped registers completely redefined, etc.
  
*/

#ifdef _TMS320C6600  /* added CJ, Oct 12*/
   #ifndef _C66XX_
      #define _C66XX_
      #define _C66xx
   #endif
#endif

#ifdef _TMS320C6X
   #ifndef _C6XXX_
     #define _C6XXX_
     #define _C6xxx
   #endif
#endif

#ifdef _TMS320C5XX
   #ifndef _C54XX_
      #define _C54XX_
      #define _C54xx
   #endif
   #ifndef _C5XXX_
      #define _C5XXX_
      #define _C5xxx
   #endif
#endif


#if defined(CHIP_C6414) || defined(CHIP_C6415) || defined(CHIP_C6416)

  #define CHIP_C64XX
  #define CHIP_C64xx
  #define CHIP_C641X    /* used in new main.c to distinguish between C6472 and C641X for CIM builds.  VR, Jan2011 */
  #define _CHIP_C64xx
  #define _TMS320C6400  /* enable correct TI intrinsics (JHB, Apr09) */

#endif

#if defined(CHIP_C6701) || defined(CHIP_C6712) || defined(CHIP_C6711)

  #define CHIP_C67XX
  #define CHIP_C67xx
  #define _CHIP_C67xx
  #define _TMS320C6700  /* enable correct TI intrinsics (JHB, Apr09) */

#endif

#if defined(CHIP_C6455) || defined(CHIP_C6472)  /* added JHB, Sep 12 */

  #define CHIP_C64XX
  #define CHIP_C64xx
  #define _CHIP_C64xx
  #define _TMS320C6400  /* enable correct TI intrinsics (JHB, Sep12) */

#endif

#if defined(CHIP_C6678)  /* added JHB, Sep 12 */

  #define CHIP_C66XX
  #define CHIP_C66xx
  #define _CHIP_C66xx
  #define _TMS320C6600  /* enable correct TI intrinsics (JHB, Sep12) */

#endif

#endif /* ifndef _X86 */

/************************************************************************/
/*                   Constant and macro definition                      */
/************************************************************************/

#ifndef FALSE
   #define FALSE                   0
#endif

#ifndef TRUE
  #define TRUE                  !FALSE
#endif

#ifndef BOOL
  #define BOOL                  unsigned int
#endif

#ifndef WORD
  #define WORD                  unsigned int
#endif

#ifdef _C5XXX_  /* C54xx and C55xx */

  #ifndef DWORD
    #define DWORD               unsigned long int
  #endif

  #ifndef UINT16
    #define UINT16              unsigned int
  #endif

  typedef short int           Bool;    //16b
  typedef char                Int8;    //16b
  typedef short int           Int16;   //16b
  typedef unsigned short int  UInt16;  //16b
  typedef long int            Int32;   //32b
  typedef unsigned long int   UInt32;  //32b
  typedef long long int       Int64;   //64b

  #ifdef _C55XX_
     typedef unsigned int        UInt8;   //16b
     typedef long int            Int40;   //40b
     typedef unsigned long int   UInt40;  //40b
  #else  /* C54xx */
     typedef unsigned char       UInt8;   //16b
     typedef long long           Int40;   //40b
     typedef unsigned long long  UInt40;  //40b
  #endif

#elif defined(_C6XXX_)

  #ifndef DWORD
    #define DWORD               unsigned int
  #endif
  
  #ifndef UINT16
    #define UINT16              unsigned short int
  #endif

  #if !defined(xdc_std__include) && !defined(_TI_STD_TYPES)
  
    typedef int                 Bool;    //32b
    typedef char                Int8;    // 8b
    typedef unsigned char       UInt8;   // 8b
    typedef short int           Int16;   //16b
    typedef unsigned short int  UInt16;  //16b
    typedef int                 Int32;   //32b
    typedef unsigned int        UInt32;  //32b
  #endif

  #ifndef ti_targets_STD_
    typedef long int            Int40;   //40b
    typedef long long int       Int64;   //64b
  #endif

  typedef short               Shortword;
  
#elif defined (_X86)

  #ifndef DWORD
    #define DWORD               unsigned int
  #endif
  
  #ifndef UINT16
    #define UINT16              unsigned short int
  #endif

  #if !defined(xdc_std__include) && !defined(_TI_STD_TYPES)
  
    typedef int                 Bool;    //32b
    typedef char                Int8;    // 8b
    typedef unsigned char       UInt8;   // 8b
    typedef short int           Int16;   //16b
    typedef unsigned short int  UInt16;  //16b
    typedef int                 Int32;   //32b
    typedef unsigned int        UInt32;  //32b
  #endif
  
#endif  

#ifndef UINT
  #define UINT                  unsigned int
#endif

#ifndef BYTE
  #define BYTE                  unsigned char
#endif

#ifndef NULL
  #define NULL                  0
#endif

#define START                   TRUE
#define PAUSE                   FALSE                           

#ifndef _X86
#define LOWORD(a)               ((WORD)(a))
#define HIWORD(a)               ((WORD)(((DWORD)(a) >> 16) & 0xffff))
#endif

#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b)  (((a) > (b)) ? (b) : (a))
#endif

/* C5xxx items */

#ifdef _C5XXX_

  #define _FAR               /* no far pointers in C5xxx */

/************************************************************************/
/*                   DSP memory mapping REGISTERS                       */
/************************************************************************/

  #define Input(port)           *(int *)(port)
  #define Output(port)          *(int *)(port)


/* status register 0 and 1 memory-mapped locations */

  #define ST0_REG               0x0006
  #define ST1_REG               0x0007

/* status register bit definitions */

  #define INTR_MASK_REG         0x0000
  #define INTR_FLAG_REG         0x0001
  #define FRCT                  6
  #define CPL                   14

/* Interrupt register parameters */

  #define MASK_INTR_ENABLE      0xF7FF
  #define MASK_INTR_DISABLE     0x0800


/* C54xx DMA Registers */

  #define DMA_PREC_REG          0x0054      /* DMA channel priority and enable control register */

#elif defined(_C6XXX_)

  #define _FAR                  far

  #define INTR_MASK_REG         0x0000L
  #define INTR_FLAG_REG         0x0001L

  #ifdef _C66XX_   /* added CJ, Oct 12 */

/* Define Timer Registers */

  // n denotes timer number
  //      TIMER_BASEADDR(n)       (0x02200000 + (n*0x00010000))     /* Base address of Timer registers; unused, only for reference */
    #define TIMER_EMUMGT_CLKSPD(n)  (0x02200004 + (n*0x00010000))     /* Emulation management and clock speed register */
    #define TIMER_CNTLO(n)          (0x02200010 + (n*0x00010000))     /* Counter register low */
    #define TIMER_CNTHI(n)          (0x02200014 + (n*0x00010000))     /* Counter register high */
    #define TIMER_PRDLO(n)          (0x02200018 + (n*0x00010000))     /* Period register low */
    #define TIMER_PRDHI(n)          (0x0220001C + (n*0x00010000))     /* Period register high */
    #define TIMER_TCR(n)            (0x02200020 + (n*0x00010000))     /* Timer control register */
    #define TIMER_TGCR(n)           (0x02200024 + (n*0x00010000))     /* Timer global control register */
    #define TIMER_WDTCR(n)          (0x02200028 + (n*0x00010000))     /* Watchdog timer control register */
    #define TIMER_RELLO(n)          (0x02200034 + (n*0x00010000))     /* Timer Reload register low */
    #define TIMER_RELHI(n)          (0x02200038 + (n*0x00010000))     /* Timer Reload register high */
    #define TIMER_CAPLO(n)          (0x0220003C + (n*0x00010000))     /* Timer Capture register low */
    #define TIMER_CAPHI(n)          (0x02200040 + (n*0x00010000))     /* Timer Capture register high */
    #define TIMER_INTCTLSTAT(n)     (0x02200044 + (n*0x00010000))     /* Timer interrupt control and status register */
  
    #define TIMER2_PERIOD           0x80000000 // temporary for building needs to be removed eventually

 #else

/* Define Timer Registers */

  #define TIMER0_CTRL           0x1940000   /* Address of timer0 control reg */
  #define TIMER0_PERIOD         0x1940004   /* Address of timer0 period reg */
  #define TIMER0_COUNT          0x1940008   /* Address of timer0 counter reg */

  #define TIMER1_CTRL           0x1980000   /* Address of timer1 control reg */
  #define TIMER1_PERIOD         0x1980004   /* Address of timer1 period reg */
  #define TIMER1_COUNT          0x1980008   /* Address of timer1 counter reg */

  #define TIMER2_CTRL           0x1AC0000   /* Address of timer2 control reg */
  #define TIMER2_PERIOD         0x1AC0004   /* Address of timer2 period reg */
  #define TIMER2_COUNT          0x1AC0008   /* Address of timer2 counter reg */

 #endif

/* key chip registers available as intrinsics in C code */

  extern cregister volatile unsigned int CSR;
  extern cregister volatile unsigned int IFR;
  extern cregister volatile unsigned int IER;
  extern cregister volatile unsigned int ICR;

#ifdef _TMS320C6700
  extern cregister volatile unsigned int FADCR;
  extern cregister volatile unsigned int FAUCR;
  extern cregister volatile unsigned int FMCR;
#endif


  #define Input(port)           ((port == INTR_MASK_REG) ? IER : ((port == INTR_FLAG_REG) ? IFR : *(volatile int*)(port)))
  #define Output(port)          (*(volatile int*)((port == INTR_MASK_REG) ? IER : ((port == INTR_FLAG_REG) ? IFR : port)))

/* interrupt selector values */

  #define         TINT1         0x2         /* 00010b  TINT1 timer1 interrupt */     
  #define         INT4          0x4         /* 00100b  INT4 external interrupt */
  #define         INT5          0x5         /* 00101b  INT5 external interrupt */
  #define         INT6          0x6         /* 00110b  INT6 external interrupt */
  #define         INT7          0x7         /* 00111b  INT7 external interrupt */
  #define         XINT0         0xC	        /* 01100b  XINT0 McBSP 0 transmit interrupt */
  #define         RINT0         0xD         /* 01101b  RINT0 McBSP 0 receive interrupt */
  #define         XINT0_rev1    0xE         /* 01110b  XINT0 McBSP 1 transmit interrupt for Rev 1.0 silicon */
  #define         RINT0_rev1    0xF         /* 01111b  RINT0 McBSP 1 receive interrupt for Rev 1.0 silicon */
  #define         XINT1         0xE         /* 01110b  XINT1 McBSP 1 transmit interrupt */
  #define         RINT1         0xF         /* 01111b  RINT1 McBSP 1 receive interrupt */
  #define         XINT2         0x11        /* 10001b  XINT2 McBSP 2 transmit interrupt */
  #define         RINT2         0x12        /* 10010b  RINT2 McBSP 2 receive interrupt */
  #define         IMH           0x19c0000   /* Mem Addr of Interrupt Multiplexer High*/
  #define         IML           0x19c0004   /* Mem Addr of Interrupt Multiplexer Low */
  #define         EXTPOL        0x19c0008   /* Mem Addr External Interrupt Polarity */
  
#endif

#ifndef _X86

/* property alias definitions allow linking to host-shared properties defined in t54def.asm 
   or t6xdef.asm */

extern volatile int* ANABFK;

extern _FAR volatile int* ANABFK;
extern _FAR int* ANABFK_C;
extern _FAR volatile int* STMBFK;
extern _FAR int* STMBFK_C;
extern _FAR volatile int* MPBBFK;
extern _FAR volatile int* RIBBFK;
extern _FAR volatile int* STBPTR;
extern _FAR volatile int* WNDBFK;
extern _FAR volatile unsigned int BUFLEN;
extern _FAR volatile unsigned int STBFLN;
extern _FAR volatile unsigned int FRMSIZ;
extern _FAR volatile int NUMCHN;
extern volatile int NUMCHNM1;
extern _FAR volatile int PCBFLG;
extern _FAR volatile int BUFNUM;
extern volatile int BFNADD;
extern _FAR volatile int MAXA;
extern _FAR volatile int MAXB;
extern volatile int MXAADD;
extern volatile int MXBADD;
extern _FAR volatile int FS_MODE;
extern _FAR volatile unsigned int FS_VAL;
extern _FAR volatile int CHNLIST;
extern _FAR volatile int BOARD;
extern _FAR volatile int SUBTYP;
extern _FAR volatile int MODVAL;
extern volatile int OPMODE;
extern _FAR volatile int AMPSCL;
extern _FAR volatile int DOFSET;
extern _FAR volatile int APSCL2;
extern _FAR volatile int DFSET2;
extern volatile int GNLIST;
extern volatile int TRGVAL;
extern volatile int MONTOR;
extern volatile int TSTMODE;
extern volatile int InterruptField;
extern volatile int ProcessorType;

/* real-time "snap-in" filtering properties */

extern volatile int FILUPD;
extern volatile int QNUM;
extern volatile int FLTYP1;
extern volatile int FLTYP2;
extern volatile int FLLEN1;
extern volatile int FLLEN2;
extern volatile _FAR int* COFXF1;
extern volatile _FAR int* COFXF2;


/* additional shared C5xxx/C6xxx definitions */

extern _FAR volatile int  TRGFLG;          /* trigger flag used inside instrumentation mode ISRs */
extern _FAR volatile int  PRVTRG;          /* trigger state machine flag "   "   " */
extern _FAR volatile int  SYNCFLG;         /* "SYNCH FLAG" FOR HOST-PC / DSP INTERLOCK */
extern _FAR volatile int* BUFPTR;


/* definitions currently only used by C6xxx SCI */

#ifdef _C6XXX_

extern _FAR volatile int  BFDONE;          /* CURRENT "TIME BUFFER DONE" NUMBER*/
extern _FAR volatile int  COUNT;
extern _FAR volatile int  SPTEMP;          /* TEMP VARIABLES*/
extern _FAR volatile int  TEMP;
extern _FAR volatile int  TEMP2;
extern _FAR volatile int  LGFLG1;          /* LOG FLAG TR A (0= NO LOG, 1=LOG, -1=-LOG) */
extern _FAR volatile int  LGFLG2;          /* LOG FLAG TR B (0= NO LOG, 1=LOG, -1=-LOG) */
extern _FAR volatile int  LOGFLG;          /* LOG FLAG FOR CURRENT TRACE */
extern _FAR volatile int  LOGC1;           /* LOG ROUTINE OUTPUT SCALING FACTOR */
extern _FAR volatile int  LOGC2;           /* LOG ROUTINE OUTPUT OFFSET FACTOR */
extern _FAR volatile int  PWFLG1;          /* POWER SPECTRA FLAG TR A */
extern _FAR volatile int  PWFLG2;          /* POWER SPECTRA FLAG TR B */
extern _FAR volatile int  PWRFLG;          /* POWER SPECTRA FLAG FOR CURRENT TRACE */
extern _FAR volatile int  TFFLG1;          /* TRANSFER FUNCTION FLAG TR A */
extern _FAR volatile int  TFFLG2;          /* TRANSFER FUNCTION FLAG TR B */
extern _FAR volatile int  TFRFLG;          /* TRANSFER FUNCTION FLAG FOR CURRENT TRACE */
extern _FAR volatile int  CURTRC;          /* CURRENT TRACE BEING OPERATED ON*/
extern _FAR volatile int  FFTSIZ;          /* FFT SIZE */
extern _FAR volatile int  MAGVAL;
extern _FAR volatile int  PHZREQ;          /* PHASE-REQUIRED FLAG */
extern _FAR volatile int  APCOEF;          /* POWER SPECTRA EXP. FILTER "A" COEFFICIENT */
extern _FAR volatile int  BPCOEF;          /* POWER SPECTRA EXP. FILTER "B" COEFFICIENT */
extern _FAR volatile int  MONTOR;
extern _FAR volatile int  CURCHN;
extern _FAR volatile int  SAMPLA;
extern _FAR volatile int  SAMPLB;
extern _FAR volatile int  SAMPLE;
extern _FAR volatile int  OUTTRA;
extern _FAR volatile int  OUTTRB;
extern _FAR volatile int  TRGCHN;
extern _FAR volatile int  OVFFLG;          /* OVERFLOW FLAG (0=NONE, 1=OVERFLOW) */
extern _FAR volatile int  WINSCL;          /* WINDOW/FRAMESIZE SCALING FACTOR*/
extern _FAR volatile int  MAXMAG;
extern _FAR volatile int  MAXFFT;          /* MAX FFT order */
extern _FAR volatile int  ORDVAL;          /* FFT order */
extern _FAR volatile int  FFTBUF;
extern _FAR volatile int  FFTFLG;          /* FLAG REFLECTING WHETHER DSP IS IN REAL-TIME */
extern _FAR volatile int  DACTR;
extern _FAR volatile int  ADCTR;
extern _FAR volatile int  ZERO;
extern _FAR volatile int  BZYFLG;          /* BUSY FLAG COMMUNICATED TO HOST */
extern _FAR volatile int  RIFLAG;          /* R/I DATA FLAG (0=NONE, 1=HOST REQUIRES) */
extern _FAR volatile int  IMAG;            /* IMAG/ANGLE PAIR USED TO STORE 32-BIT IMAG */
extern _FAR volatile int  IMAX;
extern _FAR volatile int  RMAX;
extern _FAR volatile int  RIB1;            /* TRACE A R/I BASE ADDRESS */
extern _FAR volatile int  RIB2;            /* TRACE B R/I BASE ADDRESS */
extern _FAR volatile int  MPBSAV;          /* MPBBFK ADDRESS SAVE VARIABLE */
extern _FAR volatile int  BUFRDY;

extern _FAR volatile int* SINTAB;
extern _FAR volatile int* COSINE;
extern _FAR volatile int* TRGADD;
extern _FAR volatile int* TCHADD;

extern _FAR volatile int SYNCFLG_MODE;

extern _FAR volatile int SITE0;
extern _FAR volatile int SITE1;

#elif defined(_C5XXX_)

/* C54xx analog I/O interrupt enable and low-level setup */

extern void InterruptEnable(int);
extern void AnalSetup();

#endif


#ifndef _USERPROC_FILE

/* application-specific real-time C code entry-point */

extern void UserProc(int *x, int *y,  unsigned int nBufSize,  unsigned int NumChan);  /* x and y pointers are to 16-bit fixed-point samples for C5xxx,
                                                                                         32-bit fixed-point samples for C6xxx */

#endif


/* hardware and system initialization items; initialized by boot/init code (t6xbeg or t54beg) */

extern _FAR volatile BOOL STNDALN;      /* indicates stand-alone operation when set */
#define fStandAloneMode STNDALN
extern _FAR volatile BOOL HWINIT;
#define fHwInitialized HWINIT           /* indicates low-level board resources and chip items have been initialized */
extern _FAR volatile BOOL USERCALL;
#define fHwInitFromUserCode USERCALL    /* indicates whether hardware initialization called from user code or not */


/* C code property alias definitions allow well-documented use in C code */

#define  TimeDataPtr        ANABFK_C
#define  StimDataPtr        STMBFK_C
#define  BufferLength       BUFLEN  /* length of input buffer in modes 0-2, 5-6 */
#define  StimBufferLength   STBFLN  /* length of output buffer in mode 6; see isr6 */
#define  RealTimeFramesize  FRMSIZ
#define  NumChan            NUMCHN
#define  ChanList           CHNLIST
#define  HostBufferNum      PCBFLG

#ifdef _C5XXX_
  #define DSPBufferNumLocal BUFNUM
  #define DSPBufferNumHost  BFNADD
#elif defined(_C6XXX_)
  #define DSPBufferNumHost  BUFNUM
  #define DSPBufferNumLocal BUFRDY
#endif

#define  MaxTraceA          MAXA
#define  HostMaxTraceA      MXAADD
#define  MaxTraceB          MAXB
#define  HostMaxTraceB      MXBADD
#define  FsMode             FS_MODE
#define  FsValue            FS_VAL
#define  Fs                 FsValue
#define  BoardType          BOARD
#define  BoardSubtype       SUBTYP
#define  OperatingMode      MODVAL
#define  AmpScale1          AMPSCL
#define  AmpOffset1         DOFSET
#define  AmpScale2          APSCL2
#define  AmpOffset2         DFSET2
#define  MagPhaseDataPtr    MPBBFK
#define  RIDataPtr          RIBBFK
#define  WindowDataPtr      WNDBFK   
#define  GainList           GNLIST
#define  TriggerValue       TRGVAL
#define  MonitorMode        MONTOR
#define  HostPoll           BZYFLG  /* JHB, Apr09 */

#define  InputData          TimeDataPtr
#define  OutputData         StimDataPtr

/* real-time "snap-in" filtering properties */

#define  FiltCoefUpdate     FILUPD
#define  QuantizationNum    QNUM
#define  FilterType1        FLTYP1
#define  FilterType2        FLTYP2
#define  FilterLen1         FLLEN1
#define  FilterLen2         FLLEN2
#define  FiltCoef1          COFXF1
#define  FiltCoef2          COFXF2


/* supported C54xx board types */

#define  TI_DSK_C542        0   /* Texas Inst DSK C542/548 board */
#define  SIG_C54XX          1   /* Signalogic C54xx modules / PC104 boards */
#define  TI_DSK_C5402       2   /* Texas Inst C5402 DSK board */
#define  SIG_C55XX          3   /* Signalogic C55xx modules / PC104 boards */


/* supported C6xxx board types */

#define  TI_DSK_C6X11       0   /* Texas Inst DSK C6x11 boards */
#define  IWCP_C6203         1   /* Viking-InterWorks C6203 module */
#define  II_M6X             2   /* Innovative Integration M67 and M62 boards */
#define  SIG_C67XX          3   /* Signalogic C67xx modules / PC104 boards */
#define  SIG_C64XX          4   /* Signalogic C641x PTMC boards */
#define  SIG_C66XX          5   /* Signalogic C667x PCIe cards */

/* supported C54xx processor subtypes */

#define  C549               0
#define  C5402              2
#define  C5409              9
#define  C5416              0x16
#define  C5421              0x21
#define  C5441              0x41
#define  C542X              128    // C542x dual core


/* supported C6xxx board subtypes */

#define  II_A4D4             0
#define  II_SD4              1
#define  II_AIX              2
#define  II_AIX20            3
#define  II_SD16             4
#define  II_A16D16           5

/* supported Host Application modes */

#define  CONT_INP            0
#define  CONT_OUT            1
#define  DIG_SCOPE           2
#define  ALG_FRAME           3
#define  ALG_FRAME_INV       4
#define  SPEC_ANALYZER       5
#define  CONT_IO             6
#define  ALG_FRAME2          7

#define  MCO_HOST            8
#define  VDS_HOST            9

/* maximum FFT size handled by default code */

#ifdef _C5XXX_

  #define  MAXFFTSIZE        11

#elif defined(_C6XXX_)

  #define  MAXFFTSIZE        12

#endif

#ifndef _EVMC6472  /* function prototype needed for non-EVM C6472 builds. JHB, Jan2011 */
void main_rtaf();
#endif

#endif /* ifndef _X86 */

#endif /* #ifndef RTC_DSP_INC */

