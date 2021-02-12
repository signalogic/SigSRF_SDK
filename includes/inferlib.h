/*
  $Header: /root/Signalogic/DirectCore/include/inferlib.h
 
  Description: Header file for inferlib - library handling neural network inferencing-type functions e.g. automatic speech recognition (ASR) decoding
 
  Projects: SigSRF, DirectCore
 
  Copyright (C) Signalogic Inc. 2018-2021

  Revision History
   
   Created Jan 2019 Chris Johnson
   Modified Jan 2021 JHB, add extern C declarations, API comments
   Modified Jan 2021 JHB, add DSASRConfig() to provide initialization ease-of-use and flexibility, add DS_ASR_CONFIG_xx flags
*/

#ifndef _INFERLIB_H_
#define _INFERLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ASR instance handle definition */

typedef void* HASRDECODER;

/* ASR instance configuration */

typedef struct {

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
  char*    utterance_id;              /* name of utterance when reporting timing-related stats */
  bool     do_endpointing;
  bool     online;                    /* true = real-time operation (Kaldi refers to this as "online decoding") */
  int      samp_freq;

} ASR_CONFIG;

/* DSASRConfig() configures an ASR instance. Notes:

  -must be called prior to DSASRCreate()
  -ASR_CONFIG struct should be initialized to zero. Any items pre-initialized will be used as-is, otherwise DSASRConfig() generates default values. This includes szUtteranceId and sample_rate params
  -any char* items pre-initialized in ASR_CONFIG must be malloc or strdup pointers, as DSASRDelete() will attempt to free them
  -struct items do_endpointing and online are controlled by DS_ASR_CONFIG_DO_ENDPOINTING and DS_ASR_CONFIG_ONLINE flags
  -sample rate must be 16 kHz (wideband audio). Sampling rate conversion of input audio possibly may be added later, depending on accuracy impact
  -return value <= 0 indicate error condition or config not performed
*/

int DSASRConfig(ASR_CONFIG* pASRConfig, unsigned int uFlags, const char* szUtteranceId, int sample_rate);

/* uFlags for DSASRConfig() */

#define DS_ASR_CONFIG_DO_ENDPOINTING   1
#define DS_ASR_CONFIG_ONLINE           2

HASRDECODER DSASRCreate(ASR_CONFIG* pASRConfig);                      /* create an ASR instance. DSASRConfig() must be called first to initialize an ASR_CONFIG struct. All other APIs use the returned handle */
int DSASRDelete(HASRDECODER hASRDecoder);                             /* delete an ASR instance */

int DSASRProcess(HASRDECODER hASRDecoder, float* data, int length);   /* provide input to an ASR instance for processing (typically at 20 msec frame size intervals) */
int DSASRGetText(HASRDECODER hASRDecoder, unsigned int uFlags);       /* get results from an ASR instance */

/* uFlags for DSASRGetText() */

#define DS_ASR_GET_TEXT_FULL           1
#define DS_ASR_GET_TEXT_NEW_WORDS      2

int DSASRFinalize(HASRDECODER hASRDecoder);                           /* finalize results for an ASR instance (typically at 1/2 sec interrvals) */ 

#ifdef __cplusplus
}
#endif

#endif  /* _INFERLIB_H_ */
