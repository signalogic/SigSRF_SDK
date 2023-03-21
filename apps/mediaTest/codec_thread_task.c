/*
 $Header: /root/Signalogic/DirectCore/apps/SigC641x_C667x/mediaTest/codec_thread_task.c

 Description:
 
   Functions for encoding or decoding pre-configured files as a separate thread
 
 Copyright (C) Signalogic Inc. 2017-2023
 
 Revision History
 
  Created Mar 2017 CKJ
  Modified Feb 2022 JHB, modify calling format to DSCodecDecode() and DSCodecEncode(), see comments in voplib.h
  Modified Mar 2022 JHB, add some missing initializer brackets that cause warnings with gcc compiler
  Modified Oct 2022 JHB, change DSGetCodecRawFramesize() to DSGetCodecInfo()
  Modified Jan 2023 JHB, moved _fread() wrapper to mediaTest.h - make available to all apps
  Modified Mar 2023 JHB, add pInArgs param to DSCodecEncode()
*/

#include <stdio.h>

#include "mediaTest.h"
#include "voplib.h"

extern char* encoder_input_files[MAX_CODEC_INSTANCES];
extern char* decoder_input_files[MAX_CODEC_INSTANCES];

extern int encoded_frame_cnt[MAX_CODEC_INSTANCES];
extern int decoded_frame_cnt[MAX_CODEC_INSTANCES];

extern char thread_status[2*MAX_CODEC_INSTANCES];

void *encode_thread_task(void *arg)
{
   HCODEC hCodec = *(HCODEC *)arg;
   FILE *in_fp, *out_fp;
   char out_filename[50];
   uint8_t in_buf[MAX_RAW_FRAME];
   uint8_t out_buf[MAX_CODED_FRAME];
   int ret_val;
   uint32_t in_frame_size, out_frame_size;
   MEDIAINFO MediaInfo = {{0}};  /* add 2nd set of brackets, JHB Mar2022 */

   in_fp = fopen(encoder_input_files[hCodec], "rb");
   if (!in_fp) 
   {
      fprintf(stderr, "%s:%d: Failed to open input file %s: %s\n", __FILE__, __LINE__, encoder_input_files[hCodec], strerror(errno));
      return 0;
   }
   
   switch (DSGetCodecType(hCodec)) {
      case DS_VOICE_CODEC_TYPE_G711_ULAW:
         sprintf(out_filename, "test_files/codec_%d_encoded.ul", hCodec);
         break;
      case DS_VOICE_CODEC_TYPE_G711_ALAW:
         sprintf(out_filename, "test_files/codec_%d_encoded.al", hCodec);
         break;
      case DS_VOICE_CODEC_TYPE_EVS:
         sprintf(out_filename, "test_files/codec_%d_encoded.COD", hCodec);
         break;
      default:
         sprintf(out_filename, "test_files/codec_%d_encoded.coded", hCodec);
   }
   ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &out_fp, out_filename, 0, 0, DS_CREATE, &MediaInfo);
   if (!out_fp) 
   {
      fprintf(stderr, "%s:%d: Failed to open output file %s, ret_val = %d\n", __FILE__, __LINE__, out_filename, ret_val);
      return 0;
   }
   
   in_frame_size = DSGetCodecInfo(hCodec, DS_CODEC_INFO_HANDLE | DS_CODEC_INFO_RAW_FRAMESIZE, 0, 0, NULL);

   while (pm_run && ((ret_val = _fread(in_buf, sizeof(char), in_frame_size, in_fp)) == (int)in_frame_size)) {

      out_frame_size = DSCodecEncode(&hCodec, 0, in_buf, out_buf, in_frame_size, 1, NULL, NULL);

      ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &out_fp, NULL, (uintptr_t)out_buf, out_frame_size, DS_WRITE, &MediaInfo);

      encoded_frame_cnt[hCodec]++;
   }
   
   /* Check if loop exit condition was error */
   if (pm_run && !feof(in_fp))
   {
      printf("Error did not reach EOF, last fread() read %d bytes\n", ret_val);
   }
   
   fclose(in_fp);
   DSSaveDataFile(DS_GM_HOST_MEM, &out_fp, NULL, 0, 0, DS_CLOSE, &MediaInfo);
   
   thread_status[2*hCodec] = 2;

   return NULL;  /* added to avoid -Wall compiler warning, JHB Jul 2019 */
}

void *decode_thread_task(void *arg)
{
   HCODEC hCodec = *(HCODEC *)arg;
   FILE *in_fp, *out_fp;
   char out_filename[200];
   uint8_t in_buf[MAX_CODED_FRAME];
   uint8_t out_buf[MAX_RAW_FRAME];
   int ret_val;
   uint32_t in_frame_size = 0, out_frame_size;
   MEDIAINFO MediaInfo = {{0}};  /* add 2nd set of brackets, JHB Mar2022 */

   in_fp = fopen(decoder_input_files[hCodec], "rb");
   if (!in_fp) 
   {
      fprintf(stderr, "%s:%d: Failed to open input file %s: %s\n", __FILE__, __LINE__, decoder_input_files[hCodec], strerror(errno));
      return 0;
   }
   
   sprintf(out_filename, "test_files/codec_%d_decoded.OUT", hCodec);
   ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &out_fp, out_filename, 0, 0, DS_CREATE, &MediaInfo);
   if (!out_fp) 
   {
      fprintf(stderr, "%s:%d: Failed to open output file %s, ret_val = %d\n", __FILE__, __LINE__, out_filename, ret_val);
      return 0;
   }
   
   in_frame_size = DSGetCodecInfo(hCodec, DS_CODEC_INFO_HANDLE | DS_CODEC_INFO_CODED_FRAMESIZE, 0, 0, NULL);  /* don't need to specify bitrate if we use DS_CODEC_INFO_HANDLE, JHB Oct 2022 */
#if 0
   out_frame_size = DSGetCodecInfo(hCodec, DS_CODEC_INFO_HANDLE | DS_CODEC_INFO_RAW_FRAMESIZE, 0, NULL);
#endif

{
/* Skip EVS header in input file - only for file i/o operations with decoder */
   fseek(in_fp, 16, SEEK_SET);
}

   while (pm_run && ((ret_val = _fread(in_buf, sizeof(char), in_frame_size, in_fp)) == (int)in_frame_size)) {

      out_frame_size = DSCodecDecode(&hCodec, 0, in_buf, out_buf, in_frame_size, 1, NULL);

      ret_val = DSSaveDataFile(DS_GM_HOST_MEM, &out_fp, NULL, (uintptr_t)out_buf, out_frame_size, DS_WRITE, &MediaInfo);

      decoded_frame_cnt[hCodec]++;
   }
   
   /* Check if loop exit condition was error or EOF */
   if (pm_run && !feof(in_fp))
   {
      printf("Error did not reach EOF, last fread() read %d bytes\n", ret_val);
   }
   
   fclose(in_fp);
   DSSaveDataFile(DS_GM_HOST_MEM, &out_fp, NULL, 0, 0, DS_CLOSE, &MediaInfo);
   
   thread_status[2*hCodec+1] = 2;

   return NULL;  /* added to avoid -Wall compiler warning, JHB Jul 2019 */
}
