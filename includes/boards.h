/*

  $Header: /Signalogic/DirectCore/include/boards.h

  boards.h: boards and cards definitions header file

  Copyright (C) Signalogic, 1992-2017

  Revision History:

   Created  Jan 1995 JHB

   Modified Jul2010 JHB, added product definitions that can be used with DSGetBoardInfo() (see comments at end of file) included in published .h files, 

   Modified Oct/Nov 2012 JHB, add support for C66x

   Modified Oct 2014 JHB, add support for Advantech ATCA board (look for ADV8901 definition)

   Modified Aug 2017 JHB, replaced C++ style comments with C style for compatibility with ISO C90 sources

   Projects:  Borland Win16 hwlib.ide
              MSVC    Win32 hwdriver32
              Linux LkM and shared library
              Linux voplib, callmgr, cimlib applications

*/

#ifndef SIG_BOARDS_H
#define SIG_BOARDS_H

#define        PCI_CAP          0x8000   /* PCI capable (PCI, PCIe, PMC, PTMC, PC104+) */

/* Texas Instruments TMS320C25 boards */

#define        SPEC320C25       0        /* Spectrum/LSI TMS320C25 board */
#define        SWDS320C25       1        /* TI SWDS */
#define        ARIEL_DSP16      2        /* Ariel DSP-16; DSP-C25 is subspecies */
#define        PCI20000         4        /* Burr-Brown DSP carrier */


/* Texas Instruments TMS320C5x, C54xx, C55x, C62/7x, C64x boards */

#define        C5XXX_C6XXX_BEG  TIC54X
#define        TIC54X           5        /* Texas Instruments DSK C54x board (TMS320C542/8/9) and DSK C5402 board */
#define        TIC55X           5        /* ...also includes some C55x boards (SigC5561) */
#define        TIC6XX           6        /* Texas Instruments DSK C62xx and DSK C67xx boards */
#define        IIM6X            7        /* Innovative Integration M62, M67 (TMS320C6201, TMS320C6701) */
#define        DM64X            8        /* Texas Instruments DM64x EVM card */
#define        IIC50            9        /* Innovative Integration PC50 (TMS320C50)# */
#define        C5XXX_C6XXX_END  IIC50

/* AT&T DSP32 boards */

#define        DSP32_BEG        CACDSP32
#define        CACDSP32         10       /* CAC DSP32-PC */
#define        ZPB32            14       /* Burr-Brown ZPB32 */
#define        DSP32_END        ZPB32

/* Motorola DSP56001 boards */

#define        DSP5600x_BEG     SPEC56001
#define        SPEC56001        20       /* Spectrum/LSI DSP56001 board */
#define        ADS56001         21       /* Motorola ADS development system */
#define        MULTISOUND       22       /* Turtle Beach Systems MultiSound board */
#define        DOM56            23       /* Domain DSPCard 56002 */
#define        PER56            24       /* Peralex SIP-56 (4x DSP56002, Falcon */
#define        MOT563xx         25       /* Motorola EVM563xx boards */
#define        MOT56002         26       /* Motorola EVM56002 board */
#define        DOMAUD4          27       /* Domain audio 4 unit with USB interface */
#define        ARIEL_PC56       29       /* DSP-56 is subspecies */
#define        DSP5600x_END     ARIEL_PC56

/* additional flags */

#define        MOT_309          1
#define        MOT_307          1


/* AT&T DSP32C boards */

#define        DSP32C_BEG       ARIEL_DSP32C
#define        ARIEL_DSP32C     30       /* Ariel DSP-32C */
#define        ATTDEV32C        31       /* AT&T development board */
#define        CAC32C           32       /* CAC DSP32C-AT */
#define        BB32C            33       /* Burr-Brown ZPB34 */
#define        MTT32C           34       /* MTT Lory DSP32C (DSP4100) */
#define        NAT32C           35       /* National Instruments AT-DSP2200 DSP32C */
#define        QWV32C           36       /* Quantawave Qw32C-8AFA (Sig32C-8) */
#define        SPEC32C          39       /* Spectrum/LSI DSP32C board */
#define        DSP32C_END       SPEC32C

/* Analog Devices ADSP-21xx boards */

#define        ADSP_21xx_BEG    SPEC2100
#define        SPEC2100         40       /* Spectrum/LSI ADSP-2100 board */
#define        DX2100           41       /* Logabex ADSP-2100 */
#define        SPEC2101         44       /* Spectrum/LSI ADSP-2101 board */
#define        ADSP_21xx_END    SPEC2101

/* Texas Instruments TMS320C4x boards */

#define        TMS320C4x_BEG    ARIEL_DSPC40
#define        ARIEL_DSPC40     45       /* Ariel DSP-C40 TMS320C40 board */
#define        IIC44            46       /* Innovation Integration PC44 (TMS320C44) */
#define        SIGC44           47       /* Signalogic SigC44 (TMS320C44) */
#define        IIM44            48       /* Innovative Integration M44 (TMS320C44) */
#define        SPECC40          49       /* Spectrum/LSI TMS320C40 board */
#define        TMS320C4x_END    SPECC40

/* Texas Instruments TMS320C3x boards */

#define        TMS320C3x_BEG    SPECC30
#define        SPECC30          50       /* Spectrum/LSI TMS320C30 board */
#define        SONC30           51       /* Sonitech Spirit-30 */
#define        EVMC30           52       /* TI EVM */
#define        DRIC30           53       /* DSP Research Tiger-30 */
#define        MTTC30           54       /* MTT Lory DSP4200 (TMS320C30) */
#define        IIC31            55       /* Innovation Integration PC31 (TMS320C31) */
#define        DOMC31           56       /* Domain Tech. DSPCard (TMS320C31) */
#define        QWVC31           57       /* Quantawave QwC31-4AFA (SigC31-4) */
#define        IIC32            58       /* Innovation Integration PC32 and ADC64 (TMS320C32) */
#define        ASPIC30          59       /* ASPI Banshee */
#define        TMS320C3x_END    ASPIC30

/* XT-style 1-DMA-channel boards */

#define        PCI20041         60       /* Burr-Brown PCI-20041C-2A & PCI-20041C-3A */
#define        DAS16            61       /* Metrabyte DAS-16 */


/* AT-style 2-DMA-channel boards */

#define        DT2821_F         65       /* Data Translation 282x series */
#define        ACRO_AT6400      66       /* Analogic AT6400 */
#define        DAL_DRB1         67       /* Digital Audio Labs DRB1 board */


/* Analog Devices ADSP-2106x boards */

#define        ADSP_21xxx_BEG   BIT06X
#define        BIT06X           70       /* BittWare ADSP-21062 BlackTip board */
#define        ADEVM61          71       /* Analog Devices EVM21061 board */
#define        QWV062           79       /* Quantawave Qw062 (Sig062) */
#define        ADSP_21xxx_END   QWV062

/* Motorola DSP9600x boards */

#define        DSP9600x_BEG     ARIEL_DSP96
#define        ARIEL_DSP96      80       /* Ariel DSP-96 or MM96 board */
#define        SPEC96           84       /* LSI/Spectrum DSP96002 system board */
#define        DSP9600x_END     SPEC96

/* AT&T DSP3210 boards */

#define        DSP3210x_BEG     ARIEL_DSP3210
#define        ARIEL_DSP3210    85       /* Ariel DSP-3210 or MP3210 board */
#define        CAC3210          86       /* CAC QUANTUMdsp board */
#define        SPEC3210         89       /* doesn't exist yet */
#define        DSP3210x_END     SPEC3210

/* NEC 77220 boards */

#define        NEC_77220        90       /* NEC 77P220 DDK development board */
#define        NEC_77220_END    94

/* Texas Instruments TMS320C6x boards */

#define        EVMC6201         95      /* DNA Ent/TI EVM C6x board */
#define        QUATRO67         96      /* II Quatro67 board */


/* some convenient definitions that can be used with DSGetCardInfo(hBoard, DS_GCI_TYPE) */

#define SIGC641X                (PCI_CAP | 0x0800 | TIC6XX)   /* SigC641x PCI, PCIe, and PTMC */
#define SIGC5561                (PCI_CAP | 0x0800 | TIC55X)   /* SigC5561 PCI, PCIe, and PTMC */
#if 0
#define TIDM642                 (0x0800 | TIC6XX)             /* TI EVM DM642 */
#endif
#define TIDM642                 (PCI_CAP | 0x0800 | DM64X)    /* TI EVM DM642 */
#define SIGC54XX                (0x0400 | TIC54X)             /* SigC5416 PC104 (also 5402, 5409) */
#define SIGC6X11                (0x0200 | TIC6XX)             /* SigC6711 and SigC6211 PC104 */
#define X86                     (0x0000 | 99)                 /* generic x86 platform */
#define SIGC667X                (PCI_CAP | 0x0100 | TIC6XX)   /* SigC667x PCIe */
#define SIGC66X                 SIGC667X

#define ADV8901                 (0x0100 | TIC6XX)             /* Advantech 8901 ATCA board with up to 20 C6678 devices (up to 160 C66x cores) */

#endif /* SIG_BOARDS_H */
