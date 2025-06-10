/*

 $Header: /root/Signalogic/DirectCore/lib/voplib/extract_rtp_video.cpp

 Description

  for H.264 and H.265 (HEVC) RTP packet streams, retrieve payload information and/or extract HEVC elementary bitstreams

 Notes

  -fully multithreaded, no locks, no semaphore
  -input packet stream should have all redundancy removed, fragmented packets reassembled, and be in correct RTP sequence number order. In SigSRF software this is handled by pktlib
  -called by DSGetPayloadInfo() API in voplib (https://github.com/signalogic/SigSRF_SDK/blob/master/codecs_readme.md#user-content-dsgetpayloadinfo)
  -calls Log_RT() API in diaglib
  -writing file output is done with DSSaveDataFile() in DirectCore, this can be replaced with simple fwrite() if needed
  -normally linked with voplib, but could be linked with any app or with mediaMin earlier in link order as needed. No dependencies on other SigSRF libs
 
 Projects

  SigSRF, DirectCore
 
 Copyright (C) Signalogic Inc. 2025

  based on source code Copyright (C) Denys Kozyr 2018-2025, https://github.com/dkozyr/h265_from_pcap/tree/master

 License

  Github SigSRF License, Version 1.1, https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

 Revision History

  Created Jan 2025 JHB, replicate results from https://github.com/dkozyr/h265_from_pcap
  Modified Feb 2025 JHB, add AP unit handling
  Modified Feb 2025 JHB, additional error checking and handling
  Modified Feb 2025 JHB, add SDP info handling, base64 decoding of fmtp sprop-xps fields
  Modified Mar 2025 JHB, set payload_info items regardless of whether fp_out supplied
  Modified Mar 2025 JHB, add pInfo param to allow copying to mem buffer extracted bitstream data
  Modified Apr-May 2025 JHB, add H.264 functionality
*/

/* Linux and/or other OS includes */

#include <algorithm>
using namespace std;

#include "stdlib.h"
#include "string.h"

/* SigSRF includes */

#include "voplib.h"         /* PAYLOAD_INFO and SDP_INFO struct definitions; voplib.h includes shared_include/codec.h */
#include "directcore.h"     /* DSSaveDataFile() handles bitstream and wav files */ 
#include "diaglib.h"        /* Log_RT() event logging */
#include "base64/base64.h"  /* base64 decode API header file */

#ifdef __cplusplus
extern "C" {
#endif

  int extract_rtp_video(FILE* fp_out, codec_types codec_type, unsigned int uFlags, uint8_t* rtp_payload, int rtp_pyld_len, PAYLOAD_INFO* payload_info, SDP_INFO* sdp_info, void* pInfo, int nID, const char* errstr);

  static int write_to_buffer(uint8_t* buf, uint8_t* data, int offset, int len);  /* local buffering with length and space available checks */

#ifdef __cplusplus
}
#endif

/* NAL unit definitions from the codec specs

   H.265 - https://www.itu.int/rec/dologin_pub.asp?lang=e&id=T-REC-H.265-201802-S!!PDF-E&type=items
   H.264 - https://www.itu.int/rec/dologin_pub.asp?lang=e&id=T-REC-H.264-201304-S!!PDF-E&type=items
*/

#define NAL_UNIT_VPS_HEVC       32
#define NAL_UNIT_SPS_HEVC       33
#define NAL_UNIT_PPS_HEVC       34

#define NAL_UNIT_NON_IDR_SLICE   1  /* H.264 specs */
#define NAL_UNIT_IDR_SLICE       5

#define NAL_UNIT_SEI_H264        6
#define NAL_UNIT_SPS_H264        7
#define NAL_UNIT_PPS_H264        8

/* NAL UNIT definitions from the RTP format specs. Technically these are not NAL units (i.e. not in HEVC or H.264 codec specs), but in RTP format specs (RFC 7798 and RFC 6184) they are assigned NAL unit type values marked as Reserved in the codec specs */

#define NAL_UNIT_AP             48  /* H.265 RFC 7798 section 4.4.2, Aggregation Packets */
#define NAL_UNIT_FU             49  /* H.265 RFC 7798 section 4.4.3, Fragmentation Units */

#define NAL_UNIT_STAPA          24  /* H.264 RFC 6184 */
#define NAL_UNIT_STAPB          25
#define NAL_UNIT_MTAP16         26
#define NAL_UNIT_MTAP24         27
#define NAL_UNIT_FU_A           28
#define NAL_UNIT_FU_B           29

/* misc error check limits */

#define MIN_RTP_PYLD_LEN         4
#define MAX_RTP_PYLD_LEN      5000  /* extraction is after IP fragment reassembly, so packet size could be very large. Is there a maximum MTU size prior to fragmentation ? */

/* extract_rtp_video() extracts H.264 and HEVC elementary bitstreams from RTP packets

  Arguments - any marked "optional" should be NULL if not used, unless specified otherwise

   - fp_out is an optional pointer to an elementary bitstream file. If fp_out is given it should point to an open output binary file, otherwise it should be NULL
   - codec_type specifies the codec type (see definitions in shared_include/codec.h)
   - uFlags may contain DS_PAYLOAD_INFO_IGNORE_INBAND_XPS, DS_PAYLOAD_INFO_DEBUG_OUTPUT, DS_PAYLOAD_INFO_RESET_ID, DS_VOPLIB_SUPPRESS_WARNING_ERROR_MSG, DS_VOPLIB_SUPPRESS_INFO_MSG, or a combination. DS_PAYLOAD_INFO_DEBUG_OUTPUT can be enabled/disabled at any time to control debug info visibility. All uFlags are defined in voplib.h
   - rtp_payload should point to an RTP payload
   - rtp_pyld_len should contain RTP payload length (in bytes)
   - payload_info is an optional pointer to a PAYLOAD_INFO struct to retrieve payload information, including NAL unit header type. PAYLOAD_INFO is defined in voplib.h
   - sdp_info is an optional pointer to an SDP_INFO struct containing an SDP info fmtp string with sprop-vps, sprop-sps, and/or sprop-pps "a=fmtp.." fields. An example is given below in comments after code ends. SDP_INFO is defined in voplib.h
   - pInfo is an optional pointer to a buffer to copy extracted elementary bitstream data
   - nId is an optional unique identifer for multithread or concurrent stream applications. nId should be -1 if not used
   - errstr is an optional string to be included in error/warning messages

  Return value

   - return value is (i) number of bytes written to output file or memory buffer if fp_out and/or pInfo is supplied (or 0 if nothing was written but no error), (ii) payload format if fp_out and pInfo are NULL, or (iii) < 0 on error condition

  Notes

   - structs are defined in voplib.h
   - any combination of fp_out, payload_info, and pInfo may be given. If they are all NULL a warning message will be displayed but the API will continue to function, for example displaying debug information
   - for any given stream, any sequence of calls with nId >= 0 and nId < 0 is supported. But to enable fragmented packet reassembly, packet and debug stats, duplicate detection, and other functionality that depends on prior input, nId should be >= 0
   - DSSaveDataFile() is a DirectCore API defined in directcore.h (hwlib.so)
   - LogRT() is an event-logging API defined in diagib.h (diaglib.so)
   - base64_decode() is in apps/common/base64/base64.cpp
   - xps is shorthand for VPS, SPS, and/or PPS NAL units. Note that H.264 does not have a VPS NAL unit type
*/

int extract_rtp_video(FILE* fp_out, codec_types codec_type, unsigned int uFlags, uint8_t* rtp_payload, int rtp_pyld_len, PAYLOAD_INFO* payload_info, SDP_INFO* sdp_info, void* pInfo, int nId, const char* errstr) {

#define MAX_IDs  64  /* temporary definition */

static struct {  /* persistent info for FU packet state, duplicate detection, and debug stats */

  uint8_t out_data_prev[MAX_RTP_PYLD_LEN];  /* static buffer for use in detecting and stripping consecutive duplicate packets. See fDuplicate for additional comments */
  int out_index_prev;
  int out_index_total;

  uint8_t fu_state;  /* fragment packet reassembly state */

  int duplicate_count;  /* duplicate detection */

  int nNAL_header_format_error_count;  /* debug stats */
  int fu_state_mismatch_count;
  int pkt_count;
  uint8_t uXPS_outofband_inserted;  /* set with 1 bit flags if vps, sps, and/or pps SDP info is inserted */

} stream_info[MAX_IDs] = {{ 0 }};

uint8_t out_data[MAX_RTP_PYLD_LEN] = { 0 };
int ret_val = -1, out_index = 0;

/* generic start codes */

const uint8_t NAL_unit_start_code_H264[] = { 0, 0, 1 };
const uint8_t NAL_unit_start_code_HEVC[] = { 0, 0, 0, 1 };

/* H.264 and HEVC xps NAL unit start codes. These are compared with incoming data when searching for in-band xps NAL units */

const uint8_t NAL_unit_start_code_xps_H264[][4] = { { 0, 0, 1, 0x7 }, { 0, 0, 1, 0x08 } };  /* H.264 SPS and PPS NAL unit start codes */
const uint8_t NAL_unit_start_code_xps_HEVC[][5] = { { 0, 0, 0, 1, 0x40 }, { 0, 0, 0, 1, 0x42 }, { 0, 0, 0, 1, 0x44 } };  /* HEVC VPS, SPS, and PPS NAL unit start codes */

/* SDP info sprop-xps definitions */

const char* sprop_xps[] = { "sprop-vps=", "sprop-sps=", "sprop-pps=" };

/* error checks */

   bool fError = false;

   if (nId < -1 || nId >= MAX_IDs) {
      Log_RT(2, "ERROR: DSGetPayloadInfo() -> extract_rtp_video() says nID %d < -1 or exceeds %d, uFlags = 0x%x \n", nId, MAX_IDs-1, uFlags);
      fError = true;
   }
   else if (nId >= 0 && (uFlags & DS_PAYLOAD_INFO_RESET_ID)) {  /* reset data for specified nId, return */
      memset(&stream_info[nId], 0, sizeof(stream_info[1]));
      return 0;
   }

   if (!rtp_payload) {
      Log_RT(2, "ERROR: DSGetPayloadInfo() -> extract_rtp_video() says rtp_payload is NULL, uFlags = 0x%x \n", uFlags);
      fError = true;
   }

   if (rtp_pyld_len < 0 || rtp_pyld_len < MIN_RTP_PYLD_LEN) {
      Log_RT(2, "ERROR: DSGetPayloadInfo() -> extract_rtp_video() says rtp_pyld_len %d < 0 or less than minimum %d, uFlags = 0x%x \n", rtp_pyld_len, MIN_RTP_PYLD_LEN, uFlags);
      fError = true;
   }

   if (codec_type != DS_CODEC_VIDEO_H265 && codec_type != DS_CODEC_VIDEO_H264) {
      Log_RT(2, "ERROR: DSGetPayloadInfo() -> extract_rtp_video() says unsupported codec type %d, uFlags = 0x%x \n", codec_type, uFlags);
      fError = true;
   }

   if (fError) return -1;

   if (!payload_info && !fp_out && !pInfo && !(uFlags & DS_VOPLIB_SUPPRESS_INFO_MSG)) Log_RT(3, "WARNING: DSGetPayloadInfo() -> extract_rtp_video() will process with payload_info, fp_out, and pInfo all NULL, uFlags = 0x%x \n", uFlags);  /* if payload_info, fp_out and pInfo are all NULL, continue without payload info parsing, file write, and mem retrieval; examples include checking for warnings and errors, and viewing debug information */

/* check for malformed NAL payload header */

   uint16_t nal_pyld_hdr, nal_mask_value1, nal_mask_value2;

   if (codec_type == DS_CODEC_VIDEO_H265) {

      nal_pyld_hdr = (rtp_payload[0] << 8) | rtp_payload[1];  /* form NAL payload header in host byte order */
      nal_mask_value1 = 0x81f8;
      nal_mask_value2 = 0x7;
   }
   else if (codec_type == DS_CODEC_VIDEO_H264) {

      nal_pyld_hdr = rtp_payload[0];
      nal_mask_value1 = 0x80;
      nal_mask_value2 = 0x1f;
   }

   if ((nal_pyld_hdr & nal_mask_value1) != 0 || (nal_pyld_hdr & nal_mask_value2) == 0) {  /* RFC 7798: check if F bit, LayerId, or TID are out of spec, RFC 6184: check if F bit or type is out of spec */

      if (nId >= 0) stream_info[nId].nNAL_header_format_error_count++;
 
      if (uFlags & DS_PAYLOAD_INFO_DEBUG_OUTPUT) {
         if (codec_type == DS_CODEC_VIDEO_H265) fprintf(stderr, "\n *** malformed NAL payload header F bit %d, LayerId %d, TID %d \n", nal_pyld_hdr >> 15, (nal_pyld_hdr >> 3) & 0x3f, nal_pyld_hdr & 7);
         else fprintf(stderr, "\n *** malformed NAL payload header F bit %d, Type %d \n", nal_pyld_hdr >> 7, nal_pyld_hdr & 0x1f);
      }

      if (!(uFlags & DS_VOPLIB_SUPPRESS_WARNING_ERROR_MSG)) Log_RT(3, "WARNING: DSGetPayloadInfo() -> extract_rtp_video() says invalid NAL payload header 0x%x%s%s, uFlags = 0x%x \n", nal_pyld_hdr, errstr ? " during " : "", errstr ? errstr : "", uFlags);

      return -1;

      #if 0  /* probably not a good idea, but for future reference fixing a malformed HEVC payload header would look something like this */

      if ((nal_pyld_hdr & 0x81f8) != 0) nal_pyld_hdr &= ~0x81f8;
      if ((nal_pyld_hdr & 7) == 0) nal_pyld_hdr |= 1;
      rtp_payload[0] = nal_pyld_hdr >> 8;
      rtp_payload[1] = nal_pyld_hdr & 0xff;
      #endif
   }

/* fill in payload_info if supplied by application caller. NumFrames is filled in later depending on unit type */

   if (payload_info) {

      payload_info->uFormat = codec_type == DS_CODEC_VIDEO_H265 ? DS_PYLD_FMT_H265 : DS_PYLD_FMT_H264;
      payload_info->video.NALU_Header = nal_pyld_hdr;
   }

  // #define H264_DEBUG

   #ifdef H264_DEBUG
   static int pkt_count = 1;
   #endif

/* begin extraction based on NAL unit type */

   uint8_t nal_unit_type = codec_type == DS_CODEC_VIDEO_H265 ? (rtp_payload[0] & 0x7f) >> 1 : rtp_payload[0] & 0x1f;

   switch (codec_type) {
   
      case DS_CODEC_VIDEO_H265:
 
         if (nal_unit_type == NAL_UNIT_AP) {  /* RFC 7798 section 4.4.2, Aggregation Packets. Separate out and prefix each with start code, JHB Feb 2025 */

         /* extract AP units:

            -first 2 bytes are unit length, then the unit, then another length if applicable, and so on. We use int16_t to help with error checking
            -a length of zero (or end of the payload) indicates no further units
            -length checks: can't be longer than remainder of payload, can't be negative
            -index checks: can't go beyond end of payload
         */

            int16_t len, index = 2;
            while (index+1 < rtp_pyld_len && (len = min(((rtp_payload[index] << 8) | rtp_payload[index+1]), rtp_pyld_len-index)) > 0 ) {

               index += 2;

               if (fp_out || pInfo) {
                  out_index += write_to_buffer(out_data, (uint8_t*)&NAL_unit_start_code_HEVC, out_index, sizeof(NAL_unit_start_code_HEVC));
                  out_index += write_to_buffer(out_data, &rtp_payload[index], out_index, len);
               }

               index += len;

               if (payload_info) {
                  if (payload_info->NumFrames >= 0 && payload_info->NumFrames < MAX_PAYLOAD_FRAMES) payload_info->FrameSize[payload_info->NumFrames] = len;
                  payload_info->NumFrames++;
               }
            }

            if (payload_info) payload_info->video.FU_Header = 0;
            if (nId >= 0) stream_info[nId].out_index_total = 0;  /* restart long framesize count */
         }
         else if (nal_unit_type == NAL_UNIT_FU) {  /* RFC 7798 section 4.4.3, Fragmentation Units */

            const uint8_t fu_header = rtp_payload[2];
            bool fFuStart = !!(fu_header & 0x80);
            bool fFuEnd = !!(fu_header & 0x40);  /* FU End (i.e. fragmented frame complete) used to (i) determine when to set payload_info->NumFrames and (ii) detect mismatches in FU start and end */

            const uint8_t fu_type = fu_header & 0x3f;

            if (fFuStart) {

               if (fFuEnd && !(uFlags & DS_VOPLIB_SUPPRESS_WARNING_ERROR_MSG)) Log_RT(3, "WARNING: DSGetPayloadInfo() -> extract_rtp_video() H.265 says both FuStart and FuEnd bits set in FU Header 0x%x, not all RTP redundancy or out-of-order removed from stream or RTP payload may be corrupted%s%s \n", fu_header, errstr ? " during " : "", errstr ? errstr : "");

            /* set FU packet state */

               if (nId >= 0) {

                  if (stream_info[nId].fu_state) stream_info[nId].fu_state_mismatch_count++;  /* increment mismatch count on FU start packets with no intervening FU end packet */
                  else {
                     stream_info[nId].fu_state = 1;
                     stream_info[nId].out_index_total = 0;  /* restart long frame size count */
                  }
               }

               if (fp_out || pInfo) {
               
                  out_index += write_to_buffer(out_data, (uint8_t*)&NAL_unit_start_code_HEVC, out_index, sizeof(NAL_unit_start_code_HEVC));

                  uint8_t nal_unit[] = { (uint8_t)(nal_pyld_hdr >> 8), (uint8_t)nal_pyld_hdr };  /* form NAL unit header, use payload header LayerId and TID (Temporal Id) */
                  nal_unit[0] &= 0x81;
                  nal_unit[0] |= fu_type << 1;

                  out_index += write_to_buffer(out_data, (uint8_t*)&nal_unit, out_index, sizeof(nal_unit));
               }
            }

            if (nId >= 0 && !stream_info[nId].fu_state) stream_info[nId].fu_state_mismatch_count++;  /* increment mismatch count on (i) consecutive FU end packets or (ii) FU middle or end packet with no start packet */

            if (fFuEnd && nId >= 0) stream_info[nId].fu_state = 0;  /* reset FU packet state */

            if (fp_out || pInfo) {

               int k = rtp_pyld_len;

               if (fFuEnd) while (k > 3 && rtp_payload[k-1] == 0) k--;  /* remove trailing zeros from FU end packet, if any. This was initially found with some H.264 RTP streams and has been applied here */  

               //if (k < rtp_pyld_len) printf("\n *** trimmed %d trailing zeros \n", rtp_pyld_len-k);

               out_index += write_to_buffer(out_data, &rtp_payload[3], out_index, k-3);//rtp_pyld_len-3);
            }

            if (payload_info) {

               payload_info->video.FU_Header = fu_header;

               payload_info->FrameSize[0] = (nId >= 0 ? stream_info[nId].out_index_total : 0) + out_index;  /* update frame size continuously - in case of error we have a partial value and some idea where the error occurred */

               payload_info->NumFrames = fFuEnd ? 1 : 0;  /* same with num frames - update continuously, but not a full frame until FU header indicates an end fragment */
            }
         }
         else {  /* all other NAL units */

            if (nId >= 0 && stream_info[nId].fu_state) stream_info[nId].fu_state_mismatch_count++;  /* increment mismatch count if a non FU packet shows up before an FU end packet */

            if (fp_out || pInfo) {
               out_index += write_to_buffer(out_data, (uint8_t*)&NAL_unit_start_code_HEVC, out_index, sizeof(NAL_unit_start_code_HEVC));
               out_index += write_to_buffer(out_data, rtp_payload, out_index, rtp_pyld_len);
            }

            if (payload_info) {

               payload_info->video.FU_Header = 0;
               payload_info->FrameSize[0] = out_index;
               payload_info->NumFrames = 1;
            }

            if (nId >= 0) stream_info[nId].out_index_total = 0;  /* restart long framesize count */
         }

         break;

      case DS_CODEC_VIDEO_H264:

         if (nal_unit_type == NAL_UNIT_STAPA || nal_unit_type == NAL_UNIT_STAPB) {

            #ifdef H264_DEBUG
            static bool fSTAPAOnce = false; if (!fSTAPAOnce) { fSTAPAOnce = true; printf("\n *** received STAPA unit \n"); }
            static bool fSTAPBOnce = false; if (!fSTAPBOnce) { fSTAPBOnce = true; printf("\n *** received STAPB unit \n"); }
            #endif
         }
         else if (nal_unit_type == NAL_UNIT_MTAP16 || nal_unit_type == NAL_UNIT_MTAP24) {

            #ifdef H264_DEBUG
            static bool fMTAP16Once = false; if (!fMTAP16Once) { fMTAP16Once = true; printf("\n *** received MTAP16 unit \n"); }
            static bool fMTAP24Once = false; if (!fMTAP24Once) { fMTAP24Once = true; printf("\n *** received MTAP24 unit \n"); }
            #endif
         }
         else if (nal_unit_type == NAL_UNIT_FU_A || nal_unit_type == NAL_UNIT_FU_B) {

            const uint8_t fu_header = rtp_payload[1];
            bool fFuStart = !!(fu_header & 0x80);
            bool fFuEnd = !!(fu_header & 0x40);  /* FU End (i.e. fragmented frame complete) used to (i) determine when to set payload_info->NumFrames and (ii) detect mismatches in FU start and end */

            const uint8_t fu_type = fu_header & 0x1f;

            #ifdef H264_DEBUG
            static bool fFUAOnce = false; if (nId >= 0 && !fFUAOnce && nal_unit_type == NAL_UNIT_FU_A) { fFUAOnce = true; printf("\n *** received FU A unit, FU Header = 0x%x, fFuStart = %d \n", fu_header, fFuStart); }
            static bool fFUBOnce = false; if (nId >= 0 && !fFUBOnce && nal_unit_type == NAL_UNIT_FU_B) { fFUBOnce = true; printf("\n *** received FU B unit \n"); }
            #endif

            #ifdef H264_DEBUG
            int nal_size = 0;
            #endif

            if (fFuStart) {

               #ifdef H264_DEBUG
               static bool fFUAStart = false; if (nId >= 0 && !fFUAStart && nal_unit_type == NAL_UNIT_FU_A) { fFUAStart = true; printf("\n *** received FU A Start, pkt count = %d \n", pkt_count); }
               static bool fFUBStart = false; if (nId >= 0 && !fFUBStart && nal_unit_type == NAL_UNIT_FU_B) { fFUBStart = true; printf("\n *** received FU B Start \n"); }
               #endif

               if (fFuEnd && !(uFlags & DS_VOPLIB_SUPPRESS_WARNING_ERROR_MSG)) Log_RT(3, "WARNING: DSGetPayloadInfo() -> extract_rtp_video() H.264 says both Start and End bits set in FU Header 0x%x, possibly not all RTP redundancy or out-of-order removed from stream or RTP payload is corrupted%s%s \n", fu_header, errstr ? " during " : "", errstr ? errstr : "");

            /* set FU packet state */

               if (nId >= 0) {

                  if (stream_info[nId].fu_state) {

                     stream_info[nId].fu_state_mismatch_count++;  /* increment mismatch count on FU start packets with no intervening FU end packet */

                     #ifdef H264_DEBUG
                     printf("\n *** FU state mismatch (2nd consecutive start), NAL unit type = %d \n", nal_unit_type);
                     #endif
                  }
                  else {
                     stream_info[nId].fu_state = 1;
                     stream_info[nId].out_index_total = 0;  /* restart long frame size count */
                  }
               }

               if (fp_out || pInfo) {
               
                  out_index += write_to_buffer(out_data, (uint8_t*)&NAL_unit_start_code_H264, out_index, sizeof(NAL_unit_start_code_H264));

                  uint8_t nal_unit[] = { (uint8_t)((nal_pyld_hdr & 0xe0) | fu_type) };  /* form NAL unit header, combine payload header NRI and FU header type */

                  #ifdef H264_DEBUG
                  nal_size = sizeof(nal_unit);
                  #endif

                  out_index += write_to_buffer(out_data, (uint8_t*)&nal_unit, out_index, sizeof(nal_unit));
               }
            }

               #ifdef H264_DEBUG
               if (nId >= 0 && (pkt_count == 4 || pkt_count == 12)) printf("\n *** pkt# %d FU A unit, FU Header = 0x%x, fFuStart = %d, fFuEnd = %d, nal unit size = %d, nId = %d \n", pkt_count, fu_header, fFuStart, fFuEnd, nal_size, nId);  /* applies to pcaps/h264.pcap only */
               #endif
 
            if (nId >= 0 && !stream_info[nId].fu_state) {

               stream_info[nId].fu_state_mismatch_count++;  /* increment mismatch count on (i) consecutive FU end packets or (ii) FU middle or end packet with no start packet */

               #ifdef H264_DEBUG
               printf("\n *** FU state mismatch (end without start), NAL unit type = %d \n", nal_unit_type);
               #endif
            }

            if (nId >= 0 && fFuEnd) {
            
               stream_info[nId].fu_state = 0;  /* reset FU packet state */

               #ifdef H264_DEBUG
               static bool fFUAEnd = false; if (!fFUAEnd && nal_unit_type == NAL_UNIT_FU_A) { fFUAEnd = true; printf("\n *** received FU A End, pkt count = %d \n", pkt_count); }
               static bool fFUBEnd = false; if (!fFUBEnd && nal_unit_type == NAL_UNIT_FU_B) { fFUBEnd = true; printf("\n *** received FU B End \n"); }
               #endif
            }

            if (fp_out || pInfo) {

            /* remove trailing zeros from FU end packet, if any. Notes:

               -the reason for this is to not leave zeros that will get confused with the next NAL start code
               -test with pcaps/h264.pcap (whoever made that pcap set every FU packet size to 1024 bytes regardless of middle or end)
            */
  
               int k = rtp_pyld_len;

               if (fFuEnd) while (k > 2 && rtp_payload[k-1] == 0) k--;

               //if (k < rtp_pyld_len) printf("\n *** trimmed %d trailing zeros \n", rtp_pyld_len-k);

            /* write FU data to buffer */

               out_index += write_to_buffer(out_data, &rtp_payload[2], out_index, k-2);
            }

            if (payload_info) {

               payload_info->video.FU_Header = fu_header;

               payload_info->FrameSize[0] = (nId >= 0 ? stream_info[nId].out_index_total : 0) + out_index;  /* update frame size continuously - in case of error we have a partial value and some idea where the error occurred */

               payload_info->NumFrames = fFuEnd ? 1 : 0;  /* same with num frames - update continuously, but not a full frame until FU header indicates an end fragment */
            }

         }
         else {  /* single NAL unit, includes SEI, SPS, and non-IDR slices */

            #ifdef H264_DEBUG

            if (nal_unit_type == NAL_UNIT_SEI_H264) {
               static bool fSEIOnce = false; if (nId >= 0 && !fSEIOnce) { fSEIOnce = true; printf("\n *** received SEI unit, pkt count = %d \n", pkt_count); }
            }
            else if (nal_unit_type == NAL_UNIT_SPS_H264) {
               static bool fSPSOnce = false; if (nId >= 0 && !fSPSOnce) { fSPSOnce = true; printf("\n *** received SPS unit, pkt count = %d \n", pkt_count); }
            }
            else if (nal_unit_type == NAL_UNIT_PPS_H264) {
               static bool fPPSOnce = false; if (nId >= 0 && !fPPSOnce) { fPPSOnce = true; printf("\n *** received PPS unit, pkt count = %d \n", pkt_count); }
            }
            else if (nal_unit_type == NAL_UNIT_NON_IDR_SLICE) {
               static bool fnonIDRSliceOnce = false; if (nId >= 0 && !fnonIDRSliceOnce) { fnonIDRSliceOnce = true; printf("\n *** received non-IDR slice unit, pkt count = %d \n", pkt_count); }
            }
            else printf("\n *** new single NAL unit type = %u \n", nal_unit_type);

            #endif

            if (fp_out || pInfo) {
               out_index += write_to_buffer(out_data, (uint8_t*)&NAL_unit_start_code_H264, out_index, sizeof(NAL_unit_start_code_H264));
               out_index += write_to_buffer(out_data, rtp_payload, out_index, rtp_pyld_len);
            }

         }

         break;

      default:

         Log_RT(3, "WARNING: extract_rtp_video() says unsupported codec type %d, uFlags = 0x%x \n", codec_type, uFlags);
         break;

   }  /* end of codec type switch statement */

   #if 0  // FU end packet trailing zero debug
   if (nId >= 0 && !stream_info[nId].fu_state && out_index >= 2 && out_data[out_index-1] == 0 && out_data[out_index-2] == 0) {  /* last 2 bytes of a single unit or end FU unit are zero, decoder may get confused with start code for next unit */

      printf("\n *** last 2 bytes are zero \n");
   }
   #endif

   #ifdef H264_DEBUG
   if (nId >= 0) pkt_count++;
   #endif

/* check for consecutive duplicate RTP payload */

   bool fDuplicate = nId >= 0 && stream_info[nId].out_index_prev && stream_info[nId].out_index_prev == out_index && !memcmp(out_data, stream_info[nId].out_data_prev, out_index);

/* strip consecutive duplicates, if any, notes JHB Jan 2025:

   -normally pktlib packet/media worker threads and jitter buffers handle all forms of redundancy, but occasionally RTP payloads can be duplicated dozens of packets apart, for example if the sender is using high latency protocols for redundancy (e.g. GPRS Tunnelling Protocol) and pktlib doesn't catch it

   -for vps/sps/pps units duplication shouldn't matter but for slice units it might, also we need to avoid repeated FU packets that would cause an FU state mismatch
*/

   if (fDuplicate) { if (nId >= 0) stream_info[nId].duplicate_count++; return 0; }

   #ifdef INSERTION_DEBUG
   int xps_out_index = 0;
   #endif

/* SDP info fmtp field insertion of vps, sps, and/or pps NAL units, notes JHB Feb 2025:

   -xps is shorthand for any one or all of these

   -if SIP/SDP/SAP (out-of-band) xps info is sent by application caller, see if conditions are satisfied to insert in output bitstream

   -default behavior is to favor inband xps info if found in the RTP stream, but we need to handle cases where that isn't available and the streaming source is sending xps info out-of-band. VLC is a good example

   -typically inband xps NAL units are sent in initial RTP packets of a stream, before other unit types; therefore if application callers are concerned that inband xps NAL units may not be present, they should send SDP fmtp info with the first RTP payload. Note that at any time applications may call DSGetPayloadInfo() without specifying bitstream extraction to file and examine the NALU_Header in returned payload_info struct info to determine if inband xps NAL units are present and what SDP fmtp action may be required, then call DSGetPayloadInfo() again with appropriate uFlags and file extraction enabled
*/

   if ((fp_out || pInfo) && sdp_info && sdp_info->fmtp && strlen(sdp_info->fmtp)) {

      bool fInsertSDPInfo = true;  /* assume insertion */
      int num_xps, start_code_size;
      const uint8_t* p_start;

      if (codec_type == DS_CODEC_VIDEO_H265) {

         num_xps = sizeof(NAL_unit_start_code_xps_HEVC)/sizeof(NAL_unit_start_code_xps_HEVC[0]);
         p_start = &NAL_unit_start_code_HEVC[0];
         start_code_size = sizeof(NAL_unit_start_code_HEVC);
      }
      else {

         num_xps = sizeof(NAL_unit_start_code_xps_H264)/sizeof(NAL_unit_start_code_xps_H264[0]);
         p_start = &NAL_unit_start_code_H264[0];
         start_code_size = sizeof(NAL_unit_start_code_H264);
      }

      for (int i=0; i<num_xps; i++) {  /* see if NAL unit we extracted is an xps NAL unit */

         #define temp_size max(sizeof(NAL_unit_start_code_xps_HEVC[i]), sizeof(NAL_unit_start_code_xps_H264[i]))
         uint8_t temp[temp_size];
         for (int j=0; j<(int)temp_size; j++) temp[j] = out_data[j] & (codec_type == DS_CODEC_VIDEO_H264 ? 0x1f : 0xff);  /* for H.264 mask off NRI bits */

         if ((codec_type == DS_CODEC_VIDEO_H265 && !memcmp(temp, NAL_unit_start_code_xps_HEVC[i], sizeof(NAL_unit_start_code_xps_HEVC[i]))) ||  /* search bitstream for start code + xps NAL unit */
             (codec_type == DS_CODEC_VIDEO_H264 && !memcmp(temp, NAL_unit_start_code_xps_H264[i], sizeof(NAL_unit_start_code_xps_H264[i])))) {

         /* default behavior is to use inband xps info, if found, in favor of SDP xps info. Application callers can override this by applying the DS_PAYLOAD_INFO_IGNORE_INBAND_XPS flag */

            if (!(uFlags & DS_PAYLOAD_INFO_IGNORE_INBAND_XPS)) fInsertSDPInfo = false;  /* xps NAL unit found and DS_PAYLOAD_INFO_IGNORE_INBAND_XPS flag is not active so we are not inserting SDP info */
            break;
         }
      }

      if (fInsertSDPInfo) for (int i=(int)(sizeof(sprop_xps)/sizeof(sprop_xps[0]))-1; i>=0; i--) {  /* order insertions so vps is first in bitstream sequence */

         if (char* p = strstr(sdp_info->fmtp, sprop_xps[i])) {  /* search for xps-prop= in application supplied fmtp. See SDP info example in comments below, after code ends. Remember the "x" in xps can be v, s, or p */

            p += strlen(sprop_xps[i]);
            char* p2; if ((p2 = strstr(p, ";"))) *p2 = 0;  /* temporarily remove semicolon delimiter */

            std::string xprop_str = base64_decode(std::string(p), false);  /* decode base64 string to binary sequence. 2nd param controls whether to remove line breaks (currently we assume none in SDP info). base64_decode() is in apps/common/base64/base64.cpp */

            #ifdef BASE64_DEBUG
            char hexstr[1000] = "";
            for (int j=0; j<(int)xprop_str.length(); j++) sprintf(&hexstr[strlen(hexstr)], " 0x%x", (uint8_t)xprop_str[j]);
            printf("\n *** inside xps insert, fmtp field = %s decoded hex str = %s, len = %d \n", p, hexstr, (int)xprop_str.length());
            #endif

            if (p2) *p2 = ';';  /* restore temporarily removed semicolon delimiter */

            if (xprop_str.length()) {

               memmove(&out_data[start_code_size + xprop_str.length()], out_data, out_index);  /* shift current frame data right, allow for xps insertion */

               out_index += write_to_buffer(out_data, (uint8_t*)p_start, 0, start_code_size);
               out_index += write_to_buffer(out_data, (uint8_t*)&xprop_str[0], start_code_size, xprop_str.length());

               #ifdef INSERTION_DEBUG
               char tmpstr[2000] = "";
               for (int i=0; i<100; i++) sprintf(&tmpstr[strlen(tmpstr)], " 0x%x", out_data[i]);
               fprintf(stderr, "\n *** after xps insertion %s \n", tmpstr);

               xps_out_index += start_code_size + xprop_str.length();
               #endif

               if (nId >= 0) stream_info[nId].uXPS_outofband_inserted |= (1 << i);  /* set status bit for type of insertion made */
            }
         }
      }
   }

   #ifdef INSERTION_DEBUG
   if (xps_out_index) {

      printf("\n *** inserting xps info, out_index = %d, xps_out_index = %d \n", out_index, xps_out_index);

      char tmpstr[2000] = "";
      char fu_hdr_str[10] = "n/a";
      if (nal_unit_type == NAL_UNIT_FU) sprintf(fu_hdr_str, "0x%x", rtp_payload[2]);

      if (nId >= 0) sprintf(tmpstr, "output bitstream %d for packet #%d rtp len = %d, out_index = %d, NAL unit type = %d, FU header = %s, FU state mismatch count = %d, duplicate = %d, duplicate count = %d, NAL header format errors = %d, header =", nId, stream_info[nId].pkt_count+1, rtp_pyld_len, out_index, nal_unit_type, fu_hdr_str, stream_info[nId].fu_state_mismatch_count, fDuplicate, stream_info[nId].duplicate_count, stream_info[nId].nNAL_header_format_error_count);

      int len = min(out_index, 120);
      for (int i=0; i<len; i++) sprintf(&tmpstr[strlen(tmpstr)], " 0x%x", out_data[i]);
      fprintf(stderr, "\n *** %s \n", tmpstr);
   }
   #endif

/* show debug info if requested */

   if (nId >= 0 && (uFlags & DS_PAYLOAD_INFO_DEBUG_OUTPUT)) {

      char tmpstr[2000];
      char fu_hdr_str[10] = "n/a";
      if (nal_unit_type == NAL_UNIT_FU) sprintf(fu_hdr_str, "0x%x", rtp_payload[2]);
      else if (nal_unit_type == NAL_UNIT_FU_A) sprintf(fu_hdr_str, "0x%x", rtp_payload[1]);

      sprintf(tmpstr, "output bitstream %d for packet #%d rtp len = %d, out_index = %d, NAL unit type = %d, FU header = %s, FU state mismatch count = %d, duplicate = %d, duplicate count = %d, xps out-of-band info inserted = %d, NAL header format errors = %d, header =", nId, stream_info[nId].pkt_count+1, rtp_pyld_len, out_index, nal_unit_type, fu_hdr_str, stream_info[nId].fu_state_mismatch_count, fDuplicate, stream_info[nId].duplicate_count, stream_info[nId].uXPS_outofband_inserted, stream_info[nId].nNAL_header_format_error_count);

      int len;
      if (nal_unit_type == NAL_UNIT_AP) len = out_index;
      else if (nal_unit_type == NAL_UNIT_VPS_HEVC || nal_unit_type == NAL_UNIT_SPS_HEVC || nal_unit_type == NAL_UNIT_PPS_HEVC) len = min(out_index, 100);
      else len = min(out_index, 20);
      for (int i=0; i<len; i++) sprintf(&tmpstr[strlen(tmpstr)], " 0x%x", out_data[i]);
      fprintf(stderr, "\n *** %s \n", tmpstr);
   }

/* write output buffer to file, if requested */

   if (fp_out) {

      if ((ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &fp_out, NULL, (uintptr_t)out_data, out_index, DS_WRITE | DS_DATAFILE_USE_SEMAPHORE, NULL)) < 0) { Log_RT(2, "ERROR: DSPayloadInfo() --> extract_rtp_video() call to DSSaveDataFile() output write fails for %s output, ret_val = %d \n", errstr ? errstr : "", ret_val); return -1; }  /* return number of bytes written to file */
   }

/* copy output buffer to caller mem, if requested */

   if (pInfo) {

      memcpy(pInfo, out_data, out_index);
      ret_val = out_index;  /* return number of bytes copied to buffer */
   }

   if (ret_val == -1) ret_val = codec_type == DS_CODEC_VIDEO_H265 ? DS_PYLD_FMT_H265 : DS_PYLD_FMT_H264;  /* return format type if bitstream file or memory buffer output not requested */

   if (nId >= 0) {
   
      stream_info[nId].pkt_count++;  /* update count */ 

   /* save output buffer */

      memcpy(stream_info[nId].out_data_prev, out_data, out_index);
      stream_info[nId].out_index_prev = out_index;
      stream_info[nId].out_index_total += out_index;  /* keep track of long NAL unit frame sizes. These can easily exceed 200 kB when units are fragmented */
   }

   return ret_val;  /* return (i) payload format or (ii) number of bytes written to bitstream file and/or copied to mem buffer (depending on operation specified by uFlags) */
}

/* write bitstream data to output buffer

   -offset specifies where in buf to write data
   -len specifies amount of data written
   -error checks include starting offset and space available
*/

static int write_to_buffer(uint8_t* buf, uint8_t* data, int offset, int len) {

int amount_copied = 0;

   if (offset < MAX_RTP_PYLD_LEN) {  /* don't exceed max buffer size */

      amount_copied = min(len, MAX_RTP_PYLD_LEN - offset);  /* don't exceed space available */
      memcpy(&buf[offset], data, amount_copied);
   }

   return amount_copied;
}

/*
In the following SDP info example the "fmtp..." field would be in the sdp_info->fmtp string:

application/sdp
v=0
o=- 16958848648758400015 16958848648758400015 IN IP4 DESKTOP-6ZZUYP2
s=raccoon_test
i=N/A
c=IN IP4 192.168.1.2
t=0 0
a=tool:vlc 3.0.21
a=recvonly
a=type:broadcast
a=charset:UTF-8
m=audio 5004 RTP/AVP 14
b=AS:128
b=RR:0
a=rtpmap:14 MPA/90000/2
m=video 5006 RTP/AVP 96
b=RR:0
a=rtpmap:96 H265/90000
a=fmtp:96 tx-mode=SRST;profile-id=1;level-id=3;tier-flag=0;profile-space=0;sprop-vps=QAEMAf//AWAAAAMAkAAAAwAAAwB4lZgJ;sprop-sps=QgEBAWAAAAMAkAAAAwAAAwB4oAPAgBDlllZqvK4BAAADAAEAAAMAFAg=;sprop-pps=RAHBc9CJ
*/