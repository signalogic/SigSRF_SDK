/*
  session_cmd.h

  Session commands and APIs for c66x, x86, or combined platforms

  Copyright (C) Signalogic, Inc, 2008-2012

    Created for c64x voice platforms

  Copyright (C) Mavenir Systems and Signalogic, Inc, 2013-2015
  
    Support c66x co-CPU card PCIe or ATCA blade SRIO interface

  Copyright (C) Signalogic, Inc, 2016-2018

    Add APIs to (i) encapsulate c66x PCIe and SRIO interfaces, and (ii) support x86-only or combined x86 and c66x server usage.  APIs are consistent between all use cases

  Revision History

    Sep 2013, Created, Yuan Lei, support SigC66xx multicore CPU PCIe cards

    Sep 2013, Modified, JHB
      -test and modify for TI C6000 compiler
      -verify stdint.h for TI c66x compilers, JHB

    Jan 2015
      -change elements of size less than 32-bits to use bitfields
      -add support for BIG_ENDIAN to support ATCA platforms (currently assumes SRIO interface)

    Feb 2015
      -add support for IP configuration command
      
    Dec 2016 - Jan 2017, JHB
      -add APIs for session create/delete, session encode and decode
*/


#ifndef _SESSION_CMD_H_
#define _SESSION_CMD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "session.h"
#include "config.h"
#include "alarms.h"

#include <stdint.h>

#if 1  /* moved here from pktlib.h, JHB Mar18 */

#define DS_SESSION_MODE_FRAME                 1              /* session data in frame format */
#define DS_SESSION_MODE_IP_PACKET             2              /* session data in IP/UDP/RTP packet format */
#define DS_SESSION_MODE_UDP_PACKET            4              /* session data in UDP/RTP packet format */

#define DS_SESSION_DP_NONE                    0              /* library manually receives and sends packets from application via API */
#define DS_SESSION_DP_LINUX_SOCKETS           0x10           /* packet flow path is Linux sockets */
#define DS_SESSION_DP_DPDK_QUEUE              0x20           /* packet flow path is DPDK mem queue */
#define DS_SESSION_DP_COCPU_QUEUE             0x40           /* in frame operating mode, frame flow path is coCPU card mem queue.  In packet operating mode, packets flow through-CPU card mem queue but are not processed on the card */

#endif

enum cmd_type {

   DS_CMD_CONFIGURATION,                  /* CP -> C66x */
   DS_CMD_HEARTBEAT,                      /* C66x -> CP */
   DS_CMD_OVERLOAD_NOTIFICATION,          /* C66x -> CP */
   DS_CMD_ALARM_INDICATION,               /* C66x -> CP */
   DS_CMD_EVENT_INDICATION,               /* C66x -> CP */
   DS_CMD_CREATE_SESSION,                 /* CP -> C66x */
   DS_CMD_MODIFY_SESSION,                 /* CP -> C66x */
   DS_CMD_DEL_SESSION,                    /* CP -> C66x */
   DS_CMD_CONFIGURATION_ACK,              /* C66x -> CP */
   DS_CMD_CREATE_SESSION_ACK,             /* C66x -> CP */
   DS_CMD_MODIFY_SESSION_ACK,             /* C66x -> CP */
   DS_CMD_DELETE_SESSION_ACK,             /* C66x -> CP */
   DS_CMD_CORE_STATS_QUERY,               /* CP -> C66x */
   DS_CMD_CORE_STATS_RSP,                 /* C66x -> CP */
   DS_CMD_CHANNEL_STATS_QUERY,            /* CP -> C66x */
   DS_CMD_CHANNEL_STATS_RSP,              /* C66x -> CP */
   DS_CMD_DTMF_TONE_GENERATION,           /* CP -> C66x */
   DS_CMD_DTMF_TONE_GENERATION_ACK,       /* C66x -> CP */
   DS_CMD_VAU_NOISE_DETECTION,            /* C66x -> CP */
   DS_CMD_SESSION_TIMEOUT_NOTIFICATION,   /* C66x -> CP */
   DS_CMD_SET_HA_STATE,                   /* CP -> C66x */
   DS_CMD_SET_HA_STATE_ACK,               /* C66x -> CP */
   DS_CMD_REPLICATION,                    /* CP -> C66x for standby */
   DS_CMD_REPLICATION_NOTIFICATION,       /* C66x -> CP for active */
   DS_CMD_CONFIGURE_IP,                   /* CP -> C66x */
   DS_CMD_CONFIGURE_IP_ACK,               /* C66x -> CP */
   DS_CMD_LOG_QUERY,                      /* CP -> C66x */
   DS_CMD_LOG_RSP,                        /* C66x -> CP */
   DS_CMD_GO_ACTIVE,                      /* CP -> C66x */
   DS_CMD_GO_ACTIVE_ACK,                  /* C66x -> CP */
   DS_CMD_TIME_SYNC,                      /* CP -> C66x */
   DS_CMD_TIME_SYNC_ACK,                  /* C66x -> CP */

   DS_CMD_MAX

};

enum ack_type {

   DS_ACK_NONE,
   DS_ACK_OK,
   DS_ACK_UNRECOGNIZED_CMD,
   DS_ACK_UNRECOGNIZED_ID,
   DS_ACK_CMD_INTEGRITY_ERR,
   DS_ACK_SESSION_FULL,
   DS_ACK_CHANNEL_FULL,
   DS_ACK_INVALID_IP_TYPE,
   DS_ACK_CHANNEL_NOT_FOUND,
   DS_ACK_SESSION_NOT_FOUND,
   DS_ACK_DUPLICATE_CHANNEL,
   DS_ACK_UNEXPECTED_COMMAND,
   DS_ACK_INVALID_SESSION_DATA,
   DS_ACK_INVALID_TERM_INFO,
   DS_ACK_CHANNEL_INITIALZATION_FAILED,
   DS_ACK_SESSION_INDEX_FAILURE,
   DS_ACK_HIGH_CPU_LOAD,
   DS_ACK_DUPLICATE_SESSION,
   DS_ACK_INVALID_IP_CONFIG,
#ifndef USE_ATCA_SESSION_CMD_MODS
   DS_ACK_INVALID_TIME_STAMP,                  /* NTP TimeStamp sync */
#endif
   DS_ACK_CHANNEL_INDEX_FAILURE,
#ifdef _USE_N1_REDUNDANCY_
   DS_ACK_INVALID_STANDBY_ID,
#endif
   DS_ACK_CHANNEL_IN_USE
};

enum ha_state
{
   DS_STATE_STANDBY_ONE_PLUS_ONE,
   DS_STATE_ACTIVE,
   DS_STATE_STANDBY_N_PLUS_ONE
};

struct cmd_hdr {

#ifdef __BIG_ENDIAN__
   uint32_t len : 16;      /* data length */
   uint32_t type : 16;
#else
   uint32_t type : 16;
   uint32_t len : 16;      /* data length */
#endif
 /* char data[0];         note that data immediately follows a command header in actual use */
};

/* always sent to core 0 */
struct cmd_configuration {

   uint32_t trans_id;
   GLOBAL_CONFIG gf;
};

struct cmd_create_session {

   uint32_t priv;           /* user application transparent info */
   SESSION_DATA session_data;
};

struct cmd_modify_session {

   uint32_t session_id;

   /* ha_index = 0 (ACTIVE or 1+1 case),  = X = (1..N) (Standby for Active X) */
   uint32_t ha_index;

   TERMINATION_INFO new_term;
};

struct cmd_del_session {

   uint32_t session_id;

   /* ha_index = 0 (ACTIVE or 1+1 case),  = X = (1..N) (Standby for Active X) */
   uint32_t ha_index;
};

struct cmd_configuration_ack {

   uint32_t cause_code;  /* see ack_type enums above */
};

struct cmd_create_session_ack {

   uint32_t cause_code;  /* see ack_type enums above */
};

struct cmd_modify_session_ack {

   uint32_t cause_code;  /* see ack_type enums above */
};

struct cmd_del_session_ack {

   uint32_t cause_code;  /* see ack_type enums above */
#ifdef __BIG_ENDIAN__
   uint8_t reserved;
   union
   {
      uint8_t audio;
      uint8_t video;
   } codec_type[2];
   uint8_t _media_type;  /* codec type are used for MIPs adjustment */
#else
   uint8_t _media_type;  /* codec type are used for MIPs adjustment */
   union
   {
      uint8_t audio;
      uint8_t video;
   } codec_type[2];
   uint8_t reserved;
#endif
};

struct cmd_ovld_notification {

  uint32_t Reserved;
   /* TBD */
};

struct cmd_core_stats_rsp {

#ifdef __BIG_ENDIAN__
   uint32_t cpu_usage_avg : 16;
   uint32_t cpu_usage_peak : 16;  /* usages are in percent */
#ifdef USE_ATCA_SESSION_CMD_MODS
/* no longer used, replaced by separate mem area stats */
   uint32_t mem_usage_avg : 16;
   uint32_t mem_usage_peak : 16;
#endif
   uint32_t onchip_heap_avg : 16;
   uint32_t onchip_heap_peak : 16;
   uint32_t extern_heap_avg : 16;
   uint32_t extern_heap_peak : 16;
   uint32_t extern_heapBuf_avg : 16;
   uint32_t extern_heapBuf_peak : 16;
#else
   uint32_t cpu_usage_peak : 16;  /* usages are in percent */
   uint32_t cpu_usage_avg : 16;
#ifdef USE_ATCA_SESSION_CMD_MODS
/* no longer used, replaced by separate mem area stats */
   uint32_t mem_usage_peak : 16;
   uint32_t mem_usage_avg : 16;
#endif
   uint32_t onchip_heap_peak : 16;
   uint32_t onchip_heap_avg : 16;
   uint32_t extern_heap_peak : 16;
   uint32_t extern_heap_avg : 16;
   uint32_t extern_heapBuf_peak : 16;
   uint32_t extern_heapBuf_avg : 16;
#endif
   uint64_t rx_pkts;
   uint64_t tx_pkts;
   uint64_t drop_pkts;
   uint64_t rx_octs;
   uint64_t tx_octs;
};

struct cmd_channel_stats_req {

   struct ip_addr remote_ip;
   struct ip_addr local_ip;
#ifdef __BIG_ENDIAN__
   uint32_t local_port : 16;
   uint32_t remote_port : 16;
#else
   uint32_t remote_port : 16;
   uint32_t local_port : 16;
#endif
};

struct cmd_channel_stats_rsp {
   uint32_t term_id;
   uint64_t rx_pkts;
   uint64_t tx_pkts;
   uint64_t drop_pkts;
   uint64_t rx_octs;
   uint64_t tx_octs;
   uint32_t rx_inter_arrival_time_min;
   uint32_t rx_inter_arrival_time_max;
   uint32_t rx_avg_jitter;
   uint32_t codec_mode_change_ue_init;
   uint32_t codec_mode_change_uag_init;
};

struct cmd_log_stats_rsp
{
    uint32_t next_log_idx;
    uint32_t log_wrap_flag;
    uint32_t log_buf_address;
    uint32_t avg_polling_time;
};

struct cmd_heartbeat_notification {

   uint32_t time_stamp;
#ifdef __BIG_ENDIAN__
   uint32_t reserved : 16;
   uint32_t cpu_usage_avg : 16;
#else
   uint32_t cpu_usage_avg : 16;
   uint32_t reserved : 16;
#endif
};

struct cmd_dtmf_tone_generation {

   uint32_t session_id;

   struct ip_addr remote_ip;
   struct ip_addr local_ip;
#ifdef __BIG_ENDIAN__
   uint32_t local_port : 16;
   uint32_t remote_port : 16;
#else
   uint32_t remote_port : 16;
   uint32_t local_port : 16;
#endif
   
   uint32_t padding_before_tone; /* silence padding before generated tone, duration of silence in msec */
   uint32_t padding_after_tone; /* silence padding after generated tone, duration of silence in msec */
   uint32_t tone_timestamp; /* timestamp field of rtp header */
   struct dtmf_event dtmf;
};

struct cmd_dtmf_tone_generation_ack {

   uint32_t cause_code;  /* see ack_type enums above */
};

struct cmd_vau_noise_detection {

   uint32_t session_id;
   uint32_t priv;           /* user application transparent info */
};

struct cmd_session_timeout_notification {

   uint32_t session_id;
   uint32_t priv;           /* user application transparent info */
};

struct cmd_ha_state {
   uint32_t state;      /* see ha_state */
};

struct cmd_ha_state_ack {
   uint32_t cause_code;  /* see ack_type enums above */
};

struct cmd_replication {
   uint32_t buffer_index;      /* either 0 or 1 */
};

struct cmd_replication_notification {

   uint32_t buffer_index;
};

struct cmd_configure_ip {

#ifdef __BIG_ENDIAN__
   uint32_t reserved : 24;
   uint32_t flag : 8;  /* bit 0  = (SET/UNSET IP, 0 = unset, 1 = set),
                          bit 1  = (PHYSICAL IP SELECT, 0 = invalid, 1 = valid),
                          bit 2  = (VIRTUAL IP SELECT, 0 = invalid, 1 = valid) */
#else
   uint32_t flag : 8;  /* bit 0  = (SET/UNSET IP, 0 = unset, 1 = set),
                          bit 1  = (PHYSICAL IP SELECT, 0 = invalid, 1 = valid),
                          bit 2  = (VIRTUAL IP SELECT, 0 = invalid, 1 = valid) */
   uint32_t reserved : 24;
#endif
   struct ip_addr physical_ip;
   struct ip_addr virtual_ip;
   struct ip_addr subnet_mask; /* field only valid for virtual ip */
   struct ip_addr gateway;     /* field only valid for virtual ip */
};

struct cmd_configure_ip_ack {

   uint32_t cause_code;
};


struct cmd_go_active {

   /* ha_index = X = (1..N) (Become Active X) */
   uint32_t ha_index;
};

struct cmd_go_active_ack {

   uint32_t cause_code;
};

struct cmd_NTP_time_sync {

   unsigned long long NTP;
   /* uint32_t  seconds;           // seconds since epoch */
   /* uint32_t  second_fraction;   // seconds fractional value */
   uint64_t  TSC;                /*  TSC value read by user app as close as possible to NTP timestamp time */
};

struct cmd_NTP_time_sync_ack {

   uint32_t cause_code;
};

#ifdef __cplusplus
}
#endif

#endif  /* _SESSION_CMD_H */

