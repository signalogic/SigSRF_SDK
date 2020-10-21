/*
 $Header: /root/Signalogic/apps/mediaTest/mediaTest.c

 Copyright (C) Signalogic Inc. 2015-2020
 
 Description:
 
   SigSRF test and measurement program for media transcoding, codecs, and packet streaming / processing

 Purposes:
 
   1) Test and measurement for SigMRF media transcoding, codec, and packet streaming / processing software for x86 and coCPU platforms
   
   2) Run demo/eval tests and verify performance, audio quality, and multiple concurrent instances for certain codecs
   
   3) User application source code examples, including Pktlib and Voplib API usage

   4) Supports optional coCPU card(s) for very high server capacity

 Example Command Lines (see ** Cmd Line Notes):

   Codec testing, raw audio input, encode, compressed bitstream file output:

     ./mediaTest -cx86 -i reference_files/stv8c.INP -o test_script_files/stv8c_5900_8kHz_dtx_sig.COD -C session_config/codec_test_8kHz_5900bps_config
     ./mediaTest -cx86 -i reference_files/stv16c.INP -o test_script_files/stv16c_9600_16kHz_sig.COD -C session_config/codec_test_16kHz_9600bps_config
     ./mediaTest -cx86 -i reference_files/stv32c.INP -o test_script_files/stv32c_13200_32kHz_sig.COD -C session_config/codec_test_32kHz_13200bps_config

   Codec testing, compressed bitstream file input, decode, raw audio output:

     ./mediaTest -cx86 -i reference_files/stv16c_7200_16kHz_mime_$SUFFIX.COD -o test_script_files/stv16c_7200_16kHz_sig_decoded.OUT -C session_config/codec_test_16kHz_7200bps_config
     ./mediaTest -cx86 -i reference_files/stv16c_13200_16kHz_mime_$SUFFIX.COD -o test_script_files/stv16c_13200_16kHz_sig_decoded.OUT -C session_config/codec_test_16kHz_13200bps_config 

   Packet mode testing, pcap input, decode, wav output:
   
     ./mediaTest -cx86 -M0 -Csession_config/evs_850211b0_config -itest_files/evs_850211b0.pcap -og711_850211b0.wav

   Packet mode testing, pcap input, transcoding, pcap output:

     ./mediaTest -M0 -cx86 -C session_config/pcap_file_test_config -i pcaps/pcmutest.pcap -i pcaps/EVS_13.2_16000.pcap
     ./mediaTest -M0 -cx86 -C session_config/pcap_file_test_config -i pcaps/pcmutest.pcap -i pcaps/EVS_13.2_16000.pcap -o stream1_xcoded.pcap -o stream2_xcoded.pcap

   x86 motherboard network I/O and packet + voice processing:

     ./mediaTest -cx86 -M0 -Csession_config/frame_test_script_config

   x86 motherboard network I/O with coCPU packet + voice processing:

     ./mediaTest -f1000 -m0xff -cSIGC66XX-8 -e C66xx_RTAF_SYSBIOS_CCSv54.out -M0 -Csession_config/test_config

   coCPU testing, raw audio input, encode, decode, wav output:

     ./mediaTest -f1000 -m0xff -cSIGC66XX-8 -ecoCPU_c66x.out -itest_files/stv8c.INP -otest_files/c6x8c_j.wav -Csession_config/codec_test_8kHz_13200bps_config

     ./mediaTest -f1000 -m0xff -cSIGC66XX-8 -ecoCPU_c66x.out -itest_files/stv16c.INP -otest_files/c6x16c_j.wav 

   ** Cmd Line Notes
 
     1) -M (mode) options

       -M0  Use packet input, either network sockets (e.g. motherboard network I/O) or pcap files.  Packets are processed using x86 or coCPUs depending on -c cmd
            line entry (internal C code reference is NETWORK_PACKET_TEST).  In coCPU card case, network I/O on the card is used

       -M1  Simulate network traffic and process simulated packets using coCPUs
            (COCPU_SIM_TEST)

       -M2  Use coCPU network interface and coCPUs for processing, user app uses PCIe for control plane only (x86 doesn't process packets)
            (COCPU_NETWORK_TEST)

       -M3  Test codec encoder, decoder, or both using low level codec APIs, and input / output using cmd line file I/O
            (CODEC_TEST)

       -M4  Test codec encoder, decoder or both using frame data and Voplib APIs, and input / output using either config files or cmd line file I/O
            (FRAME_TEST)

     2) Cmd line file I/O is specified using -i and -o followed by audio filenames in .wav, .inp, .au, or other audio format, and / or compressed bitstream filenames in .cod format

     3) -c entry notes:

         -cSIGC6XX-N specifies coCPU processing (using one or more c66x PCIe cards, see coCPU User Guide for more info)
         -cx86 specifies x86 processing
         -cSIGX86 specifies x86 processing with measurement and logging options

     4) Default config file (i.e. no -C option given) is "codec_test_config", which is set for 16 kHz, 13.2 kbps, no DTX

     5) ** NOTE -- this note is deprecated, should no longer be needed **
     
        lib folder configuration commands possibly needed console window (shell) operation, depending on installation parameters:
   
          ldconfig /install_path/Signalogic/SIG_LIBS/Voice/EVS_fixed-point/lib
          ldconfig /install_path/Signalogic/SIG_LIBS/Voice/EVS_floating-point/lib

 Revision History:
 
   Created Sep 2015 CKJ
   Modified Dec 2015 JHB, add EVS codec test support 
   Modified Feb 2016 SC, add profiling printouts
   Modified May 2016 SC, added CH (compact header) bitstream format support
   Modified May 2016 SC, added decoder only mode support
   Modified Jun 2016 JHB, fixed bug in output .wav file sampling rate
   Modified Dec 2016 CKJ - Jan 2017, retested with PCIe version of c66x code, removed NO_MAILBOX defines (determined now by run-time decision)
   Modified Jan 2017 CKJ - Feb 2017, additional x86 support
   Modified Mar 2017 CKJ - edits for codec testing
   Modified May 2017 CJ - added pcap extract mode
   Modified Aug 2018 JHB, command-line related items moved to cmd_line_interface.c
   Modified Jul 2019 JHB, removed XDAIS references (which were not used anyway).  See comments in x86_mediaTest.c
   Modified Sep 2019 JHB, change include folder for udp.h and ip.h from "linux" to "netinet" to fix -Wodr (one definition rule) warning with gcc 5.4.  Remove arpa/inet.h include (already in pktlib.h)
*/

/* system header files */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

/* Signalogic header files */

#include "test_programs.h"   /* demo program support (command line entry, etc) */
#include "keybd.h"           /* interactive key command support */

/* mediaTest definitions (also includes DirectCore header and host/coCPU shared header files) */

#include "mediaTest.h"
#include "pktlib.h"
#include "voplib.h"
#include "diaglib.h"
#include "alglib.h"
#if defined(_ALSA_INSTALLED_)  /* _ALSA_INSTALLED_ is defined in the mediaTest Makefile, which checks for ALSA /proc/asound folder */
  #include "aviolib.h"
#endif

#if 0
// Number of cores per chip
//#define CORES_PER_CHIP 8  /* replaced by nCoresPerCPU, JHB */
#endif

/* Max packet length */
#define BUFFER_LEN 1520

#define IN_PORT_START 10240
#if 0
//#define OUT_PORT_START 10240 /*unused*/
#endif

#define IP_BASE 10

/* Number of ports available to be used */
#define PORTS_PER_CORE 2048      /* num ports available to individual c66x core (dest ports) */
#define PORTS_PER_INSTANCE 400   /* num ports available to individual mediaTest instance (src ports) */

//#define CODEC_DEBUG
//#define CODEC_PROFILING

/* base port number used for udp source port of incoming packets, this is also the udp destination port for outgoing packets */
unsigned short inc_src_port_base = 0;

/* variables for counting total number of packets and number of valid packets received
 *    valid means an RTP packet that an instance expects and will process or send to coCPU to process
*/
volatile unsigned long valid_pkt_recv_count = 0, pkt_recv_count = 0;

/* Global variables and default values */

HCARD         hCard = (HCARD)NULL;             /* handle to coCPU card, if used */
QWORD         nCoreList;                       /* bitwise core list given in command line */
DWORD         numCores = 0;                    /* number of cores used for allocating per core objects */
BOOL          fcoCPUInitialized;
unsigned int  nCoresPerCPU;

static int    send_count = 0, send_length = 0;
int           numSessions = 0;                 /* number of active sessions */

extern int    send_sock_fd_ipv6;


/* local functions */

void send_packet(uint8_t *packet, uint32_t length)
{
   int send_len = -1;
   struct sockaddr_in dest;
//   struct sockaddr_in6 dest6;
   struct iphdr *ip_hdr;
   /*struct ipv6hdr *ipv6_hdr; - needs correct header file or needs the struct definition added to mediaTest.h*/
   struct udphdr *udp_hdr;
   uint32_t version = (packet[0] & 0xf0) >> 4;

   if (send_count == 0)
   {
      send_sock_fd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
      if (send_sock_fd == -1)
      {
         printf("ERROR: failed opening IPv4 TX network socket\n");
         return;
      }
      
      send_sock_fd_ipv6 = socket(PF_INET6, SOCK_RAW, IPPROTO_RAW);
      if (send_sock_fd_ipv6 == -1)
      {
         printf("ERROR: failed opening IPv6 TX network socket\n");
         return;
      }
   }
   
   if (version == 4)
   {
      ip_hdr = (struct iphdr *)packet;
      udp_hdr = (struct udphdr *)(packet + ip_hdr->ihl * 4);
      
      dest.sin_family = AF_INET;
      dest.sin_port = udp_hdr->dest;
      dest.sin_addr.s_addr = ip_hdr->daddr;
      
      send_len = sendto(send_sock_fd, packet, length, 0, (struct sockaddr *)&dest, sizeof dest);
   }
   else if (version == 6)
   {
      /*ipv6_hdr = (struct ipv6hdr *)packet;
      udp_hdr = (struct udphdr *)(packet + 40);
      
      dest6.sin6_family = AF_INET;
      dest6.sin6_port = udp_hdr->dest;
      dest6.sin6_addr = ipv6_hdr->daddr;
      
      send_len = sendto(send_sock_fd_ipv6, packet, length, 0, (struct sockaddr *)&dest, sizeof dest);*/
   }
   else
   {
      fprintf(stderr, "Invalid ip version number in send_packet(): %d\n", version);
      return;
   }
   
   if (send_len == -1)
   {
      printf("ERROR: send_packet() failed, %s\n", strerror(errno));
      return;
   }
   else if (send_len < (int)length)
   {
      printf("ERROR: failed to send %d bytes, sent %d bytes instead\n", length, send_len);
      return;
   }
   
   send_count++;
   send_length += length;
   /*printf("send_count = %d, packet length = %d, total length = %d\n",send_count, length, send_length);*/
}

static int process_transcoded_packet(unsigned char *packet, unsigned int length)
{
   unsigned int packet_length;
   struct iphdr *ip_hdr = (struct iphdr *)packet;
   struct udphdr *udp_hdr = (struct udphdr *)(packet + ip_hdr->ihl * 4);
   int term_id = (ntohs(udp_hdr->dest) - inc_src_port_base)/2;

   if ((term_id < 0) || (term_id > numSessions*2))
   {
      printf("ERROR: term id = %d\n", term_id);
      return -1;
   }

   packet_length = ntohs(ip_hdr->tot_len);
   //printf("packet_length = %d, length = %d\n", packet_length, length);
   if (packet_length > length)
   {
       printf("Bad IP packet length: packet_length = %d, length = %d\n", packet_length, length);
       return -1;
   }

   // send processed packet back to client
   //send_packet(packet, packet_length);
   if (send_sock_fd == -1)
   {
      send_sock_fd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
      if (send_sock_fd == -1)
      {
         printf("ERROR: failed opening IPv4 TX network socket\n");
         return -1;
      }
   }

   DSSendPackets((HSESSION*)&send_sock_fd, DS_SEND_PKT_SOCKET_HANDLE, packet, &packet_length, 1);

   return packet_length;
}

static void process_recv_buffer(unsigned char *buffer, int buffer_len)
{
   unsigned int packet_length;
   struct iphdr *ip_hdr = (struct iphdr *)buffer;
   struct udphdr *udp_hdr = (struct udphdr *)(buffer + ip_hdr->ihl * 4);
   int node_id;
   
   pkt_recv_count++;
   
   // filter out packets with src ports not assigned to this instance
   if (((ntohs(udp_hdr->source) - inc_src_port_base) < 0) || (ntohs(udp_hdr->source) - inc_src_port_base >= PORTS_PER_INSTANCE)) return;
   
   // get node_id (core num based on dest port)
   node_id = ((((ntohs(udp_hdr->dest) - IN_PORT_START) >= 0) && ((ntohs(udp_hdr->dest) - IN_PORT_START) < (int)nCoresPerCPU/*CORES_PER_CHIP*/*PORTS_PER_CORE)) ? (ntohs(udp_hdr->dest) - IN_PORT_START)/PORTS_PER_CORE : -1);

   // filter out packets with node ids outside of accepted range
   if (node_id < 0) return;
   
   // verify packet length
   packet_length = ntohs(ip_hdr->tot_len);
   if ((int)packet_length != buffer_len)
   {
       printf("Bad IP packet length: packet_length = %d, length = %d\n", packet_length, buffer_len);
       return;
   }
   
   // received valid packet, add to buffer
   valid_pkt_recv_count++;
   fill_pcie_buffer(buffer, buffer_len, node_id / 8, node_id % 8);
}

/* Returns 1 (true) if the mutex is unlocked, which is the
 * thread's signal to terminate. 
 */
int needQuit(pthread_mutex_t *mtx)
{
  switch(pthread_mutex_trylock(mtx)) {
    case 0: /* if we got the lock, unlock and return 1 (true) */
      pthread_mutex_unlock(mtx);
      return 1;
    case EBUSY: /* return 0 (false) if the mutex was locked */
      return 0;
  }
  return 1;
}

static void run_network_pcie_test(pthread_mutex_t *mx)
{
   int recv_len;
   unsigned char buffer[BUFFER_LEN];
   
   int recv_sock_fd = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
   
   if (recv_sock_fd == -1)
   {
      printf("ERROR: failed opening network socket\n");
      return;
   }
   
   // Make socket non-blocking
   fcntl(recv_sock_fd, F_SETFL, O_NONBLOCK);

   printf("Running host network-pcie test. Press 'q' at any time to quit.\n");
   
   while (!needQuit(mx))
   {
      recv_len = recv(recv_sock_fd, buffer, BUFFER_LEN, 0);
      if (recv_len > 0)
         process_recv_buffer(buffer, recv_len);
         
      check_for_host_to_c66x_xfer();
      check_for_c66x_to_host_xfer(process_transcoded_packet);
   }
   
   close(recv_sock_fd);
   close(send_sock_fd);
}

FILE *output_pcap, *input_pcap;
const char dum_eth_hdr[14] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x08, 0x00};

static void send_dum_buf()
{
   unsigned int node_id;
   QWORD nCoreList_tmp = 1;//nCoreList;

   static pcaprec_hdr_t pcaprec_hdr = {0, 0, 214, 214};
   struct timeval tv;
   
   static unsigned int timestamp = 0;
   static unsigned short seq_num = 0;
   
   /* update packet */
   ((unsigned short *)dummy_packet)[2] = htons(seq_num); /* increment IP ID */
   ((unsigned short *)dummy_packet)[15] = htons(seq_num); /* increment RTP sequence number */
   ((unsigned int *)dummy_packet)[8] = htonl(timestamp); /* increment RTP timestamp */
   
   for (node_id = 0; nCoreList_tmp > 0; node_id++, nCoreList_tmp >>= 1)
   {
      if (nCoreList_tmp & 1)
      {
         /* set UDP dst/src ports */
         ((unsigned short *)dummy_packet)[10] = htons(IN_PORT_START + (PORTS_PER_CORE * (node_id % 8)));
         ((unsigned short *)dummy_packet)[11] = htons(IN_PORT_START + (PORTS_PER_CORE * (node_id % 8)));
         
         /* set IP address */
         dummy_packet[19] = IP_BASE + (node_id / 8);
         
         /* add dummy packet to buffer */
         fill_pcie_buffer(dummy_packet, sizeof_dummy_packet, node_id / 8, node_id % 8);
         
         /* write packet to input pcap file */
         gettimeofday(&tv, NULL);
         pcaprec_hdr.ts_sec = tv.tv_sec;
         pcaprec_hdr.ts_usec = tv.tv_usec;
         
         fwrite(&pcaprec_hdr, sizeof(pcaprec_hdr), 1, input_pcap);
         fwrite(dum_eth_hdr, sizeof(dum_eth_hdr), 1, input_pcap);
         fwrite(dummy_packet, sizeof_dummy_packet, 1, input_pcap);
      }
   }
   
   timestamp+=160;
   seq_num++;
}

static int process_transcoded_dummy_packet(unsigned char *packet, unsigned int length)
{
   static pcaprec_hdr_t pcaprec_hdr = {0, 0, 214, 214};
   struct timeval tv;
   
   gettimeofday(&tv, NULL);
   pcaprec_hdr.ts_sec = tv.tv_sec;
   pcaprec_hdr.ts_usec = tv.tv_usec;
   
   fwrite(&pcaprec_hdr, sizeof(pcaprec_hdr), 1, output_pcap);
   fwrite(dum_eth_hdr, sizeof(dum_eth_hdr), 1, output_pcap);
   fwrite(packet, length, 1, output_pcap);
   
   return length;
}

static void run_cocpu_sim_test(pthread_mutex_t *mx)
{
   struct timeval tv1, tv2;
   pcap_hdr_t pcap_hdr = {0xa1b2c3d4, 2, 4, 0, 0, 65535, 1};
   
   printf("Running coCPU packet simulation test. Press 'q' at any time to quit.\n");
   
   /* open input and output pcaps and write file headers */
   output_pcap = fopen("sim_out.pcap", "wb");
   fwrite(&pcap_hdr, sizeof(pcap_hdr), 1, output_pcap); 
   
   input_pcap = fopen("sim_in.pcap", "wb");
   fwrite(&pcap_hdr, sizeof(pcap_hdr), 1, input_pcap);   
   
   /* get start time */
   gettimeofday(&tv1, NULL);
   
   while(!needQuit(mx))
   {
      /* fill buffer every 20ms */
      gettimeofday(&tv2, NULL);
      if ((1000000*(tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec)) >= 20000)
      {
         send_dum_buf();
         gettimeofday(&tv1, NULL);
      }

      check_for_host_to_c66x_xfer();
      check_for_c66x_to_host_xfer(process_transcoded_dummy_packet);
   }
   
   fclose(output_pcap);
   fclose(input_pcap);
}

static void run_cocpu_network_test(pthread_mutex_t *mx)
{
   
   DWORD  cldfb_size_verification_addr = DSGetSymbolAddr(hCard, NULL, "cldfb_size_verification_var");
   DWORD  sigin_size_verification_addr = DSGetSymbolAddr(hCard, NULL, "sigin_size_verification_var");
   DWORD  cldfb_savemem_size_verification_addr = DSGetSymbolAddr(hCard, NULL, "cldfb_savemem_size_verification_var");
   
   unsigned int cldfb_size_verification_var = 0, sigin_size_verification_var= 0, cldfb_savemem_size_verification_var=0;
   
   DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, cldfb_size_verification_addr, DS_GM_SIZE32, &cldfb_size_verification_var, sizeof(DWORD)/4);
   DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, sigin_size_verification_addr, DS_GM_SIZE32, &sigin_size_verification_var, sizeof(DWORD)/4);
   DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, cldfb_savemem_size_verification_addr, DS_GM_SIZE32, &cldfb_savemem_size_verification_var, sizeof(DWORD)/4);
   
   printf("Running coCPU network test. Press 'q' at any time to quit.\n");
   
   printf("\ncldfb_size_verification_var = %d\n",cldfb_size_verification_var);
   printf("\nsigin_size_verification_var = %d\n",sigin_size_verification_var);
   printf("\ncldfb_savemem_size_verification_var = %d\n",cldfb_savemem_size_verification_var);
   
   while (!needQuit(mx));
}

#define FILESIZE
#define DECODER_PROFILE
/* #define VERBOSE_PRINT */

static void run_codec_test() {

DWORD codec_test_start_addr;
DWORD codec_test_ready_addr;
DWORD codec_test_finished_addr;
DWORD encoder_frame_counter_addr;
DWORD decoder_frame_counter_addr;
DWORD stop_frame_addr;

#ifdef FILESIZE
DWORD FileSize_addr;
// DWORD  toc_header_repeat_addr = DSGetSymbolAddr(hCard, NULL, "toc_header_repeat");
#endif
   
#ifdef CODEC_PROFILING /*Added to see the total number of cycles SC Feb 2016 */
   DWORD evs_fx_total_time_addr;
   DWORD pre_proc_fx_total_cycles_addr;
   DWORD analy_lp_fx_total_cycles_addr;
   DWORD pitch_ol_fx_total_cycles_addr1;
   DWORD pitch_ol_fx_total_cycles_addr2;
   DWORD acelp_core_enc_fx_total_cycles_addr;
   DWORD enc_gen_voic_fx_total_cycles_addr;
   DWORD inov_enc_fx_total_cycles_addr;
   DWORD cod4t64_fx_total_cycles_addr;
   DWORD enc_acelp_total_cycles_addr;
   DWORD lsp_conv_poly_addr;
   DWORD evs_dec_fx_total_time_addr;
   DWORD encoder_total_cycles_addr;
   DWORD bitstream_fx_addr;
   //DWORD cldfb_addr = DSGetSymbolAddr(hCard, NULL, "cldfb_total_cycles");
   DWORD autocorr_fx_addr;
   DWORD conv_fx_addr;
   DWORD corr_addr;
#endif

#ifdef CODEC_DEBUG
   //DWORD max_t_max_addr = DSGetSymbolAddr(hCard, NULL, "max_t_max");
   //DWORD min_track_y_addr = DSGetSymbolAddr(hCard, NULL, "min_track_y");
   DWORD min_sect0_addr;
   DWORD max_sect0_addr;
   DWORD DSP_dotprod_count_addr;
   DWORD max_nb_pulse_addr;
   DWORD max_L_frame_fx_addr;
   DWORD max_nvec_addr;
   DWORD max_ind_value_addr;
#endif

unsigned char        codec_test_finished = 0, codec_test_ready;
unsigned int         one = 1;
DWORD                encoder_frame_count = 0, decoder_frame_count = 0;
struct timeval       tv;
uint64_t             t1e = 0, t1d = 0, t2e = 0, t2d = 0, t2, t2_prev = 0;
MEDIAINFO            MediaInfo;
PMEDIAINFO           pMediaInfo = NULL;
bool                 fOnce_e = false, fOnce_d = false, fRead;
char                 tmpstr[256];
int                  numBytes, numFrames;
DWORD                saveDataAddr, saveDataNumBytes;
uintptr_t            loadDataAddr;
codec_test_params_t  codec_test_params;

#ifdef _USE_FIXED_POINT_LIB
ISPHENC1_Handle      encoder_handle, decoder_handle;
EVSENC_SIG_PARAMS    encoderParams;
EVSDEC_SIG_PARAMS    decoderParams;
#endif

unsigned char*       pBuffer8 = NULL;
short int*           pBuffer16 = NULL;

#ifdef CODEC_PROFILING /*Added to see the total number of cycles SC Feb 2016 */
   unsigned long long  evs_fx_total_time = 0, pre_proc_fx_total_cycles = 0, analy_lp_fx_total_cycles=0,pitch_ol_fx_total_cycles1=0,pitch_ol_fx_total_cycles2=0,acelp_core_enc_fx_total_cycles=0,enc_gen_voic_fx_total_cycles=0, inov_enc_fx_total_cycles=0, cod4t64_fx_total_cycles=0, enc_acelp_total_cycles=0, lsp_conv_poly_total_cycles=0,encoder_total_cycles=0;
   unsigned long long  evs_dec_fx_total_time = 0, bitstream_fx_total_cycles=0,cldfb_total_cycles=0,autocorr_fx_total_cycles=0,conv_fx_total_cycles=0, corr_total_cycles=0;     
#endif
   
#ifdef CODEC_DEBUG
   unsigned int max_nvec =0, max_L_frame_fx =0, max_nb_pulse=0 , max_ind_value=0, min_sect0=0, max_sect0=0, DSP_dotprod_count=0;//max_t_max=0;// min_track_y=0;
#endif

   
/* Determine codec test configuration */

   if (init_codec_test(&MediaParams[0], &codec_test_params))
   {
      printf("Codec Test initialization failed\n");
      return;
   }

   if (CPU_mode & CPUMODE_C66X) {
   
      codec_test_start_addr = DSGetSymbolAddr(hCard, NULL, "unit_test_start");
      codec_test_ready_addr = DSGetSymbolAddr(hCard, NULL, "unit_test_ready");
      codec_test_finished_addr = DSGetSymbolAddr(hCard, NULL, "unit_test_finished");
      encoder_frame_counter_addr = DSGetSymbolAddr(hCard, NULL, "frame_encoder");
      decoder_frame_counter_addr = DSGetSymbolAddr(hCard, NULL, "frame_decoder");
      stop_frame_addr = DSGetSymbolAddr(hCard, NULL, "stop_frame");

      #ifdef FILESIZE
      FileSize_addr = DSGetSymbolAddr(hCard, NULL, "FileSize");
   //   toc_header_repeat_addr = DSGetSymbolAddr(hCard, NULL, "toc_header_repeat");
      #endif
   
      #ifdef CODEC_PROFILING /*Added to see the total number of cycles SC Feb 2016 */
         evs_fx_total_time_addr = DSGetSymbolAddr(hCard, NULL, "evs_fx_total_time");
         pre_proc_fx_total_cycles_addr = DSGetSymbolAddr(hCard, NULL, "pre_proc_fx_total_cycles");
         analy_lp_fx_total_cycles_addr = DSGetSymbolAddr(hCard, NULL, "analy_lp_fx_total_cycles");
         pitch_ol_fx_total_cycles_addr1 = DSGetSymbolAddr(hCard, NULL, "pitch_ol_fx_total_cycles1");
         pitch_ol_fx_total_cycles_addr2 = DSGetSymbolAddr(hCard, NULL, "pitch_ol_fx_total_cycles2");
         acelp_core_enc_fx_total_cycles_addr = DSGetSymbolAddr(hCard, NULL, "acelp_core_enc_fx_total_cycles");
         enc_gen_voic_fx_total_cycles_addr = DSGetSymbolAddr(hCard, NULL, "enc_gen_voic_fx_total_cycles");
         inov_enc_fx_total_cycles_addr = DSGetSymbolAddr(hCard, NULL, "inov_enc_fx_total_cycles");
         cod4t64_fx_total_cycles_addr = DSGetSymbolAddr(hCard, NULL, "cod4t64_fx_total_cycles");
         enc_acelp_total_cycles_addr = DSGetSymbolAddr(hCard, NULL, "enc_acelp_total_cycles");
         lsp_conv_poly_addr = DSGetSymbolAddr(hCard, NULL, "lsp_conv_poly_total_cycles");
         evs_dec_fx_total_time_addr = DSGetSymbolAddr(hCard, NULL, "evs_dec_fx_total_time");
         encoder_total_cycles_addr = DSGetSymbolAddr(hCard, NULL, "encoder_total_cycles");
         bitstream_fx_addr = DSGetSymbolAddr(hCard, NULL, "bitstream_fx_total_cycles");
         //cldfb_addr = DSGetSymbolAddr(hCard, NULL, "cldfb_total_cycles");
         autocorr_fx_addr = DSGetSymbolAddr(hCard, NULL, "autocorr_fx_total_cycles");
         conv_fx_addr = DSGetSymbolAddr(hCard, NULL, "conv_fx_total_cycles");
         corr_addr = DSGetSymbolAddr(hCard, NULL, "corr_total_cycles"); 
      #endif

      #ifdef CODEC_DEBUG
         //max_t_max_addr = DSGetSymbolAddr(hCard, NULL, "max_t_max");
         //min_track_y_addr = DSGetSymbolAddr(hCard, NULL, "min_track_y");
         min_sect0_addr = DSGetSymbolAddr(hCard, NULL, "min_sect0");
         max_sect0_addr = DSGetSymbolAddr(hCard, NULL, "max_sect0");
         DSP_dotprod_count_addr = DSGetSymbolAddr(hCard, NULL, "DSP_dotprod_count");
         max_nb_pulse_addr = DSGetSymbolAddr(hCard, NULL, "‘max_nb_pulse_addr");
         max_L_frame_fx_addr = DSGetSymbolAddr(hCard, NULL, "max_L_frame_fx");
         max_nvec_addr = DSGetSymbolAddr(hCard, NULL, "max_nvec");   
         max_ind_value_addr = DSGetSymbolAddr(hCard, NULL, "max_ind_value");
      #endif
   }
   else {
   
// XDAIS setup/init for EVS - TODO: make this non-codec specific and possibly move to new file/function

      //volatile unsigned int sampling_rate = 16000, bitrate = 13200, frame_size = 640, inter_size = 34;

      printf("non-supported mode\n");
      return;
   }

   memset(&MediaInfo, 0, sizeof(MEDIAINFO));
   

/* Load codec test input file */   

   strcpy(tmpstr, MediaParams[0].Media.inputFilename);
   strupr(tmpstr);

//   if (strstr(tmpstr, ".COD") != NULL) {
   if (codec_test_params.decoder_enable && !codec_test_params.encoder_enable) {  /* is only decoder is enabled ? */
   
      if (CPU_mode & CPUMODE_C66X) {

         loadDataAddr = 0xb1000000;
      }
      else {

         pBuffer8 = (unsigned char*)malloc(9600000L);  /* 10 minutes of 50 frames/sec, 320 bytes per frame (320 = max payload size).  To-do, replace later with inputFilesize element in MediaParams struct */
         loadDataAddr = (uintptr_t)pBuffer8;
      }
      
      printf("Loading %s to coCPU mem adddr 0x%llx\n", MediaParams[0].Media.inputFilename, (long long unsigned int)loadDataAddr);

      numBytes = DSLoadDataFile((CPU_mode & CPUMODE_CPU) ? DS_GM_HOST_MEM : hCard, NULL, MediaParams[0].Media.inputFilename, loadDataAddr, 0, (uint32_t)NULL, NULL);

      numFrames = numBytes / DSGetCompressedFramesize(DS_VOICE_CODEC_TYPE_EVS, MediaParams[0].Streaming.bitRate, HEADERFULL);  /* to-do, replace later with param from session config file, JHB Jan 2016 */
   }
   else {  /* either encoder or both encoder + decoder are enabled */

      if (CPU_mode & CPUMODE_C66X) {

         loadDataAddr = 0xb0000000;
      }
      else {

         pBuffer16 = (short int*)malloc(57600000L);  /* 10 minutes of 48 kHz 16-bit samples.  To-do:  use inputFilesize in MediaParams struct */
         loadDataAddr = (uintptr_t)pBuffer16;
      }

      printf("Loading %s to coCPU mem addr 0x%llx\n", MediaParams[0].Media.inputFilename, (long long unsigned int)loadDataAddr);

      if (strstr(tmpstr, ".WAV") != NULL) pMediaInfo = &MediaInfo;

      numBytes = DSLoadDataFile((CPU_mode & CPUMODE_CPU) ? DS_GM_HOST_MEM : hCard, NULL, MediaParams[0].Media.inputFilename, loadDataAddr, 0, (uint32_t)NULL, pMediaInfo);

      numFrames = numBytes / (MediaParams[0].samplingRate / 25);
   }

   if (numBytes < 0) {

      printf("Unable to load data file to coCPU mem %s\n", MediaParams[0].Media.inputFilename);
      return;
   }

   printf("Running coCPU codec test for %d frames...\n", numFrames);


// numFrames = 10;  // use this to force a short test-run for debug purposes, like if you expect coCPU executable to crash by a certain frame

   if (CPU_mode & CPUMODE_C66X) {

      if (stop_frame_addr) DSWriteMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, stop_frame_addr, DS_GM_SIZE32, &numFrames, 1);
      else printf("stop frame addr not found\n");

     #ifdef FILESIZE
      /* FileSize value is written by the host to c66x memory. In case of c66x decoder only test, FileSize is needed for c66x decoder to obtain number of bytes to be read SC May 2016 */
      DWORD FileSize;
      if (FileSize_addr) DSWriteMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, FileSize_addr, DS_GM_SIZE32, &numBytes, 1);
      else printf("FileSize addr not found\n");
#ifdef VERBOSE_PRINT
      printf("numBytes = %d\n", numBytes);
#endif
      DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, FileSize_addr, DS_GM_SIZE32, &FileSize, sizeof(unsigned int)/4);
#ifdef VERBOSE_PRINT
      printf("FileSize = %d\n", FileSize);
#endif
/*
      toc_header_repeat not used, was only here for debug reasons, JHB Jun 2016

      DWORD toc_header_repeat;
      if(toc_header_repeat_addr)DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, toc_header_repeat_addr, DS_GM_SIZE32, &toc_header_repeat, sizeof(DWORD)/4);
      else printf("toc_header_repeat addr not found\n");
      printf("toc_header_repeat %d ...toc_header_repeat_addr = 0x%x \n", toc_header_repeat, toc_header_repeat_addr);
*/
     #endif

   /* Check codec test status */   

      if (codec_test_ready_addr) {
   
         DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, codec_test_ready_addr, DS_GM_SIZE32, &codec_test_ready, sizeof(unsigned int)/4);
#ifdef VERBOSE_PRINT
         printf("coCPU codec test ready status = %d\n", codec_test_ready);
#endif
      }
      else printf("coCPU codec test ready status addr not found\n");
   }

/* print stats label (stats are displayed continuously on subsequent line) */

   printf("Encoder frame, elapsed time\tDecoder frame, elapsed time\n");

/* Start codec test */   

   if (CPU_mode & CPUMODE_C66X) {

      if (codec_test_start_addr) DSWriteMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, codec_test_start_addr, DS_GM_SIZE32, &one, sizeof(unsigned int)/4);
   }

/* Wait for codec test to finish */   

   while (!codec_test_finished) {

      char ch = getkey();
      if (ch == 'Q' || ch == 'q') break;
      
   /* get time */

      gettimeofday(&tv, NULL);
      t2 = (uint64_t)tv.tv_sec*1000000L + (uint64_t)tv.tv_usec;

      if (!t2_prev) {  /* initialize prev time value */

         t2_prev = t2;
         continue;
      }

      if ((t2 - t2_prev) > 1000) {  /* read coCPU mem values every 1 msec */

      /* read encode frame counter */

         if (codec_test_params.encoder_enable) {

            if (CPU_mode & CPUMODE_C66X) {

               if (encoder_frame_counter_addr)
                 DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, encoder_frame_counter_addr, DS_GM_SIZE32, &encoder_frame_count, sizeof(DWORD)/4);
            }

            if (!fOnce_e && encoder_frame_count) {

               t1e = (uint64_t)tv.tv_sec*1000000L + (uint64_t)tv.tv_usec;
               fOnce_e = true;
            }
         }

         if (codec_test_params.decoder_enable) {

            if (CPU_mode & CPUMODE_C66X) {

               if (decoder_frame_counter_addr)
                 DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, decoder_frame_counter_addr, DS_GM_SIZE32, &decoder_frame_count, sizeof(DWORD)/4);
            }

            if (!fOnce_d && decoder_frame_count) {

               t1d = (uint64_t)tv.tv_sec*1000000L + (uint64_t)tv.tv_usec;
               fOnce_d = true;
            }
         }

         fRead = false;

         if (fOnce_e) {

            if (CPU_mode & CPUMODE_C66X) {

               if (codec_test_ready_addr) {
   
                  DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, codec_test_ready_addr, DS_GM_SIZE32, &codec_test_ready, sizeof(unsigned int)/4);
                  fRead = true;
               }
            }

            if (codec_test_ready < 2) t2e = (uint64_t)tv.tv_sec*1000000L + (uint64_t)tv.tv_usec - t1e;
         }

         if (fOnce_d) {

            if (CPU_mode & CPUMODE_C66X) {

               if (!fRead && codec_test_ready_addr) {
   
                  DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, codec_test_ready_addr, DS_GM_SIZE32, &codec_test_ready, sizeof(unsigned int)/4);
               }
            }

            if (codec_test_ready < 3) t2d = (uint64_t)tv.tv_sec*1000000L + (uint64_t)tv.tv_usec - t1d;
         }

         printf("\r%d, %3.2f  \t\t\t%d, %3.2f  ", encoder_frame_count, 1.0*t2e/1e6, decoder_frame_count, 1.0*t2d/1e6);

      /* read test completion status */

         if (CPU_mode & CPUMODE_C66X) {

            if (codec_test_finished_addr) {

               DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, codec_test_finished_addr, DS_GM_SIZE32, &codec_test_finished, sizeof(DWORD)/4);
               if (codec_test_finished) continue;
            }
         }

         t2_prev = t2;
      }
   }

   printf("\n%d, %3.6f  \t\t%d, %3.6f\n", encoder_frame_count, 1.0*t2e/1e6, decoder_frame_count, 1.0*t2d/1e6);  /* print 6 significant digits to show usec */

   if (CPU_mode & CPUMODE_C66X) {

      DWORD log_buffer_addr = DSGetSymbolAddr(hCard, NULL, "log_buffer");

      if (log_buffer_addr) {
         printf("Saving coCPU log from 0x%x\n", log_buffer_addr);
         DSSaveDataFile(hCard, NULL, "coCPU_log.txt", log_buffer_addr, 0x100000, (uint32_t)NULL, NULL);
      }
      else printf("'log_buffer' symbol not found, coCPU_log.txt file not saved\n");

      DWORD stack_addr = DSGetSymbolAddr(hCard, NULL, "_stack");
      DWORD stack_size = DSGetSymbolAddr(hCard, NULL, "__TI_STACK_SIZE");
      printf("Saving coCPU low stack from addr 0x%x, size = 0x%x\n", stack_addr, stack_size);
      if (stack_addr && stack_size) DSSaveDataFile(hCard, NULL, "stack.txt", stack_addr, stack_size, (uint32_t)NULL, NULL);

      if (codec_test_params.encoder_enable) {

         DWORD heap_free_enc_before_addr = DSGetSymbolAddr(hCard, NULL, "heap_free_enc_before");
         DWORD heap_free_enc_before;

         if (heap_free_enc_before_addr) {
            DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, heap_free_enc_before_addr, DS_GM_SIZE32, &heap_free_enc_before, 1);
            printf("Heap free size before encoder init = %d\n", heap_free_enc_before);
         }

         DWORD heap_free_enc_after_init_addr = DSGetSymbolAddr(hCard, NULL, "heap_free_enc_after_init");
         DWORD heap_free_enc_after_init;

         if (heap_free_enc_after_init_addr) {
            DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, heap_free_enc_after_init_addr, DS_GM_SIZE32, &heap_free_enc_after_init, 1);
            printf("Heap free size after encoder init = %d\n", heap_free_enc_after_init);
         }

         DWORD heap_free_enc_after_addr = DSGetSymbolAddr(hCard, NULL, "heap_free_enc_after");
         DWORD heap_free_enc_after;

         if (heap_free_enc_after_addr) {
            DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, heap_free_enc_after_addr, DS_GM_SIZE32, &heap_free_enc_after, 1);
            printf("Heap free size after encoder delete = %d\n", heap_free_enc_after);
         }

         DWORD evsenc_sig_encgetbufs_addr = DSGetSymbolAddr(hCard, NULL, "evsenc_sig_encgetbufs");
         DWORD evsenc_sig_encgetbufs;

         if (evsenc_sig_encgetbufs_addr) {
            DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, evsenc_sig_encgetbufs_addr, DS_GM_SIZE32, &evsenc_sig_encgetbufs, 1);
            printf("Total encoder .size values in EVSENC_SIG_encgetbufs() = %d\n", evsenc_sig_encgetbufs);
         }

         DWORD total_free_size_main_addr = DSGetSymbolAddr(hCard, NULL, "total_free_size_main");
         DWORD total_free_size_main;

         if (total_free_size_main_addr) {
            DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, total_free_size_main_addr, DS_GM_SIZE32, &total_free_size_main, 1);
            printf("Total encoder .size values in main() = %d\n", total_free_size_main);
         }

         DWORD total_free_size_addr = DSGetSymbolAddr(hCard, NULL, "total_free_size");
         DWORD total_free_size;

         if (total_free_size_addr) {
            DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, total_free_size_addr, DS_GM_SIZE32, &total_free_size, 1);
            printf("Total encoder _ALG_freeMemory .size values = %d\n", total_free_size);
         }
      }

      if (codec_test_params.decoder_enable) {

         DWORD heap_free_dec_before_addr = DSGetSymbolAddr(hCard, NULL, "heap_free_dec_before");
         DWORD heap_free_dec_before;

         if (heap_free_dec_before_addr) {
            DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, heap_free_dec_before_addr, DS_GM_SIZE32, &heap_free_dec_before, 1);
            printf("Heap free size before decoder init = %d\n", heap_free_dec_before);
         }

         DWORD heap_free_dec_after_init_addr = DSGetSymbolAddr(hCard, NULL, "heap_free_dec_after_init");
         DWORD heap_free_dec_after_init;

         if (heap_free_dec_after_init_addr) {
            DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, heap_free_dec_after_init_addr, DS_GM_SIZE32, &heap_free_dec_after_init, 1);
            printf("Heap free size after decoder init = %d\n", heap_free_dec_after_init);
         }

         DWORD heap_free_dec_after_addr = DSGetSymbolAddr(hCard, NULL, "heap_free_dec_after");
         DWORD heap_free_dec_after;

         if (heap_free_dec_after_addr) {
            DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, heap_free_dec_after_addr, DS_GM_SIZE32, &heap_free_dec_after, 1);
            printf("Heap free size after decoder delete = %d\n", heap_free_dec_after);
         }
      }

     #ifdef CODEC_PROFILING
      /* Adding the following read statements to obtain the total number of cycles by each function in EVS ENCODER SC Feb 16*/
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, evs_fx_total_time_addr, DS_GM_SIZE32, &evs_fx_total_time, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, pre_proc_fx_total_cycles_addr, DS_GM_SIZE32, &pre_proc_fx_total_cycles, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, analy_lp_fx_total_cycles_addr, DS_GM_SIZE32, &analy_lp_fx_total_cycles, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, pitch_ol_fx_total_cycles_addr1, DS_GM_SIZE32, &pitch_ol_fx_total_cycles1, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, pitch_ol_fx_total_cycles_addr2, DS_GM_SIZE32, &pitch_ol_fx_total_cycles2, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, acelp_core_enc_fx_total_cycles_addr, DS_GM_SIZE32, &acelp_core_enc_fx_total_cycles, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, enc_gen_voic_fx_total_cycles_addr, DS_GM_SIZE32, &enc_gen_voic_fx_total_cycles, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, inov_enc_fx_total_cycles_addr, DS_GM_SIZE32, &inov_enc_fx_total_cycles, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, cod4t64_fx_total_cycles_addr, DS_GM_SIZE32, &cod4t64_fx_total_cycles, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, enc_acelp_total_cycles_addr, DS_GM_SIZE32, &enc_acelp_total_cycles, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, lsp_conv_poly_addr, DS_GM_SIZE32, &lsp_conv_poly_total_cycles, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, evs_dec_fx_total_time_addr, DS_GM_SIZE32, &evs_dec_fx_total_time, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, encoder_total_cycles_addr, DS_GM_SIZE32, &encoder_total_cycles, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, bitstream_fx_addr, DS_GM_SIZE32, &bitstream_fx_total_cycles, sizeof(DWORD)/4);
      //DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, cldfb_addr, DS_GM_SIZE32, &cldfb_total_cycles, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, autocorr_fx_addr, DS_GM_SIZE32, &autocorr_fx_total_cycles, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, conv_fx_addr, DS_GM_SIZE32, &conv_fx_total_cycles, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, corr_addr, DS_GM_SIZE32, &corr_total_cycles, sizeof(DWORD)/4);  
     #endif
   
     #ifdef CODEC_DEBUG
      //DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, max_t_max_addr, DS_GM_SIZE32, &max_t_max, sizeof(DWORD)/4);
     //DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, min_track_y_addr, DS_GM_SIZE32, &min_track_y, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, max_L_frame_fx_addr, DS_GM_SIZE32, &max_L_frame_fx, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, max_nvec_addr, DS_GM_SIZE32, &max_nvec, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, max_nb_pulse_addr, DS_GM_SIZE32, &max_nb_pulse, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, max_ind_value_addr, DS_GM_SIZE32, &max_ind_value, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, min_sect0_addr, DS_GM_SIZE32, &min_sect0, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, max_sect0_addr, DS_GM_SIZE32, &max_sect0, sizeof(DWORD)/4);
      DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, DSP_dotprod_count_addr, DS_GM_SIZE32, &DSP_dotprod_count, sizeof(DWORD)/4);
     #endif
   
     #ifdef CODEC_PROFILING
      printf("\nevs_fx_total_cycles = %lld\n",evs_fx_total_time);
      printf("\nencoder_total_cycles = %lld\n",encoder_total_cycles);
      printf("\npre_proc_fx_total_cycles = %lld\n",pre_proc_fx_total_cycles);
      printf("\nanaly_lp_fx_total_cycles = %lld\n",analy_lp_fx_total_cycles);
      printf("\npitch_ol_fx_total_cycles1 = %lld\n",pitch_ol_fx_total_cycles1);
      printf("\npitch_ol_fx_total_cycles2 = %lld\n",pitch_ol_fx_total_cycles2);
      printf("\nacelp_core_enc_fx_total_cycles = %lld\n",acelp_core_enc_fx_total_cycles);
      printf("\nenc_gen_voic_fx_total_cycles = %lld\n",enc_gen_voic_fx_total_cycles);
      printf("\ninov_enc_fx_total_cycles = %lld\n",inov_enc_fx_total_cycles);
      printf("\ncod4t64_fx_total_cycles = %lld\n",cod4t64_fx_total_cycles);
      printf("\nenc_acelp_total_cycles = %lld\n",enc_acelp_total_cycles);
      printf("\nevs_dec_fx_total_time = %lld\n",evs_dec_fx_total_time); 
      printf("\nlsp_conv_poly_total_cycles = %lld\n",lsp_conv_poly_total_cycles);
      printf("\nbitstream_fx_total_cycles = %lld\n",bitstream_fx_total_cycles);
      printf("\ncldfb_total_cycles = %lld\n",cldfb_total_cycles); 
      printf("\nautocorr_fx_total_cycles = %lld\n",autocorr_fx_total_cycles); 
      printf("\nconv_fx_total_cycles = %lld\n",conv_fx_total_cycles); 
      printf("\ncorr_total_cycles = %lld\n",corr_total_cycles); 
     #endif
   
     #ifdef CODEC_DEBUG
      //printf("\nmax_t_max = %d\n",max_t_max);
      //printf("\nmin_track_y = %d\n",min_track_y);
      printf("\nmax_L_frame_fx = %d\n",max_L_frame_fx);
      printf("\nmax_nvec = %d\n",max_nvec);
      printf("\nmax_nb_pulse = %d\n",max_nb_pulse);
      printf("\nmax_ind_value = %d\n",max_ind_value);
      printf("\nmin_sect0 = %d\n",min_sect0);
      printf("\nmax_sect0 = %d\n",max_sect0);
      printf("\nDSP_dotprod_count = %d\n",DSP_dotprod_count);
     #endif

   }

   printf("\nCodec test finished\n");
   
/* Save codec test output file */

   strcpy(tmpstr, MediaParams[0].Media.outputFilename);
   strupr(tmpstr);
   
   if (strstr(tmpstr, ".COD") != NULL) {

      if (codec_test_params.encoder_enable) numFrames = min(numFrames, (int)encoder_frame_count);  /* in case of early quite by user */
      
      if (CPU_mode & CPUMODE_C66X) {

         saveDataAddr = 0xb1000000;

         //saveDataNumBytes = numFrames * GetCompressedFramesize(MediaParams[0].Streaming.bitRate, HEADERFULL);  /* to-do, replace later with param from session config file, JHB Jan 2016 */ 
         DWORD cod_filesize_addr = DSGetSymbolAddr(hCard, NULL, "cod_filesize");
         if (cod_filesize_addr) DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, cod_filesize_addr, DS_GM_SIZE32, &saveDataNumBytes, sizeof(DWORD)/4); /* get filesize from C66x mem */
      }
      else {

         saveDataAddr = 0;  /* x86 mem */
      }

      printf("Saving %d bytes of compressed bitstream data to %s\n", saveDataNumBytes, MediaParams[0].Media.outputFilename);
   }
   else {

      if (codec_test_params.decoder_enable) numFrames = min(numFrames, (int)decoder_frame_count);  /* in case of early quit by user */

      if (CPU_mode & CPUMODE_C66X) {

         saveDataAddr = 0xb2000000;

         //saveDataNumBytes = numFrames * (MediaParams[0].samplingRate / 25);
         DWORD dec_filesize_addr = DSGetSymbolAddr(hCard, NULL, "dec_filesize");
         if (dec_filesize_addr) DSReadMem(hCard, DS_RM_LINEAR_DATA | DS_RM_MASTERMODE, dec_filesize_addr, DS_GM_SIZE32, &saveDataNumBytes, sizeof(DWORD)/4); /* get filesize from C66x mem */
      }
      else {

         saveDataAddr = 0;  /* x86 mem */
      }
   
      printf("Saving %d bytes of audio data to %s\n", saveDataNumBytes, MediaParams[0].Media.outputFilename);
   }

   if (strstr(tmpstr, ".WAV") != NULL) {
   
      MediaInfo.Fs = MediaParams[0].samplingRate;  /* MediaParams[] has some values filled in by init_codec_test(), which reads a session config file from the cmd line */
 
      MediaInfo.SampleWidth = 16;
      MediaInfo.NumChan = 1;

      pMediaInfo = &MediaInfo;
   }
   else pMediaInfo = NULL;
   
   if (saveDataNumBytes) DSSaveDataFile((CPU_mode & CPUMODE_CPU) ? DS_GM_HOST_MEM : hCard, NULL, MediaParams[0].Media.outputFilename, saveDataAddr, saveDataNumBytes, (uint32_t)NULL, pMediaInfo);
   if (saveDataNumBytes) DSSaveDataFile((CPU_mode & CPUMODE_CPU) ? DS_GM_HOST_MEM : hCard, NULL, "test_files/chan2.wav", 0xb3000000, saveDataNumBytes, (uint32_t)NULL, pMediaInfo);

   if (CPU_mode & CPUMODE_X86_TEST) {
   
      if (pBuffer8 != NULL) free(pBuffer8);
      if (pBuffer16 != NULL) free(pBuffer16);
   }
}


static void coCPU_Cleanup() {

   if (hCard) {

      if (fcoCPUInitialized) {
      
         if (!DSResetCores(hCard, nCoreList)) printf("DSResetCores failed. Exit.\n");
      }
      
      DSFreeCard(hCard);
      hCard = 0;
   }
}


int log_file_diagnostics() {

char tmpstr[1024];
uint64_t timestamp, prev_timestamp = 0;
int line_count = 0;
char* p, *p2;

int ret_val = -1;
FILE* LogFile = NULL;


   if (strlen(MediaParams[0].Media.inputFilename) == 0) {
      printf("No log file entered\n");
      goto exit;
   }

   LogFile = fopen(MediaParams[0].Media.inputFilename, "r");

   if (!LogFile) {
      printf("Unable to open log file %s\n", MediaParams[0].Media.inputFilename);
      goto exit;
   }

   while (fgets(tmpstr, sizeof(tmpstr), LogFile)) {  /* read in log file line-by-line */

      do {

         p = strchr(tmpstr, ':');
         p2 = strchr(tmpstr, '.');
         if (p) *p = ' ';
         if (p2) *p2 = ' ';

      } while (p || p2);

      int hrs = 0, min = 0, sec = 0, msec = 0, usec = 0;
      sscanf(tmpstr, "%d %d %d %d %d", &hrs, &min, &sec, &msec, &usec);

      timestamp = hrs*3600000000L + min*60000000L + sec*1000000L + msec*1000 + usec;

      line_count++;
      if (line_count % 100 == 0) printf("\rProcessing log file line %d ", line_count);

      if (prev_timestamp && timestamp < prev_timestamp) printf(" error: timestamps not sequential line %d ", line_count);
      prev_timestamp = timestamp;
   }

   printf("\n");

   if (LogFile) fclose(LogFile);

   ret_val = 0;

exit:
   return ret_val;
}


/* application entry point */

int main(int argc, char **argv) {

  
/* API app items */

WORD              wCardClass = 0;                  /* card class (required for the return value for DSGetCardClass) */
QWORD             nCoreList_temp;                  /* temporary core list used for manipulating core list without losing original core list */
   
int               ret_val;                         /* variable for holding function call return values */
int               main_ret = -1;                   /* main() return value */
pthread_t         control_thread;                  /* thread for handling c66x -> host mailbox */
pthread_mutex_t   mxq;                             /* mutex used as quit flag */
char              pthread_created = 0;
int               i;
DWORD             dw_mainprobe_addr, main_probe;
DWORD             dw_chipid_addr, chip_id;

/* code starts, display banner messages */

  	printf("SigSRF media transcoding, codec, speech recognition, and packet streaming test and measurement program for x86 and/or coCPU platforms, Rev 2.5, Copyright (C) Signalogic 2015-2019\n");
   printf("  Libraries in use: DirectCore v%s, pktlib v%s, streamlib v%s, voplib v%s, diaglib v%s, cimlib v%s", HWLIB_VERSION, PKTLIB_VERSION, STREAMLIB_VERSION, VOPLIB_VERSION, DIAGLIB_VERSION, CIMLIB_VERSION);
#if defined(_ALSA_INSTALLED_)
   printf(", aviolib v%s", AVIOLIB_VERSION);
#endif
   printf("\n");

   if (strstr(PKTLIB_VERSION, "DEMO") || strstr(VOPLIB_VERSION, "DEMO")) demo_build = true;
   if (demo_build)printf("Using demo only library versions\n");

   if (!cmdLineInterface(argc, argv, CLI_MEDIA_APPS)) exit(EXIT_FAILURE);

   if (programMode == LOG_FILE_DIAGNOSTICS) {
   
      main_ret = log_file_diagnostics();
      goto exit;
   }

/* Verify test mode settings - any errors then exit the application (no cleanup is necessary at this point) */

   if (CPU_mode & CPUMODE_C66X)
   {
      if (network_packet_test + cocpu_sim_test + cocpu_network_test + codec_test != 1)
      {
         printf ("Invalid test mode settings for c66x coCPU, please select only one coCPU test mode\n");
         goto exit;
      }
   }
   else
   {
      if (x86_frame_test + x86_pkt_test + codec_test + pcap_extract != 1)
      {
         printf ("Invalid test mode settings for x86, please select only one x86 test mode\n");
         goto exit;
      }
   }
   
/* Additional CPU and coCPU checks */
   
   if (CPU_mode & CPUMODE_X86) {

      printf("Running on x86 cores, no coCPU cores specified\n");

      x86_mediaTest();  /* Run x86 mediaTest */
      main_ret = 0;
      goto exit;
   }

/* Get more values from MediaParams[] */

   inc_src_port_base = MediaParams[0].Streaming.udpPort_src;

/* no c66x card handling if c66x coCPU mode not enabled */

   if (CPU_mode & CPUMODE_X86_TEST) goto test_start;

   
/* Assign card:  obtain card handle to be used for other DirectCore API calls */

   printf("Opening %s coCPU card...\n", PlatformParams.szCardDescription);

   hCard = DSAssignCard(NULL, PlatformParams.szCardDesignator, PlatformParams.nClockRate, 0, 0, 0, PlatformParams.maxActiveCoresPerCard);

   if (hCard == 0) {
   
      printf("DSAssignCard Failed\n");
      goto cleanup;
   }

	DSSetCoreList(hCard, nCoreList);  /* set default core list, per cmd line entry */
   
   
/* Initialize the driver, the card, and all cores on the card

   Notes:  1) The DSInitCores() function opens the driver, resets specified cores, downloads and runs the
              memory-resident "talker" program for each specified core, and performs the following sanity-checks
              and tests:

             -driver
             -PCI interface register-level
             -Card register-level
             -coCPU register-level
             -coCPU onchip memory access

           2) The last parameter in DSInitCores() is a a "Core List Field", which contains a
              list of all cores on which the function call should operate.  A 1 bit indicates a
              core should be included, zero indicates a core should be ignored.  Note that
              several other functions also include a core List Field parameter, such as
              DSResetCore(), DSRunCore(), etc.

           3) The DSInitCard() function is identical to DSInitCores(), except it has no core List
              Field, instead assuming that all cores on the card should be initialized.  This is also
              true for other "xxxCard()" versions of API calls, for example DSResetCard(), DSRunCard(), etc
*/
   
   nCoreList = PlatformParams.nCoreList;
   nCoresPerCPU = PlatformParams.maxCoresPerCPU;
//   nCoresPerCPU = DSGetCardInfo(hCard, DS_GCI_NUMCORESPERCPU);  /* another way to do it */
   
   
   for (i = 0, nCoreList_temp = nCoreList; nCoreList_temp > 0; i++, nCoreList_temp >>= 1) if (nCoreList & (1 << i)) numCores++;
   
   printf("Initializing %s coCPU card with core list 0x%lx...\n", PlatformParams.szCardDescription, nCoreList);

   if (!(fcoCPUInitialized = DSInitCores(hCard, nCoreList))) {
   
      printf("DSInitCores Failed\n");
      goto cleanup;
   }

/* Reset card if talker is not used. (cores should already be in soft-reset state with talker 
   program running; When talker is not used, this resets all the cores) */

   printf("Resetting specified devices and cores...\n");

   if (!DSResetCores(hCard, nCoreList))	{
   
      printf("DSResetCores failed. Exit \n");
      goto cleanup;
   }

	if (!(wCardClass = DSGetCardClass(hCard))) {
   
      printf("DSGetCardClass Failed\n");
      goto cleanup;
   }

/* Load C66x executable binary */

   printf("Downloading coCPU executable file %s to core list 0x%lx...\n", PlatformParams.szTargetExecutableFile, nCoreList);
   
   if (!DSLoadFileCores(hCard, PlatformParams.szTargetExecutableFile, nCoreList)) {

      printf("DSLoadFileCores failed\n");
      goto cleanup;
   }
   else printf("DSLoadFileCores succeeds, executable file format = ELF\n");
   
   printf("Starting coCPU cores...\n");

   DSRunCores(hCard, nCoreList);
   //if (!cimRun_coCPUCard(hCard, CIM_GCL_MED | CIM_RH_DEBUGPRINT, &PlatformParams, NULL)) goto cleanup;  /* start coCPU card */
   
   printf("Initializing coCPU cores, synchronizing host and coCPU cores...\n");
   
/* Initialize SigMRF software */

   if (sigMRF_init() != 0)
   {
      printf("ERROR: failed to initialize coCPU SigMRF\n");
      goto cleanup;
   }
   else printf("coCPU sigMRF_init() successfully completed\n");

   dw_mainprobe_addr = DSGetSymbolAddr(hCard, NULL, "main_probe");

   if (dw_mainprobe_addr) {
   
      DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, dw_mainprobe_addr, DS_GM_SIZE32, &main_probe, 1);
#ifdef VERBOSE_PRINT
      printf("main_probe[0x%x] = 0x%x\n", dw_mainprobe_addr, main_probe);
#endif
   }
   else printf("main_probe symbol not found\n");

   dw_chipid_addr = DSGetSymbolAddr(hCard, NULL, "chip_id");

   if (dw_chipid_addr) {
   
      DSReadMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, dw_chipid_addr, DS_GM_SIZE32, &chip_id, 1);
#ifdef VERBOSE_PRINT
      printf("Chip ID[0x%x] = 0x%x\n", dw_chipid_addr, chip_id);
#endif
   }
   else printf("chip_id symbol not found\n");


test_start:

/* Run codec test if enabled */

   if (codec_test) run_codec_test();
   
/* Create thread to handle control plane and stats */
   
   else
   {
   
      /* init and lock the mutex before creating the thread.  As long as the
         mutex stays locked, the thread should keep running.  A pointer to the
         mutex is passed as the argument to the thread function. 
      */
      pthread_mutex_init(&mxq, NULL);
      pthread_mutex_lock(&mxq);

      if ((ret_val = pthread_create(&control_thread, NULL, control_thread_task, &mxq))) 
      {
         printf("ERROR: pthread_create() failed, function returned %d\n", ret_val);
         goto cleanup;
      }
      else pthread_created = 1;
      
      /* Give other thread some time to start */
      usleep(500000);
   
/* Initialize transcoder */

      if (transcode_init())
      {
         printf("ERROR: transcode_init() failed\n");
         goto cleanup;
      }
      
      /* Delay to ensure init is complete */
      usleep(500000);

/* Set up sessions */

      if ((numSessions = create_sessions(&MediaParams[0])) < 0)
      {
         printf("ERROR: create_sessions() failed\n");
         goto cleanup;
      }
      
      if (numSessions == 0)
      {
         printf("ERROR: no sessions created\n");
         goto cleanup;
      }
   
/* Run specified test */

      if (network_packet_test) run_network_pcie_test(&mxq);
      if (cocpu_sim_test) run_cocpu_sim_test(&mxq);
      if (cocpu_network_test) run_cocpu_network_test(&mxq);
   
/* Tear down sessions */

      if (delete_sessions())
      {
         printf("ERROR: delete_sessions failed\n");
         goto cleanup;
      }
   }

   main_ret = 0;  /* successful run */


cleanup:

/* unlock mxq to tell the thread to terminate, then join the thread */
   if (pthread_created)
   {
      if ((ret_val = pthread_join(control_thread, NULL)))
      {
         printf("ERROR: pthread_join() failed, function returned %d\n", ret_val);
         main_ret = -1;
      }
   }

   if (CPU_mode & CPUMODE_C66X) {

      sigMRF_cleanup();

      if (hCard) coCPU_Cleanup();  /* coCPU card cleanup */
   }

   
/* exit */

exit:

   printf("Process %d finished\n", getpid());

   return main_ret;
}
