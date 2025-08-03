/*
$Header: /root/Signalogic/DirectCore/apps/SigC641x_C667x/mediaTest/transcoder_control.c

Description:

  Functions for controlling transcoder, include:
    - transcoder initialization
    - session set up
    - session tear down
    - additional control plane message passing

 Copyright (C) Signalogic Inc. 2015-2025

 Revision History
 
  Created Sep 2015 CKJ
  Modified 2016 SC, added session_data.ha_index=0. Might need to change it when standby case is being tested 
  Modified Jun 2016 JHB, modified dtmf_type() to accept dsp_tester-style numeric format entries
  Modified May 2017 CJ, Unix, Windows, and Legacy Mac formatted config files are now supported 
  Modified Aug 2017 CJ, use str_remove_whitespace() in all cases after getting config file value to relax restrictions on config file format
  Modified Aug 2017 JHB, remove case sensitivity for value entry (not fields, values only)
  Modified Mar 2018 JHB, use _X86 instead of _X86_ (to be consistent with shared_include / coCPU usage)
  Modified Mar 2018 JHB, modify parse_codec_params() to handle "codec_type" field.  Modify parse_codec_config() to default to EVS if codec_type field not given
  Modified Apr 2018 JHB, add MELPe to codec_type()
  Modified Aug 2018 CKJ, add session configuration file parsing for merge term
  Modified Sep 2018 JHB, add delay item to TERMINATION_INFO struct (in msec)
  Modified Sep 2018 JHB, use strtoul() for merge_mode entry, as these values can be 0x100001 or 0x300001 or similar combinations of MERGE_MODE_xxx flags in pktlib.c
  Modified Nov 2018 JHB, change "merge_term" field to "group_term", add "output_buffer_interval" field
  Modified Dec 2018 CKJ, add codec config file fields to support AMR-WB+, including "limiter", "low_complexity", "isf", "mode", and "bitrate_plus"
  Modified Jan 2020 JHB, add default settings for TERM_PKT_REPAIR_ENABLE and max_pkt_repair_ptimes
  Modified Apr 2021 JHB, add parsing of "header_format" for codec config files. This currently applies to AMR and EVS
  Modified Feb 2022 JHB, fix issues with comments and extra whitespace in codec config files, update other config/session file parsing with these changes also (look for JHB Feb2022)
  Modified Mar 2022 JHB, moved str_remove_whitespace() to dsstring.h
  Modified Sep 2022 JHB, add flexibility and readability improvements to "header_format" field in codec config files. Add a "framesize" option for codec config files, this allows a framesize to be specified for pass-thru cases such as .cod to .pcap, where no encoding/decoding is specified
  Modified Oct 2022 JHB, add payload_shift field in codec config files for debug/test purposes. +/- shift amount and filter flags can be specified (see definitions in shared_include/session.h). use strtoul() instead of atoi() to allow hex entry
  Modified Oct 2023 JHB, fix bug in handling of keywords for header_format field (full, compact, octet-align, and bandwidth-efficient). Applies to encoding only
  Modified Dec 2023 JHB, remove code not used in mediaMin and hello_codec builds, include transcoding.h for mediaTest build only
  Modified Dec 2023 JHB, look for input_sample_rate field in parse_codec_params(), this field can be used to supply an sampling rate for input raw audio files with no header
  Modified Apr 2024 JHB, comments and notes only
  Modified May 2024 JHB, change #ifdef _X86 to #if defined(_X86) || defined(_ARM)
  Modified Jun 2024 JHB, in codec_type() implement H.26x codecs
  Modified Jul 2024 JHB, use strcasestr in codec_type(), dtmf_type(), and ec_type()
  Modified Sep 2024 JHB, replace codec_type() with DSGetCodecInfo() to get codec type from standard string name, add DS_MEDIA_TYPE_AUDIO to media_type(). Codec names are in get_codec_type_from_name() in shared_include/codec.h
  Modified Oct 2024 JHB, add hf-only payload format support for EVS codec
  Modified Nov 2024 JHB, include directcore.h (no longer implicitly included in other header files), use IPvN_ADDR_XXX definitions in pktlib.h
  Modified Dec 2024 JHB, change "header_format" to "payload_format" due to renaming in codec_test_params_t struct in mediaTest.h 
  Modified Dec 2024 JHB, include <algorithm> and use std namespace if __cplusplus defined
  Modified Jun 2025 JHB, for ip_addr, voice_attributes, and TERMINATION_INFO structs, remove "u" and "attr" intermediate addressing for unions and address individual structs directly
*/

/* Linux includes */

#ifdef __cplusplus
  #include <algorithm>
  using namespace std;
#endif

#include <arpa/inet.h>

/* SigSRF includes */

#include "directcore.h"  /* DirectCore APIs */
#include "voplib.h"      /* codec related definitions */

#ifdef _MEDIATEST_
  #include "shared_include/transcoding.h"
#endif

/* app support header files */

#include "mediaTest.h"   /* application definitions */
#include "dsstring.h"    /* bring in str_remove_whitespace(), JHB Mar 2022 */

typedef struct session_params_s {
   SESSION_DATA session_data;
   int node_id;
} session_params_t;

typedef struct stack_s stack_t;

struct stack_s {
   int id;
   session_params_t param;
   stack_t *next;
};

#if 0
/* generic whitespace removal func, JHB Jun 2016 */

static void str_remove_whitespace(char* str) {

unsigned int i;
char* p = str;

   for (i=0; i<strlen(str); i++) if (str[i] != ' ') *p++ = str[i];  /* in-place whitespace removal */

   *p = 0;  /* terminating zero */
}
#endif

static int media_type(char *value) {

char tmpstr[256];

   strcpy(tmpstr, value);
   strupr(tmpstr);
   
   if (strstr(value,"VOICE"))
      return DS_MEDIA_TYPE_VOICE;
   else if (strstr(value,"AUDIO"))
      return DS_MEDIA_TYPE_AUDIO;
   else if (strstr(value,"VIDEO"))
      return DS_MEDIA_TYPE_VIDEO;
   else return -1;
}

#if 0  /* a version of this is now in shared_include/codec.h */

static int codec_type(const char* codecstr) {

   if (strcasestr(codecstr,"NONE"))
      return DS_CODEC_NONE;
   else if (strcasestr(codecstr,"G711_ULAW") || strcasestr(codecstr,"G711u"))
      return DS_CODEC_VOICE_G711_ULAW;
   else if (strcasestr(codecstr,"G711_ALAW") || strcasestr(codecstr,"G711a"))
      return DS_CODEC_VOICE_G711_ALAW;
   else if (strcasestr(codecstr,"G711_ULAW"))
      return DS_CODEC_VOICE_G711_ULAW;
   else if (strcasestr(codecstr,"G711_WB_ULAW") || strcasestr(codecstr,"G711-WBu"))
      return DS_CODEC_VOICE_G711_WB_ULAW;
   else if (strcasestr(codecstr,"G711_WB_ALAW") || strcasestr(codecstr,"G711-WBa"))
      return DS_CODEC_VOICE_G711_WB_ALAW;
   else if (strcasestr(codecstr,"G726"))
      return DS_CODEC_VOICE_G726;
   else if (strcasestr(codecstr,"G729AB"))
      return DS_CODEC_VOICE_G729AB;
   else if (strcasestr(codecstr,"G723"))
      return DS_CODEC_VOICE_G723;
   else if (strcasestr(codecstr,"G722"))
      return DS_CODEC_VOICE_G722;
   else if (strcasestr(codecstr,"AMR_NB") || strcasestr(codecstr,"AMR-NB"))
      return DS_CODEC_VOICE_AMR_NB;
   else if (strcasestr(codecstr,"AMR_WB_PLUS") || strcasestr(codecstr,"AMR-WB+"))  /* needs to be before AMR-WB */
      return DS_CODEC_VOICE_AMR_WB_PLUS;
   else if (strcasestr(codecstr,"AMR_WB") || strcasestr(codecstr,"AMR-WB"))
      return DS_CODEC_VOICE_AMR_WB;
   else if (strcasestr(codecstr,"EVRCA"))
      return DS_CODEC_VOICE_EVRC;
   else if (strcasestr(codecstr,"ILBC"))
      return DS_CODEC_VOICE_ILBC;
   else if (strcasestr(codecstr,"ISAC"))
      return DS_CODEC_VOICE_ISAC;
   else if (strcasestr(codecstr,"OPUS"))
      return DS_CODEC_VOICE_OPUS;
   else if (strcasestr(codecstr,"EVRCB"))
      return DS_CODEC_VOICE_EVRCB;
   else if (strcasestr(codecstr,"GSMFR"))
      return DS_CODEC_VOICE_GSMFR;
   else if (strcasestr(codecstr,"GSMEFR"))
      return DS_CODEC_VOICE_GSMEFR;
   else if (strcasestr(codecstr,"EVRCNW"))
      return DS_CODEC_VOICE_EVRC_NW;
   else if (strcasestr(codecstr,"CLEARMODE"))
      return DS_CODEC_VOICE_CLEARMODE;
   else if (strcasestr(codecstr,"EVS"))
      return DS_CODEC_VOICE_EVS;
   else if (strcasestr(codecstr,"MELPe"))  /* added Apr 2018, JHB */
      return DS_CODEC_VOICE_MELPE;
   else if (strcasestr(codecstr,"L16"))
      return DS_CODEC_AUDIO_L16;
   else if (strcasestr(codecstr,"MP3"))
      return DS_CODEC_VIDEO_MP3;
   else if (strcasestr(codecstr,"MPEG2"))
      return DS_CODEC_VIDEO_MPEG2;
   else if (strcasestr(codecstr,"H.263"))
      return DS_CODEC_VIDEO_H263;
   else if (strcasestr(codecstr,"H.264"))
      return DS_CODEC_VIDEO_H264;
   else if (strcasestr(codecstr,"H.265"))
      return DS_CODEC_VIDEO_H265;
   else if (strcasestr(codecstr,"VP8"))
      return DS_CODEC_VIDEO_VP8;
   else if (strcasestr(codecstr,"VP9"))
      return DS_CODEC_VIDEO_VP9;

   else return DS_CODEC_NONE;
}
#endif

/* modified to handle dsp_tester format entries, JHB Jun 2016 */

static int dtmf_type(const char* dtmfstr) {

   if (strcasestr(dtmfstr, "NONE") || strcasestr(dtmfstr, "0")) return DS_DTMF_NONE;
   else if (strcasestr(dtmfstr, "RTP") || strcasestr(dtmfstr, "1")) return DS_DTMF_RTP;
   else if (strcasestr(dtmfstr, "TONE") || strcasestr(dtmfstr, "2")) return DS_DTMF_TONE;
   else if (strcasestr(dtmfstr, "STRIP") || strcasestr(dtmfstr, "4")) return DS_DTMF_SIP_INFO;
   else if (strcasestr(dtmfstr, "SIP_INFO") || strcasestr(dtmfstr, "8")) return DS_DTMF_SIP_INFO;
   else if (strcasestr(dtmfstr, "RTP+SIP_INFO") || strcasestr(dtmfstr, "RTP|SIP_INFO") || strcasestr(dtmfstr, "9")) return DS_DTMF_RTP | DS_DTMF_SIP_INFO;  /* note -- whitespace has already been removed. See usage of str_remove_whitespace() below */

   else return DS_DTMF_NONE;  /* default */
}

int ec_type(const char* ecstr) {

   if (strcasestr(ecstr,"NONE"))
      return DS_EC_NONE;
   else if (strcasestr(ecstr,"TI_LECc"))
      return DS_EC_TI_LEC;
   else if (strcasestr(ecstr,"TI_LEC_ACOUSTIC"))
      return DS_EC_TI_LEC_ACOUSTIC;

   else return DS_EC_NONE;
}

/* added IPv6 parsing suport for parse_session_config_line() and parse_session_data(), JHB May2017 */

#define ALLOW_IPV6

#ifdef ALLOW_IPV6

#define AF_INET4             AF_INET
#define DS_REMOTE_IP_ADDR    0
#define DS_LOCAL_IP_ADDR     1
#define DS_TERM1             0X100
#define DS_TERM2             0x200
#define DS_GROUP_TERM        0x400
#define DS_SESSION_DATA      0
#define DS_SESSION_PARAMS_T  0x10000

int inet_pton_ex(char* name, char* value, void* params, UINT uFlags) {

uint32_t ipv4_addr;
uint8_t ipv6_addr[16];
char tmpstr[30];

   if (inet_pton(AF_INET4, value, &ipv4_addr) > 0) {
   
      if (uFlags & DS_LOCAL_IP_ADDR) {

         if (uFlags & DS_GROUP_TERM) {

            if (uFlags & DS_SESSION_PARAMS_T) {
               ((session_params_t*)params)->session_data.group_term.local_ip.uIpv4 = ipv4_addr;  /* per Linux doc, inet_pton() with AF_INET will return an error condition if input string has invalid format or more than 4 digits, JHB May2017 */
               ((session_params_t*)params)->session_data.group_term.local_ip.type = IPV4;
            }
            else {
               ((SESSION_DATA*)params)->group_term.local_ip.uIpv4 = ipv4_addr;
               ((SESSION_DATA*)params)->group_term.local_ip.type = IPV4;
            }
         }
         else if (uFlags & DS_TERM2) {

            if (uFlags & DS_SESSION_PARAMS_T) {
               ((session_params_t*)params)->session_data.term2.local_ip.uIpv4 = ipv4_addr;  /* per Linux doc, inet_pton() with AF_INET will return an error condition if input string has invalid format or more than 4 digits, JHB May2017 */
               ((session_params_t*)params)->session_data.term2.local_ip.type = IPV4;
            }
            else {
               ((SESSION_DATA*)params)->term2.local_ip.uIpv4 = ipv4_addr;
               ((SESSION_DATA*)params)->term2.local_ip.type = IPV4;
            }
         }
         else {

            if (uFlags & DS_SESSION_PARAMS_T) {
               ((session_params_t*)params)->session_data.term1.local_ip.uIpv4 = ipv4_addr;
               ((session_params_t*)params)->session_data.term1.local_ip.type = IPV4;
            }
            else {
               ((SESSION_DATA*)params)->term1.local_ip.uIpv4 = ipv4_addr;
               ((SESSION_DATA*)params)->term1.local_ip.type = IPV4;
            }
         }
      }
      else {

         if (uFlags & DS_GROUP_TERM) {

            if (uFlags & DS_SESSION_PARAMS_T) {
               ((session_params_t*)params)->session_data.group_term.remote_ip.uIpv4 = ipv4_addr;
               ((session_params_t*)params)->session_data.group_term.remote_ip.type = IPV4;
            }
            else {
               ((SESSION_DATA*)params)->group_term.remote_ip.uIpv4 = ipv4_addr;
               ((SESSION_DATA*)params)->group_term.remote_ip.type = IPV4;
            }
         }
         else if (uFlags & DS_TERM2) {

            if (uFlags & DS_SESSION_PARAMS_T) {
               ((session_params_t*)params)->session_data.term2.remote_ip.uIpv4 = ipv4_addr;
               ((session_params_t*)params)->session_data.term2.remote_ip.type = IPV4;
            }
            else {
               ((SESSION_DATA*)params)->term2.remote_ip.uIpv4 = ipv4_addr;
               ((SESSION_DATA*)params)->term2.remote_ip.type = IPV4;
            }
         }
         else {

            if (uFlags & DS_SESSION_PARAMS_T) {
               ((session_params_t*)params)->session_data.term1.remote_ip.uIpv4 = ipv4_addr;
               ((session_params_t*)params)->session_data.term1.remote_ip.type = IPV4;
            }
            else {
               ((SESSION_DATA*)params)->term1.remote_ip.uIpv4 = ipv4_addr;
               ((SESSION_DATA*)params)->term1.remote_ip.type = IPV4;
            }
         }
      }
   }
   else if (inet_pton(AF_INET6, value, &ipv6_addr) > 0) {

      if (uFlags & DS_LOCAL_IP_ADDR) {

         if (uFlags & DS_GROUP_TERM) {

            if (uFlags & DS_SESSION_PARAMS_T) {
               memcpy(&((session_params_t*)params)->session_data.group_term.local_ip.ipv6, &ipv6_addr, IPV6_ADDR_LEN);
               ((session_params_t*)params)->session_data.group_term.local_ip.type = IPV6;
            }
            else {
               memcpy(&((SESSION_DATA*)params)->group_term.local_ip.ipv6, &ipv6_addr, IPV6_ADDR_LEN);
               ((SESSION_DATA*)params)->group_term.local_ip.type = IPV6;
            }
         }
         else if (uFlags & DS_TERM2) {

            if (uFlags & DS_SESSION_PARAMS_T) {
               memcpy(&((session_params_t*)params)->session_data.term2.local_ip.ipv6, &ipv6_addr, IPV6_ADDR_LEN);
               ((session_params_t*)params)->session_data.term2.local_ip.type = IPV6;
            }
            else {
               memcpy(&((SESSION_DATA*)params)->term2.local_ip.ipv6, &ipv6_addr, IPV6_ADDR_LEN);
               ((SESSION_DATA*)params)->term2.local_ip.type = IPV6;
            }
         }
         else {
            if (uFlags & DS_SESSION_PARAMS_T) {
               memcpy(&((session_params_t*)params)->session_data.term1.local_ip.ipv6, &ipv6_addr, IPV6_ADDR_LEN);
               ((session_params_t*)params)->session_data.term1.local_ip.type = IPV6;
            }
            else {
               memcpy(&((SESSION_DATA*)params)->term1.local_ip.ipv6, &ipv6_addr, IPV6_ADDR_LEN);
               ((SESSION_DATA*)params)->term1.local_ip.type = IPV6;
            }
         }
      }
      else {

         if (uFlags & DS_GROUP_TERM) {

            if (uFlags & DS_SESSION_PARAMS_T) {
               memcpy(&((session_params_t*)params)->session_data.group_term.remote_ip.ipv6, &ipv6_addr, IPV6_ADDR_LEN);
               ((session_params_t*)params)->session_data.group_term.remote_ip.type = IPV6;
            }
            else {
               memcpy(&((SESSION_DATA*)params)->group_term.remote_ip.ipv6, &ipv6_addr, IPV6_ADDR_LEN);
               ((SESSION_DATA*)params)->group_term.remote_ip.type = IPV6;
            }
         }
         else if (uFlags & DS_TERM2) {

            if (uFlags & DS_SESSION_PARAMS_T) {
               memcpy(&((session_params_t*)params)->session_data.term2.remote_ip.ipv6, &ipv6_addr, IPV6_ADDR_LEN);
               ((session_params_t*)params)->session_data.term2.remote_ip.type = IPV6;
            }
            else {
               memcpy(&((SESSION_DATA*)params)->term2.remote_ip.ipv6, &ipv6_addr, IPV6_ADDR_LEN);
               ((SESSION_DATA*)params)->term2.remote_ip.type = IPV6;
            }
         }
         else {

            if (uFlags & DS_SESSION_PARAMS_T) {
               memcpy(&((session_params_t*)params)->session_data.term1.remote_ip.ipv6, &ipv6_addr, IPV6_ADDR_LEN);
               ((session_params_t*)params)->session_data.term1.remote_ip.type = IPV6;
            }
            else {
               memcpy(&((SESSION_DATA*)params)->term1.remote_ip.ipv6, &ipv6_addr, IPV6_ADDR_LEN);
               ((SESSION_DATA*)params)->term1.remote_ip.type = IPV6;
            }
         }
      }
   }
   else {

      if (uFlags & DS_SESSION_PARAMS_T) strcpy(tmpstr, "Session data");
      else strcpy(tmpstr, "Session config file");

      printf("%s parsing error, invalid %s = %s\n", tmpstr, name, value);
      return -1;
   }
   
   return 1;  /* return 1 on success, as does inet_pton() */
}

#endif  /* ALLOW_IPV6 */

/* Helper functions for session configuration file handling */

#define USESTRTOL  /* use strtol() or strtoul() as needed, instead of atoi().  For example, group_mode entries can be 0x100001 or 0x300001, JHB Sep 2018 */

static int parse_session_config_line(char *name, char *value, SESSION_DATA *params) 
{

char tmpstr[256];

   if (strstr(name, "term1.local_ip"))
   {
#ifndef ALLOW_IPV6
      params->term1.local_ip.uIpv4 = inet_addr(value);
#else
      if (inet_pton_ex(name, value, (void*)params, DS_TERM1 | DS_LOCAL_IP_ADDR | DS_SESSION_DATA) < 0) return -1;
#endif
   }
   else if (strstr(name, "term1.remote_ip"))
   {
#ifndef ALLOW_IPV6
      params->term1.remote_ip.uIpv4 = inet_addr(value);
#else
      if (inet_pton_ex(name, value, (void*)params, DS_TERM1 | DS_REMOTE_IP_ADDR | DS_SESSION_DATA) < 0) return -1;
#endif
   }
   else if (strstr(name, "term1.local_port"))
      params->term1.local_port = htons(atoi(value));
   else if (strstr(name, "term1.remote_port"))
      params->term1.remote_port = htons(atoi(value));
   else if (strstr(name, "term1.media_type"))
      params->term1.media_type = media_type(value);
   else if (strstr(name, "term1.codec_type"))
      #if 0
      params->term1.codec_type = codec_type(value);
      #else
      params->term1.codec_type = DSGetCodecInfo(0, DS_CODEC_INFO_TYPE_FROM_NAME, 0, 0, value);
      #endif
   else if (strstr(name, "term1.bitrate"))
      params->term1.bitrate = atoi(value);
   else if (strstr(name, "term1.ptime")) {

      params->term1.voice.ptime = atoi(value);
      params->term1.ptime = atoi(value);
   }
#if defined(_X86) || defined(_ARM)
   else if (strstr(name, "term1.buffer_interval") || strstr(name, "term1.input_buffer_interval"))
      params->term1.input_buffer_interval = atoi(value);
   else if (strstr(name, "term1.output_buffer_interval"))
      params->term1.output_buffer_interval = atoi(value);
#endif
   else if (strstr(name, "term1.rtp_payload_type"))
      params->term1.voice.rtp_payload_type = atoi(value);
   else if (strstr(name, "term1.dtmf_type"))
      params->term1.voice.dtmf_mode = dtmf_type(value);
   else if (strstr(name, "term1.dtmf_payload_type"))
      params->term1.voice.dtmf_payload_type = atoi(value);
   else if (strstr(name, "term1.ec"))
      params->term1.voice.ec = ec_type(value);
   else if (strstr(name, "term1.octet_align"))
      params->term1.voice.amr.codec_flags = (atoi(value) ? DS_AMR_OCTET_ALIGN : 0);
   else if (strstr(name, "term1.evrc_format"))
      params->term1.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_PACKET_FORMAT_SHIFT;
   else if (strstr(name, "term1.evrc_bitrate"))
      params->term1.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_BITRATE_SHIFT;
   else if (strstr(name, "term1.evrc_mode"))
      params->term1.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_MODE_SHIFT;
   else if (strstr(name, "term1.opus_max_bitrate"))
      params->term1.voice.opus.codec_flags |= (atoi(value) & DS_OPUS_MAX_AVG_BITRATE);
   else if (strstr(name, "term1.opus_max_playback_rate"))
      params->term1.voice.opus.max_playback_rate = atoi(value);
   else if (strstr(name, "term1.opus_sprop_max_playback_rate"))
      params->term1.voice.opus.sprop_max_capture_rate = atoi(value);
   else if (strstr(name, "term1.opus_fec"))
      params->term1.voice.opus.codec_flags |= (atoi(value) ? (~0 & DS_OPUS_FEC) : 0);
   else if (strstr(name, "term1.vad"))
      params->term1.voice.flag = (atoi(value) ? VOICE_ATTR_FLAG_VAD : 0);
#if defined(_X86) || defined(_ARM)
   else if (strstr(name, "term1.buffer_depth")) {
      strcpy(tmpstr, value);
      strupr(tmpstr);
      if (strcmp(tmpstr, "DEFAULT") == 0 || strcmp(tmpstr, "ENABLE") == 0) params->term1.buffer_depth = 0;
      else if (strcmp(tmpstr, "DISABLE") == 0) params->term1.buffer_depth = -1;
      else params->term1.buffer_depth = atoi(value);
   }
   else if (strstr(name, "term1.dtx_handling")) {
      strcpy(tmpstr, value);
      strupr(tmpstr);
      if (strcmp(tmpstr, "DEFAULT") == 0 || strcmp(tmpstr, "ENABLE") == 0 || atoi(value) == 1) params->term1.uFlags |= TERM_DTX_ENABLE;
      else if (strcmp(tmpstr, "DISABLE") == 0 || atoi(value) <= 0) params->term1.uFlags &= ~TERM_DTX_ENABLE;
   }
   else if (strstr(name, "term1.delay")) {
      params->term1.delay = atoi(value);
   }
#endif
   else if (strstr(name, "term1.sample_rate"))
   {
#if defined(_X86) || defined(_ARM)
      params->term1.sample_rate = atoi(value);
#endif      
      if (params->term1.codec_type == DS_CODEC_VOICE_EVS)
      {
         if (atoi(value) > 3) params->term1.voice.evs.codec_flags |= DSGetCodecInfo(DS_CODEC_VOICE_EVS, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE, atoi(value), 0, NULL);  /* note -- codec_type should be replaced by params->term1.codec_type, which shd be parsed in a first pass */
         else if (atoi(value) >= 0) params->term1.voice.evs.codec_flags |= atoi(value);
      }
   }
   else if (strstr(name, "term1.evs_header_full") || strstr(name, "term1.header_format") || strstr(name, "term1.payload_format"))
      params->term1.voice.evs.codec_flags |= ((atoi(value)) ? DS_EVS_PACKET_FORMAT : 0);
#ifdef ENABLE_TERM_MODE_FIELD
   else if (strstr(name, "term1.mode"))
      params->term1.mode = (atoi(value) ? TERMINATION_MODE_IP_PORT_DONTCARE : 0);
#endif
   else if (strstr(name, "term1.merge_id") || strstr(name, "term1.group_id"))
      strcpy(params->term1.group_id, value);
   else if (strstr(name, "term1.merge_mode") || strstr(name, "term1.group_mode"))
#ifdef USESTRTOL
      params->term1.group_mode = strtoul(value, NULL, 0);
#else
      params->term1.group_mode = atoi(value);
#endif
   else if (strstr(name, "term2.local_ip"))
   {
#ifndef ALLOW_IPV6
      params->term2.local_ip.uIpv4 = inet_addr(value);
#else
      if (inet_pton_ex(name, value, (void*)params, DS_TERM2 | DS_LOCAL_IP_ADDR | DS_SESSION_DATA) < 0) return -1;
#endif
   }
   else if (strstr(name, "term2.remote_ip"))
   {
#ifndef ALLOW_IPV6
      params->term2.remote_ip.uIpv4 = inet_addr(value);
#else
      if (inet_pton_ex(name, value, (void*)params, DS_TERM2 | DS_REMOTE_IP_ADDR | DS_SESSION_DATA) < 0) return -1;
#endif
   }
   else if (strstr(name, "term2.local_port"))
      params->term2.local_port = htons(atoi(value));
   else if (strstr(name, "term2.remote_port"))
      params->term2.remote_port = htons(atoi(value));
   else if (strstr(name, "term2.media_type"))
      params->term2.media_type = media_type(value);
   else if (strstr(name, "term2.codec_type"))
      #if 0
      params->term2.codec_type = codec_type(value);
      #else
      params->term2.codec_type = DSGetCodecInfo(0, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_TYPE_FROM_NAME, 0, 0, value);
      #endif
   else if (strstr(name, "term2.bitrate"))
      params->term2.bitrate = atoi(value);
   else if (strstr(name, "term2.ptime")) {

      params->term2.voice.ptime = atoi(value);
      params->term2.ptime = atoi(value);
   }
#if defined(_X86) || defined(_ARM)
   else if (strstr(name, "term2.buffer_interval") || strstr(name, "term2.input_buffer_interval"))
      params->term2.input_buffer_interval = atoi(value);
   else if (strstr(name, "term2.output_buffer_interval"))
      params->term2.output_buffer_interval = atoi(value);
#endif
   else if (strstr(name, "term2.rtp_payload_type"))
      params->term2.voice.rtp_payload_type = atoi(value);
   else if (strstr(name, "term2.dtmf_type"))
      params->term2.voice.dtmf_mode = dtmf_type(value);
   else if (strstr(name, "term2.dtmf_payload_type"))
      params->term2.voice.dtmf_payload_type = atoi(value);
   else if (strstr(name, "term2.ec"))
      params->term2.voice.ec = ec_type(value);
   else if (strstr(name, "term2.octet_align"))
      params->term2.voice.amr.codec_flags = (atoi(value) ? DS_AMR_OCTET_ALIGN : 0);
   else if (strstr(name, "term2.evrc_format"))
      params->term2.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_PACKET_FORMAT_SHIFT;
   else if (strstr(name, "term2.evrc_bitrate"))
      params->term2.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_BITRATE_SHIFT;
   else if (strstr(name, "term2.evrc_mode"))
      params->term2.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_MODE_SHIFT;
   else if (strstr(name, "term2.opus_max_bitrate"))
      params->term2.voice.opus.codec_flags |= (atoi(value) & DS_OPUS_MAX_AVG_BITRATE);
   else if (strstr(name, "term2.opus_max_playback_rate"))
      params->term2.voice.opus.max_playback_rate = atoi(value);
   else if (strstr(name, "term2.opus_sprop_max_playback_rate"))
      params->term2.voice.opus.sprop_max_capture_rate = atoi(value);
   else if (strstr(name, "term2.opus_fec"))
      params->term2.voice.opus.codec_flags |= (atoi(value) ? (~0 & DS_OPUS_FEC) : 0);
   else if (strstr(name, "term2.vad"))
      params->term2.voice.flag = (atoi(value) ? VOICE_ATTR_FLAG_VAD : 0);
#if defined(_X86) || defined(_ARM)
   else if (strstr(name, "term2.buffer_depth")) {
      strcpy(tmpstr, value);
      strupr(tmpstr);
      if (strcmp(tmpstr, "DEFAULT") == 0 || strcmp(tmpstr, "ENABLE") == 0) params->term2.buffer_depth = 0;
      else if (strcmp(tmpstr, "DISABLE") == 0) params->term2.buffer_depth = -1;
      else params->term2.buffer_depth = atoi(value);
   }
   else if (strstr(name, "term2.dtx_handling")) {
      strcpy(tmpstr, value);
      strupr(tmpstr);
      if (strcmp(tmpstr, "DEFAULT") == 0 || strcmp(tmpstr, "ENABLE") == 0 || atoi(value) == 1) params->term2.uFlags |= TERM_DTX_ENABLE;
      else if (strcmp(tmpstr, "DISABLE") == 0 || atoi(value) <= 0) params->term2.uFlags &= ~TERM_DTX_ENABLE;
   }
   else if (strstr(name, "term2.delay")) {
      params->term2.delay = atoi(value);
   }
#endif
   else if (strstr(name, "term2.sample_rate"))
   {
#if defined(_X86) || defined(_ARM)
      params->term2.sample_rate = atoi(value);
#endif
      
      if (params->term2.codec_type == DS_CODEC_VOICE_EVS)
      {
         if (atoi(value) > 3) params->term2.voice.evs.codec_flags |= DSGetCodecInfo(DS_CODEC_VOICE_EVS, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE, atoi(value), 0, NULL);  /* note -- codec_type should be replaced by params->term2.codec_type, which shd be parsed in a first pass */
         else if (atoi(value) >= 0) params->term2.voice.evs.codec_flags |= atoi(value);
      }
   }
   else if (strstr(name, "term2.evs_header_full") || strstr(name, "term2.header_format") || strstr(name, "term2.payload_format"))
      params->term2.voice.evs.codec_flags |= ((atoi(value)) ? DS_EVS_PACKET_FORMAT : 0);
#ifdef ENABLE_TERM_MODE_FIELD
   else if (strstr(name, "term2.mode"))
      params->term2.mode = (atoi(value) ? TERMINATION_MODE_IP_PORT_DONTCARE : 0);
#endif
   else if (strstr(name, "term2.merge_id") || strstr(name, "term2.group_id"))
      strcpy(params->term2.group_id, value);
   else if (strstr(name, "term2.merge_mode") || strstr(name, "term2.group_mode"))
#ifdef USESTRTOL
      params->term2.group_mode = strtoul(value, NULL, 0);
#else
      params->term2.group_mode = atoi(value);
#endif
   else if (strstr(name, "merge_term.local_ip") || strstr(name, "group_term.local_ip"))
   {
#ifndef ALLOW_IPV6
      params->group_term.local_ip.uIpv4 = inet_addr(value);
#else
      if (inet_pton_ex(name, value, (void*)params, DS_GROUP_TERM | DS_LOCAL_IP_ADDR | DS_SESSION_DATA) < 0) return -1;
#endif
   }
   else if (strstr(name, "merge_term.remote_ip") || strstr(name, "group_term.remote_ip"))
   {
#ifndef ALLOW_IPV6
      params->group_term.remote_ip.uIpv4 = inet_addr(value);
#else
      if (inet_pton_ex(name, value, (void*)params, DS_GROUP_TERM | DS_REMOTE_IP_ADDR | DS_SESSION_DATA) < 0) return -1;
#endif
   }
   else if (strstr(name, "merge_term.local_port") || strstr(name, "group_term.local_port"))
      params->group_term.local_port = htons(atoi(value));
   else if (strstr(name, "merge_term.remote_port") || strstr(name, "group_term.remote_port"))
      params->group_term.remote_port = htons(atoi(value));
   else if (strstr(name, "merge_term.media_type") || strstr(name, "group_term.media_type"))
      params->group_term.media_type = media_type(value);
   else if (strstr(name, "merge_term.codec_type") || strstr(name, "group_term.codec_type"))
      #if 0
      params->group_term.codec_type = codec_type(value);
      #else
      params->group_term.codec_type = DSGetCodecInfo(0, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_TYPE_FROM_NAME, 0, 0, value);
      #endif
   else if (strstr(name, "merge_term.bitrate") || strstr(name, "group_term.bitrate"))
      params->group_term.bitrate = atoi(value);
   else if (strstr(name, "merge_term.ptime") || strstr(name, "group_term.ptime")) {

      params->group_term.voice.ptime = atoi(value);
      params->group_term.ptime = atoi(value);
   }
#if defined(_X86) || defined(_ARM)
   else if (strstr(name, "merge_term.buffer_interval") || strstr(name, "merge_term.input_buffer_interval") || strstr(name, "group_term.input_buffer_interval"))
      params->group_term.input_buffer_interval = atoi(value);
   else if (strstr(name, "merge_term.output_buffer_interval") || strstr(name, "group_term.output_buffer_interval"))
      params->group_term.output_buffer_interval = atoi(value);
#endif
   else if (strstr(name, "merge_term.rtp_payload_type") || strstr(name, "group_term.rtp_payload_type"))
      params->group_term.voice.rtp_payload_type = atoi(value);
   else if (strstr(name, "merge_term.dtmf_type") || strstr(name, "group_term.dtmf_type"))
      params->group_term.voice.dtmf_mode = dtmf_type(value);
   else if (strstr(name, "merge_term.dtmf_payload_type") || strstr(name, "group_term.dtmf_payload_type"))
      params->group_term.voice.dtmf_payload_type = atoi(value);
   else if (strstr(name, "merge_term.ec") || strstr(name, "group_term.ec"))
      params->group_term.voice.ec = ec_type(value);
   else if (strstr(name, "merge_term.octet_align"))
      params->group_term.voice.amr.codec_flags = (atoi(value) ? DS_AMR_OCTET_ALIGN : 0);
   else if (strstr(name, "merge_term.evrc_format") || strstr(name, "group_term.evrc_format"))
      params->group_term.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_PACKET_FORMAT_SHIFT;
   else if (strstr(name, "merge_term.evrc_bitrate") || strstr(name, "group_term.evrc_bitrate"))
      params->group_term.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_BITRATE_SHIFT;
   else if (strstr(name, "merge_term.evrc_mode") || strstr(name, "group_term.evrc_mode"))
      params->group_term.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_MODE_SHIFT;
   else if (strstr(name, "merge_term.opus_max_bitrate") || strstr(name, "group_term.opus_max_bitrate"))
      params->group_term.voice.opus.codec_flags |= (atoi(value) & DS_OPUS_MAX_AVG_BITRATE);
   else if (strstr(name, "merge_term.opus_max_playback_rate") || strstr(name, "group_term.opus_max_playback_rate"))
      params->group_term.voice.opus.max_playback_rate = atoi(value);
   else if (strstr(name, "merge_term.opus_sprop_max_playback_rate") || strstr(name, "group_term.opus_sprop_max_playback_rate"))
      params->group_term.voice.opus.sprop_max_capture_rate = atoi(value);
   else if (strstr(name, "merge_term.opus_fec") || strstr(name, "group_term.opus_fec"))
      params->group_term.voice.opus.codec_flags |= (atoi(value) ? (~0 & DS_OPUS_FEC) : 0);
   else if (strstr(name, "merge_term.vad") || strstr(name, "group_term.vad"))
      params->group_term.voice.flag = (atoi(value) ? VOICE_ATTR_FLAG_VAD : 0);
#if defined(_X86) || defined(_ARM)
   else if (strstr(name, "merge_term.buffer_depth") || strstr(name, "group_term.buffer_depth")) {
      strcpy(tmpstr, value);
      strupr(tmpstr);
      if (strcmp(tmpstr, "DEFAULT") == 0 || strcmp(tmpstr, "ENABLE") == 0) params->group_term.buffer_depth = 0;
      else if (strcmp(tmpstr, "DISABLE") == 0) params->group_term.buffer_depth = -1;
      else params->group_term.buffer_depth = atoi(value);
   }
   else if (strstr(name, "merge_term.dtx_handling") || strstr(name, "group_term.dtx_handling")) {
      strcpy(tmpstr, value);
      strupr(tmpstr);
      if (strcmp(tmpstr, "DEFAULT") == 0 || strcmp(tmpstr, "ENABLE") == 0 || atoi(value) == 1) params->group_term.uFlags |= TERM_DTX_ENABLE;
      else if (strcmp(tmpstr, "DISABLE") == 0 || atoi(value) <= 0) params->group_term.uFlags &= ~TERM_DTX_ENABLE;
   }
   else if (strstr(name, "merge_term.delay") || strstr(name, "group_term.delay")) {
      params->group_term.delay = atoi(value);
   }
#endif
   else if (strstr(name, "merge_term.sample_rate") || strstr(name, "group_term.sample_rate"))
   {
#if defined(_X86) || defined(_ARM)
      params->group_term.sample_rate = atoi(value);
#endif
      
      if (params->group_term.codec_type == DS_CODEC_VOICE_EVS)
      {
         if (atoi(value) > 3) params->group_term.voice.evs.codec_flags |= DSGetCodecInfo(DS_CODEC_VOICE_EVS, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE, atoi(value), 0, NULL);  /* note -- codec_type should be replaced by params->group_term.codec_type, which shd be parsed in a first pass */
         else if (atoi(value) >= 0) params->group_term.voice.evs.codec_flags |= atoi(value);
      }
   }
   else if (strstr(name, "merge_term.evs_header_full") || strstr(name, "merge_term.header_format") || strstr(name, "group_term.header_format"))
      params->group_term.voice.evs.codec_flags |= ((atoi(value)) ? DS_EVS_PACKET_FORMAT : 0);
#ifdef ENABLE_TERM_MODE_FIELD
   else if (strstr(name, "merge_term.mode") || strstr(name, "group_term.mode"))
      params->group_term.mode = (atoi(value) ? TERMINATION_MODE_IP_PORT_DONTCARE : 0);
#endif
   else if (strstr(name, "merge_term.merge_id") || strstr(name, "group_term.group_id"))
      strcpy(params->group_term.group_id, value);
   else if (strstr(name, "merge_term.merge_mode") || strstr(name, "group_term.group_mode"))
#ifdef USESTRTOL
      params->group_term.group_mode = strtoul(value, NULL, 0);
#else
      params->group_term.group_mode = atoi(value);
#endif
   
   return 0;
}

/* x86 packet test mode config file parsing */
int parse_session_config(FILE *fp, SESSION_DATA *params)
{
   char *name, *cmt_str, found_start = 0;
   char *value, string[1024];
   int len;

#if defined(_X86) || defined(_ARM)

/* set some default values before any config file lines are processed */

   params->term1.input_buffer_interval = -1;  /* default values */
   params->term2.input_buffer_interval = -1;
   params->term1.output_buffer_interval = -1;
   params->term2.output_buffer_interval = -1;

/* note following defaults set to zero normally should not be needed, because all of a SESSION_DATA struct should be init to zero prior to filling it in */

   params->term1.uFlags = TERM_DTX_ENABLE | TERM_SID_REPAIR_ENABLE | TERM_PKT_REPAIR_ENABLE | TERM_OVERRUN_SYNC_ENABLE;  /* default values */
   params->term2.uFlags = TERM_DTX_ENABLE | TERM_SID_REPAIR_ENABLE | TERM_PKT_REPAIR_ENABLE | TERM_OVERRUN_SYNC_ENABLE;
   params->term1.max_loss_ptimes = 3;
   params->term2.max_loss_ptimes = 3;
   params->term1.max_pkt_repair_ptimes = 4;
   params->term2.max_pkt_repair_ptimes = 4;

   params->term1.delay = 0;  /* default values */
   params->term2.delay = 0;
#endif

   while (1) 
   {
      memset(string, 0, sizeof(string));
      if (fgets(string, sizeof(string), fp) == NULL) return -1;

      /* remove comments */
      cmt_str = strchr(string, '#');
      if(cmt_str != NULL) memset(cmt_str, 0, strlen(cmt_str));

      /* skip empty lines */
      if ((strlen(string) == 0) || (strcmp(string,"\n") == 0)) continue;

      /* check for start of session data block */
      if (strstr(string,"start_of_session_data") || strstr(string,"session_data_start")) found_start = 1;

      /* skip all lines until the start of a session data block is found */
      if (!found_start) continue;

      /* check for end of session data block */
      if (strstr(string,"end_of_session_data") || strstr(string,"session_data_end")) break;

      len = strlen(string);

      /* ensure that the string terminates correctly */
      if (len > 1 && (string[len-1] == 0x0A || string[len-1] == 0x0D)) string[len-1] = '\0'; /* check for CR or LF as last char in string */
      if (len > 2 && string[len-2] == 0x0D) string[len-2] = '\0'; /* check for CR as 2nd to last char in string */
      
      str_remove_whitespace(string);  /* added CKJ Aug2017. Process string prior to searching for =, in case of trailing whitespace after removing comment, JHB Feb2022 */

      name = strtok_r((char*)&string, "=", &value);

      parse_session_config_line(name, value, params);
   }

   return 0;
}

/* x86 frame test mode config file parsing */
static int parse_term_data(char *name, char *value, FRAME_TEST_INFO *info)
{
   if (strcasestr(name, "media_type"))
      info->term.media_type = media_type(value);
   else if (strcasestr(name, "codec_type"))
      #if 0
      info->term.codec_type = codec_type(value);
      #else
      info->term.codec_type = DSGetCodecInfo(0, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_TYPE_FROM_NAME, 0, 0, value);
      #endif
   else if (strcasestr(name, "bitrate"))
      info->term.bitrate = atoi(value);
   else if (strcasestr(name, "ptime")) {

      info->term.voice.ptime = atoi(value);
      info->term.ptime = atoi(value);
   }
#if defined(_X86) || defined(_ARM)
   else if (strcasestr(name, "buffer_interval") || strcasestr(name, "input_buffer_interval"))
      info->term.input_buffer_interval = atoi(value);
   else if (strcasestr(name, "output_buffer_interval"))
      info->term.output_buffer_interval = atoi(value);
#endif
   else if (strcasestr(name, "ec") == 0)
      info->term.voice.ec = ec_type(value);
   else if (strcasestr(name, "octet_align"))
      info->term.voice.amr.codec_flags = (atoi(value) ? DS_AMR_OCTET_ALIGN : 0);
   else if (strcasestr(name, "evrc_format"))
      info->term.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_PACKET_FORMAT_SHIFT;
   else if (strcasestr(name, "evrc_bitrate"))
      info->term.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_BITRATE_SHIFT;
   else if (strcasestr(name, "evrc_mode"))
      info->term.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_MODE_SHIFT;
   else if (strcasestr(name, "opus_max_bitrate"))
      info->term.voice.opus.codec_flags |= (atoi(value) & DS_OPUS_MAX_AVG_BITRATE);
   else if (strcasestr(name, "opus_max_playback_rate"))
      info->term.voice.opus.max_playback_rate = atoi(value);
   else if (strcasestr(name, "opus_sprop_max_playback_rate"))
      info->term.voice.opus.sprop_max_capture_rate = atoi(value);
   else if (strcasestr(name, "opus_fec"))
      info->term.voice.opus.codec_flags |= (atoi(value) ? (~0 & DS_OPUS_FEC) : 0);
   else if (strcasestr(name, "vad"))
      info->term.voice.flag = (atoi(value) ? VOICE_ATTR_FLAG_VAD : 0);
   else if (strcasestr(name, "evs_sample_rate") || strcasestr(name, "sample_rate"))
   {
#if defined(_X86) || defined(_ARM)
      info->term.sample_rate = atoi(value);
#endif
      if (atoi(value) > 3) info->term.voice.evs.codec_flags |= DSGetCodecInfo(DS_CODEC_VOICE_EVS, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE, atoi(value), 0, NULL);  /* note -- codec_type should be replaced by params->term1.codec_type, which shd be parsed in a first pass */
      else if (atoi(value) >= 0) info->term.voice.evs.codec_flags |= atoi(value);
   }
   else if (strcasestr(name, "evs_header_full") || strcasestr(name, "header_format") || strcasestr(name, "payload_format") || strcasestr(name, "format")) {

      int val;   
      if (strcasestr(value, "full")) val = 1;
      else if (strcasestr(value, "compact")) val = 0;
      else if (strcasestr(value, "octet-aligned")) val = 1;
      else if (strcasestr(value, "octet-align")) val = 1;
      else if (strcasestr(value, "bandwidth-efficient")) val = 0;
      else if (strcasestr(value, "hf-only")) val = 2;
      else val = atoi(value);

      info->term.voice.evs.codec_flags |= (val ? DS_EVS_PACKET_FORMAT : 0);
   }
   else if (strcasestr(name, "encoder_file"))
   {
      info->encoder_file = (char *)malloc(strlen(value)+1);
      strcpy(info->encoder_file, value);
   }
   else if (strcasestr(name, "decoder_file"))
   {
      info->decoder_file = (char *)malloc(strlen(value)+1);
      strcpy(info->decoder_file, value);
   }
   
   return 0;
}

/* x86 frame test mode config file parsing */
int parse_codec_config_frame_mode(FILE *fp, FRAME_TEST_INFO *info)
{
   char *name, *cmt_str, found_start = 0;
   char *value, string[1024];
   int len;

   while (1) 
   {
      memset(string, 0, sizeof(string));
      if (fgets(string, sizeof(string), fp) == NULL) return -1;

      /* remove comments */
      cmt_str = strchr(string, '#');
      if (cmt_str != NULL) memset(cmt_str, 0, strlen(cmt_str));

      /* skip empty lines */
      if ((strlen(string) == 0) || (strcmp(string,"\n") == 0)) continue;
      
      /* check for start of codec data block */
      if (strstr(string,"start_of_codec_data") || strstr(string,"codec_data_start")) found_start = 1;
      
      /* skip all lines until the start of a codec data block is found */
      if (!found_start) continue;
      
      /* check for end of codec data block */
      if (strstr(string,"end_of_codec_data") || strstr(string,"codec_data_end")) break;

      len = strlen(string);
      
      /* ensure that the string terminates correctly */
      if (len > 1 && (string[len-1] == 0x0A || string[len-1] == 0x0D)) string[len-1] = '\0'; /* check for CR or LF as last char in string */
      if (len > 2 && string[len-2] == 0x0D) string[len-2] = '\0'; /* check for CR as 2nd to last char in string */

      str_remove_whitespace(string);  /* added CKJ Aug2017. Process string prior to searching for =, in case of trailing whitespace after removing comment, JHB Feb2022 */

      name = strtok_r((char*)&string, "=", &value);

      parse_term_data(name, value, info);
   }

   return 0;
}

/* parse codec test config line */
static void parse_codec_params(char *name, char *value, codec_test_params_t *params) 
{
   if (strcasestr(name, "bitrate"))  /* because "bitrateplus" is another field, CKJ Dec 2018 */
      params->bitrate = atoi(value);
   else if (strcasestr(name, "input_sample_rate"))  /* input_sample_rate field added to help with sampling rates for raw audio files with no header, JHB Dec 2023 */
      params->input_sample_rate = atoi(value);
   else if (strcasestr(name, "sample_rate"))
      params->sample_rate = atoi(value);
   else if (strcasestr(name, "dtx_enable"))
      params->dtx_enable = atoi(value);
   else if (strcasestr(name, "dtx_value"))
      params->dtx_value = atoi(value);
   else if (strcasestr(name, "rf_enable"))
      params->rf_enable = atoi(value);
   else if (strcasestr(name, "fec_indicator"))
      params->fec_indicator = atoi(value);
   else if (strcasestr(name, "fec_offset"))
      params->fec_offset = atoi(value);
   else if (strcasestr(name, "bandwidth_limit")) {
      if (strcasestr(value, "NB")) params->bandwidth_limit = 0;
      else if (strcasestr(value, "WB")) params->bandwidth_limit = 1;
      else if (strcasestr(value, "SWB")) params->bandwidth_limit = 2;
      else if (strcasestr(value, "FB")) params->bandwidth_limit = 3;
      else params->bandwidth_limit = atoi(value);
   }
/* add codec type, JHB Mar 2018 */
   else if (strcasestr(name, "codec_type"))
      #if 0
      params->codec_type = codec_type(value);
      #else
      params->codec_type = DSGetCodecInfo(0, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_TYPE_FROM_NAME, 0, 0, value);
      #endif
/* add num channels, JHB Mar 2018 */
   else if (strcasestr(name, "num_chan"))
      params->num_chan = atoi(value);
   else if (strcasestr(name, "Npp"))
      params->Npp = atoi(value);
   else if (strcasestr(name, "post"))
      params->post = atoi(value);
   else if (strcasestr(name, "bitDensity"))
      params->bitDensity = atoi(value);
   else if (strcasestr(name, "vad"))
      params->vad = atoi(value);
   else if (strcasestr(name, "uncompress"))
      params->uncompress = atoi(value);
   else if (strcasestr(name, "mono"))
      params->mono = atoi(value);
   else if (strcasestr(name, "limiter"))
      params->limiter = atoi(value);
   else if (strcasestr(name, "low_complexity"))
      params->low_complexity = atoi(value);
   else if (strcasestr(name, "isf"))
      params->isf = (float)atof(value);
   else if (strcasestr(name, "mode"))
      params->mode = atoi(value);
   else if (strcasestr(name, "bitrate_plus"))
   {
      params->bitrate_plus = (float)atof(value);
      params->bitrate = params->bitrate_plus*1000;
   }
/* add header formats, JHB Apr 2021 */
   else if (strcasestr(name, "header_full"))  {  /* EVS header full */
      params->payload_format = atoi(value);
   }
   else if (strcasestr(name, "header_compact") || strcasestr(name, "compact")) {
      params->payload_format = !atoi(value);
   }
   else if (strcasestr(name, "octet_align"))  /* AMR octet align */
      params->payload_format = atoi(value);
   else if (strcasestr(name, "bandwidth_efficient")) {
      params->payload_format = !atoi(value);
   }
   else if (strcasestr(name, "header_format") || strcasestr(name, "payload_format") || strcasestr(name, "format")) {  /* see "header format definitions" in voplib.h. "header_format" should no longer be used, use "payload_format" instead, JHB Dec 2024 */

      #if 0
      params->header_format = atoi(value);

      if (params->header_format != 0 && params->header_format != 1) {  /* added some flexibility and readability to header_format entry, JHB Sep 2022 */
         if (strcasestr(value, "full")) params->header_format = 1;
         else if (strcasestr(value, "compact")) params->header_format = 0;
         else if (strcasestr(value, "octet-aligned")) params->header_format = 1;
         else if (strcasestr(value, "octet-align")) params->header_format = 1;
         else if (strcasestr(value, "bandwidth-efficient")) params->header_format = 0;
      }
      #else  /* better way, seems that atoi("full") was producing a zero, JHB Oct 2023 */
      if (strcasestr(value, "compact")) params->payload_format = DS_PYLD_FMT_COMPACT;
      else if (strcasestr(value, "full")) params->payload_format = DS_PYLD_FMT_FULL;
      else if (strcasestr(value, "bandwidth-efficient")) params->payload_format = DS_PYLD_FMT_BANDWIDTHEFFICIENT;
      else if (strcasestr(value, "octet-align")) params->payload_format = DS_PYLD_FMT_OCTETALIGN;  /* this should catch "octet-aligned" also, JHB Dec 2024 */
      else if (strcasestr(value, "hf-only")) params->payload_format = DS_PYLD_FMT_HF_ONLY;
      else params->payload_format = atoi(value);
      #endif
   }
   else if (strcasestr(name, "framesize") || strcasestr(name, "frame_size")) {  /* added JHB Sep 2022 */
      params->framesize = atoi(value);
   }
   else if (strcasestr(name, "payload_shift")) {  /*shift codec RTP payloads for debug/test purposes, added JHB Sep 2022 */
      params->payload_shift = strtoul(value, NULL, 0);  /* we use strtoul() here to allow 0x entry. See comments in shared_include/session.h for range of shift amount and filter flags to control conditions of shift */
   }
}

/* parse x86 codec test mode config file */
void parse_codec_config(FILE *fp, codec_test_params_t *params)
{
   char *name, *cmt_str;
   char *value, string[1024];
   int len;

   params->codec_type = -1;  /* if mediaTest is run with old session config files that have a bitrate specified but not the codec type, then we default to EVS.  Note that codec_type can be "NONE" for pass-thru situations, for example no encoding, USB audio saved to wav file.  JHB Mar 2018 */
   params->bitrate = -1;
   params->num_chan = -1;
   params->payload_format = -1;
   params->framesize = -1;
   params->bandwidth_limit = -1;

   while (1) 
   {
      memset(string, 0, sizeof(string));
      if (fgets(string, sizeof(string), fp) == NULL) break;

      /* remove comments */
      cmt_str = strchr(string, '#');
      if(cmt_str != NULL) memset(cmt_str, 0, strlen(cmt_str));

      /* skip empty lines */
      if ((strlen(string) == 0) || (strcmp(string,"\n") == 0)) continue;

      len = strlen(string);

      /* ensure that the string terminates correctly */
      if (len > 1 && (string[len-1] == 0x0A || string[len-1] == 0x0D)) string[len-1] = '\0'; /* check for CR or LF as last char in string */
      if (len > 2 && string[len-2] == 0x0D) string[len-2] = '\0'; /* check for CR as 2nd to last char in string */

      str_remove_whitespace(string);  /* added CKJ Aug2017. Process string prior to searching for =, in case of trailing whitespace after removing comment, JHB Feb2022 */

      name = strtok_r((char*)&string, "=", &value);

      parse_codec_params(name, value, params);
   }
   
   if ((int)params->codec_type == -1) {

      if ((int)params->bitrate > 0) params->codec_type = DS_CODEC_VOICE_EVS;  /* default to EVS if bitrate is specified but codec_type is not */
      else params->codec_type = DS_CODEC_NONE;  /* if neither are specified then set codec type to none */
   }
   
   if ((int)params->num_chan == -1) params->num_chan = 1;
}


#ifdef _MEDIATEST_ /* code needed for mediaTest build but not mediaMin or hello_codec */

/* c66x config file parsing */
static int parse_session_data(char *name, char *value, session_params_t* params) 
{

char tmpstr[256];

#if defined(_X86) || defined(_ARM)
   params->session_data.term1.input_buffer_interval = -1;  /* default values */
   params->session_data.term2.input_buffer_interval = -1;
   params->session_data.term1.output_buffer_interval = -1;
   params->session_data.term2.output_buffer_interval = -1;
#endif

   if (strstr(name, "term1.local_ip"))
   {
#ifndef ALLOW_IPV6
      params->session_data.term1.local_ip.uIpv4 = inet_addr(value);
#else
      if (inet_pton_ex(name, value, (void*)params, DS_TERM1 | DS_LOCAL_IP_ADDR | DS_SESSION_PARAMS_T) < 0) return -1;
#endif
   }
   else if (strstr(name, "term1.remote_ip"))
   {
#ifndef ALLOW_IPV6
      params->session_data.term1.remote_ip.uIpv4 = inet_addr(value);
#else
      if (inet_pton_ex(name, value, (void*)params, DS_TERM1 | DS_REMOTE_IP_ADDR | DS_SESSION_PARAMS_T) < 0) return -1;
#endif
   }
   else if (strstr(name, "term1.local_port"))
      params->session_data.term1.local_port = atoi(value);
   else if (strstr(name, "term1.remote_port"))
      params->session_data.term1.remote_port = atoi(value);
   else if (strstr(name, "term1.media_type"))
      params->session_data.term1.media_type = media_type(value);
   else if (strstr(name, "term1.codec_type"))
      #if 0
      params->session_data.term1.codec_type = codec_type(value);
      #else
      params->session_data.term1.codec_type = DSGetCodecInfo(0, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_TYPE_FROM_NAME, 0, 0, value);
      #endif
   else if (strstr(name, "term1.bitrate"))
      params->session_data.term1.bitrate = atoi(value);
   else if (strstr(name, "term1.ptime")) {

      params->session_data.term1.voice.ptime = atoi(value);
      params->session_data.term1.ptime = atoi(value);
   }
#if defined(_X86) || defined(_ARM)
   else if (strstr(name, "term1.buffer_interval") || strstr(name, "term1.input_buffer_interval"))
      params->session_data.term1.input_buffer_interval = atoi(value);
   else if (strstr(name, "term1.output_buffer_interval"))
      params->session_data.term1.output_buffer_interval = atoi(value);
#endif
   else if (strstr(name, "term1.rtp_payload_type"))
      params->session_data.term1.voice.rtp_payload_type = atoi(value);
   else if (strstr(name, "term1.dtmf_type"))
      params->session_data.term1.voice.dtmf_mode = dtmf_type(value);
   else if (strstr(name, "term1.dtmf_payload_type"))
      params->session_data.term1.voice.dtmf_payload_type = atoi(value);
   else if (strstr(name, "term1.ec"))
      params->session_data.term1.voice.ec = ec_type(value);
   else if (strstr(name, "term1.octet_align"))
      params->session_data.term1.voice.amr.codec_flags = (atoi(value) ? DS_AMR_OCTET_ALIGN : 0);
   else if (strstr(name, "term1.evrc_format"))
      params->session_data.term1.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_PACKET_FORMAT_SHIFT;
   else if (strstr(name, "term1.evrc_bitrate"))
      params->session_data.term1.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_BITRATE_SHIFT;
   else if (strstr(name, "term1.evrc_mode"))
      params->session_data.term1.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_MODE_SHIFT;
   else if (strstr(name, "term1.opus_max_bitrate"))
      params->session_data.term1.voice.opus.codec_flags |= (atoi(value) & DS_OPUS_MAX_AVG_BITRATE);
   else if (strstr(name, "term1.opus_max_playback_rate"))
      params->session_data.term1.voice.opus.max_playback_rate = atoi(value);
   else if (strstr(name, "term1.opus_sprop_max_playback_rate"))
      params->session_data.term1.voice.opus.sprop_max_capture_rate = atoi(value);
   else if (strstr(name, "term1.opus_fec"))
      params->session_data.term1.voice.opus.codec_flags |= (atoi(value) ? (~0 & DS_OPUS_FEC) : 0);
   else if (strstr(name, "term1.vad"))
      params->session_data.term1.voice.flag = (atoi(value) ? VOICE_ATTR_FLAG_VAD : 0);
#if defined(_X86) || defined(_ARM)
   else if (strstr(name, "term1.buffer_depth")) {
      strcpy(tmpstr, value);
      strupr(tmpstr);
      if (strcmp(tmpstr, "DEFAULT") == 0 || strcmp(tmpstr, "ENABLE") == 0 || atoi(value) == 1) params->session_data.term1.buffer_depth = 0;
      else if (strcmp(tmpstr, "DISABLE") == 0) params->session_data.term1.buffer_depth = -1;
      else params->session_data.term1.buffer_depth = atoi(value);
   }
   else if (strstr(name, "term1.dtx_handling")) {
      strcpy(tmpstr, value);
      strupr(tmpstr);
      if (strcmp(tmpstr, "DEFAULT") == 0 || strcmp(tmpstr, "ENABLE") == 0) params->session_data.term1.uFlags |= TERM_DTX_ENABLE;
      else if (strcmp(tmpstr, "DISABLE") == 0 || atoi(value) <= 0) params->session_data.term1.uFlags &= ~TERM_DTX_ENABLE;
   }
   else if (strstr(name, "term1.delay")) {
      params->session_data.term1.delay = atoi(value);
   }
#endif
   else if (strstr(name, "term1.evs_sample_rate") || strstr(name, "term1.sample_rate"))
   {
#if defined(_X86) || defined(_ARM)
      params->session_data.term1.sample_rate = atoi(value);
#endif
      if (atoi(value) > 3) params->session_data.term1.voice.evs.codec_flags |= DSGetCodecInfo(DS_CODEC_VOICE_EVS, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE, atoi(value), 0, NULL);  /* note -- codec_type should be replaced by params->term1.codec_type, which shd be parsed in a first pass */
      else if (atoi(value) >= 0) params->session_data.term1.voice.evs.codec_flags |= atoi(value);
   }
   else if (strstr(name, "term1.evs_header_full") || strstr(name, "term1.header_format") || strstr(name, "term1.payload_format"))
      params->session_data.term1.voice.evs.codec_flags |= ((atoi(value)) ? DS_EVS_PACKET_FORMAT : 0);

   else if (strstr(name, "term2.local_ip"))
   {
#ifndef ALLOW_IPV6
      params->session_data.term2.local_ip.uIpv4 = inet_addr(value);
#else
      if (inet_pton_ex(name, value, (void*)params, DS_TERM2 | DS_LOCAL_IP_ADDR | DS_SESSION_PARAMS_T) < 0) return -1;
#endif
   }
   else if (strstr(name, "term2.remote_ip"))
   {
#ifndef ALLOW_IPV6
      params->session_data.term2.remote_ip.uIpv4 = inet_addr(value);
#else
      if (inet_pton_ex(name, value, (void*)params, DS_TERM2 | DS_REMOTE_IP_ADDR | DS_SESSION_PARAMS_T) < 0) return -1;
#endif
   }
   else if (strstr(name, "term2.local_port"))
      params->session_data.term2.local_port = atoi(value);
   else if (strstr(name, "term2.remote_port"))
      params->session_data.term2.remote_port = atoi(value);
   else if (strstr(name, "term2.media_type"))
      params->session_data.term2.media_type = media_type(value);
   else if (strstr(name, "term2.codec_type"))
      #if 0
      params->session_data.term2.codec_type = codec_type(value);
      #else
      params->session_data.term2.codec_type = DSGetCodecInfo(0, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_TYPE_FROM_NAME, 0, 0, value);
      #endif
   else if (strstr(name, "term2.bitrate"))
      params->session_data.term2.bitrate = atoi(value);
   else if (strstr(name, "term2.ptime")) {

      params->session_data.term2.voice.ptime = atoi(value);
      params->session_data.term2.ptime = atoi(value);
   }
#if defined(_X86) || defined(_ARM)
   else if (strstr(name, "term2.buffer_interval") || strstr(name, "term2.input_buffer_interval"))
      params->session_data.term2.input_buffer_interval = atoi(value);
   else if (strstr(name, "term2.output_buffer_interval"))
      params->session_data.term2.output_buffer_interval = atoi(value);
#endif
   else if (strstr(name, "term2.rtp_payload_type"))
      params->session_data.term2.voice.rtp_payload_type = atoi(value);
   else if (strstr(name, "term2.dtmf_type"))
      params->session_data.term2.voice.dtmf_mode = dtmf_type(value);
   else if (strstr(name, "term2.dtmf_payload_type"))
      params->session_data.term2.voice.dtmf_payload_type = atoi(value);
   else if (strstr(name, "term2.ec"))
      params->session_data.term2.voice.ec = ec_type(value);
   else if (strstr(name, "term2.octet_align"))
      params->session_data.term2.voice.amr.codec_flags = (atoi(value) ? DS_AMR_OCTET_ALIGN : 0);
   else if (strstr(name, "term2.evrc_format"))
      params->session_data.term2.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_PACKET_FORMAT_SHIFT;
   else if (strstr(name, "term2.evrc_bitrate"))
      params->session_data.term2.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_BITRATE_SHIFT;
   else if (strstr(name, "term2.evrc_mode"))
      params->session_data.term2.voice.evrc.codec_flags |= atoi(value) << DS_EVRC_MODE_SHIFT;
   else if (strstr(name, "term2.opus_max_bitrate"))
      params->session_data.term2.voice.opus.codec_flags |= (atoi(value) & DS_OPUS_MAX_AVG_BITRATE);
   else if (strstr(name, "term2.opus_max_playback_rate"))
      params->session_data.term2.voice.opus.max_playback_rate = atoi(value);
   else if (strstr(name, "term2.opus_sprop_max_playback_rate"))
      params->session_data.term2.voice.opus.sprop_max_capture_rate = atoi(value);
   else if (strstr(name, "term2.opus_fec"))
      params->session_data.term2.voice.opus.codec_flags |= (atoi(value) ? (~0 & DS_OPUS_FEC) : 0);
   else if (strstr(name, "term2.vad"))
      params->session_data.term2.voice.flag = (atoi(value) ? VOICE_ATTR_FLAG_VAD : 0);
#if defined(_X86) || defined(_ARM)
   else if (strstr(name, "term2.buffer_depth")) {
      strcpy(tmpstr, value);
      strupr(tmpstr);
      if (strcmp(tmpstr, "DEFAULT") == 0 || strcmp(tmpstr, "ENABLE") == 0) params->session_data.term2.buffer_depth = 0;
      else if (strcmp(tmpstr, "DISABLE") == 0) params->session_data.term2.buffer_depth = -1;
      else params->session_data.term2.buffer_depth = atoi(value);
   }
   else if (strstr(name, "term2.dtx_handling")) {
      strcpy(tmpstr, value);
      strupr(tmpstr);
      if (strcmp(tmpstr, "DEFAULT") == 0 || strcmp(tmpstr, "ENABLE") == 0 || atoi(value) == 1) params->session_data.term2.uFlags |= TERM_DTX_ENABLE;
      else if (strcmp(tmpstr, "DISABLE") == 0 || atoi(value) <= 0) params->session_data.term2.uFlags &= ~TERM_DTX_ENABLE;
   }
   else if (strstr(name, "term2.delay")) {
      params->session_data.term2.delay = atoi(value);
   }
#endif
   else if (strstr(name, "term2.evs_sample_rate") || strstr(name, "term2.sample_rate"))
   {
#if defined(_X86) || defined(_ARM)
      params->session_data.term2.sample_rate = atoi(value);
#endif
      if (atoi(value) > 3) params->session_data.term2.voice.evs.codec_flags |= DSGetCodecInfo(DS_CODEC_VOICE_EVS, DS_CODEC_INFO_TYPE | DS_CODEC_INFO_VOICE_ATTR_SAMPLERATE, atoi(value), 0, NULL);  /* note -- codec_type should be replaced by params->term1.codec_type, which shd be parsed in a first pass */
      else if (atoi(value) >= 0) params->session_data.term2.voice.evs.codec_flags |= atoi(value);
   }
   else if (strstr(name, "term2.evs_header_full") || strstr(name, "term2.header_format") || strstr(name, "term2.payload_format"))
      params->session_data.term2.voice.evs.codec_flags |= ((atoi(value)) ? DS_EVS_PACKET_FORMAT : 0);

   else if (strstr(name, "node_id"))
      params->node_id = atoi(value);

   return 0;
}

static int global_session_id = 1;
stack_t *session_stack = NULL;

/* c66x config file parsing */
static int parse_session_params(FILE *fp, session_params_t *params)
{
   char *name, *cmt_str, found_start = 0;
   char *value, string[1024];
   int len;

   while (1) 
   {
      memset(string, 0, sizeof(string));
      if (fgets(string, sizeof(string), fp) == NULL) return -1;

      /* remove comments */
      cmt_str = strchr(string, '#');
      if(cmt_str != NULL) memset(cmt_str, 0, strlen(cmt_str));

      /* skip empty lines */
      if ((strlen(string) == 0) || (strcmp(string,"\n") == 0)) continue;

      /* check for start of session data block */
      if (strstr(string,"start_of_session_data") || strstr(string,"session_data_start")) found_start = 1;

      /* skip all lines until the start of a session data block is found */
      if (!found_start) continue;

      /* check for end of session data block */
      if (strstr(string,"end_of_session_data") || strstr(string,"session_data_end")) break;

      len = strlen(string);

      /* ensure that the string terminates correctly */
      if (len > 1 && (string[len-1] == 0x0A || string[len-1] == 0x0D)) string[len-1] = '\0'; /* check for CR or LF as last char in string */
      if (len > 2 && string[len-2] == 0x0D) string[len-2] = '\0'; /* check for CR as 2nd to last char in string */

      str_remove_whitespace(string);  /* added JHB Jun2016 */

      name = strtok_r((char*)&string, "=", &value);

      parse_session_data(name, value, params);
   }

   return 0;
}

// populate session data with hardcoded values .. make configurable later
static int prepare_session_creation(char *buffer, uint32_t session_id, session_params_t params)
{
   struct cmd_hdr *cmd_hdr;
   struct cmd_create_session *create_session_data;
   
   cmd_hdr = (struct cmd_hdr*) buffer;
   cmd_hdr->type = DS_CMD_CREATE_SESSION;
   cmd_hdr->len = sizeof(struct cmd_create_session);

   create_session_data = (struct cmd_create_session *) (buffer + sizeof(struct cmd_hdr));
   create_session_data->session_data.session_id = session_id;
   create_session_data->session_data.ha_index = 0; /*SC might need to change if we are testing standby case, Jan 2016*/

   create_session_data->session_data.term1.term_id = 1;
   create_session_data->session_data.term1.media_type = params.session_data.term1.media_type;
   create_session_data->session_data.term1.codec_type = params.session_data.term1.codec_type;
   create_session_data->session_data.term1.bitrate = params.session_data.term1.bitrate;
   create_session_data->session_data.term1.remote_ip.type = IPV4;
   create_session_data->session_data.term1.remote_ip.uIpv4 = params.session_data.term1.remote_ip.uIpv4;
   create_session_data->session_data.term1.local_ip.type = IPV4;
   create_session_data->session_data.term1.local_ip.uIpv4 = params.session_data.term1.local_ip.uIpv4;
   create_session_data->session_data.term1.remote_port = htons(params.session_data.term1.remote_port);
   create_session_data->session_data.term1.local_port = htons(params.session_data.term1.local_port);
   memcpy(&create_session_data->session_data.term1.voice, &params.session_data.term1.voice, sizeof(struct voice_attributes));

   create_session_data->session_data.term2.term_id = 2;
   create_session_data->session_data.term2.media_type = params.session_data.term2.media_type;
   create_session_data->session_data.term2.codec_type = params.session_data.term2.codec_type;
   create_session_data->session_data.term2.bitrate = params.session_data.term2.bitrate;
   create_session_data->session_data.term2.remote_ip.type = IPV4;
   create_session_data->session_data.term2.remote_ip.uIpv4 = params.session_data.term2.remote_ip.uIpv4;
   create_session_data->session_data.term2.local_ip.type = IPV4;
   create_session_data->session_data.term2.local_ip.uIpv4 = params.session_data.term2.local_ip.uIpv4;
   create_session_data->session_data.term2.remote_port = htons(params.session_data.term2.remote_port);
   create_session_data->session_data.term2.local_port = htons(params.session_data.term2.local_port);
   memcpy(&create_session_data->session_data.term2.voice, &params.session_data.term2.voice, sizeof(struct voice_attributes));

   return (sizeof(struct cmd_hdr) + sizeof(struct cmd_create_session));
}

static int prepare_session_deletion(char *buffer, uint32_t session_id)
{
   struct cmd_hdr *cmd_hdr;
   struct cmd_del_session *delete_session_data;
   cmd_hdr = (struct cmd_hdr*) buffer;
   cmd_hdr->type = DS_CMD_DEL_SESSION;
   cmd_hdr->len = sizeof(struct cmd_del_session);

   delete_session_data = (struct cmd_del_session *) (buffer + sizeof(struct cmd_hdr));
   delete_session_data->session_id = session_id;

   return (sizeof(struct cmd_hdr) + sizeof(struct cmd_del_session));
}

/* Send mailbox messages for initializing the transcoder */
int transcode_init(void)
{
   uint32_t trans_id = 0xabab;
   char *gbl_cfg_buffer = (char *)calloc(sizeof(struct cmd_hdr) + sizeof(struct cmd_configuration), sizeof(char));
   char *ha_buffer = (char *)calloc(sizeof(struct cmd_hdr) + sizeof(struct cmd_ha_state), sizeof(char));
   char *ip_cfg_buffer = (char *)calloc(sizeof(struct cmd_hdr) + sizeof(struct cmd_configure_ip), sizeof(char));
   struct cmd_hdr hdr;
   struct cmd_configuration gbl_cfg;
   struct cmd_ha_state ha;
   struct cmd_configure_ip ip_cfg;
   int i;
   QWORD nCoreList_temp = nCoreList;
   
   hdr.type = DS_CMD_CONFIGURATION;
   hdr.len = sizeof(struct cmd_configuration);
   gbl_cfg.trans_id = 0;
   gbl_cfg.gf.uMaxCoreChan = 1024;
   gbl_cfg.gf.uWatchdogTimerMode = 3;
   gbl_cfg.gf.cpu_usage_low_watermark = 50;
   gbl_cfg.gf.cpu_usage_high_watermark = 75;
   gbl_cfg.gf.uPreserve_SSRC = 0;
   gbl_cfg.gf.port_start = 10240;
   gbl_cfg.gf.num_ports = 2048;
   memcpy(gbl_cfg_buffer, &hdr, sizeof(struct cmd_hdr));
   memcpy(gbl_cfg_buffer + sizeof(struct cmd_hdr), &gbl_cfg, sizeof(struct cmd_configuration));
   
   //if(!cocpu_network_test)
   {
      hdr.type = DS_CMD_SET_HA_STATE;
      hdr.len = sizeof(struct cmd_ha_state);
      ha.state = DS_STATE_ACTIVE;
      memcpy(ha_buffer, &hdr, sizeof(struct cmd_hdr));
      memcpy(ha_buffer + sizeof(struct cmd_hdr), &ha, sizeof(struct cmd_ha_state));
   }
   
   if(cocpu_network_test)
   {
      hdr.type = DS_CMD_CONFIGURE_IP;
      hdr.len = sizeof(struct cmd_configure_ip);
      ip_cfg.flag = 0xf;
      ip_cfg.physical_ip.type = IPV4;
      ip_cfg.physical_ip.uIpv4 = 0x0a0001d2;//10.0.1.210
      ip_cfg.virtual_ip.type = IPV4;
      ip_cfg.virtual_ip.uIpv4 = 0x0a0001d3;//10.0.1.211
      ip_cfg.subnet_mask.type = IPV4;
      ip_cfg.subnet_mask.uIpv4 = 0xffffff00;
      ip_cfg.gateway.type = IPV4;
      ip_cfg.gateway.uIpv4 = 0x0a000101;
      //ip_cfg.port_start = 10240;
      //ip_cfg.num_ports = 2048;
      memcpy(ip_cfg_buffer, &hdr, sizeof(struct cmd_hdr));
      memcpy(ip_cfg_buffer + sizeof(struct cmd_hdr), &ip_cfg, sizeof(struct cmd_configure_ip)); 
   }
   
   for (i = 0; nCoreList_temp > 0; i++, nCoreList_temp >>= 8)
   {
      if (nCoreList & (0xff << (i*8)))
      {
         if (write_mb(i*8, (uint8_t *) gbl_cfg_buffer, sizeof(struct cmd_hdr) + sizeof(struct cmd_configuration), trans_id++) == -1) {
            printf("ERROR: global config: write_mb error\n");
            return -1;
         }
         printf("global config command sent to chip %d\n", i);
         
         //if(!cocpu_network_test)
         {
            if (write_mb(i*8, (uint8_t *) ha_buffer, sizeof(struct cmd_hdr) + sizeof(struct cmd_ha_state), trans_id++) == -1) {
               printf("ERROR: ha state: write_mb error\n");
               return -1;
            }
            printf("ha state command sent to chip %d\n", i);
         }
         
         if(cocpu_network_test)
         {
            if (write_mb(i*8, (uint8_t *) ip_cfg_buffer, sizeof(struct cmd_hdr) + sizeof(struct cmd_configure_ip), trans_id++) == -1) {
               printf("ERROR: configure ip: write_mb error\n");
               return -1;
            }
            printf("configure ip command sent to chip %d\n", i);
            
            ip_cfg.physical_ip.uIpv4 += 2;
            ip_cfg.virtual_ip.uIpv4 += 2;
            memcpy(ip_cfg_buffer + sizeof(struct cmd_hdr), &ip_cfg, sizeof(struct cmd_configure_ip)); 
         }
      }
   }
   
   free(gbl_cfg_buffer);
   free(ha_buffer);
   free(ip_cfg_buffer);
   
   return 0;
}

/* setup sessions
 *    returns -1 on error else returns number of sessions
 */ 
int create_sessions(PMEDIAPARAMS mediaParams)
{
   uint32_t trans_id = 0xabab;
   uint32_t size;
#if !defined(_X86) && !defined(_ARM)  /* x86 has additional session params that increase the SESSION_DATA struct size to beyond the mailbox depth, when building for c66x, _X86 should not be defined */
   char *tx_buffer = (char *)calloc(TRANS_MAILBOX_MAX_PAYLOAD_SIZE, sizeof(char));
#else
   char *tx_buffer = (char *)calloc(sizeof(struct cmd_hdr) + sizeof(struct cmd_create_session), sizeof(char));
#endif
   session_params_t params;
   memset(&params, 0, sizeof(params));
   char default_config_file[] = "session_config/test_config";
   char *config_file;

   FILE *cfg_fp = NULL;
   
   if (strlen(mediaParams->configFilename) == 0 || (access(mediaParams->configFilename, F_OK ) == -1)) 
   {
      printf("Specified config file: %s does not exist, using default file.\n", mediaParams->configFilename);
      config_file = default_config_file;
   }
   else config_file = mediaParams->configFilename;
   
   printf("Opening session config file: %s\n", config_file);
   
   cfg_fp = fopen(config_file, "r");

   // read test_config and send session data to corresponding dsp cores.
   while (parse_session_params(cfg_fp, &params) != -1) 
   {
      size = prepare_session_creation(tx_buffer, global_session_id, params);

      if (write_mb(params.node_id, (uint8_t *)tx_buffer, size, trans_id++) != 0) 
      {
         printf("ERROR: failed to send session create command\n");
         return -1;
      }
      
      // add session to stack
      stack_t *session_entry = (stack_t *)calloc(1, sizeof(stack_t));
      session_entry->id = global_session_id;
      session_entry->param = params;
      session_entry->next = session_stack;
      session_stack = session_entry;
      
      printf("session creation command sent to node %d with session_id %d\n", params.node_id, global_session_id);
      global_session_id++;
      memset(&params, 0, sizeof(params));
      usleep(10000);
   }
   
   usleep(100000);
   fclose(cfg_fp);
   free(tx_buffer);
   return global_session_id;
}

/* tear down sessions */
int delete_sessions(void)
{
   uint32_t trans_id = 0xabab;
   uint32_t size, ret_val;
   char *tx_buffer = (char *)calloc(TRANS_MAILBOX_MAX_PAYLOAD_SIZE, sizeof(char));

   // close all sessions & output files
   while (session_stack != NULL) 
   {
      // get session from stack
      stack_t *temp = session_stack;
      session_stack = session_stack->next;

      size = prepare_session_deletion(tx_buffer, temp->id);
      
      if ((ret_val = write_mb(temp->param.node_id, (uint8_t *) tx_buffer, size, trans_id++)) != 0) 
      {
         printf("ERROR: failed to send session delete command, error code = %d\n", ret_val);
         return -1;
      }

      printf("deleted session %d on node %d\n", temp->id, temp->param.node_id);
      free(temp);
      usleep(10000);
   }
   
   usleep(100000);
   free(tx_buffer);
   return 0;
}

#endif  /* _MEDIATEST_ */
