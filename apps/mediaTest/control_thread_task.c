/*
 $Header: /root/Signalogic/DirectCore/apps/SigC641x_C667x/mediaTest/control_thread_task.c

 Description:
 
   Function for handling C66x->host mailbox ran in a separate thread from main application
 
 Copyright (C) Signalogic Inc. 2015-2019
 
 Revision History:
 
   Created, CJ, September 2015
*/

#include <arpa/inet.h>

#include "mediaTest.h"

volatile debug_counters_t debug_counters = {0};
static struct cmd_core_stats_rsp core_stats = {0};
volatile probes_t probes = {-2U};
volatile packet_stats_t packet_stats = {0};
static uint8_t session_status[MAX_SESSIONS_PER_CORE] = {0};
static SESSION_DATA session = {0};

static DWORD debug_counters_addr, core_stats_addr, log_buffer_addr, probes_addr, packet_stats_addr, session_status_addr, session_data_addr;

extern volatile unsigned long valid_pkt_recv_count, pkt_recv_count;

static void get_and_display_stats(int node_id)
{
   if(debug_counters_addr == 0 || core_stats_addr == 0)
   {
      printf("Unable to find symbol address. debug_counters = 0x%x, core_stats = 0x%x\n", debug_counters_addr, core_stats_addr);
      
      printf("\n");
      return;
   }
   
   DSReadMem(hCard, DS_GM_LINEAR_DATA, 0x10000000 + debug_counters_addr + 0x01000000 * (node_id%8), DS_GM_SIZE32, (void *)&debug_counters, sizeof(debug_counters)/4);
   DSReadMem(hCard, DS_GM_LINEAR_DATA, 0x10000000 + core_stats_addr + 0x01000000 * (node_id%8), DS_GM_SIZE32, &core_stats, sizeof(core_stats)/4);
   
   printf("#Chip %d, Core %d\n", node_id/8, node_id%8);
   printf("# General statistics:\n");
   printf("   proc_pkt_cnt = %d\n", debug_counters.proc_pkt);
   printf("   voice_pkt_cnt = %d\n", debug_counters.voice_pkt);
   printf("   rtp_event_pkt_cnt = %d\n", debug_counters.rtp_event_pkt);
   printf("   dp_dsp_xfer_cnt = %d\n", debug_counters.dp_dsp_xfer);
   printf("   dsp_dsp_xfer_cnt = %d\n", debug_counters.dsp_dp_xfer);
   printf("   rtp_event_insertion = %d\n", debug_counters.rtp_event_insertion);

   printf("   Cmd Rcvd: conf = %d, create = %d, modify = %d, delete = %d\n"
          "             core_stat_query = %d, chan_stat_query = %d\n"
          "             dtmf_tone_gen = %d, set_ha_state = %d, repl = %d\n"
          "             ip_config = %d \n",
          debug_counters.Command_Receive[DS_CMD_CONFIGURATION], debug_counters.Command_Receive[DS_CMD_CREATE_SESSION],
          debug_counters.Command_Receive[DS_CMD_MODIFY_SESSION], debug_counters.Command_Receive[DS_CMD_DEL_SESSION],
          debug_counters.Command_Receive[DS_CMD_CORE_STATS_QUERY], debug_counters.Command_Receive[DS_CMD_CHANNEL_STATS_QUERY],
          debug_counters.Command_Receive[DS_CMD_DTMF_TONE_GENERATION], debug_counters.Command_Receive[DS_CMD_SET_HA_STATE],
          debug_counters.Command_Receive[DS_CMD_REPLICATION], debug_counters.Command_Receive[DS_CMD_CONFIGURE_IP]);

   printf("   Cmd Cmpl: conf = %d, create = %d, modify = %d, delete = %d\n"
          "             core_stat_query = %d, chan_stat_query = %d\n"
          "             dtmf_tone_gen = %d, set_ha_state = %d, repl = %d\n"
          "             ip_config = %d\n",
          debug_counters.Command_Complete[DS_CMD_CONFIGURATION], debug_counters.Command_Complete[DS_CMD_CREATE_SESSION],
          debug_counters.Command_Complete[DS_CMD_MODIFY_SESSION], debug_counters.Command_Complete[DS_CMD_DEL_SESSION],
          debug_counters.Command_Complete[DS_CMD_CORE_STATS_QUERY], debug_counters.Command_Complete[DS_CMD_CHANNEL_STATS_QUERY],
          debug_counters.Command_Complete[DS_CMD_DTMF_TONE_GENERATION], debug_counters.Command_Complete[DS_CMD_SET_HA_STATE],
          debug_counters.Command_Complete[DS_CMD_REPLICATION], debug_counters.Command_Complete[DS_CMD_CONFIGURE_IP]);
          
   printf("   mailbox_read_err = %d\n", debug_counters.mailbox_read_err);
   printf("   mailbox_write_err = %d\n", debug_counters.mailbox_write_err);
  
   printf("   multi_pkt_cnt = %d\n", debug_counters.multi_pkt);
   printf("   packetize_count = %d\n", debug_counters.packetize);
  
   printf("   duplicated_count = %d\n", debug_counters.duplicated);
   printf("   pastDue_drop_cnt = %d\n", debug_counters.pastDue);
  
   printf("   one_ms_overrun = %d\n", debug_counters.one_ms_overrun);
   printf("   five_ms_overrun = %d\n", debug_counters.five_ms_overrun);
  
   printf("\n# Core statistics:\n");
   //printf("cpu_usage_peak = %d, cpu_usage_avg = %d, mem_usage_peak = %d, mem_usage_avg = %d\n"
   printf("cpu_usage_peak = %d, cpu_usage_avg = %d, onchip_heap_peak = %d, onchip_heap_avg = %d\n"
          "extern_heap_peak = %d, extern_heap_avg = %d, extern_heapBuf_peak = %d, extern_heapBuf_avg = %d\n"
          "rx_pkts = %ld, tx_pkts = %ld, drop_pkts = %ld, rx_octs = %ld, tx_octs = %ld\n",
          core_stats.cpu_usage_peak, core_stats.cpu_usage_avg, 
          //core_stats.mem_usage_peak, core_stats.mem_usage_avg, 
          core_stats.onchip_heap_peak, core_stats.onchip_heap_avg, core_stats.extern_heap_peak, 
          core_stats.extern_heap_avg, core_stats.extern_heapBuf_peak, core_stats.extern_heapBuf_avg,
          core_stats.rx_pkts, core_stats.tx_pkts, core_stats.drop_pkts,
          core_stats.rx_octs, core_stats.tx_octs);
          
   printf("\n# Host statistics:\n");
   printf("total packet count = %ld\n", pkt_recv_count);
   printf("valid packet count = %ld\n", valid_pkt_recv_count);
          
   printf("\n");
}

static void get_and_display_probes(int node_id)
{
   if(probes_addr == 0)
   {
      printf("Unable to find symbol address for probes\n");
      
      printf("\n");
      return;
   }
   
   DSReadMem(hCard, DS_GM_LINEAR_DATA, 0x10000000 + probes_addr + 0x01000000 * (node_id%8), DS_GM_SIZE32, (void *)&probes, sizeof(probes)/4);
   
   printf("# Chip %d, Core %d\n", node_id/8, node_id%8);
   printf("# Probes:\n");
   printf("   main = 0x%08x\n", probes.main);
   printf("   swi = %d\n", probes.swi);
   printf("   pkt = %d\n", probes.pkt);
   printf("   proc = %d\n", probes.proc);
   printf("   mailbox = %d\n", probes.mailbox);
   
   printf("\n");
}

static void get_and_display_packet_stats(void)
{
   if(packet_stats_addr == 0)
   {
      printf("Unable to find symbol address for packet_stats\n");
      
      printf("\n");
      return;
   }
   
   DSReadMem(hCard, DS_GM_LINEAR_DATA, packet_stats_addr, DS_GM_SIZE32, (void *)&packet_stats, sizeof(packet_stats)/4);
   
   printf("# Packet Stats:\n");
   printf("   rtp_receive_counter = %d\n", packet_stats.rtp_receive_counter);
   printf("   arp_receive_counter = %d\n", packet_stats.arp_receive_counter);
   printf("   icmp_receive_counter = %d\n", packet_stats.icmp_receive_counter);
   printf("   icmp_ping_receive_counter = %d\n", packet_stats.icmp_ping_receive_counter);
   printf("   ipv4_receive_counter = %d\n", packet_stats.ipv4_receive_counter);
   printf("   packet_receive_counter = %d\n", packet_stats.packet_receive_counter);
   printf("   packet_send_counter = %d\n", packet_stats.packet_send_counter);
   
   printf("\n");
}

static void convert_ip_addr(struct ip_addr *addr, char* buffer, int max_length)
{
   if (addr->type == DS_IPV4)
   {
      struct sockaddr_in sa;
      sa.sin_addr.s_addr = addr->u.ipv4;
      inet_ntop(AF_INET, &sa.sin_addr, buffer, max_length);
   }
   else
   {
      struct sockaddr_in6 sa;
      memcpy(sa.sin6_addr.s6_addr, addr->u.ipv6, DS_IPV6_ADDR_LEN);
      inet_ntop(AF_INET6, &sa.sin6_addr, buffer, max_length);
   }
}

static void print_codec_flags(TERMINATION_INFO *term)
{
   if ((term->codec_type == DS_VOICE_CODEC_TYPE_AMR_NB) ||
      (term->codec_type == DS_VOICE_CODEC_TYPE_AMR_WB))
   {
      printf("    channels = %d, octet-align = %d, crc = %d, robust-sorting = %d, interleaving = %d\n"
            "    mode-change-period = %d, mode-change-capability = %d, mode-change-neighbor = %d\n",
            term->attr.voice_attr.u.amr.codec_flags & DS_AMR_CHANNELS,
            (term->attr.voice_attr.u.amr.codec_flags & DS_AMR_OCTET_ALIGN) ? 1: 0,
            (term->attr.voice_attr.u.amr.codec_flags & DS_AMR_CRC) ? 1: 0,
            (term->attr.voice_attr.u.amr.codec_flags & DS_AMR_ROBUST_SORTING) ? 1 :0,
            (term->attr.voice_attr.u.amr.codec_flags & DS_AMR_INTERLEAVING) ? 1 : 0,
            (term->attr.voice_attr.u.amr.codec_flags & DS_AMR_MODE_CHANGE_PERIOD) ? 2 : 1,
            (term->attr.voice_attr.u.amr.codec_flags & DS_AMR_MODE_CHANGE_CAP) ? 2 : 1,
            (term->attr.voice_attr.u.amr.codec_flags & DS_AMR_MODE_CHANGE_NEIGH) ? 1 : 0);
   }
   else if ((term->codec_type == DS_VOICE_CODEC_TYPE_EVRC) ||
      (term->codec_type == DS_VOICE_CODEC_TYPE_EVRCB) ||
      (term->codec_type == DS_VOICE_CODEC_TYPE_EVRC_NW))
   {
      const char *pktFormatStr[] = { "interleaving/bundled", "header free", "compact bundled" };
      int pktFormat = (term->attr.voice_attr.u.evrc.codec_flags & DS_EVRC_PACKET_FORMAT) >> DS_EVRC_PACKET_FORMAT_SHIFT;

      printf("    frame_size = %s, packet_format = %s, bitrate = %d, mode = %d\n"
            "    interleave = %d, noise_supp = %s, dtxmin = %d, dtxmax = %d, hangover = %d\n",
            (term->attr.voice_attr.u.evrc.codec_flags & DS_EVRC_FRAME_SIZE) ? "16khz" : "8khz",
            pktFormatStr[pktFormat],
            (term->attr.voice_attr.u.evrc.codec_flags & DS_EVRC_BITRATE) >> DS_EVRC_BITRATE_SHIFT,
            (term->attr.voice_attr.u.evrc.codec_flags & DS_EVRC_MODE) >> DS_EVRC_MODE_SHIFT,
            (term->attr.voice_attr.u.evrc.codec_flags & DS_EVRC_MAX_INTERLEAVE) >> DS_EVRC_MAX_INTERLEAVE_SHIFT,
            (term->attr.voice_attr.u.evrc.codec_flags & DS_EVRC_NOISE_SUPP) ? "enabled" : "disabled",
            term->attr.voice_attr.u.evrc.dtxmin,
            term->attr.voice_attr.u.evrc.dtxmax,
            term->attr.voice_attr.u.evrc.hangover);
   }
   else if (term->codec_type == DS_VOICE_CODEC_TYPE_OPUS)
   {
      printf(" %s, %s, %s, max_avg_bitrate = %d, \n"
            "max_playback_rate = %d, sprop_max_capture_rate = %d\n",
            (term->attr.voice_attr.u.opus.codec_flags & DS_OPUS_STEREO) ? "stereo" : "mono",
            (term->attr.voice_attr.u.opus.codec_flags & DS_OPUS_CBR) ? "CBR" : "VBR",
            (term->attr.voice_attr.u.opus.codec_flags & DS_OPUS_FEC) ? "FEC enabled" : "FEC disabled",
            term->attr.voice_attr.u.opus.codec_flags & DS_OPUS_MAX_AVG_BITRATE,
            term->attr.voice_attr.u.opus.max_playback_rate,
            term->attr.voice_attr.u.opus.sprop_max_capture_rate);
   }
}

static const char* getCodecName(uint8_t codecType)
{
   const char *codecName[] =
   {  "NONE", "G711_U", "G711_A", "G711_WB_U", "G711_WB_A", "G726", "G729AB", "G723", "AMR_NB",
      "AMR_WB", "EVRC", "ILBC", "ISAC", "OPUS", "EVRCB", "GSMFR", "GSMHR","GSMEFR", "G722", "EVRC_NW", "CLEARMODE","EVS",
      "INVALID" 
   };

   if (codecType < DS_VOICE_CODEC_TYPE_INVALID)
      return codecName[codecType];
   else
      return "INVALID";
}

static void print_term_data(TERMINATION_INFO *term)
{
   char local_addr[128], remote_addr[128];
   printf("Termination %d:\n", term->term_id);
   printf("    media_type = %d, codec_type = %s, vqe_processing_interval = %d, bit_rate = %d\n",
         term->media_type, getCodecName(term->codec_type),
         term->vqe_processing_interval, term->bitrate);
   
   convert_ip_addr(&term->remote_ip, remote_addr, 128);
   convert_ip_addr(&term->local_ip, local_addr, 128);
   
   printf("    remote = %s:%d, local = %s:%d\n",
      remote_addr, ntohs(term->remote_port), local_addr, ntohs(term->local_port));
   
   printf("    ec_tail_len = %d, ec = %d, noise_reduction = %d, VAD = %s, CNG = %s\n",
         term->attr.voice_attr.ec_tail_len, term->attr.voice_attr.ec,
         term->attr.voice_attr.noise_reduction,
         (term->attr.voice_attr.flag & VOICE_ATTR_FLAG_VAD) ? "enabled" : "disabled",
         (term->attr.voice_attr.flag & VOICE_ATTR_FLAG_CNG) ? "enabled" : "disabled");
   printf("    ptime = %d, rtp_payload_type = %d, dtmf = %d, dtmf_payload_type = %d\n",
         term->attr.voice_attr.ptime, term->attr.voice_attr.rtp_payload_type,
         term->attr.voice_attr.dtmf_mode, term->attr.voice_attr.dtmf_payload_type);
   print_codec_flags(term);
}

static void print_session_data(SESSION_DATA *session)
{

   printf("session_id = %d\n", session->session_id);
   print_term_data(&session->term1);
   print_term_data(&session->term2);
   printf("\n");
}

static void get_and_display_session_data(int node_id)
{
   int i, count = 0, core_id = node_id % 8;
   
   if(session_status_addr == 0)
   {
      printf("Unable to find symbol address for session_status\n");
      
      printf("\n");
      return;
   }
   
   if(session_data_addr == 0)
   {
      printf("Unable to find symbol address for session_data\n");
      
      printf("\n");
      return;
   }
   
   DSReadMem(hCard, DS_GM_LINEAR_DATA, session_status_addr + (core_id * MAX_SESSIONS_PER_CORE), DS_GM_SIZE32, (void *)&session_status, sizeof(session_status)/4);
   
   for (i = 0; i < MAX_SESSIONS_PER_CORE; i++)
   {     
      if (session_status[i])
      {
         count++;
         DSReadMem(hCard, DS_GM_LINEAR_DATA, session_data_addr + (core_id * MAX_SESSIONS_PER_CORE + i) * sizeof(SESSION_DATA), DS_GM_SIZE32, (void *)&session, sizeof(session)/4);
         print_session_data(&session);
      }
   }

   if (count == 0)
      printf("No session data for chip id %d, core id  %d\n", node_id, core_id);
}

//#define MAILBOX_TO_CONSOLE
#define MAILBOX_TO_FILE
FILE *mailbox_out, *mailbox_nacks;
static void mailbox_print(char *msg, char flag)
{
   #ifdef MAILBOX_TO_CONSOLE
   printf("%s", msg);
   #endif
   #ifdef MAILBOX_TO_FILE
   if (flag)
      fprintf(mailbox_nacks, "%s", msg);
   else
      fprintf(mailbox_out, "%s", msg);
   #endif
   
   memset(msg, 0, strlen(msg));
}

void *control_thread_task(void *arg)
{
   int i;
   unsigned int size, trans_id;
   int ret_val = 0;
   char mailbox_message[2048] = {0};
   
   mailbox_out = fopen("mailBox_log.txt", "w");
   mailbox_nacks = fopen("mailBox_nacks.txt", "w");
   
   uint8_t rx_buffer[TRANS_MAILBOX_MAX_PAYLOAD_SIZE];
  
   struct cmd_hdr header_in;
   struct cmd_heartbeat_notification heartbeat_notification;
   struct cmd_create_session_ack c_ack;
   struct cmd_del_session_ack d_ack;
   struct dspcmd_event_channel event_indication;
   struct dtmf_event dtmf;
   int message_count = 0, create_sess_ack_cnt = 0, delete_sess_ack_cnt = 0, dtmf_gen_ack_cnt = 0;
   
   QWORD nCoreList_temp;
   
   pthread_mutex_t *mx = (pthread_mutex_t *)arg;
   
   char key;
   
   debug_counters_addr = DSGetSymbolAddr(hCard, NULL, "debug_counters");
   core_stats_addr = DSGetSymbolAddr(hCard, NULL, "core_stats");
   log_buffer_addr = DSGetSymbolAddr(hCard, NULL, "log_buffer");
   probes_addr = DSGetSymbolAddr(hCard, NULL, "probes");
   packet_stats_addr = DSGetSymbolAddr(hCard, NULL, "packet_stats");
   session_status_addr = DSGetSymbolAddr(hCard, NULL, "session_status");
   session_data_addr = DSGetSymbolAddr(hCard, NULL, "session_data");
   
   while (!needQuit(mx)) 
   {
      key = toupper(getkey());
      if (key == 'Q')
      {
         printf("\r'q' pressed, exiting test\n");
         pthread_mutex_unlock(mx);
         break;
      }
      else if (key == 'K')
      {
         printf("\r");
         for (i = 0, nCoreList_temp = 1/*nCoreList*/; nCoreList_temp > 0; i++, nCoreList_temp >>= 1)
            if(nCoreList_temp & 1)
               get_and_display_stats(i);
      }
      else if (key == 'L')
      {
         printf("\rLog saved to log.txt\n");
         DSSaveDataFile(hCard, NULL, "log.txt", log_buffer_addr, 0x800000, (uint32_t)NULL, NULL);
      }
      else if (key == 'P')
      {
         printf("\r");
         for (i = 0, nCoreList_temp = 1/*nCoreList*/; nCoreList_temp > 0; i++, nCoreList_temp >>= 1)
            if(nCoreList_temp & 1)
               get_and_display_probes(i);
      }
      else if (key == 'N')
      {
         printf("\r");
         get_and_display_packet_stats();
      }
      else if (key == 'S')
      {
         printf("\r");
         for (i = 0, nCoreList_temp = 1/*nCoreList*/; nCoreList_temp > 0; i++, nCoreList_temp >>= 1)
            if(nCoreList_temp & 1)
               get_and_display_session_data(i);
      }
      
      // receive from mailbox
      for (i = 0, nCoreList_temp = nCoreList; nCoreList_temp > 0; i++, nCoreList_temp >>= 1)
      {
         if (nCoreList & (1 << i))
         {
            ret_val = query_mb(i);
            if (ret_val < 0)
            {
               sprintf(mailbox_message, "mailBox_query error: %d\n", ret_val);
               mailbox_print(mailbox_message, 0);
               continue;
            }
            while(ret_val-- > 0)
            {
               ret_val = read_mb(i, rx_buffer, &size, &trans_id);
               if (ret_val < 0)
               {
                  sprintf(mailbox_message, "mailBox_read error: %d\n", ret_val);
                  mailbox_print(mailbox_message, 0);
                  continue;
               }
               
               memcpy(&header_in, rx_buffer, sizeof(struct cmd_hdr));
               if (header_in.type == DS_CMD_HEARTBEAT)
               {
                  memcpy(&heartbeat_notification, (void *)((uintptr_t)rx_buffer+sizeof(struct cmd_hdr)), header_in.len);
                  //sprintf(mailbox_message, "heartbeat: %d\n", heartbeat_notification.time_stamp);
                  ///mailbox_print(mailbox_message, 0);
               }
               else if (header_in.type != DS_CMD_HEARTBEAT && header_in.type != DS_CMD_REPLICATION_NOTIFICATION)
               {
                  message_count++;
                  sprintf(mailbox_message, "***** message received from node %d, message count = %d *****\n", i, message_count);
                  mailbox_print(mailbox_message, 0);
                  sprintf(mailbox_message, "\tHeader type = %d\n", header_in.type);
                  mailbox_print(mailbox_message, 0);
                  
                  if (header_in.type == DS_CMD_CREATE_SESSION_ACK)
                  {
                     create_sess_ack_cnt++;
                     sprintf(mailbox_message, "\tcreate session ack received, count = %d\n", create_sess_ack_cnt);
                     mailbox_print(mailbox_message, 0);
                     memcpy(&c_ack, (void *)((uintptr_t)rx_buffer+sizeof(struct cmd_hdr)), header_in.len);
                     sprintf(mailbox_message, "\tcause code = %d\n", c_ack.cause_code);
                     mailbox_print(mailbox_message, 0);
                     if(c_ack.cause_code != 1) 
                     {
                        sprintf(mailbox_message, "create nack: cause code = %d\n", c_ack.cause_code);
                        mailbox_print(mailbox_message, 1);
                     }
                  }
                  else if (header_in.type == DS_CMD_DELETE_SESSION_ACK)
                  {
                     delete_sess_ack_cnt++;
                     sprintf(mailbox_message, "\tdelete session ack received, count = %d\n", delete_sess_ack_cnt);
                     mailbox_print(mailbox_message, 0);
                     memcpy(&d_ack, (void *)((uintptr_t)rx_buffer+sizeof(struct cmd_hdr)), header_in.len);
                     sprintf(mailbox_message, "\tcause code = %d\n", d_ack.cause_code);
                     mailbox_print(mailbox_message, 0);
                     if(d_ack.cause_code != 1) 
                     {
                        sprintf(mailbox_message, "delete nack: cause code = %d\n", d_ack.cause_code);
                        mailbox_print(mailbox_message, 1);
                     }
                  }
                  else if (header_in.type == DS_CMD_EVENT_INDICATION)
                  {
                     memcpy(&event_indication, (void *)((uintptr_t)rx_buffer+sizeof(struct cmd_hdr)), header_in.len);
                     dtmf = event_indication.dtmf;
                     sprintf(mailbox_message, "\tReceived DTMF tone: ID: %d, Duration: %d, Volume: %d\n", dtmf.event, dtmf.duration, dtmf.volume | 0xffffffc0);
                     mailbox_print(mailbox_message, 0);
                  }
                  else if (header_in.type == DS_CMD_DTMF_TONE_GENERATION_ACK)
                  {
                     dtmf_gen_ack_cnt++;
                     sprintf(mailbox_message, "\tdtmf generation ack received, count = %d\n", dtmf_gen_ack_cnt);
                     mailbox_print(mailbox_message, 0);
                     memcpy(&d_ack, (void *)((uintptr_t)rx_buffer+sizeof(struct cmd_hdr)), header_in.len);
                     sprintf(mailbox_message, "\tcause code = %d\n", d_ack.cause_code);
                     mailbox_print(mailbox_message, 0);
                  }
               }
            }
         }
      }
   }
   
   fclose(mailbox_out);
   fclose(mailbox_nacks);

   return NULL;  /* added to avoid -Wall compiler warning, JHB Jul 2019 */
}
