/*
$Header: /root/Signalogic/DirectCore/lib/pktlib/validate_rtp_media_payload.cpp
 
Description

  validates an RTP media packet using DSGetPayloadInfo() voplib API

Notes

  -fully multithreaded, no locks, no semaphore
  -called by DSBufferPackets() pktlib API
  -may call Log_RT() API in diaglib
  -must be linked with pktlib and voplib. No dependencies on other SigSRF libs
 
Projects

  SigSRF, DirectCore
 
Copyright Signalogic Inc. 2024-2025

License

  Github SigSRF License, Version 1.1, https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

Revision History

  Created Dec 2024 JHB, separated and moved here from pktlib source
  Revised Feb 2025 JHB, add nID and output file param in DSGetPayloadInfo() (NULL indicates not used)
*/

/* Linux and/or other OS includes */

#include <algorithm>
using namespace std;

/* SigSRF includes */

#include "pktlib.h"
#include "voplib.h"
#include "diaglib.h"  /* Log_RT() */
#include "validate_and_verify.h"

/* run-time stats Codecs items declared in packet_flow_media_proc.c */

extern uint32_t uNumDamagedFrames[], uNumBandwidthEfficientFrames[], uNumOctetAlignFrames[], uNumCompactFrames[], uNumHeaderFullFrames[], uNumHeaderFullOnlyFrames[], uNumAMRWBIOCompatibilityFrames[];

/* validate_rtp_media_payload() error-checks and inspects an RTP media payload, optionally adds info to run-time stats

  Arguments

  -payload should point to start of an RTP payload (i.e. after the UDP header in an IP packet)
  -uFlags should contain relevant DSBufferPackets() API flags as documented in pktlib.h
  -codec_type should specify a valid codec_types enum as defined in shared_include/codec.h
  -rtp_hdr_len should give the size of the RTP header in bytes
  -rtp_payload_size should give the size of the RTP payload in bytes
  -n is a channel number assigned and managed by pktlib. n is ignored if given as -1, in which case run-time stats update is omitted
  -pDamagedFrame[] should point to an array of bools to be set true or false to indicate damaged frames. The number of pDamagedFrame[] entries returned is given by payload_info.NumFrames. The max size of the array should be MAX_PAYLOAD_FRAMES (defined in voplib.h). pDamagedFrame can be NULL if not used
  -payload_info should optionally point to a PAYLOAD_INFO struct to receive detailed payload parsing and inspection info from DSGetPayloadInfo(). payload_info can be NULL if not used

  Return value is the payload format (see DS_PYLD_FMT_XXX definitions in voplib.h), or < 0 for an error condition

  Notes

  -PAYLOAD_INFO struct definition is in voplib.h
  -extensive error-checking is performed by DSGetPayloadInfo() in voplib
    -AMR payloads RTP payload sizes are sanity checked vs the number of data and padding bits specified by the bitrate FT field in the payload header ToC
    -EVS payloads are thoroughly checked per section A.1, RTP Header Usage, in the EVS spec
  -damaged frame testing
    -for AMR test with 81786.4289256.478164.pcap (173 damaged frames out of 4962), 85236.4284266.158664.pcap (354 / 48988), 65446.4425483.49980.pcap (135 / 58750), and crash1.pcap (171 / 32082)
    -for EVS test with 37_anon.pcapng (2 damaged frames out of 14594)
*/

int validate_rtp_media_payload(uint8_t* payload, unsigned int uFlags, codec_types codec_type, int rtp_hdr_len, int rtp_payload_size, int n, bool pDamagedFrame[], PAYLOAD_INFO* payload_info) {

int ret_val;
PAYLOAD_INFO PayloadInfo;  /* local payload info struct if needed */

   if (!payload) {
      Log_RT(2, "ERROR: validate_rtp_media_payload() says media payload ptr is NULL, uFlags = 0x%x \n", uFlags);
      return -2;
   }

   if (!payload_info) {  /* use local struct if caller doesn't give a PAYLOAD_INFO ptr */
      memset(&PayloadInfo, 0, sizeof(PayloadInfo));  /* init all items to zero */
      payload_info = &PayloadInfo;
   }

   if ((ret_val = DSGetPayloadInfo(codec_type, DS_CODEC_INFO_TYPE | (uFlags & DS_PKTLIB_SUPPRESS_WARNING_ERROR_MSG ? DS_VOPLIB_SUPPRESS_WARNING_ERROR_MSG : 0), &payload[rtp_hdr_len], rtp_payload_size, payload_info, NULL, -1, NULL, NULL)) < 0) return ret_val;  /* DSGetPayloadInfo() is in voplib, suppress error / messages based on uFlags */

/* analyze PAYLOAD_INFO payload format and ToC info returned by DSGetPayloadInfo(), JHB Dec 2024 */

   uint8_t uQBitpos = 0;  /* if Q bit in ToC byte is zero then frame is damaged. We use uQBitpos to isolate the Q bit (if the payload format includes one) */

   if (isAMRCodec(codec_type)) {  /* AMR NB, WB, or WB+. isXXXCodec() macros are defined in shared_include/codec.h */

      uQBitpos = 4;  /* all AMR frames have a Q bit */

      if (n >= 0) {  /* update run-time stats if channel number >= 0 */

         if (payload_info->uFormat == DS_PYLD_FMT_OCTETALIGN) uNumOctetAlignFrames[n]++;  /* DS_PYLD_FMT_XXX definitions are in voplib.h */
         else if (payload_info->uFormat == DS_PYLD_FMT_BANDWIDTHEFFICIENT) uNumBandwidthEfficientFrames[n]++;
      }
   }
   else if (isEVSCodec(codec_type)) {  /* EVS */

      if (payload_info->voice.fAMRWB_IO_Mode) {  /* AMR-WB IO compatibility mode */

         if (payload_info->uFormat != DS_PYLD_FMT_COMPACT) uQBitpos = 0x10;  /* for EVS only AMR-WB IO compatibility mode frames have a Q bit */
         if (n >= 0) uNumAMRWBIOCompatibilityFrames[n]++;
      }

      if (n >= 0) {  /* update run-time stats if channel number >= 0 */

         if (payload_info->uFormat == DS_PYLD_FMT_COMPACT) uNumCompactFrames[n]++;
         else if (payload_info->uFormat == DS_PYLD_FMT_FULL) uNumHeaderFullFrames[n]++;
         else if (payload_info->uFormat == DS_PYLD_FMT_HF_ONLY) uNumHeaderFullOnlyFrames[n]++;
      }
   }

/* look for damaged frames: if uQBitpos is non-zero use it to check ToC byte(s) Q bit value */

   for (int i=0; i<payload_info->NumFrames; i++) {  /* NumFrames can't be zero and can't be != 1 in compact format, but no need to error-check here, already done in DSGetPayloadInfo() */

      if (uQBitpos && !(payload_info->voice.ToC[i] & uQBitpos)) {

         if (pDamagedFrame) pDamagedFrame[i] = true;
         if (n >= 0) uNumDamagedFrames[n]++;  /* update run-time stats if channel number >= 0 */
      }
      else if (pDamagedFrame) pDamagedFrame[i] = false;
   }

   return ret_val;  /* return payload format (see DS_PYLD_FMT_XXX definitions in voplib.h) */
}
