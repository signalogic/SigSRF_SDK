/*
  /root/Signalogic/DirectCore/include/ds_vop.h

  Header file for DirectCore Voice/Video over Packet API library

  Copyright (C) Signalogic, 2005-2015
  Ver 2.0

  Revision History
  ----------------
  Created   May 2005, JS
  Modified  Jun-Nov 2006, development, JHB, TT
  Modified  Dec, added Vqmon support, DSConnectChannel()
  Modified  June 2007,DSOpenPktChannelEc() added, NR
  Modified  May-Jun 2010, merge with SigC641x library (v2.0), JHB
  Modified  Jan-Feb 2011, add video codec support, VR
  
*/

#ifndef _DS_VOP_H_
#define _DS_VOP_H_

/* constants */

#define MAX5561DSPS               12     /* max number of DSPs on SigC5561 card */
#define NUM5561CORESPERDSP        6      /* number of cores per DSP on SigC5561 card */
#define MAX5561CHANPERCORE        32     /* max number of channels per DSP core (G711 codec, 64 msec ec) */

#define MAXVDCDSPS                4      /* max number of DSPs on VDC card */
#define NUM6678CORESPERDSP        8      /* number of cores per DSP on VDC card */
#define MAX6678CHANPERCORE        16     /* max number of channels per DSP core (CIF profile, 30 fps) */

#if defined(NUM_TNETS) && (NUM_DSPS == 7)

  #define MAXVOPCHAN (7*NUM5561CORESPERDSP*MAX5561CHANPERCORE)

#elif defined(NUM_TNETS) && (NUM_DSPS == 2)

  #define MAXVOPCHAN (2*NUM5561CORESPERDSP*MAX5561CHANPERCORE)

#else

  #define MAXVOPCHAN (MAX5561DSPS*NUM5561CORESPERDSP*MAX5561CHANPERCORE)

#endif

#define WP_BUFSIZE32              2048   /* WinPath network processor buffer size, (num 32 bit words) */
#define WP_BUFSIZE8               8192


/* DSP code builds */

#define DS_ITU_DSPCODE_BUILD      1      /* wireline -- G729, G723, G726 */
#define DS_GSM_DSPCODE_BUILD      2      /* GSM FR, EFR, AMR */
#define DS_CDMA_DSPCODE_BUILD     3      /* EVRC */
#define DS_CABLE_DSPCODE_BUILD    5
#define DS_VDC_DSPCODE_BUILD      10     /* video content delivery */

/* Attributes for DSOpenXXXChannel() calls (XXX = Vop, Pkt, Tdm, etc.) */

/* codecs */

#define DS_CODEC_NONE             0            /* use this for diagnostics, e.g. loopback modes */
#define DS_CODEC_TYPE_MASK        0x00ff0000L

/* voice codecs, ITU */

#define DS_CODEC_G711_UL          0x00100000L  /* default is u-Law; see attributes below for A-Law */
#define DS_CODEC_G711_AL          0x00110000L
#define DS_CODEC_G726_A16         0x00120000L
#define DS_CODEC_G726_A24         0x00130000L
#define DS_CODEC_G726_A32         0x00140000L
#define DS_CODEC_G726_A40         0x00150000L
#define DS_CODEC_G729             0x00160000L
#define DS_CODEC_G729AB           0x00170000L
#define DS_CODEC_G729E            0x00180000L
#define DS_CODEC_G728             0x00190000L
#define DS_CODEC_G723_5300BPS     0x001a0000L
#define DS_CODEC_G723_6300BPS     0x001b0000L
#define IP_PASSTHRU               0x001c0000L

/* voice codecs, GSM */

#define DS_CODEC_GSM_FR           0x00200000L
#define DS_CODEC_GSM_EFR          0x00210000L
#define DS_CODEC_GSM_AMR          0x00220000L

/* voice codecs, CDMA */

#define DS_CODEC_EVRC             0x00300000L
#define DS_CODEC_SMV              0x00310000L

/* voice codecs, packet-over-cable */

#define DS_CODEC_ILBC             0x00400000L

/* voice codec attributes */

#define DS_CODEC_G726_MSBFIRST    0x00001000L
#define DS_CODEC_G711_APP1_PLC    0x00002000L  /* Appendix I Packet Loss Concealment */
#define DS_VAD_ENABLED            0x00004000L  /* make VAD active (if codec type supports VAD) */
#define DS_VAD_DISABLED           0

/* echo cancellation */

#define DS_EC_32                  0x01000000L
#define DS_EC_64                  0x02000000L
#define DS_EC_96                  0x03000000L
#define DS_EC_128                 0x04000000L
#define DS_EC_MASK                0x0f000000L

/* payloads per packet */

#define DS_PTIME_1                0x00000100L  /* number of payloads per packet (default is one packet if not specified) */
#define DS_PTIME_2                0x00000200L
#define DS_PTIME_3                0x00000300L
#define DS_PTIME_4                0x00000400L
#define DS_PTIME_5                0x00000500L
#define DS_PTIME_6                0x00000600L
#define DS_PTIME_7                0x00000700L
#define DS_PTIME_8                0x00000800L
#define DS_PTIME_9                0x00000900L
#define DS_PTIME_10               0x00000A00L
#define DS_PTIME_11               0x00000B00L
#define DS_PTIME_12               0x00000C00L  /* max of delay of 120 msec (12x 10 msec packets) */
#define DS_PTIME_MASK             0x00000f00L

/* video codecs */

#define DS_CODEC_MPEG2            0x00500000L  /* MPEG2 */
#define DS_CODEC_MPE4P2           0x00510000L  /* MPEG4 Part 2 */
#define DS_CODEC_MPE4P10          0x00520000L  /* MPEG4 Part 10 */
#define DS_CODEC_H264             0x00530000L  /* H.264 (MPEG4 Part 10 plus RTP differences) */

/* video codec profiles and fps */

#define DS_PROFILE_QCIF           0x00001000L
#define DS_PROFILE_CIF            0x00002000L
#define DS_PROFILE_SD             0x00003000L  /* same as 4CIF */
#define DS_PROFILE_HD             0x00004000L

#define DS_FPS_12                 0x0c000000L  /* some frequently used values for fps.  Otherwise specify the exact value, from 1 to 255 */
#define DS_FPS_15                 0x0f000000L
#define DS_FPS_30                 0x1e000000L
#define DS_FPS_50                 0x32000000L
#define DS_FPS_60                 0x3c000000L


/* RTP attributes (dwRtpAttributes constants) */

#define DS_G711_PAYLOAD_TYPE      0
#define DS_G729_PAYLOAD_TYPE      18
#define DS_GSM_AMR_PAYLOAD_TYPE   1

#define DS_PAYLOAD_TYPE_MASK      0x000000ffL
#define DS_TXSSRC_MASK            0x0000ff00L
#define DS_RXSSRC_MASK            0x00ff0000L


/* channel attributes (dwChanAttributes constants) */

#define DS_CH_XDPSET                 0x0001    /* should only be used with open channel APIs */
#define DS_CH_XDPRESET               0x0002    /* should only be used with DSCloseChannel() API */
#define DS_CH_NOPRINT                0
#define DS_CH_PRINT                  0x0004

#define DS_CH_RXEN                   0x0010
#define DS_CH_TXEN                   0x0020

#define DS_CH_PCM_LPBK_OFF           0         /* diagnostic attributes -- active if codec type is set to none */
#define DS_CH_PCM_LPBK_8BIT          0x0100
#define DS_CH_PCM_LPBK_16BIT         0x0200
#define DS_CH_PCM_PASSTHRU_OFF       0
#define DS_CH_PCM_PASSTHRU_ON        0x0400
#define DS_CH_PKT_TX_LPBK_OFF        0
#define DS_CH_PKT_TX_LPBK_ON         0x0800
#define DS_CH_PKT_RX_LPBK_OFF        0
#define DS_CH_PKT_RX_LPBK_ON         0x1000

#define DS_DESIRED_BITRATE_MASK      0xffff0000L

/* Encryption */

#define DS_ENCRYPTION_TYPE_NONE      0
#define DS_ENCRYPTION_TYPE_AES       1
#define DS_ENCRYPTION_TYPE_A51       2
#define DS_ENCRYPTION_TYPE_A52       3
#define DS_ENCRYPTION_TYPE_USER1     4
#define DS_ENCRYPTION_TYPE_USER2     5
#define DS_ENCRYPTION_TYPE_USER3     6
   

/* API calls */

#ifdef __cplusplus
  extern "C"{
#endif

#ifndef _DIRECTCORE_

/* following defines needed without alias.h, which is included by hwlib.h */

  #ifndef BYTE
    #define BYTE unsigned char
  #endif
  #ifndef BOOL
    #define BOOL unsigned int
  #endif
  #ifndef WORD
    #define WORD unsigned short int
  #endif
  #ifndef DWORD
    #define DWORD unsigned long
  #endif
  #ifndef HCARD
    #define HCARD unsigned int
  #endif
  #define HDRIVER unsigned int
  #ifndef UINT
     #define UINT unsigned int
  #endif
  #define TRUE 1
  #define FALSE 0
  #ifndef HWND  
    #define HWND UINT
  #endif

  #ifndef NULL
    #define NULL 0
  #endif

  #define FAR 

  UINT DSAppErrMsg(HWND, const char*, const char*, DWORD);

/* Note:  following four (4) APIs are deprecated -- used to build old version of voplib, but no longer supported under DirectCore.
          These APIs should be avoided, and equivalent APIs in cimlib (high level) or hwlib (lower level) should be used.  See
          terminal.c program for an example. (Jul2010, JHB) */

  HCARD DSAssignCard(HDRIVER, const char*, int, int, int);
  HDRIVER DSLibInit(HCARD, const char*, WORD, WORD);
  UINT DSInitCard(HCARD);
  UINT DSLibClose(HCARD);

#else

//  #define HCARD HBOARD
  #define HDRIVER unsigned int
  
#endif

/* VoP channel handle */

typedef int HCHAN;


/* channel open APIs */

HCHAN DSOpenVopChannel(HCARD hCard,
                       WORD  wInTDMTimeSlot,               /* incoming TDM data, from 0 to 4095 */
                       WORD  wOutTDMTimeSlot,              /* outgoing TDM data, from 0 to 4095 */
                       DWORD dwCodec_and_Attributes,
                       char* szSrcIP_UDP,                  /* source IP/UDP (i.e. SigC5561 card) */
                       char* szDstIP_UDP,                  /* network connection IP/UDP */
                       DWORD dwRtpAttributes,
                       DWORD dwChanAttributes);

HCHAN DSOpenVopChannelEx(HCARD hCard,
                         WORD  wInTDMTimeSlot,             /* incoming TDM data, from 0 to 4095 */
                         WORD  wOutTDMTimeSlot,            /* outgoing TDM data, from 0 to 4095 */
                         DWORD dwCodec_and_Attributes,
                         char* szSrcIP_UDP,                /* source IP/UDP (i.e. SigC5561 card) */
                         char* szDstIP_UDP,                /* network connection IP/UDP */
                         char* szSrcMACAdrs,               /* Src MAC Address for channel Eg:"2A:2B:2C:2D:2E:2F"(Default)*/
                         char* szDstMACAdrs,               /* Dst MAC Address for channel Eg:"2A:2B:2C:2D:2E:2F"(Default)*/
                         DWORD dwRtpAttributes,
                         DWORD dwChanAttributes);

HCHAN DSOpenPktChannel(HCARD hCard,
                       DWORD dwCodec_and_Attributes,       /* voice/video codec and attributes */
                       char* szSrcIP_UDP,                  /* source IP/UDP (i.e. SigC5561 or VDC card) */
                       char* szDstIP_UDP,                  /* network connection IP/UDP */   
                       DWORD dwRtpAttributes,
                       DWORD dwChanAttributes);

HCHAN DSOpenPktChannelEx(HCARD hCard,
                         DWORD dwCodec_and_Attributes,     /* voice/video codec and attributes */
                         char* szSrcIP_UDP,                /* source IP/UDP (i.e. SigC5561 or VDC card) */
                         char* szDstIP_UDP,                /* network connection IP/UDP */   
                         char* szSrcMACAdrs,               /* Src MAC Address for channel Eg:"2A:2B:2C:2D:2E:2F"(Default)*/
                         char* szDstMACAdrs,               /* Dest. MAC address for channel Eg:"1A:1B:1C:1D:1E:1F"(Default)*/
                         DWORD dwRtpAttributes,
                         DWORD dwChanAttributes);

HCHAN DSOpenTdmChannel(HCARD hCard,
                       WORD  wInTDMTimeSlot,
                       WORD  wOutTDMTimeSlot,
                       DWORD dwChanAttributes);

HCHAN DSOpenSigChannel(HCARD hCard,
                       WORD  wInTDMTimeSlot,
                       WORD  wOutTDMTimeSlot,
                       DWORD dwSigType_and_Attributes,
                       char* szSrcIP_UDP,
                       char* szDstIP_UDP,
                       DWORD dwRtpAttributes,
                       DWORD dwChanAttributes);

HCHAN DSOpenDatChannel(HCARD hCard,
                       WORD  wInTDMTimeSlot,
                       WORD  wOutTDMTimeSlot,
                       DWORD dwInFaxProtocol_and_Attributes,
                       char* szSrcIP_UDP,
                       char* szDstIP_UDP,
                       DWORD dwRtpAttributes,
                       DWORD dwChanAttributes);

#define DSOpenVOPChannel DSOpenVopChannel
#define DSOpenPKTChannel DSOpenPktChannel
#define DSOpenTDMChannel DSOpenTdmChannel
#define DSOpenSIGChannel DSOpenSigChannel
#define DSOpenDATChannel DSOpenDatChannel


/* Connect IP channels (e.g. transcoding) or TDM channels */

#define DS_CONNECT_TDM_TDM        1
#define DS_CONNECT_IP_IP          2
#define DS_CONNECT_IP_IP_FLOWTHRU 3
#define DS_CONNECT_BROADCAST_IP   0x100
#define DS_CONNECT_BROADCAST_TDM  0x200

int DSConnectChannel(HCARD hCard,
                     HCHAN hChan1,
                     HCHAN hChan2,
                     WORD  wConnectAttributes);


/* Attributes for DSSetRtp() */

#define DS_SET_RX                 1
#define DS_SET_TX                 2
#define DS_RTP_IP                 0  /* default if network type not given */
#define DS_RTP_AAL2               4
#define DS_CONT_PKT_FLOW          0  /* default */
#define DS_STOP_PKT_FLOW          8




/* Set RTP packetization parameters.  Notes:

  1) Asymmetric Rx/Tx codec and packet time (num packets per payload) settings are supported in some cases.

*/

int DSSetRtp(HCHAN hChan,
             DWORD dwAttributes,
             WORD  wPayloadType,     /* RTP payload type */
             WORD  wVif,             /* in bits, see VIF-Packet Framesize table in documentation */
             WORD  wSyncSource,      /* starting Sync Source value */
             WORD  wTimeStamp,       /* starting Timestamp value */
             WORD  wCsrcListCount);  /* CSRC List Count */
             
#define DSSetRTP DSSetRtp


/* Attributes for DSSetEc() */

#define DS_EC_NLP_ENABLE           0     /* default is non-linear processing enabled */
#define DS_EC_NLP_DISABLE          1
#define DS_EC_NLP_NORM_ENABLE      0     /* default */
#define DS_EC_NLP_NORM_DISABLE     2
#define DS_EC_CONV_MODE_ADAPT      0     /* default is adaptive mode */
#define DS_EC_CONV_MODE_FIXED      4
#define DS_EC_4WIRE_ENABLE         0     /* default is 4-wire mode */
#define DS_EC_4WIRE_DISABLE        8
#define DS_EC_UPDATE_ENABLE        0
#define DS_EC_UPDATE_DISABLE       0x10  /* disable error term calculation and convergence */
#define DS_EC_ACOUSTIC_UPD_DISABLE 0
#define DS_EC_ACOUSTIC_UPD_ENABLE  0x20
       
int DSSetEc(HCHAN hChan,
            WORD  wTailLength,   /* in msec, rounded to nearest 32.  Min is 32, max is 128.  Zero wil disable */
            WORD  wAttributes,
            WORD  wNoiseLevel);  /* in dB.  Default value is 10 dB */

#define DSSetEC DSSetEc


/* Attributes for DSSendRtpCtrlMsg() and DSSendRtcpDataPkt() */

#define DS_NO_HDR_ENCRYPTION       0  /* default if header encryption not specified */
#define DS_EN_HDR_ENCRYPTION       1
#define DS_NO_TX_CTRL_PROTOCOL     0  /* default if Tx control protocol not specified */
#define DS_EN_TX_CTRL_PROTOCOL     2
#define DS_DATA_PKT_DELAY_OFF      0  /* default */
#define DS_DATA_PKT_DELAY_ON       4
#define DS_CONT_DATA_PKT_TRAFFIC   0  /* default */
#define DS_STOP_DATA_PKT_TRAFFIC   8

int DSSendRtcpCtrlMsg(HCHAN hChan,
                      WORD  wTxRepeatInterval,
                      DWORD dwNtpTimeStamp,
                      WORD  wSessionTimeout,
                      WORD  wAttributes);

int DSSendRtcpDataPkt(HCHAN hchan,
                      WORD  wTransmitID,
                      WORD  wAttributes,
                      BYTE* pPktdata,
                      WORD  wNumBytes);
                                            
#define DSSendRTCPCtrlMsg DSSendRtcpCtrlMsg
#define DSSendRTCPDataPkt DSSendRtcpDataPkt


/* DSSetChan -- alter channel already opened.  Notes:

   1) For UDP/time-slot parameters, if high word of pointer is zero, then parameter is interpreted
      as a TDM time-slot (value contained in low word).
 
   2) Only limited combinations of codec and ec attribute switchovers are supported.  Please
      consult documentation.
*/

int DSSetChan(HCHAN hChan,
              DWORD dwCodec_or_Protocol_and_Attributes,
              char* szSrcIP_UDP_or_InTDMTimeSlot,
              char* szDstIP_UDP_or_OutTDMTimeSlot,
              DWORD dwRtpAttributes,
              DWORD dwChanAttributes);
              

/* DSCloseChannel -- close and de-allocate channel */

int DSCloseChannel(HCARD hCard, HCHAN hChan, DWORD dwChanAttributes);


typedef struct {

   DWORD      dwFlags;        /* flags */
   DWORD      dwErrCode;
       
} CHANSTAT, FAR* PCHANSTAT;

int DSGetChanStatus(HCHAN hChan,
                    PCHANSTAT pChanStats,
                    int nSizeofStruct);

typedef struct {

   int        nDSP;           /* which DSP is running the specified channel */
   int        nCore;          /* which DSP core is running the specified channel (note that TNETV3010 DSP has 6 cores per DSP) */

   int        nRxTimeSlot;    /* assigned Rx time slot for the channel (if TDM-to-IP or TDM-to-TDM channel, otherwise returns -1) */
   int        nTxTimeSlot;    /* assigned Tx time slot for the channel (if TDM-to-IP or TDM-to-TDM channel, otherwise returns -1) */
      
} CHANINFO, FAR* PCHANINFO;

int DSGetChanInfo(HCHAN hChan,
                  PCHANSTAT pChanInfo,
                  int nSizeofStruct);

typedef struct {

   HCHAN      hChan;          /* channel */
   DWORD      dwSigEvents;    /* signaling events */
   DWORD      dwFlags;        /* flags */

} VOPEVENTS, FAR* PVOPEVENTS;

int DSGetVopEvents(PVOPEVENTS, int);

#define DSGetVOPEvents DSGetVopEvents

typedef struct {

/* packet statistics */
   
   DWORD      dwRxVoicePackets;
   DWORD      dwTxVoicePackets;
   DWORD      dwRxEnetPackets;
   DWORD      dwTxEnetPackets;
   DWORD      dwRxSIDPackets;
   DWORD      dwTxSIDPackets;
   DWORD      dwRxDTMFRelayPackets;  /* RFC 2833 packets */
   DWORD      dwTxDTMFRelayPackets;
   DWORD      dwLostEnetPackets;
   
/* time, jitter statistics */
      
   WORD       wRxMinPktTime;    /* min time between packet arrivals, in msec */
   WORD       wRxMaxPktTime;    /* max time (delay) between packet arrivals, in msec */
   WORD       wRxRTPAvgJitter;  /* in PCM samples */

/* RTP parameters */
      
   DWORD      dwRxLastTimeStamp;
   DWORD      dwTxLastTimeStamp;
   WORD       wRxLastSeqNumber;
   WORD       wTxLastSeqNumber;
   WORD       wRxExtSeqNumber;
   WORD       wTxExtSeqNumber;
   //unsigned int	      RemSrcPortNum;
 //  char       RemSrcIP[16];
 
} VOPSTATISTICS, FAR* PVOPSTATISTICS;


#define DS_GCVS_CHAN_HANDLE 0
#define DS_GCVS_CHAN_NO     1
#define DS_GCVS_PRINT       2

int DSGetChanVopStatistics(HCARD hCard,
                           HCHAN hChan,
                           UINT uAttributes,
                           PVOPSTATISTICS pVopStatistics,
                           int nSizeofStruct);
   
typedef struct {

   unsigned int RemSrcPortNum;
   char RemSrcIP[15];

} PKTINFO, FAR* PPKTINFO;

#define DSGetChanVOPStatistics DSGetVopStatistics

typedef struct {

   DWORD      dwRxPackets;
   DWORD      dwTxPackets;
   DWORD      dwL2ErrorCount;
   DWORD      dwL3ErrorCount;
   DWORD      dwL4ErrorCount;
   
} CORESTATISTICS, FAR* PCORESTATISTICS;

int DSGetDspStatistics(int nCore,                        /* 0 - 71 for SigC5561 card.  Note: each TNETV3010 DSP has 6 cores; repeat this call with nCore 0 to 5 to get aggregate data for one DSP */
                       PCORESTATISTICS pCoreStatistics,
                       int nSizeofStruct);

#define DSGetCOREStatistics DSGetCoreStatistics

typedef struct {

   WORD       wImage;
   int        nChan[MAX5561CHANPERCORE];
   
} COREUSAGE, FAR* PCOREUSAGE;

BOOL DSGetCoreUsage(int nCore,                           /* 0..71 */
                    PCOREUSAGE pCoreUsage);


typedef struct {

   WORD       wMOS_LQ;               /* estimated Listening Quality MOS score (divide by 100 to get result in 0 - 4.5 range) */
   WORD       wMOS_CQ;               /* estimated Conversational Quality MOS score (   "   "   ) */
   WORD       wMOS_PQ;               /* ITU-T P.862 (PESQ) normalized raw quality score (   "   "   ) */
   WORD       wR_LQ;                 /* Listing Quality R-Factor (1 - 100 range) */
   WORD       wR_CQ;                 /* Conversational Quality R-Factor (   "   "   ) */
   WORD       wVQmon_Nom_MOS;        /* divide by 100 to get result in 0 - 4.5 range */
   WORD       wVQmon_Nom_R_Factor;   /* R-Factor -- quality score based on end-point and network parameters, ncludes codecs,
                                        packet loss, and delay (1 - 100 range) */

} VQMONSTATISTICS, FAR* PVQMONSTATISTICS;

int DSGetVqmonStatistics(HCARD hCard,
                         HCHAN hChan,
                         PVQMONSTATISTICS pVQmonStatistics,
                         int nSizeofStruct);

#define DSGetVQmonStatistics DSGetVqmonStatistics

/* Retrieve remote src port num (and IP addr if required) of incoming RTP packets on channel with src IP addr defined by ChanSrcIP */

unsigned int DSGetRemNwInfo(HCARD hCard, HCHAN hChan, char* ChanSrcIP, PPKTINFO ptr, char*);


/* constants usable in uAttributes param of DSGetNetworkDataProcessor() (note: constants can be combined (OR'd together)) */

#define DS_GNPD_READNEW   0
#define DS_GNPD_READALL   1
#define DS_GNPD_PEEKNEW   2
#define DS_GNPD_NOPRINT   0
#define DS_GNPD_PRINT     4
#define DS_GNPD_PARSE     8

/* get data from WinPath network processor on SigC5561 card */

int DSGetNetworkProcessorData(HCARD hCard,
                              UINT  uAttributes,
                              char* pData,
                              WORD  wLen);
                               

/* constants usable in uAttributes param of DSPutNetworkDataProcessor() (note: constants can be combined (OR'd together)) */

#define DS_PNPD_NOLINE  0
#define DS_PNPD_ADDLINE 1
#define DS_PNPD_NOPAD   2

/* send data to WinPath network processor on SigC5561 card */

int DSPutNetworkProcessorData(HCARD hCard,
                              UINT  uAttributes,
                              char* pData,
                              WORD  wLen);

/* Load executable code to DSPs via WinPath network processor on SigC5561 card */

BOOL DSLoadFileNetworkProcessor(HCARD hCard,
                                WORD  wImage,
                                DWORD dwProcList);

#ifdef __cplusplus
  }
#endif

#endif  /* _DS_VOP_H_ */
