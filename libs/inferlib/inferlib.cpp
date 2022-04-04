/*
 $Header: /root/Signalogic/DirectCore/lib/inferlib/inferlib.cpp

 Copyright (C) Signalogic Inc. 2018-2021

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

 Description

  Inference library including automatic speech recognition (ASR) decoder

 Projects

  SigSRF, DirectCore

 Revision History

  Created Jan 2019 Chris Johnson, initial version based on based on Kaldi executable online2-wav-nnet3-latgen-faster
  Modified Jan 2021 JHB, add diaglib for error handling (Log_RT), initialize asr_handles, zero handle member values prior to use, make local data and functions static
  Modified Jan 2021 JHB, add DSASRConfig() to provide initialization ease-of-use and flexibility
  Modified Feb 2021 JHB, make DSASRConfig() flexible on where it finds Kaldi .conf, .mdl, .fst, and other files
  Modified Apr 2022 JHB, for containers and rar package installs, handle Kaldi hard-coded paths inside ivector_extractor.conf; see comments in DSASRConfig() and find_kaldi_file()
*/

/* Kaldi includes */
#include "online2/online-nnet3-decoding.h"
#include "online2/online-timing.h"
#include "lat/lattice-functions.h"
#include "nnet3/nnet-utils.h"

/* SigSRF includes */
#include "inferlib.h"
#include "shared_include/streamlib.h"
#include "diaglib.h"  /* provides Log_RT() */

using namespace std;
using namespace kaldi;
using namespace fst;

typedef kaldi::int32 int32;
typedef kaldi::int64 int64;

namespace kaldi {

   void GetDiagnosticsAndPrintOutput(const fst::SymbolTable* word_syms, const CompactLattice &clat, int64* tot_num_frames, double* tot_like) {

      if (clat.NumStates() == 0) {
         KALDI_WARN << "Empty lattice.";
         return;
      }

      CompactLattice best_path_clat;
      CompactLatticeShortestPath(clat, &best_path_clat);

      Lattice best_path_lat;
      ConvertLattice(best_path_clat, &best_path_lat);

      double likelihood;
      LatticeWeight weight;
      int32 num_frames;
      std::vector<int32> alignment;
      std::vector<int32> words;
      GetLinearSymbolSequence(best_path_lat, &alignment, &words, &weight);

      num_frames = alignment.size();
      likelihood = -(weight.Value1() + weight.Value2());
      *tot_num_frames += num_frames;
      *tot_like += likelihood;
      KALDI_VLOG(2) << "Likelihood per frame is " << (likelihood / num_frames) << " over " << num_frames << " frames.";

      if (word_syms != NULL) {

         for (size_t i = 0; i < words.size(); i++) {
            std::string s = word_syms->Find(words[i]);
            if (s == "") KALDI_ERR << "Word-id " << words[i] << " not in symbol table.";
            std::cerr << s << ' ';
         }

         std::cerr << std::endl;
      }
   }
}

// inferlib ASR instance info

typedef struct ASR_Info {
   
   ASR_Info() : in_use(false) {}
   
   bool in_use;

   LatticeFasterDecoderConfig decoder_opts;
   nnet3::NnetSimpleLoopedComputationOptions decodable_opts;
   OnlineEndpointConfig endpoint_opts;

   bool do_endpointing;
   bool online;

   TransitionModel trans_model;
   nnet3::AmNnetSimple am_nnet;

   OnlineNnet2FeaturePipelineInfo* feature_info;
   fst::Fst<fst::StdArc>* decode_fst;

   nnet3::DecodableNnetSimpleLoopedInfo* decodable_info;

   fst::SymbolTable* word_syms;
   OnlineIvectorExtractorAdaptationState* adaptation_state;
   OnlineNnet2FeaturePipeline* feature_pipeline;
   OnlineSilenceWeighting* silence_weighting;
   SingleUtteranceNnet3Decoder* decoder;

   OnlineTimer* decoding_timer;

   int32 samp_offset;
   std::vector<std::pair<int32, BaseFloat>> delta_weights;
   int32 samp_freq;

   int text_pos;
   
   ASR_CONFIG asr_config;  /* add ASR_CONFIG (inferlib.h) to maintain a persistent copy of config info, needed for free() operations in DSASRDelete, JHB, Jan2021 */

} ASR_INFO;

#define MAX_ASR_HANDLES MAX_STREAM_GROUPS  /* max of one ASR handle per stream group, JHB Jan2021 */

static ASR_INFO asr_handles[MAX_ASR_HANDLES] = {};  /* ensure all .in_use elements are initialized to false */

/* CJ - protection needs to be added when acquiring a new handle index to make the library thread-safe */
/* JHB - to address this, both DSASRCreate() and DSASRDelete() ae currently called fom pktlib within pktlib_sem lock */

static ASR_INFO* get_asr_handle() {

   for (int i=0; i<MAX_ASR_HANDLES; i++) if (!asr_handles[i].in_use) {

      ASR_INFO zeroed_handle = {};  /* initialize all member values to zero before use */
      #if 0
      asr_handles[i] = zeroed_handle;  /* not sure why this doesn't work */
      #else
      memcpy(&asr_handles[i], &zeroed_handle, sizeof(ASR_INFO));
      #endif
      return &asr_handles[i];
   }

   return NULL;
}

/* Notes:

  -inferlib wrapper is DSASRCreate()
  -returns NULL if no ASR instance can be created
*/

static HASRDECODER SigOnline2WavNnet3LatgenFasterInit(ASR_CONFIG* config) {

ASR_INFO* handle_ptr = NULL;

   try {

      if (config->samp_freq != 16000) {

         #if 0
         cerr << "ERROR: sampling frequency " << config->samp_freq << " not supported, only 16000 is currently supported" << endl;
         #else
         Log_RT(2, "ERROR: DSASRInit() says sampling frequency %d Hz not supported, only 16000 Hz (wideband) is currently supported \n", config->samp_freq);
         #endif

cleanup:  /* all error conditions should go here, so we can check if handle has already been allocated and needs to be cleaned up / returned to the "available" pile, JHB Jan2021 */

         if (handle_ptr) DSASRDelete(handle_ptr);
         return NULL;
      }

      handle_ptr = get_asr_handle(); // get next available ASR instance handle

      if (handle_ptr == NULL) {
         cerr << "ERROR no more ASR decoder handles available" << endl;
         goto cleanup;
      }

      handle_ptr->in_use = true;

      handle_ptr->do_endpointing = config->do_endpointing;
      handle_ptr->online = config->online;

      handle_ptr->samp_offset = 0;
      handle_ptr->samp_freq = config->samp_freq;

      if (handle_ptr->samp_freq != 16000) {

         #if 0
         cerr << "ERROR: sampling frequency " << handle_ptr->samp_freq << " not supported, only 16000 is currently supported" << endl;
         #else
         Log_RT(2, "ERROR: DSASRInit() says sampling frequency %d Hz not supported, only 16000 Hz (wideband) is currently supported \n", handle_ptr->samp_freq);
         #endif

         goto cleanup;
      }

      handle_ptr->text_pos = 0;

   // feature_opts includes configuration for the iVector adaptation, as well as the basic features
      OnlineNnet2FeaturePipelineConfig feature_opts;

      // set options that differ from default values
      feature_opts.feature_type = config->feature_type;//"mfcc";
      feature_opts.mfcc_config = config->mfcc_config;//"/storage/kaldi/egs/mini_librispeech/s5/exp/chain/tdnn1h_sp_online/conf/mfcc.conf";
      feature_opts.ivector_extraction_config = config->ivector_config;//"/storage/kaldi/egs/mini_librispeech/s5/exp/chain/tdnn1h_sp_online/conf/ivector_extractor.conf";

      handle_ptr->decodable_opts.frame_subsampling_factor = config->frame_subsampling_factor;//3;
      handle_ptr->decodable_opts.acoustic_scale = config->acoustic_scale;//1.0;

      handle_ptr->decoder_opts.beam = config->beam;//15.0;
      handle_ptr->decoder_opts.max_active = config->max_active;//7000;
      handle_ptr->decoder_opts.lattice_beam = config->lattice_beam;//6.0;

      handle_ptr->endpoint_opts.silence_phones = config->silence_phones;//"1:2:3:4:5:6:7:8:9:10";

      std::string nnet3_rxfilename = config->nnet3_rxfilename,//"/storage/kaldi/egs/mini_librispeech/s5/exp/chain/tdnn1h_sp_online/final.mdl",
         fst_rxfilename = config->fst_rxfilename;//"/storage/kaldi/egs/mini_librispeech/s5/exp/chain/tree_sp/graph_tgsmall/HCLG.fst";

      handle_ptr->feature_info = new OnlineNnet2FeaturePipelineInfo(feature_opts);

      if (!handle_ptr->online) {
         handle_ptr->feature_info->ivector_extractor_info.use_most_recent_ivector = true;
         handle_ptr->feature_info->ivector_extractor_info.greedy_ivector_extractor = true;
      }

      bool binary;
      Input ki(nnet3_rxfilename, &binary);
      handle_ptr->trans_model.Read(ki.Stream(), binary);
      handle_ptr->am_nnet.Read(ki.Stream(), binary);
      SetBatchnormTestMode(true, &(handle_ptr->am_nnet.GetNnet()));
      SetDropoutTestMode(true, &(handle_ptr->am_nnet.GetNnet()));
      nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(handle_ptr->am_nnet.GetNnet()));

      handle_ptr->decode_fst = ReadFstKaldiGeneric(fst_rxfilename);

   // decodable info object contains precomputed stuff that is used by all decodable objects. It takes a pointer to am_nnet because if it has iVectors it has to modify the nnet to accept iVectors at intervals

      handle_ptr->decodable_info = new nnet3::DecodableNnetSimpleLoopedInfo(handle_ptr->decodable_opts, &handle_ptr->am_nnet);

      std::string word_syms_rxfilename = config->word_syms_filename;//"/storage/kaldi/egs/mini_librispeech/s5/exp/chain/tree_sp/graph_tgsmall/words.txt";
      if (word_syms_rxfilename != "")
         if (!(handle_ptr->word_syms = fst::SymbolTable::ReadText(word_syms_rxfilename))) KALDI_ERR << "Could not read symbol table from file " << word_syms_rxfilename;

      handle_ptr->adaptation_state = new OnlineIvectorExtractorAdaptationState(handle_ptr->feature_info->ivector_extractor_info);

      handle_ptr->feature_pipeline = new OnlineNnet2FeaturePipeline(*handle_ptr->feature_info);
      handle_ptr->feature_pipeline->SetAdaptationState(*handle_ptr->adaptation_state);

      handle_ptr->silence_weighting = new OnlineSilenceWeighting(handle_ptr->trans_model, handle_ptr->feature_info->silence_weighting_config, handle_ptr->decodable_opts.frame_subsampling_factor);

      handle_ptr->decoder = new SingleUtteranceNnet3Decoder(handle_ptr->decoder_opts, handle_ptr->trans_model, *handle_ptr->decodable_info, *handle_ptr->decode_fst, handle_ptr->feature_pipeline);

      handle_ptr->decoding_timer = new OnlineTimer(config->utterance_id);

      memcpy(&handle_ptr->asr_config, config, sizeof(ASR_CONFIG));  /* save config info */

      return handle_ptr;

   } catch(const std::exception& e) {
      std::cerr << e.what();
      return NULL;
   }
}

static int SigOnline2WavNnet3LatgenFasterProcess(HASRDECODER handle, float* data, int length) {

   try {

      ASR_INFO* handle_ptr = (ASR_INFO*)handle;

      SubVector<BaseFloat> wave_part(data, length);

      handle_ptr->feature_pipeline->AcceptWaveform(handle_ptr->samp_freq, wave_part);

      handle_ptr->samp_offset += length;
      if (handle_ptr->decoding_timer) handle_ptr->decoding_timer->WaitUntil(handle_ptr->samp_offset / handle_ptr->samp_freq);  /* note -- commenting this out made no difference in processing rate */

      if (length == 0)  
      {
         // no more input. flush out last frames
         handle_ptr->feature_pipeline->InputFinished();
      }

      if (handle_ptr->silence_weighting->Active() && handle_ptr->feature_pipeline->IvectorFeature() != NULL) {

         handle_ptr->silence_weighting->ComputeCurrentTraceback(handle_ptr->decoder->Decoder());
         handle_ptr->silence_weighting->GetDeltaWeights(handle_ptr->feature_pipeline->NumFramesReady(), &handle_ptr->delta_weights);
         handle_ptr->feature_pipeline->IvectorFeature()->UpdateFrameWeights(handle_ptr->delta_weights);
      }

      handle_ptr->decoder->AdvanceDecoding();

      if (handle_ptr->do_endpointing && handle_ptr->decoder->EndpointDetected(handle_ptr->endpoint_opts)) {
         return -1;
      }

      return 0;

   } catch(const std::exception& e) {
      std::cerr << e.what();
      return -1;
   }
}

static int SigOnline2WavNnet3LatgenFasterGetText(HASRDECODER handle, unsigned int uFlags) {

   try {

      ASR_INFO* handle_ptr = (ASR_INFO*)handle;

      CompactLattice clat;
      bool end_of_utterance = false;
      handle_ptr->decoder->GetLattice(end_of_utterance, &clat);

      CompactLattice best_path_clat;
      CompactLatticeShortestPath(clat, &best_path_clat);

      Lattice best_path_lat;
      ConvertLattice(best_path_clat, &best_path_lat);

      LatticeWeight weight;
      std::vector<int32> alignment;
      std::vector<int32> words;
      GetLinearSymbolSequence(best_path_lat, &alignment, &words, &weight);

      size_t i = (uFlags == DS_ASR_GET_TEXT_FULL) ? 0 : handle_ptr->text_pos;

      for (; i < words.size(); i++) {
         std::string s = handle_ptr->word_syms->Find(words[i]);
         if (s == "")
            KALDI_ERR << "Word-id " << words[i] << " not in symbol table.";
         std::cerr << s << ' ';
      }
      std::cerr << std::endl;

      handle_ptr->text_pos = i;

      return 0;

   } catch(const std::exception& e) {
      std::cerr << e.what();
      return -1;
   }
}

static int SigOnline2WavNnet3LatgenFasterFinalize(HASRDECODER handle) {

   try {

      ASR_INFO* handle_ptr = (ASR_INFO*)handle;

      double tot_like = 0.0;
      int64 num_frames = 0;

      OnlineTimingStats timing_stats;

      handle_ptr->decoder->FinalizeDecoding();

      CompactLattice clat;
      bool end_of_utterance = true;
      handle_ptr->decoder->GetLattice(end_of_utterance, &clat);

      GetDiagnosticsAndPrintOutput(handle_ptr->word_syms, clat, &num_frames, &tot_like);

      if (handle_ptr->decoding_timer) handle_ptr->decoding_timer->OutputStats(&timing_stats);

      // In an application you might avoid updating the adaptation state if you felt the utterance had low confidence.  See lat/confidence.h
      handle_ptr->feature_pipeline->GetAdaptationState(handle_ptr->adaptation_state);

      // we want to output the lattice with un-scaled acoustics
      BaseFloat inv_acoustic_scale = 1.0 / handle_ptr->decodable_opts.acoustic_scale;
      ScaleLattice(AcousticLatticeScale(inv_acoustic_scale), &clat);

      timing_stats.Print(handle_ptr->online);

      KALDI_LOG << "Overall likelihood per frame was " << (tot_like / num_frames) << " per frame over " << num_frames << " frames.";

      return 0;

   } catch(const std::exception& e) {
      std::cerr << e.what();
      return -1;
   }
}

static void local_free(void* ptr) {

   if (ptr) free(ptr);
}

static void config_free(ASR_CONFIG* config) {

   local_free(config->mfcc_config);
   local_free(config->ivector_config);
   local_free(config->silence_phones);
   local_free(config->nnet3_rxfilename);
   local_free(config->fst_rxfilename);
   local_free(config->word_syms_filename);

   local_free(config->utterance_id);
}

static int SigOnline2WavNnet3LatgenFasterClose(HASRDECODER handle) {  /* note - inferlib wrapper is DSASRDelete() */

   try {
   
      if (!handle) return -1;

      ASR_INFO* handle_ptr = (ASR_INFO*)handle;

      if (handle_ptr->feature_info) delete handle_ptr->feature_info;
      if (handle_ptr->decode_fst) delete handle_ptr->decode_fst;

      if (handle_ptr->decodable_info) delete handle_ptr->decodable_info;

      if (handle_ptr->word_syms) delete handle_ptr->word_syms;
      if (handle_ptr->adaptation_state) delete handle_ptr->adaptation_state;
      if (handle_ptr->feature_pipeline) delete handle_ptr->feature_pipeline;
      if (handle_ptr->silence_weighting) delete handle_ptr->silence_weighting;
      if (handle_ptr->decoder) delete handle_ptr->decoder;

      if (handle_ptr->decoding_timer) delete handle_ptr->decoding_timer;

      config_free(&handle_ptr->asr_config);

      handle_ptr->in_use = false;

      return 0;

   } catch(const std::exception& e) {
      std::cerr << e.what();
      return -1;
   }
}

/* locate Kaldi file, currently we look for local development folder first, then SigSRF executable and lib folders, JHB Feb2021

   -input: Kaldi file, including subfolder path
   -output: actual path location (tmpstr)
   -return value: true if found, false if not
   -added install_path option, resolving relative paths to absolute, JHB Apr2022
*/

bool find_kaldi_file(char* full_path, char* install_path_user, const char* kaldi_file) {

struct stat buffer;
char install_path_local[256], rel_path[256];
char* install_path = install_path_user ? install_path_user : install_path_local;

/* check for development folder first, if not found then check for executable and lib folders */

   strcpy(install_path, "/storage");
   sprintf(full_path, "%s/%s", install_path, kaldi_file);

   if (stat(full_path, &buffer)) {  /* stat() returns 0 if file exists */

      strcpy(rel_path, "../../../../..");
      if (!realpath(rel_path, install_path)) {
         Log_RT(3, "WARNING: DSASRConfig() says exe folder realpath() error = %d \n", errno);
      }
      #ifdef HARDCODED_CONF_FILE_DEBUG
      else printf(" exe folder resolved path = %s \n", install_path);
      #endif
      sprintf(full_path, "%s/%s", install_path, kaldi_file);  /* try executable folder */

      if (stat(full_path, &buffer)) {

         strcpy(rel_path, "../../..");
         if (!realpath(rel_path, install_path)) {
            Log_RT(3, "WARNING: DSASRConfig() says lib folder realpath() error = %d \n", errno);
         }
         #ifdef HARDCODED_CONF_FILE_DEBUG
         else printf(" lib folder resolved path = %s \n", install_path);
         #endif
         sprintf(full_path, "%s/%s", install_path, kaldi_file);  /* try lib folder */

         if (stat(full_path, &buffer)) {

         /* note - we could also try the "SIGNALOGIC_INSTALL_PATH=" field inside /etc/environment file, which is added by install script, e.g:

              SIGNALOGIC_INSTALL_PATH=/home/sigsrf_sdk_demo

            but for now we're not doing that, JHB Apr2022
         */

            Log_RT(2, "ERROR: DSASRConfig() says cannot locate Kaldi file %s either on local development folder or SDK install folder \n", kaldi_file);
            return false;
         }
      }
   }

   return true;
}

/* Notes

     -strdup() mallocs are freed in config_free(), which is called by DSASRDelete()
     -for uninitialized config items use hardcoded values for now, eventually add methods (or combination) (i) pass additional params to DSASRConfig(), (ii) parse a config file
*/

int DSASRConfig(ASR_CONFIG* config, unsigned int uFlags, const char* utterance_id, int sample_rate) {

/* Kaldi files we need, located either on local development folder, or SDK install folder, JHB Feb2021 */

char mfcc_conf_str[] = "kaldi/egs/mini_librispeech/s5/exp/chain/tdnn1h_sp_online/conf/mfcc.conf";
char ivector_conf_str[] = "kaldi/egs/mini_librispeech/s5/exp/chain/tdnn1h_sp_online/conf/ivector_extractor.conf";
char mdl_str[] = "kaldi/egs/mini_librispeech/s5/exp/chain/tdnn1h_sp_online/final.mdl";
char fst_str[] = "kaldi/egs/mini_librispeech/s5/exp/chain/tree_sp/graph_tgsmall/HCLG.fst";
char txt_str[] = "kaldi/egs/mini_librispeech/s5/exp/chain/tree_sp/graph_tgsmall/words.txt";

char install_dummy[] = "/home/labuser/Signalogic";

/* /home/sigsrf_sdk_demo/Signalogic_2020v8/kaldi/egs/mini_librispeech/s5/exp/chain/tdnn1h_sp_online/conf/ivector_extractor.conf */

char full_path[256], install_path[256], full_path_temp[256], line[256], line2[256];
FILE *fconf = NULL, *fconf_temp = NULL;

   if (!config) return -1;  /* error condition */

   if (!config->feature_type) config->feature_type = strdup("mfcc");

   if (!find_kaldi_file(full_path, NULL, mfcc_conf_str)) return -1;
   if (!config->mfcc_config) config->mfcc_config = strdup(full_path); //"/storage/kaldi/egs/mini_librispeech/s5/exp/chain/tdnn1h_sp_online/conf/mfcc.conf");

   if (!find_kaldi_file(full_path, install_path, ivector_conf_str)) return -1;
   if (!config->ivector_config) config->ivector_config = strdup(full_path); //"/storage/kaldi/egs/mini_librispeech/s5/exp/chain/tdnn1h_sp_online/conf/ivector_extractor.conf");

/* ivector_extractor.conf file requires special handling. This file contains several hardcoded paths needed by Kaldi libs, JHB Apr2022:

  -in the .conf file installed by SDK/demo .rar packages, paths start with a "/home/labuser/Signalogic" dummy, which we detect in order to replace with actual install path on live systems
  -this should only occur once: first time ASR is invoked after a fresh install. After that we check for the dummy each time an app runs and first initializes ASR but we won't find it
  -we use path install_path returned by find_kaldi_file(), which uses realpath() to eliminate any relative path syntax

  Additional notes:
  
    -another option would be to do this in the install script, but that doesn't cover cases where users are moving files around or otherwise changing their install path
    -later versions of Kaldi may have ways to specify these paths directly/individually, instead of using a .conf file. Several Kaldi paths (see above) are already handled separately from any .conf file
    -examples of files inside ivector_extractor.conf needed by Kaldi libs: splice.conf, online_cmvn.conf, final.mat
*/

   #ifdef HARDCODED_CONF_FILE_DEBUG
   printf(" *******inferlib, opening ivector config file %s \n", full_path);
   #endif

   if ((fconf = fopen(full_path, "r"))) {

      #ifdef HARDCODED_CONF_FILE_DEBUG
      printf(" ******* inside conf file read \n");
      #endif

      sprintf(full_path_temp, "%s_temp", full_path);
      fconf_temp = fopen(full_path_temp, "w");
 
      bool fDummy_found = false;
      char* p;

      while (fgets(line, sizeof(line), fconf)) {  /* read all lines of .conf file */

         #ifdef HARDCODED_CONF_FILE_DEBUG
         printf(" ******* conf file line = %s \n", line);
         #endif

         if ((p = strstr(line, install_dummy))) {

            *p = 0;
            strcpy(line2, line);  /* copy first part of line */
            strcat(line2, install_path);  /* add install path */
            strcat(line2, p + strlen(install_dummy));  /* add remainder of original line */
            strcpy(line, line2);

            #ifdef HARDCODED_CONF_FILE_DEBUG
            printf(" ******* conf file modified line = %s \n", line);
            #endif

            fDummy_found = true;
         }

         fwrite(line, strlen(line), 1, fconf_temp);  /* write out either original line or modified line */
      }

      #ifdef HARDCODED_CONF_FILE_DEBUG
      printf(" *******after checking for dummy install, dummy found = %d, temp conf path = %s \n", fDummy_found, full_path_temp);
      #endif
      
      if (fconf) fclose(fconf);
      if (fconf_temp) fclose(fconf_temp);

      if (fDummy_found) {  /* if any replacements made then delete the original .conf file and rename the temp file */

         remove(full_path);  /* remove original conf file */
         rename(full_path_temp, full_path);  /* rename temp file */
      }
      else remove(full_path_temp);  /* otherwise if no replacements just remove the temp file */
   }

   if (!config->frame_subsampling_factor) config->frame_subsampling_factor = 3;
   if (!config->acoustic_scale) config->acoustic_scale = 1.0;
   if (!config->beam) config->beam = 15.0;
   if (!config->max_active) config->max_active = 7000;
   if (!config->lattice_beam) config->lattice_beam = 6.0;
   if (!config->silence_phones) config->silence_phones = strdup("1:2:3:4:5:6:7:8:9:10");

   if (!find_kaldi_file(full_path, NULL, mdl_str)) return -1;
   if (!config->nnet3_rxfilename) config->nnet3_rxfilename = strdup(full_path); //"/storage/kaldi/egs/mini_librispeech/s5/exp/chain/tdnn1h_sp_online/final.mdl");

   if (!find_kaldi_file(full_path, NULL, fst_str)) return -1;
   if (!config->fst_rxfilename) config->fst_rxfilename = strdup(full_path); //"/storage/kaldi/egs/mini_librispeech/s5/exp/chain/tree_sp/graph_tgsmall/HCLG.fst");

   if (!find_kaldi_file(full_path, NULL, txt_str)) return -1;
   if (!config->word_syms_filename) config->word_syms_filename = strdup(full_path); //"/storage/kaldi/egs/mini_librispeech/s5/exp/chain/tree_sp/graph_tgsmall/words.txt");

   if (uFlags & DS_ASR_CONFIG_DO_ENDPOINTING) config->do_endpointing = true;  /* default value is false, DSSessionCreate() in pktlik calls DSASRConfig() without this flag */
   if (uFlags & DS_ASR_CONFIG_ONLINE) config->online = true;  /* should be true for real-time operation, DSSessionCreate() in pktlib calls DSASRConfig() with this flag */

   if (!config->utterance_id) {

      if (!utterance_id || !strlen(utterance_id)) config->utterance_id = strdup("test_utterance");
      else config->utterance_id = strdup(utterance_id);  /* utterance string - basically a name for input audio, used when labeling timing stats output */
   }

   if (!config->samp_freq) config->samp_freq = sample_rate;

   return 1;
}


/* wrapper Functions */

HASRDECODER DSASRCreate(ASR_CONFIG* asr_config) {return SigOnline2WavNnet3LatgenFasterInit(asr_config);}
int DSASRProcess(HASRDECODER handle, float* data, int length) {return SigOnline2WavNnet3LatgenFasterProcess(handle, data, length);}
int DSASRGetText(HASRDECODER handle, unsigned int uFlags) {return SigOnline2WavNnet3LatgenFasterGetText(handle, uFlags);}
int DSASRFinalize(HASRDECODER handle) {return SigOnline2WavNnet3LatgenFasterFinalize(handle);}
int DSASRDelete(HASRDECODER handle) {return SigOnline2WavNnet3LatgenFasterClose(handle);}
