/*
  alarms.h
  
  Description:
   
  Alarm definition for x86, coCPU, or combined platforms

    -global
    -per C66x core
    -per channel

    Currently used by Pktlib and Voplib (may be expanded in the future)

  Copyright (C) Signalogic and Mavenir Systems 2013-2015

    Support for c66x card PCIe and SRIO interfaces

  Copyright (C) Signalogic, Inc, 2016-2017

    Add APIs to support x86-only or combined x86 and coCPU platforms.  APIs are now consistent between all use cases (for example, no difference betweeen coCPU platforms that use PCIe or SRIO interfaces)

  Revision History

    Created Sep 2013, JM

    Modified Sep 2013, JHB
      -add CPU and global alarms
      -test and modify for TI C6000 compiler

    Modified Jan 2015
      -change elements of size less than 32-bits to use bitfields
      -add support for BIG_ENDIAN
*/

#ifndef _ALARMS_H_
#define _ALARMS_H_

#include <stdint.h>

enum global_alarm_type {  /* global (card) alarms */

   DS_ALARM_GLOBAL_NONE = 0,
   DS_ALARM_CARD_TEMP,     /* card running too hot */
   DS_ALARM_GLOBAL_MAX
};

enum cpu_alarm_type {  /* CPU alarms */

   DS_ALARM_CPU_NONE = 0,
   DS_ALARM_CPU_FREQ,
   DS_ALARM_CPU_MAX
};

enum core_alarm_type {  /* per core alarms */

   DS_ALARM_CORE_NONE = 0,
   DS_ALARM_CORE_CPU_THRESHOLD,
   DS_ALARM_CORE_MEM_THRESHOLD,
   DS_ALARM_WATCHDOG_TIMER,
   DS_ALARM_CORE_MAX
};

/* Alarm list to cover general packet and audio processing functions */
/* Not all functions may be supported intially - JM */

enum channel_alarm_type { /* per-channel alarms */

   DS_ALARM_CHAN_NONE = 0,

/* RTP Activity Timeout */
   DS_ALARM_CHAN_RTP_ERR_TIMEOUT,

   DS_ALARM_CHAN_JITTER_UNDERRUN,  /* Jitter Buffer Alarms */
   DS_ALARM_CHAN_JITTER_OVERRUN,
   DS_ALARM_CHAN_JITTER_MAX_DELAY_EXCEEDED,
   DS_ALARM_CHAN_JITTER_AVG_DELAY_EXCEEDED,

/* Codec Alarms - Aggregated Error */
   DS_ALARM_CHAN_CODEC_ERR_ENCODE,
   DS_ALARM_CHAN_CODEC_ERR_DECODE,

/* DTMF Tone Generate & Detect Alarms - from TI VoLib (TGE/TDU) */
   DS_ALARM_CHAN_DTMF_ERR_GENERATE,
   DS_ALARM_CHAN_DTMF_ERR_DETECT,

/* Echo Cancellation Alarms - from TI VoLib (ECU) */
   DS_ALARM_CHAN_EC_ERR_MEMORY,
   DS_ALARM_CHAN_EC_ERR_OTHER,

/* Noise Reductions Alarms - from TI VoLib (VPE) */
   DS_ALARM_CHAN_NR_ERR_BADPARAM,
   DS_ALARM_CHAN_NR_ERR_OTHER,

/* Comfort Noise Generation Alarms - from TI VoLib (NMU) */
   DS_ALARM_CHAN_CNG_ERR_MEMORY,
   DS_ALARM_CHAN_CNG_ERR_OTHER,

   DS_ALARM_CHAN_MAX
};


/* Set bitmask alarm based on above type definitions */
#define DS_ALARM_SET(alm, alm_type)   (alm |= (1 << alm_type))
#define DS_ALARM_CLEAR(alm, alm_type) (alm &= ~(1 << alm_type))

/* Per Core Alarm Report */
struct dspcmd_alarm_event_core {

   uint32_t alarm_mask;  /* core_alarm_type */
};

/* Per Channel Alarm Report */

struct dspcmd_alarm_event_channel {

/* Channel Unique Identifier */

   uint32_t session_id;  /* from hash of ipdata and lookup saved session id */
   uint32_t priv;        /* Host application transparent info */
   uint32_t term_id;     /* termination ID as set by host */
   uint32_t alarm_mask;  /* channel_alarm_type */
};

/* Option to pack array of N channel alarms into size of mailbox message */
/* to reduce message volume in high network error conditions */

#define NUM_CHANNELS_PER_MULTICHANNEL_ALARM_REPORT 10

struct dspcmd_alarm_event_multichannel {

#ifdef __BIG_ENDIAN__
   uint32_t reserved : 16;
   uint32_t num_channels_in_report : 16;
#else
   uint32_t num_channels_in_report : 16;
   uint32_t reserved : 16;
#endif

   struct dspcmd_alarm_event_channel channel_alarm[NUM_CHANNELS_PER_MULTICHANNEL_ALARM_REPORT];
};


/* Per Channel Event Enumerations */

enum channel_event_type { /* per-channel rtp events */

   DS_EVENT_CHAN_NONE = 0,

   DS_EVENT_CHAN_RTP_DTMF_EVENT,     /* RTP DTMF Payload Event   */
   DS_EVENT_CHAN_DETECT_DTMF_EVENT,  /* Detected DTMF Tone Event */

   DS_RTP_EVENT_CHAN_MAX
};

struct dtmf_event {

#ifdef __BIG_ENDIAN__
   uint32_t volume : 8;
   uint32_t duration : 16;
   uint32_t event : 8;
#else
   uint32_t event : 8;
   uint32_t duration : 16;
   uint32_t volume : 8;
#endif
};

/* Per Channel RTP Event Structure */

struct dspcmd_event_channel {

/* Channel Unique Identifier */

   uint32_t session_id;  /* from hash of ipdata and lookup saved session id */
   uint32_t priv;          /* Host application transparent info */

#ifdef __BIG_ENDIAN__
   uint32_t reserved : 16;
   uint32_t term_id : 16;     /* termination ID as set by host */
#else
   uint32_t term_id : 16;     /* termination ID as set by host */
   uint32_t reserved : 16;
#endif

   uint32_t event_mask;  /* channel_event_type */

   struct dtmf_event dtmf; /* can be a union for other event values */
};

/* Option to pack array of N channel RTP events into size of mailbox message */
/* to reduce message volume */

#define NUM_CHANNELS_PER_MULTICHANNEL_EVENT_REPORT 5

struct dspcmd_event_multichannel {

#ifdef __BIG_ENDIAN__
   uint32_t reserved : 16;
   uint32_t num_channels_in_report : 16;
#else
   uint32_t num_channels_in_report : 16;
   uint32_t reserved : 16;
#endif

   struct dspcmd_event_channel channel_event[NUM_CHANNELS_PER_MULTICHANNEL_EVENT_REPORT];
};

#endif /* _ALARMS_H_ */
