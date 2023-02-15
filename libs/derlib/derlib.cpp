/*
 $Header: /root/Signalogic/Directcore/lib/derlib/derlib.cpp

 Copyright (C) Signalogic Inc. 2021-2022

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

 Description

  DER decoding library and APIs, supporting:

  -fully abstracted, generic decoding of DER encoded packets, with no requirement for "a-priori" ASN.1 format knowledge or double-pass, non-real-time, or other batch processing. Only need is to provide TCP/IP packets as they are received
  -aggregated packets (i.e. content split across multiple TCP/IP packets)
  -multiple concurrent streams with no spinlocks outside of DSCreateDerStream() and DSDeleteDerStream()
  -missing or wrong packet arrival timestamps. In that case use mediaMin's "analytics mode"

 Purpose
 
  Decode ETSI LI HI2 and HI3 DER encoded streams in real-time without need for ASN.1 compiler. Experience shows that ASN.1 formats tend to vary widely and in many cases are incorrectly documented and/or not well supported / maintained by either CSPs / operators or LEAs
 
 Revision History

  Created Feb 2021 JHB
  Modified Mar 2021 JHB, fixes for testing with OpenLI captures
  Modified May 2021 JHB, add support for HI3 IPv6 packets, multiple destination ports per interception Id (e.g. port A HI2, port B HI3, port C HI3), test with encapsulated HI2 and HI3 streams from customers in Japan
  Modified May 2021 JHB, make dest_ports[] in DER_STREAM struct an array that represents a list of ports
                         -add DS_DER_INFO_DSTPORT_LIST to get/set the list in DSGetDerStreamInfo() and DSSetDerStreamInfo()
                         -change operation of DS_DER_INFO_DSTPORT to get/set a specific port in DSGetDerStreamInfo() and DSSetDerStreamInfo()
  Modified Dec 2021 JHB, continue to improve DSFindDerStream() flexibility in recognizing different LEA HI3 formats, both DER and BER
  Modified Feb 2022 JHB, undefine IPV4DEBUG and IPV6DEBUG
  Modified Nov 2022 JHB, additional comments for event INFO log message about interception point ID type and content. This INFO log message has proven useful for evaluating LEA user screen traces and event logs
  Modified Dec 2022 JHB, add DSDecodeDerFields() to generate XML output from DER streams in formats (i) DER tag/set notation (ii) ASN.1 notation (per ETSI LI ASN.1 specs)
  Modified Jan 2023 JHB, change DS_PKT_INFO_SUPPRESS_ERROR_MSG to generic DS_PKTLIB_SUPPRESS_ERROR_MSG. See comments in pktlib.h
*/

/* Linux includes */

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>

  #include <unistd.h>

/* Sig includes */

#include "pktlib.h"
#include "voplib.h"
#include "diaglib.h"  /* make available Log_RT() */
#include "derlib.h"
#include "minmax.h"

#include "shared_include/config.h"  /* configuration structs and definitions */

const char DERLIB_VERSION[256] = "1.2.1";

typedef struct {

  uint8_t in_use;  /* flag indicating whether der_streams[] stream is in use */

  char      szInterceptPointId[MAX_DER_STRLEN];
  uint16_t  dest_ports[MAX_DER_DSTPORTS];
  uint8_t*  packet_save;
  int       save_len;
  int       asn_index;
  uint64_t  cc_pkt_decode_count;

} DER_STREAM;

static DER_STREAM der_streams[MAX_DER_STREAMS] = {{ 0 }};

static uint16_t calc_checksum(void* p, uint16_t checksum_init, int num_bytes, int omit_index, int checksum_width);

static sem_t derlib_sem;
static bool derlib_sem_init = false;
static int stream_index = 0;

typedef struct {

   uint16_t dst_port;
   uint8_t* pBuffer;  /* packet aggregation buffer */
   int chunk_len;

} PORT_INFO;

static PORT_INFO port_info[MAX_DER_DSTPORTS] = { 0 };

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

      if (!derlib_sem_init) {  /* initialize derlib semaphore if needed */
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

   do {

      if (der_streams[i].in_use == 0) {  /* see if in_use flag indicates this stream is available */

         der_streams[i].in_use = 1;  /* set in_use flag for this stream */

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

uint8_t isSetTag(uint8_t tag, unsigned int uFlags, uint8_t* p, int* index, FILE* hFile, int* ofs) {

uint8_t set_tag = 0, tag2 = 0;
int ofs_local = 0, *pOfs;
char linestr[100];

   if (!ofs) pOfs = &ofs_local;
   else pOfs = ofs;
 
   if ((tag & 0x1f) == 31) {

      tag2 = p[*index + (*pOfs)++];  /* tag == 31 is "long form" (2 byte tag) */

      sprintf(linestr, " *** info: long form tags 0x%x 0x%x \n", tag, tag2);
      if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);
   }

   if ((tag & (DER_TAG_CLASS_CONSTRUCT << 5)) || (tag2 & (DER_TAG_CLASS_CONSTRUCT << 5))) {  /* set tag ? */

      if (tag & (DER_TAG_CLASS_CONSTRUCT << 5)) set_tag = tag;

      if (tag2 & (DER_TAG_CLASS_CONSTRUCT << 5)) {

         if (!set_tag) set_tag = tag2;
         else {
            sprintf(linestr, " *** error: consecutive long form tags 0x%x 0x%x \n", tag, tag2);
            if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);
            return DECODE_FIELDS_ERROR_CONSEC_LONGFORM_TAGS;
         }
      }
   }

   return set_tag;
}

/* local function to decode a DER field or set of subfields, including nested sets:

  -pointer to buffer (e.g. a packet payload)
  -uFlags - see DS_DER_DECODEFIELDS_xxx definitions in derlib.h
  -index into buffer
  -optional XML file handle
  -optional tag label, mainly useful for individual fields
  -recursion level 0..N (0 = top level)
  -return set length if applicable (0 indicates no set), -1 on error condition
*/

int decode_der_field(uint8_t* p, unsigned int uFlags, int* index, int buflen, FILE* hFile, const char* label, int level, bool fInSet) {

#define MAX_FIELD_LEN 2047  /* bigger than max MTU packet size, hopefully */
#define ITER_LIMIT 100

int i, len, set_len = 0, iter_limit = 0, ofs = 0;
uint8_t tag, set_tag = 0;
char linestr[MAX_FIELD_LEN+100], indentstr[500];

   tag = p[*index + ofs++];

   if ((set_tag = isSetTag(tag, uFlags, p, index, hFile, &ofs))) {

      set_len = p[*index + ofs++];  /* set length */

      if (set_len <= 0) {
         sprintf(linestr, " *** error: set len %d assignment <= zero *index = %d, \n", *index, set_len);
         if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);
         return DECODE_FIELDS_ERROR_NEGATIVE_SETLEN;
      }

      if (set_len & 0x80) {  /* long form set length, first octet contains number of length octets */

         if ((set_tag & 0x0f) == DER_TAG_NULL) {

            sprintf(linestr, " *** info: NULL set tag 0x%x \n", set_tag);
            if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);

            set_len = 0;
         }
         else {

            int num_octets = set_len & 0x7f;

            sprintf(linestr, " *** info: set tag 0x%x has long form len 0x%x, num octets = %d \n", set_tag, set_len, num_octets);
            if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);

            for (set_len=0, i=0; i<num_octets; i++) set_len = (set_len << 8) | (unsigned int)p[*index + ofs++];
         }
      }

      if (set_len >= MAX_FIELD_LEN) {
         sprintf(linestr, " *** error: set len %d >= max field len %d \n", set_len, MAX_FIELD_LEN);
         if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);
         return DECODE_FIELDS_ERROR_SETLEN_EXCEEDS_MAX;
      }
 
      strcpy(indentstr, "");  /* indentstr contains asn output indent level */
      for (i=0; i<level; i++) strcat(&indentstr[strlen(indentstr)], "  ");

      sprintf(linestr, "%s%s%sset tag = 0x%x len = %d \n", indentstr, label ? label : "", label ? " " : "", set_tag, set_len);
      if (hFile) fwrite(linestr, 1, strlen(linestr), hFile);

      if (set_len > 0) {

         if (buflen > 0 && *index + ofs > buflen) {  /* buflen == 0 is case of decoding a single field with unknown length */

            if (ofs == 2)
              sprintf(linestr, " *** info: buffer ends with set tag 0x%x with len %d and no contents, *index %d + ofs %d > %d \n", set_tag, set_len, *index, ofs, buflen);
            else
              sprintf(linestr, " *** error: buffer ends wih set tag 0x%x with len %d, *index %d + ofs %d > %d \n", set_tag, set_len, *index, ofs, buflen);

            if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);

         /* return in any case, as we can't exceed buffer mem */

            if (ofs == 2) return 0;
            else return DECODE_FIELDS_ERROR_EXCEEDS_BUFLEN1;
         }

         *index += ofs;  /* valid set: advance index accordingly */
      }
   }

   if (fInSet && set_len <= 0) return 0;  /* check only: next field not a set, return and allow prior level to continue */

   int set_len_save = set_len > 0 ? set_len + ofs : 0;

   do {

      int set_check_limit = 0;

set_chk:

      int ret_val = decode_der_field(p, uFlags, index, buflen, hFile, label, level+1, true);  /* recursive call, check for nested set. ret_val is zero if none */

      if (ret_val < 0) return ret_val;  /* error condition */

      if (buflen > 0 && *index >= buflen) return 0;  /* already reached end of buffer, don't exceed mem size under any circumstance */

      set_len -= ret_val;  /* reduce set length by nested set length, if any (including its tag and len bytes) */

   /* decode a field */

      tag = p[*index];

      if (isSetTag(tag, uFlags, p, index, hFile, NULL)) {

         sprintf(linestr, " *** info: level %d unwinds to set tag 0x%x, *index = %d, set_len = %d, set_len_save = %d, prior set_len_save = %d \n", level, tag, *index, set_len, set_len_save, ret_val);
         if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);

         if (set_check_limit++ >= ITER_LIMIT) {
            sprintf(linestr, " *** error: iteration 1 limit exceeded \n");
            if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);
            return DECODE_FIELDS_ERROR_EXCEEDS_ITER_LIMIT1;
         }
         else goto set_chk;
      }

      int ofs2 = 0;

      tag = p[*index + ofs2++];
      len = p[*index + ofs2++];

      if (len < 0) {
         bool fError = false;
         if (tag == 0 && len == 0)
           sprintf(linestr, " *** info: EOC tag and length == zero, *index = %d \n", *index + ofs2);
         else
           { sprintf(linestr, " *** error: len < zero, *index = %d, len = %d \n", *index + ofs2, len); fError = true; }
         if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);
         if (fError) return DECODE_FIELDS_ERROR_NEGATIVE_TAGLEN;
      }
 
      if (len & 0x80) {  /* long form length, first octet contains number of length octets */

         if ((tag & 0x0f) == DER_TAG_NULL) {

            sprintf(linestr, " *** info: NULL tag 0x%x \n", tag);
            if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);

            len = 0;
         }
         else {
            int num_octets = len & 0x7f;

            sprintf(linestr, " *** info: tag 0x%x has long form len 0x%x, num_octets = %d \n", tag, len, num_octets);
            if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);

            for (len=0, i=0; i<num_octets; i++) len = (len << 8) | (unsigned int)p[*index + ofs2++];
         }
      }

      if (len >= MAX_FIELD_LEN) {
         sprintf(linestr, " *** error: tag len %d >= max field len %d \n", len, MAX_FIELD_LEN);
         if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);
         return DECODE_FIELDS_ERROR_TAGLEN_EXCEEDS_MAX;
      }

      char chr, valstr[MAX_FIELD_LEN+1], contentstr[MAX_FIELD_LEN+1] = { (char)0 };
      bool fString = true, fInteger = false, fBytes = false;
      uint64_t val = 0;

      for (i=0; i<len; i++) {
         contentstr[i] = (chr = p[*index+ofs2+i]);
         if (((chr < 0x20 || chr >= 127)) && chr != 0x0a && chr != 0x0d) fString = false;
      }

      if (!fString /* || (tag & 0x3f) == DER_TAG_INTEGER */ || (tag & 0x3f) == DER_TAG_OCTETSTRING) {  /* INTEGER seems to be actually a string type */

         if (len > 8) {  /* currently we limit integers to 8 bytes (uint64_t)  ... do we need to allow 12 or 16-byte integers ? */

            sprintf(linestr, " *** info: integer value len %d \n", len);
            if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);

            for (i=0; i<len; i++) sprintf(&valstr[strlen(valstr)], "%x", p[*index+ofs2+i]);
            fBytes = true;
         }
         else {

            for (i=0; i<len; i++) val = (val << 8) | (uint64_t)p[*index+ofs2+i];
            sprintf(valstr, "%llu", (unsigned long long)val);
            fInteger = true;
         }
      }

      strcpy(indentstr, "");  /* indentstr contains asn output indent level */
      if (set_len) strcat(&indentstr[strlen(indentstr)], "  ");
      for (i=0; i<level; i++) strcat(&indentstr[strlen(indentstr)], "  ");

      sprintf(linestr, "%s%s%stag = 0x%x len = %d%s(%d) %s \n", indentstr, label ? label : "", label ? " " : "", tag, len, fInteger ? "i" : (fBytes ? "b" : "s"), set_len, (fInteger || fBytes) ? valstr : contentstr);
      if (hFile) fwrite(linestr, 1, strlen(linestr), hFile);

      if (set_len > 0) {  /* yet to reach end of set */

         if (buflen > 0 && *index + len + ofs2 > buflen) {  /* buflen == 0 is case of decoding a single field with unknown length */

            sprintf(linestr, " *** error: buffer ends with tag with len %d within set with len %d, *index %d + len %d + ofs2 %d > %d \n", len, set_len, *index, len, ofs2, buflen);
            if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);
            return DECODE_FIELDS_ERROR_EXCEEDS_BUFLEN2;
         }

         set_len -= len + ofs2;  /* reduce set length */

         if (buflen > 0 && set_len < 0) {
            sprintf(linestr, " *** error: set len %d subtraction < zero \n", set_len);
            if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);
            return DECODE_FIELDS_ERROR_NEGATIVE_SETLEN;
         }

         *index += len + ofs2;  /* advance input index */
      }

   } while (set_len > 0 && ++iter_limit < ITER_LIMIT);  /* iter_limit -- make sure we're not stuck here for any reason, including bad DER encoding */

   if (iter_limit >= ITER_LIMIT) {

      sprintf(linestr, " *** error: iteration 2 limit exceeded \n");
      if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(linestr, 1, strlen(linestr), hFile);
      return DECODE_FIELDS_ERROR_EXCEEDS_ITER_LIMIT2;  /* don't get stuck under any condition -- we can end up writing a huge, never-ending file */
   }

   return set_len_save;  /* return prior set length, if any */
}

/* DSDecodeDerFields(), decode one or more DER fields:

  -see usage and param comments in derlib.h

  -data chunk aggregation is used for large DER data chunks split across packet boundaries. Currently this can handle around 4x max size TCP/IP packets

  -to-do: instead of the Nx packet limitation, a sliding method is needed for continuous streams of max size packets, similar to the the method used in DSDecodeDerStream(). One possible idea is to check inside the while loop for return to top level, and if above some threshold then save remaining unprocessed data, and wait for new data. Definitely decode_der_field() recursion adds some difficulty -- there is no good (high performance) way to stop in the middle of that and wait for more input
*/

#define MAX_DER_BUFFER_SIZE  1448

int DSDecodeDerFields(uint8_t* p, unsigned int uFlags, int plen, FILE* hFile, const char* label) {

int index = 0, ret_val = 0;  /* input byte index, advanced by decode_der_field() */
int i, port_index = 0, ofs = 0;
bool fProcessASN = true;

   if (!p) {
      Log_RT(2, "ERROR: DSDecodeDerFields() says NULL input packet or buffer pointer, uFlags = 0x%x \n", uFlags);
      return -1;
   }

   if (!(uFlags & DS_DER_DECODEFIELDS_BUFFER)) {

      unsigned int dst_port;
      int pyld_len;

   /* get packet dest port, payload length and offset */

      if ((int)(dst_port = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_DST_PORT, p, -1, NULL, NULL)) <= 0) {

         Log_RT(2, "ERROR: DSDecodeDerFields() says input packet has invalid destination port, uFlags = 0x%x \n", uFlags);
         return -1;
      }

      if ((pyld_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDLEN, p, -1, NULL, NULL)) <= 0) {
         Log_RT(2, "ERROR: DSDecodeDerFields() says input packet invalid payload length, uFlags = 0x%x \n", uFlags);
         return -1;
      }

      ofs = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDOFS, p, -1, NULL, NULL);

      plen = min(plen, pyld_len);  /* don't exceed payload size */

   /* update list of ports */

      for (i=0; i<MAX_DER_DSTPORTS; i++) {

         if (dst_port == port_info[i].dst_port) break;  /* port already listed ? */
         else if (!port_info[i].dst_port) {
            port_info[i].dst_port = dst_port;  /* add port to list */
            break;
         }
      }

      if (i == MAX_DER_DSTPORTS) return -1;

      port_index = i;

      if (plen >= MAX_DER_BUFFER_SIZE || port_info[port_index].chunk_len) {  /* aggregate packet data chunks if needed */

         if (!port_info[port_index].chunk_len) port_info[port_index].pBuffer = (uint8_t*)calloc(4*MAX_RTP_PACKET_LEN, sizeof(uint8_t));  /* allocate packet aggregation buffer */

         memcpy(&port_info[port_index].pBuffer[port_info[port_index].chunk_len], &p[ofs], plen);  /* add to buffer */
         port_info[port_index].chunk_len += plen;

         if (plen >= MAX_DER_BUFFER_SIZE) fProcessASN = false;  /* wait for packet size less than max to process. To-do: come up with a check for case where packet is not split but has max size */
      }

      if (fProcessASN && !(uFlags & DS_DER_DECODEFIELDS_BUFFER)) {
         char tmpstr[100];
         sprintf(tmpstr, " *** asn input port = 0x%x, pyld_len = %d, plen = %d \n", dst_port, pyld_len, plen);
         if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) fwrite(tmpstr, 1, strlen(tmpstr), hFile);
      }
   }

   if (fProcessASN) {

      uint8_t* pBuffer = (!(uFlags & DS_DER_DECODEFIELDS_BUFFER) && port_info[port_index].chunk_len) ? port_info[port_index].pBuffer : &p[ofs];
      int buflen = (!(uFlags & DS_DER_DECODEFIELDS_BUFFER) && port_info[port_index].chunk_len) ? port_info[port_index].chunk_len : plen;

   /* process DER one or more encoded fields. Notes:

      -decode_der_field() returns here after each top level tag
      -top level tags may contain nested set tags, in which case decode_der_field() recurses one or more levels before returning
      -while loop continues until either 1) plen bytes are processed or 2) an error condition occurs (ret_val < 0)
   */

      int iter_limit = 0;

      do {

         #if 0  /* all possible tag values allowed, including zero tags (EOCs) */
         uint8_t tag;
         while ((tag = pBuffer[index]) < 0x30 || tag > 0xaf) index++;  /* on entry should be pointing at a valid tag, if not keep reading */
         #endif

         int index_save = index;

         ret_val = decode_der_field(pBuffer, uFlags, &index, buflen, hFile, label, 0, false);  /* upon return index contains amount of data processed */

      /* check for error condition returned by decode_der_field() or possible stuck-in-loop situation. During debug we've seen never-ending file write situations due to getting stuck, so don't let that happen */

         if (ret_val < 0) Log_RT(2, "ERROR: DSDecodeDerFields() says decode_der_field() returns error condition %d, index = %d \n", ret_val, index);
         else if (index < 0 || ((index_save - index) == 0 && iter_limit++ >= ITER_LIMIT)) {

            Log_RT(2, "ERROR: DSDecodeDerFields() exceeds iteration limit, index = %d \n", index);
            ret_val = -1;
         }

      /* reduce specified bytes by amount processed by decode_der_fields() */
 
         if (plen > 0) plen -= index;

      } while (plen > 0 && ret_val >= 0);

      if (hFile && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) {

         char tmpstr[100];
         if (index > buflen)
           sprintf(tmpstr, " *** error: asn bytes processed %d > buflen %d \n", index, buflen);
         else
           sprintf(tmpstr, " *** asn bytes processed %d vs buflen %d \n", index, buflen);
         fwrite(tmpstr, 1, strlen(tmpstr), hFile);
      }

      if (!(uFlags & DS_DER_DECODEFIELDS_BUFFER)) {

         if (port_info[port_index].pBuffer) free(port_info[port_index].pBuffer);  /* free packet aggregation buffer */
         port_info[port_index].pBuffer = NULL;
         port_info[port_index].chunk_len = 0;
      }
   }

   if (ret_val < 0) return ret_val;  /* error condition */

   return index;  /* return bytes processed */
}

/* create a DER stream and return its handle */

HDERSTREAM DSCreateDerStream(const char* szInterceptPointId, uint16_t dest_port, unsigned int uFlags) {

   (void)uFlags;

/* check for error conditions */

   if (!derlib_sem_init) return -1;  /* derlib semaphore used by get_next_stream_id() */
   if (!szInterceptPointId || !strlen(szInterceptPointId)) return -1;
   if (!dest_port) return -1;

/* allocated new stream handle */

   int stream_index = get_next_stream_id();

   if (stream_index < 0) return -1;

/* init new stream */

   strcpy(der_streams[stream_index].szInterceptPointId, szInterceptPointId);
   der_streams[stream_index].dest_ports[0] = dest_port;
   der_streams[stream_index].packet_save = (uint8_t*)malloc(MAX_RTP_PACKET_LEN);  /* create mem used to aggregate DER encoded items split across packet payload boundaries */

   return stream_index + 1;  /* when apps check for a valid stream handle, anything <= 0 is invalid */
}

int DSDeleteDerStream(HDERSTREAM hDerStream) {  /* delete DER stream */

   if (--hDerStream < 0) return -1;
   if (!derlib_sem_init) return -1;

   if (der_streams[hDerStream].packet_save) free(der_streams[hDerStream].packet_save);  /* free memory used by this stream */

   sem_wait(&derlib_sem);  /* obtain semaphore */

   memset(&der_streams[hDerStream], 0, sizeof(DER_STREAM));  /* clear der_streams[] struct, including in_use flag */

   sem_post(&derlib_sem);  /* release semaphore */

   return 1;
}


/* DSFindDerStream() - detect if packet contains a DER encapsulated HI2 or HI3 stream

   -find interception point Id
   -also detects additional ports for already existing interception point Id
   -user can set uFlags for auto-detection of interception point ID and/or dest_port
*/ 

int DSFindDerStream(uint8_t* pkt_in_buf, unsigned int uFlags, char* szInterceptPointId, uint16_t dest_port_list[], FILE* hFile_asn_output) {

int i, pyld_ofs, pyld_len, ret_val = 0;
uint16_t dst_port;
int tag = 0, len = 0, port_list_index = -1, generic_string_count = 0;
char szInterceptionIdentifier[256] = "";
char szAuthCountryIdentifier[128] = "";

#ifdef INTERCEPTPOINTDEBUG
static bool fOnce = false;
#endif

   if (pkt_in_buf && DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PROTOCOL, pkt_in_buf, -1, NULL, NULL) == TCP_PROTOCOL) {

   /* get packet's dest port */

      if ((int)(dst_port = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_DST_PORT, pkt_in_buf, -1, NULL, NULL)) <= 0) return 0;

   /* if caller provides port list and packet matches a port already on the list then nothing to do. Ports are listed once an interception point ID is found; see ret_val below */
 
      if (dest_port_list) for (i=0; i<MAX_DER_DSTPORTS; i++) if (dest_port_list[i] == dst_port) return 0;

   /* get packet's payload length and offset */
 
      if (!(pyld_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDLEN, pkt_in_buf, -1, NULL, NULL))) return 0;
      pyld_ofs = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDOFS, pkt_in_buf, -1, NULL, NULL);

   /* decode asn and write to file if requested */

      if (hFile_asn_output) DSDecodeDerFields(pkt_in_buf, DS_DER_DECODEFIELDS_PACKET | DS_DER_DECODEFIELDS_OUTPUT_ASN | (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO), pyld_len, hFile_asn_output, "find gen asn");

   /* auto-detect */

      if ((uFlags & DS_DER_FIND_INTERCEPTPOINTID) && (!(uFlags & DS_DER_FIND_PORT_MUST_BE_EVEN) || !(dst_port & 1)))  {

         for (i=0; i<pyld_len; i++) {  /* search full payload until intercept point ID found. To-do-maybe: do we need packet boundary aggregation as in DSDecodeDerStream() ? */

            uint8_t tag_chk = pkt_in_buf[pyld_ofs+i];

            if (tag_chk == DER_TAG_INTERCEPTPOINTID || (tag_chk >= 0x80 && tag_chk <= 0x82)) {

               #ifdef INTERCEPTPOINTDEBUG
               printf("\n $$$$ index = %d, dst port = %d \n", i, dst_port);
               #endif

               bool fValidTagFound = false;
               int j, k = 0, len_chk = (int8_t)pkt_in_buf[pyld_ofs+i+1];

               if (len_chk > 0) for (j=0, fValidTagFound=true; j<len_chk; j++) {
                  k = i+2+j;
                  if (k >= pyld_len || pkt_in_buf[pyld_ofs+k] <= 0x20 || pkt_in_buf[pyld_ofs+k] >= 127) { fValidTagFound = false; break; }
               }

               if (fValidTagFound) {

                  tag = tag_chk;
                  len = len_chk;

                  #ifdef INTERCEPTPOINTDEBUG
                  printf("\n $$$$ found valid tag = 0x%x, len = %d \n", tag, len);
                  #endif

                  if (tag == 0x80) {

                    generic_string_count++;  /* count valid generic string tags */
                  }
                  else if (tag == 0x81) {

                     memcpy(szInterceptionIdentifier, &pkt_in_buf[pyld_ofs+i+2], len);
                     szInterceptionIdentifier[len] = 0;
                  }
                  else if (tag == 0x82) {

                     #if 0  /* test case for buffer input with size 0 indicating unknown data length */
                     DSDecodeDerFields(&pkt_in_buf[pyld_ofs+i], DS_DER_DECODEFIELDS_BUFFER | DS_DER_DECODEFIELDS_OUTPUT_ASN, 0, hFile_asn_output, "country identifier");
                     #endif

                     #ifdef INTERCEPTPOINTDEBUG
                     printf("\n $$$$ attempting country code, len = %d \n", len);
                     if (!fOnce) { usleep(1000000); fOnce = true; }
                     #endif

                     len = min(len, (int)sizeof(szAuthCountryIdentifier)-1);
                     memcpy(szAuthCountryIdentifier, &pkt_in_buf[pyld_ofs+i+2], len);
                     szAuthCountryIdentifier[len] = 0;
                     
  
                  }
                  else if (tag == DER_TAG_INTERCEPTPOINTID) {

                     ret_val = 1;

                     if (szInterceptPointId) {

                        memcpy(szInterceptPointId, &pkt_in_buf[pyld_ofs+i+2], len);
                        szInterceptPointId[len] = 0;
                     }

                     break;  /* found valid intercept point Id, break out of payload loop */
                  }
               }
            }
         }  /* end of payload loop */

      /* some countries don't use Intercept Point Id; for those, if we can't find one then we use LI Identifier as the Id. Conditions for that are (i) valid country identifier found, or (ii) valid generic text strings found above some threshold. Notes: JHB Dec 2021

         -tag count threshold (ret_val == 3) works for some customers
         -HIx streams from Japan differ from ETSI LI standard
      */ 

         if (!ret_val && strlen(szInterceptionIdentifier) && (generic_string_count >= 3 || !strcmp(szAuthCountryIdentifier, "JP"))) {

            ret_val = !strcmp(szAuthCountryIdentifier, "JP") ? 2 : 3;  /* ret_val = 3 unless country identifier found, then ret_val = 2, JHB Nov 2022 */

            if (szInterceptPointId) strcpy(szInterceptPointId, szInterceptionIdentifier);
         }
      }

   /* intercept point Id given as input, see if we can find and verify it */

      else if (szInterceptPointId && strlen(szInterceptPointId)) {

         uint8_t* p = (uint8_t*)memmem(&pkt_in_buf[pyld_ofs], pyld_len, szInterceptPointId, strlen(szInterceptPointId));

         if (p && p - &pkt_in_buf[pyld_ofs] >= 2 && p[-2] == DER_TAG_INTERCEPTPOINTID && p[-1] == strlen(szInterceptPointId)) ret_val = 1; 
      }
   }

   if (ret_val && (uFlags & DS_DER_FIND_DSTPORT)) {
   
      if (dest_port_list) {

         for (i=0; i<MAX_DER_DSTPORTS; i++) {  /* add dest port to next available list slot */
            if (!dest_port_list[i]) {
               dest_port_list[i] = dst_port;
               port_list_index = i;
               break;
            }
         }
      }
   }

   if (ret_val) {  /* print event log message indicating DSFindDerStream() results, including type of interception point ID */

      if (hFile_asn_output && (uFlags & DS_DECODE_DER_PRINT_ASN_DEBUG_INFO)) {

         char linestr[256] = "*** asn interception point ID  \n";
         fwrite(linestr, 1, strlen(linestr), hFile_asn_output);
       }

      char msgstr[100] = "found";
      char id_type_str[100] = "HI interception point ID";

      if (port_list_index > 0) strcat(msgstr, " additional port for");

      if (ret_val == 2) strcat(id_type_str, " (country identifier)");  /* see comments above where ret_val is assigned. ret_val == 1 is default; i.e. normal interception point ID */
      else if (ret_val == 3) strcat(id_type_str, " (tag count threshold)");

      Log_RT(4, "INFO: DSFindDerStream() %s %s %s, tag = 0x%x, len = %u, dest port = %u, pyld len = %d, pyld ofs = %d", msgstr, id_type_str, szInterceptPointId, tag, len, port_list_index >= 0 ? dest_port_list[port_list_index] : dst_port, pyld_len, pyld_ofs);

      #ifdef INTERCEPTPOINTDEBUG
      usleep(1000000);
      #endif
  }

   return ret_val;
}

int64_t DSGetDerStreamInfo(HDERSTREAM hDerStream, unsigned int uFlags, void* pInfo) {

int i;
uint16_t* pList = (uint16_t*)pInfo;

   if (--hDerStream < 0) return -1;

   switch (uFlags & DS_DER_INFO_ITEM_MASK) {
   
      case DS_DER_INFO_DSTPORT:  /* get specific port */
         return der_streams[hDerStream].dest_ports[(uintptr_t)pInfo];

      case DS_DER_INFO_DSTPORT_LIST:  /* get list of ports */
         for (i=0; i<MAX_DER_DSTPORTS; i++) {
            if (!der_streams[hDerStream].dest_ports[i]) break;
            pList[i] = der_streams[hDerStream].dest_ports[i];
         }
         return i;

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

int64_t DSSetDerStreamInfo(HDERSTREAM hDerStream, unsigned int uFlags, void* pInfo) {

int i;
uint16_t* pList = (uint16_t*)pInfo;

   if (--hDerStream < 0) return -1;

   switch (uFlags & DS_DER_INFO_ITEM_MASK) {

      case DS_DER_INFO_DSTPORT:  /* set specific port */
         i = ((uintptr_t)(pInfo) & 0xffff0000) >> 16;
         der_streams[hDerStream].dest_ports[i] = (uint16_t)((uintptr_t)(pInfo) & 0xffff);
         return i;
   
      case DS_DER_INFO_DSTPORT_LIST:  /* set list of ports */
         for (i=0; i<MAX_DER_DSTPORTS; i++) {
            if (!pList[i]) break;
            der_streams[hDerStream].dest_ports[i] = pList[i];
         }
         return i;
   }

   return -1;
}

int DSDecodeDerStream(HDERSTREAM hDerStream, uint8_t* pkt_in_buf, uint8_t* pkt_out_buf, unsigned int uFlags, HI3_DER_DECODE* der_decode, FILE* hFile_asn_output) {

uint16_t pkt_dest_port, dest_port = 0, dest_port_list[MAX_DER_DSTPORTS] = { 0 };
char szInterceptPointId[MAX_DER_STRLEN] = "";

bool fPrint = false, fPointId;
int i, ret_val = 0, asn_index = 0, pyld_len = -1;

   if (--hDerStream < 0) return -1;

   if (!derlib_sem_init) return -1;  /* we don't need the derlib semaphore when decoding, but the app should not be attempting decode unless derlib has been initialized first, so we return an error condition */

   if (DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PROTOCOL, pkt_in_buf, -1, NULL, NULL) != TCP_PROTOCOL) return -1;

   if ((int)(pkt_dest_port = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_DST_PORT, pkt_in_buf, -1, NULL, NULL)) <= 0) return -1;

/* verify packet dest port is on list of ports previously determined from IRI info */

   DSGetDerStreamInfo(hDerStream + 1, DS_DER_INFO_DSTPORT_LIST, dest_port_list);
   
   for (i=0; i<MAX_DER_DSTPORTS; i++) if (dest_port_list[i] == pkt_dest_port) {
      dest_port = pkt_dest_port;
      break;
   }
   if (!dest_port) return -1;  /* not on the list */

   #if 0  /* ASN processing not ready for this yet, HI3 streams can have 10-20 or more consecutive max size packets */
   if (hFile_asn_output) {
   
      int pyld_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDLEN, pkt_in_buf, -1, NULL, NULL);
      DSDecodeDerFields(pkt_in_buf, DS_DER_DECODEFIELDS_PACKET | DS_DER_DECODEFIELDS_OUTPUT_ASN, pyld_len, hFile_asn_output, "decode gen asn");
   }
   #else
   (void)hFile_asn_output;  /* avoid compiler warning */
   #endif

/* proceed with attempted DER decode ... */

   strcpy(szInterceptPointId, der_streams[hDerStream].szInterceptPointId);

/* check for large packet continuation */

   if (der_streams[hDerStream].asn_index == 0) {

      if ((pyld_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDLEN, pkt_in_buf, -1, NULL, NULL)) == 0) {

         if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {
            printf("HI3 port %d NULL packet", dest_port);
            fPrint = true;
         }
         if (der_decode) der_decode->uList |= DS_DER_NULL_PACKET;
         goto ret;
      }
   }
   else asn_index = der_streams[hDerStream].asn_index;

   {  // extra scope level in case we need it

      int pyld_ofs = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDOFS, pkt_in_buf, -1, NULL, NULL);
      if (pyld_len == -1) pyld_len = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PYLDLEN, pkt_in_buf, -1, NULL, NULL);

      int save_len = der_streams[hDerStream].save_len;

      if (save_len) {
         memmove(&pkt_in_buf[pyld_ofs + save_len], &pkt_in_buf[pyld_ofs], pyld_len);
         memcpy(&pkt_in_buf[pyld_ofs], der_streams[hDerStream].packet_save, save_len);
         pyld_len += save_len;
      }

   /* scan for interception point Id (may also be an interception identifier, see DSFindDerStream() above) */

      uint8_t* p = (uint8_t*)memmem(&pkt_in_buf[pyld_ofs + asn_index], pyld_len, szInterceptPointId, strlen(szInterceptPointId));

      if (p && p - &pkt_in_buf[pyld_ofs] >= 2 && ((fPointId = p[-2] == DER_TAG_INTERCEPTPOINTID) || p[-2] == 0x81)) {

         asn_index = (int)(p - &pkt_in_buf[pyld_ofs] - 2);  /* start index at interception point tag */

         p = &pkt_in_buf[pyld_ofs];   /* asn_index is offset from start of payload */

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

 /* to-do: scan for seq num in all cases, with no expection of adjacent tags or tag ordering */

         int seq_num_index, seq_num_len = 0;
         uint8_t seq_num_tag;

         if (fPointId) {  /* for interception point, seq number is just prior so we reverse decode it. DER is based on type-length-value sequencing so forward decoding is a good idea, but this seems to work */
            i = 1;
            while ((p[asn_index-i] != DER_TAG_SEQNUM || p[asn_index-i+1] > 8) && i<11) i++;  /* both sec and usec together can't be more than 11 bytes total */
            seq_num_index = asn_index-i;
         }
         else {  /* for interception identifier, seq number is ahead, after a construct (multi-element) tag */

            seq_num_index = asn_index + 2 + p[asn_index+1];  /* point at tag after interception Id */
            seq_num_index += 2 + p[seq_num_index+1];  /* skip next tag */
            seq_num_index += 2 + p[seq_num_index+1];  /* skip next tag (constructed type, 0xa3), after that is seq number tag */
         }

         if (p[seq_num_index] == DER_TAG_SEQNUM) {
            seq_num_tag = p[seq_num_index];
            seq_num_len = p[seq_num_index+1];
         }

         if (uFlags & DS_DER_SEQNUM) {  /* only if asked for, as it occurs before interception point Id */

            uint64_t seq_num = 0;

            for (i=0; i<seq_num_len; i++) seq_num = (seq_num << 8) | (uint64_t)p[seq_num_index+2+i];

   #if 0
   //if (seq_num > 126 && seq_num < 130) {
      printf(" *** seq num = %d, len = %d, byte 1 = 0x%x, byte 2 = 0x%x \n", seq_num, seq_num_len, p[seq_num_index+2], p[seq_num_index+2+1]);
   //}
   #endif
            if (der_decode) {

               der_decode->uList |= DS_DER_SEQNUM;

               der_decode->sequenceNumber.tag = seq_num_tag;
               der_decode->sequenceNumber.len = seq_num_len;
               der_decode->sequenceNumber.value = seq_num;
            }

            if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {

            /* debug code to verify no missing sequence numbers. num_miss (number of misses) should stay zero */
               static int prev_seq_num[MAX_DER_DSTPORTS] = { -1, -1, -1, -1, -1, -1, -1, -1 };
               static int num_miss[MAX_DER_DSTPORTS] = { 0 };

               for (i=0; i<MAX_DER_DSTPORTS; i++) if (pkt_dest_port == der_streams[hDerStream].dest_ports[i]) break;

               if (prev_seq_num[i] == -1) prev_seq_num[i] = (int)seq_num-1;  /* in case first few packets are not in the stream */
               if ((int)seq_num-1 != prev_seq_num[i]) num_miss[i]++;
               prev_seq_num[i] = (int)seq_num;

               printf(", found seq num %llu, tag = 0x%x, len = %d, port index = %d, num_miss = %d", (unsigned long long)seq_num, seq_num_tag, seq_num_len, i, num_miss[i]);
            }
         }

         if (fPointId) asn_index += strlen(szInterceptPointId) + 2;
         else asn_index = seq_num_index + 1 + seq_num_len;

      /* decode timestamp, if present */

         if (fPointId) {

            int asn_index_save = asn_index;

            tag = p[asn_index++];  /* tag includes DER_TAG_CLASS_CONSTRUCT attribute, which indicates a construct with multiple parts. We know from ETSI standard that timestamp has 2 parts, sec and usec */
            if ((tag & 0x1f) == 31) tag = p[asn_index++];  /* tag number == 31, need to read another tag */
            len = p[asn_index++];  /* length of both parts, including sub tags and sub lengths */

            int sub_tag1 = p[asn_index++];  /* sec tag */
            int sub_len1 = p[asn_index++];
            int timestamp_sec_index = asn_index;

            asn_index += sub_len1;

            int sub_tag2 = p[asn_index++];  /* usec tag */
            int sub_len2 = p[asn_index++];
            int timestamp_usec_index = asn_index;

  #if 0
  static bool fOnce = false;
  char linestr[256];
  sprintf(linestr, " *** construct tag = 0x%x, construct len = %d, sub tag1 = 0x%x, sub len1 = %d, sub tag2 = 0x%x, sub len2 = %d \n", tag, len, sub_tag1, sub_len1, sub_tag2, sub_len2);
  if (!fOnce) { printf(linestr); fwrite(linestr, 1, strlen(linestr), hFile_asn_output); fOnce = true; }
  #endif

            asn_index += sub_len2;

            if (tag == DER_TAG_TIMESTAMP) {

               if (uFlags & DS_DER_TIMESTAMP) {

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
            }
            else asn_index = asn_index_save;  /* timestamp not present, restore main index */

         /* decode timestamp qualifier, if present */

            asn_index_save = asn_index;

            tag = p[asn_index++];
            if ((tag & 0x1f) == 31) tag = p[asn_index++];  /* tag number > 30, need to read another tag */
            len = p[asn_index++];

            if (tag == DER_TAG_TIMESTAMPQUALIFIER) {

               if (uFlags & DS_DER_TIMESTAMPQUALIFIER) {

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
            }
            else asn_index = asn_index_save;  /* timestamp qualifier not present, restore main index */
         }

         if (uFlags & DS_DER_CC_PACKET) {  /* if asked for decode CC packet */

         /* locate encapsulated IP headers using checksum matching, notes:

            -we don't look for ccXX items (ccPayloadSequence, ccContents, iPCC, etc. These seem to be more trouble than they're worth
            -slide byte at a time, calculate and compare checksum per IP header standard. Verify packet header integrity in addition to checksum match
            -always make sure to not exceed available encapsulated TCP packet payload length
         */

            while (asn_index < pyld_len) {

               uint16_t checksum_candidate, checksum;
               uint8_t ip_ver = p[asn_index] >> 4;

               if (ip_ver == 4) {  /* IPv4 */

                  checksum = ((uint16_t)p[asn_index + 11] << 8) | p[asn_index + 10];  /* 10 = byte offset of checksum in IPv4 header */
                  checksum_candidate = calc_checksum(&p[asn_index], 0, 20, 5, 16);    /* pointer to data, initial checksum, num bytes, omit index, width of checksum (in bits) */

                  //#define IPV4DEBUG
                  #ifdef IPV4DEBUG
                  static bool fOnce = false;
                  uint16_t prot = (uint16_t)p[asn_index + 9];
                  if (!fOnce && (prot == 0x11 || prot == 6)) {
                     if (prot == 0x11) {
                        uint16_t udp_len = ((uint16_t)p[asn_index + 24] << 8) | p[asn_index + 25];
                        uint16_t pktlen = ((uint16_t)p[asn_index + 2] << 8) | p[asn_index + 3];
                        printf(" ********** likely found IPv4 / UDP, checksum = 0x%x, checksum cand = 0x%x, pktlen = %d, udp len = %d \n", checksum, (uint16_t)~checksum_candidate, pktlen, udp_len);
                        for (i=0; i<udp_len-8; i++) printf("%0x ", p[asn_index+28+i]);
                     }
                     else {

                        //uint16_t udp_len = ((uint16_t)p[asn_index + 24] << 8) | p[asn_index + 25];
                        uint16_t plen = ((uint16_t)p[asn_index + 2] << 8) | p[asn_index + 3];
                        printf(" ********** likely found IPv4 / TCP, checksum = 0x%x, checksum cand = 0x%x, plen = %d \n", checksum, (uint16_t)~checksum_candidate, plen);
                        //for (i=0; i<udp_len-8; i++) printf("%0x ", p[asn_index+28+i]);
                     }
                     printf(" \n");
                     fOnce = true;
                  }
                  #endif
               }
               else if (ip_ver == 6) {  /* IPv6 */

                  checksum = ((uint16_t)p[asn_index + 47] << 8) | p[asn_index + 46];  /* 46 = byte offset of UDP checksum in IPv6/UDP header without extensions. To-do: handle extensions */
                  uint16_t udp_len = ((uint16_t)p[asn_index + 44] << 8) | p[asn_index + 45];

               /* calculate IPv6 UDP checksum, start with pseudo-header, per RFC2460 sec 8.1 */

                  checksum_candidate = calc_checksum(&p[asn_index + 8], 0, 32, -1, 16);                           /* IPv6 pseudo-header: source/dest addrs */
                  checksum_candidate = calc_checksum(&p[asn_index + 4], checksum_candidate, 2, -1, 16);           /* IPv6 pseudo-header: payload length */
                  uint16_t prot = (uint16_t)p[asn_index + 6] << 8;  /* header protocol is one byte (i.e. 0x11 expected for UDP), but needs to be in network byte-order for checksum purposes */
                  checksum_candidate = calc_checksum(&prot, checksum_candidate, 2, -1, 16);                       /* IPv6 pseudo-header: protocol */

               /* include UDP ports, length, and data */

                  checksum_candidate = calc_checksum(&p[asn_index + 40], checksum_candidate, 6, -1, 16);          /* UDP ports and length */
                  checksum_candidate = calc_checksum(&p[asn_index + 48], checksum_candidate, udp_len-8, -1, 16);  /* UDP body */

                  //#define IPV6DEBUG
                  #ifdef IPV6DEBUG
                  static bool fOnce = false;
                  if (!fOnce && prot == 0x1100) {
                     uint16_t plen = ((uint16_t)p[asn_index + 4] << 8) | p[asn_index + 5];
                     printf(" ********** likely found IPv6 / UDP, checksum = 0x%x, checksum cand = 0x%x, plen = %d, udp len = %d \n", checksum, (uint16_t)~checksum_candidate, plen, udp_len);
                     for (i=0; i<udp_len-8; i++) printf("%0x ", p[asn_index+48+i]);
                     printf(" \n");
                     fOnce = true;
                  }
                  #endif
               }
               else goto next_byte;

               if ((uint16_t)~checksum_candidate == checksum) {  /* compare checksums, 1's complement */

                  uint8_t* p2 = &p[asn_index];
                  int pktlen = DSGetPacketInfo(-1, DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTLEN | DS_PKTLIB_SUPPRESS_ERROR_MSG, p2, -1, NULL, NULL);

                  if (pktlen < 0) goto next_byte;  /* if packet header values are bad, assume checksum hash matched wrong data. Happens every so often with IPv4 */

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
         -if we don't land exactly on end of payload, we need to save data and insert at start of next packet
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

      if (!der_decode->uList) der_streams[hDerStream].asn_index = 0;  /* if nothing found, reset the asn index */
      der_decode->asn_index = der_streams[hDerStream].asn_index;  /* save asn index */
   }

   if (uFlags & DS_DECODE_DER_PRINT_DEBUG_INFO) {
      if (fPrint) printf(" \n");
   }

   return ret_val;
}

static uint16_t calc_checksum(void* p, uint16_t checksum_init, int num_bytes, int omit_index, int checksum_width) {

uint16_t checksum16 = checksum_init;
uint8_t checksum8 = checksum_init;
uint32_t long_checksum;

uint16_t* p16 = (uint16_t*)p;
uint8_t* p8 = (uint8_t*)p;
int i;

   if (checksum_width == 16) {

      for (i=0; i<num_bytes/2; i++) {

         if (i == omit_index) continue;  /* skip a value if needed -- can be used to omit a mid-data comparison checksum. Give -1 if not used */

         long_checksum = checksum16;
         long_checksum += p16[i];
         if (long_checksum > 65535) checksum16 = (uint16_t)(long_checksum + 1);
         else checksum16 = (uint16_t)long_checksum;
      }

      if ((num_bytes & 1) && i != omit_index) {  /* if num_bytes odd read last value as byte zero-extended to 16-bits */

         long_checksum = checksum16;
         long_checksum += *(uint8_t*)&p16[i];
         if (long_checksum > 65535) checksum16 = (uint16_t)(long_checksum + 1);
         else checksum16 = (uint16_t)long_checksum;
      }
   }
   else if (checksum_width == 8) {
  
      for (i=0; i<num_bytes; i++) {

         if (i == omit_index) continue;

         long_checksum = checksum8;
         long_checksum += p8[i];
         if (long_checksum > 255) checksum8 = (uint8_t)(long_checksum + 1);
         else checksum8 = (uint8_t)long_checksum;
      }
   }

   if (checksum_width == 16) return checksum16;
   else return checksum8;
}
