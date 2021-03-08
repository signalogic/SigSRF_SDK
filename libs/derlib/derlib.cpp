/*
 $Header: /root/Signalogic/Directcore/lib/derlib/derlib.cpp

 Copyright (C) Signalogic Inc. 2021

 Description

   DER decoding library, supporting:

   -full abstraction of DER encoded and aggregated packets, no requirement for look-ahead or batch processing. Only need to provide TCP/IP packets as they are received
   -multiple concurrent streams with no spinlocks outside of DSCreateDerStream() and DSDeleteDerStream()
   -DER encoded packet timestamps can be missing or wrong. In that case use mediaMin's "analytics mode"

 Purpose
 
   Support ETSI LI HI2 and HI3 DER encoded streams per ASN.1 format
 
 Revision History

  Created Feb 2021 JHB
  Modified Mar 2021 JHB, fixes for testing with OpenLI captures

*/

/* Linux includes */

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>

/* Sig includes */

#include "pktlib.h"
#include "voplib.h"
#include "diaglib.h"  /* Log_RT() */
#include "derlib.h"
#include "minmax.h"

#include "shared_include/config.h"     /* configuration structs and definitions */

#if 0
 /* detect if packet contains a DER stream. set uFlags for auto-detection of interception point ID and/or dest_port */ 
void DSDeleteDerStream(HDERSTREAM hDerStream)
#endif

typedef struct {

  uint8_t in_use;

  char      szInterceptPointId[MAX_DER_STRLEN];
  uint16_t  dest_port;
  uint8_t*  packet_save;
  int       save_len;
  int       asn_index;
  uint64_t  cc_pkt_decode_count;

} DER_STREAM;

static DER_STREAM der_streams[MAX_DER_STREAMS] = {{ 0 }};

static uint16_t calc_checksum(void* p, int num_elements, int omit_index, int element_size_bits);

static sem_t derlib_sem;
static bool derlib_sem_init = false;
static int stream_index = 0;

/* DSConfigDerLib() initializes derlib

   -has to be called once at app init time, by only one thread
*/

int DSConfigDerlib(GLOBAL_CONFIG* pGlobalConfig, DEBUG_CONFIG* pDebugConfig, unsigned int uFlags) {

   if (uFlags & DS_CD_GLOBALCONFIG) {

      if (pGlobalConfig != NULL) {

         // derlib_gbl_cfg = *pGlobalConfig;
      }
   }
   
   if (uFlags & DS_CD_DEBUGCONFIG) {

      if (pDebugConfig != NULL) {
      }
   }

   if (uFlags & DS_CD_INIT) {      

      if (!derlib_sem_init) {
         sem_init(&derlib_sem, 0, 1);
         derlib_sem_init = true;
      }
   }

   return 1;
}

static int get_next_stream_id() {

int i;

/* lock derlib semaphore before allocating new stream index */

   if (sem_wait(&derlib_sem) < 0) Log_RT(1, "CRITICAL, derlib get_next_stream_id says sem_wait() returns -1, errno = %d \n", errno); 

   i = stream_index;
   do 
   {
      if (der_streams[i].in_use == 0) 
      {
         der_streams[i].in_use = 1;

         stream_index = (i+1) & (MAX_DER_STREAMS-1);  /* always move forward in der_streams[].  Although it might be more efficient to re-use the current index (one we are allocating) if it were immediately cleared with DSDeleteDerStream(), that's unlikely so no point in worrying about that */

         #if 0
         if (lib_dbg_cfg.uLogLevel > 8) Log_RT(8, "DEBUG2: derlib get_next_stream_id() stream id allocated = %d\n", i);

         if (lib_dbg_cfg.uEnableDataObjectStats) {

            int j, total_free = 0;
            for (j=0; j<MAX_DER_STREAMS; j++) if (!der_streams[j].in_use) total_free++;
            if (total_free < min_free_der_stream_handles) min_free_der_stream_handles = total_free;  /* record this stat, if we're operating close to the limit with one or more multithreaded apps we might temporarily exceed the limit, JHB Apr 2019 */
         }
         #endif

         sem_post(&derlib_sem);  /* unlock semaphore before returning allocated stream index */

         return i;
      }

      i = (i + 1) & (MAX_DER_STREAMS - 1);  /* wrap to start of der_streams[] */

   } while (i != stream_index);  /* continue trying until we return back to the original index */
   
/* unlock semaphore before returning */

   sem_post(&derlib_sem);
   
   Log_RT(1, "CRITICAL, derlib get_next_stream_id() says allocated DER stream handles has reached max %d \n", MAX_DER_STREAMS);
   return -1;  /* error, no free stream handles available */
}


HDERSTREAM DSCreateDerStream(const char* szInterceptPointId, uint16_t dest_port, unsigned int uFlags) {  /* returns a handle to a new DER stream */

   (void)uFlags;

/* check for error conditions */

   if (!derlib_sem_init) return -1;
   if (!szInterceptPointId || !strlen(szInterceptPointId)) return -1;
   if (!dest_port) return -1;

/* allocated a new stream handle */

   int stream_index = get_next_stream_id();

   if (stream_index < 0) return -1;

/* init the new stream */

   strcpy(der_streams[stream_index].szInterceptPointId, szInterceptPointId);
   der_streams[stream_index].dest_port = dest_port;
   der_streams[stream_index].packet_save = (uint8_t*)malloc(MAX_RTP_PACKET_LEN);  /* create some mem to handle DER encoded items (mainly packets) that break across packet payloads */

   return stream_index + 1;  /* when apps check for a valid stream handle, anything <= 0 is invalid */
}

int DSDeleteDerStream(HDERSTREAM hDerStream) {  /* delete DER stream */

   if (--hDerStream < 0) return -1;
   if (!derlib_sem_init) return -1;

   sem_wait(&derlib_sem);

   if (der_streams[hDerStream].packet_save) free(der_streams[hDerStream].packet_save);

   memset(&der_streams[hDerStream], 0, sizeof(DER_STREAM));  /* clear der_streams[] struct, including in_use */

   sem_post(&derlib_sem);

   return 1;
}


int DSIsDerStream(uint8_t* pkt_in_buf, unsigned int uFlags, char* szInterceptPointId, uint16_t* dest_port) {

int pyld_ofs, pyld_len, dst_port, ret_val = 0;
int tag = 0, len = 0;

   if (pkt_in_buf && DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PROTOCOL, pkt_in_buf, -1, NULL, NULL) == TCP_PROTOCOL) {

      pyld_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDLEN, pkt_in_buf, -1, NULL, NULL);
      pyld_ofs = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDOFS, pkt_in_buf, -1, NULL, NULL);
      dst_port = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_DST_PORT, pkt_in_buf, -1, NULL, NULL);

   /* auto-detect */

      if ((uFlags & DS_ISDER_INTERCEPTPOINTID) && (!(uFlags & DS_ISDER_PORT_MUST_BE_EVEN) || !(dst_port & 1)))  {

         int i,j,k;

         for (i=0; i<pyld_len; i++) {

            tag = pkt_in_buf[pyld_ofs+i];

            if (tag == DER_TAG_INTERCEPTPOINTID) {

               bool fFound = false;
               len = (int8_t)pkt_in_buf[pyld_ofs+i+1];

               if (len > 0) {

                  for (j=0, fFound=true; j<len; j++) {
                     k = i+2+j;
                     if (k >= pyld_len || pkt_in_buf[pyld_ofs+k] <= 0x20 || pkt_in_buf[pyld_ofs+k] >= 127) { fFound = false; break; }
                  }
               }

               if (fFound) {

                  ret_val = 1;

                  if (szInterceptPointId) {
                     memcpy(szInterceptPointId, &pkt_in_buf[pyld_ofs+i+2], len);
                     szInterceptPointId[len] = 0;
                  }

                  break;  /* found valid intercept point Id, break out of i loop */
               }
            }
         }
      }

   /* intercept point Id given as input, see if we can find and verify it */

      else if (szInterceptPointId && strlen(szInterceptPointId)) {

         uint8_t* p = (uint8_t*)memmem(&pkt_in_buf[pyld_ofs], pyld_len, szInterceptPointId, strlen(szInterceptPointId));

         if (p && p - &pkt_in_buf[pyld_ofs] >= 2 && p[-2] == DER_TAG_INTERCEPTPOINTID && p[-1] == strlen(szInterceptPointId)) ret_val = 1; 
      }
   }

   if (ret_val && (uFlags & DS_ISDER_DSTPORT)) {
   
      if (dest_port) *dest_port = dst_port;
   }

   if (ret_val) Log_RT(4, "INFO: DSIsDerStream() found HI3 stream interception point %s, tag = 0x%x, len = %u, dest port = %u, pyld len = %d, pyld ofs = %d", szInterceptPointId, tag, len, dst_port, pyld_len, pyld_ofs);

   return ret_val;
}

int64_t DSGetDerStreamInfo(HDERSTREAM hDerStream, unsigned int uFlags, void* pInfo) {

   if (--hDerStream < 0) return -1;

   switch (uFlags & DS_DER_INFO_ITEM_MASK) {
   
      case DS_DER_INFO_DSTPORT:
        return der_streams[hDerStream].dest_port;

      case DS_DER_INFO_INTERCEPTPOINTID:
         if (pInfo) {
           strcpy((char*)pInfo, der_streams[hDerStream].szInterceptPointId);
           return 1;
         }

      case DS_DER_INFO_ASN_INDEX:
         return der_streams[hDerStream].asn_index;

      case DS_DER_INFO_CC_PKT_COUNT:
         return der_streams[hDerStream].cc_pkt_decode_count;
   }

   return -1;
}

int DSDecodeDerStream(HDERSTREAM hDerStream, uint8_t* pkt_in_buf, uint8_t* pkt_out_buf, unsigned int uFlags, HI3_DER_DECODE* der_decode) {

char szInterceptPointId[MAX_DER_STRLEN] = "";
uint16_t dest_port;

bool fDerStream = false, fPrint = false;
int ret_val = 0, asn_index = 0, pyld_len = -1;
int i = 1, j;

   if (--hDerStream < 0) return -1;

   if (!derlib_sem_init) return -1;

   strcpy(szInterceptPointId, der_streams[hDerStream].szInterceptPointId);
   dest_port = der_streams[hDerStream].dest_port; 

   if (der_streams[hDerStream].asn_index == 0) {

      uint16_t prot = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PROTOCOL, pkt_in_buf, -1, NULL, NULL);
      uint16_t pkt_dest_port = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_DST_PORT, pkt_in_buf, -1, NULL, NULL);

      if (prot == TCP_PROTOCOL && dest_port == pkt_dest_port) {

         if ((pyld_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDLEN, pkt_in_buf, -1, NULL, NULL)) == 0) {

            if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {
               printf("HI3 port %d NULL packet", dest_port);
               fPrint = true;
            }
            if (der_decode) der_decode->uList |= DS_DER_NULL_PACKET;
            goto ret;
         }

         fDerStream = true;
      }
   }
   else {
      asn_index = der_streams[hDerStream].asn_index;
      fDerStream = true;
   }

   if (fDerStream) {  /* DER encoded stream found */

      int pyld_ofs = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDOFS, pkt_in_buf, -1, NULL, NULL);
      if (pyld_len == -1) pyld_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDLEN, pkt_in_buf, -1, NULL, NULL);

      int save_len = der_streams[hDerStream].save_len;

      if (save_len) {
         memmove(&pkt_in_buf[pyld_ofs + save_len], &pkt_in_buf[pyld_ofs], pyld_len);
         memcpy(&pkt_in_buf[pyld_ofs], der_streams[hDerStream].packet_save, save_len);
         pyld_len += save_len;
      }

   /* scan for ETSI LI interception point */

      uint8_t* p = (uint8_t*)memmem(&pkt_in_buf[pyld_ofs + asn_index], pyld_len, szInterceptPointId, strlen(szInterceptPointId));

      if (p && p - &pkt_in_buf[pyld_ofs] >= 2 && p[-2] == DER_TAG_INTERCEPTPOINTID) {

         asn_index = (int)(p - &pkt_in_buf[pyld_ofs] - 2);  /* start index at interception point tag */
         p = &pkt_in_buf[pyld_ofs];   /* index is always offset from start of payload */

         uint8_t tag = p[asn_index];  /* interception point tag, len */
         uint8_t len = p[asn_index+1];

         if (der_decode) {

            der_decode->uList |= DS_DER_INTERCEPTPOINTID;

            der_decode->interceptionPointId.tag = tag;
            der_decode->interceptionPointId.len = len;
            strcpy(der_decode->interceptionPointId.str, szInterceptPointId);
         }


         if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {
            printf("found HI3 DER stream interception point %s, tag = 0x%x, len = %u, pyld len = %d, pyld ofs = %d", szInterceptPointId, tag, len, pyld_len, pyld_ofs);
            fPrint = true;
         }

      /* decode sequence number */ 

         if (uFlags & DS_DER_SEQNUM) {  /* only if asked for, as it occurs before interception point Id */

         /* seq number is just prior to interception point, so we reverse decode it. DER is based on type-length-value sequencing so forward decoding is a good idea, but this seems to work */

            while ((p[asn_index-i] != DER_TAG_SEQNUM || p[asn_index-i+1] > 8) && i<11) i++;  /* both sec and usec together can't be more than 11 bytes total */

            if (p[asn_index-i] == DER_TAG_SEQNUM) {

               uint8_t seq_num_tag = p[asn_index-i];
               uint8_t seq_num_len = p[asn_index-i+1];
               uint64_t seq_num = 0;
               for (j=0; j<seq_num_len; j++) seq_num = (seq_num << 8) | (uint64_t)p[asn_index-i+2+j];

               if (der_decode) {

                  der_decode->uList |= DS_DER_SEQNUM;

                  der_decode->sequenceNumber.tag = seq_num_tag;
                  der_decode->sequenceNumber.len = seq_num_len;
                  der_decode->sequenceNumber.value = seq_num;
               }

               if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {

               /* debug code to verify no missing sequence numbers. num_miss (number of misses) should stay zero */
                  static int prev_seq_num = -1;
                  static int num_miss = 0;
                  if ((int)seq_num-1 != prev_seq_num) num_miss++;
                  prev_seq_num = (int)seq_num;

                  printf(", found seq num %llu, tag = 0x%x, len = %d, num_miss = %d", (unsigned long long)seq_num, seq_num_tag, seq_num_len, num_miss);
               }
            }
         }

         asn_index += strlen(szInterceptPointId) + 2;

      /* decode timestamp */

         tag = p[asn_index++];  /* tag includes DER_TAG_CLASS_CONSTRUCT attribute, which indicate a construct with multiple parts. We know from ETSI standard that timestamp has 2 parts, sec and usec */
         if ((tag & 0x1f) == 31) tag = p[asn_index++];  /* tag number > 30, need to read another tag */
         len = p[asn_index++];  /* length of both parts, including sub tags and sub lengths */

         int sub_tag1 = p[asn_index++];  /* sec tag */
         int sub_len1 = p[asn_index++];
         int timestamp_sec_index = asn_index;

         asn_index += sub_len1;

         int sub_tag2 = p[asn_index++];  /* usec tag */
         int sub_len2 = p[asn_index++];
         int timestamp_usec_index = asn_index;

         asn_index += sub_len2;

         if ((uFlags & DS_DER_TIMESTAMP) && tag == DER_TAG_TIMESTAMP) {

            uint64_t timestamp_sec = 0;
            int32_t timestamp_usec = 0;

            for (i=0; i<sub_len1; i++) timestamp_sec = (timestamp_sec << 8) | (uint64_t)p[timestamp_sec_index+i];  /* sec since 1970 */
            for (i=0; i<sub_len2; i++) timestamp_usec = (timestamp_usec << 8) | (uint32_t)p[timestamp_usec_index+i];  /* usec */

            if (der_decode) {

               der_decode->uList |= DS_DER_TIMESTAMP;

               der_decode->timeStamp.tag = tag;
               der_decode->timeStamp.len = len;
               der_decode->timeStamp.value = 0;
               der_decode->timeStamp_sec.tag = sub_tag1;
               der_decode->timeStamp_sec.len = sub_len1;
               der_decode->timeStamp_sec.value = timestamp_sec;
               der_decode->timeStamp_usec.tag = sub_tag2;
               der_decode->timeStamp_usec.len = sub_len2;
               der_decode->timeStamp_usec.value = timestamp_usec;
            }

            if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {
               printf(", found timestamp sec %llu, usec = %d, tag = 0x%x, len = %u, len1 = %u, len2 = %u", (unsigned long long)timestamp_sec, timestamp_usec, tag, len, sub_len1, sub_len2);
            }
         }

      /* decode timestamp qualifier */

         tag = p[asn_index++];
         if ((tag & 0x1f) == 31) tag = p[asn_index++];  /* tag number > 30, need to read another tag */
         len = p[asn_index++];

         if ((uFlags & DS_DER_TIMESTAMPQUALIFIER) && tag == DER_TAG_TIMESTAMPQUALIFIER) {

            uint32_t timeStampQualifier = 0;

            for (i=0; i<len; i++) timeStampQualifier = (timeStampQualifier << 8) | (uint32_t)p[asn_index+i];

            if (der_decode) {

               der_decode->uList |= DS_DER_TIMESTAMPQUALIFIER;

               der_decode->timeStampQualifier.tag = tag;
               der_decode->timeStampQualifier.len = len;
               der_decode->timeStampQualifier.value = timeStampQualifier;
            }

            if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {
               printf(", found timeStampQualifier = %u, tag = 0x%x, len = %u", timeStampQualifier, tag, len);
            }
         }

         asn_index += len;

         if (uFlags & DS_DER_CC_PACKET) {  /* if asked for decode CC packet */

         /* locate encapsulated IP headers using checksum matching, notes:

            -we don't look for ccXX items (ccPayloadSequence, ccContents, iPCC, etc. These seem to be more trouble than they're worth
            -slide byte at a time, calculate and compare checksum per IP header standard. Verify packet header integrity in addition to checksum match
            -always make sure to not exceed available encapsulated TCP packet payload length
         */

            uint16_t checksum_candidate, checksum;

            while (asn_index < pyld_len) {

               checksum = ((uint16_t)p[asn_index + 11] << 8) | p[asn_index + 10];  /* 10 = byte offset of checksum in IPv4 header */
               checksum_candidate = calc_checksum(&p[asn_index], 10, 5, 16);  /* pointer to data, num of elements, omit index, num of bits per element */

               if ((uint16_t)~checksum_candidate == checksum) {  /* compare checksums, 1's complement */

                  uint8_t* p2 = &p[asn_index];
                  int pktlen = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTLEN | DS_PKT_INFO_SUPPRESS_ERROR_MSG, p2, -1, NULL, NULL);

                  if (pktlen < 0) goto next_byte;  /* if packet header values are bad, assume checksum hash matched wrong data. Happens every so often */

                  int rtp_pyld_type = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_RTP_PYLDTYPE, p2, -1, NULL, NULL);

                  if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {
                     printf(", found IP header, asn_index = %d, tag = 0x%x, len = %d, pkt len = %d, RTP pyld type = %d", asn_index, p2[-2], p2[-1], pktlen, rtp_pyld_type);
                     fPrint = true;
                  }

                  if (der_decode) {

                     der_decode->uList |= DS_DER_CC_PACKET;

                     der_decode->cc_packet.tag = p[asn_index-2];
                     der_decode->cc_packet.len = p[asn_index-1];
                  }

                  ret_val = pktlen;

                  if (pkt_out_buf) memmove(pkt_out_buf, &p[asn_index], pktlen);  /* fully extracted output packet */

                  asn_index += pktlen;  /* advance to end of found packet */

                  der_streams[hDerStream].asn_index = asn_index;

                  der_streams[hDerStream].cc_pkt_decode_count++;

                  if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {
                     printf(", after CC packet decode %llu asn_index = %d", (unsigned long long)der_streams[hDerStream].cc_pkt_decode_count, asn_index);
                  }

                  break;
               }
next_byte:
               asn_index++;  /* advance one byte */
            }
         }
 
      /* handle aggregated packets, notes:

         -assume this is an aggregated packet after some arbitrarily large amount of data (i.e. a lot larger than even large codec packet with multiple ptimes)
         -if we don't land on exactly on end, we need to save data and insert at start of next packet
      */

         if (asn_index > pyld_len - 500 && asn_index < pyld_len) {

            der_streams[hDerStream].save_len = pyld_len - asn_index;

            if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {
               printf(", aggregated end, save len = %d", der_streams[hDerStream].save_len);
               fPrint = true;
            }

            memcpy(der_streams[hDerStream].packet_save, &p[asn_index], der_streams[hDerStream].save_len);
            der_streams[hDerStream].asn_index = 0;
         }
         else if (asn_index == pyld_len) {

            if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {
               printf(", exact end");
               fPrint = true;
            }

            der_streams[hDerStream].save_len = 0;
            der_streams[hDerStream].asn_index = 0;
         }
         else if (asn_index > pyld_len) {

            if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {
               printf(" exceeds pyld_len %d", pyld_len);
               fPrint = true;
            }

            der_streams[hDerStream].save_len = 0;
            der_streams[hDerStream].asn_index = 0;
         }
      }
   }

ret:

   if (der_decode) {

      if (!der_decode->uList) der_streams[hDerStream].asn_index = 0;
      der_decode->asn_index = der_streams[hDerStream].asn_index;
   }

   if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {
      if (fPrint) printf(" \n");
   }

   return ret_val;
}

static uint16_t calc_checksum(void* p, int num_elements, int omit_index, int element_size_bits) {

uint16_t checksum16 = 0;
uint8_t checksum8 = 0;
uint32_t long_checksum;

uint16_t* p16 = (uint16_t*)p;
uint8_t* p8 = (uint8_t*)p;
int i;

   for (i=0; i<num_elements; i++) {

      if (i == omit_index) continue;  /* don't include checksum if omit_index is specified. -1 will include checksum */

      if (element_size_bits == 16) {

         long_checksum = checksum16;
         long_checksum += p16[i];
         if (long_checksum > 65535) checksum16 = (uint16_t)(long_checksum + 1);
         else checksum16 = (uint16_t)long_checksum;
      }
      else if (element_size_bits == 8) {
      
         long_checksum = checksum8;
         long_checksum += p8[i];
         if (long_checksum > 255) checksum8 = (uint8_t)(long_checksum + 1);
         else checksum8 = (uint8_t)long_checksum;
      }
   }

   if (element_size_bits == 16) return checksum16;
   else return checksum8;
}
