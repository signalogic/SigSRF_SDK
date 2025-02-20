/*
$Header: /root/Signalogic/DirectCore/lib/voplib/extract_rtp_video.cpp
 
Description

  extract H.264 and HEVC elementary bitstreams from RTP packets

Notes

  -fully multithreaded, no locks, no semaphore
  -input packet stream should have all redundancy removed, fragmented packets reassembled, and be in correct RTP sequence number order. In SigSRF software this is handled by pktlib
  -currently only HEVC supported, H.264 is to-do
  -called by DSGetPayloadInfo() API in voplib (https://github.com/signalogic/SigSRF_SDK/blob/master/codecs_readme.md#user-content-dsgetpayloadinfo)
  -calls Log_RT() API in diaglib
  -writing file output is done with DSSaveDataFile() in DirectCore, this can be replaced with simple fwrite() if needed
  -normally linked with voplib, but could be linked with any app or with mediaMin earlier in link order as needed. No other dependencies on other SigSRF libs
 
Projects

  SigSRF, DirectCore
 
Copyright Signalogic Inc. 2025

  based on orignal source code Copyright Denys Kozyr 2018-2025, https://github.com/dkozyr/h265_from_pcap/tree/master

License

  Github SigSRF License, Version 1.1, https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

Revision History

  Created Jan 2025 JHB, replicate results from https://github.com/dkozyr/h265_from_pcap
  Modified Feb 2025 JHB, add AP unit handling
  Modified Feb 2025 JHB, additional error checking and handling
*/

/* Linux and/or other OS includes */

#include <algorithm>
using namespace std;

#include "stdlib.h"
#include "string.h"

/* SigSRF includes */

#include "voplib.h"
#include "directcore.h"
#include "diaglib.h"  /* Log_RT() event logging */

#ifdef __cplusplus
extern "C" {
#endif

  int extract_rtp_video(FILE* fp_out, unsigned int uFlags, uint8_t* rtp_payload, int payload_size, int nID, const char* errstr);

  static int add_to_buffer(uint8_t* buf, uint8_t* data, int index, int len);  /* local buffering with length and space available checks */

#ifdef __cplusplus
}
#endif

/* NAL unit definitions from the spec, https://www.itu.int/rec/dologin_pub.asp?lang=e&id=T-REC-H.265-201802-S!!PDF-E&type=items */

#define NAL_UNIT_VPS  32
#define NAL_UNIT_SPS  33
#define NAL_UNIT_PPS  34

/* technically not NAL units (not in H.264 or HEVC specs), but in RTP format they are treated that way */

#define NAL_UNIT_AP   48
#define NAL_UNIT_FU   49

/* misc error check limits */

#define MIN_RTP_PYLD_LEN  4
#define MAX_RTP_PYLD_LEN  5000  /* extraction is after IP fragmentation reassembly, so packet size could be very large. Is there a maximum HEVC unit size prior to fragmentation ? */

/* extract_rtp_video() extracts H.264 and HEVC elementary bitstreams from RTP packets

  Arguments

   - fp_out should point to output elementary stream file
   - uFlags may contain DS_PAYLOAD_INFO_SUPPRESS_WARNING_MSG, DS_PAYLOAD_INFO_DEBUG_OUTPUT, DS_PAYLOAD_INFO_RESET_ID or a combination (voplib.h). DS_PAYLOAD_INFO_DEBUG_OUTPUT can be enabled/disabled at any time to control debug info visibility
   - rtp_payload should point to an RTP payload
   - rtp_pyld_len should contain RTP payload length (in bytes)
   - nId is an optional unique identifer for multithread or concurrent stream applications
   - errstr is an optional string to be included in error/warning messages

  Return value

   - 1 for success, 0 if nothing was written to the file but no error, or < 0 for an error condition

  Notes

   - DSSaveDataFile() is a DirectCore API defined in directcore.h
   - LogRT() is a dialib event-logging API
*/

int extract_rtp_video(FILE* fp_out, unsigned int uFlags, uint8_t* rtp_payload, int rtp_pyld_len, int nId, const char* errstr) {

#define MAX_IDs  64  /* temporary definition */

static struct {  /* persistent info for FU packet state, duplicate detection, and debug stats */

  uint8_t out_data_save[MAX_RTP_PYLD_LEN];  /* static buffer for use in detecting and stripping consecutive duplicate packets. See fDuplicate for additional comments */

  uint8_t fu_state;

  int out_index_save;  /* duplicate detection */
  int duplicate_count;

  int nNAL_header_format_error_count;  /* debug stats */
  int fu_state_mismatch_count;
  int pkt_count;

} stream_info[MAX_IDs] = {{ 0 }};

uint8_t out_data[MAX_RTP_PYLD_LEN] = { 0 };
int ret_val, out_index = 0;

uint8_t NAL_unit_start_code[] = { 0, 0, 0, 1 };

/* error checks */

   if (nId < 0 || nId >= MAX_IDs) {
      Log_RT(3, "WARNING: DSGetPayloadInfo() -> extract_rtp_video() says nID %d < 0 or exceeds %d \n", nId, MAX_IDs-1);
      return -1;
   }

   if (uFlags & DS_PAYLOAD_INFO_RESET_ID) {  /* reset data for specified nId, return */
      memset(&stream_info[nId], 0, sizeof(stream_info[1]));
      return 0;
   }

   if (!rtp_payload) {
      Log_RT(3, "WARNING: DSGetPayloadInfo() -> extract_rtp_video() says rtp_payload is NULL \n");
      return -1;
   }

   if (rtp_pyld_len < 0 || rtp_pyld_len < MIN_RTP_PYLD_LEN) {
      Log_RT(3, "WARNING: DSGetPayloadInfo() -> extract_rtp_video() says rtp_pyld_len %d < 0 or less than minimum %d \n", rtp_pyld_len, MIN_RTP_PYLD_LEN);
      return -1;
   }

   if (!fp_out && !(uFlags & DS_PAYLOAD_INFO_SUPPRESS_WARNING_MSG)) Log_RT(3, "WARNING: DSGetPayloadInfo() -> extract_rtp_video() says fp_out is NULL \n");  /* if fp_out NULL, continue with file write omitted; for example to see debug information */

   uint16_t nal_pyld_hdr = (rtp_payload[0] << 8) | rtp_payload[1];  /* form NAL payload header in host byte order */

/* check for malformed NAL payload header */

   if ((nal_pyld_hdr & 0x81f8) != 0 || (nal_pyld_hdr & 7) == 0) {  /* see if F bit, LayerId, or TID are out of spec (per RFC 7798) */

      stream_info[nId].nNAL_header_format_error_count++;
 
      if (uFlags & DS_PAYLOAD_INFO_DEBUG_OUTPUT) fprintf(stderr, "\n *** malformed NAL payload header F bit %d, LayerId %d, TID %d \n", nal_pyld_hdr >> 15, (nal_pyld_hdr >> 3) & 0x3f, nal_pyld_hdr & 7);

      if (!(uFlags & DS_PAYLOAD_INFO_SUPPRESS_WARNING_MSG)) Log_RT(3, "WARNING: DSGetPayloadInfo() -> extract_rtp_video() says invalid NAL payload header 0x%x%s%s \n", nal_pyld_hdr, errstr ? " during " : "", errstr ? errstr : "");

      return -1;

      #if 0  /* probably not a good idea, but for future reference fixing a malformed payload header would look something like this */

      if ((nal_pyld_hdr & 0x81f8) != 0) nal_pyld_hdr &= ~0x81f8;
      if ((nal_pyld_hdr & 7) == 0) nal_pyld_hdr |= 1;
      rtp_payload[0] = nal_pyld_hdr >> 8;
      rtp_payload[1] = nal_pyld_hdr & 0xff;
      #endif
   }

/* begin extraction based on NAL unit type */

   const int nal_unit_type = (rtp_payload[0] & 0x7f) >> 1;

   bool fFuStart = false;
   bool fFuEnd = false;

   if (nal_unit_type == NAL_UNIT_AP) {  /* aggregated units - separate out and prefix each with start code, JHB Feb 2025 */

   /* extract AP units:

      -first 2 bytes are unit length, then the unit, then another length if applicable, and so on. We use int16_t to help with error checking
      -a length of zero (or end of the payload) indicates no further units
      -length checks: can't be longer than remainder of payload, can't be negative
      -index checks: can't go beyond end of payload
   */

      int16_t len, index = 2;
      while ((len = min(((rtp_payload[index] << 8) | rtp_payload[index+1]), rtp_pyld_len-index)) > 0 && index < rtp_pyld_len) {

         index+= 2;
         out_index += add_to_buffer(out_data, (uint8_t*)&NAL_unit_start_code, out_index, sizeof(NAL_unit_start_code));
         out_index += add_to_buffer(out_data, &rtp_payload[index], out_index, len);
         index += len;
      }
   }
   else if (nal_unit_type == NAL_UNIT_FU) {  /* fragmented units */

      const uint8_t fu_header = rtp_payload[2];
      fFuStart = !!(fu_header & 0x80);
      fFuEnd = !!(fu_header & 0x40);  /* only used for debug, detect any mismatches in FU start and stop */

      const uint8_t fu_type = fu_header & 0x3f;

      if (fFuStart) {

         if (fFuEnd && !(uFlags & DS_PAYLOAD_INFO_SUPPRESS_WARNING_MSG)) Log_RT(3, "WARNING: DSGetPayloadInfo() -> extract_rtp_video() says both FuStart and FuEnd bits set in fu_header 0x%x, not all RTP redundancy or out-of-order removed from stream or RTP payload may be corrupted%s%s \n", fu_header, errstr ? " during " : "", errstr ? errstr : "");

      /* set FU packet state */

         if (stream_info[nId].fu_state) stream_info[nId].fu_state_mismatch_count++;  /* increment mismatch count on consecutive FU start packets */
         else stream_info[nId].fu_state = 1;

         out_index += add_to_buffer(out_data, (uint8_t*)&NAL_unit_start_code, out_index, sizeof(NAL_unit_start_code));

         uint8_t nal_unit[] = { (uint8_t)(nal_pyld_hdr >> 8), (uint8_t)nal_pyld_hdr };  /* form NAL unit header, use payload header LayerId and TID (Temporal Id) */
         nal_unit[0] &= 0x81;
         nal_unit[0] |= fu_type << 1;

         out_index += add_to_buffer(out_data, (uint8_t*)&nal_unit, out_index, sizeof(nal_unit));
      }

      if (!stream_info[nId].fu_state) stream_info[nId].fu_state_mismatch_count++;  /* increment mismatch count on (i) consecutive FU end packets or (ii) FU middle or end packet with no start packet */

      if (fFuEnd) stream_info[nId].fu_state = 0;  /* reset FU packet state */

      out_index += add_to_buffer(out_data, &rtp_payload[3], out_index, rtp_pyld_len-3);
   }
   else {  /* all other NAL units */

      if (stream_info[nId].fu_state) stream_info[nId].fu_state_mismatch_count++;  /* increment mismatch count if a non FU packet shows up before an FU end packet */

      out_index += add_to_buffer(out_data, (uint8_t*)&NAL_unit_start_code, out_index, sizeof(NAL_unit_start_code));

      out_index += add_to_buffer(out_data, rtp_payload, out_index, rtp_pyld_len);
   }

/* check for consecutive duplicate RTP payload */

   bool fDuplicate = stream_info[nId].out_index_save && stream_info[nId].out_index_save == out_index && !memcmp(out_data, stream_info[nId].out_data_save, out_index);

/* show debug info if requested */

   if (uFlags & DS_PAYLOAD_INFO_DEBUG_OUTPUT) {

      char tmpstr[500];
      sprintf(tmpstr, "output bitstream %d for packet #%d rtp len = %d, out_index = %d, NAL unit type = %d, FU start = %d, FU end = %d, FU state mismatch count = %d, duplicate = %d, duplicate count = %d, NAL header format errors = %d, header =", nId, stream_info[nId].pkt_count+1, rtp_pyld_len, out_index, nal_unit_type, fFuStart, fFuEnd, stream_info[nId].fu_state_mismatch_count, fDuplicate, stream_info[nId].duplicate_count, stream_info[nId].nNAL_header_format_error_count);

      int len;
      if (nal_unit_type == NAL_UNIT_AP) len = out_index;
      else if (nal_unit_type == NAL_UNIT_VPS || nal_unit_type == NAL_UNIT_SPS || nal_unit_type == NAL_UNIT_PPS) len = min(out_index, 100);
      else len = min(out_index, 20);
      for (int i=0; i<len; i++) sprintf(&tmpstr[strlen(tmpstr)], " 0x%x", out_data[i]);
      fprintf(stderr, "\n *** %s \n", tmpstr);

      stream_info[nId].pkt_count++;
   }

/* strip consecutive duplicates, if any */

   if (fDuplicate) {  /* normally pktlib packet/media worker threads and jitter buffers handle redundancy, but occassionally RTP payloads can be duplicated dozens of packets apart, for example if the sender is using high latency protocols for redundancy (e.g. GPRS Tunnelling Protocol). For vps/sps/pps units duplication shouldn't matter but for slice units it might, also we need to avoid repeated FU packets that would cause an FU state mismatch */

      stream_info[nId].duplicate_count++;
      return 0;
   }

/* write output buffer to file, if requested */

   if (fp_out && (ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, NULL, (uintptr_t)out_data, out_index, DS_WRITE | DS_DATAFILE_USE_SEMAPHORE, NULL)) < 0) { Log_RT(2, "ERROR: DSPayloadInfo() --> extract_rtp_video() call to DSSaveDataFile() output write fails for %s output, ret_val = %d \n", errstr ? errstr : "", ret_val); return -1; }

/* save output buffer */

   memcpy(stream_info[nId].out_data_save, out_data, out_index);
   stream_info[nId].out_index_save = out_index;

   return 1;  /* return success */
}

/* add extracted data to output buffer with checks for length and space available */

static int add_to_buffer(uint8_t* buf, uint8_t* data, int index, int len) {

int amount_copied = 0;

   if (index < MAX_RTP_PYLD_LEN) {  /* don't exceed max buffer size */

      amount_copied = min(len, MAX_RTP_PYLD_LEN - index);  /* don't exceed space available */
      memcpy(&buf[index], data, amount_copied);
   }

   return amount_copied;
}