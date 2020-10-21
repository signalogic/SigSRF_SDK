/*
  $Header: /root/Signalogic/DirectCore/include/inferlib.h
 
  Description: Header file for inferlib - library handling neural network inferencing-type functions e.g. automatic speech recognition (ASR) decoding
 
  Projects: SigSRF, DirectCore
 
  Copyright (C) Signalogic Inc. 2019-2020

  Revision History:
   
   Created Jan 2019 Chris Johnson
*/

#define DS_GET_TEXT_FULL      1
#define DS_GET_TEXT_NEW_WORDS 2

typedef void* HASRDECODER;

typedef struct
{
   char*    feature_type;
   char*    mfcc_config;
   char*    ivector_config;
   int      frame_subsampling_factor;
   float    acoustic_scale;
   float    beam;
   int      max_active;
   float    lattice_beam;
   char*    silence_phones;
   char*    nnet3_rxfilename;
   char*    fst_rxfilename;
   char*    word_syms_filename;
   char*    decode_name;
   bool     do_endpointing;
   bool     online;
   int      samp_freq;
} ASR_Config;

HASRDECODER DSASRInit(ASR_Config *config);
int DSASRProcess(HASRDECODER handle, float *data, int length);
int DSASRGetText(HASRDECODER handle, unsigned int uFlags);
int DSASRFinalize(HASRDECODER handle);
int DSASRClose(HASRDECODER handle);

