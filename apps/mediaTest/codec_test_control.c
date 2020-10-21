/*
 $Header: /root/Signalogic/DirectCore/apps/SigC641x_C667x/mediaTest/codec_test_control.c

 Description:
 
   Functions for controlling codec test, including:
      - codec test initialization
      - codec test params setup
      - codec test config file parsing
 
 Copyright (C) Signalogic Inc. 2016-2020

 Revision History:
 
   Created Jan 12016 CJ
   Modified Feb 2017 CJ, Made config file parsing support global
   Modified Mar 2017 JHB, edits for codec testing
   Modified May 2018 JHB, enforce limit size of codec_test_params write to c66x mem (see comments)
   Modified Dec 2018 CKJ, add encoded file extension ".BIT" to support AMR-WB+
   Modified Sep 2020 JHB, mods for compatibility with gcc 9.3.0: include math.h (for min/max functions)
*/

/* mediaTest definitions (also includes DirectCore header files) */

#include <math.h>
#include "mediaTest.h"

/* init codec test by setting config params */

int init_codec_test(PMEDIAPARAMS mediaParams, codec_test_params_t* codec_test_params) {

DWORD codec_test_params_addr;
char tmpstr[256] = {0};
char default_config_file[] = "session_config/codec_test_config";
char *config_file;
FILE *cfg_fp = NULL;


   codec_test_params_addr = DSGetSymbolAddr(hCard, NULL, "unit_test_params");

   if (!codec_test_params_addr) {
   
      printf("codec_test_params struct address not found\n");
      return -1;
   }
   
   memset(codec_test_params, 0, sizeof(codec_test_params_t));

   codec_test_params->dtx_value = 8;  /* to-do - also set dtx_enable depending on config file.  Both dtx_value and dtx_enable are handled in c66x main.c unit test */
   
   if (strlen(mediaParams->configFilename) == 0 || (access(mediaParams->configFilename, F_OK ) == -1)) 
   {
      printf("Specified config file does not exist, using default file\n");
      config_file = default_config_file;
   }
   else config_file = mediaParams->configFilename;
   
   printf("Opening codec test config file: %s\n", config_file);
   
   cfg_fp = fopen(config_file, "r");
   
   if (cfg_fp == NULL)
   {
      printf("Unable to open config file\n");
      return -1;
   }
   
   parse_codec_test_params(cfg_fp, codec_test_params);
   
   printf("input file: %s, output file: %s\n", mediaParams->Media.inputFilename, mediaParams->Media.outputFilename);

   strcpy(tmpstr, mediaParams->Media.inputFilename);
   strupr(tmpstr);
   
   if (strstr(tmpstr, ".COD") == NULL && strstr(tmpstr, ".BIT") == NULL) codec_test_params->encoder_enable = 1;

/* to-do:  set mediaParams->inputFilesize to size of input file */
   
   strcpy(tmpstr, mediaParams->Media.outputFilename);
   strupr(tmpstr);

   if (strstr(tmpstr, ".COD") == NULL && strstr(tmpstr, ".BIT") == NULL) codec_test_params->decoder_enable = 1;

   if (!codec_test_params->encoder_enable && !codec_test_params->decoder_enable)
   {
      printf("Both input and output filenames signify encoded format. No coding would take place\n");
      return -1;
   }

   printf("Codec test params: bitrate = %d, sample_rate = %d, encoder_enable = %d, decoder_enable = %d", codec_test_params->bitrate, codec_test_params->sample_rate, codec_test_params->encoder_enable, codec_test_params->decoder_enable);

   if (CPU_mode & CPUMODE_C66X) {   

   /* write codec test params to c66x mem */

      printf(" coCPU mem addr = 0x%x", codec_test_params_addr);

   /* c66x main.c used for codec unit test does not reference codec_params_t struct, which should be moved from mediaTest.h to a shared include file. c66x code is currently
      only declaring space for 6 ints, so this is a bad situation and needs to be corrected.  For now we enforce a limit on size of codec params written to c66x mem, JHB May 2018 */

      if (codec_test_params_addr) DSWriteMem(hCard, DS_GM_LINEAR_DATA | DS_RM_MASTERMODE, codec_test_params_addr, DS_GM_SIZE32, codec_test_params, min(6, sizeof(codec_test_params_t)/4));
   }
 
   printf("\n");

/* set some MEDIAPARAMS struct values before return */

   mediaParams->samplingRate = codec_test_params->sample_rate;
   mediaParams->Streaming.bitRate = codec_test_params->bitrate;
   
   return 0;
}
