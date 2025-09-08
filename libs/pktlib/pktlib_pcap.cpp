/*
$Header: /root/Signalogic/DirectCore/lib/pktlib/pktlib_pcap.cpp
 
Description

  APIs for managing pcap, pcapng, and rtp/rtpdump files

Notes

  -fully multithreaded, no locks, no semaphore
  -no dependencies on other pktlib APIs
 
Projects

  SigSRF, DirectCore
 
Copyright Signalogic Inc. 2017-2025

License

  Github SigSRF License, Version 1.1, https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

Documentation and Usage

  1) All pcap, pacapng, and rtp/rtpdump API definitions and flags are documented on Github (https://github.com/signalogic/SigSRF_SDK/blob/master/pktlib_readme.md) and in pktlib.h
 
  2) Functions here are included in pktlib, a SigSRF shared object library linked by mediaMin and mediaTest reference apps and user apps.

  3) If you modify DSXxx() functions, then place the resulting .o before libpktlib.so in your app link order, then your mods will take precedence over pktlib function names

Revision History

  Created Mar 2017 Chris Johnson
  Modified Jul 2017 JHB, modified DSOpenPcap() so it can be used for read or write (takes a DS_READ or DS_WRITE uFlags option, defined in filelib.h, also see DS_OPEN_PCAP_xxx defines). Added DSReadRecord() and DSWritePcapRecord() APIs
  Modified Sep 2018 CJK, added special case error message to DSOpenPcap() regarding Snoop file format magic number
  Modified Sep 2018 JHB, add VLAN header handling to DSReadPcapRecord()
  Modified Oct 2020 JHB, add pcapng format capability to DSOpenPcap() and DSReadPcapRecord(). This was done initially to support TraceWrangler output (pcap anonymizer tool) but we expect to fully support customer pcapng files
  Modified Jan 2021 JHB, fix bug in DSReadPcapRecord() when pcap has VLAN headers (see comments)
  Modified Dec 2021 JHB, modify DSOpenPcap to return packet type (based on EtherType). This allows ARP, LLC frames, etc to be differentiated
  Modified Sep 2022 JHB, adjust warning in DSOpenPcap() for link layer lengths other than standard Ethernet 14 bytes. Add get_link_layer_len() function
  Modified Sep 2023 JHB, add DSFilterPacket() and DSFindPcapPacket() APIs to support timestamp-matching mode. Currently both are called by DSProcessStreamGroupContributorsTSM() in streamlib.c
  Modified Nov 2023 JHB, modify DSOpenPcap() and DSReadPcapRecord() to handle .rtp (.rtpdump) format
  Modified Feb 2024 JHB, add start and end optional params to DSFindPcapPacket(), add optional amount read param to DSFilterPacket(), add DS_FIND_PCAP_PACKET_USE_SEEK_OFFSET flag (applicable to both DSFindPcapPacket() and DSFilterPacket()) to specify seek based search vs record based and greatly improve performance of these APIs
  Modified May 2024 JHB, add optional pcap_hdr_t* param to DSOpenReadPcap() so it can reference the file header from a prior DSOpenPcap() call. In DSReadPcapRecord() for .rtp format, use file header values for src/dst IP addr and port to extent possible, instead of hard-coded generic values
  Modified Jun 2024 JHB, rename DSReadPcapRecord() to DSReadPcap(), DSWritePcapRecord() to DSWritePcap(), and DS_READ_PCAP_RECORD_COPY to DS_READ_PCAP_COPY
  Modified Jul 2024 JHB, per changes in pktlib.h due to documentation review, DS_OPEN_PCAP_READ_HEADER and DS_OPEN_PCAP_WRITE_HEADER flags are no longer required in DSOpenPcap() calls, move uFlags to second param in DSReadPcap(), move pkt_buf_len to fourth param and add uFlags param in DSWritePcap()
  Modified Jul 2024 JHB, modify DSWritePcap() to add pcap_hdr_t* (pcap file header) param, remove timestamp param and instead use the packet record header param timestamp if uFlags definition DS_WRITE_PCAP_SET_TIMESTAMP_WALLCLOCK is given. Additional DSWritePcap() changes to support more accurate pcap-to-pcap editing and copy operations:
                        -now uses the file header param struct link_type field as-is if already initialized (non-zero)
                        -now uses packet record struct param length fields as-is if already initialized
                        -writes ethernet header struct to each packet record only if link_type == LINKTYPE_ETHERNET
  Modified Jul 2024 JHB, changes in DSOpenPcap() with DS_OPEN_PCAP_WRITE flag (write mode) including:
                        -use pcap_hdr_t* (pcap file header) param struct fields as-is if already initialized
                        -implement DS_OPEN_PCAP_FILE_HDR_PCAP_FORMAT flag to cause file header data to be returned in pcap (libpcap) format instead of pcapng
  Modified Jul 2024 JHB, split out from pktlib.c, relevant revision history moved here
  Modified Jul 2024 JHB, more error handling in DSOpenPcap(), correctly return total number of bytes written in DSWritePcap()
  Modified Sep 2024 JHB, change DS_FMT_USER_RTP_HEADER to DS_FMT_PKT_USER_UDP_PAYLOAD, per flag rename in pktlib.h
  Modified Oct 2024 JHB, debug-only sanity checks in DSReadPcap()
  Modified Nov 2024 JHB, use IPVn_ADDR_XXX defined in pktlib.h
  Modified Dec 2024 JHB, add DS_PKTLIB_SUPPRESS_INFO_MSG flag in DSGetPacketInfo() call with DS_PKT_INFO_PKTINFO
  Modified Dec 2024 JHB, include <algorithm> and use std namespace; minmax.h no longer defines min-max if __cplusplus defined
  Modified Feb 2025 JHB, DSReadPcap() improvements:
                           -handle pcapng format IDB blocks, other known blocks (e.g. NRB, interface statistics, etc), and custom (unknown) blocks
                           -detect and adjust link layer length for Null/Loopback protocol
                           -detect and fix and TCP Segmentation Offload (TSO) zero lengths
                           -comprehensive error handling
  Modified Mar 2025 JHB, per changes in pktlib.h to standardize with other SigSRF libs, adjust references to DS_PKTLIB_SUPPRESS_WARNING_ERROR_MSG, DS_PKTLIB_SUPPRESS_INFO_MSG, and DS_PKTLIB_SUPPRESS_RTP_WARNING_ERROR_MSG flags
  Modified Apr 2025 JHB, per updates in pktlib.h, rename p_eth_hdr_type to p_eth_protocol
  Modified Jun 2025 JHB, add uPktNumber and szUserMsgString params to DSReadPcap(). See comments
  Modified Jun 2025 JHB, add reporting for:
                         -TSO/LSO packets (implement DS_READ_PCAP_REPORT_TSO_LENGTH_FIX flag)
                         -interface description blocks (IDB). See comments near IDB
                         -interface statistics blocks (ISB). See comments near ISB
  Modified Jun 2025 JHB, add support for multiple SHBs. Currently when encountering a 2nd or more SHB we read the block, discard data, and report it with an info message
  Modified Jun 2025 JHB, allow possibility that pcapng file doesn't have initial IDB; i.e. first block after SHB is an EPB or SPB
  Modified Aug 2025 JHB, add profiling build-time option for DSFilterPacket() and DSFindPcapPacket(), some simplifications in DSFilterPacket()
  Modified Aug 2025 JHB, remove hardcoded DS_PKTLIB_SUPPRESS_RTP_WARNING_ERROR_MSG and DS_PKTLIB_SUPPRESS_INFO_MSG flags from pktlib calls inside DSFindPcapPacket() and DSFilterPacket(), let caller control these
  Modified Aug 2025 JHB, add uPktNumber param to DSGetPacketInfo() calls per mod in pktlib.h
*/

/* Linux and/or other OS includes */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <algorithm>
using namespace std;

/* SigSRF includes */

#include "pktlib.h"   /* pktlib API definitions, includes arpa/inet.h, netinet/if_ether.h */
#include "diaglib.h"  /* packet logging and diagnostics, including Log_RT() definition */

#include "minmax.h"   /* note - minmax.h does not define min and max macros if __cplusplus is defined */


static int get_link_layer_len(uint16_t link_type) {  /* added JHB Sep 2022 */

int link_layer_length = -1;

/* currently supported data link layer (layer 2) types and number of on-wire bytes for each (https://www.tcpdump.org/linktypes.html) */
  
   if (link_type == LINKTYPE_ETHERNET)
     link_layer_length = ETH_HLEN;  /* defined in Linux if_ether.h */
   else if (link_type == LINKTYPE_LINUX_SLL)
     link_layer_length = LINKTYPE_LINUX_SLL_LINK_LEN;  /* defined in pktlib.h */ 
   else if (link_type == LINKTYPE_RAW_BSD || link_type == LINKTYPE_RAW || link_type == LINKTYPE_IPV4 || link_type == LINKTYPE_IPV6)  /* see comments in pktlib.h */
    link_layer_length = 0;

   return link_layer_length;
}

/* block seek */

static inline int bseek(FILE* fp_pcap, int length) {  /* replacement for fseek() to give standard negative return value in case of error */

   if (!fseek(fp_pcap, length, SEEK_CUR)) return 1;

   return -1;
}

//#define DEBUG_PCAPNG  /* enable for pcapng format debug, JHB Oct 2020 */

int DSOpenPcap(const char* pcap_file, unsigned int uFlags, FILE** fp_pcap, pcap_hdr_t* pcap_file_hdr, const char* pErrstr) {

int ret_val = -1;  /* default to error condition. Note in some cases ret_val is set based on read contents */
int link_layer_length = -1;
uint32_t link_type, file_type;
pcap_hdr_t pcap_file_hdr_local;
pcap_hdr_t* p_file_hdr;
pcapng_hdr_t* p_file_hdr_ng;
pcapng_idb_t file_idb_pcapng;
char errstr[MAX_INPUT_LEN] = "";
char tmpstr[2*MAX_INPUT_LEN];

   if (!pcap_file || !strlen(pcap_file) || !fp_pcap) {  /* look for NULL path/filename, empty string, or NULL file pointer, JHB Jul 2024 */

      Log_RT(2, "ERROR: DSOpenPcap() says %s %s is %s \n", uFlags & DS_READ ? "input" : "output", !pcap_file || !strlen(pcap_file) ? "path and/or filename" : "file pointer", pcap_file && !strlen(pcap_file) ? "empty string" : "NULL");
      return ret_val;
   }

   int errstrlen = pErrstr ? min(strlen(pErrstr), sizeof(errstr)) : 0;  /* may have been a bug waiting to happen here; add min(strlen), JHB Jul 2024 */
   if (pErrstr) strncpy(errstr, pErrstr, errstrlen);
   if (errstrlen > 0) errstr[errstrlen-1] = 0;

   char extstr[20] = "";
   if (strcasestr(pcap_file, ".pcapng")) strcpy(extstr, " pcapng");
   else if (strcasestr(pcap_file, ".pcap")) strcpy(extstr, " pcap");
   else if (strcasestr(pcap_file, ".rtp")) strcpy(extstr, " rtp");

   if (uFlags & DS_WRITE) {

   /* open file for writing */

      *fp_pcap = fopen(pcap_file, "wb");

      if (!*fp_pcap) {
   
         sprintf(tmpstr, "ERROR: DSOpenPcap() unable to open output%s%s file %s, errno = %d \n", extstr, errstr, pcap_file, errno);
         Log_RT(2, tmpstr);

         goto wr_ret;
      }
      else {

         ret_val = 1;

         if (!(uFlags & DS_OPEN_PCAP_QUIET)) {

            sprintf(tmpstr, "INFO: DSOpenPcap() opened output%s file: %s \n", extstr, pcap_file);
            Log_RT(4, tmpstr);
         }
      }

      if (!(uFlags & DS_OPEN_PCAP_DONT_WRITE_HEADER)) {  /* pktlib.h changed the flag from write to "don't write" so the default becomes no flag. But we still allow user to not write file header (for whatever reason), JHB Jul 2024 */

         if (!pcap_file_hdr) {
            p_file_hdr = &pcap_file_hdr_local;
            memset(p_file_hdr, 0, SIZEOF_PCAP_HDR_T);
         }
         else p_file_hdr = pcap_file_hdr;  /* use caller supplied file header */

      /* fill out file header for output pcap. Don't touch any items already initialized, JHB Jul 2024 */

         if (!p_file_hdr->magic_number) p_file_hdr->magic_number = 0xa1b2c3d4;  /* basic libpcap format */
         if (!p_file_hdr->version_major && !p_file_hdr->version_minor) { p_file_hdr->version_major = 2; p_file_hdr->version_minor = 4; }
         if (!p_file_hdr->thiszone) p_file_hdr->thiszone = 0;  /* don't alter if not zero */
         if (!p_file_hdr->sigfigs) p_file_hdr->sigfigs = 0;
         if (!p_file_hdr->snaplen) p_file_hdr->snaplen = 65535;
         if (!p_file_hdr->link_type) p_file_hdr->link_type = LINKTYPE_ETHERNET;  /* default link type for all records is standard 14-byte ethernet header */

      /* write output file header */

         fwrite(p_file_hdr, SIZEOF_PCAP_HDR_T, 1, *fp_pcap);  /* SIZEOF_PCAP_HDR_T macro defined in pktlib.h */
      }

wr_ret:
      return ret_val;
   }
   else {  /* DS_READ is default if no flag given */

   /* open file for reading */

      if (!(uFlags & DS_OPEN_PCAP_RESET)) {
      
         *fp_pcap = fopen(pcap_file, "rb");

         if (!*fp_pcap) {

            int temp_errno = errno;
            sprintf(tmpstr, "ERROR: DSOpenPcap() %s input%s%s file: %s, errno = %d \n", temp_errno == 2 ? "unable to find" : "failed to open", extstr, errstr, pcap_file, temp_errno);
            Log_RT(2, tmpstr);
            goto rd_ret;
         }
         else {

            ret_val = 1;

            if (!(uFlags & DS_OPEN_PCAP_QUIET)) {

               sprintf(tmpstr, "INFO: DSOpenPcap() opened input%s file: %s \n", extstr, pcap_file);
               Log_RT(4, tmpstr);
            }
         }
      }
      else {  /* DS_OPEN_PCAP_RESET flag given: pcap already open, seek to start then read header info, end up pointing at first packet record, JHB Dec 2021 */

         if (*fp_pcap == NULL || fseek(*fp_pcap, 0, SEEK_SET) != 0) {
         
            char errnostr[50];
            sprintf(errnostr, "errno = %d", errno);
            sprintf(tmpstr, "WARNING: DSOpenPcap() %sreset to start fails %s", errstr, *fp_pcap == NULL ? "fp_pcap given as NULL" : errnostr);
            Log_RT(3, tmpstr);
            goto rd_ret;
         }
      }

      if (!(uFlags & DS_OPEN_PCAP_DONT_READ_HEADER)) {  /* user can specify to not read file header (for whatever reason), JHB Jul 2024 */

      /* read file header and check for magic numbers */

         if (!pcap_file_hdr) p_file_hdr = &pcap_file_hdr_local;
         else p_file_hdr = pcap_file_hdr;

         #if 0
         if (fread(p_file_hdr, sizeof(pcap_hdr_t), 1, *fp_pcap) != 1) {
         #else  /* this way handles rtp struct union inside pcap_hdr_t. See definition in pktlib.h, JHB Nov 2023 */
         if (fread(p_file_hdr, SIZEOF_PCAP_HDR_T, 1, *fp_pcap) != 1) {  /* SIZEOF_PCAP_HDR_T macro defined in pktlib.h */
         #endif

            sprintf(tmpstr, "WARNING: failed to read%s%s file header in file: %s", errstr, extstr, pcap_file ? pcap_file : "");
            Log_RT(3, tmpstr);

            #if 0  /* let app close the file on error. Maybe it's doing something where it expects there might be an error, JHB Jun 2025 */
            fclose(*fp_pcap);
            *fp_pcap = NULL;
            #endif
            ret_val = -1;
            goto rd_ret;
         }
         else {

            #if 0
            printf("\n *** inside pcap open, pcap format header magic number = 0x%x, version major = %u, version minor = %u, thiszone = %u, sigfigs = %u, snaplen = %u, link type = %u \n", p_file_hdr->magic_number, p_file_hdr->version_major, p_file_hdr->version_minor, p_file_hdr->thiszone, p_file_hdr->sigfigs, p_file_hdr->snaplen, p_file_hdr->link_type);

            p_file_hdr_ng = (pcapng_hdr_t*)p_file_hdr;

            printf("\n *** inside pcap open, pcapng format header magic number = 0x%x, version major = %u, version minor = %u, block length = %u, byte order magic = 0x%x, section length = %lld \n", p_file_hdr_ng->magic_number, p_file_hdr_ng->version_major, p_file_hdr_ng->version_minor, p_file_hdr_ng->block_header.block_length, p_file_hdr_ng->byte_order_magic, (long long int)p_file_hdr_ng->section_length);
            #endif

            char szShebang[sizeof_field(pcap_hdr_t, rtp.shebang)+1] = "";
            strncpy(szShebang, p_file_hdr->rtp.shebang, sizeof_field(pcap_hdr_t, rtp.shebang));  /* copy magic number */
            szShebang[sizeof_field(pcap_hdr_t, rtp.shebang)] = 0;  /* make it a C string */

            if (strstr(szShebang, "#!rtpplay1.0")) {

               #if 0
               printf("\n *** read %d bytes, found .rtp shebang header %s, now reading %d more bytes \n", (int)SIZEOF_PCAP_HDR_T, szShebang, (int)(sizeof(pcap_hdr_t) - SIZEOF_PCAP_HDR_T));
               #endif

            /* read remainder of rtp header. Header fields are variable length so we make sure we read more than enough, then parse through it. Once we know the actual file header length, we adjust the file ptr to the first record */

               char* p = (char*)p_file_hdr;  /* make a byte pointer we can use to handle variable length fields */

               if (fread(&p[SIZEOF_PCAP_HDR_T], sizeof(pcap_hdr_t) - SIZEOF_PCAP_HDR_T, 1, *fp_pcap) != 1) {  /* SIZEOF_PCAP_HDR_T macro defined in pktlib.h */

                  sprintf(tmpstr, "WARNING: Failed to read %srtp file header in file: %s", errstr, pcap_file ? pcap_file : "");
                  Log_RT(3, tmpstr);

                  #if 0  /* let app close the file on error. Maybe it's doing something where it expects there might be an error, JHB Jun 2025 */
                  fclose(*fp_pcap);
                  *fp_pcap = NULL;
                  #endif
                  ret_val = -1;
                  goto rd_ret;
               }

            /* parse dst ip addr and port string values */

               p = &p_file_hdr->rtp.dst_ip_addr[0];
               char* p_prev = p;
               int i = 0;

               while (*p != '/' && i < (int)sizeof_field(pcap_hdr_t, rtp.dst_ip_addr)-1) { p++; i++; }

               if (i >= (int)sizeof_field(pcap_hdr_t, rtp.dst_ip_addr)-1) {
                  Log_RT(3, "WARNING: rtp file header dst ip addr string length exceeds %d \n", sizeof_field(pcap_hdr_t, rtp.dst_ip_addr)-1);
                  ret_val = -1;
                  goto rd_ret;
               }

               *p++ = 0;

               strcpy(p_file_hdr->rtp.dst_ip_addr, p_prev);

               p_prev = p;
               i = 0;

               while (*p != 0x0a && i < (int)sizeof_field(pcap_hdr_t, rtp.dst_port)-1) { p++; i++; }

               if (i >= (int)sizeof_field(pcap_hdr_t, rtp.dst_port)-1) {
                  Log_RT(3, "WARNING: rtp file header dst port string length exceeds %d \n", sizeof_field(pcap_hdr_t, rtp.dst_port)-1);
                  ret_val = -1;
                  goto rd_ret;
               }

               *p++ = 0;
               strcpy(p_file_hdr->rtp.dst_port, p_prev);

            /* parse uint32_t and uint16_t values */

               p_file_hdr->rtp.start_sec = 0; i = 0;
               while (i < (int)sizeof_field(pcap_hdr_t, rtp.start_sec)) { i++; p_file_hdr->rtp.start_sec |= *p++ << ((sizeof_field(pcap_hdr_t, rtp.start_sec)-1-i)*8); }

               p_file_hdr->rtp.start_usec = 0; i = 0;
               while (i < (int)sizeof_field(pcap_hdr_t, rtp.start_usec)) { i++; p_file_hdr->rtp.start_usec |= *p++ << ((sizeof_field(pcap_hdr_t, rtp.start_usec)-1-i)*8); }

               p_file_hdr->rtp.src_ip_addr = 0; i = 0;
               while (i < (int)sizeof_field(pcap_hdr_t, rtp.src_ip_addr)) { i++; p_file_hdr->rtp.src_ip_addr |= *p++ << ((sizeof_field(pcap_hdr_t, rtp.src_ip_addr)-1-i)*8); }

               p_file_hdr->rtp.src_port = 0; i = 0;
               while (i < (int)sizeof_field(pcap_hdr_t, rtp.src_port)) { i++; p_file_hdr->rtp.src_port |= *p++ << ((sizeof_field(pcap_hdr_t, rtp.src_port)-1-i)*8); }

               p_file_hdr->rtp.padding = 0; i = 0;
               while (i < (int)sizeof_field(pcap_hdr_t, rtp.padding)) { i++; p_file_hdr->rtp.padding |= *p++ << ((sizeof_field(pcap_hdr_t, rtp.padding)-1-i)*8); }

               #ifdef RTP_HEADER_DEBUG
               printf("\n *** .rtp file header of size %d read, dst ip addr = %s, dst_port = %s, start sec = %u, start usec = %u, src ip addr = %u, src port = %u, padding = %u \n", (int)(p - (char*)p_file_hdr), p_file_hdr->rtp.dst_ip_addr, p_file_hdr->rtp.dst_port, p_file_hdr->rtp.start_sec, p_file_hdr->rtp.start_usec, p_file_hdr->rtp.src_ip_addr, p_file_hdr->rtp.src_port, p_file_hdr->rtp.padding);
               #endif

            /* file header info fully parsed, adjust file pointer to start of RTP records */
  
               fseek(*fp_pcap, (int)(p - (char*)p_file_hdr), SEEK_SET);

            /* for the time being we're using Raw IP link type, which has no link layer data in the packet */

               link_type = LINKTYPE_RAW;
               link_layer_length = get_link_layer_len(link_type);
               file_type = PCAP_TYPE_RTP;
               ret_val =  (link_type << 20) | (file_type << 16) | link_layer_length;
               goto rd_ret;
            }

            if (p_file_hdr->magic_number != 0xa1b2c3d4 && p_file_hdr->magic_number != 0x0a0d0d0a) {  /* check for (i) libpcap format and (ii) pcapng format magic numbers, JHB Oct 2020 */

               if (p_file_hdr->magic_number == 0x6f6f6e73)
                  sprintf(tmpstr, "%spcap file: %s, \"Snoop\" file format magic number found but not supported, try opening in Wireshark and re-saving as pcap format", errstr, pcap_file ? pcap_file : "");
               else
                  sprintf(tmpstr, "%spcap file: %s, unexpected magic number: 0x%x\nCapture file format is unsupported", errstr, pcap_file ? pcap_file : "", p_file_hdr->magic_number);

               Log_RT(3, tmpstr);

               #if 0  /* let app close the file on error. Maybe it's doing something where it expects there might be an error, JHB Jun 2025 */
               fclose(*fp_pcap);
               *fp_pcap = NULL;
               #endif
               ret_val = -1;
               goto rd_ret;
            }

            if (p_file_hdr->magic_number == 0x0a0d0d0a) {  /* pcapng handling, JHB Oct 2020 */

               p_file_hdr_ng = (pcapng_hdr_t*)p_file_hdr;

               bseek(*fp_pcap, max(p_file_hdr_ng->block_header.block_length - SIZEOF_PCAP_HDR_T, 0));  /* seek to end of SHB block */

               #if 0  /* no longer assume an IDB immediately follows the first SHB, JHB Jun 2025 */
               ret_val = fread(&file_idb_pcapng, sizeof(pcapng_idb_t), 1, *fp_pcap);  /* read interface description block */
               link_type = file_idb_pcapng.link_type;
               #else
               ret_val = fread(&file_idb_pcapng, sizeof(pcapng_block_header_t), 1, *fp_pcap);  /* read next block header */

               if (file_idb_pcapng.block_header.block_type == PCAPNG_IDB_TYPE) {  /* if block is an IDB, read remaining block header and save link type */

                  ret_val = fread((void*)((uint8_t*)&file_idb_pcapng + sizeof(pcapng_block_header_t)), sizeof(pcapng_idb_t) - sizeof(pcapng_block_header_t), 1, *fp_pcap);

                  link_type = file_idb_pcapng.link_type;

                  bseek(*fp_pcap, max(file_idb_pcapng.block_header.block_length - sizeof(pcapng_idb_t), 0));  /* seek to end of IDB block */
               }
               else {

                  bseek(*fp_pcap, -sizeof(pcapng_block_header_t));  /* otherwise seek back to start of block */

                  link_type = LINKTYPE_ETHERNET;  /* set link type to default -- this may be wrong, so we issue a warning message */

                  char* p = NULL, *szFilepath = getFilePathFromFilePointer(*fp_pcap);  /* in diaglib.h */
                  if (szFilepath) { p = strrchr(szFilepath, '/'); if (!p) p = szFilepath; else p++; }

                  Log_RT(3, "WARNING: DSOpenPcap() says %s does not have IDB after first SHB; assuming Ethernet link type, block type found = %d, block len = %d, uFlags = 0x%x \n",  p ? p : "error in getFilePathFromFilePointer()", file_idb_pcapng.block_header.block_type, file_idb_pcapng.block_header.block_length, uFlags);
                  if (szFilepath) free(szFilepath);
               }
               #endif

               file_type = PCAP_TYPE_PCAPNG;

               if ((uFlags & DS_OPEN_PCAP_FILE_HDR_PCAP_FORMAT) && pcap_file_hdr) {  /* see if user asked for file header data to be returned as pcap header struct, not pcapng, JHB Jul 2024 */

                  #if 0  /* if it turns out some things from pcapng header struct are transferable to pcap header struct, but currently doesn't seem to be the case */
                  pcapng_hdr_t pcapng_hdr = { 0 };  /* make separate copy of pcapng file header */
                  memcpy(&pcapng_hdr, p_file_hdr_ng, sizeof(pcapng_hdr_t));
                  #endif
                  memset(p_file_hdr, 0, SIZEOF_PCAP_HDR_T);

                  p_file_hdr->magic_number = 0xa1b2c3d4;
                  p_file_hdr->version_major = 2;
                  p_file_hdr->version_minor = 4;
                  #if 0  /* these don't seem to have pcapng equivalents, leave as zero */
                  p_file_hdr->thiszone =
                  p_file_hdr->sigfigs =
                  #endif
                  p_file_hdr->snaplen = 262144;  /* typical figure in online discussion */
                  p_file_hdr->link_type = link_type;
               }

               #ifdef DEBUG_PCAPNG
               printf(" section length = %llx, section block length = %u, interface block type = %u, interface block length = %u, link_type = %u \n", (unsigned long long)p_file_hdr_ng->section_length, p_file_hdr_ng->block_header.block_length, file_idb_pcapng.block_type, file_idb_pcapng.block_header.block_length, file_idb_pcapng.link_type);
               /*
               fclose(*fp_pcap);
               *fp_pcap = NULL;
               ret_val = -1;
               goto rd_ret;
               */
               #endif
            }
            else {

               link_type = p_file_hdr->link_type;
               file_type = PCAP_TYPE_LIBPCAP;
            }

            if ((link_layer_length = get_link_layer_len(link_type)) < 0) {

               sprintf(tmpstr, "%spcap file: %s, unsupported data link type: %d", errstr, pcap_file ? pcap_file : "", link_type);
               Log_RT(3, tmpstr);

               #if 0  /* let app close the file on error. Maybe it's doing something where it expects there might be an error, JHB Jun 2025 */
               fclose(*fp_pcap);
               *fp_pcap = NULL;
               #endif
               ret_val = -1;
               goto rd_ret;
            }

            ret_val =  (link_type << 20) | (file_type << 16) | link_layer_length;  /* return link type in bits 27-20, file type in bits 19-16, and link layer length in lower 16 bits. See PCAP_LINK_LAYER_xxx definitions in pktlib.h. These values are used by DSReadPcap(), JHB Oct 2020 */

            #ifdef DEBUG_PCAPNG
            printf("link type (bits 27-20) | file type (bits 19-16) | link layer len (lower 16 bits) = 0x%x \n", ret_val);
            #endif
         }
      }

rd_ret:
      return ret_val;
   }
}

#if 0  /* for reference here are the ethernet header types we currently support. The actual constants are in if_ether.h; for more info, see https://en.wikipedia.org/wiki/EtherType, JHB Sep 2018 */

#define ETH_P_IP    0x0800  /* IPv4 */
#define ETH_P_IPV6  0x86dd  /* IPv6 */
#define ETH_P_8021Q 0x8100  /* 802.1Q VLAN Extended Header */

typedef struct {

  char dest[6];
  char src[6];
  uint16_t type;  /* big-endian byte order */

} ethernet_hdr_t;

#endif

/* DSReadPcap() reads one or more pcap records at the current file position of fp_pcap into pkt_buf, and fills in one or more pcaprec_hdr_t structs (defined in pktlib.h). Notes:

   -fp_pcap should point to a pcap, pcapng, or rtpXXX file previously opened by DSOpenPcap()
   -uFlags are given in DS_READ_PCAP_XXX definitions below
   -pkt_buf should point to a sufficiently large memory area to contain returned packet data
   -link_layer_info should be supplied from a prior DSOpenPcap() call. See comments above
   -if an optional p_eth_protocol pointer is supplied, one or more ETH_P_XXX ethernet protocol flags will be returned (as defined in linux/if_ether.h). NULL indicates not used
   -if an optional p_block_type pointer is supplied, one or more PCAP_XXX_TYPE or PCAPNG_XXX_TYPE flags will be returned (as defined above). When reading pcap or .rtpxxx files, PCAP_PB_TYPE or RTP_PB_TYPE is returned. NULL indicates not used
   -if an optional pcap_file_hdr pointer is supplied, the file header will be copied to this pointer (see pcap_hdr_t struct definition)
   -if a non-zero uPktNumber is supplied, warning, error, and/or information messages displayed will include this number at the end of the message. For messages concerning Interface Description, Interface Statistics, Journal, Decryption, or other block types the text "last transmitted data " is prefixed and uPktNumber-1 is displayed, as these block types do not contain actual transmitted packet data. Applications are expected to keep track of packet numbers, for example to match accurately with Wireshark, even if they perform non-sequential file access
   -if an optional szUserMsgString pointer is supplied, warning, error, and/or information messages displayed will include this string at the end of the message

   -return value is the length of the packet, zero if file end has been reached, or < 0 for an error condition
*/

int DSReadPcap(FILE* fp_pcap, unsigned int uFlags, uint8_t* pkt_buffer, pcaprec_hdr_t* pcap_pkt_hdr, int link_layer_info, uint16_t* p_eth_protocol, uint16_t* p_block_type, pcap_hdr_t* pcap_file_hdr, unsigned int uPktNumber, const char* szUserMsgString) {

pcaprec_hdr_t pcap_pkt_hdr_local;  /* defined in pktlib.h */
pcaprec_hdr_t* p_pkt_hdr;
pcapng_block_header_t pcapng_block_header;
pcapng_epb_t pcapng_epb = { 0 };
pcapng_spb_t pcapng_spb = { 0 };
pcapng_idb_t pcapng_idb = { 0 };
int packet_length;
uint64_t usec = 0, fp_save = 0;
uint8_t pkt_buffer_local[MAX_TCP_PACKET_LEN];  /* overly large but at least no chance of error */
uint8_t* pkt_ptr;
struct ethhdr eth_hdr;  /* defined in netinet/if_ether.h */
vlan_hdr_t vlan_hdr;  /* defined in pktlib.h */
uint16_t eth_protocol = 0, file_type, link_len, link_type, padding = 0;
uint16_t rtp_len = 0, record_len = 0;  /* rtp file format items */
char errstr[1024] = "";
bool fUnusedBlockType = false;
char szPktNumber[30] = "";

uint16_t block_type_local = 0;  /* use this for intermediate results if user gives p_block_type as NULL */

/* to-do: for pcapng simple packet blocks use SnapLen if less than Original Packet Length */

/*
a few useful constants from if_ether.h (one copy here https://github.com/spotify/linux/blob/master/include/linux/if_ether.h):

#define ETH_P_IP	0x0800		// Internet Protocol packet
#define ETH_P_X25	0x0805		// CCITT X.25
#define ETH_P_ARP	0x0806		// Address Resolution packet
*/

   if (!fp_pcap) return -1;  /* error condition */

   if (feof(fp_pcap)) return 0;  /* check for invalid file handle or eof */

   if (!pcap_pkt_hdr) p_pkt_hdr = &pcap_pkt_hdr_local;
   else p_pkt_hdr = pcap_pkt_hdr;

   if (!pkt_buffer) pkt_ptr = pkt_buffer_local;
   else pkt_ptr = pkt_buffer;

   if (!p_block_type) p_block_type = &block_type_local;

   if (uPktNumber) sprintf(szPktNumber, ", pkt# %u", uPktNumber);

/* read pcap or rtp record header, link layer header, packet data */

   if (uFlags & DS_READ_PCAP_COPY) fp_save = ftell(fp_pcap);

   file_type = (link_layer_info & PCAP_LINK_LAYER_FILE_TYPE_MASK) >> 16;
   link_type = (link_layer_info & PCAP_LINK_LAYER_LINK_TYPE_MASK) >> 20;
   link_len = link_layer_info & PCAP_LINK_LAYER_LEN_MASK;

   if (file_type == PCAP_TYPE_RTP) {

   /* following rtp format at https://formats.kaitai.io/rtpdump */
 
      uint32_t timestamp;  /* msec offset from start, per https://github.com/irtlab/rtptools/blob/master/rtpdump.h */

      *p_block_type = RTP_PB_TYPE;  /* return block type */

      if (fread(&record_len, sizeof(record_len), 1, fp_pcap) != 1) { sprintf(errstr, "unable to read rtp record header"); goto pcap_read_error; };
      record_len = (record_len >> 8) | (record_len << 8);  /* .rtp format has big-endian items. Is this always true ? I haven't found any documentation yet on this, JHB Nov 2023 */

      if (fread(&rtp_len, sizeof(rtp_len), 1, fp_pcap) != 1) { sprintf(errstr, "unable to read rtp record data"); goto pcap_read_error; };;
      rtp_len = (rtp_len >> 8) | (rtp_len << 8);

      if ((int)record_len - (int)rtp_len != 8) Log_RT(3, "WARNING: DSReadPcap() says rtp format record header fails sanity check, record_len = %u, rtp_len = %u%s%s%s \n", record_len, rtp_len, szUserMsgString ? ", " : "", szUserMsgString ? szUserMsgString : "", szPktNumber);

      if (fread(&timestamp, sizeof(timestamp), 1, fp_pcap) != 1) return 0;
      timestamp = (timestamp >> 24) | ((timestamp & 0x00ff0000) >> 8) | ((timestamp & 0x0000ff00) << 8) | (timestamp << 24);  /* .rtp format has big-endian items. Is this always true ? I haven't found any documentation yet on this, JHB Nov 2023 */

      #if 0
      printf("\n *** rtp record_len = %u, rtp_len = %u, timestamp = %u \n", record_len, rtp_len, timestamp);
      #endif

      p_pkt_hdr->ts_sec = timestamp / 1000L;
      p_pkt_hdr->ts_usec = 1000*timestamp - 1000000L * p_pkt_hdr->ts_sec;

   /* for now, set only incl_len to allow common error checking for all file types. Later we create an IPv4 packet (since each .rtp record includes only RTP header and payload) and we adjust incl_len and orig_len at that time */

      p_pkt_hdr->incl_len = rtp_len;

      eth_protocol = 0;  /* no concept of ARP or 802.2 packet types in .rtp files */
   }
   else if (file_type == PCAP_TYPE_LIBPCAP) {

      *p_block_type = PCAP_PB_TYPE;  /* return block type */

      if (fread(p_pkt_hdr, sizeof(pcaprec_hdr_t), 1, fp_pcap) != 1) { sprintf(errstr, "unable to read packet record header"); goto pcap_read_error; } /* read block header */
   }
   else {  /* pcapng format */

   /* read simple or enhanced block, fill in standard libpcap record header struct with packet length and timestamp values, JHB Oct 2020 */

      int ret_val;

      if ((ret_val = fread(&pcapng_block_header, sizeof(pcapng_block_header), 1, fp_pcap)) != 1) { /* read block header */

         sprintf(errstr, "unable to read pcapng block header, %d bytes left in file", (int)(sizeof(pcapng_block_header) - ret_val));
         goto pcap_read_error;
      }

 //printf("\n *** inside pcapng read, block_type = %d, block_length = %d \n", pcapng_block_header.block_type, pcapng_block_header.block_length);

      *p_block_type = pcapng_block_header.block_type;  /* return block type */

      if (pcapng_block_header.block_type == PCAPNG_SPB_TYPE) {  /* read simple block (used in sustained packet capture and other cases where performance and reduced packet storage are critical), JHB Feb 2025 */

         static bool fOnce = false;  /* so far no SPB test cases, in case it happens in the wild we can find out */
         if (!fOnce) { fOnce = true; fprintf(stderr, "\n *** inside pcapng read spb \n"); }

         memcpy(&pcapng_spb.block_header, &pcapng_block_header, sizeof(pcapng_block_header_t));

         if (fread(&pcapng_spb.original_pkt_len, sizeof(pcapng_spb_t)-sizeof(pcapng_block_header_t), 1, fp_pcap) != 1) { sprintf(errstr, "unable to read simple block packet length"); goto pcap_read_error; }
         p_pkt_hdr->orig_len = p_pkt_hdr->incl_len = pcapng_spb.original_pkt_len;

         p_pkt_hdr->ts_sec = 0;  /* simple blocks don't have timestamps, so we set to zero and let calling application do something about it. mediaMin can handle this using auto-ajust packet push */
         p_pkt_hdr->ts_usec = 0;
      }
      else if (pcapng_block_header.block_type == PCAPNG_EPB_TYPE) {  /* read enhanced block */

 //printf("\n *** inside pcapng read epb \n");

         memcpy(&pcapng_epb.block_header, &pcapng_block_header, sizeof(pcapng_block_header_t));

         if (fread(&pcapng_epb.interface_id, sizeof(pcapng_epb_t)-sizeof(pcapng_block_header_t), 1, fp_pcap) != 1) { sprintf(errstr, "unable to read enhanced block data"); goto pcap_read_error; }

         p_pkt_hdr->incl_len = pcapng_epb.captured_pkt_len;
         p_pkt_hdr->orig_len = pcapng_epb.original_pkt_len;

         p_pkt_hdr->ts_sec = ((usec = (((uint64_t)pcapng_epb.timestamp_hi << 32) | pcapng_epb.timestamp_lo))) / 1000000L;
         p_pkt_hdr->ts_usec = usec - 1000000L * p_pkt_hdr->ts_sec;
      }
      else if (pcapng_block_header.block_type == PCAPNG_IDB_TYPE) {  /* read interface description block (IDB) */

         memcpy(&pcapng_idb.block_header, &pcapng_block_header, sizeof(pcapng_block_header_t));

         if (fread(&pcapng_idb.link_type, sizeof(pcapng_idb_t)-sizeof(pcapng_block_header_t), 1, fp_pcap) != 1) { sprintf(errstr, "unable to read interface description block data"); goto pcap_read_error; }

         #if 0  /* snaplen produces a huge value of 262,144, test with capture_test2.pcapng, JHB Feb 2025 */
         p_pkt_hdr->incl_len = pcapng_idb.snaplen;
         #else
         p_pkt_hdr->incl_len = pcapng_block_header.block_length - sizeof(pcapng_idb_t) - 4;  /* duplicated block length field is handled together with padding below */
         #endif

      /* report basic IDB info, JHB Jun 2025 */

         int block_len = p_pkt_hdr->incl_len - link_len;
         char szPktNumber[50] = "";
         if (uPktNumber > 1) sprintf(szPktNumber, ", last transmitted data pkt# %u", uPktNumber-1);

         if (!(uFlags & DS_READ_PCAP_SUPPRESS_INFO_MSG)) {

            char* p = NULL, *szFilepath = getFilePathFromFilePointer(fp_pcap);  /* in diaglib.h */
            if (szFilepath) { p = strrchr(szFilepath, '/'); if (!p) p = szFilepath; else p++; }

            Log_RT(4, "INFO: DSReadPcap() says reading interface description block (type = %d)%s%s, ignoring data, block len = %d, link len = %d, uFlags = 0x%x%s%s%s \n", pcapng_block_header.block_type, p ? " in " : "", p ? p : "error in getFilePathFromFilePointer()", block_len, link_len, uFlags, szUserMsgString ? ", " : "", szUserMsgString ? szUserMsgString : "", szPktNumber);
            if (szFilepath) free(szFilepath);
         }

         #if 0
         printf("\n *** inside idb, reading %d struct data, p_pkt_hdr->incl_len = %d \n", (int)(sizeof(pcapng_idb_t)-sizeof(pcapng_block_header_t)), p_pkt_hdr->incl_len);
         #endif
      }
      else if (pcapng_block_header.magic_number == 0x0a0d0d0a) {

         bseek(fp_pcap, max(pcapng_block_header.block_length - sizeof(pcapng_hdr_t), 0));  /* seek to end of SHB block and continue. Currently we are not differentiating packets read from multiple pcapng sections */

      /* report pcap has multiple SHBs, notes JHB Jun 2025:

         -currently there is nothing in the SHB block format (https://www.ietf.org/archive/id/draft-tuexen-opsawg-pcapng-03.html#name-section-header-block) we can use to identify which SHB; i.e. no "SHB Id" or SHB count
         -I opened a pcapng spec Github issue (https://github.com/IETF-OPSAWG-WG/draft-ietf-opsawg-pcap/issues/186) to see if there might be a way, answer was no
         -we could keep our own count but that would require per-app / per-thread persistent state information
      */

         if (!(uFlags & DS_READ_PCAP_SUPPRESS_INFO_MSG)) {

            char* p = NULL, *szFilepath = getFilePathFromFilePointer(fp_pcap);  /* in diaglib.h */
            if (szFilepath) { p = strrchr(szFilepath, '/'); if (!p) p = szFilepath; else p++; }
            int block_len = p_pkt_hdr->incl_len - link_len;
            char szPktNumber[50] = "";
            if (uPktNumber > 1) sprintf(szPktNumber, ", last transmitted data pkt# %u", uPktNumber-1);  /* for non-packet blocks we show the last packet block number. Applications should not increment packet numbers for non-packet blocks, otherwise their packet numbering won't match Wireshark */

            Log_RT(4, "INFO: DSReadPcap() says reading 2 or more SHBs (type = 0x%x)%s%s, ignoring data, block len = %d, link len = %d, uFlags = 0x%x%s%s%s \n", pcapng_block_header.block_type, p ? " in " : "", p ? p : "error in getFilePathFromFilePointer()", block_len, link_len, uFlags, szUserMsgString ? ", " : "", szUserMsgString ? szUserMsgString : "", szPktNumber);
            if (szFilepath) free(szFilepath);
         }
      }
      else {  /* unused or unknown block type */

      /* unused/unknown block type notes:

         1) Currently we don't read block data, instead seek past it

         2) We force link_len to zero even if link type is Ethernet, Linux SLL, etc. This assumes that non-packet blocks have no Ethernet header stored after block header and before block data. I assume pcapng spec guys thought about this but I don't see a mention in the spec (https://www.ietf.org/archive/id/draft-tuexen-opsawg-pcapng-03.html, 2021 is latest I can find, no RFC number assigned yet)

         3) Testing with capture_test2.pcapng shows 2 interface statistics blocks at tail end of the pcap, 1920x1080_H.265.pcapng shows 1
            -Wireshark stores a statistics block for each interface captured
            -for capture_test2.png Wireshark "Statistics -> Capture File Properties" shows the two interfaces as "Wi-Fi" and "\Device\NPF_Loopback", 1920x1080_H.265.pcapng shows 1 interface ("\Device\NPF_{E5D1F3BD-2748-4AB0-8C89-8ADD3B34E991}")

         4) mediaMin does not increment per-stream packet numbers for non-packet blocks (that includes IDB blocks typically found at start of a capture pcap)

         5) Additional test cases with unused block types can be listed here
      */

         p_pkt_hdr->incl_len = pcapng_block_header.block_length - sizeof(pcapng_block_header_t) - 4;  /* duplicated block length field is handled together with padding below */

         fUnusedBlockType = true;  /* prevent reading data below, and seek instead */
         int link_len_save = link_len;
         link_len = 0;

      /* report some basic block type info, JHB Jun 2025 */

         int block_len = p_pkt_hdr->incl_len - link_len;
         char blkstr[50] = "";
         bool fWarn = false;

         char szPktNumber[50] = "";
         if (uPktNumber > 1) sprintf(szPktNumber, ", last transmitted data pkt# %u", uPktNumber-1);  /* for non-packet blocks we show the last packet block number. Applications should not increment packet numbers for non-packet blocks, otherwise their packet numbering won't match Wireshark */

         switch (pcapng_block_header.block_type) {

            case 4:
               strcpy(blkstr, "name resolution");
               break;

            case 5:  /* interface statistics block (ISB) */
               uint32_t InterfaceId;
               if (fread(&InterfaceId, sizeof(InterfaceId), 1, fp_pcap) != 1) { sprintf(errstr, "unable to read statistics block interface Id"); goto pcap_read_error; }
               fseek(fp_pcap, -sizeof(InterfaceId), SEEK_CUR);
               sprintf(blkstr, "interface #%u statistics", InterfaceId);  /* include the Interface Id in report info, JHB Jun 2025 */
               break;

             case 9:
               strcpy(blkstr, "journal");
               break;

            case 10:
               strcpy(blkstr, "decryption");
               break;

            default:
               strcpy(blkstr, "unknown / custom");
               fWarn = true;  /* warning message instead of information message for unknown or custom block. If there are many custom blocks that may not be correct thing to do (DS_READ_PCAP_SUPPRESS_WARNING_ERROR_MSG flag can be applied if needed), JHB Jun 2025 */
               break;
         }

      /* display and write to event log */

         if (!(uFlags & (fWarn ? DS_READ_PCAP_SUPPRESS_WARNING_ERROR_MSG : DS_READ_PCAP_SUPPRESS_INFO_MSG))) {

            char* p = NULL, *szFilepath = getFilePathFromFilePointer(fp_pcap);  /* in diaglib.h */
            if (szFilepath) { p = strrchr(szFilepath, '/'); if (!p) p = szFilepath; else p++; }

            Log_RT(fWarn ? 3 : 4, "%s: DSReadPcap() says reading %s block (type = %d)%s%s, ignoring data, block len = %d, link len = %d, uFlags = 0x%x%s%s%s \n", fWarn ? "WARNING" : "INFO", blkstr, pcapng_block_header.block_type, p ? " in " : "", p ? p : " error in getFilePathFromFilePointer()", block_len, link_len_save, uFlags, szUserMsgString ? ", " : "", szUserMsgString ? szUserMsgString : "", szPktNumber);
            if (szFilepath) free(szFilepath);
         }
      }
   }

   #ifdef DEBUG_PCAPNG  /* seems to no longer apply since block_type is error checked above, JHB Feb 2025 */
   static int count = 0;
   if (count < 1000 /*5*/ || (file_type == PCAP_TYPE_PCAPNG && pcapng_epb.block_type != 6)) {
      printf(" pkt count = %d, epb type = %d, block length = %d, usec = %llu, ts_sec = %u, ts_usec = %u \n", count, pcapng_epb.block_type, pcapng_epb.block_header.block_length, (unsigned long long)usec, p_pkt_hdr->ts_sec, p_pkt_hdr->ts_usec);
      count++;
   }
   #endif

//#define DEBUG_SUBHEADERS  /* turn on to debug ethernet and vlan headers */

#if 0
 static bool fOnce = false;
 if (!fOnce) { printf(" *** DSReadPcap() link_layer_info = 0x%x \n", link_layer_info); fOnce = true; }
#endif

   #if 0
   if (file_type == PCAP_TYPE_LIBPCAP && link_len == sizeof(eth_hdr)) {  /* note this makes an assumption on values of link layer length read from the pcap file header when it's first opened.  If there is another type of link layer with length of 14 that is not an Ethernet header, then it's a problem  */
   #else
   if (link_len == sizeof(ethhdr)) {  /* note this makes an assumption on values of link layer length read from the pcap file header when it's first opened. If there is another type of link layer with length of 14 that is not an Ethernet header, then it's a problem. Note the recent addition of Wireshark capture Null/loopback protocol handling, JHB Feb 2025  */
   #endif

   /* read first PRE_ETH_HDR_READ bytes of Ethernet header. We do this to allow scanning for Null/Loopback protocol, JHB Feb 2025 */

      #define PRE_ETH_HDR_READ        6
      #define NULL_LOOPBACK_LINK_LEN  4

      if (fread(&eth_hdr, PRE_ETH_HDR_READ, 1, fp_pcap) != 1) { sprintf(errstr, "unable to read link layer %d bytes", PRE_ETH_HDR_READ); goto pcap_read_error; }  /* read link_len bytes */

      uint32_t null_loopback_af_inet = *((uint32_t*)(&eth_hdr));  /* first 4 bytes of link layer could be Null/Loopback protocol ID */

 // printf("\n *** first 4 bytes = 0x%x \n", null_loopback_af_inet);
  
      if (!(uFlags & DS_READ_PCAP_DISABLE_NULL_LOOPBACK_PROTOCOL) && null_loopback_af_inet == 2) {  /* check for Wireshark capture "Null/loopback" protocol, first 4 bytes == AF_INETxx. Test with capture_test2.pcapng, JHB Feb 2025 */

         link_len = NULL_LOOPBACK_LINK_LEN;
         eth_protocol = ETH_P_IP;
         #ifdef NULL_LOOPBACK_DEBUG
         uint8_t* pkt = (uint8_t*)&eth_hdr;
         printf("\n *** inside null/loopback af_inet = 2, incl_len = %d, orig_len = %d, first pkt byte = 0x%x \n", p_pkt_hdr->incl_len, p_pkt_hdr->orig_len, pkt[NULL_LOOPBACK_LINK_LEN]);
         #endif

         fseek(fp_pcap, NULL_LOOPBACK_LINK_LEN - PRE_ETH_HDR_READ, SEEK_CUR);
      }
      else if (!(uFlags & DS_READ_PCAP_DISABLE_NULL_LOOPBACK_PROTOCOL) && (null_loopback_af_inet == 24 || null_loopback_af_inet == 28 || null_loopback_af_inet == 30)) {

         static bool fOnce = false;  /* so far no test cases, in case it happens in the wild we can find out */
         if (!fOnce) { fOnce = true; fprintf(stderr, "\n *** inside pcapng null/loopback af_inet = 4 \n"); }

         link_len = NULL_LOOPBACK_LINK_LEN;
         eth_protocol = ETH_P_IPV6;

         fseek(fp_pcap, NULL_LOOPBACK_LINK_LEN - PRE_ETH_HDR_READ, SEEK_CUR);
      }
      else {  /* read remainder of Ethernet header */

         if (fread((uint8_t*)&eth_hdr + PRE_ETH_HDR_READ, link_len - PRE_ETH_HDR_READ, 1, fp_pcap) != 1) { sprintf(errstr, "unable to read link layer %d bytes", link_len-PRE_ETH_HDR_READ); goto pcap_read_error; }  /* read link_len bytes */

         eth_protocol = ((eth_hdr.h_proto & 0xff) << 8) | ((eth_hdr.h_proto & 0xff00) >> 8);  /* stored in file as big-endian */

         if (eth_protocol == ETH_P_8021Q) {  /* check for VLAN header type. Note if there is "double-tagging" (stacked VLAN) we need to add a little more code here */

            if (fread(&vlan_hdr, sizeof(vlan_hdr), 1, fp_pcap) != 1) return 0;  /* read vlan header */

            link_len += sizeof(vlan_hdr);  /* adjust amount read, so packet_length below is calculated correctly. This fixes a bug where mediaMin got the wrong timestamp (and stopped pushing packets because it calculated negative time) after the first packet when reading AMRWB_SID.pcap, and presumably other pcaps with VLAN headers, JHB Jan 2021 */
         }

         #ifdef DEBUG_SUBHEADERS
         static bool fOnce = false; if (!fOnce) { printf("Ethernet header type field = 0x%x\n", eth_protocol); fOnce = true; }
         #endif
      }
   }
   else if (link_len == LINKTYPE_LINUX_SLL_LINK_LEN) {  /* parse Linux SLL format, JHB Feb 2025, test with AMR_MusixFile.pcap and AMRWB_SID.pcap */

      fseek(fp_pcap, sizeof(ethhdr), SEEK_CUR);  /* skip first 14 bytes (12 bytes MAC addresses, 2 unused bytes, per  https://stackoverflow.com/questions/37889179/converting-pcap-format-from-linktype-linux-ssl-to-linktype-ethernet) */

      if (fread(&eth_protocol, sizeof(eth_protocol), 1, fp_pcap) != 1) { sprintf(errstr, "unable to read Linux SLL link layer ethernet header type %d bytes", (int)sizeof(eth_protocol)); goto pcap_read_error; }  /* read ethernet header type */

      eth_protocol = ((eth_protocol & 0xff) << 8) | ((eth_protocol & 0xff00) >> 8);  /* stored in file as big-endian */
   }
   else {

      fseek(fp_pcap, link_len, SEEK_CUR);  /* seek forward (wrt to current file position) ethernet header (i.e. link_len number of bytes) */

   /* warn on Ethernet header lengths that don't match expected value associated with link type, JHB Sep 2022 */
  
      if (get_link_layer_len(link_type) < 0) {

         Log_RT(3, "WARNING: DSReadPcap() says unexpected link type = %d, file_type = %d, link_len = %d%s%s%s \n", link_type, file_type, link_len, szUserMsgString ? ", " : "", szUserMsgString ? szUserMsgString : "", szPktNumber);

         #ifdef DEBUG_SUBHEADERS
         static bool fOnce = false; if (!fOnce) { printf("link_len %d does not equal sizeof(eth_hdr) = %d, file_type = %d, link_type = %d \n", link_len, (int)sizeof(eth_hdr), file_type, link_type); fOnce = true; }
         #endif
      }
   }

   if (p_eth_protocol) *p_eth_protocol = eth_protocol;

// if (eth_protocol == ETH_P_ARP) printf(" *************** found ARP packet, include len = %d, link len = %d \n", p_pkt_hdr->incl_len, link_len);

   if ((packet_length = p_pkt_hdr->incl_len - link_len) <= 0) { sprintf(errstr, "incl_len %d - link_len %d <= 0 when reading", p_pkt_hdr->incl_len, link_len); goto pcap_read_error; }  /* error check amount of next file read */

 //if (packet_length > (int)MAX_RTP_PACKET_LEN) printf(" *** DSReadPcap says way huge packet len = %d \n", packet_length);

   if (file_type != PCAP_TYPE_RTP) {  /* for pcap formats we read whole packet data from each record */

      if (!fUnusedBlockType) {
         if (fread(pkt_ptr, packet_length, 1, fp_pcap) != 1) { sprintf(errstr, "unable to read packet length %d bytes", packet_length); goto pcap_read_error; }
      }
      else fseek(fp_pcap, packet_length, SEEK_CUR);  /* unused block type data might exceed what we allow for maximum packet size, so we don't attempt and seek over it instead */
   }
   else {  /* for rtp format we read only rtp data from each record (i.e. each .rtp record includes only RTP header and payload), and then create IPv4 packet header */

      uint8_t rtp_data[8000];  /* size constant 8000 from https://github.com/irtlab/rtptools/blob/master/rtpdump.h */

      if (fread(rtp_data, rtp_len, 1, fp_pcap) != 1) { sprintf(errstr, "unable to read block length %d bytes", rtp_len); goto pcap_read_error; }

      p_pkt_hdr->incl_len = packet_length = rtp_len + IPV4_HEADER_LEN + UDP_HEADER_LEN;
      p_pkt_hdr->orig_len = p_pkt_hdr->incl_len;

      FORMAT_PKT format_pkt;
      format_pkt.IP_Version = IPv4;

   /* create IPv4 packet, JHB May 2024. Notes:

      -use .rtp file header source and destination IP address and port fields if they have non-zero values
      -only if pcap_file_hdr has been given by caller (which implies the caller saved it from original DSOpenPcap() call)
      -so far I've only seen .rtp file format specs that allow (i) one RTP stream per file and (ii) IPv4 addresses (see other comments with https links). Also I'm not sure why destination IP address and port are in string format but source values are not
   */

      if (pcap_file_hdr && pcap_file_hdr->rtp.src_ip_addr != 0) memcpy(&format_pkt.SrcAddr, &pcap_file_hdr->rtp.src_ip_addr, IPV4_ADDR_LEN);  /* use src IP addr and port if non-zero, otherwise use generic local IP values */
      else {
         uint32_t src_ip_addr = htonl(0xC0A80003);  /* 192.168.0.3 */
         memcpy(&format_pkt.SrcAddr, &src_ip_addr, IPV4_ADDR_LEN);  /* FORMAT_PKT SrcAddr field is 16 bytes, so we use memcpy() to fill first 4 bytes */
      }
      if (pcap_file_hdr && pcap_file_hdr->rtp.src_port != 0) format_pkt.udpHeader.SrcPort = pcap_file_hdr->rtp.src_port;
      else format_pkt.udpHeader.SrcPort = 0x0228;  /* 10242, network byte order */

      #if 0
      uint32_t src_ip_addr;
      memcpy(&src_ip_addr, &format_pkt.SrcAddr, IPV4_ADDR_LEN);
      printf(" src ip addr = %x, port = %u \n", src_ip_addr, format_pkt.udpHeader.SrcPort);
      #endif

      uint32_t dst_ip_addr;
      uint16_t dst_port;

      if (!pcap_file_hdr || inet_pton(AF_INET, (const char*)&pcap_file_hdr->rtp.dst_ip_addr, &dst_ip_addr) != 1 || dst_ip_addr == 0) {  /* if dest IP address is non-zero then convert xx.xx.xx.xx format to integer */
         dst_ip_addr = htonl(0xC0A80001);  /* otherwise use generic local IP addr 192.168.0.1 */
      }
      memcpy(&format_pkt.DstAddr, &dst_ip_addr, IPV4_ADDR_LEN);

      if (!pcap_file_hdr || (dst_port = htons(atoi((const char*)&pcap_file_hdr->rtp.dst_port))) == 0) {  /* convert dest port from string (network byte order) */
         dst_port = 0x0A18;  /* 6154, network byte order */
      }
      format_pkt.udpHeader.DstPort = dst_port;

      #if 0
      memcpy(&dst_ip_addr, &format_pkt.DstAddr, IPV4_ADDR_LEN);
      printf(" dst ip addr = %x, port = %u, dst ip string = %s \n", dst_ip_addr, format_pkt.udpHeader.DstPort, pcap_file_hdr ? (const char*)&pcap_file_hdr->rtp.dst_ip_addr : "n/a");
      #endif

      uint32_t pkt_len_fmt = DSFormatPacket(-1, DS_FMT_PKT_STANDALONE | DS_FMT_PKT_USER_HDRALL | DS_FMT_PKT_USER_UDP_PAYLOAD, rtp_data, rtp_len, &format_pkt, pkt_buffer);

      #if 0  /* additional .rtp format sanity checks - verify all payload sub lengths match after submitting packet to DSPacketInfo(), JHB Oct 2024 */
      int pktlen = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTLEN, pkt_buffer, -1, NULL, NULL, 0);
      int udplen = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDLEN, pkt_buffer, -1, NULL, NULL, 0);
      int rtplen = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDLEN, pkt_buffer, -1, NULL, NULL);
      uint32_t rtp_timestamp = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_TIMESTAMP, pkt_buffer, -1, NULL, NULL, 0);

      printf("\n *** p_pkt_hdr->incl_len = %u, packet_length = %u, pkt_len_fmt = %u, rtp_len = %u, timestamp = %u.%u sec, pktlib info: pkt len = %d, udp len = %d, rtp pyld len = %d, rtp timestamp = %u \n", p_pkt_hdr->incl_len, packet_length, pkt_len_fmt, rtp_len, p_pkt_hdr->ts_sec, p_pkt_hdr->ts_usec/1000, pktlen, udplen, rtplen, rtp_timestamp);
      #endif

      if (pkt_len_fmt != p_pkt_hdr->incl_len) Log_RT(3, "WARNING: DSReadPcap() says packet len after format %u not matching file record len %u%s%s%s \n", pkt_len_fmt, p_pkt_hdr->incl_len, szUserMsgString ? ", " : "", szUserMsgString ? szUserMsgString : "", szPktNumber);
   }

/* if not disabled by uFlags, apply TSO "zero length" fix and set packet length to what was captured. Note that Wireshark will label these as "length reported as 0, presumed to be because of TCP segmentation offload (TSO)", test with capture_test2.png, JHB Feb 2025 */

   if (!(uFlags & DS_READ_PCAP_DISABLE_TSO_LENGTH_FIX) && (*p_block_type == PCAP_PB_TYPE || *p_block_type == RTP_PB_TYPE || *p_block_type == PCAPNG_EPB_TYPE || *p_block_type == PCAPNG_SPB_TYPE) && (pkt_ptr[0] >> 4) == IPV4 && pkt_ptr[9] == TCP) {  /* to-do: handle IPv6 */

      if (!((pkt_ptr[2] << 8) | pkt_ptr[3])) {  /* if length is reported as zero then set to packet capture length */

         pkt_ptr[2] = p_pkt_hdr->incl_len >> 8;
         pkt_ptr[3] = p_pkt_hdr->incl_len & 0xff;

         if ((uFlags & DS_READ_PCAP_REPORT_TSO_LENGTH_FIX) && !(uFlags & DS_READ_PCAP_SUPPRESS_INFO_MSG)) {

            char* p = NULL, *szFilepath = getFilePathFromFilePointer(fp_pcap);  /* in diaglib.h */
            if (szFilepath) { p = strrchr(szFilepath, '/'); if (!p) p = szFilepath; else p++; }

            Log_RT(4, "INFO: DSReadPcap() says TSO zero length fixed to %d for file %s, uFlags = 0x%x%s%s%s \n",  (pkt_ptr[2] << 8) | pkt_ptr[3], p ? p : "error in getFilePathFromFilePointer()", uFlags, szUserMsgString ? ", " : "", szUserMsgString ? szUserMsgString : "", szPktNumber);
            if (szFilepath) free(szFilepath);
         }
         #ifdef TSO_LENGTH_FIX_DEBUG
         printf("\n *** applying TSO length fix, inserting pkt len = %d \n", (pkt_ptr[2] << 8) | pkt_ptr[3]);
         #endif
      }
   }

   if (file_type == PCAP_TYPE_PCAPNG) {

      uint32_t dummy_uint32;
      padding = (4 - (p_pkt_hdr->incl_len & 3)) & 3;
      int ret_val = fread(&dummy_uint32, padding, 1, fp_pcap);  /* pcapng format pads packet data to 32 bits */
      (void)ret_val;

      int remaining_bytes;

      if (pcapng_block_header.block_type == PCAPNG_EPB_TYPE) remaining_bytes = pcapng_block_header.block_length - sizeof(pcapng_epb_t) - p_pkt_hdr->incl_len - padding;
      else if (pcapng_block_header.block_type == PCAPNG_SPB_TYPE) remaining_bytes = pcapng_block_header.block_length - sizeof(pcapng_spb_t) - p_pkt_hdr->incl_len - padding;
      else if (pcapng_block_header.block_type == PCAPNG_IDB_TYPE) remaining_bytes = pcapng_block_header.block_length - sizeof(pcapng_idb_t) - p_pkt_hdr->incl_len - padding;
      else remaining_bytes = pcapng_block_header.block_length - sizeof(pcapng_block_header_t) - p_pkt_hdr->incl_len - padding;
      
      bseek(fp_pcap, max(remaining_bytes, 0));
   }

   #ifdef DEBUG_PCAPNG
   static int count2 = 0;
   if (count2 < 5 || (file_type == PCAP_TYPE_PCAPNG && pcapng_epb.block_type != 6)) {
     printf(" packet len = %d, padding = %d \n", packet_length, padding);
     count2++;
   }
   #endif

   if (uFlags & DS_READ_PCAP_COPY) fseek(fp_pcap, fp_save, SEEK_SET);  /* restore filepos if needed */

   return packet_length;

/* read error handler */

pcap_read_error:

   if (!strlen(errstr)) fprintf(stderr, "\n *** code bug: inside DSReadPcap() error handler but errstr len is zero \n");  /* errstr should have non-zero length for error condition */

   if (feof(fp_pcap)) return 0;  /* we already checked feof() on entry, but sometimes fread() returns 0 bytes even if not feof, then checking feof again shows true. I have no idea why, but for now let it be, JHB Feb 2025 */ 

   char* p = NULL, *szFilepath = getFilePathFromFilePointer(fp_pcap);
   if (szFilepath) { p = strrchr(szFilepath, '/'); if (!p) p = szFilepath; else p++; }

   Log_RT(2, "ERROR: DSReadPcap() says %s from file %s, uFlags = 0x%x%s%s%s \n", errstr, p ? p : "error in getFilePathFromPointer()", uFlags, szUserMsgString ? ", " : "", szUserMsgString ? szUserMsgString : "", szPktNumber);
   if (szFilepath) free(szFilepath);

   return -1;  /* return error condition */
}

/* write a pcap record */

int DSWritePcap(FILE* fp_pcap, unsigned int uFlags, uint8_t* pkt_buffer, int packet_length, pcaprec_hdr_t* pcap_pkt_hdr, struct ethhdr* eth_hdr, pcap_hdr_t* pcap_file_hdr
#ifdef USE_LEGACY_PARAMS  /* remove pTermInfo and timespec params, JHB Jul 2024 */
, TERMINATION_INFO* pTermInfo
,  struct timespec* ts
#endif
) {  /* add pcap_hdr_t* param, JHB Jul 2024 */

pcaprec_hdr_t         pcap_pkt_hdr_local = { 0 };
pcaprec_hdr_t*        p_pkt_hdr;
struct ethhdr         eth_hdr_local;
struct ethhdr*        p_eth_hdr;
#ifdef USE_LEGACY_PARAMS
struct timespec       ts_local = { 0 };
struct timespec*      p_ts;
#else
struct timespec       ts = { 0 };
#endif

   if (!pcap_pkt_hdr) p_pkt_hdr = &pcap_pkt_hdr_local;
   else p_pkt_hdr = pcap_pkt_hdr;

   if (!eth_hdr) {
      p_eth_hdr = &eth_hdr_local;
      memset(p_eth_hdr, 0, sizeof(struct ethhdr));
   }
   else p_eth_hdr = eth_hdr;

   int link_type = pcap_file_hdr ? pcap_file_hdr->link_type : LINKTYPE_ETHERNET;  /* default link type is standard 14-byte ethernet header unless file header struct is given, JHB Jul 2024 */

   #ifdef USE_LEGACY_PARAMS
   if (!ts) p_ts = &ts_local;
   else p_ts = ts;

   if (!p_ts->tv_sec) clock_gettime(CLOCK_REALTIME, p_ts);  /* if sec is uninitialized, then get current time */
   p_pkt_hdr->ts_sec = p_ts->tv_sec;
   p_pkt_hdr->ts_usec = p_ts->tv_nsec/1000;
   #else
   if (uFlags & DS_WRITE_PCAP_SET_TIMESTAMP_WALLCLOCK) {  /* get wall clock time if instructed, otherwise leave pcap_pkt_hdr as-is, JHB Jul 2024 */

      clock_gettime(CLOCK_REALTIME, &ts);
      p_pkt_hdr->ts_sec = ts.tv_sec;
      p_pkt_hdr->ts_usec = ts.tv_nsec/1000;
   }
   #endif

   #if 0
   static int count = 0;
   if (count++ < 100) printf("\n *** pkt incl len = %u, pkt orig len = %u, pcap_file_hdr = %p, link type = %u \n", p_pkt_hdr->incl_len, p_pkt_hdr->orig_len, pcap_file_hdr, link_type);
   #endif

   bool fWriteEthHdr = link_type == LINKTYPE_ETHERNET;  /* write out ethernet header depending on link type, JHB Jul 2024 */ 

   if (!p_pkt_hdr->incl_len) p_pkt_hdr->incl_len = packet_length + (fWriteEthHdr ? ETH_HLEN : 0);  /* add 14 for ethernet header (ETH_HLEN is in Linux if_ether.h). If lengths are not zero then leave as-is (i.e. use what's already in pacp_pkt_hdr), JHB Jul 2024 */
   if (!p_pkt_hdr->orig_len) p_pkt_hdr->orig_len = packet_length + (fWriteEthHdr ? ETH_HLEN : 0);

   if (!eth_hdr && fWriteEthHdr) {  /* if no ethernet header struct given and we need to write out a header then Localhost placeholder data */ 

   /* create placeholder ethernet header for output pcap, assume standard 14-byte header */

      memset(p_eth_hdr->h_dest, 0, sizeof(eth_hdr_local.h_dest));  /* for MAC addresses we use Localhost (all zeros) */
      memset(p_eth_hdr->h_source, 0, sizeof(eth_hdr_local.h_source));

      #ifdef USE_LEGACY_PARAMS  /* remove legacy pTermInfo param. See comments in packet_flow_media_proc.c near DSWritePcap() that give additional explanation, JHB Jul 2024 */
      if (pTermInfo != NULL) {

         if (pTermInfo->remote_ip.type == DS_IPV4 || pTermInfo->remote_ip.type == IPv4) p_eth_hdr->h_proto = htons(0x0800);
         else if (pTermInfo->remote_ip.type == DS_IPV6 || pTermInfo->remote_ip.type == IPv6) p_eth_hdr->h_proto = htons(0x86DD);
         else
         {
            Log_RT(2, "ERROR: DSWritePcap, invalid IP type in term info, unable to write packet\n");
            return -1;
         }
      }
      else
      #endif
      {

         uint8_t version = pkt_buffer[0] >> 4;
         if (version == IPv4) p_eth_hdr->h_proto = htons(0x0800);
         else if (version == IPv6) p_eth_hdr->h_proto = htons(0x86DD);
         else
         {
            Log_RT(2, "ERROR: DSWritePcap() says invalid IP header version number: %d found in pkt_buf \n", version);
            return -1;
         }
      }
   }

   int num_bytes_written = fwrite(p_pkt_hdr, sizeof(pcaprec_hdr_t), 1, fp_pcap) * sizeof(pcaprec_hdr_t);
   if (fWriteEthHdr) num_bytes_written += fwrite(p_eth_hdr, sizeof(eth_hdr_local), 1, fp_pcap) * sizeof(eth_hdr_local);
   num_bytes_written += fwrite(pkt_buffer, packet_length, 1, fp_pcap) * packet_length;

   return num_bytes_written;
}

/* DSFilterPcapRecord() reads a pcap file to find the next occurrence of a desired packet type. Notes JHB Sep 2023:

  -filters out packets specified by DS_FILTER_PKT_xxx flags and continues searching record-by-record. To-do: add flags to search for other types
  -link_layer_info must be given from a prior DSOpenPcap(). Maybe at some point we can get this using only fp (DSOpenPcap() would need to keep track)
  -fills a packet buffer if both fp and pktbuf are given. If pktbuf is given but not pktlen then we get pktlen using DSGetPacketInfo() 
  -fills in a PKTINFO struct with packet info, if specified
  -pNumRead is returned in records or in bytes if DS_FIND_PCAP_PACKET_USE_SEEK_OFFSET is given in uFlags

  -returns packet length if packet search succeeds, 0 if not, or < 0 for error condition
*/

//#define PROFILE  /* enable profiling for DSFilterPacket() and DSFindPcapPacket(). Results are printf'd at end of DSFindPcapPacket(), including main filter time inside filter loop, secondary filter time, pcap per-record read time, DSGetPacketInfo() time, and total number of loops in DSFindPcapPacket(), JHB Aug 2025 */

#ifdef PROFILE
uint64_t pktinfo_time = 0, pcapread_time = 0;
#endif

int DSFilterPacket(FILE* fp, unsigned int uFlags, int link_layer_info, pcaprec_hdr_t* p_pcap_rec_hdr, uint8_t* pktbuf, int pktlen, PKTINFO* PktInfo, uint64_t* pNumRead) {

int ret_val = -1;
uint64_t num_read = 0;

PKTINFO PktInfo_local;

pcaprec_hdr_t pcap_pkt_hdr_local;  /* defined in pktlib.h */
pcaprec_hdr_t* p_pkt_hdr;

uint8_t pktbuf_local[MAX_TCP_PACKET_LEN];
uint8_t* pkt_in_buf;
uint64_t cur_pos = 0;

   if (!fp && !pktbuf) {
      Log_RT(3, "WARNING: DSFilterPacket() says both fp_pcap and pktbuf cannot be NULL. If a file handle is not specified a valid pktbuf must be supplied \n");
      return -1;
   }

   if (!PktInfo) PktInfo = &PktInfo_local;

   if (p_pcap_rec_hdr) p_pkt_hdr = p_pcap_rec_hdr;
   else p_pkt_hdr = &pcap_pkt_hdr_local;

   if (pktbuf) pkt_in_buf = pktbuf;
   else pkt_in_buf = pktbuf_local;
   
   uint16_t pkt_type, block_type, input_type = (link_layer_info & PCAP_LINK_LAYER_FILE_TYPE_MASK) >> 16;

   #ifdef PROFILE
   uint64_t start_time = 0;
   struct timespec ts;
   #endif

   if (input_type == PCAP_TYPE_LIBPCAP || input_type == PCAP_TYPE_PCAPNG) {  /* .rtpXXX format files not supported */

      if (fp && (uFlags & DS_FIND_PCAP_PACKET_USE_SEEK_OFFSET)) cur_pos = ftell(fp);

read_packet:

      if (fp) {  /* if a file handle given, we read packet and length from the file. Otherwise we assume pktbuf and pktlen params are valid */

         #ifdef PROFILE
         clock_gettime(CLOCK_MONOTONIC, &ts);
         start_time = ts.tv_sec * 1000000L + ts.tv_nsec/1000;
         #endif

         pktlen = DSReadPcap(fp, uFlags, pkt_in_buf, p_pkt_hdr, link_layer_info, &pkt_type, &block_type, NULL, 0, NULL);

         #ifdef PROFILE
         clock_gettime(CLOCK_MONOTONIC, &ts);
         pcapread_time += ts.tv_sec * 1000000L + ts.tv_nsec/1000 - start_time;
         #endif

         if (uFlags & DS_FIND_PCAP_PACKET_USE_SEEK_OFFSET) {

            uint64_t new_pos = ftell(fp);
            num_read += new_pos - cur_pos;  /* increment by bytes read */
            cur_pos = new_pos;
         }
         else num_read++;  /* increment by records read */
      }
      else if (!pktlen) pktlen = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTLEN | (uFlags & (DS_PKTLIB_SUPPRESS_WARNING_ERROR_MSG | DS_PKTLIB_SUPPRESS_RTP_WARNING_ERROR_MSG | DS_PKTLIB_SUPPRESS_INFO_MSG)), pkt_in_buf, -1, NULL, NULL, 0);  /* get packet length, apply message suppression as specified by caller */

      if (pktlen > 0) {

         if (block_type != PCAP_PB_TYPE && block_type != PCAPNG_EPB_TYPE && block_type != PCAPNG_SPB_TYPE) {  /* ignore IDB, NRB, and other non-packet data block types. See definitions in pktlib.h */

            if (fp) goto read_packet;
            else ret_val = -1;
         }

         if ((uFlags & DS_FILTER_PKT_ARP) && (pkt_type == ETH_P_ARP)) {  /* ignore ARP packets (ETH_P_ARP defined in if_ether.h Linux header file, typically value of 0x0806) */
            #ifdef PKT_DISCARD_DEBUG
            printf(" ************* ignoring ARP pkt \n");
            #endif
            if (fp) goto read_packet;
            else ret_val = -1;
         }

         if ((uFlags & DS_FILTER_PKT_802) && (pkt_type >= 82 && pkt_type <= 1536)) { /* ignore 802.2 LLC frames (https://networkengineering.stackexchange.com/questions/50586/eth-ii-vs-802-2-llc-snap). Note - added the lower range check of 82 after some .pcapng test files with Ethernet prototype value of zero were misinterpreted as 802.2, JHB Sep 2022 */
            #ifdef PKT_DISCARD_DEBUG
            printf(" ************* ignoring LLC frame, pkt_type = %d \n", pkt_type);
            #endif
            if (fp) goto read_packet;
            else ret_val = -1;
         }

      /* fill in PktInfo struct with IP, UDP, and RTP header items */
 
         #ifdef PROFILE
         clock_gettime(CLOCK_MONOTONIC, &ts);
         start_time = ts.tv_sec * 1000000L + ts.tv_nsec/1000;
         #endif

         ret_val = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTINFO | (uFlags & (DS_PKTLIB_SUPPRESS_WARNING_ERROR_MSG | DS_PKTLIB_SUPPRESS_RTP_WARNING_ERROR_MSG | DS_PKTLIB_SUPPRESS_INFO_MSG)), pkt_in_buf, -1, PktInfo, NULL, 0);  /* fill PktInfo struct, apply message suppression as specified by caller. If packets have not yet been input and/or vetted and there is a chance that packets are malformed (invalid IP version, incorrect header, mismatching length, etc), the message suppression flags should not be given, otherwise return value is < 0 and a warning message will be printed by DSGetPacketInfo() */

         if (ret_val < 0) {
            #ifdef PKT_DISCARD_DEBUG
            printf("************* invalid IP version or malformed packet, pkt type = %d, pkt len = %d \n", pkt_type, pktlen);
            #endif
            if (fp) goto read_packet;
            else ret_val = -1;
         }

         uint8_t protocol = PktInfo->protocol;

         if ((uFlags & DS_FILTER_PKT_TCP) && protocol == TCP_PROTOCOL) {
            if (fp) goto read_packet;
            else ret_val = -1;
         }

         if ((uFlags & DS_FILTER_PKT_UDP) && protocol == UDP_PROTOCOL) {
            if (fp) goto read_packet;
            else ret_val = -1;
         }

         if ((uFlags & DS_FILTER_PKT_UDP_SIP) && protocol == UDP_PROTOCOL) {

            #if 0  /* use PktInfo, avoid additional DSGetPacketInfo() calls, JHB Aug 2025 */
            uint16_t dst_port = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_DST_PORT, pkt_in_buf, -1, pPktInfo, NULL, 0);
            uint16_t src_port = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_SRC_PORT, pkt_in_buf, -1, pPktInfo, NULL, 0);

            if (dst_port == SIP_PORT || src_port == SIP_PORT) {
            #else
            if (PktInfo->dst_port == SIP_PORT || PktInfo->src_port == SIP_PORT) {
            #endif
               if (fp) goto read_packet;
               else ret_val = -1;
            }
         }

         if (protocol != UDP_PROTOCOL && protocol != TCP_PROTOCOL) { /* ignore ICMP and various other protocols */
            #ifdef PKT_DISCARD_DEBUG
            printf(" ************* ignoring non UDP or TCP pkt with protocol = %d \n", ret_val);
            #endif
            if (fp) goto read_packet;
            else ret_val = -1;
         }

         if ((uFlags & DS_FILTER_PKT_RTCP) && protocol == UDP_PROTOCOL && (PktInfo->rtp_pyld_type >= RTCP_PYLD_TYPE_MIN && PktInfo->rtp_pyld_type <= RTCP_PYLD_TYPE_MAX)) {  /* skip over RTCP */

            if (fp) goto read_packet;
            else ret_val = -1;
         }

         #ifdef PROFILE
         clock_gettime(CLOCK_MONOTONIC, &ts);
         pktinfo_time += ts.tv_sec * 1000000L + ts.tv_nsec/1000 - start_time;
         #endif

         #if 0
         printf("\n *** file pos end of filter packet = %llu \n", (unsigned long long)ftell(fp));
         #endif
 
      /* packet has met filter specs, leave num_read pointing at matching packet, return packet length */ 

         ret_val = pktlen;
      }
      else if (pktlen == 0) ret_val = 0;
   }

   if (pNumRead) *pNumRead = num_read;  /* return records or bytes read */

   return ret_val;
}


/* DSFindPcapPacket() finds specific packets in a pcap given packet matching specs */

uint64_t DSFindPcapPacket(const char* szInputPcap, unsigned int uFlags, PKTINFO* PktInfo, uint64_t offset_start, uint64_t offset_end, uint64_t* pFoundOffset, int* error_cond) {

FILE* fp_pcap = NULL;
pcaprec_hdr_t pcap_pkt_hdr;
PKTINFO PktInfo_pcap;
int link_layer_info;
uint64_t packet_time = 0, offset_count = 0, num_read = 0;
bool fUseSeek, fFindFirstMatch;
unsigned int uFlags_filter;
int ret_val = 0;

   if (error_cond) *error_cond = 1;  /* initialize error condition to no error */

   #ifdef PROFILE
   uint64_t start_time = 0, read_time = 0, filter_time = 0, filter_time2 = 0;
   struct timespec ts;
   int num_loops = 0;
   pktinfo_time = 0;
   pcapread_time = 0;
   #endif

   fUseSeek = uFlags & DS_FIND_PCAP_PACKET_USE_SEEK_OFFSET;
   fFindFirstMatch = uFlags & DS_FIND_PCAP_PACKET_FIRST_MATCHING;
   uFlags_filter = uFlags | DS_FILTER_PKT_ARP | DS_FILTER_PKT_802 | DS_FILTER_PKT_TCP | DS_FILTER_PKT_UDP_SIP | DS_FILTER_PKT_RTCP;

   if ((link_layer_info = DSOpenPcap(szInputPcap, DS_READ | DS_OPEN_PCAP_QUIET, &fp_pcap, NULL, "")) > 0 && fp_pcap) {

      bool fMatchSSRC = false, fMatchTimestamp = false, fMatchSeqnum = false, fMatchPyldType = false;
      uint32_t rtp_ssrc, rtp_timestamp;
      uint16_t rtp_seqnum;
      uint8_t rtp_pyld_type = 0;

      if (uFlags & DS_FIND_PCAP_PACKET_RTP_SSRC) { fMatchSSRC = true; rtp_ssrc = PktInfo->rtp_ssrc; }
      if (uFlags & DS_FIND_PCAP_PACKET_RTP_TIMESTAMP) { fMatchTimestamp = true; rtp_timestamp = PktInfo->rtp_timestamp; }
      if (uFlags & DS_FIND_PCAP_PACKET_SEQNUM) { fMatchSeqnum = true; rtp_seqnum = PktInfo->seqnum; }
      if (uFlags & DS_FIND_PCAP_PACKET_RTP_PYLDTYPE) { fMatchPyldType = true; rtp_pyld_type = PktInfo->rtp_pyld_type; }

      uint64_t base_time = 0;
      #ifdef PACKET_TIME_DEBUG
      uint32_t last_rtp_time = 0;
      #endif

      if (fUseSeek && !offset_start) offset_count = ftell(fp_pcap);  /* account for pcap header read by DSOpenPcap(). The first action inside the loop will overwrite num_read so we use offset_count */

      bool fFound = false;
      #ifdef SEEK_DEBUG
      int count = 0;
      #endif

      do {  /* use DSFilterPacket() to read packets and filter for unwanted packet types */

         #ifdef PROFILE
         clock_gettime(CLOCK_MONOTONIC, &ts);
         start_time = ts.tv_sec * 1000000L + ts.tv_nsec/1000;
         #endif

         if (offset_end && offset_count > offset_end) break;  /* break out of while loop if we exceed offset_end, JHB Feb 2024 */
         else if (offset_count < offset_start) {  /* skip-over (seek past) or ignore (read) packets already consumed (indicated by offset_start), JHB Feb 2024 */

            if (fUseSeek)  {

               ret_val = 1;  /* ret_val must be > 0 to stay in while loop */
               fseek(fp_pcap, offset_start, SEEK_SET);
               num_read = offset_start;  /* num_read is in bytes */
            }
            else {

               ret_val = DSReadPcap(fp_pcap, uFlags, NULL, NULL, link_layer_info, NULL, NULL, NULL, 0, NULL);
               num_read = 1;  /* num_read is in records */
            }
         }
         else {

            #ifdef SEEK_DEBUG
            if (count == 0) printf("\n *** find packet before first filter packet, ftell = %llu, offset start = %llu, offset_end = %llu, offset_count = %llu, num_read = %llu ", (unsigned long long)ftell(fp_pcap), (unsigned long long)offset_start, (unsigned long long)offset_end, (unsigned long long)offset_count, (unsigned long long)num_read);
            count++;
            #endif

            #ifdef PROFILE
            clock_gettime(CLOCK_MONOTONIC, &ts);
            uint64_t temp_time = ts.tv_sec * 1000000L + ts.tv_nsec/1000;
            read_time += temp_time - start_time;
            start_time = temp_time;
            #endif

            if ((ret_val = DSFilterPacket(fp_pcap, uFlags_filter, link_layer_info, &pcap_pkt_hdr, NULL, 0, &PktInfo_pcap, &num_read)) > 0) {

               #ifdef PROFILE
               clock_gettime(CLOCK_MONOTONIC, &ts);
               temp_time = ts.tv_sec * 1000000L + ts.tv_nsec/1000;
               filter_time += temp_time - start_time;
               start_time = temp_time;
               #endif

               if (!base_time) base_time = (uint64_t)pcap_pkt_hdr.ts_sec*1000000L + pcap_pkt_hdr.ts_usec;  /* save base time of first RTP packet, regardless of which stream. This is not super accurate depending on the user's capture / network setup, and subject to jitter, but best we can do and still maintain repeatability */

            /* all conditions must match; continue until we match a packet with required RTP params */

               if (fMatchSSRC && PktInfo_pcap.rtp_ssrc != rtp_ssrc) {}
               else if (fMatchTimestamp && PktInfo_pcap.rtp_timestamp != rtp_timestamp) {}
               else if (fMatchSeqnum && PktInfo_pcap.seqnum != rtp_seqnum) {}
               else if (fMatchPyldType && PktInfo_pcap.rtp_pyld_type != rtp_pyld_type) {}

               else {  /* match found */

                  packet_time = (uint64_t)pcap_pkt_hdr.ts_sec*1000000L + pcap_pkt_hdr.ts_usec - base_time;

               /* if DS_FIND_PCAP_PACKET_FIRST_MATCHING uFlag we are finished. For last matching packet we continue to update packet_time until the pcap is fully read */

                  if (fFindFirstMatch) fFound = true;  /* done if first match specified */

                  if (pFoundOffset) *pFoundOffset = offset_count + num_read;  /* update offset of matching record, JHB Feb 2024 */
               }

               #ifdef PACKET_TIME_DEBUG
               else if (last_rtp_time) rtp_time_sum += PktInfo_pcap.rtp_timestamp - last_rtp_time;  /* rtp_time_sum not viable for streams > 2, especially if stream shows up much later or after a pause, then it's unclear how to count total rtp time for multiple earlier streams */

               last_rtp_time = PktInfo_pcap.rtp_timestamp;
               #endif
            }
         }

         #ifdef PROFILE
         clock_gettime(CLOCK_MONOTONIC, &ts);
         filter_time2 += ts.tv_sec * 1000000L + ts.tv_nsec/1000 - start_time;
         num_loops++;
         #endif

         offset_count += num_read;

      } while (!fFound && ret_val > 0);  /* ret_val is zero if the pcap reaches its end */

      #ifdef SEEK_DEBUG
      char tstr[20];
      if (pFoundOffset) sprintf(tstr, "%llu", (unsigned long long)*pFoundOffset);
      else strcpy(tstr, "NULL");
      printf("\n *** find packet end, ftell = %llu, found offset = %s, fFound = %d, num filters = %d, ret_val = %d \n", (unsigned long long)ftell(fp_pcap), tstr, fFound, count, ret_val);
      #endif
   }

   if (error_cond && (!fp_pcap || ret_val < 0)) *error_cond = -1;  /* indicate error condition */

   if (fp_pcap) DSClosePcap(fp_pcap, DS_CLOSE_PCAP_QUIET);

   #ifdef PROFILE
   printf("\n *** findpcap read_time = %llu, filter_time = %llu, filter_time2 = %llu, pcapread_time = %llu, pktinfo_time = %llu, num_loops = %d \n", (long long unsigned)read_time, (long long unsigned)filter_time, (long long unsigned)filter_time2, (long long unsigned)pcapread_time, (long long unsigned)pktinfo_time, num_loops);
   #endif
   
   return packet_time;
}

/* close pcap file */

int DSClosePcap(FILE* fp_pcap, unsigned int uFlags) {

int ret_val = -1;

   if (fp_pcap) ret_val = fclose(fp_pcap);

   if (!(uFlags & DS_CLOSE_PCAP_QUIET)) Log_RT(4, "INFO: DSClosePcap() closed pcap file, ret val = %d \n", ret_val);

   return ret_val;
}
