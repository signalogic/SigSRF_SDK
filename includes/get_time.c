/*
 $Header: /root/Signalogic/DirectCore/include/get_time.c

 Copyright (C) Signalogic Inc. 1994-2024

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  source for get_time() function in hwlib.h

 Projects

  SigSRF, DirectCore

 Revision History

  Created May 2024 JHB, moved here from hwlib.h
*/

  uint64_t ret_val;

   #ifdef MONITOR_TSC_INTEGRITY
   extern bool fRDTSCPSupported;  /* global var in hwlib */

   static int64_t prev_rdtsc = 0;
   static unsigned int prev_core_id = 0;
   int64_t rdtsc1, rdtsc2, rdtsc3 = 0, slip1 = 0, slip2 = 0;

   if (fRDTSCPSupported) {
      unsigned int dummy;
      #ifdef USE_X86INTRIN
      rdtsc1 = __rdtscp(&dummy);
      #else
      rdtsc1 = __builtin_ia32_rdtscp(&dummy);
      #endif
   }
   else {
      #ifdef USE_X86INTRIN
      rdtsc1 = __rdtsc();
      #else
      rdtsc1 = __builtin_ia32_rdtsc();
      #endif
    }
   #endif

   if (uFlags == USE_CLOCK_GETTIME) {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      ret_val = ts.tv_sec * 1000000L + ts.tv_nsec/1000;
   }
   else {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      ret_val = tv.tv_sec * 1000000L + tv.tv_usec;
   }

   #ifdef MONITOR_TSC_INTEGRITY
   unsigned int core_id;

   if (fRDTSCPSupported) {
      #ifdef USE_X86INTRIN
      rdtsc2 = __rdtscp(&core_id);
      #else
      rdtsc2 = __builtin_ia32_rdtscp(&core_id);
      #endif
      if (rdtsc2 <= rdtsc1) slip1 = rdtsc2 - rdtsc1;
      core_id &= 0xff;  /* core id in lower 8 bits of TSC AUX register */
      if (rdtsc2 <= (rdtsc3 = __sync_fetch_and_add(&prev_rdtsc, 0)) && core_id == prev_core_id) slip2 = rdtsc2 - rdtsc3;  /* only compare same-core TSC reads, in case CPU does not support invariant TSC synchronized between cores (Sig lab servers with Xeon 2660 do not) */
   }
   else {
      #ifdef USE_X86INTRIN
      rdtsc2 = __rdtsc();
      #else
      rdtsc2 = __builtin_ia32_rdtsc();
      #endif
      if (rdtsc2 <= rdtsc1) slip1 = rdtsc2 - rdtsc1;
   }

   if (slip1 || slip2) Log_RT(3, "WARNING: get_time() reports TSC integrity / adjustment issue, time slip = %ld, context switch slip = %ld (cycles), r2 = %ld, r3 = %ld, core_id = %u, prev_core_id = %u \n", -slip1, -slip2, rdtsc2, rdtsc3, core_id, prev_core_id);

   if (fRDTSCPSupported) {
      __sync_lock_test_and_set(&prev_rdtsc, &rdtsc2);
      __sync_lock_test_and_set(&prev_core_id, &core_id);
   }
   #endif

   return ret_val;
