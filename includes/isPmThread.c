/*
 $Header: /root/Signalogic/DirectCore/include/isPmThread.c

 Copyright (C) Signalogic Inc. 1994-2024

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Description

  source for isPmThread() function in pktlib.h

 Projects

  SigSRF, DirectCore

 Revision History

  Created May 2024 JHB, moved here from pktlib.h
*/

int i;
bool fTest, fIsPmThread = false;

   if (hSession >= 0 && !sessions[hSession].threadid) return false;

   for (i=0; i<nPktMediaThreads; i++) {
   
      if (hSession >= 0) fTest = pthread_equal(sessions[hSession].threadid, packet_media_thread_info[i].threadid);  /* compare sessions's p/m thread Id with active p/m thread thread Ids */
      else fTest = pthread_equal(pthread_self(), packet_media_thread_info[i].threadid);  /* compare current thread with p/m thread Id */

      if (fTest) {
   
         fIsPmThread = true;  /* session belongs to a pkt/media thread */
         if (pThreadIndex) *pThreadIndex = i;  /* return thread index if if pointer non-NULL */
         break;
      }
   }

   return fIsPmThread;
