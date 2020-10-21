/*
  debug.h

  Header file that contains debug structs and function prototypes

  Copyright © Signalogic, 2015-2017

  Revision History:

    Created Aug 2014, CKJ
    Modified Apr 2016, JHB modify Dsp_Log function so it can appear as a null macro and be completely compiled out of code if needed
    Modified Dec 2016, CKJ, heartbeat priority option
    Modified Jan 2017, CKJ, moved logging APIs and definitions to debug_rt.h (to support consistent interface between x86, c66x, or combined platforms)
*/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef _C66XX_
  #include <xdc/runtime/System.h>
#endif

#include "session_cmd.h"

#include "debug_rt.h"

/* logging related defines */
#define LOG_SINGLE_CORE_ONLY 0
#define CORE_TO_LOG 0

/* Define priority for sending heartbeat messages:  1 = high priority, sent from swi_manager() 1 msec SWI in swi_control_task.c, 0 = low priority, sent from oam_processing_task() 100 ms SWI in oam_task.c, CKJ Dec2016 */
#define HEARTBEAT_PRIORITY 1

/* Debug/logging related structures */
typedef struct probes_s
{
   uint32_t main;
   uint32_t swi;
   uint32_t pkt;
   uint32_t proc;
   uint32_t mailbox;
} probes_t;

extern volatile probes_t probes;
#define MOVING_AVERAGE_SIZE 16
#define MOVING_AVERAGE_SIZE_5ms 50 
typedef struct cycles_s
{
   uint32_t pkt_task_max;
   uint32_t pkt_task_inst[MOVING_AVERAGE_SIZE];
#ifdef USE_ATCA_DEBUG_MODS
   uint32_t proc_task_max;
#endif
   uint32_t proc_task_inst[MOVING_AVERAGE_SIZE];
#ifndef USE_ATCA_DEBUG_MODS
   uint32_t proc_task_max;
#endif
   uint32_t mb_loop_max;
   uint32_t dp_buf_max;
   uint32_t tx_buf_max;
   uint32_t recv_memcpy_max;
   uint32_t send_memcpy_max;
   uint32_t recv_memcpy_total;
   uint32_t send_memcpy_total;
   uint32_t jb_add_max;
   uint32_t pkt_loop_max;
   uint32_t preempted_cycles_max;
   uint32_t pkt_rcv_inst;
   uint32_t pkt_rcv_max;
   uint32_t pkt_proc_inst;
   uint32_t pkt_proc_max;
   uint32_t processing_max;
   uint32_t processing_avg;
   uint32_t processing_moving_avg[MOVING_AVERAGE_SIZE_5ms]; 
   uint32_t packetization_max;
   uint32_t packetization_avg;
   uint32_t packetization_moving_avg[MOVING_AVERAGE_SIZE_5ms]; 
   uint32_t jb_get_max; 
} cycles_t; 

extern volatile cycles_t cycles;

typedef struct debug_counters_s
{
   uint32_t proc_pkt;
   uint32_t voice_pkt;
   uint32_t rtp_event_pkt;
   uint32_t packetize;
   uint32_t recv_from_jb;
   uint32_t add_to_jb;
   uint32_t pastDue;
   uint32_t duplicated;
   uint32_t jb_add_fail;
   uint32_t multi_pkt;
   uint32_t dp_dsp_xfer;
   uint32_t dsp_dp_xfer;
   uint32_t dp_dsp_buffer_processing_deferred;
   uint32_t Command_Receive[DS_CMD_MAX];
   uint32_t Command_Complete[DS_CMD_MAX];
   uint32_t mailbox_read_err;
   uint32_t mailbox_write_err;
   uint32_t hash_lookup_fail;
   uint32_t one_ms_overrun;
   uint32_t five_ms_overrun;
   uint32_t next_log_idx;
   uint32_t log_wrap_flag;
   uint32_t avg_polling_time;
   uint32_t rtp_event_insertion;
} debug_counters_t;

extern volatile debug_counters_t debug_counters;

/* UNUSEDx entries are free to be used as needed.  Each entry must have one and only one bit set */

enum debug_ERR_bitfld
{
   IPV4_CHANNEL_NOT_FOUND               =   0x00000001,
   CREATE_SESSION_CHANNELS_FULL_TERM1   =   0x00000002,
   CREATE_SESSION_INVALID_IP_TYPE_TERM1 =   0x00000004,
   CREATE_SESSION_CHANNELS_FULL_TERM2   =   0x00000008,
   CREATE_SESSION_INVALID_IP_TYPE_TERM2 =   0x00000010,
   MODIFY_SESSION_CHANNEL_NOT_FOUND     =   0x00000020,
   MODIFY_SESSION_INVALID_IP_TYPE       =   0x00000040,
   DELETE_SESSION_CHANNEL_NOT_FOUND     =   0x00000080,
   DELETE_SESSION_INVALID_IP_TYPE_TERM1 =   0x00000100,
   UNUSED0                              =   0x00000200,
   DELETE_SESSION_INVALID_IP_TYPE_TERM2 =   0x00000400,
   INVALID_MEDIA_TYPE                   =   0x00000800,
   PAYLOAD_TYPE_MISMATCH                =   0x00001000,
   RTP_VALIDATION_FAILED                =   0x00002000,
   UNUSED1                              =   0x00004000,
   DELETE_SESSION_SESSION_NOT_FOUND     =   0x00008000,
   INVALID_PACKET_LENGTH                =   0x00010000,
   UNUSED2                              =   0x00020000,
   UNUSED3                              =   0x00040000,
   UNUSED4                              =   0x00080000,
   ADD_TO_JITTER_BUFFER_FAILED          =   0x00100000,
   UNUSED5                              =   0x00200000,
   JITTER_BUFFER_BYPASS_OVERFLOW        =   0x00400000,
   UNUSED6                              =   0x00800000,
   TX_BUFFER_OVERFLOW                   =   0x01000000,
   MALFORMED_PACKET                     =   0x02000000,
   INVALID_IP_VERSION                   =   0x10000000,
   IPV6_CHANNEL_NOT_FOUND               =   0x20000000
};

#define DEBUG_ARRAY_SIZE 50

typedef struct misc_debug_s
{
   uint32_t debug_ERR;  /* use with debug_ERR_bitfld enum */
   uint32_t dp_dsp_xfer_octs;
   uint32_t dsp_dp_xfer_octs;
   uint32_t max_dp_dsp_buffer_length;
   uint32_t max_dsp_dp_buffer_length;
   uint32_t jb_add_fail_status[DEBUG_ARRAY_SIZE];
   uint32_t early_term_pkt_loop;
} misc_debug_t;

extern volatile misc_debug_t debug;

typedef struct packet_stats_s
{
   uint32_t rtp_receive_counter;
   uint32_t arp_receive_counter;
   uint32_t icmp_receive_counter;
   uint32_t icmp_ping_receive_counter;
   uint32_t ipv4_receive_counter;
   uint32_t packet_receive_counter;
   uint32_t packet_send_counter;
   uint32_t inst_packets_available;
   uint32_t max_packets_available;
   uint32_t inst_packets_processed;
   uint32_t max_packets_processed;
   uint32_t max_iterations_to_empty_pkt_buf;
} packet_stats_t;

extern volatile packet_stats_t packet_stats;

#ifdef USE_ATCA_DEBUG_MODS
/* dspDebug variables */
typedef struct rtcp_counters_s
{
   unsigned long long c66_rtp_timestamp;
   unsigned int c66_senders_cumu_pkt_count;
   unsigned int c66_senders_cumu_byte_count;
   unsigned long long c66_senders_ssrc;
   unsigned int c66_frcn_pkt_lost;
   unsigned int c66_inter_arrival_pkt_jitter;
   unsigned long long c66_last_sender_report_timestamp;
   unsigned long long c66_last_received_timestamp;
} rtcp_counters_t;

extern volatile rtcp_counters_t rtcp_counters_host;
#endif

#endif /* end of header file */
